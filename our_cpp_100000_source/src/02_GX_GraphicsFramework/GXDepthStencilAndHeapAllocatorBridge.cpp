// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Module 02: GX_GraphicsFramework
// File: GXDepthStencilAndHeapAllocatorBridge.cpp
// Address Range: 0x219c0e0 .. 0x219f740 (83 functions, 12,722 bytes)
//   (Covers full RTL range 0x219b000 .. 0x219f80f; 0x219ac70 spans 0x219ac70..0x219c04c)
//
// Contains:
//   1. GXDepthStencilResource (0x219ed90 .. 0x219f740, 8 functions):
//      Depth-stencil texture & view wrapper (vtable 0x53b6860 -> 0x53b6a20 ->
//      0x53b6b00) managing read-write per-mip DSVs (+0x60), read-only per-mip
//      DSVs (+0x68), depth SRV (+0x48), optional stencil SRV (+0x70), and
//      GXResourceMemoryTracker bucket 3 telemetry (+0xB0..+0xD8).
//   2. GXThreadLocalObjectPool, GXGammaRampLookupTable & GXDoubleBufferedRing
//      (0x219e5b0 .. 0x219ecf0, 14 functions):
//      Per-worker-thread object cache (20 entries per worker thread, indexed
//      via TLS +0x53e4658), 1025-entry RGB gamma ramp generator (identity &
//      power-law curve), and double-buffered ring slot helpers.
//   3. GXDisplayOutputManager, GXRenderThreadJobDispatcher &
//      GXHeapAllocatorAdapter (0x219c8a0 .. 0x219e320, 38 functions):
//      Multi-output presentation & gamma-ramp uploader (0x219d220),
//      multi-context render job submission (0x219d2c0 -> 0x21b4a00),
//      thread-safe DLHeap allocator bridge (0x219c8a0 .. 0x219d550), and
//      DLVector reallocation helpers.
//   4. GXDebugPropertyNodes (0x219c0e0 .. 0x219c89e & 0x219db10 .. 0x219dee0,
//      23 functions):
//      Intrusive strong/weak reference-counted (15-bit + 15-bit) debug UI
//      property nodes for GX runtime parameters.
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>

namespace Bloodborne {
namespace GX {

struct GXResourceMemoryTracker;

extern "C" {
    void* GXGlobalDevice_GetInstance(); // *(void**)0x55406c8
    void* CGObject_GetCreatorAllocator(void* pObj); // 0x207b720
    void  CGBufferGnm_Release(void* pBuf); // 0x2192660
    void  GXTexture2DResource_ReleaseGpuTexture(void* pTexRes); // 0x21a6a00

    int   GXDevice_CreateTexture(void* pDev, void** outTex, const void* pDesc, const void* pInitData, void* a5, void* a6, void* a7); // 0x2165f90
    int   GXDevice_CreateDepthStencilView(void* pDev, void** outDsv, void* pTex, const void* pDsvDesc, void* a5); // 0x2166610
    int   GXDevice_CreateShaderResourceView(void* pDev, void** outSrv, void* pTex, const void* pSrvDesc, void* a5); // 0x21669a0
    void  GXResourceMemoryTracker_RegisterDepthStencil(GXResourceMemoryTracker* pTracker, void* pDsRes); // 0x21aca50

    void  GXFlipController_PresentAndWaitFramePacing(void* pFlipCtrl, uint32_t syncInterval); // 0x21b2fb0
    void  GXRenderJobQueue_SubmitJob(void* pJobQueue, void* pJob, uint32_t flags); // 0x21b4a00
    void  CGDisplayOutput_SetGammaRamp(void* pDisplayOut, const void* pGammaData); // 0x26d5d90

    void* DLHeap_Allocate(void* pHeap, size_t size, size_t align); // 0x2088030
    void* DLHeap_AllocateAligned(void* pHeap, size_t size, size_t align); // 0x20882f0
    size_t DLHeap_GetAllocationSize(void* pHeap, const void* pPtr); // 0x20887e0
    void  DLHeap_Free(void* pHeap, void* pPtr); // 0x2088900
    bool  DLHeap_CheckIntegrity(void* pHeap); // 0x2088cf0
    void  DLHeap_FreeAligned(void* pHeap, void* pPtr); // 0x2088f40

    void     sub_e316e0(void* pMasterPool);
    uint32_t sub_e31820(void* pMasterPool, void** ppOutSlots, uint32_t batchCount);
    uint32_t sub_e31990(void* pMasterPool);
    void*    sub_e45560(void* pRingAlloc, uint32_t size, uint32_t align);
    void     sub_e47930(void* pJobEntry, void* pDeviceCtx);
    void     sub_207b8c0(void* pSubObj);
    void     sub_207ee90(void* pLockAndSem);
    void     sub_2084ae0(void* pSubHeap, void* pArenaMem);
    void     sub_20b55b0(const char* pFile, uint32_t line, const char* pMsg, ...);
    void     sub_23586d0(void* pPropNode, void* pParentDesc, const void* pNamePair, uint32_t flags);
    void     sub_2367220(void* pParentNode, void* pWrapperNode);
    void     sub_261a1b0(void* pDst, const void* pSrc, size_t size);
    void     sub_26295d0(void* pOutWString, const wchar_t* pFmt, ...);
    void     sub_2bbe588(void* pPtr);
    void     sub_2bbe978(void* pJmpBuf, int status);
    int      sub_2bbfee8(void* pSema);
    int      sub_2bbff08(void* pSema);
}

// ============================================================================
// 1. GXDepthStencilResource (0x219ed90 .. 0x219f740, 8 functions)
// ============================================================================

struct CGTextureGnmHeader {
    void**           m_vptr;             // +0x00
    void*            m_pAllocator;       // +0x08
    volatile int32_t m_refCount;         // +0x10
    uint8_t          m_pad14[0x18];
    uint32_t         m_byteSize;         // +0x2C
    uint8_t          m_pad30[0x14];
    uint16_t         m_width;            // +0x44
    uint16_t         m_height;           // +0x46
    uint16_t         m_depthOrArraySize; // +0x48
    uint8_t          m_mipLevels;        // +0x4A
    uint8_t          m_format;           // +0x4B
};

struct __attribute__((packed)) GXDepthStencilResource {
    void**              m_vptr;                       // +0x00 (0x53b6860)
    uint32_t            m_refCount;                   // +0x08
    uint32_t            m_pad0C;                      // +0x0C
    float               m_colorScale[4];              // +0x10
    float               m_clearValues[4];             // +0x20
    void*               m_pColorScaleCBuf;            // +0x30
    uint64_t            m_pad38;                      // +0x38
    CGTextureGnmHeader* m_pGnmTexture;                // +0x40
    void*               m_pDepthSrv;                  // +0x48
    void*               m_pUav;                       // +0x50
    uint32_t            m_byteSize;                   // +0x58
    uint32_t            m_pad5C;                      // +0x5C
    void**              m_ppDepthStencilViews;        // +0x60 (read-write DSV per mip)
    void**              m_ppReadOnlyDepthStencilViews;// +0x68 (read-only DSV per mip)
    void*               m_pStencilSrv;                // +0x70
    int32_t             m_unk78;                      // +0x78 (-1)
    int64_t             m_unk7C;                      // +0x7C (-1)
};

struct GXDepthStencilCreateDesc {
    uint32_t width;      // +0x00
    uint32_t height;     // +0x04
    uint32_t format;     // +0x08
    uint32_t mipLevels;  // +0x0C
    uint64_t sampleDesc; // +0x10
    uint32_t miscFlags;  // +0x18
    uint32_t dsvFlags;   // +0x1C
    uint64_t unk20;      // +0x20
};

// 0x219f740 (56 B): GXDepthStencilResource_GetReadOnlyDepthStencilViewByMip
void* GXDepthStencilResource_GetReadOnlyDepthStencilViewByMip(
    GXDepthStencilResource* pThis,
    uint32_t mipIndex)
{
    if (!pThis->m_ppDepthStencilViews) return nullptr;
    auto fnGetMips = reinterpret_cast<uint32_t(*)(GXDepthStencilResource*)>(pThis->m_vptr[8]); // +0x40
    if (mipIndex >= fnGetMips(pThis)) return nullptr;
    return pThis->m_ppReadOnlyDepthStencilViews[mipIndex];
}

// 0x219f700 (56 B): GXDepthStencilResource_GetDepthStencilViewByMip
void* GXDepthStencilResource_GetDepthStencilViewByMip(
    GXDepthStencilResource* pThis,
    uint32_t mipIndex)
{
    if (!pThis->m_ppDepthStencilViews) return nullptr;
    auto fnGetMips = reinterpret_cast<uint32_t(*)(GXDepthStencilResource*)>(pThis->m_vptr[8]); // +0x40
    if (mipIndex >= fnGetMips(pThis)) return nullptr;
    return pThis->m_ppDepthStencilViews[mipIndex];
}

// 0x219f130 (1352 B): GXDepthStencilResource_CreateFromDesc
bool GXDepthStencilResource_CreateFromDesc(
    GXDepthStencilResource* pThis,
    const GXDepthStencilCreateDesc* pDesc,
    void* pExtra1,
    void* pExtra2)
{
    auto fnRelease = reinterpret_cast<void(*)(GXDepthStencilResource*)>(pThis->m_vptr[18]); // +0x90
    fnRelease(pThis);

    uint32_t texFormat = 0;
    uint32_t depthSrvFormat = 0;
    uint32_t stencilSrvFormat = 0;

    switch (pDesc->format) {
        case 0x14: // DXGI_FORMAT_D32_FLOAT_S8X24_UINT
            texFormat        = 0x15;
            depthSrvFormat   = 0x15;
            stencilSrvFormat = 0x16;
            break;
        case 0x28: // DXGI_FORMAT_D32_FLOAT
            texFormat        = 0x28;
            depthSrvFormat   = 0x28;
            stencilSrvFormat = 0;
            break;
        case 0x2D: // DXGI_FORMAT_D24_UNORM_S8_UINT
            texFormat        = 0x2C;
            depthSrvFormat   = 0x2E;
            stencilSrvFormat = 0x2F;
            break;
        case 0x37: // DXGI_FORMAT_D16_UNORM
            texFormat        = 0x35;
            depthSrvFormat   = 0x38;
            stencilSrvFormat = 0;
            break;
        default:
            return false;
    }

    struct __attribute__((packed)) TextureDesc {
        uint32_t width;
        uint32_t height;
        uint32_t mipLevels;
        uint32_t arraySize;
        uint32_t format;
        uint64_t sampleDesc;
        uint32_t usage;
        uint32_t bindFlags;
        uint32_t cpuAccess;
        uint32_t miscFlags;
    } hwDesc{};

    hwDesc.width      = pDesc->width;
    hwDesc.height     = pDesc->height;
    hwDesc.mipLevels  = pDesc->mipLevels;
    hwDesc.arraySize  = 1;
    hwDesc.format     = texFormat;
    hwDesc.sampleDesc = pDesc->sampleDesc;
    hwDesc.usage      = 0;
    hwDesc.bindFlags  = 0x48; // D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL
    hwDesc.cpuAccess  = 0;
    hwDesc.miscFlags  = pDesc->miscFlags;

    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGTextureGnmHeader* pNewTex = nullptr;
    int err = GXDevice_CreateTexture(
        pDev, reinterpret_cast<void**>(&pNewTex), &hwDesc, nullptr, pExtra1, pExtra2, nullptr);

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

    // Allocate read-write DSV array (+0x60) and read-only DSV array (+0x68)
    void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    auto fnAllocAligned = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(
        *reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);

    uint32_t mipCount = pDesc->mipLevels;
    uint8_t* pRwBlock = reinterpret_cast<uint8_t*>(fnAllocAligned(pAlloc, mipCount * 8 + 0x10, 0x10));
    if (pRwBlock) {
        void** ppRw = reinterpret_cast<void**>(pRwBlock + 0x10);
        std::memset(ppRw, 0, mipCount * sizeof(void*));
        *reinterpret_cast<void**>(pRwBlock + 0x00)    = pRwBlock;
        *reinterpret_cast<uint64_t*>(pRwBlock + 0x08) = mipCount;
        pThis->m_ppDepthStencilViews = ppRw;
    } else {
        pThis->m_ppDepthStencilViews = nullptr;
    }

    uint8_t* pRoBlock = reinterpret_cast<uint8_t*>(fnAllocAligned(pAlloc, mipCount * 8 + 0x10, 0x10));
    if (!pRoBlock) {
        pThis->m_ppReadOnlyDepthStencilViews = nullptr;
        fnRelease(pThis);
        return false;
    }
    void** ppRo = reinterpret_cast<void**>(pRoBlock + 0x10);
    std::memset(ppRo, 0, mipCount * sizeof(void*));
    *reinterpret_cast<void**>(pRoBlock + 0x00)    = pRoBlock;
    *reinterpret_cast<uint64_t*>(pRoBlock + 0x08) = mipCount;
    pThis->m_ppReadOnlyDepthStencilViews = ppRo;

    if (!pThis->m_ppDepthStencilViews) {
        fnRelease(pThis);
        return false;
    }

    uint32_t sampleCount = static_cast<uint32_t>(pDesc->sampleDesc);
    for (uint32_t mip = 0; mip < mipCount; ++mip) {
        uint32_t dsvDesc[6];
        dsvDesc[0] = pDesc->format;
        dsvDesc[1] = (sampleCount > 1) ? 5u : 3u;
        dsvDesc[2] = pDesc->dsvFlags;
        dsvDesc[3] = mip;
        dsvDesc[4] = 0;
        dsvDesc[5] = 1;

        void* localRwDsv = nullptr;
        if (GXDevice_CreateDepthStencilView(
                pDev, &localRwDsv, pNewTex, dsvDesc, pExtra1) != 0) {
            fnRelease(pThis);
            return false;
        }
        pThis->m_ppDepthStencilViews[mip] = localRwDsv;

        // Read-only DSV: D3D11_DSV_READ_ONLY_DEPTH (1) or DEPTH | STENCIL (3)
        void* localRoDsv = nullptr;
        dsvDesc[2] = (pDesc->format == 0x28 || pDesc->format == 0x37) ? 1u : 3u;
        if (GXDevice_CreateDepthStencilView(
                pDev, &localRoDsv, pNewTex, dsvDesc, pExtra1) != 0) {
            fnRelease(pThis);
            return false;
        }
        pThis->m_ppReadOnlyDepthStencilViews[mip] = localRoDsv;
    }

    if (!pThis->m_pGnmTexture) {
        fnRelease(pThis);
        return false;
    }

    uint32_t srvDesc[6];
    srvDesc[0] = depthSrvFormat;
    srvDesc[1] = (sampleCount > 1) ? 6u : 4u;
    srvDesc[2] = 0;
    srvDesc[3] = mipCount;
    srvDesc[4] = 0;
    srvDesc[5] = 1;
    if (GXDevice_CreateShaderResourceView(pDev, &pThis->m_pDepthSrv, pThis->m_pGnmTexture, srvDesc, pExtra1) != 0) {
        fnRelease(pThis);
        return false;
    }

    if (stencilSrvFormat != 0) {
        srvDesc[0] = stencilSrvFormat;
        if (GXDevice_CreateShaderResourceView(pDev, &pThis->m_pStencilSrv, pThis->m_pGnmTexture, srvDesc, pExtra1) != 0) {
            fnRelease(pThis);
            return false;
        }
    }

    if (pGlobal) {
        void** ppTrackerOwner = *reinterpret_cast<void***>(
            reinterpret_cast<uint8_t*>(pGlobal) + 0x650);
        if (ppTrackerOwner) {
            GXResourceMemoryTracker_RegisterDepthStencil(
                reinterpret_cast<GXResourceMemoryTracker*>(*ppTrackerOwner), pThis);
        }
    }
    return true;
}

// 0x219f680 (119 B): GXDepthStencilResource_Create
bool GXDepthStencilResource_Create(
    GXDepthStencilResource* pThis,
    uint32_t width,
    uint32_t height,
    uint32_t format,
    uint32_t mipLevels,
    const uint64_t* pSampleDesc,
    uint32_t miscFlags,
    void* pExtra1,
    void* pExtra2,
    uint64_t unk20)
{
    GXDepthStencilCreateDesc desc{};
    desc.width      = width;
    desc.height     = height;
    desc.format     = format;
    desc.mipLevels  = mipLevels;
    desc.sampleDesc = *pSampleDesc;
    desc.miscFlags  = miscFlags;
    desc.dsvFlags   = 0;
    desc.unk20      = unk20;
    return GXDepthStencilResource_CreateFromDesc(pThis, &desc, pExtra1, pExtra2);
}

// 0x219ef50 (475 B): GXDepthStencilResource_ReleaseViewsAndTexture
void GXDepthStencilResource_ReleaseViewsAndTexture(GXDepthStencilResource* pThis) {
    void* pGlobal = GXGlobalDevice_GetInstance();
    if (pGlobal) {
        void** ppTrackerOwner = *reinterpret_cast<void***>(
            reinterpret_cast<uint8_t*>(pGlobal) + 0x650);
        if (ppTrackerOwner) {
            uint8_t* pTracker = reinterpret_cast<uint8_t*>(*ppTrackerOwner);
            void** pBegin = *reinterpret_cast<void***>(pTracker + 0xB0);
            void** pEnd   = *reinterpret_cast<void***>(pTracker + 0xB8);
            for (void** it = pBegin; it != pEnd; ++it) {
                if (*it == pThis) {
                    std::memmove(it, it + 1, reinterpret_cast<uint8_t*>(pEnd) - reinterpret_cast<uint8_t*>(it + 1));
                    *reinterpret_cast<void***>(pTracker + 0xB8) = pEnd - 1;
                    auto fnGetByteSize = reinterpret_cast<uint32_t(*)(GXDepthStencilResource*)>(pThis->m_vptr[12]); // +0x60
                    uint32_t sz = fnGetByteSize(pThis);
                    uint32_t totalBytes = *reinterpret_cast<uint32_t*>(pTracker + 0xD4) - sz;
                    *reinterpret_cast<uint32_t*>(pTracker + 0xD4) = totalBytes;
                    *reinterpret_cast<float*>(pTracker + 0xD8) =
                        static_cast<float>(totalBytes) * (1.0f / 1048576.0f);
                    *reinterpret_cast<uint32_t*>(pTracker + 0xD0) -= 1;
                    break;
                }
            }
        }
    }

    if (pThis->m_ppDepthStencilViews) {
        uint32_t mipCount = pThis->m_pGnmTexture ? pThis->m_pGnmTexture->m_mipLevels : 0;
        for (uint32_t i = 0; i < mipCount; ++i) {
            if (pThis->m_ppReadOnlyDepthStencilViews && pThis->m_ppReadOnlyDepthStencilViews[i]) {
                void* pRoDsv = pThis->m_ppReadOnlyDepthStencilViews[i];
                void* pCreatorAlloc = CGObject_GetCreatorAllocator(pRoDsv);
                (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pRoDsv)))(pRoDsv);
                (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pCreatorAlloc) + 0x70))(pCreatorAlloc, pRoDsv);
                pThis->m_ppReadOnlyDepthStencilViews[i] = nullptr;
            }
            if (pThis->m_ppDepthStencilViews[i]) {
                void* pRwDsv = pThis->m_ppDepthStencilViews[i];
                void* pCreatorAlloc = CGObject_GetCreatorAllocator(pRwDsv);
                (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pRwDsv)))(pRwDsv);
                (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pCreatorAlloc) + 0x70))(pCreatorAlloc, pRwDsv);
                pThis->m_ppDepthStencilViews[i] = nullptr;
            }
        }

        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        if (pThis->m_ppReadOnlyDepthStencilViews) {
            void* pRawRo = *(pThis->m_ppReadOnlyDepthStencilViews - 2);
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, pRawRo);
            pThis->m_ppReadOnlyDepthStencilViews = nullptr;
        }
        if (pThis->m_ppDepthStencilViews) {
            void* pRawRw = *(pThis->m_ppDepthStencilViews - 2);
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, pRawRw);
            pThis->m_ppDepthStencilViews = nullptr;
        }
    }

    if (pThis->m_pStencilSrv) {
        void* pSrv = pThis->m_pStencilSrv;
        void* pCreatorAlloc = CGObject_GetCreatorAllocator(pSrv);
        (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pSrv)))(pSrv);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pCreatorAlloc) + 0x70))(pCreatorAlloc, pSrv);
        pThis->m_pStencilSrv = nullptr;
    }

    GXTexture2DResource_ReleaseGpuTexture(pThis);
}

// 0x219eeb0 (146 B): GXDepthStencilResource_BaseDtor
void GXDepthStencilResource_BaseDtor(GXDepthStencilResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6860ULL));
    GXDepthStencilResource_ReleaseViewsAndTexture(pThis);
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6a20ULL));
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    if (pThis->m_pColorScaleCBuf) {
        CGBufferGnm_Release(pThis->m_pColorScaleCBuf);
        pThis->m_pColorScaleCBuf = nullptr;
    }
}

// 0x219ee10 (145 B): GXDepthStencilResource_DeletingDtor
void GXDepthStencilResource_DeletingDtor(GXDepthStencilResource* pThis) {
    GXDepthStencilResource_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x219ed90 (116 B): GXDepthStencilResource_Construct
void GXDepthStencilResource_Construct(GXDepthStencilResource* pThis) {
    pThis->m_refCount                    = 0;
    pThis->m_clearValues[0]              = 1.0f;
    pThis->m_clearValues[1]              = 1.0f;
    pThis->m_clearValues[2]              = 1.0f;
    pThis->m_clearValues[3]              = 1.0f;
    pThis->m_pColorScaleCBuf             = nullptr;
    pThis->m_byteSize                    = 0;
    pThis->m_pUav                        = nullptr;
    pThis->m_pDepthSrv                   = nullptr;
    pThis->m_pGnmTexture                 = nullptr;
    pThis->m_colorScale[0]               = 1.0f;
    pThis->m_vptr                        = reinterpret_cast<void**>(GX_VMA(0x53b6860ULL));
    pThis->m_pStencilSrv                 = nullptr;
    pThis->m_ppReadOnlyDepthStencilViews = nullptr;
    pThis->m_ppDepthStencilViews         = nullptr;
    pThis->m_unk78                       = -1;
    pThis->m_unk7C                       = -1;
}

// ============================================================================
// 2. GXThreadLocalObjectPool, GXGammaRampLookupTable & GXDoubleBufferedRing
//    (0x219e5b0 .. 0x219ecf0, 14 functions)
// ============================================================================

struct GXThreadLocalObjectPool {
    void*     m_pMasterPool;         // +0x00
    void**    m_ppThreadObjectSlots; // +0x08 (workerCount * 20 pointers)
    uint32_t* m_pThreadReadIndices;  // +0x10 (workerCount uint32s)
    uint32_t* m_pThreadBaseOffsets;  // +0x18 (workerCount uint32s: 0, 20, 40...)
    uint32_t* m_pThreadValidCounts;  // +0x20 (workerCount uint32s)
    uint32_t  m_batchSizePerThread;  // +0x28 (20)
};

// 0x219e8d0 (47 B): GXThreadLocalObjectPool_Construct
void GXThreadLocalObjectPool_Construct(GXThreadLocalObjectPool* pThis) {
    pThis->m_batchSizePerThread  = 0;
    pThis->m_pThreadValidCounts  = nullptr;
    pThis->m_pThreadBaseOffsets  = nullptr;
    pThis->m_pThreadReadIndices  = nullptr;
    pThis->m_ppThreadObjectSlots = nullptr;
    pThis->m_pMasterPool         = nullptr;
}

// 0x219eb30 (201 B): GXThreadLocalObjectPool_Destroy
void GXThreadLocalObjectPool_Destroy(GXThreadLocalObjectPool* pThis) {
    if (void* p = pThis->m_pThreadValidCounts) {
        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, p);
        pThis->m_pThreadValidCounts = nullptr;
    }
    if (void* p = pThis->m_pThreadReadIndices) {
        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, p);
        pThis->m_pThreadReadIndices = nullptr;
    }
    if (void* p = pThis->m_pThreadBaseOffsets) {
        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, p);
        pThis->m_pThreadBaseOffsets = nullptr;
    }
    if (void* p = pThis->m_ppThreadObjectSlots) {
        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, p);
        pThis->m_ppThreadObjectSlots = nullptr;
    }
    if (void* pMaster = pThis->m_pMasterPool) {
        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        reinterpret_cast<void(*)(void*)>(GX_VMA(0xe313d0ULL))(pMaster);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x70))(pAlloc, pMaster);
        pThis->m_pMasterPool = nullptr;
    }
}

// 0x219e900 (546 B): GXThreadLocalObjectPool_Initialize
bool GXThreadLocalObjectPool_Initialize(GXThreadLocalObjectPool* pThis) {
    void** ppAllocGlobal = reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* pAlloc = *ppAllocGlobal;
    auto fnAllocAligned = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);

    void* pMaster = fnAllocAligned(pAlloc, 0x40, 8);
    if (pMaster) {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0xe31370ULL))(pMaster);
    }
    pThis->m_pMasterPool = pMaster;
    reinterpret_cast<void(*)(void*, uint32_t)>(GX_VMA(0xe31510ULL))(pMaster, 0x3e8u);

    void** ppDeviceGlobal = reinterpret_cast<void**>(GX_VMA(0x55406c8ULL));
    void* pGlobal = *ppDeviceGlobal;
    uint32_t workerCount = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pGlobal) + 0x280);

    pAlloc = *ppAllocGlobal;
    auto fnAlloc = *reinterpret_cast<void*(**)(void*, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x50);
    pThis->m_ppThreadObjectSlots = reinterpret_cast<void**>(fnAlloc(pAlloc, static_cast<size_t>(workerCount) * 160));
    if (!pThis->m_ppThreadObjectSlots) {
        GXThreadLocalObjectPool_Destroy(pThis);
        return false;
    }

    size_t u32ArrayBytes = static_cast<size_t>(workerCount) * 4;
    pAlloc = *ppAllocGlobal;
    fnAlloc = *reinterpret_cast<void*(**)(void*, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x50);
    pThis->m_pThreadReadIndices = reinterpret_cast<uint32_t*>(fnAlloc(pAlloc, u32ArrayBytes));
    if (!pThis->m_pThreadReadIndices) {
        GXThreadLocalObjectPool_Destroy(pThis);
        return false;
    }
    reinterpret_cast<void*(*)(void*, int, size_t)>(GX_VMA(0x2bbe688ULL))(pThis->m_pThreadReadIndices, 0, u32ArrayBytes);

    pAlloc = *ppAllocGlobal;
    fnAlloc = *reinterpret_cast<void*(**)(void*, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x50);
    pThis->m_pThreadBaseOffsets = reinterpret_cast<uint32_t*>(fnAlloc(pAlloc, u32ArrayBytes));
    if (!pThis->m_pThreadBaseOffsets) {
        GXThreadLocalObjectPool_Destroy(pThis);
        return false;
    }
    for (uint32_t i = 0; i < workerCount; ++i) {
        pThis->m_pThreadBaseOffsets[i] = i * 20;
    }

    pAlloc = *ppAllocGlobal;
    fnAlloc = *reinterpret_cast<void*(**)(void*, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x50);
    pThis->m_pThreadValidCounts = reinterpret_cast<uint32_t*>(fnAlloc(pAlloc, u32ArrayBytes));
    if (!pThis->m_pThreadValidCounts) {
        GXThreadLocalObjectPool_Destroy(pThis);
        return false;
    }
    reinterpret_cast<void*(*)(void*, int, size_t)>(GX_VMA(0x2bbe688ULL))(pThis->m_pThreadValidCounts, 0, u32ArrayBytes);

    pThis->m_batchSizePerThread = 20;

    pGlobal = *ppDeviceGlobal;
    uint32_t liveWorkers = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pGlobal) + 0x280);
    auto fnPopBatch = reinterpret_cast<void(*)(void*, void**, uint32_t)>(GX_VMA(0xe31820ULL));
    for (uint32_t i = 0; i < liveWorkers; ++i) {
        uint32_t baseOff = pThis->m_pThreadBaseOffsets[i];
        fnPopBatch(pThis->m_pMasterPool, &pThis->m_ppThreadObjectSlots[baseOff], pThis->m_batchSizePerThread);
    }
    return true;
}

// 0x219ec00 (94 B): GXThreadLocalObjectPool_ResetAllThreadCaches
void GXThreadLocalObjectPool_ResetAllThreadCaches(GXThreadLocalObjectPool* pThis) {
    sub_e316e0(pThis->m_pMasterPool);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint32_t workerCount = *reinterpret_cast<uint32_t*>(pGlobal + 0x280);
    for (uint32_t i = 0; i < workerCount; ++i) {
        uint32_t baseOffset = pThis->m_pThreadBaseOffsets[i];
        sub_e31820(
            pThis->m_pMasterPool,
            &pThis->m_ppThreadObjectSlots[baseOffset],
            pThis->m_batchSizePerThread);
    }
}

// 0x219ec60 (141 B): GXThreadLocalObjectPool_AllocateForCurrentThread
void* GXThreadLocalObjectPool_AllocateForCurrentThread(GXThreadLocalObjectPool* pThis) {
    uint8_t* pTlsCtx = *reinterpret_cast<uint8_t**>(gx_guest_tls_slot(0x53e4658ULL));
    uint32_t threadIdx = pTlsCtx ? *reinterpret_cast<uint32_t*>(pTlsCtx + 0xA0) : 0;
    uint32_t baseOffset = pThis->m_pThreadBaseOffsets[threadIdx];
    uint32_t readIdx    = pThis->m_pThreadReadIndices[threadIdx];
    uint32_t validCount = pThis->m_pThreadValidCounts[threadIdx];
    if (readIdx >= validCount) {
        validCount = sub_e31820(
            pThis->m_pMasterPool,
            &pThis->m_ppThreadObjectSlots[baseOffset],
            pThis->m_batchSizePerThread);
        pThis->m_pThreadValidCounts[threadIdx] = validCount;
        readIdx = 0;
    }
    if (readIdx < validCount) {
        void* pObj = pThis->m_ppThreadObjectSlots[baseOffset + readIdx];
        pThis->m_pThreadReadIndices[threadIdx] = readIdx + 1;
        return pObj;
    }
    return nullptr;
}

// 0x219ecf0 (8 B): GXThreadLocalObjectPool_GetTotalAllocatedCount
uint32_t GXThreadLocalObjectPool_GetTotalAllocatedCount(GXThreadLocalObjectPool* pThis) {
    return sub_e31990(pThis->m_pMasterPool);
}

// 0x219e700 (54 B): GXGammaRampLookupTable_InitIdentity
// Fills 1025 (0x401) RGB float triplets with linear identity ramp [0.0 .. 1.0].
void GXGammaRampLookupTable_InitIdentity(float* pTableBase) {
    float* pOut = pTableBase + 2; // starts at +0x08
    const float kInv1024 = 1.0f / 1024.0f;
    for (uint32_t i = 0; i < 0x401; ++i) {
        float val = static_cast<float>(i) * kInv1024;
        pOut[-2] = val;
        pOut[-1] = val;
        pOut[0]  = val;
        pOut += 3;
    }
}

// 0x219e740 (255 B): GXGammaRampLookupTable_ComputePowCurve
// Computes `clamp(powf(i / 1024.0f, expR/G/B), 0.0f, 1.0f)` for 1025 entries.
void GXGammaRampLookupTable_ComputePowCurve(const float* pGammaParams, float* pOutTableBase) {
    float expR = (pGammaParams[5] != 0.0f) ? (pGammaParams[2] / pGammaParams[5]) : 1.0f; // (+0x08) / (+0x14)
    float expG = (pGammaParams[6] != 0.0f) ? (pGammaParams[3] / pGammaParams[6]) : 1.0f; // (+0x0C) / (+0x18)
    float expB = (pGammaParams[7] != 0.0f) ? (pGammaParams[4] / pGammaParams[7]) : 1.0f; // (+0x10) / (+0x1C)

    float* pOut = pOutTableBase + 2;
    const float kInv1024 = 1.0f / 1024.0f;
    if (expR == expG && expG == expB) {
        for (uint32_t i = 0; i < 0x401; ++i) {
            float x = static_cast<float>(i) * kInv1024;
            float v = std::pow(x, expR);
            v = (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v);
            pOut[-2] = v;
            pOut[-1] = v;
            pOut[0]  = v;
            pOut += 3;
        }
    } else {
        for (uint32_t i = 0; i < 0x401; ++i) {
            float x = static_cast<float>(i) * kInv1024;
            float r = std::pow(x, expR);
            float g = std::pow(x, expG);
            float b = std::pow(x, expB);
            pOut[-2] = (r < 0.0f) ? 0.0f : ((r > 1.0f) ? 1.0f : r);
            pOut[-1] = (g < 0.0f) ? 0.0f : ((g > 1.0f) ? 1.0f : g);
            pOut[0]  = (b < 0.0f) ? 0.0f : ((b > 1.0f) ? 1.0f : b);
            pOut += 3;
        }
    }
}

// 0x219e650 (17 B): GXDoubleBufferedRing_ResetBackCount
void GXDoubleBufferedRing_ResetBackCount(uint8_t* pThis) {
    int64_t idx = *reinterpret_cast<int32_t*>(pThis + 0x24);
    *reinterpret_cast<uint32_t*>(pThis + idx * 16 + 0x08) = 0;
}

// 0x219e620 (33 B): GXDoubleBufferedRing_ResetBothCounts
void GXDoubleBufferedRing_ResetBothCounts(uint8_t* pThis) {
    int64_t idxBack  = *reinterpret_cast<int32_t*>(pThis + 0x24);
    *reinterpret_cast<uint32_t*>(pThis + idxBack * 16 + 0x08) = 0;
    int64_t idxFront = *reinterpret_cast<int32_t*>(pThis + 0x20);
    *reinterpret_cast<uint32_t*>(pThis + idxFront * 16 + 0x08) = 0;
}

// 0x219e600 (26 B): GXDoubleBufferedRing_SwapAndResetNewFrontCount
void GXDoubleBufferedRing_SwapAndResetNewFrontCount(uint8_t* pThis) {
    int32_t backIdx  = *reinterpret_cast<int32_t*>(pThis + 0x24);
    int32_t frontIdx = *reinterpret_cast<int32_t*>(pThis + 0x20);
    *reinterpret_cast<int32_t*>(pThis + 0x24) = frontIdx;
    *reinterpret_cast<int32_t*>(pThis + 0x20) = backIdx;
    *reinterpret_cast<uint32_t*>(pThis + static_cast<int64_t>(backIdx) * 16 + 0x08) = 0;
}

// 0x219e5f0 (3 B): GXDoubleBufferedRing_ReturnTrueStub
bool GXDoubleBufferedRing_ReturnTrueStub() { return true; }

// 0x219e5e0 (1 B): GXDoubleBufferedRing_NoOpStub
void GXDoubleBufferedRing_NoOpStub() {}

// 0x219e5b0 (44 B): GXDoubleBufferedRing_Construct
void GXDoubleBufferedRing_Construct(uint8_t* pThis) {
    *reinterpret_cast<uint64_t*>(pThis + 0x00) = 0;
    *reinterpret_cast<uint32_t*>(pThis + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(pThis + 0x10) = 0;
    *reinterpret_cast<uint32_t*>(pThis + 0x18) = 0;
    *reinterpret_cast<int32_t*>(pThis + 0x20)  = 0;
    *reinterpret_cast<int32_t*>(pThis + 0x24)  = 1;
}

// ============================================================================
// 3. GXDisplayOutputManager, GXRenderThreadJobDispatcher &
//    GXHeapAllocatorAdapter (0x219c8a0 .. 0x219e320, 38 functions)
// ============================================================================

// 0x219e320 (499 B): GXPropertyEnumVector_Reallocate24ByteEntries
void GXPropertyEnumVector_Reallocate24ByteEntries(uint8_t* pVec, size_t newCap) {
    void* pAlloc = *reinterpret_cast<void**>(pVec + 0x20);
    auto fnAlloc = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);
    uint8_t* pNewBuf = reinterpret_cast<uint8_t*>(fnAlloc(pAlloc, newCap * 24, 8));
    if (!pNewBuf) {
        if (*reinterpret_cast<uint32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0) {
            sub_2bbe978(reinterpret_cast<void*>(gx_guest_tls_slot(0x53e4320ULL)), 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    uint8_t* pOldBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
    uint8_t* pOldEnd   = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    size_t byteCount   = static_cast<size_t>(pOldEnd - pOldBegin);
    if (pNewBuf && byteCount != 0) {
        std::memcpy(pNewBuf, pOldBegin, byteCount);
    }

    if (pOldBegin) {
        void* pDealloc = *reinterpret_cast<void**>(pVec + 0x20);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pDealloc) + 0x70))(pDealloc, pOldBegin);
    }

    size_t count = byteCount / 24;
    *reinterpret_cast<uint8_t**>(pVec + 0x18) = pNewBuf ? (pNewBuf + newCap * 24) : nullptr;
    *reinterpret_cast<uint8_t**>(pVec + 0x10) = pNewBuf ? (pNewBuf + count * 24) : nullptr;
    *reinterpret_cast<uint8_t**>(pVec + 0x08) = pNewBuf;
}

// 0x219e220 (251 B): GXPointerVector_Reallocate8ByteEntriesA
void GXPointerVector_Reallocate8ByteEntriesA(uint8_t* pVec, size_t newCap) {
    void* pAlloc = *reinterpret_cast<void**>(pVec + 0x20);
    auto fnAlloc = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);
    uint64_t* pNewBuf = reinterpret_cast<uint64_t*>(fnAlloc(pAlloc, newCap * 8, 8));
    if (!pNewBuf) {
        if (*reinterpret_cast<uint32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0) {
            sub_2bbe978(reinterpret_cast<void*>(gx_guest_tls_slot(0x53e4320ULL)), 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    uint64_t* pOldBegin = *reinterpret_cast<uint64_t**>(pVec + 0x08);
    uint64_t* pOldEnd   = *reinterpret_cast<uint64_t**>(pVec + 0x10);
    if (pOldBegin != pOldEnd) {
        uint64_t* pDst = pNewBuf;
        for (uint64_t* pSrc = pOldBegin; pSrc != pOldEnd; ++pSrc, ++pDst) {
            if (pDst) *pDst = *pSrc;
        }
        pOldBegin = *reinterpret_cast<uint64_t**>(pVec + 0x08);
        pOldEnd   = *reinterpret_cast<uint64_t**>(pVec + 0x10);
    }

    size_t count = static_cast<size_t>(pOldEnd - pOldBegin);
    if (pOldBegin) {
        void* pDealloc = *reinterpret_cast<void**>(pVec + 0x20);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pDealloc) + 0x70))(pDealloc, pOldBegin);
    }
    *reinterpret_cast<uint64_t**>(pVec + 0x18) = pNewBuf + newCap;
    *reinterpret_cast<uint64_t**>(pVec + 0x10) = pNewBuf + count;
    *reinterpret_cast<uint64_t**>(pVec + 0x08) = pNewBuf;
}

// 0x219e120 (251 B): GXPointerVector_Reallocate8ByteEntriesB
void GXPointerVector_Reallocate8ByteEntriesB(uint8_t* pVec, size_t newCap) {
    GXPointerVector_Reallocate8ByteEntriesA(pVec, newCap);
}

// 0x219d9c0 (325 B): GXPairVector_GrowAndReallocate16ByteEntries
void GXPairVector_GrowAndReallocate16ByteEntries(uint8_t* pVec) {
    uint8_t* pCap = *reinterpret_cast<uint8_t**>(pVec + 0x18);
    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    if (pCap != pEnd) return;

    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
    size_t oldCap = static_cast<size_t>(pCap - pBegin) >> 4;
    size_t halfCap = oldCap >> 1;
    size_t newCap = (0x0FFFFFFFFFFFFFFFULL - halfCap >= oldCap) ? (oldCap + halfCap) : 0;
    if (newCap < oldCap + 1) newCap = oldCap + 1;

    void* pAlloc = *reinterpret_cast<void**>(pVec + 0x20);
    auto fnAlloc = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);
    uint8_t* pNewBuf = reinterpret_cast<uint8_t*>(fnAlloc(pAlloc, newCap << 4, 8));
    if (!pNewBuf) {
        if (*reinterpret_cast<uint32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0) {
            sub_2bbe978(reinterpret_cast<void*>(gx_guest_tls_slot(0x53e4320ULL)), 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
    pEnd   = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    if (pBegin != pEnd) {
        uint8_t* pDst = pNewBuf;
        for (uint8_t* pSrc = pBegin; pSrc != pEnd; pSrc += 16, pDst += 16) {
            if (pDst) {
                uint64_t v0 = *reinterpret_cast<uint64_t*>(pSrc + 0x00);
                uint64_t v1 = *reinterpret_cast<uint64_t*>(pSrc + 0x08);
                *reinterpret_cast<uint64_t*>(pDst + 0x08) = v1;
                *reinterpret_cast<uint64_t*>(pDst + 0x00) = v0;
            }
        }
        pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
        pEnd   = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    }

    size_t count = static_cast<size_t>(pEnd - pBegin) >> 4;
    if (pBegin) {
        void* pDealloc = *reinterpret_cast<void**>(pVec + 0x20);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pDealloc) + 0x70))(pDealloc, pBegin);
    }
    *reinterpret_cast<uint8_t**>(pVec + 0x18) = pNewBuf + (newCap << 4);
    *reinterpret_cast<uint8_t**>(pVec + 0x10) = pNewBuf + (count << 4);
    *reinterpret_cast<uint8_t**>(pVec + 0x08) = pNewBuf;
}

// 0x219d7b0 (376 B): GXDisplayOutputVector_GrowAndReallocate176ByteEntries
void GXDisplayOutputVector_GrowAndReallocate176ByteEntries(uint8_t* pVec) {
    uint8_t* pCap = *reinterpret_cast<uint8_t**>(pVec + 0x18);
    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    if (pCap != pEnd) return;

    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
    size_t oldCap = static_cast<size_t>(pCap - pBegin) / 0xB0;
    size_t halfCap = oldCap >> 1;
    size_t newCap = (0x01745D1745D1745DULL - halfCap >= oldCap) ? (oldCap + halfCap) : 0;
    if (newCap < oldCap + 1) newCap = oldCap + 1;

    void* pAlloc = *reinterpret_cast<void**>(pVec + 0x20);
    auto fnAlloc = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pAlloc) + 0x58);
    size_t byteCap = newCap * 0xB0;
    uint8_t* pNewBuf = reinterpret_cast<uint8_t*>(fnAlloc(pAlloc, byteCap, 8));
    if (!pNewBuf) {
        if (*reinterpret_cast<uint32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0) {
            sub_2bbe978(reinterpret_cast<void*>(gx_guest_tls_slot(0x53e4320ULL)), 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
    pEnd   = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    if (pBegin != pEnd) {
        uint8_t* pDst = pNewBuf;
        for (uint8_t* pSrc = pBegin; pSrc != pEnd; pSrc += 0xB0, pDst += 0xB0) {
            if (pDst) {
                std::memcpy(pDst, pSrc, 0xB0);
            }
        }
        pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
        pEnd   = *reinterpret_cast<uint8_t**>(pVec + 0x10);
    }

    size_t count = static_cast<size_t>(pEnd - pBegin) / 0xB0;
    if (pBegin) {
        void* pDealloc = *reinterpret_cast<void**>(pVec + 0x20);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pDealloc) + 0x70))(pDealloc, pBegin);
    }
    *reinterpret_cast<uint8_t**>(pVec + 0x18) = pNewBuf + byteCap;
    *reinterpret_cast<uint8_t**>(pVec + 0x10) = pNewBuf + count * 0xB0;
    *reinterpret_cast<uint8_t**>(pVec + 0x08) = pNewBuf;
}

// 0x219d9b0 & 0x219d9a0 (1 B): Job completion no-op callbacks
void GXRenderJob_NoOpCallbackB() {}
void GXRenderJob_NoOpCallbackA() {}

// 0x219d930 (108 B): GXDisplayOutputManager_UpdateGammaRampFromSource
void GXDisplayOutputManager_UpdateGammaRampFromSource(void* /*pUnused*/, void** ppManagerAndSource) {
    uint8_t* pMgr = reinterpret_cast<uint8_t*>(ppManagerAndSource[0]);
    void* pSource = ppManagerAndSource[1];
    (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pSource)))(pSource, pMgr + 0x10);
    pMgr[0x301C] = 1;
}

// 0x219d220 (122 B): GXDisplayOutputManager_PresentAllOutputs
// Iterates over registered 0xB0-byte display output entries, uploads the
// 0x300C-byte gamma ramp if dirty (+0x301C), and invokes Bloodborne's
// 30 FPS / 60 Hz frame-pacer (`GXFlipController_PresentAndWaitFramePacing` at 0x21b2fb0).
void GXDisplayOutputManager_PresentAllOutputs(uint8_t* pThis, uint32_t syncInterval) {
    uint8_t* pVec   = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pVec + 0x08);
    uint8_t* pEnd   = *reinterpret_cast<uint8_t**>(pVec + 0x10);

    for (uint8_t* it = pBegin; it != pEnd; it += 0xB0) {
        void* pFlipCtrl = *reinterpret_cast<void**>(it + 0x08);
        if (pThis[0x301C] != 0) {
            void* pDisplayOut = *reinterpret_cast<void**>(pFlipCtrl);
            CGDisplayOutput_SetGammaRamp(pDisplayOut, pThis + 0x10);
        }
        GXFlipController_PresentAndWaitFramePacing(pFlipCtrl, syncInterval);
    }
    pThis[0x301C] = 0;
}

// 0x219d2c0 (643 B): GXRenderThreadJobDispatcher_SubmitAllContextJobs
// Submits a begin-frame job, packages and submits command lists for all active
// GXDeviceContexts (`pGlobal+0x68`), and submits an end-frame job to `GXRenderJobQueue`.
void GXRenderThreadJobDispatcher_SubmitAllContextJobs() {
    uint8_t* pGlobal = reinterpret_cast<uint8_t*>(GXGlobalDevice_GetInstance());
    int64_t frameSlot = *reinterpret_cast<int32_t*>(pGlobal + 0x380);
    uint8_t* pJobQueue = *reinterpret_cast<uint8_t**>(pGlobal + frameSlot * 8 + 0x360);

    // 1. Allocate and enqueue Begin-Frame job packet (callback 0x219d9a0)
    {
        uint8_t* pLock = pJobQueue + 0xB0;
        (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
        uint8_t* pHdr = reinterpret_cast<uint8_t*>(sub_e45560(pJobQueue + 0x48, 0x1C, 4));
        uint8_t* pPayload = nullptr;
        if (pHdr) {
            pHdr[0] = 1;
            *reinterpret_cast<uint16_t*>(pHdr + 2) = 0x1C;
            uint32_t misalign = (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pHdr)) + 4) & 3u;
            uint8_t pad = misalign ? static_cast<uint8_t>(4 - misalign) : 0;
            pHdr[1] = pad;
            pPayload = pHdr + 4 + pad;

            int32_t curCount = *reinterpret_cast<int32_t*>(pJobQueue + 0x38);
            if (curCount < *reinterpret_cast<int32_t*>(pJobQueue + 0x3C)) {
                *reinterpret_cast<int32_t*>(pJobQueue + 0x38) = curCount + 1;
                uint8_t* pSlot = *reinterpret_cast<uint8_t**>(pJobQueue + 0x30) + static_cast<int64_t>(curCount) * 16;
                *reinterpret_cast<uint8_t**>(pSlot + 0x00) = pHdr;
                *reinterpret_cast<uint32_t*>(pSlot + 0x08) = 0;
            }
        }
        (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);

        if (pPayload) {
            *reinterpret_cast<void(**)()>(pPayload + 0x00) = &GXRenderJob_NoOpCallbackA;
            *reinterpret_cast<uint64_t*>(pPayload + 0x08)  = 1;
            *reinterpret_cast<uint64_t*>(pPayload + 0x10)  = 0;
        }
    }

    // 2. Package and submit command lists for each active GXDeviceContext
    uint32_t ctxCount = *reinterpret_cast<uint32_t*>(pGlobal + 0x80);
    for (uint32_t i = 0; i < ctxCount; ++i) {
        uint8_t* pCtx = (*reinterpret_cast<uint8_t***>(pGlobal + 0x68))[i];
        if (pCtx[0x2093] == 0) {
            int64_t slot = *reinterpret_cast<int32_t*>(pGlobal + 0x380);
            uint8_t* pCmdPool = *reinterpret_cast<uint8_t**>(pGlobal + slot * 8 + 0x350);
            uint8_t* pPoolLock = pCmdPool + 0x30;
            (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pPoolLock) + 0x18))(pPoolLock, ~0u);

            int64_t usedCount = *reinterpret_cast<int32_t*>(pCmdPool + 0x28);
            uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pCmdPool + 0x08);
            uint8_t* pEnd   = *reinterpret_cast<uint8_t**>(pCmdPool + 0x10);
            size_t capEntries = static_cast<size_t>(pEnd - pBegin) / 0xF0;
            uint8_t* pJobEntry = nullptr;
            if (static_cast<size_t>(usedCount) < capEntries) {
                *reinterpret_cast<int32_t*>(pCmdPool + 0x28) = static_cast<int32_t>(usedCount + 1);
                pJobEntry = pBegin + usedCount * 0xF0;
            }
            (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pPoolLock) + 0x28))(pPoolLock);

            if (pJobEntry) {
                sub_e47930(pJobEntry, pCtx);
                int64_t qSlot = *reinterpret_cast<int32_t*>(pGlobal + 0x380);
                void* pActiveQueue = *reinterpret_cast<void**>(pGlobal + qSlot * 8 + 0x360);
                GXRenderJobQueue_SubmitJob(pActiveQueue, pJobEntry, 0);
            }
        }
    }

    // 3. Allocate and enqueue End-Frame job packet (callback 0x219d9b0)
    {
        int64_t endSlot = *reinterpret_cast<int32_t*>(pGlobal + 0x380);
        uint8_t* pEndQueue = *reinterpret_cast<uint8_t**>(pGlobal + endSlot * 8 + 0x360);
        uint8_t* pLock = pEndQueue + 0xB0;
        (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
        uint8_t* pHdr = reinterpret_cast<uint8_t*>(sub_e45560(pEndQueue + 0x48, 0x1C, 4));
        uint8_t* pPayload = nullptr;
        if (pHdr) {
            pHdr[0] = 1;
            *reinterpret_cast<uint16_t*>(pHdr + 2) = 0x1C;
            uint32_t misalign = (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pHdr)) + 4) & 3u;
            uint8_t pad = misalign ? static_cast<uint8_t>(4 - misalign) : 0;
            pHdr[1] = pad;
            pPayload = pHdr + 4 + pad;

            int32_t curCount = *reinterpret_cast<int32_t*>(pEndQueue + 0x38);
            if (curCount < *reinterpret_cast<int32_t*>(pEndQueue + 0x3C)) {
                *reinterpret_cast<int32_t*>(pEndQueue + 0x38) = curCount + 1;
                uint8_t* pSlot = *reinterpret_cast<uint8_t**>(pEndQueue + 0x30) + static_cast<int64_t>(curCount) * 16;
                *reinterpret_cast<uint8_t**>(pSlot + 0x00) = pHdr;
                *reinterpret_cast<uint32_t*>(pSlot + 0x08) = 0;
            }
        }
        (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);

        if (pPayload) {
            *reinterpret_cast<void(**)()>(pPayload + 0x00) = &GXRenderJob_NoOpCallbackB;
            *reinterpret_cast<uint64_t*>(pPayload + 0x08)  = 1;
            *reinterpret_cast<uint64_t*>(pPayload + 0x10)  = 0;
        }
    }
}

// ----------------------------------------------------------------------------
// GXHeapAllocatorAdapter (0x219c8a0 .. 0x219d550 & 0x219e040)
// Thread-safe DLAllocator wrapper around an internal DLHeap (`+0x28`) and
// synchronization lock (`+0xE8`).
// ----------------------------------------------------------------------------

// 0x219e040 (211 B): GXHeapSubAllocator_ReallocateAligned16
void* GXHeapSubAllocator_ReallocateAligned16(uint8_t* pSubHeap, void* pOldPtr, size_t newSize) {
    uint8_t* pLock = pSubHeap + 0xC0;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    void* pResult = nullptr;
    if (pOldPtr) {
        if (newSize == 0) {
            DLHeap_Free(pSubHeap, pOldPtr);
        } else {
            size_t oldSize = DLHeap_GetAllocationSize(pSubHeap, pOldPtr);
            if (oldSize >= newSize) {
                pResult = pOldPtr;
            } else {
                pResult = DLHeap_Allocate(pSubHeap, newSize, 0x10);
                if (pResult) {
                    sub_261a1b0(pResult, pOldPtr, oldSize);
                    DLHeap_Free(pSubHeap, pOldPtr);
                }
            }
        }
    } else if (newSize != 0) {
        pResult = DLHeap_Allocate(pSubHeap, newSize, 0x10);
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219d550 (596 B): GXHeapInternalState_DestroyAndFreeArena
void GXHeapInternalState_DestroyAndFreeArena(uint8_t* pState) {
    uint8_t* pHeapBase = pState + 0x28;
    if (pState[0x8F] != 0) {
        sub_207b8c0(pState + 0x18);
        if (pState[0x8F] != 0) {
            uintptr_t alignPad = (0ULL - reinterpret_cast<uintptr_t>(pHeapBase)) & 7ULL;
            uint8_t* pAlignedHeap = pState + alignPad + 0x28;
            void* pArena = *reinterpret_cast<void**>(pState + alignPad + 0x80);
            if (pArena) {
                sub_2084ae0(pAlignedHeap, pArena);
            }
            uintptr_t highOff = alignPad | 0x48ULL;
            *reinterpret_cast<uint64_t*>(pAlignedHeap + 0x08) = 0;
            *reinterpret_cast<uint64_t*>(pAlignedHeap + 0x00) = 0;
            *reinterpret_cast<uint64_t*>(pState + highOff + 0x38) = 0;
            *reinterpret_cast<uint64_t*>(pState + highOff + 0x30) = 0;
            *reinterpret_cast<uint64_t*>(pState + highOff + 0x28) = 0;
            pState[0x8F] = 0;
        }
        std::memset(pState + 0x90, 0, 0x48); // clears +0x90..+0xD0

        uint8_t* pLock = pState + 0xE8;
        bool hasSema = (*reinterpret_cast<bool(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x10))(pLock);
        if (hasSema) {
            int err = sub_2bbfee8(pState + 0xF0);
            if (err != static_cast<int>(0x80020010u)) {
                if (err != 0) {
                    if (err == static_cast<int>(0x80020016u)) {
                        sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x55, reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                    }
                    sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x57, reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
                }
                pState[0xF8] = 0;
            }
        }

        void* pParentAlloc = *reinterpret_cast<void**>(pState + 0x00);
        if (pParentAlloc) {
            void* pBackingMem = *reinterpret_cast<void**>(pState + 0x10);
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pParentAlloc) + 0x70))(pParentAlloc, pBackingMem);
            *reinterpret_cast<void**>(pState + 0x00) = nullptr;
            *reinterpret_cast<void**>(pState + 0x10) = nullptr;
        }
    }

    *reinterpret_cast<void**>(pState + 0xE8) = reinterpret_cast<void*>(GX_VMA(0x53b2fa0ULL));
    if (pState[0xF8] != 0) {
        int err = sub_2bbfee8(pState + 0xF0);
        if (err != static_cast<int>(0x80020010u)) {
            if (err != 0) {
                if (err == static_cast<int>(0x80020016u)) {
                    sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x55, reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                }
                sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x57, reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
            }
            pState[0xF8] = 0;
        }
    }
}

// 0x219d2b0 (5 B): GXHeapAllocatorAdapter_DeletingDtor
void GXHeapAllocatorAdapter_DeletingDtor(void* pThis) {
    sub_2bbe588(pThis);
}

// 0x219d2a0 (1 B): GXHeapAllocatorAdapter_BaseDtor
void GXHeapAllocatorAdapter_BaseDtor() {}

// 0x219d1e0 (56 B): GXHeapAllocatorAdapter_LockAndUnlockBarrier
int GXHeapAllocatorAdapter_LockAndUnlockBarrier(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return 0;
}

// 0x219d150 (143 B): GXHeapAllocatorAdapter_ReleaseThreadSemaphore
void GXHeapAllocatorAdapter_ReleaseThreadSemaphore(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    bool ok = (*reinterpret_cast<bool(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x10))(pLock);
    if (!ok) {
        sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493bcaaULL)), 0x99, reinterpret_cast<const char*>(GX_VMA(0x493bd11ULL)));
    }
    int err = sub_2bbff08(pState + 0xF0);
    if (err != static_cast<int>(0x80020001u) && err != 0) {
        if (err == static_cast<int>(0x80020016u)) {
            sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0xC0, reinterpret_cast<const char*>(GX_VMA(0x493c2f3ULL)));
        } else {
            sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0xC5, reinterpret_cast<const char*>(GX_VMA(0x493c34dULL)));
        }
    }
}

// 0x219d130 (17 B): GXHeapAllocatorAdapter_AcquireThreadSemaphore
void GXHeapAllocatorAdapter_AcquireThreadSemaphore(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    sub_207ee90(pState + 0xE8);
}

// 0x219d090 (158 B): GXHeapAllocatorAdapter_IterateNextBlock
bool GXHeapAllocatorAdapter_IterateNextBlock(uint8_t* pThis, uint64_t* pIterState) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    bool ok = false;
    if (pIterState) {
        uint8_t* pNode = reinterpret_cast<uint8_t*>(pIterState[0]);
        if (!pNode) {
            pNode = *reinterpret_cast<uint8_t**>(pState + 0xA0);
            pIterState[0] = reinterpret_cast<uintptr_t>(pNode);
        }
        if (pNode != pState + 0x90) {
            uint8_t* pNext = *reinterpret_cast<uint8_t**>(pNode + 0x10);
            pIterState[0] = reinterpret_cast<uintptr_t>(pNext);
            uint64_t blockStart = *reinterpret_cast<uint64_t*>(pNode + 0x00);
            uint64_t blockSize  = *reinterpret_cast<uint64_t*>(pNext + 0x00) - blockStart;
            pIterState[1] = blockStart;
            pIterState[2] = blockStart;
            pIterState[3] = blockSize;
            pIterState[4] = blockSize;
            *reinterpret_cast<uint8_t*>(pIterState + 5) = (*reinterpret_cast<uint64_t*>(pNode + 0x18) != 0) ? 1 : 0;
            ok = true;
        }
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return ok;
}

// 0x219cff0 (156 B): GXHeapAllocatorAdapter_ContainsAddress
bool GXHeapAllocatorAdapter_ContainsAddress(uint8_t* pThis, const void* pAddr) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    uintptr_t addr = reinterpret_cast<uintptr_t>(pAddr);
    bool found = false;
    if (addr >= *reinterpret_cast<uintptr_t*>(pState + 0xC8) &&
        addr <  *reinterpret_cast<uintptr_t*>(pState + 0xD0)) {
        for (uint8_t* pNode = *reinterpret_cast<uint8_t**>(pState + 0xB8);
             pNode != nullptr;
             pNode = *reinterpret_cast<uint8_t**>(pNode + 0x20)) {
            if (*reinterpret_cast<uintptr_t*>(pNode) == addr) {
                found = true;
                break;
            }
        }
        if (!found) {
            for (uint8_t* pNode = *reinterpret_cast<uint8_t**>(pState + 0xC0);
                 pNode != nullptr;
                 pNode = *reinterpret_cast<uint8_t**>(pNode + 0x20)) {
                if (*reinterpret_cast<uintptr_t*>(pNode) == addr) {
                    found = true;
                    break;
                }
            }
        }
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return found;
}

// 0x219cf90 (81 B): GXHeapAllocatorAdapter_CheckIntegrity
bool GXHeapAllocatorAdapter_CheckIntegrity(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    bool ok = DLHeap_CheckIntegrity(pState + 0x28);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return ok;
}

// 0x219cf40 (79 B): GXHeapAllocatorAdapter_FreeAligned
void GXHeapAllocatorAdapter_FreeAligned(uint8_t* pThis, void* pPtr) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    DLHeap_FreeAligned(pState + 0x28, pPtr);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
}

// 0x219ce20 (283 B): GXHeapAllocatorAdapter_ReallocateAligned
void* GXHeapAllocatorAdapter_ReallocateAligned(uint8_t* pThis, void* pOldPtr, size_t newSize, size_t align) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    uint8_t* pHeap  = pState + 0x28;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    void* pResult = nullptr;
    size_t actualAlign = (align > 0x10) ? align : 0x10;
    bool isPow2 = ((align & (align - 1)) == 0);

    if (pOldPtr) {
        if (newSize == 0) {
            DLHeap_FreeAligned(pHeap, pOldPtr);
        } else {
            size_t oldSize = DLHeap_GetAllocationSize(pHeap, pOldPtr);
            if (oldSize >= newSize) {
                pResult = pOldPtr;
            } else if (isPow2) {
                pResult = DLHeap_AllocateAligned(pHeap, newSize, actualAlign);
                if (pResult) {
                    sub_261a1b0(pResult, pOldPtr, oldSize);
                    DLHeap_FreeAligned(pHeap, pOldPtr);
                }
            }
        }
    } else if (newSize != 0 && isPow2) {
        pResult = DLHeap_AllocateAligned(pHeap, newSize, actualAlign);
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219ce10 (13 B): GXHeapAllocatorAdapter_ReallocateAligned16ThunkA
void* GXHeapAllocatorAdapter_ReallocateAligned16ThunkA(uint8_t* pThis, void* pOldPtr, size_t newSize) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    return GXHeapSubAllocator_ReallocateAligned16(pState + 0x28, pOldPtr, newSize);
}

// 0x219cd90 (118 B): GXHeapAllocatorAdapter_AllocateAligned
void* GXHeapAllocatorAdapter_AllocateAligned(uint8_t* pThis, size_t size, size_t align) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    void* pResult = nullptr;
    if ((align & (align - 1)) == 0) {
        size_t actualAlign = (align > 0x10) ? align : 0x10;
        pResult = DLHeap_AllocateAligned(pState + 0x28, size, actualAlign);
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219cd30 (91 B): GXHeapAllocatorAdapter_AllocateDefaultAligned16
void* GXHeapAllocatorAdapter_AllocateDefaultAligned16(uint8_t* pThis, size_t size) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    void* pResult = DLHeap_AllocateAligned(pState + 0x28, size, 0x10);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219cd10 (26 B): GXHeapAllocatorAdapter_LogUnsupportedOperation
void GXHeapAllocatorAdapter_LogUnsupportedOperation() {
    sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x49400ceULL)), 0x1CC, reinterpret_cast<const char*>(GX_VMA(0x494013dULL)));
}

// 0x219ccc0 (79 B): GXHeapAllocatorAdapter_Free
void GXHeapAllocatorAdapter_Free(uint8_t* pThis, void* pPtr) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    DLHeap_Free(pState + 0x28, pPtr);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
}

// 0x219cba0 (283 B): GXHeapAllocatorAdapter_Reallocate
void* GXHeapAllocatorAdapter_Reallocate(uint8_t* pThis, void* pOldPtr, size_t newSize, size_t align) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    uint8_t* pHeap  = pState + 0x28;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    void* pResult = nullptr;
    size_t actualAlign = (align > 0x10) ? align : 0x10;
    bool isPow2 = ((align & (align - 1)) == 0);

    if (pOldPtr) {
        if (newSize == 0) {
            DLHeap_Free(pHeap, pOldPtr);
        } else {
            size_t oldSize = DLHeap_GetAllocationSize(pHeap, pOldPtr);
            if (oldSize >= newSize) {
                pResult = pOldPtr;
            } else if (isPow2) {
                pResult = DLHeap_Allocate(pHeap, newSize, actualAlign);
                if (pResult) {
                    sub_261a1b0(pResult, pOldPtr, oldSize);
                    DLHeap_Free(pHeap, pOldPtr);
                }
            }
        }
    } else if (newSize != 0 && isPow2) {
        pResult = DLHeap_Allocate(pHeap, newSize, actualAlign);
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219cb90 (13 B): GXHeapAllocatorAdapter_ReallocateAligned16ThunkB
void* GXHeapAllocatorAdapter_ReallocateAligned16ThunkB(uint8_t* pThis, void* pOldPtr, size_t newSize) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    return GXHeapSubAllocator_ReallocateAligned16(pState + 0x28, pOldPtr, newSize);
}

// 0x219cb10 (118 B): GXHeapAllocatorAdapter_AllocateWithAlign
void* GXHeapAllocatorAdapter_AllocateWithAlign(uint8_t* pThis, size_t size, size_t align) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    void* pResult = nullptr;
    if ((align & (align - 1)) == 0) {
        size_t actualAlign = (align > 0x10) ? align : 0x10;
        pResult = DLHeap_Allocate(pState + 0x28, size, actualAlign);
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219cab0 (91 B): GXHeapAllocatorAdapter_Allocate
void* GXHeapAllocatorAdapter_Allocate(uint8_t* pThis, size_t size) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    void* pResult = DLHeap_Allocate(pState + 0x28, size, 0x10);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return pResult;
}

// 0x219ca50 (86 B): GXHeapAllocatorAdapter_GetAllocationSize
size_t GXHeapAllocatorAdapter_GetAllocationSize(uint8_t* pThis, const void* pPtr) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    size_t sz = DLHeap_GetAllocationSize(pState + 0x28, pPtr);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return sz;
}

// 0x219ca00 (73 B): GXHeapAllocatorAdapter_GetAllocatedBlockCount
size_t GXHeapAllocatorAdapter_GetAllocatedBlockCount(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    size_t count = *reinterpret_cast<size_t*>(pState + 0xD8);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return count;
}

// 0x219c950 (161 B): GXHeapAllocatorAdapter_GetMaxContiguousFreeSize
size_t GXHeapAllocatorAdapter_GetMaxContiguousFreeSize(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);

    size_t maxFree = 0;
    uint8_t* pSentinel = pState + 0x90;
    for (uint8_t* pNode = *reinterpret_cast<uint8_t**>(pState + 0xB0);
         pNode != pSentinel;
         pNode = *reinterpret_cast<uint8_t**>(pNode + 0x20)) {
        uint8_t* pNext = *reinterpret_cast<uint8_t**>(pNode + 0x10);
        if (!pNext) {
            sub_20b55b0(
                reinterpret_cast<const char*>(GX_VMA(0x493c8e8ULL)),
                0x310,
                reinterpret_cast<const char*>(GX_VMA(0x493cb05ULL)));
        }
        size_t sz = *reinterpret_cast<size_t*>(pNext) - *reinterpret_cast<size_t*>(pNode);
        if (sz > maxFree) maxFree = sz;
    }

    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return maxFree;
}

// 0x219c900 (73 B): GXHeapAllocatorAdapter_GetTotalFreeSize
size_t GXHeapAllocatorAdapter_GetTotalFreeSize(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    uint8_t* pLock  = pState + 0xE8;
    (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x18))(pLock, ~0u);
    size_t freeBytes = *reinterpret_cast<size_t*>(pState + 0xE0);
    (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pLock) + 0x28))(pLock);
    return freeBytes;
}

// 0x219c8e0 (19 B): GXHeapAllocatorAdapter_GetTotalArenaSize
size_t GXHeapAllocatorAdapter_GetTotalArenaSize(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    return *reinterpret_cast<size_t*>(pState + 0xD0) - *reinterpret_cast<size_t*>(pState + 0xC8);
}

// 0x219c8d0 (10 B): GXHeapAllocatorAdapter_GetHeapTypeTag
uint32_t* GXHeapAllocatorAdapter_GetHeapTypeTag(uint32_t* outTag) {
    *outTag = 0x33;
    return outTag;
}

// 0x219c8c0 (8 B): GXHeapAllocatorAdapter_GetHeapId
uint32_t GXHeapAllocatorAdapter_GetHeapId(uint8_t* pThis) {
    uint8_t* pState = *reinterpret_cast<uint8_t**>(pThis + 0x08);
    return *reinterpret_cast<uint32_t*>(pState + 0x08);
}

// 0x219c8b0 (5 B): GXHeapAllocatorAdapter_DeletingDtorAlt
void GXHeapAllocatorAdapter_DeletingDtorAlt(void* pThis) {
    sub_2bbe588(pThis);
}

// 0x219c8a0 (1 B): GXHeapAllocatorAdapter_BaseDtorAlt
void GXHeapAllocatorAdapter_BaseDtorAlt() {}

// ============================================================================
// 4. GXDebugPropertyNodes (0x219c0e0 .. 0x219c89e & 0x219db10 .. 0x219dee0,
//    23 functions)
// ============================================================================

struct GXIntrusiveRefObject {
    void**   m_vptr;     // +0x00
    uint32_t m_refFlags; // +0x08 (bits 0..14: strongCount, bits 15..29: weakCount, bit 30: aliveFlag)
    uint32_t m_flags0C;  // +0x0C
};

static inline void GXIntrusiveRefObject_ReleaseStrongAndWeak(GXIntrusiveRefObject* pObj) {
    if (!pObj) return;
    uint32_t oldVal = pObj->m_refFlags;
    uint32_t newStrong = (oldVal - 1) & 0x7FFFu;
    uint32_t updated = (oldVal & 0xFFFF8000u) | newStrong;
    pObj->m_refFlags = updated;
    if (newStrong == 0 && (oldVal & 0x40000000u)) {
        auto fnDestroy = reinterpret_cast<void(*)(GXIntrusiveRefObject*)>(pObj->m_vptr[3]); // +0x18
        fnDestroy(pObj);
        updated = pObj->m_refFlags & 0xBFFFFFFFu;
        pObj->m_refFlags = updated;
    }

    uint32_t newWeak = ((updated >> 15) - 1) & 0x7FFFu;
    pObj->m_refFlags = (updated & 0xC0007FFFu) | (newWeak << 15);
    if (newWeak == 0) {
        pObj->m_refFlags = updated | 0x3FFF8000u;
        auto fnDelete = reinterpret_cast<void(*)(GXIntrusiveRefObject*)>(pObj->m_vptr[4]); // +0x20
        fnDelete(pObj);
    }
}

// 0x219dee0 (345 B): GXDebugEnumBindingNode_BaseDtor
void GXDebugEnumBindingNode_BaseDtor(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6720ULL));
    if (auto* pObj = *reinterpret_cast<GXIntrusiveRefObject**>(pThis + 0x30)) {
        GXIntrusiveRefObject_ReleaseStrongAndWeak(pObj);
        *reinterpret_cast<void**>(pThis + 0x30) = nullptr;
    }
    *reinterpret_cast<void**>(pThis + 0x28) = nullptr;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2351f90ULL))(pThis);
    pThis[0x0B] &= 0xBFu;
    if (auto* pObj2 = *reinterpret_cast<GXIntrusiveRefObject**>(pThis + 0x30)) {
        GXIntrusiveRefObject_ReleaseStrongAndWeak(pObj2);
        *reinterpret_cast<void**>(pThis + 0x30) = nullptr;
    }
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2351d90ULL))(pThis);
}

// 0x219de00 (219 B): GXDebugPropertyHandleNode_BaseDtor
void GXDebugPropertyHandleNode_BaseDtor(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6490ULL));
    if (void* pChild = *reinterpret_cast<void**>(pThis + 0x10)) {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x23286b0ULL))(pChild);
        if (auto* pObj = *reinterpret_cast<GXIntrusiveRefObject**>(pThis + 0x10)) {
            GXIntrusiveRefObject_ReleaseStrongAndWeak(pObj);
            *reinterpret_cast<void**>(pThis + 0x10) = nullptr;
        }
    }
    uint32_t newFlags = *reinterpret_cast<uint32_t*>(pThis + 0x08) & 0xBFFFFFFFu;
    *reinterpret_cast<uint32_t*>(pThis + 0x08) = newFlags;
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b93b0ULL));
    *reinterpret_cast<uint32_t*>(pThis + 0x08) = newFlags;
}

// 0x219db10 (738 B): GXDebugPropertyTree_RegisterBytePropertyNode
void GXDebugPropertyTree_RegisterBytePropertyNode(uint8_t* pTree, const uint64_t* pNamePair, uint8_t* pTargetByte) {
    void* pPropAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    auto fnAlloc = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pPropAlloc) + 0x58);
    uint8_t* pNode = reinterpret_cast<uint8_t*>(fnAlloc(pPropAlloc, 0x88, 8));
    if (!pNode) return;

    void** ppStackEnd = *reinterpret_cast<void***>(pTree + 0x10);
    void* pParentDesc = *(ppStackEnd - 1);

    uint64_t cStrPtr = pNamePair[1];
    uint8_t* pRefStr = nullptr;
    if (cStrPtr == 0) {
        pRefStr = reinterpret_cast<uint8_t*>(pNamePair[0]);
        if (pRefStr) {
            *reinterpret_cast<uint32_t*>(pRefStr + 0x38) += 1;
        }
    }

    uint64_t localPair[2] = { 0, 0 };
    if (cStrPtr != 0) {
        localPair[1] = cStrPtr;
        sub_23586d0(pNode, pParentDesc, localPair, 1);
        localPair[1] = 0;
    } else if (pRefStr) {
        localPair[0] = reinterpret_cast<uintptr_t>(pRefStr);
        *reinterpret_cast<uint32_t*>(pRefStr + 0x38) += 1;
        sub_23586d0(pNode, pParentDesc, localPair, 1);
        localPair[1] = 0;
        if (--(*reinterpret_cast<uint32_t*>(pRefStr + 0x38)) == 0) {
            void* pStrNodeAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540e10ULL));
            if (*reinterpret_cast<uint64_t*>(pRefStr + 0x20) >= 8) {
                void* pBufAlloc = *reinterpret_cast<void**>(pRefStr + 0x28);
                void* pStrData  = *reinterpret_cast<void**>(pRefStr + 0x08);
                (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pBufAlloc) + 0x70))(pBufAlloc, pStrData);
            }
            *reinterpret_cast<uint64_t*>(pRefStr + 0x20) = 7;
            *reinterpret_cast<uint64_t*>(pRefStr + 0x18) = 0;
            *reinterpret_cast<uint16_t*>(pRefStr + 0x08) = 0;
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pStrNodeAlloc) + 0x70))(pStrNodeAlloc, pRefStr);
        }
        localPair[0] = 0;
    } else {
        sub_23586d0(pNode, pParentDesc, localPair, 1);
        localPair[1] = 0;
    }

    *reinterpret_cast<void**>(pNode + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b64e0ULL));
    *reinterpret_cast<uint8_t**>(pNode + 0x78) = pTargetByte;

    uint32_t refFlags = *reinterpret_cast<uint32_t*>(pNode + 0x08);
    refFlags = (refFlags & 0xFFFF8000u) | ((refFlags + 1) & 0x7FFFu);
    refFlags = (refFlags & 0xC0007FFFu) | ((refFlags + 0x8000u) & 0x3FFF8000u);
    *reinterpret_cast<uint32_t*>(pNode + 0x08) = refFlags;

    if (pRefStr) {
        if (--(*reinterpret_cast<uint32_t*>(pRefStr + 0x38)) == 0) {
            void* pStrNodeAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540e10ULL));
            if (*reinterpret_cast<uint64_t*>(pRefStr + 0x20) >= 8) {
                void* pBufAlloc = *reinterpret_cast<void**>(pRefStr + 0x28);
                void* pStrData  = *reinterpret_cast<void**>(pRefStr + 0x08);
                (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pBufAlloc) + 0x70))(pBufAlloc, pStrData);
            }
            *reinterpret_cast<uint64_t*>(pRefStr + 0x20) = 7;
            *reinterpret_cast<uint64_t*>(pRefStr + 0x18) = 0;
            *reinterpret_cast<uint16_t*>(pRefStr + 0x08) = 0;
            (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pStrNodeAlloc) + 0x70))(pStrNodeAlloc, pRefStr);
        }
    }

    void* pWrapAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    auto fnWrapAlloc = *reinterpret_cast<void*(**)(void*, size_t, size_t)>(*reinterpret_cast<uintptr_t*>(pWrapAlloc) + 0x58);
    uint8_t* pWrapper = reinterpret_cast<uint8_t*>(fnWrapAlloc(pWrapAlloc, 0x18, 8));
    if (pWrapper) {
        *reinterpret_cast<uint32_t*>(pWrapper + 0x08) =
            (*reinterpret_cast<uint32_t*>(pWrapper + 0x08) & 0x80000000u) | 0x40000000u;
        *reinterpret_cast<void**>(pWrapper + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6490ULL));
        *reinterpret_cast<uint8_t**>(pWrapper + 0x10) = pNode;

        uint32_t nFlags = *reinterpret_cast<uint32_t*>(pNode + 0x08);
        nFlags = (nFlags & 0xFFFF8000u) | ((nFlags + 1) & 0x7FFFu);
        nFlags = (nFlags & 0xC0007FFFu) | ((nFlags + 0x8000u) & 0x3FFF8000u);
        *reinterpret_cast<uint32_t*>(pNode + 0x08) = nFlags;

        void** ppCurEnd = *reinterpret_cast<void***>(pTree + 0x10);
        sub_2367220(*(ppCurEnd - 1), pWrapper);
    }

    GXIntrusiveRefObject_ReleaseStrongAndWeak(reinterpret_cast<GXIntrusiveRefObject*>(pNode));
}

// 0x219c850 (79 B): GXDebugEnumBindingNode_OnSelectEnumIndex
void GXDebugEnumBindingNode_OnSelectEnumIndex(uint8_t* pThis, uint32_t selectedIndex) {
    if (pThis[0x0C] & 0x01) return;
    uint8_t* pEnumList = *reinterpret_cast<uint8_t**>(pThis + 0x30);
    uint8_t* pBegin    = *reinterpret_cast<uint8_t**>(pEnumList + 0x18);
    uint8_t* pEnd      = *reinterpret_cast<uint8_t**>(pEnumList + 0x20);
    size_t count = static_cast<size_t>(pEnd - pBegin) / 24;
    if (selectedIndex < count) {
        uint8_t val = pBegin[selectedIndex * 24 + 0x10];
        pThis[0x20] = val;
        **reinterpret_cast<uint8_t**>(pThis + 0x28) = val;
        void* pListener = *reinterpret_cast<void**>(pThis + 0x10);
        if (pListener) {
            (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(pListener) + 0x28))(pListener);
        }
    }
}

// 0x219c830 (23 B): GXDebugEnumBindingNode_DetachTargetPointer
void GXDebugEnumBindingNode_DetachTargetPointer(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x28) = nullptr;
    void* pChild = *reinterpret_cast<void**>(pThis + 0x18);
    if (pChild) {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x23286b0ULL))(pChild);
    }
}

// 0x219c7c0 (103 B): GXDebugEnumBindingNode_SyncSelectionFromTargetByte
void GXDebugEnumBindingNode_SyncSelectionFromTargetByte(uint8_t* pThis) {
    uint8_t* pTarget = *reinterpret_cast<uint8_t**>(pThis + 0x28);
    if (!pTarget) return;
    uint8_t val = *pTarget;
    if (val == pThis[0x20]) return;
    pThis[0x20] = val;

    uint8_t* pOptionList = *reinterpret_cast<uint8_t**>(pThis + 0x30);
    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pOptionList + 0x18);
    uint8_t* pEnd   = *reinterpret_cast<uint8_t**>(pOptionList + 0x20);
    uint32_t count  = static_cast<uint32_t>(static_cast<uint64_t>(pEnd - pBegin) / 0x18ULL);

    uint32_t foundIdx = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < count; ++i) {
        if (pBegin[i * 0x18 + 0x10] == val) {
            foundIdx = i;
            break;
        }
    }

    uint8_t* pOwner = *reinterpret_cast<uint8_t**>(pThis + 0x18);
    void* pWidget = *reinterpret_cast<void**>(pOwner + 0x110);
    reinterpret_cast<void(*)(void*, uint32_t, uint32_t)>(GX_VMA(0xf48fe0ULL))(pWidget, foundIdx, 0u);
}

// 0x219c710 (176 B): GXDebugEnumBindingNode_ReleaseEnumListAndReset
void GXDebugEnumBindingNode_ReleaseEnumListAndReset(uint8_t* pThis) {
    if (auto* pObj = *reinterpret_cast<GXIntrusiveRefObject**>(pThis + 0x30)) {
        GXIntrusiveRefObject_ReleaseStrongAndWeak(pObj);
        *reinterpret_cast<void**>(pThis + 0x30) = nullptr;
    }
    *reinterpret_cast<void**>(pThis + 0x28) = nullptr;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2351f90ULL))(pThis);
}

// 0x219c6f0 (28 B): GXDebugEnumBindingNode_DeletingDtor
void GXDebugEnumBindingNode_DeletingDtor(uint8_t* pThis) {
    GXDebugEnumBindingNode_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x219c6e0 (5 B): GXDebugEnumBindingNode_BaseDtorThunk
void GXDebugEnumBindingNode_BaseDtorThunk(uint8_t* pThis) {
    GXDebugEnumBindingNode_BaseDtor(pThis);
}

// 0x219c630 (166 B): GXDebugEnumOptionList_ClearOptions
void GXDebugEnumOptionList_ClearOptions(uint8_t* pThis) {
    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pThis + 0x18);
    uint8_t* pEnd   = *reinterpret_cast<uint8_t**>(pThis + 0x20);
    if (pBegin != pEnd) {
        for (uint8_t* pCur = pBegin; pCur != pEnd; pCur += 24) {
            *reinterpret_cast<uint64_t*>(pCur + 0x08) = 0;
            uint8_t* pRefStr = *reinterpret_cast<uint8_t**>(pCur + 0x00);
            if (pRefStr) {
                if (--(*reinterpret_cast<uint32_t*>(pRefStr + 0x38)) == 0) {
                    void* pStrNodeAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540e10ULL));
                    if (*reinterpret_cast<uint64_t*>(pRefStr + 0x20) >= 8) {
                        void* pBufAlloc = *reinterpret_cast<void**>(pRefStr + 0x28);
                        void* pStrData  = *reinterpret_cast<void**>(pRefStr + 0x08);
                        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pBufAlloc) + 0x70))(pBufAlloc, pStrData);
                    }
                    *reinterpret_cast<uint64_t*>(pRefStr + 0x20) = 7;
                    *reinterpret_cast<uint64_t*>(pRefStr + 0x18) = 0;
                    *reinterpret_cast<uint16_t*>(pRefStr + 0x08) = 0;
                    (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pStrNodeAlloc) + 0x70))(pStrNodeAlloc, pRefStr);
                }
                *reinterpret_cast<uint64_t*>(pCur + 0x00) = 0;
            }
        }
        pBegin = *reinterpret_cast<uint8_t**>(pThis + 0x18);
    }
    *reinterpret_cast<uint8_t**>(pThis + 0x20) = pBegin;
}

// 0x219c4d0 (337 B): GXDebugEnumOptionList_DeletingDtor
void GXDebugEnumOptionList_DeletingDtor(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b66e0ULL));
    GXDebugEnumOptionList_ClearOptions(pThis);
    pThis[0x0B] &= 0xBFu;
    void* pBuf = *reinterpret_cast<void**>(pThis + 0x18);
    if (pBuf) {
        void* pDealloc = *reinterpret_cast<void**>(pThis + 0x30);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pDealloc) + 0x70))(pDealloc, pBuf);
    }
    sub_2bbe588(pThis);
}

// 0x219c340 (400 B): GXDebugEnumOptionList_BaseDtor
void GXDebugEnumOptionList_BaseDtor(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b66e0ULL));
    GXDebugEnumOptionList_ClearOptions(pThis);
    *reinterpret_cast<uint32_t*>(pThis + 0x08) &= 0xBFFFFFFFu;
    void* pBuf = *reinterpret_cast<void**>(pThis + 0x18);
    if (pBuf) {
        void* pDealloc = *reinterpret_cast<void**>(pThis + 0x30);
        (*reinterpret_cast<void(**)(void*, void*)>(*reinterpret_cast<uintptr_t*>(pDealloc) + 0x70))(pDealloc, pBuf);
        *reinterpret_cast<uint64_t*>(pThis + 0x28) = 0;
        *reinterpret_cast<uint64_t*>(pThis + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(pThis + 0x18) = 0;
    }
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b93b0ULL));
    *reinterpret_cast<uint32_t*>(pThis + 0x08) &= 0xBFFFFFFFu;
}

// 0x219c300 (58 B): GXDebugBytePropertyNode_FormatValueString
bool GXDebugBytePropertyNode_FormatValueString(uint8_t* pThis, uint8_t* pOutString) {
    uint8_t* pTarget = *reinterpret_cast<uint8_t**>(pThis + 0x78);
    if (!pTarget) return false;
    sub_26295d0(pOutString, reinterpret_cast<const wchar_t*>(GX_VMA(0x49d80b6ULL)), static_cast<uint32_t>(*pTarget));
    return *reinterpret_cast<uint64_t*>(pOutString + 0x18) != 0;
}

// 0x219c290 (101 B): GXDebugBytePropertyNode_ReleaseWeakChildAndDetach
void GXDebugBytePropertyNode_ReleaseWeakChildAndDetach(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x78) = nullptr;
    GXIntrusiveRefObject* pChild = *reinterpret_cast<GXIntrusiveRefObject**>(pThis + 0x18);
    if (pChild) {
        uint32_t oldVal = pChild->m_refFlags;
        uint32_t newWeak = ((oldVal >> 15) - 1) & 0x7FFFu;
        pChild->m_refFlags = (oldVal & 0xC0007FFFu) | (newWeak << 15);
        if (newWeak == 0) {
            pChild->m_refFlags = oldVal | 0x3FFF8000u;
            auto fnDelete = reinterpret_cast<void(*)(GXIntrusiveRefObject*)>(pChild->m_vptr[4]);
            fnDelete(pChild);
        }
        *reinterpret_cast<void**>(pThis + 0x18) = nullptr;
    }
}

// 0x219c280 (13 B): GXDebugBytePropertyNode_DetachTargetByte
void GXDebugBytePropertyNode_DetachTargetByte(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x78) = nullptr;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2328bf0ULL))(pThis);
}

// 0x219c1e0 (72 B): GXDebugBytePropertyNode_BaseDtor
void GXDebugBytePropertyNode_BaseDtor(uint8_t* pThis) {
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b64e0ULL));
    *reinterpret_cast<void**>(pThis + 0x78) = nullptr;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2328bf0ULL))(pThis);
    pThis[0x0B] &= 0xBFu;
    *reinterpret_cast<void**>(pThis + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53bbab0ULL));
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2328bf0ULL))(pThis);
    pThis[0x0B] &= 0xBFu;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2328990ULL))(pThis);
}

// 0x219c230 (80 B): GXDebugBytePropertyNode_DeletingDtor
void GXDebugBytePropertyNode_DeletingDtor(uint8_t* pThis) {
    GXDebugBytePropertyNode_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x219c1d0 (1 B): GXDebugPropertyHandleNode_NoOpStub
void GXDebugPropertyHandleNode_NoOpStub() {}

// 0x219c110 (179 B): GXDebugPropertyHandleNode_ReleaseChildAndReset
void GXDebugPropertyHandleNode_ReleaseChildAndReset(uint8_t* pThis) {
    void* pChild = *reinterpret_cast<void**>(pThis + 0x10);
    if (!pChild) return;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x23286b0ULL))(pChild);
    if (auto* pObj = *reinterpret_cast<GXIntrusiveRefObject**>(pThis + 0x10)) {
        GXIntrusiveRefObject_ReleaseStrongAndWeak(pObj);
        *reinterpret_cast<void**>(pThis + 0x10) = nullptr;
    }
}

// 0x219c0f0 (28 B): GXDebugPropertyHandleNode_DeletingDtor
void GXDebugPropertyHandleNode_DeletingDtor(uint8_t* pThis) {
    GXDebugPropertyHandleNode_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x219c0e0 (5 B): GXDebugPropertyHandleNode_BaseDtorThunk
void GXDebugPropertyHandleNode_BaseDtorThunk(uint8_t* pThis) {
    GXDebugPropertyHandleNode_BaseDtor(pThis);
}

} // namespace GX
} // namespace Bloodborne
