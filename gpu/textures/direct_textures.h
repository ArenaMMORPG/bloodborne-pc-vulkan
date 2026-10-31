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
#include "gcn/sharps.h"

namespace Core {
class MemoryManager;
}

namespace VideoCore {

class TextureCache;

namespace DirectTextures {

inline constexpr u32 kSamplerTagMask = 0x3FFFF000u; // S# dword 3 hardware-reserved bits [29:12]
inline constexpr u32 kSamplerTagMagic = 0x2Au;      // 6-bit magic in S# dword 3 bits [29:24]
inline constexpr u32 kMaxNativeSamplers = 4096u;    // uid = 1..4095

inline constexpr u32 kSrvTagMagic = 0xB2u;          // 8-bit magic in T# dword 7 bits [31:24]
inline constexpr u32 kMaxNativeSrvs = 65536u;       // 16384 buckets x 4 ways, uid = 1..65535

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

// N2.1: Lock-free content-deduplicated native S# and T# descriptor tables.
u16 RegisterNativeSampler(void* ssharp_4dw) noexcept;
[[nodiscard]] u16 UnpackAndCleanSampler(Gcn::Sampler& ssharp) noexcept;

u16 RegisterNativeSrv(void* tsharp_8dw, u32 resource_id, bool is_immutable) noexcept;
[[nodiscard]] u16 UnpackAndCleanSrv(Gcn::Image& tsharp, u8& out_gen8) noexcept;

} // namespace DirectTextures
} // namespace VideoCore
