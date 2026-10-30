// ============================================================================
// Bloodborne Engine Decompilation Project (CUSA03173 v1.09)
// Module: 01_CoreGraphics2_GNM
// File: CGRasterizerAndOutputMergerCacheGnm.cpp
// Range: 0x26d0bb0 .. 0x26d3370 (24 Functions — Rasterizer, Viewport, Scissor,
//        Output Merger RTV/DSV/Blend/DepthStencil Cache, CMASK Fast-Clear,
//        HTILE/FMASK Decompression, and Dynamic Buffer Ring Lookup)
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <immintrin.h>
#include "gx_direct_vulkan.h"

namespace Bloodborne::CoreGraphics2::Gnm {

extern "C" {
extern uintptr_t g_eboot_live_slide;

#define DEFINE_EBOOT_TRAMPOLINE(sym_name, vma_hex, ret_type, ...) \
    __attribute__((weak, naked, used, visibility("hidden"))) ret_type sym_name(__VA_ARGS__) { \
        __asm__ volatile( \
            "movq g_eboot_live_slide(%rip), %r11\n\t" \
            "testq %r11, %r11\n\t" \
            "jz 1f\n\t" \
            "addq $" #vma_hex ", %r11\n\t" \
            "jmpq *%r11\n\t" \
            "1:\n\t" \
            "xorl %eax, %eax\n\t" \
            "retq\n\t" \
        ); \
    }

DEFINE_EBOOT_TRAMPOLINE(sub_10738b0_setRenderTarget,      0x10738b0, void,     void* gnmx_gfxc, uint32_t slot, const void* rtv)
DEFINE_EBOOT_TRAMPOLINE(sub_10739b0_setDepthRenderTarget, 0x10739b0, void,     void* gnmx_gfxc, const void* dsv)
DEFINE_EBOOT_TRAMPOLINE(sub_1073840_setCmaskClearColor,   0x1073840, void,     void* gnmx_gfxc, uint32_t slot, const uint32_t* clear_dwords)
DEFINE_EBOOT_TRAMPOLINE(sub_1073590_setDepthClearValue,   0x1073590, void,     void* gnmx_gfxc, float depth_clear)
DEFINE_EBOOT_TRAMPOLINE(sub_10735f0_setAaSampleMask,      0x10735f0, void,     void* gnmx_gfxc, uint32_t sample_mask)
DEFINE_EBOOT_TRAMPOLINE(sub_1076ab0_setCbControl,         0x1076ab0, void,     void* gnmx_gfxc, uint32_t mode, uint32_t rop3)
DEFINE_EBOOT_TRAMPOLINE(sub_10868c0_getRtWidthHeight,     0x10868c0, uint32_t, const void* gnm_rt)
DEFINE_EBOOT_TRAMPOLINE(sub_10726c0_getDtNumSamples,      0x10726c0, uint32_t, const void* gnm_dt)
DEFINE_EBOOT_TRAMPOLINE(sub_1096bd0_setScreenScissor,     0x1096bd0, void,     void* gnmx_gfxc, uint32_t wh_packed)
DEFINE_EBOOT_TRAMPOLINE(sub_26b7a90_mapSampleCount,       0x26b7a90, uint32_t, uint32_t aa_samples)
DEFINE_EBOOT_TRAMPOLINE(sub_1085510_scanModeInit,         0x1085510, void,     uint32_t* scan_mode)
DEFINE_EBOOT_TRAMPOLINE(sub_1085520_scanModeSetSamples,   0x1085520, void,     uint32_t* scan_mode, uint32_t log2_samples)
DEFINE_EBOOT_TRAMPOLINE(sub_10855e0_scanModeSetGrids,     0x10855e0, void,     uint32_t* scan_mode, uint32_t enable)
DEFINE_EBOOT_TRAMPOLINE(sub_1085560_scanModeSetWh,        0x1085560, void,     uint32_t* scan_mode, uint32_t wh_packed)
DEFINE_EBOOT_TRAMPOLINE(sub_1073460_setScanModeControl,   0x1073460, void,     void* gnmx_gfxc, uint32_t scan_mode)
DEFINE_EBOOT_TRAMPOLINE(sub_2164ed0_getFormatBytesPerElem,0x2164ed0, uint64_t, uint32_t data_format)
DEFINE_EBOOT_TRAMPOLINE(sub_2165130_calcMipCount,         0x2165130, uint32_t, uint32_t width, uint32_t height, uint32_t depth)
DEFINE_EBOOT_TRAMPOLINE(sub_216e1d0_initRtFromTexMip,     0x216e1d0, int32_t,  void* out_rt, const void* tex, uint32_t fmt, uint32_t mip, uint32_t base_slice, uint32_t num_slices)
DEFINE_EBOOT_TRAMPOLINE(sub_216fa60_initDtFromTexMip,     0x216fa60, int32_t,  void* out_dt, const void* tex, uint32_t dsv_fmt, uint32_t mip, uint32_t base_slice, uint32_t num_slices)
DEFINE_EBOOT_TRAMPOLINE(sub_26b5a70_decompressRt,         0x26b5a70, void,     void* gnmx_ctx_wrapper, const void* gnm_rt, const void* gnm_tex, uint32_t slice_start, uint32_t slice_count, uint32_t flags)
DEFINE_EBOOT_TRAMPOLINE(sub_26b6810_decompressDt,         0x26b6810, void,     void* gnmx_ctx_wrapper, const void* gnm_dt, const void* gnm_tex, uint32_t slice_start, uint32_t slice_count, uint32_t flags)
DEFINE_EBOOT_TRAMPOLINE(sub_1085600_oxControlInit,        0x1085600, void,     uint32_t* ox_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1085610_oxControlSetAlphaCov, 0x1085610, void,     uint32_t* ox_ctrl, uint32_t enable)
DEFINE_EBOOT_TRAMPOLINE(sub_1073030_setAlphaToMaskControl,0x1073030, void,     void* gnmx_gfxc, uint32_t ox_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1072fd0_setRenderTargetMask,  0x1072fd0, void,     void* gnmx_gfxc, uint32_t rt_mask)
DEFINE_EBOOT_TRAMPOLINE(sub_1084ea0_blendControlInit,     0x1084ea0, void,     uint32_t* bc_dummy)
DEFINE_EBOOT_TRAMPOLINE(sub_1073090_setBlendControl,      0x1073090, void,     void* gnmx_gfxc, uint32_t slot, uint32_t bc)
DEFINE_EBOOT_TRAMPOLINE(sub_1073100_setBlendColor,        0x1073100, void,     void* gnmx_gfxc, float r, float g, float b, float a)
DEFINE_EBOOT_TRAMPOLINE(sub_10733a0_setDepthControl,      0x10733a0, void,     void* gnmx_gfxc, uint32_t depth_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1073400_setStencil,           0x1073400, void,     void* gnmx_gfxc, uint32_t stencil_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1073190_setStencilSeparate,   0x1073190, void,     void* gnmx_gfxc, uint32_t stencil_op_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1073520_setDepthBoundsRange,  0x1073520, void,     void* gnmx_gfxc, float zmin, float zmax)
DEFINE_EBOOT_TRAMPOLINE(sub_1076b30_setPrimitiveSetup,    0x1076b30, void,     void* gnmx_gfxc, uint32_t prim_setup)
DEFINE_EBOOT_TRAMPOLINE(sub_1076e00_setClipControl,       0x1076e00, void,     void* gnmx_gfxc, uint32_t clip_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1077070_setPolygonOffsetFront,0x1077070, void,     void* gnmx_gfxc, float offset, float scale)
DEFINE_EBOOT_TRAMPOLINE(sub_10770e0_setPolygonOffsetBack, 0x10770e0, void,     void* gnmx_gfxc, float offset, float scale)
DEFINE_EBOOT_TRAMPOLINE(sub_1076fa0_setPolygonOffsetClamp,0x1076fa0, void,     void* gnmx_gfxc, float clamp)
DEFINE_EBOOT_TRAMPOLINE(sub_1077000_setPolygonOffsetZead, 0x1077000, void,     void* gnmx_gfxc, uint32_t z_fmt)
DEFINE_EBOOT_TRAMPOLINE(sub_1077150_setAaScissorEnable,   0x1077150, void,     void* gnmx_gfxc, uint32_t msaa_en, uint32_t scissor_en)
DEFINE_EBOOT_TRAMPOLINE(sub_10765e0_setViewport,          0x10765e0, void,     void* gnmx_gfxc, uint32_t vp_id, float dmin, float dmax, const float* scale, const float* offset)
DEFINE_EBOOT_TRAMPOLINE(sub_1085210_vpControlInit,        0x1085210, void,     uint32_t* vp_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1085220_vpControlSet,         0x1085220, void,     uint32_t* vp_ctrl, uint32_t flags)
DEFINE_EBOOT_TRAMPOLINE(sub_1076980_setViewportTransform, 0x1076980, void,     void* gnmx_gfxc, uint32_t vp_ctrl)
DEFINE_EBOOT_TRAMPOLINE(sub_1076790_setGenericScissor,    0x1076790, void,     void* gnmx_gfxc, int32_t left, int32_t top, int32_t right, int32_t bottom)
DEFINE_EBOOT_TRAMPOLINE(sub_1073650_setHardwareScreenOff, 0x1073650, void,     void* gnmx_gfxc, int32_t off_x, int32_t off_y)
DEFINE_EBOOT_TRAMPOLINE(sub_10736d0_setGuardBandClip,     0x10736d0, void,     void* gnmx_gfxc, float clip_x, float clip_y)
DEFINE_EBOOT_TRAMPOLINE(sub_1073750_setGuardBandDiscard,  0x1073750, void,     void* gnmx_gfxc, float disc_x, float disc_y)
DEFINE_EBOOT_TRAMPOLINE(sub_10766d0_setViewportScissor,   0x10766d0, void,     void* gnmx_gfxc, uint32_t vp_id, int32_t left, int32_t top, int32_t right, int32_t bottom, uint32_t win_off_disable)
DEFINE_EBOOT_TRAMPOLINE(sub_216e1b0_getBufferBaseAddr,    0x216e1b0, uint64_t, void* buffer_res)

#undef DEFINE_EBOOT_TRAMPOLINE
} // extern "C"

// ============================================================================
// 1. Hardware Rasterizer / Viewport / Scissor State Cache (0x26d3300..0x26d3370)
// ============================================================================

struct CGRasterizerStateObject {
    uint32_t primitive_setup;       // +0x00 (sce::Gnm::PrimitiveSetup)
    uint32_t clip_control;          // +0x04 (sce::Gnm::ClipControl)
    float    slope_scale_depth_bias;// +0x08
    float    depth_bias_clamp;      // +0x0c
    float    depth_bias;            // +0x10
    uint8_t  msaa_enable;           // +0x14
    uint8_t  scissor_enable;        // +0x15
    uint16_t pad16;                 // +0x16
};

struct CGRasterizerCacheGnm {
    uint32_t primitive_setup;       // +0x00
    uint32_t clip_control;          // +0x04
    float    depth_bias;            // +0x08
    float    slope_scale_depth_bias;// +0x0c
    float    depth_bias_clamp;      // +0x10
    uint32_t z_format;              // +0x14 (0, 1=R32_FLOAT, 3=UNORM)
    uint8_t  msaa_enable;           // +0x18
    uint8_t  scissor_enable;        // +0x19
    uint16_t pad1a;                 // +0x1a
    uint32_t pad1c;                 // +0x1c
    const CGRasterizerStateObject* bound_rs; // +0x20
    uint32_t dirty_viewport_mask;   // +0x28
    uint32_t dirty_scissor_mask;    // +0x2c
};
static_assert(sizeof(CGRasterizerCacheGnm) == 0x30, "Exact 48-byte CGRasterizerCacheGnm");

static inline bool FloatEqualNotNan(float a, float b) {
    return !std::isunordered(a, b) && (a == b);
}

// VMA: 0x26d3370 (1526 B)
// Flushes dirty rasterizer state, viewport 0 (plus guardband clip/discard and hardware screen offset),
// scissor 0, and viewports/scissors 1..15 (when a multi-viewport Geometry Shader is bound at +0x1970).
void CGRasterizerCacheGnm_FlushToGfxContext(
    CGRasterizerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* pipeline_state)
{
    void* gfxc = gnmx_ctx_wrapper + 0x10;
    const bool direct_rs = gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_RasterState);
    gx_direct_vk::ContextRegBatch batch{};

    const auto* rs = *reinterpret_cast<const CGRasterizerStateObject* const*>(pipeline_state + 0x240);
    if (rs != self->bound_rs) {
        const auto* eff_rs = rs ? rs : reinterpret_cast<const CGRasterizerStateObject*>(g_eboot_live_slide + 0x546ca58ULL);
        if (eff_rs->primitive_setup != self->primitive_setup) {
            if (direct_rs) {
                batch.SetPrimitiveSetup(gfxc, eff_rs->primitive_setup);
            } else {
                sub_1076b30_setPrimitiveSetup(gfxc, eff_rs->primitive_setup);
            }
            self->primitive_setup = eff_rs->primitive_setup;
        }
        if (eff_rs->clip_control != self->clip_control) {
            if (direct_rs) {
                batch.SetClipControl(gfxc, eff_rs->clip_control);
            } else {
                sub_1076e00_setClipControl(gfxc, eff_rs->clip_control);
            }
            self->clip_control = eff_rs->clip_control;
        }
        float slope_bias = eff_rs->slope_scale_depth_bias;
        float const_bias = eff_rs->depth_bias;
        if (!FloatEqualNotNan(const_bias, self->depth_bias) ||
            !FloatEqualNotNan(slope_bias, self->slope_scale_depth_bias)) {
            float scaled_bias = const_bias * 16.0f;
            if (direct_rs) {
                batch.SetPolygonOffsetFrontBack(gfxc, scaled_bias, slope_bias);
            } else {
                sub_1077070_setPolygonOffsetFront(gfxc, scaled_bias, slope_bias);
                sub_10770e0_setPolygonOffsetBack(gfxc, scaled_bias, slope_bias);
            }
            self->depth_bias = const_bias;
            self->slope_scale_depth_bias = slope_bias;
        }
        float clamp_val = eff_rs->depth_bias_clamp;
        if (!FloatEqualNotNan(clamp_val, self->depth_bias_clamp)) {
            if (direct_rs) {
                batch.SetPolygonOffsetClamp(gfxc, clamp_val);
            } else {
                sub_1076fa0_setPolygonOffsetClamp(gfxc, clamp_val);
            }
            self->depth_bias_clamp = clamp_val;
        }
        uint32_t cur_z_fmt = self->z_format;
        uint32_t new_z_fmt = cur_z_fmt;
        const uint8_t* dsv = *reinterpret_cast<const uint8_t* const*>(pipeline_state + 0x520);
        if (dsv != nullptr) {
            new_z_fmt = (*reinterpret_cast<const uint32_t*>(dsv + 0x44) == 0x37u) ? 1u : 3u;
        }
        if (new_z_fmt != cur_z_fmt) {
            if (direct_rs) {
                batch.SetPolygonOffsetZFormat(gfxc, new_z_fmt);
            } else {
                sub_1077000_setPolygonOffsetZead(gfxc, new_z_fmt);
            }
            self->z_format = new_z_fmt;
        }
        uint32_t rs_flags_dword = *reinterpret_cast<const uint32_t*>(&eff_rs->msaa_enable);
        uint32_t cur_flags_dword = *reinterpret_cast<const uint32_t*>(&self->msaa_enable);
        if (rs_flags_dword != cur_flags_dword) {
            if (direct_rs) {
                batch.SetAaScissorEnable(gfxc, eff_rs->msaa_enable, eff_rs->scissor_enable);
            } else {
                sub_1077150_setAaScissorEnable(gfxc, eff_rs->msaa_enable, eff_rs->scissor_enable);
            }
            *reinterpret_cast<uint32_t*>(&self->msaa_enable) = *reinterpret_cast<const uint32_t*>(&eff_rs->msaa_enable);
        }
        self->bound_rs = rs;
    }

    if (self->dirty_viewport_mask & 1u) {
        float vp_x    = *reinterpret_cast<const float*>(pipeline_state + 0x248);
        float vp_y    = *reinterpret_cast<const float*>(pipeline_state + 0x24c);
        float vp_x1   = vp_x + *reinterpret_cast<const float*>(pipeline_state + 0x250);
        float vp_y1   = vp_y + *reinterpret_cast<const float*>(pipeline_state + 0x254);
        float vp_zmax = *reinterpret_cast<const float*>(pipeline_state + 0x25c);
        float vp_zmin = *reinterpret_cast<const float*>(pipeline_state + 0x258);

        float dx = vp_x1 - vp_x;
        float dy = vp_y1 - vp_y;
        float scale[3];
        scale[0] = dx * 0.5f;
        scale[1] = dy * -0.5f;
        scale[2] = vp_zmax - vp_zmin;

        float offset[3];
        offset[0] = vp_x + scale[0];
        offset[1] = vp_y + (dy * 0.5f);
        offset[2] = 0.0f;

        int32_t hw_off_x = static_cast<int32_t>(std::floor(offset[0] * 0.0625f + 0.5f));
        int32_t hw_off_y = static_cast<int32_t>(std::floor(offset[1] * 0.0625f + 0.5f));

        float s0 = scale[0];
        float s1 = scale[1];
        float o0 = offset[0];
        float o1 = offset[1];

        if (direct_rs) {
            batch.SetViewport(gfxc, 0u, 0.0f, 1.0f, scale, offset);
            batch.SetViewportTransform(gfxc, 0x0000043Fu);
            batch.SetGenericScissor(
                gfxc,
                static_cast<int32_t>(vp_x),
                static_cast<int32_t>(vp_y),
                static_cast<int32_t>(vp_x1),
                static_cast<int32_t>(vp_y1));
            batch.SetHardwareScreenOffset(gfxc, hw_off_x, hw_off_y);

            float abs_s0 = std::fabs(s0);
            float f_off_x = static_cast<float>(hw_off_x << 4);
            float clip_x0 = ((o0 - abs_s0) - f_off_x) + 32768.0f;
            float clip_x1 = ((32767.99609375f - abs_s0) - o0) + f_off_x;
            float clip_x  = std::min(clip_x0, clip_x1) / abs_s0;

            float abs_s1 = std::fabs(s1);
            float f_off_y = static_cast<float>(hw_off_y << 4);
            float clip_y0 = ((o1 - abs_s1) - f_off_y) + 32768.0f;
            float clip_y1 = ((32767.99609375f - abs_s1) - o1) + f_off_y;
            float clip_y  = std::min(clip_y0, clip_y1) / abs_s1;

            batch.SetGuardBands(gfxc, clip_x, clip_y, 1.0f, 1.0f);
        } else {
            sub_10765e0_setViewport(gfxc, 0u, 0.0f, 1.0f, scale, offset);

            uint32_t vp_ctrl = 0;
            sub_1085210_vpControlInit(&vp_ctrl);
            sub_1085220_vpControlSet(&vp_ctrl, 0u);
            sub_1076980_setViewportTransform(gfxc, vp_ctrl);

            sub_1076790_setGenericScissor(
                gfxc,
                static_cast<int32_t>(vp_x),
                static_cast<int32_t>(vp_y),
                static_cast<int32_t>(vp_x1),
                static_cast<int32_t>(vp_y1));

            sub_1073650_setHardwareScreenOff(gfxc, hw_off_x, hw_off_y);

            uint64_t rem_dwords = (*reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x18) -
                                   *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x20)) >> 2;
            if (static_cast<uint32_t>(rem_dwords) <= 11u) {
                using CallbackFn = void(*)(void*, uint32_t, void*);
                auto cb = *reinterpret_cast<CallbackFn*>(gnmx_ctx_wrapper + 0x28);
                cb(gfxc, 0u, *reinterpret_cast<void**>(gnmx_ctx_wrapper + 0x30));
                rem_dwords = (*reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x18) -
                              *reinterpret_cast<uint64_t*>(gnmx_ctx_wrapper + 0x20)) >> 2;
            }
            if (static_cast<uint32_t>(rem_dwords) >= 12u) {
                float abs_s0 = std::fabs(s0);
                float f_off_x = static_cast<float>(hw_off_x << 4);
                float clip_x0 = ((o0 - abs_s0) - f_off_x) + 32768.0f;
                float clip_x1 = ((32767.99609375f - abs_s0) - o0) + f_off_x;
                float clip_x  = std::min(clip_x0, clip_x1) / abs_s0;

                float abs_s1 = std::fabs(s1);
                float f_off_y = static_cast<float>(hw_off_y << 4);
                float clip_y0 = ((o1 - abs_s1) - f_off_y) + 32768.0f;
                float clip_y1 = ((32767.99609375f - abs_s1) - o1) + f_off_y;
                float clip_y  = std::min(clip_y0, clip_y1) / abs_s1;

                sub_10736d0_setGuardBandClip(gfxc, clip_x, clip_y);
                sub_1073750_setGuardBandDiscard(gfxc, 1.0f, 1.0f);
            }
        }
        self->dirty_viewport_mask &= ~1u;
    }

    if ((self->dirty_scissor_mask & 1u) && self->scissor_enable != 0) {
        int32_t left   = std::max(0, *reinterpret_cast<const int32_t*>(pipeline_state + 0x3c8));
        int32_t top    = std::max(0, *reinterpret_cast<const int32_t*>(pipeline_state + 0x3cc));
        int32_t right  = std::max(0, *reinterpret_cast<const int32_t*>(pipeline_state + 0x3d0));
        int32_t bottom = std::max(0, *reinterpret_cast<const int32_t*>(pipeline_state + 0x3d4));
        if (direct_rs) {
            batch.SetViewportScissor(gfxc, 0u, left, top, right, bottom, 0u);
        } else {
            sub_10766d0_setViewportScissor(gfxc, 0u, left, top, right, bottom, 0u);
        }
        self->dirty_scissor_mask &= ~1u;
    }

    if (*reinterpret_cast<const void* const*>(pipeline_state + 0x1970) != nullptr) {
        uint32_t inv_vp = ~self->dirty_viewport_mask & 0xFFFFFFFEu;
        uint32_t vp_limit = inv_vp ? static_cast<uint32_t>(__builtin_ctz(inv_vp)) : 32u;
        if (vp_limit >= 2u) {
            const float* vp_ptr = reinterpret_cast<const float*>(pipeline_state + 0x260);
            for (uint32_t slot = 1u; slot < vp_limit; ++slot, vp_ptr += 6) {
                float scale[3];
                scale[0] = vp_ptr[2] * 0.5f;
                scale[1] = vp_ptr[3] * -0.5f;
                scale[2] = vp_ptr[5] - vp_ptr[4];
                float offset[3];
                offset[0] = vp_ptr[0] + scale[0];
                offset[1] = vp_ptr[1] - scale[1];
                offset[2] = 0.0f;
                if (direct_rs) {
                    batch.SetViewport(gfxc, slot, 0.0f, 1.0f, scale, offset);
                } else {
                    sub_10765e0_setViewport(gfxc, slot, 0.0f, 1.0f, scale, offset);
                }
            }
        }
        self->dirty_viewport_mask = 0;

        uint32_t inv_sc = ~self->dirty_scissor_mask & 0xFFFFFFFEu;
        uint32_t sc_limit = inv_sc ? static_cast<uint32_t>(__builtin_ctz(inv_sc)) : 32u;
        if (sc_limit >= 2u) {
            const int32_t* sc_ptr = reinterpret_cast<const int32_t*>(pipeline_state + 0x3d8);
            for (uint32_t slot = 1u; slot < sc_limit; ++slot, sc_ptr += 4) {
                int32_t left   = std::max(0, sc_ptr[0]);
                int32_t top    = std::max(0, sc_ptr[1]);
                int32_t right  = std::max(0, sc_ptr[2]);
                int32_t bottom = std::max(0, sc_ptr[3]);
                if (direct_rs) {
                    batch.SetViewportScissor(gfxc, slot, left, top, right, bottom, 0u);
                } else {
                    sub_10766d0_setViewportScissor(gfxc, slot, left, top, right, bottom, 0u);
                }
            }
        }
        self->dirty_scissor_mask = 0;
    }

    if (direct_rs) {
        batch.Flush(gfxc);
    }
}

// VMA: 0x26d3360 (16 B)
void CGRasterizerCacheGnm_MarkScissorsDirty(CGRasterizerCacheGnm* self, uint32_t count) {
    self->dirty_scissor_mask |= (1u << count) - 1u;
}

// VMA: 0x26d3350 (16 B)
void CGRasterizerCacheGnm_MarkViewportsDirty(CGRasterizerCacheGnm* self, uint32_t count) {
    self->dirty_viewport_mask |= (1u << count) - 1u;
}

// VMA: 0x26d3300 (76 B)
void CGRasterizerCacheGnm_Reset(CGRasterizerCacheGnm* self) {
    self->primitive_setup        = 0xFFFFFFFFu;
    self->clip_control           = 0xFFFFFFFFu;
    const uint32_t qnan_bits     = 0x7FC00000u;
    std::memcpy(&self->depth_bias, &qnan_bits, sizeof(float));
    std::memcpy(&self->slope_scale_depth_bias, &qnan_bits, sizeof(float));
    std::memcpy(&self->depth_bias_clamp, &qnan_bits, sizeof(float));
    self->z_format               = 0;
    self->msaa_enable            = 0xFFu;
    self->scissor_enable         = 0xFFu;
    self->pad1a                  = 0xFFFFu;
    self->bound_rs               = reinterpret_cast<const CGRasterizerStateObject*>(-1ULL);
    self->dirty_viewport_mask    = 0x0000FFFFu;
    self->dirty_scissor_mask     = 0x0000FFFFu;
}

// ============================================================================
// 2. Output Merger (RTV / DSV / Blend / DepthStencil / CMASK) Cache (0x26d1720..0x26d3070)
// ============================================================================

struct CGBlendStateObject {
    uint32_t render_target_channel_mask; // +0x00
    uint32_t blend_control[8];           // +0x04..+0x23
    uint8_t  alpha_to_coverage_enable;   // +0x24
};

struct CGDepthStencilStateObject {
    uint32_t depth_control;              // +0x00
    uint32_t stencil_control;            // +0x04
    uint32_t stencil_op_control;         // +0x08
};

struct CGOutputMergerCacheGnm {
    uint32_t alpha_to_coverage;          // +0x000
    uint64_t rt_channel_mask;            // +0x008
    uint32_t blend_control[8];           // +0x010..+0x02f
    float    blend_factor[4];            // +0x030..+0x03f
    uint64_t pad040;                     // +0x040
    uint32_t depth_control;              // +0x048
    uint32_t stencil_control;            // +0x04c
    uint32_t stencil_op_control;         // +0x050
    uint32_t pad054;                     // +0x054
    uint64_t depth_bounds_packed;        // +0x058
    void*    rtv_bank[2][9];             // +0x060..+0x0ef ([bank 0..1][0..7 = RTV, 8 = DSV])
    uint32_t width_height_packed;        // +0x0f0
    uint32_t aa_sample_count;            // +0x0f4
    float    pending_depth_clear;        // +0x0f8
    uint16_t pending_sample_mask;        // +0x0fc
    uint8_t  pending_rtv_count;          // +0x0fe
    uint8_t  pending_om_flags;           // +0x0ff
    uint64_t pending_cmask_clear[8];     // +0x100..+0x13f
    float    active_depth_clear;         // +0x140
    uint16_t active_sample_mask;         // +0x144
    uint8_t  active_rtv_count;           // +0x146
    uint8_t  active_om_flags;            // +0x147
    uint64_t active_cmask_clear[8];      // +0x148..+0x187
    uint32_t state_dirty_bits;           // +0x188 (bit0=rtv_bank, bit1=dsv_bank, bit2=depth_clear, bit3=sample_mask, bits4..11=cmask_dirty)
    uint32_t pad18c;                     // +0x18c
    const CGBlendStateObject*        bound_blend_state; // +0x190
    const CGDepthStencilStateObject* bound_ds_state;    // +0x198
};
static_assert(sizeof(CGOutputMergerCacheGnm) == 0x1a0, "Exact 0x1a0 CGOutputMergerCacheGnm");

void CGOutputMergerCacheGnm_FlushCmaskClearColors(
    CGOutputMergerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper);

// VMA: 0x26d3070 (384 B)
// Checks RTVs and DSV being unbound against `new_targets`, and triggers CMASK/FMASK/HTILE decompression.
void CGOutputMergerCacheGnm_DecompressUnboundTargets(
    CGOutputMergerCacheGnm* self,
    void* gnmx_ctx_wrapper,
    void* const* new_targets)
{
    for (uint32_t slot = 0; slot < 8u; ++slot) {
        uint32_t rtv_bank = self->state_dirty_bits & 1u;
        const uint8_t* rtv = reinterpret_cast<const uint8_t*>(self->rtv_bank[rtv_bank][slot]);
        if (rtv != reinterpret_cast<const uint8_t*>(new_targets[slot]) &&
            reinterpret_cast<uintptr_t>(rtv) + 1ULL >= 2ULL) {
            const uint8_t* tex_view = *reinterpret_cast<const uint8_t* const*>(rtv + 0x08);
            if (tex_view != nullptr && (tex_view[0x3b] & 0x8Bu) != 0) {
                uint32_t view_type = *reinterpret_cast<const uint32_t*>(rtv + 0x40);
                if (view_type - 1u <= 7u) {
                    uint32_t slice_start = (view_type == 1u) ? 0u : *reinterpret_cast<const uint32_t*>(rtv + 0x48);
                    uint32_t slice_count = (view_type == 1u) ? 0u : *reinterpret_cast<const uint32_t*>(rtv + 0x4c);
                    CGOutputMergerCacheGnm_FlushCmaskClearColors(self, static_cast<uint8_t*>(gnmx_ctx_wrapper));
                    sub_26b5a70_decompressRt(gnmx_ctx_wrapper, rtv + 0x10, tex_view + 0x18, slice_start, slice_count, 0u);
                }
            }
        }
    }

    uint32_t dsv_bank = (self->state_dirty_bits >> 1) & 1u;
    const uint8_t* dsv = reinterpret_cast<const uint8_t*>(self->rtv_bank[dsv_bank][8]);
    if (dsv != reinterpret_cast<const uint8_t*>(new_targets[8]) &&
        reinterpret_cast<uintptr_t>(dsv) + 1ULL >= 2ULL) {
        const uint8_t* tex_view = *reinterpret_cast<const uint8_t* const*>(dsv + 0x08);
        if (tex_view != nullptr &&
            (tex_view[0x3b] & 0x08u) != 0 &&
            *reinterpret_cast<const uint32_t*>(dsv + 0x5c) == 0u &&
            *reinterpret_cast<const uint32_t*>(dsv + 0x48) - 1u <= 5u) {
            sub_26b6810_decompressDt(
                gnmx_ctx_wrapper,
                dsv + 0x10,
                tex_view + 0x18,
                *reinterpret_cast<const uint32_t*>(dsv + 0x54),
                *reinterpret_cast<const uint32_t*>(dsv + 0x58),
                0u);
        }
    }
}

// VMA: 0x26d2f30 (308 B)
// Copies out the currently active RTVs[0..7] and DSV, then invalidates both banks to -1.
void CGOutputMergerCacheGnm_SnapshotAndInvalidateTargets(
    CGOutputMergerCacheGnm* self,
    void** out_bank0_copy,
    void** out_active_targets)
{
    std::memcpy(out_bank0_copy, self->rtv_bank[0], 9 * sizeof(void*));
    uint32_t rtv_bank_idx = self->state_dirty_bits & 1u;
    std::memcpy(out_active_targets, self->rtv_bank[rtv_bank_idx], 8 * sizeof(void*));
    uint32_t dsv_bank_idx = (self->state_dirty_bits >> 1) & 1u;
    out_active_targets[8] = self->rtv_bank[dsv_bank_idx][8];

    std::memset(self->rtv_bank, 0xFF, sizeof(self->rtv_bank));
    self->state_dirty_bits &= ~3u;
}

// VMA: 0x26d2d20 (517 B)
// Resets OM state cache while preserving active CMASK clear values and bank bits.
void CGOutputMergerCacheGnm_Reset(CGOutputMergerCacheGnm* self) {
    self->alpha_to_coverage = 0xFFFFFFFFu;
    self->rt_channel_mask   = ~0ULL;
    for (int i = 0; i < 8; ++i) {
        self->blend_control[i]      = 0xFFFFFFFFu;
        self->rtv_bank[0][i]        = reinterpret_cast<void*>(-1ULL);
        self->rtv_bank[1][i]        = reinterpret_cast<void*>(-1ULL);
        self->active_cmask_clear[i] = 0;
    }
    self->rtv_bank[0][8] = reinterpret_cast<void*>(-1ULL);
    self->rtv_bank[1][8] = reinterpret_cast<void*>(-1ULL);
    const uint32_t qnan_bits = 0x7FC00000u;
    for (int i = 0; i < 4; ++i) {
        std::memcpy(&self->blend_factor[i], &qnan_bits, sizeof(float));
    }
    self->pad040              = 0;
    self->depth_control       = 0xFFFFFFFFu;
    self->stencil_control     = 0xFFFFFFFFu;
    self->stencil_op_control  = 0;
    self->depth_bounds_packed = ~0ULL;
    self->width_height_packed = 0xFFFFFFFFu;
    self->aa_sample_count     = 0xFFFFFFFFu;
    self->bound_blend_state   = reinterpret_cast<const CGBlendStateObject*>(-1ULL);
    self->bound_ds_state      = reinterpret_cast<const CGDepthStencilStateObject*>(-1ULL);
    self->pending_rtv_count   = 0;
    self->pending_om_flags    = 0;
    self->active_depth_clear  = 1.0f;
    self->active_sample_mask  = 0;
    self->active_rtv_count    = 0;
    self->active_om_flags     = 0x1Cu;
    self->state_dirty_bits    = (self->state_dirty_bits & 3u) | 0xFFCu;
}

// VMA: 0x26d2cb0 (107 B)
// Packs a 64-bit fast-clear color into 1 or 2 32-bit dwords according to the DataFormat bit-width.
void CGOutputMergerCacheGnm_FormatCmaskClearColor(
    void* self,
    uint32_t* out_dwords,
    uint64_t packed_rgba16,
    uint32_t data_format)
{
    (void)self;
    uint32_t bits = static_cast<uint32_t>(sub_2164ed0_getFormatBytesPerElem(data_format) << 3);
    if (bits == 64u) {
        out_dwords[0] = static_cast<uint32_t>(packed_rgba16);
        out_dwords[1] = static_cast<uint32_t>(packed_rgba16 >> 32);
    } else if (bits == 32u) {
        uint32_t b0 = (static_cast<uint32_t>(packed_rgba16) >> 8) & 0xFFu;
        uint32_t b1 = (static_cast<uint32_t>(packed_rgba16) >> 16) & 0xFF00u;
        uint32_t b2 = static_cast<uint32_t>(packed_rgba16 >> 24) & 0xFF0000u;
        uint32_t b3 = static_cast<uint32_t>(packed_rgba16 >> 32) & 0xFF000000u;
        out_dwords[0] = b0 | b1 | b2 | b3;
    }
}

// VMA: 0x26d2c30 (120 B)
// Flushes dirty depth clear value (bit 2) and AA sample mask (bit 3) to Gnmx::GfxContext.
void CGOutputMergerCacheGnm_FlushDepthClearAndSampleMask(
    CGOutputMergerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper)
{
    void* gfxc = gnmx_ctx_wrapper + 0x10;
    const bool direct_bds = gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_BlendDsState);
    gx_direct_vk::ContextRegBatch batch{};
    uint32_t dirty = self->state_dirty_bits;
    if (dirty & 4u) {
        if (direct_bds) {
            batch.SetDepthClearValue(gfxc, self->pending_depth_clear);
        } else {
            sub_1073590_setDepthClearValue(gfxc, self->pending_depth_clear);
        }
        self->active_depth_clear = self->pending_depth_clear;
        dirty = self->state_dirty_bits & ~4u;
        self->state_dirty_bits = dirty;
    }
    if (dirty & 8u) {
        if (direct_bds) {
            batch.SetStencilClearValue(gfxc, self->pending_sample_mask);
        } else {
            sub_10735f0_setAaSampleMask(gfxc, self->pending_sample_mask);
        }
        self->active_sample_mask = self->pending_sample_mask;
        self->state_dirty_bits &= ~8u;
    }
    if (direct_bds) {
        batch.Flush(gfxc);
    }
}

// VMA: 0x26d1f80 (386 B)
// Flushes dirty CMASK fast-clear colors (bits 4..11) for bound RTVs[0..active_rtv_count-1].
void CGOutputMergerCacheGnm_FlushCmaskClearColors(
    CGOutputMergerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper)
{
    uint8_t count = self->active_rtv_count;
    if (count == 0) {
        return;
    }
    void* gfxc = gnmx_ctx_wrapper + 0x10;
    const bool direct_bds = gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_BlendDsState);
    gx_direct_vk::ContextRegBatch batch{};
    uint32_t dirty = self->state_dirty_bits;
    for (uint32_t slot = 0; slot < count; ++slot) {
        uint32_t rtv_bank = dirty & 1u;
        const uint8_t* rtv = reinterpret_cast<const uint8_t*>(self->rtv_bank[rtv_bank][slot]);
        if (reinterpret_cast<uintptr_t>(rtv) + 1ULL >= 2ULL) {
            uint32_t mask = 1u << slot;
            if (((dirty >> 4) & mask) != 0) {
                uint32_t clear_dwords[2] = { 0u, 0u };
                uint64_t packed = self->pending_cmask_clear[slot];
                uint32_t fmt = *reinterpret_cast<const uint32_t*>(rtv + 0x3c);
                uint32_t bits = static_cast<uint32_t>(sub_2164ed0_getFormatBytesPerElem(fmt) << 3);
                if (bits == 64u) {
                    clear_dwords[0] = static_cast<uint32_t>(packed);
                    clear_dwords[1] = static_cast<uint32_t>(packed >> 32);
                } else if (bits == 32u) {
                    uint32_t b0 = (static_cast<uint32_t>(packed) >> 8) & 0xFFu;
                    uint32_t b1 = (static_cast<uint32_t>(packed) >> 16) & 0xFF00u;
                    uint32_t b2 = static_cast<uint32_t>(packed >> 24) & 0xFF0000u;
                    uint32_t b3 = static_cast<uint32_t>(packed >> 32) & 0xFF000000u;
                    clear_dwords[0] = b0 | b1 | b2 | b3;
                }
                if (direct_bds) {
                    batch.SetCmaskClearColor(gfxc, slot, clear_dwords);
                } else {
                    sub_1073840_setCmaskClearColor(gfxc, slot, clear_dwords);
                }
                self->active_cmask_clear[slot] = self->pending_cmask_clear[slot];
                dirty = self->state_dirty_bits & ~((mask << 4) & 0xFF0u);
                self->state_dirty_bits = dirty;
                count = self->active_rtv_count;
            }
        }
    }
    if (direct_bds) {
        batch.Flush(gfxc);
    }
}

// VMA: 0x26d2790 (1171 B)
// Binds RTVs[0..7] and DSV to Gnmx::GfxContext, updates screen scissor & AA sample config,
// flushes CMASK clear colors, depth clear value, sample mask, and CB shader mask.
void CGOutputMergerCacheGnm_BindRenderTargetsAndDepth(
    CGOutputMergerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* pipeline_state)
{
    void* gfxc = gnmx_ctx_wrapper + 0x10;
    const bool direct_rt  = gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_RenderTargets);
    const bool direct_bds = gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_BlendDsState);

    uint32_t wh_packed  = self->width_height_packed;
    uint32_t aa_samples = self->aa_sample_count;
    uint32_t rtv_bank   = self->state_dirty_bits & 1u;

    uint8_t act_flags  = self->active_om_flags;
    uint8_t pend_flags = self->pending_om_flags;
    if (((pend_flags ^ act_flags) & 1u) != 0) {
        uint32_t max_rtvs = std::max<uint32_t>(self->active_rtv_count, self->pending_rtv_count);
        if (pend_flags & 1u) {
            for (uint32_t slot = 0; slot < max_rtvs; ++slot) {
                if (direct_rt) {
                    gx_direct_vk::EmitSetRenderTarget(gfxc, slot, nullptr);
                } else {
                    sub_10738b0_setRenderTarget(gfxc, slot, nullptr);
                }
            }
        } else {
            for (int i = 0; i < 8; ++i) {
                self->rtv_bank[rtv_bank][i] = nullptr;
            }
            self->pending_rtv_count = static_cast<uint8_t>(max_rtvs);
        }
        act_flags = (self->active_om_flags & ~1u) | (self->pending_om_flags & 1u);
        self->active_om_flags = act_flags;
    }

    if ((act_flags & 1u) == 0) {
        uint8_t pend_count = self->pending_rtv_count;
        if (pend_count != 0) {
            for (uint32_t slot = 0; slot < pend_count; ++slot) {
                void* new_rtv = *reinterpret_cast<void* const*>(pipeline_state + 0x4e0 + slot * 8);
                if (self->rtv_bank[rtv_bank][slot] != new_rtv) {
                    if (new_rtv == nullptr) {
                        if (direct_rt) {
                            gx_direct_vk::EmitSetRenderTarget(gfxc, slot, nullptr);
                        } else {
                            sub_10738b0_setRenderTarget(gfxc, slot, nullptr);
                        }
                    } else {
                        const void* gnm_rt = static_cast<const uint8_t*>(new_rtv) + 0x10;
                        if (direct_rt) {
                            gx_direct_vk::EmitSetRenderTarget(gfxc, slot, gnm_rt);
                            wh_packed = gx_direct_vk::GnmRt_GetWidthHeightSampleCode(gnm_rt);
                        } else {
                            sub_10738b0_setRenderTarget(gfxc, slot, gnm_rt);
                            wh_packed = sub_10868c0_getRtWidthHeight(gnm_rt);
                        }
                    }
                    self->rtv_bank[rtv_bank][slot] = *reinterpret_cast<void* const*>(pipeline_state + 0x4e0 + slot * 8);
                    pend_count = self->pending_rtv_count;
                }
            }
            self->active_rtv_count  = pend_count;
            self->pending_rtv_count = 0;
        }
        if ((self->state_dirty_bits & 1u) == 0) {
            std::memcpy(self->rtv_bank[1], self->rtv_bank[0], 8 * sizeof(void*));
            self->state_dirty_bits |= 1u;
        }
    }

    uint32_t dsv_bank = (self->state_dirty_bits >> 1) & 1u;
    void* cached_dsv = self->rtv_bank[dsv_bank][8];
    uint8_t pend_dsv_bit = (self->pending_om_flags >> 1) & 1u;
    uint8_t act_dsv_bit  = (self->active_om_flags >> 1) & 1u;
    if (act_dsv_bit != pend_dsv_bit) {
        if (pend_dsv_bit != 0) {
            if (direct_rt) {
                gx_direct_vk::EmitSetDepthRenderTarget(gfxc, nullptr);
            } else {
                sub_10739b0_setDepthRenderTarget(gfxc, nullptr);
            }
        } else {
            self->rtv_bank[dsv_bank][8] = nullptr;
        }
        self->active_om_flags = (self->active_om_flags & ~2u) | (self->pending_om_flags & 2u);
    }

    if ((self->active_om_flags & 2u) == 0) {
        void* new_dsv = *reinterpret_cast<void* const*>(pipeline_state + 0x520);
        if (cached_dsv != new_dsv) {
            if (new_dsv != nullptr) {
                const void* gnm_dt = static_cast<const uint8_t*>(new_dsv) + 0x10;
                if (direct_rt) {
                    gx_direct_vk::EmitSetDepthRenderTarget(gfxc, gnm_dt);
                    aa_samples = gx_direct_vk::GnmDt_GetNumSamplesLog2(gnm_dt);
                } else {
                    sub_10739b0_setDepthRenderTarget(gfxc, gnm_dt);
                    aa_samples = sub_10726c0_getDtNumSamples(gnm_dt);
                }
            } else {
                if (direct_rt) {
                    gx_direct_vk::EmitSetDepthRenderTarget(gfxc, nullptr);
                } else {
                    sub_10739b0_setDepthRenderTarget(gfxc, nullptr);
                }
            }
            uint32_t db = (self->state_dirty_bits >> 1) & 1u;
            self->rtv_bank[db][8] = new_dsv;
        }
        if ((self->state_dirty_bits & 2u) == 0) {
            self->rtv_bank[1][8] = self->rtv_bank[0][8];
            self->state_dirty_bits |= 2u;
        }
    }

    if (wh_packed != self->width_height_packed) {
        sub_1096bd0_setScreenScissor(gfxc, wh_packed);
        self->width_height_packed = wh_packed;
    }

    gx_direct_vk::ContextRegBatch tail_batch{};
    if (aa_samples != self->aa_sample_count) {
        uint32_t log2_samples = sub_26b7a90_mapSampleCount(aa_samples);
        if (direct_bds) {
            tail_batch.SetScanModeControl(gfxc, log2_samples, self->width_height_packed);
        } else {
            uint32_t scan_mode = 0;
            sub_1085510_scanModeInit(&scan_mode);
            sub_1085520_scanModeSetSamples(&scan_mode, log2_samples);
            sub_10855e0_scanModeSetGrids(&scan_mode, 1u);
            sub_1085560_scanModeSetWh(&scan_mode, self->width_height_packed);
            sub_1073460_setScanModeControl(gfxc, scan_mode);
        }
        self->aa_sample_count = aa_samples;
    }

    if (direct_bds) {
        uint8_t count = self->active_rtv_count;
        uint32_t dirty_cm = self->state_dirty_bits;
        for (uint32_t slot = 0; slot < count; ++slot) {
            uint32_t bank = dirty_cm & 1u;
            const uint8_t* rtv = reinterpret_cast<const uint8_t*>(self->rtv_bank[bank][slot]);
            if (reinterpret_cast<uintptr_t>(rtv) + 1ULL >= 2ULL) {
                uint32_t mask = 1u << slot;
                if (((dirty_cm >> 4) & mask) != 0) {
                    uint32_t clear_dwords[2] = { 0u, 0u };
                    uint64_t packed = self->pending_cmask_clear[slot];
                    uint32_t fmt = *reinterpret_cast<const uint32_t*>(rtv + 0x3c);
                    uint32_t bits = static_cast<uint32_t>(sub_2164ed0_getFormatBytesPerElem(fmt) << 3);
                    if (bits == 64u) {
                        clear_dwords[0] = static_cast<uint32_t>(packed);
                        clear_dwords[1] = static_cast<uint32_t>(packed >> 32);
                    } else if (bits == 32u) {
                        uint32_t b0 = (static_cast<uint32_t>(packed) >> 8) & 0xFFu;
                        uint32_t b1 = (static_cast<uint32_t>(packed) >> 16) & 0xFF00u;
                        uint32_t b2 = static_cast<uint32_t>(packed >> 24) & 0xFF0000u;
                        uint32_t b3 = static_cast<uint32_t>(packed >> 32) & 0xFF000000u;
                        clear_dwords[0] = b0 | b1 | b2 | b3;
                    }
                    tail_batch.SetCmaskClearColor(gfxc, slot, clear_dwords);
                    self->active_cmask_clear[slot] = self->pending_cmask_clear[slot];
                    dirty_cm = self->state_dirty_bits & ~((mask << 4) & 0xFF0u);
                    self->state_dirty_bits = dirty_cm;
                    count = self->active_rtv_count;
                }
            }
        }
    } else {
        CGOutputMergerCacheGnm_FlushCmaskClearColors(self, gnmx_ctx_wrapper);
    }

    uint32_t dirty = self->state_dirty_bits;
    if (dirty & 4u) {
        if (direct_bds) {
            tail_batch.SetDepthClearValue(gfxc, self->pending_depth_clear);
        } else {
            sub_1073590_setDepthClearValue(gfxc, self->pending_depth_clear);
        }
        self->active_depth_clear = self->pending_depth_clear;
        dirty = self->state_dirty_bits & ~4u;
        self->state_dirty_bits = dirty;
    }
    if (dirty & 8u) {
        if (direct_bds) {
            tail_batch.SetStencilClearValue(gfxc, self->pending_sample_mask);
        } else {
            sub_10735f0_setAaSampleMask(gfxc, self->pending_sample_mask);
        }
        self->active_sample_mask = self->pending_sample_mask;
        self->state_dirty_bits &= ~8u;
    }

    if (((self->pending_om_flags ^ self->active_om_flags) & 0x1Cu) != 0) {
        if (direct_bds) {
            tail_batch.SetDbRenderOverrideControl(gfxc, (self->pending_om_flags >> 2) & 1u, 0u);
        } else {
            sub_1076ab0_setCbControl(gfxc, (self->pending_om_flags >> 2) & 1u, 0u);
        }
        self->active_om_flags = (self->active_om_flags & ~0x1Cu) | (self->pending_om_flags & 0x1Cu);
    }

    if (direct_bds) {
        tail_batch.Flush(gfxc);
    }
}

// VMA: 0x26d2230 (1368 B)
// Decompresses replaced RTVs/DSV, then flushes BlendState (+0x4d0), BlendFactor (+0x568),
// DepthStencilState (+0x4d8), StencilRef (+0x584), and DepthBounds (+0x578) to Gnmx::GfxContext.
void CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(
    CGOutputMergerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* pipeline_state)
{
    uint8_t count = self->active_rtv_count;
    for (uint32_t slot = 0; slot < count; ++slot) {
        uint32_t rtv_bank = self->state_dirty_bits & 1u;
        const uint8_t* rtv = reinterpret_cast<const uint8_t*>(self->rtv_bank[rtv_bank][slot]);
        const uint8_t* new_rtv = *reinterpret_cast<const uint8_t* const*>(pipeline_state + 0x4e0 + slot * 8);
        if (rtv != new_rtv && reinterpret_cast<uintptr_t>(rtv) + 1ULL >= 2ULL) {
            const uint8_t* tex_view = *reinterpret_cast<const uint8_t* const*>(rtv + 0x08);
            if (tex_view != nullptr && (tex_view[0x3b] & 0x8Bu) != 0) {
                uint32_t view_type = *reinterpret_cast<const uint32_t*>(rtv + 0x40);
                if (view_type - 1u <= 7u) {
                    uint32_t slice_start = (view_type == 1u) ? 0u : *reinterpret_cast<const uint32_t*>(rtv + 0x48);
                    uint32_t slice_count = (view_type == 1u) ? 0u : *reinterpret_cast<const uint32_t*>(rtv + 0x4c);
                    CGOutputMergerCacheGnm_FlushCmaskClearColors(self, gnmx_ctx_wrapper);
                    sub_26b5a70_decompressRt(gnmx_ctx_wrapper, rtv + 0x10, tex_view + 0x18, slice_start, slice_count, 0u);
                    count = self->active_rtv_count;
                }
            }
        }
    }

    uint32_t dsv_bank = (self->state_dirty_bits >> 1) & 1u;
    const uint8_t* dsv = reinterpret_cast<const uint8_t*>(self->rtv_bank[dsv_bank][8]);
    if (dsv != *reinterpret_cast<const uint8_t* const*>(pipeline_state + 0x520) &&
        reinterpret_cast<uintptr_t>(dsv) + 1ULL >= 2ULL) {
        const uint8_t* tex_view = *reinterpret_cast<const uint8_t* const*>(dsv + 0x08);
        if (tex_view != nullptr &&
            (tex_view[0x3b] & 0x08u) != 0 &&
            *reinterpret_cast<const uint32_t*>(dsv + 0x5c) == 0u &&
            *reinterpret_cast<const uint32_t*>(dsv + 0x48) - 1u <= 5u) {
            sub_26b6810_decompressDt(
                gnmx_ctx_wrapper,
                dsv + 0x10,
                tex_view + 0x18,
                *reinterpret_cast<const uint32_t*>(dsv + 0x54),
                *reinterpret_cast<const uint32_t*>(dsv + 0x58),
                0u);
        }
    }

    void* gfxc = gnmx_ctx_wrapper + 0x10;
    const bool direct_bds = gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_BlendDsState);
    gx_direct_vk::ContextRegBatch batch{};

    const auto* bs = *reinterpret_cast<const CGBlendStateObject* const*>(pipeline_state + 0x4d0);
    if (bs != self->bound_blend_state) {
        const auto* eff_bs = bs ? bs : reinterpret_cast<const CGBlendStateObject*>(g_eboot_live_slide + 0x546c8f4ULL);
        uint32_t a2c = eff_bs->alpha_to_coverage_enable;
        if (a2c != self->alpha_to_coverage) {
            if (direct_bds) {
                batch.SetAlphaToMaskControl(gfxc, a2c);
            } else {
                uint32_t ox_ctrl = 0;
                sub_1085600_oxControlInit(&ox_ctrl);
                sub_1085610_oxControlSetAlphaCov(&ox_ctrl, a2c);
                sub_1073030_setAlphaToMaskControl(gfxc, ox_ctrl);
            }
            self->alpha_to_coverage = a2c;
        }
        uint32_t rt_mask = eff_bs->render_target_channel_mask;
        if (static_cast<uint64_t>(rt_mask) != self->rt_channel_mask) {
            if (direct_bds) {
                batch.SetRenderTargetMask(gfxc, rt_mask);
            } else {
                sub_1072fd0_setRenderTargetMask(gfxc, rt_mask);
            }
            self->bound_blend_state = reinterpret_cast<const CGBlendStateObject*>(-1ULL);
            self->rt_channel_mask = rt_mask;
        }
        if (!direct_bds) {
            uint32_t dummy_bc = 0;
            sub_1084ea0_blendControlInit(&dummy_bc);
        }
        for (uint32_t slot = 0; slot < 8u; ++slot) {
            if (self->blend_control[slot] != eff_bs->blend_control[slot]) {
                if (direct_bds) {
                    batch.SetBlendControl(gfxc, slot, eff_bs->blend_control[slot]);
                } else {
                    sub_1073090_setBlendControl(gfxc, slot, eff_bs->blend_control[slot]);
                }
                self->blend_control[slot] = eff_bs->blend_control[slot];
                self->bound_blend_state = reinterpret_cast<const CGBlendStateObject*>(-1ULL);
            }
        }
        self->bound_blend_state = bs;
    }

    float bf0 = *reinterpret_cast<const float*>(pipeline_state + 0x568);
    float bf1 = *reinterpret_cast<const float*>(pipeline_state + 0x56c);
    float bf2 = *reinterpret_cast<const float*>(pipeline_state + 0x570);
    float bf3 = *reinterpret_cast<const float*>(pipeline_state + 0x574);
    if (!FloatEqualNotNan(bf0, self->blend_factor[0]) ||
        !FloatEqualNotNan(bf1, self->blend_factor[1]) ||
        !FloatEqualNotNan(bf2, self->blend_factor[2]) ||
        !FloatEqualNotNan(bf3, self->blend_factor[3])) {
        if (direct_bds) {
            batch.SetBlendColor(gfxc, bf0, bf1, bf2, bf3);
        } else {
            sub_1073100_setBlendColor(gfxc, bf0, bf1, bf2, bf3);
        }
        self->blend_factor[0] = bf0;
        self->blend_factor[1] = bf1;
        self->blend_factor[2] = bf2;
        self->blend_factor[3] = bf3;
    }

    const uint32_t* stencil_op_src = &self->stencil_op_control;
    const auto* ds = *reinterpret_cast<const CGDepthStencilStateObject* const*>(pipeline_state + 0x4d8);
    if (ds != self->bound_ds_state) {
        const auto* eff_ds = ds ? ds : reinterpret_cast<const CGDepthStencilStateObject*>(g_eboot_live_slide + 0x546ca1cULL);
        if (eff_ds->depth_control != self->depth_control) {
            if (direct_bds) {
                batch.SetDepthControl(gfxc, eff_ds->depth_control);
            } else {
                sub_10733a0_setDepthControl(gfxc, eff_ds->depth_control);
            }
            self->depth_control = eff_ds->depth_control;
            self->bound_ds_state = reinterpret_cast<const CGDepthStencilStateObject*>(-1ULL);
        }
        if (eff_ds->stencil_control != self->stencil_control) {
            if (direct_bds) {
                batch.SetStencil(gfxc, eff_ds->stencil_control);
            } else {
                sub_1073400_setStencil(gfxc, eff_ds->stencil_control);
            }
            self->stencil_control = eff_ds->stencil_control;
            self->bound_ds_state = reinterpret_cast<const CGDepthStencilStateObject*>(-1ULL);
        }
        stencil_op_src = &eff_ds->stencil_op_control;
    }

    uint32_t sref = pipeline_state[0x584];
    uint32_t combined_stencil = ((*stencil_op_src) & 0x00FFFF00u) | sref | (sref << 24);
    if (combined_stencil != self->stencil_op_control) {
        if (direct_bds) {
            batch.SetStencilSeparate(gfxc, combined_stencil);
        } else {
            sub_1073190_setStencilSeparate(gfxc, combined_stencil);
        }
        self->stencil_op_control = combined_stencil;
        self->bound_ds_state = reinterpret_cast<const CGDepthStencilStateObject*>(-1ULL);
    }

    float db_min = *reinterpret_cast<const float*>(pipeline_state + 0x578);
    float db_max = *reinterpret_cast<const float*>(pipeline_state + 0x57c);
    uint32_t db_min_bits = 0, db_max_bits = 0;
    std::memcpy(&db_min_bits, &db_min, sizeof(uint32_t));
    std::memcpy(&db_max_bits, &db_max, sizeof(uint32_t));
    uint64_t db_packed = static_cast<uint64_t>(db_min_bits) | (static_cast<uint64_t>(db_max_bits) << 32);
    if (db_packed != self->depth_bounds_packed) {
        if (direct_bds) {
            batch.SetDepthBoundsRangeBits(gfxc, db_min_bits, db_max_bits);
        } else {
            sub_1073520_setDepthBoundsRange(gfxc, db_min, db_max);
        }
        self->depth_bounds_packed = db_packed;
    }

    if (direct_bds) {
        batch.Flush(gfxc);
    }
}

// VMA: 0x26d2110 (275 B)
// Iterates over all mip levels of a depth texture and decompresses HTILE surfaces via 0x26b6810.
void CGOutputMergerCacheGnm_DecompressDepthTextureMips(
    CGOutputMergerCacheGnm* self,
    void* gnmx_ctx_wrapper,
    const uint8_t* depth_tex_view)
{
    (void)self;
    uint32_t fmt = depth_tex_view[0x4b];
    if (fmt > 0x3Bu) {
        return;
    }
    uint32_t dsv_fmt = 0;
    if ((0x0FE0000000000000ULL >> fmt) & 1ULL) {
        dsv_fmt = 0x37u;
    } else if ((0x00000F8000000000ULL >> fmt) & 1ULL) {
        dsv_fmt = 0x28u;
    } else if ((0x0000000000780000ULL >> fmt) & 1ULL) {
        dsv_fmt = 0x14u;
    } else {
        return;
    }

    const uint8_t* tex = depth_tex_view + 0x18;
    uint32_t count = tex[0x32];
    if (count == 0) {
        count = sub_2165130_calcMipCount(
            *reinterpret_cast<const uint16_t*>(tex + 0x2c),
            *reinterpret_cast<const uint16_t*>(tex + 0x2e),
            1u);
    }
    if (count != 0) {
        alignas(16) uint8_t dsv_buf[0x38];
        for (uint32_t mip = 0; mip < count; ++mip) {
            uint16_t num_slices = *reinterpret_cast<const uint16_t*>(depth_tex_view + 0x48);
            if (sub_216fa60_initDtFromTexMip(dsv_buf, tex, dsv_fmt, mip, 0u, num_slices) != 0) {
                break;
            }
            sub_26b6810_decompressDt(
                gnmx_ctx_wrapper,
                dsv_buf,
                tex,
                0u,
                *reinterpret_cast<const uint16_t*>(depth_tex_view + 0x48),
                0u);
        }
    }
}

// VMA: 0x26d1eb0 (195 B)
// Flushes CMASK clear colors and decompresses all mip levels of a color texture via 0x26b5a70.
void CGOutputMergerCacheGnm_DecompressColorTextureMips(
    CGOutputMergerCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* color_tex_view)
{
    CGOutputMergerCacheGnm_FlushCmaskClearColors(self, gnmx_ctx_wrapper);
    const uint8_t* tex = color_tex_view + 0x18;
    uint32_t count = color_tex_view[0x4a];
    if (count == 0) {
        count = sub_2165130_calcMipCount(
            *reinterpret_cast<const uint16_t*>(tex + 0x2c),
            *reinterpret_cast<const uint16_t*>(tex + 0x2e),
            1u);
    }
    if (count != 0) {
        alignas(16) uint8_t rtv_buf[0x30];
        for (uint32_t mip = 0; mip < count; ++mip) {
            if (sub_216e1d0_initRtFromTexMip(
                    rtv_buf,
                    tex,
                    color_tex_view[0x4b],
                    mip,
                    0u,
                    *reinterpret_cast<const uint16_t*>(color_tex_view + 0x48)) != 0) {
                break;
            }
            sub_26b5a70_decompressRt(
                gnmx_ctx_wrapper,
                rtv_buf,
                tex,
                0u,
                *reinterpret_cast<const uint16_t*>(color_tex_view + 0x48),
                0u);
        }
    }
}

// VMA: 0x26d1e70 (61 B)
// Re-binds the currently cached DSV to Gnmx::GfxContext::setDepthRenderTarget (0x10739b0).
void CGOutputMergerCacheGnm_RebindCachedDepthTarget(
    const CGOutputMergerCacheGnm* self,
    void* gnmx_gfxc)
{
    uint32_t dsv_bank = (self->state_dirty_bits >> 1) & 1u;
    const uint8_t* dsv_obj = reinterpret_cast<const uint8_t*>(self->rtv_bank[dsv_bank][8]);
    const void* gnm_dt = (reinterpret_cast<uintptr_t>(dsv_obj) + 1ULL > 1ULL) ? (dsv_obj + 0x10) : nullptr;
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_RenderTargets)) {
        gx_direct_vk::EmitSetDepthRenderTarget(gnmx_gfxc, gnm_dt);
    } else {
        sub_10739b0_setDepthRenderTarget(gnmx_gfxc, gnm_dt);
    }
}

// VMA: 0x26d1e30 (55 B)
// Re-binds the currently cached RTV[slot] to Gnmx::GfxContext::setRenderTarget (0x10738b0).
void CGOutputMergerCacheGnm_RebindCachedRenderTarget(
    const CGOutputMergerCacheGnm* self,
    void* gnmx_gfxc,
    uint32_t slot)
{
    uint32_t rtv_bank = self->state_dirty_bits & 1u;
    const uint8_t* rtv_obj = reinterpret_cast<const uint8_t*>(self->rtv_bank[rtv_bank][slot]);
    const void* gnm_rt = (reinterpret_cast<uintptr_t>(rtv_obj) + 1ULL > 1ULL) ? (rtv_obj + 0x10) : nullptr;
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_RenderTargets)) {
        gx_direct_vk::EmitSetRenderTarget(gnmx_gfxc, slot, gnm_rt);
    } else {
        sub_10738b0_setRenderTarget(gnmx_gfxc, slot, gnm_rt);
    }
}

// VMA: 0x26d1e20 (11 B)
void CGOutputMergerCacheGnm_SetDepthRenderTargetDirect(void* self, void* gnmx_gfxc, const void* dsv) {
    (void)self;
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_RenderTargets)) {
        gx_direct_vk::EmitSetDepthRenderTarget(gnmx_gfxc, dsv);
    } else {
        sub_10739b0_setDepthRenderTarget(gnmx_gfxc, dsv);
    }
}

// VMA: 0x26d1e10 (13 B)
void CGOutputMergerCacheGnm_SetRenderTargetDirect(void* self, void* gnmx_gfxc, uint32_t slot, const void* rtv) {
    (void)self;
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_RenderTargets)) {
        gx_direct_vk::EmitSetRenderTarget(gnmx_gfxc, slot, rtv);
    } else {
        sub_10738b0_setRenderTarget(gnmx_gfxc, slot, rtv);
    }
}

static inline uint64_t QuantizeClearChannelUnorm16(float c) {
    uint32_t bits = 0;
    std::memcpy(&bits, &c, sizeof(uint32_t));
    if ((bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0) {
        const uint32_t qnan_bits = 0x7FC00000u;
        std::memcpy(&c, &qnan_bits, sizeof(float));
    } else {
        c = (c < 1.0f) ? c : 1.0f;
        std::memcpy(&bits, &c, sizeof(uint32_t));
        if ((bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0) {
            const uint32_t qnan_bits = 0x7FC00000u;
            std::memcpy(&c, &qnan_bits, sizeof(float));
        } else {
            c = (c > 0.0f) ? c : 0.0f;
        }
    }
    float scaled = c * 65535.0f;
    if (std::isnan(scaled)) {
        return 0ULL;
    }
    return static_cast<uint64_t>(scaled);
}

// VMA: 0x26d1bd0 (573 B)
// Quantizes float4 RGBA clear color [0.0..1.0] into a 64-bit packed UNORM16x4 value.
uint64_t CGOutputMergerCacheGnm_PackClearColorUnorm16x4(void* self, const float rgba[4]) {
    (void)self;
    uint64_t c0 = QuantizeClearChannelUnorm16(rgba[0]);
    uint64_t c1 = QuantizeClearChannelUnorm16(rgba[1]);
    uint64_t c2 = QuantizeClearChannelUnorm16(rgba[2]);
    uint64_t c3 = QuantizeClearChannelUnorm16(rgba[3]);
    return c0 | (c1 << 16) | (c2 << 32) | (c3 << 48);
}

// VMA: 0x26d1b50 (118 B)
// Updates pending CMASK clear colors for RTV slots [start_slot .. start_slot + count - 1].
void CGOutputMergerCacheGnm_SetPendingCmaskClearColors(
    CGOutputMergerCacheGnm* self,
    uint32_t start_slot,
    uint32_t count,
    const float (*rgba_array)[4])
{
    if (start_slot + count > start_slot) {
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t slot = start_slot + i;
            uint64_t packed = CGOutputMergerCacheGnm_PackClearColorUnorm16x4(self, rgba_array[slot]);
            self->pending_cmask_clear[slot] = packed;
            if (self->active_cmask_clear[slot] != packed) {
                self->state_dirty_bits |= ((1u << (slot & 0x1Fu)) << 4) & 0xFF0u;
            }
        }
    }
}

// VMA: 0x26d1b40 (1 B)
void CGOutputMergerCacheGnm_NoOpStub() {}

// VMA: 0x26d1980 (447 B)
// Copies an entire CGOutputMergerCacheGnm state block.
void CGOutputMergerCacheGnm_Copy(CGOutputMergerCacheGnm* dst, const CGOutputMergerCacheGnm* src) {
    std::memcpy(dst, src, sizeof(CGOutputMergerCacheGnm));
}

// VMA: 0x26d1720 (595 B)
// Full constructor/initializer for CGOutputMergerCacheGnm (zeros both pending and active CMASK clears).
void CGOutputMergerCacheGnm_Construct(CGOutputMergerCacheGnm* self) {
    self->alpha_to_coverage   = 0xFFFFFFFFu;
    self->rt_channel_mask     = ~0ULL;
    self->pad040              = 0;
    self->width_height_packed = 0xFFFFFFFFu;
    self->aa_sample_count     = 0xFFFFFFFFu;
    self->state_dirty_bits    = 0xFFCu;
    self->bound_blend_state   = reinterpret_cast<const CGBlendStateObject*>(-1ULL);
    self->bound_ds_state      = reinterpret_cast<const CGDepthStencilStateObject*>(-1ULL);
    for (int i = 0; i < 8; ++i) {
        self->blend_control[i]       = 0xFFFFFFFFu;
        self->rtv_bank[0][i]         = reinterpret_cast<void*>(-1ULL);
        self->rtv_bank[1][i]         = reinterpret_cast<void*>(-1ULL);
        self->pending_cmask_clear[i] = 0;
        self->active_cmask_clear[i]  = 0;
    }
    self->rtv_bank[0][8] = reinterpret_cast<void*>(-1ULL);
    self->rtv_bank[1][8] = reinterpret_cast<void*>(-1ULL);
    const uint32_t qnan_bits = 0x7FC00000u;
    for (int i = 0; i < 4; ++i) {
        std::memcpy(&self->blend_factor[i], &qnan_bits, sizeof(float));
    }
    self->depth_control       = 0xFFFFFFFFu;
    self->stencil_control     = 0xFFFFFFFFu;
    self->stencil_op_control  = 0;
    self->depth_bounds_packed = ~0ULL;
    self->active_depth_clear  = 1.0f;
    self->active_sample_mask  = 0;
    self->active_rtv_count    = 0;
    self->active_om_flags     = 0x1Cu;
    std::memcpy(&self->pending_depth_clear, &self->active_depth_clear, 8);
}

// ============================================================================
// 3. Dynamic Buffer Map Ring Lookup & Cleanup (0x26d0bb0 .. 0x26d1082)
// ============================================================================

struct DynamicMapChunkNode {
    void*    mapped_gpu_ptr; // +0x00
    uint64_t resource_id;    // +0x08
    uint8_t* mapped_buf_res; // +0x10
};

struct CGDynamicBufferRingGnm {
    void*    gpu_allocator;   // +0x00
    uint8_t* head_chunk;      // +0x08
    uint8_t* tail_chunk;      // +0x10
    uint32_t nodes_per_chunk; // +0x18 (default 8)
    uint32_t tail_used_count; // +0x1c
};

static inline void CallAllocatorFree70(void* alloc, void* ptr) {
    using FreeFn = void(*)(void*, void*);
    void** vtbl = *reinterpret_cast<void***>(alloc);
    reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(alloc, ptr);
}

// VMA: 0x26d0ff0 (147 B)
// Resolves the latest dynamically mapped GPU base address for a buffer resource,
// or falls back to the static GPU address at `[buffer_res + 0x08]`.
void* CGDynamicBufferRingGnm_ResolveGpuAddress(
    const CGDynamicBufferRingGnm* self,
    uint8_t* buffer_res)
{
    uint8_t* chunk = self->head_chunk;
    uint32_t cap   = self->nodes_per_chunk;
    uint64_t key   = sub_216e1b0_getBufferBaseAddr(buffer_res);
    if (chunk != nullptr) {
        size_t next_off = static_cast<size_t>(cap) * 24ULL;
        while (chunk != nullptr) {
            uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
            uint32_t count = (next_chunk != nullptr) ? cap : self->tail_used_count;
            for (uint32_t i = 0; i < count; ++i) {
                uint8_t* node = chunk + static_cast<size_t>(i) * 24ULL;
                if (*reinterpret_cast<const uint64_t*>(node + 0x08) == key) {
                    buffer_res = *reinterpret_cast<uint8_t**>(node + 0x10);
                    return *reinterpret_cast<void**>(buffer_res + 0x08);
                }
            }
            chunk = next_chunk;
        }
    }
    return *reinterpret_cast<void**>(buffer_res + 0x08);
}

// VMA: 0x26d0f60 (139 B)
// Finds the active DynamicMapChunkNode for a given buffer resource, or returns nullptr.
void* CGDynamicBufferRingGnm_FindMappedNode(
    const CGDynamicBufferRingGnm* self,
    uint8_t* buffer_res)
{
    uint8_t* chunk = self->head_chunk;
    uint32_t cap   = self->nodes_per_chunk;
    uint64_t key   = sub_216e1b0_getBufferBaseAddr(buffer_res);
    if (chunk != nullptr) {
        size_t next_off = static_cast<size_t>(cap) * 24ULL;
        while (chunk != nullptr) {
            uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
            uint32_t count = (next_chunk != nullptr) ? cap : self->tail_used_count;
            for (uint32_t i = 0; i < count; ++i) {
                uint8_t* node = chunk + static_cast<size_t>(i) * 24ULL;
                if (*reinterpret_cast<const uint64_t*>(node + 0x08) == key) {
                    return node;
                }
            }
            chunk = next_chunk;
        }
    }
    return nullptr;
}

// VMA: 0x26d0ca0 (132 B)
// Frees all linked chunk blocks in the dynamic buffer ring and resets pointers.
void CGDynamicBufferRingGnm_ClearChunks(CGDynamicBufferRingGnm* self) {
    if (self->gpu_allocator != nullptr) {
        uint8_t* chunk = self->head_chunk;
        if (chunk != nullptr) {
            size_t next_off = static_cast<size_t>(self->nodes_per_chunk) * 24ULL;
            while (chunk != nullptr) {
                uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
                CallAllocatorFree70(self->gpu_allocator, chunk);
                chunk = next_chunk;
            }
        }
    }
    self->head_chunk      = nullptr;
    self->tail_chunk      = nullptr;
    self->tail_used_count = 0;
}

// VMA: 0x26d0c70 (36 B)
int32_t CGDynamicBufferRingGnm_Init(CGDynamicBufferRingGnm* self, void* gpu_alloc) {
    self->gpu_allocator   = gpu_alloc;
    self->head_chunk      = nullptr;
    self->tail_chunk      = nullptr;
    self->nodes_per_chunk = 8;
    self->tail_used_count = 0;
    return 0;
}

// VMA: 0x26d0bf0 (124 B)
void CGDynamicBufferRingGnm_Destroy(CGDynamicBufferRingGnm* self) {
    if (self->gpu_allocator != nullptr) {
        uint8_t* chunk = self->head_chunk;
        if (chunk != nullptr) {
            size_t next_off = static_cast<size_t>(self->nodes_per_chunk) * 24ULL;
            while (chunk != nullptr) {
                uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
                CallAllocatorFree70(self->gpu_allocator, chunk);
                chunk = next_chunk;
            }
        }
        self->tail_used_count = 0;
        self->tail_chunk      = nullptr;
        self->head_chunk      = nullptr;
    }
}

// VMA: 0x26d0bd0 (31 B)
void CGDynamicBufferRingGnm_MoveInit(CGDynamicBufferRingGnm* dst, const CGDynamicBufferRingGnm* src) {
    std::memcpy(dst, src, sizeof(CGDynamicBufferRingGnm));
}

// VMA: 0x26d0bb0 (32 B)
void CGDynamicBufferRingGnm_ZeroInit(CGDynamicBufferRingGnm* self) {
    std::memset(self, 0, sizeof(CGDynamicBufferRingGnm));
}

} // namespace Bloodborne::CoreGraphics2::Gnm
