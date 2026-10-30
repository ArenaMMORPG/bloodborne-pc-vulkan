// ============================================================================
// Bloodborne Engine Decompilation Project (CUSA03173 v1.09)
// Module: 01_CoreGraphics2_GNM
// File: CGContextStateShadowGnm.cpp
// Range: 0x26d4550 .. 0x26d5960 (78 DB Functions / 89 Entry Points — D3D11-Style
//        Immediate/Deferred Context Shadow State & Draw/Dispatch Forwarding Layer)
// ============================================================================

#include <cstdint>
#include <cstring>

namespace Bloodborne::CoreGraphics2::Gnm {

// Each of the 6 shader stages (VS=0, PS=1, GS=2, HS=3, DS=4, CS=5) has an
// identical 0x4f8-byte (1272-byte) CPU shadow block starting at +0x5c0:
//   VS: +0x05c0 .. +0x0ab7
//   PS: +0x0ab8 .. +0x0faf
//   GS: +0x0fb0 .. +0x14a7
//   HS: +0x14a8 .. +0x199f
//   DS: +0x19a0 .. +0x1e97
//   CS: +0x1e98 .. +0x238f
struct ShaderStageShadowState {
    void* constant_buffers[16];  // +0x000 (0x080 B)
    void* shader_resources[128]; // +0x080 (0x400 B)
    void* samplers[14];          // +0x480 (0x070 B)
    void* shader;                // +0x4f0 (0x008 B)
};
static_assert(sizeof(ShaderStageShadowState) == 0x4f8, "Exact 0x4f8 stage shadow size");

struct ViewportDesc {
    float x, y, width, height, min_depth, max_depth; // 24 bytes (0x18)
};

struct ScissorRect {
    int32_t left, top, right, bottom; // 16 bytes (0x10)
};

// Embedded at `outer_context + 0xb730` (size 0x23d0 total, shadow region +0x28..+0x23cf = 0x23a8 bytes)
struct CGContextStateShadowGnm {
    uint8_t                inner_cmd_header[0x28]; // +0x000
    void*                  input_layout;           // +0x028 (also start of inner_ctx forwarding)
    uint32_t               primitive_topology;     // +0x030
    uint32_t               pad034;                 // +0x034
    void*                  index_buffer;           // +0x038
    uint32_t               index_format;           // +0x040
    uint32_t               index_offset;           // +0x044
    void*                  vertex_buffers[32];     // +0x048 (0x100 B)
    uint32_t               vertex_strides[32];     // +0x148 (0x080 B)
    uint32_t               vertex_offsets[32];     // +0x1c8 (0x080 B)
    void*                  rasterizer_state;       // +0x248
    uint32_t               num_viewports;          // +0x250
    ViewportDesc           viewports[16];          // +0x254 (0x180 B)
    uint32_t               num_scissors;           // +0x3d4
    ScissorRect            scissors[16];           // +0x3d8 (0x100 B)
    uint32_t               pad4d8[2];              // +0x4d8
    void*                  blend_state;            // +0x4e0
    void*                  depth_stencil_state;    // +0x4e8
    void*                  render_target_views[8]; // +0x508 (shifted in shadow: +0x508..+0x547)
    void*                  depth_stencil_view;     // +0x548
    void*                  unordered_access_views[8]; // +0x550..+0x58f
    float                  blend_factor[4];        // +0x590
    float                  depth_bounds_min;       // +0x5a0
    float                  depth_bounds_max;       // +0x5a4
    uint32_t               sample_mask;            // +0x5a8
    uint8_t                stencil_ref;            // +0x5ac
    uint8_t                pad5ad[0x13];           // +0x5ad..+0x5bf
    ShaderStageShadowState stages[6];              // +0x5c0..+0x238f (VS=0, PS=1, GS=2, HS=3, DS=4, CS=5)
    void*                  cs_uavs[8];             // +0x2390..+0x23cf
};

// ============================================================================
// External Guest Bridges for 0x26d4550 .. 0x26d5960
// ============================================================================

extern "C" {
extern uintptr_t g_eboot_live_slide;

#define DEFINE_SHADOW_BRIDGE(fn_name, hex_vma) \
    __attribute__((weak, naked, used, visibility("hidden"))) void fn_name() { \
        __asm__ volatile( \
            "movq g_eboot_live_slide(%rip), %r11\n\t" \
            "testq %r11, %r11\n\t" \
            "jz 1f\n\t" \
            "addq $" #hex_vma ", %r11\n\t" \
            "jmpq *%r11\n\t" \
            "1:\n\t" \
            "xorl %eax, %eax\n\t" \
            "retq\n\t" \
        ); \
    }

DEFINE_SHADOW_BRIDGE(sub_26ab7f0_bridge, 0x26ab7f0)
DEFINE_SHADOW_BRIDGE(sub_26ab7e0_bridge, 0x26ab7e0)
DEFINE_SHADOW_BRIDGE(sub_2167bf0_bridge, 0x2167bf0)
DEFINE_SHADOW_BRIDGE(sub_2167c00_bridge, 0x2167c00)
DEFINE_SHADOW_BRIDGE(sub_2167c10_bridge, 0x2167c10)
DEFINE_SHADOW_BRIDGE(sub_2167c20_bridge, 0x2167c20)
DEFINE_SHADOW_BRIDGE(sub_2167c30_bridge, 0x2167c30)
DEFINE_SHADOW_BRIDGE(sub_2167c50_bridge, 0x2167c50)
DEFINE_SHADOW_BRIDGE(sub_2167c60_bridge, 0x2167c60)
DEFINE_SHADOW_BRIDGE(sub_2167c70_bridge, 0x2167c70)
DEFINE_SHADOW_BRIDGE(sub_2167c80_bridge, 0x2167c80)
DEFINE_SHADOW_BRIDGE(sub_2167c90_bridge, 0x2167c90)
DEFINE_SHADOW_BRIDGE(sub_2167ca0_bridge, 0x2167ca0)
DEFINE_SHADOW_BRIDGE(sub_2167cb0_bridge, 0x2167cb0)
DEFINE_SHADOW_BRIDGE(sub_2167cc0_bridge, 0x2167cc0)
DEFINE_SHADOW_BRIDGE(sub_2167cd0_bridge, 0x2167cd0)
DEFINE_SHADOW_BRIDGE(sub_2167ce0_bridge, 0x2167ce0)
DEFINE_SHADOW_BRIDGE(sub_2167cf0_bridge, 0x2167cf0)
DEFINE_SHADOW_BRIDGE(sub_2167d00_bridge, 0x2167d00)
DEFINE_SHADOW_BRIDGE(sub_2167d10_bridge, 0x2167d10)
DEFINE_SHADOW_BRIDGE(sub_2167d20_bridge, 0x2167d20)
DEFINE_SHADOW_BRIDGE(sub_2167d30_bridge, 0x2167d30)
DEFINE_SHADOW_BRIDGE(sub_2167d40_bridge, 0x2167d40)
DEFINE_SHADOW_BRIDGE(sub_2167d50_bridge, 0x2167d50)
DEFINE_SHADOW_BRIDGE(sub_2167d60_bridge, 0x2167d60)
DEFINE_SHADOW_BRIDGE(sub_2167d70_bridge, 0x2167d70)
DEFINE_SHADOW_BRIDGE(sub_2167d80_bridge, 0x2167d80)
DEFINE_SHADOW_BRIDGE(sub_2167d90_bridge, 0x2167d90)
DEFINE_SHADOW_BRIDGE(sub_2167da0_bridge, 0x2167da0)
DEFINE_SHADOW_BRIDGE(sub_2167db0_bridge, 0x2167db0)
DEFINE_SHADOW_BRIDGE(sub_2167dc0_bridge, 0x2167dc0)
DEFINE_SHADOW_BRIDGE(sub_2167dd0_bridge, 0x2167dd0)
DEFINE_SHADOW_BRIDGE(sub_2167de0_bridge, 0x2167de0)
DEFINE_SHADOW_BRIDGE(sub_2167df0_bridge, 0x2167df0)
DEFINE_SHADOW_BRIDGE(sub_2167e00_bridge, 0x2167e00)
DEFINE_SHADOW_BRIDGE(sub_2167e10_bridge, 0x2167e10)
DEFINE_SHADOW_BRIDGE(sub_2167e20_bridge, 0x2167e20)
DEFINE_SHADOW_BRIDGE(sub_2167e40_bridge, 0x2167e40)
DEFINE_SHADOW_BRIDGE(sub_2167e50_bridge, 0x2167e50)
DEFINE_SHADOW_BRIDGE(sub_2167e60_bridge, 0x2167e60)
DEFINE_SHADOW_BRIDGE(sub_2167e70_bridge, 0x2167e70)
DEFINE_SHADOW_BRIDGE(sub_2168400_bridge, 0x2168400)
DEFINE_SHADOW_BRIDGE(sub_21682c0_bridge, 0x21682c0)
DEFINE_SHADOW_BRIDGE(sub_216aca0_bridge, 0x216aca0)
DEFINE_SHADOW_BRIDGE(sub_26d0bf0_bridge, 0x26d0bf0)
DEFINE_SHADOW_BRIDGE(sub_26d1b40_bridge, 0x26d1b40)
DEFINE_SHADOW_BRIDGE(sub_26aec40_bridge, 0x26aec40)

#undef DEFINE_SHADOW_BRIDGE
} // extern "C"

// ============================================================================
// Outer Deferred/Immediate Context Lifecycle (0x26d55e0 .. 0x26d5960)
// ============================================================================

// VMA: 0x26d5960 (143 B)
// Frees linked command chunks at `+0x08` and zeroes `+0x08..+0xaf`.
int32_t CGDeviceGnm_CreateDeferredContext(
    void* out_handle,
    [[maybe_unused]] void* device_wrapper,
    [[maybe_unused]] void* desc)
{
    uint8_t* self = static_cast<uint8_t*>(out_handle);
    void*** alloc_ptr = reinterpret_cast<void***>(*reinterpret_cast<void**>(self + 0x00));
    if (alloc_ptr) {
        uint64_t next_off = static_cast<uint64_t>(*reinterpret_cast<uint32_t*>(self + 0x18)) << 6;
        uint8_t* chunk = *reinterpret_cast<uint8_t**>(self + 0x08);
        while (chunk) {
            alloc_ptr = reinterpret_cast<void***>(*reinterpret_cast<void**>(self + 0x00));
            uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
            using FreeFn = void (*)(void*, void*);
            reinterpret_cast<FreeFn>((*alloc_ptr)[14])(alloc_ptr, chunk);
            chunk = next_chunk;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
    std::memset(self + 0x1c, 0, 0x94);
    return 0;
}

// VMA: 0x26d5940 (18 B)
void CGDeferredContextGnm_Destroy(uint8_t* outer_ctx) {
    std::memset(outer_ctx, 0, 0xB0);
}

// VMA: 0x26d5850 (83 B)
// Finishes recording a deferred command list on `outer_ctx` (`0x216aca0`) and zeroes shadow state at `+0xb730` if `!restore_state`.
int32_t CGImmediateContextGnm_Init(uint8_t* outer_ctx, void* out_cmd_list, bool restore_state)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm26CGImmediateContextGnm_InitEPhPv");
int32_t CGImmediateContextGnm_Init(uint8_t* outer_ctx, void* out_cmd_list, bool restore_state) {
    if (!out_cmd_list) {
        return 8;
    }
    int32_t res = reinterpret_cast<int32_t (*)(void*, void*, void*, uint32_t)>(&sub_216aca0_bridge)(
        outer_ctx, outer_ctx + 0xb730, out_cmd_list, static_cast<uint32_t>(restore_state));
    if (!restore_state) {
        std::memset(outer_ctx + 0xb730, 0, 0x23a8);
    }
    return res;
}

// VMA: 0x26d5810 (56 B)
// Finalizes immediate context sub-objects (+0xb6f8, +0xb558, +0x10).
extern "C" void sub_26d5810(uint8_t* outer_ctx) {
    reinterpret_cast<void (*)(void*)>(&sub_21682c0_bridge)(outer_ctx);
    reinterpret_cast<void (*)(void*)>(&sub_26d0bf0_bridge)(outer_ctx + 0xb6f8);
    reinterpret_cast<void (*)(void*)>(&sub_26d1b40_bridge)(outer_ctx + 0xb558);
    reinterpret_cast<void (*)(void*)>(&sub_26aec40_bridge)(outer_ctx + 0x10);
}

// VMA: 0x26d57e0 (43 B)
// Initializes deferred context and zeroes the 0x23a8-byte shadow state at +0xb730.
int32_t CGDeferredContextGnm_Init(uint8_t* outer_ctx, [[maybe_unused]] void* init_params) {
    reinterpret_cast<void (*)(void*)>(&sub_2168400_bridge)(outer_ctx);
    std::memset(outer_ctx + 0xb730, 0, 0x23a8);
    return 0;
}

// VMA: 0x26d5740 (4 B)
// Identity accessor (`mov rax, rdi; ret`).
extern "C" void* sub_26d5740(void* self) {
    return self;
}

// VMA: 0x26d56c0 (124 B)
// Releases linked command chunk chain (`self + 0x08`, stride `self->u32_18 << 6`) via allocator (`self + 0x00`).
extern "C" void sub_26d56c0(uint8_t* self) {
    CGDeviceGnm_CreateDeferredContext(self, nullptr, nullptr);
    void*** alloc_ptr = reinterpret_cast<void***>(*reinterpret_cast<void**>(self + 0x00));
    if (alloc_ptr) {
        uint64_t next_off = static_cast<uint64_t>(*reinterpret_cast<uint32_t*>(self + 0x18)) << 6;
        uint8_t* chunk = *reinterpret_cast<uint8_t**>(self + 0x08);
        while (chunk) {
            alloc_ptr = reinterpret_cast<void***>(*reinterpret_cast<void**>(self + 0x00));
            uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
            using FreeFn = void (*)(void*, void*);
            reinterpret_cast<FreeFn>((*alloc_ptr)[14])(alloc_ptr, chunk);
            chunk = next_chunk;
        }
        *reinterpret_cast<uint32_t*>(self + 0x1c) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x08) = 0;
    }
}

// VMA: 0x26d56b0 (16 B)
// Copies 0xB0 bytes from `src` (`rsi`) to `dst` (`rdi`).
extern "C" void* sub_26d56b0(void* dst, const void* src) {
    return std::memcpy(dst, src, 0xB0);
}

// VMA: 0x26d5680 (36 B)
// Zeroes 32-byte command list header (`+0x00..+0x1f`) and tail-calls `CGDeferredContextGnm_Destroy`.
extern "C" void sub_26d5680(uint64_t* cmd_list) {
    cmd_list[3] = 0;
    cmd_list[2] = 0;
    cmd_list[1] = 0;
    cmd_list[0] = 0;
    CGDeferredContextGnm_Destroy(reinterpret_cast<uint8_t*>(cmd_list));
}

// VMA: 0x26d55e0 (160 B)
// Finishes recording a deferred command list and resets the shadow state (+0x28, 0x23a8 B).
int32_t CGContextStateShadowGnm_FinishCommandList(
    CGContextStateShadowGnm* self,
    [[maybe_unused]] void* out_cmd_list,
    bool restore_state)
{
    if (!restore_state) {
        std::memset(&self->input_layout, 0, 0x23a8);
    }
    return 0;
}

// ============================================================================
// Draw / Dispatch / Sync Forwarding Wrappers (0x26d54d0 .. 0x26d55d0)
// ============================================================================

// VMA: 0x26d55d0 (26 B)
int32_t CGContextStateShadowGnm_Vma26d55d0_ExecuteCommandList(
    CGContextStateShadowGnm* self,
    void* cmd_list) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm42CGContextStateShadowGnm_ExecuteCommandListEPNS1_23CGContextStateShadowGnmEPvb");
int32_t CGContextStateShadowGnm_Vma26d55d0_ExecuteCommandList(
    CGContextStateShadowGnm* self,
    void* cmd_list)
{
    if (!cmd_list) {
        return 8;
    }
    return reinterpret_cast<int32_t (*)(void*, void*, void*)>(&sub_2167e70_bridge)(
        self, &self->input_layout, cmd_list);
}

// VMA: 0x26d55c0 (9 B)
void CGContextStateShadowGnm_Flush(CGContextStateShadowGnm* self) {
    reinterpret_cast<void (*)(void*, void*)>(&sub_2167e60_bridge)(self, &self->input_layout);
}

// VMA: 0x26d55a0 (19 B)
void CGContextStateShadowGnm_Vma26d55a0_DrawIndexedInstancedIndirect(
    CGContextStateShadowGnm* self,
    void* buf,
    uint32_t offset) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm34CGContextStateShadowGnm_PushMarkerEPNS1_23CGContextStateShadowGnmEPKc");
void CGContextStateShadowGnm_Vma26d55a0_DrawIndexedInstancedIndirect(
    CGContextStateShadowGnm* self,
    void* buf,
    uint32_t offset)
{
    reinterpret_cast<void (*)(void*, void*, void*, uint32_t)>(&sub_2167e50_bridge)(
        self, &self->input_layout, buf, offset);
}

// VMA: 0x26d5580 (19 B)
void CGContextStateShadowGnm_Vma26d5580_DrawInstancedIndirect(
    CGContextStateShadowGnm* self,
    void* buf,
    uint32_t offset) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm32CGContextStateShadowGnm_EndQueryEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d5580_DrawInstancedIndirect(
    CGContextStateShadowGnm* self,
    void* buf,
    uint32_t offset)
{
    reinterpret_cast<void (*)(void*, void*, void*, uint32_t)>(&sub_2167e40_bridge)(
        self, &self->input_layout, buf, offset);
}

// VMA: 0x26d5550 (42 B)
void CGContextStateShadowGnm_Vma26d5550_DrawIndexedInstanced(
    CGContextStateShadowGnm* self,
    uint32_t idx_cnt,
    uint32_t inst_cnt,
    uint32_t start_idx,
    int32_t base_vtx,
    uint32_t start_inst) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm36CGContextStateShadowGnm_CopyResourceEPNS1_23CGContextStateShadowGnmEPvS4_");
void CGContextStateShadowGnm_Vma26d5550_DrawIndexedInstanced(
    CGContextStateShadowGnm* self,
    uint32_t idx_cnt,
    uint32_t inst_cnt,
    uint32_t start_idx,
    int32_t base_vtx,
    uint32_t start_inst)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, uint32_t, int32_t, uint32_t)>(&sub_2167e20_bridge)(
        self, &self->input_layout, idx_cnt, inst_cnt, start_idx, base_vtx, start_inst);
}

// VMA: 0x26d5530 (21 B)
void CGContextStateShadowGnm_Vma26d5530_DrawInstanced(
    CGContextStateShadowGnm* self,
    uint32_t vtx_cnt,
    uint32_t inst_cnt,
    uint32_t start_vtx,
    uint32_t start_inst) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm45CGContextStateShadowGnm_DrawInstancedIndirectEPNS1_23CGContextStateShadowGnmEPvj");
void CGContextStateShadowGnm_Vma26d5530_DrawInstanced(
    CGContextStateShadowGnm* self,
    uint32_t vtx_cnt,
    uint32_t inst_cnt,
    uint32_t start_vtx,
    uint32_t start_inst)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, uint32_t, uint32_t)>(&sub_2167e10_bridge)(
        self, &self->input_layout, vtx_cnt, inst_cnt, start_vtx, start_inst);
}

// VMA: 0x26d5510 (18 B)
void CGContextStateShadowGnm_Vma26d5510_DrawIndexed(
    CGContextStateShadowGnm* self,
    uint32_t idx_cnt,
    uint32_t start_idx,
    int32_t base_vtx) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_DrawIndexedInstancedEPNS1_23CGContextStateShadowGnmEjjjij");
void CGContextStateShadowGnm_Vma26d5510_DrawIndexed(
    CGContextStateShadowGnm* self,
    uint32_t idx_cnt,
    uint32_t start_idx,
    int32_t base_vtx)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, int32_t)>(&sub_2167e00_bridge)(
        self, &self->input_layout, idx_cnt, start_idx, base_vtx);
}

// VMA: 0x26d54f0 (17 B)
void CGContextStateShadowGnm_Vma26d54f0_Draw(
    CGContextStateShadowGnm* self,
    uint32_t vtx_cnt,
    uint32_t start_vtx) __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_DrawIndexedEPNS1_23CGContextStateShadowGnmEjji");
void CGContextStateShadowGnm_Vma26d54f0_Draw(
    CGContextStateShadowGnm* self,
    uint32_t vtx_cnt,
    uint32_t start_vtx)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t)>(&sub_2167df0_bridge)(
        self, &self->input_layout, vtx_cnt, start_vtx);
}

// VMA: 0x26d54d0 (18 B)
void CGContextStateShadowGnm_Dispatch(
    CGContextStateShadowGnm* self,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, uint32_t)>(&sub_2167de0_bridge)(
        self, &self->input_layout, x, y, z);
}

// ============================================================================
// Stage Set/Get Shadow Helpers (VS=0, PS=1, GS=2, HS=3, DS=4, CS=5)
// ============================================================================

namespace {
inline void Stage_SetConstantBuffers(ShaderStageShadowState& st, uint32_t start_slot, uint32_t count, void* const* cbs) {
    for (uint32_t i = 0; i < count; ++i) st.constant_buffers[start_slot + i] = cbs[i];
}
inline void Stage_SetShaderResources(ShaderStageShadowState& st, uint32_t start_slot, uint32_t count, void* const* srvs) {
    for (uint32_t i = 0; i < count; ++i) st.shader_resources[start_slot + i] = srvs[i];
}
inline void Stage_SetSamplers(ShaderStageShadowState& st, uint32_t start_slot, uint32_t count, void* const* samplers) {
    for (uint32_t i = 0; i < count; ++i) st.samplers[start_slot + i] = samplers[i];
}
inline void Stage_GetConstantBuffers(const ShaderStageShadowState& st, uint32_t start_slot, uint32_t count, void** out_cbs) {
    if (out_cbs) std::memcpy(out_cbs + start_slot, &st.constant_buffers[start_slot], count * sizeof(void*));
}
inline void Stage_GetShaderResources(const ShaderStageShadowState& st, uint32_t start_slot, uint32_t count, void** out_srvs) {
    if (out_srvs) std::memcpy(out_srvs + start_slot, &st.shader_resources[start_slot], count * sizeof(void*));
}
inline void Stage_GetSamplers(const ShaderStageShadowState& st, uint32_t start_slot, uint32_t count, void** out_samplers) {
    if (out_samplers) std::memcpy(out_samplers + start_slot, &st.samplers[start_slot], count * sizeof(void*));
}
} // namespace

// --- CS Stage (stages[5], base +0x1e98, 0x26d52e0 .. 0x26d54c0) ---

// VMA: 0x26d54c0 (16 B)
void CGContextStateShadowGnm_CSGetShader(const CGContextStateShadowGnm* self, void** out_cs) {
    if (out_cs) *out_cs = self->stages[5].shader;
}

// VMA: 0x26d5490 (34 B)
void CGContextStateShadowGnm_Vma26d5490_CSGetSamplers(
    const CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void** out_s)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm49CGContextStateShadowGnm_CSGetUnorderedAccessViewsEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d5490_CSGetSamplers(
    const CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void** out_s)
{
    Stage_GetSamplers(self->stages[5], start, count, out_s);
}

// VMA: 0x26d5460 (34 B)
void CGContextStateShadowGnm_Vma26d5460_CSGetShaderResources(
    const CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void** out_srvs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm37CGContextStateShadowGnm_CSGetSamplersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d5460_CSGetShaderResources(
    const CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void** out_srvs)
{
    Stage_GetShaderResources(self->stages[5], start, count, out_srvs);
}

// VMA: 0x26d5430 (34 B)
void CGContextStateShadowGnm_Vma26d5430_CSGetConstantBuffers(
    const CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void** out_cbs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_CSGetShaderResourcesEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d5430_CSGetConstantBuffers(
    const CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void** out_cbs)
{
    Stage_GetConstantBuffers(self->stages[5], start, count, out_cbs);
}

// VMA: 0x26d5400 (37 B)
void CGContextStateShadowGnm_Vma26d5400_CSSetShader(
    CGContextStateShadowGnm* self, void* cs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_CSGetConstantBuffersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d5400_CSSetShader(
    CGContextStateShadowGnm* self, void* cs)
{
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167dd0_bridge)(self, &self->input_layout, cs);
    self->stages[5].shader = cs;
}

// VMA: 0x26d53a0 (81 B)
void CGContextStateShadowGnm_Vma26d53a0_CSSetSamplers(
    CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void* const* s)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_CSSetShaderEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d53a0_CSSetSamplers(
    CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void* const* s)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167dc0_bridge)(
        self, &self->input_layout, start, count, s);
    Stage_SetSamplers(self->stages[5], start, count, s);
}

// VMA: 0x26d5350 (64 B)
void CGContextStateShadowGnm_CSSetUnorderedAccessViews(
    CGContextStateShadowGnm* self,
    uint32_t start,
    uint32_t count,
    void* const* uavs,
    [[maybe_unused]] const uint32_t* init_counts)
{
    for (uint32_t i = 0; i < count; ++i) self->cs_uavs[start + i] = uavs[i];
}

// VMA: 0x26d52e0 (81 B)
void CGContextStateShadowGnm_Vma26d52e0_CSSetConstantBuffers(
    CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void* const* cbs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm37CGContextStateShadowGnm_CSSetSamplersEPNS1_23CGContextStateShadowGnmEjjPKPv");
void CGContextStateShadowGnm_Vma26d52e0_CSSetConstantBuffers(
    CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void* const* cbs)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167da0_bridge)(
        self, &self->input_layout, start, count, cbs);
    Stage_SetConstantBuffers(self->stages[5], start, count, cbs);
}

// VMA: 0x26d5340 (81 B)
void CGContextStateShadowGnm_CSSetShaderResources(CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void* const* srvs) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167db0_bridge)(
        self, &self->input_layout, start, count, srvs);
    Stage_SetShaderResources(self->stages[5], start, count, srvs);
}

// VMA: 0x26d52d0 (81 B)
void CGContextStateShadowGnm_CSSetConstantBuffers(CGContextStateShadowGnm* self, uint32_t start, uint32_t count, void* const* cbs) {
    Stage_SetConstantBuffers(self->stages[5], start, count, cbs);
}

// --- OM Stage (0x26d5180 .. 0x26d5290) ---

// VMA: 0x26d5290 (65 B)
void CGContextStateShadowGnm_OMGetRenderTargets(const CGContextStateShadowGnm* self, uint32_t count, void** out_rtvs, void** out_dsv) {
    if (out_rtvs) std::memcpy(out_rtvs, self->render_target_views, count * sizeof(void*));
    if (out_dsv) *out_dsv = self->depth_stencil_view;
}

// VMA: 0x26d5180 (258 B)
void CGContextStateShadowGnm_OMSetRenderTargets(CGContextStateShadowGnm* self, uint32_t count, void* const* rtvs, void* dsv) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, void* const*, void*)>(&sub_2167d90_bridge)(
        self, &self->input_layout, count, rtvs, dsv);
    for (uint32_t i = 0; i < count; ++i) self->render_target_views[i] = rtvs[i];
    self->depth_stencil_view = dsv;
}

// --- DS Stage (stages[4], base +0x19a0, 0x26d4f90 .. 0x26d5170) ---

// VMA: 0x26d5170 (16 B)
void CGContextStateShadowGnm_DSGetShader(const CGContextStateShadowGnm* self, void** out_ds) {
    if (out_ds) *out_ds = self->stages[4].shader;
}
// VMA: 0x26d5140 (34 B)
void CGContextStateShadowGnm_DSGetSamplers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetSamplers(self->stages[4], s, c, out); }
// VMA: 0x26d5110 (34 B)
void CGContextStateShadowGnm_DSGetShaderResources(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetShaderResources(self->stages[4], s, c, out); }
// VMA: 0x26d50b0 (37 B)
void CGContextStateShadowGnm_Vma26d50b0_DSSetShader(CGContextStateShadowGnm* self, void* ds)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_DSGetConstantBuffersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d50b0_DSSetShader(CGContextStateShadowGnm* self, void* ds) {
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167d80_bridge)(self, &self->input_layout, ds);
    self->stages[4].shader = ds;
}
// VMA: 0x26d50e0 (34 B)
void CGContextStateShadowGnm_Vma26d50e0_DSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_DSSetShaderEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d50e0_DSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) {
    Stage_GetConstantBuffers(self->stages[4], s, c, out);
}
// VMA: 0x26d5050 (81 B)
void CGContextStateShadowGnm_DSSetSamplers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167d70_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetSamplers(self->stages[4], s, c, in);
}
// VMA: 0x26d4ff0 (81 B)
void CGContextStateShadowGnm_DSSetShaderResources(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167d60_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetShaderResources(self->stages[4], s, c, in);
}
// VMA: 0x26d4f90 (81 B)
void CGContextStateShadowGnm_DSSetConstantBuffers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167d50_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetConstantBuffers(self->stages[4], s, c, in);
}

// --- HS Stage (stages[3], base +0x14a8, 0x26d4da0 .. 0x26d4f80) ---

// VMA: 0x26d4f80 (16 B)
void CGContextStateShadowGnm_HSGetShader(const CGContextStateShadowGnm* self, void** out_hs) {
    if (out_hs) *out_hs = self->stages[3].shader;
}
// VMA: 0x26d4f50 (34 B)
void CGContextStateShadowGnm_HSGetSamplers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetSamplers(self->stages[3], s, c, out); }
// VMA: 0x26d4f20 (34 B)
void CGContextStateShadowGnm_HSGetShaderResources(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetShaderResources(self->stages[3], s, c, out); }
// VMA: 0x26d4ec0 (37 B)
void CGContextStateShadowGnm_Vma26d4ec0_HSSetShader(CGContextStateShadowGnm* self, void* hs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_HSGetConstantBuffersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d4ec0_HSSetShader(CGContextStateShadowGnm* self, void* hs) {
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167d40_bridge)(self, &self->input_layout, hs);
    self->stages[3].shader = hs;
}
// VMA: 0x26d4ef0 (34 B)
void CGContextStateShadowGnm_Vma26d4ef0_HSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_HSSetShaderEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d4ef0_HSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) {
    Stage_GetConstantBuffers(self->stages[3], s, c, out);
}
// VMA: 0x26d4e60 (81 B)
void CGContextStateShadowGnm_HSSetSamplers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167d30_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetSamplers(self->stages[3], s, c, in);
}
// VMA: 0x26d4e00 (81 B)
void CGContextStateShadowGnm_HSSetShaderResources(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167d20_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetShaderResources(self->stages[3], s, c, in);
}
// VMA: 0x26d4da0 (81 B)
void CGContextStateShadowGnm_HSSetConstantBuffers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167d10_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetConstantBuffers(self->stages[3], s, c, in);
}

// --- GS Stage (stages[2], base +0x0fb0, 0x26d4bb0 .. 0x26d4d90) ---

// VMA: 0x26d4d90 (16 B)
void CGContextStateShadowGnm_GSGetShader(const CGContextStateShadowGnm* self, void** out_gs) {
    if (out_gs) *out_gs = self->stages[2].shader;
}
// VMA: 0x26d4d60 (34 B)
void CGContextStateShadowGnm_GSGetSamplers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetSamplers(self->stages[2], s, c, out); }
// VMA: 0x26d4d30 (34 B)
void CGContextStateShadowGnm_GSGetShaderResources(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetShaderResources(self->stages[2], s, c, out); }
// VMA: 0x26d4cd0 (37 B)
void CGContextStateShadowGnm_Vma26d4cd0_GSSetShader(CGContextStateShadowGnm* self, void* gs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_GSGetConstantBuffersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d4cd0_GSSetShader(CGContextStateShadowGnm* self, void* gs) {
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167d00_bridge)(self, &self->input_layout, gs);
    self->stages[2].shader = gs;
}
// VMA: 0x26d4d00 (34 B)
void CGContextStateShadowGnm_Vma26d4d00_GSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_GSSetShaderEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d4d00_GSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) {
    Stage_GetConstantBuffers(self->stages[2], s, c, out);
}
// VMA: 0x26d4c70 (81 B)
void CGContextStateShadowGnm_GSSetSamplers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167cf0_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetSamplers(self->stages[2], s, c, in);
}
// VMA: 0x26d4c10 (81 B)
void CGContextStateShadowGnm_GSSetShaderResources(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167ce0_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetShaderResources(self->stages[2], s, c, in);
}
// VMA: 0x26d4bb0 (81 B)
void CGContextStateShadowGnm_GSSetConstantBuffers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167cd0_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetConstantBuffers(self->stages[2], s, c, in);
}

// --- PS Stage (stages[1], base +0x0ab8, 0x26d49c0 .. 0x26d4ba0) ---

// VMA: 0x26d4ba0 (16 B)
void CGContextStateShadowGnm_PSGetShader(const CGContextStateShadowGnm* self, void** out_ps) {
    if (out_ps) *out_ps = self->stages[1].shader;
}
// VMA: 0x26d4b70 (34 B)
void CGContextStateShadowGnm_PSGetSamplers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetSamplers(self->stages[1], s, c, out); }
// VMA: 0x26d4b40 (34 B)
void CGContextStateShadowGnm_PSGetShaderResources(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetShaderResources(self->stages[1], s, c, out); }
// VMA: 0x26d4ae0 (37 B)
void CGContextStateShadowGnm_Vma26d4ae0_PSSetShader(CGContextStateShadowGnm* self, void* ps)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_PSGetConstantBuffersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d4ae0_PSSetShader(CGContextStateShadowGnm* self, void* ps) {
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167cc0_bridge)(self, &self->input_layout, ps);
    self->stages[1].shader = ps;
}
// VMA: 0x26d4b10 (34 B)
void CGContextStateShadowGnm_Vma26d4b10_PSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_PSSetShaderEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d4b10_PSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) {
    Stage_GetConstantBuffers(self->stages[1], s, c, out);
}
// VMA: 0x26d4a80 (81 B)
void CGContextStateShadowGnm_PSSetSamplers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167cb0_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetSamplers(self->stages[1], s, c, in);
}
// VMA: 0x26d4a20 (81 B)
void CGContextStateShadowGnm_PSSetShaderResources(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167ca0_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetShaderResources(self->stages[1], s, c, in);
}
// VMA: 0x26d49c0 (81 B)
void CGContextStateShadowGnm_PSSetConstantBuffers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167c90_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetConstantBuffers(self->stages[1], s, c, in);
}

// --- VS Stage (stages[0], base +0x05c0, 0x26d47d0 .. 0x26d49b0) ---

// VMA: 0x26d49b0 (16 B)
void CGContextStateShadowGnm_VSGetShader(const CGContextStateShadowGnm* self, void** out_vs) {
    if (out_vs) *out_vs = self->stages[0].shader;
}
// VMA: 0x26d4980 (34 B)
void CGContextStateShadowGnm_VSGetSamplers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetSamplers(self->stages[0], s, c, out); }
// VMA: 0x26d4950 (34 B)
void CGContextStateShadowGnm_VSGetShaderResources(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) { Stage_GetShaderResources(self->stages[0], s, c, out); }
// VMA: 0x26d48f0 (37 B)
void CGContextStateShadowGnm_Vma26d48f0_VSSetShader(CGContextStateShadowGnm* self, void* vs)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm44CGContextStateShadowGnm_VSGetConstantBuffersEPKNS1_23CGContextStateShadowGnmEjjPPv");
void CGContextStateShadowGnm_Vma26d48f0_VSSetShader(CGContextStateShadowGnm* self, void* vs) {
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167c80_bridge)(self, &self->input_layout, vs);
    self->stages[0].shader = vs;
}
// VMA: 0x26d4920 (34 B)
void CGContextStateShadowGnm_Vma26d4920_VSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm35CGContextStateShadowGnm_VSSetShaderEPNS1_23CGContextStateShadowGnmEPv");
void CGContextStateShadowGnm_Vma26d4920_VSGetConstantBuffers(const CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void** out) {
    Stage_GetConstantBuffers(self->stages[0], s, c, out);
}
// VMA: 0x26d4890 (81 B)
void CGContextStateShadowGnm_VSSetSamplers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167c70_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetSamplers(self->stages[0], s, c, in);
}
// VMA: 0x26d4830 (81 B)
void CGContextStateShadowGnm_VSSetShaderResources(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167c60_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetShaderResources(self->stages[0], s, c, in);
}
// VMA: 0x26d47d0 (81 B)
void CGContextStateShadowGnm_VSSetConstantBuffers(CGContextStateShadowGnm* self, uint32_t s, uint32_t c, void* const* in) {
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*)>(&sub_2167c50_bridge)(
        self, &self->input_layout, s, c, in);
    Stage_SetConstantBuffers(self->stages[0], s, c, in);
}

// --- IA Stage & Context Base (0x26d4550 .. 0x26d4730) ---

// VMA: 0x26d4730 (151 B)
void CGContextStateShadowGnm_IAGetVertexBuffers(
    const CGContextStateShadowGnm* self,
    uint32_t start_slot,
    uint32_t num_buffers,
    void** out_vbs,
    uint32_t* out_strides,
    uint32_t* out_offsets)
{
    if (out_vbs) std::memcpy(out_vbs + start_slot, &self->vertex_buffers[start_slot], num_buffers * sizeof(void*));
    if (out_strides) std::memcpy(out_strides + start_slot, &self->vertex_strides[start_slot], num_buffers * sizeof(uint32_t));
    if (out_offsets) std::memcpy(out_offsets + start_slot, &self->vertex_offsets[start_slot], num_buffers * sizeof(uint32_t));
}

// VMA: 0x26d4700 (33 B)
void CGContextStateShadowGnm_IAGetIndexBuffer(
    const CGContextStateShadowGnm* self,
    void** out_ib,
    uint32_t* out_format,
    uint32_t* out_offset)
{
    if (out_ib) *out_ib = self->index_buffer;
    if (out_format) *out_format = self->index_format;
    if (out_offset) *out_offset = self->index_offset;
}

// VMA: 0x26d46f0 (11 B)
void CGContextStateShadowGnm_IAGetPrimitiveTopology(const CGContextStateShadowGnm* self, uint32_t* out_topo) {
    if (out_topo) *out_topo = self->primitive_topology;
}

// VMA: 0x26d46e0 (13 B)
void CGContextStateShadowGnm_IAGetInputLayout(const CGContextStateShadowGnm* self, void** out_layout) {
    if (out_layout) *out_layout = self->input_layout;
}

// VMA: 0x26d4640 (145 B)
void CGContextStateShadowGnm_IASetVertexBuffers(
    CGContextStateShadowGnm* self,
    uint32_t start_slot,
    uint32_t num_buffers,
    void* const* vbs,
    const uint32_t* strides,
    const uint32_t* offsets)
{
    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t, void* const*, const uint32_t*, const uint32_t*)>(&sub_2167c30_bridge)(
        self, &self->input_layout, start_slot, num_buffers, vbs, strides, offsets);
    for (uint32_t i = 0; i < num_buffers; ++i) {
        uint32_t slot = start_slot + i;
        self->vertex_buffers[slot] = vbs[i];
        self->vertex_strides[slot] = strides[i];
        self->vertex_offsets[slot] = offsets[i];
    }
}

// VMA: 0x26d4600 (62 B)
void CGContextStateShadowGnm_IASetIndexBuffer(
    CGContextStateShadowGnm* self,
    void* ib,
    uint32_t format,
    uint32_t offset)
{
    reinterpret_cast<void (*)(void*, void*, void*, uint32_t, uint32_t)>(&sub_2167c20_bridge)(
        self, &self->input_layout, ib, format, offset);
    self->index_buffer = ib;
    self->index_format = format;
    self->index_offset = offset;
}

// VMA: 0x26d45d0 (34 B)
void CGContextStateShadowGnm_IASetPrimitiveTopology(CGContextStateShadowGnm* self, uint32_t topology) {
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(&sub_2167c10_bridge)(
        self, &self->input_layout, topology);
    self->primitive_topology = topology;
}

// VMA: 0x26d45a0 (34 B)
void CGContextStateShadowGnm_IASetInputLayout(CGContextStateShadowGnm* self, void* layout) {
    reinterpret_cast<void (*)(void*, void*, void*)>(&sub_2167c00_bridge)(
        self, &self->input_layout, layout);
    self->input_layout = layout;
}

// VMA: 0x26d4580 (28 B)
void CGContextStateShadowGnm_DestroyBase(CGContextStateShadowGnm* self) {
    reinterpret_cast<void (*)(void*)>(&sub_2167bf0_bridge)(self);
    reinterpret_cast<void (*)(void*)>(&sub_26ab7e0_bridge)(self);
}

// VMA: 0x26d4550 (40 B)
void CGContextStateShadowGnm_ConstructBase(CGContextStateShadowGnm* self) {
    reinterpret_cast<void (*)(void*)>(&sub_26ab7f0_bridge)(self);
    std::memset(&self->input_layout, 0, 0x23a8);
}

} // namespace Bloodborne::CoreGraphics2::Gnm
