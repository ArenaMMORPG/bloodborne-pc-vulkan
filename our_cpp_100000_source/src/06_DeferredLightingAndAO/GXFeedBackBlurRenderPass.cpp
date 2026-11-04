// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 06: DeferredLightingAndAO (Tier 8/10)
// File: GXFeedBackBlurRenderPass.cpp
// Range: 0x22ab7e0 .. 0x22b0000 (16 functions, 18,758 bytes)
// Order: Strict Right-to-Left (0x22af630 down to 0x22ab7e0)
// ============================================================================
// Architectural Subsystem Overview:
//   1. GXSceneRenderPass Frame Begin & Camera/Fog/ColorBalance CBuffer Setup
//      - sub_22af630 (0x22af630): Full 4x4 camera matrix inversion (SIMD),
//        fog & color-balance parameter interpolation (sub_e9bb60 / sub_e8ee20),
//        constant buffer uploads (sub_2168a20), and GPU timestamp query update
//      - sub_22af5c0 (0x22af5c0): GPU timestamp metric query & minimum tracker
//   2. Dynamic Environment CubeMap Face Rendering (6 faces cycled via 0x56c3e90)
//      - sub_22af5a0 (0x22af5a0): CubeMap pass end callback
//      - sub_22af0e0 (0x22af0e0): CubeMap sky quad draw callback
//      - sub_22af0b0 (0x22af0b0): CubeMap pass begin callback
//      - sub_22ae500 (0x22ae500): Per-frame dynamic environment cubemap face
//        camera basis builder (+X/-X/+Y/-Y/+Z/-Z) and render task submission
//   3. Scene & Reflection Pass Submission Wrappers
//      - sub_22ae480 (0x22ae480): Reflection render pass submitter (pass 0x36)
//      - sub_22ae420 (0x22ae420): Main scene render pass submitter (pass 0x3e)
//      - sub_22ae0a0 (0x22ae0a0): Fullscreen quad target copy pass
//      - sub_22ae030 (0x22ae030): MSAA color target resolve helper
//   4. GXFeedBackBlurRenderPass ("FeedBackBlurMaskMargePS.ppo",
//      "FeedBackBlurMaskPS.ppo", "FeedBackBlurFallOffPS.ppo")
//      - sub_22ad0e0 (0x22ad0e0): Multi-stage feedback blur mask merge, mask,
//        and falloff fullscreen shader passes
//      - sub_22ad020 (0x22ad020): Post-lighting target lookup & callback
//   5. Background Clear Quad, Layer Compositor & Deferred Fog/Scattering Pass
//      - sub_22ac970 (0x22ac970): Gamma-squared RGBA8 unpack & clear quad draw
//      - sub_22ac130 (0x22ac130): Two-stage stencil-masked layer compositor
//      - sub_22ab850 (0x22ab850): Deferred fog/scattering fullscreen pass
//      - sub_22ab7e0 (0x22ab7e0): Depth-stencil & rasterizer state initializer
// ============================================================================

#include <cstdint>
#include <cstring>
#include <ctime>
#include <immintrin.h>

#include "gx_direct_vulkan.h"

extern "C" {

// External engine functions called by 0x22ab7e0 .. 0x22b0000
void sub_e46f40(void* pTargetMgr, void* pRenderTarget, void* pDepthTarget, uint32_t flags);
void sub_e47290(void* pTargetMgr, uint32_t flags);
void* sub_e47840(void* pTargetMgr);
void sub_e788f0(void* pOutParams, const void* pRowA, const void* pRowB, float blendFactor);
void sub_e8ee20(void* pOutColorBal, const void* pRowA, const void* pRowB, float blendFactor);
void* sub_e91820(void* pColorBalTable, const void* pParamId);
void sub_e9bb60(void* pOutFog, const void* pRowA, const void* pRowB, float blendFactor);
void sub_20b55b0(uint64_t level, uint32_t category, const char* fmt, ...);
void sub_2168660(void* pDrawImpl, void* pStateCtx, void* pDstSurface, void* pSrcSurface);
void sub_2168a20(
    void* pDrawImpl,
    void* pStateBlock,
    void* pConstantBuffer,
    uint32_t offset,
    uint32_t size,
    const void* pSrcData,
    uint32_t flag0,
    uint32_t flag1);
void sub_21696c0(void* pDrawImpl, void* pStateBlock, uint32_t vertexCount, uint32_t startVertex);
void sub_219fcf0(void* pDrawContext);
void sub_21a1c50(void* pDrawContext);
void sub_21aaa90(void* pOutMatrix, const void* pCameraData);
void sub_21baff0(void* pGeomContext, void* pTarget);
void* sub_21f2b90(void* pBlurTable, const void* pParamBlock);
void* sub_221eaa0(void* pTable, const void* pKey);
bool sub_221efb0(void* pTable, const void* pKey);
void* sub_2232970(void* pFogTable, const void* pParamId);
void sub_22740d0(
    void* pShaderRepo,
    void* pGeomContextSlot,
    void* pTexture,
    uint32_t arg3,
    uint32_t arg4,
    void* pArg5,
    float fogParam,
    void* stackArg0,
    uint32_t stackArg1,
    uint32_t stackArg2,
    uint32_t stackArg3);
void* sub_2274be0(void* pShaderPkgSlot, const char16_t* pShaderName);
uint8_t* sub_2289bb0(void* pCmdQueue, uint32_t cmdType, uint32_t payloadSize, uint32_t align);
void sub_2289ca0(void* pCmdQueue, const void* pPacketDesc);
void sub_228c310(void* pSceneLightEnv);
void sub_228ea50(void* pEnvPassBuilder, void* pColorTarget, void* pDepthTarget);
void sub_228ece0(void* pEnvPassBuilder);
void sub_228f010(
    void* pEnvPassBuilder,
    void* pSceneGraph,
    uint64_t faceIndex,
    uint32_t arg3,
    uint32_t arg4,
    uint32_t arg5,
    void* pTargetView,
    uint32_t stackArg1,
    uint64_t stackArg2);
void sub_22919c0(void* pFrustumPlanes);
void sub_2291c30(void* pFrustumPlanes, const void* pCameraViewData);
void sub_229d2a0(void* pSelf, uint32_t maxSlotIndex);
void sub_229e0c0(void* pArg);
void sub_229e990(
    void* pRenderPassMgr,
    void* pArg1,
    void* pCameraParams,
    void* pCameraParamsDup,
    void* pArg4,
    void* pTargetDesc,
    uint32_t passFlags,
    uint32_t passId,
    void* pExtraArg);
void sub_22b01a0(void* pPassObj, void* pDrawContext);
void sub_22b0490(void* pArg);
void sub_22d4d30(
    void* pEnvPassBuilder,
    void* pArg1,
    uint32_t passFlags,
    uint32_t passId,
    void* pCameraParams,
    uint32_t arg5,
    uint32_t width,
    uint32_t height,
    uint32_t zero0,
    uint32_t zero1);
void sub_22d5480(void* pEnvPassBuilder);
void sub_22f0530(void* pSubMgr, void* pDrawContext);
void* sub_261a1b0(void* dst, const void* src, size_t size);
void sub_26af700(void* pSubState);
void sub_26b9850(void* pDstState, void* pSrcState);
void sub_26bba10(void* pSubState);
void sub_26cd190(void* pDstState, void* pSrcState);
void sub_26d2d20(void* pSubState);
void sub_26d3070(void* pSubState, void* pDrawImpl, const void* pClearBlock);
void sub_26d3300(void* pViewportState);
void sub_26d3350(void* pViewportState, uint32_t viewportCount);
void* sub_2ba2b10();

// Forward declarations for internal functions within this module
void sub_22af630(
    void* pRenderPassMgr,
    void* pDrawContext,
    float* pCameraViewData,
    uint8_t* pViewportState,
    void* pSceneLightEnv);
void sub_22af5c0(void* pRenderPassMgr);
void sub_22af5a0(void* pTargetState);
void sub_22af0e0(void* pTaskContext);
void sub_22af0b0(void* pTargetState, void* pViewportState, uint64_t activeFlag);
void sub_22ae500(
    void* pRenderPassMgr,
    void* pArg1,
    float* pCameraParams,
    void* pArg3,
    void* pViewportState,
    uint32_t passFlags,
    void* pStackArg0);
void sub_22ae480(
    void* pRenderPassMgr,
    void* pArg1,
    void* pArg2,
    void* pArg3,
    void* pArg4,
    void* pViewportState,
    uint32_t passFlags,
    void* pStackArg1);
void sub_22ae420(
    void* pRenderPassMgr,
    void* pArg1,
    void* pArg2,
    void* pArg3,
    void* pArg4,
    void* pArg5,
    uint32_t passFlags,
    void* pStackArg1);
void sub_22ae0a0(void* pTaskContext, void* pSrcTarget, void* pDstTarget);
void sub_22ae030(void* pTaskContext, void* pViewportState);
void sub_22ad0e0(void* pTaskContext, void* pViewportState);
void sub_22ad020(void* pTaskContext);
void sub_22ac970(void* pTaskContext, void* pClearParams);
void sub_22ac130(void* pTaskContext, void* pTexSlot3, void* pTexSlot33);
void sub_22ab850(void* pTaskContext, void* pViewportState);
void sub_22ab7e0(void* pPassObj);

} // extern "C"

namespace {

using FnVtableCall0 = void (*)(void*);
using FnVtableRetU64 = uint64_t (*)(void*);
using FnVtableRetPtr = void* (*)(void*);
using FnVtableInitContainer = void (*)(void*, void*, uint32_t);
using FnVtableFreeContainer = void (*)(void*, void*);

// Atomic resource usage bitflag marker (32-bit aligned at res+0x44)
inline void gx_mark_resource_bound(void* pResource, uint64_t bitMask) noexcept {
    if (pResource != nullptr) {
        uint8_t* pBytes = static_cast<uint8_t*>(pResource);
        const uint32_t mask32 = static_cast<uint32_t>(bitMask);
        uint32_t curFlags = *reinterpret_cast<uint32_t*>(pBytes + 0x44);
        if ((curFlags & mask32) != mask32) {
            __atomic_fetch_or(
                reinterpret_cast<uint32_t*>(pBytes + 0x44),
                mask32,
                __ATOMIC_RELAXED);
        }
    }
}

// Helper to push/pop state stacks on GXDrawContext (+0x2090 RS, +0x2091 DSS, +0x2092 BS)
inline void gx_push_rasterizer_state(uint8_t* pDrawCtx) noexcept {
    uint8_t idx = pDrawCtx[0x2090];
    reinterpret_cast<void**>(pDrawCtx + 0x1ed0)[idx] =
        *reinterpret_cast<void**>(pDrawCtx + 0x258);
    pDrawCtx[0x2090] = static_cast<uint8_t>(idx + 1);
}

inline void gx_pop_rasterizer_state(uint8_t* pDrawCtx) noexcept {
    uint8_t idx = static_cast<uint8_t>(pDrawCtx[0x2090] - 1);
    pDrawCtx[0x2090] = idx;
    void* pState = reinterpret_cast<void**>(pDrawCtx + 0x1ed0)[idx];
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pState;
    uint8_t* pImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pImpl + 0xb970) = pState;
}

inline void gx_push_depth_stencil_state(uint8_t* pDrawCtx) noexcept {
    uint8_t idx = pDrawCtx[0x2091];
    uint8_t* pSlot = pDrawCtx + 0x1f10 + (static_cast<size_t>(idx) << 4);
    *reinterpret_cast<void**>(pSlot) = *reinterpret_cast<void**>(pDrawCtx + 0x210);
    pSlot[0x8] = pDrawCtx[0x218];
    pDrawCtx[0x2091] = static_cast<uint8_t>(idx + 1);
}

inline void gx_pop_depth_stencil_state(uint8_t* pDrawCtx) noexcept {
    uint8_t idx = static_cast<uint8_t>(pDrawCtx[0x2091] - 1);
    pDrawCtx[0x2091] = idx;
    uint8_t* pSlot = pDrawCtx + 0x1f10 + (static_cast<size_t>(idx) << 4);
    void* pState = *reinterpret_cast<void**>(pSlot);
    uint8_t stencilRef = pSlot[0x8];
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pState;
    pDrawCtx[0x218] = stencilRef;
    uint8_t* pImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pImpl + 0xbc08) = pState;
    pImpl[0xbcb4] = stencilRef;
}

inline void gx_push_blend_state(uint8_t* pDrawCtx) noexcept {
    uint8_t idx = pDrawCtx[0x2092];
    uint8_t* pSlot = pDrawCtx + 0x1f90 + (static_cast<size_t>(idx) << 5);
    *reinterpret_cast<void**>(pSlot) = *reinterpret_cast<void**>(pDrawCtx + 0x480);
    _mm_storeu_ps(
        reinterpret_cast<float*>(pSlot + 0x8),
        _mm_loadu_ps(reinterpret_cast<const float*>(pDrawCtx + 0x488)));
    *reinterpret_cast<uint32_t*>(pSlot + 0x18) =
        *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498);
    pDrawCtx[0x2092] = static_cast<uint8_t>(idx + 1);
}

inline void gx_pop_blend_state(uint8_t* pDrawCtx) noexcept {
    uint8_t idx = static_cast<uint8_t>(pDrawCtx[0x2092] - 1);
    pDrawCtx[0x2092] = idx;
    uint8_t* pSlot = pDrawCtx + 0x1f90 + (static_cast<size_t>(idx) << 5);
    void* pState = *reinterpret_cast<void**>(pSlot);
    uint32_t sampleMask = *reinterpret_cast<uint32_t*>(pSlot + 0x18);
    __m128 blendFactor = _mm_loadu_ps(reinterpret_cast<const float*>(pSlot + 0x8));
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pState;
    _mm_storeu_ps(reinterpret_cast<float*>(pDrawCtx + 0x488), blendFactor);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = sampleMask;
    uint8_t* pImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pImpl + 0xbc00) = pState;
    _mm_storeu_ps(reinterpret_cast<float*>(pImpl + 0xbc98), blendFactor);
    *reinterpret_cast<uint32_t*>(pImpl + 0xbcb0) = sampleMask;
}

// Helper to issue a 4-vertex fullscreen TriangleStrip quad draw on pGeomCtx (+0x440)
inline void gx_draw_fullscreen_quad(uint8_t* pGeomCtx, uint8_t* pShaderRepo) noexcept {
    void* pVertexBuffer = *reinterpret_cast<void**>(pShaderRepo + 0x9a0);
    *reinterpret_cast<void**>(pGeomCtx + 0x280) = pVertexBuffer;
    *reinterpret_cast<uint32_t*>(pGeomCtx + 0x380) = 0x18;
    *reinterpret_cast<uint32_t*>(pGeomCtx + 0x400) = 0;

    uint8_t* pImpl = *reinterpret_cast<uint8_t**>(pGeomCtx + 0x8);
    *reinterpret_cast<uint32_t*>(pImpl + 0x25f0) |= 0x1u;
    *reinterpret_cast<void**>(pImpl + 0xb750) = pVertexBuffer;
    *reinterpret_cast<uint32_t*>(pImpl + 0xb850) = 0x18;
    *reinterpret_cast<uint32_t*>(pImpl + 0xb8d0) = 0;

    *reinterpret_cast<uint32_t*>(pGeomCtx + 0x268) = 5;
    pImpl = *reinterpret_cast<uint8_t**>(pGeomCtx + 0x8);
    *reinterpret_cast<uint32_t*>(pImpl + 0xb738) = 5;

    std::memset(pGeomCtx + 0x1670, 0, 0xb8);
    *reinterpret_cast<uint64_t*>(pGeomCtx + 0x1680) = 0xe;
    *reinterpret_cast<uint64_t*>(pGeomCtx + 0x1688) = 0x8;

    sub_21696c0(pImpl, pImpl + 0xb730, 4, 0);
}

// Helper to pack and enqueue a 24-byte render command packet on pCmdQueue
inline uint8_t* gx_alloc_and_submit_cmd_node(
    void* pCmdQueue,
    uint64_t& rStreamWord0,
    uint64_t& rStreamWord1,
    uint8_t& rNeedHeader,
    uint8_t& rFlag2,
    uint8_t& rFlag3) noexcept {
    uint8_t* pNode = sub_2289bb0(pCmdQueue, 4, 0x18, 4);
    if (pNode == nullptr) {
        return nullptr;
    }
    if (rNeedHeader != 0) {
        uint64_t w0 = rStreamWord0;
        uint64_t w1 = rStreamWord1;
        uint32_t packed = (static_cast<uint32_t>(w0) << 28) |
                          (static_cast<uint32_t>(w0 >> 10) & 0x0fc00000u) |
                          ((static_cast<uint32_t>(w1) & 0x3fu) << 16) |
                          static_cast<uint32_t>(static_cast<uint16_t>(w1 >> 32));

        struct alignas(8) CmdPacketDesc {
            uint32_t packedHeader;
            uint32_t pad0;
            void* pNodePtr;
            uint8_t subFlag0;
            uint8_t subFlag1;
            uint8_t pad1[6];
        } desc{};

        desc.packedHeader = packed;
        desc.pNodePtr = pNode;
        desc.subFlag0 = (rFlag3 != 0) ? 0 : (rFlag2 != 0 ? 1 : 0);
        desc.subFlag1 = rFlag3;
        sub_2289ca0(pCmdQueue, &desc);
        rNeedHeader = 0;
    }
    uint8_t offset = pNode[1];
    return pNode + offset + 4;
}

} // namespace

extern "C" {

// ============================================================================
// 1. sub_22af630 (0x22af630) - 2921 bytes
// GXSceneRenderPass_BeginFrameAndUpdateFogAndColorCorrectionCBuffer
// ============================================================================
void sub_22af630(
    void* pRenderPassMgr,
    void* pDrawContext,
    float* pCameraViewData,
    uint8_t* pViewportState,
    void* pSceneLightEnv) {
    uint8_t* pMgr = static_cast<uint8_t*>(pRenderPassMgr);
    uint8_t* pDrawCtx = static_cast<uint8_t*>(pDrawContext);
    uint8_t* pCamBytes = reinterpret_cast<uint8_t*>(pCameraViewData);

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8[0x3a9] != 0) {
        return;
    }

    __atomic_exchange_n(reinterpret_cast<uint32_t*>(GX_VMA(0x56c3dbcULL)), 0u, __ATOMIC_SEQ_CST);

    if (pViewportState[0x53] != 0) {
        uint8_t* pDispCfg = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x660);
        uint32_t scaleMode = (pDispCfg[0x4c] != 0)
                                 ? *reinterpret_cast<uint32_t*>(pDispCfg + 0x40)
                                 : 1u;
        uint32_t dispParam1 = *reinterpret_cast<uint32_t*>(pDispCfg + 0x44);
        uint32_t borderPixels = *reinterpret_cast<uint32_t*>(pDispCfg + 0x48);

        *reinterpret_cast<uint32_t*>(pCamBytes + 0x70) = scaleMode;
        *reinterpret_cast<uint32_t*>(pCamBytes + 0x74) = dispParam1;
        *reinterpret_cast<uint32_t*>(pCamBytes + 0x78) = borderPixels;

        uint32_t border2x = borderPixels * 2u;
        uint32_t effWidth = *reinterpret_cast<uint32_t*>(pViewportState + 0x180) - border2x;
        uint32_t effHeight = *reinterpret_cast<uint32_t*>(pViewportState + 0x184) - border2x;
        *reinterpret_cast<uint32_t*>(pCamBytes + 0x7c) = effWidth;
        *reinterpret_cast<uint32_t*>(pCamBytes + 0x80) = effHeight;

        if (pViewportState[0x297e] != 0 && scaleMode >= 2u) {
            *reinterpret_cast<uint32_t*>(pViewportState + 0x29f0) = scaleMode;
            *reinterpret_cast<uint32_t*>(pViewportState + 0x29f4) = dispParam1;
            *reinterpret_cast<uint32_t*>(pViewportState + 0x29f8) = borderPixels;
            *reinterpret_cast<uint32_t*>(pViewportState + 0x29fc) = effWidth;
            *reinterpret_cast<uint32_t*>(pViewportState + 0x2a00) = effHeight;
        }
    }

    *reinterpret_cast<uint32_t*>(pViewportState + 0x2a18) += 1u;
    pViewportState[0x2a1d] = pViewportState[0x2a1c];
    pViewportState[0x2a1c] = 0;
    pViewportState[0x2a1e] = 0;
    _mm_store_ps(
        reinterpret_cast<float*>(pViewportState + 0x2a30),
        _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f));
    *reinterpret_cast<uint32_t*>(pViewportState + 0x2a40) = 0x7f7fffffu;

    alignas(16) uint8_t localViewTmp[0x40];
    sub_21aaa90(localViewTmp, pCameraViewData);

    uint8_t* pSubMgr6070 = *reinterpret_cast<uint8_t**>(pMgr + 0x6070);
    void* pCameraCBuffer = *reinterpret_cast<void**>(pSubMgr6070 + 0x230);

    // Load 4x4 camera matrix and compute its exact SIMD inverse
    alignas(16) __m128 camMatrices[8];
    __m128 row0 = _mm_load_ps(pCameraViewData + 0);
    __m128 row1 = _mm_load_ps(pCameraViewData + 4);
    __m128 row2 = _mm_load_ps(pCameraViewData + 8);
    __m128 row3 = _mm_load_ps(pCameraViewData + 12);
    camMatrices[0] = row0;
    camMatrices[1] = row1;
    camMatrices[2] = row2;
    camMatrices[3] = row3;

    __m128 tmp0 = _mm_movelh_ps(row0, row1);
    __m128 tmp1 = _mm_movehl_ps(row1, row0);
    __m128 tmp2 = _mm_movelh_ps(row2, row3);
    __m128 tmp3 = _mm_movehl_ps(row3, row2);

    __m128 col0 = _mm_shuffle_ps(tmp0, tmp2, 0x88);
    __m128 col1 = _mm_shuffle_ps(tmp0, tmp2, 0xdd);
    __m128 col2 = _mm_shuffle_ps(tmp1, tmp3, 0x88);
    __m128 col3 = _mm_shuffle_ps(tmp1, tmp3, 0xdd);

    __m128 pairs0 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(col2, col2, 0x50), _mm_movehl_ps(col3, col3)),
        _mm_mul_ps(_mm_movehl_ps(col2, col2), _mm_shuffle_ps(col3, col3, 0x50)));
    __m128 pairs1 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0x50), _mm_movehl_ps(col1, col1)),
        _mm_mul_ps(_mm_movehl_ps(col0, col0), _mm_shuffle_ps(col1, col1, 0x50)));
    __m128 pairs2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(col2, col0, 0x88), _mm_shuffle_ps(col3, col1, 0xdd)),
        _mm_mul_ps(_mm_shuffle_ps(col2, col0, 0xdd), _mm_shuffle_ps(col3, col1, 0x88)));

    __m128 shufA = _mm_shuffle_ps(pairs0, pairs2, 0x5d);
    __m128 shufB = _mm_shuffle_ps(pairs1, pairs2, 0xfd);

    __m128 prod0 = _mm_mul_ps(_mm_shuffle_ps(col1, col1, 0x49), _mm_shuffle_ps(shufA, pairs0, 0x32));
    __m128 prod1 = _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0x12), _mm_shuffle_ps(shufA, pairs0, 0x99));
    __m128 prod2 = _mm_mul_ps(_mm_shuffle_ps(col3, col3, 0x49), _mm_shuffle_ps(shufB, pairs1, 0x32));
    __m128 prod3 = _mm_mul_ps(_mm_shuffle_ps(col2, col2, 0x12), _mm_shuffle_ps(shufB, pairs1, 0x99));

    __m128 shufC = _mm_shuffle_ps(pairs0, pairs2, 0x04);
    __m128 shufD = _mm_shuffle_ps(pairs1, pairs2, 0xa4);

    __m128 sub0 = _mm_sub_ps(
        prod0,
        _mm_mul_ps(_mm_shuffle_ps(col1, col1, 0x9e), _mm_shuffle_ps(pairs0, shufC, 0x93)));
    __m128 sub1 = _mm_sub_ps(
        prod1,
        _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0x7b), _mm_shuffle_ps(pairs0, shufC, 0x26)));
    __m128 sub2 = _mm_sub_ps(
        prod2,
        _mm_mul_ps(_mm_shuffle_ps(col3, col3, 0x9e), _mm_shuffle_ps(pairs1, shufD, 0x93)));
    __m128 sub3 = _mm_sub_ps(
        prod3,
        _mm_mul_ps(_mm_shuffle_ps(col2, col2, 0x7b), _mm_shuffle_ps(pairs1, shufD, 0x26)));

    __m128 alignA = _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(pairs2), _mm_castps_si128(pairs0), 8));
    __m128 shufE = _mm_shuffle_ps(pairs0, pairs2, 0x4c);
    __m128 hlB = _mm_movehl_ps(pairs2, pairs1);
    __m128 shufF = _mm_shuffle_ps(pairs1, pairs2, 0xec);

    __m128 term0 = _mm_mul_ps(_mm_shuffle_ps(col1, col1, 0x33), _mm_shuffle_ps(alignA, alignA, 0x2c));
    __m128 term1 = _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0x8d), _mm_shuffle_ps(shufE, shufE, 0x93));
    __m128 term2 = _mm_mul_ps(_mm_shuffle_ps(col3, col3, 0x33), _mm_shuffle_ps(hlB, hlB, 0x2c));
    __m128 term3 = _mm_mul_ps(_mm_shuffle_ps(col2, col2, 0x8d), _mm_shuffle_ps(shufF, shufF, 0x93));

    __m128 adj0 = _mm_shuffle_ps(_mm_add_ps(term0, sub0), _mm_sub_ps(sub0, term0), 0xd8);
    __m128 adj1 = _mm_shuffle_ps(_mm_sub_ps(sub1, term1), _mm_add_ps(term1, sub1), 0xd8);
    __m128 adj2 = _mm_shuffle_ps(_mm_add_ps(term2, sub2), _mm_sub_ps(sub2, term2), 0xd8);
    __m128 adj3 = _mm_shuffle_ps(_mm_sub_ps(sub3, term3), _mm_add_ps(term3, sub3), 0xd8);

    adj0 = _mm_shuffle_ps(adj0, adj0, 0xd8);
    adj1 = _mm_shuffle_ps(adj1, adj1, 0xd8);
    adj2 = _mm_shuffle_ps(adj2, adj2, 0xd8);
    adj3 = _mm_shuffle_ps(adj3, adj3, 0xd8);

    __m128 det = _mm_mul_ps(col0, adj0);
    det = _mm_hadd_ps(det, det);
    det = _mm_hadd_ps(det, det);
    __m128 invDet = _mm_div_ps(_mm_set1_ps(1.0f), det);

    adj0 = _mm_mul_ps(invDet, adj0);
    adj1 = _mm_mul_ps(invDet, adj1);
    adj2 = _mm_mul_ps(invDet, adj2);
    adj3 = _mm_mul_ps(invDet, adj3);

    camMatrices[4] = adj0;
    camMatrices[5] = adj1;
    camMatrices[6] = adj2;
    camMatrices[7] = adj3;

    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pCameraCBuffer, 0, 0, camMatrices, 0, 0);

    *reinterpret_cast<void**>(pDrawCtx + 0x6d8) = pCameraCBuffer;
    uint32_t vsMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0) = (vsMin < 11u) ? vsMin : 11u;
    uint32_t vsMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4) = (vsMax > 12u) ? vsMax : 12u;

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x78) |= 0x8000000ULL;
    gx_mark_resource_bound(pCameraCBuffer, 0x10800ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xc1a0) = pCameraCBuffer;

    // Interpolate fog parameters (sub_2232970 + sub_e9bb60)
    float fogBlendT = *reinterpret_cast<float*>(pViewportState + 0x2a70);
    uint64_t fogId0 = *reinterpret_cast<uint64_t*>(pViewportState + 0x2a60);
    uint64_t fogId1 = *reinterpret_cast<uint64_t*>(pViewportState + 0x2a68);

    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    void* pFogTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x5958);
    uint8_t* pFogRow0 = static_cast<uint8_t*>(sub_2232970(pFogTable, &fogId0));
    pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    pFogTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x5958);
    uint8_t* pFogRow1 = static_cast<uint8_t*>(sub_2232970(pFogTable, &fogId1));

    void* pHeapAllocator = *reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    if (pHeapAllocator == nullptr) {
        pHeapAllocator = sub_2ba2b10();
        *reinterpret_cast<void**>(GX_VMA(0x5540670ULL)) = pHeapAllocator;
    }

    uint8_t* pContainerCheck = reinterpret_cast<uint8_t*>(camMatrices);
    void** pHeapVtable = *reinterpret_cast<void***>(pHeapAllocator);
    reinterpret_cast<FnVtableInitContainer>(pHeapVtable[4])(pContainerCheck, pHeapAllocator, 0);
    if ((pContainerCheck[0] & 0x20u) == 0) {
        sub_20b55b0(0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    struct alignas(16) FogEvalContext {
        uint8_t header[0x30];
        union {
            void* pStrData;
            uint16_t inlineWstr[4];
        };
        uint64_t pad0;
        uint64_t strLen;
        uint64_t strCap;
        void* pAllocator;
        uint8_t flag;
        uint8_t pad1[0x27];
        __m128 vec90;
        float outFloat80;
        uint8_t pad2[0xc];
        float f170;
        float f16c;
        uint8_t pad4[0x8];
        __m128 vec160;
        float f150;
        float f14c;
        float f148;
        float f144;
        uint8_t pad5[0x10];
        float f130;
        float f12c;
        uint8_t pad6[0x8];
    } fogCtx{};

    fogCtx.pAllocator = pHeapAllocator;
    fogCtx.strCap = 7;
    fogCtx.strLen = 0;
    fogCtx.inlineWstr[0] = 0;
    fogCtx.flag = 1;

    sub_e9bb60(&fogCtx, pFogRow0 + 0x10, pFogRow1 + 0x10, fogBlendT);

    *reinterpret_cast<float*>(pMgr + 0x7908) = fogCtx.outFloat80;

    alignas(16) float lightEnvCBufferData[44] = {};
    _mm_store_ps(&lightEnvCBufferData[16], fogCtx.vec90);

    float f170 = fogCtx.f170;
    float f16c = fogCtx.f16c;
    lightEnvCBufferData[20] = f170;
    lightEnvCBufferData[21] = f16c;
    lightEnvCBufferData[22] = (1.0f - f170) * 0.5f;
    lightEnvCBufferData[23] = (1.0f - f16c) * 0.5f;

    _mm_store_ps(&lightEnvCBufferData[24], fogCtx.vec160);
    lightEnvCBufferData[28] = fogCtx.f150;
    float f14c = fogCtx.f14c;
    lightEnvCBufferData[29] = f14c;
    lightEnvCBufferData[30] = 1.0f / (fogCtx.f148 - f14c);
    lightEnvCBufferData[31] = fogCtx.f144;

    float f130 = fogCtx.f130;
    float f12c = fogCtx.f12c;
    lightEnvCBufferData[32] = f130;
    lightEnvCBufferData[33] = f12c;
    lightEnvCBufferData[34] = (1.0f - f130) * 0.5f;
    lightEnvCBufferData[35] = (1.0f - f12c) * 0.5f;
    lightEnvCBufferData[36] = *reinterpret_cast<float*>(pMgr + 0x78cc);

    // Interpolate color balance parameters (sub_e91820 + sub_e8ee20)
    float cbBlendT = *reinterpret_cast<float*>(pViewportState + 0x2a70);
    uint64_t cbId0 = *reinterpret_cast<uint64_t*>(pViewportState + 0x2bb0);
    uint64_t cbId1 = *reinterpret_cast<uint64_t*>(pViewportState + 0x2bb8);

    pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    void* pCbTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x59e0);
    uint8_t* pCbRow0 = static_cast<uint8_t*>(sub_e91820(pCbTable, &cbId0));
    pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    pCbTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x59e0);
    uint8_t* pCbRow1 = static_cast<uint8_t*>(sub_e91820(pCbTable, &cbId1));

    alignas(16) float cbOut[4] = {0.5f, 0.0f, 1.25f, 1.0f};
    sub_e8ee20(cbOut, pCbRow0 + 0x8, pCbRow1 + 0x8, cbBlendT);

    lightEnvCBufferData[40] = cbOut[0];
    lightEnvCBufferData[41] = cbOut[1];
    lightEnvCBufferData[42] = cbOut[2];
    lightEnvCBufferData[43] = cbOut[3];

    _mm_store_ps(&lightEnvCBufferData[0], _mm_set1_ps(1.0f));
    _mm_store_ps(&lightEnvCBufferData[8], _mm_setzero_ps());
    _mm_store_ps(&lightEnvCBufferData[12], _mm_set1_ps(1.0f));

    pSubMgr6070 = *reinterpret_cast<uint8_t**>(pMgr + 0x6070);
    void* pEnvCBuffer = *reinterpret_cast<void**>(pSubMgr6070 + 0x290);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pEnvCBuffer, 0, 0, lightEnvCBufferData, 0, 0);

    // Update GPU timestamp query if frame counter advanced
    sub_22af5c0(pMgr);

    sub_22f0530(*reinterpret_cast<void**>(pMgr + 0x6070), pDrawCtx);

    uint8_t envFlag = (pMgr[0x7980] == 0 && *reinterpret_cast<uint32_t*>(pMgr + 0x7990) == 0) ? 1 : 0;
    *reinterpret_cast<uint8_t*>(GX_VMA(0x513ad1bULL)) = envFlag;

    *reinterpret_cast<uint64_t*>(pMgr + 0x58f8) = 0;
    *reinterpret_cast<uint64_t*>(pMgr + 0x58f0) = 0;
    *reinterpret_cast<uint64_t*>(pMgr + 0x58e8) = 0;
    *reinterpret_cast<uint64_t*>(pMgr + 0x58e0) = 0;

    uint8_t* pLightQueue = *reinterpret_cast<uint8_t**>(pMgr + 0x5878);
    *reinterpret_cast<uint64_t*>(pLightQueue + 0x78) = 0;
    *reinterpret_cast<uint64_t*>(pLightQueue + 0x70) = 0;
    *reinterpret_cast<uint64_t*>(pLightQueue + 0x108) = 0;
    *reinterpret_cast<uint64_t*>(pLightQueue + 0x100) = 0;
    *reinterpret_cast<uint64_t*>(pLightQueue + 0x170) =
        *reinterpret_cast<uint64_t*>(pLightQueue + 0x168);

    pLightQueue = *reinterpret_cast<uint8_t**>(pMgr + 0x5878);
    pLightQueue[0x190] = pMgr[0x78b3];

    sub_228c310(pSceneLightEnv);

    void* pColorTarget = *reinterpret_cast<void**>(pViewportState + 0x160);
    void* pDepthTarget = *reinterpret_cast<void**>(pViewportState + 0x170);
    void* pTargetMgr = pDrawCtx + 0x1728;
    sub_e46f40(pTargetMgr, pColorTarget, pDepthTarget, 1);
    *reinterpret_cast<void**>(pDrawCtx + 0x1ec0) = sub_e47840(pTargetMgr);
    sub_21a1c50(pDrawCtx);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pDefaultDSS = *reinterpret_cast<void**>(pDefaultStates + 0x1d0);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pDefaultDSS;
    pDrawCtx[0x218] = 0xff;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pDefaultDSS;
    pDrawImpl[0xbcb4] = 0xff;

    void* pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x208);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    sub_22b01a0(pSceneLightEnv, pDrawCtx);

    // Re-bind VS CBuffer slot 11 (+0x230) and PS CBuffer slot 9 (+0x290)
    pSubMgr6070 = *reinterpret_cast<uint8_t**>(pMgr + 0x6070);
    void* pVsCbuf11 = *reinterpret_cast<void**>(pSubMgr6070 + 0x230);
    *reinterpret_cast<void**>(pDrawCtx + 0x6d8) = pVsCbuf11;
    vsMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0) = (vsMin < 11u) ? vsMin : 11u;
    vsMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4) = (vsMax > 12u) ? vsMax : 12u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x78) |= 0x8000000ULL;
    gx_mark_resource_bound(pVsCbuf11, 0x10800ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xc1a0) = pVsCbuf11;

    pSubMgr6070 = *reinterpret_cast<uint8_t**>(pMgr + 0x6070);
    void* pPsCbuf9 = *reinterpret_cast<void**>(pSubMgr6070 + 0x290);
    *reinterpret_cast<void**>(pDrawCtx + 0x9b0) = pPsCbuf9;
    uint32_t psMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) = (psMin < 9u) ? psMin : 9u;
    uint32_t psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 10u) ? psMax : 10u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x2000000ULL;
    gx_mark_resource_bound(pPsCbuf9, 0x100200ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd570) = pPsCbuf9;

    if (fogCtx.strCap >= 8 && fogCtx.pAllocator != nullptr) {
        void** pAllocVt = *reinterpret_cast<void***>(fogCtx.pAllocator);
        reinterpret_cast<FnVtableFreeContainer>(pAllocVt[14])(fogCtx.pAllocator, fogCtx.pStrData);
    }
}

// ============================================================================
// 2. sub_22af5c0 (0x22af5c0) - 103 bytes
// GXSceneRenderPass_UpdateGpuTimestampMetrics
// ============================================================================
void sub_22af5c0(void* pRenderPassMgr) {
    uint8_t* pMgr = static_cast<uint8_t*>(pRenderPassMgr);
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint64_t curFrame = *reinterpret_cast<uint64_t*>(pGlobal6c8 + 0x600);

    if (*reinterpret_cast<uint64_t*>(pMgr + 0x50) != curFrame) {
        uint8_t* pSub20 = *reinterpret_cast<uint8_t**>(pMgr + 0x20);
        void* pQueryObj = *reinterpret_cast<void**>(pSub20 + 0x58);
        void** pVtable = *reinterpret_cast<void***>(pQueryObj);
        uint64_t elapsed = reinterpret_cast<FnVtableRetU64>(pVtable[6])(pQueryObj);
        *reinterpret_cast<uint64_t*>(GX_VMA(0x56c3e78ULL)) = elapsed;
        if (elapsed < *reinterpret_cast<uint64_t*>(GX_VMA(0x56c3e80ULL))) {
            *reinterpret_cast<uint64_t*>(GX_VMA(0x56c3e80ULL)) = elapsed;
        }

        pSub20 = *reinterpret_cast<uint8_t**>(pMgr + 0x20);
        pQueryObj = *reinterpret_cast<void**>(pSub20 + 0x58);
        pVtable = *reinterpret_cast<void***>(pQueryObj);
        reinterpret_cast<FnVtableCall0>(pVtable[15])(pQueryObj);

        pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        *reinterpret_cast<uint64_t*>(pMgr + 0x50) =
            *reinterpret_cast<uint64_t*>(pGlobal6c8 + 0x600);
    }
}

// ============================================================================
// 3. sub_22af5a0 (0x22af5a0) - 29 bytes
// GXEnvCubeMapPass_EndCallback
// ============================================================================
void sub_22af5a0(void* pTargetState) {
    static_cast<uint8_t*>(pTargetState)[0x658] = 1;
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    *reinterpret_cast<uint64_t*>(pGlobalDd8 + 0x7710) = 0;
}

// ============================================================================
// 4. sub_22af0e0 (0x22af0e0) - 1204 bytes
// GXEnvCubeMapPass_DrawSkyQuadCallback
// ============================================================================
void sub_22af0e0(void* pTaskContext) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pTask + 0x430);

    gx_push_depth_stencil_state(pDrawCtx);
    gx_push_blend_state(pDrawCtx);
    gx_push_rasterizer_state(pDrawCtx);

    void* pSkyBlendState = *reinterpret_cast<void**>(pGlobalDd8 + 0x6610);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pSkyBlendState;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pSkyBlendState;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    void* pSkyRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pSkyRS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pSkyRS;

    void* pSkyDSS = *reinterpret_cast<void**>(pDefaultStates + 0x1c8);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pSkyDSS;
    pDrawCtx[0x218] = 0xff;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pSkyDSS;
    pDrawImpl[0xbcb4] = 0xff;

    uint8_t* pShaderRepo = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6078);
    uint8_t* pVsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0xa28);
    void* pInputLayout = *reinterpret_cast<void**>(pShaderRepo + 0x9a8);
    uint8_t* pPsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0xa18);

    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    void* pVertexShader = *reinterpret_cast<void**>(pVsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0x780) = pVertexShader;
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = pVertexShader;

    void* pPixelShader = *reinterpret_cast<void**>(pPsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pPixelShader;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pPixelShader;

    *reinterpret_cast<void**>(pDrawCtx + 0x1320) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xc6b0) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x1038) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xcba8) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0xd50) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd0a0) = nullptr;

    void* pSkyCBuffer = *reinterpret_cast<void**>(pShaderRepo + 0x968);
    alignas(16) const float kUnitVec4[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pSkyCBuffer, 0, 0, kUnitVec4, 0, 0);

    *reinterpret_cast<void**>(pDrawCtx + 0x978) = pSkyCBuffer;
    uint32_t psMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) = (psMin < 2u) ? psMin : 2u;
    uint32_t psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 3u) ? psMax : 3u;

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x40000ULL;
    gx_mark_resource_bound(pSkyCBuffer, 0x100004ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd538) = pSkyCBuffer;

    uint8_t* pGeomCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    gx_draw_fullscreen_quad(pGeomCtx, pShaderRepo);

    gx_pop_rasterizer_state(pDrawCtx);
    gx_pop_blend_state(pDrawCtx);
    gx_pop_depth_stencil_state(pDrawCtx);
}

// ============================================================================
// 5. sub_22af0b0 (0x22af0b0) - 35 bytes
// GXEnvCubeMapPass_BeginCallback
// ============================================================================
void sub_22af0b0(void* pTargetState, void* pViewportState, uint64_t activeFlag) {
    static_cast<uint8_t*>(pTargetState)[0x658] = (activeFlag != 0) ? 1 : 0;
    void* pEnvTex = *reinterpret_cast<void**>(static_cast<uint8_t*>(pViewportState) + 0xf48);
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    *reinterpret_cast<void**>(pGlobalDd8 + 0x7710) = pEnvTex;
}

// ============================================================================
// 6. sub_22ae500 (0x22ae500) - 2988 bytes
// GXSceneRenderPass_RenderDynamicEnvCubeMapFace
// ============================================================================
void sub_22ae500(
    void* pRenderPassMgr,
    void* pArg1,
    float* pCameraParams,
    void* pArg3,
    void* pViewportState,
    uint32_t passFlags,
    void* pStackArg0) {
    uint8_t* pMgr = static_cast<uint8_t*>(pRenderPassMgr);
    uint8_t* pVpState = static_cast<uint8_t*>(pViewportState);

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8[0x3a9] != 0) {
        return;
    }

    if (pMgr[0x7980] == 0 && *reinterpret_cast<uint32_t*>(pMgr + 0x7990) == 0) {
        *reinterpret_cast<int32_t*>(GX_VMA(0x56c3e90ULL)) = 0;
        return;
    }
    if (*reinterpret_cast<uint32_t*>(pMgr + 0x7994) == 2u) {
        return;
    }

    // Throttle dynamic 6-face environment cubemap updates to 30 Hz (1 face per 33.3 ms, matching PS4 30 FPS)
    // after the initial 6 faces have been rendered at least once.
    {
        static uint32_t s_totalFacesRendered = 0;
        static uint64_t s_lastFaceTimeNs = 0;
        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t nowNs = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
        if (s_totalFacesRendered >= 6u && (nowNs - s_lastFaceTimeNs) < 33000000ULL) {
            return;
        }
        s_lastFaceTimeNs = nowNs;
        if (s_totalFacesRendered < 6u) {
            ++s_totalFacesRendered;
        }
    }

    alignas(16) float localCamParams[36] = {};
    _mm_store_ps(&localCamParams[0], _mm_load_ps(pCameraParams + 0));
    _mm_store_ps(&localCamParams[4], _mm_load_ps(pCameraParams + 4));
    _mm_store_ps(&localCamParams[8], _mm_load_ps(pCameraParams + 8));
    _mm_store_ps(&localCamParams[12], _mm_load_ps(pCameraParams + 12));
    _mm_store_ps(&localCamParams[16], _mm_load_ps(pCameraParams + 16));
    _mm_store_ps(&localCamParams[20], _mm_load_ps(pCameraParams + 20));
    _mm_store_ps(&localCamParams[24], _mm_load_ps(pCameraParams + 24));
    _mm_store_ps(&localCamParams[28], _mm_load_ps(pCameraParams + 28));
    localCamParams[32] = pCameraParams[32];

    *reinterpret_cast<uint32_t*>(pMgr + 0x2cbc) = *reinterpret_cast<uint32_t*>(pVpState + 0x5c);
    *reinterpret_cast<void**>(pMgr + 0x47f8) = pVpState + 0x1320;

    float f2b90 = *reinterpret_cast<float*>(pVpState + 0x2b90);
    *reinterpret_cast<uint32_t*>(pMgr + 0x57e4) = *reinterpret_cast<uint32_t*>(pVpState + 0x2b84);
    *reinterpret_cast<uint32_t*>(pMgr + 0x57e0) = *reinterpret_cast<uint32_t*>(pVpState + 0x2b80);
    *reinterpret_cast<uint32_t*>(pMgr + 0x57ec) = *reinterpret_cast<uint32_t*>(pVpState + 0x2b8c);
    *reinterpret_cast<uint32_t*>(pMgr + 0x57e8) = *reinterpret_cast<uint32_t*>(pVpState + 0x2b88);
    *reinterpret_cast<float*>(pMgr + 0x57f0) = f2b90;

    float f2bc0 = *reinterpret_cast<float*>(pVpState + 0x2bc0);
    *reinterpret_cast<uint32_t*>(pMgr + 0x5814) = *reinterpret_cast<uint32_t*>(pVpState + 0x2bb4);
    *reinterpret_cast<uint32_t*>(pMgr + 0x5810) = *reinterpret_cast<uint32_t*>(pVpState + 0x2bb0);
    *reinterpret_cast<uint32_t*>(pMgr + 0x581c) = *reinterpret_cast<uint32_t*>(pVpState + 0x2bbc);
    *reinterpret_cast<uint32_t*>(pMgr + 0x5818) = *reinterpret_cast<uint32_t*>(pVpState + 0x2bb8);
    *reinterpret_cast<float*>(pMgr + 0x5820) = f2bc0;

    float f2a70 = *reinterpret_cast<float*>(pVpState + 0x2a70);
    *reinterpret_cast<uint32_t*>(pMgr + 0x56c4) = *reinterpret_cast<uint32_t*>(pVpState + 0x2a64);
    *reinterpret_cast<uint32_t*>(pMgr + 0x56c0) = *reinterpret_cast<uint32_t*>(pVpState + 0x2a60);
    *reinterpret_cast<uint32_t*>(pMgr + 0x56cc) = *reinterpret_cast<uint32_t*>(pVpState + 0x2a6c);
    *reinterpret_cast<uint32_t*>(pMgr + 0x56c8) = *reinterpret_cast<uint32_t*>(pVpState + 0x2a68);
    *reinterpret_cast<float*>(pMgr + 0x56d0) = f2a70;

    int32_t faceIdx = *reinterpret_cast<int32_t*>(GX_VMA(0x56c3e90ULL));
    if (faceIdx >= 6) {
        *reinterpret_cast<int32_t*>(GX_VMA(0x56c3e90ULL)) = 0;
        faceIdx = 0;
    }
    *reinterpret_cast<int32_t*>(GX_VMA(0x56c3e90ULL)) = faceIdx + 1;

    uint8_t* pCubeTargetState = pMgr + 0x2c60;
    __m128 camPos = _mm_load_ps(pCameraParams + 12);
    localCamParams[20] = 1.57079637f; // 0x3fc90fdb (pi / 2 = 90 deg FOV)
    localCamParams[25] = 0.0f;

    // Select cubemap face up (xmm1) and forward (xmm2) vectors
    __m128 upVec = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    __m128 fwdVec;
    switch (faceIdx) {
        case 1:
            fwdVec = _mm_setr_ps(-1.0f, 0.0f, 0.0f, 0.0f);
            break;
        case 2:
            upVec = _mm_setr_ps(0.0f, 0.0f, -1.0f, 0.0f);
            fwdVec = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
            break;
        case 3:
            upVec = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            fwdVec = _mm_setr_ps(0.0f, -1.0f, 0.0f, 0.0f);
            break;
        case 4:
            fwdVec = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            break;
        case 5:
            fwdVec = _mm_setr_ps(0.0f, 0.0f, -1.0f, 0.0f);
            break;
        default:
            fwdVec = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
            break;
    }

    // 3D cross product: rightVec = cross(upVec, fwdVec)
    __m128 upYZX = _mm_shuffle_ps(upVec, upVec, 0xc9);
    __m128 fwdYZX = _mm_shuffle_ps(fwdVec, fwdVec, 0xc9);
    __m128 crossTmp = _mm_sub_ps(_mm_mul_ps(upVec, fwdYZX), _mm_mul_ps(fwdVec, upYZX));
    __m128 rightVec = _mm_shuffle_ps(crossTmp, crossTmp, 0x49);

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    rightVec = _mm_and_ps(rightVec, maskXYZ);
    camPos = _mm_add_ps(_mm_and_ps(camPos, maskXYZ), _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

    _mm_store_ps(&localCamParams[0], rightVec);
    _mm_store_ps(&localCamParams[4], upVec);
    _mm_store_ps(&localCamParams[8], fwdVec);
    _mm_store_ps(&localCamParams[12], camPos);

    uint32_t passId = static_cast<uint32_t>(faceIdx + 0x37);
    uint32_t cubeWidth = *reinterpret_cast<uint32_t*>(pMgr + 0x2c78);
    uint32_t cubeHeight = *reinterpret_cast<uint32_t*>(pMgr + 0x2c7c);

    struct alignas(16) EnvPassBuilder {
        uint8_t data0[0x110];
        uint64_t streamWord0;
        uint64_t streamWord1;
        uint8_t data1[0x2d8];
        uint8_t flag0;
        uint8_t needHeader;
        uint8_t flag2;
        uint8_t flag3;
        uint8_t data2[0x4c];
        void* pCmdQueue;
        uint8_t data3[0x400];
    } envBuilder{};

    sub_22d4d30(&envBuilder, pArg1, passFlags, passId, localCamParams, 0, cubeWidth, cubeHeight, 0, 0);

    envBuilder.flag0 = 1;
    envBuilder.needHeader = 1;
    envBuilder.flag3 = 1;
    envBuilder.streamWord1 = 0;

    // Enqueue task 1: sub_229e0c0(pCubeTargetState)
    uint8_t* pPayload = gx_alloc_and_submit_cmd_node(
        envBuilder.pCmdQueue,
        envBuilder.streamWord0,
        envBuilder.streamWord1,
        envBuilder.needHeader,
        envBuilder.flag2,
        envBuilder.flag3);
    if (pPayload != nullptr) {
        *reinterpret_cast<uint64_t*>(pPayload + 0x0) = GX_VMA(0x229e0c0ULL);
        *reinterpret_cast<void**>(pPayload + 0x8) = pCubeTargetState;
        *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
    }

    // Enqueue task 2: sub_229d2a0(u2cd0)
    uint64_t param2cd0 = *reinterpret_cast<uint32_t*>(pMgr + 0x2cd0);
    pPayload = gx_alloc_and_submit_cmd_node(
        envBuilder.pCmdQueue,
        envBuilder.streamWord0,
        envBuilder.streamWord1,
        envBuilder.needHeader,
        envBuilder.flag2,
        envBuilder.flag3);
    if (pPayload != nullptr) {
        *reinterpret_cast<uint64_t*>(pPayload + 0x0) = GX_VMA(0x229d2a0ULL);
        *reinterpret_cast<uint64_t*>(pPayload + 0x8) = param2cd0;
        *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
    }

    // Enqueue task 3: sub_22b0490(0)
    pPayload = gx_alloc_and_submit_cmd_node(
        envBuilder.pCmdQueue,
        envBuilder.streamWord0,
        envBuilder.streamWord1,
        envBuilder.needHeader,
        envBuilder.flag2,
        envBuilder.flag3);
    if (pPayload != nullptr) {
        *reinterpret_cast<uint64_t*>(pPayload + 0x0) = GX_VMA(0x22b0490ULL);
        *reinterpret_cast<uint64_t*>(pPayload + 0x8) = 0;
        *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
    }

    // Enqueue task 4: sub_22af0b0(pCubeTargetState, pVpState, flag79d0)
    uint64_t flag79d0 = pMgr[0x79d0];
    pPayload = gx_alloc_and_submit_cmd_node(
        envBuilder.pCmdQueue,
        envBuilder.streamWord0,
        envBuilder.streamWord1,
        envBuilder.needHeader,
        envBuilder.flag2,
        envBuilder.flag3);
    bool shouldFlushCmd = (envBuilder.needHeader == 0);
    if (pPayload != nullptr) {
        *reinterpret_cast<uint64_t*>(pPayload + 0x0) = GX_VMA(0x22af0b0ULL);
        *reinterpret_cast<void**>(pPayload + 0x8) = pCubeTargetState;
        *reinterpret_cast<uint64_t*>(pPayload + 0x10) = flag79d0;
        shouldFlushCmd = true;
    }
    envBuilder.flag0 = 0;
    envBuilder.flag3 = 0;
    envBuilder.flag2 = 0;
    if (shouldFlushCmd) {
        sub_2289bb0(envBuilder.pCmdQueue, 0xff, 0, 4);
    }

    // Enqueue barrier command 0xf8
    envBuilder.flag0 = 1;
    envBuilder.needHeader = 1;
    envBuilder.flag3 = 1;
    envBuilder.streamWord1 = (5ULL << 32);
    uint8_t* pBarrierNode = sub_2289bb0(envBuilder.pCmdQueue, 0xf8, 0x8, 4);
    if (pBarrierNode != nullptr) {
        if (envBuilder.needHeader != 0) {
            uint64_t w0 = envBuilder.streamWord0;
            uint64_t w1 = envBuilder.streamWord1;
            uint32_t packed = (static_cast<uint32_t>(w0) << 28) |
                              (static_cast<uint32_t>(w0 >> 10) & 0x0fc00000u) |
                              ((static_cast<uint32_t>(w1) & 0x3fu) << 16) |
                              static_cast<uint32_t>(static_cast<uint16_t>(w1 >> 32));
            struct alignas(8) CmdPacketDesc {
                uint32_t packedHeader;
                uint32_t pad0;
                void* pNodePtr;
                uint8_t subFlag0;
                uint8_t subFlag1;
                uint8_t pad1[6];
            } desc{packed, 0, pBarrierNode, 0, envBuilder.flag3, {}};
            sub_2289ca0(envBuilder.pCmdQueue, &desc);
            envBuilder.needHeader = 0;
        }
        envBuilder.flag0 = 0;
        envBuilder.flag3 = 0;
        envBuilder.flag2 = 0;
        sub_2289bb0(envBuilder.pCmdQueue, 0xff, 0, 4);
    } else {
        uint8_t prevNeed = envBuilder.needHeader;
        envBuilder.flag0 = 0;
        envBuilder.flag3 = 0;
        envBuilder.flag2 = 0;
        if (prevNeed == 0) {
            sub_2289bb0(envBuilder.pCmdQueue, 0xff, 0, 4);
        }
    }

    // Bind cubemap face render targets and enqueue sky quad callback (sub_22af0e0)
    envBuilder.flag0 = 1;
    envBuilder.needHeader = 1;
    envBuilder.flag3 = 0;
    envBuilder.streamWord1 = 0x3fULL;

    uint8_t* pTargetBank = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0)
                               ? (pMgr + 0x3208)
                               : (pMgr + 0x2de8);
    sub_228ea50(
        &envBuilder,
        *reinterpret_cast<void**>(pTargetBank + 0x30),
        *reinterpret_cast<void**>(pTargetBank + 0x40));

    pPayload = gx_alloc_and_submit_cmd_node(
        envBuilder.pCmdQueue,
        envBuilder.streamWord0,
        envBuilder.streamWord1,
        envBuilder.needHeader,
        envBuilder.flag2,
        envBuilder.flag3);
    if (pPayload != nullptr) {
        *reinterpret_cast<uint64_t*>(pPayload + 0x0) = GX_VMA(0x22af0e0ULL);
        *reinterpret_cast<void**>(pPayload + 0x8) = pVpState;
        *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
    }

    sub_228ece0(&envBuilder);

    void* pSceneGraph = *reinterpret_cast<void**>(pMgr + 0x76f0);
    uint8_t* pActiveBank = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0)
                               ? (pMgr + 0x3208)
                               : (pMgr + 0x2de8);
    void* pTargetObj = *reinterpret_cast<void**>(pActiveBank + 0x18);
    void** pTargetVt = *reinterpret_cast<void***>(pTargetObj);
    void* pTargetView = reinterpret_cast<FnVtableRetPtr>(pTargetVt[5])(pTargetObj);

    sub_228f010(
        &envBuilder,
        pSceneGraph,
        static_cast<uint64_t>(faceIdx),
        0,
        0,
        0,
        pTargetView,
        0,
        0);

    envBuilder.flag0 = 0;
    envBuilder.flag3 = 0;
    envBuilder.flag2 = 0;
    if (envBuilder.needHeader == 0) {
        sub_2289bb0(envBuilder.pCmdQueue, 0xff, 0, 4);
    }

    // Enqueue end callback: sub_22af5a0(pCubeTargetState)
    envBuilder.flag0 = 1;
    envBuilder.needHeader = 1;
    envBuilder.flag3 = 1;
    envBuilder.streamWord1 = 0x3fULL;

    pPayload = gx_alloc_and_submit_cmd_node(
        envBuilder.pCmdQueue,
        envBuilder.streamWord0,
        envBuilder.streamWord1,
        envBuilder.needHeader,
        envBuilder.flag2,
        envBuilder.flag3);
    shouldFlushCmd = (envBuilder.needHeader == 0);
    if (pPayload != nullptr) {
        *reinterpret_cast<uint64_t*>(pPayload + 0x0) = GX_VMA(0x22af5a0ULL);
        *reinterpret_cast<void**>(pPayload + 0x8) = pCubeTargetState;
        *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
        shouldFlushCmd = true;
    }
    envBuilder.flag0 = 0;
    envBuilder.flag3 = 0;
    envBuilder.flag2 = 0;
    if (shouldFlushCmd) {
        sub_2289bb0(envBuilder.pCmdQueue, 0xff, 0, 4);
    }

    envBuilder.streamWord1 &= 0xffffffffULL;
    sub_229e990(
        pMgr,
        pArg1,
        localCamParams,
        localCamParams,
        pArg3,
        pCubeTargetState,
        passFlags,
        passId,
        pStackArg0);
    sub_22d5480(&envBuilder);
}

// ============================================================================
// 7. sub_22ae480 (0x22ae480) - 125 bytes
// GXSceneRenderPass_SubmitReflectionPassIfActive
// ============================================================================
void sub_22ae480(
    void* pRenderPassMgr,
    void* pArg1,
    void* pArg2,
    void* pArg3,
    void* pArg4,
    void* pViewportState,
    uint32_t passFlags,
    void* pStackArg1) {
    uint8_t* pMgr = static_cast<uint8_t*>(pRenderPassMgr);
    uint8_t* pVpState = static_cast<uint8_t*>(pViewportState);

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8[0x3a9] != 0) {
        return;
    }
    if (pMgr[0x7980] != 0 && pMgr[0x7981] != 0) {
        return;
    }
    if (*reinterpret_cast<uint32_t*>(pMgr + 0x7990) == 2u) {
        return;
    }
    void* pReflTarget = *reinterpret_cast<void**>(pVpState + 0x128);
    if (pReflTarget == nullptr) {
        return;
    }
    if (pVpState[0x2a1f] == 0 && *reinterpret_cast<int32_t*>(pVpState + 0x2a20) <= 0) {
        return;
    }

    sub_229e990(pMgr, pArg1, pArg2, pArg3, pArg4, pReflTarget, passFlags, 0x36u, pStackArg1);
}

// ============================================================================
// 8. sub_22ae420 (0x22ae420) - 90 bytes
// GXSceneRenderPass_SubmitMainScenePassIfActive
// ============================================================================
void sub_22ae420(
    void* pRenderPassMgr,
    void* pArg1,
    void* pArg2,
    void* pArg3,
    void* pArg4,
    void* pArg5,
    uint32_t passFlags,
    void* pStackArg1) {
    uint8_t* pMgr = static_cast<uint8_t*>(pRenderPassMgr);

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8[0x3a9] != 0) {
        return;
    }
    if (pMgr[0x7980] != 0 && pMgr[0x7981] != 0) {
        return;
    }
    if (*reinterpret_cast<uint32_t*>(pMgr + 0x7990) == 2u) {
        return;
    }

    sub_229e990(pMgr, pArg1, pArg2, pArg3, pArg4, pArg5, passFlags, 0x3eu, pStackArg1);
}

// ============================================================================
// 9. sub_22ae0a0 (0x22ae0a0) - 886 bytes
// GXSceneRenderPass_CopyDepthOrColorTargetQuad
// ============================================================================
void sub_22ae0a0(void* pTaskContext, void* pSrcTarget, void* pDstTarget) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pSrc = static_cast<uint8_t*>(pSrcTarget);
    uint8_t* pDst = static_cast<uint8_t*>(pDstTarget);

    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pShaderRepo = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6078);

    void* pDstColorSurface = *reinterpret_cast<void**>(pDst + 0x30);
    void* pTargetMgr = pDrawCtx + 0x1728;
    sub_e46f40(pTargetMgr, pDstColorSurface, nullptr, 1);
    *reinterpret_cast<void**>(pDrawCtx + 0x1ec0) = sub_e47840(pTargetMgr);
    sub_21a1c50(pDrawCtx);

    void* pSrcTexture = *reinterpret_cast<void**>(pSrc + 0x48);
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = pSrcTexture;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    uint32_t srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax <= 1u) ? 1u : srvMax;

    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = pSrcTexture;

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pPointSampler = *reinterpret_cast<void**>(pDefaultStates + 0x2d8);
    *reinterpret_cast<void**>(pDrawCtx + 0x9e0) = pPointSampler;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa60) = 0;
    uint32_t smpMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa64);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa64) = (smpMax <= 1u) ? 1u : smpMax;

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd0a8) = pPointSampler;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pOpaqueBS = *reinterpret_cast<void**>(pDefaultStates + 0x0);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pOpaqueBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pOpaqueBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pNoDepthDSS = *reinterpret_cast<void**>(pDefaultStates + 0x1c8);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pNoDepthDSS;
    pDrawCtx[0x218] = 0xff;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pNoDepthDSS;
    pDrawImpl[0xbcb4] = 0xff;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pNoCullRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pNoCullRS;
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pNoCullRS;

    void* pInputLayout = *reinterpret_cast<void**>(pShaderRepo + 0x9a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    uint8_t* pVsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0xa28);
    void* pVertexShader = *reinterpret_cast<void**>(pVsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0x780) = pVertexShader;
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = pVertexShader;

    *reinterpret_cast<void**>(pDrawCtx + 0x1320) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xc6b0) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x1038) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xcba8) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0xd50) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd0a0) = nullptr;

    uint8_t* pPsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0x9d0);
    void* pPixelShader = *reinterpret_cast<void**>(pPsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pPixelShader;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pPixelShader;

    uint8_t* pGeomCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    gx_draw_fullscreen_quad(pGeomCtx, pShaderRepo);

    *reinterpret_cast<void**>(pDrawCtx + 0x780) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x788) = nullptr;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax <= 1u) ? 1u : srvMax;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = nullptr;

    sub_e47290(pTargetMgr, 1);
    *reinterpret_cast<void**>(pDrawCtx + 0x1ec0) = sub_e47840(pTargetMgr);
    pDrawCtx[0x1670] &= 0xfeu;
}

// ============================================================================
// 10. sub_22ae030 (0x22ae030) - 109 bytes
// GXSceneRenderPass_ResolveMsaaColorTargetIfRequired
// ============================================================================
void sub_22ae030(void* pTaskContext, void* pViewportState) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pVpState = static_cast<uint8_t*>(pViewportState);

    if (pVpState[0x2a1e] == 0) {
        return;
    }

    uint8_t* pTargetSlot = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0)
                               ? (pVpState + 0x5a8)
                               : (pVpState + 0x188);
    uint8_t* pSurface = *reinterpret_cast<uint8_t**>(pTargetSlot + 0x30);
    uint8_t* pTexDesc = *reinterpret_cast<uint8_t**>(pSurface + 0x8);

    if (pTexDesc[0x38] == 3 && *reinterpret_cast<uint16_t*>(pTexDesc + 0x4c) > 1u) {
        return;
    }

    uint8_t* pResolveCtx = *reinterpret_cast<uint8_t**>(pTask + 0x438);
    uint8_t* pResolveImpl = *reinterpret_cast<uint8_t**>(pResolveCtx + 0x8);
    void* pSrcTargetObj = *reinterpret_cast<void**>(pVpState + 0x1060);
    void** pSrcVt = *reinterpret_cast<void***>(pSrcTargetObj);
    void* pSrcSurface = reinterpret_cast<FnVtableRetPtr>(pSrcVt[5])(pSrcTargetObj);

    pSurface = *reinterpret_cast<uint8_t**>(pTargetSlot + 0x30);
    void* pDstSurface = *reinterpret_cast<void**>(pSurface + 0x8);
    sub_2168660(pResolveImpl, pResolveImpl + 0xb730, pSrcSurface, pDstSurface);
}

// ============================================================================
// 11. sub_22ad0e0 (0x22ad0e0) - 3916 bytes
// GXFeedBackBlurRenderPass_ExecuteMaskAndFallOffPasses
// Shaders:
//   - u"FeedBackBlurMaskMargePS.ppo" (0x49cee18)
//   - u"FeedBackBlurMaskPS.ppo"      (0x49cee50)
//   - u"FeedBackBlurFallOffPS.ppo"   (0x49cee7e)
// ============================================================================
void sub_22ad0e0(void* pTaskContext, void* pViewportState) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pVpState = static_cast<uint8_t*>(pViewportState);

    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    float blurBlendT = *reinterpret_cast<float*>(pVpState + 0x2b5c);

    void* pBlurTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x59a8);
    uint8_t* pBlurRow0 = static_cast<uint8_t*>(sub_21f2b90(pBlurTable, pVpState + 0x2aa0));
    pBlurTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x59a8);
    uint8_t* pBlurRow1 = static_cast<uint8_t*>(sub_21f2b90(pBlurTable, pVpState + 0x2b08));

    alignas(16) float blurParams[8] = {
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.8f, 0.9f, 4.5f
    };
    sub_e788f0(blurParams, pBlurRow0 + 0x8, pBlurRow1 + 0x8, blurBlendT);

    float paramD4 = blurParams[5];
    float paramD8 = blurParams[6];
    float paramDc = blurParams[7];

    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pTask + 0x430);
    sub_219fcf0(pDrawCtx);

    void* pSavedColorTarget = *reinterpret_cast<void**>(pDrawCtx + 0x18);
    void* pSavedDepthTarget = *reinterpret_cast<void**>(pDrawCtx + 0x58);
    uint32_t savedVpCount = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xc8);
    uint8_t* pVpArrayBase = pDrawCtx + 0xcc;
    size_t savedVpBytes = static_cast<size_t>(savedVpCount) * 24u;

    alignas(16) uint8_t savedViewports[0xc0];
    sub_261a1b0(savedViewports, pVpArrayBase, savedVpBytes);

    uint8_t* pShaderRepo = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6078);
    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);

    alignas(32) uint8_t clearBlock[0x48] = {};
    sub_26d3070(pDrawImpl + 0xb558, pDrawImpl, clearBlock);
    sub_26b9850(pDrawImpl + 0xb478, pDrawImpl + 0x10);
    sub_26cd190(pDrawImpl + 0x50, pDrawImpl);
    sub_26af700(pDrawImpl + 0x10);
    sub_26bba10(pDrawImpl + 0xb518);
    sub_26d3300(pDrawImpl + 0xb528);
    sub_26d2d20(pDrawImpl + 0xb558);

    std::memset(pDrawImpl + 0xb730, 0, 0x23a8);

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);

    void* pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    void* pOpaqueBS = *reinterpret_cast<void**>(pDefaultStates + 0x0);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pOpaqueBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pOpaqueBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    void* pNoDepthDSS = *reinterpret_cast<void**>(pDefaultStates + 0x1c8);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pNoDepthDSS;
    pDrawCtx[0x218] = 0xff;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pNoDepthDSS;
    pDrawImpl[0xbcb4] = 0xff;

    // ------------------------------------------------------------------------
    // Stage 1: FeedBackBlurMaskMargePS.ppo
    // ------------------------------------------------------------------------
    void* pBlurRT0 = *reinterpret_cast<void**>(pVpState + 0x1030);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pDrawCtx + 0x18) = pBlurRT0;
    *reinterpret_cast<void**>(pDrawCtx + 0x58) = nullptr;
    pDrawImpl[0xb656] = 1;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc10) = pBlurRT0;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc50) = nullptr;
    pDrawImpl[0xbcb5] = 1;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pAlphaBS = *reinterpret_cast<void**>(pDefaultStates + 0x8);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pAlphaBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pAlphaBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    uint32_t rtWidth = *reinterpret_cast<uint32_t*>(pVpState + 0x1050);
    uint32_t rtHeight = *reinterpret_cast<uint32_t*>(pVpState + 0x1054);
    void* pShaderPkgSlot = pShaderRepo + 0x9a8;

    uint8_t* pVsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0xa28);
    void* pVertexShader = *reinterpret_cast<void**>(pVsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0x780) = pVertexShader;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = pVertexShader;

    static void* s_cachedPkgSlot = nullptr;
    static void* s_cachedMergePS = nullptr;
    static void* s_cachedMaskPS = nullptr;
    static void* s_cachedFallOffPS = nullptr;
    if (s_cachedPkgSlot != pShaderPkgSlot || s_cachedMergePS == nullptr) {
        s_cachedPkgSlot = pShaderPkgSlot;
        void* pMergeShaderObj = sub_2274be0(pShaderPkgSlot, reinterpret_cast<const char16_t*>(GX_VMA(0x49cee18ULL)));
        s_cachedMergePS = nullptr;
        if (pMergeShaderObj != nullptr) {
            void** pVt = *reinterpret_cast<void***>(pMergeShaderObj);
            s_cachedMergePS = reinterpret_cast<FnVtableRetPtr>(pVt[3])(pMergeShaderObj);
        }
        void* pMaskShaderObj = sub_2274be0(pShaderPkgSlot, reinterpret_cast<const char16_t*>(GX_VMA(0x49cee50ULL)));
        s_cachedMaskPS = nullptr;
        if (pMaskShaderObj != nullptr) {
            void** pVt = *reinterpret_cast<void***>(pMaskShaderObj);
            s_cachedMaskPS = reinterpret_cast<FnVtableRetPtr>(pVt[3])(pMaskShaderObj);
        }
        void* pFallOffShaderObj = sub_2274be0(pShaderPkgSlot, reinterpret_cast<const char16_t*>(GX_VMA(0x49cee7eULL)));
        s_cachedFallOffPS = nullptr;
        if (pFallOffShaderObj != nullptr) {
            void** pVt = *reinterpret_cast<void***>(pFallOffShaderObj);
            s_cachedFallOffPS = reinterpret_cast<FnVtableRetPtr>(pVt[3])(pFallOffShaderObj);
        }
    }
    void* pMergePS = s_cachedMergePS;

    float fWidth = static_cast<float>(static_cast<int64_t>(rtWidth));
    float fHeight = static_cast<float>(static_cast<int64_t>(rtHeight));

    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pMergePS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pMergePS;

    void* pTex0fd0 = *reinterpret_cast<void**>(pVpState + 0xfd0);
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = pTex0fd0;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    uint32_t srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 1u) ? srvMax : 1u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = pTex0fd0;

    void* pBlurCBuffer = *reinterpret_cast<void**>(pShaderRepo + 0x958);
    *reinterpret_cast<void**>(pDrawCtx + 0x970) = pBlurCBuffer;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) =
        (*reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) != 0) ? 1u : 0u;
    uint32_t psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 2u) ? psMax : 2u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x20000ULL;
    gx_mark_resource_bound(pBlurCBuffer, 0x100002ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd530) = pBlurCBuffer;

    alignas(16) float mergeCbData[4] = {1.0f - paramD4, paramD8, paramDc, 0.0f};
    pBlurCBuffer = *reinterpret_cast<void**>(pShaderRepo + 0x958);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pBlurCBuffer, 0, 0, mergeCbData, 0, 0);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pSampler = *reinterpret_cast<void**>(pDefaultStates + 0x2d8);

    // Bind sampler to PS slot 0 and slot 3
    *reinterpret_cast<void**>(pDrawCtx + 0x9e0) = pSampler;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa60) = 0;
    uint32_t smpMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa64);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa64) = (smpMax <= 1u) ? 1u : smpMax;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd0a8) = pSampler;

    *reinterpret_cast<void**>(pDrawCtx + 0x9f8) = pSampler;
    uint32_t smpMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa60);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa60) = (smpMin >= 3u) ? 3u : smpMin;
    smpMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa64);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xa64) = (smpMax <= 4u) ? 4u : smpMax;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x8ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd0c0) = pSampler;

    // Set 1 viewport (0, 0, fWidth, fHeight, 0.0f, 1.0f)
    alignas(16) float vpRect[4] = {0.0f, 0.0f, fWidth, fHeight};
    uint32_t curVpCount = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xc8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xc8) = (curVpCount <= 1u) ? 1u : curVpCount;
    _mm_storeu_ps(reinterpret_cast<float*>(pVpArrayBase), _mm_load_ps(vpRect));
    *reinterpret_cast<uint64_t*>(pDrawCtx + 0xdc) = 0x3f80000000000000ULL;

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_26d3350(pDrawImpl + 0xb528, 1);
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbbf8) = 1;
    _mm_storeu_ps(reinterpret_cast<float*>(pDrawImpl + 0xb978), _mm_load_ps(vpRect));
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xb988) = 0x3f80000000000000ULL;

    void* pInputLayout = *reinterpret_cast<void**>(pShaderRepo + 0x9a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    gx_draw_fullscreen_quad(pDrawCtx, pShaderRepo);

    // Unbind SRV slot 0 and RT0, then flush
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = nullptr;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax <= 1u) ? 1u : srvMax;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = nullptr;

    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pDrawCtx + 0x18) = nullptr;
    *reinterpret_cast<void**>(pDrawCtx + 0x58) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    pDrawImpl[0xb656] = 1;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc10) = nullptr;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc50) = nullptr;
    pDrawImpl[0xbcb5] = 1;

    sub_219fcf0(pDrawCtx);

    // ------------------------------------------------------------------------
    // Stage 2: FeedBackBlurMaskPS.ppo
    // ------------------------------------------------------------------------
    uint8_t* pTexWrapper1058 = *reinterpret_cast<uint8_t**>(pVpState + 0x1058);
    void* pTex1058 = *reinterpret_cast<void**>(pTexWrapper1058 + 0x48);
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = pTex1058;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 1u) ? srvMax : 1u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = pTex1058;

    void* pTex1028 = *reinterpret_cast<void**>(pVpState + 0x1028);
    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = pTex1028;
    uint32_t srvMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = (srvMin < 3u) ? srvMin : 3u;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 4u) ? srvMax : 4u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x8ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = pTex1028;

    void* pMaskPS = s_cachedMaskPS;

    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pMaskPS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pMaskPS;

    *reinterpret_cast<void**>(pDrawCtx + 0x970) = pBlurCBuffer;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) =
        (*reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) != 0) ? 1u : 0u;
    psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 2u) ? psMax : 2u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x20000ULL;
    gx_mark_resource_bound(pBlurCBuffer, 0x100002ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd530) = pBlurCBuffer;

    alignas(16) float maskCbData[4] = {paramD4, paramD8, paramDc, 0.0f};
    pBlurCBuffer = *reinterpret_cast<void**>(pShaderRepo + 0x958);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pBlurCBuffer, 0, 0, maskCbData, 0, 0);

    uint8_t* pTargetSlot = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0)
                               ? (pVpState + 0x5a8)
                               : (pVpState + 0x188);
    void* pMainRT = *reinterpret_cast<void**>(pTargetSlot + 0x30);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pDrawCtx + 0x18) = pMainRT;
    *reinterpret_cast<void**>(pDrawCtx + 0x58) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    pDrawImpl[0xb656] = 1;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc10) = pMainRT;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc50) = nullptr;
    pDrawImpl[0xbcb5] = 1;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    pAlphaBS = *reinterpret_cast<void**>(pDefaultStates + 0x8);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pAlphaBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pAlphaBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    pInputLayout = *reinterpret_cast<void**>(pShaderRepo + 0x9a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    gx_draw_fullscreen_quad(pDrawCtx, pShaderRepo);

    // Unbind SRV slots 0 and 3 and RT0, then flush
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = nullptr;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 1u) ? srvMax : 1u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = nullptr;
    srvMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = (srvMin < 3u) ? srvMin : 3u;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 4u) ? srvMax : 4u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x8ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = nullptr;

    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pDrawCtx + 0x18) = nullptr;
    *reinterpret_cast<void**>(pDrawCtx + 0x58) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    pDrawImpl[0xb656] = 1;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc10) = nullptr;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc50) = nullptr;
    pDrawImpl[0xbcb5] = 1;

    sub_219fcf0(pDrawCtx);

    // ------------------------------------------------------------------------
    // Stage 3: Copy targets + FeedBackBlurFallOffPS.ppo
    // ------------------------------------------------------------------------
    pTexWrapper1058 = *reinterpret_cast<uint8_t**>(pVpState + 0x1058);
    void* pCopyDst0 = *reinterpret_cast<void**>(pTexWrapper1058 + 0x40);
    pTargetSlot = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0)
                      ? (pVpState + 0x5a8)
                      : (pVpState + 0x188);
    uint8_t* pMainSlot18 = *reinterpret_cast<uint8_t**>(pTargetSlot + 0x18);
    void* pCopySrc0 = *reinterpret_cast<void**>(pMainSlot18 + 0x40);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168660(pDrawImpl, pDrawImpl + 0xb730, pCopyDst0, pCopySrc0);

    uint8_t* pSlot0fc0 = *reinterpret_cast<uint8_t**>(pVpState + 0xfc0);
    uint8_t* pSlot1018 = *reinterpret_cast<uint8_t**>(pVpState + 0x1018);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168660(
        pDrawImpl,
        pDrawImpl + 0xb730,
        *reinterpret_cast<void**>(pSlot1018 + 0x40),
        *reinterpret_cast<void**>(pSlot0fc0 + 0x40));

    void* pFallOffPS = s_cachedFallOffPS;

    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pFallOffPS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pFallOffPS;

    pBlurRT0 = *reinterpret_cast<void**>(pVpState + 0x1030);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pDrawCtx + 0x18) = pBlurRT0;
    *reinterpret_cast<void**>(pDrawCtx + 0x58) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    pDrawImpl[0xb656] = 1;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc10) = pBlurRT0;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc50) = nullptr;
    pDrawImpl[0xbcb5] = 1;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    pOpaqueBS = *reinterpret_cast<void**>(pDefaultStates + 0x0);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pOpaqueBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pOpaqueBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    pTex0fd0 = *reinterpret_cast<void**>(pVpState + 0xfd0);
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = pTex0fd0;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax <= 1u) ? 1u : srvMax;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = pTex0fd0;

    pInputLayout = *reinterpret_cast<void**>(pShaderRepo + 0x9a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    gx_draw_fullscreen_quad(pDrawCtx, pShaderRepo);

    // Unbind SRV slots 0 and 3 and restore previous render target & viewports
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = nullptr;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 1u) ? srvMax : 1u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = nullptr;
    srvMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = (srvMin < 3u) ? srvMin : 3u;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 4u) ? srvMax : 4u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x8ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = nullptr;

    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pDrawCtx + 0x18) = pSavedColorTarget;
    *reinterpret_cast<void**>(pDrawCtx + 0x58) = pSavedDepthTarget;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    pDrawImpl[0xb656] = 1;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc10) = pSavedColorTarget;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc50) = pSavedDepthTarget;
    pDrawImpl[0xbcb5] = 1;

    curVpCount = *reinterpret_cast<uint32_t*>(pDrawCtx + 0xc8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xc8) =
        (curVpCount <= savedVpCount) ? savedVpCount : curVpCount;
    if (savedVpCount != 0) {
        sub_261a1b0(pVpArrayBase, savedViewports, savedVpBytes);
    }

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_26d3350(pDrawImpl + 0xb528, savedVpCount);
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbbf8) = savedVpCount;
    if (savedVpCount != 0) {
        sub_261a1b0(pDrawImpl + 0xb978, savedViewports, savedVpBytes);
    }

    sub_219fcf0(pDrawCtx);
}

// ============================================================================
// 12. sub_22ad020 (0x22ad020) - 186 bytes
// GXSceneRenderPass_SubmitPostLightingCallback
// ============================================================================
void sub_22ad020(void* pTaskContext) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pGeomSlot = pTask + 0x440;

    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pSubMgr6070 = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6070);
    uint8_t* pTable5948 = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x5948);

    uint64_t rawKey = *reinterpret_cast<uint64_t*>(pTable5948 + 0x108);
    uint32_t keyPair[2] = {
        static_cast<uint32_t>(rawKey),
        static_cast<uint32_t>(rawKey >> 32)
    };

    void** pTargetSlot = nullptr;
    if (sub_221efb0(pTable5948, keyPair)) {
        uint8_t* pEntry = static_cast<uint8_t*>(sub_221eaa0(pTable5948, keyPair));
        pTargetSlot = reinterpret_cast<void**>(pEntry + 0x48);
    } else if (*reinterpret_cast<void**>(pSubMgr6070 + 0x1d0) != nullptr) {
        pTargetSlot = reinterpret_cast<void**>(pSubMgr6070 + 0x1d0);
    } else {
        pTargetSlot = reinterpret_cast<void**>(pSubMgr6070 + 0x150);
    }

    sub_21baff0(pGeomSlot, *pTargetSlot);
}

// ============================================================================
// 13. sub_22ac970 (0x22ac970) - 1700 bytes
// GXSceneRenderPass_ClearAndFillBackgroundColorQuad
// ============================================================================
void sub_22ac970(void* pTaskContext, void* pClearParams) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pParams = static_cast<uint8_t*>(pClearParams);

    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);

    // Unpack RGBA8 color at +0x5c, square RGB channels, and requantize via 8-bit integer roundtrip
    uint32_t packedColor = *reinterpret_cast<uint32_t*>(pParams + 0x5c);
    const float kInv255 = 1.0f / 255.0f;

    float rNorm = static_cast<float>((packedColor >> 16) & 0xffu) * kInv255;
    float gNorm = static_cast<float>((packedColor >> 8) & 0xffu) * kInv255;
    float bNorm = static_cast<float>(packedColor & 0xffu) * kInv255;
    float aNorm = static_cast<float>(packedColor >> 24) * kInv255;

    __m128 rgba = _mm_setr_ps(rNorm, gNorm, bNorm, aNorm);
    __m128 rgbaSq = _mm_mul_ps(rgba, rgba);
    __m128 linearRgba = _mm_shuffle_ps(rgbaSq, rgba, 0xc8);
    linearRgba = _mm_shuffle_ps(rgbaSq, linearRgba, 0xd4);

    linearRgba = _mm_max_ps(linearRgba, _mm_setzero_ps());
    linearRgba = _mm_min_ps(linearRgba, _mm_set1_ps(1.0f));
    __m128 scaled255 = _mm_mul_ps(linearRgba, _mm_set1_ps(255.0f));

    // Exact IEEE-754 round-to-nearest-even for |x| < 8388608.0f
    const __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    const __m128 signBit = _mm_castsi128_ps(_mm_set1_epi32(static_cast<int32_t>(0x80000000u)));
    const __m128i negTwoPow23 = _mm_set1_epi32(static_cast<int32_t>(0xcb000000u));

    __m128 negAbs = _mm_xor_ps(_mm_and_ps(scaled255, absMask), signBit);
    __m128 cmpMask = _mm_castsi128_ps(_mm_cmpgt_epi32(negTwoPow23, _mm_castps_si128(negAbs)));
    __m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(scaled255));
    scaled255 = _mm_or_ps(_mm_and_ps(cmpMask, rounded), _mm_andnot_ps(cmpMask, scaled255));

    int64_t qR = _mm_cvttss_si64(scaled255);
    int64_t qG = _mm_cvttss_si64(_mm_shuffle_ps(scaled255, scaled255, 0x01));
    int64_t qB = _mm_cvttss_si64(_mm_movehl_ps(scaled255, scaled255));
    int64_t qA = _mm_cvttss_si64(_mm_shuffle_ps(scaled255, scaled255, 0x03));

    alignas(16) float finalColor[4] = {
        static_cast<float>(static_cast<uint8_t>(qR)) * kInv255,
        static_cast<float>(static_cast<uint8_t>(qG)) * kInv255,
        static_cast<float>(static_cast<uint8_t>(qB)) * kInv255,
        static_cast<float>(static_cast<uint8_t>(qA)) * kInv255
    };

    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pTask + 0x430);
    gx_push_depth_stencil_state(pDrawCtx);
    gx_push_blend_state(pDrawCtx);
    gx_push_rasterizer_state(pDrawCtx);

    void* pOpaqueBS = *reinterpret_cast<void**>(pDefaultStates + 0x0);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pOpaqueBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pOpaqueBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    void* pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    void* pClearDSS = *reinterpret_cast<void**>(pDefaultStates + 0x1d8);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pClearDSS;
    pDrawCtx[0x218] = 0xff;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pClearDSS;
    pDrawImpl[0xbcb4] = 0xff;

    uint8_t* pShaderRepo = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6078);
    uint8_t* pVsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0xa30);
    void* pInputLayout = *reinterpret_cast<void**>(pShaderRepo + 0x9b0);
    uint8_t* pPsWrapper = *reinterpret_cast<uint8_t**>(pShaderRepo + 0xa18);

    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    void* pVertexShader = *reinterpret_cast<void**>(pVsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0x780) = pVertexShader;
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = pVertexShader;

    void* pPixelShader = *reinterpret_cast<void**>(pPsWrapper + 0x10);
    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pPixelShader;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pPixelShader;

    *reinterpret_cast<void**>(pDrawCtx + 0x1320) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xc6b0) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x1038) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xcba8) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0xd50) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd0a0) = nullptr;

    // Upload PS CBuffer slot 2 (+0x968)
    void* pPsCbuf2 = *reinterpret_cast<void**>(pShaderRepo + 0x968);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pPsCbuf2, 0, 0, finalColor, 0, 0);

    *reinterpret_cast<void**>(pDrawCtx + 0x978) = pPsCbuf2;
    uint32_t psMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) = (psMin < 2u) ? psMin : 2u;
    uint32_t psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 3u) ? psMax : 3u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x40000ULL;
    gx_mark_resource_bound(pPsCbuf2, 0x100004ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd538) = pPsCbuf2;

    // Upload VS CBuffer slot 8 (+0x998)
    void* pVsCbuf8 = *reinterpret_cast<void**>(pShaderRepo + 0x998);
    alignas(16) const float kVsDepthVec[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pVsCbuf8, 0, 0, kVsDepthVec, 0, 0);

    *reinterpret_cast<void**>(pDrawCtx + 0x6c0) = pVsCbuf8;
    uint32_t vsMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0) = (vsMin < 8u) ? vsMin : 8u;
    uint32_t vsMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4) = (vsMax > 9u) ? vsMax : 9u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x78) |= 0x1000000ULL;
    gx_mark_resource_bound(pVsCbuf8, 0x10100ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xc188) = pVsCbuf8;

    uint8_t* pGeomCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    gx_draw_fullscreen_quad(pGeomCtx, pShaderRepo);

    gx_pop_rasterizer_state(pDrawCtx);
    gx_pop_blend_state(pDrawCtx);
    gx_pop_depth_stencil_state(pDrawCtx);
}

// ============================================================================
// 14. sub_22ac130 (0x22ac130) - 2099 bytes
// GXSceneRenderPass_CompositeDepthAndColorLayers
// ============================================================================
void sub_22ac130(void* pTaskContext, void* pTexSlot3, void* pTexSlot33) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pShaderRepo = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6078);
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pTask + 0x430);

    gx_push_blend_state(pDrawCtx);
    gx_push_depth_stencil_state(pDrawCtx);
    gx_push_rasterizer_state(pDrawCtx);

    void* pCompositeBS = *reinterpret_cast<void**>(pDefaultStates + 0x28);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pCompositeBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pCompositeBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    void* pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    // Bind VS CBuffer slot 4 (+0x6680)
    void* pVsCbuf4 = **reinterpret_cast<void***>(pGlobalDd8 + 0x6680);
    *reinterpret_cast<void**>(pDrawCtx + 0x6a0) = pVsCbuf4;
    uint32_t vsMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0) = (vsMin < 4u) ? vsMin : 4u;
    uint32_t vsMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4) = (vsMax > 5u) ? vsMax : 5u;
    pDrawImpl[0x7a] |= 0x10u;
    gx_mark_resource_bound(pVsCbuf4, 0x10010ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xc168) = pVsCbuf4;

    alignas(16) const float kZeroVec4[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    pVsCbuf4 = **reinterpret_cast<void***>(pGlobalDd8 + 0x6680);
    if (pVsCbuf4 != nullptr) {
        pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
        sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pVsCbuf4, 0, 0, kZeroVec4, 0, 0);
    }

    // Upload PS CBuffer slot 5 (+0x66e8) for pass 1
    alignas(16) const float kPass1Weights[8] = {
        0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f
    };
    void* pPsCbuf5 = **reinterpret_cast<void***>(pGlobalDd8 + 0x66e8);
    void* pBoundPsCbuf5 = nullptr;
    if (pPsCbuf5 != nullptr) {
        pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
        sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pPsCbuf5, 0, 0, kPass1Weights, 0, 0);
        pBoundPsCbuf5 = **reinterpret_cast<void***>(pGlobalDd8 + 0x66e8);
    }

    *reinterpret_cast<void**>(pDrawCtx + 0x990) = pBoundPsCbuf5;
    uint32_t psMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) = (psMin < 5u) ? psMin : 5u;
    uint32_t psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 6u) ? psMax : 6u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x200000ULL;
    gx_mark_resource_bound(pBoundPsCbuf5, 0x100020ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd550) = pBoundPsCbuf5;

    // Bind stencil state (stencilRef = 0x00)
    void* pStencilDSS = *reinterpret_cast<void**>(pGlobalDd8 + 0x65b0);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pStencilDSS;
    pDrawCtx[0x218] = 0x00;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pStencilDSS;
    pDrawImpl[0xbcb4] = 0x00;

    // Bind texture slot 3 (pTexSlot3) and slot 33 (pTexSlot33)
    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = pTexSlot3;
    uint32_t srvMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = (srvMin < 3u) ? srvMin : 3u;
    uint32_t srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax <= 4u) ? 4u : srvMax;
    pDrawImpl[0x1968] |= 0x8u;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = pTexSlot3;

    *reinterpret_cast<void**>(pDrawCtx + 0x890) = pTexSlot33;
    srvMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = (srvMin >= 0x21u) ? 0x21u : srvMin;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax <= 0x22u) ? 0x22u : srvMax;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x200000000ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd230) = pTexSlot33;

    void* pInputLayout = *reinterpret_cast<void**>(pGlobalDd8 + 0x60a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    void* pVertexShader = *reinterpret_cast<void**>(pGlobalDd8 + 0x60a0);
    *reinterpret_cast<void**>(pDrawCtx + 0x780) = pVertexShader;
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = pVertexShader;

    void* pPixelShader = *reinterpret_cast<void**>(pGlobalDd8 + 0x6578);
    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pPixelShader;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pPixelShader;

    uint8_t* pGeomCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    gx_draw_fullscreen_quad(pGeomCtx, pShaderRepo);

    // Upload PS CBuffer slot 5 (+0x66e8) for pass 2 (stencilRef = 0x10)
    alignas(16) const float kPass2Weights[8] = {
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f
    };
    pPsCbuf5 = **reinterpret_cast<void***>(pGlobalDd8 + 0x66e8);
    if (pPsCbuf5 != nullptr) {
        pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
        sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pPsCbuf5, 0, 0, kPass2Weights, 0, 0);
    }

    pStencilDSS = *reinterpret_cast<void**>(pGlobalDd8 + 0x65b0);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pStencilDSS;
    pDrawCtx[0x218] = 0x10;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pStencilDSS;
    pDrawImpl[0xbcb4] = 0x10;

    pGeomCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    gx_draw_fullscreen_quad(pGeomCtx, pShaderRepo);

    // Unbind SRV slots 0, 3 and 33
    *reinterpret_cast<void**>(pDrawCtx + 0x788) = nullptr;
    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = nullptr;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 4u) ? srvMax : 4u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x9ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = nullptr;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = nullptr;

    *reinterpret_cast<void**>(pDrawCtx + 0x890) = nullptr;
    srvMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = (srvMin < 0x21u) ? srvMin : 0x21u;
    srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 0x22u) ? srvMax : 0x22u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x200000000ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd230) = nullptr;

    gx_pop_rasterizer_state(pDrawCtx);
    gx_pop_depth_stencil_state(pDrawCtx);
    gx_pop_blend_state(pDrawCtx);
}

// ============================================================================
// 15. sub_22ab850 (0x22ab850) - 2260 bytes
// GXSceneRenderPass_RenderDeferredFogAndScattering
// ============================================================================
void sub_22ab850(void* pTaskContext, void* pViewportState) {
    uint8_t* pTask = static_cast<uint8_t*>(pTaskContext);
    uint8_t* pVpState = static_cast<uint8_t*>(pViewportState);

    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pShaderRepo = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6078);
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    uint32_t mode68 = *reinterpret_cast<uint32_t*>(pVpState + 0x68);
    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pTask + 0x430);

    gx_push_blend_state(pDrawCtx);
    gx_push_depth_stencil_state(pDrawCtx);
    gx_push_rasterizer_state(pDrawCtx);

    void* pOpaqueBS = *reinterpret_cast<void**>(pDefaultStates + 0x0);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pOpaqueBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pOpaqueBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    void* pFogDSS = *reinterpret_cast<void**>(pGlobalDd8 + 0x65a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pFogDSS;
    pDrawCtx[0x218] = 0x02;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pFogDSS;
    pDrawImpl[0xbcb4] = 0x02;

    void* pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    // Bind PS CBuffer slot 8 (+0x288) and slot 9 (+0x290)
    uint8_t* pSubMgr6070 = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6070);
    void* pPsCbuf8 = *reinterpret_cast<void**>(pSubMgr6070 + 0x288);
    *reinterpret_cast<void**>(pDrawCtx + 0x9a8) = pPsCbuf8;
    uint32_t psMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) = (psMin < 8u) ? psMin : 8u;
    uint32_t psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 9u) ? psMax : 9u;
    pDrawImpl[0x197b] |= 0x01u;
    gx_mark_resource_bound(pPsCbuf8, 0x100100ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd568) = pPsCbuf8;

    pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    pSubMgr6070 = *reinterpret_cast<uint8_t**>(pGlobalDd8 + 0x6070);
    void* pPsCbuf9 = *reinterpret_cast<void**>(pSubMgr6070 + 0x290);
    *reinterpret_cast<void**>(pDrawCtx + 0x9b0) = pPsCbuf9;
    psMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9d8) = (psMin < 9u) ? psMin : 9u;
    psMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x9dc) = (psMax > 10u) ? psMax : 10u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1978) |= 0x2000000ULL;
    gx_mark_resource_bound(pPsCbuf9, 0x100200ULL);
    *reinterpret_cast<void**>(pDrawImpl + 0xd570) = pPsCbuf9;

    // Bind 3 SRVs (slots 0, 1, 2) from pVpState + 0x1068, +0x1070, +0xf48
    uint8_t* pSlot1068 = *reinterpret_cast<uint8_t**>(pVpState + 0x1068);
    void* pSrv0 = *reinterpret_cast<void**>(pSlot1068 + 0x48);
    uint8_t* pSlot1070 = *reinterpret_cast<uint8_t**>(pVpState + 0x1070);
    void* pSrv1 = *reinterpret_cast<void**>(pSlot1070 + 0x48);
    uint8_t* pSlot0f48 = *reinterpret_cast<uint8_t**>(pVpState + 0xf48);
    void* pSrv2 = *reinterpret_cast<void**>(pSlot0f48 + 0x48);

    *reinterpret_cast<void**>(pDrawCtx + 0x788) = pSrv0;
    *reinterpret_cast<void**>(pDrawCtx + 0x790) = pSrv1;
    *reinterpret_cast<void**>(pDrawCtx + 0x798) = pSrv2;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x960) = 0;
    uint32_t srvMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x964) = (srvMax > 3u) ? srvMax : 3u;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x7ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd128) = pSrv0;
    *reinterpret_cast<void**>(pDrawImpl + 0xd130) = pSrv1;
    *reinterpret_cast<void**>(pDrawImpl + 0xd138) = pSrv2;

    void* pInputLayout = *reinterpret_cast<void**>(pGlobalDd8 + 0x6590);
    *reinterpret_cast<void**>(pDrawCtx + 0x260) = pInputLayout;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xb730) = pInputLayout;

    void* pVertexShader = *reinterpret_cast<void**>(pGlobalDd8 + 0x6588);
    *reinterpret_cast<void**>(pDrawCtx + 0x780) = pVertexShader;
    *reinterpret_cast<void**>(pDrawImpl + 0xc1b8) = pVertexShader;

    void* pPixelShader = *reinterpret_cast<void**>(pGlobalDd8 + 0x6580);
    *reinterpret_cast<void**>(pDrawCtx + 0xa68) = pPixelShader;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xd598) = pPixelShader;

    uint8_t* pGeomCtx = *reinterpret_cast<uint8_t**>(pTask + 0x440);
    uint8_t* pGeomSlot = pTask + 0x440;
    gx_draw_fullscreen_quad(pGeomCtx, pShaderRepo);

    gx_pop_rasterizer_state(pDrawCtx);
    gx_pop_depth_stencil_state(pDrawCtx);
    gx_pop_blend_state(pDrawCtx);
    sub_219fcf0(pDrawCtx);

    // Copy color/depth surface via sub_2168660
    uint8_t* pSrcBank = (pVpState[0x297a] != 0 && *reinterpret_cast<uint32_t*>(pVpState + 0x28) >= 2u)
                            ? (pVpState + 0x708)
                            : (pVpState + 0x130);
    pSlot1068 = *reinterpret_cast<uint8_t**>(pVpState + 0x1068);
    void* pCopyDst = *reinterpret_cast<void**>(pSlot1068 + 0x40);
    uint8_t* pSrcSub120 = *reinterpret_cast<uint8_t**>(pSrcBank + 0x120);
    void* pCopySrc = *reinterpret_cast<void**>(pSrcSub120 + 0x40);
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    sub_2168660(pDrawImpl, pDrawImpl + 0xb730, pCopyDst, pCopySrc);

    // Push pDrawImpl hardware states directly onto pDrawCtx state stacks
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    uint8_t rsIdx = pDrawCtx[0x2090];
    reinterpret_cast<void**>(pDrawCtx + 0x1ed0)[rsIdx] =
        *reinterpret_cast<void**>(pDrawImpl + 0xb970);
    pDrawCtx[0x2090] = static_cast<uint8_t>(rsIdx + 1);

    uint8_t dssIdx = pDrawCtx[0x2091];
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    uint8_t* pDssSlot = pDrawCtx + 0x1f10 + (static_cast<size_t>(dssIdx) << 4);
    *reinterpret_cast<void**>(pDssSlot) = *reinterpret_cast<void**>(pDrawImpl + 0xbc08);
    pDssSlot[0x8] = pDrawImpl[0xbcb4];
    pDrawCtx[0x2091] = static_cast<uint8_t>(dssIdx + 1);

    uint8_t bsIdx = pDrawCtx[0x2092];
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    uint8_t* pBsSlot = pDrawCtx + 0x1f90 + (static_cast<size_t>(bsIdx) << 5);
    *reinterpret_cast<void**>(pBsSlot) = *reinterpret_cast<void**>(pDrawImpl + 0xbc00);
    _mm_storeu_ps(
        reinterpret_cast<float*>(pBsSlot + 0x8),
        _mm_loadu_ps(reinterpret_cast<const float*>(pDrawImpl + 0xbc98)));
    *reinterpret_cast<uint32_t*>(pBsSlot + 0x18) =
        *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0);
    pDrawCtx[0x2092] = static_cast<uint8_t>(bsIdx + 1);

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    void* pAlphaBS = *reinterpret_cast<void**>(pDefaultStates + 0x8);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = pAlphaBS;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = 0xffffffffu;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = pAlphaBS;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbca0) = 0;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0xbc98) = 0;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = 0xffffffffu;

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    pFogDSS = *reinterpret_cast<void**>(pGlobalDd8 + 0x65a8);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pFogDSS;
    pDrawCtx[0x218] = 0x02;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pFogDSS;
    pDrawImpl[0xbcb4] = 0x02;
    pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x1f8);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    pSlot1070 = *reinterpret_cast<uint8_t**>(pVpState + 0x1070);
    void* pSrvSlot3 = *reinterpret_cast<void**>(pSlot1070 + 0x48);
    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = pSrvSlot3;
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x8ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = pSrvSlot3;

    // Interpolate fog parameters (sub_2232970 + sub_e9bb60)
    float fogBlendT = *reinterpret_cast<float*>(pVpState + 0x2a70);
    uint64_t fogId0 = *reinterpret_cast<uint64_t*>(pVpState + 0x2a60);
    uint64_t fogId1 = *reinterpret_cast<uint64_t*>(pVpState + 0x2a68);

    void* pFogTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x5958);
    uint8_t* pFogRow0 = static_cast<uint8_t*>(sub_2232970(pFogTable, &fogId0));
    pFogTable = *reinterpret_cast<void**>(pGlobalDd8 + 0x5958);
    uint8_t* pFogRow1 = static_cast<uint8_t*>(sub_2232970(pFogTable, &fogId1));

    void* pHeapAllocator = *reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    if (pHeapAllocator == nullptr) {
        pHeapAllocator = sub_2ba2b10();
        *reinterpret_cast<void**>(GX_VMA(0x5540670ULL)) = pHeapAllocator;
    }

    alignas(16) uint8_t containerCheck[0x10] = {};
    void** pHeapVt = *reinterpret_cast<void***>(pHeapAllocator);
    reinterpret_cast<FnVtableInitContainer>(pHeapVt[4])(containerCheck, pHeapAllocator, 0);
    if ((containerCheck[0] & 0x20u) == 0) {
        sub_20b55b0(0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    struct alignas(16) FogScatteringEvalContext {
        uint8_t header[0x30];
        union {
            void* pStrData;
            uint16_t inlineWstr[4];
        };
        uint64_t pad0;
        uint64_t strLen;
        uint64_t strCap;
        void* pAllocator;
        uint8_t flag;
        uint8_t pad1[0x77];
        float scatterParamD0;
        uint8_t pad2[0x9c];
    } scatterCtx{};

    scatterCtx.pAllocator = pHeapAllocator;
    scatterCtx.strCap = 7;
    scatterCtx.strLen = 0;
    scatterCtx.inlineWstr[0] = 0;
    scatterCtx.flag = 1;

    sub_e9bb60(&scatterCtx, pFogRow0 + 0x10, pFogRow1 + 0x10, fogBlendT);

    pSlot1068 = *reinterpret_cast<uint8_t**>(pVpState + 0x1068);
    void* pTex1068 = *reinterpret_cast<void**>(pSlot1068 + 0x48);
    float scatterVal = scatterCtx.scatterParamD0;
    void* pArg268 = *reinterpret_cast<void**>(pVpState + 0x268);
    void* pArg278 = *reinterpret_cast<void**>(pVpState + 0x278);
    uint32_t passMode = (mode68 == 2u) ? 4u : 3u;

    sub_22740d0(pShaderRepo, pGeomSlot, pTex1068, 0, 0, pArg268, scatterVal, pArg278, 0, passMode, 0);

    // Pop states directly back into pDrawCtx and pDrawImpl, and unbind SRV slot 3
    *reinterpret_cast<void**>(pDrawCtx + 0x7a0) = nullptr;
    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x1968) |= 0x8ULL;
    *reinterpret_cast<void**>(pDrawImpl + 0xd140) = nullptr;

    rsIdx = static_cast<uint8_t>(pDrawCtx[0x2090] - 1);
    pDrawCtx[0x2090] = rsIdx;
    void* poppedRS = reinterpret_cast<void**>(pDrawCtx + 0x1ed0)[rsIdx];
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = poppedRS;
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = poppedRS;

    dssIdx = static_cast<uint8_t>(pDrawCtx[0x2091] - 1);
    pDrawCtx[0x2091] = dssIdx;
    pDssSlot = pDrawCtx + 0x1f10 + (static_cast<size_t>(dssIdx) << 4);
    void* poppedDSS = *reinterpret_cast<void**>(pDssSlot);
    uint8_t poppedStencilRef = pDssSlot[0x8];
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = poppedDSS;
    pDrawCtx[0x218] = poppedStencilRef;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = poppedDSS;
    pDrawImpl[0xbcb4] = poppedStencilRef;

    pDrawCtx[0x1670] &= 0xefu;

    bsIdx = static_cast<uint8_t>(pDrawCtx[0x2092] - 1);
    pDrawCtx[0x2092] = bsIdx;
    pBsSlot = pDrawCtx + 0x1f90 + (static_cast<size_t>(bsIdx) << 5);
    void* poppedBS = *reinterpret_cast<void**>(pBsSlot);
    *reinterpret_cast<void**>(pDrawCtx + 0x480) = poppedBS;
    *reinterpret_cast<void**>(pDrawImpl + 0xbc00) = poppedBS;
    __m128 bfVec = _mm_loadu_ps(reinterpret_cast<const float*>(pBsSlot + 0x8));
    _mm_storeu_ps(reinterpret_cast<float*>(pDrawCtx + 0x488), bfVec);
    _mm_storeu_ps(reinterpret_cast<float*>(pDrawImpl + 0xbc98), bfVec);
    uint32_t poppedSampleMask = *reinterpret_cast<uint32_t*>(pBsSlot + 0x18);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x498) = poppedSampleMask;
    *reinterpret_cast<uint32_t*>(pDrawImpl + 0xbcb0) = poppedSampleMask;

    if (scatterCtx.strCap >= 8 && scatterCtx.pAllocator != nullptr) {
        void** pAllocVt = *reinterpret_cast<void***>(scatterCtx.pAllocator);
        reinterpret_cast<FnVtableFreeContainer>(pAllocVt[14])(
            scatterCtx.pAllocator, scatterCtx.pStrData);
    }
}

// ============================================================================
// 16. sub_22ab7e0 (0x22ab7e0) - 107 bytes
// GXSceneRenderPass_SetupDepthStencilAndInitPass
// ============================================================================
void sub_22ab7e0(void* pPassObj) {
    uint8_t* pPass = static_cast<uint8_t*>(pPassObj);
    void* pSub720 = pPass + 0x720;
    void** pVtable = *reinterpret_cast<void***>(pSub720);
    uint8_t* pDrawCtx = static_cast<uint8_t*>(
        reinterpret_cast<FnVtableRetPtr>(pVtable[2])(pSub720));

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDefaultStates = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);

    void* pDefaultDSS = *reinterpret_cast<void**>(pDefaultStates + 0x1d0);
    *reinterpret_cast<void**>(pDrawCtx + 0x210) = pDefaultDSS;
    pDrawCtx[0x218] = 0xff;
    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x8);
    *reinterpret_cast<void**>(pDrawImpl + 0xbc08) = pDefaultDSS;
    pDrawImpl[0xbcb4] = 0xff;

    void* pDefaultRS = *reinterpret_cast<void**>(pDefaultStates + 0x208);
    *reinterpret_cast<void**>(pDrawCtx + 0x258) = pDefaultRS;
    *reinterpret_cast<void**>(pDrawImpl + 0xb970) = pDefaultRS;

    sub_22b01a0(pPassObj, pDrawCtx);
}

} // extern "C"
