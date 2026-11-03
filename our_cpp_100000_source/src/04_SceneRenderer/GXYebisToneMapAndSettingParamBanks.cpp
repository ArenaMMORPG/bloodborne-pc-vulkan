// ============================================================================
// Bloodborne Engine Decompilation Project (CUSA03173 v1.09)
// Module 04: 04_SceneRenderer (Priority Tier 8/10)
// File: GXYebisToneMapAndSettingParamBanks.cpp
// VMA Range: 0x21fa9b0 .. 0x2206730 (55 functions, 48,886 bytes)
// Processing Order: Strictly Right-to-Left (0x2206730 down to 0x21fa9b0)
//
// Subsystem Overview:
//   1. GXDecalManager Constructor (0x2206730):
//      Initializes the deferred box decal / blood spatter / height POM decal
//      manager (vtable 0x5360420), verifying heap compatibility for its three
//      DLFixedStdAllocator vectors, zeroing the render target/decal tables,
//      initializing its embedded DLPlainLightMutex (+0x880), and setting all
//      default decal projection and fade parameters (+0x860..+0x990).
//   2. Dantelion2 Yebis2 / SceneRenderer Parameter Bank System (Groups A, B, C):
//      - Group A (0x22014b0 .. 0x22061e0): Yebis2 Post-Process & ToneMap
//        Parameter Bank Manager (element size 0x138, default entry at +0x2d8,
//        Setting Info[Yebis], Tone Map[Yebis], AutoExposure, Vignetting,
//        Chromatic Aberration, LightCollect, GS Presents - ToneMap).
//      - Group B (0x21fcc90 .. 0x2200c00): Secondary Scene Parameter Bank
//        Manager (element size 0xc0, default entry at +0x1f0).
//      - Group C (0x21fa9b0 .. 0x21fc6f0): Tertiary Scene Parameter Bank
//        Manager (element size 0x60, default entry at +0x190).
//      Includes the two-level Lehmer-LCG hash table containers (0xdeadbeef
//      seed, Park-Miller modulus q=127773 / a=16807 / r=2836), bucket vector
//      insertion/rehash routines, and Debug GUI parameter inspector dialogs.
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace {

// ----------------------------------------------------------------------------
// External Engine & CRT Helpers Resolved via VMA
// ----------------------------------------------------------------------------
using FnGetDefaultHeap          = void* (*)();
using FnLogIncompatibleHeap     = void (*)(uint32_t level, uint32_t code, const char* msg);
using FnMemset                  = void* (*)(void* dst, int val, size_t len);
using FnThrowLengthError        = void (*)(const char* msg);
using FnHandleAllocFailure      = void (*)(void* tlsSlot, uint32_t flag);
using FnSignedDivMod64          = int64_t (*)(int64_t num, int64_t den, int64_t* outRem);
using FnOperatorDelete          = void (*)(void* ptr);
using FnFormatWideString        = void (*)(void* dstString, const char16_t* fmt, ...);
using FnFormatTempWideString    = void (*)(void* dstTempStr, const char16_t* fmt, ...);
using FnConstructGuiDialog      = void (*)(void* dialog, const void* titleDesc);
using FnAttachGuiDialogChild    = void (*)(void* childDialog, void* parentDialog, void* ctx, const void* rectDesc);
using FnInitGuiBuilder          = void (*)(void* builder, void* dialog);
using FnAddGuiSectionNode       = void* (*)(void* builder, const void* labelDesc, uint32_t flags);
using FnFinishGuiBuilder        = void (*)(void* builder);
using FnAddGuiActionItem        = void* (*)(void* parentMenu, const void* emptyDesc);
using FnBindGuiActionCallback   = void (*)(void* item, const void* labelDesc, void* callbackObj);
using FnDispatchSceneBankEvent  = uint64_t (*)(void* sceneMgr, uint32_t slotIdx, uint32_t a2, uint32_t a3, uint64_t mask);
using FnRegisterSceneBank       = void (*)(void* sceneMgr, void* parentMenu, uint32_t slotIdx, uint32_t flag);

using FnParamEntryBuildGui      = void (*)(void* paramEntry, void* guiBuilder);
using FnParamEntryUnregister    = void (*)(void* paramEntry);
using FnParamEntryRegister      = void (*)(void* paramEntry, float initialVal);
using FnParamEntryBindOwner     = void (*)(void* paramEntry, void* owner);

static inline uintptr_t GetThreadLocalBase() {
    uintptr_t tlsBase = 0;
#if defined(__x86_64__)
    tlsBase = gx_guest_tls_base();
#endif
    return tlsBase;
}

// Park-Miller / Schrage LCG hash step used across Dantelion2 hash tables:
//   seed = key ^ 0xdeadbeef
//   q = seed / 0x1f31d, r = seed % 0x1f31d
//   h = r * 0x41a7 - q * 0xb14
//   if (h < 0) h += 0x7fffffffffffffff
static inline uint64_t ComputeDLHashBucket(uint32_t key, uint64_t mask, uint64_t maxBuckets) {
    const int64_t dividend = static_cast<int64_t>(static_cast<uint64_t>(key) ^ 0xdeadbeefULL);
    const int64_t divisor  = 0x1f31dLL;
    const int64_t quot     = dividend / divisor;
    const int64_t rem      = dividend % divisor;
    const int64_t prod     = rem * 0x41a7LL + quot * (-0xb14LL);
    const uint64_t hashVal = (prod < 0)
        ? static_cast<uint64_t>(prod + 0x7fffffffffffffffLL)
        : static_cast<uint64_t>(prod);

    uint64_t bucket = hashVal & mask;
    if (maxBuckets <= bucket) {
        bucket += ~(mask >> 1);
    }
    return bucket;
}

// ----------------------------------------------------------------------------
// Dantelion2 Hash Table & Bucket Vector Structures
// ----------------------------------------------------------------------------
struct DLAllocatorVtable {
    void*    slots[11];
    void*    (*Allocate)(void* self, size_t bytes, size_t align); // +0x58
    void*    slots2[2];
    void     (*Free)(void* self, void* ptr);                      // +0x70
};

struct DLAllocator {
    const DLAllocatorVtable* vptr;
};

struct DLBucketIter {
    uint64_t proxy0; // +0x00
    uint64_t proxy1; // +0x08
    void*    node;   // +0x10
};

struct DLHashBucketVector {
    uint64_t     reserved0; // +0x00
    DLBucketIter* begin;    // +0x08
    DLBucketIter* end;      // +0x10
    DLBucketIter* cap;      // +0x18
    DLAllocator*  alloc;    // +0x20
};

struct DLHashListNode {
    DLHashListNode* next;  // +0x00
    DLHashListNode* prev;  // +0x08
    uint32_t        key;   // +0x10
    uint32_t        pad14; // +0x14
    void*           value; // +0x18
};

struct DLHashTable {
    uint64_t           reserved0;     // +0x00
    DLHashListNode*    listHead;      // +0x08
    uint64_t           listSize;      // +0x10
    DLAllocator*       listAlloc;     // +0x18
    DLHashBucketVector buckets;       // +0x20 (+0x28 begin, +0x30 end, +0x38 cap, +0x40 alloc)
    uint64_t           bucketMask;    // +0x48
    uint64_t           maxBuckets;    // +0x50
    float              maxLoadFactor; // +0x58
    uint32_t           pad5C;         // +0x5C
};

struct DLHashInsertResult {
    uint64_t        proxy0;   // +0x00
    uint64_t        proxy1;   // +0x08
    DLHashListNode* node;     // +0x10
    uint8_t         inserted; // +0x18
    uint8_t         pad19[7];
};

// Shared implementation for the 6 identical 1352-byte bucket-vector _Insert_n functions:
//   0x22061e0, 0x2205c90, 0x22006b0, 0x2200160, 0x21fc1a0, 0x21fbc50
static DLBucketIter* DLHashBucketVector_InsertFillImpl(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    DLBucketIter* const oldBegin = vec->begin;
    DLBucketIter* const posPtr   = reinterpret_cast<DLBucketIter*>(posIter.node);

    if (count != 0) {
        DLBucketIter* const oldEnd = vec->end;
        DLBucketIter* const oldCap = vec->cap;
        const uint64_t availCap = static_cast<uint64_t>(oldCap - oldEnd);

        if (availCap < count) {
            const uint64_t oldSize = static_cast<uint64_t>(oldEnd - oldBegin);
            const uint64_t maxLen  = 0x0aaaaaaaaaaaaaaaULL;
            if (maxLen - oldSize < count) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                    reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            const uint64_t reqSize = oldSize + count;
            const uint64_t curCap  = static_cast<uint64_t>(oldCap - oldBegin);
            const uint64_t halfCap = curCap >> 1;
            uint64_t newCap = (maxLen - halfCap >= curCap) ? (curCap + halfCap) : 0ULL;
            if (newCap < reqSize) {
                newCap = reqSize;
            }

            DLAllocator* alloc = vec->alloc;
            DLBucketIter* newBuf = static_cast<DLBucketIter*>(
                alloc->vptr->Allocate(alloc, newCap * sizeof(DLBucketIter), 8));
            if (!newBuf) {
                int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                    GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                if (*tlsFlag != 0) {
                    void* tlsObj = reinterpret_cast<void*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                    reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                } else {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                }
                outIter->proxy0 = 0;
                outIter->proxy1 = 0;
                outIter->node   = nullptr;
                return outIter;
            }

            const uint64_t prefixCount = static_cast<uint64_t>(posPtr - vec->begin);
            DLBucketIter* fillDst = newBuf + prefixCount;
            for (uint64_t i = 0; i < count; ++i) {
                if (fillDst) {
                    fillDst->node   = fillVal->node;
                    fillDst->proxy1 = fillVal->proxy1;
                    fillDst->proxy0 = fillVal->proxy0;
                }
                ++fillDst;
            }

            DLBucketIter* srcCur = vec->begin;
            DLBucketIter* srcMid = reinterpret_cast<DLBucketIter*>(posIter.node);
            if (srcCur != srcMid) {
                DLBucketIter* dstCur = newBuf;
                while (srcCur != srcMid) {
                    if (dstCur) {
                        dstCur->node   = srcCur->node;
                        dstCur->proxy1 = srcCur->proxy1;
                        dstCur->proxy0 = srcCur->proxy0;
                    }
                    ++dstCur;
                    ++srcCur;
                }
                srcCur = reinterpret_cast<DLBucketIter*>(posIter.node);
            }

            DLBucketIter* srcEnd = vec->end;
            if (srcCur != srcEnd) {
                DLBucketIter* dstSuffix = newBuf + prefixCount + count;
                while (srcCur != srcEnd) {
                    if (dstSuffix) {
                        dstSuffix->node   = srcCur->node;
                        dstSuffix->proxy1 = srcCur->proxy1;
                        dstSuffix->proxy0 = srcCur->proxy0;
                    }
                    ++dstSuffix;
                    ++srcCur;
                }
                srcEnd = vec->end;
            }

            DLBucketIter* prevBegin = vec->begin;
            const uint64_t newSize = static_cast<uint64_t>(srcEnd - prevBegin) + count;
            if (prevBegin) {
                DLAllocator* freeAlloc = vec->alloc;
                freeAlloc->vptr->Free(freeAlloc, prevBegin);
            }
            vec->cap   = newBuf + newCap;
            vec->end   = newBuf + newSize;
            vec->begin = newBuf;
        } else {
            const uint64_t elemsAfter = static_cast<uint64_t>(oldEnd - posPtr);
            const DLBucketIter savedVal = *fillVal;
            if (elemsAfter < count) {
                if (posPtr != oldEnd) {
                    for (DLBucketIter* p = posPtr; p != oldEnd; ++p) {
                        DLBucketIter* dst = p + count;
                        if (dst) {
                            *dst = *p;
                        }
                    }
                }
                DLBucketIter* curEnd = vec->end;
                DLBucketIter* midPtr = reinterpret_cast<DLBucketIter*>(posIter.node);
                const uint64_t uninitCount = count - static_cast<uint64_t>(curEnd - midPtr);
                for (uint64_t i = 0; i < uninitCount; ++i) {
                    if (curEnd) {
                        *curEnd = savedVal;
                    }
                    ++curEnd;
                }
                DLBucketIter* origEnd = vec->end;
                vec->end = origEnd + count;
                for (DLBucketIter* p = midPtr; p != origEnd; ++p) {
                    *p = savedVal;
                }
            } else {
                DLBucketIter* tailSrc = oldEnd - count;
                for (uint64_t i = 0; i < count; ++i) {
                    DLBucketIter* dst = oldEnd + i;
                    if (dst) {
                        *dst = tailSrc[i];
                    }
                }
                vec->end = oldEnd + count;
                DLBucketIter* midPtr = reinterpret_cast<DLBucketIter*>(posIter.node);
                if (tailSrc != midPtr) {
                    DLBucketIter* dstBack = oldEnd - 1;
                    DLBucketIter* srcBack = tailSrc - 1;
                    while (true) {
                        *dstBack = *srcBack;
                        if (srcBack == midPtr) {
                            break;
                        }
                        --dstBack;
                        --srcBack;
                    }
                }
                for (uint64_t i = 0; i < count; ++i) {
                    midPtr[i] = savedVal;
                }
            }
        }
    }

    const uint64_t finalIndex = static_cast<uint64_t>(posPtr - oldBegin);
    outIter->proxy0 = 0;
    outIter->proxy1 = 0;
    outIter->node   = vec->begin + finalIndex;
    return outIter;
}

// Shared implementation for the 6 identical 1040-byte hash table insert/rehash functions:
//   0x2205880, 0x2205470, 0x21ffd50, 0x21ff940, 0x21fb840, 0x21fb430
static DLHashInsertResult* DLHashTable_InsertNodeImpl(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    const uint32_t key = *keyPtr;
    const uint64_t bucket = ComputeDLHashBucket(key, table->bucketMask, table->maxBuckets);

    DLHashListNode* const listHead = table->listHead;
    DLBucketIter* const bucketsBegin = table->buckets.begin;
    DLHashListNode* bucketFirst = static_cast<DLHashListNode*>(bucketsBegin[bucket * 2].node);
    DLHashListNode* cur = listHead;

    if (bucketFirst != listHead) {
        DLHashListNode* bucketLast = static_cast<DLHashListNode*>(bucketsBegin[bucket * 2 + 1].node);
        cur = bucketLast->next;
        while (cur != bucketFirst) {
            cur = cur->prev;
            if (key < cur->key) {
                continue;
            }
            if (cur->key >= key) {
                // Duplicate key found: unlink & free candidate node if not sentinel
                DLHashListNode* cand = static_cast<DLHashListNode*>(nodeIter.node);
                if (cand != listHead) {
                    cand->prev->next = cand->next;
                    cand->next->prev = cand->prev;
                    DLAllocator* alloc = table->listAlloc;
                    alloc->vptr->Free(alloc, cand);
                    table->listSize--;
                }
                outRes->proxy0   = 0;
                outRes->proxy1   = 0;
                outRes->node     = cur;
                outRes->inserted = 0;
                return outRes;
            }
            cur = cur->next;
            break;
        }
        if (cur == bucketFirst) {
            cur = bucketFirst;
        }
    } else {
        cur = bucketFirst;
    }

    DLHashListNode* const insertPos = cur;
    DLHashListNode* const newNode   = static_cast<DLHashListNode*>(nodeIter.node);
    DLHashListNode* const nextNode  = newNode->next;

    if (insertPos != nextNode) {
        newNode->prev->next  = nextNode;
        nextNode->prev->next = insertPos;
        insertPos->prev->next = newNode;

        DLHashListNode* tmpPrev = insertPos->prev;
        insertPos->prev = nextNode->prev;
        nextNode->prev  = newNode->prev;
        newNode->prev   = tmpPrev;
    }

    DLBucketIter* const curBuckets = table->buckets.begin;
    DLBucketIter* const slotFirst  = &curBuckets[bucket * 2];
    DLBucketIter* const slotLast   = &curBuckets[bucket * 2 + 1];

    if ( static_cast<DLHashListNode*>(slotFirst->node) == table->listHead ) {
        slotFirst->proxy0 = nodeIter.proxy0;
        slotFirst->proxy1 = nodeIter.proxy1;
        slotFirst->node   = newNode;

        slotLast->proxy0  = nodeIter.proxy0;
        slotLast->proxy1  = nodeIter.proxy1;
        slotLast->node    = newNode;
    } else if (static_cast<DLHashListNode*>(slotFirst->node) == insertPos) {
        slotFirst->proxy0 = nodeIter.proxy0;
        slotFirst->proxy1 = nodeIter.proxy1;
        slotFirst->node   = newNode;
    } else {
        DLHashListNode* lastNode = static_cast<DLHashListNode*>(slotLast->node)->next;
        slotLast->node = lastNode;
        if (lastNode != newNode) {
            slotLast->node = lastNode->prev;
        }
    }

    // Check load factor & rehash if necessary
    const float sizeF   = static_cast<float>(table->listSize);
    const float bucketF = static_cast<float>(table->maxBuckets);
    if ((sizeF / bucketF) > table->maxLoadFactor) {
        uint64_t newMaxBuckets = table->maxBuckets;
        if (newMaxBuckets <= 0x1ffULL) {
            newMaxBuckets <<= 3;
        } else if (newMaxBuckets < 0x0555555555555555ULL) {
            newMaxBuckets <<= 1;
        }

        DLBucketIter fillSentinel{0, 0, table->listHead};
        table->buckets.end = table->buckets.begin;
        DLBucketIter posBegin{0, 0, table->buckets.begin};
        DLBucketIter dummyOut{};
        DLHashBucketVector_InsertFillImpl(
            &dummyOut, &table->buckets, newMaxBuckets * 2, &fillSentinel, posBegin);

        table->bucketMask = newMaxBuckets - 1;
        table->maxBuckets = newMaxBuckets;

        DLHashListNode* head = table->listHead;
        if (head->next != head) {
            DLHashListNode* lastOriginal = head->prev;
            while (true) {
                DLHashListNode* moveNode = table->listHead->next;
                DLBucketIter moveIter{0, 0, moveNode};
                DLHashInsertResult reinsertOut{};
                DLHashTable_InsertNodeImpl(&reinsertOut, table, &moveNode->key, moveIter);
                if (moveNode == lastOriginal) {
                    break;
                }
            }
        }
    }

    outRes->proxy0   = nodeIter.proxy0;
    outRes->proxy1   = nodeIter.proxy1;
    outRes->node     = newNode;
    outRes->inserted = 1;
    return outRes;
}

// Shared implementation for the 6 identical 363-byte hash table erase functions:
//   0x2205300, 0x2205190, 0x21ff7d0, 0x21ff660, 0x21fb2c0, 0x21fb150
static DLBucketIter* DLHashTable_EraseNodeImpl(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    const uint64_t bucket = ComputeDLHashBucket(
        eraseNode->key, table->bucketMask, table->maxBuckets);

    DLBucketIter* const buckets = table->buckets.begin;
    DLBucketIter* const slotFirst = &buckets[bucket * 2];
    DLBucketIter* const slotLast  = &buckets[bucket * 2 + 1];

    if (static_cast<DLHashListNode*>(slotLast->node) == eraseNode) {
        if (static_cast<DLHashListNode*>(slotFirst->node) == eraseNode) {
            slotFirst->proxy0 = 0;
            slotFirst->proxy1 = 0;
            slotFirst->node   = table->listHead;

            slotLast->proxy0  = 0;
            slotLast->proxy1  = 0;
            slotLast->node    = table->listHead;
        } else {
            slotLast->proxy0  = 0;
            slotLast->proxy1  = 0;
            slotLast->node    = eraseNode->prev;
        }
    } else if (static_cast<DLHashListNode*>(slotFirst->node) == eraseNode) {
        slotFirst->proxy0 = 0;
        slotFirst->proxy1 = 0;
        slotFirst->node   = eraseNode->next;
    }

    DLHashListNode* const nextNode = eraseNode->next;
    if (eraseNode != table->listHead) {
        eraseNode->prev->next = nextNode;
        eraseNode->next->prev = eraseNode->prev;
        DLAllocator* alloc = table->listAlloc;
        alloc->vptr->Free(alloc, eraseNode);
        table->listSize--;
    }

    outIter->proxy0 = 0;
    outIter->proxy1 = 0;
    outIter->node   = nextNode;
    return outIter;
}

// Helper to decrement refcount on a Dantelion2 ref-counted GUI string/node (+0x38)
static void ReleaseTempGuiStringNode(uint8_t* node) {
    if (!node) {
        return;
    }
    int32_t* refCount = reinterpret_cast<int32_t*>(node + 0x38);
    if (--(*refCount) == 0) {
        DLAllocator* strPoolAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e10ULL));
        uint64_t strCap = *reinterpret_cast<uint64_t*>(node + 0x20);
        if (strCap >= 8) {
            void* strBuf = *reinterpret_cast<void**>(node + 0x08);
            DLAllocator* bufAlloc = *reinterpret_cast<DLAllocator**>(node + 0x28);
            bufAlloc->vptr->Free(bufAlloc, strBuf);
        }
        *reinterpret_cast<uint64_t*>(node + 0x20) = 7;
        *reinterpret_cast<uint64_t*>(node + 0x18) = 0;
        *reinterpret_cast<uint16_t*>(node + 0x08) = 0;
        strPoolAlloc->vptr->Free(strPoolAlloc, node);
    }
}

// Helper to decrement weak/strong reference count on a Dantelion2 GUI widget (+0x08)
static void ReleaseGuiWidgetRef(uint8_t* widget) {
    if (!widget) {
        return;
    }
    uint32_t oldBits = *reinterpret_cast<uint32_t*>(widget + 0x08);
    uint32_t strongDec = oldBits + 0x7fffU;
    uint32_t newBits = (oldBits & 0xffff8000U) | (strongDec & 0x7fffU);
    *reinterpret_cast<uint32_t*>(widget + 0x08) = newBits;

    if ((strongDec & 0x7fffU) == 0 && (oldBits & 0x40000000U) != 0) {
        uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(widget);
        reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl + 0x18))(widget);
        newBits = *reinterpret_cast<uint32_t*>(widget + 0x08) & 0xbfffffffU;
        *reinterpret_cast<uint32_t*>(widget + 0x08) = newBits;
    }

    uint32_t weakDec = (newBits >> 15) + 0x7fffU;
    uint32_t finalBits = (newBits & 0xc0007fffU) | ((weakDec & 0x7fffU) << 15);
    *reinterpret_cast<uint32_t*>(widget + 0x08) = finalBits;

    if ((weakDec & 0x7fffU) == 0) {
        *reinterpret_cast<uint32_t*>(widget + 0x08) = newBits | 0x3fff8000U;
        uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(widget);
        reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl + 0x20))(widget);
    }
}

// Shared implementation for the 3 1708-byte OpenParamEditListDialog functions:
//   0x22049f0 (Group A: slotOffset=0x34, strOffset=0x228, defOffset=0x2d8, buildFn=0xe8d710)
//   0x21feec0 (Group B: slotOffset=0x30, strOffset=0x140, defOffset=0x1f0, buildFn=0xe84bb0)
//   0x21fa9b0 (Group C: slotOffset=0x48, strOffset=0x0e0, defOffset=0x190, buildFn=0xe83020)
static void GXParamBankManager_OpenParamEditListDialogImpl(
    uint8_t* self,
    uint32_t sceneSlotOffset,
    uint32_t titleStrOffset,
    uint32_t defaultEntryOffset,
    FnParamEntryBuildGui buildGuiFn)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + sceneSlotOffset);
    uint32_t activeBank = *reinterpret_cast<uint32_t*>(sceneMgr + slotIdx * 4 + 0x588);

    // Format "ParamEditId List: Bank[%d]" into embedded DLString at self + titleStrOffset
    reinterpret_cast<FnFormatWideString>(GX_VMA(0x2936030ULL))(
        self + titleStrOffset,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49cbebeULL)),
        activeBank);

    // Allocate 0x178-byte GUI dialog window
    DLAllocator* guiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    uint8_t* dialog = static_cast<uint8_t*>(
        guiAlloc->vptr->Allocate(guiAlloc, 0x178, 8));
    if (dialog) {
        uint8_t* strData = self + titleStrOffset + 0x08;
        uint64_t strCap  = *reinterpret_cast<uint64_t*>(self + titleStrOffset + 0x20);
        if (strCap >= 8) {
            strData = *reinterpret_cast<uint8_t**>(strData);
        }
        uint64_t titleDesc[2] = {0, reinterpret_cast<uint64_t>(strData)};
        reinterpret_cast<FnConstructGuiDialog>(GX_VMA(0x2362cf0ULL))(dialog, titleDesc);
    }

    uint8_t* guiSysRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6bc8ULL));
    uint8_t* guiDesktop = *reinterpret_cast<uint8_t**>(
        *reinterpret_cast<uint8_t**>(guiSysRoot + 0x10) + 0x18);

    struct DialogRectDesc {
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
        uint64_t flags;
    } rectDesc{0, 0, 300, 400, 0x51502ULL};

    if (dialog && (dialog[0x0b] & 0x40) != 0 && dialog[0x6f] == 0 &&
        guiDesktop != dialog && guiDesktop != nullptr && (guiDesktop[0x0b] & 0x40) != 0)
    {
        uintptr_t desktopVtbl = *reinterpret_cast<uintptr_t*>(guiDesktop);
        int32_t canAttach = reinterpret_cast<int32_t (*)(void*, void*)>(
            *reinterpret_cast<uintptr_t*>(desktopVtbl + 0x28))(guiDesktop, dialog);
        if (canAttach != 0) {
            void* desktopCtx = *reinterpret_cast<void**>(guiDesktop + 0x20);
            reinterpret_cast<FnAttachGuiDialogChild>(GX_VMA(0x2328e40ULL))(
                dialog, guiDesktop, desktopCtx, &rectDesc);
        } else {
            ReleaseGuiWidgetRef(dialog);
            dialog = nullptr;
        }
    }

    alignas(16) uint8_t guiBuilder[32] = {};
    reinterpret_cast<FnInitGuiBuilder>(GX_VMA(0x236c4d0ULL))(guiBuilder, dialog);

    // Add "Dummy" section node for the default parameter entry
    alignas(16) uint64_t tempStrDesc[2] = {0, 0};
    reinterpret_cast<FnFormatTempWideString>(GX_VMA(0x2ba7d40ULL))(
        tempStrDesc, reinterpret_cast<const char16_t*>(GX_VMA(0x49d27c0ULL)));

    alignas(16) uint64_t labelRef[2] = {0, 0};
    uint8_t* refNode = nullptr;
    if (tempStrDesc[1] != 0) {
        labelRef[1] = tempStrDesc[1];
    } else if (tempStrDesc[0] != 0) {
        refNode = reinterpret_cast<uint8_t*>(tempStrDesc[0]);
        labelRef[0] = tempStrDesc[0];
        (*reinterpret_cast<int32_t*>(refNode + 0x38))++;
    }

    void* dummySection = reinterpret_cast<FnAddGuiSectionNode>(GX_VMA(0x236c900ULL))(
        guiBuilder, labelRef, 0);
    ReleaseTempGuiStringNode(refNode);

    if (dummySection != nullptr) {
        buildGuiFn(self + defaultEntryOffset, guiBuilder);
        uint8_t** stackBegin = *reinterpret_cast<uint8_t***>(guiBuilder + 0x08);
        uint8_t** stackEnd   = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
        if (stackBegin != stackEnd) {
            uint8_t* poppedWidget = stackEnd[-1];
            if (poppedWidget != nullptr) {
                ReleaseGuiWidgetRef(poppedWidget);
                stackEnd[-1] = nullptr;
                stackEnd = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
            }
            *reinterpret_cast<uint8_t***>(guiBuilder + 0x10) = stackEnd - 1;
        }
    }
    ReleaseTempGuiStringNode(reinterpret_cast<uint8_t*>(tempStrDesc[0]));

    // Look up activeBank in self's outer hash table (at self + 0x08)
    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    const uint64_t bankBucket = ComputeDLHashBucket(
        activeBank, outerTable->bucketMask, outerTable->maxBuckets);
    DLHashListNode* const outerHead = outerTable->listHead;
    DLBucketIter* const outerBuckets = outerTable->buckets.begin;
    DLHashListNode* bankNode = static_cast<DLHashListNode*>(outerBuckets[bankBucket * 2].node);

    if (bankNode != outerHead) {
        DLHashListNode* bankLast = static_cast<DLHashListNode*>(
            outerBuckets[bankBucket * 2 + 1].node)->next;
        while (bankNode != bankLast) {
            if (bankNode->key >= activeBank) {
                if (activeBank < bankNode->key) {
                    bankNode = outerHead;
                }
                break;
            }
            bankNode = bankNode->next;
        }
        if (bankNode != outerHead && bankNode != bankLast) {
            DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
            DLHashListNode* paramNode = innerTable->listHead->next;
            while (paramNode != innerTable->listHead) {
                if (paramNode->key != 0xffffffffU) {
                    alignas(16) uint64_t paramStrDesc[2] = {0, 0};
                    reinterpret_cast<FnFormatTempWideString>(GX_VMA(0x2ba7d40ULL))(
                        paramStrDesc,
                        reinterpret_cast<const char16_t*>(GX_VMA(0x49cbef4ULL)),
                        paramNode->key);

                    alignas(16) uint64_t paramLabelRef[2] = {0, 0};
                    uint8_t* paramRefNode = nullptr;
                    if (paramStrDesc[1] != 0) {
                        paramLabelRef[1] = paramStrDesc[1];
                    } else if (paramStrDesc[0] != 0) {
                        paramRefNode = reinterpret_cast<uint8_t*>(paramStrDesc[0]);
                        paramLabelRef[0] = paramStrDesc[0];
                        (*reinterpret_cast<int32_t*>(paramRefNode + 0x38))++;
                    }

                    void* paramSection = reinterpret_cast<FnAddGuiSectionNode>(GX_VMA(0x236c900ULL))(
                        guiBuilder, paramLabelRef, 0);
                    ReleaseTempGuiStringNode(paramRefNode);

                    if (paramSection != nullptr) {
                        buildGuiFn(paramNode->value, guiBuilder);
                        uint8_t** stackBegin = *reinterpret_cast<uint8_t***>(guiBuilder + 0x08);
                        uint8_t** stackEnd   = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
                        if (stackBegin != stackEnd) {
                            uint8_t* poppedWidget = stackEnd[-1];
                            if (poppedWidget != nullptr) {
                                ReleaseGuiWidgetRef(poppedWidget);
                                stackEnd[-1] = nullptr;
                                stackEnd = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
                            }
                            *reinterpret_cast<uint8_t***>(guiBuilder + 0x10) = stackEnd - 1;
                        }
                    }
                    ReleaseTempGuiStringNode(reinterpret_cast<uint8_t*>(paramStrDesc[0]));
                    innerTable = static_cast<DLHashTable*>(bankNode->value);
                }
                paramNode = paramNode->next;
            }
        }
    }

    reinterpret_cast<FnFinishGuiBuilder>(GX_VMA(0x236c720ULL))(guiBuilder);
}

} // namespace

// ============================================================================
// [1 / 55] 0x2206730 (1161 B) — GXDecalManager_Construct
// ============================================================================
extern "C" void GXDecalManager_Construct(uint8_t* self, void* ownerContext) {
    *reinterpret_cast<uintptr_t*>(self + 0x00) = GX_VMA(0x5360420ULL);
    *reinterpret_cast<uint64_t*>(self + 0x08)  = 0;
    *reinterpret_cast<void**>(self + 0x10)     = ownerContext;

    void** globalHeapSlot = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));

    auto acquireCheckedHeap = [&]() -> void* {
        void* heap = *globalHeapSlot;
        if (!heap) {
            heap = reinterpret_cast<FnGetDefaultHeap>(GX_VMA(0x2ba2b10ULL))();
            *globalHeapSlot = heap;
        }
        uint8_t heapFlags[8] = {};
        uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(heap);
        reinterpret_cast<void (*)(void*, void*, uint32_t)>(
            *reinterpret_cast<uintptr_t*>(vtbl + 0x20))(heapFlags, heap, 0);
        if ((heapFlags[0] & 0x20) == 0) {
            reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }
        return heap;
    };

    void* heap0 = acquireCheckedHeap();
    *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x20) = 0;
    *reinterpret_cast<void**>(self + 0x38)    = heap0;

    void* heap1 = acquireCheckedHeap();
    *reinterpret_cast<uint64_t*>(self + 0x58) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
    *reinterpret_cast<void**>(self + 0x60)    = heap1;
    self[0x68] = 0;

    void* heap2 = acquireCheckedHeap();
    *reinterpret_cast<uint64_t*>(self + 0x88) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x80) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x78) = 0;
    *reinterpret_cast<void**>(self + 0x90)    = heap2;

    *reinterpret_cast<uint64_t*>(self + 0xa0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x98) = 0;

    for (size_t off = 0; off < 0x1c0; off += 8) {
        *reinterpret_cast<uint64_t*>(self + 0x588 + off) = 0;
    }

    *reinterpret_cast<uint64_t*>(self + 0x7f0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x828) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x768) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x760) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x758) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x750) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x748) = 0;

    self[0x830] = 1;
    *reinterpret_cast<uint64_t*>(self + 0x834) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x858) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x850) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x848) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x840) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x860) = 0x200;
    *reinterpret_cast<uint32_t*>(self + 0x864) = 0;

    // Embedded DLPlainLightMutex at +0x880
    *reinterpret_cast<uintptr_t*>(self + 0x880) = GX_VMA(0x53b3020ULL);
    __atomic_exchange_n(reinterpret_cast<uint32_t*>(self + 0x888), 0U, __ATOMIC_SEQ_CST);
    *reinterpret_cast<uintptr_t*>(self + 0x880) = GX_VMA(0x52ef3b0ULL) + 0x10ULL;
    *reinterpret_cast<uint64_t*>(self + 0x890)  = 0;

    *reinterpret_cast<uint64_t*>(self + 0x820) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x818) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x810) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x808) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x800) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x7f8) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x990) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x988) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x980) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x868) = 0;

    reinterpret_cast<FnMemset>(GX_VMA(0x2bbe688ULL))(self + 0xa8, 0, 0x4e0);

    *reinterpret_cast<uint32_t*>(self + 0x86c) = 0x3e800000U; // 0.25f
    *reinterpret_cast<uint32_t*>(self + 0x870) = 0xbf800000U; // -1.0f
    *reinterpret_cast<uint32_t*>(self + 0x874) = 0x3f000000U; // 0.5f
    *reinterpret_cast<uint32_t*>(self + 0x878) = 0x40490fdbU; // 3.1415927f
    *reinterpret_cast<uint32_t*>(self + 0x87c) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x8a0) = 0;

    self[0x8f0] = 1;
    self[0x8f1] = 0;
    self[0x8f2] = 1;
    self[0x8f3] = 0;
    self[0x8f4] = 0;
    self[0x8f5] = 1;
    self[0x8f6] = 1;
    self[0x8f7] = 0;

    *reinterpret_cast<uint32_t*>(self + 0x8f8) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x8fc) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x900) = 0x3f800000U; // 1.0f
    *reinterpret_cast<uint32_t*>(self + 0x904) = 0x3d75c28fU; // 0.06f
    *reinterpret_cast<uint32_t*>(self + 0x908) = 0x3f000000U; // 0.5f
    *reinterpret_cast<uint32_t*>(self + 0x924) = 0x3f800000U; // 1.0f
    *reinterpret_cast<uint32_t*>(self + 0x928) = 1;
    *reinterpret_cast<uint64_t*>(self + 0x92c) = 0x40a0000040000000ULL; // {2.0f, 5.0f}
    *reinterpret_cast<uint16_t*>(self + 0x940) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x93c) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x934) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x944) = 0x3f800000U; // 1.0f

    self[0x960] = 0;
    self[0x961] = 0;
    *reinterpret_cast<uint32_t*>(self + 0x964) = 0x3f800000U; // 1.0f
    self[0x968] = 0;
    self[0x969] = 0;
    self[0x96a] = 1;
    self[0x96b] = 0;

    std::memset(self + 0x950, 0, 16);

    *reinterpret_cast<uint32_t*>(self + 0x90c) = 0x3f800000U;
    *reinterpret_cast<uint32_t*>(self + 0x910) = 0x3f800000U;
    *reinterpret_cast<uint32_t*>(self + 0x914) = 0x3f800000U;
    *reinterpret_cast<uint32_t*>(self + 0x918) = 0x3f800000U;
    *reinterpret_cast<uint32_t*>(self + 0x91c) = 0x3f800000U;
    *reinterpret_cast<uint32_t*>(self + 0x920) = 0x3f800000U;
    *reinterpret_cast<uint32_t*>(self + 0x96c) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x970) = 0x3f800000U;
}

// ============================================================================
// [2 / 55] 0x22061e0 (1352 B) — DLHashBucketVector_InsertFill_InnerA
// ============================================================================
extern "C" DLBucketIter* DLHashBucketVector_InsertFill_InnerA(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    return DLHashBucketVector_InsertFillImpl(outIter, vec, count, fillVal, posIter);
}

// ============================================================================
// [3 / 55] 0x2205c90 (1352 B) — DLHashBucketVector_InsertFill_OuterA
// ============================================================================
extern "C" DLBucketIter* DLHashBucketVector_InsertFill_OuterA(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    return DLHashBucketVector_InsertFillImpl(outIter, vec, count, fillVal, posIter);
}

// ============================================================================
// [4 / 55] 0x2205880 (1040 B) — DLHashTable_InsertNode_InnerA
// ============================================================================
extern "C" DLHashInsertResult* DLHashTable_InsertNode_InnerA(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    return DLHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// ============================================================================
// [5 / 55] 0x2205470 (1040 B) — DLHashTable_InsertNode_OuterA
// ============================================================================
extern "C" DLHashInsertResult* DLHashTable_InsertNode_OuterA(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    return DLHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// ============================================================================
// [6 / 55] 0x2205300 (363 B) — DLHashTable_EraseNode_InnerA
// ============================================================================
extern "C" DLBucketIter* DLHashTable_EraseNode_InnerA(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    return DLHashTable_EraseNodeImpl(outIter, table, eraseNode);
}

// ============================================================================
// [7 / 55] 0x2205190 (363 B) — DLHashTable_EraseNode_OuterA
// ============================================================================
extern "C" DLBucketIter* DLHashTable_EraseNode_OuterA(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    return DLHashTable_EraseNodeImpl(outIter, table, eraseNode);
}

// ============================================================================
// [8 / 55] 0x2205160 (40 B) — DLDebugGuiCallback_InvokeMethodA
// ============================================================================
extern "C" uint64_t DLDebugGuiCallback_InvokeMethodA(uint8_t* self) {
    if ((self[0x0b] & 0x40) != 0) {
        uint8_t* target = *reinterpret_cast<uint8_t**>(self + 0x10)
                        + *reinterpret_cast<int64_t*>(self + 0x28);
        uintptr_t fnPtr = *reinterpret_cast<uintptr_t*>(self + 0x20);
        if ((fnPtr & 1ULL) != 0) {
            uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(target);
            fnPtr = *reinterpret_cast<uintptr_t*>(vtbl + fnPtr - 1ULL);
        }
        uint32_t arg = *reinterpret_cast<uint32_t*>(self + 0x18);
        return reinterpret_cast<uint64_t (*)(void*, uint32_t)>(fnPtr)(target, arg);
    }
    return 0;
}

// ============================================================================
// [9 / 55] 0x2205150 (5 B) — DLDebugGuiCallback_OperatorDeleteA
// ============================================================================
extern "C" void DLDebugGuiCallback_OperatorDeleteA(void* self) {
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// [10 / 55] 0x2205130 (22 B) — DLDebugGuiCallback_DestroyBaseA
// ============================================================================
extern "C" void DLDebugGuiCallback_DestroyBaseA(uint8_t* self) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xbfffffffU;
    *reinterpret_cast<uintptr_t*>(self + 0x00) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x08)  = flags;
}

// ============================================================================
// [11 / 55] 0x22049f0 (1708 B) — GXParamBankManagerA_OpenParamEditListDialog
// ============================================================================
extern "C" void GXParamBankManagerA_OpenParamEditListDialog(uint8_t* self) {
    GXParamBankManager_OpenParamEditListDialogImpl(
        self,
        0x34,
        0x228,
        0x2d8,
        reinterpret_cast<FnParamEntryBuildGui>(GX_VMA(0xe8d710ULL)));
}

// ============================================================================
// [12 / 55] 0x2204780 (612 B) — GXParamBankManagerA_RegisterDebugMenu
// ============================================================================
extern "C" void GXParamBankManagerA_RegisterDebugMenu(uint8_t* self, void* parentMenu) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + 0x34);
    reinterpret_cast<FnRegisterSceneBank>(GX_VMA(0x21877f0ULL))(sceneMgr, parentMenu, slotIdx, 0);

    DLAllocator* cbAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));

    auto bindMenuAction = [&](const char16_t* labelWStr, uintptr_t methodAddr) {
        alignas(16) uint64_t emptyDesc[2] = {0, 0};
        void* item = reinterpret_cast<FnAddGuiActionItem>(GX_VMA(0x236d620ULL))(parentMenu, emptyDesc);

        alignas(16) uint64_t labelDesc[2] = {0, reinterpret_cast<uint64_t>(labelWStr)};
        uint8_t* cbObj = static_cast<uint8_t*>(cbAlloc->vptr->Allocate(cbAlloc, 0x30, 8));
        if (cbObj) {
            *reinterpret_cast<uint32_t*>(cbObj + 0x08)  = 0x40000000U;
            *reinterpret_cast<uintptr_t*>(cbObj + 0x00) = GX_VMA(0x53b7260ULL);
            *reinterpret_cast<void**>(cbObj + 0x10)     = self;
            *reinterpret_cast<uint32_t*>(cbObj + 0x18)  = 0;
            *reinterpret_cast<uintptr_t*>(cbObj + 0x20) = methodAddr;
            *reinterpret_cast<uint64_t*>(cbObj + 0x28)  = 0;
        }
        reinterpret_cast<FnBindGuiActionCallback>(GX_VMA(0x235e4c0ULL))(item, labelDesc, cbObj);
    };

    bindMenuAction(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL)), GX_VMA(0x22043a0ULL));
    bindMenuAction(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL)), GX_VMA(0x2204430ULL));
    bindMenuAction(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL)), GX_VMA(0x22049f0ULL));
}

namespace {

static void* GXParamBankManager_ResolveParamEntryImpl(
    uint8_t* self,
    const uint32_t* bankAndParamPair,
    uint32_t defaultEntryOffset)
{
    uint64_t packed = *reinterpret_cast<const uint64_t*>(bankAndParamPair);
    uint32_t bankId  = static_cast<uint32_t>(packed);
    uint32_t paramId = static_cast<uint32_t>(packed >> 32);

    if (bankId == 0xffffffffU && bankAndParamPair[1] == 0xffffffffU) {
        bankId  = 1U;
        paramId = 0U;
    }

    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);

    auto findOuterNode = [&](uint32_t bId) -> DLHashListNode* {
        const uint64_t bBucket = ComputeDLHashBucket(bId, outerTable->bucketMask, outerTable->maxBuckets);
        DLHashListNode* const head = outerTable->listHead;
        DLBucketIter* const buckets = outerTable->buckets.begin;
        DLHashListNode* cur = static_cast<DLHashListNode*>(buckets[bBucket * 2].node);
        if (cur == head) {
            return head;
        }
        DLHashListNode* end = static_cast<DLHashListNode*>(buckets[bBucket * 2 + 1].node)->next;
        while (cur != end) {
            if (cur->key >= bId) {
                return (bId < cur->key) ? head : cur;
            }
            cur = cur->next;
        }
        return head;
    };

    auto findInnerNode = [](DLHashTable* innerTable, uint32_t pId) -> DLHashListNode* {
        const uint64_t pBucket = ComputeDLHashBucket(pId, innerTable->bucketMask, innerTable->maxBuckets);
        DLHashListNode* const head = innerTable->listHead;
        DLBucketIter* const buckets = innerTable->buckets.begin;
        DLHashListNode* cur = static_cast<DLHashListNode*>(buckets[pBucket * 2].node);
        if (cur == head) {
            return head;
        }
        DLHashListNode* end = static_cast<DLHashListNode*>(buckets[pBucket * 2 + 1].node)->next;
        while (cur != end) {
            if (cur->key >= pId) {
                return (pId < cur->key) ? head : cur;
            }
            cur = cur->next;
        }
        return head;
    };

    DLHashListNode* bankNode = findOuterNode(bankId);
    if (bankNode != outerTable->listHead) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
        DLHashListNode* paramNode = findInnerNode(innerTable, paramId);
        if (paramNode != innerTable->listHead) {
            return paramNode->value;
        }
    }

    // Fallback to bankId = 1, paramId = 0
    DLHashListNode* fallbackBank = findOuterNode(1U);
    if (fallbackBank != outerTable->listHead) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(fallbackBank->value);
        DLHashListNode* paramNode = findInnerNode(innerTable, 0U);
        if (paramNode != innerTable->listHead) {
            return paramNode->value;
        }
    }

    return self + defaultEntryOffset;
}

} // namespace

// ============================================================================
// [13 / 55] 0x22044a0 (732 B) — GXParamBankManagerA_ResolveParamEntry
// ============================================================================
extern "C" void* GXParamBankManagerA_ResolveParamEntry(uint8_t* self, const uint32_t* bankAndParamPair) {
    return GXParamBankManager_ResolveParamEntryImpl(self, bankAndParamPair, 0x2d8);
}

// ============================================================================
// [14 / 55] 0x2204430 (103 B) — GXParamBankManagerA_UnregisterAllParams
// ============================================================================
extern "C" void GXParamBankManagerA_UnregisterAllParams(uint8_t* self) {
    DLHashListNode* outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    for (DLHashListNode* bankNode = outerHead->next; bankNode != outerHead; bankNode = bankNode->next) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
        DLHashListNode* innerHead = innerTable->listHead;
        for (DLHashListNode* paramNode = innerHead->next; paramNode != innerHead; paramNode = paramNode->next) {
            reinterpret_cast<FnParamEntryUnregister>(GX_VMA(0xe8b480ULL))(paramNode->value);
        }
        outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    }
}

// ============================================================================
// [15 / 55] 0x22043a0 (138 B) — GXParamBankManagerA_RegisterAllParams
// ============================================================================
extern "C" void GXParamBankManagerA_RegisterAllParams(uint8_t* self) {
    DLHashListNode* outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    for (DLHashListNode* bankNode = outerHead->next; bankNode != outerHead; bankNode = bankNode->next) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
        DLHashListNode* innerHead = innerTable->listHead;
        for (DLHashListNode* paramNode = innerHead->next; paramNode != innerHead; paramNode = paramNode->next) {
            uint8_t* paramEntry = static_cast<uint8_t*>(paramNode->value);
            *reinterpret_cast<uint32_t*>(paramEntry + 0x130) = paramNode->key;
            *reinterpret_cast<uint32_t*>(paramEntry + 0x134) = bankNode->key;
            reinterpret_cast<FnParamEntryRegister>(GX_VMA(0xe8b4e0ULL))(paramEntry, 0.0f);
            reinterpret_cast<FnParamEntryBindOwner>(GX_VMA(0xe8b650ULL))(
                static_cast<uint8_t*>(paramNode->value),
                *reinterpret_cast<void**>(self));
        }
        outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    }
}

// ============================================================================
// [16 / 55] 0x2204390 (1 B) — GXParamBankManagerA_OnUpdateNoOp
// ============================================================================
extern "C" void GXParamBankManagerA_OnUpdateNoOp() {
    asm volatile("" ::: "memory");
}

// ============================================================================
// [17 / 55] 0x2204360 (40 B) — GXParamBankManagerA_NotifyParamChanged
// ============================================================================
extern "C" uint64_t GXParamBankManagerA_NotifyParamChanged(void* self, uint32_t bankId, uint32_t paramId) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + 0x34);
    uintptr_t selfTag   = reinterpret_cast<uintptr_t>(self);
    return reinterpret_cast<FnDispatchSceneBankEvent>(GX_VMA(0x2186da0ULL))(
        sceneMgr, slotIdx, bankId, paramId, selfTag ? ~0ULL : ~0ULL);
}

namespace {

using FnOnSceneBankUnregister   = void (*)(void* sceneMgr, uint32_t slotIdx, uint32_t bankId, uint32_t overrideSlotIdx, float blendTime);
using FnCheckSceneParamActive   = bool (*)(void* sceneMgr, uint32_t slotIdx, uint32_t paramId, uint32_t bankId);
using FnBeginSceneBankLoad      = int32_t (*)(void* sceneMgr, uint32_t slotIdx, void* streamDesc);
using FnQuerySceneParamIdAt     = uint32_t (*)(void* sceneMgr, uint32_t slotIdx, void* streamDesc, uint32_t index);
using FnFinishSceneBankLoad     = void (*)(void* sceneMgr, uint32_t slotIdx, uint32_t bankId, uint32_t flags, void* streamDesc, float weight);
using FnParamEntryConstruct     = void (*)(void* paramEntry);
using FnParamEntryInitDefaults  = void (*)(void* paramEntry);
using FnParamEntryDestruct      = void (*)(void* paramEntry);
using FnMemcpy                  = void* (*)(void* dst, const void* src, size_t len);
using FnDLStringReserve         = void (*)(void* dlStr, size_t cap, uint32_t flags);
using FnDLStringAssign          = void (*)(void* dlStr, const void* srcWStr, size_t len);

using FnPropMgrCreateBankGroup  = void (*)(void* propMgr, uint32_t* outSlot, const char16_t* name, const char16_t* desc);
using FnPropMgrBeginSection     = void (*)(void* propMgr, uint32_t slot, const char16_t* name, const char16_t* desc, uint32_t openFlag, uint32_t activeFlag);
using FnPropMgrEndSection       = void (*)(void* propMgr, uint32_t slot, uint32_t flag);
using FnPropMgrAddScalarProp    = void (*)(void* propMgr, uint32_t slot, const char16_t* name, const char16_t* desc, const void* propDesc, uint32_t count, uint32_t stepFlag);
using FnPropMgrAddEnumProp      = void (*)(void* propMgr, uint32_t slot, const char16_t* name, const char16_t* desc, void* fieldPtr, uint32_t enumCount, const void* enumTable, uint32_t flag);
using FnSceneBindBankPropsA     = void (*)(void* subObj, void* propMgr, uint32_t flag);
using FnSceneBindBankPropsB     = void (*)(void* subObj, void* propMgr, uint32_t a2, uint32_t slot, uint32_t a4, float a5);
using FnSceneFinalizeBankSlot   = void (*)(void* sceneMgr, uint32_t slot);

struct DLPropertyScalarDesc {
    uintptr_t vtable0;    // +0x00 (e.g. 0x52e83f0+0x10 for float, 0x52e8760+0x10 for bool)
    void*     fieldPtr;   // +0x08
    uint8_t   typeCode;   // +0x10 (9 = float, 3 = bool/u8, 5 = s32)
    uint8_t   pad11[3];
    uint32_t  accessMode; // +0x14 (0, 1, or 0x18)
    uint8_t   enabled;    // +0x18 (1)
    uint8_t   pad19[7];
    uintptr_t vtable20;   // +0x20 (e.g. 0x52e4890+0x10 for float, 0x52e4a70+0x10 for bool)
    uint32_t  minBits;    // +0x28
    uint32_t  maxBits;    // +0x2C
    uint32_t  stepBits;   // +0x30
    uint32_t  defBits;    // +0x34
};

struct DLPropertyEnumEntry {
    const char16_t* label; // +0x00
    uint32_t        value; // +0x08
    uint32_t        pad0C; // +0x0C
};

static void RegisterScalarPropHelper(
    void* propMgr,
    uint32_t slot,
    uintptr_t vtbl0,
    uintptr_t vtbl20,
    void* fieldPtr,
    uint8_t typeCode,
    uint32_t accessMode,
    uint32_t minBits,
    uint32_t maxBits,
    uint32_t stepBits,
    uint32_t defBits,
    uintptr_t nameWStrAddr,
    uintptr_t descWStrAddr,
    uint32_t stepFlag)
{
    DLPropertyScalarDesc desc{};
    desc.vtable0    = vtbl0;
    desc.fieldPtr   = fieldPtr;
    desc.typeCode   = typeCode;
    desc.accessMode = accessMode;
    desc.enabled    = 1;
    desc.vtable20   = vtbl20;
    desc.minBits    = minBits;
    desc.maxBits    = maxBits;
    desc.stepBits   = stepBits;
    desc.defBits    = defBits;

    reinterpret_cast<FnPropMgrAddScalarProp>(GX_VMA(0xe327d0ULL))(
        propMgr,
        slot,
        reinterpret_cast<const char16_t*>(nameWStrAddr),
        reinterpret_cast<const char16_t*>(descWStrAddr),
        &desc,
        1U,
        stepFlag);
}

// Helper to destroy and deallocate an inner parameter hash table
static void DestroyInnerParamHashTable(DLAllocator* managerAlloc, DLHashTable* innerTable) {
    if (!innerTable) {
        return;
    }
    if (innerTable->buckets.begin) {
        DLAllocator* bAlloc = innerTable->buckets.alloc;
        bAlloc->vptr->Free(bAlloc, innerTable->buckets.begin);
        innerTable->buckets.cap   = nullptr;
        innerTable->buckets.end   = nullptr;
        innerTable->buckets.begin = nullptr;
    }

    DLHashListNode* head = innerTable->listHead;
    DLHashListNode* cur  = head->next;
    head->next = head;
    innerTable->listHead->prev = innerTable->listHead;
    innerTable->listSize = 0;

    while (cur != innerTable->listHead) {
        DLHashListNode* next = cur->next;
        DLAllocator* lAlloc = innerTable->listAlloc;
        lAlloc->vptr->Free(lAlloc, cur);
        cur = next;
    }

    DLAllocator* lAlloc = innerTable->listAlloc;
    lAlloc->vptr->Free(lAlloc, innerTable->listHead);
    managerAlloc->vptr->Free(managerAlloc, innerTable);
}

// Shared implementation for the 2 698-byte RemoveBankOrInactiveParams functions:
//   0x22040a0 (Group A: slotOffset=0x34, activeFlagOffset=0x128)
//   0x21fe570 (Group B: slotOffset=0x30, activeFlagOffset=0x0b0)
static void GXParamBankManager_PruneInactiveParamsImpl(
    uint8_t* self,
    uint32_t bankId,
    uint32_t overrideSlotIdx,
    float blendTime,
    uint32_t sceneSlotOffset,
    uint32_t activeFlagOffset)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + sceneSlotOffset);

    reinterpret_cast<FnOnSceneBankUnregister>(GX_VMA(0x2185b50ULL))(
        sceneMgr, slotIdx, bankId, overrideSlotIdx, blendTime);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = 1;

    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    const uint64_t bucket = ComputeDLHashBucket(
        bankId, outerTable->bucketMask, outerTable->maxBuckets);

    DLHashListNode* const outerHead = outerTable->listHead;
    DLBucketIter* const outerBuckets = outerTable->buckets.begin;
    DLHashListNode* bankNode = static_cast<DLHashListNode*>(outerBuckets[bucket * 2].node);
    if (bankNode == outerHead) {
        return;
    }

    DLHashListNode* bankEnd = static_cast<DLHashListNode*>(
        outerBuckets[bucket * 2 + 1].node)->next;
    if (bankNode == bankEnd) {
        return;
    }

    while (bankNode->key < bankId) {
        bankNode = bankNode->next;
        if (bankNode == bankEnd) {
            return;
        }
    }
    if (bankNode->key > bankId) {
        bankNode = outerHead;
    }
    if (bankNode == outerHead) {
        return;
    }

    DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
    if (innerTable != nullptr) {
        DLHashListNode* innerHead = innerTable->listHead;
        DLHashListNode* paramNode = innerHead->next;
        while (paramNode != innerTable->listHead) {
            bool keep = reinterpret_cast<FnCheckSceneParamActive>(GX_VMA(0x2185e00ULL))(
                sceneMgr, slotIdx, paramNode->key, bankId);
            uint8_t* paramEntry = static_cast<uint8_t*>(paramNode->value);
            if (keep) {
                paramEntry[activeFlagOffset] = 1;
                paramNode = paramNode->next;
            } else {
                if (paramEntry != nullptr) {
                    DLAllocator* mgrAlloc = *reinterpret_cast<DLAllocator**>(self);
                    uintptr_t entryVtbl = *reinterpret_cast<uintptr_t*>(paramEntry);
                    reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(entryVtbl))(paramEntry);
                    mgrAlloc->vptr->Free(mgrAlloc, paramEntry);
                    paramNode->value = nullptr;
                }
                DLBucketIter nextIter{};
                DLHashTable_EraseNodeImpl(
                    &nextIter,
                    static_cast<DLHashTable*>(bankNode->value),
                    paramNode);
                paramNode = static_cast<DLHashListNode*>(nextIter.node);
            }
            innerTable = static_cast<DLHashTable*>(bankNode->value);
        }

        if (innerTable->listSize != 0) {
            return;
        }

        DestroyInnerParamHashTable(*reinterpret_cast<DLAllocator**>(self), innerTable);
        bankNode->value = nullptr;
    }

    DLBucketIter outErase{};
    DLHashTable_EraseNodeImpl(&outErase, outerTable, bankNode);
}

// Shared implementation for the 2 395-byte ClearAllBanks functions:
//   0x22015b0 (Group A) and 0x21fcd90 (Group B)
static void GXParamBankManager_ClearAllBanksImpl(uint8_t* self) {
    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    DLHashListNode* bankNode = outerTable->listHead->next;

    while (bankNode != outerTable->listHead) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
        DLHashListNode* paramNode = innerTable->listHead->next;
        while (paramNode != innerTable->listHead) {
            uint8_t* paramEntry = static_cast<uint8_t*>(paramNode->value);
            if (paramEntry != nullptr) {
                DLAllocator* mgrAlloc = *reinterpret_cast<DLAllocator**>(self);
                uintptr_t entryVtbl = *reinterpret_cast<uintptr_t*>(paramEntry);
                reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(entryVtbl))(paramEntry);
                mgrAlloc->vptr->Free(mgrAlloc, paramEntry);
                paramNode->value = nullptr;
                innerTable = static_cast<DLHashTable*>(bankNode->value);
            }
            DLBucketIter nextParamIter{};
            DLHashTable_EraseNodeImpl(&nextParamIter, innerTable, paramNode);
            paramNode = static_cast<DLHashListNode*>(nextParamIter.node);
            innerTable = static_cast<DLHashTable*>(bankNode->value);
        }

        if (innerTable != nullptr) {
            DestroyInnerParamHashTable(*reinterpret_cast<DLAllocator**>(self), innerTable);
            bankNode->value = nullptr;
        }

        DLBucketIter nextBankIter{};
        DLHashTable_EraseNodeImpl(&nextBankIter, outerTable, bankNode);
        bankNode = static_cast<DLHashListNode*>(nextBankIter.node);
    }
}

} // namespace

// ============================================================================
// [18 / 55] 0x22040a0 (698 B) — GXParamBankManagerA_PruneInactiveParams
// ============================================================================
extern "C" void GXParamBankManagerA_PruneInactiveParams(
    uint8_t* self,
    void* unusedSlot,
    uint32_t bankId,
    uint32_t overrideSlotIdx,
    float blendTime)
{
    (void)unusedSlot;
    GXParamBankManager_PruneInactiveParamsImpl(self, bankId, overrideSlotIdx, blendTime, 0x34, 0x128);
}

// ============================================================================
// [19 / 55] 0x2203920 (1912 B) — GXParamBankManagerA_LoadBankFromStream
// ============================================================================
extern "C" void GXParamBankManagerA_LoadBankFromStream(
    uint8_t* self,
    void* streamDesc,
    uint32_t bankId,
    uint32_t loadFlags,
    float blendWeight)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + 0x34);

    int32_t paramCount = reinterpret_cast<FnBeginSceneBankLoad>(GX_VMA(0x2184cf0ULL))(
        sceneMgr, slotIdx, streamDesc);
    if (paramCount != 0) {
        DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
        const uint64_t bBucket = ComputeDLHashBucket(
            bankId, outerTable->bucketMask, outerTable->maxBuckets);

        DLHashListNode* const outerHead = outerTable->listHead;
        DLBucketIter* const outerBuckets = outerTable->buckets.begin;
        DLHashListNode* bankNode = static_cast<DLHashListNode*>(outerBuckets[bBucket * 2].node);

        if (bankNode != outerHead) {
            DLHashListNode* bankEnd = static_cast<DLHashListNode*>(
                outerBuckets[bBucket * 2 + 1].node)->next;
            while (bankNode != bankEnd) {
                if (bankNode->key >= bankId) {
                    if (bankNode->key > bankId) {
                        bankNode = outerHead;
                    }
                    break;
                }
                bankNode = bankNode->next;
            }
        }

        if (bankNode == outerHead) {
            // Allocate & initialize new inner DLHashTable (0x60 bytes)
            DLAllocator* mgrAlloc = *reinterpret_cast<DLAllocator**>(self);
            DLHashTable* newInner = static_cast<DLHashTable*>(
                mgrAlloc->vptr->Allocate(mgrAlloc, 0x60, 8));
            if (newInner) {
                uint8_t heapFlags[8] = {};
                uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(mgrAlloc);
                reinterpret_cast<void (*)(void*, void*, uint32_t)>(
                    *reinterpret_cast<uintptr_t*>(vtbl + 0x20))(heapFlags, mgrAlloc, 0);
                if ((heapFlags[0] & 0x20) == 0) {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
                }
                newInner->listSize  = 0;
                newInner->listHead  = nullptr;
                newInner->listAlloc = mgrAlloc;

                DLHashListNode* innerHead = static_cast<DLHashListNode*>(
                    mgrAlloc->vptr->Allocate(mgrAlloc, 0x20, 8));
                if (innerHead) {
                    innerHead->next = innerHead;
                    innerHead->prev = innerHead;
                } else {
                    int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                    if (*tlsFlag != 0) {
                        void* tlsObj = reinterpret_cast<void*>(
                            GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                        reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                    } else {
                        reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                            0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    }
                }
                newInner->listHead = innerHead;

                newInner->buckets.cap   = nullptr;
                newInner->buckets.end   = nullptr;
                newInner->buckets.begin = nullptr;
                newInner->buckets.alloc = mgrAlloc;
                newInner->maxLoadFactor = 1.0f;

                DLBucketIter fillSentinel{0, 0, innerHead};
                DLBucketIter posZero{0, 0, nullptr};
                DLBucketIter dummyIter{};
                DLHashBucketVector_InsertFillImpl(
                    &dummyIter, &newInner->buckets, 0x10, &fillSentinel, posZero);
                newInner->bucketMask = 7;
                newInner->maxBuckets = 8;
            }

            // Allocate outer list node (0x20 bytes) and insert into outerTable
            DLHashListNode* firstNode = outerTable->listHead->next;
            DLHashListNode* prevNode  = firstNode->prev;
            DLAllocator* listAlloc    = outerTable->listAlloc;
            DLHashListNode* newOuterNode = static_cast<DLHashListNode*>(
                listAlloc->vptr->Allocate(listAlloc, 0x20, 8));
            if (!newOuterNode) {
                int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                    GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                if (*tlsFlag != 0) {
                    void* tlsObj = reinterpret_cast<void*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                    reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                } else {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                }
                return;
            }
            newOuterNode->next = firstNode ? firstNode : newOuterNode;
            if (!firstNode) {
                prevNode = newOuterNode;
            }
            newOuterNode->prev  = prevNode;
            newOuterNode->key   = bankId;
            newOuterNode->value = newInner;

            if (outerTable->listSize == 0x07fffffffffffffeULL) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                    reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            outerTable->listSize++;
            firstNode->prev = newOuterNode;
            newOuterNode->prev->next = newOuterNode;

            DLHashListNode* headNext = outerTable->listHead->next;
            DLBucketIter insertIter{0, 0, headNext};
            DLHashInsertResult insertOut{};
            DLHashTable_InsertNodeImpl(&insertOut, outerTable, &headNext->key, insertIter);
            bankNode = insertOut.node ? static_cast<DLHashListNode*>(insertOut.node) : headNext;
        }

        for (int32_t i = 0; i < paramCount; ++i) {
            uint32_t paramId = reinterpret_cast<FnQuerySceneParamIdAt>(GX_VMA(0x2184de0ULL))(
                sceneMgr, slotIdx, streamDesc, static_cast<uint32_t>(i));

            DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
            const uint64_t pBucket = ComputeDLHashBucket(
                paramId, innerTable->bucketMask, innerTable->maxBuckets);
            DLHashListNode* const innerHead = innerTable->listHead;
            DLHashListNode* foundParam = innerHead;
            DLHashListNode* pCur = static_cast<DLHashListNode*>(
                innerTable->buckets.begin[pBucket * 2].node);
            if (pCur != innerHead) {
                DLHashListNode* pEnd = static_cast<DLHashListNode*>(
                    innerTable->buckets.begin[pBucket * 2 + 1].node)->next;
                while (pCur != pEnd) {
                    if (pCur->key >= paramId) {
                        if (paramId >= pCur->key) {
                            foundParam = pCur;
                        }
                        break;
                    }
                    pCur = pCur->next;
                }
            }

            if (foundParam != innerHead) {
                uint8_t* existingEntry = static_cast<uint8_t*>(foundParam->value);
                existingEntry[0x128] = 1;
            } else {
                DLAllocator* mgrAlloc = *reinterpret_cast<DLAllocator**>(self);
                uint8_t* newEntry = static_cast<uint8_t*>(
                    mgrAlloc->vptr->Allocate(mgrAlloc, 0x138, 8));
                if (!newEntry) {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    continue;
                }
                reinterpret_cast<FnParamEntryConstruct>(GX_VMA(0xe8ad90ULL))(newEntry);
                reinterpret_cast<FnParamEntryInitDefaults>(GX_VMA(0xe8b150ULL))(newEntry);
                *reinterpret_cast<uint32_t*>(newEntry + 0x130) = paramId;
                *reinterpret_cast<uint32_t*>(newEntry + 0x134) = bankId;
                reinterpret_cast<FnParamEntryRegister>(GX_VMA(0xe8b4e0ULL))(newEntry, 0.0f);
                reinterpret_cast<FnParamEntryBindOwner>(GX_VMA(0xe8b650ULL))(
                    newEntry, *reinterpret_cast<void**>(self));

                DLHashListNode* pFirst = innerTable->listHead->next;
                DLHashListNode* pPrev  = pFirst->prev;
                DLAllocator* pAlloc    = innerTable->listAlloc;
                DLHashListNode* newParamNode = static_cast<DLHashListNode*>(
                    pAlloc->vptr->Allocate(pAlloc, 0x20, 8));
                if (!newParamNode) {
                    int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                    if (*tlsFlag != 0) {
                        void* tlsObj = reinterpret_cast<void*>(
                            GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                        reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                    } else {
                        reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                            0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    }
                    continue;
                }
                newParamNode->next = pFirst ? pFirst : newParamNode;
                if (!pFirst) {
                    pPrev = newParamNode;
                }
                newParamNode->prev  = pPrev;
                newParamNode->key   = paramId;
                newParamNode->value = newEntry;

                if (innerTable->listSize == 0x07fffffffffffffeULL) {
                    reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
                }
                innerTable->listSize++;
                pFirst->prev = newParamNode;
                newParamNode->prev->next = newParamNode;

                DLHashListNode* iHeadNext = innerTable->listHead->next;
                DLBucketIter pIter{0, 0, iHeadNext};
                DLHashInsertResult pInsertOut{};
                DLHashTable_InsertNodeImpl(&pInsertOut, innerTable, &iHeadNext->key, pIter);
            }
        }
    }

    reinterpret_cast<FnFinishSceneBankLoad>(GX_VMA(0x2184ff0ULL))(
        sceneMgr, slotIdx, bankId, loadFlags, streamDesc, blendWeight);
}

// ============================================================================
// [20 / 55] 0x2201740 (8663 B) — GXParamBankManagerA_RegisterYebisToneMapProperties
// ============================================================================
extern "C" uint8_t GXParamBankManagerA_RegisterYebisToneMapProperties(uint8_t* self) {
    GXParamBankManager_ClearAllBanksImpl(self);

    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    void*    propMgr    = *reinterpret_cast<void**>(sceneMgr + 0x28);

    uint32_t slot = 0;
    reinterpret_cast<FnPropMgrCreateBankGroup>(GX_VMA(0xe32510ULL))(
        propMgr,
        &slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7f62ULL)),  // "Tone Map"
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7f74ULL))); // "Tone Map[Yebis]"
    *reinterpret_cast<uint32_t*>(sceneMgr + 0x34) = slot;

    const uintptr_t fltVtbl0  = GX_VMA(0x52e83f0ULL) + 0x10ULL;
    const uintptr_t fltVtbl20 = GX_VMA(0x52e4890ULL) + 0x10ULL;
    const uintptr_t bolVtbl0  = GX_VMA(0x52e8760ULL) + 0x10ULL;
    const uintptr_t bolVtbl20 = GX_VMA(0x52e4a70ULL) + 0x10ULL;

    // Section: "Base Parameter" / "Yebis - Base Parameter"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7f94ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7fb2ULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x144,
        9, 0, 0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8cb2ULL), GX_VMA(0x49c7fe0ULL), 1); // "Exposure" / "Yebis-ToneMapExposure"

    // Section: "Detail Info" / "Yebis - ToneMap Detail Info"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49cb990ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c800cULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x148,
        9, 0, 0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c90aaULL), GX_VMA(0x49c8044ULL), 1); // "Gamma" / "Yebis-Gamma"

    // Enum: "ToneType" / "Yebis-ToneType" (+0x14c, 7 entries from 0x53601e0..0x5360248)
    alignas(16) uint8_t toneTypeEnumTable[112];
    std::memcpy(toneTypeEnumTable + 0x00, reinterpret_cast<const void*>(GX_VMA(0x53601e0ULL)), 16);
    std::memcpy(toneTypeEnumTable + 0x10, reinterpret_cast<const void*>(GX_VMA(0x53601f0ULL)), 16);
    std::memcpy(toneTypeEnumTable + 0x20, reinterpret_cast<const void*>(GX_VMA(0x5360200ULL)), 16);
    std::memcpy(toneTypeEnumTable + 0x30, reinterpret_cast<const void*>(GX_VMA(0x5360210ULL)), 16);
    std::memcpy(toneTypeEnumTable + 0x40, reinterpret_cast<const void*>(GX_VMA(0x5360220ULL)), 16);
    std::memcpy(toneTypeEnumTable + 0x50, reinterpret_cast<const void*>(GX_VMA(0x5360230ULL)), 16);
    std::memcpy(toneTypeEnumTable + 0x60, reinterpret_cast<const void*>(GX_VMA(0x5360240ULL)), 16);
    reinterpret_cast<FnPropMgrAddEnumProp>(GX_VMA(0x21e72a0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8130ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8142ULL)),
        self + 0x14c, 7, toneTypeEnumTable, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x150,
        9, 0, 0x3c23d70aU, 0x447a0000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8160ULL), GX_VMA(0x49c8184ULL), 1); // "ToneMappingFactor"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x154,
        9, 0, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c81b4ULL), GX_VMA(0x49c81d2ULL), 1); // "FilmicContrast"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x158,
        9, 0, 0x00000000U, 0x43b40000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c81fcULL), GX_VMA(0x49c820aULL), 1); // "ColorH"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x15c,
        9, 0, 0x00000000U, 0x40800000U, 0x38d1b717U, 0x3f800000U,
        GX_VMA(0x49c8224ULL), GX_VMA(0x49c8232ULL), 1); // "ColorS"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x160,
        9, 0, 0x00000000U, 0x40800000U, 0x38d1b717U, 0x3f800000U,
        GX_VMA(0x49c824cULL), GX_VMA(0x49c825aULL), 1); // "ColorV"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x164,
        9, 0, 0x00000000U, 0x40000000U, 0x38d1b717U, 0x3f800000U,
        GX_VMA(0x49c8274ULL), GX_VMA(0x49c8286ULL), 1); // "Contrust"

    // Section: "AutoExposure" / "Yebis - AutoExposure"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c91baULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c82a4ULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x168,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c82ceULL), 0); // "Enable" / "Yebis-AutoExposure"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x16c,
        9, 0, 0x00000000U, 0x447a0000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c82f4ULL), GX_VMA(0x49c830aULL), 1); // "DelayFrame"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x170,
        9, 0, 0x3a83126fU, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c832cULL), GX_VMA(0x49c8342ULL), 1); // "MiddleGray"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x174,
        9, 0, 0x3a83126fU, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8364ULL), GX_VMA(0x49c8390ULL), 1); // "ToneInfluencedByGlare"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x178,
        9, 0, 0x3a83126fU, 0x41f00000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c83c8ULL), GX_VMA(0x49c83e0ULL), 1); // "MinExposure"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x17c,
        9, 0, 0x3a83126fU, 0x41f00000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8404ULL), GX_VMA(0x49c841cULL), 1); // "MaxExposure"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x180,
        9, 0, 0x3a83126fU, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8440ULL), GX_VMA(0x49c846eULL), 1); // "Exposure MeteringWidth"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x184,
        9, 0, 0x3a83126fU, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c84a8ULL), GX_VMA(0x49c84d8ULL), 1); // "Exposure MeteringHeight"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x188,
        9, 0, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8514ULL), GX_VMA(0x49c8546ULL), 1); // "Exposure MeteringOffsetX"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x18c,
        9, 0, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8584ULL), GX_VMA(0x49c85b6ULL), 1); // "Exposure MeteringOffsetY"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x190,
        9, 0, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c85f4ULL), GX_VMA(0x49c8620ULL), 1); // "AdaptationSensitivity"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x194,
        9, 0, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8658ULL), GX_VMA(0x49c8678ULL), 1); // "AdaptationScale"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x198,
        9, 0, 0x38d1b717U, 0x42700000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c86a4ULL), GX_VMA(0x49c86ceULL), 1); // "AdaptationSpeedLimit"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x19c,
        9, 0, 0xbf800000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8704ULL), GX_VMA(0x49c8738ULL), 1); // "DarkAdaptationSensitivity"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1a0,
        9, 0, 0xbf800000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8778ULL), GX_VMA(0x49c87a0ULL), 1); // "DarkAdaptationScale"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1a4,
        9, 0, 0xbf800000U, 0x42700000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c87d4ULL), GX_VMA(0x49c8806ULL), 1); // "DarkAdaptationSpeedLimit"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1a8,
        9, 0, 0x3a83126fU, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8844ULL), GX_VMA(0x49c8894ULL), 1); // "fAdaptationScale / fDarkAdaptationScale"

    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);
    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);
    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);

    // Section: "Vignetting" / "Yebis - Vignetting"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c88f0ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8906ULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1ac,
        9, 0, 0xba83126fU, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c892cULL), GX_VMA(0x49c894aULL), 1); // "Vignette Power"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1b0,
        9, 0, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8974ULL), GX_VMA(0x49c89a0ULL), 1); // "VignetteFovDependence"

    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);

    // Section: "Chromatic Aberration" / "Yebis - Chromatic Aberration"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c89d8ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8a02ULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x1b4,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c8a3cULL), 0); // "Enable" / "Yebis-Chromatic Aberration Enable"

    // Enum: "WaveSample" / "Yebis-WaveSample" (+0x1b8, 3 entries: {3, 5, 7})
    alignas(16) DLPropertyEnumEntry waveSampleEnum[3] = {
        {reinterpret_cast<const char16_t*>(GX_VMA(0x49c8a80ULL)), 3U, 0U},
        {reinterpret_cast<const char16_t*>(GX_VMA(0x49c8a86ULL)), 5U, 0U},
        {reinterpret_cast<const char16_t*>(GX_VMA(0x49c8a8aULL)), 7U, 0U},
    };
    reinterpret_cast<FnPropMgrAddEnumProp>(GX_VMA(0x21e72a0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8a8eULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8aa4ULL)),
        self + 0x1b8, 3, waveSampleEnum, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1bc,
        9, 0, 0xbf800000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8ac6ULL), GX_VMA(0x49c8af8ULL), 1); // "ToneLateralDispersion[0]"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1c0,
        9, 0, 0xbf800000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8b36ULL), GX_VMA(0x49c8b68ULL), 1); // "ToneLateralDispersion[1]"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1c4,
        9, 0, 0xbf800000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8ba6ULL), GX_VMA(0x49c8bd8ULL), 1); // "ToneUniformDispersion[0]"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1c8,
        9, 0, 0xbf800000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8c16ULL), GX_VMA(0x49c8c48ULL), 1); // "ToneUniformDispersion[1]"

    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);

    // Section: "Tone Map" / "GS Presents - ToneMap"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7f62ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c8c86ULL)),
        0, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1d0,
        9, 0x18, 0xc1000000U, 0x41000000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8cb2ULL), GX_VMA(0x49c8cc4ULL), 1); // "Exposure" / "GS Presents - ToneMap-Exposure"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1d4,
        9, 0x18, 0x3c23d70aU, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8d02ULL), GX_VMA(0x49c8d1cULL), 1); // "AvgLuminance"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1d8,
        9, 0x18, 0x3f000000U, 0x40000000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8d62ULL), GX_VMA(0x49c8d88ULL), 1); // "LuminanceBalance R"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1dc,
        9, 0x18, 0x3f000000U, 0x40000000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8dd8ULL), GX_VMA(0x49c8dfeULL), 1); // "LuminanceBalance G"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1e0,
        9, 0x18, 0x3f000000U, 0x40000000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8e4eULL), GX_VMA(0x49c8e74ULL), 1); // "LuminanceBalance B"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1e8,
        9, 0x18, 0x3f666666U, 0x3f99999aU, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8ec4ULL), GX_VMA(0x49c8ed6ULL), 1); // "Scaler R"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1ec,
        9, 0x18, 0x3f666666U, 0x3f99999aU, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8f14ULL), GX_VMA(0x49c8f26ULL), 1); // "Scaler G"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1f0,
        9, 0x18, 0x3f666666U, 0x3f99999aU, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c8f64ULL), GX_VMA(0x49c8f76ULL), 1); // "Scaler B"

    // Enum: "ToneMapMode" / "GS Presents - ToneMap-ToneMapMode" (+0x1f4, 8 entries)
    alignas(16) uint8_t toneMapModeEnumTable[128];
    std::memcpy(toneMapModeEnumTable + 0x00, reinterpret_cast<const void*>(GX_VMA(0x5360250ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x10, reinterpret_cast<const void*>(GX_VMA(0x5360260ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x20, reinterpret_cast<const void*>(GX_VMA(0x5360270ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x30, reinterpret_cast<const void*>(GX_VMA(0x5360280ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x40, reinterpret_cast<const void*>(GX_VMA(0x5360290ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x50, reinterpret_cast<const void*>(GX_VMA(0x53602a0ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x60, reinterpret_cast<const void*>(GX_VMA(0x53602b0ULL)), 16);
    std::memcpy(toneMapModeEnumTable + 0x70, reinterpret_cast<const void*>(GX_VMA(0x53602c0ULL)), 16);
    reinterpret_cast<FnPropMgrAddEnumProp>(GX_VMA(0x21e72a0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c9010ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c9028ULL)),
        self + 0x1f4, 8, toneMapModeEnumTable, 0);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1f8,
        9, 0x18, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49d1a5eULL), GX_VMA(0x49c906cULL), 1); // "Contrast" / "GS Presents - ToneMap-Contrast"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x1fc,
        9, 0x18, 0x3c23d70aU, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c90aaULL), GX_VMA(0x49c90b6ULL), 1); // "Gamma" / "GS Presents - ToneMap-Gamma"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x200,
        3, 0x18, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c90eeULL), GX_VMA(0x49c9100ULL), 0); // "UseTable"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x204,
        3, 0x18, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c913eULL), GX_VMA(0x49c9166ULL), 0); // "Enable AutoExposure"

    // Section: "AutoExposure" / "GS Presents - AutoExposure"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c91baULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c91d4ULL)),
        0, 1);

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x208,
        9, 0x18, 0x3dcccccdU, 0x41200000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c9272ULL), GX_VMA(0x49c920aULL), 1); // "AutoExposure Speed" (LightAdapt)

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x20c,
        9, 0x18, 0x3dcccccdU, 0x41200000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c9272ULL), GX_VMA(0x49c9298ULL), 1); // "AutoExposure Speed" (DarkAdapt)

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x210,
        9, 0x18, 0xc1000000U, 0x41000000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c92feULL), GX_VMA(0x49c9330ULL), 1); // "AutoExposure Middle Gray"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x214,
        9, 0x18, 0x00000000U, 0x41200000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c938eULL), GX_VMA(0x49c93c4ULL), 1); // "AutoExposure Center Weight"

    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 0);
    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 0);

    reinterpret_cast<FnSceneBindBankPropsA>(GX_VMA(0x218e2b0ULL))(
        sceneMgr + 0x130, *reinterpret_cast<void**>(sceneMgr + 0x28), 0);
    reinterpret_cast<FnSceneBindBankPropsB>(GX_VMA(0xe41f00ULL))(
        sceneMgr + 0x1b8, *reinterpret_cast<void**>(sceneMgr + 0x28), 0, slot, 0, 0.0f);
    reinterpret_cast<FnSceneFinalizeBankSlot>(GX_VMA(0x21874c0ULL))(sceneMgr, slot);
    return 1;
}

// ============================================================================
// [21 / 55] 0x22015b0 (395 B) — GXParamBankManagerA_ClearAllBanks
// ============================================================================
extern "C" void GXParamBankManagerA_ClearAllBanks(uint8_t* self) {
    GXParamBankManager_ClearAllBanksImpl(self);
}

// ============================================================================
// [22 / 55] 0x22014b0 (255 B) — GXParamBankManagerA_Destruct
// ============================================================================
extern "C" void GXParamBankManagerA_Destruct(uint8_t* self) {
    GXParamBankManager_ClearAllBanksImpl(self);
    reinterpret_cast<FnParamEntryDestruct>(GX_VMA(0xe8b330ULL))(self + 0x2d8);

    uint64_t strCap = *reinterpret_cast<uint64_t*>(self + 0x248);
    if (strCap >= 8) {
        void* heapBuf = *reinterpret_cast<void**>(self + 0x230);
        if ((strCap + 1) >= 0x41ULL) {
            DLAllocator* sAlloc = *reinterpret_cast<DLAllocator**>(self + 0x250);
            if (sAlloc && heapBuf) {
                sAlloc->vptr->Free(sAlloc, heapBuf);
            }
        } else {
            self[0x2d1] &= 0xfdU;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0x248) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x240) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x230) = 0;

    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    if (outerTable->buckets.begin) {
        DLAllocator* bAlloc = outerTable->buckets.alloc;
        bAlloc->vptr->Free(bAlloc, outerTable->buckets.begin);
        outerTable->buckets.cap   = nullptr;
        outerTable->buckets.end   = nullptr;
        outerTable->buckets.begin = nullptr;
    }

    DLHashListNode* head = outerTable->listHead;
    DLHashListNode* cur  = head->next;
    head->next = head;
    outerTable->listHead->prev = outerTable->listHead;
    outerTable->listSize = 0;

    while (cur != outerTable->listHead) {
        DLHashListNode* next = cur->next;
        DLAllocator* lAlloc = outerTable->listAlloc;
        lAlloc->vptr->Free(lAlloc, cur);
        cur = next;
    }

    DLAllocator* lAlloc = outerTable->listAlloc;
    lAlloc->vptr->Free(lAlloc, outerTable->listHead);
}

// ============================================================================
// [23 / 55] 0x2200c00 (2209 B) — GXParamBankManagerA_Construct
// ============================================================================
extern "C" void GXParamBankManagerA_Construct(uint8_t* self, DLAllocator* alloc) {
    *reinterpret_cast<DLAllocator**>(self + 0x00) = alloc;

    uint8_t heapFlags[0x88] = {};
    uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(alloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(
        *reinterpret_cast<uintptr_t*>(vtbl + 0x20))(heapFlags, alloc, 0);
    if ((heapFlags[0] & 0x20) == 0) {
        reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    outerTable->listSize  = 0;
    outerTable->listHead  = nullptr;
    outerTable->listAlloc = alloc;

    DLHashListNode* headNode = static_cast<DLHashListNode*>(
        alloc->vptr->Allocate(alloc, 0x20, 8));
    if (headNode) {
        headNode->next = headNode;
        headNode->prev = headNode;
    } else {
        int32_t* tlsFlag = reinterpret_cast<int32_t*>(
            GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
        if (*tlsFlag != 0) {
            void* tlsObj = reinterpret_cast<void*>(
                GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
            reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
        } else {
            reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
        }
    }
    outerTable->listHead = headNode;

    outerTable->buckets.cap   = nullptr;
    outerTable->buckets.end   = nullptr;
    outerTable->buckets.begin = nullptr;
    outerTable->buckets.alloc = alloc;
    outerTable->maxLoadFactor = 1.0f;

    DLBucketIter fillSentinel{0, 0, headNode};
    DLBucketIter posZero{0, 0, nullptr};
    DLBucketIter dummyOut{};
    DLHashBucketVector_InsertFillImpl(
        &dummyOut, &outerTable->buckets, 0x10, &fillSentinel, posZero);
    outerTable->bucketMask = 7;
    outerTable->maxBuckets = 8;

    auto initToneMapParamBlock = [](uint8_t* blk) {
        *reinterpret_cast<uint32_t*>(blk + 0x24) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x3c) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x40) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x44) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x48) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x28) = 0x3e4ccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x4c) = 0x3f4ccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x50) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x54) = 0x42700000U;
        *reinterpret_cast<uint32_t*>(blk + 0x58) = 0x3dcccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x5c) = 0x3dcccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x60) = 0x3dcccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x64) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x00) = 0x40800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x04) = 0x400ccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x08) = 2;
        *reinterpret_cast<uint32_t*>(blk + 0x0c) = 0x42000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x10) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x70) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x74) = 3;
        *reinterpret_cast<uint32_t*>(blk + 0x14) = 0;
        *reinterpret_cast<uint64_t*>(blk + 0x80) = 0;
        *reinterpret_cast<uint64_t*>(blk + 0x78) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x18) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x1c) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x20) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x2c) = 0x3e3851ecU;
        *reinterpret_cast<uint32_t*>(blk + 0x30) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x34) = 0x3dcccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x38) = 0x41200000U;
        *reinterpret_cast<uint32_t*>(blk + 0x68) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x6c) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x88) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x8c) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x90) = 0x3fe66666U;
        *reinterpret_cast<uint32_t*>(blk + 0x94) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x98) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x9c) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0xa0) = 0x41200000U;
        *reinterpret_cast<uint32_t*>(blk + 0xa4) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0xa8) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0xac) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0xb0) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0xb4) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0xb8) = 0x400ccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0xbc) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0xc0) = 0;
        *reinterpret_cast<uint64_t*>(blk + 0xc4) = 0x3f0000003f800000ULL;
        *reinterpret_cast<uint64_t*>(blk + 0xcc) = 0x40800000c0000000ULL;
        *reinterpret_cast<uint64_t*>(blk + 0xd4) = 0x0000000040800000ULL;
    };

    initToneMapParamBlock(self + 0x68);
    reinterpret_cast<FnMemcpy>(GX_VMA(0x2bbe578ULL))(self + 0x144, self + 0x68, 0xdc);
    *reinterpret_cast<uint32_t*>(self + 0x220) = 0;

    // Embedded DLFixedBasicString<char16_t, 64> at self + 0x228
    reinterpret_cast<FnMemcpy>(GX_VMA(0x2bbe578ULL))(self + 0x250, heapFlags, 0x81);
    self[0x2d1] = 0;
    *reinterpret_cast<uint64_t*>(self + 0x248) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x240) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x230) = 0;
    self[0x2d2] = 1;

    reinterpret_cast<FnDLStringReserve>(GX_VMA(0x2935350ULL))(self + 0x228, 0x3f, 0);
    uint64_t curCap = *reinterpret_cast<uint64_t*>(self + 0x248);
    *reinterpret_cast<uint64_t*>(self + 0x240) = 0;
    uint16_t* strPtr = reinterpret_cast<uint16_t*>(self + 0x230);
    if (curCap >= 8) {
        strPtr = *reinterpret_cast<uint16_t**>(self + 0x230);
    }
    *strPtr = 0;

    // Default parameter entry at self + 0x2d8
    reinterpret_cast<FnParamEntryConstruct>(GX_VMA(0xe8ad90ULL))(self + 0x2d8);
    reinterpret_cast<FnParamEntryInitDefaults>(GX_VMA(0xe8b150ULL))(self + 0x2d8);

    reinterpret_cast<FnDLStringAssign>(GX_VMA(0x29354e0ULL))(
        self + 0x228, reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// [24 / 55] 0x22006b0 (1352 B) — DLHashBucketVector_InsertFill_InnerB
// ============================================================================
extern "C" DLBucketIter* DLHashBucketVector_InsertFill_InnerB(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    return DLHashBucketVector_InsertFillImpl(outIter, vec, count, fillVal, posIter);
}

// ============================================================================
// [25 / 55] 0x2200160 (1352 B) — DLHashBucketVector_InsertFill_OuterB
// ============================================================================
extern "C" DLBucketIter* DLHashBucketVector_InsertFill_OuterB(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    return DLHashBucketVector_InsertFillImpl(outIter, vec, count, fillVal, posIter);
}

// ============================================================================
// [26 / 55] 0x21ffd50 (1040 B) — DLHashTable_InsertNode_InnerB
// ============================================================================
extern "C" DLHashInsertResult* DLHashTable_InsertNode_InnerB(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    return DLHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// ============================================================================
// [27 / 55] 0x21ff940 (1040 B) — DLHashTable_InsertNode_OuterB
// ============================================================================
extern "C" DLHashInsertResult* DLHashTable_InsertNode_OuterB(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    return DLHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// ============================================================================
// [28 / 55] 0x21ff7d0 (363 B) — DLHashTable_EraseNode_InnerB
// ============================================================================
extern "C" DLBucketIter* DLHashTable_EraseNode_InnerB(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    return DLHashTable_EraseNodeImpl(outIter, table, eraseNode);
}

// ============================================================================
// [29 / 55] 0x21ff660 (363 B) — DLHashTable_EraseNode_OuterB
// ============================================================================
extern "C" DLBucketIter* DLHashTable_EraseNode_OuterB(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    return DLHashTable_EraseNodeImpl(outIter, table, eraseNode);
}

// ============================================================================
// [30 / 55] 0x21ff630 (40 B) — DLDebugGuiCallback_InvokeMethodB
// ============================================================================
extern "C" uint64_t DLDebugGuiCallback_InvokeMethodB(uint8_t* self) {
    return DLDebugGuiCallback_InvokeMethodA(self);
}

// ============================================================================
// [31 / 55] 0x21ff620 (5 B) — DLDebugGuiCallback_OperatorDeleteB
// ============================================================================
extern "C" void DLDebugGuiCallback_OperatorDeleteB(void* self) {
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// [32 / 55] 0x21ff600 (22 B) — DLDebugGuiCallback_DestroyBaseB
// ============================================================================
extern "C" void DLDebugGuiCallback_DestroyBaseB(uint8_t* self) {
    DLDebugGuiCallback_DestroyBaseA(self);
}

// ============================================================================
// [33 / 55] 0x21feec0 (1708 B) — GXParamBankManagerB_OpenParamEditListDialog
// ============================================================================
extern "C" void GXParamBankManagerB_OpenParamEditListDialog(uint8_t* self) {
    GXParamBankManager_OpenParamEditListDialogImpl(
        self,
        0x30,
        0x140,
        0x1f0,
        reinterpret_cast<FnParamEntryBuildGui>(GX_VMA(0xe84bb0ULL)));
}

// ============================================================================
// [34 / 55] 0x21fec50 (612 B) — GXParamBankManagerB_RegisterDebugMenu
// ============================================================================
extern "C" void GXParamBankManagerB_RegisterDebugMenu(uint8_t* self, void* parentMenu) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + 0x30);
    reinterpret_cast<FnRegisterSceneBank>(GX_VMA(0x21877f0ULL))(sceneMgr, parentMenu, slotIdx, 0);

    DLAllocator* cbAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));

    auto bindMenuAction = [&](const char16_t* labelWStr, uintptr_t methodAddr) {
        alignas(16) uint64_t emptyDesc[2] = {0, 0};
        void* item = reinterpret_cast<FnAddGuiActionItem>(GX_VMA(0x236d620ULL))(parentMenu, emptyDesc);

        alignas(16) uint64_t labelDesc[2] = {0, reinterpret_cast<uint64_t>(labelWStr)};
        uint8_t* cbObj = static_cast<uint8_t*>(cbAlloc->vptr->Allocate(cbAlloc, 0x30, 8));
        if (cbObj) {
            *reinterpret_cast<uint32_t*>(cbObj + 0x08)  = 0x40000000U;
            *reinterpret_cast<uintptr_t*>(cbObj + 0x00) = GX_VMA(0x53b7220ULL);
            *reinterpret_cast<void**>(cbObj + 0x10)     = self;
            *reinterpret_cast<uint32_t*>(cbObj + 0x18)  = 0;
            *reinterpret_cast<uintptr_t*>(cbObj + 0x20) = methodAddr;
            *reinterpret_cast<uint64_t*>(cbObj + 0x28)  = 0;
        }
        reinterpret_cast<FnBindGuiActionCallback>(GX_VMA(0x235e4c0ULL))(item, labelDesc, cbObj);
    };

    bindMenuAction(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL)), GX_VMA(0x21fe870ULL));
    bindMenuAction(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL)), GX_VMA(0x21fe900ULL));
    bindMenuAction(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL)), GX_VMA(0x21feec0ULL));
}

// ============================================================================
// [35 / 55] 0x21fe970 (732 B) — GXParamBankManagerB_ResolveParamEntry
// ============================================================================
extern "C" void* GXParamBankManagerB_ResolveParamEntry(uint8_t* self, const uint32_t* bankAndParamPair) {
    return GXParamBankManager_ResolveParamEntryImpl(self, bankAndParamPair, 0x1f0);
}

// ============================================================================
// [36 / 55] 0x21fe900 (103 B) — GXParamBankManagerB_UnregisterAllParams
// ============================================================================
extern "C" void GXParamBankManagerB_UnregisterAllParams(uint8_t* self) {
    DLHashListNode* outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    for (DLHashListNode* bankNode = outerHead->next; bankNode != outerHead; bankNode = bankNode->next) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
        DLHashListNode* innerHead = innerTable->listHead;
        for (DLHashListNode* paramNode = innerHead->next; paramNode != innerHead; paramNode = paramNode->next) {
            reinterpret_cast<FnParamEntryUnregister>(GX_VMA(0xe83aa0ULL))(paramNode->value);
        }
        outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    }
}

// ============================================================================
// [37 / 55] 0x21fe870 (138 B) — GXParamBankManagerB_RegisterAllParams
// ============================================================================
extern "C" void GXParamBankManagerB_RegisterAllParams(uint8_t* self) {
    DLHashListNode* outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    for (DLHashListNode* bankNode = outerHead->next; bankNode != outerHead; bankNode = bankNode->next) {
        DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
        DLHashListNode* innerHead = innerTable->listHead;
        for (DLHashListNode* paramNode = innerHead->next; paramNode != innerHead; paramNode = paramNode->next) {
            uint8_t* paramEntry = static_cast<uint8_t*>(paramNode->value);
            *reinterpret_cast<uint32_t*>(paramEntry + 0xb8) = paramNode->key;
            *reinterpret_cast<uint32_t*>(paramEntry + 0xbc) = bankNode->key;
            reinterpret_cast<FnParamEntryRegister>(GX_VMA(0xe83b00ULL))(paramEntry, 0.0f);
            reinterpret_cast<FnParamEntryBindOwner>(GX_VMA(0xe83c50ULL))(
                static_cast<uint8_t*>(paramNode->value),
                *reinterpret_cast<void**>(self));
        }
        outerHead = *reinterpret_cast<DLHashListNode**>(self + 0x10);
    }
}

// ============================================================================
// [38 / 55] 0x21fe860 (1 B) — GXParamBankManagerB_OnUpdateNoOp
// ============================================================================
extern "C" void GXParamBankManagerB_OnUpdateNoOp() {
    asm volatile("" ::: "memory");
}

// ============================================================================
// [39 / 55] 0x21fe830 (40 B) — GXParamBankManagerB_NotifyParamChanged
// ============================================================================
extern "C" uint64_t GXParamBankManagerB_NotifyParamChanged(void* self, uint32_t bankId, uint32_t paramId) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + 0x30);
    uintptr_t selfTag   = reinterpret_cast<uintptr_t>(self);
    return reinterpret_cast<FnDispatchSceneBankEvent>(GX_VMA(0x2186da0ULL))(
        sceneMgr, slotIdx, bankId, paramId, selfTag ? ~0ULL : ~0ULL);
}

// ============================================================================
// [40 / 55] 0x21fe570 (698 B) — GXParamBankManagerB_PruneInactiveParams
// ============================================================================
extern "C" void GXParamBankManagerB_PruneInactiveParams(
    uint8_t* self,
    void* unusedSlot,
    uint32_t bankId,
    uint32_t overrideSlotIdx,
    float blendTime)
{
    (void)unusedSlot;
    GXParamBankManager_PruneInactiveParamsImpl(self, bankId, overrideSlotIdx, blendTime, 0x30, 0xb0);
}

// ============================================================================
// [41 / 55] 0x21fddf0 (1912 B) — GXParamBankManagerB_LoadBankFromStream
// ============================================================================
extern "C" void GXParamBankManagerB_LoadBankFromStream(
    uint8_t* self,
    void* streamDesc,
    uint32_t bankId,
    uint32_t loadFlags,
    float blendWeight)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + 0x30);

    int32_t paramCount = reinterpret_cast<FnBeginSceneBankLoad>(GX_VMA(0x2184cf0ULL))(
        sceneMgr, slotIdx, streamDesc);
    if (paramCount != 0) {
        DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
        const uint64_t bBucket = ComputeDLHashBucket(
            bankId, outerTable->bucketMask, outerTable->maxBuckets);

        DLHashListNode* const outerHead = outerTable->listHead;
        DLBucketIter* const outerBuckets = outerTable->buckets.begin;
        DLHashListNode* bankNode = static_cast<DLHashListNode*>(outerBuckets[bBucket * 2].node);

        if (bankNode != outerHead) {
            DLHashListNode* bankEnd = static_cast<DLHashListNode*>(
                outerBuckets[bBucket * 2 + 1].node)->next;
            while (bankNode != bankEnd) {
                if (bankNode->key >= bankId) {
                    if (bankNode->key > bankId) {
                        bankNode = outerHead;
                    }
                    break;
                }
                bankNode = bankNode->next;
            }
        }

        if (bankNode == outerHead) {
            DLAllocator* mgrAlloc = *reinterpret_cast<DLAllocator**>(self);
            DLHashTable* newInner = static_cast<DLHashTable*>(
                mgrAlloc->vptr->Allocate(mgrAlloc, 0x60, 8));
            if (newInner) {
                uint8_t heapFlags[8] = {};
                uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(mgrAlloc);
                reinterpret_cast<void (*)(void*, void*, uint32_t)>(
                    *reinterpret_cast<uintptr_t*>(vtbl + 0x20))(heapFlags, mgrAlloc, 0);
                if ((heapFlags[0] & 0x20) == 0) {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
                }
                newInner->listSize  = 0;
                newInner->listHead  = nullptr;
                newInner->listAlloc = mgrAlloc;

                DLHashListNode* innerHead = static_cast<DLHashListNode*>(
                    mgrAlloc->vptr->Allocate(mgrAlloc, 0x20, 8));
                if (innerHead) {
                    innerHead->next = innerHead;
                    innerHead->prev = innerHead;
                } else {
                    int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                    if (*tlsFlag != 0) {
                        void* tlsObj = reinterpret_cast<void*>(
                            GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                        reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                    } else {
                        reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                            0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    }
                }
                newInner->listHead = innerHead;

                newInner->buckets.cap   = nullptr;
                newInner->buckets.end   = nullptr;
                newInner->buckets.begin = nullptr;
                newInner->buckets.alloc = mgrAlloc;
                newInner->maxLoadFactor = 1.0f;

                DLBucketIter fillSentinel{0, 0, innerHead};
                DLBucketIter posZero{0, 0, nullptr};
                DLBucketIter dummyIter{};
                DLHashBucketVector_InsertFillImpl(
                    &dummyIter, &newInner->buckets, 0x10, &fillSentinel, posZero);
                newInner->bucketMask = 7;
                newInner->maxBuckets = 8;
            }

            DLHashListNode* firstNode = outerTable->listHead->next;
            DLHashListNode* prevNode  = firstNode->prev;
            DLAllocator* listAlloc    = outerTable->listAlloc;
            DLHashListNode* newOuterNode = static_cast<DLHashListNode*>(
                listAlloc->vptr->Allocate(listAlloc, 0x20, 8));
            if (!newOuterNode) {
                int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                    GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                if (*tlsFlag != 0) {
                    void* tlsObj = reinterpret_cast<void*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                    reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                } else {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                }
                return;
            }
            newOuterNode->next = firstNode ? firstNode : newOuterNode;
            if (!firstNode) {
                prevNode = newOuterNode;
            }
            newOuterNode->prev  = prevNode;
            newOuterNode->key   = bankId;
            newOuterNode->value = newInner;

            if (outerTable->listSize == 0x07fffffffffffffeULL) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                    reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            outerTable->listSize++;
            firstNode->prev = newOuterNode;
            newOuterNode->prev->next = newOuterNode;

            DLHashListNode* headNext = outerTable->listHead->next;
            DLBucketIter insertIter{0, 0, headNext};
            DLHashInsertResult insertOut{};
            DLHashTable_InsertNodeImpl(&insertOut, outerTable, &headNext->key, insertIter);
            bankNode = insertOut.node ? static_cast<DLHashListNode*>(insertOut.node) : headNext;
        }

        for (int32_t i = 0; i < paramCount; ++i) {
            uint32_t paramId = reinterpret_cast<FnQuerySceneParamIdAt>(GX_VMA(0x2184de0ULL))(
                sceneMgr, slotIdx, streamDesc, static_cast<uint32_t>(i));

            DLHashTable* innerTable = static_cast<DLHashTable*>(bankNode->value);
            const uint64_t pBucket = ComputeDLHashBucket(
                paramId, innerTable->bucketMask, innerTable->maxBuckets);
            DLHashListNode* const innerHead = innerTable->listHead;
            DLHashListNode* foundParam = innerHead;
            DLHashListNode* pCur = static_cast<DLHashListNode*>(
                innerTable->buckets.begin[pBucket * 2].node);
            if (pCur != innerHead) {
                DLHashListNode* pEnd = static_cast<DLHashListNode*>(
                    innerTable->buckets.begin[pBucket * 2 + 1].node)->next;
                while (pCur != pEnd) {
                    if (pCur->key >= paramId) {
                        if (paramId >= pCur->key) {
                            foundParam = pCur;
                        }
                        break;
                    }
                    pCur = pCur->next;
                }
            }

            if (foundParam != innerHead) {
                uint8_t* existingEntry = static_cast<uint8_t*>(foundParam->value);
                existingEntry[0xb0] = 1;
            } else {
                DLAllocator* mgrAlloc = *reinterpret_cast<DLAllocator**>(self);
                uint8_t* newEntry = static_cast<uint8_t*>(
                    mgrAlloc->vptr->Allocate(mgrAlloc, 0xc0, 8));
                if (!newEntry) {
                    reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                        0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    continue;
                }
                reinterpret_cast<FnParamEntryConstruct>(GX_VMA(0xe836f0ULL))(newEntry);
                reinterpret_cast<FnParamEntryInitDefaults>(GX_VMA(0xe83860ULL))(newEntry);
                reinterpret_cast<FnParamEntryBindOwner>(GX_VMA(0xe83c50ULL))(
                    newEntry, *reinterpret_cast<void**>(self));
                *reinterpret_cast<uint32_t*>(newEntry + 0xb8) = paramId;
                *reinterpret_cast<uint32_t*>(newEntry + 0xbc) = bankId;
                reinterpret_cast<FnParamEntryRegister>(GX_VMA(0xe83b00ULL))(newEntry, 0.0f);

                DLHashListNode* pFirst = innerTable->listHead->next;
                DLHashListNode* pPrev  = pFirst->prev;
                DLAllocator* pAlloc    = innerTable->listAlloc;
                DLHashListNode* newParamNode = static_cast<DLHashListNode*>(
                    pAlloc->vptr->Allocate(pAlloc, 0x20, 8));
                if (!newParamNode) {
                    int32_t* tlsFlag = reinterpret_cast<int32_t*>(
                        GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
                    if (*tlsFlag != 0) {
                        void* tlsObj = reinterpret_cast<void*>(
                            GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
                        reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
                    } else {
                        reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                            0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    }
                    continue;
                }
                newParamNode->next = pFirst ? pFirst : newParamNode;
                if (!pFirst) {
                    pPrev = newParamNode;
                }
                newParamNode->prev  = pPrev;
                newParamNode->key   = paramId;
                newParamNode->value = newEntry;

                if (innerTable->listSize == 0x07fffffffffffffeULL) {
                    reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
                }
                innerTable->listSize++;
                pFirst->prev = newParamNode;
                newParamNode->prev->next = newParamNode;

                DLHashListNode* iHeadNext = innerTable->listHead->next;
                DLBucketIter pIter{0, 0, iHeadNext};
                DLHashInsertResult pInsertOut{};
                DLHashTable_InsertNodeImpl(&pInsertOut, innerTable, &iHeadNext->key, pIter);
            }
        }
    }

    reinterpret_cast<FnFinishSceneBankLoad>(GX_VMA(0x2184ff0ULL))(
        sceneMgr, slotIdx, bankId, loadFlags, streamDesc, blendWeight);
}

// ============================================================================
// [42 / 55] 0x21fcf20 (3792 B) — GXParamBankManagerB_RegisterYebisSettingInfoProperties
// ============================================================================
extern "C" uint8_t GXParamBankManagerB_RegisterYebisSettingInfoProperties(uint8_t* self) {
    GXParamBankManager_ClearAllBanksImpl(self);

    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    void*    propMgr    = *reinterpret_cast<void**>(sceneMgr + 0x28);

    uint32_t slot = 0;
    reinterpret_cast<FnPropMgrCreateBankGroup>(GX_VMA(0xe32510ULL))(
        propMgr,
        &slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7410ULL)),  // "Setting Info"
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c73e8ULL))); // "Setting Info[Yebis]"
    *reinterpret_cast<uint32_t*>(sceneMgr + 0x30) = slot;

    const uintptr_t bolVtbl0  = GX_VMA(0x52e8760ULL) + 0x10ULL;
    const uintptr_t bolVtbl20 = GX_VMA(0x52e4a70ULL) + 0x10ULL;
    const uintptr_t fltVtbl0  = GX_VMA(0x52e83f0ULL) + 0x10ULL;
    const uintptr_t fltVtbl20 = GX_VMA(0x52e4890ULL) + 0x10ULL;
    const uintptr_t u8Vtbl0   = GX_VMA(0x52e8850ULL) + 0x10ULL;
    const uintptr_t u8Vtbl20  = GX_VMA(0x52e49d0ULL) + 0x10ULL;

    // Section: "Setting Info" / "Yebis - SettingInfo"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7410ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c742aULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0d0,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c7452ULL), GX_VMA(0x49c7474ULL), 0); // "Use DepthTexture"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0d4,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c74a2ULL), GX_VMA(0x49c74ceULL), 0); // "Enable LensDistortion"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0d8,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c7506ULL), GX_VMA(0x49c7536ULL), 0); // "Enable ApertureairyDisc"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0dc,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c7572ULL), GX_VMA(0x49c75b8ULL), 0); // "Enable ApertureairyUnlimitedRadius"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0e0,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c760aULL), GX_VMA(0x49c7636ULL), 0); // "Enable MotionBlur-Dof"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0e4,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c766eULL), GX_VMA(0x49c76b6ULL), 0); // "Enable MotionBlurUnlimitedrecursion"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0e8,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c770aULL), GX_VMA(0x49c7754ULL), 0); // "Create MotionBlurInternalVelocityTex"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0ec,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c77aaULL), GX_VMA(0x49c77caULL), 0); // "Enable Gaussian"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0f0,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c77f6ULL), GX_VMA(0x49c7822ULL), 0); // "Enable FeedBackEffect"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0f4,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c785aULL), GX_VMA(0x49c787eULL), 0); // "Enable Anti-Alias"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0f8,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c78b6ULL), GX_VMA(0x49c78daULL), 0); // "Enable LightShaft"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x0fc,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c790aULL), GX_VMA(0x49c7942ULL), 0); // "Enable Choromaticaberration"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x100,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c7986ULL), GX_VMA(0x49c79c6ULL), 0); // "Enable AnamophicLensAspectratio"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x104,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c7a12ULL), GX_VMA(0x49c7a4eULL), 0); // "Enable AnamorphicLensAperture"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x108,
        3, 1, 0U, 1U, 0U, 0x3f800000U,
        GX_VMA(0x49c7a96ULL), GX_VMA(0x49c7abeULL), 0); // "Enable ColorGrading"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x120,
        9, 0, 0x3dcccccdU, 0x41200000U, 0x3c23d70aU, 0x3f800000U,
        GX_VMA(0x49c7af2ULL), GX_VMA(0x49c7b1aULL), 1); // "SceneLuminanceScale"

    // Enum: "BlurShape" / "Yebis-BlurShape" (+0x10c, 5 entries from 0x5360190..0x53601d8)
    alignas(16) uint8_t blurShapeEnumTable[80];
    std::memcpy(blurShapeEnumTable + 0x00, reinterpret_cast<const void*>(GX_VMA(0x5360190ULL)), 16);
    std::memcpy(blurShapeEnumTable + 0x10, reinterpret_cast<const void*>(GX_VMA(0x53601a0ULL)), 16);
    std::memcpy(blurShapeEnumTable + 0x20, reinterpret_cast<const void*>(GX_VMA(0x53601b0ULL)), 16);
    std::memcpy(blurShapeEnumTable + 0x30, reinterpret_cast<const void*>(GX_VMA(0x53601c0ULL)), 16);
    std::memcpy(blurShapeEnumTable + 0x40, reinterpret_cast<const void*>(GX_VMA(0x53601d0ULL)), 16);
    reinterpret_cast<FnPropMgrAddEnumProp>(GX_VMA(0x29358f0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7bb2ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7bc6ULL)),
        self + 0x10c, 5, blurShapeEnumTable, 1);

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x110,
        3, 0, 0U, 7U, 1U, 0x3f800000U,
        GX_VMA(0x49c7be6ULL), GX_VMA(0x49c7c00ULL), 1); // "GlareQuality"

    RegisterScalarPropHelper(
        propMgr, slot, bolVtbl0, bolVtbl20, self + 0x114,
        3, 0, 0U, 12U, 1U, 0x3f800000U,
        GX_VMA(0x49c7c2eULL), GX_VMA(0x49c7c56ULL), 1); // "DepthOfFieldQuality"

    // Section: "Auto Dist Compute Param" / "SettingInfo-LightCollect-Auto Dist Compute Param"
    reinterpret_cast<FnPropMgrBeginSection>(GX_VMA(0xe333e0ULL))(
        propMgr, slot,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7c8aULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c7cbaULL)),
        1, 1);

    RegisterScalarPropHelper(
        propMgr, slot, u8Vtbl0, u8Vtbl20, self + 0x134,
        5, 1, 0x00010100U, 0x3f800000U, 0U, 0U,
        GX_VMA(0x49c7d1cULL), GX_VMA(0x49c7d44ULL), 0); // "Auto Compute Enable"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x128,
        9, 0, 0xc2c80000U, 0x42c80000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c7da6ULL), GX_VMA(0x49c7db8ULL), 1); // "BaseDist"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x12c,
        9, 0, 0x00000000U, 0x42c80000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c7e04ULL), GX_VMA(0x49c7e1aULL), 1); // "DistScaler"

    RegisterScalarPropHelper(
        propMgr, slot, fltVtbl0, fltVtbl20, self + 0x130,
        9, 0, 0xbf800000U, 0x47c35000U, 0x3dcccccdU, 0x3f800000U,
        GX_VMA(0x49c7e68ULL), GX_VMA(0x49c7e78ULL), 1); // "DistMax"

    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);

    RegisterScalarPropHelper(
        propMgr, slot, u8Vtbl0, u8Vtbl20, self + 0x135,
        5, 1, 0x00010100U, 0x3f800000U, 0U, 0U,
        GX_VMA(0x49c7ec0ULL), GX_VMA(0x49c7f02ULL), 0); // "ReflectScene Assign DynamicLight"

    reinterpret_cast<FnPropMgrEndSection>(GX_VMA(0xe33600ULL))(propMgr, slot, 1);

    reinterpret_cast<FnSceneBindBankPropsA>(GX_VMA(0x218e2b0ULL))(
        sceneMgr + 0x130, *reinterpret_cast<void**>(sceneMgr + 0x28), 0);
    reinterpret_cast<FnSceneBindBankPropsB>(GX_VMA(0xe41f00ULL))(
        sceneMgr + 0x1b8, *reinterpret_cast<void**>(sceneMgr + 0x28), 0, slot, 0, 0.0f);
    reinterpret_cast<FnSceneFinalizeBankSlot>(GX_VMA(0x21874c0ULL))(sceneMgr, slot);
    return 1;
}

// ============================================================================
// [43 / 55] 0x21fcd90 (395 B) — GXParamBankManagerB_ClearAllBanks
// ============================================================================
extern "C" void GXParamBankManagerB_ClearAllBanks(uint8_t* self) {
    GXParamBankManager_ClearAllBanksImpl(self);
}

// ============================================================================
// [44 / 55] 0x21fcc90 (255 B) — GXParamBankManagerB_Destruct
// ============================================================================
extern "C" void GXParamBankManagerB_Destruct(uint8_t* self) {
    GXParamBankManager_ClearAllBanksImpl(self);
    reinterpret_cast<FnParamEntryDestruct>(GX_VMA(0xe83950ULL))(self + 0x1f0);

    uint64_t strCap = *reinterpret_cast<uint64_t*>(self + 0x160);
    if (strCap >= 8) {
        void* heapBuf = *reinterpret_cast<void**>(self + 0x148);
        if ((strCap + 1) >= 0x41ULL) {
            DLAllocator* sAlloc = *reinterpret_cast<DLAllocator**>(self + 0x168);
            if (sAlloc && heapBuf) {
                sAlloc->vptr->Free(sAlloc, heapBuf);
            }
        } else {
            self[0x1e9] &= 0xfdU;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0x160) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x158) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x148) = 0;

    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    if (outerTable->buckets.begin) {
        DLAllocator* bAlloc = outerTable->buckets.alloc;
        bAlloc->vptr->Free(bAlloc, outerTable->buckets.begin);
        outerTable->buckets.cap   = nullptr;
        outerTable->buckets.end   = nullptr;
        outerTable->buckets.begin = nullptr;
    }

    DLHashListNode* head = outerTable->listHead;
    DLHashListNode* cur  = head->next;
    head->next = head;
    outerTable->listHead->prev = outerTable->listHead;
    outerTable->listSize = 0;

    while (cur != outerTable->listHead) {
        DLHashListNode* next = cur->next;
        DLAllocator* lAlloc = outerTable->listAlloc;
        lAlloc->vptr->Free(lAlloc, cur);
        cur = next;
    }

    DLAllocator* lAlloc = outerTable->listAlloc;
    lAlloc->vptr->Free(lAlloc, outerTable->listHead);
}

// ============================================================================
// [45 / 55] 0x21fc6f0 (1434 B) — GXParamBankManagerB_Construct
// ============================================================================
extern "C" void GXParamBankManagerB_Construct(uint8_t* self, DLAllocator* alloc) {
    *reinterpret_cast<DLAllocator**>(self + 0x00) = alloc;

    uint8_t heapFlags[0x88] = {};
    uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(alloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(
        *reinterpret_cast<uintptr_t*>(vtbl + 0x20))(heapFlags, alloc, 0);
    if ((heapFlags[0] & 0x20) == 0) {
        reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    DLHashTable* outerTable = reinterpret_cast<DLHashTable*>(self + 0x08);
    outerTable->listSize  = 0;
    outerTable->listHead  = nullptr;
    outerTable->listAlloc = alloc;

    DLHashListNode* headNode = static_cast<DLHashListNode*>(
        alloc->vptr->Allocate(alloc, 0x20, 8));
    if (headNode) {
        headNode->next = headNode;
        headNode->prev = headNode;
    } else {
        int32_t* tlsFlag = reinterpret_cast<int32_t*>(
            GetThreadLocalBase() + gx_guest_tls_offset(0x53e4318ULL));
        if (*tlsFlag != 0) {
            void* tlsObj = reinterpret_cast<void*>(
                GetThreadLocalBase() + gx_guest_tls_offset(0x53e4320ULL));
            reinterpret_cast<FnHandleAllocFailure>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
        } else {
            reinterpret_cast<FnLogIncompatibleHeap>(GX_VMA(0x20b55b0ULL))(
                0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
        }
    }
    outerTable->listHead = headNode;

    outerTable->buckets.cap   = nullptr;
    outerTable->buckets.end   = nullptr;
    outerTable->buckets.begin = nullptr;
    outerTable->buckets.alloc = alloc;
    outerTable->maxLoadFactor = 1.0f;

    DLBucketIter fillSentinel{0, 0, headNode};
    DLBucketIter posZero{0, 0, nullptr};
    DLBucketIter dummyOut{};
    DLHashBucketVector_InsertFillImpl(
        &dummyOut, &outerTable->buckets, 0x10, &fillSentinel, posZero);
    outerTable->bucketMask = 7;
    outerTable->maxBuckets = 8;

    auto initSettingInfoBlock = [](uint8_t* blk) {
        *reinterpret_cast<uint32_t*>(blk + 0x00) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x04) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x08) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x0c) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x10) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x14) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x18) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x1c) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x20) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x48) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x24) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x28) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x2c) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x4c) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x30) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x34) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x40) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x44) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x3c) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x38) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x50) = 0x400ccccdU;
        *reinterpret_cast<uint32_t*>(blk + 0x54) = 0xbf800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x58) = 0x41200000U;
        *reinterpret_cast<uint32_t*>(blk + 0x5c) = 0x41200000U;
        *reinterpret_cast<uint32_t*>(blk + 0x60) = 0x43020000U;
        *reinterpret_cast<uint16_t*>(blk + 0x64) = 0;
    };

    initSettingInfoBlock(self + 0x68);
    std::memcpy(self + 0xd0, self + 0x68, 0x66);
    *reinterpret_cast<uint32_t*>(self + 0x138) = 0;

    // Embedded DLFixedBasicString<char16_t, 64> at self + 0x140
    reinterpret_cast<FnMemcpy>(GX_VMA(0x2bbe578ULL))(self + 0x168, heapFlags, 0x81);
    self[0x1e9] = 0;
    *reinterpret_cast<uint64_t*>(self + 0x160) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x158) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x148) = 0;
    self[0x1ea] = 1;

    reinterpret_cast<FnDLStringReserve>(GX_VMA(0x2935350ULL))(self + 0x140, 0x3f, 0);
    uint64_t curCap = *reinterpret_cast<uint64_t*>(self + 0x160);
    *reinterpret_cast<uint64_t*>(self + 0x158) = 0;
    uint16_t* strPtr = reinterpret_cast<uint16_t*>(self + 0x148);
    if (curCap >= 8) {
        strPtr = *reinterpret_cast<uint16_t**>(self + 0x148);
    }
    *strPtr = 0;

    // Default parameter entry at self + 0x1f0
    reinterpret_cast<FnParamEntryConstruct>(GX_VMA(0xe836f0ULL))(self + 0x1f0);
    reinterpret_cast<FnParamEntryInitDefaults>(GX_VMA(0xe83860ULL))(self + 0x1f0);

    reinterpret_cast<FnDLStringAssign>(GX_VMA(0x29354e0ULL))(
        self + 0x140, reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// [46 / 55] 0x21fc1a0 (1352 B) — DLHashBucketVector_InsertFill_InnerC
// ============================================================================
extern "C" DLBucketIter* DLHashBucketVector_InsertFill_InnerC(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    return DLHashBucketVector_InsertFillImpl(outIter, vec, count, fillVal, posIter);
}

// ============================================================================
// [47 / 55] 0x21fbc50 (1352 B) — DLHashBucketVector_InsertFill_OuterC
// ============================================================================
extern "C" DLBucketIter* DLHashBucketVector_InsertFill_OuterC(
    DLBucketIter* outIter,
    DLHashBucketVector* vec,
    uint64_t count,
    const DLBucketIter* fillVal,
    DLBucketIter posIter)
{
    return DLHashBucketVector_InsertFillImpl(outIter, vec, count, fillVal, posIter);
}

// ============================================================================
// [48 / 55] 0x21fb840 (1040 B) — DLHashTable_InsertNode_InnerC
// ============================================================================
extern "C" DLHashInsertResult* DLHashTable_InsertNode_InnerC(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    return DLHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// ============================================================================
// [49 / 55] 0x21fb430 (1040 B) — DLHashTable_InsertNode_OuterC
// ============================================================================
extern "C" DLHashInsertResult* DLHashTable_InsertNode_OuterC(
    DLHashInsertResult* outRes,
    DLHashTable* table,
    const uint32_t* keyPtr,
    DLBucketIter nodeIter)
{
    return DLHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// ============================================================================
// [50 / 55] 0x21fb2c0 (363 B) — DLHashTable_EraseNode_InnerC
// ============================================================================
extern "C" DLBucketIter* DLHashTable_EraseNode_InnerC(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    return DLHashTable_EraseNodeImpl(outIter, table, eraseNode);
}

// ============================================================================
// [51 / 55] 0x21fb150 (363 B) — DLHashTable_EraseNode_OuterC
// ============================================================================
extern "C" DLBucketIter* DLHashTable_EraseNode_OuterC(
    DLBucketIter* outIter,
    DLHashTable* table,
    DLHashListNode* eraseNode)
{
    return DLHashTable_EraseNodeImpl(outIter, table, eraseNode);
}

// ============================================================================
// [52 / 55] 0x21fb120 (40 B) — DLDebugGuiCallback_InvokeMethodC
// ============================================================================
extern "C" uint64_t DLDebugGuiCallback_InvokeMethodC(uint8_t* self) {
    return DLDebugGuiCallback_InvokeMethodA(self);
}

// ============================================================================
// [53 / 55] 0x21fb110 (5 B) — DLDebugGuiCallback_OperatorDeleteC
// ============================================================================
extern "C" void DLDebugGuiCallback_OperatorDeleteC(void* self) {
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// [54 / 55] 0x21fb0f0 (22 B) — DLDebugGuiCallback_DestroyBaseC
// ============================================================================
extern "C" void DLDebugGuiCallback_DestroyBaseC(uint8_t* self) {
    DLDebugGuiCallback_DestroyBaseA(self);
}

// ============================================================================
// [55 / 55] 0x21fa9b0 (1708 B) — GXParamBankManagerC_OpenParamEditListDialog
// ============================================================================
extern "C" void GXParamBankManagerC_OpenParamEditListDialog(uint8_t* self) {
    GXParamBankManager_OpenParamEditListDialogImpl(
        self,
        0x48,
        0x0e0,
        0x190,
        reinterpret_cast<FnParamEntryBuildGui>(GX_VMA(0xe83020ULL)));
}
