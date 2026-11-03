// ============================================================================
// Bloodborne (CUSA03173 v1.09) - CoreGraphics2 GNM Backend
// File: CGCommandStreamRecorderAndPlayerGnm.cpp
// Address Range: 0x026aac90 .. 0x026adb80 (89 functions)
// Subsystem:
//   1. CGResourceStagingHashMapGnm Lifecycle (0x26ada80 .. 0x26adb80, 5 functions)
//      - Constructor, copy-constructor, chunk deallocator, initializer, and reset
//   2. CGCommandStreamRecorderGnm (0x26ab7b0 .. 0x26ad970, 52 functions)
//      - Records high-level CoreGraphics2 state & draw commands into a linked list
//        of 0x610-byte chunks (opcodes 0x00 .. 0x24) and bakes them into a
//        contiguous 16-byte aligned command buffer (`0x26ab900`)
//   3. CGCommandStreamPlayerGnm (0x26aac90 .. 0x26ab7a0, 32 functions)
//      - Replays baked 16-byte aligned command streams (`0x26aac90`) via dispatch
//        table `0x052b9fb0` against `CGContextStateShadowGnm` (`0x02168c90 .. 0x02169ba0`)
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

// ============================================================================
// 1. CGResourceStagingHashMapGnm Lifecycle (0x26ada80 .. 0x26adb80)
// ============================================================================

struct CGResourceStagingHashMapGnm {
    IAllocator* allocator;        // +0x00
    void**      chunkTable;       // +0x08
    uint32_t    entryCount;       // +0x10
    uint32_t    pad_14;           // +0x14
    uint16_t    bucketCount;      // +0x18
};

// 0x026ada80 (29 bytes)
void CGResourceStagingHashMapGnm_Construct(CGResourceStagingHashMapGnm* self) {
    self->entryCount  = 0;
    self->chunkTable  = nullptr;
    self->allocator   = nullptr;
    self->bucketCount = 0x100;
}

// 0x026adaa0 (31 bytes)
void CGResourceStagingHashMapGnm_CopyConstruct(
    CGResourceStagingHashMapGnm* dst,
    const CGResourceStagingHashMapGnm* src)
{
    *reinterpret_cast<uint64_t*>(&dst->entryCount) =
        *reinterpret_cast<const uint64_t*>(&src->entryCount);
    dst->allocator   = src->allocator;
    dst->chunkTable  = src->chunkTable;
    dst->bucketCount = src->bucketCount;
}

// 0x026adac0 (134 bytes)
void CGResourceStagingHashMapGnm_FreeAll(CGResourceStagingHashMapGnm* self) {
    IAllocator* alloc = self->allocator;
    if (!alloc) {
        return;
    }
    uint32_t count = self->entryCount;
    if (count == 0) {
        return;
    }
    uint32_t numChunks = (count + 0x7Fu) >> 7;
    if (self->chunkTable) {
        if (numChunks != 0) {
            for (uint32_t i = 0; i < numChunks; ++i) {
                alloc->vtable->deallocate(self->allocator, self->chunkTable[i]);
            }
        }
        self->allocator->vtable->deallocate(self->allocator, self->chunkTable);
        self->chunkTable = nullptr;
    }
    self->entryCount = 0;
}

// 0x026adb50 (37 bytes)
int32_t CGResourceStagingHashMapGnm_Initialize(
    CGResourceStagingHashMapGnm* self,
    IAllocator* alloc,
    uint16_t bucketCount)
{
    self->allocator  = alloc;
    self->chunkTable = nullptr;
    self->entryCount = 0;
    if (bucketCount == 0x100) {
        return 8;
    }
    self->bucketCount = bucketCount;
    return 0;
}

// 0x026adb80 (7 bytes)
void CGResourceStagingHashMapGnm_Reset(CGResourceStagingHashMapGnm* self) {
    self->bucketCount = 0x100;
}

// ============================================================================
// 2. CGCommandStreamRecorderGnm (0x26ab7b0 .. 0x26ad970)
// ============================================================================

struct CGCommandChunkNode {
    uint64_t            usedBytes; // +0x00 (max 0x600)
    CGCommandChunkNode* next;      // +0x08
    uint8_t             data[0x600]; // +0x10
};

struct CGCommandStreamRecorderGnm {
    IAllocator*         chunkAllocator;  // +0x00
    IAllocator*         streamAllocator; // +0x08
    CGCommandChunkNode* headChunk;       // +0x10
    CGCommandChunkNode* tailChunk;       // +0x18
    uint64_t            totalBytes;      // +0x20
};

// Internal helper matching the inline chunk-extension pattern in 0x26abb00..0x26ad970
static inline uint8_t* AllocateFromCommandChunk(
    CGCommandStreamRecorderGnm* rec,
    size_t alignedPayloadBytes)
{
    CGCommandChunkNode* tail = rec->tailChunk;
    uint64_t curOffset = tail->usedBytes;
    if (curOffset + alignedPayloadBytes >= 0x601u) {
        IAllocator* alloc = rec->chunkAllocator;
        tail = static_cast<CGCommandChunkNode*>(
            alloc->vtable->allocateAligned(alloc, 0x610, 0x10));
        if (!tail) {
            return nullptr;
        }
        tail->next      = nullptr;
        tail->usedBytes = 0;
        rec->tailChunk->next = tail;
        rec->tailChunk       = tail;
        curOffset            = 0;
    }
    tail->usedBytes  = curOffset + alignedPayloadBytes;
    rec->totalBytes += alignedPayloadBytes;
    return tail->data + curOffset;
}

// 0x026ab7b0 (40 bytes)
void CGCommandStreamRecorderGnm_Construct(CGCommandStreamRecorderGnm* self) {
    self->totalBytes      = 0;
    self->tailChunk       = nullptr;
    self->headChunk       = nullptr;
    self->streamAllocator = nullptr;
    self->chunkAllocator  = nullptr;
}

// 0x026ab7e0 (1 byte)
void CGCommandStreamRecorderGnm_EmptyNoop(CGCommandStreamRecorderGnm* /*self*/) {}

// 0x026ab7f0 (39 bytes)
void CGCommandStreamRecorderGnm_CopyConstruct(
    CGCommandStreamRecorderGnm* dst,
    const CGCommandStreamRecorderGnm* src)
{
    dst->chunkAllocator  = src->chunkAllocator;
    dst->streamAllocator = src->streamAllocator;
    dst->headChunk       = src->headChunk;
    dst->tailChunk       = src->tailChunk;
    dst->totalBytes      = src->totalBytes;
}

// 0x026ab820 (94 bytes)
int32_t CGCommandStreamRecorderGnm_Initialize(
    CGCommandStreamRecorderGnm* self,
    IAllocator* chunkAlloc,
    IAllocator* streamAlloc)
{
    self->chunkAllocator  = chunkAlloc;
    self->streamAllocator = streamAlloc;
    CGCommandChunkNode* root = static_cast<CGCommandChunkNode*>(
        chunkAlloc->vtable->allocateAligned(chunkAlloc, 0x610, 0x10));
    self->headChunk = root;
    if (!root) {
        return 9;
    }
    self->tailChunk  = root;
    self->totalBytes = 0;
    root->usedBytes  = 0;
    root->next       = nullptr;
    return 0;
}

// 0x026ab880 (113 bytes)
void CGCommandStreamRecorderGnm_Finalize(CGCommandStreamRecorderGnm* self) {
    if (!self->chunkAllocator || !self->streamAllocator) {
        return;
    }
    CGCommandChunkNode* node = self->headChunk;
    while (node) {
        CGCommandChunkNode* next = node->next;
        self->chunkAllocator->vtable->deallocate(self->chunkAllocator, node);
        node = next;
    }
    self->totalBytes      = 0;
    self->tailChunk       = nullptr;
    self->headChunk       = nullptr;
    self->streamAllocator = nullptr;
    self->chunkAllocator  = nullptr;
}

// 0x026ab900 (369 bytes)
// Appends terminating opcode 0x00, allocates a contiguous buffer of `totalBytes`
// from `streamAllocator`, copies all recorded chunks into it, frees overflow
// chunks, and resets `headChunk`.
int32_t CGCommandStreamRecorderGnm_BakeContiguousStream(
    CGCommandStreamRecorderGnm* self,
    void** outStreamBuffer,
    uint64_t* outTotalBytes,
    IAllocator** outStreamAlloc)
{
    uint8_t* termCmd = AllocateFromCommandChunk(self, 0x10);
    if (!termCmd) {
        return 9;
    }
    termCmd[0] = 0x00; // End-of-stream sentinel

    uint8_t* flatBuf = static_cast<uint8_t*>(
        self->streamAllocator->vtable->allocateAligned(
            self->streamAllocator, self->totalBytes, 0x10));
    if (!flatBuf) {
        if (self->tailChunk && self->tailChunk->usedBytes >= 0x10) {
            self->tailChunk->usedBytes -= 0x10;
            self->totalBytes -= 0x10;
        }
        return 9;
    }

    uint8_t* writePtr = flatBuf;
    for (CGCommandChunkNode* node = self->headChunk; node != nullptr; node = node->next) {
        std::memcpy(writePtr, node->data, node->usedBytes);
        writePtr += node->usedBytes;
    }

    *outStreamBuffer = flatBuf;
    *outTotalBytes   = self->totalBytes;
    *outStreamAlloc  = self->streamAllocator;

    // Free all overflow chunks after headChunk
    CGCommandChunkNode* overflow = self->headChunk->next;
    while (overflow) {
        CGCommandChunkNode* next = overflow->next;
        self->chunkAllocator->vtable->deallocate(self->chunkAllocator, overflow);
        overflow = next;
    }

    self->headChunk->next      = nullptr;
    self->headChunk->usedBytes = 0;
    self->tailChunk            = self->headChunk;
    self->totalBytes           = 0;
    return 0;
}

// 0x026aba80 (120 bytes)
void* CGCommandStreamRecorderGnm_AllocateAlignedPayload(
    CGCommandStreamRecorderGnm* self,
    size_t sizeInBytes)
{
    size_t alignedSize = (sizeInBytes + 0xFu) & ~static_cast<size_t>(0xFu);
    return AllocateFromCommandChunk(self, alignedSize);
}

// --- Opcode Writers (0x01 .. 0x24) ---

// 0x026abb00 (125 bytes) - Opcode 0x01: SetRasterizerState (ptr)
void CGCommandStreamRecorderGnm_WriteOp01_SetRasterizerState(
    CGCommandStreamRecorderGnm* self, void* statePtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x01;
        *reinterpret_cast<void**>(p + 0x08) = statePtr;
    }
}

// 0x026abb80 (125 bytes) - Opcode 0x02: SetPrimitiveTopology (uint32_t)
void CGCommandStreamRecorderGnm_WriteOp02_SetPrimitiveTopology(
    CGCommandStreamRecorderGnm* self, uint32_t topology)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x02;
        *reinterpret_cast<uint32_t*>(p + 0x04) = topology;
    }
}

// 0x026abc00 (149 bytes) - Opcode 0x03: SetIndexBuffer (ptr, u32, u32)
void CGCommandStreamRecorderGnm_WriteOp03_SetIndexBuffer(
    CGCommandStreamRecorderGnm* self, void* ibPtr, uint32_t offset, uint32_t format)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x20)) {
        p[0] = 0x03;
        *reinterpret_cast<void**>(p + 0x08)    = ibPtr;
        *reinterpret_cast<uint32_t*>(p + 0x10) = offset;
        *reinterpret_cast<uint32_t*>(p + 0x14) = format;
    }
}

// 0x026abca0 (300 bytes) - Opcode 0x04: SetVertexBuffers (startSlot, count, vbPtrs, strides, offsets)
void CGCommandStreamRecorderGnm_WriteOp04_SetVertexBuffers(
    CGCommandStreamRecorderGnm* self,
    uint32_t startSlot,
    uint32_t count,
    const void* vbPtrs,
    const void* strides,
    const void* offsets)
{
    size_t alignedBytes = ((static_cast<size_t>(count) * 16u) + 0x1Bu) & ~static_cast<size_t>(0xFu);
    if (uint8_t* p = AllocateFromCommandChunk(self, alignedBytes)) {
        p[0] = 0x04;
        *reinterpret_cast<uint32_t*>(p + 0x04) = startSlot;
        *reinterpret_cast<uint32_t*>(p + 0x08) = count;
        size_t ptrBytes   = static_cast<size_t>(count) * 8u;
        size_t dwordBytes = static_cast<size_t>(count) * 4u;
        std::memcpy(p + 0x0C, vbPtrs, ptrBytes);
        std::memcpy(p + 0x0C + ptrBytes, strides, dwordBytes);
        std::memcpy(p + 0x0C + ptrBytes + dwordBytes, offsets, dwordBytes);
    }
}

// Helper for array-of-8-byte-pointers opcodes (0x05..0x20)
static inline void WritePointerArrayOpcode(
    CGCommandStreamRecorderGnm* self,
    uint8_t opcode,
    uint32_t startSlot,
    uint32_t count,
    const void* ptrArray)
{
    size_t alignedBytes = ((static_cast<size_t>(count) * 8u) + 0x1Bu) & ~static_cast<size_t>(0xFu);
    if (uint8_t* p = AllocateFromCommandChunk(self, alignedBytes)) {
        p[0] = opcode;
        *reinterpret_cast<uint32_t*>(p + 0x04) = startSlot;
        *reinterpret_cast<uint32_t*>(p + 0x08) = count;
        std::memcpy(p + 0x0C, ptrArray, static_cast<size_t>(count) * 8u);
    }
}

// 0x026abdd0 (227 bytes) - Opcode 0x05: SetVSConstantBuffers
void CGCommandStreamRecorderGnm_WriteOp05_SetVSConstantBuffers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x05, startSlot, count, ptrs);
}

// 0x026abec0 (233 bytes) - Generic Opcode + Pointer Array writer A
void CGCommandStreamRecorderGnm_WriteGenericPtrArrayOp_A(
    CGCommandStreamRecorderGnm* self, uint8_t opcode, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, opcode, startSlot, count, ptrs);
}

// 0x026abfb0 (227 bytes) - Opcode 0x06: SetVSTextures
void CGCommandStreamRecorderGnm_WriteOp06_SetVSTextures(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x06, startSlot, count, ptrs);
}

// 0x026ac0a0 (233 bytes) - Generic Opcode + Pointer Array writer B
void CGCommandStreamRecorderGnm_WriteGenericPtrArrayOp_B(
    CGCommandStreamRecorderGnm* self, uint8_t opcode, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, opcode, startSlot, count, ptrs);
}

// 0x026ac190 (227 bytes) - Opcode 0x07: SetVSSamplers
void CGCommandStreamRecorderGnm_WriteOp07_SetVSSamplers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x07, startSlot, count, ptrs);
}

// 0x026ac280 (233 bytes) - Generic Opcode + Pointer Array writer C
void CGCommandStreamRecorderGnm_WriteGenericPtrArrayOp_C(
    CGCommandStreamRecorderGnm* self, uint8_t opcode, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, opcode, startSlot, count, ptrs);
}

// 0x026ac370 (125 bytes) - Opcode 0x08: SetVertexShader
void CGCommandStreamRecorderGnm_WriteOp08_SetVertexShader(
    CGCommandStreamRecorderGnm* self, void* shaderPtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x08;
        *reinterpret_cast<void**>(p + 0x08) = shaderPtr;
    }
}

// 0x026ac3f0 (137 bytes) - Generic single-pointer opcode writer
void CGCommandStreamRecorderGnm_WriteGenericSinglePtrOp(
    CGCommandStreamRecorderGnm* self, uint8_t opcode, void* ptr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = opcode;
        *reinterpret_cast<void**>(p + 0x08) = ptr;
    }
}

// 0x026ac480 (227 bytes) - Opcode 0x09: SetPSConstantBuffers
void CGCommandStreamRecorderGnm_WriteOp09_SetPSConstantBuffers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x09, startSlot, count, ptrs);
}

// 0x026ac570 (227 bytes) - Opcode 0x0A: SetPSTextures
void CGCommandStreamRecorderGnm_WriteOp0A_SetPSTextures(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x0A, startSlot, count, ptrs);
}

// 0x026ac660 (227 bytes) - Opcode 0x0B: SetPSSamplers
void CGCommandStreamRecorderGnm_WriteOp0B_SetPSSamplers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x0B, startSlot, count, ptrs);
}

// 0x026ac750 (125 bytes) - Opcode 0x0C: SetPixelShader
void CGCommandStreamRecorderGnm_WriteOp0C_SetPixelShader(
    CGCommandStreamRecorderGnm* self, void* shaderPtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x0C;
        *reinterpret_cast<void**>(p + 0x08) = shaderPtr;
    }
}

// 0x026ac7d0 (227 bytes) - Opcode 0x0D: SetGSConstantBuffers
void CGCommandStreamRecorderGnm_WriteOp0D_SetGSConstantBuffers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x0D, startSlot, count, ptrs);
}

// 0x026ac8c0 (227 bytes) - Opcode 0x0E: SetGSTextures
void CGCommandStreamRecorderGnm_WriteOp0E_SetGSTextures(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x0E, startSlot, count, ptrs);
}

// 0x026ac9b0 (227 bytes) - Opcode 0x0F: SetGSSamplers
void CGCommandStreamRecorderGnm_WriteOp0F_SetGSSamplers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x0F, startSlot, count, ptrs);
}

// 0x026acaa0 (125 bytes) - Opcode 0x10: SetGeometryShader
void CGCommandStreamRecorderGnm_WriteOp10_SetGeometryShader(
    CGCommandStreamRecorderGnm* self, void* shaderPtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x10;
        *reinterpret_cast<void**>(p + 0x08) = shaderPtr;
    }
}

// 0x026acb20 (227 bytes) - Opcode 0x11: SetHSConstantBuffers
void CGCommandStreamRecorderGnm_WriteOp11_SetHSConstantBuffers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x11, startSlot, count, ptrs);
}

// 0x026acc10 (227 bytes) - Opcode 0x12: SetHSTextures
void CGCommandStreamRecorderGnm_WriteOp12_SetHSTextures(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x12, startSlot, count, ptrs);
}

// 0x026acd00 (227 bytes) - Opcode 0x13: SetHSSamplers
void CGCommandStreamRecorderGnm_WriteOp13_SetHSSamplers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x13, startSlot, count, ptrs);
}

// 0x026acdf0 (125 bytes) - Opcode 0x14: SetHullShader
void CGCommandStreamRecorderGnm_WriteOp14_SetHullShader(
    CGCommandStreamRecorderGnm* self, void* shaderPtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x14;
        *reinterpret_cast<void**>(p + 0x08) = shaderPtr;
    }
}

// 0x026ace70 (227 bytes) - Opcode 0x15: SetDSConstantBuffers
void CGCommandStreamRecorderGnm_WriteOp15_SetDSConstantBuffers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x15, startSlot, count, ptrs);
}

// 0x026acf60 (227 bytes) - Opcode 0x16: SetDSTextures
void CGCommandStreamRecorderGnm_WriteOp16_SetDSTextures(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x16, startSlot, count, ptrs);
}

// 0x026ad050 (227 bytes) - Opcode 0x17: SetDSSamplers
void CGCommandStreamRecorderGnm_WriteOp17_SetDSSamplers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x17, startSlot, count, ptrs);
}

// 0x026ad140 (125 bytes) - Opcode 0x18: SetDomainShader
void CGCommandStreamRecorderGnm_WriteOp18_SetDomainShader(
    CGCommandStreamRecorderGnm* self, void* shaderPtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x18;
        *reinterpret_cast<void**>(p + 0x08) = shaderPtr;
    }
}

// 0x026ad1c0 (227 bytes) - Opcode 0x19: SetCSConstantBuffers
void CGCommandStreamRecorderGnm_WriteOp19_SetCSConstantBuffers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x19, startSlot, count, ptrs);
}

// 0x026ad2b0 (227 bytes) - Opcode 0x1A: SetCSTextures
void CGCommandStreamRecorderGnm_WriteOp1A_SetCSTextures(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x1A, startSlot, count, ptrs);
}

// 0x026ad3a0 (227 bytes) - Opcode 0x1B: SetCSSamplers
void CGCommandStreamRecorderGnm_WriteOp1B_SetCSSamplers(
    CGCommandStreamRecorderGnm* self, uint32_t startSlot, uint32_t count, const void* ptrs)
{
    WritePointerArrayOpcode(self, 0x1B, startSlot, count, ptrs);
}

// 0x026ad490 (125 bytes) - Opcode 0x1C: SetComputeShader
void CGCommandStreamRecorderGnm_WriteOp1C_SetComputeShader(
    CGCommandStreamRecorderGnm* self, void* shaderPtr)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x1C;
        *reinterpret_cast<void**>(p + 0x08) = shaderPtr;
    }
}

// 0x026ad510 (149 bytes) - Opcode 0x1D: Dispatch (groupsX, groupsY, groupsZ)
void CGCommandStreamRecorderGnm_WriteOp1D_Dispatch(
    CGCommandStreamRecorderGnm* self, uint32_t x, uint32_t y, uint32_t z)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x1D;
        *reinterpret_cast<uint32_t*>(p + 0x04) = x;
        *reinterpret_cast<uint32_t*>(p + 0x08) = y;
        *reinterpret_cast<uint32_t*>(p + 0x0C) = z;
    }
}

// 0x026ad5b0 (142 bytes) - Opcode 0x1E: Draw (vertexCount, startVertex)
void CGCommandStreamRecorderGnm_WriteOp1E_Draw(
    CGCommandStreamRecorderGnm* self, uint32_t vertexCount, uint32_t startVertex)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x1E;
        *reinterpret_cast<uint32_t*>(p + 0x04) = vertexCount;
        *reinterpret_cast<uint32_t*>(p + 0x08) = startVertex;
    }
}

// 0x026ad640 (149 bytes) - Opcode 0x1F: DrawIndexed (indexCount, startIndex, baseVertex)
void CGCommandStreamRecorderGnm_WriteOp1F_DrawIndexed(
    CGCommandStreamRecorderGnm* self, uint32_t indexCount, uint32_t startIndex, uint32_t baseVertex)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x1F;
        *reinterpret_cast<uint32_t*>(p + 0x04) = indexCount;
        *reinterpret_cast<uint32_t*>(p + 0x08) = startIndex;
        *reinterpret_cast<uint32_t*>(p + 0x0C) = baseVertex;
    }
}

// 0x026ad6e0 (166 bytes) - Opcode 0x20: DrawInstanced
void CGCommandStreamRecorderGnm_WriteOp20_DrawInstanced(
    CGCommandStreamRecorderGnm* self,
    uint32_t vertexCountPerInstance,
    uint32_t instanceCount,
    uint32_t startVertexLocation,
    uint32_t startInstanceLocation)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x20)) {
        p[0] = 0x20;
        *reinterpret_cast<uint32_t*>(p + 0x04) = vertexCountPerInstance;
        *reinterpret_cast<uint32_t*>(p + 0x08) = instanceCount;
        *reinterpret_cast<uint32_t*>(p + 0x0C) = startVertexLocation;
        *reinterpret_cast<uint32_t*>(p + 0x10) = startInstanceLocation;
    }
}

// 0x026ad790 (188 bytes) - Opcode 0x21: DrawIndexedInstanced
void CGCommandStreamRecorderGnm_WriteOp21_DrawIndexedInstanced(
    CGCommandStreamRecorderGnm* self,
    uint32_t indexCountPerInstance,
    uint32_t instanceCount,
    uint32_t startIndexLocation,
    int32_t  baseVertexLocation,
    uint32_t startInstanceLocation)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x20)) {
        p[0] = 0x21;
        *reinterpret_cast<uint32_t*>(p + 0x04) = indexCountPerInstance;
        *reinterpret_cast<uint32_t*>(p + 0x08) = instanceCount;
        *reinterpret_cast<uint32_t*>(p + 0x0C) = startIndexLocation;
        *reinterpret_cast<int32_t*>(p + 0x10)  = baseVertexLocation;
        *reinterpret_cast<uint32_t*>(p + 0x14) = startInstanceLocation;
    }
}

// 0x026ad850 (142 bytes) - Opcode 0x22: DrawInstancedIndirect
void CGCommandStreamRecorderGnm_WriteOp22_DrawInstancedIndirect(
    CGCommandStreamRecorderGnm* self, void* argsBuffer, uint32_t alignedByteOffset)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x20)) {
        p[0] = 0x22;
        *reinterpret_cast<void**>(p + 0x08)    = argsBuffer;
        *reinterpret_cast<uint32_t*>(p + 0x10) = alignedByteOffset;
    }
}

// 0x026ad8e0 (142 bytes) - Opcode 0x23: DrawIndexedInstancedIndirect
void CGCommandStreamRecorderGnm_WriteOp23_DrawIndexedInstancedIndirect(
    CGCommandStreamRecorderGnm* self, void* argsBuffer, uint32_t alignedByteOffset)
{
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x20)) {
        p[0] = 0x23;
        *reinterpret_cast<void**>(p + 0x08)    = argsBuffer;
        *reinterpret_cast<uint32_t*>(p + 0x10) = alignedByteOffset;
    }
}

// 0x026ad970 (115 bytes) - Opcode 0x24: FlushCaches
void CGCommandStreamRecorderGnm_WriteOp24_FlushCaches(CGCommandStreamRecorderGnm* self) {
    if (uint8_t* p = AllocateFromCommandChunk(self, 0x10)) {
        p[0] = 0x24;
    }
}

// ============================================================================
// 3. CGCommandStreamPlayerGnm (0x26aac90 .. 0x26ab7a0)
// ============================================================================

// External state-shadow & context forwarding calls (in 02_GX_GraphicsFramework)
extern void GXContext_SetRasterizerState(void* ctx, void* state);                                      // 0x02168c90
extern void GXContext_SetPrimitiveTopology(void* ctx, uint32_t topology);                              // 0x02168ca0
extern void GXContext_SetIndexBuffer(void* ctx, void* ib, uint32_t offset, uint32_t fmt);              // 0x02168cb0
extern void GXContext_SetVertexBuffers(void* ctx, uint32_t start, uint32_t count,
                                       const void* vbs, const void* strides, const void* offsets);     // 0x02168cc0
extern void GXContext_SetVSConstantBuffers(void* ctx, uint32_t start, uint32_t count, const void* cb); // 0x02168ce0
extern void GXContext_SetVSTextures(void* ctx, uint32_t start, uint32_t count, const void* tex);       // 0x02168d00
extern void GXContext_SetVSSamplers(void* ctx, uint32_t start, uint32_t count, const void* smp);       // 0x02168d90
extern void GXContext_SetVertexShader(void* ctx, void* vs);                                            // 0x02168e00
extern void GXContext_SetPSConstantBuffers(void* ctx, uint32_t start, uint32_t count, const void* cb); // 0x02168e10
extern void GXContext_SetPSTextures(void* ctx, uint32_t start, uint32_t count, const void* tex);       // 0x02168e30
extern void GXContext_SetPSSamplers(void* ctx, uint32_t start, uint32_t count, const void* smp);       // 0x02168ec0
extern void GXContext_SetPixelShader(void* ctx, void* ps);                                             // 0x02168f30
extern void GXContext_SetGSConstantBuffers(void* ctx, uint32_t start, uint32_t count, const void* cb); // 0x02168f40
extern void GXContext_SetGSTextures(void* ctx, uint32_t start, uint32_t count, const void* tex);       // 0x02168f60
extern void GXContext_SetGSSamplers(void* ctx, uint32_t start, uint32_t count, const void* smp);       // 0x02168ff0
extern void GXContext_SetGeometryShader(void* ctx, void* gs);                                          // 0x02169060
extern void GXContext_SetHSConstantBuffers(void* ctx, uint32_t start, uint32_t count, const void* cb); // 0x02169070
extern void GXContext_SetHSTextures(void* ctx, uint32_t start, uint32_t count, const void* tex);       // 0x02169090
extern void GXContext_SetHSSamplers(void* ctx, uint32_t start, uint32_t count, const void* smp);       // 0x02169120
extern void GXContext_SetHullShader(void* ctx, void* hs);                                              // 0x02169190
extern void GXContext_SetDSConstantBuffers(void* ctx, uint32_t start, uint32_t count, const void* cb); // 0x021692c0
extern void GXContext_SetDSTextures(void* ctx, uint32_t start, uint32_t count, const void* tex);       // 0x021692e0
extern void GXContext_SetDSSamplers(void* ctx, uint32_t start, uint32_t count, const void* smp);       // 0x02169370
extern void GXContext_SetDomainShader(void* ctx, void* ds);                                            // 0x021693e0
extern void GXContext_SetCSConstantBuffers(void* ctx, uint32_t start, uint32_t count, const void* cb); // 0x02169440
extern void GXContext_SetCSTextures(void* ctx, uint32_t start, uint32_t count, const void* tex);       // 0x02169460
extern void GXContext_SetCSSamplers(void* ctx, uint32_t start, uint32_t count, const void* smp);       // 0x021694f0
extern void GXContext_SetComputeShader(void* ctx, void* cs);                                           // 0x021695b0
extern void GXContext_Dispatch(void* ctx, uint32_t x, uint32_t y, uint32_t z);                         // 0x021695c0
extern void GXContext_Draw(void* ctx, uint32_t vtxCount, uint32_t startVtx);                           // 0x021696c0
extern void GXContext_DrawIndexed(void* ctx, uint32_t idxCount, uint32_t startIdx, int32_t baseVtx);   // 0x02169780
extern void GXContext_DrawInstanced(void* ctx, uint32_t vtxPerInst, uint32_t instCount,
                                    uint32_t startVtx, uint32_t startInst);                            // 0x02169840
extern void GXContext_DrawIndexedInstanced(void* ctx, uint32_t idxPerInst, uint32_t instCount,
                                           uint32_t startIdx, int32_t baseVtx, uint32_t startInst);    // 0x02169900
extern void GXContext_DrawInstancedIndirect(void* ctx, void* buf, uint32_t offset);                    // 0x021699d0
extern void GXContext_DrawIndexedInstancedIndirect(void* ctx, void* buf, uint32_t offset);             // 0x02169ab0
extern void GXContext_FlushCaches(void* ctx);                                                          // 0x02169ba0

using CommandStreamOpHandler = uint64_t (*)(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
#undef g_CommandStreamOpDispatchTable
#define g_CommandStreamOpDispatchTable (reinterpret_cast<CommandStreamOpHandler*>(GX_VMA(0x52b9fb0ULL)))

uint64_t CGCommandStreamPlayerGnm_PlayOp01_SetRasterizerState(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp02_SetPrimitiveTopology(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp03_SetIndexBuffer(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp04_SetVertexBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp05_SetVSConstantBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp06_SetVSTextures(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp07_SetVSSamplers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp08_SetVertexShader(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp09_SetPSConstantBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp0A_SetPSTextures(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp0B_SetPSSamplers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp0C_SetPixelShader(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp0D_SetGSConstantBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp0E_SetGSTextures(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp0F_SetGSSamplers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp10_SetGeometryShader(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp11_SetHSConstantBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp12_SetHSTextures(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp13_SetHSSamplers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp14_SetHullShader(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp15_SetDSConstantBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp16_SetDSTextures(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp17_SetDSSamplers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp18_SetDomainShader(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp19_SetCSConstantBuffers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp1A_SetCSTextures(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp1B_SetCSSamplers(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp1C_SetComputeShader(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp1D_Dispatch(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp1E_Draw(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp1F_DrawIndexed(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp20_DrawInstanced(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp21_DrawIndexedInstanced(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp22_DrawInstancedIndirect(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp23_DrawIndexedInstancedIndirect(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);
uint64_t CGCommandStreamPlayerGnm_PlayOp24_FlushCaches(void* ctx, uint8_t* stateShadow, const uint8_t* cmd);

// 0x026aac90 (92 bytes)
// Loops over a baked command stream and invokes the handler for each opcode
// until the terminating `0x00` opcode is reached.
void CGCommandStreamPlayerGnm_ExecuteStream(
    void* ctx,
    uint8_t* stateShadow,
    const uint8_t* streamCursor)
{
    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_CommandStream)) {
        uint8_t opcode = *streamCursor;
        while (opcode != 0) {
            uint64_t bytesRead = 0;
            switch (opcode) {
            case 0x01: bytesRead = CGCommandStreamPlayerGnm_PlayOp01_SetRasterizerState(ctx, stateShadow, streamCursor); break;
            case 0x02: bytesRead = CGCommandStreamPlayerGnm_PlayOp02_SetPrimitiveTopology(ctx, stateShadow, streamCursor); break;
            case 0x03: bytesRead = CGCommandStreamPlayerGnm_PlayOp03_SetIndexBuffer(ctx, stateShadow, streamCursor); break;
            case 0x04: bytesRead = CGCommandStreamPlayerGnm_PlayOp04_SetVertexBuffers(ctx, stateShadow, streamCursor); break;
            case 0x05: bytesRead = CGCommandStreamPlayerGnm_PlayOp05_SetVSConstantBuffers(ctx, stateShadow, streamCursor); break;
            case 0x06: bytesRead = CGCommandStreamPlayerGnm_PlayOp06_SetVSTextures(ctx, stateShadow, streamCursor); break;
            case 0x07: bytesRead = CGCommandStreamPlayerGnm_PlayOp07_SetVSSamplers(ctx, stateShadow, streamCursor); break;
            case 0x08: bytesRead = CGCommandStreamPlayerGnm_PlayOp08_SetVertexShader(ctx, stateShadow, streamCursor); break;
            case 0x09: bytesRead = CGCommandStreamPlayerGnm_PlayOp09_SetPSConstantBuffers(ctx, stateShadow, streamCursor); break;
            case 0x0A: bytesRead = CGCommandStreamPlayerGnm_PlayOp0A_SetPSTextures(ctx, stateShadow, streamCursor); break;
            case 0x0B: bytesRead = CGCommandStreamPlayerGnm_PlayOp0B_SetPSSamplers(ctx, stateShadow, streamCursor); break;
            case 0x0C: bytesRead = CGCommandStreamPlayerGnm_PlayOp0C_SetPixelShader(ctx, stateShadow, streamCursor); break;
            case 0x0D: bytesRead = CGCommandStreamPlayerGnm_PlayOp0D_SetGSConstantBuffers(ctx, stateShadow, streamCursor); break;
            case 0x0E: bytesRead = CGCommandStreamPlayerGnm_PlayOp0E_SetGSTextures(ctx, stateShadow, streamCursor); break;
            case 0x0F: bytesRead = CGCommandStreamPlayerGnm_PlayOp0F_SetGSSamplers(ctx, stateShadow, streamCursor); break;
            case 0x10: bytesRead = CGCommandStreamPlayerGnm_PlayOp10_SetGeometryShader(ctx, stateShadow, streamCursor); break;
            case 0x11: bytesRead = CGCommandStreamPlayerGnm_PlayOp11_SetHSConstantBuffers(ctx, stateShadow, streamCursor); break;
            case 0x12: bytesRead = CGCommandStreamPlayerGnm_PlayOp12_SetHSTextures(ctx, stateShadow, streamCursor); break;
            case 0x13: bytesRead = CGCommandStreamPlayerGnm_PlayOp13_SetHSSamplers(ctx, stateShadow, streamCursor); break;
            case 0x14: bytesRead = CGCommandStreamPlayerGnm_PlayOp14_SetHullShader(ctx, stateShadow, streamCursor); break;
            case 0x15: bytesRead = CGCommandStreamPlayerGnm_PlayOp15_SetDSConstantBuffers(ctx, stateShadow, streamCursor); break;
            case 0x16: bytesRead = CGCommandStreamPlayerGnm_PlayOp16_SetDSTextures(ctx, stateShadow, streamCursor); break;
            case 0x17: bytesRead = CGCommandStreamPlayerGnm_PlayOp17_SetDSSamplers(ctx, stateShadow, streamCursor); break;
            case 0x18: bytesRead = CGCommandStreamPlayerGnm_PlayOp18_SetDomainShader(ctx, stateShadow, streamCursor); break;
            case 0x19: bytesRead = CGCommandStreamPlayerGnm_PlayOp19_SetCSConstantBuffers(ctx, stateShadow, streamCursor); break;
            case 0x1A: bytesRead = CGCommandStreamPlayerGnm_PlayOp1A_SetCSTextures(ctx, stateShadow, streamCursor); break;
            case 0x1B: bytesRead = CGCommandStreamPlayerGnm_PlayOp1B_SetCSSamplers(ctx, stateShadow, streamCursor); break;
            case 0x1C: bytesRead = CGCommandStreamPlayerGnm_PlayOp1C_SetComputeShader(ctx, stateShadow, streamCursor); break;
            case 0x1D: bytesRead = CGCommandStreamPlayerGnm_PlayOp1D_Dispatch(ctx, stateShadow, streamCursor); break;
            case 0x1E: bytesRead = CGCommandStreamPlayerGnm_PlayOp1E_Draw(ctx, stateShadow, streamCursor); break;
            case 0x1F: bytesRead = CGCommandStreamPlayerGnm_PlayOp1F_DrawIndexed(ctx, stateShadow, streamCursor); break;
            case 0x20: bytesRead = CGCommandStreamPlayerGnm_PlayOp20_DrawInstanced(ctx, stateShadow, streamCursor); break;
            case 0x21: bytesRead = CGCommandStreamPlayerGnm_PlayOp21_DrawIndexedInstanced(ctx, stateShadow, streamCursor); break;
            case 0x22: bytesRead = CGCommandStreamPlayerGnm_PlayOp22_DrawInstancedIndirect(ctx, stateShadow, streamCursor); break;
            case 0x23: bytesRead = CGCommandStreamPlayerGnm_PlayOp23_DrawIndexedInstancedIndirect(ctx, stateShadow, streamCursor); break;
            case 0x24: bytesRead = CGCommandStreamPlayerGnm_PlayOp24_FlushCaches(ctx, stateShadow, streamCursor); break;
            default:   bytesRead = g_CommandStreamOpDispatchTable[opcode](ctx, stateShadow, streamCursor); break;
            }
            uint64_t alignedAdvance = (bytesRead + 0xFu) & ~static_cast<uint64_t>(0xFu);
            streamCursor += alignedAdvance;
            opcode = *streamCursor;
        }
        return;
    }

    uint8_t opcode = *streamCursor;
    while (opcode != 0) {
        uint64_t bytesRead = g_CommandStreamOpDispatchTable[opcode](ctx, stateShadow, streamCursor);
        uint64_t alignedAdvance = (bytesRead + 0xFu) & ~static_cast<uint64_t>(0xFu);
        streamCursor += alignedAdvance;
        opcode = *streamCursor;
    }
}

// 0x026aad80 (39 bytes) - Play Op 0x01: SetRasterizerState
uint64_t CGCommandStreamPlayerGnm_PlayOp01_SetRasterizerState(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* state = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetRasterizerState(ctx, state);
    *reinterpret_cast<void**>(stateShadow + 0x0000) = state;
    return 0x10;
}

// 0x026aadb0 (38 bytes) - Play Op 0x02: SetPrimitiveTopology
uint64_t CGCommandStreamPlayerGnm_PlayOp02_SetPrimitiveTopology(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    uint32_t topo = *reinterpret_cast<const uint32_t*>(cmd + 0x04);
    GXContext_SetPrimitiveTopology(ctx, topo);
    *reinterpret_cast<uint32_t*>(stateShadow + 0x0008) = topo;
    return 0x08;
}

// 0x026aade0 (61 bytes) - Play Op 0x03: SetIndexBuffer
uint64_t CGCommandStreamPlayerGnm_PlayOp03_SetIndexBuffer(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void*    ib     = *reinterpret_cast<void* const*>(cmd + 0x08);
    uint32_t offset = *reinterpret_cast<const uint32_t*>(cmd + 0x10);
    uint32_t fmt    = *reinterpret_cast<const uint32_t*>(cmd + 0x14);
    GXContext_SetIndexBuffer(ctx, ib, offset, fmt);
    *reinterpret_cast<void**>(stateShadow + 0x0010)    = ib;
    *reinterpret_cast<uint32_t*>(stateShadow + 0x0018) = offset;
    *reinterpret_cast<uint32_t*>(stateShadow + 0x001c) = fmt;
    return 0x18;
}

// 0x026aae20 (161 bytes) - Play Op 0x04: SetVertexBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp04_SetVertexBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    uint32_t startSlot = *reinterpret_cast<const uint32_t*>(cmd + 0x04);
    uint32_t count     = *reinterpret_cast<const uint32_t*>(cmd + 0x08);
    const void* const* vbs     = reinterpret_cast<const void* const*>(cmd + 0x0C);
    const uint32_t*    strides = reinterpret_cast<const uint32_t*>(cmd + 0x0C + count * 8u);
    const uint32_t*    offsets = reinterpret_cast<const uint32_t*>(cmd + 0x0C + count * 12u);

    GXContext_SetVertexBuffers(ctx, startSlot, count, vbs, strides, offsets);
    if (count == 0) {
        return 0x0C;
    }
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t slot = startSlot + i;
        *reinterpret_cast<const void**>(stateShadow + 0x0020 + slot * 8u) = vbs[i];
        *reinterpret_cast<uint32_t*>(stateShadow + 0x0120 + slot * 4u)    = strides[i];
        *reinterpret_cast<uint32_t*>(stateShadow + 0x01a0 + slot * 4u)    = offsets[i];
    }
    return (static_cast<uint64_t>(count) << 4) | 0x0Cu;
}

// Helper for playing array-of-pointers state updates
static inline uint64_t PlayPointerArrayStateUpdate(
    uint8_t* stateShadow,
    size_t shadowBaseOffset,
    const uint8_t* cmd)
{
    uint32_t startSlot = *reinterpret_cast<const uint32_t*>(cmd + 0x04);
    uint32_t count     = *reinterpret_cast<const uint32_t*>(cmd + 0x08);
    if (count == 0) {
        return 0x0C;
    }
    const void* const* ptrs = reinterpret_cast<const void* const*>(cmd + 0x0C);
    for (uint32_t i = 0; i < count; ++i) {
        *reinterpret_cast<const void**>(stateShadow + shadowBaseOffset + (startSlot + i) * 8u) = ptrs[i];
    }
    return (static_cast<uint64_t>(count) * 8u) + 0x0Cu;
}

// 0x026aaed0 (90 bytes) - Play Op 0x05: SetVSConstantBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp05_SetVSConstantBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetVSConstantBuffers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0598, cmd);
}

// 0x026aaf30 (90 bytes) - Play Op 0x06: SetVSTextures
uint64_t CGCommandStreamPlayerGnm_PlayOp06_SetVSTextures(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetVSTextures(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0618, cmd);
}

// 0x026aaf90 (90 bytes) - Play Op 0x07: SetVSSamplers
uint64_t CGCommandStreamPlayerGnm_PlayOp07_SetVSSamplers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetVSSamplers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0a18, cmd);
}

// 0x026aaff0 (43 bytes) - Play Op 0x08: SetVertexShader
uint64_t CGCommandStreamPlayerGnm_PlayOp08_SetVertexShader(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* vs = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetVertexShader(ctx, vs);
    *reinterpret_cast<void**>(stateShadow + 0x0a88) = vs;
    return 0x10;
}

// 0x026ab020 (90 bytes) - Play Op 0x09: SetPSConstantBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp09_SetPSConstantBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetPSConstantBuffers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0a90, cmd);
}

// 0x026ab080 (90 bytes) - Play Op 0x0A: SetPSTextures
uint64_t CGCommandStreamPlayerGnm_PlayOp0A_SetPSTextures(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetPSTextures(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0b10, cmd);
}

// 0x026ab0e0 (90 bytes) - Play Op 0x0B: SetPSSamplers
uint64_t CGCommandStreamPlayerGnm_PlayOp0B_SetPSSamplers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetPSSamplers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0f10, cmd);
}

// 0x026ab140 (43 bytes) - Play Op 0x0C: SetPixelShader
uint64_t CGCommandStreamPlayerGnm_PlayOp0C_SetPixelShader(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* ps = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetPixelShader(ctx, ps);
    *reinterpret_cast<void**>(stateShadow + 0x0f80) = ps;
    return 0x10;
}

// 0x026ab170 (90 bytes) - Play Op 0x0D: SetGSConstantBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp0D_SetGSConstantBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetGSConstantBuffers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x0f88, cmd);
}

// 0x026ab1d0 (90 bytes) - Play Op 0x0E: SetGSTextures
uint64_t CGCommandStreamPlayerGnm_PlayOp0E_SetGSTextures(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetGSTextures(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1008, cmd);
}

// 0x026ab230 (90 bytes) - Play Op 0x0F: SetGSSamplers
uint64_t CGCommandStreamPlayerGnm_PlayOp0F_SetGSSamplers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetGSSamplers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1408, cmd);
}

// 0x026ab290 (43 bytes) - Play Op 0x10: SetGeometryShader
uint64_t CGCommandStreamPlayerGnm_PlayOp10_SetGeometryShader(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* gs = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetGeometryShader(ctx, gs);
    *reinterpret_cast<void**>(stateShadow + 0x1478) = gs;
    return 0x10;
}

// 0x026ab2c0 (90 bytes) - Play Op 0x11: SetHSConstantBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp11_SetHSConstantBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetHSConstantBuffers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1480, cmd);
}

// 0x026ab320 (90 bytes) - Play Op 0x12: SetHSTextures
uint64_t CGCommandStreamPlayerGnm_PlayOp12_SetHSTextures(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetHSTextures(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1500, cmd);
}

// 0x026ab380 (90 bytes) - Play Op 0x13: SetHSSamplers
uint64_t CGCommandStreamPlayerGnm_PlayOp13_SetHSSamplers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetHSSamplers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1900, cmd);
}

// 0x026ab3e0 (43 bytes) - Play Op 0x14: SetHullShader
uint64_t CGCommandStreamPlayerGnm_PlayOp14_SetHullShader(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* hs = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetHullShader(ctx, hs);
    *reinterpret_cast<void**>(stateShadow + 0x1970) = hs;
    return 0x10;
}

// 0x026ab410 (90 bytes) - Play Op 0x15: SetDSConstantBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp15_SetDSConstantBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetDSConstantBuffers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1978, cmd);
}

// 0x026ab470 (90 bytes) - Play Op 0x16: SetDSTextures
uint64_t CGCommandStreamPlayerGnm_PlayOp16_SetDSTextures(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetDSTextures(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x19f8, cmd);
}

// 0x026ab4d0 (90 bytes) - Play Op 0x17: SetDSSamplers
uint64_t CGCommandStreamPlayerGnm_PlayOp17_SetDSSamplers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetDSSamplers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1df8, cmd);
}

// 0x026ab530 (43 bytes) - Play Op 0x18: SetDomainShader
uint64_t CGCommandStreamPlayerGnm_PlayOp18_SetDomainShader(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* ds = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetDomainShader(ctx, ds);
    *reinterpret_cast<void**>(stateShadow + 0x1e68) = ds;
    return 0x10;
}

// 0x026ab560 (90 bytes) - Play Op 0x19: SetCSConstantBuffers
uint64_t CGCommandStreamPlayerGnm_PlayOp19_SetCSConstantBuffers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetCSConstantBuffers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1e70, cmd);
}

// 0x026ab5c0 (90 bytes) - Play Op 0x1A: SetCSTextures
uint64_t CGCommandStreamPlayerGnm_PlayOp1A_SetCSTextures(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetCSTextures(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x1ef0, cmd);
}

// 0x026ab620 (90 bytes) - Play Op 0x1B: SetCSSamplers
uint64_t CGCommandStreamPlayerGnm_PlayOp1B_SetCSSamplers(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    GXContext_SetCSSamplers(
        ctx, *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08), cmd + 0x0C);
    return PlayPointerArrayStateUpdate(stateShadow, 0x22f0, cmd);
}

// 0x026ab680 (43 bytes) - Play Op 0x1C: SetComputeShader
uint64_t CGCommandStreamPlayerGnm_PlayOp1C_SetComputeShader(
    void* ctx, uint8_t* stateShadow, const uint8_t* cmd)
{
    void* cs = *reinterpret_cast<void* const*>(cmd + 0x08);
    GXContext_SetComputeShader(ctx, cs);
    *reinterpret_cast<void**>(stateShadow + 0x2360) = cs;
    return 0x10;
}

// 0x026ab6b0 (28 bytes) - Play Op 0x1D: Dispatch
uint64_t CGCommandStreamPlayerGnm_PlayOp1D_Dispatch(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_Dispatch(
        ctx,
        *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08),
        *reinterpret_cast<const uint32_t*>(cmd + 0x0C));
    return 0x10;
}

// 0x026ab6d0 (24 bytes) - Play Op 0x1E: Draw
uint64_t CGCommandStreamPlayerGnm_PlayOp1E_Draw(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_Draw(
        ctx,
        *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08));
    return 0x0C;
}

// 0x026ab6f0 (28 bytes) - Play Op 0x1F: DrawIndexed
uint64_t CGCommandStreamPlayerGnm_PlayOp1F_DrawIndexed(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_DrawIndexed(
        ctx,
        *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08),
        *reinterpret_cast<const int32_t*>(cmd + 0x0C));
    return 0x10;
}

// 0x026ab710 (32 bytes) - Play Op 0x20: DrawInstanced
uint64_t CGCommandStreamPlayerGnm_PlayOp20_DrawInstanced(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_DrawInstanced(
        ctx,
        *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08),
        *reinterpret_cast<const uint32_t*>(cmd + 0x0C),
        *reinterpret_cast<const uint32_t*>(cmd + 0x10));
    return 0x14;
}

// 0x026ab730 (46 bytes) - Play Op 0x21: DrawIndexedInstanced
uint64_t CGCommandStreamPlayerGnm_PlayOp21_DrawIndexedInstanced(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_DrawIndexedInstanced(
        ctx,
        *reinterpret_cast<const uint32_t*>(cmd + 0x04),
        *reinterpret_cast<const uint32_t*>(cmd + 0x08),
        *reinterpret_cast<const uint32_t*>(cmd + 0x0C),
        *reinterpret_cast<const int32_t*>(cmd + 0x10),
        *reinterpret_cast<const uint32_t*>(cmd + 0x14));
    return 0x18;
}

// 0x026ab760 (26 bytes) - Play Op 0x22: DrawInstancedIndirect
uint64_t CGCommandStreamPlayerGnm_PlayOp22_DrawInstancedIndirect(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_DrawInstancedIndirect(
        ctx,
        *reinterpret_cast<void* const*>(cmd + 0x08),
        *reinterpret_cast<const uint32_t*>(cmd + 0x10));
    return 0x18;
}

// 0x026ab780 (26 bytes) - Play Op 0x23: DrawIndexedInstancedIndirect
uint64_t CGCommandStreamPlayerGnm_PlayOp23_DrawIndexedInstancedIndirect(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* cmd)
{
    GXContext_DrawIndexedInstancedIndirect(
        ctx,
        *reinterpret_cast<void* const*>(cmd + 0x08),
        *reinterpret_cast<const uint32_t*>(cmd + 0x10));
    return 0x18;
}

// 0x026ab7a0 (16 bytes) - Play Op 0x24: FlushCaches
uint64_t CGCommandStreamPlayerGnm_PlayOp24_FlushCaches(
    void* ctx, uint8_t* /*stateShadow*/, const uint8_t* /*cmd*/)
{
    GXContext_FlushCaches(ctx);
    return 0x01;
}

} // namespace CoreGraphics2
} // namespace from
