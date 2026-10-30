/**
 * @file GXDeviceSingletonAndCameraFrustum.cpp
 * @brief Decompiled GXDevice singleton lifecycle, frame submission, debug property tree builder,
 *        camera matrix/frustum block, deferred resource deletion handlers, and constant buffer helpers
 *        (VMA 0x2192000 .. 0x219afff, 80 functions, strictly Right-to-Left).
 *
 * All functions in this file have been physically carved from build/eboot_carved.bin
 * (patched with 0xE9 jmp + 0xCC int3 padding) and recorded in db/functions.json.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <immintrin.h>

namespace GX {

// ============================================================================
// Forward Declarations & External Helpers
// ============================================================================

struct GXAllocator;
struct GXDevice;
struct GXDeviceContext;
struct GXDisplayOutput;
struct GXRenderTargetResource;
struct GXDepthStencilResource;
struct GXTextureResource;
struct GXDebugPropertyNode;

extern "C" {
    void* sub_236c900(void* pTreeBuilder, const void* pNameDesc, uint32_t flags);
    void* sub_2351960(void* pNode, void* pParent, const void* pNameDesc, uint32_t flags);
    void  sub_2352120(void* pEnumNode, uint32_t index, const void* pOptionString);
    void  sub_2367220(void* pParentNode, void* pChildNode);
    void  sub_292f830(void* pTreeBuilder, const void* pNameDesc, void* pTargetBoolOrByte);
    void  sub_29316a0(void* pTreeBuilder, const void* pNameDesc, void* pTargetInt, int32_t minVal, int32_t maxVal, int32_t step, float scale);
    void  sub_2930840(void* pTreeBuilder, const void* pNameDesc, void* pTargetEnumInt, uint32_t count, const void* pEnumEntries);
    void  sub_2934560(void* pTreeBuilder, const void* pNameDesc, void* pTargetEnumByte, uint32_t count, const void* pEnumEntries);
    void  sub_2905e60(void* pTreeBuilder, const void* pNameDesc, void* pTargetUint32);
    void  sub_236d800(void* pTreeBuilder, const void* pNameDesc, void* pChildNode);
    void  sub_236c4d0(void* pTreeBuilder, void* pRootNode);
    void  sub_236c720(void* pTreeBuilder);
    void  sub_231d3a0(void* pOutString, const char* pLiteral);
    void  sub_2363080(void* pRootNode, void* pParentMenu, const void* pTitleString, uint32_t flags);
    void  sub_2330a80(void* pMenuHandle);
    void  sub_21ba300(void* pTexturePreviewNode);

    void  sub_219e320(void* pVec, uint64_t newCap);
    void  sub_219e220(void* pVec, uint64_t newCap);
    void  sub_219e120(void* pVec, uint64_t newCap);
    void  sub_219d9c0(void* pVec);
    void  sub_219d7b0(void* pVec);
    void  sub_219d550(void* pHeapState);
    void  sub_219db10(void* pTreeBuilder, const void* pNameDesc, uint8_t* pTargetByte);

    void  sub_21b4b00(void* pCmdQueue, void (*pfnCallback)(void*, void*), void* pArg0, void* pArg1, uint32_t mask);
    uint8_t sub_21b4a00(void* pCmdQueue, void* pSlot, uint32_t sortKey);
    void  sub_21b4f70(void* pBegin, void* pEnd, int64_t jobCount, void* pScratchDesc);
    void  sub_21b4d00(void* pSelfUnused, void* pImmediateCtx, void* pJobsArray, uint32_t jobCount);
    void  sub_219fcf0(void* pDeviceCtx);
    void  sub_21a1c50(void* pDeviceCtx);
    void  sub_21b6aa0(void* pSyncObj, uint8_t bMultiThreaded, uint8_t bVsyncChanged, void* pGammaRamp);
    void  sub_21b8710(void* pWorkerPool);
    void  sub_21b84b0(void* pWorkerPool);
    void  sub_21b8140(void* pWorkerPool, const void* pPoolDesc, const void* pThreadAffinities);
    void  sub_21b7050(uint32_t threadCount, uint32_t maxQueries);
    void* sub_21b7f60(void* pQueryMgr);
    void  sub_21b6d20(void* pQueryMgr);
    void  sub_21b6620(void* pSyncObj, uint64_t affinityMask, void* pConfig, void* pInitPacket);
    void  sub_21b3910(void* pCtxPool, uint32_t capacity);
    void  sub_21b3eb0(void* pCtxPool);
    void  sub_21b43c0(void* pCmdQueue, uint32_t capacity, void* pLinearHeap);
    void  sub_21b4770(void* pCmdQueue);
    void  sub_219e900(void* pThreadLocalPool);
    void  sub_219eb30(void* pThreadLocalPool);
    void  sub_21aeca0(void* pResourcePool);
    void  sub_21b26a0(void* pResourcePool);
    void  sub_21abac0(void* pSubMgr);
    void  sub_21abe40(void* pSubMgr);
    void  sub_21abf90(void* pSubMgr);
    void  sub_21a7f20(void* pTex3D, uint32_t w, uint32_t fmt, uint32_t h, const void* pSubRes, uint32_t d, void* pOut);
    void  sub_21a8680(void* pTexRegistry, void* pAllocator, uint32_t flags);
    void  sub_21a8950(void* pTexRegistry, const char* name, void* pTexture);
    void  sub_219f130(void* pDepthStencil, const void* pDesc, uint32_t a3, uint32_t a4);
    void  sub_21a32f0(void* pRenderTarget, uint32_t w, uint32_t h, uint32_t fmt, uint32_t mips, void* pDeviceAndHeap);
    void  sub_21919b0(void* pShaderPack, const void* pData, uint32_t size);
    bool  sub_21915a0(void* pShaderPack);
    void  sub_2191030(void* pShaderPack);
    void* sub_21aa290(void* pOutProjMatrix, const void* pCameraFrustum);
    void  sub_2bbe588(void* pPtr);
    float sub_2bbe648(float radians);

    void* sub_207b720(void* pAllocatedBlock);
    void  sub_26d73f0(void* pConstantBuffer, void* pLowLevelCtx, uint32_t offset, const void* pData);
    void** sub_26d7440(void* pConstantBuffer);

    void  sub_e46f40(void* pTargetHelper1728, void* pRtv0, void* pDsv, uint32_t flags);
    void  sub_e46fd0(void* pTargetHelper1728, void* pRtv0, void* pRtv1, void* pDsv, uint32_t flags);
    void* sub_e47840(void* pTargetHelper1728);
    void  sub_262a720(void* pOutWString, const wchar_t* pLiteral, uint32_t flags);
    void  sub_26af700(void* pGnmxGfxCtx);
    void  sub_26b4ef0(void* pLowLevelCtx, void* pRtvInner, void* pTexInner, const float* pClearColor);
    void  sub_26b5f50(void* pLowLevelCtx, void* pDsvInner, void* pTexInner, uint32_t clearFlags, uint32_t stencilVal, float depthVal);
    void  sub_26d3350(void* pViewportState, uint32_t count);
}

// ============================================================================
// 1. GXDevice Debug Property Subtree & Root Builder (0x219a160 .. 0x219ac70)
// ============================================================================

namespace {

struct GXScopedWNameDesc {
    void*       pManagedStrObj;
    const void* pLiteral;
};

static inline void GXRefObj_AddStrongAndWeak(uint8_t* pObj) {
    uint32_t v = *reinterpret_cast<uint32_t*>(pObj + 0x08);
    v = (v & 0xFFFF8000u) | ((v + 1u) & 0x7FFFu);
    v = (v & 0xC0007FFFu) | ((v + 0x8000u) & 0x3FFF8000u);
    *reinterpret_cast<uint32_t*>(pObj + 0x08) = v;
}

static inline void GXRefObj_ReleaseStrongAndWeak(uint8_t* pObj) {
    uint32_t oldRef    = *reinterpret_cast<uint32_t*>(pObj + 0x08);
    uint32_t newStrong = (oldRef + 0x7FFFu) & 0x7FFFu;
    uint32_t curRef    = (oldRef & 0xFFFF8000u) | newStrong;
    *reinterpret_cast<uint32_t*>(pObj + 0x08) = curRef;

    if (newStrong == 0u && (oldRef & 0x40000000u) != 0u) {
        void (**vtable)(void*) = *reinterpret_cast<void (***)(void*)>(pObj);
        vtable[3](pObj); // *0x18(%rax)
        curRef = *reinterpret_cast<uint32_t*>(pObj + 0x08) & 0xBFFFFFFFu;
        *reinterpret_cast<uint32_t*>(pObj + 0x08) = curRef;
    }

    uint32_t newWeak = ((curRef >> 15) + 0x7FFFu) & 0x7FFFu;
    *reinterpret_cast<uint32_t*>(pObj + 0x08) = (curRef & 0xC0007FFFu) | (newWeak << 15);
    if (newWeak == 0u) {
        *reinterpret_cast<uint32_t*>(pObj + 0x08) = curRef | 0x3FFF8000u;
        void (**vtable)(void*) = *reinterpret_cast<void (***)(void*)>(pObj);
        vtable[4](pObj); // *0x20(%rax)
    }
}

static inline void GXReleaseScopedWNameDesc(GXScopedWNameDesc* pDesc) {
    pDesc->pLiteral = nullptr;
    uint8_t* pStr = static_cast<uint8_t*>(pDesc->pManagedStrObj);
    if (pStr != nullptr) {
        if (--(*reinterpret_cast<int32_t*>(pStr + 0x38)) == 0) {
            void* pStrHeap = *reinterpret_cast<void**>(GX_VMA(0x5540e10ULL));
            if (*reinterpret_cast<uint64_t*>(pStr + 0x20) >= 8ULL) {
                void* pBufAlloc = *reinterpret_cast<void**>(pStr + 0x28);
                void (**bufVtbl)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pBufAlloc);
                bufVtbl[14](pBufAlloc, *reinterpret_cast<void**>(pStr + 0x08)); // *0x70
            }
            *reinterpret_cast<uint64_t*>(pStr + 0x20) = 7ULL;
            *reinterpret_cast<uint64_t*>(pStr + 0x18) = 0ULL;
            *reinterpret_cast<uint16_t*>(pStr + 0x08) = 0u;
            void (**heapVtbl)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pStrHeap);
            heapVtbl[14](pStrHeap, pStr); // *0x70
        }
        pDesc->pManagedStrObj = nullptr;
    }
}

static inline void GXPopTreeBuilderScope(uint8_t* pBuilder) {
    uint8_t** pBegin = *reinterpret_cast<uint8_t***>(pBuilder + 0x08);
    uint8_t** pEnd   = *reinterpret_cast<uint8_t***>(pBuilder + 0x10);
    if (pBegin != pEnd) {
        uint8_t* pNode = pEnd[-1];
        if (pNode != nullptr) {
            GXRefObj_ReleaseStrongAndWeak(pNode);
            pEnd[-1] = nullptr;
            pEnd = *reinterpret_cast<uint8_t***>(pBuilder + 0x10);
        }
        *reinterpret_cast<uint8_t***>(pBuilder + 0x10) = pEnd - 1;
    }
}

static inline void* GXTreeBeginGroup(uint8_t* pBuilder, const void* pWideLiteral, uint32_t flags) {
    auto pfnBeginGroup = reinterpret_cast<void* (*)(void*, const void*, uint32_t)>(GX_VMA(0x236c900ULL));
    GXScopedWNameDesc desc = { nullptr, pWideLiteral };
    void* pRes = pfnBeginGroup(pBuilder, &desc, flags);
    GXReleaseScopedWNameDesc(&desc);
    return pRes;
}

static inline void GXTreeAddBoolProp(uint8_t* pBuilder, const void* pWideLiteral, void* pTargetBool) {
    auto pfnAddBool = reinterpret_cast<void (*)(void*, const void*, void*)>(GX_VMA(0x292f830ULL));
    GXScopedWNameDesc desc = { nullptr, pWideLiteral };
    pfnAddBool(pBuilder, &desc, pTargetBool);
    GXReleaseScopedWNameDesc(&desc);
}

static inline void GXTreeAddUint32ReadOnlyProp(uint8_t* pBuilder, const void* pWideLiteral, void* pTargetU32) {
    auto pfnAddU32 = reinterpret_cast<void (*)(void*, const void*, void*)>(GX_VMA(0x2905e60ULL));
    GXScopedWNameDesc desc = { nullptr, pWideLiteral };
    pfnAddU32(pBuilder, &desc, pTargetU32);
    GXReleaseScopedWNameDesc(&desc);
}

static inline void GXAttachTexturePreviewAdapter(uint8_t* pBuilder, uintptr_t labelVma, uint8_t* pTexRes) {
    if (pTexRes == nullptr) {
        return;
    }
    GXScopedWNameDesc desc = { nullptr, reinterpret_cast<const void*>(GX_VMA(labelVma)) };
    void* pUiHeap = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    void* (**uiHeapVtbl)(void*, size_t, size_t) = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(pUiHeap);
    uint8_t* pAdapter = static_cast<uint8_t*>(uiHeapVtbl[11](pUiHeap, 0x28u, 8u)); // *0x58
    if (pAdapter != nullptr) {
        uint32_t hdr = (*reinterpret_cast<uint32_t*>(pAdapter + 0x08) & 0x80000000u) | 0x40000000u;
        *reinterpret_cast<uint32_t*>(pAdapter + 0x08) = hdr;
        *reinterpret_cast<void**>(pAdapter) = reinterpret_cast<void*>(GX_VMA(0x53b6c80ULL));
        *reinterpret_cast<uint16_t*>(pAdapter + 0x20) = 0u;
        *reinterpret_cast<uint64_t*>(pAdapter + 0x18) = 0ULL;
        *reinterpret_cast<void**>(pAdapter + 0x10) = pTexRes;
        if (pTexRes != nullptr) {
            uint32_t rc = *reinterpret_cast<uint32_t*>(pTexRes + 0x08);
            if (rc != 0u) {
                *reinterpret_cast<uint32_t*>(pTexRes + 0x08) = rc + 1u;
            }
            auto pfnInitPreview = reinterpret_cast<void (*)(void*)>(GX_VMA(0x21ba300ULL));
            pfnInitPreview(pAdapter);
        }
    }
    auto pfnAddChildNode = reinterpret_cast<void (*)(void*, const void*, void*)>(GX_VMA(0x236d800ULL));
    pfnAddChildNode(pBuilder, &desc, pAdapter);
    GXReleaseScopedWNameDesc(&desc);
}

} // namespace

/**
 * @brief Builds the "Draw System Settings" debug property subtree for GXDevice.
 * VMA: 0x219ac70 (5,084 bytes)
 */
void GXDevice_BuildDisplayAndShaderDebugPropertySubtree(uint8_t* pDevice, void* pTreeBuilder) {
    uint8_t* pBuilder = static_cast<uint8_t*>(pTreeBuilder);

    // 1. Open "Draw System Settings" (0x49c2e5c, flags = 0)
    if (GXTreeBeginGroup(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c2e5cULL)), 0u) == nullptr) {
        return;
    }

    // 2. Build custom "Execution Mode" enum combo widget bound to pDevice + 0x84
    void* pUiHeap = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    void* (**uiHeapVtbl)(void*, size_t, size_t) = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(pUiHeap);
    uint8_t* pWidget = static_cast<uint8_t*>(uiHeapVtbl[11](pUiHeap, 0x38u, 8u)); // *0x58
    if (pWidget != nullptr) {
        void* pParentNode = (*reinterpret_cast<void***>(pBuilder + 0x10))[-1];

        pUiHeap = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
        uiHeapVtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(pUiHeap);
        uint8_t* pEnumList = static_cast<uint8_t*>(uiHeapVtbl[11](pUiHeap, 0x38u, 8u)); // *0x58
        if (pEnumList != nullptr) {
            uint32_t hdr = (*reinterpret_cast<uint32_t*>(pEnumList + 0x08) & 0x80000000u) | 0x40000000u;
            *reinterpret_cast<uint32_t*>(pEnumList + 0x08) = hdr;
            *reinterpret_cast<void**>(pEnumList) = reinterpret_cast<void*>(GX_VMA(0x53b66e0ULL));

            void* pListAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540e18ULL));
            alignas(16) uint8_t heapCapsBuf[0x40] = {};
            void (**listAllocVtbl)(void*, void*, uint32_t) = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(pListAlloc);
            listAllocVtbl[4](heapCapsBuf, pListAlloc, 0u); // *0x20
            if ((heapCapsBuf[0] & 0x20u) == 0u) {
                auto pfnLogWarn = reinterpret_cast<void (*)(const char*, uint32_t, const char*, ...)>(GX_VMA(0x20b55b0ULL));
                pfnLogWarn(nullptr, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }

            *reinterpret_cast<void**>(pEnumList + 0x28) = nullptr;
            *reinterpret_cast<void**>(pEnumList + 0x20) = nullptr;
            *reinterpret_cast<void**>(pEnumList + 0x18) = nullptr;
            *reinterpret_cast<void**>(pEnumList + 0x30) = pListAlloc;

            auto pfnReserve24 = reinterpret_cast<void (*)(void*, uint64_t)>(GX_VMA(0x219e320ULL));
            pfnReserve24(pEnumList + 0x10, 2ULL);

            auto pfnCopyWNameDesc = reinterpret_cast<void (*)(void*, const void*)>(GX_VMA(0x2ba7bc0ULL));
            const uint8_t* pTable = reinterpret_cast<const uint8_t*>(GX_VMA(0x535f730ULL));
            for (uint32_t byteOff = 0; byteOff < 0x20u; byteOff += 0x10u) {
                struct TempEnumOption {
                    GXScopedWNameDesc name;
                    uint8_t           val;
                    uint8_t           pad[7];
                };
                TempEnumOption tempOpt = { { nullptr, nullptr }, pTable[byteOff + 0x08], {} };
                const void* pRawName = *reinterpret_cast<const void* const*>(pTable + byteOff);
                GXScopedWNameDesc srcDesc = { nullptr, pRawName };
                pfnCopyWNameDesc(&tempOpt.name, &srcDesc);
                srcDesc.pLiteral = nullptr;

                uint8_t* pEnd = *reinterpret_cast<uint8_t**>(pEnumList + 0x20);
                uint8_t* pCap = *reinterpret_cast<uint8_t**>(pEnumList + 0x28);
                if (pEnd == pCap) {
                    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pEnumList + 0x18);
                    uint64_t curLen = static_cast<uint64_t>(pEnd - pBegin) / 24ULL;
                    uint64_t half   = curLen >> 1;
                    uint64_t maxCap = 0xAAAAAAAAAAAAAAAULL;
                    uint64_t grown  = (maxCap - half >= curLen) ? (curLen + half) : 0ULL;
                    uint64_t nextCap = (grown < curLen + 1ULL) ? (curLen + 1ULL) : grown;
                    pfnReserve24(pEnumList + 0x10, nextCap);
                    pEnd = *reinterpret_cast<uint8_t**>(pEnumList + 0x20);
                }
                if (pEnd != nullptr) {
                    *reinterpret_cast<void**>(pEnd + 0x08) = nullptr;
                    *reinterpret_cast<void**>(pEnd + 0x00) = nullptr;
                    if (tempOpt.name.pLiteral != nullptr) {
                        *reinterpret_cast<const void**>(pEnd + 0x08) = tempOpt.name.pLiteral;
                    } else if (tempOpt.name.pManagedStrObj != nullptr) {
                        uint8_t* pStr = static_cast<uint8_t*>(tempOpt.name.pManagedStrObj);
                        *reinterpret_cast<void**>(pEnd + 0x00) = pStr;
                        ++(*reinterpret_cast<int32_t*>(pStr + 0x38));
                    }
                    pEnd[0x10] = tempOpt.val;
                }
                *reinterpret_cast<uint8_t**>(pEnumList + 0x20) += 0x18;
                GXReleaseScopedWNameDesc(&tempOpt.name);
            }
        }

        GXScopedWNameDesc execModeDesc = { nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c2e9aULL)) };
        auto pfnInitEnumWidget = reinterpret_cast<void (*)(void*, void*, const void*, uint32_t)>(GX_VMA(0x2351960ULL));
        pfnInitEnumWidget(pWidget, pParentNode, &execModeDesc, 0u);
        GXReleaseScopedWNameDesc(&execModeDesc);

        *reinterpret_cast<void**>(pWidget) = reinterpret_cast<void*>(GX_VMA(0x53b6720ULL));
        *reinterpret_cast<void**>(pWidget + 0x30) = nullptr;
        *reinterpret_cast<void**>(pWidget + 0x28) = pDevice + 0x84;
        pWidget[0x20] = pDevice[0x84];
        if (pEnumList != nullptr) {
            *reinterpret_cast<void**>(pWidget + 0x30) = pEnumList;
            GXRefObj_AddStrongAndWeak(pEnumList);
        }

        uint8_t* pUiSubObj = *reinterpret_cast<uint8_t**>(pWidget + 0x18);
        auto pfnClearCombo = reinterpret_cast<void (*)(void*)>(GX_VMA(0xf48810ULL));
        pfnClearCombo(*reinterpret_cast<void**>(pUiSubObj + 0x110));

        uint8_t* pListHolder = *reinterpret_cast<uint8_t**>(pWidget + 0x30);
        uint8_t* pItemsBegin = *reinterpret_cast<uint8_t**>(pListHolder + 0x18);
        uint8_t* pItemsEnd   = *reinterpret_cast<uint8_t**>(pListHolder + 0x20);
        auto pfnAddComboItem = reinterpret_cast<void (*)(void*, uint32_t, const void*)>(GX_VMA(0x2352120ULL));

        if (pItemsBegin != pItemsEnd) {
            uint32_t idx = 0u;
            while (true) {
                uint8_t* pItem = pItemsBegin + static_cast<size_t>(idx) * 24u;
                GXScopedWNameDesc optDesc = { nullptr, nullptr };
                const void* pLit = *reinterpret_cast<const void**>(pItem + 0x08);
                if (pLit != nullptr) {
                    optDesc.pLiteral = pLit;
                } else {
                    uint8_t* pStr = *reinterpret_cast<uint8_t**>(pItem + 0x00);
                    if (pStr != nullptr) {
                        optDesc.pManagedStrObj = pStr;
                        ++(*reinterpret_cast<int32_t*>(pStr + 0x38));
                    }
                }
                pfnAddComboItem(pWidget, idx, &optDesc);
                GXReleaseScopedWNameDesc(&optDesc);

                ++idx;
                pListHolder = *reinterpret_cast<uint8_t**>(pWidget + 0x30);
                pItemsBegin = *reinterpret_cast<uint8_t**>(pListHolder + 0x18);
                pItemsEnd   = *reinterpret_cast<uint8_t**>(pListHolder + 0x20);
                uint64_t totalCount = static_cast<uint64_t>(pItemsEnd - pItemsBegin) / 24ULL;
                if (idx >= totalCount) {
                    break;
                }
            }
        }

        uint32_t itemCount = static_cast<uint32_t>(static_cast<uint64_t>(pItemsEnd - pItemsBegin) / 24ULL);
        uint8_t curVal = pWidget[0x20];
        uint32_t selectedIdx = 0xFFFFFFFFu;
        for (uint32_t i = 0; i < itemCount; ++i) {
            if (pItemsBegin[static_cast<size_t>(i) * 24u + 0x10] == curVal) {
                selectedIdx = i;
                break;
            }
        }

        pUiSubObj = *reinterpret_cast<uint8_t**>(pWidget + 0x18);
        auto pfnSelectCombo = reinterpret_cast<void (*)(void*, uint32_t, uint32_t)>(GX_VMA(0xf48fe0ULL));
        pfnSelectCombo(*reinterpret_cast<void**>(pUiSubObj + 0x110), selectedIdx, 0u);

        GXRefObj_AddStrongAndWeak(pWidget);
        auto pfnAttachWidget = reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x2367220ULL));
        pfnAttachWidget((*reinterpret_cast<void***>(pBuilder + 0x10))[-1], pWidget);
        GXRefObj_ReleaseStrongAndWeak(pWidget);
    }

    // 3. Direct boolean & integer properties under "Draw System Settings"
    GXTreeAddBoolProp(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c2eb8ULL)), pDevice + 0x131); // "NoSubmitDrawSkip"
    GXTreeAddBoolProp(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c2edaULL)), pDevice + 0x132); // "FlipRenderThread"
    GXTreeAddBoolProp(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c2efcULL)), reinterpret_cast<void*>(GX_VMA(0x513ac85ULL))); // "Enable CommandListArray"

    {
        auto pfnAddIntSlider = reinterpret_cast<void (*)(void*, const void*, void*, int32_t, int32_t, int32_t, float)>(GX_VMA(0x29316a0ULL));
        GXScopedWNameDesc tcDesc = { nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c2f2cULL)) }; // "ThreadCount"
        float stepScale = *reinterpret_cast<const float*>(GX_VMA(0x49295f0ULL));
        pfnAddIntSlider(pBuilder, &tcDesc, pDevice + 0x2f0, 1, *reinterpret_cast<int32_t*>(pDevice + 0x280), 1, stepScale);
        GXReleaseScopedWNameDesc(&tcDesc);
    }

    // 4. "Thread Affinity" subgroup (0x49c2f44, flags = 0)
    if (GXTreeBeginGroup(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c2f44ULL)), 0u) != nullptr) {
        struct EnumPair32 {
            const void* pLabel;
            uint32_t    value;
            uint32_t    pad;
        };
        EnumPair32 affinityTypes[3] = {
            { reinterpret_cast<const void*>(GX_VMA(0x49c2f64ULL)), 0u, 0u }, // "All"
            { reinterpret_cast<const void*>(GX_VMA(0x49c2f6cULL)), 1u, 0u }, // "Cluster"
            { reinterpret_cast<const void*>(GX_VMA(0x49c2f7cULL)), 2u, 0u }, // "Core"
        };
        {
            auto pfnAddEnum32 = reinterpret_cast<void (*)(void*, const void*, void*, uint32_t, const void*)>(GX_VMA(0x2930840ULL));
            GXScopedWNameDesc affDesc = { nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c2f86ULL)) }; // "Affinity Type"
            pfnAddEnum32(pBuilder, &affDesc, reinterpret_cast<void*>(GX_VMA(0x56a6bd4ULL)), 3u, affinityTypes);
            GXReleaseScopedWNameDesc(&affDesc);
        }

        const uintptr_t kCoreLabelVMAs[6] = {
            0x49c303cULL, 0x49c304aULL, 0x49c3058ULL,
            0x49c3066ULL, 0x49c3074ULL, 0x49c3082ULL
        };

        // 5 Worker threads (table at 0x535f750, flags = 1, bools at 0x56a6bd8 + w * 6 + core)
        const void* const* pWorkerNames = reinterpret_cast<const void* const*>(GX_VMA(0x535f750ULL));
        uint8_t* pWorkerAffBase = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6bd8ULL));
        for (uint32_t w = 0; w < 5u; ++w) {
            if (GXTreeBeginGroup(pBuilder, pWorkerNames[w], 1u) != nullptr) {
                for (uint32_t c = 0; c < 6u; ++c) {
                    GXTreeAddBoolProp(pBuilder, reinterpret_cast<const void*>(GX_VMA(kCoreLabelVMAs[c])), pWorkerAffBase + w * 6u + c);
                }
                GXPopTreeBuilderScope(pBuilder);
            }
        }

        // "RenderThread" (0x49c3090, flags = 1, bools at 0x56a6c0c..0x56a6c11)
        if (GXTreeBeginGroup(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c3090ULL)), 1u) != nullptr) {
            uint8_t* pRenderAffBase = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c0cULL));
            for (uint32_t c = 0; c < 6u; ++c) {
                GXTreeAddBoolProp(pBuilder, reinterpret_cast<const void*>(GX_VMA(kCoreLabelVMAs[c])), pRenderAffBase + c);
            }
            GXPopTreeBuilderScope(pBuilder);
        }

        // "MainThread" (0x49c30aa, flags = 0, bools at 0x56a6c18..0x56a6c1d)
        if (GXTreeBeginGroup(pBuilder, reinterpret_cast<const void*>(GX_VMA(0x49c30aaULL)), 0u) != nullptr) {
            uint8_t* pMainAffBase = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c18ULL));
            for (uint32_t c = 0; c < 6u; ++c) {
                GXTreeAddBoolProp(pBuilder, reinterpret_cast<const void*>(GX_VMA(kCoreLabelVMAs[c])), pMainAffBase + c);
            }
            GXPopTreeBuilderScope(pBuilder);
        }

        GXPopTreeBuilderScope(pBuilder); // End "Thread Affinity"
    }

    GXPopTreeBuilderScope(pBuilder); // End "Draw System Settings"
}

/**
 * @brief Builds the root debug property tree (`+0x690`) for GXDevice, including per-display-output
 *        render target texture previews.
 * VMA: 0x219a170 (2,811 bytes)
 */
void* GXDevice_BuildRootDebugPropertyTree(uint8_t* pDevice) {
    uint8_t* pDebugMenuMgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6bc8ULL));
    if (pDebugMenuMgr == nullptr) {
        return nullptr;
    }

    uint8_t* pExisting = *reinterpret_cast<uint8_t**>(pDevice + 0x690);
    if (pExisting != nullptr) {
        if ((*reinterpret_cast<uint32_t*>(pExisting + 0x08) & 0x40000000u) != 0u && pExisting[0x6f] >= 2u) {
            for (uint8_t* cur = pExisting; cur != nullptr; cur = *reinterpret_cast<uint8_t**>(cur + 0x10)) {
                if ((cur[0x50] & 0x02u) != 0u) {
                    uint8_t* pSub20 = *reinterpret_cast<uint8_t**>(cur + 0x20);
                    auto pfnActivateMenu = reinterpret_cast<void (*)(void*)>(GX_VMA(0x2330a80ULL));
                    pfnActivateMenu(*reinterpret_cast<void**>(pSub20 + 0x10));
                    return *reinterpret_cast<void**>(pDevice + 0x690);
                }
            }
            return pExisting;
        }
        GXRefObj_ReleaseStrongAndWeak(pExisting);
        *reinterpret_cast<void**>(pDevice + 0x690) = nullptr;
    }

    void* pUiHeap = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    void* (**uiHeapVtbl)(void*, size_t, size_t) = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(pUiHeap);
    uint8_t* pNewRoot = static_cast<uint8_t*>(uiHeapVtbl[11](pUiHeap, 0x178u, 8u)); // *0x58

    void* pRootForBuilder = nullptr;
    if (pNewRoot != nullptr) {
        pDebugMenuMgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6bc8ULL));
        uint8_t* pMgrInner = *reinterpret_cast<uint8_t**>(pDebugMenuMgr + 0x10);
        void* pParentMenu  = *reinterpret_cast<void**>(pMgrInner + 0x18);

        GXScopedWNameDesc titleDesc = { nullptr, nullptr };
        auto pfnMakeWTitle = reinterpret_cast<void (*)(void*, const void*)>(GX_VMA(0x231d3a0ULL));
        pfnMakeWTitle(&titleDesc, reinterpret_cast<const void*>(GX_VMA(0x49c2e42ULL))); // "GXDrawSystem"

        auto pfnInitRootNode = reinterpret_cast<void (*)(void*, void*, const void*, uint32_t)>(GX_VMA(0x2363080ULL));
        pfnInitRootNode(pNewRoot, pParentMenu, &titleDesc, 0x51502u);

        uint8_t* pOldRoot = *reinterpret_cast<uint8_t**>(pDevice + 0x690);
        if (pOldRoot != pNewRoot) {
            if (pOldRoot != nullptr) {
                GXRefObj_ReleaseStrongAndWeak(pOldRoot);
                *reinterpret_cast<void**>(pDevice + 0x690) = nullptr;
            }
            *reinterpret_cast<void**>(pDevice + 0x690) = pNewRoot;
            GXRefObj_AddStrongAndWeak(pNewRoot);
        }

        GXReleaseScopedWNameDesc(&titleDesc);
        pRootForBuilder = *reinterpret_cast<void**>(pDevice + 0x690);
    } else {
        uint8_t* pOldRoot = *reinterpret_cast<uint8_t**>(pDevice + 0x690);
        if (pOldRoot != nullptr) {
            GXRefObj_ReleaseStrongAndWeak(pOldRoot);
            *reinterpret_cast<void**>(pDevice + 0x690) = nullptr;
        }
        pRootForBuilder = nullptr;
    }

    alignas(16) uint8_t builder[0x40] = {};
    auto pfnBuilderCtor = reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x236c4d0ULL));
    auto pfnBuilderDtor = reinterpret_cast<void (*)(void*)>(GX_VMA(0x236c720ULL));
    pfnBuilderCtor(builder, pRootForBuilder);

    GXDevice_BuildDisplayAndShaderDebugPropertySubtree(pDevice, builder);

    // "Properties" group (0x49c30c0, flags = 1)
    if (GXTreeBeginGroup(builder, reinterpret_cast<const void*>(GX_VMA(0x49c30c0ULL)), 1u) != nullptr) {
        {
            auto pfnAddWindowMode = reinterpret_cast<void (*)(void*, const void*, uint8_t*)>(GX_VMA(0x219db10ULL));
            GXScopedWNameDesc wmDesc = { nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c30d6ULL)) }; // "Window Mode"
            pfnAddWindowMode(builder, &wmDesc, pDevice + 0x105);
            GXReleaseScopedWNameDesc(&wmDesc);
        }
        GXTreeAddBoolProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c30eeULL)), pDevice + 0x104); // "60Hz Emulation"
        GXTreeAddBoolProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c310cULL)), reinterpret_cast<void*>(GX_VMA(0x55406d0ULL))); // "CopyResourcetoBackBuffer"
        GXTreeAddBoolProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c313eULL)), reinterpret_cast<void*>(GX_VMA(0x513ac86ULL))); // "FlushCmdBeforeSleep"

        {
            alignas(16) uint8_t debugLevelEnums[64];
            memcpy(debugLevelEnums +  0, reinterpret_cast<const void*>(GX_VMA(0x535f790ULL)), 16);
            memcpy(debugLevelEnums + 16, reinterpret_cast<const void*>(GX_VMA(0x535f7a0ULL)), 16);
            memcpy(debugLevelEnums + 32, reinterpret_cast<const void*>(GX_VMA(0x535f7b0ULL)), 16);
            memcpy(debugLevelEnums + 48, reinterpret_cast<const void*>(GX_VMA(0x535f7c0ULL)), 16);
            auto pfnAddEnumByte = reinterpret_cast<void (*)(void*, const void*, void*, uint32_t, const void*)>(GX_VMA(0x2934560ULL));
            GXScopedWNameDesc dlDesc = { nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c3184ULL)) }; // "Debug Level"
            pfnAddEnumByte(builder, &dlDesc, reinterpret_cast<void*>(GX_VMA(0x513ac80ULL)), 4u, debugLevelEnums);
            GXReleaseScopedWNameDesc(&dlDesc);
        }

        GXTreeAddBoolProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c319cULL)), reinterpret_cast<void*>(GX_VMA(0x55406c0ULL))); // "[Debug] Delay Render"
        GXPopTreeBuilderScope(builder);
    }

    // Per-Display-Output "Window" groups (stride 0xb0)
    for (uint8_t* pOut = *reinterpret_cast<uint8_t**>(pDevice + 0x90);
         pOut != *reinterpret_cast<uint8_t**>(pDevice + 0x98);
         pOut += 0xb0) {
        if (GXTreeBeginGroup(builder, reinterpret_cast<const void*>(GX_VMA(0x49c31c6ULL)), 1u) != nullptr) {
            uint32_t modeIdx = *reinterpret_cast<uint32_t*>(pOut + 0xa0);
            GXTreeAddUint32ReadOnlyProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c31d4ULL)), pDevice + 0x180 + static_cast<size_t>(modeIdx) * 24u);
            GXTreeAddUint32ReadOnlyProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c31eeULL)), pDevice + 0x184 + static_cast<size_t>(modeIdx) * 24u);
            GXTreeAddUint32ReadOnlyProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c320aULL)), pOut + 0x80);
            GXTreeAddUint32ReadOnlyProp(builder, reinterpret_cast<const void*>(GX_VMA(0x49c3232ULL)), pOut + 0x84);

            if (GXTreeBeginGroup(builder, reinterpret_cast<const void*>(GX_VMA(0x49c325cULL)), 0u) != nullptr) {
                GXAttachTexturePreviewAdapter(builder, 0x49d4304ULL, *reinterpret_cast<uint8_t**>(pOut + 0x68)); // "RenderTarget"
                GXAttachTexturePreviewAdapter(builder, 0x49c3278ULL, *reinterpret_cast<uint8_t**>(pOut + 0x78)); // "DepthR32F"
                GXPopTreeBuilderScope(builder);
            }
            GXPopTreeBuilderScope(builder);
        }
    }

    pfnBuilderDtor(builder);
    return *reinterpret_cast<void**>(pDevice + 0x690);
}

/**
 * @brief Sets the global debug menu manager pointer (`0x56a6bc8`).
 * VMA: 0x219a160 (8 bytes)
 */
void GXDevice_SetGlobalDebugMenuManager(void* /*pUnused*/, void* pDebugMenuMgr) {
#undef g_pGXDebugMenuManager_56a6bc8
#define g_pGXDebugMenuManager_56a6bc8 (*reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL)))
    g_pGXDebugMenuManager_56a6bc8 = pDebugMenuMgr;
}

// ============================================================================
// 2. GXDevice Listener & Callback Registration (0x2199be0 .. 0x219a070)
// ============================================================================

/**
 * @brief Appends a 16-byte `(pUserData, pCallbackObj)` pair to `pDevice->m_callbackPairs` (`+0x610`).
 * VMA: 0x219a070 (236 bytes)
 */
bool GXDevice_RegisterCallbackPair(uint8_t* pDevice, void* pUserData, void* pCallbackObj) {
    if (!pUserData || !pCallbackObj) {
        return false;
    }

    struct PairEntry {
        void* pUserData;
        void* pCallbackObj;
    };
    PairEntry localEntry = { pUserData, pCallbackObj };

    PairEntry* pEnd = *reinterpret_cast<PairEntry**>(pDevice + 0x620);
    PairEntry* pCap = *reinterpret_cast<PairEntry**>(pDevice + 0x628);

    if (&localEntry < pEnd) {
        PairEntry* pBegin = *reinterpret_cast<PairEntry**>(pDevice + 0x618);
        if (pBegin <= &localEntry) {
            ptrdiff_t idx = &localEntry - pBegin;
            if (pEnd == pCap) {
                sub_219d9c0(pDevice + 0x610);
                pEnd = *reinterpret_cast<PairEntry**>(pDevice + 0x620);
            }
            if (pEnd) {
                const PairEntry& src = (*reinterpret_cast<PairEntry**>(pDevice + 0x618))[idx];
                pEnd->pCallbackObj = src.pCallbackObj;
                pEnd->pUserData    = src.pUserData;
            }
            *reinterpret_cast<uint8_t**>(pDevice + 0x620) += sizeof(PairEntry);
            return true;
        }
    }

    if (pEnd == pCap) {
        sub_219d9c0(pDevice + 0x610);
        pEnd = *reinterpret_cast<PairEntry**>(pDevice + 0x620);
    }
    if (pEnd) {
        pEnd->pCallbackObj = localEntry.pCallbackObj;
        pEnd->pUserData    = localEntry.pUserData;
    }
    *reinterpret_cast<uint8_t**>(pDevice + 0x620) += sizeof(PairEntry);
    return true;
}

/**
 * @brief Removes a frame listener pointer from `pDevice->m_frameListeners` (`+0x328..0x330`).
 * VMA: 0x219a020 (73 bytes)
 */
void GXDevice_UnregisterFrameListener(uint8_t* pDevice, void* pListener) {
    void** pBegin = *(void***)(pDevice + 0x328);
    void** pEnd   = *(void***)(pDevice + 0x330);
    for (void** pCur = pBegin; pCur != pEnd; ++pCur) {
        if (*pCur == pListener) {
            size_t tailBytes = (size_t)((uint8_t*)pEnd - (uint8_t*)(pCur + 1));
            memmove(pCur, pCur + 1, tailBytes);
            *(void***)(pDevice + 0x330) -= 1;
            return;
        }
    }
}

/**
 * @brief Appends a frame listener pointer to `pDevice->m_frameListeners` (`+0x320..0x340`).
 * VMA: 0x2199f00 (288 bytes)
 */
void GXDevice_RegisterFrameListener(uint8_t* pDevice, void* pListener) {
    void* localListener = pListener;
    void** pEnd = *reinterpret_cast<void***>(pDevice + 0x330);
    void** pCap = *reinterpret_cast<void***>(pDevice + 0x338);

    auto calcNewCap = [](void** beginPtr, void** endPtr) -> uint64_t {
        uint64_t curLen = static_cast<uint64_t>(endPtr - beginPtr);
        uint64_t half   = curLen >> 1;
        uint64_t newCap = (0x1FFFFFFFFFFFFFFFULL - half >= curLen) ? (curLen + half) : 0ULL;
        if (newCap < curLen + 1ULL) {
            newCap = curLen + 1ULL;
        }
        return newCap;
    };

    if (&localListener < pEnd) {
        void** pBegin = *reinterpret_cast<void***>(pDevice + 0x328);
        if (pBegin <= &localListener) {
            ptrdiff_t idx = &localListener - pBegin;
            if (pEnd == pCap) {
                sub_219e220(pDevice + 0x320, calcNewCap(pBegin, pEnd));
                pEnd = *reinterpret_cast<void***>(pDevice + 0x330);
            }
            if (pEnd) {
                *pEnd = (*reinterpret_cast<void***>(pDevice + 0x328))[idx];
            }
            *reinterpret_cast<uint8_t**>(pDevice + 0x330) += sizeof(void*);
            return;
        }
    }

    if (pEnd == pCap) {
        void** pBegin = *reinterpret_cast<void***>(pDevice + 0x328);
        sub_219e220(pDevice + 0x320, calcNewCap(pBegin, pEnd));
        pEnd = *reinterpret_cast<void***>(pDevice + 0x330);
    }
    if (pEnd) {
        *pEnd = localListener;
    }
    *reinterpret_cast<uint8_t**>(pDevice + 0x330) += sizeof(void*);
}

/**
 * @brief Removes a resize listener from `pDevice->m_pResizeListenerList` (`+0x5f8`).
 * VMA: 0x2199eb0 (73 bytes)
 */
void GXDevice_UnregisterResizeListenerA(uint8_t* pDevice, void* pListener) {
    uint8_t* pVec = *(uint8_t**)(pDevice + 0x5f8);
    if (!pVec) {
        return;
    }
    void** pBegin = *(void***)(pVec + 0x08);
    void** pEnd   = *(void***)(pVec + 0x10);
    for (void** pCur = pBegin; pCur != pEnd; ++pCur) {
        if (*pCur == pListener) {
            memmove(pCur, pCur + 1, (size_t)((uint8_t*)pEnd - (uint8_t*)(pCur + 1)));
            *(void***)(pVec + 0x10) -= 1;
            return;
        }
    }
}

/**
 * @brief Removes a resize listener from `pDevice->m_pResizeListenerList` (`+0x5f8`, duplicate entry).
 * VMA: 0x2199e60 (73 bytes)
 */
void GXDevice_UnregisterResizeListenerB(uint8_t* pDevice, void* pListener) {
    GXDevice_UnregisterResizeListenerA(pDevice, pListener);
}

/**
 * @brief Appends a resize listener to `pDevice->m_pResizeListenerList` (`+0x5f8`).
 * VMA: 0x2199d40 (273 bytes)
 */
void GXDevice_RegisterResizeListener(uint8_t* pDevice, void* pListener) {
    uint8_t* pVec = *reinterpret_cast<uint8_t**>(pDevice + 0x5f8);
    if (!pVec) {
        return;
    }
    void* localListener = pListener;
    void** pEnd = *reinterpret_cast<void***>(pVec + 0x10);
    void** pCap = *reinterpret_cast<void***>(pVec + 0x18);

    auto calcNewCap = [](void** beginPtr, void** endPtr) -> uint64_t {
        uint64_t curLen = static_cast<uint64_t>(endPtr - beginPtr);
        uint64_t half   = curLen >> 1;
        uint64_t newCap = (0x1FFFFFFFFFFFFFFFULL - half >= curLen) ? (curLen + half) : 0ULL;
        if (newCap < curLen + 1ULL) {
            newCap = curLen + 1ULL;
        }
        return newCap;
    };

    if (&localListener < pEnd) {
        void** pBegin = *reinterpret_cast<void***>(pVec + 0x08);
        if (pBegin <= &localListener) {
            ptrdiff_t idx = &localListener - pBegin;
            if (pEnd == pCap) {
                sub_219e120(pVec, calcNewCap(pBegin, pEnd));
                pEnd = *reinterpret_cast<void***>(pVec + 0x10);
            }
            if (pEnd) {
                *pEnd = (*reinterpret_cast<void***>(pVec + 0x08))[idx];
            }
            *reinterpret_cast<uint8_t**>(pVec + 0x10) += sizeof(void*);
            return;
        }
    }

    if (pEnd == pCap) {
        void** pBegin = *reinterpret_cast<void***>(pVec + 0x08);
        sub_219e120(pVec, calcNewCap(pBegin, pEnd));
        pEnd = *reinterpret_cast<void***>(pVec + 0x10);
    }
    if (pEnd) {
        *pEnd = localListener;
    }
    *reinterpret_cast<uint8_t**>(pVec + 0x10) += sizeof(void*);
}

/**
 * @brief Returns `pDevice->m_displayConfigFlags100` (`+0x100`).
 * VMA: 0x2199d30 (7 bytes)
 */
uint32_t GXDevice_GetDisplayConfigFlags100(const uint8_t* pDevice) {
    return *(const uint32_t*)(pDevice + 0x100);
}

/**
 * @brief Sets `pDevice->m_displayConfigFlags100` (`+0x100`).
 * VMA: 0x2199d20 (7 bytes)
 */
void GXDevice_SetDisplayConfigFlags100(uint8_t* pDevice, uint32_t flags) {
    *(uint32_t*)(pDevice + 0x100) = flags;
}

/**
 * @brief Copies 24 bytes of gamma curve parameters into `+0x3b8..0x3d0` and marks gamma dirty (`+0x3ab = 1`).
 * VMA: 0x2199cf0 (41 bytes)
 */
void GXDevice_SetGammaCurveParams(uint8_t* pDevice, const uint64_t* pGammaSource) {
    uint64_t w2 = pGammaSource[3];
    *(uint64_t*)(pDevice + 0x3c8) = w2;
    uint64_t w0 = pGammaSource[1];
    uint64_t w1 = pGammaSource[2];
    *(uint64_t*)(pDevice + 0x3c0) = w1;
    *(uint64_t*)(pDevice + 0x3b8) = w0;
    pDevice[0x3ab] = 1;
}

/**
 * @brief Notifies all registered resize listeners in `+0x5f8` via virtual slot `+0x18`.
 * VMA: 0x2199c90 (90 bytes)
 */
void GXDevice_NotifyResizeListeners(uint8_t* pDevice, uint32_t width, uint32_t height) {
    uint8_t* pVec = *(uint8_t**)(pDevice + 0x5f8);
    if (!pVec) {
        return;
    }
    for (void** pCur = *(void***)(pVec + 0x08); pCur != *(void***)(pVec + 0x10); ++pCur) {
        void* pListener = *pCur;
        void (**vtable)(void*, uint32_t, uint32_t) = *(void (***)(void*, uint32_t, uint32_t))pListener;
        vtable[3](pListener, width, height);
    }
}

/**
 * @brief Returns the global debug menu manager pointer (`0x56a6bc8`).
 * VMA: 0x2199c80 (8 bytes)
 */
void* GXDevice_GetGlobalDebugMenuManager() {
#undef g_pGXDebugMenuManager_56a6bc8
#define g_pGXDebugMenuManager_56a6bc8 (*reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL)))
    return g_pGXDebugMenuManager_56a6bc8;
}

/**
 * @brief Stub returning true (`1`).
 * VMA: 0x2199c70 (3 bytes)
 */
bool GXDevice_ReturnTrueStubA() {
    return true;
}

/**
 * @brief No-op stub.
 * VMA: 0x2199c60 (1 byte)
 */
void GXDevice_NoOpStubA() {}

/**
 * @brief Returns `pDevice->m_pDefaultSamplerState` (`+0x248`).
 * VMA: 0x2199c50 (8 bytes)
 */
void* GXDevice_GetDefaultSamplerState(const uint8_t* pDevice) {
    return *(void* const*)(pDevice + 0x248);
}

/**
 * @brief Returns the address of global static handle `0x56a6c24`.
 * VMA: 0x2199c40 (8 bytes)
 */
void* GXDevice_GetGlobalStaticHandle56A6C24() {
#undef g_unk56a6c24
#define g_unk56a6c24 (*reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c24ULL)))
    return &g_unk56a6c24;
}

/**
 * @brief Stub returning true (`1`).
 * VMA: 0x2199c30 (3 bytes)
 */
bool GXDevice_ReturnTrueStubB() {
    return true;
}

/**
 * @brief No-op stub.
 * VMA: 0x2199c20 (1 byte)
 */
void GXDevice_NoOpStubB() {}

/**
 * @brief Presents the frame via `m_pRenderThreadSync` (`+0x348`) unless `m_bSkipPresent3A9` (`+0x3a9`) is set.
 * VMA: 0x2199be0 (64 bytes)
 */
void GXDevice_PresentOrClearSkipFlag(uint8_t* pDevice) {
    if (pDevice[0x3a9] != 0) {
        pDevice[0x3a9] = 0;
        return;
    }
#undef g_lastVsyncState_513ac84
#define g_lastVsyncState_513ac84 (*reinterpret_cast<uint8_t*>(GX_VMA(0x513ac84ULL)))
    uint8_t bVsyncChanged = (g_lastVsyncState_513ac84 != pDevice[0x104]) ? 1 : 0;
    sub_21b6aa0(
        *(void**)(pDevice + 0x348),
        pDevice[0x84],
        bVsyncChanged,
        *(void**)(pDevice + 0xb0));
}

// ============================================================================
// 3. GXDevice Frame Submission, Sync & Singleton Lifecycle (0x2193880 .. 0x2199af0)
// ============================================================================

void GXDevice_ResetContextStateAndBindDefaultDisplayTargets(uint8_t* pDevice, uint8_t* pDeviceCtx, uint8_t bClearTargets);

/**
 * @brief Render job callback that resets the immediate `GnmxGfxContext` and flushes dirty state.
 * VMA: 0x2199a40 (173 bytes)
 */
void GXDevice_PrepareImmediateContextForSubmissionJob(void* /*pJobUnused*/, void* pDeviceVoid) {
    uint8_t* pDevice = static_cast<uint8_t*>(pDeviceVoid);
    uint8_t* pImmediateCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x10);

    void* pPerfDebugMenu = *reinterpret_cast<void**>(pDevice + 0x638);
    if (pPerfDebugMenu != nullptr) {
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x217b870ULL))(pPerfDebugMenu, pImmediateCtx);
    }

    uint8_t* pRtStack = pImmediateCtx + 0x1728;
    reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0xe47290ULL))(pRtStack, 1u);
    void* pTopRt = sub_e47840(pRtStack);
    *reinterpret_cast<void**>(pImmediateCtx + 0x1ec0) = pTopRt;
    pImmediateCtx[0x1670] &= 0xFEu;

    if (pImmediateCtx[0x209c] != 0) {
        sub_219fcf0(pImmediateCtx);
        pImmediateCtx[0x209c] = 0;
    }

    *reinterpret_cast<uint64_t*>(pDevice + 0x608) += 1ULL;

    void* pTimestampProfiler = *reinterpret_cast<void**>(pDevice + 0x640);
    if (pTimestampProfiler != nullptr) {
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c1bd0ULL))(
            pTimestampProfiler, *reinterpret_cast<void**>(pDevice + 0x10));
    }

    void* pDeferredReleaseQueue = *reinterpret_cast<void**>(pDevice + 0x660);
    if (pDeferredReleaseQueue != nullptr) {
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21bd250ULL))(
            pDeferredReleaseQueue, *reinterpret_cast<void**>(pDevice + 0x10));
    }
}

/**
 * @brief Immediately sorts and executes a `GXCommandQueue` on the immediate context.
 * VMA: 0x2199af0 (232 bytes)
 */
void GXDevice_ExecuteCommandQueueImmediate(uint8_t* pDevice, uint8_t* pCmdQueue) {
    if (!pCmdQueue) {
        return;
    }
    sub_21b4b00(
        pCmdQueue,
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x2199a40ULL)),
        pDevice,
        nullptr,
        0xFFFFFFFFu);

    int64_t jobCount = *reinterpret_cast<int32_t*>(pCmdQueue + 0x38);
    if (jobCount != 0) {
        uint8_t* pJobs = *reinterpret_cast<uint8_t**>(pCmdQueue + 0x30);
        struct SortScratchContext {
            void*    pBuffer;
            int64_t  allocCount;
            uint64_t zero10;
            int64_t  maxHalfCount;
            void*    pSelf;
        } scratch = { nullptr, 0, 0ULL, (jobCount + 1) / 2, nullptr };
        scratch.pSelf = &scratch;
        sub_21b4f70(pJobs, pJobs + (jobCount << 4), jobCount, &scratch);
        if (scratch.pBuffer != nullptr) {
            sub_2bbe588(scratch.pBuffer);
        }
    }

    void* pTimestampProfiler = *reinterpret_cast<void**>(pDevice + 0x640);
    if (pTimestampProfiler != nullptr) {
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c17b0ULL))(
            pTimestampProfiler, *reinterpret_cast<void**>(pDevice + 0x10));
    }

    GXDevice_ResetContextStateAndBindDefaultDisplayTargets(
        pDevice, *reinterpret_cast<uint8_t**>(pDevice + 0x10), 0);
    sub_21b4d00(
        pDevice,
        *reinterpret_cast<void**>(pDevice + 0x10),
        *reinterpret_cast<void**>(pCmdQueue + 0x30),
        *reinterpret_cast<uint32_t*>(pCmdQueue + 0x38));
}

static inline void GXDevice_SortAndReplayQueue(uint8_t* pDevice) {
    int64_t curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    uint8_t* pCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + curIdx * 8);
    int64_t jobCount = *reinterpret_cast<int32_t*>(pCmdQueue + 0x38);
    if (jobCount != 0) {
        uint8_t* pBegin = *reinterpret_cast<uint8_t**>(pCmdQueue + 0x30);
        uint8_t* pEnd = pBegin + (jobCount << 4);
        struct SortScratchDesc {
            void*    pTempBuffer;
            uint64_t field_08;
            uint64_t field_10;
            int64_t  halfCapacity;
            void*    pSelfDesc;
        } scratch;
        scratch.pTempBuffer  = nullptr;
        scratch.field_08     = 0;
        scratch.field_10     = 0;
        scratch.halfCapacity = (jobCount + 1) / 2;
        scratch.pSelfDesc    = &scratch;

        sub_21b4f70(pBegin, pEnd, jobCount, &scratch);
        if (scratch.pTempBuffer != nullptr) {
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x2bbe588ULL))(scratch.pTempBuffer);
        }
    }

    curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    pCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + curIdx * 8);
    sub_21b4d00(
        pCmdQueue,
        *reinterpret_cast<void**>(pDevice + 0x10),
        *reinterpret_cast<void**>(pCmdQueue + 0x30),
        *reinterpret_cast<uint32_t*>(pCmdQueue + 0x38));
}

static inline void GXDevice_FlipAndResetNextFrameSlot(uint8_t* pDevice) {
    int64_t oldIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    int64_t nextIdx = oldIdx ^ 1;
    *reinterpret_cast<int32_t*>(pDevice + 0x380) = static_cast<int32_t>(nextIdx);

    uint8_t* pNextCtxPool = *reinterpret_cast<uint8_t**>(pDevice + 0x350 + nextIdx * 8);
    *reinterpret_cast<uint32_t*>(pNextCtxPool + 0x28) = 0u;

    uint8_t* pNextCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + nextIdx * 8);
    *reinterpret_cast<uint32_t*>(pNextCmdQueue + 0x28) = 0u;
    *reinterpret_cast<uint32_t*>(pNextCmdQueue + 0x38) = 0u;
    auto pfnResetRing = reinterpret_cast<void (*)(void*, void*, uint32_t)>(GX_VMA(0xe45530ULL));
    pfnResetRing(pNextCmdQueue + 0x48, *reinterpret_cast<void**>(pNextCmdQueue + 0xa8), 0x800u);
    pfnResetRing(pNextCmdQueue + 0x78, *reinterpret_cast<void**>(pNextCmdQueue + 0xa8), 0x800u);
    *reinterpret_cast<uint32_t*>(pNextCmdQueue + 0x40) = 0u;

    int64_t reloadedIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    void* pNextLinearHeap = *reinterpret_cast<void**>(pDevice + 0x370 + reloadedIdx * 8);
    void (**pHeapVtable)(void*) = *reinterpret_cast<void (***)(void*)>(pNextLinearHeap);
    pHeapVtable[15](pNextLinearHeap);
}

static inline void GXDevice_CopyProfilerAveragesToPerfMenu(uint8_t* pPerfMenu, const uint8_t* pProfiler) {
    const float* pSrc = reinterpret_cast<const float*>(pProfiler + 0x6a3d8);
    float* pDst = reinterpret_cast<float*>(pPerfMenu + 0x08);
    for (size_t i = 0; i < 19; ++i) {
        pDst[i] = (pSrc[i] * 1000.0f) / 60.0f;
    }
}

static inline void GXDevice_ResetTaskStatsAndThreadLocalPool(uint8_t* pDevice) {
    uint8_t* pTaskStatsMgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    if (pTaskStatsMgr != nullptr) {
        uint32_t count = *reinterpret_cast<uint32_t*>(pTaskStatsMgr + 0x08);
        if (count != 0) {
            uint8_t* pEntry = *reinterpret_cast<uint8_t**>(pTaskStatsMgr);
            for (uint32_t i = 0; i < count; ++i, pEntry += 0x60) {
                int32_t cur50 = *reinterpret_cast<int32_t*>(pEntry + 0x50);
                *reinterpret_cast<int32_t*>(pEntry + 0x44) = cur50;
                int32_t max48 = *reinterpret_cast<int32_t*>(pEntry + 0x48);
                if (cur50 >= max48) {
                    max48 = cur50;
                }
                *reinterpret_cast<int32_t*>(pEntry + 0x48) = max48;
                int32_t cur58 = *reinterpret_cast<int32_t*>(pEntry + 0x58);
                *reinterpret_cast<int32_t*>(pEntry + 0x4c) = cur58;
                *reinterpret_cast<uint32_t*>(pEntry + 0x50) = 0u;
                *reinterpret_cast<uint32_t*>(pEntry + 0x58) = 0u;
            }
        }
    }

    uint8_t* pTlPool = *reinterpret_cast<uint8_t**>(pDevice + 0x258);
    reinterpret_cast<void (*)(void*)>(GX_VMA(0xe316e0ULL))(*reinterpret_cast<void**>(pTlPool));

    uint8_t* pSingleton = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint32_t workerCount = *reinterpret_cast<uint32_t*>(pSingleton + 0x280);
    auto pfnResetSlice = reinterpret_cast<void (*)(void*, void*, uint32_t)>(GX_VMA(0xe31820ULL));
    for (uint32_t i = 0; i < workerCount; ++i) {
        const uint32_t* pOffsets = *reinterpret_cast<const uint32_t**>(pTlPool + 0x18);
        uint32_t elemIdx = pOffsets[i];
        void* pPoolCore = *reinterpret_cast<void**>(pTlPool);
        uint8_t* pSliceBase = *reinterpret_cast<uint8_t**>(pTlPool + 0x08);
        void* pSlice = pSliceBase + static_cast<uint64_t>(elemIdx) * 8ULL;
        uint32_t sliceCount = *reinterpret_cast<uint32_t*>(pTlPool + 0x28);
        pfnResetSlice(pPoolCore, pSlice, sliceCount);
    }
}

static inline void GXDevice_FinalizeContextRenderTargetsAndFlush(uint8_t* pDeviceCtx) {
    void* pRtStack = pDeviceCtx + 0x1728;
    reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0xe47290ULL))(pRtStack, 1u);
    *reinterpret_cast<void**>(pDeviceCtx + 0x1ec0) =
        reinterpret_cast<void* (*)(void*)>(GX_VMA(0xe47840ULL))(pRtStack);
    pDeviceCtx[0x1670] &= 0xFEu;
    if (pDeviceCtx[0x209c] != 0) {
        sub_219fcf0(pDeviceCtx);
        pDeviceCtx[0x209c] = 0;
    }
}

/**
 * @brief Ends the current frame, sorts and submits the active command queue, flips double-buffered
 *        heaps/allocators, updates gamma ramp and GPU timer queries, and signals the render thread.
 * VMA: 0x2198fd0 (2,660 bytes)
 */
void GXDevice_EndFrameAndFlip(uint8_t* pDevice) {
    if (pDevice[0x3a9] != 0) {
        if (pDevice[0x131] == 0) {
            uint8_t* pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
            void (**pWaitVtable)(void*, uint32_t) = *reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68);
            pWaitVtable[3](pSync + 0x68, 0xFFFFFFFFu);

            GXDevice_SortAndReplayQueue(pDevice);
            GXDevice_FlipAndResetNextFrameSlot(pDevice);

            void* pProfiler = *reinterpret_cast<void**>(pDevice + 0x640);
            if (pProfiler != nullptr) {
                reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c1bd0ULL))(
                    pProfiler, *reinterpret_cast<void**>(pDevice + 0x10));
            }
        }
        sub_21b8710(pDevice + 0x278);
        GXDevice_ResetTaskStatsAndThreadLocalPool(pDevice);
        return;
    }

    sub_21b8710(pDevice + 0x278);

    if (pDevice[0x84] != 0) {
        if (pDevice[0x3ab] != 0) {
            auto pfnRingAlloc = reinterpret_cast<uint8_t* (*)(void*, uint32_t, uint32_t)>(GX_VMA(0xe45560ULL));

            int64_t curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
            uint8_t* pCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + curIdx * 8);
            void* pLock = pCmdQueue + 0xc8;
            (*reinterpret_cast<void (***)(void*, uint32_t)>(pLock))[3](pLock, 0xFFFFFFFFu);
            uint8_t* pGammaPacket = pfnRingAlloc(pCmdQueue + 0x78, 0x20u, 0x10u);
            (*reinterpret_cast<void (***)(void*)>(pLock))[5](pLock);

            uint8_t* pInitGamma = nullptr;
            if (pGammaPacket != nullptr) {
                *reinterpret_cast<void**>(pGammaPacket) =
                    reinterpret_cast<uint8_t*>(GX_VMA(0x535f820ULL)) + 0x10;
                memcpy(pGammaPacket + 0x08, pDevice + 0x3b8, 24);
                pInitGamma = pGammaPacket;
            }

            curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
            pCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + curIdx * 8);
            pLock = pCmdQueue + 0xc8;
            (*reinterpret_cast<void (***)(void*, uint32_t)>(pLock))[3](pLock, 0xFFFFFFFFu);
            void** pNode = reinterpret_cast<void**>(pfnRingAlloc(pCmdQueue + 0x78, 0x10u, 0x10u));
            (*reinterpret_cast<void (***)(void*)>(pLock))[5](pLock);

            // Native C++ equivalent of bbport InstallGxNodeGuard (0x2199310 -> 0x219934e)
            if (pNode != nullptr) {
                pNode[1] = pInitGamma;
                pNode[0] = *reinterpret_cast<void**>(pDevice + 0xb0);
                curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
                sub_21b4b00(
                    *reinterpret_cast<void**>(pDevice + 0x360 + curIdx * 8),
                    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x219d930ULL)),
                    pNode,
                    nullptr,
                    0xFFFFFFFFu);
                pDevice[0x3ab] = 0;
            }
        }

        uint8_t* pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
        (*reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68))[3](pSync + 0x68, 0xFFFFFFFFu);

        int64_t slotIdx = *reinterpret_cast<int32_t*>(pDevice + 0x68c);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + slotIdx * 16) = 0u;

        uint8_t* pPerfMenu = *reinterpret_cast<uint8_t**>(pDevice + 0x638);
        uint8_t* pMainCtx = nullptr;
        if (pPerfMenu == nullptr) {
            pMainCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x78);
        } else {
            uint8_t* pProfiler = *reinterpret_cast<uint8_t**>(pDevice + 0x640);
            if (pProfiler == nullptr) {
                pMainCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x78);
                reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x217b870ULL))(pPerfMenu, pMainCtx);
            } else {
                GXDevice_CopyProfilerAveragesToPerfMenu(pPerfMenu, pProfiler);
                pPerfMenu = *reinterpret_cast<uint8_t**>(pDevice + 0x638);
                pMainCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x78);
                if (pPerfMenu != nullptr) {
                    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x217b870ULL))(pPerfMenu, pMainCtx);
                }
            }
        }
        GXDevice_FinalizeContextRenderTargetsAndFlush(pMainCtx);
    } else {
        uint8_t* pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
        (*reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68))[3](pSync + 0x68, 0xFFFFFFFFu);

        GXDevice_SortAndReplayQueue(pDevice);
        GXDevice_FlipAndResetNextFrameSlot(pDevice);

        if (pDevice[0x3ab] != 0) {
            uint8_t* pGammaTarget = *reinterpret_cast<uint8_t**>(pDevice + 0xb0);
            alignas(16) uint8_t gammaScratch[0x3010];
            void* pGammaEval = pDevice + 0x3b0;
            (**reinterpret_cast<void (***)(void*, void*)>(pGammaEval))(pGammaEval, gammaScratch);
            reinterpret_cast<void* (*)(void*, const void*, uint32_t)>(GX_VMA(0x2bbe578ULL))(
                pGammaTarget + 0x10, gammaScratch, 0x300Cu);
            pGammaTarget[0x301c] = 1u;
            pDevice[0x3ab] = 0;
        }

        uint8_t* pImmCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x10);
        void* pPerfMenu = *reinterpret_cast<void**>(pDevice + 0x638);
        if (pPerfMenu != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x217b870ULL))(pPerfMenu, pImmCtx);
        }
        GXDevice_FinalizeContextRenderTargetsAndFlush(pImmCtx);

        *reinterpret_cast<uint64_t*>(pDevice + 0x608) += 1ULL;
        void* pProfiler = *reinterpret_cast<void**>(pDevice + 0x640);
        if (pProfiler != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c1bd0ULL))(
                pProfiler, *reinterpret_cast<void**>(pDevice + 0x10));
        }
        void* pDeferredQueue = *reinterpret_cast<void**>(pDevice + 0x660);
        if (pDeferredQueue != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21bd250ULL))(
                pDeferredQueue, *reinterpret_cast<void**>(pDevice + 0x10));
        }

        pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
        (*reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68))[3](pSync + 0x68, 0xFFFFFFFFu);

        int64_t slotIdx = *reinterpret_cast<int32_t*>(pDevice + 0x68c);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + slotIdx * 16) = 0u;

        uint8_t* pPerfMenu2 = *reinterpret_cast<uint8_t**>(pDevice + 0x638);
        if (pPerfMenu2 != nullptr) {
            uint8_t* pProf2 = *reinterpret_cast<uint8_t**>(pDevice + 0x640);
            if (pProf2 != nullptr) {
                GXDevice_CopyProfilerAveragesToPerfMenu(pPerfMenu2, pProf2);
            }
        }
    }

    GXDevice_ResetTaskStatsAndThreadLocalPool(pDevice);
    pDevice[0x3aa] = 0;

    if (pDevice[0x84] != 0) {
        int64_t curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
        sub_21b4b00(
            *reinterpret_cast<void**>(pDevice + 0x360 + curIdx * 8),
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x2194410ULL)),
            pDevice,
            *reinterpret_cast<void**>(pDevice + 0x600),
            0xFFFFFFFFu);

        uint8_t* pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
        curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
        void* pSubmittedQueue = *reinterpret_cast<void**>(pDevice + 0x360 + curIdx * 8);
        void* pSavedGammaOut = *reinterpret_cast<void**>(pDevice + 0xb0);

        (*reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68))[3](pSync + 0x68, 0xFFFFFFFFu);
        *reinterpret_cast<void**>(pSync + 0x18) = pSubmittedQueue;

        uint8_t* pSyncCfg = *reinterpret_cast<uint8_t**>(pSync + 0x88);
        if (pSyncCfg[0x72] != 0) {
            *reinterpret_cast<void**>(pSync + 0x20) = pSavedGammaOut;
        }

        uint8_t* pSingleton = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        if (*reinterpret_cast<void**>(pSingleton + 0x640) != nullptr) {
            *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cbaULL)) =
                *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cb9ULL));
        }

        auto pfnSetEvent = reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x207e780ULL));
        pfnSetEvent(pSync + 0x68, 0u);
        pfnSetEvent(pSync + 0x28, 1u);

        GXDevice_FlipAndResetNextFrameSlot(pDevice);

        int64_t old68c = *reinterpret_cast<int32_t*>(pDevice + 0x68c);
        int32_t old688 = *reinterpret_cast<int32_t*>(pDevice + 0x688);
        *reinterpret_cast<int32_t*>(pDevice + 0x68c) = old688;
        *reinterpret_cast<int32_t*>(pDevice + 0x688) = static_cast<int32_t>(old68c);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + old68c * 16) = 0u;
    } else {
        *reinterpret_cast<uint64_t*>(pDevice + 0x608) = *reinterpret_cast<uint64_t*>(pDevice + 0x600);
        int64_t idx68c = *reinterpret_cast<int32_t*>(pDevice + 0x68c);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + idx68c * 16) = 0u;
        int64_t idx688 = *reinterpret_cast<int32_t*>(pDevice + 0x688);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + idx688 * 16) = 0u;
    }
}

/**
 * @brief Resets a `GXDeviceContext`'s shadow state if uninitialized, binds the primary display output's
 *        backbuffer RTV and depth-stencil DSV, and optionally clears both targets.
 * VMA: 0x2198c10 (947 bytes)
 */
void GXDevice_ResetContextStateAndBindDefaultDisplayTargets(
    uint8_t* pDevice,
    uint8_t* pDeviceCtx,
    uint8_t bClearTargets)
{
    uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
    uint8_t* pPrimaryOutput = *(uint8_t**)(pDevice + 0x90);

    pLowLevelCtx[0xb656] = 1;
    *(void**)(pLowLevelCtx + 0xbc10) = *(void**)(pPrimaryOutput + 0x60);
    *(void**)(pLowLevelCtx + 0xbc50) = nullptr;
    pLowLevelCtx[0xbcb5] = 1;

    // Set default viewport to primary display resolution (`pDevice + modeIdx * 24 + 0x180`)
    uint32_t modeIdx = *(uint32_t*)(pPrimaryOutput + 0xa0);
    uint32_t dispW   = *(uint32_t*)(pDevice + (size_t)modeIdx * 24 + 0x180);
    uint32_t dispH   = *(uint32_t*)(pDevice + (size_t)modeIdx * 24 + 0x184);
    float vp[6] = { 0.0f, 0.0f, (float)dispW, (float)dispH, 0.0f, 1.0f };
    sub_26d3350(pLowLevelCtx + 0xb528, 1);
    *(uint32_t*)(pLowLevelCtx + 0xbbf8) = 1;
    memcpy(pLowLevelCtx + 0xb978, vp, sizeof(vp));

    if (pDeviceCtx[0x209c] == 0) {
        pDeviceCtx[0x2092] = 0;
        *(uint16_t*)(pDeviceCtx + 0x2090) = 0;
        memset(pDeviceCtx + 0x10, 0, 0x1718);
        *(uint64_t*)(pDeviceCtx + 0x1ec8) = 0;
        *(uint64_t*)(pDeviceCtx + 0x1ec0) = 0;

        // Mark all 6 shader stages' CB/SRV/Sampler/Shader masks dirty
        for (size_t stageOffset = 0x678; stageOffset <= 0x1500; stageOffset += 0x2E8) {
            *(uint32_t*)(pDeviceCtx + stageOffset + 0x000) = 0x3B;
            *(uint32_t*)(pDeviceCtx + stageOffset + 0x078) = 0x0E;
            *(uint32_t*)(pDeviceCtx + stageOffset + 0x100) = 0x10;
        }
        *(uint32_t*)(pDeviceCtx + 0x1680) = 0x20;
        *(uint32_t*)(pDeviceCtx + 0x1688) = 0x08;
        sub_262a720(pDeviceCtx + 0x20a8, (const wchar_t*)GX_VMA(0x49d85f0ULL), 0);
        *(uint32_t*)(pDeviceCtx + 0x20e0) = 0;
        pDeviceCtx[0x209c] = 1;
    }

    // Resolve primary color RTV0 (+0x68), optional secondary RTV1 (+0x78), and DSV (+0x70)
    uint8_t* pColorRes0 = *(uint8_t**)(pPrimaryOutput + 0x68);
    uint8_t* pColorRes1 = *(uint8_t**)(pPrimaryOutput + 0x78);
    void* pRtv0 = nullptr;
    if (*(void**)(pColorRes0 + 0x40) && *(void***)(pColorRes0 + 0x60)) {
        auto fnGetMips = *(uint32_t(**)(void*))(*(uintptr_t*)pColorRes0 + 0x40);
        if (fnGetMips(pColorRes0) != 0) {
            pRtv0 = (**(void****)(pColorRes0 + 0x60));
        }
    }

    uint8_t* pTargetHelper = pDeviceCtx + 0x1728;
    if (pColorRes1) {
        void* pRtv1 = nullptr;
        if (*(void**)(pColorRes1 + 0x40) && *(void***)(pColorRes1 + 0x60)) {
            auto fnGetMips = *(uint32_t(**)(void*))(*(uintptr_t*)pColorRes1 + 0x40);
            if (fnGetMips(pColorRes1) != 0) {
                pRtv1 = (**(void****)(pColorRes1 + 0x60));
            }
        }
        uint8_t* pDepthRes = *(uint8_t**)(pPrimaryOutput + 0x70);
        void* pDsv = nullptr;
        if (*(void***)(pDepthRes + 0x60)) {
            auto fnGetMips = *(uint32_t(**)(void*))(*(uintptr_t*)pDepthRes + 0x40);
            if (fnGetMips(pDepthRes) != 0) {
                pDsv = (**(void****)(pDepthRes + 0x60));
            }
        }
        sub_e46fd0(pTargetHelper, pRtv0, pRtv1, pDsv, 1);
    } else {
        uint8_t* pDepthRes = *(uint8_t**)(pPrimaryOutput + 0x70);
        void* pDsv = nullptr;
        if (*(void***)(pDepthRes + 0x60)) {
            auto fnGetMips = *(uint32_t(**)(void*))(*(uintptr_t*)pDepthRes + 0x40);
            if (fnGetMips(pDepthRes) != 0) {
                pDsv = (**(void****)(pDepthRes + 0x60));
            }
        }
        sub_e46f40(pTargetHelper, pRtv0, pDsv, 1);
    }

    *(void**)(pDeviceCtx + 0x1ec0) = sub_e47840(pTargetHelper);
    sub_21a1c50(pDeviceCtx);

    if (bClearTargets != 0) {
        uint8_t* pMainColorRes = *(uint8_t**)(pPrimaryOutput + 0x68);
        (*(uint32_t(**)(void*))(*(uintptr_t*)pMainColorRes + 0x40))(pMainColorRes);
        uint8_t* pMainRtv = **(uint8_t***)(pMainColorRes + 0x60);
        uint8_t* pMainTex = *(uint8_t**)(pMainRtv + 0x08);
        sub_26b4ef0(pLowLevelCtx, pMainRtv + 0x10, pMainTex + 0x18, (const float*)(pDevice + 0x120));
        sub_26af700(pLowLevelCtx + 0x10);

        uint8_t* pMainDepthRes = *(uint8_t**)(pPrimaryOutput + 0x70);
        uint8_t* pMainDsv = nullptr;
        if (*(void***)(pMainDepthRes + 0x60)) {
            auto fnGetMips = *(uint32_t(**)(void*))(*(uintptr_t*)pMainDepthRes + 0x40);
            if (fnGetMips(pMainDepthRes) != 0) {
                pMainDsv = **(uint8_t***)(pMainDepthRes + 0x60);
            }
        }
        uint8_t* pDepthTex = *(uint8_t**)(pMainDsv + 0x08);
        sub_26b5f50(pLowLevelCtx, pMainDsv + 0x10, pDepthTex + 0x18, 0, 1, 1.0f);
        sub_26af700(pLowLevelCtx + 0x10);
    }
}

/**
 * @brief Dispatches queued worker thread render jobs, notifies frame listeners before and after,
 *        and waits for worker pool completion.
 * VMA: 0x2198940 (710 bytes)
 */
static inline void GXWorkerPool_WakePendingWorkers(uint8_t* pDevice) {
    *reinterpret_cast<uint64_t*>(pDevice + 0x288) = *reinterpret_cast<uint64_t*>(pDevice + 0x2e8);
    uint32_t threadCount = *reinterpret_cast<uint32_t*>(pDevice + 0x280);
    uint32_t wakeCount   = threadCount - 1u;
    volatile uint32_t* pAtomicPending = reinterpret_cast<volatile uint32_t*>(pDevice + 0x290);

    uint32_t releasedCount = 0u;
    if (pDevice[0x2e0] != 0) {
        uint32_t pending = *pAtomicPending;
        if (pending < wakeCount) {
            wakeCount = pending;
        }
        if (pDevice[0x2e0] != 0) {
            if (wakeCount != 0u) {
                for (uint32_t i = 0u; i < wakeCount; ++i) {
                    __sync_fetch_and_sub(pAtomicPending, 1u);
                }
                void* pSemaphore = *reinterpret_cast<void**>(pDevice + 0x298);
                if (pSemaphore != nullptr) {
                    reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b0d0ULL))(pSemaphore, wakeCount);
                }
                releasedCount = wakeCount;
            }
        } else {
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            __atomic_exchange_n(pAtomicPending, 0u, __ATOMIC_SEQ_CST);
            if (wakeCount != 0u) {
                void* pSemaphore = *reinterpret_cast<void**>(pDevice + 0x298);
                if (pSemaphore != nullptr) {
                    reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b0d0ULL))(pSemaphore, wakeCount);
                }
                releasedCount = wakeCount;
            }
        }
    } else {
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        __atomic_exchange_n(pAtomicPending, 0u, __ATOMIC_SEQ_CST);
        if (wakeCount != 0u) {
            void* pSemaphore = *reinterpret_cast<void**>(pDevice + 0x298);
            if (pSemaphore != nullptr) {
                reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b0d0ULL))(pSemaphore, wakeCount);
            }
            releasedCount = wakeCount;
        }
    }

    if (pDevice[0x2e0] != 0) {
        releasedCount += *reinterpret_cast<uint32_t*>(pDevice + 0x2ac);
    }
    *reinterpret_cast<uint32_t*>(pDevice + 0x2ac) = releasedCount;
    pDevice[0x2a9] = 0u;
    pDevice[0x2e0] = 1u;
}

void GXDevice_DispatchWorkerThreadJobsAndWait(uint8_t* pDevice) {
    uint8_t* pWorkerPool = pDevice + 0x278;
    sub_21b8710(pWorkerPool);

    uint8_t* pSingleton = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pSingleton[0x2e0] != 0) {
        sub_21b8710(pSingleton + 0x278);
    }

    for (void** pCur = *reinterpret_cast<void***>(pDevice + 0x328);
         pCur != *reinterpret_cast<void***>(pDevice + 0x330);
         ++pCur) {
        void* pListener = *pCur;
        void (**vtable)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pListener);
        vtable[0](pListener, pWorkerPool);
    }

    for (void** pTaskCur = *reinterpret_cast<void***>(pDevice + 0x300);
         pTaskCur != *reinterpret_cast<void***>(pDevice + 0x308);
         ++pTaskCur) {
        void* pTask = *pTaskCur;
        uint8_t* pInnerQueue = *reinterpret_cast<uint8_t**>(pWorkerPool) + 0x18;
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(pInnerQueue, pTask);
        if (pDevice[0x2e0] != 0) {
            *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1u;
        }
        pDevice[0x2a9] = 1u;
    }

    GXWorkerPool_WakePendingWorkers(pDevice);

    sub_21b8710(pWorkerPool);
    pSingleton = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pSingleton[0x2e0] != 0) {
        sub_21b8710(pSingleton + 0x278);
    }

    for (void** pCur = *reinterpret_cast<void***>(pDevice + 0x328);
         pCur != *reinterpret_cast<void***>(pDevice + 0x330);
         ++pCur) {
        void* pListener = *pCur;
        void (**vtable)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pListener);
        vtable[1](pListener, pWorkerPool);
    }

    void** ppTaskMgr = *reinterpret_cast<void***>(GX_VMA(0x56a6c28ULL));
    uint8_t* pBarrierNode = static_cast<uint8_t*>(
        reinterpret_cast<void* (*)(void*)>(GX_VMA(0x21b7f60ULL))(*ppTaskMgr));
    *reinterpret_cast<void**>(pBarrierNode + 0x08) = reinterpret_cast<void*>(GX_VMA(0x53bf1e8ULL));
    *reinterpret_cast<uint32_t*>(pBarrierNode + 0x10) = 0u;
    *reinterpret_cast<uint64_t*>(pBarrierNode + 0x14) = 0x0000000100000001ULL;

    uint8_t* pInnerQueue = *reinterpret_cast<uint8_t**>(pWorkerPool) + 0x18;
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(pInnerQueue, pBarrierNode);
    if (pDevice[0x2e0] != 0) {
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1u;
    }
    pDevice[0x2a9] = 1u;

    GXWorkerPool_WakePendingWorkers(pDevice);
}

static inline uint32_t GXDevice_PackSixCoreAffinityBools(const uint8_t* pCoreBools) {
    uint32_t mask = pCoreBools[0];
    if (pCoreBools[1] != 0) mask |= 0x02u;
    if (pCoreBools[2] != 0) mask |= 0x04u;
    if (pCoreBools[3] != 0) mask |= 0x08u;
    if (pCoreBools[4] != 0) mask |= 0x10u;
    if (pCoreBools[5] != 0) mask |= 0x20u;
    return mask;
}

/**
 * @brief Begins a new frame on `GXDevice`, updates worker thread CPU core affinity masks if changed,
 *        processes deferred deletions, and prepares the active context.
 * VMA: 0x21982f0 (1,614 bytes)
 */
void GXDevice_BeginFrameAndSyncThreadAffinities(uint8_t* pDevice, uint8_t bSkipPresent) {
    uint32_t affinityType = *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bd4ULL));
    if (affinityType != *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bd0ULL))) {
        uint8_t* pWorkerBools = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6bd8ULL));
        if (affinityType == 0u) {
            memset(pWorkerBools, 1, 30);
        } else if (affinityType == 1u) {
            for (size_t w = 0; w < 3; ++w) {
                pWorkerBools[w * 6 + 0] = 1u;
                pWorkerBools[w * 6 + 1] = 1u;
                pWorkerBools[w * 6 + 2] = 1u;
                pWorkerBools[w * 6 + 3] = 1u;
                pWorkerBools[w * 6 + 4] = 0u;
                pWorkerBools[w * 6 + 5] = 0u;
            }
            for (size_t w = 3; w < 5; ++w) {
                pWorkerBools[w * 6 + 0] = 0u;
                pWorkerBools[w * 6 + 1] = 0u;
                pWorkerBools[w * 6 + 2] = 0u;
                pWorkerBools[w * 6 + 3] = 0u;
                pWorkerBools[w * 6 + 4] = 1u;
                pWorkerBools[w * 6 + 5] = 1u;
            }
        } else if (affinityType == 2u) {
            memset(pWorkerBools, 0, 30);
            for (size_t w = 0; w < 5; ++w) {
                pWorkerBools[w * 6 + (w + 1)] = 1u;
            }
        }
        *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bd0ULL)) = affinityType;
    }

    auto pfnSetAffinity = reinterpret_cast<int (*)(void*, uint64_t)>(GX_VMA(0x2bbf2f8ULL));
    const uint8_t* pWorkerBoolRows = reinterpret_cast<const uint8_t*>(GX_VMA(0x56a6bd8ULL));
    uint32_t* pCachedWorkerMasks = reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bf8ULL));
    for (size_t i = 0; i < 5; ++i) {
        uint32_t mask = GXDevice_PackSixCoreAffinityBools(pWorkerBoolRows + i * 6);
        if (mask != pCachedWorkerMasks[i]) {
            uint8_t* pWorkersArray = *reinterpret_cast<uint8_t**>(pDevice + 0x278);
            void* hWorkerThread = *reinterpret_cast<void**>(pWorkersArray + 0x5d8 + i * 0x530);
            pfnSetAffinity(hWorkerThread, static_cast<uint64_t>(mask) & 0x3FULL);
            pCachedWorkerMasks[i] = mask;
        }
    }

    uint8_t* pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
    if (pSync != nullptr) {
        uint32_t rtMask = GXDevice_PackSixCoreAffinityBools(
            reinterpret_cast<const uint8_t*>(GX_VMA(0x56a6c0cULL)));
        uint32_t* pCachedRtMask = reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c14ULL));
        if (rtMask != *pCachedRtMask) {
            uint8_t* pRtThreadObj = *reinterpret_cast<uint8_t**>(pSync + 0x08);
            void* hRenderThread = *reinterpret_cast<void**>(pRtThreadObj + 0x18);
            pfnSetAffinity(hRenderThread, static_cast<uint64_t>(rtMask) & 0x3FULL);
            *pCachedRtMask = rtMask;
        }
    }

    {
        uint32_t mainMask = GXDevice_PackSixCoreAffinityBools(
            reinterpret_cast<const uint8_t*>(GX_VMA(0x56a6c18ULL)));
        uint32_t* pCachedMainMask = reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c20ULL));
        if (mainMask != *pCachedMainMask) {
            void* hSelfThread = reinterpret_cast<void* (*)()>(GX_VMA(0x2bbfe78ULL))();
            pfnSetAffinity(hSelfThread, static_cast<uint64_t>(mainMask) & 0x3FULL);
            *pCachedMainMask = mainMask;
        }
    }

    pDevice[0x3a9] = bSkipPresent;
    if (bSkipPresent != 0) {
        if (pDevice[0x131] != 0) {
            return;
        }
        GXDevice_DispatchWorkerThreadJobsAndWait(pDevice);

        pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
        (*reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68))[3](pSync + 0x68, 0xFFFFFFFFu);

        int64_t slotIdx = *reinterpret_cast<int32_t*>(pDevice + 0x68c);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + slotIdx * 16) = 0u;

        uint8_t* pPerfMenu = *reinterpret_cast<uint8_t**>(pDevice + 0x638);
        if (pPerfMenu != nullptr) {
            uint8_t* pProfiler = *reinterpret_cast<uint8_t**>(pDevice + 0x640);
            if (pProfiler == nullptr) {
                return;
            }
            GXDevice_CopyProfilerAveragesToPerfMenu(pPerfMenu, pProfiler);
        }

        void* pProfiler = *reinterpret_cast<void**>(pDevice + 0x640);
        if (pProfiler != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c1540ULL))(
                pProfiler, *reinterpret_cast<void**>(pDevice + 0x10));
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c17b0ULL))(
                *reinterpret_cast<void**>(pDevice + 0x640), *reinterpret_cast<void**>(pDevice + 0x10));
        }
        return;
    }

    *reinterpret_cast<uint64_t*>(pDevice + 0x600) += 1ULL;
    reinterpret_cast<void (*)(void*)>(GX_VMA(0xe45d80ULL))(
        *reinterpret_cast<void**>(pDevice + 0x268));

    uint8_t execMode;
    if (pDevice[0x86] != 0) {
        execMode = pDevice[0x84];
    } else {
        execMode = pDevice[0x130];
        pDevice[0x84] = execMode;
        pDevice[0x86] = 1u;
    }

    if (execMode == 0 && (pDevice[0x85] != 0 || pDevice[0x132] != 0)) {
        pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
        (*reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68))[3](pSync + 0x68, 0xFFFFFFFFu);

        int64_t slotIdx = *reinterpret_cast<int32_t*>(pDevice + 0x68c);
        *reinterpret_cast<uint32_t*>(pDevice + 0x670 + slotIdx * 16) = 0u;
        execMode = pDevice[0x84];
    }
    pDevice[0x85] = execMode;

    struct {
        int64_t tv_sec;
        int64_t tv_usec;
    } tv = {0, 0};
    reinterpret_cast<int (*)(void*, void*)>(GX_VMA(0x2bbe728ULL))(&tv, nullptr);
    int64_t nowUs = tv.tv_sec * 1000000LL + tv.tv_usec;

    if (*reinterpret_cast<void**>(pDevice + 0x638) != nullptr) {
        uint8_t* pSingleton = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        uint8_t* pLowDev = *reinterpret_cast<uint8_t**>(pSingleton);
        reinterpret_cast<uint64_t (*)(void*)>(GX_VMA(0x26aab20ULL))(
            *reinterpret_cast<void**>(pLowDev + 0x18));
    }
    *reinterpret_cast<int64_t*>(pDevice + 0x658) = nowUs;

    void* pProfiler = *reinterpret_cast<void**>(pDevice + 0x640);
    if (pProfiler != nullptr) {
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c1540ULL))(
            pProfiler, *reinterpret_cast<void**>(pDevice + 0x78));
        uint8_t* pProfReload = *reinterpret_cast<uint8_t**>(pDevice + 0x640);
        uint8_t flag = reinterpret_cast<uint8_t (*)()>(GX_VMA(0x1089c30ULL))();
        *reinterpret_cast<uint8_t*>(GX_VMA(0x55406d1ULL)) |= flag;
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cb9ULL)) = pProfReload[0x6a432];
    }

    if (pDevice[0x84] != 0) {
        GXDevice_ResetContextStateAndBindDefaultDisplayTargets(
            pDevice, *reinterpret_cast<uint8_t**>(pDevice + 0x78), 1u);
        GXDevice_DispatchWorkerThreadJobsAndWait(pDevice);
    } else {
        GXDevice_DispatchWorkerThreadJobsAndWait(pDevice);
        void* pProf = *reinterpret_cast<void**>(pDevice + 0x640);
        if (pProf != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x21c17b0ULL))(
                pProf, *reinterpret_cast<void**>(pDevice + 0x10));
        }
        GXDevice_ResetContextStateAndBindDefaultDisplayTargets(
            pDevice, *reinterpret_cast<uint8_t**>(pDevice + 0x10), 1u);
    }
    pDevice[0x3aa] = 1u;
}

void GXDevice_BaseDtor(uint8_t* pDevice);

namespace {

static inline void GXDevice_HeapFreeBlock(void* pHeap, void* pPtr) {
    void (**heapVtbl)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pHeap);
    heapVtbl[14](pHeap, pPtr); // *0x70(%rax)
}

static inline void GXDevice_ReleaseSimpleRef8(uint8_t* pObj) {
    int32_t oldRc = *reinterpret_cast<int32_t*>(pObj + 0x08);
    *reinterpret_cast<int32_t*>(pObj + 0x08) = oldRc - 1;
    if (oldRc <= 1) {
        void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pObj);
        vtbl[0](pObj);
    }
}

static inline void GXDevice_ResetLowLevelDeferredGnmCtx(uint8_t* pLowCtx) {
    alignas(32) uint8_t zero72[0x48] = {};
    reinterpret_cast<void (*)(void*, void*, const void*)>(GX_VMA(0x26d3070ULL))(pLowCtx + 0xb558, pLowCtx, zero72);
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x26b9850ULL))(pLowCtx + 0xb478, pLowCtx + 0x10);
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x26cd190ULL))(pLowCtx + 0x50, pLowCtx);
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x26af700ULL))(pLowCtx + 0x10);
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x26bba10ULL))(pLowCtx + 0xb518);
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d3300ULL))(pLowCtx + 0xb528);
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d2d20ULL))(pLowCtx + 0xb558);
    memset(pLowCtx + 0xb730, 0, 0x23a8u);
}

static inline bool GXDevice_DestroyPthreadMutexWithLog(void* pMutex, uint8_t* pInitFlag) {
    auto pfnPthreadMutexDestroy = reinterpret_cast<uint32_t (*)(void*)>(GX_VMA(0x2bbfee8ULL));
    uint32_t err = pfnPthreadMutexDestroy(pMutex);
    if (err == 0x80020010u) {
        return false; // EBUSY: original eboot.bin skips clearing flag (and skips freeing embedded heap backing)
    }
    if (err != 0u) {
        auto pfnLog = reinterpret_cast<void (*)(const char*, uint32_t, const char*, ...)>(GX_VMA(0x20b55b0ULL));
        const char* pFile = reinterpret_cast<const char*>(GX_VMA(0x493c20eULL));
        if (err == 0x80020016u) {
            pfnLog(pFile, 0x55u, reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
        }
        pfnLog(pFile, 0x57u, reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
    }
    *pInitFlag = 0u;
    return true;
}

static inline void GXDevice_DestroyEmbeddedHeapState(uint8_t* base) {
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x207b8c0ULL))(base + 0x18);
    if (base[0x8f] != 0u) {
        uintptr_t alignPad = (-reinterpret_cast<uintptr_t>(base + 0x28)) & 7ULL;
        uint8_t* pAligned  = base + 0x28 + alignPad;
        void* pTreeRoot    = *reinterpret_cast<void**>(pAligned + 0x58);
        if (pTreeRoot != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x2084ae0ULL))(pAligned, pTreeRoot);
        }
        uintptr_t tailOff = alignPad | 0x48ULL;
        *reinterpret_cast<uint64_t*>(pAligned + 0x08) = 0ULL;
        *reinterpret_cast<uint64_t*>(pAligned + 0x00) = 0ULL;
        *reinterpret_cast<uint64_t*>(base + 0x28 + tailOff + 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(base + 0x28 + tailOff + 0x08) = 0ULL;
        *reinterpret_cast<uint64_t*>(base + 0x28 + tailOff + 0x00) = 0ULL;
        base[0x8f] = 0u;
    }

    memset(base + 0x90, 0, 9u * sizeof(uint64_t)); // +0x90 .. +0xD0 inclusive

    uint8_t (**csVtbl)(void*) = *reinterpret_cast<uint8_t (***)(void*)>(base + 0xe8);
    bool canFreeBacking = true;
    if (csVtbl[2](base + 0xe8) != 0u) { // *0x10(%rax)
        canFreeBacking = GXDevice_DestroyPthreadMutexWithLog(base + 0xf0, base + 0xf8);
    }

    if (canFreeBacking) {
        void* pBackingAlloc = *reinterpret_cast<void**>(base + 0x00);
        if (pBackingAlloc != nullptr) {
            GXDevice_HeapFreeBlock(pBackingAlloc, *reinterpret_cast<void**>(base + 0x10));
            *reinterpret_cast<void**>(base + 0x00) = nullptr;
            *reinterpret_cast<void**>(base + 0x10) = nullptr;
        }
    }
}

} // namespace

/**
 * @brief Destroys and frees the global `GXDevice` singleton (`0x55406c8`) and all sub-resources.
 * VMA: 0x2196e10 (5,344 bytes)
 */
void GXDevice_DestroyAndShutdownSingleton() {
    auto pfnGetOwnerHeap = reinterpret_cast<void* (*)(void*)>(GX_VMA(0x207b720ULL));
    uint8_t** ppSingleton = reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDevice = *ppSingleton;
    if (pDevice == nullptr) {
        return;
    }

    // 1. Detach and release debug property root node (+0x690) and clear global debug menu mgr (0x56a6bc8)
    if (uint8_t* pRoot = *reinterpret_cast<uint8_t**>(pDevice + 0x690)) {
        void (**rootVtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pRoot);
        rootVtbl[3](pRoot); // *0x18(%rax)
        pRoot[0x0b] &= 0xBFu;
        pRoot = *reinterpret_cast<uint8_t**>(pDevice + 0x690);
        if (pRoot != nullptr) {
            GXRefObj_ReleaseStrongAndWeak(pRoot);
            *reinterpret_cast<void**>(pDevice + 0x690) = nullptr;
        }
    }
    *reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL)) = nullptr;

    // 2. Full subsystem shutdown if device was initialized (pDevice[0x3a8] != 0)
    if (pDevice[0x3a8] != 0u) {
        void** ppMainHeap = reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        void** ppDbgHeap  = reinterpret_cast<void**>(GX_VMA(0x5540690ULL));

        // 2.1 Invoke cleanup callbacks in m_callbackPairs (+0x618..+0x620)
        for (void** pPair = *reinterpret_cast<void***>(pDevice + 0x618);
             pPair != *reinterpret_cast<void***>(pDevice + 0x620);
             pPair += 2) {
            void* pUserData    = pPair[0];
            void* pCallbackObj = pPair[1];
            void (**vtbl)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pCallbackObj);
            vtbl[2](pCallbackObj, pUserData); // *0x10(%rax)
        }
        *reinterpret_cast<void**>(pDevice + 0x620) = *reinterpret_cast<void**>(pDevice + 0x618);

        // 2.2 Destroy +0x5f8 container
        if (uint8_t* p5f8 = *reinterpret_cast<uint8_t**>(pDevice + 0x5f8)) {
            void* pHeap = *ppMainHeap;
            if (void* pBuf = *reinterpret_cast<void**>(p5f8 + 0x08)) {
                GXDevice_HeapFreeBlock(*reinterpret_cast<void**>(p5f8 + 0x20), pBuf);
                memset(p5f8 + 0x08, 0, 24u);
            }
            GXDevice_HeapFreeBlock(pHeap, p5f8);
            *reinterpret_cast<void**>(pDevice + 0x5f8) = nullptr;
        }

        // 2.3 Destroy global query/task manager (0x56a6c28)
        void** ppGlobalQueryMgr = reinterpret_cast<void**>(GX_VMA(0x56a6c28ULL));
        if (void* pQMgr = *ppGlobalQueryMgr) {
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x21b6d20ULL))(pQMgr);
            pQMgr = *ppGlobalQueryMgr;
            if (pQMgr != nullptr) {
                void* pHeap = *ppMainHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x21b6d20ULL))(pQMgr);
                GXDevice_HeapFreeBlock(pHeap, pQMgr);
                *ppGlobalQueryMgr = nullptr;
            }
        }

        // 2.4 Destroy per-buffer dynamic ring cursors (+0x3a0, count = *(uint32_t*)(pDevice + 0x18))
        uint32_t ringCount = *reinterpret_cast<uint32_t*>(pDevice + 0x18);
        for (uint32_t i = 0; i < ringCount; ++i) {
            void** pCursors = *reinterpret_cast<void***>(pDevice + 0x3a0);
            if (void* pCur = pCursors[i]) {
                void* pHeap = *ppMainHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d8000ULL))(pCur);
                GXDevice_HeapFreeBlock(pHeap, pCur);
                (*reinterpret_cast<void***>(pDevice + 0x3a0))[i] = nullptr;
            }
        }
        if (void* pCursorsArr = *reinterpret_cast<void**>(pDevice + 0x3a0)) {
            GXDevice_HeapFreeBlock(*ppMainHeap, pCursorsArr);
            *reinterpret_cast<void**>(pDevice + 0x3a0) = nullptr;
        }

        // 2.5 Destroy dynamic buffer rings (+0x398 and +0x390)
        if (void* pRing1 = *reinterpret_cast<void**>(pDevice + 0x398)) {
            void* pHeap = pfnGetOwnerHeap(pRing1);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d7fa0ULL))(pRing1);
            GXDevice_HeapFreeBlock(pHeap, pRing1);
            *reinterpret_cast<void**>(pDevice + 0x398) = nullptr;
        }
        if (void* pRing0 = *reinterpret_cast<void**>(pDevice + 0x390)) {
            void* pHeap = pfnGetOwnerHeap(pRing0);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d7fa0ULL))(pRing0);
            GXDevice_HeapFreeBlock(pHeap, pRing0);
            *reinterpret_cast<void**>(pDevice + 0x390) = nullptr;
        }

        // 2.6 Destroy default shader pack (+0x388)
        if (void* pDefPack = *reinterpret_cast<void**>(pDevice + 0x388)) {
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x2191030ULL))(pDefPack);
            pDefPack = *reinterpret_cast<void**>(pDevice + 0x388);
            if (pDefPack != nullptr) {
                void* pHeap = *ppMainHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x2191030ULL))(pDefPack);
                GXDevice_HeapFreeBlock(pHeap, pDefPack);
                *reinterpret_cast<void**>(pDevice + 0x388) = nullptr;
            }
        }

        // 2.7 Destroy deferred release / async queue (+0x660)
        if (uint8_t* p660 = *reinterpret_cast<uint8_t**>(pDevice + 0x660)) {
            void* pHeap = *ppMainHeap;
            *reinterpret_cast<void**>(p660 + 0x28) = reinterpret_cast<uint8_t*>(GX_VMA(0x52b6220ULL)) + 0x10;
            if (reinterpret_cast<uint8_t (*)(void*)>(GX_VMA(0x207ee80ULL))(p660 + 0x28) != 0u) {
                GXDevice_DestroyPthreadMutexWithLog(p660 + 0x30, p660 + 0x38);
            }
            *reinterpret_cast<void**>(p660 + 0x28) = reinterpret_cast<void*>(GX_VMA(0x53b2fa0ULL));
            if (p660[0x38] != 0u) {
                GXDevice_DestroyPthreadMutexWithLog(p660 + 0x30, p660 + 0x38);
            }
            if (void* pBuf = *reinterpret_cast<void**>(p660 + 0x08)) {
                GXDevice_HeapFreeBlock(*reinterpret_cast<void**>(p660 + 0x20), pBuf);
                memset(p660 + 0x08, 0, 24u);
            }
            GXDevice_HeapFreeBlock(pHeap, p660);
            *reinterpret_cast<void**>(pDevice + 0x660) = nullptr;
        }

        // 2.8 Destroy GPU timestamp profiler (+0x640) and PerfDebugMenu (+0x638)
        if (void* pProfiler = *reinterpret_cast<void**>(pDevice + 0x640)) {
            void* pHeap = *ppDbgHeap;
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x21c12b0ULL))(pProfiler);
            GXDevice_HeapFreeBlock(pHeap, pProfiler);
            *reinterpret_cast<void**>(pDevice + 0x640) = nullptr;
        }
        if (uint8_t* pPerfMenu = *reinterpret_cast<uint8_t**>(pDevice + 0x638)) {
            if (void* pPerfSub = *reinterpret_cast<void**>(pPerfMenu)) {
                void* pSubHeap = pfnGetOwnerHeap(pPerfSub);
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d7fa0ULL))(pPerfSub);
                GXDevice_HeapFreeBlock(pSubHeap, pPerfSub);
                *reinterpret_cast<void**>(pPerfMenu) = nullptr;
                pPerfMenu = *reinterpret_cast<uint8_t**>(pDevice + 0x638);
            }
            if (pPerfMenu != nullptr) {
                void* pHeap = *ppDbgHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x217ac30ULL))(pPerfMenu);
                GXDevice_HeapFreeBlock(pHeap, pPerfMenu);
                *reinterpret_cast<void**>(pDevice + 0x638) = nullptr;
            }
        }

        // 2.9 Reset primary low-level immediate Gnm context (+0x08)
        if (uint8_t* pImmLowCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x08)) {
            GXDevice_ResetLowLevelDeferredGnmCtx(pImmLowCtx);
        }

        // 2.10 Destroy per-worker deferred contexts #2 (+0x68 / +0x60) and #1 (+0x58 / +0x50)
        auto destroyWorkerCtxPair = [&](size_t hiArrOff, size_t loArrOff) {
            uint32_t workerCtxCount = *reinterpret_cast<uint32_t*>(pDevice + 0x80);
            for (uint32_t i = 0; i < workerCtxCount; ++i) {
                uint8_t** pHiArr = *reinterpret_cast<uint8_t***>(pDevice + hiArrOff);
                if (uint8_t* pHi = pHiArr[i]) {
                    void* pHeap = pfnGetOwnerHeap(pHi);
                    void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pHi);
                    vtbl[0](pHi);
                    GXDevice_HeapFreeBlock(pHeap, pHi);
                    (*reinterpret_cast<void***>(pDevice + hiArrOff))[i] = nullptr;
                }
                uint8_t** pLoArr = *reinterpret_cast<uint8_t***>(pDevice + loArrOff);
                if (uint8_t* pLo = pLoArr[i]) {
                    GXDevice_ResetLowLevelDeferredGnmCtx(pLo);
                    pLo = (*reinterpret_cast<uint8_t***>(pDevice + loArrOff))[i];
                    if (pLo != nullptr) {
                        void* pHeap = pfnGetOwnerHeap(pLo);
                        reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d5810ULL))(pLo);
                        GXDevice_HeapFreeBlock(pHeap, pLo);
                        (*reinterpret_cast<void***>(pDevice + loArrOff))[i] = nullptr;
                    }
                }
            }
            if (void* pLoArr = *reinterpret_cast<void**>(pDevice + loArrOff)) {
                GXDevice_HeapFreeBlock(*ppMainHeap, pLoArr);
                *reinterpret_cast<void**>(pDevice + loArrOff) = nullptr;
            }
            if (void* pHiArr = *reinterpret_cast<void**>(pDevice + hiArrOff)) {
                GXDevice_HeapFreeBlock(*ppMainHeap, pHiArr);
                *reinterpret_cast<void**>(pDevice + hiArrOff) = nullptr;
            }
        };
        destroyWorkerCtxPair(0x68u, 0x60u);
        destroyWorkerCtxPair(0x58u, 0x50u);

        // 2.11 Destroy RenderThread sync object (+0x348) via vtable[1] (*0x08)
        if (uint8_t* pRenderThread = *reinterpret_cast<uint8_t**>(pDevice + 0x348)) {
            void* pHeap = *ppMainHeap;
            void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pRenderThread);
            vtbl[1](pRenderThread); // *0x08(%rax)
            GXDevice_HeapFreeBlock(pHeap, pRenderThread);
            *reinterpret_cast<void**>(pDevice + 0x348) = nullptr;
        }

        // 2.12 Destroy main deferred context (+0x78 / +0x70) and clear +0x20
        if (uint8_t* pMainDefHi = *reinterpret_cast<uint8_t**>(pDevice + 0x78)) {
            void* pHeap = pfnGetOwnerHeap(pMainDefHi);
            void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pMainDefHi);
            vtbl[0](pMainDefHi);
            GXDevice_HeapFreeBlock(pHeap, pMainDefHi);
            *reinterpret_cast<void**>(pDevice + 0x78) = nullptr;
        }
        if (uint8_t* pMainDefLo = *reinterpret_cast<uint8_t**>(pDevice + 0x70)) {
            GXDevice_ResetLowLevelDeferredGnmCtx(pMainDefLo);
            pMainDefLo = *reinterpret_cast<uint8_t**>(pDevice + 0x70);
            if (pMainDefLo != nullptr) {
                void* pHeap = pfnGetOwnerHeap(pMainDefLo);
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d5810ULL))(pMainDefLo);
                GXDevice_HeapFreeBlock(pHeap, pMainDefLo);
                *reinterpret_cast<void**>(pDevice + 0x70) = nullptr;
            }
        }
        *reinterpret_cast<void**>(pDevice + 0x20) = nullptr;

        // 2.13 Destroy double-buffered context pools (+0x350/+0x358), cmd queues (+0x360/+0x368), linear heaps (+0x370/+0x378)
        const size_t kPoolOffs[2] = { 0x350u, 0x358u };
        for (size_t off : kPoolOffs) {
            if (void* pPool = *reinterpret_cast<void**>(pDevice + off)) {
                void* pHeap = *ppMainHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x21b3eb0ULL))(pPool);
                GXDevice_HeapFreeBlock(pHeap, pPool);
                *reinterpret_cast<void**>(pDevice + off) = nullptr;
            }
        }
        const size_t kQueueOffs[2] = { 0x360u, 0x368u };
        for (size_t off : kQueueOffs) {
            if (void* pQueue = *reinterpret_cast<void**>(pDevice + off)) {
                void* pHeap = *ppMainHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x21b4770ULL))(pQueue);
                GXDevice_HeapFreeBlock(pHeap, pQueue);
                *reinterpret_cast<void**>(pDevice + off) = nullptr;
            }
        }
        const size_t kLinHeapOffs[2] = { 0x370u, 0x378u };
        for (size_t off : kLinHeapOffs) {
            if (uint8_t* pLinHeap = *reinterpret_cast<uint8_t**>(pDevice + off)) {
                void* pHeap = *ppMainHeap;
                void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pLinHeap);
                vtbl[0](pLinHeap);
                GXDevice_HeapFreeBlock(pHeap, pLinHeap);
                *reinterpret_cast<void**>(pDevice + off) = nullptr;
            }
        }

        // 2.14 Shutdown worker task pool (+0x278) and resource registries (+0x260, +0x258, +0x250, +0x268)
        reinterpret_cast<void (*)(void*)>(GX_VMA(0x21b84b0ULL))(pDevice + 0x278);

        auto destroyOwnedSubMgr = [&](size_t off, uintptr_t dtorVma) {
            if (void* pMgr = *reinterpret_cast<void**>(pDevice + off)) {
                void* pHeap = pfnGetOwnerHeap(pMgr);
                reinterpret_cast<void (*)(void*)>(GX_VMA(dtorVma))(pMgr);
                GXDevice_HeapFreeBlock(pHeap, pMgr);
                *reinterpret_cast<void**>(pDevice + off) = nullptr;
            }
        };
        destroyOwnedSubMgr(0x260u, 0xe31d70ULL);
        destroyOwnedSubMgr(0x258u, 0x219eb30ULL);
        destroyOwnedSubMgr(0x250u, 0x21b26a0ULL);
        destroyOwnedSubMgr(0x268u, 0xe45aa0ULL);

        // 2.15 Display outputs pass 1: release RTV/DSV/Tex views (+0x70, +0x68, +0x78) and target helper (+0x60)
        const size_t kOutViewOffs[3] = { 0x70u, 0x68u, 0x78u };
        for (uint8_t* pOut = *reinterpret_cast<uint8_t**>(pDevice + 0x90);
             pOut != *reinterpret_cast<uint8_t**>(pDevice + 0x98);
             pOut += 0xb0) {
            for (size_t off : kOutViewOffs) {
                if (uint8_t* pRes = *reinterpret_cast<uint8_t**>(pOut + off)) {
                    GXDevice_ReleaseSimpleRef8(pRes);
                    *reinterpret_cast<void**>(pOut + off) = nullptr;
                }
            }
            if (uint8_t* pHelper60 = *reinterpret_cast<uint8_t**>(pOut + 0x60)) {
                void* pHeap = pfnGetOwnerHeap(pHelper60);
                void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pHelper60);
                vtbl[0](pHelper60);
                GXDevice_HeapFreeBlock(pHeap, pHelper60);
                *reinterpret_cast<void**>(pOut + 0x60) = nullptr;
            }
        }

        // 2.16 Release default sampler (+0x248), texture registry (+0x240), 16 default textures (+0x238), +0x230, and immediate ctx (+0x10)
        if (uint8_t* pDefSamp = *reinterpret_cast<uint8_t**>(pDevice + 0x248)) {
            GXDevice_ReleaseSimpleRef8(pDefSamp);
            *reinterpret_cast<void**>(pDevice + 0x248) = nullptr;
        }
        if (uint8_t* pTexReg = *reinterpret_cast<uint8_t**>(pDevice + 0x240)) {
            void* pHeap = *ppMainHeap;
            void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pTexReg);
            vtbl[0](pTexReg);
            GXDevice_HeapFreeBlock(pHeap, pTexReg);
            *reinterpret_cast<void**>(pDevice + 0x240) = nullptr;
        }
        for (uint32_t i = 0; i < 16u; ++i) {
            uint8_t** ppDefTexArr = *reinterpret_cast<uint8_t***>(pDevice + 0x238);
            if (uint8_t* pTex = ppDefTexArr[i]) {
                GXDevice_ReleaseSimpleRef8(pTex);
                (*reinterpret_cast<void***>(pDevice + 0x238))[i] = nullptr;
            }
        }
        if (uint8_t* pDefTexArr = *reinterpret_cast<uint8_t**>(pDevice + 0x238)) {
            GXDevice_HeapFreeBlock(*ppMainHeap, *reinterpret_cast<void**>(pDefTexArr - 0x10));
            *reinterpret_cast<void**>(pDevice + 0x238) = nullptr;
        }
        if (uint8_t* pTex230 = *reinterpret_cast<uint8_t**>(pDevice + 0x230)) {
            GXDevice_ReleaseSimpleRef8(pTex230);
            *reinterpret_cast<void**>(pDevice + 0x230) = nullptr;
        }
        if (uint8_t* pImmHiCtx = *reinterpret_cast<uint8_t**>(pDevice + 0x10)) {
            void* pHeap = *ppMainHeap;
            void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pImmHiCtx);
            vtbl[0](pImmHiCtx);
            GXDevice_HeapFreeBlock(pHeap, pImmHiCtx);
            *reinterpret_cast<void**>(pDevice + 0x10) = nullptr;
        }

        // 2.17 Display outputs pass 2: release atomic-refcounted flip/surface handle (+0x58)
        for (uint8_t* pOut = *reinterpret_cast<uint8_t**>(pDevice + 0x90);
             pOut != *reinterpret_cast<uint8_t**>(pDevice + 0x98);
             pOut += 0xb0) {
            if (uint8_t* p58 = *reinterpret_cast<uint8_t**>(pOut + 0x58)) {
                if (__atomic_fetch_sub(reinterpret_cast<int32_t*>(p58 + 0x10), 1, __ATOMIC_SEQ_CST) == 1) {
                    void* pHeap = *reinterpret_cast<void**>(p58 + 0x08);
                    void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(p58);
                    vtbl[0](p58);
                    GXDevice_HeapFreeBlock(pHeap, p58);
                }
                *reinterpret_cast<void**>(pOut + 0x58) = nullptr;
            }
        }

        // 2.18 Destroy resource memory tracker (+0x650), gamma table (+0xb0), and +0x5e8 / +0x5e0
        if (void** ppTracker = *reinterpret_cast<void***>(pDevice + 0x650)) {
            void* pOuterHeap = pfnGetOwnerHeap(ppTracker);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x21abf90ULL))(*ppTracker);
            if (void* pInner = *ppTracker) {
                void* pHeap = *ppMainHeap;
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x21abe40ULL))(pInner);
                GXDevice_HeapFreeBlock(pHeap, pInner);
                *ppTracker = nullptr;
            }
            GXDevice_HeapFreeBlock(pOuterHeap, ppTracker);
            *reinterpret_cast<void**>(pDevice + 0x650) = nullptr;
        }
        if (void* pGamma = *reinterpret_cast<void**>(pDevice + 0xb0)) {
            GXDevice_HeapFreeBlock(*ppMainHeap, pGamma);
            *reinterpret_cast<void**>(pDevice + 0xb0) = nullptr;
        }
        const size_t kExtraOffs[2] = { 0x5e8u, 0x5e0u };
        for (size_t off : kExtraOffs) {
            if (uint8_t* pObj = *reinterpret_cast<uint8_t**>(pDevice + off)) {
                void* pHeap = *ppMainHeap;
                void (**vtbl)(void*) = *reinterpret_cast<void (***)(void*)>(pObj);
                vtbl[0](pObj);
                GXDevice_HeapFreeBlock(pHeap, pObj);
                *reinterpret_cast<void**>(pDevice + off) = nullptr;
            }
        }

        // 2.19 Tear down embedded heap states (+0x4d8 and +0x3d0)
        GXDevice_DestroyEmbeddedHeapState(pDevice + 0x4d8);
        GXDevice_DestroyEmbeddedHeapState(pDevice + 0x3d0);

        // 2.20 Display outputs pass 3: free display output (+0x08) and swap chain (+0x00)
        for (uint8_t* pOut = *reinterpret_cast<uint8_t**>(pDevice + 0x90);
             pOut != *reinterpret_cast<uint8_t**>(pDevice + 0x98);
             pOut += 0xb0) {
            if (void* pDispOut = *reinterpret_cast<void**>(pOut + 0x08)) {
                void* pHeap = pfnGetOwnerHeap(pDispOut);
                GXDevice_HeapFreeBlock(pHeap, pDispOut);
                *reinterpret_cast<void**>(pOut + 0x08) = nullptr;
            }
            if (void* pSwapChain = *reinterpret_cast<void**>(pOut + 0x00)) {
                void* pHeap = pfnGetOwnerHeap(pSwapChain);
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d5c80ULL))(pSwapChain);
                GXDevice_HeapFreeBlock(pHeap, pSwapChain);
                *reinterpret_cast<void**>(pOut + 0x00) = nullptr;
            }
        }

        // 2.21 Destroy low-level Gnm device (+0x00)
        if (uint8_t* pGnmDev = *reinterpret_cast<uint8_t**>(pDevice + 0x00)) {
            void* pHeap = pfnGetOwnerHeap(pGnmDev);
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x216b900ULL))(pGnmDev, pGnmDev + 0x130);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x21682c0ULL))(pGnmDev + 0x130);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d0bf0ULL))(pGnmDev + 0xb828);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x26d1b40ULL))(pGnmDev + 0xb688);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x26aec40ULL))(pGnmDev + 0x140);
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x26b10a0ULL))(pGnmDev + 0x40);
            GXDevice_HeapFreeBlock(pHeap, pGnmDev);
            *reinterpret_cast<void**>(pDevice + 0x00) = nullptr;
        }

        // 2.22 Free global 17-slot scratch table (0x56a6ca8 / 0x56a6cb0 / 0x56a6cb8)
        void** ppScratchAlloc = reinterpret_cast<void**>(GX_VMA(0x56a6ca8ULL));
        void*** pppScratchArr = reinterpret_cast<void***>(GX_VMA(0x56a6cb0ULL));
        uint8_t* pScratchFlag = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cb8ULL));
        if (*ppScratchAlloc != nullptr) {
            if (*pScratchFlag != 0u) {
                for (uint32_t i = 0; i < ((*pScratchFlag != 0u) ? 17u : 0u); ++i) {
                    void** pArr = *pppScratchArr;
                    if (void* pSlot = pArr[i]) {
                        GXDevice_HeapFreeBlock(*ppScratchAlloc, pSlot);
                        (*pppScratchArr)[i] = nullptr;
                    }
                }
            }
            if (void* pArr = *pppScratchArr) {
                GXDevice_HeapFreeBlock(*ppScratchAlloc, pArr);
                *pppScratchArr = nullptr;
            }
            *pScratchFlag = 0u;
            *ppScratchAlloc = nullptr;
        }

        pDevice[0x3a8] = 0u;
    }

    // 3. Invoke GXDevice_BaseDtor, free singleton memory, and clear global singleton pointer
    pDevice = *ppSingleton;
    if (pDevice != nullptr) {
        void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        GXDevice_BaseDtor(pDevice);
        GXDevice_HeapFreeBlock(pHeap, pDevice);
        *ppSingleton = nullptr;
    }
}

void GXDevice_Construct(uint8_t* pDevice);

namespace {

static inline uint8_t* GXDevice_HeapAllocAligned(void* pHeap, size_t size, size_t alignment) {
    void* (**vtbl)(void*, size_t, size_t) = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(pHeap);
    return static_cast<uint8_t*>(vtbl[11](pHeap, size, alignment)); // *0x58(%rax)
}

static inline uint8_t* GXDevice_HeapAllocUnaligned(void* pHeap, size_t size) {
    void* (**vtbl)(void*, size_t) = *reinterpret_cast<void* (***)(void*, size_t)>(pHeap);
    return static_cast<uint8_t*>(vtbl[10](pHeap, size)); // *0x50(%rax)
}

static inline void* GXDevice_EnsureDefaultContainerHeapChecked() {
    void** ppDefHeap = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* pHeap = *ppDefHeap;
    if (pHeap == nullptr) {
        pHeap = reinterpret_cast<void* (*)()>(GX_VMA(0x2ba2b10ULL))();
        *ppDefHeap = pHeap;
    }
    alignas(16) uint8_t capsBuf[0x40] = {};
    void (**vtbl)(void*, void*, uint32_t) = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(pHeap);
    vtbl[4](capsBuf, pHeap, 0u); // *0x20(%rax)
    if ((capsBuf[0] & 0x20u) == 0u) {
        auto pfnLog = reinterpret_cast<void (*)(const char*, uint32_t, const char*, ...)>(GX_VMA(0x20b55b0ULL));
        pfnLog(nullptr, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
    return pHeap;
}

static inline uint8_t* GXDevice_AllocAndConstructEmptyRTResource(void* pAlloc) {
    uint8_t* pRT = GXDevice_HeapAllocAligned(pAlloc, 0x80u, 0x10u);
    if (pRT != nullptr) {
        *reinterpret_cast<uint32_t*>(pRT + 0x08) = 0u;
        memcpy(pRT + 0x20, reinterpret_cast<const void*>(GX_VMA(0x4920760ULL)), 16u);
        *reinterpret_cast<uint64_t*>(pRT + 0x30) = 0ULL;
        *reinterpret_cast<uint32_t*>(pRT + 0x58) = 0u;
        *reinterpret_cast<uint64_t*>(pRT + 0x50) = 0ULL;
        *reinterpret_cast<uint64_t*>(pRT + 0x48) = 0ULL;
        *reinterpret_cast<uint64_t*>(pRT + 0x40) = 0ULL;
        *reinterpret_cast<uint32_t*>(pRT + 0x10) = 0x3f800000u;
        *reinterpret_cast<void**>(pRT + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6940ULL));
        *reinterpret_cast<uint64_t*>(pRT + 0x60) = 0ULL;
        *reinterpret_cast<uint64_t*>(pRT + 0x68) = ~0ULL;
        *reinterpret_cast<uint32_t*>(pRT + 0x70) = 0xFFFFFFFFu;
    }
    return pRT;
}

static inline uint8_t* GXDevice_AllocAndInitDeviceContext(
    void*    pMainHeap,
    void*    pLowGnmCtx,
    uint8_t  isImmediateFlag,
    uint32_t workerSlotIdx) {
    uint8_t* pCtx = GXDevice_HeapAllocAligned(pMainHeap, 0x20e8u, 8u);
    if (pCtx != nullptr) {
        *reinterpret_cast<void**>(pCtx + 0x00) = reinterpret_cast<uint8_t*>(GX_VMA(0x52e74d0ULL)) + 0x10;
        *reinterpret_cast<void**>(pCtx + 0x08) = pLowGnmCtx;
        memset(pCtx + 0x1728, 0, 0x794u);
        *reinterpret_cast<uint64_t*>(pCtx + 0x1ec8) = 0ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x1ec0) = 0ULL;
        pCtx[0x2093] = isImmediateFlag;
        pCtx[0x2094] = 0u;
        *reinterpret_cast<uint32_t*>(pCtx + 0x2098) = workerSlotIdx;
        pCtx[0x209c] = 0u;
        *reinterpret_cast<uint32_t*>(pCtx + 0x20a0) = 0u;

        void* pStrHeap = GXDevice_EnsureDefaultContainerHeapChecked();
        *reinterpret_cast<void**>(pCtx + 0x20d0) = pStrHeap;
        *reinterpret_cast<uint64_t*>(pCtx + 0x20c8) = 7ULL;
        *reinterpret_cast<uint64_t*>(pCtx + 0x20c0) = 0ULL;
        *reinterpret_cast<uint16_t*>(pCtx + 0x20b0) = 0u;
        pCtx[0x20d8] = 1u;
        *reinterpret_cast<uint32_t*>(pCtx + 0x20e0) = 0u;
    }
    return pCtx;
}

static inline void GXDevice_ResetDeviceContextStateShadow(uint8_t* pCtx) {
    *reinterpret_cast<void**>(pCtx + 0x1728) = *reinterpret_cast<void**>(pCtx + 0x08);
    *reinterpret_cast<void**>(pCtx + 0x1730) = pCtx;
    pCtx[0x2092] = 0u;
    *reinterpret_cast<uint16_t*>(pCtx + 0x2090) = 0u;
    memset(pCtx + 0x10, 0, 0x1718u);
    *reinterpret_cast<uint64_t*>(pCtx + 0x1ec8) = 0ULL;
    *reinterpret_cast<uint64_t*>(pCtx + 0x1ec0) = 0ULL;

    for (size_t stage = 0; stage < 6u; ++stage) {
        size_t stageBase = stage * 0x2e8u;
        *reinterpret_cast<uint32_t*>(pCtx + 0x778u + stageBase) = 0x10u;
        *reinterpret_cast<uint32_t*>(pCtx + 0x6f0u + stageBase) = 0x0Eu;
        *reinterpret_cast<uint32_t*>(pCtx + 0x678u + stageBase) = 0x3Bu;
    }
    *reinterpret_cast<uint32_t*>(pCtx + 0x1688) = 0x08u;
    *reinterpret_cast<uint32_t*>(pCtx + 0x1680) = 0x20u;

    reinterpret_cast<void (*)(void*, const void*, uint64_t)>(GX_VMA(0x262a720ULL))(
        pCtx + 0x20a8,
        reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)),
        0ULL);
    *reinterpret_cast<uint32_t*>(pCtx + 0x20e0) = 0u;
}

static inline void GXDevice_InitEmbeddedGarlicHeap(
    uint8_t*  base,
    uint64_t  byteSize,
    uint8_t** ppOutWrapperSlot,
    uintptr_t globalWrapperVma) {
    if (byteSize == 0ULL) {
        return;
    }

    void* pMetaHeap   = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* pGarlicHeap = *reinterpret_cast<void**>(GX_VMA(0x5540698ULL));
    if (pMetaHeap != nullptr && pGarlicHeap != nullptr) {
        uint8_t* pBacking = GXDevice_HeapAllocUnaligned(pGarlicHeap, static_cast<size_t>(byteSize));
        *reinterpret_cast<void**>(base + 0x10) = pBacking;
        if (pBacking != nullptr) {
            *reinterpret_cast<void**>(base + 0x00) = pGarlicHeap;
            bool csOk = true;
            uint8_t (**csVtbl)(void*) = *reinterpret_cast<uint8_t (***)(void*)>(base + 0xe8);
            if (csVtbl[2](base + 0xe8) == 0u) { // *0x10(%rax)
                if (reinterpret_cast<int32_t (*)(void*)>(GX_VMA(0x2083fe0ULL))(base + 0xf0) == 0) {
                    base[0xf8] = 1u;
                } else {
                    csOk = false;
                }
            }
            if (csOk &&
                reinterpret_cast<uint8_t (*)(void*, void*, void*, uint64_t)>(GX_VMA(0x2087e50ULL))(
                    base + 0x28, pMetaHeap, pBacking, byteSize) != 0u) {
                *reinterpret_cast<uint32_t*>(base + 0x100) = 0u;
                *reinterpret_cast<void**>(base + 0x20) = base;
                void* pMgr = reinterpret_cast<void* (*)()>(GX_VMA(0x207bbf0ULL))();
                reinterpret_cast<void (*)(void*, void*, void*, uint64_t)>(GX_VMA(0x207c000ULL))(
                    pMgr, base + 0x18, *reinterpret_cast<void**>(base + 0x10), byteSize);
            } else {
                GXDevice_HeapFreeBlock(pGarlicHeap, *reinterpret_cast<void**>(base + 0x10));
                *reinterpret_cast<void**>(base + 0x10) = nullptr;
            }
        }
    }

    uint8_t* pWrap = GXDevice_HeapAllocAligned(*reinterpret_cast<void**>(GX_VMA(0x5540680ULL)), 0x18u, 8u);
    if (pWrap != nullptr) {
        *reinterpret_cast<void**>(pWrap + 0x00) = reinterpret_cast<uint8_t*>(GX_VMA(0x535fac0ULL)) + 0x10;
        *reinterpret_cast<void**>(pWrap + 0x08) = base + 0x18;
        *reinterpret_cast<void**>(pWrap + 0x10) = *reinterpret_cast<void**>(GX_VMA(0x5540698ULL));
    }
    *ppOutWrapperSlot = pWrap;
    *reinterpret_cast<void**>(GX_VMA(globalWrapperVma)) = pWrap;
}

} // namespace

/**
 * @brief Allocates, constructs, and initializes the global `GXDevice` singleton (`0x55406c8`, `0x6a0` bytes).
 * VMA: 0x2194580 (10,378 bytes)
 */
bool GXDevice_CreateAndInitializeSingleton(uint8_t* pInitParams) {
    uint8_t** ppSingleton = reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pPrimaryAlloc = *reinterpret_cast<void**>(pInitParams + 0x00);
    uint8_t* pDevice = GXDevice_HeapAllocAligned(pPrimaryAlloc, 0x6a0u, 0x10u);
    if (pDevice == nullptr) {
        *ppSingleton = nullptr;
        return false;
    }

    GXDevice_Construct(pDevice);
    *ppSingleton = pDevice;

    // 1. Store global subsystem heap pointers (0x5540680 .. 0x55406a8)
    void* h00 = *reinterpret_cast<void**>(pInitParams + 0x00);
    void* h08 = *reinterpret_cast<void**>(pInitParams + 0x08);
    void* h10 = *reinterpret_cast<void**>(pInitParams + 0x10);
    void* h18 = *reinterpret_cast<void**>(pInitParams + 0x18);
    void* h20 = *reinterpret_cast<void**>(pInitParams + 0x20);
    void* h28 = *reinterpret_cast<void**>(pInitParams + 0x28);

    void** ppMainHeap = reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    *ppMainHeap = h00;
    *reinterpret_cast<void**>(GX_VMA(0x5540688ULL)) = (h08 != nullptr) ? h08 : h00;
    *reinterpret_cast<void**>(GX_VMA(0x5540690ULL)) = (h10 != nullptr) ? h10 : h00;
    *reinterpret_cast<void**>(GX_VMA(0x55406a0ULL)) = (h18 != nullptr) ? h18 : h00;
    *reinterpret_cast<void**>(GX_VMA(0x5540698ULL)) = h28;
    *reinterpret_cast<void**>(GX_VMA(0x55406a8ULL)) = h20;

    // 2. Initialize global 17-slot scratch table (0x56a6ca8 / 0x56a6cb0 / 0x56a6cb8)
    {
        void** ppDefContainerHeap = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
        void* pScratchHeap = *ppDefContainerHeap;
        if (pScratchHeap == nullptr) {
            pScratchHeap = reinterpret_cast<void* (*)()>(GX_VMA(0x2ba2b10ULL))();
            *ppDefContainerHeap = pScratchHeap;
        }
        void** ppScratchAlloc = reinterpret_cast<void**>(GX_VMA(0x56a6ca8ULL));
        void*** pppScratchArr = reinterpret_cast<void***>(GX_VMA(0x56a6cb0ULL));
        uint8_t* pScratchFlag = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cb8ULL));

        *ppScratchAlloc = pScratchHeap;
        *pScratchFlag   = 1u;
        *pppScratchArr  = reinterpret_cast<void**>(GXDevice_HeapAllocUnaligned(pScratchHeap, 0x88u));

        if (*pScratchFlag != 0u) {
            void* const* pVtblTable = reinterpret_cast<void* const*>(GX_VMA(0x535fcd0ULL));
            for (int32_t i = 0; i < ((*pScratchFlag != 0u) ? 17 : 0); ++i) {
                uint8_t* pSlot = GXDevice_HeapAllocAligned(*ppScratchAlloc, 0x18u, 8u);
                if (pSlot != nullptr) {
                    *reinterpret_cast<void**>(pSlot + 0x00) = pVtblTable[i];
                    *reinterpret_cast<uint64_t*>(pSlot + 0x10) = 0ULL;
                    *reinterpret_cast<uint64_t*>(pSlot + 0x08) = 0ULL;
                }
                (*pppScratchArr)[i] = pSlot;
            }
        }
    }

    // 3. Copy and sanitize pInitParams (0x170 bytes) into pDevice + 0xc0
    memcpy(pDevice + 0xc0, pInitParams, 0x170u);

    uint32_t alignedW = (*reinterpret_cast<uint32_t*>(pDevice + 0xf0) + 3u) & ~3u;
    uint32_t alignedH = (*reinterpret_cast<uint32_t*>(pDevice + 0xf4) + 3u) & ~3u;
    *reinterpret_cast<uint32_t*>(pDevice + 0xf0) = alignedW;
    *reinterpret_cast<uint32_t*>(pDevice + 0xf4) = alignedH;

    uint32_t topMsaa = *reinterpret_cast<uint32_t*>(pDevice + 0xf8);
    if (topMsaa == 0u || (pDevice[0x10b] != 0u && topMsaa != 1u)) {
        *reinterpret_cast<uint32_t*>(pDevice + 0xf8) = 1u;
    }

    if (*reinterpret_cast<uint32_t*>(pDevice + 0x1e0) == 0u) {
        *reinterpret_cast<uint32_t*>(pDevice + 0x1e0) = 1u;
        *reinterpret_cast<uint32_t*>(pDevice + 0x180) = alignedW;
        *reinterpret_cast<uint32_t*>(pDevice + 0x184) = alignedH;
        *reinterpret_cast<uint64_t*>(pDevice + 0x188) = *reinterpret_cast<uint64_t*>(pDevice + 0xf8);
        *reinterpret_cast<uint64_t*>(pDevice + 0x190) = *reinterpret_cast<uint64_t*>(pDevice + 0x110);
    }

    uint32_t displayCount = *reinterpret_cast<uint32_t*>(pDevice + 0x1e0);
    for (uint32_t m = 0; m < displayCount; ++m) {
        uint8_t* pMode = pDevice + 0x180 + static_cast<size_t>(m) * 24u;
        *reinterpret_cast<uint32_t*>(pMode + 0x00) = (*reinterpret_cast<uint32_t*>(pMode + 0x00) + 3u) & ~3u;
        *reinterpret_cast<uint32_t*>(pMode + 0x04) = (*reinterpret_cast<uint32_t*>(pMode + 0x04) + 3u) & ~3u;
        uint32_t modeMsaa = *reinterpret_cast<uint32_t*>(pMode + 0x08);
        if (modeMsaa == 0u || (pDevice[0x10b] != 0u && modeMsaa != 1u)) {
            *reinterpret_cast<uint32_t*>(pMode + 0x08) = 1u;
        }
    }

    // 4. Validate Gnm memory parameters and launch GXRenderThread (+0x348) to create GXGnmDevice (+0x00)
    alignas(32) uint8_t gnmInitCfg[0x68] = {};
    gnmInitCfg[0x04] = pDevice[0x170];
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x00) = 0u;
    *reinterpret_cast<void**>(gnmInitCfg + 0x08) = *reinterpret_cast<void**>(pInitParams + 0x00);

    void* pOnionAlloc = *reinterpret_cast<void**>(pInitParams + 0x20);
    if (pOnionAlloc == nullptr) {
        return false;
    }
    *reinterpret_cast<void**>(gnmInitCfg + 0x10) = pOnionAlloc;

    void* pGarlicAlloc = *reinterpret_cast<void**>(pInitParams + 0x28);
    if (pGarlicAlloc == nullptr) {
        return false;
    }
    *reinterpret_cast<void**>(gnmInitCfg + 0x18) = pGarlicAlloc;

    uint32_t sz74 = *reinterpret_cast<uint32_t*>(pInitParams + 0x74);
    uint32_t sz78 = *reinterpret_cast<uint32_t*>(pInitParams + 0x78);
    uint32_t sz84 = *reinterpret_cast<uint32_t*>(pInitParams + 0x84);
    uint32_t sz88 = *reinterpret_cast<uint32_t*>(pInitParams + 0x88);
    if (sz74 == 0u || sz78 == 0u || sz84 == 0u || sz88 == 0u) {
        return false;
    }
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x40) = sz74;
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x44) = sz78;
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x48) = *reinterpret_cast<uint32_t*>(pInitParams + 0x7c);
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x4c) = *reinterpret_cast<uint32_t*>(pInitParams + 0x80);
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x50) = sz84;
    *reinterpret_cast<uint32_t*>(gnmInitCfg + 0x54) = sz88;

    struct RenderThreadInitPacket {
        uint64_t reserved0;
        void*    pGnmCfg;
        void*    pHeap0;
        uint64_t reserved18;
    } rtInitPkt = { 0ULL, gnmInitCfg, h00, 0ULL };

    uint8_t* pRenderThread = GXDevice_HeapAllocAligned(*ppMainHeap, 0xa0u, 8u);
    void* pGnmDev = nullptr;
    if (pRenderThread != nullptr) {
        reinterpret_cast<void (*)(void*, uint64_t, void*, void*)>(GX_VMA(0x21b6620ULL))(
            pRenderThread,
            *reinterpret_cast<uint64_t*>(pDevice + 0x1e8),
            pDevice + 0xc0,
            &rtInitPkt);
        void (**waitVtbl)(void*, uint32_t) = *reinterpret_cast<void (***)(void*, uint32_t)>(pRenderThread + 0x68);
        waitVtbl[3](pRenderThread + 0x68, 0xFFFFFFFFu); // *0x18(%rax)
        pGnmDev = *reinterpret_cast<void**>(pRenderThread + 0x98);
    }
    *reinterpret_cast<void**>(pDevice + 0x348) = pRenderThread;
    *reinterpret_cast<void**>(pDevice + 0x00)  = pGnmDev;

    // 5. Create SwapChains and GXDisplayOutput entries (0xb0 bytes each in pDevice + 0x88)
    for (uint32_t outIdx = 0; outIdx < *reinterpret_cast<uint32_t*>(pDevice + 0x1e0); ++outIdx) {
        alignas(32) uint8_t outEntry[0xb0] = {};
        alignas(32) uint8_t swapDesc[0x48] = {};

        uint8_t* pMode = pDevice + 0x180 + static_cast<size_t>(outIdx) * 24u;
        *reinterpret_cast<uint64_t*>(pMode + 0x08) = 1ULL;

        uint32_t modeW = *reinterpret_cast<uint32_t*>(pMode + 0x00);
        uint32_t modeH = *reinterpret_cast<uint32_t*>(pMode + 0x04);
        uint64_t modeSampleQ = *reinterpret_cast<uint64_t*>(pMode + 0x08);
        uint64_t modeWinHdl  = *reinterpret_cast<uint64_t*>(pMode + 0x10);

        *reinterpret_cast<uint32_t*>(swapDesc + 0x00) = modeW;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x04) = modeH;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x08) = 0x3Cu;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x0c) = 1u;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x10) = 0x57u;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x14) = 0u;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x18) = 0u;
        memcpy(swapDesc + 0x1c, &modeSampleQ, sizeof(uint64_t));
        *reinterpret_cast<uint32_t*>(swapDesc + 0x24) = 3u;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x28) = 3u;
        *reinterpret_cast<uint64_t*>(swapDesc + 0x30) = modeWinHdl;
        swapDesc[0x38] = 1u;
        swapDesc[0x39] = 1u;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x3c) = 0u;
        *reinterpret_cast<uint32_t*>(swapDesc + 0x40) = 0u;

        void* pSwapChain = nullptr;
        reinterpret_cast<void (*)(void**, void*, const void*, void*, void*)>(GX_VMA(0x26d5a80ULL))(
            &pSwapChain,
            *reinterpret_cast<void**>(pDevice + 0x00),
            swapDesc,
            h00,
            *reinterpret_cast<void**>(pDevice + 0xe8));

        *reinterpret_cast<void**>(outEntry + 0x00) = pSwapChain;
        memcpy(outEntry + 0x10, swapDesc, 0x48u);

        uint8_t* pDispCtrl = GXDevice_HeapAllocAligned(*ppMainHeap, 0x28u, 8u);
        if (pDispCtrl != nullptr) {
            *reinterpret_cast<uint64_t*>(pDispCtrl + 0x00) = 0ULL;
            pDispCtrl[0x08] = 1u;
            *reinterpret_cast<uint32_t*>(pDispCtrl + 0x0c) = 0u;
            pDispCtrl[0x10] = 1u;
            *reinterpret_cast<uint64_t*>(pDispCtrl + 0x18) = 0ULL;
            *reinterpret_cast<uint32_t*>(pDispCtrl + 0x20) = 0u;
        }
        *reinterpret_cast<void**>(outEntry + 0x08) = pDispCtrl;
        *reinterpret_cast<void**>(pDispCtrl + 0x00) = pSwapChain;

        uint8_t fsStateBuf[0x20] = {};
        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x26d5d50ULL))(pSwapChain, fsStateBuf);
        *reinterpret_cast<uint32_t*>(pDispCtrl + 0x0c) = *reinterpret_cast<uint32_t*>(fsStateBuf + 0x08);
        pDispCtrl[0x10] = pDevice[0x105];

        *reinterpret_cast<uint32_t*>(outEntry + 0xa0) = outIdx;
        *reinterpret_cast<uint32_t*>(outEntry + 0xa4) = *reinterpret_cast<uint32_t*>(pMode + 0x00);
        *reinterpret_cast<uint32_t*>(outEntry + 0xa8) = *reinterpret_cast<uint32_t*>(pMode + 0x04);

        if (*reinterpret_cast<uint8_t**>(pDevice + 0x98) == *reinterpret_cast<uint8_t**>(pDevice + 0xa0)) {
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x219d7b0ULL))(pDevice + 0x88);
        }
        uint8_t* pDstOut = *reinterpret_cast<uint8_t**>(pDevice + 0x98);
        if (pDstOut != nullptr) {
            memcpy(pDstOut, outEntry, 0xb0u);
        }
        *reinterpret_cast<uint8_t**>(pDevice + 0x98) = pDstOut + 0xb0;

        uint8_t vsync = pDevice[0x104];
        for (uint8_t* pCur = *reinterpret_cast<uint8_t**>(pDevice + 0x90);
             pCur != *reinterpret_cast<uint8_t**>(pDevice + 0x98);
             pCur += 0xb0) {
            (*reinterpret_cast<uint8_t**>(pCur + 0x08))[0x08] = vsync;
        }
        *reinterpret_cast<uint8_t*>(GX_VMA(0x513ac84ULL)) = vsync;
    }

    // 6. Initialize gamma ramp table (+0xb0, 1025 RGB float entries) and resource memory tracker (+0x650)
    {
        uint8_t* pGamma = GXDevice_HeapAllocAligned(*ppMainHeap, 0x3020u, 8u);
        if (pGamma != nullptr) {
            *reinterpret_cast<void**>(pGamma + 0x00) = reinterpret_cast<void*>(GX_VMA(0x535f7e0ULL));
            *reinterpret_cast<void**>(pGamma + 0x08) = pDevice + 0x88;
            pGamma[0x301c] = 0u;
            float inv1024 = *reinterpret_cast<const float*>(GX_VMA(0x49295d4ULL));
            for (uint32_t k = 0; k < 0x401u; ++k) {
                float val = static_cast<float>(static_cast<int32_t>(k)) * inv1024;
                float* pRgb = reinterpret_cast<float*>(pGamma + 0x10 + static_cast<size_t>(k) * 12u);
                pRgb[0] = val;
                pRgb[1] = val;
                pRgb[2] = val;
            }
        }
        *reinterpret_cast<void**>(pDevice + 0xb0) = pGamma;
    }

    {
        void** ppTracker = reinterpret_cast<void**>(GXDevice_HeapAllocAligned(*ppMainHeap, 8u, 8u));
        if (ppTracker != nullptr) {
            uint8_t* pInner = GXDevice_HeapAllocAligned(*ppMainHeap, 0x188u, 8u);
            if (pInner != nullptr) {
                reinterpret_cast<void (*)(void*)>(GX_VMA(0x21abac0ULL))(pInner);
            }
            *ppTracker = pInner;
            *reinterpret_cast<void***>(pDevice + 0x650) = ppTracker;
        } else {
            *reinterpret_cast<void**>(pDevice + 0x650) = nullptr;
        }
    }
    *reinterpret_cast<uint32_t*>(pDevice + 0x5f0) = 0u;

    // 7. Create per-output backbuffer RTV (+0x68), default DSV (+0x70), optional DepthR32F (+0x78), and viewport (+0x88)
    void* pCfgAlloc = *reinterpret_cast<void**>(pDevice + 0xc0);
    for (uint8_t* pOut = *reinterpret_cast<uint8_t**>(pDevice + 0x90);
         pOut != *reinterpret_cast<uint8_t**>(pDevice + 0x98);
         pOut += 0xb0) {
        uint32_t modeIdx = *reinterpret_cast<uint32_t*>(pOut + 0xa0);
        uint8_t* pMode   = pDevice + 0x180 + static_cast<size_t>(modeIdx) * 24u;
        uint32_t rtW     = *reinterpret_cast<uint32_t*>(pMode + 0x00);
        uint32_t modeH   = *reinterpret_cast<uint32_t*>(pMode + 0x04);
        uint32_t rtH     = modeH;

        *reinterpret_cast<uint32_t*>(pOut + 0x80) = rtW;
        *reinterpret_cast<uint32_t*>(pOut + 0x84) = modeH;

        if (pDevice[0x105] == 0u && pDevice[0x106] != 0u) {
            uint32_t outW = *reinterpret_cast<uint32_t*>(pOut + 0xa4);
            uint32_t outH = *reinterpret_cast<uint32_t*>(pOut + 0xa8);
            if (rtW != outW || modeH != outH) {
                float modeAspect = static_cast<float>(static_cast<uint64_t>(rtW)) / static_cast<float>(static_cast<uint64_t>(modeH));
                float outAspect  = static_cast<float>(static_cast<uint64_t>(outW)) / static_cast<float>(static_cast<uint64_t>(outH));
                if (outAspect > modeAspect) {
                    *reinterpret_cast<uint32_t*>(pOut + 0x80) = rtW;
                    float roundBias = *reinterpret_cast<const float*>(GX_VMA(0x49295d8ULL));
                    float scaledH   = floorf(static_cast<float>(static_cast<uint64_t>(rtW)) / outAspect + roundBias);
                    rtH = static_cast<uint32_t>(static_cast<int64_t>(scaledH));
                    *reinterpret_cast<uint32_t*>(pOut + 0x84) = rtH;
                }
            }
        }

        *reinterpret_cast<float*>(pOut + 0x88) = 0.0f;
        *reinterpret_cast<float*>(pOut + 0x8c) = static_cast<float>(static_cast<int32_t>((modeH - rtH) >> 1));
        *reinterpret_cast<float*>(pOut + 0x90) = static_cast<float>(static_cast<uint64_t>(rtW));
        *reinterpret_cast<float*>(pOut + 0x94) = static_cast<float>(static_cast<uint64_t>(rtH));
        *reinterpret_cast<uint64_t*>(pOut + 0x98) = 0x3f80000000000000ULL;

        reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x26d5d00ULL))(
            *reinterpret_cast<void**>(pOut + 0x00),
            pOut + 0x58);

        if (uint8_t* pSurf58 = *reinterpret_cast<uint8_t**>(pOut + 0x58)) {
            if (*reinterpret_cast<void**>(pOut + 0x60) == nullptr) {
                uint32_t viewDesc[5] = {
                    pSurf58[0x4b],
                    (*reinterpret_cast<uint16_t*>(pSurf58 + 0x4c) > 1u) ? 6u : 4u,
                    0u,
                    0u,
                    1u
                };
                reinterpret_cast<void (*)(void*, void*, void*, const void*, void*)>(GX_VMA(0x21667a0ULL))(
                    *reinterpret_cast<void**>(pDevice + 0x00),
                    pOut + 0x60,
                    pSurf58,
                    viewDesc,
                    pCfgAlloc);
            }
        }

        if (*reinterpret_cast<void**>(pOut + 0x70) == nullptr) {
            uint8_t* pDS = GXDevice_HeapAllocAligned(pCfgAlloc, 0x90u, 0x10u);
            if (pDS != nullptr) {
                *reinterpret_cast<uint32_t*>(pDS + 0x08) = 0u;
                memcpy(pDS + 0x20, reinterpret_cast<const void*>(GX_VMA(0x4920760ULL)), 16u);
                *reinterpret_cast<uint64_t*>(pDS + 0x30) = 0ULL;
                *reinterpret_cast<uint32_t*>(pDS + 0x58) = 0u;
                *reinterpret_cast<uint64_t*>(pDS + 0x50) = 0ULL;
                *reinterpret_cast<uint64_t*>(pDS + 0x48) = 0ULL;
                *reinterpret_cast<uint64_t*>(pDS + 0x40) = 0ULL;
                *reinterpret_cast<uint32_t*>(pDS + 0x10) = 0x3f800000u;
                *reinterpret_cast<void**>(pDS + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6860ULL));
                *reinterpret_cast<uint64_t*>(pDS + 0x70) = 0ULL;
                *reinterpret_cast<uint64_t*>(pDS + 0x68) = 0ULL;
                *reinterpret_cast<uint64_t*>(pDS + 0x60) = 0ULL;
                *reinterpret_cast<uint64_t*>(pDS + 0x78) = ~0ULL;
                *reinterpret_cast<uint32_t*>(pDS + 0x80) = 0xFFFFFFFFu;
            }
            *reinterpret_cast<void**>(pOut + 0x70) = pDS;

            struct DepthStencilCreateDesc {
                uint32_t width;
                uint32_t height;
                uint32_t format;
                uint32_t mips;
                uint64_t sampleDesc;
                uint32_t bindFlags;
                uint32_t miscFlags;
                uint64_t extra0;
            } dsDesc = {
                *reinterpret_cast<uint32_t*>(pOut + 0x80),
                *reinterpret_cast<uint32_t*>(pOut + 0x84),
                0x14u,
                1u,
                *reinterpret_cast<uint64_t*>(pOut + 0x2c),
                0x100u,
                0u,
                0ULL
            };
            reinterpret_cast<void (*)(void*, const void*, uint32_t, uint32_t)>(GX_VMA(0x219f130ULL))(pDS, &dsDesc, 0u, 0u);
            pDS = *reinterpret_cast<uint8_t**>(pOut + 0x70);
            ++(*reinterpret_cast<uint32_t*>(pDS + 0x08));
            (*reinterpret_cast<void (***)(void*, const void*)>(pDS))[16](pDS, reinterpret_cast<const void*>(GX_VMA(0x49c2e02ULL))); // *0x80
        }

        if (*reinterpret_cast<void**>(pOut + 0x68) == nullptr) {
            uint8_t* pRT = GXDevice_AllocAndConstructEmptyRTResource(pCfgAlloc);
            *reinterpret_cast<void**>(pOut + 0x68) = pRT;
            (*reinterpret_cast<void (***)(void*, void*, uint32_t)>(pRT))[19](
                pRT,
                *reinterpret_cast<void**>(pOut + 0x58),
                0u); // *0x98
            pRT = *reinterpret_cast<uint8_t**>(pOut + 0x68);
            ++(*reinterpret_cast<uint32_t*>(pRT + 0x08));
            (*reinterpret_cast<void (***)(void*, const void*)>(pRT))[16](pRT, reinterpret_cast<const void*>(GX_VMA(0x49c2e22ULL))); // *0x80
        }

        if (pDevice[0x10b] != 0u && *reinterpret_cast<void**>(pOut + 0x78) == nullptr) {
            uint8_t* pDepthR32F = GXDevice_AllocAndConstructEmptyRTResource(pCfgAlloc);
            *reinterpret_cast<void**>(pOut + 0x78) = pDepthR32F;
            reinterpret_cast<void (*)(void*, uint32_t, uint32_t, uint32_t, uint32_t, const void*, uint32_t, uint64_t, uint64_t)>(
                GX_VMA(0x21a32f0ULL))(
                pDepthR32F,
                *reinterpret_cast<uint32_t*>(pOut + 0x80),
                *reinterpret_cast<uint32_t*>(pOut + 0x84),
                0x1Eu,
                1u,
                pOut + 0x2c,
                0u,
                0ULL,
                0ULL);
            pDepthR32F = *reinterpret_cast<uint8_t**>(pOut + 0x78);
            ++(*reinterpret_cast<uint32_t*>(pDepthR32F + 0x08));
        }
    }

    // 8. Create immediate GXDeviceContext (+0x08 / +0x10) and 2 x 6 per-worker deferred contexts (+0x50/+0x58, +0x60/+0x68)
    *reinterpret_cast<void**>(pDevice + 0x08) = *reinterpret_cast<uint8_t**>(pDevice + 0x00) + 0x130;
    {
        uint8_t* pImmCtx = GXDevice_AllocAndInitDeviceContext(*ppMainHeap, *reinterpret_cast<void**>(pDevice + 0x08), 1u, 0u);
        *reinterpret_cast<void**>(pDevice + 0x10) = pImmCtx;
        GXDevice_ResetDeviceContextStateShadow(pImmCtx);
    }

    *reinterpret_cast<uint32_t*>(pDevice + 0x18) = 0x48u;
    *reinterpret_cast<uint32_t*>(pDevice + 0x80) = 6u;

    auto pfnCreateLowDeferredCtx = reinterpret_cast<void (*)(void*, void**, uint32_t, uint32_t)>(GX_VMA(0x21676c0ULL));

    *reinterpret_cast<void**>(pDevice + 0x50) = GXDevice_HeapAllocUnaligned(*ppMainHeap, 0x30u);
    *reinterpret_cast<void**>(pDevice + 0x58) = GXDevice_HeapAllocUnaligned(*ppMainHeap, 0x30u);
    if (*reinterpret_cast<void**>(pDevice + 0x58) != nullptr) {
        for (uint32_t i = 0; i < 6u; ++i) {
            void** ppLoArr = *reinterpret_cast<void***>(pDevice + 0x50);
            pfnCreateLowDeferredCtx(*reinterpret_cast<void**>(pDevice + 0x00), &ppLoArr[i], 0u, 0u);
            uint8_t* pDefCtx = GXDevice_AllocAndInitDeviceContext(*ppMainHeap, (*reinterpret_cast<void***>(pDevice + 0x50))[i], 0u, i);
            (*reinterpret_cast<uint8_t***>(pDevice + 0x58))[i] = pDefCtx;
            pDefCtx = (*reinterpret_cast<uint8_t***>(pDevice + 0x58))[i];
            GXDevice_ResetDeviceContextStateShadow(pDefCtx);
            pDefCtx = (*reinterpret_cast<uint8_t***>(pDevice + 0x58))[i];
            *reinterpret_cast<uint32_t*>(pDefCtx + 0x20a0) = i;
        }
    }

    *reinterpret_cast<void**>(pDevice + 0x60) = GXDevice_HeapAllocUnaligned(*ppMainHeap, 0x30u);
    *reinterpret_cast<void**>(pDevice + 0x68) = GXDevice_HeapAllocUnaligned(*ppMainHeap, 0x30u);
    if (*reinterpret_cast<void**>(pDevice + 0x68) != nullptr) {
        const void* pResUpdTitle = reinterpret_cast<const void*>(GX_VMA(0x49c2da0ULL));
        auto pfnWcsLen = reinterpret_cast<uint64_t (*)(const void*)>(GX_VMA(0x2bbf228ULL));
        auto pfnAssignWStr = reinterpret_cast<void (*)(void*, const void*, uint64_t)>(GX_VMA(0x262a720ULL));
        for (uint32_t i = 0; i < 6u; ++i) {
            void** ppLoArr = *reinterpret_cast<void***>(pDevice + 0x60);
            pfnCreateLowDeferredCtx(*reinterpret_cast<void**>(pDevice + 0x00), &ppLoArr[i], 0u, 0u);
            uint8_t* pDefCtx = GXDevice_AllocAndInitDeviceContext(*ppMainHeap, (*reinterpret_cast<void***>(pDevice + 0x60))[i], 0u, i);
            (*reinterpret_cast<uint8_t***>(pDevice + 0x68))[i] = pDefCtx;
            pDefCtx = (*reinterpret_cast<uint8_t***>(pDevice + 0x68))[i];
            GXDevice_ResetDeviceContextStateShadow(pDefCtx);
            pDefCtx = (*reinterpret_cast<uint8_t***>(pDevice + 0x68))[i];
            *reinterpret_cast<uint32_t*>(pDefCtx + 0x20a0) = i;
            *reinterpret_cast<uint32_t*>(pDefCtx + 0x20e0) = 1u;
            uint64_t titleLen = pfnWcsLen(pResUpdTitle);
            pfnAssignWStr(pDefCtx + 0x20a8, pResUpdTitle, titleLen);
        }
    }

    // 9. Create double-buffered context pools (+0x350/+0x358), linear heaps (+0x370/+0x378), cmd queues (+0x360/+0x368), and main deferred ctx (+0x70/+0x78)
    uint32_t queueCap = *reinterpret_cast<uint32_t*>(pDevice + 0x164);
    if (queueCap == 0u) {
        queueCap = 0x6Cu;
    }

    const size_t kCtxPoolSlots[2] = { 0x350u, 0x358u };
    for (size_t off : kCtxPoolSlots) {
        uint8_t* pPool = GXDevice_HeapAllocAligned(*ppMainHeap, 0x48u, 8u);
        if (pPool != nullptr) {
            reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x21b3910ULL))(pPool, queueCap);
        }
        *reinterpret_cast<void**>(pDevice + off) = pPool;
    }

    const size_t kLinHeapSlots[2] = { 0x370u, 0x378u };
    for (size_t off : kLinHeapSlots) {
        uint8_t* pLinHeap = GXDevice_HeapAllocAligned(*ppMainHeap, 0x50u, 8u);
        if (pLinHeap != nullptr) {
            reinterpret_cast<void (*)(void*)>(GX_VMA(0xe45050ULL))(pLinHeap);
        }
        *reinterpret_cast<void**>(pDevice + off) = pLinHeap;
        reinterpret_cast<void (*)(void*, void*, uint32_t)>(GX_VMA(0xe45200ULL))(pLinHeap, *ppMainHeap, 0x40000u);
    }

    for (size_t idx = 0; idx < 2u; ++idx) {
        uint8_t* pQueue = GXDevice_HeapAllocAligned(*ppMainHeap, 0xe0u, 8u);
        if (pQueue != nullptr) {
            reinterpret_cast<void (*)(void*, uint32_t, void*)>(GX_VMA(0x21b43c0ULL))(
                pQueue,
                queueCap,
                *reinterpret_cast<void**>(pDevice + 0x370 + idx * 8u));
        }
        *reinterpret_cast<void**>(pDevice + 0x360 + idx * 8u) = pQueue;
    }

    pfnCreateLowDeferredCtx(*reinterpret_cast<void**>(pDevice + 0x00), reinterpret_cast<void**>(pDevice + 0x70), 0u, 0u);
    {
        uint8_t* pMainDefCtx = GXDevice_AllocAndInitDeviceContext(*ppMainHeap, *reinterpret_cast<void**>(pDevice + 0x70), 0u, 0u);
        *reinterpret_cast<void**>(pDevice + 0x78) = pMainDefCtx;
        GXDevice_ResetDeviceContextStateShadow(pMainDefCtx);
    }

    // 10. Initialize worker thread pool (+0x278), global task pool (0x21b7050), CPU affinity tables, and frame counters (+0x668..+0x68c)
    {
        struct WorkerPoolInitDesc {
            uint32_t threadCount;
            uint32_t stackSize;
            uint64_t userContext;
        } poolDesc = { 6u, 0x20000u, *reinterpret_cast<uint64_t*>(pDevice + 0x178) };
        reinterpret_cast<void (*)(void*, const void*, const void*)>(GX_VMA(0x21b8140ULL))(
            pDevice + 0x278,
            &poolDesc,
            pDevice + 0x1f0);
    }
    *reinterpret_cast<uint32_t*>(pDevice + 0x2f0) = 6u;
    reinterpret_cast<void (*)(uint32_t, uint32_t)>(GX_VMA(0x21b7050ULL))(6u, 0x64u);

    {
        uint8_t*  pWorkerCoreBools = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6bd8ULL));
        uint32_t* pWorkerCoreMasks = reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bf8ULL));
        for (size_t w = 0; w < 5u; ++w) {
            uint32_t mask = static_cast<uint32_t>(*reinterpret_cast<uint64_t*>(pDevice + 0x1f0 + w * 8u));
            for (size_t c = 0; c < 6u; ++c) {
                pWorkerCoreBools[w * 6u + c] = static_cast<uint8_t>((mask >> c) & 1u);
            }
            pWorkerCoreMasks[w] = mask & 0x3Fu;
        }

        uint32_t rtMask = static_cast<uint32_t>(*reinterpret_cast<uint64_t*>(pDevice + 0x1e8));
        uint8_t* pRtCoreBools = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c0cULL));
        for (size_t c = 0; c < 6u; ++c) {
            pRtCoreBools[c] = static_cast<uint8_t>((rtMask >> c) & 1u);
        }
        *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c14ULL)) = rtMask & 0x3Fu;
    }

    *reinterpret_cast<uint64_t*>(pDevice + 0x668) = 0ULL;
    *reinterpret_cast<uint32_t*>(pDevice + 0x670) = 0u;
    *reinterpret_cast<uint64_t*>(pDevice + 0x678) = 0ULL;
    *reinterpret_cast<uint32_t*>(pDevice + 0x680) = 0u;
    *reinterpret_cast<uint32_t*>(pDevice + 0x688) = 0u;
    *reinterpret_cast<uint32_t*>(pDevice + 0x68c) = 1u;

    // 11. Initialize embedded CBHeap (+0x3d0 / +0x5e0) and ILHeap (+0x4d8 / +0x5e8)
    GXDevice_InitEmbeddedGarlicHeap(
        pDevice + 0x3d0,
        *reinterpret_cast<uint32_t*>(pDevice + 0x150),
        reinterpret_cast<uint8_t**>(pDevice + 0x5e0),
        0x55406b8ULL);
    GXDevice_InitEmbeddedGarlicHeap(
        pDevice + 0x4d8,
        *reinterpret_cast<uint32_t*>(pDevice + 0x154),
        reinterpret_cast<uint8_t**>(pDevice + 0x5e8),
        0x55406b0ULL);

    // 12. Initialize state presets (+0x250), thread-local pool (+0x258), +0x260, +0x268, 3D noise (+0x230), 16 fallback textures (+0x238), named map (+0x240), and default sampler (+0x248)
    {
        uint8_t* p250 = GXDevice_HeapAllocAligned(*ppMainHeap, 0x778u, 8u);
        *reinterpret_cast<void**>(pDevice + 0x250) = p250;
        reinterpret_cast<void (*)(void*)>(GX_VMA(0x21aeca0ULL))(p250);

        uint8_t* p258 = GXDevice_HeapAllocAligned(*ppMainHeap, 0x30u, 8u);
        if (p258 != nullptr) {
            memset(p258, 0, 0x2cu);
        }
        *reinterpret_cast<void**>(pDevice + 0x258) = p258;
        reinterpret_cast<void (*)(void*)>(GX_VMA(0x219e900ULL))(p258);

        uint8_t* p260 = GXDevice_HeapAllocAligned(*ppMainHeap, 0x48u, 8u);
        if (p260 != nullptr) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe31c60ULL))(p260, *ppMainHeap);
        }
        *reinterpret_cast<void**>(pDevice + 0x260) = p260;

        uint8_t* p268 = GXDevice_HeapAllocAligned(*ppMainHeap, 0x4b8u, 8u);
        if (p268 != nullptr) {
            reinterpret_cast<void (*)(void*, void*, uint32_t)>(GX_VMA(0xe45740ULL))(
                p268,
                *ppMainHeap,
                *reinterpret_cast<uint32_t*>(pDevice + 0x168));
        }
        *reinterpret_cast<void**>(pDevice + 0x268) = p268;
    }

    reinterpret_cast<void (*)(void*, void*, uint32_t, uint32_t)>(GX_VMA(0x21bac20ULL))(
        pDevice + 0x230,
        nullptr,
        0u,
        0u);

    {
        uint8_t* pRawTexArr = GXDevice_HeapAllocAligned(*ppMainHeap, 0x90u, 0x10u);
        if (pRawTexArr != nullptr) {
            memset(pRawTexArr + 0x10, 0, 0x80u);
            *reinterpret_cast<uint64_t*>(pRawTexArr + 0x08) = 0x10ULL;
            *reinterpret_cast<void**>(pRawTexArr + 0x00) = pRawTexArr;
            *reinterpret_cast<void**>(pDevice + 0x238) = pRawTexArr + 0x10;
        } else {
            *reinterpret_cast<void**>(pDevice + 0x238) = nullptr;
        }

        const uint32_t kDefaultTexColors[16] = {
            0xFFFFFFFFu, 0xFF000000u, 0xFF808080u, 0x800A0A0Au,
            0x01000000u, 0xFF757575u, 0xFF8080FFu, 0x00000000u,
            0xFFFF0000u, 0xFFFFFF00u, 0xFFFF00FFu, 0xFF00FF00u,
            0xFF0000FFu, 0xFF00FFFFu, 0xFF000000u, 0xFF808080u
        };

        struct TexSubResData {
            const void* pSysMem;
            uint32_t    sysMemPitch;
            uint32_t    sysMemSlicePitch;
        };

        for (uint32_t idx = 0; idx < 16u; ++idx) {
            uint8_t* pTex = nullptr;
            if (idx <= 13u) {
                pTex = GXDevice_HeapAllocAligned(*ppMainHeap, 0x60u, 0x10u);
                if (pTex != nullptr) {
                    *reinterpret_cast<uint32_t*>(pTex + 0x08) = 0u;
                    memcpy(pTex + 0x20, reinterpret_cast<const void*>(GX_VMA(0x4920760ULL)), 16u);
                    *reinterpret_cast<uint64_t*>(pTex + 0x30) = 0ULL;
                    *reinterpret_cast<void**>(pTex + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6a20ULL));
                    *reinterpret_cast<uint32_t*>(pTex + 0x58) = 0u;
                    *reinterpret_cast<uint64_t*>(pTex + 0x50) = 0ULL;
                    *reinterpret_cast<uint64_t*>(pTex + 0x48) = 0ULL;
                    *reinterpret_cast<uint64_t*>(pTex + 0x40) = 0ULL;
                    *reinterpret_cast<uint32_t*>(pTex + 0x10) = 0x3f800000u;
                }
                TexSubResData subRes = { &kDefaultTexColors[idx], 4u, 0u };
                uint32_t fmt = (idx == 5u) ? 0x5Bu : 0x57u;
                (*reinterpret_cast<void (***)(
                    void*, uint32_t, uint32_t, uint32_t, uint32_t, const void*,
                    uint32_t, uint32_t, uint64_t, uint64_t, uint32_t)>(pTex))[20](
                    pTex, 1u, 1u, fmt, 1u, &subRes, 0u, 0u, 0ULL, 0ULL, 0u); // *0xa0
            } else {
                pTex = GXDevice_HeapAllocAligned(*ppMainHeap, 0x60u, 0x10u);
                if (pTex != nullptr) {
                    *reinterpret_cast<uint32_t*>(pTex + 0x08) = 0u;
                    *reinterpret_cast<uint32_t*>(pTex + 0x10) = 0x3f800000u;
                    memcpy(pTex + 0x20, reinterpret_cast<const void*>(GX_VMA(0x4920760ULL)), 16u);
                    *reinterpret_cast<uint64_t*>(pTex + 0x30) = 0ULL;
                    *reinterpret_cast<void**>(pTex + 0x00) = reinterpret_cast<void*>(GX_VMA(0x53b6ba0ULL));
                    *reinterpret_cast<uint32_t*>(pTex + 0x58) = 0u;
                    *reinterpret_cast<uint64_t*>(pTex + 0x50) = 0ULL;
                    *reinterpret_cast<uint64_t*>(pTex + 0x48) = 0ULL;
                    *reinterpret_cast<uint64_t*>(pTex + 0x40) = 0ULL;
                }
                TexSubResData cubeFaces[6];
                for (size_t f = 0; f < 6u; ++f) {
                    cubeFaces[f] = { &kDefaultTexColors[idx], 4u, 0u };
                }
                reinterpret_cast<void (*)(void*, uint32_t, uint32_t, uint32_t, const void*, uint32_t, uint64_t)>(
                    GX_VMA(0x21a7f20ULL))(pTex, 1u, 0x57u, 1u, cubeFaces, 0u, 0ULL);
            }
            (*reinterpret_cast<uint8_t***>(pDevice + 0x238))[idx] = pTex;
            uint8_t* pStored = (*reinterpret_cast<uint8_t***>(pDevice + 0x238))[idx];
            ++(*reinterpret_cast<uint32_t*>(pStored + 0x08));
        }
    }

    {
        uint8_t* pNamedMap = GXDevice_HeapAllocAligned(*ppMainHeap, 0x50u, 8u);
        if (pNamedMap != nullptr) {
            reinterpret_cast<void (*)(void*, void*, uint32_t)>(GX_VMA(0x21a8680ULL))(pNamedMap, h00, 0u);
        }
        *reinterpret_cast<void**>(pDevice + 0x240) = pNamedMap;

        auto pfnRegNamedTex = reinterpret_cast<void (*)(void*, const void*, void*)>(GX_VMA(0x21a8950ULL));
        pfnRegNamedTex(
            pNamedMap,
            reinterpret_cast<const void*>(GX_VMA(0x49c2dc0ULL)), // u"dummy128"
            (*reinterpret_cast<void***>(pDevice + 0x238))[2]);
        pfnRegNamedTex(
            *reinterpret_cast<void**>(pDevice + 0x240),
            reinterpret_cast<const void*>(GX_VMA(0x49c2dd2ULL)), // u"spe_040_s"
            (*reinterpret_cast<void***>(pDevice + 0x238))[3]);
    }

    {
        void* pTex0 = (*reinterpret_cast<void***>(pDevice + 0x238))[0];
        void* pTex1 = (*reinterpret_cast<void***>(pDevice + 0x238))[1];
        uint8_t* pDefSamp = GXDevice_HeapAllocAligned(*ppMainHeap, 0x68u, 8u);
        if (pDefSamp != nullptr) {
            void* pCurMainHeap = *ppMainHeap;
            *reinterpret_cast<uint32_t*>(pDefSamp + 0x08) = 0u;
            *reinterpret_cast<void**>(pDefSamp + 0x00) = reinterpret_cast<void*>(GX_VMA(0x536bdd0ULL));
            void* pStrHeap = GXDevice_EnsureDefaultContainerHeapChecked();
            *reinterpret_cast<void**>(pDefSamp + 0x38) = pStrHeap;
            *reinterpret_cast<uint64_t*>(pDefSamp + 0x30) = 7ULL;
            *reinterpret_cast<uint64_t*>(pDefSamp + 0x28) = 0ULL;
            *reinterpret_cast<uint16_t*>(pDefSamp + 0x18) = 0u;
            pDefSamp[0x40] = 1u;
            *reinterpret_cast<void**>(pDefSamp + 0x48) = pCurMainHeap;
            *reinterpret_cast<void**>(pDefSamp + 0x00) = reinterpret_cast<void*>(GX_VMA(0x535fe40ULL));
            *reinterpret_cast<uint32_t*>(pDefSamp + 0x60) = 0u;
            *reinterpret_cast<uint32_t*>(pDefSamp + 0x64) = 0x3f000000u;
            *reinterpret_cast<void**>(pDefSamp + 0x50) = pTex0;
            *reinterpret_cast<void**>(pDefSamp + 0x58) = pTex1;
        }
        uint8_t* pOldSamp = *reinterpret_cast<uint8_t**>(pDevice + 0x248);
        if (pOldSamp != pDefSamp) {
            if (pOldSamp != nullptr) {
                GXDevice_ReleaseSimpleRef8(pOldSamp);
                *reinterpret_cast<void**>(pDevice + 0x248) = nullptr;
            }
            *reinterpret_cast<void**>(pDevice + 0x248) = pDefSamp;
            if (pDefSamp != nullptr) {
                ++(*reinterpret_cast<uint32_t*>(pDefSamp + 0x08));
            }
        }
    }

    // 13. Load default shader pack (+0x388) and initialize dynamic buffer rings (+0x390, +0x398, +0x3a0)
    {
        uint8_t* pShaderPack = GXDevice_HeapAllocAligned(*ppMainHeap, 0x208u, 8u);
        if (pShaderPack != nullptr) {
            *reinterpret_cast<void**>(pShaderPack + 0x00) = h00;
            pShaderPack[0x08] = 0u;
            *reinterpret_cast<uint64_t*>(pShaderPack + 0x200) = 0ULL;
            memset(pShaderPack + 0x10, 0, 0x1e9u);
        }
        *reinterpret_cast<void**>(pDevice + 0x388) = pShaderPack;
        reinterpret_cast<void (*)(void*, const void*, uint32_t)>(GX_VMA(0x21919b0ULL))(
            pShaderPack,
            *reinterpret_cast<const void**>(pInitParams + 0x98),
            *reinterpret_cast<uint32_t*>(pInitParams + 0xa0));
    }

    auto pfnCreateDynRing = reinterpret_cast<int32_t (*)(void*, void*, uint32_t, uint32_t, void*, uint32_t, uint32_t, uint32_t)>(
        GX_VMA(0x26d7e60ULL));
    if (pfnCreateDynRing(pDevice + 0x390, *reinterpret_cast<void**>(pDevice + 0x00), 0x40000u, 0x10u, *ppMainHeap, 0u, 0u, 0u) != 0) {
        return false;
    }
    if (pfnCreateDynRing(pDevice + 0x398, *reinterpret_cast<void**>(pDevice + 0x00), 0x40000u, 0x10u, *ppMainHeap, 0u, 0u, 0u) != 0) {
        return false;
    }

    {
        uint32_t cursorCount = *reinterpret_cast<uint32_t*>(pDevice + 0x18);
        void** pCursors = reinterpret_cast<void**>(
            GXDevice_HeapAllocUnaligned(*ppMainHeap, static_cast<size_t>(cursorCount) * sizeof(void*)));
        *reinterpret_cast<void***>(pDevice + 0x3a0) = pCursors;
        if (cursorCount != 0u) {
            auto pfnCreateCursor = reinterpret_cast<void (*)(void*, void**, uint32_t, uint32_t)>(GX_VMA(0x26d7fb0ULL));
            for (uint32_t c = 0; c < cursorCount; ++c) {
                pfnCreateCursor(
                    *reinterpret_cast<void**>(pDevice + 0x398),
                    &(*reinterpret_cast<void***>(pDevice + 0x3a0))[c],
                    0x40u,
                    0u);
            }
        }
    }

    // 14. Initialize GXPerfDebugMenu (+0x638), post-init +0x268, async queue (+0x660), +0x5f8, and mark initialized (+0x3a8 = 1)
    {
        void* pDbgHeap = *reinterpret_cast<void**>(GX_VMA(0x5540690ULL));
        uint8_t* pPerfMenu = GXDevice_HeapAllocAligned(pDbgHeap, 0x860u, 8u);
        if (pPerfMenu != nullptr) {
            memset(pPerfMenu, 0, 0x64u);
            *reinterpret_cast<uint64_t*>(pPerfMenu + 0x7f0) = 0ULL;
            *reinterpret_cast<uint64_t*>(pPerfMenu + 0x7f8) = 0ULL;
            *reinterpret_cast<uint32_t*>(pPerfMenu + 0x800) = 0u;
            *reinterpret_cast<uint32_t*>(pPerfMenu + 0x804) = 0x500u;
            *reinterpret_cast<uint32_t*>(pPerfMenu + 0x808) = 0x2D0u;
            *reinterpret_cast<uint16_t*>(pPerfMenu + 0x80c) = 0x100u;
            pPerfMenu[0x80e] = 1u;
            memset(pPerfMenu + 0x810, 0, 9u * sizeof(uint64_t));
            *reinterpret_cast<uint32_t*>(pPerfMenu + 0x858) = 1u;
        }
        *reinterpret_cast<void**>(pDevice + 0x638) = pPerfMenu;

        pfnCreateDynRing(
            pPerfMenu,
            *reinterpret_cast<void**>(*ppSingleton),
            0x40000u,
            0x10u,
            *ppMainHeap,
            0u,
            0u,
            0u);

        auto pfnRegPerfHeapTimer = reinterpret_cast<void (*)(void*, void*, const void*)>(GX_VMA(0x217b440ULL));
        pfnRegPerfHeapTimer(
            *reinterpret_cast<void**>(pDevice + 0x638),
            *reinterpret_cast<void**>(pDevice + 0x5e0),
            reinterpret_cast<const void*>(GX_VMA(0x49c2de6ULL))); // u"CBHeap"
        pfnRegPerfHeapTimer(
            *reinterpret_cast<void**>(pDevice + 0x638),
            *reinterpret_cast<void**>(pDevice + 0x5e8),
            reinterpret_cast<const void*>(GX_VMA(0x49c2df4ULL))); // u"ILHeap"
    }

    reinterpret_cast<void (*)(void*)>(GX_VMA(0xe46350ULL))(*reinterpret_cast<void**>(pDevice + 0x268));

    {
        uint8_t* pAsyncQ = GXDevice_HeapAllocAligned(*ppMainHeap, 0x60u, 8u);
        if (pAsyncQ != nullptr) {
            reinterpret_cast<void (*)(void*)>(GX_VMA(0x21bce40ULL))(pAsyncQ);
        }
        *reinterpret_cast<void**>(pDevice + 0x660) = pAsyncQ;
    }

    {
        uint8_t* p5f8 = GXDevice_HeapAllocAligned(*ppMainHeap, 0x28u, 8u);
        if (p5f8 != nullptr) {
            alignas(16) uint8_t capsBuf[0x40] = {};
            void (**vtbl)(void*, void*, uint32_t) = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(h00);
            vtbl[4](capsBuf, h00, 0u); // *0x20(%rax)
            if ((capsBuf[0] & 0x20u) == 0u) {
                auto pfnLog = reinterpret_cast<void (*)(const char*, uint32_t, const char*, ...)>(GX_VMA(0x20b55b0ULL));
                pfnLog(nullptr, 0x3Eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }
            *reinterpret_cast<uint64_t*>(p5f8 + 0x18) = 0ULL;
            *reinterpret_cast<uint64_t*>(p5f8 + 0x10) = 0ULL;
            *reinterpret_cast<uint64_t*>(p5f8 + 0x08) = 0ULL;
            *reinterpret_cast<void**>(p5f8 + 0x20) = h00;
        }
        *reinterpret_cast<void**>(pDevice + 0x5f8) = p5f8;
    }

    pDevice[0x3a8] = 1u;
    return true;
}

/**
 * @brief Waits on the render thread sync, sorts and submits the current command queue, toggles the
 *        active double-buffer index (`+0x380 ^= 1`), and resets the new front command queue's allocators.
 * VMA: 0x2194420 (342 bytes)
 */
void GXDevice_FlushAndFlipCurrentCommandQueue(uint8_t* pDevice) {
    uint8_t* pSync = *reinterpret_cast<uint8_t**>(pDevice + 0x348);
    void (**pWaitVtable)(void*, uint32_t) = *reinterpret_cast<void (***)(void*, uint32_t)>(pSync + 0x68);
    pWaitVtable[3](pSync + 0x68, 0xFFFFFFFFu);

    int64_t curIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    uint8_t* pCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + curIdx * 8);
    int64_t jobCount = *reinterpret_cast<int32_t*>(pCmdQueue + 0x38);
    if (jobCount != 0) {
        uint8_t* pJobs = *reinterpret_cast<uint8_t**>(pCmdQueue + 0x30);
        struct SortScratchContext {
            void*    pBuffer;
            int64_t  allocCount;
            uint64_t zero10;
            int64_t  maxHalfCount;
            void*    pSelf;
        } scratch = { nullptr, 0, 0ULL, (jobCount + 1) / 2, nullptr };
        scratch.pSelf = &scratch;
        sub_21b4f70(pJobs, pJobs + (jobCount << 4), jobCount, &scratch);
        if (scratch.pBuffer != nullptr) {
            sub_2bbe588(scratch.pBuffer);
        }
    }

    int64_t submitIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    uint8_t* pSubmitQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + submitIdx * 8);
    sub_21b4d00(
        pDevice,
        *reinterpret_cast<void**>(pDevice + 0x10),
        *reinterpret_cast<void**>(pSubmitQueue + 0x30),
        *reinterpret_cast<uint32_t*>(pSubmitQueue + 0x38));

    int64_t flipBaseIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    int64_t nextIdx = flipBaseIdx ^ 1;
    *reinterpret_cast<int32_t*>(pDevice + 0x380) = static_cast<int32_t>(nextIdx);

    uint8_t* pNextCtxPool = *reinterpret_cast<uint8_t**>(pDevice + 0x350 + nextIdx * 8);
    *reinterpret_cast<uint32_t*>(pNextCtxPool + 0x28) = 0u;

    uint8_t* pNextCmdQueue = *reinterpret_cast<uint8_t**>(pDevice + 0x360 + nextIdx * 8);
    *reinterpret_cast<uint32_t*>(pNextCmdQueue + 0x28) = 0u;
    *reinterpret_cast<uint32_t*>(pNextCmdQueue + 0x38) = 0u;
    auto fnResetLinearRing = reinterpret_cast<uint8_t (*)(void*, void*, uint32_t)>(GX_VMA(0xe45530ULL));
    fnResetLinearRing(pNextCmdQueue + 0x48, *reinterpret_cast<void**>(pNextCmdQueue + 0xa8), 0x800u);
    fnResetLinearRing(pNextCmdQueue + 0x78, *reinterpret_cast<void**>(pNextCmdQueue + 0xa8), 0x800u);
    *reinterpret_cast<uint32_t*>(pNextCmdQueue + 0x40) = 0u;

    int64_t activeIdx = *reinterpret_cast<int32_t*>(pDevice + 0x380);
    void* pNextLinearHeap = *reinterpret_cast<void**>(pDevice + 0x370 + activeIdx * 8);
    void (**heapVtable)(void*) = *reinterpret_cast<void (***)(void*)>(pNextLinearHeap);
    heapVtable[15](pNextLinearHeap);
}

/**
 * @brief Render job callback that stores `rdx` into `pDevice->m_completedFrameCounter608` (`+0x608`).
 * VMA: 0x2194410 (8 bytes)
 */
void GXDevice_UpdateCompletedFrameCounterJob(void* /*pJobUnused*/, uint8_t* pDevice, uint64_t frameValue) {
    *(uint64_t*)(pDevice + 0x608) = frameValue;
}

/**
 * @brief Acquires a `0xF0`-byte command list slot from the active `GXCommandContextPool` (`+0x350[idx]`),
 *        finalizes the deferred `GXDeviceContext` into it, and enqueues it into the active `GXCommandQueue`.
 * VMA: 0x2194340 (205 bytes)
 */
uint8_t GXDevice_SubmitDeferredContextToCommandQueue(uint8_t* pDevice, uint8_t* pDeferredCtx, uint32_t sortKey) {
    if (pDeferredCtx[0x2093] != 0) {
        return 0;
    }

    int64_t activeIdx = *(int32_t*)(pDevice + 0x380);
    uint8_t* pCtxPool = *(uint8_t**)(pDevice + 0x350 + activeIdx * 8);
    uint8_t* pMutex   = pCtxPool + 0x30;

    void (**mutexVtable)(void*, uint32_t) = *(void (***)(void*, uint32_t))pMutex;
    mutexVtable[3](pMutex, 0xFFFFFFFFu);

    int64_t curUsed = *(int32_t*)(pCtxPool + 0x28);
    uint8_t* pBegin = *(uint8_t**)(pCtxPool + 0x08);
    uint8_t* pEnd   = *(uint8_t**)(pCtxPool + 0x10);
    uint64_t capacity = (uint64_t)(pEnd - pBegin) / 0xF0u;

    uint8_t* pSlot = nullptr;
    if ((uint64_t)curUsed < capacity) {
        *(int32_t*)(pCtxPool + 0x28) = (int32_t)(curUsed + 1);
        pSlot = pBegin + (uint64_t)curUsed * 0xF0u;
    }

    mutexVtable = *(void (***)(void*, uint32_t))pMutex;
    reinterpret_cast<void (*)(void*)>(reinterpret_cast<uintptr_t>(mutexVtable[5]))(pMutex);

    if (!pSlot) {
        return 0;
    }

    extern void sub_e47930(void* pOutSlot, void* pDeferredCtx);
    sub_e47930(pSlot, pDeferredCtx);

    int64_t submitIdx = *(int32_t*)(pDevice + 0x380);
    void* pActiveCmdQueue = *(void**)(pDevice + 0x360 + submitIdx * 8);
    return sub_21b4a00(pActiveCmdQueue, pSlot, sortKey);
}

/**
 * @brief Returns the active primary `GXDeviceContext*` (`+0x78` if multi-threaded `+0x84` is set, else `+0x10`).
 * VMA: 0x2194320 (25 bytes)
 */
void* GXDevice_GetActiveImmediateContext(const uint8_t* pDevice) {
    if (pDevice[0x84] != 0) {
        return *(void* const*)(pDevice + 0x78);
    }
    return *(void* const*)(pDevice + 0x10);
}

/**
 * @brief Waits for the render thread sync object (`+0x348`) and resets the current frame's counter slot (`+0x670`).
 * VMA: 0x21942e0 (61 bytes)
 */
void GXDevice_WaitForRenderThreadIdle(uint8_t* pDevice) {
    uint8_t* pSync = *(uint8_t**)(pDevice + 0x348);
    void (**pWaitVtable)(void*, uint32_t) = *(void (***)(void*, uint32_t))(pSync + 0x68);
    pWaitVtable[3](pSync + 0x68, 0xFFFFFFFFu);

    int64_t idx = *(int32_t*)(pDevice + 0x68c);
    *(uint32_t*)(pDevice + 0x670 + idx * 16) = 0;
}

/**
 * @brief Ensures the built-in shader pack (`+0x388`) is initialized, calling `0x21915a0` if not yet initialized.
 * VMA: 0x21942c0 (26 bytes)
 */
bool GXDevice_EnsureDefaultShadersInitialized(uint8_t* pDevice) {
    uint8_t* pShaderPack = *(uint8_t**)(pDevice + 0x388);
    if (pShaderPack[0x08] != 0) {
        return true;
    }
    return sub_21915a0(pShaderPack);
}

/**
 * @brief Returns one of the 16 built-in fallback textures (`+0x238[index]`).
 * VMA: 0x21942b0 (14 bytes)
 */
void* GXDevice_GetDefaultFallbackTexture(const uint8_t* pDevice, uint32_t index) {
    void* const* ppTextures = *(void* const**)(pDevice + 0x238);
    return ppTextures[index];
}

/**
 * @brief Sets the VSync enable flag (`+0x104`) and propagates it to all display outputs (`+0x90..0x98`).
 * VMA: 0x2194270 (59 bytes)
 */
void GXDevice_SetVsyncEnabled(uint8_t* pDevice, uint8_t bEnabled) {
    pDevice[0x104] = bEnabled;
    uint8_t* pCur = *(uint8_t**)(pDevice + 0x90);
    uint8_t* pEnd = *(uint8_t**)(pDevice + 0x98);
    for (; pCur != pEnd; pCur += 0xB0) {
        uint8_t* pOutput = *(uint8_t**)(pCur + 0x08);
        pOutput[0x08] = bEnabled;
    }
#undef g_lastVsyncState_513ac84
#define g_lastVsyncState_513ac84 (*reinterpret_cast<uint8_t*>(GX_VMA(0x513ac84ULL)))
    g_lastVsyncState_513ac84 = bEnabled;
}

/**
 * @brief Base destructor for `GXDevice`, releasing debug property nodes, internal heaps,
 *        listener vectors, worker pools, default sampler state, and display output vectors.
 * VMA: 0x2194030 (572 bytes)
 */
static inline void GXDevice_FreeDLVectorBuffer(uint8_t* pVecBeginSlot) {
    void* pBegin = *reinterpret_cast<void**>(pVecBeginSlot);
    if (pBegin != nullptr) {
        void* pAlloc = *reinterpret_cast<void**>(pVecBeginSlot + 0x18);
        void (**vtable)(void*, void*) = *reinterpret_cast<void (***)(void*, void*)>(pAlloc);
        vtable[14](pAlloc, pBegin); // *0x70(%rax)
        *reinterpret_cast<void**>(pVecBeginSlot + 0x10) = nullptr;
        *reinterpret_cast<void**>(pVecBeginSlot + 0x08) = nullptr;
        *reinterpret_cast<void**>(pVecBeginSlot + 0x00) = nullptr;
    }
}

void GXDevice_BaseDtor(uint8_t* pDevice) {
    // 1. Release m_pDebugRootNode (+0x690) using Dantelion2 dual 15-bit strong/weak refcount
    uint8_t* pRootNode = *reinterpret_cast<uint8_t**>(pDevice + 0x690);
    if (pRootNode != nullptr) {
        uint32_t oldRef   = *reinterpret_cast<uint32_t*>(pRootNode + 0x08);
        uint32_t newStrong = (oldRef + 0x7FFFu) & 0x7FFFu;
        uint32_t curRef   = (oldRef & 0xFFFF8000u) | newStrong;
        *reinterpret_cast<uint32_t*>(pRootNode + 0x08) = curRef;

        if (newStrong == 0u && (oldRef & 0x40000000u) != 0u) {
            void (**vtable)(void*) = *reinterpret_cast<void (***)(void*)>(pRootNode);
            vtable[3](pRootNode); // *0x18(%rax)
            curRef = *reinterpret_cast<uint32_t*>(pRootNode + 0x08) & 0xBFFFFFFFu;
            *reinterpret_cast<uint32_t*>(pRootNode + 0x08) = curRef;
        }

        uint32_t newWeak = ((curRef >> 15) + 0x7FFFu) & 0x7FFFu;
        *reinterpret_cast<uint32_t*>(pRootNode + 0x08) = (curRef & 0xC0007FFFu) | (newWeak << 15);
        if (newWeak == 0u) {
            *reinterpret_cast<uint32_t*>(pRootNode + 0x08) = curRef | 0x3FFF8000u;
            void (**vtable)(void*) = *reinterpret_cast<void (***)(void*)>(pRootNode);
            vtable[4](pRootNode); // *0x20(%rax)
        }
        *reinterpret_cast<void**>(pDevice + 0x690) = nullptr;
    }

    // 2. Free m_callbackPairs (+0x618..+0x630)
    GXDevice_FreeDLVectorBuffer(pDevice + 0x618);

    // 3. Destroy internal heaps (+0x4d8 and +0x3d0)
    sub_219d550(pDevice + 0x4d8);
    sub_219d550(pDevice + 0x3d0);

    // 4. Free m_frameListeners (+0x328..+0x340) and m_pendingWorkerTasks (+0x300..+0x318)
    GXDevice_FreeDLVectorBuffer(pDevice + 0x328);
    GXDevice_FreeDLVectorBuffer(pDevice + 0x300);

    // 5. Destroy worker thread pool (+0x278) and free +0x2c0..+0x2d8
    sub_21b84b0(pDevice + 0x278);
    GXDevice_FreeDLVectorBuffer(pDevice + 0x2c0);

    // 6. Release default sampler state (+0x248)
    uint8_t* pSampler = *reinterpret_cast<uint8_t**>(pDevice + 0x248);
    if (pSampler != nullptr) {
        int32_t oldRef = *reinterpret_cast<int32_t*>(pSampler + 0x08);
        *reinterpret_cast<int32_t*>(pSampler + 0x08) = oldRef - 1;
        if (oldRef <= 1) {
            void (**vtable)(void*) = *reinterpret_cast<void (***)(void*)>(pSampler);
            vtable[0](pSampler);
        }
        *reinterpret_cast<void**>(pDevice + 0x248) = nullptr;
    }

    // 7. Free display output vectors (+0x90..+0xa8 and +0x30..+0x48)
    GXDevice_FreeDLVectorBuffer(pDevice + 0x90);
    GXDevice_FreeDLVectorBuffer(pDevice + 0x30);
}

static inline void GXDevice_InitDLVectorWithDefaultHeap(uint8_t* pVec) {
    void** ppDefaultHeap = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* pHeap = *ppDefaultHeap;
    if (pHeap == nullptr) {
        pHeap = reinterpret_cast<void* (*)()>(GX_VMA(0x2ba2b10ULL))();
        *ppDefaultHeap = pHeap;
    }

    uint64_t caps = 0;
    void (**pVtable)(void*, void*, uint32_t) =
        *reinterpret_cast<void (***)(void*, void*, uint32_t)>(pHeap);
    pVtable[4](&caps, pHeap, 0u);
    if ((reinterpret_cast<const uint8_t*>(&caps)[0] & 0x20u) == 0u) {
        reinterpret_cast<void (*)(void*, uint32_t, const char*, ...)>(GX_VMA(0x20b55b0ULL))(
            nullptr,
            0x3Eu,
            reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(pVec + 0x10) = 0ULL;
    *reinterpret_cast<uint64_t*>(pVec + 0x08) = 0ULL;
    *reinterpret_cast<uint64_t*>(pVec + 0x00) = 0ULL;
    *reinterpret_cast<void**>(pVec + 0x18)    = pHeap;
}

static inline void GXDevice_InitEmbeddedHeapState(uint8_t* pHeapState) {
    *reinterpret_cast<void**>(pHeapState + 0x18) = reinterpret_cast<void*>(GX_VMA(0x53b6770ULL));
    *reinterpret_cast<void**>(pHeapState + 0x20) = nullptr;
    memset(pHeapState + 0x8F, 0, 0x59);
    *reinterpret_cast<void**>(pHeapState + 0xE8) = reinterpret_cast<void*>(GX_VMA(0x53b2fa0ULL));
    pHeapState[0xF8] = 0u;
    *reinterpret_cast<uint64_t*>(pHeapState + 0x00) = 0ULL;
    *reinterpret_cast<uint32_t*>(pHeapState + 0x08) =
        reinterpret_cast<uint32_t (*)()>(GX_VMA(0x207b9f0ULL))();
    *reinterpret_cast<uint64_t*>(pHeapState + 0x10) = 0ULL;
    *reinterpret_cast<uint32_t*>(pHeapState + 0x100) = 0u;
    *reinterpret_cast<void**>(pHeapState + 0x20) = pHeapState;
}

/**
 * @brief Constructs a `GXDevice` (`0x6a0` bytes), initializing all internal vectors, heaps,
 *        gamma curve defaults (`2.2f` = `0x400ccccd`), and thread affinity tables.
 * VMA: 0x2193880 (1,954 bytes)
 */
void GXDevice_Construct(uint8_t* pDevice) {
    *reinterpret_cast<uint64_t*>(pDevice + 0x20) = 0ULL;
    *reinterpret_cast<uint32_t*>(pDevice + 0x18) = 0u;
    *reinterpret_cast<uint64_t*>(pDevice + 0x10) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x08) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x00) = 0ULL;

    GXDevice_InitDLVectorWithDefaultHeap(pDevice + 0x30);

    pDevice[0x86] = 0u;
    *reinterpret_cast<uint16_t*>(pDevice + 0x84) = 0u;
    *reinterpret_cast<uint32_t*>(pDevice + 0x80) = 0u;
    *reinterpret_cast<uint64_t*>(pDevice + 0x78) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x70) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x68) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x60) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x58) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x50) = 0ULL;

    GXDevice_InitDLVectorWithDefaultHeap(pDevice + 0x90);

    *reinterpret_cast<uint64_t*>(pDevice + 0xb0) = 0ULL;
    for (size_t off = 0x230; off <= 0x270; off += 8) {
        *reinterpret_cast<uint64_t*>(pDevice + off) = 0ULL;
    }

    GXDevice_InitDLVectorWithDefaultHeap(pDevice + 0x2c0);

    pDevice[0x2e0] = 0u;
    *reinterpret_cast<uint64_t*>(pDevice + 0x2e8) = 0ULL;
    for (size_t off = 0x278; off <= 0x2b0; off += 8) {
        *reinterpret_cast<uint64_t*>(pDevice + off) = 0ULL;
    }
    *reinterpret_cast<uint32_t*>(pDevice + 0x2f0) = 0u;

    GXDevice_InitDLVectorWithDefaultHeap(pDevice + 0x300);
    GXDevice_InitDLVectorWithDefaultHeap(pDevice + 0x328);

    *reinterpret_cast<uint64_t*>(pDevice + 0x348) = 0ULL;
    *reinterpret_cast<uint32_t*>(pDevice + 0x380) = 0u;
    *reinterpret_cast<uint32_t*>(pDevice + 0x3a8) = 0u;
    *reinterpret_cast<uint64_t*>(pDevice + 0x3a0) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x398) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x390) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x388) = 0ULL;

    *reinterpret_cast<void**>(pDevice + 0x3b0) =
        reinterpret_cast<uint8_t*>(GX_VMA(0x535f820ULL)) + 0x10;
    for (size_t offset = 0x3b8; offset <= 0x3cc; offset += 4) {
        *reinterpret_cast<uint32_t*>(pDevice + offset) = 0x400ccccdu;
    }

    GXDevice_InitEmbeddedHeapState(pDevice + 0x3d0);
    GXDevice_InitEmbeddedHeapState(pDevice + 0x4d8);

    *reinterpret_cast<uint64_t*>(pDevice + 0x5e8) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x5e0) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x608) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x600) = 0ULL;
    *reinterpret_cast<uint64_t*>(pDevice + 0x5f8) = 0ULL;

    GXDevice_InitDLVectorWithDefaultHeap(pDevice + 0x618);

    *reinterpret_cast<uint64_t*>(pDevice + 0x690) = 0ULL;
    for (size_t off = 0x638; off <= 0x660; off += 8) {
        *reinterpret_cast<uint64_t*>(pDevice + off) = 0ULL;
    }

    reinterpret_cast<void (*)(void*)>(GX_VMA(0x1d99260ULL))(pDevice + 0xc0);

    *reinterpret_cast<uint32_t*>(pDevice + 0x5f0) = 0u;
    for (size_t off = 0x350; off <= 0x378; off += 8) {
        *reinterpret_cast<uint64_t*>(pDevice + off) = 0ULL;
    }

    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bd4ULL)) = 0u;
    *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6be8ULL)) = 0x0101010101010101ULL;
    *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6be0ULL)) = 0x0101010101010101ULL;
    *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6bd8ULL)) = 0x0101010101010101ULL;
    *reinterpret_cast<uint16_t*>(GX_VMA(0x56a6bf4ULL)) = 0x0101u;
    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bf0ULL)) = 0x01010101u;
    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6bd0ULL)) = 0u;

    *reinterpret_cast<uint16_t*>(GX_VMA(0x56a6c10ULL)) = 0x0101u;
    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c0cULL)) = 0x01010101u;
    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c14ULL)) = 0x3Fu;

    *reinterpret_cast<uint16_t*>(GX_VMA(0x56a6c1cULL)) = 0x0101u;
    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c18ULL)) = 0x01010101u;
    *reinterpret_cast<uint32_t*>(GX_VMA(0x56a6c20ULL)) = 0x3Fu;
}

// ============================================================================
// 4. GXCameraMatrixBlock & View Frustum Planes (0x2192be0 .. 0x21935f0)
// ============================================================================

/**
 * @brief Copies all matrices, frustum planes, and viewport parameters (`+0x10..0x219`) between
 *        two `GXCameraMatrixBlock` instances.
 * VMA: 0x21935f0 (505 bytes)
 */
void GXCameraMatrixBlock_CopyState(uint8_t* pDst, const uint8_t* pSrc) {
    for (size_t off = 0x10; off <= 0x100; off += 0x10) {
        _mm_store_ps(reinterpret_cast<float*>(pDst + off),
                     _mm_load_ps(reinterpret_cast<const float*>(pSrc + off)));
    }
    *reinterpret_cast<uint32_t*>(pDst + 0x110) = *reinterpret_cast<const uint32_t*>(pSrc + 0x110);
    *reinterpret_cast<uint32_t*>(pDst + 0x114) = *reinterpret_cast<const uint32_t*>(pSrc + 0x114);

    _mm_store_ps(reinterpret_cast<float*>(pDst + 0x120),
                 _mm_load_ps(reinterpret_cast<const float*>(pSrc + 0x120)));
    _mm_store_ps(reinterpret_cast<float*>(pDst + 0x130),
                 _mm_load_ps(reinterpret_cast<const float*>(pSrc + 0x130)));
    *reinterpret_cast<float*>(pDst + 0x140) = *reinterpret_cast<const float*>(pSrc + 0x140);

    for (size_t off = 0x150; off <= 0x200; off += 0x10) {
        _mm_store_ps(reinterpret_cast<float*>(pDst + off),
                     _mm_load_ps(reinterpret_cast<const float*>(pSrc + off)));
    }
    *reinterpret_cast<uint64_t*>(pDst + 0x210) = *reinterpret_cast<const uint64_t*>(pSrc + 0x210);
    pDst[0x218] = pSrc[0x218];
}

static inline void GXSimdInvert4x4(const __m128 inRows[4], __m128 outRows[4]) {
    __m128 xmm0 = inRows[0];
    __m128 xmm1 = inRows[1];
    __m128 xmm2 = inRows[2];
    __m128 xmm3 = inRows[3];

    __m128 xmm4  = _mm_movelh_ps(xmm0, xmm1);
    xmm0         = _mm_movehl_ps(xmm1, xmm0);
    xmm1         = _mm_movelh_ps(xmm2, xmm3);
    xmm2         = _mm_movehl_ps(xmm3, xmm2);

    __m128 xmm15 = _mm_shuffle_ps(xmm4, xmm1, 0x88);
    xmm1         = _mm_shuffle_ps(xmm4, xmm1, 0xDD);
    xmm4         = _mm_shuffle_ps(xmm0, xmm2, 0x88);
    xmm0         = _mm_shuffle_ps(xmm0, xmm2, 0xDD);

    __m128 xmm8  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x50));
    __m128 xmm9  = _mm_movehl_ps(xmm0, xmm0);
    __m128 xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x50));
    __m128 xmm7  = _mm_movehl_ps(xmm1, xmm1);
    xmm2         = _mm_shuffle_ps(xmm4, xmm15, 0x88);
    __m128 xmm5  = _mm_shuffle_ps(xmm0, xmm1, 0xDD);

    xmm8         = _mm_mul_ps(xmm8, xmm9);
    __m128 xmm10 = _mm_mul_ps(xmm6, xmm7);
    xmm9         = _mm_mul_ps(xmm2, xmm5);

    __m128 xmm11 = _mm_movehl_ps(xmm4, xmm4);
    xmm7         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x50));
    __m128 xmm12 = _mm_movehl_ps(xmm15, xmm15);
    xmm6         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x50));
    xmm5         = _mm_shuffle_ps(xmm4, xmm15, 0xDD);
    xmm2         = _mm_shuffle_ps(xmm0, xmm1, 0x88);

    xmm7         = _mm_mul_ps(xmm11, xmm7);
    xmm6         = _mm_mul_ps(xmm12, xmm6);
    xmm2         = _mm_mul_ps(xmm5, xmm2);

    xmm5         = _mm_sub_ps(xmm8, xmm7);
    xmm6         = _mm_sub_ps(xmm10, xmm6);
    xmm2         = _mm_sub_ps(xmm9, xmm2);

    xmm7         = _mm_shuffle_ps(xmm5, xmm2, 0x5D);
    xmm8         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x49));
    xmm9         = _mm_shuffle_ps(xmm7, xmm5, 0x32);
    xmm10        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x12));
    xmm11        = _mm_shuffle_ps(xmm7, xmm5, 0x99);
    xmm7         = _mm_shuffle_ps(xmm6, xmm2, 0xFD);
    xmm12        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x49));
    __m128 xmm13 = _mm_shuffle_ps(xmm7, xmm6, 0x32);
    __m128 xmm14 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x12));
    xmm7         = _mm_shuffle_ps(xmm7, xmm6, 0x99);

    xmm8         = _mm_mul_ps(xmm8, xmm9);
    xmm9         = _mm_mul_ps(xmm10, xmm11);
    xmm10        = _mm_mul_ps(xmm12, xmm13);
    xmm11        = _mm_mul_ps(xmm14, xmm7);

    xmm7         = _mm_shuffle_ps(xmm5, xmm2, 0x04);
    xmm12        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x9E));
    xmm13        = _mm_shuffle_ps(xmm5, xmm7, 0x93);
    xmm14        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x7B));
    xmm3         = _mm_shuffle_ps(xmm5, xmm7, 0x26);
    xmm7         = _mm_shuffle_ps(xmm6, xmm2, 0xA4);
    xmm12        = _mm_mul_ps(xmm12, xmm13);
    xmm13        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x9E));
    xmm14        = _mm_mul_ps(xmm14, xmm3);
    xmm3         = _mm_shuffle_ps(xmm6, xmm7, 0x93);
    xmm13        = _mm_mul_ps(xmm13, xmm3);
    xmm3         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x7B));
    xmm7         = _mm_shuffle_ps(xmm6, xmm7, 0x26);
    xmm3         = _mm_mul_ps(xmm3, xmm7);

    xmm7         = _mm_sub_ps(xmm8, xmm12);
    xmm8         = _mm_sub_ps(xmm9, xmm14);
    xmm9         = _mm_sub_ps(xmm10, xmm13);
    xmm12        = _mm_sub_ps(xmm11, xmm3);

    xmm10        = _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(xmm2), _mm_castps_si128(xmm5), 8));
    xmm11        = _mm_shuffle_ps(xmm5, xmm2, 0x4C);
    xmm5         = _mm_movehl_ps(xmm2, xmm6);
    xmm2         = _mm_shuffle_ps(xmm6, xmm2, 0xEC);

    xmm1         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x33));
    xmm6         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm10), 0x2C));
    xmm1         = _mm_mul_ps(xmm1, xmm6);
    xmm6         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x8D));
    xmm3         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm11), 0x93));
    xmm3         = _mm_mul_ps(xmm6, xmm3);
    xmm0         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x33));
    xmm5         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm5), 0x2C));
    xmm0         = _mm_mul_ps(xmm0, xmm5);
    xmm4         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x8D));
    xmm2         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x93));
    xmm2         = _mm_mul_ps(xmm4, xmm2);

    xmm4         = _mm_sub_ps(xmm7, xmm1);
    xmm1         = _mm_add_ps(xmm1, xmm7);
    xmm5         = _mm_add_ps(xmm3, xmm8);
    xmm3         = _mm_sub_ps(xmm8, xmm3);
    xmm6         = _mm_sub_ps(xmm9, xmm0);
    xmm0         = _mm_add_ps(xmm0, xmm9);
    xmm7         = _mm_add_ps(xmm2, xmm12);
    xmm2         = _mm_sub_ps(xmm12, xmm2);

    xmm1         = _mm_shuffle_ps(xmm1, xmm4, 0xD8);
    xmm3         = _mm_shuffle_ps(xmm3, xmm5, 0xD8);
    xmm0         = _mm_shuffle_ps(xmm0, xmm6, 0xD8);
    xmm2         = _mm_shuffle_ps(xmm2, xmm7, 0xD8);

    xmm1         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0xD8));
    xmm3         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm3), 0xD8));
    xmm0         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0xD8));
    xmm2         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0xD8));

    xmm4         = _mm_mul_ps(xmm15, xmm1);
    xmm4         = _mm_hadd_ps(xmm4, xmm4);
    xmm4         = _mm_hadd_ps(xmm4, xmm4);
    xmm5         = _mm_load_ps(reinterpret_cast<const float*>(GX_VMA(0x4920750ULL)));
    xmm4         = _mm_div_ps(xmm5, xmm4);

    outRows[0]   = _mm_mul_ps(xmm4, xmm1);
    outRows[1]   = _mm_mul_ps(xmm4, xmm3);
    outRows[2]   = _mm_mul_ps(xmm4, xmm0);
    outRows[3]   = _mm_mul_ps(xmm4, xmm2);
}

static inline __m128 GXSimdMulVec4Mat4Rows(__m128 v, const __m128 mRows[4]) {
    __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), 0x00));
    __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), 0x55));
    __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), 0xAA));
    __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), 0xFF));
    __m128 s02 = _mm_add_ps(_mm_mul_ps(x, mRows[0]), _mm_mul_ps(z, mRows[2]));
    __m128 s13 = _mm_add_ps(_mm_mul_ps(y, mRows[1]), _mm_mul_ps(w, mRows[3]));
    return _mm_add_ps(s02, s13);
}

static inline __m128 GXSimdTransformPlane(__m128 plane, const __m128 vCols[4]) {
    __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(plane), 0x00));
    __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(plane), 0x55));
    __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(plane), 0xAA));
    __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(plane), 0xFF));
    __m128 s01 = _mm_add_ps(_mm_mul_ps(x, vCols[0]), _mm_mul_ps(y, vCols[1]));
    __m128 s23 = _mm_add_ps(_mm_mul_ps(z, vCols[2]), _mm_mul_ps(w, vCols[3]));
    return _mm_add_ps(s01, s23);
}

/**
 * @brief Updates the camera's View (`+0x10`), InvView (`+0x50`), Projection (`+0x90`),
 *        ViewProjection (`+0xd0`), and both view-space (`+0x1b0..0x200`) and world-space
 *        (`+0x150..0x1a0`) 6 frustum planes from the source camera transform (`rsi`).
 * VMA: 0x2192d20 (2,243 bytes)
 */
void GXCameraMatrixBlock_UpdateMatricesAndFrustumPlanes(uint8_t* pBlock, const uint8_t* pCameraTransform) {
    *reinterpret_cast<const uint8_t**>(pBlock + 0x210) = pCameraTransform;
    *reinterpret_cast<uint32_t*>(pBlock + 0x110) = *reinterpret_cast<const uint32_t*>(pCameraTransform + 0x5c);
    *reinterpret_cast<uint32_t*>(pBlock + 0x114) = *reinterpret_cast<const uint32_t*>(pCameraTransform + 0x60);
    *reinterpret_cast<float*>(pBlock + 0x140)    = *reinterpret_cast<const float*>(pCameraTransform + 0x50);

    // 1. Invert source camera world matrix (+0x00..+0x30) -> View matrix
    __m128 srcRows[4] = {
        _mm_load_ps(reinterpret_cast<const float*>(pCameraTransform + 0x00)),
        _mm_load_ps(reinterpret_cast<const float*>(pCameraTransform + 0x10)),
        _mm_load_ps(reinterpret_cast<const float*>(pCameraTransform + 0x20)),
        _mm_load_ps(reinterpret_cast<const float*>(pCameraTransform + 0x30)),
    };
    alignas(16) __m128 viewRows[4];
    GXSimdInvert4x4(srcRows, viewRows);

    // 2. Build Projection matrix from camera frustum parameters (0x21aa290)
    alignas(16) __m128 projRows[4];
    sub_21aa290(projRows, pCameraTransform);

    // 3. Store View (+0x10..+0x40) and Projection (+0x90..+0xc0)
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x10), viewRows[0]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x20), viewRows[1]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x30), viewRows[2]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x40), viewRows[3]);

    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x90), projRows[0]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0xa0), projRows[1]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0xb0), projRows[2]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0xc0), projRows[3]);

    // 4. Extract View Z-column (+0x120) and FOV/resolution-scaled Z-column (+0x130)
    __m128 hi01    = _mm_unpackhi_ps(viewRows[0], viewRows[1]);
    __m128 z012    = _mm_shuffle_ps(hi01, viewRows[2], 0x24);
    __m128 hi32    = _mm_movehl_ps(z012, viewRows[3]);
    __m128 viewColZ = _mm_shuffle_ps(z012, hi32, 0x24);

    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x130), viewColZ);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x120), viewColZ);

    float fovY       = *reinterpret_cast<const float*>(pBlock + 0x140);
    float halfFov    = fovY * (*reinterpret_cast<const float*>(GX_VMA(0x49295c8ULL))); // 0.5f
    float tanHalfFov = sub_2bbe648(halfFov);
    float tanRefFov  = sub_2bbe648(*reinterpret_cast<const float*>(GX_VMA(0x49295ccULL))); // 0.47996554f
    float heightNorm = static_cast<float>(static_cast<int64_t>(*reinterpret_cast<const uint32_t*>(pBlock + 0x114)))
                     / (*reinterpret_cast<const float*>(GX_VMA(0x49295d0ULL))); // 720.0f
    float lodScale   = (tanHalfFov / tanRefFov) / heightNorm;
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x130), _mm_mul_ps(viewColZ, _mm_set1_ps(lodScale)));

    // 5. Compute ViewProjection = View * Projection (+0xd0..+0x100)
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x0d0), GXSimdMulVec4Mat4Rows(viewRows[0], projRows));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x0e0), GXSimdMulVec4Mat4Rows(viewRows[1], projRows));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x0f0), GXSimdMulVec4Mat4Rows(viewRows[2], projRows));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x100), GXSimdMulVec4Mat4Rows(viewRows[3], projRows));

    // 6. Invert View (+0x10..+0x40) -> InvView (+0x50..+0x80)
    __m128 storedViewRows[4] = {
        _mm_load_ps(reinterpret_cast<const float*>(pBlock + 0x10)),
        _mm_load_ps(reinterpret_cast<const float*>(pBlock + 0x20)),
        _mm_load_ps(reinterpret_cast<const float*>(pBlock + 0x30)),
        _mm_load_ps(reinterpret_cast<const float*>(pBlock + 0x40)),
    };
    __m128 invViewRows[4];
    GXSimdInvert4x4(storedViewRows, invViewRows);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x50), invViewRows[0]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x60), invViewRows[1]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x70), invViewRows[2]);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x80), invViewRows[3]);

    // 7. Extract 6 view-space frustum planes (+0x1b0..+0x200) from Projection columns
    __m128 pLo01 = _mm_movelh_ps(projRows[0], projRows[1]);
    __m128 pHi01 = _mm_movehl_ps(projRows[1], projRows[0]);
    __m128 pLo23 = _mm_movelh_ps(projRows[2], projRows[3]);
    __m128 pHi23 = _mm_movehl_ps(projRows[3], projRows[2]);

    __m128 pCol0 = _mm_shuffle_ps(pLo01, pLo23, 0x88);
    __m128 pCol1 = _mm_shuffle_ps(pLo01, pLo23, 0xDD);
    __m128 pCol2 = _mm_shuffle_ps(pHi01, pHi23, 0x88);
    __m128 pCol3 = _mm_shuffle_ps(pHi01, pHi23, 0xDD);

    __m128 zero = _mm_setzero_ps();
    __m128 vp0  = _mm_sub_ps(zero, _mm_add_ps(pCol3, pCol0)); // Left
    __m128 vp1  = _mm_sub_ps(zero, _mm_sub_ps(pCol3, pCol0)); // Right
    __m128 vp2  = _mm_sub_ps(zero, _mm_add_ps(pCol3, pCol1)); // Bottom
    __m128 vp3  = _mm_sub_ps(zero, _mm_sub_ps(pCol3, pCol1)); // Top
    __m128 vp4  = _mm_sub_ps(zero, pCol2);                    // Near
    __m128 vp5  = _mm_sub_ps(zero, _mm_sub_ps(pCol3, pCol2)); // Far

    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x1b0), vp0);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x1c0), vp1);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x1d0), vp2);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x1e0), vp3);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x1f0), vp4);
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x200), vp5);

    // 8. Transform 6 frustum planes to world space (+0x150..+0x1a0) via View^T
    __m128 vLo01 = _mm_movelh_ps(viewRows[0], viewRows[1]);
    __m128 vHi01 = _mm_movehl_ps(viewRows[1], viewRows[0]);
    __m128 vLo23 = _mm_movelh_ps(viewRows[2], viewRows[3]);
    __m128 vHi23 = _mm_movehl_ps(viewRows[3], viewRows[2]);

    __m128 vCols[4] = {
        _mm_shuffle_ps(vLo01, vLo23, 0x88),
        _mm_shuffle_ps(vLo01, vLo23, 0xDD),
        _mm_shuffle_ps(vHi01, vHi23, 0x88),
        _mm_shuffle_ps(vHi01, vHi23, 0xDD),
    };

    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x150), GXSimdTransformPlane(vp0, vCols));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x160), GXSimdTransformPlane(vp1, vCols));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x170), GXSimdTransformPlane(vp2, vCols));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x180), GXSimdTransformPlane(vp3, vCols));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x190), GXSimdTransformPlane(vp4, vCols));
    _mm_store_ps(reinterpret_cast<float*>(pBlock + 0x1a0), GXSimdTransformPlane(vp5, vCols));
}

/**
 * @brief Constructs a `GXCameraMatrixBlock`, initializing View, InvView, Proj, and ViewProj
 *        to identity, reading default viewport width/height from the primary display output,
 *        setting default FOV (`0x3f75be0b`), and zeroing the 6 frustum planes (`+0x150..0x210`).
 * VMA: 0x2192be0 (315 bytes)
 */
void GXCameraMatrixBlock_Construct(uint8_t* pBlock, uint8_t* pOwnerContext) {
    *(uint8_t**)(pBlock + 0x00) = pOwnerContext;
    *(void**)(pBlock + 0x08)    = *(void**)(pOwnerContext + 0x08);

    static const float kIdentity4x4[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    memcpy(pBlock + 0xd0, kIdentity4x4, sizeof(kIdentity4x4));
    memcpy(pBlock + 0x90, kIdentity4x4, sizeof(kIdentity4x4));
    memcpy(pBlock + 0x10, kIdentity4x4, sizeof(kIdentity4x4));

    *(void**)(pBlock + 0x210) = nullptr;
    memset(pBlock + 0x120, 0, 32);

#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
    uint8_t* pPrimaryOutput = *(uint8_t**)(g_pGXDeviceSingleton_55406c8 + 0x90);
    *(uint32_t*)(pBlock + 0x110) = *(uint32_t*)(pPrimaryOutput + 0x80);
    *(uint32_t*)(pBlock + 0x114) = *(uint32_t*)(pPrimaryOutput + 0x84);
    *(uint32_t*)(pBlock + 0x140) = 0x3f75be0bu;

    memset(pBlock + 0x150, 0, 0xC0);
    pBlock[0x218] = 1;
}

// ============================================================================
// 5. GXDeferredDeleter Handlers & Enqueue Helpers (0x2192660 .. 0x2192b40)
// ============================================================================

/**
 * @brief Returns deferred resource deleter type ID `2`.
 * VMA: 0x2192b40 (6 bytes)
 */
uint32_t GXDeferredDeleterType2_GetTypeId() { return 2; }

/**
 * @brief Invokes virtual destructor (`vfunc[0]`) on `pPayload` and frees its memory block via `0x207b720`.
 * VMA: 0x2192b00 (58 bytes)
 */
void GXDeferredDeleterType2_DestroyPayload(void* /*pThis*/, void* pPayload) {
    if (!pPayload) return;
    void* pAlloc = sub_207b720(pPayload);
    void (**vtable)(void*) = *(void (***)(void*))pPayload;
    vtable[0](pPayload);
    void (**allocVtable)(void*, void*) = *(void (***)(void*, void*))pAlloc;
    allocVtable[14](pAlloc, pPayload);
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType2`.
 * VMA: 0x2192af0 (5 bytes)
 */
void GXDeferredDeleterType2_DeletingDtor(void* pThis) {
    extern void sub_2bbe588(void*);
    sub_2bbe588(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType2`.
 * VMA: 0x2192ae0 (1 byte)
 */
void GXDeferredDeleterType2_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Returns deferred resource deleter type ID `3`.
 * VMA: 0x2192ad0 (6 bytes)
 */
uint32_t GXDeferredDeleterType3_GetTypeId() { return 3; }

/**
 * @brief Invokes virtual destructor (`vfunc[0]`) on `pPayload` and frees its memory block via `0x207b720`.
 * VMA: 0x2192a90 (58 bytes)
 */
void GXDeferredDeleterType3_DestroyPayload(void* pThis, void* pPayload) {
    GXDeferredDeleterType2_DestroyPayload(pThis, pPayload);
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType3`.
 * VMA: 0x2192a80 (5 bytes)
 */
void GXDeferredDeleterType3_DeletingDtor(void* pThis) {
    GXDeferredDeleterType2_DeletingDtor(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType3`.
 * VMA: 0x2192a70 (1 byte)
 */
void GXDeferredDeleterType3_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Returns deferred resource deleter type ID `4`.
 * VMA: 0x2192a60 (6 bytes)
 */
uint32_t GXDeferredDeleterType4_GetTypeId() { return 4; }

/**
 * @brief Invokes virtual destructor (`vfunc[0]`) on `pPayload` and frees its memory block via `0x207b720`.
 * VMA: 0x2192a20 (58 bytes)
 */
void GXDeferredDeleterType4_DestroyPayload(void* pThis, void* pPayload) {
    GXDeferredDeleterType2_DestroyPayload(pThis, pPayload);
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType4`.
 * VMA: 0x2192a10 (5 bytes)
 */
void GXDeferredDeleterType4_DeletingDtor(void* pThis) {
    GXDeferredDeleterType2_DeletingDtor(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType4`.
 * VMA: 0x2192a00 (1 byte)
 */
void GXDeferredDeleterType4_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Returns deferred resource deleter type ID `5`.
 * VMA: 0x21929f0 (6 bytes)
 */
uint32_t GXDeferredDeleterType5_GetTypeId() { return 5; }

/**
 * @brief Destroys a low-level buffer/surface payload via `0x26a95a0` and frees its memory block.
 * VMA: 0x21929b0 (58 bytes)
 */
void GXDeferredDeleterType5_DestroyPayload(void* /*pThis*/, void* pPayload) {
    if (!pPayload) return;
    void* pAlloc = sub_207b720(pPayload);
    extern void sub_26a95a0(void*);
    sub_26a95a0(pPayload);
    void (**allocVtable)(void*, void*) = *(void (***)(void*, void*))pAlloc;
    allocVtable[14](pAlloc, pPayload);
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType5`.
 * VMA: 0x21929a0 (5 bytes)
 */
void GXDeferredDeleterType5_DeletingDtor(void* pThis) {
    GXDeferredDeleterType2_DeletingDtor(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType5`.
 * VMA: 0x2192990 (1 byte)
 */
void GXDeferredDeleterType5_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Returns deferred resource deleter type ID `6`.
 * VMA: 0x2192980 (6 bytes)
 */
uint32_t GXDeferredDeleterType6_GetTypeId() { return 6; }

/**
 * @brief Destroys a low-level constant buffer payload via `0x26d7360` and frees its memory block.
 * VMA: 0x2192940 (58 bytes)
 */
void GXDeferredDeleterType6_DestroyPayload(void* /*pThis*/, void* pPayload) {
    if (!pPayload) return;
    void* pAlloc = sub_207b720(pPayload);
    extern void sub_26d7360(void*);
    sub_26d7360(pPayload);
    void (**allocVtable)(void*, void*) = *(void (***)(void*, void*))pAlloc;
    allocVtable[14](pAlloc, pPayload);
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType6`.
 * VMA: 0x2192930 (5 bytes)
 */
void GXDeferredDeleterType6_DeletingDtor(void* pThis) {
    GXDeferredDeleterType2_DeletingDtor(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType6`.
 * VMA: 0x2192920 (1 byte)
 */
void GXDeferredDeleterType6_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Returns deferred resource deleter type ID `7`.
 * VMA: 0x2192910 (6 bytes)
 */
uint32_t GXDeferredDeleterType7_GetTypeId() { return 7; }

/**
 * @brief Atomically decrements `[pPayload+0x10]` and, if last reference, invokes `vfunc[0]` and frees via `[pPayload+0x08]`.
 * VMA: 0x21928c0 (66 bytes)
 */
void GXDeferredDeleterType7_DestroyPayload(void* /*pThis*/, uint8_t* pPayload) {
    if (__sync_fetch_and_add((int32_t*)(pPayload + 0x10), -1) == 1 && pPayload) {
        void* pAlloc = *(void**)(pPayload + 0x08);
        void (**vtable)(void*) = *(void (***)(void*))pPayload;
        vtable[0](pPayload);
        void (**allocVtable)(void*, void*) = *(void (***)(void*, void*))pAlloc;
        allocVtable[14](pAlloc, pPayload);
    }
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType7`.
 * VMA: 0x21928b0 (5 bytes)
 */
void GXDeferredDeleterType7_DeletingDtor(void* pThis) {
    GXDeferredDeleterType2_DeletingDtor(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType7`.
 * VMA: 0x21928a0 (1 byte)
 */
void GXDeferredDeleterType7_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Returns deferred resource deleter type ID `8`.
 * VMA: 0x2192890 (6 bytes)
 */
uint32_t GXDeferredDeleterType8_GetTypeId() { return 8; }

/**
 * @brief Decrements `[pPayload+0x08]` and, if `<= 1`, invokes `vfunc[0]` on `pPayload`.
 * VMA: 0x2192870 (23 bytes)
 */
void GXDeferredDeleterType8_DestroyPayload(void* /*pThis*/, uint8_t* pPayload) {
    int32_t oldRef = *(int32_t*)(pPayload + 0x08);
    *(int32_t*)(pPayload + 0x08) = oldRef - 1;
    if (oldRef <= 1) {
        void (**vtable)(void*) = *(void (***)(void*))pPayload;
        vtable[0](pPayload);
    }
}

/**
 * @brief Deleting destructor for `GXDeferredDeleterType8`.
 * VMA: 0x2192860 (5 bytes)
 */
void GXDeferredDeleterType8_DeletingDtor(void* pThis) {
    GXDeferredDeleterType2_DeletingDtor(pThis);
}

/**
 * @brief Base destructor for `GXDeferredDeleterType8`.
 * VMA: 0x2192850 (1 byte)
 */
void GXDeferredDeleterType8_BaseDtor(void* /*pThis*/) {}

/**
 * @brief Increments `[pResource+0x08]` and enqueues `pResource` into `g_pGXDeviceSingleton->m_pDeferredDeleter268`
 *        with handler descriptor `0x53bf1e0`.
 * VMA: 0x2192780 (58 bytes)
 */
uint8_t GXDeferredDeleter_EnqueueRefCountedResource(uint8_t* pResource) {
    if (!pResource) {
        return 0;
    }
    *(int32_t*)(pResource + 0x08) += 1;
#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
#undef g_deleterDesc_53bf1e0
#define g_deleterDesc_53bf1e0 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1e0ULL)))
    extern uint8_t sub_e46040(void* pDeleterQueue, void* pResource, const void* pDesc, uint32_t count);
    return sub_e46040(*(void**)(g_pGXDeviceSingleton_55406c8 + 0x268), pResource, g_deleterDesc_53bf1e0, 1);
}

/**
 * @brief Enqueues `pResource` into `m_pDeferredDeleter268` with handler descriptor `0x53bf1d8`.
 * VMA: 0x2192750 (40 bytes)
 */
uint8_t GXDeferredDeleter_EnqueueResourceD8(void* pResource) {
#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
#undef g_deleterDesc_53bf1d8
#define g_deleterDesc_53bf1d8 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1d8ULL)))
    extern uint8_t sub_e46040(void* pDeleterQueue, void* pResource, const void* pDesc, uint32_t count);
    return sub_e46040(*(void**)(g_pGXDeviceSingleton_55406c8 + 0x268), pResource, g_deleterDesc_53bf1d8, 1);
}

/**
 * @brief Enqueues `pResource` into `m_pDeferredDeleter268` with handler descriptor `0x53bf1d0`.
 * VMA: 0x2192720 (40 bytes)
 */
uint8_t GXDeferredDeleter_EnqueueResourceD0(void* pResource) {
#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
#undef g_deleterDesc_53bf1d0
#define g_deleterDesc_53bf1d0 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1d0ULL)))
    extern uint8_t sub_e46040(void* pDeleterQueue, void* pResource, const void* pDesc, uint32_t count);
    return sub_e46040(*(void**)(g_pGXDeviceSingleton_55406c8 + 0x268), pResource, g_deleterDesc_53bf1d0, 1);
}

/**
 * @brief Enqueues `pResource` into `m_pDeferredDeleter268` with handler descriptor `0x53bf1c8`.
 * VMA: 0x21926f0 (40 bytes)
 */
uint8_t GXDeferredDeleter_EnqueueResourceC8(void* pResource) {
#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
#undef g_deleterDesc_53bf1c8
#define g_deleterDesc_53bf1c8 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1c8ULL)))
    extern uint8_t sub_e46040(void* pDeleterQueue, void* pResource, const void* pDesc, uint32_t count);
    return sub_e46040(*(void**)(g_pGXDeviceSingleton_55406c8 + 0x268), pResource, g_deleterDesc_53bf1c8, 1);
}

/**
 * @brief Enqueues `pResource` into `m_pDeferredDeleter268` with handler descriptor `0x53bf1c0`.
 * VMA: 0x21926c0 (40 bytes)
 */
uint8_t GXDeferredDeleter_EnqueueResourceC0(void* pResource) {
#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
#undef g_deleterDesc_53bf1c0
#define g_deleterDesc_53bf1c0 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1c0ULL)))
    extern uint8_t sub_e46040(void* pDeleterQueue, void* pResource, const void* pDesc, uint32_t count);
    return sub_e46040(*(void**)(g_pGXDeviceSingleton_55406c8 + 0x268), pResource, g_deleterDesc_53bf1c0, 1);
}

/**
 * @brief Enqueues a buffer or texture resource (`+0x38 == 1 && (+0x3b & 4)`) into `m_pDeferredDeleter268`
 *        with handler descriptor `0x53bf1b8` or `0x53bf1b0`.
 * VMA: 0x2192660 (84 bytes)
 */
uint8_t GXDeferredDeleter_EnqueueBufferOrTextureResource(uint8_t* pResource) {
#undef g_pGXDeviceSingleton_55406c8
#define g_pGXDeviceSingleton_55406c8 (*reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL)))
#undef g_deleterDesc_53bf1b8
#define g_deleterDesc_53bf1b8 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1b8ULL)))
#undef g_deleterDesc_53bf1b0
#define g_deleterDesc_53bf1b0 (reinterpret_cast<uint8_t*>(GX_VMA(0x53bf1b0ULL)))
    extern uint8_t sub_e46040(void* pDeleterQueue, void* pResource, const void* pDesc, uint32_t count);

    const void* pDesc = (pResource[0x38] == 1 && (pResource[0x3b] & 0x04) != 0)
        ? g_deleterDesc_53bf1b8
        : g_deleterDesc_53bf1b0;
    return sub_e46040(*(void**)(g_pGXDeviceSingleton_55406c8 + 0x268), pResource, pDesc, 1);
}

// ============================================================================
// 6. GXConstantBufferHelper (0x2192040 .. 0x2192350)
// ============================================================================

static inline void MarkConstantBufferStageSlotMask(void** ppLowLevelCB, uint64_t stageBit, uint64_t slotBit) {
    uint8_t* pBuf = reinterpret_cast<uint8_t*>(ppLowLevelCB[0]);
    if (pBuf) {
        const uint64_t reqMask = stageBit | slotBit;
        const uint64_t curLow  = *reinterpret_cast<const uint32_t*>(pBuf + 0x44);
        if ((curLow & reqMask) != reqMask) {
            __atomic_fetch_or(reinterpret_cast<uint64_t*>(pBuf + 0x44), reqMask, __ATOMIC_SEQ_CST);
        }
    }
}

void GXConstantBufferHelper_UpdateAndBindPSConstantSlot0(
    uint8_t* pHelper,
    uint8_t* pDeviceCtx,
    const float* pColorRGBA,
    uint8_t bDirectLowLevelBind);

/**
 * @brief Lazily initializes two 4-byte PS constant buffers (`+0x1e8` = 0, `+0x1f0` = 1, flag `+0x1f8`),
 *        inspects the texture's sRGB/linear DXGI format (`vfunc +0x18`), and binds the appropriate
 *        constant buffer to PS constant buffer slot 1 (`+0x970` or low-level `+0xd530`).
 * VMA: 0x2192350 (627 bytes)
 */
void GXConstantBufferHelper_BindTextureAndUpdatePSConstantSlot1(
    uint8_t* pHelper,
    uint8_t* pDeviceCtx,
    uint8_t* pTexture,
    uint8_t bDirectLowLevelBind)
{
    if (!pTexture) {
        return;
    }

    if (pHelper[0x1f8] == 0) {
        void* pLowLevelCtx = *(void**)(pDeviceCtx + 0x08);
        uint32_t zeroVal = 0;
        sub_26d73f0(*(void**)(pHelper + 0x1e8), pLowLevelCtx, 0, &zeroVal);
        uint32_t oneVal = 1;
        sub_26d73f0(*(void**)(pHelper + 0x1f0), pLowLevelCtx, 0, &oneVal);
        pHelper[0x1f8] = 1;
    }

    void* (**texVtable)(void*) = *(void* (***)(void*))pTexture;
    uint8_t* pTexDesc = (uint8_t*)texVtable[3](pTexture);
    void* pSelectedCB = *(void**)(pHelper + 0x1e8);
    if (pTexDesc) {
        uint32_t fmt = *(uint32_t*)(pTexDesc + 0x30);
        if (fmt == 0x1D || fmt == 0x48 || fmt == 0x4B || fmt == 0x4E ||
            fmt == 0x5B || fmt == 0x5D || fmt == 0x63) {
            pSelectedCB = *(void**)(pHelper + 0x1f0);
        }
    }

    if (bDirectLowLevelBind != 0) {
        uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
        void** ppLowLevelCB = sub_26d7440(pSelectedCB);
        pLowLevelCtx[0x197a] |= 0x02;
        MarkConstantBufferStageSlotMask(ppLowLevelCB, 0x100000ULL, 0x2ULL);
        *(void**)(pLowLevelCtx + 0xd530) = ppLowLevelCB[0];
    } else {
        void** ppLowLevelCB = sub_26d7440(pSelectedCB);
        *(void**)(pDeviceCtx + 0x970) = ppLowLevelCB[0];
        if (*(uint32_t*)(pDeviceCtx + 0x9d8) != 0) {
            *(uint32_t*)(pDeviceCtx + 0x9d8) = 1;
        }
        if (*(uint32_t*)(pDeviceCtx + 0x9dc) < 2) {
            *(uint32_t*)(pDeviceCtx + 0x9dc) = 2;
        }
        uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
        *(uint64_t*)(pLowLevelCtx + 0x1978) |= 0x20000ULL;
        MarkConstantBufferStageSlotMask(ppLowLevelCB, 0x100000ULL, 0x2ULL);
        *(void**)(pLowLevelCtx + 0xd530) = ppLowLevelCB[0];
    }
}

/**
 * @brief Unpacks a 32-bit packed `0xAARRGGBB` color (`edx`) into normalized `float[4]` (`R, G, B, A`)
 *        and updates/binds PS constant buffer slot 0 via `0x2192040`.
 * VMA: 0x21922b0 (146 bytes)
 */
void GXConstantBufferHelper_UpdateColorFromPackedRGBA8(
    uint8_t* pHelper,
    uint8_t* pDeviceCtx,
    uint32_t packedColor,
    uint8_t bDirectLowLevelBind)
{
    alignas(16) float colorRGBA[4];
    colorRGBA[0] = (float)((packedColor >> 16) & 0xFFu) / 255.0f;
    colorRGBA[1] = (float)((packedColor >> 8)  & 0xFFu) / 255.0f;
    colorRGBA[2] = (float)((packedColor >> 0)  & 0xFFu) / 255.0f;
    colorRGBA[3] = (float)((packedColor >> 24) & 0xFFu) / 255.0f;

    GXConstantBufferHelper_UpdateAndBindPSConstantSlot0(pHelper, pDeviceCtx, colorRGBA, bDirectLowLevelBind);
}

/**
 * @brief Updates the constant buffer at `[pHelper+0x1d8]` and binds it to VS constant buffer slot 0
 *        (`[pDeviceCtx+0x680]` or low-level `[pLowLevelCtx+0xc148]`).
 * VMA: 0x2192170 (310 bytes)
 */
void GXConstantBufferHelper_UpdateAndBindVSConstantSlot0(
    uint8_t* pHelper,
    uint8_t* pDeviceCtx,
    const void* pData,
    uint8_t bDirectLowLevelBind)
{
    void* pCB = *(void**)(pHelper + 0x1d8);
    if (!pCB) {
        return;
    }

    sub_26d73f0(pCB, *(void**)(pDeviceCtx + 0x08), 0, pData);

    if (bDirectLowLevelBind != 0) {
        uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
        void** ppLowLevelCB = sub_26d7440(*(void**)(pHelper + 0x1d8));
        pLowLevelCtx[0x7a] |= 0x01;
        MarkConstantBufferStageSlotMask(ppLowLevelCB, 0x10000ULL, 0x1ULL);
        *(void**)(pLowLevelCtx + 0xc148) = ppLowLevelCB[0];
    } else {
        void** ppLowLevelCB = sub_26d7440(*(void**)(pHelper + 0x1d8));
        *(void**)(pDeviceCtx + 0x680) = ppLowLevelCB[0];
        *(uint32_t*)(pDeviceCtx + 0x6f0) = 0;
        if (*(uint32_t*)(pDeviceCtx + 0x6f4) < 1) {
            *(uint32_t*)(pDeviceCtx + 0x6f4) = 1;
        }
        uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
        *(uint64_t*)(pLowLevelCtx + 0x78) |= 0x10000ULL;
        MarkConstantBufferStageSlotMask(ppLowLevelCB, 0x10000ULL, 0x1ULL);
        *(void**)(pLowLevelCtx + 0xc148) = ppLowLevelCB[0];
    }
}

/**
 * @brief Updates the constant buffer at `[pHelper+0x1e0]` and binds it to PS constant buffer slot 0
 *        (`[pDeviceCtx+0x968]` or low-level `[pLowLevelCtx+0xd528]`).
 * VMA: 0x2192040 (294 bytes)
 */
void GXConstantBufferHelper_UpdateAndBindPSConstantSlot0(
    uint8_t* pHelper,
    uint8_t* pDeviceCtx,
    const float* pColorRGBA,
    uint8_t bDirectLowLevelBind)
{
    void* pCB = *(void**)(pHelper + 0x1e0);
    sub_26d73f0(pCB, *(void**)(pDeviceCtx + 0x08), 0, pColorRGBA);

    if (bDirectLowLevelBind != 0) {
        uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
        void** ppLowLevelCB = sub_26d7440(*(void**)(pHelper + 0x1e0));
        pLowLevelCtx[0x197a] |= 0x01;
        MarkConstantBufferStageSlotMask(ppLowLevelCB, 0x100000ULL, 0x1ULL);
        *(void**)(pLowLevelCtx + 0xd528) = ppLowLevelCB[0];
    } else {
        void** ppLowLevelCB = sub_26d7440(*(void**)(pHelper + 0x1e0));
        *(void**)(pDeviceCtx + 0x968) = ppLowLevelCB[0];
        *(uint32_t*)(pDeviceCtx + 0x9d8) = 0;
        if (*(uint32_t*)(pDeviceCtx + 0x9dc) < 1) {
            *(uint32_t*)(pDeviceCtx + 0x9dc) = 1;
        }
        uint8_t* pLowLevelCtx = *(uint8_t**)(pDeviceCtx + 0x08);
        *(uint64_t*)(pLowLevelCtx + 0x1978) |= 0x10000ULL;
        MarkConstantBufferStageSlotMask(ppLowLevelCB, 0x100000ULL, 0x1ULL);
        *(void**)(pLowLevelCtx + 0xd528) = ppLowLevelCB[0];
    }
}

} // namespace GX
