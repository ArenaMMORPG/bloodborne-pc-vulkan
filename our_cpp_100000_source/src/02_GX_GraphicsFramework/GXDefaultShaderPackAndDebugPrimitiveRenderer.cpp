// ============================================================================
// Bloodborne Engine Decompilation Project (v1.09 / CUSA03173)
// Module: 02_GX_GraphicsFramework
// Address Range: 0x218a010 .. 0x2191c30 (Batch 8 — 75 Functions, 31,601 Bytes)
// ============================================================================
// Decompiled Strictly Right-to-Left (from 0x2191c30 down to 0x218a010):
//   1. GXDefaultShaderPack (0x2190ff0 .. 0x2191c30, 8 functions)
//      - Built-in "GXSimple" shader pack (6 VS, 6 InputLayouts, 9 PS, 12 presets)
//   2. GXDynamicGeometryDrawer & GXDebugPrimitiveRenderer (0x218f9d0 .. 0x2190fe0, 17 functions)
//      - Dynamic ring-buffer user primitive submission (Draw / DrawIndexed)
//      - Fullscreen quad blitting & 3D wireframe debug shapes (Box, Grid, Axes)
//   3. GXGParamBinaryReader (0x218f470 .. 0x218f8f0, 20 functions)
//      - Binary reader for FromSoftware ".gparam" ("filt" v3) parameter files
//   4. GXGParamGroupLookupTable (0x218e1b0 .. 0x218f3b0, 9 functions)
//      - Group/parameter name, comment, and ID lookup table for GParam files
//   5. GParamManager Hash Tables & Entry Containers (0x218a010 .. 0x218e1a5, 21 functions)
//      - Dantelion2 / STL hash table operations (Park-Miller LCG hash 0xdeadbeef),
//        bucket vector reallocation, and GParamLoadedFileEntry constructors
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <immintrin.h>

namespace GX {

// ============================================================================
// Forward Declarations & External Engine Helpers
// ============================================================================

struct GXDevice;
struct GXRenderContext;
struct GXCommandContext;
struct GXDynamicRingBuffer;
struct GXShaderArchive;
struct GXVertexShaderResource;
struct GXPixelShaderResource;
struct GXInputLayoutResource;
struct GXTextureResource;

#undef g_pGXDeviceSingleton
#define g_pGXDeviceSingleton (*reinterpret_cast<GXDevice**>(GX_VMA(0x55406c8ULL)))

struct DLAllocator {
    struct VTable {
        void* (*dtor)(DLAllocator*, uint32_t);                             // +0x00
        void* (*delself)(DLAllocator*);                                    // +0x08
        uint64_t (*GetHeapId)(DLAllocator*);                               // +0x10
        void* (*fn18)(DLAllocator*);                                       // +0x18
        void* (*GetBlockInfo)(void* pOutInfo, DLAllocator*, void* pBlock); // +0x20
        void* (*fn28)(DLAllocator*);                                       // +0x28
        void* (*fn30)(DLAllocator*);                                       // +0x30
        void* (*fn38)(DLAllocator*);                                       // +0x38
        void* (*fn40)(DLAllocator*);                                       // +0x40
        void* (*fn48)(DLAllocator*);                                       // +0x48
        void* (*Allocate)(DLAllocator*, size_t size);                      // +0x50
        void* (*AllocateAligned)(DLAllocator*, size_t size, size_t align); // +0x58
        void* (*fn60)(DLAllocator*);                                       // +0x60
        void* (*fn68)(DLAllocator*);                                       // +0x68
        void  (*Free)(DLAllocator*, void* pMem);                           // +0x70
    };
    VTable* vtable;
};

struct GXShaderResourceBase {
    struct VTable {
        void* (*dtor)(GXShaderResourceBase*, uint32_t); // +0x00
        void* (*fn08)(GXShaderResourceBase*);           // +0x08
        void* (*fn10)(GXShaderResourceBase*);           // +0x10
        void* (*GetHardwareShader)(GXShaderResourceBase*); // +0x18
    };
    VTable* vtable;
};

struct GXShaderArchiveVTable {
    void  (*dtor)(GXShaderArchive*);                                              // +0x00
    void* (*fn08)(GXShaderArchive*);                                              // +0x08
    void* (*fn10)(GXShaderArchive*);                                              // +0x10
    void* (*fn18)(GXShaderArchive*);                                              // +0x18
    void* (*fn20)(GXShaderArchive*);                                              // +0x20
    void* (*fn28)(GXShaderArchive*);                                              // +0x28
    void* (*fn30)(GXShaderArchive*);                                              // +0x30
    GXPixelShaderResource*  (*FindPixelShader)(GXShaderArchive*, uint32_t, const char16_t*);  // +0x38
    GXVertexShaderResource* (*FindVertexShader)(GXShaderArchive*, uint32_t, const char16_t*); // +0x40
};

struct GXShaderArchive {
    GXShaderArchiveVTable* vtable;      // +0x00 (0x53b6d80)
    uint64_t               m_unk08;     // +0x08
    void*                  m_pVecBegin; // +0x10
    void*                  m_pVecEnd;   // +0x18
    void*                  m_pVecCap;   // +0x20
    DLAllocator*           m_pAlloc;    // +0x28
    uint32_t               m_unk30;     // +0x30
    uint32_t               m_pad34;     // +0x34
};

struct GXInputLayoutInner {
    DLAllocator* m_pAlloc0; // +0x00
    DLAllocator* m_pAlloc1; // +0x08
    void*        m_pBuf10;  // +0x10
    void*        m_pBuf18;  // +0x18
};

struct GXInputLayoutResource {
    GXInputLayoutInner* m_pInner; // +0x00
};

#undef g_pGXMainAllocator
#define g_pGXMainAllocator (*reinterpret_cast<DLAllocator**>(GX_VMA(0x5540680ULL)))
#undef g_pGXStringAllocator
#define g_pGXStringAllocator (*reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL)))

struct GParamHashNodeBase {
    GParamHashNodeBase* m_pNext; // +0x00
    GParamHashNodeBase* m_pPrev; // +0x08
};

struct GParamHashBucketIter {
    void*               pListHead;  // +0x00
    void*               pListProxy; // +0x08
    GParamHashNodeBase* pNode;      // +0x10
};

// External calls into other engine subsystems
extern "C" {
    void sub_2192170(void* pDefaultShaderPack, GXRenderContext* pCtx, const void* pMatrix4x4, uint32_t slot);
    void sub_21696c0(void* pHwCtx, void* pHwInputState, uint32_t vertexCount, uint32_t startVertex);
    void sub_2169780(void* pHwCtx, void* pHwInputState, uint32_t indexCount, uint32_t startIndex, int32_t baseVertex);
    void* sub_26db050(void* pRingBuffer);
    int32_t sub_26db1b0(void* pRingRef, void** ppOutCpuAddr, void* pLockToken, uint32_t byteSize);
    void sub_26db300(void* pRingRef, void** ppOutBufferRes, uint32_t* pOutOffset, void* pLockToken);
    void sub_261a1b0(void* pDst, const void* pSrc, size_t byteSize);
    void sub_26681d0(const void* pAABB, float (*pOutCorners)[4]);
    DLAllocator* sub_2ba2b10();
    void sub_20b55b0(uint32_t level, uint32_t code, const char* msg, ...);
    void sub_26d73f0(void* pConstantBuffer, void* pHwCtx, uint32_t offset, const void* pData);
    void** sub_26d7440(void* pConstantBuffer);
    void sub_26d70e0(void** ppOutConstantBuffer, void* pDeviceArg, uint32_t count, const uint32_t* pByteSize, uint32_t r8d, uint32_t r9d, uint32_t stackFlag);
    bool sub_21bd870(GXShaderArchive* pArchive, const void* pBlob, size_t blobSize);
    void sub_21a2ce0(GXInputLayoutResource* pOutLayout, const void* pElements, uint32_t elemCount, GXVertexShaderResource* pVS, DLAllocator* pAlloc, uint32_t flags);
    void sub_e46040(void* pDeferQueue, void* pResource, void* pDeleterCallback, uint32_t count);
    DLAllocator* sub_207b720(void* pBlock);
    void sub_e46f40(void* pRenderTargetGroup, uint32_t index, void* pDepthBuffer, uint32_t flag);
    void sub_e47290(void* pRenderTargetGroup, uint32_t flag);
    void* sub_e47840(void* pRenderTargetGroup);
    void sub_21a1c50(GXRenderContext* pRenderCtx);
    int32_t sub_20e3ea0(char16_t* pDst, size_t maxCount, const char16_t* pFmt, ...);
    const uint16_t* sub_2bbec48();
    void sub_262a720(void* pWString, const char16_t* pSrc, size_t len);
    void sub_27c1a20(void* pWStringVec, const void* pWString);
    void sub_272b310(void* pOutIter, void* pWStringVec, GParamHashBucketIter firstIter, GParamHashBucketIter lastIter);
    void sub_26279a0(void* pDstWString, const void* pSrcWString, size_t pos, size_t count);
    void sub_2935350(void* pFixedWString, size_t maxLen, uint32_t flags);
    void sub_2935150(void* pDstFixedWString, const void* pSrcFixedWString, uint32_t pos, size_t count);
    void sub_2188a80(void* pPayload);
    void* sub_2189490(void* pOutIter, void* pBucketVec, size_t count, const void* pFillVal, GParamHashBucketIter whereIter);
    void* sub_2188dc0(void* pOutIter, void* pBucketVec, size_t count, const void* pFillVal, GParamHashBucketIter whereIter);
    void* sub_21899e0(void* pOutIter, void* pBucketVec, size_t count, const void* pFillVal, GParamHashBucketIter whereIter);
    [[noreturn]] void sub_2bbf1b8(const char* msg);
    [[noreturn]] void sub_2bbe978(void* pTlsExcept, uint32_t code);
}

static inline void GX_ReportOomOrThrow0x109()
{
    if (*reinterpret_cast<const int32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0) {
        sub_2bbe978(gx_guest_tls_slot(0x53e4320ULL), 1u);
    }
    sub_20b55b0(0u, 0x109u, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
}

// ============================================================================
// 1. GXDefaultShaderPack (0x2190ff0 .. 0x2191c30)
// ============================================================================

struct GXShaderPresetTriple {
    GXInputLayoutResource*  m_pInputLayout;  // +0x00
    GXVertexShaderResource* m_pVertexShader; // +0x08
    GXPixelShaderResource*  m_pPixelShader;  // +0x10
};

struct GXDefaultShaderPack {
    DLAllocator*            m_pAllocator;             // +0x00
    uint8_t                 m_bInitialized;           // +0x08
    uint8_t                 m_pad09[7];               // +0x09
    GXShaderPresetTriple    m_presets[12];            // +0x10 .. +0x130 (12 * 24 = 0x120 bytes)
    GXInputLayoutResource*  m_pInputLayouts[6];       // +0x130 .. +0x160
    GXVertexShaderResource* m_pVertexShaders[6];      // +0x160 .. +0x190
    GXPixelShaderResource*  m_pPixelShaders[9];       // +0x190 .. +0x1D8
    void*                   m_pConstantBuffers[4];    // +0x1D8 .. +0x1F8 (+0x1D8, +0x1E0, +0x1E8, +0x1F0)
    uint8_t                 m_bAuxCBsInitialized;     // +0x1F8
    uint8_t                 m_pad1F9[7];              // +0x1F9
    GXShaderArchive*        m_pOwnedShaderArchive;    // +0x200
};

void GXDefaultShaderPack_Destroy(GXDefaultShaderPack* pThis);
bool GXDefaultShaderPack_PopulateShadersAndInputLayoutsFromArchive(GXDefaultShaderPack* pThis, GXShaderArchive* pArchive);

// [VMA: 0x2191c30 - 0x2192032] (1026 B) — Bind built-in shader preset (0..11) and its input layout / VS / PS to render context
bool GXDefaultShaderPack_BindPresetPipelineToContext(
    GXDefaultShaderPack* pThis,
    GXRenderContext* pContext,
    int32_t presetIndex,
    uint8_t bDirectHwOnly)
{
    const GXShaderPresetTriple& preset = pThis->m_presets[presetIndex];

    void* pHwInputLayout = nullptr;
    if (preset.m_pInputLayout != nullptr) {
        pHwInputLayout = preset.m_pInputLayout->m_pInner;
    }

    void* pHwVS = nullptr;
    if (preset.m_pVertexShader != nullptr) {
        auto* pVSBase = reinterpret_cast<GXShaderResourceBase*>(preset.m_pVertexShader);
        pHwVS = pVSBase->vtable->GetHardwareShader(pVSBase);
    }

    if (preset.m_pPixelShader == nullptr) {
        return false;
    }
    auto* pPSBase = reinterpret_cast<GXShaderResourceBase*>(preset.m_pPixelShader);
    void* pHwPS = pPSBase->vtable->GetHardwareShader(pPSBase);

    if (pHwInputLayout == nullptr || pHwVS == nullptr || pHwPS == nullptr) {
        return false;
    }

    auto* pCtxBytes = reinterpret_cast<uint8_t*>(pContext);
    auto* pHwState = *reinterpret_cast<uint8_t**>(pCtxBytes + 0x08);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultStateTable = *reinterpret_cast<uint8_t**>(pDeviceBytes + 0x250);
    void* pDefaultSampler = *reinterpret_cast<void**>(pDefaultStateTable + 0x2C0);

    if (bDirectHwOnly != 0) {
        *reinterpret_cast<void**>(pHwState + 0xB730) = pHwInputLayout;
        *reinterpret_cast<void**>(pHwState + 0xC1B8) = pHwVS;
        *reinterpret_cast<void**>(pHwState + 0xD598) = pHwPS;
        *reinterpret_cast<void**>(pHwState + 0xC6B0) = nullptr;
        *reinterpret_cast<void**>(pHwState + 0xCBA8) = nullptr;
        *reinterpret_cast<void**>(pHwState + 0xD0A0) = nullptr;
    } else {
        *reinterpret_cast<void**>(pCtxBytes + 0x0260) = pHwInputLayout;
        *reinterpret_cast<void**>(pHwState + 0xB730) = pHwInputLayout;
        *reinterpret_cast<void**>(pCtxBytes + 0x0780) = pHwVS;
        *reinterpret_cast<void**>(pHwState + 0xC1B8) = pHwVS;
        *reinterpret_cast<void**>(pCtxBytes + 0x0A68) = pHwPS;
        *reinterpret_cast<void**>(pHwState + 0xD598) = pHwPS;

        *reinterpret_cast<void**>(pCtxBytes + 0x1320) = nullptr;
        *reinterpret_cast<void**>(pHwState + 0xC6B0) = nullptr;
        *reinterpret_cast<void**>(pCtxBytes + 0x1038) = nullptr;
        *reinterpret_cast<void**>(pHwState + 0xCBA8) = nullptr;
        *reinterpret_cast<void**>(pCtxBytes + 0x0D50) = nullptr;
        *reinterpret_cast<void**>(pHwState + 0xD0A0) = nullptr;

        *reinterpret_cast<void**>(pCtxBytes + 0x09E0) = pDefaultSampler;
        *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0A60) = 0;
        uint32_t samplerEnd = *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0A64);
        *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0A64) = (samplerEnd > 1u) ? samplerEnd : 1u;
    }

    *reinterpret_cast<uint64_t*>(pHwState + 0x1978) |= 0x1ULL;
    *reinterpret_cast<void**>(pHwState + 0xD0A8) = pDefaultSampler;

    // Upload default 16-byte PS constant (0x4738020 = {1, 1, 1, 1}) to m_pConstantBuffers[1] (+0x1E0) and bind to PS CB slot 0
    sub_26d73f0(pThis->m_pConstantBuffers[1], pHwState, 0, reinterpret_cast<const float*>(GX_VMA(0x4738020ULL)));
    void** ppCb1Views = sub_26d7440(pThis->m_pConstantBuffers[1]);

    if (bDirectHwOnly != 0) {
        pHwState[0x197A] |= 0x01;
        auto* pViewBytes = reinterpret_cast<uint8_t*>(ppCb1Views[0]);
        if (pViewBytes != nullptr) {
            const uint32_t reqFlags = 0x100001u;
            uint32_t curFlags = *reinterpret_cast<uint32_t*>(pViewBytes + 0x44);
            if ((curFlags & reqFlags) != reqFlags) {
                __sync_fetch_and_or(reinterpret_cast<uint32_t*>(pViewBytes + 0x44), reqFlags);
            }
        }
        *reinterpret_cast<void**>(pHwState + 0xD528) = ppCb1Views[0];
    } else {
        *reinterpret_cast<void**>(pCtxBytes + 0x0968) = ppCb1Views[0];
        *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09D8) = 0;
        uint32_t cbEnd0 = *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09DC);
        *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09DC) = (cbEnd0 > 1u) ? cbEnd0 : 1u;

        *reinterpret_cast<uint64_t*>(pHwState + 0x1978) |= 0x10000ULL;
        auto* pViewBytes = reinterpret_cast<uint8_t*>(ppCb1Views[0]);
        if (pViewBytes != nullptr) {
            const uint32_t reqFlags = 0x100001u;
            uint32_t curFlags = *reinterpret_cast<uint32_t*>(pViewBytes + 0x44);
            if ((curFlags & reqFlags) != reqFlags) {
                __sync_fetch_and_or(reinterpret_cast<uint32_t*>(pViewBytes + 0x44), reqFlags);
            }
        }
        *reinterpret_cast<void**>(pHwState + 0xD528) = ppCb1Views[0];
    }

    // Presets 2, 5, 6 (bitmask 0x64 = 0b01100100) also bind auxiliary CB (+0x1E8) to VS CB slot 1
    if (static_cast<uint32_t>(presetIndex) <= 6u && ((0x64u >> presetIndex) & 1u) != 0) {
        if (pThis->m_bAuxCBsInitialized == 0) {
            uint32_t valZero = 0;
            sub_26d73f0(pThis->m_pConstantBuffers[2], pHwState, 0, &valZero);
            uint32_t valOne = 1;
            sub_26d73f0(pThis->m_pConstantBuffers[3], pHwState, 0, &valOne);
            pThis->m_bAuxCBsInitialized = 1;
        }

        void** ppCb2Views = sub_26d7440(pThis->m_pConstantBuffers[2]);
        if (bDirectHwOnly != 0) {
            pHwState[0x197A] |= 0x02;
            auto* pViewBytes = reinterpret_cast<uint8_t*>(ppCb2Views[0]);
            if (pViewBytes != nullptr) {
                const uint32_t reqFlags = 0x100002u;
                uint32_t curFlags = *reinterpret_cast<uint32_t*>(pViewBytes + 0x44);
                if ((curFlags & reqFlags) != reqFlags) {
                    __sync_fetch_and_or(reinterpret_cast<uint32_t*>(pViewBytes + 0x44), reqFlags);
                }
            }
            *reinterpret_cast<void**>(pHwState + 0xD530) = ppCb2Views[0];
        } else {
            *reinterpret_cast<void**>(pCtxBytes + 0x0970) = ppCb2Views[0];
            uint32_t cbStart = *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09D8);
            *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09D8) = (cbStart != 0) ? 1u : 0u;
            uint32_t cbEnd1 = *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09DC);
            *reinterpret_cast<uint32_t*>(pCtxBytes + 0x09DC) = (cbEnd1 > 2u) ? cbEnd1 : 2u;

            *reinterpret_cast<uint64_t*>(pHwState + 0x1978) |= 0x20000ULL;
            auto* pViewBytes = reinterpret_cast<uint8_t*>(ppCb2Views[0]);
            if (pViewBytes != nullptr) {
                const uint32_t reqFlags = 0x100002u;
                uint32_t curFlags = *reinterpret_cast<uint32_t*>(pViewBytes + 0x44);
                if ((curFlags & reqFlags) != reqFlags) {
                    __sync_fetch_and_or(reinterpret_cast<uint32_t*>(pViewBytes + 0x44), reqFlags);
                }
            }
            *reinterpret_cast<void**>(pHwState + 0xD530) = ppCb2Views[0];
        }
    }

    return true;
}

// [VMA: 0x2191bc0 - 0x2191c25] (101 B) — Query whether built-in shader preset (0..11) has valid InputLayout, VS, and PS
bool GXDefaultShaderPack_IsPresetValid(const GXDefaultShaderPack* pThis, int32_t presetIndex)
{
    const GXShaderPresetTriple& preset = pThis->m_presets[presetIndex];
    if (preset.m_pInputLayout == nullptr || preset.m_pInputLayout->m_pInner == nullptr) {
        return false;
    }
    if (preset.m_pPixelShader == nullptr) {
        return false;
    }
    auto* pPSBase = reinterpret_cast<GXShaderResourceBase*>(preset.m_pPixelShader);
    if (pPSBase->vtable->GetHardwareShader(pPSBase) == nullptr) {
        return false;
    }
    if (preset.m_pVertexShader == nullptr) {
        return false;
    }
    auto* pVSBase = reinterpret_cast<GXShaderResourceBase*>(preset.m_pVertexShader);
    return pVSBase->vtable->GetHardwareShader(pVSBase) != nullptr;
}

// [VMA: 0x21919b0 - 0x2191bb1] (513 B) — Load built-in shader archive from memory blob and populate presets
bool GXDefaultShaderPack_LoadFromMemoryArchive(
    GXDefaultShaderPack* pThis,
    const void* pArchiveBlob,
    size_t archiveSize)
{
    if (pArchiveBlob == nullptr || archiveSize == 0) {
        return false;
    }

    GXDefaultShaderPack_Destroy(pThis);

    auto* pArchive = static_cast<GXShaderArchive*>(
        g_pGXMainAllocator->vtable->AllocateAligned(g_pGXMainAllocator, 0x38, 8));
    if (pArchive != nullptr) {
#undef g_GXShaderArchiveVTable
#define g_GXShaderArchiveVTable (*reinterpret_cast<GXShaderArchiveVTable*>(GX_VMA(0x53b6d80ULL)))
        pArchive->vtable = &g_GXShaderArchiveVTable;

        DLAllocator* pVecAlloc = g_pGXStringAllocator;
        if (pVecAlloc == nullptr) {
            pVecAlloc = sub_2ba2b10();
            g_pGXStringAllocator = pVecAlloc;
        }
        uint8_t blockInfo[8]{};
        pVecAlloc->vtable->GetBlockInfo(blockInfo, pVecAlloc, nullptr);
        if ((blockInfo[0] & 0x20) == 0) {
            sub_20b55b0(0u, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }
        pArchive->m_pVecBegin = nullptr;
        pArchive->m_pVecEnd = nullptr;
        pArchive->m_pVecCap = nullptr;
        pArchive->m_pAlloc = pVecAlloc;
        pArchive->m_unk30 = 0;
    }
    pThis->m_pOwnedShaderArchive = pArchive;

    if (!sub_21bd870(pArchive, pArchiveBlob, archiveSize)) {
        return false;
    }
    if (!GXDefaultShaderPack_PopulateShadersAndInputLayoutsFromArchive(pThis, pThis->m_pOwnedShaderArchive)) {
        return false;
    }

    void* pDeviceInner = *reinterpret_cast<void**>(g_pGXDeviceSingleton);
    uint32_t cbSize = 0x40;
    sub_26d70e0(&pThis->m_pConstantBuffers[0], pDeviceInner, 1, &cbSize, 0, 0, 1);
    cbSize = 0x10;
    sub_26d70e0(&pThis->m_pConstantBuffers[1], pDeviceInner, 1, &cbSize, 0, 0, 1);
    cbSize = 0x10;
    sub_26d70e0(&pThis->m_pConstantBuffers[2], pDeviceInner, 1, &cbSize, 0, 0, 1);
    sub_26d70e0(&pThis->m_pConstantBuffers[3], pDeviceInner, 1, &cbSize, 0, 0, 1);

    pThis->m_bInitialized = 1;
    return true;
}

// [VMA: 0x21916d0 - 0x21919a8] (728 B) — Populate 6 VS, 6 InputLayouts, 9 PS, and 12 preset triples from shader archive
bool GXDefaultShaderPack_PopulateShadersAndInputLayoutsFromArchive(
    GXDefaultShaderPack* pThis,
    GXShaderArchive* pArchive)
{
    if (pArchive == nullptr) {
        return false;
    }

#undef g_GXSimpleVSNames
#define g_GXSimpleVSNames (reinterpret_cast<const char16_t**>(GX_VMA(0x535f560ULL)))
#undef g_GXSimplePSNames
#define g_GXSimplePSNames (reinterpret_cast<const char16_t**>(GX_VMA(0x535f590ULL)))
#undef g_GXInputLayoutDesc0
#define g_GXInputLayoutDesc0 (reinterpret_cast<const uint8_t*>(GX_VMA(0x4738de0ULL)))
#undef g_GXInputLayoutDesc1
#define g_GXInputLayoutDesc1 (reinterpret_cast<const uint8_t*>(GX_VMA(0x4738e20ULL)))
#undef g_GXInputLayoutDesc2
#define g_GXInputLayoutDesc2 (reinterpret_cast<const uint8_t*>(GX_VMA(0x4738e80ULL)))
#undef g_GXInputLayoutDesc3
#define g_GXInputLayoutDesc3 (reinterpret_cast<const uint8_t*>(GX_VMA(0x4738ea0ULL)))

    char16_t basePrefix[260]{};
    sub_20e3ea0(
        basePrefix,
        0x104,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c2bb4ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c2bbaULL)));

    char16_t fullShaderName[260]{};
    for (uint32_t i = 0; i < 6; ++i) {
        sub_20e3ea0(
            fullShaderName,
            0x104,
            reinterpret_cast<const char16_t*>(GX_VMA(0x49d2cdcULL)),
            basePrefix,
            g_GXSimpleVSNames[i]);
        GXVertexShaderResource* pVS = pArchive->vtable->FindVertexShader(pArchive, 0, fullShaderName);
        pThis->m_pVertexShaders[i] = pVS;
        if (pVS == nullptr) {
            continue;
        }

        uint32_t layoutType = 2;
        if (i != 4) {
            layoutType = (i > 1) ? 1u : 0u;
            if (i == 5) {
                layoutType = 3;
            }
        }

        if (pThis->m_pInputLayouts[i] == nullptr) {
            auto* pNewLayout = static_cast<GXInputLayoutResource*>(
                pThis->m_pAllocator->vtable->AllocateAligned(pThis->m_pAllocator, 8, 8));
            if (pNewLayout != nullptr) {
                pNewLayout->m_pInner = nullptr;
            }
            pThis->m_pInputLayouts[i] = pNewLayout;

            switch (layoutType) {
            case 0:
                sub_21a2ce0(pNewLayout, g_GXInputLayoutDesc0, 2, pVS, pThis->m_pAllocator, 0);
                break;
            case 1:
                sub_21a2ce0(pNewLayout, g_GXInputLayoutDesc1, 3, pVS, pThis->m_pAllocator, 0);
                break;
            case 2:
                sub_21a2ce0(pNewLayout, g_GXInputLayoutDesc2, 1, pVS, pThis->m_pAllocator, 0);
                break;
            case 3:
                sub_21a2ce0(pNewLayout, g_GXInputLayoutDesc3, 3, pVS, pThis->m_pAllocator, 0);
                break;
            default:
                break;
            }
        }
    }

    for (uint32_t j = 0; j < 9; ++j) {
        sub_20e3ea0(
            fullShaderName,
            0x104,
            reinterpret_cast<const char16_t*>(GX_VMA(0x49d2cdcULL)),
            basePrefix,
            g_GXSimplePSNames[j]);
        pThis->m_pPixelShaders[j] = pArchive->vtable->FindPixelShader(pArchive, 0, fullShaderName);
    }

    // 12 built-in presets table at 0x4738f00 (each entry: {int32_t vsIdx, int32_t psIdx, int32_t ilIdx})
#undef g_GXSimplePresetTable
#define g_GXSimplePresetTable (reinterpret_cast<const int32_t(*)[3]>(GX_VMA(0x4738f00ULL)))
    for (uint32_t p = 0; p < 12; ++p) {
        int32_t vsIdx = g_GXSimplePresetTable[p][0];
        int32_t psIdx = g_GXSimplePresetTable[p][1];
        int32_t ilIdx = g_GXSimplePresetTable[p][2];
        if (pThis->m_pVertexShaders[vsIdx] != nullptr &&
            pThis->m_pPixelShaders[psIdx] != nullptr &&
            pThis->m_pInputLayouts[ilIdx] != nullptr)
        {
            pThis->m_presets[p].m_pInputLayout  = pThis->m_pInputLayouts[ilIdx];
            pThis->m_presets[p].m_pVertexShader = pThis->m_pVertexShaders[vsIdx];
            pThis->m_presets[p].m_pPixelShader  = pThis->m_pPixelShaders[psIdx];
        }
    }

    return true;
}

// [VMA: 0x21915a0 - 0x21916ce] (302 B) — Initialize built-in shader pack from an already-created archive instance
bool GXDefaultShaderPack_InitializeFromExistingArchive(
    GXDefaultShaderPack* pThis,
    GXShaderArchive* pArchive)
{
    if (pArchive == nullptr) {
        return false;
    }

    GXDefaultShaderPack_Destroy(pThis);
    if (!GXDefaultShaderPack_PopulateShadersAndInputLayoutsFromArchive(pThis, pArchive)) {
        return false;
    }

    void* pDeviceInner = *reinterpret_cast<void**>(g_pGXDeviceSingleton);
    uint32_t cbSize = 0x40;
    sub_26d70e0(&pThis->m_pConstantBuffers[0], pDeviceInner, 1, &cbSize, 0, 0, 1);
    cbSize = 0x10;
    sub_26d70e0(&pThis->m_pConstantBuffers[1], pDeviceInner, 1, &cbSize, 0, 0, 1);
    cbSize = 0x10;
    sub_26d70e0(&pThis->m_pConstantBuffers[2], pDeviceInner, 1, &cbSize, 0, 0, 1);
    sub_26d70e0(&pThis->m_pConstantBuffers[3], pDeviceInner, 1, &cbSize, 0, 0, 1);

    pThis->m_bInitialized = 1;
    return true;
}

// [VMA: 0x2191030 - 0x2191595] (1381 B) — Release all constant buffers, pixel/vertex shaders, input layouts, and archive
void GXDefaultShaderPack_Destroy(GXDefaultShaderPack* pThis)
{
#undef g_GXConstantBufferDeleterDesc
#define g_GXConstantBufferDeleterDesc (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1d0ULL)))

    for (int i = 3; i >= 0; --i) {
        if (pThis->m_pConstantBuffers[i] != nullptr) {
            auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
            void* pDeferQueue = *reinterpret_cast<void**>(pDeviceBytes + 0x268);
            sub_e46040(pDeferQueue, pThis->m_pConstantBuffers[i], g_GXConstantBufferDeleterDesc, 1);
            pThis->m_pConstantBuffers[i] = nullptr;
        }
    }

    if (pThis->m_pOwnedShaderArchive != nullptr) {
        DLAllocator* pMainAlloc = g_pGXMainAllocator;
        pThis->m_pOwnedShaderArchive->vtable->dtor(pThis->m_pOwnedShaderArchive);
        pMainAlloc->vtable->Free(pMainAlloc, pThis->m_pOwnedShaderArchive);
        pThis->m_pOwnedShaderArchive = nullptr;
    }

    for (uint32_t i = 0; i < 6; ++i) {
        GXInputLayoutResource* pLayout = pThis->m_pInputLayouts[i];
        if (pLayout != nullptr) {
            DLAllocator* pLayoutAlloc = pThis->m_pAllocator;
            GXInputLayoutInner* pInner = pLayout->m_pInner;
            if (pInner != nullptr) {
                DLAllocator* pInnerBlockAlloc = sub_207b720(pInner);
                if (pInner->m_pBuf18 != nullptr) {
                    pInner->m_pAlloc0->vtable->Free(pInner->m_pAlloc0, pInner->m_pBuf18);
                    pInner->m_pBuf18 = nullptr;
                }
                if (pInner->m_pBuf10 != nullptr) {
                    pInner->m_pAlloc1->vtable->Free(pInner->m_pAlloc1, pInner->m_pBuf10);
                    pInner->m_pBuf10 = nullptr;
                }
                pInnerBlockAlloc->vtable->Free(pInnerBlockAlloc, pInner);
                pLayout->m_pInner = nullptr;
            }
            pLayout->m_pInner = nullptr;
            pLayoutAlloc->vtable->Free(pLayoutAlloc, pLayout);
            pThis->m_pInputLayouts[i] = nullptr;
        }
    }

    std::memset(pThis->m_presets, 0, sizeof(pThis->m_presets));
    pThis->m_bInitialized = 0;
    for (int j = 8; j >= 0; --j) {
        pThis->m_pPixelShaders[j] = nullptr;
    }
    for (int i = 5; i >= 0; --i) {
        pThis->m_pVertexShaders[i] = nullptr;
    }
}

// [VMA: 0x2191020 - 0x2191025] (5 B) — Thunk to GXDefaultShaderPack_Destroy
void GXDefaultShaderPack_DestroyThunk(GXDefaultShaderPack* pThis)
{
    GXDefaultShaderPack_Destroy(pThis);
}

// [VMA: 0x2190ff0 - 0x2191018] (40 B) — Construct empty GXDefaultShaderPack (0x208 bytes)
void GXDefaultShaderPack_Construct(GXDefaultShaderPack* pThis, DLAllocator* pAllocator)
{
    pThis->m_pAllocator = pAllocator;
    pThis->m_bInitialized = 0;
    pThis->m_pOwnedShaderArchive = nullptr;
    std::memset(&pThis->m_presets[0], 0, 0x1E9);
}

// ============================================================================
// 2. GXDynamicGeometryDrawer & GXDebugPrimitiveRenderer (0x218f9d0 .. 0x2190fe0)
// ============================================================================

struct GXDynamicGeometryDrawer {
    void**            vtable;         // +0x00 (0x535f520)
    GXCommandContext* m_pCommandCtx;  // +0x08
    void*             m_pRingBuffer;  // +0x10
};

// [VMA: 0x2190fe0 - 0x2190fe1] (1 B) — Virtual no-op stub #5
void GXDynamicGeometryDrawer_VFuncNoOp5(GXDynamicGeometryDrawer*) {}

// [VMA: 0x2190fd0 - 0x2190fd1] (1 B) — Virtual no-op stub #4
void GXDynamicGeometryDrawer_VFuncNoOp4(GXDynamicGeometryDrawer*) {}

// [VMA: 0x2190fc0 - 0x2190fc1] (1 B) — Virtual no-op stub #3
void GXDynamicGeometryDrawer_VFuncNoOp3(GXDynamicGeometryDrawer*) {}

// [VMA: 0x2190fb0 - 0x2190fb1] (1 B) — Virtual no-op stub #2
void GXDynamicGeometryDrawer_VFuncNoOp2(GXDynamicGeometryDrawer*) {}

// [VMA: 0x2190fa0 - 0x2190fa1] (1 B) — Virtual no-op stub #1
void GXDynamicGeometryDrawer_VFuncNoOp1(GXDynamicGeometryDrawer*) {}

// [VMA: 0x2190f90 - 0x2190f93] (3 B) — Virtual stub returning false/0
bool GXDynamicGeometryDrawer_VFuncReturnFalse(GXDynamicGeometryDrawer*)
{
    return false;
}

// [VMA: 0x2190d30 - 0x2190ef2] (450 B) — Allocate dynamic VB/IB space in ring buffer, copy user data, and issue DrawIndexed
void GXDynamicGeometryDrawer_DrawIndexedUserPrimitives(
    GXDynamicGeometryDrawer* pThis,
    uint32_t topology,
    int32_t minVertexIndex,
    uint32_t maxVertexIndex,
    uint32_t indexCount,
    const void* pIndexData,
    uint32_t indexFormat,
    const void* pVertexData,
    uint32_t vertexStride)
{
    auto* pCmdCtxPtrs = reinterpret_cast<void**>(pThis->m_pCommandCtx);
    auto* pRenderCtx = static_cast<uint8_t*>(pCmdCtxPtrs[0]);
    void* pLockToken = pCmdCtxPtrs[1];

    void* pBufferRes = nullptr;
    uint32_t allocByteOffset = 0;

    uint32_t indexElemBytes = 2;
    if (indexFormat != 0x39) {
        indexElemBytes = (indexFormat == 0x2A) ? 4u : 0u;
    }

    uint32_t vertexBytes = ((maxVertexIndex - static_cast<uint32_t>(minVertexIndex)) + 1u) * vertexStride;
    uint32_t indexBytes = indexElemBytes * indexCount;

    void* pCpuWriteAddr = nullptr;
    if (sub_26db1b0(pThis->m_pRingBuffer, &pCpuWriteAddr, pLockToken, vertexBytes + indexBytes) != 0) {
        return;
    }

    sub_261a1b0(pCpuWriteAddr, pVertexData, vertexBytes);
    sub_261a1b0(static_cast<uint8_t*>(pCpuWriteAddr) + vertexBytes, pIndexData, indexBytes);
    sub_26db300(pThis->m_pRingBuffer, &pBufferRes, &allocByteOffset, pLockToken);

    uint32_t ibByteOffset = vertexBytes + allocByteOffset;
    auto* pHwState = *reinterpret_cast<uint8_t**>(pRenderCtx + 0x08);

    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0268) = topology;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB738) = topology;

    *reinterpret_cast<void**>(pRenderCtx + 0x0280) = pBufferRes;
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0380) = vertexStride;
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0400) = allocByteOffset;

    pHwState[0x25F0] |= 0x01;
    *reinterpret_cast<void**>(pHwState + 0xB750) = pBufferRes;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB850) = vertexStride;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB8D0) = allocByteOffset;

    *reinterpret_cast<void**>(pRenderCtx + 0x0270) = pBufferRes;
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0278) = indexFormat;
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x027C) = ibByteOffset;

    *reinterpret_cast<void**>(pHwState + 0xB740) = pBufferRes;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB748) = indexFormat;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB74C) = ibByteOffset;

    std::memset(pRenderCtx + 0x1670, 0, 0xB8);
    *reinterpret_cast<uint64_t*>(pRenderCtx + 0x1680) = 0xEULL;
    *reinterpret_cast<uint64_t*>(pRenderCtx + 0x1688) = 0x8ULL;

    sub_2169780(pHwState, pHwState + 0xB730, indexCount, 0, 0);
}

// [VMA: 0x2190be0 - 0x2190d24] (324 B) — Allocate dynamic VB space in ring buffer, copy user vertices, and issue Draw
void GXDynamicGeometryDrawer_DrawUserPrimitives(
    GXDynamicGeometryDrawer* pThis,
    uint32_t topology,
    uint32_t vertexCount,
    const void* pVertexData,
    uint32_t vertexStride)
{
    auto* pCmdCtxPtrs = reinterpret_cast<void**>(pThis->m_pCommandCtx);
    auto* pRenderCtx = static_cast<uint8_t*>(pCmdCtxPtrs[0]);
    void* pLockToken = pCmdCtxPtrs[1];

    void* pBufferRes = nullptr;
    uint32_t allocByteOffset = 0;
    uint32_t totalBytes = vertexStride * vertexCount;

    void* pCpuWriteAddr = nullptr;
    if (sub_26db1b0(pThis->m_pRingBuffer, &pCpuWriteAddr, pLockToken, totalBytes) != 0) {
        return;
    }

    sub_261a1b0(pCpuWriteAddr, pVertexData, totalBytes);
    sub_26db300(pThis->m_pRingBuffer, &pBufferRes, &allocByteOffset, pLockToken);

    auto* pHwState = *reinterpret_cast<uint8_t**>(pRenderCtx + 0x08);
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0268) = topology;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB738) = topology;

    *reinterpret_cast<void**>(pRenderCtx + 0x0280) = pBufferRes;
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0380) = vertexStride;
    *reinterpret_cast<uint32_t*>(pRenderCtx + 0x0400) = allocByteOffset;

    pHwState[0x25F0] |= 0x01;
    *reinterpret_cast<void**>(pHwState + 0xB750) = pBufferRes;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB850) = vertexStride;
    *reinterpret_cast<uint32_t*>(pHwState + 0xB8D0) = allocByteOffset;

    std::memset(pRenderCtx + 0x1670, 0, 0xB8);
    *reinterpret_cast<uint64_t*>(pRenderCtx + 0x1680) = 0xEULL;
    *reinterpret_cast<uint64_t*>(pRenderCtx + 0x1688) = 0x8ULL;

    sub_21696c0(pHwState, pHwState + 0xB730, vertexCount, 0);
}

// [VMA: 0x2190bb0 - 0x2190bda] (42 B) — Construct GXDynamicGeometryDrawer with explicit ring buffer reference
void GXDynamicGeometryDrawer_ConstructWithRingBuffer(
    GXDynamicGeometryDrawer* pThis,
    GXCommandContext* pCommandCtx,
    void* pRingBuffer)
{
#undef g_GXDynamicGeometryDrawerVTable
#define g_GXDynamicGeometryDrawerVTable (reinterpret_cast<void**>(GX_VMA(0x535f520ULL)))
    pThis->vtable = g_GXDynamicGeometryDrawerVTable;
    pThis->m_pCommandCtx = pCommandCtx;
    pThis->m_pRingBuffer = sub_26db050(pRingBuffer);
}

// [VMA: 0x2190b50 - 0x2190ba7] (87 B) — Construct GXDynamicGeometryDrawer selecting immediate or worker ring buffer from GXDevice
void GXDynamicGeometryDrawer_ConstructForContext(
    GXDynamicGeometryDrawer* pThis,
    GXCommandContext* pCommandCtx)
{
#undef g_GXDynamicGeometryDrawerVTable
#define g_GXDynamicGeometryDrawerVTable (reinterpret_cast<void**>(GX_VMA(0x535f520ULL)))
    pThis->vtable = g_GXDynamicGeometryDrawerVTable;
    pThis->m_pCommandCtx = pCommandCtx;

    auto* pRenderCtx = *reinterpret_cast<uint8_t**>(pCommandCtx);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    if (pRenderCtx[0x2093] != 0) {
        pThis->m_pRingBuffer = sub_26db050(*reinterpret_cast<void**>(pDeviceBytes + 0x390));
    } else {
        uint32_t workerIdx = *reinterpret_cast<uint32_t*>(pRenderCtx + 0x2098);
        void** ppWorkerRings = *reinterpret_cast<void***>(pDeviceBytes + 0x3A0);
        pThis->m_pRingBuffer = ppWorkerRings[workerIdx];
    }
}

struct GXDebugDrawContext {
    GXCommandContext*        m_pCommandCtx; // +0x00
    GXDynamicGeometryDrawer* m_pDrawer;     // +0x08
};

struct GXDebugPrimitiveRenderer {
    GXDebugDrawContext* m_pDrawCtx;       // +0x00
    bool                m_bPipelineBound; // +0x08
};

// [VMA: 0x21908d0 - 0x2190ab5] (485 B) — Clear or blit fullscreen quad using preset 0 (solid constant color)
void GXDebugPrimitiveRenderer_ClearOrBlitFullscreenQuad(
    GXDebugDrawContext* pDrawCtx,
    uint8_t stencilRef)
{
    auto* pRenderCtx = *reinterpret_cast<GXRenderContext**>(pDrawCtx->m_pCommandCtx);
    auto* pCtxBytes = reinterpret_cast<uint8_t*>(pRenderCtx);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultShaders = *reinterpret_cast<GXDefaultShaderPack**>(pDeviceBytes + 0x388);

    if (!GXDefaultShaderPack_BindPresetPipelineToContext(pDefaultShaders, pRenderCtx, 0, 0)) {
        return;
    }

    void* pDepthBuffer = *reinterpret_cast<void**>(pCtxBytes + 0x58);
    void* pRtGroup = pCtxBytes + 0x1728;
    sub_e46f40(pRtGroup, 0, pDepthBuffer, 1);
    *reinterpret_cast<void**>(pCtxBytes + 0x1EC0) = sub_e47840(pRtGroup);
    sub_21a1c50(pRenderCtx);

    auto* pDefaultStateTable = *reinterpret_cast<uint8_t**>(pDeviceBytes + 0x250);
    auto* pHwState = *reinterpret_cast<uint8_t**>(pCtxBytes + 0x08);

    // Bind default blend state (+0x190)
    void* pBlendState = *reinterpret_cast<void**>(pDefaultStateTable + 0x190);
    *reinterpret_cast<void**>(pCtxBytes + 0x0480) = pBlendState;
    *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0498) = 0xFFFFFFFFu;
    *reinterpret_cast<void**>(pHwState + 0xBC00) = pBlendState;
    *reinterpret_cast<uint64_t*>(pHwState + 0xBCA0) = 0;
    *reinterpret_cast<uint64_t*>(pHwState + 0xBC98) = 0;
    *reinterpret_cast<uint32_t*>(pHwState + 0xBCB0) = 0xFFFFFFFFu;

    // Bind default rasterizer state (+0x1F8)
    void* pRasterState = *reinterpret_cast<void**>(pDefaultStateTable + 0x1F8);
    *reinterpret_cast<void**>(pCtxBytes + 0x0258) = pRasterState;
    *reinterpret_cast<void**>(pHwState + 0xB970) = pRasterState;

    // Bind default depth-stencil state (+0x1F0) and stencilRef
    void* pDepthStencilState = *reinterpret_cast<void**>(pDefaultStateTable + 0x1F0);
    *reinterpret_cast<void**>(pCtxBytes + 0x0210) = pDepthStencilState;
    pCtxBytes[0x0218] = stencilRef;
    *reinterpret_cast<void**>(pHwState + 0xBC08) = pDepthStencilState;
    pHwState[0xBCB4] = stencilRef;

    // 4 fullscreen quad vertices (stride 0x20: pos4 + color4) from rodata 0x49206e0..0x4920710
    alignas(16) float quadVerts[4][8] = {
        {-1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f}
    };

    using DrawFn = void (*)(GXDynamicGeometryDrawer*, uint32_t, uint32_t, const void*, uint32_t);
    auto fnDraw = reinterpret_cast<DrawFn>(pDrawCtx->m_pDrawer->vtable[0]);
    fnDraw(pDrawCtx->m_pDrawer, 5 /* TriangleStrip */, 4, quadVerts, 0x20);

    sub_e47290(pRtGroup, 1);
    *reinterpret_cast<void**>(pCtxBytes + 0x1EC0) = sub_e47840(pRtGroup);
    pCtxBytes[0x1670] &= 0xFE;
}

// [VMA: 0x2190750 - 0x21908c4] (372 B) — Draw textured fullscreen quad using preset 2 (Single2DTexture)
void GXDebugPrimitiveRenderer_DrawTexturedFullscreenQuad(
    GXDebugDrawContext* pDrawCtx,
    GXTextureResource* pTexture)
{
    auto* pRenderCtx = *reinterpret_cast<GXRenderContext**>(pDrawCtx->m_pCommandCtx);
    auto* pCtxBytes = reinterpret_cast<uint8_t*>(pRenderCtx);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultShaders = *reinterpret_cast<GXDefaultShaderPack**>(pDeviceBytes + 0x388);

    if (!GXDefaultShaderPack_BindPresetPipelineToContext(pDefaultShaders, pRenderCtx, 2, 0)) {
        return;
    }

    auto* pTexBase = reinterpret_cast<GXShaderResourceBase*>(pTexture);
    void* pHwSrv = pTexBase->vtable->GetHardwareShader(pTexBase);

    *reinterpret_cast<void**>(pCtxBytes + 0x0788) = pHwSrv;
    *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0960) = 0;
    uint32_t srvEnd = *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0964);
    *reinterpret_cast<uint32_t*>(pCtxBytes + 0x0964) = (srvEnd > 1u) ? srvEnd : 1u;

    auto* pHwState = *reinterpret_cast<uint8_t**>(pCtxBytes + 0x08);
    *reinterpret_cast<uint64_t*>(pHwState + 0x1968) |= 0x1ULL;
    *reinterpret_cast<void**>(pHwState + 0xD128) = pHwSrv;

    // 4 fullscreen quad vertices with UVs (stride 0x30: pos4 + color4 + uv4) from 0x49206a0..0x49206d0
    alignas(16) float quadVerts[4][12] = {
        {-1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 0.0f}
    };

    using DrawFn = void (*)(GXDynamicGeometryDrawer*, uint32_t, uint32_t, const void*, uint32_t);
    auto fnDraw = reinterpret_cast<DrawFn>(pDrawCtx->m_pDrawer->vtable[0]);
    fnDraw(pDrawCtx->m_pDrawer, 5 /* TriangleStrip */, 4, quadVerts, 0x30);
}

// [VMA: 0x2190660 - 0x2190745] (229 B) — Emit 4 fullscreen quad vertices with UVs (stride 0x30) without changing shader state
void GXDebugPrimitiveRenderer_EmitFullscreenQuadVertices(GXDebugDrawContext* pDrawCtx)
{
    alignas(16) float quadVerts[4][12] = {
        {-1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 0.0f}
    };

    using DrawFn = void (*)(GXDynamicGeometryDrawer*, uint32_t, uint32_t, const void*, uint32_t);
    auto fnDraw = reinterpret_cast<DrawFn>(pDrawCtx->m_pDrawer->vtable[0]);
    fnDraw(pDrawCtx->m_pDrawer, 5 /* TriangleStrip */, 4, quadVerts, 0x30);
}

// [VMA: 0x21900a0 - 0x2190660] (1472 B) — Draw 3D wireframe bounding box (AABB / OBB) with optional RGB local axis edges
void GXDebugPrimitiveRenderer_DrawWireframeBox(
    GXDebugPrimitiveRenderer* pThis,
    const float (*pWorldMatrix)[4],
    const float (*pAABB)[4],
    uint32_t packedColorRGBA8,
    uint8_t bUseSimpleIndexTable,
    uint8_t bCenterPivotTransform)
{
    if (!pThis->m_bPipelineBound) {
        return;
    }

    auto* pCmdCtx = reinterpret_cast<uint8_t*>(pThis->m_pDrawCtx->m_pCommandCtx);
    auto* pRenderCtx = *reinterpret_cast<GXRenderContext**>(pCmdCtx);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultShaders = *reinterpret_cast<GXDefaultShaderPack**>(pDeviceBytes + 0x388);

    alignas(16) float corners[8][4];
    sub_26681d0(pAABB, corners);

    const auto* pViewProj = reinterpret_cast<const __m128*>(pCmdCtx + 0xD0);
    __m128 vpRow0 = pViewProj[0];
    __m128 vpRow1 = pViewProj[1];
    __m128 vpRow2 = pViewProj[2];
    __m128 vpRow3 = pViewProj[3];

    alignas(16) __m128 mvpMatrix[4];

    if (bCenterPivotTransform != 0) {
        __m128 aabbMin = _mm_load_ps(pAABB[0]);
        __m128 aabbMax = _mm_load_ps(pAABB[1]);
        __m128 center = _mm_mul_ps(_mm_add_ps(aabbMin, aabbMax), _mm_set1_ps(0.5f));
        __m128 centerXYZ0 = _mm_and_ps(center, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)));

        __m128 wRow0 = _mm_load_ps(pWorldMatrix[0]);
        __m128 wRow1 = _mm_load_ps(pWorldMatrix[1]);
        __m128 wRow2 = _mm_load_ps(pWorldMatrix[2]);
        __m128 wRow3 = _mm_load_ps(pWorldMatrix[3]);

        for (int i = 0; i < 8; ++i) {
            __m128 rel = _mm_sub_ps(_mm_load_ps(corners[i]), centerXYZ0);
            __m128 x = _mm_shuffle_ps(rel, rel, 0x00);
            __m128 y = _mm_shuffle_ps(rel, rel, 0x55);
            __m128 z = _mm_shuffle_ps(rel, rel, 0xAA);
            __m128 w = _mm_shuffle_ps(rel, rel, 0xFF);
            __m128 transformed = _mm_add_ps(
                centerXYZ0,
                _mm_add_ps(_mm_add_ps(_mm_mul_ps(wRow0, x), _mm_mul_ps(wRow1, y)),
                           _mm_add_ps(_mm_mul_ps(wRow2, z), _mm_mul_ps(wRow3, w))));
            _mm_store_ps(corners[i], transformed);
        }

        mvpMatrix[0] = vpRow0;
        mvpMatrix[1] = vpRow1;
        mvpMatrix[2] = vpRow2;
        mvpMatrix[3] = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(center, center, 0x00), vpRow0),
                       _mm_mul_ps(_mm_shuffle_ps(center, center, 0xAA), vpRow2)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(center, center, 0x55), vpRow1),
                       _mm_mul_ps(_mm_shuffle_ps(center, center, 0xFF), vpRow3)));
    } else {
        for (int r = 0; r < 4; ++r) {
            __m128 row = _mm_load_ps(pWorldMatrix[r]);
            __m128 x = _mm_shuffle_ps(row, row, 0x00);
            __m128 y = _mm_shuffle_ps(row, row, 0x55);
            __m128 z = _mm_shuffle_ps(row, row, 0xAA);
            __m128 w = _mm_shuffle_ps(row, row, 0xFF);
            mvpMatrix[r] = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(x, vpRow0), _mm_mul_ps(z, vpRow2)),
                _mm_add_ps(_mm_mul_ps(y, vpRow1), _mm_mul_ps(w, vpRow3)));
        }
    }

    sub_2192170(pDefaultShaders, pRenderCtx, mvpMatrix, 0);

    const float inv255 = 1.0f / 255.0f;
    float rCol = static_cast<float>((packedColorRGBA8 >> 16) & 0xFFu) * inv255;
    float gCol = static_cast<float>((packedColorRGBA8 >> 8) & 0xFFu) * inv255;
    float bCol = static_cast<float>(packedColorRGBA8 & 0xFFu) * inv255;
    float aCol = static_cast<float>(packedColorRGBA8 >> 24) * inv255;
    __m128 baseColor = _mm_set_ps(aCol, bCol, gCol, rCol);

    // Build 14 vertices (8 box corners + 3 RGB axis edge pairs, 0x20 bytes per vertex)
    alignas(16) __m128 boxVertices[14][2];
    for (int i = 0; i < 8; ++i) {
        boxVertices[i][0] = _mm_load_ps(corners[i]);
        boxVertices[i][1] = baseColor;
    }

    // Axis edge X (corners 0 -> 1, Red: 0x4920630)
    __m128 redColor = _mm_set_ps(1.0f, 0.0f, 0.0f, 1.0f);
    boxVertices[8][0] = boxVertices[0][0];
    boxVertices[8][1] = redColor;
    boxVertices[9][0] = boxVertices[1][0];
    boxVertices[9][1] = redColor;

    // Axis edge Y (corners 0 -> 3, Green: 0x4920640)
    __m128 greenColor = _mm_set_ps(1.0f, 0.0f, 1.0f, 0.0f);
    boxVertices[10][0] = boxVertices[0][0];
    boxVertices[10][1] = greenColor;
    boxVertices[11][0] = boxVertices[3][0];
    boxVertices[11][1] = greenColor;

    // Axis edge Z (corners 0 -> 4, Blue: 0x4920650)
    __m128 blueColor = _mm_set_ps(1.0f, 1.0f, 0.0f, 0.0f);
    boxVertices[12][0] = boxVertices[0][0];
    boxVertices[12][1] = blueColor;
    boxVertices[13][0] = boxVertices[4][0];
    boxVertices[13][1] = blueColor;

    // 24 uint16_t indices (12 wireframe edges): standard (0x4738d80) vs colored-axes (0x4738db0)
    static const uint16_t kBoxIndicesStandard[24] = {
        0, 1,  1, 2,  2, 3,  3, 0,
        4, 5,  5, 6,  6, 7,  7, 4,
        0, 4,  1, 5,  2, 6,  3, 7
    };
    static const uint16_t kBoxIndicesColoredAxes[24] = {
        8, 9,  1, 2,  2, 3,  10, 11,
        4, 5,  5, 6,  6, 7,  7, 4,
        12, 13, 1, 5,  2, 6,  3, 7
    };
    const uint16_t* pIndices = (bUseSimpleIndexTable != 0) ? kBoxIndicesColoredAxes : kBoxIndicesStandard;

    using DrawIndexedFn = void (*)(
        GXDynamicGeometryDrawer*, uint32_t, int32_t, uint32_t, uint32_t, const void*, uint32_t, const void*, uint32_t);
    auto fnDrawIndexed = reinterpret_cast<DrawIndexedFn>(pThis->m_pDrawCtx->m_pDrawer->vtable[1]);
    fnDrawIndexed(
        pThis->m_pDrawCtx->m_pDrawer,
        2 /* LineList */,
        0,
        13,
        24,
        pIndices,
        0x39 /* 16-bit index format */,
        boxVertices,
        0x20);
}

// [VMA: 0x218fcf0 - 0x219009a] (938 B) — Draw 3D wireframe ground grid on XZ plane
void GXDebugPrimitiveRenderer_DrawWireframeGrid(
    GXDebugPrimitiveRenderer* pThis,
    const float (*pWorldMatrix)[4],
    int32_t gridLineCount,
    uint32_t packedColorRGBA8,
    float gridExtent)
{
    if (!pThis->m_bPipelineBound || gridLineCount < 2) {
        return;
    }

    auto* pCmdCtx = reinterpret_cast<uint8_t*>(pThis->m_pDrawCtx->m_pCommandCtx);
    auto* pRenderCtx = *reinterpret_cast<GXRenderContext**>(pCmdCtx);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultShaders = *reinterpret_cast<GXDefaultShaderPack**>(pDeviceBytes + 0x388);

    const auto* pViewProj = reinterpret_cast<const __m128*>(pCmdCtx + 0xD0);
    __m128 vpRow0 = pViewProj[0];
    __m128 vpRow1 = pViewProj[1];
    __m128 vpRow2 = pViewProj[2];
    __m128 vpRow3 = pViewProj[3];

    alignas(16) __m128 mvpMatrix[4];
    for (int r = 0; r < 4; ++r) {
        __m128 row = _mm_load_ps(pWorldMatrix[r]);
        __m128 x = _mm_shuffle_ps(row, row, 0x00);
        __m128 y = _mm_shuffle_ps(row, row, 0x55);
        __m128 z = _mm_shuffle_ps(row, row, 0xAA);
        __m128 w = _mm_shuffle_ps(row, row, 0xFF);
        mvpMatrix[r] = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(x, vpRow0), _mm_mul_ps(z, vpRow2)),
            _mm_add_ps(_mm_mul_ps(y, vpRow1), _mm_mul_ps(w, vpRow3)));
    }
    sub_2192170(pDefaultShaders, pRenderCtx, mvpMatrix, 0);

    const float inv255 = 1.0f / 255.0f;
    float rCol = static_cast<float>((packedColorRGBA8 >> 16) & 0xFFu) * inv255;
    float gCol = static_cast<float>((packedColorRGBA8 >> 8) & 0xFFu) * inv255;
    float bCol = static_cast<float>(packedColorRGBA8 & 0xFFu) * inv255;
    float aCol = static_cast<float>(packedColorRGBA8 >> 24) * inv255;
    __m128 lineColor = _mm_set_ps(aCol, bCol, gCol, rCol);

    if (gridLineCount == 0) {
        return;
    }

    float minCoord = gridExtent * -0.5f;
    float maxCoord = gridExtent *  0.5f;
    float step = gridExtent / static_cast<float>(gridLineCount - 1);

    alignas(16) __m128 lineVerts[2][2];
    lineVerts[0][1] = lineColor;
    lineVerts[1][1] = lineColor;

    using DrawFn = void (*)(GXDynamicGeometryDrawer*, uint32_t, uint32_t, const void*, uint32_t);
    auto fnDraw = reinterpret_cast<DrawFn>(pThis->m_pDrawCtx->m_pDrawer->vtable[0]);

    // Lines parallel to Z axis (varying X from minCoord to maxCoord)
    float curX = minCoord;
    for (int32_t i = 0; i < gridLineCount; ++i) {
        lineVerts[0][0] = _mm_set_ps(1.0f, minCoord, 0.0f, curX);
        lineVerts[1][0] = _mm_set_ps(1.0f, maxCoord, 0.0f, curX);
        fnDraw(pThis->m_pDrawCtx->m_pDrawer, 3 /* LineStrip */, 2, lineVerts, 0x20);
        curX += step;
    }

    // Lines parallel to X axis (varying Z from minCoord to maxCoord)
    float curZ = minCoord;
    for (int32_t i = 0; i < gridLineCount; ++i) {
        lineVerts[0][0] = _mm_set_ps(1.0f, curZ, 0.0f, minCoord);
        lineVerts[1][0] = _mm_set_ps(1.0f, curZ, 0.0f, maxCoord);
        fnDraw(pThis->m_pDrawCtx->m_pDrawer, 3 /* LineStrip */, 2, lineVerts, 0x20);
        curZ += step;
    }
}

// [VMA: 0x218fa30 - 0x218fcec] (700 B) — Draw 3D RGB coordinate axes triad for a 4x4 transform matrix
void GXDebugPrimitiveRenderer_DrawTransformAxes(
    GXDebugPrimitiveRenderer* pThis,
    const float (*pTransformMatrix)[4],
    uint32_t packedColorRGBA8,
    float axisLength)
{
    if (!pThis->m_bPipelineBound) {
        return;
    }

    auto* pCmdCtx = reinterpret_cast<uint8_t*>(pThis->m_pDrawCtx->m_pCommandCtx);
    auto* pRenderCtx = *reinterpret_cast<GXRenderContext**>(pCmdCtx);
    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultShaders = *reinterpret_cast<GXDefaultShaderPack**>(pDeviceBytes + 0x388);

    // Upload ViewProj * Identity to VS CB slot 0
    const auto* pViewProj = reinterpret_cast<const __m128*>(pCmdCtx + 0xD0);
    alignas(16) __m128 mvpMatrix[4] = {
        pViewProj[0], pViewProj[1], pViewProj[2], pViewProj[3]
    };
    sub_2192170(pDefaultShaders, pRenderCtx, mvpMatrix, 0);

    __m128 origin = _mm_load_ps(pTransformMatrix[3]);
    __m128 scaleVec = _mm_set1_ps(axisLength);

    const float inv255 = 1.0f / 255.0f;
    float rIntensity = static_cast<float>((packedColorRGBA8 >> 16) & 0xFFu) * inv255;
    float gIntensity = static_cast<float>((packedColorRGBA8 >> 8) & 0xFFu) * inv255;
    float bIntensity = static_cast<float>(packedColorRGBA8 & 0xFFu) * inv255;
    float aIntensity = static_cast<float>(packedColorRGBA8 >> 24) * inv255;

    alignas(16) __m128 lineVerts[2][2];
    lineVerts[0][0] = origin;

    using DrawFn = void (*)(GXDynamicGeometryDrawer*, uint32_t, uint32_t, const void*, uint32_t);
    auto fnDraw = reinterpret_cast<DrawFn>(pThis->m_pDrawCtx->m_pDrawer->vtable[0]);

    // X axis: origin -> origin + row0 * axisLength, color = (rIntensity, 0, 0, aIntensity)
    lineVerts[1][0] = _mm_add_ps(_mm_mul_ps(scaleVec, _mm_load_ps(pTransformMatrix[0])), origin);
    __m128 xCol = _mm_set_ps(aIntensity, 0.0f, 0.0f, rIntensity);
    lineVerts[0][1] = xCol;
    lineVerts[1][1] = xCol;
    fnDraw(pThis->m_pDrawCtx->m_pDrawer, 2 /* LineList */, 2, lineVerts, 0x20);

    // Y axis: origin -> origin + row1 * axisLength, color0 = (0, gIntensity, 0, aIntensity)
    lineVerts[1][0] = _mm_add_ps(_mm_mul_ps(scaleVec, _mm_load_ps(pTransformMatrix[1])), origin);
    __m128 yCol = _mm_set_ps(aIntensity, 0.0f, gIntensity, 0.0f);
    lineVerts[0][1] = yCol;
    lineVerts[1][1] = yCol;
    fnDraw(pThis->m_pDrawCtx->m_pDrawer, 2 /* LineList */, 2, lineVerts, 0x20);

    // Z axis: origin -> origin + row2 * axisLength, color = (0, 0, bIntensity, aIntensity)
    lineVerts[1][0] = _mm_add_ps(_mm_mul_ps(scaleVec, _mm_load_ps(pTransformMatrix[2])), origin);
    __m128 zCol = _mm_set_ps(aIntensity, bIntensity, 0.0f, 0.0f);
    lineVerts[0][1] = zCol;
    lineVerts[1][1] = zCol;
    fnDraw(pThis->m_pDrawCtx->m_pDrawer, 2 /* LineList */, 2, lineVerts, 0x20);
}

// [VMA: 0x218f9d0 - 0x218fa2b] (91 B) — Bind wireframe debug pipeline (preset 3) and upload ViewProj matrix to VS slot 0
void GXDebugPrimitiveRenderer_PrepareWireframePipeline(
    GXDebugPrimitiveRenderer* pThis,
    GXDebugDrawContext* pDrawCtx)
{
    pThis->m_pDrawCtx = pDrawCtx;
    GXCommandContext* pCmdCtx = pDrawCtx->m_pCommandCtx;
    GXRenderContext* pRenderCtx = *reinterpret_cast<GXRenderContext**>(pCmdCtx);

    auto* pDeviceBytes = reinterpret_cast<uint8_t*>(g_pGXDeviceSingleton);
    auto* pDefaultShaders = *reinterpret_cast<GXDefaultShaderPack**>(pDeviceBytes + 0x388);

    pThis->m_bPipelineBound = GXDefaultShaderPack_BindPresetPipelineToContext(
        pDefaultShaders, pRenderCtx, 3, 0);

    const void* pViewProj = reinterpret_cast<const uint8_t*>(pCmdCtx) + 0xD0;
    sub_2192170(pDefaultShaders, pRenderCtx, pViewProj, 0);
}

// ============================================================================
// 3. GXGParamBinaryReader (0x218f470 .. 0x218f8f0)
//    Reader for FromSoftware ".gparam" ("filt" v3) graphic parameter files
// ============================================================================

struct GParamFileHeaderV3 {
    char16_t  magic[4];                  // +0x00: L"filt"
    uint32_t version;                   // +0x08: 3
    uint32_t unk0C;                     // +0x0C
    uint32_t groupCount;                // +0x10
    uint32_t unkBlock2Count;            // +0x14
    uint32_t groupHeaderOffsetsOffset;  // +0x18
    uint32_t groupHeadersOffset;        // +0x1C
    uint32_t paramHeaderOffsetsOffset;  // +0x20
    uint32_t paramHeadersOffset;        // +0x24
    uint32_t valuesOffset;              // +0x28
    uint32_t valueIdsOffset;            // +0x2C
    uint32_t unkBlock2Offset;           // +0x30
    uint32_t effectGroupCount;          // +0x34
    uint32_t effectGroupsOffset;        // +0x38
    uint32_t effectGroupValuesOffset;   // +0x3C
    uint32_t unk40;                     // +0x40
    uint32_t commentOffsetsTableOffset; // +0x44
    uint32_t commentGroupOffsetsOffset; // +0x48
    uint32_t commentStringsOffset;      // +0x4C
};
static_assert(sizeof(GParamFileHeaderV3) == 0x50, "PS4 GParamFileHeaderV3 must be 0x50 bytes");

struct GXGParamBinaryReader {
    void*                     vtable;    // +0x00 (0x53b63f0)
    bool                      m_bValid;  // +0x08
    uint32_t                  m_version; // +0x0C
    const GParamFileHeaderV3* m_pHeader; // +0x10
};

// [VMA: 0x218f8f0 - 0x218f931] (65 B) — Lookup value at index within an effect ID group (offsets +0x34, +0x38, +0x3C)
uint32_t GXGParamBinaryReader_GetEffectIdGroupValue(
    const GXGParamBinaryReader* pThis,
    uint32_t effectGroupId,
    uint32_t elementIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    uint32_t count = pThis->m_pHeader->effectGroupCount;
    const auto* pEntries = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->effectGroupsOffset);

    for (uint32_t i = 0; i < count; ++i, pEntries += 3) {
        if (pEntries[0] == effectGroupId) {
            uint32_t subOffset = pEntries[2] + pThis->m_pHeader->effectGroupValuesOffset;
            const auto* pValues = reinterpret_cast<const uint32_t*>(pBase + subOffset);
            return pValues[elementIndex];
        }
    }
    return 0;
}

// [VMA: 0x218f8b0 - 0x218f8e4] (52 B) — Get element count of an effect ID group by group ID
uint32_t GXGParamBinaryReader_GetEffectIdGroupCount(
    const GXGParamBinaryReader* pThis,
    uint32_t effectGroupId)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    uint32_t count = pThis->m_pHeader->effectGroupCount;
    const auto* pEntries = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->effectGroupsOffset);

    for (uint32_t i = 0; i < count; ++i, pEntries += 3) {
        if (pEntries[0] == effectGroupId) {
            return pEntries[1];
        }
    }
    return 0;
}

// [VMA: 0x218f870 - 0x218f8a4] (52 B) — Check whether an effect ID group with the given ID exists
bool GXGParamBinaryReader_HasEffectIdGroup(
    const GXGParamBinaryReader* pThis,
    uint32_t effectGroupId)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    uint32_t count = pThis->m_pHeader->effectGroupCount;
    const auto* pEntries = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->effectGroupsOffset);

    for (uint32_t i = 0; i < count; ++i, pEntries += 3) {
        if (pEntries[0] == effectGroupId) {
            return true;
        }
    }
    return false;
}

// [VMA: 0x218f850 - 0x218f866] (22 B) — Get pointer to UnkBlock2 entry by index
const void* GXGParamBinaryReader_GetUnkBlock2EntryByIndex(
    const GXGParamBinaryReader* pThis,
    uint32_t entryIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    uint32_t strideBytes = pThis->m_pHeader->groupCount + 1;
    return pBase + static_cast<size_t>(pThis->m_pHeader->unkBlock2Offset) + static_cast<size_t>(entryIndex * strideBytes);
}

// [VMA: 0x218f800 - 0x218f84c] (76 B) — Find UnkBlock2 entry by ID
const void* GXGParamBinaryReader_FindUnkBlock2ById(
    const GXGParamBinaryReader* pThis,
    uint32_t targetId)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    uint32_t count = pThis->m_pHeader->unkBlock2Count;
    if (count == 0) {
        return nullptr;
    }

    uint32_t stride = pThis->m_pHeader->groupCount + 1;
    uint32_t offset = 0;
    const void* pFound = nullptr;
    for (uint32_t i = 0; i < count; ++i, offset += stride) {
        const auto* pEntry = pBase + static_cast<size_t>(pThis->m_pHeader->unkBlock2Offset) + static_cast<size_t>(offset);
        if (*reinterpret_cast<const uint32_t*>(pEntry) == targetId) {
            pFound = pEntry;
            break;
        }
    }
    return pFound;
}

// [VMA: 0x218f7f0 - 0x218f7f8] (8 B) — Get UnkBlock2 entry count (+0x14)
uint32_t GXGParamBinaryReader_GetUnkBlock2Count(const GXGParamBinaryReader* pThis)
{
    return pThis->m_pHeader->unkBlock2Count;
}

// [VMA: 0x218f7a0 - 0x218f7eb] (75 B) — Get pointer to parameter value ID array and array count
bool GXGParamBinaryReader_GetParamValueIdListPtr(
    const GXGParamBinaryReader* pThis,
    const void** ppOutIds,
    uint8_t* pOutCount,
    uint32_t groupIndex,
    uint32_t paramIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupOffsets = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->groupHeaderOffsetsOffset);
    const auto* pGroupHdr = pBase + pThis->m_pHeader->groupHeadersOffset + pGroupOffsets[groupIndex];
    uint32_t paramOffsetsBase = *reinterpret_cast<const uint32_t*>(pGroupHdr + 0x04);

    const auto* pParamOffsets = reinterpret_cast<const uint32_t*>(
        pBase + pThis->m_pHeader->paramHeaderOffsetsOffset + paramOffsetsBase);
    const auto* pParamHdr = pBase + pThis->m_pHeader->paramHeadersOffset + pParamOffsets[paramIndex];

    *pOutCount = pParamHdr[0x09];
    uint32_t idOffset = *reinterpret_cast<const uint32_t*>(pParamHdr + 0x04);
    *ppOutIds = pBase + pThis->m_pHeader->valueIdsOffset + idOffset;
    return true;
}

// [VMA: 0x218f750 - 0x218f79c] (76 B) — Get pointer to parameter raw value array, count, and parameter type
uint8_t GXGParamBinaryReader_GetParamValuePtrAndCount(
    const GXGParamBinaryReader* pThis,
    const void** ppOutValues,
    uint8_t* pOutCount,
    uint32_t groupIndex,
    uint32_t paramIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupOffsets = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->groupHeaderOffsetsOffset);
    const auto* pGroupHdr = pBase + pThis->m_pHeader->groupHeadersOffset + pGroupOffsets[groupIndex];
    uint32_t paramOffsetsBase = *reinterpret_cast<const uint32_t*>(pGroupHdr + 0x04);

    const auto* pParamOffsets = reinterpret_cast<const uint32_t*>(
        pBase + pThis->m_pHeader->paramHeaderOffsetsOffset + paramOffsetsBase);
    const auto* pParamHdr = pBase + pThis->m_pHeader->paramHeadersOffset + pParamOffsets[paramIndex];

    *pOutCount = pParamHdr[0x09];
    uint32_t valOffset = *reinterpret_cast<const uint32_t*>(pParamHdr + 0x00);
    *ppOutValues = pBase + pThis->m_pHeader->valuesOffset + valOffset;
    return pParamHdr[0x08];
}

extern "C" {
    int    sub_2bbf3b8(const void* a, const void* b);               // 0x2bbf3b8: wcscmp (UTF-16LE)
    size_t sub_2bbf228(const void* s);                              // 0x2bbf228: wcslen (UTF-16LE)
}

// Helper for 4-wchar aligned wide string skipping in GParam headers
static inline const char16_t* SkipAlignedGParamWString(const char16_t* pStr)
{
    int32_t lenPlus2 = static_cast<int32_t>(sub_2bbf228(pStr)) + 2;
    int32_t rem = lenPlus2 - (lenPlus2 & ~3);
    if (rem != 0) {
        lenPlus2 += (4 - rem);
    }
    return pStr + lenPlus2;
}

// [VMA: 0x218f690 - 0x218f74c] (188 B) — Get parameter value pointer, name & comment wide strings, array count, and type
uint8_t GXGParamBinaryReader_GetParamMetadataAndValues(
    const GXGParamBinaryReader* pThis,
    const void** ppOutValues,
    const char16_t** ppOutNameW,
    const char16_t** ppOutCommentW,
    uint8_t* pOutCount,
    uint32_t groupIndex,
    uint32_t paramIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupOffsets = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->groupHeaderOffsetsOffset);
    const auto* pGroupHdr = pBase + pThis->m_pHeader->groupHeadersOffset + pGroupOffsets[groupIndex];
    uint32_t paramOffsetsBase = *reinterpret_cast<const uint32_t*>(pGroupHdr + 0x04);

    const auto* pParamOffsets = reinterpret_cast<const uint32_t*>(
        pBase + pThis->m_pHeader->paramHeaderOffsetsOffset + paramOffsetsBase);
    const auto* pParamHdr = pBase + pThis->m_pHeader->paramHeadersOffset + pParamOffsets[paramIndex];

    const auto* pNameW = reinterpret_cast<const char16_t*>(pParamHdr + 0x0C);
    if (ppOutNameW != nullptr) {
        *ppOutNameW = pNameW;
    }
    *ppOutCommentW = SkipAlignedGParamWString(pNameW);
    *pOutCount = pParamHdr[0x09];
    uint32_t valOffset = *reinterpret_cast<const uint32_t*>(pParamHdr + 0x00);
    *ppOutValues = pBase + pThis->m_pHeader->valuesOffset + valOffset;
    return pParamHdr[0x08];
}

// [VMA: 0x218f660 - 0x218f681] (33 B) — Get comment string by (groupIndex, entryIndex) from comment table (+0x44/+0x48/+0x4C)
const char16_t* GXGParamBinaryReader_GetCommentStringW(
    const GXGParamBinaryReader* pThis,
    uint32_t groupIndex,
    uint32_t entryIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupTable = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->commentOffsetsTableOffset);
    const auto* pEntryTable = reinterpret_cast<const uint32_t*>(
        pBase + pThis->m_pHeader->commentGroupOffsetsOffset + pGroupTable[groupIndex]);
    return reinterpret_cast<const char16_t*>(pBase + pThis->m_pHeader->commentStringsOffset + pEntryTable[entryIndex]);
}

// [VMA: 0x218f620 - 0x218f653] (51 B) — Get parameter internal identifier string (UTF-16)
const char16_t* GXGParamBinaryReader_GetParamNameW(
    const GXGParamBinaryReader* pThis,
    uint32_t groupIndex,
    uint32_t paramIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupOffsets = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->groupHeaderOffsetsOffset);
    const auto* pGroupHdr = pBase + pThis->m_pHeader->groupHeadersOffset + pGroupOffsets[groupIndex];
    uint32_t paramOffsetsBase = *reinterpret_cast<const uint32_t*>(pGroupHdr + 0x04);

    const auto* pParamOffsets = reinterpret_cast<const uint32_t*>(
        pBase + pThis->m_pHeader->paramHeaderOffsetsOffset + paramOffsetsBase);
    const auto* pParamHdr = pBase + pThis->m_pHeader->paramHeadersOffset + pParamOffsets[paramIndex];
    return reinterpret_cast<const char16_t*>(pParamHdr + 0x0C);
}

// [VMA: 0x218f5b0 - 0x218f61f] (111 B) — Get parameter display/comment string (UTF-16)
const char16_t* GXGParamBinaryReader_GetParamCommentW(
    const GXGParamBinaryReader* pThis,
    uint32_t groupIndex,
    uint32_t paramIndex)
{
    return SkipAlignedGParamWString(GXGParamBinaryReader_GetParamNameW(pThis, groupIndex, paramIndex));
}

// [VMA: 0x218f590 - 0x218f5a9] (25 B) — Get number of parameters in group
uint32_t GXGParamBinaryReader_GetGroupParamCount(
    const GXGParamBinaryReader* pThis,
    uint32_t groupIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupOffsets = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->groupHeaderOffsetsOffset);
    const auto* pGroupHdr = pBase + pThis->m_pHeader->groupHeadersOffset + pGroupOffsets[groupIndex];
    return *reinterpret_cast<const uint32_t*>(pGroupHdr);
}

// [VMA: 0x218f570 - 0x218f58b] (27 B) — Get group internal identifier string (UTF-16)
const char16_t* GXGParamBinaryReader_GetGroupNameW(
    const GXGParamBinaryReader* pThis,
    uint32_t groupIndex)
{
    const auto* pBase = reinterpret_cast<const uint8_t*>(pThis->m_pHeader);
    const auto* pGroupOffsets = reinterpret_cast<const uint32_t*>(pBase + pThis->m_pHeader->groupHeaderOffsetsOffset);
    const auto* pGroupHdr = pBase + pThis->m_pHeader->groupHeadersOffset + pGroupOffsets[groupIndex];
    return reinterpret_cast<const char16_t*>(pGroupHdr + 0x08);
}

// [VMA: 0x218f510 - 0x218f567] (87 B) — Get group display/comment string (UTF-16)
const char16_t* GXGParamBinaryReader_GetGroupCommentW(
    const GXGParamBinaryReader* pThis,
    uint32_t groupIndex)
{
    return SkipAlignedGParamWString(GXGParamBinaryReader_GetGroupNameW(pThis, groupIndex));
}

// [VMA: 0x218f500 - 0x218f508] (8 B) — Get total number of parameter groups in GParam file
uint32_t GXGParamBinaryReader_GetGroupCount(const GXGParamBinaryReader* pThis)
{
    return pThis->m_pHeader->groupCount;
}

// [VMA: 0x218f4e0 - 0x218f4f4] (20 B) — Reset GParam binary reader state
void GXGParamBinaryReader_Reset(GXGParamBinaryReader* pThis)
{
    pThis->m_bValid = false;
    pThis->m_version = 0;
    pThis->m_pHeader = nullptr;
}

// [VMA: 0x218f4a0 - 0x218f4e0] (64 B) — Initialize GParam binary reader from memory buffer and validate L"filt" magic
bool GXGParamBinaryReader_InitializeFromBuffer(
    GXGParamBinaryReader* pThis,
    const void* pFileBuffer)
{
    pThis->m_pHeader = static_cast<const GParamFileHeaderV3*>(pFileBuffer);
    auto fnWcsncmp = reinterpret_cast<int (*)(const void*, const void*, size_t)>(GX_VMA(0x2bbf3e8ULL));
    if (fnWcsncmp(pFileBuffer, reinterpret_cast<const void*>(GX_VMA(0x49c2baaULL)), 4) != 0) {
        return false;
    }
    pThis->m_bValid = true;
    pThis->m_version = 3;
    return true;
}

// [VMA: 0x218f490 - 0x218f491] (1 B) — Base destructor stub
void GXGParamBinaryReader_BaseDtor(GXGParamBinaryReader*) {}

// [VMA: 0x218f470 - 0x218f48e] (30 B) — Construct empty GXGParamBinaryReader
void GXGParamBinaryReader_Construct(GXGParamBinaryReader* pThis)
{
#undef g_GXGParamBinaryReaderVTable
#define g_GXGParamBinaryReaderVTable (reinterpret_cast<void**>(GX_VMA(0x53b63f0ULL)))
    pThis->m_bValid = false;
    pThis->m_version = 0;
    pThis->m_pHeader = nullptr;
    pThis->vtable = g_GXGParamBinaryReaderVTable;
}

// ============================================================================
// 4. GXGParamGroupLookupTable (0x218e1b0 .. 0x218f3b0)
// ============================================================================

struct GParamIdLookupNode {
    uint32_t            m_id;            // +0x00
    uint32_t            m_pad04;         // +0x04
    uint32_t*           m_pValueIndices; // +0x08
    GParamIdLookupNode* m_pPrev;         // +0x10
    GParamIdLookupNode* m_pNext;         // +0x18
};

struct DLWString {
    void*        m_pProxy; // +0x00
    union {
        char16_t  m_buf[8];
        char16_t* m_pPtr;
    } m_u;                 // +0x08
    uint64_t     m_size;   // +0x18
    uint64_t     m_cap;    // +0x20
    DLAllocator* m_pAlloc; // +0x28
    uint8_t      m_flag30; // +0x30
    uint8_t      m_pad31[7];
};
static_assert(sizeof(DLWString) == 0x38, "PS4 DLWString must be 0x38 bytes");

struct DLWStringVector {
    void*        m_pProxy; // +0x00
    DLWString*   m_pBegin; // +0x08
    DLWString*   m_pEnd;   // +0x10
    DLWString*   m_pCap;   // +0x18
    DLAllocator* m_pAlloc; // +0x20
};

struct GParamRuntimeParamEntry {
    void*          m_unk00;     // +0x00
    const char16_t* m_pNameW;    // +0x08
    const char16_t* m_pCommentW; // +0x10
    uint8_t        m_rest[24];  // +0x18 .. +0x30
};

struct GParamRuntimeGroupData {
    void*                    m_unk00;  // +0x00
    void*                    m_unk08;  // +0x08
    GParamRuntimeParamEntry* m_pBegin; // +0x10
    GParamRuntimeParamEntry* m_pEnd;   // +0x18
};

struct GXGParamBinaryReaderVTable {
    bool           (*InitializeFromBuffer)(GXGParamBinaryReader*, const void*);  // +0x00 (0x218f4a0)
    void           (*Reset)(GXGParamBinaryReader*);                              // +0x08 (0x218f4e0)
    uint32_t       (*GetGroupCount)(const GXGParamBinaryReader*);                // +0x10 (0x218f500)
    const char16_t* (*GetGroupCommentW)(const GXGParamBinaryReader*, uint32_t);   // +0x18 (0x218f510)
    const char16_t* (*GetGroupNameW)(const GXGParamBinaryReader*, uint32_t);      // +0x20 (0x218f570)
    uint32_t       (*GetGroupParamCount)(const GXGParamBinaryReader*, uint32_t); // +0x28 (0x218f590)
    const char16_t* (*GetParamCommentW)(const GXGParamBinaryReader*, uint32_t, uint32_t); // +0x30 (0x218f5b0)
    const char16_t* (*GetParamNameW)(const GXGParamBinaryReader*, uint32_t, uint32_t);    // +0x38 (0x218f620)
    const char16_t* (*GetCommentStringW)(const GXGParamBinaryReader*, uint32_t, uint32_t);// +0x40 (0x218f660)
    uint8_t        (*GetParamMetadataAndValues)(const GXGParamBinaryReader*, const void**, const char16_t**, const char16_t**, uint8_t*, uint32_t, uint32_t); // +0x48 (0x218f690)
    uint8_t        (*GetParamValuePtrAndCount)(const GXGParamBinaryReader*, const void**, uint8_t*, uint32_t, uint32_t); // +0x50 (0x218f750)
    bool           (*GetParamValueIdListPtr)(const GXGParamBinaryReader*, const void**, uint8_t*, uint32_t, uint32_t);   // +0x58 (0x218f7a0)
    uint32_t       (*GetUnkBlock2Count)(const GXGParamBinaryReader*);            // +0x60 (0x218f7f0)
    const void*    (*FindUnkBlock2ById)(const GXGParamBinaryReader*, uint32_t);  // +0x68 (0x218f800)
    const uint32_t*(*GetUnkBlock2EntryByIndex)(const GXGParamBinaryReader*, uint32_t); // +0x70 (0x218f850)
    bool           (*HasEffectIdGroup)(const GXGParamBinaryReader*, uint32_t);   // +0x78 (0x218f870)
    uint32_t       (*GetEffectIdGroupCount)(const GXGParamBinaryReader*, uint32_t); // +0x80 (0x218f8b0)
    uint32_t       (*GetEffectIdGroupValue)(const GXGParamBinaryReader*, uint32_t, uint32_t); // +0x88 (0x218f8f0)
};

struct GXGParamGroupLookupTable {
    DLAllocator*            m_pAllocator;      // +0x00
    GParamRuntimeGroupData* m_pGroupData;      // +0x08
    GXGParamBinaryReader*   m_pReader;         // +0x10
    char16_t**               m_ppParamNames;    // +0x18
    char16_t**               m_ppParamComments; // +0x20
    DLWStringVector         m_nameVec;         // +0x28 .. +0x50
    DLWStringVector         m_commentVec;      // +0x50 .. +0x78
    uint32_t                m_paramCount;      // +0x78
    uint32_t                m_pad7C;           // +0x7C
    GParamIdLookupNode*     m_pIdListHead;     // +0x80
};

static inline bool CaseInsensitiveAsciiWStrEquals(const char16_t* pStrA, const char16_t* pStrB)
{
    if (pStrA == pStrB) return true;
    if (!pStrA || !pStrB) return false;
    const uint16_t* pCtypeTable = sub_2bbec48();

    for (;;) {
        uint16_t ca = static_cast<uint16_t>(*pStrA++);
        uint16_t cb = static_cast<uint16_t>(*pStrB++);
        uint16_t ma = (ca < 0x100u && pCtypeTable) ? static_cast<uint16_t>(pCtypeTable[ca] & 0xFFu) : ca;
        uint16_t mb = (cb < 0x100u && pCtypeTable) ? static_cast<uint16_t>(pCtypeTable[cb] & 0xFFu) : cb;
        if (ma != mb) {
            return false;
        }
        if (ca == 0u) {
            return true;
        }
    }
}

// [VMA: 0x218f3b0 - 0x218f3d9] (41 B) — Get low byte of ID at linked-list node index
uint8_t GXGParamGroupLookupTable_GetIdByNodeIndex(
    const GXGParamGroupLookupTable* pThis,
    uint32_t nodeIndex)
{
    const GParamIdLookupNode* pNode = pThis->m_pIdListHead;
    if (pNode == nullptr) {
        return 0;
    }
    uint32_t remaining = nodeIndex + 1;
    while (--remaining != 0) {
        pNode = pNode->m_pNext;
        if (pNode == nullptr) {
            return 0;
        }
    }
    return static_cast<uint8_t>(pNode->m_id);
}

// [VMA: 0x218f380 - 0x218f3aa] (42 B) — Set parameter value index for a given (id, paramIndex)
void GXGParamGroupLookupTable_SetValueIndexForIdAndParam(
    GXGParamGroupLookupTable* pThis,
    uint32_t targetId,
    uint32_t paramIndex,
    uint32_t valueIndex)
{
    for (GParamIdLookupNode* pNode = pThis->m_pIdListHead; pNode != nullptr; pNode = pNode->m_pNext) {
        if (pNode->m_id == targetId) {
            pNode->m_pValueIndices[paramIndex] = valueIndex;
            return;
        }
    }
}

// [VMA: 0x218f360 - 0x218f37c] (28 B) — Count number of ID nodes in linked list
uint32_t GXGParamGroupLookupTable_GetIdNodeCount(const GXGParamGroupLookupTable* pThis)
{
    uint32_t count = 0;
    for (const GParamIdLookupNode* pNode = pThis->m_pIdListHead; pNode != nullptr; pNode = pNode->m_pNext) {
        ++count;
    }
    return count;
}

// [VMA: 0x218f2a0 - 0x218f353] (179 B) — Find value index and parameter index by (targetId, paramCommentW)
bool GXGParamGroupLookupTable_FindValueIndexByIdAndComment(
    const GXGParamGroupLookupTable* pThis,
    uint32_t* pOutValueIndex,
    uint32_t* pOutParamIndex,
    uint32_t targetId,
    const char16_t* pParamCommentW)
{
    *pOutValueIndex = 0;
    *pOutParamIndex = 0;

    uint32_t iterIdx = 0;
    uint32_t matchedIdx = 0;
    for (;;) {
        if (iterIdx >= pThis->m_paramCount) {
            return false;
        }
        matchedIdx = iterIdx;
        int cmp = sub_2bbf3b8(pThis->m_ppParamComments[matchedIdx], pParamCommentW);
        ++iterIdx;
        if (cmp == 0) {
            break;
        }
    }

    if (iterIdx == 0) {
        return false;
    }
    --iterIdx;

    for (const GParamIdLookupNode* pNode = pThis->m_pIdListHead; pNode != nullptr; pNode = pNode->m_pNext) {
        if (pNode->m_id == targetId) {
            *pOutValueIndex = pNode->m_pValueIndices[matchedIdx];
            *pOutParamIndex = iterIdx;
            return true;
        }
    }
    return false;
}

// [VMA: 0x218f1d0 - 0x218f295] (197 B) — Find parameter index by ASCII/wide case-insensitive comment match
bool GXGParamGroupLookupTable_FindParamIndexByCommentCaseInsensitive(
    const GXGParamGroupLookupTable* pThis,
    uint32_t* pOutParamIndex,
    const char16_t* pParamCommentW)
{
    *pOutParamIndex = 0;
    for (uint32_t i = 0; i < pThis->m_paramCount; ++i) {
        if (CaseInsensitiveAsciiWStrEquals(pThis->m_ppParamComments[i], pParamCommentW)) {
            if (i == 0xFFFFFFFFu) {
                return false;
            }
            *pOutParamIndex = i;
            return true;
        }
    }
    return false;
}

// Helper to initialize a temporary stack DLWString and push into a DLWStringVector
static void PushTempDLWStringToVector(DLWStringVector* pVec, const char16_t* pSrcW)
{
    DLAllocator* pStrAlloc = g_pGXStringAllocator;
    if (pStrAlloc == nullptr) {
        pStrAlloc = sub_2ba2b10();
        g_pGXStringAllocator = pStrAlloc;
    }
    uint8_t blockInfo[8]{};
    pStrAlloc->vtable->GetBlockInfo(blockInfo, pStrAlloc, nullptr);
    if ((blockInfo[0] & 0x20) == 0) {
        sub_20b55b0(0u, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    DLWString tempStr{};
    tempStr.m_pAlloc = pStrAlloc;
    tempStr.m_cap = 7;
    tempStr.m_size = 0;
    tempStr.m_u.m_buf[0] = u'\0';
    size_t len = (pSrcW != nullptr && pSrcW[0] != u'\0') ? sub_2bbf228(pSrcW) : 0;
    sub_262a720(&tempStr, pSrcW, len);
    tempStr.m_flag30 = 1;

    sub_27c1a20(pVec, &tempStr);

    if (tempStr.m_cap >= 8) {
        tempStr.m_pAlloc->vtable->Free(tempStr.m_pAlloc, tempStr.m_u.m_pPtr);
    }
}

// Helper to find or insert an ID node in sorted order in GXGParamGroupLookupTable::m_pIdListHead
static GParamIdLookupNode* FindOrInsertGParamIdNode(GXGParamGroupLookupTable* pThis, uint32_t targetId)
{
    GParamIdLookupNode* pCur = pThis->m_pIdListHead;
    if (pCur == nullptr) {
        auto* pNew = static_cast<GParamIdLookupNode*>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, sizeof(GParamIdLookupNode)));
        pThis->m_pIdListHead = pNew;
        if (pNew == nullptr) {
            return nullptr;
        }
        std::memset(pNew, 0, sizeof(GParamIdLookupNode));
        pNew->m_id = targetId;
        return pNew;
    }

    GParamIdLookupNode* pPrev = nullptr;
    for (;;) {
        if (pCur->m_id == targetId) {
            return nullptr;
        }
        GParamIdLookupNode* pAnchor = pCur;
        if (pCur->m_id > targetId) {
            auto* pNew = static_cast<GParamIdLookupNode*>(
                pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, sizeof(GParamIdLookupNode)));
            if (pNew != nullptr) {
                std::memset(pNew, 0, sizeof(GParamIdLookupNode));
                pNew->m_id = targetId;
                if (pPrev != nullptr) {
                    pPrev->m_pNext = pNew;
                    pNew->m_pPrev = pPrev;
                } else {
                    pThis->m_pIdListHead = pNew;
                }
                pNew->m_pNext = pAnchor;
                pAnchor->m_pPrev = pNew;
                return pNew;
            }
        }
        pCur = pAnchor->m_pNext;
        if (pCur != nullptr) {
            pPrev = pAnchor;
            continue;
        }
        auto* pNew = static_cast<GParamIdLookupNode*>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, sizeof(GParamIdLookupNode)));
        if (pNew != nullptr) {
            std::memset(pNew, 0, sizeof(GParamIdLookupNode));
            pNew->m_id = targetId;
            pAnchor->m_pNext = pNew;
            pNew->m_pPrev = pAnchor;
            return pNew;
        }
        pCur = pAnchor->m_pNext;
        if (pCur == nullptr) {
            return nullptr;
        }
        pPrev = pAnchor;
    }
}

// [VMA: 0x218ec40 - 0x218f1cd] (1421 B) — Update a single parameter slot and rebuild ID lookup nodes from reader
void GXGParamGroupLookupTable_UpdateParamSlotFromGroup(
    GXGParamGroupLookupTable* pThis,
    const GParamRuntimeGroupData* pGroupRuntimeData,
    GXGParamBinaryReader* pReader,
    uint32_t paramIndex)
{
    ptrdiff_t byteDiff = reinterpret_cast<const uint8_t*>(pGroupRuntimeData->m_pEnd) -
                         reinterpret_cast<const uint8_t*>(pGroupRuntimeData->m_pBegin);
    pThis->m_paramCount = static_cast<uint32_t>(byteDiff / 48);

    const GParamRuntimeParamEntry& entry = pGroupRuntimeData->m_pBegin[paramIndex];
    const char16_t* pNameW = entry.m_pNameW;
    const char16_t* pCommentW = entry.m_pCommentW;

    size_t vecCount = static_cast<size_t>(pThis->m_nameVec.m_pEnd - pThis->m_nameVec.m_pBegin);
    if (vecCount > paramIndex) {
        size_t nameLen = (pNameW[0] != u'\0') ? sub_2bbf228(pNameW) : 0;
        sub_262a720(&pThis->m_nameVec.m_pBegin[paramIndex], pNameW, nameLen);

        size_t commentLen = (pCommentW[0] != u'\0') ? sub_2bbf228(pCommentW) : 0;
        sub_262a720(&pThis->m_commentVec.m_pBegin[paramIndex], pCommentW, commentLen);
    } else {
        PushTempDLWStringToVector(&pThis->m_nameVec, pNameW);
        PushTempDLWStringToVector(&pThis->m_commentVec, pCommentW);
    }

    if (pReader == nullptr || !pReader->m_bValid || pThis->m_paramCount == 0) {
        return;
    }

    auto* pReaderVtbl = static_cast<GXGParamBinaryReaderVTable*>(pReader->vtable);
    uint32_t readerGroupCount = pReaderVtbl->GetGroupCount(pReader);
    uint32_t block2Count = pReaderVtbl->GetUnkBlock2Count(pReader);
    if (block2Count == 0) {
        return;
    }

    for (uint32_t b = 0; b < block2Count; ++b) {
        const uint32_t* pBlock2Entry = pReaderVtbl->GetUnkBlock2EntryByIndex(pReader, b);
        uint32_t entryId = pBlock2Entry[0];

        GParamIdLookupNode* pNode = FindOrInsertGParamIdNode(pThis, entryId);
        if (pNode == nullptr) {
            continue;
        }

        size_t indicesByteSize = static_cast<size_t>(pThis->m_paramCount) * sizeof(uint32_t);
        auto* pIndices = static_cast<uint32_t*>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, indicesByteSize));
        pNode->m_pValueIndices = pIndices;
        if (pIndices == nullptr) {
            continue;
        }
        std::memset(pIndices, 0, indicesByteSize);

        for (uint32_t g = 0; g < readerGroupCount; ++g) {
            const char16_t* pReaderNameW = pReaderVtbl->GetGroupNameW(pReader, g);
            const DLWString& commentStrObj = pThis->m_commentVec.m_pBegin[paramIndex];
            const char16_t* pCommentBuf = (commentStrObj.m_cap >= 8)
                ? commentStrObj.m_u.m_pPtr
                : commentStrObj.m_u.m_buf;

            if (CaseInsensitiveAsciiWStrEquals(pReaderNameW, pCommentBuf)) {
                pNode->m_pValueIndices[paramIndex] = pBlock2Entry[1 + g];
                break;
            }
        }
    }
}

// [VMA: 0x218ea00 - 0x218ec40] (576 B) — Free all allocated name/comment arrays, vectors, and ID lookup nodes
void GXGParamGroupLookupTable_ClearAndFree(GXGParamGroupLookupTable* pThis)
{
    GParamIdLookupNode* pNode = pThis->m_pIdListHead;
    while (pNode != nullptr) {
        GParamIdLookupNode* pNext = pNode->m_pNext;
        if (pNode->m_pValueIndices != nullptr) {
            pThis->m_pAllocator->vtable->Free(pThis->m_pAllocator, pNode->m_pValueIndices);
            pNode->m_pValueIndices = nullptr;
        }
        pThis->m_pAllocator->vtable->Free(pThis->m_pAllocator, pNode);
        pNode = pNext;
    }
    pThis->m_pIdListHead = nullptr;

    if (pThis->m_ppParamNames != nullptr) {
        for (uint32_t i = 0; i < pThis->m_paramCount; ++i) {
            if (pThis->m_ppParamNames[i] != nullptr) {
                pThis->m_pAllocator->vtable->Free(pThis->m_pAllocator, pThis->m_ppParamNames[i]);
                pThis->m_ppParamNames[i] = nullptr;
            }
        }
        if (pThis->m_ppParamNames != nullptr) {
            pThis->m_pAllocator->vtable->Free(pThis->m_pAllocator, pThis->m_ppParamNames);
            pThis->m_ppParamNames = nullptr;
        }
    }

    if (pThis->m_ppParamComments != nullptr) {
        for (uint32_t i = 0; i < pThis->m_paramCount; ++i) {
            if (pThis->m_ppParamComments[i] != nullptr) {
                pThis->m_pAllocator->vtable->Free(pThis->m_pAllocator, pThis->m_ppParamComments[i]);
                pThis->m_ppParamComments[i] = nullptr;
            }
        }
        if (pThis->m_ppParamComments != nullptr) {
            pThis->m_pAllocator->vtable->Free(pThis->m_pAllocator, pThis->m_ppParamComments);
            pThis->m_ppParamComments = nullptr;
        }
    }

    uint64_t dummyIter[3]{};
    GParamHashBucketIter nameFirst{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pThis->m_nameVec.m_pBegin)};
    GParamHashBucketIter nameLast{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pThis->m_nameVec.m_pEnd)};
    sub_272b310(dummyIter, &pThis->m_nameVec, nameFirst, nameLast);

    GParamHashBucketIter commentFirst{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pThis->m_commentVec.m_pBegin)};
    GParamHashBucketIter commentLast{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pThis->m_commentVec.m_pEnd)};
    sub_272b310(dummyIter, &pThis->m_commentVec, commentFirst, commentLast);

    pThis->m_pReader = nullptr;
    pThis->m_pGroupData = nullptr;
}

// [VMA: 0x218e2b0 - 0x218e9f1] (1857 B) — Initialize group lookup table from runtime group data and binary reader
bool GXGParamGroupLookupTable_Initialize(
    GXGParamGroupLookupTable* pThis,
    GParamRuntimeGroupData* pGroupRuntimeData,
    GXGParamBinaryReader* pReader)
{
    GXGParamGroupLookupTable_ClearAndFree(pThis);
    if (pGroupRuntimeData == nullptr) {
        GXGParamGroupLookupTable_ClearAndFree(pThis);
        return false;
    }

    ptrdiff_t byteDiff = reinterpret_cast<const uint8_t*>(pGroupRuntimeData->m_pEnd) -
                         reinterpret_cast<const uint8_t*>(pGroupRuntimeData->m_pBegin);
    uint32_t paramCount = static_cast<uint32_t>(byteDiff / 48);
    pThis->m_paramCount = paramCount;

    size_t ptrArrayBytes = static_cast<size_t>(paramCount) * sizeof(char16_t*);
    if (ptrArrayBytes != 0) {
        pThis->m_ppParamNames = static_cast<char16_t**>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, ptrArrayBytes));
        pThis->m_ppParamComments = static_cast<char16_t**>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, ptrArrayBytes));
    }

    if (pThis->m_ppParamNames == nullptr || pThis->m_ppParamComments == nullptr) {
        GXGParamGroupLookupTable_ClearAndFree(pThis);
        return false;
    }

    std::memset(pThis->m_ppParamNames, 0, ptrArrayBytes);
    std::memset(pThis->m_ppParamComments, 0, ptrArrayBytes);

    for (uint32_t i = 0; i < pThis->m_paramCount; ++i) {
        const GParamRuntimeParamEntry& entry = pGroupRuntimeData->m_pBegin[i];
        const char16_t* pNameW = entry.m_pNameW;
        const char16_t* pCommentW = entry.m_pCommentW;

        size_t nameByteLen = (sub_2bbf228(pNameW) & 0x7FFFFFFFULL) * sizeof(char16_t);
        auto* pNameCopy = static_cast<char16_t*>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, nameByteLen + sizeof(char16_t)));
        pThis->m_ppParamNames[i] = pNameCopy;

        size_t commentByteLen = (sub_2bbf228(pCommentW) & 0x7FFFFFFFULL) * sizeof(char16_t);
        auto* pCommentCopy = static_cast<char16_t*>(
            pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, commentByteLen + sizeof(char16_t)));
        pThis->m_ppParamComments[i] = pCommentCopy;

        if (pNameCopy == nullptr || pCommentCopy == nullptr) {
            GXGParamGroupLookupTable_ClearAndFree(pThis);
            return false;
        }

        std::memset(pNameCopy, 0, nameByteLen + sizeof(char16_t));
        sub_261a1b0(pNameCopy, pNameW, nameByteLen);

        std::memset(pCommentCopy, 0, commentByteLen + sizeof(char16_t));
        sub_261a1b0(pCommentCopy, pCommentW, commentByteLen);

        PushTempDLWStringToVector(&pThis->m_nameVec, pNameW);
        PushTempDLWStringToVector(&pThis->m_commentVec, pCommentW);
    }

    if (pReader != nullptr && pReader->m_bValid && pThis->m_paramCount != 0) {
        auto* pReaderVtbl = static_cast<GXGParamBinaryReaderVTable*>(pReader->vtable);
        uint32_t readerGroupCount = pReaderVtbl->GetGroupCount(pReader);
        uint32_t block2Count = pReaderVtbl->GetUnkBlock2Count(pReader);

        for (uint32_t b = 0; b < block2Count; ++b) {
            const uint32_t* pBlock2Entry = pReaderVtbl->GetUnkBlock2EntryByIndex(pReader, b);
            GParamIdLookupNode* pNode = FindOrInsertGParamIdNode(pThis, pBlock2Entry[0]);
            if (pNode == nullptr) {
                continue;
            }

            size_t indicesByteSize = static_cast<size_t>(pThis->m_paramCount) * sizeof(uint32_t);
            auto* pIndices = static_cast<uint32_t*>(
                pThis->m_pAllocator->vtable->Allocate(pThis->m_pAllocator, indicesByteSize));
            pNode->m_pValueIndices = pIndices;
            if (pIndices == nullptr) {
                continue;
            }
            std::memset(pIndices, 0, indicesByteSize);

            for (uint32_t g = 0; g < readerGroupCount; ++g) {
                const char16_t* pReaderNameW = pReaderVtbl->GetGroupNameW(pReader, g);
                for (uint32_t p = 0; p < pThis->m_paramCount; ++p) {
                    if (CaseInsensitiveAsciiWStrEquals(pReaderNameW, pThis->m_ppParamComments[p])) {
                        pNode->m_pValueIndices[p] = pBlock2Entry[1 + g];
                        break;
                    }
                }
            }
        }
    }

    pThis->m_pGroupData = pGroupRuntimeData;
    pThis->m_pReader = pReader;
    return true;
}

// [VMA: 0x218e1b0 - 0x218e2a9] (249 B) — Construct empty GXGParamGroupLookupTable with allocator
void GXGParamGroupLookupTable_Construct(
    GXGParamGroupLookupTable* pThis,
    DLAllocator* pAllocator)
{
    pThis->m_pAllocator = pAllocator;
    pThis->m_ppParamComments = nullptr;
    pThis->m_ppParamNames = nullptr;
    pThis->m_pReader = nullptr;
    pThis->m_pGroupData = nullptr;

    uint8_t blockInfo[8]{};
    pAllocator->vtable->GetBlockInfo(blockInfo, pAllocator, nullptr);
    if ((blockInfo[0] & 0x20) == 0) {
        sub_20b55b0(0u, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
    pThis->m_nameVec.m_pCap = nullptr;
    pThis->m_nameVec.m_pEnd = nullptr;
    pThis->m_nameVec.m_pBegin = nullptr;
    pThis->m_nameVec.m_pAlloc = pAllocator;

    pAllocator->vtable->GetBlockInfo(blockInfo, pAllocator, nullptr);
    if ((blockInfo[0] & 0x20) == 0) {
        sub_20b55b0(0u, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
    pThis->m_commentVec.m_pCap = nullptr;
    pThis->m_commentVec.m_pEnd = nullptr;
    pThis->m_commentVec.m_pBegin = nullptr;
    pThis->m_commentVec.m_pAlloc = pAllocator;

    pThis->m_paramCount = 0;
    pThis->m_pIdListHead = nullptr;
}

// ============================================================================
// 5. GParamManager Hash Tables & Entry Containers (0x218a010 .. 0x218e1a5)
// ============================================================================

// Park-Miller LCG hash helper used by all GParamManager std::_Hash tables:
//   seed = 0xdeadbeef ^ key
//   q = seed / 127773 (0x1f31d), r = seed % 127773
//   h = 16807 (0x41a7) * r - 2836 (0xb14) * q
//   if (h < 0) h += 0x7fffffffffffffff
static inline uint64_t GParamHashKey(uint64_t key)
{
    uint64_t x = 0xdeadbeefULL ^ (key & 0x7fffffffffffffffULL);
    uint64_t q = x / 0x1f31dULL;
    uint64_t r = x % 0x1f31dULL;
    int64_t  h = static_cast<int64_t>(r * 0x41a7ULL) - static_cast<int64_t>(q * 0xb14ULL);
    if (h < 0) {
        h += 0x7fffffffffffffffLL;
    }
    return static_cast<uint64_t>(h);
}

struct GParamHashInsertResult {
    void*               pListHead;  // +0x00
    void*               pListProxy; // +0x08
    GParamHashNodeBase* pNode;      // +0x10
    bool                bInserted;  // +0x18
};

struct GParamBucketVector {
    void*                 m_pProxy; // +0x00
    GParamHashBucketIter* m_pBegin; // +0x08
    GParamHashBucketIter* m_pEnd;   // +0x10
    GParamHashBucketIter* m_pCap;   // +0x18
    DLAllocator*          m_pAlloc; // +0x20
};

struct GParamHashTable {
    void*               m_pListProxy;   // +0x00
    GParamHashNodeBase* m_pListHead;    // +0x08
    uint64_t            m_size;         // +0x10
    DLAllocator*        m_pNodeAlloc;   // +0x18
    GParamBucketVector  m_buckets;      // +0x20 .. +0x48 (m_pBegin at +0x28, m_pEnd at +0x30, m_pCap at +0x38, m_pAlloc at +0x40)
    uint64_t            m_bucketMask;   // +0x48
    uint64_t            m_bucketCount;  // +0x50
    float               m_maxLoadFactor;// +0x58
    uint32_t            m_pad5C;        // +0x5C
};

static inline float U64ToFloat(uint64_t val)
{
    if (static_cast<int64_t>(val) >= 0) {
        return static_cast<float>(static_cast<int64_t>(val));
    }
    uint64_t half = (val >> 1) | (val & 1ULL);
    return static_cast<float>(static_cast<int64_t>(half)) * 2.0f;
}

static inline void SpliceNodeBeforeAndUpdateBucketBounds(
    GParamHashTable* pTable,
    GParamHashNodeBase* pWhere,
    uint64_t bucketIndex,
    const GParamHashBucketIter& nodeIter)
{
    GParamHashNodeBase* pNode = nodeIter.pNode;
    GParamHashNodeBase* pAfterNode = pNode->m_pNext;
    if (pAfterNode != pWhere) {
        pNode->m_pPrev->m_pNext = pAfterNode;
        pAfterNode->m_pPrev->m_pNext = pWhere;
        pWhere->m_pPrev->m_pNext = pNode;

        GParamHashNodeBase* pWherePrev = pWhere->m_pPrev;
        pWhere->m_pPrev = pAfterNode->m_pPrev;
        pAfterNode->m_pPrev = pNode->m_pPrev;
        pNode->m_pPrev = pWherePrev;
    }

    GParamHashBucketIter* pBuckets = pTable->m_buckets.m_pBegin;
    uint64_t loIdx = bucketIndex * 2;
    uint64_t hiIdx = loIdx | 1ULL;

    if (pBuckets[loIdx].pNode == pTable->m_pListHead) {
        pBuckets[loIdx].pListHead  = nodeIter.pListHead;
        pBuckets[loIdx].pListProxy = nodeIter.pListProxy;
        pBuckets[loIdx].pNode      = pNode;
        pBuckets[hiIdx].pListHead  = nodeIter.pListHead;
        pBuckets[hiIdx].pListProxy = nodeIter.pListProxy;
        pBuckets[hiIdx].pNode      = pNode;
    } else if (pBuckets[loIdx].pNode == pWhere) {
        pBuckets[loIdx].pListHead  = nodeIter.pListHead;
        pBuckets[loIdx].pListProxy = nodeIter.pListProxy;
        pBuckets[loIdx].pNode      = pNode;
    } else {
        GParamHashNodeBase* pHiNext = pBuckets[hiIdx].pNode->m_pNext;
        pBuckets[hiIdx].pNode = pHiNext;
        if (pHiNext != pNode) {
            pBuckets[hiIdx].pNode = pHiNext->m_pPrev;
        }
    }
}

static inline uint64_t ComputeNextBucketCount(uint64_t curBuckets)
{
    if (curBuckets <= 0x1FFULL) {
        return curBuckets << 3;
    }
    if (curBuckets < 0x0555555555555555ULL) {
        return curBuckets << 1;
    }
    return curBuckets;
}

GParamHashInsertResult* GParamHashTableU32_InsertOrFreeDuplicate(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint32_t* pKey,
    GParamHashBucketIter nodeIter);

GParamHashInsertResult* GParamHashTableU64B_InsertOrFreeDuplicate(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint64_t* pKey,
    GParamHashBucketIter nodeIter);

GParamHashInsertResult* GParamHashTableU64A_InsertOrFreeDuplicate(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint64_t* pKey,
    GParamHashBucketIter nodeIter);

GParamHashBucketIter* GParamBucketVectorA_InsertFill(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pBucketVector,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter);

GParamHashBucketIter* GParamBucketVectorB_InsertFill(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pBucketVector,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter);

GParamHashBucketIter* GParamBucketVectorC_InsertFill(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pBucketVector,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter);

// [VMA: 0x218ded0 - 0x218e1ab] (731 B) — Splice node into uint32-keyed hash bucket and check load factor / rehash
GParamHashInsertResult* GParamHashTableU32_InsertNodeIntoBucketAndCheckRehash(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    GParamHashNodeBase* pWhereNode,
    uint64_t bucketIndex,
    GParamHashBucketIter nodeIter)
{
    SpliceNodeBeforeAndUpdateBucketBounds(pHashTable, pWhereNode, bucketIndex, nodeIter);

    uint64_t curBuckets = pHashTable->m_bucketCount;
    float loadFactor = U64ToFloat(pHashTable->m_size) / U64ToFloat(curBuckets);
    if (loadFactor > pHashTable->m_maxLoadFactor) {
        uint64_t newBuckets = ComputeNextBucketCount(curBuckets);
        pHashTable->m_buckets.m_pEnd = pHashTable->m_buckets.m_pBegin;

        GParamHashBucketIter fillVal{nullptr, nullptr, pHashTable->m_pListHead};
        GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pHashTable->m_buckets.m_pBegin)};
        GParamHashBucketIter dummyOut{};
        GParamBucketVectorA_InsertFill(&dummyOut, &pHashTable->m_buckets, newBuckets * 2, &fillVal, whereIter);

        pHashTable->m_bucketMask = newBuckets - 1;
        pHashTable->m_bucketCount = newBuckets;

        GParamHashNodeBase* pCur = pHashTable->m_pListHead->m_pNext;
        if (pCur != pHashTable->m_pListHead) {
            GParamHashNodeBase* pLast = pHashTable->m_pListHead->m_pPrev;
            do {
                pCur = pHashTable->m_pListHead->m_pNext;
                const auto* pKey = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
                GParamHashBucketIter reinsertIter{dummyOut.pListHead, dummyOut.pListProxy, pCur};
                GParamHashInsertResult tmpRes{};
                GParamHashTableU32_InsertOrFreeDuplicate(&tmpRes, pHashTable, pKey, reinsertIter);
            } while (pCur != pLast);
        }
    }

    pOutResult->pListHead  = nodeIter.pListHead;
    pOutResult->pListProxy = nodeIter.pListProxy;
    pOutResult->pNode      = nodeIter.pNode;
    pOutResult->bInserted  = true;
    return pOutResult;
}

// [VMA: 0x218dab0 - 0x218ded0] (1056 B) — Insert or erase duplicate node in uint64-keyed GParam resource hash table
GParamHashInsertResult* GParamHashTableU64Res_InsertOrDeduplicateNode(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint64_t* pKey,
    GParamHashBucketIter nodeIter)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pBucketStart = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pCur = pListHead;
    if (pBucketStart != pListHead) {
        pCur = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
    }

    GParamHashNodeBase* pWhere = pBucketStart;
    while (pCur != pBucketStart) {
        pCur = pCur->m_pPrev;
        uint64_t curKey = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (*pKey < curKey) {
            continue;
        }
        if (curKey >= *pKey) {
            // Duplicate key found: unlink nodeIter.pNode, destroy payload (+0x18) via sub_2188a80, and free node
            GParamHashNodeBase* pDupNode = nodeIter.pNode;
            if (pDupNode != pListHead) {
                pDupNode->m_pPrev->m_pNext = pDupNode->m_pNext;
                pDupNode->m_pNext->m_pPrev = pDupNode->m_pPrev;
                sub_2188a80(reinterpret_cast<uint8_t*>(pDupNode) + 0x18);
                pHashTable->m_pNodeAlloc->vtable->Free(pHashTable->m_pNodeAlloc, pDupNode);
                --pHashTable->m_size;
            }
            pOutResult->pListHead  = nullptr;
            pOutResult->pListProxy = nullptr;
            pOutResult->pNode      = pCur;
            pOutResult->bInserted  = false;
            return pOutResult;
        }
        pWhere = pCur->m_pNext;
        break;
    }

    SpliceNodeBeforeAndUpdateBucketBounds(pHashTable, pWhere, bucketIdx, nodeIter);

    uint64_t curBuckets = pHashTable->m_bucketCount;
    float loadFactor = U64ToFloat(pHashTable->m_size) / U64ToFloat(curBuckets);
    if (loadFactor > pHashTable->m_maxLoadFactor) {
        uint64_t newBuckets = ComputeNextBucketCount(curBuckets);
        pHashTable->m_buckets.m_pEnd = pHashTable->m_buckets.m_pBegin;

        GParamHashBucketIter fillVal{nullptr, nullptr, pHashTable->m_pListHead};
        GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pHashTable->m_buckets.m_pBegin)};
        GParamHashBucketIter dummyOut{};
        sub_2189490(&dummyOut, &pHashTable->m_buckets, newBuckets * 2, &fillVal, whereIter);

        pHashTable->m_bucketMask = newBuckets - 1;
        pHashTable->m_bucketCount = newBuckets;

        GParamHashNodeBase* pRehashCur = pHashTable->m_pListHead->m_pNext;
        if (pRehashCur != pHashTable->m_pListHead) {
            GParamHashNodeBase* pLast = pHashTable->m_pListHead->m_pPrev;
            do {
                pRehashCur = pHashTable->m_pListHead->m_pNext;
                const auto* pRehashKey = reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pRehashCur) + 0x10);
                GParamHashBucketIter reinsertIter{dummyOut.pListHead, dummyOut.pListProxy, pRehashCur};
                GParamHashInsertResult tmpRes{};
                GParamHashTableU64Res_InsertOrDeduplicateNode(&tmpRes, pHashTable, pRehashKey, reinsertIter);
            } while (pRehashCur != pLast);
        }
    }

    pOutResult->pListHead  = nodeIter.pListHead;
    pOutResult->pListProxy = nodeIter.pListProxy;
    pOutResult->pNode      = nodeIter.pNode;
    pOutResult->bInserted  = true;
    return pOutResult;
}

// [VMA: 0x218d7d0 - 0x218daab] (731 B) — Splice node into uint64-keyed hash bucket (variant B) and check rehash
GParamHashInsertResult* GParamHashTableU64B_InsertNodeIntoBucketAndCheckRehash(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    GParamHashNodeBase* pWhereNode,
    uint64_t bucketIndex,
    GParamHashBucketIter nodeIter)
{
    SpliceNodeBeforeAndUpdateBucketBounds(pHashTable, pWhereNode, bucketIndex, nodeIter);

    uint64_t curBuckets = pHashTable->m_bucketCount;
    float loadFactor = U64ToFloat(pHashTable->m_size) / U64ToFloat(curBuckets);
    if (loadFactor > pHashTable->m_maxLoadFactor) {
        uint64_t newBuckets = ComputeNextBucketCount(curBuckets);
        pHashTable->m_buckets.m_pEnd = pHashTable->m_buckets.m_pBegin;

        GParamHashBucketIter fillVal{nullptr, nullptr, pHashTable->m_pListHead};
        GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pHashTable->m_buckets.m_pBegin)};
        GParamHashBucketIter dummyOut{};
        GParamBucketVectorB_InsertFill(&dummyOut, &pHashTable->m_buckets, newBuckets * 2, &fillVal, whereIter);

        pHashTable->m_bucketMask = newBuckets - 1;
        pHashTable->m_bucketCount = newBuckets;

        GParamHashNodeBase* pCur = pHashTable->m_pListHead->m_pNext;
        if (pCur != pHashTable->m_pListHead) {
            GParamHashNodeBase* pLast = pHashTable->m_pListHead->m_pPrev;
            do {
                pCur = pHashTable->m_pListHead->m_pNext;
                const auto* pKey = reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
                GParamHashBucketIter reinsertIter{dummyOut.pListHead, dummyOut.pListProxy, pCur};
                GParamHashInsertResult tmpRes{};
                GParamHashTableU64B_InsertOrFreeDuplicate(&tmpRes, pHashTable, pKey, reinsertIter);
            } while (pCur != pLast);
        }
    }

    pOutResult->pListHead  = nodeIter.pListHead;
    pOutResult->pListProxy = nodeIter.pListProxy;
    pOutResult->pNode      = nodeIter.pNode;
    pOutResult->bInserted  = true;
    return pOutResult;
}

// [VMA: 0x218d4f0 - 0x218d7cb] (731 B) — Splice node into uint64-keyed hash bucket (variant A) and check rehash
GParamHashInsertResult* GParamHashTableU64A_InsertNodeIntoBucketAndCheckRehash(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    GParamHashNodeBase* pWhereNode,
    uint64_t bucketIndex,
    GParamHashBucketIter nodeIter)
{
    SpliceNodeBeforeAndUpdateBucketBounds(pHashTable, pWhereNode, bucketIndex, nodeIter);

    uint64_t curBuckets = pHashTable->m_bucketCount;
    float loadFactor = U64ToFloat(pHashTable->m_size) / U64ToFloat(curBuckets);
    if (loadFactor > pHashTable->m_maxLoadFactor) {
        uint64_t newBuckets = ComputeNextBucketCount(curBuckets);
        pHashTable->m_buckets.m_pEnd = pHashTable->m_buckets.m_pBegin;

        GParamHashBucketIter fillVal{nullptr, nullptr, pHashTable->m_pListHead};
        GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pHashTable->m_buckets.m_pBegin)};
        GParamHashBucketIter dummyOut{};
        sub_2188dc0(&dummyOut, &pHashTable->m_buckets, newBuckets * 2, &fillVal, whereIter);

        pHashTable->m_bucketMask = newBuckets - 1;
        pHashTable->m_bucketCount = newBuckets;

        GParamHashNodeBase* pCur = pHashTable->m_pListHead->m_pNext;
        if (pCur != pHashTable->m_pListHead) {
            GParamHashNodeBase* pLast = pHashTable->m_pListHead->m_pPrev;
            do {
                pCur = pHashTable->m_pListHead->m_pNext;
                const auto* pKey = reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
                GParamHashBucketIter reinsertIter{dummyOut.pListHead, dummyOut.pListProxy, pCur};
                GParamHashInsertResult tmpRes{};
                GParamHashTableU64A_InsertOrFreeDuplicate(&tmpRes, pHashTable, pKey, reinsertIter);
            } while (pCur != pLast);
        }
    }

    pOutResult->pListHead  = nodeIter.pListHead;
    pOutResult->pListProxy = nodeIter.pListProxy;
    pOutResult->pNode      = nodeIter.pNode;
    pOutResult->bInserted  = true;
    return pOutResult;
}

// [VMA: 0x218d360 - 0x218d4e2] (386 B) — Find bucket for uint64 key (variant A) and insert node or free duplicate
GParamHashInsertResult* GParamHashTableU64A_InsertOrFreeDuplicate(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint64_t* pKey,
    GParamHashBucketIter nodeIter)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pWhere = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pCur = pListHead;
    if (pWhere != pListHead) {
        pCur = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
    }

    while (pCur != pWhere) {
        pCur = pCur->m_pPrev;
        uint64_t curKey = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (*pKey < curKey) {
            continue;
        }
        if (curKey >= *pKey) {
            GParamHashNodeBase* pDupNode = nodeIter.pNode;
            if (pDupNode != pListHead) {
                pDupNode->m_pPrev->m_pNext = pDupNode->m_pNext;
                pDupNode->m_pNext->m_pPrev = pDupNode->m_pPrev;
                pHashTable->m_pNodeAlloc->vtable->Free(pHashTable->m_pNodeAlloc, pDupNode);
                --pHashTable->m_size;
            }
            pOutResult->pListHead  = nullptr;
            pOutResult->pListProxy = nullptr;
            pOutResult->pNode      = pCur;
            pOutResult->bInserted  = false;
            return pOutResult;
        }
        pWhere = pCur->m_pNext;
        break;
    }

    GParamHashInsertResult tmpRes{};
    GParamHashTableU64A_InsertNodeIntoBucketAndCheckRehash(&tmpRes, pHashTable, pWhere, bucketIdx, nodeIter);
    *pOutResult = tmpRes;
    return pOutResult;
}

// [VMA: 0x218cf50 - 0x218d35f] (1039 B) — Insert or deduplicate node in uint64-keyed hash table (variant C)
GParamHashInsertResult* GParamHashTableU64C_InsertOrDeduplicateNode(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint64_t* pKey,
    GParamHashBucketIter nodeIter)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pBucketStart = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pCur = pListHead;
    if (pBucketStart != pListHead) {
        pCur = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
    }

    GParamHashNodeBase* pWhere = pBucketStart;
    while (pCur != pBucketStart) {
        pCur = pCur->m_pPrev;
        uint64_t curKey = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (*pKey < curKey) {
            continue;
        }
        if (curKey >= *pKey) {
            GParamHashNodeBase* pDupNode = nodeIter.pNode;
            if (pDupNode != pListHead) {
                pDupNode->m_pPrev->m_pNext = pDupNode->m_pNext;
                pDupNode->m_pNext->m_pPrev = pDupNode->m_pPrev;
                pHashTable->m_pNodeAlloc->vtable->Free(pHashTable->m_pNodeAlloc, pDupNode);
                --pHashTable->m_size;
            }
            pOutResult->pListHead  = nullptr;
            pOutResult->pListProxy = nullptr;
            pOutResult->pNode      = pCur;
            pOutResult->bInserted  = false;
            return pOutResult;
        }
        pWhere = pCur->m_pNext;
        break;
    }

    SpliceNodeBeforeAndUpdateBucketBounds(pHashTable, pWhere, bucketIdx, nodeIter);

    uint64_t curBuckets = pHashTable->m_bucketCount;
    float loadFactor = U64ToFloat(pHashTable->m_size) / U64ToFloat(curBuckets);
    if (loadFactor > pHashTable->m_maxLoadFactor) {
        uint64_t newBuckets = ComputeNextBucketCount(curBuckets);
        pHashTable->m_buckets.m_pEnd = pHashTable->m_buckets.m_pBegin;

        GParamHashBucketIter fillVal{nullptr, nullptr, pHashTable->m_pListHead};
        GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pHashTable->m_buckets.m_pBegin)};
        GParamHashBucketIter dummyOut{};
        sub_21899e0(&dummyOut, &pHashTable->m_buckets, newBuckets * 2, &fillVal, whereIter);

        pHashTable->m_bucketMask = newBuckets - 1;
        pHashTable->m_bucketCount = newBuckets;

        GParamHashNodeBase* pRehashCur = pHashTable->m_pListHead->m_pNext;
        if (pRehashCur != pHashTable->m_pListHead) {
            GParamHashNodeBase* pLast = pHashTable->m_pListHead->m_pPrev;
            do {
                pRehashCur = pHashTable->m_pListHead->m_pNext;
                const auto* pRehashKey = reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pRehashCur) + 0x10);
                GParamHashBucketIter reinsertIter{dummyOut.pListHead, dummyOut.pListProxy, pRehashCur};
                GParamHashInsertResult tmpRes{};
                GParamHashTableU64C_InsertOrDeduplicateNode(&tmpRes, pHashTable, pRehashKey, reinsertIter);
            } while (pRehashCur != pLast);
        }
    }

    pOutResult->pListHead  = nodeIter.pListHead;
    pOutResult->pListProxy = nodeIter.pListProxy;
    pOutResult->pNode      = nodeIter.pNode;
    pOutResult->bInserted  = true;
    return pOutResult;
}

// [VMA: 0x218ca90 - 0x218cf41] (1201 B) — Insert or deduplicate complex entry node in uint32-keyed hash table
GParamHashInsertResult* GParamHashTableU32Complex_InsertOrDeduplicateNode(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint32_t* pKey,
    GParamHashBucketIter nodeIter)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pBucketStart = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pCur = pListHead;
    if (pBucketStart != pListHead) {
        pCur = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
    }

    GParamHashNodeBase* pWhere = pBucketStart;
    while (pCur != pBucketStart) {
        pCur = pCur->m_pPrev;
        uint32_t curKey = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (*pKey < curKey) {
            continue;
        }
        if (curKey >= *pKey) {
            // Duplicate found: unlink nodeIter.pNode, free embedded vector (+0x40..+0x58) and embedded list (+0x20..+0x30)
            GParamHashNodeBase* pDupNode = nodeIter.pNode;
            if (pDupNode != pListHead) {
                pDupNode->m_pPrev->m_pNext = pDupNode->m_pNext;
                pDupNode->m_pNext->m_pPrev = pDupNode->m_pPrev;

                auto* pNodeBytes = reinterpret_cast<uint8_t*>(pDupNode);
                void* pVecBuf = *reinterpret_cast<void**>(pNodeBytes + 0x40);
                if (pVecBuf != nullptr) {
                    DLAllocator* pVecAlloc = *reinterpret_cast<DLAllocator**>(pNodeBytes + 0x58);
                    pVecAlloc->vtable->Free(pVecAlloc, pVecBuf);
                    *reinterpret_cast<void**>(pNodeBytes + 0x50) = nullptr;
                    *reinterpret_cast<void**>(pNodeBytes + 0x48) = nullptr;
                    *reinterpret_cast<void**>(pNodeBytes + 0x40) = nullptr;
                }

                auto* pSubListHead = *reinterpret_cast<GParamHashNodeBase**>(pNodeBytes + 0x20);
                GParamHashNodeBase* pSubCur = pSubListHead->m_pNext;
                pSubListHead->m_pNext = pSubListHead;
                pSubListHead->m_pPrev = pSubListHead;
                *reinterpret_cast<uint64_t*>(pNodeBytes + 0x28) = 0;

                DLAllocator* pSubAlloc = *reinterpret_cast<DLAllocator**>(pNodeBytes + 0x30);
                while (pSubCur != pSubListHead) {
                    GParamHashNodeBase* pSubNext = pSubCur->m_pNext;
                    pSubAlloc->vtable->Free(pSubAlloc, pSubCur);
                    pSubCur = pSubNext;
                }
                pSubAlloc->vtable->Free(pSubAlloc, pSubListHead);

                pHashTable->m_pNodeAlloc->vtable->Free(pHashTable->m_pNodeAlloc, pDupNode);
                --pHashTable->m_size;
            }

            pOutResult->pListHead  = nullptr;
            pOutResult->pListProxy = nullptr;
            pOutResult->pNode      = pCur;
            pOutResult->bInserted  = false;
            return pOutResult;
        }
        pWhere = pCur->m_pNext;
        break;
    }

    SpliceNodeBeforeAndUpdateBucketBounds(pHashTable, pWhere, bucketIdx, nodeIter);

    uint64_t curBuckets = pHashTable->m_bucketCount;
    float loadFactor = U64ToFloat(pHashTable->m_size) / U64ToFloat(curBuckets);
    if (loadFactor > pHashTable->m_maxLoadFactor) {
        uint64_t newBuckets = ComputeNextBucketCount(curBuckets);
        pHashTable->m_buckets.m_pEnd = pHashTable->m_buckets.m_pBegin;

        GParamHashBucketIter fillVal{nullptr, nullptr, pHashTable->m_pListHead};
        GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pHashTable->m_buckets.m_pBegin)};
        GParamHashBucketIter dummyOut{};
        GParamBucketVectorC_InsertFill(&dummyOut, &pHashTable->m_buckets, newBuckets * 2, &fillVal, whereIter);

        pHashTable->m_bucketMask = newBuckets - 1;
        pHashTable->m_bucketCount = newBuckets;

        GParamHashNodeBase* pRehashCur = pHashTable->m_pListHead->m_pNext;
        if (pRehashCur != pHashTable->m_pListHead) {
            GParamHashNodeBase* pLast = pHashTable->m_pListHead->m_pPrev;
            do {
                pRehashCur = pHashTable->m_pListHead->m_pNext;
                const auto* pRehashKey = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pRehashCur) + 0x10);
                GParamHashBucketIter reinsertIter{dummyOut.pListHead, dummyOut.pListProxy, pRehashCur};
                GParamHashInsertResult tmpRes{};
                GParamHashTableU32Complex_InsertOrDeduplicateNode(&tmpRes, pHashTable, pRehashKey, reinsertIter);
            } while (pRehashCur != pLast);
        }
    }

    pOutResult->pListHead  = nodeIter.pListHead;
    pOutResult->pListProxy = nodeIter.pListProxy;
    pOutResult->pNode      = nodeIter.pNode;
    pOutResult->bInserted  = true;
    return pOutResult;
}

// [VMA: 0x218c910 - 0x218ca86] (374 B) — Find bucket for uint32 key and insert node or free duplicate
GParamHashInsertResult* GParamHashTableU32_InsertOrFreeDuplicate(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint32_t* pKey,
    GParamHashBucketIter nodeIter)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pWhere = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pCur = pListHead;
    if (pWhere != pListHead) {
        pCur = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
    }

    while (pCur != pWhere) {
        pCur = pCur->m_pPrev;
        uint32_t curKey = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (*pKey < curKey) {
            continue;
        }
        if (curKey >= *pKey) {
            GParamHashNodeBase* pDupNode = nodeIter.pNode;
            if (pDupNode != pListHead) {
                pDupNode->m_pPrev->m_pNext = pDupNode->m_pNext;
                pDupNode->m_pNext->m_pPrev = pDupNode->m_pPrev;
                pHashTable->m_pNodeAlloc->vtable->Free(pHashTable->m_pNodeAlloc, pDupNode);
                --pHashTable->m_size;
            }
            pOutResult->pListHead  = nullptr;
            pOutResult->pListProxy = nullptr;
            pOutResult->pNode      = pCur;
            pOutResult->bInserted  = false;
            return pOutResult;
        }
        pWhere = pCur->m_pNext;
        break;
    }

    GParamHashInsertResult tmpRes{};
    GParamHashTableU32_InsertNodeIntoBucketAndCheckRehash(&tmpRes, pHashTable, pWhere, bucketIdx, nodeIter);
    *pOutResult = tmpRes;
    return pOutResult;
}

// Shared implementation for the 3 identical 1,352-byte 24-byte-element bucket vector fill-insert functions
static GParamHashBucketIter* GParamBucketVector_InsertFillImpl(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pVec,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter)
{
    auto* pWhere = reinterpret_cast<GParamHashBucketIter*>(whereIter.pNode);
    GParamHashBucketIter* pOldBegin = pVec->m_pBegin;

    if (count != 0) {
        GParamHashBucketIter* pOldEnd = pVec->m_pEnd;
        GParamHashBucketIter* pOldCap = pVec->m_pCap;
        size_t availCap = static_cast<size_t>(pOldCap - pOldEnd);

        if (availCap < count) {
            size_t curSize = static_cast<size_t>(pOldEnd - pOldBegin);
            const size_t kMaxSize = 0x0AAAAAAAAAAAAAAAULL;
            if (kMaxSize - curSize < count) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            size_t reqSize = curSize + count;
            size_t curCap = static_cast<size_t>(pOldCap - pOldBegin);
            size_t grownCap = (kMaxSize - (curCap >> 1) >= curCap) ? (curCap + (curCap >> 1)) : 0;
            size_t newCap = (grownCap < reqSize) ? reqSize : grownCap;

            auto* pNewBuf = static_cast<GParamHashBucketIter*>(
                pVec->m_pAlloc->vtable->AllocateAligned(pVec->m_pAlloc, newCap * sizeof(GParamHashBucketIter), 8));
            if (pNewBuf == nullptr) {
                GX_ReportOomOrThrow0x109();
            }

            size_t prefixCount = static_cast<size_t>(pWhere - pOldBegin);
            GParamHashBucketIter* pFillDst = pNewBuf + prefixCount;
            for (size_t i = 0; i < count; ++i) {
                pFillDst[i] = *pFillVal;
            }
            for (size_t i = 0; i < prefixCount; ++i) {
                pNewBuf[i] = pOldBegin[i];
            }
            size_t suffixCount = static_cast<size_t>(pOldEnd - pWhere);
            GParamHashBucketIter* pSuffixDst = pNewBuf + prefixCount + count;
            for (size_t i = 0; i < suffixCount; ++i) {
                pSuffixDst[i] = pWhere[i];
            }

            if (pOldBegin != nullptr) {
                pVec->m_pAlloc->vtable->Free(pVec->m_pAlloc, pOldBegin);
            }
            pVec->m_pCap   = pNewBuf + newCap;
            pVec->m_pEnd   = pNewBuf + curSize + count;
            pVec->m_pBegin = pNewBuf;
        } else {
            size_t tailElems = static_cast<size_t>(pOldEnd - pWhere);
            GParamHashBucketIter fillCopy = *pFillVal;
            if (tailElems < count) {
                for (GParamHashBucketIter* p = pWhere; p != pOldEnd; ++p) {
                    *(p + count) = *p;
                }
                size_t uninitCount = count - tailElems;
                for (size_t i = 0; i < uninitCount; ++i) {
                    pOldEnd[i] = fillCopy;
                }
                pVec->m_pEnd = pOldEnd + count;
                for (GParamHashBucketIter* p = pWhere; p != pOldEnd; ++p) {
                    *p = fillCopy;
                }
            } else {
                GParamHashBucketIter* pSrcTail = pOldEnd - count;
                for (size_t i = 0; i < count; ++i) {
                    pOldEnd[i] = pSrcTail[i];
                }
                pVec->m_pEnd = pOldEnd + count;
                for (GParamHashBucketIter* p = pOldEnd - 1; p >= pWhere + count; --p) {
                    *p = *(p - count);
                }
                for (size_t i = 0; i < count; ++i) {
                    pWhere[i] = fillCopy;
                }
            }
        }
    }

    size_t offsetIdx = static_cast<size_t>(pWhere - pOldBegin);
    pOutIter->pListHead  = nullptr;
    pOutIter->pListProxy = nullptr;
    pOutIter->pNode      = reinterpret_cast<GParamHashNodeBase*>(pVec->m_pBegin + offsetIdx);
    return pOutIter;
}

// [VMA: 0x218c3c0 - 0x218c908] (1352 B) — Fill-insert N bucket iterators (24 bytes each) into hash bucket vector (variant A)
GParamHashBucketIter* GParamBucketVectorA_InsertFill(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pBucketVector,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter)
{
    return GParamBucketVector_InsertFillImpl(pOutIter, pBucketVector, count, pFillVal, whereIter);
}

// [VMA: 0x218c2c0 - 0x218c3bb] (251 B) — Reallocate pointer vector to new capacity
void GParamPointerVector_Reallocate(void* pVector, size_t newCapacity)
{
    struct PtrVec {
        void*        m_pProxy; // +0x00
        void**       m_pBegin; // +0x08
        void**       m_pEnd;   // +0x10
        void**       m_pCap;   // +0x18
        DLAllocator* m_pAlloc; // +0x20
    };
    auto* pVec = static_cast<PtrVec*>(pVector);
    auto** pNewBuf = static_cast<void**>(
        pVec->m_pAlloc->vtable->AllocateAligned(pVec->m_pAlloc, newCapacity * sizeof(void*), 8));
    if (pNewBuf == nullptr) {
        GX_ReportOomOrThrow0x109();
    }

    void** pOldBegin = pVec->m_pBegin;
    void** pOldEnd = pVec->m_pEnd;
    size_t curSize = static_cast<size_t>(pOldEnd - pOldBegin);
    for (size_t i = 0; i < curSize; ++i) {
        pNewBuf[i] = pOldBegin[i];
    }

    if (pOldBegin != nullptr) {
        pVec->m_pAlloc->vtable->Free(pVec->m_pAlloc, pOldBegin);
    }

    pVec->m_pCap   = pNewBuf + newCapacity;
    pVec->m_pEnd   = pNewBuf + curSize;
    pVec->m_pBegin = pNewBuf;
}

// [VMA: 0x218be20 - 0x218c2b6] (1174 B) — Copy-construct 0x420-byte GParamLoadedFileEntry (2 DLWStrings + 5 DLFixedWString<64>)
void GParamLoadedFileEntry_CopyConstruct(void* pDst, const void* pSrc)
{
    auto* pD = static_cast<uint8_t*>(pDst);
    const auto* pS = static_cast<const uint8_t*>(pSrc);

    *reinterpret_cast<uint32_t*>(pD + 0x08) = *reinterpret_cast<const uint32_t*>(pS + 0x08);
    *reinterpret_cast<uint64_t*>(pD + 0x00) = *reinterpret_cast<const uint64_t*>(pS + 0x00);

    // Copy DLWString #1 (+0x10..+0x40)
    *reinterpret_cast<uint64_t*>(pD + 0x38) = *reinterpret_cast<const uint64_t*>(pS + 0x38);
    *reinterpret_cast<uint64_t*>(pD + 0x30) = 7;
    *reinterpret_cast<uint64_t*>(pD + 0x28) = 0;
    *reinterpret_cast<uint16_t*>(pD + 0x18) = 0;
    sub_26279a0(pD + 0x10, pS + 0x10, 0, ~0ULL);
    pD[0x40] = pS[0x40];

    // Copy DLWString #2 (+0x48..+0x78)
    *reinterpret_cast<uint64_t*>(pD + 0x70) = *reinterpret_cast<const uint64_t*>(pS + 0x70);
    *reinterpret_cast<uint64_t*>(pD + 0x68) = 7;
    *reinterpret_cast<uint64_t*>(pD + 0x60) = 0;
    *reinterpret_cast<uint16_t*>(pD + 0x50) = 0;
    sub_26279a0(pD + 0x48, pS + 0x48, 0, ~0ULL);
    pD[0x78] = pS[0x78];

    // Copy 5 DLFixedWString<64> blocks (stride 0xA8 each, at offsets +0x80, +0x130, +0x1E0, +0x290, +0x348)
    static const size_t kFixedStrOffsets[5] = {0x80, 0x130, 0x1E0, 0x290, 0x348};
    uint8_t dummyLocal[0x81]{};
    for (int i = 0; i < 5; ++i) {
        if (i == 4) {
            pD[0x340] = pS[0x340];
        }
        size_t base = kFixedStrOffsets[i];
        std::memcpy(pD + base + 0x28, dummyLocal, 0x81);
        pD[base + 0xA9] = 0;
        *reinterpret_cast<uint64_t*>(pD + base + 0x20) = 7;
        *reinterpret_cast<uint64_t*>(pD + base + 0x18) = 0;
        *reinterpret_cast<uint16_t*>(pD + base + 0x08) = 0;
        pD[base + 0xAA] = pS[base + 0xAA];

        sub_2935350(pD + base, 0x3F, 0);
        uint64_t cap = *reinterpret_cast<uint64_t*>(pD + base + 0x20);
        *reinterpret_cast<uint64_t*>(pD + base + 0x18) = 0;
        uint16_t* pBuf = (cap >= 8)
            ? *reinterpret_cast<uint16_t**>(pD + base + 0x08)
            : reinterpret_cast<uint16_t*>(pD + base + 0x08);
        pBuf[0] = 0;

        size_t srcLen = *reinterpret_cast<const size_t*>(pS + base + 0x18);
        if (srcLen > 0x3F) {
            srcLen = 0x3F;
        }
        sub_2935150(pD + base, pS + base, 0, srcLen);
    }

    *reinterpret_cast<uint64_t*>(pD + 0x418) = *reinterpret_cast<const uint64_t*>(pS + 0x418);
    *reinterpret_cast<uint64_t*>(pD + 0x410) = *reinterpret_cast<const uint64_t*>(pS + 0x410);
    *reinterpret_cast<uint64_t*>(pD + 0x408) = *reinterpret_cast<const uint64_t*>(pS + 0x408);
    *reinterpret_cast<uint64_t*>(pD + 0x400) = *reinterpret_cast<const uint64_t*>(pS + 0x400);
    *reinterpret_cast<uint64_t*>(pD + 0x3F8) = *reinterpret_cast<const uint64_t*>(pS + 0x3F8);
}

// [VMA: 0x218bc90 - 0x218be12] (386 B) — Find bucket for uint64 key (variant B) and insert node or free duplicate
GParamHashInsertResult* GParamHashTableU64B_InsertOrFreeDuplicate(
    GParamHashInsertResult* pOutResult,
    GParamHashTable* pHashTable,
    const uint64_t* pKey,
    GParamHashBucketIter nodeIter)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pWhere = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pCur = pListHead;
    if (pWhere != pListHead) {
        pCur = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
    }

    while (pCur != pWhere) {
        pCur = pCur->m_pPrev;
        uint64_t curKey = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (*pKey < curKey) {
            continue;
        }
        if (curKey >= *pKey) {
            GParamHashNodeBase* pDupNode = nodeIter.pNode;
            if (pDupNode != pListHead) {
                pDupNode->m_pPrev->m_pNext = pDupNode->m_pNext;
                pDupNode->m_pNext->m_pPrev = pDupNode->m_pPrev;
                pHashTable->m_pNodeAlloc->vtable->Free(pHashTable->m_pNodeAlloc, pDupNode);
                --pHashTable->m_size;
            }
            pOutResult->pListHead  = nullptr;
            pOutResult->pListProxy = nullptr;
            pOutResult->pNode      = pCur;
            pOutResult->bInserted  = false;
            return pOutResult;
        }
        pWhere = pCur->m_pNext;
        break;
    }

    GParamHashInsertResult tmpRes{};
    GParamHashTableU64B_InsertNodeIntoBucketAndCheckRehash(&tmpRes, pHashTable, pWhere, bucketIdx, nodeIter);
    *pOutResult = tmpRes;
    return pOutResult;
}

// [VMA: 0x218b740 - 0x218bc88] (1352 B) — Fill-insert N bucket iterators (24 bytes each) into hash bucket vector (variant B)
GParamHashBucketIter* GParamBucketVectorB_InsertFill(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pBucketVector,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter)
{
    return GParamBucketVector_InsertFillImpl(pOutIter, pBucketVector, count, pFillVal, whereIter);
}

// [VMA: 0x218b1f0 - 0x218b738] (1352 B) — Fill-insert N bucket iterators (24 bytes each) into hash bucket vector (variant C)
GParamHashBucketIter* GParamBucketVectorC_InsertFill(
    GParamHashBucketIter* pOutIter,
    GParamBucketVector* pBucketVector,
    size_t count,
    const GParamHashBucketIter* pFillVal,
    GParamHashBucketIter whereIter)
{
    return GParamBucketVector_InsertFillImpl(pOutIter, pBucketVector, count, pFillVal, whereIter);
}

// [VMA: 0x218afd0 - 0x218b1f0] (544 B) — operator[](uint64_t key) on GParam uint64->ptr hash map (variant A)
void* GParamHashTableU64A_FindOrInsertDefault(GParamHashTable* pHashTable, const uint64_t* pKey)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pBucketStart = pBuckets[bucketIdx * 2].pNode;
    GParamHashNodeBase* pBucketEnd = (pBucketStart == pListHead)
        ? pListHead
        : pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;

    for (GParamHashNodeBase* pCur = pBucketStart; pCur != pBucketEnd; pCur = pCur->m_pNext) {
        uint64_t curKey = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
        if (curKey >= *pKey) {
            if (*pKey >= curKey && pCur != pListHead) {
                return reinterpret_cast<uint8_t*>(pCur) + 0x18;
            }
            break;
        }
    }

    uint64_t newKey = *pKey;
    GParamHashNodeBase* pFirst = pListHead->m_pNext;
    GParamHashNodeBase* pFirstPrev = pFirst->m_pPrev;

    auto* pNewNode = static_cast<GParamHashNodeBase*>(
        pHashTable->m_pNodeAlloc->vtable->AllocateAligned(pHashTable->m_pNodeAlloc, 0x20, 8));
    if (pNewNode == nullptr) {
        GX_ReportOomOrThrow0x109();
    }

    pNewNode->m_pNext = (pFirst != nullptr) ? pFirst : pNewNode;
    pNewNode->m_pPrev = (pFirst != nullptr) ? pFirstPrev : pNewNode;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(pNewNode) + 0x10) = newKey;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(pNewNode) + 0x18) = 0;

    if (pHashTable->m_size == 0x07FFFFFFFFFFFFFEULL) {
        sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
    }
    ++pHashTable->m_size;
    pFirst->m_pPrev = pNewNode;
    pNewNode->m_pPrev->m_pNext = pNewNode;

    GParamHashNodeBase* pInserted = pHashTable->m_pListHead->m_pNext;
    const auto* pInsertedKey = reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pInserted) + 0x10);
    GParamHashBucketIter iter{nullptr, nullptr, pInserted};
    GParamHashInsertResult res{};
    GParamHashTableU64A_InsertOrFreeDuplicate(&res, pHashTable, pInsertedKey, iter);

    return reinterpret_cast<uint8_t*>(res.pNode) + 0x18;
}

struct GParamBlendSlot {
    uint64_t m_targetId; // +0x00 (-1 = 0xFFFFFFFFULL by default)
    uint8_t  m_bActive;  // +0x08
    uint32_t m_unk0C;    // +0x0C
    uint32_t m_unk10;    // +0x10
};

struct GParamBlendGroupState {
    GParamBlendSlot m_slots[17];     // +0x000 .. +0x198 (17 * 24 = 0x198 bytes)
    uint8_t         m_slotFlags[17]; // +0x198 .. +0x1A9
    uint8_t         m_pad1A9[7];     // +0x1A9 .. +0x1B0
};

void GParamBlendGroupState_Construct(GParamBlendGroupState* pThis);

// [VMA: 0x218ac30 - 0x218afc4] (916 B) — Copy-assign GParamOverrideMap (clear destination, resize buckets, clone 0x1C8-byte nodes)
void GParamOverrideMap_CopyAssign(GParamHashTable* pDstMap, const GParamHashTable* pSrcMap)
{
    pDstMap->m_bucketMask    = pSrcMap->m_bucketMask;
    pDstMap->m_bucketCount   = pSrcMap->m_bucketCount;
    pDstMap->m_maxLoadFactor = pSrcMap->m_maxLoadFactor;

    GParamHashNodeBase* pDstHead = pDstMap->m_pListHead;
    GParamHashNodeBase* pCur = pDstHead->m_pNext;
    pDstHead->m_pNext = pDstHead;
    pDstHead->m_pPrev = pDstHead;
    pDstMap->m_size = 0;

    while (pCur != pDstHead) {
        GParamHashNodeBase* pNext = pCur->m_pNext;
        pDstMap->m_pNodeAlloc->vtable->Free(pDstMap->m_pNodeAlloc, pCur);
        pCur = pNext;
    }

    pDstMap->m_pNodeAlloc->vtable->GetHeapId(pDstMap->m_pNodeAlloc);
    pSrcMap->m_pNodeAlloc->vtable->GetHeapId(pSrcMap->m_pNodeAlloc);

    size_t srcBucketVecCount = static_cast<size_t>(pSrcMap->m_buckets.m_pEnd - pSrcMap->m_buckets.m_pBegin);
    pDstMap->m_buckets.m_pEnd = pDstMap->m_buckets.m_pBegin;

    GParamHashBucketIter fillVal{nullptr, nullptr, pDstMap->m_pListHead};
    GParamHashBucketIter whereIter{nullptr, nullptr, reinterpret_cast<GParamHashNodeBase*>(pDstMap->m_buckets.m_pBegin)};
    GParamHashBucketIter dummyOut{};
    GParamBucketVectorA_InsertFill(&dummyOut, &pDstMap->m_buckets, srcBucketVecCount, &fillVal, whereIter);

    GParamHashNodeBase* pSrcHead = pSrcMap->m_pListHead;
    for (GParamHashNodeBase* pSrcNode = pSrcHead->m_pNext; pSrcNode != pSrcHead; pSrcNode = pSrcNode->m_pNext) {
        const auto* pSrcKey = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pSrcNode) + 0x10);
        uint64_t h = GParamHashKey(*pSrcKey);
        uint64_t mask = pDstMap->m_bucketMask;
        uint64_t bucketIdx = h & mask;
        if (pDstMap->m_bucketCount <= bucketIdx) {
            bucketIdx += ~(mask >> 1);
        }

        GParamHashBucketIter* pBuckets = pDstMap->m_buckets.m_pBegin;
        GParamHashNodeBase* pWhere = pBuckets[bucketIdx * 2].pNode;
        GParamHashNodeBase* pScan = pDstMap->m_pListHead;
        if (pWhere != pScan) {
            pScan = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
        }

        bool bDuplicate = false;
        while (pScan != pWhere) {
            pScan = pScan->m_pPrev;
            uint32_t scanKey = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pScan) + 0x10);
            if (*pSrcKey < scanKey) {
                continue;
            }
            if (scanKey >= *pSrcKey) {
                bDuplicate = true;
                break;
            }
            pWhere = pScan->m_pNext;
            break;
        }
        if (bDuplicate) {
            continue;
        }

        GParamHashNodeBase* pFirst = pDstMap->m_pListHead->m_pNext;
        GParamHashNodeBase* pFirstPrev = pFirst->m_pPrev;

        auto* pNewNode = static_cast<GParamHashNodeBase*>(
            pDstMap->m_pNodeAlloc->vtable->AllocateAligned(pDstMap->m_pNodeAlloc, 0x1C8, 8));
        if (pNewNode == nullptr) {
            GX_ReportOomOrThrow0x109();
        }

        pNewNode->m_pNext = (pFirst != nullptr) ? pFirst : pNewNode;
        pNewNode->m_pPrev = (pFirst != nullptr) ? pFirstPrev : pNewNode;
        std::memcpy(reinterpret_cast<uint8_t*>(pNewNode) + 0x10, pSrcKey, 0x1B8);

        if (pDstMap->m_size == 0x008FB823EE08FB81ULL) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        ++pDstMap->m_size;
        pFirst->m_pPrev = pNewNode;
        pNewNode->m_pPrev->m_pNext = pNewNode;

        GParamHashBucketIter insertIter{nullptr, nullptr, pDstMap->m_pListHead->m_pNext};
        GParamHashInsertResult tmpRes{};
        GParamHashTableU32_InsertNodeIntoBucketAndCheckRehash(&tmpRes, pDstMap, pWhere, bucketIdx, insertIter);
    }
}

// [VMA: 0x218a9f0 - 0x218ac27] (567 B) — operator[](uint32_t key) on GParamOverrideMap (allocates 0x1C8-byte node with GParamBlendGroupState)
GParamBlendGroupState* GParamOverrideMap_FindOrInsertDefault(GParamHashTable* pOverrideMap, const uint32_t* pKey)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pOverrideMap->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pOverrideMap->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pOverrideMap->m_pListHead;
    GParamHashBucketIter* pBuckets = pOverrideMap->m_buckets.m_pBegin;
    GParamHashNodeBase* pBucketStart = pBuckets[bucketIdx * 2].pNode;
    if (pBucketStart != pListHead) {
        GParamHashNodeBase* pBucketEnd = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
        for (GParamHashNodeBase* pCur = pBucketStart; pCur != pBucketEnd; pCur = pCur->m_pNext) {
            uint32_t curKey = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
            if (curKey >= *pKey) {
                if (*pKey >= curKey && pCur != pListHead) {
                    return reinterpret_cast<GParamBlendGroupState*>(reinterpret_cast<uint8_t*>(pCur) + 0x18);
                }
                break;
            }
        }
    }

    GParamBlendGroupState defaultBlendState{};
    GParamBlendGroupState_Construct(&defaultBlendState);

    uint32_t newKey = *pKey;
    GParamHashNodeBase* pFirst = pListHead->m_pNext;
    GParamHashNodeBase* pFirstPrev = pFirst->m_pPrev;

    auto* pNewNode = static_cast<GParamHashNodeBase*>(
        pOverrideMap->m_pNodeAlloc->vtable->AllocateAligned(pOverrideMap->m_pNodeAlloc, 0x1C8, 8));
    if (pNewNode == nullptr) {
        GX_ReportOomOrThrow0x109();
    }

    pNewNode->m_pNext = (pFirst != nullptr) ? pFirst : pNewNode;
    pNewNode->m_pPrev = (pFirst != nullptr) ? pFirstPrev : pNewNode;
    *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pNewNode) + 0x10) = newKey;
    std::memcpy(reinterpret_cast<uint8_t*>(pNewNode) + 0x18, &defaultBlendState, sizeof(GParamBlendGroupState));

    if (pOverrideMap->m_size == 0x008FB823EE08FB81ULL) {
        sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
    }
    ++pOverrideMap->m_size;
    pFirst->m_pPrev = pNewNode;
    pNewNode->m_pPrev->m_pNext = pNewNode;

    GParamHashNodeBase* pInserted = pOverrideMap->m_pListHead->m_pNext;
    const auto* pInsertedKey = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pInserted) + 0x10);
    GParamHashBucketIter iter{nullptr, nullptr, pInserted};
    GParamHashInsertResult res{};
    GParamHashTableU32_InsertOrFreeDuplicate(&res, pOverrideMap, pInsertedKey, iter);

    return reinterpret_cast<GParamBlendGroupState*>(reinterpret_cast<uint8_t*>(res.pNode) + 0x18);
}

// [VMA: 0x218a7a0 - 0x218a9e8] (584 B) — Construct default 0x1B0-byte GParamBlendGroupState (17 slots initialized to -1)
void GParamBlendGroupState_Construct(GParamBlendGroupState* pThis)
{
    for (int i = 0; i < 17; ++i) {
        pThis->m_slots[i].m_targetId = 0xFFFFFFFFFFFFFFFFULL;
        pThis->m_slots[i].m_bActive  = 0;
        pThis->m_slots[i].m_unk0C    = 0;
        pThis->m_slots[i].m_unk10    = 0;
        pThis->m_slotFlags[i]        = 0;
    }
}

// [VMA: 0x218a630 - 0x218a79e] (366 B) — Construct empty GParamOverrideMap (allocates sentinel 0x1C8-byte node and 16 bucket iterators)
void GParamOverrideMap_Construct(GParamHashTable* pOverrideMap, DLAllocator** ppAllocator)
{
    pOverrideMap->m_size = 0;
    pOverrideMap->m_pListHead = nullptr;
    DLAllocator* pAlloc = *ppAllocator;
    pOverrideMap->m_pNodeAlloc = pAlloc;

    auto* pHead = static_cast<GParamHashNodeBase*>(
        pAlloc->vtable->AllocateAligned(pAlloc, 0x1C8, 8));
    if (pHead != nullptr) {
        pHead->m_pNext = pHead;
    } else {
        GX_ReportOomOrThrow0x109();
    }
    pHead->m_pPrev = pHead;
    pOverrideMap->m_pListHead = pHead;

    pOverrideMap->m_buckets.m_pCap   = nullptr;
    pOverrideMap->m_buckets.m_pEnd   = nullptr;
    pOverrideMap->m_buckets.m_pBegin = nullptr;
    pOverrideMap->m_buckets.m_pAlloc = *ppAllocator;

    pOverrideMap->m_maxLoadFactor = 1.0f;
    pOverrideMap->m_buckets.m_pEnd = nullptr;

    GParamHashBucketIter fillVal{nullptr, nullptr, pHead};
    GParamHashBucketIter whereIter{nullptr, nullptr, nullptr};
    GParamHashBucketIter dummyOut{};
    GParamBucketVectorA_InsertFill(&dummyOut, &pOverrideMap->m_buckets, 16, &fillVal, whereIter);

    pOverrideMap->m_bucketMask  = 7;
    pOverrideMap->m_bucketCount = 8;
}

// [VMA: 0x218a220 - 0x218a628] (1032 B) — Default-construct 0x420-byte GParamLoadedFileEntry
void GParamLoadedFileEntry_Construct(void* pEntry)
{
    auto* pD = static_cast<uint8_t*>(pEntry);

    for (int s = 0; s < 2; ++s) {
        size_t strOffset = (s == 0) ? 0x10 : 0x48;
        DLAllocator* pStrAlloc = g_pGXStringAllocator;
        if (pStrAlloc == nullptr) {
            pStrAlloc = sub_2ba2b10();
            g_pGXStringAllocator = pStrAlloc;
        }
        uint8_t blockInfo[8]{};
        pStrAlloc->vtable->GetBlockInfo(blockInfo, pStrAlloc, nullptr);
        if ((blockInfo[0] & 0x20) == 0) {
            sub_20b55b0(0u, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }
        *reinterpret_cast<DLAllocator**>(pD + strOffset + 0x28) = pStrAlloc;
        *reinterpret_cast<uint64_t*>(pD + strOffset + 0x20) = 7;
        *reinterpret_cast<uint64_t*>(pD + strOffset + 0x18) = 0;
        *reinterpret_cast<uint16_t*>(pD + strOffset + 0x08) = 0;
        pD[strOffset + 0x30] = 1;
    }

    static const size_t kFixedStrOffsets[5] = {0x80, 0x130, 0x1E0, 0x290, 0x348};
    uint8_t dummyLocal[0x81]{};
    for (int i = 0; i < 5; ++i) {
        size_t base = kFixedStrOffsets[i];
        std::memcpy(pD + base + 0x28, dummyLocal, 0x81);
        pD[base + 0xA9] = 0;
        *reinterpret_cast<uint64_t*>(pD + base + 0x20) = 7;
        *reinterpret_cast<uint64_t*>(pD + base + 0x18) = 0;
        *reinterpret_cast<uint16_t*>(pD + base + 0x08) = 0;
        pD[base + 0xAA] = 1;

        sub_2935350(pD + base, 0x3F, 0);
        uint64_t cap = *reinterpret_cast<uint64_t*>(pD + base + 0x20);
        *reinterpret_cast<uint64_t*>(pD + base + 0x18) = 0;
        uint16_t* pBuf = (cap >= 8)
            ? *reinterpret_cast<uint16_t**>(pD + base + 0x08)
            : reinterpret_cast<uint16_t*>(pD + base + 0x08);
        pBuf[0] = 0;
    }

    *reinterpret_cast<uint64_t*>(pD + 0x00) = 0;
    *reinterpret_cast<uint32_t*>(pD + 0x08) = 0;
    const char16_t* pszEmpty = reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL));
    sub_262a720(pD + 0x10, pszEmpty, 0);
    sub_262a720(pD + 0x48, pszEmpty, 0);
    pD[0x340] = 0;
    *reinterpret_cast<uint64_t*>(pD + 0x418) = 0;
    *reinterpret_cast<uint64_t*>(pD + 0x410) = 0;
    *reinterpret_cast<uint64_t*>(pD + 0x408) = 0;
    *reinterpret_cast<uint64_t*>(pD + 0x400) = 0;
    *reinterpret_cast<uint64_t*>(pD + 0x3F8) = 0;
}

// [VMA: 0x218a010 - 0x218a219] (521 B) — operator[](uint64_t key) on GParam uint64->ptr hash map (variant B)
void* GParamHashTableU64B_FindOrInsertDefault(GParamHashTable* pHashTable, const uint64_t* pKey)
{
    uint64_t h = GParamHashKey(*pKey);
    uint64_t mask = pHashTable->m_bucketMask;
    uint64_t bucketIdx = h & mask;
    if (pHashTable->m_bucketCount <= bucketIdx) {
        bucketIdx += ~(mask >> 1);
    }

    GParamHashNodeBase* pListHead = pHashTable->m_pListHead;
    GParamHashBucketIter* pBuckets = pHashTable->m_buckets.m_pBegin;
    GParamHashNodeBase* pBucketStart = pBuckets[bucketIdx * 2].pNode;
    if (pBucketStart != pListHead) {
        GParamHashNodeBase* pBucketEnd = pBuckets[(bucketIdx * 2) | 1ULL].pNode->m_pNext;
        for (GParamHashNodeBase* pCur = pBucketStart; pCur != pBucketEnd; pCur = pCur->m_pNext) {
            uint64_t curKey = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pCur) + 0x10);
            if (curKey >= *pKey) {
                if (*pKey >= curKey && pCur != pListHead) {
                    return reinterpret_cast<uint8_t*>(pCur) + 0x18;
                }
                break;
            }
        }
    }

    uint64_t newKey = *pKey;
    GParamHashNodeBase* pFirst = pListHead->m_pNext;
    GParamHashNodeBase* pFirstPrev = pFirst->m_pPrev;

    auto* pNewNode = static_cast<GParamHashNodeBase*>(
        pHashTable->m_pNodeAlloc->vtable->AllocateAligned(pHashTable->m_pNodeAlloc, 0x20, 8));
    if (pNewNode == nullptr) {
        GX_ReportOomOrThrow0x109();
    }

    pNewNode->m_pNext = (pFirst != nullptr) ? pFirst : pNewNode;
    pNewNode->m_pPrev = (pFirst != nullptr) ? pFirstPrev : pNewNode;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(pNewNode) + 0x10) = newKey;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(pNewNode) + 0x18) = 0;

    if (pHashTable->m_size == 0x07FFFFFFFFFFFFFEULL) {
        sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
    }
    ++pHashTable->m_size;
    pFirst->m_pPrev = pNewNode;
    pNewNode->m_pPrev->m_pNext = pNewNode;

    GParamHashNodeBase* pInserted = pHashTable->m_pListHead->m_pNext;
    const auto* pInsertedKey = reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(pInserted) + 0x10);
    GParamHashBucketIter iter{nullptr, nullptr, pInserted};
    GParamHashInsertResult res{};
    GParamHashTableU64B_InsertOrFreeDuplicate(&res, pHashTable, pInsertedKey, iter);

    return reinterpret_cast<uint8_t*>(res.pNode) + 0x18;
}

} // namespace GX
