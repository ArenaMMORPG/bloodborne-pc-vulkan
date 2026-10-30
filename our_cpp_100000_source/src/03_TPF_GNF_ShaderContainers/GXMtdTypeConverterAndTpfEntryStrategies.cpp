// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Tier 9/10 Container & Material Parsers
// Module: 03_TPF_GNF_ShaderContainers
// File:   GXMtdTypeConverterAndTpfEntryStrategies.cpp
// Range:  0x26dab60 .. 0x26dc370 (65 functions, 3,029 bytes, 100% complete)
//
// Subsystem Overview:
//   1. MTD Material Container lifecycle & SIMD AVX parameter type conversion
//      (`0x26dbed0 .. 0x26dc370`, 6 functions)
//   2. Display / SwapChain 1080p60 Output Config & CGDisplayConfig Bridge
//      (`0x26dbbc0 .. 0x26dbe00`, 13 functions)
//   3. GX Dynamic Ring Buffer Sub-Allocator & Command Context Map/Unmap Cursor
//      (`0x26dae20 .. 0x26db300`, 8 functions)
//   4. TPF Texture Container Entry Strategy Accessors (V2 0x24-byte Header &
//      V1 0x14-byte Embedded DDS Header) (`0x26dab60 .. 0x26dae00`, 38 functions)
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace FromSoftware {
namespace GX {

// ============================================================================
// Core Structures for Batch 2
// ============================================================================

struct DLAllocator {
    struct VTable {
        void*    (*dtor)(DLAllocator* self, uint32_t flags);                       // +0x00
        void*    (*dtorDeleting)(DLAllocator* self);                               // +0x08
        uint32_t (*getAllocatorId)(const DLAllocator* self);                       // +0x10
        uint32_t (*getAllocatorTag)(const DLAllocator* self);                      // +0x18
        void     (*getCapabilities)(uint32_t* outCaps,
                                    DLAllocator* selfArg, uint32_t flags);         // +0x20
        void*    pad28[5];                                                         // +0x28..+0x48
        void*    (*allocate)(DLAllocator* self, size_t bytes);                     // +0x50
        void*    (*allocateAligned)(DLAllocator* self, size_t bytes, size_t align);// +0x58
        void*    pad60[2];                                                         // +0x60..+0x68
        void     (*deallocate)(DLAllocator* self, void* ptr);                      // +0x70
    };
    VTable* vptr;
};

struct GXMtdBlockObject {
    struct VTable {
        void  (*dtor)(GXMtdBlockObject* self);                                     // +0x00
    };
    VTable* vptr;
};

struct GXMtdMaterialContainer {
    GXMtdBlockObject* rootBlock; // +0x00
    uint32_t          lastError; // +0x08
    uint32_t          pad0C;     // +0x0C
};

struct GXMtdConvertedValue16 {
    uint64_t lo;
    uint64_t hi;
};

// Display Mode Descriptor (0x1C bytes, initialized at 0x26dbe00)
struct GXDisplayModeDesc {
    uint32_t width;              // +0x00 (1920 = 0x780)
    uint32_t height;             // +0x04 (1080 = 0x438)
    uint32_t refreshRateNum;     // +0x08 (60000 = 0xEA60)
    uint32_t refreshRateDen;     // +0x0C (1000 = 0x3E8)
    uint32_t pixelFormat;        // +0x10 (0x57 = 87 = B8G8R8A8_UNORM)
    uint32_t scanlineOrder;      // +0x14 (1)
    uint32_t scalingMode;        // +0x18 (0)
};

// Display Output Adapter Info (0x60 bytes, initialized at 0x26dbdb0)
struct GXDisplayOutputDesc {
    char16_t deviceName[32];     // +0x00..+0x3F (u"\\\\.\\DISPLAY1")
    int32_t  desktopLeft;        // +0x40 (0)
    int32_t  desktopTop;         // +0x44 (0)
    int32_t  desktopRight;       // +0x48 (1920 = 0x780)
    int32_t  desktopBottom;      // +0x4C (1080 = 0x438)
    uint8_t  isAttachedToDesktop;// +0x50 (1)
    uint8_t  pad51[3];           // +0x51..+0x53
    uint32_t rotationMode;       // +0x54 (1)
    void*    monitorHandle;      // +0x58 (nullptr)
};

// Ref-counted GXGnmBuffer wrapper (+0x08 = allocator, +0x10 = atomic refCount, +0x18 = hardware buffer)
struct GXGnmRefCountedBuffer {
    struct VTable {
        void (*dtor)(GXGnmRefCountedBuffer* self); // +0x00
    };
    VTable*      vptr;           // +0x00
    DLAllocator* allocator;      // +0x08
    int32_t      refCount;       // +0x10
    uint32_t     pad14;          // +0x14
    uint8_t      hwBuffer[0x40]; // +0x18
};

struct GXBufferDescriptor24 {
    uint32_t byteSize;            // +0x00
    uint32_t usageType;           // +0x04
    uint32_t bindFlags;           // +0x08
    uint32_t cpuAccessFlags;      // +0x0C
    uint32_t miscFlags;           // +0x10
    uint32_t structureByteStride; // +0x14
};

// 0x18-byte Dynamic Ring Buffer Sub-Allocator Cursor (constructed at 0x26db140)
struct GXDynamicBufferRingCursor {
    GXGnmRefCountedBuffer* bufferObj;        // +0x00
    uint32_t               totalByteSize;    // +0x08
    uint32_t               alignment;        // +0x0C
    uint32_t               nextFreeOffset;   // +0x10
    uint32_t               lastMappedOffset; // +0x14
};

// 0x18-byte Dynamic Ring Buffer Owner (constructed at 0x26dae20, destroyed at 0x26dafd0)
struct GXDynamicBufferRingOwner {
    DLAllocator*               allocator;    // +0x00
    GXGnmRefCountedBuffer*     bufferObj;    // +0x08
    GXDynamicBufferRingCursor* ringCursor;   // +0x10
};

// TPF Entry Header V2 (0x24 bytes, used by 0x26daca0 .. 0x26dae00)
struct GXTpfEntryHeaderV2 {
    uint32_t dataOffset;         // +0x00
    uint32_t dataSize;           // +0x04
    uint8_t  formatCode;         // +0x08
    uint8_t  textureType;        // +0x09 (1 = cubemap -> 6 faces)
    int8_t   mipCount;           // +0x0A
    int8_t   flags;              // +0x0B
    uint16_t width;              // +0x0C
    uint16_t height;             // +0x0E
    uint32_t depthOrArraySize;   // +0x10
    uint32_t headerExtValue;     // +0x14
    uint32_t fileNameOffset;     // +0x18
    uint32_t headerExtSize;      // +0x1C
    uint32_t dxgiFormat;         // +0x20
};

// TPF Entry Header V1 (0x14 bytes, used by 0x26dab60 .. 0x26dac70, points to embedded DDS)
struct GXTpfEntryHeaderV1 {
    uint32_t dataOffset;         // +0x00 (offset to embedded DDS header)
    uint32_t dataSize;           // +0x04
    uint8_t  formatCode;         // +0x08
    uint8_t  textureType;        // +0x09
    int8_t   mipCount;           // +0x0A
    int8_t   flags;              // +0x0B
    uint32_t fileNameOffset;     // +0x0C
    uint32_t headerExtSize;      // +0x10
};

// View pairing the TPF file base address with the current entry header
struct GXTpfTextureSourceViewV2 {
    const uint8_t*            tpfBase;  // +0x00
    const GXTpfEntryHeaderV2* entryHdr; // +0x08
};

struct GXTpfTextureSourceViewV1 {
    const uint8_t*            tpfBase;  // +0x00
    const GXTpfEntryHeaderV1* entryHdr; // +0x08
};

struct GXTpfExtraMetadataOut {
    uint8_t  pad00;              // +0x00
    uint16_t flags01;            // +0x01 (set to 0x0101)
    uint8_t  pad03[9];           // +0x03..+0x0B
    uint32_t extValue0C;         // +0x0C
} __attribute__((packed));

} // namespace GX
} // namespace FromSoftware

// ============================================================================
// External Engine & Module 01 / 02 / Batch 3 Declarations
// ============================================================================
extern "C" {

// Module 01 (01_CoreGraphics2_GNM) & Module 02 (02_GX_GraphicsFramework) functions
const void* CGDisplayConfig_GetGlobalInstance();                                   // 0x26d4320
void        GXGnmBuffer_ExtractDescriptorFromHardware(const void* hwBuf,
                                                      FromSoftware::GX::GXBufferDescriptor24* outDesc); // 0x216d730
uint32_t    GXGnmCommandContext_GetContextSlotId(const void* cmdCtx);              // 0x216a7c0
uint32_t    GXGnmBuffer_ValidateMapMode(void* bufObj, uint32_t mapMode, uint32_t ctxSlotId); // 0x2165150
uint32_t    GXGnmCommandContext_MapBuffer(void* cmdCtx, void* cmdCtxRingBase, void* bufObj,
                                          uint32_t subresource, uint32_t mapMode, void** outMappedPtr); // 0x2169c30
void        GXGnmCommandContext_UnmapBuffer(void* cmdCtx, void* cmdCtxRingBase,
                                            void* bufObj, uint32_t subresource);   // 0x2169eb0
uint32_t    GXGnmDevice_CreateBuffer(void* gxDevice,
                                     FromSoftware::GX::GXGnmRefCountedBuffer** outBuf,
                                     const void* bufDesc,
                                     const void* initialData,
                                     FromSoftware::GX::DLAllocator* allocator,
                                     void* ownerObj,
                                     uint64_t reservedZero);                       // 0x2165c10

// Dantelion2 memory & atomic helpers
void*       DLHeap_AllocateAligned(size_t bytes, size_t alignment,
                                   FromSoftware::GX::DLAllocator* allocator) __asm__("_ZN4from13CoreGraphics220DLAllocator_AllocateEmmPv");      // 0x207c4e0
int32_t     DLAtomic_FetchDecrement32(volatile int32_t* ptr);                      // 0x207ec00
char16_t*   DLCharTraitsW_CopyString(char16_t* dst, const char16_t* src);          // 0x2bbf608

// Batch 3 TPF / DDS format & mip helpers (0x26d7530 .. 0x26d8010)
uint32_t    GXTpfFormat_DxgiToEngineFormat(uint32_t dxgiFormat);                   // 0x26d7a00
uint32_t    GXDdsHeader_GetEnginePixelFormat(const uint8_t* ddsHeader);            // 0x26d7530
uint32_t    GXDdsHeader_GetArrayOrCubeSliceCount(const uint8_t* ddsHeader);        // 0x26d7a20
uint32_t    GXDdsHeader_GetMipSlicePitchOrRowSize(const uint8_t* ddsHeader, uint32_t mipIndex); // 0x26d7a50
uint32_t    GXDdsHeader_GetMipSliceByteSize(const uint8_t* ddsHeader, uint32_t mipIndex);       // 0x26d7ab0
uint32_t    GXDdsHeader_GetSubresourceOffset(const uint8_t* ddsHeader,
                                             uint32_t sliceIndex, uint32_t mipIndex);           // 0x26d7b40
void        GXDynamicBufferRingCursor_CtorThunk(
                FromSoftware::GX::GXDynamicBufferRingCursor* self,
                uint32_t* outStatus,
                FromSoftware::GX::DLAllocator* allocator,
                FromSoftware::GX::GXGnmRefCountedBuffer* bufferObj,
                uint32_t alignment,
                uint32_t flags);                                                   // 0x26d8010
void        GXDynamicBufferRingCursor_DtorThunk(
                FromSoftware::GX::GXDynamicBufferRingCursor* self);                // 0x26d8000

} // extern "C"

namespace FromSoftware {
namespace GX {

namespace {

// 0x4871640: Scalar component counts per GXMtdParamType (0..11)
constexpr uint32_t kMtdParamComponentCounts[12] = {
    1, 2, 3, 4,
    1, 2, 3, 4,
    1, 2, 3, 4
};

// 0x4871670: Base category per GXMtdParamType (0 = bool, 1 = int, 2 = float)
constexpr uint32_t kMtdParamBaseCategories[12] = {
    0, 0, 0, 0,
    1, 1, 1, 1,
    2, 2, 2, 2
};

// 0x4871540: Default display output name L"\\\\.\\DISPLAY1"
constexpr char16_t kDefaultDisplayDeviceName[] = u"\\\\.\\DISPLAY1";

using BlockFreeFn = void (*)(void* ptr);
#define g_pfnMtdBlockDeallocateBatch2 (*reinterpret_cast<BlockFreeFn*>(GX_VMA(0x52c45d0ULL)))

} // anonymous namespace

extern "C" {

// ============================================================================
// 1. VMA 0x26dc370 (13 B) — GXMtdMaterialContainer_CopyCtorShallow
// ============================================================================
void GXMtdMaterialContainer_CopyCtorShallow(
    GXMtdMaterialContainer* self,
    const GXMtdMaterialContainer* rhs)
{
    self->rootBlock = rhs->rootBlock;
    self->lastError = rhs->lastError;
}

// ============================================================================
// 2. VMA 0x26dc330 (58 B) — GXMtdMaterialContainer_DestroyRootBlockA
// ============================================================================
void GXMtdMaterialContainer_DestroyRootBlockA(GXMtdMaterialContainer* self) {
    if (self->rootBlock != nullptr) {
        self->rootBlock->vptr->dtor(self->rootBlock);
        g_pfnMtdBlockDeallocateBatch2(self->rootBlock);
        self->rootBlock = nullptr;
    }
    self->lastError = 0;
}

// ============================================================================
// 3. VMA 0x26dc2f0 (58 B) — GXMtdMaterialContainer_DestroyRootBlockB
// ============================================================================
void GXMtdMaterialContainer_DestroyRootBlockB(GXMtdMaterialContainer* self) {
    if (self->rootBlock != nullptr) {
        self->rootBlock->vptr->dtor(self->rootBlock);
        g_pfnMtdBlockDeallocateBatch2(self->rootBlock);
        self->rootBlock = nullptr;
    }
    self->lastError = 0;
}

// ============================================================================
// 4. VMA 0x26dc2e0 (15 B) — GXMtdMaterialContainer_DefaultCtor
// ============================================================================
void GXMtdMaterialContainer_DefaultCtor(GXMtdMaterialContainer* self) {
    self->rootBlock = nullptr;
    self->lastError = 0;
}

// ============================================================================
// 5. VMA 0x26dc2b0 (43 B) — GXMtdParamType_GetCategoryAndComponentCount
//    Returns (componentCount << 32) | baseCategory for typeIndex in 0..11,
//    or (0ULL << 32) | 3u if typeIndex > 11.
// ============================================================================
uint64_t GXMtdParamType_GetCategoryAndComponentCount(
    const void* unusedSelf,
    uint32_t typeIndex)
{
    uint64_t compCount = (unusedSelf != nullptr) ? 0ULL : 0ULL;
    uint64_t category  = 3ULL;
    if (typeIndex <= 11u) {
        compCount = static_cast<uint64_t>(kMtdParamComponentCounts[typeIndex]) << 32;
        category  = static_cast<uint64_t>(kMtdParamBaseCategories[typeIndex]);
    }
    return compCount | category;
}

// ============================================================================
// 6. VMA 0x26dbed0 (978 B) — GXMtdParamValue_ConvertBetweenTypes
//    Converts up to 4 parameter components from srcVal4 (of srcType 0..11) to
//    dstType (0..11), initializing all 4 output slots from defaultVal4 first
//    so any components beyond srcType's component count retain defaultVal4.
// ============================================================================
GXMtdConvertedValue16 GXMtdParamValue_ConvertBetweenTypes(
    GXMtdMaterialContainer* container,
    const void* srcVal4,
    uint32_t srcType,
    const void* defaultVal4,
    uint32_t dstType)
{
    uint32_t tempConverted[4];
    uint32_t outResult[4];

    std::memcpy(tempConverted, srcVal4, 16);
    std::memcpy(outResult, defaultVal4, 16);

    (void)container;
    if (srcType <= 11u && dstType <= 11u) {
        int32_t  srcCount = static_cast<int32_t>(kMtdParamComponentCounts[srcType]);
        uint32_t srcCat   = kMtdParamBaseCategories[srcType];
        uint32_t dstCat   = kMtdParamBaseCategories[dstType];

        if (srcCat == dstCat) {
            std::memcpy(tempConverted, srcVal4, 16);
        } else if (srcCat == 2u) {
            // Source is float (cat 2)
            const float* srcFloats = static_cast<const float*>(srcVal4);
            if (srcCount > 0) {
                if (dstCat == 0u) {
                    // float -> bool (0 or 1)
                    for (int32_t i = 0; i < srcCount; ++i) {
                        tempConverted[i] = (srcFloats[i] != 0.0f) ? 1u : 0u;
                    }
                } else {
                    // float -> int32 (cvttss2si)
                    for (int32_t i = 0; i < srcCount; ++i) {
                        tempConverted[i] = static_cast<uint32_t>(static_cast<int32_t>(srcFloats[i]));
                    }
                }
            }
        } else if (srcCat == 1u) {
            // Source is int/uint (cat 1)
            const uint32_t* srcInts = static_cast<const uint32_t*>(srcVal4);
            if (srcCount > 0) {
                if (dstCat == 0u) {
                    // int -> bool (0 or 1)
                    for (int32_t i = 0; i < srcCount; ++i) {
                        tempConverted[i] = (srcInts[i] != 0u) ? 1u : 0u;
                    }
                } else {
                    // int -> float (unsigned 32-bit to float)
                    float* dstFloats = reinterpret_cast<float*>(tempConverted);
                    for (int32_t i = 0; i < srcCount; ++i) {
                        dstFloats[i] = static_cast<float>(srcInts[i]);
                    }
                }
            }
        } else {
            // Source is bool (cat 0)
            const uint32_t* srcBools = static_cast<const uint32_t*>(srcVal4);
            if (srcCount > 0) {
                if (dstCat == 2u) {
                    // bool -> float (0.0f or 1.0f)
                    float* dstFloats = reinterpret_cast<float*>(tempConverted);
                    for (int32_t i = 0; i < srcCount; ++i) {
                        dstFloats[i] = (srcBools[i] != 0u) ? 1.0f : 0.0f;
                    }
                } else {
                    // bool -> int (0 or 1)
                    for (int32_t i = 0; i < srcCount; ++i) {
                        tempConverted[i] = (srcBools[i] != 0u) ? 1u : 0u;
                    }
                }
            }
        }

        if (srcCount > 0) {
            std::memcpy(outResult, tempConverted, static_cast<size_t>(srcCount) * sizeof(uint32_t));
        }
    }

    GXMtdConvertedValue16 ret{};
    std::memcpy(&ret, outResult, 16);
    return ret;
}

// ============================================================================
// 7. VMA 0x26dbe00 (51 B) — GXDisplayOutput_GetDefaultModeDesc1080p60
//    Populates a 1920x1080 @ 60Hz (60000/1000) B8G8R8A8_UNORM mode descriptor.
// ============================================================================
uint32_t GXDisplayOutput_GetDefaultModeDesc1080p60(
    void* unusedSelf,
    uint32_t unusedAdapterIdx,
    uint32_t unusedModeIdx,
    GXDisplayModeDesc* outModeDesc)
{
    outModeDesc->width          = 1920;  // 0x780
    outModeDesc->height         = 1080;  // 0x438
    outModeDesc->refreshRateNum = 60000; // 0xEA60
    outModeDesc->refreshRateDen = 1000;  // 0x3E8
    outModeDesc->pixelFormat    = 0x57;  // 87 (DXGI_FORMAT_B8G8R8A8_UNORM)
    outModeDesc->scanlineOrder  = 1;
    outModeDesc->scalingMode    = 0;
    return (unusedSelf != nullptr || unusedAdapterIdx != 0 || unusedModeIdx != 0) ? 0u : 0u;
}

// ============================================================================
// 8. VMA 0x26dbdb0 (78 B) — GXDisplayOutput_GetDefaultOutputDesc
//    Populates "\\\\.\\DISPLAY1" and 1920x1080 desktop coordinates.
// ============================================================================
void GXDisplayOutput_GetDefaultOutputDesc(
    void* unusedSelf,
    GXDisplayOutputDesc* outDesc)
{
    (void)unusedSelf;
    DLCharTraitsW_CopyString(outDesc->deviceName, reinterpret_cast<const char16_t*>(GX_VMA(0x4871540ULL)));
    outDesc->desktopLeft         = 0;
    outDesc->desktopTop          = 0;
    outDesc->desktopRight        = 1920; // 0x780
    outDesc->desktopBottom       = 1080; // 0x438
    outDesc->isAttachedToDesktop = 1;
    outDesc->rotationMode        = 1;
    outDesc->monitorHandle       = nullptr;
}

// ============================================================================
// 9. VMA 0x26dbda0 (1 B) — GXDisplayOutput_NoOpA
// ============================================================================
void GXDisplayOutput_NoOpA() {}

// ============================================================================
// 10. VMA 0x26dbd90 (3 B) — GXDisplayOutput_ReturnZero
// ============================================================================
uint32_t GXDisplayOutput_ReturnZero() {
    return 0;
}

// ============================================================================
// 11. VMA 0x26dbcf0 (4 B) — GXDisplayOutput_ReturnSelf
// ============================================================================
void* GXDisplayOutput_ReturnSelf(void* self) {
    return self;
}

// ============================================================================
// 12. VMA 0x26dbce0 (5 B) — GXDisplayOutput_GetDefaultModeDescThunk
// ============================================================================
uint32_t GXDisplayOutput_GetDefaultModeDescThunk(
    void* self,
    uint32_t adapterIdx,
    uint32_t modeIdx,
    GXDisplayModeDesc* outModeDesc)
{
    return GXDisplayOutput_GetDefaultModeDesc1080p60(self, adapterIdx, modeIdx, outModeDesc);
}

// ============================================================================
// 13. VMA 0x26dbcd0 (5 B) — GXDisplayOutput_GetDefaultOutputDescThunk
// ============================================================================
void GXDisplayOutput_GetDefaultOutputDescThunk(
    void* self,
    GXDisplayOutputDesc* outDesc)
{
    GXDisplayOutput_GetDefaultOutputDesc(self, outDesc);
}

// ============================================================================
// 14. VMA 0x26dbcc0 (5 B) — GXDisplayOutput_NoOpThunk
// ============================================================================
void GXDisplayOutput_NoOpThunk() {
    GXDisplayOutput_NoOpA();
}

// ============================================================================
// 15. VMA 0x26dbcb0 (1 B) — GXDisplayOutput_NoOpB
// ============================================================================
void GXDisplayOutput_NoOpB() {}

// ============================================================================
// 16. VMA 0x26dbc10 (1 B) — GXDisplayOutput_NoOpC
// ============================================================================
void GXDisplayOutput_NoOpC() {}

// ============================================================================
// 17. VMA 0x26dbbe0 (39 B) — GXDisplayConfig_CopyFromGlobalCGDisplayConfig
//     Copies the 0xC0-byte global CGDisplayConfig structure (0x26d4320).
// ============================================================================
uint32_t GXDisplayConfig_CopyFromGlobalCGDisplayConfig(void* outDisplayConfigC0) {
    const void* srcConfig = CGDisplayConfig_GetGlobalInstance();
    std::memcpy(outDisplayConfigC0, srcConfig, 0xC0);
    return 0;
}

// ============================================================================
// 18. VMA 0x26dbbd0 (1 B) — GXDisplayConfig_NoOpD
// ============================================================================
void GXDisplayConfig_NoOpD() {}

// ============================================================================
// 19. VMA 0x26dbbc0 (12 B) — GXDisplayConfig_ClearFieldC0
// ============================================================================
void GXDisplayConfig_ClearFieldC0(uint8_t* self) {
    *reinterpret_cast<uint64_t*>(self + 0xC0) = 0;
}

// ============================================================================
// 20. VMA 0x26db300 (65 B) — GXDynamicBufferRingCursor_UnmapAndGetBinding
//     Unmaps the dynamic buffer on cmdCtx and writes outBufferObj and
//     outLastMappedOffset.
// ============================================================================
void GXDynamicBufferRingCursor_UnmapAndGetBinding(
    GXDynamicBufferRingCursor* self,
    GXGnmRefCountedBuffer** outBufferObj,
    uint32_t* outLastMappedOffset,
    uint8_t* cmdCtx)
{
    GXGnmCommandContext_UnmapBuffer(cmdCtx, cmdCtx + 0xB730, self->bufferObj, 0);
    *outBufferObj         = self->bufferObj;
    *outLastMappedOffset  = self->lastMappedOffset;
}

// ============================================================================
// 21. VMA 0x26db1b0 (323 B) — GXDynamicBufferRingCursor_AllocateAndMapSlice
//     Sub-allocates an aligned slice of requestedBytes from the dynamic ring
//     buffer. Uses MAP_WRITE_NO_OVERWRITE (3) if the slice fits before
//     totalByteSize, or wraps around with MAP_WRITE_DISCARD (4) if full.
// ============================================================================
uint32_t GXDynamicBufferRingCursor_AllocateAndMapSlice(
    GXDynamicBufferRingCursor* self,
    void** outMappedCpuPtr,
    uint8_t* cmdCtx,
    uint32_t requestedBytes)
{
    if (self->totalByteSize < requestedBytes) {
        return 9;
    }

    uint32_t align     = self->alignment;
    uint32_t alignMask = align - 1u;
    uint32_t alignedSize = (requestedBytes + alignMask) & ~alignMask;

    void* baseMappedPtr = nullptr;

    if (self->nextFreeOffset + requestedBytes <= self->totalByteSize) {
        // Try MAP_WRITE_NO_OVERWRITE (3) or check if MAP_WRITE_DISCARD (4) is required first
        uint32_t slotId = GXGnmCommandContext_GetContextSlotId(cmdCtx);
        uint32_t status = GXGnmBuffer_ValidateMapMode(self->bufferObj, 4, slotId);
        if (status == 0) {
            status = GXGnmCommandContext_MapBuffer(
                cmdCtx, cmdCtx + 0xB730, self->bufferObj, 0, 4, &baseMappedPtr);
            if (status == 0) {
                self->lastMappedOffset = self->nextFreeOffset;
                self->nextFreeOffset  += alignedSize;
                *outMappedCpuPtr = static_cast<uint8_t*>(baseMappedPtr) + self->lastMappedOffset;
                return 0;
            }
        }
        if (status != 4) {
            return 9;
        }
    }

    // Map with mode 3 (MAP_WRITE_NO_OVERWRITE / ring continuation)
    uint32_t slotId = GXGnmCommandContext_GetContextSlotId(cmdCtx);
    if (GXGnmBuffer_ValidateMapMode(self->bufferObj, 3, slotId) != 0) {
        return 9;
    }
    if (GXGnmCommandContext_MapBuffer(
            cmdCtx, cmdCtx + 0xB730, self->bufferObj, 0, 3, &baseMappedPtr) != 0) {
        return 9;
    }

    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(baseMappedPtr);
    uint32_t padBytes = static_cast<uint32_t>(((rawAddr + alignMask) & ~static_cast<uintptr_t>(alignMask)) - rawAddr);
    self->lastMappedOffset = padBytes;
    self->nextFreeOffset   = padBytes + alignedSize;
    *outMappedCpuPtr       = static_cast<uint8_t*>(baseMappedPtr) + self->lastMappedOffset;
    return 0;
}

// ============================================================================
// 22. VMA 0x26db1a0 (1 B) — GXDynamicBufferRingCursor_Dtor
// ============================================================================
void GXDynamicBufferRingCursor_Dtor(GXDynamicBufferRingCursor* self) {
    if (self != nullptr) {}
}

// ============================================================================
// 23. VMA 0x26db140 (92 B) — GXDynamicBufferRingCursor_Ctor
//     Initializes the ring cursor and queries totalByteSize from hwBuffer.
// ============================================================================
void GXDynamicBufferRingCursor_Ctor(
    GXDynamicBufferRingCursor* self,
    uint32_t* unusedOutStatus,
    DLAllocator* unusedAllocator,
    GXGnmRefCountedBuffer* bufferObj,
    uint32_t alignment)
{
    self->bufferObj        = bufferObj;
    self->alignment        = alignment;
    self->nextFreeOffset   = 0;
    self->lastMappedOffset = 0;

    GXBufferDescriptor24 desc{};
    GXGnmBuffer_ExtractDescriptorFromHardware(bufferObj->hwBuffer, &desc);
    self->totalByteSize = desc.byteSize;
    if (unusedOutStatus != nullptr && unusedAllocator == nullptr) {}
}

// ============================================================================
// 24. VMA 0x26db060 (209 B) — GXDynamicBufferRingOwner_CreateCursorInstance
//     Allocates and constructs a 0x18-byte GXDynamicBufferRingCursor bound to
//     self->bufferObj.
// ============================================================================
uint32_t GXDynamicBufferRingOwner_CreateCursorInstance(
    GXDynamicBufferRingOwner* self,
    GXDynamicBufferRingCursor** outCursor,
    uint32_t alignment,
    DLAllocator* overrideAllocator)
{
    DLAllocator* alloc = (overrideAllocator != nullptr) ? overrideAllocator : self->allocator;
    uint32_t status = 0;

    GXDynamicBufferRingCursor* cursor = static_cast<GXDynamicBufferRingCursor*>(
        DLHeap_AllocateAligned(sizeof(GXDynamicBufferRingCursor), 8, alloc));
    if (cursor == nullptr) {
        *outCursor = nullptr;
        return 9;
    }

    GXDynamicBufferRingCursor_CtorThunk(cursor, &status, alloc, self->bufferObj, alignment, 0);
    *outCursor = cursor;

    if (status != 0) {
        GXDynamicBufferRingCursor_DtorThunk(cursor);
        alloc->vptr->deallocate(alloc, cursor);
        *outCursor = nullptr;
        return status;
    }
    return 0;
}

// ============================================================================
// 25. VMA 0x26db050 (5 B) — GXDynamicBufferRingOwner_GetCursor
// ============================================================================
GXDynamicBufferRingCursor* GXDynamicBufferRingOwner_GetCursor(const GXDynamicBufferRingOwner* self) {
    return self->ringCursor;
}

// ============================================================================
// 26. VMA 0x26dafd0 (124 B) — GXDynamicBufferRingOwner_Destroy
//     Destroys and frees ringCursor, then decrements bufferObj->refCount and
//     destroys bufferObj if its refCount reached 1 -> 0.
// ============================================================================
void GXDynamicBufferRingOwner_Destroy(GXDynamicBufferRingOwner* self) {
    GXDynamicBufferRingCursor* cursor = self->ringCursor;
    if (cursor != nullptr) {
        DLAllocator* alloc = self->allocator;
        GXDynamicBufferRingCursor_DtorThunk(cursor);
        alloc->vptr->deallocate(alloc, cursor);
        self->ringCursor = nullptr;
    }

    GXGnmRefCountedBuffer* buf = self->bufferObj;
    if (DLAtomic_FetchDecrement32(&buf->refCount) == 1 && buf != nullptr) {
        DLAllocator* bufAlloc = buf->allocator;
        buf->vptr->dtor(buf);
        bufAlloc->vptr->deallocate(bufAlloc, buf);
    }
}

// ============================================================================
// 27. VMA 0x26dae20 (421 B) — GXDynamicBufferRingOwner_Initialize
//     Creates a dynamic hardware buffer via GXGnmDevice_CreateBuffer (0x2165c10)
//     and allocates the primary GXDynamicBufferRingCursor.
// ============================================================================
void GXDynamicBufferRingOwner_Initialize(
    GXDynamicBufferRingOwner* self,
    uint32_t* outStatus,
    void* gxDevice,
    uint32_t byteSize,
    uint32_t alignment,
    DLAllocator* allocator,
    void* ownerContext,
    uint8_t isConstantBuffer)
{
    self->allocator  = allocator;
    self->ringCursor = nullptr;
    self->bufferObj  = nullptr;

    GXBufferDescriptor24 bufDesc{};
    bufDesc.byteSize            = byteSize;
    bufDesc.usageType           = 2; // Dynamic usage
    bufDesc.bindFlags           = 3; // Vertex / Index / Constant bind
    bufDesc.cpuAccessFlags      = 1; // CPU write
    bufDesc.miscFlags           = (isConstantBuffer == 0) ? 0x800u : 0u;
    bufDesc.structureByteStride = 0;

    uint32_t createErr = GXGnmDevice_CreateBuffer(
        gxDevice, &self->bufferObj, &bufDesc, nullptr, allocator, ownerContext, 0);
    if (createErr != 0) {
        *outStatus = createErr;
        return;
    }

    GXDynamicBufferRingCursor* cursor = static_cast<GXDynamicBufferRingCursor*>(
        DLHeap_AllocateAligned(sizeof(GXDynamicBufferRingCursor), 8, allocator));
    if (cursor == nullptr) {
        self->ringCursor = nullptr;
        GXGnmRefCountedBuffer* buf = self->bufferObj;
        if (DLAtomic_FetchDecrement32(&buf->refCount) == 1 && buf != nullptr) {
            DLAllocator* bufAlloc = buf->allocator;
            buf->vptr->dtor(buf);
            bufAlloc->vptr->deallocate(bufAlloc, buf);
        }
        *outStatus = 9;
        return;
    }

    GXDynamicBufferRingCursor_CtorThunk(cursor, outStatus, allocator, self->bufferObj, alignment, 0);
    self->ringCursor = cursor;

    if (*outStatus != 0) {
        GXGnmRefCountedBuffer* buf = self->bufferObj;
        if (DLAtomic_FetchDecrement32(&buf->refCount) == 1 && buf != nullptr) {
            DLAllocator* bufAlloc = buf->allocator;
            buf->vptr->dtor(buf);
            bufAlloc->vptr->deallocate(bufAlloc, buf);
        }
        return;
    }

    *outStatus = 0;
}

// ============================================================================
// 28..46: TPF Texture Entry V2 Strategy Methods (0x24-byte GXTpfEntryHeaderV2)
// ============================================================================

// 28. VMA 0x26dae00 (17 B) — GXTpfEntryV2_ExtractExtraMetadata
void GXTpfEntryV2_ExtractExtraMetadata(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV2* view,
    GXTpfExtraMetadataOut* outMeta)
{
    (void)unusedSelf;
    outMeta->flags01    = 0x0101;
    outMeta->extValue0C = view->entryHdr->headerExtValue;
}

// 29. VMA 0x26dadf0 (10 B) — GXTpfEntryV2_GetPayloadPointer
const uint8_t* GXTpfEntryV2_GetPayloadPointer(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV2* view)
{
    return (unusedSelf != nullptr ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
}

// 30. VMA 0x26dade0 (6 B) — GXTpfEntryV2_GetDefaultOneA
uint32_t GXTpfEntryV2_GetDefaultOneA() {
    return 1;
}

// 31. VMA 0x26dadd0 (6 B) — GXTpfEntryV2_GetDefaultOneB
uint32_t GXTpfEntryV2_GetDefaultOneB() {
    return 1;
}

// 32. VMA 0x26dadc0 (9 B) — GXTpfEntryV2_GetFlags
int32_t GXTpfEntryV2_GetFlags(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->flags : view->entryHdr->flags;
}

// 33. VMA 0x26dadb0 (12 B) — GXTpfEntryV2_GetEnginePixelFormat
uint32_t GXTpfEntryV2_GetEnginePixelFormat(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV2* view)
{
    uint32_t dxgiFmt = (unusedSelf != nullptr) ? view->entryHdr->dxgiFormat : view->entryHdr->dxgiFormat;
    return GXTpfFormat_DxgiToEngineFormat(dxgiFmt);
}

// 34. VMA 0x26dad90 (19 B) — GXTpfEntryV2_GetEffectiveMipCount
uint32_t GXTpfEntryV2_GetEffectiveMipCount(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV2* view)
{
    int32_t mips = (unusedSelf != nullptr) ? view->entryHdr->mipCount : view->entryHdr->mipCount;
    return (mips != 0) ? static_cast<uint32_t>(mips) : 1u;
}

// 35. VMA 0x26dad70 (19 B) — GXTpfEntryV2_GetSliceCount
uint32_t GXTpfEntryV2_GetSliceCount(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV2* view)
{
    const GXTpfEntryHeaderV2* hdr = (unusedSelf != nullptr) ? view->entryHdr : view->entryHdr;
    return (hdr->textureType == 1) ? 6u : hdr->depthOrArraySize;
}

// 36. VMA 0x26dad60 (8 B) — GXTpfEntryV2_GetDepth
uint32_t GXTpfEntryV2_GetDepth(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->depthOrArraySize : view->entryHdr->depthOrArraySize;
}

// 37. VMA 0x26dad50 (9 B) — GXTpfEntryV2_GetHeight
uint32_t GXTpfEntryV2_GetHeight(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->height : view->entryHdr->height;
}

// 38. VMA 0x26dad40 (9 B) — GXTpfEntryV2_GetWidth
uint32_t GXTpfEntryV2_GetWidth(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->width : view->entryHdr->width;
}

// 39. VMA 0x26dad30 (5 B) — GXTpfEntryV2_GetHeaderPtr
const GXTpfEntryHeaderV2* GXTpfEntryV2_GetHeaderPtr(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV2* view)
{
    return (unusedSelf != nullptr) ? view->entryHdr : view->entryHdr;
}

// 40. VMA 0x26dad20 (6 B) — GXTpfEntryV2_GetHeaderByteSize
uint32_t GXTpfEntryV2_GetHeaderByteSize() {
    return 0x24;
}

// 41. VMA 0x26dad10 (8 B) — GXTpfEntryV2_GetHeaderExtSize
uint32_t GXTpfEntryV2_GetHeaderExtSize(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->headerExtSize : view->entryHdr->headerExtSize;
}

// 42. VMA 0x26dad00 (8 B) — GXTpfEntryV2_GetFileNameOffset
uint32_t GXTpfEntryV2_GetFileNameOffset(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->fileNameOffset : view->entryHdr->fileNameOffset;
}

// 43. VMA 0x26dacf0 (9 B) — GXTpfEntryV2_GetFormatCode
uint32_t GXTpfEntryV2_GetFormatCode(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->formatCode : view->entryHdr->formatCode;
}

// 44. VMA 0x26dace0 (9 B) — GXTpfEntryV2_GetTextureType
uint32_t GXTpfEntryV2_GetTextureType(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->textureType : view->entryHdr->textureType;
}

// 45. VMA 0x26dacd0 (8 B) — GXTpfEntryV2_GetDataSize
uint32_t GXTpfEntryV2_GetDataSize(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->dataSize : view->entryHdr->dataSize;
}

// 46. VMA 0x26dacc0 (7 B) — GXTpfEntryV2_GetDataOffset
uint32_t GXTpfEntryV2_GetDataOffset(const void* unusedSelf, const GXTpfTextureSourceViewV2* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->dataOffset : view->entryHdr->dataOffset;
}

// 47. VMA 0x26dacb0 (3 B) — GXTpfEntry_ReturnTrue
bool GXTpfEntry_ReturnTrue() {
    return true;
}

// 48. VMA 0x26daca0 (1 B) — GXTpfEntryV1_ExtractExtraMetadataNoOp
void GXTpfEntryV1_ExtractExtraMetadataNoOp() {}

// ============================================================================
// 49..65: TPF Texture Entry V1 Strategy Methods (0x14-byte GXTpfEntryHeaderV1
//         with embedded DDS header at tpfBase + entryHdr->dataOffset)
// ============================================================================

// 49. VMA 0x26dac70 (42 B) — GXTpfEntryV1_GetSubresourcePayloadPointer
const uint8_t* GXTpfEntryV1_GetSubresourcePayloadPointer(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view,
    uint32_t mipIndex,
    uint32_t sliceIndex)
{
    const uint8_t* tpfBase = (unusedSelf != nullptr) ? view->tpfBase : view->tpfBase;
    uint32_t ddsOffset     = view->entryHdr->dataOffset;
    uint32_t subOff        = GXDdsHeader_GetSubresourceOffset(tpfBase + ddsOffset, mipIndex, sliceIndex);
    return tpfBase + ddsOffset + subOff;
}

// 50. VMA 0x26dac60 (16 B) — GXTpfEntryV1_GetMipSliceByteSize
uint32_t GXTpfEntryV1_GetMipSliceByteSize(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view,
    uint32_t mipIndex)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return GXDdsHeader_GetMipSliceByteSize(ddsPtr, mipIndex);
}

// 51. VMA 0x26dac50 (16 B) — GXTpfEntryV1_GetMipSlicePitchOrRowSize
uint32_t GXTpfEntryV1_GetMipSlicePitchOrRowSize(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view,
    uint32_t mipIndex)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return GXDdsHeader_GetMipSlicePitchOrRowSize(ddsPtr, mipIndex);
}

// 52. VMA 0x26dac40 (9 B) — GXTpfEntryV1_GetFlags
int32_t GXTpfEntryV1_GetFlags(const void* unusedSelf, const GXTpfTextureSourceViewV1* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->flags : view->entryHdr->flags;
}

// 53. VMA 0x26dac30 (14 B) — GXTpfEntryV1_GetEnginePixelFormat
uint32_t GXTpfEntryV1_GetEnginePixelFormat(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return GXDdsHeader_GetEnginePixelFormat(ddsPtr);
}

// 54. VMA 0x26dac10 (24 B) — GXTpfEntryV1_GetEffectiveMipCount
uint32_t GXTpfEntryV1_GetEffectiveMipCount(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    uint32_t mips = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x1C);
    return (mips != 0u) ? mips : 1u;
}

// 55. VMA 0x26dac00 (14 B) — GXTpfEntryV1_GetSliceCount
uint32_t GXTpfEntryV1_GetSliceCount(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return GXDdsHeader_GetArrayOrCubeSliceCount(ddsPtr);
}

// 56. VMA 0x26dabf0 (14 B) — GXTpfEntryV1_GetDepth
uint32_t GXTpfEntryV1_GetDepth(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return *reinterpret_cast<const uint32_t*>(ddsPtr + 0x18);
}

// 57. VMA 0x26dabe0 (14 B) — GXTpfEntryV1_GetHeight
uint32_t GXTpfEntryV1_GetHeight(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return *reinterpret_cast<const uint32_t*>(ddsPtr + 0x0C);
}

// 58. VMA 0x26dabd0 (14 B) — GXTpfEntryV1_GetWidth
uint32_t GXTpfEntryV1_GetWidth(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    const uint8_t* ddsPtr = ((unusedSelf != nullptr) ? view->tpfBase : view->tpfBase) + view->entryHdr->dataOffset;
    return *reinterpret_cast<const uint32_t*>(ddsPtr + 0x10);
}

// 59. VMA 0x26dabc0 (5 B) — GXTpfEntryV1_GetHeaderPtr
const GXTpfEntryHeaderV1* GXTpfEntryV1_GetHeaderPtr(
    const void* unusedSelf,
    const GXTpfTextureSourceViewV1* view)
{
    return (unusedSelf != nullptr) ? view->entryHdr : view->entryHdr;
}

// 60. VMA 0x26dabb0 (6 B) — GXTpfEntryV1_GetHeaderByteSize
uint32_t GXTpfEntryV1_GetHeaderByteSize() {
    return 0x14;
}

// 61. VMA 0x26daba0 (8 B) — GXTpfEntryV1_GetHeaderExtSize
uint32_t GXTpfEntryV1_GetHeaderExtSize(const void* unusedSelf, const GXTpfTextureSourceViewV1* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->headerExtSize : view->entryHdr->headerExtSize;
}

// 62. VMA 0x26dab90 (8 B) — GXTpfEntryV1_GetFileNameOffset
uint32_t GXTpfEntryV1_GetFileNameOffset(const void* unusedSelf, const GXTpfTextureSourceViewV1* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->fileNameOffset : view->entryHdr->fileNameOffset;
}

// 63. VMA 0x26dab80 (9 B) — GXTpfEntryV1_GetFormatCode
uint32_t GXTpfEntryV1_GetFormatCode(const void* unusedSelf, const GXTpfTextureSourceViewV1* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->formatCode : view->entryHdr->formatCode;
}

// 64. VMA 0x26dab70 (9 B) — GXTpfEntryV1_GetTextureType
uint32_t GXTpfEntryV1_GetTextureType(const void* unusedSelf, const GXTpfTextureSourceViewV1* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->textureType : view->entryHdr->textureType;
}

// 65. VMA 0x26dab60 (8 B) — GXTpfEntryV1_GetDataSize
uint32_t GXTpfEntryV1_GetDataSize(const void* unusedSelf, const GXTpfTextureSourceViewV1* view) {
    return (unusedSelf != nullptr) ? view->entryHdr->dataSize : view->entryHdr->dataSize;
}

} // extern "C"

} // namespace GX
} // namespace FromSoftware
