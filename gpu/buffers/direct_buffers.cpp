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
#include "render/runtime.h"
#include "render/scheduler.h"
#include "textures/texture_cache.h"

extern "C" void runtime_memory_note_write(uintptr_t address, uint64_t size);

namespace VideoCore::DirectBuffers {

namespace {

std::atomic<BufferCache*> g_active_buf_cache{nullptr};
std::atomic<TextureCache*> g_active_tex_cache{nullptr};
std::atomic<u32> g_pending_count{0};
std::atomic<bool> g_ever_registered{false};
std::mutex g_queue_mutex;
std::vector<Event> g_queue;

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

} // namespace

void SetActiveCaches(BufferCache* buf_cache, TextureCache* tex_cache) noexcept {
    g_active_buf_cache.store(buf_cache, std::memory_order_release);
    g_active_tex_cache.store(tex_cache, std::memory_order_release);
    if (!buf_cache) {
        std::scoped_lock lock{g_queue_mutex};
        g_queue.clear();
        g_pending_count.store(0, std::memory_order_release);
        g_ever_registered.store(false, std::memory_order_release);
    }
}

bool HasPendingEvents() noexcept {
    return g_pending_count.load(std::memory_order_acquire) != 0;
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
    : instance{instance_}, scheduler{scheduler_}, runtime{runtime_} {}

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

void Manager::ReclaimCompleted() noexcept {
    while (!retired.empty() && scheduler.IsFree(retired.front().first)) {
        retired.pop_front();
    }
}

void Manager::RetireOverlapping(VAddr addr, u64 size) {
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
        const VAddr e_end = e_start + it->second.alloc_size;
        if (e_end <= addr) {
            ++it;
            continue;
        }
        ClearMruFor(it->second.buffer.get());
        retired.emplace_back(tick, std::move(it->second.buffer));
        it = entries_by_base.erase(it);
        RebuildBitmapAroundErase(e_start, e_end);
    }
}

void Manager::OnGpuWrite(VAddr addr, u64 size) noexcept {
    if (entries_by_base.empty() || size == 0) {
        return;
    }
    RetireOverlapping(addr, size);
}

std::pair<const Buffer*, u64> Manager::TryResolveRead(VAddr addr, u64 size) noexcept {
    if (entries_by_base.empty() || size == 0 || addr >= kMaxGuestVa) {
        return {nullptr, 0};
    }

    for (const auto& slot : mru) {
        if (slot.buffer && addr >= slot.base_addr && size <= slot.alloc_size &&
            (addr - slot.base_addr) <= (slot.alloc_size - size)) {
            return {slot.buffer, addr - slot.base_addr};
        }
    }

    const u64 page = addr >> kPageShift64K;
    if ((bitmap_64k[page >> 6] & (1ULL << (page & 63ULL))) == 0ULL) {
        return {nullptr, 0};
    }

    auto it = entries_by_base.upper_bound(addr);
    if (it == entries_by_base.begin()) {
        return {nullptr, 0};
    }
    --it;
    const VAddr base = it->first;
    const u64 alloc_size = it->second.alloc_size;
    if (addr >= base && size <= alloc_size && (addr - base) <= (alloc_size - size)) {
        Buffer* buf = it->second.buffer.get();
        mru[mru_hand++ & 3u] = MruSlot{base, alloc_size, buf};
        return {buf, addr - base};
    }
    return {nullptr, 0};
}

void Manager::ProcessEvents(Core::MemoryManager* memory) {
    ReclaimCompleted();
    if (!HasPendingEvents()) {
        return;
    }
    static thread_local std::vector<Event> events;
    DrainEvents(events);
    if (events.empty()) {
        return;
    }

    // Coalesce transient buffers freed or overwritten later in the same batch.
    const size_t n = events.size();
    static thread_local std::vector<u8> superseded;
    superseded.assign(n, 0u);
    for (size_t i = 0; i < n; ++i) {
        if (events[i].kind != EventKind::RegisterImmutable) {
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

    for (size_t i = 0; i < n; ++i) {
        const auto& ev = events[i];
        if (ev.kind == EventKind::RetireRange) {
            if (ev.addr != 0 && ev.byte_size != 0) {
                RetireOverlapping(ev.addr, ev.byte_size);
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

        RetireOverlapping(ev.addr, guest_span);

        auto buf = std::make_unique<Buffer>(instance, ev.addr, vk_alloc_size,
                                            MemoryType::DeviceLocal, "DirectImmutableBuf");

        u64 copied = 0;
        while (copied < raw_size) {
            const u64 raw_chunk = std::min<u64>(raw_size - copied, kChunkBytes);
            const bool is_last = (copied + raw_chunk == raw_size);
            const u64 stage_chunk = is_last ? (vk_alloc_size - copied) : raw_chunk;

            const auto staging =
                runtime.GetStagingPool().Request(stage_chunk, MemoryType::HostUncached);
            std::memcpy(staging.mapped, reinterpret_cast<const u8*>(ev.addr + copied), raw_chunk);
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

        Buffer* raw_ptr = buf.get();
        Entry entry{};
        entry.resource_id = ev.resource_id;
        entry.bind_flags = ev.bind_flags;
        entry.raw_size = raw_size;
        entry.alloc_size = guest_span;
        entry.buffer = std::move(buf);
        entries_by_base[ev.addr] = std::move(entry);
        SetBitmapRange(ev.addr, ev.addr + guest_span);
        mru[mru_hand++ & 3u] = MruSlot{ev.addr, guest_span, raw_ptr};

        uploaded_bytes_since_flush += vk_alloc_size;
        if (uploaded_bytes_since_flush >= (32ULL << 20)) {
            runtime.FlushBarriers();
            Vulkan::SubmitInfo info{};
            scheduler.Flush(info);
            ReclaimCompleted();
            uploaded_bytes_since_flush = 0;
        }
    }
}

} // namespace VideoCore::DirectBuffers

extern "C" {

void bb_direct_vk_note_fresh_gpu_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    BbLayer::WriteTraps::Set(addr, byte_size, BbLayer::WriteTraps::Mirror, false);
    if (auto* tex = VideoCore::DirectBuffers::g_active_tex_cache.load(std::memory_order_acquire)) {
        tex->DisarmWatchedRange(addr, static_cast<size_t>(byte_size), /*mark_dirty=*/false);
    }
    if (auto* buf = VideoCore::DirectBuffers::g_active_buf_cache.load(std::memory_order_acquire)) {
        buf->NoteFreshRange(addr, byte_size);
    }
    if (VideoCore::DirectBuffers::g_ever_registered.load(std::memory_order_acquire)) {
        VideoCore::DirectBuffers::Event ev{};
        ev.kind = VideoCore::DirectBuffers::EventKind::RetireRange;
        ev.addr = addr;
        ev.byte_size = byte_size;
        VideoCore::DirectBuffers::PushEvent(ev);
    }
}

void bb_direct_vk_prepare_buffer_write(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    BbLayer::WriteTraps::Set(addr, byte_size, BbLayer::WriteTraps::Mirror, false);
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

void bb_direct_vk_register_immutable_buffer(uint32_t resource_id,
                                            const void* gpu_addr,
                                            uint32_t raw_byte_size,
                                            uint32_t alloc_byte_size,
                                            uint32_t bind_flags) noexcept {
    if (!gpu_addr || raw_byte_size == 0) {
        return;
    }
    VideoCore::DirectBuffers::g_ever_registered.store(true, std::memory_order_release);
    VideoCore::DirectBuffers::Event ev{};
    ev.kind = VideoCore::DirectBuffers::EventKind::RegisterImmutable;
    ev.resource_id = resource_id;
    ev.bind_flags = bind_flags;
    ev.raw_byte_size = raw_byte_size;
    ev.alloc_byte_size = alloc_byte_size >= raw_byte_size ? alloc_byte_size : raw_byte_size;
    ev.addr = reinterpret_cast<VAddr>(gpu_addr);
    ev.byte_size = ev.alloc_byte_size;
    VideoCore::DirectBuffers::PushEvent(ev);
}

void bb_direct_vk_retire_buffer_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    if (!VideoCore::DirectBuffers::g_ever_registered.load(std::memory_order_acquire)) {
        return;
    }
    VideoCore::DirectBuffers::Event ev{};
    ev.kind = VideoCore::DirectBuffers::EventKind::RetireRange;
    ev.addr = reinterpret_cast<VAddr>(gpu_addr);
    ev.byte_size = byte_size;
    VideoCore::DirectBuffers::PushEvent(ev);
}

} // extern "C"
