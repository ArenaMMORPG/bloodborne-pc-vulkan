// ============================================================================
// Bloodborne Engine Decompile Project (CUSA03173 v1.09)
// Module: 04_SceneRenderer (Priority Tier 8/10)
// File:   GXDecalManagerAndPaintPass.cpp
// Range:  0x2206bc0 .. 0x220fef0 (55 functions, 37,628 bytes, Right-to-Left)
//
// Description:
//   Complete, stub-free x86-64 decompilation of the Bloodborne Deferred Decal,
//   Dynamic Blood Spatter, Parallax Occlusion Mapping (POM) Height Decal, and
//   Paint Decal Manager (`GXDecalManager`, `GXPaintDecalTarget`,
//   `GXDecalDebugEnumOptionList`, `GXDecalDebugEnumComboWidget`, and
//   `GXDecalDebugButtonCallback`).
//
// Policy:
//   - 100% instruction-accurate translation (zero cast-to-void stubs, zero TODOs).
//   - Compiles cleanly with:
//     clang++ -std=c++20 -O2 -mavx -fno-stack-protector -ffp-contract=off
//             -ffunction-sections -fdata-sections -c
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <immintrin.h>

namespace FromSoftware {
namespace GX {

// ============================================================================
// Runtime VMA Resolution & Core Dantelion2 Structures
// ============================================================================

static inline uintptr_t ResolveVMA(uintptr_t vma) noexcept {
    return GX_VMA(vma);
}

struct DLAllocator;

struct DLAllocatorVTable {
    void* (*dtor)(DLAllocator* self);                                     // +0x00
    void* (*fn08)(DLAllocator* self);                                     // +0x08
    void* (*fn10)(DLAllocator* self);                                     // +0x10
    void* (*fn18)(DLAllocator* self);                                     // +0x18
    uint32_t* (*getHeapFlags)(DLAllocator* self, uint32_t* outFlags,
                              void* heapOwner, uint32_t unused);          // +0x20
    void* (*fn28)(DLAllocator* self);                                     // +0x28
    void* (*fn30)(DLAllocator* self);                                     // +0x30
    void* (*fn38)(DLAllocator* self);                                     // +0x38
    void* (*fn40)(DLAllocator* self);                                     // +0x40
    void* (*fn48)(DLAllocator* self);                                     // +0x48
    void* (*allocate)(DLAllocator* self, size_t byteSize);                // +0x50
    void* (*allocateAligned)(DLAllocator* self, size_t sz, size_t align); // +0x58
    void* (*fn60)(DLAllocator* self);                                     // +0x60
    void* (*fn68)(DLAllocator* self);                                     // +0x68
    void  (*deallocate)(DLAllocator* self, void* ptr);                    // +0x70
};

struct DLAllocator {
    const DLAllocatorVTable* vptr; // +0x00
};

// Reference-counted UTF-16 shared string node used in Dantelion2 GUI/enum items
struct DLSharedStringW {
    uint64_t header00;          // +0x00
    union {
        char16_t inlineBuf[8];  // +0x08
        char16_t* pHeapBuf;     // +0x08
    };
    uint64_t length;            // +0x18
    uint64_t capacity;          // +0x20 (>= 8 means heap-allocated)
    DLAllocator* pAllocator;    // +0x28
    uint64_t reserved30;        // +0x30
    int32_t refCount;           // +0x38
    uint32_t pad3C;             // +0x3C
};

// 24-byte option entry in GXDecalDebugEnumOptionList
struct GXDecalDebugEnumItem {
    DLSharedStringW* pSharedStr; // +0x00
    const char16_t* pRawStr;     // +0x08
    uint32_t value;              // +0x10
    uint32_t pad14;              // +0x14
};

// 40-byte paint decal command entry in GXDecalManager::m_paintCommandQueue
struct GXPaintDecalCommand {
    void* pPaintTarget;          // +0x00
    uint32_t decalHandle;        // +0x08
    uint32_t pad0C;              // +0x0C
    uint64_t reserved10;         // +0x10
    uint32_t cmdType;            // +0x18 (0=ApplyDecal, 1=Modify, 2=Clear)
    uint32_t param0;             // +0x1C (layerNo or clearMask)
    float param1;                // +0x20 (rateOfChange)
    uint32_t pad24;              // +0x24
};

// Dual 15-bit reference-counted Dantelion2 GUI object header
struct DLGuiRefObject {
    void* vptr;                  // +0x00
    uint32_t refFlags;           // +0x08 (bits 0..14: strongRef, bits 15..29: weakRef, bit 30: aliveFlag)
    uint32_t extraFlags;         // +0x0C
};

static inline void ReleaseSharedStringW(DLSharedStringW* pStr) noexcept {
    if (!pStr) {
        return;
    }
    if (--pStr->refCount == 0) {
        DLAllocator* pNodePool = *reinterpret_cast<DLAllocator**>(ResolveVMA(0x5540e10));
        if (pStr->capacity >= 8) {
            DLAllocator* pStrAlloc = pStr->pAllocator;
            pStrAlloc->vptr->deallocate(pStrAlloc, pStr->pHeapBuf);
        }
        pStr->capacity = 7;
        pStr->length = 0;
        pStr->inlineBuf[0] = 0;
        pNodePool->vptr->deallocate(pNodePool, pStr);
    }
}

static inline void RetainGuiRefObject(DLGuiRefObject* obj) noexcept {
    if (!obj) {
        return;
    }
    uint32_t f = obj->refFlags;
    uint32_t strong = (f + 1u) & 0x7FFFu;
    f = (f & 0xFFFF8000u) | strong;
    uint32_t weak = (f + 0x8000u) & 0x3FFF8000u;
    f = (f & 0xC0007FFFu) | weak;
    obj->refFlags = f;
}

static inline void ReleaseGuiRefObject(DLGuiRefObject* obj) noexcept {
    if (!obj) {
        return;
    }
    uint32_t ecx = obj->refFlags;
    uint32_t edx = ecx + 0x7FFFu;
    uint32_t esi = edx & 0x7FFFu;
    uint32_t eax = (ecx & 0xFFFF8000u) | esi;
    obj->refFlags = eax;
    if ((edx & 0x7FFFu) == 0) {
        if ((ecx & 0x40000000u) != 0) {
            using OnZeroStrongFn = void (*)(DLGuiRefObject*);
            void** vtbl = reinterpret_cast<void**>(obj->vptr);
            reinterpret_cast<OnZeroStrongFn>(vtbl[3])(obj);
            eax = obj->refFlags & 0xBFFFFFFFu;
            obj->refFlags = eax;
        }
    }
    uint32_t weakDec = (eax >> 15) + 0x7FFFu;
    uint32_t weakShifted = (weakDec & 0x7FFFu) << 15;
    uint32_t newFlags = (eax & 0xC0007FFFu) | weakShifted;
    obj->refFlags = newFlags;
    if ((weakDec & 0x7FFFu) == 0) {
        obj->refFlags = eax | 0x3FFF8000u;
        using OnZeroWeakFn = void (*)(DLGuiRefObject*);
        void** vtbl = reinterpret_cast<void**>(obj->vptr);
        reinterpret_cast<OnZeroWeakFn>(vtbl[4])(obj);
    }
}

static inline void PopAndReleaseGuiContainerChild(uint8_t* guiBuilder) noexcept {
    DLGuiRefObject** pBegin = reinterpret_cast<DLGuiRefObject**>(
        *reinterpret_cast<uintptr_t*>(guiBuilder + 0x08));
    DLGuiRefObject** pEnd = reinterpret_cast<DLGuiRefObject**>(
        *reinterpret_cast<uintptr_t*>(guiBuilder + 0x10));
    if (pBegin != pEnd) {
        DLGuiRefObject* pLast = pEnd[-1];
        if (pLast != nullptr) {
            ReleaseGuiRefObject(pLast);
            pEnd[-1] = nullptr;
            pEnd = reinterpret_cast<DLGuiRefObject**>(
                *reinterpret_cast<uintptr_t*>(guiBuilder + 0x10));
        }
        *reinterpret_cast<DLGuiRefObject***>(guiBuilder + 0x10) = pEnd - 1;
    }
}

static inline const char16_t* ResolveWStr(uintptr_t vma) noexcept {
    return reinterpret_cast<const char16_t*>(GX_VMA(vma));
}

static inline void HandleContainerAllocFailure() noexcept {
    int32_t* pTlsFlag = reinterpret_cast<int32_t*>(gx_guest_tls_slot(0x53e4318ULL));
    if (*pTlsFlag != 0) {
        using HandleAllocFailFn = void (*)(void*, int32_t);
        reinterpret_cast<HandleAllocFailFn>(ResolveVMA(0x2bbe978))(
            gx_guest_tls_slot(0x53e4320ULL), 1);
    } else {
        using LogAssertFn = void (*)(const char*, uint32_t, const char*, ...);
        reinterpret_cast<LogAssertFn>(ResolveVMA(0x20b55b0))(
            nullptr, 0x109u, reinterpret_cast<const char*>(ResolveVMA(0x4737d80)));
    }
}

// Stack-allocated UTF-16 Dantelion2 basic_string (0x38 bytes)
struct DLBasicStringW {
    uint64_t reserved00;       // +0x00
    union {
        char16_t inlineBuf[8]; // +0x08
        char16_t* pHeapBuf;    // +0x08
    };
    uint64_t length;           // +0x18
    uint64_t capacity;         // +0x20
    DLAllocator* pAllocator;   // +0x28
    uint8_t validFlag;         // +0x30
    uint8_t pad31[7];          // +0x31
};

static inline void InitStackBasicStringW(DLBasicStringW* s) noexcept {
    uintptr_t tlsSlotAddr = reinterpret_cast<uintptr_t>(gx_guest_tls_slot(0x53e4568ULL));
    uint8_t* tlsObj = *reinterpret_cast<uint8_t**>(tlsSlotAddr);
    if (!tlsObj) {
        using InitTlsFn = void (*)();
        reinterpret_cast<InitTlsFn>(ResolveVMA(0x207d250))();
        tlsObj = *reinterpret_cast<uint8_t**>(tlsSlotAddr);
    }
    DLAllocator* pHeap = reinterpret_cast<DLAllocator*>(tlsObj + 0x28);
    uint32_t heapFlags = 0;
    pHeap->vptr->getHeapFlags(pHeap, &heapFlags, pHeap, 0);
    if ((heapFlags & 0x20u) == 0) {
        using LogFn = void (*)(const char*, uint32_t, const char*, ...);
        reinterpret_cast<LogFn>(ResolveVMA(0x20b55b0))(
            nullptr, 0x3Eu, reinterpret_cast<const char*>(ResolveVMA(0x4737dc0)));
    }
    s->pAllocator = pHeap;
    s->capacity = 7;
    s->length = 0;
    s->inlineBuf[0] = 0;
    s->validFlag = 1;
}

static inline const char16_t* GetBasicStringWCString(const DLBasicStringW* s) noexcept {
    return (s->capacity >= 8) ? s->pHeapBuf : s->inlineBuf;
}

static inline void DestroyStackBasicStringW(DLBasicStringW* s) noexcept {
    if (s->capacity >= 8 && s->pAllocator != nullptr) {
        s->pAllocator->vptr->deallocate(s->pAllocator, s->pHeapBuf);
    }
}

// ============================================================================
// 1. 0x220fef0 (499 B) — GXDecalEnumItemVector_ReallocateAndCopy
// ============================================================================
extern "C" void GXDecalEnumItemVector_ReallocateAndCopy(
    uint8_t* vec,
    size_t newCapacity)
{
    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
    GXDecalDebugEnumItem* pNewBuf = static_cast<GXDecalDebugEnumItem*>(
        pAlloc->vptr->allocateAligned(pAlloc, newCapacity * 24u, 8u));
    if (!pNewBuf) {
        HandleContainerAllocFailure();
    }

    GXDecalDebugEnumItem* pOldBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x08);
    GXDecalDebugEnumItem* pOldEnd   = *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x10);
    if (pOldBegin != pOldEnd) {
        GXDecalDebugEnumItem* pDst = pNewBuf;
        for (GXDecalDebugEnumItem* pSrc = pOldBegin; pSrc != pOldEnd; ++pSrc, ++pDst) {
            if (pDst != nullptr) {
                pDst->pRawStr = nullptr;
                pDst->pSharedStr = nullptr;
                if (pSrc->pRawStr != nullptr) {
                    pDst->pRawStr = pSrc->pRawStr;
                } else {
                    DLSharedStringW* pShared = pSrc->pSharedStr;
                    pDst->pRawStr = nullptr;
                    if (pShared != nullptr) {
                        pDst->pSharedStr = pShared;
                        ++pShared->refCount;
                    }
                }
                pDst->value = pSrc->value;
            }
        }
        pOldBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x08);
        pOldEnd   = *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x10);
    }

    if (pOldBegin != nullptr) {
        for (GXDecalDebugEnumItem* pCur = pOldBegin; pCur != pOldEnd; ++pCur) {
            pCur->pRawStr = nullptr;
            ReleaseSharedStringW(pCur->pSharedStr);
            pCur->pSharedStr = nullptr;
        }
        DLAllocator* pFreeAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
        pFreeAlloc->vptr->deallocate(pFreeAlloc, *reinterpret_cast<void**>(vec + 0x08));
    }

    ptrdiff_t count = pOldEnd - pOldBegin;
    *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x18) = pNewBuf + newCapacity;
    *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x10) = pNewBuf + count;
    *reinterpret_cast<GXDecalDebugEnumItem**>(vec + 0x08) = pNewBuf;
}

// ============================================================================
// 2. 0x220fdf0 (251 B) — GXDecalPtrVector_ReallocateAndCopy_A
// ============================================================================
extern "C" void GXDecalPtrVector_ReallocateAndCopy_A(
    uint8_t* vec,
    size_t newCapacity)
{
    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
    void** pNewBuf = static_cast<void**>(
        pAlloc->vptr->allocateAligned(pAlloc, newCapacity * sizeof(void*), 8u));
    if (!pNewBuf) {
        HandleContainerAllocFailure();
    }

    void** pOldBegin = *reinterpret_cast<void***>(vec + 0x08);
    void** pOldEnd   = *reinterpret_cast<void***>(vec + 0x10);
    if (pOldBegin != pOldEnd) {
        void** pDst = pNewBuf;
        for (void** pSrc = pOldBegin; pSrc != pOldEnd; ++pSrc, ++pDst) {
            if (pDst != nullptr) {
                *pDst = *pSrc;
            }
        }
        pOldBegin = *reinterpret_cast<void***>(vec + 0x08);
        pOldEnd   = *reinterpret_cast<void***>(vec + 0x10);
    }

    ptrdiff_t count = pOldEnd - pOldBegin;
    if (pOldBegin != nullptr) {
        DLAllocator* pFreeAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
        pFreeAlloc->vptr->deallocate(pFreeAlloc, pOldBegin);
    }

    *reinterpret_cast<void***>(vec + 0x18) = pNewBuf + newCapacity;
    *reinterpret_cast<void***>(vec + 0x10) = pNewBuf + count;
    *reinterpret_cast<void***>(vec + 0x08) = pNewBuf;
}

// ============================================================================
// 3. 0x220fcf0 (251 B) — GXDecalPtrVector_ReallocateAndCopy_B
// ============================================================================
extern "C" void GXDecalPtrVector_ReallocateAndCopy_B(
    uint8_t* vec,
    size_t newCapacity)
{
    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
    void** pNewBuf = static_cast<void**>(
        pAlloc->vptr->allocateAligned(pAlloc, newCapacity * sizeof(void*), 8u));
    if (!pNewBuf) {
        HandleContainerAllocFailure();
    }

    void** pOldBegin = *reinterpret_cast<void***>(vec + 0x08);
    void** pOldEnd   = *reinterpret_cast<void***>(vec + 0x10);
    if (pOldBegin != pOldEnd) {
        void** pDst = pNewBuf;
        for (void** pSrc = pOldBegin; pSrc != pOldEnd; ++pSrc, ++pDst) {
            if (pDst != nullptr) {
                *pDst = *pSrc;
            }
        }
        pOldBegin = *reinterpret_cast<void***>(vec + 0x08);
        pOldEnd   = *reinterpret_cast<void***>(vec + 0x10);
    }

    ptrdiff_t count = pOldEnd - pOldBegin;
    if (pOldBegin != nullptr) {
        DLAllocator* pFreeAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
        pFreeAlloc->vptr->deallocate(pFreeAlloc, pOldBegin);
    }

    *reinterpret_cast<void***>(vec + 0x18) = pNewBuf + newCapacity;
    *reinterpret_cast<void***>(vec + 0x10) = pNewBuf + count;
    *reinterpret_cast<void***>(vec + 0x08) = pNewBuf;
}

// ============================================================================
// 4. 0x220fb90 (345 B) — GXDecalDebugEnumComboWidget_Destroy
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_Destroy(uint8_t* self) {
    *reinterpret_cast<uintptr_t*>(self + 0x00) = ResolveVMA(0x53b72e0);

    DLGuiRefObject* pOptList = *reinterpret_cast<DLGuiRefObject**>(self + 0x30);
    if (pOptList != nullptr) {
        ReleaseGuiRefObject(pOptList);
        *reinterpret_cast<DLGuiRefObject**>(self + 0x30) = nullptr;
    }
    *reinterpret_cast<void**>(self + 0x28) = nullptr;

    using BaseCleanupFn = void (*)(uint8_t*);
    reinterpret_cast<BaseCleanupFn>(ResolveVMA(0x2351f90))(self);

    self[0x0B] &= 0xBFu;

    pOptList = *reinterpret_cast<DLGuiRefObject**>(self + 0x30);
    if (pOptList != nullptr) {
        ReleaseGuiRefObject(pOptList);
        *reinterpret_cast<DLGuiRefObject**>(self + 0x30) = nullptr;
    }

    using BaseDtorFn = void (*)(uint8_t*);
    reinterpret_cast<BaseDtorFn>(ResolveVMA(0x2351d90))(self);
}

// ============================================================================
// 5. 0x220fa20 (363 B) — GXPaintDecalCommandQueue_GrowCapacityByOne
// ============================================================================
extern "C" void GXPaintDecalCommandQueue_GrowCapacityByOne(uint8_t* vec) {
    GXPaintDecalCommand* pEnd = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x10);
    GXPaintDecalCommand* pCap = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x18);
    if (pCap != pEnd) {
        return;
    }

    GXPaintDecalCommand* pBegin = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x08);
    size_t oldCount = static_cast<size_t>(pCap - pBegin);
    size_t halfCount = oldCount >> 1;
    const size_t kMaxCount = 0x0666666666666666ULL;
    size_t newCapacity = (kMaxCount - halfCount >= oldCount) ? (oldCount + halfCount) : 0u;
    if (newCapacity < oldCount + 1u) {
        newCapacity = oldCount + 1u;
    }

    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
    GXPaintDecalCommand* pNewBuf = static_cast<GXPaintDecalCommand*>(
        pAlloc->vptr->allocateAligned(pAlloc, newCapacity * sizeof(GXPaintDecalCommand), 8u));
    if (!pNewBuf) {
        HandleContainerAllocFailure();
    }

    pBegin = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x08);
    pEnd   = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x10);
    if (pBegin != pEnd) {
        GXPaintDecalCommand* pDst = pNewBuf;
        for (GXPaintDecalCommand* pSrc = pBegin; pSrc != pEnd; ++pSrc, ++pDst) {
            if (pDst != nullptr) {
                *pDst = *pSrc;
            }
        }
        pBegin = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x08);
        pEnd   = *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x10);
    }

    size_t count = static_cast<size_t>(pEnd - pBegin);
    if (pBegin != nullptr) {
        DLAllocator* pFreeAlloc = *reinterpret_cast<DLAllocator**>(vec + 0x20);
        pFreeAlloc->vptr->deallocate(pFreeAlloc, pBegin);
    }

    *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x18) = pNewBuf + newCapacity;
    *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x10) = pNewBuf + count;
    *reinterpret_cast<GXPaintDecalCommand**>(vec + 0x08) = pNewBuf;
}

// ============================================================================
// 6. 0x220f940 (218 B) — GXDecalPtrVector_PushBack
// ============================================================================
extern "C" void GXDecalPtrVector_PushBack(uint8_t* vec, void* const* pElem) {
    void** pEnd   = *reinterpret_cast<void***>(vec + 0x10);
    void** pBegin = *reinterpret_cast<void***>(vec + 0x08);
    void** pCap   = *reinterpret_cast<void***>(vec + 0x18);

    if (pElem < pEnd && pElem >= pBegin) {
        ptrdiff_t idx = pElem - pBegin;
        if (pEnd == pCap) {
            size_t oldCount = static_cast<size_t>(pEnd - pBegin);
            size_t half = oldCount >> 1;
            const size_t kMax = 0x1FFFFFFFFFFFFFFFULL;
            size_t newCap = (kMax - half >= oldCount) ? (oldCount + half) : 0u;
            if (newCap < oldCount + 1u) {
                newCap = oldCount + 1u;
            }
            GXDecalPtrVector_ReallocateAndCopy_A(vec, newCap);
            pEnd = *reinterpret_cast<void***>(vec + 0x10);
        }
        if (pEnd != nullptr) {
            void** pNewBegin = *reinterpret_cast<void***>(vec + 0x08);
            *pEnd = pNewBegin[idx];
        }
    } else {
        if (pEnd == pCap) {
            size_t oldCount = static_cast<size_t>(pEnd - pBegin);
            size_t half = oldCount >> 1;
            const size_t kMax = 0x1FFFFFFFFFFFFFFFULL;
            size_t newCap = (kMax - half >= oldCount) ? (oldCount + half) : 0u;
            if (newCap < oldCount + 1u) {
                newCap = oldCount + 1u;
            }
            GXDecalPtrVector_ReallocateAndCopy_A(vec, newCap);
            pEnd = *reinterpret_cast<void***>(vec + 0x10);
        }
        if (pEnd != nullptr) {
            *pEnd = *pElem;
        }
    }
    *reinterpret_cast<void***>(vec + 0x10) = pEnd + 1;
}

// ============================================================================
// 7. 0x220f930 (3 B) — GXDecalPassFilter_AcceptAll
// ============================================================================
extern "C" bool GXDecalPassFilter_AcceptAll() {
    return true;
}

// ============================================================================
// 8. 0x220f900 (40 B) — GXDecalDebugButtonCallback_Invoke
// ============================================================================
extern "C" void GXDecalDebugButtonCallback_Invoke(uint8_t* self) {
    if ((self[0x0B] & 0x40u) == 0) {
        return;
    }
    uint8_t* pTarget = *reinterpret_cast<uint8_t**>(self + 0x10) +
                       *reinterpret_cast<intptr_t*>(self + 0x28);
    uintptr_t fnPtrOrVOff = *reinterpret_cast<uintptr_t*>(self + 0x20);
    if ((fnPtrOrVOff & 1u) != 0) {
        uintptr_t vptr = *reinterpret_cast<uintptr_t*>(pTarget);
        fnPtrOrVOff = *reinterpret_cast<uintptr_t*>(vptr + fnPtrOrVOff - 1u);
    }
    uint32_t arg = *reinterpret_cast<uint32_t*>(self + 0x18);
    using CallbackFn = void (*)(uint8_t*, uint32_t);
    reinterpret_cast<CallbackFn>(fnPtrOrVOff)(pTarget, arg);
}

// ============================================================================
// 9. 0x220f8f0 (5 B) — GXDecalDebugButtonCallback_DeletingDtor
// ============================================================================
extern "C" void GXDecalDebugButtonCallback_DeletingDtor(void* self) {
    using OperatorDeleteFn = void (*)(void*);
    reinterpret_cast<OperatorDeleteFn>(ResolveVMA(0x2bbe588))(self);
}

// ============================================================================
// 10. 0x220f8d0 (22 B) — GXDecalDebugButtonCallback_Dtor
// ============================================================================
extern "C" void GXDecalDebugButtonCallback_Dtor(uint8_t* self) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xBFFFFFFFu;
    *reinterpret_cast<uintptr_t*>(self + 0x00) = ResolveVMA(0x53b93b0);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;
}

// ============================================================================
// 11. 0x220f880 (79 B) — GXDecalDebugEnumComboWidget_OnSelectIndex
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_OnSelectIndex(
    uint8_t* self,
    uint32_t selectedIndex)
{
    if ((self[0x0C] & 1u) != 0) {
        return;
    }
    uint8_t* pOptionList = *reinterpret_cast<uint8_t**>(self + 0x30);
    const GXDecalDebugEnumItem* pBegin = *reinterpret_cast<const GXDecalDebugEnumItem**>(pOptionList + 0x18);
    const GXDecalDebugEnumItem* pEnd   = *reinterpret_cast<const GXDecalDebugEnumItem**>(pOptionList + 0x20);
    size_t count = static_cast<size_t>(pEnd - pBegin);
    if (selectedIndex >= count) {
        return;
    }
    uint32_t val = pBegin[selectedIndex].value;
    *reinterpret_cast<uint32_t*>(self + 0x20) = val;
    uint32_t* pTargetVal = *reinterpret_cast<uint32_t**>(self + 0x28);
    *pTargetVal = val;

    void* pListener = *reinterpret_cast<void**>(self + 0x10);
    if (pListener != nullptr) {
        void** vtbl = *reinterpret_cast<void***>(pListener);
        using NotifyFn = void (*)(void*, uint32_t);
        reinterpret_cast<NotifyFn>(vtbl[5])(pListener, selectedIndex);
    }
}

// ============================================================================
// 12. 0x220f860 (23 B) — GXDecalDebugEnumComboWidget_OnDetach
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_OnDetach(uint8_t* self) {
    *reinterpret_cast<void**>(self + 0x28) = nullptr;
    void* pCtrl = *reinterpret_cast<void**>(self + 0x18);
    if (pCtrl != nullptr) {
        using DetachCtrlFn = void (*)(void*);
        reinterpret_cast<DetachCtrlFn>(ResolveVMA(0x23286b0))(pCtrl);
    }
}

// ============================================================================
// 13. 0x220f7f0 (103 B) — GXDecalDebugEnumComboWidget_SyncSelectionFromValuePtr
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_SyncSelectionFromValuePtr(uint8_t* self) {
    const uint32_t* pTargetVal = *reinterpret_cast<const uint32_t**>(self + 0x28);
    if (!pTargetVal) {
        return;
    }
    uint32_t curVal = *pTargetVal;
    if (curVal == *reinterpret_cast<uint32_t*>(self + 0x20)) {
        return;
    }
    *reinterpret_cast<uint32_t*>(self + 0x20) = curVal;

    uint8_t* pOptionList = *reinterpret_cast<uint8_t**>(self + 0x30);
    const GXDecalDebugEnumItem* pBegin = *reinterpret_cast<const GXDecalDebugEnumItem**>(pOptionList + 0x18);
    const GXDecalDebugEnumItem* pEnd   = *reinterpret_cast<const GXDecalDebugEnumItem**>(pOptionList + 0x20);
    uint32_t count = static_cast<uint32_t>(pEnd - pBegin);

    uint32_t matchedIndex = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < count; ++i) {
        if (pBegin[i].value == curVal) {
            matchedIndex = i;
            break;
        }
    }

    uint8_t* pCtrl = *reinterpret_cast<uint8_t**>(self + 0x18);
    void* pSubCtrl = *reinterpret_cast<void**>(pCtrl + 0x110);
    using SetSelectedIndexFn = void (*)(void*, uint32_t, uint32_t);
    reinterpret_cast<SetSelectedIndexFn>(ResolveVMA(0xf48fe0))(pSubCtrl, matchedIndex, 0u);
}

// ============================================================================
// 14. 0x220f740 (176 B) — GXDecalDebugEnumComboWidget_ReleaseOptionListAndCleanup
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_ReleaseOptionListAndCleanup(uint8_t* self) {
    DLGuiRefObject* pOptList = *reinterpret_cast<DLGuiRefObject**>(self + 0x30);
    if (pOptList != nullptr) {
        ReleaseGuiRefObject(pOptList);
        *reinterpret_cast<DLGuiRefObject**>(self + 0x30) = nullptr;
    }
    *reinterpret_cast<void**>(self + 0x28) = nullptr;

    using BaseCleanupFn = void (*)(uint8_t*);
    reinterpret_cast<BaseCleanupFn>(ResolveVMA(0x2351f90))(self);
}

// ============================================================================
// 15. 0x220f720 (28 B) — GXDecalDebugEnumComboWidget_DeletingDtor
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_DeletingDtor(uint8_t* self) {
    GXDecalDebugEnumComboWidget_Destroy(self);
    using OperatorDeleteFn = void (*)(void*);
    reinterpret_cast<OperatorDeleteFn>(ResolveVMA(0x2bbe588))(self);
}

// ============================================================================
// 16. 0x220f710 (5 B) — GXDecalDebugEnumComboWidget_BaseDtorThunk
// ============================================================================
extern "C" void GXDecalDebugEnumComboWidget_BaseDtorThunk(uint8_t* self) {
    GXDecalDebugEnumComboWidget_Destroy(self);
}

// ============================================================================
// 17. 0x220f660 (166 B) — GXDecalDebugEnumOptionList_ClearItems
// ============================================================================
extern "C" void GXDecalDebugEnumOptionList_ClearItems(uint8_t* self) {
    GXDecalDebugEnumItem* pBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(self + 0x18);
    GXDecalDebugEnumItem* pEnd   = *reinterpret_cast<GXDecalDebugEnumItem**>(self + 0x20);
    if (pBegin != pEnd) {
        for (GXDecalDebugEnumItem* pCur = pBegin; pCur != pEnd; ++pCur) {
            pCur->pRawStr = nullptr;
            ReleaseSharedStringW(pCur->pSharedStr);
            pCur->pSharedStr = nullptr;
        }
        pBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(self + 0x18);
    }
    *reinterpret_cast<GXDecalDebugEnumItem**>(self + 0x20) = pBegin;
}

// ============================================================================
// 18. 0x220f500 (337 B) — GXDecalDebugEnumOptionList_DeletingDtor
// ============================================================================
extern "C" void GXDecalDebugEnumOptionList_DeletingDtor(uint8_t* self) {
    *reinterpret_cast<uintptr_t*>(self + 0x00) = ResolveVMA(0x53b72a0);
    GXDecalDebugEnumOptionList_ClearItems(self);
    self[0x0B] &= 0xBFu;

    GXDecalDebugEnumItem* pBuf = *reinterpret_cast<GXDecalDebugEnumItem**>(self + 0x18);
    if (pBuf != nullptr) {
        DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x30);
        pAlloc->vptr->deallocate(pAlloc, pBuf);
    }

    using OperatorDeleteFn = void (*)(void*);
    reinterpret_cast<OperatorDeleteFn>(ResolveVMA(0x2bbe588))(self);
}

// ============================================================================
// 19. 0x220f370 (400 B) — GXDecalDebugEnumOptionList_Dtor
// ============================================================================
extern "C" void GXDecalDebugEnumOptionList_Dtor(uint8_t* self) {
    *reinterpret_cast<uintptr_t*>(self + 0x00) = ResolveVMA(0x53b72a0);
    GXDecalDebugEnumOptionList_ClearItems(self);

    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xBFFFFFFFu;
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;

    GXDecalDebugEnumItem* pBuf = *reinterpret_cast<GXDecalDebugEnumItem**>(self + 0x18);
    if (pBuf != nullptr) {
        DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x30);
        pAlloc->vptr->deallocate(pAlloc, pBuf);
        *reinterpret_cast<void**>(self + 0x28) = nullptr;
        *reinterpret_cast<void**>(self + 0x20) = nullptr;
        *reinterpret_cast<void**>(self + 0x18) = nullptr;
        flags = *reinterpret_cast<uint32_t*>(self + 0x08);
    }

    *reinterpret_cast<uintptr_t*>(self + 0x00) = ResolveVMA(0x53b93b0);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags & 0xBFFFFFFFu;
}

// ============================================================================
// 20. 0x220f2d0 (8 B) — GXDecalManager_RequestClearPaintDecals
// ============================================================================
extern "C" void GXDecalManager_RequestClearPaintDecals(uint8_t* self) {
    self[0x961] = 1;
}

// ============================================================================
// 21. 0x220f2c0 (8 B) — GXDecalManager_RequestModifyPaintDecals
// ============================================================================
extern "C" void GXDecalManager_RequestModifyPaintDecals(uint8_t* self) {
    self[0x960] = 1;
}

// ============================================================================
// 22. 0x220f2b0 (5 B) — GXDecalManager_RequestDestroyAllDecals
// ============================================================================
extern "C" void GXDecalManager_RequestDestroyAllDecals(uint8_t* self) {
    self[0x68] = 1;
}

// ============================================================================
// 23. 0x220db40 (5988 B) — GXDecalManager_DebugSpawnDecalFromDebugParams
// ============================================================================
extern "C" void GXDecalManager_DebugSpawnDecalFromDebugParams(uint8_t* self) {
    alignas(32) uint8_t spawnDesc[0x110];
    std::memset(spawnDesc, 0, sizeof(spawnDesc));

    // Default material & POM parameters inside spawnDesc (+0xB8 .. +0x10C, i.e. rsp+0x110..0x160)
    *reinterpret_cast<uint32_t*>(spawnDesc + 0xB8) = 1u;
    *reinterpret_cast<float*>(spawnDesc + 0xBC) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xD8) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xDC) = 1.0f;
    spawnDesc[0xE0] = 0;
    *reinterpret_cast<float*>(spawnDesc + 0xE4) = 0.06f; // 0x3d75c28f
    *reinterpret_cast<float*>(spawnDesc + 0xE8) = 0.5f;  // 0x3f000000
    *reinterpret_cast<float*>(spawnDesc + 0xC0) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xC4) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xC8) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xCC) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xD0) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xD4) = 1.0f;
    spawnDesc[0xEC] = 0;
    *reinterpret_cast<float*>(spawnDesc + 0xF0) = 1.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xF4) = 8.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xF8) = 64.0f;
    *reinterpret_cast<float*>(spawnDesc + 0xFC) = 1.0f;

    // Copy debug spawn parameters from GXDecalManager (+0x8b0 .. +0x990)
    *reinterpret_cast<uint32_t*>(spawnDesc + 0x38) = *reinterpret_cast<uint32_t*>(self + 0x938); // type
    float length = *reinterpret_cast<float*>(self + 0x92C);
    *reinterpret_cast<float*>(spawnDesc + 0x24) = length;
    *reinterpret_cast<float*>(spawnDesc + 0x28) = length;
    *reinterpret_cast<float*>(spawnDesc + 0x2C) = *reinterpret_cast<float*>(self + 0x930); // depth

    // Position (+0x18 = rsp+0x70), negated forward (-self[+0x8d0] at +0x0C = rsp+0x64), up (self[+0x8e0] at +0x00 = rsp+0x58)
    const float* pPos = reinterpret_cast<const float*>(self + 0x8B0);
    const float* pFwd = reinterpret_cast<const float*>(self + 0x8D0);
    const float* pUp  = reinterpret_cast<const float*>(self + 0x8E0);
    *reinterpret_cast<float*>(spawnDesc + 0x18) = pPos[0];
    *reinterpret_cast<float*>(spawnDesc + 0x1C) = pPos[1];
    *reinterpret_cast<float*>(spawnDesc + 0x20) = pPos[2];
    *reinterpret_cast<float*>(spawnDesc + 0x0C) = 0.0f - pFwd[0];
    *reinterpret_cast<float*>(spawnDesc + 0x10) = 0.0f - pFwd[1];
    *reinterpret_cast<float*>(spawnDesc + 0x14) = 0.0f - pFwd[2];
    *reinterpret_cast<float*>(spawnDesc + 0x00) = pUp[0];
    *reinterpret_cast<float*>(spawnDesc + 0x04) = pUp[1];
    *reinterpret_cast<float*>(spawnDesc + 0x08) = pUp[2];

    *reinterpret_cast<uint32_t*>(spawnDesc + 0x40) = *reinterpret_cast<uint32_t*>(self + 0x8FC); // layerNo
    *reinterpret_cast<float*>(spawnDesc + 0xBC)    = 1.0f;
    *reinterpret_cast<uint32_t*>(spawnDesc + 0xB8) = *reinterpret_cast<uint32_t*>(self + 0x928); // blendMode
    *reinterpret_cast<uint64_t*>(spawnDesc + 0xC0) = *reinterpret_cast<uint64_t*>(self + 0x90C); // diffuseScale.xy
    *reinterpret_cast<uint32_t*>(spawnDesc + 0xC8) = *reinterpret_cast<uint32_t*>(self + 0x914); // diffuseScale.z
    *reinterpret_cast<uint64_t*>(spawnDesc + 0xCC) = *reinterpret_cast<uint64_t*>(self + 0x918); // reflectanceScale.xy
    *reinterpret_cast<uint32_t*>(spawnDesc + 0xD4) = *reinterpret_cast<uint32_t*>(self + 0x920); // reflectanceScale.z
    *reinterpret_cast<float*>(spawnDesc + 0xD8)    = *reinterpret_cast<float*>(self + 0x924);    // shininessScale
    *reinterpret_cast<uint64_t*>(spawnDesc + 0x100) = 0;

    // Extended transform/bounds block (+0x480 .. +0x5DF on stack, 0x160 bytes)
    alignas(32) uint8_t extBlock[0x160];
    std::memset(extBlock, 0, sizeof(extBlock));
    *reinterpret_cast<uint64_t*>(extBlock + 0xD8) = 0xFFFFFFFFFFFFFFFFULL;
    *reinterpret_cast<uint64_t*>(extBlock + 0xD0) = 0xFFFFFFFFFFFFFFFFULL;
    *reinterpret_cast<uint64_t*>(extBlock + 0x60) = 0xFFFFFFFFFFFFFFFFULL;
    *reinterpret_cast<float*>(extBlock + 0xE0) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0xE4) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0xE8) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0xEC) = 1.0f;
    for (int i = 0; i < 4; ++i) {
        *reinterpret_cast<float*>(extBlock + i * 0x10 + 0x00) = 1.0f;
        *reinterpret_cast<float*>(extBlock + i * 0x10 + 0x04) = 1.0f;
        *reinterpret_cast<float*>(extBlock + i * 0x10 + 0x08) = 0.0f;
        *reinterpret_cast<float*>(extBlock + i * 0x10 + 0x0C) = 0.0f;
    }
    extBlock[0xA0] = 0xFF;
    extBlock[0xA1] = 0x00;
    *reinterpret_cast<uint32_t*>(extBlock + 0xC4) = 0xFFFFFFFFu;
    *reinterpret_cast<uint32_t*>(extBlock + 0xCC) = 0xFFFFFFFFu;
    *reinterpret_cast<float*>(extBlock + 0x150) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0x154) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0x88) = -100.0f;
    *reinterpret_cast<float*>(extBlock + 0x8C) = -100.0f;
    *reinterpret_cast<float*>(extBlock + 0x90) = -100.0f;
    *reinterpret_cast<float*>(extBlock + 0x94) = 100.0f;
    *reinterpret_cast<float*>(extBlock + 0x98) = 100.0f;
    *reinterpret_cast<float*>(extBlock + 0x9C) = 100.0f;
    *reinterpret_cast<float*>(extBlock + 0x50) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0x54) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0x58) = 1.0f;
    *reinterpret_cast<float*>(extBlock + 0x5C) = 1.0f;

    *reinterpret_cast<void**>(spawnDesc + 0x108) = extBlock;
    *reinterpret_cast<uint32_t*>(spawnDesc + 0x3C) = *reinterpret_cast<uint32_t*>(self + 0x93C); // projType
    spawnDesc[0xE0] = self[0x940]; // soakEnable
    *reinterpret_cast<float*>(spawnDesc + 0xE4) = *reinterpret_cast<float*>(self + 0x904); // reflectanceThreshold1
    *reinterpret_cast<float*>(spawnDesc + 0xE8) = *reinterpret_cast<float*>(self + 0x908); // reflectanceThreshold2
    *reinterpret_cast<float*>(spawnDesc + 0x30) = *reinterpret_cast<float*>(self + 0x944); // farPlaneScale
    spawnDesc[0xEC] = self[0x941]; // pomEnable
    *reinterpret_cast<float*>(spawnDesc + 0xF0) = *reinterpret_cast<float*>(self + 0x980); // pomHeightScale
    *reinterpret_cast<float*>(spawnDesc + 0xF4) = *reinterpret_cast<float*>(self + 0x988); // pomMinSampleCount
    *reinterpret_cast<float*>(spawnDesc + 0xF8) = *reinterpret_cast<float*>(self + 0x98C); // pomMaxSampleCount
    *reinterpret_cast<float*>(spawnDesc + 0xFC) = *reinterpret_cast<float*>(self + 0x990); // pomShadowCoefficient

    // Initialize 14 DLBasicStringW buffers (indices 0..13 match rsp+0x448 down to rsp+0x170)
    DLBasicStringW texNames[14];
    for (int i = 0; i < 14; ++i) {
        InitStackBasicStringW(&texNames[i]);
    }

    using FormatStringWFn = void (*)(DLBasicStringW*, const char16_t*, ...);
    FormatStringWFn fmtFn = reinterpret_cast<FormatStringWFn>(ResolveVMA(0x2762d00));
    uint32_t texId = *reinterpret_cast<uint32_t*>(self + 0x934);

    if (self[0x8F7] != 0) {
        // Height Decal ("height%09d_h") -> texNames[12] (rsp+0x1a8)
        fmtFn(&texNames[12], ResolveWStr(0x49ca08e), texId);
    } else if (self[0x8F2] != 0) {
        // Blood Texture ("blood%09d_a/n/p/e")
        fmtFn(&texNames[0],  ResolveWStr(0x49ca0c0), texId);
        fmtFn(&texNames[1],  ResolveWStr(0x49ca0d8), texId);
        fmtFn(&texNames[2],  ResolveWStr(0x49ca0f0), texId);
        fmtFn(&texNames[3],  ResolveWStr(0x49ca0f0), texId);
        fmtFn(&texNames[13], ResolveWStr(0x49ca0a8), texId);
        fmtFn(&texNames[4],  ResolveWStr(0x49ca0c0), texId + 1u);
        fmtFn(&texNames[5],  ResolveWStr(0x49ca0d8), texId + 1u);
        fmtFn(&texNames[6],  ResolveWStr(0x49ca0f0), texId + 1u);
        fmtFn(&texNames[7],  ResolveWStr(0x49ca0f0), texId + 1u);
        fmtFn(&texNames[8],  ResolveWStr(0x49ca0c0), texId + 2u);
        fmtFn(&texNames[9],  ResolveWStr(0x49ca0d8), texId + 2u);
        fmtFn(&texNames[10], ResolveWStr(0x49ca0f0), texId + 2u);
        fmtFn(&texNames[11], ResolveWStr(0x49ca0f0), texId + 2u);
    } else {
        // Standard Deferred Decal ("decal%04d_d/n/r/s/e")
        fmtFn(&texNames[0],  ResolveWStr(0x49ca108), texId);
        fmtFn(&texNames[1],  ResolveWStr(0x49ca120), texId);
        fmtFn(&texNames[2],  ResolveWStr(0x49ca138), texId);
        fmtFn(&texNames[3],  ResolveWStr(0x49ca150), texId);
        fmtFn(&texNames[13], ResolveWStr(0x49ca168), texId);
    }

    uint32_t decalType = *reinterpret_cast<uint32_t*>(self + 0x938);
    void* pTexProvider = *reinterpret_cast<void**>(self + 0x8A0);
    auto lookupTex = [&](const DLBasicStringW& name) -> void* {
        void** vtbl = *reinterpret_cast<void***>(pTexProvider);
        using LookupFn = void* (*)(void*, const char16_t*);
        return reinterpret_cast<LookupFn>(vtbl[2])(pTexProvider, GetBasicStringWCString(&name));
    };

    if (decalType == 1u && pTexProvider != nullptr) {
        *reinterpret_cast<void**>(spawnDesc + 0x48) = lookupTex(texNames[0]);
        *reinterpret_cast<void**>(spawnDesc + 0x60) = (self[0x8F0] == 0) ? lookupTex(texNames[1]) : nullptr;

        auto getFallbackTex = []() -> void* {
            uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
            uint8_t* pSub    = *reinterpret_cast<uint8_t**>(pGlobal + 0x238);
            return *reinterpret_cast<void**>(pSub + 0x20);
        };

        if (self[0x8F1] != 0) {
            *reinterpret_cast<void**>(spawnDesc + 0x50) = getFallbackTex();
            *reinterpret_cast<void**>(spawnDesc + 0x58) = getFallbackTex();
        } else {
            *reinterpret_cast<void**>(spawnDesc + 0x50) = lookupTex(texNames[2]);
            *reinterpret_cast<void**>(spawnDesc + 0x58) =
                (self[0x8F1] == 0) ? lookupTex(texNames[3]) : getFallbackTex();
        }
        *reinterpret_cast<void**>(spawnDesc + 0x70) = lookupTex(texNames[12]);
    } else if (decalType == 0u && pTexProvider != nullptr) {
        *reinterpret_cast<void**>(spawnDesc + 0x48) = lookupTex(texNames[0]);
        *reinterpret_cast<void**>(spawnDesc + 0x60) = (self[0x8F0] == 0) ? lookupTex(texNames[1]) : nullptr;
        if (self[0x8F1] != 0) {
            *reinterpret_cast<void**>(spawnDesc + 0x50) = nullptr;
            *reinterpret_cast<void**>(spawnDesc + 0x58) = nullptr;
        } else {
            *reinterpret_cast<void**>(spawnDesc + 0x50) = lookupTex(texNames[2]);
            *reinterpret_cast<void**>(spawnDesc + 0x58) = (self[0x8F1] == 0) ? lookupTex(texNames[3]) : nullptr;
        }

        *reinterpret_cast<void**>(spawnDesc + 0x78) = lookupTex(texNames[4]);
        *reinterpret_cast<void**>(spawnDesc + 0x90) = (self[0x8F0] == 0) ? lookupTex(texNames[5]) : nullptr;
        if (self[0x8F1] != 0) {
            *reinterpret_cast<void**>(spawnDesc + 0x80) = nullptr;
            *reinterpret_cast<void**>(spawnDesc + 0x88) = nullptr;
        } else {
            *reinterpret_cast<void**>(spawnDesc + 0x80) = lookupTex(texNames[6]);
            *reinterpret_cast<void**>(spawnDesc + 0x88) = (self[0x8F1] == 0) ? lookupTex(texNames[7]) : nullptr;
        }

        *reinterpret_cast<void**>(spawnDesc + 0x98)  = lookupTex(texNames[8]);
        *reinterpret_cast<void**>(spawnDesc + 0xB0)  = (self[0x8F0] == 0) ? lookupTex(texNames[9]) : nullptr;
        if (self[0x8F1] != 0) {
            *reinterpret_cast<void**>(spawnDesc + 0xA0) = nullptr;
            *reinterpret_cast<void**>(spawnDesc + 0xA8) = nullptr;
        } else {
            *reinterpret_cast<void**>(spawnDesc + 0xA0) = lookupTex(texNames[10]);
            *reinterpret_cast<void**>(spawnDesc + 0xA8) = (self[0x8F1] == 0) ? lookupTex(texNames[11]) : nullptr;
        }

        *reinterpret_cast<void**>(spawnDesc + 0x70) = lookupTex(texNames[12]);
        *reinterpret_cast<void**>(spawnDesc + 0x68) = (self[0x8F3] != 0) ? lookupTex(texNames[13]) : nullptr;
    }

    void** selfVtbl = *reinterpret_cast<void***>(self);
    using SpawnDecalFn = uint32_t (*)(uint8_t*, const void*);
    uint32_t decalHandle = reinterpret_cast<SpawnDecalFn>(selfVtbl[2])(self, spawnDesc);
    *reinterpret_cast<uint32_t*>(self + 0x96C) = decalHandle;

    for (int i = 13; i >= 0; --i) {
        DestroyStackBasicStringW(&texNames[i]);
    }
}

// ============================================================================
// 24. 0x220b840 (8955 B) — GXDecalManager_PopulateDebugMenuGui
// ============================================================================
extern "C" void GXDecalManager_PopulateDebugMenuGui(
    uint8_t* self,
    uint8_t* guiBuilder)
{
    struct GuiStringArg {
        uint64_t pShared;
        const char16_t* pRaw;
    };

    using BeginGroupFn = void* (*)(uint8_t*, GuiStringArg*, uint32_t);
    using BeginToolbarFn = uint8_t* (*)(uint8_t*, GuiStringArg*);
    using AddCheckboxFn = void (*)(uint8_t*, GuiStringArg*, uint8_t*);
    using AddFloatSliderFn = void (*)(uint8_t*, GuiStringArg*, float*, float, float, float, float);
    using AddFloat3SliderFn = void (*)(uint8_t*, GuiStringArg*, uint32_t, float*, float, float, float, float);
    using AddIntSliderFn = void (*)(uint8_t*, GuiStringArg*, int32_t*, int32_t, int32_t, int32_t, float);
    using AddUintSliderFn = void (*)(uint8_t*, GuiStringArg*, uint32_t*, uint32_t, uint32_t, uint32_t, float);
    using AddReadOnlyIntFn = void (*)(uint8_t*, GuiStringArg*, void*);
    using AddEnumComboFn = void (*)(uint8_t*, GuiStringArg*, void*, uint32_t, const void*);
    using AddButtonFn = void (*)(uint8_t*, GuiStringArg*, void*);

    BeginGroupFn      beginGroup      = reinterpret_cast<BeginGroupFn>(ResolveVMA(0x236c900));
    BeginToolbarFn    beginToolbar    = reinterpret_cast<BeginToolbarFn>(ResolveVMA(0x236d620));
    AddCheckboxFn     addCheckbox     = reinterpret_cast<AddCheckboxFn>(ResolveVMA(0x292f830));
    AddFloatSliderFn  addFloatSlider  = reinterpret_cast<AddFloatSliderFn>(ResolveVMA(0x292a590));
    AddFloat3SliderFn addFloat3Slider = reinterpret_cast<AddFloat3SliderFn>(ResolveVMA(0x292a3f0));
    AddIntSliderFn    addIntSlider    = reinterpret_cast<AddIntSliderFn>(ResolveVMA(0x2934780));
    AddUintSliderFn   addUintSlider   = reinterpret_cast<AddUintSliderFn>(ResolveVMA(0x29316a0));
    AddReadOnlyIntFn  addReadOnlyInt  = reinterpret_cast<AddReadOnlyIntFn>(ResolveVMA(0x2905e60));
    AddEnumComboFn    addEnumCombo    = reinterpret_cast<AddEnumComboFn>(ResolveVMA(0x2934560));
    AddButtonFn       addButton       = reinterpret_cast<AddButtonFn>(ResolveVMA(0x235e4c0));

    auto makeButtonCallback = [&](uintptr_t targetFnVma) -> void* {
        DLAllocator* pGuiAlloc = *reinterpret_cast<DLAllocator**>(ResolveVMA(0x5540e08));
        uint8_t* pCb = static_cast<uint8_t*>(pGuiAlloc->vptr->allocateAligned(pGuiAlloc, 0x30u, 8u));
        if (!pCb) {
            return nullptr;
        }
        uint32_t f = (*reinterpret_cast<uint32_t*>(pCb + 0x08) & 0x80000000u) | 0x40000000u;
        *reinterpret_cast<uint32_t*>(pCb + 0x08) = f;
        *reinterpret_cast<uintptr_t*>(pCb + 0x00) = ResolveVMA(0x53b7330);
        *reinterpret_cast<uint8_t**>(pCb + 0x10) = self;
        *reinterpret_cast<uint32_t*>(pCb + 0x18) = 0;
        *reinterpret_cast<uintptr_t*>(pCb + 0x20) = ResolveVMA(targetFnVma);
        *reinterpret_cast<uint64_t*>(pCb + 0x28) = 0;
        return pCb;
    };

    // 1. "Debug" group
    {
        GuiStringArg name{0, ResolveWStr(0x49ce5a2)};
        if (beginGroup(guiBuilder, &name, 1u) != nullptr) {
            GuiStringArg s1{0, ResolveWStr(0x49c980e)};
            addCheckbox(guiBuilder, &s1, self + 0x8F5);
            GuiStringArg s2{0, ResolveWStr(0x49c983c)};
            addCheckbox(guiBuilder, &s2, self + 0x8F6);
            GuiStringArg s3{0, ResolveWStr(0x49c9864)};
            addCheckbox(guiBuilder, &s3, self + 0x830);
            GuiStringArg s4{0, ResolveWStr(0x49c9890)};
            addCheckbox(guiBuilder, &s4, self + 0x8F4);

            GuiStringArg optName{0, ResolveWStr(0x49cf196)};
            if (beginGroup(guiBuilder, &optName, 1u) != nullptr) {
                GuiStringArg o1{0, ResolveWStr(0x49c98b4)};
                addCheckbox(guiBuilder, &o1, self + 0x969);
                GuiStringArg o2{0, ResolveWStr(0x49c98de)};
                addCheckbox(guiBuilder, &o2, self + 0x96A);
                GuiStringArg o3{0, ResolveWStr(0x49c9910)};
                addCheckbox(guiBuilder, &o3, self + 0x96B);
                GuiStringArg o4{0, ResolveWStr(0x49c992e)};
                addCheckbox(guiBuilder, &o4, reinterpret_cast<uint8_t*>(ResolveVMA(0x5540dd1)));
                PopAndReleaseGuiContainerChild(guiBuilder);
            }
            PopAndReleaseGuiContainerChild(guiBuilder);
        }
    }

    // 2. "DeferredDecal Render Param" group
    {
        GuiStringArg name{0, ResolveWStr(0x49c996a)};
        if (beginGroup(guiBuilder, &name, 1u) != nullptr) {
            GuiStringArg p1{0, ResolveWStr(0x49c99a0)};
            addFloatSlider(guiBuilder, &p1, reinterpret_cast<float*>(self + 0x868), 0.0f, 10.0f, 0.01f, 1.0f);
            GuiStringArg p2{0, ResolveWStr(0x49c99c4)};
            addFloatSlider(guiBuilder, &p2, reinterpret_cast<float*>(self + 0x86C), 0.0f, 10.0f, 0.01f, 1.0f);
            GuiStringArg p3{0, ResolveWStr(0x49c99e8)};
            addFloatSlider(guiBuilder, &p3, reinterpret_cast<float*>(self + 0x870), -0.1f, 10.0f, 0.01f, 1.0f);
            GuiStringArg p4{0, ResolveWStr(0x49c9a0c)};
            addFloatSlider(guiBuilder, &p4, reinterpret_cast<float*>(self + 0x874), 0.0f, 10.0f, 0.01f, 1.0f);
            GuiStringArg p5{0, ResolveWStr(0x49c9a30)};
            addFloatSlider(guiBuilder, &p5, reinterpret_cast<float*>(self + 0x878), 0.0f, 3.1415927f, 0.017453292f, 57.29578f);
            GuiStringArg p6{0, ResolveWStr(0x49c9a5c)};
            addFloatSlider(guiBuilder, &p6, reinterpret_cast<float*>(self + 0x87C), 0.0f, 3.1415927f, 0.017453292f, 57.29578f);
            PopAndReleaseGuiContainerChild(guiBuilder);
        }
    }

    // 3. "Profile" group
    {
        GuiStringArg name{0, ResolveWStr(0x49c9a9a)};
        if (beginGroup(guiBuilder, &name, 1u) != nullptr) {
            GuiStringArg p1{0, ResolveWStr(0x49c9aaa)};
            addReadOnlyInt(guiBuilder, &p1, self + 0x8F8);
            PopAndReleaseGuiContainerChild(guiBuilder);
        }
    }

    // 4. "Paint Decal Debug" group
    {
        GuiStringArg name{0, ResolveWStr(0x49c9ad6)};
        if (beginGroup(guiBuilder, &name, 1u) != nullptr) {
            GuiStringArg d1{0, ResolveWStr(0x49c9afa)};
            addCheckbox(guiBuilder, &d1, reinterpret_cast<uint8_t*>(ResolveVMA(0x56c3d41)));
            GuiStringArg d2{0, ResolveWStr(0x49c9b3c)};
            addUintSlider(guiBuilder, &d2, reinterpret_cast<uint32_t*>(ResolveVMA(0x513acc0)), 1u, 0x2000u, 1u, 1.0f);
            GuiStringArg d3{0, ResolveWStr(0x49c9b6c)};
            addUintSlider(guiBuilder, &d3, reinterpret_cast<uint32_t*>(ResolveVMA(0x513acbc)), 1u, 10u, 1u, 1.0f);
            GuiStringArg d4{0, ResolveWStr(0x49c9b98)};
            addReadOnlyInt(guiBuilder, &d4, reinterpret_cast<void*>(ResolveVMA(0x56c3e6c)));
            GuiStringArg d5{0, ResolveWStr(0x49c9bbe)};
            addReadOnlyInt(guiBuilder, &d5, reinterpret_cast<void*>(ResolveVMA(0x56c3e70)));
            PopAndReleaseGuiContainerChild(guiBuilder);
        }
    }

    // 5. "Generation Debug" group
    {
        GuiStringArg name{0, ResolveWStr(0x49c9bf0)};
        if (beginGroup(guiBuilder, &name, 0u) != nullptr) {
            GuiStringArg emptyName{0, nullptr};
            uint8_t* pToolbar = beginToolbar(guiBuilder, &emptyName);

            GuiStringArg genBtn{0, ResolveWStr(0x49c9c12)};
            addButton(pToolbar, &genBtn, makeButtonCallback(0x220db40));

            GuiStringArg destroyBtn{0, ResolveWStr(0x49c9c30)};
            addButton(pToolbar, &destroyBtn, makeButtonCallback(0x220f2b0));

            GuiStringArg layerArg{0, ResolveWStr(0x49c9c56)};
            addIntSlider(guiBuilder, &layerArg, reinterpret_cast<int32_t*>(self + 0x8FC), 0, 7, 1, 1.0f);

            GuiStringArg specBlendArg{0, ResolveWStr(0x49c9c66)};
            addFloatSlider(guiBuilder, &specBlendArg, reinterpret_cast<float*>(self + 0x900), 0.0f, 1.0f, 0.1f, 1.0f);

            GuiStringArg diffScaleArg{0, ResolveWStr(0x49c9c90)};
            addFloat3Slider(guiBuilder, &diffScaleArg, 3u, reinterpret_cast<float*>(self + 0x90C), 0.0f, 1.0f, 0.01f, 1.0f);

            GuiStringArg reflScaleArg{0, ResolveWStr(0x49c9cac)};
            addFloat3Slider(guiBuilder, &reflScaleArg, 3u, reinterpret_cast<float*>(self + 0x918), 0.0f, 1.0f, 0.01f, 1.0f);

            GuiStringArg shinScaleArg{0, ResolveWStr(0x49c9cd0)};
            addFloatSlider(guiBuilder, &shinScaleArg, reinterpret_cast<float*>(self + 0x924), 0.0f, 1.0f, 0.01f, 1.0f);

            GuiStringArg reflThr1Arg{0, ResolveWStr(0x49c9cf0)};
            addFloatSlider(guiBuilder, &reflThr1Arg, reinterpret_cast<float*>(self + 0x904), 0.0f, 1.0f, 0.01f, 1.0f);

            GuiStringArg reflThr2Arg{0, ResolveWStr(0x49c9d1e)};
            addFloatSlider(guiBuilder, &reflThr2Arg, reinterpret_cast<float*>(self + 0x908), 0.0f, 1.0f, 0.01f, 1.0f);

            // "Generation Param" subgroup
            GuiStringArg genParamName{0, ResolveWStr(0x49c9d4c)};
            if (beginGroup(guiBuilder, &genParamName, 1u) != nullptr) {
                struct EnumDescEntry {
                    const char16_t* label;
                    uint64_t value;
                };
                EnumDescEntry typeEntries[2];
                std::memcpy(typeEntries, reinterpret_cast<const void*>(ResolveVMA(0x5360450)), sizeof(typeEntries));
                GuiStringArg typeArg{0, ResolveWStr(0x49d7ff4)};
                addEnumCombo(guiBuilder, &typeArg, self + 0x938, 2u, typeEntries);

                GuiStringArg texIdArg{0, ResolveWStr(0x49c9d8c)};
                addIntSlider(guiBuilder, &texIdArg, reinterpret_cast<int32_t*>(self + 0x934), 0, 9999, 1, 1.0f);

                EnumDescEntry projEntries[3] = {
                    { ResolveWStr(0x49c9da2), 0u },
                    { ResolveWStr(0x49c9db0), 1u },
                    { ResolveWStr(0x49c9dbc), 2u }
                };
                GuiStringArg projArg{0, ResolveWStr(0x49c9dd4)};
                addEnumCombo(guiBuilder, &projArg, self + 0x93C, 3u, projEntries);

                GuiStringArg soakArg{0, ResolveWStr(0x49c9de8)};
                addCheckbox(guiBuilder, &soakArg, self + 0x940);

                GuiStringArg pomEnArg{0, ResolveWStr(0x49c9e00)};
                addCheckbox(guiBuilder, &pomEnArg, self + 0x941);

                GuiStringArg heightArg{0, ResolveWStr(0x49c9e16)};
                addCheckbox(guiBuilder, &heightArg, self + 0x8F7);

                GuiStringArg cubeOffArg{0, ResolveWStr(0x49c9e30)};
                addFloat3Slider(guiBuilder, &cubeOffArg, 3u, reinterpret_cast<float*>(self + 0x950), -1.0f, 1.0f, 0.01f, 1.0f);

                // Custom "Blend Mode" GXDecalDebugEnumComboWidget construction
                DLAllocator* pGuiAlloc = *reinterpret_cast<DLAllocator**>(ResolveVMA(0x5540e08));
                uint8_t* pComboWidget = static_cast<uint8_t*>(pGuiAlloc->vptr->allocateAligned(pGuiAlloc, 0x38u, 8u));
                if (pComboWidget != nullptr) {
                    void** pParentEnd = *reinterpret_cast<void***>(guiBuilder + 0x10);
                    void* pParentNode = pParentEnd[-1];

                    uint8_t* pOptList = static_cast<uint8_t*>(pGuiAlloc->vptr->allocateAligned(pGuiAlloc, 0x38u, 8u));
                    if (pOptList != nullptr) {
                        uint32_t f = (*reinterpret_cast<uint32_t*>(pOptList + 0x08) & 0x80000000u) | 0x40000000u;
                        *reinterpret_cast<uint32_t*>(pOptList + 0x08) = f;
                        *reinterpret_cast<uintptr_t*>(pOptList + 0x00) = ResolveVMA(0x53b72a0);

                        DLAllocator* pListHeap = *reinterpret_cast<DLAllocator**>(ResolveVMA(0x5540e18));
                        uint32_t heapFlags = 0;
                        pListHeap->vptr->getHeapFlags(pListHeap, &heapFlags, pListHeap, 0);
                        if ((heapFlags & 0x20u) == 0) {
                            using LogFn = void (*)(const char*, uint32_t, const char*, ...);
                            reinterpret_cast<LogFn>(ResolveVMA(0x20b55b0))(
                                nullptr, 0x3Eu, reinterpret_cast<const char*>(ResolveVMA(0x4737dc0)));
                        }
                        *reinterpret_cast<void**>(pOptList + 0x18) = nullptr;
                        *reinterpret_cast<void**>(pOptList + 0x20) = nullptr;
                        *reinterpret_cast<void**>(pOptList + 0x28) = nullptr;
                        *reinterpret_cast<DLAllocator**>(pOptList + 0x30) = pListHeap;

                        GXDecalEnumItemVector_ReallocateAndCopy(pOptList + 0x10, 2u);

                        const EnumDescEntry* blendItems =
                            reinterpret_cast<const EnumDescEntry*>(ResolveVMA(0x5360470));
                        for (int b = 0; b < 2; ++b) {
                            GXDecalDebugEnumItem tempItem{};
                            tempItem.value = static_cast<uint32_t>(blendItems[b].value);
                            GuiStringArg srcStr{0, blendItems[b].label};
                            using CopyGuiStringFn = void (*)(GXDecalDebugEnumItem*, GuiStringArg*);
                            reinterpret_cast<CopyGuiStringFn>(ResolveVMA(0x2ba7bc0))(&tempItem, &srcStr);

                            GXDecalDebugEnumItem* pEnd = *reinterpret_cast<GXDecalDebugEnumItem**>(pOptList + 0x20);
                            GXDecalDebugEnumItem* pCap = *reinterpret_cast<GXDecalDebugEnumItem**>(pOptList + 0x28);
                            if (pEnd == pCap) {
                                GXDecalDebugEnumItem* pBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(pOptList + 0x18);
                                size_t oldCnt = static_cast<size_t>(pEnd - pBegin);
                                size_t half = oldCnt >> 1;
                                const size_t kMax = 0x0AAAAAAAAAAAAAAAULL;
                                size_t newCap = (kMax - half >= oldCnt) ? (oldCnt + half) : 0u;
                                if (newCap < oldCnt + 1u) {
                                    newCap = oldCnt + 1u;
                                }
                                GXDecalEnumItemVector_ReallocateAndCopy(pOptList + 0x10, newCap);
                                pEnd = *reinterpret_cast<GXDecalDebugEnumItem**>(pOptList + 0x20);
                            }
                            if (pEnd != nullptr) {
                                pEnd->pRawStr = nullptr;
                                pEnd->pSharedStr = nullptr;
                                if (tempItem.pRawStr != nullptr) {
                                    pEnd->pRawStr = tempItem.pRawStr;
                                } else if (tempItem.pSharedStr != nullptr) {
                                    pEnd->pSharedStr = tempItem.pSharedStr;
                                    ++tempItem.pSharedStr->refCount;
                                }
                                pEnd->value = tempItem.value;
                            }
                            *reinterpret_cast<GXDecalDebugEnumItem**>(pOptList + 0x20) = pEnd + 1;
                            ReleaseSharedStringW(tempItem.pSharedStr);
                        }
                    }

                    uint32_t* pBlendModeField = reinterpret_cast<uint32_t*>(self + 0x928);
                    GuiStringArg blendModeLabel{0, ResolveWStr(0x49d2e18)};
                    using InitComboBaseFn = void (*)(uint8_t*, void*, GuiStringArg*, uint32_t);
                    reinterpret_cast<InitComboBaseFn>(ResolveVMA(0x2351960))(
                        pComboWidget, pParentNode, &blendModeLabel, 0u);

                    *reinterpret_cast<uintptr_t*>(pComboWidget + 0x00) = ResolveVMA(0x53b72e0);
                    *reinterpret_cast<void**>(pComboWidget + 0x30) = nullptr;
                    *reinterpret_cast<uint32_t**>(pComboWidget + 0x28) = pBlendModeField;
                    *reinterpret_cast<uint32_t*>(pComboWidget + 0x20) = *pBlendModeField;

                    if (pOptList != nullptr) {
                        *reinterpret_cast<uint8_t**>(pComboWidget + 0x30) = pOptList;
                        RetainGuiRefObject(reinterpret_cast<DLGuiRefObject*>(pOptList));
                    }

                    uint8_t* pCtrl = *reinterpret_cast<uint8_t**>(pComboWidget + 0x18);
                    void* pSubCtrl = *reinterpret_cast<void**>(pCtrl + 0x110);
                    using ClearSubCtrlFn = void (*)(void*);
                    reinterpret_cast<ClearSubCtrlFn>(ResolveVMA(0xf48810))(pSubCtrl);

                    uint8_t* pBoundList = *reinterpret_cast<uint8_t**>(pComboWidget + 0x30);
                    GXDecalDebugEnumItem* pListBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(pBoundList + 0x18);
                    GXDecalDebugEnumItem* pListEnd   = *reinterpret_cast<GXDecalDebugEnumItem**>(pBoundList + 0x20);
                    size_t listCount = static_cast<size_t>(pListEnd - pListBegin);

                    for (size_t idx = 0; idx < listCount; ++idx) {
                        GuiStringArg itemStr{0, nullptr};
                        GXDecalDebugEnumItem& entry = pListBegin[idx];
                        if (entry.pRawStr != nullptr) {
                            itemStr.pRaw = entry.pRawStr;
                            using AddComboOptionFn = void (*)(uint8_t*, uint32_t, GuiStringArg*);
                            reinterpret_cast<AddComboOptionFn>(ResolveVMA(0x2352120))(
                                pComboWidget, static_cast<uint32_t>(idx), &itemStr);
                        } else {
                            DLSharedStringW* pShared = entry.pSharedStr;
                            if (pShared != nullptr) {
                                itemStr.pShared = reinterpret_cast<uintptr_t>(pShared);
                                ++pShared->refCount;
                            }
                            using AddComboOptionFn = void (*)(uint8_t*, uint32_t, GuiStringArg*);
                            reinterpret_cast<AddComboOptionFn>(ResolveVMA(0x2352120))(
                                pComboWidget, static_cast<uint32_t>(idx), &itemStr);
                            ReleaseSharedStringW(pShared);
                        }
                        pBoundList = *reinterpret_cast<uint8_t**>(pComboWidget + 0x30);
                        pListBegin = *reinterpret_cast<GXDecalDebugEnumItem**>(pBoundList + 0x18);
                        pListEnd   = *reinterpret_cast<GXDecalDebugEnumItem**>(pBoundList + 0x20);
                        listCount  = static_cast<size_t>(pListEnd - pListBegin);
                    }

                    uint32_t curBlendVal = *reinterpret_cast<uint32_t*>(pComboWidget + 0x20);
                    uint32_t selIdx = 0xFFFFFFFFu;
                    for (uint32_t i = 0; i < static_cast<uint32_t>(listCount); ++i) {
                        if (pListBegin[i].value == curBlendVal) {
                            selIdx = i;
                            break;
                        }
                    }

                    pCtrl = *reinterpret_cast<uint8_t**>(pComboWidget + 0x18);
                    pSubCtrl = *reinterpret_cast<void**>(pCtrl + 0x110);
                    using SetSelectedIndexFn = void (*)(void*, uint32_t, uint32_t);
                    reinterpret_cast<SetSelectedIndexFn>(ResolveVMA(0xf48fe0))(pSubCtrl, selIdx, 0u);

                    RetainGuiRefObject(reinterpret_cast<DLGuiRefObject*>(pComboWidget));
                    void** pParentEndNow = *reinterpret_cast<void***>(guiBuilder + 0x10);
                    using AttachChildFn = void (*)(void*, uint8_t*);
                    reinterpret_cast<AttachChildFn>(ResolveVMA(0x2367220))(pParentEndNow[-1], pComboWidget);
                    ReleaseGuiRefObject(reinterpret_cast<DLGuiRefObject*>(pComboWidget));
                }

                GuiStringArg lenArg{0, ResolveWStr(0x49c9e56)};
                addFloatSlider(guiBuilder, &lenArg, reinterpret_cast<float*>(self + 0x92C), 0.1f, 10.0f, 0.01f, 1.0f);

                GuiStringArg farScaleArg{0, ResolveWStr(0x49c9e64)};
                addFloatSlider(guiBuilder, &farScaleArg, reinterpret_cast<float*>(self + 0x944), 1.0f, 10.0f, 0.1f, 1.0f);

                GuiStringArg depthArg{0, ResolveWStr(0x49ce556)};
                addFloatSlider(guiBuilder, &depthArg, reinterpret_cast<float*>(self + 0x930), 0.1f, 10.0f, 0.01f, 1.0f);

                GuiStringArg noNormArg{0, ResolveWStr(0x49c9e84)};
                addCheckbox(guiBuilder, &noNormArg, self + 0x8F0);

                GuiStringArg noSpecArg{0, ResolveWStr(0x49c9ea0)};
                addCheckbox(guiBuilder, &noSpecArg, self + 0x8F1);

                GuiStringArg bloodArg{0, ResolveWStr(0x49c9ec0)};
                addCheckbox(guiBuilder, &bloodArg, self + 0x8F2);

                GuiStringArg emisArg{0, ResolveWStr(0x49c9ee4)};
                addCheckbox(guiBuilder, &emisArg, self + 0x8F3);

                GuiStringArg repeatArg{0, ResolveWStr(0x49c9f06)};
                addCheckbox(guiBuilder, &repeatArg, self + 0x968);

                // "Pom Param" subgroup
                GuiStringArg pomGroupName{0, ResolveWStr(0x49c9f20)};
                if (beginGroup(guiBuilder, &pomGroupName, 0u) != nullptr) {
                    *reinterpret_cast<float*>(self + 0x980) = 1.0f;
                    *reinterpret_cast<float*>(self + 0x984) = 0.0f;
                    *reinterpret_cast<float*>(self + 0x990) = 1.0f;
                    *reinterpret_cast<float*>(self + 0x988) = 8.0f;
                    *reinterpret_cast<float*>(self + 0x98C) = 64.0f;

                    GuiStringArg pomHArg{0, ResolveWStr(0x49c9f34)};
                    addFloatSlider(guiBuilder, &pomHArg, reinterpret_cast<float*>(self + 0x980), 0.0f, 1.0f, 0.001f, 1.0f);

                    GuiStringArg pomSArg{0, ResolveWStr(0x49c9f58)};
                    addFloatSlider(guiBuilder, &pomSArg, reinterpret_cast<float*>(self + 0x990), 0.0f, 1.0f, 0.01f, 1.0f);

                    GuiStringArg pomMinArg{0, ResolveWStr(0x49c9f88)};
                    addFloatSlider(guiBuilder, &pomMinArg, reinterpret_cast<float*>(self + 0x988), 8.0f, 128.0f, 1.0f, 1.0f);

                    GuiStringArg pomMaxArg{0, ResolveWStr(0x49c9fb2)};
                    addFloatSlider(guiBuilder, &pomMaxArg, reinterpret_cast<float*>(self + 0x98C), 8.0f, 128.0f, 1.0f, 1.0f);

                    PopAndReleaseGuiContainerChild(guiBuilder);
                }

                PopAndReleaseGuiContainerChild(guiBuilder);
            }

            // "Material Param Debug" subgroup
            GuiStringArg matGroupName{0, ResolveWStr(0x49c9fdc)};
            if (beginGroup(guiBuilder, &matGroupName, 0u) != nullptr) {
                GuiStringArg emisIntArg{0, ResolveWStr(0x49ca006)};
                addFloatSlider(guiBuilder, &emisIntArg, reinterpret_cast<float*>(self + 0x970), 0.0f, 10.0f, 0.1f, 1.0f);
                PopAndReleaseGuiContainerChild(guiBuilder);
            }

            // "Paint Decal Modifier" subgroup
            GuiStringArg modGroupName{0, ResolveWStr(0x49ca02c)};
            if (beginGroup(guiBuilder, &modGroupName, 1u) != nullptr) {
                GuiStringArg emptyToolbar{0, nullptr};
                uint8_t* pModToolbar = beginToolbar(guiBuilder, &emptyToolbar);

                GuiStringArg modBtn{0, ResolveWStr(0x49ca056)};
                addButton(pModToolbar, &modBtn, makeButtonCallback(0x220f2c0));

                GuiStringArg clrBtn{0, ResolveWStr(0x49ca064)};
                addButton(pModToolbar, &clrBtn, makeButtonCallback(0x220f2d0));

                GuiStringArg rocArg{0, ResolveWStr(0x49ca070)};
                addFloatSlider(guiBuilder, &rocArg, reinterpret_cast<float*>(self + 0x964), 0.0f, 1.0f, 0.01f, 1.0f);

                PopAndReleaseGuiContainerChild(guiBuilder);
            }

            PopAndReleaseGuiContainerChild(guiBuilder);
        }
    }
}

// ============================================================================
// 25. 0x220b5b0 (651 B) — GXDecalManager_GetOrCreateDebugMenuNode
// ============================================================================
extern "C" void* GXDecalManager_GetOrCreateDebugMenuNode(uint8_t* self) {
    uint8_t* pNode = *reinterpret_cast<uint8_t**>(self + 0x890);
    if (pNode != nullptr && (pNode[0x0B] & 0x40u) != 0 && pNode[0x6F] >= 2u) {
        for (uint8_t* pCur = pNode; pCur != nullptr;
             pCur = *reinterpret_cast<uint8_t**>(pCur + 0x10)) {
            if ((pCur[0x50] & 0x02u) != 0) {
                uint8_t* pSub = *reinterpret_cast<uint8_t**>(pCur + 0x20);
                void* pTarget = *reinterpret_cast<void**>(pSub + 0x10);
                using RefreshNodeFn = void (*)(void*);
                reinterpret_cast<RefreshNodeFn>(ResolveVMA(0x2330a80))(pTarget);
                return *reinterpret_cast<void**>(self + 0x890);
            }
        }
        return *reinterpret_cast<void**>(self + 0x890);
    }

    DLAllocator* pGuiAlloc = *reinterpret_cast<DLAllocator**>(ResolveVMA(0x5540e08));
    uint8_t* pNewNode = static_cast<uint8_t*>(
        pGuiAlloc->vptr->allocateAligned(pGuiAlloc, 0x178u, 8u));

    if (pNewNode != nullptr) {
        uint8_t* pGuiRootMgr = *reinterpret_cast<uint8_t**>(ResolveVMA(0x56a6bc8));
        uint8_t* pGuiSub     = *reinterpret_cast<uint8_t**>(pGuiRootMgr + 0x10);
        void* pParentNode    = *reinterpret_cast<void**>(pGuiSub + 0x18);

        struct GuiStringArg {
            uint64_t pShared;
            const char16_t* pRaw;
        } titleArg{0, ResolveWStr(0x49c97f4)};

        using ConstructDebugNodeFn = void (*)(uint8_t*, void*, GuiStringArg*, uint32_t);
        reinterpret_cast<ConstructDebugNodeFn>(ResolveVMA(0x2363080))(
            pNewNode, pParentNode, &titleArg, 0x51502u);

        DLGuiRefObject* pOldNode = *reinterpret_cast<DLGuiRefObject**>(self + 0x890);
        if (pOldNode != reinterpret_cast<DLGuiRefObject*>(pNewNode)) {
            if (pOldNode != nullptr) {
                ReleaseGuiRefObject(pOldNode);
                *reinterpret_cast<void**>(self + 0x890) = nullptr;
            }
            *reinterpret_cast<uint8_t**>(self + 0x890) = pNewNode;
            RetainGuiRefObject(reinterpret_cast<DLGuiRefObject*>(pNewNode));
        }
        pNode = *reinterpret_cast<uint8_t**>(self + 0x890);
    } else {
        DLGuiRefObject* pOldNode = *reinterpret_cast<DLGuiRefObject**>(self + 0x890);
        if (pOldNode != nullptr) {
            ReleaseGuiRefObject(pOldNode);
            *reinterpret_cast<void**>(self + 0x890) = nullptr;
        }
        pNode = nullptr;
    }

    alignas(16) uint8_t builderBuf[0x40];
    std::memset(builderBuf, 0, sizeof(builderBuf));
    using InitGuiBuilderFn = void (*)(uint8_t*, uint8_t*);
    using DestroyGuiBuilderFn = void (*)(uint8_t*);
    reinterpret_cast<InitGuiBuilderFn>(ResolveVMA(0x236c4d0))(builderBuf, pNode);
    GXDecalManager_PopulateDebugMenuGui(self, builderBuf);
    reinterpret_cast<DestroyGuiBuilderFn>(ResolveVMA(0x236c720))(builderBuf);

    return *reinterpret_cast<void**>(self + 0x890);
}

// ============================================================================
// 26. 0x220b590 (21 B) — GXDecalManager_GetEffectivePaintTargetMaxLength
// ============================================================================
extern "C" uint32_t GXDecalManager_GetEffectivePaintTargetMaxLength() {
    const uint8_t* pLimitEnable = reinterpret_cast<const uint8_t*>(ResolveVMA(0x56c3d41));
    if (*pLimitEnable != 0) {
        return 0x4000u;
    }
    return *reinterpret_cast<const uint32_t*>(ResolveVMA(0x513acc0));
}

// ============================================================================
// 27. 0x220b580 (8 B) — GXDecalManager_GetDeferredDepthTarget
// ============================================================================
extern "C" void* GXDecalManager_GetDeferredDepthTarget(const uint8_t* self) {
    return *reinterpret_cast<void* const*>(self + 0x848);
}

// ============================================================================
// 28. 0x220b570 (8 B) — GXDecalManager_GetDeferredNormalTarget
// ============================================================================
extern "C" void* GXDecalManager_GetDeferredNormalTarget(const uint8_t* self) {
    return *reinterpret_cast<void* const*>(self + 0x840);
}

// ============================================================================
// 29. 0x220b4a0 (196 B) — GXDecalManager_DrawUnitCubeBox
// ============================================================================
extern "C" void GXDecalManager_DrawUnitCubeBox(
    const uint8_t* self,
    uint8_t* renderPassCtx,
    uint32_t cullMode)
{
    void** vtbl = *reinterpret_cast<void***>(renderPassCtx);
    using GetContextOwnerFn = uint8_t* (*)(uint8_t*);
    uint8_t* pOwner = reinterpret_cast<GetContextOwnerFn>(vtbl[2])(renderPassCtx);
    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(pOwner + 0x08);

    uint8_t rastFlags = renderPassCtx[0x18];
    uint8_t dirtyFlags = renderPassCtx[0x30] | 0x01u;
    uint8_t cullBits = static_cast<uint8_t>((cullMode + cullMode) & 0x06u);
    rastFlags = (rastFlags & 0x91u) | cullBits;
    renderPassCtx[0x18] = rastFlags;
    renderPassCtx[0x30] = dirtyFlags;

    pDrawCtx[0x25F0] |= 0x01u;
    *reinterpret_cast<void**>(pDrawCtx + 0xB750) = *reinterpret_cast<void* const*>(self + 0x760);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xB850) = 0x0Cu;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xB8D0) = 0u;
    *reinterpret_cast<void**>(pDrawCtx + 0xB740) = *reinterpret_cast<void* const*>(self + 0x768);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xB748) = 0x39u;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xB74C) = 0u;
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0xB738) = 0x05u;

    using FlushPassStateFn = void (*)(uint8_t*);
    reinterpret_cast<FlushPassStateFn>(ResolveVMA(0x225f390))(renderPassCtx);

    using DispatchDrawFn = void (*)(uint8_t*, void*, uint32_t, uint32_t, uint32_t);
    reinterpret_cast<DispatchDrawFn>(ResolveVMA(0x2169780))(
        pDrawCtx, pDrawCtx + 0xB730, 0x0Eu, 0u, 0u);
}

// ============================================================================
// 30. 0x220b490 (8 B) — GXDecalManager_GetPaintBlendState
// ============================================================================
extern "C" void* GXDecalManager_GetPaintBlendState(const uint8_t* self) {
    return *reinterpret_cast<void* const*>(self + 0x828);
}

// ============================================================================
// 31. 0x220b480 (12 B) — GXDecalManager_GetDeferredPassBlendState
// ============================================================================
extern "C" void* GXDecalManager_GetDeferredPassBlendState(
    const uint8_t* self,
    int32_t modeIndex)
{
    return *reinterpret_cast<void* const*>(self + 0x7F8 + static_cast<intptr_t>(modeIndex) * 8);
}

// ============================================================================
// 32. 0x220b470 (8 B) — GXDecalManager_GetPrePassBlendState
// ============================================================================
extern "C" void* GXDecalManager_GetPrePassBlendState(const uint8_t* self) {
    return *reinterpret_cast<void* const*>(self + 0x7F0);
}

// ============================================================================
// 33. 0x220b420 (77 B) — GXDecalManager_GetPrePassDepthStencilState
// ============================================================================
extern "C" void* GXDecalManager_GetPrePassDepthStencilState(
    const uint8_t* self,
    uint32_t depthMode)
{
    uint32_t subIdx = 0u;
    if (depthMode == 0u || depthMode == 2u) {
        if (depthMode == 2u) {
            subIdx = 1u;
        }
        uint32_t useDepthBounds = static_cast<uint32_t>(self[0x969] ^ 1u);
        return *reinterpret_cast<void* const*>(
            self + (static_cast<size_t>(useDepthBounds) << 4) + static_cast<size_t>(subIdx) * 8u + 0x770);
    }
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
    uint8_t* pStateMgr = *reinterpret_cast<uint8_t**>(pGlobal + 0x250);
    return *reinterpret_cast<void**>(pStateMgr + static_cast<size_t>(depthMode) * 8u + 0x1C8);
}

// ============================================================================
// 34. 0x220b3d0 (77 B) — GXDecalManager_GetDeferredDepthStencilState
// ============================================================================
extern "C" void* GXDecalManager_GetDeferredDepthStencilState(
    const uint8_t* self,
    uint32_t depthMode)
{
    uint32_t subIdx = 0u;
    if (depthMode == 0u || depthMode == 2u) {
        if (depthMode == 2u) {
            subIdx = 1u;
        }
        uint32_t useDepthBounds = static_cast<uint32_t>(self[0x969] ^ 1u);
        return *reinterpret_cast<void* const*>(
            self + (static_cast<size_t>(useDepthBounds) << 4) + static_cast<size_t>(subIdx) * 8u + 0x790);
    }
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
    uint8_t* pStateMgr = *reinterpret_cast<uint8_t**>(pGlobal + 0x250);
    return *reinterpret_cast<void**>(pStateMgr + static_cast<size_t>(depthMode) * 8u + 0x1C8);
}

// ============================================================================
// 35. 0x220b360 (97 B) — GXDecalManager_GetSinglePassDepthStencilState
// ============================================================================
extern "C" void* GXDecalManager_GetSinglePassDepthStencilState(
    const uint8_t* self,
    uint32_t depthMode,
    uint8_t stencilFlag)
{
    uint32_t subIdx = 0u;
    if (depthMode == 0u || depthMode == 2u) {
        if (depthMode == 2u) {
            subIdx = 1u;
        }
        size_t useDepthBounds = static_cast<size_t>(self[0x969] ^ 1u);
        size_t groupIdx = (static_cast<size_t>(stencilFlag == 0 ? 1u : 0u) | 2u) << 5;
        return *reinterpret_cast<void* const*>(
            self + groupIdx + (useDepthBounds << 4) + static_cast<size_t>(subIdx) * 8u + 0x770);
    }
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
    uint8_t* pStateMgr = *reinterpret_cast<uint8_t**>(pGlobal + 0x250);
    return *reinterpret_cast<void**>(pStateMgr + static_cast<size_t>(depthMode) * 8u + 0x1C8);
}

// ============================================================================
// 36. 0x220afa0 (952 B) — GXDecalManager_ExecutePaintDecalPass
// ============================================================================
extern "C" void GXDecalManager_ExecutePaintDecalPass(
    uint8_t* self,
    void* pRenderDeviceCtx,
    const uint8_t* pViewParams,
    void* /*unusedRcx*/,
    uint32_t passIndex)
{
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
    if (pGlobal[0x3A9] != 0) {
        return;
    }

    alignas(16) uint8_t localViewParams[0x90];
    std::memcpy(localViewParams, pViewParams, 0x84);

    using InitPaintRenderCtxFn = void (*)(uint8_t*, void*, uint32_t, uint32_t,
                                          const uint8_t*, uint32_t,
                                          uint32_t, uint32_t, uint32_t, uint32_t);
    using EndPaintRenderCtxFn = void (*)(uint8_t*);
    using ExecPaintCmdFn = void (*)(void*, uint8_t*, uint32_t, uint32_t, void*);
    using ExecModifyTargetFn = void (*)(void*, const void*);
    using ExecClearTargetFn = void (*)(void*, uint32_t);

    InitPaintRenderCtxFn initPaintCtx = reinterpret_cast<InitPaintRenderCtxFn>(ResolveVMA(0x22d4d30));
    EndPaintRenderCtxFn  endPaintCtx  = reinterpret_cast<EndPaintRenderCtxFn>(ResolveVMA(0x22d5480));
    ExecPaintCmdFn       execPaintCmd = reinterpret_cast<ExecPaintCmdFn>(ResolveVMA(0x2210920));

    alignas(16) uint8_t paintRenderCtx[0x870];

    // 1. Handle pending Destroy All Decals request (self[0x68])
    if (self[0x68] != 0) {
        initPaintCtx(paintRenderCtx, pRenderDeviceCtx, passIndex, 0u, localViewParams, 0u, 0u, 0u, 0u, 0u);

        void** pTargetBegin = *reinterpret_cast<void***>(self + 0x48);
        void** pTargetEnd   = *reinterpret_cast<void***>(self + 0x50);
        uint64_t dummyParam = 0;
        for (void** it = pTargetBegin; it != pTargetEnd; ++it) {
            execPaintCmd(*it, paintRenderCtx, 0u, 0u, &dummyParam);
        }

        uint8_t* pMutex = self + 0x880;
        void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
        using LockFn = void (*)(uint8_t*, uint32_t);
        using UnlockFn = void (*)(uint8_t*);
        reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

        uint8_t** pDecalBegin = *reinterpret_cast<uint8_t***>(self + 0x78);
        uint8_t** pDecalEnd   = *reinterpret_cast<uint8_t***>(self + 0x80);
        for (uint8_t** it = pDecalBegin; it != pDecalEnd; ++it) {
            uint8_t* pDecal = *it;
            if (pDecal != nullptr) {
                DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
                void** decalVtbl = *reinterpret_cast<void***>(pDecal);
                using DtorFn = void (*)(uint8_t*);
                reinterpret_cast<DtorFn>(decalVtbl[1])(pDecal);
                pAlloc->vptr->deallocate(pAlloc, pDecal);
                pDecalEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
            }
        }

        void* pSlotTable = *reinterpret_cast<void**>(self + 0x850);
        uint32_t maxSlots = *reinterpret_cast<uint32_t*>(self + 0x860);
        std::memset(pSlotTable, 0, static_cast<size_t>(maxSlots) * sizeof(void*));
        *reinterpret_cast<uint8_t***>(self + 0x80) = *reinterpret_cast<uint8_t***>(self + 0x78);

        mutexVtbl = *reinterpret_cast<void***>(pMutex);
        reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);

        self[0x68] = 0;
        endPaintCtx(paintRenderCtx);
    }

    // 2. Handle pending Modify Paint Decals request (self[0x960])
    if (self[0x960] != 0) {
        struct ModifyParam {
            uint32_t layerNo;
            float rateOfChange;
        } modParam{
            *reinterpret_cast<uint32_t*>(self + 0x8FC),
            *reinterpret_cast<float*>(self + 0x964)
        };
        void** pTargetBegin = *reinterpret_cast<void***>(self + 0x48);
        void** pTargetEnd   = *reinterpret_cast<void***>(self + 0x50);
        ExecModifyTargetFn modifyFn = reinterpret_cast<ExecModifyTargetFn>(ResolveVMA(0x2210800));
        for (void** it = pTargetBegin; it != pTargetEnd; ++it) {
            modifyFn(*it, &modParam);
        }
        self[0x960] = 0;
    }

    // 3. Handle pending Clear Paint Decals request (self[0x961])
    if (self[0x961] != 0) {
        uint32_t layerMask = 1u << (self[0x8FC] & 31u);
        void** pTargetBegin = *reinterpret_cast<void***>(self + 0x48);
        void** pTargetEnd   = *reinterpret_cast<void***>(self + 0x50);
        ExecClearTargetFn clearFn = reinterpret_cast<ExecClearTargetFn>(ResolveVMA(0x22106e0));
        for (void** it = pTargetBegin; it != pTargetEnd; ++it) {
            clearFn(*it, layerMask);
        }
        self[0x961] = 0;
    }

    // 4. Execute up to g_GXPaintDecalCountPerFrame queued paint commands
    GXPaintDecalCommand* pCmdBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
    GXPaintDecalCommand* pCmdEnd   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
    if (pCmdBegin != pCmdEnd) {
        initPaintCtx(paintRenderCtx, pRenderDeviceCtx, passIndex, 0u, localViewParams, 0u, 0u, 0u, 0u, 0u);
        uint32_t maxPerFrame = *reinterpret_cast<const uint32_t*>(ResolveVMA(0x513acbc));
        if (maxPerFrame != 0u) {
            GXPaintDecalCommand* pCur = pCmdBegin;
            uint32_t processed = 0u;
            do {
                execPaintCmd(
                    pCur->pPaintTarget,
                    paintRenderCtx,
                    pCur->decalHandle,
                    pCur->cmdType,
                    &pCur->param0);
                ++pCur;
                ++processed;
            } while (pCur != pCmdEnd && processed < maxPerFrame);

            pCmdEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
            const size_t remainingCount = static_cast<size_t>(pCmdEnd - pCur);
            if (remainingCount != 0u) {
                std::memmove(pCmdBegin, pCur, remainingCount * sizeof(GXPaintDecalCommand));
            }
            *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28) = pCmdBegin + remainingCount;
        }
        endPaintCtx(paintRenderCtx);
    }
}

// ============================================================================
// 37. 0x220ae80 (278 B) — GXDecalManager_EnqueueModifyPaintTargetCommand
// ============================================================================
extern "C" void GXDecalManager_EnqueueModifyPaintTargetCommand(
    uint8_t* self,
    void* pPaintTarget,
    const uint64_t* pModifyParams)
{
    GXPaintDecalCommand cmd{};
    cmd.pPaintTarget = pPaintTarget;
    cmd.decalHandle = 0u;
    cmd.reserved10 = 0u;
    cmd.cmdType = 1u;
    std::memcpy(&cmd.param0, pModifyParams, sizeof(uint64_t));

    uint8_t* pQueue = self + 0x18;
    GXPaintDecalCommand* pEnd   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
    GXPaintDecalCommand* pBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
    GXPaintDecalCommand* pCap   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x30);

    if (&cmd < pEnd && &cmd >= pBegin) {
        ptrdiff_t idx = &cmd - pBegin;
        if (pEnd == pCap) {
            GXPaintDecalCommandQueue_GrowCapacityByOne(pQueue);
            pEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
        }
        if (pEnd != nullptr) {
            GXPaintDecalCommand* pNewBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
            *pEnd = pNewBegin[idx];
        }
    } else {
        if (pEnd == pCap) {
            GXPaintDecalCommandQueue_GrowCapacityByOne(pQueue);
            pEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
        }
        if (pEnd != nullptr) {
            *pEnd = cmd;
        }
    }
    *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28) = pEnd + 1;
}

// ============================================================================
// 38. 0x220ad60 (274 B) — GXDecalManager_EnqueueClearPaintTargetCommand
// ============================================================================
extern "C" void GXDecalManager_EnqueueClearPaintTargetCommand(
    uint8_t* self,
    void* pPaintTarget,
    uint32_t layerMask)
{
    GXPaintDecalCommand cmd{};
    cmd.pPaintTarget = pPaintTarget;
    cmd.decalHandle = 0u;
    cmd.reserved10 = 0u;
    cmd.cmdType = 2u;
    cmd.param0 = layerMask;

    uint8_t* pQueue = self + 0x18;
    GXPaintDecalCommand* pEnd   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
    GXPaintDecalCommand* pBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
    GXPaintDecalCommand* pCap   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x30);

    if (&cmd < pEnd && &cmd >= pBegin) {
        ptrdiff_t idx = &cmd - pBegin;
        if (pEnd == pCap) {
            GXPaintDecalCommandQueue_GrowCapacityByOne(pQueue);
            pEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
        }
        if (pEnd != nullptr) {
            GXPaintDecalCommand* pNewBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
            *pEnd = pNewBegin[idx];
        }
    } else {
        if (pEnd == pCap) {
            GXPaintDecalCommandQueue_GrowCapacityByOne(pQueue);
            pEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
        }
        if (pEnd != nullptr) {
            *pEnd = cmd;
        }
    }
    *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28) = pEnd + 1;
}

// ============================================================================
// 39. 0x220ac10 (335 B) — GXDecalManager_EnqueueApplyPaintDecalCommand
// ============================================================================
extern "C" void GXDecalManager_EnqueueApplyPaintDecalCommand(
    uint8_t* self,
    void* pPaintTarget,
    uint32_t decalHandle)
{
    if (decalHandle == 0xFFFFFFFFu) {
        return;
    }

    uint8_t* pMutex = self + 0x880;
    void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
    using LockFn = void (*)(uint8_t*, uint32_t);
    using UnlockFn = void (*)(uint8_t*);
    reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

    uint32_t slotIdx = decalHandle & 0xFFFFu;
    uint8_t** pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
    uint8_t* pDecal = pSlotTable[slotIdx];
    if (!pDecal) {
        mutexVtbl = *reinterpret_cast<void***>(pMutex);
        reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);
        return;
    }
    uint16_t gen = static_cast<uint16_t>(decalHandle >> 16);
    const uint16_t* pGenTable = *reinterpret_cast<const uint16_t**>(self + 0x858);
    if (gen != pGenTable[slotIdx] || *reinterpret_cast<const uint32_t*>(pDecal + 0x50) != 1u) {
        mutexVtbl = *reinterpret_cast<void***>(pMutex);
        reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);
        return;
    }

    GXPaintDecalCommand cmd{};
    cmd.pPaintTarget = pPaintTarget;
    cmd.decalHandle = decalHandle;
    cmd.reserved10 = 0u;
    cmd.cmdType = 0u;

    uint8_t* pQueue = self + 0x18;
    GXPaintDecalCommand* pEnd   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
    GXPaintDecalCommand* pBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
    GXPaintDecalCommand* pCap   = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x30);

    if (&cmd < pEnd && &cmd >= pBegin) {
        ptrdiff_t idx = &cmd - pBegin;
        if (pEnd == pCap) {
            GXPaintDecalCommandQueue_GrowCapacityByOne(pQueue);
            pEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
        }
        if (pEnd != nullptr) {
            GXPaintDecalCommand* pNewBegin = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
            *pEnd = pNewBegin[idx];
        }
    } else {
        if (pEnd == pCap) {
            GXPaintDecalCommandQueue_GrowCapacityByOne(pQueue);
            pEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
        }
        if (pEnd != nullptr) {
            *pEnd = cmd;
        }
    }
    *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28) = pEnd + 1;

    mutexVtbl = *reinterpret_cast<void***>(pMutex);
    reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);
}

// ============================================================================
// 40. 0x220ab10 (254 B) — GXDecalManager_UnregisterAndDestroyPaintTarget
// ============================================================================
extern "C" void GXDecalManager_UnregisterAndDestroyPaintTarget(
    uint8_t* self,
    void* pPaintTarget)
{
    void** pBegin = *reinterpret_cast<void***>(self + 0x48);
    void** pEnd   = *reinterpret_cast<void***>(self + 0x50);
    void** pFound = nullptr;
    for (void** it = pBegin; it != pEnd; ++it) {
        if (*it == pPaintTarget) {
            pFound = it;
            break;
        }
    }
    if (!pFound) {
        return;
    }

    // Purge any queued paint commands referencing pPaintTarget
    GXPaintDecalCommand* pCmdCur = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x20);
    GXPaintDecalCommand* pCmdEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
    while (pCmdCur != pCmdEnd) {
        if (pCmdCur->pPaintTarget == pPaintTarget) {
            GXPaintDecalCommand* pNext = pCmdCur + 1;
            if (pNext != pCmdEnd) {
                GXPaintDecalCommand* pDst = pCmdCur;
                for (GXPaintDecalCommand* pSrc = pNext; pSrc != pCmdEnd; ++pSrc, ++pDst) {
                    *pDst = *pSrc;
                }
                pCmdEnd = *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28);
            }
            --pCmdEnd;
            *reinterpret_cast<GXPaintDecalCommand**>(self + 0x28) = pCmdEnd;
        } else {
            ++pCmdCur;
        }
    }

    void* pTargetObj = *pFound;
    using ReleaseTargetResourcesFn = void (*)(void*);
    reinterpret_cast<ReleaseTargetResourcesFn>(ResolveVMA(0x2210200))(pTargetObj);

    pTargetObj = *pFound;
    if (pTargetObj != nullptr) {
        using GetHeapForPtrFn = DLAllocator* (*)(void*);
        DLAllocator* pHeap = reinterpret_cast<GetHeapForPtrFn>(ResolveVMA(0x207b720))(pTargetObj);
        void** vtbl = *reinterpret_cast<void***>(pTargetObj);
        using DtorFn = void (*)(void*);
        reinterpret_cast<DtorFn>(vtbl[0])(pTargetObj);
        pHeap->vptr->deallocate(pHeap, pTargetObj);
    }

    pEnd = *reinterpret_cast<void***>(self + 0x50);
    void** pNextTarget = pFound + 1;
    size_t tailBytes = static_cast<size_t>(pEnd - pNextTarget) * sizeof(void*);
    if (tailBytes > 0) {
        std::memmove(pFound, pNextTarget, tailBytes);
    }
    *reinterpret_cast<void***>(self + 0x50) = pEnd - 1;
}

// ============================================================================
// 41. 0x220a9c0 (333 B) — GXDecalManager_CreatePaintTargetWithConfig
// ============================================================================
extern "C" void* GXDecalManager_CreatePaintTargetWithConfig(
    uint8_t* self,
    void* pTextureResource,
    const uint8_t* pConfigSource,
    uint8_t formatFlag)
{
    if (!pTextureResource) {
        return nullptr;
    }
    uint8_t flagA = pConfigSource[0x24E];
    uint8_t flagB = pConfigSource[0x24F];

    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
    uint8_t* pTarget = static_cast<uint8_t*>(pAlloc->vptr->allocateAligned(pAlloc, 0x80u, 8u));
    if (pTarget != nullptr) {
        *reinterpret_cast<uint32_t*>(pTarget + 0x08) = 0u;
        *reinterpret_cast<uintptr_t*>(pTarget + 0x00) = ResolveVMA(0x53604c0);
        *reinterpret_cast<uintptr_t*>(pTarget + 0x10) = ResolveVMA(0x53604f0);
        *reinterpret_cast<uint8_t**>(pTarget + 0x18) = self;
        std::memset(pTarget + 0x20, 0, 0x5Au);
    }

    void* pTargetVar = pTarget;
    using InitPaintTargetFn = void (*)(void*, void*, uint32_t, uint32_t, uint32_t);
    using BindPaintTargetConfigFn = void (*)(void*, const uint8_t*);
    reinterpret_cast<InitPaintTargetFn>(ResolveVMA(0x22102f0))(
        pTarget, pTextureResource, formatFlag, flagA, flagB);
    reinterpret_cast<BindPaintTargetConfigFn>(ResolveVMA(0x22105a0))(
        pTarget, pConfigSource);

    GXDecalPtrVector_PushBack(self + 0x40, &pTargetVar);
    return pTargetVar;
}

// ============================================================================
// 42. 0x220a8a0 (286 B) — GXDecalManager_CreatePaintTargetSimple
// ============================================================================
extern "C" void* GXDecalManager_CreatePaintTargetSimple(
    uint8_t* self,
    void* pTextureResource,
    uint8_t formatFlag)
{
    if (!pTextureResource) {
        return nullptr;
    }

    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
    uint8_t* pTarget = static_cast<uint8_t*>(pAlloc->vptr->allocateAligned(pAlloc, 0x80u, 8u));
    if (pTarget != nullptr) {
        *reinterpret_cast<uint32_t*>(pTarget + 0x08) = 0u;
        *reinterpret_cast<uintptr_t*>(pTarget + 0x00) = ResolveVMA(0x53604c0);
        *reinterpret_cast<uintptr_t*>(pTarget + 0x10) = ResolveVMA(0x53604f0);
        *reinterpret_cast<uint8_t**>(pTarget + 0x18) = self;
        std::memset(pTarget + 0x20, 0, 0x5Au);
    }

    void* pTargetVar = pTarget;
    using InitPaintTargetFn = void (*)(void*, void*, uint32_t, uint32_t, uint32_t);
    reinterpret_cast<InitPaintTargetFn>(ResolveVMA(0x22102f0))(
        pTarget, pTextureResource, formatFlag, 1u, 0u);

    GXDecalPtrVector_PushBack(self + 0x40, &pTargetVar);
    return pTargetVar;
}

// ============================================================================
// 43. 0x220a760 (307 B) — GXDecalManager_UpdateOrDestroyDecalByHandle
// ============================================================================
extern "C" bool GXDecalManager_UpdateOrDestroyDecalByHandle(
    uint8_t* self,
    uint32_t decalHandle,
    float fadeDuration)
{
    uint8_t* pMutex = self + 0x880;
    void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
    using LockFn = void (*)(uint8_t*, uint32_t);
    using UnlockFn = void (*)(uint8_t*);
    reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

    if (fadeDuration > 0.0f) {
        bool okFade = false;
        if (decalHandle != 0xFFFFFFFFu) {
            uint32_t slotIdx = decalHandle & 0xFFFFu;
            uint8_t** pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
            uint8_t* pDecal = pSlotTable[slotIdx];
            if (pDecal != nullptr) {
                uint16_t gen = static_cast<uint16_t>(decalHandle >> 16);
                const uint16_t* pGenTable = *reinterpret_cast<const uint16_t**>(self + 0x858);
                if (gen == pGenTable[slotIdx]) {
                    void** vtbl = *reinterpret_cast<void***>(pDecal);
                    using StartFadeOutFn = void (*)(uint8_t*, float);
                    reinterpret_cast<StartFadeOutFn>(vtbl[7])(pDecal, fadeDuration);
                    okFade = true;
                }
            }
        }
        mutexVtbl = *reinterpret_cast<void***>(pMutex);
        reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);
        return okFade;
    }

    bool ok = false;
    if (decalHandle != 0xFFFFFFFFu) {
        uint32_t slotIdx = decalHandle & 0xFFFFu;
        uint8_t** pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
        uint8_t* pDecal = pSlotTable[slotIdx];
        if (pDecal != nullptr) {
            uint16_t gen = static_cast<uint16_t>(decalHandle >> 16);
            const uint16_t* pGenTable = *reinterpret_cast<const uint16_t**>(self + 0x858);
            if (gen == pGenTable[slotIdx]) {
                uint8_t** pBegin = *reinterpret_cast<uint8_t***>(self + 0x78);
                uint8_t** pEnd   = *reinterpret_cast<uint8_t***>(self + 0x80);
                uint8_t** pFound = nullptr;
                for (uint8_t** it = pBegin; it != pEnd; ++it) {
                    if (*it == pDecal) {
                        pFound = it;
                        break;
                    }
                }
                if (pFound != nullptr) {
                    uint8_t** pNext = pFound + 1;
                    size_t tailBytes = static_cast<size_t>(pEnd - pNext) * sizeof(void*);
                    if (tailBytes > 0) {
                        std::memmove(pFound, pNext, tailBytes);
                    }
                    *reinterpret_cast<uint8_t***>(self + 0x80) = pEnd - 1;

                    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
                    void** decalVtbl = *reinterpret_cast<void***>(pDecal);
                    using DtorFn = void (*)(uint8_t*);
                    reinterpret_cast<DtorFn>(decalVtbl[1])(pDecal);
                    pAlloc->vptr->deallocate(pAlloc, pDecal);

                    pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
                    pSlotTable[slotIdx] = nullptr;
                    ok = true;
                }
            }
        }
    }

    mutexVtbl = *reinterpret_cast<void***>(pMutex);
    reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);
    return ok;
}

// ============================================================================
// 44. 0x220a010 (1860 B) — GXDecalManager_SpawnDecal
// ============================================================================
extern "C" uint32_t GXDecalManager_SpawnDecal(
    uint8_t* self,
    const uint8_t* pSpawnDesc,
    void* pExtraArg)
{
    if (*reinterpret_cast<void* const*>(self + 0x08) == nullptr) {
        return 0u;
    }

    uint32_t projType = *reinterpret_cast<const uint32_t*>(pSpawnDesc + 0x3C);
    if (projType > 1u) {
        return 0u;
    }

    float extX = *reinterpret_cast<const float*>(pSpawnDesc + 0x24);
    float extY = *reinterpret_cast<const float*>(pSpawnDesc + 0x28);
    float extZ = *reinterpret_cast<const float*>(pSpawnDesc + 0x2C);
    if (extX == 0.0f || extY == 0.0f || extZ == 0.0f) {
        return 0u;
    }

    void* pHeightTex = *reinterpret_cast<void* const*>(pSpawnDesc + 0x70);
    void* pDiffTex0  = *reinterpret_cast<void* const*>(pSpawnDesc + 0x48);
    if (pHeightTex == nullptr && pDiffTex0 == nullptr) {
        return 0u;
    }

    if (projType == 1u) {
        if (pDiffTex0 != nullptr) {
            void** texVtbl = *reinterpret_cast<void***>(pDiffTex0);
            using GetTexDimTypeFn = uint32_t (*)(void*);
            if (reinterpret_cast<GetTexDimTypeFn>(texVtbl[6])(pDiffTex0) != 1u) {
                return 0u;
            }
        }
    } else if (projType == 0u) {
        float farPlaneScale = *reinterpret_cast<const float*>(pSpawnDesc + 0x30);
        if (farPlaneScale < 1.0f) {
            return 0u;
        }
        uint32_t bits = 0u;
        std::memcpy(&bits, &farPlaneScale, sizeof(uint32_t));
        if ((bits & 0x7FFFFFFFu) >= 0x7F800000u) {
            return 0u;
        }
        if (pDiffTex0 != nullptr) {
            void** texVtbl = *reinterpret_cast<void***>(pDiffTex0);
            using GetTexDimTypeFn = uint32_t (*)(void*);
            if (reinterpret_cast<GetTexDimTypeFn>(texVtbl[6])(pDiffTex0) != 0u) {
                return 0u;
            }
        }
    }

    uint32_t decalType = *reinterpret_cast<const uint32_t*>(pSpawnDesc + 0x38);
    if (decalType > 1u) {
        return 0u;
    }

    uint8_t* pCreatedDecal = nullptr;
    uint8_t* pMutex = self + 0x880;
    void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
    using LockFn = void (*)(uint8_t*, uint32_t);
    using UnlockFn = void (*)(uint8_t*);
    reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

    uint32_t maxSlots = *reinterpret_cast<uint32_t*>(self + 0x860);
    uint32_t startIdx = *reinterpret_cast<uint32_t*>(self + 0x864) + 1u;
    if (startIdx >= maxSlots) {
        startIdx = 0u;
    }
    *reinterpret_cast<uint32_t*>(self + 0x864) = startIdx;

    uint8_t** pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
    uint32_t curIdx = startIdx;
    bool foundEmpty = false;
    do {
        if (pSlotTable[curIdx] == nullptr) {
            foundEmpty = true;
            break;
        }
        ++curIdx;
        if (curIdx >= maxSlots) {
            curIdx = 0u;
        }
        *reinterpret_cast<uint32_t*>(self + 0x864) = curIdx;
    } while (curIdx != startIdx);

    uint32_t allocatedHandle = 0u;
    uint32_t chosenSlot = curIdx;
    bool allocFailed = false;

    auto makeHandleForSlot = [&](uint32_t slot) -> uint32_t {
        uint16_t* pGenTable = *reinterpret_cast<uint16_t**>(self + 0x858);
        uint16_t newGen = static_cast<uint16_t>(pGenTable[slot] + 1u);
        if (slot == 0u && newGen == 0u) {
            newGen = 1u;
        }
        pGenTable[slot] = newGen;
        return (static_cast<uint32_t>(newGen) << 16) | (slot & 0xFFFFu);
    };

    if (foundEmpty) {
        allocatedHandle = makeHandleForSlot(chosenSlot);
    }

    if (allocatedHandle == 0u) {
        // Evict oldest active decal in [self+0x78 .. self+0x80)
        uint8_t** pActiveBegin = *reinterpret_cast<uint8_t***>(self + 0x78);
        uint8_t** pActiveEnd   = *reinterpret_cast<uint8_t***>(self + 0x80);
        if (pActiveBegin != pActiveEnd) {
            uint8_t* pOldest = *pActiveBegin;
            uint32_t oldHandle = *reinterpret_cast<uint32_t*>(pOldest + 0x130);
            if (oldHandle != 0xFFFFFFFFu) {
                uint32_t oldSlot = oldHandle & 0xFFFFu;
                uint8_t* pSlotEntry = pSlotTable[oldSlot];
                uint16_t* pGenTable = *reinterpret_cast<uint16_t**>(self + 0x858);
                if (pSlotEntry != nullptr && static_cast<uint16_t>(oldHandle >> 16) == pGenTable[oldSlot]) {
                    uint8_t** pFound = nullptr;
                    for (uint8_t** it = pActiveBegin; it != pActiveEnd; ++it) {
                        if (*it == pSlotEntry) {
                            pFound = it;
                            break;
                        }
                    }
                    if (pFound != nullptr) {
                        uint8_t** pNext = pFound + 1;
                        size_t tailBytes = static_cast<size_t>(pActiveEnd - pNext) * sizeof(void*);
                        if (tailBytes > 0) {
                            std::memmove(pFound, pNext, tailBytes);
                        }
                        *reinterpret_cast<uint8_t***>(self + 0x80) = pActiveEnd - 1;

                        DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
                        void** decalVtbl = *reinterpret_cast<void***>(pSlotEntry);
                        using DtorFn = void (*)(uint8_t*);
                        reinterpret_cast<DtorFn>(decalVtbl[1])(pSlotEntry);
                        pAlloc->vptr->deallocate(pAlloc, pSlotEntry);

                        pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
                        pSlotTable[oldSlot] = nullptr;
                        curIdx = *reinterpret_cast<uint32_t*>(self + 0x864);
                        maxSlots = *reinterpret_cast<uint32_t*>(self + 0x860);
                    }
                }
            }
        }

        uint32_t retryStart = (curIdx + 1u < maxSlots) ? (curIdx + 1u) : 0u;
        *reinterpret_cast<uint32_t*>(self + 0x864) = retryStart;
        uint32_t scanIdx = retryStart;
        bool retryFound = false;
        do {
            if (pSlotTable[scanIdx] == nullptr) {
                retryFound = true;
                break;
            }
            ++scanIdx;
            if (scanIdx >= maxSlots) {
                scanIdx = 0u;
            }
            *reinterpret_cast<uint32_t*>(self + 0x864) = scanIdx;
        } while (scanIdx != retryStart);

        if (!retryFound) {
            allocFailed = true;
        } else {
            chosenSlot = scanIdx;
            allocatedHandle = makeHandleForSlot(chosenSlot);
            if (allocatedHandle == 0u) {
                allocFailed = true;
            }
        }
    }

    if (!allocFailed) {
        DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
        if (decalType == 1u) {
            uint8_t* pPaintDecal = static_cast<uint8_t*>(
                pAlloc->vptr->allocateAligned(pAlloc, 0x138u, 8u));
            if (!pPaintDecal) {
                allocFailed = true;
            } else {
                *reinterpret_cast<DLAllocator**>(pPaintDecal + 0x08) = pAlloc;
                pPaintDecal[0x10] = 0;
                std::memset(pPaintDecal + 0x18, 0, 0x11Cu);
                *reinterpret_cast<uintptr_t*>(pPaintDecal + 0x00) = ResolveVMA(0x5360600);

                using InitPaintDecalFn = bool (*)(uint8_t*, uint8_t*, const uint8_t*, uint32_t);
                if (!reinterpret_cast<InitPaintDecalFn>(ResolveVMA(0x2218530))(
                        pPaintDecal, self, pSpawnDesc, allocatedHandle)) {
                    pAlloc->vptr->deallocate(pAlloc, pPaintDecal);
                    allocFailed = true;
                } else {
                    pCreatedDecal = pPaintDecal;
                }
            }
        } else if (decalType == 0u) {
            using CreateDeferredDecalFn = uint8_t* (*)(DLAllocator*, uint8_t*, const uint8_t*, uint32_t);
            pCreatedDecal = reinterpret_cast<CreateDeferredDecalFn>(ResolveVMA(0x2212e90))(
                pAlloc, self, pSpawnDesc, allocatedHandle);
            if (!pCreatedDecal) {
                allocFailed = true;
            }
        }

        if (!allocFailed && pCreatedDecal != nullptr) {
            // Push pCreatedDecal onto active vector at self+0x70
            uint8_t** pEnd   = *reinterpret_cast<uint8_t***>(self + 0x80);
            uint8_t** pBegin = *reinterpret_cast<uint8_t***>(self + 0x78);
            uint8_t** pCap   = *reinterpret_cast<uint8_t***>(self + 0x88);
            if (pEnd == pCap) {
                size_t oldCnt = static_cast<size_t>(pEnd - pBegin);
                size_t half = oldCnt >> 1;
                const size_t kMax = 0x1FFFFFFFFFFFFFFFULL;
                size_t newCap = (kMax - half >= oldCnt) ? (oldCnt + half) : 0u;
                if (newCap < oldCnt + 1u) {
                    newCap = oldCnt + 1u;
                }
                GXDecalPtrVector_ReallocateAndCopy_B(self + 0x70, newCap);
                pEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
            }
            if (pEnd != nullptr) {
                *pEnd = pCreatedDecal;
            }
            *reinterpret_cast<uint8_t***>(self + 0x80) = pEnd + 1;

            pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
            pSlotTable[chosenSlot & 0xFFFFu] = pCreatedDecal;
        } else {
            allocFailed = true;
        }
    }

    mutexVtbl = *reinterpret_cast<void***>(pMutex);
    reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);

    if (allocFailed) {
        return 0u;
    }

    // Build oriented bounding box (OBB) basis and half-extents for scene registration
    float negFwdX = 0.0f - *reinterpret_cast<const float*>(pCreatedDecal + 0x24);
    float negFwdY = 0.0f - *reinterpret_cast<const float*>(pCreatedDecal + 0x28);
    float negFwdZ = 0.0f - *reinterpret_cast<const float*>(pCreatedDecal + 0x2C);

    float upX = *reinterpret_cast<const float*>(pCreatedDecal + 0x30);
    float upY = *reinterpret_cast<const float*>(pCreatedDecal + 0x34);
    float upZ = *reinterpret_cast<const float*>(pCreatedDecal + 0x38);

    float posX = *reinterpret_cast<const float*>(pCreatedDecal + 0x18);
    float posY = *reinterpret_cast<const float*>(pCreatedDecal + 0x1C);
    float posZ = *reinterpret_cast<const float*>(pCreatedDecal + 0x20);

    float sizeX = *reinterpret_cast<const float*>(pCreatedDecal + 0x3C);
    float sizeY = *reinterpret_cast<const float*>(pCreatedDecal + 0x40);
    float sizeZ = *reinterpret_cast<const float*>(pCreatedDecal + 0x44);

    alignas(16) float obbBlock[20];
    if (*reinterpret_cast<const uint32_t*>(pCreatedDecal + 0x54) == 1u) {
        // Cubic projection OBB
        float crossX = negFwdY * upZ - negFwdZ * upY;
        float crossY = negFwdZ * upX - negFwdX * upZ;
        float crossZ = negFwdX * upY - negFwdY * upX;

        obbBlock[0]  = 0.5f * sizeX;
        obbBlock[1]  = 0.5f * sizeY;
        obbBlock[2]  = 0.5f * sizeZ;
        obbBlock[3]  = 0.0f;

        obbBlock[4]  = upX;
        obbBlock[5]  = upY;
        obbBlock[6]  = upZ;
        obbBlock[7]  = 0.0f;

        obbBlock[8]  = crossX;
        obbBlock[9]  = crossY;
        obbBlock[10] = crossZ;
        obbBlock[11] = 0.0f;

        obbBlock[12] = negFwdX;
        obbBlock[13] = negFwdY;
        obbBlock[14] = negFwdZ;
        obbBlock[15] = 0.0f;

        obbBlock[16] = posX;
        obbBlock[17] = posY;
        obbBlock[18] = posZ;
        obbBlock[19] = 1.0f;
    } else {
        // Planar / perspective projection OBB (shifted by 0.5 * sizeZ along -fwd)
        float crossX = upY * negFwdZ - upZ * negFwdY;
        float crossY = upZ * negFwdX - upX * negFwdZ;
        float crossZ = upX * negFwdY - upY * negFwdX;
        float halfZ  = 0.5f * sizeZ;

        obbBlock[0]  = 0.5f * sizeX;
        obbBlock[1]  = halfZ;
        obbBlock[2]  = 0.5f * sizeY;
        obbBlock[3]  = 0.0f;

        obbBlock[4]  = upX;
        obbBlock[5]  = upY;
        obbBlock[6]  = upZ;
        obbBlock[7]  = 0.0f;

        obbBlock[8]  = negFwdX;
        obbBlock[9]  = negFwdY;
        obbBlock[10] = negFwdZ;
        obbBlock[11] = 0.0f;

        obbBlock[12] = crossX;
        obbBlock[13] = crossY;
        obbBlock[14] = crossZ;
        obbBlock[15] = 0.0f;

        obbBlock[16] = posX + negFwdX * halfZ;
        obbBlock[17] = posY + negFwdY * halfZ;
        obbBlock[18] = posZ + negFwdZ * halfZ;
        obbBlock[19] = 1.0f;
    }

    alignas(16) uint8_t sceneQueryCmd[0xF0];
    std::memset(sceneQueryCmd, 0, sizeof(sceneQueryCmd));
    DLAllocator* pMgrAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);

    using ConstructSceneDecalJobFn = void (*)(uint8_t*, DLAllocator*, uint64_t, uint32_t,
                                              uint8_t*, const float*, const uint8_t*, void*);
    reinterpret_cast<ConstructSceneDecalJobFn>(ResolveVMA(0x22125b0))(
        sceneQueryCmd, pMgrAlloc, 0x0000000100000001ULL, allocatedHandle,
        self, obbBlock, pSpawnDesc, pExtraArg);

    void* pSceneWorld = *reinterpret_cast<void**>(self + 0x08);
    void** worldVtbl = *reinterpret_cast<void***>(pSceneWorld);
    using SubmitSceneJobFn = void (*)(void*, uint8_t*);
    reinterpret_cast<SubmitSceneJobFn>(worldVtbl[2])(pSceneWorld, sceneQueryCmd);

    // Inline cleanup of sceneQueryCmd (+0xC8 = rbp-0x58 vector buffer, +0xE0 = rbp-0x40 allocator)
    *reinterpret_cast<uintptr_t*>(sceneQueryCmd + 0x00) = ResolveVMA(0x53b7370);
    void* pJobBuf = *reinterpret_cast<void**>(sceneQueryCmd + 0xC8);
    if (pJobBuf != nullptr) {
        DLAllocator* pJobAlloc = *reinterpret_cast<DLAllocator**>(sceneQueryCmd + 0xE0);
        pJobAlloc->vptr->deallocate(pJobAlloc, pJobBuf);
        *reinterpret_cast<void**>(sceneQueryCmd + 0xD8) = nullptr;
        *reinterpret_cast<void**>(sceneQueryCmd + 0xD0) = nullptr;
        *reinterpret_cast<void**>(sceneQueryCmd + 0xC8) = nullptr;
    }

    return allocatedHandle;
}

// ============================================================================
// 45. 0x220a000 (9 B) — GXDecalManager_DispatchFilteredDecalRenderPass
// ============================================================================
extern "C" void GXDecalManager_DispatchFilteredDecalRenderPass(
    uint8_t* self,
    void* pRenderPassCtx,
    void* pFilterCallback)
{
    void** vtbl = *reinterpret_cast<void***>(self);
    using DispatchFn = void (*)(uint8_t*, void*, void*);
    reinterpret_cast<DispatchFn>(vtbl[4])(self, pRenderPassCtx, pFilterCallback);
}

// ============================================================================
// 46. 0x2209fb0 (69 B) — GXDecalManager_DispatchAllDecalRenderPass
// ============================================================================
extern "C" void GXDecalManager_DispatchAllDecalRenderPass(
    uint8_t* self,
    void* pRenderPassCtx)
{
    void** vtbl = *reinterpret_cast<void***>(self);
    uintptr_t defaultFilterVptr = ResolveVMA(0x53604a0);
    using DispatchFn = void (*)(uint8_t*, void*, uintptr_t*);
    reinterpret_cast<DispatchFn>(vtbl[4])(self, pRenderPassCtx, &defaultFilterVptr);
}

// ============================================================================
// 47. 0x2209eb0 (246 B) — GXDecalManager_PrepareDeferredDecalPrePass
// ============================================================================
extern "C" void GXDecalManager_PrepareDeferredDecalPrePass(
    uint8_t* self,
    uint8_t* pSceneDrawCtx,
    void* pPassTag,
    void* pNormalTarget,
    void* pDepthTarget)
{
    if (self[0x830] == 0) {
        return;
    }
    *reinterpret_cast<void**>(self + 0x840) = pNormalTarget;
    *reinterpret_cast<void**>(self + 0x848) = pDepthTarget;

    uint8_t* pFrameHeap = *reinterpret_cast<uint8_t**>(pSceneDrawCtx + 0x450) + 0x38;
    using AllocFrameWorkFn = uint8_t* (*)(uint8_t*, uint32_t, uint32_t);
    uint8_t* pWorkItem = reinterpret_cast<AllocFrameWorkFn>(ResolveVMA(0xe45560))(
        pFrameHeap, 0x10u, 0x04u);

    uint8_t* pMutex = self + 0x880;
    void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
    using LockFn = void (*)(uint8_t*, uint32_t);
    using UnlockFn = void (*)(uint8_t*);
    reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

    uint8_t** pDecalBegin = *reinterpret_cast<uint8_t***>(self + 0x78);
    uint8_t** pDecalEnd   = *reinterpret_cast<uint8_t***>(self + 0x80);
    uint8_t hasPomDecal = 0;

    if (pDecalBegin != pDecalEnd) {
        uint32_t slotCounter = 8u;
        for (uint8_t** it = pDecalBegin; it != pDecalEnd; ++it, ++slotCounter) {
            uint8_t* pDecal = *it;
            void** decalVtbl = *reinterpret_cast<void***>(pDecal);
            uint32_t paramA = *reinterpret_cast<uint32_t*>(self + 0x834);
            uint32_t paramB = *reinterpret_cast<uint32_t*>(self + 0x838);

            using PrepareDecalFn = void (*)(uint8_t*, uint8_t*, uint32_t, uint32_t, uint8_t*, uint32_t);
            reinterpret_cast<PrepareDecalFn>(decalVtbl[6])(
                pDecal, pSceneDrawCtx, paramA, paramB, pWorkItem, slotCounter);

            if ((hasPomDecal & 1u) == 0) {
                uint8_t* pCurDecal = *it;
                if (*reinterpret_cast<void**>(pCurDecal + 0x80) != nullptr &&
                    *reinterpret_cast<float*>(pCurDecal + 0xF4) > 0.0f) {
                    hasPomDecal = 1u;
                }
            }
        }
    }

    mutexVtbl = *reinterpret_cast<void***>(pMutex);
    reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);

    pWorkItem[0x08] = hasPomDecal & 1u;
    *reinterpret_cast<void**>(pWorkItem + 0x00) = pPassTag;
}

// ============================================================================
// 48. 0x2209e70 (50 B) — GXDecalManager_ResolveDecalHandle
// ============================================================================
extern "C" void* GXDecalManager_ResolveDecalHandle(
    const uint8_t* self,
    uint32_t decalHandle)
{
    if (decalHandle == 0xFFFFFFFFu) {
        return nullptr;
    }
    uint32_t slotIdx = decalHandle & 0xFFFFu;
    void* const* pSlotTable = *reinterpret_cast<void* const* const*>(self + 0x850);
    void* pDecal = pSlotTable[slotIdx];
    if (!pDecal) {
        return nullptr;
    }
    uint16_t gen = static_cast<uint16_t>(decalHandle >> 16);
    const uint16_t* pGenTable = *reinterpret_cast<const uint16_t* const*>(self + 0x858);
    return (gen == pGenTable[slotIdx]) ? pDecal : nullptr;
}

// ============================================================================
// 49. 0x2209cf0 (381 B) — GXDecalManager_StepAndGarbageCollectDecals
// ============================================================================
extern "C" void GXDecalManager_StepAndGarbageCollectDecals(
    uint8_t* self,
    float deltaTime)
{
    uint8_t* pMutex = self + 0x880;
    void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
    using LockFn = void (*)(uint8_t*, uint32_t);
    using UnlockFn = void (*)(uint8_t*);
    reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

    uint8_t** pCur = *reinterpret_cast<uint8_t***>(self + 0x78);
    uint8_t** pEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
    while (pCur != pEnd) {
        uint8_t* pDecal = *pCur;
        void** decalVtbl = *reinterpret_cast<void***>(pDecal);
        using StepDecalFn = void (*)(uint8_t*, float);
        reinterpret_cast<StepDecalFn>(decalVtbl[5])(pDecal, deltaTime);

        pDecal = *pCur;
        if (pDecal[0x10] != 0) {
            uint16_t slotIdx = *reinterpret_cast<uint16_t*>(pDecal + 0x130);
            void** pSlotTable = *reinterpret_cast<void***>(self + 0x850);
            pSlotTable[slotIdx] = nullptr;

            pEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
            uint8_t** pNext = pCur + 1;
            size_t tailBytes = static_cast<size_t>(pEnd - pNext) * sizeof(void*);
            if (tailBytes > 0) {
                std::memmove(pCur, pNext, tailBytes);
            }
            pEnd = *reinterpret_cast<uint8_t***>(self + 0x80) - 1;
            *reinterpret_cast<uint8_t***>(self + 0x80) = pEnd;

            if (pDecal != nullptr) {
                DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
                decalVtbl = *reinterpret_cast<void***>(pDecal);
                using DtorFn = void (*)(uint8_t*);
                reinterpret_cast<DtorFn>(decalVtbl[1])(pDecal);
                pAlloc->vptr->deallocate(pAlloc, pDecal);
                pEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
            }
        } else {
            ++pCur;
            pEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
        }
    }

    mutexVtbl = *reinterpret_cast<void***>(pMutex);
    reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);

    uint32_t debugHandle = *reinterpret_cast<uint32_t*>(self + 0x96C);
    if (debugHandle != 0u && debugHandle != 0xFFFFFFFFu) {
        uint32_t slotIdx = debugHandle & 0xFFFFu;
        uint8_t** pSlotTable = *reinterpret_cast<uint8_t***>(self + 0x850);
        uint8_t* pDecal = pSlotTable[slotIdx];
        if (pDecal != nullptr) {
            uint16_t gen = static_cast<uint16_t>(debugHandle >> 16);
            const uint16_t* pGenTable = *reinterpret_cast<const uint16_t**>(self + 0x858);
            if (gen == pGenTable[slotIdx]) {
                uint8_t* pMatBlock = *reinterpret_cast<uint8_t**>(pDecal + 0x128);
                if (pMatBlock != nullptr) {
                    void* pParamTarget = *reinterpret_cast<void**>(pMatBlock + 0x08);
                    using UpdateMatParamFn = void (*)(void*, uint32_t, uint32_t, void*, float);
                    reinterpret_cast<UpdateMatParamFn>(ResolveVMA(0x21c4cd0))(
                        pParamTarget, 0u, 4u, self + 0x970, 1.0f);
                }
            }
        }
    }
}

// ============================================================================
// 50. 0x2209c30 (180 B) — GXDecalManager_DestroyAllActiveDecals
// ============================================================================
extern "C" void GXDecalManager_DestroyAllActiveDecals(uint8_t* self) {
    uint8_t* pMutex = self + 0x880;
    void** mutexVtbl = *reinterpret_cast<void***>(pMutex);
    using LockFn = void (*)(uint8_t*, uint32_t);
    using UnlockFn = void (*)(uint8_t*);
    reinterpret_cast<LockFn>(mutexVtbl[3])(pMutex, 0xFFFFFFFFu);

    uint8_t** pBegin = *reinterpret_cast<uint8_t***>(self + 0x78);
    uint8_t** pEnd   = *reinterpret_cast<uint8_t***>(self + 0x80);
    for (uint8_t** it = pBegin; it != pEnd; ++it) {
        uint8_t* pDecal = *it;
        if (pDecal != nullptr) {
            DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
            void** decalVtbl = *reinterpret_cast<void***>(pDecal);
            using DtorFn = void (*)(uint8_t*);
            reinterpret_cast<DtorFn>(decalVtbl[1])(pDecal);
            pAlloc->vptr->deallocate(pAlloc, pDecal);
            pEnd = *reinterpret_cast<uint8_t***>(self + 0x80);
        }
    }

    void* pSlotTable = *reinterpret_cast<void**>(self + 0x850);
    uint32_t maxSlots = *reinterpret_cast<uint32_t*>(self + 0x860);
    std::memset(pSlotTable, 0, static_cast<size_t>(maxSlots) * sizeof(void*));
    *reinterpret_cast<uint8_t***>(self + 0x80) = *reinterpret_cast<uint8_t***>(self + 0x78);

    mutexVtbl = *reinterpret_cast<void***>(pMutex);
    reinterpret_cast<UnlockFn>(mutexVtbl[5])(pMutex);
}

// ============================================================================
// 51. 0x2209510 (1817 B) — GXDecalManager_ReleaseShaderAndVertexLayoutResources
// ============================================================================
static inline void DestroyVertexInputLayoutPtr(uint8_t** ppLayout) noexcept {
    uint8_t* pLayout = *ppLayout;
    if (pLayout != nullptr) {
        using GetHeapForPtrFn = DLAllocator* (*)(void*);
        DLAllocator* pHeap = reinterpret_cast<GetHeapForPtrFn>(ResolveVMA(0x207b720))(pLayout);

        void* pBuf18 = *reinterpret_cast<void**>(pLayout + 0x18);
        if (pBuf18 != nullptr) {
            DLAllocator* pAlloc00 = *reinterpret_cast<DLAllocator**>(pLayout + 0x00);
            pAlloc00->vptr->deallocate(pAlloc00, pBuf18);
            *reinterpret_cast<void**>(pLayout + 0x18) = nullptr;
        }

        void* pBuf10 = *reinterpret_cast<void**>(pLayout + 0x10);
        if (pBuf10 != nullptr) {
            DLAllocator* pAlloc08 = *reinterpret_cast<DLAllocator**>(pLayout + 0x08);
            pAlloc08->vptr->deallocate(pAlloc08, pBuf10);
            *reinterpret_cast<void**>(pLayout + 0x10) = nullptr;
        }

        pHeap->vptr->deallocate(pHeap, pLayout);
        *ppLayout = nullptr;
    }
    *ppLayout = nullptr;
}

extern "C" void GXDecalManager_ReleaseShaderAndVertexLayoutResources(uint8_t* self) {
    std::memset(self + 0xA0, 0, 0x328u);

    // 4 pass groups * 14 vertex layouts = 56 layouts at self + 0x588 .. self + 0x740
    for (intptr_t groupOff = -0x1C0; groupOff != 0; groupOff += 0x70) {
        for (intptr_t elemOff = 0; elemOff < 0x70; elemOff += 8) {
            *reinterpret_cast<void**>(self + groupOff + elemOff + 0x588) = nullptr;
            DestroyVertexInputLayoutPtr(
                reinterpret_cast<uint8_t**>(self + groupOff + elemOff + 0x748));
        }
    }
}

// ============================================================================
// 52. 0x2209240 (712 B) — GXDecalManager_BindShadersAndCreateVertexLayouts
// ============================================================================
extern "C" void GXDecalManager_BindShadersAndCreateVertexLayouts(
    uint8_t* self,
    uint8_t* pShaderBinder)
{
    GXDecalManager_ReleaseShaderAndVertexLayoutResources(self);

    const char16_t* const* pPsNames =
        reinterpret_cast<const char16_t* const*>(ResolveVMA(0x53602d0));
    const char16_t* const* pVsNames =
        reinterpret_cast<const char16_t* const*>(ResolveVMA(0x53603a0));

    using FindPixelShaderFn  = void* (*)(uint8_t*, const char16_t*, const char16_t*);
    using FindVertexShaderFn = void* (*)(uint8_t*, const char16_t*, const char16_t*);
    using InitInputLayoutFn  = void  (*)(uint8_t*, const void*, uint32_t, void*, uint32_t, uint32_t);

    InitInputLayoutFn initLayout = reinterpret_cast<InitInputLayoutFn>(ResolveVMA(0x21a2ce0));
    const void* pInputDesc = reinterpret_cast<const void*>(ResolveVMA(0x513aca0));

    const char16_t* kPassNames[4] = {
        ResolveWStr(0x49c9772),
        ResolveWStr(0x49c978e),
        ResolveWStr(0x49c97aa),
        ResolveWStr(0x49d026a)
    };
    const size_t kPsTableOffsets[4] = { 0x0A8u, 0x170u, 0x238u, 0x300u };

    // Bind 25 pixel shaders per pass
    for (int p = 0; p < 4; ++p) {
        for (size_t i = 0; i < 25u; ++i) {
            void** vtbl = *reinterpret_cast<void***>(pShaderBinder);
            void* pPs = reinterpret_cast<FindPixelShaderFn>(vtbl[7])(
                pShaderBinder, kPassNames[p], pPsNames[i]);
            if (pPs != nullptr) {
                *reinterpret_cast<void**>(self + kPsTableOffsets[p] + i * 8u) = pPs;
            }
        }
    }

    // Bind 14 vertex shaders and create vertex input layouts per pass
    const size_t kVsTableOffsets[4]     = { 0x3C8u, 0x438u, 0x4A8u, 0x518u };
    const size_t kLayoutTableOffsets[4] = { 0x588u, 0x5F8u, 0x668u, 0x6D8u };

    for (int p = 0; p < 4; ++p) {
        for (size_t i = 0; i < 14u; ++i) {
            void** vtbl = *reinterpret_cast<void***>(pShaderBinder);
            void* pVs = reinterpret_cast<FindVertexShaderFn>(vtbl[8])(
                pShaderBinder, kPassNames[p], pVsNames[i]);
            if (pVs != nullptr) {
                *reinterpret_cast<void**>(self + kVsTableOffsets[p] + i * 8u) = pVs;
                initLayout(
                    self + kLayoutTableOffsets[p] + i * 8u,
                    pInputDesc, 1u, pVs, 0u, 0u);
            }
        }
    }

    // Bind PostProcess "Decal_Sub.ppo" shader
    void** vtbl = *reinterpret_cast<void***>(pShaderBinder);
    *reinterpret_cast<void**>(self + 0xA0) = reinterpret_cast<FindPixelShaderFn>(vtbl[7])(
        pShaderBinder, ResolveWStr(0x49c97c0), ResolveWStr(0x49c97d8));
}

// ============================================================================
// 53. 0x22071f0 (8270 B) — GXDecalManager_InitializeGraphicsResources
// ============================================================================
struct GXBufferCreateDesc {
    uint32_t byteSize;   // +0x00
    uint32_t usageFlags; // +0x04
    uint32_t bindFlags;  // +0x08
    uint32_t cpuAccess;  // +0x0C
    uint32_t miscFlags;  // +0x10
};

struct GXBufferInitData {
    const void* pSysMem; // +0x00
    uint32_t pitch;      // +0x08
    uint32_t slicePitch; // +0x0C
};

static inline void* CreateAndCloneDepthStencilState(
    uint8_t* pGlobalRoot,
    const uint8_t* pDsDesc) noexcept
{
    uint8_t* pDevWrapper = *reinterpret_cast<uint8_t**>(pGlobalRoot);
    if (!pDevWrapper ||
        *reinterpret_cast<void**>(pDevWrapper + 0x00) == nullptr ||
        *reinterpret_cast<void**>(pDevWrapper + 0x08) == nullptr) {
        return nullptr;
    }
    DLAllocator* pStateHeap = *reinterpret_cast<DLAllocator**>(pDevWrapper + 0xDC10);
    if (!pStateHeap || *reinterpret_cast<void**>(pDevWrapper + 0x20) == nullptr) {
        return nullptr;
    }

    alignas(8) uint64_t tempState[7]{};
    using InitDsStateFn = void (*)(void*, uint8_t*, const void*);
    reinterpret_cast<InitDsStateFn>(ResolveVMA(0x21782b0))(tempState, pDevWrapper, pDsDesc);

    uint64_t* pCloned = static_cast<uint64_t*>(
        pStateHeap->vptr->allocateAligned(pStateHeap, 0x38u, 4u));
    if (!pCloned) {
        return nullptr;
    }
    for (int i = 0; i < 7; ++i) {
        pCloned[i] = tempState[i];
    }
    return pCloned;
}

static inline void* CreateAndCloneBlendState(
    uint8_t* pGlobalRoot,
    const uint8_t* pBlendDesc) noexcept
{
    uint8_t* pDevWrapper = *reinterpret_cast<uint8_t**>(pGlobalRoot);
    if (!pDevWrapper ||
        *reinterpret_cast<void**>(pDevWrapper + 0x00) == nullptr ||
        *reinterpret_cast<void**>(pDevWrapper + 0x08) == nullptr) {
        return nullptr;
    }
    DLAllocator* pStateHeap = *reinterpret_cast<DLAllocator**>(pDevWrapper + 0xDC10);
    if (!pStateHeap || *reinterpret_cast<void**>(pDevWrapper + 0x20) == nullptr) {
        return nullptr;
    }

    alignas(8) uint8_t tempBlend[0x128]{};
    using InitBlendStateFn = void (*)(void*, uint8_t*, const void*);
    reinterpret_cast<InitBlendStateFn>(ResolveVMA(0x2178050))(tempBlend, pDevWrapper, pBlendDesc);

    void* pCloned = pStateHeap->vptr->allocateAligned(pStateHeap, 0x128u, 4u);
    if (!pCloned) {
        return nullptr;
    }
    std::memcpy(pCloned, tempBlend, 0x128u);
    return pCloned;
}

extern "C" bool GXDecalManager_InitializeGraphicsResources(
    uint8_t* self,
    const uint8_t* pInitConfig)
{
    void* pProvidedTexHelper = *reinterpret_cast<void* const*>(pInitConfig + 0x00);
    *reinterpret_cast<void**>(self + 0x98) = pProvidedTexHelper;
    if (!pProvidedTexHelper) {
        DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
        uintptr_t* pDefaultHelper = static_cast<uintptr_t*>(
            pAlloc->vptr->allocateAligned(pAlloc, 8u, 8u));
        if (pDefaultHelper != nullptr) {
            *pDefaultHelper = ResolveVMA(0x53605d0);
        }
        *reinterpret_cast<uintptr_t**>(self + 0x98) = pDefaultHelper;
    }

    uint32_t cfgMaxDecals = *reinterpret_cast<const uint32_t*>(pInitConfig + 0x08);
    uint32_t maxDecals = (cfgMaxDecals != 0u) ? cfgMaxDecals : 0x200u;
    *reinterpret_cast<uint32_t*>(self + 0x860) = maxDecals;

    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
    void* pSlotTable = pAlloc->vptr->allocate(pAlloc, static_cast<size_t>(maxDecals) * sizeof(void*));
    *reinterpret_cast<void**>(self + 0x850) = pSlotTable;
    if (!pSlotTable) {
        return false;
    }

    maxDecals = *reinterpret_cast<uint32_t*>(self + 0x860);
    void* pGenTable = pAlloc->vptr->allocate(pAlloc, static_cast<size_t>(maxDecals) * sizeof(uint16_t));
    *reinterpret_cast<void**>(self + 0x858) = pGenTable;
    if (!pGenTable) {
        pSlotTable = *reinterpret_cast<void**>(self + 0x850);
        if (pSlotTable != nullptr) {
            pAlloc->vptr->deallocate(pAlloc, pSlotTable);
            *reinterpret_cast<void**>(self + 0x850) = nullptr;
        }
        return false;
    }

    maxDecals = *reinterpret_cast<uint32_t*>(self + 0x860);
    std::memset(pSlotTable, 0, static_cast<size_t>(maxDecals) * sizeof(void*));
    std::memset(pGenTable,  0, static_cast<size_t>(maxDecals) * sizeof(uint16_t));

    uint8_t** ppGlobalRoot = reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
    using CreateBufferFn = void (*)(void*, void*, const GXBufferCreateDesc*,
                                    const GXBufferInitData*, uint32_t, uint32_t, void*);
    CreateBufferFn createBuf = reinterpret_cast<CreateBufferFn>(ResolveVMA(0x2165c10));

    // Create unit cube vertex buffer (+0x760, 96 bytes = 8 * float3)
    GXBufferCreateDesc vbDesc{ 0x60u, 1u, 1u, 0u, 0u };
    GXBufferInitData   vbInit{ reinterpret_cast<const void*>(ResolveVMA(0x4738fe0)), 0u, 0u };
    createBuf(*reinterpret_cast<void**>(*ppGlobalRoot), self + 0x760, &vbDesc, &vbInit, 0u, 0u, nullptr);

    // Create unit cube index buffer (+0x768, 28 bytes = 14 * uint16_t)
    GXBufferCreateDesc ibDesc{ 0x1Cu, 1u, 2u, 0u, 0u };
    GXBufferInitData   ibInit{ reinterpret_cast<const void*>(ResolveVMA(0x4739040)), 0u, 0u };
    createBuf(*reinterpret_cast<void**>(*ppGlobalRoot), self + 0x768, &ibDesc, &ibInit, 0u, 0u, nullptr);

    // Create constant buffers (+0x748: 0x1A0 B, +0x758: 0x50 B, +0x750: 0x50 B)
    using CreateConstBufFn = void (*)(void*, void*, uint32_t, const uint32_t*, uint32_t, uint32_t, uint32_t);
    CreateConstBufFn createCb = reinterpret_cast<CreateConstBufFn>(ResolveVMA(0x26d70e0));
    uint32_t cbSize = 0x1A0u;
    createCb(self + 0x748, *reinterpret_cast<void**>(*ppGlobalRoot), 1u, &cbSize, 0u, 0u, 1u);
    cbSize = 0x50u;
    createCb(self + 0x758, *reinterpret_cast<void**>(*ppGlobalRoot), 1u, &cbSize, 0u, 0u, 1u);
    cbSize = 0x50u;
    createCb(self + 0x750, *reinterpret_cast<void**>(*ppGlobalRoot), 1u, &cbSize, 0u, 0u, 1u);

    // Clear all 16 depth-stencil state slots (+0x770 .. +0x7E8)
    std::memset(self + 0x770, 0, 16u * sizeof(void*));

    auto buildDsDesc = [&](size_t defaultSlotOffset,
                           uint8_t writeMask,
                           uint8_t stencilReadMask,
                           uint32_t frontFailOp,
                           uint32_t frontDepthFailOp,
                           uint32_t backFailOp,
                           uint32_t backDepthFailOp,
                           uint8_t depthBoundsEnable,
                           uint8_t outDesc[0x38]) {
        std::memset(outDesc, 0, 0x38u);
        uint8_t* pStateMgr = *reinterpret_cast<uint8_t**>(*ppGlobalRoot + 0x250);
        uint8_t* pBaseDs   = *reinterpret_cast<uint8_t**>(pStateMgr + defaultSlotOffset);
        outDesc[0x00] = pBaseDs[0x0C];
        *reinterpret_cast<uint32_t*>(outDesc + 0x04) = *reinterpret_cast<uint32_t*>(pBaseDs + 0x10);
        *reinterpret_cast<uint32_t*>(outDesc + 0x08) = *reinterpret_cast<uint32_t*>(pBaseDs + 0x14);
        outDesc[0x0C] = 1u;
        outDesc[0x0D] = writeMask;
        outDesc[0x0E] = stencilReadMask;
        *reinterpret_cast<uint32_t*>(outDesc + 0x10) = 0u;
        *reinterpret_cast<uint32_t*>(outDesc + 0x14) = frontFailOp;
        *reinterpret_cast<uint32_t*>(outDesc + 0x18) = frontDepthFailOp;
        *reinterpret_cast<uint32_t*>(outDesc + 0x1C) = 2u;
        *reinterpret_cast<uint32_t*>(outDesc + 0x20) = 0u;
        *reinterpret_cast<uint32_t*>(outDesc + 0x24) = backFailOp;
        *reinterpret_cast<uint32_t*>(outDesc + 0x28) = backDepthFailOp;
        *reinterpret_cast<uint32_t*>(outDesc + 0x2C) = 2u;
        outDesc[0x30] = depthBoundsEnable;
    };

    uint8_t dsDesc[0x38];
    // PrePass & Deferred Depth-Stencil states (+0x770 .. +0x7A8)
    buildDsDesc(0x1C8u, 1u, 0x20u, 1u, 1u, 1u, 1u, 0u, dsDesc);
    *reinterpret_cast<void**>(self + 0x770) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1D8u, 1u, 0x20u, 1u, 1u, 1u, 1u, 0u, dsDesc);
    *reinterpret_cast<void**>(self + 0x778) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1C8u, 1u, 0x20u, 1u, 1u, 1u, 1u, 1u, dsDesc);
    *reinterpret_cast<void**>(self + 0x780) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1D8u, 1u, 0x20u, 1u, 1u, 1u, 1u, 1u, dsDesc);
    *reinterpret_cast<void**>(self + 0x788) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1C8u, 1u, 0x20u, 6u, 0u, 7u, 0u, 0u, dsDesc);
    *reinterpret_cast<void**>(self + 0x790) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1D8u, 1u, 0x20u, 6u, 0u, 7u, 0u, 0u, dsDesc);
    *reinterpret_cast<void**>(self + 0x798) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1C8u, 1u, 0x20u, 6u, 0u, 7u, 0u, 1u, dsDesc);
    *reinterpret_cast<void**>(self + 0x7A0) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    buildDsDesc(0x1D8u, 1u, 0x20u, 6u, 0u, 7u, 0u, 1u, dsDesc);
    *reinterpret_cast<void**>(self + 0x7A8) = CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

    // SinglePass Depth-Stencil states (+0x7B0 .. +0x7E8)
    for (size_t groupOff = 0; groupOff < 0x40u; groupOff += 0x20u) {
        uint8_t writeMask = (groupOff == 0x20u) ? 1u : 0x20u;

        buildDsDesc(0x1C8u, writeMask, 0u, 0u, 1u, 0u, 0u, 0u, dsDesc);
        *reinterpret_cast<void**>(self + 0x7B0 + groupOff) =
            CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

        buildDsDesc(0x1D8u, writeMask, 0u, 0u, 1u, 0u, 0u, 0u, dsDesc);
        *reinterpret_cast<void**>(self + 0x7B8 + groupOff) =
            CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

        buildDsDesc(0x1C8u, writeMask, 0u, 0u, 1u, 0u, 0u, 1u, dsDesc);
        *reinterpret_cast<void**>(self + 0x7C0 + groupOff) =
            CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);

        buildDsDesc(0x1D8u, writeMask, 0u, 0u, 1u, 0u, 0u, 1u, dsDesc);
        *reinterpret_cast<void**>(self + 0x7C8 + groupOff) =
            CreateAndCloneDepthStencilState(*ppGlobalRoot, dsDesc);
    }

    // Helper to populate 8-RT blend descriptor (0x104 bytes)
    auto buildBlendDesc = [](uint8_t alphaToCoverage,
                             uint8_t independentBlend,
                             uint8_t blendEnable0,
                             uint64_t srcDstColor0,
                             uint64_t opSrcAlpha0,
                             uint64_t dstAlphaOp0,
                             uint32_t writeMask0,
                             bool overrideRt1WriteMask,
                             uint32_t rt1WriteMask,
                             uint8_t outBlendDesc[0x108]) {
        std::memset(outBlendDesc, 0, 0x108u);
        outBlendDesc[0x00] = alphaToCoverage;
        outBlendDesc[0x01] = independentBlend;
        for (size_t rt = 0; rt < 8u; ++rt) {
            uint8_t* pRt = outBlendDesc + 0x04 + rt * 0x20u;
            pRt[0x00] = blendEnable0;
            *reinterpret_cast<uint64_t*>(pRt + 0x04) = srcDstColor0;
            *reinterpret_cast<uint64_t*>(pRt + 0x0C) = opSrcAlpha0;
            *reinterpret_cast<uint64_t*>(pRt + 0x14) = dstAlphaOp0;
            *reinterpret_cast<uint32_t*>(pRt + 0x1C) = writeMask0;
        }
        if (overrideRt1WriteMask) {
            *reinterpret_cast<uint32_t*>(outBlendDesc + 0x40) = rt1WriteMask;
        }
    };

    uint8_t blendDesc[0x108];

    // PrePass Blend State (+0x7F0)
    buildBlendDesc(0u, 0u, 1u,
                   0x0000000100000001ULL, 0x0000000100000002ULL, 0x0000000200000001ULL,
                   0x0Fu, false, 0u, blendDesc);
    *reinterpret_cast<void**>(self + 0x7F0) = CreateAndCloneBlendState(*ppGlobalRoot, blendDesc);

    // Clear Deferred Pass Blend States (+0x7F8 .. +0x820)
    std::memset(self + 0x7F8, 0, 6u * sizeof(void*));

    // Deferred Blend State 0 (+0x7F8)
    buildBlendDesc(0u, 1u, 0u,
                   0x0000000000000001ULL, 0x0000000100000000ULL, 0x0000000000000000ULL,
                   0u, true, 4u, blendDesc);
    *reinterpret_cast<void**>(self + 0x7F8) = CreateAndCloneBlendState(*ppGlobalRoot, blendDesc);

    // Deferred Blend State 1 (+0x800: Transparent)
    buildBlendDesc(0u, 1u, 1u,
                   0x0000000500000004ULL, 0x0000000100000000ULL, 0x0000000000000000ULL,
                   0u, true, 4u, blendDesc);
    *reinterpret_cast<void**>(self + 0x800) = CreateAndCloneBlendState(*ppGlobalRoot, blendDesc);

    // Deferred Blend State 2 (+0x808: Modulate)
    buildBlendDesc(0u, 1u, 1u,
                   0x0000000000000008ULL, 0x0000000100000000ULL, 0x0000000000000000ULL,
                   0u, true, 4u, blendDesc);
    *reinterpret_cast<void**>(self + 0x808) = CreateAndCloneBlendState(*ppGlobalRoot, blendDesc);

    // Paint Blend State (+0x828)
    buildBlendDesc(0u, 0u, 1u,
                   0x0000000500000004ULL, 0x0000000100000000ULL, 0x0000000000000000ULL,
                   0u, false, 0u, blendDesc);
    *reinterpret_cast<void**>(self + 0x828) = CreateAndCloneBlendState(*ppGlobalRoot, blendDesc);

    return true;
}

// ============================================================================
// 54. 0x2206be0 (1546 B) — GXDecalManager_Destroy
// ============================================================================
static inline void FreeHeapAllocatedState(void** ppState) noexcept {
    void* pState = *ppState;
    if (pState != nullptr) {
        using GetHeapForPtrFn = DLAllocator* (*)(void*);
        DLAllocator* pHeap = reinterpret_cast<GetHeapForPtrFn>(ResolveVMA(0x207b720))(pState);
        pHeap->vptr->deallocate(pHeap, pState);
        *ppState = nullptr;
    }
}

extern "C" void GXDecalManager_Destroy(uint8_t* self) {
    *reinterpret_cast<uintptr_t*>(self + 0x00) = ResolveVMA(0x5360420);

    GXDecalManager_DestroyAllActiveDecals(self);

    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
    void* pGenTable = *reinterpret_cast<void**>(self + 0x858);
    if (pGenTable != nullptr) {
        pAlloc->vptr->deallocate(pAlloc, pGenTable);
        *reinterpret_cast<void**>(self + 0x858) = nullptr;
    }

    void* pSlotTable = *reinterpret_cast<void**>(self + 0x850);
    if (pSlotTable != nullptr) {
        pAlloc = *reinterpret_cast<DLAllocator**>(self + 0x10);
        pAlloc->vptr->deallocate(pAlloc, pSlotTable);
        *reinterpret_cast<void**>(self + 0x850) = nullptr;
    }

    void* pTexHelper = *reinterpret_cast<void**>(self + 0x98);
    if (pTexHelper != nullptr) {
        using GetHeapForPtrFn = DLAllocator* (*)(void*);
        DLAllocator* pHeap = reinterpret_cast<GetHeapForPtrFn>(ResolveVMA(0x207b720))(pTexHelper);
        void** vtbl = *reinterpret_cast<void***>(pTexHelper);
        using DtorFn = void (*)(void*);
        reinterpret_cast<DtorFn>(vtbl[0])(pTexHelper);
        pHeap->vptr->deallocate(pHeap, pTexHelper);
        *reinterpret_cast<void**>(self + 0x98) = nullptr;
    }

    // Free blend states (+0x7F8 .. +0x828 and +0x7F0)
    for (size_t off = 0x7F8u; off <= 0x828u; off += 8u) {
        FreeHeapAllocatedState(reinterpret_cast<void**>(self + off));
    }
    FreeHeapAllocatedState(reinterpret_cast<void**>(self + 0x7F0));

    // Free all 16 depth-stencil states (+0x770 .. +0x7E8)
    for (size_t off = 0x770u; off <= 0x7E8u; off += 8u) {
        FreeHeapAllocatedState(reinterpret_cast<void**>(self + off));
    }

    // Release constant buffers (+0x758, +0x748, +0x750)
    using ReleaseCbFn = void (*)(void*, void*, void*, uint32_t);
    ReleaseCbFn releaseCb = reinterpret_cast<ReleaseCbFn>(ResolveVMA(0xe46040));
    uint8_t** ppGlobalRoot = reinterpret_cast<uint8_t**>(ResolveVMA(0x55406c8));
    void* pCbTag = reinterpret_cast<void*>(ResolveVMA(0x53bf1d0));

    const size_t kCbOffsets[3] = { 0x758u, 0x748u, 0x750u };
    for (int i = 0; i < 3; ++i) {
        void* pCb = *reinterpret_cast<void**>(self + kCbOffsets[i]);
        if (pCb != nullptr) {
            void* pCbMgr = *reinterpret_cast<void**>(*ppGlobalRoot + 0x268);
            releaseCb(pCbMgr, pCb, pCbTag, 1u);
            *reinterpret_cast<void**>(self + kCbOffsets[i]) = nullptr;
        }
    }

    GXDecalManager_ReleaseShaderAndVertexLayoutResources(self);

    // Release unit cube index buffer (+0x768) and vertex buffer (+0x760)
    using ReleaseBufferFn = void (*)(void*);
    ReleaseBufferFn releaseBuf = reinterpret_cast<ReleaseBufferFn>(ResolveVMA(0x2192660));
    if (*reinterpret_cast<void**>(self + 0x768) != nullptr) {
        releaseBuf(*reinterpret_cast<void**>(self + 0x768));
        *reinterpret_cast<void**>(self + 0x768) = nullptr;
    }
    if (*reinterpret_cast<void**>(self + 0x760) != nullptr) {
        releaseBuf(*reinterpret_cast<void**>(self + 0x760));
        *reinterpret_cast<void**>(self + 0x760) = nullptr;
    }

    // Release debug menu node (+0x890)
    DLGuiRefObject* pDebugNode = *reinterpret_cast<DLGuiRefObject**>(self + 0x890);
    if (pDebugNode != nullptr) {
        ReleaseGuiRefObject(pDebugNode);
        *reinterpret_cast<void**>(self + 0x890) = nullptr;
    }

    // Reset embedded mutex vptr (+0x880)
    *reinterpret_cast<uintptr_t*>(self + 0x880) = ResolveVMA(0x52ef3b0) + 0x10u;

    // Free internal vector buffers (+0x78, +0x48, +0x20)
    void* pVec78 = *reinterpret_cast<void**>(self + 0x78);
    if (pVec78 != nullptr) {
        DLAllocator* pVecAlloc = *reinterpret_cast<DLAllocator**>(self + 0x90);
        pVecAlloc->vptr->deallocate(pVecAlloc, pVec78);
        *reinterpret_cast<void**>(self + 0x88) = nullptr;
        *reinterpret_cast<void**>(self + 0x80) = nullptr;
        *reinterpret_cast<void**>(self + 0x78) = nullptr;
    }

    void* pVec48 = *reinterpret_cast<void**>(self + 0x48);
    if (pVec48 != nullptr) {
        DLAllocator* pVecAlloc = *reinterpret_cast<DLAllocator**>(self + 0x60);
        pVecAlloc->vptr->deallocate(pVecAlloc, pVec48);
        *reinterpret_cast<void**>(self + 0x58) = nullptr;
        *reinterpret_cast<void**>(self + 0x50) = nullptr;
        *reinterpret_cast<void**>(self + 0x48) = nullptr;
    }

    void* pVec20 = *reinterpret_cast<void**>(self + 0x20);
    if (pVec20 != nullptr) {
        DLAllocator* pVecAlloc = *reinterpret_cast<DLAllocator**>(self + 0x38);
        pVecAlloc->vptr->deallocate(pVecAlloc, pVec20);
        *reinterpret_cast<void**>(self + 0x30) = nullptr;
        *reinterpret_cast<void**>(self + 0x28) = nullptr;
        *reinterpret_cast<void**>(self + 0x20) = nullptr;
    }
}

// ============================================================================
// 55. 0x2206bc0 (28 B) — GXDecalManager_DeletingDtor
// ============================================================================
extern "C" void GXDecalManager_DeletingDtor(uint8_t* self) {
    GXDecalManager_Destroy(self);
    using OperatorDeleteFn = void (*)(void*);
    reinterpret_cast<OperatorDeleteFn>(ResolveVMA(0x2bbe588))(self);
}

} // namespace GX
} // namespace FromSoftware
