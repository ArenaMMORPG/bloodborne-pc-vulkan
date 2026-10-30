// SPDX-License-Identifier: GPL-2.0-or-later
// V3.2: Direct .tpf / .gnf Asset Texture Upload & 0-trap Free-List Retirement Queue.
//
// Bridges C++20 GXGnmTexture{1D,2D,3D}_{CreateAndInitialize,ReleaseMemory} and
// CGGpuFreeListAllocatorGnm_{FreeBlockImmediateA,ReclaimCompletedDeferredFrees} directly to
// VideoCore::TextureCache:
//   - Phase A (guest thread, synchronous): disarms watched WriteTraps::Image pages in user space
//     before CGMemoryGnm_Memcpy / SceGpuAddress_TileSurface or block reclamation, eliminating
//     SIGSEGV page-fault traps and DrainDrawPipe() stalls during asset streaming.
//   - Phase B (Stage B recording thread, stream order): retires stale images on freed heap blocks,
//     uploads immutable .tpf/.gnf asset textures straight into DEVICE_LOCAL VkImage outside
//     render passes, transitions them immediately to eShaderReadOnlyOptimal, and registers them in
//     direct_assets_by_addr without arming mprotect traps.
#pragma once

#include <array>
#include <vector>

#include "common/types.h"

namespace Core {
class MemoryManager;
}

namespace VideoCore {

class TextureCache;

namespace DirectTextures {

enum class EventKind : u8 {
    RetireRange = 0,
    RegisterAsset = 1,
};

struct Event {
    EventKind kind{EventKind::RetireRange};
    bool is_immutable{false};
    u32 resource_id{0};
    u32 raw_byte_size{0};
    u32 alloc_byte_size{0};
    VAddr addr{0};
    u64 byte_size{0};
    std::array<u32, 8> tsharp{};
};

void SetActiveCache(TextureCache* cache) noexcept;
[[nodiscard]] bool HasPendingEvents() noexcept;
void DrainEvents(std::vector<Event>& out);

} // namespace DirectTextures
} // namespace VideoCore
