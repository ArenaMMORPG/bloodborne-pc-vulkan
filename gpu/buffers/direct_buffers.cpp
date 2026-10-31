// SPDX-License-Identifier: GPL-2.0-or-later
// V3.3: Direct DEVICE_LOCAL VkBuffer Lifecycle & 0-trap Allocator/Loader Bridge.
#include "buffers/direct_buffers.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <span>

#include "buffers/buffer_cache.h"
#include "common/alignment.h"
#include "core/memory.h"
#include "layer/bblayer_write_traps.h"
#include "render/direct_shaders.h"
#include "render/runtime.h"
#include "render/scheduler.h"
#include "shim/bbport_toggles.h"
#include "textures/texture_cache.h"
#include "vulkan/instance.h"
#include "../../our_cpp_100000_source/include/gx_direct_vulkan.h"

extern "C" void runtime_memory_note_write(uintptr_t address, uint64_t size);
extern "C" bool BbLookupMappedEnd(u64 address, uintptr_t& out_end);

namespace VideoCore::DirectBuffers {

namespace {

std::atomic<BufferCache*> g_active_buf_cache{nullptr};
std::atomic<TextureCache*> g_active_tex_cache{nullptr};
std::atomic<u32> g_pending_count{0};
std::atomic<bool> g_ever_registered{false};
std::atomic<VAddr> g_cue_heap_base{0};
std::atomic<VAddr> g_cue_heap_end{0};
std::atomic<bool> g_cue_heap_gpu_modified{false};
std::mutex g_queue_mutex;
std::vector<Event> g_queue;

// Guest-side lock-free 64 KiB page bitmap tracking pages that currently hold (or held since last
// retire) a registered DirectImmutableBuf. Filters ~27,000/s transient ring alloc/free calls down
// to ~0 mutex acquisitions while guaranteeing immediate retirement on any address reuse.
std::array<std::atomic<u64>, Manager::kBitmapWords> g_guest_immutable_64k{};

void MarkGuestImmutableRange(VAddr addr, u64 size) noexcept {
    if (size == 0 || addr >= Manager::kMaxGuestVa) {
        return;
    }
    const VAddr end = std::min<VAddr>(addr + size, Manager::kMaxGuestVa);
    if (addr >= end) {
        return;
    }
    const u64 first_page = addr >> Manager::kPageShift64K;
    const u64 last_page = (end - 1ULL) >> Manager::kPageShift64K;
    for (u64 page = first_page; page <= last_page; ++page) {
        g_guest_immutable_64k[page >> 6].fetch_or(1ULL << (page & 63ULL),
                                                  std::memory_order_release);
    }
}

[[nodiscard]] bool GuestImmutableIntersects(VAddr addr, u64 size) noexcept {
    if (!g_ever_registered.load(std::memory_order_acquire) || size == 0 ||
        addr >= Manager::kMaxGuestVa) {
        return false;
    }
    const VAddr end = std::min<VAddr>(addr + size, Manager::kMaxGuestVa);
    if (addr >= end) {
        return false;
    }
    const u64 first_page = addr >> Manager::kPageShift64K;
    const u64 last_page = (end - 1ULL) >> Manager::kPageShift64K;
    const u64 first_word = first_page >> 6;
    const u64 last_word = last_page >> 6;
    if (first_word == last_word) {
        const u64 lo_mask = ~0ULL << (first_page & 63ULL);
        const u64 hi_mask = ~0ULL >> (63ULL - (last_page & 63ULL));
        return (g_guest_immutable_64k[first_word].load(std::memory_order_acquire) &
                (lo_mask & hi_mask)) != 0ULL;
    }
    const u64 lo_mask = ~0ULL << (first_page & 63ULL);
    if ((g_guest_immutable_64k[first_word].load(std::memory_order_acquire) & lo_mask) != 0ULL) {
        return true;
    }
    for (u64 w = first_word + 1; w < last_word; ++w) {
        if (g_guest_immutable_64k[w].load(std::memory_order_acquire) != 0ULL) {
            return true;
        }
    }
    const u64 hi_mask = ~0ULL >> (63ULL - (last_page & 63ULL));
    return (g_guest_immutable_64k[last_word].load(std::memory_order_acquire) & hi_mask) != 0ULL;
}

void ClearWholeGuestImmutablePages(VAddr addr, u64 size) noexcept {
    if (size < (1ULL << Manager::kPageShift64K) || addr >= Manager::kMaxGuestVa) {
        return;
    }
    const VAddr end = std::min<VAddr>(addr + size, Manager::kMaxGuestVa);
    const u64 whole_first =
        (addr + (1ULL << Manager::kPageShift64K) - 1ULL) >> Manager::kPageShift64K;
    const u64 whole_end = end >> Manager::kPageShift64K;
    for (u64 page = whole_first; page < whole_end; ++page) {
        g_guest_immutable_64k[page >> 6].fetch_and(~(1ULL << (page & 63ULL)),
                                                   std::memory_order_release);
    }
}

void PushEvent(const Event& ev) {
    std::scoped_lock lock{g_queue_mutex};
    if (g_queue.capacity() < 2048) {
        g_queue.reserve(2048);
    }
    g_queue.push_back(ev);
    g_pending_count.fetch_add(1, std::memory_order_release);
}

[[nodiscard]] constexpr bool RangesOverlap(VAddr a_start, u64 a_size,
                                           VAddr b_start, u64 b_size) noexcept {
    if (a_size == 0 || b_size == 0) {
        return false;
    }
    const VAddr a_end = a_start + a_size;
    const VAddr b_end = b_start + b_size;
    return a_start < b_end && b_start < a_end;
}

// V7.1: Lock-free 4 KiB page directory over the up-to-512 MiB transient cueHeap (131,072 x 4B = 512 KiB).
// Stores the relative byte offset (from g_cue_heap_base) of the end of the highest live
// CGGpuFreeListAllocatorGnm block covering each 4 KiB page, allowing O(1) lock-free bounding of
// 0xFFFFFFFF unbounded V# shader/vertex descriptors inside cueHeap instead of clamping to 480+ MiB.
constexpr u64 kMaxCueHeapBytes = 512ULL << 20;
constexpr u32 kCuePageShift = 12u;
constexpr u64 kCuePageSize = 1ULL << kCuePageShift;
constexpr size_t kMaxCueHeapPages = static_cast<size_t>(kMaxCueHeapBytes >> kCuePageShift);
constexpr u64 kMinBigCueBlockBytes = 64ULL << 10;
constexpr u64 kMaxBigCueBlockBytes = 4ULL << 20;
std::array<std::atomic<u32>, kMaxCueHeapPages> g_cue_page_end_rel{};
std::array<std::atomic<u32>, kMaxCueHeapPages> g_cue_page_big_start_p1{};
std::array<std::atomic<u32>, kMaxCueHeapPages> g_cue_page_big_end{};

inline void AtomicMaxU32(std::atomic<u32>& cell, u32 val) noexcept {
    u32 cur = cell.load(std::memory_order_relaxed);
    while (cur < val && !cell.compare_exchange_weak(cur, val, std::memory_order_release,
                                                    std::memory_order_relaxed)) {
    }
}

// N1.1: Global 16 GiB lock-free 4 KiB page directory over [0x1000000000, 0x1400000000)
// (4,194,304 x 4 B = 16 MiB BSS). Stores `(rel_end + 3) >> 2` (4-byte precision, never overflows
// u32) for every live GXGnmBuffer (DEFAULT/IMMUTABLE/DYNAMIC/STAGING) and CGGpuFreeListAllocatorGnm
// block so unbounded 0xFFFFFFFF V# vertex/shader streams clamp to their exact buffer instead of
// pulling 2.5..4.1 GiB mmaps into BufferCache + mprotect(PROT_READ).
constexpr VAddr kNativeDirBase = 0x1000000000ULL;
constexpr u64 kNativeDirSpan = 16ULL << 30;
constexpr u64 kNativeDirMaxBytes = kNativeDirSpan - 4096ULL;
constexpr u32 kNativePageShift = 12u;
constexpr u64 kNativePageSize = 1ULL << kNativePageShift;
constexpr size_t kNativeDirPages = static_cast<size_t>(kNativeDirSpan >> kNativePageShift);
constexpr u64 kNativeMaxSingleBlockBytes = 128ULL << 20; // ignore whole-heap container carve-outs
std::array<std::atomic<u32>, kNativeDirPages> g_native_page_end_rel4{};

// N1.2c: Lossless lock-free 4-byte-granule NativeOwnedBitmap over [0x1000000000, 0x1400000000)
// (67,108,864 u64 words; Linux demand-pages only touched ~2.5 GiB buffer ranges).
// Guarantees 0 hash collisions, 0 evictions, 0 4-byte/16-byte neighbor false positives, and O(1)
// single-load detection for both `gpuBaseAddr` and interior `gpuBaseAddr + byteOffset` views.
constexpr u32 kNativeOwnedGranuleShift = 2u; // 4 bytes per bit
constexpr u32 kNativeOwnedWordShift = kNativeOwnedGranuleShift + 6u; // 256 bytes per u64 word
constexpr size_t kNativeOwnedWords = static_cast<size_t>(kNativeDirSpan >> kNativeOwnedWordShift);
std::array<std::atomic<u64>, kNativeOwnedWords> g_native_owned_4b{};

// N1.2 / N1.2c: 262,144-slot (32,768 sets x 8 ways + 4-set linear probe = 32 candidate ways per
// address, 4 MiB BSS) Murmur3-avalanched lock-free ExactBufferTable keyed by exact
// `GXGnmBuffer::gpuBaseAddr` (always >= 4-byte aligned, so bit 0 of `addr_and_flag` stores
// `is_native_owned`, and `sizes_packed` stores `(u64(alloc_size) << 32) | u64(req_size)`).
constexpr size_t kExactBufSets = 32768;
constexpr size_t kExactBufWays = 8;
constexpr size_t kExactBufProbeSets = 4;
struct ExactBufSlot {
    std::atomic<u64> addr_and_flag{0};
    std::atomic<u64> sizes_packed{0};
};
std::array<std::array<ExactBufSlot, kExactBufWays>, kExactBufSets> g_exact_buf_table{};
std::atomic<u32> g_exact_rr_hand{0};

[[nodiscard]] constexpr size_t ExactBufSetIdx(VAddr addr) noexcept {
    u64 h = addr >> 2;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return static_cast<size_t>(h & (kExactBufSets - 1));
}

} // namespace

void SetActiveCaches(BufferCache* buf_cache, TextureCache* tex_cache) noexcept {
    g_active_buf_cache.store(buf_cache, std::memory_order_release);
    g_active_tex_cache.store(tex_cache, std::memory_order_release);
    if (!buf_cache) {
        std::scoped_lock lock{g_queue_mutex};
        g_queue.clear();
        g_pending_count.store(0, std::memory_order_release);
        g_ever_registered.store(false, std::memory_order_release);
        for (auto& word : g_guest_immutable_64k) {
            if (word.load(std::memory_order_relaxed) != 0ULL) {
                word.store(0ULL, std::memory_order_relaxed);
            }
        }
        for (auto& word : g_native_owned_4b) {
            if (word.load(std::memory_order_relaxed) != 0ULL) {
                word.store(0ULL, std::memory_order_relaxed);
            }
        }
        for (auto& cell : g_native_page_end_rel4) {
            if (cell.load(std::memory_order_relaxed) != 0u) {
                cell.store(0u, std::memory_order_relaxed);
            }
        }
        for (auto& cell : g_cue_page_big_start_p1) {
            if (cell.load(std::memory_order_relaxed) != 0u) {
                cell.store(0u, std::memory_order_relaxed);
            }
        }
        for (auto& cell : g_cue_page_big_end) {
            if (cell.load(std::memory_order_relaxed) != 0u) {
                cell.store(0u, std::memory_order_relaxed);
            }
        }
        for (auto& set : g_exact_buf_table) {
            for (auto& way : set) {
                way.addr_and_flag.store(0ULL, std::memory_order_relaxed);
                way.sizes_packed.store(0ULL, std::memory_order_relaxed);
            }
        }
    }
}

bool HasPendingEvents() noexcept {
    return g_pending_count.load(std::memory_order_acquire) != 0;
}

void RegisterTransientCueHeap(VAddr addr, u64 byte_size) noexcept {
    if (!addr || byte_size == 0) {
        return;
    }
    g_cue_heap_base.store(addr, std::memory_order_relaxed);
    g_cue_heap_end.store(addr + byte_size, std::memory_order_release);
}

bool IsTransientCueHeap(VAddr addr, u64 byte_size) noexcept {
    static const bool enabled =
        gx_direct_vk::EnabledV6Sub(gx_direct_vk::kV6Sub_ZeroFaultCueStaging);
    if (!enabled || byte_size == 0) {
        return false;
    }
    const VAddr end = g_cue_heap_end.load(std::memory_order_acquire);
    if (end == 0) {
        return false;
    }
    const VAddr base = g_cue_heap_base.load(std::memory_order_relaxed);
    return addr >= base && addr < end && (end - addr) >= byte_size;
}

bool OverlapsTransientCueHeap(VAddr addr, u64 byte_size) noexcept {
    if (byte_size == 0) {
        return false;
    }
    const VAddr end = g_cue_heap_end.load(std::memory_order_acquire);
    if (end == 0) {
        return false;
    }
    const VAddr base = g_cue_heap_base.load(std::memory_order_relaxed);
    return RangesOverlap(addr, byte_size, base, end - base);
}

void NoteTransientCueHeapGpuWrite(VAddr addr, u64 byte_size) noexcept {
    if (!g_cue_heap_gpu_modified.load(std::memory_order_relaxed) &&
        OverlapsTransientCueHeap(addr, byte_size)) [[unlikely]] {
        g_cue_heap_gpu_modified.store(true, std::memory_order_release);
    }
}

bool IsCueHeapEverGpuModified() noexcept {
    return g_cue_heap_gpu_modified.load(std::memory_order_acquire);
}

void NoteTransientCueHeapAlloc(VAddr addr, u64 byte_size) noexcept {
    if (byte_size == 0) {
        return;
    }
    const VAddr end = g_cue_heap_end.load(std::memory_order_acquire);
    if (end == 0) {
        return;
    }
    const VAddr base = g_cue_heap_base.load(std::memory_order_relaxed);
    if (addr < base || addr >= end) {
        return;
    }
    const u64 max_span = std::min<u64>(end - base, kMaxCueHeapBytes);
    const u64 rel_start64 = addr - base;
    if (rel_start64 >= max_span) {
        return;
    }
    const u64 rel_end64 = std::min<u64>(rel_start64 + byte_size, max_span);
    if (rel_end64 <= rel_start64) {
        return;
    }
    const u32 rel_start = static_cast<u32>(rel_start64);
    const u32 rel_end = static_cast<u32>(rel_end64);
    const size_t first_p = static_cast<size_t>(rel_start >> kCuePageShift);
    const size_t last_p = static_cast<size_t>((rel_end - 1u) >> kCuePageShift);
    if (first_p == last_p) {
        if ((rel_start & (kCuePageSize - 1u)) == 0u && (rel_end & (kCuePageSize - 1u)) == 0u) {
            g_cue_page_end_rel[first_p].store(rel_end, std::memory_order_release);
        } else {
            AtomicMaxU32(g_cue_page_end_rel[first_p], rel_end);
        }
        return;
    }
    if ((rel_start & (kCuePageSize - 1u)) == 0u) {
        g_cue_page_end_rel[first_p].store(rel_end, std::memory_order_release);
    } else {
        AtomicMaxU32(g_cue_page_end_rel[first_p], rel_end);
    }
    for (size_t p = first_p + 1u; p < last_p; ++p) {
        g_cue_page_end_rel[p].store(rel_end, std::memory_order_release);
    }
    if ((rel_end & (kCuePageSize - 1u)) == 0u) {
        g_cue_page_end_rel[last_p].store(rel_end, std::memory_order_release);
    } else {
        AtomicMaxU32(g_cue_page_end_rel[last_p], rel_end);
    }
    if (byte_size > kMinBigCueBlockBytes && byte_size <= kMaxBigCueBlockBytes) {
        const u32 start_p1 = rel_start + 1u;
        for (size_t p = first_p; p < last_p; ++p) {
            g_cue_page_big_end[p].store(rel_end, std::memory_order_relaxed);
            g_cue_page_big_start_p1[p].store(start_p1, std::memory_order_release);
        }
        if ((rel_end & (kCuePageSize - 1u)) == 0u ||
            g_cue_page_big_start_p1[last_p].load(std::memory_order_relaxed) == 0u) {
            g_cue_page_big_end[last_p].store(rel_end, std::memory_order_relaxed);
            g_cue_page_big_start_p1[last_p].store(start_p1, std::memory_order_release);
        }
    }
}

void NoteTransientCueHeapFree(VAddr addr, u64 byte_size) noexcept {
    if (byte_size == 0) {
        return;
    }
    const VAddr end = g_cue_heap_end.load(std::memory_order_acquire);
    if (end == 0) {
        return;
    }
    const VAddr base = g_cue_heap_base.load(std::memory_order_relaxed);
    if (addr < base || addr >= end) {
        return;
    }
    const u64 max_span = std::min<u64>(end - base, kMaxCueHeapBytes);
    const u64 rel_start64 = addr - base;
    if (rel_start64 >= max_span) {
        return;
    }
    const u64 rel_end64 = std::min<u64>(rel_start64 + byte_size, max_span);
    if (rel_end64 <= rel_start64) {
        return;
    }
    const u32 rel_start = static_cast<u32>(rel_start64);
    const u32 rel_end = static_cast<u32>(rel_end64);
    if (byte_size > kMinBigCueBlockBytes && byte_size <= kMaxBigCueBlockBytes) {
        const size_t first_p = static_cast<size_t>(rel_start >> kCuePageShift);
        const size_t last_p = static_cast<size_t>((rel_end - 1u) >> kCuePageShift);
        const u32 start_p1 = rel_start + 1u;
        for (size_t p = first_p; p <= last_p; ++p) {
            u32 expected = start_p1;
            g_cue_page_big_start_p1[p].compare_exchange_strong(
                expected, 0u, std::memory_order_release, std::memory_order_relaxed);
        }
    }
    const u64 full_begin = (rel_start64 + kCuePageSize - 1ULL) & ~(kCuePageSize - 1ULL);
    const u64 full_end = rel_end64 & ~(kCuePageSize - 1ULL);
    if (full_begin < full_end) {
        const size_t p_begin = static_cast<size_t>(full_begin >> kCuePageShift);
        const size_t p_end = static_cast<size_t>(full_end >> kCuePageShift);
        for (size_t p = p_begin; p < p_end; ++p) {
            g_cue_page_end_rel[p].store(0u, std::memory_order_release);
        }
    }
    if ((rel_start & (kCuePageSize - 1u)) != 0u && rel_end64 >= full_begin) {
        const size_t head_p = static_cast<size_t>(rel_start >> kCuePageShift);
        u32 expected = rel_end;
        g_cue_page_end_rel[head_p].compare_exchange_strong(
            expected, rel_start, std::memory_order_release, std::memory_order_relaxed);
    }
}

u64 ClampTransientCueReadSize(VAddr addr, u64 requested_size) noexcept {
    static const bool enabled =
        gx_direct_vk::EnabledV6Sub(gx_direct_vk::kV6Sub_ZeroFaultCueStaging) &&
        gx_direct_vk::EnabledV7Sub(gx_direct_vk::kV7Sub_BoundedCueAndQueries);
    if (!enabled || requested_size == 0) {
        return requested_size;
    }
    const VAddr end = g_cue_heap_end.load(std::memory_order_acquire);
    if (end == 0) {
        return requested_size;
    }
    const VAddr base = g_cue_heap_base.load(std::memory_order_relaxed);
    if (addr < base || addr >= end) {
        return requested_size;
    }
    const u64 heap_rem = end - addr;
    u64 clamped = std::min<u64>(requested_size, heap_rem);
    const u64 rel = addr - base;
    if (rel < kMaxCueHeapBytes) {
        const u32 end_rel =
            g_cue_page_end_rel[static_cast<size_t>(rel >> kCuePageShift)].load(
                std::memory_order_acquire);
        if (end_rel > rel) {
            clamped = std::min<u64>(clamped, static_cast<u64>(end_rel) - rel);
        }
    }
    return clamped;
}

bool LookupTransientCueBlock(VAddr addr, u64 size, VAddr& out_block_base,
                             u32& out_block_size) noexcept {
    if (size == 0) {
        return false;
    }
    const VAddr end = g_cue_heap_end.load(std::memory_order_acquire);
    if (end == 0) {
        return false;
    }
    const VAddr base = g_cue_heap_base.load(std::memory_order_relaxed);
    if (addr < base || addr >= end) {
        return false;
    }
    const u64 rel = addr - base;
    if (rel >= kMaxCueHeapBytes || rel + size > std::min<u64>(end - base, kMaxCueHeapBytes)) {
        return false;
    }
    const auto try_page = [&](size_t p) noexcept -> bool {
        const u32 s_p1 = g_cue_page_big_start_p1[p].load(std::memory_order_acquire);
        if (s_p1 == 0u) {
            return false;
        }
        const u64 b_start = static_cast<u64>(s_p1 - 1u);
        const u64 b_end = static_cast<u64>(g_cue_page_big_end[p].load(std::memory_order_relaxed));
        if (b_end > b_start && (b_end - b_start) <= kMaxBigCueBlockBytes && rel >= b_start &&
            rel + size <= b_end) {
            out_block_base = base + b_start;
            out_block_size = static_cast<u32>(b_end - b_start);
            return true;
        }
        return false;
    };
    const size_t cur_p = static_cast<size_t>(rel >> kCuePageShift);
    if (try_page(cur_p)) {
        return true;
    }
    return cur_p > 0u && try_page(cur_p - 1u);
}

void NoteNativeGpuBlockAlloc(VAddr addr, u64 byte_size) noexcept {
    if (byte_size == 0 || byte_size > kNativeMaxSingleBlockBytes || addr < kNativeDirBase) {
        return;
    }
    const u64 rel_start = addr - kNativeDirBase;
    if (rel_start >= kNativeDirMaxBytes) {
        return;
    }
    const u64 rel_end = std::min<u64>(rel_start + byte_size, kNativeDirMaxBytes);
    if (rel_end <= rel_start) {
        return;
    }
    const u32 end_rel4 = static_cast<u32>((rel_end + 3ULL) >> 2);
    const size_t first_p = static_cast<size_t>(rel_start >> kNativePageShift);
    const size_t last_p = static_cast<size_t>((rel_end - 1ULL) >> kNativePageShift);
    if (first_p == last_p) {
        if ((rel_start & (kNativePageSize - 1ULL)) == 0ULL &&
            (rel_end & (kNativePageSize - 1ULL)) == 0ULL) {
            g_native_page_end_rel4[first_p].store(end_rel4, std::memory_order_release);
        } else {
            AtomicMaxU32(g_native_page_end_rel4[first_p], end_rel4);
        }
        return;
    }
    if ((rel_start & (kNativePageSize - 1ULL)) == 0ULL) {
        g_native_page_end_rel4[first_p].store(end_rel4, std::memory_order_release);
    } else {
        AtomicMaxU32(g_native_page_end_rel4[first_p], end_rel4);
    }
    for (size_t p = first_p + 1u; p < last_p; ++p) {
        g_native_page_end_rel4[p].store(end_rel4, std::memory_order_release);
    }
    if ((rel_end & (kNativePageSize - 1ULL)) == 0ULL) {
        g_native_page_end_rel4[last_p].store(end_rel4, std::memory_order_release);
    } else {
        AtomicMaxU32(g_native_page_end_rel4[last_p], end_rel4);
    }
}

void MarkNativeOwnedRange(VAddr addr, u64 byte_size) noexcept {
    if (byte_size == 0 || addr >= kNativeDirBase + kNativeDirMaxBytes) {
        return;
    }
    const VAddr end_va = addr + byte_size;
    if (end_va <= kNativeDirBase) {
        return;
    }
    const u64 rel_start = (addr > kNativeDirBase) ? (addr - kNativeDirBase) : 0ULL;
    const u64 rel_end = std::min<u64>(end_va - kNativeDirBase, kNativeDirMaxBytes);
    if (rel_end <= rel_start) {
        return;
    }
    const u64 first_bit = rel_start >> kNativeOwnedGranuleShift;
    const u64 end_bit = (rel_end + ((1ULL << kNativeOwnedGranuleShift) - 1ULL)) >>
                        kNativeOwnedGranuleShift;
    if (end_bit <= first_bit) {
        return;
    }
    const u64 last_bit = end_bit - 1ULL;
    const u64 first_word = first_bit >> 6;
    const u64 last_word = last_bit >> 6;
    const u64 lo_mask = ~0ULL << (first_bit & 63ULL);
    const u64 hi_mask = ~0ULL >> (63ULL - (last_bit & 63ULL));
    if (first_word == last_word) {
        g_native_owned_4b[first_word].fetch_or(lo_mask & hi_mask, std::memory_order_release);
        return;
    }
    g_native_owned_4b[first_word].fetch_or(lo_mask, std::memory_order_release);
    for (u64 w = first_word + 1ULL; w < last_word; ++w) {
        g_native_owned_4b[w].store(~0ULL, std::memory_order_release);
    }
    g_native_owned_4b[last_word].fetch_or(hi_mask, std::memory_order_release);
}

void ClearNativeOwnedRange(VAddr addr, u64 byte_size) noexcept {
    if (byte_size == 0 || addr >= kNativeDirBase + kNativeDirMaxBytes) {
        return;
    }
    const VAddr end_va = addr + byte_size;
    if (end_va <= kNativeDirBase) {
        return;
    }
    const u64 rel_start = (addr > kNativeDirBase) ? (addr - kNativeDirBase) : 0ULL;
    const u64 rel_end = std::min<u64>(end_va - kNativeDirBase, kNativeDirMaxBytes);
    if (rel_end <= rel_start) {
        return;
    }
    const u64 first_bit = rel_start >> kNativeOwnedGranuleShift;
    const u64 end_bit = (rel_end + ((1ULL << kNativeOwnedGranuleShift) - 1ULL)) >>
                        kNativeOwnedGranuleShift;
    if (end_bit <= first_bit) {
        return;
    }
    const u64 last_bit = end_bit - 1ULL;
    const u64 first_word = first_bit >> 6;
    const u64 last_word = last_bit >> 6;
    const u64 lo_mask = ~0ULL << (first_bit & 63ULL);
    const u64 hi_mask = ~0ULL >> (63ULL - (last_bit & 63ULL));
    if (first_word == last_word) {
        g_native_owned_4b[first_word].fetch_and(~(lo_mask & hi_mask), std::memory_order_release);
        return;
    }
    g_native_owned_4b[first_word].fetch_and(~lo_mask, std::memory_order_release);
    for (u64 w = first_word + 1ULL; w < last_word; ++w) {
        g_native_owned_4b[w].store(0ULL, std::memory_order_release);
    }
    g_native_owned_4b[last_word].fetch_and(~hi_mask, std::memory_order_release);
}

bool IsNativeOwnedAddress(VAddr addr) noexcept {
    if (!g_ever_registered.load(std::memory_order_acquire) || addr == 0) {
        return false;
    }
    if (addr >= kNativeDirBase) {
        const u64 rel = addr - kNativeDirBase;
        if (rel < kNativeDirMaxBytes) {
            const u64 bit = rel >> kNativeOwnedGranuleShift;
            return (g_native_owned_4b[bit >> 6].load(std::memory_order_acquire) &
                    (1ULL << (bit & 63ULL))) != 0ULL;
        }
    }
    return IsExactNativeOwnedBuffer(addr);
}

bool OverlapsNativeOwnedRange(VAddr addr, u64 byte_size) noexcept {
    if (!g_ever_registered.load(std::memory_order_acquire) || byte_size == 0) {
        return false;
    }
    if (addr >= kNativeDirBase && addr < kNativeDirBase + kNativeDirMaxBytes) {
        const u64 rel_start = addr - kNativeDirBase;
        const u64 rel_end = std::min<u64>(rel_start + byte_size, kNativeDirMaxBytes);
        if (rel_end > rel_start) {
            const u64 first_bit = rel_start >> kNativeOwnedGranuleShift;
            const u64 last_bit =
                ((rel_end + ((1ULL << kNativeOwnedGranuleShift) - 1ULL)) >>
                 kNativeOwnedGranuleShift) -
                1ULL;
            const u64 first_word = first_bit >> 6;
            const u64 last_word = last_bit >> 6;
            const u64 lo_mask = ~0ULL << (first_bit & 63ULL);
            const u64 hi_mask = ~0ULL >> (63ULL - (last_bit & 63ULL));
            if (first_word == last_word) {
                return (g_native_owned_4b[first_word].load(std::memory_order_acquire) &
                        (lo_mask & hi_mask)) != 0ULL;
            }
            if ((g_native_owned_4b[first_word].load(std::memory_order_acquire) & lo_mask) != 0ULL) {
                return true;
            }
            for (u64 w = first_word + 1ULL; w < last_word; ++w) {
                if (g_native_owned_4b[w].load(std::memory_order_acquire) != 0ULL) {
                    return true;
                }
            }
            return (g_native_owned_4b[last_word].load(std::memory_order_acquire) & hi_mask) != 0ULL;
        }
    }
    return IsExactNativeOwnedBuffer(addr);
}

void NoteExactNativeBuffer(VAddr addr, u32 req_size, u32 alloc_size,
                           bool is_native_owned) noexcept {
    if (addr == 0 || (addr & 3ULL) != 0ULL || alloc_size == 0) {
        return;
    }
    const u32 safe_req = req_size > 0 && req_size <= alloc_size ? req_size : alloc_size;
    const u64 packed_sizes = (static_cast<u64>(alloc_size) << 32) | static_cast<u64>(safe_req);
    const u64 tagged_addr = (addr & ~3ULL) | (is_native_owned ? 1ULL : 0ULL);
    const size_t base_set = ExactBufSetIdx(addr);

    // 1. Update matching address in place across all probed sets if present.
    for (size_t p = 0; p < kExactBufProbeSets; ++p) {
        auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
        for (size_t w = 0; w < kExactBufWays; ++w) {
            const u64 cur = set[w].addr_and_flag.load(std::memory_order_relaxed);
            if ((cur & ~3ULL) == addr) {
                set[w].sizes_packed.store(packed_sizes, std::memory_order_relaxed);
                set[w].addr_and_flag.store(tagged_addr, std::memory_order_release);
                return;
            }
        }
    }
    // 2. Fill first empty way across the 32 probed candidate slots.
    for (size_t p = 0; p < kExactBufProbeSets; ++p) {
        auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
        for (size_t w = 0; w < kExactBufWays; ++w) {
            u64 expected = 0ULL;
            if (set[w].addr_and_flag.compare_exchange_strong(
                    expected, 2ULL, std::memory_order_acquire, std::memory_order_relaxed)) {
                set[w].sizes_packed.store(packed_sizes, std::memory_order_relaxed);
                set[w].addr_and_flag.store(tagged_addr, std::memory_order_release);
                return;
            }
        }
    }
    // 3. Fallback round-robin across all 32 candidate slots (virtually unreachable at <10% load).
    const u32 rr = g_exact_rr_hand.fetch_add(1u, std::memory_order_relaxed);
    auto& victim_set =
        g_exact_buf_table[(base_set + ((rr >> 3) & (kExactBufProbeSets - 1))) & (kExactBufSets - 1)];
    const size_t victim_way = static_cast<size_t>(rr & (kExactBufWays - 1));
    victim_set[victim_way].addr_and_flag.store(0ULL, std::memory_order_release);
    victim_set[victim_way].sizes_packed.store(packed_sizes, std::memory_order_relaxed);
    victim_set[victim_way].addr_and_flag.store(tagged_addr, std::memory_order_release);
}

void SetExactNativeBufferOwned(VAddr addr, bool is_native_owned) noexcept {
    if (addr == 0 || (addr & 3ULL) != 0ULL) {
        return;
    }
    const size_t base_set = ExactBufSetIdx(addr);
    const u64 tagged_addr = (addr & ~3ULL) | (is_native_owned ? 1ULL : 0ULL);
    for (size_t p = 0; p < kExactBufProbeSets; ++p) {
        auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
        for (size_t w = 0; w < kExactBufWays; ++w) {
            const u64 cur = set[w].addr_and_flag.load(std::memory_order_relaxed);
            if ((cur & ~3ULL) == addr) {
                set[w].addr_and_flag.store(tagged_addr, std::memory_order_release);
                return;
            }
        }
    }
}

bool IsExactNativeOwnedBuffer(VAddr addr) noexcept {
    if (addr == 0 || (addr & 3ULL) != 0ULL) {
        return false;
    }
    const size_t base_set = ExactBufSetIdx(addr);
    const u64 wanted = addr | 1ULL;
    for (size_t p = 0; p < kExactBufProbeSets; ++p) {
        const auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
        for (size_t w = 0; w < kExactBufWays; ++w) {
            if (set[w].addr_and_flag.load(std::memory_order_acquire) == wanted) {
                return true;
            }
        }
    }
    return false;
}

VAddr LookupExactBufferEnd(VAddr addr) noexcept {
    if (addr == 0 || (addr & 3ULL) != 0ULL) {
        return 0;
    }
    const size_t base_set = ExactBufSetIdx(addr);
    for (size_t p = 0; p < kExactBufProbeSets; ++p) {
        const auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
        for (size_t w = 0; w < kExactBufWays; ++w) {
            const u64 tag = set[w].addr_and_flag.load(std::memory_order_acquire);
            if ((tag & ~3ULL) == addr) {
                const u64 sizes = set[w].sizes_packed.load(std::memory_order_relaxed);
                const u32 alloc_sz = static_cast<u32>(sizes >> 32);
                return alloc_sz != 0 ? (addr + alloc_sz) : 0;
            }
        }
    }
    return 0;
}

void NoteNativeGpuBlockFree(VAddr addr, u64 byte_size) noexcept {
    if ((addr & 3ULL) == 0ULL && addr != 0) {
        const size_t base_set = ExactBufSetIdx(addr);
        for (size_t p = 0; p < kExactBufProbeSets; ++p) {
            auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
            for (size_t w = 0; w < kExactBufWays; ++w) {
                const u64 cur = set[w].addr_and_flag.load(std::memory_order_relaxed);
                if ((cur & ~3ULL) == addr) {
                    set[w].addr_and_flag.store(0ULL, std::memory_order_release);
                }
            }
        }
    }
    if (byte_size == 0 || byte_size > kNativeMaxSingleBlockBytes || addr < kNativeDirBase) {
        return;
    }
    const u64 rel_start = addr - kNativeDirBase;
    if (rel_start >= kNativeDirMaxBytes) {
        return;
    }
    const u64 rel_end = std::min<u64>(rel_start + byte_size, kNativeDirMaxBytes);
    if (rel_end <= rel_start) {
        return;
    }
    const u32 start_rel4 = static_cast<u32>((rel_start + 3ULL) >> 2);
    const u32 end_rel4 = static_cast<u32>((rel_end + 3ULL) >> 2);
    const u64 full_begin = (rel_start + kNativePageSize - 1ULL) & ~(kNativePageSize - 1ULL);
    const u64 full_end = rel_end & ~(kNativePageSize - 1ULL);
    if (full_begin < full_end) {
        const size_t p_begin = static_cast<size_t>(full_begin >> kNativePageShift);
        const size_t p_end = static_cast<size_t>(full_end >> kNativePageShift);
        for (size_t p = p_begin; p < p_end; ++p) {
            g_native_page_end_rel4[p].store(0u, std::memory_order_release);
        }
    }
    if ((rel_start & (kNativePageSize - 1ULL)) != 0ULL) {
        const size_t head_p = static_cast<size_t>(rel_start >> kNativePageShift);
        u32 expected = end_rel4;
        g_native_page_end_rel4[head_p].compare_exchange_strong(
            expected, start_rel4, std::memory_order_release, std::memory_order_relaxed);
    }
    if ((rel_end & (kNativePageSize - 1ULL)) != 0ULL) {
        const size_t tail_p = static_cast<size_t>((rel_end - 1ULL) >> kNativePageShift);
        const u64 tail_page_start = static_cast<u64>(tail_p) << kNativePageShift;
        if (rel_start <= tail_page_start) {
            u32 expected = end_rel4;
            g_native_page_end_rel4[tail_p].compare_exchange_strong(
                expected, 0u, std::memory_order_release, std::memory_order_relaxed);
        }
    }
}

u64 ClampNativeGpuReadSize(VAddr addr, u64 requested_size, bool for_write) noexcept {
    u64 clamped = ClampTransientCueReadSize(addr, requested_size);
    static const bool n1_enabled =
        gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers);
    if (!n1_enabled || clamped == 0) {
        return clamped;
    }
    if ((addr & 3ULL) == 0ULL && addr != 0) {
        const size_t base_set = ExactBufSetIdx(addr);
        bool matched = false;
        for (size_t p = 0; p < kExactBufProbeSets && !matched; ++p) {
            const auto& set = g_exact_buf_table[(base_set + p) & (kExactBufSets - 1)];
            for (size_t w = 0; w < kExactBufWays; ++w) {
                const u64 tag = set[w].addr_and_flag.load(std::memory_order_acquire);
                if ((tag & ~3ULL) == addr) {
                    const u64 sizes = set[w].sizes_packed.load(std::memory_order_relaxed);
                    const u32 req_sz = static_cast<u32>(sizes & 0xFFFFFFFFULL);
                    const u32 alloc_sz = static_cast<u32>(sizes >> 32);
                    if (alloc_sz != 0) {
                        if (clamped > alloc_sz) {
                            return for_write ? alloc_sz : (req_sz != 0 ? req_sz : alloc_sz);
                        }
                        return clamped;
                    }
                    matched = true;
                    break;
                }
            }
        }
    }
    if (addr < kNativeDirBase) {
        return clamped;
    }
    const u64 rel = addr - kNativeDirBase;
    if (rel >= kNativeDirMaxBytes) {
        return clamped;
    }
    const size_t cur_page = static_cast<size_t>(rel >> kNativePageShift);
    const u32 end_rel4 = g_native_page_end_rel4[cur_page].load(std::memory_order_acquire);
    const u64 end_rel = static_cast<u64>(end_rel4) << 2;
    if (end_rel > rel) {
        clamped = std::min<u64>(clamped, end_rel - rel);
        return clamped;
    }
    // N1.2d: Unbounded 0xFFFFFFFF V# descriptor on an unrecorded system/bump range: scan forward
    // up to 4 MiB (1024 pages) to the next recorded allocation boundary and cap unknown whole-heap
    // mmap tails so a single 0xFFFFFFFF UAV/SRV never sweeps 3.95 GiB / 2.56 GiB into BufferCache
    // or wipes out neighboring DirectBuffers via OnGpuWrite.
    if (requested_size > kNativeMaxSingleBlockBytes) {
        const size_t max_scan = std::min<size_t>(kNativeDirPages, cur_page + 1024u);
        for (size_t p = cur_page + 1u; p < max_scan; ++p) {
            const u64 page_rel = static_cast<u64>(p) << kNativePageShift;
            const u32 p_end4 = g_native_page_end_rel4[p].load(std::memory_order_relaxed);
            if (p_end4 != 0u && (static_cast<u64>(p_end4) << 2) > page_rel) {
                clamped = std::min<u64>(clamped, page_rel - rel);
                break;
            }
        }
        constexpr u64 kUnboundedUnknownCap = 16ULL << 20;
        clamped = std::min<u64>(clamped, kUnboundedUnknownCap);
    }
    return clamped;
}

void DrainEvents(std::vector<Event>& out) {
    out.clear();
    if (g_pending_count.load(std::memory_order_acquire) == 0) {
        return;
    }
    std::scoped_lock lock{g_queue_mutex};
    if (g_queue.empty()) {
        g_pending_count.store(0, std::memory_order_release);
        return;
    }
    out.swap(g_queue);
    g_pending_count.store(0, std::memory_order_release);
}

Manager::Manager(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
                 Vulkan::Runtime& runtime_)
    : instance{instance_}, scheduler{scheduler_}, runtime{runtime_},
      bitmap_64k(kBitmapWords, 0ULL) {}

Manager::~Manager() = default;

void Manager::SetBitmapRange(VAddr start, VAddr end) noexcept {
    if (start >= end || start >= kMaxGuestVa) {
        return;
    }
    end = std::min<VAddr>(end, kMaxGuestVa);
    const u64 first_page = start >> kPageShift64K;
    const u64 last_page = (end - 1ULL) >> kPageShift64K;
    for (u64 page = first_page; page <= last_page; ++page) {
        bitmap_64k[page >> 6] |= (1ULL << (page & 63ULL));
    }
}

void Manager::RebuildBitmapAroundErase(VAddr erased_start, VAddr erased_end) noexcept {
    if (erased_start >= erased_end || erased_start >= kMaxGuestVa) {
        return;
    }
    erased_end = std::min<VAddr>(erased_end, kMaxGuestVa);
    const u64 first_page = erased_start >> kPageShift64K;
    const u64 last_page = (erased_end - 1ULL) >> kPageShift64K;
    for (u64 page = first_page; page <= last_page; ++page) {
        bitmap_64k[page >> 6] &= ~(1ULL << (page & 63ULL));
    }

    // Re-set 64 KiB bits for any surviving entries sharing the boundary 64 KiB pages.
    const VAddr window_begin = first_page << kPageShift64K;
    const VAddr window_end = std::min<VAddr>((last_page + 1ULL) << kPageShift64K, kMaxGuestVa);
    auto it = entries_by_base.lower_bound(window_begin);
    if (it != entries_by_base.begin()) {
        --it;
    }
    for (; it != entries_by_base.end() && it->first < window_end; ++it) {
        const VAddr e_start = std::max<VAddr>(it->first, window_begin);
        const VAddr e_end = std::min<VAddr>(it->first + it->second.alloc_size, window_end);
        if (e_start < e_end) {
            SetBitmapRange(e_start, e_end);
        }
    }
}

bool Manager::AnyBitmap64k(VAddr addr, u64 size) const noexcept {
    if (size == 0 || addr >= kMaxGuestVa) {
        return false;
    }
    const VAddr end = std::min<VAddr>(addr + size, kMaxGuestVa);
    if (addr >= end) {
        return false;
    }
    const u64 first_page = addr >> kPageShift64K;
    const u64 last_page = (end - 1ULL) >> kPageShift64K;
    for (u64 page = first_page; page <= last_page; ++page) {
        if ((bitmap_64k[page >> 6] & (1ULL << (page & 63ULL))) != 0ULL) {
            return true;
        }
    }
    return false;
}

void Manager::ClearMruFor(const Buffer* buf) noexcept {
    for (auto& slot : mru) {
        if (slot.buffer == buf) {
            slot = {};
        }
    }
}

void Manager::UpdateMruEntry(VAddr base_addr, u64 alloc_size, const Buffer* buf,
                             u64 base_offset) noexcept {
    if (!buf) {
        for (auto& slot : mru) {
            if (slot.base_addr == base_addr) {
                slot = {};
            }
        }
        return;
    }
    bool updated_existing = false;
    for (auto& slot : mru) {
        if (slot.base_addr == base_addr) {
            slot.alloc_size = alloc_size;
            slot.buffer = buf;
            slot.base_offset = base_offset;
            updated_existing = true;
        }
    }
    if (!updated_existing) {
        const size_t h0 =
            static_cast<size_t>(((base_addr >> 12) ^ (base_addr >> 20)) & (kMruSlots - 1));
        mru[(h0 + 1u) & (kMruSlots - 1)] = mru[h0];
        mru[h0] = MruSlot{base_addr, alloc_size, buf, base_offset};
    }
}

void Manager::ReclaimCompleted() noexcept {
    while (!retired.empty() && scheduler.IsFree(retired.front().first)) {
        retired.pop_front();
    }
}

void Manager::RetireOverlapping(VAddr addr, u64 size, bool clear_global_owned) {
    if (entries_by_base.empty() || size == 0 || !AnyBitmap64k(addr, size)) {
        return;
    }
    const VAddr range_end = addr + size;
    auto it = entries_by_base.lower_bound(addr);
    if (it != entries_by_base.begin()) {
        --it;
    }
    const u64 tick = scheduler.CurrentTick();
    while (it != entries_by_base.end() && it->first < range_end) {
        const VAddr e_start = it->first;
        const u64 e_size = it->second.alloc_size;
        const VAddr e_end = e_start + e_size;
        if (e_end <= addr) {
            ++it;
            continue;
        }
        UpdateMruEntry(e_start, 0, nullptr, 0);
        ClearMruFor(it->second.buffer.get());
        if (clear_global_owned) {
            ClearNativeOwnedRange(e_start, e_size);
            SetExactNativeBufferOwned(e_start, false);
        }
        retired.emplace_back(tick, std::move(it->second.buffer));
        it = entries_by_base.erase(it);
        RebuildBitmapAroundErase(e_start, e_end);
    }
}

bool Manager::OverlapsLiveEntry(VAddr addr, u64 size) const noexcept {
    if (entries_by_base.empty() || size == 0 || !AnyBitmap64k(addr, size)) {
        return false;
    }
    const VAddr range_end = addr + size;
    auto it = entries_by_base.lower_bound(addr);
    if (it != entries_by_base.begin()) {
        --it;
    }
    while (it != entries_by_base.end() && it->first < range_end) {
        const VAddr e_end = it->first + it->second.alloc_size;
        if (e_end > addr) {
            return true;
        }
        ++it;
    }
    return false;
}

void Manager::OnGpuWrite(VAddr addr, u64 size) noexcept {
    if (size == 0) {
        return;
    }
    NoteTransientCueHeapGpuWrite(addr, size);
    Vulkan::DirectShaders::InvalidateRange(addr, size);
    if (entries_by_base.empty()) {
        return;
    }
    RetireOverlapping(addr, size, /*clear_global_owned=*/true);
}

std::pair<const Buffer*, u64> Manager::TryResolveReadClamped(VAddr addr,
                                                             u64& inout_size) noexcept {
    if (inout_size == 0 || addr >= kMaxGuestVa) {
        return {nullptr, 0};
    }
    const auto note_hit = []() noexcept {
        BbStats::native_buf_hits.store(
            BbStats::native_buf_hits.load(std::memory_order_relaxed) + 1ULL,
            std::memory_order_relaxed);
    };
    const auto note_miss = [addr, &inout_size]() noexcept {
        if (!IsTransientCueHeap(addr, inout_size)) {
            BbStats::native_buf_misses.store(
                BbStats::native_buf_misses.load(std::memory_order_relaxed) + 1ULL,
                std::memory_order_relaxed);
        }
    };
    if (entries_by_base.empty()) {
        note_miss();
        return {nullptr, 0};
    }

    static const bool n1_enabled =
        gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers);
    const size_t h0 = static_cast<size_t>(((addr >> 12) ^ (addr >> 20)) & (kMruSlots - 1));
    const size_t h1 = (h0 + 1u) & (kMruSlots - 1);
    for (const size_t idx : {h0, h1}) {
        const auto& slot = mru[idx];
        if (slot.buffer && addr >= slot.base_addr && addr < slot.base_addr + slot.alloc_size) {
            const u64 rem = slot.alloc_size - (addr - slot.base_addr);
            if (inout_size <= rem || n1_enabled) {
                if (inout_size > rem) {
                    inout_size = rem;
                }
                note_hit();
                return {slot.buffer, slot.base_offset + (addr - slot.base_addr)};
            }
        }
    }

    const u64 page = addr >> kPageShift64K;
    if ((bitmap_64k[page >> 6] & (1ULL << (page & 63ULL))) == 0ULL) {
        note_miss();
        return {nullptr, 0};
    }

    auto it = entries_by_base.upper_bound(addr);
    if (it == entries_by_base.begin()) {
        note_miss();
        return {nullptr, 0};
    }
    --it;
    const VAddr base = it->first;
    const u64 alloc_size = it->second.alloc_size;
    if (addr >= base && addr < base + alloc_size) {
        const u64 rem = alloc_size - (addr - base);
        if (inout_size <= rem || n1_enabled) {
            if (inout_size > rem) {
                inout_size = rem;
            }
            const Buffer* active_buf = it->second.stream_override_buf
                                           ? it->second.stream_override_buf
                                           : (it->second.needs_zero_init && zero_buffer
                                                  ? zero_buffer.get()
                                                  : it->second.buffer.get());
            const u64 active_off =
                it->second.stream_override_buf ? it->second.stream_override_off : 0ULL;
            mru[h1] = mru[h0];
            mru[h0] = MruSlot{base, alloc_size, active_buf, active_off};
            note_hit();
            return {active_buf, active_off + (addr - base)};
        }
    }
    note_miss();
    return {nullptr, 0};
}

std::pair<const Buffer*, u64> Manager::TryResolveWrite(VAddr addr, u64 size, bool full_overwrite,
                                                       bool track_access) {
    if (size == 0 || addr >= kMaxGuestVa || entries_by_base.empty() || !AnyBitmap64k(addr, size)) {
        return {nullptr, 0};
    }
    Vulkan::DirectShaders::InvalidateRange(addr, size);

    auto it = entries_by_base.upper_bound(addr);
    if (it != entries_by_base.begin()) {
        --it;
        const VAddr base = it->first;
        const u64 alloc_size = it->second.alloc_size;
        if (addr >= base && addr + size <= base + alloc_size) {
            if (it->second.dirty_writeback) {
                FlushPendingWritebacks();
                it = entries_by_base.find(base);
                if (it == entries_by_base.end() || !it->second.buffer) {
                    return {nullptr, 0};
                }
            }
            const Buffer* dst_buf = it->second.buffer.get();
            if (it->second.needs_zero_init) {
                it->second.needs_zero_init = false;
                if (!full_overwrite || addr > base || size < dst_buf->SizeBytes()) {
                    runtime.FillBuffer(dst_buf, 0, dst_buf->SizeBytes(), 0);
                    runtime.FlushBarriers();
                }
                UpdateMruEntry(base, alloc_size, dst_buf, 0);
            }
            const u64 dst_off = addr - base;
            if (track_access) {
                runtime.AccessBuffer(dst_buf, dst_off, size,
                                     vk::PipelineStageFlagBits2::eAllGraphics |
                                         vk::PipelineStageFlagBits2::eComputeShader,
                                     vk::AccessFlagBits2::eShaderRead |
                                         vk::AccessFlagBits2::eUniformRead |
                                         vk::AccessFlagBits2::eVertexAttributeRead |
                                         vk::AccessFlagBits2::eIndexRead |
                                         vk::AccessFlagBits2::eIndirectCommandRead);
            }
            return {dst_buf, dst_off};
        }
    }
    NoteTransientCueHeapGpuWrite(addr, size);
    RetireOverlapping(addr, size);
    return {nullptr, 0};
}

void Manager::StageNativeCpuCopyBatch(std::span<const NativeCopyRegion> regions,
                                      std::span<u8> out_handled,
                                      BufferCache& buf_cache,
                                      TextureCache& tex_cache,
                                      Core::MemoryManager* memory) {
    if (!out_handled.empty()) {
        std::memset(out_handled.data(), 0, out_handled.size());
    }
    if (regions.empty() || out_handled.size() < regions.size() || entries_by_base.empty() ||
        !memory) {
        return;
    }

    struct StagedSlice {
        u32 region_idx{0};
        bool fast_memcpy{false};
        VAddr base_va{0};
        VAddr src_va{0};
        u32 dst_in_buf_off{0};
        u32 copy_bytes{0};
        u32 slice_span{0};
        u32 rel_offset{0};
    };

    const u64 align = std::max<u64>(
        16ULL, std::max<u64>(instance.UniformMinAlignment(), instance.StorageMinAlignment()));
    auto& sbuf = buf_cache.GetStreamBuffer();

    std::array<StagedSlice, 32> staged{};
    u32 staged_count = 0;
    u64 total_bytes = 0;

    const auto commit_chunk = [&]() -> bool {
        if (staged_count == 0 || total_bytes == 0) {
            staged_count = 0;
            total_bytes = 0;
            return true;
        }
        u64 base_off = 0;
        if (!sbuf.mapped_data.empty() && scheduler.IsRecordingDeferred() &&
            !BbToggle::Disabled(BbToggle::DeferredStreamCopies) &&
            !BbToggle::Disabled(BbToggle::PoolSmallCopies)) {
            const auto reserved = sbuf.Reserve(total_bytes, align, /*allow_wait=*/false);
            if (!reserved) {
                staged_count = 0;
                total_bytes = 0;
                return false;
            }
            base_off = *reserved;
            u8* const base_dst = sbuf.mapped_data.data() + base_off;
            Buffer* const sbuf_ptr = &sbuf;
            Core::MemoryManager* const mem = memory;
            const auto chunk_slices = staged;
            const u32 chunk_count = staged_count;
            const u64 chunk_bytes = total_bytes;
            std::array<std::pair<u64, u64>, 32> host_ranges{};
            for (u32 k = 0; k < chunk_count; ++k) {
                host_ranges[k] = {chunk_slices[k].src_va, chunk_slices[k].copy_bytes};
            }
            scheduler.RecordHostCopy(
                [chunk_slices, chunk_count, base_dst, base_off, chunk_bytes, sbuf_ptr, mem] {
                    for (u32 k = 0; k < chunk_count; ++k) {
                        const auto& s = chunk_slices[k];
                        u8* dst = base_dst + s.rel_offset;
                        if (s.dst_in_buf_off != 0) {
                            std::memset(dst, 0, s.dst_in_buf_off);
                        }
                        u8* copy_dst = dst + s.dst_in_buf_off;
                        if (s.fast_memcpy) {
                            std::memcpy(copy_dst, reinterpret_cast<const void*>(s.src_va),
                                        s.copy_bytes);
                        } else {
                            mem->CopySparseMemory(s.src_va, copy_dst, s.copy_bytes);
                        }
                        const u32 written_end = s.dst_in_buf_off + s.copy_bytes;
                        if (s.slice_span > written_end) {
                            std::memset(dst + written_end, 0, s.slice_span - written_end);
                        }
                    }
                    sbuf_ptr->Flush(base_off, chunk_bytes);
                });
            scheduler.NoteHostCopySources(std::span{host_ranges.data(), chunk_count});
        } else {
            const auto [data, offset] = sbuf.Map(total_bytes, align, /*allow_wait=*/false);
            if (!data) {
                staged_count = 0;
                total_bytes = 0;
                return false;
            }
            for (u32 k = 0; k < staged_count; ++k) {
                const auto& s = staged[k];
                u8* dst = data + s.rel_offset;
                if (s.dst_in_buf_off != 0) {
                    std::memset(dst, 0, s.dst_in_buf_off);
                }
                u8* copy_dst = dst + s.dst_in_buf_off;
                if (s.fast_memcpy) {
                    std::memcpy(copy_dst, reinterpret_cast<const void*>(s.src_va), s.copy_bytes);
                } else {
                    memory->CopySparseMemory(s.src_va, copy_dst, s.copy_bytes);
                }
                const u32 written_end = s.dst_in_buf_off + s.copy_bytes;
                if (s.slice_span > written_end) {
                    std::memset(dst + written_end, 0, s.slice_span - written_end);
                }
            }
            sbuf.Commit();
            base_off = offset;
        }

        BbStats::buffer_upload_bytes.fetch_add(total_bytes, std::memory_order_relaxed);
        for (u32 k = 0; k < staged_count; ++k) {
            const auto& s = staged[k];
            auto it = entries_by_base.find(s.base_va);
            if (it == entries_by_base.end()) {
                continue;
            }
            auto& e = it->second;
            const u64 stream_off = base_off + s.rel_offset;
            e.needs_zero_init = false;
            e.stream_override_buf = &sbuf;
            e.stream_override_off = stream_off;
            if (!e.dirty_writeback) {
                e.dirty_writeback = true;
                dirty_writeback_bases.push_back(s.base_va);
            }
            UpdateMruEntry(s.base_va, e.alloc_size, &sbuf, stream_off);
            buf_cache.NoteExternalGpuModified(s.base_va, e.alloc_size);
            Vulkan::DirectShaders::InvalidateRange(s.base_va, e.alloc_size);
            out_handled[s.region_idx] = 1u;
        }
        staged_count = 0;
        total_bytes = 0;
        return true;
    };

    for (u32 idx = 0; idx < regions.size(); ++idx) {
        const auto& reg = regions[idx];
        if (reg.byte_size == 0 || reg.byte_size > (512ULL << 10) || reg.dst_va >= kMaxGuestVa) {
            continue;
        }

        const u64 page = reg.dst_va >> kPageShift64K;
        if ((bitmap_64k[page >> 6] & (1ULL << (page & 63ULL))) == 0ULL) {
            continue;
        }

        auto it = entries_by_base.upper_bound(reg.dst_va);
        if (it == entries_by_base.begin()) {
            continue;
        }
        --it;
        const VAddr base_va = it->first;
        const auto& e = it->second;
        if (!e.buffer || reg.dst_va < base_va ||
            reg.dst_va + reg.byte_size > base_va + e.alloc_size ||
            e.alloc_size > (512ULL << 10)) {
            continue;
        }
        const u64 dst_in_buf_off = reg.dst_va - base_va;
        const bool full_overwrite = (dst_in_buf_off == 0 && reg.byte_size >= e.raw_size);
        if (!full_overwrite && !e.needs_zero_init) {
            continue;
        }

        bool ordering_hazard = false;
        for (u32 prev = 0; prev < idx; ++prev) {
            if (RangesOverlap(reg.src_va, reg.byte_size, regions[prev].dst_va,
                              regions[prev].byte_size)) {
                ordering_hazard = true;
                break;
            }
            const bool prev_will_handle =
                out_handled[prev] != 0u ||
                std::any_of(staged.begin(), staged.begin() + staged_count,
                            [prev](const StagedSlice& s) { return s.region_idx == prev; });
            if ((!prev_will_handle || !full_overwrite) &&
                RangesOverlap(base_va, e.alloc_size, regions[prev].dst_va,
                              regions[prev].byte_size)) {
                ordering_hazard = true;
                break;
            }
        }
        if (ordering_hazard) {
            continue;
        }

        if (tex_cache.HasActiveImagePages(reg.dst_va, reg.byte_size) ||
            tex_cache.IsMeta(reg.dst_va)) {
            continue;
        }

        // Source cannot itself be a native GPU VkBuffer (whose guest RAM is uninitialized/stale).
        if (OverlapsNativeOwnedRange(reg.src_va, reg.byte_size) ||
            OverlapsLiveEntry(reg.src_va, reg.byte_size)) {
            continue;
        }

        const bool cue_src =
            IsTransientCueHeap(reg.src_va, reg.byte_size) &&
            (!IsCueHeapEverGpuModified() ||
             !buf_cache.IsRegionGpuModified(reg.src_va, reg.byte_size));
        uintptr_t mapped_end = 0;
        const bool fast_mapped =
            cue_src || (BbLookupMappedEnd(reg.src_va, mapped_end) &&
                        mapped_end > reg.src_va && (mapped_end - reg.src_va) >= reg.byte_size);
        if (!cue_src) {
            if ((!fast_mapped &&
                 memory->ClampRangeSize(reg.src_va, reg.byte_size) != reg.byte_size) ||
                buf_cache.IsRegionGpuModified(reg.src_va, reg.byte_size)) {
                continue;
            }
        }

        const u64 slice_span = Common::AlignUp<u64>(e.alloc_size, 16ULL);
        u64 rel_off = Common::AlignUp<u64>(total_bytes, align);
        if (staged_count == 32 || rel_off + slice_span > (4ULL << 20)) {
            if (!commit_chunk()) {
                return;
            }
            rel_off = 0;
        }
        total_bytes = rel_off + slice_span;
        staged[staged_count++] = StagedSlice{
            .region_idx = idx,
            .fast_memcpy = fast_mapped,
            .base_va = base_va,
            .src_va = reg.src_va,
            .dst_in_buf_off = static_cast<u32>(dst_in_buf_off),
            .copy_bytes = reg.byte_size,
            .slice_span = static_cast<u32>(slice_span),
            .rel_offset = static_cast<u32>(rel_off),
        };
    }

    commit_chunk();
}

void Manager::FlushPendingWritebacks() {
    if (dirty_writeback_bases.empty()) {
        return;
    }
    std::vector<VAddr> dirty;
    dirty.swap(dirty_writeback_bases);

    auto* buf_cache = g_active_buf_cache.load(std::memory_order_acquire);
    if (!buf_cache) {
        for (const VAddr base : dirty) {
            if (auto it = entries_by_base.find(base); it != entries_by_base.end()) {
                it->second.stream_override_buf = nullptr;
                it->second.stream_override_off = 0;
                it->second.dirty_writeback = false;
                it->second.needs_zero_init = false;
                UpdateMruEntry(base, it->second.alloc_size, it->second.buffer.get(), 0);
            }
        }
        return;
    }

    scheduler.EndRendering();

    struct WritebackCmd {
        vk::Buffer dst_vk{};
        u64 src_off{0};
        u64 size{0};
    };
    static thread_local std::vector<WritebackCmd> cmds;
    cmds.clear();
    cmds.reserve(dirty.size());

    const Buffer& sbuf = buf_cache->GetStreamBuffer();
    const u64 sbuf_bytes = sbuf.SizeBytes();
    for (const VAddr base : dirty) {
        auto it = entries_by_base.find(base);
        if (it == entries_by_base.end()) {
            continue;
        }
        auto& e = it->second;
        if (!e.dirty_writeback || !e.stream_override_buf || !e.buffer) {
            e.stream_override_buf = nullptr;
            e.stream_override_off = 0;
            e.dirty_writeback = false;
            continue;
        }
        const u64 copy_bytes =
            std::min<u64>(e.buffer->SizeBytes(), Common::AlignUp<u64>(e.alloc_size, 16ULL));
        const u64 src_off = e.stream_override_off;
        e.stream_override_buf = nullptr;
        e.stream_override_off = 0;
        e.dirty_writeback = false;
        e.needs_zero_init = false;
        UpdateMruEntry(base, e.alloc_size, e.buffer.get(), 0);
        if (copy_bytes == 0 || src_off + copy_bytes > sbuf_bytes) {
            continue;
        }
        cmds.push_back(WritebackCmd{
            .dst_vk = e.buffer->Handle(),
            .src_off = src_off,
            .size = copy_bytes,
        });
    }

    if (cmds.empty()) {
        return;
    }

    const vk::Buffer src_vk = sbuf.Handle();
    constexpr size_t kBatchCap = 512;
    for (size_t offset = 0; offset < cmds.size(); offset += kBatchCap) {
        const size_t count = std::min(kBatchCap, cmds.size() - offset);
        const bool is_first = (offset == 0);
        const bool is_last = (offset + count >= cmds.size());
        const auto recorded_cmds =
            scheduler.RecordData(std::span<const WritebackCmd>{cmds.data() + offset, count});
        scheduler.Record([src_vk, recorded_cmds, is_first, is_last](vk::CommandBuffer cmdbuf) {
            if (is_first) {
                const vk::MemoryBarrier2 pre_barrier{
                    .srcStageMask = vk::PipelineStageFlagBits2::eAllGraphics |
                                    vk::PipelineStageFlagBits2::eComputeShader |
                                    vk::PipelineStageFlagBits2::eCopy |
                                    vk::PipelineStageFlagBits2::eClear,
                    .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
                    .dstStageMask = vk::PipelineStageFlagBits2::eCopy,
                    .dstAccessMask = vk::AccessFlagBits2::eTransferWrite |
                                     vk::AccessFlagBits2::eTransferRead,
                };
                cmdbuf.pipelineBarrier2(vk::DependencyInfo{
                    .memoryBarrierCount = 1,
                    .pMemoryBarriers = &pre_barrier,
                });
            }
            for (const auto& cmd : recorded_cmds) {
                const vk::BufferCopy region{
                    .srcOffset = cmd.src_off,
                    .dstOffset = 0,
                    .size = cmd.size,
                };
                cmdbuf.copyBuffer(src_vk, cmd.dst_vk, 1, &region);
            }
            if (is_last) {
                const vk::MemoryBarrier2 post_barrier{
                    .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
                    .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                    .dstStageMask = vk::PipelineStageFlagBits2::eAllGraphics |
                                    vk::PipelineStageFlagBits2::eComputeShader |
                                    vk::PipelineStageFlagBits2::eCopy |
                                    vk::PipelineStageFlagBits2::eClear,
                    .dstAccessMask = vk::AccessFlagBits2::eMemoryRead |
                                     vk::AccessFlagBits2::eMemoryWrite,
                };
                cmdbuf.pipelineBarrier2(vk::DependencyInfo{
                    .memoryBarrierCount = 1,
                    .pMemoryBarriers = &post_barrier,
                });
            }
        });
    }
}

void Manager::ProcessEvents(Core::MemoryManager* memory) {
    ReclaimCompleted();
    const bool is_rendering = scheduler.IsRendering();
    if (!NeedsProcessEvents(is_rendering)) {
        return;
    }

    static thread_local std::vector<Event> incoming;
    static thread_local std::vector<Event> events;
    events.clear();
    if (!deferred_events.empty()) {
        events.swap(deferred_events);
    }
    if (HasPendingEvents()) {
        DrainEvents(incoming);
        events.insert(events.end(), incoming.begin(), incoming.end());
    }
    if (events.empty()) {
        return;
    }

    // Coalesce transient buffers freed or overwritten later in the same batch.
    const size_t n = events.size();
    static thread_local std::vector<u8> superseded;
    superseded.assign(n, 0u);
    for (size_t i = 0; i < n; ++i) {
        if (events[i].kind != EventKind::RegisterImmutable &&
            events[i].kind != EventKind::RegisterNative) {
            continue;
        }
        const VAddr i_addr = events[i].addr;
        const u64 i_span = std::max<u64>(events[i].raw_byte_size, events[i].alloc_byte_size);
        for (size_t j = i + 1; j < n; ++j) {
            const u64 j_span = (events[j].kind == EventKind::RetireRange)
                                   ? events[j].byte_size
                                   : std::max<u64>(events[j].raw_byte_size,
                                                   events[j].alloc_byte_size);
            if (RangesOverlap(i_addr, i_span, events[j].addr, j_span)) {
                superseded[i] = 1u;
                break;
            }
        }
    }

    constexpr u64 kChunkBytes = 16ULL << 20; // 16 MiB staging segment slice
    u64 uploaded_bytes_since_flush = 0;
    bool needs_barrier_flush = false;
    auto* active_buf_cache = g_active_buf_cache.load(std::memory_order_acquire);

    if (!is_rendering && !zero_buffer) {
        zero_buffer = std::make_unique<Buffer>(instance, 0, kSharedZeroBytes,
                                               MemoryType::DeviceLocal, "DirectSharedZeroBuf");
        runtime.FillBuffer(zero_buffer.get(), 0, kSharedZeroBytes, 0);
        needs_barrier_flush = true;
    }

    const u64 stream_align = std::max<u64>(
        16ULL, std::max<u64>(instance.UniformMinAlignment(), instance.StorageMinAlignment()));

    const auto stage_into_stream = [&](VAddr src_addr, u64 raw_bytes, u64 alloc_bytes,
                                       bool copy_src, u64& out_stream_off) -> const Buffer* {
        if (!active_buf_cache || alloc_bytes == 0 || alloc_bytes > (256ULL << 10)) {
            return nullptr;
        }
        auto& sbuf = active_buf_cache->GetStreamBuffer();
        const auto [data, offset] = sbuf.Map(alloc_bytes, stream_align, /*allow_wait=*/false);
        if (!data) {
            return nullptr;
        }
        if (copy_src && raw_bytes != 0) {
            std::memcpy(data, reinterpret_cast<const void*>(src_addr), raw_bytes);
            if (alloc_bytes > raw_bytes) {
                std::memset(data + raw_bytes, 0, alloc_bytes - raw_bytes);
            }
        } else {
            std::memset(data, 0, alloc_bytes);
        }
        sbuf.Commit();
        BbStats::buffer_upload_bytes.fetch_add(alloc_bytes, std::memory_order_relaxed);
        out_stream_off = offset;
        return &sbuf;
    };

    for (size_t i = 0; i < n; ++i) {
        const auto& ev = events[i];
        if (ev.kind == EventKind::RetireRange) {
            if (ev.addr != 0 && ev.byte_size != 0) {
                RetireOverlapping(ev.addr, ev.byte_size, /*clear_global_owned=*/false);
            }
            continue;
        }

        if (superseded[i] != 0u || ev.addr == 0 || ev.raw_byte_size == 0 ||
            ev.addr >= kMaxGuestVa) {
            continue;
        }
        if (memory && !memory->IsValidGpuMapping(ev.addr, ev.raw_byte_size)) {
            continue;
        }

        const u64 raw_size = ev.raw_byte_size;
        const u64 guest_span = std::max<u64>(raw_size, ev.alloc_byte_size);
        const u64 vk_alloc_size = Common::AlignUp<u64>(guest_span, 16ULL);

        const char* debug_name = (ev.usage_type == 1)   ? "DirectImmutableBuf"
                                 : (ev.usage_type == 2) ? "DirectDynamicBuf"
                                                        : "DirectDefaultBuf";

        // N1.2b/N1.2c: Inside an open RenderPass, NEVER call runtime.CopyBuffer / runtime.FillBuffer
        // (which would call scheduler.EndRendering() and break the 2560x1440 6-MRT pass!).
        // Guest-timeline tables (`g_native_owned_4b` / `g_exact_buf_table`) are updated strictly on
        // the Guest CPU thread so delayed Stage B `ProcessEvents` can never race with subsequent
        // guest-thread frees/reallocations at the same VA.
        if (is_rendering) {
            if (ev.has_initial_data != 0) {
                u64 stream_off = 0;
                const Buffer* sbuf_ptr =
                    stage_into_stream(ev.addr, raw_size, vk_alloc_size, /*copy_src=*/true,
                                      stream_off);
                if (!sbuf_ptr) {
                    deferred_events.push_back(ev);
                    continue;
                }
                RetireOverlapping(ev.addr, guest_span, /*clear_global_owned=*/false);
                auto buf = std::make_unique<Buffer>(instance, ev.addr, vk_alloc_size,
                                                    MemoryType::DeviceLocal, debug_name);
                Entry entry{};
                entry.resource_id = ev.resource_id;
                entry.bind_flags = ev.bind_flags;
                entry.usage_type = ev.usage_type;
                entry.dirty_writeback = true;
                entry.needs_zero_init = false;
                entry.raw_size = raw_size;
                entry.alloc_size = guest_span;
                entry.stream_override_buf = sbuf_ptr;
                entry.stream_override_off = stream_off;
                entry.buffer = std::move(buf);
                entries_by_base[ev.addr] = std::move(entry);
                dirty_writeback_bases.push_back(ev.addr);
                SetBitmapRange(ev.addr, ev.addr + guest_span);
                const size_t h0 =
                    static_cast<size_t>(((ev.addr >> 12) ^ (ev.addr >> 20)) & (kMruSlots - 1));
                mru[(h0 + 1u) & (kMruSlots - 1)] = mru[h0];
                mru[h0] = MruSlot{ev.addr, guest_span, sbuf_ptr, stream_off};
                continue;
            }

            // ev.has_initial_data == 0 inside an active RenderPass:
            if (zero_buffer && vk_alloc_size <= kSharedZeroBytes) {
                RetireOverlapping(ev.addr, guest_span, /*clear_global_owned=*/false);
                auto buf = std::make_unique<Buffer>(instance, ev.addr, vk_alloc_size,
                                                    MemoryType::DeviceLocal, debug_name);
                Entry entry{};
                entry.resource_id = ev.resource_id;
                entry.bind_flags = ev.bind_flags;
                entry.usage_type = ev.usage_type;
                entry.needs_zero_init = true;
                entry.raw_size = raw_size;
                entry.alloc_size = guest_span;
                entry.buffer = std::move(buf);
                entries_by_base[ev.addr] = std::move(entry);
                SetBitmapRange(ev.addr, ev.addr + guest_span);
                if (active_buf_cache) {
                    active_buf_cache->NoteExternalGpuModified(ev.addr, guest_span);
                }
                const size_t h0 =
                    static_cast<size_t>(((ev.addr >> 12) ^ (ev.addr >> 20)) & (kMruSlots - 1));
                mru[(h0 + 1u) & (kMruSlots - 1)] = mru[h0];
                mru[h0] = MruSlot{ev.addr, guest_span, zero_buffer.get(), 0};
                continue;
            }
            u64 stream_off = 0;
            const Buffer* sbuf_ptr =
                stage_into_stream(ev.addr, raw_size, vk_alloc_size, /*copy_src=*/false, stream_off);
            if (!sbuf_ptr) {
                deferred_events.push_back(ev);
                continue;
            }
            RetireOverlapping(ev.addr, guest_span, /*clear_global_owned=*/false);
            auto buf = std::make_unique<Buffer>(instance, ev.addr, vk_alloc_size,
                                                MemoryType::DeviceLocal, debug_name);
            Entry entry{};
            entry.resource_id = ev.resource_id;
            entry.bind_flags = ev.bind_flags;
            entry.usage_type = ev.usage_type;
            entry.dirty_writeback = true;
            entry.needs_zero_init = false;
            entry.raw_size = raw_size;
            entry.alloc_size = guest_span;
            entry.stream_override_buf = sbuf_ptr;
            entry.stream_override_off = stream_off;
            entry.buffer = std::move(buf);
            entries_by_base[ev.addr] = std::move(entry);
            dirty_writeback_bases.push_back(ev.addr);
            SetBitmapRange(ev.addr, ev.addr + guest_span);
            if (active_buf_cache) {
                active_buf_cache->NoteExternalGpuModified(ev.addr, guest_span);
            }
            const size_t h0 =
                static_cast<size_t>(((ev.addr >> 12) ^ (ev.addr >> 20)) & (kMruSlots - 1));
            mru[(h0 + 1u) & (kMruSlots - 1)] = mru[h0];
            mru[h0] = MruSlot{ev.addr, guest_span, sbuf_ptr, stream_off};
            continue;
        }

        // Outside any active RenderPass (!is_rendering):
        RetireOverlapping(ev.addr, guest_span, /*clear_global_owned=*/false);
        auto buf = std::make_unique<Buffer>(instance, ev.addr, vk_alloc_size,
                                            MemoryType::DeviceLocal, debug_name);
        bool lazy_zero = false;
        if (ev.has_initial_data != 0) {
            u64 copied = 0;
            while (copied < raw_size) {
                const u64 raw_chunk = std::min<u64>(raw_size - copied, kChunkBytes);
                const bool is_last = (copied + raw_chunk == raw_size);
                const u64 stage_chunk = is_last ? (vk_alloc_size - copied) : raw_chunk;

                const auto staging =
                    runtime.GetStagingPool().Request(stage_chunk, MemoryType::HostUncached);
                std::memcpy(staging.mapped, reinterpret_cast<const u8*>(ev.addr + copied),
                            raw_chunk);
                if (stage_chunk > raw_chunk) {
                    std::memset(staging.mapped + raw_chunk, 0, stage_chunk - raw_chunk);
                }
                staging.Flush();

                const vk::BufferCopy copy_region{
                    .srcOffset = staging.offset,
                    .dstOffset = copied,
                    .size = stage_chunk,
                };
                runtime.CopyBuffer(staging.buffer, buf.get(), std::span{&copy_region, 1});
                copied += raw_chunk;
            }
            uploaded_bytes_since_flush += vk_alloc_size;
            needs_barrier_flush = true;
        } else if (zero_buffer && vk_alloc_size <= kSharedZeroBytes) {
            lazy_zero = true;
            if (active_buf_cache) {
                active_buf_cache->NoteExternalGpuModified(ev.addr, guest_span);
            }
        } else {
            runtime.FillBuffer(buf.get(), 0, vk_alloc_size, 0);
            if (active_buf_cache) {
                active_buf_cache->NoteExternalGpuModified(ev.addr, guest_span);
            }
            needs_barrier_flush = true;
        }

        const Buffer* mru_ptr = lazy_zero ? zero_buffer.get() : buf.get();
        Entry entry{};
        entry.resource_id = ev.resource_id;
        entry.bind_flags = ev.bind_flags;
        entry.usage_type = ev.usage_type;
        entry.needs_zero_init = lazy_zero;
        entry.raw_size = raw_size;
        entry.alloc_size = guest_span;
        entry.buffer = std::move(buf);
        entries_by_base[ev.addr] = std::move(entry);
        SetBitmapRange(ev.addr, ev.addr + guest_span);
        const size_t h0 =
            static_cast<size_t>(((ev.addr >> 12) ^ (ev.addr >> 20)) & (kMruSlots - 1));
        mru[(h0 + 1u) & (kMruSlots - 1)] = mru[h0];
        mru[h0] = MruSlot{ev.addr, guest_span, mru_ptr, 0};

        if (uploaded_bytes_since_flush >= (32ULL << 20)) {
            FlushPendingWritebacks();
            runtime.FlushBarriers();
            needs_barrier_flush = false;
            Vulkan::SubmitInfo info{};
            scheduler.Flush(info);
            ReclaimCompleted();
            uploaded_bytes_since_flush = 0;
        }
    }
    if (needs_barrier_flush) {
        runtime.FlushBarriers();
    }
}

} // namespace VideoCore::DirectBuffers

extern "C" {

void bb_direct_vk_note_fresh_gpu_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    const bool had_native = VideoCore::DirectBuffers::OverlapsNativeOwnedRange(addr, byte_size);
    if (had_native) {
        VideoCore::DirectBuffers::ClearNativeOwnedRange(addr, byte_size);
    }
    VideoCore::DirectBuffers::NoteTransientCueHeapAlloc(addr, byte_size);
    VideoCore::DirectBuffers::NoteNativeGpuBlockAlloc(addr, byte_size);
    if (auto* buf = VideoCore::DirectBuffers::g_active_buf_cache.load(std::memory_order_acquire)) {
        buf->NoteFreshRange(addr, byte_size);
    }
    const bool had_guest_64k = VideoCore::DirectBuffers::GuestImmutableIntersects(addr, byte_size);
    if (had_native || had_guest_64k) {
        if (had_guest_64k) {
            VideoCore::DirectBuffers::ClearWholeGuestImmutablePages(addr, byte_size);
        }
        VideoCore::DirectBuffers::Event ev{};
        ev.kind = VideoCore::DirectBuffers::EventKind::RetireRange;
        ev.addr = addr;
        ev.byte_size = byte_size;
        VideoCore::DirectBuffers::PushEvent(ev);
    }
    Vulkan::DirectShaders::InvalidateRange(addr, byte_size);
}

void bb_direct_vk_prepare_buffer_write(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    BbLayer::WriteTraps::Set(addr, byte_size, BbLayer::WriteTraps::Mirror, false);
    if (auto* buf = VideoCore::DirectBuffers::g_active_buf_cache.load(std::memory_order_acquire)) {
        buf->DisarmWatchedRange(addr, byte_size);
    }
    if (auto* tex = VideoCore::DirectBuffers::g_active_tex_cache.load(std::memory_order_acquire)) {
        tex->DisarmWatchedRange(addr, static_cast<size_t>(byte_size), /*mark_dirty=*/false);
    }
}

void bb_direct_vk_note_asset_buffer_write(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    runtime_memory_note_write(reinterpret_cast<uintptr_t>(gpu_addr), byte_size);
}

void bb_direct_vk_note_exact_buffer_alloc(const void* gpu_addr,
                                          uint32_t raw_byte_size,
                                          uint32_t alloc_byte_size,
                                          uint32_t is_native_owned) noexcept {
    if (!gpu_addr || raw_byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    const u32 span = alloc_byte_size >= raw_byte_size ? alloc_byte_size : raw_byte_size;
    if (is_native_owned != 0u) {
        VideoCore::DirectBuffers::g_ever_registered.store(true, std::memory_order_release);
        VideoCore::DirectBuffers::MarkNativeOwnedRange(addr, span);
    }
    VideoCore::DirectBuffers::NoteNativeGpuBlockAlloc(addr, span);
    VideoCore::DirectBuffers::NoteExactNativeBuffer(addr, raw_byte_size, span,
                                                    is_native_owned != 0u);
}

void bb_direct_vk_register_immutable_buffer(uint32_t resource_id,
                                            const void* gpu_addr,
                                            uint32_t raw_byte_size,
                                            uint32_t alloc_byte_size,
                                            uint32_t bind_flags) noexcept {
    if (!gpu_addr || raw_byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    const u64 span = alloc_byte_size >= raw_byte_size ? alloc_byte_size : raw_byte_size;
    VideoCore::DirectBuffers::g_ever_registered.store(true, std::memory_order_release);
    VideoCore::DirectBuffers::NoteNativeGpuBlockAlloc(addr, span);
    VideoCore::DirectBuffers::MarkNativeOwnedRange(addr, span);
    VideoCore::DirectBuffers::NoteExactNativeBuffer(addr, raw_byte_size, static_cast<u32>(span),
                                                    true);
    VideoCore::DirectBuffers::MarkGuestImmutableRange(addr, span);
    VideoCore::DirectBuffers::Event ev{};
    ev.kind = VideoCore::DirectBuffers::EventKind::RegisterImmutable;
    ev.usage_type = 1u;
    ev.has_initial_data = 1u;
    ev.resource_id = resource_id;
    ev.bind_flags = bind_flags;
    ev.raw_byte_size = raw_byte_size;
    ev.alloc_byte_size = static_cast<u32>(span);
    ev.addr = addr;
    ev.byte_size = span;
    VideoCore::DirectBuffers::PushEvent(ev);
}

void bb_direct_vk_register_native_buffer(uint32_t resource_id,
                                         const void* gpu_addr,
                                         uint32_t raw_byte_size,
                                         uint32_t alloc_byte_size,
                                         uint32_t bind_flags,
                                         uint32_t usage_type,
                                         uint32_t has_initial_data) noexcept {
    if (!gpu_addr || raw_byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    const u64 span = alloc_byte_size >= raw_byte_size ? alloc_byte_size : raw_byte_size;
    VideoCore::DirectBuffers::g_ever_registered.store(true, std::memory_order_release);
    VideoCore::DirectBuffers::NoteNativeGpuBlockAlloc(addr, span);
    VideoCore::DirectBuffers::MarkNativeOwnedRange(addr, span);
    VideoCore::DirectBuffers::NoteExactNativeBuffer(addr, raw_byte_size, static_cast<u32>(span),
                                                    true);
    VideoCore::DirectBuffers::MarkGuestImmutableRange(addr, span);
    VideoCore::DirectBuffers::Event ev{};
    ev.kind = (usage_type == 1u && has_initial_data != 0u)
                  ? VideoCore::DirectBuffers::EventKind::RegisterImmutable
                  : VideoCore::DirectBuffers::EventKind::RegisterNative;
    ev.usage_type = static_cast<u8>(usage_type);
    ev.has_initial_data = static_cast<u8>(has_initial_data != 0u ? 1u : 0u);
    ev.resource_id = resource_id;
    ev.bind_flags = bind_flags;
    ev.raw_byte_size = raw_byte_size;
    ev.alloc_byte_size = static_cast<u32>(span);
    ev.addr = addr;
    ev.byte_size = span;
    VideoCore::DirectBuffers::PushEvent(ev);
}

void bb_direct_vk_retire_buffer_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    const bool had_native = VideoCore::DirectBuffers::OverlapsNativeOwnedRange(addr, byte_size);
    if (had_native) {
        VideoCore::DirectBuffers::ClearNativeOwnedRange(addr, byte_size);
    }
    VideoCore::DirectBuffers::NoteTransientCueHeapFree(addr, byte_size);
    VideoCore::DirectBuffers::NoteNativeGpuBlockFree(addr, byte_size);
    if (auto* buf = VideoCore::DirectBuffers::g_active_buf_cache.load(std::memory_order_acquire)) {
        buf->DisarmWatchedRange(addr, byte_size);
    }
    const bool had_guest_64k = VideoCore::DirectBuffers::GuestImmutableIntersects(addr, byte_size);
    if (!had_native && !had_guest_64k) {
        return;
    }
    if (had_guest_64k) {
        VideoCore::DirectBuffers::ClearWholeGuestImmutablePages(addr, byte_size);
    }
    VideoCore::DirectBuffers::Event ev{};
    ev.kind = VideoCore::DirectBuffers::EventKind::RetireRange;
    ev.addr = addr;
    ev.byte_size = byte_size;
    VideoCore::DirectBuffers::PushEvent(ev);
}

void bb_direct_vk_register_transient_cue_heap(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    VideoCore::DirectBuffers::RegisterTransientCueHeap(reinterpret_cast<VAddr>(gpu_addr),
                                                       byte_size);
}

} // extern "C"
