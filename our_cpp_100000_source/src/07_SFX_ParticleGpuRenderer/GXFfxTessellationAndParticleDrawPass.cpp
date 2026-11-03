// ============================================================================
// Bloodborne (CUSA03173 v1.09) - 07_SFX_ParticleGpuRenderer (Step 3 / 3)
// File: GXFfxTessellationAndParticleDrawPass.cpp
// Range: 0x2304000 .. 0x2309000 (67 functions, 18,787 bytes)
// Description:
//   Particle Draw Command Linear Heap & Bucket Dispatchers, FXR3 Hash/Vector
//   Containers, GXFfxSceneCtrl Action List & Tessellation Draw Pass Helpers
//   (Right-to-Left Decompilation, 0 Stubs).
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <immintrin.h>

namespace {

struct FfxDiv64Result {
    int64_t quot;
    int64_t rem;
};

struct alignas(8) FfxIter24 {
    uint64_t w0;
    uint64_t w1;
    void*    pNode;
};

struct alignas(8) FfxInsertResult32 {
    uint64_t w0;
    uint64_t w1;
    void*    pNode;
    uint8_t  inserted;
    uint8_t  pad[7];
};

static inline uint64_t computeParkMillerHash64(uint64_t key, FfxDiv64Result (*pfnLdiv)(int64_t, int64_t)) {
    const uint64_t masked = key & 0x7fffffffffffffffULL;
    const int64_t xored = static_cast<int64_t>(masked ^ 0xdeadbeefULL);
    const FfxDiv64Result divRes = pfnLdiv(xored, 0x1f31dLL);
    const int64_t prodRem = divRes.rem * 0x41a7LL;
    const int64_t prodQuot = divRes.quot * -2836LL;
    const int64_t combined = prodQuot + prodRem;
    const uint64_t uCombined = static_cast<uint64_t>(combined);
    return (combined >= 0) ? uCombined : (uCombined + 0x7fffffffffffffffULL);
}

static inline uint64_t u64ToFloatBitsHelper(uint64_t v, float* outFloat) {
    if (static_cast<int64_t>(v) >= 0) {
        *outFloat = static_cast<float>(static_cast<int64_t>(v));
    } else {
        uint64_t half = (v >> 1) | (v & 1ULL);
        float f = static_cast<float>(static_cast<int64_t>(half));
        *outFloat = f + f;
    }
    return v;
}

} // namespace

extern "C" {

// External engine / kernel / CRT functions
void sub_ee5de0(uint32_t code);
void sub_f16a90(void* pDst, void* pStr, void* a3, void* a4, void* a5, uint32_t a6);
void sub_f16e70(void* pSelf);
uint64_t sub_f1d630(void* pObj, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
void sub_20b55b0(const char* pFile, uint32_t line, const char* pMsg, ...);
void sub_2282d10(void* pSelf);
void sub_2302050(void* pOutDesc, void* pSceneCtrl, uint32_t id, void* pMatData, void* a5, uint32_t a6, uint32_t stackZero);
void sub_236c4d0(void* pCtx);
void sub_236c720(void* pCtx);
void sub_284cf50(void* pDst, void* pSrc);
void sub_284d4f0(void* pDst1, void* pDst2, const void* pSrcMat);
void* sub_284f0e0(void* pNode);
void* sub_284f340(void* pNode);
void sub_284ff30(void* pItem, void* pCtx, float dt);
void sub_28512d0(void* pDst, const void* pSrc);
void sub_2851450(void* pObj);
void sub_28517d0(void* pObj, void* pSub, uint32_t size, void* pFn);
void sub_2851840(void* pObj);
uint32_t* sub_2869560(void* pObj);
void sub_29416c0(void* pVec, const void* pVal);
void sub_2952bf0(void* pVec, uint64_t newCap);
void* sub_2ba2b10();
void sub_2bbe588(void* ptr);
void* sub_2bbe658(void* dst, const void* src, size_t len);
void* sub_2bbe688(void* dst, int val, size_t len);
int sub_2bbe8f8(void* pJmpBuf);
[[noreturn]] void sub_2bbe978(void* pJmpBuf, int val);
[[noreturn]] void sub_2bbf1b8(const char* msg);
FfxDiv64Result sub_2bbfd68(int64_t numer, int64_t denom);

// Step 2 (0x2309000 .. 0x2310000) sibling functions
void sub_2309350(void* pHash, void** ppAllocator);
bool sub_23094c0(void* pVec, uint64_t newCap);
bool sub_2309560(void* pVec, uint64_t newCap);
void sub_23096e0(void* pVec);
FfxIter24* sub_2309830(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter);
void sub_2309d80(void* pVec, uint64_t newCap);
FfxInsertResult32* sub_2309e80(
    FfxInsertResult32* pOutPair,
    void* pHash,
    uint8_t* pWhereNode,
    uint64_t bucket,
    FfxIter24 plistIter);

// Forward declarations for all 67 functions in 0x2304000 .. 0x2309000
void* sub_2308900(void* pSelf, void* pItem, uint32_t typeId, uint64_t allocSize);
void sub_23088f0();
void sub_23087a0(void* pSelf);
void sub_2308780(void* pSelf);
void sub_2308560(
    void* pSelf,
    void* pDrawQueue,
    void* pBucketMgr,
    void* pField08,
    void* pField28,
    const void* pDescR9,
    void* pAllocator,
    void* pCtxA8,
    uint32_t blockSize);
void sub_23084f0(void* pSelf, const int32_t* pRange);
void sub_2308440(void* pSelf);
void sub_23083b0(void* pSelf);
void sub_23081d0(void* pSelf, void* pAllocator, void* pField08, void* pField10);
void sub_2308150(void* pSelf, const int32_t* pRange);
void sub_2307ff0(void* pSelf);
void sub_2307fd0(void* pSelf);
void sub_23078a0(void* pSelf, void* pAllocator, void* pField08, void* pField10);
void sub_23077a0(void* pVec, uint64_t newCap);
FfxInsertResult32* sub_2307390(
    FfxInsertResult32* pOutPair,
    void* pHash,
    const uint64_t* pKey,
    FfxIter24 plistIter);
[[noreturn]] void sub_2307380();
void sub_2307280(void* pVec, uint64_t newCap);
void sub_23071a0(void* pVec, const uint64_t* pVal);
void sub_2307120(void* pVec);
FfxIter24* sub_2306bd0(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter);
[[noreturn]] void sub_2306bc0();
void sub_2306ac0(void* pVec, uint64_t newCap);
void sub_2306700(void* pList, void* pWhereNode, const void* pSrcData);
FfxIter24* sub_2306580(FfxIter24* pOutIter, void* pHash, uint8_t* pEraseNode);
void sub_2305f50(
    void* pVec,
    FfxIter24 posIter,
    FfxIter24 firstIter,
    FfxIter24 lastIter);
void sub_2305eb0(void* pVec, uint64_t minCap);
void sub_2305dd0(void* pVec, const uint64_t* pVal);
void sub_23059d0(
    void* pVec,
    FfxIter24 posIter,
    FfxIter24 firstIter,
    FfxIter24 lastIter);
void sub_2305900(void* pSelf, void* pObj);
void sub_2305790(void* pVec, uint64_t newCap);
void sub_2305640(void* pVec);
void sub_2305560(void* pVec, const uint64_t* pVal);
void sub_23054f0(void* pSelf, const int32_t* pRange);
void sub_2305490(void* pSelf);
void sub_2305410(void* pSelf);
void sub_23053c0(void* pSelf, const int32_t* pRange, int32_t slotIdx);
void sub_2305350(void* pSelf);
void sub_23052d0(void* pSelf);
void sub_2305270(void* pSelf, const int32_t* pRange, int32_t slotIdx);
void sub_2305230(void* pSelf);
void sub_23051e0(void* pSelf);
const void* sub_23051c0(const void* pSelf);
uint64_t sub_23051a0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_2305180(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
void sub_2305160(void* pSelf);
void sub_2305150(void* pSelf);
void sub_2305120(void* pSelf);
void sub_2305110(void* pSelf);
void sub_23050f0(void* pSelf);
void sub_23050e0();
uint64_t sub_23050d0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_23050c0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_23050b0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_23050a0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_2305090(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_2305080(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_2305070(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
bool sub_2305060();
void sub_2304e00(void* pSelf, uint64_t key);
void sub_2304a60(void* pSelf, uint64_t key, void* pVal20, uint32_t val2C, uint32_t val28);
void sub_2304a00(void* pSelf);
uint64_t sub_23049f0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
void sub_23048f0(void* pSelf);
void sub_23047d0(void* pSelf);
void sub_23046b0(void* pSelf);
void sub_23045f0(void* pSelf);
void sub_2304570(void* pSelf);

} // extern "C"

namespace {

static inline void handleContainerAllocFailure() {
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    if (*reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) != 0) {
        sub_2bbe978(reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL))), 1);
    }
    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
}

static inline void validateContainerHeap(void* pHeap) {
    uint8_t flagsBuf[8] = {};
    void* vtbl = *reinterpret_cast<void**>(pHeap);
    auto fnCheck = *reinterpret_cast<void (**)(void*, void*, uint32_t)>(
        static_cast<uint8_t*>(vtbl) + 0x20);
    fnCheck( flagsBuf, pHeap, 0);
    if ((flagsBuf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
}

static inline void setTlsAllocGuard(uint32_t val) {
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    *reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) = val;
}

static inline void* getTlsJmpBuf() {
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    return reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL)));
}

} // namespace

// ============================================================================
// 1. sub_2308900 (0x2308900, 2,238 B)
//    Allocates an aligned draw packet from the linear heap at pSelf+0x30 and
//    queues {pItem, pHeader} into either the draw vector (+0x90->+0x2ad0) or
//    the shader-keyed bucket hash table (+0x98).
// ============================================================================
extern "C" void* sub_2308900(void* pSelf, void* pItem, uint32_t typeId, uint64_t allocSize) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint64_t* pLinearBlock = *reinterpret_cast<uint64_t**>(self + 0x30);
    if (!pLinearBlock) {
        return nullptr;
    }

    uint64_t cur = pLinearBlock[2];
    uint64_t end = pLinearBlock[3];
    if ((end - cur) < (allocSize + 0x20ULL)) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0xa0);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnGetAvail = *reinterpret_cast<uint64_t (**)(void*)>(static_cast<uint8_t*>(vtbl) + 0x38);
        uint64_t avail = fnGetAvail(pAlloc);
        uint32_t blockSize = *reinterpret_cast<uint32_t*>(self + 0x88);
        if (avail <= static_cast<uint64_t>(blockSize + 0x80000u)) {
            *reinterpret_cast<uint64_t**>(self + 0x30) = nullptr;
            sub_ee5de0(6);
            return nullptr;
        }

        pAlloc = *reinterpret_cast<void**>(self + 0xa0);
        vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnAlloc = *reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58);
        uint64_t rawBuf = reinterpret_cast<uint64_t>(fnAlloc(pAlloc, *reinterpret_cast<uint32_t*>(self + 0x88), 0x10));

        pAlloc = *reinterpret_cast<void**>(self + 0xa0);
        vtbl = *reinterpret_cast<void**>(pAlloc);
        fnAlloc = *reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58);
        uint64_t* newBlock = static_cast<uint64_t*>(fnAlloc(pAlloc, 0x20, 0x8));
        if (!rawBuf || !newBlock) {
            auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
            if (newBlock) {
                fnFree(pAlloc, newBlock);
            }
            if (rawBuf) {
                fnFree(pAlloc, reinterpret_cast<void*>(rawBuf));
            }
            *reinterpret_cast<uint64_t**>(self + 0x30) = nullptr;
            sub_ee5de0(6);
            return nullptr;
        }
        uint64_t blkEnd = rawBuf + *reinterpret_cast<uint32_t*>(self + 0x88);
        newBlock[0] = rawBuf;
        newBlock[1] = blkEnd;
        newBlock[2] = rawBuf;
        newBlock[3] = blkEnd;
        uint64_t blockPtrVal = reinterpret_cast<uint64_t>(newBlock);
        *reinterpret_cast<uint64_t**>(self + 0x30) = newBlock;
        sub_2305560(self + 0x38, &blockPtrVal);
        sub_29416c0(self + 0x60, &rawBuf);

        pLinearBlock = *reinterpret_cast<uint64_t**>(self + 0x30);
        if (!pLinearBlock) {
            sub_ee5de0(6);
            return nullptr;
        }
        cur = pLinearBlock[2];
        end = pLinearBlock[3];
    }

    uint64_t pad = static_cast<uint64_t>(-static_cast<int32_t>(cur)) & 0xFULL;
    uint64_t nextCur = cur + pad + allocSize + 0x10ULL;
    if (nextCur > end) {
        sub_ee5de0(6);
        return nullptr;
    }

    pLinearBlock[2] = nextCur;
    uint64_t headerAddr = cur + pad;
    if (headerAddr == 0) {
        sub_ee5de0(6);
        return nullptr;
    }

    uint8_t* pHeader = reinterpret_cast<uint8_t*>(headerAddr);
    *reinterpret_cast<uint64_t*>(pHeader + 0x8) = 0;
    *reinterpret_cast<uint64_t*>(pHeader + 0x0) = 0;
    *reinterpret_cast<uint32_t*>(pHeader + 0x0) = typeId;

    void* itemVtbl = *reinterpret_cast<void**>(pItem);
    auto fnItemV20 = *reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(itemVtbl) + 0x20);
    fnItemV20(pItem);

    if (typeId <= 5u && ((0x27u >> typeId) & 1u) != 0) {
        if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x5540df9ULL)) != 0) {
            return reinterpret_cast<void*>(cur + (pad | 0x10ULL));
        }
        itemVtbl = *reinterpret_cast<void**>(pItem);
        auto fnItemV50 = *reinterpret_cast<uint8_t* (**)(void*)>(static_cast<uint8_t*>(itemVtbl) + 0x50);
        uint8_t* pRes = fnItemV50(pItem);
        uint8_t* pSub = *reinterpret_cast<uint8_t**>(pRes + 0x58);
        uint8_t* pInner = *reinterpret_cast<uint8_t**>(pSub + 0x40);
        if (pInner) {
            uint8_t* pCtx = *reinterpret_cast<uint8_t**>(self + 0xa8);
            uint32_t mask = ~(*reinterpret_cast<uint32_t*>(pCtx + 0x164));
            if ((*reinterpret_cast<uint32_t*>(pInner + 0x1f8) & mask) == 0) {
                return reinterpret_cast<void*>(cur + (pad | 0x10ULL));
            }
        }
    }

    if ((typeId - 3u) <= 2u) {
        uint8_t* pMgr = *reinterpret_cast<uint8_t**>(self + 0x98);
        if (*reinterpret_cast<uint64_t*>(pMgr + 0x98) == 0) {
            return nullptr;
        }
        uint64_t nodeAddr = *reinterpret_cast<uint64_t*>(pMgr + 0x88);
        uint64_t nextNodeAddr = nodeAddr + 0x18ULL;
        if (nextNodeAddr > *reinterpret_cast<uint64_t*>(pMgr + 0x90) || nodeAddr == 0) {
            sub_ee5de0(6);
            return nullptr;
        }
        *reinterpret_cast<uint64_t*>(pMgr + 0x88) = nextNodeAddr;
        uint64_t* pChainNode = reinterpret_cast<uint64_t*>(nodeAddr);

        itemVtbl = *reinterpret_cast<void**>(pItem);
        auto fnItemV50 = *reinterpret_cast<uint8_t* (**)(void*)>(static_cast<uint8_t*>(itemVtbl) + 0x50);
        uint8_t* pRes = fnItemV50(pItem);
        uint64_t key = (pRes != nullptr) ? *reinterpret_cast<uint64_t*>(pRes + 0x58) : 0ULL;
        if (!pRes || key == 0) {
            if (nodeAddr < *reinterpret_cast<uint64_t*>(pMgr + 0x78) ||
                nodeAddr >= *reinterpret_cast<uint64_t*>(pMgr + 0x80)) {
                sub_20b55b0(
                    reinterpret_cast<const char*>(GX_VMA(0x493c62dULL)),
                    0x16e,
                    reinterpret_cast<const char*>(GX_VMA(0x493c801ULL)));
            }
            return nullptr;
        }

        uint64_t hashVal = computeParkMillerHash64(key, sub_2bbfd68);
        uint64_t mask = *reinterpret_cast<uint64_t*>(pMgr + 0x60);
        uint64_t bucket = hashVal & mask;
        if (bucket >= *reinterpret_cast<uint64_t*>(pMgr + 0x68)) {
            bucket += ~(mask >> 1);
        }

        uint8_t* pHead = *reinterpret_cast<uint8_t**>(pMgr + 0x20);
        uint8_t* pBuckets = *reinterpret_cast<uint8_t**>(pMgr + 0x40);
        uint8_t* pIter = *reinterpret_cast<uint8_t**>(pBuckets + bucket * 48 + 0x10);
        uint8_t* pFound = pHead;
        uint8_t* pWhere = pHead;
        if (pIter != pHead) {
            uint8_t* pBucketEnd = **reinterpret_cast<uint8_t***>(pBuckets + ((bucket * 2) | 1ULL) * 24 + 0x10);
            while (pIter != pBucketEnd) {
                uint64_t iterKey = *reinterpret_cast<uint64_t*>(pIter + 0x10);
                if (iterKey >= key) {
                    if (key < iterKey) {
                        pWhere = pIter;
                    } else {
                        pFound = pIter;
                    }
                    break;
                }
                pIter = *reinterpret_cast<uint8_t**>(pIter);
            }
            if (pFound == pHead && pWhere == pHead) {
                pWhere = pBucketEnd;
            }
        }

        uint64_t* pSlot = nullptr;
        if (pFound != pHead) {
            pSlot = reinterpret_cast<uint64_t*>(pFound + 0x18);
        } else {
            uint8_t* pOldFirst = *reinterpret_cast<uint8_t**>(pHead);
            uint8_t* pOldPrev = *reinterpret_cast<uint8_t**>(pOldFirst + 0x8);
            void* pNodeAlloc = *reinterpret_cast<void**>(pMgr + 0x30);
            void* nodeAllocVtbl = *reinterpret_cast<void**>(pNodeAlloc);
            auto fnNodeAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(
                static_cast<uint8_t*>(nodeAllocVtbl) + 0x58);
            uint8_t* pNewNode = fnNodeAlloc(pNodeAlloc, 0x28, 0x8);
            if (!pNewNode) {
                handleContainerAllocFailure();
                return nullptr;
            }
            *reinterpret_cast<uint8_t**>(pNewNode + 0x0) = pOldFirst ? pOldFirst : pNewNode;
            *reinterpret_cast<uint8_t**>(pNewNode + 0x8) = pOldFirst ? pOldPrev : pNewNode;
            *reinterpret_cast<uint64_t*>(pNewNode + 0x10) = key;
            *reinterpret_cast<uint64_t*>(pNewNode + 0x18) = 0;
            *reinterpret_cast<uint64_t*>(pNewNode + 0x20) = 0;

            uint64_t count = *reinterpret_cast<uint64_t*>(pMgr + 0x28);
            if (count == 0x666666666666665ULL) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            *reinterpret_cast<uint64_t*>(pMgr + 0x28) = count + 1;
            *reinterpret_cast<uint8_t**>(pOldFirst + 0x8) = pNewNode;
            **reinterpret_cast<uint8_t***>(pNewNode + 0x8) = pNewNode;

            FfxIter24 plistIter = {
                0ULL,
                0ULL,
                *reinterpret_cast<void**>(*reinterpret_cast<void**>(pMgr + 0x20))
            };
            FfxInsertResult32 insRes = {};
            sub_2309e80(&insRes, pMgr + 0x18, pWhere, bucket, plistIter);
            pSlot = reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(insRes.pNode) + 0x18);

            uint8_t* pSlotVec = pMgr + 0xa8;
            uint64_t vecCap = (*reinterpret_cast<uint64_t*>(pMgr + 0xc0) - *reinterpret_cast<uint64_t*>(pMgr + 0xb0)) >> 3;
            uint64_t vecSize = (*reinterpret_cast<uint64_t*>(pMgr + 0xb8) - *reinterpret_cast<uint64_t*>(pMgr + 0xb0)) >> 3;
            if (vecCap == vecSize) {
                if (!sub_23094c0(pSlotVec, vecCap + 0x400ULL)) {
                    sub_ee5de0(6);
                    return nullptr;
                }
            }
            uint64_t** pEnd = *reinterpret_cast<uint64_t***>(pMgr + 0xb8);
            uint64_t** pCap = *reinterpret_cast<uint64_t***>(pMgr + 0xc0);
            if (pEnd == pCap) {
                uint64_t curLen = static_cast<uint64_t>(pEnd - *reinterpret_cast<uint64_t***>(pMgr + 0xb0));
                uint64_t half = curLen >> 1;
                uint64_t grown = (0x1fffffffffffffffULL - half >= curLen) ? (curLen + half) : 0ULL;
                if (grown < curLen + 1ULL) {
                    grown = curLen + 1ULL;
                }
                sub_2309d80(pSlotVec, grown);
                pEnd = *reinterpret_cast<uint64_t***>(pMgr + 0xb8);
            }
            if (pEnd) {
                *pEnd = pSlot;
            }
            *reinterpret_cast<uint64_t***>(pMgr + 0xb8) = pEnd + 1;
        }

        pChainNode[0] = reinterpret_cast<uint64_t>(pItem);
        pChainNode[1] = reinterpret_cast<uint64_t>(pHeader);
        pChainNode[2] = 0;
        if (pSlot[0] != 0) {
            *reinterpret_cast<uint64_t*>(pSlot[1] + 0x10) = reinterpret_cast<uint64_t>(pChainNode);
        } else {
            pSlot[0] = reinterpret_cast<uint64_t>(pChainNode);
        }
        pSlot[1] = reinterpret_cast<uint64_t>(pChainNode);
        return reinterpret_cast<void*>(cur + (pad | 0x10ULL));
    }

    struct DrawPair16 {
        void* pItem;
        void* pHeader;
    };
    DrawPair16 localPair = { pItem, pHeader };
    uint8_t* pQueueOwner = *reinterpret_cast<uint8_t**>(self + 0x90);
    uint8_t* pQueueVec = pQueueOwner + 0x2ad0;
    uint64_t qCap = (*reinterpret_cast<uint64_t*>(pQueueOwner + 0x2ae8) - *reinterpret_cast<uint64_t*>(pQueueOwner + 0x2ad8)) >> 4;
    uint64_t qSize = (*reinterpret_cast<uint64_t*>(pQueueOwner + 0x2ae0) - *reinterpret_cast<uint64_t*>(pQueueOwner + 0x2ad8)) >> 4;
    if (qCap == qSize) {
        if (!sub_2309560(pQueueVec, qCap + 0x400ULL)) {
            sub_ee5de0(6);
            return nullptr;
        }
    }
    DrawPair16* pQEnd = *reinterpret_cast<DrawPair16**>(pQueueOwner + 0x2ae0);
    DrawPair16* pQCap = *reinterpret_cast<DrawPair16**>(pQueueOwner + 0x2ae8);
    if (pQEnd == pQCap) {
        sub_23096e0(pQueueVec);
        pQEnd = *reinterpret_cast<DrawPair16**>(pQueueOwner + 0x2ae0);
    }
    if (pQEnd) {
        *pQEnd = localPair;
    }
    *reinterpret_cast<DrawPair16**>(pQueueOwner + 0x2ae0) = pQEnd + 1;
    return reinterpret_cast<void*>(cur + (pad | 0x10ULL));
}

// ============================================================================
// 2. sub_23088f0 (0x23088f0, 1 B)
// ============================================================================
extern "C" void sub_23088f0() {
    return;
}

// ============================================================================
// 3. sub_23087a0 (0x23087a0, 328 B)
//    Non-deleting destructor for particle linear command buffer manager.
// ============================================================================
extern "C" void sub_23087a0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53b91b0ULL);

    void** pVec1Begin = *reinterpret_cast<void***>(self + 0x40);
    void** pVec1End = *reinterpret_cast<void***>(self + 0x48);
    for (void** pIt = pVec1Begin; pIt != pVec1End; ++pIt) {
        void* pElem = *pIt;
        if (pElem) {
            void* pAlloc = *reinterpret_cast<void**>(self + 0xa0);
            void* vtbl = *reinterpret_cast<void**>(pAlloc);
            auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
            fnFree(pAlloc, pElem);
        }
    }
    *reinterpret_cast<void***>(self + 0x48) = pVec1Begin;

    void** pVec2Begin = *reinterpret_cast<void***>(self + 0x68);
    void** pVec2End = *reinterpret_cast<void***>(self + 0x70);
    for (void** pIt = pVec2Begin; pIt != pVec2End; ++pIt) {
        void* pElem = *pIt;
        if (pElem) {
            void* pAlloc = *reinterpret_cast<void**>(self + 0xa0);
            void* vtbl = *reinterpret_cast<void**>(pAlloc);
            auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
            fnFree(pAlloc, pElem);
        }
    }
    *reinterpret_cast<void***>(self + 0x70) = pVec2Begin;
    *reinterpret_cast<uint64_t*>(self + 0x30) = 0;

    if (pVec2Begin) {
        void* pAlloc2 = *reinterpret_cast<void**>(self + 0x80);
        void* vtbl2 = *reinterpret_cast<void**>(pAlloc2);
        auto fnFree2 = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl2) + 0x70);
        fnFree2(pAlloc2, pVec2Begin);
        *reinterpret_cast<uint64_t*>(self + 0x78) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x70) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x68) = 0;
    }

    pVec1Begin = *reinterpret_cast<void***>(self + 0x40);
    if (pVec1Begin) {
        void* pAlloc1 = *reinterpret_cast<void**>(self + 0x58);
        void* vtbl1 = *reinterpret_cast<void**>(pAlloc1);
        auto fnFree1 = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl1) + 0x70);
        fnFree1(pAlloc1, pVec1Begin);
        *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }
}

// ============================================================================
// 4. sub_2308780 (0x2308780, 28 B)
//    Deleting destructor for particle linear command buffer manager.
// ============================================================================
extern "C" void sub_2308780(void* pSelf) {
    sub_23087a0(pSelf);
    sub_2bbe588(pSelf);
}

// ============================================================================
// 5. sub_2308560 (0x2308560, 539 B)
//    Constructor for particle linear command buffer manager (vtable 0x53b91b0).
// ============================================================================
extern "C" void sub_2308560(
    void* pSelf,
    void* pDrawQueue,
    void* pBucketMgr,
    void* pField08,
    void* pField28,
    const void* pDescR9,
    void* pAllocator,
    void* pCtxA8,
    uint32_t blockSize) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    const uint8_t* desc = static_cast<const uint8_t*>(pDescR9);

    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53b91b0ULL);
    *reinterpret_cast<void**>(self + 0x8) = pField08;
    *reinterpret_cast<uint32_t*>(self + 0x20) = *reinterpret_cast<const uint32_t*>(desc + 0x10);
    *reinterpret_cast<uint64_t*>(self + 0x10) = *reinterpret_cast<const uint64_t*>(desc + 0x0);
    *reinterpret_cast<uint64_t*>(self + 0x18) = *reinterpret_cast<const uint64_t*>(desc + 0x8);

    validateContainerHeap(pAllocator);
    *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    *reinterpret_cast<void**>(self + 0x58) = pAllocator;

    validateContainerHeap(pAllocator);
    *reinterpret_cast<uint64_t*>(self + 0x78) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x70) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x68) = 0;
    *reinterpret_cast<void**>(self + 0x80) = pAllocator;

    *reinterpret_cast<void**>(self + 0x90) = pDrawQueue;
    *reinterpret_cast<void**>(self + 0x98) = pBucketMgr;
    *reinterpret_cast<void**>(self + 0xa8) = pCtxA8;
    *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb8) = 0;
    *reinterpret_cast<void**>(self + 0x28) = pField28;
    *reinterpret_cast<void**>(self + 0xa0) = pAllocator;
    self[0xb0] = 0;
    *reinterpret_cast<uint32_t*>(self + 0x88) = blockSize;

    void* vtbl = *reinterpret_cast<void**>(pAllocator);
    auto fnGetAvail = *reinterpret_cast<uint64_t (**)(void*)>(static_cast<uint8_t*>(vtbl) + 0x38);
    if (fnGetAvail(pAllocator) > static_cast<uint64_t>(blockSize + 0x80000u)) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0xa0);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnAlloc = *reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58);
        uint64_t rawBuf = reinterpret_cast<uint64_t>(fnAlloc(pAlloc, blockSize, 0x10));

        pAlloc = *reinterpret_cast<void**>(self + 0xa0);
        allocVtbl = *reinterpret_cast<void**>(pAlloc);
        fnAlloc = *reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58);
        uint64_t* newBlock = static_cast<uint64_t*>(fnAlloc(pAlloc, 0x20, 0x8));
        if (rawBuf && newBlock) {
            uint64_t blkEnd = rawBuf + *reinterpret_cast<uint32_t*>(self + 0x88);
            newBlock[0] = rawBuf;
            newBlock[1] = blkEnd;
            newBlock[2] = rawBuf;
            newBlock[3] = blkEnd;
            uint64_t blockPtrVal = reinterpret_cast<uint64_t>(newBlock);
            *reinterpret_cast<uint64_t**>(self + 0x30) = newBlock;
            sub_2305560(self + 0x38, &blockPtrVal);
            sub_29416c0(self + 0x60, &rawBuf);
        } else {
            auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70);
            if (newBlock) {
                fnFree(pAlloc, newBlock);
            }
            if (rawBuf) {
                fnFree(pAlloc, reinterpret_cast<void*>(rawBuf));
            }
        }
    }

    if (*reinterpret_cast<uint64_t*>(self + 0x30) == 0) {
        sub_ee5de0(6);
    }
}

// ============================================================================
// 6. sub_23084f0 (0x23084f0, 102 B)
//    Dispatches draw items in [pRange[0], pRange[1]) from vector at +0x2ad8.
// ============================================================================
extern "C" void sub_23084f0(void* pSelf, const int32_t* pRange) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    int32_t cur = pRange[0];
    int32_t end = pRange[1];
    if (cur < end) {
        uint8_t* pCtx = self + 0x2110;
        int64_t offset = (static_cast<int64_t>(cur) << 4) | 8LL;
        for (; cur < end; ++cur, offset += 0x10LL) {
            uint8_t* pBase = *reinterpret_cast<uint8_t**>(self + 0x2ad8);
            void* pItem = *reinterpret_cast<void**>(pBase + offset - 8);
            void* pData = *reinterpret_cast<void**>(pBase + offset);
            void* vtbl = *reinterpret_cast<void**>(pItem);
            auto fnDraw = *reinterpret_cast<void (**)(void*, void*, void*, uint32_t)>(
                static_cast<uint8_t*>(vtbl) + 0x10);
            fnDraw(pItem, pCtx, pData, 0);
        }
    }
}

// ============================================================================
// 7. sub_2308440 (0x2308440, 168 B)
//    Non-deleting destructor for particle draw queue (vtable 0x5362530).
// ============================================================================
extern "C" void sub_2308440(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362530ULL);

    void* pVecBuf = *reinterpret_cast<void**>(self + 0x2ad8);
    if (pVecBuf) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x2af0);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pVecBuf);
        *reinterpret_cast<uint64_t*>(self + 0x2ae8) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x2ae0) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x2ad8) = 0;
    }

    sub_f16e70(self + 0x2110);
    *reinterpret_cast<uint64_t*>(self + 0x28) = GX_VMA(0x52e74e0ULL);
    if (*reinterpret_cast<uint64_t*>(self + 0x20f0) >= 8ULL) {
        void* pStrBuf = *reinterpret_cast<void**>(self + 0x20d8);
        void* pStrAlloc = *reinterpret_cast<void**>(self + 0x20f8);
        void* vtbl = *reinterpret_cast<void**>(pStrAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pStrAlloc, pStrBuf);
    }
    *reinterpret_cast<uint64_t*>(self + 0x20f0) = 7ULL;
    *reinterpret_cast<uint64_t*>(self + 0x20e8) = 0ULL;
    *reinterpret_cast<uint16_t*>(self + 0x20d8) = 0;
}

// ============================================================================
// 8. sub_23083b0 (0x23083b0, 144 B)
//    Deleting destructor for particle draw queue (vtable 0x5362530).
// ============================================================================
extern "C" void sub_23083b0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362530ULL);

    void* pVecBuf = *reinterpret_cast<void**>(self + 0x2ad8);
    if (pVecBuf) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x2af0);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pVecBuf);
        *reinterpret_cast<uint64_t*>(self + 0x2ae8) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x2ae0) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x2ad8) = 0;
    }

    sub_f16e70(self + 0x2110);
    *reinterpret_cast<uint64_t*>(self + 0x28) = GX_VMA(0x52e74e0ULL);
    if (*reinterpret_cast<uint64_t*>(self + 0x20f0) >= 8ULL) {
        void* pStrBuf = *reinterpret_cast<void**>(self + 0x20d8);
        void* pStrAlloc = *reinterpret_cast<void**>(self + 0x20f8);
        void* vtbl = *reinterpret_cast<void**>(pStrAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pStrAlloc, pStrBuf);
    }
    sub_2bbe588(self);
}

// ============================================================================
// 9. sub_23081d0 (0x23081d0, 477 B)
//    Constructor for particle draw queue (vtable 0x5362530).
// ============================================================================
extern "C" void sub_23081d0(void* pSelf, void* pAllocator, void* pField08, void* pField10) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362530ULL);
    *reinterpret_cast<void**>(self + 0x8) = pField08;
    *reinterpret_cast<void**>(self + 0x10) = pField10;

    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pSub68 = *reinterpret_cast<uint8_t**>(pGlobal + 0x68);
    uint8_t* pDeref = *reinterpret_cast<uint8_t**>(pSub68);
    void* pDev = *reinterpret_cast<void**>(pDeref + 0x8);

    *reinterpret_cast<void**>(self + 0x20) = pDev;
    *reinterpret_cast<uint64_t*>(self + 0x28) = GX_VMA(0x52e74e0ULL);
    *reinterpret_cast<void**>(self + 0x30) = pDev;

    sub_2bbe688(self + 0x1750, 0, 0x794);
    *reinterpret_cast<uint64_t*>(self + 0x1ef0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x1ee8) = 0;
    self[0x20bb] = 1;
    self[0x20bc] = 1;
    *reinterpret_cast<uint32_t*>(self + 0x20c0) = 0;
    self[0x20c4] = 0;
    *reinterpret_cast<uint32_t*>(self + 0x20c8) = 0;

    void** ppDefHeap = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* pDefHeap = *ppDefHeap;
    if (!pDefHeap) {
        pDefHeap = sub_2ba2b10();
        *ppDefHeap = pDefHeap;
    }
    validateContainerHeap(pDefHeap);
    *reinterpret_cast<void**>(self + 0x20f8) = pDefHeap;
    *reinterpret_cast<uint64_t*>(self + 0x20f0) = 7ULL;
    *reinterpret_cast<uint64_t*>(self + 0x20e8) = 0ULL;
    *reinterpret_cast<uint16_t*>(self + 0x20d8) = 0;
    self[0x2100] = 1;
    *reinterpret_cast<uint32_t*>(self + 0x2108) = 0;

    sub_f16a90(
        self + 0x2110,
        self + 0x28,
        *reinterpret_cast<void**>(self + 0x20),
        *reinterpret_cast<void**>(self + 0x8),
        *reinterpret_cast<void**>(self + 0x10),
        0);

    validateContainerHeap(pAllocator);
    *reinterpret_cast<uint64_t*>(self + 0x2ae8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x2ae0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x2ad8) = 0;
    *reinterpret_cast<void**>(self + 0x2af0) = pAllocator;
    sub_2309560(self + 0x2ad0, 0x400ULL);
    *reinterpret_cast<void**>(self + 0x18) = pAllocator;
}

// ============================================================================
// 10. sub_2308150 (0x2308150, 114 B)
//     Dispatches shader-bucketed draw chains in [pRange[0], pRange[1]).
// ============================================================================
extern "C" void sub_2308150(void* pSelf, const int32_t* pRange) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    int64_t cur = pRange[0];
    int32_t end = pRange[1];
    if (static_cast<int32_t>(cur) < end) {
        uint8_t* pCtx = self + 0x21c0;
        for (; static_cast<int32_t>(cur) < end; ++cur) {
            uint64_t** ppSlots = *reinterpret_cast<uint64_t***>(self + 0xb0);
            uint64_t* pSlot = ppSlots[cur];
            uint64_t* pNode = reinterpret_cast<uint64_t*>(pSlot[0]);
            while (pNode) {
                void* pItem = reinterpret_cast<void*>(pNode[0]);
                void* pData = reinterpret_cast<void*>(pNode[1]);
                void* vtbl = *reinterpret_cast<void**>(pItem);
                auto fnDraw = *reinterpret_cast<void (**)(void*, void*, void*, uint32_t)>(
                    static_cast<uint8_t*>(vtbl) + 0x10);
                fnDraw(pItem, pCtx, pData, 0);
                pNode = reinterpret_cast<uint64_t*>(pNode[2]);
            }
        }
    }
}

// ============================================================================
// 11. sub_2307ff0 (0x2307ff0, 351 B)
//     Non-deleting destructor for bucketed particle draw pass (vtable 0x5362500).
// ============================================================================
extern "C" void sub_2307ff0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362500ULL);

    void* pArenaBuf = *reinterpret_cast<void**>(self + 0x98);
    if (pArenaBuf) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0xa0);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pArenaBuf);
        *reinterpret_cast<uint64_t*>(self + 0x98) = 0;
    }
    *reinterpret_cast<uint64_t*>(self + 0x98) = 0;

    sub_f16e70(self + 0x21c0);
    *reinterpret_cast<uint64_t*>(self + 0xd8) = GX_VMA(0x52e74e0ULL);
    if (*reinterpret_cast<uint64_t*>(self + 0x21a0) >= 8ULL) {
        void* pStrBuf = *reinterpret_cast<void**>(self + 0x2188);
        void* pStrAlloc = *reinterpret_cast<void**>(self + 0x21a8);
        void* vtbl = *reinterpret_cast<void**>(pStrAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pStrAlloc, pStrBuf);
    }
    *reinterpret_cast<uint64_t*>(self + 0x21a0) = 7ULL;
    *reinterpret_cast<uint64_t*>(self + 0x2198) = 0ULL;
    *reinterpret_cast<uint16_t*>(self + 0x2188) = 0;

    void* pSlotVecBuf = *reinterpret_cast<void**>(self + 0xb0);
    if (pSlotVecBuf) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0xc8);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pSlotVecBuf);
        *reinterpret_cast<uint64_t*>(self + 0xc0) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb8) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb0) = 0;
    }

    void* pBucketVecBuf = *reinterpret_cast<void**>(self + 0x40);
    if (pBucketVecBuf) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x58);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pBucketVecBuf);
        *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x20);
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pHead);
    *reinterpret_cast<uint8_t**>(pHead) = pHead;
    pHead = *reinterpret_cast<uint8_t**>(self + 0x20);
    *reinterpret_cast<uint8_t**>(pHead + 0x8) = pHead;
    *reinterpret_cast<uint64_t*>(self + 0x28) = 0;

    while (pCur != *reinterpret_cast<uint8_t**>(self + 0x20)) {
        uint8_t* pNext = *reinterpret_cast<uint8_t**>(pCur);
        void* pAlloc = *reinterpret_cast<void**>(self + 0x30);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pCur);
        pCur = pNext;
    }

    void* pAlloc = *reinterpret_cast<void**>(self + 0x30);
    void* vtbl = *reinterpret_cast<void**>(pAlloc);
    auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
    fnFree(pAlloc, pCur);
}

// ============================================================================
// 12. sub_2307fd0 (0x2307fd0, 28 B)
//     Deleting destructor for bucketed particle draw pass (vtable 0x5362500).
// ============================================================================
extern "C" void sub_2307fd0(void* pSelf) {
    sub_2307ff0(pSelf);
    sub_2bbe588(pSelf);
}

// ============================================================================
// 13. sub_23078a0 (0x23078a0, 1,825 B)
//     Constructor for bucketed particle draw pass (vtable 0x5362500).
// ============================================================================
extern "C" void sub_23078a0(void* pSelf, void* pAllocator, void* pField08, void* pField10) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362500ULL);
    *reinterpret_cast<void**>(self + 0x8) = pField08;
    *reinterpret_cast<void**>(self + 0x10) = pField10;

    void** ppDefHeap = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* pDefHeap = *ppDefHeap;
    if (!pDefHeap) {
        pDefHeap = sub_2ba2b10();
        *ppDefHeap = pDefHeap;
    }
    validateContainerHeap(pDefHeap);
    void* pHeapArg = pDefHeap;
    sub_2309350(self + 0x18, &pHeapArg);

    *reinterpret_cast<uint64_t*>(self + 0x90) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x88) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x80) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x78) = 0;

    validateContainerHeap(pAllocator);
    *reinterpret_cast<uint64_t*>(self + 0xc0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb0) = 0;
    *reinterpret_cast<void**>(self + 0xc8) = pAllocator;

    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pSub68 = *reinterpret_cast<uint8_t**>(pGlobal + 0x68);
    uint8_t* pDeref = *reinterpret_cast<uint8_t**>(pSub68);
    void* pDev = *reinterpret_cast<void**>(pDeref + 0x8);

    *reinterpret_cast<void**>(self + 0xd0) = pDev;
    *reinterpret_cast<uint64_t*>(self + 0xd8) = GX_VMA(0x52e74e0ULL);
    *reinterpret_cast<void**>(self + 0xe0) = pDev;

    sub_2bbe688(self + 0x1800, 0, 0x794);
    *reinterpret_cast<uint64_t*>(self + 0x1fa0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x1f98) = 0;
    self[0x216b] = 1;
    self[0x216c] = 1;
    *reinterpret_cast<uint32_t*>(self + 0x2170) = 0;
    self[0x2174] = 0;
    *reinterpret_cast<uint32_t*>(self + 0x2178) = 0;

    pDefHeap = *ppDefHeap;
    if (!pDefHeap) {
        pDefHeap = sub_2ba2b10();
        *ppDefHeap = pDefHeap;
    }
    validateContainerHeap(pDefHeap);
    *reinterpret_cast<void**>(self + 0x21a8) = pDefHeap;
    *reinterpret_cast<uint64_t*>(self + 0x21a0) = 7ULL;
    *reinterpret_cast<uint64_t*>(self + 0x2198) = 0ULL;
    *reinterpret_cast<uint16_t*>(self + 0x2188) = 0;
    self[0x21b0] = 1;
    *reinterpret_cast<uint32_t*>(self + 0x21b8) = 0;

    sub_f16a90(
        self + 0x21c0,
        self + 0xd8,
        *reinterpret_cast<void**>(self + 0xd0),
        *reinterpret_cast<void**>(self + 0x8),
        *reinterpret_cast<void**>(self + 0x10),
        0);

    *reinterpret_cast<void**>(self + 0xa0) = pAllocator;
    void* allocVtbl = *reinterpret_cast<void**>(pAllocator);
    auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58);
    uint8_t* pArena = fnAlloc(pAllocator, 0x40000ULL, 0x10ULL);
    *reinterpret_cast<uint8_t**>(self + 0x98) = pArena;
    if (!pArena) {
        sub_ee5de0(6);
        return;
    }

    if (*reinterpret_cast<uint64_t*>(self + 0x78) == 0) {
        *reinterpret_cast<uint8_t**>(self + 0x88) = pArena;
        *reinterpret_cast<uint8_t**>(self + 0x78) = pArena;
        uint8_t* pArenaEnd = pArena + 0x40000;
        *reinterpret_cast<uint8_t**>(self + 0x90) = pArenaEnd;
        *reinterpret_cast<uint8_t**>(self + 0x80) = pArenaEnd;
    }

    validateContainerHeap(pAllocator);
    sub_23094c0(self + 0xa8, 0x400ULL);
}

// ============================================================================
// 14. sub_23077a0 (0x23077a0, 251 B)
//     Reallocates an 8-byte pointer vector buffer to capacity newCap.
// ============================================================================
extern "C" void sub_23077a0(void* pVec, uint64_t newCap) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    void* pAlloc = *reinterpret_cast<void**>(vec + 0x20);
    void* vtbl = *reinterpret_cast<void**>(pAlloc);
    auto fnAlloc = *reinterpret_cast<uint64_t* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58);
    uint64_t* pNewBuf = fnAlloc(pAlloc, newCap * 8ULL, 0x8ULL);
    if (!pNewBuf) {
        handleContainerAllocFailure();
        return;
    }

    uint64_t* pBegin = *reinterpret_cast<uint64_t**>(vec + 0x8);
    uint64_t* pEnd = *reinterpret_cast<uint64_t**>(vec + 0x10);
    if (pBegin != pEnd) {
        uint64_t* pDst = pNewBuf;
        for (uint64_t* pCur = pBegin; pCur != pEnd; ++pCur, ++pDst) {
            if (pDst) {
                *pDst = *pCur;
            }
        }
        pBegin = *reinterpret_cast<uint64_t**>(vec + 0x8);
        pEnd = *reinterpret_cast<uint64_t**>(vec + 0x10);
    }
    uint64_t oldCount = static_cast<uint64_t>(pEnd - pBegin);
    if (pBegin) {
        pAlloc = *reinterpret_cast<void**>(vec + 0x20);
        vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnFree = *reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70);
        fnFree(pAlloc, pBegin);
    }

    *reinterpret_cast<uint64_t**>(vec + 0x18) = pNewBuf + newCap;
    *reinterpret_cast<uint64_t**>(vec + 0x10) = pNewBuf + oldCount;
    *reinterpret_cast<uint64_t**>(vec + 0x8) = pNewBuf;
}

// ============================================================================
// 15. sub_2307390 (0x2307390, 1,039 B)
//     Splices or inserts plistIter.pNode with key *pKey into the 64-bit
//     Park-Miller hash table at pHash, rehashing via sub_2306bd0 if needed.
// ============================================================================
extern "C" FfxInsertResult32* sub_2307390(
    FfxInsertResult32* pOutPair,
    void* pHash,
    const uint64_t* pKey,
    FfxIter24 plistIter) {
    uint8_t* hash = static_cast<uint8_t*>(pHash);
    uint64_t key = *pKey;
    uint64_t hashVal = computeParkMillerHash64(key, sub_2bbfd68);
    uint64_t mask = *reinterpret_cast<uint64_t*>(hash + 0x48);
    uint64_t bucket = hashVal & mask;
    if (bucket >= *reinterpret_cast<uint64_t*>(hash + 0x50)) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(hash + 0x8);
    uint8_t* pBuckets = *reinterpret_cast<uint8_t**>(hash + 0x28);
    uint8_t* pFirst = *reinterpret_cast<uint8_t**>(pBuckets + bucket * 48 + 0x10);
    uint8_t* pScan = pHead;
    if (pFirst != pHead) {
        pScan = **reinterpret_cast<uint8_t***>(pBuckets + ((bucket * 2) | 1ULL) * 24 + 0x10);
    }

    uint8_t* pWhere = pFirst;
    while (pScan != pFirst) {
        pScan = *reinterpret_cast<uint8_t**>(pScan + 0x8);
        key = *pKey;
        uint64_t scanKey = *reinterpret_cast<uint64_t*>(pScan + 0x10);
        if (key >= scanKey) {
            if (scanKey >= key) {
                uint8_t* pCandidate = static_cast<uint8_t*>(plistIter.pNode);
                if (pCandidate != pHead) {
                    uint8_t* pNext = *reinterpret_cast<uint8_t**>(pCandidate);
                    uint8_t* pPrev = *reinterpret_cast<uint8_t**>(pCandidate + 0x8);
                    *reinterpret_cast<uint8_t**>(pPrev) = pNext;
                    *reinterpret_cast<uint8_t**>(pNext + 0x8) = pPrev;

                    void* pAlloc = *reinterpret_cast<void**>(hash + 0x18);
                    void* vtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(
                        pAlloc, pCandidate);
                    *reinterpret_cast<uint64_t*>(hash + 0x10) -= 1ULL;
                }
                pOutPair->w0 = 0;
                pOutPair->w1 = 0;
                pOutPair->pNode = pScan;
                pOutPair->inserted = 0;
                return pOutPair;
            }
            pWhere = *reinterpret_cast<uint8_t**>(pScan);
            break;
        }
    }

    uint8_t* pNewNode = static_cast<uint8_t*>(plistIter.pNode);
    uint8_t* pNodeNext = *reinterpret_cast<uint8_t**>(pNewNode);
    if (pWhere != pNodeNext) {
        uint8_t* pNewPrev = *reinterpret_cast<uint8_t**>(pNewNode + 0x8);
        *reinterpret_cast<uint8_t**>(pNewPrev) = pNodeNext;
        uint8_t* pNextPrev = *reinterpret_cast<uint8_t**>(pNodeNext + 0x8);
        *reinterpret_cast<uint8_t**>(pNextPrev) = pWhere;
        uint8_t* pWherePrev = *reinterpret_cast<uint8_t**>(pWhere + 0x8);
        *reinterpret_cast<uint8_t**>(pWherePrev) = pNewNode;

        *reinterpret_cast<uint8_t**>(pWhere + 0x8) = pNextPrev;
        *reinterpret_cast<uint8_t**>(pNodeNext + 0x8) = pNewPrev;
        *reinterpret_cast<uint8_t**>(pNewNode + 0x8) = pWherePrev;

        pHead = *reinterpret_cast<uint8_t**>(hash + 0x8);
        pBuckets = *reinterpret_cast<uint8_t**>(hash + 0x28);
        pFirst = *reinterpret_cast<uint8_t**>(pBuckets + (bucket * 2) * 24 + 0x10);
    }

    uint64_t bIdxEven = bucket * 2;
    uint8_t* pBucketEven = pBuckets + bIdxEven * 24;
    if (pFirst == pHead) {
        *reinterpret_cast<uint64_t*>(pBucketEven + 0x0) = plistIter.w0;
        *reinterpret_cast<uint64_t*>(pBucketEven + 0x8) = plistIter.w1;
        *reinterpret_cast<uint8_t**>(pBucketEven + 0x10) = pNewNode;

        uint8_t* pBucketOdd = *reinterpret_cast<uint8_t**>(hash + 0x28) + (bIdxEven | 1ULL) * 24;
        *reinterpret_cast<uint64_t*>(pBucketOdd + 0x0) = plistIter.w0;
        *reinterpret_cast<uint64_t*>(pBucketOdd + 0x8) = plistIter.w1;
        *reinterpret_cast<uint8_t**>(pBucketOdd + 0x10) = pNewNode;
    } else if (pFirst == pWhere) {
        *reinterpret_cast<uint64_t*>(pBucketEven + 0x0) = plistIter.w0;
        *reinterpret_cast<uint64_t*>(pBucketEven + 0x8) = plistIter.w1;
        *reinterpret_cast<uint8_t**>(pBucketEven + 0x10) = pNewNode;
    } else {
        uint8_t* pBucketOdd = pBuckets + (bIdxEven | 1ULL) * 24;
        uint8_t* pOddNode = **reinterpret_cast<uint8_t***>(pBucketOdd + 0x10);
        *reinterpret_cast<uint8_t**>(pBucketOdd + 0x10) = pOddNode;
        if (pOddNode != pNewNode) {
            *reinterpret_cast<uint8_t**>(pBucketOdd + 0x10) = *reinterpret_cast<uint8_t**>(pOddNode + 0x8);
        }
    }

    float fSize = 0.0f;
    float fBuckets = 0.0f;
    u64ToFloatBitsHelper(*reinterpret_cast<uint64_t*>(hash + 0x10), &fSize);
    uint64_t curMaxBuckets = *reinterpret_cast<uint64_t*>(hash + 0x50);
    u64ToFloatBitsHelper(curMaxBuckets, &fBuckets);

    if ((fSize / fBuckets) > *reinterpret_cast<float*>(hash + 0x58)) {
        uint64_t newBuckets = curMaxBuckets;
        if (newBuckets <= 0x1ffULL) {
            newBuckets <<= 3;
        } else if (newBuckets < 0x555555555555555ULL) {
            newBuckets <<= 1;
        }
        *reinterpret_cast<uint64_t*>(hash + 0x30) = *reinterpret_cast<uint64_t*>(hash + 0x28);
        FfxIter24 fillVal = { 0ULL, 0ULL, *reinterpret_cast<void**>(hash + 0x8) };
        FfxIter24 whereIter = { 0ULL, 0ULL, *reinterpret_cast<void**>(hash + 0x28) };
        FfxIter24 outIter = {};
        sub_2306bd0(&outIter, hash + 0x20, newBuckets * 2ULL, &fillVal, whereIter);

        *reinterpret_cast<uint64_t*>(hash + 0x48) = newBuckets - 1ULL;
        *reinterpret_cast<uint64_t*>(hash + 0x50) = newBuckets;

        uint8_t* pListHead = *reinterpret_cast<uint8_t**>(hash + 0x8);
        uint8_t* pRehashNode = *reinterpret_cast<uint8_t**>(pListHead);
        if (pRehashNode != pListHead) {
            uint8_t* pLastNode = *reinterpret_cast<uint8_t**>(pListHead + 0x8);
            for (;;) {
                pRehashNode = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(hash + 0x8));
                FfxIter24 nodeIter = { 0ULL, 0ULL, pRehashNode };
                FfxInsertResult32 dummyRes = {};
                sub_2307390(
                    &dummyRes,
                    hash,
                    reinterpret_cast<const uint64_t*>(pRehashNode + 0x10),
                    nodeIter);
                if (pRehashNode == pLastNode) {
                    break;
                }
            }
        }
    }

    pOutPair->w0 = 0;
    pOutPair->w1 = 0;
    pOutPair->pNode = pNewNode;
    pOutPair->inserted = 1;
    return pOutPair;
}

// ============================================================================
// 16. sub_2307380 (0x2307380, 16 B)
// ============================================================================
extern "C" [[noreturn]] void sub_2307380() {
    sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
}

// ============================================================================
// 17. sub_2307280 (0x2307280, 251 B)
//     Reallocates an 8-byte pointer vector buffer to capacity newCap.
// ============================================================================
extern "C" void sub_2307280(void* pVec, uint64_t newCap) {
    sub_23077a0(pVec, newCap);
}

// ============================================================================
// 18. sub_23071a0 (0x23071a0, 218 B)
//     Appends *pVal to an 8-byte pointer vector, growing via sub_23077a0.
// ============================================================================
extern "C" void sub_23071a0(void* pVec, const uint64_t* pVal) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    uint64_t* pEnd = *reinterpret_cast<uint64_t**>(vec + 0x10);
    uint64_t* pBegin = *reinterpret_cast<uint64_t**>(vec + 0x8);
    uint64_t* pCap = *reinterpret_cast<uint64_t**>(vec + 0x18);

    if (pVal < pEnd && pBegin <= pVal) {
        int64_t idx = pVal - pBegin;
        if (pEnd == pCap) {
            uint64_t len = static_cast<uint64_t>(pEnd - pBegin);
            uint64_t half = len >> 1;
            uint64_t grown = (0x1fffffffffffffffULL - half >= len) ? (len + half) : 0ULL;
            if (grown < len + 1ULL) {
                grown = len + 1ULL;
            }
            sub_23077a0(vec, grown);
            pEnd = *reinterpret_cast<uint64_t**>(vec + 0x10);
        }
        if (pEnd) {
            uint64_t* pNewBegin = *reinterpret_cast<uint64_t**>(vec + 0x8);
            *pEnd = pNewBegin[idx];
        }
    } else {
        if (pEnd == pCap) {
            uint64_t len = static_cast<uint64_t>(pEnd - pBegin);
            uint64_t half = len >> 1;
            uint64_t grown = (0x1fffffffffffffffULL - half >= len) ? (len + half) : 0ULL;
            if (grown < len + 1ULL) {
                grown = len + 1ULL;
            }
            sub_23077a0(vec, grown);
            pEnd = *reinterpret_cast<uint64_t**>(vec + 0x10);
        }
        if (pEnd) {
            *pEnd = *pVal;
        }
    }
    *reinterpret_cast<uint64_t**>(vec + 0x10) = pEnd + 1;
}

// ============================================================================
// 19. sub_2307120 (0x2307120, 119 B)
//     Reserves at least 0x80 elements in an 8-byte pointer vector.
// ============================================================================
extern "C" void sub_2307120(void* pVec) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    setTlsAllocGuard(1);
    if (sub_2bbe8f8(getTlsJmpBuf()) == 0) {
        uint64_t cap = (*reinterpret_cast<uint64_t*>(vec + 0x18) - *reinterpret_cast<uint64_t*>(vec + 0x8)) >> 3;
        if (cap <= 0x7fULL) {
            sub_23077a0(vec, 0x80ULL);
        }
    }
    setTlsAllocGuard(0);
}

// ============================================================================
// 20. sub_2306bd0 (0x2306bd0, 1,352 B)
//     Inserts `count` copies of 24-byte iterator `*pVal` at `whereIter.pNode`
//     into the 24-byte bucket vector at `pVec`.
// ============================================================================
extern "C" FfxIter24* sub_2306bd0(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter) {
    return sub_2309830(pOutIter, pVec, count, pVal, whereIter);
}

// ============================================================================
// 21. sub_2306bc0 (0x2306bc0, 16 B)
// ============================================================================
extern "C" [[noreturn]] void sub_2306bc0() {
    sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
}

// ============================================================================
// 22. sub_2306ac0 (0x2306ac0, 251 B)
//     Reallocates an 8-byte pointer vector buffer to capacity newCap.
// ============================================================================
extern "C" void sub_2306ac0(void* pVec, uint64_t newCap) {
    sub_23077a0(pVec, newCap);
}

// ============================================================================
// 23. sub_2306700 (0x2306700, 959 B)
//     Allocates and initializes a 0x270-byte FFX draw action node and links it
//     into the doubly-linked list at pList before pWhereNode.
// ============================================================================
extern "C" void sub_2306700(void* pList, void* pWhereNode, const void* pSrcData) {
    uint8_t* list = static_cast<uint8_t*>(pList);
    uint8_t* whereNode = static_cast<uint8_t*>(pWhereNode);
    const uint8_t* src = static_cast<const uint8_t*>(pSrcData);

    uint8_t* pOldPrev = *reinterpret_cast<uint8_t**>(whereNode + 0x8);
    void* pAlloc = *reinterpret_cast<void**>(list + 0x18);
    void* vtbl = *reinterpret_cast<void**>(pAlloc);
    auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58);
    uint8_t* pNewNode = fnAlloc(pAlloc, 0x270ULL, 0x10ULL);
    if (!pNewNode) {
        handleContainerAllocFailure();
        return;
    }

    *reinterpret_cast<uint8_t**>(pNewNode + 0x0) = whereNode ? whereNode : pNewNode;
    *reinterpret_cast<uint8_t**>(pNewNode + 0x8) = whereNode ? pOldPrev : pNewNode;

    sub_28512d0(pNewNode + 0x10, src);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x30) = *reinterpret_cast<const uint64_t*>(src + 0x20);

    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x40), _mm_load_ps(reinterpret_cast<const float*>(src + 0x30)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x50), _mm_load_ps(reinterpret_cast<const float*>(src + 0x40)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x60), _mm_load_ps(reinterpret_cast<const float*>(src + 0x50)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x70), _mm_load_ps(reinterpret_cast<const float*>(src + 0x60)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x80), _mm_load_ps(reinterpret_cast<const float*>(src + 0x70)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x90), _mm_load_ps(reinterpret_cast<const float*>(src + 0x80)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0xa0), _mm_load_ps(reinterpret_cast<const float*>(src + 0x90)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0xb0), _mm_load_ps(reinterpret_cast<const float*>(src + 0xa0)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0xc0), _mm_load_ps(reinterpret_cast<const float*>(src + 0xb0)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0xd0), _mm_load_ps(reinterpret_cast<const float*>(src + 0xc0)));

    pNewNode[0xe8] = src[0xd8];
    *reinterpret_cast<uint64_t*>(pNewNode + 0xe0) = *reinterpret_cast<const uint64_t*>(src + 0xd0);

    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0xf0),  _mm_load_ps(reinterpret_cast<const float*>(src + 0xe0)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x100), _mm_load_ps(reinterpret_cast<const float*>(src + 0xf0)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x110), _mm_load_ps(reinterpret_cast<const float*>(src + 0x100)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x120), _mm_load_ps(reinterpret_cast<const float*>(src + 0x110)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x130), _mm_load_ps(reinterpret_cast<const float*>(src + 0x120)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x140), _mm_load_ps(reinterpret_cast<const float*>(src + 0x130)));

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
    const __m128 unitW   = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);

    _mm_store_ps(
        reinterpret_cast<float*>(pNewNode + 0x150),
        _mm_or_ps(_mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(src + 0x140))), unitW));
    _mm_store_ps(
        reinterpret_cast<float*>(pNewNode + 0x160),
        _mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(src + 0x150))));
    *reinterpret_cast<float*>(pNewNode + 0x170) = *reinterpret_cast<const float*>(src + 0x160);

    _mm_store_ps(
        reinterpret_cast<float*>(pNewNode + 0x180),
        _mm_or_ps(_mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(src + 0x170))), unitW));
    _mm_store_ps(
        reinterpret_cast<float*>(pNewNode + 0x190),
        _mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(src + 0x180))));
    *reinterpret_cast<float*>(pNewNode + 0x1a0) = *reinterpret_cast<const float*>(src + 0x190);

    pNewNode[0x1e0] = src[0x1d0];
    *reinterpret_cast<uint64_t*>(pNewNode + 0x1d8) = *reinterpret_cast<const uint64_t*>(src + 0x1c8);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x1d0) = *reinterpret_cast<const uint64_t*>(src + 0x1c0);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x1c8) = *reinterpret_cast<const uint64_t*>(src + 0x1b8);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x1c0) = *reinterpret_cast<const uint64_t*>(src + 0x1b0);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x1b8) = *reinterpret_cast<const uint64_t*>(src + 0x1a8);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x1b0) = *reinterpret_cast<const uint64_t*>(src + 0x1a0);

    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x1f0), _mm_load_ps(reinterpret_cast<const float*>(src + 0x1e0)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x200), _mm_load_ps(reinterpret_cast<const float*>(src + 0x1f0)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x210), _mm_load_ps(reinterpret_cast<const float*>(src + 0x200)));
    _mm_store_ps(reinterpret_cast<float*>(pNewNode + 0x220), _mm_load_ps(reinterpret_cast<const float*>(src + 0x210)));

    *reinterpret_cast<uint32_t*>(pNewNode + 0x260) = *reinterpret_cast<const uint32_t*>(src + 0x250);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x258) = *reinterpret_cast<const uint64_t*>(src + 0x248);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x250) = *reinterpret_cast<const uint64_t*>(src + 0x240);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x248) = *reinterpret_cast<const uint64_t*>(src + 0x238);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x240) = *reinterpret_cast<const uint64_t*>(src + 0x230);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x238) = *reinterpret_cast<const uint64_t*>(src + 0x228);
    *reinterpret_cast<uint64_t*>(pNewNode + 0x230) = *reinterpret_cast<const uint64_t*>(src + 0x220);

    uint64_t count = *reinterpret_cast<uint64_t*>(list + 0x10);
    if (count == 0x69069069069068ULL) {
        sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
    }
    *reinterpret_cast<uint64_t*>(list + 0x10) = count + 1ULL;
    *reinterpret_cast<uint8_t**>(whereNode + 0x8) = pNewNode;
    **reinterpret_cast<uint8_t***>(pNewNode + 0x8) = pNewNode;
}

// ============================================================================
// 24. sub_2306580 (0x2306580, 370 B)
//     Erases pEraseNode from the 64-bit Park-Miller hash table at pHash.
// ============================================================================
extern "C" FfxIter24* sub_2306580(FfxIter24* pOutIter, void* pHash, uint8_t* pEraseNode) {
    uint8_t* hash = static_cast<uint8_t*>(pHash);
    uint64_t key = *reinterpret_cast<uint64_t*>(pEraseNode + 0x10);
    uint64_t hashVal = computeParkMillerHash64(key, sub_2bbfd68);
    uint64_t mask = *reinterpret_cast<uint64_t*>(hash + 0x48);
    uint64_t bucket = hashVal & mask;
    if (bucket >= *reinterpret_cast<uint64_t*>(hash + 0x50)) {
        bucket += ~(mask >> 1);
    }

    uint64_t oddIdx = bucket * 2ULL + 1ULL;
    uint8_t* pBuckets = *reinterpret_cast<uint8_t**>(hash + 0x28);
    uint8_t* pEvenBucket = pBuckets + bucket * 48ULL;
    uint8_t* pOddBucket  = pBuckets + oddIdx * 24ULL;
    uint8_t* pEvenNode   = *reinterpret_cast<uint8_t**>(pEvenBucket + 0x10);
    uint8_t* pOddNode    = *reinterpret_cast<uint8_t**>(pOddBucket + 0x10);

    if (pOddNode == pEraseNode) {
        if (pEvenNode == pEraseNode) {
            uint8_t* pHead = *reinterpret_cast<uint8_t**>(hash + 0x8);
            *reinterpret_cast<uint64_t*>(pEvenBucket + 0x0) = 0;
            *reinterpret_cast<uint64_t*>(pEvenBucket + 0x8) = 0;
            *reinterpret_cast<uint8_t**>(pEvenBucket + 0x10) = pHead;

            pHead = *reinterpret_cast<uint8_t**>(hash + 0x8);
            pBuckets = *reinterpret_cast<uint8_t**>(hash + 0x28);
            pOddBucket = pBuckets + oddIdx * 24ULL;
            *reinterpret_cast<uint64_t*>(pOddBucket + 0x0) = 0;
            *reinterpret_cast<uint64_t*>(pOddBucket + 0x8) = 0;
            *reinterpret_cast<uint8_t**>(pOddBucket + 0x10) = pHead;
        } else {
            uint8_t* pPrev = *reinterpret_cast<uint8_t**>(pEraseNode + 0x8);
            *reinterpret_cast<uint64_t*>(pOddBucket + 0x0) = 0;
            *reinterpret_cast<uint64_t*>(pOddBucket + 0x8) = 0;
            *reinterpret_cast<uint8_t**>(pOddBucket + 0x10) = pPrev;
        }
    } else if (pEvenNode == pEraseNode) {
        uint8_t* pNext = *reinterpret_cast<uint8_t**>(pEraseNode + 0x0);
        *reinterpret_cast<uint64_t*>(pEvenBucket + 0x0) = 0;
        *reinterpret_cast<uint64_t*>(pEvenBucket + 0x8) = 0;
        *reinterpret_cast<uint8_t**>(pEvenBucket + 0x10) = pNext;
    }

    uint8_t* pNextNode = *reinterpret_cast<uint8_t**>(pEraseNode + 0x0);
    if (*reinterpret_cast<uint8_t**>(hash + 0x8) != pEraseNode) {
        uint8_t* pPrevNode = *reinterpret_cast<uint8_t**>(pEraseNode + 0x8);
        *reinterpret_cast<uint8_t**>(pPrevNode) = pNextNode;
        uint8_t* pAfterNode = *reinterpret_cast<uint8_t**>(pEraseNode + 0x0);
        *reinterpret_cast<uint8_t**>(pAfterNode + 0x8) = pPrevNode;

        void* pAlloc = *reinterpret_cast<void**>(hash + 0x18);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pEraseNode);
        *reinterpret_cast<uint64_t*>(hash + 0x10) -= 1ULL;
    }

    pOutIter->w0 = 0;
    pOutIter->w1 = 0;
    pOutIter->pNode = pNextNode;
    return pOutIter;
}

// ============================================================================
// 25. sub_2305f50 (0x2305f50, 1,572 B)
//     Inserts range [firstIter.pNode, lastIter.pNode) of 80-byte (0x50)
//     particle records into vector pVec at posIter.pNode.
// ============================================================================
namespace {

static inline void copyRecord80(uint8_t* dst, const uint8_t* src) {
    if (!dst) {
        return;
    }
    const __m128 maskXYZ = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
    const __m128 unitW   = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);

    *reinterpret_cast<uint64_t*>(dst + 0x0) = *reinterpret_cast<const uint64_t*>(src + 0x0);
    _mm_store_ps(
        reinterpret_cast<float*>(dst + 0x10),
        _mm_or_ps(_mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(src + 0x10))), unitW));
    _mm_store_ps(
        reinterpret_cast<float*>(dst + 0x20),
        _mm_or_ps(_mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(src + 0x20))), unitW));
    *reinterpret_cast<uint16_t*>(dst + 0x4c) = *reinterpret_cast<const uint16_t*>(src + 0x4c);
    *reinterpret_cast<uint32_t*>(dst + 0x48) = *reinterpret_cast<const uint32_t*>(src + 0x48);
    *reinterpret_cast<uint64_t*>(dst + 0x40) = *reinterpret_cast<const uint64_t*>(src + 0x40);
    _mm_store_ps(
        reinterpret_cast<float*>(dst + 0x30),
        _mm_load_ps(reinterpret_cast<const float*>(src + 0x30)));
}

} // namespace

extern "C" void sub_2305f50(
    void* pVec,
    FfxIter24 posIter,
    FfxIter24 firstIter,
    FfxIter24 lastIter) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    const uint8_t* pFirst = static_cast<const uint8_t*>(firstIter.pNode);
    const uint8_t* pLast  = static_cast<const uint8_t*>(lastIter.pNode);
    ptrdiff_t rangeBytes  = pLast - pFirst;
    if (rangeBytes == 0) {
        return;
    }

    uint64_t insertCount = static_cast<uint64_t>(rangeBytes >> 4) * 0xcccccccccccccccdULL;
    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
    uint8_t* pCap = *reinterpret_cast<uint8_t**>(vec + 0x18);
    uint64_t availCount = static_cast<uint64_t>((pCap - pEnd) >> 4) * 0xcccccccccccccccdULL;

    if (availCount < insertCount) {
        uint8_t* pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
        uint64_t curSize = static_cast<uint64_t>((pEnd - pBegin) >> 4) * 0xcccccccccccccccdULL;
        if ((0x333333333333333ULL - curSize) < insertCount) {
            sub_2307380();
        }
        uint64_t needed = curSize + insertCount;
        uint64_t curCap = static_cast<uint64_t>((pCap - pBegin) >> 4) * 0xcccccccccccccccdULL;
        uint64_t halfCap = curCap >> 1;
        uint64_t newCap = (0x333333333333333ULL - halfCap >= curCap) ? (curCap + halfCap) : 0ULL;
        if (newCap < needed) {
            newCap = needed;
        }

        void* pAlloc = *reinterpret_cast<void**>(vec + 0x20);
        void* vtbl = *reinterpret_cast<void**>(pAlloc);
        auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58);
        uint8_t* pNewBuf = fnAlloc(pAlloc, newCap * 80ULL, 0x10ULL);
        if (!pNewBuf) {
            handleContainerAllocFailure();
            return;
        }

        pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
        uint8_t* pPos = static_cast<uint8_t*>(posIter.pNode);
        uint8_t* pWrite = pNewBuf;
        for (uint8_t* pCur = pBegin; pCur != pPos; pCur += 0x50, pWrite += 0x50) {
            copyRecord80(pWrite, pCur);
        }
        for (const uint8_t* pCur = pFirst; pCur != pLast; pCur += 0x50, pWrite += 0x50) {
            copyRecord80(pWrite, pCur);
        }
        pEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
        for (uint8_t* pCur = pPos; pCur != pEnd; pCur += 0x50, pWrite += 0x50) {
            copyRecord80(pWrite, pCur);
        }

        pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
        pEnd   = *reinterpret_cast<uint8_t**>(vec + 0x10);
        uint64_t newSize = static_cast<uint64_t>((pEnd - pBegin) >> 4) * 0xcccccccccccccccdULL + insertCount;
        if (pBegin) {
            pAlloc = *reinterpret_cast<void**>(vec + 0x20);
            vtbl = *reinterpret_cast<void**>(pAlloc);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBegin);
        }
        *reinterpret_cast<uint8_t**>(vec + 0x18) = pNewBuf + newCap * 80ULL;
        *reinterpret_cast<uint8_t**>(vec + 0x10) = pNewBuf + newSize * 80ULL;
        *reinterpret_cast<uint8_t**>(vec + 0x8)  = pNewBuf;
        return;
    }

    for (ptrdiff_t off = 0; off != rangeBytes; off += 0x50) {
        copyRecord80(pEnd + off, pFirst + off);
    }

    uint8_t* pOldEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
    uint8_t* pPos    = static_cast<uint8_t*>(posIter.pNode);
    uint64_t insBytes = insertCount * 80ULL;
    if (pPos != pOldEnd) {
        uint8_t* pNewEnd = pOldEnd + insBytes;
        int64_t tailCount  = static_cast<int64_t>((pOldEnd - pPos) >> 4) * static_cast<int64_t>(0xcccccccccccccccdULL);
        int64_t totalCount = static_cast<int64_t>((pNewEnd - pPos) >> 4) * static_cast<int64_t>(0xcccccccccccccccdULL);

        int64_t a = totalCount;
        int64_t b = tailCount;
        int64_t gcd = b;
        while (true) {
            gcd = b;
            b = a % gcd;
            a = gcd;
            if (b == 0) {
                break;
            }
        }

        if (gcd < totalCount && gcd > 0) {
            int64_t tailBytes = tailCount * 80LL;
            for (int64_t cycle = gcd; cycle > 0; ) {
                --cycle;
                uint8_t* pStart = pPos + cycle * 80LL;
                uint8_t* pNext  = pPos + (cycle + tailCount) * 80LL;
                if (pNext == pNewEnd) {
                    pNext = pPos;
                }
                uint8_t* pDst = pStart;
                do {
                    alignas(16) uint8_t tmp[0x50];
                    copyRecord80(tmp, pDst);

                    _mm_store_ps(reinterpret_cast<float*>(pDst + 0x0),  _mm_load_ps(reinterpret_cast<const float*>(pNext + 0x0)));
                    _mm_store_ps(reinterpret_cast<float*>(pDst + 0x10), _mm_load_ps(reinterpret_cast<const float*>(pNext + 0x10)));
                    _mm_store_ps(reinterpret_cast<float*>(pDst + 0x20), _mm_load_ps(reinterpret_cast<const float*>(pNext + 0x20)));
                    _mm_store_ps(reinterpret_cast<float*>(pDst + 0x30), _mm_load_ps(reinterpret_cast<const float*>(pNext + 0x30)));
                    *reinterpret_cast<uint16_t*>(pDst + 0x4c) = *reinterpret_cast<const uint16_t*>(pNext + 0x4c);
                    *reinterpret_cast<uint32_t*>(pDst + 0x48) = *reinterpret_cast<const uint32_t*>(pNext + 0x48);
                    *reinterpret_cast<uint64_t*>(pDst + 0x40) = *reinterpret_cast<const uint64_t*>(pNext + 0x40);

                    *reinterpret_cast<uint64_t*>(pNext + 0x0)  = *reinterpret_cast<const uint64_t*>(tmp + 0x0);
                    _mm_store_ps(reinterpret_cast<float*>(pNext + 0x10), _mm_load_ps(reinterpret_cast<const float*>(tmp + 0x10)));
                    _mm_store_ps(reinterpret_cast<float*>(pNext + 0x20), _mm_load_ps(reinterpret_cast<const float*>(tmp + 0x20)));
                    _mm_store_ps(reinterpret_cast<float*>(pNext + 0x30), _mm_load_ps(reinterpret_cast<const float*>(tmp + 0x30)));
                    *reinterpret_cast<uint64_t*>(pNext + 0x40) = *reinterpret_cast<const uint64_t*>(tmp + 0x40);
                    *reinterpret_cast<uint32_t*>(pNext + 0x48) = *reinterpret_cast<const uint32_t*>(tmp + 0x48);
                    *reinterpret_cast<uint16_t*>(pNext + 0x4c) = *reinterpret_cast<const uint16_t*>(tmp + 0x4c);

                    int64_t remCount = static_cast<int64_t>((pNewEnd - pNext) >> 4) * static_cast<int64_t>(0xcccccccccccccccdULL);
                    int64_t diff = tailCount - remCount;
                    uint8_t* pStep = (diff >= 0) ? (pPos + diff * 80LL) : (pNext + tailBytes);
                    pDst  = pNext;
                    pNext = pStep;
                } while (pNext != pStart);
            }
            pOldEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
        }
    }
    *reinterpret_cast<uint8_t**>(vec + 0x10) = pOldEnd + insBytes;
}

// ============================================================================
// 26. sub_2305eb0 (0x2305eb0, 152 B)
//     Reserves minCap 80-byte elements in pVec using TLS setjmp guard.
// ============================================================================
extern "C" void sub_2305eb0(void* pVec, uint64_t minCap) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    setTlsAllocGuard(1);
    if (sub_2bbe8f8(getTlsJmpBuf()) == 0) {
        uint64_t curCap = ((*reinterpret_cast<uint64_t*>(vec + 0x18) - *reinterpret_cast<uint64_t*>(vec + 0x8)) >> 4)
                          * 0xcccccccccccccccdULL;
        if (curCap < minCap) {
            if (minCap >= 0x333333333333334ULL) {
                sub_2307380();
            }
            sub_2952bf0(vec, minCap);
        }
    }
    setTlsAllocGuard(0);
}

// ============================================================================
// 27. sub_2305dd0 (0x2305dd0, 218 B)
//     Appends *pVal to an 8-byte pointer vector, growing via sub_2307280.
// ============================================================================
extern "C" void sub_2305dd0(void* pVec, const uint64_t* pVal) {
    sub_23071a0(pVec, pVal);
}

// ============================================================================
// 28. sub_23059d0 (0x23059d0, 1,009 B)
//     Inserts range [firstIter.pNode, lastIter.pNode) of 16-byte elements into
//     vector pVec at posIter.pNode under TLS setjmp guard.
// ============================================================================
extern "C" void sub_23059d0(
    void* pVec,
    FfxIter24 posIter,
    FfxIter24 firstIter,
    FfxIter24 lastIter) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    setTlsAllocGuard(1);
    if (sub_2bbe8f8(getTlsJmpBuf()) == 0) {
        const uint8_t* pFirst = static_cast<const uint8_t*>(firstIter.pNode);
        const uint8_t* pLast  = static_cast<const uint8_t*>(lastIter.pNode);
        uint64_t insertCount  = static_cast<uint64_t>((pLast - pFirst) >> 4);
        if (insertCount != 0) {
            uint8_t* pPos = static_cast<uint8_t*>(posIter.pNode);
            uint8_t* pEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
            uint8_t* pCap = *reinterpret_cast<uint8_t**>(vec + 0x18);
            uint64_t availCount = static_cast<uint64_t>((pCap - pEnd) >> 4);

            if (availCount < insertCount) {
                uint8_t* pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
                uint64_t curSize = static_cast<uint64_t>((pEnd - pBegin) >> 4);
                if ((0xfffffffffffffffULL - curSize) < insertCount) {
                    sub_2306bc0();
                }
                uint64_t needed = curSize + insertCount;
                uint64_t curCap = static_cast<uint64_t>((pCap - pBegin) >> 4);
                uint64_t halfCap = curCap >> 1;
                uint64_t newCap = (0xfffffffffffffffULL - halfCap >= curCap) ? (curCap + halfCap) : 0ULL;
                if (newCap < needed) {
                    newCap = needed;
                }

                void* pAlloc = *reinterpret_cast<void**>(vec + 0x20);
                void* vtbl = *reinterpret_cast<void**>(pAlloc);
                auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(
                    static_cast<uint8_t*>(vtbl) + 0x58);
                uint64_t allocBytes = newCap << 4;
                uint8_t* pNewBuf = fnAlloc(pAlloc, allocBytes, 0x8ULL);
                if (!pNewBuf) {
                    handleContainerAllocFailure();
                }

                pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
                uint8_t* pWrite = pNewBuf;
                for (uint8_t* pCur = pBegin; pCur != pPos; pCur += 0x10, pWrite += 0x10) {
                    if (pWrite) {
                        *reinterpret_cast<uint64_t*>(pWrite + 0x0) = *reinterpret_cast<const uint64_t*>(pCur + 0x0);
                        *reinterpret_cast<uint64_t*>(pWrite + 0x8) = *reinterpret_cast<const uint64_t*>(pCur + 0x8);
                    }
                }
                for (const uint8_t* pCur = pFirst; pCur != pLast; pCur += 0x10, pWrite += 0x10) {
                    if (pWrite) {
                        *reinterpret_cast<uint64_t*>(pWrite + 0x0) = *reinterpret_cast<const uint64_t*>(pCur + 0x0);
                        *reinterpret_cast<uint64_t*>(pWrite + 0x8) = *reinterpret_cast<const uint64_t*>(pCur + 0x8);
                    }
                }
                pEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
                for (uint8_t* pCur = pPos; pCur != pEnd; pCur += 0x10, pWrite += 0x10) {
                    if (pWrite) {
                        *reinterpret_cast<uint64_t*>(pWrite + 0x0) = *reinterpret_cast<const uint64_t*>(pCur + 0x0);
                        *reinterpret_cast<uint64_t*>(pWrite + 0x8) = *reinterpret_cast<const uint64_t*>(pCur + 0x8);
                    }
                }

                pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
                pEnd   = *reinterpret_cast<uint8_t**>(vec + 0x10);
                uint64_t newSize = static_cast<uint64_t>((pEnd - pBegin) >> 4) + insertCount;
                if (pBegin) {
                    pAlloc = *reinterpret_cast<void**>(vec + 0x20);
                    vtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBegin);
                }
                *reinterpret_cast<uint8_t**>(vec + 0x18) = pNewBuf + allocBytes;
                *reinterpret_cast<uint8_t**>(vec + 0x10) = pNewBuf + (newSize << 4);
                *reinterpret_cast<uint8_t**>(vec + 0x8)  = pNewBuf;
            } else {
                for (const uint8_t* pCur = pFirst; pCur != pLast; pCur += 0x10, pEnd += 0x10) {
                    if (pEnd) {
                        *reinterpret_cast<uint64_t*>(pEnd + 0x0) = *reinterpret_cast<const uint64_t*>(pCur + 0x0);
                        *reinterpret_cast<uint64_t*>(pEnd + 0x8) = *reinterpret_cast<const uint64_t*>(pCur + 0x8);
                    }
                }
                uint8_t* pOldEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
                uint64_t insBytes = insertCount << 4;
                if (pPos != pOldEnd) {
                    int64_t tailCount = (pOldEnd - pPos) >> 4;
                    if (tailCount != 0) {
                        uint8_t* pNewEnd = pOldEnd + insBytes;
                        int64_t totalCount = (pNewEnd - pPos) >> 4;
                        int64_t a = totalCount;
                        int64_t b = tailCount;
                        int64_t gcd = b;
                        while (true) {
                            gcd = b;
                            b = a % gcd;
                            a = gcd;
                            if (b == 0) {
                                break;
                            }
                        }
                        if (gcd < totalCount && gcd > 0) {
                            int64_t tailBytes = tailCount << 4;
                            for (int64_t cycle = gcd; cycle > 0; ) {
                                --cycle;
                                uint8_t* pStart = pPos + (cycle << 4);
                                uint8_t* pNext  = pPos + ((cycle + tailCount) << 4);
                                if (pNext == pNewEnd) {
                                    pNext = pPos;
                                }
                                uint8_t* pDst = pStart;
                                do {
                                    uint64_t t0 = *reinterpret_cast<uint64_t*>(pDst + 0x0);
                                    uint64_t t1 = *reinterpret_cast<uint64_t*>(pDst + 0x8);
                                    *reinterpret_cast<uint64_t*>(pDst + 0x0) = *reinterpret_cast<uint64_t*>(pNext + 0x0);
                                    *reinterpret_cast<uint64_t*>(pDst + 0x8) = *reinterpret_cast<uint64_t*>(pNext + 0x8);
                                    *reinterpret_cast<uint64_t*>(pNext + 0x0) = t0;
                                    *reinterpret_cast<uint64_t*>(pNext + 0x8) = t1;

                                    int64_t remCount = (pNewEnd - pNext) >> 4;
                                    int64_t diff = tailCount - remCount;
                                    uint8_t* pStep = (diff >= 0) ? (pPos + (diff << 4)) : (pNext + tailBytes);
                                    pDst  = pNext;
                                    pNext = pStep;
                                } while (pNext != pStart);
                            }
                            pOldEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
                        }
                    }
                } else {
                    pOldEnd = pPos;
                }
                *reinterpret_cast<uint8_t**>(vec + 0x10) = pOldEnd + insBytes;
            }
        }
    }
    setTlsAllocGuard(0);
}

// ============================================================================
// 29. sub_2305900 (0x2305900, 198 B)
//     Routes particle instance pObj into either list pSelf+0x38 (if it has an
//     active tessellation/light-shaft/decal node of type 3, 0x3d, or 0x6c and
//     global flag 0x513b318 is enabled) or list pSelf+0x10.
// ============================================================================
extern "C" void sub_2305900(void* pSelf, void* pObj) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint8_t* obj  = static_cast<uint8_t*>(pObj);
    uint64_t objVal = reinterpret_cast<uint64_t>(pObj);

    if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x513b318ULL)) != 0) {
        for (void* pNode = *reinterpret_cast<void**>(obj + 0x78);
             pNode != nullptr;
             pNode = sub_284f0e0(pNode)) {
            uint8_t* pData = static_cast<uint8_t*>(sub_284f340(pNode));
            if (!pData || (pData[0x4] & 0x5) == 0) {
                continue;
            }
            void* pSub = *reinterpret_cast<void**>(pData + 0x58);
            if (!pSub) {
                continue;
            }
            uint32_t* pType = sub_2869560(pSub);
            if (!pType) {
                continue;
            }
            uint32_t typeId = *pType;
            if (typeId == 0x3u || typeId == 0x3du || typeId == 0x6cu) {
                sub_23071a0(self + 0x38, &objVal);
                return;
            }
        }
    }
    sub_23071a0(self + 0x10, &objVal);
}

// ============================================================================
// 30. sub_2305790 (0x2305790, 368 B)
//     Reserves newCap 16-byte elements in pVec under TLS setjmp guard.
// ============================================================================
extern "C" void sub_2305790(void* pVec, uint64_t newCap) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    setTlsAllocGuard(1);
    if (sub_2bbe8f8(getTlsJmpBuf()) == 0) {
        uint64_t curCap = (*reinterpret_cast<uint64_t*>(vec + 0x18) - *reinterpret_cast<uint64_t*>(vec + 0x8)) >> 4;
        if (curCap < newCap) {
            if ((newCap >> 60) != 0) {
                sub_2306bc0();
            }
            void* pAlloc = *reinterpret_cast<void**>(vec + 0x20);
            void* vtbl = *reinterpret_cast<void**>(pAlloc);
            auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(
                static_cast<uint8_t*>(vtbl) + 0x58);
            uint64_t allocBytes = newCap << 4;
            uint8_t* pNewBuf = fnAlloc(pAlloc, allocBytes, 0x8ULL);
            if (!pNewBuf) {
                handleContainerAllocFailure();
            }

            uint8_t* pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
            uint8_t* pEnd   = *reinterpret_cast<uint8_t**>(vec + 0x10);
            if (pBegin != pEnd) {
                uint8_t* pDst = pNewBuf;
                for (uint8_t* pCur = pBegin; pCur != pEnd; pCur += 0x10, pDst += 0x10) {
                    if (pDst) {
                        *reinterpret_cast<uint64_t*>(pDst + 0x0) = *reinterpret_cast<const uint64_t*>(pCur + 0x0);
                        *reinterpret_cast<uint64_t*>(pDst + 0x8) = *reinterpret_cast<const uint64_t*>(pCur + 0x8);
                    }
                }
                pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
                pEnd   = *reinterpret_cast<uint8_t**>(vec + 0x10);
            }
            uint64_t oldCount = static_cast<uint64_t>((pEnd - pBegin) >> 4);
            if (pBegin) {
                pAlloc = *reinterpret_cast<void**>(vec + 0x20);
                vtbl = *reinterpret_cast<void**>(pAlloc);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBegin);
            }
            *reinterpret_cast<uint8_t**>(vec + 0x18) = pNewBuf + allocBytes;
            *reinterpret_cast<uint8_t**>(vec + 0x10) = pNewBuf + (oldCount << 4);
            *reinterpret_cast<uint8_t**>(vec + 0x8)  = pNewBuf;
        }
    }
    setTlsAllocGuard(0);
}

// ============================================================================
// 31. sub_2305640 (0x2305640, 325 B)
//     Grows a 16-byte element vector by 1.5x when size == capacity.
// ============================================================================
extern "C" void sub_2305640(void* pVec) {
    uint8_t* vec = static_cast<uint8_t*>(pVec);
    uint8_t* pCap = *reinterpret_cast<uint8_t**>(vec + 0x18);
    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(vec + 0x10);
    if (pCap != pEnd) {
        return;
    }

    uint8_t* pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
    uint64_t curCap = static_cast<uint64_t>((pCap - pBegin) >> 4);
    uint64_t halfCap = curCap >> 1;
    uint64_t newCap = (0xfffffffffffffffULL - halfCap >= curCap) ? (curCap + halfCap) : 0ULL;
    if (newCap < curCap + 1ULL) {
        newCap = curCap + 1ULL;
    }

    void* pAlloc = *reinterpret_cast<void**>(vec + 0x20);
    void* vtbl = *reinterpret_cast<void**>(pAlloc);
    auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58);
    uint64_t allocBytes = newCap << 4;
    uint8_t* pNewBuf = fnAlloc(pAlloc, allocBytes, 0x8ULL);
    if (!pNewBuf) {
        handleContainerAllocFailure();
        return;
    }

    pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
    pEnd   = *reinterpret_cast<uint8_t**>(vec + 0x10);
    if (pBegin != pEnd) {
        uint8_t* pDst = pNewBuf;
        for (uint8_t* pCur = pBegin; pCur != pEnd; pCur += 0x10, pDst += 0x10) {
            if (pDst) {
                *reinterpret_cast<uint64_t*>(pDst + 0x0) = *reinterpret_cast<const uint64_t*>(pCur + 0x0);
                *reinterpret_cast<uint64_t*>(pDst + 0x8) = *reinterpret_cast<const uint64_t*>(pCur + 0x8);
            }
        }
        pBegin = *reinterpret_cast<uint8_t**>(vec + 0x8);
        pEnd   = *reinterpret_cast<uint8_t**>(vec + 0x10);
    }
    uint64_t oldCount = static_cast<uint64_t>((pEnd - pBegin) >> 4);
    if (pBegin) {
        pAlloc = *reinterpret_cast<void**>(vec + 0x20);
        vtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBegin);
    }
    *reinterpret_cast<uint8_t**>(vec + 0x18) = pNewBuf + allocBytes;
    *reinterpret_cast<uint8_t**>(vec + 0x10) = pNewBuf + (oldCount << 4);
    *reinterpret_cast<uint8_t**>(vec + 0x8)  = pNewBuf;
}

// ============================================================================
// 32. sub_2305560 (0x2305560, 218 B)
//     Appends *pVal to an 8-byte pointer vector, growing via sub_2306ac0.
// ============================================================================
extern "C" void sub_2305560(void* pVec, const uint64_t* pVal) {
    sub_23071a0(pVec, pVal);
}

// ============================================================================
// 33. sub_23054f0 (0x23054f0, 103 B)
//     Dispatches draw items in [pRange[0], pRange[1]) from *(pSelf+0x20)+0x8.
// ============================================================================
extern "C" void sub_23054f0(void* pSelf, const int32_t* pRange) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    int32_t cur = pRange[0];
    int32_t end = pRange[1];
    if (cur < end) {
        uint8_t* pCtx = self + 0x2130;
        int64_t offset = (static_cast<int64_t>(cur) << 4) | 8LL;
        for (; cur < end; ++cur, offset += 0x10LL) {
            uint8_t* pVecObj = *reinterpret_cast<uint8_t**>(self + 0x20);
            uint8_t* pBase   = *reinterpret_cast<uint8_t**>(pVecObj + 0x8);
            void* pItem = *reinterpret_cast<void**>(pBase + offset - 8);
            void* pData = *reinterpret_cast<void**>(pBase + offset);
            void* vtbl = *reinterpret_cast<void**>(pItem);
            auto fnDraw = *reinterpret_cast<void (**)(void*, void*, void*, uint32_t)>(
                static_cast<uint8_t*>(vtbl) + 0x10);
            fnDraw(pItem, pCtx, pData, 0);
        }
    }
}

// ============================================================================
// 34. sub_2305490 (0x2305490, 90 B)
//     Deleting destructor for vtable 0x53624d0.
// ============================================================================
extern "C" void sub_2305490(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53624d0ULL);
    sub_f16e70(self + 0x2130);
    *reinterpret_cast<uint64_t*>(self + 0x40) = GX_VMA(0x52e74e0ULL);
    if (*reinterpret_cast<uint64_t*>(self + 0x2108) >= 8ULL) {
        void* pStrBuf = *reinterpret_cast<void**>(self + 0x20f0);
        void* pAlloc  = *reinterpret_cast<void**>(self + 0x2110);
        void* vtbl    = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pStrBuf);
    }
    sub_2bbe588(self);
}

// ============================================================================
// 35. sub_2305410 (0x2305410, 114 B)
//     Non-deleting destructor for vtable 0x53624d0.
// ============================================================================
extern "C" void sub_2305410(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53624d0ULL);
    sub_f16e70(self + 0x2130);
    *reinterpret_cast<uint64_t*>(self + 0x40) = GX_VMA(0x52e74e0ULL);
    if (*reinterpret_cast<uint64_t*>(self + 0x2108) >= 8ULL) {
        void* pStrBuf = *reinterpret_cast<void**>(self + 0x20f0);
        void* pAlloc  = *reinterpret_cast<void**>(self + 0x2110);
        void* vtbl    = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pStrBuf);
    }
    *reinterpret_cast<uint64_t*>(self + 0x2108) = 7ULL;
    *reinterpret_cast<uint64_t*>(self + 0x2100) = 0ULL;
    *reinterpret_cast<uint16_t*>(self + 0x20f0) = 0;
}

// ============================================================================
// 36. sub_23053c0 (0x23053c0, 76 B)
//     Updates particle emitters in [pRange[0], pRange[1]) via sub_284ff30.
// ============================================================================
extern "C" void sub_23053c0(void* pSelf, const int32_t* pRange, int32_t slotIdx) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    int64_t cur = pRange[0];
    int32_t end = pRange[1];
    if (static_cast<int32_t>(cur) < end) {
        int64_t slot = slotIdx;
        for (; static_cast<int32_t>(cur) < end; ++cur) {
            void** ppItems = *reinterpret_cast<void***>(self + 0x18);
            void** ppCtxs  = *reinterpret_cast<void***>(self + 0x60);
            float dt       = *reinterpret_cast<float*>(self + 0x68);
            sub_284ff30(ppItems[cur], ppCtxs[slot], dt);
        }
    }
}

// ============================================================================
// 37. sub_2305350 (0x2305350, 97 B)
//     Deleting destructor for vtable 0x53624a0.
// ============================================================================
extern "C" void sub_2305350(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53624a0ULL);

    void* pBuf40 = *reinterpret_cast<void**>(self + 0x40);
    if (pBuf40) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x58);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBuf40);
        *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }

    void* pBuf18 = *reinterpret_cast<void**>(self + 0x18);
    if (pBuf18) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x30);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBuf18);
    }
    sub_2bbe588(self);
}

// ============================================================================
// 38. sub_23052d0 (0x23052d0, 117 B)
//     Non-deleting destructor for vtable 0x53624a0.
// ============================================================================
extern "C" void sub_23052d0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53624a0ULL);

    void* pBuf40 = *reinterpret_cast<void**>(self + 0x40);
    if (pBuf40) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x58);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBuf40);
        *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }

    void* pBuf18 = *reinterpret_cast<void**>(self + 0x18);
    if (pBuf18) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x30);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBuf18);
        *reinterpret_cast<uint64_t*>(self + 0x28) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    }
}

// ============================================================================
// 39. sub_2305270 (0x2305270, 95 B)
//     Dispatches particle post-step in [pRange[0], pRange[1]) via sub_284cf50.
// ============================================================================
extern "C" void sub_2305270(void* pSelf, const int32_t* pRange, int32_t slotIdx) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    int64_t cur = pRange[0];
    int32_t end = pRange[1];
    if (static_cast<int32_t>(cur) < end) {
        uint32_t uSlot = static_cast<uint32_t>(slotIdx);
        for (; static_cast<int32_t>(cur) < end; ++cur) {
            uint8_t** ppItems = *reinterpret_cast<uint8_t***>(self + 0x10);
            void** ppCtxs     = *reinterpret_cast<void***>(self + 0x30);
            sub_284cf50(ppItems[cur] + 0x80, ppCtxs[uSlot]);
        }
    }
}

// ============================================================================
// 40. sub_2305230 (0x2305230, 52 B)
//     Deleting destructor for vtable 0x5362470.
// ============================================================================
extern "C" void sub_2305230(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362470ULL);
    void* pBuf10 = *reinterpret_cast<void**>(self + 0x10);
    if (pBuf10) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x28);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBuf10);
    }
    sub_2bbe588(self);
}

// ============================================================================
// 41. sub_23051e0 (0x23051e0, 72 B)
//     Non-deleting destructor for vtable 0x5362470.
// ============================================================================
extern "C" void sub_23051e0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x5362470ULL);
    void* pBuf10 = *reinterpret_cast<void**>(self + 0x10);
    if (pBuf10) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x28);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pBuf10);
        *reinterpret_cast<uint64_t*>(self + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
    }
}

// ============================================================================
// 42. sub_23051c0 (0x23051c0, 23 B)
//     Returns u"FFXSceneCtrl" (0x49d3c10) if *(pSelf+0x70) != nullptr,
//     else u"NoName" (0x49d431e).
// ============================================================================
extern "C" const void* sub_23051c0(const void* pSelf) {
    const uint8_t* self = static_cast<const uint8_t*>(pSelf);
    if (*reinterpret_cast<void* const*>(self + 0x70) != nullptr) {
        return reinterpret_cast<const void*>(GX_VMA(0x49d3c10ULL));
    }
    return reinterpret_cast<const void*>(GX_VMA(0x49d431eULL));
}

// ============================================================================
// 43. sub_23051a0 (0x23051a0, 19 B)
// ============================================================================
extern "C" uint64_t sub_23051a0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x70);
    if (pInner) {
        void* vtbl = *reinterpret_cast<void**>(pInner);
        return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x60))(pInner, a2, a3, a4, a5, a6);
    }
    return 0;
}

// ============================================================================
// 44. sub_2305180 (0x2305180, 19 B)
// ============================================================================
extern "C" uint64_t sub_2305180(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* pInner = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x70);
    if (pInner) {
        void* vtbl = *reinterpret_cast<void**>(pInner);
        return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58))(pInner, a2, a3, a4, a5, a6);
    }
    return 0;
}

// ============================================================================
// 45. sub_2305160 (0x2305160, 28 B)
// ============================================================================
extern "C" void sub_2305160(void* pSelf) {
    sub_2282d10(pSelf);
    sub_2bbe588(pSelf);
}

// ============================================================================
// 46. sub_2305150 (0x2305150, 5 B)
// ============================================================================
extern "C" void sub_2305150(void* pSelf) {
    sub_2282d10(pSelf);
}

// ============================================================================
// 47. sub_2305120 (0x2305120, 40 B)
//     Invokes a C++ pointer-to-member callback if flag 0x40 at +0x0b is set.
// ============================================================================
extern "C" void sub_2305120(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    if ((self[0x0b] & 0x40u) != 0) {
        uint8_t* pTarget = *reinterpret_cast<uint8_t**>(self + 0x10) + *reinterpret_cast<int64_t*>(self + 0x28);
        uintptr_t fnOrVoff = *reinterpret_cast<uintptr_t*>(self + 0x20);
        if ((fnOrVoff & 1ULL) != 0) {
            uint8_t* vtbl = *reinterpret_cast<uint8_t**>(pTarget);
            fnOrVoff = *reinterpret_cast<uintptr_t*>(vtbl + fnOrVoff - 1ULL);
        }
        uint32_t arg = *reinterpret_cast<uint32_t*>(self + 0x18);
        reinterpret_cast<void (*)(void*, uint32_t)>(fnOrVoff)(pTarget, arg);
    }
}

// ============================================================================
// 48. sub_2305110 (0x2305110, 5 B)
// ============================================================================
extern "C" void sub_2305110(void* pSelf) {
    sub_2bbe588(pSelf);
}

// ============================================================================
// 49. sub_23050f0 (0x23050f0, 22 B)
// ============================================================================
extern "C" void sub_23050f0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x8) & 0xbfffffffu;
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x8) = flags;
}

// ============================================================================
// 50. sub_23050e0 (0x23050e0, 1 B)
// ============================================================================
extern "C" void sub_23050e0() {
    return;
}

// ============================================================================
// 51..57. sub_23050d0 .. sub_2305070 (0x23050d0 .. 0x2305070, 9 B each)
//         Forwards to vtable[+0x28](pSelf).
// ============================================================================
extern "C" uint64_t sub_23050d0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

extern "C" uint64_t sub_23050c0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

extern "C" uint64_t sub_23050b0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

extern "C" uint64_t sub_23050a0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

extern "C" uint64_t sub_2305090(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

extern "C" uint64_t sub_2305080(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

extern "C" uint64_t sub_2305070(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a2, a3, a4, a5, a6);
}

// ============================================================================
// 58. sub_2305060 (0x2305060, 3 B)
// ============================================================================
extern "C" bool sub_2305060() {
    return true;
}

// ============================================================================
// 59. sub_2304e00 (0x2304e00, 370 B)
//     Removes entry `key` from the GXFfxSceneCtrl hash table at pSelf+0x660.
// ============================================================================
extern "C" void sub_2304e00(void* pSelf, uint64_t key) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    if (*reinterpret_cast<uint64_t*>(self + 0xb8) == 0 || self[0x65c] == 0) {
        return;
    }

    uint8_t* pLock = self + 0x6c8;
    void* lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*, uint32_t)>(static_cast<uint8_t*>(lockVtbl) + 0x18))(
        pLock, 0xffffffffu);

    uint64_t hashVal = computeParkMillerHash64(key, sub_2bbfd68);
    uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x6a8);
    uint64_t bucket = hashVal & mask;
    if (bucket >= *reinterpret_cast<uint64_t*>(self + 0x6b0)) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x668);
    uint8_t* pBuckets = *reinterpret_cast<uint8_t**>(self + 0x688);
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pBuckets + bucket * 48 + 0x10);
    uint8_t* pFound = pHead;
    if (pCur != pHead) {
        uint8_t* pEnd = **reinterpret_cast<uint8_t***>(pBuckets + ((bucket * 2) | 1ULL) * 24 + 0x10);
        while (pCur != pEnd) {
            uint64_t curKey = *reinterpret_cast<uint64_t*>(pCur + 0x10);
            if (curKey >= key) {
                pFound = (curKey > key) ? pHead : pCur;
                break;
            }
            pCur = *reinterpret_cast<uint8_t**>(pCur);
        }
    }

    if (pFound != pHead) {
        FfxIter24 outIter = {};
        sub_2306580(&outIter, self + 0x660, pFound);
    }

    lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(lockVtbl) + 0x28))(pLock);
}

// ============================================================================
// 60. sub_2304a60 (0x2304a60, 927 B)
//     Inserts or updates entry `key` in the GXFfxSceneCtrl hash table at
//     pSelf+0x660 under lock pSelf+0x6c8.
// ============================================================================
extern "C" void sub_2304a60(void* pSelf, uint64_t key, void* pVal20, uint32_t val2C, uint32_t val28) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    if (*reinterpret_cast<uint64_t*>(self + 0xb8) == 0 || self[0x65c] == 0) {
        return;
    }

    uint8_t* pLock = self + 0x6c8;
    void* lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*, uint32_t)>(static_cast<uint8_t*>(lockVtbl) + 0x18))(
        pLock, 0xffffffffu);

    uint64_t hashVal = computeParkMillerHash64(key, sub_2bbfd68);
    uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x6a8);
    uint64_t bucket = hashVal & mask;
    if (bucket >= *reinterpret_cast<uint64_t*>(self + 0x6b0)) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x668);
    uint8_t* pBuckets = *reinterpret_cast<uint8_t**>(self + 0x688);
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pBuckets + bucket * 48 + 0x10);
    uint8_t* pFound = pHead;
    if (pCur != pHead) {
        uint8_t* pEnd = **reinterpret_cast<uint8_t***>(pBuckets + ((bucket * 2) | 1ULL) * 24 + 0x10);
        while (pCur != pEnd) {
            uint64_t curKey = *reinterpret_cast<uint64_t*>(pCur + 0x10);
            if (curKey >= key) {
                pFound = (curKey > key) ? pHead : pCur;
                break;
            }
            pCur = *reinterpret_cast<uint8_t**>(pCur);
        }
    }

    if (pFound == pHead) {
        uint8_t* pTargetNode = pHead;
        uint8_t* pOldFirst = *reinterpret_cast<uint8_t**>(pHead);
        uint8_t* pOldPrev  = *reinterpret_cast<uint8_t**>(pOldFirst + 0x8);
        void* pNodeAlloc   = *reinterpret_cast<void**>(self + 0x678);
        void* vtbl         = *reinterpret_cast<void**>(pNodeAlloc);
        auto fnAlloc = *reinterpret_cast<uint8_t* (**)(void*, uint64_t, uint64_t)>(
            static_cast<uint8_t*>(vtbl) + 0x58);
        uint8_t* pNewNode = fnAlloc(pNodeAlloc, 0x30ULL, 0x8ULL);
        if (!pNewNode) {
            handleContainerAllocFailure();
            lockVtbl = *reinterpret_cast<void**>(pLock);
            (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(lockVtbl) + 0x28))(pLock);
            return;
        }

        *reinterpret_cast<uint8_t**>(pNewNode + 0x0) = pOldFirst ? pOldFirst : pNewNode;
        *reinterpret_cast<uint8_t**>(pNewNode + 0x8) = pOldFirst ? pOldPrev  : pNewNode;
        *reinterpret_cast<uint64_t*>(pNewNode + 0x10) = key;
        *reinterpret_cast<uint64_t*>(pNewNode + 0x18) = 0;
        *reinterpret_cast<uint64_t*>(pNewNode + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(pNewNode + 0x28) = 0;

        uint64_t count = *reinterpret_cast<uint64_t*>(self + 0x670);
        if (count == 0x555555555555554ULL) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        *reinterpret_cast<uint64_t*>(self + 0x670) = count + 1ULL;
        *reinterpret_cast<uint8_t**>(pOldFirst + 0x8) = pNewNode;
        **reinterpret_cast<uint8_t***>(pNewNode + 0x8) = pNewNode;

        uint8_t* pFirstNode = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(self + 0x668));
        FfxIter24 plistIter = { 0ULL, 0ULL, pFirstNode };
        FfxInsertResult32 insRes = {};
        sub_2307390(
            &insRes,
            self + 0x660,
            reinterpret_cast<const uint64_t*>(pFirstNode + 0x10),
            plistIter);
        pTargetNode = static_cast<uint8_t*>(insRes.pNode);

        *reinterpret_cast<uint64_t*>(pTargetNode + 0x18) = key;
        *reinterpret_cast<void**>(pTargetNode + 0x20)    = pVal20;
        *reinterpret_cast<uint32_t*>(pTargetNode + 0x28) = val28;
        *reinterpret_cast<uint32_t*>(pTargetNode + 0x2c) = val2C;
    }

    lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(lockVtbl) + 0x28))(pLock);
}

// ============================================================================
// 61. sub_2304a00 (0x2304a00, 88 B)
// ============================================================================
extern "C" void sub_2304a00(void* pSelf) {
    alignas(16) uint8_t tempCtx[0x28] = {};
    sub_236c4d0(tempCtx);
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x68))(pSelf, tempCtx);
    sub_236c720(tempCtx);
}

// ============================================================================
// 62. sub_23049f0 (0x23049f0, 15 B)
// ============================================================================
extern "C" uint64_t sub_23049f0(void* pSelf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* pSub = *reinterpret_cast<void**>(static_cast<uint8_t*>(pSelf) + 0x30);
    if (pSub) {
        return sub_f1d630(pSub, a2, a3, a4, a5, a6);
    }
    return 0;
}

// ============================================================================
// 63. sub_23048f0 (0x23048f0, 241 B)
//     Finalizes and frees all 0x270-byte FFX action nodes in pSelf+0x638.
// ============================================================================
extern "C" void sub_23048f0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x640);
    uint8_t* pCur  = *reinterpret_cast<uint8_t**>(pHead);
    uint8_t* pFirst = pHead;

    if (pCur != pHead) {
        void* pCallbackFn = reinterpret_cast<void*>(GX_VMA(0x22fc280ULL));
        for (; pCur != *reinterpret_cast<uint8_t**>(self + 0x640);
             pCur = *reinterpret_cast<uint8_t**>(pCur)) {
            uint8_t* pAction = pCur + 0x10;
            uint64_t* pCbSlot = reinterpret_cast<uint64_t*>(pCur + 0x88);
            if (pCbSlot[1] != 0 || pCbSlot[0] != 0) {
                pCbSlot[2] = 0;
                pCbSlot[1] = 0;
                pCbSlot[0] = 0;
                sub_28517d0(pAction, pCur + 0x40, 0x230u, pCallbackFn);
            }
            sub_2851840(pAction);
        }
        pFirst = *reinterpret_cast<uint8_t**>(pCur);
        pHead  = pCur;
    }

    *reinterpret_cast<uint8_t**>(pHead) = pHead;
    pHead = *reinterpret_cast<uint8_t**>(self + 0x640);
    *reinterpret_cast<uint8_t**>(pHead + 0x8) = pHead;
    *reinterpret_cast<uint64_t*>(self + 0x648) = 0;

    while (pFirst != *reinterpret_cast<uint8_t**>(self + 0x640)) {
        uint8_t* pNext = *reinterpret_cast<uint8_t**>(pFirst);
        sub_2851450(pFirst + 0x10);
        void* pAlloc = *reinterpret_cast<void**>(self + 0x650);
        void* vtbl   = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAlloc, pFirst);
        pFirst = pNext;
    }
}

// ============================================================================
// 64. sub_23047d0 (0x23047d0, 282 B)
//     Builds a default world-origin FFX action node (from 0x56c4028..0x56c4030)
//     and appends it to pSelf+0x638 via sub_2306700.
// ============================================================================
extern "C" void sub_23047d0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint32_t effectId = *reinterpret_cast<uint32_t*>(self + 0x5d4);

    alignas(16) float mat4x4[16];
    _mm_store_ps(&mat4x4[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f));
    _mm_store_ps(&mat4x4[4], _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f));
    _mm_store_ps(&mat4x4[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f));
    float tx = *reinterpret_cast<const float*>(GX_VMA(0x56c4028ULL));
    float ty = *reinterpret_cast<const float*>(GX_VMA(0x56c402cULL));
    float tz = *reinterpret_cast<const float*>(GX_VMA(0x56c4030ULL));
    _mm_store_ps(&mat4x4[12], _mm_set_ps(1.0f, tz, ty, tx));

    alignas(16) uint8_t matPair[0x40] = {};
    sub_284d4f0(matPair + 0x20, matPair, mat4x4);

    alignas(16) uint8_t actionDesc[0x260] = {};
    sub_2302050(actionDesc, self, effectId, matPair, nullptr, 0, 0);
    sub_2306700(self + 0x638, *reinterpret_cast<void**>(self + 0x640), actionDesc);
    sub_2851450(actionDesc);
}

// ============================================================================
// 65. sub_23046b0 (0x23046b0, 275 B)
//     Computes translated FFX action position (pSelf+0x630 * pSelf+0x610 +
//     pSelf+0x620) and appends a new 0x270-byte FFX action node to pSelf+0x638.
// ============================================================================
extern "C" void sub_23046b0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    __m128 scale = _mm_set1_ps(*reinterpret_cast<const float*>(self + 0x630));
    __m128 dir   = _mm_load_ps(reinterpret_cast<const float*>(self + 0x610));
    __m128 base  = _mm_load_ps(reinterpret_cast<const float*>(self + 0x620));
    __m128 pos   = _mm_add_ps(_mm_mul_ps(scale, dir), base);
    pos = _mm_insert_ps(pos, _mm_set_ss(1.0f), 0x30);

    uint32_t effectId = *reinterpret_cast<uint32_t*>(self + 0x5d4);

    alignas(16) float mat4x4[16];
    _mm_store_ps(&mat4x4[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f));
    _mm_store_ps(&mat4x4[4], _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f));
    _mm_store_ps(&mat4x4[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f));
    _mm_store_ps(&mat4x4[12], pos);

    alignas(16) uint8_t matPair[0x40] = {};
    sub_284d4f0(matPair + 0x20, matPair, mat4x4);

    alignas(16) uint8_t actionDesc[0x260] = {};
    sub_2302050(actionDesc, self, effectId, matPair, nullptr, 0, 0);
    sub_2306700(self + 0x638, *reinterpret_cast<void**>(self + 0x640), actionDesc);
    sub_2851450(actionDesc);
}

// ============================================================================
// 66. sub_23045f0 (0x23045f0, 184 B)
//     Erases all entries in the GXFfxSceneCtrl hash table at pSelf+0x660 under
//     lock pSelf+0x6c8.
// ============================================================================
extern "C" void sub_23045f0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    if (*reinterpret_cast<uint64_t*>(self + 0xb8) == 0) {
        return;
    }

    uint8_t* pLock = self + 0x6c8;
    void* lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*, uint32_t)>(static_cast<uint8_t*>(lockVtbl) + 0x18))(
        pLock, 0xffffffffu);

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x668);
    uint8_t* pCur  = *reinterpret_cast<uint8_t**>(pHead);
    while (pCur != *reinterpret_cast<uint8_t**>(self + 0x668)) {
        FfxIter24 nextIter = {};
        sub_2306580(&nextIter, self + 0x660, pCur);
        pCur = static_cast<uint8_t*>(nextIter.pNode);
    }

    lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(lockVtbl) + 0x28))(pLock);
}

// ============================================================================
// 67. sub_2304570 (0x2304570, 116 B)
//     Reports any remaining entries in pSelf+0x668 via sub_ee5de0(6) under
//     lock pSelf+0x6c8.
// ============================================================================
extern "C" void sub_2304570(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    if (*reinterpret_cast<uint64_t*>(self + 0xb8) == 0) {
        return;
    }

    uint8_t* pLock = self + 0x6c8;
    void* lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*, uint32_t)>(static_cast<uint8_t*>(lockVtbl) + 0x18))(
        pLock, 0xffffffffu);

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x668);
    for (uint8_t* pCur = *reinterpret_cast<uint8_t**>(pHead);
         pCur != *reinterpret_cast<uint8_t**>(self + 0x668);
         pCur = *reinterpret_cast<uint8_t**>(pCur)) {
        sub_ee5de0(6);
    }

    lockVtbl = *reinterpret_cast<void**>(pLock);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(lockVtbl) + 0x28))(pLock);
}
