// ============================================================================
// Bloodborne (CUSA03173 v1.09) - CoreGraphics2 GNM Backend
// File: CGGpuFreeListHeapAndViewFactoryGnm.cpp
// Address Range: 0x026a8090 .. 0x026aaba0 (67 raw functions + 3 carved refs)
// Subsystem:
//   1. CGGpuFreeListAllocatorGnm (0x26a9a70 .. 0x026aaba0, 15 raw + 3 carved)
//      - Thread-safe GPU Garlic/Onion range heap with doubly-linked address
//        list (+0x10/+0x18), free-block list (+0x20/+0x28), lock-free deferred
//        free queues (+0x98..+0xb0), End-of-Pipe GPU fence emission (`0x10778f0`),
//        and frame-boundary deferred block reclamation & coalescing (`0x26aa860`)
//   2. CGDescriptorViewFactoryGnm (0x26a94b0 .. 0x26a99d0, 15 functions)
//      - Allocates and initializes hardware descriptor views (T#/V#/S#) via
//        `0x026bad70` for shader resource/UAV/render-target/sampler views
//   3. DLScript::LuaTableValueProxy (0x26a8090 .. 0x26a93e0, 37 functions)
//      - Left-boundary Lua 5.x table/value proxy methods situated at the start
//        of the `0x26a8000` segment
// ============================================================================

#include <cstdint>
#include <cstring>
#include "gx_direct_vulkan.h"

namespace from {
namespace CoreGraphics2 {

struct IAllocator {
    struct VTable {
        void* (*dtor)(IAllocator*, int);
        void* pad_08[9];
        void* (*allocate)(IAllocator*, size_t size);                          // +0x50
        void* (*allocateAligned)(IAllocator*, size_t size, size_t alignment); // +0x58
        void* pad_60;
        void* pad_68;
        void  (*deallocate)(IAllocator*, void* ptr);                          // +0x70
    };
    VTable* vtable;
};

struct IMutex {
    struct VTable {
        void* (*dtor)(IMutex*, int);
        void* pad_08;
        void* pad_10;
        void  (*lock)(IMutex*, uint32_t timeout); // +0x18
        void* pad_20;
        void  (*unlock)(IMutex*);                 // +0x28
    };
    VTable* vtable;
};

// ============================================================================
// 1. CGGpuFreeListAllocatorGnm (0x26a9a70 .. 0x26aaba0)
// ============================================================================

struct CGGpuBlockNode {
    uint64_t        stateOrTaggedFence; // +0x00 (0 = free, 2 = allocated/pending EoP, bit0=1 -> external fence ptr)
    uintptr_t       gpuAddress;         // +0x08
    CGGpuBlockNode* prevByAddr;         // +0x10
    CGGpuBlockNode* nextByAddr;         // +0x18
    CGGpuBlockNode* prevFree;           // +0x20
    CGGpuBlockNode* nextFree;           // +0x28
};

extern "C" {
    void* FixedBlockPool_AllocateNode(void* pool);                              // 0x0263b8b0
    void  FixedBlockPool_FreeNode(void* pool, void* node);                      // 0x0263b9b0
    void  FixedBlockPool_Initialize(void* pool, void* mem, uint32_t nodeSize,
                                    uint32_t capacity, uint32_t align);         // 0x0263baa0
    void  FixedBlockPool_Finalize(void* pool);                                  // 0x0263bc40
    void* AtomicPtr_CompareExchange(void* volatile* dst, void* xchg, void* cmp);// 0x0207ebc0
    void  Mutex_Construct(void* mutex);                                         // 0x0207ecb0
    void  Mutex_Initialize(void* mutex);                                        // 0x0207ee40
    void  GfxOrComputeCtx_WriteReleaseMemEoP(
        void* ctx, uint32_t eventType, uint32_t cacheAction, void* dstGpuAddr,
        uint32_t dataSel, uint32_t value, uint32_t intSel, uint32_t wrConfirm); // 0x010778f0
}

// 0x026a9a70 (14 bytes)
// Reads the current state of a `CGGpuBlockNode` (dereferencing external fence if bit 0 is set).
uint32_t CGGpuBlockNode_GetState(const CGGpuBlockNode* node) {
    uint64_t val = node->stateOrTaggedFence;
    if (val & 1u) {
        return *reinterpret_cast<const uint32_t*>(val & ~1ULL);
    }
    return static_cast<uint32_t>(val);
}

// 0x026a9a80 (205 bytes)
// Default constructor of `CGGpuFreeListAllocatorGnm`.
void CGGpuFreeListAllocatorGnm_Construct(uint8_t* self) {
    self[0x67] = 0;
    std::memset(self + 0x98, 0, 0x40);
    Mutex_Construct(self + 0xd8);
    *reinterpret_cast<uint64_t*>(self + 0xd8) = GX_VMA(0x52b6230ULL); // vtable
    Mutex_Initialize(self + 0xd8);
    std::memset(self + 0x68, 0, 0x30);
}

// 0x026a9b50 (329 bytes)
// Initializes the GPU range heap over `[baseGpuAddr, baseGpuAddr + byteSize)`
// and sets up the internal 48-byte `CGGpuBlockNode` pool (`0xff0` nodes).
bool CGGpuFreeListAllocatorGnm_Initialize(
    uint8_t* self,
    void* nodePoolMem,
    uintptr_t baseGpuAddr,
    size_t byteSize)
{
    uintptr_t alignPad = (0ULL - baseGpuAddr) & 0xFULL;
    if (byteSize < alignPad) {
        return false;
    }
    uintptr_t alignedBase = baseGpuAddr + alignPad;
    size_t alignedSize = (byteSize - alignPad) & ~static_cast<size_t>(0xFULL);

    *reinterpret_cast<uintptr_t*>(self + 0xb8) = alignedBase;
    *reinterpret_cast<uintptr_t*>(self + 0xc0) = alignedBase + alignedSize;

    uintptr_t poolOffset = (0ULL - reinterpret_cast<uintptr_t>(self)) & 7ULL;
    uint8_t* poolPtr = self + poolOffset;
    if (poolPtr) {
        FixedBlockPool_Initialize(poolPtr, nodePoolMem, 0x30, 0xFF0, 8);
    }

    self[0x67] = 1;
    *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xd0) = alignedSize;

    CGGpuBlockNode* sentinel = reinterpret_cast<CGGpuBlockNode*>(self + 0x68);
    sentinel->stateOrTaggedFence = 0;
    sentinel->gpuAddress = baseGpuAddr + (alignPad | alignedSize);
    sentinel->prevByAddr = sentinel;
    sentinel->nextByAddr = sentinel;
    sentinel->prevFree   = sentinel;
    sentinel->nextFree   = sentinel;

    std::memset(self + 0x98, 0, 0x20);

    CGGpuBlockNode* rootBlock = static_cast<CGGpuBlockNode*>(FixedBlockPool_AllocateNode(poolPtr));
    if (!rootBlock) {
        if (self[0x67]) {
            FixedBlockPool_Finalize(poolPtr);
            self[0x67] = 0;
        }
        return false;
    }

    rootBlock->stateOrTaggedFence = 0;
    rootBlock->gpuAddress         = alignedBase;
    rootBlock->nextByAddr         = sentinel;
    rootBlock->prevByAddr         = sentinel;
    sentinel->prevByAddr          = rootBlock;
    sentinel->nextByAddr          = rootBlock;

    CGGpuBlockNode* oldFreeHead = sentinel->nextFree;
    sentinel->nextFree  = rootBlock;
    rootBlock->nextFree = oldFreeHead;
    rootBlock->prevFree = sentinel;
    oldFreeHead->prevFree = rootBlock;
    return true;
}

// 0x026a9ca0 (165 bytes)
// Finalizes `CGGpuFreeListAllocatorGnm` and destroys its fixed block pool.
void CGGpuFreeListAllocatorGnm_Finalize(uint8_t* self) {
    if (self[0x67] != 0) {
        uintptr_t poolOffset = (0ULL - reinterpret_cast<uintptr_t>(self)) & 7ULL;
        FixedBlockPool_Finalize(self + poolOffset);
        self[0x67] = 0;
    }
    std::memset(self + 0x68, 0, 0x60);
}

CGGpuBlockNode* CGGpuFreeListAllocatorGnm_AllocateAlignedInternal(
    uint8_t* self, size_t sizeInBytes, size_t alignment); // 0x026a9db0

// 0x026a9d50 (87 bytes)
// Thread-safe 16-byte aligned allocation from `CGGpuFreeListAllocatorGnm`.
CGGpuBlockNode* CGGpuFreeListAllocatorGnm_AllocateAlign16(uint8_t* self, size_t sizeInBytes) {
    IMutex* mutex = reinterpret_cast<IMutex*>(self + 0xd8);
    mutex->vtable->lock(mutex, 0xFFFFFFFFu);
    CGGpuBlockNode* block = CGGpuFreeListAllocatorGnm_AllocateAlignedInternal(self, sizeInBytes, 0x10);
    mutex->vtable->unlock(mutex);
    return block;
}

// 0x026a9db0 (487 bytes)
// First-fit aligned allocation from the doubly-linked free list (`sentinel->nextFree`).
// Splits front alignment padding and back remainder into separate free `CGGpuBlockNode`s.
CGGpuBlockNode* CGGpuFreeListAllocatorGnm_AllocateAlignedInternal(
    uint8_t* self,
    size_t sizeInBytes,
    size_t alignment)
{
    size_t alignedSize = (sizeInBytes + 0xFu) & ~static_cast<size_t>(0xFu);
    CGGpuBlockNode* sentinel = reinterpret_cast<CGGpuBlockNode*>(self + 0x68);
    size_t alignMask = alignment - 1u;
    size_t invAlignMask = ~alignMask;

    for (CGGpuBlockNode* cur = sentinel->nextFree; cur != sentinel; cur = cur->nextFree) {
        uintptr_t blockStart = cur->gpuAddress;
        uintptr_t blockEnd   = cur->nextByAddr->gpuAddress;
        uintptr_t alignedStart = (blockStart + alignMask) & invAlignMask;

        if (blockEnd < alignedStart) {
            continue;
        }
        size_t frontPad = alignedStart - blockStart;
        size_t availAfterAlign = blockEnd - alignedStart;
        if (availAfterAlign < alignedSize) {
            continue;
        }

        ++(*reinterpret_cast<uint64_t*>(self + 0xc8)); // allocation count

        if (frontPad != 0 || availAfterAlign > alignedSize) {
            uintptr_t poolOffset = (0ULL - reinterpret_cast<uintptr_t>(self)) & 7ULL;
            uint8_t* poolPtr = self + poolOffset;

            if (frontPad != 0) {
                CGGpuBlockNode* headPad = static_cast<CGGpuBlockNode*>(FixedBlockPool_AllocateNode(poolPtr));
                if (headPad) {
                    CGGpuBlockNode* oldPrev = cur->prevByAddr;
                    oldPrev->nextByAddr = headPad;
                    headPad->prevByAddr = oldPrev;
                    headPad->nextByAddr = cur;
                    cur->prevByAddr     = headPad;
                    headPad->stateOrTaggedFence = 0;
                    headPad->gpuAddress = cur->gpuAddress;

                    CGGpuBlockNode* freePrev = cur->prevFree;
                    CGGpuBlockNode* freeNext = freePrev->nextFree;
                    freePrev->nextFree = headPad;
                    headPad->nextFree  = freeNext;
                    headPad->prevFree  = freePrev;
                    freeNext->prevFree = headPad;
                }
                cur->gpuAddress = alignedStart;
            }

            if (availAfterAlign > alignedSize) {
                CGGpuBlockNode* tailRem = static_cast<CGGpuBlockNode*>(FixedBlockPool_AllocateNode(poolPtr));
                if (tailRem) {
                    CGGpuBlockNode* oldNext = cur->nextByAddr;
                    cur->nextByAddr     = tailRem;
                    tailRem->prevByAddr = cur;
                    tailRem->nextByAddr = oldNext;
                    oldNext->prevByAddr = tailRem;
                    tailRem->stateOrTaggedFence = 0;
                    tailRem->gpuAddress = cur->gpuAddress + alignedSize;

                    CGGpuBlockNode* freePrev = cur->prevFree;
                    CGGpuBlockNode* freeNext = freePrev->nextFree;
                    freePrev->nextFree = tailRem;
                    tailRem->nextFree  = freeNext;
                    tailRem->prevFree  = freePrev;
                    freeNext->prevFree = tailRem;
                }
            }
        }

        // Unlink `cur` from free list
        cur->prevFree->nextFree = cur->nextFree;
        cur->nextFree->prevFree = cur->prevFree;

        size_t finalSize = cur->nextByAddr->gpuAddress - cur->gpuAddress;
        *reinterpret_cast<uint64_t*>(self + 0xd0) -= finalSize;
        cur->nextFree = nullptr;
        cur->prevFree = nullptr;
        cur->stateOrTaggedFence = 2; // allocated
        gx_direct_vk::NoteDirectFreshGpuRange(reinterpret_cast<const void*>(cur->gpuAddress),
                                              finalSize);
        return cur;
    }
    return nullptr;
}

// 0x026a9fa0 (108 bytes)
// Thread-safe power-of-two aligned allocation from `CGGpuFreeListAllocatorGnm`.
CGGpuBlockNode* CGGpuFreeListAllocatorGnm_AllocateAligned(
    uint8_t* self,
    size_t sizeInBytes,
    size_t alignment)
{
    if ((alignment & (alignment - 1u)) != 0) {
        return nullptr;
    }
    IMutex* mutex = reinterpret_cast<IMutex*>(self + 0xd8);
    mutex->vtable->lock(mutex, 0xFFFFFFFFu);
    size_t effAlign = (alignment > 0x10u) ? alignment : 0x10u;
    CGGpuBlockNode* block = CGGpuFreeListAllocatorGnm_AllocateAlignedInternal(self, sizeInBytes, effAlign);
    mutex->vtable->unlock(mutex);
    return block;
}

// 0x026aa2e0 (346 bytes)
// Immediately frees `block` and coalesces it with adjacent free blocks in `nextByAddr`/`prevByAddr`.
void CGGpuFreeListAllocatorGnm_FreeBlockImmediateA(uint8_t* self, CGGpuBlockNode* block) {
    IMutex* mutex = reinterpret_cast<IMutex*>(self + 0xd8);
    mutex->vtable->lock(mutex, 0xFFFFFFFFu);

    const uint64_t freedAddr = block->gpuAddress;
    const uint64_t freedSize = block->nextByAddr->gpuAddress - freedAddr;
    gx_direct_vk::RetireDirectGpuRange(reinterpret_cast<const void*>(freedAddr), freedSize);

    --(*reinterpret_cast<uint64_t*>(self + 0xc8));
    *reinterpret_cast<uint64_t*>(self + 0xd0) += freedSize;

    CGGpuBlockNode* sentinel = reinterpret_cast<CGGpuBlockNode*>(self + 0x68);
    uintptr_t poolOffset = (0ULL - reinterpret_cast<uintptr_t>(self)) & 7ULL;
    uint8_t* poolPtr = self + poolOffset;

    // Coalesce forward
    CGGpuBlockNode* next = block->nextByAddr;
    while (next->stateOrTaggedFence == 0 && next != sentinel) {
        CGGpuBlockNode* after = next->nextByAddr;
        FixedBlockPool_FreeNode(poolPtr, next);
        next = after;
    }

    // Coalesce backward
    CGGpuBlockNode* prev = block->prevByAddr;
    while (prev->stateOrTaggedFence == 0 && prev != sentinel) {
        CGGpuBlockNode* victim = block;
        block = prev;
        std::memset(victim, 0, sizeof(CGGpuBlockNode));
        FixedBlockPool_FreeNode(poolPtr, victim);
        prev = block->prevByAddr;
    }

    if (next != block->nextByAddr) {
        block->nextByAddr = next;
        next->prevByAddr  = block;
    }

    // Re-insert into address-ordered free list
    CGGpuBlockNode* nextFreeNeighbor = block->nextByAddr;
    while (nextFreeNeighbor->stateOrTaggedFence != 0) {
        nextFreeNeighbor = nextFreeNeighbor->nextByAddr;
    }
    CGGpuBlockNode* prevFreeNeighbor = block->prevByAddr;
    while (prevFreeNeighbor->stateOrTaggedFence != 0) {
        prevFreeNeighbor = prevFreeNeighbor->prevByAddr;
    }

    prevFreeNeighbor->nextFree = block;
    block->nextFree            = nextFreeNeighbor;
    block->prevFree            = prevFreeNeighbor;
    nextFreeNeighbor->prevFree = block;
    block->stateOrTaggedFence  = 0;

    mutex->vtable->unlock(mutex);
}

// 0x026aa440 (346 bytes)
// Duplicate/alias immediate free entry point (`FreeBlockImmediateB`).
void CGGpuFreeListAllocatorGnm_FreeBlockImmediateB(uint8_t* self, CGGpuBlockNode* block) {
    CGGpuFreeListAllocatorGnm_FreeBlockImmediateA(self, block);
}

// 0x026aa5a0 (124 bytes)
// Pushes `block` onto lock-free deferred-free queue `+0x98` and emits an
// End-of-Pipe releaseMem write (`value = 4`) into `&block->stateOrTaggedFence`.
void CGGpuFreeListAllocatorGnm_DeferFreeWithEoPLabelA(
    uint8_t* self,
    void** ctxHandle,
    CGGpuBlockNode* block)
{
    void* volatile* queueHead = reinterpret_cast<void* volatile*>(self + 0x98);
    CGGpuBlockNode* oldHead;
    do {
        oldHead = static_cast<CGGpuBlockNode*>(*queueHead);
        block->nextFree = oldHead;
    } while (AtomicPtr_CompareExchange(queueHead, block, oldHead) != oldHead);

    block->stateOrTaggedFence = 2;
    if (!gx_direct_vk::TryEmitSingleReleaseU32Label(
            *ctxHandle, reinterpret_cast<uint64_t>(block), 4u)) {
        GfxOrComputeCtx_WriteReleaseMemEoP(*ctxHandle, 0x28, 0, block, 1, 4, 0, 2);
    }
}

// 0x026aa620 (124 bytes)
// Pushes `block` onto lock-free deferred-free queue `+0xa0` and emits an EoP fence (`value = 4`).
void CGGpuFreeListAllocatorGnm_DeferFreeWithEoPLabelB(
    uint8_t* self,
    void** ctxHandle,
    CGGpuBlockNode* block)
{
    void* volatile* queueHead = reinterpret_cast<void* volatile*>(self + 0xa0);
    CGGpuBlockNode* oldHead;
    do {
        oldHead = static_cast<CGGpuBlockNode*>(*queueHead);
        block->nextFree = oldHead;
    } while (AtomicPtr_CompareExchange(queueHead, block, oldHead) != oldHead);

    block->stateOrTaggedFence = 2;
    if (!gx_direct_vk::TryEmitSingleReleaseU32Label(
            *ctxHandle, reinterpret_cast<uint64_t>(block), 4u)) {
        GfxOrComputeCtx_WriteReleaseMemEoP(*ctxHandle, 0x28, 0, block, 1, 4, 0, 2);
    }
}

// 0x026aa6a0 (210 bytes)
// Deferred-free helper A: either chains `block` to an existing fence `sharedFenceBlock`
// (queue `+0xa8`) or emits a new EoP fence on queue `+0x98`.
CGGpuBlockNode* CGGpuFreeListAllocatorGnm_DeferFreeOrChainA(
    uint8_t* self,
    void** ctxHandle,
    CGGpuBlockNode* block,
    CGGpuBlockNode* sharedFenceBlock)
{
    if (sharedFenceBlock) {
        void* volatile* queueHead = reinterpret_cast<void* volatile*>(self + 0xa8);
        CGGpuBlockNode* oldHead;
        do {
            oldHead = static_cast<CGGpuBlockNode*>(*queueHead);
            block->nextFree = oldHead;
        } while (AtomicPtr_CompareExchange(queueHead, block, oldHead) != oldHead);

        block->stateOrTaggedFence = reinterpret_cast<uintptr_t>(sharedFenceBlock) | 1ULL;
        return sharedFenceBlock;
    }

    CGGpuFreeListAllocatorGnm_DeferFreeWithEoPLabelA(self, ctxHandle, block);
    return block;
}

// 0x026aa780 (210 bytes)
// Deferred-free helper B: either chains `block` to `sharedFenceBlock` (queue `+0xb0`)
// or emits a new EoP fence on queue `+0xa0`.
CGGpuBlockNode* CGGpuFreeListAllocatorGnm_DeferFreeOrChainB(
    uint8_t* self,
    void** ctxHandle,
    CGGpuBlockNode* block,
    CGGpuBlockNode* sharedFenceBlock)
{
    if (sharedFenceBlock) {
        void* volatile* queueHead = reinterpret_cast<void* volatile*>(self + 0xb0);
        CGGpuBlockNode* oldHead;
        do {
            oldHead = static_cast<CGGpuBlockNode*>(*queueHead);
            block->nextFree = oldHead;
        } while (AtomicPtr_CompareExchange(queueHead, block, oldHead) != oldHead);

        block->stateOrTaggedFence = reinterpret_cast<uintptr_t>(sharedFenceBlock) | 1ULL;
        return sharedFenceBlock;
    }

    CGGpuFreeListAllocatorGnm_DeferFreeWithEoPLabelB(self, ctxHandle, block);
    return block;
}

// 0x026aa860 (703 bytes)
// Drains all 4 lock-free deferred-free queues (`+0xa8`, `+0xb0`, `+0x98`, `+0xa0`),
// reclaims and coalesces all blocks whose GPU EoP fence has reached `4`, and
// pushes still-in-flight blocks back onto the lock-free queues.
void CGGpuFreeListAllocatorGnm_ReclaimCompletedDeferredFrees(uint8_t* self, uint8_t** ctxHandle) {
    gx_direct_vk::NoteGpuAllocator(
        self,
        *reinterpret_cast<const uint64_t*>(self + 0xc8),
        *reinterpret_cast<const uint64_t*>(self + 0xd0));
    IMutex* mutex = reinterpret_cast<IMutex*>(self + 0xd8);
    mutex->vtable->lock(mutex, 0xFFFFFFFFu);

    void* volatile* queues[4] = {
        reinterpret_cast<void* volatile*>(self + 0xa8),
        reinterpret_cast<void* volatile*>(self + 0xb0),
        reinterpret_cast<void* volatile*>(self + 0x98),
        reinterpret_cast<void* volatile*>(self + 0xa0)
    };

    CGGpuBlockNode* sentinel = reinterpret_cast<CGGpuBlockNode*>(self + 0x68);
    uintptr_t poolOffset = (0ULL - reinterpret_cast<uintptr_t>(self)) & 7ULL;
    uint8_t* poolPtr = self + poolOffset;

    for (int q = 0; q < 4; ++q) {
        void* volatile* qHead = queues[q];
        CGGpuBlockNode* list;
        do {
            list = static_cast<CGGpuBlockNode*>(*qHead);
        } while (AtomicPtr_CompareExchange(qHead, nullptr, list) != list);

        if (!list) {
            continue;
        }

        CGGpuBlockNode dummyPending{};
        CGGpuBlockNode* pendingTail = &dummyPending;

        CGGpuBlockNode* curr = list;
        while (curr) {
            CGGpuBlockNode* nextInQueue = curr->nextFree;
            uint32_t fenceVal = CGGpuBlockNode_GetState(curr);
            if (fenceVal != 4u) {
                pendingTail->nextFree = curr;
                pendingTail = curr;
                curr = nextInQueue;
                continue;
            }

            // Block GPU fence completed (== 4): reclaim and coalesce
            const uint64_t freedAddr = curr->gpuAddress;
            const uint64_t freedSize = curr->nextByAddr->gpuAddress - freedAddr;
            gx_direct_vk::RetireDirectGpuRange(reinterpret_cast<const void*>(freedAddr), freedSize);

            --(*reinterpret_cast<uint64_t*>(self + 0xc8));
            *reinterpret_cast<uint64_t*>(self + 0xd0) += freedSize;

            CGGpuBlockNode* next = curr->nextByAddr;
            while (next->stateOrTaggedFence == 0 && next != sentinel) {
                CGGpuBlockNode* after = next->nextByAddr;
                FixedBlockPool_FreeNode(poolPtr, next);
                next = after;
            }

            CGGpuBlockNode* prev = curr->prevByAddr;
            while (prev->stateOrTaggedFence == 0 && prev != sentinel) {
                CGGpuBlockNode* victim = curr;
                curr = prev;
                std::memset(victim, 0, sizeof(CGGpuBlockNode));
                FixedBlockPool_FreeNode(poolPtr, victim);
                prev = curr->prevByAddr;
            }

            if (next != curr->nextByAddr) {
                curr->nextByAddr = next;
                next->prevByAddr = curr;
            }

            CGGpuBlockNode* nextFreeNeighbor = curr->nextByAddr;
            while (nextFreeNeighbor->stateOrTaggedFence != 0) {
                nextFreeNeighbor = nextFreeNeighbor->nextByAddr;
            }
            CGGpuBlockNode* prevFreeNeighbor = curr->prevByAddr;
            while (prevFreeNeighbor->stateOrTaggedFence != 0) {
                prevFreeNeighbor = prevFreeNeighbor->prevByAddr;
            }

            prevFreeNeighbor->nextFree = curr;
            curr->nextFree             = nextFreeNeighbor;
            curr->prevFree             = prevFreeNeighbor;
            nextFreeNeighbor->prevFree = curr;
            curr->stateOrTaggedFence   = 0;

            curr = nextInQueue;
        }

        // Re-enqueue any blocks still in flight on the GPU
        if (dummyPending.nextFree) {
            CGGpuBlockNode* oldHead;
            do {
                oldHead = static_cast<CGGpuBlockNode*>(*qHead);
                pendingTail->nextFree = oldHead;
            } while (AtomicPtr_CompareExchange(qHead, dummyPending.nextFree, oldHead) != oldHead);
        }
    }

    // Mark shader cache / CB invalidation flags on context
    uint8_t* gfxCtx = *ctxHandle;
    *reinterpret_cast<uint32_t*>(gfxCtx + 0xb468) |= 0x3u;
    gfxCtx[0xb46f] |= 0x8u;

    mutex->vtable->unlock(mutex);
}

// 0x026aab20 (118 bytes)
// Returns the size in bytes of the largest contiguous free block in the heap.
size_t CGGpuFreeListAllocatorGnm_GetLargestFreeBlockSize(uint8_t* self) {
    IMutex* mutex = reinterpret_cast<IMutex*>(self + 0xd8);
    mutex->vtable->lock(mutex, 0xFFFFFFFFu);

    CGGpuBlockNode* sentinel = reinterpret_cast<CGGpuBlockNode*>(self + 0x68);
    size_t maxFree = 0;
    for (CGGpuBlockNode* cur = sentinel->nextFree; cur != sentinel; cur = cur->nextFree) {
        size_t blockSize = cur->nextByAddr->gpuAddress - cur->gpuAddress;
        if (blockSize > maxFree) {
            maxFree = blockSize;
        }
    }

    mutex->vtable->unlock(mutex);
    return maxFree;
}

// 0x026aaba0 (82 bytes)
// Returns the size in bytes of a specific allocated or free `CGGpuBlockNode`.
size_t CGGpuFreeListAllocatorGnm_GetBlockSize(uint8_t* self, const CGGpuBlockNode* block) {
    IMutex* mutex = reinterpret_cast<IMutex*>(self + 0xd8);
    mutex->vtable->lock(mutex, 0xFFFFFFFFu);
    size_t sz = block->nextByAddr->gpuAddress - block->gpuAddress;
    mutex->vtable->unlock(mutex);
    return sz;
}

// ============================================================================
// 2. CGDescriptorViewFactoryGnm (0x26a94b0 .. 0x26a99d0)
// ============================================================================

struct CGDescriptorViewGnm {
    uint32_t    viewCategory; // +0x00 (0 = resource/target view, 1 = sampler/cbv view)
    uint32_t    viewSubType;  // +0x04
    IAllocator* allocator;    // +0x08
    void*       hwDescMem;    // +0x10
};

extern bool CGDescriptorHwGnm_InitializeDefault(void* hwDescMem); // 0x026bad70

// 0x026a94b0 (1 byte)
void CGDescriptorViewGnm_BaseDtorNoop(void* /*self*/) {}

// 0x026a94c0 (4 bytes)
void* CGDescriptorViewGnm_Identity(void* self) { return self; }

// 0x026a94d0 (1 byte)
void CGDescriptorViewGnm_BaseCopyNoop(void* /*dst*/, const void* /*src*/) {}

// 0x026a9570 (45 bytes)
void CGDescriptorViewGnm_CopyConstructSamplerView(
    CGDescriptorViewGnm* dst, const CGDescriptorViewGnm* src)
{
    CGDescriptorViewGnm_BaseCopyNoop(dst, src);
    dst->hwDescMem = src->hwDescMem;
    *reinterpret_cast<uint64_t*>(&dst->viewCategory) =
        *reinterpret_cast<const uint64_t*>(&src->viewCategory);
    dst->allocator = src->allocator;
}

void CGDescriptorViewGnm_DestroySamplerView(CGDescriptorViewGnm* self); // 0x026a97d0

// 0x026a95a0 (28 bytes)
void CGDescriptorViewGnm_DestructSamplerView(CGDescriptorViewGnm* self) {
    CGDescriptorViewGnm_DestroySamplerView(self);
    CGDescriptorViewGnm_BaseDtorNoop(self);
}

// 0x026a95c0 (5 bytes)
void CGDescriptorViewGnm_GetSamplerSubTypeThunk(const CGDescriptorViewGnm* self, uint32_t* outType);

// 0x026a9660 (45 bytes)
void CGDescriptorViewGnm_CopyConstructResourceView(
    CGDescriptorViewGnm* dst, const CGDescriptorViewGnm* src)
{
    CGDescriptorViewGnm_BaseCopyNoop(dst, src);
    dst->hwDescMem = src->hwDescMem;
    *reinterpret_cast<uint64_t*>(&dst->viewCategory) =
        *reinterpret_cast<const uint64_t*>(&src->viewCategory);
    dst->allocator = src->allocator;
}

void CGDescriptorViewGnm_DestroyResourceView(CGDescriptorViewGnm* self); // 0x026a99a0

// 0x026a9690 (28 bytes)
void CGDescriptorViewGnm_DestructResourceView(CGDescriptorViewGnm* self) {
    CGDescriptorViewGnm_DestroyResourceView(self);
    CGDescriptorViewGnm_BaseDtorNoop(self);
}

// 0x026a96b0 (5 bytes)
void CGDescriptorViewGnm_GetResourceSubTypeThunk(const CGDescriptorViewGnm* self, uint32_t* outType);

// 0x026a9750 (127 bytes)
// Creates a category-1 view (subType 0 -> 0x80 bytes, subType 1 -> 0x08 bytes).
int32_t CGDescriptorViewGnm_CreateSamplerView(
    CGDescriptorViewGnm* self,
    void* /*unusedRsi*/,
    const uint32_t* subTypePtr,
    void* /*unusedRcx*/,
    IAllocator* alloc)
{
    self->viewCategory = 1;
    self->viewSubType  = *subTypePtr;
    self->allocator    = alloc;
    self->hwDescMem    = nullptr;

    size_t allocBytes = 0;
    if (*subTypePtr == 0) {
        allocBytes = 0x80;
    } else if (*subTypePtr == 1) {
        allocBytes = 0x08;
    } else {
        return 8;
    }

    void* mem = alloc->vtable->allocate(alloc, allocBytes);
    self->hwDescMem = mem;
    if (!mem) {
        return 9;
    }
    if (!CGDescriptorHwGnm_InitializeDefault(mem)) {
        alloc->vtable->deallocate(alloc, self->hwDescMem);
        self->hwDescMem = nullptr;
        return 8;
    }
    return 0;
}

// 0x026a97d0 (48 bytes)
void CGDescriptorViewGnm_DestroySamplerView(CGDescriptorViewGnm* self) {
    if (self->allocator && self->hwDescMem) {
        self->allocator->vtable->deallocate(self->allocator, self->hwDescMem);
        self->hwDescMem = nullptr;
    }
}

// 0x026a9800 (6 bytes)
void CGDescriptorViewGnm_GetSamplerSubType(const CGDescriptorViewGnm* self, uint32_t* outType) {
    *outType = self->viewSubType;
}

void CGDescriptorViewGnm_GetSamplerSubTypeThunk(const CGDescriptorViewGnm* self, uint32_t* outType) {
    CGDescriptorViewGnm_GetSamplerSubType(self, outType);
}

// 0x026a98a0 (256 bytes)
// Creates a category-0 hardware resource/target descriptor view (subTypes 0..15).
int32_t CGDescriptorViewGnm_CreateResourceView(
    CGDescriptorViewGnm* self,
    void* /*unusedRsi*/,
    const uint32_t* subTypePtr,
    void* /*unusedRcx*/,
    IAllocator* alloc)
{
    self->viewCategory = 0;
    self->viewSubType  = *subTypePtr;
    self->allocator    = alloc;
    self->hwDescMem    = nullptr;

    uint32_t st = *subTypePtr;
    if (st > 0x0F) {
        return 8;
    }

    size_t allocBytes = 0x08;
    switch (st) {
        case 0:  allocBytes = 0x28; break;
        case 1:  allocBytes = 0x80; break;
        case 2:  allocBytes = 0x10; break;
        case 3:  allocBytes = 0x18; break;
        case 4:  allocBytes = 0xB8; break;
        case 5:  allocBytes = 0x88; break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 13:
        case 14:
        case 15: allocBytes = 0x08; break;
        case 11: allocBytes = 0x80; break;
        case 12: allocBytes = 0x88; break;
    }

    void* mem = alloc->vtable->allocate(alloc, allocBytes);
    self->hwDescMem = mem;
    if (!mem) {
        return 9;
    }
    if (!CGDescriptorHwGnm_InitializeDefault(mem)) {
        alloc->vtable->deallocate(alloc, self->hwDescMem);
        self->hwDescMem = nullptr;
        return 8;
    }
    return 0;
}

// 0x026a99a0 (48 bytes)
void CGDescriptorViewGnm_DestroyResourceView(CGDescriptorViewGnm* self) {
    if (self->allocator && self->hwDescMem) {
        self->allocator->vtable->deallocate(self->allocator, self->hwDescMem);
        self->hwDescMem = nullptr;
    }
}

// 0x026a99d0 (6 bytes)
void CGDescriptorViewGnm_GetResourceSubType(const CGDescriptorViewGnm* self, uint32_t* outType) {
    *outType = self->viewSubType;
}

void CGDescriptorViewGnm_GetResourceSubTypeThunk(const CGDescriptorViewGnm* self, uint32_t* outType) {
    CGDescriptorViewGnm_GetResourceSubType(self, outType);
}

// ============================================================================
// 3. DLScript::LuaTableValueProxy (0x26a8090 .. 0x26a93e0, 26 functions)
// ============================================================================

extern "C" {
    int32_t     lua_gettop(void* L);                                       // 0x02107550
    void        lua_settop(void* L, int32_t idx);                          // 0x02107560
    void        lua_pushvalue(void* L, int32_t idx);                       // 0x02107670
    int32_t     lua_type(void* L, int32_t idx);                            // 0x02107820
    double      lua_tonumber(void* L, int32_t idx);                        // 0x02107e60
    const char* lua_tostring(void* L, int32_t idx);                        // 0x02107ff0
    void*       lua_touserdata(void* L, int32_t idx);                      // 0x02108260
    void        lua_pushnil(void* L);                                      // 0x02108520
    void        lua_pushnumber(void* L, double n);                         // 0x02108530
    void        lua_pushlstring(void* L, const void* s, size_t len);       // 0x02108550
    void        lua_pushstring(void* L, const char* s);                    // 0x021085b0
    void        lua_getfield(void* L, int32_t idx, const char* k);         // 0x021088e0
    void        lua_rawgeti(void* L, int32_t idx, int32_t n);              // 0x02108a30
    void        lua_settable(void* L, int32_t idx);                        // 0x02108d00
    int32_t     lua_setmetatable(void* L, int32_t objindex);               // 0x02109010
    void*       lua_newuserdata(void* L, size_t sz);                       // 0x02109610
    void        lua_rawseti(void* L, int32_t idx, int32_t n);              // 0x02109770
    void        lua_getUserKey(void* L, int32_t keyId, void* outBuf);      // 0x021099a0
    void        lua_setUserKeyVal(void* L, void* keyBuf, int32_t val);     // 0x02109b20
    void        lua_getUserKeyTable(void* L, const char* tbl, void* kBuf); // 0x02109c20
    int32_t     luaL_ref(void* L, int32_t t);                              // 0x0211f0b0
    void        luaL_unref(void* L, int32_t t, int32_t ref);               // 0x0211f1c0
    void        LuaProxy_PushOnStack(void* proxy);                         // 0x026a7f80
}

struct LuaStateWrapper {
    void* pad_00;
    void* allocOwner; // +0x08
    void* L;          // +0x10
};

struct LuaTableValueProxy {
    void*            vtable;      // +0x00 (0x052b9eb0)
    uint32_t         valueType;   // +0x08 (0=nil, 3=number, 4=string, 5=table, 6=userdata)
    uint32_t         pad_0C;      // +0x0C
    LuaStateWrapper* stateWrap;   // +0x10
    void*            allocator;   // +0x18
    int32_t          luaRefId;    // +0x20
    int32_t          keyId;       // +0x24
    uint32_t         storageKind; // +0x28 (0=direct, 1=tableSlot, 2=registryRef)
    int32_t          subIndex;    // +0x2C
};

// 0x026a8090 (295 bytes)
// Commits the value on top of the Lua stack into the target location described by `self`.
void LuaTableValueProxy_CommitTopOfStack(LuaTableValueProxy* self) {
    void* L = self->stateWrap->L;
    if (self->storageKind == 2) {
        lua_rawgeti(L, -10000, self->luaRefId);
        lua_pushvalue(L, -2);
        lua_settable(L, -10001);
    } else if (self->storageKind == 1) {
        uint8_t keyBuf[128];
        lua_getUserKey(L, self->keyId, keyBuf);
        lua_getUserKeyTable(L, reinterpret_cast<const char*>(GX_VMA(0x486f5b5ULL)), keyBuf);
        lua_pushvalue(L, -2);
        lua_rawseti(L, -2, self->subIndex);
        lua_settop(L, -2);
    } else if (self->storageKind == 0) {
        uint8_t keyBuf[128];
        lua_getUserKey(L, self->keyId, keyBuf);
        lua_setUserKeyVal(L, keyBuf, self->subIndex);
    }
}

static inline void UpdateProxyValueTypeFromStack(LuaTableValueProxy* self) {
    void* L = self->stateWrap->L;
    int32_t t = lua_type(L, -1);
    switch (t) {
        case 1:  self->valueType = 3; break;
        case 2:
        case 7:  self->valueType = 6; break;
        case 3:  self->valueType = 3; break;
        case 4:  self->valueType = 4; break;
        case 5:  self->valueType = 5; break;
        default: break;
    }
    lua_settop(L, -2);
}

// 0x026a81c0 (144 bytes)
void LuaTableValueProxy_RefreshValueType(LuaTableValueProxy* self) {
    LuaProxy_PushOnStack(self);
    UpdateProxyValueTypeFromStack(self);
}

// 0x026a8250 (188 bytes)
void LuaTableValueProxy_ConstructDirect(
    LuaTableValueProxy* self,
    LuaStateWrapper* stateWrap,
    void* alloc,
    int32_t keyId,
    uint32_t storageKind,
    int32_t subIndex)
{
    self->valueType   = 0;
    self->vtable      = reinterpret_cast<void*>(GX_VMA(0x52b9eb0ULL));
    self->stateWrap   = stateWrap;
    self->allocator   = alloc;
    self->luaRefId    = 0;
    self->keyId       = keyId;
    self->storageKind = storageKind;
    self->subIndex    = subIndex;
    LuaTableValueProxy_RefreshValueType(self);
}

// 0x026a8310 (208 bytes)
void LuaTableValueProxy_ConstructRegistryRef(
    LuaTableValueProxy* self,
    LuaStateWrapper* stateWrap,
    void* alloc,
    int32_t keyId)
{
    self->valueType   = 0;
    self->vtable      = reinterpret_cast<void*>(GX_VMA(0x52b9eb0ULL));
    self->stateWrap   = stateWrap;
    self->allocator   = alloc;
    self->keyId       = keyId;
    self->storageKind = 2;
    self->luaRefId    = luaL_ref(stateWrap->L, -10000);
    self->subIndex    = 0;
    LuaTableValueProxy_RefreshValueType(self);
}

// 0x026a83e0 (252 bytes)
void LuaTableValueProxy_CopyConstruct(LuaTableValueProxy* self, const LuaTableValueProxy* src) {
    self->valueType   = 0;
    self->vtable      = reinterpret_cast<void*>(GX_VMA(0x52b9eb0ULL));
    self->stateWrap   = src->stateWrap;
    self->allocator   = src->allocator;
    self->keyId       = src->keyId;
    self->storageKind = src->storageKind;
    self->subIndex    = src->subIndex;
    if (src->storageKind == 2) {
        void* L = self->stateWrap->L;
        lua_rawgeti(L, -10000, src->luaRefId);
        self->luaRefId = luaL_ref(L, -10000);
    } else {
        self->luaRefId = 0;
    }
    LuaTableValueProxy_RefreshValueType(self);
}

extern "C" void operator_delete_dl(void* ptr); // 0x02bbe588

// 0x026a84e0 (64 bytes)
void LuaTableValueProxy_DeletingDtor(LuaTableValueProxy* self) {
    self->vtable = reinterpret_cast<void*>(GX_VMA(0x52b9eb0ULL));
    if (self->storageKind == 2) {
        luaL_unref(self->stateWrap->L, -10000, self->luaRefId);
    }
    operator_delete_dl(self);
}

// 0x026a8520 (45 bytes)
void LuaTableValueProxy_Destruct(LuaTableValueProxy* self) {
    self->vtable = reinterpret_cast<void*>(GX_VMA(0x52b9eb0ULL));
    if (self->storageKind == 2) {
        luaL_unref(self->stateWrap->L, -10000, self->luaRefId);
    }
}

// 0x026a8550 (65 bytes)
int32_t LuaTableValueProxy_ToInt32(LuaTableValueProxy* self) {
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    double val = lua_tonumber(L, -1);
    lua_settop(L, -2);
    return static_cast<int32_t>(val);
}

// 0x026a85a0 (65 bytes)
double LuaTableValueProxy_ToDouble(LuaTableValueProxy* self) {
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    double val = lua_tonumber(L, -1);
    lua_settop(L, -2);
    return val;
}

// 0x026a85f0 (60 bytes)
const char* LuaTableValueProxy_ToString(LuaTableValueProxy* self) {
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    const char* s = lua_tostring(L, -1);
    lua_settop(L, -2);
    return s;
}

// 0x026a8630 (144 bytes)
void* LuaTableValueProxy_ToUserDataPair(uint64_t* outPair, LuaTableValueProxy* self) {
    if (self->valueType != 6) {
        if (self->valueType == 0) {
            outPair[1] = 0;
            outPair[0] = 0;
            return outPair;
        }
        reinterpret_cast<void(*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
            reinterpret_cast<const char*>(GX_VMA(0x486f5b7ULL)), 0xFBu,
            reinterpret_cast<const char*>(GX_VMA(0x486f62fULL)));
    }
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    uint64_t* ud = static_cast<uint64_t*>(lua_touserdata(L, -1));
    lua_settop(L, -2);
    if (!ud) {
        outPair[0] = 0;
        outPair[1] = 0;
        return outPair;
    }
    outPair[0] = ud[0];
    outPair[1] = ud[1];
    return outPair;
}

// 0x026a86c0 (40 bytes)
void LuaTableValueProxy_SetInt32(LuaTableValueProxy* self, int32_t v) {
    lua_pushnumber(self->stateWrap->L, static_cast<double>(v));
    LuaTableValueProxy_CommitTopOfStack(self);
}

// 0x026a86f0 (36 bytes)
void LuaTableValueProxy_SetDouble(LuaTableValueProxy* self, double v) {
    lua_pushnumber(self->stateWrap->L, v);
    LuaTableValueProxy_CommitTopOfStack(self);
}

// 0x026a8720 (36 bytes)
void LuaTableValueProxy_SetString(LuaTableValueProxy* self, const char* s) {
    lua_pushstring(self->stateWrap->L, s);
    LuaTableValueProxy_CommitTopOfStack(self);
}

extern void LuaState_PushEmptyTable(void* L); // 0x026a56e0

// 0x026a8750 (36 bytes)
void LuaTableValueProxy_SetEmptyTable(LuaTableValueProxy* self) {
    LuaState_PushEmptyTable(self->stateWrap->L);
    LuaTableValueProxy_CommitTopOfStack(self);
}

// 0x026a8780 (183 bytes)
void LuaTableValueProxy_SetUserDataPair(LuaTableValueProxy* self, uint64_t* pairPtr) {
    void* L = self->stateWrap->L;
    uint64_t u0 = pairPtr[0];
    uint64_t u1 = pairPtr[1];
    lua_newuserdata(L, 0x10);
    uint64_t* ud = static_cast<uint64_t*>(lua_touserdata(L, -1));
    if (ud) {
        ud[0] = u0;
        ud[1] = u1;
    }
    uint64_t metaKey = (*reinterpret_cast<uint64_t(**)(uint64_t)>(*reinterpret_cast<uint64_t*>(u0) + 0x28))(u0);
    lua_pushlstring(L, &metaKey, 8);
    reinterpret_cast<void(*)(void*, int32_t)>(GX_VMA(0x21088e0ULL))(L, -10001);
    lua_setmetatable(L, -2);
    LuaTableValueProxy_CommitTopOfStack(self);
}

extern void* DLAllocator_Allocate(size_t size, size_t align, void* alloc); // 0x0207c4e0
extern void  LuaSubProxy_InitA(void* sub, void* stateWrap);                // 0x02138df0
extern void  LuaSubProxy_InitB(void* sub, void* stateWrap);                // 0x02138ed0
extern void  LuaSubProxy_InitC(void* sub, void* stateWrap);                // 0x02138ef0
extern void  LuaSubProxy_InitD(void* sub, void* stateWrap);                // 0x0213b160
extern void  DLString_EnsureCString(void* dlStr);                          // 0x02131b10
extern void  DLWideString_EnsureCString(void* dlWStr);                     // 0x02139630

static inline void LuaProxy_AssertIsTable(const LuaTableValueProxy* self, uint32_t lineNo) {
    if (self->valueType != 5) {
        reinterpret_cast<void(*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
            reinterpret_cast<const char*>(GX_VMA(0x486f5b7ULL)), lineNo,
            reinterpret_cast<const char*>(GX_VMA(0x486f65aULL)));
    }
}

// 0x026a8840 (108 bytes)
void* LuaTableValueProxy_CreateIteratorBegin(LuaTableValueProxy* self) {
    LuaProxy_AssertIsTable(self, 0x132u);
    LuaProxy_PushOnStack(self);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) {
        LuaSubProxy_InitA(mem, self->stateWrap);
    }
    return mem;
}

// 0x026a88b0 (100 bytes)
void* LuaTableValueProxy_CreateIteratorEnd(LuaTableValueProxy* self) {
    LuaProxy_AssertIsTable(self, 0x13Cu);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) {
        LuaSubProxy_InitB(mem, self->stateWrap);
    }
    return mem;
}

// 0x026a8920 (138 bytes)
void* LuaTableValueProxy_GetFieldByInt(LuaTableValueProxy* self, int32_t key) {
    LuaProxy_AssertIsTable(self, 0x149u);
    LuaProxy_PushOnStack(self);
    lua_pushnumber(self->stateWrap->L, static_cast<double>(key));
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) LuaSubProxy_InitC(mem, self->stateWrap);
    return mem;
}

// 0x026a89b0 (139 bytes)
void* LuaTableValueProxy_GetFieldByDouble(LuaTableValueProxy* self, double key) {
    LuaProxy_AssertIsTable(self, 0x154u);
    LuaProxy_PushOnStack(self);
    lua_pushnumber(self->stateWrap->L, key);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) LuaSubProxy_InitC(mem, self->stateWrap);
    return mem;
}

// 0x026a8a40 (136 bytes)
void* LuaTableValueProxy_GetFieldByCString(LuaTableValueProxy* self, const char* key) {
    LuaProxy_AssertIsTable(self, 0x15Fu);
    LuaProxy_PushOnStack(self);
    lua_pushstring(self->stateWrap->L, key);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) LuaSubProxy_InitC(mem, self->stateWrap);
    return mem;
}

// 0x026a8ad0 (272 bytes)
void* LuaTableValueProxy_GetFieldByUserData(LuaTableValueProxy* self, uint64_t* pairPtr) {
    LuaProxy_AssertIsTable(self, 0x16Bu);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    uint64_t u0 = pairPtr[0];
    uint64_t u1 = pairPtr[1];
    lua_newuserdata(L, 0x10);
    uint64_t* ud = static_cast<uint64_t*>(lua_touserdata(L, -1));
    if (ud) { ud[0] = u0; ud[1] = u1; }
    uint64_t metaKey = (*reinterpret_cast<uint64_t(**)(uint64_t)>(*reinterpret_cast<uint64_t*>(u0) + 0x28))(u0);
    lua_pushlstring(L, &metaKey, 8);
    reinterpret_cast<void(*)(void*, int32_t)>(GX_VMA(0x21088e0ULL))(L, -10001);
    lua_setmetatable(L, -2);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) LuaSubProxy_InitC(mem, self->stateWrap);
    return mem;
}

// 0x026a8be0 (128 bytes)
void* LuaTableValueProxy_GetFieldByDLString(LuaTableValueProxy* self, void* dlStr) {
    LuaProxy_AssertIsTable(self, 0x173u);
    LuaProxy_PushOnStack(self);
    DLString_EnsureCString(dlStr);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x28, 8, alloc);
    if (mem) LuaSubProxy_InitC(mem, self->stateWrap);
    return mem;
}

// 0x026a8c60 (138 bytes)
void* LuaTableValueProxy_GetConstFieldByInt(LuaTableValueProxy* self, int32_t key) {
    LuaProxy_AssertIsTable(self, 0x17Cu);
    LuaProxy_PushOnStack(self);
    lua_pushnumber(self->stateWrap->L, static_cast<double>(key));
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x20, 8, alloc);
    if (mem) LuaSubProxy_InitD(mem, self->stateWrap);
    return mem;
}

// 0x026a8cf0 (139 bytes)
void* LuaTableValueProxy_GetConstFieldByDouble(LuaTableValueProxy* self, double key) {
    LuaProxy_AssertIsTable(self, 0x187u);
    LuaProxy_PushOnStack(self);
    lua_pushnumber(self->stateWrap->L, key);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x20, 8, alloc);
    if (mem) LuaSubProxy_InitD(mem, self->stateWrap);
    return mem;
}

// 0x026a8d80 (136 bytes)
void* LuaTableValueProxy_GetConstFieldByCString(LuaTableValueProxy* self, const char* key) {
    LuaProxy_AssertIsTable(self, 0x192u);
    LuaProxy_PushOnStack(self);
    lua_pushstring(self->stateWrap->L, key);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x20, 8, alloc);
    if (mem) LuaSubProxy_InitD(mem, self->stateWrap);
    return mem;
}

// 0x026a8e10 (272 bytes)
void* LuaTableValueProxy_GetConstFieldByUserData(LuaTableValueProxy* self, uint64_t* pairPtr) {
    LuaProxy_AssertIsTable(self, 0x19Eu);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    uint64_t u0 = pairPtr[0];
    uint64_t u1 = pairPtr[1];
    lua_newuserdata(L, 0x10);
    uint64_t* ud = static_cast<uint64_t*>(lua_touserdata(L, -1));
    if (ud) { ud[0] = u0; ud[1] = u1; }
    uint64_t metaKey = (*reinterpret_cast<uint64_t(**)(uint64_t)>(*reinterpret_cast<uint64_t*>(u0) + 0x28))(u0);
    lua_pushlstring(L, &metaKey, 8);
    reinterpret_cast<void(*)(void*, int32_t)>(GX_VMA(0x21088e0ULL))(L, -10001);
    lua_setmetatable(L, -2);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x20, 8, alloc);
    if (mem) LuaSubProxy_InitD(mem, self->stateWrap);
    return mem;
}

// 0x026a8f20 (128 bytes)
void* LuaTableValueProxy_GetConstFieldByDLString(LuaTableValueProxy* self, void* dlStr) {
    LuaProxy_AssertIsTable(self, 0x1A6u);
    LuaProxy_PushOnStack(self);
    DLString_EnsureCString(dlStr);
    void* alloc = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self->stateWrap->allocOwner) + 8);
    void* mem = DLAllocator_Allocate(0x20, 8, alloc);
    if (mem) LuaSubProxy_InitD(mem, self->stateWrap);
    return mem;
}

// 0x026a8fa0 (147 bytes)
void LuaTableValueProxy_DeleteFieldByInt(LuaTableValueProxy* self, int32_t key) {
    LuaProxy_AssertIsTable(self, 0x1B2u);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    int32_t top = lua_gettop(L);
    lua_pushnumber(L, static_cast<double>(key));
    lua_pushnil(L);
    lua_settable(L, top);
    lua_settop(L, -2);
}

// 0x026a9040 (148 bytes)
void LuaTableValueProxy_DeleteFieldByDouble(LuaTableValueProxy* self, double key) {
    LuaProxy_AssertIsTable(self, 0x1BFu);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    int32_t top = lua_gettop(L);
    lua_pushnumber(L, key);
    lua_pushnil(L);
    lua_settable(L, top);
    lua_settop(L, -2);
}

// 0x026a90e0 (145 bytes)
void LuaTableValueProxy_DeleteFieldByCString(LuaTableValueProxy* self, const char* key) {
    LuaProxy_AssertIsTable(self, 0x1CCu);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    int32_t top = lua_gettop(L);
    lua_pushstring(L, key);
    lua_pushnil(L);
    lua_settable(L, top);
    lua_settop(L, -2);
}

// 0x026a9180 (290 bytes)
void LuaTableValueProxy_DeleteFieldByUserData(LuaTableValueProxy* self, uint64_t* pairPtr) {
    LuaProxy_AssertIsTable(self, 0x1DAu);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    int32_t top = lua_gettop(L);
    uint64_t u0 = pairPtr[0];
    uint64_t u1 = pairPtr[1];
    lua_newuserdata(L, 0x10);
    uint64_t* ud = static_cast<uint64_t*>(lua_touserdata(L, -1));
    if (ud) { ud[0] = u0; ud[1] = u1; }
    uint64_t metaKey = (*reinterpret_cast<uint64_t(**)(uint64_t)>(*reinterpret_cast<uint64_t*>(u0) + 0x28))(u0);
    lua_pushlstring(L, &metaKey, 8);
    reinterpret_cast<void(*)(void*, int32_t)>(GX_VMA(0x21088e0ULL))(L, -10001);
    lua_setmetatable(L, -2);
    lua_pushnil(L);
    lua_settable(L, top);
    lua_settop(L, -2);
}

// 0x026a92b0 (137 bytes)
void LuaTableValueProxy_DeleteFieldByDLString(LuaTableValueProxy* self, void* dlStr) {
    LuaProxy_AssertIsTable(self, 0x1E4u);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    int32_t top = lua_gettop(L);
    DLString_EnsureCString(dlStr);
    lua_pushnil(L);
    lua_settable(L, top);
    lua_settop(L, -2);
}

// 0x026a9340 (137 bytes)
void LuaTableValueProxy_DeleteFieldByDLWideString(LuaTableValueProxy* self, void* dlWStr) {
    LuaProxy_AssertIsTable(self, 0x1F0u);
    LuaProxy_PushOnStack(self);
    void* L = self->stateWrap->L;
    int32_t top = lua_gettop(L);
    DLWideString_EnsureCString(dlWStr);
    lua_pushnil(L);
    lua_settable(L, top);
    lua_settop(L, -2);
}

// 0x026a93d0 (3 bytes)
int32_t LuaTableValueProxy_ReturnZero(LuaTableValueProxy* /*self*/) {
    return 0;
}

// 0x026a93e0 (58 bytes)
LuaTableValueProxy* LuaTableValueProxy_Clone(const LuaTableValueProxy* self) {
    LuaTableValueProxy* copy = static_cast<LuaTableValueProxy*>(
        DLAllocator_Allocate(0x30, 8, self->allocator));
    if (!copy) {
        return nullptr;
    }
    LuaTableValueProxy_CopyConstruct(copy, self);
    return copy;
}

} // namespace CoreGraphics2
} // namespace from
