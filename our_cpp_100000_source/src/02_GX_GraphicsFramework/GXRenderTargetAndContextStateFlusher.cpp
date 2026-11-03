// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Module 02: GX_GraphicsFramework
// File: GXRenderTargetAndContextStateFlusher.cpp
// Address Range: 0x219f810 .. 0x21a4fe0 (95 functions, 21,440 bytes)
//
// Contains:
//   1. GXHullShaderResource, GXDomainShaderResource &
//      GXGeometryShaderSimpleResource (0x21a4510 .. 0x21a4fe0, 15 functions):
//      Remaining shader resource wrappers (vtable 0x535f8b0, 0x535f880,
//      0x535f850) managing CG*ShaderGnm* lifecycle and GXResourceMemoryTracker
//      shader telemetry (+0x17C..+0x184).
//   2. GXRenderTargetResource & Base Texture/View Accessors
//      (0x21a2fd0 .. 0x21a4500 & 0x219f810 .. 0x219f920, 36 functions):
//      Render target resource wrapper (vtable 0x53b6940 -> 0x53b6a20 ->
//      0x53b6b00), per-mip CGRenderTargetViewGnm* array management (+0x60),
//      DXGI typeless-to-RTV format remapping (0x21a32f0), fullscreen quad
//      copy/clear helper (0x21a3c80), and memory tracker bucket 2 telemetry.
//   3. GXInputLayoutResource (0x21a2ab0 .. 0x21a2e60, 6 functions):
//      Vertex declaration / input layout wrapper creating CGInputLayoutGnm*
//      from shader bytecode (0x2165650) or CGVertexShaderGnm* (0x21657a0).
//   4. GXDeviceContext State Shadowing, State Stack & Dirty State Flusher
//      (0x219f930 .. 0x21a2a10, 38 functions):
//      - GXDeviceContext_FlushDirtyStateToGnmContext (0x219fcf0, 7,286 B):
//        Flushes dirty RTVs, DSV, UAVs, viewports, scissors, blend/depth/
//        rasterizer states, IA vertex/index buffers, and 6-stage (VS/PS/GS/
//        HS/DS/CS) constant buffers, SRVs, and samplers to CGGfxContextGnm.
//      - Render-pass target/viewport/scissor binders (0x21a19b0 .. 0x21a2650).
//      - 3-type state stacks (Rasterizer +0x2090, DepthStencil +0x2091,
//        Blend +0x2092) and Draw/Dispatch submission wrappers.
// ============================================================================

#include <cstdint>
#include <cstring>

#include "gx_direct_vulkan.h"

namespace Bloodborne {
namespace GX {

struct GXResourceMemoryTracker;
struct GXDeviceContext;

extern "C" {
    void* GXGlobalDevice_GetInstance(); // *(void**)0x55406c8
    void* CGObject_GetCreatorAllocator(void* pObj); // 0x207b720
    void  CGBufferGnm_Release(void* pBuf); // 0x2192660
    void  GXTexture2DResource_ReleaseGpuTexture(void* pTexRes); // 0x21a6a00

    int   GXDevice_CreateInputLayoutFromBytecode(void* pDev, void** outLayout, const void* pElements, uint32_t count, const void* pCode, size_t codeSize, void* a7, void* a8); // 0x2165650
    int   GXDevice_CreateInputLayoutFromShader(void* pDev, void** outLayout, const void* pElements, uint32_t count, void* pGnmVs, void* a6, void* a7); // 0x21657a0
    int   GXDevice_CreateTexture(void* pDev, void** outTex, const void* pDesc, const void* pInitData, void* a5, void* a6, void* a7); // 0x2165f90
    int   GXDevice_CreateRenderTargetView(void* pDev, void** outRtv, void* pTex, const void* pRtvDesc, void* a5); // 0x21667a0
    int   GXDevice_CreateShaderResourceView(void* pDev, void** outSrv, void* pTex, const void* pSrvDesc, void* a5); // 0x21669a0
    int   GXDevice_CreateDomainShader(void* pDev, void** outDs, const void* pCode, size_t size, void* a5, void* a6); // 0x2166f20
    int   GXDevice_CreateGeometryShader(void* pDev, void** outGs, const void* pCode, size_t size, void* a5, void* a6); // 0x21673e0

    void  CGGfxContext_DrawAutoOrInstanced(void* pGfxCtx, void* pState, uint32_t vertCount, uint32_t startVert); // 0x21696c0
    void  CGGfxContext_DrawIndexed(void* pGfxCtx, void* pState, uint32_t idxCount, uint32_t startIdx, int32_t baseVert); // 0x2169780
    void  CGGfxContext_DrawIndexedInstanced(void* pGfxCtx, void* pState, uint32_t idxCountPerInst, uint32_t instCount, uint32_t startIdx, int32_t baseVert); // 0x2169900
    void  CGGfxContext_DrawInstancedIndirect(void* pGfxCtx, void* pState, void* pArgBuf, uint32_t alignedByteOffset); // 0x21699d0

    void  GXResourceMemoryTracker_RegisterRenderTarget(GXResourceMemoryTracker* pTracker, void* pRtRes); // 0x21ac900
    void  CGGfxContext_SetViewportCount(void* pViewportState, uint32_t count); // 0x26d3350
    void  CGGfxContext_SetScissorCount(void* pViewportState, uint32_t count); // 0x26d3360
    int   CGGfxContext_EndCommandRecording(void* pGfxCtx, void* a2, uint32_t flags); // 0x26d5850

    bool  sub_e46f40(void* pTargetHelper1728, void* pRtv0, void* pDsv, uint32_t flags);
    bool  sub_e46fd0(void* pTargetHelper1728, void* pRtv0, void* pRtv1, void* pDsv, uint32_t flags);
    bool  sub_e47290(void* pTargetHelper1728, uint32_t flags);
    void* sub_e47840(void* pTargetHelper1728);
    void  sub_e47880(void* pTargetHelper1728);
    void  sub_10742f0(void* pGnmxGfxCtx, void* pIndirectBaseAddr);
    void  sub_1074c50(void* pGnmxGfxCtx, uint32_t groupX, uint32_t groupY, uint32_t groupZ);
    void  sub_1074de0(void* pGnmxGfxCtx, uint32_t alignedByteOffset);
    void  sub_26af700(void* pGnmxGfxCtx);
    void  sub_26cbe50(void* pCsStageState, void* pGfxCtx, void* pDrawState);
    void  sub_26ccf20(void* pCsStageState, void* pGfxCtx, void* pDrawState);
    void  sub_2bbe588(void* pMem);
}

// ============================================================================
// 1. GXHullShaderResource, GXDomainShaderResource &
//    GXGeometryShaderSimpleResource (0x21a4510 .. 0x21a4fe0)
// ============================================================================

struct CGShaderGnm {
    uint8_t       m_pad00[0x100];
    const uint32_t* m_pShaderBinaryHeader; // +0x100 (low 23 bits = bytecode size)
    void*         m_pGpuShaderBinary;      // +0x108
    void*         m_pHeaderAllocator;      // +0x110
    void*         m_pBinaryAllocator;      // +0x118
};

struct GXShaderResource {
    void**        m_vptr;             // +0x00
    uint32_t      m_refCount;         // +0x08
    uint32_t      m_bytecodeSize;     // +0x0C
    CGShaderGnm*  m_pGnmShader;       // +0x10
};

static inline void GXShaderResource_UnregisterAndFree(
    GXShaderResource* pThis,
    uint32_t (*fnDirectGetSize)(const GXShaderResource*))
{
    if (!pThis->m_pGnmShader) {
        pThis->m_pGnmShader = nullptr;
        return;
    }

    void* pGlobal = GXGlobalDevice_GetInstance();
    if (pGlobal) {
        void** ppTrackerOwner = *reinterpret_cast<void***>(
            reinterpret_cast<uint8_t*>(pGlobal) + 0x650);
        if (ppTrackerOwner && pThis) {
            uint8_t* pTracker = reinterpret_cast<uint8_t*>(*ppTrackerOwner);
            uint32_t byteSize = fnDirectGetSize
                ? fnDirectGetSize(pThis)
                : reinterpret_cast<uint32_t(*)(GXShaderResource*)>(pThis->m_vptr[2])(pThis);
            if (byteSize != 0) {
                uint32_t totalBytes = *reinterpret_cast<uint32_t*>(pTracker + 0x180) - byteSize;
                *reinterpret_cast<uint32_t*>(pTracker + 0x180) = totalBytes;
                *reinterpret_cast<float*>(pTracker + 0x184) =
                    static_cast<float>(static_cast<uint64_t>(totalBytes)) * (1.0f / 1048576.0f);
                *reinterpret_cast<uint32_t*>(pTracker + 0x17C) -= 1;
            }
        }
    }

    CGShaderGnm* pShd = pThis->m_pGnmShader;
    if (pShd) {
        void* pCreatorAlloc = CGObject_GetCreatorAllocator(pShd);

        void* pHdrAlloc = pShd->m_pHeaderAllocator;
        auto fnFreeHdr = *reinterpret_cast<void(**)(void*, const void*)>(
            *reinterpret_cast<uintptr_t*>(pHdrAlloc) + 0x70);
        fnFreeHdr(pHdrAlloc, pShd->m_pShaderBinaryHeader);

        void* pBinAlloc = pShd->m_pBinaryAllocator;
        auto fnFreeBin = *reinterpret_cast<void(**)(void*, void*)>(
            *reinterpret_cast<uintptr_t*>(pBinAlloc) + 0x70);
        fnFreeBin(pBinAlloc, pShd->m_pGpuShaderBinary);

        auto fnFreeObj = *reinterpret_cast<void(**)(void*, void*)>(
            *reinterpret_cast<uintptr_t*>(pCreatorAlloc) + 0x70);
        fnFreeObj(pCreatorAlloc, pShd);
        pThis->m_pGnmShader = nullptr;
    }
    pThis->m_pGnmShader = nullptr;
}

static inline void GXShaderResource_RegisterWithTracker(GXShaderResource* pThis) {
    if (!pThis || !pThis->m_pGnmShader) return;
    void* pGlobal = GXGlobalDevice_GetInstance();
    if (!pGlobal) return;
    void** ppTrackerOwner = *reinterpret_cast<void***>(
        reinterpret_cast<uint8_t*>(pGlobal) + 0x650);
    if (!ppTrackerOwner) return;

    uint8_t* pTracker = reinterpret_cast<uint8_t*>(*ppTrackerOwner);
    auto fnGetSize = reinterpret_cast<uint32_t(*)(GXShaderResource*)>(pThis->m_vptr[2]);
    uint32_t sz = fnGetSize(pThis);
    if (sz != 0) {
        uint32_t totalBytes = *reinterpret_cast<uint32_t*>(pTracker + 0x180) + sz;
        *reinterpret_cast<uint32_t*>(pTracker + 0x180) = totalBytes;
        *reinterpret_cast<float*>(pTracker + 0x184) =
            static_cast<float>(static_cast<uint64_t>(totalBytes)) * (1.0f / 1048576.0f);
        *reinterpret_cast<uint32_t*>(pTracker + 0x17C) += 1;
    }
}

uint32_t GXHullOrDomainShaderResource_GetByteSize(const GXShaderResource* pThis); // 0x21a5390

// 0x21a4eb0 (28 B): GXDomainShaderResource_GetByteSize
uint32_t GXDomainShaderResource_GetByteSize(const GXShaderResource* pThis) {
    if (pThis->m_pGnmShader && pThis->m_pGnmShader->m_pShaderBinaryHeader) {
        return (*pThis->m_pGnmShader->m_pShaderBinaryHeader) & 0x7FFFFFu;
    }
    return pThis->m_bytecodeSize;
}

// 0x21a4fe0 (223 B): GXHullShaderResource_BaseDtor
void GXHullShaderResource_BaseDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f8b0ULL));
    GXShaderResource_UnregisterAndFree(pThis, &GXHullOrDomainShaderResource_GetByteSize);
}

// 0x21a4f00 (214 B): GXHullShaderResource_DeletingDtor
void GXHullShaderResource_DeletingDtor(GXShaderResource* pThis) {
    GXHullShaderResource_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x21a4ed0 (33 B): GXHullShaderResource_Construct
void GXHullShaderResource_Construct(GXShaderResource* pThis) {
    pThis->m_vptr         = reinterpret_cast<void**>(GX_VMA(0x535f8b0ULL));
    pThis->m_refCount     = 0;
    pThis->m_bytecodeSize = 0;
    pThis->m_pGnmShader   = nullptr;
}

// 0x21a4be0 (214 B): GXDomainShaderResource_ReleaseGpuShader
void GXDomainShaderResource_ReleaseGpuShader(GXShaderResource* pThis) {
    GXShaderResource_UnregisterAndFree(pThis, nullptr);
}

// 0x21a4cc0 (495 B): GXDomainShaderResource_CreateFromShaderBinary
bool GXDomainShaderResource_CreateFromShaderBinary(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* a4,
    void* a5)
{
    GXDomainShaderResource_ReleaseGpuShader(pThis);
    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGShaderGnm* pNewDs = nullptr;
    int err = GXDevice_CreateDomainShader(
        pDev, reinterpret_cast<void**>(&pNewDs), pCode, codeSize, a4, a5);
    if (err != 0) return false;
    pThis->m_pGnmShader   = pNewDs;
    pThis->m_bytecodeSize = static_cast<uint32_t>(codeSize);
    GXShaderResource_RegisterWithTracker(pThis);
    return pThis->m_pGnmShader != nullptr;
}

// 0x21a4b00 (223 B): GXDomainShaderResource_BaseDtor
void GXDomainShaderResource_BaseDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f880ULL));
    GXShaderResource_UnregisterAndFree(pThis, &GXDomainShaderResource_GetByteSize);
}

// 0x21a4a20 (214 B): GXDomainShaderResource_DeletingDtor
void GXDomainShaderResource_DeletingDtor(GXShaderResource* pThis) {
    GXDomainShaderResource_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x21a49f0 (33 B): GXDomainShaderResource_Construct
void GXDomainShaderResource_Construct(GXShaderResource* pThis) {
    pThis->m_vptr         = reinterpret_cast<void**>(GX_VMA(0x535f880ULL));
    pThis->m_refCount     = 0;
    pThis->m_bytecodeSize = 0;
    pThis->m_pGnmShader   = nullptr;
}

// 0x21a49d0 (28 B): GXGeometryShaderSimpleResource_GetByteSize
uint32_t GXGeometryShaderSimpleResource_GetByteSize(const GXShaderResource* pThis) {
    return GXDomainShaderResource_GetByteSize(pThis);
}

// 0x21a4700 (214 B): GXGeometryShaderSimpleResource_ReleaseGpuShader
void GXGeometryShaderSimpleResource_ReleaseGpuShader(GXShaderResource* pThis) {
    GXDomainShaderResource_ReleaseGpuShader(pThis);
}

// 0x21a47e0 (495 B): GXGeometryShaderSimpleResource_CreateFromShaderBinary
bool GXGeometryShaderSimpleResource_CreateFromShaderBinary(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* a4,
    void* a5)
{
    GXGeometryShaderSimpleResource_ReleaseGpuShader(pThis);
    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGShaderGnm* pNewGs = nullptr;
    int err = GXDevice_CreateGeometryShader(
        pDev, reinterpret_cast<void**>(&pNewGs), pCode, codeSize, a4, a5);
    if (err != 0) return false;
    pThis->m_pGnmShader   = pNewGs;
    pThis->m_bytecodeSize = static_cast<uint32_t>(codeSize);
    GXShaderResource_RegisterWithTracker(pThis);
    return pThis->m_pGnmShader != nullptr;
}

// 0x21a4620 (223 B): GXGeometryShaderSimpleResource_BaseDtor
void GXGeometryShaderSimpleResource_BaseDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f850ULL));
    GXShaderResource_UnregisterAndFree(pThis, &GXGeometryShaderSimpleResource_GetByteSize);
}

// 0x21a4540 (214 B): GXGeometryShaderSimpleResource_DeletingDtor
void GXGeometryShaderSimpleResource_DeletingDtor(GXShaderResource* pThis) {
    GXGeometryShaderSimpleResource_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x21a4510 (33 B): GXGeometryShaderSimpleResource_Construct
void GXGeometryShaderSimpleResource_Construct(GXShaderResource* pThis) {
    pThis->m_vptr         = reinterpret_cast<void**>(GX_VMA(0x535f850ULL));
    pThis->m_refCount     = 0;
    pThis->m_bytecodeSize = 0;
    pThis->m_pGnmShader   = nullptr;
}

// ============================================================================
// 2. GXRenderTargetResource & Base Texture/View Accessors
//    (0x21a2fd0 .. 0x21a4500 & 0x219f810 .. 0x219f920)
// ============================================================================

struct CGTextureGnmHeader {
    void**        m_vptr;             // +0x00
    void*         m_pAllocator;       // +0x08
    volatile int32_t m_refCount;      // +0x10
    uint8_t       m_pad14[0x18];
    uint32_t      m_byteSize;         // +0x2C
    uint8_t       m_pad30[8];
    uint8_t       m_resourceType;     // +0x38 (3 = Texture2D/RenderTarget)
    uint8_t       m_cpuAccessFlags;   // +0x39
    uint8_t       m_miscFlags;        // +0x3A
    uint8_t       m_bindFlags;        // +0x3B
    uint32_t      m_usageFlags;       // +0x3C
    uint8_t       m_pad40[4];
    uint16_t      m_width;            // +0x44
    uint16_t      m_height;           // +0x46
    uint16_t      m_depthOrArraySize; // +0x48
    uint8_t       m_mipLevels;        // +0x4A
    uint8_t       m_format;           // +0x4B
    uint16_t      m_sampleCount;      // +0x4C
    uint16_t      m_sampleQuality;    // +0x4E
};

struct __attribute__((packed)) GXRenderTargetResource {
    void**              m_vptr;              // +0x00 (0x53b6940)
    uint32_t            m_refCount;          // +0x08
    uint32_t            m_pad0C;             // +0x0C
    float               m_colorScale[4];     // +0x10 (1.0f)
    float               m_clearColor[4];     // +0x20
    void*               m_pColorScaleCBuf;   // +0x30
    uint64_t            m_pad38;             // +0x38
    CGTextureGnmHeader* m_pGnmTexture;       // +0x40
    void*               m_pSrv;              // +0x48
    void*               m_pUav;              // +0x50
    uint32_t            m_byteSize;          // +0x58
    uint32_t            m_pad5C;             // +0x5C
    void**              m_ppRenderTargetViews; // +0x60 (allocated with 16-byte header)
    int32_t             m_unk68;             // +0x68 (-1)
    int64_t             m_unk6C;             // +0x6C (-1)
};

// 0x21a4500 .. 0x21a44c0 (5 x 3 B): GXRenderTargetResource_ReturnZeroStubs
int GXRenderTargetResource_Stub4500() { return 0; }
int GXRenderTargetResource_Stub44f0() { return 0; }
int GXRenderTargetResource_Stub44e0() { return 0; }
int GXRenderTargetResource_Stub44d0() { return 0; }
int GXRenderTargetResource_Stub44c0() { return 0; }

// 0x21a3be0 (65 B): GXRenderTargetResource_GetRenderTargetViewByMip
void* GXRenderTargetResource_GetRenderTargetViewByMip(GXRenderTargetResource* pThis, uint32_t mipIndex) {
    if (!pThis->m_pGnmTexture || !pThis->m_ppRenderTargetViews) return nullptr;
    auto fnGetMipCount = reinterpret_cast<uint32_t(*)(GXRenderTargetResource*)>(pThis->m_vptr[8]); // +0x40
    if (mipIndex >= fnGetMipCount(pThis)) return nullptr;
    return pThis->m_ppRenderTargetViews[mipIndex];
}

// 0x21a3c30 (75 B): GXRenderTargetResource_TryGetRenderTargetViewByMip
bool GXRenderTargetResource_TryGetRenderTargetViewByMip(
    GXRenderTargetResource* pThis,
    uint32_t mipIndex,
    void** outRtv)
{
    if (!pThis->m_ppRenderTargetViews) return false;
    auto fnGetMipCount = reinterpret_cast<uint32_t(*)(GXRenderTargetResource*)>(pThis->m_vptr[8]); // +0x40
    if (mipIndex >= fnGetMipCount(pThis)) return false;
    *outRtv = pThis->m_ppRenderTargetViews[mipIndex];
    return true;
}

// 0x21a38b0 (391 B): GXRenderTargetResource_CreateMipRenderTargetViews
int GXRenderTargetResource_CreateMipRenderTargetViews(
    GXRenderTargetResource* pThis,
    uint32_t rtvFormat,
    const uint32_t* pSampleCountDesc,
    uint32_t mipCount,
    void* pExtraArg)
{
    if (!pThis->m_pGnmTexture) return 5;

    // Allocate [16-byte header + mipCount * sizeof(void*)] from g_pGXDefaultHeapAllocator (0x5540680)
    void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    auto fnAllocAligned = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(
        *reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);
    uint8_t* pRaw = reinterpret_cast<uint8_t*>(fnAllocAligned(pAlloc, mipCount * 8 + 0x10, 0x10));
    if (!pRaw) {
        pThis->m_ppRenderTargetViews = nullptr;
        auto fnReleaseViews = reinterpret_cast<void(*)(GXRenderTargetResource*)>(pThis->m_vptr[18]); // +0x90
        fnReleaseViews(pThis);
        return 9;
    }

    void** ppViews = reinterpret_cast<void**>(pRaw + 0x10);
    for (uint32_t i = 0; i < mipCount; ++i) {
        ppViews[i] = nullptr;
    }
    *reinterpret_cast<void**>(pRaw + 0x00)     = pRaw;
    *reinterpret_cast<uint64_t*>(pRaw + 0x08)  = mipCount;
    pThis->m_ppRenderTargetViews = ppViews;

    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);

    for (uint32_t mip = 0; mip < mipCount; ++mip) {
        uint32_t rtvDesc[5];
        rtvDesc[0] = rtvFormat;
        rtvDesc[1] = (*pSampleCountDesc > 1) ? 6u : 4u; // Texture2DMS vs Texture2D RTV dimension
        rtvDesc[2] = mip;
        rtvDesc[3] = 0;
        rtvDesc[4] = 1;

        void* pRtv = nullptr;
        int err = GXDevice_CreateRenderTargetView(pDev, &pRtv, pThis->m_pGnmTexture, rtvDesc, pExtraArg);
        if (err != 0) {
            auto fnReleaseViews = reinterpret_cast<void(*)(GXRenderTargetResource*)>(pThis->m_vptr[18]);
            fnReleaseViews(pThis);
            return err;
        }
        pThis->m_ppRenderTargetViews[mip] = pRtv;
    }
    return 0;
}

// 0x21a3a40 (408 B): GXRenderTargetResource_InitializeFromExistingTextureAndSrv
bool GXRenderTargetResource_InitializeFromExistingTextureAndSrv(
    GXRenderTargetResource* pThis,
    CGTextureGnmHeader* pTexture,
    void* pExistingSrv)
{
    if (!pTexture) return false;

    auto fnRelease = reinterpret_cast<void(*)(GXRenderTargetResource*)>(pThis->m_vptr[18]); // +0x90
    fnRelease(pThis);

    CGTextureGnmHeader* pOld = pThis->m_pGnmTexture;
    if (pOld != pTexture) {
        if (pOld) {
            if (__sync_fetch_and_add(&pOld->m_refCount, -1) == 1) {
                void* pAlloc = pOld->m_pAllocator;
                auto fnDtor = *reinterpret_cast<void(**)(void*)>(pOld->m_vptr);
                fnDtor(pOld);
                auto fnFree = *reinterpret_cast<void(**)(void*, void*)>(
                    *reinterpret_cast<uintptr_t*>(pAlloc) + 0x70);
                fnFree(pAlloc, pOld);
            }
        }
        pThis->m_pGnmTexture = pTexture;
        __sync_fetch_and_add(&pTexture->m_refCount, 1);
    }

    pThis->m_pSrv      = pExistingSrv;
    pThis->m_byteSize  = pTexture->m_byteSize;
    uint32_t mipLevels = pTexture->m_mipLevels;
    uint32_t format    = pTexture->m_format;
    uint32_t sampleCnt = pTexture->m_sampleCount;

    if (!pExistingSrv && (pTexture->m_bindFlags & 0x08)) {
        if (!pThis->m_pGnmTexture) return false;
        uint32_t srvDesc[6];
        srvDesc[0] = format;
        srvDesc[1] = (sampleCnt > 1) ? 6u : 4u;
        srvDesc[2] = 0;
        srvDesc[3] = mipLevels;
        srvDesc[4] = 0;
        srvDesc[5] = 1;

        void* pGlobal = GXGlobalDevice_GetInstance();
        void* pDev = *reinterpret_cast<void**>(pGlobal);
        if (GXDevice_CreateShaderResourceView(pDev, &pThis->m_pSrv, pThis->m_pGnmTexture, srvDesc, nullptr) != 0) {
            return false;
        }
    }

    return GXRenderTargetResource_CreateMipRenderTargetViews(
        pThis, format, &sampleCnt, mipLevels, nullptr) == 0;
}

// 0x21a32f0 (1460 B): GXRenderTargetResource_Create
bool GXRenderTargetResource_Create(
    GXRenderTargetResource* pThis,
    uint32_t width,
    uint32_t height,
    uint32_t format,
    uint32_t mipLevels,
    const uint64_t* pSampleDesc,
    uint32_t miscFlags,
    void* pExtra1,
    void* pExtra2)
{
    auto fnRelease = reinterpret_cast<void(*)(GXRenderTargetResource*)>(pThis->m_vptr[18]); // +0x90
    fnRelease(pThis);

    // Remap ONLY the 7 sRGB DXGI formats to their typeless base format (0x21a3343..0x21a33db);
    // all non-sRGB formats keep texFormat == format unchanged.
    uint32_t texFormat = format;
    switch (format) {
        case 0x1D: texFormat = 0x1B; break; // R8G8B8A8_UNORM_SRGB -> R8G8B8A8_TYPELESS
        case 0x48: texFormat = 0x46; break; // BC1_UNORM_SRGB      -> BC1_TYPELESS
        case 0x4B: texFormat = 0x49; break; // BC2_UNORM_SRGB      -> BC2_TYPELESS
        case 0x4E: texFormat = 0x4C; break; // BC3_UNORM_SRGB      -> BC3_TYPELESS
        case 0x5B: texFormat = 0x5A; break; // B8G8R8A8_UNORM_SRGB -> B8G8R8A8_TYPELESS
        case 0x5D: texFormat = 0x5C; break; // B8G8R8X8_UNORM_SRGB -> B8G8R8X8_TYPELESS
        case 0x63: texFormat = 0x61; break; // BC7_UNORM_SRGB      -> BC7_TYPELESS
        default: break;
    }

    // Reject depth-only formats (bitmask 0x80210000100000ULL: bits 20, 40, 45, 55)
    if (format <= 0x37 && ((0x80210000100000ULL >> format) & 1ULL)) {
        return false;
    }

    struct __attribute__((packed)) TextureDesc {
        uint32_t width;        // +0x00
        uint32_t height;       // +0x04
        uint32_t mipLevels;    // +0x08
        uint32_t arraySize;    // +0x0C (1)
        uint32_t format;       // +0x10
        uint64_t sampleDesc;   // +0x14
        uint32_t usage;        // +0x1C (0)
        uint32_t bindFlags;    // +0x20 (0x28 = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET)
        uint32_t cpuAccess;    // +0x24 (0)
        uint32_t miscFlags;    // +0x28
    } desc{};

    desc.width      = width;
    desc.height     = height;
    desc.mipLevels  = mipLevels;
    desc.arraySize  = 1;
    desc.format     = texFormat;
    desc.sampleDesc = *pSampleDesc;
    desc.usage      = 0;
    desc.bindFlags  = 0x28;
    desc.cpuAccess  = 0;
    desc.miscFlags  = miscFlags;

    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGTextureGnmHeader* pNewTex = nullptr;
    int err = GXDevice_CreateTexture(
        pDev, reinterpret_cast<void**>(&pNewTex), &desc, nullptr, pExtra1, pExtra2, nullptr);

    CGTextureGnmHeader* pOld = pThis->m_pGnmTexture;
    if (pOld != pNewTex) {
        if (pOld && __sync_fetch_and_add(&pOld->m_refCount, -1) == 1) {
            void* pAlloc = pOld->m_pAllocator;
            (*reinterpret_cast<void(**)(void*)>(pOld->m_vptr))(pOld);
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, pOld);
        }
        pThis->m_pGnmTexture = pNewTex;
        if (pNewTex) __sync_fetch_and_add(&pNewTex->m_refCount, 1);
    }

    if (err != 0) return false;

    if (pNewTex && __sync_fetch_and_add(&pNewTex->m_refCount, -1) == 1) {
        void* pAlloc = pNewTex->m_pAllocator;
        (*reinterpret_cast<void(**)(void*)>(pNewTex->m_vptr))(pNewTex);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, pNewTex);
    }

    pThis->m_byteSize = pNewTex ? pNewTex->m_byteSize : 0;

    uint32_t actualMips = mipLevels;
    if (actualMips == 0) {
        actualMips = pNewTex ? pNewTex->m_mipLevels : 0;
        if (actualMips == 0 && width > 0 && height > 0) {
            uint32_t w = width, h = height;
            while (w > 0 && h > 0) {
                ++actualMips;
                w >>= 1;
                if (w == 0) break;
                h >>= 1;
            }
        }
    }

    GXRenderTargetResource_CreateMipRenderTargetViews(
        pThis, format, reinterpret_cast<const uint32_t*>(pSampleDesc), actualMips, pExtra1);

    pThis->m_pSrv = nullptr;
    int srvErr = 5;
    if (pThis->m_pGnmTexture) {
        uint32_t srvDesc[6];
        srvDesc[0] = format;
        srvDesc[1] = (*reinterpret_cast<const uint32_t*>(pSampleDesc) > 1) ? 6u : 4u;
        srvDesc[2] = 0;
        srvDesc[3] = actualMips;
        srvDesc[4] = 0;
        srvDesc[5] = 1;
        srvErr = GXDevice_CreateShaderResourceView(pDev, &pThis->m_pSrv, pThis->m_pGnmTexture, srvDesc, pExtra1);
    }

    if (pGlobal) {
        void** ppTrackerOwner = *reinterpret_cast<void***>(
            reinterpret_cast<uint8_t*>(pGlobal) + 0x650);
        if (ppTrackerOwner) {
            GXResourceMemoryTracker_RegisterRenderTarget(
                reinterpret_cast<GXResourceMemoryTracker*>(*ppTrackerOwner), pThis);
        }
    }
    return srvErr == 0;
}

// 0x21a32c0 (44 B): GXRenderTargetResource_CreateDefaultFlags
bool GXRenderTargetResource_CreateDefaultFlags(
    GXRenderTargetResource* pThis,
    uint32_t width,
    uint32_t height,
    uint32_t format,
    uint32_t mipLevels,
    const uint64_t* pSampleDesc,
    void* pExtra1,
    void* pExtra2)
{
    return GXRenderTargetResource_Create(
        pThis, width, height, format, mipLevels, pSampleDesc, 0, pExtra1, pExtra2);
}

// 0x21a3180 (313 B): GXRenderTargetResource_ReleaseViewsAndTexture
void GXRenderTargetResource_ReleaseViewsAndTexture(GXRenderTargetResource* pThis) {
    void* pGlobal = GXGlobalDevice_GetInstance();
    if (pGlobal) {
        void** ppTrackerOwner = *reinterpret_cast<void***>(
            reinterpret_cast<uint8_t*>(pGlobal) + 0x650);
        if (ppTrackerOwner) {
            uint8_t* pTracker = reinterpret_cast<uint8_t*>(*ppTrackerOwner);
            void** pBegin = *reinterpret_cast<void***>(pTracker + 0x78);
            void** pEnd   = *reinterpret_cast<void***>(pTracker + 0x80);
            for (void** it = pBegin; it != pEnd; ++it) {
                if (*it == pThis) {
                    std::memmove(it, it + 1, reinterpret_cast<uint8_t*>(pEnd) - reinterpret_cast<uint8_t*>(it + 1));
                    *reinterpret_cast<void***>(pTracker + 0x80) = pEnd - 1;
                    auto fnGetByteSize = reinterpret_cast<uint32_t(*)(GXRenderTargetResource*)>(pThis->m_vptr[12]); // +0x60
                    uint32_t sz = fnGetByteSize(pThis);
                    uint32_t totalBytes = *reinterpret_cast<uint32_t*>(pTracker + 0x9C) - sz;
                    *reinterpret_cast<uint32_t*>(pTracker + 0x9C) = totalBytes;
                    *reinterpret_cast<float*>(pTracker + 0xA0) =
                        static_cast<float>(totalBytes) * (1.0f / 1048576.0f);
                    *reinterpret_cast<uint32_t*>(pTracker + 0x98) -= 1;
                    break;
                }
            }
        }
    }

    if (pThis->m_ppRenderTargetViews) {
        uint64_t allocCount = *reinterpret_cast<uint64_t*>(pThis->m_ppRenderTargetViews - 1);
        uint32_t mipCount = static_cast<uint32_t>(allocCount);
        for (uint32_t i = 0; i < mipCount; ++i) {
            void* pRtv = pThis->m_ppRenderTargetViews[i];
            if (pRtv) {
                void* pCreatorAlloc = CGObject_GetCreatorAllocator(pRtv);
                (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pRtv)))(pRtv);
                (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pCreatorAlloc) + 0x70))(pCreatorAlloc, pRtv);
                pThis->m_ppRenderTargetViews[i] = nullptr;
            }
        }
        if (pThis->m_ppRenderTargetViews) {
            void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
            void* pRawBlock = *(pThis->m_ppRenderTargetViews - 2);
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, pRawBlock);
            pThis->m_ppRenderTargetViews = nullptr;
        }
    }

    GXTexture2DResource_ReleaseGpuTexture(pThis);
}

// 0x21a30e0 (146 B): GXRenderTargetResource_BaseDtor
void GXRenderTargetResource_BaseDtor(GXRenderTargetResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6940ULL));
    GXRenderTargetResource_ReleaseViewsAndTexture(pThis);
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6a20ULL));
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    if (pThis->m_pColorScaleCBuf) {
        CGBufferGnm_Release(pThis->m_pColorScaleCBuf);
        pThis->m_pColorScaleCBuf = nullptr;
    }
}

// 0x21a3040 (145 B): GXRenderTargetResource_DeletingDtor
void GXRenderTargetResource_DeletingDtor(GXRenderTargetResource* pThis) {
    GXRenderTargetResource_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x21a2fd0 (100 B): GXRenderTargetResource_Construct
void GXRenderTargetResource_Construct(GXRenderTargetResource* pThis) {
    pThis->m_refCount            = 0;
    pThis->m_clearColor[0]       = 1.0f; // 0x4920810 = {1.0f, 1.0f, 1.0f, 1.0f} (m_rgbaColorScale at +0x20)
    pThis->m_clearColor[1]       = 1.0f;
    pThis->m_clearColor[2]       = 1.0f;
    pThis->m_clearColor[3]       = 1.0f;
    pThis->m_pColorScaleCBuf     = nullptr;
    pThis->m_byteSize            = 0;
    pThis->m_pUav                = nullptr;
    pThis->m_pSrv                = nullptr;
    pThis->m_pGnmTexture         = nullptr;
    pThis->m_colorScale[0]       = 1.0f; // movl $0x3f800000, 0x10(%rdi)
    pThis->m_vptr                = reinterpret_cast<void**>(GX_VMA(0x53b6940ULL));
    pThis->m_ppRenderTargetViews = nullptr;
    pThis->m_unk68               = -1;
    pThis->m_unk6C               = -1;
}

// Base Texture/View Accessor Virtual Methods (0x219f810 .. 0x219f920)
// 0x219f920 .. 0x219f8d0 (6 x 3 B): Return-zero virtual stubs
int GXTextureResource_Stub920() { return 0; }
int GXTextureResource_Stub910() { return 0; }
int GXTextureResource_Stub900() { return 0; }
int GXTextureResource_Stub8f0() { return 0; }
int GXTextureResource_Stub8e0() { return 0; }
int GXTextureResource_Stub8d0() { return 0; }

// 0x219f8b0 (28 B): GXTextureResource_IsInitializedAndValid
bool GXTextureResource_IsInitializedAndValid(GXRenderTargetResource* pThis) {
    if (!pThis->m_pGnmTexture) return false;
    auto fnCheck = reinterpret_cast<int(*)(GXRenderTargetResource*)>(pThis->m_vptr[7]); // +0x38
    return fnCheck(pThis) != 0;
}

// 0x219f8a0 (4 B): GXTextureResource_GetByteSize
uint32_t GXTextureResource_GetByteSize(const GXRenderTargetResource* pThis) {
    return pThis->m_byteSize;
}

// 0x219f890 (6 B): GXTextureResource_GetArrayOrSliceCount
uint32_t GXTextureResource_GetArrayOrSliceCount() {
    return 1;
}

// 0x219f880 (14 B): GXTextureResource_GetMipHeight
uint32_t GXTextureResource_GetMipHeight(const GXRenderTargetResource* pThis, uint8_t mipLevel) {
    return static_cast<uint32_t>(pThis->m_pGnmTexture->m_height) >> mipLevel;
}

// 0x219f870 (14 B): GXTextureResource_GetMipWidth
uint32_t GXTextureResource_GetMipWidth(const GXRenderTargetResource* pThis, uint8_t mipLevel) {
    return static_cast<uint32_t>(pThis->m_pGnmTexture->m_width) >> mipLevel;
}

// 0x219f860 (9 B): GXTextureResource_GetMipLevels
uint32_t GXTextureResource_GetMipLevels(const GXRenderTargetResource* pThis) {
    return pThis->m_pGnmTexture->m_mipLevels;
}

// 0x219f850 (16 B): GXTextureResource_GetFormat
uint32_t GXTextureResource_GetFormat(const GXRenderTargetResource* pThis) {
    return pThis->m_pGnmTexture ? pThis->m_pGnmTexture->m_format : 0;
}

// 0x219f840 (3 B): GXTextureResource_GetZeroStub
uint32_t GXTextureResource_GetZeroStub() {
    return 0;
}

// 0x219f830 (5 B): GXTextureResource_GetGnmTexture
CGTextureGnmHeader* GXTextureResource_GetGnmTexture(const GXRenderTargetResource* pThis) {
    return pThis->m_pGnmTexture;
}

// 0x219f820 (5 B): GXTextureResource_GetUavOrDsv
void* GXTextureResource_GetUavOrDsv(const GXRenderTargetResource* pThis) {
    return pThis->m_pUav;
}

// 0x219f810 (5 B): GXTextureResource_GetShaderResourceView
void* GXTextureResource_GetShaderResourceView(const GXRenderTargetResource* pThis) {
    return pThis->m_pSrv;
}

// ============================================================================
// 3. GXInputLayoutResource (0x21a2ab0 .. 0x21a2e60, 6 functions)
// ============================================================================

struct CGInputLayoutGnm {
    void* m_pElementArrayAlloc; // +0x00
    void* m_pFetchShaderAlloc;  // +0x08
    void* m_pFetchShaderBinary; // +0x10
    void* m_pElementArray;      // +0x18
};

struct GXInputLayoutResource {
    CGInputLayoutGnm* m_pGnmInputLayout; // +0x00
};

static inline void GXInputLayout_FreeGnmLayout(CGInputLayoutGnm* pLayout) {
    if (!pLayout) return;
    void* pCreatorAlloc = CGObject_GetCreatorAllocator(pLayout);
    if (pLayout->m_pElementArray) {
        void* pElemAlloc = pLayout->m_pElementArrayAlloc;
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pElemAlloc) + 0x70))(
            pElemAlloc, pLayout->m_pElementArray);
        pLayout->m_pElementArray = nullptr;
    }
    if (pLayout->m_pFetchShaderBinary) {
        void* pFsAlloc = pLayout->m_pFetchShaderAlloc;
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pFsAlloc) + 0x70))(
            pFsAlloc, pLayout->m_pFetchShaderBinary);
        pLayout->m_pFetchShaderBinary = nullptr;
    }
    (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pCreatorAlloc) + 0x70))(
        pCreatorAlloc, pLayout);
}

// 0x21a2ab0 (8 B): GXInputLayoutResource_Construct
void GXInputLayoutResource_Construct(GXInputLayoutResource* pThis) {
    pThis->m_pGnmInputLayout = nullptr;
}

// 0x21a2ac0 (122 B): GXInputLayoutResource_Destroy
void GXInputLayoutResource_Destroy(GXInputLayoutResource* pThis) {
    GXInputLayout_FreeGnmLayout(pThis->m_pGnmInputLayout);
    pThis->m_pGnmInputLayout = nullptr;
}

// 0x21a2b40 (122 B): GXInputLayoutResource_Release
void GXInputLayoutResource_Release(GXInputLayoutResource* pThis) {
    GXInputLayout_FreeGnmLayout(pThis->m_pGnmInputLayout);
    pThis->m_pGnmInputLayout = nullptr;
}

// 0x21a2bc0 (278 B): GXInputLayoutResource_CreateFromBytecode
bool GXInputLayoutResource_CreateFromBytecode(
    GXInputLayoutResource* pThis,
    const void* pElements,
    uint32_t elementCount,
    const void* pShaderBytecode,
    size_t bytecodeLength,
    void* pExtra1,
    void* pExtra2)
{
    GXInputLayout_FreeGnmLayout(pThis->m_pGnmInputLayout);
    pThis->m_pGnmInputLayout = nullptr;

    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGInputLayoutGnm* pNewLayout = nullptr;
    int err = GXDevice_CreateInputLayoutFromBytecode(
        pDev,
        reinterpret_cast<void**>(&pNewLayout),
        pElements,
        elementCount,
        pShaderBytecode,
        bytecodeLength,
        pExtra1,
        pExtra2);
    pThis->m_pGnmInputLayout = pNewLayout;
    return err == 0;
}

// 0x21a2ce0 (377 B): GXInputLayoutResource_CreateFromVertexShaderResource
bool GXInputLayoutResource_CreateFromVertexShaderResource(
    GXInputLayoutResource* pThis,
    const void* pElements,
    uint32_t elementCount,
    void* pVertexShaderRes,
    void* pExtra1,
    void* pExtra2)
{
    GXInputLayout_FreeGnmLayout(pThis->m_pGnmInputLayout);
    pThis->m_pGnmInputLayout = nullptr;
    if (!pVertexShaderRes) return false;

    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    void** vtbl = *reinterpret_cast<void***>(pVertexShaderRes);

    auto fnGetCpuCode = reinterpret_cast<const void*(*)(void*)>(vtbl[4]); // +0x20
    auto fnGetCpuSize = reinterpret_cast<size_t(*)(void*)>(vtbl[5]);      // +0x28
    auto fnGetGnmVs   = reinterpret_cast<void*(*)(void*)>(vtbl[3]);       // +0x18

    CGInputLayoutGnm* pNewLayout = nullptr;
    int err = 8;
    bool useCpuBytecode = (*reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac87ULL)) == 0) ||
                          (fnGetCpuCode(pVertexShaderRes) != nullptr) ||
                          (fnGetCpuSize(pVertexShaderRes) != 0);
    if (useCpuBytecode) {
        const void* pCode = fnGetCpuCode(pVertexShaderRes);
        size_t sz = fnGetCpuSize(pVertexShaderRes);
        err = GXDevice_CreateInputLayoutFromBytecode(
            pDev, reinterpret_cast<void**>(&pNewLayout), pElements, elementCount, pCode, sz, pExtra1, pExtra2);
    } else {
        void* pGnmVs = fnGetGnmVs(pVertexShaderRes);
        err = GXDevice_CreateInputLayoutFromShader(
            pDev, reinterpret_cast<void**>(&pNewLayout), pElements, elementCount, pGnmVs, pExtra1, pExtra2);
    }
    pThis->m_pGnmInputLayout = pNewLayout;
    return err == 0;
}

// 0x21a2e60 (211 B): GXDevice_CreateInputLayoutFromVertexShaderHelper
int GXDevice_CreateInputLayoutFromVertexShaderHelper(
    void** outLayout,
    void* pDev,
    const void* pElements,
    uint32_t elementCount,
    void* pVertexShaderRes,
    void* pExtra1,
    void* pExtra2)
{
    if (!pVertexShaderRes) return 8;
    void** vtbl = *reinterpret_cast<void***>(pVertexShaderRes);
    auto fnGetCpuCode = reinterpret_cast<const void*(*)(void*)>(vtbl[4]);
    auto fnGetCpuSize = reinterpret_cast<size_t(*)(void*)>(vtbl[5]);
    auto fnGetGnmVs   = reinterpret_cast<void*(*)(void*)>(vtbl[3]);

    bool useCpuBytecode = (*reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac87ULL)) == 0) ||
                          (fnGetCpuCode(pVertexShaderRes) != nullptr) ||
                          (fnGetCpuSize(pVertexShaderRes) != 0);
    if (useCpuBytecode) {
        const void* pCode = fnGetCpuCode(pVertexShaderRes);
        size_t sz = fnGetCpuSize(pVertexShaderRes);
        return GXDevice_CreateInputLayoutFromBytecode(
            pDev, outLayout, pElements, elementCount, pCode, sz, pExtra1, pExtra2);
    }
    return GXDevice_CreateInputLayoutFromShader(
        pDev, outLayout, pElements, elementCount, fnGetGnmVs(pVertexShaderRes), pExtra1, pExtra2);
}

// ============================================================================
// 4. GXDeviceContext State Shadowing, State Stack & Dirty State Flusher
//    (0x219f930 .. 0x21a2a10 & 0x21a3c80)
// ============================================================================

// Forward declaration of the main state flusher
void GXDeviceContext_FlushDirtyStateToGnmContext(uint8_t* pCtx); // 0x219fcf0

// 0x21a2a10 (1 B): GXDeviceContext_NoOpStub
void GXDeviceContext_NoOpStub() {}

// 0x21a2760 (688 B): GXDeviceContext_MarkDirtyStateCategories
// Marks dirty bits in `pCtx + 0x1670..0x1724` and resets per-stage slot ranges
// `[minSlot=0, maxSlot=count]` based on `categoryMask`:
//   0x001 = OutputMerger (RTV/DSV/Blend/DepthStencil)
//   0x002 = Rasterizer / Viewport / Scissor
//   0x004 = VertexShader stage (59 SRVs, 14 CBs, 16 Samplers)
//   0x008 = PixelShader stage
//   0x010 = GeometryShader stage
//   0x020 = DomainShader stage
//   0x040 = ComputeShader stage (+ 8 CS UAVs)
//   0x080 = HullShader stage
//   0x200 = InputAssembler (VertexBuffers, IndexBuffer, InputLayout, Topology)
void GXDeviceContext_MarkDirtyStateCategories(uint8_t* pCtx, uint32_t categoryMask) {
    if (categoryMask & 0x001) {
        pCtx[0x1670] |= 0x51;
    }
    if (categoryMask & 0x002) {
        pCtx[0x1670] |= 0x2C;
    }
    if (categoryMask & 0x004) {
        *reinterpret_cast<uint64_t*>(pCtx + 0x1670) |= 0x780ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16A0) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1698) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1690) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0678) = 0x3B00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x06F0) = 0x0E00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0778) = 0x1000000000ULL;
    }
    if (categoryMask & 0x008) {
        pCtx[0x1671] |= 0x78;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16B8) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16B0) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16A8) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0960) = 0x3B00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x09D8) = 0x0E00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0A60) = 0x1000000000ULL;
    }
    if (categoryMask & 0x010) {
        *reinterpret_cast<uint64_t*>(pCtx + 0x1670) |= 0x78000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16D0) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16C8) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16C0) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0C48) = 0x3B00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0CC0) = 0x0E00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0D48) = 0x1000000000ULL;
    }
    if (categoryMask & 0x080) {
        pCtx[0x1672] |= 0x78;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16E8) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16E0) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16D8) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0F30) = 0x3B00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x0FA8) = 0x0E00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1030) = 0x1000000000ULL;
    }
    if (categoryMask & 0x020) {
        *reinterpret_cast<uint64_t*>(pCtx + 0x1670) |= 0x7800000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1700) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16F8) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x16F0) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1218) = 0x3B00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1290) = 0x0E00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1318) = 0x1000000000ULL;
    }
    if (categoryMask & 0x040) {
        pCtx[0x1673] |= 0xF8;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1720) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1718) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1710) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1708) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1500) = 0x3B00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1578) = 0x0E00000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1600) = 0x1000000000ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1688) = 0x0800000000ULL;
    }
    if (categoryMask & 0x200) {
        pCtx[0x1674] |= 0x1E;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1678) = ~0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1680) = 0x2000000000ULL;
    }
}

// 0x21a2720 (55 B): GXDeviceContext_CopyShadowStateAndDirtyMasks
void GXDeviceContext_CopyShadowStateAndDirtyMasks(
    uint8_t* pDstCtx,
    const void* pSrcShadowState,
    const void* pSrcDirtyMasks)
{
    std::memcpy(pDstCtx + 0x10, pSrcShadowState, 0x1660);
    std::memcpy(pDstCtx + 0x1670, pSrcDirtyMasks, 0xB8);
}

// Helper for resetting dirty bitmasks after a Draw/Dispatch call
static inline void GXDeviceContext_ResetDirtyMasksAfterDraw(uint8_t* pCtx) {
    std::memset(pCtx + 0x1670, 0, 0xB8);
    *reinterpret_cast<uint64_t*>(pCtx + 0x1680) = 0x20;
    *reinterpret_cast<uint64_t*>(pCtx + 0x1688) = 0x08;
}

// 0x21a2520 (92 B): GXDeviceContext_DrawInstancedIndirect
void GXDeviceContext_DrawInstancedIndirect(uint8_t* pCtx, void* pArgBuffer, uint32_t alignedByteOffset) {
    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_DrawInstancedIndirect(pGnmCtx, pGnmCtx + 0xB730, pArgBuffer, alignedByteOffset);
}

// 0x21a24a0 (113 B): GXDeviceContext_DrawIndexedInstanced
void GXDeviceContext_DrawIndexedInstanced(
    uint8_t* pCtx,
    uint32_t indexCountPerInstance,
    uint32_t instanceCount,
    uint32_t startIndexLocation,
    int32_t baseVertexLocation)
{
    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_DrawIndexedInstanced(
        pGnmCtx, pGnmCtx + 0xB730, indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation);
}

// 0x21a2430 (97 B): GXDeviceContext_DrawIndexed
void GXDeviceContext_DrawIndexed(
    uint8_t* pCtx,
    uint32_t indexCount,
    uint32_t startIndexLocation,
    int32_t baseVertexLocation)
{
    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_DrawIndexed(pGnmCtx, pGnmCtx + 0xB730, indexCount, startIndexLocation, baseVertexLocation);
}

// 0x21a23d0 (92 B): GXDeviceContext_Draw
void GXDeviceContext_Draw(uint8_t* pCtx, uint32_t vertexCount, uint32_t startVertexLocation) {
    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_DrawAutoOrInstanced(pGnmCtx, pGnmCtx + 0xB730, vertexCount, startVertexLocation);
}

// 0x21a2320 (164 B): GXDeviceContext_DispatchIndirect
void GXDeviceContext_DispatchIndirect(uint8_t* pCtx, void* pArgBuffer, uint32_t alignedByteOffset) {
    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
    uint8_t* pGnmCtx    = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    void*    pDrawState = pGnmCtx + 0xB730;
    void*    pGnmxCtx   = pGnmCtx + 0x10;
    void*    pCsStage   = pGnmCtx + 0x50;

    sub_26af700(pGnmxCtx);
    sub_26cbe50(pCsStage, pGnmCtx, pDrawState);
    void* pBaseAddr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pArgBuffer) + 0x20);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_Dispatch)) {
        gx_direct_vk::EmitDispatchIndirect(pGnmxCtx, reinterpret_cast<uint64_t>(pBaseAddr),
                                           alignedByteOffset);
    } else {
        sub_10742f0(pGnmxCtx, pBaseAddr);
        sub_1074de0(pGnmxCtx, alignedByteOffset);
    }
    sub_26ccf20(pCsStage, pGnmCtx, pDrawState);
    sub_26af700(pGnmxCtx);
}

// 0x21a2270 (161 B): GXDeviceContext_Dispatch
void GXDeviceContext_Dispatch(
    uint8_t* pCtx,
    uint32_t threadGroupCountX,
    uint32_t threadGroupCountY,
    uint32_t threadGroupCountZ)
{
    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
    uint8_t* pGnmCtx    = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    void*    pDrawState = pGnmCtx + 0xB730;
    void*    pGnmxCtx   = pGnmCtx + 0x10;
    void*    pCsStage   = pGnmCtx + 0x50;

    sub_26af700(pGnmxCtx);
    sub_26cbe50(pCsStage, pGnmCtx, pDrawState);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_Dispatch)) {
        gx_direct_vk::EmitDispatchDirect(pGnmxCtx, threadGroupCountX, threadGroupCountY,
                                         threadGroupCountZ);
    } else {
        sub_1074c50(pGnmxCtx, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }
    sub_26ccf20(pCsStage, pGnmCtx, pDrawState);
    sub_26af700(pGnmxCtx);
}

// ----------------------------------------------------------------------------
// State Stacks (0x21a1f70 .. 0x21a2240, 12 functions):
//   - RasterizerState stack:   index at `+0x2090`, 8-byte entries at `+0x1ED0`
//   - DepthStencilState stack: index at `+0x2091`, 16-byte entries at `+0x1F10`
//   - BlendState stack:        index at `+0x2092`, 32-byte entries at `+0x1F90`
// ----------------------------------------------------------------------------

// 0x21a2240 (44 B): GXDeviceContext_PopRasterizerStateBoth
void GXDeviceContext_PopRasterizerStateBoth(uint8_t* pCtx) {
    uint8_t idx = (pCtx[0x2090] > 0) ? --pCtx[0x2090] : 0;
    idx &= 7u;
    void* pRs = *reinterpret_cast<void**>(pCtx + 0x1ED0 + static_cast<size_t>(idx) * 8);
    *reinterpret_cast<void**>(pCtx + 0x258) = pRs;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xB970) = pRs;
}

// 0x21a2220 (29 B): GXDeviceContext_PushRasterizerStateFromShadow
void GXDeviceContext_PushRasterizerStateFromShadow(uint8_t* pCtx) {
    uint8_t idx = pCtx[0x2090];
    if (idx < 8) pCtx[0x2090] = idx + 1;
    idx &= 7u;
    *reinterpret_cast<void**>(pCtx + 0x1ED0 + static_cast<size_t>(idx) * 8) =
        *reinterpret_cast<void**>(pCtx + 0x258);
}

// 0x21a21f0 (37 B): GXDeviceContext_PopRasterizerStateToGnmOnly
void GXDeviceContext_PopRasterizerStateToGnmOnly(uint8_t* pCtx) {
    uint8_t idx = (pCtx[0x2090] > 0) ? --pCtx[0x2090] : 0;
    idx &= 7u;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xB970) =
        *reinterpret_cast<void**>(pCtx + 0x1ED0 + static_cast<size_t>(idx) * 8);
}

// 0x21a21c0 (33 B): GXDeviceContext_PushRasterizerStateFromGnm
void GXDeviceContext_PushRasterizerStateFromGnm(uint8_t* pCtx) {
    uint8_t idx = pCtx[0x2090];
    if (idx < 8) pCtx[0x2090] = idx + 1;
    idx &= 7u;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pCtx + 0x1ED0 + static_cast<size_t>(idx) * 8) =
        *reinterpret_cast<void**>(pGnmCtx + 0xB970);
}

// 0x21a2170 (67 B): GXDeviceContext_PopDepthStencilStateBoth
void GXDeviceContext_PopDepthStencilStateBoth(uint8_t* pCtx) {
    uint8_t idx = (pCtx[0x2091] > 0) ? --pCtx[0x2091] : 0;
    size_t off = static_cast<size_t>(idx & 7u) * 16;
    void* pDss = *reinterpret_cast<void**>(pCtx + 0x1F10 + off);
    uint8_t stencilRef = pCtx[0x1F18 + off];
    *reinterpret_cast<void**>(pCtx + 0x210) = pDss;
    pCtx[0x218] = stencilRef;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xBC08) = pDss;
    pGnmCtx[0xBCB4] = stencilRef;
}

// 0x21a2140 (46 B): GXDeviceContext_PushDepthStencilStateFromShadow
void GXDeviceContext_PushDepthStencilStateFromShadow(uint8_t* pCtx) {
    uint8_t idx = pCtx[0x2091];
    if (idx < 8) pCtx[0x2091] = idx + 1;
    size_t off = static_cast<size_t>(idx & 7u) * 16;
    *reinterpret_cast<void**>(pCtx + 0x1F10 + off) = *reinterpret_cast<void**>(pCtx + 0x210);
    pCtx[0x1F18 + off] = pCtx[0x218];
}

// 0x21a2100 (61 B): GXDeviceContext_PopDepthStencilStateToGnmOnly
void GXDeviceContext_PopDepthStencilStateToGnmOnly(uint8_t* pCtx) {
    uint8_t idx = (pCtx[0x2091] > 0) ? --pCtx[0x2091] : 0;
    size_t off = static_cast<size_t>(idx & 7u) * 16;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xBC08) = *reinterpret_cast<void**>(pCtx + 0x1F10 + off);
    pGnmCtx[0xBCB4] = pCtx[0x1F18 + off];
    pCtx[0x1670] &= ~0x10u;
}

// 0x21a20c0 (50 B): GXDeviceContext_PushDepthStencilStateFromGnm
void GXDeviceContext_PushDepthStencilStateFromGnm(uint8_t* pCtx) {
    uint8_t idx = pCtx[0x2091];
    if (idx < 8) pCtx[0x2091] = idx + 1;
    size_t off = static_cast<size_t>(idx & 7u) * 16;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pCtx + 0x1F10 + off) = *reinterpret_cast<void**>(pGnmCtx + 0xBC08);
    pCtx[0x1F18 + off] = pGnmCtx[0xBCB4];
}

// 0x21a2050 (101 B): GXDeviceContext_PopBlendStateBoth
void GXDeviceContext_PopBlendStateBoth(uint8_t* pCtx) {
    uint8_t idx = (pCtx[0x2092] > 0) ? --pCtx[0x2092] : 0;
    size_t off = static_cast<size_t>(idx & 7u) * 32;
    void* pBs = *reinterpret_cast<void**>(pCtx + 0x1F90 + off);
    uint32_t sampleMask = *reinterpret_cast<uint32_t*>(pCtx + 0x1FA8 + off);
    *reinterpret_cast<void**>(pCtx + 0x480) = pBs;
    std::memcpy(pCtx + 0x488, pCtx + 0x1F98 + off, 16);
    *reinterpret_cast<uint32_t*>(pCtx + 0x498) = sampleMask;

    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xBC00) = pBs;
    std::memcpy(pGnmCtx + 0xBC98, pCtx + 0x1F98 + off, 16);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBCB0) = sampleMask;
}

// 0x21a2010 (63 B): GXDeviceContext_PushBlendStateFromShadow
void GXDeviceContext_PushBlendStateFromShadow(uint8_t* pCtx) {
    uint8_t idx = pCtx[0x2092];
    if (idx < 8) pCtx[0x2092] = idx + 1;
    size_t off = static_cast<size_t>(idx & 7u) * 32;
    *reinterpret_cast<void**>(pCtx + 0x1F90 + off) = *reinterpret_cast<void**>(pCtx + 0x480);
    std::memcpy(pCtx + 0x1F98 + off, pCtx + 0x488, 16);
    *reinterpret_cast<uint32_t*>(pCtx + 0x1FA8 + off) = *reinterpret_cast<uint32_t*>(pCtx + 0x498);
}

// 0x21a1fc0 (71 B): GXDeviceContext_PopBlendStateToGnmOnly
void GXDeviceContext_PopBlendStateToGnmOnly(uint8_t* pCtx) {
    uint8_t idx = (pCtx[0x2092] > 0) ? --pCtx[0x2092] : 0;
    size_t off = static_cast<size_t>(idx & 7u) * 32;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xBC00) = *reinterpret_cast<void**>(pCtx + 0x1F90 + off);
    std::memcpy(pGnmCtx + 0xBC98, pCtx + 0x1F98 + off, 16);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBCB0) = *reinterpret_cast<uint32_t*>(pCtx + 0x1FA8 + off);
}

// 0x21a1f70 (67 B): GXDeviceContext_PushBlendStateFromGnm
void GXDeviceContext_PushBlendStateFromGnm(uint8_t* pCtx) {
    uint8_t idx = pCtx[0x2092];
    if (idx < 8) pCtx[0x2092] = idx + 1;
    size_t off = static_cast<size_t>(idx & 7u) * 32;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pCtx + 0x1F90 + off) = *reinterpret_cast<void**>(pGnmCtx + 0xBC00);
    std::memcpy(pCtx + 0x1F98 + off, pGnmCtx + 0xBC98, 16);
    *reinterpret_cast<uint32_t*>(pCtx + 0x1FA8 + off) = *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBCB0);
}

// ----------------------------------------------------------------------------
// Viewport / Scissor Auto-Sizing & Render Pass Target Helpers
// (0x21a1970 .. 0x21a1e90 & 0x21a2580 .. 0x21a2650)
// ----------------------------------------------------------------------------

// 0x21a1e90 (220 B): GXDeviceContext_UpdateViewportFromBoundRtvOrDsv
void GXDeviceContext_UpdateViewportFromBoundRtvOrDsv(uint8_t* pCtx) {
    uint8_t* pView = *reinterpret_cast<uint8_t**>(pCtx + 0x18); // RTV[0]
    uint32_t mipSlice = 0;
    CGTextureGnmHeader* pTex = nullptr;
    if (pView) {
        pTex = *reinterpret_cast<CGTextureGnmHeader**>(pView + 0x08);
        mipSlice = *reinterpret_cast<uint32_t*>(pView + 0x44);
    } else {
        pView = *reinterpret_cast<uint8_t**>(pCtx + 0x58); // DSV
        if (!pView) return;
        pTex = *reinterpret_cast<CGTextureGnmHeader**>(pView + 0x08);
        mipSlice = *reinterpret_cast<uint32_t*>(pView + 0x50);
    }

    float vp[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    if (pTex && pTex->m_resourceType == 3) {
        vp[2] = static_cast<float>(static_cast<uint32_t>(pTex->m_width) >> mipSlice);
        vp[3] = static_cast<float>(static_cast<uint32_t>(pTex->m_height) >> mipSlice);
    }

    uint32_t vpCount = *reinterpret_cast<uint32_t*>(pCtx + 0xC8);
    if (vpCount <= 1) *reinterpret_cast<uint32_t*>(pCtx + 0xC8) = 1;
    std::memcpy(pCtx + 0xCC, vp, 24);

    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_SetViewportCount(pGnmCtx + 0xB528, 1);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBF8) = 1;
    std::memcpy(pGnmCtx + 0xB978, vp, 24);
}

// 0x21a2650 (193 B): GXDeviceContext_BindTargetsAndResetScissor
bool GXDeviceContext_BindTargetsAndResetScissor(uint8_t* pCtx) {
    // Pops render target stack via sub-object at +0x1728 (flags=0) and resets scissor to RTV0 size
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e47290(pTargetHelper, 0);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);

    uint8_t* pRtv0 = *reinterpret_cast<uint8_t**>(pCtx + 0x18);
    if (pRtv0) {
        CGTextureGnmHeader* pTex = *reinterpret_cast<CGTextureGnmHeader**>(pRtv0 + 0x08);
        uint32_t scissor[4] = { 0, 0, 0, 0 };
        if (pTex && pTex->m_resourceType == 3) {
            scissor[2] = pTex->m_width;
            scissor[3] = pTex->m_height;
        }
        if (*reinterpret_cast<uint32_t*>(pCtx + 0x18C) <= 1) {
            *reinterpret_cast<uint32_t*>(pCtx + 0x18C) = 1;
        }
        std::memcpy(pCtx + 0x190, scissor, 16);
        uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
        CGGfxContext_SetScissorCount(pGnmCtx + 0xB528, 1);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBFC) = 1;
        std::memcpy(pGnmCtx + 0xBAF8, scissor, 16);
    }
    return ok;
}

// 0x21a2580 (201 B): GXDeviceContext_BindTargetsAndResetViewportAndScissor
bool GXDeviceContext_BindTargetsAndResetViewportAndScissor(uint8_t* pCtx, void* pRtv0, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e46f40(pTargetHelper, pRtv0, pDsv, 0);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_UpdateViewportFromBoundRtvOrDsv(pCtx);

    uint8_t* pBoundRtv0 = *reinterpret_cast<uint8_t**>(pCtx + 0x18);
    if (pBoundRtv0) {
        CGTextureGnmHeader* pTex = *reinterpret_cast<CGTextureGnmHeader**>(pBoundRtv0 + 0x08);
        uint32_t scissor[4] = { 0, 0, 0, 0 };
        if (pTex && pTex->m_resourceType == 3) {
            scissor[2] = pTex->m_width;
            scissor[3] = pTex->m_height;
        }
        if (*reinterpret_cast<uint32_t*>(pCtx + 0x18C) <= 1) {
            *reinterpret_cast<uint32_t*>(pCtx + 0x18C) = 1;
        }
        std::memcpy(pCtx + 0x190, scissor, 16);
        uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
        CGGfxContext_SetScissorCount(pGnmCtx + 0xB528, 1);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBFC) = 1;
        std::memcpy(pGnmCtx + 0xBAF8, scissor, 16);
    }
    return ok;
}

// 0x21a1e40 (72 B): GXDeviceContext_ApplyRenderTargetsOnly
bool GXDeviceContext_ApplyRenderTargetsOnly(uint8_t* pCtx) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e47290(pTargetHelper, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    pCtx[0x1670] &= ~0x01u;
    return ok;
}

// 0x21a1c50 (321 B): GXDeviceContext_SyncViewportAndScissorToGnm
void GXDeviceContext_SyncViewportAndScissorToGnm(uint8_t* pCtx) {
    GXDeviceContext_UpdateViewportFromBoundRtvOrDsv(pCtx);

    uint8_t* pRtv0 = *reinterpret_cast<uint8_t**>(pCtx + 0x18);
    if (pRtv0) {
        CGTextureGnmHeader* pTex = *reinterpret_cast<CGTextureGnmHeader**>(pRtv0 + 0x08);
        uint32_t scissor[4] = { 0, 0, 0, 0 };
        if (pTex && pTex->m_resourceType == 3) {
            scissor[2] = pTex->m_width;
            scissor[3] = pTex->m_height;
        }
        if (*reinterpret_cast<uint32_t*>(pCtx + 0x18C) <= 1) {
            *reinterpret_cast<uint32_t*>(pCtx + 0x18C) = 1;
        }
        std::memcpy(pCtx + 0x190, scissor, 16);
        uint8_t* pGnmCtx0 = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
        CGGfxContext_SetScissorCount(pGnmCtx0 + 0xB528, 1);
        *reinterpret_cast<uint32_t*>(pGnmCtx0 + 0xBBFC) = 1;
        std::memcpy(pGnmCtx0 + 0xBAF8, scissor, 16);
    }

    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_SetViewportCount(pGnmCtx + 0xB528, 1);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBF8) = 1;
    std::memcpy(pGnmCtx + 0xB978, pCtx + 0xCC, 24);

    uint32_t scCount = *reinterpret_cast<uint32_t*>(pCtx + 0x18C);
    uint8_t* pGnmCtx2 = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_SetScissorCount(pGnmCtx2 + 0xB528, scCount);
    *reinterpret_cast<uint32_t*>(pGnmCtx2 + 0xBBFC) = scCount;
    if (scCount != 0) {
        std::memcpy(pGnmCtx2 + 0xBAF8, pCtx + 0x190, static_cast<size_t>(scCount) * 16);
    }

    pCtx[0x1670] &= ~0x0Cu;
}

// 0x21a1df0 (74 B): GXDeviceContext_SetRenderTargetAndSyncViewportScissorA
bool GXDeviceContext_SetRenderTargetAndSyncViewportScissorA(uint8_t* pCtx, uint32_t numRtvs, void* ppRtvs, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = reinterpret_cast<bool(*)(void*, uint32_t, void*, void*, uint32_t)>(GX_VMA(0xe47100ULL))(
        pTargetHelper, numRtvs, ppRtvs, pDsv, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_SyncViewportAndScissorToGnm(pCtx);
    return ok;
}

// 0x21a1da0 (74 B): GXDeviceContext_SetRenderTargetAndSyncViewportScissorB
bool GXDeviceContext_SetRenderTargetAndSyncViewportScissorB(uint8_t* pCtx, void* pRtv0, void* pRtv1, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e46fd0(pTargetHelper, pRtv0, pRtv1, pDsv, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_SyncViewportAndScissorToGnm(pCtx);
    return ok;
}

// 0x21a1c00 (73 B): GXDeviceContext_SetRenderTargetsAndSyncViewportScissor
bool GXDeviceContext_SetRenderTargetsAndSyncViewportScissor(uint8_t* pCtx, void* pRtv0, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e46f40(pTargetHelper, pRtv0, pDsv, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_SyncViewportAndScissorToGnm(pCtx);
    return ok;
}

// 0x21a1bb0 (65 B): GXDeviceContext_SetRenderTargetNoViewportSync
bool GXDeviceContext_SetRenderTargetNoViewportSync(uint8_t* pCtx) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e47290(pTargetHelper, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    return ok;
}

// 0x21a1a00 (270 B): GXDeviceContext_UpdateViewportFromActiveTarget1EC0
void GXDeviceContext_UpdateViewportFromActiveTarget1EC0(uint8_t* pCtx) {
    uint8_t* pTarget = *reinterpret_cast<uint8_t**>(pCtx + 0x1EC0);
    if (!pTarget) return;

    CGTextureGnmHeader* pTex = *reinterpret_cast<CGTextureGnmHeader**>(pTarget + 0x08);
    float vp[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    if (pTex && pTex->m_resourceType == 3) {
        uint32_t mip = *reinterpret_cast<uint32_t*>(pTarget + 0x44);
        vp[2] = static_cast<float>(static_cast<uint32_t>(pTex->m_width) >> mip);
        vp[3] = static_cast<float>(static_cast<uint32_t>(pTex->m_height) >> mip);
    }

    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    CGGfxContext_SetViewportCount(pGnmCtx + 0xB528, 1);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBF8) = 1;
    std::memcpy(pGnmCtx + 0xB978, vp, 24);

    if (*reinterpret_cast<uint32_t*>(pCtx + 0xC8) <= 1) {
        *reinterpret_cast<uint32_t*>(pCtx + 0xC8) = 1;
    }
    std::memcpy(pCtx + 0xCC, vp, 24);
    pCtx[0x1670] &= ~0x04u;
}

// 0x21a1b60 (74 B): GXDeviceContext_BindTargetAndUpdateViewportA
bool GXDeviceContext_BindTargetAndUpdateViewportA(uint8_t* pCtx, uint32_t numRtvs, void* ppRtvs, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = reinterpret_cast<bool(*)(void*, uint32_t, void*, void*, uint32_t)>(GX_VMA(0xe47100ULL))(
        pTargetHelper, numRtvs, ppRtvs, pDsv, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_UpdateViewportFromActiveTarget1EC0(pCtx);
    return ok;
}

// 0x21a1b10 (74 B): GXDeviceContext_BindTargetAndUpdateViewportB
bool GXDeviceContext_BindTargetAndUpdateViewportB(uint8_t* pCtx, void* pRtv0, void* pRtv1, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e46fd0(pTargetHelper, pRtv0, pRtv1, pDsv, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_UpdateViewportFromActiveTarget1EC0(pCtx);
    return ok;
}

// 0x21a19b0 (73 B): GXDeviceContext_BindTargetsAndUpdateViewport
bool GXDeviceContext_BindTargetsAndUpdateViewport(uint8_t* pCtx, void* pRtv0, void* pDsv) {
    void* pTargetHelper = pCtx + 0x1728;
    bool ok = sub_e46f40(pTargetHelper, pRtv0, pDsv, 1);
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = sub_e47840(pTargetHelper);
    GXDeviceContext_UpdateViewportFromActiveTarget1EC0(pCtx);
    return ok;
}

// 0x21a1970 (61 B): GXDeviceContext_FinalizeCommandListIfRecording
bool GXDeviceContext_FinalizeCommandListIfRecording(uint8_t* pCtx, void* a2, uint8_t flags) {
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    if (!pGnmCtx) return false;
    if (*reinterpret_cast<uint32_t*>(pGnmCtx + 0xB510) == 0) return false;
    if (pCtx[0x2094] != 0) return false;
    return CGGfxContext_EndCommandRecording(pGnmCtx, a2, flags) == 0;
}

// ----------------------------------------------------------------------------
// Per-stage dirty-run helpers for `GXDeviceContext_FlushDirtyStateToGnmContext`
// ----------------------------------------------------------------------------
static inline void GXDeviceContext_FlushStageSamplers(
    uint8_t* pCtx,
    uint8_t* pGnmCtx,
    size_t minMaxOff,
    size_t maskOff,
    size_t ctxSamplersOff,
    size_t gnmSamplersOff,
    size_t gnmDirtyOff)
{
    uint32_t minSlot = *reinterpret_cast<uint32_t*>(pCtx + minMaxOff);
    uint32_t maxSlot = *reinterpret_cast<uint32_t*>(pCtx + minMaxOff + 4);
    if (minSlot < maxSlot) {
        uint64_t slotMask = *reinterpret_cast<uint64_t*>(pCtx + maskOff);
        uint32_t cur = minSlot;
        while (cur < maxSlot) {
            uint32_t end = cur + 1;
            while (end < maxSlot && ((slotMask >> end) & 1ULL)) {
                ++end;
            }
            uint64_t runBits = (((1ULL << (end - cur)) - 1ULL) << cur);
            *reinterpret_cast<uint64_t*>(pGnmCtx + gnmDirtyOff) |= runBits;
            for (uint32_t s = cur; s < end; ++s) {
                *reinterpret_cast<void**>(pGnmCtx + gnmSamplersOff + static_cast<size_t>(s) * 8) =
                    *reinterpret_cast<void**>(pCtx + ctxSamplersOff + static_cast<size_t>(s) * 8);
            }
            uint32_t next = end + 1;
            while (next < maxSlot && !((slotMask >> next) & 1ULL)) {
                ++next;
            }
            cur = next;
        }
    }
    *reinterpret_cast<uint64_t*>(pCtx + minMaxOff) = 0x10ULL;
}

static inline void GXDeviceContext_FlushStageConstantBuffers(
    uint8_t* pCtx,
    uint8_t* pGnmCtx,
    size_t minMaxOff,
    size_t maskOff,
    size_t ctxCbsOff,
    size_t gnmCbsOff,
    size_t gnmDirtyOff,
    uint64_t stageUsageBit)
{
    uint32_t minSlot = *reinterpret_cast<uint32_t*>(pCtx + minMaxOff);
    uint32_t maxSlot = *reinterpret_cast<uint32_t*>(pCtx + minMaxOff + 4);
    if (minSlot < maxSlot) {
        uint64_t slotMask = *reinterpret_cast<uint64_t*>(pCtx + maskOff);
        uint32_t cur = minSlot;
        while (cur < maxSlot) {
            uint32_t end = cur + 1;
            while (end < maxSlot && ((slotMask >> end) & 1ULL)) {
                ++end;
            }
            uint64_t runBits = (((1ULL << (end - cur)) - 1ULL) << (cur + 16));
            *reinterpret_cast<uint64_t*>(pGnmCtx + gnmDirtyOff) |= runBits;
            for (uint32_t s = cur; s < end; ++s) {
                uint8_t* pBuf = *reinterpret_cast<uint8_t**>(pCtx + ctxCbsOff + static_cast<size_t>(s) * 8);
                if (pBuf) {
                    uint32_t req = (1u << s) | static_cast<uint32_t>(stageUsageBit);
                    uint32_t curUsage = *reinterpret_cast<uint32_t*>(pBuf + 0x44);
                    if ((curUsage & req) != req) {
                        __sync_fetch_and_or(reinterpret_cast<uint32_t*>(pBuf + 0x44), req);
                    }
                }
            }
            for (uint32_t s = cur; s < end; ++s) {
                *reinterpret_cast<void**>(pGnmCtx + gnmCbsOff + static_cast<size_t>(s) * 8) =
                    *reinterpret_cast<void**>(pCtx + ctxCbsOff + static_cast<size_t>(s) * 8);
            }
            uint32_t next = end + 1;
            while (next < maxSlot && !((slotMask >> next) & 1ULL)) {
                ++next;
            }
            cur = next;
        }
    }
    *reinterpret_cast<uint64_t*>(pCtx + minMaxOff) = 0x0EULL;
}

static inline void GXDeviceContext_FlushStageSrvs(
    uint8_t* pCtx,
    uint8_t* pGnmCtx,
    size_t minMaxOff,
    size_t maskOff,
    size_t ctxSrvsOff,
    size_t gnmSrvsOff,
    size_t gnmSrvLoOff,
    size_t gnmSrvHiOff)
{
    uint32_t minSlot = *reinterpret_cast<uint32_t*>(pCtx + minMaxOff);
    uint32_t maxSlot = *reinterpret_cast<uint32_t*>(pCtx + minMaxOff + 4);
    if (minSlot < maxSlot) {
        uint64_t slotMask = *reinterpret_cast<uint64_t*>(pCtx + maskOff);
        uint32_t cur = minSlot;
        while (cur < maxSlot) {
            uint32_t end = cur + 1;
            while (end < maxSlot && ((slotMask >> end) & 1ULL)) {
                ++end;
            }
            for (uint32_t s = cur; s < end; ++s) {
                if (s < 64) {
                    *reinterpret_cast<uint64_t*>(pGnmCtx + gnmSrvLoOff) |= (1ULL << s);
                } else {
                    *reinterpret_cast<uint64_t*>(pGnmCtx + gnmSrvHiOff) |= (1ULL << (s - 64));
                }
                *reinterpret_cast<void**>(pGnmCtx + gnmSrvsOff + static_cast<size_t>(s) * 8) =
                    *reinterpret_cast<void**>(pCtx + ctxSrvsOff + static_cast<size_t>(s) * 8);
            }
            uint32_t next = end + 1;
            while (next < maxSlot && !((slotMask >> next) & 1ULL)) {
                ++next;
            }
            cur = next;
        }
    }
    *reinterpret_cast<uint64_t*>(pCtx + minMaxOff) = 0x3BULL;
}

// ----------------------------------------------------------------------------
// 0x219fcf0 (7,286 B): GXDeviceContext_FlushDirtyStateToGnmContext
// Synchronizes all dirty GX shadow state in `pCtx` (+0x10..+0x1724) into the
// underlying `CGGfxContextGnm` (`pGnmCtx` at `*(pCtx + 0x08)`) and clears the
// dirty state bitmasks (`+0x1670..+0x1727`).
// ----------------------------------------------------------------------------
void GXDeviceContext_FlushDirtyStateToGnmContext(uint8_t* pCtx) {
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    uint64_t dirtyFlags = *reinterpret_cast<uint64_t*>(pCtx + 0x1670);

    // 1. Render Targets, Depth-Stencil View & OM UAVs (bits 0x01 / 0x02)
    if (dirtyFlags & 0x02ULL) {
        uint32_t numRtvs = *reinterpret_cast<uint32_t*>(pCtx + 0x10);
        void* pDsv       = *reinterpret_cast<void**>(pCtx + 0x58);
        uint32_t uavStart = *reinterpret_cast<uint32_t*>(pCtx + 0x60);
        uint32_t uavCount = *reinterpret_cast<uint32_t*>(pCtx + 0x64);

        pGnmCtx[0xB656] = static_cast<uint8_t>(numRtvs);
        if (numRtvs > 0) {
            std::memcpy(pGnmCtx + 0xBC10, pCtx + 0x18, static_cast<size_t>(numRtvs) * sizeof(void*));
        }
        *reinterpret_cast<void**>(pGnmCtx + 0xBC50) = pDsv;
        for (uint32_t i = 0; i < uavCount; ++i) {
            uint32_t slot = uavStart + i;
            *reinterpret_cast<void**>(pGnmCtx + 0xBC58 + static_cast<size_t>(slot) * 8) =
                *reinterpret_cast<void**>(pCtx + 0x68 + static_cast<size_t>(slot) * 8);
        }
    } else if (dirtyFlags & 0x01ULL) {
        uint32_t numRtvs = *reinterpret_cast<uint32_t*>(pCtx + 0x10);
        void* pDsv       = *reinterpret_cast<void**>(pCtx + 0x58);
        pGnmCtx[0xB656] = static_cast<uint8_t>(numRtvs);
        if (numRtvs > 0) {
            std::memcpy(pGnmCtx + 0xBC10, pCtx + 0x18, static_cast<size_t>(numRtvs) * sizeof(void*));
        }
        *reinterpret_cast<void**>(pGnmCtx + 0xBC50) = pDsv;
        pGnmCtx[0xBCB5] = static_cast<uint8_t>(numRtvs);
    }

    // 2. Viewports (bit 0x04)
    if (pCtx[0x1670] & 0x04u) {
        uint32_t vpCount = *reinterpret_cast<uint32_t*>(pCtx + 0xC8);
        if (vpCount == 0) {
            GXDeviceContext_UpdateViewportFromBoundRtvOrDsv(pCtx);
            vpCount = *reinterpret_cast<uint32_t*>(pCtx + 0xC8);
        }
        CGGfxContext_SetViewportCount(pGnmCtx + 0xB528, vpCount);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBF8) = vpCount;
        if (vpCount > 0) {
            std::memcpy(pGnmCtx + 0xB978, pCtx + 0xCC, static_cast<size_t>(vpCount) * 24);
        }
    }

    // 3. Scissor Rectangles (bit 0x08)
    if (pCtx[0x1670] & 0x08u) {
        uint32_t scCount = *reinterpret_cast<uint32_t*>(pCtx + 0x18C);
        if (scCount == 0) {
            uint8_t* pRtv0 = *reinterpret_cast<uint8_t**>(pCtx + 0x18);
            if (pRtv0) {
                CGTextureGnmHeader* pTex = *reinterpret_cast<CGTextureGnmHeader**>(pRtv0 + 0x08);
                uint32_t scissor[4] = { 0, 0, 0, 0 };
                if (pTex && pTex->m_resourceType == 3) {
                    scissor[2] = pTex->m_width;
                    scissor[3] = pTex->m_height;
                }
                *reinterpret_cast<uint32_t*>(pCtx + 0x18C) = 1;
                std::memcpy(pCtx + 0x190, scissor, 16);
                uint8_t* pGnmCtx0 = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
                CGGfxContext_SetScissorCount(pGnmCtx0 + 0xB528, 1);
                *reinterpret_cast<uint32_t*>(pGnmCtx0 + 0xBBFC) = 1;
                std::memcpy(pGnmCtx0 + 0xBAF8, scissor, 16);
                scCount = *reinterpret_cast<uint32_t*>(pCtx + 0x18C);
            }
        }
        CGGfxContext_SetScissorCount(pGnmCtx + 0xB528, scCount);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBFC) = scCount;
        if (scCount > 0) {
            std::memcpy(pGnmCtx + 0xBAF8, pCtx + 0x190, static_cast<size_t>(scCount) * 16);
        }
    }

    // 4. Stream-Out Targets (bit 1ULL << 32, i.e., `pCtx[0x1674] & 0x01`)
    if (pCtx[0x1674] & 0x01u) {
        uint32_t soCount = *reinterpret_cast<uint32_t*>(pCtx + 0x250);
        if (soCount > 0) {
            std::memcpy(pGnmCtx + 0x2920, pCtx + 0x240, static_cast<size_t>(soCount) * sizeof(uint32_t));
            *reinterpret_cast<uint64_t*>(pGnmCtx + 0x1338) |= (((1ULL << soCount) - 1ULL) << 52);
            std::memcpy(pGnmCtx + 0xB950, pCtx + 0x220, static_cast<size_t>(soCount) * sizeof(void*));
        }
    }

    // 5. DepthStencilState (bit 0x10), BlendState (bit 0x40), RasterizerState (bit 0x20)
    uint64_t flags = *reinterpret_cast<uint64_t*>(pCtx + 0x1670);
    if (flags & 0x10ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xBC08) = *reinterpret_cast<void**>(pCtx + 0x210);
        pGnmCtx[0xBCB4] = pCtx[0x218];
        flags = (*reinterpret_cast<uint64_t*>(pCtx + 0x1670) &= ~0x10ULL);
    }
    if (flags & 0x40ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xBC00) = *reinterpret_cast<void**>(pCtx + 0x480);
        std::memcpy(pGnmCtx + 0xBC98, pCtx + 0x488, 16);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBCB0) = *reinterpret_cast<uint32_t*>(pCtx + 0x498);
        flags = (*reinterpret_cast<uint64_t*>(pCtx + 0x1670) &= ~0x40ULL);
    }
    if (flags & 0x20ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xB970) = *reinterpret_cast<void**>(pCtx + 0x258);
        flags = (*reinterpret_cast<uint64_t*>(pCtx + 0x1670) &= ~0x20ULL);
    }

    // 6. Shader Stage Bindings: VS, PS, GS, HS, DS, CS
    // VS
    if (flags & 0x200ULL) {
        GXDeviceContext_FlushStageSamplers(pCtx, pGnmCtx, 0x0778, 0x16A0, 0x06F8, 0xBCC8, 0x0078);
    }
    if (flags & 0x100ULL) {
        GXDeviceContext_FlushStageConstantBuffers(pCtx, pGnmCtx, 0x06F0, 0x1698, 0x0680, 0xC148, 0x0078, 0x10000ULL);
    }
    if (flags & 0x80ULL) {
        GXDeviceContext_FlushStageSrvs(pCtx, pGnmCtx, 0x0678, 0x1690, 0x04A0, 0xBD48, 0x0068, 0x0070);
    }
    if (flags & 0x400ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xC1B8) = *reinterpret_cast<void**>(pCtx + 0x0780);
    }

    // PS
    if (flags & 0x2000ULL) {
        GXDeviceContext_FlushStageSamplers(pCtx, pGnmCtx, 0x0A60, 0x16B8, 0x09E0, 0xD0A8, 0x1978);
    }
    if (flags & 0x1000ULL) {
        GXDeviceContext_FlushStageConstantBuffers(pCtx, pGnmCtx, 0x09D8, 0x16B0, 0x0968, 0xD528, 0x1978, 0x100000ULL);
    }
    if (flags & 0x800ULL) {
        GXDeviceContext_FlushStageSrvs(pCtx, pGnmCtx, 0x0960, 0x16A8, 0x0788, 0xD128, 0x1968, 0x1970);
    }
    if (flags & 0x4000ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xD598) = *reinterpret_cast<void**>(pCtx + 0x0A68);
    }

    // GS
    if (flags & 0x20000ULL) {
        GXDeviceContext_FlushStageSamplers(pCtx, pGnmCtx, 0x0D48, 0x16D0, 0x0CC8, 0xCBB0, 0x1338);
    }
    if (flags & 0x10000ULL) {
        GXDeviceContext_FlushStageConstantBuffers(pCtx, pGnmCtx, 0x0CC0, 0x16C8, 0x0C50, 0xD030, 0x1338, 0x80000ULL);
    }
    if (flags & 0x8000ULL) {
        GXDeviceContext_FlushStageSrvs(pCtx, pGnmCtx, 0x0C48, 0x16C0, 0x0A70, 0xCC30, 0x1328, 0x1330);
    }
    if (flags & 0x40000ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xD0A0) = *reinterpret_cast<void**>(pCtx + 0x0D50);
    }

    // HS
    if (flags & 0x200000ULL) {
        GXDeviceContext_FlushStageSamplers(pCtx, pGnmCtx, 0x1030, 0x16E8, 0x0FB0, 0xC6B8, 0x0CF8);
    }
    if (flags & 0x100000ULL) {
        GXDeviceContext_FlushStageConstantBuffers(pCtx, pGnmCtx, 0x0FA8, 0x16E0, 0x0F38, 0xCB38, 0x0CF8, 0x40000ULL);
    }
    if (flags & 0x80000ULL) {
        GXDeviceContext_FlushStageSrvs(pCtx, pGnmCtx, 0x0F30, 0x16D8, 0x0D58, 0xC738, 0x0CE8, 0x0CF0);
    }
    if (flags & 0x400000ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xCBA8) = *reinterpret_cast<void**>(pCtx + 0x1038);
    }

    // DS
    if (flags & 0x2000000ULL) {
        GXDeviceContext_FlushStageSamplers(pCtx, pGnmCtx, 0x1318, 0x1700, 0x1298, 0xC1C0, 0x06B8);
    }
    if (flags & 0x1000000ULL) {
        GXDeviceContext_FlushStageConstantBuffers(pCtx, pGnmCtx, 0x1290, 0x16F8, 0x1220, 0xC640, 0x06B8, 0x20000ULL);
    }
    if (flags & 0x800000ULL) {
        GXDeviceContext_FlushStageSrvs(pCtx, pGnmCtx, 0x1218, 0x16F0, 0x1040, 0xC240, 0x06A8, 0x06B0);
    }
    if (flags & 0x4000000ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xC6B0) = *reinterpret_cast<void**>(pCtx + 0x1320);
    }

    // CS
    if (flags & 0x20000000ULL) {
        GXDeviceContext_FlushStageSamplers(pCtx, pGnmCtx, 0x1600, 0x1718, 0x1580, 0xD5A0, 0x1FB8);
    }
    if (flags & 0x10000000ULL) {
        GXDeviceContext_FlushStageConstantBuffers(pCtx, pGnmCtx, 0x1578, 0x1710, 0x1508, 0xDA20, 0x1FB8, 0x200000ULL);
    }
    if (flags & 0x8000000ULL) {
        GXDeviceContext_FlushStageSrvs(pCtx, pGnmCtx, 0x1500, 0x1708, 0x1328, 0xD620, 0x1FA8, 0x1FB0);
    }
    if (flags & 0x40000000ULL) {
        *reinterpret_cast<void**>(pGnmCtx + 0xDA90) = *reinterpret_cast<void**>(pCtx + 0x1608);
    }

    // 7. CS UAVs (bit 0x80000000ULL)
    if (flags & 0x80000000ULL) {
        uint32_t minSlot = *reinterpret_cast<uint32_t*>(pCtx + 0x1688);
        uint32_t maxSlot = *reinterpret_cast<uint32_t*>(pCtx + 0x168C);
        if (minSlot < maxSlot) {
            uint64_t slotMask = *reinterpret_cast<uint64_t*>(pCtx + 0x1720);
            uint32_t cur = minSlot;
            while (cur < maxSlot) {
                uint32_t end = cur + 1;
                while (end < maxSlot && ((slotMask >> end) & 1ULL)) {
                    ++end;
                }
                uint64_t runBits = (((1ULL << (end - cur)) - 1ULL) << (cur + 0x24));
                *reinterpret_cast<uint64_t*>(pGnmCtx + 0x1FB8) |= runBits;
                for (uint32_t s = cur; s < end; ++s) {
                    *reinterpret_cast<uint32_t*>(pGnmCtx + 0x1FD0 + static_cast<size_t>(s) * 4) =
                        *reinterpret_cast<uint32_t*>(pCtx + 0x1650 + static_cast<size_t>(s) * 4);
                }
                for (uint32_t s = cur; s < end; ++s) {
                    *reinterpret_cast<void**>(pGnmCtx + 0xDA98 + static_cast<size_t>(s) * 8) =
                        *reinterpret_cast<void**>(pCtx + 0x1610 + static_cast<size_t>(s) * 8);
                }
                uint32_t next = end + 1;
                while (next < maxSlot && !((slotMask >> next) & 1ULL)) {
                    ++next;
                }
                cur = next;
            }
        }
        *reinterpret_cast<uint64_t*>(pCtx + 0x1688) = 0x08ULL;
    }

    // 8. Input Assembler (IA): InputLayout (bit 36), Topology (bit 35), IndexBuffer (bit 33), VertexBuffers (bit 34)
    if (flags & (1ULL << 36)) {
        *reinterpret_cast<void**>(pGnmCtx + 0xB730) = *reinterpret_cast<void**>(pCtx + 0x0260);
    }
    if (flags & (1ULL << 35)) {
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xB738) = *reinterpret_cast<uint32_t*>(pCtx + 0x0268);
    }
    if (flags & (1ULL << 33)) {
        *reinterpret_cast<void**>(pGnmCtx + 0xB740)     = *reinterpret_cast<void**>(pCtx + 0x0270);
        *reinterpret_cast<uint64_t*>(pGnmCtx + 0xB748) = *reinterpret_cast<uint64_t*>(pCtx + 0x0278);
    }
    if (flags & (1ULL << 34)) {
        uint32_t minSlot = *reinterpret_cast<uint32_t*>(pCtx + 0x1680);
        uint32_t maxSlot = *reinterpret_cast<uint32_t*>(pCtx + 0x1684);
        if (minSlot < maxSlot) {
            uint64_t slotMask = *reinterpret_cast<uint64_t*>(pCtx + 0x1678);
            uint32_t cur = minSlot;
            while (cur < maxSlot) {
                uint32_t end = cur + 1;
                while (end < maxSlot && ((slotMask >> end) & 1ULL)) {
                    ++end;
                }
                uint32_t runBits = static_cast<uint32_t>(((1ULL << (end - cur)) - 1ULL) << cur);
                *reinterpret_cast<uint32_t*>(pGnmCtx + 0x25F0) |= runBits;
                for (uint32_t s = cur; s < end; ++s) {
                    *reinterpret_cast<void**>(pGnmCtx + 0xB750 + static_cast<size_t>(s) * 8) =
                        *reinterpret_cast<void**>(pCtx + 0x0280 + static_cast<size_t>(s) * 8);
                    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xB850 + static_cast<size_t>(s) * 4) =
                        *reinterpret_cast<uint32_t*>(pCtx + 0x0380 + static_cast<size_t>(s) * 4);
                    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xB8D0 + static_cast<size_t>(s) * 4) =
                        *reinterpret_cast<uint32_t*>(pCtx + 0x0400 + static_cast<size_t>(s) * 4);
                }
                uint32_t next = end + 1;
                while (next < maxSlot && !((slotMask >> next) & 1ULL)) {
                    ++next;
                }
                cur = next;
            }
        }
    }

    GXDeviceContext_ResetDirtyMasksAfterDraw(pCtx);
}

// 0x219fcc0 (40 B): GXDeviceContext_EndFrameOrFlushIfActive
void GXDeviceContext_EndFrameOrFlushIfActive(uint8_t* pCtx) {
    if (pCtx[0x209C] != 0) {
        GXDeviceContext_FlushDirtyStateToGnmContext(pCtx);
        pCtx[0x209C] = 0;
    }
}

// 0x219fa70 (232 B): GXDeviceContext_ResetShadowStateAndBounds
void GXDeviceContext_ResetShadowStateAndBounds(uint8_t* pCtx) {
    std::memset(pCtx + 0x10, 0, 0x1718);
    // VS
    *reinterpret_cast<uint32_t*>(pCtx + 0x0778) = 0x10;
    *reinterpret_cast<uint32_t*>(pCtx + 0x06F0) = 0x0E;
    *reinterpret_cast<uint32_t*>(pCtx + 0x0678) = 0x3B;
    // PS
    *reinterpret_cast<uint32_t*>(pCtx + 0x0A60) = 0x10;
    *reinterpret_cast<uint32_t*>(pCtx + 0x09D8) = 0x0E;
    *reinterpret_cast<uint32_t*>(pCtx + 0x0960) = 0x3B;
    // GS
    *reinterpret_cast<uint32_t*>(pCtx + 0x0D48) = 0x10;
    *reinterpret_cast<uint32_t*>(pCtx + 0x0CC0) = 0x0E;
    *reinterpret_cast<uint32_t*>(pCtx + 0x0C48) = 0x3B;
    // HS
    *reinterpret_cast<uint32_t*>(pCtx + 0x1030) = 0x10;
    *reinterpret_cast<uint32_t*>(pCtx + 0x0FA8) = 0x0E;
    *reinterpret_cast<uint32_t*>(pCtx + 0x0F30) = 0x3B;
    // DS
    *reinterpret_cast<uint32_t*>(pCtx + 0x1318) = 0x10;
    *reinterpret_cast<uint32_t*>(pCtx + 0x1290) = 0x0E;
    *reinterpret_cast<uint32_t*>(pCtx + 0x1218) = 0x3B;
    // CS
    *reinterpret_cast<uint32_t*>(pCtx + 0x1600) = 0x10;
    *reinterpret_cast<uint32_t*>(pCtx + 0x1578) = 0x0E;
    *reinterpret_cast<uint32_t*>(pCtx + 0x1500) = 0x3B;
    // UAVs & Vertex Streams
    *reinterpret_cast<uint32_t*>(pCtx + 0x1688) = 0x08;
    *reinterpret_cast<uint32_t*>(pCtx + 0x1680) = 0x20;
}

// 0x219fb70 (321 B): GXDeviceContext_BeginRecordingAndResetShadow
void GXDeviceContext_BeginRecordingAndResetShadow(uint8_t* pCtx) {
    if (pCtx[0x209C] != 0) return;
    pCtx[0x2092] = 0;
    *reinterpret_cast<uint16_t*>(pCtx + 0x2090) = 0;
    GXDeviceContext_ResetShadowStateAndBounds(pCtx);
    *reinterpret_cast<void**>(pCtx + 0x1EC8) = nullptr;
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = nullptr;
    reinterpret_cast<void(*)(void*, const void*, uint32_t)>(GX_VMA(0x262a720ULL))(
        pCtx + 0x20A8, reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)), 0);
    *reinterpret_cast<uint32_t*>(pCtx + 0x20E0) = 0;
    pCtx[0x209C] = 1;
}

// 0x219fb60 (12 B): GXDeviceContext_DestroyTargetHelper1728
void GXDeviceContext_DestroyTargetHelper1728(uint8_t* pCtx) {
    sub_e47880(pCtx + 0x1728);
}

// 0x219f930 (319 B): GXDeviceContext_InitializeStateShadow
void GXDeviceContext_InitializeStateShadow(uint8_t* pCtx) {
    *reinterpret_cast<void**>(pCtx + 0x1728) = *reinterpret_cast<void**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pCtx + 0x1730) = pCtx;
    pCtx[0x2092] = 0;
    *reinterpret_cast<uint16_t*>(pCtx + 0x2090) = 0;
    GXDeviceContext_ResetShadowStateAndBounds(pCtx);
    *reinterpret_cast<void**>(pCtx + 0x1EC8) = nullptr;
    *reinterpret_cast<void**>(pCtx + 0x1EC0) = nullptr;
    reinterpret_cast<void(*)(void*, const void*, uint32_t)>(GX_VMA(0x262a720ULL))(
        pCtx + 0x20A8, reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)), 0);
    *reinterpret_cast<uint32_t*>(pCtx + 0x20E0) = 0;
}

// 0x21a3c80 (1962 B): GXRenderTargetResource_BlitOrClearFullscreenQuad
// Saves current GXDeviceContext state, binds `pThis` mip-0 RTV, sets fullscreen
// quad VS/PS/VB/InputLayout (4 vertices, TriangleStrip = 5), draws, and restores
// previous GXDeviceContext state.
bool GXRenderTargetResource_BlitOrClearFullscreenQuad(
    GXRenderTargetResource* pThis,
    uint8_t* pCtx,
    uint8_t** ppSubCtx,
    void* pSourceSrv,
    const uint32_t* pOptionalScissorRect)
{
    uint8_t* pGlobal5540dd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pQuadResources = *reinterpret_cast<uint8_t**>(pGlobal5540dd8 + 0x6078);
    if (!pQuadResources) return false;

    bool hasCustomScissor = pOptionalScissorRect &&
                            ((pOptionalScissorRect[0] != 0) ||
                             (pOptionalScissorRect[2] != 0) ||
                             (pOptionalScissorRect[1] != 0) ||
                             (pOptionalScissorRect[3] != 0));

    // 1. Save previous viewports, RTV0, DSV, BlendState, DepthStencilState, and optional Scissor/RS
    uint32_t savedVpCount   = *reinterpret_cast<uint32_t*>(pCtx + 0xC8);
    if (savedVpCount > 16) savedVpCount = 16;
    size_t   savedVpBytes   = static_cast<size_t>(savedVpCount) * 24;
    uint8_t  savedViewports[16 * 24];
    std::memcpy(savedViewports, pCtx + 0xCC, savedVpBytes);

    void*    savedRtv0      = *reinterpret_cast<void**>(pCtx + 0x18);
    void*    savedDsv       = *reinterpret_cast<void**>(pCtx + 0x58);
    void*    savedBlend     = *reinterpret_cast<void**>(pCtx + 0x480);
    uint8_t  savedBlendFactor[16];
    std::memcpy(savedBlendFactor, pCtx + 0x488, 16);
    uint32_t savedSampleMask = *reinterpret_cast<uint32_t*>(pCtx + 0x498);
    void*    savedDss        = *reinterpret_cast<void**>(pCtx + 0x210);
    uint8_t  savedStencilRef = pCtx[0x218];

    void*    savedRasterizer = nullptr;
    uint32_t savedScCount    = 0;
    uint8_t  savedScissors[16 * 16];
    if (hasCustomScissor) {
        savedRasterizer = *reinterpret_cast<void**>(pCtx + 0x258);
        savedScCount = *reinterpret_cast<uint32_t*>(pCtx + 0x18C);
        if (savedScCount > 16) savedScCount = 16;
        std::memcpy(savedScissors, pCtx + 0x190, static_cast<size_t>(savedScCount) * 16);

        uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        uint8_t* pPresetCache = *reinterpret_cast<uint8_t**>(pGlobal + 0x250);
        void* pScissorRs = *reinterpret_cast<void**>(pPresetCache + 0x200);
        *reinterpret_cast<void**>(pCtx + 0x258) = pScissorRs;
        uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
        *reinterpret_cast<void**>(pGnmCtx + 0xB970) = pScissorRs;

        if (*reinterpret_cast<uint32_t*>(pCtx + 0x18C) <= 1) {
            *reinterpret_cast<uint32_t*>(pCtx + 0x18C) = 1;
        }
        std::memcpy(pCtx + 0x190, pOptionalScissorRect, 16);
        CGGfxContext_SetScissorCount(pGnmCtx + 0xB528, 1);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBFC) = 1;
        std::memcpy(pGnmCtx + 0xBAF8, pOptionalScissorRect, 16);
    }

    // 2. Resolve mip-0 RTV from `pThis`
    void* pRtv0 = nullptr;
    if (pThis->m_pGnmTexture && pThis->m_ppRenderTargetViews) {
        auto fnGetMips = reinterpret_cast<uint32_t(*)(GXRenderTargetResource*)>(pThis->m_vptr[8]); // +0x40
        if (fnGetMips(pThis) != 0) {
            pRtv0 = pThis->m_ppRenderTargetViews[0];
        }
    }

    // 3. Bind default BlendState (+0x00), Sampler (+0x2D0), RTV0, null DSV, and default DepthStencilState (+0x1C8)
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pPresetCache = *reinterpret_cast<uint8_t**>(pGlobal + 0x250);
    void* pDefaultSampler = *reinterpret_cast<void**>(pPresetCache + 0x2D0);
    void* pDefaultBlend   = *reinterpret_cast<void**>(pPresetCache + 0x00);

    *reinterpret_cast<void**>(pCtx + 0x480)    = pDefaultBlend;
    *reinterpret_cast<uint32_t*>(pCtx + 0x498) = ~0u;
    uint8_t* pGnmCtx = *reinterpret_cast<uint8_t**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pGnmCtx + 0xBC00)    = pDefaultBlend;
    *reinterpret_cast<uint64_t*>(pGnmCtx + 0xBC98) = 0;
    *reinterpret_cast<uint64_t*>(pGnmCtx + 0xBCA0) = 0;
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBCB0) = ~0u;

    *reinterpret_cast<uint32_t*>(pCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pCtx + 0x18)    = pRtv0;
    *reinterpret_cast<void**>(pCtx + 0x58)    = nullptr;
    pGnmCtx[0xB656] = 1;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC10) = pRtv0;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC50) = nullptr;
    pGnmCtx[0xBCB5] = 1;

    void* pDefaultDss = *reinterpret_cast<void**>(
        *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)) + 0x250) + 0x1C8);
    *reinterpret_cast<void**>(pCtx + 0x210) = pDefaultDss;
    pCtx[0x218] = 0xFF;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC08) = pDefaultDss;
    pGnmCtx[0xBCB4] = 0xFF;

    // 4. Bind PS SRV[0] (`pSourceSrv`), PS Samplers[0] and [2], PS Shader (+0x9B8), VS Shader (+0xA28)
    *reinterpret_cast<void**>(pCtx + 0x788)    = pSourceSrv;
    *reinterpret_cast<uint32_t*>(pCtx + 0x960) = 0;
    if (*reinterpret_cast<uint32_t*>(pCtx + 0x964) <= 1) {
        *reinterpret_cast<uint32_t*>(pCtx + 0x964) = 1;
    }
    pGnmCtx[0x1968] |= 0x01;
    *reinterpret_cast<void**>(pGnmCtx + 0xD128) = pSourceSrv;

    *reinterpret_cast<void**>(pCtx + 0x9E0)    = pDefaultSampler;
    *reinterpret_cast<uint32_t*>(pCtx + 0xA60) = 0;
    if (*reinterpret_cast<uint32_t*>(pCtx + 0xA64) <= 1) {
        *reinterpret_cast<uint32_t*>(pCtx + 0xA64) = 1;
    }
    *reinterpret_cast<uint64_t*>(pGnmCtx + 0x1978) |= 0x01ULL;
    *reinterpret_cast<void**>(pGnmCtx + 0xD0A8)    = pDefaultSampler;

    *reinterpret_cast<void**>(pCtx + 0x9F0) = pDefaultSampler;
    if (*reinterpret_cast<uint32_t*>(pCtx + 0xA60) > 2) {
        *reinterpret_cast<uint32_t*>(pCtx + 0xA60) = 2;
    }
    if (*reinterpret_cast<uint32_t*>(pCtx + 0xA64) < 3) {
        *reinterpret_cast<uint32_t*>(pCtx + 0xA64) = 3;
    }
    *reinterpret_cast<uint64_t*>(pGnmCtx + 0x1978) |= 0x04ULL;
    *reinterpret_cast<void**>(pGnmCtx + 0xD0B8)    = pDefaultSampler;

    void* pPsGnm = *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(pQuadResources + 0x9B8) + 0x10);
    *reinterpret_cast<void**>(pCtx + 0xA68)    = pPsGnm;
    *reinterpret_cast<void**>(pGnmCtx + 0xD598) = pPsGnm;

    void* pVsGnm = *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(pQuadResources + 0xA28) + 0x10);
    *reinterpret_cast<void**>(pCtx + 0x780)    = pVsGnm;
    *reinterpret_cast<void**>(pGnmCtx + 0xC1B8) = pVsGnm;

    // 5. Configure viewport to `pThis` mip-0 width x height
    auto fnGetWidth  = reinterpret_cast<uint32_t(*)(GXRenderTargetResource*, uint32_t)>(pThis->m_vptr[9]);  // +0x48
    auto fnGetHeight = reinterpret_cast<uint32_t(*)(GXRenderTargetResource*, uint32_t)>(pThis->m_vptr[10]); // +0x50
    float vp[6] = {
        0.0f,
        0.0f,
        static_cast<float>(fnGetWidth(pThis, 0)),
        static_cast<float>(fnGetHeight(pThis, 0)),
        0.0f,
        1.0f
    };
    if (*reinterpret_cast<uint32_t*>(pCtx + 0xC8) <= 1) {
        *reinterpret_cast<uint32_t*>(pCtx + 0xC8) = 1;
    }
    std::memcpy(pCtx + 0xCC, vp, 24);
    CGGfxContext_SetViewportCount(pGnmCtx + 0xB528, 1);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBF8) = 1;
    std::memcpy(pGnmCtx + 0xB978, vp, 24);

    // 6. Bind fullscreen quad InputLayout (+0x9A8), VertexBuffer (+0x9A0, stride=24, offset=0), Topology=5, and Draw(4, 0)
    uint8_t* pDrawCtx    = *ppSubCtx;
    uint8_t* pDrawGnmCtx = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x08);
    void* pQuadLayout    = *reinterpret_cast<void**>(pQuadResources + 0x9A8);
    void* pQuadVb        = *reinterpret_cast<void**>(pQuadResources + 0x9A0);

    *reinterpret_cast<void**>(pDrawCtx + 0x260)    = pQuadLayout;
    *reinterpret_cast<void**>(pDrawGnmCtx + 0xB730) = pQuadLayout;
    *reinterpret_cast<void**>(pDrawCtx + 0x280)    = pQuadVb;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x380) = 24;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x400) = 0;
    pDrawGnmCtx[0x25F0] |= 0x01;
    *reinterpret_cast<void**>(pDrawGnmCtx + 0xB750)    = pQuadVb;
    *reinterpret_cast<uint32_t*>(pDrawGnmCtx + 0xB850) = 24;
    *reinterpret_cast<uint32_t*>(pDrawGnmCtx + 0xB8D0) = 0;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x268)    = 5;
    *reinterpret_cast<uint32_t*>(pDrawGnmCtx + 0xB738) = 5;

    GXDeviceContext_ResetDirtyMasksAfterDraw(pDrawCtx);
    CGGfxContext_DrawAutoOrInstanced(pDrawGnmCtx, pDrawGnmCtx + 0xB730, 4, 0);

    // 7. Unbind PS SRV[0], flush, and restore all saved GXDeviceContext state
    *reinterpret_cast<void**>(pCtx + 0x788)    = nullptr;
    *reinterpret_cast<uint32_t*>(pCtx + 0x960) = 0;
    if (*reinterpret_cast<uint32_t*>(pCtx + 0x964) <= 1) {
        *reinterpret_cast<uint32_t*>(pCtx + 0x964) = 1;
    }
    *reinterpret_cast<uint64_t*>(pGnmCtx + 0x1968) |= 0x01ULL;
    *reinterpret_cast<void**>(pGnmCtx + 0xD128)    = nullptr;
    GXDeviceContext_FlushDirtyStateToGnmContext(pCtx);

    *reinterpret_cast<uint32_t*>(pCtx + 0x10) = 1;
    *reinterpret_cast<void**>(pCtx + 0x18)    = savedRtv0;
    *reinterpret_cast<void**>(pCtx + 0x58)    = savedDsv;
    pGnmCtx[0xB656] = 1;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC10) = savedRtv0;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC50) = savedDsv;
    pGnmCtx[0xBCB5] = 1;

    if (*reinterpret_cast<uint32_t*>(pCtx + 0xC8) <= savedVpCount) {
        *reinterpret_cast<uint32_t*>(pCtx + 0xC8) = savedVpCount;
    }
    if (savedVpCount != 0) {
        std::memcpy(pCtx + 0xCC, savedViewports, savedVpBytes);
    }
    CGGfxContext_SetViewportCount(pGnmCtx + 0xB528, savedVpCount);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBF8) = savedVpCount;
    if (savedVpCount != 0) {
        std::memcpy(pGnmCtx + 0xB978, savedViewports, savedVpBytes);
    }

    *reinterpret_cast<void**>(pCtx + 0x480) = savedBlend;
    std::memcpy(pCtx + 0x488, savedBlendFactor, 16);
    *reinterpret_cast<uint32_t*>(pCtx + 0x498) = savedSampleMask;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC00) = savedBlend;
    std::memcpy(pGnmCtx + 0xBC98, savedBlendFactor, 16);
    *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBCB0) = savedSampleMask;

    *reinterpret_cast<void**>(pCtx + 0x210) = savedDss;
    pCtx[0x218] = savedStencilRef;
    *reinterpret_cast<void**>(pGnmCtx + 0xBC08) = savedDss;
    pGnmCtx[0xBCB4] = savedStencilRef;

    if (hasCustomScissor) {
        *reinterpret_cast<void**>(pCtx + 0x258)    = savedRasterizer;
        *reinterpret_cast<void**>(pGnmCtx + 0xB970) = savedRasterizer;
        uint32_t restoreScCount = (savedScCount != 0) ? savedScCount : 1u;
        *reinterpret_cast<uint32_t*>(pCtx + 0x18C) = savedScCount;
        if (savedScCount != 0) {
            std::memcpy(pCtx + 0x190, savedScissors, static_cast<size_t>(savedScCount) * 16);
        }
        CGGfxContext_SetScissorCount(pGnmCtx + 0xB528, restoreScCount);
        *reinterpret_cast<uint32_t*>(pGnmCtx + 0xBBFC) = savedScCount;
        if (savedScCount != 0) {
            std::memcpy(pGnmCtx + 0xBAF8, savedScissors, static_cast<size_t>(savedScCount) * 16);
        }
    }

    GXDeviceContext_FlushDirtyStateToGnmContext(pCtx);
    return true;
}

} // namespace GX
} // namespace Bloodborne
