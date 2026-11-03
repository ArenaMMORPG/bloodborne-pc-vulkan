// ============================================================================
// Bloodborne (CUSA03173 v1.09) - CoreGraphics2 GNM Backend
// Module: 01_CoreGraphics2_GNM
// Range:  0x26b2d30 .. 0x26b984f (62 functions, 26,507 bytes)
//
// Reconstructed classes & subsystems:
//   1. `CGSurfaceSyncTrackerGnm` Read/Write Resource Transitions (`0x26b8260 .. 0x26b984f`):
//      Tracks read/write hazard transitions across all 6 shader stages and render/depth/UAV
//      targets, computing `sce::Gnm::TextureChannel` swizzle masks (`0x6E0000`, `0x6E0001`,
//      `0x600001`, `0x100000`) and accumulating 256-byte aligned barrier address ranges.
//   2. `CGFormatAndStateTranslatorGnm` (`0x26b7310 .. 0x26b81b0`):
//      43 lookup & conversion functions translating Bloodborne's cross-platform D3D11-like
//      `CGFormat`, `CGPrimitiveTopology`, `CGBlendOp`, `CGComparisonFunc`, `CGFilterMode`,
//      vertex element formats, and shader semantic/constant-buffer names (`$Globals`, `$Params`)
//      into native PS4 `sce::Gnm` hardware register encodings.
//   3. `CGGpuBlitAndClearPassesGnm` (`0x26b2d30 .. 0x26b6cb0`):
//      12 built-in GPU utility passes (MSAA resolve `0x26b6cb0`, render-target slice clear
//      `0x26b6810`, HTILE/depth-stencil clear `0x26b5f50`, fullscreen RT/depth copy `0x26b5a70` /
//      `0x26b55a0`, compute UAV `uint4` clear `0x26b5380` / `0x26b4ef0`, CPU-to-tiled texture
//      upload `0x26b4230` / `0x26b35e0`, 1D/2D/3D compute texture region copy `0x26b2f40`, and
//      compute buffer region copy `0x26b2d30`).
// ============================================================================

#include <cstdint>
#include <cstddef>
#include "gx_direct_vulkan.h"
#include <cstring>

namespace Bloodborne::CoreGraphics2::Gnm {

// ============================================================================
// External GNM SDK & Engine Helpers
// ============================================================================

extern "C" {
uint32_t CGGpuAddressGnm_Pack256ByteAlignedA(uint64_t byte_addr); // 0x26baed0
uint32_t CGGpuAddressGnm_Pack256ByteAlignedB(uint64_t byte_addr); // 0x26baeb0
int32_t  sub_2bbe568_strcmp(const char* a, const char* b);        // 0x2bbe568
}

#define GX_CALL(ret, vma, sig, ...) \
    (reinterpret_cast<ret(*) sig>(GX_VMA(vma))(__VA_ARGS__))

static inline void* sub_2bbe578_memcpy(void* dst, const void* src, size_t n) {
    return GX_CALL(void*, 0x2bbe578ULL, (void*, const void*, size_t), dst, src, n);
}

// ============================================================================
// 1. CGSurfaceSyncTrackerGnm - Resource Read/Write Transitions (0x26b8260 .. 0x26b984f)
// ============================================================================

struct CGSurfaceSyncTrackerState {
    uint32_t pending_event_mode; // +0x00
    uint32_t event_cache_action; // +0x04
    uint32_t event_stall_mode;   // +0x08
    uint32_t gfx_min_addr_256;   // +0x0c
    uint32_t gfx_max_addr_256;   // +0x10
    uint32_t cs_min_addr_256;    // +0x14
    uint32_t cs_max_addr_256;    // +0x18
    uint32_t target_mask_flags;  // +0x1c
    uint32_t cache_sync_mode;    // +0x20
    uint8_t  is_disabled;        // +0x24
};

// VMA: 0x26b8f80 (2252 B)
// Records a write hazard transition (`usage_a` `0..5`, `usage_b` `0..5`,
// `swizzle_code`, `[gpu_addr, gpu_addr + byte_size)`, `target_mask`) into `CGSurfaceSyncTrackerState`.
static inline void CGSurfaceSyncTrackerGnm_AssertInvalidStage(uint32_t line_no) {
    reinterpret_cast<void(*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
        reinterpret_cast<const char*>(GX_VMA(0x487000cULL)),
        line_no,
        reinterpret_cast<const char*>(GX_VMA(0x487008fULL)));
}

static inline void CGSurfaceSyncTrackerGnm_AccumGfxRange(
    CGSurfaceSyncTrackerState* self,
    uint64_t gpu_addr,
    uint64_t byte_size,
    uint32_t target_mask)
{
    if (gpu_addr == 0) {
        self->cache_sync_mode |= 0x07u;
    } else {
        self->cache_sync_mode |= 0x02u;
        const uint32_t s256 = CGGpuAddressGnm_Pack256ByteAlignedB(gpu_addr);
        const uint32_t e256 = s256 + CGGpuAddressGnm_Pack256ByteAlignedA(byte_size);
        if (s256 < self->gfx_min_addr_256) self->gfx_min_addr_256 = s256;
        if (e256 > self->gfx_max_addr_256) self->gfx_max_addr_256 = e256;
        self->target_mask_flags |= target_mask;
    }
}

static inline void CGSurfaceSyncTrackerGnm_AccumCsRange(
    CGSurfaceSyncTrackerState* self,
    uint64_t gpu_addr,
    uint64_t byte_size)
{
    if (gpu_addr == 0) {
        self->cache_sync_mode |= 0x07u;
    } else {
        self->cache_sync_mode |= 0x02u;
        const uint32_t s256 = CGGpuAddressGnm_Pack256ByteAlignedB(gpu_addr);
        const uint32_t e256 = s256 + CGGpuAddressGnm_Pack256ByteAlignedA(byte_size);
        if (s256 < self->cs_min_addr_256) self->cs_min_addr_256 = s256;
        if (e256 > self->cs_max_addr_256) self->cs_max_addr_256 = e256;
        reinterpret_cast<uint8_t*>(self)[0x1d] |= 0x01u;
    }
}

static inline void CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(
    CGSurfaceSyncTrackerState* self,
    uint32_t swizzle_code)
{
    if (swizzle_code <= 0x600000u) {
        if (swizzle_code == 0x100000u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x0Fu;
            reinterpret_cast<uint8_t*>(self)[0x07] |= 0x08u;
        }
    } else if (swizzle_code <= 0x6D0100u) {
        if (swizzle_code == 0x600001u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x0Fu;
        }
    } else if (swizzle_code <= 0x6DFFFFu) {
        if (swizzle_code == 0x6D0101u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x0Fu;
        }
    } else {
        if (swizzle_code == 0x6E0000u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x03u;
            reinterpret_cast<uint8_t*>(self)[0x07] |= 0x08u;
        } else if (swizzle_code == 0x6E0001u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x03u;
        } else if (swizzle_code == 0x6E0101u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x02u;
        }
    }
}

static inline void CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(
    CGSurfaceSyncTrackerState* self,
    uint32_t swizzle_code,
    bool merge_6e0000_into_04)
{
    if (swizzle_code <= 0x6D0100u) {
        if (swizzle_code == 0x600001u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x0Fu;
        }
    } else if (swizzle_code <= 0x6DFFFFu) {
        if (swizzle_code == 0x6D0101u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x0Fu;
        }
    } else {
        if (swizzle_code == 0x6E0000u || swizzle_code == 0x6E0001u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= (merge_6e0000_into_04 ? 0x04u : 0x05u);
        } else if (swizzle_code == 0x6E0101u) {
            reinterpret_cast<uint8_t*>(self)[0x00] |= 0x04u;
        }
    }
}

void CGSurfaceSyncTrackerGnm_TransitionWriteAccess(
    CGSurfaceSyncTrackerState* self,
    uint32_t access_category,
    uint32_t stage_kind,
    uint32_t swizzle_code,
    uint64_t gpu_addr,
    uint64_t byte_size,
    uint32_t target_mask)
{
    if (access_category > 5u) {
        CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x190u);
        return;
    }

    switch (access_category) {
    case 0:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x145u);
            return;
        }
        reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
        switch (stage_kind) {
        case 0:
            if (swizzle_code == 0x6E0001u || swizzle_code == 0x600001u) {
                reinterpret_cast<uint8_t*>(self)[0x00] |= 0x01u;
            } else if (swizzle_code == 0x6E0000u) {
                reinterpret_cast<uint8_t*>(self)[0x00] |= 0x01u;
                reinterpret_cast<uint8_t*>(self)[0x07] |= 0x08u;
            }
            break;
        case 1:
        case 2:
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, false);
            break;
        case 3:
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, false);
            self->event_stall_mode = 0;
            break;
        case 4:
        case 5:
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, true);
            break;
        }
        break;

    case 1:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x153u);
            return;
        }
        switch (stage_kind) {
        case 0:
            CGSurfaceSyncTrackerGnm_AccumGfxRange(self, gpu_addr, byte_size, target_mask);
            CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
            break;
        case 1:
            break;
        case 2:
        case 4:
        case 5:
            CGSurfaceSyncTrackerGnm_AccumGfxRange(self, gpu_addr, byte_size, target_mask);
            break;
        case 3:
            CGSurfaceSyncTrackerGnm_AccumGfxRange(self, gpu_addr, byte_size, target_mask);
            self->event_stall_mode = 0;
            break;
        }
        break;

    case 2:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x161u);
            return;
        }
        switch (stage_kind) {
        case 0:
            CGSurfaceSyncTrackerGnm_AccumCsRange(self, gpu_addr, byte_size);
            CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
            break;
        case 1:
        case 4:
        case 5:
            CGSurfaceSyncTrackerGnm_AccumCsRange(self, gpu_addr, byte_size);
            break;
        case 2:
            break;
        case 3:
            CGSurfaceSyncTrackerGnm_AccumCsRange(self, gpu_addr, byte_size);
            self->event_stall_mode = 0;
            break;
        }
        break;

    case 3:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x16Fu);
            return;
        }
        reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
        break;

    case 4:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x17Du);
            return;
        }
        if (((0x36u >> stage_kind) & 1u) != 0) {
            return;
        }
        if (stage_kind == 0u) {
            CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
        } else if (stage_kind == 3u) {
            self->event_stall_mode = 0;
        }
        break;

    case 5:
        if ((stage_kind - 1u) < 5u) {
            return;
        }
        if (stage_kind != 0u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x18Bu);
            return;
        }
        CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
        break;
    }
}

// VMA: 0x26b8e20 (344 B)
// Transitions a resource (`[res + 0x08]` GPU address, `[res + 0x14]` byte size, `[res + 0x23]`
// usage flags) for write access at `stage_kind` (`0..5`), returning its channel swizzle code.
uint32_t CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(
    CGSurfaceSyncTrackerState* self,
    uint32_t stage_kind,
    const uint8_t* resource)
{
    const uint8_t usage_flags = resource[0x23];
    const uint32_t ext_flags =
        ((*reinterpret_cast<const uint32_t*>(resource + 0x24) & 0x08u) << 5) | usage_flags;

    uint32_t swizzle = 0x6E0000u;
    if ((usage_flags & 0x04u) == 0) {
        if ((usage_flags & 0x60u) != 0) {
            swizzle = 0x600001u;
        } else if ((ext_flags & 0x102u) != 0) {
            swizzle = 0x6E0001u;
        } else if ((usage_flags & 0x90u) != 0) {
            swizzle = 0x600001u;
        } else if (resource[0x21] == 2u) {
            swizzle = 0x6E0001u;
        } else {
            swizzle = 0x100000u;
        }
    }

    if ((usage_flags & 0x9Du) != 0) {
        CGSurfaceSyncTrackerGnm_TransitionWriteAccess(self, 0, stage_kind, swizzle, 0, 0, 0);
    }
    if ((usage_flags & 0x20u) != 0) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08);
        const uint32_t byte_size = *reinterpret_cast<const uint32_t*>(resource + 0x14);
        CGSurfaceSyncTrackerGnm_TransitionWriteAccess(self, 1, stage_kind, swizzle, gpu_addr, byte_size, 0xFFu);
    }
    if ((usage_flags & 0x40u) != 0) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08);
        const uint32_t byte_size = *reinterpret_cast<const uint32_t*>(resource + 0x14);
        CGSurfaceSyncTrackerGnm_TransitionWriteAccess(self, 2, stage_kind, swizzle, gpu_addr, byte_size, 0x100u);
    }
    if ((ext_flags & 0x102u) != 0) {
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x16Fu);
        } else {
            reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
        }
    }
    return swizzle;
}

// VMA: 0x26b8420 (2152 B)
// Records a read hazard transition (`access_category` `0..5`, `stage_kind` `0..5`,
// `swizzle_code`, `[gpu_addr, gpu_addr + byte_size)`, `target_mask`).
void CGSurfaceSyncTrackerGnm_TransitionReadAccess(
    CGSurfaceSyncTrackerState* self,
    uint32_t access_category,
    uint32_t stage_kind,
    uint32_t swizzle_code,
    uint64_t gpu_addr,
    uint64_t byte_size,
    uint32_t target_mask)
{
    if (access_category > 5u) {
        CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1F1u);
        return;
    }

    switch (access_category) {
    case 0:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1A6u);
            return;
        }
        reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
        switch (stage_kind) {
        case 0:
            if (swizzle_code == 0x6E0001u || swizzle_code == 0x600001u) {
                reinterpret_cast<uint8_t*>(self)[0x00] |= 0x01u;
            } else if (swizzle_code == 0x6E0000u) {
                reinterpret_cast<uint8_t*>(self)[0x00] |= 0x01u;
                reinterpret_cast<uint8_t*>(self)[0x07] |= 0x08u;
            }
            break;
        case 1:
        case 2:
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, false);
            break;
        case 3:
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, false);
            self->event_stall_mode = 0;
            break;
        case 4:
            break;
        case 5:
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, true);
            break;
        }
        break;

    case 1:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1B4u);
            return;
        }
        switch (stage_kind) {
        case 0:
            CGSurfaceSyncTrackerGnm_AccumGfxRange(self, gpu_addr, byte_size, target_mask);
            CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
            break;
        case 1:
            break;
        case 2:
        case 4:
        case 5:
            CGSurfaceSyncTrackerGnm_AccumGfxRange(self, gpu_addr, byte_size, target_mask);
            break;
        case 3:
            CGSurfaceSyncTrackerGnm_AccumGfxRange(self, gpu_addr, byte_size, target_mask);
            self->event_stall_mode = 0;
            break;
        }
        break;

    case 2:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1C2u);
            return;
        }
        switch (stage_kind) {
        case 0:
            CGSurfaceSyncTrackerGnm_AccumCsRange(self, gpu_addr, byte_size);
            CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
            break;
        case 1:
        case 4:
        case 5:
            CGSurfaceSyncTrackerGnm_AccumCsRange(self, gpu_addr, byte_size);
            break;
        case 2:
            break;
        case 3:
            CGSurfaceSyncTrackerGnm_AccumCsRange(self, gpu_addr, byte_size);
            self->event_stall_mode = 0;
            break;
        }
        break;

    case 3:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1D0u);
            return;
        }
        reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
        break;

    case 4:
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1DEu);
            return;
        }
        if (((0x36u >> stage_kind) & 1u) != 0) {
            return;
        }
        if (stage_kind == 0u) {
            CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
        } else if (stage_kind == 3u) {
            self->event_stall_mode = 0;
        }
        break;

    case 5:
        if ((stage_kind - 1u) < 5u) {
            return;
        }
        if (stage_kind != 0u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1ECu);
            return;
        }
        CGSurfaceSyncTrackerGnm_ApplySwizzleForTargetToShader(self, swizzle_code);
        break;
    }
}

// VMA: 0x26b8c90 (400 B)
// Records post-operation hazard synchronization for `resource` after a blit/compute pass.
void CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(
    CGSurfaceSyncTrackerState* self,
    uint32_t swizzle_code,
    uint32_t stage_kind,
    const uint8_t* resource)
{
    const uint8_t usage_flags = resource[0x23];
    const uint32_t ext_flags =
        (*reinterpret_cast<const uint32_t*>(resource + 0x24) << 5) | usage_flags;

    if ((usage_flags & 0x9Du) != 0) {
        CGSurfaceSyncTrackerGnm_TransitionReadAccess(self, stage_kind, 0, swizzle_code, 0, 0, 0);
    }
    if ((usage_flags & 0x20u) != 0) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08);
        const uint32_t byte_size = *reinterpret_cast<const uint32_t*>(resource + 0x14);
        CGSurfaceSyncTrackerGnm_TransitionReadAccess(self, stage_kind, 1, swizzle_code, gpu_addr, byte_size, 0xFFu);
    }
    if ((usage_flags & 0x40u) != 0) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08);
        const uint32_t byte_size = *reinterpret_cast<const uint32_t*>(resource + 0x14);
        CGSurfaceSyncTrackerGnm_TransitionReadAccess(self, stage_kind, 2, swizzle_code, gpu_addr, byte_size, 0x100u);
    }
    if ((ext_flags & 0x102u) != 0) {
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1F1u);
            return;
        }
        switch (stage_kind) {
        case 0:
            reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
            CGSurfaceSyncTrackerGnm_ApplySwizzleForShaderToTarget(self, swizzle_code, false);
            self->event_stall_mode = 0;
            break;
        case 1:
        case 2:
            reinterpret_cast<uint8_t*>(self)[0x20] |= 0x07u;
            self->event_stall_mode = 0;
            break;
        case 3:
            reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
            break;
        case 4:
            self->event_stall_mode = 0;
            break;
        case 5:
            break;
        }
    }
}

// VMA: 0x26b82b0 (368 B)
// Transitions a resource for read access at `stage_kind` (`0..5`) and returns its swizzle code.
uint32_t CGSurfaceSyncTrackerGnm_TransitionResourceForRead(
    CGSurfaceSyncTrackerState* self,
    uint32_t stage_kind,
    const uint8_t* resource)
{
    const uint8_t usage_flags = resource[0x23];
    const uint32_t ext_flags =
        ((*reinterpret_cast<const uint32_t*>(resource + 0x24) & 0x08u) << 5) | usage_flags;

    uint32_t swizzle = 0x6E0000u;
    if ((usage_flags & 0x04u) == 0) {
        if ((usage_flags & 0x60u) != 0) {
            swizzle = 0x600001u;
        } else if ((ext_flags & 0x102u) != 0) {
            swizzle = 0x6E0001u;
        } else if ((usage_flags & 0x90u) != 0) {
            swizzle = 0x600001u;
        } else if (resource[0x21] == 2u) {
            swizzle = 0x6E0001u;
        } else {
            swizzle = 0x100000u;
        }
    }

    if ((usage_flags & 0x9Du) != 0) {
        CGSurfaceSyncTrackerGnm_TransitionReadAccess(self, 0, stage_kind, swizzle, 0, 0, 0);
    }
    if ((usage_flags & 0x20u) != 0) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08);
        const uint32_t byte_size = *reinterpret_cast<const uint32_t*>(resource + 0x14);
        CGSurfaceSyncTrackerGnm_TransitionReadAccess(self, 1, stage_kind, swizzle, gpu_addr, byte_size, 0xFFu);
    }
    if ((usage_flags & 0x40u) != 0) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08);
        const uint32_t byte_size = *reinterpret_cast<const uint32_t*>(resource + 0x14);
        CGSurfaceSyncTrackerGnm_TransitionReadAccess(self, 2, stage_kind, swizzle, gpu_addr, byte_size, 0x100u);
    }
    if ((ext_flags & 0x102u) != 0) {
        if (stage_kind > 5u) {
            CGSurfaceSyncTrackerGnm_AssertInvalidStage(0x1D0u);
        } else {
            reinterpret_cast<uint8_t*>(self)[0x20] |= 0x01u;
        }
    }

    if (swizzle == 0x100000u && stage_kind == 0u) {
        swizzle = 0x600001u;
    }
    return swizzle;
}

// VMA: 0x26b82a0 (1 B)
void CGSurfaceSyncTrackerGnm_NoOp() {}

// VMA: 0x26b8260 (55 B)
// Resets `CGSurfaceSyncTrackerState` to its empty default state.
void CGSurfaceSyncTrackerGnm_Reset(CGSurfaceSyncTrackerState* self) {
    self->pending_event_mode = 0;
    self->event_cache_action = 0;
    self->event_stall_mode   = 1;
    self->gfx_min_addr_256   = 0xFFFFFFFFu;
    self->gfx_max_addr_256   = 0;
    self->cs_min_addr_256    = 0xFFFFFFFFu;
    self->cs_max_addr_256    = 0;
    self->target_mask_flags  = 0;
    self->cache_sync_mode    = 0;
    self->is_disabled        = 0;
}

// ============================================================================
// 2. CGFormatAndStateTranslatorGnm (0x26b7310 .. 0x26b81b0)
// ============================================================================

// VMA: 0x26b81b0 (22 B)
uint32_t CGTranslatorGnm_TranslateBorderColor(uint32_t border_color_enum) {
    static const uint32_t kTable[7] = { 0, 1, 2, 4, 0, 0, 3 };
    return (border_color_enum <= 6u) ? kTable[border_color_enum] : 0u;
}

// VMA: 0x26b8100 (171 B)
// Combines `(min_filter, mag_filter, mip_mode)` into `sce::Gnm::FilterMode` (`0..8`).
uint32_t CGTranslatorGnm_TranslateSamplerFilterMode(
    uint32_t min_filter,
    uint32_t mag_filter,
    uint32_t mip_mode)
{
    if (min_filter == 0 && mag_filter == 0) {
        if (mip_mode == 1) return 0;
        if (mip_mode == 2) return 1;
    }
    if (min_filter == 0 && mag_filter == 1) {
        if (mip_mode == 1) return 2;
        if (mip_mode == 2) return 3;
    }
    if (min_filter == 1 && mag_filter == 0) {
        if (mip_mode == 1) return 4;
        if (mip_mode == 2) return 5;
    }
    if (min_filter == 1 && mag_filter == 1 && (mip_mode == 1 || mip_mode == 2)) {
        return 6;
    }
    if (min_filter == 3 && mag_filter == 3 && mip_mode == 2) {
        return 8;
    }
    return 0;
}

// VMA: 0x26b80f0 (9 B)
uint32_t CGTranslatorGnm_ClampFilterType(uint32_t val) {
    return (val < 3u) ? val : 0u;
}

// VMA: 0x26b80e0 (10 B)
uint32_t CGTranslatorGnm_IsNotPointMipMode(uint32_t val) {
    return (val != 2u) ? 1u : 0u;
}

// VMA: 0x26b80c0 (22 B)
uint32_t CGTranslatorGnm_TranslateWrapMode(uint32_t wrap_enum) {
    static const uint32_t kTable[10] = { 0, 1, 0, 2, 2, 3, 4, 5, 6, 7 };
    return (wrap_enum <= 9u) ? kTable[wrap_enum] : 0u;
}

// VMA: 0x26b80a0 (22 B)
uint32_t CGTranslatorGnm_TranslateDepthCompareFuncA(uint32_t func_enum) {
    static const uint32_t kTable[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    return (func_enum <= 7u) ? kTable[func_enum] : 0u;
}

// VMA: 0x26b8080 (22 B)
uint32_t CGTranslatorGnm_TranslateDepthCompareFuncB(uint32_t func_enum) {
    return CGTranslatorGnm_TranslateDepthCompareFuncA(func_enum);
}

// VMA: 0x26b8070 (9 B)
uint32_t CGTranslatorGnm_ClampBoolEnumA(uint32_t val) {
    return (val < 2u) ? val : 0u;
}

// VMA: 0x26b8060 (15 B)
uint32_t CGTranslatorGnm_ExtractRenderTargetWriteMaskNibble(uint32_t rt_index, uint32_t packed_mask) {
    return (packed_mask >> (rt_index * 4u)) & 0x0Fu;
}

// VMA: 0x26b8040 (22 B)
uint32_t CGTranslatorGnm_TranslateBlendFunc(uint32_t blend_op) {
    static const uint32_t kTable[5] = { 0, 1, 3, 4, 2 };
    return (blend_op <= 4u) ? kTable[blend_op] : 0u;
}

// VMA: 0x26b8020 (22 B)
uint32_t CGTranslatorGnm_TranslateBlendMultiplier(uint32_t blend_factor) {
    static const uint32_t kTable[19] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0, 11, 12, 13, 14, 15, 16
    };
    return (blend_factor <= 0x12u) ? kTable[blend_factor] : 0u;
}

// VMA: 0x26b7f30 (232 B)
// Decomposes a vertex element format enum (`1..15`) into `(out_base_type, out_num_components)`.
bool CGTranslatorGnm_DecomposeVertexElementFormat(
    uint32_t* out_base_type,
    uint32_t* out_num_components,
    uint32_t vertex_fmt)
{
    switch (vertex_fmt) {
    case 1:  *out_base_type = 0; *out_num_components = 1; return true;
    case 2:  *out_base_type = 1; *out_num_components = 1; return true;
    case 3:  *out_base_type = 0; *out_num_components = 2; return true;
    case 4:  *out_base_type = 2; *out_num_components = 1; return true;
    case 6:  *out_base_type = 1; *out_num_components = 2; return true;
    case 7:  *out_base_type = 0; *out_num_components = 3; return true;
    case 8:  *out_base_type = 3; *out_num_components = 1; return true;
    case 12: *out_base_type = 2; *out_num_components = 2; return true;
    case 14: *out_base_type = 1; *out_num_components = 3; return true;
    case 15: *out_base_type = 0; *out_num_components = 4; return true;
    default: return false;
    }
}

// VMA: 0x26b7ec0 (107 B)
// Classifies a shader constant buffer reflection entry (`[entry+0x07]` prefix char, `[entry+0x0c]`
// relative name offset): `'.'` -> `1`, `"$Globals"` -> `2`, `"$Params"` -> `3`.
bool CGTranslatorGnm_ClassifyShaderConstantBufferName(
    uint32_t* out_cb_class,
    const uint8_t* reflection_entry)
{
    const uint8_t prefix = reflection_entry[0x07];
    if (prefix == '.') {
        *out_cb_class = 1;
        return true;
    }
    if (prefix == '$') {
        const uint32_t name_rel = *reinterpret_cast<const uint32_t*>(reflection_entry + 0x0c);
        const char* name_str = reinterpret_cast<const char*>(reflection_entry + 0x0c + name_rel);
        if (sub_2bbe568_strcmp("$Globals", name_str) == 0) {
            *out_cb_class = 2;
            return true;
        }
        if (sub_2bbe568_strcmp("$Params", name_str) == 0) {
            *out_cb_class = 3;
            return true;
        }
    }
    return false;
}

// VMA: 0x26b7e60 (82 B)
// Resolves a shader vertex input semantic name string against the 10-entry semantic table
// at `0x52ba0e8`.
bool CGTranslatorGnm_ResolveVertexSemanticName(
    uint32_t* out_semantic_id,
    const char* semantic_str)
{
    struct SemanticEntry {
        uint32_t    semantic_id;
        uint32_t    reserved;
        const char* name;
    };
    static const SemanticEntry kSemantics[10] = {
        { 0, 0, "POSITION" },
        { 1, 0, "BLENDWEIGHT" },
        { 2, 0, "BLENDINDICES" },
        { 3, 0, "NORMAL" },
        { 4, 0, "PSIZE" },
        { 5, 0, "TEXCOORD" },
        { 6, 0, "TANGENT" },
        { 7, 0, "BINORMAL" },
        { 8, 0, "TESSFACTOR" },
        { 9, 0, "COLOR" },
    };
    for (uint32_t i = 0; i < 10; ++i) {
        if (sub_2bbe568_strcmp(kSemantics[i].name, semantic_str) == 0) {
            *out_semantic_id = kSemantics[i].semantic_id;
            return true;
        }
    }
    return false;
}

// VMA: 0x26b7e40 (22 B)
uint32_t CGTranslatorGnm_TranslateIndexFormat(uint32_t idx_fmt) {
    static const uint32_t kTable[4] = { 1, 2, 4, 8 };
    return (idx_fmt <= 3u) ? kTable[idx_fmt] : 0u;
}

// VMA: 0x26b7e20 (22 B)
uint32_t CGTranslatorGnm_TranslateStencilOp(uint32_t stencil_op) {
    static const uint32_t kTable[5] = { 1, 2, 4, 8, 16 };
    return (stencil_op <= 4u) ? kTable[stencil_op] : 0u;
}

// VMA: 0x26b7e00 (18 B)
uint32_t CGTranslatorGnm_TranslateSampleMask(uint32_t mask) {
    return (mask == 0) ? 0u : 0x0Fu;
}

// VMA: 0x26b7d20 (65 B)
// Converts MSAA sample count (`1, 2, 4, 8, 16`) to `sce::Gnm::NumSamples` log2 (`0, 1, 2, 3, 4`).
uint32_t CGTranslatorGnm_Log2SampleCount(uint32_t sample_count) {
    if (sample_count < 2u) return 0u;
    if ((sample_count & ~1u) == 2u) return 1u;
    if ((sample_count & ~3u) == 4u) return 2u;
    if ((sample_count & ~7u) == 8u) return 3u;
    return (sample_count == 16u) ? 4u : 0u;
}

// VMA: 0x26b7d00 (22 B)
uint32_t CGTranslatorGnm_TranslateCullMode(uint32_t cull_mode) {
    static const uint32_t kTable[5] = { 0, 1, 2, 6, 3 };
    return (cull_mode <= 4u) ? kTable[cull_mode] : 0u;
}

// VMA: 0x26b7ce0 (30 B)
uint32_t CGTranslatorGnm_TranslateDepthFormatStencil(uint32_t fmt, uint32_t sub_mode) {
    if ((fmt - 9u) > 8u || sub_mode > 7u) return 0u;
    static const uint32_t kTable[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    return kTable[sub_mode];
}

// VMA: 0x26b7cc0 (25 B)
uint32_t CGTranslatorGnm_TranslateVertexFormatChannelX(uint32_t fmt) {
    static const uint32_t kTable[18] = { 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2 };
    return (fmt <= 0x11u) ? kTable[fmt] : 1u;
}

// VMA: 0x26b7ca0 (25 B)
uint32_t CGTranslatorGnm_TranslateVertexFormatChannelY(uint32_t fmt) {
    static const uint32_t kTable[18] = { 1, 2, 1, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2 };
    return (fmt <= 0x11u) ? kTable[fmt] : 1u;
}

// VMA: 0x26b7c80 (22 B)
uint32_t CGTranslatorGnm_TranslateVertexFormatChannelZ(uint32_t fmt) {
    static const uint32_t kTable[18] = { 0, 0, 1, 1, 0, 0, 1, 1, 3, 0, 0, 1, 1, 0, 0, 1, 1, 3 };
    return (fmt <= 0x11u) ? kTable[fmt] : 0u;
}

// VMA: 0x26b7c60 (22 B)
uint32_t CGTranslatorGnm_TranslateVertexFormatChannelW(uint32_t fmt) {
    static const uint32_t kTable[18] = { 0, 0, 0, 0, 1, 1, 1, 1, 3, 0, 0, 0, 0, 1, 1, 1, 1, 3 };
    return (fmt <= 0x11u) ? kTable[fmt] : 0u;
}

// VMA: 0x26b7c50 (9 B)
uint32_t CGTranslatorGnm_ClampPolygonMode(uint32_t mode) {
    return (mode < 3u) ? mode : 0u;
}

// VMA: 0x26b7c40 (12 B)
uint32_t CGTranslatorGnm_TranslateFrontFace(uint32_t front_ccw) {
    return (front_ccw == 0u) ? 2u : 1u;
}

// VMA: 0x26b7c20 (22 B)
uint32_t CGTranslatorGnm_TranslateStencilFunc(uint32_t func) {
    static const uint32_t kTable[8] = { 0, 1, 3, 5, 6, 7, 8, 9 };
    return (func <= 7u) ? kTable[func] : 0u;
}

// VMA: 0x26b7c00 (22 B)
uint32_t CGTranslatorGnm_TranslateDepthFunc(uint32_t func) {
    static const uint32_t kTable[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    return (func <= 7u) ? kTable[func] : 0u;
}

// VMA: 0x26b7bf0 (9 B)
uint32_t CGTranslatorGnm_ClampBoolEnumB(uint32_t val) {
    return (val < 2u) ? val : 0u;
}

// VMA: 0x26b7be0 (15 B)
uint32_t CGTranslatorGnm_PackRenderTargetWriteMaskNibble(uint32_t mask_nibble, uint32_t rt_index) {
    return (mask_nibble & 0x0Fu) << (rt_index * 4u);
}

// VMA: 0x26b7bc0 (22 B)
uint32_t CGTranslatorGnm_TranslateBlendOpB(uint32_t op) {
    static const uint32_t kTable[5] = { 0, 1, 4, 2, 3 };
    return (op <= 4u) ? kTable[op] : 0u;
}

// VMA: 0x26b7ba0 (22 B)
uint32_t CGTranslatorGnm_TranslateBlendFactorB(uint32_t factor) {
    static const uint32_t kTable[17] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18 };
    return (factor <= 0x10u) ? kTable[factor] : 0u;
}

// VMA: 0x26b7b80 (25 B)
uint32_t CGTranslatorGnm_TranslatePatchControlPoints(uint32_t topology) {
    return ((topology - 10u) <= 0x1Fu) ? (topology - 9u) : 0u;
}

// VMA: 0x26b7b60 (22 B)
// Translates engine primitive topology (`0..0x29`) to `sce::Gnm::PrimitiveType`.
uint32_t CGTranslatorGnm_TranslatePrimitiveTopology(uint32_t topology) {
    static const uint32_t kTable[42] = {
        0, 1, 2, 3, 4, 6, 10, 11, 12, 13,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9
    };
    return (topology <= 0x29u) ? kTable[topology] : 0u;
}

// VMA: 0x26b7b50 (9 B)
uint32_t CGTranslatorGnm_ClampBoolEnumC(uint32_t val) {
    return (val < 2u) ? val : 0u;
}

// VMA: 0x26b7ab0 (156 B)
// Translates `(num_samples, num_fragments, is_eqaa)` into `sce::Gnm::NumSamples` / `NumFragments`.
uint32_t CGTranslatorGnm_TranslateNumFragmentsAndSamples(
    uint32_t num_samples,
    uint32_t num_fragments,
    bool is_eqaa)
{
    if (!is_eqaa) {
        if (num_samples == 2u) return 1u;
        if (num_samples == 4u) return 2u;
        if (num_samples == 8u || num_samples == 16u) return 3u;
        return 0u;
    }
    if (num_samples == 2u)  return (num_fragments < 2u) ? num_fragments : 0u;
    if (num_samples == 4u)  return (num_fragments < 3u) ? num_fragments : 0u;
    if (num_samples == 8u)  return (num_fragments < 4u) ? num_fragments : 0u;
    if (num_samples == 16u) return (num_fragments < 4u) ? num_fragments : 0u;
    return 0u;
}

// VMA: 0x26b7a90 (22 B)
uint32_t CGTranslatorGnm_TranslateTileModeHint(uint32_t hint) {
    static const uint32_t kTable[4] = { 0, 1, 2, 3 };
    return (hint <= 3u) ? kTable[hint] : 0u;
}

// VMA: 0x26b7a20 (100 B)
uint32_t CGTranslatorGnm_TranslateSampleCountToGnmLog2(uint32_t samples, bool allow_16x) {
    if (samples == 2u) return 1u;
    if (samples == 4u) return 2u;
    if (samples == 8u) return 3u;
    if (samples == 16u && allow_16x) return 4u;
    return 0u;
}

// VMA: 0x26b7960 (184 B)
// Translates an engine depth/stencil format (`0x13..0x3B`) to its metadata `sce::Gnm::DataFormat`.
int32_t CGTranslatorGnm_TranslateMetadataDataFormat(uint32_t* out_gnm_fmt, uint32_t engine_fmt) {
    if (engine_fmt >= 0x13u && engine_fmt <= 0x16u) {
        *out_gnm_fmt = 0x4401u;
        return 0;
    }
    if ((engine_fmt >= 0x27u && engine_fmt <= 0x2Bu) ||
        (engine_fmt >= 0x35u && engine_fmt <= 0x3Bu)) {
        *out_gnm_fmt = 0u;
        return 0;
    }
    *out_gnm_fmt = 0u;
    return 12;
}

// VMA: 0x26b78a0 (184 B)
int32_t CGTranslatorGnm_TranslateDepthFormatToZFormat(uint32_t* out_z_fmt, uint32_t engine_fmt) {
    if (engine_fmt >= 0x13u && engine_fmt <= 0x16u) {
        *out_z_fmt = 1u;
        return 0;
    }
    if ((engine_fmt >= 0x27u && engine_fmt <= 0x2Bu) ||
        (engine_fmt >= 0x35u && engine_fmt <= 0x3Bu)) {
        *out_z_fmt = 0u;
        return 0;
    }
    *out_z_fmt = 0u;
    return 12;
}

// VMA: 0x26b77e0 (184 B)
int32_t CGTranslatorGnm_TranslateDepthFormatToSrvDataFormat(uint32_t* out_gnm_fmt, uint32_t engine_fmt) {
    if ((engine_fmt >= 0x13u && engine_fmt <= 0x16u) ||
        (engine_fmt >= 0x27u && engine_fmt <= 0x2Bu)) {
        *out_gnm_fmt = 0x204704u;
        return 0;
    }
    if (engine_fmt >= 0x35u && engine_fmt <= 0x3Bu) {
        *out_gnm_fmt = 0x204702u;
        return 0;
    }
    *out_gnm_fmt = 0u;
    return 12;
}

// VMA: 0x26b7720 (184 B)
int32_t CGTranslatorGnm_TranslateDepthFormatToStencilFormat(uint32_t* out_s_fmt, uint32_t engine_fmt) {
    if ((engine_fmt >= 0x13u && engine_fmt <= 0x16u) ||
        (engine_fmt >= 0x27u && engine_fmt <= 0x2Bu)) {
        *out_s_fmt = 3u;
        return 0;
    }
    if (engine_fmt >= 0x35u && engine_fmt <= 0x3Bu) {
        *out_s_fmt = 1u;
        return 0;
    }
    *out_s_fmt = 0u;
    return 12;
}

// VMA: 0x26b7310 (1032 B)
// Translates Bloodborne's 100-entry `CGFormat` enum (`0..0x63`) into a 32-bit `sce::Gnm::DataFormat`.
int32_t CGTranslatorGnm_TranslateEngineFormatToGnmDataFormat(
    uint32_t* out_gnm_fmt,
    uint32_t engine_fmt)
{
    if (engine_fmt > 0x63u) {
        *out_gnm_fmt = 0u;
        return 12;
    }
    static const uint32_t kGnmFormats[100] = {
        0x000000u, 0xfac40eu, 0xfac70eu, 0xfac40eu, 0xfac50eu, 0x1ac40du, 0x3ac70du, 0x1ac40du,
        0x1ac50du, 0xfac40cu, 0xfac70cu, 0xfac00cu, 0xfac40cu, 0xfac10cu, 0xfac50cu, 0x02c40bu,
        0x22c70bu, 0x02c40bu, 0x02c50bu, 0x004404u, 0x204704u, 0x204704u, 0x020401u, 0xfac409u,
        0xfac009u, 0xfac409u, 0x3ac706u, 0xfac40au, 0xfac00au, 0xfac90au, 0xfac40au, 0xfac10au,
        0xfac50au, 0x02c405u, 0x22c705u, 0x22c005u, 0x02c405u, 0x22c105u, 0x02c505u, 0x004404u,
        0x204704u, 0x204704u, 0x004404u, 0x004504u, 0x000000u, 0x000000u, 0x000000u, 0x000000u,
        0x02c403u, 0x22c003u, 0x02c403u, 0x22c103u, 0x02c503u, 0x004402u, 0x204702u, 0x204002u,
        0x204002u, 0x004402u, 0x204102u, 0x004502u, 0x004401u, 0x204001u, 0x004401u, 0x204101u,
        0x004501u, 0x800001u, 0x000000u, 0x000000u, 0x000000u, 0x000000u, 0xfac023u, 0xfac023u,
        0xfac923u, 0xfac024u, 0xfac024u, 0xfac924u, 0xfac025u, 0xfac025u, 0xfac925u, 0x204026u,
        0x204026u, 0x204126u, 0x22c027u, 0x22c027u, 0x22c127u, 0x32e010u, 0xf2e011u, 0xf2e00au,
        0x32e00au, 0x000000u, 0xf2e90au, 0xf2e90au, 0x32e90au, 0x32e90au, 0x3ac028u, 0x3ac028u,
        0x3ac128u, 0xfac029u, 0xfac029u, 0xfac929u
    };
    const uint32_t fmt = kGnmFormats[engine_fmt];
    *out_gnm_fmt = fmt;
    if (engine_fmt != 0u && fmt == 0u) {
        return 12;
    }
    return 0;
}

// ============================================================================
// 3. CGGpuBlitAndClearPassesGnm (0x26b2d30 .. 0x26b6cb0)
// ============================================================================

static inline uint32_t CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(
    CGSurfaceSyncTrackerState* self, uint32_t stage_kind, uint32_t subresource, const uint8_t* tex) {
    return GX_CALL(uint32_t, 0x26b94c0ULL, (void*, uint32_t, uint32_t, const void*), self, stage_kind, subresource, tex);
}
static inline uint32_t CGSurfaceSyncTrackerGnm_TransitionResourceForRead(
    CGSurfaceSyncTrackerState* self, uint32_t stage_kind, uint32_t subresource, const uint8_t* tex) {
    return GX_CALL(uint32_t, 0x26b91c0ULL, (void*, uint32_t, uint32_t, const void*), self, stage_kind, subresource, tex);
}
static inline void CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(
    CGSurfaceSyncTrackerState* self, uint32_t swz, uint32_t stage_kind, uint32_t subresource, const uint8_t* tex) {
    GX_CALL(void, 0x26b9320ULL, (void*, uint32_t, uint32_t, uint32_t, const void*), self, swz, stage_kind, subresource, tex);
}
static inline uint32_t CGSurfaceSyncTrackerGnm_TransitionUavViewForRead(
    CGSurfaceSyncTrackerState* self, uint32_t stage_kind, const uint8_t* uav_desc, const uint8_t* resource) {
    return GX_CALL(uint32_t, 0x26b9770ULL, (void*, uint32_t, const void*, const void*), self, stage_kind, uav_desc, resource);
}
static inline void CGSurfaceSyncTrackerGnm_TransitionUavViewPostOp(
    CGSurfaceSyncTrackerState* self, uint32_t swz, uint32_t stage_kind, const uint8_t* uav_desc, const uint8_t* resource) {
    GX_CALL(void, 0x26b9630ULL, (void*, uint32_t, uint32_t, const void*, const void*), self, swz, stage_kind, uav_desc, resource);
}
static inline uint32_t CGSurfaceSyncTrackerGnm_TransitionRenderTargetForWrite(
    CGSurfaceSyncTrackerState* self, const uint8_t* rt_view, const uint8_t* tex_resource, uint32_t flag) {
    return GX_CALL(uint32_t, 0x26b8ef0ULL, (void*, const void*, const void*, uint32_t), self, rt_view, tex_resource, flag);
}
static inline void CGSurfaceSyncTrackerGnm_TransitionRenderTargetPostOp(
    CGSurfaceSyncTrackerState* self, uint32_t swz, const uint8_t* rt_view, const uint8_t* tex_resource, uint32_t flag) {
    GX_CALL(void, 0x26b8da0ULL, (void*, uint32_t, const void*, const void*, uint32_t), self, swz, rt_view, tex_resource, flag);
}
static inline uint32_t CGSurfaceSyncTrackerGnm_TransitionDepthTargetForWrite(
    CGSurfaceSyncTrackerState* self, const uint8_t* dt_view, const uint8_t* depth_resource, uint32_t flag) {
    return GX_CALL(uint32_t, 0x26b8c40ULL, (void*, const void*, const void*, uint32_t), self, dt_view, depth_resource, flag);
}
static inline void CGSurfaceSyncTrackerGnm_TransitionDepthTargetPostOp(
    CGSurfaceSyncTrackerState* self, uint32_t swz, const uint8_t* dt_view, const uint8_t* depth_resource, uint32_t flag) {
    GX_CALL(void, 0x26b8bd0ULL, (void*, uint32_t, const void*, const void*, uint32_t), self, swz, dt_view, depth_resource, flag);
}
static inline uint32_t CGGpuBlitAndClearPassesGnm_GetNumSamples(uint32_t a, uint32_t b, uint32_t c) {
    return GX_CALL(uint32_t, 0x26b7ab0ULL, (uint32_t, uint32_t, uint32_t), a, b, c);
}
static inline int32_t CGFormatTranslatorGnm_TranslateToGnmDataFormat(uint32_t* out_fmt, uint32_t engine_fmt) {
    return GX_CALL(int32_t, 0x26b7310ULL, (uint32_t*, uint32_t), out_fmt, engine_fmt);
}
static inline uint32_t CGFormatTranslatorGnm_SelectOptimalTileMode(uint32_t a, uint32_t b, uint32_t c) {
    return GX_CALL(uint32_t, 0x26b7ab0ULL, (uint32_t, uint32_t, uint32_t), a, b, c);
}
static inline uint32_t CGPipelineStatePackerGnm_AlignTo256Blocks(uint64_t byte_addr) {
    return CGGpuAddressGnm_Pack256ByteAlignedB(byte_addr);
}
static inline uint32_t CGPipelineStatePackerGnm_BytesTo256BlocksCeil(uint64_t byte_size) {
    return CGGpuAddressGnm_Pack256ByteAlignedA(byte_size);
}
static inline int32_t CGPipelineStatePackerGnm_GetSubresourceByteOffsetAndSize(
    uint64_t* out_off, uint64_t* out_size, const uint8_t* resource, uint32_t subres, uint32_t a, uint32_t b) {
    return GX_CALL(int32_t, 0x26baf60ULL,
                   (uint64_t*, uint64_t*, const void*, uint32_t, uint32_t, uint32_t),
                   out_off, out_size, resource, subres, a, b);
}

// VMA: 0x26b6cb0 (1480 B)
// Note: Despite historical naming, 0x26b6cb0 is GXSwapChain's Present + 3-Channel 1D-Array Gamma-LUT
// blit pass (ps 0x546c778, t0 = frame color, t1 = 3x256 Gamma LUT, MRT0 = DisplayBuffer), which hooks
// into TemporalUpscaler::OnDisplayPassStart / RedirectColor / RedirectSampled (-> Module V3.3).
void CGGpuBlitAndClearPassesGnm_ResolveMsaaColorTarget(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* dst_resolved_tex,
    const uint8_t* src_msaa_tex,
    const uint8_t* src_fmask_tex)
{
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);

    CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(sync_tracker, 0u, src_msaa_tex);

    const uint32_t w = *reinterpret_cast<const uint16_t*>(dst_resolved_tex + 0x2c);
    const uint32_t h = *reinterpret_cast<const uint16_t*>(dst_resolved_tex + 0x2e);
    const uint32_t num_samples = CGGpuBlitAndClearPassesGnm_GetNumSamples(
        *reinterpret_cast<const uint16_t*>(dst_resolved_tex + 0x34),
        *reinterpret_cast<const uint16_t*>(dst_resolved_tex + 0x36),
        (*reinterpret_cast<const uint32_t*>(dst_resolved_tex + 0x24) >> 10) & 1u);
    if (gx_direct_vk::DirectRingEnabled()) {
        gx_direct_vk::ContextRegBatch vp_batch{};
        vp_batch.SetupScreenViewport(gnmx_ctx, 0u, 0u, w, h, 1.0f, 0.0f);
        vp_batch.SetAaScissorEnable(gnmx_ctx, num_samples != 0u ? 1u : 0u, 0u);
        vp_batch.Flush(gnmx_ctx);
    } else {
        GX_CALL(void, 0x1096f60ULL,
                (void*, uint32_t, uint32_t, uint32_t, uint32_t, float, float),
                gnmx_ctx, 0u, 0u, w, h, 1.0f, 0.0f);
        GX_CALL(void, 0x1077150ULL, (void*, uint32_t, uint32_t), gnmx_ctx, num_samples != 0u ? 1u : 0u, 0u);
    }

    uint64_t smp[2] = {};
    GX_CALL(void, 0x1086dd0ULL, (void*), smp);
    GX_CALL(void, 0x1086ef0ULL, (void*, uint32_t, uint32_t, uint32_t), smp, 2u, 0u, 0u);
    GX_CALL(void, 0x1086f40ULL, (void*, uint32_t, uint32_t), smp, 1u, 1u);
    GX_CALL(void, 0x1086e10ULL, (void*, uint32_t), smp, 0u);
    GX_CALL(void, 0x1086df0ULL, (void*, uint32_t), smp, 0u);
    GX_CALL(void, 0x1086f60ULL, (void*, uint32_t), smp, 0u);
    GX_CALL(void, 0x1086fc0ULL, (void*, uint32_t), smp, 0u);
    GX_CALL(void, 0x1086e30ULL, (void*, uint32_t, uint32_t), smp, 0u, 0u);
    GX_CALL(void, 0x1087020ULL, (void*, uint32_t, uint32_t), smp, 0u, 0u);
    gx_direct_vk::RegisterDirectNativeSampler(smp);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9930) = smp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9938) = smp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1978) |= 1ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1980) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x1dc8);

    uint64_t tsharp0[4] = {};
    if (GX_CALL(int32_t, 0x216d900ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                tsharp0, src_msaa_tex, src_msaa_tex[0x33], 0u, 0u, 1u, 1u, 0u) != 0) {
        return;
    }
    gx_direct_vk::RegisterDirectNativeSrv(
        tsharp0, *reinterpret_cast<const uint32_t*>(src_msaa_tex + 0x10), 0u);
    uint64_t tsharp1[4] = {};
    if (GX_CALL(int32_t, 0x216d900ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                tsharp1, src_fmask_tex, src_fmask_tex[0x33], 0u, 0u, 1u, 3u, 0u) != 0) {
        return;
    }
    gx_direct_vk::RegisterDirectNativeSrv(
        tsharp1, *reinterpret_cast<const uint32_t*>(src_fmask_tex + 0x10), 0u);
    auto* t0 = reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x6930);
    t0[0] = tsharp0[0]; t0[1] = tsharp0[1]; t0[2] = tsharp0[2]; t0[3] = tsharp0[3];
    auto* t1 = reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x6950);
    t1[0] = tsharp1[0]; t1[1] = tsharp1[1]; t1[2] = tsharp1[2]; t1[3] = tsharp1[3];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1980) |=
        *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x19c8) |
        *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x19d0);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1968) |= 3ULL;

    const uint32_t slice = GX_CALL(uint32_t, 0x2164d50ULL, (uint32_t, uint32_t), 0u, dst_resolved_tex[0x32]);
    const uint32_t mip   = GX_CALL(uint32_t, 0x2164d60ULL, (uint32_t, uint32_t), 0u, dst_resolved_tex[0x32]);
    uint64_t rt_view[12] = {};
    if (GX_CALL(int32_t, 0x216e1d0ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t),
                rt_view, dst_resolved_tex, dst_resolved_tex[0x33], mip, slice, 1u) != 0) {
        return;
    }

    void* binder = gnmx_ctx_wrapper + 0x50;
    void* om_cache = gnmx_ctx_wrapper + 0xb558;
    GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, 0u, rt_view);
    for (uint32_t i = 1u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, i, nullptr);
    }
    GX_CALL(void, 0x26d1e20ULL, (void*, void*, const void*), om_cache, gnmx_ctx, nullptr);

    uint32_t blend_ctl = 0u, ds_ctl = 0u, prim_ctl = 0u;
    GX_CALL(void, 0x1084ea0ULL, (uint32_t*), &blend_ctl);
    GX_CALL(void, 0x1085380ULL, (uint32_t*), &ds_ctl);
    GX_CALL(void, 0x10854b0ULL, (uint32_t*), &prim_ctl);
    const uint64_t ffff_50 = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870350ULL));
    const uint64_t ffff_58 = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870358ULL));
    if (gx_direct_vk::DirectRingEnabled()) {
        gx_direct_vk::ContextRegBatch om_batch{};
        om_batch.SetBlendControl(gnmx_ctx, 0u, blend_ctl);
        om_batch.SetRenderTargetMask(gnmx_ctx, 0xFu);
        om_batch.SetDepthControl(gnmx_ctx, ds_ctl);
        om_batch.SetStencil(gnmx_ctx, prim_ctl);
        om_batch.Flush(gnmx_ctx);
    } else {
        GX_CALL(void, 0x1073090ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 0u, blend_ctl);
        GX_CALL(void, 0x1072fd0ULL, (void*, uint32_t), gnmx_ctx, 0xFu);
        GX_CALL(void, 0x10733a0ULL, (void*, uint32_t), gnmx_ctx, ds_ctl);
        GX_CALL(void, 0x1073400ULL, (void*, uint32_t), gnmx_ctx, prim_ctl);
    }
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb568) = blend_ctl;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb560) = 0xFULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6e8) = ffff_50;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a0) = ds_ctl;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a4) = prim_ctl;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;

    const void* vs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c530ULL));
    const void* ps_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c778ULL));
    GX_CALL(void, 0x26ce1e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, ps_hdr);
    if (gx_direct_vk::DirectRingEnabled()) {
        const gx_direct_vk::SetIndexStateArgs idx_args{0ULL, 0x11u, gx_direct_vk::kIndexFlag_SetPrim};
        gx_direct_vk::EmitSetIndexState(gnmx_ctx, idx_args);
    } else {
        GX_CALL(void, 0x1073fe0ULL, (void*, uint32_t), gnmx_ctx, 0x11u);
    }
    GX_CALL(void, 0x26ce5e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, ps_hdr);
    if (gx_direct_vk::DirectRingEnabled()) {
        gx_direct_vk::EmitDrawAuto(gnmx_ctx, 3u, 1u, 0);
    } else {
        GX_CALL(void, 0x1075370ULL, (void*, uint32_t), gnmx_ctx, 1u);
        GX_CALL(void, 0x10741b0ULL, (void*, uint32_t), gnmx_ctx, 3u);
    }
    GX_CALL(void, 0x26cfc20ULL, (void*, void*), binder, gnmx_ctx);

    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb548) = ~0ULL;
    gnmx_ctx_wrapper[0xb550] |= 1u;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb540) = ~0u;
    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e30ULL, (void*, void*, uint32_t), om_cache, gnmx_ctx, i);
    }
    GX_CALL(void, 0x26d1e70ULL, (void*, void*), om_cache, gnmx_ctx);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb520) = 0x11u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb518) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb524) = ~0u;
    gnmx_ctx_wrapper[0x1968] |= 3u;
    gnmx_ctx_wrapper[0x1978] |= 1u;

    CGSurfaceSyncTrackerGnm_AccumGfxRange(
        sync_tracker,
        *reinterpret_cast<const uint64_t*>(dst_resolved_tex + 0x08),
        *reinterpret_cast<const uint32_t*>(dst_resolved_tex + 0x14),
        0u);
    reinterpret_cast<uint8_t*>(sync_tracker)[0] |= 0x0Fu;
}

// VMA: 0x26b6810 (1174 B)
void CGGpuBlitAndClearPassesGnm_ClearRenderTargetSlices(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* depth_target,
    const uint8_t* tex_resource,
    uint32_t first_slice,
    uint32_t slice_count,
    bool force_draw_clear)
{
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
    if (!GX_CALL(bool, 0x1072730ULL, (const void*), depth_target)) {
        CGSurfaceSyncTrackerGnm_AccumCsRange(
            sync_tracker,
            *reinterpret_cast<const uint64_t*>(tex_resource + 0x08),
            *reinterpret_cast<const uint32_t*>(tex_resource + 0x14));
        reinterpret_cast<uint8_t*>(sync_tracker)[0] |= 0x0Fu;
        return;
    }
    if (!force_draw_clear && (tex_resource[0x25] & 0x20u) != 0u) {
        return;
    }

    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_DecompressDepth) &&
        depth_target && tex_resource) {
        if (slice_count == 0u) {
            return;
        }
        gx_direct_vk::DecompressDepthArgs dargs{};
        dargs.dst_va = *reinterpret_cast<const uint64_t*>(tex_resource + 0x08);
        dargs.dst_byte_size = *reinterpret_cast<const uint32_t*>(tex_resource + 0x14);
        dargs.db_view = *reinterpret_cast<const uint32_t*>(depth_target + 0x20);
        std::memcpy(dargs.db_regs, depth_target, sizeof(dargs.db_regs));
        dargs.htile_data_base = *reinterpret_cast<const uint32_t*>(depth_target + 0x24);
        dargs.htile_surface = *reinterpret_cast<const uint32_t*>(depth_target + 0x2c);
        dargs.cb_db_extent = *reinterpret_cast<const uint32_t*>(depth_target + 0x30);
        dargs.first_slice = first_slice;
        dargs.slice_count = slice_count;
        dargs.reserved = *reinterpret_cast<const uint32_t*>(gnmx_ctx_wrapper + 0xb650);
        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
        if (gx_direct_vk::EmitDecompressDepth(gnmx_ctx, dargs)) {
            gnmx_ctx_wrapper[0xb498] |= 0x07u;
            gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
            return;
        }
    }

    void* binder = gnmx_ctx_wrapper + 0x50;
    void* om_cache = gnmx_ctx_wrapper + 0xb558;

    GX_CALL(void, 0x26d2c30ULL, (void*, void*), om_cache, gnmx_ctx_wrapper);
    GX_CALL(void, 0x1075da0ULL, (void*, uint32_t), gnmx_ctx, 0x2Cu);

    const uint32_t wh = *reinterpret_cast<const uint32_t*>(depth_target + 0x30);
    GX_CALL(void, 0x1096f60ULL,
            (void*, uint32_t, uint32_t, uint32_t, uint32_t, float, float),
            gnmx_ctx, 0u, 0u, wh & 0xFFFFu, wh >> 16, 1.0f, 0.0f);

    const uint32_t num_samples = GX_CALL(uint32_t, 0x10726c0ULL, (const void*), depth_target);
    GX_CALL(void, 0x1077150ULL, (void*, uint32_t, uint32_t), gnmx_ctx, num_samples != 0u ? 1u : 0u, 0u);

    uint32_t ds_ctl = 0u, prim_ctl = 0u;
    GX_CALL(void, 0x1085380ULL, (uint32_t*), &ds_ctl);
    GX_CALL(void, 0x10854b0ULL, (uint32_t*), &prim_ctl);
    GX_CALL(void, 0x10733a0ULL, (void*, uint32_t), gnmx_ctx, ds_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a0) = ds_ctl;
    const uint64_t ffff_58 = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870358ULL));
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;
    GX_CALL(void, 0x1073400ULL, (void*, uint32_t), gnmx_ctx, prim_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a4) = prim_ctl;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;

    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, i, nullptr);
    }
    GX_CALL(void, 0x1073320ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 0u, 0xCCu);

    uint32_t db_rc = 0u;
    GX_CALL(void, 0x1085020ULL, (uint32_t*), &db_rc);
    GX_CALL(void, 0x1085080ULL, (uint32_t*, uint32_t), &db_rc, 1u);
    GX_CALL(void, 0x1085070ULL, (uint32_t*, uint32_t), &db_rc, 1u);
    GX_CALL(void, 0x10769e0ULL, (void*, uint32_t), gnmx_ctx, db_rc);

    const void* vs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c530ULL));
    GX_CALL(void, 0x26ce1e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, nullptr);
    GX_CALL(void, 0x1073fe0ULL, (void*, uint32_t), gnmx_ctx, 0x11u);
    GX_CALL(void, 0x1075370ULL, (void*, uint32_t), gnmx_ctx, 1u);
    GX_CALL(void, 0x26ce5e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, nullptr);

    struct alignas(8) DepthTargetCopy {
        uint64_t q[6];
        uint32_t d;
    } dt_copy;
    for (int i = 0; i < 6; ++i) {
        dt_copy.q[i] = *reinterpret_cast<const uint64_t*>(depth_target + i * 8);
    }
    dt_copy.d = *reinterpret_cast<const uint32_t*>(depth_target + 0x30);

    for (uint32_t i = 0u; i < slice_count; ++i) {
        const uint32_t s = first_slice + i;
        GX_CALL(void, 0x10721d0ULL, (void*, uint32_t, uint32_t), &dt_copy, s, s);
        GX_CALL(void, 0x26d1e20ULL, (void*, void*, const void*), om_cache, gnmx_ctx, &dt_copy);
        GX_CALL(void, 0x10741b0ULL, (void*, uint32_t), gnmx_ctx, 3u);
    }

    GX_CALL(void, 0x26cfc20ULL, (void*, void*), binder, gnmx_ctx);
    gnmx_ctx_wrapper[0xb498] |= 0x07u;
    gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb548) = ~0ULL;
    gnmx_ctx_wrapper[0xb550] |= 1u;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb540) = ~0u;

    GX_CALL(void, 0x1073320ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 1u, 0xCCu);
    uint32_t db_rc_reset = 0u;
    GX_CALL(void, 0x1085020ULL, (uint32_t*), &db_rc_reset);
    GX_CALL(void, 0x10769e0ULL, (void*, uint32_t), gnmx_ctx, db_rc_reset);
    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e30ULL, (void*, void*, uint32_t), om_cache, gnmx_ctx, i);
    }
    GX_CALL(void, 0x26d1e70ULL, (void*, void*), om_cache, gnmx_ctx);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb520) = 0x11u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb518) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb524) = ~0u;
}

void CGGpuBlitAndClearPassesGnm_ClearBufferOrSurfaceViaCompute(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* resource,
    uint32_t byte_offset,
    uint64_t byte_size,
    const uint64_t clear_value[2]);

// VMA: 0x26b5f50 (2230 B)
void CGGpuBlitAndClearPassesGnm_ClearDepthStencilTarget(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* depth_target,
    const uint8_t* depth_resource,
    uint8_t stencil_clear_value,
    uint8_t clear_flags,
    float depth_clear_value)
{
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    void* binder = gnmx_ctx_wrapper + 0x50;
    void* om_cache = gnmx_ctx_wrapper + 0xb558;
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);

    bool clear_z = (clear_flags & 1u) != 0u;
    bool clear_s = ((clear_flags & 2u) != 0u) && (GX_CALL(uint32_t, 0x10727a0ULL, (const void*), depth_target) != 0u);

    if (GX_CALL(bool, 0x1072730ULL, (const void*), depth_target)) {
        const bool htile_stencil = GX_CALL(bool, 0x1072750ULL, (const void*), depth_target);
        const float htile_scale = *reinterpret_cast<const float*>(gnmx_ctx_wrapper + 0xb650) *
                                  (*reinterpret_cast<const float*>(GX_VMA(0x486fa44ULL)));
        const int64_t q = static_cast<int64_t>(__builtin_floorf(htile_scale));
        const uint32_t q32 = static_cast<uint32_t>(q);
        const uint32_t htile_word = htile_stencil
            ? (((q32 << 4) & 0x3FFF0u) | (q32 << 18))
            : (q32 << 18);

        GX_CALL(void, 0x1075da0ULL, (void*, uint32_t), gnmx_ctx, 0x2Cu);
        const uint32_t htile_vec[4] = {htile_word, htile_word, htile_word, htile_word};

        uint64_t htile_addr = static_cast<uint64_t>(GX_CALL(uint32_t, 0x1072830ULL, (const void*), depth_target)) << 8;
        const uint32_t htile_size = GX_CALL(uint32_t, 0x1072630ULL, (const void*), depth_target);
        const uint32_t base_slice = GX_CALL(uint32_t, 0x1072800ULL, (const void*), depth_target);
        if (base_slice != 0u) {
            const uint32_t max_slice = GX_CALL(uint32_t, 0x1072810ULL, (const void*), depth_target);
            const uint32_t slice_cnt = (1u - base_slice) + max_slice;
            htile_addr += static_cast<uint64_t>((htile_size / slice_cnt) * base_slice);
        }
        const uint64_t htile_offset = htile_addr - *reinterpret_cast<const uint64_t*>(depth_resource + 0x08);

        if ((depth_target[0x3c] & 0x08u) != 0u) {
            const uint32_t num_dwords = static_cast<uint32_t>((static_cast<uint64_t>(htile_size) + 3ULL) >> 2);
            const uint64_t target_gpu_addr = *reinterpret_cast<const uint64_t*>(depth_resource + 0x08) + static_cast<uint32_t>(htile_offset);
            bool handled_direct = false;
            if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_ClearHtileUnaligned) &&
                ((target_gpu_addr & 3ULL) == 0ULL)) {
                if (num_dwords == 0u) {
                    handled_direct = true;
                } else {
                    const uint32_t swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0u, depth_resource);
                    GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
                    if (gx_direct_vk::EmitFillBuffer(gnmx_ctx, target_gpu_addr, static_cast<uint64_t>(num_dwords) << 2, htile_word)) {
                        CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swz, 0u, depth_resource);
                        GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
                        handled_direct = true;
                    }
                }
            }
            if (!handled_direct) {
                uint32_t* cb_mem = static_cast<uint32_t*>(
                    GX_CALL(void*, 0x26ae7f0ULL, (void*, void*, uint32_t),
                            gnmx_ctx_wrapper + 0xb3c0, gnmx_ctx_wrapper, 16u));
                if (cb_mem != nullptr) {
                    cb_mem[0] = num_dwords;
                    cb_mem[1] = htile_word;

                    const uint32_t swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0u, depth_resource);
                    const void* cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c0b0ULL));
                    GX_CALL(void, 0x26cd400ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);

                    uint64_t uav_vsharp[2] = {};
                    GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
                            uav_vsharp, reinterpret_cast<const void*>(target_gpu_addr), 0x4404u, num_dwords);
                    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t),
                            uav_vsharp, static_cast<int32_t>(swz) >> 16, (swz >> 8) & 0xFFu, swz & 0xFFu);
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9330) = uav_vsharp[0];
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9338) = uav_vsharp[1];
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000000000ULL;
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2528);

                    uint64_t cb_vsharp[2] = {};
                    GX_CALL(void, 0x1070960ULL, (void*, const void*, uint32_t), cb_vsharp, cb_mem, 16u);
                    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), cb_vsharp, 0x6E, 0u, 0u);
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa170) = cb_vsharp[0];
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa178) = cb_vsharp[1];
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x10000ULL;
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2488);

                    GX_CALL(void, 0x26cd5d0ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);
                    GX_CALL(void, 0x1074c50ULL, (void*, uint32_t, uint32_t, uint32_t), gnmx_ctx, (num_dwords + 63u) >> 6, 1u, 1u);
                    GX_CALL(void, 0x26ce110ULL, (void*, void*), binder, gnmx_ctx);
                    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swz, 0u, depth_resource);
                    GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
                    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000010000ULL;
                }
            }
        } else {
            CGGpuBlitAndClearPassesGnm_ClearBufferOrSurfaceViaCompute(
                gnmx_ctx_wrapper,
                depth_resource,
                static_cast<uint32_t>(htile_offset),
                htile_size,
                reinterpret_cast<const uint64_t*>(htile_vec));
        }
        clear_s = clear_s && htile_stencil;
        clear_z = false;
    }

    if (!clear_z && !clear_s) {
        return;
    }
    if ((*reinterpret_cast<const uint32_t*>(depth_target + 0x38) - 1u) > 5u) {
        return;
    }

    const uint32_t wh = *reinterpret_cast<const uint32_t*>(depth_target + 0x30);
    const uint32_t first_slice = *reinterpret_cast<const uint32_t*>(depth_target + 0x44);
    const uint32_t slice_cnt   = *reinterpret_cast<const uint32_t*>(depth_target + 0x48);

    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_ClearDepthDraw) &&
        depth_target && depth_resource) {
        if (slice_cnt == 0u) {
            return;
        }
        gx_direct_vk::ClearDepthStencilArgs cargs{};
        cargs.dst_va = *reinterpret_cast<const uint64_t*>(depth_resource + 0x08);
        cargs.dst_byte_size = *reinterpret_cast<const uint32_t*>(depth_resource + 0x14);
        cargs.db_view = *reinterpret_cast<const uint32_t*>(depth_target + 0x20);
        std::memcpy(cargs.db_regs, depth_target, sizeof(cargs.db_regs));
        cargs.htile_data_base = *reinterpret_cast<const uint32_t*>(depth_target + 0x24);
        cargs.cb_db_extent = wh;
        cargs.first_slice = first_slice;
        cargs.slice_count = slice_cnt;
        cargs.depth_clear = depth_clear_value;
        cargs.stencil_and_flags = static_cast<uint32_t>(stencil_clear_value) |
                                  (clear_z ? 0x100u : 0u) |
                                  (clear_s ? 0x200u : 0u);
        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
        if (gx_direct_vk::EmitClearDepthStencil(gnmx_ctx, cargs)) {
            const uint32_t s_base256 = GX_CALL(uint32_t, 0x1072770ULL, (const void*), depth_target);
            if (s_base256 != 0u) {
                const uint32_t fmt_raw  = GX_CALL(uint32_t, 0x10726b0ULL, (const void*), depth_target);
                uint32_t dfmt           = GX_CALL(uint32_t, 0x10715b0ULL, (uint32_t), fmt_raw);
                const uint32_t bpp      = GX_CALL(uint32_t, 0x1071750ULL, (const uint32_t*), &dfmt);
                const uint32_t pitch_m1 = GX_CALL(uint32_t, 0x10727d0ULL, (const void*), depth_target);
                const uint32_t log2_s   = GX_CALL(uint32_t, 0x10726c0ULL, (const void*), depth_target);
                const uint32_t s_bytes  = (((pitch_m1 << 6) + 0x40u) * bpp) << (log2_s & 0xFFu);
                CGSurfaceSyncTrackerGnm_AccumCsRange(sync_tracker, static_cast<uint64_t>(s_base256) << 8, s_bytes);
            } else {
                sync_tracker->cache_sync_mode |= 0x07u;
            }
            gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
            return;
        }
    }
    GX_CALL(void, 0x1096f60ULL,
            (void*, uint32_t, uint32_t, uint32_t, uint32_t, float, float),
            gnmx_ctx, 0u, 0u, wh & 0xFFFFu, wh >> 16,
            *reinterpret_cast<const float*>(GX_VMA(0x486fa48ULL)), depth_clear_value);

    const uint32_t num_samples = GX_CALL(uint32_t, 0x10726c0ULL, (const void*), depth_target);
    GX_CALL(void, 0x1077150ULL, (void*, uint32_t, uint32_t), gnmx_ctx, num_samples != 0u ? 1u : 0u, 0u);
    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, i, nullptr);
    }

    uint32_t ds_ctl = 0u;
    GX_CALL(void, 0x1085380ULL, (uint32_t*), &ds_ctl);
    if (clear_z) {
        GX_CALL(void, 0x10853f0ULL, (uint32_t*, uint32_t), &ds_ctl, 1u);
        GX_CALL(void, 0x1085390ULL, (uint32_t*, uint32_t, uint32_t), &ds_ctl, 1u, 7u);
    }
    const uint64_t ffff_58 = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870358ULL));
    if (clear_s) {
        GX_CALL(void, 0x1085400ULL, (uint32_t*, uint32_t), &ds_ctl, 1u);
        GX_CALL(void, 0x10853b0ULL, (uint32_t*, uint32_t), &ds_ctl, 7u);
        uint32_t prim_ctl = 0u;
        GX_CALL(void, 0x10854b0ULL, (uint32_t*), &prim_ctl);
        GX_CALL(void, 0x10854c0ULL, (uint32_t*, uint32_t, uint32_t, uint32_t), &prim_ctl, 4u, 4u, 4u);
        GX_CALL(void, 0x1073400ULL, (void*, uint32_t), gnmx_ctx, prim_ctl);
        *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a4) = prim_ctl;
        *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;
        const uint32_t st_val = (static_cast<uint32_t>(stencil_clear_value) << 24) |
                                static_cast<uint32_t>(stencil_clear_value) | 0xFFFF00u;
        GX_CALL(void, 0x1073190ULL, (void*, uint32_t), gnmx_ctx, st_val);
        *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a8) = st_val;
        *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;
    } else {
        GX_CALL(void, 0x1085400ULL, (uint32_t*, uint32_t), &ds_ctl, 0u);
        GX_CALL(void, 0x10853b0ULL, (uint32_t*, uint32_t), &ds_ctl, 0u);
    }
    GX_CALL(void, 0x10733a0ULL, (void*, uint32_t), gnmx_ctx, ds_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a0) = ds_ctl;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;

    const void* vs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c530ULL));
    GX_CALL(void, 0x26ce1e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, nullptr);
    GX_CALL(void, 0x1073fe0ULL, (void*, uint32_t), gnmx_ctx, 0x11u);
    GX_CALL(void, 0x1075370ULL, (void*, uint32_t), gnmx_ctx, 1u);
    GX_CALL(void, 0x26ce5e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, nullptr);

    struct alignas(8) DepthTargetCopy {
        uint64_t q[6];
        uint32_t d;
    } dt_copy;
    for (int i = 0; i < 6; ++i) {
        dt_copy.q[i] = *reinterpret_cast<const uint64_t*>(depth_target + i * 8);
    }
    dt_copy.d = *reinterpret_cast<const uint32_t*>(depth_target + 0x30);

    for (uint32_t i = 0u; i < slice_cnt; ++i) {
        const uint32_t s = first_slice + i;
        GX_CALL(void, 0x10721d0ULL, (void*, uint32_t, uint32_t), &dt_copy, s, s);
        GX_CALL(void, 0x26d1e20ULL, (void*, void*, const void*), om_cache, gnmx_ctx, &dt_copy);
        GX_CALL(void, 0x10741b0ULL, (void*, uint32_t), gnmx_ctx, 3u);
    }
    GX_CALL(void, 0x26cfc20ULL, (void*, void*), binder, gnmx_ctx);

    const uint32_t s_base256 = GX_CALL(uint32_t, 0x1072770ULL, (const void*), depth_target);
    const uint32_t fmt_raw   = GX_CALL(uint32_t, 0x10726b0ULL, (const void*), depth_target);
    uint32_t dfmt            = GX_CALL(uint32_t, 0x10715b0ULL, (uint32_t), fmt_raw);
    const uint32_t bpp       = GX_CALL(uint32_t, 0x1071750ULL, (const uint32_t*), &dfmt);
    const uint32_t pitch_m1  = GX_CALL(uint32_t, 0x10727d0ULL, (const void*), depth_target);
    const uint32_t log2_s    = GX_CALL(uint32_t, 0x10726c0ULL, (const void*), depth_target);

    if (s_base256 != 0u) {
        const uint32_t s_bytes = (((pitch_m1 << 6) + 0x40u) * bpp) << (log2_s & 0xFFu);
        CGSurfaceSyncTrackerGnm_AccumCsRange(sync_tracker, static_cast<uint64_t>(s_base256) << 8, s_bytes);
    } else {
        sync_tracker->cache_sync_mode |= 0x07u;
    }
    gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb548) = ~0ULL;
    gnmx_ctx_wrapper[0xb550] |= 1u;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb540) = ~0u;
    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e30ULL, (void*, void*, uint32_t), om_cache, gnmx_ctx, i);
    }
    GX_CALL(void, 0x26d1e70ULL, (void*, void*), om_cache, gnmx_ctx);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb520) = 0x11u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb518) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb524) = ~0u;
}

// VMA: 0x26b5a70 (1236 B)
void CGGpuBlitAndClearPassesGnm_CopyRenderTargetFullscreen(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* rt_target,
    const uint8_t* tex_resource,
    uint32_t first_slice,
    uint32_t slice_count,
    bool force_draw_clear)
{
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
    const bool has_cmask = GX_CALL(bool, 0x1086d60ULL, (const void*), rt_target);
    const bool has_fmask = GX_CALL(bool, 0x1086d50ULL, (const void*), rt_target);
    if (has_fmask || !has_cmask) {
        const uint64_t gpu_addr = *reinterpret_cast<const uint64_t*>(tex_resource + 0x08);
        if (gpu_addr != 0ULL) {
            sync_tracker->cache_sync_mode |= 0x02u;
            const uint32_t s256 = CGGpuAddressGnm_Pack256ByteAlignedB(gpu_addr);
            const uint32_t e256 = s256 + CGGpuAddressGnm_Pack256ByteAlignedA(*reinterpret_cast<const uint32_t*>(tex_resource + 0x14));
            if (s256 < sync_tracker->gfx_min_addr_256) sync_tracker->gfx_min_addr_256 = s256;
            if (e256 > sync_tracker->gfx_max_addr_256) sync_tracker->gfx_max_addr_256 = e256;
            gnmx_ctx_wrapper[0xb494] = 0xFFu;
        } else {
            sync_tracker->cache_sync_mode |= 0x07u;
        }
        gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
        return;
    }
    if (!force_draw_clear && (tex_resource[0x25] & 0x20u) != 0u) {
        return;
    }

    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_EliminateFastClear) &&
        rt_target && tex_resource) {
        if (slice_count == 0u) {
            return;
        }
        gx_direct_vk::EliminateFastClearArgs eargs{};
        eargs.dst_va = *reinterpret_cast<const uint64_t*>(tex_resource + 0x08);
        eargs.dst_byte_size = *reinterpret_cast<const uint32_t*>(tex_resource + 0x14);
        eargs.first_slice = first_slice;
        eargs.slice_count = slice_count;
        std::memcpy(eargs.cb_regs, rt_target, sizeof(eargs.cb_regs));
        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
        if (gx_direct_vk::EmitEliminateFastClear(gnmx_ctx, eargs)) {
            gnmx_ctx_wrapper[0xb498] |= 0x07u;
            gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
            return;
        }
    }

    void* binder = gnmx_ctx_wrapper + 0x50;
    void* om_cache = gnmx_ctx_wrapper + 0xb558;

    GX_CALL(void, 0x1075da0ULL, (void*, uint32_t), gnmx_ctx, 0x2Eu);
    const uint32_t w = GX_CALL(uint32_t, 0x1086d70ULL, (const void*), rt_target);
    const uint32_t h = GX_CALL(uint32_t, 0x10868d0ULL, (const void*), rt_target);
    GX_CALL(void, 0x1096f60ULL,
            (void*, uint32_t, uint32_t, uint32_t, uint32_t, float, float),
            gnmx_ctx, 0u, 0u, w, h, 1.0f, 0.0f);

    const uint32_t num_samples = GX_CALL(uint32_t, 0x10868f0ULL, (const void*), rt_target);
    GX_CALL(void, 0x1077150ULL, (void*, uint32_t, uint32_t), gnmx_ctx, num_samples != 0u ? 1u : 0u, 0u);

    uint32_t blend_ctl = 0u, prim_ctl = 0u, ds_ctl = 0u;
    GX_CALL(void, 0x1084ea0ULL, (uint32_t*), &blend_ctl);
    GX_CALL(void, 0x10854b0ULL, (uint32_t*), &prim_ctl);
    GX_CALL(void, 0x1085380ULL, (uint32_t*), &ds_ctl);
    GX_CALL(void, 0x1085390ULL, (uint32_t*, uint32_t, uint32_t), &ds_ctl, 0u, 7u);
    GX_CALL(void, 0x10853f0ULL, (uint32_t*, uint32_t), &ds_ctl, 1u);

    GX_CALL(void, 0x1073090ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 0u, blend_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb568) = blend_ctl;
    const uint64_t ffff_50 = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870350ULL));
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6e8) = ffff_50;
    GX_CALL(void, 0x1072fd0ULL, (void*, uint32_t), gnmx_ctx, 0xFu);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb560) = 0xFULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6e8) = ffff_50;
    GX_CALL(void, 0x10733a0ULL, (void*, uint32_t), gnmx_ctx, ds_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a0) = ds_ctl;
    const uint64_t ffff_58 = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870358ULL));
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;
    GX_CALL(void, 0x1073400ULL, (void*, uint32_t), gnmx_ctx, prim_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a4) = prim_ctl;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = ffff_58;

    GX_CALL(void, 0x26d1e20ULL, (void*, void*, const void*), om_cache, gnmx_ctx, nullptr);
    GX_CALL(void, 0x1073320ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 2u, 0xCCu);

    const void* vs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c530ULL));
    GX_CALL(void, 0x26ce1e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, nullptr);
    for (uint32_t i = 1u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, i, nullptr);
    }
    GX_CALL(void, 0x1073fe0ULL, (void*, uint32_t), gnmx_ctx, 0x11u);
    GX_CALL(void, 0x1075370ULL, (void*, uint32_t), gnmx_ctx, 1u);
    GX_CALL(void, 0x26ce5e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, nullptr);

    struct alignas(8) RenderTargetCopy {
        uint64_t q[5];
        uint32_t d;
    } rt_copy;
    for (int i = 0; i < 5; ++i) {
        rt_copy.q[i] = *reinterpret_cast<const uint64_t*>(rt_target + i * 8);
    }
    rt_copy.d = *reinterpret_cast<const uint32_t*>(rt_target + 0x28);

    for (uint32_t i = 0u; i < slice_count; ++i) {
        const uint32_t s = first_slice + i;
        GX_CALL(void, 0x1086520ULL, (void*, uint32_t, uint32_t), &rt_copy, s, s);
        GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, 0u, &rt_copy);
        GX_CALL(void, 0x10741b0ULL, (void*, uint32_t), gnmx_ctx, 3u);
    }

    GX_CALL(void, 0x26cfc20ULL, (void*, void*), binder, gnmx_ctx);
    gnmx_ctx_wrapper[0xb498] |= 0x07u;
    gnmx_ctx_wrapper[0xb478] |= 0x0Fu;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb548) = ~0ULL;
    gnmx_ctx_wrapper[0xb550] |= 1u;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb540) = ~0u;
    GX_CALL(void, 0x1073320ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 1u, 0xCCu);
    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e30ULL, (void*, void*, uint32_t), om_cache, gnmx_ctx, i);
    }
    GX_CALL(void, 0x26d1e70ULL, (void*, void*), om_cache, gnmx_ctx);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb520) = 0x11u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb518) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb524) = ~0u;
}

// VMA: 0x26b55a0 (1228 B)
void CGGpuBlitAndClearPassesGnm_CopyDualRenderTargetsFullscreen(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* rt_tex_b,
    uint32_t subresource_b,
    const uint8_t* rt_tex_a,
    uint32_t subresource_a)
{
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    GX_CALL(void, 0x1075da0ULL, (void*, uint32_t), gnmx_ctx, 0x2Eu);

    const uint32_t slice_a = GX_CALL(uint32_t, 0x2164d50ULL, (uint32_t, uint32_t), subresource_a, rt_tex_a[0x32]);
    const uint32_t mip_a = GX_CALL(uint32_t, 0x2164d60ULL, (uint32_t, uint32_t), subresource_a, rt_tex_a[0x32]);
    uint8_t rt_view_a[48] = {};
    if (GX_CALL(int32_t, 0x216e1d0ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t),
                rt_view_a, rt_tex_a, rt_tex_a[0x33], mip_a, slice_a, 1u) != 0) {
        return;
    }

    const uint32_t slice_b = GX_CALL(uint32_t, 0x2164d50ULL, (uint32_t, uint32_t), subresource_b, rt_tex_b[0x32]);
    const uint32_t mip_b = GX_CALL(uint32_t, 0x2164d60ULL, (uint32_t, uint32_t), subresource_b, rt_tex_b[0x32]);
    uint8_t rt_view_b[48] = {};
    if (GX_CALL(int32_t, 0x216e1d0ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t),
                rt_view_b, rt_tex_b, rt_tex_b[0x33], mip_b, slice_b, 1u) != 0) {
        return;
    }

    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_ResolveColor) &&
        rt_tex_a && rt_tex_b) {
        auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
        gx_direct_vk::ResolveColorArgs rargs{};
        rargs.dst_va = *reinterpret_cast<const uint64_t*>(rt_tex_b + 0x08);
        rargs.src_va = *reinterpret_cast<const uint64_t*>(rt_tex_a + 0x08);
        rargs.dst_byte_size = *reinterpret_cast<const uint32_t*>(rt_tex_b + 0x14);
        rargs.src_byte_size = *reinterpret_cast<const uint32_t*>(rt_tex_a + 0x14);
        std::memcpy(rargs.src_cb_regs, rt_view_a, sizeof(rargs.src_cb_regs));
        std::memcpy(rargs.dst_cb_regs, rt_view_b, sizeof(rargs.dst_cb_regs));
        if (gx_direct_vk::EmitResolveColor(gnmx_ctx, rargs)) {
            const uint64_t base_a = rargs.src_va;
            if (base_a != 0ULL) {
                *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 2u;
                const uint32_t b256 = CGPipelineStatePackerGnm_AlignTo256Blocks(base_a);
                const uint32_t e256 = b256 + CGPipelineStatePackerGnm_BytesTo256BlocksCeil(rargs.src_byte_size);
                uint32_t& min_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb484);
                uint32_t& max_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb488);
                if (b256 < min_b) min_b = b256;
                if (e256 > max_b) max_b = e256;
            } else {
                *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 7u;
            }
            *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb478) |= 0xFu;

            const uint64_t base_b = rargs.dst_va;
            if (base_b != 0ULL) {
                *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 2u;
                const uint32_t b256 = CGPipelineStatePackerGnm_AlignTo256Blocks(base_b);
                const uint32_t e256 = b256 + CGPipelineStatePackerGnm_BytesTo256BlocksCeil(rargs.dst_byte_size);
                uint32_t& min_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb484);
                uint32_t& max_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb488);
                if (b256 < min_b) min_b = b256;
                if (e256 > max_b) max_b = e256;
                gnmx_ctx_wrapper[0xb494] |= 1u;
            } else {
                *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 7u;
            }
            *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb478) |= 0xFu;
            return;
        }
    }

    void* binder = gnmx_ctx_wrapper + 0x50;
    void* om_cache = gnmx_ctx_wrapper + 0xb558;
    GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, 0u, rt_view_a);
    GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, 1u, rt_view_b);
    for (uint32_t i = 2u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e10ULL, (void*, void*, uint32_t, const void*), om_cache, gnmx_ctx, i, nullptr);
    }
    GX_CALL(void, 0x26d1e20ULL, (void*, void*, const void*), om_cache, gnmx_ctx, nullptr);

    uint32_t blend_ctl = 0u, ds_ctl = 0u, prim_ctl = 0u;
    GX_CALL(void, 0x1084ea0ULL, (uint32_t*), &blend_ctl);
    GX_CALL(void, 0x1085380ULL, (uint32_t*), &ds_ctl);
    GX_CALL(void, 0x10854b0ULL, (uint32_t*), &prim_ctl);

    GX_CALL(void, 0x1073090ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 0u, blend_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb568) = blend_ctl;
    const uint64_t def_blend = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870350ULL));
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6e8) = def_blend;
    GX_CALL(void, 0x1072fd0ULL, (void*, uint32_t), gnmx_ctx, 0xFu);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb560) = 0xFULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6e8) = def_blend;

    GX_CALL(void, 0x10733a0ULL, (void*, uint32_t), gnmx_ctx, ds_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a0) = ds_ctl;
    const uint64_t def_prim = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4870358ULL));
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = def_prim;
    GX_CALL(void, 0x1073400ULL, (void*, uint32_t), gnmx_ctx, prim_ctl);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb5a4) = prim_ctl;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb6f0) = def_prim;

    GX_CALL(void, 0x1073320ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 3u, 0xCCu);
    const void* vs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c530ULL));
    const void* ps_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c658ULL));
    GX_CALL(void, 0x26ce1e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, ps_hdr);
    GX_CALL(void, 0x1073fe0ULL, (void*, uint32_t), gnmx_ctx, 0x11u);
    GX_CALL(void, 0x26ce5e0ULL, (void*, void*, const void*, const void*), binder, gnmx_ctx, vs_hdr, ps_hdr);
    GX_CALL(void, 0x1075370ULL, (void*, uint32_t), gnmx_ctx, 1u);
    GX_CALL(void, 0x10741b0ULL, (void*, uint32_t), gnmx_ctx, 3u);
    GX_CALL(void, 0x26cfc20ULL, (void*, void*), binder, gnmx_ctx);

    GX_CALL(void, 0x1073320ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 1u, 0xCCu);
    for (uint32_t i = 0u; i < 8u; ++i) {
        GX_CALL(void, 0x26d1e30ULL, (void*, void*, uint32_t), om_cache, gnmx_ctx, i);
    }
    GX_CALL(void, 0x26d1e70ULL, (void*, void*), om_cache, gnmx_ctx);
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb520) = 0x11u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xb518) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb524) = ~0u;
    GX_CALL(void, 0x1075da0ULL, (void*, uint32_t), gnmx_ctx, 0x2Eu);

    const uint64_t base_a = *reinterpret_cast<const uint64_t*>(rt_tex_a + 0x08);
    if (base_a != 0ULL) {
        *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 2u;
        const uint32_t b256 = CGPipelineStatePackerGnm_AlignTo256Blocks(base_a);
        const uint32_t e256 = b256 + CGPipelineStatePackerGnm_BytesTo256BlocksCeil(*reinterpret_cast<const uint32_t*>(rt_tex_a + 0x14));
        uint32_t& min_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb484);
        uint32_t& max_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb488);
        if (b256 < min_b) min_b = b256;
        if (e256 > max_b) max_b = e256;
    } else {
        *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 7u;
    }
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb478) |= 0xFu;

    const uint64_t base_b = *reinterpret_cast<const uint64_t*>(rt_tex_b + 0x08);
    if (base_b != 0ULL) {
        *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 2u;
        const uint32_t b256 = CGPipelineStatePackerGnm_AlignTo256Blocks(base_b);
        const uint32_t e256 = b256 + CGPipelineStatePackerGnm_BytesTo256BlocksCeil(*reinterpret_cast<const uint32_t*>(rt_tex_b + 0x14));
        uint32_t& min_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb484);
        uint32_t& max_b = *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb488);
        if (b256 < min_b) min_b = b256;
        if (e256 > max_b) max_b = e256;
        gnmx_ctx_wrapper[0xb494] |= 1u;
    } else {
        *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb498) |= 7u;
    }
    *reinterpret_cast<uint32_t*>(gnmx_ctx_wrapper + 0xb478) |= 0xFu;
}

// VMA: 0x26b5380 (540 B)
void CGGpuBlitAndClearPassesGnm_ClearBufferOrSurfaceViaCompute(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* resource,
    uint32_t byte_offset,
    uint64_t byte_size,
    const uint64_t clear_value[2])
{
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_ClearBufferSurf) && gnmx_ctx_wrapper && resource && clear_value) {
        const auto* cv32 = reinterpret_cast<const uint32_t*>(clear_value);
        const uint64_t base_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08) + byte_offset;
        const uint64_t fill_bytes = ((byte_size + 15ULL) >> 4) << 4;
        if (cv32[0] == cv32[1] && cv32[0] == cv32[2] && cv32[0] == cv32[3] &&
            ((base_addr | fill_bytes) & 3ULL) == 0ULL) {
            if (fill_bytes == 0ULL) {
                return;
            }
            void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
            auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
            const uint32_t swizzle = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, resource);
            GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
            if (gx_direct_vk::EmitFillBuffer(gnmx_ctx, base_addr, fill_bytes, cv32[0])) {
                CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swizzle, 0, resource);
                GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
                return;
            }
        }
    }
    uint64_t* cb_mem = static_cast<uint64_t*>(
        GX_CALL(void*, 0x26ae7f0ULL, (void*, void*, uint32_t),
                gnmx_ctx_wrapper + 0xb3c0, gnmx_ctx_wrapper, 16u));
    if (cb_mem == nullptr) {
        return;
    }
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    void* binder = gnmx_ctx_wrapper + 0x50;
    cb_mem[0] = clear_value[0];
    cb_mem[1] = clear_value[1];

    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
    const uint32_t swizzle = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, resource);

    const uint64_t num_vec4 = (byte_size + 15ULL) >> 4;
    const void* cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546bf90ULL));
    GX_CALL(void, 0x26cd400ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);

    const uint64_t base_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08) + byte_offset;
    uint64_t uav_vsharp[2] = {};
    GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
            uav_vsharp, reinterpret_cast<const void*>(base_addr), 0xFAC40Eu, static_cast<uint32_t>(num_vec4));
    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t),
            uav_vsharp, static_cast<int32_t>(swizzle) >> 16, (swizzle >> 8) & 0xFFu, swizzle & 0xFFu);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9330) = uav_vsharp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9338) = uav_vsharp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000000000ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2528);

    uint64_t cb_vsharp[2] = {};
    GX_CALL(void, 0x1070960ULL, (void*, const void*, uint32_t), cb_vsharp, cb_mem, 16u);
    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), cb_vsharp, 0x6E, 0u, 0u);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa170) = cb_vsharp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa178) = cb_vsharp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x10000ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2488);

    GX_CALL(void, 0x26cd5d0ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);
    GX_CALL(void, 0x1074c50ULL, (void*, uint32_t, uint32_t, uint32_t),
            gnmx_ctx, static_cast<uint32_t>((num_vec4 + 63ULL) >> 6), 1u, 1u);
    GX_CALL(void, 0x26ce110ULL, (void*, void*), binder, gnmx_ctx);
    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swizzle, 0, resource);
    GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000010000ULL;
}

// VMA: 0x26b4ef0 (1168 B)
void CGGpuBlitAndClearPassesGnm_ClearUnorderedAccessViewUint(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* uav_desc,
    const uint8_t* resource,
    const uint64_t clear_rgba[2])
{
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    if (GX_CALL(bool, 0x1086d60ULL, (const void*), uav_desc)) {
        GX_CALL(void, 0x1075da0ULL, (void*, uint32_t), gnmx_ctx, 0x31u);
        uint64_t cmask_off = static_cast<uint64_t>(GX_CALL(uint32_t, 0x1086d80ULL, (const void*), uav_desc)) << 8;
        const uint32_t cmask_slice_stride = (GX_CALL(uint32_t, 0x1086d90ULL, (const void*), uav_desc) << 7) + 128u;
        const uint32_t base_slice = GX_CALL(uint32_t, 0x1086cd0ULL, (const void*), uav_desc);
        const uint32_t last_slice = GX_CALL(uint32_t, 0x1086ce0ULL, (const void*), uav_desc);
        const uint32_t slice_cnt = (1u - base_slice) + last_slice;
        if (base_slice != 0u) {
            cmask_off += static_cast<uint64_t>(base_slice * cmask_slice_stride);
        }
        const uint32_t byte_offset = static_cast<uint32_t>(cmask_off) - *reinterpret_cast<const uint32_t*>(resource + 0x08);
        const uint32_t byte_size = slice_cnt * cmask_slice_stride;
        CGGpuBlitAndClearPassesGnm_ClearBufferOrSurfaceViaCompute(
            gnmx_ctx_wrapper, resource, byte_offset, byte_size,
            reinterpret_cast<const uint64_t*>(GX_VMA(0x486fa60ULL)));
        return;
    }

    const uint32_t uav_type = *reinterpret_cast<const uint32_t*>(uav_desc + 0x30) - 1u;
    if (uav_type > 7u) {
        return;
    }
    uint32_t mip_level = 0u, first_slice = 0u, slice_count = 1u, mip_count = 1u;
    if (uav_type != 0u) {
        mip_level = *reinterpret_cast<const uint32_t*>(uav_desc + 0x34);
        first_slice = *reinterpret_cast<const uint32_t*>(uav_desc + 0x38);
        mip_count = resource[0x32];
        slice_count = *reinterpret_cast<const uint32_t*>(uav_desc + 0x3c);
    }

    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_ClearColorImage) &&
        gnmx_ctx_wrapper && uav_desc && resource && clear_rgba) {
        const uint32_t view_fmt = *reinterpret_cast<const uint32_t*>(uav_desc + 0x2c);
        uint32_t check_fmt = 0u;
        if (CGFormatTranslatorGnm_TranslateToGnmDataFormat(&check_fmt, view_fmt) != 0) {
            return;
        }
        if (uav_type == 0u) {
            if ((clear_rgba[0] | clear_rgba[1]) == 0ULL) {
                uint64_t sub_off = 0ULL, sub_size = 0ULL;
                if (CGPipelineStatePackerGnm_GetSubresourceByteOffsetAndSize(
                        &sub_off, &sub_size, resource, 0u, 0u, 0u) == 0) {
                    const uint64_t base_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08) + sub_off;
                    if (((base_addr | sub_size) & 3ULL) == 0ULL) {
                        if (sub_size == 0ULL) {
                            return;
                        }
                        auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
                        const uint32_t swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, resource);
                        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
                        if (gx_direct_vk::EmitFillBuffer(gnmx_ctx, base_addr, sub_size, 0u)) {
                            CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swz, 0, resource);
                            GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
                            return;
                        }
                    }
                }
            }
        } else if ((resource[0x23] & 0x40u) == 0u && slice_count > 0u && slice_count <= 64u) {
            const uint64_t bpe = GX_CALL(uint64_t, 0x2164ed0ULL, (uint32_t), view_fmt);
            if (bpe != 0ULL && bpe <= 16ULL) {
                gx_direct_vk::ClearColorArgs slice_args[64]{};
                bool all_valid = true;
                for (uint32_t i = 0u; i < slice_count; ++i) {
                    const uint32_t slice = first_slice + i;
                    const uint32_t subres = GX_CALL(uint32_t, 0x2164d40ULL,
                                                    (uint32_t, uint32_t, uint32_t),
                                                    mip_level, slice, mip_count);
                    uint64_t sub_off = 0ULL, sub_size = 0ULL;
                    if (CGPipelineStatePackerGnm_GetSubresourceByteOffsetAndSize(
                            &sub_off, &sub_size, resource, subres, 0u, 0u) != 0 ||
                        sub_size == 0ULL || sub_size > 0xFFFFFFFFULL) {
                        all_valid = false;
                        break;
                    }
                    uint64_t tsharp[4] = {};
                    int32_t rc = GX_CALL(int32_t, 0x216d900ULL,
                                         (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                                         tsharp, resource, view_fmt, mip_level, slice, 1u, 1u, 0u);
                    if (rc != 0 && view_fmt != resource[0x33]) {
                        rc = GX_CALL(int32_t, 0x216d900ULL,
                                     (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                                     tsharp, resource, resource[0x33], mip_level, slice, 1u, 1u, 0u);
                    }
                    const uint32_t dfmt = static_cast<uint32_t>((tsharp[0] >> 52) & 0x3FULL);
                    if (rc != 0 || !((dfmt >= 1u && dfmt <= 19u) || dfmt == 34u)) {
                        all_valid = false;
                        break;
                    }
                    auto& arg = slice_args[i];
                    arg.dst_va = *reinterpret_cast<const uint64_t*>(resource + 0x08) + sub_off;
                    arg.byte_size = static_cast<uint32_t>(sub_size);
                    arg.num_bits = static_cast<uint32_t>(bpe * 8ULL);
                    std::memcpy(arg.clear_rgba, clear_rgba, 16u);
                    std::memcpy(arg.tsharp, tsharp, sizeof(tsharp));
                }
                if (all_valid) {
                    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
                    const uint32_t swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, resource);
                    GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
                    bool emitted = true;
                    for (uint32_t i = 0u; i < slice_count; ++i) {
                        if (!gx_direct_vk::EmitClearColorImage(gnmx_ctx, slice_args[i])) {
                            emitted = false;
                            break;
                        }
                    }
                    if (emitted) {
                        CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swz, 0, resource);
                        GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
                        return;
                    }
                }
            }
        }
    }

    uint64_t* cb_mem = static_cast<uint64_t*>(
        GX_CALL(void*, 0x26ae7f0ULL, (void*, void*, uint32_t),
                gnmx_ctx_wrapper + 0xb3c0, gnmx_ctx_wrapper, 16u));
    if (cb_mem == nullptr) {
        return;
    }
    cb_mem[0] = clear_rgba[0];
    cb_mem[1] = clear_rgba[1];

    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
    const uint32_t swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, resource);

    uint32_t data_fmt = 0u;
    if (CGFormatTranslatorGnm_TranslateToGnmDataFormat(
            &data_fmt, *reinterpret_cast<const uint32_t*>(uav_desc + 0x2c)) != 0) {
        return;
    }
    if ((data_fmt & 0xF00u) == 0x900u) {
        data_fmt &= 0xFFFFF0FFu;
    }

    uint64_t cb_vsharp[2] = {};
    GX_CALL(void, 0x1070960ULL, (void*, const void*, uint32_t), cb_vsharp, cb_mem, 16u);
    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), cb_vsharp, 0x6E, 0u, 0u);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa170) = cb_vsharp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa178) = cb_vsharp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x10000ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2488);

    GX_CALL(void, 0x1072e20ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 0u, 0x3Fu);
    GX_CALL(void, 0x1072e20ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 1u, 0x3Fu);
    const uint64_t bpe = GX_CALL(uint64_t, 0x2164ed0ULL, (uint32_t), *reinterpret_cast<const uint32_t*>(uav_desc + 0x2c));

    void* binder = gnmx_ctx_wrapper + 0x50;
    const void* cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546be70ULL));
    for (uint32_t i = 0u; i < slice_count; ++i) {
        const uint32_t slice = first_slice + i;
        GX_CALL(void, 0x26cd400ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);
        const uint32_t subres = GX_CALL(uint32_t, 0x2164d40ULL, (uint32_t, uint32_t, uint32_t), mip_level, slice, mip_count);
        uint64_t sub_off = 0ULL, sub_size = 0ULL;
        if (bpe != 0ULL && CGPipelineStatePackerGnm_GetSubresourceByteOffsetAndSize(&sub_off, &sub_size, resource, subres, 0u, 0u) == 0) {
            const uint64_t num_elems = (sub_size + bpe - 1ULL) / bpe;
            const uint64_t base_addr = *reinterpret_cast<const uint64_t*>(resource + 0x08) + sub_off;
            uint64_t uav_vsharp[2] = {};
            GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
                    uav_vsharp, reinterpret_cast<const void*>(base_addr), data_fmt, static_cast<uint32_t>(num_elems));
            GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t),
                    uav_vsharp, static_cast<int32_t>(swz) >> 16, (swz >> 8) & 0xFFu, swz & 0xFFu);
            *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9330) = uav_vsharp[0];
            *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9338) = uav_vsharp[1];
            *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000000000ULL;
            *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2528);
            GX_CALL(void, 0x26cd5d0ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);
            GX_CALL(void, 0x1074c50ULL, (void*, uint32_t, uint32_t, uint32_t),
                    gnmx_ctx, static_cast<uint32_t>(((num_elems >> 1) + 63ULL) >> 6), 1u, 1u);
            GX_CALL(void, 0x26ce110ULL, (void*, void*), binder, gnmx_ctx);
        }
    }

    GX_CALL(void, 0x1072e20ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 0u, 0x1FFu);
    GX_CALL(void, 0x1072e20ULL, (void*, uint32_t, uint32_t), gnmx_ctx, 1u, 0x1FFu);
    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swz, 0, resource);
    GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000010000ULL;
}

struct CGTextureUpdatePrep {
    uint64_t tsharp[4];
    const void* cs_hdr;
    uint64_t total_bytes;
    uint32_t bpe;
    uint32_t data_fmt;
    uint32_t mip_level;
    uint32_t array_slice;
    uint32_t bc_mips;
    uint32_t box_w;
    uint32_t box_h;
    uint32_t adj_x0;
    uint32_t adj_y0;
    uint32_t z0;
};

static inline bool CGGpuBlitAndClearPassesGnm_PrepareTextureUpdate(
    CGTextureUpdatePrep& out,
    const uint8_t* dst_tex,
    uint32_t subresource_idx,
    const uint32_t* dst_box)
{
    const uint32_t dim = dst_tex[0x20];
    const uint32_t flags_shr2 = *reinterpret_cast<const uint32_t*>(dst_tex + 0x24) >> 2;
    const uint32_t cg_fmt = dst_tex[0x33];
    uint32_t tex_w = *reinterpret_cast<const uint16_t*>(dst_tex + 0x2c);
    uint32_t tex_h = *reinterpret_cast<const uint16_t*>(dst_tex + 0x2e);
    uint32_t tex_d = 1u;
    uint32_t num_slices = 1u;
    if (dim == 4u) {
        tex_d = *reinterpret_cast<const uint16_t*>(dst_tex + 0x30);
    } else {
        num_slices = *reinterpret_cast<const uint16_t*>(dst_tex + 0x30);
        if ((flags_shr2 & 1u) != 0u) {
            num_slices = (num_slices * 0xAAABu) >> 18;
        }
    }

    uint32_t mip_cnt = dst_tex[0x32];
    if (mip_cnt == 0u) {
        mip_cnt = GX_CALL(uint32_t, 0x2165130ULL, (uint32_t, uint32_t, uint32_t), tex_w, tex_h, tex_d);
    }
    const uint32_t mip_level = GX_CALL(uint32_t, 0x2164d60ULL, (uint32_t, uint32_t), subresource_idx, mip_cnt);
    const uint32_t array_slice = GX_CALL(uint32_t, 0x2164d50ULL, (uint32_t, uint32_t), subresource_idx, mip_cnt);

    uint32_t x0 = 0u, y0 = 0u, z0 = 0u, x1 = 0u, y1 = 0u, z1 = 0u;
    if (dst_box != nullptr) {
        x0 = dst_box[0]; y0 = dst_box[1]; z0 = dst_box[2];
        x1 = dst_box[3]; y1 = dst_box[4]; z1 = dst_box[5];
    } else {
        x1 = tex_w >> mip_level; if (!x1) x1 = 1u;
        y1 = tex_h >> mip_level; if (!y1) y1 = 1u;
        z1 = tex_d >> mip_level; if (!z1) z1 = 1u;
    }
    uint32_t box_w = x1 - x0;
    uint32_t box_h = y1 - y0;

    const uint32_t bc_idx = cg_fmt - 0x46u;
    uint32_t adj_x0 = x0, adj_y0 = y0;
    uint32_t adj_fmt = cg_fmt, adj_w = tex_w, adj_h = tex_h;
    if (bc_idx <= 0x1Du) {
        if (((0x3F0071F8u >> bc_idx) & 1u) != 0u) {
            box_w = (box_w + 3u) >> 2;
            box_h = (box_h + 3u) >> 2;
            adj_x0 = (x0 + 3u) >> 2;
            adj_y0 = (y0 + 3u) >> 2;
            adj_w = (tex_w + 3u) >> 2;
            adj_h = (tex_h + 3u) >> 2;
            adj_fmt = 3u;
        } else if (((0xE07u >> bc_idx) & 1u) != 0u) {
            box_w = (box_w + 3u) >> 2;
            box_h = (box_h + 3u) >> 2;
            adj_x0 = (x0 + 3u) >> 2;
            adj_y0 = (y0 + 3u) >> 2;
            adj_w = (tex_w + 3u) >> 2;
            adj_h = (tex_h + 3u) >> 2;
            adj_fmt = 0xCu;
        }
    }

    uint32_t bc_mips = GX_CALL(uint32_t, 0x2165130ULL, (uint32_t, uint32_t, uint32_t), adj_w, adj_h, tex_d);
    if (mip_cnt < bc_mips) bc_mips = mip_cnt;
    if (bc_mips <= mip_level) {
        adj_fmt = cg_fmt;
        adj_x0 = x0;
        adj_y0 = y0;
    } else {
        tex_w = adj_w;
        tex_h = adj_h;
        mip_cnt = bc_mips;
    }

    uint32_t data_fmt = 0u;
    if (CGFormatTranslatorGnm_TranslateToGnmDataFormat(&data_fmt, adj_fmt) != 0) {
        return false;
    }
    const uint32_t bpe = static_cast<uint32_t>(GX_CALL(uint64_t, 0x2164ed0ULL, (uint32_t), adj_fmt));

    out.tsharp[0] = 0ULL; out.tsharp[1] = 0ULL; out.tsharp[2] = 0ULL; out.tsharp[3] = 0ULL;
    uint32_t total_elems = 0u;
    if (dim == 3u) {
        total_elems = box_w * box_h;
        const uint32_t tile_mode = CGFormatTranslatorGnm_SelectOptimalTileMode(
            *reinterpret_cast<const uint16_t*>(dst_tex + 0x34),
            *reinterpret_cast<const uint16_t*>(dst_tex + 0x36),
            (*reinterpret_cast<const uint32_t*>(dst_tex + 0x24) >> 10) & 1u);
        GX_CALL(int64_t, 0x1087ee0ULL,
                (void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                out.tsharp, tex_w, tex_h, num_slices, mip_cnt, data_fmt,
                *reinterpret_cast<const uint32_t*>(dst_tex + 0x28), tile_mode, flags_shr2 & 1u);
        out.cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c2f0ULL));
    } else if (dim == 4u) {
        total_elems = (z1 - z0) * box_w * box_h;
        GX_CALL(int64_t, 0x1088590ULL,
                (void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                out.tsharp, tex_w, tex_h, tex_d, mip_cnt, data_fmt,
                *reinterpret_cast<const uint32_t*>(dst_tex + 0x28));
        out.cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c410ULL));
    } else if (dim == 2u) {
        GX_CALL(int64_t, 0x1087ee0ULL,
                (void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                out.tsharp, tex_w, 1u, num_slices, mip_cnt, data_fmt,
                *reinterpret_cast<const uint32_t*>(dst_tex + 0x28), 0u, 0u);
        GX_CALL(int32_t, 0x1088370ULL, (void*, uint32_t), out.tsharp, 0xCu);
        out.cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546c1d0ULL));
        total_elems = box_w;
    } else {
        return false;
    }

    out.total_bytes = static_cast<uint64_t>(total_elems * bpe);
    out.bpe = bpe;
    out.data_fmt = data_fmt;
    out.mip_level = mip_level;
    out.array_slice = array_slice;
    out.bc_mips = bc_mips;
    out.box_w = box_w;
    out.box_h = box_h;
    out.adj_x0 = adj_x0;
    out.adj_y0 = adj_y0;
    out.z0 = z0;
    return true;
}

static inline void CGGpuBlitAndClearPassesGnm_CopyStagedChunk(
    uint8_t* stage_ptr,
    uint64_t chunk_bytes,
    uint32_t& cur_x,
    uint64_t& cur_y,
    uint64_t& cur_z,
    uint32_t bpe,
    uint64_t row_bytes,
    uint64_t slice_bytes,
    const uint8_t* src_base,
    uint64_t src_row_pitch,
    uint64_t src_slice_pitch)
{
    uint64_t x_byte_off = static_cast<uint32_t>(cur_x * bpe);
    uint64_t y_dst_off = cur_y * row_bytes;
    uint64_t z_dst_off = cur_z * slice_bytes;
    uint64_t y_src_off = cur_y * src_row_pitch;
    uint64_t z_src_off = cur_z * src_slice_pitch;
    const uint64_t start_linear = y_dst_off + z_dst_off + x_byte_off;
    uint64_t bytes_left = chunk_bytes;
    while (bytes_left != 0ULL) {
        const uint8_t* s_ptr = src_base + z_src_off + y_src_off + x_byte_off;
        const uint64_t row_rem = row_bytes - x_byte_off;
        const uint64_t copy_len = (static_cast<uint32_t>(bytes_left) < static_cast<uint32_t>(row_rem)) ? bytes_left : row_rem;
        uint8_t* d_ptr = stage_ptr + (x_byte_off - start_linear) + y_dst_off + z_dst_off;
        sub_2bbe578_memcpy(d_ptr, s_ptr, static_cast<uint32_t>(copy_len));
        bytes_left -= static_cast<uint32_t>(copy_len);
        x_byte_off += static_cast<uint32_t>(copy_len);
        if (x_byte_off < row_bytes) continue;
        y_dst_off += row_bytes;
        y_src_off += src_row_pitch;
        x_byte_off = 0ULL;
        if (y_dst_off < slice_bytes) continue;
        z_dst_off += slice_bytes;
        z_src_off += src_slice_pitch;
        y_dst_off = 0ULL;
        y_src_off = 0ULL;
    }
    cur_x = static_cast<uint32_t>(x_byte_off / bpe);
    cur_y = static_cast<uint32_t>(y_dst_off / row_bytes);
    cur_z = static_cast<uint32_t>(z_dst_off / slice_bytes);
}

static bool TryDirectUploadTextureSubresource(
    uint8_t* ctx,
    bool is_compute_ctx,
    const CGTextureUpdatePrep& p,
    const uint8_t* dst_tex,
    const void* src_data_desc,
    const uint32_t* dst_box)
{
    if (!gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_UploadTextureCpu) ||
        !ctx || !dst_tex || !src_data_desc) {
        return false;
    }
    const uint8_t dim = dst_tex[0x20];
    if ((dim != 2u && dim != 3u && dim != 4u) ||
        (dst_tex[0x23] & 0x40u) != 0u ||
        *reinterpret_cast<const uint16_t*>(dst_tex + 0x34) > 1u) {
        return false;
    }
    if (p.total_bytes == 0ULL || p.box_w == 0u || p.box_h == 0u || p.bpe == 0u) {
        return true;
    }

    const uint32_t tex_w = *reinterpret_cast<const uint16_t*>(dst_tex + 0x2c);
    const uint32_t tex_h = (dim == 2u) ? 1u : *reinterpret_cast<const uint16_t*>(dst_tex + 0x2e);
    const uint32_t tex_d = (dim == 4u) ? *reinterpret_cast<const uint16_t*>(dst_tex + 0x30) : 1u;
    const uint32_t total_slices = (dim == 4u) ? 1u : *reinterpret_cast<const uint16_t*>(dst_tex + 0x30);
    uint32_t total_mips = dst_tex[0x32];
    if (total_mips == 0u) {
        total_mips = GX_CALL(uint32_t, 0x2165130ULL, (uint32_t, uint32_t, uint32_t), tex_w, tex_h, tex_d);
    }
    if (p.mip_level >= total_mips || (dim != 4u && p.array_slice >= total_slices)) {
        return false;
    }

    uint64_t real_tsharp[4] = {};
    if (GX_CALL(int32_t, 0x216d900ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                real_tsharp, dst_tex, dst_tex[0x33], 0u, 0u, total_mips, total_slices, 0u) != 0) {
        return false;
    }

    const uint32_t dfmt = static_cast<uint32_t>((real_tsharp[0] >> 52) & 0x3FULL);
    const bool is_bc = (dfmt >= 35u && dfmt <= 41u);
    if (!is_bc && !((dfmt >= 1u && dfmt <= 19u) || dfmt == 34u)) {
        return false;
    }
    const uint32_t blk_w = is_bc ? 4u : 1u;
    const uint32_t blk_h = (is_bc && dim != 2u) ? 4u : 1u;

    const uint32_t mip_w = (tex_w >> p.mip_level) ? (tex_w >> p.mip_level) : 1u;
    const uint32_t mip_h = (dim == 2u) ? 1u : ((tex_h >> p.mip_level) ? (tex_h >> p.mip_level) : 1u);
    const uint32_t mip_d = (dim == 4u) ? ((tex_d >> p.mip_level) ? (tex_d >> p.mip_level) : 1u) : 1u;

    uint32_t tx0 = 0u, ty0 = 0u, tz0 = 0u;
    uint32_t tx1 = mip_w, ty1 = mip_h, tz1 = mip_d;
    if (dst_box != nullptr) {
        tx0 = dst_box[0]; ty0 = dst_box[1]; tz0 = dst_box[2];
        tx1 = dst_box[3]; ty1 = dst_box[4]; tz1 = dst_box[5];
        if (dim == 2u) { ty0 = 0u; ty1 = 1u; }
        if (dim != 4u) { tz0 = 0u; tz1 = 1u; }
    }
    if (tx0 >= tx1 || ty0 >= ty1 || tz0 >= tz1 ||
        tx1 > mip_w || ty1 > mip_h || tz1 > mip_d) {
        return false;
    }
    if ((tx0 % blk_w) != 0u || (ty0 % blk_h) != 0u) {
        return false;
    }
    if (((tx1 - tx0) % blk_w) != 0u && tx1 != mip_w) {
        return false;
    }
    if (((ty1 - ty0) % blk_h) != 0u && ty1 != mip_h) {
        return false;
    }

    const uint32_t box_d = tz1 - tz0;
    const uint64_t row_bytes = static_cast<uint64_t>(p.box_w) * p.bpe;
    const uint64_t slice_bytes = row_bytes * p.box_h;
    if (row_bytes == 0ULL || row_bytes > 0x100000ULL) {
        return false;
    }

    const auto* src_desc_bytes = static_cast<const uint8_t*>(src_data_desc);
    const uint8_t* src_base = *reinterpret_cast<const uint8_t* const*>(src_desc_bytes);
    if (!src_base) {
        return false;
    }
    uint64_t src_row_pitch = *reinterpret_cast<const uint32_t*>(src_desc_bytes + 0x08);
    if (src_row_pitch == 0ULL) src_row_pitch = row_bytes;
    uint64_t src_slice_pitch = *reinterpret_cast<const uint32_t*>(src_desc_bytes + 0x0c);
    if (src_slice_pitch == 0ULL) src_slice_pitch = slice_bytes;

    if (!is_compute_ctx) {
        auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(ctx + 0xb468);
        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, ctx);
    }

    const uint64_t min_alloc = (row_bytes + 0x3FULL) & ~0x3FULL;
    uint32_t cur_block_y = 0u;
    uint32_t cur_z = 0u;
    uint64_t rem_bytes = slice_bytes * box_d;

    while (cur_z < box_d) {
        const uint64_t need_aligned = (rem_bytes + 0x3FULL) & ~0x3FULL;
        const uint64_t avail_a_raw = is_compute_ctx
            ? GX_CALL(uint64_t, 0x26b1790ULL, (void*), ctx)
            : GX_CALL(uint64_t, 0x26af6e0ULL, (void*), ctx);
        const uint64_t avail_a = (avail_a_raw > 0x100ULL) ? ((avail_a_raw - 0x100ULL) & ~0x3FULL) : 0ULL;
        const uint64_t avail_b_raw = is_compute_ctx
            ? GX_CALL(uint64_t, 0x26b1b90ULL, (void*, uint32_t), ctx, 0x40u)
            : GX_CALL(uint64_t, 0x26b0ae0ULL, (void*, uint32_t), ctx, 0x40u);
        const uint64_t avail_b = (avail_b_raw > 0x100ULL) ? ((avail_b_raw - 0x100ULL) & ~0x3FULL) : 0ULL;

        uint64_t alloc_sz = (avail_a < need_aligned) ? avail_a : need_aligned;
        if (alloc_sz < min_alloc) alloc_sz = min_alloc;
        const uint64_t req_sz = (avail_b >= min_alloc && avail_b < alloc_sz) ? avail_b : alloc_sz;

        uint8_t* stage_ptr = static_cast<uint8_t*>(is_compute_ctx
            ? GX_CALL(void*, 0x26b1990ULL, (void*, uint64_t, uint32_t), ctx, req_sz, 0x40u)
            : GX_CALL(void*, 0x26b07f0ULL, (void*, uint64_t, uint32_t), ctx, req_sz, 0x40u));
        if (!stage_ptr) {
            return (cur_z != 0u || cur_block_y != 0u);
        }

        uint32_t chunk_d = 1u;
        uint32_t chunk_block_rows = 1u;
        if (cur_block_y == 0u && req_sz >= slice_bytes) {
            const uint64_t max_slices = req_sz / slice_bytes;
            const uint32_t rem_slices = box_d - cur_z;
            chunk_d = (max_slices < rem_slices) ? static_cast<uint32_t>(max_slices) : rem_slices;
            chunk_block_rows = p.box_h;
        } else {
            const uint64_t max_rows = req_sz / row_bytes;
            const uint32_t rem_rows = p.box_h - cur_block_y;
            chunk_block_rows = (max_rows < rem_rows) ? static_cast<uint32_t>(max_rows) : rem_rows;
        }

        const uint64_t chunk_bytes = (chunk_d > 1u)
            ? (static_cast<uint64_t>(chunk_d) * slice_bytes)
            : (static_cast<uint64_t>(chunk_block_rows) * row_bytes);

        if (src_row_pitch == row_bytes && (chunk_d == 1u || src_slice_pitch == slice_bytes)) {
            const uint8_t* s_ptr = src_base +
                static_cast<uint64_t>(cur_z) * src_slice_pitch +
                static_cast<uint64_t>(cur_block_y) * src_row_pitch;
            std::memcpy(stage_ptr, s_ptr, static_cast<size_t>(chunk_bytes));
        } else {
            for (uint32_t dz = 0u; dz < chunk_d; ++dz) {
                const uint8_t* src_slice = src_base + static_cast<uint64_t>(cur_z + dz) * src_slice_pitch;
                uint8_t* dst_slice = stage_ptr + static_cast<uint64_t>(dz) * slice_bytes;
                if (src_row_pitch == row_bytes) {
                    std::memcpy(
                        dst_slice,
                        src_slice + static_cast<uint64_t>(cur_block_y) * src_row_pitch,
                        static_cast<size_t>(chunk_block_rows) * static_cast<size_t>(row_bytes));
                } else {
                    for (uint32_t ry = 0u; ry < chunk_block_rows; ++ry) {
                        std::memcpy(
                            dst_slice + static_cast<uint64_t>(ry) * row_bytes,
                            src_slice + static_cast<uint64_t>(cur_block_y + ry) * src_row_pitch,
                            static_cast<size_t>(row_bytes));
                    }
                }
            }
        }

        const uint32_t off_y = ty0 + cur_block_y * blk_h;
        const uint32_t max_tex_h = ty1 - off_y;
        const uint32_t ext_h = (chunk_block_rows * blk_h < max_tex_h) ? (chunk_block_rows * blk_h) : max_tex_h;

        gx_direct_vk::UploadTextureArgs up_args{};
        std::memcpy(up_args.real_tsharp, real_tsharp, sizeof(real_tsharp));
        up_args.src_stage_va = reinterpret_cast<uint64_t>(stage_ptr);
        up_args.dst_va = *reinterpret_cast<const uint64_t*>(dst_tex + 0x08);
        up_args.src_byte_size = static_cast<uint32_t>(chunk_bytes);
        up_args.dst_byte_size = *reinterpret_cast<const uint32_t*>(dst_tex + 0x14);
        up_args.offset_x = tx0;
        up_args.offset_y = off_y;
        up_args.offset_z = tz0 + cur_z;
        up_args.extent_w = tx1 - tx0;
        up_args.extent_h = ext_h;
        up_args.extent_d = chunk_d;
        const uint8_t dim_type = dst_tex[0x20];
        const uint8_t usage_mode = dst_tex[0x21];
        const uint8_t cpu_access = dst_tex[0x22];
        const uint8_t bind_flags = dst_tex[0x23];
        const uint16_t sample_cnt =
            (dim_type == 3u) ? *reinterpret_cast<const uint16_t*>(dst_tex + 0x34) : 1u;
        const bool is_immutable_asset =
            gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN2_NativeTextures) &&
            usage_mode == 1u && bind_flags == 8u && cpu_access == 0u && sample_cnt <= 1u;

        up_args.buf_row_len = p.box_w * blk_w;
        up_args.buf_img_height = chunk_block_rows * blk_h;
        up_args.reserved0 = p.mip_level | (is_immutable_asset ? 0x80000000u : 0u);
        up_args.reserved1 = p.array_slice;

        gx_direct_vk::EmitUploadTextureSubresource(ctx, up_args);
        if (is_compute_ctx) {
            GX_CALL(void, 0x26b17b0ULL, (void*), ctx);
        } else {
            GX_CALL(void, 0x26af700ULL, (void*), ctx);
        }

        rem_bytes -= chunk_bytes;
        if (chunk_d > 1u) {
            cur_z += chunk_d;
        } else {
            cur_block_y += chunk_block_rows;
            if (cur_block_y >= p.box_h) {
                cur_block_y = 0u;
                cur_z += 1u;
            }
        }
    }
    return true;
}

// VMA: 0x26b4230 (3261 B)
void CGGpuBlitAndClearPassesGnm_UpdateTextureSubresourceFromCpu(
    uint8_t* compute_ctx,
    const uint8_t* dst_tex,
    const void* src_data_desc,
    uint32_t subresource_idx,
    const uint32_t* dst_box)
{
    CGTextureUpdatePrep p = {};
    if (!CGGpuBlitAndClearPassesGnm_PrepareTextureUpdate(p, dst_tex, subresource_idx, dst_box)) {
        return;
    }
    if (TryDirectUploadTextureSubresource(compute_ctx, true, p, dst_tex, src_data_desc, dst_box)) {
        return;
    }
    GX_CALL(void, 0x1089760ULL, (void*, int32_t, uint32_t), p.tsharp, 0x60, 0u);

    if (p.bc_mips <= p.mip_level) {
        uint64_t surf_off = 0ULL, surf_sz = 0ULL;
        const int32_t err = GX_CALL(int32_t, 0x10a2c40ULL,
                                    (uint64_t*, uint64_t*, const void*, uint32_t, uint32_t),
                                    &surf_off, &surf_sz, p.tsharp, p.mip_level, p.array_slice);
        if (err != 0) {
            GX_CALL(void, 0x26bb4a0ULL, (int32_t), err);
            return;
        }
        void* dst_gpu_addr = *reinterpret_cast<uint8_t* const*>(dst_tex + 0x08) + surf_off;
        sub_2bbe578_memcpy(dst_gpu_addr, *reinterpret_cast<const void* const*>(src_data_desc), p.bpe);
        return;
    }

    GX_CALL(int32_t, 0x10884f0ULL, (void*, uint32_t, uint32_t), p.tsharp, p.array_slice, p.array_slice);
    GX_CALL(int32_t, 0x1088510ULL, (void*, uint32_t, uint32_t), p.tsharp, p.mip_level, p.mip_level);
    GX_CALL(void, 0x10890c0ULL, (void*, uint32_t),
            p.tsharp, CGPipelineStatePackerGnm_AlignTo256Blocks(*reinterpret_cast<const uint64_t*>(dst_tex + 0x08)));

    const uint64_t row_bytes = static_cast<uint64_t>(p.box_w) * p.bpe;
    const uint64_t slice_bytes = static_cast<uint64_t>(p.box_w * p.box_h) * p.bpe;
    uint64_t src_row_pitch = *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(src_data_desc) + 0x08);
    if (src_row_pitch == 0ULL) src_row_pitch = row_bytes;
    uint64_t src_slice_pitch = *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(src_data_desc) + 0x0c);
    if (src_slice_pitch == 0ULL) src_slice_pitch = slice_bytes;

    const uint8_t* cs_bytes = static_cast<const uint8_t*>(p.cs_hdr);
    const uint32_t num_usages = *reinterpret_cast<const uint32_t*>(cs_bytes + 0xe8);
    const uint32_t* usage_tbl = *reinterpret_cast<const uint32_t* const*>(cs_bytes + 0xe0);

    const uint32_t uav_idx = cs_bytes[0xbc];
    if (uav_idx == 0xFFu || uav_idx >= num_usages) return;
    const uint32_t uav_entry = usage_tbl[uav_idx];
    const uint32_t uav_type = uav_entry & 0xFFu, uav_reg = (uav_entry >> 8) & 0xFFu;
    uint32_t max_eud_reg = 0u;
    if (uav_type == 0x10u) max_eud_reg = uav_reg;
    else if (uav_type != 3u) return;

    const uint32_t srv_idx = cs_bytes[0x18];
    if (srv_idx == 0xFFu || srv_idx >= num_usages) return;
    const uint32_t srv_entry = usage_tbl[srv_idx];
    const uint32_t srv_type = srv_entry & 0xFFu, srv_reg = (srv_entry >> 8) & 0xFFu;
    if (srv_type == 0xDu) { if (srv_reg > max_eud_reg) max_eud_reg = srv_reg; }
    else if (srv_type != 0u) return;

    const uint32_t cb_idx = cs_bytes[0xa8];
    if (cb_idx == 0xFFu || cb_idx >= num_usages) return;
    const uint32_t cb_entry = usage_tbl[cb_idx];
    const uint32_t cb_type = cb_entry & 0xFFu, cb_reg = (cb_entry >> 8) & 0xFFu;
    if (cb_type == 0x12u) { if (cb_reg > max_eud_reg) max_eud_reg = cb_reg; }
    else if (cb_type != 5u) return;

    uint32_t eud_bytes = 0u, eud_start_reg = 0u;
    if (max_eud_reg != 0u) {
        eud_bytes = max_eud_reg * 4u - 0x20u;
        eud_start_reg = reinterpret_cast<const uint8_t*>(usage_tbl)[static_cast<size_t>(cs_bytes[0xd3]) * 4u + 1u];
    }

    GX_CALL(void, 0x108a870ULL, (void*, const void*),
            compute_ctx, *reinterpret_cast<const uint8_t* const*>(cs_bytes + 0x100) + 0x08);
    if (cs_bytes[0xd2] != 0u) {
        GX_CALL(void, 0x108aad0ULL, (void*, uint32_t, const void*), compute_ctx, 0xFu, cs_bytes + 0xf0);
    }
    if (p.total_bytes == 0ULL) return;

    const uint64_t hdr_overhead = static_cast<uint64_t>(eud_bytes) + 0x20ULL;
    const uint64_t min_alloc = static_cast<uint64_t>(eud_bytes + 0x30u);
    uint32_t cur_x = 0u;
    uint64_t cur_y = 0ULL, cur_z = 0ULL, rem_bytes = p.total_bytes;
    while (rem_bytes != 0ULL) {
        const uint64_t need_aligned = ((rem_bytes + 0x3FULL) & ~0x3FULL) + hdr_overhead;
        const uint64_t avail_a_raw = GX_CALL(uint64_t, 0x26b1790ULL, (void*), compute_ctx);
        const uint64_t avail_a = (avail_a_raw > 0x100ULL) ? ((avail_a_raw - 0x100ULL) & ~0x3FULL) : 0ULL;
        const uint64_t avail_b_raw = GX_CALL(uint64_t, 0x26b1b90ULL, (void*, uint32_t), compute_ctx, 0x40u);
        const uint64_t avail_b = (avail_b_raw > 0x100ULL) ? ((avail_b_raw - 0x100ULL) & ~0x3FULL) : 0ULL;
        uint64_t alloc_sz = (avail_a < need_aligned) ? avail_a : need_aligned;
        if (alloc_sz < min_alloc) alloc_sz = min_alloc;
        const uint64_t req_sz = (avail_b >= min_alloc && avail_b < alloc_sz) ? avail_b : alloc_sz;
        uint8_t* stage_ptr = static_cast<uint8_t*>(
            GX_CALL(void*, 0x26b1990ULL, (void*, uint64_t, uint32_t), compute_ctx, req_sz, 0x40u));
        if (stage_ptr == nullptr) return;
        alloc_sz = req_sz;

        const uint64_t max_payload = alloc_sz - hdr_overhead;
        const uint64_t chunk_bytes = (rem_bytes < max_payload) ? rem_bytes : max_payload;
        uint8_t* eud_ptr = stage_ptr + max_payload;
        if (eud_bytes != 0u) {
            GX_CALL(void, 0x108ac90ULL, (void*, uint32_t, const void*), compute_ctx, eud_start_reg, eud_ptr);
        }
        uint32_t* cb_ptr = reinterpret_cast<uint32_t*>(stage_ptr + alloc_sz - 0x20ULL);
        if (uav_reg <= 0xFu) {
            GX_CALL(void, 0x108ab60ULL, (void*, uint32_t, const void*), compute_ctx, uav_reg, p.tsharp);
        } else {
            auto* dst = reinterpret_cast<uint64_t*>(eud_ptr + (uav_reg - 0x10u) * 4u);
            dst[0] = p.tsharp[0]; dst[1] = p.tsharp[1]; dst[2] = p.tsharp[2]; dst[3] = p.tsharp[3];
        }

        const uint64_t chunk_elems = chunk_bytes / p.bpe;
        CGGpuBlitAndClearPassesGnm_CopyStagedChunk(
            stage_ptr, chunk_bytes, cur_x, cur_y, cur_z, p.bpe, row_bytes, slice_bytes,
            *reinterpret_cast<const uint8_t* const*>(src_data_desc), src_row_pitch, src_slice_pitch);

        uint64_t srv_vsharp[2] = {};
        GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
                srv_vsharp, stage_ptr, p.data_fmt, static_cast<uint32_t>(chunk_elems));
        GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), srv_vsharp, 0x6E, 0u, 1u);
        if (srv_reg <= 0xFu) {
            GX_CALL(void, 0x108aad0ULL, (void*, uint32_t, const void*), compute_ctx, srv_reg, srv_vsharp);
        } else {
            auto* dst = reinterpret_cast<uint64_t*>(eud_ptr + (srv_reg - 0x10u) * 4u);
            dst[0] = srv_vsharp[0]; dst[1] = srv_vsharp[1];
        }

        cb_ptr[0] = static_cast<uint32_t>(chunk_elems);
        cb_ptr[1] = static_cast<uint32_t>((p.total_bytes - rem_bytes) / p.bpe);
        cb_ptr[2] = p.box_w;  cb_ptr[3] = p.box_h;
        cb_ptr[4] = p.adj_x0; cb_ptr[5] = p.adj_y0;
        cb_ptr[6] = p.z0;     cb_ptr[7] = 0u;

        uint64_t cb_vsharp[2] = {};
        GX_CALL(void, 0x1070960ULL, (void*, const void*, uint32_t), cb_vsharp, cb_ptr, 0x20u);
        GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), cb_vsharp, 0x6E, 1u, 1u);
        if (cb_reg <= 0xFu) {
            GX_CALL(void, 0x108aad0ULL, (void*, uint32_t, const void*), compute_ctx, cb_reg, cb_vsharp);
        } else {
            auto* dst = reinterpret_cast<uint64_t*>(eud_ptr + (cb_reg - 0x10u) * 4u);
            dst[0] = cb_vsharp[0]; dst[1] = cb_vsharp[1];
        }

        GX_CALL(void, 0x108a350ULL, (void*, uint32_t, uint32_t, uint32_t),
                compute_ctx, (static_cast<uint32_t>(chunk_elems) + 63u) >> 6, 1u, 1u);
        GX_CALL(void, 0x26b17b0ULL, (void*), compute_ctx);
        rem_bytes -= chunk_bytes;
    }
}

// VMA: 0x26b35e0 (3145 B)
void CGGpuBlitAndClearPassesGnm_UpdateTextureSubresourceViaCompute(
    uint8_t* gnmx_ctx,
    const uint8_t* dst_tex,
    const void* src_data_desc,
    uint32_t subresource_idx,
    const uint32_t* dst_box,
    bool sync_barriers)
{
    void* binder = gnmx_ctx + 0x40;
    CGTextureUpdatePrep p = {};
    if (!CGGpuBlitAndClearPassesGnm_PrepareTextureUpdate(p, dst_tex, subresource_idx, dst_box)) {
        return;
    }

    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx + 0xb468);
    const uint32_t dst_swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, dst_tex);
    if (TryDirectUploadTextureSubresource(gnmx_ctx, false, p, dst_tex, src_data_desc, dst_box)) {
        if (sync_barriers) {
            CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_tex);
        }
        return;
    }
    GX_CALL(void, 0x1089760ULL, (void*, int32_t, uint32_t),
            p.tsharp, static_cast<int32_t>(dst_swz) >> 16, (dst_swz >> 8) & 0xFFu);

    if (p.bc_mips <= p.mip_level) {
        uint64_t surf_off = 0ULL, surf_sz = 0ULL;
        const int32_t err = GX_CALL(int32_t, 0x10a2c40ULL,
                                    (uint64_t*, uint64_t*, const void*, uint32_t, uint32_t),
                                    &surf_off, &surf_sz, p.tsharp, p.mip_level, p.array_slice);
        if (err != 0) {
            GX_CALL(void, 0x26bb4a0ULL, (int32_t), err);
            return;
        }
        void* dst_gpu_addr = *reinterpret_cast<uint8_t* const*>(dst_tex + 0x08) + surf_off;
        GX_CALL(void, 0x26b1c40ULL, (void*, void*, const void*, size_t, uint32_t),
                gnmx_ctx, dst_gpu_addr, *reinterpret_cast<const void* const*>(src_data_desc), p.bpe, dst_swz);
        if (sync_barriers) {
            CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_tex);
        }
        return;
    }

    GX_CALL(int32_t, 0x10884f0ULL, (void*, uint32_t, uint32_t), p.tsharp, p.array_slice, p.array_slice);
    GX_CALL(int32_t, 0x1088510ULL, (void*, uint32_t, uint32_t), p.tsharp, p.mip_level, p.mip_level);
    GX_CALL(void, 0x10890c0ULL, (void*, uint32_t),
            p.tsharp, CGPipelineStatePackerGnm_AlignTo256Blocks(*reinterpret_cast<const uint64_t*>(dst_tex + 0x08)));

    const uint64_t row_bytes = static_cast<uint64_t>(p.box_w) * p.bpe;
    const uint64_t slice_bytes = static_cast<uint64_t>(p.box_w * p.box_h) * p.bpe;
    uint64_t src_row_pitch = *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(src_data_desc) + 0x08);
    if (src_row_pitch == 0ULL) src_row_pitch = row_bytes;
    uint64_t src_slice_pitch = *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(src_data_desc) + 0x0c);
    if (src_slice_pitch == 0ULL) src_slice_pitch = slice_bytes;

    uint32_t cur_x = 0u;
    uint64_t cur_y = 0ULL, cur_z = 0ULL, rem_bytes = p.total_bytes;
    while (static_cast<uint32_t>(rem_bytes) != 0u) {
        GX_CALL(void, 0x26cd400ULL, (void*, void*, const void*), binder, gnmx_ctx, p.cs_hdr);
        auto* udst = reinterpret_cast<uint64_t*>(gnmx_ctx + 0x9320);
        udst[0] = p.tsharp[0]; udst[1] = p.tsharp[1]; udst[2] = p.tsharp[2]; udst[3] = p.tsharp[3];
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fa8) |= 0x1000000000ULL;
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fb0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx + 0x2518);

        const uint64_t need_aligned = ((rem_bytes + 0x3FULL) & ~0x3FULL) | 0x20ULL;
        const uint64_t avail_a_raw = GX_CALL(uint64_t, 0x26af6e0ULL, (void*), gnmx_ctx);
        const uint64_t avail_a = (avail_a_raw > 0x100ULL) ? ((avail_a_raw - 0x100ULL) & ~0x3FULL) : 0ULL;
        const uint64_t avail_b_raw = GX_CALL(uint64_t, 0x26b0ae0ULL, (void*, uint32_t), gnmx_ctx, 0x40u);
        const uint64_t avail_b = (avail_b_raw > 0x100ULL) ? ((avail_b_raw - 0x100ULL) & ~0x3FULL) : 0ULL;
        uint64_t alloc_sz = (avail_a < need_aligned) ? avail_a : need_aligned;
        if (alloc_sz < 0x30ULL) alloc_sz = 0x30ULL;
        const uint64_t req_sz = (avail_b >= 0x30ULL && avail_b < alloc_sz) ? avail_b : alloc_sz;
        uint8_t* stage_ptr = static_cast<uint8_t*>(
            GX_CALL(void*, 0x26b07f0ULL, (void*, uint64_t, uint32_t), gnmx_ctx, req_sz, 0x40u));
        if (stage_ptr == nullptr) {
            gnmx_ctx[0x1f98] |= 1u;
            *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fa8) |= 0x1000010000ULL;
            return;
        }
        alloc_sz = req_sz;

        const uint64_t max_payload = alloc_sz - 0x20ULL;
        const uint64_t chunk_bytes = (rem_bytes < max_payload) ? rem_bytes : max_payload;
        const uint64_t chunk_elems = chunk_bytes / p.bpe;
        CGGpuBlitAndClearPassesGnm_CopyStagedChunk(
            stage_ptr, chunk_bytes, cur_x, cur_y, cur_z, p.bpe, row_bytes, slice_bytes,
            *reinterpret_cast<const uint8_t* const*>(src_data_desc), src_row_pitch, src_slice_pitch);

        uint64_t srv_vsharp[2] = {};
        GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
                srv_vsharp, stage_ptr, p.data_fmt, static_cast<uint32_t>(chunk_elems));
        GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), srv_vsharp, 0x6E, 0u, 1u);
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x7920) = srv_vsharp[0];
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x7928) = srv_vsharp[1];
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fb0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx + 0x1ff8);
        gnmx_ctx[0x1f98] |= 1u;

        auto* cb_ptr = reinterpret_cast<uint32_t*>(stage_ptr + max_payload);
        cb_ptr[0] = static_cast<uint32_t>(chunk_elems);
        cb_ptr[1] = static_cast<uint32_t>((p.total_bytes - rem_bytes) / p.bpe);
        cb_ptr[2] = p.box_w;  cb_ptr[3] = p.box_h;
        cb_ptr[4] = p.adj_x0; cb_ptr[5] = p.adj_y0;
        cb_ptr[6] = p.z0;     cb_ptr[7] = 0u;

        uint64_t cb_vsharp[2] = {};
        GX_CALL(void, 0x1070960ULL, (void*, const void*, uint32_t), cb_vsharp, cb_ptr, 0x20u);
        GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), cb_vsharp, 0x6E, 0u, 0u);
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0xa160) = cb_vsharp[0];
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0xa168) = cb_vsharp[1];
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fa8) |= 0x10000ULL;
        *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fb0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx + 0x2478);

        GX_CALL(void, 0x26cd5d0ULL, (void*, void*, const void*), binder, gnmx_ctx, p.cs_hdr);
        GX_CALL(void, 0x1074c50ULL, (void*, uint32_t, uint32_t, uint32_t),
                gnmx_ctx, (static_cast<uint32_t>(chunk_elems) + 63u) >> 6, 1u, 1u);
        GX_CALL(void, 0x26ce110ULL, (void*, void*), binder, gnmx_ctx);
        GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
        rem_bytes -= chunk_bytes;
    }

    if (sync_barriers) {
        CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_tex);
    }
    gnmx_ctx[0x1f98] |= 1u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx + 0x1fa8) |= 0x1000010000ULL;
}

// VMA: 0x26b3560 (113 B)
void CGGpuBlitAndClearPassesGnm_UpdateBufferSubresource(
    uint8_t* gnmx_ctx,
    const uint8_t* dst_buf,
    const void* src_data,
    uint32_t byte_offset,
    size_t byte_size)
{
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx + 0xb468);
    const uint32_t swizzle = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, dst_buf);
    void* dst_gpu_addr = *reinterpret_cast<uint8_t* const*>(dst_buf + 0x08) + byte_offset;
    GX_CALL(void, 0x26b1c40ULL, (void*, void*, const void*, size_t, uint32_t),
            gnmx_ctx, dst_gpu_addr, src_data, byte_size, swizzle);
    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, swizzle, 0, dst_buf);
}

// VMA: 0x26b2f40 (1561 B)
void CGGpuBlitAndClearPassesGnm_CopyTextureRegionViaCompute(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* dst_tex,
    uint32_t dst_subresource,
    uint32_t dst_x,
    uint32_t dst_y,
    uint32_t dst_z,
    const uint8_t* src_tex,
    uint32_t src_subresource,
    const uint32_t* src_box)
{
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
    const uint32_t dst_swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, dst_tex);
    const uint32_t src_swz = CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(sync_tracker, 0, src_tex);

    const uint32_t src_depth = (src_tex[0x20] == 4u) ? *reinterpret_cast<const uint16_t*>(src_tex + 0x30) : 1u;
    uint32_t src_mip_cnt = src_tex[0x32];
    if (src_mip_cnt == 0u) {
        src_mip_cnt = GX_CALL(uint32_t, 0x2165130ULL, (uint32_t, uint32_t, uint32_t),
                              *reinterpret_cast<const uint16_t*>(src_tex + 0x2c),
                              *reinterpret_cast<const uint16_t*>(src_tex + 0x2e),
                              src_depth);
    }
    const uint32_t src_mip = GX_CALL(uint32_t, 0x2164d60ULL, (uint32_t, uint32_t), src_subresource, src_mip_cnt);
    const uint32_t src_slice = GX_CALL(uint32_t, 0x2164d50ULL, (uint32_t, uint32_t), src_subresource, src_mip_cnt);

    const uint32_t dst_depth = (dst_tex[0x20] == 4u) ? *reinterpret_cast<const uint16_t*>(dst_tex + 0x30) : 1u;
    uint32_t dst_mip_cnt = dst_tex[0x32];
    if (dst_mip_cnt == 0u) {
        dst_mip_cnt = GX_CALL(uint32_t, 0x2165130ULL, (uint32_t, uint32_t, uint32_t),
                              *reinterpret_cast<const uint16_t*>(dst_tex + 0x2c),
                              *reinterpret_cast<const uint16_t*>(dst_tex + 0x2e),
                              dst_depth);
    }
    const uint32_t dst_mip = GX_CALL(uint32_t, 0x2164d60ULL, (uint32_t, uint32_t), dst_subresource, dst_mip_cnt);
    const uint32_t dst_slice = GX_CALL(uint32_t, 0x2164d50ULL, (uint32_t, uint32_t), dst_subresource, dst_mip_cnt);

    uint64_t srv_tsharp[4] = {};
    if (GX_CALL(int32_t, 0x216d900ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                srv_tsharp, src_tex, src_tex[0x33], src_mip, src_slice, 1u, 1u, 0u) != 0) {
        return;
    }
    GX_CALL(void, 0x1089760ULL, (void*, int32_t, uint32_t),
            srv_tsharp, static_cast<int32_t>(src_swz) >> 16, (src_swz >> 8) & 0xFFu);
    gx_direct_vk::RegisterDirectNativeSrv(
        srv_tsharp, *reinterpret_cast<const uint32_t*>(src_tex + 0x10), 0u);

    uint64_t uav_tsharp[4] = {};
    if (GX_CALL(int32_t, 0x216d900ULL,
                (void*, const void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t),
                uav_tsharp, dst_tex, dst_tex[0x33], dst_mip, dst_slice, 1u, 1u, 0u) != 0) {
        return;
    }
    GX_CALL(void, 0x1089760ULL, (void*, int32_t, uint32_t),
            uav_tsharp, static_cast<int32_t>(dst_swz) >> 16, (dst_swz >> 8) & 0xFFu);
    gx_direct_vk::RegisterDirectNativeSrv(
        uav_tsharp, *reinterpret_cast<const uint32_t*>(dst_tex + 0x10), 0u);

    uint32_t dst_mip_w = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(dst_tex + 0x2c)) >> dst_mip;
    if (!dst_mip_w) dst_mip_w = 1u;
    uint32_t dst_mip_h = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(dst_tex + 0x2e)) >> dst_mip;
    if (!dst_mip_h) dst_mip_h = 1u;
    uint32_t dst_mip_d = dst_depth >> dst_mip;
    if (!dst_mip_d) dst_mip_d = 1u;

    const uint32_t src_w = *reinterpret_cast<const uint16_t*>(src_tex + 0x2c);
    uint32_t src_mip_w = src_w >> src_mip;
    if (!src_mip_w) src_mip_w = 1u;
    const uint32_t src_h = *reinterpret_cast<const uint16_t*>(src_tex + 0x2e);
    uint32_t src_mip_h = src_h >> src_mip;
    if (!src_mip_h) src_mip_h = 1u;
    const uint32_t src_d = src_depth;
    uint32_t src_mip_d = src_d >> src_mip;
    if (!src_mip_d) src_mip_d = 1u;

    uint32_t sx0 = 0u, sy0 = 0u, sz0 = 0u, sx1 = src_w, sy1 = src_h, sz1 = src_d;
    if (src_box != nullptr) {
        sx0 = src_box[0]; sy0 = src_box[1]; sz0 = src_box[2];
        sx1 = src_box[3]; sy1 = src_box[4]; sz1 = src_box[5];
    }
    if (sx1 < src_mip_w) src_mip_w = sx1;
    uint32_t max_sx1 = (dst_mip_w - static_cast<uint32_t>(dst_x)) + sx0;
    if (src_mip_w < max_sx1) max_sx1 = src_mip_w;

    if (sy1 < src_mip_h) src_mip_h = sy1;
    uint32_t max_sy1 = (dst_mip_h - static_cast<uint32_t>(dst_y)) + sy0;
    if (src_mip_h < max_sy1) max_sy1 = src_mip_h;

    if (sz1 < src_mip_d) src_mip_d = sz1;
    uint32_t max_sz1 = (dst_mip_d - static_cast<uint32_t>(dst_z)) + sz0;
    if (src_mip_d < max_sz1) max_sz1 = src_mip_d;

    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_CopyTextureRegion)) {
        const uint8_t dim = dst_tex[0x20];
        const uint32_t srv_fmt10 = static_cast<uint32_t>((srv_tsharp[0] >> 52) & 0x3FFULL);
        const uint32_t uav_fmt10 = static_cast<uint32_t>((uav_tsharp[0] >> 52) & 0x3FFULL);
        const uint32_t dfmt = srv_fmt10 & 0x3Fu;
        const uint32_t srv_sel12 = static_cast<uint32_t>((srv_tsharp[1] >> 32) & 0xFFFULL);
        const uint32_t uav_sel12 = static_cast<uint32_t>((uav_tsharp[1] >> 32) & 0xFFFULL);
        if ((dim == 2u || dim == 3u || dim == 4u) &&
            src_tex[0x20] == dim &&
            (src_tex[0x23] & 0x40u) == 0u && (dst_tex[0x23] & 0x40u) == 0u &&
            *reinterpret_cast<const uint16_t*>(src_tex + 0x34) ==
                *reinterpret_cast<const uint16_t*>(dst_tex + 0x34) &&
            srv_fmt10 == uav_fmt10 && srv_sel12 == uav_sel12 &&
            ((dfmt >= 1u && dfmt <= 19u) || dfmt == 34u) &&
            max_sx1 >= sx0 && max_sy1 >= sy0 && max_sz1 >= sz0) {
            void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
            gx_direct_vk::CopyTextureRegionArgs copy_args{};
            std::memcpy(copy_args.srv_tsharp, srv_tsharp, sizeof(srv_tsharp));
            std::memcpy(copy_args.uav_tsharp, uav_tsharp, sizeof(uav_tsharp));
            copy_args.dst_va = *reinterpret_cast<const uint64_t*>(dst_tex + 0x08);
            copy_args.src_va = *reinterpret_cast<const uint64_t*>(src_tex + 0x08);
            copy_args.dst_byte_size = *reinterpret_cast<const uint32_t*>(dst_tex + 0x14);
            copy_args.src_byte_size = *reinterpret_cast<const uint32_t*>(src_tex + 0x14);
            copy_args.src_x = sx0;
            copy_args.src_y = sy0;
            copy_args.src_z = sz0;
            copy_args.dst_x = dst_x;
            copy_args.dst_y = dst_y;
            copy_args.dst_z = dst_z;
            copy_args.copy_w = max_sx1 - sx0;
            copy_args.copy_h = max_sy1 - sy0;
            copy_args.copy_d = max_sz1 - sz0;
            copy_args.reserved = 0u;
            GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
            if (gx_direct_vk::EmitCopyTextureRegion(gnmx_ctx, copy_args)) {
                CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_tex);
                GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
                return;
            }
        }
    }

    uint32_t* cb_mem = static_cast<uint32_t*>(
        GX_CALL(void*, 0x26ae7f0ULL, (void*, void*, uint32_t),
                gnmx_ctx_wrapper + 0xb3c0, gnmx_ctx_wrapper, 0x30u));
    if (cb_mem == nullptr || max_sx1 <= sx0 || max_sy1 <= sy0 || max_sz1 <= sz0) {
        return;
    }

    const uint32_t copy_w = max_sx1 - sx0;
    const uint32_t copy_h = max_sy1 - sy0;
    const uint32_t copy_d = max_sz1 - sz0;
    cb_mem[0] = sx0; cb_mem[1] = sy0; cb_mem[2] = sz0; cb_mem[3] = 0u;
    cb_mem[4] = static_cast<uint32_t>(dst_x);
    cb_mem[5] = static_cast<uint32_t>(dst_y);
    cb_mem[6] = static_cast<uint32_t>(dst_z);
    cb_mem[7] = 0u;
    cb_mem[8]  = copy_w + static_cast<uint32_t>(dst_x);
    cb_mem[9]  = copy_h + static_cast<uint32_t>(dst_y);
    cb_mem[10] = copy_d + static_cast<uint32_t>(dst_z);
    cb_mem[11] = 0u;

    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    void* binder = gnmx_ctx_wrapper + 0x50;
    const void* cs_hdr = nullptr;
    uint32_t gx = 1u, gy = 1u, gz = 1u;
    const uint8_t dim = dst_tex[0x20];
    if (dim == 3u) {
        gx = (copy_w + 7u) >> 3;
        gy = (copy_h + 7u) >> 3;
        cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546bc30ULL));
    } else if (dim == 4u) {
        gx = (copy_w + 3u) >> 2;
        gy = (copy_h + 3u) >> 2;
        gz = (copy_d + 3u) >> 2;
        cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546bd50ULL));
    } else if (dim == 2u) {
        gx = (copy_w + 63u) >> 6;
        cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546bb10ULL));
    } else {
        return;
    }
    GX_CALL(void, 0x26cd400ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);

    auto* sdst = reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x7930);
    sdst[0] = srv_tsharp[0]; sdst[1] = srv_tsharp[1]; sdst[2] = srv_tsharp[2]; sdst[3] = srv_tsharp[3];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2008);
    gnmx_ctx_wrapper[0x1fa8] |= 1u;

    auto* udst = reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9330);
    udst[0] = uav_tsharp[0]; udst[1] = uav_tsharp[1]; udst[2] = uav_tsharp[2]; udst[3] = uav_tsharp[3];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000000000ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2528);

    uint64_t cb_vsharp[2] = {};
    GX_CALL(void, 0x1070960ULL, (void*, const void*, uint32_t), cb_vsharp, cb_mem, 0x30u);
    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t), cb_vsharp, 0x6E, 0u, 0u);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa170) = cb_vsharp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0xa178) = cb_vsharp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x10000ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2488);

    GX_CALL(void, 0x26cd5d0ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);
    GX_CALL(void, 0x1074c50ULL, (void*, uint32_t, uint32_t, uint32_t), gnmx_ctx, gx, gy, gz);
    GX_CALL(void, 0x26ce110ULL, (void*, void*), binder, gnmx_ctx);
    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_tex);
    GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
    gnmx_ctx_wrapper[0x1fa8] |= 1u;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000010000ULL;
}

// VMA: 0x26b2d30 (524 B)
void CGGpuBlitAndClearPassesGnm_CopyBufferRegionViaCompute(
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* dst_buf,
    const uint8_t* src_buf,
    uint32_t dst_offset,
    uint32_t src_offset,
    uint64_t byte_size)
{
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_CopyRegion) && gnmx_ctx_wrapper && dst_buf && src_buf) {
        const uint64_t copy_bytes = (byte_size >> 2) << 2;
        if (copy_bytes == 0ULL) {
            return;
        }
        void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
        auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
        const uint32_t dst_swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, dst_buf);
        (void)CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(sync_tracker, 0, src_buf);
        GX_CALL(void, 0x26b9850ULL, (void*, void*), sync_tracker, gnmx_ctx);
        const uint64_t dst_va = *reinterpret_cast<const uint64_t*>(dst_buf + 0x08) + dst_offset;
        const uint64_t src_va = *reinterpret_cast<const uint64_t*>(src_buf + 0x08) + src_offset;
        const uint32_t copy_flags =
            (src_buf[0x21] == 3u) ? gx_direct_vk::kCopyFlagKnownCpuSource : 0u;
        if (gx_direct_vk::EmitCopyBuffer(gnmx_ctx, dst_va, src_va, copy_bytes, copy_flags)) {
            CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_buf);
            GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
            return;
        }
    }
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    void* binder = gnmx_ctx_wrapper + 0x50;
    const uint32_t num_dwords = static_cast<uint32_t>(byte_size >> 2);
    auto* sync_tracker = reinterpret_cast<CGSurfaceSyncTrackerState*>(gnmx_ctx_wrapper + 0xb478);
    const uint32_t dst_swz = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(sync_tracker, 0, dst_buf);
    const uint32_t src_swz = CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(sync_tracker, 0, src_buf);

    const void* cs_hdr = reinterpret_cast<const void*>(GX_VMA(0x546b8c8ULL));
    GX_CALL(void, 0x26cd400ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);

    uint64_t srv_vsharp[2] = {};
    GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
            srv_vsharp, *reinterpret_cast<const uint8_t* const*>(src_buf + 0x08) + src_offset, 0x4404u, num_dwords);
    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t),
            srv_vsharp, static_cast<int32_t>(src_swz) >> 16, (src_swz >> 8) & 0xFFu, src_swz & 0xFFu);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x7930) = srv_vsharp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x7938) = srv_vsharp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2008);
    gnmx_ctx_wrapper[0x1fa8] |= 1u;

    uint64_t uav_vsharp[2] = {};
    GX_CALL(void, 0x10707c0ULL, (void*, const void*, uint32_t, uint32_t),
            uav_vsharp, *reinterpret_cast<const uint8_t* const*>(dst_buf + 0x08) + dst_offset, 0x4404u, num_dwords);
    GX_CALL(void, 0x10710c0ULL, (void*, int32_t, uint32_t, uint32_t),
            uav_vsharp, static_cast<int32_t>(dst_swz) >> 16, (dst_swz >> 8) & 0xFFu, dst_swz & 0xFFu);
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9330) = uav_vsharp[0];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x9338) = uav_vsharp[1];
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fb8) |= 0x1000000000ULL;
    *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x1fc0) |= *reinterpret_cast<const uint64_t*>(gnmx_ctx_wrapper + 0x2528);

    GX_CALL(void, 0x26cd5d0ULL, (void*, void*, const void*), binder, gnmx_ctx, cs_hdr);
    GX_CALL(void, 0x1074c50ULL, (void*, uint32_t, uint32_t, uint32_t), gnmx_ctx, (num_dwords + 63u) >> 6, 1u, 1u);
    GX_CALL(void, 0x26ce110ULL, (void*, void*), binder, gnmx_ctx);
    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(sync_tracker, dst_swz, 0, dst_buf);
    GX_CALL(void, 0x26af700ULL, (void*), gnmx_ctx);
    gnmx_ctx_wrapper[0x1fa8] |= 1u;
    gnmx_ctx_wrapper[0x1fbc] |= 0x10u;
}

} // namespace Bloodborne::CoreGraphics2::Gnm
