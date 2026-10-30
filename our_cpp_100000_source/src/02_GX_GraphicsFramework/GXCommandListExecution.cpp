// ============================================================================
// Bloodborne (CUSA03173 v1.09) — Decompiled Engine Source
// Module:  02_GX_GraphicsFramework (0x2167000 .. 0x21b6000)
// Original Path: N:\GS\Source\GraphicsFramework\Source\Base\System\GXCommandContext.cpp
// ============================================================================

#include <cstdint>
#include <cstring>

namespace GX {
void GXGnmCommandContext_UpdateSubresource(
    void* self, void* dstResource, uint32_t dstSubresource, const uint32_t* dstBox,
    const void* srcData, uint32_t srcRowPitch, uint32_t srcDepthPitch);
int32_t GXGnmCommandContext_ExecuteCommandList(void* self, void* cmdList, uint8_t restoreContextState);
int32_t GXGnmCommandContext_ExecuteCommandLists(
    void* self, void* const* cmdLists, uint32_t numCommandLists, uint8_t restoreContextState);
} // namespace GX

namespace FromSoftware::GX {

struct GfxContext;
struct GfxCommandList;
struct GfxResource;
struct Box;

// ----------------------------------------------------------------------------
// VMA: 0x2168a20 (Size: 480 bytes) — GfxContext::UpdateSubresource
// Suballocates a 64-KB staging chunk (`0x26addc0`), copies CPU source data,
// and enqueues the deferred buffer/texture copy (`0x26adb90`).
// ----------------------------------------------------------------------------
void GfxContext_UpdateSubresource(
    GfxContext* ctx,            // rdi
    GfxResource* resource,      // rsi
    uint32_t subresource,       // edx
    const Box* dst_box,         // rcx
    const void* src_data,       // r8
    uint32_t src_row_pitch,     // r9d
    uint32_t src_depth_pitch)   // [rbp+0x10]
{
    ::GX::GXGnmCommandContext_UpdateSubresource(
        ctx, resource, subresource, reinterpret_cast<const uint32_t*>(dst_box),
        src_data, src_row_pitch, src_depth_pitch);
}

// ----------------------------------------------------------------------------
// VMA: 0x216a7d0 (Size: 432 bytes) — GfxContext::ExecuteCommandList_Single
// Executes a single deferred command list (`GfxCommandList`):
//   1) 0x216a80d: call 0x26d1090 (FlushBufferUpdates_B -> cs fefebf9f)
//   2) 0x216a81c: call 0x26adf50 (FlushBufferUpdates_A -> cs fefebf9f)
//   3) Chains sub-list PM4 chunks into the main DrawCommandBuffer (`0x26afda0`).
// ----------------------------------------------------------------------------
void GfxContext_ExecuteCommandList_Single(GfxContext* main_ctx, GfxCommandList* cmd_list) {
    ::GX::GXGnmCommandContext_ExecuteCommandList(main_ctx, cmd_list, 0);
}

// ----------------------------------------------------------------------------
// VMA: 0x216a980 (Size: 880 bytes) — GfxContext::ExecuteCommandLists_Batch
// Invoked by the render-thread job replay dispatcher (`0x21b4d00`) in batches
// of up to 72 command lists (`0x48`).
// ----------------------------------------------------------------------------
void GfxContext_ExecuteCommandLists_Batch(
    GfxContext* main_ctx,           // rdi
    GfxCommandList* const* lists,   // rsi
    uint32_t list_count)            // edx
{
    ::GX::GXGnmCommandContext_ExecuteCommandLists(
        main_ctx, reinterpret_cast<void* const*>(lists), list_count, 0);
}

} // namespace FromSoftware::GX
