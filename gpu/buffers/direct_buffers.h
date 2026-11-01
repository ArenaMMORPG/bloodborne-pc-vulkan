// SPDX-License-Identifier: GPL-2.0-or-later
// V3.3: Direct DEVICE_LOCAL VkBuffer Lifecycle & 0-trap Allocator/Loader Bridge.
//
// Bridges C++20 CGGpuFreeListAllocatorGnm_AllocateAlignedInternal,
// GXGnmBuffer_{CreateAndInitialize,ReleaseMemoryAndNotifyContexts}, and shader binary loaders
// directly to VideoCore::BufferCache and dedicated DEVICE_LOCAL VkBuffers:
//   - Phase A (guest thread, synchronous):
//       * NoteFreshRange on recycled free-list blocks so stale CPU/GPU residency flags clear and
//         newly loaded blocks promote cleanly into DEVICE_LOCAL VRAM mirrors.
//       * Disarms WriteTraps::Mirror & WriteTraps::Image before CPU buffer/shader/swapchain/query
//         initialization (0 SIGSEGV page faults) and records loaded asset_bytes via
//         runtime_memory_note_write.
//   - Phase B (Stage B recording thread, stream order):
//       * Uploads immutable GXGnmBuffers (usageType == 1, cpuAccessFlags == 0, no UAV/SO —
//         including 100% of combined FLVER model vertex + index buffers) into dedicated VMA
//         DEVICE_LOCAL VkBuffers and resolves arbitrary VB/IB/CB/SRV sub-ranges in O(1) via a
//         4-slot MRU cache + 128 KiB 64K-page presence bitmap.
#pragma once

#include <array>
#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "buffers/buffer.h"
#include "common/types.h"

namespace Core {
class MemoryManager;
}

namespace Vulkan {
class Instance;
class Runtime;
class Scheduler;
} // namespace Vulkan

namespace VideoCore {

class BufferCache;
class TextureCache;

namespace DirectBuffers {

enum class EventKind : u8 {
    RetireRange = 0,
    RegisterImmutable = 1,
    RegisterNative = 2,
};

struct Event {
    EventKind kind{EventKind::RetireRange};
    u8 usage_type{1};
    u8 has_initial_data{1};
    u16 pad0{0};
    u32 resource_id{0};
    u32 bind_flags{0};
    u32 raw_byte_size{0};
    u32 alloc_byte_size{0};
    VAddr addr{0};
    u64 byte_size{0};
};

struct NativeCopyRegion {
    VAddr dst_va{0};
    VAddr src_va{0};
    u32 byte_size{0};
    u32 flags{0};
};

void SetActiveCaches(BufferCache* buf_cache, TextureCache* tex_cache) noexcept;
[[nodiscard]] bool HasPendingEvents() noexcept;
void DrainEvents(std::vector<Event>& out);

/// V6.1/V6.2/V7.1: Registers the transient GX resource cache (`cueHeap`, up to 512 MiB CPU->GPU
/// staging ring) so DirectCopyBuffers and BufferCache::ObtainBuffer stream host RAM directly
/// without creating a 512 MiB VRAM mirror or arming mprotect(PROT_READ) page traps on cueHeap.
void RegisterTransientCueHeap(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsTransientCueHeap(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool OverlapsTransientCueHeap(VAddr addr, u64 byte_size) noexcept;
/// N2.5: Registers auxiliary transient CPU command/constant rings (`dcbRingBuffer`, `ccbRingBuffer`,
/// `CGGlobalRing` DCB/CCB & Yebis2 `g_GlobalCueHeap`) so inline command-buffer constants and
/// Yebis2 transient quad VBs/CBs stream directly into `stream_buffer` without allocating 64 MiB
/// `BufferCache` VRAM chunks, arming `mprotect` traps, or registering false `NatBuf` misses.
void RegisterTransientCpuRing(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsTransientAuxCpuRing(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsTransientCpuRingAddress(VAddr addr) noexcept;
void NoteTransientCueHeapGpuWrite(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsCueHeapEverGpuModified() noexcept;
void NoteTransientCueHeapAlloc(VAddr addr, u64 byte_size) noexcept;
void NoteTransientCueHeapFree(VAddr addr, u64 byte_size) noexcept;
/// V7.1: O(1) lock-free bounding of unbounded (0xFFFFFFFF) read-only V# descriptors inside cueHeap
/// to the end of their live CGGpuFreeListAllocatorGnm allocation block.
[[nodiscard]] u64 ClampTransientCueReadSize(VAddr addr, u64 requested_size) noexcept;
/// N1.3: O(1) lock-free recovery of the enclosing live `> 64 KiB` (`<= 4 MiB`) `MapDynamicBuffer_B`
/// `isDynamicRing` block `[out_block_base, out_block_base + out_block_size)` inside `cueHeap` so all
/// `NO_OVERWRITE` sub-draws share one single `stream_buffer` upload instead of re-copying 3..4 MiB.
[[nodiscard]] bool LookupTransientCueBlock(VAddr addr, u64 size, VAddr& out_block_base,
                                           u32& out_block_size) noexcept;
/// N2.4: Lock-free 64 KiB-granule write generation directory over cueHeap so 4/8-byte EoP /
/// WRITE_DATA status labels never falsely invalidate non-overlapping 4 MiB MapDynamicBuffer_B
/// dynamic vertex buffer uploads in g_tls_cue_memo.
void NoteTransientCueWrite(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] u64 ComputeTransientCueStamp(VAddr addr, u64 byte_size) noexcept;

/// N1.1 / N1.2 / N1.2c: Global 16 GiB lock-free NativeAllocDirectory ([0x1000000000, 0x1400000000)),
/// lossless 4-byte-granule NativeOwnedBitmap (0 collisions, 0 evictions, O(1) interior-offset hit),
/// and 262,144-slot Murmur3-avalanched ExactBufferTable tracking every live GXGnmBuffer (exact
/// requestedSize, allocSize, and native VkBuffer ownership) so unbounded 0xFFFFFFFF V# descriptors
/// clamp to the exact byte size of their GXGnmBuffer (zero 4 KiB neighbor overshoot) and Stage A /
/// DrawPrep workers skip ConstRing guest-RAM memcpy for 100% of native-owned buffers.
void NoteNativeGpuBlockAlloc(VAddr addr, u64 byte_size) noexcept;
void NoteNativeGpuBlockFree(VAddr addr, u64 byte_size) noexcept;
void MarkNativeOwnedRange(VAddr addr, u64 byte_size) noexcept;
void ClearNativeOwnedRange(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsNativeOwnedAddress(VAddr addr) noexcept;
[[nodiscard]] bool OverlapsNativeOwnedRange(VAddr addr, u64 byte_size) noexcept;
void NoteExactNativeBuffer(VAddr addr, u32 req_size, u32 alloc_size, bool is_native_owned) noexcept;
void SetExactNativeBufferOwned(VAddr addr, bool is_native_owned) noexcept;
[[nodiscard]] bool IsExactNativeOwnedBuffer(VAddr addr) noexcept;
[[nodiscard]] VAddr LookupExactBufferEnd(VAddr addr) noexcept;
[[nodiscard]] u64 ClampNativeGpuReadSize(VAddr addr, u64 requested_size,
                                         bool for_write = false) noexcept;

class Manager {
public:
    Manager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
            Vulkan::Runtime& runtime);
    ~Manager();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    [[nodiscard]] bool NeedsProcessEvents(bool is_rendering) const noexcept {
        return HasPendingEvents() || (!is_rendering && !deferred_events.empty());
    }
    [[nodiscard]] bool HasPendingWritebacks() const noexcept {
        return !dirty_writeback_bases.empty();
    }
    [[nodiscard]] u64 EraseGeneration() const noexcept {
        return erase_generation;
    }

    /// Stage B: drains pending RegisterImmutable / RegisterNative / RetireRange events in exact
    /// stream order without breaking active RenderPasses, and reclaims retired buffers whose GPU
    /// tick has completed.
    void ProcessEvents(Core::MemoryManager* memory);

    /// N1.1 / N1.2 (~2 ns fast path): resolves a read-only [addr, addr + inout_size) binding
    /// inside a live native VkBuffer (or its active zero-break stream_buffer version), clamping
    /// oversized/0xFFFFFFFF V# sizes to the exact end of that GXGnmBuffer.
    [[nodiscard]] std::pair<const Buffer*, u64> TryResolveReadClamped(VAddr addr,
                                                                      u64& inout_size) noexcept;
    [[nodiscard]] std::pair<const Buffer*, u64> TryResolveRead(VAddr addr, u64 size) noexcept {
        return TryResolveReadClamped(addr, size);
    }

    /// N2.5: O(1) lookup of a valid CPU-side mirror pointer (`cpu_shadow` or untouched
    /// initial-data guest RAM) for `[addr, addr + size)` when scanning index buffers of dynamic
    /// `cueHeap` vertex draws; returns `nullptr` if the buffer was GPU-written without a shadow.
    [[nodiscard]] const u8* TryGetCpuMirrorPtr(VAddr addr, u64 size) noexcept;

    /// N1.2 / N1.3: Resolves a non-stream GPU write [addr, addr + size) directly into a native
    /// VkBuffer (flushing any active stream override first and arming WAR barrier tracking when
    /// `track_access` is true). If a write partially crosses buffer boundaries, retires the entry
    /// and returns {nullptr, 0}.
    [[nodiscard]] std::pair<const Buffer*, u64> TryResolveWrite(
        VAddr addr, u64 size, bool full_overwrite = false, bool track_access = true,
        const TextureCache* tex_cache = nullptr);

    /// N1.2b: Renames full-buffer CPU-source `DirectCopyBuffers` updates (`FlushBufferUpdates_A/B`)
    /// of native `DEFAULT` `GXGnmBuffer`s directly into `stream_buffer` without calling
    /// `EndRendering()` or `BufferCache::ObtainBuffer`, coalescing the final deduplicated writeback
    /// into the persistent native `VkBuffer` once per command submission (`Flush()` / `OnSubmit()`).
    /// Sets `out_handled[idx] = 1` for each handled region (supports arbitrary batch sizes).
    void StageNativeCpuCopyBatch(std::span<const NativeCopyRegion> regions,
                                 std::span<u8> out_handled,
                                 BufferCache& buf_cache,
                                 TextureCache& tex_cache,
                                 Core::MemoryManager* memory);

    /// N1.2b: Writes the latest deduplicated `stream_buffer` version of each dirty native buffer
    /// back into its persistent `DEVICE_LOCAL` `VkBuffer` once per submission outside RenderPasses.
    void FlushPendingWritebacks();

    /// Stage B: checks whether [addr, addr + size) overlaps any live Stage B native VkBuffer entry.
    [[nodiscard]] bool OverlapsLiveEntry(VAddr addr, u64 size) const noexcept;

    /// Stage B: invalidates any native VkBuffer overlapping an external/UAV GPU write.
    void OnGpuWrite(VAddr addr, u64 size) noexcept;

    static constexpr u64 kMaxGuestVa = 1ULL << 40; // 1 TiB (40-bit PS4 VA space, matches BbMem::AddressBits = 40)
    static constexpr u32 kPageShift64K = 16u;
    static constexpr size_t kBitmapWords =
        static_cast<size_t>((kMaxGuestVa >> kPageShift64K) / 64ULL); // 262144 u64s = 2 MiB

private:

    struct Entry {
        u32 resource_id{0};
        u32 bind_flags{0};
        u8 usage_type{1};
        bool dirty_writeback{false};
        bool needs_zero_init{false};
        bool has_cpu_shadow{false};
        bool zero_known{false};
        bool gpu_written_without_shadow{false};
        bool noted_ext_gpu_mod{false};
        u64 raw_size{0};
        u64 alloc_size{0};
        u64 verified_no_tex_gen{0};
        const Buffer* stream_override_buf{nullptr};
        u64 stream_override_off{0};
        std::unique_ptr<u8[]> cpu_shadow{};
        std::unique_ptr<Buffer> buffer;
    };

    struct MruSlot {
        VAddr base_addr{0};
        u64 alloc_size{0};
        u64 erase_gen{0};
        Entry* entry{nullptr};
    };

    [[nodiscard]] static constexpr size_t MruHash(VAddr addr) noexcept {
        return static_cast<size_t>(((addr >> 6) ^ (addr >> 15) ^ (addr >> 23)) & (kMruSlots - 1));
    }
    void InsertMru(VAddr query_addr, VAddr base_addr, u64 alloc_size, Entry* entry) noexcept {
        const size_t h0 = MruHash(query_addr);
        const size_t h1 = (h0 + 1u) & (kMruSlots - 1);
        if (mru[h0].base_addr != base_addr || mru[h0].erase_gen != erase_generation) {
            mru[h1] = mru[h0];
        }
        mru[h0] = MruSlot{base_addr, alloc_size, erase_generation, entry};
    }

    void SetBitmapRange(VAddr start, VAddr end) noexcept;
    void RebuildBitmapAroundErase(VAddr erased_start, VAddr erased_end) noexcept;
    [[nodiscard]] bool AnyBitmap64k(VAddr addr, u64 size) const noexcept;
    void RetireOverlapping(VAddr addr, u64 size, bool clear_global_owned = true);
    void ReclaimCompleted() noexcept;
    void ClearMruFor(const Buffer* buf) noexcept;
    void UpdateMruEntry(VAddr base_addr, u64 alloc_size, const Buffer* buf,
                        u64 base_offset) noexcept;

    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    Vulkan::Runtime& runtime;

    static constexpr size_t kMruSlots = 16384;
    static constexpr u64 kSharedZeroBytes = 1ULL << 20; // 1 MiB shared zero-filled VkBuffer
    std::unique_ptr<Buffer> zero_buffer;
    std::map<VAddr, Entry> entries_by_base;
    std::vector<VAddr> dirty_writeback_bases;
    std::vector<Event> deferred_events;
    std::deque<std::pair<u64, std::unique_ptr<Buffer>>> retired;
    std::vector<u64> bitmap_64k;
    std::vector<MruSlot> mru;
    u64 erase_generation{1};
    u32 mru_hand{0};
};

} // namespace DirectBuffers
} // namespace VideoCore
