/**
 * Bloodborne Engine Decompile Project
 * Module: 02_GX_GraphicsFramework
 * File: GXGnmCommandContextAndDeviceFactory.cpp
 * Address Range: 0x2167000 .. 0x216af4f (141 functions, 13,983 bytes)
 * Description:
 *   Orbis (PS4) sce::Gnm command context draw/dispatch/query/copy/clear methods,
 *   per-stage resource dirty bitmask setters (CS, HS, DS, GS, PS, VS),
 *   stage state binding thunks, deferred context management, and device factory
 *   creation methods for shaders, queries, and predicates.
 *   100% complete instruction-level C++ reconstruction (Zero Stubs Policy).
 */

#include <cstdint>
#include <cstring>

#include "gx_direct_vulkan.h"

namespace GX {

// ============================================================================
// External Engine / CoreGraphics2_GNM / Orbis sceGnm Declarations
// ============================================================================

struct DLAllocator {
    struct VTable {
        void* pad00[10]; // 0x00 .. 0x48
        void* (*Allocate)(DLAllocator* self, std::size_t size);                       // +0x50
        void* (*AllocateAligned)(DLAllocator* self, std::size_t size, std::size_t align); // +0x58
        void* pad60[2];  // 0x60 .. 0x68
        void  (*Free)(DLAllocator* self, void* ptr);                                  // +0x70
    };
    VTable* vptr;
};

struct DLMutex {
    struct VTable {
        void* pad00[3];
        void (*Lock)(DLMutex* self, std::uint32_t timeout); // +0x18
        void* pad20;
        void (*Unlock)(DLMutex* self);                      // +0x28
    };
    VTable* vptr;
};

#undef g_GXCommandContextSlotMutex
#define g_GXCommandContextSlotMutex (*reinterpret_cast<DLMutex*>(GX_VMA(0x56a63a0ULL)))
#undef g_GXCommandContextSlotBitmask
#define g_GXCommandContextSlotBitmask (reinterpret_cast<std::uint64_t*>(GX_VMA(0x56a6380ULL)))
#undef g_pGXRegisteredCommandContexts
#define g_pGXRegisteredCommandContexts (reinterpret_cast<void**>(GX_VMA(0x56a63c0ULL)))
#undef g_GnmFormatBlockWidthTable
#define g_GnmFormatBlockWidthTable (reinterpret_cast<const std::uint64_t*>(GX_VMA(0x473ae60ULL)))
#undef g_GnmFormatBlockHeightTable
#define g_GnmFormatBlockHeightTable (reinterpret_cast<const std::uint64_t*>(GX_VMA(0x473b180ULL)))
#undef g_GnmFormatBytesPerElementTable
#define g_GnmFormatBytesPerElementTable (reinterpret_cast<const std::uint64_t*>(GX_VMA(0x47388d0ULL)))
#undef g_VTable_GXGnmCommandBufferWrapper_Deferred
#define g_VTable_GXGnmCommandBufferWrapper_Deferred (*reinterpret_cast<void**>(GX_VMA(0x535f350ULL)))
#undef g_VTable_GXGnmCommandBufferWrapper_Immediate
#define g_VTable_GXGnmCommandBufferWrapper_Immediate (*reinterpret_cast<void**>(GX_VMA(0x535f300ULL)))

extern "C" {
    // 01_CoreGraphics2_GNM imports
    void* CGGpuFreeListAllocatorGnm_AllocateAligned(void* alloc, std::uint32_t size, std::uint32_t align); // 0x26a9fa0
    void* CGGpuFreeListAllocatorGnm_AllocateAlign16(void* alloc, std::uint32_t size); // 0x26a9d50
    void* GpuHeap_AllocDefaultAlign16(void* alloc, std::uint32_t size); // 0x26aa010
    void* GpuRangeHeap_AllocateFromTree(void* alloc, std::uint32_t size, std::uint32_t align); // 0x26aa270
    void  CGGpuFreeListAllocatorGnm_FreeBlockImmediateB(void* alloc); // 0x26aa440
    void  CGGpuFreeListAllocatorGnm_DeferFreeWithEoPLabelB(void* alloc); // 0x26aa620
    void  CGGpuFreeListAllocatorGnm_DeferFreeOrChainB(void* alloc); // 0x26aa780

    void* CGDescriptorViewGnm_Identity(void* obj); // 0x26a94c0
    void  CGDescriptorViewGnm_CopyConstructSamplerView(void* dst, const void* src); // 0x26a9570
    void  CGDescriptorViewGnm_CopyConstructResourceView(void* dst, const void* src); // 0x26a9660
    std::uint32_t CGDescriptorViewGnm_CreateSamplerView(void* outView, void* dev, const void* desc, void* cpuAlloc, void* gpuAlloc); // 0x26a9750
    void  CGDescriptorViewGnm_DestroySamplerView(void* view); // 0x26a97d0
    std::uint32_t CGDescriptorViewGnm_CreateResourceView(void* outView, void* dev, const void* desc, void* cpuAlloc, void* gpuAlloc); // 0x26a98a0
    void  CGDescriptorViewGnm_DestroyResourceView(void* view); // 0x26a99a0

    void  CGCommandStreamRecorderGnm_Initialize(void* rec, void* targets, void* offsets); // 0x26ab820
    void  CGCommandStreamRecorderGnm_Finalize(void* rec); // 0x26ab880
    std::uint32_t CGCommandStreamRecorderGnm_BakeContiguousStream(void* context, void* cmdList, void* outChunkHead, void* outAllocator); // 0x26ab900
    void  CGCommandStreamRecorderGnm_WriteOp01_SetRasterizerState(void* rec, void* state); // 0x26abb00
    void  CGCommandStreamRecorderGnm_WriteOp02_SetPrimitiveTopology(void* rec, std::uint32_t ref); // 0x26abb80
    void  CGCommandStreamRecorderGnm_WriteOp03_SetIndexBuffer(void* rec, void* state, std::uint32_t factor, std::uint32_t mask); // 0x26abc00
    void  CGCommandStreamRecorderGnm_WriteOp04_SetVertexBuffers(void* rec, std::uint32_t numRTVs, std::uint32_t count, void* rtvs, void* dsv, void* uavs); // 0x26abca0
    void  CGCommandStreamRecorderGnm_WriteOp05_SetVSConstantBuffers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* rects); // 0x26abdd0
    void  CGCommandStreamRecorderGnm_WriteOp06_SetVSTextures(void* rec, std::uint32_t startSlot, std::uint32_t count, void* viewports); // 0x26abfb0
    void  CGCommandStreamRecorderGnm_WriteOp07_SetVSSamplers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* vbs); // 0x26ac190
    void  CGCommandStreamRecorderGnm_WriteOp08_SetVertexShader(void* rec, void* shader); // 0x26ac370
    void  CGCommandStreamRecorderGnm_WriteOp09_SetPSConstantBuffers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* samplers); // 0x26ac480
    void  CGCommandStreamRecorderGnm_WriteOp0A_SetPSTextures(void* rec, std::uint32_t startSlot, std::uint32_t count, void* srvs); // 0x26ac570
    void  CGCommandStreamRecorderGnm_WriteOp0B_SetPSSamplers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* cbs); // 0x26ac660
    void  CGCommandStreamRecorderGnm_WriteOp0C_SetPixelShader(void* rec, void* shader); // 0x26ac750
    void  CGCommandStreamRecorderGnm_WriteOp0D_SetGSConstantBuffers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* samplers); // 0x26ac7d0
    void  CGCommandStreamRecorderGnm_WriteOp0E_SetGSTextures(void* rec, std::uint32_t startSlot, std::uint32_t count, void* srvs); // 0x26ac8c0
    void  CGCommandStreamRecorderGnm_WriteOp0F_SetGSSamplers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* cbs); // 0x26ac9b0
    void  CGCommandStreamRecorderGnm_WriteOp10_SetGeometryShader(void* rec, void* shader); // 0x26acaa0
    void  CGCommandStreamRecorderGnm_WriteOp11_SetHSConstantBuffers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* samplers); // 0x26acb20
    void  CGCommandStreamRecorderGnm_WriteOp12_SetHSTextures(void* rec, std::uint32_t startSlot, std::uint32_t count, void* srvs); // 0x26acc10
    void  CGCommandStreamRecorderGnm_WriteOp13_SetHSSamplers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* cbs); // 0x26acd00
    void  CGCommandStreamRecorderGnm_WriteOp14_SetHullShader(void* rec, void* shader); // 0x26acdf0
    void  CGCommandStreamRecorderGnm_WriteOp15_SetDSConstantBuffers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* samplers); // 0x26ace70
    void  CGCommandStreamRecorderGnm_WriteOp16_SetDSTextures(void* rec, std::uint32_t startSlot, std::uint32_t count, void* srvs); // 0x26acf60
    void  CGCommandStreamRecorderGnm_WriteOp17_SetDSSamplers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* cbs); // 0x26ad050
    void  CGCommandStreamRecorderGnm_WriteOp18_SetDomainShader(void* rec, void* shader); // 0x26ad140
    void  CGCommandStreamRecorderGnm_WriteOp19_SetCSConstantBuffers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* samplers); // 0x26ad1c0
    void  CGCommandStreamRecorderGnm_WriteOp1A_SetCSTextures(void* rec, std::uint32_t startSlot, std::uint32_t count, void* srvs); // 0x26ad2b0
    void  CGCommandStreamRecorderGnm_WriteOp1B_SetCSSamplers(void* rec, std::uint32_t startSlot, std::uint32_t count, void* cbs); // 0x26ad3a0
    void  CGCommandStreamRecorderGnm_WriteOp1C_SetComputeShader(void* rec, void* layout); // 0x26ad490
    void  CGCommandStreamRecorderGnm_WriteOp1D_Dispatch(void* rec, std::uint32_t fmt, std::uint32_t offset, std::uint32_t buf); // 0x26ad510
    void  CGCommandStreamRecorderGnm_WriteOp1E_Draw(void* rec, std::uint32_t topo, std::uint32_t flags); // 0x26ad5b0
    void  CGCommandStreamRecorderGnm_WriteOp1F_DrawIndexed(void* rec, std::uint32_t s, std::uint32_t c, std::uint32_t v); // 0x26ad640
    void  CGCommandStreamRecorderGnm_WriteOp20_DrawInstanced(void* rec, std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d); // 0x26ad6e0
    void  CGCommandStreamRecorderGnm_WriteOp21_DrawIndexedInstanced(void* rec, std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t e); // 0x26ad790
    void  CGCommandStreamRecorderGnm_WriteOp22_DrawInstancedIndirect(void* rec, void* uav, std::uint32_t count); // 0x26ad850
    void  CGCommandStreamRecorderGnm_WriteOp23_DrawIndexedInstancedIndirect(void* rec, void* shader, std::uint32_t flags); // 0x26ad8e0
    void  CGCommandStreamRecorderGnm_WriteOp24_FlushCaches(void* rec); // 0x26ad970

    void  CGConstantBufferUpdaterGnm_UpdateSubresource(void* updater, void* ctx, void* dst, void* src, const void* data); // 0x26adb90
    void  CGConstantBufferUpdaterGnm_FlushToGfxContext(void* updater, void* ctx); // 0x26adf50

    void  CGGfxContextGnm_Construct(void* gfxCtx); // 0x26ae9d0
    void  CGGfxContextGnm_CopyConstruct(void* dstGfxCtx, const void* srcGfxCtx); // 0x26aeab0
    void  CGGfxContextGnm_Finalize(void* gfxCtx); // 0x26aec40
    std::uint32_t CGGfxContextGnm_Initialize(
        void* gfxCtx, std::uint32_t ctxType, void* dev, void* ringInfo, void* cpuAlloc, void* gpuAlloc,
        std::uint32_t ringSize, std::uint32_t flag, std::uint32_t slotId); // 0x26aee20
    void  CGGfxContextGnm_ResetCommandChunks(void* gfxCtx); // 0x26af3f0
    void  CGGfxContextGnm_FlushPendingStagingUploads(void* gfxCtx); // 0x26af700
    void  CGGfxContextGnm_FlushAndSubmitImmediate(void* gfxCtx); // 0x26afa00
    std::uint32_t FlushAndPrepareSubmit(void* gfxCtx, void* cmdList); // 0x26afd30
    void  CGGfxContextGnm_PrepareBatchCommandListExecution(void* gfxCtx); // 0x26b0110
    std::uint32_t CGGfxContextGnm_ExecuteCommandChunk(void* gfxCtx, void* chunkDesc); // 0x26b03f0
    void  CGGfxContextGnm_FinalizeBatchCommandListExecution(void* gfxCtx); // 0x26b0630
    void* CGGfxContextGnm_AllocateFromCommandBufferAligned(void* gfxCtx, std::uint32_t size, std::uint32_t align); // 0x26b07f0

    void  GpuUtil_SingleCopy(void* gfxCtx, std::uint64_t dstGpuAddr, std::uint32_t dstSync, std::uint64_t srcGpuAddr, std::uint32_t srcSync, std::uint64_t size); // 0x26b2670
    void  CGGpuBlitAndClearPassesGnm_CopyBufferRegionViaCompute(void* ctx, void* dstBuf, void* srcBuf, std::uint32_t dstOff, std::uint32_t srcOff, std::uint64_t alignedSize); // 0x26b2d30
    void  CGGpuBlitAndClearPassesGnm_CopyTextureRegionViaCompute(
        void* ctx, void* dstTex, std::uint32_t dstSub, std::uint32_t dstX, std::uint32_t dstY, std::uint32_t dstZ,
        void* srcTex, std::uint32_t srcSub, const void* srcBox); // 0x26b2f40
    void  CGGpuBlitAndClearPassesGnm_UpdateBufferSubresourceViaCompute(
        void* gfxCtx, void* dstBuf, const void* srcData, std::uint32_t dstOff, std::uint64_t alignedSize); // 0x26b3560
    void  CGGpuBlitAndClearPassesGnm_UpdateTextureSubresourceViaCompute(
        void* gfxCtx, void* dstTex, const void* subresourceDataDesc, std::uint32_t dstSub, const void* dstBox, std::uint32_t flags); // 0x26b35e0
    void  CGGpuBlitAndClearPassesGnm_ClearUnorderedAccessViewUint(void* ctx, void* uavView, void* parentRes, const std::uint32_t* vals, std::uint32_t mask); // 0x26b4ef0
    void  CGGpuBlitAndClearPassesGnm_CopyDualRenderTargetsFullscreen(
        void* ctx, void* dstTex, std::uint32_t dstSub, void* srcTex, std::uint32_t srcSub, std::uint32_t fmt); // 0x26b55a0
    void  CGGpuBlitAndClearPassesGnm_ClearDepthStencilTarget(
        void* ctx, void* dsvView, void* parentRes, std::uint8_t stencil, std::uint32_t clearFlags, float depth); // 0x26b5f50

    std::uint32_t CGSurfaceSyncTrackerGnm_TransitionResourceForRead(void* tracker, std::uint32_t stageMask, void* res); // 0x26b82b0
    void  CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(void* tracker, std::uint32_t token, std::uint32_t stageMask, void* res); // 0x26b8c90
    std::uint32_t CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(void* tracker, std::uint32_t stageMask, void* res); // 0x26b8e20
    void  CGSurfaceSyncTrackerGnm_FlushToGfxContext(void* tracker, void* gfxCtx); // 0x26b9850
    void  CGSurfaceSyncTrackerGnm_Enable(void* tracker); // 0x26b9a40
    bool  CGSurfaceSyncTrackerGnm_IsDisabled(const void* tracker); // 0x26b9a50

    std::uint32_t CGTextureGnm_CreateSubresourceViewTSharp(
        std::uint64_t* outByteOffset, std::uint64_t* outByteSize, void* texRes, std::uint32_t subresource, std::uint32_t flag0, std::uint32_t flag1); // 0x26baf60
    void  CGCommandBufferGnm_WriteEndOfPipeImmediate(void* gfxCtx, std::uint32_t eventType, void* dstGpuAddr, std::uint64_t val, std::uint32_t cacheAction); // 0x26bb6d0
    void  CGCommandBufferGnm_WriteEndOfPipeCounter(void* gfxCtx, std::uint32_t eventType, void* dstGpuAddr, std::uint32_t cacheAction); // 0x26bb700
    void  CGCommandListGnm_SubmitRecordedChunks(void* cmdList); // 0x26bb730

    void  CGIndexBufferCacheGnm_Reset(void* ibCache); // 0x26bba10
    void  CGIndexBufferCacheGnm_BindToGfxContext(void* ibCache, void* ctx, void* stateCtx, std::uint32_t isIndexed); // 0x26bba30
    void  CGShaderResourceBinderGnm_ResetStateCaches(void* binder); // 0x26bc090
    void  CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(void* binder, void* ctx, void* stateCtx); // 0x26bc2e0
    void  CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(void* binder, void* ctx); // 0x26cbd50
    void  CGShaderResourceBinderGnm_SyncAndBindComputeStage(void* binder, void* ctx, void* stateCtx); // 0x26cbe50
    void  CGShaderResourceBinderGnm_PostDispatchSyncAndGdsReadback(void* binder, void* ctx, void* stateCtx); // 0x26ccf20
    void  CGShaderResourceBinderGnm_Reset(void* binder, void* ctx); // 0x26cd190

    void  CGDynamicBufferRingGnm_ZeroInit(void* ring); // 0x26d0bb0
    void  CGDynamicBufferRingGnm_MoveInit(void* dstRing, void* srcRing); // 0x26d0bd0
    void  CGDynamicBufferRingGnm_Destroy(void* ring); // 0x26d0bf0
    std::uint32_t CGDynamicBufferRingGnm_Init(void* ring, void* cpuAlloc); // 0x26d0c70
    void  CGDynamicBufferRingGnm_ClearChunks(void* ring); // 0x26d0ca0
    std::uint32_t MapDynamicBuffer_B(void* ring, void* gfxCtx, void** outMappedPtr, void* bufRes); // 0x26d0d30
    std::uint64_t CGDynamicBufferRingGnm_ResolveGpuAddress(void* ring, void* bufRes); // 0x26d0ff0
    void  CGDynamicBufferRingGnm_FlushToGfxContext(void* ring, void* gfxCtx); // 0x26d1090

    void  CGOutputMergerCacheGnm_Construct(void* omCache); // 0x26d1720
    void  CGOutputMergerCacheGnm_Copy(void* dstOmCache, const void* srcOmCache); // 0x26d1980
    void  CGOutputMergerCacheGnm_NoOpStub(void* omCache); // 0x26d1b40
    void  CGOutputMergerCacheGnm_DecompressColorTextureMips(void* omCache, void* ctx, void* texRes); // 0x26d1eb0
    void  CGOutputMergerCacheGnm_DecompressDepthTextureMips(void* omCache, void* ctx, void* texRes); // 0x26d2110
    void  CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(void* omCache, void* ctx, void* stateCtx); // 0x26d2230
    void  CGOutputMergerCacheGnm_Reset(void* omCache); // 0x26d2d20
    void  CGOutputMergerCacheGnm_SnapshotAndInvalidateTargets(void* omCache, void* outSnapA, void* outSnapB); // 0x26d2f30
    void  CGOutputMergerCacheGnm_DecompressUnboundTargets(void* omCache, void* ctx, const void* newTargets); // 0x26d3070

    void  CGRasterizerCacheGnm_Reset(void* rsCache); // 0x26d3300
    void  CGRasterizerCacheGnm_MarkViewportsDirty(void* rsCache, std::uint32_t count); // 0x26d3350
    void  CGRasterizerCacheGnm_MarkScissorsDirty(void* rsCache, std::uint32_t count); // 0x26d3360
    void  CGRasterizerCacheGnm_FlushToGfxContext(void* rsCache, void* ctx, void* stateCtx); // 0x26d3370

    void* CGDeviceChild_GetSelf(void* obj); // 0x26d44b0
    void* sub_26d5740(void* cmdListObj); // 0x26d5740
    void  CGDeferredContextGnm_Init(void* dstDeferredCtx, const void* srcTempCtx, void* cpuAlloc); // 0x26d57e0

    // 02_GX_GraphicsFramework internal imports
    std::uint32_t GXGnmShaderResourceView_InitializeForSurfaceMipSlice(
        void* outTSharp, const void* texRes, std::uint32_t format,
        std::uint32_t mipSlice, std::uint32_t arraySlice, std::uint32_t mipCount,
        std::uint32_t arraySize, std::uint32_t isSrgb); // 0x216d900
    std::uint32_t GXGnmExportShaderProgram_InitializeFromBinary(
        void* outProg, void* dev, const void* bin, std::uint64_t size, void* cpuAlloc, void* gpuAlloc); // 0x2172db0
    std::uint32_t GXGnmGeometryShaderProgram_InitializeFromShdrBinary(
        void* outProg, void* dev, const void* bin, std::uint64_t size, void* cpuAlloc, void* gpuAlloc); // 0x2173220
    std::uint32_t GXGnmGeometryShaderWithStreamOut_InitializeFromBinary(
        void* outProg, void* dev, const void* bin, std::uint64_t size,
        const void* soDescs, std::uint32_t numEntries, const std::uint32_t* strides,
        std::uint32_t numStrides, std::uint32_t rasterizedStream, void* cpuAlloc, void* gpuAlloc); // 0x2173620
    std::uint32_t GXGnmDomainShaderProgram_InitializeFromBinary(
        void* outProg, void* dev, const void* bin, std::uint64_t size, void* cpuAlloc, void* gpuAlloc); // 0x2175080
    std::uint32_t GXGnmHullShaderProgram_InitializeFromBinary(
        void* outProg, void* dev, const void* bin, std::uint64_t size, void* cpuAlloc, void* gpuAlloc); // 0x2175280
    int GXGnmShaderContainer_InitializeFromShdrBinary(
        void* outProg, void* dev, const void* bin, std::uint64_t size, void* cpuAlloc, void* gpuAlloc); // 0x2177a40

    // Orbis sce::Gnm::DrawCommandBuffer / ConstantCommandBuffer / Texture PLT imports
    void  sub_1073e60(void* gfxCtx, std::uint32_t dstType, std::uint64_t dstGpuAddr, std::uint32_t srcType, std::uint64_t srcGpuAddr, std::uint32_t byteCount, std::uint32_t writeConfirm); // 0x1073e60 (dmaData / copyData)
    void  sub_1074040(void* gfxCtx, std::int32_t vertexOffset); // 0x1074040 (setBaseVertexOffset)
    void  sub_10741b0(void* gfxCtx, std::uint32_t vertexCount); // 0x10741b0 (drawIndexAuto)
    void  sub_10742f0(void* gfxCtx, std::uint64_t indirectArgsGpuAddr); // 0x10742f0 (setBaseIndirectArgs)
    void  sub_1074420(void* gfxCtx, std::uint32_t startIndex, std::uint32_t indexCount); // 0x1074420 (drawIndexOffset)
    void  sub_10744d0(void* gfxCtx, std::uint32_t byteOffset, std::uint32_t stageUsage, std::uint32_t vtxOffReg, std::uint32_t instOffReg); // 0x10744d0 (drawIndirect)
    void  sub_10746f0(void* gfxCtx, std::uint32_t byteOffset, std::uint32_t stageUsage, std::uint32_t vtxOffReg, std::uint32_t instOffReg); // 0x10746f0 (drawIndexIndirect)
    void  sub_1074c50(void* gfxCtx, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ); // 0x1074c50 (dispatch)
    void  sub_1074de0(void* gfxCtx, std::uint32_t byteOffset); // 0x1074de0 (dispatchIndirect)
    void  sub_1075370(void* gfxCtx, std::uint32_t numInstances); // 0x1075370 (setNumInstances)
    void  sub_1075da0(void* gfxCtx, std::uint32_t eventType); // 0x1075da0 (triggerEvent: 0x19=PerfCounterStart, 0x1A=PerfCounterStop)
    void  sub_1077570(void* gfxCtx, void* predGpuAddr, std::uint32_t hint, std::uint32_t predAction); // 0x1077570 (setZPassPredicationEnable)
    void  sub_1077600(void* gfxCtx); // 0x1077600 (setZPassPredicationDisable)
    void  sub_1077660(void* gfxCtx, std::uint32_t op, void* occlusionResultsGpuAddr); // 0x1077660 (writeOcclusionQuery)
    void  sub_1077850(void* gfxCtx, std::uint32_t pipeStatIdx, void* dstGpuAddr); // 0x1077850 (samplePipelineStats)
    std::uint32_t sub_1089100(const void* tsharp); // 0x1089100 (sce::Gnm::Texture::getHeightMinus1)
    std::uint32_t sub_1089890(const void* tsharp); // 0x1089890 (sce::Gnm::Texture::getWidthMinus1)

    // System / CRT imports
    void  operator_delete(void* ptr); // 0x2bbe588
    std::uint64_t sceKernelGetProcessTimeCounter(); // 0x2bbfe78
    std::uint32_t sceKernelUsleep(std::uint32_t microseconds); // 0x2bc07c8
}

// Forward declarations within this translation unit
void GXGnmCommandContext_CopySubresourceRegion(
    void* context, void* unused, void* dstResource, std::uint32_t dstSubresource,
    std::uint32_t dstX, std::uint32_t dstY, std::uint32_t dstZ,
    void* srcResource, std::uint32_t srcSubresource, const void* srcBox);
void GXGnmCommandContext_ShutdownAndUnregisterSlot(void* context);
void GXGnmCommandContext_CopyConstruct(void* dstContext, const void* srcContext);

// ============================================================================
// 1. Gnm Command Buffer Wrapper Virtual Thunks (0x216aea0 .. 0x216af40)
// ============================================================================

// [VMA: 0x216af40 | Size: 13 B]
void* GXGnmCommandBufferWrapper_SetViewportTransformControlThunk(void* self, std::uint32_t size, std::uint32_t align) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    return CGGpuFreeListAllocatorGnm_AllocateAligned(inner, size, align);
}

// [VMA: 0x216af30 | Size: 11 B]
void* GXGnmCommandBufferWrapper_SetHardwareScreenOffsetThunk(void* self, std::uint32_t size) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    return CGGpuFreeListAllocatorGnm_AllocateAlign16(inner, size);
}

// [VMA: 0x216af20 | Size: 5 B]
void GXGnmCommandBufferWrapper_DeletingDestructorA(void* self) {
    operator_delete(self);
}

// [VMA: 0x216af10 | Size: 1 B]
void GXGnmCommandBufferWrapper_DestroyNoOpA() {}

// [VMA: 0x216af00 | Size: 9 B]
void GXGnmCommandBufferWrapper_ResetToDefaultStateThunk(void* self) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    CGGpuFreeListAllocatorGnm_DeferFreeOrChainB(inner);
}

// [VMA: 0x216aef0 | Size: 9 B]
void GXGnmCommandBufferWrapper_FlushAndSubmitAltThunk(void* self) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    CGGpuFreeListAllocatorGnm_FreeBlockImmediateB(inner);
}

// [VMA: 0x216aee0 | Size: 9 B]
void GXGnmCommandBufferWrapper_WaitUntilSafeThunk(void* self) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    CGGpuFreeListAllocatorGnm_DeferFreeWithEoPLabelB(inner);
}

// [VMA: 0x216aed0 | Size: 13 B]
void* GXGnmCommandBufferWrapper_SetGuardBandClipThunk(void* self, std::uint32_t size, std::uint32_t align) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    return GpuRangeHeap_AllocateFromTree(inner, size, align);
}

// [VMA: 0x216aec0 | Size: 11 B]
void* GXGnmCommandBufferWrapper_SetSubpixelControlThunk(void* self, std::uint32_t size) {
    void* inner = *reinterpret_cast<void**>(static_cast<std::uint8_t*>(self) + 0x08);
    return GpuHeap_AllocDefaultAlign16(inner, size);
}

// [VMA: 0x216aeb0 | Size: 5 B]
void GXGnmCommandBufferWrapper_DeletingDestructorB(void* self) {
    operator_delete(self);
}

// [VMA: 0x216aea0 | Size: 1 B]
void GXGnmCommandBufferWrapper_DestroyNoOpB() {}

// ============================================================================
// 2. Gnm Command Context Draw, Dispatch, Query & Map (0x21695c0 .. 0x216aca0)
// ============================================================================

// [VMA: 0x216aca0 | Size: 177 B]
std::uint32_t GXGnmCommandContext_BeginCommandList(void* context, void* unused, void* targetList) {
    if (!targetList) {
        return 8;
    }
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* syncTracker = ctxBase + 0xB478;

    if (CGSurfaceSyncTrackerGnm_IsDisabled(syncTracker)) {
        CGSurfaceSyncTrackerGnm_Enable(syncTracker);
    }
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);

    auto* cmdListImpl = static_cast<std::uint8_t*>(sub_26d5740(targetList));
    std::uint32_t res = FlushAndPrepareSubmit(gfxCtx, cmdListImpl);
    if (res != 0) {
        return res;
    }

    void* omCache = ctxBase + 0xB558;
    CGOutputMergerCacheGnm_SnapshotAndInvalidateTargets(omCache, cmdListImpl + 0x20, cmdListImpl + 0x68);
    CGIndexBufferCacheGnm_Reset(ctxBase + 0xB518);
    CGRasterizerCacheGnm_Reset(ctxBase + 0xB528);
    CGOutputMergerCacheGnm_Reset(omCache);
    CGShaderResourceBinderGnm_ResetStateCaches(ctxBase + 0x50);
    return 0;
}

// [VMA: 0x216ac60 | Size: 53 B]
void GXGnmCommandContext_FlushCommandBuffer(void* context) {
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* syncTracker = ctxBase + 0xB478;
    if (CGSurfaceSyncTrackerGnm_IsDisabled(syncTracker)) {
        CGSurfaceSyncTrackerGnm_Enable(syncTracker);
    }
    CGGfxContextGnm_FlushAndSubmitImmediate(gfxCtx);
}

// [VMA: 0x216a980 | Size: 732 B]
void GfxContext_ExecuteCommandLists_Batch(
    void* context,
    void* unused,
    void* const* commandLists,
    std::uint32_t numCommandLists)
{
    if (!commandLists || numCommandLists == 0) {
        return;
    }
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* syncTracker = ctxBase + 0xB478;

    if (CGSurfaceSyncTrackerGnm_IsDisabled(syncTracker)) {
        CGSurfaceSyncTrackerGnm_Enable(syncTracker);
    }
    CGSurfaceSyncTrackerGnm_FlushToGfxContext(syncTracker, gfxCtx);
    CGGfxContextGnm_FlushAndSubmitImmediate(gfxCtx);
    CGGfxContextGnm_PrepareBatchCommandListExecution(gfxCtx);

    void* dynRing = ctxBase + 0xB6F8;
    void* cbUpdater = ctxBase + 0xB3C0;
    void* omCache = ctxBase + 0xB558;
    void* ibCache = ctxBase + 0xB518;
    void* rsCache = ctxBase + 0xB528;
    void* srBinder = ctxBase + 0x50;

    for (std::uint32_t i = 0; i < numCommandLists; ++i) {
        void* listObj = commandLists[i];
        if (!listObj) {
            continue;
        }
        auto* cmdList = static_cast<std::uint8_t*>(sub_26d5740(listObj));

        CGDynamicBufferRingGnm_FlushToGfxContext(dynRing, gfxCtx);
        CGConstantBufferUpdaterGnm_FlushToGfxContext(cbUpdater, ctxBase);
        CGOutputMergerCacheGnm_DecompressUnboundTargets(omCache, ctxBase, cmdList + 0x20);
        CGSurfaceSyncTrackerGnm_FlushToGfxContext(syncTracker, gfxCtx);

        auto* chunkHead = *reinterpret_cast<std::uint8_t**>(cmdList + 0x08);
        if (chunkHead) {
            std::uint64_t maxPerBlock = *reinterpret_cast<std::uint32_t*>(cmdList + 0x18);
            while (chunkHead) {
                auto** nextBlockPtr = reinterpret_cast<std::uint8_t**>(chunkHead + (maxPerBlock << 6));
                std::uint32_t idx = 0;
                while (true) {
                    std::uint64_t limit = (*nextBlockPtr != nullptr)
                        ? maxPerBlock
                        : static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(cmdList + 0x1C));
                    if (idx >= limit) {
                        break;
                    }
                    std::uint32_t stop = CGGfxContextGnm_ExecuteCommandChunk(gfxCtx, chunkHead + (static_cast<std::size_t>(idx) << 6));
                    ++idx;
                    if (stop != 0) {
                        break;
                    }
                }
                chunkHead = *nextBlockPtr;
            }
        }

        CGIndexBufferCacheGnm_Reset(ibCache);
        CGRasterizerCacheGnm_Reset(rsCache);
        CGOutputMergerCacheGnm_Reset(omCache);
        CGShaderResourceBinderGnm_ResetStateCaches(srBinder);

        std::uint64_t slotIdx0 = (*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB6E0) & 1u) * 9ULL;
        std::memcpy(ctxBase + 0xB5B8 + slotIdx0 * 8, cmdList + 0x68, 0x40);

        std::uint64_t slotIdx1 = ((*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB6E0) >> 1) & 1u) * 9ULL;
        *reinterpret_cast<std::uint64_t*>(ctxBase + 0xB5F8 + slotIdx1 * 8) =
            *reinterpret_cast<std::uint64_t*>(cmdList + 0xA8);

        auto* alloc = *reinterpret_cast<DLAllocator**>(cmdList + 0x00);
        if (alloc) {
            std::uint64_t nextLinkOffset = static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(cmdList + 0x18)) << 6;
            auto* block = *reinterpret_cast<std::uint8_t**>(cmdList + 0x08);
            while (block) {
                auto* nextBlock = *reinterpret_cast<std::uint8_t**>(block + nextLinkOffset);
                alloc = *reinterpret_cast<DLAllocator**>(cmdList + 0x00);
                alloc->vptr->Free(alloc, block);
                block = nextBlock;
            }
        }
        *reinterpret_cast<void**>(cmdList + 0x08) = nullptr;
        *reinterpret_cast<void**>(cmdList + 0x10) = nullptr;
        std::memset(cmdList + 0x1C, 0, 0x94);
    }

    CGGfxContextGnm_FinalizeBatchCommandListExecution(gfxCtx);
}

// [VMA: 0x216a7d0 | Size: 428 B]
void GfxContext_ExecuteCommandList_Single(
    void* context,
    void* unused,
    void* commandList)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* syncTracker = ctxBase + 0xB478;

    if (CGSurfaceSyncTrackerGnm_IsDisabled(syncTracker)) {
        CGSurfaceSyncTrackerGnm_Enable(syncTracker);
    }
    CGDynamicBufferRingGnm_FlushToGfxContext(ctxBase + 0xB6F8, gfxCtx);
    CGConstantBufferUpdaterGnm_FlushToGfxContext(ctxBase + 0xB3C0, ctxBase);

    auto* cmdList = static_cast<std::uint8_t*>(sub_26d5740(commandList));
    void* omCache = ctxBase + 0xB558;
    CGOutputMergerCacheGnm_DecompressUnboundTargets(omCache, ctxBase, cmdList + 0x20);
    CGSurfaceSyncTrackerGnm_FlushToGfxContext(syncTracker, gfxCtx);
    CGGfxContextGnm_FlushAndSubmitImmediate(gfxCtx);
    CGCommandListGnm_SubmitRecordedChunks(cmdList);

    CGIndexBufferCacheGnm_Reset(ctxBase + 0xB518);
    CGRasterizerCacheGnm_Reset(ctxBase + 0xB528);
    CGOutputMergerCacheGnm_Reset(omCache);
    CGShaderResourceBinderGnm_ResetStateCaches(ctxBase + 0x50);

    std::uint64_t slotIdx0 = (*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB6E0) & 1u) * 9ULL;
    std::memcpy(ctxBase + 0xB5B8 + slotIdx0 * 8, cmdList + 0x68, 0x40);

    std::uint64_t slotIdx1 = ((*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB6E0) >> 1) & 1u) * 9ULL;
    *reinterpret_cast<std::uint64_t*>(ctxBase + 0xB5F8 + slotIdx1 * 8) =
        *reinterpret_cast<std::uint64_t*>(cmdList + 0xA8);

    auto* alloc = *reinterpret_cast<DLAllocator**>(cmdList + 0x00);
    if (alloc) {
        std::uint64_t nextLinkOffset = static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(cmdList + 0x18)) << 6;
        auto* block = *reinterpret_cast<std::uint8_t**>(cmdList + 0x08);
        while (block) {
            auto* nextBlock = *reinterpret_cast<std::uint8_t**>(block + nextLinkOffset);
            alloc = *reinterpret_cast<DLAllocator**>(cmdList + 0x00);
            alloc->vptr->Free(alloc, block);
            block = nextBlock;
        }
    }
    *reinterpret_cast<void**>(cmdList + 0x08) = nullptr;
    *reinterpret_cast<void**>(cmdList + 0x10) = nullptr;
    std::memset(cmdList + 0x1C, 0, 0x94);
}

// [VMA: 0x216a7c0 | Size: 7 B]
std::uint32_t GXGnmCommandContext_GetContextType(const void* context) {
    return *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(context) + 0xB510);
}

// [VMA: 0x216a770 | Size: 73 B]
void GXGnmCommandContext_SetPredication(void* context, void* unused, void* predicate, bool predicateValue) {
    void* gfxCtx = static_cast<std::uint8_t*>(context) + 0x10;
    if (predicate) {
        auto* predView = static_cast<std::uint8_t*>(CGDescriptorViewGnm_Identity(predicate));
        void* predGpuAddr = *reinterpret_cast<void**>(predView + 0x10);
        std::uint32_t predAction = static_cast<std::uint32_t>(predicateValue) ^ 1u;
        sub_1077570(gfxCtx, predGpuAddr, 0, predAction);
    } else {
        sub_1077600(gfxCtx);
    }
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x216a2f0 | Size: 1152 B]
std::uint32_t GXGnmCommandContext_GetDataFromQuery(
    void* context,
    void* unused,
    void* queryObj,
    void* outData,
    std::uint32_t dataSize,
    bool flushIfNeeded)
{
    if (flushIfNeeded) {
        CGGfxContextGnm_FlushAndSubmitImmediate(static_cast<std::uint8_t*>(context) + 0x10);
    }

    auto* queryView = static_cast<std::uint8_t*>(CGDescriptorViewGnm_Identity(queryObj));
    std::uint32_t queryType = *reinterpret_cast<const std::uint32_t*>(queryView + 0x04);
    if (queryType > 15) {
        return 7;
    }

    auto* gpuData = *reinterpret_cast<const std::uint64_t* const*>(queryView + 0x10);
    switch (queryType) {
        case 0: {
            if (gpuData[0] == 0) {
                if (outData) {
                    *static_cast<std::uint32_t*>(outData) = 0;
                }
                return 0xA;
            }
            if (outData) {
                *static_cast<std::uint32_t*>(outData) = 1;
            }
            return 0;
        }
        case 1: {
            std::uint64_t combined = gpuData[0];
            for (int i = 1; i < 16; ++i) {
                combined &= gpuData[i];
            }
            if ((combined & 0x8000000000000000ULL) == 0) {
                return 0xA;
            }
            if (outData) {
                std::uint64_t passedSamples = 0;
                for (int i = 0; i < 8; ++i) {
                    passedSamples += (gpuData[i * 2 + 1] - gpuData[i * 2]);
                }
                *static_cast<std::uint64_t*>(outData) = passedSamples;
            }
            return 0;
        }
        case 2: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                *static_cast<std::uint64_t*>(outData) = gpuData[1];
            }
            return 0;
        }
        case 3: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                auto* outU64 = static_cast<std::uint64_t*>(outData);
                outU64[0] = 0x5F5E100ULL; // 100,000,000 Hz GPU timestamp frequency
                *reinterpret_cast<std::uint8_t*>(outU64 + 1) = (gpuData[1] < gpuData[2]) ? 1u : 0u;
            }
            return 0;
        }
        case 4: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                auto* outStats = static_cast<std::uint64_t*>(outData);
                outStats[0]  = gpuData[19] - gpuData[8];  // +0x98 - +0x40
                outStats[1]  = gpuData[18] - gpuData[7];  // +0x90 - +0x38
                outStats[2]  = gpuData[15] - gpuData[4];  // +0x78 - +0x20
                outStats[3]  = gpuData[16] - gpuData[5];  // +0x80 - +0x28
                outStats[4]  = gpuData[17] - gpuData[6];  // +0x88 - +0x30
                outStats[5]  = gpuData[14] - gpuData[3];  // +0x70 - +0x18
                outStats[6]  = gpuData[13] - gpuData[2];  // +0x68 - +0x10
                outStats[7]  = gpuData[12] - gpuData[1];  // +0x60 - +0x08
                outStats[8]  = gpuData[20] - gpuData[9];  // +0xA0 - +0x48
                outStats[9]  = gpuData[21] - gpuData[10]; // +0xA8 - +0x50
                outStats[10] = gpuData[22] - gpuData[11]; // +0xB0 - +0x58
            }
            return 0;
        }
        case 5: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                auto* outSo = static_cast<std::uint64_t*>(outData);
                outSo[0] = (gpuData[10] - gpuData[2]) + (gpuData[12] - gpuData[4])
                         + (gpuData[14] - gpuData[6]) + (gpuData[16] - gpuData[8]);
                outSo[1] = (gpuData[9]  - gpuData[1]) + (gpuData[11] - gpuData[3])
                         + (gpuData[13] - gpuData[5]) + (gpuData[15] - gpuData[7]);
            }
            return 0;
        }
        case 6:
        case 7:
        case 8:
        case 9: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                auto* outSo = static_cast<std::uint64_t*>(outData);
                outSo[0] = gpuData[4] - gpuData[2];
                outSo[1] = gpuData[3] - gpuData[1];
            }
            return 0;
        }
        case 10: {
            std::uint64_t combined = gpuData[0];
            for (int i = 1; i < 16; ++i) {
                combined &= gpuData[i];
            }
            if ((combined & 0x8000000000000000ULL) == 0) {
                return 0xA;
            }
            if (outData) {
                std::uint64_t acc = (gpuData[1] - gpuData[0])
                                  + (gpuData[3] - gpuData[2])
                                  + (gpuData[5] - gpuData[4])
                                  + (gpuData[7] - gpuData[6])
                                  + (gpuData[9] - gpuData[8])
                                  + (gpuData[11] - gpuData[10])
                                  + (gpuData[13] - gpuData[12])
                                  + gpuData[15];
                *static_cast<std::uint32_t*>(outData) = (acc != gpuData[14]) ? 1u : 0u;
            }
            return 0;
        }
        case 11: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                std::uint64_t numWritten = (gpuData[10] - gpuData[2]) + (gpuData[12] - gpuData[4])
                                         + (gpuData[14] - gpuData[6]) + (gpuData[16] - gpuData[8]);
                std::uint64_t numNeeded  = (gpuData[9]  - gpuData[1]) + (gpuData[11] - gpuData[3])
                                         + (gpuData[13] - gpuData[5]) + (gpuData[15] - gpuData[7]);
                *static_cast<std::uint32_t*>(outData) = (numWritten < numNeeded) ? 1u : 0u;
            }
            return 0;
        }
        case 12:
        case 13:
        case 14:
        case 15: {
            if (gpuData[0] == 0) {
                return 0xA;
            }
            if (outData) {
                std::uint64_t numWritten = gpuData[4] - gpuData[2];
                std::uint64_t numNeeded  = gpuData[3] - gpuData[1];
                *static_cast<std::uint32_t*>(outData) = (numWritten < numNeeded) ? 1u : 0u;
            }
            return 0;
        }
        default:
            return 7;
    }
}

// [VMA: 0x216a070 | Size: 636 B]
void GXGnmCommandContext_EndQuery(void* context, void* unused, void* queryObj) {
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    auto* queryView = static_cast<std::uint8_t*>(CGDescriptorViewGnm_Identity(queryObj));

    std::uint32_t viewCategory = *reinterpret_cast<const std::uint32_t*>(queryView + 0x00);
    std::uint32_t queryType = *reinterpret_cast<const std::uint32_t*>(queryView + 0x04);
    auto* gpuBuf = *reinterpret_cast<std::uint8_t**>(queryView + 0x10);

    if (viewCategory == 1) {
        if (queryType == 0) {
            if (!gx_direct_vk::TryDirectOcclusionQuery(gfxCtx, 1u, gpuBuf)) {
                sub_1077660(gfxCtx, 1, gpuBuf);
            }
        }
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    if (viewCategory != 0 || queryType > 15) {
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    switch (queryType) {
        case 0: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (!gx_direct_vk::TryDirectTimestampQuery(gfxCtx, 2u, gpuBuf)) {
                CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            }
            break;
        }
        case 1:
        case 10: {
            if (!gx_direct_vk::TryDirectOcclusionQuery(gfxCtx, 1u, gpuBuf)) {
                sub_1077660(gfxCtx, 1, gpuBuf);
            }
            ctxBase[0xB657] &= 0xE3u;
            break;
        }
        case 2:
        case 3: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (!gx_direct_vk::TryDirectTimestampQuery(gfxCtx, 1u, gpuBuf)) {
                CGCommandBufferGnm_WriteEndOfPipeCounter(gfxCtx, 4, gpuBuf + 0x08, 0);
                CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            }
            break;
        }
        case 4: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (--(*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720)) == 0) {
                sub_1075da0(gfxCtx, 0x1A);
            }
            sub_1077850(gfxCtx, 1, gpuBuf + 0x60);
            CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            break;
        }
        case 5:
        case 11: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (--(*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720)) == 0) {
                sub_1075da0(gfxCtx, 0x1A);
            }
            sub_1077850(gfxCtx, 2, gpuBuf + 0x48);
            sub_1077850(gfxCtx, 3, gpuBuf + 0x58);
            sub_1077850(gfxCtx, 4, gpuBuf + 0x68);
            sub_1077850(gfxCtx, 5, gpuBuf + 0x78);
            CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            break;
        }
        case 6:
        case 12: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (--(*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720)) == 0) {
                sub_1075da0(gfxCtx, 0x1A);
            }
            sub_1077850(gfxCtx, 2, gpuBuf + 0x18);
            CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            break;
        }
        case 7:
        case 13: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (--(*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720)) == 0) {
                sub_1075da0(gfxCtx, 0x1A);
            }
            sub_1077850(gfxCtx, 3, gpuBuf + 0x18);
            CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            break;
        }
        case 8:
        case 14: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (--(*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720)) == 0) {
                sub_1075da0(gfxCtx, 0x1A);
            }
            sub_1077850(gfxCtx, 4, gpuBuf + 0x18);
            CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            break;
        }
        case 9:
        case 15: {
            *reinterpret_cast<std::uint64_t*>(gpuBuf) = 0;
            if (--(*reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720)) == 0) {
                sub_1075da0(gfxCtx, 0x1A);
            }
            sub_1077850(gfxCtx, 5, gpuBuf + 0x18);
            CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, gpuBuf, 1, 0);
            break;
        }
        default:
            break;
    }

    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169ec0 | Size: 420 B]
void GXGnmCommandContext_BeginQuery(void* context, void* unused, void* queryObj) {
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    auto* queryView = static_cast<std::uint8_t*>(CGDescriptorViewGnm_Identity(queryObj));

    std::uint32_t viewCategory = *reinterpret_cast<const std::uint32_t*>(queryView + 0x00);
    std::uint32_t queryType = *reinterpret_cast<const std::uint32_t*>(queryView + 0x04);
    auto* gpuBuf = *reinterpret_cast<std::uint8_t**>(queryView + 0x10);

    if (viewCategory == 1) {
        if (queryType == 0) {
            ctxBase[0xB657] = (ctxBase[0xB657] & 0xE3u) | 0x04u;
            if (!gx_direct_vk::TryDirectOcclusionQuery(gfxCtx, 0u, gpuBuf)) {
                sub_1077660(gfxCtx, 0, gpuBuf);
            }
        }
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    if (viewCategory != 0 || queryType == 0 || queryType > 15) {
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    auto startPerfCounterAndSample = [&](std::uint32_t statIdx, std::uint8_t* sampleDst) {
        sub_1077850(gfxCtx, statIdx, sampleDst);
        std::uint32_t prevCount = *reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720);
        *reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720) = prevCount + 1;
        if (prevCount == 0) {
            sub_1075da0(gfxCtx, 0x19);
        }
    };

    switch (queryType) {
        case 1:
        case 10: {
            ctxBase[0xB657] = (ctxBase[0xB657] & 0xE3u) | 0x04u;
            if (!gx_direct_vk::TryDirectOcclusionQuery(gfxCtx, 0u, gpuBuf)) {
                sub_1077660(gfxCtx, 0, gpuBuf);
            }
            break;
        }
        case 2:
            break;
        case 3: {
            if (!gx_direct_vk::TryDirectTimestampQuery(gfxCtx, 0u, gpuBuf)) {
                CGCommandBufferGnm_WriteEndOfPipeCounter(gfxCtx, 4, gpuBuf + 0x10, 0);
            }
            break;
        }
        case 4: {
            startPerfCounterAndSample(1, gpuBuf + 0x08);
            break;
        }
        case 5:
        case 11: {
            sub_1077850(gfxCtx, 2, gpuBuf + 0x08);
            sub_1077850(gfxCtx, 3, gpuBuf + 0x18);
            sub_1077850(gfxCtx, 4, gpuBuf + 0x28);
            startPerfCounterAndSample(5, gpuBuf + 0x38);
            break;
        }
        case 6:
        case 12: {
            startPerfCounterAndSample(2, gpuBuf + 0x08);
            break;
        }
        case 7:
        case 13: {
            startPerfCounterAndSample(3, gpuBuf + 0x08);
            break;
        }
        case 8:
        case 14: {
            startPerfCounterAndSample(4, gpuBuf + 0x08);
            break;
        }
        case 9:
        case 15: {
            startPerfCounterAndSample(5, gpuBuf + 0x08);
            break;
        }
        default:
            break;
    }

    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169eb0 | Size: 1 B]
void GXGnmCommandContext_UnmapSubresourceNoOp() {}

extern "C" __attribute__((weak)) void BbGoldenEngine_NotifyGpuSurfaceWrite(void* dst, std::uint64_t size) {
    (void)dst;
    (void)size;
}
extern "C" __attribute__((weak)) void BbGoldenEngine_OnDrawCall(
    std::uint32_t count, std::uint32_t instanceCount, bool isIndexed, bool isIndirect) {
    (void)count;
    (void)instanceCount;
    (void)isIndexed;
    (void)isIndirect;
}
extern "C" __attribute__((weak)) void BbGoldenEngine_OnDispatchCall(
    std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ, bool isIndirect) {
    (void)groupX;
    (void)groupY;
    (void)groupZ;
    (void)isIndirect;
}

// [VMA: 0x2169c30 | Size: 639 B]
std::uint32_t GXGnmCommandContext_MapSubresource(
    void* context,
    void* unused,
    void* resource,
    std::uint32_t subresource,
    std::uint32_t mapType,
    void* outMappedSubresource)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    auto* resBase = static_cast<std::uint8_t*>(resource);
    void* resCore = resBase + 0x18;
    auto* mappedOut = static_cast<std::uint8_t*>(outMappedSubresource);

    if (mapType == 4) {
        std::uint64_t gpuAddr = CGDynamicBufferRingGnm_ResolveGpuAddress(ctxBase + 0xB6F8, resCore);
        if (*reinterpret_cast<std::uint64_t*>(resBase + 0x20) == gpuAddr) {
            return 4;
        }
        *reinterpret_cast<std::uint64_t*>(mappedOut + 0x00) = gpuAddr;
        std::uint32_t byteSize = *reinterpret_cast<std::uint32_t*>( static_cast<std::uint8_t*>(resCore) + 0x14 );
        *reinterpret_cast<std::uint32_t*>(mappedOut + 0x08) = byteSize;
        *reinterpret_cast<std::uint32_t*>(mappedOut + 0x0C) = byteSize;
        return 0;
    }

    void* gfxCtx = ctxBase + 0x10;
    if (mapType == 3) {
        if (resBase[0x38] != 1) {
            return 8;
        }
        void* mappedPtr = nullptr;
        std::uint32_t err = MapDynamicBuffer_B(ctxBase + 0xB6F8, gfxCtx, &mappedPtr, resCore);
        if (err != 0) {
            return err;
        }
        *reinterpret_cast<void**>(mappedOut + 0x00) = mappedPtr;
        std::uint32_t byteSize = *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(resCore) + 0x14);
        *reinterpret_cast<std::uint32_t*>(mappedOut + 0x08) = byteSize;
        *reinterpret_cast<std::uint32_t*>(mappedOut + 0x0C) = byteSize;
        return 0;
    }

    if (mapType > 2) {
        return 0;
    }

    auto* eopLabel = static_cast<volatile std::uint64_t*>(
        CGGfxContextGnm_AllocateFromCommandBufferAligned(gfxCtx, 8, 8));
    *eopLabel = 0;
    CGCommandBufferGnm_WriteEndOfPipeImmediate(gfxCtx, 4, const_cast<std::uint64_t*>(eopLabel), 1, 0);
    CGGfxContextGnm_FlushAndSubmitImmediate(gfxCtx);
    while (*eopLabel == 0) {
        reinterpret_cast<void (*)()>(GX_VMA(0x2bc07c8ULL))();
    }

    std::uint64_t subByteOffset = 0;
    std::uint64_t subByteSize = 0;
    std::uint32_t viewRes = CGTextureGnm_CreateSubresourceViewTSharp(
        &subByteOffset, &subByteSize, resCore, subresource, 0, 1);
    if (viewRes != 0) {
        return viewRes;
    }

    *reinterpret_cast<std::uint64_t*>(mappedOut + 0x00) =
        *reinterpret_cast<std::uint64_t*>(resBase + 0x20) + subByteOffset;

    std::uint8_t resType = resBase[0x38];
    if (static_cast<std::uint8_t>(resType - 2) >= 3u) {
        if (resType == 1) {
            const void* gpuBase = *reinterpret_cast<const void* const*>(resBase + 0x20);
            std::uint32_t byteSize = *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(resCore) + 0x14);
            std::uint32_t allocSize = *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(resCore) + 0x18);
            const std::uint32_t span = allocSize >= byteSize ? allocSize : byteSize;
            if (mapType != 0u && gpuBase != nullptr && span != 0u) {
                gx_direct_vk::PrepareDirectBufferWrite(gpuBase, span);
                gx_direct_vk::RetireDirectGpuRange(gpuBase, span);
            }
            *reinterpret_cast<std::uint32_t*>(mappedOut + 0x08) = byteSize;
            *reinterpret_cast<std::uint32_t*>(mappedOut + 0x0C) = byteSize;
            return 0;
        }
        return 8;
    }

    std::uint32_t numMips = static_cast<std::uint8_t*>(resCore)[0x32];
    std::uint32_t mipSlice = 0;
    std::uint32_t arraySlice = 0;
    if (numMips != 0) {
        mipSlice = subresource % numMips;
        arraySlice = subresource / numMips;
    }

    std::uint32_t isSrgb = (*reinterpret_cast<std::uint32_t*>(resBase + 0x3C) & 4u) >> 2;
    std::uint32_t format = resBase[0x4B];
    std::uint32_t arraySize = *reinterpret_cast<std::uint16_t*>(resBase + 0x48);

    std::uint64_t tempTSharp[4] = {};
    std::uint32_t srvErr = GXGnmShaderResourceView_InitializeForSurfaceMipSlice(
        tempTSharp, resCore, format, mipSlice, arraySlice, numMips, arraySize, isSrgb);
    if (srvErr != 0) {
        return srvErr;
    }

    std::uint32_t fmtIdx = format - 1;
    std::uint64_t blockW = 8;
    std::uint64_t blockH = 1;
    if (fmtIdx <= 0x62u) {
        blockW = g_GnmFormatBlockWidthTable[fmtIdx];
        blockH = g_GnmFormatBlockHeightTable[fmtIdx];
    }
    if (blockW == 0 || blockH == 0) {
        return 8;
    }

    std::uint64_t width = static_cast<std::uint64_t>(sub_1089890(tempTSharp) + 1u);
    std::uint64_t blocksX = (width + blockW - 1) / blockW;

    std::uint64_t height = static_cast<std::uint64_t>(sub_1089100(tempTSharp) + 1u);
    std::uint64_t blocksY = (height + blockH - 1) / blockH;

    std::uint32_t bytesPerElem = 0;
    if (fmtIdx <= 0x62u) {
        bytesPerElem = static_cast<std::uint32_t>(g_GnmFormatBytesPerElementTable[fmtIdx]);
    }
    std::uint32_t rowPitch = bytesPerElem * static_cast<std::uint32_t>(blocksX);
    *reinterpret_cast<std::uint32_t*>(mappedOut + 0x08) = rowPitch;
    *reinterpret_cast<std::uint32_t*>(mappedOut + 0x0C) = rowPitch * static_cast<std::uint32_t>(blocksY);
    return 0;
}

// [VMA: 0x2169ba0 | Size: 133 B]
void GXGnmCommandContext_DrawAuto(void* context, void* stateContext) {
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 1);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);
    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169ab0 | Size: 226 B]
void GXGnmCommandContext_DrawIndexedInstancedIndirect(
    void* context,
    void* stateContext,
    void* bufferForArgs,
    std::uint32_t alignedByteOffsetForArgs)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 1);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);

    std::uint64_t argsGpuAddr = *reinterpret_cast<std::uint64_t*>(static_cast<std::uint8_t*>(bufferForArgs) + 0x20);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_DrawIndirect)) {
        gx_direct_vk::EmitDrawIndirect(gfxCtx, argsGpuAddr, alignedByteOffsetForArgs, true);
    } else {
        sub_10742f0(gfxCtx, argsGpuAddr);

        std::uint32_t activeStagesMode = *reinterpret_cast<std::uint32_t*>(ctxBase + 0x50);
        std::uint32_t stageUsage = 4;
        if (activeStagesMode <= 0x1FFu) {
            if (activeStagesMode == 0) {
                stageUsage = 2;
            } else if (activeStagesMode == 0x45u || activeStagesMode == 0xADu) {
                stageUsage = 6;
            }
        } else if (activeStagesMode == 0x200u) {
            stageUsage = 2;
        }

        sub_10746f0(gfxCtx, alignedByteOffsetForArgs, stageUsage, 0, 0);
    }
    BbGoldenEngine_OnDrawCall(0, 1, true, true);
    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x21699d0 | Size: 223 B]
void GXGnmCommandContext_DrawInstancedIndirect(
    void* context,
    void* stateContext,
    void* bufferForArgs,
    std::uint32_t alignedByteOffsetForArgs)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 0);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);

    std::uint64_t argsGpuAddr = *reinterpret_cast<std::uint64_t*>(static_cast<std::uint8_t*>(bufferForArgs) + 0x20);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_DrawIndirect)) {
        gx_direct_vk::EmitDrawIndirect(gfxCtx, argsGpuAddr, alignedByteOffsetForArgs, false);
    } else {
        sub_10742f0(gfxCtx, argsGpuAddr);

        std::uint32_t activeStagesMode = *reinterpret_cast<std::uint32_t*>(ctxBase + 0x50);
        std::uint32_t stageUsage = 4;
        if (activeStagesMode <= 0x1FFu) {
            if (activeStagesMode == 0) {
                stageUsage = 2;
            } else if (activeStagesMode == 0x45u || activeStagesMode == 0xADu) {
                stageUsage = 6;
            }
        } else if (activeStagesMode == 0x200u) {
            stageUsage = 2;
        }

        sub_10744d0(gfxCtx, alignedByteOffsetForArgs, stageUsage, 0, 0);
    }
    BbGoldenEngine_OnDrawCall(0, 1, false, true);
    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169900 | Size: 194 B]
void GXGnmCommandContext_DrawIndexedInstanced(
    void* context,
    void* stateContext,
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 1);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);

    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_DrawDirect)) {
        gx_direct_vk::EmitDrawIndexed(gfxCtx, indexCountPerInstance, instanceCount,
                                      startIndexLocation, baseVertexLocation);
    } else {
        sub_1074040(gfxCtx, baseVertexLocation);
        sub_1075370(gfxCtx, instanceCount);
        sub_1074420(gfxCtx, startIndexLocation, indexCountPerInstance);
    }
    BbGoldenEngine_OnDrawCall(indexCountPerInstance, instanceCount, true, false);

    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169840 | Size: 181 B]
void GXGnmCommandContext_DrawInstanced(
    void* context,
    void* stateContext,
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 0);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);

    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_DrawDirect)) {
        gx_direct_vk::EmitDrawAuto(gfxCtx, vertexCountPerInstance, instanceCount,
                                   static_cast<std::int32_t>(startVertexLocation));
    } else {
        sub_1074040(gfxCtx, static_cast<std::int32_t>(startVertexLocation));
        sub_1075370(gfxCtx, instanceCount);
        sub_10741b0(gfxCtx, vertexCountPerInstance);
    }
    BbGoldenEngine_OnDrawCall(vertexCountPerInstance, instanceCount, false, false);

    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169780 | Size: 189 B]
void GXGnmCommandContext_DrawIndexed(
    void* context,
    void* stateContext,
    std::uint32_t indexCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 1);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);

    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_DrawDirect)) {
        gx_direct_vk::EmitDrawIndexed(gfxCtx, indexCount, 1u, startIndexLocation,
                                      baseVertexLocation);
    } else {
        sub_1074040(gfxCtx, baseVertexLocation);
        sub_1075370(gfxCtx, 1);
        sub_1074420(gfxCtx, startIndexLocation, indexCount);
    }
    BbGoldenEngine_OnDrawCall(indexCount, 1, true, false);

    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x21696c0 | Size: 180 B]
void GXGnmCommandContext_Draw(
    void* context,
    void* stateContext,
    std::uint32_t vertexCount,
    std::uint32_t startVertexLocation)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGOutputMergerCacheGnm_FlushBlendAndDepthStencilState(ctxBase + 0xB558, ctxBase, stateContext);
    CGIndexBufferCacheGnm_BindToGfxContext(ctxBase + 0xB518, ctxBase, stateContext, 0);
    CGRasterizerCacheGnm_FlushToGfxContext(ctxBase + 0xB528, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindGraphicsPipeline(srBinder, ctxBase, stateContext);

    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_DrawDirect)) {
        gx_direct_vk::EmitDrawAuto(gfxCtx, vertexCount, 1u,
                                   static_cast<std::int32_t>(startVertexLocation));
    } else {
        sub_1074040(gfxCtx, static_cast<std::int32_t>(startVertexLocation));
        sub_1075370(gfxCtx, 1);
        sub_10741b0(gfxCtx, vertexCount);
    }
    BbGoldenEngine_OnDrawCall(vertexCount, 1, false, false);

    CGShaderResourceBinderGnm_PostDrawSyncAndResetStreamout(srBinder, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2169640 | Size: 115 B]
void GXGnmCommandContext_DispatchIndirect(
    void* context,
    void* stateContext,
    void* bufferForArgs,
    std::uint32_t alignedByteOffsetForArgs)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindComputeStage(srBinder, ctxBase, stateContext);

    std::uint64_t argsGpuAddr = *reinterpret_cast<std::uint64_t*>(static_cast<std::uint8_t*>(bufferForArgs) + 0x20);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_Dispatch)) {
        gx_direct_vk::EmitDispatchIndirect(gfxCtx, argsGpuAddr, alignedByteOffsetForArgs);
    } else {
        sub_10742f0(gfxCtx, argsGpuAddr);
        sub_1074de0(gfxCtx, alignedByteOffsetForArgs);
    }
    BbGoldenEngine_OnDispatchCall(0, 0, 0, true);

    CGShaderResourceBinderGnm_PostDispatchSyncAndGdsReadback(srBinder, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x21695c0 | Size: 113 B]
void GXGnmCommandContext_Dispatch(
    void* context,
    void* stateContext,
    std::uint32_t threadGroupCountX,
    std::uint32_t threadGroupCountY,
    std::uint32_t threadGroupCountZ)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    void* gfxCtx = ctxBase + 0x10;
    void* srBinder = ctxBase + 0x50;

    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    CGShaderResourceBinderGnm_SyncAndBindComputeStage(srBinder, ctxBase, stateContext);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_Dispatch)) {
        gx_direct_vk::EmitDispatchDirect(gfxCtx, threadGroupCountX, threadGroupCountY,
                                         threadGroupCountZ);
    } else {
        sub_1074c50(gfxCtx, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }
    BbGoldenEngine_OnDispatchCall(threadGroupCountX, threadGroupCountY, threadGroupCountZ, false);
    CGShaderResourceBinderGnm_PostDispatchSyncAndGdsReadback(srBinder, ctxBase, stateContext);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// ============================================================================
// 3. Per-Stage Resource Dirty Bitmask Setters (0x2168c90 .. 0x21695b0)
// ============================================================================

namespace {

inline void Set128BitResourceRangeMask(std::uint8_t* stageDirtyBase, std::uint32_t startSlot, std::uint32_t numViews) {
    std::int64_t s = static_cast<std::int64_t>(startSlot);
    std::int64_t n = static_cast<std::int64_t>(numViews);
    std::int64_t end = s + n;

    std::int64_t maskS = (s - 64) >> 6;
    std::int64_t maskEnd = (end - 65) >> 6;

    std::uint64_t loStart = static_cast<std::uint64_t>((static_cast<std::int64_t>(static_cast<std::uint32_t>(maskS) | 0x40u)) & s);
    std::uint64_t hiStart = static_cast<std::uint64_t>((~maskS) & (s - 64));

    std::uint64_t loEnd = static_cast<std::uint64_t>((maskEnd | 0x40) & end);
    std::uint64_t hiEnd = static_cast<std::uint64_t>((~maskEnd) & (end - 64));

    std::uint64_t loCount = loEnd - loStart;
    std::uint64_t hiCount = hiEnd - hiStart;

    std::uint64_t loBits = ((~(loCount >> 6)) + (1ULL << (loCount & 63u))) << (loStart & 63u);
    std::uint64_t hiBits = ((~(hiCount >> 6)) + (1ULL << (hiCount & 63u))) << (hiStart & 63u);

    *reinterpret_cast<std::uint64_t*>(stageDirtyBase + 0x00) |= loBits;
    *reinterpret_cast<std::uint64_t*>(stageDirtyBase + 0x08) |= hiBits;
}

inline void MarkConstantBuffersBoundAtomic(
    void* context,
    std::size_t dirtyMaskOffset,
    std::uint64_t stageBitFlag,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    std::uint64_t slotMask = ((1ULL << numBuffers) - 1ULL) << (startSlot + 0x10);
    *reinterpret_cast<std::uint64_t*>(base + dirtyMaskOffset) |= slotMask;

    if (numBuffers == 0) {
        return;
    }
    std::uint64_t curBit = 1ULL << startSlot;
    for (std::uint32_t i = 0; i < numBuffers; ++i) {
        auto* cb = static_cast<std::uint8_t*>(constantBuffers[i]);
        if (cb) {
            std::uint64_t reqFlags = curBit | stageBitFlag;
            std::uint64_t existing = *reinterpret_cast<std::uint32_t*>(cb + 0x44);
            if ((existing & reqFlags) != reqFlags) {
                __sync_fetch_and_or(reinterpret_cast<volatile std::uint64_t*>(cb + 0x44), reqFlags);
            }
        }
        curBit <<= 1;
    }
}

} // anonymous namespace

// [VMA: 0x21695b0 | Size: 1 B]
void GXGnmCommandContext_CSSetShaderNoOp() {}

// [VMA: 0x2169560 | Size: 71 B]
void GXGnmCommandContext_CSSetUnorderedAccessViewsDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numUAVs,
    void* const* uavs,
    const std::uint32_t* initialCounts)
{
    auto* base = static_cast<std::uint8_t*>(context);
    std::uint64_t mask = ((1ULL << numUAVs) - 1ULL) << (startSlot + 0x24);
    *reinterpret_cast<std::uint64_t*>(base + 0x1FB8) |= mask;
    if (initialCounts && numUAVs) {
        auto* dst = reinterpret_cast<std::uint32_t*>(base + 0x1FD0) + startSlot;
        for (std::uint32_t i = 0; i < numUAVs; ++i) {
            dst[i] = initialCounts[i];
        }
    }
}

// [VMA: 0x21694f0 | Size: 97 B]
void GXGnmCommandContext_CSSetConstantBuffersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    MarkConstantBuffersBoundAtomic(context, 0x1FB8, 0x200000ULL, startSlot, numBuffers, constantBuffers);
}

// [VMA: 0x2169460 | Size: 143 B]
void GXGnmCommandContext_CSSetShaderResourcesDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViews)
{
    Set128BitResourceRangeMask(static_cast<std::uint8_t*>(context) + 0x1FA8, startSlot, numViews);
}

// [VMA: 0x2169440 | Size: 24 B]
void GXGnmCommandContext_CSSetSamplersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numSamplers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint64_t*>(base + 0x1FB8) |= (((1ULL << numSamplers) - 1ULL) << startSlot);
}

// [VMA: 0x2169430 | Size: 7 B]
void GXGnmCommandContext_IASetPrimitiveTopologyDirtyFlagA(void* context, void* unused, std::uint8_t topology) {
    static_cast<std::uint8_t*>(context)[0xB656] = topology;
}

// [VMA: 0x2169420 | Size: 7 B]
void GXGnmCommandContext_IASetPrimitiveTopologyDirtyFlagB(void* context, void* unused, std::uint8_t topology) {
    static_cast<std::uint8_t*>(context)[0xB656] = topology;
}

// [VMA: 0x2169410 | Size: 1 B]
void GXGnmCommandContext_IASetIndexBufferNoOp() {}

// [VMA: 0x2169400 | Size: 1 B]
void GXGnmCommandContext_IASetVertexBuffersNoOp() {}

// [VMA: 0x21693f0 | Size: 1 B]
void GXGnmCommandContext_IASetInputLayoutNoOp() {}

// [VMA: 0x21693e0 | Size: 1 B]
void GXGnmCommandContext_DSSetShaderNoOp() {}

// [VMA: 0x2169370 | Size: 97 B]
void GXGnmCommandContext_DSSetConstantBuffersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    MarkConstantBuffersBoundAtomic(context, 0x1978, 0x100000ULL, startSlot, numBuffers, constantBuffers);
}

// [VMA: 0x21692e0 | Size: 143 B]
void GXGnmCommandContext_DSSetShaderResourcesDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViews)
{
    Set128BitResourceRangeMask(static_cast<std::uint8_t*>(context) + 0x1968, startSlot, numViews);
}

// [VMA: 0x21692c0 | Size: 24 B]
void GXGnmCommandContext_DSSetSamplersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numSamplers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint64_t*>(base + 0x1978) |= (((1ULL << numSamplers) - 1ULL) << startSlot);
}

// [VMA: 0x21692b0 | Size: 14 B]
void GXGnmCommandContext_SOSetTargetsCountThunk(void* context, void* unused, std::uint32_t numBuffers) {
    CGRasterizerCacheGnm_MarkScissorsDirty(static_cast<std::uint8_t*>(context) + 0xB528, numBuffers);
}

// [VMA: 0x21692a0 | Size: 14 B]
void GXGnmCommandContext_SOSetTargetsThunk(void* context, void* unused, std::uint32_t numBuffers) {
    CGRasterizerCacheGnm_MarkViewportsDirty(static_cast<std::uint8_t*>(context) + 0xB528, numBuffers);
}

// [VMA: 0x2169290 | Size: 1 B]
void GXGnmCommandContext_OMSetDepthStencilStateNoOp() {}

// [VMA: 0x21691a0 | Size: 227 B]
void GXGnmCommandContext_OMSetRenderTargetsAndUnorderedAccessViewsDirtyMask(
    void* context,
    void* unused,
    std::uint32_t numUAVs,
    void* const* uavs,
    const std::uint32_t* initialCounts)
{
    (void)unused;
    (void)uavs;
    auto* base = static_cast<std::uint8_t*>(context);
    if (numUAVs != 0) {
        auto* dst = reinterpret_cast<std::uint32_t*>(base + 0x2920);
        for (std::uint32_t i = 0; i < numUAVs; ++i) {
            dst[i] = initialCounts[i];
        }
    }
    std::uint64_t uavBits = ((1ULL << numUAVs) - 1ULL) << 52;
    *reinterpret_cast<std::uint64_t*>(base + 0x1338) |= uavBits;
}

// [VMA: 0x2169190 | Size: 1 B]
void GXGnmCommandContext_PSSetShaderNoOp() {}

// [VMA: 0x2169120 | Size: 97 B]
void GXGnmCommandContext_PSSetConstantBuffersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    MarkConstantBuffersBoundAtomic(context, 0x1338, 0x80000ULL, startSlot, numBuffers, constantBuffers);
}

// [VMA: 0x2169090 | Size: 143 B]
void GXGnmCommandContext_PSSetShaderResourcesDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViews)
{
    Set128BitResourceRangeMask(static_cast<std::uint8_t*>(context) + 0x1328, startSlot, numViews);
}

// [VMA: 0x2169070 | Size: 24 B]
void GXGnmCommandContext_PSSetSamplersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numSamplers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint64_t*>(base + 0x1338) |= (((1ULL << numSamplers) - 1ULL) << startSlot);
}

// [VMA: 0x2169060 | Size: 1 B]
void GXGnmCommandContext_GSSetShaderNoOp() {}

// [VMA: 0x2168ff0 | Size: 97 B]
void GXGnmCommandContext_GSSetConstantBuffersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    MarkConstantBuffersBoundAtomic(context, 0x0CF8, 0x40000ULL, startSlot, numBuffers, constantBuffers);
}

// [VMA: 0x2168f60 | Size: 143 B]
void GXGnmCommandContext_GSSetShaderResourcesDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViews)
{
    Set128BitResourceRangeMask(static_cast<std::uint8_t*>(context) + 0x0CE8, startSlot, numViews);
}

// [VMA: 0x2168f40 | Size: 24 B]
void GXGnmCommandContext_GSSetSamplersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numSamplers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint64_t*>(base + 0x0CF8) |= (((1ULL << numSamplers) - 1ULL) << startSlot);
}

// [VMA: 0x2168f30 | Size: 1 B]
void GXGnmCommandContext_HSSetShaderNoOp() {}

// [VMA: 0x2168ec0 | Size: 97 B]
void GXGnmCommandContext_HSSetConstantBuffersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    MarkConstantBuffersBoundAtomic(context, 0x06B8, 0x20000ULL, startSlot, numBuffers, constantBuffers);
}

// [VMA: 0x2168e30 | Size: 143 B]
void GXGnmCommandContext_HSSetShaderResourcesDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViews)
{
    Set128BitResourceRangeMask(static_cast<std::uint8_t*>(context) + 0x06A8, startSlot, numViews);
}

// [VMA: 0x2168e10 | Size: 24 B]
void GXGnmCommandContext_HSSetSamplersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numSamplers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint64_t*>(base + 0x06B8) |= (((1ULL << numSamplers) - 1ULL) << startSlot);
}

// [VMA: 0x2168e00 | Size: 1 B]
void GXGnmCommandContext_VSSetShaderNoOp() {}

// [VMA: 0x2168d90 | Size: 97 B]
void GXGnmCommandContext_VSSetConstantBuffersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numBuffers,
    void* const* constantBuffers)
{
    MarkConstantBuffersBoundAtomic(context, 0x0078, 0x10000ULL, startSlot, numBuffers, constantBuffers);
}

// [VMA: 0x2168d00 | Size: 137 B]
void GXGnmCommandContext_VSSetShaderResourcesDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViews)
{
    Set128BitResourceRangeMask(static_cast<std::uint8_t*>(context) + 0x0068, startSlot, numViews);
}

// [VMA: 0x2168ce0 | Size: 21 B]
void GXGnmCommandContext_VSSetSamplersDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numSamplers)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint64_t*>(base + 0x0078) |= (((1ULL << numSamplers) - 1ULL) << startSlot);
}

// [VMA: 0x2168cc0 | Size: 23 B]
void GXGnmCommandContext_RSSetViewportsDirtyMask(
    void* context,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t numViewports)
{
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<std::uint32_t*>(base + 0x25F0) |= static_cast<std::uint32_t>(((1ULL << numViewports) - 1ULL) << startSlot);
}

// [VMA: 0x2168cb0 | Size: 1 B]
void GXGnmCommandContext_RSSetScissorRectsNoOp() {}

// [VMA: 0x2168ca0 | Size: 1 B]
void GXGnmCommandContext_RSSetStateNoOp() {}

// [VMA: 0x2168c90 | Size: 1 B]
void GXGnmCommandContext_OMSetBlendStateNoOp() {}

// ============================================================================
// 4. Gnm Command Context Resource Copy, Clear & Slot Lifecycle (0x2168080 .. 0x2168be0)
// ============================================================================

// [VMA: 0x2168be0 | Size: 161 B]
void GXGnmCommandContext_CopyStructureCount(
    void* context,
    void* unused,
    void* dstBuffer,
    std::uint32_t dstAlignedByteOffset,
    void* srcUAV)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    auto* dstBase = static_cast<std::uint8_t*>(dstBuffer);
    auto* srcUavRes = *reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(srcUAV) + 0x08);

    void* dstCore = dstBase + 0x18;
    void* gfxCtx = ctxBase + 0x10;
    void* syncTracker = ctxBase + 0xB478;

    std::uint32_t token = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(syncTracker, 4, dstCore);
    CGSurfaceSyncTrackerGnm_FlushToGfxContext(syncTracker, gfxCtx);

    std::uint64_t srcCounterGpuAddr = *reinterpret_cast<std::uint64_t*>(srcUavRes + 0x20)
                                    + static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(srcUavRes + 0x30))
                                    - 8ULL;
    std::uint64_t dstGpuAddr = *reinterpret_cast<std::uint64_t*>(dstBase + 0x20) + dstAlignedByteOffset;

    sub_1073e60(gfxCtx, 0, dstGpuAddr, 0, srcCounterGpuAddr, 4, 1);

    CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(syncTracker, token, 4, dstCore);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
}

// [VMA: 0x2168b60 | Size: 114 B]
void GXGnmCommandContext_ResolveSubresource(
    void* context,
    void* unused,
    void* dstResource,
    std::uint32_t dstSubresource,
    void* srcResource,
    std::uint32_t srcSubresource,
    std::uint32_t format)
{
    auto* srcBase = static_cast<std::uint8_t*>(srcResource);
    auto* dstBase = static_cast<std::uint8_t*>(dstResource);

    if (*reinterpret_cast<const std::uint16_t*>(srcBase + 0x4C) == 1) {
        GXGnmCommandContext_CopySubresourceRegion(
            context, unused, dstResource, dstSubresource, 0, 0, 0, srcResource, srcSubresource, nullptr);
        return;
    }

    CGGpuBlitAndClearPassesGnm_CopyDualRenderTargetsFullscreen(
        context, dstBase + 0x18, dstSubresource, srcBase + 0x18, srcSubresource, format);
    CGGfxContextGnm_FlushPendingStagingUploads(static_cast<std::uint8_t*>(context) + 0x10);
}

// [VMA: 0x2168a20 | Size: 311 B]
void GfxContext_UpdateSubresource(
    void* context,
    void* stateContext,
    void* dstResource,
    std::uint32_t dstSubresource,
    const void* dstBox,
    const void* srcData,
    std::uint32_t srcRowPitch,
    std::uint32_t srcDepthPitch)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    auto* dstBase = static_cast<std::uint8_t*>(dstResource);
    void* gfxCtx = ctxBase + 0x10;

    std::uint8_t bindFlags = dstBase[0x3B];
    if ((bindFlags & 0x04u) != 0) {
        CGConstantBufferUpdaterGnm_UpdateSubresource(
            ctxBase + 0xB3C0, ctxBase, stateContext, dstResource, srcData);
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    if ((bindFlags & 0x20u) != 0) {
        CGOutputMergerCacheGnm_DecompressColorTextureMips(ctxBase + 0xB558, ctxBase, dstResource);
    }
    if ((bindFlags & 0x40u) != 0) {
        CGOutputMergerCacheGnm_DecompressDepthTextureMips(ctxBase + 0xB558, ctxBase, dstResource);
    }

    std::uint8_t resType = dstBase[0x38];
    void* dstCore = dstBase + 0x18;
    if (static_cast<std::uint8_t>(resType - 2) < 3u) {
        struct SubresourceDataDesc {
            const void* pSysMem;
            std::uint32_t sysMemPitch;
            std::uint32_t sysMemSlicePitch;
        } subDesc = { srcData, srcRowPitch, srcDepthPitch };
        CGGpuBlitAndClearPassesGnm_UpdateTextureSubresourceViaCompute(
            gfxCtx, dstCore, &subDesc, dstSubresource, dstBox, 1);
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    if (resType == 1) {
        std::uint32_t dstOff = 0;
        std::uint32_t byteCount = 0;
        if (dstBox) {
            const auto* boxU32 = static_cast<const std::uint32_t*>(dstBox);
            dstOff = boxU32[0];
            byteCount = boxU32[3] - dstOff;
        } else {
            byteCount = *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(dstCore) + 0x14);
        }
        std::uint64_t alignedSize = (static_cast<std::uint64_t>(byteCount) + 3ULL) & 0x1FFFFFFFCULL;
        CGGpuBlitAndClearPassesGnm_UpdateBufferSubresourceViaCompute(
            gfxCtx, dstCore, srcData, dstOff, alignedSize);
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
    }
}

// [VMA: 0x2168880 | Size: 409 B]
void GXGnmCommandContext_CopySubresourceRegion(
    void* context,
    void* unused,
    void* dstResource,
    std::uint32_t dstSubresource,
    std::uint32_t dstX,
    std::uint32_t dstY,
    std::uint32_t dstZ,
    void* srcResource,
    std::uint32_t srcSubresource,
    const void* srcBox)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    auto* dstBase = static_cast<std::uint8_t*>(dstResource);
    auto* srcBase = static_cast<std::uint8_t*>(srcResource);

    std::uint8_t dstFlags = dstBase[0x3B];
    if ((dstFlags & 0x20u) != 0) {
        CGOutputMergerCacheGnm_DecompressColorTextureMips(ctxBase + 0xB558, ctxBase, dstBase);
    }
    if ((dstFlags & 0x40u) != 0) {
        CGOutputMergerCacheGnm_DecompressDepthTextureMips(ctxBase + 0xB558, ctxBase, dstBase);
    }

    std::uint8_t srcFlags = srcBase[0x3B];
    if ((srcFlags & 0x20u) != 0) {
        CGOutputMergerCacheGnm_DecompressColorTextureMips(ctxBase + 0xB558, ctxBase, srcBase);
    }
    if ((srcFlags & 0x40u) != 0) {
        CGOutputMergerCacheGnm_DecompressDepthTextureMips(ctxBase + 0xB558, ctxBase, srcBase);
    }

    void* dstCore = dstBase + 0x18;
    if ((static_cast<std::uint8_t*>(dstCore)[0x23] & 0x04u) != 0) {
        void* gfxCtx = ctxBase + 0x10;
        ctxBase[0xB498] |= 0x01u;
        CGSurfaceSyncTrackerGnm_FlushToGfxContext(ctxBase + 0xB478, gfxCtx);
        GpuUtil_SingleCopy(
            gfxCtx,
            *reinterpret_cast<std::uint64_t*>(dstBase + 0x20),
            0x6E0000u,
            *reinterpret_cast<std::uint64_t*>(srcBase + 0x20),
            0x6E0000u,
            *reinterpret_cast<std::uint32_t*>(dstBase + 0x2C));
        ctxBase[0xB498] |= 0x01u;
        ctxBase[0xB478] |= 0x01u;
        ctxBase[0xB47F] |= 0x08u;
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    void* srcCore = srcBase + 0x18;
    std::uint8_t resType = dstBase[0x38];
    if (static_cast<std::uint8_t>(resType - 2) < 3u) {
        CGGpuBlitAndClearPassesGnm_CopyTextureRegionViaCompute(
            ctxBase, dstCore, dstSubresource, dstX, dstY, dstZ, srcCore, srcSubresource, srcBox);
        CGGfxContextGnm_FlushPendingStagingUploads(ctxBase + 0x10);
        return;
    }

    if (resType == 1) {
        std::uint32_t srcOff = 0;
        std::uint32_t byteCount = 0;
        if (srcBox) {
            const auto* boxU32 = static_cast<const std::uint32_t*>(srcBox);
            srcOff = boxU32[0];
            byteCount = boxU32[3] - srcOff;
        } else {
            byteCount = *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(dstCore) + 0x14);
        }
        std::uint64_t alignedSize = (static_cast<std::uint64_t>(byteCount) + 3ULL) & 0x1FFFFFFFCULL;
        CGGpuBlitAndClearPassesGnm_CopyBufferRegionViaCompute(
            ctxBase, dstCore, srcCore, dstX, srcOff, alignedSize);
        CGGfxContextGnm_FlushPendingStagingUploads(ctxBase + 0x10);
    }
}

// [VMA: 0x2168660 | Size: 536 B]
void GXGnmCommandContext_CopyResource(void* context, void* unused, void* dstResource, void* srcResource) {
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    auto* dstBase = static_cast<std::uint8_t*>(dstResource);
    auto* srcBase = static_cast<std::uint8_t*>(srcResource);

    std::uint8_t dstFlags = dstBase[0x3B];
    if ((dstFlags & 0x20u) != 0) {
        CGOutputMergerCacheGnm_DecompressColorTextureMips(ctxBase + 0xB558, ctxBase, dstBase);
    }
    if ((dstFlags & 0x40u) != 0) {
        CGOutputMergerCacheGnm_DecompressDepthTextureMips(ctxBase + 0xB558, ctxBase, dstBase);
    }

    std::uint8_t srcFlags = srcBase[0x3B];
    if ((srcFlags & 0x20u) != 0) {
        CGOutputMergerCacheGnm_DecompressColorTextureMips(ctxBase + 0xB558, ctxBase, srcBase);
    }
    if ((srcFlags & 0x40u) != 0) {
        CGOutputMergerCacheGnm_DecompressDepthTextureMips(ctxBase + 0xB558, ctxBase, srcBase);
    }

    void* dstCore = dstBase + 0x18;
    if ((static_cast<std::uint8_t*>(dstCore)[0x23] & 0x04u) != 0) {
        void* gfxCtx = ctxBase + 0x10;
        ctxBase[0xB498] |= 0x01u;
        CGSurfaceSyncTrackerGnm_FlushToGfxContext(ctxBase + 0xB478, gfxCtx);
        GpuUtil_SingleCopy(
            gfxCtx,
            *reinterpret_cast<std::uint64_t*>(dstBase + 0x20),
            0x6E0000u,
            *reinterpret_cast<std::uint64_t*>(srcBase + 0x20),
            0x6E0000u,
            *reinterpret_cast<std::uint32_t*>(dstBase + 0x2C));
        ctxBase[0xB498] |= 0x01u;
        ctxBase[0xB478] |= 0x01u;
        ctxBase[0xB47F] |= 0x08u;
        CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);
        return;
    }

    void* srcCore = srcBase + 0x18;
    std::uint8_t resType = dstBase[0x38];
    if (static_cast<std::uint8_t>(resType - 2) < 3u) {
        if (*reinterpret_cast<std::uint32_t*>(dstBase + 0x40) == *reinterpret_cast<std::uint32_t*>(srcBase + 0x40)) {
            std::uint64_t dstTotalBytes = *reinterpret_cast<std::uint32_t*>(dstBase + 0x2C);
            std::uint64_t srcTotalBytes = *reinterpret_cast<std::uint32_t*>(srcBase + 0x2C);
            std::uint64_t copySize = (dstTotalBytes < srcTotalBytes) ? dstTotalBytes : srcTotalBytes;

            void* gfxCtx = ctxBase + 0x10;
            void* syncTracker = ctxBase + 0xB478;
            std::uint32_t dstSync = CGSurfaceSyncTrackerGnm_TransitionResourceForRead(syncTracker, 0, dstCore);
            std::uint32_t srcSync = CGSurfaceSyncTrackerGnm_TransitionResourceForWrite(syncTracker, 0, srcCore);
            GpuUtil_SingleCopy(
                gfxCtx,
                *reinterpret_cast<std::uint64_t*>(dstBase + 0x20),
                dstSync,
                *reinterpret_cast<std::uint64_t*>(srcBase + 0x20),
                srcSync,
                copySize);
            CGSurfaceSyncTrackerGnm_TransitionResourcePostOp(syncTracker, dstSync, 0, dstCore);
        } else {
            std::uint32_t mipCount = static_cast<std::uint8_t*>(dstCore)[0x32];
            std::uint32_t arrayCount = (resType == 4) ? 1u : static_cast<std::uint32_t>(*reinterpret_cast<std::uint16_t*>(dstBase + 0x48));
            std::uint32_t totalSubresources = mipCount * arrayCount;
            for (std::uint32_t sub = 0; sub < totalSubresources; ++sub) {
                GXGnmCommandContext_CopySubresourceRegion(
                    ctxBase, unused, dstBase, sub, 0, 0, 0, srcBase, sub, nullptr);
            }
        }
        CGGfxContextGnm_FlushPendingStagingUploads(ctxBase + 0x10);
        return;
    }

    if (resType == 1) {
        std::uint32_t byteSize = *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(dstCore) + 0x14);
        CGGpuBlitAndClearPassesGnm_CopyBufferRegionViaCompute(ctxBase, dstCore, srcCore, 0, 0, byteSize);
        CGGfxContextGnm_FlushPendingStagingUploads(ctxBase + 0x10);
    }
}

// [VMA: 0x2168630 | Size: 47 B]
void GXGnmCommandContext_ClearDepthStencilView(
    void* context,
    void* unused,
    void* unorderedAccessView,
    const std::uint32_t values[4],
    std::uint32_t clearMask)
{
    auto* uavBase = static_cast<std::uint8_t*>(unorderedAccessView);
    auto* parentRes = *reinterpret_cast<std::uint8_t**>(uavBase + 0x08);
    CGGpuBlitAndClearPassesGnm_ClearUnorderedAccessViewUint(
        context, uavBase + 0x10, parentRes + 0x18, values, clearMask);
    CGGfxContextGnm_FlushPendingStagingUploads(static_cast<std::uint8_t*>(context) + 0x10);
}

// [VMA: 0x21685f0 | Size: 55 B]
void GXGnmCommandContext_ClearUnorderedAccessViewUint(
    void* context,
    void* unused,
    void* depthStencilView,
    std::uint32_t clearFlags,
    float depth,
    std::uint8_t stencil)
{
    auto* dsvBase = static_cast<std::uint8_t*>(depthStencilView);
    auto* parentRes = *reinterpret_cast<std::uint8_t**>(dsvBase + 0x08);
    CGGpuBlitAndClearPassesGnm_ClearDepthStencilTarget(
        context, dsvBase + 0x10, parentRes + 0x18, stencil, clearFlags, depth);
    CGGfxContextGnm_FlushPendingStagingUploads(static_cast<std::uint8_t*>(context) + 0x10);
}

// [VMA: 0x2168530 | Size: 191 B]
void GXGnmCommandContext_ClearState(void* context) {
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    std::uint64_t emptyTargets[9] = {};

    void* omCache = ctxBase + 0xB558;
    CGOutputMergerCacheGnm_DecompressUnboundTargets(omCache, ctxBase, emptyTargets);

    void* gfxCtx = ctxBase + 0x10;
    CGSurfaceSyncTrackerGnm_FlushToGfxContext(ctxBase + 0xB478, gfxCtx);
    CGShaderResourceBinderGnm_Reset(ctxBase + 0x50, ctxBase);
    CGGfxContextGnm_FlushPendingStagingUploads(gfxCtx);

    CGIndexBufferCacheGnm_Reset(ctxBase + 0xB518);
    CGRasterizerCacheGnm_Reset(ctxBase + 0xB528);
    CGOutputMergerCacheGnm_Reset(omCache);
}

// [VMA: 0x2168400 | Size: 296 B]
void GXGnmCommandContext_CopyConstruct(void* dstContext, const void* srcContext) {
    auto* dst = static_cast<std::uint8_t*>(dstContext);
    const auto* src = static_cast<const std::uint8_t*>(srcContext);

    *reinterpret_cast<void**>(dst + 0x00) = *reinterpret_cast<void* const*>(src + 0x00);
    *reinterpret_cast<void**>(dst + 0x08) = *reinterpret_cast<void* const*>(src + 0x08);
    CGGfxContextGnm_CopyConstruct(dst + 0x10, src + 0x10);

    *reinterpret_cast<std::uint32_t*>(dst + 0xB510) = *reinterpret_cast<const std::uint32_t*>(src + 0xB510);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB518) = *reinterpret_cast<const std::uint64_t*>(src + 0xB518);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB520) = *reinterpret_cast<const std::uint64_t*>(src + 0xB520);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB550) = *reinterpret_cast<const std::uint64_t*>(src + 0xB550);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB548) = *reinterpret_cast<const std::uint64_t*>(src + 0xB548);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB540) = *reinterpret_cast<const std::uint64_t*>(src + 0xB540);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB538) = *reinterpret_cast<const std::uint64_t*>(src + 0xB538);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB528) = *reinterpret_cast<const std::uint64_t*>(src + 0xB528);
    *reinterpret_cast<std::uint64_t*>(dst + 0xB530) = *reinterpret_cast<const std::uint64_t*>(src + 0xB530);

    CGOutputMergerCacheGnm_Copy(dst + 0xB558, src + 0xB558);
    CGDynamicBufferRingGnm_MoveInit(dst + 0xB6F8, const_cast<std::uint8_t*>(src) + 0xB6F8);

    *reinterpret_cast<std::uint64_t*>(dst + 0xB718) = *reinterpret_cast<const std::uint64_t*>(src + 0xB718);
    *reinterpret_cast<std::uint32_t*>(dst + 0xB720) = *reinterpret_cast<const std::uint32_t*>(src + 0xB720);
    std::uint16_t slotId = *reinterpret_cast<const std::uint16_t*>(src + 0xB724);
    *reinterpret_cast<std::uint16_t*>(dst + 0xB724) = slotId;
    *reinterpret_cast<std::uint64_t*>(dst + 0xB728) = *reinterpret_cast<const std::uint64_t*>(src + 0xB728);
    *reinterpret_cast<void**>(dst + 0xB4A0) = dstContext;

    if (slotId <= 0xFFu && g_pGXRegisteredCommandContexts[slotId] == srcContext) {
        g_pGXRegisteredCommandContexts[slotId] = dstContext;
    }
}

// [VMA: 0x2168380 | Size: 122 B]
void GXGnmCommandContext_Construct(void* context) {
    auto* base = static_cast<std::uint8_t*>(context);
    *reinterpret_cast<void**>(base + 0x08) = nullptr;
    *reinterpret_cast<void**>(base + 0x00) = nullptr;
    CGGfxContextGnm_Construct(base + 0x10);
    *reinterpret_cast<std::uint32_t*>(base + 0xB510) = 0;
    CGOutputMergerCacheGnm_Construct(base + 0xB558);
    CGDynamicBufferRingGnm_ZeroInit(base + 0xB6F8);
    *reinterpret_cast<void**>(base + 0xB718) = nullptr;
    *reinterpret_cast<std::uint32_t*>(base + 0xB720) = 0;
    *reinterpret_cast<std::uint16_t*>(base + 0xB724) = 0x0100;
    *reinterpret_cast<void**>(base + 0xB728) = nullptr;
    *reinterpret_cast<void**>(base + 0xB4A0) = context;
}

// [VMA: 0x21682c0 | Size: 186 B]
void GXGnmCommandContext_ShutdownAndUnregisterSlot(void* context) {
    auto* base = static_cast<std::uint8_t*>(context);
    auto* cpuAlloc = *reinterpret_cast<DLAllocator**>(base + 0x00);
    if (cpuAlloc) {
        void* cmdBufWrapper = *reinterpret_cast<void**>(base + 0x08);
        if (cmdBufWrapper) {
            auto* vtbl = *reinterpret_cast<void (***)(void*)>(cmdBufWrapper);
            vtbl[0](cmdBufWrapper);
            cpuAlloc->vptr->Free(cpuAlloc, cmdBufWrapper);
            *reinterpret_cast<void**>(base + 0x08) = nullptr;
        }
    }

    CGDynamicBufferRingGnm_ClearChunks(base + 0xB6F8);
    CGGfxContextGnm_ResetCommandChunks(base + 0x10);

    std::uint16_t slotId = *reinterpret_cast<std::uint16_t*>(base + 0xB724);
    g_pGXRegisteredCommandContexts[slotId] = nullptr;

    g_GXCommandContextSlotMutex.vptr->Lock(&g_GXCommandContextSlotMutex, 0xFFFFFFFFu);
    if (slotId <= 0xFFu) {
        g_GXCommandContextSlotBitmask[slotId >> 6] &= ~(1ULL << (slotId & 63u));
    }
    g_GXCommandContextSlotMutex.vptr->Unlock(&g_GXCommandContextSlotMutex);
}

// [VMA: 0x2168080 | Size: 572 B]
std::uint32_t GXGnmCommandContext_InitializeAndRegisterSlot(
    void* context,
    std::uint32_t contextType,
    void* device,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* ctxBase = static_cast<std::uint8_t*>(context);
    auto* devBase = static_cast<std::uint8_t*>(device);

    void* devRingDesc = (contextType != 0)
        ? *reinterpret_cast<void**>(devBase + 0x08)
        : *reinterpret_cast<void**>(devBase + 0x00);
    std::uint64_t initTimestamp = (contextType == 0) ? sceKernelGetProcessTimeCounter() : 0ULL;

    *reinterpret_cast<void**>(ctxBase + 0x00) = cpuAlloc;
    *reinterpret_cast<std::uint32_t*>(ctxBase + 0xB510) = contextType;

    g_GXCommandContextSlotMutex.vptr->Lock(&g_GXCommandContextSlotMutex, 0xFFFFFFFFu);
    std::uint32_t allocatedSlot = 0x100;
    for (std::uint32_t wordIdx = 0; wordIdx < 4; ++wordIdx) {
        std::uint64_t maskWord = g_GXCommandContextSlotBitmask[wordIdx];
        if (maskWord != ~0ULL) {
            std::uint32_t bitIdx = static_cast<std::uint32_t>(__builtin_ctzll(~maskWord));
            std::uint32_t candidate = wordIdx * 64u + bitIdx;
            if (candidate < 0x100u) {
                g_GXCommandContextSlotBitmask[wordIdx] = maskWord | (1ULL << bitIdx);
                allocatedSlot = candidate;
            }
            break;
        }
    }
    g_GXCommandContextSlotMutex.vptr->Unlock(&g_GXCommandContextSlotMutex);

    if (allocatedSlot >= 0x100u) {
        *reinterpret_cast<std::uint16_t*>(ctxBase + 0xB724) = 0x0100;
        GXGnmCommandContext_ShutdownAndUnregisterSlot(ctxBase);
        return 9;
    }

    g_pGXRegisteredCommandContexts[allocatedSlot] = ctxBase;
    *reinterpret_cast<std::uint16_t*>(ctxBase + 0xB724) = static_cast<std::uint16_t>(allocatedSlot);

    std::uint32_t gfxErr = CGGfxContextGnm_Initialize(
        ctxBase + 0x10,
        contextType,
        devRingDesc,
        *reinterpret_cast<void**>(devBase + 0x18),
        cpuAlloc,
        gpuAlloc,
        *reinterpret_cast<std::uint32_t*>(devBase + 0x38),
        devBase[0x121],
        allocatedSlot);
    if (gfxErr != 0) {
        return gfxErr;
    }

    auto* alloc = static_cast<DLAllocator*>(cpuAlloc);
    auto* wrapper = static_cast<void**>(alloc->vptr->AllocateAligned(alloc, 0x10, 8));
    if (!wrapper) {
        *reinterpret_cast<void**>(ctxBase + 0x08) = nullptr;
        *reinterpret_cast<std::uint16_t*>(ctxBase + 0xB724) = 0x0100;
        GXGnmCommandContext_ShutdownAndUnregisterSlot(ctxBase);
        return 9;
    }

    wrapper[1] = *reinterpret_cast<void**>(devBase + 0x18);
    wrapper[0] = (contextType != 0)
        ? &g_VTable_GXGnmCommandBufferWrapper_Deferred
        : &g_VTable_GXGnmCommandBufferWrapper_Immediate;
    *reinterpret_cast<void**>(ctxBase + 0x08) = wrapper;

    CGIndexBufferCacheGnm_Reset(ctxBase + 0xB518);
    CGRasterizerCacheGnm_Reset(ctxBase + 0xB528);

    std::uint32_t ringErr = CGDynamicBufferRingGnm_Init(ctxBase + 0xB6F8, cpuAlloc);
    if (ringErr != 0) {
        GXGnmCommandContext_ShutdownAndUnregisterSlot(ctxBase);
        return ringErr;
    }

    *reinterpret_cast<std::uint64_t*>(ctxBase + 0xB718) = initTimestamp;
    *reinterpret_cast<std::uint32_t*>(ctxBase + 0xB720) = 0;
    *reinterpret_cast<void**>(ctxBase + 0xB728) = nullptr;
    return 0;
}

// ============================================================================
// 5. Gnm Command List & Stage State Binding Thunks (0x2167be0 .. 0x2167fb0)
// ============================================================================

// [VMA: 0x2167fb0 | Size: 62 B]
void GXGnmCommandList_ReleaseMemory(void* cmdList) {
    auto* base = static_cast<std::uint8_t*>(cmdList);
    auto* alloc = *reinterpret_cast<DLAllocator**>(base + 0x10);
    if (alloc) {
        void* streamBuf = *reinterpret_cast<void**>(base + 0x00);
        if (streamBuf) {
            alloc->vptr->Free(alloc, streamBuf);
            *reinterpret_cast<void**>(base + 0x10) = nullptr;
            *reinterpret_cast<void**>(base + 0x00) = nullptr;
        }
    }
    *reinterpret_cast<void**>(base + 0x08) = nullptr;
}

// [VMA: 0x2167f90 | Size: 24 B]
void GXGnmCommandList_Construct(void* cmdList) {
    auto* base = static_cast<std::uint8_t*>(cmdList);
    *reinterpret_cast<void**>(base + 0x10) = nullptr;
    *reinterpret_cast<void**>(base + 0x08) = nullptr;
    *reinterpret_cast<void**>(base + 0x00) = nullptr;
}

// [VMA: 0x2167e70 | Size: 131 B]
std::uint32_t GXGnmCommandContext_FinishCommandList(void* context, void* stateBlock, void* outCmdList) {
    auto* cmdList = static_cast<std::uint8_t*>(CGDeviceChild_GetSelf(outCmdList));
    auto* alloc = *reinterpret_cast<DLAllocator**>(cmdList + 0x10);
    if (alloc) {
        void* streamBuf = *reinterpret_cast<void**>(cmdList + 0x00);
        if (streamBuf) {
            alloc->vptr->Free(alloc, streamBuf);
            *reinterpret_cast<void**>(cmdList + 0x10) = nullptr;
            *reinterpret_cast<void**>(cmdList + 0x00) = nullptr;
        }
    }
    *reinterpret_cast<void**>(cmdList + 0x08) = nullptr;

    std::uint32_t err = CGCommandStreamRecorderGnm_BakeContiguousStream(
        context, cmdList, cmdList + 0x08, cmdList + 0x10);
    if (err == 0) {
        std::memset(stateBlock, 0, 0x23A8);
    }
    return err;
}

// [VMA: 0x2167e60 | Size: 5 B]
void GXGnmStateBlock_ResetAllStageBindingsThunk(void* stateBlock) {
    CGCommandStreamRecorderGnm_WriteOp24_FlushCaches(stateBlock);
}

// [VMA: 0x2167e50 | Size: 10 B]
void GXGnmStateBlock_CSSetShaderThunk(void* stateBlock, void* unused, void* shader, std::uint32_t flags) {
    CGCommandStreamRecorderGnm_WriteOp23_DrawIndexedInstancedIndirect(stateBlock, shader, flags);
}

// [VMA: 0x2167e40 | Size: 10 B]
void GXGnmStateBlock_CSSetUnorderedAccessViewsThunk(void* stateBlock, void* unused, void* uav, std::uint32_t count) {
    CGCommandStreamRecorderGnm_WriteOp22_DrawInstancedIndirect(stateBlock, uav, count);
}

// [VMA: 0x2167e20 | Size: 22 B]
void GXGnmStateBlock_CSSetUnorderedAccessViewsRangeThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t arg0,
    std::uint32_t arg1,
    std::uint32_t arg2,
    std::uint32_t arg3,
    std::uint32_t arg4)
{
    CGCommandStreamRecorderGnm_WriteOp21_DrawIndexedInstanced(stateBlock, arg0, arg1, arg2, arg3, arg4);
}

// [VMA: 0x2167e10 | Size: 15 B]
void GXGnmStateBlock_CSSetConstantBuffersRangeThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t arg0,
    std::uint32_t arg1,
    std::uint32_t arg2,
    std::uint32_t arg3)
{
    CGCommandStreamRecorderGnm_WriteOp20_DrawInstanced(stateBlock, arg0, arg1, arg2, arg3);
}

// [VMA: 0x2167e00 | Size: 12 B]
void GXGnmStateBlock_CSSetShaderResourcesRangeThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t arg0,
    std::uint32_t arg1,
    std::uint32_t arg2)
{
    CGCommandStreamRecorderGnm_WriteOp1F_DrawIndexed(stateBlock, arg0, arg1, arg2);
}

// [VMA: 0x2167df0 | Size: 9 B]
void GXGnmStateBlock_IASetPrimitiveTopologyThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t topology,
    std::uint32_t flags)
{
    CGCommandStreamRecorderGnm_WriteOp1E_Draw(stateBlock, topology, flags);
}

// [VMA: 0x2167de0 | Size: 12 B]
void GXGnmStateBlock_IASetIndexBufferThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t format,
    std::uint32_t offset,
    std::uint32_t buffer)
{
    CGCommandStreamRecorderGnm_WriteOp1D_Dispatch(stateBlock, format, offset, buffer);
}

// [VMA: 0x2167dd0 | Size: 8 B]
void GXGnmStateBlock_IASetInputLayoutThunk(void* stateBlock, void* unused, void* layout) {
    CGCommandStreamRecorderGnm_WriteOp1C_SetComputeShader(stateBlock, layout);
}

// [VMA: 0x2167dc0 | Size: 12 B]
void GXGnmStateBlock_DSSetConstantBuffersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* buffers)
{
    CGCommandStreamRecorderGnm_WriteOp1B_SetCSSamplers(stateBlock, startSlot, count, buffers);
}

// [VMA: 0x2167db0 | Size: 12 B]
void GXGnmStateBlock_DSSetShaderResourcesThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* srvs)
{
    CGCommandStreamRecorderGnm_WriteOp1A_SetCSTextures(stateBlock, startSlot, count, srvs);
}

// [VMA: 0x2167da0 | Size: 12 B]
void GXGnmStateBlock_DSSetSamplersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* samplers)
{
    CGCommandStreamRecorderGnm_WriteOp19_SetCSConstantBuffers(stateBlock, startSlot, count, samplers);
}

// [VMA: 0x2167d90 | Size: 1 B]
void GXGnmStateBlock_NoOpThunk() {}

// [VMA: 0x2167d80 | Size: 8 B]
void GXGnmStateBlock_DSSetShaderThunk(void* stateBlock, void* unused, void* shader) {
    CGCommandStreamRecorderGnm_WriteOp18_SetDomainShader(stateBlock, shader);
}

// [VMA: 0x2167d70 | Size: 12 B]
void GXGnmStateBlock_PSSetConstantBuffersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* buffers)
{
    CGCommandStreamRecorderGnm_WriteOp17_SetDSSamplers(stateBlock, startSlot, count, buffers);
}

// [VMA: 0x2167d60 | Size: 12 B]
void GXGnmStateBlock_PSSetShaderResourcesThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* srvs)
{
    CGCommandStreamRecorderGnm_WriteOp16_SetDSTextures(stateBlock, startSlot, count, srvs);
}

// [VMA: 0x2167d50 | Size: 12 B]
void GXGnmStateBlock_PSSetSamplersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* samplers)
{
    CGCommandStreamRecorderGnm_WriteOp15_SetDSConstantBuffers(stateBlock, startSlot, count, samplers);
}

// [VMA: 0x2167d40 | Size: 8 B]
void GXGnmStateBlock_PSSetShaderThunk(void* stateBlock, void* unused, void* shader) {
    CGCommandStreamRecorderGnm_WriteOp14_SetHullShader(stateBlock, shader);
}

// [VMA: 0x2167d30 | Size: 12 B]
void GXGnmStateBlock_GSSetConstantBuffersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* buffers)
{
    CGCommandStreamRecorderGnm_WriteOp13_SetHSSamplers(stateBlock, startSlot, count, buffers);
}

// [VMA: 0x2167d20 | Size: 12 B]
void GXGnmStateBlock_GSSetShaderResourcesThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* srvs)
{
    CGCommandStreamRecorderGnm_WriteOp12_SetHSTextures(stateBlock, startSlot, count, srvs);
}

// [VMA: 0x2167d10 | Size: 12 B]
void GXGnmStateBlock_GSSetSamplersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* samplers)
{
    CGCommandStreamRecorderGnm_WriteOp11_SetHSConstantBuffers(stateBlock, startSlot, count, samplers);
}

// [VMA: 0x2167d00 | Size: 8 B]
void GXGnmStateBlock_GSSetShaderThunk(void* stateBlock, void* unused, void* shader) {
    CGCommandStreamRecorderGnm_WriteOp10_SetGeometryShader(stateBlock, shader);
}

// [VMA: 0x2167cf0 | Size: 12 B]
void GXGnmStateBlock_HSSetConstantBuffersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* buffers)
{
    CGCommandStreamRecorderGnm_WriteOp0F_SetGSSamplers(stateBlock, startSlot, count, buffers);
}

// [VMA: 0x2167ce0 | Size: 12 B]
void GXGnmStateBlock_HSSetShaderResourcesThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* srvs)
{
    CGCommandStreamRecorderGnm_WriteOp0E_SetGSTextures(stateBlock, startSlot, count, srvs);
}

// [VMA: 0x2167cd0 | Size: 12 B]
void GXGnmStateBlock_HSSetSamplersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* samplers)
{
    CGCommandStreamRecorderGnm_WriteOp0D_SetGSConstantBuffers(stateBlock, startSlot, count, samplers);
}

// [VMA: 0x2167cc0 | Size: 8 B]
void GXGnmStateBlock_HSSetShaderThunk(void* stateBlock, void* unused, void* shader) {
    CGCommandStreamRecorderGnm_WriteOp0C_SetPixelShader(stateBlock, shader);
}

// [VMA: 0x2167cb0 | Size: 12 B]
void GXGnmStateBlock_VSSetConstantBuffersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* buffers)
{
    CGCommandStreamRecorderGnm_WriteOp0B_SetPSSamplers(stateBlock, startSlot, count, buffers);
}

// [VMA: 0x2167ca0 | Size: 12 B]
void GXGnmStateBlock_VSSetShaderResourcesThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* srvs)
{
    CGCommandStreamRecorderGnm_WriteOp0A_SetPSTextures(stateBlock, startSlot, count, srvs);
}

// [VMA: 0x2167c90 | Size: 12 B]
void GXGnmStateBlock_VSSetSamplersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* samplers)
{
    CGCommandStreamRecorderGnm_WriteOp09_SetPSConstantBuffers(stateBlock, startSlot, count, samplers);
}

// [VMA: 0x2167c80 | Size: 8 B]
void GXGnmStateBlock_VSSetShaderThunk(void* stateBlock, void* unused, void* shader) {
    CGCommandStreamRecorderGnm_WriteOp08_SetVertexShader(stateBlock, shader);
}

// [VMA: 0x2167c70 | Size: 12 B]
void GXGnmStateBlock_IASetVertexBuffersThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* buffers)
{
    CGCommandStreamRecorderGnm_WriteOp07_SetVSSamplers(stateBlock, startSlot, count, buffers);
}

// [VMA: 0x2167c60 | Size: 12 B]
void GXGnmStateBlock_RSSetViewportsThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* viewports)
{
    CGCommandStreamRecorderGnm_WriteOp06_SetVSTextures(stateBlock, startSlot, count, viewports);
}

// [VMA: 0x2167c50 | Size: 12 B]
void GXGnmStateBlock_RSSetScissorRectsThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t startSlot,
    std::uint32_t count,
    void* rects)
{
    CGCommandStreamRecorderGnm_WriteOp05_SetVSConstantBuffers(stateBlock, startSlot, count, rects);
}

// [VMA: 0x2167c30 | Size: 23 B]
void GXGnmStateBlock_OMSetRenderTargetsAndUAVsThunk(
    void* stateBlock,
    void* unused,
    std::uint32_t numRTVs,
    std::uint32_t count,
    void* rtvs,
    void* dsv,
    void* uavs)
{
    CGCommandStreamRecorderGnm_WriteOp04_SetVertexBuffers(stateBlock, numRTVs, count, rtvs, dsv, uavs);
}

// [VMA: 0x2167c20 | Size: 13 B]
void GXGnmStateBlock_OMSetBlendStateThunk(
    void* stateBlock,
    void* unused,
    void* blendState,
    std::uint32_t blendFactor,
    std::uint32_t mask)
{
    CGCommandStreamRecorderGnm_WriteOp03_SetIndexBuffer(stateBlock, blendState, blendFactor, mask);
}

// [VMA: 0x2167c10 | Size: 7 B]
void GXGnmStateBlock_OMSetStencilRefThunk(void* stateBlock, void* unused, std::uint32_t stencilRef) {
    CGCommandStreamRecorderGnm_WriteOp02_SetPrimitiveTopology(stateBlock, stencilRef);
}

// [VMA: 0x2167c00 | Size: 8 B]
void GXGnmStateBlock_OMSetDepthStencilStateThunk(void* stateBlock, void* unused, void* depthStencilState) {
    CGCommandStreamRecorderGnm_WriteOp01_SetRasterizerState(stateBlock, depthStencilState);
}

// [VMA: 0x2167bf0 | Size: 5 B]
void GXGnmStateBlock_ClearStateThunk(void* stateBlock) {
    CGCommandStreamRecorderGnm_Finalize(stateBlock);
}

// [VMA: 0x2167be0 | Size: 11 B]
void GXGnmStateBlock_SOSetTargetsThunk(void* stateBlock, void* unused, void* targets, void* offsets) {
    CGCommandStreamRecorderGnm_Initialize(stateBlock, targets, offsets);
}

// ============================================================================
// 6. Gnm Deferred Context & Device Factory Methods (0x2167030 .. 0x2167b00)
// ============================================================================

// [VMA: 0x2167b00 | Size: 69 B]
void GXGnmShaderStageProgram_ReleaseBinaries_Variant7(void* program) {
    auto* base = static_cast<std::uint8_t*>(program);
    void* binA = *reinterpret_cast<void**>(base + 0x18);
    if (binA) {
        auto* allocA = *reinterpret_cast<DLAllocator**>(base + 0x00);
        allocA->vptr->Free(allocA, binA);
        *reinterpret_cast<void**>(base + 0x18) = nullptr;
    }
    void* binB = *reinterpret_cast<void**>(base + 0x10);
    if (binB) {
        auto* allocB = *reinterpret_cast<DLAllocator**>(base + 0x08);
        allocB->vptr->Free(allocB, binB);
        *reinterpret_cast<void**>(base + 0x10) = nullptr;
    }
}

// [VMA: 0x2167af0 | Size: 9 B]
void GXGnmShaderStageProgram_CopyDescriptor58(void* dst, const void* src) {
    std::memcpy(dst, src, 0x58);
}

// [VMA: 0x2167a20 | Size: 53 B]
void GXGnmDeferredContext_Flush(void* deferredContext) {
    auto* ctxBase = static_cast<std::uint8_t*>(deferredContext);
    void* gfxCtx = ctxBase + 0x10;
    void* syncTracker = ctxBase + 0xB478;
    if (CGSurfaceSyncTrackerGnm_IsDisabled(syncTracker)) {
        CGSurfaceSyncTrackerGnm_Enable(syncTracker);
    }
    CGGfxContextGnm_FlushAndSubmitImmediate(gfxCtx);
}

// [VMA: 0x21679e0 | Size: 60 B]
void GXGnmDeferredContext_ExecuteCommandLists(
    void* deferredContext,
    void* const* commandLists,
    std::uint32_t numCommandLists,
    bool restoreState)
{
    GfxContext_ExecuteCommandLists_Batch(deferredContext, nullptr, commandLists, numCommandLists);
    if (!restoreState) {
        std::memset(static_cast<std::uint8_t*>(deferredContext) + 0xB730, 0, 0x23A8);
    }
}

// [VMA: 0x21679a0 | Size: 55 B]
void GXGnmDeferredContext_ExecuteCommandList(
    void* deferredContext,
    void* commandList,
    bool restoreState)
{
    GfxContext_ExecuteCommandList_Single(deferredContext, nullptr, commandList);
    if (!restoreState) {
        std::memset(static_cast<std::uint8_t*>(deferredContext) + 0xB730, 0, 0x23A8);
    }
}

// [VMA: 0x2167960 | Size: 56 B]
void GXGnmDeferredContext_Destroy(void* deferredContext) {
    auto* base = static_cast<std::uint8_t*>(deferredContext);
    GXGnmCommandContext_ShutdownAndUnregisterSlot(base);
    CGDynamicBufferRingGnm_Destroy(base + 0xB6F8);
    CGOutputMergerCacheGnm_NoOpStub(base + 0xB558);
    CGGfxContextGnm_Finalize(base + 0x10);
}

// [VMA: 0x2167930 | Size: 43 B]
void GXGnmDeferredContext_CopyConstruct(void* dstContext, const void* srcContext) {
    GXGnmCommandContext_CopyConstruct(dstContext, srcContext);
    std::memset(static_cast<std::uint8_t*>(dstContext) + 0xB730, 0, 0x23A8);
}

// [VMA: 0x2167890 | Size: 4 B]
void* GXGnmDevice_GetSelf(void* device) {
    return device;
}

// [VMA: 0x2167880 | Size: 8 B]
void* GXGnmDevice_GetGpuAllocator(const void* device) {
    return *reinterpret_cast<void* const*>(static_cast<const std::uint8_t*>(device) + 0xDC20);
}

// [VMA: 0x2167870 | Size: 8 B]
void* GXGnmDevice_GetCpuAllocator(const void* device) {
    return *reinterpret_cast<void* const*>(static_cast<const std::uint8_t*>(device) + 0xDC10);
}

// [VMA: 0x21676c0 | Size: 419 B]
std::uint32_t GXGnmDevice_CreateDeferredContext(
    void* device,
    void** outDeferredContext,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC20);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || devUnready) {
        return 8;
    }

    alignas(16) std::uint8_t tempCtx[0xB730];
    *reinterpret_cast<void**>(tempCtx + 0x00) = nullptr;
    *reinterpret_cast<void**>(tempCtx + 0x08) = nullptr;
    CGGfxContextGnm_Construct(tempCtx + 0x10);
    *reinterpret_cast<std::uint32_t*>(tempCtx + 0xB510) = 0;
    CGOutputMergerCacheGnm_Construct(tempCtx + 0xB558);
    CGDynamicBufferRingGnm_ZeroInit(tempCtx + 0xB6F8);
    *reinterpret_cast<void**>(tempCtx + 0xB718) = nullptr;
    *reinterpret_cast<std::uint32_t*>(tempCtx + 0xB720) = 0;
    *reinterpret_cast<std::uint16_t*>(tempCtx + 0xB724) = 0x0100;
    *reinterpret_cast<void**>(tempCtx + 0xB728) = nullptr;
    *reinterpret_cast<void**>(tempCtx + 0xB4A0) = tempCtx;

    std::uint32_t err = GXGnmCommandContext_InitializeAndRegisterSlot(
        tempCtx, 1, devBase, resolvedCpuAlloc, resolvedGpuAlloc);
    if (err == 0) {
        void* newCtx = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0xDAE0, 0x10);
        if (newCtx) {
            CGDeferredContextGnm_Init(newCtx, tempCtx, resolvedCpuAlloc);
            *outDeferredContext = newCtx;
            err = 0;
        } else {
            *outDeferredContext = nullptr;
            GXGnmCommandContext_ShutdownAndUnregisterSlot(tempCtx);
            err = 9;
        }
    }

    CGDynamicBufferRingGnm_Destroy(tempCtx + 0xB6F8);
    CGOutputMergerCacheGnm_NoOpStub(tempCtx + 0xB558);
    CGGfxContextGnm_Finalize(tempCtx + 0x10);
    return err;
}

// [VMA: 0x21676b0 | Size: 8 B]
void* GXGnmDevice_GetImmediateContext(void* device) {
    return static_cast<std::uint8_t*>(device) + 0x130;
}

// [VMA: 0x21675d0 | Size: 218 B]
std::uint32_t GXGnmDevice_CreateQuery(
    void* device,
    void** outQuery,
    const void* queryDesc,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC18);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || devUnready) {
        return 8;
    }

    std::uint64_t tempQueryView[3] = {};
    std::uint32_t err = CGDescriptorViewGnm_CreateSamplerView(
        tempQueryView, devBase, queryDesc, resolvedCpuAlloc, resolvedGpuAlloc);
    if (err != 0) {
        return err;
    }

    void* newQuery = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0x18, 8);
    if (newQuery) {
        CGDescriptorViewGnm_CopyConstructSamplerView(newQuery, tempQueryView);
        *outQuery = newQuery;
        return 0;
    }

    *outQuery = nullptr;
    CGDescriptorViewGnm_DestroySamplerView(tempQueryView);
    return 9;
}

// [VMA: 0x21674f0 | Size: 218 B]
std::uint32_t GXGnmDevice_CreatePredicate(
    void* device,
    void** outPredicate,
    const void* predicateDesc,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC18);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || devUnready) {
        return 8;
    }

    std::uint64_t tempPredView[3] = {};
    std::uint32_t err = CGDescriptorViewGnm_CreateResourceView(
        tempPredView, devBase, predicateDesc, resolvedCpuAlloc, resolvedGpuAlloc);
    if (err != 0) {
        return err;
    }

    void* newPred = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0x18, 8);
    if (newPred) {
        CGDescriptorViewGnm_CopyConstructResourceView(newPred, tempPredView);
        *outPredicate = newPred;
        return 0;
    }

    *outPredicate = nullptr;
    CGDescriptorViewGnm_DestroyResourceView(tempPredView);
    return 9;
}

// [VMA: 0x21673e0 | Size: 272 B]
std::uint32_t GXGnmDevice_CreateExportShader(
    void* device,
    void** outExportShader,
    const void* shdrBinary,
    std::uint64_t binarySize,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC20);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || binarySize == 0 || !shdrBinary || devUnready) {
        return 8;
    }

    alignas(8) std::uint8_t tempShader[0x120] = {};
    std::uint32_t err = GXGnmExportShaderProgram_InitializeFromBinary(
        tempShader, devBase, shdrBinary, static_cast<std::uint32_t>(binarySize), resolvedCpuAlloc, resolvedGpuAlloc);
    if (err != 0) {
        return err;
    }

    void* newShader = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0x120, 8);
    if (newShader) {
        std::memcpy(newShader, tempShader, 0x120);
        *outExportShader = newShader;
        return 0;
    }

    *outExportShader = nullptr;
    auto* alloc0 = *reinterpret_cast<DLAllocator**>(tempShader + 0x110);
    alloc0->vptr->Free(alloc0, *reinterpret_cast<void**>(tempShader + 0x100));
    auto* alloc1 = *reinterpret_cast<DLAllocator**>(tempShader + 0x118);
    alloc1->vptr->Free(alloc1, *reinterpret_cast<void**>(tempShader + 0x108));
    return 9;
}

// [VMA: 0x21672d0 | Size: 272 B]
std::uint32_t GXGnmDevice_CreateHullShader(
    void* device,
    void** outHullShader,
    const void* shdrBinary,
    std::uint64_t binarySize,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC20);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || binarySize == 0 || !shdrBinary || devUnready) {
        return 8;
    }

    alignas(8) std::uint8_t tempShader[0x120] = {};
    std::uint32_t err = GXGnmHullShaderProgram_InitializeFromBinary(
        tempShader, devBase, shdrBinary, static_cast<std::uint32_t>(binarySize), resolvedCpuAlloc, resolvedGpuAlloc);
    if (err != 0) {
        return err;
    }

    void* newShader = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0x120, 8);
    if (newShader) {
        std::memcpy(newShader, tempShader, 0x120);
        *outHullShader = newShader;
        return 0;
    }

    *outHullShader = nullptr;
    auto* alloc0 = *reinterpret_cast<DLAllocator**>(tempShader + 0x110);
    alloc0->vptr->Free(alloc0, *reinterpret_cast<void**>(tempShader + 0x100));
    auto* alloc1 = *reinterpret_cast<DLAllocator**>(tempShader + 0x118);
    alloc1->vptr->Free(alloc1, *reinterpret_cast<void**>(tempShader + 0x108));
    return 9;
}

// [VMA: 0x2167140 | Size: 388 B]
std::uint32_t GXGnmDevice_CreateGeometryShaderWithStreamOutput(
    void* device,
    void** outGeometryShader,
    const void* shdrBinary,
    std::uint64_t binarySize,
    const void* streamOutDescs,
    std::uint32_t numEntries,
    const std::uint32_t* bufferStrides,
    std::uint32_t numStrides,
    std::uint32_t rasterizedStream,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC20);

    bool invalidSoEntries = (streamOutDescs == nullptr) && (numEntries != 0);
    bool invalidStreamIdx = (rasterizedStream > 3u) && (rasterizedStream != 0xFFFFFFFFu);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || numStrides > 4u || invalidSoEntries
        || binarySize == 0 || !shdrBinary || devUnready || invalidStreamIdx)
    {
        return 8;
    }

    alignas(8) std::uint8_t tempShader[0x150] = {};
    std::uint32_t err = GXGnmGeometryShaderWithStreamOut_InitializeFromBinary(
        tempShader,
        devBase,
        shdrBinary,
        binarySize,
        streamOutDescs,
        numEntries,
        bufferStrides,
        numStrides,
        rasterizedStream,
        resolvedCpuAlloc,
        resolvedGpuAlloc);
    if (err != 0) {
        return err;
    }

    void* newShader = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0x150, 8);
    if (newShader) {
        std::memcpy(newShader, tempShader, 0x150);
        *outGeometryShader = newShader;
        return 0;
    }

    *outGeometryShader = nullptr;
    auto* alloc0 = *reinterpret_cast<DLAllocator**>(tempShader + 0x110);
    alloc0->vptr->Free(alloc0, *reinterpret_cast<void**>(tempShader + 0x100));
    auto* alloc1 = *reinterpret_cast<DLAllocator**>(tempShader + 0x118);
    alloc1->vptr->Free(alloc1, *reinterpret_cast<void**>(tempShader + 0x108));
    return 9;
}

// [VMA: 0x2167030 | Size: 269 B]
std::uint32_t GXGnmDevice_CreateGeometryShader(
    void* device,
    void** outGeometryShader,
    const void* shdrBinary,
    std::uint64_t binarySize,
    void* cpuAlloc,
    void* gpuAlloc)
{
    auto* devBase = static_cast<std::uint8_t*>(device);
    bool devUnready = (*reinterpret_cast<void**>(devBase + 0x00) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x08) == nullptr)
                   || (*reinterpret_cast<void**>(devBase + 0x20) == nullptr);

    auto* resolvedCpuAlloc = static_cast<DLAllocator*>(
        cpuAlloc ? cpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC10));
    void* resolvedGpuAlloc = gpuAlloc ? gpuAlloc : *reinterpret_cast<void**>(devBase + 0xDC20);

    if (!resolvedGpuAlloc || !resolvedCpuAlloc || binarySize == 0 || !shdrBinary || devUnready) {
        return 8;
    }

    alignas(8) std::uint8_t tempShader[0x150] = {};
    std::uint32_t err = GXGnmGeometryShaderProgram_InitializeFromShdrBinary(
        tempShader, devBase, shdrBinary, static_cast<std::uint32_t>(binarySize), resolvedCpuAlloc, resolvedGpuAlloc);
    if (err != 0) {
        return err;
    }

    void* newShader = resolvedCpuAlloc->vptr->AllocateAligned(resolvedCpuAlloc, 0x150, 8);
    if (newShader) {
        std::memcpy(newShader, tempShader, 0x150);
        *outGeometryShader = newShader;
        return 0;
    }

    *outGeometryShader = nullptr;
    auto* alloc0 = *reinterpret_cast<DLAllocator**>(tempShader + 0x110);
    alloc0->vptr->Free(alloc0, *reinterpret_cast<void**>(tempShader + 0x100));
    auto* alloc1 = *reinterpret_cast<DLAllocator**>(tempShader + 0x118);
    alloc1->vptr->Free(alloc1, *reinterpret_cast<void**>(tempShader + 0x108));
    return 9;
}

} // namespace GX
