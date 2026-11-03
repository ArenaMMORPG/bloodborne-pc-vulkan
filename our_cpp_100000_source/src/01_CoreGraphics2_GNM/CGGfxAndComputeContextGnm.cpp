// ============================================================================
// Bloodborne (CUSA03173 v1.09) - CoreGraphics2 GNM Backend
// File: CGGfxAndComputeContextGnm.cpp
// Address Range: 0x026ae700 .. 0x026b2500 (44 raw functions + 0x026afd30 ref)
// Subsystem:
//   1. CGGpuCopyGnm & CGEmbeddedShadersGnm (0x26b1c40 .. 0x26b2500)
//      - CPU->GPU scratch-ring + Compute Shader / CP DMA upload helpers
//      - Reference-counted initialization/teardown of 13 built-in GNM utility
//        shaders (buffer copy CS, 1D/2D/3D copy/upload CS, clear CS, fullscreen
//        quad VS, copy PS, MSAA resolve PS)
//   2. CGComputeContextGnm & CGComputeQueueDingDongGnm (0x26b0f10 .. 0x26b1b90)
//      - Async Compute queue (`sce::Gnmx::ComputeContext`) lifecycle, DingDong
//        ring management, DCB out-of-space callbacks, scratch allocation, and
//        releaseMem fence synchronization
//   3. CGGfxContextGnm & CGResourceStagingHashMapGnm (0x26ae700 .. 0x26b0e30)
//      - Master 0xb730-byte Graphics Command Context wrapping `sce::Gnmx::GfxContext`,
//        immediate & deferred DCB/CCB chunk chains, staging upload hash map,
//        shader cache flushes, and `sce::Gnm::submitCommandBuffers` /
//        `submitAndFlipCommandBuffers` dispatchers
// ============================================================================

#include <cstdint>
#include <cstring>
#include "gx_direct_vulkan.h"

namespace from {
namespace CoreGraphics2 {

// Forward declarations of external Orbis / GNM / Gnmx SDK routines
extern "C" {
    void sceGnmFlushGarlic();                                               // 0x031bf7cc
    int32_t sceGnmSubmitCommandBuffers(uint32_t count,
                                       void* dcbGpuAddrs[],
                                       uint32_t* dcbSizesInBytes,
                                       void* ccbGpuAddrs[],
                                       uint32_t* ccbSizesInBytes);          // 0x031bf83c
    int32_t sceGnmSubmitAndFlipCommandBuffers(uint32_t count,
                                              void* dcbGpuAddrs[],
                                              uint32_t* dcbSizesInBytes,
                                              void* ccbGpuAddrs[],
                                              uint32_t* ccbSizesInBytes,
                                              int32_t videoOutHandle,
                                              int32_t displayBufferIndex,
                                              int32_t flipMode,
                                              int64_t flipArg);             // 0x031bf6ec
    int32_t sceKernelUsleep(uint32_t microseconds);                         // 0x031bfc1c

    void Gnmx_GfxContext_init(void* gfxCtx, void* cueHeap, uint32_t numRingEntries,
                              void* dcbBuf, uint32_t dcbSize,
                              void* ccbBuf, uint32_t ccbSize);              // 0x01072c50
    void Gnmx_GfxContext_reset(void* gfxCtx);                               // 0x01072ce0
    int32_t Gnmx_GfxContext_submit(void* gfxCtx);                           // 0x01072db0
    int32_t Gnmx_GfxContext_submitAndFlip(void* gfxCtx, int32_t videoOutHandle,
                                          int32_t rtIndex, uint32_t flipMode,
                                          int64_t flipArg, void* labelAddr,
                                          uint32_t labelVal);               // 0x01072ef0
    void Gnmx_GfxContext_writeImmediateAtEndOfPipeWithInterrupt(
        void* dcb, uint32_t eventType, void* dstGpuAddr, uint64_t value);   // 0x01060720
    void Gnmx_GfxContext_writeImmediateDwordAtEndOfPipe(
        void* dcb, uint32_t eventType, void* dstGpuAddr, uint64_t value);   // 0x010606b0
    void Gnmx_GfxContext_waitOnAddress(
        void* dcb, void* gpuAddr, uint32_t mask, uint32_t cmpFunc,
        uint32_t refValue);                                                 // 0x010607e0
    void Gnmx_GfxContext_triggerEvent(void* dcb, uint32_t eventType);       // 0x010604c0
    void Gnmx_GfxContext_setupScreenViewport(
        void* gfxCtx, uint32_t left, uint32_t top,
        uint32_t right, uint32_t bottom, float zScale, float zOffset);      // 0x01074280
    void* Gnmx_GfxContext_allocateFromCommandBuffer(
        void* gfxCtx, uint32_t sizeInBytes, uint32_t alignmentPowerOfTwo);  // 0x01060fb0
    void Gnmx_GfxContext_cpRamDumpOrDmaData(
        void* gfxCtx, void* dstGpuAddr, const void* srcGpuAddr,
        uint32_t numBytes, uint32_t isBlocking);                            // 0x01073e60

    void Gnmx_ComputeContext_init(void* cmpCtx, void* dcbBuffer,
                                  uint32_t dcbSizeInBytes);                 // 0x010772e0
    void Gnmx_ComputeContext_reset(void* cmpCtx);                           // 0x01077360
    void* Gnmx_ComputeContext_allocateFromCommandBuffer(
        void* dcb, uint32_t sizeInBytes, uint32_t alignmentPowerOfTwo);     // 0x0105cc20
    void Gnmx_ComputeContext_writeReleaseMemEvent(
        void* dcb, uint32_t eventType, uint32_t dstSelector, void* dstGpuAddr,
        uint64_t immValue, uint32_t dataSelector, uint32_t intSelector);    // 0x0105c770
    void Gnmx_ComputeContext_waitMem(
        void* dcb, void* gpuAddr, uint32_t mask, uint32_t cmpFunc,
        uint32_t refValue);                                                 // 0x0105c830

    void Gnmx_ComputeQueue_initialize(
        void* computeQueue, uint32_t vqueueId, uint32_t pipeId,
        uint32_t queueId);                                                  // 0x01075bd0
    int32_t Gnmx_ComputeQueue_map(
        void* computeQueue, void* ringBaseAddr, uint32_t ringSizeDW,
        void* readPtrAddr);                                                 // 0x01075c30
    void Gnmx_ComputeQueue_unmap(void* computeQueue);                       // 0x01075d10
    int32_t Gnmx_ComputeQueue_submit(void* computeQueue, void* cmpCtx);     // 0x01075db0
}

// Forward declarations of internal subsystem helpers
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

struct CGResourceStagingHashMapGnm {
    IAllocator* allocator;        // +0x00
    void***     chunkTable;       // +0x08 (array of pointers to 128-entry (0x800-byte) chunks)
    uint32_t    entryCount;       // +0x10
    uint32_t    pad_14;           // +0x14
    uint16_t    bucketCount;      // +0x18 (0x100)
    uint16_t    bucketHeads[256]; // +0x1a (1-based entry indices, 0 = empty)
};

// ============================================================================
// 1. CGResourceStagingHashMapGnm & Staging Pool Helpers (0x26ae700 .. 0x26ae8ff)
// ============================================================================

// 0x026ae700 (226 bytes)
// Suballocates a 16-byte {keyPtr, stagingDescPtr} node from the 128-entry
// segmented array in `CGResourceStagingHashMapGnm`.
void* CGResourceStagingHashMapGnm_AllocateEntry(CGResourceStagingHashMapGnm* map) {
    uint32_t idx = map->entryCount;
    uint32_t chunkIdx = idx >> 7;
    uint32_t slotIdx  = idx & 0x7F;
    void*** chunkTable = map->chunkTable;

    if (slotIdx == 0) {
        uint32_t numChunks = (idx + 0x7F) >> 7;
        void* newChunk = map->allocator->vtable->allocateAligned(map->allocator, 0x800, 8);
        if (!newChunk) {
            return nullptr;
        }
        void*** newTable = static_cast<void***>(
            map->allocator->vtable->allocateAligned(map->allocator, (numChunks * 8) + 0x10, 8));
        if (!newTable) {
            map->allocator->vtable->deallocate(map->allocator, newChunk);
            return nullptr;
        }
        newTable[numChunks]     = static_cast<void**>(newChunk);
        newTable[numChunks + 1] = nullptr;
        if (map->chunkTable) {
            std::memcpy(newTable, map->chunkTable, numChunks * sizeof(void*));
            map->allocator->vtable->deallocate(map->allocator, map->chunkTable);
        }
        map->chunkTable = newTable;
        chunkTable = newTable;
        idx = map->entryCount;
    }

    map->entryCount = idx + 1;
    uint8_t* chunkBase = reinterpret_cast<uint8_t*>(chunkTable[chunkIdx]);
    return chunkBase + (slotIdx << 4);
}

extern void* StagingPool_Suballocate64K(void* pool, void* ctxRef, uint32_t alignmentShift); // 0x026addc0

// 0x026ae7f0 (71 bytes)
// Wrapper that suballocates from the context's 64KB staging pool at +0xb3e0.
void* CGGfxContextGnm_AllocateStagingChunk64K(
    void* /*unusedRdi*/, uint8_t* gfxCtx, uint32_t alignmentShift)
    __asm__("_ZN4from13CoreGraphics239CGGfxContextGnm_AllocateStagingChunk64KEPvPhj");
void* CGGfxContextGnm_AllocateStagingChunk64K(
    void* /*unusedRdi*/, uint8_t* gfxCtx, uint32_t alignmentShift)
{
    struct CtxRef {
        void*   ctxInner;
        uint8_t isCompute;
    } ref{ gfxCtx + 0x10, 0 };
    return StagingPool_Suballocate64K(gfxCtx + 0xb3e0, &ref, alignmentShift);
}

// 0x026ae840 (192 bytes)
// Looks up `resourceKey` in `CGResourceStagingHashMapGnm`. If found, returns
// the staged shadow buffer pointer at `entry[1]`; otherwise returns `resourceKey[1]`.
void* CGResourceStagingHashMapGnm_FindStagingBuffer(
    CGResourceStagingHashMapGnm* map,
    void** resourceKey)
{
    uint16_t slot = map->bucketCount;
    if (slot <= 0xFu) {
        void*** chunkTable = map->chunkTable;
        if (chunkTable) {
            uint16_t* slotArray = static_cast<uint16_t*>(resourceKey[7]);
            uint32_t entryIdx = static_cast<uint32_t>(slotArray[slot]) - 1u;
            if (map->entryCount > entryIdx) {
                void** chunk = chunkTable[entryIdx >> 7];
                void** entry = reinterpret_cast<void**>(
                    reinterpret_cast<uint8_t*>(chunk) + ((entryIdx & 0x7Fu) << 4));
                if (entry && entry[0] == resourceKey) {
                    return entry[1];
                }
            }
        }
        return resourceKey[1];
    }

    void*** chunkTable = map->chunkTable;
    uint32_t count = map->entryCount;
    uint32_t remInLast = (count != 0) ? (((count + 0x7Fu) & 0x7Fu) + 1u) : 0u;
    if (!chunkTable) {
        return resourceKey[1];
    }

    for (void*** chunkIter = chunkTable; *chunkIter != nullptr; ++chunkIter) {
        uint32_t limit = (chunkIter[1] != nullptr) ? 0x80u : remInLast;
        void** entry = *chunkIter;
        for (uint32_t i = 0; i < limit; ++i, entry += 2) {
            if (entry[0] == resourceKey) {
                return entry[1];
            }
        }
    }
    return resourceKey[1];
}

// ============================================================================
// 2. CGGfxContextGnm Master Graphics Command Context (0x26ae9d0 .. 0x26b0e30)
// ============================================================================

extern void CGResourceStagingHashMapGnm_Construct(void* map);                        // 0x026ada80
extern void CGResourceStagingHashMapGnm_CopyConstruct(void* dst, const void* src);   // 0x026adaa0
extern void CGResourceStagingHashMapGnm_FreeAll(void* map);                          // 0x026adac0
extern void CGShaderResourceBinderGnm_Construct(void* binder);                       // 0x026bbbd0
extern void CGShaderResourceBinderGnm_CopyConstruct(void* dst, const void* src);     // 0x026bbcb0
extern void CGShaderResourceBinderGnm_Destroy(void* binder);                         // 0x026bbd50
extern void CGShaderResourceBinderGnm_Initialize(void* binder, void* alloc,
                                                 void* gfxCtx);                      // 0x026bbd80
extern void CGShaderResourceBinderGnm_ResetAndAllocateRootChunk(void* binder);       // 0x026bbe10
extern void CGRasterizerCacheGnm_Construct(void* cache);                             // 0x026d0bb0
extern void CGRasterizerCacheGnm_CopyConstruct(void* dst, const void* src);          // 0x026d0be0
extern void CGOutputMergerCacheGnm_Construct(void* cache);                           // 0x026d1600
extern void CGOutputMergerCacheGnm_CopyConstruct(void* dst, const void* src);        // 0x026d1660
extern void CGIndexBufferCacheGnm_Construct(void* cache);                            // 0x026bba00
extern void CGIndexBufferCacheGnm_CopyConstruct(void* dst, const void* src);         // 0x026bba20
extern void CGSurfaceSyncTrackerGnm_Construct(void* tracker);                        // 0x026b2d30
extern void CGSurfaceSyncTrackerGnm_CopyConstruct(void* dst, const void* src);       // 0x026b2e90

// 0x026ae9d0 (211 bytes)
// Default constructor of `CGGfxContextGnm` (inner context at wrapper + 0x10, size 0xb4f8).
void CGGfxContextGnm_Construct(uint8_t* self) {
    self[0x28] &= 0xFCu;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x26bbba0ULL))(self + 0x40);
    CGResourceStagingHashMapGnm_Construct(self + 0xb3b0);
    std::memset(self + 0xb3d0, 0, 0x94);
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x26b8260ULL))(self + 0xb468);
    self[0xb4f0] = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4e8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4e0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4d8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4d0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4c8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4c0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4b0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4a8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4a0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb498) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb490) = 0;
}

// 0x026aeab0 (396 bytes)
// Copy constructor of `CGGfxContextGnm`.
void CGGfxContextGnm_CopyConstruct(uint8_t* dst, const uint8_t* src) {
    std::memcpy(dst, src, 0x40);
    std::memcpy(dst + 0x40, src + 0x40, 0xb370);
    CGResourceStagingHashMapGnm_CopyConstruct(dst + 0xb3b0, src + 0xb3b0);
    std::memcpy(dst + 0xb3d0, src + 0xb3d0, 0x98);
    std::memcpy(dst + 0xb468, src + 0xb468, 0x28);
    std::memcpy(dst + 0xb490, src + 0xb490, 0x60);
    dst[0xb4f0] = src[0xb4f0];
    *reinterpret_cast<uint8_t**>(dst + 0x20) = dst;
}

// 0x026aec40 (468 bytes)
// Releases all dynamic DCB/CCB chunks, staging pools, and CUE heaps owned by `CGGfxContextGnm`.
void CGGfxContextGnm_Finalize(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics224CGGfxContextGnm_FinalizeEPh");
void CGGfxContextGnm_Finalize(uint8_t* self) {
    if (*reinterpret_cast<IAllocator**>(self + 0xb4d0) != nullptr) {
        uint32_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
        while (node) {
            uint8_t* next = *reinterpret_cast<uint8_t**>(node + static_cast<size_t>(cap) * 8);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
            alloc->vtable->deallocate(alloc, node);
            node = next;
        }
        *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb4e0) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb4d8) = 0;
    }

    if (*reinterpret_cast<IAllocator**>(self + 0xb498) != nullptr) {
        size_t nextOff = static_cast<size_t>(*reinterpret_cast<uint32_t*>(self + 0xb4b0)) << 6;
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xb4a0);
        while (node) {
            uint8_t* next = *reinterpret_cast<uint8_t**>(node + nextOff);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            alloc->vtable->deallocate(alloc, node);
            node = next;
        }
        *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb4a8) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb4a0) = 0;
    }

    reinterpret_cast<void(*)(void*)>(GX_VMA(0x26b82a0ULL))(self + 0xb468);

    if (*reinterpret_cast<void**>(self + 0xb3d0) != nullptr) {
        uint32_t mask = *reinterpret_cast<uint32_t*>(self + 0xb460);
        while (mask != 0) {
            uint32_t bit = static_cast<uint32_t>(__builtin_ctz(mask));
            uint8_t* blk = *reinterpret_cast<uint8_t**>(self + 0xb3d8 + static_cast<size_t>(bit) * 8);
            while (blk) {
                uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
                reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26aa2e0ULL))(
                    *reinterpret_cast<void**>(self + 0xb3d0), blk);
                blk = nextBlk;
            }
            *reinterpret_cast<uint64_t*>(self + 0xb3d8 + static_cast<size_t>(bit) * 8) = 0;
            mask -= (1u << bit);
        }
        *reinterpret_cast<uint32_t*>(self + 0xb460) = 0;

        uint8_t* blk = *reinterpret_cast<uint8_t**>(self + 0xb458);
        while (blk) {
            uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
            reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26aa2e0ULL))(
                *reinterpret_cast<void**>(self + 0xb3d0), blk);
            blk = nextBlk;
        }
        *reinterpret_cast<uint64_t*>(self + 0xb458) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xb3d0) = 0;
    }

    CGResourceStagingHashMapGnm_FreeAll(self + 0xb3b0);
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x26bbe00ULL))(self + 0x40);
}

extern int32_t CGResourceStagingHashMapGnm_Initialize(void* map, void* alloc, uint16_t buckets); // 0x026adb50
bool CGGfxContextGnm_ImmediateDcbFullCallback(uint8_t* dcb, uint32_t sizeInDwords, uint8_t* self)
    __asm__("_ZN4from13CoreGraphics240CGGfxContextGnm_ImmediateDcbFullCallbackEPvjS1_");
bool CGGfxContextGnm_RecordImmediateCommandChunk(uint8_t* dcb, uint32_t sizeInDwords, uint8_t* self)
    __asm__("_ZN4from13CoreGraphics243CGGfxContextGnm_RecordImmediateCommandChunkEPh");
bool CGGfxContextGnm_DeferredDcbFullCallback(uint8_t* dcb, uint32_t sizeInDwords, uint8_t* self)
    __asm__("_ZN4from13CoreGraphics239CGGfxContextGnm_DeferredDcbFullCallbackEPvjS1_");
void CGGfxContextGnm_ResetCommandChunks(uint8_t* self); // 0x026af3f0
void CGGfxContextGnm_SubmitDcbPair(uint8_t* self, uint8_t* dcb0, uint8_t* dcb1, uint8_t flag)
    __asm__("_ZN4from13CoreGraphics229CGGfxContextGnm_SubmitDcbPairEPhPvjS2_jS2_jS2_j");

// 0x026aee20 (528 bytes)
// Initializes `CGGfxContextGnm`.
int32_t CGGfxContextGnm_Initialize(
    uint8_t* self,
    int32_t isDeferred,
    uint8_t* labelPool,
    void* freeListAlloc,
    IAllocator* cpuAlloc,
    void* binderAlloc,
    uint32_t binderCap,
    uint8_t flag,
    uint16_t hashBuckets)
    __asm__("_ZN4from13CoreGraphics226CGGfxContextGnm_InitializeEPhPPNS0_10IAllocatorEh");
int32_t CGGfxContextGnm_Initialize(
    uint8_t* self,
    int32_t isDeferred,
    uint8_t* labelPool,
    void* freeListAlloc,
    IAllocator* cpuAlloc,
    void* binderAlloc,
    uint32_t binderCap,
    uint8_t flag,
    uint16_t hashBuckets)
{
    uint64_t poolQwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    *reinterpret_cast<uint64_t*>(self + 0xb490) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = 0;
    *reinterpret_cast<IAllocator**>(self + 0xb4c0) = cpuAlloc;
    *reinterpret_cast<uint8_t**>(self + 0xb4c8) = labelPool;
    self[0xb4f0] = flag;

    *reinterpret_cast<IAllocator**>(self + 0xb498) = cpuAlloc;
    *reinterpret_cast<uint64_t*>(self + 0xb4a8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4a0) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb4b0) = 8;
    *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;

    *reinterpret_cast<IAllocator**>(self + 0xb4d0) = cpuAlloc;
    *reinterpret_cast<uint64_t*>(self + 0xb4e0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4d8) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb4e8) = 8;
    *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;

    if (isDeferred != 0) {
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
            gx_direct_vk::InitDrawCommandBuffer(
                self, nullptr, 0, reinterpret_cast<void*>(GX_VMA(0x26af230ULL)), self);
        } else {
            reinterpret_cast<void(*)(void*, void*, uint32_t, void*, void*)>(GX_VMA(0x1071270ULL))(
                self, nullptr, 0, reinterpret_cast<void*>(GX_VMA(0x26af230ULL)), self);
        }
        *reinterpret_cast<uint64_t*>(self + 0xb4b8) = *reinterpret_cast<uint64_t*>(self + 0x00);
    } else {
        void* dcbBuf = nullptr;
        int32_t poolErr = reinterpret_cast<int32_t(*)(void*, void**)>(GX_VMA(0x26d3b80ULL))(
            labelPool, &dcbBuf);
        if (poolErr != 0) {
            return 9;
        }
        uint32_t dcbBytes = static_cast<uint32_t>((poolQwords << 3) - 8);
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
            gx_direct_vk::InitDrawCommandBuffer(
                self, dcbBuf, dcbBytes, reinterpret_cast<void*>(GX_VMA(0x26af030ULL)), self);
        } else {
            reinterpret_cast<void(*)(void*, void*, uint32_t, void*, void*)>(GX_VMA(0x1071270ULL))(
                self, dcbBuf, dcbBytes, reinterpret_cast<void*>(GX_VMA(0x26af030ULL)), self);
        }
        *reinterpret_cast<uint64_t*>(self + 0xb4b8) = *reinterpret_cast<uint64_t*>(self + 0x00);
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x26bb6c0ULL))(self);
    }

    int32_t binderErr = reinterpret_cast<int32_t(*)(void*, void*, uint32_t)>(GX_VMA(0x26bbe10ULL))(
        self + 0x40, binderAlloc, binderCap);
    if (binderErr != 0) {
        CGGfxContextGnm_ResetCommandChunks(self);
        return binderErr;
    }

    int32_t mapErr = CGResourceStagingHashMapGnm_Initialize(self + 0xb3b0, cpuAlloc, hashBuckets);
    if (mapErr != 0) {
        CGGfxContextGnm_ResetCommandChunks(self);
        return mapErr;
    }

    uint8_t tempPool[0x94]{};
    *reinterpret_cast<void**>(tempPool) = freeListAlloc;
    std::memcpy(self + 0xb3d0, tempPool, 0x94);
    return 0;
}

// 0x026af030 (509 bytes)
// Out-of-space callback for immediate `Gnmx::GfxContext` DCB ring.
bool CGGfxContextGnm_ImmediateDcbFullCallback(
    uint8_t* dcb,
    uint32_t /*sizeInDwords*/,
    uint8_t* self)
{
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0xb4c8);
    void* curSlot = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d3d40ULL))(
        labelPool, *reinterpret_cast<void**>(dcb + 0x00));
    uint64_t poolQwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    void* newDcbBuf = nullptr;
    while (reinterpret_cast<int32_t(*)(void*, void**)>(GX_VMA(0x26d3b80ULL))(
               labelPool, &newDcbBuf) != 0) {
        reinterpret_cast<void(*)()>(GX_VMA(0x207f970ULL))();
    }

    uint64_t dcbWords = ((poolQwords << 3) - 8ULL) >> 2;
    uint32_t cap      = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
    size_t   allocSz  = static_cast<size_t>(cap) * 8ULL + 8ULL;

    uint8_t* chunkNode = *reinterpret_cast<uint8_t**>(self + 0xb4e0);
    uint32_t idx       = *reinterpret_cast<uint32_t*>(self + 0xb4ec);
    if (*reinterpret_cast<void**>(self + 0xb4d8) == nullptr) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
        chunkNode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, allocSz, 8));
        if (!chunkNode) {
            return false;
        }
        *reinterpret_cast<void**>(chunkNode + static_cast<size_t>(cap) * 8ULL) = nullptr;
        *reinterpret_cast<uint8_t**>(self + 0xb4d8) = chunkNode;
        *reinterpret_cast<uint8_t**>(self + 0xb4e0) = chunkNode;
        *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
        idx = 0;
    } else if (idx == cap) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
        uint8_t* newChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, allocSz, 8));
        if (!newChunk) {
            return false;
        }
        *reinterpret_cast<void**>(newChunk + static_cast<size_t>(cap) * 8ULL) = nullptr;
        *reinterpret_cast<uint8_t**>(chunkNode + static_cast<size_t>(cap) * 8ULL) = newChunk;
        *reinterpret_cast<uint8_t**>(self + 0xb4e0) = newChunk;
        *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
        chunkNode = newChunk;
        idx = 0;
    }

    *reinterpret_cast<uint32_t*>(self + 0xb4ec) = idx + 1u;
    void** slotPtr = reinterpret_cast<void**>(chunkNode + static_cast<size_t>(idx) * 8ULL);
    if (!slotPtr) {
        return false;
    }
    *slotPtr = curSlot;

    uint8_t oldDcb[0x40];
    std::memcpy(oldDcb, dcb, 0x40);
    *reinterpret_cast<void**>(dcb + 0x00) = newDcbBuf;
    *reinterpret_cast<void**>(dcb + 0x10) = newDcbBuf;
    *reinterpret_cast<void**>(dcb + 0x08) = static_cast<uint8_t*>(newDcbBuf) + (dcbWords << 2);

    CGGfxContextGnm_SubmitDcbPair(self, dcb, oldDcb, self[0xb4f0]);
    void* newBase = *reinterpret_cast<void**>(dcb + 0x10);
    *reinterpret_cast<void**>(dcb + 0x00)   = newBase;
    *reinterpret_cast<void**>(self + 0xb4b8) = newBase;
    return true;
}

// 0x026af230 (448 bytes)
// Out-of-space callback for recording command chunks into `self + 0xb498..0xb4b4`.
bool CGGfxContextGnm_RecordImmediateCommandChunk(
    uint8_t* dcb,
    uint32_t /*sizeInDwords*/,
    uint8_t* self)
{
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0xb4c8);
    uint64_t poolQwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    void* newDcbBuf = nullptr;
    if (reinterpret_cast<int32_t(*)(void*, void**)>(GX_VMA(0x26d3b80ULL))(
            labelPool, &newDcbBuf) != 0) {
        return false;
    }

    uint64_t dcbWords = ((poolQwords << 3) - 8ULL) >> 2;
    uintptr_t curBase = *reinterpret_cast<uintptr_t*>(self + 0x00);
    uintptr_t curPtr  = *reinterpret_cast<uintptr_t*>(self + 0x10);
    uintptr_t fullTop = curBase ? (curBase + (*reinterpret_cast<uint64_t*>(labelPool + 0x08) << 3) - 8ULL) : 0ULL;

    if (curPtr != curBase || *reinterpret_cast<uintptr_t*>(self + 0x08) != fullTop) {
        uint32_t cap     = *reinterpret_cast<uint32_t*>(self + 0xb4b0);
        size_t   nextOff = static_cast<size_t>(cap) << 6;
        uint8_t* chunkNode = *reinterpret_cast<uint8_t**>(self + 0xb4a8);
        uint32_t idx       = *reinterpret_cast<uint32_t*>(self + 0xb4b4);

        if (*reinterpret_cast<void**>(self + 0xb4a0) == nullptr) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            chunkNode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
            if (!chunkNode) {
                return false;
            }
            *reinterpret_cast<void**>(chunkNode + nextOff) = nullptr;
            *reinterpret_cast<uint8_t**>(self + 0xb4a0) = chunkNode;
            *reinterpret_cast<uint8_t**>(self + 0xb4a8) = chunkNode;
            *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
            idx = 0;
        } else if (idx == cap) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            uint8_t* newChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
            if (!newChunk) {
                return false;
            }
            *reinterpret_cast<void**>(newChunk + nextOff) = nullptr;
            *reinterpret_cast<uint8_t**>(chunkNode + nextOff) = newChunk;
            *reinterpret_cast<uint8_t**>(self + 0xb4a8) = newChunk;
            *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
            chunkNode = newChunk;
            idx = 0;
        }

        *reinterpret_cast<uint32_t*>(self + 0xb4b4) = idx + 1u;
        uint8_t* dstSlot = chunkNode + (static_cast<size_t>(idx) << 6);
        if (!dstSlot) {
            return false;
        }
        std::memcpy(dstSlot, self, 0x40);
    }

    *reinterpret_cast<void**>(dcb + 0x00) = newDcbBuf;
    *reinterpret_cast<void**>(dcb + 0x10) = newDcbBuf;
    *reinterpret_cast<void**>(dcb + 0x08) = static_cast<uint8_t*>(newDcbBuf) + (dcbWords << 2);
    return true;
}

extern void CGRasterizerCacheGnm_Reset(void* cache);     // 0x026d0bd0
extern void CGOutputMergerCacheGnm_Reset(void* cache);   // 0x026d1650
extern void CGIndexBufferCacheGnm_Reset(void* cache);    // 0x026bba10
extern void CGSurfaceSyncTrackerGnm_Reset(void* tracker);// 0x026b2fe0
extern void CGResourceStagingHashMapGnm_Reset(void* map);// 0x026adb80

// 0x026af3f0 (746 bytes)
// Resets `CGGfxContextGnm` at the beginning of a new frame/command list recording pass.
void CGGfxContextGnm_ResetCommandChunks(uint8_t* self) {
    uint32_t mask = *reinterpret_cast<uint32_t*>(self + 0xb460);
    while (mask != 0) {
        uint32_t bit = static_cast<uint32_t>(__builtin_ctz(mask));
        uint8_t* blk = *reinterpret_cast<uint8_t**>(self + 0xb3d8 + static_cast<size_t>(bit) * 8);
        while (blk) {
            uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
            reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26aa2e0ULL))(
                *reinterpret_cast<void**>(self + 0xb3d0), blk);
            blk = nextBlk;
        }
        *reinterpret_cast<uint64_t*>(self + 0xb3d8 + static_cast<size_t>(bit) * 8) = 0;
        mask -= (1u << bit);
    }
    *reinterpret_cast<uint32_t*>(self + 0xb460) = 0;

    uint8_t* blk = *reinterpret_cast<uint8_t**>(self + 0xb458);
    while (blk) {
        uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
        reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26aa2e0ULL))(
            *reinterpret_cast<void**>(self + 0xb3d0), blk);
        blk = nextBlk;
    }
    *reinterpret_cast<uint64_t*>(self + 0xb458) = 0;

    CGResourceStagingHashMapGnm_Reset(self + 0xb3b0);
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x26bc2d0ULL))(self + 0x40);

    void* curDcb = *reinterpret_cast<void**>(self + 0x00);
    if (curDcb) {
        void* labelPool = *reinterpret_cast<void**>(self + 0xb4c8);
        void* labelSlot = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d3d40ULL))(
            labelPool, curDcb);
        reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26d3d30ULL))(labelPool, labelSlot);
        std::memset(self, 0, 0x40);
    }

    uint8_t* chunkNode = *reinterpret_cast<uint8_t**>(self + 0xb4a0);
    if (chunkNode) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4b0);
        size_t nextOff = cap << 6;
        void* prevSlot = nullptr;
        while (chunkNode) {
            uint8_t* nextChunk = *reinterpret_cast<uint8_t**>(chunkNode + nextOff);
            uint64_t limit = nextChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xb4b4);
            uint8_t* entry = chunkNode;
            for (uint64_t i = 0; i < limit; ++i, entry += 0x40) {
                void* labelPool = *reinterpret_cast<void**>(self + 0xb4c8);
                void* slot = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d3d40ULL))(
                    labelPool, *reinterpret_cast<void**>(entry));
                if (slot != prevSlot) {
                    reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26d3d30ULL))(labelPool, slot);
                    prevSlot = slot;
                }
            }
            chunkNode = nextChunk;
        }
    }

    uint8_t* labelChunk = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
    if (labelChunk) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
        while (labelChunk) {
            uint8_t* nextLabelChunk = *reinterpret_cast<uint8_t**>(labelChunk + cap * 8);
            uint64_t limit = nextLabelChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xb4ec);
            for (uint64_t i = 0; i < limit; ++i) {
                reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26d3d30ULL))(
                    *reinterpret_cast<void**>(self + 0xb4c8),
                    *reinterpret_cast<void**>(labelChunk + i * 8));
            }
            labelChunk = nextLabelChunk;
        }
    }

    if (*reinterpret_cast<IAllocator**>(self + 0xb498) != nullptr) {
        size_t nextOff = static_cast<size_t>(*reinterpret_cast<uint32_t*>(self + 0xb4b0)) << 6;
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xb4a0);
        while (node) {
            uint8_t* next = *reinterpret_cast<uint8_t**>(node + nextOff);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            alloc->vtable->deallocate(alloc, node);
            node = next;
        }
    }

    if (*reinterpret_cast<IAllocator**>(self + 0xb4d0) != nullptr) {
        size_t nextLabelOff = static_cast<size_t>(*reinterpret_cast<uint32_t*>(self + 0xb4e8)) * 8ULL;
        uint8_t* lnode = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
        while (lnode) {
            uint8_t* nextLnode = *reinterpret_cast<uint8_t**>(lnode + nextLabelOff);
            IAllocator* lalloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
            lalloc->vtable->deallocate(lalloc, lnode);
            lnode = nextLnode;
        }
    }

    self[0xb4f0] = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb490) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4a8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4a0) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4d8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4e0) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
}

// 0x026af6e0 (20 bytes)
// Returns the usable byte capacity `(poolQwords * 8) - 8` of a DCB chunk from `+0xb4c8`.
uint64_t CGGfxContextGnm_FreeStagingHashMap(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics234CGGfxContextGnm_FreeStagingHashMapEPh");
uint64_t CGGfxContextGnm_FreeStagingHashMap(uint8_t* self) {
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0xb4c8);
    uint64_t qwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    return (qwords << 3) - 8ULL;
}

extern void FlushBufferUpdates_A(void* gfxCtx, void* updateChain); // 0x026adf50
extern void FlushBufferUpdates_B(void* gfxCtx, void* updateChain); // 0x026aa010

// 0x026af700 (763 bytes)
// Flushes pending staging pools, resource binder, label chunks, and submits the immediate DCB.
void CGGfxContextGnm_FlushPendingStagingUploads(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics242CGGfxContextGnm_FlushPendingStagingUploadsEPh");
void CGGfxContextGnm_FlushPendingStagingUploads(uint8_t* self) {
    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = *reinterpret_cast<uint64_t*>(self + 0x10);
    if (*reinterpret_cast<uint64_t*>(self + 0xb4d8) == 0) {
        return;
    }
    if (*reinterpret_cast<uintptr_t*>(self + 0x18) != GX_VMA(0x26af030ULL) &&
        *reinterpret_cast<uintptr_t*>(self + 0x18) !=
            reinterpret_cast<uintptr_t>(&CGGfxContextGnm_ImmediateDcbFullCallback)) {
        return;
    }

    if (gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_DecommissionLegacyPm4)) {
        gx_direct_vk::ReleaseU32LabelBatch u32Batch(self, 4u);
        uint8_t* chainHead = nullptr;
        uint8_t* chainTail = nullptr;
        auto enqueueBlock = [&](uint8_t* node) noexcept {
            *reinterpret_cast<uint64_t*>(node) = 2ULL;
            if (!chainHead) {
                chainHead = node;
            } else {
                *reinterpret_cast<uint8_t**>(chainTail + 0x28) = node;
            }
            chainTail = node;
            u32Batch.Add(reinterpret_cast<uint64_t>(node));
        };

        uint32_t mask = *reinterpret_cast<uint32_t*>(self + 0xb460);
        while (mask != 0) {
            uint32_t bit = static_cast<uint32_t>(__builtin_ctz(mask));
            uint8_t* blk =
                *reinterpret_cast<uint8_t**>(self + 0xb3d8 + static_cast<size_t>(bit) * 8ULL);
            while (blk) {
                uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
                enqueueBlock(blk);
                blk = nextBlk;
            }
            *reinterpret_cast<uint64_t*>(self + 0xb3d8 + static_cast<size_t>(bit) * 8ULL) = 0;
            mask -= (1u << bit);
        }
        *reinterpret_cast<uint32_t*>(self + 0xb460) = 0;

        uint8_t* blk = *reinterpret_cast<uint8_t**>(self + 0xb458);
        while (blk) {
            uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
            enqueueBlock(blk);
            blk = nextBlk;
        }
        *reinterpret_cast<uint64_t*>(self + 0xb458) = 0;

        if (chainHead) {
            uint8_t* alloc = *reinterpret_cast<uint8_t**>(self + 0xb3d0);
            uint8_t** headSlot = reinterpret_cast<uint8_t**>(alloc + 0x98);
            uint8_t* oldHead = __atomic_load_n(headSlot, __ATOMIC_RELAXED);
            do {
                *reinterpret_cast<uint8_t**>(chainTail + 0x28) = oldHead;
            } while (!__atomic_compare_exchange_n(
                headSlot, &oldHead, chainHead, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
            u32Batch.Flush();
            *reinterpret_cast<uint64_t*>(self + 0xb4b8) = *reinterpret_cast<uint64_t*>(self + 0x10);
        }
    } else {
        uint8_t* ctxRef = self;
        uint32_t mask = *reinterpret_cast<uint32_t*>(self + 0xb460);
        while (mask != 0) {
            uint32_t bit = static_cast<uint32_t>(__builtin_ctz(mask));
            uint8_t* blk =
                *reinterpret_cast<uint8_t**>(self + 0xb3d8 + static_cast<size_t>(bit) * 8ULL);
            while (blk) {
                uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
                reinterpret_cast<void(*)(void*, void*, void*)>(GX_VMA(0x26aa5a0ULL))(
                    *reinterpret_cast<void**>(self + 0xb3d0), &ctxRef, blk);
                blk = nextBlk;
            }
            *reinterpret_cast<uint64_t*>(self + 0xb3d8 + static_cast<size_t>(bit) * 8ULL) = 0;
            mask -= (1u << bit);
        }
        *reinterpret_cast<uint32_t*>(self + 0xb460) = 0;

        uint8_t* blk = *reinterpret_cast<uint8_t**>(self + 0xb458);
        while (blk) {
            uint8_t* nextBlk = *reinterpret_cast<uint8_t**>(blk + 0x28);
            reinterpret_cast<void(*)(void*, void*, void*)>(GX_VMA(0x26aa5a0ULL))(
                *reinterpret_cast<void**>(self + 0xb3d0), &ctxRef, blk);
            blk = nextBlk;
        }
        *reinterpret_cast<uint64_t*>(self + 0xb458) = 0;
    }

    reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26cd3f0ULL))(self + 0x40, self);

    uint8_t* labelChunk = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
    if (labelChunk) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
            const void* pool = *reinterpret_cast<void**>(self + 0xb4c8);
            gx_direct_vk::ReleaseLabelBatch batch(self);
            while (labelChunk) {
                uint8_t* nextLabelChunk = *reinterpret_cast<uint8_t**>(labelChunk + cap * 8ULL);
                uint64_t limit = nextLabelChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xb4ec);
                for (uint64_t i = 0; i < limit; ++i) {
                    batch.AddPoolSlot(pool, *reinterpret_cast<uint64_t**>(labelChunk + i * 8ULL));
                }
                labelChunk = nextLabelChunk;
            }
            batch.Flush();
            *reinterpret_cast<uint64_t*>(self + 0xb4b8) = *reinterpret_cast<uint64_t*>(self + 0x10);
        } else {
            while (labelChunk) {
                for (uint64_t i = 0;; ++i) {
                    uint8_t* nextLabelChunk = *reinterpret_cast<uint8_t**>(labelChunk + cap * 8ULL);
                    uint64_t limit = nextLabelChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xb4ec);
                    if (i >= limit) {
                        labelChunk = nextLabelChunk;
                        break;
                    }
                    reinterpret_cast<void(*)(void*, void*, void*)>(GX_VMA(0x26d3c80ULL))(
                        *reinterpret_cast<void**>(self + 0xb4c8),
                        self,
                        *reinterpret_cast<void**>(labelChunk + i * 8ULL));
                    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = *reinterpret_cast<uint64_t*>(self + 0x10);
                }
            }
        }
    }

    if (*reinterpret_cast<IAllocator**>(self + 0xb4d0) != nullptr) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
        while (node) {
            uint8_t* nextNode = *reinterpret_cast<uint8_t**>(node + cap * 8ULL);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
            alloc->vtable->deallocate(alloc, node);
            node = nextNode;
        }
    }
    *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4e0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4d8) = 0;

    uint8_t flag = self[0xb4f0];
    uint8_t* parentCtx = *reinterpret_cast<uint8_t**>(self + 0xb490);
    uint8_t* syncObj   = *reinterpret_cast<uint8_t**>(parentCtx + 0xb728);
    uint32_t targetVal = **reinterpret_cast<uint32_t**>(syncObj + 0x18) - 1u;
    uint32_t lastVal   = **reinterpret_cast<uint32_t**>(syncObj + 0x20);

    uint8_t savedDcb[0x40]{};
    uint8_t* firstDcb = nullptr;
    if (lastVal != targetVal) {
        if (((*reinterpret_cast<uintptr_t*>(self + 0x08) - *reinterpret_cast<uintptr_t*>(self + 0x10)) >> 2) < 7u) {
            (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
                self, 0, *reinterpret_cast<void**>(self + 0x20));
            return;
        }
        std::memcpy(savedDcb, self, 0x40);
        *reinterpret_cast<uint64_t*>(self + 0x00) = *reinterpret_cast<uint64_t*>(self + 0x10);
        firstDcb = self;
    } else {
        std::memcpy(savedDcb, self, 0x40);
    }

    CGGfxContextGnm_SubmitDcbPair(self, firstDcb, savedDcb, flag);
    uint64_t newBase = *reinterpret_cast<uint64_t*>(self + 0x10);
    *reinterpret_cast<uint64_t*>(self + 0x00)   = newBase;
    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = newBase;
}

// 0x026afa00 (295 bytes)
// Submits the immediate `Gnmx::GfxContext` command buffers to the GPU.
void CGGfxContextGnm_FlushAndSubmitImmediate(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics239CGGfxContextGnm_FlushAndSubmitImmediateEPhPmh");
void CGGfxContextGnm_FlushAndSubmitImmediate(uint8_t* self) {
    uint8_t flag = self[0xb4f0];
    uint8_t* parentCtx = *reinterpret_cast<uint8_t**>(self + 0xb490);
    uint8_t* syncObj   = *reinterpret_cast<uint8_t**>(parentCtx + 0xb728);
    uint32_t targetVal = **reinterpret_cast<uint32_t**>(syncObj + 0x18) - 1u;
    uint32_t lastVal   = **reinterpret_cast<uint32_t**>(syncObj + 0x20);

    uint8_t savedDcb[0x40]{};
    uint8_t* firstDcb = nullptr;
    if (lastVal != targetVal) {
        if (((*reinterpret_cast<uintptr_t*>(self + 0x08) - *reinterpret_cast<uintptr_t*>(self + 0x10)) >> 2) < 7u) {
            (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
                self, 0, *reinterpret_cast<void**>(self + 0x20));
            return;
        }
        std::memcpy(savedDcb, self, 0x40);
        *reinterpret_cast<uint64_t*>(self + 0x00) = *reinterpret_cast<uint64_t*>(self + 0x10);
        firstDcb = self;
    } else {
        std::memcpy(savedDcb, self, 0x40);
    }

    CGGfxContextGnm_SubmitDcbPair(self, firstDcb, savedDcb, flag);
    uint64_t newBase = *reinterpret_cast<uint64_t*>(self + 0x10);
    *reinterpret_cast<uint64_t*>(self + 0x00)   = newBase;
    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = newBase;
}

struct CGFlipSubmitArgs {
    uint32_t videoOutHandle; // +0x00
    uint32_t rtIndex;        // +0x04
    uint32_t flipMode;       // +0x08
    uint32_t pad_0c;         // +0x0c
    int64_t  flipArg;        // +0x10
    uint64_t flagsAndPad;    // +0x18
};

void CGGfxContextGnm_SubmitSingleDcbAndFlip(
    const CGFlipSubmitArgs* flipArgs, uint8_t* dcb)
    __asm__("_ZN4from13CoreGraphics238CGGfxContextGnm_SubmitSingleDcbAndFlipEPhPvjS2_jiijl");

void CGGfxContextGnm_SubmitDcbPairAndFlip(
    uint8_t* self, uint8_t* dcb0, uint8_t* dcb1, CGFlipSubmitArgs flipArgs)
    __asm__("_ZN4from13CoreGraphics236CGGfxContextGnm_SubmitDcbPairAndFlipEPhPvjS2_jS2_jS2_jiijl");

// 0x026afb30 (510 bytes)
// Submits all recorded DCB/CCB chunks on the immediate context and triggers
// a `sceVideoOut` display flip.
int32_t CGGfxContextGnm_SubmitAndFlipVideoOut(
    uint8_t* self,
    uint8_t* swapChainWrapper,
    uint32_t flipMode,
    int64_t flipArg,
    uint64_t* eopLabelPair)
{
    uint32_t videoOutHandle = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(swapChainWrapper));
    uint32_t rtIndex        = flipMode;
    uint32_t mode           = static_cast<uint32_t>(flipArg);
    int64_t  arg            = reinterpret_cast<int64_t>(eopLabelPair);

    uintptr_t dcbEnd = *reinterpret_cast<uintptr_t*>(self + 0x08);
    uintptr_t dcbCur = *reinterpret_cast<uintptr_t*>(self + 0x10);
    if (((dcbEnd - dcbCur) >> 2) <= 0x3Fu) {
        (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
            self, 0, self);
        dcbEnd = *reinterpret_cast<uintptr_t*>(self + 0x08);
        dcbCur = *reinterpret_cast<uintptr_t*>(self + 0x10);
        if (((dcbEnd - dcbCur) >> 2) < 0x40u) {
            return 9;
        }
    }

    reinterpret_cast<void(*)(void*)>(GX_VMA(0x10784a0ULL))(self);

    CGFlipSubmitArgs fargs{};
    fargs.videoOutHandle = videoOutHandle;
    fargs.rtIndex        = rtIndex;
    fargs.flipMode       = mode;
    fargs.flipArg        = arg;
    fargs.flagsAndPad    = self[0xb4f0];

    uint8_t* parentCtx = *reinterpret_cast<uint8_t**>(self + 0xb490);
    uint8_t* syncObj   = *reinterpret_cast<uint8_t**>(parentCtx + 0xb728);
    uint32_t targetVal = **reinterpret_cast<uint32_t**>(syncObj + 0x18) - 1u;
    uint32_t lastVal   = **reinterpret_cast<uint32_t**>(syncObj + 0x20);

    uint8_t savedDcb[0x40]{};
    uint8_t* firstDcb = nullptr;
    if (lastVal != targetVal) {
        if (((*reinterpret_cast<uintptr_t*>(self + 0x08) - *reinterpret_cast<uintptr_t*>(self + 0x10)) >> 2) < 7u) {
            (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
                self, 0, *reinterpret_cast<void**>(self + 0x20));
            return 0;
        }
        std::memcpy(savedDcb, self, 0x40);
        *reinterpret_cast<uint64_t*>(self + 0x00) = *reinterpret_cast<uint64_t*>(self + 0x10);
        firstDcb = self;
    } else {
        std::memcpy(savedDcb, self, 0x40);
    }

    CGGfxContextGnm_SubmitDcbPairAndFlip(self, firstDcb, savedDcb, fargs);
    uint64_t newBase = *reinterpret_cast<uint64_t*>(self + 0x10);
    *reinterpret_cast<uint64_t*>(self + 0x00)   = newBase;
    *reinterpret_cast<uint64_t*>(self + 0xb4b8) = newBase;
    return 0;
}

// 0x026b0110 (32 bytes)
// Flushes pending staging uploads and installs `CGGfxContextGnm_DeferredDcbFullCallback` (`0x26b0130`).
void CGGfxContextGnm_GetCommandChunkChain(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics236CGGfxContextGnm_GetCommandChunkChainEPhPPv");
void CGGfxContextGnm_GetCommandChunkChain(uint8_t* self) {
    CGGfxContextGnm_FlushPendingStagingUploads(self);
    *reinterpret_cast<void**>(self + 0x18) = reinterpret_cast<void*>(GX_VMA(0x26b0130ULL));
}

// 0x026b0130 (690 bytes)
// Out-of-space callback for deferred `CGGfxContextGnm` command contexts.
bool CGGfxContextGnm_DeferredDcbFullCallback(
    uint8_t* dcb,
    uint32_t /*sizeInDwords*/,
    uint8_t* self)
{
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0xb4c8);
    uint64_t poolQwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    void* newDcbBuf = nullptr;
    if (reinterpret_cast<int32_t(*)(void*, void**)>(GX_VMA(0x26d3b80ULL))(
            labelPool, &newDcbBuf) != 0) {
        return false;
    }

    uint64_t dcbWords = ((poolQwords << 3) - 8ULL) >> 2;
    uintptr_t curBase = *reinterpret_cast<uintptr_t*>(self + 0x00);
    uintptr_t curPtr  = *reinterpret_cast<uintptr_t*>(self + 0x10);
    uintptr_t fullTop = curBase ? (curBase + (*reinterpret_cast<uint64_t*>(labelPool + 0x08) << 3) - 8ULL) : 0ULL;

    if (curPtr != curBase || *reinterpret_cast<uintptr_t*>(self + 0x08) != fullTop) {
        uint32_t cap     = *reinterpret_cast<uint32_t*>(self + 0xb4b0);
        size_t   nextOff = static_cast<size_t>(cap) << 6;
        uint8_t* chunkNode = *reinterpret_cast<uint8_t**>(self + 0xb4a8);
        uint32_t idx       = *reinterpret_cast<uint32_t*>(self + 0xb4b4);

        if (*reinterpret_cast<void**>(self + 0xb4a0) == nullptr) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            chunkNode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
            if (!chunkNode) {
                return false;
            }
            *reinterpret_cast<void**>(chunkNode + nextOff) = nullptr;
            *reinterpret_cast<uint8_t**>(self + 0xb4a0) = chunkNode;
            *reinterpret_cast<uint8_t**>(self + 0xb4a8) = chunkNode;
            *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
            idx = 0;
        } else if (idx == cap) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            uint8_t* newChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
            if (!newChunk) {
                return false;
            }
            *reinterpret_cast<void**>(newChunk + nextOff) = nullptr;
            *reinterpret_cast<uint8_t**>(chunkNode + nextOff) = newChunk;
            *reinterpret_cast<uint8_t**>(self + 0xb4a8) = newChunk;
            *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
            chunkNode = newChunk;
            idx = 0;
        }

        *reinterpret_cast<uint32_t*>(self + 0xb4b4) = idx + 1u;
        uint8_t* dstSlot = chunkNode + (static_cast<size_t>(idx) << 6);
        if (!dstSlot) {
            return false;
        }
        std::memcpy(dstSlot, self, 0x40);
    }

    uint32_t lcap    = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
    size_t   lalloc  = static_cast<size_t>(lcap) * 8ULL + 8ULL;
    uint8_t* lnode   = *reinterpret_cast<uint8_t**>(self + 0xb4e0);
    uint32_t lidx    = *reinterpret_cast<uint32_t*>(self + 0xb4ec);
    if (*reinterpret_cast<void**>(self + 0xb4d8) == nullptr) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
        lnode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, lalloc, 8));
        if (!lnode) {
            return false;
        }
        *reinterpret_cast<void**>(lnode + static_cast<size_t>(lcap) * 8ULL) = nullptr;
        *reinterpret_cast<uint8_t**>(self + 0xb4d8) = lnode;
        *reinterpret_cast<uint8_t**>(self + 0xb4e0) = lnode;
        *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
        lidx = 0;
    } else if (lidx == lcap) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
        uint8_t* newLnode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, lalloc, 8));
        if (!newLnode) {
            return false;
        }
        *reinterpret_cast<void**>(newLnode + static_cast<size_t>(lcap) * 8ULL) = nullptr;
        *reinterpret_cast<uint8_t**>(lnode + static_cast<size_t>(lcap) * 8ULL) = newLnode;
        *reinterpret_cast<uint8_t**>(self + 0xb4e0) = newLnode;
        *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
        lnode = newLnode;
        lidx = 0;
    }

    *reinterpret_cast<uint32_t*>(self + 0xb4ec) = lidx + 1u;
    void** slotPtr = reinterpret_cast<void**>(lnode + static_cast<size_t>(lidx) * 8ULL);
    if (!slotPtr) {
        return false;
    }
    *slotPtr = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d3d40ULL))(
        labelPool, *reinterpret_cast<void**>(dcb + 0x00));

    *reinterpret_cast<void**>(dcb + 0x00) = newDcbBuf;
    *reinterpret_cast<void**>(dcb + 0x10) = newDcbBuf;
    *reinterpret_cast<void**>(dcb + 0x08) = static_cast<uint8_t*>(newDcbBuf) + (dcbWords << 2);
    return true;
}

// 0x026b03f0 (565 bytes)
// Records a deferred DCB chunk and appends `srcChunk` into the chunk chain.
int32_t CGGfxContextGnm_RecordDeferredCommandChunk(
    uint8_t* self,
    const uint8_t* srcChunk)
    __asm__("_ZN4from13CoreGraphics242CGGfxContextGnm_RecordDeferredCommandChunkEPhjj");
int32_t CGGfxContextGnm_RecordDeferredCommandChunk(
    uint8_t* self,
    const uint8_t* srcChunk)
{
    if (*reinterpret_cast<uintptr_t*>(self + 0x00) < *reinterpret_cast<uintptr_t*>(self + 0x10)) {
        uint32_t cap     = *reinterpret_cast<uint32_t*>(self + 0xb4b0);
        size_t   nextOff = static_cast<size_t>(cap) << 6;
        uint8_t* chunkNode = *reinterpret_cast<uint8_t**>(self + 0xb4a8);
        uint32_t idx       = *reinterpret_cast<uint32_t*>(self + 0xb4b4);

        if (*reinterpret_cast<void**>(self + 0xb4a0) == nullptr) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            chunkNode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
            if (!chunkNode) {
                return 9;
            }
            *reinterpret_cast<void**>(chunkNode + nextOff) = nullptr;
            *reinterpret_cast<uint8_t**>(self + 0xb4a0) = chunkNode;
            *reinterpret_cast<uint8_t**>(self + 0xb4a8) = chunkNode;
            *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
            idx = 0;
        } else if (idx == cap) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            uint8_t* newChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
            if (!newChunk) {
                return 9;
            }
            *reinterpret_cast<void**>(newChunk + nextOff) = nullptr;
            *reinterpret_cast<uint8_t**>(chunkNode + nextOff) = newChunk;
            *reinterpret_cast<uint8_t**>(self + 0xb4a8) = newChunk;
            *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
            chunkNode = newChunk;
            idx = 0;
        }

        *reinterpret_cast<uint32_t*>(self + 0xb4b4) = idx + 1u;
        uint8_t* dstSlot = chunkNode + (static_cast<size_t>(idx) << 6);
        if (!dstSlot) {
            return 9;
        }
        std::memcpy(dstSlot, self, 0x40);
        *reinterpret_cast<uint64_t*>(self + 0x00) = *reinterpret_cast<uint64_t*>(self + 0x10);
        CGGfxContextGnm_FlushPendingStagingUploads(self);
    }

    uint32_t cap     = *reinterpret_cast<uint32_t*>(self + 0xb4b0);
    size_t   nextOff = static_cast<size_t>(cap) << 6;
    uint8_t* chunkNode = *reinterpret_cast<uint8_t**>(self + 0xb4a8);
    uint32_t idx       = *reinterpret_cast<uint32_t*>(self + 0xb4b4);

    if (*reinterpret_cast<void**>(self + 0xb4a0) == nullptr) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
        chunkNode = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
        if (!chunkNode) {
            return 9;
        }
        *reinterpret_cast<void**>(chunkNode + nextOff) = nullptr;
        *reinterpret_cast<uint8_t**>(self + 0xb4a0) = chunkNode;
        *reinterpret_cast<uint8_t**>(self + 0xb4a8) = chunkNode;
        *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
        idx = 0;
    } else if (idx == cap) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
        uint8_t* newChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, nextOff + 8ULL, 8));
        if (!newChunk) {
            return 9;
        }
        *reinterpret_cast<void**>(newChunk + nextOff) = nullptr;
        *reinterpret_cast<uint8_t**>(chunkNode + nextOff) = newChunk;
        *reinterpret_cast<uint8_t**>(self + 0xb4a8) = newChunk;
        *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
        chunkNode = newChunk;
        idx = 0;
    }

    *reinterpret_cast<uint32_t*>(self + 0xb4b4) = idx + 1u;
    uint8_t* dstSlot = chunkNode + (static_cast<size_t>(idx) << 6);
    if (!dstSlot) {
        return 9;
    }
    std::memcpy(dstSlot, srcChunk, 0x40);
    return 0;
}

// 0x026b0630 (433 bytes)
// Submits all recorded chunks on `CGGfxContextGnm` and releases the chunk chains.
void CGGfxContextGnm_SubmitAllAndReleaseChunks(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics241CGGfxContextGnm_SubmitAllAndReleaseChunksEPhS1_");
void CGGfxContextGnm_SubmitAllAndReleaseChunks(uint8_t* self) {
    CGGfxContextGnm_FlushPendingStagingUploads(self);

    uint8_t* labelChunk = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
    if (labelChunk) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
            const void* pool = *reinterpret_cast<void**>(self + 0xb4c8);
            gx_direct_vk::ReleaseLabelBatch batch(self);
            while (labelChunk) {
                uint8_t* nextLabelChunk = *reinterpret_cast<uint8_t**>(labelChunk + cap * 8ULL);
                uint64_t limit = nextLabelChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xb4ec);
                for (uint64_t i = 0; i < limit; ++i) {
                    batch.AddPoolSlot(pool, *reinterpret_cast<uint64_t**>(labelChunk + i * 8ULL));
                }
                labelChunk = nextLabelChunk;
            }
            batch.Flush();
            CGGfxContextGnm_FlushPendingStagingUploads(self);
        } else {
            while (labelChunk) {
                for (uint64_t i = 0;; ++i) {
                    uint8_t* nextLabelChunk = *reinterpret_cast<uint8_t**>(labelChunk + cap * 8ULL);
                    uint64_t limit = nextLabelChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xb4ec);
                    if (i >= limit) {
                        labelChunk = nextLabelChunk;
                        break;
                    }
                    reinterpret_cast<void(*)(void*, void*, void*)>(GX_VMA(0x26d3c80ULL))(
                        *reinterpret_cast<void**>(self + 0xb4c8),
                        self,
                        *reinterpret_cast<void**>(labelChunk + i * 8ULL));
                    CGGfxContextGnm_FlushPendingStagingUploads(self);
                }
            }
        }
    }

    if (*reinterpret_cast<IAllocator**>(self + 0xb4d0) != nullptr) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xb4e8);
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xb4d8);
        while (node) {
            uint8_t* nextNode = *reinterpret_cast<uint8_t**>(node + cap * 8ULL);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb4d0);
            alloc->vtable->deallocate(alloc, node);
            node = nextNode;
        }
    }
    *reinterpret_cast<uint32_t*>(self + 0xb4ec) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4e0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4d8) = 0;

    reinterpret_cast<void(*)(void*)>(GX_VMA(0x26bb730ULL))(self + 0xb498);

    if (*reinterpret_cast<IAllocator**>(self + 0xb498) != nullptr) {
        size_t nextOff = static_cast<size_t>(*reinterpret_cast<uint32_t*>(self + 0xb4b0)) << 6;
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xb4a0);
        while (node) {
            uint8_t* nextNode = *reinterpret_cast<uint8_t**>(node + nextOff);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb498);
            alloc->vtable->deallocate(alloc, node);
            node = nextNode;
        }
    }
    *reinterpret_cast<uint64_t*>(self + 0xb4a0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xb4a8) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xb4b4) = 0;
    *reinterpret_cast<void**>(self + 0x18) = reinterpret_cast<void*>(GX_VMA(0x26af030ULL));
}

// 0x026b07f0 (231 bytes)
// Suballocates `sizeInBytes` with power-of-two `alignmentInBytes` downwards from
// the top of the current DCB chunk (`*(uintptr_t*)(self + 0x08)`).
void* CGGfxContextGnm_AllocateFromCommandBufferAligned(
    uint8_t* self,
    size_t sizeInBytes,
    size_t alignmentInBytes)
    __asm__("_ZN4from13CoreGraphics248CGGfxContextGnm_AllocateFromCommandBufferAlignedEPhjj");
void* CGGfxContextGnm_AllocateFromCommandBufferAligned(
    uint8_t* self,
    size_t sizeInBytes,
    size_t alignmentInBytes)
{
    if (*reinterpret_cast<uint64_t*>(self + 0x00) == 0) {
        (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
            self, 0, *reinterpret_cast<void**>(self + 0x20));
        if (*reinterpret_cast<uint64_t*>(self + 0x00) == 0) {
            return nullptr;
        }
    }

    size_t alignMask = alignmentInBytes - 1ULL;
    size_t dwords    = (sizeInBytes + 3ULL) >> 2;
    size_t byteWords = dwords << 2;

    uintptr_t top = *reinterpret_cast<uintptr_t*>(self + 0x08);
    size_t pad    = (top - byteWords) & alignMask;
    size_t needed = pad + byteWords;

    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0xb4c8);
    size_t maxChunkBytes = (*reinterpret_cast<uint64_t*>(labelPool + 0x08) << 3) - 8ULL;
    if (needed >= maxChunkBytes) {
        return nullptr;
    }

    size_t neededWords = (needed >> 2) << 2;
    if ((top - neededWords) <= *reinterpret_cast<uintptr_t*>(self + 0x10)) {
        (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
            self, 0, *reinterpret_cast<void**>(self + 0x20));
        top = *reinterpret_cast<uintptr_t*>(self + 0x08);
        pad = (top - byteWords) & alignMask;
        needed = pad + byteWords;
        neededWords = (needed >> 2) << 2;
        if ((top - neededWords) <= *reinterpret_cast<uintptr_t*>(self + 0x10)) {
            return nullptr;
        }
    }

    uintptr_t res = top - neededWords;
    *reinterpret_cast<uintptr_t*>(self + 0x08) = res;
    return reinterpret_cast<void*>(res);
}

// 0x026b08e0 (243 bytes)
// Allocates an 8-byte label in the DCB, writes 1 at EoP (`0x1078320`), and waits on it (`0x1077b30`).
void CGGfxContextGnm_AllocateScratchFromCommandBuffer(
    uint8_t* self,
    uint32_t eventType)
    __asm__("_ZN4from13CoreGraphics248CGGfxContextGnm_AllocateScratchFromCommandBufferEPhjjPPv");
void CGGfxContextGnm_AllocateScratchFromCommandBuffer(
    uint8_t* self,
    uint32_t eventType)
{
    if (gx_direct_vk::EnabledV7Sub(gx_direct_vk::kV7Sub_BoundedCueAndQueries)) {
        if (*reinterpret_cast<uintptr_t*>(self) == 0) {
            (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
                self, 0, *reinterpret_cast<void**>(self + 0x20));
        }
        return;
    }
    uint64_t* labelPtr = static_cast<uint64_t*>(
        CGGfxContextGnm_AllocateFromCommandBufferAligned(self, 8, 8));
    if (!labelPtr) {
        return;
    }
    *labelPtr = 0;
    reinterpret_cast<void(*)(void*, uint32_t, void*, uint64_t)>(GX_VMA(0x1078320ULL))(
        self, eventType, labelPtr, 1ULL);
    reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1077b30ULL))(
        self, labelPtr, 0xFFFFFFFFu, 3u, 1u);
}

// 0x026b09e0 (249 bytes)
// Allocates an 8-byte label in the DCB, emits EoP with cache action (`0x26bb6d0`), and waits on it (`0x1077b30`).
void CGGfxContextGnm_FlushShaderCachesAndWait(
    uint8_t* self,
    uint32_t eventType,
    uint32_t cacheAction)
    __asm__("_ZN4from13CoreGraphics240CGGfxContextGnm_FlushShaderCachesAndWaitEPh");
void CGGfxContextGnm_FlushShaderCachesAndWait(
    uint8_t* self,
    uint32_t eventType,
    uint32_t cacheAction)
{
    if (gx_direct_vk::EnabledV7Sub(gx_direct_vk::kV7Sub_BoundedCueAndQueries)) {
        if (*reinterpret_cast<uintptr_t*>(self) == 0) {
            (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
                self, 0, *reinterpret_cast<void**>(self + 0x20));
        }
        return;
    }
    uint64_t* labelPtr = static_cast<uint64_t*>(
        CGGfxContextGnm_AllocateFromCommandBufferAligned(self, 8, 8));
    if (!labelPtr) {
        return;
    }
    *labelPtr = 0;
    reinterpret_cast<void(*)(void*, uint32_t, void*, uint64_t, uint32_t)>(GX_VMA(0x26bb6d0ULL))(
        self, eventType, labelPtr, 1ULL, cacheAction);
    reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1077b30ULL))(
        self, labelPtr, 0xFFFFFFFFu, 3u, 1u);
}

// 0x026b0ae0 (27 bytes)
// Returns the available aligned space in the current DCB chunk.
int64_t CGGfxContextGnm_GetAvailableDcbBytesAligned(uint8_t* self, size_t alignment)
    __asm__("_ZN4from13CoreGraphics243CGGfxContextGnm_GetAvailableDcbBytesAlignedEPh");
int64_t CGGfxContextGnm_GetAvailableDcbBytesAligned(uint8_t* self, size_t alignment) {
    uintptr_t top = *reinterpret_cast<uintptr_t*>(self + 0x08);
    uintptr_t cur = *reinterpret_cast<uintptr_t*>(self + 0x10);
    uintptr_t alignedCur = (cur + alignment - 1ULL) & ~(alignment - 1ULL);
    return static_cast<int64_t>((top - 4ULL) - alignedCur);
}

// 0x026b0b00 (327 bytes)
// Submits a pair of DCB command buffers via `0x1089a10`.
void CGGfxContextGnm_SubmitDcbPair(
    uint8_t* self,
    uint8_t* dcb0,
    uint8_t* dcb1,
    uint8_t /*flag*/)
{
    void*    dcbAddrs[2];
    uint32_t dcbSizes[2];
    void*    dcb0Base  = nullptr;
    uint32_t dcb0Bytes = 0;
    void*    dcb1Base  = nullptr;
    uint32_t dcb1Bytes = 0;

    if (dcb0) {
        uint8_t* parentCtx = *reinterpret_cast<uint8_t**>(self + 0xb490);
        uint8_t* syncObj   = *reinterpret_cast<uint8_t**>(parentCtx + 0xb728);
        uint32_t targetVal = **reinterpret_cast<uint32_t**>(syncObj + 0x18) - 1u;
        uint32_t lastVal   = **reinterpret_cast<uint32_t**>(syncObj + 0x20);
        if (lastVal != targetVal) {
            if (static_cast<int32_t>(lastVal) < 0 && static_cast<int32_t>(targetVal) >= 0) {
                reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t)>(GX_VMA(0x1077b30ULL))(
                    dcb0, *reinterpret_cast<void**>(syncObj + 0x10), 0xFFFFFFFFu, 1u);
            }
            reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1077b30ULL))(
                dcb0, *reinterpret_cast<void**>(syncObj + 0x10), 0xFFFFFFFFu, 5u, targetVal);
            **reinterpret_cast<uint32_t**>(syncObj + 0x20) = targetVal;
        }
        dcb0Base  = *reinterpret_cast<void**>(dcb0 + 0x00);
        dcb0Bytes = (*reinterpret_cast<uint32_t*>(dcb0 + 0x10) -
                     static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb0Base))) & ~3u;
        dcb1Base  = *reinterpret_cast<void**>(dcb1 + 0x00);
        dcb1Bytes = (*reinterpret_cast<uint32_t*>(dcb1 + 0x10) -
                     static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb1Base))) & ~3u;
    } else {
        uint8_t dummyDcb[0x40]{};
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
            gx_direct_vk::InitDrawCommandBuffer(dummyDcb, nullptr, 0, nullptr, nullptr);
        } else {
            reinterpret_cast<void(*)(void*, void*, uint32_t, void*, void*)>(GX_VMA(0x1071270ULL))(
                dummyDcb, nullptr, 0, nullptr, nullptr);
        }
        dcb0Base  = *reinterpret_cast<void**>(dcb1 + 0x00);
        dcb0Bytes = (*reinterpret_cast<uint32_t*>(dcb1 + 0x10) -
                     static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb0Base))) & ~3u;
        dcb1Base  = *reinterpret_cast<void**>(dummyDcb + 0x00);
        dcb1Bytes = (*reinterpret_cast<uint32_t*>(dummyDcb + 0x10) -
                     static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb1Base))) & ~3u;
    }

    uint32_t count = 0;
    if (dcb0Bytes != 0 && dcb1Bytes != 0) {
        dcbAddrs[0] = dcb0Base;
        dcbSizes[0] = dcb0Bytes;
        dcbAddrs[1] = dcb1Base;
        dcbSizes[1] = dcb1Bytes;
        count = 2;
    } else if (dcb0Bytes != 0) {
        dcbAddrs[0] = dcb0Base;
        dcbSizes[0] = dcb0Bytes;
        count = 1;
    } else if (dcb1Bytes != 0) {
        dcbAddrs[0] = dcb1Base;
        dcbSizes[0] = dcb1Bytes;
        count = 1;
    } else {
        return;
    }

    reinterpret_cast<int32_t(*)(uint32_t, void**, uint32_t*, void**, uint32_t*)>(GX_VMA(0x1089a10ULL))(
        count, dcbAddrs, dcbSizes, nullptr, nullptr);
}

// 0x026b0ce0 (327 bytes)
// Submits a pair of DCB command buffers and requests a VideoOut flip via `0x1089a20` + `0x1089b50`.
void CGGfxContextGnm_SubmitDcbPairAndFlip(
    uint8_t* self,
    uint8_t* dcb0,
    uint8_t* dcb1,
    CGFlipSubmitArgs flipArgs)
{
    if (!dcb0) {
        CGGfxContextGnm_SubmitSingleDcbAndFlip(&flipArgs, dcb1);
        return;
    }

    uint8_t* parentCtx = *reinterpret_cast<uint8_t**>(self + 0xb490);
    uint8_t* syncObj   = *reinterpret_cast<uint8_t**>(parentCtx + 0xb728);
    uint32_t targetVal = **reinterpret_cast<uint32_t**>(syncObj + 0x18) - 1u;
    uint32_t lastVal   = **reinterpret_cast<uint32_t**>(syncObj + 0x20);
    if (lastVal != targetVal) {
        if (static_cast<int32_t>(lastVal) < 0 && static_cast<int32_t>(targetVal) >= 0) {
            reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t)>(GX_VMA(0x1077b30ULL))(
                dcb0, *reinterpret_cast<void**>(syncObj + 0x10), 0xFFFFFFFFu, 1u);
        }
        reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1077b30ULL))(
            dcb0, *reinterpret_cast<void**>(syncObj + 0x10), 0xFFFFFFFFu, 5u, targetVal);
        **reinterpret_cast<uint32_t**>(syncObj + 0x20) = targetVal;
    }

    void*    dcb0Base  = *reinterpret_cast<void**>(dcb0 + 0x00);
    uint32_t dcb0Bytes = (*reinterpret_cast<uint32_t*>(dcb0 + 0x10) -
                          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb0Base))) & ~3u;
    void*    dcb1Base  = *reinterpret_cast<void**>(dcb1 + 0x00);
    uint32_t dcb1Bytes = (*reinterpret_cast<uint32_t*>(dcb1 + 0x10) -
                          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb1Base))) & ~3u;

    void*    dcbAddrs[2];
    uint32_t dcbSizes[2];
    uint32_t count = 0;
    if (dcb0Bytes != 0 && dcb1Bytes != 0) {
        dcbAddrs[0] = dcb0Base;
        dcbSizes[0] = dcb0Bytes;
        dcbAddrs[1] = dcb1Base;
        dcbSizes[1] = dcb1Bytes;
        count = 2;
    } else if (dcb0Bytes != 0) {
        dcbAddrs[0] = dcb0Base;
        dcbSizes[0] = dcb0Bytes;
        count = 1;
    } else if (dcb1Bytes != 0) {
        dcbAddrs[0] = dcb1Base;
        dcbSizes[0] = dcb1Bytes;
        count = 1;
    } else {
        return;
    }

    reinterpret_cast<int32_t(*)(uint32_t, void**, uint32_t*, void**, uint32_t*, uint32_t, uint32_t, uint32_t, int64_t)>(
        GX_VMA(0x1089a20ULL))(
        count, dcbAddrs, dcbSizes, nullptr, nullptr,
        flipArgs.videoOutHandle, flipArgs.rtIndex, flipArgs.flipMode, flipArgs.flipArg);
    reinterpret_cast<int32_t(*)()>(GX_VMA(0x1089b50ULL))();
}

// 0x026b0e30 (214 bytes)
// Submits a single DCB command buffer and requests a VideoOut flip.
void CGGfxContextGnm_SubmitSingleDcbAndFlip(
    const CGFlipSubmitArgs* flipArgs,
    uint8_t* dcb)
{
    uint8_t dummyDcb[0x40]{};
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
        gx_direct_vk::InitDrawCommandBuffer(dummyDcb, nullptr, 0, nullptr, nullptr);
    } else {
        reinterpret_cast<void(*)(void*, void*, uint32_t, void*, void*)>(GX_VMA(0x1071270ULL))(
            dummyDcb, nullptr, 0, nullptr, nullptr);
    }

    void*    dcb0Base  = *reinterpret_cast<void**>(dcb + 0x00);
    uint32_t dcb0Bytes = (*reinterpret_cast<uint32_t*>(dcb + 0x10) -
                          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb0Base))) & ~3u;
    void*    dcb1Base  = *reinterpret_cast<void**>(dummyDcb + 0x00);
    uint32_t dcb1Bytes = (*reinterpret_cast<uint32_t*>(dummyDcb + 0x10) -
                          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dcb1Base))) & ~3u;

    void*    dcbAddrs[2];
    uint32_t dcbSizes[2];
    uint32_t count = 0;
    if (dcb0Bytes != 0 && dcb1Bytes != 0) {
        dcbAddrs[0] = dcb0Base;
        dcbSizes[0] = dcb0Bytes;
        dcbAddrs[1] = dcb1Base;
        dcbSizes[1] = dcb1Bytes;
        count = 2;
    } else if (dcb0Bytes != 0) {
        dcbAddrs[0] = dcb0Base;
        dcbSizes[0] = dcb0Bytes;
        count = 1;
    } else if (dcb1Bytes != 0) {
        dcbAddrs[0] = dcb1Base;
        dcbSizes[0] = dcb1Bytes;
        count = 1;
    } else {
        return;
    }

    reinterpret_cast<int32_t(*)(uint32_t, void**, uint32_t*, void**, uint32_t*, uint32_t, uint32_t, uint32_t, int64_t)>(
        GX_VMA(0x1089a20ULL))(
        count, dcbAddrs, dcbSizes, nullptr, nullptr,
        flipArgs->videoOutHandle, flipArgs->rtIndex, flipArgs->flipMode, flipArgs->flipArg);
    reinterpret_cast<int32_t(*)()>(GX_VMA(0x1089b50ULL))();
}

// ============================================================================
// 3. CGComputeContextGnm & Async Compute Queue (0x26b0f10 .. 0x26b1b90)
// ============================================================================

extern void CGShaderResourceBinderGnm_InitializeCompute(void* binder, void* alloc,
                                                        void* cmpCtx); // 0x026bbdd0

// 0x026b0f10 (202 bytes)
// Default constructor of `CGComputeContextGnm` (size 0xd8 bytes).
void CGComputeContextGnm_Construct(uint8_t* self) {
    *reinterpret_cast<uint64_t*>(self + 0x28) =
        (*reinterpret_cast<uint64_t*>(self + 0x28) & ~3ULL) | 2ULL;
    std::memset(self + 0x40, 0, 0x98);
}

// 0x026b0fe0 (183 bytes)
// Copy constructor of `CGComputeContextGnm`.
void CGComputeContextGnm_CopyConstruct(uint8_t* dst, const uint8_t* src) {
    std::memcpy(dst, src, 0xd8);
    *reinterpret_cast<uint8_t**>(dst + 0x20) = dst;
}

// 0x026b10a0 (134 bytes)
// Frees the linked list of label/fence chunks at `self + 0xc0`.
void CGComputeContextGnm_FreeLabelChunks(uint8_t* self) {
    if (*reinterpret_cast<IAllocator**>(self + 0xb8) != nullptr) {
        uint32_t cap = *reinterpret_cast<uint32_t*>(self + 0xd0);
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xc0);
        while (node) {
            uint8_t* next = *reinterpret_cast<uint8_t**>(node + static_cast<size_t>(cap) * 8);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb8);
            alloc->vtable->deallocate(alloc, node);
            node = next;
        }
        *reinterpret_cast<uint32_t*>(self + 0xd4) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
        *reinterpret_cast<uint64_t*>(self + 0xc0) = 0;
    }
}

bool CGComputeContextGnm_ImmediateDcbFullCallback(
    uint8_t* dcb, uint32_t sizeInDwords, uint8_t* userData)
    __asm__("_ZN4from13CoreGraphics244CGComputeContextGnm_ImmediateDcbFullCallbackEPvjS1_");
void CGComputeContextGnm_DeferredDcbFullCallback(void* dcb, uint32_t sizeInDwords, void* userData);  // 0x026b17b0
int32_t CGComputeContextGnm_Finalize(uint8_t* self); // 0x026b12f0
void* CGComputeQueueDingDongGnm_Create(
    void* cpuAlloc, IAllocator* gpuAlloc, uint32_t count, const uint32_t* initValPtr)
    __asm__("_ZN4from13CoreGraphics232CGComputeQueueDingDongGnm_CreateEPh");

// 0x026b1130 (440 bytes)
// Initializes `CGComputeContextGnm` (Async Compute queue + DCB ring).
int32_t CGComputeContextGnm_Initialize(
    uint8_t* self,
    uint32_t pipeId,
    uint32_t queueId,
    uint64_t ringSizeInBytes,
    uint8_t* labelPool,
    void* /*unusedR9*/,
    IAllocator* cpuAlloc,
    IAllocator* gpuAlloc)
    __asm__("_ZN4from13CoreGraphics230CGComputeContextGnm_InitializeEPhPPNS0_10IAllocatorEh");
int32_t CGComputeContextGnm_Initialize(
    uint8_t* self,
    uint32_t pipeId,
    uint32_t queueId,
    uint64_t ringSizeInBytes,
    uint8_t* labelPool,
    void* /*unusedR9*/,
    IAllocator* cpuAlloc,
    IAllocator* gpuAlloc)
{
    uint64_t poolQwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    *reinterpret_cast<IAllocator**>(self + 0x48) = gpuAlloc;
    *reinterpret_cast<uint8_t**>(self + 0x50) = labelPool;
    *reinterpret_cast<uint64_t*>(self + 0xb0) = 0;
    *reinterpret_cast<IAllocator**>(self + 0xb8) = cpuAlloc;
    *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xc0) = 0;
    *reinterpret_cast<uint32_t*>(self + 0xd0) = 8;
    *reinterpret_cast<uint32_t*>(self + 0xd4) = 0;

    void* ringMem = gpuAlloc->vtable->allocateAligned(gpuAlloc, ringSizeInBytes + 0x18ULL, 0x100);
    *reinterpret_cast<void**>(self + 0xb0) = ringMem;
    if (!ringMem) {
        CGComputeContextGnm_Finalize(self);
        return 9;
    }

    *reinterpret_cast<uint64_t*>(ringMem) = 0;
    uint8_t* ringBase = *reinterpret_cast<uint8_t**>(self + 0xb0);
    void* readPtrAddr = ringBase + ringSizeInBytes;

    reinterpret_cast<void(*)(void*, uint32_t, uint32_t)>(GX_VMA(0x108c7a0ULL))(
        self + 0x58, pipeId, queueId);
    bool mapped = reinterpret_cast<bool(*)(void*, void*, uint32_t, void*)>(GX_VMA(0x108c7b0ULL))(
        self + 0x58, ringBase, static_cast<uint32_t>(ringSizeInBytes >> 2), readPtrAddr);
    if (!mapped) {
        CGComputeContextGnm_Finalize(self);
        return 5;
    }

    void* dingDong = CGComputeQueueDingDongGnm_Create(cpuAlloc, gpuAlloc, 1, nullptr);
    *reinterpret_cast<void**>(self + 0x40) = dingDong;
    if (!dingDong) {
        return 9;
    }

    void* dcbBuf = nullptr;
    int32_t poolErr = reinterpret_cast<int32_t(*)(void*, void**)>(GX_VMA(0x26d3b80ULL))(
        labelPool, &dcbBuf);
    if (poolErr != 0) {
        CGComputeContextGnm_Finalize(self);
        return 9;
    }

    uint32_t dcbBytes = static_cast<uint32_t>((poolQwords << 3) - 8ULL);
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb)) {
        gx_direct_vk::InitDrawCommandBuffer(
            self, dcbBuf, dcbBytes,
            reinterpret_cast<void*>(&CGComputeContextGnm_ImmediateDcbFullCallback), self);
    } else {
        reinterpret_cast<void(*)(void*, void*, uint32_t, void*, void*)>(GX_VMA(0x1071270ULL))(
            self, dcbBuf, dcbBytes,
            reinterpret_cast<void*>(&CGComputeContextGnm_ImmediateDcbFullCallback), self);
    }
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x1089c50ULL))(self);
    return 0;
}

// 0x026b12f0 (390 bytes)
// Finalizes `CGComputeContextGnm`, unmaps the compute queue, and releases all buffers.
int32_t CGComputeContextGnm_Finalize(uint8_t* self) {
    void* curDcb = *reinterpret_cast<void**>(self + 0x00);
    if (curDcb) {
        void* labelPool = *reinterpret_cast<void**>(self + 0x50);
        void* slot = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d3d40ULL))(
            labelPool, curDcb);
        reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26d3d30ULL))(labelPool, slot);
        std::memset(self, 0, 0x40);
    }

    uint8_t* labelChunk = *reinterpret_cast<uint8_t**>(self + 0xc0);
    if (labelChunk) {
        uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xd0);
        while (labelChunk) {
            uint8_t* nextChunk = *reinterpret_cast<uint8_t**>(labelChunk + cap * 8);
            uint64_t limit = nextChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xd4);
            for (uint64_t i = 0; i < limit; ++i) {
                reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26d3d30ULL))(
                    *reinterpret_cast<void**>(self + 0x50),
                    *reinterpret_cast<void**>(labelChunk + i * 8));
            }
            labelChunk = nextChunk;
        }
    }

    uint8_t* dingDong = *reinterpret_cast<uint8_t**>(self + 0x40);
    if (dingDong) {
        IAllocator* ownerAlloc = reinterpret_cast<IAllocator*(*)(void*)>(GX_VMA(0x207b720ULL))(dingDong);
        void* cpuMem = *reinterpret_cast<void**>(dingDong + 0x18);
        if (cpuMem) {
            IAllocator* a0 = *reinterpret_cast<IAllocator**>(dingDong + 0x00);
            a0->vtable->deallocate(a0, cpuMem);
            *reinterpret_cast<uint64_t*>(dingDong + 0x18) = 0;
        }
        void* gpuMem = *reinterpret_cast<void**>(dingDong + 0x10);
        if (gpuMem) {
            IAllocator* a1 = *reinterpret_cast<IAllocator**>(dingDong + 0x08);
            a1->vtable->deallocate(a1, gpuMem);
            *reinterpret_cast<uint64_t*>(dingDong + 0x10) = 0;
        }
        ownerAlloc->vtable->deallocate(ownerAlloc, dingDong);
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }

    if (*reinterpret_cast<uint32_t*>(self + 0x60) != 0) {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x108c810ULL))(self + 0x58);
    }

    IAllocator* gpuAlloc = *reinterpret_cast<IAllocator**>(self + 0x48);
    if (gpuAlloc) {
        void* ringMem = *reinterpret_cast<void**>(self + 0xb0);
        if (ringMem) {
            gpuAlloc->vtable->deallocate(gpuAlloc, ringMem);
            *reinterpret_cast<uint64_t*>(self + 0xb0) = 0;
        }
        *reinterpret_cast<uint64_t*>(self + 0x48) = 0;
    }
    *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
    return 0;
}

// 0x026b1480 (310 bytes)
// Allocates and initializes a 0x30-byte `CGComputeQueueDingDongGnm` synchronization object.
void* CGComputeQueueDingDongGnm_Create(
    void* cpuAllocVoid,
    IAllocator* gpuAlloc,
    uint32_t count,
    const uint32_t* initValPtr)
{
    IAllocator* cpuAlloc = static_cast<IAllocator*>(cpuAllocVoid);
    uint8_t* obj = static_cast<uint8_t*>(
        reinterpret_cast<void*(*)(size_t, size_t, void*)>(GX_VMA(0x207c4e0ULL))(0x30, 8, cpuAlloc));
    if (!obj) {
        return nullptr;
    }

    *reinterpret_cast<IAllocator**>(obj + 0x00) = cpuAlloc;
    *reinterpret_cast<IAllocator**>(obj + 0x08) = gpuAlloc;
    *reinterpret_cast<uint64_t*>(obj + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(obj + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(obj + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(obj + 0x28) = initValPtr ? static_cast<uint64_t>(*initValPtr) : ~0ULL;

    size_t byteSize = static_cast<size_t>(count) * 8ULL;
    void* gpuMem = gpuAlloc->vtable->allocate(gpuAlloc, byteSize);
    *reinterpret_cast<void**>(obj + 0x10) = gpuMem;
    if (gpuMem) {
        std::memset(gpuMem, 0, byteSize);
        void* cpuMem = cpuAlloc->vtable->allocate(cpuAlloc, byteSize);
        *reinterpret_cast<void**>(obj + 0x18) = cpuMem;
        if (cpuMem) {
            *reinterpret_cast<void**>(obj + 0x20) = static_cast<uint8_t*>(cpuMem) + static_cast<size_t>(count) * 4ULL;
            std::memset(cpuMem, 0, byteSize);
            return obj;
        }
    }

    void* cpuMem = *reinterpret_cast<void**>(obj + 0x18);
    if (cpuMem) {
        IAllocator* a0 = *reinterpret_cast<IAllocator**>(obj + 0x00);
        a0->vtable->deallocate(a0, cpuMem);
        *reinterpret_cast<uint64_t*>(obj + 0x18) = 0;
    }
    void* gMem = *reinterpret_cast<void**>(obj + 0x10);
    if (gMem) {
        IAllocator* a1 = *reinterpret_cast<IAllocator**>(obj + 0x08);
        a1->vtable->deallocate(a1, gMem);
        *reinterpret_cast<uint64_t*>(obj + 0x10) = 0;
    }
    cpuAlloc->vtable->deallocate(cpuAlloc, obj);
    return nullptr;
}

// 0x026b15c0 (456 bytes)
// Out-of-space callback for immediate `CGComputeContextGnm`.
bool CGComputeContextGnm_ImmediateDcbFullCallback(
    uint8_t* dcb,
    uint32_t /*sizeInDwords*/,
    uint8_t* self)
{
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0x50);
    void* curSlot = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d3d40ULL))(
        labelPool, *reinterpret_cast<void**>(dcb + 0x00));
    uint64_t poolQwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);

    void* newDcb = nullptr;
    while (reinterpret_cast<int32_t(*)(void*, void**)>(GX_VMA(0x26d3b80ULL))(labelPool, &newDcb) != 0) {
        reinterpret_cast<void(*)()>(GX_VMA(0x207f970ULL))();
    }

    uint64_t dcbDwords = ((poolQwords << 3) - 8ULL) >> 2;
    uint32_t cap = *reinterpret_cast<uint32_t*>(self + 0xd0);
    size_t chunkAllocBytes = static_cast<size_t>(cap) * 8ULL + 8ULL;

    uint8_t* tailChunk = nullptr;
    uint32_t idx = 0;
    if (*reinterpret_cast<void**>(self + 0xc0) == nullptr) {
        IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb8);
        tailChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, chunkAllocBytes, 8));
        if (!tailChunk) {
            return false;
        }
        *reinterpret_cast<uint64_t*>(tailChunk + static_cast<size_t>(cap) * 8ULL) = 0;
        *reinterpret_cast<uint8_t**>(self + 0xc0) = tailChunk;
        *reinterpret_cast<uint8_t**>(self + 0xc8) = tailChunk;
        *reinterpret_cast<uint32_t*>(self + 0xd4) = 0;
    } else {
        idx = *reinterpret_cast<uint32_t*>(self + 0xd4);
        if (idx == cap) {
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb8);
            tailChunk = static_cast<uint8_t*>(alloc->vtable->allocateAligned(alloc, chunkAllocBytes, 8));
            if (!tailChunk) {
                return false;
            }
            *reinterpret_cast<uint64_t*>(tailChunk + static_cast<size_t>(cap) * 8ULL) = 0;
            uint8_t* prevTail = *reinterpret_cast<uint8_t**>(self + 0xc8);
            *reinterpret_cast<uint8_t**>(prevTail + static_cast<size_t>(cap) * 8ULL) = tailChunk;
            *reinterpret_cast<uint8_t**>(self + 0xc8) = tailChunk;
            *reinterpret_cast<uint32_t*>(self + 0xd4) = 0;
            idx = 0;
        } else {
            tailChunk = *reinterpret_cast<uint8_t**>(self + 0xc8);
        }
    }

    *reinterpret_cast<uint32_t*>(self + 0xd4) = idx + 1u;
    void** slotDst = reinterpret_cast<void**>(tailChunk + static_cast<size_t>(idx) * 8ULL);
    if (!slotDst) {
        return false;
    }
    *slotDst = curSlot;

    void* submitAddr = *reinterpret_cast<void**>(self + 0x00);
    uint32_t submitBytes = (*reinterpret_cast<uint32_t*>(self + 0x10) -
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(submitAddr))) & ~3u;
    reinterpret_cast<int32_t(*)(void*, uint32_t, void**, uint32_t*)>(GX_VMA(0x108c830ULL))(
        self + 0x58, 1u, &submitAddr, &submitBytes);

    *reinterpret_cast<uint64_t*>(self + 0x00) = *reinterpret_cast<uint64_t*>(self + 0x10);
    *reinterpret_cast<void**>(dcb + 0x00) = newDcb;
    *reinterpret_cast<void**>(dcb + 0x10) = newDcb;
    *reinterpret_cast<uintptr_t*>(dcb + 0x08) = reinterpret_cast<uintptr_t>(newDcb) + (dcbDwords << 2);
    return true;
}

// 0x026b1790 (17 bytes)
// Returns the usable byte capacity `(poolQwords * 8) - 8` of a compute DCB chunk from `+0x50`.
uint64_t CGComputeContextGnm_GetCommandChunkChain(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics240CGComputeContextGnm_GetCommandChunkChainEPh");
uint64_t CGComputeContextGnm_GetCommandChunkChain(uint8_t* self) {
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0x50);
    uint64_t qwords = *reinterpret_cast<uint64_t*>(labelPool + 0x08);
    return (qwords << 3) - 8ULL;
}

void CGComputeContextGnm_RecordDeferredCommandChunk(uint8_t* self); // 0x026b18c0

// 0x026b17b0 (263 bytes)
// Releases recorded compute label slots in `self + 0xc0` via `0x26d3cb0` and frees the chunk chain.
void CGComputeContextGnm_DeferredDcbFullCallback(void* dcb, uint32_t /*sizeInDwords*/, void* /*userData*/) {
    uint8_t* self = static_cast<uint8_t*>(dcb);
    uint8_t* chunk = *reinterpret_cast<uint8_t**>(self + 0xc0);
    if (!chunk) {
        return;
    }

    uint64_t cap = *reinterpret_cast<uint32_t*>(self + 0xd0);
    while (chunk) {
        uint8_t* nextChunk = *reinterpret_cast<uint8_t**>(chunk + cap * 8ULL);
        uint64_t limit = nextChunk ? cap : *reinterpret_cast<uint32_t*>(self + 0xd4);
        for (uint64_t i = 0; i < limit; ++i) {
            reinterpret_cast<void(*)(void*, void*, void*)>(GX_VMA(0x26d3cb0ULL))(
                *reinterpret_cast<void**>(self + 0x50),
                self,
                *reinterpret_cast<void**>(chunk + i * 8ULL));
        }
        chunk = nextChunk;
    }

    if (*reinterpret_cast<IAllocator**>(self + 0xb8) != nullptr) {
        uint64_t c = *reinterpret_cast<uint32_t*>(self + 0xd0);
        uint8_t* node = *reinterpret_cast<uint8_t**>(self + 0xc0);
        while (node) {
            uint8_t* next = *reinterpret_cast<uint8_t**>(node + c * 8ULL);
            IAllocator* alloc = *reinterpret_cast<IAllocator**>(self + 0xb8);
            alloc->vtable->deallocate(alloc, node);
            node = next;
        }
    }
    *reinterpret_cast<uint32_t*>(self + 0xd4) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xc0) = 0;
}

// 0x026b18c0 (99 bytes)
// Submits the current compute DCB range `[+0x00, +0x10)` to the mapped `ComputeQueue` (`+0x58`).
void CGComputeContextGnm_RecordDeferredCommandChunk(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics246CGComputeContextGnm_RecordDeferredCommandChunkEPhj");
void CGComputeContextGnm_RecordDeferredCommandChunk(uint8_t* self) {
    void* submitAddr = *reinterpret_cast<void**>(self + 0x00);
    uint32_t submitBytes = (*reinterpret_cast<uint32_t*>(self + 0x10) -
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(submitAddr))) & ~3u;
    reinterpret_cast<int32_t(*)(void*, uint32_t, void**, uint32_t*)>(GX_VMA(0x108c830ULL))(
        self + 0x58, 1u, &submitAddr, &submitBytes);
    *reinterpret_cast<uint64_t*>(self + 0x00) = *reinterpret_cast<uint64_t*>(self + 0x10);
}

// 0x026b1930 (96 bytes)
// Increments the DingDong counter at `self + 0x40` and emits a releaseMem EoP write (`0x108b0f0`).
void CGComputeContextGnm_ResetCmdChunkHead(uint8_t* self)
    __asm__("_ZN4from13CoreGraphics237CGComputeContextGnm_ResetCmdChunkHeadEPhS1_");
void CGComputeContextGnm_ResetCmdChunkHead(uint8_t* self) {
    uint8_t* dingDong = *reinterpret_cast<uint8_t**>(self + 0x40);
    uint32_t val = reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x207ebd0ULL))(
        *reinterpret_cast<void**>(dingDong + 0x18));
    if (static_cast<uint64_t>(val) == *reinterpret_cast<uint64_t*>(dingDong + 0x28)) {
        val = reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x207ebd0ULL))(
            *reinterpret_cast<void**>(dingDong + 0x18));
    }
    reinterpret_cast<void(*)(void*, uint32_t, uint32_t, void*, uint32_t, uint32_t, uint32_t, uint32_t)>(
        GX_VMA(0x108b0f0ULL))(
        self, 0x2Fu, 0u, *reinterpret_cast<void**>(dingDong + 0x10), 1u, val, 0x38u, 0u);
}

// 0x026b1990 (228 bytes)
// Suballocates `sizeInBytes` with power-of-two `alignmentInBytes` downwards from
// the top of the current compute DCB chunk (`*(uintptr_t*)(self + 0x08)`).
void* CGComputeContextGnm_AllocateScratchFromDcb(
    uint8_t* self,
    size_t sizeInBytes,
    size_t alignmentInBytes)
    __asm__("_ZN4from13CoreGraphics242CGComputeContextGnm_AllocateScratchFromDcbEPhjj");
void* CGComputeContextGnm_AllocateScratchFromDcb(
    uint8_t* self,
    size_t sizeInBytes,
    size_t alignmentInBytes)
{
    if (*reinterpret_cast<uint64_t*>(self + 0x00) == 0) {
        (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
            self, 0, *reinterpret_cast<void**>(self + 0x20));
        if (*reinterpret_cast<uint64_t*>(self + 0x00) == 0) {
            return nullptr;
        }
    }

    size_t alignMask = alignmentInBytes - 1ULL;
    size_t dwords    = (sizeInBytes + 3ULL) >> 2;
    size_t byteWords = dwords << 2;

    uintptr_t top = *reinterpret_cast<uintptr_t*>(self + 0x08);
    uint8_t* labelPool = *reinterpret_cast<uint8_t**>(self + 0x50);
    size_t pad    = (top - byteWords) & alignMask;
    size_t needed = pad + byteWords;

    size_t maxChunkBytes = (*reinterpret_cast<uint64_t*>(labelPool + 0x08) << 3) - 8ULL;
    if (needed >= maxChunkBytes) {
        return nullptr;
    }

    size_t neededWords = (needed >> 2) << 2;
    if ((top - neededWords) <= *reinterpret_cast<uintptr_t*>(self + 0x10)) {
        (*reinterpret_cast<void(**)(void*, uint32_t, void*)>(self + 0x18))(
            self, 0, *reinterpret_cast<void**>(self + 0x20));
        top = *reinterpret_cast<uintptr_t*>(self + 0x08);
        pad = (top - byteWords) & alignMask;
        needed = pad + byteWords;
        neededWords = (needed >> 2) << 2;
        if ((top - neededWords) <= *reinterpret_cast<uintptr_t*>(self + 0x10)) {
            return nullptr;
        }
    }

    uintptr_t res = top - neededWords;
    *reinterpret_cast<uintptr_t*>(self + 0x08) = res;
    return reinterpret_cast<void*>(res);
}

// 0x026b1a80 (272 bytes)
// Suballocates an 8-byte aligned label from the compute DCB, emits a `releaseMem`
// event (`0x108b0f0`), and waits on the label value (`1`) via `0x108b430`.
void CGComputeContextGnm_WriteReleaseMemEventAndWait(
    uint8_t* self,
    uint32_t eventType,
    uint32_t cacheAction)
    __asm__("_ZN4from13CoreGraphics247CGComputeContextGnm_WriteReleaseMemEventAndWaitEPh");
void CGComputeContextGnm_WriteReleaseMemEventAndWait(
    uint8_t* self,
    uint32_t eventType,
    uint32_t cacheAction)
{
    uint64_t* labelPtr = static_cast<uint64_t*>(
        CGComputeContextGnm_AllocateScratchFromDcb(self, 8, 8));
    if (!labelPtr) {
        return;
    }
    *labelPtr = 0;

    reinterpret_cast<void(*)(void*, uint32_t, uint32_t, void*, uint32_t, uint32_t, uint32_t, uint32_t)>(
        GX_VMA(0x108b0f0ULL))(
        self, eventType, 0u, labelPtr, 2u, 1u, cacheAction, 0u);
    reinterpret_cast<void(*)(void*, void*, uint32_t, uint32_t, uint32_t)>(
        GX_VMA(0x108b430ULL))(
        self, labelPtr, 0xFFFFFFFFu, 3u, 1u);
}

// 0x026b1b90 (27 bytes)
// Returns the available aligned bytes in the current compute DCB chunk.
int64_t CGComputeContextGnm_GetAvailableDcbBytesAligned(uint8_t* self, size_t alignment)
    __asm__("_ZN4from13CoreGraphics247CGComputeContextGnm_GetAvailableDcbBytesAlignedEPh");
int64_t CGComputeContextGnm_GetAvailableDcbBytesAligned(uint8_t* self, size_t alignment) {
    uintptr_t top = *reinterpret_cast<uintptr_t*>(self + 0x08);
    uintptr_t cur = *reinterpret_cast<uintptr_t*>(self + 0x10);
    uintptr_t alignedCur = (cur + alignment - 1ULL) & ~(alignment - 1ULL);
    return static_cast<int64_t>((top - 4ULL) - alignedCur);
}

// ============================================================================
// 4. Embedded Utility Shaders & GPU Copy / DMA Helpers (0x26b1c40 .. 0x26b2500)
// ============================================================================

#undef g_EmbeddedShadersRefCount
#define g_EmbeddedShadersRefCount (*reinterpret_cast<uint32_t*>(GX_VMA(0x546b9e8ULL)))
#undef g_EmbeddedCS_BufferCopy
#define g_EmbeddedCS_BufferCopy (reinterpret_cast<uint8_t*>(GX_VMA(0x546b8c8ULL)))
#undef g_EmbeddedCS_ScatterCopy
#define g_EmbeddedCS_ScatterCopy (reinterpret_cast<uint8_t*>(GX_VMA(0x546b9f0ULL)))
#undef g_EmbeddedCS_Copy1D
#define g_EmbeddedCS_Copy1D (reinterpret_cast<uint8_t*>(GX_VMA(0x546bb10ULL)))
#undef g_EmbeddedCS_Copy2D
#define g_EmbeddedCS_Copy2D (reinterpret_cast<uint8_t*>(GX_VMA(0x546bc30ULL)))
#undef g_EmbeddedCS_Copy3D
#define g_EmbeddedCS_Copy3D (reinterpret_cast<uint8_t*>(GX_VMA(0x546bd50ULL)))
#undef g_EmbeddedCS_ClearRaw
#define g_EmbeddedCS_ClearRaw (reinterpret_cast<uint8_t*>(GX_VMA(0x546be70ULL)))
#undef g_EmbeddedCS_ClearUint4
#define g_EmbeddedCS_ClearUint4 (reinterpret_cast<uint8_t*>(GX_VMA(0x546bf90ULL)))
#undef g_EmbeddedCS_ClearHtile
#define g_EmbeddedCS_ClearHtile (reinterpret_cast<uint8_t*>(GX_VMA(0x546c0b0ULL)))
#undef g_EmbeddedCS_Upload1D
#define g_EmbeddedCS_Upload1D (reinterpret_cast<uint8_t*>(GX_VMA(0x546c1d0ULL)))
#undef g_EmbeddedCS_Upload2D
#define g_EmbeddedCS_Upload2D (reinterpret_cast<uint8_t*>(GX_VMA(0x546c2f0ULL)))
#undef g_EmbeddedCS_Upload3D
#define g_EmbeddedCS_Upload3D (reinterpret_cast<uint8_t*>(GX_VMA(0x546c410ULL)))
#undef g_EmbeddedVS_Fullscreen
#define g_EmbeddedVS_Fullscreen (reinterpret_cast<uint8_t*>(GX_VMA(0x546c530ULL)))
#undef g_EmbeddedPS_CopyTex2D
#define g_EmbeddedPS_CopyTex2D (reinterpret_cast<uint8_t*>(GX_VMA(0x546c658ULL)))
#undef g_EmbeddedPS_MsaaResolve
#define g_EmbeddedPS_MsaaResolve (reinterpret_cast<uint8_t*>(GX_VMA(0x546c778ULL)))

extern int32_t CGShaderFactoryGnm_CreateComputeShader(
    void* shaderObj, void* alloc, const void* sbBinary); // 0x026ba220
extern int32_t CGShaderFactoryGnm_CreateVertexShader(
    void* shaderObj, void* alloc, const void* sbBinary); // 0x026ba100
extern int32_t CGShaderFactoryGnm_CreatePixelShader(
    void* shaderObj, void* alloc, const void* sbBinary); // 0x026ba190
extern int32_t CGShaderFactoryGnm_DestroyComputeShader(void* shaderObj); // 0x026ba870
extern int32_t CGShaderFactoryGnm_DestroyVertexShader(void* shaderObj);  // 0x026ba810
extern int32_t CGShaderFactoryGnm_DestroyPixelShader(void* shaderObj);   // 0x026ba840

extern void CGGpuBlitAndClearPassesGnm_CopyBufferCs(
    void* ctx, void* dstAddr, const void* srcAddr, uint32_t sizeBytes,
    uint8_t* syncFlags); // 0x026b8c40

// 0x026b1c40 (1002 bytes)
void CGGpuCopyGnm_UploadCpuToGpuViaComputeShader(
    uint8_t* ctx,
    uint8_t* dstGpuAddr,
    const uint8_t* srcCpuAddr,
    uint64_t sizeInBytes,
    uint32_t dstSwizzle)
    __asm__("_ZN4from13CoreGraphics243CGGpuCopyGnm_UploadCpuToGpuViaComputeShaderEPPhS1_PKhjh");
void CGGpuCopyGnm_UploadCpuToGpuViaComputeShader(
    uint8_t* ctx,
    uint8_t* dstGpuAddr,
    const uint8_t* srcCpuAddr,
    uint64_t sizeInBytes,
    uint32_t dstSwizzle)
{
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_UploadCpuToGpu)) {
        if (!ctx || !dstGpuAddr || !srcCpuAddr || sizeInBytes == 0) {
            return;
        }
        while (sizeInBytes != 0) {
            const uint64_t availA = reinterpret_cast<uint64_t(*)(void*)>(GX_VMA(0x26af6e0ULL))(ctx);
            const uint64_t usableA = (availA > 0x100ULL) ? ((availA - 0x100ULL) & ~0x3FULL) : 0ULL;
            const int64_t availB = CGGfxContextGnm_GetAvailableDcbBytesAligned(ctx, 0x40);
            const uint64_t usableB = (availB > 0x100) ? (static_cast<uint64_t>(availB - 0x100) & ~0x3FULL) : 0ULL;

            uint64_t targetChunk = (usableA < sizeInBytes) ? usableA : sizeInBytes;
            if (targetChunk < 0x10ULL) {
                targetChunk = 0x10ULL;
            }
            uint64_t reqAlloc = targetChunk;
            if (usableB >= 0x10ULL && usableB < reqAlloc) {
                reqAlloc = usableB;
            }
            uint8_t* scratch = static_cast<uint8_t*>(
                reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(GX_VMA(0x26b07f0ULL))(ctx, reqAlloc, 0x40));
            if (scratch == nullptr) {
                break;
            }
            const uint64_t copyBytes = (sizeInBytes < reqAlloc) ? sizeInBytes : reqAlloc;
            std::memcpy(scratch, srcCpuAddr, copyBytes);
            gx_direct_vk::EmitCopyBufferFromCpu(
                ctx,
                reinterpret_cast<uint64_t>(dstGpuAddr),
                reinterpret_cast<uint64_t>(scratch),
                copyBytes);
            dstGpuAddr  += copyBytes;
            srcCpuAddr  += copyBytes;
            reinterpret_cast<void(*)(void*)>(GX_VMA(0x26af700ULL))(ctx);
            sizeInBytes -= copyBytes;
        }
        return;
    }
    if (sizeInBytes != 0) {
        const int32_t swzX = static_cast<int32_t>(dstSwizzle) >> 16;
        const uint32_t swzY = (dstSwizzle >> 8) & 0xFFu;
        const uint32_t swzZ = dstSwizzle & 0xFFu;

        while (sizeInBytes != 0) {
            const uint64_t reqWithCb = sizeInBytes + 0x10ULL;

            const uint64_t availA = reinterpret_cast<uint64_t(*)(void*)>(GX_VMA(0x26af6e0ULL))(ctx);
            uint64_t usableA = (availA > 0x100ULL) ? ((availA - 0x100ULL) & ~0x3FULL) : 0ULL;

            const int64_t availB = CGGfxContextGnm_GetAvailableDcbBytesAligned(ctx, 0x40);
            uint64_t usableB = (availB > 0x100) ? (static_cast<uint64_t>(availB - 0x100) & ~0x3FULL) : 0ULL;

            if (usableA > reqWithCb) {
                usableA = reqWithCb;
            }
            if (usableA < 0x20ULL) {
                usableA = 0x20ULL;
            }

            uint64_t allocSize = usableA;
            if (usableB >= 0x20ULL && usableB < usableA) {
                allocSize = usableB;
            }

            uint8_t* scratch = static_cast<uint8_t*>(
                reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(GX_VMA(0x26b07f0ULL))(ctx, allocSize, 0x40));
            if (scratch == nullptr) {
                allocSize = 0;
                break;
            }

            reinterpret_cast<void(*)(void*, void*, const void*)>(GX_VMA(0x26cd400ULL))(
                ctx + 0x40, ctx, g_EmbeddedCS_BufferCopy);

            uint32_t copyBytes = static_cast<uint32_t>(allocSize - 0x10ULL);
            if (static_cast<uint32_t>(sizeInBytes) < copyBytes) {
                copyBytes = static_cast<uint32_t>(sizeInBytes);
            }

            std::memcpy(scratch, srcCpuAddr, copyBytes);

            const uint32_t numDwords = copyBytes >> 2;

            uint64_t srvVSharp[2] = {};
            reinterpret_cast<void(*)(void*, const void*, uint32_t, uint32_t)>(GX_VMA(0x10707c0ULL))(
                srvVSharp, scratch, 0x4404u, numDwords);
            reinterpret_cast<void(*)(void*, int32_t, uint32_t, uint32_t)>(GX_VMA(0x10710c0ULL))(
                srvVSharp, 0x6E, 0, 1);

            uint64_t uavVSharp[2] = {};
            reinterpret_cast<void(*)(void*, const void*, uint32_t, uint32_t)>(GX_VMA(0x10707c0ULL))(
                uavVSharp, dstGpuAddr, 0x4404u, numDwords);
            reinterpret_cast<void(*)(void*, int32_t, uint32_t, uint32_t)>(GX_VMA(0x10710c0ULL))(
                uavVSharp, swzX, swzY, swzZ);

            *reinterpret_cast<uint64_t*>(ctx + 0x7920) = srvVSharp[0];
            *reinterpret_cast<uint64_t*>(ctx + 0x7928) = srvVSharp[1];
            *reinterpret_cast<uint64_t*>(ctx + 0x1fb0) |= *reinterpret_cast<uint64_t*>(ctx + 0x1ff8);
            ctx[0x1f98] |= 0x01u;

            *reinterpret_cast<uint64_t*>(ctx + 0x9320) = uavVSharp[0];
            *reinterpret_cast<uint64_t*>(ctx + 0x9328) = uavVSharp[1];
            *reinterpret_cast<uint64_t*>(ctx + 0x1fa8) |= (1ULL << 36);
            *reinterpret_cast<uint64_t*>(ctx + 0x1fb0) |= *reinterpret_cast<uint64_t*>(ctx + 0x2518);

            uint32_t* cbData = reinterpret_cast<uint32_t*>(scratch + allocSize - 0x10ULL);
            cbData[0] = numDwords;
            cbData[1] = numDwords;
            cbData[2] = numDwords;
            cbData[3] = numDwords;

            uint64_t cbVSharp[2] = {};
            reinterpret_cast<void(*)(void*, const void*, uint32_t)>(GX_VMA(0x1070960ULL))(
                cbVSharp, cbData, 0x10u);
            reinterpret_cast<void(*)(void*, int32_t, uint32_t, uint32_t)>(GX_VMA(0x10710c0ULL))(
                cbVSharp, 0x6E, 0, 0);

            *reinterpret_cast<uint64_t*>(ctx + 0xa160) = cbVSharp[0];
            *reinterpret_cast<uint64_t*>(ctx + 0xa168) = cbVSharp[1];
            *reinterpret_cast<uint64_t*>(ctx + 0x1fa8) |= 0x10000ULL;
            *reinterpret_cast<uint64_t*>(ctx + 0x1fb0) |= *reinterpret_cast<uint64_t*>(ctx + 0x2478);

            const uint32_t groupsX = (numDwords + 63u) >> 6;
            reinterpret_cast<void(*)(void*, void*, const void*)>(GX_VMA(0x26cd5d0ULL))(
                ctx + 0x40, ctx, g_EmbeddedCS_BufferCopy);
            reinterpret_cast<void(*)(void*, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x1074c50ULL))(
                ctx, groupsX, 1, 1);
            reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26ce110ULL))(
                ctx + 0x40, ctx);
            reinterpret_cast<void(*)(void*)>(GX_VMA(0x26af700ULL))(ctx);

            dstGpuAddr  += copyBytes;
            srcCpuAddr  += copyBytes;
            sizeInBytes -= copyBytes;
        }
    }

    ctx[0x1f98] |= 0x01u;
    *reinterpret_cast<uint64_t*>(ctx + 0x1fa8) |= 0x1000010000ULL;
}

// 0x026b2030 (714 bytes)
// Initializes all 14 built-in engine utility shaders on first reference (`0x546b9e8`).
int32_t CGEmbeddedShadersGnm_InitializeAll(void* arg0, void* arg1, void* arg2)
    __asm__("_ZN4from13CoreGraphics234CGEmbeddedShadersGnm_InitializeAllEPNS0_10IAllocatorE");
int32_t CGEmbeddedShadersGnm_InitializeAll(void* arg0, void* arg1, void* arg2) {
    uint32_t prevRef = reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x207ebd0ULL))(
        reinterpret_cast<void*>(GX_VMA(0x546b9e8ULL)));
    if (prevRef != 0) {
        return 0;
    }

    using InitShaderFn = int32_t(*)(void*, void*, const void*, int64_t, void*, void*);
    auto initCs = reinterpret_cast<InitShaderFn>(GX_VMA(0x2172db0ULL));
    auto initVs = reinterpret_cast<InitShaderFn>(GX_VMA(0x2177a40ULL));
    auto initPs = reinterpret_cast<InitShaderFn>(GX_VMA(0x2175280ULL));

    int32_t err = 0;
    if ((err = initCs(g_EmbeddedCS_BufferCopy,  arg0, reinterpret_cast<const void*>(GX_VMA(0x52bb300ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bb418ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_ScatterCopy, arg0, reinterpret_cast<const void*>(GX_VMA(0x52ba390ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52ba4f8ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_Copy1D,      arg0, reinterpret_cast<const void*>(GX_VMA(0x52bb420ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bb64cULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_Copy2D,      arg0, reinterpret_cast<const void*>(GX_VMA(0x52bb650ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bb89cULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_Copy3D,      arg0, reinterpret_cast<const void*>(GX_VMA(0x52bb8a0ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bbaf4ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_ClearRaw,    arg0, reinterpret_cast<const void*>(GX_VMA(0x52ba500ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52ba69cULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_ClearUint4,  arg0, reinterpret_cast<const void*>(GX_VMA(0x52ba860ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52ba9c4ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_ClearHtile,  arg0, reinterpret_cast<const void*>(GX_VMA(0x52ba6a0ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52ba858ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_Upload1D,    arg0, reinterpret_cast<const void*>(GX_VMA(0x52ba9d0ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52babf0ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_Upload2D,    arg0, reinterpret_cast<const void*>(GX_VMA(0x52bac00ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52baf00ULL)), arg1, arg2)) != 0) return err;
    if ((err = initCs(g_EmbeddedCS_Upload3D,    arg0, reinterpret_cast<const void*>(GX_VMA(0x52baf10ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bb2f0ULL)), arg1, arg2)) != 0) return err;
    if ((err = initVs(g_EmbeddedVS_Fullscreen,  arg0, reinterpret_cast<const void*>(GX_VMA(0x52bbde0ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bbefcULL)), arg1, arg2)) != 0) return err;
    if ((err = initPs(g_EmbeddedPS_CopyTex2D,   arg0, reinterpret_cast<const void*>(GX_VMA(0x52bbcf0ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bbdd8ULL)), arg1, arg2)) != 0) return err;
    if ((err = initPs(g_EmbeddedPS_MsaaResolve, arg0, reinterpret_cast<const void*>(GX_VMA(0x52bbb00ULL)), *reinterpret_cast<const int32_t*>(GX_VMA(0x52bbce4ULL)), arg1, arg2)) != 0) return err;
    return 0;
}

// 0x026b2300 (196 bytes)
// Atomically decrements `0x546b9e8` (`0x207ebf0`) and destroys all 14 built-in utility shaders when previous value was 1.
void CGEmbeddedShadersGnm_ReleaseAll()
    __asm__("_ZN4from13CoreGraphics231CGEmbeddedShadersGnm_ReleaseAllEv");
void CGEmbeddedShadersGnm_ReleaseAll() {
    uint32_t prevRef = reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x207ebf0ULL))(
        reinterpret_cast<void*>(GX_VMA(0x546b9e8ULL)));
    if (prevRef != 1u) {
        return;
    }

    auto destroyPs = reinterpret_cast<void(*)(void*)>(GX_VMA(0x21753b0ULL));
    auto destroyVs = reinterpret_cast<void(*)(void*)>(GX_VMA(0x2177c50ULL));
    auto destroyCs = reinterpret_cast<void(*)(void*)>(GX_VMA(0x2172ee0ULL));

    destroyPs(g_EmbeddedPS_MsaaResolve);
    destroyPs(g_EmbeddedPS_CopyTex2D);
    destroyVs(g_EmbeddedVS_Fullscreen);
    destroyCs(g_EmbeddedCS_Upload3D);
    destroyCs(g_EmbeddedCS_Upload2D);
    destroyCs(g_EmbeddedCS_Upload1D);
    destroyCs(g_EmbeddedCS_ClearUint4);
    destroyCs(g_EmbeddedCS_ClearHtile);
    destroyCs(g_EmbeddedCS_ClearRaw);
    destroyCs(g_EmbeddedCS_Copy3D);
    destroyCs(g_EmbeddedCS_Copy2D);
    destroyCs(g_EmbeddedCS_Copy1D);
    destroyCs(g_EmbeddedCS_ScatterCopy);
    destroyCs(g_EmbeddedCS_BufferCopy);
}

// 0x026b23d0 (301 bytes)
// Copies `sizeInBytes` from `srcGpuAddr` to `dstGpuAddr` in `0x1FFFFC`-byte chunks
// using CP DMA (`0x01073e60`).
void CGGpuCopyGnm_CopyGpuToGpuDmaLoop(
    uint8_t* gfxCtx,
    uint8_t* dstGpuAddr,
    const uint8_t* srcGpuAddr,
    size_t sizeInBytes,
    uint8_t isBlocking)
    __asm__("_ZN4from13CoreGraphics232CGGpuCopyGnm_CopyGpuToGpuDmaLoopEPPhS1_PKhj");
void CGGpuCopyGnm_CopyGpuToGpuDmaLoop(
    uint8_t* gfxCtx,
    uint8_t* dstGpuAddr,
    const uint8_t* srcGpuAddr,
    size_t sizeInBytes,
    uint8_t isBlocking)
{
    if (!dstGpuAddr || !srcGpuAddr || sizeInBytes == 0) {
        return;
    }

    auto dmaData = reinterpret_cast<void(*)(void*, uint32_t, void*, uint32_t, const void*, uint32_t, uint32_t)>(
        GX_VMA(0x1073e60ULL));

    while (sizeInBytes > 0x1FFFFCu) {
        dmaData(gfxCtx, 0, dstGpuAddr, 0, srcGpuAddr, 0x1FFFFCu, 0);
        dstGpuAddr  += 0x1FFFFCu;
        srcGpuAddr  += 0x1FFFFCu;
        sizeInBytes -= 0x1FFFFCu;
    }
    dmaData(gfxCtx, 0, dstGpuAddr, 0, srcGpuAddr, static_cast<uint32_t>(sizeInBytes), isBlocking);
}

// 0x026b2500 (354 bytes)
// Copies `sizeInBytes` from CPU `srcCpuAddr` to GPU `dstGpuAddr` by suballocating
// 64-byte aligned chunks from the DCB scratch ring, `memcpy`ing the CPU payload,
// issuing CP DMA (`0x01073e60`), and flushing chunks via `0x26af700`.
void CGGpuCopyGnm_CopyCpuToGpuViaDynamicScratch(
    uint8_t* gfxCtx,
    uint8_t* dstGpuAddr,
    const uint8_t* srcCpuAddr,
    size_t sizeInBytes,
    uint8_t isBlocking)
    __asm__("_ZN4from13CoreGraphics242CGGpuCopyGnm_CopyCpuToGpuViaDynamicScratchEPPhS1_PKhj");
void CGGpuCopyGnm_CopyCpuToGpuViaDynamicScratch(
    uint8_t* gfxCtx,
    uint8_t* dstGpuAddr,
    const uint8_t* srcCpuAddr,
    size_t sizeInBytes,
    uint8_t isBlocking)
{
    if (!dstGpuAddr || !srcCpuAddr || sizeInBytes == 0) {
        return;
    }

    auto dmaData = reinterpret_cast<void(*)(void*, uint32_t, void*, uint32_t, const void*, uint32_t, uint32_t)>(
        GX_VMA(0x1073e60ULL));

    while (sizeInBytes != 0) {
        size_t reqCap = (sizeInBytes < 0x1FFFFCu) ? sizeInBytes : 0x1FFFFCu;

        uint64_t maxChunkCap = CGGfxContextGnm_FreeStagingHashMap(gfxCtx);
        size_t maxTransfer = (maxChunkCap > 0x1Cu) ? static_cast<size_t>((maxChunkCap - 0x1Cu) & ~0x3FULL) : 0ULL;

        int64_t availNow = CGGfxContextGnm_GetAvailableDcbBytesAligned(gfxCtx, 0x40);
        size_t availTransfer = (availNow > 0x1C) ? (static_cast<size_t>(availNow - 0x1C) & ~0x3FULL) : 0ULL;

        size_t targetChunk = (maxTransfer < reqCap) ? maxTransfer : reqCap;
        if (targetChunk < 0x10ULL) {
            targetChunk = 0x10ULL;
        }

        void* scratch = nullptr;
        size_t actualChunk = 0;
        if (availTransfer >= 0x10ULL && availTransfer < targetChunk) {
            scratch = CGGfxContextGnm_AllocateFromCommandBufferAligned(gfxCtx, availTransfer, 0x40);
            actualChunk = scratch ? availTransfer : 0ULL;
        } else {
            scratch = CGGfxContextGnm_AllocateFromCommandBufferAligned(gfxCtx, targetChunk, 0x40);
            actualChunk = scratch ? targetChunk : 0ULL;
        }
        if (!scratch || actualChunk == 0ULL) {
            break;
        }

        std::memcpy(scratch, srcCpuAddr, actualChunk);
        uint32_t blockFlag = (sizeInBytes == actualChunk) ? (isBlocking & 1u) : 0u;
        dmaData(gfxCtx, 0, dstGpuAddr, 0, scratch, static_cast<uint32_t>(actualChunk), blockFlag);

        dstGpuAddr  += actualChunk;
        srcCpuAddr  += actualChunk;
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x26af700ULL))(gfxCtx);
        sizeInBytes -= actualChunk;
    }
}

} // namespace CoreGraphics2
} // namespace from
