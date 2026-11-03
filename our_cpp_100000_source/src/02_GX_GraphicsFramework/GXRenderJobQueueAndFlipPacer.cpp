// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Module 02: GX Graphics Framework
// File: GXRenderJobQueueAndFlipPacer.cpp
// VMA Range: 0x21b2f10 .. 0x21b5990 (49 newly carved functions + 2 previously carved)
// ============================================================================
// Architectural Overview:
//   1. GX Render Job Queue & Keyed Stable Merge Sort (0x21b43c0 .. 0x21b5990)
//      - Manages a multi-threaded render job packet stream (`GXRenderJobQueue`)
//        with two linear ring-buffer allocators (+0x48 for job packets, +0x78
//        for auxiliary payloads), two recursive mutexes (+0xB0, +0xC8), and a
//        16-byte entry array (`{ void* jobPacket, uint32_t sortKey, uint32_t pad }`)
//        sorted via a custom `std::stable_sort` (`0x21b4f70`, `0x21b5170`,
//        `0x21b5990`) before dispatching via `ReplayRenderJobStream` (`0x21b4d00`).
//   2. GX Context State Shadow Pool & Deferred Stream Helpers (0x21b3910 .. 0x21b42a0)
//      - Manages a mutex-protected pool of `0xF0`-byte `GXNamedContextStateShadow`
//        objects (each embedding a `0xB0`-byte `CGContextStateShadowGnm` at `+0x00`
//        and a `DLString` name at `+0xB0`), plus command-list begin/submit helpers
//        (`0x21b4190`, `0x21b42a0`).
//   3. GX Dual-Allocator Fallback Bridge (0x21b3390 .. 0x21b3900)
//      - 28 virtual thunks and allocation methods that attempt allocation on a
//        primary heap (`[this+0x08]`) and transparently fall back to a secondary
//        heap (`[this+0x10]`) or thread-local heap (`TLS+0x53e44a8`).
//   4. GX Flip Controller & Frame Pacer (0x21b2f10 .. 0x21b2fb0)
//      - `GXFlipController_PresentAndWaitFramePacing` (`0x21b2fb0`): Flushes the
//        immediate GNM graphics context (`0x26afa00`), triggers swap-chain flip
//        (`0x26d5dd0`), and runs Bloodborne's infamous 30 FPS / 60 Hz frame-pacing
//        busy-wait + `sceKernelUsleep` loop (`16666 us * flipInterval - elapsed`).
// ============================================================================

#include <cstdint>
#include <cstring>
#include <algorithm>
#include "gx_runtime_vma_and_tls.h"

namespace FromSoftware {
namespace GX {

// Forward declarations of external engine / Orbis OS services
struct DLAllocator {
    struct VTable {
        void* (*dtor)(DLAllocator*, int);                // +0x00
        void* (*fn08)(DLAllocator*);                     // +0x08
        void* (*fn10)(DLAllocator*);                     // +0x10
        uint32_t (*GetAllocatorId)(DLAllocator*);        // +0x18
        void* (*GetCapabilities)( void* out, DLAllocator*, uint32_t); // +0x20
        uint64_t (*GetTotalSize)(DLAllocator*);          // +0x28
        uint64_t (*GetFreeSize)(DLAllocator*);           // +0x30
        uint64_t (*GetMaxAllocatable)(DLAllocator*);     // +0x38
        uint64_t (*GetAllocationCount)(DLAllocator*);    // +0x40
        uint64_t (*GetBlockSize)(DLAllocator*, void*);   // +0x48
        void* (*Allocate)(DLAllocator*, size_t size);    // +0x50
        void* (*AllocateAligned)(DLAllocator*, size_t size, size_t align); // +0x58
        void* (*Reallocate)(DLAllocator*, void* ptr, size_t size);         // +0x60
        void* (*ReallocateAligned)(DLAllocator*, void* ptr, size_t size, size_t align); // +0x68
        void  (*Free)(DLAllocator*, void* ptr);          // +0x70
        void  (*FreeAll)(DLAllocator*);                  // +0x78
        void* (*BackAllocate)(DLAllocator*, size_t size);                                   // +0x80
        void* (*BackAllocateAligned)(DLAllocator*, size_t size, size_t align);              // +0x88
        void* (*BackReallocate)(DLAllocator*, void* ptr, size_t size);                      // +0x90
        void* (*BackReallocateAligned)(DLAllocator*, void* ptr, size_t size, size_t align); // +0x98
        void  (*BackFree)(DLAllocator*, void* ptr);      // +0xA0
        bool  (*ValidateHeap)(DLAllocator*);             // +0xA8 (CheckIntegrity)
        bool  (*OwnsAllocation)(DLAllocator*, void* ptr);// +0xB0 (ContainsAddress)
        void  (*DumpStats)(DLAllocator*);                // +0xB8
        void  (*Lock)(DLAllocator*);                     // +0xC0
        void  (*Unlock)(DLAllocator*);                   // +0xC8
        void* (*GetBlockBase)(DLAllocator*, void* ptr);  // +0xD0
        uint64_t (*GetHighWaterMark)(DLAllocator*);      // +0xD8
    };
    VTable* vptr;
};

struct DLMutex {
    struct VTable {
        void (*dtor)(DLMutex*);
        void (*fn08)(DLMutex*);
        void (*fn10)(DLMutex*);
        void (*Lock)(DLMutex*, uint32_t timeout);        // +0x18
        bool (*TryLock)(DLMutex*);                       // +0x20
        void (*Unlock)(DLMutex*);                        // +0x28
    };
    VTable*  vptr;         // +0x00
    uint64_t pthreadMutex; // +0x08
    uint8_t  initialized;  // +0x10
    uint8_t  pad[7];
};

struct GXLinearRingBuffer {
    uint8_t data[0x30];
};

extern "C" {
    void  GXLinearRingBuffer_Construct(GXLinearRingBuffer* self, uint32_t flag, uint32_t chunkSize); // 0x0e454e0
    void  GXLinearRingBuffer_Destroy(GXLinearRingBuffer* self);                                      // 0x0e45520
    void  GXLinearRingBuffer_Reset(GXLinearRingBuffer* self, void* backingStore, uint32_t chunkSize);// 0x0e45530
    void* GXLinearRingBuffer_Allocate(GXLinearRingBuffer* self, size_t size, size_t align);          // 0x0e45560

    void  CGContextStateShadowGnm_Construct(void* self); // 0x26d5680
    void  CGContextStateShadowGnm_Destroy(void* self);   // 0x26d56c0
    void  CGContextStateShadowGnm_CopyFromDevice(void* dstShadow, void* srcDevice); // 0x0e47930

    void  CGGfxContextGnm_FlushAndSubmitImmediate(uint32_t flags); // 0x26afa00
    bool  CGSwapChainGnm_PresentFlip(int32_t flipMode, int64_t flipArg, int32_t flipRate); // 0x26d5dd0

    int   sceKernelGettimeofday(struct timeval* tp); // 0x2bbe728
    int   sceKernelUsleep(uint32_t microseconds);    // 0x2bbfe68
}

// ============================================================================
// 1. GX Render Job Queue & Keyed Stable Merge Sort (0x21b43c0 .. 0x21b5990)
// ============================================================================

struct GXRenderJobEntry {
    uint8_t* jobPacket; // +0x00: Pointer to packet header (byte 0 = type, byte 1 = pad, word 2 = size)
    uint32_t sortKey;   // +0x08: 32-bit sort key for draw/job ordering
};

struct GXRenderJobTempBuffer {
    GXRenderJobEntry* begin;    // +0x00
    GXRenderJobEntry* current;  // +0x08
    GXRenderJobEntry* end;      // +0x10
    int64_t           capacity; // +0x18
};

struct GXRenderJobSortContext {
    GXRenderJobTempBuffer  inlineBuf; // +0x00 .. +0x1F
    GXRenderJobTempBuffer* tempBuf;   // +0x20
};

static inline void GXRenderJobTempBuffer_LazyInit(GXRenderJobTempBuffer* tmp) {
    if (tmp->begin != nullptr || tmp->capacity <= 0) {
        return;
    }
    int64_t reqCap = tmp->capacity;
    if ((static_cast<uint64_t>(reqCap) >> 60) != 0ULL) {
        reinterpret_cast<void (*)()>(GX_VMA(0x2bbf188ULL))();
    }
    const void* nothrowTag = *reinterpret_cast<const void* const*>(GX_VMA(0x53e4558ULL));
    auto fnOpNewNothrow = reinterpret_cast<GXRenderJobEntry* (*)(size_t, const void*)>(GX_VMA(0x2bc08b8ULL));

    GXRenderJobEntry* mem = nullptr;
    while (true) {
        mem = fnOpNewNothrow(static_cast<size_t>(reqCap) * sizeof(GXRenderJobEntry), nothrowTag);
        if (mem != nullptr) {
            break;
        }
        int64_t nextCap = reqCap / 2;
        if (reqCap <= 1) {
            reqCap = nextCap;
            break;
        }
        reqCap = nextCap;
    }
    tmp->begin    = mem;
    tmp->current  = mem;
    tmp->end      = mem;
    tmp->capacity = reqCap;
}

static inline void GXRenderJobTempBuffer_WriteRange(
    GXRenderJobTempBuffer* tmp,
    GXRenderJobEntry* srcFirst,
    GXRenderJobEntry* srcLast)
{
    GXRenderJobEntry* dst = tmp->current;
    GXRenderJobEntry* hi  = tmp->end;
    for (GXRenderJobEntry* p = srcFirst; p != srcLast; ++p) {
        if (dst < hi) {
            *dst = *p;
            ++dst;
            tmp->current = dst;
        } else {
            if (dst != nullptr) {
                *dst = *p;
                dst = tmp->current + 1;
            } else {
                dst = reinterpret_cast<GXRenderJobEntry*>(sizeof(GXRenderJobEntry));
            }
            tmp->current = dst;
            tmp->end     = dst;
            hi           = dst;
        }
    }
}

static inline GXRenderJobEntry* GXRenderJobSort_BufferedRotate(
    GXRenderJobEntry* first,
    GXRenderJobEntry* middle,
    GXRenderJobEntry* last,
    int64_t len1,
    int64_t len2,
    GXRenderJobTempBuffer* tmp)
{
    GXRenderJobTempBuffer_LazyInit(tmp);
    if (len1 <= len2 && len1 <= tmp->capacity) {
        tmp->current = tmp->begin;
        if (first != middle) {
            GXRenderJobTempBuffer_WriteRange(tmp, first, middle);
        }
        GXRenderJobEntry* dst = first;
        for (GXRenderJobEntry* p = middle; p != last; ++p, ++dst) {
            *dst = *p;
        }
        GXRenderJobEntry* tBegin = tmp->begin;
        GXRenderJobEntry* tEnd   = tmp->current;
        for (GXRenderJobEntry* p = tBegin; p != tEnd; ++p, ++dst) {
            *dst = *p;
        }
        return first + len2;
    }

    GXRenderJobTempBuffer_LazyInit(tmp);
    if (len2 <= tmp->capacity) {
        tmp->current = tmp->begin;
        if (middle != last) {
            GXRenderJobTempBuffer_WriteRange(tmp, middle, last);
        }
        GXRenderJobEntry* dst = last;
        for (GXRenderJobEntry* p = middle; p != first; ) {
            --p;
            --dst;
            *dst = *p;
        }
        GXRenderJobEntry* out    = first;
        GXRenderJobEntry* tBegin = tmp->begin;
        GXRenderJobEntry* tEnd   = tmp->current;
        for (GXRenderJobEntry* p = tBegin; p != tEnd; ++p, ++out) {
            *out = *p;
        }
        return first + len2;
    }

    std::rotate(first, middle, last);
    return first + len2;
}

struct GXRenderJobQueue {
    void*              vptr;            // +0x00
    GXRenderJobEntry*  vecBegin;        // +0x08
    GXRenderJobEntry*  vecEnd;          // +0x10
    GXRenderJobEntry*  vecCap;          // +0x18
    DLAllocator*       allocator;       // +0x20
    uint32_t           stateIndex;      // +0x28
    uint32_t           pad2C;           // +0x2C
    GXRenderJobEntry*  jobEntries;      // +0x30
    int32_t            jobCount;        // +0x38
    int32_t            jobCapacity;     // +0x3C
    uint32_t           highestSortKey;  // +0x40
    uint32_t           pad44;           // +0x44
    GXLinearRingBuffer jobPacketArena;  // +0x48 (0x30 bytes)
    GXLinearRingBuffer payloadArena;    // +0x78 (0x30 bytes)
    void*              backingBuffer;   // +0xA8
    DLMutex            jobQueueMutex;   // +0xB0 (0x18 bytes)
    DLMutex            payloadMutex;    // +0xC8 (0x18 bytes)
};

// Forward declaration of recursive sort helper
void GXRenderJobSort_StableSortRecursive(
    GXRenderJobEntry* first,
    GXRenderJobEntry* last,
    int64_t count,
    GXRenderJobSortContext* ctx);

// ----------------------------------------------------------------------------
// VMA: 0x21b5990 (2,693 Bytes)
// Merges two adjacent sorted sub-arrays [first, middle) and [middle, last)
// of 16-byte GXRenderJobEntry elements by ascending `sortKey` (+0x08), using
// `ctx->tempBuf` when large enough or falling back to recursive in-place merge.
// ----------------------------------------------------------------------------
void GXRenderJobSort_MergeWithTempBuffer(
    GXRenderJobEntry* first,
    GXRenderJobEntry* middle,
    GXRenderJobEntry* last,
    int64_t len1,
    int64_t len2,
    GXRenderJobSortContext* ctx)
{
    while (len1 != 0 && len2 != 0) {
        if (len1 + len2 == 2) {
            if (middle->sortKey < first->sortKey) {
                GXRenderJobEntry tmp = *first;
                *first = *middle;
                *middle = tmp;
            }
            return;
        }

        GXRenderJobTempBuffer* tmp = ctx->tempBuf;
        if (len1 <= len2) {
            GXRenderJobTempBuffer_LazyInit(tmp);
            if (len1 <= tmp->capacity) {
                tmp->current = tmp->begin;
                if (first != middle) {
                    GXRenderJobTempBuffer_WriteRange(tmp, first, middle);
                }
                GXRenderJobEntry* leftCur  = tmp->begin;
                GXRenderJobEntry* leftEnd  = tmp->current;
                GXRenderJobEntry* rightCur = middle;
                GXRenderJobEntry* out      = first;

                while (leftCur != leftEnd && rightCur != last) {
                    if (rightCur->sortKey < leftCur->sortKey) {
                        *out++ = *rightCur++;
                    } else {
                        *out++ = *leftCur++;
                    }
                }
                while (leftCur != leftEnd) {
                    *out++ = *leftCur++;
                }
                return;
            }
        }

        GXRenderJobTempBuffer_LazyInit(tmp);
        if (len2 <= tmp->capacity) {
            tmp->current = tmp->begin;
            if (middle != last) {
                GXRenderJobTempBuffer_WriteRange(tmp, middle, last);
            }
            GXRenderJobEntry* rightBegin = tmp->begin;
            GXRenderJobEntry* rightCur   = tmp->current;
            GXRenderJobEntry* leftCur    = middle;
            GXRenderJobEntry* out        = last;

            if (first != middle && rightBegin != rightCur) {
                while (true) {
                    if ((rightCur - 1)->sortKey < (leftCur - 1)->sortKey) {
                        --leftCur;
                        --out;
                        *out = *leftCur;
                        if (leftCur == first) {
                            break;
                        }
                    } else {
                        --rightCur;
                        --out;
                        *out = *rightCur;
                        if (rightCur == rightBegin) {
                            break;
                        }
                    }
                }
            }
            while (rightCur != rightBegin) {
                --rightCur;
                --out;
                *out = *rightCur;
            }
            return;
        }

        // In-place binary split + buffered rotate when temporary buffer is smaller than both halves
        int64_t half1 = 0;
        int64_t half2 = 0;
        GXRenderJobEntry* firstCut = first;
        GXRenderJobEntry* secondCut = middle;

        if (len1 > len2) {
            half1 = len1 / 2;
            firstCut = first + half1;
            int64_t count = last - middle;
            GXRenderJobEntry* base = middle;
            while (count > 0) {
                int64_t step = count / 2;
                if (base[step].sortKey < firstCut->sortKey) {
                    base += step + 1;
                    count -= step + 1;
                } else {
                    count = step;
                }
            }
            secondCut = base;
            half2 = secondCut - middle;
        } else {
            half2 = len2 / 2;
            secondCut = middle + half2;
            int64_t count = middle - first;
            GXRenderJobEntry* base = first;
            while (count > 0) {
                int64_t step = count / 2;
                if (secondCut->sortKey < base[step].sortKey) {
                    count = step;
                } else {
                    base += step + 1;
                    count -= step + 1;
                }
            }
            firstCut = base;
            half1 = firstCut - first;
        }

        int64_t rotLen1 = len1 - half1;
        GXRenderJobEntry* newMiddle =
            GXRenderJobSort_BufferedRotate(firstCut, middle, secondCut, rotLen1, half2, tmp);

        GXRenderJobSort_MergeWithTempBuffer(first, firstCut, newMiddle, half1, half2, ctx);

        first  = newMiddle;
        middle = secondCut;
        len1   = rotLen1;
        len2  -= half2;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b5170 (2,069 Bytes)
// Chunked insertion sort (32-entry chunks) followed by bottom-up ping-pong
// merge passes (`first -> tmp -> first`, quadrupling step per outer loop)
// using `ctx->tempBuf`.
// ----------------------------------------------------------------------------
void GXRenderJobSort_ChunkInsertionAndMerge(
    GXRenderJobEntry* first,
    GXRenderJobEntry* last,
    int64_t count,
    GXRenderJobSortContext* ctx)
{
    int64_t remaining = count;
    GXRenderJobEntry* chunkBase = first;

    // 1. Insertion-sort every 32-element chunk
    while (remaining >= 32) {
        GXRenderJobEntry* chunkEnd = chunkBase + 32;
        for (GXRenderJobEntry* cur = chunkBase + 1; cur != chunkEnd; ++cur) {
            GXRenderJobEntry val = *cur;
            if (val.sortKey < chunkBase->sortKey) {
                for (GXRenderJobEntry* p = cur; p != chunkBase; --p) {
                    *p = *(p - 1);
                }
                *chunkBase = val;
            } else {
                GXRenderJobEntry* p = cur;
                while (val.sortKey < (p - 1)->sortKey) {
                    *p = *(p - 1);
                    --p;
                }
                *p = val;
            }
        }
        chunkBase = chunkEnd;
        remaining -= 32;
    }

    // 2. Insertion-sort the tail (< 32 elements)
    if (chunkBase != last && (chunkBase + 1) != last) {
        for (GXRenderJobEntry* cur = chunkBase + 1; cur != last; ++cur) {
            GXRenderJobEntry val = *cur;
            if (val.sortKey < chunkBase->sortKey) {
                for (GXRenderJobEntry* p = cur; p != chunkBase; --p) {
                    *p = *(p - 1);
                }
                *chunkBase = val;
            } else {
                GXRenderJobEntry* p = cur;
                while (val.sortKey < (p - 1)->sortKey) {
                    *p = *(p - 1);
                    --p;
                }
                *p = val;
            }
        }
    }

    if (count < 33) {
        return;
    }

    // 3. Bottom-up ping-pong merge passes: [first, last) -> tmp -> [first, last)
    GXRenderJobTempBuffer* tmp = ctx->tempBuf;
    int64_t step = 32;
    while (step < count) {
        tmp->current = tmp->begin;
        int64_t twoStep = step * 2;
        int64_t rem1 = count;
        GXRenderJobEntry* src = first;

        // Pass A: merge pairs of `step` from `first` into `tmp`
        while (rem1 >= twoStep) {
            GXRenderJobEntry* a = src;
            GXRenderJobEntry* b = src + step;
            GXRenderJobEntry* c = src + twoStep;
            GXRenderJobEntry* l = a;
            GXRenderJobEntry* r = b;
            while (l != b && r != c) {
                if (r->sortKey < l->sortKey) {
                    GXRenderJobTempBuffer_WriteRange(tmp, r, r + 1);
                    ++r;
                } else {
                    GXRenderJobTempBuffer_WriteRange(tmp, l, l + 1);
                    ++l;
                }
            }
            if (l != b) {
                GXRenderJobTempBuffer_WriteRange(tmp, l, b);
            }
            if (r != c) {
                GXRenderJobTempBuffer_WriteRange(tmp, r, c);
            }
            src  = c;
            rem1 -= twoStep;
        }
        if (rem1 > step) {
            GXRenderJobEntry* a = src;
            GXRenderJobEntry* b = src + step;
            GXRenderJobEntry* c = last;
            GXRenderJobEntry* l = a;
            GXRenderJobEntry* r = b;
            while (l != b && r != c) {
                if (r->sortKey < l->sortKey) {
                    GXRenderJobTempBuffer_WriteRange(tmp, r, r + 1);
                    ++r;
                } else {
                    GXRenderJobTempBuffer_WriteRange(tmp, l, l + 1);
                    ++l;
                }
            }
            if (l != b) {
                GXRenderJobTempBuffer_WriteRange(tmp, l, b);
            }
            if (r != c) {
                GXRenderJobTempBuffer_WriteRange(tmp, r, c);
            }
        } else if (src != last) {
            GXRenderJobTempBuffer_WriteRange(tmp, src, last);
        }

        // Pass B: merge pairs of `twoStep` from `tmp` back into `first`
        int64_t fourStep = step * 4;
        int64_t rem2 = count;
        GXRenderJobEntry* tSrc = tmp->begin;
        GXRenderJobEntry* tEnd = tmp->current;
        GXRenderJobEntry* dst  = first;

        while (rem2 >= fourStep) {
            GXRenderJobEntry* a = tSrc;
            GXRenderJobEntry* b = tSrc + twoStep;
            GXRenderJobEntry* c = tSrc + fourStep;
            GXRenderJobEntry* l = a;
            GXRenderJobEntry* r = b;
            while (l != b && r != c) {
                if (r->sortKey < l->sortKey) {
                    *dst++ = *r++;
                } else {
                    *dst++ = *l++;
                }
            }
            while (l != b) {
                *dst++ = *l++;
            }
            while (r != c) {
                *dst++ = *r++;
            }
            tSrc = c;
            rem2 -= fourStep;
        }
        if (rem2 > twoStep) {
            GXRenderJobEntry* a = tSrc;
            GXRenderJobEntry* b = tSrc + twoStep;
            GXRenderJobEntry* c = tEnd;
            GXRenderJobEntry* l = a;
            GXRenderJobEntry* r = b;
            while (l != b && r != c) {
                if (r->sortKey < l->sortKey) {
                    *dst++ = *r++;
                } else {
                    *dst++ = *l++;
                }
            }
            while (l != b) {
                *dst++ = *l++;
            }
            while (r != c) {
                *dst++ = *r++;
            }
        } else {
            while (tSrc != tEnd) {
                *dst++ = *tSrc++;
            }
        }

        step = fourStep;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4f70 (499 Bytes)
// Recursive entry point for stable sort of `GXRenderJobEntry` array.
// For <= 32 entries, runs insertion sort directly; otherwise lazily allocates
// the temporary merge buffer via guest `operator new(size, std::nothrow)`
// (`0x2bc08b8`) and splits the range in half.
// ----------------------------------------------------------------------------
void GXRenderJobSort_StableSortRecursive(
    GXRenderJobEntry* first,
    GXRenderJobEntry* last,
    int64_t count,
    GXRenderJobSortContext* ctx)
{
    if (count <= 32) {
        if (first == last || (first + 1) == last) {
            return;
        }
        for (GXRenderJobEntry* cur = first + 1; cur != last; ++cur) {
            GXRenderJobEntry val = *cur;
            if (val.sortKey < first->sortKey) {
                for (GXRenderJobEntry* p = cur; p != first; --p) {
                    *p = *(p - 1);
                }
                *first = val;
            } else {
                GXRenderJobEntry* p = cur;
                while (val.sortKey < (p - 1)->sortKey) {
                    *p = *(p - 1);
                    --p;
                }
                *p = val;
            }
        }
        return;
    }

    GXRenderJobTempBuffer* tmp = ctx->tempBuf;
    GXRenderJobTempBuffer_LazyInit(tmp);

    int64_t half1 = (count + 1) / 2;
    int64_t half2 = count - half1;
    GXRenderJobEntry* middle = first + half1;

    if (half1 <= tmp->capacity) {
        GXRenderJobSort_ChunkInsertionAndMerge(first, middle, half1, ctx);
        GXRenderJobSort_ChunkInsertionAndMerge(middle, last, half2, ctx);
    } else {
        GXRenderJobSort_StableSortRecursive(first, middle, half1, ctx);
        GXRenderJobSort_StableSortRecursive(middle, last, half2, ctx);
    }

    GXRenderJobSort_MergeWithTempBuffer(first, middle, last, half1, half2, ctx);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4d00 (479 Bytes) — `ReplayRenderJobStream` (`sub_21b4d00`)
// Replays `jobCount` sorted render job packets onto `devCtx`, batching up to
// 72 (`0x48`) Type-0 command streams into `GXGnmCommandContext_ExecuteCommandLists`
// (`0x216a980`) when `0x513ac85` batching is active, or invoking Type-1
// callbacks (`fn(devCtx, arg0, arg1)`) after flushing pending batches.
// ----------------------------------------------------------------------------
static inline void GXRenderJobStream_FlushBatch(
    uint8_t* devCtx,
    void** batchCmdStreams,
    uint32_t* pBatchCount)
{
    uint32_t count = *pBatchCount;
    if (count != 0u && devCtx[0x2093] != 0u) {
        uint8_t* gnmCtx = *reinterpret_cast<uint8_t**>(devCtx + 0x08);
        reinterpret_cast<void (*)(void*, void*, void* const*, uint32_t, uint32_t)>(GX_VMA(0x216a980ULL))(
            gnmCtx, nullptr, batchCmdStreams, count, 0u);
        reinterpret_cast<void* (*)(void*, int, size_t)>(GX_VMA(0x2bbe688ULL))(
            gnmCtx + 0xB730, 0, 0x23A8u);
        *pBatchCount = 0u;
    }
}

extern "C" void sub_21b4d00(
    void* /*self*/,
    uint8_t* devCtx,
    const GXRenderJobEntry* entries,
    uint32_t jobCount)
{
    void* batchCmdStreams[0x48];
    uint32_t batchCount = 0u;

    if (jobCount == 0u) {
        return;
    }

    for (uint32_t i = 0; i < jobCount; ++i) {
        const uint8_t* pkt = entries[i].jobPacket;
        uint8_t pktType = *pkt;
        void* const* payload = nullptr;
        if (pkt != nullptr) {
            uint8_t padBytes = pkt[1];
            payload = reinterpret_cast<void* const*>(pkt + 4 + padBytes);
        }

        if (pktType == 1u) {
            GXRenderJobStream_FlushBatch(devCtx, batchCmdStreams, &batchCount);
            auto fnCallback = reinterpret_cast<void (*)(void*, void*, void*)>(payload[0]);
            fnCallback(devCtx, payload[1], payload[2]);
        } else if (pktType == 0u) {
            void* cmdStream = payload[0];
            if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac85ULL)) == 0u) {
                reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe479a0ULL))(cmdStream, devCtx);
            } else {
                if (batchCount == 0x48u) {
                    GXRenderJobStream_FlushBatch(devCtx, batchCmdStreams, &batchCount);
                }
                uint32_t slot = (batchCount < 0x48u) ? batchCount : 0x47u;
                batchCmdStreams[slot] = cmdStream;
                ++batchCount;
            }
        }
    }

    GXRenderJobStream_FlushBatch(devCtx, batchCmdStreams, &batchCount);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4ca0 (94 Bytes)
// Thread-safe allocation from the render job queue's auxiliary payload arena
// (`+0x78`), guarded by `payloadMutex` (`+0xC8`).
// ----------------------------------------------------------------------------
void* GXRenderJobQueue_AllocatePayload(GXRenderJobQueue* self, uint32_t size, uint32_t align) {
    self->payloadMutex.vptr->Lock(&self->payloadMutex, 0xFFFFFFFFu);
    uint32_t alignedSize = (size + 3u) & ~3u;
    void* ptr = GXLinearRingBuffer_Allocate(&self->payloadArena, alignedSize, align);
    self->payloadMutex.vptr->Unlock(&self->payloadMutex);
    return ptr;
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4c10 (132 Bytes)
// Sorts all queued render job entries (`[self+0x30]`, count `[self+0x38]`)
// stably by their 32-bit sort key (`+0x08`).
// ----------------------------------------------------------------------------
void GXRenderJobQueue_SortJobsByKey(GXRenderJobQueue* self) {
    int64_t count = self->jobCount;
    if (count == 0) {
        return;
    }

    GXRenderJobEntry* first = self->jobEntries;
    GXRenderJobEntry* last  = first + count;

    alignas(16) GXRenderJobSortContext sortCtx{};
    sortCtx.inlineBuf.begin    = nullptr;
    sortCtx.inlineBuf.current  = nullptr;
    sortCtx.inlineBuf.end      = nullptr;
    sortCtx.inlineBuf.capacity = (count + 1) / 2;
    sortCtx.tempBuf            = &sortCtx.inlineBuf;

    GXRenderJobSort_StableSortRecursive(first, last, count, &sortCtx);

    if (sortCtx.inlineBuf.begin != nullptr) {
        reinterpret_cast<void (*)(void*)>(GX_VMA(0x2bbe588ULL))(sortCtx.inlineBuf.begin);
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4b00 (269 Bytes)
// Enqueues a Type-1 render job packet (0x1C bytes: callback + 2 QWORD args)
// into `jobPacketArena` (`+0x48`) and appends a `GXRenderJobEntry` to `jobEntries`.
// ----------------------------------------------------------------------------
bool GXRenderJobQueue_EnqueueJobType1(
    GXRenderJobQueue* self,
    void* callbackFn,
    void* arg0,
    void* arg1,
    uint32_t sortKey)
{
    self->jobQueueMutex.vptr->Lock(&self->jobQueueMutex, 0xFFFFFFFFu);

    if (sortKey == 0xFFFFFFFFu) {
        uint32_t curKey = self->highestSortKey;
        uint32_t nextBucket = (curKey + 0x400000u) & 0x0FC00000u;
        sortKey = (curKey & 0xF03FFFFFu) | nextBucket;
    }

    auto* pkt = static_cast<uint8_t*>(GXLinearRingBuffer_Allocate(&self->jobPacketArena, 0x1Cu, 4u));
    void** payload = nullptr;
    if (pkt && self->jobCount < self->jobCapacity) {
        pkt[0] = 1; // Job Type 1 (callback + 2 arguments)
        *reinterpret_cast<uint16_t*>(pkt + 2) = 0x1Cu;
        uint32_t misalign = (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkt)) + 4u) & 3u;
        uint8_t padBytes = (misalign != 0) ? static_cast<uint8_t>(4u - misalign) : 0u;
        pkt[1] = padBytes;

        int32_t idx = self->jobCount++;
        GXRenderJobEntry* entry = &self->jobEntries[idx];
        entry->jobPacket = pkt;
        entry->sortKey = sortKey;
        payload = reinterpret_cast<void**>(pkt + 4 + padBytes);

        if (self->highestSortKey < sortKey) {
            self->highestSortKey = sortKey;
        }
    }

    self->jobQueueMutex.vptr->Unlock(&self->jobQueueMutex);
    if (!payload) {
        return false;
    }

    payload[0] = callbackFn;
    payload[1] = arg0;
    payload[2] = arg1;
    return true;
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4a00 (248 Bytes)
// Enqueues a Type-0 render job packet (0x0C bytes: single job object pointer
// whose vtable+0x00 is invoked during replay) into `jobPacketArena` (`+0x48`).
// ----------------------------------------------------------------------------
bool GXRenderJobQueue_EnqueueJobType0(
    GXRenderJobQueue* self,
    void* jobObject,
    uint32_t sortKey)
{
    if (jobObject == nullptr) {
        return false;
    }

    self->jobQueueMutex.vptr->Lock(&self->jobQueueMutex, 0xFFFFFFFFu);

    if (sortKey == 0xFFFFFFFFu) {
        uint32_t curKey = self->highestSortKey;
        uint32_t nextBucket = (curKey + 0x400000u) & 0x0FC00000u;
        sortKey = (curKey & 0xF03FFFFFu) | nextBucket;
    }

    auto* pkt = static_cast<uint8_t*>(GXLinearRingBuffer_Allocate(&self->jobPacketArena, 0x0Cu, 4u));
    void** payload = nullptr;
    if (pkt && self->jobCount < self->jobCapacity) {
        pkt[0] = 0; // Job Type 0 (job object pointer)
        *reinterpret_cast<uint16_t*>(pkt + 2) = 0x0Cu;
        uint32_t misalign = (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkt)) + 4u) & 3u;
        uint8_t padBytes = (misalign != 0) ? static_cast<uint8_t>(4u - misalign) : 0u;
        pkt[1] = padBytes;

        int32_t idx = self->jobCount++;
        GXRenderJobEntry* entry = &self->jobEntries[idx];
        entry->jobPacket = pkt;
        entry->sortKey = sortKey;
        payload = reinterpret_cast<void**>(pkt + 4 + padBytes);

        if (self->highestSortKey < sortKey) {
            self->highestSortKey = sortKey;
        }
    }

    self->jobQueueMutex.vptr->Unlock(&self->jobQueueMutex);
    if (!payload) {
        return false;
    }

    payload[0] = jobObject;
    return true;
}

// ----------------------------------------------------------------------------
// VMA: 0x21b49b0 (79 Bytes)
// Resets the render job queue counts (`+0x28`, `+0x38`), resets both linear
// ring buffers (`+0x48`, `+0x78`), and clears `highestSortKey` (`+0x40`).
// ----------------------------------------------------------------------------
void GXRenderJobQueue_Reset(GXRenderJobQueue* self) {
    self->stateIndex = 0;
    self->jobCount = 0;
    GXLinearRingBuffer_Reset(&self->jobPacketArena, self->backingBuffer, 0x800u);
    GXLinearRingBuffer_Reset(&self->payloadArena, self->backingBuffer, 0x800u);
    self->highestSortKey = 0;
}

// Helpers for Dantelion2 container allocator & recursive mutex lifecycle
static inline DLAllocator* GX_AcquireValidatedContainerAllocator() {
    auto** ppAlloc = reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL));
    DLAllocator* alloc = *ppAlloc;
    if (alloc == nullptr) {
        alloc = reinterpret_cast<DLAllocator* (*)()>(GX_VMA(0x2ba2b10ULL))();
        *ppAlloc = alloc;
    }
    uint64_t caps = 0ULL;
    alloc->vptr->GetCapabilities(&caps, alloc, 0u);
    if ((caps & 0x20ULL) == 0ULL) {
        reinterpret_cast<void (*)(uint32_t, uint32_t, uintptr_t, ...)>(GX_VMA(0x20b55b0ULL))(
            0u, 0x3Eu, GX_VMA(0x4737dc0ULL));
    }
    return alloc;
}

static inline void GX_HandleContainerAllocFailure() {
    if (*reinterpret_cast<uint32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0u) {
        reinterpret_cast<void (*)(void*, int)>(GX_VMA(0x2bbe978ULL))(
            gx_guest_tls_slot(0x53e4320ULL), 1);
    } else {
        reinterpret_cast<void (*)(uint32_t, uint32_t, uintptr_t, ...)>(GX_VMA(0x20b55b0ULL))(
            0u, 0x109u, GX_VMA(0x4737d80ULL));
    }
}

static inline void GX_InitDLMutex(DLMutex* mtx) {
    mtx->initialized = 0u;
    mtx->vptr = reinterpret_cast<DLMutex::VTable*>(GX_VMA(0x52b6230ULL));
    auto fnIsInitialized = reinterpret_cast<bool (*)(DLMutex*)>(GX_VMA(0x207ee80ULL));
    if (!fnIsInitialized(mtx)) {
        auto fnMutexInit = reinterpret_cast<int (*)(uint64_t*)>(GX_VMA(0x2083fe0ULL));
        if (fnMutexInit(&mtx->pthreadMutex) == 0) {
            mtx->initialized = 1u;
        }
    }
}

static inline void GX_DestroyPthreadMutexField(DLMutex* mtx) {
    auto fnPthreadDestroy = reinterpret_cast<int (*)(uint64_t*)>(GX_VMA(0x2bbfee8ULL));
    auto fnAssertLog      = reinterpret_cast<void (*)(uintptr_t, uint32_t, uintptr_t, ...)>(GX_VMA(0x20b55b0ULL));
    uint32_t rc = static_cast<uint32_t>(fnPthreadDestroy(&mtx->pthreadMutex));
    mtx->initialized = 0u;
    if (rc == 0x8001C010u) {
        return;
    }
    if (rc != 0u) {
        if (rc == 0x8001C016u) {
            fnAssertLog(GX_VMA(0x493c20eULL), 0x55u, GX_VMA(0x493c275ULL));
        }
        fnAssertLog(GX_VMA(0x493c20eULL), 0x57u, GX_VMA(0x493c2aeULL));
    }
}

static inline void GX_DestroyDLMutex(DLMutex* mtx) {
    mtx->vptr = reinterpret_cast<DLMutex::VTable*>(GX_VMA(0x52b6230ULL));
    auto fnIsInitialized = reinterpret_cast<bool (*)(DLMutex*)>(GX_VMA(0x207ee80ULL));
    if (fnIsInitialized(mtx)) {
        GX_DestroyPthreadMutexField(mtx);
    }
    mtx->vptr = reinterpret_cast<DLMutex::VTable*>(GX_VMA(0x53b2fa0ULL));
    if (mtx->initialized != 0u) {
        GX_DestroyPthreadMutexField(mtx);
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4770 (573 Bytes)
// Finalizes and frees the render job queue's entry arrays (via global allocator
// `0x5540680`), both mutexes (`+0xC8`, `+0xB0`), and both linear ring buffers
// (`+0x78`, `+0x48`). Note: `unusedRsi` is preserved for symbol mangling ABI.
// ----------------------------------------------------------------------------
void GXRenderJobQueue_Finalize(GXRenderJobQueue* self, DLAllocator* /*unusedRsi*/) {
    if (self->jobEntries != nullptr) {
        DLAllocator* jobAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540680ULL));
        jobAlloc->vptr->Free(jobAlloc, self->jobEntries);
        self->jobEntries = nullptr;
    }

    GX_DestroyDLMutex(&self->payloadMutex);
    GX_DestroyDLMutex(&self->jobQueueMutex);

    GXLinearRingBuffer_Destroy(&self->payloadArena);
    GXLinearRingBuffer_Destroy(&self->jobPacketArena);

    if (self->vecBegin != nullptr) {
        self->allocator->vptr->Free(self->allocator, self->vecBegin);
        self->vecCap   = nullptr;
        self->vecEnd   = nullptr;
        self->vecBegin = nullptr;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b43c0 (941 Bytes)
// Initializes `GXRenderJobQueue` with `maxJobs` capacity and `backingBuffer`
// for the packet and payload linear ring buffers using global allocators
// `0x5540670` and `0x5540680`. Note: dummy 4th/5th params preserved for ABI.
// ----------------------------------------------------------------------------
void GXRenderJobQueue_Initialize(
    GXRenderJobQueue* self,
    uint32_t maxJobs,
    void* backingBuffer,
    DLAllocator* /*unusedRcx*/,
    DLAllocator* /*unusedR8*/)
{
    DLAllocator* vecAlloc = GX_AcquireValidatedContainerAllocator();

    self->vecCap         = nullptr;
    self->vecEnd         = nullptr;
    self->vecBegin       = nullptr;
    self->allocator      = vecAlloc;
    self->stateIndex     = 0u;
    self->jobCount       = 0;
    self->highestSortKey = 0u;

    GXLinearRingBuffer_Construct(&self->jobPacketArena, 0u, 0x800u);
    GXLinearRingBuffer_Construct(&self->payloadArena, 0u, 0x800u);

    GX_InitDLMutex(&self->jobQueueMutex);
    GX_InitDLMutex(&self->payloadMutex);

    if (maxJobs > 0u) {
        size_t count = static_cast<size_t>(maxJobs);
        if (count > 0xFFFFFFFFFFFFFFFULL) {
            reinterpret_cast<void (*)(const char*)>(GX_VMA(0x2bbf1b8ULL))(
                reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
        }
        size_t byteSize = count * sizeof(GXRenderJobEntry);
        auto* vecMem = static_cast<GXRenderJobEntry*>(
            vecAlloc->vptr->AllocateAligned(vecAlloc, byteSize, 8u));
        if (vecMem == nullptr) {
            GX_HandleContainerAllocFailure();
        }
        if (vecMem != nullptr) {
            std::memset(vecMem, 0, byteSize);
        }
        self->vecCap   = vecMem + count;
        self->vecEnd   = vecMem + count;
        self->vecBegin = vecMem;
    }

    DLAllocator* jobAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540680ULL));
    size_t jobArrayBytes = static_cast<size_t>(maxJobs) * sizeof(GXRenderJobEntry);
    self->jobEntries    = static_cast<GXRenderJobEntry*>(jobAlloc->vptr->Allocate(jobAlloc, jobArrayBytes));
    self->jobCapacity   = static_cast<int32_t>(maxJobs);
    self->backingBuffer = backingBuffer;

    GXLinearRingBuffer_Reset(&self->jobPacketArena, backingBuffer, 0x800u);
    GXLinearRingBuffer_Reset(&self->payloadArena, backingBuffer, 0x800u);
}

// ============================================================================
// 2. GX Context State Shadow Pool & Deferred Stream Helpers (0x21b3910 .. 0x21b42a0)
// ============================================================================

struct DLWStringInline {
    void*        allocator; // +0xD8 (relative to parent +0x00)
    wchar_t      inlineBuf[8];
    uint64_t     length;    // +0xC8
    uint64_t     capacity;  // +0xD0
};

struct GXNamedContextStateShadow {
    uint8_t      stateShadow[0xB0]; // +0x00: CGContextStateShadowGnm
    uint8_t      nameString[0x30];  // +0xB0: DLWString (wchar_t name, len +0xC8, cap +0xD0, alloc +0xD8)
    uint8_t      activeFlag;        // +0xE0
    uint8_t      padE1[7];          // +0xE1
    uint32_t     stateId;           // +0xE8
    uint32_t     padEC;             // +0xEC
}; // sizeof == 0xF0 (240 bytes)

struct GXContextStateShadowPool {
    void*                      vptr;          // +0x00
    GXNamedContextStateShadow* begin;         // +0x08
    GXNamedContextStateShadow* end;           // +0x10
    GXNamedContextStateShadow* cap;           // +0x18
    DLAllocator*               allocator;     // +0x20
    uint32_t                   allocatedUsed; // +0x28
    uint32_t                   pad2C;         // +0x2C
    DLMutex                    poolMutex;     // +0x30 (0x18 bytes)
};

// ----------------------------------------------------------------------------
// VMA: 0x21b42a0 (130 Bytes)
// Finalizes the active command-list recording stream on the current thread's
// GX device context and submits it to the GX command dispatcher (`0x2194340`).
// ----------------------------------------------------------------------------
extern "C" void GXCommandListRecorder_EndRecording(void* recorder, uint32_t flags); // 0x0e47290
extern "C" void* GXCommandListRecorder_FinalizeStream(void* recorder);              // 0x0e47840
extern "C" void GXDevice_SubmitRecordedCommandList(void* gxMgr, void* devCtx, uint32_t mask); // 0x2194340

void GXDevice_CloseAndSubmitCommandList(void* /*unused*/) {
    auto** ppGlobalMgr = reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    auto* mgr = *ppGlobalMgr;
    if (mgr[0x3A9] != 0) {
        return;
    }

    auto* devCtx = *reinterpret_cast<uint8_t**>(mgr + (mgr[0x84] != 0 ? 0x78 : 0x10));
    void* recorder = devCtx + 0x1728;

    GXCommandListRecorder_EndRecording(recorder, 1u);
    *reinterpret_cast<void**>(devCtx + 0x1EC0) = GXCommandListRecorder_FinalizeStream(recorder);
    devCtx[0x1670] &= ~1u;

    GXDevice_SubmitRecordedCommandList(*ppGlobalMgr, devCtx, 0xFFFFFFFFu);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4190 (259 Bytes)
// Begins recording a new deferred command list on the active GX device context,
// binding the primary and secondary ring buffers and tagging it `"MainCommandList"`.
// ----------------------------------------------------------------------------
extern "C" void GXCommandListRecorder_BeginRecording(void* recorder, void* buf0, void* buf1, uint32_t flags); // 0x0e46f40
extern "C" void GXDeviceContext_ResetStateCache(void* devCtx); // 0x21a1c50

void GXDevice_BeginMainCommandListRecording(void* /*unusedRdi*/) {
    auto** ppGlobalMgr = reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* mgr = *ppGlobalMgr;
    if (mgr[0x3A9] != 0u) {
        return;
    }

    uint8_t* devCtx = *reinterpret_cast<uint8_t**>(mgr + (mgr[0x84] != 0u ? 0x78 : 0x10));

    uint8_t* sub90_a = *reinterpret_cast<uint8_t**>(mgr + 0x90);
    uint8_t* ring0   = *reinterpret_cast<uint8_t**>(sub90_a + 0x68);
    void* buf0 = nullptr;
    if (*reinterpret_cast<void**>(ring0 + 0x40) != nullptr
        && *reinterpret_cast<void**>(ring0 + 0x60) != nullptr)
    {
        auto fnValidate0 = reinterpret_cast<uint32_t (*)(void*)>(
            (*reinterpret_cast<void***>(ring0))[0x40 / sizeof(void*)]);
        if (fnValidate0(ring0) != 0u) {
            buf0 = **reinterpret_cast<void***>(ring0 + 0x60);
        }
    }

    mgr = *ppGlobalMgr;
    uint8_t* sub90_b = *reinterpret_cast<uint8_t**>(mgr + 0x90);
    uint8_t* ring1   = *reinterpret_cast<uint8_t**>(sub90_b + 0x70);
    void* buf1 = nullptr;
    if (*reinterpret_cast<void**>(ring1 + 0x60) != nullptr) {
        auto fnValidate1 = reinterpret_cast<uint32_t (*)(void*)>(
            (*reinterpret_cast<void***>(ring1))[0x40 / sizeof(void*)]);
        if (fnValidate1(ring1) != 0u) {
            buf1 = **reinterpret_cast<void***>(ring1 + 0x60);
        }
    }

    void* recorder = devCtx + 0x1728;
    GXCommandListRecorder_BeginRecording(recorder, buf0, buf1, 1u);
    *reinterpret_cast<void**>(devCtx + 0x1EC0) = GXCommandListRecorder_FinalizeStream(recorder);
    GXDeviceContext_ResetStateCache(devCtx);

    const void* tagUtf16 = reinterpret_cast<const void*>(GX_VMA(0x49c3696ULL));
    size_t tagLen = reinterpret_cast<size_t (*)(const void*)>(GX_VMA(0x2bbf228ULL))(tagUtf16);
    reinterpret_cast<void (*)(void*, const void*, size_t)>(GX_VMA(0x262a720ULL))(
        devCtx + 0x20A8, tagUtf16, tagLen);
    *reinterpret_cast<uint32_t*>(devCtx + 0x20E0) = 0x12u;
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4050 (166 Bytes)
// Allocates the next `GXNamedContextStateShadow` (`0xF0` bytes) from the pool
// and snapshots the current `CGContextStateShadowGnm` from `srcDeviceCtx`.
// ----------------------------------------------------------------------------
bool GXContextStateShadowPool_AcquireAndSnapshot(
    GXContextStateShadowPool* self,
    GXNamedContextStateShadow** outShadow,
    uint8_t* srcDeviceCtx)
{
    if (srcDeviceCtx[0x2093] != 0) {
        return false;
    }

    self->poolMutex.vptr->Lock(&self->poolMutex, 0xFFFFFFFFu);

    size_t totalCapacity = static_cast<size_t>(self->end - self->begin);
    GXNamedContextStateShadow* slot = nullptr;
    if (self->allocatedUsed < totalCapacity) {
        slot = &self->begin[self->allocatedUsed++];
    }

    self->poolMutex.vptr->Unlock(&self->poolMutex);

    if (slot == nullptr) {
        return false;
    }

    CGContextStateShadowGnm_CopyFromDevice(slot->stateShadow, srcDeviceCtx);
    *outShadow = slot;
    return true;
}

// ----------------------------------------------------------------------------
// VMA: 0x21b4040 (8 Bytes)
// Resets the used count (`+0x28`) of `GXContextStateShadowPool` to zero.
// ----------------------------------------------------------------------------
void GXContextStateShadowPool_ResetUsedCount(GXContextStateShadowPool* self) {
    self->allocatedUsed = 0;
}

static inline void GXNamedContextStateShadow_DestroyElement(GXNamedContextStateShadow* elem) {
    auto* raw = reinterpret_cast<uint8_t*>(elem);
    if (*reinterpret_cast<uint64_t*>(raw + 0xD0) >= 8ULL) {
        void* strBuf = *reinterpret_cast<void**>(raw + 0xB8);
        DLAllocator* strAlloc = *reinterpret_cast<DLAllocator**>(raw + 0xD8);
        strAlloc->vptr->Free(strAlloc, strBuf);
    }
    *reinterpret_cast<uint64_t*>(raw + 0xD0) = 7ULL;
    *reinterpret_cast<uint64_t*>(raw + 0xC8) = 0ULL;
    *reinterpret_cast<uint16_t*>(raw + 0xB8) = 0u;
    CGContextStateShadowGnm_Destroy(elem->stateShadow);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b3eb0 (385 Bytes)
// Destroys the pool's mutex (`+0x30`), releases embedded `DLWString` buffers
// and calls `CGContextStateShadowGnm_Destroy` (`0x26d56c0`) on all `0xF0`-byte
// `GXNamedContextStateShadow` elements, and frees the backing array.
// ----------------------------------------------------------------------------
void GXContextStateShadowPool_Finalize(GXContextStateShadowPool* self) {
    GX_DestroyDLMutex(&self->poolMutex);

    if (self->begin != nullptr) {
        GXNamedContextStateShadow* end = self->end;
        for (GXNamedContextStateShadow* it = self->begin; it != end; ++it) {
            GXNamedContextStateShadow_DestroyElement(it);
        }
        self->allocator->vptr->Free(self->allocator, self->begin);
        self->cap   = nullptr;
        self->end   = nullptr;
        self->begin = nullptr;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b3910 (1,436 Bytes)
// Initializes `GXContextStateShadowPool` using global container allocator
// `0x5540670`, initializes `poolMutex` (`+0x30`), and resizes its vector of
// `0xF0`-byte `GXNamedContextStateShadow` entries to `poolSize`, constructing
// each embedded `CGContextStateShadowGnm` (`0x26d5680`) and `DLWString` (`+0xB0`).
// Note: `unusedRdx` is preserved for symbol mangling ABI compatibility.
// ----------------------------------------------------------------------------
void GXContextStateShadowPool_Initialize(
    GXContextStateShadowPool* self,
    uint32_t poolSize,
    DLAllocator* /*unusedRdx*/)
{
    DLAllocator* alloc = GX_AcquireValidatedContainerAllocator();

    self->cap           = nullptr;
    self->end           = nullptr;
    self->begin         = nullptr;
    self->allocator     = alloc;
    self->allocatedUsed = 0u;

    GX_InitDLMutex(&self->poolMutex);

    if (poolSize > 0u) {
        size_t count = static_cast<size_t>(poolSize);
        if (count > 0x111111111111111ULL) {
            reinterpret_cast<void (*)(const char*)>(GX_VMA(0x2bbf1b8ULL))(
                reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
        }
        size_t totalBytes = count * sizeof(GXNamedContextStateShadow);
        auto* mem = static_cast<GXNamedContextStateShadow*>(
            alloc->vptr->AllocateAligned(alloc, totalBytes, 8u));
        if (mem == nullptr) {
            GX_HandleContainerAllocFailure();
        }
        if (mem != nullptr) {
            for (size_t i = 0; i < count; ++i) {
                auto* raw = reinterpret_cast<uint8_t*>(&mem[i]);
                CGContextStateShadowGnm_Construct(mem[i].stateShadow);
                DLAllocator* strAlloc = GX_AcquireValidatedContainerAllocator();
                *reinterpret_cast<DLAllocator**>(raw + 0xD8) = strAlloc;
                *reinterpret_cast<uint64_t*>(raw + 0xD0)     = 7ULL;
                *reinterpret_cast<uint64_t*>(raw + 0xC8)     = 0ULL;
                *reinterpret_cast<uint16_t*>(raw + 0xB8)     = 0u;
                mem[i].activeFlag = 1u;
                mem[i].stateId    = 0u;
            }
        }
        self->cap   = mem + count;
        self->end   = mem + count;
        self->begin = mem;
    }
}

// ============================================================================
// 3. GX Dual-Allocator Fallback Bridge (0x21b3390 .. 0x21b3900, 28 functions)
// ============================================================================
// Attempts memory allocations on `primary` (`[this+0x08]`), falling back to
// `secondary` (`[this+0x10]`) or the thread-local allocator when `primary`
// returns `nullptr`, and routes `Free`/`Reallocate` to whichever heap owns `ptr`.

struct GXFallbackAllocatorBridge {
    void*        vptr;      // +0x00
    DLAllocator* primary;   // +0x08
    DLAllocator* secondary; // +0x10
};

// VMA: 0x21b3900 (16 B) — vtable+0xD0 (GetBlockBase)
void* GXFallbackAllocator_GetBlockBase(GXFallbackAllocatorBridge* self, void* ptr) {
    return self->primary->vptr->GetBlockBase(self->primary, ptr);
}
// VMA: 0x21b38f0 (13 B) — vtable+0xC8 (Unlock)
void GXFallbackAllocator_Unlock(GXFallbackAllocatorBridge* self) {
    self->primary->vptr->Unlock(self->primary);
}
// VMA: 0x21b38e0 (13 B) — vtable+0xC0 (Lock)
void GXFallbackAllocator_Lock(GXFallbackAllocatorBridge* self) {
    self->primary->vptr->Lock(self->primary);
}
// VMA: 0x21b38d0 (15 B) — vtable+0xB8 (DumpStats)
void GXFallbackAllocator_DumpStats(GXFallbackAllocatorBridge* self) {
    self->primary->vptr->DumpStats(self->primary);
}
// VMA: 0x21b38c0 (16 B) — vtable+0xB0 (OwnsAllocation / ContainsAddress)
bool GXFallbackAllocator_OwnsAllocation(GXFallbackAllocatorBridge* self, void* ptr) {
    return self->primary->vptr->OwnsAllocation(self->primary, ptr);
}
// VMA: 0x21b38b0 (13 B) — vtable+0xA8 (ValidateHeap / CheckIntegrity)
bool GXFallbackAllocator_ValidateHeap(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->ValidateHeap(self->primary);
}
// VMA: 0x21b38a0 (10 B) — vtable+0x78 (FreeAll)
void GXFallbackAllocator_FreeAll(GXFallbackAllocatorBridge* self) {
    self->primary->vptr->FreeAll(self->primary);
}
// VMA: 0x21b3890 (13 B) — vtable+0x48 (GetBlockSize)
uint64_t GXFallbackAllocator_GetBlockSize(GXFallbackAllocatorBridge* self, void* ptr) {
    return self->primary->vptr->GetBlockSize(self->primary, ptr);
}
// VMA: 0x21b3880 (10 B) — vtable+0x40 (GetAllocationCount)
uint64_t GXFallbackAllocator_GetAllocationCount(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->GetAllocationCount(self->primary);
}
// VMA: 0x21b3870 (10 B) — vtable+0x38 (GetMaxAllocatable)
uint64_t GXFallbackAllocator_GetMaxAllocatable(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->GetMaxAllocatable(self->primary);
}
// VMA: 0x21b3860 (10 B) — vtable+0x30 (GetFreeSize)
uint64_t GXFallbackAllocator_GetFreeSize(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->GetFreeSize(self->primary);
}
// VMA: 0x21b3850 (10 B) — vtable+0x28 (GetTotalSize)
uint64_t GXFallbackAllocator_GetTotalSize(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->GetTotalSize(self->primary);
}
// VMA: 0x21b3830 (31 B) — vtable+0x20 (GetCapabilities)
void* GXFallbackAllocator_GetCapabilities(void* outCaps, GXFallbackAllocatorBridge* self) {
    self->primary->vptr->GetCapabilities(outCaps, self->primary, 0);
    return outCaps;
}
// VMA: 0x21b3820 (10 B) — vtable+0x18 (GetAllocatorId)
uint32_t GXFallbackAllocator_GetAllocatorId(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->GetAllocatorId(self->primary);
}
// VMA: 0x21b3810 (10 B) — vtable+0x10 (GetHeapTag)
void* GXFallbackAllocator_GetHeapTag(GXFallbackAllocatorBridge* self) {
    return self->primary->vptr->fn10(self->primary);
}
// VMA: 0x21b3800 (5 B) — vtable+0x08 (deleting destructor -> guest operator delete 0x2bbe588)
void GXFallbackAllocator_Delete(GXFallbackAllocatorBridge* self) {
    auto fnOpDelete = reinterpret_cast<void (*)(void*)>(GX_VMA(0x2bbe588ULL));
    fnOpDelete(self);
}
// VMA: 0x21b37f0 (1 B) — vtable+0x00 (in-place destructor)
void GXFallbackAllocator_DestroyInPlace(GXFallbackAllocatorBridge* /*self*/) {}

// VMA: 0x21b36e0 (119 B) — vtable+0xA0 (BackFree)
void GXFallbackAllocator_BackFree(GXFallbackAllocatorBridge* /*self*/, void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    uint8_t* tlsSlot = gx_guest_tls_slot(0x53e44a8ULL);
    DLAllocator* owner = *reinterpret_cast<DLAllocator**>(tlsSlot + 0x10);
    *reinterpret_cast<DLAllocator**>(tlsSlot + 0x10) = nullptr;
    if (owner == nullptr) {
        auto fnGetAlloc = reinterpret_cast<DLAllocator* (*)(void*)>(GX_VMA(0x207b720ULL));
        owner = fnGetAlloc(ptr);
        if (owner == nullptr) {
            auto fnAssertLog = reinterpret_cast<void (*)(uintptr_t, uint32_t, uintptr_t, ...)>(GX_VMA(0x20b55b0ULL));
            fnAssertLog(GX_VMA(0x49401b6ULL), 0xAEu, GX_VMA(0x4940207ULL));
            return;
        }
    }
    owner->vptr->BackFree(owner, ptr);
}

// VMA: 0x21b3680 (90 B) — vtable+0x98 (BackReallocateAligned)
void* GXFallbackAllocator_BackReallocateAligned(
    GXFallbackAllocatorBridge* self, void* ptr, size_t size, size_t align)
{
    if (ptr && self->secondary && !self->primary->vptr->OwnsAllocation(self->primary, ptr)) {
        return self->secondary->vptr->BackReallocateAligned(self->secondary, ptr, size, align);
    }
    void* p = self->primary->vptr->BackReallocateAligned(self->primary, ptr, size, align);
    if (!p && !ptr && self->secondary) {
        p = self->secondary->vptr->BackReallocateAligned(self->secondary, ptr, size, align);
    }
    return p;
}

// VMA: 0x21b3620 (87 B) — vtable+0x90 (BackReallocate)
void* GXFallbackAllocator_BackReallocate(
    GXFallbackAllocatorBridge* self, void* ptr, size_t size)
{
    if (ptr && self->secondary && !self->primary->vptr->OwnsAllocation(self->primary, ptr)) {
        return self->secondary->vptr->BackReallocate(self->secondary, ptr, size);
    }
    void* p = self->primary->vptr->BackReallocate(self->primary, ptr, size);
    if (!p && !ptr && self->secondary) {
        p = self->secondary->vptr->BackReallocate(self->secondary, ptr, size);
    }
    return p;
}

// VMA: 0x21b35c0 (87 B) — vtable+0x88 (BackAllocateAligned)
void* GXFallbackAllocator_BackAllocateAligned(
    GXFallbackAllocatorBridge* self, size_t size, size_t align)
{
    void* p = self->primary->vptr->BackAllocateAligned(self->primary, size, align);
    if (!p && self->secondary) {
        p = self->secondary->vptr->BackAllocateAligned(self->secondary, size, align);
    }
    return p;
}

// VMA: 0x21b3570 (66 B) — vtable+0x80 (BackAllocate)
void* GXFallbackAllocator_BackAllocate(
    GXFallbackAllocatorBridge* self, size_t size)
{
    void* p = self->primary->vptr->BackAllocate(self->primary, size);
    if (!p && self->secondary) {
        p = self->secondary->vptr->BackAllocate(self->secondary, size);
    }
    return p;
}

// VMA: 0x21b34f0 (116 B) — vtable+0x70 (Free)
void GXFallbackAllocator_Free(GXFallbackAllocatorBridge* /*self*/, void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    uint8_t* tlsSlot = gx_guest_tls_slot(0x53e44a8ULL);
    DLAllocator* owner = *reinterpret_cast<DLAllocator**>(tlsSlot + 0x10);
    *reinterpret_cast<DLAllocator**>(tlsSlot + 0x10) = nullptr;
    if (owner == nullptr) {
        auto fnGetAlloc = reinterpret_cast<DLAllocator* (*)(void*)>(GX_VMA(0x207b720ULL));
        owner = fnGetAlloc(ptr);
        if (owner == nullptr) {
            auto fnAssertLog = reinterpret_cast<void (*)(uintptr_t, uint32_t, uintptr_t)>(GX_VMA(0x20b55b0ULL));
            fnAssertLog(GX_VMA(0x493bac3ULL), 0x64u, GX_VMA(0x4940207ULL));
            return;
        }
    }
    owner->vptr->Free(owner, ptr);
}

// VMA: 0x21b3490 (84 B) — vtable+0x68 (ReallocateAligned)
void* GXFallbackAllocator_ReallocateAligned(
    GXFallbackAllocatorBridge* self, void* ptr, size_t size, size_t align)
{
    if (ptr && self->secondary && !self->primary->vptr->OwnsAllocation(self->primary, ptr)) {
        return self->secondary->vptr->ReallocateAligned(self->secondary, ptr, size, align);
    }
    void* p = self->primary->vptr->ReallocateAligned(self->primary, ptr, size, align);
    if (!p && !ptr && self->secondary) {
        p = self->secondary->vptr->ReallocateAligned(self->secondary, ptr, size, align);
    }
    return p;
}

// VMA: 0x21b3430 (81 B) — vtable+0x60 (Reallocate)
void* GXFallbackAllocator_Reallocate(
    GXFallbackAllocatorBridge* self, void* ptr, size_t size)
{
    if (ptr && self->secondary && !self->primary->vptr->OwnsAllocation(self->primary, ptr)) {
        return self->secondary->vptr->Reallocate(self->secondary, ptr, size);
    }
    void* p = self->primary->vptr->Reallocate(self->primary, ptr, size);
    if (!p && !ptr && self->secondary) {
        p = self->secondary->vptr->Reallocate(self->secondary, ptr, size);
    }
    return p;
}

// VMA: 0x21b33d0 (81 B) — vtable+0x58 (AllocateAligned)
void* GXFallbackAllocator_AllocateAligned(
    GXFallbackAllocatorBridge* self, size_t size, size_t align)
{
    void* p = self->primary->vptr->AllocateAligned(self->primary, size, align);
    if (!p && self->secondary) {
        p = self->secondary->vptr->AllocateAligned(self->secondary, size, align);
    }
    return p;
}

// VMA: 0x21b3390 (60 B) — vtable+0x50 (Allocate)
void* GXFallbackAllocator_Allocate(
    GXFallbackAllocatorBridge* self, size_t size)
{
    void* p = self->primary->vptr->Allocate(self->primary, size);
    if (!p && self->secondary) {
        p = self->secondary->vptr->Allocate(self->secondary, size);
    }
    return p;
}

// ============================================================================
// 4. GX Flip Controller & 30 FPS / 60 Hz Frame Pacer (0x21b2f10 .. 0x21b32e0)
// ============================================================================

struct GXFlipController {
    void*    pSwapChain;           // +0x00 (CGSwapChainGnm*)
    uint8_t  enableCpuFramePacer;  // +0x08 (default 1)
    uint8_t  pad09[3];             // +0x09
    uint32_t displayRefreshRateHz; // +0x0C (60 Hz = 0x3C)
    uint8_t  unkFlag10;            // +0x10 (default 1, copied from displayConfig+0x45)
    uint8_t  pad11[7];             // +0x11
    uint64_t lastPresentTimeUs;    // +0x18 (microseconds from sceKernelGettimeofday)
    uint32_t accumulatedFrames;    // +0x20 (frame interval accumulator)
    uint32_t pad24;                // +0x24
}; // sizeof == 0x28

struct GXOrbisTimeval {
    int64_t tv_sec;
    int64_t tv_usec;
};

static inline uint64_t GXFlipController_ReadTimeUs() {
    auto fnGetTime = reinterpret_cast<int (*)(GXOrbisTimeval*, void*)>(GX_VMA(0x2bbe728ULL));
    GXOrbisTimeval tv{};
    fnGetTime(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000000ULL + static_cast<uint64_t>(tv.tv_usec);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b32e0 (19 Bytes)
// Queries `CGSwapChainGnm_GetFlipStatus` (`0x26d5d90`) and returns `status == 0`.
// ----------------------------------------------------------------------------
bool GXFlipController_IsFlipPending(GXFlipController* self) {
    auto fnGetFlipStatus = reinterpret_cast<int (*)(void*)>(GX_VMA(0x26d5d90ULL));
    return fnGetFlipStatus(self->pSwapChain) == 0;
}

// ----------------------------------------------------------------------------
// VMA: 0x21b2fb0 (816 Bytes)
// Flushes dirty surface-sync ranges and the GNM graphics context if required,
// runs the CPU-side 60 Hz frame-pacing wait (`sceKernelUsleep` + busy-wait),
// and triggers `CGSwapChainGnm_PresentFlip` (`0x26d5dd0`).
// ----------------------------------------------------------------------------
bool GXFlipController_PresentAndWaitFramePacing(GXFlipController* self, uint32_t flipMode) {
    auto fnPresentFlip = reinterpret_cast<bool (*)(void*, uint32_t)>(GX_VMA(0x26d5dd0ULL));
    auto fnAssertLog   = reinterpret_cast<void (*)(uintptr_t, uint32_t, uintptr_t, ...)>(GX_VMA(0x20b55b0ULL));

    if (self->lastPresentTimeUs == 0ULL) {
        bool ok = fnPresentFlip(self->pSwapChain, flipMode);
        self->lastPresentTimeUs = GXFlipController_ReadTimeUs();
        self->accumulatedFrames = 0u;
        return ok;
    }

    const uint8_t preFlushEnabled = *reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac86ULL));
    if (preFlushEnabled != 0 && self->enableCpuFramePacer != 0 && flipMode != 0u) {
        bool doFlush = true;
        if (self->displayRefreshRateHz == 60u && static_cast<uint32_t>(flipMode - 2u) >= 3u) {
            doFlush = false;
            if (flipMode != 1u) {
                fnAssertLog(GX_VMA(0x4940153ULL), 0xCFu, GX_VMA(0x4940199ULL));
            }
        }
        if (doFlush) {
            uint8_t* pRoot = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
            uint8_t* pMgr10 = *reinterpret_cast<uint8_t**>(pRoot + 0x10);
            uint8_t* pCtxOwner = *reinterpret_cast<uint8_t**>(pMgr10 + 0x08);

            void* pSurfaceSync = pCtxOwner + 0xB478;
            void* pGfxCtx      = pCtxOwner + 0x10;

            auto fnHasDirty   = reinterpret_cast<bool (*)(void*)>(GX_VMA(0x26b9a50ULL));
            auto fnFlushDirty = reinterpret_cast<void (*)(void*)>(GX_VMA(0x26b9a40ULL));
            auto fnFlushGfx   = reinterpret_cast<void (*)(void*)>(GX_VMA(0x26afa00ULL));

            if (fnHasDirty(pSurfaceSync)) {
                fnFlushDirty(pSurfaceSync);
            }
            fnFlushGfx(pGfxCtx);
        }
    }

    if (self->enableCpuFramePacer == 0 || flipMode == 0u) {
        return fnPresentFlip(self->pSwapChain, flipMode);
    }

    if (self->displayRefreshRateHz == 60u && static_cast<uint32_t>(flipMode - 2u) >= 3u) {
        uint32_t presentMode = 1u;
        if (flipMode != 1u) {
            fnAssertLog(GX_VMA(0x4940153ULL), 0xCFu, GX_VMA(0x4940199ULL));
            presentMode = flipMode;
        }
        return fnPresentFlip(self->pSwapChain, presentMode);
    }

    const uint32_t newAccum = self->accumulatedFrames + flipMode;
    uint64_t nowUs = GXFlipController_ReadTimeUs();
    const uint32_t frameDelta = newAccum - self->accumulatedFrames;
    const uint64_t targetTimeUs =
        self->lastPresentTimeUs + (static_cast<uint64_t>(frameDelta) * 1000000ULL) / 60ULL;

    if (targetTimeUs > nowUs) {
        const uint64_t diffUs = targetTimeUs - nowUs;
        if (((diffUs * 1000000ULL) >> 6) >= 0xFD55AB1ULL) {
            const int32_t sleepUs = static_cast<int32_t>(diffUs) - 17000;
            if (sleepUs > 0) {
                auto fnUsleep = reinterpret_cast<int (*)(uint32_t)>(GX_VMA(0x2bbfe68ULL));
                fnUsleep(static_cast<uint32_t>(sleepUs));
            }
            nowUs = GXFlipController_ReadTimeUs();
        }
        while (targetTimeUs > nowUs) {
            nowUs = GXFlipController_ReadTimeUs();
        }
    }

    self->lastPresentTimeUs = GXFlipController_ReadTimeUs();
    self->accumulatedFrames = newAccum;

    uint32_t presentMode = 0u;
    const uint8_t pacerActive = self->enableCpuFramePacer;
    if (self->displayRefreshRateHz != 60u && pacerActive != 0) {
        return fnPresentFlip(self->pSwapChain, 0u);
    }

    if (pacerActive != 0) {
        if (flipMode <= 4u) {
            presentMode = (flipMode >= 2u) ? 0u : flipMode;
        } else {
            fnAssertLog(GX_VMA(0x4940153ULL), 0xCFu, GX_VMA(0x4940199ULL));
            presentMode = flipMode;
        }
    } else {
        if (flipMode <= 4u) {
            presentMode = (flipMode >= 2u) ? 1u : flipMode;
        } else {
            fnAssertLog(GX_VMA(0x4940153ULL), 0xBFu, GX_VMA(0x4940199ULL));
            fnAssertLog(GX_VMA(0x4940153ULL), 0xCFu, GX_VMA(0x4940199ULL));
            presentMode = flipMode;
        }
    }

    return fnPresentFlip(self->pSwapChain, presentMode);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b2f50 (90 Bytes)
// Initializes `GXFlipController` from a `CGSwapChainGnm*` and display config.
// ----------------------------------------------------------------------------
void GXFlipController_Initialize(GXFlipController* self, void* pSwapChain, const uint8_t* displayConfig) {
    self->pSwapChain = pSwapChain;
    uint32_t swapChainDesc[18]{};
    auto fnGetSwapChainDesc = reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x26d5d50ULL));
    fnGetSwapChainDesc(pSwapChain, swapChainDesc);
    self->displayRefreshRateHz = swapChainDesc[2];
    self->unkFlag10 = displayConfig[0x45];
}

// ----------------------------------------------------------------------------
// VMA: 0x21b2f40 (1 Byte)
// No-op finalizer for `GXFlipController`.
// ----------------------------------------------------------------------------
void GXFlipController_Finalize(GXFlipController* /*self*/) {}

// ----------------------------------------------------------------------------
// VMA: 0x21b2f10 (38 Bytes)
// Default constructor for `GXFlipController`.
// ----------------------------------------------------------------------------
void GXFlipController_Construct(GXFlipController* self) {
    self->pSwapChain = nullptr;
    self->enableCpuFramePacer = 0;
    self->displayRefreshRateHz = 0u;
    self->unkFlag10 = 1;
    self->lastPresentTimeUs = 0ULL;
    self->accumulatedFrames = 0u;
}

} // namespace GX
} // namespace FromSoftware
