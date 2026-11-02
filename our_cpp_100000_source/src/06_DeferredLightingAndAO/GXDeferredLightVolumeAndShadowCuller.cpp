// ============================================================================
// Bloodborne Engine Decompilation Project (CUSA03173 v1.09 eboot.bin)
// Module 06: 06_DeferredLightingAndAO
// File: GXDeferredLightVolumeAndShadowCuller.cpp
// VMA Range: 0x2290000 .. 0x2297000 (89 functions, 29,387 bytes)
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <initializer_list>
#include <immintrin.h>

#include "gx_direct_vulkan.h"

extern "C" {
uint8_t* sub_2ba2b10();
void sub_20b55b0(const char* file, uint32_t line, const char* fmt, ...);
}

namespace {

static inline void removePointerFromVector(uint8_t* pVecObj, void* targetPtr, size_t beginOff = 0x08, size_t endOff = 0x10) {
    if (pVecObj == nullptr) {
        return;
    }
    void** pBegin = *reinterpret_cast<void***>(pVecObj + beginOff);
    void** pEnd = *reinterpret_cast<void***>(pVecObj + endOff);
    void** pCur = pBegin;
    while (pCur != pEnd) {
        void** pNext = pCur + 1;
        if (*pCur == targetPtr) {
            size_t tailBytes = reinterpret_cast<uint8_t*>(pEnd) - reinterpret_cast<uint8_t*>(pNext);
            if (tailBytes > 0) {
                std::memmove(pCur, pNext, tailBytes);
            }
            *reinterpret_cast<uint8_t**>(pVecObj + endOff) -= 8;
            break;
        }
        pCur = pNext;
    }
}

static inline void releaseRefCountOnlySlot(uint8_t* pOwner, size_t slotOffset) {
    uint8_t* pObj = *reinterpret_cast<uint8_t**>(pOwner + slotOffset);
    if (pObj == nullptr) {
        return;
    }
    uint32_t oldRef = *reinterpret_cast<uint32_t*>(pObj + 0x08);
    uint32_t decStrong = oldRef + 0x7fffu;
    uint32_t newStrong = (oldRef & 0xffff8000u) | (decStrong & 0x7fffu);
    *reinterpret_cast<uint32_t*>(pObj + 0x8) = newStrong;
    if ((decStrong & 0x7fffu) == 0 && (oldRef & 0x40000000u) != 0) {
        void* vtbl1 = *reinterpret_cast<void**>(pObj);
        (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl1) + 0x18))(pObj);
        newStrong = *reinterpret_cast<uint32_t*>(pObj + 0x08) & 0xbfffffffu;
        *reinterpret_cast<uint32_t*>(pObj + 0x08) = newStrong;
    }
    uint32_t decWeak = (newStrong >> 15) + 0x7fffu;
    uint32_t newWeak = (newStrong & 0xc0007fffu) | ((decWeak & 0x7fffu) << 15);
    *reinterpret_cast<uint32_t*>(pObj + 0x08) = newWeak;
    if ((decWeak & 0x7fffu) == 0) {
        *reinterpret_cast<uint32_t*>(pObj + 0x08) = newStrong | 0x3fff8000u;
        void* vtbl2 = *reinterpret_cast<void**>(pObj);
        (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl2) + 0x20))(pObj);
    }
    *reinterpret_cast<void**>(pOwner + slotOffset) = nullptr;
}

static inline void releaseTargetAndRefCountSlot(uint8_t* pOwner, size_t slotOffset) {
    uint8_t* pObj = *reinterpret_cast<uint8_t**>(pOwner + slotOffset);
    if (pObj == nullptr) {
        return;
    }
    void* vtbl0 = *reinterpret_cast<void**>(pObj);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl0) + 0x18))(pObj);
    pObj[0x0b] &= 0xbfu;
    releaseRefCountOnlySlot(pOwner, slotOffset);
}

static inline uint8_t* getDefaultContainerHeap() {
    uint8_t* pHeap = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540670ULL));
    if (pHeap == nullptr) {
        pHeap = sub_2ba2b10();
        *reinterpret_cast<uint8_t**>(GX_VMA(0x5540670ULL)) = pHeap;
    }
    void* vtbl = *reinterpret_cast<void**>(pHeap);
    uint8_t flagsBuf[8] = {};
    (*reinterpret_cast<void (**)(void*, void*, uint32_t)>(static_cast<uint8_t*>(vtbl) + 0x20))(flagsBuf, pHeap, 0);
    if ((flagsBuf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
    return pHeap;
}

} // namespace

extern "C" {

void sub_eaac10(void* pObj);
void sub_ea8d10(void* pObj);
void sub_22c5a60(void* pSubTarget, uint32_t flag);
void sub_2192660(void* pResource);
void* sub_207b720(void* pMemory);
void sub_21cad60(void* pObj);
void sub_221b290(void* pSub5860);
void sub_e30b30(void* pSub440);
void sub_e46450(void* pSub78c8);
void sub_21a6a00(void* pTexture2D);
void sub_21a7c20(void* pStructuredBuffer);
void sub_221a630(void* pSub5860);
void sub_22c2e10(void* pSubTarget);
void sub_2bbe588(void* pMemory);
uint8_t* sub_2ba2b10();
void sub_20b55b0(const char* file, uint32_t line, const char* fmt, ...);
void sub_22c1050(void* pSubTarget, void* pArg2);
void sub_221a2b0(void* pSub5860, void* pArg2);
void sub_e46410(void* pSub78c8);
void sub_2bbe978(void* pExceptionObj, uint32_t flag);
void sub_e47ab0(void* pQueue, void* pJob);

static inline void freeConstantBufferSlot(uint8_t* selfBytes, size_t slotOffset) {
    uint8_t* pWrapper = *reinterpret_cast<uint8_t**>(selfBytes + slotOffset);
    if (pWrapper == nullptr) {
        return;
    }
    void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* pRes = *reinterpret_cast<void**>(pWrapper);
    if (pRes != nullptr) {
        sub_2192660(pRes);
        *reinterpret_cast<void**>(pWrapper) = nullptr;
    }
    void* heapVtbl = *reinterpret_cast<void**>(pHeap);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pWrapper);
    *reinterpret_cast<void**>(selfBytes + slotOffset) = nullptr;
}

static inline void freeViaLookupAllocatorSlot(uint8_t* selfBytes, size_t slotOffset) {
    void* pObj = *reinterpret_cast<void**>(selfBytes + slotOffset);
    if (pObj == nullptr) {
        return;
    }
    void* pAlloc = sub_207b720(pObj);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pObj);
    *reinterpret_cast<void**>(selfBytes + slotOffset) = nullptr;
}

static inline void freeShaderLayoutHolderSlot(uint8_t* selfBytes, size_t slotOffset) {
    uint8_t* pHolder = *reinterpret_cast<uint8_t**>(selfBytes + slotOffset);
    if (pHolder == nullptr) {
        return;
    }
    void* pMainAlloc = sub_207b720(pHolder);
    void* pPtr18 = *reinterpret_cast<void**>(pHolder + 0x18);
    if (pPtr18 != nullptr) {
        void* pAlloc0 = *reinterpret_cast<void**>(pHolder + 0x00);
        void* vtbl0 = *reinterpret_cast<void**>(pAlloc0);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl0) + 0x70))(pAlloc0, pPtr18);
        *reinterpret_cast<void**>(pHolder + 0x18) = nullptr;
    }
    void* pPtr10 = *reinterpret_cast<void**>(pHolder + 0x10);
    if (pPtr10 != nullptr) {
        void* pAlloc8 = *reinterpret_cast<void**>(pHolder + 0x08);
        void* vtbl8 = *reinterpret_cast<void**>(pAlloc8);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl8) + 0x70))(pAlloc8, pPtr10);
        *reinterpret_cast<void**>(pHolder + 0x10) = nullptr;
    }
    void* mainVtbl = *reinterpret_cast<void**>(pMainAlloc);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(mainVtbl) + 0x70))(pMainAlloc, pHolder);
    *reinterpret_cast<void**>(selfBytes + slotOffset) = nullptr;
}

// ============================================================================
// 0x2296aa0: sub_2296aa0 (3,845 bytes)
// Shuts down and releases all deferred lighting, state, and texture resources
// ============================================================================
void sub_2296aa0(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    removePointerFromVector(*reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x5f8), selfBytes + 0x08);

    releaseRefCountOnlySlot(selfBytes, 0x7988);
    releaseTargetAndRefCountSlot(selfBytes, 0x7970);
    releaseTargetAndRefCountSlot(selfBytes, 0x7978);
    releaseTargetAndRefCountSlot(selfBytes, 0x7948);
    releaseTargetAndRefCountSlot(selfBytes, 0x7950);
    releaseTargetAndRefCountSlot(selfBytes, 0x7958);
    releaseTargetAndRefCountSlot(selfBytes, 0x7960);
    releaseTargetAndRefCountSlot(selfBytes, 0x7968);

    void* pSub6088 = *reinterpret_cast<void**>(selfBytes + 0x6088);
    if (pSub6088 != nullptr) {
        void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        sub_eaac10(pSub6088);
        void* heapVtbl = *reinterpret_cast<void**>(pHeap);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pSub6088);
        *reinterpret_cast<void**>(selfBytes + 0x6088) = nullptr;
    }

    void* pSub6080 = *reinterpret_cast<void**>(selfBytes + 0x6080);
    if (pSub6080 != nullptr) {
        void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        sub_ea8d10(pSub6080);
        void* heapVtbl = *reinterpret_cast<void**>(pHeap);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pSub6080);
        *reinterpret_cast<void**>(selfBytes + 0x6080) = nullptr;
    }

    if (selfBytes[0xb0] != 0 || selfBytes[0xb2] != 0 || selfBytes[0xb3] != 0) {
        pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        removePointerFromVector(*reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x5f8), selfBytes + 0x68);
    }
    sub_22c5a60(selfBytes + 0x60, 0);

    if (selfBytes[0x2cb0] != 0 || selfBytes[0x2cb2] != 0 || selfBytes[0x2cb3] != 0) {
        pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        removePointerFromVector(*reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x5f8), selfBytes + 0x2c68);
    }
    sub_22c5a60(selfBytes + 0x2c60, 0);

    for (size_t i = 0; i < 12; ++i) {
        freeConstantBufferSlot(selfBytes, 0x6688 + i * 8);
    }
    for (size_t i = 0; i < 12; ++i) {
        freeConstantBufferSlot(selfBytes, 0x6618 + i * 8);
    }
    freeConstantBufferSlot(selfBytes, 0x66e8);
    freeConstantBufferSlot(selfBytes, 0x6678);
    freeConstantBufferSlot(selfBytes, 0x6680);

    freeViaLookupAllocatorSlot(selfBytes, 0x65d0);
    freeViaLookupAllocatorSlot(selfBytes, 0x65c8);
    freeViaLookupAllocatorSlot(selfBytes, 0x65c0);
    freeViaLookupAllocatorSlot(selfBytes, 0x65b8);
    freeViaLookupAllocatorSlot(selfBytes, 0x65b0);
    freeViaLookupAllocatorSlot(selfBytes, 0x65a8);
    freeViaLookupAllocatorSlot(selfBytes, 0x65a0);
    freeViaLookupAllocatorSlot(selfBytes, 0x6598);

    freeViaLookupAllocatorSlot(selfBytes, 0x6600);
    freeViaLookupAllocatorSlot(selfBytes, 0x65f8);
    freeViaLookupAllocatorSlot(selfBytes, 0x65f0);
    freeViaLookupAllocatorSlot(selfBytes, 0x65e8);
    freeViaLookupAllocatorSlot(selfBytes, 0x65e0);
    freeViaLookupAllocatorSlot(selfBytes, 0x65d8);
    freeViaLookupAllocatorSlot(selfBytes, 0x6608);
    freeViaLookupAllocatorSlot(selfBytes, 0x6610);

    for (size_t i = 0; i < 256; ++i) {
        freeViaLookupAllocatorSlot(selfBytes, 0x6e70 + i * 8);
    }
    *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6cf0ULL)) = 0;

    uint8_t* pSub6090 = *reinterpret_cast<uint8_t**>(selfBytes + 0x6090);
    if (pSub6090 != nullptr) {
        void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        void* pInner = *reinterpret_cast<void**>(pSub6090);
        if (pInner != nullptr) {
            sub_21cad60(pInner);
            void* heapVtbl = *reinterpret_cast<void**>(pHeap);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pInner);
            *reinterpret_cast<void**>(pSub6090) = nullptr;
        }
        void* heapVtbl = *reinterpret_cast<void**>(pHeap);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pSub6090);
        *reinterpret_cast<void**>(selfBytes + 0x6090) = nullptr;
    }

    sub_221b290(selfBytes + 0x5860);
    freeShaderLayoutHolderSlot(selfBytes, 0x60a8);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6570) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6568) = 0;
    std::memset(selfBytes + 0x60a0, 0, 0x4b8);

    uint8_t* pSub6070 = *reinterpret_cast<uint8_t**>(selfBytes + 0x6070);
    if (pSub6070 != nullptr) {
        removePointerFromVector(pSub6070, selfBytes, 0x10, 0x18);
        pSub6070 = *reinterpret_cast<uint8_t**>(selfBytes + 0x6070);
        *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6ce8ULL)) = 0;
        void* pSub440 = *reinterpret_cast<void**>(pSub6070 + 0x440);
        if (pSub440 != nullptr) {
            void* pAlloc440 = sub_207b720(pSub440);
            sub_e30b30(pSub440);
            void* vtbl440 = *reinterpret_cast<void**>(pAlloc440);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl440) + 0x70))(pAlloc440, pSub440);
            *reinterpret_cast<void**>(pSub6070 + 0x440) = nullptr;
            pSub6070 = *reinterpret_cast<uint8_t**>(selfBytes + 0x6070);
        }
        if (pSub6070 != nullptr) {
            void* pAlloc6070 = sub_207b720(pSub6070);
            void* vtbl6070 = *reinterpret_cast<void**>(pSub6070);
            (*reinterpret_cast<void (**)(void*)>(vtbl6070))(pSub6070);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc6070);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc6070, pSub6070);
            *reinterpret_cast<void**>(selfBytes + 0x6070) = nullptr;
        }
    }

    void** pNodeCur = *reinterpret_cast<void***>(selfBytes + 0x30);
    void** pNodeEnd = *reinterpret_cast<void***>(selfBytes + 0x38);
    if (pNodeCur != pNodeEnd) {
        while (pNodeCur != pNodeEnd) {
            void* pNode = *pNodeCur;
            if (pNode != nullptr) {
                void* pAllocNode = sub_207b720(pNode);
                void* nodeVtbl = *reinterpret_cast<void**>(pNode);
                (*reinterpret_cast<void (**)(void*)>(nodeVtbl))(pNode);
                void* allocVtbl = *reinterpret_cast<void**>(pAllocNode);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAllocNode, pNode);
                pNodeEnd = *reinterpret_cast<void***>(selfBytes + 0x38);
            }
            ++pNodeCur;
        }
        pNodeCur = *reinterpret_cast<void***>(selfBytes + 0x30);
    }
    *reinterpret_cast<void***>(selfBytes + 0x38) = pNodeCur;

    void* pNode20 = *reinterpret_cast<void**>(selfBytes + 0x20);
    if (pNode20 != nullptr) {
        void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        void* nodeVtbl = *reinterpret_cast<void**>(pNode20);
        (*reinterpret_cast<void (**)(void*)>(nodeVtbl))(pNode20);
        void* heapVtbl = *reinterpret_cast<void**>(pHeap);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pNode20);
        *reinterpret_cast<void**>(selfBytes + 0x20) = nullptr;
    }

    void* pSub6078 = *reinterpret_cast<void**>(selfBytes + 0x6078);
    if (pSub6078 != nullptr) {
        void* pAlloc6078 = sub_207b720(pSub6078);
        void* vtbl6078 = *reinterpret_cast<void**>(pSub6078);
        (*reinterpret_cast<void (**)(void*)>(vtbl6078))(pSub6078);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc6078);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc6078, pSub6078);
        *reinterpret_cast<void**>(selfBytes + 0x6078) = nullptr;
    }

    for (size_t i = 0; i < 4; ++i) {
        uint8_t* pDitherTex = selfBytes + 0x7720 + i * 0x60;
        void* texVtbl = *reinterpret_cast<void**>(pDitherTex);
        (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(texVtbl) + 0x90))(pDitherTex);
    }

    uint8_t* pObj78a0 = *reinterpret_cast<uint8_t**>(selfBytes + 0x78a0);
    if (pObj78a0 != nullptr) {
        int32_t oldCount = *reinterpret_cast<int32_t*>(pObj78a0 + 0x08);
        *reinterpret_cast<int32_t*>(pObj78a0 + 0x08) = oldCount - 1;
        if (oldCount <= 1) {
            void* vtbl = *reinterpret_cast<void**>(pObj78a0);
            (*reinterpret_cast<void (**)(void*)>(vtbl))(pObj78a0);
        }
        *reinterpret_cast<void**>(selfBytes + 0x78a0) = nullptr;
    }
}

// ============================================================================
// 0x2296a90: sub_2296a90 (9 bytes)
// Non-virtual thunk to sub_2296130 (adjusts this by -8)
// ============================================================================
void sub_2296130(void* pSelf);

void sub_2296a90(void* pSelf) {
    sub_2296130(static_cast<uint8_t*>(pSelf) - 8);
}

// ============================================================================
// 0x2296130: sub_2296130 (2,393 bytes)
// Base destructor of GXDeferredLightingManager
// ============================================================================
static inline void destroyEmbeddedTexture2D(uint8_t* pTexObj) {
    *reinterpret_cast<uint64_t*>(pTexObj) = GX_VMA(0x53b6a20ULL);
    sub_21a6a00(pTexObj);
    uint8_t* pShared = *reinterpret_cast<uint8_t**>(pTexObj + 0x40);
    if (pShared != nullptr) {
        if (__sync_fetch_and_add(reinterpret_cast<int32_t*>(pShared + 0x10), -1) == 1) {
            void* vtbl = *reinterpret_cast<void**>(pShared);
            void* pAlloc = *reinterpret_cast<void**>(pShared + 0x08);
            (*reinterpret_cast<void (**)(void*)>(vtbl))(pShared);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pShared);
        }
        *reinterpret_cast<void**>(pTexObj + 0x40) = nullptr;
    }
    *reinterpret_cast<uint64_t*>(pTexObj) = GX_VMA(0x53b6b00ULL);
    void* pRes = *reinterpret_cast<void**>(pTexObj + 0x30);
    if (pRes != nullptr) {
        sub_2192660(pRes);
        *reinterpret_cast<void**>(pTexObj + 0x30) = nullptr;
    }
}

void sub_2296130(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x00) = GX_VMA(0x53b8040ULL);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x08) = GX_VMA(0x53b8070ULL);

    sub_2296aa0(selfBytes);

    if (*reinterpret_cast<uint64_t*>(selfBytes + 0x79b8) >= 8) {
        void* pStrBuf = *reinterpret_cast<void**>(selfBytes + 0x79a0);
        void* pStrAlloc = *reinterpret_cast<void**>(selfBytes + 0x79c0);
        void* allocVtbl = *reinterpret_cast<void**>(pStrAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pStrAlloc, pStrBuf);
    }
    *reinterpret_cast<uint64_t*>(selfBytes + 0x79b8) = 7;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x79b0) = 0;
    *reinterpret_cast<uint16_t*>(selfBytes + 0x79a0) = 0;

    releaseRefCountOnlySlot(selfBytes, 0x7988);
    releaseRefCountOnlySlot(selfBytes, 0x7978);
    releaseRefCountOnlySlot(selfBytes, 0x7970);
    releaseRefCountOnlySlot(selfBytes, 0x7968);
    releaseRefCountOnlySlot(selfBytes, 0x7960);
    releaseRefCountOnlySlot(selfBytes, 0x7958);
    releaseRefCountOnlySlot(selfBytes, 0x7950);
    releaseRefCountOnlySlot(selfBytes, 0x7948);

    sub_e46450(selfBytes + 0x78c8);

    uint8_t* pObj78a0 = *reinterpret_cast<uint8_t**>(selfBytes + 0x78a0);
    if (pObj78a0 != nullptr) {
        int32_t oldCount = *reinterpret_cast<int32_t*>(pObj78a0 + 0x08);
        *reinterpret_cast<int32_t*>(pObj78a0 + 0x08) = oldCount - 1;
        if (oldCount <= 1) {
            void* vtbl = *reinterpret_cast<void**>(pObj78a0);
            (*reinterpret_cast<void (**)(void*)>(vtbl))(pObj78a0);
        }
        *reinterpret_cast<void**>(selfBytes + 0x78a0) = nullptr;
    }

    destroyEmbeddedTexture2D(selfBytes + 0x7840);
    destroyEmbeddedTexture2D(selfBytes + 0x77e0);
    destroyEmbeddedTexture2D(selfBytes + 0x7780);
    destroyEmbeddedTexture2D(selfBytes + 0x7720);

    uint8_t* pBuf76b0 = selfBytes + 0x76b0;
    *reinterpret_cast<uint64_t*>(pBuf76b0) = GX_VMA(0x53b6ba0ULL);
    sub_21a7c20(pBuf76b0);
    uint8_t* pShared76f0 = *reinterpret_cast<uint8_t**>(selfBytes + 0x76f0);
    if (pShared76f0 != nullptr) {
        if (__sync_fetch_and_add(reinterpret_cast<int32_t*>(pShared76f0 + 0x10), -1) == 1) {
            void* vtbl = *reinterpret_cast<void**>(pShared76f0);
            void* pAlloc = *reinterpret_cast<void**>(pShared76f0 + 0x08);
            (*reinterpret_cast<void (**)(void*)>(vtbl))(pShared76f0);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pShared76f0);
        }
        *reinterpret_cast<void**>(selfBytes + 0x76f0) = nullptr;
    }
    *reinterpret_cast<uint64_t*>(pBuf76b0) = GX_VMA(0x53b6b00ULL);
    void* pRes76e0 = *reinterpret_cast<void**>(selfBytes + 0x76e0);
    if (pRes76e0 != nullptr) {
        sub_2192660(pRes76e0);
        *reinterpret_cast<void**>(selfBytes + 0x76e0) = nullptr;
    }

    void* pVec7690 = *reinterpret_cast<void**>(selfBytes + 0x7690);
    if (pVec7690 != nullptr) {
        void* pAlloc = *reinterpret_cast<void**>(selfBytes + 0x76a8);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pVec7690);
        *reinterpret_cast<uint64_t*>(selfBytes + 0x76a0) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x7698) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x7690) = 0;
    }

    freeShaderLayoutHolderSlot(selfBytes, 0x6590);
    *reinterpret_cast<void**>(selfBytes + 0x6590) = nullptr;
    freeShaderLayoutHolderSlot(selfBytes, 0x60a8);
    *reinterpret_cast<void**>(selfBytes + 0x60a8) = nullptr;

    sub_221a630(selfBytes + 0x5860);
    sub_22c2e10(selfBytes + 0x2c60);
    sub_22c2e10(selfBytes + 0x60);

    void* pVec30 = *reinterpret_cast<void**>(selfBytes + 0x30);
    if (pVec30 != nullptr) {
        void* pAlloc = *reinterpret_cast<void**>(selfBytes + 0x48);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pVec30);
        *reinterpret_cast<uint64_t*>(selfBytes + 0x40) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x38) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x30) = 0;
    }
}

// ============================================================================
// 0x2296100: sub_2296100 (35 bytes)
// Non-virtual deleting destructor thunk (adjusts this by -8)
// ============================================================================
void sub_2296100(void* pSelf) {
    uint8_t* base = static_cast<uint8_t*>(pSelf) - 8;
    sub_2296130(base);
    sub_2bbe588(base);
}

// ============================================================================
// 0x22960e0: sub_22960e0 (28 bytes)
// Deleting destructor of GXDeferredLightingManager
// ============================================================================
void sub_22960e0(void* pSelf) {
    sub_2296130(pSelf);
    sub_2bbe588(pSelf);
}

// ============================================================================
// 0x22958d0: sub_22958d0 (2,056 bytes)
// Constructor of GXDeferredLightingManager
// ============================================================================
void sub_22958d0(void* pSelf, void* pOwner) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x00) = GX_VMA(0x53b8040ULL);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x08) = GX_VMA(0x53b8070ULL);
    *reinterpret_cast<void**>(selfBytes + 0x10) = pOwner;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x18) = 0xff003020u;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x20) = 0;

    uint8_t* pHeap0 = getDefaultContainerHeap();
    *reinterpret_cast<uint64_t*>(selfBytes + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x30) = 0;
    *reinterpret_cast<uint8_t**>(selfBytes + 0x48) = pHeap0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x50) = 0;

    sub_22c1050(selfBytes + 0x60, pOwner);
    sub_22c1050(selfBytes + 0x2c60, pOwner);
    sub_221a2b0(selfBytes + 0x5860, pOwner);

    selfBytes[0x6098] = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6090) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6088) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6080) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6078) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6070) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x66e8) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6680) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6678) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x60a8) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x60a0) = 0;

    std::memset(selfBytes + 0x6530, 0, 0xe8);
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7670) = 0x41300000u;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7674) = 0xbba3d70a00000000ULL;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x767c) = 0xbf4ccccdc1200000ULL;

    uint8_t* pHeap1 = getDefaultContainerHeap();
    *reinterpret_cast<uint64_t*>(selfBytes + 0x76a0) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7698) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7690) = 0;
    *reinterpret_cast<uint8_t**>(selfBytes + 0x76a8) = pHeap1;

    const __m128 oneVec = _mm_set1_ps(1.0f);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x76b0) = GX_VMA(0x53b6ba0ULL);
    *reinterpret_cast<uint32_t*>(selfBytes + 0x76b8) = 0;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x76c0) = 0x3f800000u;
    _mm_storeu_ps(reinterpret_cast<float*>(selfBytes + 0x76d0), oneVec);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x76e0) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x76f0) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x76f8) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7700) = 0;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7708) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7710) = 0;

    for (size_t i = 0; i < 4; ++i) {
        uint8_t* pTex = selfBytes + 0x7720 + i * 0x60;
        *reinterpret_cast<uint64_t*>(pTex + 0x00) = GX_VMA(0x53b6a20ULL);
        *reinterpret_cast<uint32_t*>(pTex + 0x08) = 0;
        *reinterpret_cast<uint32_t*>(pTex + 0x10) = 0x3f800000u;
        _mm_storeu_ps(reinterpret_cast<float*>(pTex + 0x20), oneVec);
        *reinterpret_cast<uint64_t*>(pTex + 0x30) = 0;
        *reinterpret_cast<uint64_t*>(pTex + 0x40) = 0;
        *reinterpret_cast<uint64_t*>(pTex + 0x48) = 0;
        *reinterpret_cast<uint64_t*>(pTex + 0x50) = 0;
        *reinterpret_cast<uint32_t*>(pTex + 0x58) = 0;
    }

    uint32_t lightGridW = 0x80u;
    uint32_t lightGridH = 0x80u;
    gx_direct_vk::GetLightGridOverride(&lightGridW, &lightGridH);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x78a0) = 0;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x78a8) = lightGridW;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x78ac) = lightGridH;
    selfBytes[0x78b0] = 0;
    selfBytes[0x78b1] = 1;
    selfBytes[0x78b2] = 1;
    selfBytes[0x78b3] = 0;
    selfBytes[0x78b4] = 1;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x78b8) = 0;
    selfBytes[0x78bc] = 0;
    selfBytes[0x78bd] = 0;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x78be) = 0x01010101u;
    *reinterpret_cast<uint16_t*>(selfBytes + 0x78c2) = 0x0100u;

    sub_e46410(selfBytes + 0x78c8);
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7908) = 0;
    *reinterpret_cast<uint16_t*>(selfBytes + 0x7980) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7978) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7970) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7968) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7960) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7958) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7950) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7948) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7990) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7988) = 0;

    uint8_t* pHeap2 = getDefaultContainerHeap();
    *reinterpret_cast<uint8_t**>(selfBytes + 0x79c0) = pHeap2;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x79b8) = 7;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x79b0) = 0;
    *reinterpret_cast<uint16_t*>(selfBytes + 0x79a0) = 0;
    selfBytes[0x79c8] = 1;
    selfBytes[0x79d0] = 0;

    for (size_t i = 0; i < 12; ++i) {
        *reinterpret_cast<uint64_t*>(selfBytes + 0x6618 + i * 8) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x6688 + i * 8) = 0;
    }
    std::memset(selfBytes + 0x60b0, 0, 0x480);
    std::memset(selfBytes + 0x6e70, 0, 0x800);

    *reinterpret_cast<uint64_t*>(selfBytes + 0x7910) = 0x200000ULL;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7918) = 0x2710u;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x791c) = 0x5dcu;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7920) = 0x400u;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7924) = 0x100u;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7928) = 0x200u;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x792c) = 0;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7930) = 0x3d8600u;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x7934) = 0x400u;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7938) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7940) = 0x1000000ULL;

    static const uint32_t kDebugColorPalette64[64] = {
        0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u,
        0x3f800000u, 0x00000000u, 0x00000000u, 0x3f800000u,
        0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
        0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u,
        0x3f000000u, 0x3f000000u, 0x3f000000u, 0x3f800000u,
        0x3f800000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
        0x3f800000u, 0x00000000u, 0x3f800000u, 0x3f800000u,
        0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u,
        0x3e4ccccdu, 0x3e4ccccdu, 0x3e4ccccdu, 0x3f800000u,
        0x3f800000u, 0x3f000000u, 0x00000000u, 0x3f800000u,
        0x3f800000u, 0x00000000u, 0x3f000000u, 0x3f800000u,
        0x00000000u, 0x3f800000u, 0x3f000000u, 0x3f800000u,
        0x3f000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
        0x3f000000u, 0x00000000u, 0x3f800000u, 0x3f800000u,
        0x00000000u, 0x3f000000u, 0x3f800000u, 0x3f800000u,
        0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u
    };
    std::memcpy(selfBytes + 0x7a20, kDebugColorPalette64, sizeof(kDebugColorPalette64));
    *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL)) = selfBytes;
}

// ============================================================================
// 0x2295770: sub_2295770 (350 bytes)
// Reallocates intrusive ref-counted listener pointer vector to newCapacity
// ============================================================================
void sub_2295770(void* pVec, size_t newCapacity) {
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    void* pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    uint8_t** pNewBuf = static_cast<uint8_t**>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAlloc, newCapacity * 8, 8));
    if (pNewBuf == nullptr) {
        uint64_t fsBase = 0;
        fsBase = gx_guest_tls_base();
        if (*reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) != 0) {
            sub_2bbe978(reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL))), 1);
            return;
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    uint8_t** pOldBegin = *reinterpret_cast<uint8_t***>(vecBytes + 0x08);
    uint8_t** pOldEnd = *reinterpret_cast<uint8_t***>(vecBytes + 0x10);
    if (pOldBegin != pOldEnd) {
        uint8_t** pSrc = pOldBegin;
        uint8_t** pDst = pNewBuf;
        while (pSrc != pOldEnd) {
            if (pDst != nullptr) {
                uint8_t* pItem = *pSrc;
                *pDst = pItem;
                if (pItem != nullptr) {
                    ++(*reinterpret_cast<uint32_t*>(pItem + 0x08));
                }
            }
            ++pDst;
            ++pSrc;
        }
    }

    if (pOldBegin != nullptr) {
        for (uint8_t** pCur = pOldBegin; pCur != pOldEnd; ++pCur) {
            uint8_t* pItem = *pCur;
            if (pItem != nullptr) {
                if (--(*reinterpret_cast<uint32_t*>(pItem + 0x08)) == 0) {
                    void* pItemAlloc = sub_207b720(pItem);
                    void* itemVtbl = *reinterpret_cast<void**>(pItem);
                    (*reinterpret_cast<void (**)(void*)>(itemVtbl))(pItem);
                    void* itemAllocVtbl = *reinterpret_cast<void**>(pItemAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(itemAllocVtbl) + 0x70))(pItemAlloc, pItem);
                }
                *pCur = nullptr;
            }
        }
        pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
        allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pOldBegin);
    }

    ptrdiff_t count = pOldEnd - pOldBegin;
    *reinterpret_cast<uint8_t***>(vecBytes + 0x18) = pNewBuf + newCapacity;
    *reinterpret_cast<uint8_t***>(vecBytes + 0x10) = pNewBuf + count;
    *reinterpret_cast<uint8_t***>(vecBytes + 0x08) = pNewBuf;
}

// ============================================================================
// 0x2295760, 0x2295750, 0x2295740: virtual stubs returning 0 (3 bytes each)
// ============================================================================
uint32_t sub_2295760() { return 0; }
uint32_t sub_2295750() { return 0; }
uint32_t sub_2295740() { return 0; }

// ============================================================================
// 0x22956b0: sub_22956b0 (133 bytes)
// Executes deferred listener command task (vtable 0x5361010) and frees self
// ============================================================================
void sub_22956b0(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void* pHeap688 = *reinterpret_cast<void**>(GX_VMA(0x5540688ULL));

    void* pCtx10 = *reinterpret_cast<void**>(selfBytes + 0x10);
    void* vtbl10 = *reinterpret_cast<void**>(pCtx10);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl10) + 0x78))(pCtx10);

    void* pTarget08 = *reinterpret_cast<void**>(selfBytes + 0x08);
    void* pArg10 = *reinterpret_cast<void**>(selfBytes + 0x10);
    void* vtbl08 = *reinterpret_cast<void**>(pTarget08);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl08) + 0x10))(pTarget08, pArg10);

    pCtx10 = *reinterpret_cast<void**>(selfBytes + 0x10);
    vtbl10 = *reinterpret_cast<void**>(pCtx10);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl10) + 0x80))(pCtx10);

    pCtx10 = *reinterpret_cast<void**>(selfBytes + 0x10);
    if (pCtx10 != nullptr) {
        vtbl10 = *reinterpret_cast<void**>(pCtx10);
        (*reinterpret_cast<void (**)(void*)>(vtbl10))(pCtx10);
        void* heapVtbl = *reinterpret_cast<void**>(pHeap688);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap688, pCtx10);
        *reinterpret_cast<void**>(selfBytes + 0x10) = nullptr;
    }

    void* pFreeHeap = *reinterpret_cast<void**>(GX_VMA(0x5540688ULL));
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x5361010ULL);
    void* freeVtbl = *reinterpret_cast<void**>(pFreeHeap);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(freeVtbl) + 0x70))(pFreeHeap, selfBytes);
}

// ============================================================================
// 0x2295500: sub_2295500 (276 bytes)
// Dispatches render callback across registered listeners [pSelf+0x18 .. +0x20)
// ============================================================================
void sub_2295500(void* pSelf, void* pContext) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void** pCur = *reinterpret_cast<void***>(selfBytes + 0x18);
    void** pEnd = *reinterpret_cast<void***>(selfBytes + 0x20);
    while (pCur != pEnd) {
        void* ctxVtbl = *reinterpret_cast<void**>(pContext);
        uint8_t mode = (*reinterpret_cast<uint8_t (**)(void*)>(static_cast<uint8_t*>(ctxVtbl) + 0x48))(pContext);
        if (mode == 1 && *reinterpret_cast<uint8_t*>(GX_VMA(0x513ad19ULL)) != 0 && *reinterpret_cast<uint8_t*>(GX_VMA(0x513ad1bULL)) != 0) {
            void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540688ULL));
            void* heapVtbl = *reinterpret_cast<void**>(pHeap);
            uint8_t* pJob = static_cast<uint8_t*>(
                (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 0x18, 8));
            if (pJob != nullptr) {
                void* pListener = *pCur;
                *reinterpret_cast<uint64_t*>(pJob + 0x00) = GX_VMA(0x5361010ULL);
                *reinterpret_cast<void**>(pJob + 0x08) = pListener;
                void* pCloneHeap = *reinterpret_cast<void**>(GX_VMA(0x5540688ULL));
                ctxVtbl = *reinterpret_cast<void**>(pContext);
                *reinterpret_cast<void**>(pJob + 0x10) =
                    (*reinterpret_cast<void* (**)(void*, void*)>(static_cast<uint8_t*>(ctxVtbl) + 0x70))(pContext, pCloneHeap);
            }
            uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
            uint8_t* pQueueOwner = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x278);
            sub_e47ab0(pQueueOwner + 0x18, pJob);
            if (pGlobal6c8[0x2e0] != 0) {
                *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1;
            }
            pGlobal6c8[0x2a9] = 1;
        } else {
            void* pListener = *pCur;
            void* lisVtbl = *reinterpret_cast<void**>(pListener);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(lisVtbl) + 0x10))(pListener, pContext);
        }
        ++pCur;
    }
}

// ============================================================================
// 0x22953f0: sub_22953f0 (269 bytes)
// Unregisters a ref-counted listener from [pSelf+0x18 .. +0x20)
// ============================================================================
bool sub_22953f0(void* pSelf, void* pListener) {
    if (pListener == nullptr) {
        return false;
    }
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t** pCur = *reinterpret_cast<uint8_t***>(selfBytes + 0x18);
    uint8_t** pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
    while (pCur != pEnd) {
        uint8_t** pNext = pCur + 1;
        if (*pCur == pListener) {
            pCur = pNext;
            while (pCur != pEnd) {
                uint8_t* pPrevObj = pCur[-1];
                uint8_t* pNextObj = pCur[0];
                if (pPrevObj != pNextObj) {
                    if (pPrevObj != nullptr) {
                        if (--(*reinterpret_cast<uint32_t*>(pPrevObj + 0x08)) == 0) {
                            void* pAlloc = sub_207b720(pPrevObj);
                            void* vtbl = *reinterpret_cast<void**>(pPrevObj);
                            (*reinterpret_cast<void (**)(void*)>(vtbl))(pPrevObj);
                            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pPrevObj);
                            pNextObj = pCur[0];
                        }
                        pCur[-1] = nullptr;
                    }
                    pCur[-1] = pNextObj;
                    if (pNextObj != nullptr) {
                        ++(*reinterpret_cast<uint32_t*>(pNextObj + 0x08));
                    }
                }
                ++pCur;
            }
            pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
            uint8_t* pTailObj = pEnd[-1];
            if (pTailObj != nullptr) {
                if (--(*reinterpret_cast<uint32_t*>(pTailObj + 0x08)) == 0) {
                    void* pAlloc = sub_207b720(pTailObj);
                    void* vtbl = *reinterpret_cast<void**>(pTailObj);
                    (*reinterpret_cast<void (**)(void*)>(vtbl))(pTailObj);
                    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pTailObj);
                }
                pEnd[-1] = nullptr;
                pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
            }
            *reinterpret_cast<uint8_t***>(selfBytes + 0x20) = pEnd - 1;
            return true;
        }
        pCur = pNext;
    }
    return false;
}

// ============================================================================
// 0x2295260: sub_2295260 (394 bytes)
// Registers a unique ref-counted listener into [pSelf+0x18 .. +0x20)
// ============================================================================
bool sub_2295260(void* pSelf, void* pListener) {
    if (pListener == nullptr) {
        return false;
    }
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t** pBegin = *reinterpret_cast<uint8_t***>(selfBytes + 0x18);
    uint8_t** pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
    for (uint8_t** pCur = pBegin; pCur != pEnd; ++pCur) {
        if (*pCur == pListener) {
            return false;
        }
    }

    uint8_t* pLisBytes = static_cast<uint8_t*>(pListener);
    ++(*reinterpret_cast<uint32_t*>(pLisBytes + 0x08));

    uint8_t** pCap = *reinterpret_cast<uint8_t***>(selfBytes + 0x28);
    if (pEnd == pCap) {
        size_t oldSize = static_cast<size_t>(pEnd - pBegin);
        size_t half = oldSize >> 1;
        size_t maxCap = 0x1fffffffffffffffULL;
        size_t newCap = (maxCap - half >= oldSize) ? (oldSize + half) : 0;
        if (newCap < oldSize + 1) {
            newCap = oldSize + 1;
        }
        sub_2295770(selfBytes + 0x10, newCap);
        pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
    }

    if (pEnd != nullptr) {
        *pEnd = pLisBytes;
        ++(*reinterpret_cast<uint32_t*>(pLisBytes + 0x08));
    }
    *reinterpret_cast<uint8_t***>(selfBytes + 0x20) = pEnd + 1;

    if (--(*reinterpret_cast<uint32_t*>(pLisBytes + 0x08)) == 0) {
        void* pAlloc = sub_207b720(pLisBytes);
        void* vtbl = *reinterpret_cast<void**>(pLisBytes);
        (*reinterpret_cast<void (**)(void*)>(vtbl))(pLisBytes);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pLisBytes);
    }
    return true;
}

// ============================================================================
// 0x2295190: sub_2295190 (194 bytes)
// Base destructor of listener container (vtable 0x53b8010)
// ============================================================================
void sub_2295190(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x53b8010ULL);
    uint8_t** pBegin = *reinterpret_cast<uint8_t***>(selfBytes + 0x18);
    if (pBegin != nullptr) {
        uint8_t** pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
        for (uint8_t** pCur = pBegin; pCur != pEnd; ++pCur) {
            uint8_t* pItem = *pCur;
            if (pItem != nullptr) {
                if (--(*reinterpret_cast<uint32_t*>(pItem + 0x08)) == 0) {
                    void* pAlloc = sub_207b720(pItem);
                    void* vtbl = *reinterpret_cast<void**>(pItem);
                    (*reinterpret_cast<void (**)(void*)>(vtbl))(pItem);
                    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pItem);
                }
                *pCur = nullptr;
            }
        }
        pBegin = *reinterpret_cast<uint8_t***>(selfBytes + 0x18);
        void* pVecAlloc = *reinterpret_cast<void**>(selfBytes + 0x30);
        void* vecAllocVtbl = *reinterpret_cast<void**>(pVecAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vecAllocVtbl) + 0x70))(pVecAlloc, pBegin);
        *reinterpret_cast<uint64_t*>(selfBytes + 0x28) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(selfBytes + 0x18) = 0;
    }
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x53b7cf0ULL);
}

// ============================================================================
// 0x22950e0: sub_22950e0 (165 bytes)
// Deleting destructor of listener container (vtable 0x53b8010)
// ============================================================================
void sub_22950e0(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x53b8010ULL);
    uint8_t** pBegin = *reinterpret_cast<uint8_t***>(selfBytes + 0x18);
    if (pBegin != nullptr) {
        uint8_t** pEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x20);
        for (uint8_t** pCur = pBegin; pCur != pEnd; ++pCur) {
            uint8_t* pItem = *pCur;
            if (pItem != nullptr) {
                if (--(*reinterpret_cast<uint32_t*>(pItem + 0x08)) == 0) {
                    void* pAlloc = sub_207b720(pItem);
                    void* vtbl = *reinterpret_cast<void**>(pItem);
                    (*reinterpret_cast<void (**)(void*)>(vtbl))(pItem);
                    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pItem);
                }
                *pCur = nullptr;
            }
        }
        pBegin = *reinterpret_cast<uint8_t***>(selfBytes + 0x18);
        void* pVecAlloc = *reinterpret_cast<void**>(selfBytes + 0x30);
        void* vecAllocVtbl = *reinterpret_cast<void**>(pVecAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vecAllocVtbl) + 0x70))(pVecAlloc, pBegin);
    }
    sub_2bbe588(selfBytes);
}

// ============================================================================
// 0x22950d0 .. 0x2295050: Wrapper forwarding methods
// ============================================================================
void sub_22950d0() {}

uint64_t sub_22950c0(void* pSelf, void* a2, void* a3, void* a4) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x10);
    void* vtbl = *reinterpret_cast<void**>(pInner);
    return (*reinterpret_cast<uint64_t (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x28))(pInner, a2, a3, a4);
}

uint64_t sub_22950a0(void* pSelf, void* a2, void* a3) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void* pInner = *reinterpret_cast<void**>(selfBytes + 0x10);
    void* pArg4 = *reinterpret_cast<void**>(selfBytes + 0x18);
    void* vtbl = *reinterpret_cast<void**>(pInner);
    return (*reinterpret_cast<uint64_t (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x20))(pInner, a2, a3, pArg4);
}

uint64_t sub_2295090(void* pSelf, void* a2, void* a3, void* a4) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x10);
    void* vtbl = *reinterpret_cast<void**>(pInner);
    return (*reinterpret_cast<uint64_t (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x38))(pInner, a2, a3, a4);
}

uint64_t sub_2295080(void* pSelf, void* a2, void* a3, void* a4) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x10);
    void* vtbl = *reinterpret_cast<void**>(pInner);
    return (*reinterpret_cast<uint64_t (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x30))(pInner, a2, a3, a4);
}

uint32_t sub_2295070() {
    return 0;
}

uint64_t sub_2295060(void* pSelf, void* a2, void* a3, void* a4) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x10);
    void* vtbl = *reinterpret_cast<void**>(pInner);
    return (*reinterpret_cast<uint64_t (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x18))(pInner, a2, a3, a4);
}

uint64_t sub_2295050(void* pSelf, void* a2, void* a3, void* a4) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x10);
    void* vtbl = *reinterpret_cast<void**>(pInner);
    return (*reinterpret_cast<uint64_t (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x10))(pInner, a2, a3, a4);
}

// ============================================================================
// 0x2294f30: sub_2294f30 (144 bytes)
// Base destructor of wrapper object (vtable 0x5360fb0)
// ============================================================================
void sub_2294f30(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x5360fb0ULL);
    for (size_t off : { static_cast<size_t>(0x10), static_cast<size_t>(0x18) }) {
        void* pSub = *reinterpret_cast<void**>(selfBytes + off);
        if (pSub != nullptr) {
            void* pAlloc = sub_207b720(pSub);
            void* vtbl = *reinterpret_cast<void**>(pSub);
            (*reinterpret_cast<void (**)(void*)>(vtbl))(pSub);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pSub);
            *reinterpret_cast<void**>(selfBytes + off) = nullptr;
        }
    }
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x53b7cf0ULL);
}

// ============================================================================
// 0x2294ea0: sub_2294ea0 (133 bytes)
// Deleting destructor of wrapper object (vtable 0x5360fb0)
// ============================================================================
void sub_2294ea0(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes) = GX_VMA(0x5360fb0ULL);
    void* pSub10 = *reinterpret_cast<void**>(selfBytes + 0x10);
    if (pSub10 != nullptr) {
        void* pAlloc = sub_207b720(pSub10);
        void* vtbl = *reinterpret_cast<void**>(pSub10);
        (*reinterpret_cast<void (**)(void*)>(vtbl))(pSub10);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pSub10);
        *reinterpret_cast<void**>(selfBytes + 0x10) = nullptr;
    }
    void* pSub18 = *reinterpret_cast<void**>(selfBytes + 0x18);
    if (pSub18 != nullptr) {
        void* pAlloc = sub_207b720(pSub18);
        void* vtbl = *reinterpret_cast<void**>(pSub18);
        (*reinterpret_cast<void (**)(void*)>(vtbl))(pSub18);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pSub18);
    }
    sub_2bbe588(selfBytes);
}

// ============================================================================
// 0x2294e90: sub_2294e90 (4 bytes)
// Returns uint32_t field at +0x0c
// ============================================================================
uint32_t sub_2294e90(const void* pSelf) {
    return *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(pSelf) + 0x0c);
}

// ============================================================================
// 0x2294d40: sub_2294d40 (334 bytes)
// Copies 0x84-byte camera/cull state block into primary (+0x00) and prev (+0x90)
// ============================================================================
void sub_2294d40(void* pDst, const void* pSrc) {
    uint8_t* dst = static_cast<uint8_t*>(pDst);
    const uint8_t* src = static_cast<const uint8_t*>(pSrc);

    auto copyBlock = [](uint8_t* d, const uint8_t* s) {
        _mm_store_ps(reinterpret_cast<float*>(d + 0x00), _mm_load_ps(reinterpret_cast<const float*>(s + 0x00)));
        _mm_store_ps(reinterpret_cast<float*>(d + 0x10), _mm_load_ps(reinterpret_cast<const float*>(s + 0x10)));
        _mm_store_ps(reinterpret_cast<float*>(d + 0x20), _mm_load_ps(reinterpret_cast<const float*>(s + 0x20)));
        _mm_store_ps(reinterpret_cast<float*>(d + 0x30), _mm_load_ps(reinterpret_cast<const float*>(s + 0x30)));
        _mm_store_ps(reinterpret_cast<float*>(d + 0x40), _mm_load_ps(reinterpret_cast<const float*>(s + 0x40)));
        *reinterpret_cast<float*>(d + 0x50) = *reinterpret_cast<const float*>(s + 0x50);
        *reinterpret_cast<float*>(d + 0x54) = *reinterpret_cast<const float*>(s + 0x54);
        *reinterpret_cast<float*>(d + 0x58) = *reinterpret_cast<const float*>(s + 0x58);
        *reinterpret_cast<uint32_t*>(d + 0x5c) = *reinterpret_cast<const uint32_t*>(s + 0x5c);
        *reinterpret_cast<uint32_t*>(d + 0x60) = *reinterpret_cast<const uint32_t*>(s + 0x60);
        *reinterpret_cast<float*>(d + 0x64) = *reinterpret_cast<const float*>(s + 0x64);
        *reinterpret_cast<float*>(d + 0x6c) = *reinterpret_cast<const float*>(s + 0x6c);
        _mm_storeu_ps(reinterpret_cast<float*>(d + 0x70), _mm_loadu_ps(reinterpret_cast<const float*>(s + 0x70)));
        *reinterpret_cast<uint32_t*>(d + 0x80) = *reinterpret_cast<const uint32_t*>(s + 0x80);
    };

    copyBlock(dst, src);
    if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x5540dd4ULL)) == 0) {
        copyBlock(dst + 0x90, src);
    }
}

// ============================================================================
// 0x2294b30: sub_2294b30 (513 bytes)
// Merges two sorted runs of 16-byte (uint32_t key, uint64_t payload) items
// into the temporary buffer at *(void**)(pOutHolder + 0x20)
// ============================================================================
struct GXLightSortItem16 {
    uint32_t key;
    uint32_t pad;
    uint64_t payload;
};

void* sub_2294b30(void* pRetStorage,
                  const GXLightSortItem16* pFirst1, const GXLightSortItem16* pLast1,
                  const GXLightSortItem16* pFirst2, const GXLightSortItem16* pLast2,
                  void* pOutHolder) {
    uint8_t* pBufDesc = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pOutHolder) + 0x20);
    bool bSecondEmpty = (pFirst2 == pLast2);

    auto pushItem = [&pBufDesc, pOutHolder](const GXLightSortItem16* src, bool reloadHolder) {
        GXLightSortItem16* pWrite = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
        GXLightSortItem16* pEnd = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10);
        if (pWrite < pEnd) {
            *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pWrite + 1;
            pWrite->key = src->key;
            pWrite->payload = src->payload;
        } else {
            GXLightSortItem16* pNext = nullptr;
            if (pWrite != nullptr) {
                uint32_t k = src->key;
                uint32_t pd = src->pad;
                uint64_t w1 = src->payload;
                pWrite->payload = w1;
                pWrite->key = k;
                pWrite->pad = pd;
                if (reloadHolder) {
                    pBufDesc = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pOutHolder) + 0x20);
                }
                pNext = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
            }
            pNext += 1;
            *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pNext;
            *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10) = pNext;
        }
    };

    while (pFirst1 != pLast1 && pFirst2 != pLast2) {
        if (pFirst2->key < pFirst1->key) {
            pushItem(pFirst2, true);
            ++pFirst2;
            bSecondEmpty = (pFirst2 == pLast2);
        } else {
            pushItem(pFirst1, true);
            ++pFirst1;
        }
    }

    while (pFirst1 != pLast1) {
        pushItem(pFirst1, false);
        ++pFirst1;
    }

    *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pOutHolder) + 0x20) = pBufDesc;
    if (!bSecondEmpty) {
        while (pFirst2 != pLast2) {
            pushItem(pFirst2, false);
            ++pFirst2;
        }
    }

    uint8_t* retBytes = static_cast<uint8_t*>(pRetStorage);
    *reinterpret_cast<uint64_t*>(retBytes + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(retBytes + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(retBytes + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(retBytes + 0x00) = 0;
    *reinterpret_cast<uint8_t**>(retBytes + 0x20) = pBufDesc;
    return pRetStorage;
}

void* sub_2bc08b8(size_t byteSize, void* pAllocatorTag);
void sub_2bbf188();
void sub_2948330(void* pOutIter, void* pVec, void* pArg3, void* pArg4, void* pErasePos);
void sub_2948240(void* pVec, const void* pItemPtr);
void sub_207d250();
void sub_2194340(void* pDeviceRoot, void* pSubTarget, uint32_t encodedCount);
uint32_t sub_e4a6b0(const void* pVolume, const void* pPlane);
bool sub_20b01c0(const void* pAABB, const void* pVolume);
void sub_2668a30(const void* pPlanes, void* pOutEdges);
float sub_20ad260(const void* pVolume, const void* pEdge, uint32_t a3, uint32_t a4, uint32_t a5);
uint32_t sub_1dd8ad0(const void* pPlanes, __m128 v0, __m128 v1);
uint32_t sub_20b26f0(const void* pPlanes, const void* pSphere);
uint32_t sub_20b2c40(const void* pPlanes, const void* pVolume);
float sub_2bbe648(float x);
void sub_21aa6c0(void* pOutProjMat, const void* pCameraDesc);
void sub_2668760(const void* pPlanes, void* pOutCorners8);

namespace {

static inline void invertMatrix4x4Culler(const __m128* src, __m128* dst) {
    __m128 r0 = src[0];
    __m128 r1 = src[1];
    __m128 r2 = src[2];
    __m128 r3 = src[3];

    __m128 t0 = _mm_movelh_ps(r0, r1);
    __m128 t1 = _mm_movehl_ps(r1, r0);
    __m128 t2 = _mm_movelh_ps(r2, r3);
    __m128 t3 = _mm_movehl_ps(r3, r2);

    __m128 c0 = _mm_shuffle_ps(t0, t2, 0x88);
    __m128 c1 = _mm_shuffle_ps(t0, t2, 0xdd);
    __m128 c2 = _mm_shuffle_ps(t1, t3, 0x88);
    __m128 c3 = _mm_shuffle_ps(t1, t3, 0xdd);

    __m128 p0 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x50), _mm_movehl_ps(c3, c3)),
        _mm_mul_ps(_mm_movehl_ps(c2, c2), _mm_shuffle_ps(c3, c3, 0x50)));
    __m128 p1 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x50), _mm_movehl_ps(c1, c1)),
        _mm_mul_ps(_mm_movehl_ps(c0, c0), _mm_shuffle_ps(c1, c1, 0x50)));
    __m128 p2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c2, c0, 0x88), _mm_shuffle_ps(c3, c1, 0xdd)),
        _mm_mul_ps(_mm_shuffle_ps(c2, c0, 0xdd), _mm_shuffle_ps(c3, c1, 0x88)));

    __m128 s0 = _mm_shuffle_ps(p0, p2, 0x5d);
    __m128 s1 = _mm_shuffle_ps(p1, p2, 0xfd);
    __m128 s2 = _mm_shuffle_ps(p0, p2, 0x04);
    __m128 s3 = _mm_shuffle_ps(p1, p2, 0xa4);

    __m128 m0 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c1, c1, 0x49), _mm_shuffle_ps(s0, p0, 0x32)),
        _mm_mul_ps(_mm_shuffle_ps(c1, c1, 0x9e), _mm_shuffle_ps(p0, s2, 0x93)));
    __m128 m1 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x12), _mm_shuffle_ps(s0, p0, 0x99)),
        _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x7b), _mm_shuffle_ps(p0, s2, 0x26)));
    __m128 m2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c3, c3, 0x49), _mm_shuffle_ps(s1, p1, 0x32)),
        _mm_mul_ps(_mm_shuffle_ps(c3, c3, 0x9e), _mm_shuffle_ps(p1, s3, 0x93)));
    __m128 m3 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x12), _mm_shuffle_ps(s1, p1, 0x99)),
        _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x7b), _mm_shuffle_ps(p1, s3, 0x26)));

    __m128 u0 = _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(p2), _mm_castps_si128(p0), 8));
    __m128 u1 = _mm_shuffle_ps(p0, p2, 0x4c);
    __m128 u2 = _mm_movehl_ps(p2, p1);
    __m128 u3 = _mm_shuffle_ps(p1, p2, 0xec);

    __m128 q0 = _mm_mul_ps(_mm_shuffle_ps(c1, c1, 0x33), _mm_shuffle_ps(u0, u0, 0x2c));
    __m128 q1 = _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x8d), _mm_shuffle_ps(u1, u1, 0x93));
    __m128 q2 = _mm_mul_ps(_mm_shuffle_ps(c3, c3, 0x33), _mm_shuffle_ps(u2, u2, 0x2c));
    __m128 q3 = _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x8d), _mm_shuffle_ps(u3, u3, 0x93));

    __m128 a0 = _mm_shuffle_ps(_mm_add_ps(q0, m0), _mm_sub_ps(m0, q0), 0xd8);
    __m128 a1 = _mm_shuffle_ps(_mm_sub_ps(m1, q1), _mm_add_ps(q1, m1), 0xd8);
    __m128 a2 = _mm_shuffle_ps(_mm_add_ps(q2, m2), _mm_sub_ps(m2, q2), 0xd8);
    __m128 a3 = _mm_shuffle_ps(_mm_sub_ps(m3, q3), _mm_add_ps(q3, m3), 0xd8);

    a0 = _mm_shuffle_ps(a0, a0, 0xd8);
    a1 = _mm_shuffle_ps(a1, a1, 0xd8);
    a2 = _mm_shuffle_ps(a2, a2, 0xd8);
    a3 = _mm_shuffle_ps(a3, a3, 0xd8);

    __m128 det = _mm_mul_ps(c0, a0);
    det = _mm_hadd_ps(det, det);
    det = _mm_hadd_ps(det, det);
    __m128 invDet = _mm_div_ps(_mm_set1_ps(1.0f), det);

    dst[0] = _mm_mul_ps(invDet, a0);
    dst[1] = _mm_mul_ps(invDet, a1);
    dst[2] = _mm_mul_ps(invDet, a2);
    dst[3] = _mm_mul_ps(invDet, a3);
}

static inline void ensureTempBufferAllocated(uint8_t* pHolder) {
    uint8_t* pBufDesc = *reinterpret_cast<uint8_t**>(pHolder + 0x20);
    void* pBase = *reinterpret_cast<void**>(pBufDesc + 0x00);
    int64_t reqCap = *reinterpret_cast<int64_t*>(pBufDesc + 0x18);
    if (pBase != nullptr || reqCap <= 0) {
        return;
    }
    if ((static_cast<uint64_t>(reqCap) >> 60) != 0) {
        sub_2bbf188();
    }
    void* pNew = nullptr;
    int64_t curCap = reqCap;
    for (;;) {
        pNew = sub_2bc08b8(static_cast<size_t>(curCap) << 4, *reinterpret_cast<void**>(GX_VMA(0x53e4558ULL)));
        if (pNew != nullptr) {
            break;
        }
        int64_t half = curCap / 2;
        if (curCap <= 1) {
            curCap = half;
            break;
        }
        curCap = half;
    }
    pBufDesc = *reinterpret_cast<uint8_t**>(pHolder + 0x20);
    *reinterpret_cast<void**>(pBufDesc + 0x00) = pNew;
    *reinterpret_cast<void**>(pBufDesc + 0x08) = pNew;
    *reinterpret_cast<void**>(pBufDesc + 0x10) = pNew;
    *reinterpret_cast<int64_t*>(pBufDesc + 0x18) = curCap;
}

} // namespace

// ============================================================================
// 0x22940b0: sub_22940b0 (2,677 bytes)
// Buffered merge of two sorted GXLightSortItem16 runs [pFirst, pMid) and [pMid, pLast)
// ============================================================================
void sub_22940b0(GXLightSortItem16* pFirst, GXLightSortItem16* pMid, GXLightSortItem16* pLast,
                 int64_t count1, int64_t count2, void* pTempHolder) {
    uint8_t* holderBytes = static_cast<uint8_t*>(pTempHolder);
    while (count1 + count2 != 2) {
        uint8_t* pBufDesc = *reinterpret_cast<uint8_t**>(holderBytes + 0x20);
        if (count1 <= count2) {
            ensureTempBufferAllocated(holderBytes);
            pBufDesc = *reinterpret_cast<uint8_t**>(holderBytes + 0x20);
            int64_t cap = *reinterpret_cast<int64_t*>(pBufDesc + 0x18);
            if (cap >= count1) {
                GXLightSortItem16* pTmpBase = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x00);
                *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pTmpBase;
                for (GXLightSortItem16* p = pFirst; p != pMid; ++p) {
                    GXLightSortItem16* pW = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
                    pW->key = p->key;
                    pW->payload = p->payload;
                    *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pW + 1;
                    if (pW >= *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10)) {
                        *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10) = pW + 1;
                    }
                }
                GXLightSortItem16* pTmpCur = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x00);
                GXLightSortItem16* pTmpEnd = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
                GXLightSortItem16* pRight = pMid;
                GXLightSortItem16* pOut = pFirst;
                while (pTmpCur != pTmpEnd && pRight != pLast) {
                    if (pRight->key < pTmpCur->key) {
                        pOut->key = pRight->key;
                        pOut->payload = pRight->payload;
                        ++pRight;
                    } else {
                        pOut->key = pTmpCur->key;
                        pOut->payload = pTmpCur->payload;
                        ++pTmpCur;
                    }
                    ++pOut;
                }
                while (pTmpCur != pTmpEnd) {
                    pOut->key = pTmpCur->key;
                    pOut->payload = pTmpCur->payload;
                    ++pTmpCur;
                    ++pOut;
                }
                while (pRight != pLast) {
                    pOut->key = pRight->key;
                    pOut->payload = pRight->payload;
                    ++pRight;
                    ++pOut;
                }
                return;
            }
        }

        ensureTempBufferAllocated(holderBytes);
        pBufDesc = *reinterpret_cast<uint8_t**>(holderBytes + 0x20);
        int64_t cap = *reinterpret_cast<int64_t*>(pBufDesc + 0x18);
        if (cap >= count2) {
            GXLightSortItem16* pTmpBase = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x00);
            *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pTmpBase;
            for (GXLightSortItem16* p = pMid; p != pLast; ++p) {
                GXLightSortItem16* pW = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
                pW->key = p->key;
                pW->payload = p->payload;
                *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pW + 1;
                if (pW >= *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10)) {
                    *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10) = pW + 1;
                }
            }
            GXLightSortItem16* pTmpStart = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x00);
            GXLightSortItem16* pTmpCur = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
            GXLightSortItem16* pLeftCur = pMid;
            GXLightSortItem16* pOut = pLast;
            while (pLeftCur != pFirst && pTmpCur != pTmpStart) {
                if (pTmpCur[-1].key < pLeftCur[-1].key) {
                    --pLeftCur;
                    --pOut;
                    pOut->key = pLeftCur->key;
                    pOut->payload = pLeftCur->payload;
                } else {
                    --pTmpCur;
                    --pOut;
                    pOut->key = pTmpCur->key;
                    pOut->payload = pTmpCur->payload;
                }
            }
            while (pLeftCur != pFirst) {
                --pLeftCur;
                --pOut;
                pOut->key = pLeftCur->key;
                pOut->payload = pLeftCur->payload;
            }
            while (pTmpCur != pTmpStart) {
                --pTmpCur;
                --pOut;
                pOut->key = pTmpCur->key;
                pOut->payload = pTmpCur->payload;
            }
            return;
        }

        int64_t left1 = 0;
        int64_t left2 = 0;
        GXLightSortItem16* pMid1 = nullptr;
        GXLightSortItem16* pMid2 = nullptr;
        if (count1 > count2) {
            left1 = count1 / 2;
            pMid1 = pFirst + left1;
            int64_t lo = 0;
            int64_t hi = count2;
            while (hi > 0) {
                int64_t half = hi / 2;
                if (pMid[lo + half].key < pMid1->key) {
                    lo += half + 1;
                    hi -= half + 1;
                } else {
                    hi = half;
                }
            }
            left2 = lo;
            pMid2 = pMid + left2;
        } else {
            left2 = count2 / 2;
            pMid2 = pMid + left2;
            int64_t lo = 0;
            int64_t hi = count1;
            while (hi > 0) {
                int64_t half = hi / 2;
                if (pMid2->key < pFirst[lo + half].key) {
                    hi = half;
                } else {
                    lo += half + 1;
                    hi -= half + 1;
                }
            }
            left1 = lo;
            pMid1 = pFirst + left1;
        }

        int64_t right1 = count1 - left1;
        GXLightSortItem16* pNewMid = nullptr;
        if (pMid1 == pMid) {
            pNewMid = pMid2;
        } else if (pMid2 == pMid) {
            pNewMid = pMid1;
        } else {
            int64_t totalRot = (pMid2 - pMid1);
            int64_t shift = (pMid - pMid1);
            int64_t a = totalRot;
            int64_t b = shift;
            while (b != 0) {
                int64_t t = a % b;
                a = b;
                b = t;
            }
            for (int64_t c = a; c > 0; ) {
                --c;
                GXLightSortItem16 tmp = pMid1[c];
                int64_t cur = c;
                int64_t next = c + shift;
                if (next == totalRot) next = 0;
                while (next != c) {
                    pMid1[cur].key = pMid1[next].key;
                    pMid1[cur].payload = pMid1[next].payload;
                    cur = next;
                    int64_t rem = totalRot - next;
                    next = (shift >= rem) ? (shift - rem) : (next + shift);
                }
                pMid1[cur].key = tmp.key;
                pMid1[cur].payload = tmp.payload;
            }
            pNewMid = pMid1 + left2;
        }

        sub_22940b0(pFirst, pMid1, pNewMid, left1, left2, holderBytes);
        pFirst = pNewMid;
        pMid = pMid2;
        count1 = right1;
        count2 -= left2;
    }

    if (pMid->key < pFirst->key) {
        uint64_t p0 = pFirst->payload;
        uint64_t p1 = pMid->payload;
        uint32_t k0 = pFirst->key;
        uint32_t k1 = pMid->key;
        pFirst->key = k1;
        pFirst->payload = p1;
        pMid->key = k0;
        pMid->payload = p0;
    }
}

// ============================================================================
// 0x2293900: sub_2293900 (1,968 bytes)
// Chunked insertion sort (32-item chunks) and bottom-up merge pass
// ============================================================================
static inline void insertionSortLightItems(GXLightSortItem16* pFirst, GXLightSortItem16* pLast) {
    if (pFirst == pLast) {
        return;
    }
    for (GXLightSortItem16* pCur = pFirst + 1; pCur != pLast; ++pCur) {
        uint32_t key = pCur->key;
        uint64_t payload = pCur->payload;
        if (key < pFirst->key) {
            for (GXLightSortItem16* pShift = pCur; pShift != pFirst; --pShift) {
                pShift[0].key = pShift[-1].key;
                pShift[0].payload = pShift[-1].payload;
            }
            pFirst->key = key;
            pFirst->payload = payload;
        } else {
            GXLightSortItem16* pIns = pCur;
            while (key < pIns[-1].key) {
                pIns[0].key = pIns[-1].key;
                pIns[0].payload = pIns[-1].payload;
                --pIns;
            }
            pIns->key = key;
            pIns->payload = payload;
        }
    }
}

void sub_2293900(GXLightSortItem16* pFirst, GXLightSortItem16* pLast, int64_t count, void* pTempHolder) {
    GXLightSortItem16* pRem = pFirst;
    if (count >= 32) {
        int64_t rem = count;
        GXLightSortItem16* pChunk = pFirst;
        while (rem > 31) {
            insertionSortLightItems(pChunk, pChunk + 32);
            pChunk += 32;
            rem -= 32;
        }
        pRem = pFirst + (count & ~int64_t(31));
    }
    insertionSortLightItems(pRem, pLast);

    if (count < 33) {
        return;
    }

    uint8_t* holderBytes = static_cast<uint8_t*>(pTempHolder);
    uint8_t* pBufDesc = *reinterpret_cast<uint8_t**>(holderBytes + 0x20);
    GXLightSortItem16* pTmpBase = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x00);

    for (int64_t step = 32; step < count; ) {
        *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pTmpBase;
        int64_t twoStep = step * 2;
        int64_t rem = count;
        GXLightSortItem16* pCur = pFirst;

        alignas(32) uint8_t localHolder[0x28] = {};
        *reinterpret_cast<uint8_t**>(localHolder + 0x20) = pBufDesc;
        alignas(16) uint8_t dummyRet[0x28] = {};

        while (rem >= twoStep) {
            sub_2294b30(dummyRet, pCur, pCur + step, pCur + step, pCur + twoStep, localHolder);
            pCur += twoStep;
            rem -= twoStep;
        }
        if (rem > step) {
            sub_2294b30(dummyRet, pCur, pCur + step, pCur + step, pLast, localHolder);
        } else {
            while (pCur != pLast) {
                GXLightSortItem16* pW = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
                pW->key = pCur->key;
                pW->payload = pCur->payload;
                *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08) = pW + 1;
                if (pW >= *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10)) {
                    *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x10) = pW + 1;
                }
                ++pCur;
            }
        }

        GXLightSortItem16* pTmpEnd = *reinterpret_cast<GXLightSortItem16**>(pBufDesc + 0x08);
        int64_t fourStep = step * 4;
        rem = count;
        GXLightSortItem16* pSrc = pTmpBase;
        GXLightSortItem16* pDst = pFirst;
        step = twoStep;

        while (rem >= fourStep) {
            GXLightSortItem16* pL = pSrc;
            GXLightSortItem16* pMid = pSrc + twoStep;
            GXLightSortItem16* pM = pMid;
            GXLightSortItem16* pR = pSrc + fourStep;
            while (pL != pMid && pM != pR) {
                if (pM->key < pL->key) {
                    pDst->key = pM->key;
                    pDst->payload = pM->payload;
                    ++pM;
                } else {
                    pDst->key = pL->key;
                    pDst->payload = pL->payload;
                    ++pL;
                }
                ++pDst;
            }
            while (pL != pMid) {
                pDst->key = pL->key;
                pDst->payload = pL->payload;
                ++pL;
                ++pDst;
            }
            while (pM != pR) {
                pDst->key = pM->key;
                pDst->payload = pM->payload;
                ++pM;
                ++pDst;
            }
            pSrc += fourStep;
            rem -= fourStep;
        }

        if (rem > twoStep) {
            GXLightSortItem16* pL = pSrc;
            GXLightSortItem16* pM = pSrc + twoStep;
            GXLightSortItem16* pR = pTmpEnd;
            while (pL != pSrc + twoStep && pM != pR) {
                if (pM->key < pL->key) {
                    pDst->key = pM->key;
                    pDst->payload = pM->payload;
                    ++pM;
                } else {
                    pDst->key = pL->key;
                    pDst->payload = pL->payload;
                    ++pL;
                }
                ++pDst;
            }
            while (pL != pSrc + twoStep) {
                pDst->key = pL->key;
                pDst->payload = pL->payload;
                ++pL;
                ++pDst;
            }
            while (pM != pR) {
                pDst->key = pM->key;
                pDst->payload = pM->payload;
                ++pM;
                ++pDst;
            }
        } else {
            while (pSrc != pTmpEnd) {
                pDst->key = pSrc->key;
                pDst->payload = pSrc->payload;
                ++pSrc;
                ++pDst;
            }
        }
        step = fourStep;
    }
}

// ============================================================================
// 0x2293710: sub_2293710 (483 bytes)
// Recursive stable sort entry point for GXLightSortItem16 [pFirst, pLast)
// ============================================================================
void sub_2293710(GXLightSortItem16* pFirst, GXLightSortItem16* pLast, int64_t count, void* pTempHolder) {
    if (count <= 32) {
        insertionSortLightItems(pFirst, pLast);
        return;
    }
    uint8_t* holderBytes = static_cast<uint8_t*>(pTempHolder);
    ensureTempBufferAllocated(holderBytes);
    uint8_t* pBufDesc = *reinterpret_cast<uint8_t**>(holderBytes + 0x20);
    int64_t cap = *reinterpret_cast<int64_t*>(pBufDesc + 0x18);
    int64_t half = (count + 1) / 2;
    GXLightSortItem16* pMid = pFirst + half;
    if (half > cap) {
        sub_2293710(pFirst, pMid, half, holderBytes);
        sub_2293710(pMid, pLast, count - half, holderBytes);
    } else {
        sub_2293900(pFirst, pMid, half, holderBytes);
        sub_2293900(pMid, pLast, count - half, holderBytes);
    }
    sub_22940b0(pFirst, pMid, pLast, half, count - half, holderBytes);
}

// ============================================================================
// 0x22935c0: sub_22935c0 (325 bytes)
// Grows capacity of a GXLightSortItem16 vector ([+0x08..+0x18), alloc at +0x20)
// ============================================================================
void sub_22935c0(void* pVec) {
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    GXLightSortItem16* pBegin = *reinterpret_cast<GXLightSortItem16**>(vecBytes + 0x08);
    GXLightSortItem16* pEnd = *reinterpret_cast<GXLightSortItem16**>(vecBytes + 0x10);
    GXLightSortItem16* pCap = *reinterpret_cast<GXLightSortItem16**>(vecBytes + 0x18);
    if (pEnd != pCap) {
        return;
    }
    size_t oldCount = static_cast<size_t>(pEnd - pBegin);
    size_t half = oldCount >> 1;
    size_t maxCount = 0x0fffffffffffffffULL;
    size_t newCount = (maxCount - half >= oldCount) ? (oldCount + half) : 0;
    if (newCount < oldCount + 1) {
        newCount = oldCount + 1;
    }

    void* pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    size_t byteCap = newCount << 4;
    uint8_t* pNewMem = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAlloc, byteCap, 8));
    if (pNewMem == nullptr) {
        uint64_t fsBase = 0;
        fsBase = gx_guest_tls_base();
        if (*reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) != 0) {
            sub_2bbe978(reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL))), 1);
            return;
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    GXLightSortItem16* pSrcBegin = *reinterpret_cast<GXLightSortItem16**>(vecBytes + 0x08);
    GXLightSortItem16* pSrcEnd = *reinterpret_cast<GXLightSortItem16**>(vecBytes + 0x10);
    GXLightSortItem16* pDst = reinterpret_cast<GXLightSortItem16*>(pNewMem);
    for (GXLightSortItem16* pCur = pSrcBegin; pCur != pSrcEnd; ++pCur, ++pDst) {
        if (pDst != nullptr) {
            uint64_t w0 = *reinterpret_cast<const uint64_t*>(pCur);
            uint64_t w1 = pCur->payload;
            pDst->payload = w1;
            *reinterpret_cast<uint64_t*>(pDst) = w0;
        }
    }

    ptrdiff_t count = pSrcEnd - pSrcBegin;
    if (pSrcBegin != nullptr) {
        pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
        allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pSrcBegin);
    }

    *reinterpret_cast<uint8_t**>(vecBytes + 0x18) = pNewMem + byteCap;
    *reinterpret_cast<GXLightSortItem16**>(vecBytes + 0x10) = reinterpret_cast<GXLightSortItem16*>(pNewMem) + count;
    *reinterpret_cast<uint8_t**>(vecBytes + 0x08) = pNewMem;
}

// ============================================================================
// 0x2293450: sub_2293450 (209 bytes)
// Removes a ref-counted light/shadow node from [pSelf+0x08 .. +0x10)
// ============================================================================
bool sub_2293450(void* pSelf, void* pItem) {
    if (pItem == nullptr) {
        return false;
    }
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void** pCur = *reinterpret_cast<void***>(selfBytes + 0x08);
    void** pEnd = *reinterpret_cast<void***>(selfBytes + 0x10);
    while (pCur != pEnd) {
        void** pNext = pCur + 1;
        if (*pCur == pItem) {
            uint8_t outIter[0x18] = {};
            sub_2948330(outIter, selfBytes, nullptr, nullptr, pCur);
            return true;
        }
        pCur = pNext;
    }
    return false;
}

// ============================================================================
// 0x2293390: sub_2293390 (187 bytes)
// Inserts a unique ref-counted light/shadow node into [pSelf+0x08 .. +0x10)
// ============================================================================
bool sub_2293390(void* pSelf, void* pItem) {
    if (pItem == nullptr) {
        return false;
    }
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void** pBegin = *reinterpret_cast<void***>(selfBytes + 0x08);
    void** pEnd = *reinterpret_cast<void***>(selfBytes + 0x10);
    for (void** pCur = pBegin; pCur != pEnd; ++pCur) {
        if (*pCur == pItem) {
            return false;
        }
    }

    uint8_t* itemBytes = static_cast<uint8_t*>(pItem);
    ++(*reinterpret_cast<uint32_t*>(itemBytes + 0x08));
    void* localHolder = itemBytes;
    sub_2948240(selfBytes, &localHolder);

    uint8_t* pHeld = static_cast<uint8_t*>(localHolder);
    if (pHeld != nullptr) {
        if (--(*reinterpret_cast<uint32_t*>(pHeld + 0x08)) == 0) {
            void* pAlloc = sub_207b720(pHeld);
            void* vtbl = *reinterpret_cast<void**>(pHeld);
            (*reinterpret_cast<void (**)(void*)>(vtbl))(pHeld);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pHeld);
        }
    }
    return true;
}

// ============================================================================
// 0x2293060: sub_2293060 (815 bytes)
// Collects active light/shadow items, sorts them by inverted priority (~+0x0c),
// and executes their vtable+0x20 / vtable+0x38 callbacks
// ============================================================================
void sub_2293060(void* pSelf, void* pRenderCtx) {
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    uint8_t* pTlsBlock = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    if (pTlsBlock == nullptr) {
        sub_207d250();
        fsBase = gx_guest_tls_base();
        pTlsBlock = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    }
    uint8_t* pTlsAlloc = pTlsBlock + 0x28;
    void* tlsVtbl = *reinterpret_cast<void**>(pTlsAlloc);
    uint8_t flagsBuf[8] = {};
    (*reinterpret_cast<void (**)(void*, void*, uint32_t)>(static_cast<uint8_t*>(tlsVtbl) + 0x20))(flagsBuf, pTlsAlloc, 0);
    if ((flagsBuf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    struct LocalSortVec {
        uint64_t pad00;
        GXLightSortItem16* begin;
        GXLightSortItem16* end;
        GXLightSortItem16* cap;
        uint8_t* alloc;
    } sortVec = { 0, nullptr, nullptr, nullptr, pTlsAlloc };

    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t** pItemCur = *reinterpret_cast<uint8_t***>(selfBytes + 0x08);
    uint8_t** pItemEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x10);

    while (pItemCur != pItemEnd) {
        uint8_t* pObj = *pItemCur;
        if (sortVec.end == sortVec.cap) {
            sub_22935c0(&sortVec);
        }
        if (sortVec.end != nullptr) {
            sortVec.end->key = ~(*reinterpret_cast<uint32_t*>(pObj + 0x0c));
            sortVec.end->pad = 0;
            sortVec.end->payload = reinterpret_cast<uint64_t>(pObj);
        }
        ++sortVec.end;
        pItemCur += 1;
        pItemEnd = *reinterpret_cast<uint8_t***>(selfBytes + 0x10);
    }

    uint32_t encodedCount = 0;
    GXLightSortItem16* pFirst = sortVec.begin;
    GXLightSortItem16* pLast = sortVec.end;
    if (pFirst != pLast) {
        int64_t count = static_cast<int64_t>(pLast - pFirst);
        struct TempBufferDesc {
            void* base;
            void* cur;
            void* end;
            int64_t cap;
            void* descPtr;
        } tempDesc = { nullptr, nullptr, nullptr, (count + 1) / 2, nullptr };
        tempDesc.descPtr = &tempDesc;

        sub_2293710(pFirst, pLast, count, &tempDesc);
        if (tempDesc.base != nullptr) {
            sub_2bbe588(tempDesc.base);
        }

        for (GXLightSortItem16* p = pFirst; p != pLast; ++p) {
            void* pObj = reinterpret_cast<void*>(p->payload);
            void* vtbl = *reinterpret_cast<void**>(pObj);
            void* pSub = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl) + 0x20))(pObj);
            if (pSub != nullptr) {
                void* subVtbl = *reinterpret_cast<void**>(pSub);
                (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(subVtbl) + 0x10))(pSub);
            }
        }

        uint32_t idx = 0;
        for (GXLightSortItem16* p = pFirst; p != pLast; ++p, ++idx) {
            void* pObj = reinterpret_cast<void*>(p->payload);
            void* vtbl = *reinterpret_cast<void**>(pObj);
            (*reinterpret_cast<void (**)(void*, void*, uint32_t)>(static_cast<uint8_t*>(vtbl) + 0x38))(
                pObj, pRenderCtx, idx);
        }
        encodedCount = (static_cast<uint32_t>(count) << 28) & 0xf0000000u;
    }

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pSubTarget = (pGlobal6c8[0x84] != 0)
        ? *reinterpret_cast<void**>(pGlobal6c8 + 0x78)
        : *reinterpret_cast<void**>(pGlobal6c8 + 0x10);
    sub_2194340(pGlobal6c8, pSubTarget, encodedCount);

    if (sortVec.begin != nullptr) {
        void* vtbl = *reinterpret_cast<void**>(sortVec.alloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(sortVec.alloc, sortVec.begin);
    }
}

// ============================================================================
// 0x2292f20: sub_2292f20 (167 bytes)
// Tests bounding volume against 6 frustum planes at pSelf + 0x160..0x1b0
// Returns 0 = outside, 1 = intersecting, 2 = inside
// ============================================================================
uint32_t sub_2292f20(const void* pSelf, const void* pVolume) {
    const uint8_t* planesBase = static_cast<const uint8_t*>(pSelf) + 0x160;
    uint32_t hasIntersect = 0;
    for (uint32_t i = 0; i < 6; ++i) {
        alignas(16) __m128 plane = _mm_load_ps(reinterpret_cast<const float*>(planesBase + i * 0x10));
        uint32_t res = sub_e4a6b0(pVolume, &plane);
        if (res == 0) {
            return 0;
        }
        if (res == 1) {
            hasIntersect = 1;
        }
    }
    return (hasIntersect == 1) ? 1u : 2u;
}

// ============================================================================
// 0x2292dc0: sub_2292dc0 (340 bytes)
// Tests capsule/sphere/OBB light volume against frustum AABB, 12 edges, and planes
// ============================================================================
uint32_t sub_2292dc0(const void* pSelf, const void* pVolume) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    if (!sub_20b01c0(selfBytes + 0x220, pVolume)) {
        return 0;
    }
    const uint8_t* planesBase = selfBytes + 0x160;
    alignas(16) __m128 edges[36];
    sub_2668a30(planesBase, edges);

    const __m128 signFlipXY = _mm_setr_ps(-1.0f, -1.0f, 1.0f, 1.0f);
    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    for (size_t i = 0; i < 36; i += 3) {
        edges[i + 0] = _mm_or_ps(_mm_and_ps(_mm_mul_ps(signFlipXY, edges[i + 0]), maskXYZ), unitW);
        edges[i + 1] = _mm_and_ps(_mm_mul_ps(signFlipXY, edges[i + 1]), maskXYZ);
        edges[i + 2] = _mm_and_ps(_mm_mul_ps(signFlipXY, edges[i + 2]), maskXYZ);
    }

    const uint8_t* volBytes = static_cast<const uint8_t*>(pVolume);
    float radius = *reinterpret_cast<const float*>(volBytes + 0x20);
    float radiusSq = radius * radius;
    for (uint32_t i = 0; i < 12; ++i) {
        float distSq = sub_20ad260(pVolume, &edges[i * 3], 0, 0, 0);
        if (radiusSq > distSq) {
            return 1;
        }
    }

    __m128 v0 = _mm_load_ps(reinterpret_cast<const float*>(volBytes + 0x00));
    __m128 v1 = _mm_load_ps(reinterpret_cast<const float*>(volBytes + 0x10));
    uint32_t planeRes = sub_1dd8ad0(planesBase, v0, v1);
    return (planeRes == 2) ? 2u : 0u;
}

// ============================================================================
// 0x2292d50: sub_2292d50 (111 bytes)
// Tests bounding sphere [cx, cy, cz, radius] against frustum AABB and 6 planes
// ============================================================================
uint32_t sub_2292d50(const void* pSelf, const void* pSphere) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    __m128 sph = _mm_load_ps(static_cast<const float*>(pSphere));
    __m128 rad = _mm_shuffle_ps(sph, sph, 0xff);
    __m128 sMin = _mm_sub_ps(sph, rad);
    __m128 sMax = _mm_add_ps(sph, rad);

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    sMin = _mm_or_ps(_mm_and_ps(sMin, maskXYZ), unitW);
    sMax = _mm_or_ps(_mm_and_ps(sMax, maskXYZ), unitW);

    __m128 fMin = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x220));
    __m128 fMax = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x230));

    __m128 cmp0 = _mm_cmplt_ps(fMax, sMin);
    __m128 cmp1 = _mm_cmplt_ps(sMax, fMin);
    __m128 outMask = _mm_and_ps(_mm_or_ps(cmp0, cmp1), maskXYZ);
    __m128 eqZero = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(outMask), _mm_setzero_si128()));
    if (_mm_movemask_ps(eqZero) == 0x0f) {
        return sub_20b26f0(selfBytes + 0x160, pSphere);
    }
    return 0;
}

// ============================================================================
// 0x2292ca0: sub_2292ca0 (167 bytes)
// Tests bounding volume against 6 frustum planes at pSelf + 0x160..0x1b0
// ============================================================================
uint32_t sub_2292ca0(const void* pSelf, const void* pVolume) {
    return sub_2292f20(pSelf, pVolume);
}

// ============================================================================
// 0x2292b80: sub_2292b80 (284 bytes)
// Pure SIMD Point vs Frustum AABB + 6-Plane test (returns 2 if inside, 0 if outside)
// ============================================================================
uint32_t sub_2292b80(const void* pSelf, const void* pPoint) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    __m128 pt = _mm_load_ps(static_cast<const float*>(pPoint));
    __m128 fMin = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x220));
    __m128 fMax = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x230));

    __m128 cMin = _mm_cmplt_ps(pt, fMin);
    __m128 cMax = _mm_cmplt_ps(fMax, pt);
    __m128 cOr = _mm_or_ps(cMin, cMax);
    __m128 cXYZ = _mm_shuffle_ps(cOr, cOr, 0x24);
    __m128 eqZ = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(cXYZ), _mm_setzero_si128()));
    if (_mm_movemask_ps(eqZ) != 0x0f) {
        return 0;
    }

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    __m128 pt4 = _mm_or_ps(_mm_and_ps(pt, maskXYZ), unitW);
    const __m128 zero = _mm_setzero_ps();

    static const size_t kPlaneOffsets[6] = { 0x160, 0x170, 0x190, 0x180, 0x1b0, 0x1a0 };
    for (size_t i = 0; i < 6; ++i) {
        __m128 pl = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + kPlaneOffsets[i]));
        __m128 prod = _mm_mul_ps(pt4, pl);
        prod = _mm_hadd_ps(prod, prod);
        prod = _mm_hadd_ps(prod, prod);
        if (_mm_movemask_ps(_mm_cmplt_ps(prod, zero)) != 0x0f) {
            return 0;
        }
    }
    return 2;
}

// ============================================================================
// 0x2292a20: sub_2292a20 (340 bytes)
// Tests capsule/sphere/OBB light volume against frustum AABB, 12 edges, and planes
// ============================================================================
uint32_t sub_2292a20(const void* pSelf, const void* pVolume) {
    return sub_2292dc0(pSelf, pVolume);
}

// ============================================================================
// 0x22929b0: sub_22929b0 (111 bytes)
// Tests bounding sphere [cx, cy, cz, radius] against frustum AABB and 6 planes
// ============================================================================
uint32_t sub_22929b0(const void* pSelf, const void* pSphere) {
    return sub_2292d50(pSelf, pSphere);
}

// ============================================================================
// 0x22929a0: sub_22929a0 (12 bytes)
// Forwards frustum planes (+0x160) and volume to sub_20b2c40
// ============================================================================
uint32_t sub_22929a0(const void* pSelf, const void* pVolume) {
    return sub_20b2c40(static_cast<const uint8_t*>(pSelf) + 0x160, pVolume);
}

// ============================================================================
// 0x22928a0: sub_22928a0 (247 bytes)
// Pure SIMD AABB [minVec, maxVec] vs Frustum AABB + 6-Plane P/N-vertex test
// Returns 0 = outside, 1 = intersecting, 2 = completely inside
// ============================================================================
uint32_t sub_22928a0(const void* pSelf, const void* pAABB) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    const float* boxFloats = static_cast<const float*>(pAABB);

    __m128 fMin = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x220));
    __m128 fMax = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x230));
    __m128 bMin = _mm_load_ps(boxFloats + 0);
    __m128 bMax = _mm_load_ps(boxFloats + 4);

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    const __m128 zero = _mm_setzero_ps();

    __m128 sep = _mm_and_ps(_mm_or_ps(_mm_cmplt_ps(fMax, bMin), _mm_cmplt_ps(bMax, fMin)), maskXYZ);
    __m128 eqZ = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(sep), _mm_setzero_si128()));
    if (_mm_movemask_ps(eqZ) != 0x0f) {
        return 0;
    }

    bMin = _mm_or_ps(_mm_and_ps(bMin, maskXYZ), unitW);
    bMax = _mm_or_ps(_mm_and_ps(bMax, maskXYZ), unitW);
    const __m128 allOnes = _mm_castsi128_ps(_mm_set1_epi32(-1));

    const uint8_t* planes = selfBytes + 0x160;
    uint32_t anyIntersect = 0;
    for (int i = 0; i < 6; ++i) {
        __m128 pl = _mm_load_ps(reinterpret_cast<const float*>(planes + i * 0x10));
        __m128 geZero = _mm_cmple_ps(zero, _mm_and_ps(pl, maskXYZ));

        __m128 pVert = _mm_or_ps(_mm_and_ps(bMin, geZero), _mm_andnot_ps(geZero, bMax));
        __m128 pDot = _mm_mul_ps(pl, pVert);
        pDot = _mm_hadd_ps(pDot, pDot);
        pDot = _mm_hadd_ps(pDot, pDot);
        if (_mm_movemask_ps(_mm_cmplt_ps(zero, pDot)) != 0) {
            return 0;
        }

        __m128 nVert = _mm_or_ps(_mm_and_ps(bMax, geZero), _mm_and_ps(bMin, _mm_xor_ps(geZero, allOnes)));
        __m128 nDot = _mm_mul_ps(pl, nVert);
        nDot = _mm_hadd_ps(nDot, nDot);
        nDot = _mm_hadd_ps(nDot, nDot);
        if (_mm_movemask_ps(_mm_cmplt_ps(zero, nDot)) != 0) {
            anyIntersect = 1;
        }
    }
    return 2u - (anyIntersect & 1u);
}

// ============================================================================
// 0x2292840: sub_2292840 (95 bytes)
// Computes far-plane corner distance from camera origin:
// sqrt(farZ^2 + (farZ * tan(fovY*0.5))^2 + (farZ * tan(fovY*0.5) * aspect)^2)
// ============================================================================
float sub_2292840(const void* pSelf) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    float fovY = *reinterpret_cast<const float*>(selfBytes + 0x140);
    float farZ = *reinterpret_cast<const float*>(selfBytes + 0x144);
    float tanHalf = sub_2bbe648(fovY * 0.5f);
    float aspect = *reinterpret_cast<const float*>(selfBytes + 0x14c);
    float halfW = farZ * (tanHalf * aspect);
    float halfH = farZ * tanHalf;
    return std::sqrt(farZ * farZ + (halfH * halfH + halfW * halfW));
}

// ============================================================================
// 0x22926f0: sub_22926f0 (331 bytes)
// Computes the 8 view-space frustum corner points (4 near + 4 far) into pOutCorners8
// ============================================================================
void sub_22926f0(const void* pSelf, void* pOutCorners8) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    float fovY = *reinterpret_cast<const float*>(selfBytes + 0x140);
    float farZ = *reinterpret_cast<const float*>(selfBytes + 0x144);
    float nearZ = *reinterpret_cast<const float*>(selfBytes + 0x148);
    float tanHalf = sub_2bbe648(fovY * 0.5f);
    float aspect = *reinterpret_cast<const float*>(selfBytes + 0x14c);

    float tanAspect = tanHalf * aspect;
    float farX = farZ * tanAspect;
    float nearX = nearZ * tanAspect;
    float farY = farZ * tanHalf;
    float nearY = nearZ * tanHalf;

    __m128* out = static_cast<__m128*>(pOutCorners8);
    out[0] = _mm_setr_ps(nearX, nearY, nearZ, 0.0f);
    out[1] = _mm_setr_ps(-nearX, nearY, nearZ, 0.0f);
    out[2] = _mm_setr_ps(nearX, -nearY, nearZ, 0.0f);
    out[3] = _mm_setr_ps(-nearX, -nearY, nearZ, 0.0f);
    out[4] = _mm_setr_ps(farX, farY, farZ, 0.0f);
    out[5] = _mm_setr_ps(-farX, farY, farZ, 0.0f);
    out[6] = _mm_setr_ps(farX, -farY, farZ, 0.0f);
    out[7] = _mm_setr_ps(-farX, -farY, farZ, 0.0f);
}

// ============================================================================
// 0x2291c30: sub_2291c30 (2,742 bytes)
// Updates GXCullVolume matrices, 6 world-space frustum planes, 8 corners,
// and world-space frustum AABB from a camera descriptor
// ============================================================================
void sub_2291c30(void* pSelf, const void* pCameraDesc) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    const uint8_t* camBytes = static_cast<const uint8_t*>(pCameraDesc);

    uint32_t width = *reinterpret_cast<const uint32_t*>(camBytes + 0x5c);
    uint32_t height = *reinterpret_cast<const uint32_t*>(camBytes + 0x60);
    *reinterpret_cast<uint32_t*>(selfBytes + 0x110) = width;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x114) = height;

    float fovY = *reinterpret_cast<const float*>(camBytes + 0x50);
    float farZ = *reinterpret_cast<const float*>(camBytes + 0x54);
    float nearZ = *reinterpret_cast<const float*>(camBytes + 0x58);
    *reinterpret_cast<float*>(selfBytes + 0x140) = fovY;
    *reinterpret_cast<float*>(selfBytes + 0x144) = farZ;
    *reinterpret_cast<float*>(selfBytes + 0x148) = nearZ;
    *reinterpret_cast<float*>(selfBytes + 0x14c) = static_cast<float>(static_cast<int64_t>(width)) /
                                                   static_cast<float>(static_cast<int64_t>(height));

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x240), _mm_load_ps(reinterpret_cast<const float*>(camBytes + 0x00)));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x250), _mm_load_ps(reinterpret_cast<const float*>(camBytes + 0x10)));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x260), _mm_load_ps(reinterpret_cast<const float*>(camBytes + 0x20)));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x270), _mm_load_ps(reinterpret_cast<const float*>(camBytes + 0x30)));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x280), _mm_load_ps(reinterpret_cast<const float*>(camBytes + 0x40)));
    *reinterpret_cast<float*>(selfBytes + 0x290) = fovY;
    *reinterpret_cast<float*>(selfBytes + 0x294) = farZ;
    *reinterpret_cast<float*>(selfBytes + 0x298) = nearZ;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x29c) = width;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x2a0) = height;
    *reinterpret_cast<float*>(selfBytes + 0x2a4) = *reinterpret_cast<const float*>(camBytes + 0x64);
    *reinterpret_cast<float*>(selfBytes + 0x2ac) = *reinterpret_cast<const float*>(camBytes + 0x6c);
    _mm_storeu_ps(reinterpret_cast<float*>(selfBytes + 0x2b0), _mm_loadu_ps(reinterpret_cast<const float*>(camBytes + 0x70)));
    *reinterpret_cast<uint32_t*>(selfBytes + 0x2c0) = *reinterpret_cast<const uint32_t*>(camBytes + 0x80);

    alignas(16) __m128 invCamCols[4];
    invertMatrix4x4Culler(reinterpret_cast<const __m128*>(camBytes), invCamCols);

    alignas(16) __m128 projRows[4];
    sub_21aa6c0(projRows, camBytes);

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x10), invCamCols[0]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x20), invCamCols[1]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x30), invCamCols[2]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x40), invCamCols[3]);

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x90), projRows[0]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xa0), projRows[1]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xb0), projRows[2]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xc0), projRows[3]);

    __m128 hi01 = _mm_unpackhi_ps(invCamCols[0], invCamCols[1]);
    __m128 sh0 = _mm_shuffle_ps(hi01, invCamCols[2], 0x24);
    __m128 hl = _mm_movehl_ps(sh0, invCamCols[3]);
    __m128 camAxisZ = _mm_shuffle_ps(sh0, hl, 0x24);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x130), camAxisZ);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x120), camAxisZ);

    float tanHalfFov = sub_2bbe648(*reinterpret_cast<const float*>(selfBytes + 0x140) * 0.5f);
    float tanRef = sub_2bbe648(0.47996554f); // 0x3ef5be0b
    float heightScale = static_cast<float>(static_cast<int64_t>(*reinterpret_cast<uint32_t*>(selfBytes + 0x114))) / 720.0f;
    float lodScale = (tanHalfFov / tanRef) / heightScale;
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x130), _mm_mul_ps(camAxisZ, _mm_set1_ps(lodScale)));
    *reinterpret_cast<float*>(selfBytes + 0x150) = lodScale;

    for (int i = 0; i < 4; ++i) {
        __m128 r = invCamCols[i];
        __m128 m = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0x00), projRows[0]),
                       _mm_mul_ps(_mm_shuffle_ps(r, r, 0xaa), projRows[2])),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0x55), projRows[1]),
                       _mm_mul_ps(_mm_shuffle_ps(r, r, 0xff), projRows[3])));
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xd0 + i * 0x10), m);
    }

    alignas(16) __m128 viewWorldCols[4];
    invertMatrix4x4Culler(reinterpret_cast<const __m128*>(selfBytes + 0x10), viewWorldCols);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x50), viewWorldCols[0]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x60), viewWorldCols[1]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x70), viewWorldCols[2]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x80), viewWorldCols[3]);

    __m128 pt0 = _mm_movelh_ps(projRows[0], projRows[1]);
    __m128 pt1 = _mm_movehl_ps(projRows[1], projRows[0]);
    __m128 pt2 = _mm_movelh_ps(projRows[2], projRows[3]);
    __m128 pt3 = _mm_movehl_ps(projRows[3], projRows[2]);

    __m128 pc0 = _mm_shuffle_ps(pt0, pt2, 0x88);
    __m128 pc1 = _mm_shuffle_ps(pt0, pt2, 0xdd);
    __m128 pc2 = _mm_shuffle_ps(pt1, pt3, 0x88);
    __m128 pc3 = _mm_shuffle_ps(pt1, pt3, 0xdd);
    const __m128 zero = _mm_setzero_ps();

    __m128 vPlanes[6];
    vPlanes[0] = _mm_sub_ps(zero, _mm_add_ps(pc3, pc0));
    vPlanes[1] = _mm_sub_ps(zero, _mm_sub_ps(pc3, pc0));
    vPlanes[2] = _mm_sub_ps(zero, _mm_add_ps(pc3, pc1));
    vPlanes[3] = _mm_sub_ps(zero, _mm_sub_ps(pc3, pc1));
    vPlanes[4] = _mm_sub_ps(zero, pc2);
    vPlanes[5] = _mm_sub_ps(zero, _mm_sub_ps(pc3, pc2));

    for (int i = 0; i < 6; ++i) {
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x1c0 + i * 0x10), vPlanes[i]);
    }

    __m128 vt0 = _mm_movelh_ps(invCamCols[0], invCamCols[1]);
    __m128 vt1 = _mm_movehl_ps(invCamCols[1], invCamCols[0]);
    __m128 vt2 = _mm_movelh_ps(invCamCols[2], invCamCols[3]);
    __m128 vt3 = _mm_movehl_ps(invCamCols[3], invCamCols[2]);

    __m128 vc0 = _mm_shuffle_ps(vt0, vt2, 0x88);
    __m128 vc1 = _mm_shuffle_ps(vt0, vt2, 0xdd);
    __m128 vc2 = _mm_shuffle_ps(vt1, vt3, 0x88);
    __m128 vc3 = _mm_shuffle_ps(vt1, vt3, 0xdd);

    const size_t kWorldPlaneWriteOrder[6] = { 0x160, 0x170, 0x180, 0x190, 0x1b0, 0x1a0 };
    const int kViewPlaneSrcIdx[6] = { 0, 1, 2, 3, 5, 4 };
    for (int i = 0; i < 6; ++i) {
        __m128 vp = vPlanes[kViewPlaneSrcIdx[i]];
        __m128 wp = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vp, vp, 0x00), vc0),
                       _mm_mul_ps(_mm_shuffle_ps(vp, vp, 0x55), vc1)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vp, vp, 0xaa), vc2),
                       _mm_mul_ps(_mm_shuffle_ps(vp, vp, 0xff), vc3)));
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + kWorldPlaneWriteOrder[i]), wp);
    }

    alignas(16) __m128 corners[8];
    sub_2668760(selfBytes + 0x1c0, corners);

    __m128 aabbMin = _mm_castsi128_ps(_mm_setr_epi32(0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x3f800000));
    __m128 aabbMax = _mm_castsi128_ps(_mm_setr_epi32(static_cast<int32_t>(0xff7fffffu),
                                                     static_cast<int32_t>(0xff7fffffu),
                                                     static_cast<int32_t>(0xff7fffffu),
                                                     0x3f800000));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x220), aabbMin);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x230), aabbMax);

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < 8; ++i) {
        __m128 c = _mm_add_ps(_mm_and_ps(corners[i], maskXYZ), unitW);
        __m128 wc = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(viewWorldCols[0], _mm_shuffle_ps(c, c, 0x00)),
                       _mm_mul_ps(viewWorldCols[1], _mm_shuffle_ps(c, c, 0x55))),
            _mm_add_ps(_mm_mul_ps(viewWorldCols[2], _mm_shuffle_ps(c, c, 0xaa)),
                       _mm_mul_ps(viewWorldCols[3], _mm_shuffle_ps(c, c, 0xff))));
        corners[i] = wc;
        aabbMax = _mm_max_ps(aabbMax, wc);
        aabbMin = _mm_min_ps(aabbMin, wc);
    }

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x230), aabbMax);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x220), aabbMin);
}

// ============================================================================
// 0x2291c20: sub_2291c20 (1 byte)
// Base destructor of GXCullVolume (no-op)
// ============================================================================
void sub_2291c20(void* /*pSelf*/) {}

// ============================================================================
// 0x2291c10: sub_2291c10 (5 bytes)
// Deleting destructor of GXCullVolume
// ============================================================================
void sub_2291c10(void* pSelf) {
    sub_2bbe588(pSelf);
}

// ============================================================================
// 0x22919c0: sub_22919c0 (578 bytes)
// Constructor of GXCullVolume (vtable 0x5360f40)
// ============================================================================
void sub_22919c0(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x00) = GX_VMA(0x5360f40ULL);

    const __m128 id0 = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
    const __m128 id1 = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    const __m128 id2 = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    const __m128 id3 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    for (size_t base : { static_cast<size_t>(0x10), static_cast<size_t>(0x50),
                         static_cast<size_t>(0x90), static_cast<size_t>(0xd0) }) {
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x00), id0);
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x10), id1);
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x20), id2);
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x30), id3);
    }

    *reinterpret_cast<uint64_t*>(selfBytes + 0x110) = 0x000002d000000500ULL;
    const __m128 zero = _mm_setzero_ps();
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x130), zero);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x120), zero);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x140) = 0x3f75be0bULL;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x148) = 0;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x150) = 0x3f800000u;

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x240), id0);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x250), id1);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x260), id2);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x270), id3);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x290) = 0x3dcccccd3f75be0bULL;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x298) = 0x459c4000u;
    *reinterpret_cast<uint32_t*>(selfBytes + 0x2a4) = 0;

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8 != nullptr) {
        uint8_t* pDev90 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x90);
        *reinterpret_cast<uint32_t*>(selfBytes + 0x29c) = *reinterpret_cast<uint32_t*>(pDev90 + 0x80);
        *reinterpret_cast<uint32_t*>(selfBytes + 0x2a0) = *reinterpret_cast<uint32_t*>(pDev90 + 0x84);
    } else {
        *reinterpret_cast<uint64_t*>(selfBytes + 0x29c) = 0;
    }

    selfBytes[0x2a8] = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x2bc) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x2b4) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x2ac) = 0;

    std::memset(selfBytes + 0x160, 0, 0xc0);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x220),
                 _mm_castsi128_ps(_mm_setr_epi32(0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x3f800000)));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x230),
                 _mm_castsi128_ps(_mm_setr_epi32(static_cast<int32_t>(0xff7fffffu),
                                                 static_cast<int32_t>(0xff7fffffu),
                                                 static_cast<int32_t>(0xff7fffffu),
                                                 0x3f800000)));
}

void sub_21aa290(void* pOutProjMat, void* pCameraOrVolume);
void sub_2168b60(void* pCtx, const void* a2, void* res0, uint32_t a4, void* res1, uint32_t a6, uint32_t fmt);
void sub_21bb550(void* pSubTarget, void* a2, void* a3);
void sub_2168880(void* pCtx, void* a2, void* a3, uint32_t a4, uint32_t a5, uint32_t a6,
                 uint32_t a7, void* a8, uint32_t a9, void* a10);
void sub_2168660(void* pCtx, void* a2, void* a3, void* a4);
void sub_26b4ef0(void* pCtx, void* pTarget, void* pDesc, const float* rgba);
void sub_26af700(void* pCtxSub);
void sub_26b5f50(void* pCtx, void* pTarget, void* pDesc, uint32_t stencilVal, uint32_t flags, float depthVal);
uint8_t* sub_2289bb0(void* pCmdBuf, uint32_t type, uint32_t size, uint32_t align);
void sub_2289ca0(void* pCmdBuf, const void* pHeaderDesc);
void* sub_e45560(void* pArena, uint32_t size, uint32_t align);

// ============================================================================
// 0x2291530: sub_2291530 (1,010 bytes)
// Copies GXCullContext state (+0x10 .. +0x3f8) from pSrc to pDst
// ============================================================================
void sub_2291530(void* pDst, const void* pSrc) {
    uint8_t* dstBytes = static_cast<uint8_t*>(pDst);
    const uint8_t* srcBytes = static_cast<const uint8_t*>(pSrc);

    for (size_t off = 0x10; off <= 0x100; off += 0x10) {
        _mm_store_ps(reinterpret_cast<float*>(dstBytes + off),
                     _mm_load_ps(reinterpret_cast<const float*>(srcBytes + off)));
    }

    *reinterpret_cast<uint32_t*>(dstBytes + 0x110) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x110);
    *reinterpret_cast<uint32_t*>(dstBytes + 0x114) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x114);
    *reinterpret_cast<uint32_t*>(dstBytes + 0x118) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x118);
    *reinterpret_cast<uint32_t*>(dstBytes + 0x11c) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x11c);

    for (size_t off = 0x130; off <= 0x220; off += 0x10) {
        _mm_store_ps(reinterpret_cast<float*>(dstBytes + off),
                     _mm_load_ps(reinterpret_cast<const float*>(srcBytes + off)));
    }

    std::memcpy(dstBytes + 0x230, srcBytes + 0x230, 0x11 * sizeof(uint32_t));

    for (size_t off = 0x280; off <= 0x3a0; off += 0x10) {
        _mm_store_ps(reinterpret_cast<float*>(dstBytes + off),
                     _mm_load_ps(reinterpret_cast<const float*>(srcBytes + off)));
    }

    *reinterpret_cast<float*>(dstBytes + 0x3b0) = *reinterpret_cast<const float*>(srcBytes + 0x3b0);
    *reinterpret_cast<float*>(dstBytes + 0x3b4) = *reinterpret_cast<const float*>(srcBytes + 0x3b4);
    *reinterpret_cast<float*>(dstBytes + 0x3b8) = *reinterpret_cast<const float*>(srcBytes + 0x3b8);
    *reinterpret_cast<uint32_t*>(dstBytes + 0x3bc) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x3bc);
    *reinterpret_cast<uint32_t*>(dstBytes + 0x3c0) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x3c0);
    *reinterpret_cast<float*>(dstBytes + 0x3c4) = *reinterpret_cast<const float*>(srcBytes + 0x3c4);
    *reinterpret_cast<float*>(dstBytes + 0x3cc) = *reinterpret_cast<const float*>(srcBytes + 0x3cc);
    _mm_storeu_ps(reinterpret_cast<float*>(dstBytes + 0x3d0),
                  _mm_loadu_ps(reinterpret_cast<const float*>(srcBytes + 0x3d0)));
    *reinterpret_cast<uint32_t*>(dstBytes + 0x3e0) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x3e0);
    *reinterpret_cast<uint64_t*>(dstBytes + 0x3f0) = *reinterpret_cast<const uint64_t*>(srcBytes + 0x3f0);
    *reinterpret_cast<uint64_t*>(dstBytes + 0x3f8) = *reinterpret_cast<const uint64_t*>(srcBytes + 0x3f8);
}

// ============================================================================
// 0x22911b0: sub_22911b0 (884 bytes)
// Updates GXCullContext view (+0x10), proj (+0x50), viewProj (+0x90), invView (+0xd0)
// ============================================================================
void sub_22911b0(void* pSelf, const __m128* pViewMat, const __m128* pProjMat) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);

    __m128 v0 = pViewMat[0];
    __m128 v1 = pViewMat[1];
    __m128 v2 = pViewMat[2];
    __m128 v3 = pViewMat[3];
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x10), v0);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x20), v1);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x30), v2);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x40), v3);

    __m128 p0 = pProjMat[0];
    __m128 p1 = pProjMat[1];
    __m128 p2 = pProjMat[2];
    __m128 p3 = pProjMat[3];
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x50), p0);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x60), p1);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x70), p2);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x80), p3);

    auto mulRow = [&](__m128 r) -> __m128 {
        __m128 t0 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0x00), p0),
                               _mm_mul_ps(_mm_shuffle_ps(r, r, 0xaa), p2));
        __m128 t1 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0x55), p1),
                               _mm_mul_ps(_mm_shuffle_ps(r, r, 0xff), p3));
        return _mm_add_ps(t0, t1);
    };

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x90), mulRow(v0));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xa0), mulRow(v1));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xb0), mulRow(v2));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xc0), mulRow(v3));

    invertMatrix4x4Culler(pViewMat, reinterpret_cast<__m128*>(selfBytes + 0xd0));
}

// ============================================================================
// 0x2290dd0: sub_2290dd0 (987 bytes)
// Updates GXCullContext from a world/camera volume object
// ============================================================================
void sub_2290dd0(void* pSelf, void* pCameraVolume) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    const __m128* camCols = static_cast<const __m128*>(pCameraVolume);

    invertMatrix4x4Culler(camCols, reinterpret_cast<__m128*>(selfBytes + 0x10));

    alignas(16) __m128 projCols[4];
    sub_21aa290(projCols, pCameraVolume);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x50), projCols[0]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x60), projCols[1]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x70), projCols[2]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x80), projCols[3]);

    __m128 v0 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x10));
    __m128 v1 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x20));
    __m128 v2 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x30));
    __m128 v3 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x40));

    __m128 p0 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x50));
    __m128 p1 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x60));
    __m128 p2 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x70));
    __m128 p3 = _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x80));

    auto mulRow = [&](__m128 r) -> __m128 {
        __m128 t0 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0x00), p0),
                               _mm_mul_ps(_mm_shuffle_ps(r, r, 0xaa), p2));
        __m128 t1 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0x55), p1),
                               _mm_mul_ps(_mm_shuffle_ps(r, r, 0xff), p3));
        return _mm_add_ps(t0, t1);
    };

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x90), mulRow(v0));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xa0), mulRow(v1));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xb0), mulRow(v2));
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xc0), mulRow(v3));

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xd0), camCols[0]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xe0), camCols[1]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0xf0), camCols[2]);
    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x100), camCols[3]);

    sub_2291c30(selfBytes + 0x120, pCameraVolume);
    *reinterpret_cast<void**>(selfBytes + 0x3f8) = pCameraVolume;
}

// ============================================================================
// 0x2290dc0: sub_2290dc0 (11 bytes)
// Base destructor of GXCullContext (vtable 0x53b7fe0)
// ============================================================================
void sub_2290dc0(void* pSelf) {
    *reinterpret_cast<uint64_t*>(pSelf) = GX_VMA(0x53b7fe0ULL);
}

// ============================================================================
// 0x2290db0: sub_2290db0 (5 bytes)
// Deleting destructor of GXCullContext
// ============================================================================
void sub_2290db0(void* pSelf) {
    sub_2bbe588(pSelf);
}

// ============================================================================
// 0x2290c80: sub_2290c80 (296 bytes)
// Constructor of GXCullContext (vtable 0x53b7fe0)
// ============================================================================
void sub_2290c80(void* pSelf, void* pCullVolumeOverride) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x00) = GX_VMA(0x53b7fe0ULL);

    const __m128 id0 = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
    const __m128 id1 = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    const __m128 id2 = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    const __m128 id3 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    for (size_t base : { static_cast<size_t>(0x10), static_cast<size_t>(0x50),
                         static_cast<size_t>(0x90), static_cast<size_t>(0xd0) }) {
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x00), id0);
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x10), id1);
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x20), id2);
        _mm_store_ps(reinterpret_cast<float*>(selfBytes + base + 0x30), id3);
    }

    *reinterpret_cast<uint64_t*>(selfBytes + 0x118) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x110) = 0;
    sub_22919c0(selfBytes + 0x120);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x3f8) = 0;
    *reinterpret_cast<void**>(selfBytes + 0x3f0) =
        (pCullVolumeOverride != nullptr) ? pCullVolumeOverride : (selfBytes + 0x120);
}

// ============================================================================
// 0x2290c70: sub_2290c70 (11 bytes)
// Sets boolean flag at pSelf + 0x6f8
// ============================================================================
void sub_2290c70(void* pSelf, uint64_t flag) {
    static_cast<uint8_t*>(pSelf)[0x6f8] = (flag != 0) ? 1 : 0;
}

// ============================================================================
// 0x2290c60: sub_2290c60 (9 bytes)
// Updates GXCullContext matrices from contiguous [viewMat, projMat] pair
// ============================================================================
void sub_2290c60(void* pSelf, const __m128* pViewAndProjMats) {
    sub_22911b0(pSelf, pViewAndProjMats, pViewAndProjMats + 4);
}

// ============================================================================
// 0x2290c20: sub_2290c20 (64 bytes)
// Binds a single resource via vtable + 0x68
// ============================================================================
void sub_2290c20(void* pSelf, uint32_t slot, void* pResource) {
    void* localRes = pResource;
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    (*reinterpret_cast<void (**)(void*, uint32_t, uint32_t, void**)>(static_cast<uint8_t*>(vtbl) + 0x68))(
        pSelf, slot, 1, &localRes);
}

// ============================================================================
// 0x2290c10: sub_2290c10 (13 bytes)
// Stores 16-byte vector at pSelf + 0x710
// ============================================================================
void sub_2290c10(void* pSelf, const __m128* pVec) {
    _mm_store_ps(reinterpret_cast<float*>(static_cast<uint8_t*>(pSelf) + 0x710), *pVec);
}

// ============================================================================
// 0x2290c00: sub_2290c00 (9 bytes)
// ============================================================================
uint64_t sub_2290c00(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x60))(
        pSelf, a2, a3, a4);
}

// ============================================================================
// 0x2290bf0: sub_2290bf0 (6 bytes)
// ============================================================================
uint64_t sub_2290bf0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58))(
        pSelf, a2, a3, a4);
}

// ============================================================================
// 0x2290be0: sub_2290be0 (6 bytes)
// ============================================================================
uint64_t sub_2290be0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x50))(
        pSelf, a2, a3, a4);
}

// ============================================================================
// 0x2290bd0: sub_2290bd0 (9 bytes)
// ============================================================================
uint64_t sub_2290bd0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x48))(
        pSelf, a2, a3, a4);
}

// ============================================================================
// 0x2290bc0: sub_2290bc0 (6 bytes)
// ============================================================================
uint64_t sub_2290bc0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x40))(
        pSelf, a2, a3, a4);
}

// ============================================================================
// 0x2290bb0: sub_2290bb0 (6 bytes)
// ============================================================================
uint64_t sub_2290bb0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x38))(
        pSelf, a2, a3, a4);
}

// ============================================================================
// 0x2290ab0: sub_2290ab0 (245 bytes)
// Copies/resolves render targets and depth targets via pSelf + 0x438 / +0x440
// ============================================================================
void sub_2290ab0(void* pSelf, void* pDesc) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* descBytes = static_cast<uint8_t*>(pDesc);
    uint8_t* pObj10 = *reinterpret_cast<uint8_t**>(descBytes + 0x10);

    if (pObj10 != nullptr) {
        uint8_t* pObj00 = *reinterpret_cast<uint8_t**>(descBytes + 0x00);
        if (pObj00 != nullptr &&
            *reinterpret_cast<void**>(descBytes + 0x08) != nullptr &&
            *reinterpret_cast<void**>(descBytes + 0x18) != nullptr) {
            void* vtbl0 = *reinterpret_cast<void**>(pObj00);
            uint8_t* pSub48 = *reinterpret_cast<uint8_t**>(pObj00 + 0x48);
            uint32_t fmt = *reinterpret_cast<uint32_t*>(pSub48 + 0x30);
            void* pCtx = *reinterpret_cast<void**>(selfBytes + 0x438);
            void* res0 = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl0) + 0x28))(pObj00);
            pObj10 = *reinterpret_cast<uint8_t**>(descBytes + 0x10);
            void* vtbl1 = *reinterpret_cast<void**>(pObj10);
            void* res1 = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl1) + 0x28))(pObj10);
            sub_2168b60(pCtx, pDesc, res0, 0, res1, 0, fmt);
            sub_21bb550(selfBytes + 0x440,
                        *reinterpret_cast<void**>(descBytes + 0x08),
                        *reinterpret_cast<void**>(descBytes + 0x18));
            return;
        }
    }

    void* pObj08 = *reinterpret_cast<void**>(descBytes + 0x08);
    if (pObj08 != nullptr) {
        void* pObj18 = *reinterpret_cast<void**>(descBytes + 0x18);
        if (pObj18 != nullptr) {
            sub_21bb550(selfBytes + 0x440, pObj08, pObj18);
            return;
        }
    }

    if (pObj10 != nullptr) {
        uint8_t* pObj00 = *reinterpret_cast<uint8_t**>(descBytes + 0x00);
        if (pObj00 != nullptr) {
            void* vtbl0 = *reinterpret_cast<void**>(pObj00);
            uint8_t* pSub48 = *reinterpret_cast<uint8_t**>(pObj00 + 0x48);
            uint32_t fmt = *reinterpret_cast<uint32_t*>(pSub48 + 0x30);
            void* pCtx = *reinterpret_cast<void**>(selfBytes + 0x438);
            void* res0 = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl0) + 0x28))(pObj00);
            pObj10 = *reinterpret_cast<uint8_t**>(descBytes + 0x10);
            void* vtbl1 = *reinterpret_cast<void**>(pObj10);
            void* res1 = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl1) + 0x28))(pObj10);
            sub_2168b60(pCtx, nullptr, res0, 0, res1, 0, fmt);
        }
    }
}

// ============================================================================
// 0x2290a80: sub_2290a80 (46 bytes)
// Dispatches copy/blit command packet via sub_2168b60
// ============================================================================
void sub_2290a80(void* pSelf, const void* pCmd) {
    const uint8_t* cmd = static_cast<const uint8_t*>(pCmd);
    void* pCtx = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x438);
    sub_2168b60(pCtx,
                pCmd,
                *reinterpret_cast<void* const*>(cmd + 0x00),
                *reinterpret_cast<const uint32_t*>(cmd + 0x08),
                *reinterpret_cast<void* const*>(cmd + 0x10),
                *reinterpret_cast<const uint32_t*>(cmd + 0x18),
                *reinterpret_cast<const uint32_t*>(cmd + 0x1c));
}

// ============================================================================
// 0x2290a00: sub_2290a00 (122 bytes)
// Dispatches surface copy if format is neither 0x15 nor 0x2e
// ============================================================================
void sub_2290a00(void* pSelf, void* pDesc) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* descBytes = static_cast<uint8_t*>(pDesc);
    if (*reinterpret_cast<void**>(descBytes + 0x08) == nullptr) {
        return;
    }
    uint8_t* pObj00 = *reinterpret_cast<uint8_t**>(descBytes + 0x00);
    if (pObj00 == nullptr) {
        return;
    }
    void* vtbl0 = *reinterpret_cast<void**>(pObj00);
    uint32_t fmt = (*reinterpret_cast<uint32_t (**)(void*)>(static_cast<uint8_t*>(vtbl0) + 0x38))(pObj00);
    if (fmt == 0x15u || fmt == 0x2eu) {
        return;
    }
    void* pCtx = *reinterpret_cast<void**>(selfBytes + 0x438);
    pObj00 = *reinterpret_cast<uint8_t**>(descBytes + 0x00);
    vtbl0 = *reinterpret_cast<void**>(pObj00);
    void* res0 = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl0) + 0x28))(pObj00);

    uint8_t* pObj08 = *reinterpret_cast<uint8_t**>(descBytes + 0x08);
    void* vtbl1 = *reinterpret_cast<void**>(pObj08);
    void* res1 = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl1) + 0x28))(pObj08);

    sub_2168b60(pCtx, pDesc, res0, 0, res1, 0, fmt);
}

// ============================================================================
// 0x22909b0: sub_22909b0 (73 bytes)
// Dispatches region copy command via sub_2168880
// ============================================================================
void sub_22909b0(void* pSelf, const void* pCmd) {
    const uint8_t* cmd = static_cast<const uint8_t*>(pCmd);
    void* pCtx = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x438);
    void* arg28 = *reinterpret_cast<void* const*>(cmd + 0x28);
    sub_2168880(pCtx,
                arg28,
                *reinterpret_cast<void* const*>(cmd + 0x00),
                *reinterpret_cast<const uint32_t*>(cmd + 0x08),
                *reinterpret_cast<const uint32_t*>(cmd + 0x0c),
                *reinterpret_cast<const uint32_t*>(cmd + 0x10),
                *reinterpret_cast<const uint32_t*>(cmd + 0x14),
                *reinterpret_cast<void* const*>(cmd + 0x18),
                *reinterpret_cast<const uint32_t*>(cmd + 0x20),
                arg28);
}

// ============================================================================
// 0x2290990: sub_2290990 (21 bytes)
// Forwards command to sub_2168660
// ============================================================================
void sub_2290990(void* pSelf, void* a2, void* a3) {
    void* pCtx = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x438);
    sub_2168660(pCtx, a2, a2, a3);
}

// ============================================================================
// 0x2290960: sub_2290960 (43 bytes)
// Calls vtbl + 0x28 then tail-calls vtbl + 0x30
// ============================================================================
void sub_2290960(void* pSelf, const void* pCmd) {
    const uint8_t* cmd = static_cast<const uint8_t*>(pCmd);
    void* vtbl0 = *reinterpret_cast<void**>(pSelf);
    (*reinterpret_cast<void (**)(void*, uint32_t, const void*, void*)>(static_cast<uint8_t*>(vtbl0) + 0x28))(
        pSelf,
        *reinterpret_cast<const uint32_t*>(cmd + 0x00),
        cmd + 0x08,
        *reinterpret_cast<void* const*>(cmd + 0x48));
    void* vtbl1 = *reinterpret_cast<void**>(pSelf);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl1) + 0x30))(pSelf);
}

// ============================================================================
// 0x2290950: sub_2290950 (6 bytes)
// ============================================================================
void sub_2290950(void* pSelf, void* a2) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x20))(pSelf, a2);
}

// ============================================================================
// 0x2290930: sub_2290930 (22 bytes)
// Unpacks command and tail-calls vtbl + 0x18
// ============================================================================
void sub_2290930(void* pSelf, const void* pCmd) {
    const uint8_t* cmd = static_cast<const uint8_t*>(pCmd);
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    (*reinterpret_cast<void (**)(void*, uint32_t, const void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x18))(
        pSelf,
        *reinterpret_cast<const uint32_t*>(cmd + 0x00),
        cmd + 0x08,
        *reinterpret_cast<void* const*>(cmd + 0x48));
}

// ============================================================================
// 0x2290810: sub_2290810 (276 bytes)
// Executes render target color clear and depth/stencil clear packet
// ============================================================================
void sub_2290810(void* pSelf, const void* pCmd) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    const uint8_t* cmd = static_cast<const uint8_t*>(pCmd);

    uint8_t* pColorTarget = *reinterpret_cast<uint8_t* const*>(cmd + 0x00);
    if (pColorTarget != nullptr) {
        uint32_t packedColor = *reinterpret_cast<const uint32_t*>(cmd + 0x14);
        const float inv255 = 1.0f / 255.0f;
        alignas(16) float rgba[4];
        rgba[0] = static_cast<float>((packedColor >> 16) & 0xffu) * inv255;
        rgba[1] = static_cast<float>((packedColor >> 8) & 0xffu) * inv255;
        rgba[2] = static_cast<float>(packedColor & 0xffu) * inv255;
        rgba[3] = static_cast<float>(packedColor >> 24) * inv255;

        uint8_t* pCtx = *reinterpret_cast<uint8_t**>(selfBytes + 0x438);
        uint8_t* pDesc = *reinterpret_cast<uint8_t**>(pColorTarget + 0x08);
        sub_26b4ef0(pCtx, pColorTarget + 0x10, pDesc + 0x18, rgba);
        sub_26af700(pCtx + 0x10);
    }

    uint8_t* pDepthTarget = *reinterpret_cast<uint8_t* const*>(cmd + 0x08);
    if (pDepthTarget != nullptr) {
        uint8_t* pCtx = *reinterpret_cast<uint8_t**>(selfBytes + 0x438);
        uint32_t clearFlags = *reinterpret_cast<const uint32_t*>(cmd + 0x10);
        float depthVal = *reinterpret_cast<const float*>(cmd + 0x18);
        uint32_t stencilVal = cmd[0x1c];
        uint8_t* pDesc = *reinterpret_cast<uint8_t**>(pDepthTarget + 0x08);
        sub_26b5f50(pCtx, pDepthTarget + 0x10, pDesc + 0x18, stencilVal, clearFlags, depthVal);
        sub_26af700(pCtx + 0x10);
    }
}

// ============================================================================
// 0x2290800: sub_2290800 (7 bytes)
// ============================================================================
void sub_2290800(void* pSelf, uint32_t val) {
    *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(pSelf) + 0x700) = val;
}

// ============================================================================
// 0x22907f0: sub_22907f0 (7 bytes)
// ============================================================================
void sub_22907f0(void* pSelf, uint32_t val) {
    *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(pSelf) + 0x6fc) = val;
}

// ============================================================================
// 0x22907e0: sub_22907e0 (3 bytes)
// ============================================================================
uint32_t sub_22907e0() {
    return 0;
}

// ============================================================================
// 0x22907d0: sub_22907d0 (1 byte)
// ============================================================================
void sub_22907d0() {}

// ============================================================================
// 0x22907c0: sub_22907c0 (3 bytes)
// ============================================================================
uint32_t sub_22907c0() {
    return 0;
}

// ============================================================================
// 0x22907b0: sub_22907b0 (4 bytes)
// ============================================================================
void* sub_22907b0(void* pSelf) {
    return pSelf;
}

// ============================================================================
// 0x2290660: sub_2290660 (182 bytes)
// Copies extended GXCommandContext state (+0x10 .. +0x44c)
// ============================================================================
void sub_2290660(void* pDst, const void* pSrc) {
    uint8_t* dstBytes = static_cast<uint8_t*>(pDst);
    const uint8_t* srcBytes = static_cast<const uint8_t*>(pSrc);

    sub_2291530(dstBytes, srcBytes);
    dstBytes[0x400] = srcBytes[0x400];
    dstBytes[0x401] = srcBytes[0x401];
    dstBytes[0x402] = srcBytes[0x402];
    dstBytes[0x403] = srcBytes[0x403];
    dstBytes[0x404] = srcBytes[0x404];

    __m128 u0 = _mm_loadu_ps(reinterpret_cast<const float*>(srcBytes + 0x405));
    __m128 u1 = _mm_loadu_ps(reinterpret_cast<const float*>(srcBytes + 0x415));
    __m128 u2 = _mm_loadu_ps(reinterpret_cast<const float*>(srcBytes + 0x425));
    __m128 u3 = _mm_loadu_ps(reinterpret_cast<const float*>(srcBytes + 0x435));
    _mm_storeu_ps(reinterpret_cast<float*>(dstBytes + 0x405), u0);
    _mm_storeu_ps(reinterpret_cast<float*>(dstBytes + 0x415), u1);
    _mm_storeu_ps(reinterpret_cast<float*>(dstBytes + 0x425), u2);
    _mm_storeu_ps(reinterpret_cast<float*>(dstBytes + 0x435), u3);

    *reinterpret_cast<uint32_t*>(dstBytes + 0x448) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x448);
    *reinterpret_cast<uint32_t*>(dstBytes + 0x44c) = *reinterpret_cast<const uint32_t*>(srcBytes + 0x44c);
}

// ============================================================================
// 0x2290640: sub_2290640 (18 bytes)
// Returns head pointer of command buffer at pSelf + 0x450
// ============================================================================
void* sub_2290640(const void* pSelf) {
    void* const* pCmdBuf = *reinterpret_cast<void* const* const*>(static_cast<const uint8_t*>(pSelf) + 0x450);
    return (pCmdBuf != nullptr) ? *pCmdBuf : nullptr;
}

// ============================================================================
// 0x2290570: sub_2290570 (199 bytes)
// Allocates a 0xf8-type command packet and commits pending sort key header
// ============================================================================
void sub_2290570(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pPacket = sub_2289bb0(*reinterpret_cast<void**>(selfBytes + 0x450), 0xf8u, 0x08u, 0x04u);
    if (pPacket != nullptr && selfBytes[0x401] != 0) {
        uint32_t packedKey = (*reinterpret_cast<uint32_t*>(selfBytes + 0x110) << 28) |
                             ((*reinterpret_cast<uint32_t*>(selfBytes + 0x114) & 0x3fu) << 22) |
                             ((*reinterpret_cast<uint32_t*>(selfBytes + 0x118) & 0x3fu) << 16) |
                             static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(selfBytes + 0x11c));
        struct PacketCommitDesc {
            uint32_t key;
            uint32_t pad;
            void* pPacket;
            uint8_t flag0;
            uint8_t flag1;
            uint8_t padTail[6];
        } desc = {};
        desc.key = packedKey;
        desc.pPacket = pPacket;
        uint8_t f1 = selfBytes[0x403];
        desc.flag0 = (f1 != 0) ? 0 : ((selfBytes[0x402] != 0) ? 1 : 0);
        desc.flag1 = f1;
        sub_2289ca0(*reinterpret_cast<void**>(selfBytes + 0x450), &desc);
        selfBytes[0x401] = 0;
    }
}

// ============================================================================
// 0x2290470: sub_2290470 (248 bytes)
// Enqueues a command packet for sub_2290c70 (set flag at +0x6f8)
// ============================================================================
void sub_2290470(void* pSelf, uint8_t flagVal) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pPacket = sub_2289bb0(*reinterpret_cast<void**>(selfBytes + 0x450), 0x04u, 0x18u, 0x04u);
    uint8_t* pPayload = nullptr;
    if (pPacket != nullptr) {
        if (selfBytes[0x401] != 0) {
            uint32_t packedKey = (*reinterpret_cast<uint32_t*>(selfBytes + 0x110) << 28) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x114) & 0x3fu) << 22) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x118) & 0x3fu) << 16) |
                                 static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(selfBytes + 0x11c));
            struct PacketCommitDesc {
                uint32_t key;
                uint32_t pad;
                void* pPacket;
                uint8_t flag0;
                uint8_t flag1;
                uint8_t padTail[6];
            } desc = {};
            desc.key = packedKey;
            desc.pPacket = pPacket;
            uint8_t f1 = selfBytes[0x403];
            desc.flag0 = (f1 != 0) ? 0 : ((selfBytes[0x402] != 0) ? 1 : 0);
            desc.flag1 = f1;
            sub_2289ca0(*reinterpret_cast<void**>(selfBytes + 0x450), &desc);
            selfBytes[0x401] = 0;
        }
        pPayload = pPacket + pPacket[1] + 4;
    }
    *reinterpret_cast<uint64_t*>(pPayload + 0x00) = GX_VMA(0x2290c70ULL);
    *reinterpret_cast<uint64_t*>(pPayload + 0x08) = static_cast<uint64_t>(flagVal);
    *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
}

// ============================================================================
// 0x22902d0: sub_22902d0 (412 bytes)
// Enqueues a command packet for sub_2290c60 and updates GXCullContext matrices
// ============================================================================
void sub_22902d0(void* pSelf, const __m128* pViewMat, const __m128* pProjMat) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pCmdBuf = *reinterpret_cast<uint8_t**>(selfBytes + 0x450);
    __m128* pArenaMats = static_cast<__m128*>(sub_e45560(pCmdBuf + 0x38, 0x80u, 0x10u));
    pArenaMats[0] = pViewMat[0];
    pArenaMats[1] = pViewMat[1];
    pArenaMats[2] = pViewMat[2];
    pArenaMats[3] = pViewMat[3];
    pArenaMats[4] = pProjMat[0];
    pArenaMats[5] = pProjMat[1];
    pArenaMats[6] = pProjMat[2];
    pArenaMats[7] = pProjMat[3];

    uint8_t* pPacket = sub_2289bb0(*reinterpret_cast<void**>(selfBytes + 0x450), 0x04u, 0x18u, 0x04u);
    uint8_t* pPayload = nullptr;
    if (pPacket != nullptr) {
        if (selfBytes[0x401] != 0) {
            uint32_t packedKey = (*reinterpret_cast<uint32_t*>(selfBytes + 0x110) << 28) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x114) & 0x3fu) << 22) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x118) & 0x3fu) << 16) |
                                 static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(selfBytes + 0x11c));
            struct PacketCommitDesc {
                uint32_t key;
                uint32_t pad;
                void* pPacket;
                uint8_t flag0;
                uint8_t flag1;
                uint8_t padTail[6];
            } desc = {};
            desc.key = packedKey;
            desc.pPacket = pPacket;
            uint8_t f1 = selfBytes[0x403];
            desc.flag0 = (f1 != 0) ? 0 : ((selfBytes[0x402] != 0) ? 1 : 0);
            desc.flag1 = f1;
            sub_2289ca0(*reinterpret_cast<void**>(selfBytes + 0x450), &desc);
            selfBytes[0x401] = 0;
        }
        pPayload = pPacket + pPacket[1] + 4;
    }
    *reinterpret_cast<uint64_t*>(pPayload + 0x00) = GX_VMA(0x2290c60ULL);
    *reinterpret_cast<__m128**>(pPayload + 0x08) = pArenaMats;
    *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;

    sub_22911b0(selfBytes, pViewMat, pProjMat);
}

// ============================================================================
// 0x22901d0: sub_22901d0 (252 bytes)
// Enqueues a command packet for sub_2290c20 (bind resource slot)
// ============================================================================
void sub_22901d0(void* pSelf, uint32_t slot, void* pResource) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pPacket = sub_2289bb0(*reinterpret_cast<void**>(selfBytes + 0x450), 0x04u, 0x18u, 0x04u);
    uint8_t* pPayload = nullptr;
    if (pPacket != nullptr) {
        if (selfBytes[0x401] != 0) {
            uint32_t packedKey = (*reinterpret_cast<uint32_t*>(selfBytes + 0x110) << 28) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x114) & 0x3fu) << 22) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x118) & 0x3fu) << 16) |
                                 static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(selfBytes + 0x11c));
            struct PacketCommitDesc {
                uint32_t key;
                uint32_t pad;
                void* pPacket;
                uint8_t flag0;
                uint8_t flag1;
                uint8_t padTail[6];
            } desc = {};
            desc.key = packedKey;
            desc.pPacket = pPacket;
            uint8_t f1 = selfBytes[0x403];
            desc.flag0 = (f1 != 0) ? 0 : ((selfBytes[0x402] != 0) ? 1 : 0);
            desc.flag1 = f1;
            sub_2289ca0(*reinterpret_cast<void**>(selfBytes + 0x450), &desc);
            selfBytes[0x401] = 0;
        }
        pPayload = pPacket + pPacket[1] + 4;
    }
    *reinterpret_cast<uint64_t*>(pPayload + 0x00) = GX_VMA(0x2290c20ULL);
    *reinterpret_cast<uint64_t*>(pPayload + 0x08) = static_cast<uint64_t>(slot);
    *reinterpret_cast<void**>(pPayload + 0x10) = pResource;
}

// ============================================================================
// 0x22900a0: sub_22900a0 (299 bytes)
// Enqueues a command packet for sub_2290c10 (store 16-byte vector + int at +0x10)
// ============================================================================
void sub_22900a0(void* pSelf, uint32_t extraVal, const __m128* pVec) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pCmdBuf = *reinterpret_cast<uint8_t**>(selfBytes + 0x450);
    uint8_t* pArenaBlock = static_cast<uint8_t*>(sub_e45560(pCmdBuf + 0x38, 0x20u, 0x10u));
    *reinterpret_cast<uint32_t*>(pArenaBlock + 0x10) = extraVal;
    _mm_store_ps(reinterpret_cast<float*>(pArenaBlock + 0x00), *pVec);

    uint8_t* pPacket = sub_2289bb0(*reinterpret_cast<void**>(selfBytes + 0x450), 0x04u, 0x18u, 0x04u);
    uint8_t* pPayload = nullptr;
    if (pPacket != nullptr) {
        if (selfBytes[0x401] != 0) {
            uint32_t packedKey = (*reinterpret_cast<uint32_t*>(selfBytes + 0x110) << 28) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x114) & 0x3fu) << 22) |
                                 ((*reinterpret_cast<uint32_t*>(selfBytes + 0x118) & 0x3fu) << 16) |
                                 static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(selfBytes + 0x11c));
            struct PacketCommitDesc {
                uint32_t key;
                uint32_t pad;
                void* pPacket;
                uint8_t flag0;
                uint8_t flag1;
                uint8_t padTail[6];
            } desc = {};
            desc.key = packedKey;
            desc.pPacket = pPacket;
            uint8_t f1 = selfBytes[0x403];
            desc.flag0 = (f1 != 0) ? 0 : ((selfBytes[0x402] != 0) ? 1 : 0);
            desc.flag1 = f1;
            sub_2289ca0(*reinterpret_cast<void**>(selfBytes + 0x450), &desc);
            selfBytes[0x401] = 0;
        }
        pPayload = pPacket + pPacket[1] + 4;
    }
    *reinterpret_cast<uint64_t*>(pPayload + 0x00) = GX_VMA(0x2290c10ULL);
    *reinterpret_cast<uint8_t**>(pPayload + 0x08) = pArenaBlock;
    *reinterpret_cast<uint64_t*>(pPayload + 0x10) = 0;
}

} // extern "C"
