#include <cstdint>
#include <cstring>
#include <immintrin.h>

// ============================================================================
// Module 04: GXYebisMotionBlurAndLightShaftBanks.cpp (0x21f18c0 .. 0x21fa740, 55 functions)
// Strictly Right-to-Left (highest VMA to lowest VMA), 100% logic, 0 stubs.
// Covers:
//   - Group C (0x21f8b40 .. 0x21fa740, 12 functions):
//     MotionBlur[Yebis] Parameter Bank Manager (remainder from Batch 2)
//   - Group D (0x21f4e20 .. 0x21f85f0, 22 functions):
//     LightShaft[Yebis] Parameter Bank Manager
//   - Group E (0x21f18c0 .. 0x21f48d0, 21 functions):
//     FeedBackBlur[Yebis] Parameter Bank Manager
// ============================================================================

namespace {

struct DLAllocatorVtable {
    void* (*dtor)(void* self);
    void* fn_08;
    void* fn_10;
    void* fn_18;
    void  (*getHeapCaps)(void* outCaps, void* self, uint32_t flags);
    void* fn_28;
    void* fn_30;
    void* fn_38;
    void* fn_40;
    void* fn_48;
    void* fn_50;
    void* (*allocateAligned)(void* self, size_t size, size_t alignment);
    void* fn_60;
    void* fn_68;
    void  (*freeBlock)(void* self, void* ptr);
};

struct DLAllocator {
    DLAllocatorVtable* vptr;
};

struct DLVirtualObjVtable {
    void (*dtor)(void* self);
    void (*deleteSelf)(void* self);
};

struct DLVirtualObj {
    DLVirtualObjVtable* vptr;
};

struct ParamHashNode {
    ParamHashNode* next;
    ParamHashNode* prev;
    uint32_t       key;
    uint32_t       pad14;
    void*          value;
};

struct ParamBucketIter {
    uint64_t       proxy0;    // +0x00
    uint64_t       proxy1;    // +0x08
    ParamHashNode* node;      // +0x10
};

struct ParamHashBucket {
    uint64_t       proxy0;    // +0x00
    uint64_t       proxy1;    // +0x08
    ParamHashNode* beginNode; // +0x10
    uint64_t       proxy2;    // +0x18
    uint64_t       proxy3;    // +0x20
    ParamHashNode* endNode;   // +0x28
};

struct ParamInnerHashTable {
    uint64_t         unk00;
    ParamHashNode*   sentinel;      // +0x08
    uint64_t         count;         // +0x10
    DLAllocator*     nodeAlloc;     // +0x18
    uint64_t         unk20;         // +0x20
    ParamHashBucket* buckets;       // +0x28
    uint64_t         bucketCapBytes;// +0x30
    uint64_t         bucketEndOff;  // +0x38
    DLAllocator*     bucketAlloc;   // +0x40
    uint64_t         mask;          // +0x48
    uint64_t         maxBucketIdx;  // +0x50
    float            maxLoadFactor; // +0x58
    uint32_t         pad5C;
};

struct ParamOuterHashTable {
    DLAllocator*     ownerAlloc;    // +0x00
    uint64_t         unk08;         // +0x08
    ParamHashNode*   sentinel;      // +0x10
    uint64_t         count;         // +0x18
    DLAllocator*     nodeAlloc;     // +0x20
    uint64_t         unk28;         // +0x28
    ParamHashBucket* buckets;       // +0x30
    uint64_t         bucketCapBytes;// +0x38
    uint64_t         bucketEndOff;  // +0x40
    DLAllocator*     bucketAlloc;   // +0x48
    uint64_t         mask;          // +0x50
    uint64_t         maxBucketIdx;  // +0x58
    float            maxLoadFactor; // +0x60
    uint32_t         pad64;
};

struct ParamIterPair {
    uint64_t       proxy0;    // +0x00
    uint64_t       proxy1;    // +0x08
    ParamHashNode* node;      // +0x10
    uint8_t        inserted;  // +0x18
    uint8_t        pad[7];    // +0x19..0x1f
};

struct ParamGuiPropDesc {
    void*    vtablePrimary;   // +0x00
    void*    targetFieldPtr;  // +0x08
    uint8_t  propType;        // +0x10
    uint8_t  pad11[3];
    uint32_t propSubType;     // +0x14
    uint8_t  propFlags;       // +0x18
    uint8_t  pad19[7];
    void*    vtableSecondary; // +0x20
    uint32_t minValBits;      // +0x28
    uint32_t maxValBits;      // +0x2c
    uint32_t stepValBits;     // +0x30
    uint32_t multValBits;     // +0x34
};

// External engine helpers resolved by VMA
using FnDivMod64 = uint64_t (*)(uint64_t dividend, uint64_t divisor, uint64_t* outRemainder);
static inline uint64_t CallDivMod64(uint64_t a, uint64_t b, uint64_t* rem) {
    uint64_t q = a / b;
    *rem = a % b;
    return q;
}

static inline uint64_t ComputeLehmerHashBucket(uint32_t key, uint32_t xorConst, uint64_t mask, uint64_t maxBucketIdx) {
    uint64_t seed = static_cast<uint64_t>(xorConst) ^ static_cast<uint64_t>(key);
    uint64_t rem = 0;
    uint64_t quot = CallDivMod64(seed, 0x1f31dULL, &rem);
    int64_t prod1 = static_cast<int64_t>(rem) * 0x41a7LL;
    int64_t prod2 = static_cast<int64_t>(quot) * -2836LL; // 0xfffffffffffff4ec
    int64_t diff = prod1 + prod2;
    uint64_t h = (diff < 0) ? (static_cast<uint64_t>(diff) + 0x7fffffffffffffffULL) : static_cast<uint64_t>(diff);
    uint64_t idx = h & mask;
    if (maxBucketIdx <= idx) {
        idx += ~(mask >> 1);
    }
    return idx;
}

using FnLogAssert          = void (*)(uint32_t level, uint32_t line, const char* msg, ...);
using FnFileLineAssert     = void (*)(const char* file, uint32_t line, const char* msg, ...);
using FnThrowLengthError   = void (*)(const char* msg);
using FnSetTlsErrorFlag    = void (*)(void* tlsSlot, uint32_t val);
using FnMemset             = void* (*)(void* dst, int val, size_t len);
using FnMemmove            = void* (*)(void* dst, const void* src, size_t len);
using FnFormatWideStr      = int (*)(void* dstStrObj, const char16_t* fmt, ...);
using FnResizeWideStr      = void (*)(void* dstStrObj, size_t cap, char16_t fillCh);
using FnAssignWideStr      = void (*)(void* dstStrObj, const char16_t* src, size_t len);

using FnSceneGetBankCount  = uint32_t (*)(void* sceneSys, uint32_t slotId, void* streamPtr);
using FnSceneGetParamIdAt  = uint32_t (*)(void* sceneSys, uint32_t slotId, void* streamPtr, uint32_t index);
using FnSceneFinalizeLoad  = void (*)(void* sceneSys, uint32_t slotId, uint32_t bankId, uint32_t flags, void* streamPtr, float weight);
using FnSceneBeginUnload   = void (*)(void* sceneSys, uint32_t slotId, uint32_t bankId, uint32_t overrideSlotIdx, float blendTime);
using FnSceneCheckEntry    = bool (*)(void* sceneSys, uint32_t slotId, uint32_t paramId, uint32_t bankId);
using FnSceneSetActiveId   = void (*)(void* sceneSys, uint32_t slotId, uint32_t paramId, uint32_t bankId, int64_t negOne);
using FnSceneSyncBank      = void (*)(void* sceneSys, uint32_t slotId);
using FnSceneGetEditSlot   = void (*)(void* sceneSys, void* parentGui, uint32_t slotId, uint32_t zero);
using FnSceneInitSubCompA  = void (*)(void* subCompA, void* propSys, uint32_t zero);
using FnSceneInitSubCompB  = void (*)(void* subCompB, void* propSys, uint32_t zero, uint32_t slotId, uint32_t r8Zero, float xmmZero);

using FnPropRegisterGroup  = void (*)(void* propSys, uint32_t* outSlotId, const char16_t* name, const char16_t* editorName);
using FnPropBeginSection   = void (*)(void* propSys, uint32_t slotId, const char16_t* title, const char16_t* path, uint32_t r8Flag, uint32_t r9Flag);
using FnPropAddField       = void (*)(void* propSys, uint32_t slotId, const char16_t* label, const char16_t* key, ParamGuiPropDesc* desc, uint32_t r9Flag, uint32_t stackFlag);
using FnPropEndSection     = void (*)(void* propSys, uint32_t slotId, uint32_t flag);

using FnGuiCreateSubMenu   = void* (*)(void* parentGui, void* tempDesc);
using FnGuiAddActionItem   = void (*)(void* menuNode, void* labelDesc, void* callbackObj);
using FnGuiAllocNode       = void* (*)(void* alloc);
using FnGuiRegisterPanel   = void (*)(void* panelRoot, void* parentMenu, void* panelData);
using FnGuiAddSeparator    = void (*)(void* panelData, uint64_t zero);
using FnGuiAddTextItem     = void (*)(void* panelData, const char16_t* label, int64_t negOne);
using FnGuiBeginFoldout    = void* (*)(void* panelData, const char16_t* label, uint32_t flag);

using FnElemConstruct      = void (*)(void* elem);
using FnElemInit           = void (*)(void* elem);
using FnElemDestruct       = void (*)(void* elem);
using FnElemUnregister     = void (*)(void* elem);
using FnElemApply          = void (*)(void* elem, float weight);
using FnElemRegister       = void (*)(void* elem, void* alloc);
using FnElemBuildGui       = void (*)(void* elem, void* guiNode);

static inline void ReportTransactionFailure() {
#if defined(__x86_64__)
    const uintptr_t tlsBase = gx_guest_tls_base();
    if (*reinterpret_cast<int32_t*>(tlsBase + gx_guest_tls_offset(0x53e4318ULL)) != 0) {
        void* tlsObj = reinterpret_cast<void*>(tlsBase + gx_guest_tls_offset(0x53e4320ULL));
        reinterpret_cast<FnSetTlsErrorFlag>(GX_VMA(0x2bbe978ULL))(tlsObj, 1);
        return;
    }
#endif
    reinterpret_cast<FnLogAssert>(GX_VMA(0x20b55b0ULL))(
        0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
}

static inline float ConvertU64ToFloat(uint64_t v) {
    if (static_cast<int64_t>(v) < 0) {
        return static_cast<float>((v >> 1) | (v & 1ULL)) * 2.0f;
    }
    return static_cast<float>(static_cast<int64_t>(v));
}

static ParamBucketIter* ParamBucketVector_InsertFillImpl(
    ParamBucketIter* outIter,
    void* vecBase,
    uint64_t count,
    const ParamBucketIter* fillVal,
    ParamBucketIter posIter)
{
    uint8_t* vec = static_cast<uint8_t*>(vecBase);
    ParamBucketIter* beginPtr = *reinterpret_cast<ParamBucketIter**>(vec + 0x08);
    ParamBucketIter* endPtr   = *reinterpret_cast<ParamBucketIter**>(vec + 0x10);
    ParamBucketIter* capPtr   = *reinterpret_cast<ParamBucketIter**>(vec + 0x18);
    DLAllocator* alloc        = *reinterpret_cast<DLAllocator**>(vec + 0x20);

    ParamBucketIter* pos = reinterpret_cast<ParamBucketIter*>(posIter.node);

    const ptrdiff_t posOffsetBytes = reinterpret_cast<uint8_t*>(pos) - reinterpret_cast<uint8_t*>(beginPtr);
    const uint64_t posIndex = static_cast<uint64_t>(posOffsetBytes / static_cast<ptrdiff_t>(sizeof(ParamBucketIter)));

    if (count != 0) {
        const ptrdiff_t availBytes = reinterpret_cast<uint8_t*>(capPtr) - reinterpret_cast<uint8_t*>(endPtr);
        const uint64_t availItems = static_cast<uint64_t>(availBytes / static_cast<ptrdiff_t>(sizeof(ParamBucketIter)));

        ParamHashNode* const fillNode = fillVal ? fillVal->node : nullptr;

        if (availItems < count) {
            const ptrdiff_t curSizeBytes = reinterpret_cast<uint8_t*>(endPtr) - reinterpret_cast<uint8_t*>(beginPtr);
            const uint64_t curSizeItems = static_cast<uint64_t>(curSizeBytes / static_cast<ptrdiff_t>(sizeof(ParamBucketIter)));

            constexpr uint64_t kMaxVecItems = 0xaaaaaaaaaaaaaaaULL;
            if (kMaxVecItems - curSizeItems < count) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }

            const uint64_t minNeeded = curSizeItems + count;
            const ptrdiff_t curCapBytes = reinterpret_cast<uint8_t*>(capPtr) - reinterpret_cast<uint8_t*>(beginPtr);
            const uint64_t curCapItems = static_cast<uint64_t>(curCapBytes / static_cast<ptrdiff_t>(sizeof(ParamBucketIter)));

            uint64_t newCapItems = 0;
            if (kMaxVecItems - (curCapItems >> 1) >= curCapItems) {
                newCapItems = curCapItems + (curCapItems >> 1);
            }
            if (newCapItems < minNeeded) {
                newCapItems = minNeeded;
            }

            ParamBucketIter* newBuf = nullptr;
            if (newCapItems != 0) {
                newBuf = static_cast<ParamBucketIter*>(
                    alloc->vptr->allocateAligned(alloc, newCapItems * sizeof(ParamBucketIter), 8));
                if (!newBuf) {
                    ReportTransactionFailure();
                    if (outIter) {
                        outIter->proxy0 = 0;
                        outIter->proxy1 = 0;
                        outIter->node = nullptr;
                    }
                    return outIter;
                }
            }

            ParamBucketIter* dstFill = newBuf + posIndex;
            for (uint64_t i = 0; i < count; ++i) {
                dstFill[i].proxy0 = 0;
                dstFill[i].proxy1 = 0;
                dstFill[i].node   = fillNode;
            }

            ParamBucketIter* dstPrefix = newBuf;
            for (ParamBucketIter* src = beginPtr; src != pos; ++src, ++dstPrefix) {
                *dstPrefix = *src;
            }

            ParamBucketIter* dstSuffix = newBuf + posIndex + count;
            for (ParamBucketIter* src = pos; src != endPtr; ++src, ++dstSuffix) {
                *dstSuffix = *src;
            }

            if (beginPtr != nullptr) {
                alloc->vptr->freeBlock(alloc, beginPtr);
            }

            *reinterpret_cast<ParamBucketIter**>(vec + 0x08) = newBuf;
            *reinterpret_cast<ParamBucketIter**>(vec + 0x10) = newBuf + curSizeItems + count;
            *reinterpret_cast<ParamBucketIter**>(vec + 0x18) = newBuf + newCapItems;
            beginPtr = newBuf;
        } else {
            const ptrdiff_t tailBytes = reinterpret_cast<uint8_t*>(endPtr) - reinterpret_cast<uint8_t*>(pos);
            const uint64_t tailItems = static_cast<uint64_t>(tailBytes / static_cast<ptrdiff_t>(sizeof(ParamBucketIter)));

            if (tailItems < count) {
                const uint64_t overflowFill = count - tailItems;
                ParamBucketIter* oldEnd = endPtr;
                for (uint64_t i = 0; i < overflowFill; ++i) {
                    oldEnd[i].proxy0 = 0;
                    oldEnd[i].proxy1 = 0;
                    oldEnd[i].node   = fillNode;
                }
                endPtr += overflowFill;
                *reinterpret_cast<ParamBucketIter**>(vec + 0x10) = endPtr;

                ParamBucketIter* dstTail = endPtr;
                for (ParamBucketIter* src = pos; src != oldEnd; ++src, ++dstTail) {
                    dstTail->proxy0 = 0;
                    dstTail->proxy1 = 0;
                    dstTail->node   = src->node;
                }
                *reinterpret_cast<ParamBucketIter**>(vec + 0x10) = dstTail;

                for (ParamBucketIter* dst = pos; dst != oldEnd; ++dst) {
                    dst->node = fillNode;
                }
            } else {
                ParamBucketIter* oldEnd = endPtr;
                ParamBucketIter* moveSrc = oldEnd - count;
                ParamBucketIter* dstEnd = oldEnd;
                for (ParamBucketIter* src = moveSrc; src != oldEnd; ++src, ++dstEnd) {
                    dstEnd->proxy0 = 0;
                    dstEnd->proxy1 = 0;
                    dstEnd->node   = src->node;
                }
                *reinterpret_cast<ParamBucketIter**>(vec + 0x10) = dstEnd;

                ParamBucketIter* bwdDst = oldEnd;
                ParamBucketIter* bwdSrc = moveSrc;
                while (bwdSrc != pos) {
                    --bwdSrc;
                    --bwdDst;
                    bwdDst->node = bwdSrc->node;
                }

                for (uint64_t i = 0; i < count; ++i) {
                    pos[i].node = fillNode;
                }
            }
        }
    }

    ParamBucketIter* resultPtr = beginPtr + posIndex;
    if (outIter) {
        outIter->proxy0 = 0;
        outIter->proxy1 = 0;
        outIter->node   = reinterpret_cast<ParamHashNode*>(resultPtr);
    }
    return outIter;
}

static ParamIterPair* ParamHashTable_InsertNodeImpl(
    ParamIterPair* outRes,
    ParamInnerHashTable* table,
    const uint32_t* keyPtr,
    ParamBucketIter nodeIter)
{
    ParamHashNode* const newNode = nodeIter.node;
    const uint32_t key = *keyPtr;
    const uint64_t bucketIdx = ComputeLehmerHashBucket(key, 0xdeadbeefU, table->mask, table->maxBucketIdx);

    ParamHashNode* const head = table->sentinel;
    ParamHashBucket* const buckets = table->buckets;
    ParamHashNode* const bucketFirst = buckets[bucketIdx].beginNode;

    ParamHashNode* insertBefore = head;
    if (bucketFirst != head) {
        ParamHashNode* const bucketAfterLast = buckets[bucketIdx].endNode->next;
        ParamHashNode* cur = bucketAfterLast;
        while (cur != bucketFirst) {
            cur = cur->prev;
            if (key < cur->key) {
                continue;
            }
            if (cur->key >= key) {
                if (newNode != head) {
                    ParamHashNode* nextNode = newNode->next;
                    ParamHashNode* prevNode = newNode->prev;
                    nextNode->prev = prevNode;
                    prevNode->next = nextNode;
                    table->count -= 1;
                    table->nodeAlloc->vptr->freeBlock(table->nodeAlloc, newNode);
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
        insertBefore = cur;
    } else {
        insertBefore = bucketFirst;
    }

    ParamHashNode* const newNext = newNode->next;
    if (insertBefore != newNext && insertBefore != newNode) {
        newNode->prev->next = newNext;
        newNext->prev       = newNode->prev;

        newNode->next       = insertBefore;
        newNode->prev       = insertBefore->prev;
        insertBefore->prev->next = newNode;
        insertBefore->prev  = newNode;
    }

    ParamHashBucket& bSlot = table->buckets[bucketIdx];
    if (bSlot.beginNode == table->sentinel) {
        bSlot.beginNode = newNode;
        bSlot.endNode   = newNode;
    } else if (bSlot.beginNode == insertBefore) {
        bSlot.beginNode = newNode;
    } else {
        ParamHashNode* lastNext = bSlot.endNode->next;
        bSlot.endNode = (lastNext != newNode) ? lastNext->prev : lastNext;
    }

    uint64_t curBuckets = table->maxBucketIdx;
    const float sizeF   = ConvertU64ToFloat(table->count);
    const float bucketF = ConvertU64ToFloat(curBuckets);

    if ((sizeF / bucketF) > table->maxLoadFactor) {
        uint64_t newMaxBuckets = curBuckets;
        if (newMaxBuckets < 0x555555555555555ULL) {
            do {
                if (newMaxBuckets > 0x1ffULL) {
                    newMaxBuckets <<= 1;
                } else {
                    newMaxBuckets <<= 3;
                }
                const float newSizeF   = ConvertU64ToFloat(table->count);
                const float newBucketF = ConvertU64ToFloat(newMaxBuckets);
                if ((newSizeF / newBucketF) <= table->maxLoadFactor) {
                    break;
                }
            } while (newMaxBuckets < 0x555555555555555ULL);
        }

        table->bucketCapBytes = reinterpret_cast<uint64_t>(table->buckets);
        ParamBucketIter fillVal{0, 0, table->sentinel};
        ParamBucketIter dummyOut{};
        ParamBucketIter posIter{0, 0, reinterpret_cast<ParamHashNode*>(table->buckets)};
        ParamBucketVector_InsertFillImpl(
            &dummyOut,
            reinterpret_cast<uint8_t*>(table) + 0x20,
            newMaxBuckets * 2ULL,
            &fillVal,
            posIter);

        table->mask         = newMaxBuckets - 1ULL;
        table->maxBucketIdx = newMaxBuckets;

        ParamHashNode* headNode = table->sentinel;
        if (headNode->next != headNode) {
            ParamHashNode* const lastOriginal = headNode->prev;
            while (true) {
                ParamHashNode* moveNode = table->sentinel->next;
                ParamBucketIter moveIter{0, 0, moveNode};
                ParamIterPair reinsertOut{};
                ParamHashTable_InsertNodeImpl(&reinsertOut, table, &moveNode->key, moveIter);
                if (moveNode == lastOriginal) {
                    break;
                }
            }
        }
    }

    outRes->proxy0   = 0;
    outRes->proxy1   = 0;
    outRes->node     = newNode;
    outRes->inserted = 1;
    return outRes;
}

static ParamBucketIter* ParamHashTable_EraseNodeImpl(
    ParamBucketIter* outNextIter,
    ParamInnerHashTable* table,
    ParamHashNode* eraseNode)
{
    const uint64_t bucketIdx = ComputeLehmerHashBucket(
        eraseNode->key, 0xdeadbeefU, table->mask, table->maxBucketIdx);

    ParamHashBucket& bSlot = table->buckets[bucketIdx];
    ParamHashNode* const head   = table->sentinel;
    ParamHashNode* const bFirst = bSlot.beginNode;
    ParamHashNode* const bLast  = bSlot.endNode;

    if (eraseNode == bFirst) {
        bSlot.beginNode = (eraseNode == bLast) ? head : eraseNode->next;
    }
    if (eraseNode == bLast) {
        bSlot.endNode = (eraseNode == bFirst) ? head : eraseNode->prev;
    }

    ParamHashNode* const nextNode = eraseNode->next;
    ParamHashNode* const prevNode = eraseNode->prev;
    prevNode->next = nextNode;
    nextNode->prev = prevNode;
    table->count -= 1;

    table->nodeAlloc->vptr->freeBlock(table->nodeAlloc, eraseNode);

    if (outNextIter) {
        outNextIter->proxy0 = 0;
        outNextIter->proxy1 = 0;
        outNextIter->node   = nextNode;
    }
    return outNextIter;
}

static void ReleaseTempGuiStringNode_DE(uint8_t* node) {
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
            bufAlloc->vptr->freeBlock(bufAlloc, strBuf);
        }
        *reinterpret_cast<uint64_t*>(node + 0x20) = 7;
        *reinterpret_cast<uint64_t*>(node + 0x18) = 0;
        *reinterpret_cast<uint16_t*>(node + 0x08) = 0;
        strPoolAlloc->vptr->freeBlock(strPoolAlloc, node);
    }
}

static void ReleaseGuiWidgetRef_DE(uint8_t* widget) {
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

static void GXParamBankManager_OpenParamEditListDialogImpl_DE(
    uint8_t* self,
    uint32_t sceneSlotOffset,
    uint32_t titleStrOffset,
    uint32_t defaultEntryOffset,
    FnElemBuildGui buildGuiFn)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneMgr   = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotIdx    = *reinterpret_cast<uint32_t*>(sceneMgr + sceneSlotOffset);
    uint32_t activeBank = *reinterpret_cast<uint32_t*>(sceneMgr + slotIdx * 4 + 0x588);

    reinterpret_cast<void (*)(void*, const char16_t*, uint32_t)>(GX_VMA(0x2936030ULL))(
        self + titleStrOffset,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49cbebeULL)),
        activeBank);

    DLAllocator* guiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    uint8_t* dialog = static_cast<uint8_t*>(
        guiAlloc->vptr->allocateAligned(guiAlloc, 0x178, 8));
    if (dialog) {
        uint8_t* strData = self + titleStrOffset + 0x08;
        uint64_t strCap  = *reinterpret_cast<uint64_t*>(self + titleStrOffset + 0x20);
        if (strCap >= 8) {
            strData = *reinterpret_cast<uint8_t**>(strData);
        }
        uint64_t titleDesc[2] = {0, reinterpret_cast<uint64_t>(strData)};
        reinterpret_cast<void (*)(void*, const void*)>(GX_VMA(0x2362cf0ULL))(dialog, titleDesc);
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
            reinterpret_cast<void (*)(void*, void*, void*, const void*)>(GX_VMA(0x2328e40ULL))(
                dialog, guiDesktop, desktopCtx, &rectDesc);
        } else {
            ReleaseGuiWidgetRef_DE(dialog);
            dialog = nullptr;
        }
    }

    alignas(16) uint8_t guiBuilder[32] = {};
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x236c4d0ULL))(guiBuilder, dialog);

    alignas(16) uint64_t tempStrDesc[2] = {0, 0};
    reinterpret_cast<void (*)(void*, const char16_t*, ...)>(GX_VMA(0x2ba7d40ULL))(
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

    void* dummySection = reinterpret_cast<void* (*)(void*, const void*, uint32_t)>(GX_VMA(0x236c900ULL))(
        guiBuilder, labelRef, 0);
    ReleaseTempGuiStringNode_DE(refNode);

    if (dummySection != nullptr) {
        buildGuiFn(self + defaultEntryOffset, guiBuilder);
        uint8_t** stackBegin = *reinterpret_cast<uint8_t***>(guiBuilder + 0x08);
        uint8_t** stackEnd   = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
        if (stackBegin != stackEnd) {
            uint8_t* poppedWidget = stackEnd[-1];
            if (poppedWidget != nullptr) {
                ReleaseGuiWidgetRef_DE(poppedWidget);
                stackEnd[-1] = nullptr;
                stackEnd = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
            }
            *reinterpret_cast<uint8_t***>(guiBuilder + 0x10) = stackEnd - 1;
        }
    }
    ReleaseTempGuiStringNode_DE(reinterpret_cast<uint8_t*>(tempStrDesc[0]));

    ParamOuterHashTable* outer = reinterpret_cast<ParamOuterHashTable*>(self);
    const uint64_t bankBucket = ComputeLehmerHashBucket(activeBank, 0xdeadbeefU, outer->mask, outer->maxBucketIdx);
    ParamHashNode* const outerHead = outer->sentinel;
    ParamHashBucket* const outerBuckets = outer->buckets;
    ParamHashNode* bankNode = outerBuckets[bankBucket].beginNode;

    if (bankNode != outerHead) {
        ParamHashNode* bankLast = outerBuckets[bankBucket].endNode->next;
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
            ParamInnerHashTable* innerTable = static_cast<ParamInnerHashTable*>(bankNode->value);
            ParamHashNode* paramNode = innerTable->sentinel->next;
            while (paramNode != innerTable->sentinel) {
                if (paramNode->key != 0xffffffffU) {
                    alignas(16) uint64_t paramStrDesc[2] = {0, 0};
                    reinterpret_cast<void (*)(void*, const char16_t*, ...)>(GX_VMA(0x2ba7d40ULL))(
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

                    void* paramSection = reinterpret_cast<void* (*)(void*, const void*, uint32_t)>(GX_VMA(0x236c900ULL))(
                        guiBuilder, paramLabelRef, 0);
                    ReleaseTempGuiStringNode_DE(paramRefNode);

                    if (paramSection != nullptr) {
                        buildGuiFn(paramNode->value, guiBuilder);
                        uint8_t** stackBegin = *reinterpret_cast<uint8_t***>(guiBuilder + 0x08);
                        uint8_t** stackEnd   = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
                        if (stackBegin != stackEnd) {
                            uint8_t* poppedWidget = stackEnd[-1];
                            if (poppedWidget != nullptr) {
                                ReleaseGuiWidgetRef_DE(poppedWidget);
                                stackEnd[-1] = nullptr;
                                stackEnd = *reinterpret_cast<uint8_t***>(guiBuilder + 0x10);
                            }
                            *reinterpret_cast<uint8_t***>(guiBuilder + 0x10) = stackEnd - 1;
                        }
                    }
                    ReleaseTempGuiStringNode_DE(reinterpret_cast<uint8_t*>(paramStrDesc[0]));
                    innerTable = static_cast<ParamInnerHashTable*>(bankNode->value);
                }
                paramNode = paramNode->next;
            }
        }
    }

    reinterpret_cast<void (*)(void*)>(GX_VMA(0x236c720ULL))(guiBuilder);
}

} // namespace

// Declare Group C HashTable helpers defined in GXYebisToneMapAndSettingParamBanks.cpp
extern "C" {
    void GXParamBankManagerC_OpenParamEditListDialog(void* self);
    ParamBucketIter* DLHashTable_EraseNode_InnerC(
        ParamBucketIter* outIter, ParamInnerHashTable* table, ParamHashNode* targetNode);
    ParamBucketIter* DLHashTable_EraseNode_OuterC(
        ParamBucketIter* outIter, void* tableMinus8, ParamHashNode* targetNode);
    ParamIterPair* DLHashTable_InsertNode_OuterC(
        ParamIterPair* outRes, void* tableMinus8, const uint32_t* keyPtr, ParamBucketIter nodeIter);
    ParamIterPair* DLHashTable_InsertNode_InnerC(
        ParamIterPair* outRes, ParamInnerHashTable* table, const uint32_t* keyPtr, ParamBucketIter nodeIter);
    ParamBucketIter* DLHashBucketVector_InsertFill_OuterC(
        ParamBucketIter* outIter, void* vecBase, uint64_t count,
        const ParamBucketIter* fillVal, ParamBucketIter posIter);
    ParamBucketIter* DLHashBucketVector_InsertFill_InnerC(
        ParamBucketIter* outIter, void* vecBase, uint64_t count,
        const ParamBucketIter* fillVal, ParamBucketIter posIter);

    // Forward declarations for Group C methods in Batch 3
    void GXParamBankManagerC_RegisterAllParams(void* self);
    void GXParamBankManagerC_UnregisterAllParams(void* self);
    void GXParamBankManagerC_ClearAllBanks(void* self);

    // Forward declarations for Group D methods in Batch 3
    ParamBucketIter* GXParamBankManagerD_InnerHashTable_RehashInsert(
        ParamBucketIter* outIter, void* vecBase, uint64_t count,
        const ParamBucketIter* fillVal, ParamBucketIter posIter);
    ParamBucketIter* GXParamBankManagerD_OuterHashTable_RehashInsert(
        ParamBucketIter* outIter, void* vecBase, uint64_t count,
        const ParamBucketIter* fillVal, ParamBucketIter posIter);
    ParamIterPair* GXParamBankManagerD_InnerHashTable_InsertNode(
        ParamIterPair* outRes, ParamInnerHashTable* table, const uint32_t* keyPtr, ParamBucketIter nodeIter);
    ParamIterPair* GXParamBankManagerD_OuterHashTable_InsertNode(
        ParamIterPair* outRes, void* tableMinus8, const uint32_t* keyPtr, ParamBucketIter nodeIter);
    ParamBucketIter* GXParamBankManagerD_OuterHashTable_EraseNode(
        ParamBucketIter* outIter, void* tableMinus8, ParamHashNode* targetNode);
    ParamBucketIter* GXParamBankManagerD_InnerHashTable_EraseNode(
        ParamBucketIter* outIter, ParamInnerHashTable* table, ParamHashNode* targetNode);
    void GXParamBankManagerD_OpenParamEditListDialog(void* self);
    void GXParamBankManagerD_RegisterAllParams(void* self);
    void GXParamBankManagerD_UnregisterAllParams(void* self);
    void GXParamBankManagerD_ClearAllBanks(void* self);

    // Forward declarations for Group E methods in Batch 3
    ParamBucketIter* GXParamBankManagerE_InnerHashTable_RehashInsert(
        ParamBucketIter* outIter, void* vecBase, uint64_t count,
        const ParamBucketIter* fillVal, ParamBucketIter posIter);
    ParamBucketIter* GXParamBankManagerE_OuterHashTable_RehashInsert(
        ParamBucketIter* outIter, void* vecBase, uint64_t count,
        const ParamBucketIter* fillVal, ParamBucketIter posIter);
    ParamIterPair* GXParamBankManagerE_InnerHashTable_InsertNode(
        ParamIterPair* outRes, ParamInnerHashTable* table, const uint32_t* keyPtr, ParamBucketIter nodeIter);
    ParamIterPair* GXParamBankManagerE_OuterHashTable_InsertNode(
        ParamIterPair* outRes, void* tableMinus8, const uint32_t* keyPtr, ParamBucketIter nodeIter);
    ParamBucketIter* GXParamBankManagerE_OuterHashTable_EraseNode(
        ParamBucketIter* outIter, void* tableMinus8, ParamHashNode* targetNode);
    ParamBucketIter* GXParamBankManagerE_InnerHashTable_EraseNode(
        ParamBucketIter* outIter, ParamInnerHashTable* table, ParamHashNode* targetNode);
    void GXParamBankManagerE_OpenParamEditListDialog(void* self);
    void GXParamBankManagerE_RegisterAllParams(void* self);
    void GXParamBankManagerE_UnregisterAllParams(void* self);
    void GXParamBankManagerE_ClearAllBanks(void* self);
}

// ============================================================================
// GROUP C: MotionBlur[Yebis] Parameter Bank Manager (0x21f8b40 .. 0x21fa740)
// ============================================================================

// 1. 0x21fa740 (612 B) — GXParamBankManagerC_RegisterDebugMenu
extern "C" void GXParamBankManagerC_RegisterDebugMenu(void* self, void* parentGui) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x48);

    reinterpret_cast<FnSceneGetEditSlot>(GX_VMA(0x21877f0ULL))(sceneSys, parentGui, slotId, 0);

    DLAllocator* guiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    FnGuiCreateSubMenu fnCreateSubMenu = reinterpret_cast<FnGuiCreateSubMenu>(GX_VMA(0x236d620ULL));
    FnGuiAddActionItem fnAddAction = reinterpret_cast<FnGuiAddActionItem>(GX_VMA(0x235e4c0ULL));

    // 1) "ParameterSystem Regist" -> GXParamBankManagerC_RegisterAllParams (0x21fa360)
    uint64_t subDesc1[3] = {0, 0, 0};
    void* menu1 = fnCreateSubMenu(parentGui, subDesc1);
    subDesc1[1] = 0;

    uint64_t labelDesc1[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL))), 0};
    uint8_t* cb1 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb1 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb1 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb1 + 0x00) = GX_VMA(0x53b71e0ULL);
        *reinterpret_cast<void**>(cb1 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb1 + 0x18) = 0;
        *reinterpret_cast<void**>(cb1 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerC_RegisterAllParams);
        *reinterpret_cast<uint64_t*>(cb1 + 0x28) = 0;
    }
    fnAddAction(menu1, labelDesc1, cb1);
    labelDesc1[1] = 0;

    // 2) "ParameterSystem Unregist" -> GXParamBankManagerC_UnregisterAllParams (0x21fa3f0)
    uint64_t subDesc2[3] = {0, 0, 0};
    void* menu2 = fnCreateSubMenu(parentGui, subDesc2);
    subDesc2[1] = 0;

    uint64_t labelDesc2[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL))), 0};
    uint8_t* cb2 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb2 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb2 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb2 + 0x00) = GX_VMA(0x53b71e0ULL);
        *reinterpret_cast<void**>(cb2 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb2 + 0x18) = 0;
        *reinterpret_cast<void**>(cb2 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerC_UnregisterAllParams);
        *reinterpret_cast<uint64_t*>(cb2 + 0x28) = 0;
    }
    fnAddAction(menu2, labelDesc2, cb2);
    labelDesc2[1] = 0;

    // 3) "Open List" -> GXParamBankManagerC_OpenParamEditListDialog (0x21fa9b0)
    uint64_t subDesc3[3] = {0, 0, 0};
    void* menu3 = fnCreateSubMenu(parentGui, subDesc3);
    subDesc3[1] = 0;

    uint64_t labelDesc3[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL))), 0};
    uint8_t* cb3 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb3 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb3 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb3 + 0x00) = GX_VMA(0x53b71e0ULL);
        *reinterpret_cast<void**>(cb3 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb3 + 0x18) = 0;
        *reinterpret_cast<void**>(cb3 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerC_OpenParamEditListDialog);
        *reinterpret_cast<uint64_t*>(cb3 + 0x28) = 0;
    }
    fnAddAction(menu3, labelDesc3, cb3);
    labelDesc3[1] = 0;
}

// 2. 0x21fa460 (732 B) — GXParamBankManagerC_ResolveParamEntry
extern "C" void* GXParamBankManagerC_ResolveParamEntry(ParamOuterHashTable* self, const uint64_t* packedKeyPtr) {
    uint64_t packed = *packedKeyPtr;
    uint32_t bankId = static_cast<uint32_t>(packed);
    uint32_t paramId = static_cast<uint32_t>(packed >> 32);

    if (bankId == 0xffffffffU) {
        if (paramId == 0xffffffffU) {
            paramId = 0;
            bankId = 1;
        } else {
            bankId = 0xffffffffU;
        }
    }

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerCur = outerBuckets[outerIdx].beginNode;

    if (outerCur != outerSentinel) {
        ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
        while (outerCur != outerEnd) {
            if (outerCur->key >= bankId) {
                if (bankId < outerCur->key) {
                    outerCur = outerSentinel;
                }
                break;
            }
            outerCur = outerCur->next;
        }
        if (outerCur != outerSentinel && outerCur != outerEnd) {
            ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerCur->value);
            uint64_t innerIdx = ComputeLehmerHashBucket(paramId, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
            ParamHashNode* innerSentinel = inner->sentinel;
            ParamHashBucket* innerBuckets = inner->buckets;
            ParamHashNode* innerCur = innerBuckets[innerIdx].beginNode;
            ParamHashNode* foundInner = innerSentinel;

            if (innerCur != innerSentinel) {
                ParamHashNode* innerEnd = innerBuckets[innerIdx].endNode->next;
                while (innerCur != innerEnd) {
                    if (innerCur->key >= paramId) {
                        if (paramId >= innerCur->key) {
                            foundInner = innerCur;
                        }
                        break;
                    }
                    innerCur = innerCur->next;
                }
            }
            if (foundInner != inner->sentinel) {
                return foundInner->value;
            }
        }
    }

    // Fallback to bankId = 1 (0xdeadbeee ^ 0xdeadbeef == 1), paramId = 0 (0xdeadbeef ^ 0xdeadbeef == 0)
    uint64_t fbOuterIdx = ComputeLehmerHashBucket(1U, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* fbOuterCur = outerBuckets[fbOuterIdx].beginNode;
    if (fbOuterCur != outerSentinel) {
        ParamHashNode* fbOuterEnd = outerBuckets[fbOuterIdx].endNode->next;
        while (fbOuterCur != fbOuterEnd) {
            if (fbOuterCur->key != 0) {
                if (fbOuterCur->key > 1U) {
                    fbOuterCur = outerSentinel;
                }
                break;
            }
            fbOuterCur = fbOuterCur->next;
        }
        if (fbOuterCur != outerSentinel && fbOuterCur != fbOuterEnd) {
            ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(fbOuterCur->value);
            uint64_t fbInnerIdx = ComputeLehmerHashBucket(0U, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
            ParamHashNode* innerSentinel = inner->sentinel;
            ParamHashBucket* innerBuckets = inner->buckets;
            ParamHashNode* fbInnerCur = innerBuckets[fbInnerIdx].beginNode;
            ParamHashNode* foundFb = innerSentinel;

            if (fbInnerCur != innerSentinel) {
                ParamHashNode* fbInnerEnd = innerBuckets[fbInnerIdx].endNode->next;
                if (fbInnerCur != fbInnerEnd && fbInnerCur->key == 0U) {
                    foundFb = fbInnerCur;
                }
            }
            if (foundFb != inner->sentinel) {
                return foundFb->value;
            }
        }
    }

    return reinterpret_cast<uint8_t*>(self) + 0x190;
}

// 3. 0x21fa3f0 (103 B) — GXParamBankManagerC_UnregisterAllParams
extern "C" void GXParamBankManagerC_UnregisterAllParams(void* self) {
    ParamOuterHashTable* outer = static_cast<ParamOuterHashTable*>(self);
    ParamHashNode* outerSentinel = outer->sentinel;
    for (ParamHashNode* outerNode = outerSentinel->next; outerNode != outerSentinel; outerNode = outerNode->next) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
        ParamHashNode* innerSentinel = inner->sentinel;
        for (ParamHashNode* innerNode = innerSentinel->next; innerNode != innerSentinel; innerNode = innerNode->next) {
            reinterpret_cast<FnElemUnregister>(GX_VMA(0xe826d0ULL))(innerNode->value);
        }
        outerSentinel = outer->sentinel;
    }
}

// 4. 0x21fa360 (138 B) — GXParamBankManagerC_RegisterAllParams
extern "C" void GXParamBankManagerC_RegisterAllParams(void* self) {
    ParamOuterHashTable* outer = static_cast<ParamOuterHashTable*>(self);
    ParamHashNode* outerSentinel = outer->sentinel;
    for (ParamHashNode* outerNode = outerSentinel->next; outerNode != outerSentinel; outerNode = outerNode->next) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
        ParamHashNode* innerSentinel = inner->sentinel;
        for (ParamHashNode* innerNode = innerSentinel->next; innerNode != innerSentinel; innerNode = innerNode->next) {
            uint8_t* elem = static_cast<uint8_t*>(innerNode->value);
            *reinterpret_cast<uint32_t*>(elem + 0x88) = innerNode->key;
            *reinterpret_cast<uint32_t*>(elem + 0x8c) = outerNode->key;
            reinterpret_cast<FnElemApply>(GX_VMA(0xe82730ULL))(elem, 0.0f);
            reinterpret_cast<FnElemRegister>(GX_VMA(0xe82860ULL))(elem, outer->ownerAlloc);
        }
        outerSentinel = outer->sentinel;
    }
}

// 5. 0x21fa350 (1 B) — GXParamBankManagerC_OnEventNoOp
extern "C" void GXParamBankManagerC_OnEventNoOp(void*) {
    return;
}

// 6. 0x21fa320 (40 B) — GXParamBankManagerC_SetActiveParamId
extern "C" void GXParamBankManagerC_SetActiveParamId(void* /*self*/, uint32_t paramId, uint32_t bankId) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x48);
    reinterpret_cast<FnSceneSetActiveId>(GX_VMA(0x2186da0ULL))(sceneSys, slotId, paramId, bankId, -1LL);
}

// 7. 0x21fa060 (698 B) — GXParamBankManagerC_UnloadBankById
extern "C" void GXParamBankManagerC_UnloadBankById(
    ParamOuterHashTable* self,
    void* /*unused*/,
    uint32_t bankId,
    uint32_t overrideSlotIdx,
    float blendTime)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x48);

    reinterpret_cast<FnSceneBeginUnload>(GX_VMA(0x2185b50ULL))(
        sceneSys, slotId, bankId, overrideSlotIdx, blendTime);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = 1;

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerNode = outerBuckets[outerIdx].beginNode;
    if (outerNode == outerSentinel) {
        return;
    }

    ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
    while (outerNode != outerEnd && outerNode->key < bankId) {
        outerNode = outerNode->next;
    }
    if (outerNode == outerEnd || outerNode->key > bankId || outerNode == outerSentinel) {
        return;
    }

    ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
    if (inner != nullptr) {
        ParamHashNode* innerCur = inner->sentinel->next;
        while (innerCur != inner->sentinel) {
            bool keep = reinterpret_cast<FnSceneCheckEntry>(GX_VMA(0x2185e00ULL))(sceneSys, slotId, innerCur->key, bankId);
            DLVirtualObj* elem = static_cast<DLVirtualObj*>(innerCur->value);
            if (keep) {
                *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(elem) + 0x80) = 1;
                innerCur = innerCur->next;
            } else {
                if (elem != nullptr) {
                    elem->vptr->dtor(elem);
                    self->ownerAlloc->vptr->freeBlock(self->ownerAlloc, elem);
                    innerCur->value = nullptr;
                }
                ParamBucketIter nextIt{};
                DLHashTable_EraseNode_InnerC(&nextIt, inner, innerCur);
                innerCur = nextIt.node;
            }
        }

        if (inner->count != 0) {
            return;
        }

        DLAllocator* ownerAlloc = self->ownerAlloc;
        if (inner->buckets != nullptr) {
            inner->bucketAlloc->vptr->freeBlock(inner->bucketAlloc, inner->buckets);
            inner->buckets = nullptr;
            inner->bucketCapBytes = 0;
            inner->bucketEndOff = 0;
        }

        ParamHashNode* head = inner->sentinel;
        ParamHashNode* n = head->next;
        head->next = head;
        inner->sentinel->prev = inner->sentinel;
        inner->count = 0;
        while (n != inner->sentinel) {
            ParamHashNode* nextN = n->next;
            inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, n);
            n = nextN;
        }
        inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, inner->sentinel);
        ownerAlloc->vptr->freeBlock(ownerAlloc, inner);
        outerNode->value = nullptr;
    }

    ParamBucketIter nextOuter{};
    DLHashTable_EraseNode_OuterC(&nextOuter, reinterpret_cast<uint8_t*>(self) + 0x08, outerNode);
}

// 8. 0x21f98e0 (1912 B) — GXParamBankManagerC_LoadBankFromStream
extern "C" void GXParamBankManagerC_LoadBankFromStream(
    ParamOuterHashTable* self, void* streamPtr, uint32_t bankId, uint32_t flags, float weight) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x48);

    uint32_t entryCount = reinterpret_cast<FnSceneGetBankCount>(GX_VMA(0x2184cf0ULL))(sceneSys, slotId, streamPtr);
    if (entryCount == 0) {
        reinterpret_cast<FnSceneFinalizeLoad>(GX_VMA(0x2184ff0ULL))(sceneSys, slotId, bankId, flags, streamPtr, weight);
        return;
    }

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerNode = outerBuckets[outerIdx].beginNode;
    if (outerNode != outerSentinel) {
        ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
        while (outerNode != outerEnd && outerNode->key < bankId) {
            outerNode = outerNode->next;
        }
        if (outerNode == outerEnd || outerNode->key > bankId) {
            outerNode = outerSentinel;
        }
    }

    ParamInnerHashTable* inner = nullptr;
    if (outerNode == outerSentinel) {
        DLAllocator* ownerAlloc = self->ownerAlloc;
        inner = static_cast<ParamInnerHashTable*>(ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x60, 0x8));
        if (inner != nullptr) {
            uint8_t heapCaps[32] = {0};
            ownerAlloc->vptr->getHeapCaps(heapCaps, ownerAlloc, 0);
            if ((heapCaps[0] & 0x20) == 0) {
                reinterpret_cast<FnLogAssert>(GX_VMA(0x20b55b0ULL))(
                    0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }
            inner->count = 0;
            inner->sentinel = nullptr;
            inner->nodeAlloc = ownerAlloc;
            ParamHashNode* innerSent = static_cast<ParamHashNode*>(
                ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x20, 0x8));
            if (innerSent != nullptr) {
                innerSent->next = innerSent;
                innerSent->prev = innerSent;
            } else {
                ReportTransactionFailure();
            }
            inner->sentinel = innerSent;
            inner->buckets = nullptr;
            inner->bucketCapBytes = 0;
            inner->bucketEndOff = 0;
            inner->bucketAlloc = ownerAlloc;
            inner->maxLoadFactor = 1.0f;

            ParamBucketIter fillVal{0, 0, innerSent};
            ParamBucketIter dummyOut{};
            ParamBucketIter posIter{0, 0, nullptr};
            DLHashBucketVector_InsertFill_InnerC(
                &dummyOut, reinterpret_cast<uint8_t*>(inner) + 0x20, 0x10, &fillVal, posIter);
            inner->mask = 7;
            inner->maxBucketIdx = 8;
        }

        ParamHashNode* headFirst = self->sentinel->next;
        ParamHashNode* newOuterNode = static_cast<ParamHashNode*>(
            self->nodeAlloc->vptr->allocateAligned(self->nodeAlloc, 0x20, 0x8));
        if (newOuterNode == nullptr) {
            ReportTransactionFailure();
            return;
        }
        newOuterNode->next = headFirst ? headFirst : newOuterNode;
        newOuterNode->prev = headFirst ? headFirst->prev : newOuterNode;
        newOuterNode->key = bankId;
        newOuterNode->value = inner;
        if (self->count == 0x7fffffffffffffeULL) {
            reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        self->count += 1;
        headFirst->prev = newOuterNode;
        newOuterNode->prev->next = newOuterNode;

        ParamIterPair insRes{};
        ParamHashNode* insertedOuter = self->sentinel->next;
        DLHashTable_InsertNode_OuterC(
            &insRes, reinterpret_cast<uint8_t*>(self) + 0x08, &insertedOuter->key, ParamBucketIter{0, 0, insertedOuter});
    } else {
        inner = static_cast<ParamInnerHashTable*>(outerNode->value);
    }

    for (uint32_t i = 0; i < entryCount; ++i) {
        uint32_t paramId = reinterpret_cast<FnSceneGetParamIdAt>(GX_VMA(0x2184de0ULL))(sceneSys, slotId, streamPtr, i);
        uint64_t innerIdx = ComputeLehmerHashBucket(paramId, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
        ParamHashNode* innerSentinel = inner->sentinel;
        ParamHashBucket* innerBuckets = inner->buckets;
        ParamHashNode* innerNode = innerBuckets[innerIdx].beginNode;
        if (innerNode != innerSentinel) {
            ParamHashNode* innerEnd = innerBuckets[innerIdx].endNode->next;
            while (innerNode != innerEnd && innerNode->key < paramId) {
                innerNode = innerNode->next;
            }
            if (innerNode == innerEnd || innerNode->key > paramId) {
                innerNode = innerSentinel;
            }
        }

        if (innerNode == innerSentinel) {
            DLAllocator* ownerAlloc = self->ownerAlloc;
            uint8_t* elem = static_cast<uint8_t*>(ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x90, 0x8));
            if (elem == nullptr) {
                ReportTransactionFailure();
                continue;
            }
            reinterpret_cast<FnElemConstruct>(GX_VMA(0xe82420ULL))(elem);
            reinterpret_cast<FnElemInit>(GX_VMA(0xe82510ULL))(elem);
            *reinterpret_cast<uint32_t*>(elem + 0x88) = paramId;
            *reinterpret_cast<uint32_t*>(elem + 0x8c) = bankId;
            reinterpret_cast<FnElemApply>(GX_VMA(0xe82730ULL))(elem, 0.0f);
            reinterpret_cast<FnElemRegister>(GX_VMA(0xe82860ULL))(elem, self->ownerAlloc);

            ParamHashNode* iHeadFirst = inner->sentinel->next;
            ParamHashNode* newInnerNode = static_cast<ParamHashNode*>(
                inner->nodeAlloc->vptr->allocateAligned(inner->nodeAlloc, 0x20, 0x8));
            if (newInnerNode == nullptr) {
                ReportTransactionFailure();
                continue;
            }
            newInnerNode->next = iHeadFirst ? iHeadFirst : newInnerNode;
            newInnerNode->prev = iHeadFirst ? iHeadFirst->prev : newInnerNode;
            newInnerNode->key = paramId;
            newInnerNode->value = elem;
            if (inner->count == 0x7fffffffffffffeULL) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            inner->count += 1;
            iHeadFirst->prev = newInnerNode;
            newInnerNode->prev->next = newInnerNode;

            ParamIterPair insRes{};
            ParamHashNode* insertedInner = inner->sentinel->next;
            DLHashTable_InsertNode_InnerC(
                &insRes, inner, &insertedInner->key, ParamBucketIter{0, 0, insertedInner});
        } else {
            uint8_t* elem = static_cast<uint8_t*>(innerNode->value);
            *reinterpret_cast<uint8_t*>(elem + 0x80) = 1;
        }
    }

    reinterpret_cast<FnSceneFinalizeLoad>(GX_VMA(0x2184ff0ULL))(sceneSys, slotId, bankId, flags, streamPtr, weight);
}

// 9. 0x21f9110 (1989 B) — GXParamBankManagerC_RegisterProperties
extern "C" bool GXParamBankManagerC_RegisterProperties(uint8_t* self) {
    GXParamBankManagerC_ClearAllBanks(self);

    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    void* propSys = *reinterpret_cast<void**>(sceneSys + 0x28);

    uint32_t slotId = 0;
    reinterpret_cast<FnPropRegisterGroup>(GX_VMA(0xe32510ULL))(
        propSys, &slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6f2aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6ef8ULL)));
    *reinterpret_cast<uint32_t*>(sceneSys + 0x48) = slotId;

    FnPropBeginSection fnBeginSec = reinterpret_cast<FnPropBeginSection>(GX_VMA(0xe333e0ULL));
    FnPropAddField fnAddField = reinterpret_cast<FnPropAddField>(GX_VMA(0xe327d0ULL));
    FnPropEndSection fnEndSec = reinterpret_cast<FnPropEndSection>(GX_VMA(0xe33600ULL));

    fnBeginSec(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6f2aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6f40ULL)), 1, 1);

    void* vtIntPrimary = reinterpret_cast<void*>(GX_VMA(0x52e8760ULL) + 0x10);
    void* vtIntSecondary = reinterpret_cast<void*>(GX_VMA(0x52e4a70ULL) + 0x10);
    void* vtFloatPrimary = reinterpret_cast<void*>(GX_VMA(0x52e83f0ULL) + 0x10);
    void* vtFloatSecondary = reinterpret_cast<void*>(GX_VMA(0x52e4890ULL) + 0x10);

    // 1. Enable (+0x68)
    ParamGuiPropDesc descEnable = {
        vtIntPrimary, self + 0x68, 0x03, {0, 0, 0}, 1, 1, {0}, vtIntSecondary, 0U, 1U, 0U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49cf3a4ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6f64ULL)), &descEnable, 1, 0);

    // 2. BlurTimeRatio (+0x70)
    ParamGuiPropDesc descBlurTimeRatio = {
        vtFloatPrimary, self + 0x70, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x3a83126fU, 0x3f800000U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6f94ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6fb0ULL)), &descBlurTimeRatio, 1, 1);

    // 3. NumBaseSamples (+0x78)
    ParamGuiPropDesc descNumBaseSamples = {
        vtIntPrimary, self + 0x78, 0x03, {0, 0, 0}, 0, 1, {0}, vtIntSecondary, 3U, 7U, 1U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6fd8ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6ff6ULL)), &descNumBaseSamples, 1, 1);

    // Section: Detail Info / Yebis - MotionBlur-Detail Info
    fnBeginSec(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49cb990ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c7020ULL)), 1, 1);

    // 4. MaxBlurLength (+0x74)
    ParamGuiPropDesc descMaxBlurLength = {
        vtFloatPrimary, self + 0x74, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x3a83126fU, 0x40a00000U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c705eULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c707aULL)), &descMaxBlurLength, 1, 1);

    // 5. MotionBlurScale (+0x98)
    ParamGuiPropDesc descMotionBlurScale = {
        vtFloatPrimary, self + 0x98, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x3c23d70aU, 0x40a00000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c70a2ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c70c2ULL)), &descMotionBlurScale, 1, 1);

    // 6. MaxRecurrences (+0x7c)
    ParamGuiPropDesc descMaxRecurrences = {
        vtIntPrimary, self + 0x7c, 0x03, {0, 0, 0}, 0, 1, {0}, vtIntSecondary, 1U, 8U, 1U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c70eeULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c710cULL)), &descMaxRecurrences, 1, 1);

    // 7. SampleInterleaved (+0x80)
    ParamGuiPropDesc descSampleInterleaved = {
        vtFloatPrimary, self + 0x80, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x3c23d70aU, 0x40000000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c7136ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c715aULL)), &descSampleInterleaved, 1, 1);

    // 8. CameraRotationPerspectiveThreshold (+0x84)
    ParamGuiPropDesc descCamRotThresh = {
        vtFloatPrimary, self + 0x84, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x38d1b717U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c718aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c71d0ULL)), &descCamRotThresh, 1, 1);

    // 9. CameraTranslationThreshold (+0x88)
    ParamGuiPropDesc descCamTransThresh = {
        vtFloatPrimary, self + 0x88, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x3c23d70aU, 0x447a0000U, 0x38d1b717U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c7222ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c7258ULL)), &descCamTransThresh, 1, 1);

    // 10. FalloffDistanceMin (+0x90)
    ParamGuiPropDesc descFalloffMin = {
        vtFloatPrimary, self + 0x90, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x42c80000U, 0x3dcccccdU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c729aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c72c0ULL)), &descFalloffMin, 1, 1);

    // 11. FalloffDistanceMax (+0x94)
    ParamGuiPropDesc descFalloffMax = {
        vtFloatPrimary, self + 0x94, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x42c80000U, 0x3dcccccdU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c72f2ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c7318ULL)), &descFalloffMax, 1, 1);

    // 12. Bidirectional PermissionEnable (+0x9c)
    ParamGuiPropDesc descBidirEnable = {
        vtIntPrimary, self + 0x9c, 0x03, {0, 0, 0}, 1, 1, {0}, vtIntSecondary, 0U, 1U, 0U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c734aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c7388ULL)), &descBidirEnable, 1, 0);

    fnEndSec(propSys, slotId, 1);
    fnEndSec(propSys, slotId, 1);

    reinterpret_cast<FnSceneInitSubCompA>(GX_VMA(0x218e2b0ULL))(sceneSys + 0x130, propSys, 0);
    reinterpret_cast<FnSceneInitSubCompB>(GX_VMA(0xe41f00ULL))(sceneSys + 0x1b8, propSys, 0, slotId, 0, 0.0f);
    reinterpret_cast<FnSceneSyncBank>(GX_VMA(0x21874c0ULL))(sceneSys, slotId);
    return true;
}

// 10. 0x21f8f80 (395 B) — GXParamBankManagerC_ClearAllBanks
extern "C" void GXParamBankManagerC_ClearAllBanks(void* selfPtr) {
    ParamOuterHashTable* self = static_cast<ParamOuterHashTable*>(selfPtr);
    ParamHashNode* outerCur = self->sentinel->next;
    while (outerCur != self->sentinel) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerCur->value);
        ParamHashNode* innerCur = inner->sentinel->next;
        while (innerCur != inner->sentinel) {
            DLVirtualObj* elem = static_cast<DLVirtualObj*>(innerCur->value);
            if (elem != nullptr) {
                elem->vptr->dtor(elem);
                self->ownerAlloc->vptr->freeBlock(self->ownerAlloc, elem);
                innerCur->value = nullptr;
            }
            ParamBucketIter nextIt{};
            DLHashTable_EraseNode_InnerC(&nextIt, inner, innerCur);
            innerCur = nextIt.node;
            inner = static_cast<ParamInnerHashTable*>(outerCur->value);
        }

        if (inner != nullptr) {
            DLAllocator* ownerAlloc = self->ownerAlloc;
            if (inner->buckets != nullptr) {
                inner->bucketAlloc->vptr->freeBlock(inner->bucketAlloc, inner->buckets);
                inner->buckets = nullptr;
                inner->bucketCapBytes = 0;
                inner->bucketEndOff = 0;
            }
            ParamHashNode* head = inner->sentinel;
            ParamHashNode* n = head->next;
            head->next = head;
            inner->sentinel->prev = inner->sentinel;
            inner->count = 0;
            while (n != inner->sentinel) {
                ParamHashNode* nextN = n->next;
                inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, n);
                n = nextN;
            }
            inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, inner->sentinel);
            ownerAlloc->vptr->freeBlock(ownerAlloc, inner);
            outerCur->value = nullptr;
        }

        ParamBucketIter nextOuter{};
        DLHashTable_EraseNode_OuterC(&nextOuter, reinterpret_cast<uint8_t*>(self) + 0x08, outerCur);
        outerCur = nextOuter.node;
    }
}

// 11. 0x21f8e80 (255 B) — GXParamBankManagerC_Destruct
extern "C" void GXParamBankManagerC_Destruct(uint8_t* self) {
    GXParamBankManagerC_ClearAllBanks(self);
    reinterpret_cast<FnElemDestruct>(GX_VMA(0xe825a0ULL))(self + 0x190);

    uint64_t strCap = *reinterpret_cast<uint64_t*>(self + 0x100);
    if (strCap >= 8) {
        void* heapBuf = *reinterpret_cast<void**>(self + 0x0e8);
        if (strCap + 1 >= 0x41) {
            DLAllocator* sAlloc = *reinterpret_cast<DLAllocator**>(self + 0x108);
            if (sAlloc && heapBuf) {
                sAlloc->vptr->freeBlock(sAlloc, heapBuf);
            }
        } else {
            self[0x189] &= 0xfdU;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0x100) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x0f8) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x0e8) = 0;

    ParamOuterHashTable* outer = reinterpret_cast<ParamOuterHashTable*>(self);
    if (outer->buckets != nullptr) {
        outer->bucketAlloc->vptr->freeBlock(outer->bucketAlloc, outer->buckets);
        outer->buckets = nullptr;
        outer->bucketCapBytes = 0;
        outer->bucketEndOff = 0;
    }

    ParamHashNode* head = outer->sentinel;
    ParamHashNode* cur = head->next;
    head->next = head;
    outer->sentinel->prev = outer->sentinel;
    outer->count = 0;
    while (cur != outer->sentinel) {
        ParamHashNode* nextNode = cur->next;
        outer->nodeAlloc->vptr->freeBlock(outer->nodeAlloc, cur);
        cur = nextNode;
    }
    outer->nodeAlloc->vptr->freeBlock(outer->nodeAlloc, outer->sentinel);
}

// 12. 0x21f8b40 (830 B) — GXParamBankManagerC_Construct
extern "C" void GXParamBankManagerC_Construct(uint8_t* self, DLAllocator* alloc) {
    ParamOuterHashTable* outer = reinterpret_cast<ParamOuterHashTable*>(self);
    outer->ownerAlloc = alloc;

    uint8_t heapCaps[136] = {0};
    alloc->vptr->getHeapCaps(heapCaps, alloc, 0);
    if ((heapCaps[0] & 0x20) == 0) {
        reinterpret_cast<FnLogAssert>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    outer->count = 0;
    outer->sentinel = nullptr;
    outer->nodeAlloc = alloc;

    ParamHashNode* sentinel = static_cast<ParamHashNode*>(alloc->vptr->allocateAligned(alloc, 0x20, 0x8));
    if (sentinel != nullptr) {
        sentinel->next = sentinel;
        sentinel->prev = sentinel;
    } else {
        ReportTransactionFailure();
    }
    outer->sentinel = sentinel;
    outer->buckets = nullptr;
    outer->bucketCapBytes = 0;
    outer->bucketEndOff = 0;
    outer->bucketAlloc = alloc;
    outer->maxLoadFactor = 1.0f;

    ParamBucketIter fillVal{0, 0, sentinel};
    ParamBucketIter dummyOut{};
    ParamBucketIter posIter{0, 0, nullptr};
    DLHashBucketVector_InsertFill_OuterC(
        &dummyOut, self + 0x28, 0x10, &fillVal, posIter);
    outer->mask = 7;
    outer->maxBucketIdx = 8;

    *reinterpret_cast<uint32_t*>(self + 0xd8) = 0;
    std::memcpy(self + 0x108, heapCaps, 0x81);
    self[0x189] = 0;
    *reinterpret_cast<uint64_t*>(self + 0x100) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x0f8) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x0e8) = 0;
    self[0x18a] = 1;

    reinterpret_cast<FnResizeWideStr>(GX_VMA(0x2935350ULL))(self + 0xe0, 0x3f, 0);
    bool isHeapStr = (*reinterpret_cast<uint64_t*>(self + 0x100) >= 8);
    *reinterpret_cast<uint64_t*>(self + 0x0f8) = 0;
    uint16_t* strBuf = isHeapStr ? *reinterpret_cast<uint16_t**>(self + 0xe8) : reinterpret_cast<uint16_t*>(self + 0xe8);
    strBuf[0] = 0;

    reinterpret_cast<FnElemConstruct>(GX_VMA(0xe82420ULL))(self + 0x190);
    reinterpret_cast<FnElemInit>(GX_VMA(0xe82510ULL))(self + 0x190);

    *reinterpret_cast<uint32_t*>(self + 0x68) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x6c) = 2;
    *reinterpret_cast<uint32_t*>(self + 0x70) = 0x3d4ccccdU; // 0.05f
    *reinterpret_cast<uint32_t*>(self + 0x74) = 0x3dcccccdU; // 0.1f
    *reinterpret_cast<uint32_t*>(self + 0x78) = 4;
    *reinterpret_cast<uint32_t*>(self + 0x7c) = 1;
    *reinterpret_cast<uint32_t*>(self + 0x80) = 0x3f800000U; // 1.0f
    *reinterpret_cast<uint32_t*>(self + 0x84) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x88) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x8c) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x90) = 0x40000000ULL; // FalloffDistanceMin=2.0f, FalloffDistanceMax=0.0f
    *reinterpret_cast<uint64_t*>(self + 0x98) = 0x000000013f800000ULL; // MotionBlurScale=1.0f, BidirEnable=1

    *reinterpret_cast<uint64_t*>(self + 0xd0) = *reinterpret_cast<uint64_t*>(self + 0x98);
    *reinterpret_cast<uint64_t*>(self + 0xc8) = *reinterpret_cast<uint64_t*>(self + 0x90);
    *reinterpret_cast<uint64_t*>(self + 0xc0) = *reinterpret_cast<uint64_t*>(self + 0x88);
    *reinterpret_cast<uint64_t*>(self + 0xb8) = *reinterpret_cast<uint64_t*>(self + 0x80);
    *reinterpret_cast<uint64_t*>(self + 0xb0) = *reinterpret_cast<uint64_t*>(self + 0x78);
    *reinterpret_cast<uint64_t*>(self + 0xa8) = *reinterpret_cast<uint64_t*>(self + 0x70);
    *reinterpret_cast<uint64_t*>(self + 0xa0) = *reinterpret_cast<uint64_t*>(self + 0x68);
    *reinterpret_cast<uint32_t*>(self + 0xd8) = 0;

    reinterpret_cast<FnAssignWideStr>(GX_VMA(0x29354e0ULL))(
        self + 0xe0, reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// GROUP D: LightShaft[Yebis] Parameter Bank Manager (0x21f4e20 .. 0x21f85f0)
// ============================================================================

// 13. 0x21f85f0 (1352 B) — GXParamBankManagerD_InnerHashTable_RehashInsert
extern "C" ParamBucketIter* GXParamBankManagerD_InnerHashTable_RehashInsert(
    ParamBucketIter* outIter, void* vecBase, uint64_t count,
    const ParamBucketIter* fillVal, ParamBucketIter posIter) {
    return ParamBucketVector_InsertFillImpl(outIter, vecBase, count, fillVal, posIter);
}

// 14. 0x21f80a0 (1352 B) — GXParamBankManagerD_OuterHashTable_RehashInsert
extern "C" ParamBucketIter* GXParamBankManagerD_OuterHashTable_RehashInsert(
    ParamBucketIter* outIter, void* vecBase, uint64_t count,
    const ParamBucketIter* fillVal, ParamBucketIter posIter) {
    return ParamBucketVector_InsertFillImpl(outIter, vecBase, count, fillVal, posIter);
}

// 15. 0x21f7c90 (1040 B) — GXParamBankManagerD_InnerHashTable_InsertNode
extern "C" ParamIterPair* GXParamBankManagerD_InnerHashTable_InsertNode(
    ParamIterPair* outRes, ParamInnerHashTable* table, const uint32_t* keyPtr, ParamBucketIter nodeIter) {
    return ParamHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// 16. 0x21f7880 (1040 B) — GXParamBankManagerD_OuterHashTable_InsertNode
extern "C" ParamIterPair* GXParamBankManagerD_OuterHashTable_InsertNode(
    ParamIterPair* outRes, void* tableMinus8, const uint32_t* keyPtr, ParamBucketIter nodeIter) {
    return ParamHashTable_InsertNodeImpl(
        outRes, reinterpret_cast<ParamInnerHashTable*>(tableMinus8), keyPtr, nodeIter);
}

// 17. 0x21f7710 (363 B) — GXParamBankManagerD_OuterHashTable_EraseNode
extern "C" ParamBucketIter* GXParamBankManagerD_OuterHashTable_EraseNode(
    ParamBucketIter* outIter, void* tableMinus8, ParamHashNode* targetNode) {
    return ParamHashTable_EraseNodeImpl(
        outIter, reinterpret_cast<ParamInnerHashTable*>(tableMinus8), targetNode);
}

// 18. 0x21f75a0 (363 B) — GXParamBankManagerD_InnerHashTable_EraseNode
extern "C" ParamBucketIter* GXParamBankManagerD_InnerHashTable_EraseNode(
    ParamBucketIter* outIter, ParamInnerHashTable* table, ParamHashNode* targetNode) {
    return ParamHashTable_EraseNodeImpl(outIter, table, targetNode);
}

// 19. 0x21f7570 (40 B) — GXParamBankManagerD_DebugCallback_Delete (InvokeMethodD)
extern "C" uint64_t GXParamBankManagerD_DebugCallback_Delete(uint8_t* self) {
    if ((self[0x0b] & 0x40) != 0) {
        uint8_t* target = *reinterpret_cast<uint8_t**>(self + 0x10) + *reinterpret_cast<int64_t*>(self + 0x28);
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

// 20. 0x21f7560 (5 B) — GXParamBankManagerD_DebugCallback_Invoke (OperatorDeleteD)
extern "C" void GXParamBankManagerD_DebugCallback_Invoke(uint8_t* self) {
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x2bbe588ULL))(self);
}

// 21. 0x21f7540 (22 B) — GXParamBankManagerD_DebugCallback_GetTypeInfo (DestroyBaseD)
extern "C" void* GXParamBankManagerD_DebugCallback_GetTypeInfo(void* selfPtr) {
    uint8_t* self = static_cast<uint8_t*>(selfPtr);
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xbfffffffU;
    *reinterpret_cast<uintptr_t*>(self + 0x00) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;
    return selfPtr;
}

// 22. 0x21f6e00 (1708 B) — GXParamBankManagerD_OpenParamEditListDialog
extern "C" void GXParamBankManagerD_OpenParamEditListDialog(void* selfPtr) {
    GXParamBankManager_OpenParamEditListDialogImpl_DE(
        static_cast<uint8_t*>(selfPtr),
        0x54,
        0x158,
        0x210,
        reinterpret_cast<FnElemBuildGui>(GX_VMA(0xe81b80ULL)));
}

// 23. 0x21f6b90 (612 B) — GXParamBankManagerD_RegisterDebugMenu
extern "C" void GXParamBankManagerD_RegisterDebugMenu(void* self, void* parentGui) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x54);

    reinterpret_cast<FnSceneGetEditSlot>(GX_VMA(0x21877f0ULL))(sceneSys, parentGui, slotId, 0);

    DLAllocator* guiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    FnGuiCreateSubMenu fnCreateSubMenu = reinterpret_cast<FnGuiCreateSubMenu>(GX_VMA(0x236d620ULL));
    FnGuiAddActionItem fnAddAction = reinterpret_cast<FnGuiAddActionItem>(GX_VMA(0x235e4c0ULL));

    // 1) "ParameterSystem Regist" -> GXParamBankManagerD_RegisterAllParams (0x21f67b0)
    uint64_t subDesc1[3] = {0, 0, 0};
    void* menu1 = fnCreateSubMenu(parentGui, subDesc1);
    subDesc1[1] = 0;

    uint64_t labelDesc1[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL))), 0};
    uint8_t* cb1 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb1 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb1 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb1 + 0x00) = GX_VMA(0x53b71a0ULL);
        *reinterpret_cast<void**>(cb1 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb1 + 0x18) = 0;
        *reinterpret_cast<void**>(cb1 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerD_RegisterAllParams);
        *reinterpret_cast<uint64_t*>(cb1 + 0x28) = 0;
    }
    fnAddAction(menu1, labelDesc1, cb1);
    labelDesc1[1] = 0;

    // 2) "ParameterSystem Unregist" -> GXParamBankManagerD_UnregisterAllParams (0x21f6840)
    uint64_t subDesc2[3] = {0, 0, 0};
    void* menu2 = fnCreateSubMenu(parentGui, subDesc2);
    subDesc2[1] = 0;

    uint64_t labelDesc2[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL))), 0};
    uint8_t* cb2 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb2 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb2 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb2 + 0x00) = GX_VMA(0x53b71a0ULL);
        *reinterpret_cast<void**>(cb2 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb2 + 0x18) = 0;
        *reinterpret_cast<void**>(cb2 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerD_UnregisterAllParams);
        *reinterpret_cast<uint64_t*>(cb2 + 0x28) = 0;
    }
    fnAddAction(menu2, labelDesc2, cb2);
    labelDesc2[1] = 0;

    // 3) "Open List" -> GXParamBankManagerD_OpenParamEditListDialog (0x21f6e00)
    uint64_t subDesc3[3] = {0, 0, 0};
    void* menu3 = fnCreateSubMenu(parentGui, subDesc3);
    subDesc3[1] = 0;

    uint64_t labelDesc3[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL))), 0};
    uint8_t* cb3 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb3 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb3 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb3 + 0x00) = GX_VMA(0x53b71a0ULL);
        *reinterpret_cast<void**>(cb3 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb3 + 0x18) = 0;
        *reinterpret_cast<void**>(cb3 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerD_OpenParamEditListDialog);
        *reinterpret_cast<uint64_t*>(cb3 + 0x28) = 0;
    }
    fnAddAction(menu3, labelDesc3, cb3);
    labelDesc3[1] = 0;
}

// 24. 0x21f68b0 (732 B) — GXParamBankManagerD_ResolveParamEntry
extern "C" void* GXParamBankManagerD_ResolveParamEntry(ParamOuterHashTable* self, const uint64_t* packedKeyPtr) {
    uint64_t packed = *packedKeyPtr;
    uint32_t bankId = static_cast<uint32_t>(packed);
    uint32_t paramId = static_cast<uint32_t>(packed >> 32);

    if (bankId == 0xffffffffU) {
        if (paramId == 0xffffffffU) {
            paramId = 0;
            bankId = 1;
        } else {
            bankId = 0xffffffffU;
        }
    }

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerCur = outerBuckets[outerIdx].beginNode;

    if (outerCur != outerSentinel) {
        ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
        while (outerCur != outerEnd) {
            if (outerCur->key >= bankId) {
                if (bankId < outerCur->key) {
                    outerCur = outerSentinel;
                }
                break;
            }
            outerCur = outerCur->next;
        }
        if (outerCur != outerSentinel && outerCur != outerEnd) {
            ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerCur->value);
            uint64_t innerIdx = ComputeLehmerHashBucket(paramId, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
            ParamHashNode* innerSentinel = inner->sentinel;
            ParamHashBucket* innerBuckets = inner->buckets;
            ParamHashNode* innerCur = innerBuckets[innerIdx].beginNode;
            ParamHashNode* foundInner = innerSentinel;

            if (innerCur != innerSentinel) {
                ParamHashNode* innerEnd = innerBuckets[innerIdx].endNode->next;
                while (innerCur != innerEnd) {
                    if (innerCur->key >= paramId) {
                        if (paramId >= innerCur->key) {
                            foundInner = innerCur;
                        }
                        break;
                    }
                    innerCur = innerCur->next;
                }
            }
            if (foundInner != inner->sentinel) {
                return foundInner->value;
            }
        }
    }

    uint64_t fbOuterIdx = ComputeLehmerHashBucket(1U, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* fbOuterCur = outerBuckets[fbOuterIdx].beginNode;
    if (fbOuterCur != outerSentinel) {
        ParamHashNode* fbOuterEnd = outerBuckets[fbOuterIdx].endNode->next;
        while (fbOuterCur != fbOuterEnd) {
            if (fbOuterCur->key != 0) {
                if (fbOuterCur->key > 1U) {
                    fbOuterCur = outerSentinel;
                }
                break;
            }
            fbOuterCur = fbOuterCur->next;
        }
        if (fbOuterCur != outerSentinel && fbOuterCur != fbOuterEnd) {
            ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(fbOuterCur->value);
            uint64_t fbInnerIdx = ComputeLehmerHashBucket(0U, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
            ParamHashNode* innerSentinel = inner->sentinel;
            ParamHashBucket* innerBuckets = inner->buckets;
            ParamHashNode* fbInnerCur = innerBuckets[fbInnerIdx].beginNode;
            ParamHashNode* foundFb = innerSentinel;

            if (fbInnerCur != innerSentinel) {
                ParamHashNode* fbInnerEnd = innerBuckets[fbInnerIdx].endNode->next;
                if (fbInnerCur != fbInnerEnd && fbInnerCur->key == 0U) {
                    foundFb = fbInnerCur;
                }
            }
            if (foundFb != inner->sentinel) {
                return foundFb->value;
            }
        }
    }

    return reinterpret_cast<uint8_t*>(self) + 0x210;
}

// 25. 0x21f6840 (103 B) — GXParamBankManagerD_UnregisterAllParams
extern "C" void GXParamBankManagerD_UnregisterAllParams(void* self) {
    ParamOuterHashTable* outer = static_cast<ParamOuterHashTable*>(self);
    ParamHashNode* outerSentinel = outer->sentinel;
    for (ParamHashNode* outerNode = outerSentinel->next; outerNode != outerSentinel; outerNode = outerNode->next) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
        ParamHashNode* innerSentinel = inner->sentinel;
        for (ParamHashNode* innerNode = innerSentinel->next; innerNode != innerSentinel; innerNode = innerNode->next) {
            reinterpret_cast<FnElemUnregister>(GX_VMA(0xe80f90ULL))(innerNode->value);
        }
        outerSentinel = outer->sentinel;
    }
}

// 26. 0x21f67b0 (138 B) — GXParamBankManagerD_RegisterAllParams
extern "C" void GXParamBankManagerD_RegisterAllParams(void* self) {
    ParamOuterHashTable* outer = static_cast<ParamOuterHashTable*>(self);
    ParamHashNode* outerSentinel = outer->sentinel;
    for (ParamHashNode* outerNode = outerSentinel->next; outerNode != outerSentinel; outerNode = outerNode->next) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
        ParamHashNode* innerSentinel = inner->sentinel;
        for (ParamHashNode* innerNode = innerSentinel->next; innerNode != innerSentinel; innerNode = innerNode->next) {
            uint8_t* elem = static_cast<uint8_t*>(innerNode->value);
            *reinterpret_cast<uint32_t*>(elem + 0xc8) = innerNode->key;
            *reinterpret_cast<uint32_t*>(elem + 0xcc) = outerNode->key;
            reinterpret_cast<FnElemApply>(GX_VMA(0xe80ff0ULL))(elem, 0.0f);
            reinterpret_cast<FnElemRegister>(GX_VMA(0xe81160ULL))(elem, outer->ownerAlloc);
        }
        outerSentinel = outer->sentinel;
    }
}

// 27. 0x21f67a0 (1 B) — GXParamBankManagerD_OnEventNoOp
extern "C" void GXParamBankManagerD_OnEventNoOp(void*) {
    return;
}

// 28. 0x21f6770 (40 B) — GXParamBankManagerD_SetActiveParamId
extern "C" void GXParamBankManagerD_SetActiveParamId(void* /*self*/, uint32_t paramId, uint32_t bankId) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x54);
    reinterpret_cast<FnSceneSetActiveId>(GX_VMA(0x2186da0ULL))(sceneSys, slotId, paramId, bankId, -1LL);
}

// 29. 0x21f64b0 (698 B) — GXParamBankManagerD_UnloadBankById
extern "C" void GXParamBankManagerD_UnloadBankById(
    ParamOuterHashTable* self,
    void* /*unused*/,
    uint32_t bankId,
    uint32_t overrideSlotIdx,
    float blendTime)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x54);

    reinterpret_cast<FnSceneBeginUnload>(GX_VMA(0x2185b50ULL))(
        sceneSys, slotId, bankId, overrideSlotIdx, blendTime);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = 1;

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerNode = outerBuckets[outerIdx].beginNode;
    if (outerNode == outerSentinel) {
        return;
    }

    ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
    while (outerNode != outerEnd && outerNode->key < bankId) {
        outerNode = outerNode->next;
    }
    if (outerNode == outerEnd || outerNode->key > bankId || outerNode == outerSentinel) {
        return;
    }

    ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
    if (inner != nullptr) {
        ParamHashNode* innerCur = inner->sentinel->next;
        while (innerCur != inner->sentinel) {
            bool keep = reinterpret_cast<FnSceneCheckEntry>(GX_VMA(0x2185e00ULL))(sceneSys, slotId, innerCur->key, bankId);
            DLVirtualObj* elem = static_cast<DLVirtualObj*>(innerCur->value);
            if (keep) {
                *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(elem) + 0xc0) = 1;
                innerCur = innerCur->next;
            } else {
                if (elem != nullptr) {
                    elem->vptr->dtor(elem);
                    self->ownerAlloc->vptr->freeBlock(self->ownerAlloc, elem);
                    innerCur->value = nullptr;
                }
                ParamBucketIter nextIt{};
                GXParamBankManagerD_InnerHashTable_EraseNode(&nextIt, inner, innerCur);
                innerCur = nextIt.node;
            }
        }

        if (inner->count != 0) {
            return;
        }

        DLAllocator* ownerAlloc = self->ownerAlloc;
        if (inner->buckets != nullptr) {
            inner->bucketAlloc->vptr->freeBlock(inner->bucketAlloc, inner->buckets);
            inner->buckets = nullptr;
            inner->bucketCapBytes = 0;
            inner->bucketEndOff = 0;
        }

        ParamHashNode* head = inner->sentinel;
        ParamHashNode* n = head->next;
        head->next = head;
        inner->sentinel->prev = inner->sentinel;
        inner->count = 0;
        while (n != inner->sentinel) {
            ParamHashNode* nextN = n->next;
            inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, n);
            n = nextN;
        }
        inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, inner->sentinel);
        ownerAlloc->vptr->freeBlock(ownerAlloc, inner);
        outerNode->value = nullptr;
    }

    ParamBucketIter nextOuter{};
    GXParamBankManagerD_OuterHashTable_EraseNode(&nextOuter, reinterpret_cast<uint8_t*>(self) + 0x08, outerNode);
}

// 30. 0x21f5d30 (1912 B) — GXParamBankManagerD_LoadBankFromStream
extern "C" void GXParamBankManagerD_LoadBankFromStream(
    ParamOuterHashTable* self, void* streamPtr, uint32_t bankId, uint32_t flags, float weight) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x54);

    uint32_t entryCount = reinterpret_cast<FnSceneGetBankCount>(GX_VMA(0x2184cf0ULL))(sceneSys, slotId, streamPtr);
    if (entryCount == 0) {
        reinterpret_cast<FnSceneFinalizeLoad>(GX_VMA(0x2184ff0ULL))(sceneSys, slotId, bankId, flags, streamPtr, weight);
        return;
    }

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerNode = outerBuckets[outerIdx].beginNode;
    if (outerNode != outerSentinel) {
        ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
        while (outerNode != outerEnd && outerNode->key < bankId) {
            outerNode = outerNode->next;
        }
        if (outerNode == outerEnd || outerNode->key > bankId) {
            outerNode = outerSentinel;
        }
    }

    ParamInnerHashTable* inner = nullptr;
    if (outerNode == outerSentinel) {
        DLAllocator* ownerAlloc = self->ownerAlloc;
        inner = static_cast<ParamInnerHashTable*>(ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x60, 0x8));
        if (inner != nullptr) {
            uint8_t heapCaps[32] = {0};
            ownerAlloc->vptr->getHeapCaps(heapCaps, ownerAlloc, 0);
            if ((heapCaps[0] & 0x20) == 0) {
                reinterpret_cast<FnLogAssert>(GX_VMA(0x20b55b0ULL))(
                    0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }
            inner->count = 0;
            inner->sentinel = nullptr;
            inner->nodeAlloc = ownerAlloc;
            ParamHashNode* innerSent = static_cast<ParamHashNode*>(
                ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x20, 0x8));
            if (innerSent != nullptr) {
                innerSent->next = innerSent;
                innerSent->prev = innerSent;
            } else {
                ReportTransactionFailure();
            }
            inner->sentinel = innerSent;
            inner->buckets = nullptr;
            inner->bucketCapBytes = 0;
            inner->bucketEndOff = 0;
            inner->bucketAlloc = ownerAlloc;
            inner->maxLoadFactor = 1.0f;

            ParamBucketIter fillVal{0, 0, innerSent};
            ParamBucketIter dummyOut{};
            ParamBucketIter posIter{0, 0, nullptr};
            GXParamBankManagerD_InnerHashTable_RehashInsert(
                &dummyOut, reinterpret_cast<uint8_t*>(inner) + 0x20, 0x10, &fillVal, posIter);
            inner->mask = 7;
            inner->maxBucketIdx = 8;
        }

        ParamHashNode* headFirst = self->sentinel->next;
        ParamHashNode* newOuterNode = static_cast<ParamHashNode*>(
            self->nodeAlloc->vptr->allocateAligned(self->nodeAlloc, 0x20, 0x8));
        if (newOuterNode == nullptr) {
            ReportTransactionFailure();
            return;
        }
        newOuterNode->next = headFirst ? headFirst : newOuterNode;
        newOuterNode->prev = headFirst ? headFirst->prev : newOuterNode;
        newOuterNode->key = bankId;
        newOuterNode->value = inner;
        if (self->count == 0x7fffffffffffffeULL) {
            reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        self->count += 1;
        headFirst->prev = newOuterNode;
        newOuterNode->prev->next = newOuterNode;

        ParamIterPair insRes{};
        ParamHashNode* insertedOuter = self->sentinel->next;
        GXParamBankManagerD_OuterHashTable_InsertNode(
            &insRes, reinterpret_cast<uint8_t*>(self) + 0x08, &insertedOuter->key, ParamBucketIter{0, 0, insertedOuter});
    } else {
        inner = static_cast<ParamInnerHashTable*>(outerNode->value);
    }

    for (uint32_t i = 0; i < entryCount; ++i) {
        uint32_t paramId = reinterpret_cast<FnSceneGetParamIdAt>(GX_VMA(0x2184de0ULL))(sceneSys, slotId, streamPtr, i);
        uint64_t innerIdx = ComputeLehmerHashBucket(paramId, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
        ParamHashNode* innerSentinel = inner->sentinel;
        ParamHashBucket* innerBuckets = inner->buckets;
        ParamHashNode* innerNode = innerBuckets[innerIdx].beginNode;
        if (innerNode != innerSentinel) {
            ParamHashNode* innerEnd = innerBuckets[innerIdx].endNode->next;
            while (innerNode != innerEnd && innerNode->key < paramId) {
                innerNode = innerNode->next;
            }
            if (innerNode == innerEnd || innerNode->key > paramId) {
                innerNode = innerSentinel;
            }
        }

        if (innerNode == innerSentinel) {
            DLAllocator* ownerAlloc = self->ownerAlloc;
            uint8_t* elem = static_cast<uint8_t*>(ownerAlloc->vptr->allocateAligned(ownerAlloc, 0xd0, 0x10));
            if (elem == nullptr) {
                ReportTransactionFailure();
                continue;
            }
            reinterpret_cast<FnElemConstruct>(GX_VMA(0xe80c60ULL))(elem);
            reinterpret_cast<FnElemInit>(GX_VMA(0xe80d90ULL))(elem);
            *reinterpret_cast<uint32_t*>(elem + 0xc8) = paramId;
            *reinterpret_cast<uint32_t*>(elem + 0xcc) = bankId;
            reinterpret_cast<FnElemApply>(GX_VMA(0xe80ff0ULL))(elem, 0.0f);
            reinterpret_cast<FnElemRegister>(GX_VMA(0xe81160ULL))(elem, self->ownerAlloc);

            ParamHashNode* iHeadFirst = inner->sentinel->next;
            ParamHashNode* newInnerNode = static_cast<ParamHashNode*>(
                inner->nodeAlloc->vptr->allocateAligned(inner->nodeAlloc, 0x20, 0x8));
            if (newInnerNode == nullptr) {
                ReportTransactionFailure();
                continue;
            }
            newInnerNode->next = iHeadFirst ? iHeadFirst : newInnerNode;
            newInnerNode->prev = iHeadFirst ? iHeadFirst->prev : newInnerNode;
            newInnerNode->key = paramId;
            newInnerNode->value = elem;
            if (inner->count == 0x7fffffffffffffeULL) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            inner->count += 1;
            iHeadFirst->prev = newInnerNode;
            newInnerNode->prev->next = newInnerNode;

            ParamIterPair insRes{};
            ParamHashNode* insertedInner = inner->sentinel->next;
            GXParamBankManagerD_InnerHashTable_InsertNode(
                &insRes, inner, &insertedInner->key, ParamBucketIter{0, 0, insertedInner});
        } else {
            uint8_t* elem = static_cast<uint8_t*>(innerNode->value);
            *reinterpret_cast<uint8_t*>(elem + 0xc0) = 1;
        }
    }

    reinterpret_cast<FnSceneFinalizeLoad>(GX_VMA(0x2184ff0ULL))(sceneSys, slotId, bankId, flags, streamPtr, weight);
}

// 31. 0x21f5460 (2255 B) — GXParamBankManagerD_RegisterProperties
extern "C" bool GXParamBankManagerD_RegisterProperties(uint8_t* self) {
    GXParamBankManagerD_ClearAllBanks(self);

    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    void* propSys = *reinterpret_cast<void**>(sceneSys + 0x28);

    uint32_t slotId = 0;
    reinterpret_cast<FnPropRegisterGroup>(GX_VMA(0xe32510ULL))(
        propSys, &slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6a22ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6a38ULL)));
    *reinterpret_cast<uint32_t*>(sceneSys + 0x54) = slotId;

    FnPropBeginSection fnBeginSec = reinterpret_cast<FnPropBeginSection>(GX_VMA(0xe333e0ULL));
    FnPropAddField fnAddField = reinterpret_cast<FnPropAddField>(GX_VMA(0xe327d0ULL));
    FnPropEndSection fnEndSec = reinterpret_cast<FnPropEndSection>(GX_VMA(0xe33600ULL));

    fnBeginSec(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6a5cULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6a74ULL)), 1, 1);

    void* vtIntPrimary = reinterpret_cast<void*>(GX_VMA(0x52e8760ULL) + 0x10);
    void* vtIntSecondary = reinterpret_cast<void*>(GX_VMA(0x52e4a70ULL) + 0x10);
    void* vtFloatPrimary = reinterpret_cast<void*>(GX_VMA(0x52e83f0ULL) + 0x10);
    void* vtFloatSecondary = reinterpret_cast<void*>(GX_VMA(0x52e4890ULL) + 0x10);
    void* vtColorPrimary = reinterpret_cast<void*>(GX_VMA(0x52e8470ULL) + 0x10);

    // 1. Enable (+0x70)
    ParamGuiPropDesc descEnable = {
        vtIntPrimary, self + 0x70, 0x03, {0, 0, 0}, 1, 1, {0}, vtIntSecondary, 0U, 1U, 0U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49cf3a4ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6a9cULL)), &descEnable, 1, 0);

    // 2. ColorScale (+0x80)
    ParamGuiPropDesc descColorScale = {
        vtColorPrimary, self + 0x80, 0x0e, {0, 0, 0}, 0x12, 1, {0}, vtFloatSecondary, 0U, 0U, 0U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6accULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6ae2ULL)), &descColorScale, 1, 1);

    // 3. ShaftLength (+0x90)
    ParamGuiPropDesc descShaftLength = {
        vtFloatPrimary, self + 0x90, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x40e00000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6b04ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6b1cULL)), &descShaftLength, 1, 1);

    // 4. GlareRatio (+0x9c)
    ParamGuiPropDesc descGlareRatio = {
        vtFloatPrimary, self + 0x9c, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6b40ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6b56ULL)), &descGlareRatio, 1, 1);

    // 5. AngleAttenuation (+0xa0) & 6. NoiseMask (+0xa4)
    ParamGuiPropDesc descAngleAtten = {
        vtFloatPrimary, self + 0xa0, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x42480000U, 0x3c23d70aU, 0x3f800000U
    };
    ParamGuiPropDesc descNoiseMask = {
        vtFloatPrimary, self + 0xa4, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6b78ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6b9aULL)), &descAngleAtten, 1, 1);
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6bc8ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6bdcULL)), &descNoiseMask, 1, 1);

    // 7. NoiseFrequency (+0xa8)
    ParamGuiPropDesc descNoiseFreq = {
        vtFloatPrimary, self + 0xa8, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6bfcULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6c1aULL)), &descNoiseFreq, 1, 1);

    // 8. RadX (+0xac)
    ParamGuiPropDesc descRadX = {
        vtFloatPrimary, self + 0xac, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0xc2b40000U, 0x42b40000U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6c44ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6c4eULL)), &descRadX, 1, 1);

    // 9. RadY (+0xb0)
    ParamGuiPropDesc descRadY = {
        vtFloatPrimary, self + 0xb0, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0xc3340000U, 0x43340000U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6c78ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6c82ULL)), &descRadY, 1, 1);

    // 10. MaskDepthThereshold (+0xb4)
    ParamGuiPropDesc descMaskDepth = {
        vtFloatPrimary, self + 0xb4, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x3f800000U, 0x459c4000U, 0x3f800000U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6cacULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6cd4ULL)), &descMaskDepth, 1, 1);

    // 11. DiffractionRing (+0xb8)
    ParamGuiPropDesc descDiffRing = {
        vtFloatPrimary, self + 0xb8, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6d08ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6d28ULL)), &descDiffRing, 1, 1);

    // 12. DiffractionRingRadius (+0xbc)
    ParamGuiPropDesc descDiffRadius = {
        vtFloatPrimary, self + 0xbc, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6d54ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6d80ULL)), &descDiffRadius, 1, 1);

    // 13. DiffractionRingAttenuation (+0xc0)
    ParamGuiPropDesc descDiffAtten = {
        vtFloatPrimary, self + 0xc0, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6db8ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6deeULL)), &descDiffAtten, 1, 1);

    // 14. DiffractionRingSpectrumOrder (+0xc4)
    ParamGuiPropDesc descDiffSpec = {
        vtFloatPrimary, self + 0xc4, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6e30ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6e6aULL)), &descDiffSpec, 1, 1);

    // 15. RingOuterColor (+0xd0)
    ParamGuiPropDesc descRingColor = {
        vtColorPrimary, self + 0xd0, 0x0e, {0, 0, 0}, 0x12, 1, {0}, vtFloatSecondary, 0U, 0U, 0U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6eb0ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6eceULL)), &descRingColor, 1, 1);

    fnEndSec(propSys, slotId, 1);

    reinterpret_cast<FnSceneInitSubCompA>(GX_VMA(0x218e2b0ULL))(sceneSys + 0x130, propSys, 0);
    reinterpret_cast<FnSceneInitSubCompB>(GX_VMA(0xe41f00ULL))(sceneSys + 0x1b8, propSys, 0, slotId, 0, 0.0f);
    reinterpret_cast<FnSceneSyncBank>(GX_VMA(0x21874c0ULL))(sceneSys, slotId);
    return true;
}

// 32. 0x21f52d0 (395 B) — GXParamBankManagerD_ClearAllBanks
extern "C" void GXParamBankManagerD_ClearAllBanks(void* selfPtr) {
    ParamOuterHashTable* self = static_cast<ParamOuterHashTable*>(selfPtr);
    ParamHashNode* outerCur = self->sentinel->next;
    while (outerCur != self->sentinel) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerCur->value);
        ParamHashNode* innerCur = inner->sentinel->next;
        while (innerCur != inner->sentinel) {
            DLVirtualObj* elem = static_cast<DLVirtualObj*>(innerCur->value);
            if (elem != nullptr) {
                elem->vptr->dtor(elem);
                self->ownerAlloc->vptr->freeBlock(self->ownerAlloc, elem);
                innerCur->value = nullptr;
            }
            ParamBucketIter nextIt{};
            GXParamBankManagerD_InnerHashTable_EraseNode(&nextIt, inner, innerCur);
            innerCur = nextIt.node;
            inner = static_cast<ParamInnerHashTable*>(outerCur->value);
        }

        if (inner != nullptr) {
            DLAllocator* ownerAlloc = self->ownerAlloc;
            if (inner->buckets != nullptr) {
                inner->bucketAlloc->vptr->freeBlock(inner->bucketAlloc, inner->buckets);
                inner->buckets = nullptr;
                inner->bucketCapBytes = 0;
                inner->bucketEndOff = 0;
            }
            ParamHashNode* head = inner->sentinel;
            ParamHashNode* n = head->next;
            head->next = head;
            inner->sentinel->prev = inner->sentinel;
            inner->count = 0;
            while (n != inner->sentinel) {
                ParamHashNode* nextN = n->next;
                inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, n);
                n = nextN;
            }
            inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, inner->sentinel);
            ownerAlloc->vptr->freeBlock(ownerAlloc, inner);
            outerCur->value = nullptr;
        }

        ParamBucketIter nextOuter{};
        GXParamBankManagerD_OuterHashTable_EraseNode(&nextOuter, reinterpret_cast<uint8_t*>(self) + 0x08, outerCur);
        outerCur = nextOuter.node;
    }
}

// 33. 0x21f51d0 (255 B) — GXParamBankManagerD_Destruct
extern "C" void GXParamBankManagerD_Destruct(uint8_t* self) {
    GXParamBankManagerD_ClearAllBanks(self);
    reinterpret_cast<FnElemDestruct>(GX_VMA(0xe80e40ULL))(self + 0x210);

    uint64_t strCap = *reinterpret_cast<uint64_t*>(self + 0x178);
    if (strCap >= 8) {
        void* heapBuf = *reinterpret_cast<void**>(self + 0x160);
        if (strCap + 1 >= 0x41) {
            DLAllocator* sAlloc = *reinterpret_cast<DLAllocator**>(self + 0x180);
            if (sAlloc && heapBuf) {
                sAlloc->vptr->freeBlock(sAlloc, heapBuf);
            }
        } else {
            self[0x201] &= 0xfdU;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0x178) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x170) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x160) = 0;

    ParamOuterHashTable* outer = reinterpret_cast<ParamOuterHashTable*>(self);
    if (outer->buckets != nullptr) {
        outer->bucketAlloc->vptr->freeBlock(outer->bucketAlloc, outer->buckets);
        outer->buckets = nullptr;
        outer->bucketCapBytes = 0;
        outer->bucketEndOff = 0;
    }

    ParamHashNode* head = outer->sentinel;
    ParamHashNode* cur = head->next;
    head->next = head;
    outer->sentinel->prev = outer->sentinel;
    outer->count = 0;
    while (cur != outer->sentinel) {
        ParamHashNode* nextNode = cur->next;
        outer->nodeAlloc->vptr->freeBlock(outer->nodeAlloc, cur);
        cur = nextNode;
    }
    outer->nodeAlloc->vptr->freeBlock(outer->nodeAlloc, outer->sentinel);
}

// 34. 0x21f4e20 (930 B) — GXParamBankManagerD_Construct
extern "C" void GXParamBankManagerD_Construct(uint8_t* self, DLAllocator* alloc) {
    ParamOuterHashTable* outer = reinterpret_cast<ParamOuterHashTable*>(self);
    outer->ownerAlloc = alloc;

    uint8_t heapCaps[136] = {0};
    alloc->vptr->getHeapCaps(heapCaps, alloc, 0);
    if ((heapCaps[0] & 0x20) == 0) {
        reinterpret_cast<FnLogAssert>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    outer->count = 0;
    outer->sentinel = nullptr;
    outer->nodeAlloc = alloc;

    ParamHashNode* sentinel = static_cast<ParamHashNode*>(alloc->vptr->allocateAligned(alloc, 0x20, 0x8));
    if (sentinel != nullptr) {
        sentinel->next = sentinel;
        sentinel->prev = sentinel;
    } else {
        ReportTransactionFailure();
    }
    outer->sentinel = sentinel;
    outer->buckets = nullptr;
    outer->bucketCapBytes = 0;
    outer->bucketEndOff = 0;
    outer->bucketAlloc = alloc;
    outer->maxLoadFactor = 1.0f;

    ParamBucketIter fillVal{0, 0, sentinel};
    ParamBucketIter dummyOut{};
    ParamBucketIter posIter{0, 0, nullptr};
    GXParamBankManagerD_OuterHashTable_RehashInsert(
        &dummyOut, self + 0x28, 0x10, &fillVal, posIter);
    outer->mask = 7;
    outer->maxBucketIdx = 8;

    *reinterpret_cast<uint32_t*>(self + 0x150) = 0;
    std::memcpy(self + 0x180, heapCaps, 0x81);
    self[0x201] = 0;
    *reinterpret_cast<uint64_t*>(self + 0x178) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x170) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x160) = 0;
    self[0x202] = 1;

    reinterpret_cast<FnResizeWideStr>(GX_VMA(0x2935350ULL))(self + 0x158, 0x3f, 0);
    bool isHeapStr = (*reinterpret_cast<uint64_t*>(self + 0x178) >= 8);
    *reinterpret_cast<uint64_t*>(self + 0x170) = 0;
    uint16_t* strBuf = isHeapStr ? *reinterpret_cast<uint16_t**>(self + 0x160) : reinterpret_cast<uint16_t*>(self + 0x160);
    strBuf[0] = 0;

    reinterpret_cast<FnElemConstruct>(GX_VMA(0xe80c60ULL))(self + 0x210);
    reinterpret_cast<FnElemInit>(GX_VMA(0xe80d90ULL))(self + 0x210);

    *reinterpret_cast<uint32_t*>(self + 0x70) = 0;
    __m128 ones = _mm_set1_ps(1.0f);
    _mm_storeu_ps(reinterpret_cast<float*>(self + 0x80), ones);
    *reinterpret_cast<uint32_t*>(self + 0x90) = 0x3f800000U; // 1.0f
    *reinterpret_cast<uint32_t*>(self + 0x94) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x98) = 0x3f7fff58U;
    *reinterpret_cast<uint32_t*>(self + 0x9c) = 0x3f400000U; // 0.75f
    *reinterpret_cast<uint32_t*>(self + 0xa0) = 0x41200000U; // 10.0f
    *reinterpret_cast<uint32_t*>(self + 0xa4) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xa8) = 0x41200000U; // 10.0f
    *reinterpret_cast<uint32_t*>(self + 0xac) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb0) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb4) = 0x44fa0000U; // 2000.0f
    *reinterpret_cast<uint32_t*>(self + 0xb8) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xbc) = 0x3e4ccccdU; // 0.2f
    *reinterpret_cast<uint32_t*>(self + 0xc0) = 0x3c23d70aU; // 0.01f
    *reinterpret_cast<uint32_t*>(self + 0xc4) = 0x3f800000U; // 1.0f
    _mm_storeu_ps(reinterpret_cast<float*>(self + 0xd0), ones);

    // Copy +0x70..+0xdf (0x70 bytes) into +0xe0..+0x14f
    std::memcpy(self + 0xe0, self + 0x70, 0x70);
    *reinterpret_cast<uint32_t*>(self + 0x150) = 0;

    reinterpret_cast<FnAssignWideStr>(GX_VMA(0x29354e0ULL))(
        self + 0x158, reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// GROUP E: FeedBackBlur[Yebis] Parameter Bank Manager (0x21f18c0 .. 0x21f48d0)
// ============================================================================

// 35. 0x21f48d0 (1352 B) — GXParamBankManagerE_InnerHashTable_RehashInsert
extern "C" ParamBucketIter* GXParamBankManagerE_InnerHashTable_RehashInsert(
    ParamBucketIter* outIter, void* vecBase, uint64_t count,
    const ParamBucketIter* fillVal, ParamBucketIter posIter) {
    return ParamBucketVector_InsertFillImpl(outIter, vecBase, count, fillVal, posIter);
}

// 36. 0x21f4380 (1352 B) — GXParamBankManagerE_OuterHashTable_RehashInsert
extern "C" ParamBucketIter* GXParamBankManagerE_OuterHashTable_RehashInsert(
    ParamBucketIter* outIter, void* vecBase, uint64_t count,
    const ParamBucketIter* fillVal, ParamBucketIter posIter) {
    return ParamBucketVector_InsertFillImpl(outIter, vecBase, count, fillVal, posIter);
}

// 37. 0x21f3f70 (1040 B) — GXParamBankManagerE_InnerHashTable_InsertNode
extern "C" ParamIterPair* GXParamBankManagerE_InnerHashTable_InsertNode(
    ParamIterPair* outRes, ParamInnerHashTable* table, const uint32_t* keyPtr, ParamBucketIter nodeIter) {
    return ParamHashTable_InsertNodeImpl(outRes, table, keyPtr, nodeIter);
}

// 38. 0x21f3b60 (1040 B) — GXParamBankManagerE_OuterHashTable_InsertNode
extern "C" ParamIterPair* GXParamBankManagerE_OuterHashTable_InsertNode(
    ParamIterPair* outRes, void* tableMinus8, const uint32_t* keyPtr, ParamBucketIter nodeIter) {
    return ParamHashTable_InsertNodeImpl(
        outRes, reinterpret_cast<ParamInnerHashTable*>(tableMinus8), keyPtr, nodeIter);
}

// 39. 0x21f39f0 (363 B) — GXParamBankManagerE_OuterHashTable_EraseNode
extern "C" ParamBucketIter* GXParamBankManagerE_OuterHashTable_EraseNode(
    ParamBucketIter* outIter, void* tableMinus8, ParamHashNode* targetNode) {
    return ParamHashTable_EraseNodeImpl(
        outIter, reinterpret_cast<ParamInnerHashTable*>(tableMinus8), targetNode);
}

// 40. 0x21f3880 (363 B) — GXParamBankManagerE_InnerHashTable_EraseNode
extern "C" ParamBucketIter* GXParamBankManagerE_InnerHashTable_EraseNode(
    ParamBucketIter* outIter, ParamInnerHashTable* table, ParamHashNode* targetNode) {
    return ParamHashTable_EraseNodeImpl(outIter, table, targetNode);
}

// 41. 0x21f3850 (40 B) — GXParamBankManagerE_DebugCallback_Delete
extern "C" uint64_t GXParamBankManagerE_DebugCallback_Delete(uint8_t* self) {
    if ((self[0x0b] & 0x40) != 0) {
        uint8_t* target = *reinterpret_cast<uint8_t**>(self + 0x10) + *reinterpret_cast<int64_t*>(self + 0x28);
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

// 42. 0x21f3840 (5 B) — GXParamBankManagerE_DebugCallback_Invoke
extern "C" void GXParamBankManagerE_DebugCallback_Invoke(uint8_t* self) {
    reinterpret_cast<void (*)(void*)>(GX_VMA(0x2bbe588ULL))(self);
}

// 43. 0x21f3820 (22 B) — GXParamBankManagerE_DebugCallback_GetTypeInfo
extern "C" void* GXParamBankManagerE_DebugCallback_GetTypeInfo(void* selfPtr) {
    uint8_t* self = static_cast<uint8_t*>(selfPtr);
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xbfffffffU;
    *reinterpret_cast<uintptr_t*>(self + 0x00) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;
    return selfPtr;
}

// 44. 0x21f30e0 (1708 B) — GXParamBankManagerE_OpenParamEditListDialog
extern "C" void GXParamBankManagerE_OpenParamEditListDialog(void* selfPtr) {
    GXParamBankManager_OpenParamEditListDialogImpl_DE(
        static_cast<uint8_t*>(selfPtr),
        0x44,
        0x0b0,
        0x160,
        reinterpret_cast<FnElemBuildGui>(GX_VMA(0xe79280ULL)));
}

// 45. 0x21f2e70 (612 B) — GXParamBankManagerE_RegisterDebugMenu
extern "C" void GXParamBankManagerE_RegisterDebugMenu(void* self, void* parentGui) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x44);

    reinterpret_cast<FnSceneGetEditSlot>(GX_VMA(0x21877f0ULL))(sceneSys, parentGui, slotId, 0);

    DLAllocator* guiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    FnGuiCreateSubMenu fnCreateSubMenu = reinterpret_cast<FnGuiCreateSubMenu>(GX_VMA(0x236d620ULL));
    FnGuiAddActionItem fnAddAction = reinterpret_cast<FnGuiAddActionItem>(GX_VMA(0x235e4c0ULL));

    // 1) "ParameterSystem Regist" -> GXParamBankManagerE_RegisterAllParams (0x21f2a90)
    uint64_t subDesc1[3] = {0, 0, 0};
    void* menu1 = fnCreateSubMenu(parentGui, subDesc1);
    subDesc1[1] = 0;

    uint64_t labelDesc1[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL))), 0};
    uint8_t* cb1 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb1 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb1 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb1 + 0x00) = GX_VMA(0x53b7160ULL);
        *reinterpret_cast<void**>(cb1 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb1 + 0x18) = 0;
        *reinterpret_cast<void**>(cb1 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerE_RegisterAllParams);
        *reinterpret_cast<uint64_t*>(cb1 + 0x28) = 0;
    }
    fnAddAction(menu1, labelDesc1, cb1);
    labelDesc1[1] = 0;

    // 2) "ParameterSystem Unregist" -> GXParamBankManagerE_UnregisterAllParams (0x21f2b20)
    uint64_t subDesc2[3] = {0, 0, 0};
    void* menu2 = fnCreateSubMenu(parentGui, subDesc2);
    subDesc2[1] = 0;

    uint64_t labelDesc2[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL))), 0};
    uint8_t* cb2 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb2 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb2 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb2 + 0x00) = GX_VMA(0x53b7160ULL);
        *reinterpret_cast<void**>(cb2 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb2 + 0x18) = 0;
        *reinterpret_cast<void**>(cb2 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerE_UnregisterAllParams);
        *reinterpret_cast<uint64_t*>(cb2 + 0x28) = 0;
    }
    fnAddAction(menu2, labelDesc2, cb2);
    labelDesc2[1] = 0;

    // 3) "Open List" -> GXParamBankManagerE_OpenParamEditListDialog (0x21f30e0)
    uint64_t subDesc3[3] = {0, 0, 0};
    void* menu3 = fnCreateSubMenu(parentGui, subDesc3);
    subDesc3[1] = 0;

    uint64_t labelDesc3[3] = {0, reinterpret_cast<uint64_t>(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL))), 0};
    uint8_t* cb3 = static_cast<uint8_t*>(guiAlloc->vptr->allocateAligned(guiAlloc, 0x30, 0x8));
    if (cb3 != nullptr) {
        *reinterpret_cast<uint32_t*>(cb3 + 0x08) = 0x40000000U;
        *reinterpret_cast<uint64_t*>(cb3 + 0x00) = GX_VMA(0x53b7160ULL);
        *reinterpret_cast<void**>(cb3 + 0x10) = self;
        *reinterpret_cast<uint32_t*>(cb3 + 0x18) = 0;
        *reinterpret_cast<void**>(cb3 + 0x20) = reinterpret_cast<void*>(&GXParamBankManagerE_OpenParamEditListDialog);
        *reinterpret_cast<uint64_t*>(cb3 + 0x28) = 0;
    }
    fnAddAction(menu3, labelDesc3, cb3);
    labelDesc3[1] = 0;
}

// 46. 0x21f2b90 (732 B) — GXParamBankManagerE_ResolveParamEntry
extern "C" void* GXParamBankManagerE_ResolveParamEntry(ParamOuterHashTable* self, const uint64_t* packedKeyPtr) {
    uint64_t packed = *packedKeyPtr;
    uint32_t bankId = static_cast<uint32_t>(packed);
    uint32_t paramId = static_cast<uint32_t>(packed >> 32);

    if (bankId == 0xffffffffU) {
        if (paramId == 0xffffffffU) {
            paramId = 0;
            bankId = 1;
        } else {
            bankId = 0xffffffffU;
        }
    }

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerCur = outerBuckets[outerIdx].beginNode;

    if (outerCur != outerSentinel) {
        ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
        while (outerCur != outerEnd) {
            if (outerCur->key >= bankId) {
                if (bankId < outerCur->key) {
                    outerCur = outerSentinel;
                }
                break;
            }
            outerCur = outerCur->next;
        }
        if (outerCur != outerSentinel && outerCur != outerEnd) {
            ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerCur->value);
            uint64_t innerIdx = ComputeLehmerHashBucket(paramId, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
            ParamHashNode* innerSentinel = inner->sentinel;
            ParamHashBucket* innerBuckets = inner->buckets;
            ParamHashNode* innerCur = innerBuckets[innerIdx].beginNode;
            ParamHashNode* foundInner = innerSentinel;

            if (innerCur != innerSentinel) {
                ParamHashNode* innerEnd = innerBuckets[innerIdx].endNode->next;
                while (innerCur != innerEnd) {
                    if (innerCur->key >= paramId) {
                        if (paramId >= innerCur->key) {
                            foundInner = innerCur;
                        }
                        break;
                    }
                    innerCur = innerCur->next;
                }
            }
            if (foundInner != inner->sentinel) {
                return foundInner->value;
            }
        }
    }

    uint64_t fbOuterIdx = ComputeLehmerHashBucket(1U, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* fbOuterCur = outerBuckets[fbOuterIdx].beginNode;
    if (fbOuterCur != outerSentinel) {
        ParamHashNode* fbOuterEnd = outerBuckets[fbOuterIdx].endNode->next;
        while (fbOuterCur != fbOuterEnd) {
            if (fbOuterCur->key != 0) {
                if (fbOuterCur->key > 1U) {
                    fbOuterCur = outerSentinel;
                }
                break;
            }
            fbOuterCur = fbOuterCur->next;
        }
        if (fbOuterCur != outerSentinel && fbOuterCur != fbOuterEnd) {
            ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(fbOuterCur->value);
            uint64_t fbInnerIdx = ComputeLehmerHashBucket(0U, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
            ParamHashNode* innerSentinel = inner->sentinel;
            ParamHashBucket* innerBuckets = inner->buckets;
            ParamHashNode* fbInnerCur = innerBuckets[fbInnerIdx].beginNode;
            ParamHashNode* foundFb = innerSentinel;

            if (fbInnerCur != innerSentinel) {
                ParamHashNode* fbInnerEnd = innerBuckets[fbInnerIdx].endNode->next;
                if (fbInnerCur != fbInnerEnd && fbInnerCur->key == 0U) {
                    foundFb = fbInnerCur;
                }
            }
            if (foundFb != inner->sentinel) {
                return foundFb->value;
            }
        }
    }

    return reinterpret_cast<uint8_t*>(self) + 0x160;
}

// 47. 0x21f2b20 (103 B) — GXParamBankManagerE_UnregisterAllParams
extern "C" void GXParamBankManagerE_UnregisterAllParams(void* self) {
    ParamOuterHashTable* outer = static_cast<ParamOuterHashTable*>(self);
    ParamHashNode* outerSentinel = outer->sentinel;
    for (ParamHashNode* outerNode = outerSentinel->next; outerNode != outerSentinel; outerNode = outerNode->next) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
        ParamHashNode* innerSentinel = inner->sentinel;
        for (ParamHashNode* innerNode = innerSentinel->next; innerNode != innerSentinel; innerNode = innerNode->next) {
            reinterpret_cast<FnElemUnregister>(GX_VMA(0xe78c10ULL))(innerNode->value);
        }
        outerSentinel = outer->sentinel;
    }
}

// 48. 0x21f2a90 (132 B) — GXParamBankManagerE_RegisterAllParams
extern "C" void GXParamBankManagerE_RegisterAllParams(void* self) {
    ParamOuterHashTable* outer = static_cast<ParamOuterHashTable*>(self);
    ParamHashNode* outerSentinel = outer->sentinel;
    for (ParamHashNode* outerNode = outerSentinel->next; outerNode != outerSentinel; outerNode = outerNode->next) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
        ParamHashNode* innerSentinel = inner->sentinel;
        for (ParamHashNode* innerNode = innerSentinel->next; innerNode != innerSentinel; innerNode = innerNode->next) {
            uint8_t* elem = static_cast<uint8_t*>(innerNode->value);
            *reinterpret_cast<uint32_t*>(elem + 0x70) = innerNode->key;
            *reinterpret_cast<uint32_t*>(elem + 0x74) = outerNode->key;
            reinterpret_cast<FnElemApply>(GX_VMA(0xe78c70ULL))(elem, 0.0f);
            reinterpret_cast<FnElemRegister>(GX_VMA(0xe78da0ULL))(elem, outer->ownerAlloc);
        }
        outerSentinel = outer->sentinel;
    }
}

// 49. 0x21f2a80 (1 B) — GXParamBankManagerE_OnEventNoOp
extern "C" void GXParamBankManagerE_OnEventNoOp(void*) {
    return;
}

// 50. 0x21f2a50 (40 B) — GXParamBankManagerE_SetActiveParamId
extern "C" void GXParamBankManagerE_SetActiveParamId(void* /*self*/, uint32_t paramId, uint32_t bankId) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x44);
    reinterpret_cast<FnSceneSetActiveId>(GX_VMA(0x2186da0ULL))(sceneSys, slotId, paramId, bankId, -1LL);
}

// 51. 0x21f2790 (698 B) — GXParamBankManagerE_UnloadBankById
extern "C" void GXParamBankManagerE_UnloadBankById(
    ParamOuterHashTable* self,
    void* /*unused*/,
    uint32_t bankId,
    uint32_t overrideSlotIdx,
    float blendTime)
{
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x44);

    reinterpret_cast<FnSceneBeginUnload>(GX_VMA(0x2185b50ULL))(
        sceneSys, slotId, bankId, overrideSlotIdx, blendTime);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = 1;

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerNode = outerBuckets[outerIdx].beginNode;
    if (outerNode == outerSentinel) {
        return;
    }

    ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
    while (outerNode != outerEnd && outerNode->key < bankId) {
        outerNode = outerNode->next;
    }
    if (outerNode == outerEnd || outerNode->key > bankId || outerNode == outerSentinel) {
        return;
    }

    ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerNode->value);
    if (inner != nullptr) {
        ParamHashNode* innerCur = inner->sentinel->next;
        while (innerCur != inner->sentinel) {
            bool keep = reinterpret_cast<FnSceneCheckEntry>(GX_VMA(0x2185e00ULL))(sceneSys, slotId, innerCur->key, bankId);
            DLVirtualObj* elem = static_cast<DLVirtualObj*>(innerCur->value);
            if (keep) {
                *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(elem) + 0x68) = 1;
                innerCur = innerCur->next;
            } else {
                if (elem != nullptr) {
                    elem->vptr->dtor(elem);
                    self->ownerAlloc->vptr->freeBlock(self->ownerAlloc, elem);
                    innerCur->value = nullptr;
                }
                ParamBucketIter nextIt = {0, 0, nullptr};
                GXParamBankManagerE_InnerHashTable_EraseNode(&nextIt, inner, innerCur);
                innerCur = nextIt.node;
            }
        }

        if (inner->count != 0) {
            return;
        }

        DLAllocator* ownerAlloc = self->ownerAlloc;
        if (inner->buckets != nullptr) {
            inner->bucketAlloc->vptr->freeBlock(inner->bucketAlloc, inner->buckets);
            inner->buckets = nullptr;
            inner->bucketCapBytes = 0;
            inner->bucketEndOff = 0;
        }

        ParamHashNode* head = inner->sentinel;
        ParamHashNode* n = head->next;
        head->next = head;
        inner->sentinel->prev = inner->sentinel;
        inner->count = 0;
        while (n != inner->sentinel) {
            ParamHashNode* nextN = n->next;
            inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, n);
            n = nextN;
        }
        inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, inner->sentinel);
        ownerAlloc->vptr->freeBlock(ownerAlloc, inner);
        outerNode->value = nullptr;
    }

    ParamBucketIter nextOuter = {0, 0, nullptr};
    GXParamBankManagerE_OuterHashTable_EraseNode(&nextOuter, reinterpret_cast<uint8_t*>(self) + 0x08, outerNode);
}

// 52. 0x21f2010 (1912 B) — GXParamBankManagerE_LoadBankFromStream
extern "C" void GXParamBankManagerE_LoadBankFromStream(
    ParamOuterHashTable* self, void* streamPtr, uint32_t bankId, uint32_t flags, float weight) {
    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    uint32_t slotId = *reinterpret_cast<uint32_t*>(sceneSys + 0x44);

    uint32_t entryCount = reinterpret_cast<FnSceneGetBankCount>(GX_VMA(0x2184cf0ULL))(sceneSys, slotId, streamPtr);
    if (entryCount == 0) {
        reinterpret_cast<FnSceneFinalizeLoad>(GX_VMA(0x2184ff0ULL))(sceneSys, slotId, bankId, flags, streamPtr, weight);
        return;
    }

    uint64_t outerIdx = ComputeLehmerHashBucket(bankId, 0xdeadbeefU, self->mask, self->maxBucketIdx);
    ParamHashNode* outerSentinel = self->sentinel;
    ParamHashBucket* outerBuckets = self->buckets;
    ParamHashNode* outerNode = outerBuckets[outerIdx].beginNode;
    if (outerNode != outerSentinel) {
        ParamHashNode* outerEnd = outerBuckets[outerIdx].endNode->next;
        while (outerNode != outerEnd && outerNode->key < bankId) {
            outerNode = outerNode->next;
        }
        if (outerNode == outerEnd || outerNode->key > bankId) {
            outerNode = outerSentinel;
        }
    }

    ParamInnerHashTable* inner = nullptr;
    if (outerNode == outerSentinel) {
        DLAllocator* ownerAlloc = self->ownerAlloc;
        inner = static_cast<ParamInnerHashTable*>(ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x60, 0x8));
        if (inner != nullptr) {
            uint8_t heapCaps[32] = {0};
            ownerAlloc->vptr->getHeapCaps(heapCaps, ownerAlloc, 0);
            if ((heapCaps[0] & 0x20) == 0) {
                reinterpret_cast<FnLogAssert>(GX_VMA(0x20b55b0ULL))(
                    0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }
            inner->count = 0;
            inner->sentinel = nullptr;
            inner->nodeAlloc = ownerAlloc;
            ParamHashNode* innerSent = static_cast<ParamHashNode*>(
                ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x20, 0x8));
            if (innerSent != nullptr) {
                innerSent->next = innerSent;
                innerSent->prev = innerSent;
            } else {
                ReportTransactionFailure();
            }
            inner->sentinel = innerSent;
            inner->buckets = nullptr;
            inner->bucketCapBytes = 0;
            inner->bucketEndOff = 0;
            inner->bucketAlloc = ownerAlloc;
            inner->maxLoadFactor = 1.0f;

            ParamBucketIter fillVal{0, 0, innerSent};
            ParamBucketIter dummyOut{};
            ParamBucketIter posIter{0, 0, nullptr};
            GXParamBankManagerE_InnerHashTable_RehashInsert(
                &dummyOut, reinterpret_cast<uint8_t*>(inner) + 0x20, 0x10, &fillVal, posIter);
            inner->mask = 7;
            inner->maxBucketIdx = 8;
        }

        ParamHashNode* newOuterNode = static_cast<ParamHashNode*>(
            self->nodeAlloc->vptr->allocateAligned(self->nodeAlloc, 0x20, 0x8));
        if (newOuterNode == nullptr) {
            ReportTransactionFailure();
            return;
        }
        ParamHashNode* headNext = self->sentinel->next;
        newOuterNode->next = headNext ? headNext : newOuterNode;
        newOuterNode->prev = headNext ? headNext->prev : newOuterNode;
        newOuterNode->key = bankId;
        newOuterNode->value = inner;
        if (self->count == 0x7fffffffffffffeULL) {
            reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        self->count += 1;
        headNext->prev = newOuterNode;
        newOuterNode->prev->next = newOuterNode;

        ParamHashNode* firstOuter = self->sentinel->next;
        const uint32_t* kvPtr = &firstOuter->key;
        ParamBucketIter nodeIter = {0, 0, firstOuter};
        ParamIterPair outPair{};
        GXParamBankManagerE_OuterHashTable_InsertNode(
            &outPair, reinterpret_cast<uint8_t*>(self) + 0x08, kvPtr, nodeIter);
    } else {
        inner = static_cast<ParamInnerHashTable*>(outerNode->value);
    }

    for (uint32_t i = 0; i < entryCount; ++i) {
        uint32_t paramId = reinterpret_cast<FnSceneGetParamIdAt>(GX_VMA(0x2184de0ULL))(sceneSys, slotId, streamPtr, i);
        uint64_t innerIdx = ComputeLehmerHashBucket(paramId, 0xdeadbeefU, inner->mask, inner->maxBucketIdx);
        ParamHashNode* innerSentinel = inner->sentinel;
        ParamHashBucket* innerBuckets = inner->buckets;
        ParamHashNode* innerNode = innerBuckets[innerIdx].beginNode;
        if (innerNode != innerSentinel) {
            ParamHashNode* innerEnd = innerBuckets[innerIdx].endNode->next;
            while (innerNode != innerEnd && innerNode->key < paramId) {
                innerNode = innerNode->next;
            }
            if (innerNode == innerEnd || innerNode->key > paramId) {
                innerNode = innerSentinel;
            }
        }

        if (innerNode == innerSentinel) {
            DLAllocator* ownerAlloc = self->ownerAlloc;
            uint8_t* elem = static_cast<uint8_t*>(ownerAlloc->vptr->allocateAligned(ownerAlloc, 0x78, 0x8));
            if (elem == nullptr) {
                ReportTransactionFailure();
                continue;
            }
            reinterpret_cast<FnElemConstruct>(GX_VMA(0xe789d0ULL))(elem);
            reinterpret_cast<FnElemInit>(GX_VMA(0xe78a80ULL))(elem);
            *reinterpret_cast<uint32_t*>(elem + 0x70) = paramId;
            *reinterpret_cast<uint32_t*>(elem + 0x74) = bankId;
            reinterpret_cast<FnElemApply>(GX_VMA(0xe78c70ULL))(elem, 0.0f);
            reinterpret_cast<FnElemRegister>(GX_VMA(0xe78da0ULL))(elem, self->ownerAlloc);

            ParamHashNode* newInnerNode = static_cast<ParamHashNode*>(
                inner->nodeAlloc->vptr->allocateAligned(inner->nodeAlloc, 0x20, 0x8));
            if (newInnerNode == nullptr) {
                ReportTransactionFailure();
                continue;
            }
            ParamHashNode* iHeadNext = inner->sentinel->next;
            newInnerNode->next = iHeadNext ? iHeadNext : newInnerNode;
            newInnerNode->prev = iHeadNext ? iHeadNext->prev : newInnerNode;
            newInnerNode->key = paramId;
            newInnerNode->value = elem;
            if (inner->count == 0x7fffffffffffffeULL) {
                reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            inner->count += 1;
            iHeadNext->prev = newInnerNode;
            newInnerNode->prev->next = newInnerNode;

            ParamHashNode* firstInner = inner->sentinel->next;
            const uint32_t* kvPtr = &firstInner->key;
            ParamBucketIter nodeIter = {0, 0, firstInner};
            ParamIterPair outPair{};
            GXParamBankManagerE_InnerHashTable_InsertNode(&outPair, inner, kvPtr, nodeIter);
        } else {
            uint8_t* elem = static_cast<uint8_t*>(innerNode->value);
            *reinterpret_cast<uint8_t*>(elem + 0x68) = 1;
        }
    }

    reinterpret_cast<FnSceneFinalizeLoad>(GX_VMA(0x2184ff0ULL))(sceneSys, slotId, bankId, flags, streamPtr, weight);
}

// 53. 0x21f1b50 (1207 B) — GXParamBankManagerE_RegisterProperties
extern "C" bool GXParamBankManagerE_RegisterProperties(uint8_t* self) {
    GXParamBankManagerE_ClearAllBanks(self);

    uint8_t* globalRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* sceneSys = *reinterpret_cast<uint8_t**>(globalRoot + 0x5ad0);
    void* propSys = *reinterpret_cast<void**>(sceneSys + 0x28);

    uint32_t slotId = 0;
    reinterpret_cast<FnPropRegisterGroup>(GX_VMA(0xe32510ULL))(
        propSys, &slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c676aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6734ULL)));
    *reinterpret_cast<uint32_t*>(sceneSys + 0x44) = slotId;

    FnPropBeginSection fnBeginSec = reinterpret_cast<FnPropBeginSection>(GX_VMA(0xe333e0ULL));
    FnPropAddField fnAddField = reinterpret_cast<FnPropAddField>(GX_VMA(0xe327d0ULL));
    FnPropEndSection fnEndSec = reinterpret_cast<FnPropEndSection>(GX_VMA(0xe33600ULL));

    fnBeginSec(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c676aULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6784ULL)), 1, 1);

    void* vtIntPrimary = reinterpret_cast<void*>(GX_VMA(0x52e8760ULL) + 0x10);
    void* vtIntSecondary = reinterpret_cast<void*>(GX_VMA(0x52e4a70ULL) + 0x10);
    void* vtFloatPrimary = reinterpret_cast<void*>(GX_VMA(0x52e83f0ULL) + 0x10);
    void* vtFloatSecondary = reinterpret_cast<void*>(GX_VMA(0x52e4890ULL) + 0x10);

    // 1. Enable (+0x68)
    ParamGuiPropDesc descEnable = {
        vtIntPrimary, self + 0x68, 0x03, {0, 0, 0}, 1, 1, {0}, vtIntSecondary, 0U, 1U, 0U, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49cf3a4ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c67acULL)), &descEnable, 1, 0);

    // 2. FeedbackWeight (+0x6c)
    ParamGuiPropDesc descWeight = {
        vtFloatPrimary, self + 0x6c, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f7fbe77U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c67e0ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c67feULL)), &descWeight, 1, 1);

    // 3. FeedbackUnitTimeInSeconds (+0x74)
    ParamGuiPropDesc descUnitTime = {
        vtFloatPrimary, self + 0x74, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6828ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c685cULL)), &descUnitTime, 1, 1);

    // 4. FeedbackAspectRatio (+0x78)
    ParamGuiPropDesc descAspect = {
        vtFloatPrimary, self + 0x78, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3feccccdU, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c689eULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c68c6ULL)), &descAspect, 1, 1);

    fnEndSec(propSys, slotId, 1);

    // Section: FeedBack ObjectBlur
    fnBeginSec(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c68fcULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c68fcULL)), 1, 1);

    // 5. FeedbackRate (+0x7c)
    ParamGuiPropDesc descRate = {
        vtFloatPrimary, self + 0x7c, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f7fbe77U, 0x3a83126fU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c6924ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c693eULL)), &descRate, 1, 1);

    // 6. FeedbackBlurFallOff (+0x80)
    ParamGuiPropDesc descFallOff = {
        vtFloatPrimary, self + 0x80, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c696eULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c6996ULL)), &descFallOff, 1, 1);

    // 7. FeedbackBlurPower (+0x84)
    ParamGuiPropDesc descPower = {
        vtFloatPrimary, self + 0x84, 0x09, {0, 0, 0}, 0, 1, {0}, vtFloatSecondary,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U
    };
    fnAddField(propSys, slotId, reinterpret_cast<const char16_t*>(GX_VMA(0x49c69ccULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c69f0ULL)), &descPower, 1, 1);

    fnEndSec(propSys, slotId, 1);

    reinterpret_cast<FnSceneInitSubCompA>(GX_VMA(0x218e2b0ULL))(sceneSys + 0x130, propSys, 0);
    reinterpret_cast<FnSceneInitSubCompB>(GX_VMA(0xe41f00ULL))(sceneSys + 0x1b8, propSys, 0, slotId, 0, 0.0f);
    reinterpret_cast<FnSceneSyncBank>(GX_VMA(0x21874c0ULL))(sceneSys, slotId);
    return true;
}

// 54. 0x21f19c0 (395 B) — GXParamBankManagerE_ClearAllBanks
extern "C" void GXParamBankManagerE_ClearAllBanks(void* selfPtr) {
    ParamOuterHashTable* self = static_cast<ParamOuterHashTable*>(selfPtr);
    ParamHashNode* outerCur = self->sentinel->next;
    while (outerCur != self->sentinel) {
        ParamInnerHashTable* inner = static_cast<ParamInnerHashTable*>(outerCur->value);
        ParamHashNode* innerCur = inner->sentinel->next;
        while (innerCur != inner->sentinel) {
            DLVirtualObj* elem = static_cast<DLVirtualObj*>(innerCur->value);
            if (elem != nullptr) {
                elem->vptr->dtor(elem);
                self->ownerAlloc->vptr->freeBlock(self->ownerAlloc, elem);
                innerCur->value = nullptr;
            }
            ParamBucketIter nextIt = {0, 0, nullptr};
            GXParamBankManagerE_InnerHashTable_EraseNode(&nextIt, inner, innerCur);
            innerCur = nextIt.node;
            inner = static_cast<ParamInnerHashTable*>(outerCur->value);
        }

        if (inner != nullptr) {
            DLAllocator* ownerAlloc = self->ownerAlloc;
            if (inner->buckets != nullptr) {
                inner->bucketAlloc->vptr->freeBlock(inner->bucketAlloc, inner->buckets);
                inner->buckets = nullptr;
                inner->bucketCapBytes = 0;
                inner->bucketEndOff = 0;
            }
            ParamHashNode* head = inner->sentinel;
            ParamHashNode* n = head->next;
            head->next = head;
            inner->sentinel->prev = inner->sentinel;
            inner->count = 0;
            while (n != inner->sentinel) {
                ParamHashNode* nextN = n->next;
                inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, n);
                n = nextN;
            }
            inner->nodeAlloc->vptr->freeBlock(inner->nodeAlloc, inner->sentinel);
            ownerAlloc->vptr->freeBlock(ownerAlloc, inner);
            outerCur->value = nullptr;
        }

        ParamBucketIter nextOuter = {0, 0, nullptr};
        GXParamBankManagerE_OuterHashTable_EraseNode(&nextOuter, reinterpret_cast<uint8_t*>(self) + 0x08, outerCur);
        outerCur = nextOuter.node;
    }
}

// 55. 0x21f18c0 (255 B) — GXParamBankManagerE_Destruct
extern "C" void GXParamBankManagerE_Destruct(uint8_t* self) {
    GXParamBankManagerE_ClearAllBanks(self);
    reinterpret_cast<FnElemDestruct>(GX_VMA(0xe78ae0ULL))(self + 0x160);

    uint64_t strCap = *reinterpret_cast<uint64_t*>(self + 0xd0);
    if (strCap >= 8) {
        void* heapBuf = *reinterpret_cast<void**>(self + 0xb8);
        if (strCap + 1 >= 0x41) {
            DLAllocator* sAlloc = *reinterpret_cast<DLAllocator**>(self + 0xd8);
            if (sAlloc && heapBuf) {
                sAlloc->vptr->freeBlock(sAlloc, heapBuf);
            }
        } else {
            self[0x159] &= 0xfdU;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0xd0) = 7;
    *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
    *reinterpret_cast<uint16_t*>(self + 0xb8) = 0;

    ParamOuterHashTable* outer = reinterpret_cast<ParamOuterHashTable*>(self);
    if (outer->buckets != nullptr) {
        outer->bucketAlloc->vptr->freeBlock(outer->bucketAlloc, outer->buckets);
        outer->buckets = nullptr;
        outer->bucketCapBytes = 0;
        outer->bucketEndOff = 0;
    }

    ParamHashNode* head = outer->sentinel;
    ParamHashNode* cur = head->next;
    head->next = head;
    outer->sentinel->prev = outer->sentinel;
    outer->count = 0;
    while (cur != outer->sentinel) {
        ParamHashNode* nextNode = cur->next;
        outer->nodeAlloc->vptr->freeBlock(outer->nodeAlloc, cur);
        cur = nextNode;
    }
    outer->nodeAlloc->vptr->freeBlock(outer->nodeAlloc, outer->sentinel);
}
