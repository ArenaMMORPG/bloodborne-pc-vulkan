// ============================================================================
// Bloodborne (CUSA03173 v1.09) - CoreGraphics2 GNM Backend
// Module: 01_CoreGraphics2_GNM
// Range:  0x26b9850 .. 0x26bba30 (43 functions)
//
// Reconstructed classes & subsystems:
//   1. `CGIndexBufferCacheGnm` (`0x26bba10 .. 0x26bba30`):
//      16-byte hardware index buffer & primitive topology state cache (`setPrimitiveType`,
//      `setIndexBuffer`, `setIndexSize`).
//   2. `CGCommandListChainGnm` & End-Of-Pipe Labels (`0x26bb6c0 .. 0x26bb8e0`):
//      Walks linked command buffer chunks, gathers DCB pointers/sizes, and submits them via
//      `sceGnmSubmitCommandBuffers` (`0x1089a10`), plus EOP label writes (`writeAtEndOfPipe` `0x10778f0`).
//   3. `CGShaderBinaryGnm` & `CGTextureGnm` View Builders (`0x26bad70 .. 0x26bb670`):
//      Validates `"Shdr"` magic (`0x72646853`), allocates 256-byte aligned GPU memory for shader
//      microcode, queries Orbis kernel memory protection (`sceKernelQueryMemoryProtection` `0x2bbf538`
//      & `sceKernelVirtualQuery` `0x2bbf548`), and builds `sce::Gnm::Texture` (`T#`) subresource views.
//   4. `CGShaderFactoryGnm` (`0x26b9c00 .. 0x26baac0`):
//      6-stage shader factory (`CS`, `PS`, `VS`, `GS`, `HS`, `DS`) parsing OrbShdr containers
//      (`0x10a3b70`, `0x10a3c40`), building `CGShaderBindingHeaderGnm` (`0x2175480`, `0x21754e0`),
//      patching 256-byte aligned GPU ucode base addresses, and resolving semantic tables (`0x26b7e60`).
//   5. `CGSurfaceSyncTrackerGnm` (`0x26b9850 .. 0x26b9a60`):
//      38-byte (`0x26`-byte) GPU surface/cache synchronization range accumulator emitting
//      `waitForGraphicsWrites` (`0x1075e10`), `flushShaderCachesAndWait` (`0x26b09e0`), and
//      `triggerEvent` (`0x1077f40`).
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

#include "gx_direct_vulkan.h"

namespace Bloodborne::CoreGraphics2::Gnm {

// ============================================================================
// External GNM SDK & Engine Helpers
// ============================================================================

extern "C" {
uint32_t sub_26b7b60_TranslatePrimitiveTopology(uint32_t topology); // 0x26b7b60
void sub_1073fe0_setPrimitiveType(void* gnmx_ctx, uint32_t prim_type); // 0x1073fe0
void sub_10752c0_setIndexBuffer(void* gnmx_ctx, const void* index_addr); // 0x10752c0
void sub_1075270_setIndexSize(void* gnmx_ctx, uint32_t index_size); // 0x1075270
const uint8_t* CGDynamicBufferRingGnm_ResolveGpuAddress(void* ring, const void* buf); // 0x26d0ff0

void sub_1071270_initDrawCommandBuffer(void* dcb, void* buf, uint32_t size, void* cb, void* ud); // 0x1071270
void sub_10783c0_insertNop(void* dcb, uint32_t num_dwords); // 0x10783c0
int32_t sub_1089a30_sceGnmValidateCommandBuffers(uint32_t count, void* dcb_addrs, void* dcb_sizes, void* ccb_addrs, void* ccb_sizes); // 0x1089a30
int32_t sub_1089a10_sceGnmSubmitCommandBuffers(uint32_t count, void* dcb_addrs, void* dcb_sizes, void* ccb_addrs, void* ccb_sizes); // 0x1089a10
void sub_10778f0_writeAtEndOfPipe(void* gnmx_ctx, uint32_t event_type, uint32_t dst_sel, void* dst_gpu_addr, uint32_t val_sel, uint64_t imm_val, uint32_t int_sel, uint32_t wr_confirm); // 0x10778f0
void sub_1072870_waitOnAddress(void* gnmx_ctx, void* gpu_addr, uint32_t mask, uint32_t cmp_func, uint32_t ref_val); // 0x1072870
void sub_1070960_initAsConstantBuffer(void* out_vsharp, const void* base_addr, uint32_t size_bytes); // 0x1070960

void sub_109c3e0_parseShaderBinaryInfo(void* out_info, const void* shdr_header); // 0x109c3e0
int32_t sub_2bbf538_sceKernelQueryMemoryProtection(const void* addr, void** start, void** end, uint32_t* prot); // 0x2bbf538
int32_t sub_2bbf548_sceKernelVirtualQuery(const void* addr, int32_t flags, void* info, size_t info_size); // 0x2bbf548
int32_t sub_2bbf558_sceKernelGetDirectMemoryType(uint64_t phys_addr, int32_t* out_type, uint64_t* out_start, uint64_t* out_end); // 0x2bbf558
void* sub_261a1b0_memcpy(void* dst, const void* src, size_t size); // 0x261a1b0
uint32_t sub_207ebd0_AtomicIncrement(volatile uint32_t* ptr); // 0x207ebd0

void sub_10a3b70_ShaderContainer_Construct(void* container); // 0x10a3b70
int32_t sub_10a3c40_ShaderContainer_Parse(void* container, const void* data, uint32_t size); // 0x10a3c40
void sub_10a3bf0_ShaderContainer_Destroy(void* container); // 0x10a3bf0
int32_t sub_2175480_CalcShaderBindingHeaderSize(uint64_t* out_size, uint32_t* out_count, const void* hw_shdr, uint32_t num_slots); // 0x2175480
int32_t sub_21754e0_BuildShaderBindingHeader(void* out_hdr_ptr, void* hdr_mem, uint32_t slot_count, uint32_t stage_flags, const void* container, const void* hw_shdr, uint32_t num_slots, void* alloc); // 0x21754e0
inline bool sub_26b7e60_ResolveSemanticName(uint32_t* out_semantic_id, const char* semantic_str) {
    return reinterpret_cast<bool (*)(uint32_t*, const char*)>(GX_VMA(0x26b7e60ULL))(out_semantic_id, semantic_str);
}

inline void* sub_207d5b0_GetTlsTempAlloc() {
    return reinterpret_cast<void* (*)()>(GX_VMA(0x207d5b0ULL))();
}
inline void* sub_207d510_TempAllocAligned(void* temp_alloc, size_t size, size_t align) {
    return reinterpret_cast<void* (*)(void*, size_t, size_t)>(GX_VMA(0x207d510ULL))(temp_alloc, size, align);
}
inline void sub_207d5f0_TempAllocRestore(void* tls) {
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x207d5f0ULL))(tls);
}

inline int32_t CGTranslatorGnm_TranslateEngineFormatToGnmDataFormat(uint32_t* out_gnm_fmt, uint32_t engine_fmt) {
    return reinterpret_cast<int32_t (*)(uint32_t*, uint32_t)>(GX_VMA(0x26b7310ULL))(out_gnm_fmt, engine_fmt);
}
inline int32_t CGTranslatorGnm_TranslateMetadataDataFormat(uint32_t* out_gnm_fmt, uint32_t engine_fmt) {
    return reinterpret_cast<int32_t (*)(uint32_t*, uint32_t)>(GX_VMA(0x26b7960ULL))(out_gnm_fmt, engine_fmt);
}
inline uint32_t CGTranslatorGnm_TranslateNumFragmentsAndSamples(uint32_t num_samples, uint32_t num_fragments, bool is_eqaa) {
    return reinterpret_cast<uint32_t (*)(uint32_t, uint32_t, bool)>(GX_VMA(0x26b7ab0ULL))(num_samples, num_fragments, is_eqaa);
}
inline uint32_t sub_2165130_CalcDefaultMipCount(uint32_t w, uint32_t h, uint32_t d) {
    return reinterpret_cast<uint32_t (*)(uint32_t, uint32_t, uint32_t)>(GX_VMA(0x2165130ULL))(w, h, d);
}
inline uint32_t sub_2164d60_CalcMipLevel(uint32_t subres_idx, uint32_t mip_count) {
    return reinterpret_cast<uint32_t (*)(uint32_t, uint32_t)>(GX_VMA(0x2164d60ULL))(subres_idx, mip_count);
}
inline uint32_t sub_2164d50_CalcArraySlice(uint32_t subres_idx, uint32_t mip_count) {
    return reinterpret_cast<uint32_t (*)(uint32_t, uint32_t)>(GX_VMA(0x2164d50ULL))(subres_idx, mip_count);
}
inline uint64_t sub_1087ee0_initAs2d(void* tsharp, uint32_t width, uint32_t height, uint32_t num_slices, uint32_t num_mips, uint32_t data_fmt, uint32_t tile_mode_hint, uint32_t num_fragments, uint32_t is_cubemap) {
    return reinterpret_cast<uint64_t (*)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1087ee0ULL))(tsharp, width, height, num_slices, num_mips, data_fmt, tile_mode_hint, num_fragments, is_cubemap);
}
inline void sub_1088370_setResourceMemoryType(void* tsharp, uint32_t mem_type) {
    reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x1088370ULL))(tsharp, mem_type);
}
inline uint64_t sub_1088590_initAs3d(void* tsharp, uint32_t width, uint32_t height, uint32_t depth, uint32_t num_mips, uint32_t data_fmt, uint32_t tile_mode_hint) {
    return reinterpret_cast<uint64_t (*)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1088590ULL))(tsharp, width, height, depth, num_mips, data_fmt, tile_mode_hint);
}
inline int32_t sub_10a2c40_computeSurfaceOffsetAndSize(uint64_t* out_offset, uint64_t* out_size, const void* tsharp, uint32_t mip_level, uint32_t array_slice) {
    return reinterpret_cast<int32_t (*)(uint64_t*, uint64_t*, const void*, uint32_t, uint32_t)>(GX_VMA(0x10a2c40ULL))(out_offset, out_size, tsharp, mip_level, array_slice);
}

void sub_26b09e0_FlushShaderCachesAndWait(void* gnmx_ctx, uint32_t cache_action, uint32_t extended_action); // 0x26b09e0
void sub_1075e10_waitForGraphicsWrites(void* gnmx_ctx, uint32_t base_addr_256, uint32_t size_256, uint32_t target_mask, uint32_t cache_action, uint32_t extended_cache_mask, uint32_t stall_cmd); // 0x1075e10
void sub_1077f40_triggerEvent(void* gnmx_ctx, uint32_t event_type, uint32_t cache_action, uint32_t stall); // 0x1077f40
}

// ============================================================================
// 1. CGIndexBufferCacheGnm (0x26bba10 .. 0x26bba30)
// ============================================================================

struct CGIndexBufferCacheGnm {
    const void* bound_index_buffer_gpu_addr; // +0x00
    uint32_t    bound_primitive_type;        // +0x08
    uint32_t    bound_index_size;            // +0x0c (0 = 16-bit, 1 = 32-bit, 0xFFFFFFFF = invalid)
};

// VMA: 0x26bba30 (219 B)
// Binds primitive topology (`[state+0x08]`), index buffer GPU address (`[state+0x10]` + `[state+0x1c]`),
// and index element size (`[state+0x18] == 0x2a` -> 32-bit else 16-bit) to `Gnmx::GfxContext`.
void CGIndexBufferCacheGnm_BindToGfxContext(
    CGIndexBufferCacheGnm* self,
    uint8_t* gnmx_ctx_wrapper,
    const uint8_t* pipeline_state,
    bool bind_index_buffer)
{
    void* gnmx_ctx = gnmx_ctx_wrapper + 0x10;
    const uint32_t prim_type = sub_26b7b60_TranslatePrimitiveTopology(
        *reinterpret_cast<const uint32_t*>(pipeline_state + 0x08));

    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_IndexAndPrim)) {
        gx_direct_vk::SetIndexStateArgs args{};
        if (self->bound_primitive_type != prim_type) {
            args.prim_type = prim_type;
            args.index_and_flags |= gx_direct_vk::kIndexFlag_SetPrim;
            self->bound_primitive_type = prim_type;
        }

        if (!bind_index_buffer) {
            self->bound_index_buffer_gpu_addr = nullptr;
            self->bound_index_size = 0xFFFFFFFFu;
            gx_direct_vk::EmitSetIndexState(gnmx_ctx, args);
            return;
        }

        const uint8_t* ib_obj = *reinterpret_cast<const uint8_t* const*>(pipeline_state + 0x10);
        if (ib_obj == nullptr) {
            self->bound_index_buffer_gpu_addr = nullptr;
            gx_direct_vk::EmitSetIndexState(gnmx_ctx, args);
            return;
        }

        const uint8_t* gpu_addr = nullptr;
        const uint32_t byte_offset = *reinterpret_cast<const uint32_t*>(pipeline_state + 0x1c);
        if (ib_obj[0x40] != 0) {
            gpu_addr = CGDynamicBufferRingGnm_ResolveGpuAddress(
                           gnmx_ctx_wrapper + 0xb6f8,
                           ib_obj + 0x18) + byte_offset;
        } else {
            gpu_addr = *reinterpret_cast<const uint8_t* const*>(ib_obj + 0x20) + byte_offset;
        }

        if (self->bound_index_buffer_gpu_addr != gpu_addr) {
            args.ib_va = reinterpret_cast<uint64_t>(gpu_addr);
            args.index_and_flags |= gx_direct_vk::kIndexFlag_SetIbAddr;
            self->bound_index_buffer_gpu_addr = gpu_addr;
        }

        const uint32_t index_fmt = *reinterpret_cast<const uint32_t*>(pipeline_state + 0x18);
        const uint32_t index_size = (index_fmt == 0x2Au) ? 1u : 0u;
        if (self->bound_index_size != index_size) {
            args.index_and_flags |= (index_size & 0xFFu) | gx_direct_vk::kIndexFlag_SetIndexSize;
            self->bound_index_size = index_size;
        }

        gx_direct_vk::EmitSetIndexState(gnmx_ctx, args);
        return;
    }

    if (self->bound_primitive_type != prim_type) {
        sub_1073fe0_setPrimitiveType(gnmx_ctx, prim_type);
        self->bound_primitive_type = prim_type;
    }

    if (!bind_index_buffer) {
        self->bound_index_buffer_gpu_addr = nullptr;
        self->bound_index_size = 0xFFFFFFFFu;
        return;
    }

    const uint8_t* ib_obj = *reinterpret_cast<const uint8_t* const*>(pipeline_state + 0x10);
    if (ib_obj == nullptr) {
        self->bound_index_buffer_gpu_addr = nullptr;
        return;
    }

    const uint8_t* gpu_addr = nullptr;
    const uint32_t byte_offset = *reinterpret_cast<const uint32_t*>(pipeline_state + 0x1c);
    if (ib_obj[0x40] != 0) {
        gpu_addr = CGDynamicBufferRingGnm_ResolveGpuAddress(
                       gnmx_ctx_wrapper + 0xb6f8,
                       ib_obj + 0x18) + byte_offset;
    } else {
        gpu_addr = *reinterpret_cast<const uint8_t* const*>(ib_obj + 0x20) + byte_offset;
    }

    if (self->bound_index_buffer_gpu_addr != gpu_addr) {
        sub_10752c0_setIndexBuffer(gnmx_ctx, gpu_addr);
        self->bound_index_buffer_gpu_addr = gpu_addr;
    }

    const uint32_t index_fmt = *reinterpret_cast<const uint32_t*>(pipeline_state + 0x18);
    const uint32_t index_size = (index_fmt == 0x2Au) ? 1u : 0u;
    if (self->bound_index_size != index_size) {
        sub_1075270_setIndexSize(gnmx_ctx, index_size);
        self->bound_index_size = index_size;
    }
}

// VMA: 0x26bba10 (22 B)
// Resets cached index buffer address, primitive type, and index size.
void CGIndexBufferCacheGnm_Reset(CGIndexBufferCacheGnm* self) {
    self->bound_index_buffer_gpu_addr = nullptr;
    self->bound_primitive_type = 0;
    self->bound_index_size = 0xFFFFFFFFu;
}

// ============================================================================
// 2. CGCommandListChainGnm & End-Of-Pipe Labels (0x26bb6c0 .. 0x26bb8e0)
// ============================================================================

// VMA: 0x26bb8e0 (146 B)
// Builds a tiny 16-byte NOP command buffer on the stack and calls `sceGnmValidateCommandBuffers`
// (`0x1089a30`), returning true if result != `0x80D13FFF`.
bool CGCommandBufferGnm_ValidateTestSubmit() {
    alignas(16) uint8_t cmd_buf[16] = {};
    uint8_t dcb[64] = {};
    void* dcb_addr = cmd_buf;
    sub_1071270_initDrawCommandBuffer(dcb, cmd_buf, 16, nullptr, nullptr);
    sub_10783c0_insertNop(dcb, 1);
    uint32_t dcb_byte_size = static_cast<uint32_t>(
        *reinterpret_cast<uintptr_t*>(dcb + 0x10) - reinterpret_cast<uintptr_t>(cmd_buf));
    int32_t res = sub_1089a30_sceGnmValidateCommandBuffers(1, &dcb_addr, &dcb_byte_size, nullptr, nullptr);
    return res != static_cast<int32_t>(0x80D13FFFu);
}

// VMA: 0x26bb8d0 (1 B)
void CGCommandBufferGnm_NoOp() {}

struct CGCommandListChainGnm {
    void*    allocator;         // +0x00
    uint8_t* head_chunk;        // +0x08
    uint64_t reserved_10;       // +0x10
    uint32_t entries_per_chunk; // +0x18
    uint32_t tail_entry_count;  // +0x1c
};

// VMA: 0x26bb730 (403 B)
// Walks the linked list of command buffer chunks (`+0x08`), extracts non-empty DCB base pointers
// and byte sizes (`[entry+0x10] - [entry+0x00]`), and submits them all in one batch via
// `sceGnmSubmitCommandBuffers` (`0x1089a10`).
void CGCommandListChainGnm_SubmitAllChunks(CGCommandListChainGnm* self) {
    if (self->head_chunk == nullptr) {
        return;
    }
    uint32_t total_entries = 0;
    {
        const uint32_t cap = self->entries_per_chunk;
        const uint64_t next_ptr_offset = static_cast<uint64_t>(cap) << 6;
        for (uint8_t* chunk = *reinterpret_cast<uint8_t**>(self->head_chunk + next_ptr_offset);
             chunk != nullptr;
             chunk = *reinterpret_cast<uint8_t**>(chunk + next_ptr_offset)) {
            total_entries += cap;
        }
        total_entries += self->tail_entry_count;
    }

    struct TempAllocScope {
        uint64_t vtable;
        void*    tls;
    };
    const uint64_t vtbl_addr = GX_VMA(0x52b49e0ULL) + 0x10ULL;
    TempAllocScope temp_alloc{ vtbl_addr, sub_207d5b0_GetTlsTempAlloc() };

    const size_t alloc_bytes = static_cast<size_t>(total_entries) * 12u;
    uint8_t* buf = static_cast<uint8_t*>(sub_207d510_TempAllocAligned(&temp_alloc, alloc_bytes, 8));
    void* fallback_alloc = nullptr;
    if (self->allocator != nullptr && buf == nullptr) {
        using AllocAlignedFn = void* (*)(void*, size_t, size_t);
        void** vtbl = *reinterpret_cast<void***>(self->allocator);
        buf = static_cast<uint8_t*>(reinterpret_cast<AllocAlignedFn>(vtbl[0x58 / 8])(self->allocator, alloc_bytes, 8));
        fallback_alloc = self->allocator;
    }

    if (self->head_chunk != nullptr && buf != nullptr) {
        void** dcb_addrs = reinterpret_cast<void**>(buf);
        uint32_t* dcb_sizes = reinterpret_cast<uint32_t*>(buf + (static_cast<size_t>(total_entries) * 8u));
        const uint32_t cap = self->entries_per_chunk;
        const uint64_t next_ptr_offset = static_cast<uint64_t>(cap) << 6;
        uint32_t valid_count = 0;

        for (uint8_t* chunk = self->head_chunk; chunk != nullptr;) {
            uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_ptr_offset);
            const uint32_t count = (next_chunk != nullptr) ? cap : self->tail_entry_count;
            for (uint32_t i = 0; i < count; ++i) {
                const uint8_t* entry = chunk + (static_cast<size_t>(i) << 6);
                void* base_ptr = *reinterpret_cast<void* const*>(entry + 0x00);
                uint32_t cmd_ptr_lo = *reinterpret_cast<const uint32_t*>(entry + 0x10);
                uint32_t base_ptr_lo = *reinterpret_cast<const uint32_t*>(entry + 0x00);
                uint32_t size_bytes = cmd_ptr_lo - base_ptr_lo;
                dcb_addrs[valid_count] = base_ptr;
                dcb_sizes[valid_count] = size_bytes;
                if (size_bytes >= 1u) {
                    ++valid_count;
                }
            }
            chunk = next_chunk;
        }

        if (valid_count != 0) {
            sub_1089a10_sceGnmSubmitCommandBuffers(valid_count, dcb_addrs, dcb_sizes, nullptr, nullptr);
        }
    }

    if (fallback_alloc != nullptr && buf != nullptr) {
        using FreeFn = void (*)(void*, void*);
        void** vtbl = *reinterpret_cast<void***>(fallback_alloc);
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(fallback_alloc, buf);
    }
    temp_alloc.vtable = vtbl_addr;
    sub_207d5f0_TempAllocRestore(temp_alloc.tls);
}

// VMA: 0x26bb700 (47 B)
// Writes a 32-bit GPU timestamp/counter (`val_sel = 3`) at End-of-Pipe (`0x10778f0`).
void CGCommandBufferGnm_WriteEndOfPipeCounter(
    void* gnmx_ctx,
    uint32_t event_type,
    void* dst_gpu_addr,
    uint32_t cache_action)
{
    sub_10778f0_writeAtEndOfPipe(gnmx_ctx, event_type, 0, dst_gpu_addr, 3, 0, cache_action, 0);
}

// VMA: 0x26bb6d0 (48 B)
// Writes a 64-bit immediate value (`val_sel = 2`) at End-of-Pipe (`0x10778f0`).
void CGCommandBufferGnm_WriteEndOfPipeImmediate(
    void* gnmx_ctx,
    uint32_t event_type,
    void* dst_gpu_addr,
    uint64_t imm_value,
    uint32_t cache_action)
{
    sub_10778f0_writeAtEndOfPipe(gnmx_ctx, event_type, 0, dst_gpu_addr, 2, imm_value, cache_action, 0);
}

// VMA: 0x26bb6c0 (5 B)
// Tail-jump wrapper to `DrawCommandBuffer::initializeDefaultHardwareState` (`0x1072870`).
void CGCommandBufferGnm_WaitOnAddress(
    void* gnmx_ctx,
    void* gpu_addr,
    uint32_t mask,
    uint32_t cmp_func,
    uint32_t ref_val)
{
    if (gx_direct_vk::DirectRingEnabled()) {
        gx_direct_vk::EmitInitDefaultHardwareState(gnmx_ctx, 3u);
        return;
    }
    sub_1072870_waitOnAddress(gnmx_ctx, gpu_addr, mask, cmp_func, ref_val);
}

// ============================================================================
// 3. CGShaderBinaryGnm & CGTextureGnm View Builders (0x26bad70 .. 0x26bb670)
// ============================================================================

// VMA: 0x26bb670 (70 B)
// Builds a 16-byte `sce::Gnm::Buffer` (`V#`) constant buffer descriptor from a shader embedded
// constant buffer slot (`[slot+0x00]` 23-bit offset, `[slot+0x04]` 16-byte count).
bool CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
    uint64_t* out_vsharp,
    const uint8_t* slot_desc,
    uint32_t shader_base_hi,
    uint32_t shader_base_lo)
{
    const uint16_t num_vec4 = *reinterpret_cast<const uint16_t*>(slot_desc + 0x04);
    if (num_vec4 == 0) {
        out_vsharp[0] = 0;
        out_vsharp[1] = 0;
        return false;
    }
    const uint64_t shader_base =
        ((static_cast<uint64_t>(shader_base_hi) << 32) | shader_base_lo) << 8;
    const uint64_t offset = *reinterpret_cast<const uint32_t*>(slot_desc + 0x00) & 0x7FFFFFu;
    sub_1070960_initAsConstantBuffer(
        out_vsharp,
        reinterpret_cast<const void*>(shader_base + offset),
        static_cast<uint32_t>(num_vec4) << 4);
    return true;
}

// VMA: 0x26bb570 (253 B)
// Validates `"Shdr"` magic (`0x72646853` at `[shdr_hdr + 0x24]`) and expected stage type (`r9d`),
// parses `Gnmx::ShaderInfo` (`0x109c3e0`), allocates 256-byte aligned GPU memory via `allocator`,
// verifies GPU read protection (`0x10`), and copies the shader ucode binary (`0x261a1b0`).
int32_t CGShaderBinaryGnm_AllocateAndUploadUcode(
    void** out_gpu_ucode_ptr,
    void** out_shader_struct_ptr,
    uint32_t unused_edx,
    const uint8_t* shdr_header,
    void* allocator,
    uint32_t expected_shader_type)
{
    if (*reinterpret_cast<const uint32_t*>(shdr_header + 0x24) != 0x72646853u) { // "Shdr"
        return 8;
    }
    if (shdr_header[0x2c] != expected_shader_type) {
        return 8;
    }

    struct ShaderBinaryInfo {
        void*    shader_struct; // +0x00 (`rbp-0x50`)
        const void* ucode_src;  // +0x08 (`rbp-0x48`)
        uint32_t ucode_size;    // +0x10 (`rbp-0x40`)
        uint32_t reserved;      // +0x14
    } info = {};
    sub_109c3e0_parseShaderBinaryInfo(&info, shdr_header);

    using AllocFn = void* (*)(void*, size_t, size_t);
    using FreeFn  = void (*)(void*, void*);
    void** vtbl = *reinterpret_cast<void***>(allocator);
    void* gpu_mem = reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(allocator, info.ucode_size, 0x100);
    if (gpu_mem == nullptr) {
        return 9;
    }

    uint32_t prot = 0;
    if (sub_2bbf538_sceKernelQueryMemoryProtection(gpu_mem, nullptr, nullptr, &prot) != 0 ||
        (prot & 0x10u) == 0) {
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(allocator, gpu_mem);
        return 8;
    }

    *out_gpu_ucode_ptr = gpu_mem;
    uint32_t cpu_prot = 0;
    if (sub_2bbf538_sceKernelQueryMemoryProtection(gpu_mem, nullptr, nullptr, &cpu_prot) == 0 &&
        (cpu_prot & 0x01u) != 0) {
        if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectBuffers) &&
            info.ucode_size > 0u) {
            gx_direct_vk::PrepareDirectBufferWrite(gpu_mem, info.ucode_size);
            gx_direct_vk::NoteDirectAssetBufferWrite(gpu_mem, info.ucode_size);
            std::memcpy(gpu_mem, info.ucode_src, info.ucode_size);
            gx_direct_vk::NoteDirectAssetBufferWrite(gpu_mem, info.ucode_size);
            if (gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers)) {
                gx_direct_vk::RegisterDirectImmutableBuffer(
                    0u, gpu_mem, info.ucode_size, info.ucode_size, 4u);
            }
        } else {
            sub_261a1b0_memcpy(gpu_mem, info.ucode_src, info.ucode_size);
        }
        gx_direct_vk::RegisterDirectShaderBinary(
            gpu_mem, info.ucode_size, shdr_header, 0u, expected_shader_type);
    }
    *out_shader_struct_ptr = info.shader_struct;
    return 0;
}

// VMA: 0x26bb550 (28 B)
// Returns string name for `sce::Gnmx::ShaderType` (`0..6`).
const char* CGShaderBinaryGnm_GetShaderTypeName(uint32_t shader_type) {
    static const char* const kNames[7] = {
        "Invalid", "VsShader", "PsShader", "CsShader", "GsShader", "HsShader", "DsShader"
    };
    return (shader_type <= 6u) ? kNames[shader_type] : "Unknown";
}

// VMA: 0x26bb530 (28 B)
// Returns string name for `sce::Gnm::ShaderStage` (`0..7`).
const char* CGShaderBinaryGnm_GetShaderStageName(uint32_t shader_stage) {
    static const char* const kNames[8] = {
        "CsStage", "PsStage", "VsStage", "GsStage", "EsStage", "HsStage", "LsStage", "Invalid"
    };
    return (shader_stage <= 7u) ? kNames[shader_stage] : "Unknown";
}

// VMA: 0x26bb510 (25 B)
bool CGShaderBinaryGnm_IsConstantBufferOrTypedSlot(uint32_t flags, uint32_t slot_kind) {
    if (slot_kind == 0x14u) return true;
    if (slot_kind == 0x16u) return ((flags >> 6) & 1u) != 0;
    return false;
}

// VMA: 0x26bb4f0 (31 B)
bool CGShaderBinaryGnm_IsSrvOrUavSlot(uint32_t flags, uint32_t slot_kind) {
    if (slot_kind == 0x13u || slot_kind == 0x14u) return true;
    if (slot_kind == 0x15u || slot_kind == 0x16u) return ((flags >> 6) & 1u) != 0;
    return false;
}

// VMA: 0x26bb4b0 (52 B)
// Checks if a texture format ID (`0..0x59`) is valid for creating a subresource view.
bool CGTextureGnm_IsSupportedViewFormat(uint32_t format_id) {
    if (format_id == 0 || format_id == 0x59u) return false;
    if (format_id > 0x12u && format_id <= 0x45u) {
        const uint64_t invalid_mask = 0x000780001E00000FULL;
        if (((invalid_mask >> (format_id - 0x13u)) & 1ULL) != 0) {
            return false;
        }
    }
    return true;
}

// VMA: 0x26bb4a0 (1 B)
void CGTextureGnm_NoOp() {}

// VMA: 0x26baf60 (1340 B)
// Creates a subresource `T#` view (`out_gpu_offset`, `out_byte_size`) for a 1D, 2D, 3D, or Cube
// texture (`[tex_desc + 0x20]` dimension `1..4`) at a given `subresource_idx`.
int32_t CGTextureGnm_CreateSubresourceViewTSharp(
    uint64_t* out_byte_offset,
    uint64_t* out_byte_size,
    const uint8_t* tex_desc,
    uint32_t subresource_idx,
    bool include_metadata,
    bool allow_cubemap)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm40CGTextureGnm_CreateSubresourceViewTSharpEPmS2_PKhjb");
int32_t CGTextureGnm_CreateSubresourceViewTSharp(
    uint64_t* out_byte_offset,
    uint64_t* out_byte_size,
    const uint8_t* tex_desc,
    uint32_t subresource_idx,
    bool include_metadata,
    bool allow_cubemap)
{
    const uint8_t dim = tex_desc[0x20];
    if (dim < 1u || dim > 4u) {
        return 8;
    }

    switch (dim) {
    case 1: // Buffer
        if (subresource_idx != 0) {
            return 8;
        }
        *out_byte_offset = 0;
        *out_byte_size = *reinterpret_cast<const uint32_t*>(tex_desc + 0x14);
        return 0;

    case 2: { // 1D Texture
        const uint32_t array_slices = *reinterpret_cast<const uint16_t*>(tex_desc + 0x30);
        uint32_t mip_count = tex_desc[0x32];
        if (mip_count == 0) {
            mip_count = sub_2165130_CalcDefaultMipCount(
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c), 1, 1);
        }
        const uint32_t mip_level = sub_2164d60_CalcMipLevel(subresource_idx, mip_count);
        const uint32_t array_slice = sub_2164d50_CalcArraySlice(subresource_idx, mip_count);
        if (subresource_idx >= (mip_count * array_slices)) {
            return 8;
        }
        uint32_t gnm_fmt = 0;
        int32_t err = CGTranslatorGnm_TranslateEngineFormatToGnmDataFormat(&gnm_fmt, tex_desc[0x33]);
        if (err != 0) {
            return err;
        }
        uint8_t tsharp[32] = {};
        const uint64_t base_sa = sub_1087ee0_initAs2d(
            tsharp,
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
            1,
            array_slices,
            mip_count,
            gnm_fmt,
            *reinterpret_cast<const uint32_t*>(tex_desc + 0x28),
            0,
            0);
        sub_1088370_setResourceMemoryType(tsharp, 0x0Cu);

        uint64_t meta_offset = 0;
        if (include_metadata) {
            err = CGTranslatorGnm_TranslateMetadataDataFormat(&gnm_fmt, tex_desc[0x33]);
            if (err != 0) {
                return err;
            }
            const uint64_t meta_sa = sub_1087ee0_initAs2d(
                tsharp,
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
                1,
                array_slices,
                mip_count,
                gnm_fmt,
                *reinterpret_cast<const uint32_t*>(tex_desc + 0x28),
                0,
                0);
            sub_1088370_setResourceMemoryType(tsharp, 0x0Cu);
            const uint32_t align = static_cast<uint32_t>(meta_sa >> 32);
            meta_offset = (static_cast<uint32_t>(base_sa) + align - 1u) & ~(align - 1u);
        }

        uint64_t slice_off = 0, slice_sz = 0;
        if (sub_10a2c40_computeSurfaceOffsetAndSize(&slice_off, &slice_sz, tsharp, mip_level, array_slice) != 0) {
            return 8;
        }
        *out_byte_offset = meta_offset + slice_off;
        *out_byte_size = slice_sz;
        return 0;
    }

    case 3: { // 2D / Cube Texture
        uint32_t num_slices = *reinterpret_cast<const uint16_t*>(tex_desc + 0x30);
        uint32_t is_cubemap = 0;
        if (allow_cubemap && (tex_desc[0x24] & 0x04u) != 0) {
            num_slices = (num_slices * 0xAAABu) >> 18;
            is_cubemap = 1;
        }
        uint32_t mip_count = tex_desc[0x32];
        if (mip_count == 0) {
            mip_count = sub_2165130_CalcDefaultMipCount(
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2e),
                1);
        }
        const uint32_t mip_level = sub_2164d60_CalcMipLevel(subresource_idx, mip_count);
        const uint32_t array_slice = sub_2164d50_CalcArraySlice(subresource_idx, mip_count);
        const uint32_t total_subres =
            static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(tex_desc + 0x30)) * mip_count;
        const uint32_t num_frags = CGTranslatorGnm_TranslateNumFragmentsAndSamples(
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x34),
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x36),
            ((*reinterpret_cast<const uint32_t*>(tex_desc + 0x24) >> 10) & 1u) != 0);
        if (subresource_idx >= total_subres) {
            return 8;
        }
        uint32_t gnm_fmt = 0;
        int32_t err = CGTranslatorGnm_TranslateEngineFormatToGnmDataFormat(&gnm_fmt, tex_desc[0x33]);
        if (err != 0) {
            return err;
        }
        uint8_t tsharp[32] = {};
        const uint64_t base_sa = sub_1087ee0_initAs2d(
            tsharp,
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x2e),
            static_cast<uint16_t>(num_slices),
            mip_count,
            gnm_fmt,
            *reinterpret_cast<const uint32_t*>(tex_desc + 0x28),
            num_frags,
            is_cubemap);

        uint64_t meta_offset = 0;
        if (include_metadata) {
            err = CGTranslatorGnm_TranslateMetadataDataFormat(&gnm_fmt, tex_desc[0x33]);
            if (err != 0) {
                return err;
            }
            const uint64_t meta_sa = sub_1087ee0_initAs2d(
                tsharp,
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2e),
                static_cast<uint16_t>(num_slices),
                mip_count,
                gnm_fmt,
                *reinterpret_cast<const uint32_t*>(tex_desc + 0x28),
                num_frags,
                is_cubemap);
            const uint32_t align = static_cast<uint32_t>(meta_sa >> 32);
            meta_offset = (static_cast<uint32_t>(base_sa) + align - 1u) & ~(align - 1u);
        }

        uint64_t slice_off = 0, slice_sz = 0;
        if (sub_10a2c40_computeSurfaceOffsetAndSize(&slice_off, &slice_sz, tsharp, mip_level, array_slice) != 0) {
            return 8;
        }
        *out_byte_offset = meta_offset + slice_off;
        *out_byte_size = slice_sz;
        return 0;
    }

    case 4: { // 3D Texture
        uint32_t mip_count = tex_desc[0x32];
        if (mip_count == 0) {
            mip_count = sub_2165130_CalcDefaultMipCount(
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x2e),
                *reinterpret_cast<const uint16_t*>(tex_desc + 0x30));
        }
        if (subresource_idx >= mip_count) {
            return 8;
        }
        uint32_t gnm_fmt = 0;
        int32_t err = CGTranslatorGnm_TranslateEngineFormatToGnmDataFormat(&gnm_fmt, tex_desc[0x33]);
        if (err != 0) {
            return err;
        }
        uint8_t tsharp[32] = {};
        sub_1088590_initAs3d(
            tsharp,
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x2c),
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x2e),
            *reinterpret_cast<const uint16_t*>(tex_desc + 0x30),
            mip_count,
            gnm_fmt,
            *reinterpret_cast<const uint32_t*>(tex_desc + 0x28));
        uint64_t slice_off = 0, slice_sz = 0;
        if (sub_10a2c40_computeSurfaceOffsetAndSize(&slice_off, &slice_sz, tsharp, subresource_idx, 0) != 0) {
            return 8;
        }
        *out_byte_offset = slice_off;
        *out_byte_size = slice_sz;
        return 0;
    }
    }
    return 8;
}

// VMA: 0x26baf10 (78 B)
// Resolves `sce::Gnm::TextureType` from engine dimension (`edi`), array size (`edx`),
// flags (`cl`), and sample count (`r8d`).
uint32_t CGTextureGnm_ResolveTextureType(
    uint32_t dimension,
    uint32_t unused_esi,
    uint32_t array_size,
    uint32_t tex_flags,
    uint32_t sample_count)
{
    if ((tex_flags & 0x20u) != 0) return 2u; // Cubemap
    if ((tex_flags & 0x40u) != 0) return 4u; // 3D Volume
    const uint32_t is_array = (array_size != 1u) ? 3u : 0u;
    if (dimension == 4u) return 8u + is_array; // 1D / 1DArray
    if (dimension == 3u && sample_count == 4u) return 9u + is_array; // 2DMsaa / 2DArrayMsaa
    return 7u + is_array; // 2D / 2DArray
}

// VMA: 0x26baef0 (23 B)
// Atomically allocates a 24-bit resource ID from global counter `0x52ba180`.
uint32_t CGTextureGnm_AllocateUniqueResourceId() {
    return sub_207ebd0_AtomicIncrement(reinterpret_cast<volatile uint32_t*>(GX_VMA(0x52ba180ULL))) & 0x00FFFFFFu;
}

// VMA: 0x26baee0 (7 B)
uint64_t CGGpuAddressGnm_Unpack256ByteAlignedA(uint32_t addr_256) {
    return static_cast<uint64_t>(addr_256) << 8;
}

// VMA: 0x26baed0 (7 B)
uint32_t CGGpuAddressGnm_Pack256ByteAlignedA(uint64_t byte_addr) {
    return static_cast<uint32_t>(byte_addr >> 8);
}

// VMA: 0x26baec0 (7 B)
uint64_t CGGpuAddressGnm_Unpack256ByteAlignedB(uint32_t addr_256) {
    return static_cast<uint64_t>(addr_256) << 8;
}

// VMA: 0x26baeb0 (7 B)
uint32_t CGGpuAddressGnm_Pack256ByteAlignedB(uint64_t byte_addr) {
    return static_cast<uint32_t>(byte_addr >> 8);
}

// VMA: 0x26bae30 (114 B)
// Queries `sceKernelVirtualQuery` (`0x2bbf548`) and `sceKernelGetDirectMemoryType` (`0x2bbf558`)
// to verify if `ptr` resides in WC/Garlic (`type == 3`) direct memory.
bool CGGpuMemoryGnm_IsGarlicDirectMemory(const void* ptr) {
    if (ptr == nullptr) return false;
    struct OrbisVirtualQueryInfo {
        void*    start;
        void*    end;
        uint64_t offset;
        int32_t  protection;
        int32_t  memory_type;
        uint8_t  flags[40];
    } vq_info = {};
    if (sub_2bbf548_sceKernelVirtualQuery(ptr, 0, &vq_info, sizeof(vq_info)) != 0) {
        return false;
    }
    int32_t mem_type = 0;
    uint64_t region_start = 0, region_end = 0;
    if (sub_2bbf558_sceKernelGetDirectMemoryType(vq_info.offset, &mem_type, &region_start, &region_end) != 0) {
        return false;
    }
    return mem_type == 3;
}

// VMA: 0x26badd0 (84 B)
// Queries `sceKernelQueryMemoryProtection` (`0x2bbf538`) and checks `SCE_KERNEL_PROT_GPU_READ` (`0x10`).
bool CGGpuMemoryGnm_IsGpuReadable(const void* ptr) {
    if (ptr == nullptr) return false;
    uint32_t prot = 0;
    if (sub_2bbf538_sceKernelQueryMemoryProtection(ptr, nullptr, nullptr, &prot) != 0) {
        return false;
    }
    return (prot & 0x10u) != 0;
}

// VMA: 0x26bad70 (81 B)
// Queries `sceKernelQueryMemoryProtection` (`0x2bbf538`) and checks `SCE_KERNEL_PROT_CPU_READ` (`0x01`).
bool CGGpuMemoryGnm_IsCpuReadable(const void* ptr) {
    if (ptr == nullptr) return false;
    uint32_t prot = 0;
    if (sub_2bbf538_sceKernelQueryMemoryProtection(ptr, nullptr, nullptr, &prot) != 0) {
        return false;
    }
    return (prot & 0x01u) != 0;
}

// ============================================================================
// 4. CGShaderFactoryGnm - 6-Stage Shader Creation (0x26b9c00 .. 0x26baac0)
// ============================================================================

namespace {

int32_t CreateStageShaderCommon(
    void** out_hw_shader_clone,
    void** out_binding_header,
    const uint8_t* src_hw_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_table_offset,
    uint32_t* out_semantic_count,
    uint32_t hw_shader_clone_size,
    uint32_t aligned_header_offset,
    uint32_t sinput_offset)
{
    alignas(16) uint8_t container[128] = {};
    sub_10a3b70_ShaderContainer_Construct(container);
    if (sub_10a3c40_ShaderContainer_Parse(container, container_bytes, container_size) != 0) {
        sub_10a3bf0_ShaderContainer_Destroy(container);
        return 8;
    }

    const uint32_t num_slots = src_hw_shader[0x03];
    const bool want_semantics = (out_semantic_table_offset != nullptr) && (out_semantic_count != nullptr);
    const uint32_t sem_count = want_semantics ? container[0x28] : 0u;

    uint64_t binding_hdr_size = 0;
    uint32_t binding_slot_count = 0;
    int32_t err = sub_2175480_CalcShaderBindingHeaderSize(
        &binding_hdr_size,
        &binding_slot_count,
        src_hw_shader + sinput_offset,
        num_slots);
    if (err != 0) {
        sub_10a3bf0_ShaderContainer_Destroy(container);
        return err;
    }

    const size_t total_alloc_size =
        aligned_header_offset + binding_hdr_size + (static_cast<size_t>(sem_count) * 4u);
    using AllocFn = void* (*)(void*, size_t);
    using FreeFn  = void (*)(void*, void*);
    void** vtbl = *reinterpret_cast<void***>(allocator);
    uint8_t* mem = static_cast<uint8_t*>(reinterpret_cast<AllocFn>(vtbl[0x50 / 8])(allocator, total_alloc_size));
    *out_hw_shader_clone = mem;
    if (mem == nullptr) {
        sub_10a3bf0_ShaderContainer_Destroy(container);
        return 9;
    }

    sub_261a1b0_memcpy(mem, src_hw_shader, hw_shader_clone_size);
    // Patch 40-bit GPU shader binary address (`>> 8`) into `SPI_SHADER_PGM_LO/HI_*` at `+0x08` / `+0x0c`
    *reinterpret_cast<uint32_t*>(mem + 0x08) = static_cast<uint32_t>(gpu_ucode_addr >> 8);
    *reinterpret_cast<uint32_t*>(mem + 0x0c) = static_cast<uint32_t>(gpu_ucode_addr >> 40);
    gx_direct_vk::RegisterDirectShaderBinary(
        reinterpret_cast<const void*>(gpu_ucode_addr),
        (*reinterpret_cast<const uint32_t*>(mem) & 0x7FFFFFu),
        container_bytes,
        container_size,
        stage_flags);

    err = sub_21754e0_BuildShaderBindingHeader(
        out_binding_header,
        mem + aligned_header_offset,
        binding_slot_count,
        stage_flags,
        container,
        src_hw_shader + sinput_offset,
        num_slots,
        allocator);
    if (err != 0) {
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(allocator, mem);
        *out_hw_shader_clone = nullptr;
        sub_10a3bf0_ShaderContainer_Destroy(container);
        return err;
    }

    if (want_semantics) {
        const uint32_t sem_table_off = static_cast<uint32_t>(aligned_header_offset + binding_hdr_size);
        const uint32_t actual_sem_count = container[0x28];
        if (actual_sem_count != 0) {
            uint8_t* dst_sem = static_cast<uint8_t*>(*out_hw_shader_clone) + sem_table_off;
            for (uint32_t i = 0; i < actual_sem_count; ++i) {
                const uint8_t* sem_base = *reinterpret_cast<const uint8_t* const*>(container + 0x58);
                const size_t entry_off = static_cast<size_t>(i) * 16u + 0x0Cu;
                const uint8_t usage_idx = sem_base[entry_off - 0x0Au];
                const int32_t rel_name = *reinterpret_cast<const int32_t*>(sem_base + entry_off);
                const char* sem_name = reinterpret_cast<const char*>(sem_base + entry_off + rel_name);
                uint32_t resolved_id = 0;
                if (sub_26b7e60_ResolveSemanticName(&resolved_id, sem_name)) {
                    dst_sem[0] = static_cast<uint8_t>(resolved_id);
                    dst_sem[1] = usage_idx;
                } else {
                    *reinterpret_cast<uint16_t*>(dst_sem) = 0x00FFu;
                }
                const uint8_t* sem_base_after = *reinterpret_cast<const uint8_t* const*>(container + 0x58);
                dst_sem[2] = sem_base_after[entry_off - 0x09u];
                dst_sem[3] = 0;
                dst_sem += 4;
            }
        }
        *out_semantic_count = sem_count;
        *out_semantic_table_offset = sem_table_off;
    }

    sub_10a3bf0_ShaderContainer_Destroy(container);
    return 0;
}

} // namespace

// VMA: 0x26baac0 (688 B)
// Creates a Compute Shader (`Gnmx::CsShader` + `CGShaderBindingHeaderGnm`).
int32_t CGShaderFactoryGnm_CreateComputeShader(
    void** out_hw_shader,
    void** out_binding_header,
    const uint8_t* src_cs_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_offset,
    uint32_t* out_semantic_count)
{
    const uint32_t num_slots = (*reinterpret_cast<const uint32_t*>(src_cs_shader) >> 24);
    const uint32_t clone_size = (num_slots * 4u) + 0x28u;
    const uint32_t aligned_offset = (clone_size + 7u) & ~7u;
    return CreateStageShaderCommon(
        out_hw_shader, out_binding_header, src_cs_shader, gpu_ucode_addr,
        container_bytes, container_size, stage_flags, allocator,
        out_semantic_offset, out_semantic_count, clone_size, aligned_offset, 0x28u);
}

// VMA: 0x26ba800 (689 B)
// Creates a Pixel Shader (`Gnmx::PsShader` + `CGShaderBindingHeaderGnm`).
int32_t CGShaderFactoryGnm_CreatePixelShader(
    void** out_hw_shader,
    void** out_binding_header,
    const uint8_t* src_ps_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_offset,
    uint32_t* out_semantic_count)
{
    const uint32_t num_slots = (*reinterpret_cast<const uint32_t*>(src_ps_shader) >> 24);
    const uint32_t num_inputs = src_ps_shader[0x38];
    const uint32_t clone_size = ((num_inputs * 2u) + (num_slots * 4u) + 0x3Fu) & ~3u;
    const uint32_t aligned_offset = (clone_size + 7u) & ~7u;
    return CreateStageShaderCommon(
        out_hw_shader, out_binding_header, src_ps_shader, gpu_ucode_addr,
        container_bytes, container_size, stage_flags, allocator,
        out_semantic_offset, out_semantic_count, clone_size, aligned_offset, 0x3Cu);
}

// VMA: 0x26ba540 (689 B)
// Creates a Vertex Shader (`Gnmx::VsShader` + `CGShaderBindingHeaderGnm`).
int32_t CGShaderFactoryGnm_CreateVertexShader(
    void** out_hw_shader,
    void** out_binding_header,
    const uint8_t* src_vs_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_offset,
    uint32_t* out_semantic_count)
{
    const uint32_t num_slots = (*reinterpret_cast<const uint32_t*>(src_vs_shader) >> 24);
    const uint32_t clone_size = ((num_slots * 4u) + 0x43u) & ~3u;
    const uint32_t aligned_offset = (clone_size + 7u) & ~7u;
    return CreateStageShaderCommon(
        out_hw_shader, out_binding_header, src_vs_shader, gpu_ucode_addr,
        container_bytes, container_size, stage_flags, allocator,
        out_semantic_offset, out_semantic_count, clone_size, aligned_offset, 0x40u);
}

// VMA: 0x26ba280 (704 B)
// Creates a Geometry Shader (`Gnmx::GsShader` + `CGShaderBindingHeaderGnm`).
int32_t CGShaderFactoryGnm_CreateGeometryShader(
    void** out_hw_shader,
    void** out_binding_header,
    const uint8_t* src_gs_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_offset,
    uint32_t* out_semantic_count)
{
    const uint32_t total_slots = static_cast<uint32_t>(src_gs_shader[0x03]) + src_gs_shader[0x18];
    const uint32_t clone_size = (total_slots * 4u) + 0x20u;
    const uint32_t aligned_offset = (clone_size + 7u) & ~7u;
    return CreateStageShaderCommon(
        out_hw_shader, out_binding_header, src_gs_shader, gpu_ucode_addr,
        container_bytes, container_size, stage_flags, allocator,
        out_semantic_offset, out_semantic_count, clone_size, aligned_offset, 0x20u);
}

// VMA: 0x26b9fc0 (704 B)
// Creates a Hull Shader (`Gnmx::HsShader` + `CGShaderBindingHeaderGnm`).
int32_t CGShaderFactoryGnm_CreateHullShader(
    void** out_hw_shader,
    void** out_binding_header,
    const uint8_t* src_hs_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_offset,
    uint32_t* out_semantic_count)
{
    const uint32_t total_slots = static_cast<uint32_t>(src_hs_shader[0x03]) + src_hs_shader[0x18];
    const uint32_t clone_size = (total_slots * 4u) + 0x20u;
    const uint32_t aligned_offset = (clone_size + 7u) & ~7u;
    return CreateStageShaderCommon(
        out_hw_shader, out_binding_header, src_hs_shader, gpu_ucode_addr,
        container_bytes, container_size, stage_flags, allocator,
        out_semantic_offset, out_semantic_count, clone_size, aligned_offset, 0x20u);
}

// VMA: 0x26b9cf0 (705 B)
// Creates a Domain Shader (`Gnmx::DsShader` + `CGShaderBindingHeaderGnm`).
int32_t CGShaderFactoryGnm_CreateDomainShader(
    void** out_hw_shader,
    void** out_binding_header,
    const uint8_t* src_ds_shader,
    uint64_t gpu_ucode_addr,
    const void* container_bytes,
    uint32_t container_size,
    uint32_t stage_flags,
    void* allocator,
    uint32_t* out_semantic_offset,
    uint32_t* out_semantic_count)
{
    const uint32_t total_slots = static_cast<uint32_t>(src_ds_shader[0x03]) + src_ds_shader[0x24];
    const uint32_t extra_halfwords = src_ds_shader[0x25];
    const uint32_t clone_size = ((total_slots * 4u) + (extra_halfwords * 2u) + 0x2Bu) & ~3u;
    const uint32_t aligned_offset = (clone_size + 7u) & ~7u;
    return CreateStageShaderCommon(
        out_hw_shader, out_binding_header, src_ds_shader, gpu_ucode_addr,
        container_bytes, container_size, stage_flags, allocator,
        out_semantic_offset, out_semantic_count, clone_size, aligned_offset, 0x28u);
}

// VMA: 0x26b9c50 (3 B)
int32_t CGShaderFactoryGnm_StubReturnZeroA() { return 0; }

// VMA: 0x26b9c40 (3 B)
int32_t CGShaderFactoryGnm_StubReturnZeroB() { return 0; }

// VMA: 0x26b9c30 (1 B)
void CGShaderFactoryGnm_StubNoOpA() {}

// VMA: 0x26b9c20 (1 B)
void CGShaderFactoryGnm_StubNoOpB() {}

// VMA: 0x26b9c10 (1 B)
void CGShaderFactoryGnm_StubNoOpC() {}

// VMA: 0x26b9c00 (3 B)
int32_t CGShaderFactoryGnm_StubReturnZeroC() { return 0; }

// ============================================================================
// 5. CGSurfaceSyncTrackerGnm (0x26b9850 .. 0x26b9a60)
// ============================================================================

struct CGSurfaceSyncTrackerGnm {
    uint32_t pending_event_mode; // +0x00 (1..15 -> `triggerEvent` combinations)
    uint32_t event_cache_action; // +0x04
    uint32_t event_stall_mode;   // +0x08
    uint32_t gfx_min_addr_256;   // +0x0c (`0xFFFFFFFF` when empty)
    uint32_t gfx_max_addr_256;   // +0x10
    uint32_t cs_min_addr_256;    // +0x14 (`0xFFFFFFFF` when empty)
    uint32_t cs_max_addr_256;    // +0x18
    uint32_t target_mask_flags;  // +0x1c
    uint32_t cache_sync_mode;    // +0x20 (bitmask `0..7`)
    uint8_t  is_disabled;        // +0x24
};

// VMA: 0x26b9a60 (260 B)
// Records a GPU memory range (`[gpu_addr, gpu_addr + byte_size)`) and target cache mask into the
// surface sync tracker for subsequent `waitForGraphicsWrites` / `flushShaderCachesAndWait`.
void CGSurfaceSyncTrackerGnm_RecordRange(
    CGSurfaceSyncTrackerGnm* self,
    uint32_t sync_category,
    uint64_t gpu_addr,
    uint64_t byte_size,
    uint32_t target_mask)
{
    if (sync_category > 5u) {
        return;
    }
    switch (sync_category) {
    case 0:
    case 3:
        self->cache_sync_mode |= 0x01u;
        break;
    case 1:
        if (gpu_addr == 0) {
            self->cache_sync_mode |= 0x07u;
        } else {
            self->cache_sync_mode |= 0x02u;
            const uint32_t start_256 = CGGpuAddressGnm_Pack256ByteAlignedB(gpu_addr);
            const uint32_t end_256 = start_256 + CGGpuAddressGnm_Pack256ByteAlignedA(byte_size + 255ULL);
            if (start_256 < self->gfx_min_addr_256) self->gfx_min_addr_256 = start_256;
            if (end_256 > self->gfx_max_addr_256)   self->gfx_max_addr_256 = end_256;
            self->target_mask_flags |= target_mask;
        }
        break;
    case 2:
        if (gpu_addr == 0) {
            self->cache_sync_mode |= 0x07u;
        } else {
            self->cache_sync_mode |= 0x02u;
            const uint32_t start_256 = CGGpuAddressGnm_Pack256ByteAlignedB(gpu_addr);
            const uint32_t end_256 = start_256 + CGGpuAddressGnm_Pack256ByteAlignedA(byte_size + 255ULL);
            if (start_256 < self->cs_min_addr_256) self->cs_min_addr_256 = start_256;
            if (end_256 > self->cs_max_addr_256)   self->cs_max_addr_256 = end_256;
            self->target_mask_flags |= 0x100u;
        }
        break;
    case 4:
    case 5:
        break;
    }
}

// VMA: 0x26b9a50 (4 B)
bool CGSurfaceSyncTrackerGnm_IsDisabled(const CGSurfaceSyncTrackerGnm* self) {
    return self->is_disabled != 0;
}

// VMA: 0x26b9a40 (5 B)
void CGSurfaceSyncTrackerGnm_Enable(CGSurfaceSyncTrackerGnm* self) {
    self->is_disabled = 0;
}

// VMA: 0x26b9a30 (5 B)
void CGSurfaceSyncTrackerGnm_Disable(CGSurfaceSyncTrackerGnm* self) {
    self->is_disabled = 1;
}

// VMA: 0x26b9850 (468 B)
// Emits accumulated surface synchronization barriers (`flushShaderCachesAndWait` `0x26b09e0`,
// `waitForGraphicsWrites` `0x1075e10`, and `triggerEvent` `0x1077f40`) to `Gnmx::GfxContext`,
// then resets the tracker state.
void CGSurfaceSyncTrackerGnm_FlushToGfxContext(
    CGSurfaceSyncTrackerGnm* self,
    void* gnmx_ctx)
{
    if (self->is_disabled != 0) {
        return;
    }

    // Block 4 Step 4.2 (BB_DIRECT_RING, default on): Vulkan TextureCache & BufferCache track
    // render target / UAV / buffer hazards natively, and `AcquireMem` / `PfpSyncMe` / `EventWrite`
    // (`0x10 / 0x33 / 0x1B / 0x3B / 0x38`) are 100% no-ops in `cp_graphics.cpp`. Reset the tracker
    // directly without emitting 875 dead PM4 packets (1.22M dwords/s) per frame.
    if (gx_direct_vk::DirectRingEnabled()) {
        if (self->cache_sync_mode != 0) {
            self->gfx_min_addr_256 = 0xFFFFFFFFu;
            self->gfx_max_addr_256 = 0;
            self->cs_min_addr_256  = 0xFFFFFFFFu;
            self->cs_max_addr_256  = 0;
            self->target_mask_flags = 0;
            self->cache_sync_mode   = 0;
        }
        self->pending_event_mode = 0;
        self->event_cache_action = 0;
        self->event_stall_mode   = 1;
        return;
    }

    if (self->cache_sync_mode != 0) {
        switch (self->cache_sync_mode) {
        case 1:
            sub_26b09e0_FlushShaderCachesAndWait(gnmx_ctx, 0x28u, 0);
            break;
        case 2:
        case 3: {
            if (self->cache_sync_mode == 3) {
                sub_26b09e0_FlushShaderCachesAndWait(gnmx_ctx, 0x28u, 0);
            }
            const uint32_t min_256 = (self->gfx_min_addr_256 < self->cs_min_addr_256)
                                         ? self->gfx_min_addr_256
                                         : self->cs_min_addr_256;
            if (min_256 != 0xFFFFFFFFu) {
                const uint32_t max_256 = (self->gfx_max_addr_256 > self->cs_max_addr_256)
                                             ? self->gfx_max_addr_256
                                             : self->cs_max_addr_256;
                const uint32_t flags = self->target_mask_flags;
                const uint32_t target_mask = flags << 6;
                const uint32_t ext_mask =
                    ((flags & 0xFFu) != 0 ? 0x02000000u : 0u) | ((flags << 18) & 0x04000000u);
                sub_1075e10_waitForGraphicsWrites(
                    gnmx_ctx, min_256, max_256 - min_256, target_mask, 0, ext_mask, 1);
            }
            break;
        }
        case 4:
        case 5:
        case 6:
            break;
        case 7:
            sub_26b09e0_FlushShaderCachesAndWait(gnmx_ctx, 0x04u, 0);
            break;
        default:
            break;
        }

        self->gfx_min_addr_256 = 0xFFFFFFFFu;
        self->gfx_max_addr_256 = 0;
        self->cs_min_addr_256  = 0xFFFFFFFFu;
        self->cs_max_addr_256  = 0;
        self->target_mask_flags = 0;
        self->cache_sync_mode   = 0;
    }

    if (self->pending_event_mode >= 1u && self->pending_event_mode <= 15u) {
        const uint32_t cache_act = self->event_cache_action;
        const uint32_t stall     = self->event_stall_mode;
        switch (self->pending_event_mode) {
        case 1:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x10u, cache_act, stall);
            break;
        case 2:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x33u, cache_act, stall);
            break;
        case 3:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x10u, 0, 1);
            sub_1077f40_triggerEvent(gnmx_ctx, 0x33u, cache_act, stall);
            break;
        case 4:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x1Bu, cache_act, stall);
            break;
        case 5:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x10u, 0, 1);
            sub_1077f40_triggerEvent(gnmx_ctx, 0x1Bu, cache_act, stall);
            break;
        case 6:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x3Bu, cache_act, stall);
            break;
        case 7:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x10u, 0, 1);
            sub_1077f40_triggerEvent(gnmx_ctx, 0x3Bu, cache_act, stall);
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            break;
        case 15:
            sub_1077f40_triggerEvent(gnmx_ctx, 0x38u, cache_act, stall);
            break;
        }
    }

    self->pending_event_mode = 0;
    self->event_cache_action = 0;
    self->event_stall_mode   = 1;
}

} // namespace Bloodborne::CoreGraphics2::Gnm
