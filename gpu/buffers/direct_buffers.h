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
};

struct Event {
    EventKind kind{EventKind::RetireRange};
    u32 resource_id{0};
    u32 bind_flags{0};
    u32 raw_byte_size{0};
    u32 alloc_byte_size{0};
    VAddr addr{0};
    u64 byte_size{0};
};

void SetActiveCaches(BufferCache* buf_cache, TextureCache* tex_cache) noexcept;
[[nodiscard]] bool HasPendingEvents() noexcept;
void DrainEvents(std::vector<Event>& out);

/// V6.1/V6.2: Registers the transient GX resource cache (`cueHeap`, up to 512 MiB CPU->GPU staging ring)
/// so DirectCopyBuffers and BufferCache::ObtainBuffer stream host RAM directly without creating a
/// 512 MiB VRAM mirror or arming mprotect(PROT_READ) page traps on cueHeap.
void RegisterTransientCueHeap(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsTransientCueHeap(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool OverlapsTransientCueHeap(VAddr addr, u64 byte_size) noexcept;
void NoteTransientCueHeapGpuWrite(VAddr addr, u64 byte_size) noexcept;
[[nodiscard]] bool IsCueHeapEverGpuModified() noexcept;

class Manager {
public:
    Manager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
            Vulkan::Runtime& runtime);
    ~Manager();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    /// Stage B: drains pending RegisterImmutable / RetireRange events, uploads immutable buffers
    /// into DEVICE_LOCAL VRAM outside render passes, and reclaims retired buffers whose GPU tick
    /// has completed.
    void ProcessEvents(Core::MemoryManager* memory);

    /// Stage B (~2 ns fast path): resolves a read-only [addr, addr + size) sub-range inside a
    /// live DirectImmutableBuf. Returns {nullptr, 0} on miss so BufferCache handles dynamic/UAV
    /// ranges normally.
    [[nodiscard]] std::pair<const Buffer*, u64> TryResolveRead(VAddr addr, u64 size) noexcept;

    /// Stage B: invalidates any DirectImmutableBuf overlapping a GPU write [addr, addr + size) so
    /// stale immutable data can never shadow subsequent GPU-written bytes.
    void OnGpuWrite(VAddr addr, u64 size) noexcept;

    static constexpr u64 kMaxGuestVa = 1ULL << 36; // 64 GiB guest VA coverage
    static constexpr u32 kPageShift64K = 16u;
    static constexpr size_t kBitmapWords =
        static_cast<size_t>((kMaxGuestVa >> kPageShift64K) / 64ULL); // 16384 u64s = 128 KiB

private:

    struct Entry {
        u32 resource_id{0};
        u32 bind_flags{0};
        u64 raw_size{0};
        u64 alloc_size{0};
        std::unique_ptr<Buffer> buffer;
    };

    struct MruSlot {
        VAddr base_addr{0};
        u64 alloc_size{0};
        Buffer* buffer{nullptr};
    };

    void SetBitmapRange(VAddr start, VAddr end) noexcept;
    void RebuildBitmapAroundErase(VAddr erased_start, VAddr erased_end) noexcept;
    [[nodiscard]] bool AnyBitmap64k(VAddr addr, u64 size) const noexcept;
    void RetireOverlapping(VAddr addr, u64 size);
    void ReclaimCompleted() noexcept;
    void ClearMruFor(const Buffer* buf) noexcept;

    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    Vulkan::Runtime& runtime;

    std::map<VAddr, Entry> entries_by_base;
    std::deque<std::pair<u64, std::unique_ptr<Buffer>>> retired;
    std::array<u64, kBitmapWords> bitmap_64k{};
    std::array<MruSlot, 4> mru{};
    u32 mru_hand{0};
};

} // namespace DirectBuffers
} // namespace VideoCore
