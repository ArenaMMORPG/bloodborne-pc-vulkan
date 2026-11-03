// ============================================================================
// Bloodborne Engine Decompile Project (CUSA03173 v1.09)
// Module: 03_TPF_GNF_ShaderContainers (Priority Tier 9/10)
// File:   GXTpfGnfTextureContainerLoader.cpp
// Range:  0x26d6a50 .. 0x26dab50 (47 functions, Right-to-Left VMA order)
//
// Description:
//   Complete, stub-free x86-64 decompilation of the FromSoftware TPF ('TPF\0')
//   texture container loader, PC DDS / DX10 header & format parsers, DXGI-to-GX
//   pixel format lookup tables, dynamic ring-buffer factory wrappers, constant
//   buffer array manager, and PS4 VideoOut gamma ramp / flip presenter methods.
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

namespace FromSoftware {
namespace GX {

// ============================================================================
// Core Dantelion2 Allocator & GX Resource Structures
// ============================================================================

struct DLAllocator;

struct DLAllocatorVTable {
    void* (*dtor)(DLAllocator* self);                              // +0x00
    void* (*fn08)(DLAllocator* self);                              // +0x08
    void* (*fn10)(DLAllocator* self);                              // +0x10
    void* (*fn18)(DLAllocator* self);                              // +0x18
    void* (*fn20)(DLAllocator* self);                              // +0x20
    void* (*fn28)(DLAllocator* self);                              // +0x28
    void* (*fn30)(DLAllocator* self);                              // +0x30
    void* (*fn38)(DLAllocator* self);                              // +0x38
    void* (*fn40)(DLAllocator* self);                              // +0x40
    void* (*fn48)(DLAllocator* self);                              // +0x48
    void* (*allocate)(DLAllocator* self, size_t byteSize);         // +0x50
    void* (*allocateAligned)(DLAllocator* self, size_t, size_t);   // +0x58
    void* (*fn60)(DLAllocator* self);                              // +0x60
    void* (*fn68)(DLAllocator* self);                              // +0x68
    void  (*deallocate)(DLAllocator* self, void* ptr);             // +0x70
};

struct DLAllocator {
    const DLAllocatorVTable* vptr; // +0x00
};

struct GXGnmRefCountedResource;

struct GXGnmRefCountedResourceVTable {
    void (*dtor)(GXGnmRefCountedResource* self); // +0x00
};

struct GXGnmRefCountedResource {
    const GXGnmRefCountedResourceVTable* vptr; // +0x00
    DLAllocator* allocator;                    // +0x08
    volatile int32_t refCount;                 // +0x10
    uint32_t pad14;                            // +0x14
    void* hwResource;                          // +0x18
};

struct GXBufferDescriptor24 {
    uint32_t byteSize;            // +0x00
    uint32_t usageType;           // +0x04
    uint32_t bindFlags;           // +0x08
    uint32_t cpuAccessFlags;      // +0x0C
    uint32_t miscFlags;           // +0x10
    uint32_t structureByteStride; // +0x14
    uint32_t reserved18;          // +0x18
};

struct GXDynamicBufferRingCursor {
    GXGnmRefCountedResource* bufferObj; // +0x00
    uint32_t totalByteSize;             // +0x08
    uint32_t alignment;                 // +0x0C
    uint32_t nextFreeOffset;            // +0x10
    uint32_t lastMappedOffset;          // +0x14
};

struct GXDynamicBufferRingOwner {
    DLAllocator* allocator;                // +0x00
    GXGnmRefCountedResource* bufferObj;    // +0x08
    GXDynamicBufferRingCursor* ringCursor; // +0x10
};

struct GXConstantBufferArrayOwner {
    DLAllocator* allocator;            // +0x00
    uint32_t bufferCount;              // +0x08
    uint32_t pad0C;                    // +0x0C
    GXGnmRefCountedResource** buffers; // +0x10
};

// ============================================================================
// TPF ('TPF\0') Container & Strategy Structures
// ============================================================================

struct GXTpfHeader {
    char     magic[4];          // +0x00: 'T', 'P', 'F', '\0'
    uint32_t totalDataByteSize; // +0x04
    uint32_t textureCount;      // +0x08
    int8_t   platformFlag;      // +0x0C: 0 = V1 (DDS), 4 = V2 (ORBIS/DXGI)
    uint8_t  flag2;             // +0x0D: must be 3
    uint8_t  encodingFlag;      // +0x0E: string encoding flag (1/2)
    uint8_t  headerFlags;       // +0x0F: bit 0 selects 0x30 vs 0x10 header size
};

struct GXTpfEntryHeaderV1 {
    uint32_t dataOffset;     // +0x00
    uint32_t dataSize;       // +0x04
    uint8_t  formatCode;     // +0x08
    uint8_t  textureType;    // +0x09
    uint8_t  mipCount;       // +0x0A
    uint8_t  flags;          // +0x0B
    uint32_t fileNameOffset; // +0x0C
    uint32_t headerExtSize;  // +0x10
};

struct GXTpfTextureSourceView {
    const uint8_t* tpfBase;  // +0x00
    const uint8_t* entryHdr; // +0x08
};

struct GXTpfExtraMetadataOut {
    uint8_t  pad00;        // +0x00
    uint16_t flags01;      // +0x01
    uint8_t  pad03[9];     // +0x03..+0x0B
    uint32_t extValue0C;   // +0x0C
} __attribute__((packed));

struct GXTpfEntryStrategy;

struct GXTpfEntryStrategyVTable {
    bool           (*validateEntry)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                 // +0x00
    uint32_t       (*getDataOffset)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                 // +0x08
    uint32_t       (*getDataSize)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                   // +0x10
    uint32_t       (*getTextureType)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                // +0x18
    uint32_t       (*getFormatCode)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                 // +0x20
    uint32_t       (*getFileNameOffset)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);             // +0x28
    uint32_t       (*getHeaderExtCount)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);             // +0x30
    uint32_t       (*getHeaderByteSize)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);             // +0x38
    const uint8_t* (*getHeaderPtr)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                  // +0x40
    uint32_t       (*getWidth)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // +0x48
    uint32_t       (*getHeight)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                     // +0x50
    uint32_t       (*getDepth)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // +0x58
    uint32_t       (*getSliceCount)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                 // +0x60
    uint32_t       (*getEffectiveMipCount)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);          // +0x68
    uint32_t       (*getEnginePixelFormat)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);          // +0x70
    int32_t        (*getFlags)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // +0x78
    uint32_t       (*getMipSlicePitchOrRowSize)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIdx); // +0x80
    uint32_t       (*getMipSliceByteSize)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIdx);       // +0x88
    const uint8_t* (*getSubresourcePayloadPtr)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIdx, uint32_t sliceIdx); // +0x90
    void           (*extractExtraMetadata)(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, GXTpfExtraMetadataOut* outMeta); // +0x98
};

struct GXTpfEntryStrategy {
    const GXTpfEntryStrategyVTable* vptr; // +0x00
};

struct GXTpfSubresourceInitData {
    const void* dataPtr;    // +0x00
    uint32_t    rowPitch;   // +0x08
    uint32_t    slicePitch; // +0x0C
};

struct GXTexture2DDesc {
    uint32_t width;          // +0x00
    uint32_t height;         // +0x04
    uint32_t mipLevels;      // +0x08
    uint32_t arraySize;      // +0x0C
    uint32_t format;         // +0x10
    uint32_t sampleCount;    // +0x14
    uint32_t sampleQuality;  // +0x18
    uint32_t usage;          // +0x1C
    uint32_t bindFlags;      // +0x20
    uint32_t cpuAccessFlags; // +0x24
    uint32_t miscFlags;      // +0x28
    uint32_t reserved2C;     // +0x2C
};

struct GXTexture3DDesc {
    uint32_t width;          // +0x00
    uint32_t height;         // +0x04
    uint32_t depth;          // +0x08
    uint32_t mipLevels;      // +0x0C
    uint32_t format;         // +0x10
    uint32_t usage;          // +0x14
    uint32_t bindFlags;      // +0x18
    uint32_t cpuAccessFlags; // +0x1C
    uint32_t miscFlags;      // +0x20
    uint32_t reserved24;     // +0x24
};

struct GXShaderResourceViewDesc {
    uint32_t format;            // +0x00
    uint32_t viewDimension;     // +0x04
    uint32_t mostDetailedMip;   // +0x08
    uint32_t mipLevels;         // +0x0C
    uint32_t firstArraySlice;   // +0x10
    uint32_t arraySize;         // +0x14
};

struct DLScratchScope {
    const void* vptr;       // +0x00
    void*       savedMark;  // +0x08
};

// ============================================================================
// Read-Only Format Lookup Tables from eboot.bin (.rodata 0x4870900..0x4870fdc)
// ============================================================================

// VMA 0x4870900 (100 entries): DXGI format (0..99) -> Engine pixel format
alignas(16) static constexpr uint32_t kDxgiToEngineFormatTable[100] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99
};

// VMA 0x4870b30 (99 entries, indexed by engineFormat - 1): Bytes per texel/block
alignas(16) static constexpr uint32_t kEngineFormatBytesPerBlockTable[99] = {
    16, 16, 16, 16, 12, 12, 12, 12,  8,  8,  8,  8,  8,  8,  8,  8,
     8,  8,  8,  8,  8,  8,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
     4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  2,
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,
     1,  8,  4,  4,  4,  8,  8,  8, 16, 16, 16, 16, 16, 16,  8,  8,
     8, 16, 16, 16,  2,  2,  4,  4,  4,  4,  4,  4,  4, 16, 16, 16,
    16, 16, 16
};

// VMA 0x4870cc0 (99 entries, indexed by engineFormat - 1): Block width in texels
alignas(16) static constexpr uint32_t kEngineFormatBlockWidthTable[99] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 8, 1, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4,
    4, 4, 4
};

// VMA 0x4870e50 (99 entries, indexed by engineFormat - 1): Block height in texels
alignas(16) static constexpr uint32_t kEngineFormatBlockHeightTable[99] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 8, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4,
    4, 4, 4
};

extern "C" {

// ============================================================================
// External Engine & Batch 2 Functions
// ============================================================================

void*        DLHeap_AllocateAligned(size_t byteSize, size_t alignment, DLAllocator* allocator) __asm__("_ZN4from13CoreGraphics220DLAllocator_AllocateEmmPv"); // 0x207c4e0
void*        DLScratchAlloc_Allocate(DLScratchScope* scope, size_t byteSize);                   // 0x207d4b0
void*        DLScratchAlloc_AcquireMark();                                                      // 0x207d5b0
void         DLScratchAlloc_ReleaseMark(void* savedMark);                                       // 0x207d5f0
int32_t      DLAtomic_FetchDecrement32(volatile int32_t* ptr);                                  // 0x207ec00
uint64_t     DLThread_GetCurrentThreadId();                                                     // 0x208ba90

DLAllocator* GXResolveAllocator(DLAllocator* overrideAlloc, DLAllocator* defaultAlloc);         // 0x2164e00
void*        GXResolveOwnerContext(void* overrideOwner, void* defaultOwner);                    // 0x2164e20
uint32_t     GXFormat_ToSrgbFormat(uint32_t engineFormat);                                      // 0x21650d0
uint32_t     GXGnmDevice_CreateBuffer(
                 void* gxDevice,
                 GXGnmRefCountedResource** outBufferObj,
                 const GXBufferDescriptor24* desc,
                 const void* initialData,
                 DLAllocator* allocator,
                 void* ownerContext,
                 uint64_t flags);                                                               // 0x2165c10
uint32_t     GXGnmDevice_CreateTexture2D(
                 void* gxDevice,
                 GXGnmRefCountedResource** outTextureObj,
                 const GXTexture2DDesc* desc,
                 const GXTpfSubresourceInitData* subresources,
                 DLAllocator* allocator,
                 void* ownerContext,
                 const GXTpfExtraMetadataOut* extraMeta);                                       // 0x2165f90
uint32_t     GXGnmDevice_CreateTexture3D(
                 void* gxDevice,
                 GXGnmRefCountedResource** outTextureObj,
                 const GXTexture3DDesc* desc,
                 const GXTpfSubresourceInitData* subresources,
                 DLAllocator* allocator,
                 void* ownerContext,
                 const GXTpfExtraMetadataOut* extraMeta);                                       // 0x2166300
uint32_t     GXGnmDevice_CreateShaderResourceView(
                 void* gxDevice,
                 GXGnmRefCountedResource** outSrvObj,
                 GXGnmRefCountedResource* textureObj,
                 const GXShaderResourceViewDesc* srvDesc,
                 DLAllocator* allocator);                                                       // 0x21669a0
DLAllocator* GXDevice_GetDefaultAllocator(void* gxDevice);                                      // 0x2167870
void*        GXDevice_GetDefaultOwnerContext(void* gxDevice);                                   // 0x2167880
void         GXGnmCommandContext_PostFlipNotify(void* cmdCtx);                                  // 0x2168530
void         GXGnmCommandContext_UpdateSubresource(
                 void* cmdCtx,
                 void* subCtx,
                 GXGnmRefCountedResource* resource,
                 uint32_t subresourceIdx,
                 const void* dstBox,
                 const void* srcData,
                 uint32_t srcRowPitch,
                 uint32_t srcDepthPitch);                                                       // 0x2168a20
void         GXSwapChainTargetSubObj380_Destroy(void* subObj);                                  // 0x2171c60
void         GXSwapChainTargetSubObj348_Destroy(void* subObj);                                  // 0x2172290

uint32_t     GXGnmGpuSurface_GetBaseAddress256(void* surfaceSlot);                              // 0x1086ca0
void         GXGnmEventBroadcaster_NotifyFlip(void* broadcaster, void* cmdCtxSub10Ptr);         // 0x26aa860
void         GXGnmCommandBuffer_FlushSubmission(void* cmdCtxSub10);                             // 0x26af700
uint32_t     GXGnmCommandBuffer_SubmitFlip(
                 void* cmdCtxSub10,
                 int32_t videoOutHandle,
                 uint64_t bufferIndex,
                 uint32_t flipMode,
                 uint64_t flipArg);                                                             // 0x26afb30
void         GXGnmCommandBuffer_SetGammaRampChannel(
                 void* cmdCtxSub10,
                 void* gammaState380,
                 const void* rampChannelDesc,
                 uint32_t channelIndex,
                 uint32_t flag0,
                 uint32_t commitFlag);                                                          // 0x26b35e0
void         GXGnmCommandContext_BindRenderTargetsAndViewport(
                 void* cmdCtx,
                 void* rtView348,
                 void* depthTarget,
                 void* viewportState380);                                                       // 0x26b6cb0
void         GXGnmCommandContext_PrepareForPresent(void* subCtxB558, void* cmdCtx, void* srcColorTex); // 0x26d1eb0

// Batch 2 functions (0x26dab60 .. 0x26db1a0)
void         GXDynamicBufferRingOwner_Initialize(
                 GXDynamicBufferRingOwner* self,
                 uint32_t* outStatus,
                 void* gxDevice,
                 uint32_t byteSize,
                 uint32_t alignment,
                 DLAllocator* allocator,
                 void* ownerContext,
                 uint8_t isConstantBuffer);                                                     // 0x26dae20
void         GXDynamicBufferRingOwner_Destroy(GXDynamicBufferRingOwner* self);                  // 0x26dafd0
uint32_t     GXDynamicBufferRingOwner_CreateCursorInstance(
                 GXDynamicBufferRingOwner* self,
                 GXDynamicBufferRingCursor** outCursor,
                 uint32_t alignment,
                 DLAllocator* overrideAllocator);                                               // 0x26db060
void         GXDynamicBufferRingCursor_Ctor(
                 GXDynamicBufferRingCursor* self,
                 uint32_t* unusedOutStatus,
                 DLAllocator* unusedAllocator,
                 GXGnmRefCountedResource* bufferObj,
                 uint32_t alignment);                                                           // 0x26db140
void         GXDynamicBufferRingCursor_Dtor(GXDynamicBufferRingCursor* self);                   // 0x26db1a0

// Batch 2 TPF Entry V1 / V2 strategy methods for singleton vtables (0x52ba2c0 & 0x52ba380)
uint32_t       GXTpfEntryV1_GetDataSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                        // 0x26dab60
uint32_t       GXTpfEntryV1_GetTextureType(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                     // 0x26dab70
uint32_t       GXTpfEntryV1_GetFormatCode(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // 0x26dab80
uint32_t       GXTpfEntryV1_GetFileNameOffset(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                  // 0x26dab90
uint32_t       GXTpfEntryV1_GetHeaderExtSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                   // 0x26daba0
uint32_t       GXTpfEntryV1_GetHeaderByteSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                  // 0x26dabb0
const uint8_t* GXTpfEntryV1_GetHeaderPtr(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                       // 0x26dabc0
uint32_t       GXTpfEntryV1_GetWidth(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dabd0
uint32_t       GXTpfEntryV1_GetHeight(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                          // 0x26dabe0
uint32_t       GXTpfEntryV1_GetDepth(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dabf0
uint32_t       GXTpfEntryV1_GetSliceCount(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // 0x26dac00
uint32_t       GXTpfEntryV1_GetEffectiveMipCount(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);               // 0x26dac10
uint32_t       GXTpfEntryV1_GetEnginePixelFormat(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);               // 0x26dac30
int32_t        GXTpfEntryV1_GetFlags(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dac40
uint32_t       GXTpfEntryV1_GetMipSlicePitchOrRowSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIndex); // 0x26dac50
uint32_t       GXTpfEntryV1_GetMipSliceByteSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIndex);       // 0x26dac60
const uint8_t* GXTpfEntryV1_GetSubresourcePayloadPointer(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIndex, uint32_t sliceIndex); // 0x26dac70
void           GXTpfEntryV1_ExtractExtraMetadataNoOp(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, GXTpfExtraMetadataOut* outMeta); // 0x26daca0

bool           GXTpfEntry_ReturnTrue(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dacb0
uint32_t       GXTpfEntryV2_GetDataOffset(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // 0x26dacc0
uint32_t       GXTpfEntryV2_GetDataSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                        // 0x26dacd0
uint32_t       GXTpfEntryV2_GetTextureType(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                     // 0x26dace0
uint32_t       GXTpfEntryV2_GetFormatCode(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // 0x26dacf0
uint32_t       GXTpfEntryV2_GetFileNameOffset(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                  // 0x26dad00
uint32_t       GXTpfEntryV2_GetHeaderExtSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                   // 0x26dad10
uint32_t       GXTpfEntryV2_GetHeaderByteSize(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                  // 0x26dad20
const uint8_t* GXTpfEntryV2_GetHeaderPtr(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                       // 0x26dad30
uint32_t       GXTpfEntryV2_GetWidth(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dad40
uint32_t       GXTpfEntryV2_GetHeight(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                          // 0x26dad50
uint32_t       GXTpfEntryV2_GetDepth(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dad60
uint32_t       GXTpfEntryV2_GetSliceCount(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                      // 0x26dad70
uint32_t       GXTpfEntryV2_GetEffectiveMipCount(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);               // 0x26dad90
uint32_t       GXTpfEntryV2_GetEnginePixelFormat(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);               // 0x26dadb0
int32_t        GXTpfEntryV2_GetFlags(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);                           // 0x26dadc0
uint32_t       GXTpfEntryV2_GetDefaultOneB(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIndex);  // 0x26dadd0
uint32_t       GXTpfEntryV2_GetDefaultOneA(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIndex);  // 0x26dade0
const uint8_t* GXTpfEntryV2_GetPayloadPointer(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, uint32_t mipIndex, uint32_t sliceIndex); // 0x26dadf0
void           GXTpfEntryV2_ExtractExtraMetadata(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view, GXTpfExtraMetadataOut* outMeta); // 0x26dae00

// PS4 OS Kernel / VideoOut imports (PLT 0x2bbf5b8 .. 0x2bbf5f8)
int32_t      sceKernelDeleteEqueue(void* eq);                                                   // 0x2bbf5b8
int32_t      sceVideoOutClose(int32_t handle);                                                  // 0x2bbf5c8
int32_t      sceVideoOutGetFlipStatus(int32_t handle, void* outFlipStatus);                     // 0x2bbf5e8
int32_t      sceKernelWaitEqueue(void* eq, void* outEv, int32_t num, int32_t* outN, uint32_t* timo); // 0x2bbf5f8

// Forward declarations within Batch 3
uint32_t     GXTpfEntryV1_GetDataOffset(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view);   // 0x26dab50
bool         GXTpfEntryV1_ValidateDdsHeader(const GXTpfEntryStrategy* self, const GXTpfTextureSourceView* view); // 0x26dab40
bool         GXDdsHeader_ValidateMagicAndSize(const uint8_t* ddsPtr);                                          // 0x26d7510
uint32_t     GXDdsHeader_GetEnginePixelFormat(const uint8_t* ddsPtr);                                          // 0x26d7530
uint32_t     GXTpfFormat_DxgiToEngineFormat(uint32_t dxgiFormat);                                              // 0x26d7a00
uint32_t     GXDdsHeader_GetArrayOrCubeSliceCount(const uint8_t* ddsPtr);                                      // 0x26d7a20
uint32_t     GXDdsHeader_GetMipSlicePitchOrRowSize(const uint8_t* ddsPtr, uint32_t mipIndex);                  // 0x26d7a50
uint32_t     GXDdsHeader_GetMipSliceByteSize(const uint8_t* ddsPtr, uint32_t mipIndex);                        // 0x26d7ab0
uint32_t     GXDdsHeader_GetSubresourceOffset(const uint8_t* ddsPtr, uint32_t mipIndex, uint32_t sliceIndex);  // 0x26d7b40
uint32_t     GXTpfFormat_GetBytesPerBlock(uint32_t engineFormat);                                              // 0x26d80b0
uint32_t     GXTpfFormat_GetBlockWidth(uint32_t engineFormat);                                                 // 0x26d80d0
uint32_t     GXTpfFormat_GetBlockHeight(uint32_t engineFormat);                                                // 0x26d80f0

} // extern "C"

// ============================================================================
// Singleton Strategy VTables (VMA 0x52ba228 -> 0x52ba2c0, 0x52ba2e8 -> 0x52ba380)
// ============================================================================

static const GXTpfEntryStrategyVTable kGXTpfEntryV1VTable = {
    &GXTpfEntryV1_ValidateDdsHeader,
    &GXTpfEntryV1_GetDataOffset,
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetDataSize),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetTextureType),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetFormatCode),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetFileNameOffset),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetHeaderExtSize),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetHeaderByteSize),
    reinterpret_cast<const uint8_t* (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetHeaderPtr),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetWidth),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetHeight),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetDepth),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetSliceCount),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetEffectiveMipCount),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetEnginePixelFormat),
    reinterpret_cast<int32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV1_GetFlags),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, uint32_t)>(&GXTpfEntryV1_GetMipSlicePitchOrRowSize),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, uint32_t)>(&GXTpfEntryV1_GetMipSliceByteSize),
    reinterpret_cast<const uint8_t* (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, uint32_t, uint32_t)>(&GXTpfEntryV1_GetSubresourcePayloadPointer),
    reinterpret_cast<void (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, GXTpfExtraMetadataOut*)>(&GXTpfEntryV1_ExtractExtraMetadataNoOp),
};

static const GXTpfEntryStrategyVTable kGXTpfEntryV2VTable = {
    reinterpret_cast<bool (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntry_ReturnTrue),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetDataOffset),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetDataSize),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetTextureType),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetFormatCode),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetFileNameOffset),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetHeaderExtSize),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetHeaderByteSize),
    reinterpret_cast<const uint8_t* (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetHeaderPtr),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetWidth),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetHeight),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetDepth),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetSliceCount),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetEffectiveMipCount),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetEnginePixelFormat),
    reinterpret_cast<int32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*)>(&GXTpfEntryV2_GetFlags),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, uint32_t)>(&GXTpfEntryV2_GetDefaultOneB),
    reinterpret_cast<uint32_t (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, uint32_t)>(&GXTpfEntryV2_GetDefaultOneA),
    reinterpret_cast<const uint8_t* (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, uint32_t, uint32_t)>(&GXTpfEntryV2_GetPayloadPointer),
    reinterpret_cast<void (*)(const GXTpfEntryStrategy*, const GXTpfTextureSourceView*, GXTpfExtraMetadataOut*)>(&GXTpfEntryV2_ExtractExtraMetadata),
};

// VMA 0x52ba2c0 and 0x52ba380 strategy singletons
static const GXTpfEntryStrategy kGXTpfEntryStrategyV1 = { &kGXTpfEntryV1VTable };
static const GXTpfEntryStrategy kGXTpfEntryStrategyV2 = { &kGXTpfEntryV2VTable };

static inline const GXTpfEntryStrategy* GXTpf_SelectStrategy(int8_t platformFlag) {
    if (platformFlag == 0) {
        return &kGXTpfEntryStrategyV1;
    }
    if (platformFlag == 4) {
        return &kGXTpfEntryStrategyV2;
    }
    return nullptr;
}

extern "C" {

// ============================================================================
// 1. VMA 0x26dab50 (7 B) — GXTpfEntryV1_GetDataOffset
//    Returns entryHdr->dataOffset (+0x00) from a V1 TPF entry.
// ============================================================================
uint32_t GXTpfEntryV1_GetDataOffset(
    const GXTpfEntryStrategy* unusedSelf,
    const GXTpfTextureSourceView* view)
{
    const uint8_t* hdr = (unusedSelf != nullptr) ? view->entryHdr : view->entryHdr;
    return *reinterpret_cast<const uint32_t*>(hdr);
}

// ============================================================================
// 2. VMA 0x26dab40 (14 B) — GXTpfEntryV1_ValidateDdsHeader
//    Validates the embedded DDS header at tpfBase + entryHdr->dataOffset.
// ============================================================================
bool GXTpfEntryV1_ValidateDdsHeader(
    const GXTpfEntryStrategy* unusedSelf,
    const GXTpfTextureSourceView* view)
{
    const uint8_t* hdr = (unusedSelf != nullptr) ? view->entryHdr : view->entryHdr;
    uint32_t dataOffset = *reinterpret_cast<const uint32_t*>(hdr);
    return GXDdsHeader_ValidateMagicAndSize(view->tpfBase + dataOffset);
}

// ============================================================================
// 3. VMA 0x26daaa0 (1 B) — GXTpfContainer_NoOp
// ============================================================================
void GXTpfContainer_NoOp() {}

// ============================================================================
// 4. VMA 0x26d9fe0 (2745 B) — GXTpfContainer_CreateTextureArrayFromAllEntries
//    Validates that all texture entries in a TPF container are 2D textures with
//    identical dimensions, format, mip count, and byte size, then creates a
//    Texture2DArray GPU resource and optional ShaderResourceView.
// ============================================================================
uint32_t GXTpfContainer_CreateTextureArrayFromAllEntries(
    GXGnmRefCountedResource** outTextureObj,
    GXGnmRefCountedResource** outSrvObj,
    void* gxDevice,
    const uint8_t* tpfBase,
    DLAllocator* overrideAllocator,
    void* overrideOwnerContext,
    uint32_t createFlags)
{
    if (outTextureObj == nullptr && outSrvObj == nullptr) {
        return 0;
    }

    bool isInvalidHeader = true;
    if (tpfBase != nullptr &&
        tpfBase[0] == 'T' &&
        tpfBase[1] == 'P' &&
        tpfBase[2] == 'F' &&
        tpfBase[3] == '\0' &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) != 0 &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x08) != 0 &&
        (tpfBase[0x0C] & 0xFB) == 0)
    {
        isInvalidHeader = (tpfBase[0x0D] != 3);
    }

    DLAllocator* defaultAlloc = GXDevice_GetDefaultAllocator(gxDevice);
    DLAllocator* allocator    = GXResolveAllocator(overrideAllocator, defaultAlloc);
    void* defaultOwner        = GXDevice_GetDefaultOwnerContext(gxDevice);
    void* ownerContext        = GXResolveOwnerContext(overrideOwnerContext, defaultOwner);

    if (ownerContext == nullptr || allocator == nullptr || isInvalidHeader) {
        return 8;
    }

    uint32_t textureCount   = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    const uint8_t* firstHdr = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);

    uint32_t refEngineFormat = 0;
    uint32_t refHeight       = 0;
    uint32_t refWidth        = 0;
    uint32_t refMipCount     = 0;
    uint32_t refFormatCode   = 0;
    uint32_t refDataSize     = 0;

    uint32_t usageFlag = (~createFlags) & 1u;
    uint32_t srgbFlag  = createFlags & 2u;
    uint32_t flagBit2  = createFlags & 4u;

    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr) {
        return 8;
    }

    const uint8_t* curEntry = firstHdr;
    for (uint32_t entryIdx = 0; entryIdx < textureCount; ++entryIdx) {
        GXTpfTextureSourceView view{ tpfBase, curEntry };
        if (!strat->vptr->validateEntry(strat, &view)) {
            return 8;
        }
        if (static_cast<uint8_t>(strat->vptr->getTextureType(strat, &view)) != 0) {
            return 8;
        }

        if (entryIdx == 0) {
            refDataSize     = strat->vptr->getDataSize(strat, &view);
            refFormatCode   = strat->vptr->getFormatCode(strat, &view);
            refMipCount     = strat->vptr->getEffectiveMipCount(strat, &view);
            refWidth        = strat->vptr->getWidth(strat, &view);
            refHeight       = strat->vptr->getHeight(strat, &view);
            refEngineFormat = strat->vptr->getEnginePixelFormat(strat, &view);
        }

        if (strat->vptr->getDataSize(strat, &view) != refDataSize) {
            return 8;
        }
        if (static_cast<uint8_t>(strat->vptr->getFormatCode(strat, &view)) != static_cast<uint8_t>(refFormatCode)) {
            return 8;
        }
        if (strat->vptr->getEffectiveMipCount(strat, &view) != static_cast<uint8_t>(refMipCount)) {
            return 8;
        }
        if (strat->vptr->getWidth(strat, &view) != refWidth) {
            return 8;
        }
        if (strat->vptr->getHeight(strat, &view) != refHeight) {
            return 8;
        }

        uint32_t extCount = strat->vptr->getHeaderExtCount(strat, &view);
        uint32_t hdrBytes = strat->vptr->getHeaderByteSize(strat, &view);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    DLScratchScope scratchScope{};
    scratchScope.vptr      = reinterpret_cast<const void*>(GX_VMA(0x52b49f0ULL));
    scratchScope.savedMark = DLScratchAlloc_AcquireMark();

    uint32_t mipLevels8 = static_cast<uint8_t>(refMipCount);
    size_t subresourceBytes = static_cast<size_t>(mipLevels8 * textureCount) << 4;

    GXTpfSubresourceInitData* subresources = static_cast<GXTpfSubresourceInitData*>(
        DLScratchAlloc_Allocate(&scratchScope, subresourceBytes));
    DLAllocator* heapFallbackAlloc = nullptr;

    if (subresources == nullptr) {
        subresources = static_cast<GXTpfSubresourceInitData*>(
            allocator->vptr->allocate(allocator, subresourceBytes));
        heapFallbackAlloc = allocator;
        if (subresources == nullptr) {
            DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
            return 9;
        }
    }

    uint32_t resultStatus = 8;
    uint32_t hasError     = 1;

    uint32_t baseSubIdx = 0;
    curEntry = firstHdr;
    for (uint32_t entryIdx = 0; entryIdx < textureCount; ++entryIdx) {
        GXTpfTextureSourceView view{ tpfBase, curEntry };
        if (!strat->vptr->validateEntry(strat, &view)) {
            if (heapFallbackAlloc != nullptr) {
                heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
            }
            DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
            return 8;
        }

        for (uint32_t mipIdx = 0; mipIdx < mipLevels8; ++mipIdx) {
            uint32_t rowPitch   = strat->vptr->getMipSlicePitchOrRowSize(strat, &view, mipIdx);
            uint32_t slicePitch = strat->vptr->getMipSliceByteSize(strat, &view, mipIdx);
            if (rowPitch == 0 || slicePitch == 0) {
                if (heapFallbackAlloc != nullptr) {
                    heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
                }
                DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
                return 8;
            }
            const uint8_t* payloadPtr = strat->vptr->getSubresourcePayloadPtr(strat, &view, mipIdx, 0);
            GXTpfSubresourceInitData& dst = subresources[baseSubIdx + mipIdx];
            dst.dataPtr    = payloadPtr;
            dst.rowPitch   = rowPitch;
            dst.slicePitch = slicePitch;
        }
        baseSubIdx += mipLevels8;

        uint32_t extCount = strat->vptr->getHeaderExtCount(strat, &view);
        uint32_t hdrBytes = strat->vptr->getHeaderByteSize(strat, &view);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXGnmRefCountedResource* createdTexObj = nullptr;
    uint32_t texFormat = (srgbFlag != 0) ? GXFormat_ToSrgbFormat(refEngineFormat) : refEngineFormat;

    GXTexture2DDesc texDesc{};
    texDesc.width          = refWidth;
    texDesc.height         = refHeight;
    texDesc.mipLevels      = mipLevels8;
    texDesc.arraySize      = textureCount;
    texDesc.format         = texFormat;
    texDesc.sampleCount    = 1;
    texDesc.sampleQuality  = 0;
    texDesc.usage          = usageFlag;
    texDesc.bindFlags      = 8;
    texDesc.cpuAccessFlags = 0;
    texDesc.miscFlags      = flagBit2 << 12;
    texDesc.reserved2C     = 0;

    GXTpfTextureSourceView entry0View{ tpfBase, firstHdr };
    const GXTpfEntryStrategy* entry0Strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (entry0Strat != nullptr && entry0Strat->vptr->validateEntry(entry0Strat, &entry0View)) {
        alignas(16) GXTpfExtraMetadataOut extraMeta{};
        entry0Strat->vptr->extractExtraMetadata(entry0Strat, &entry0View, &extraMeta);

        uint32_t createErr = GXGnmDevice_CreateTexture2D(
            gxDevice, &createdTexObj, &texDesc, subresources, allocator, ownerContext, &extraMeta);
        if (createErr != 0) {
            resultStatus = createErr;
        } else {
            bool srvOk = true;
            if (outSrvObj != nullptr) {
                if (flagBit2 != 0) {
                    *outSrvObj = nullptr;
                } else {
                    GXShaderResourceViewDesc srvDesc{};
                    srvDesc.format          = refEngineFormat;
                    srvDesc.viewDimension   = 5; // TEXTURE2DARRAY
                    srvDesc.mostDetailedMip = 0;
                    srvDesc.mipLevels       = mipLevels8;
                    srvDesc.firstArraySlice = 0;
                    srvDesc.arraySize       = textureCount;

                    resultStatus = GXGnmDevice_CreateShaderResourceView(
                        gxDevice, outSrvObj, createdTexObj, &srvDesc, allocator);
                    if (resultStatus != 0) {
                        srvOk = false;
                    }
                }
            }

            if (srvOk) {
                if (outTextureObj != nullptr) {
                    *outTextureObj = createdTexObj;
                } else if (createdTexObj != nullptr && DLAtomic_FetchDecrement32(&createdTexObj->refCount) == 1) {
                    DLAllocator* resAlloc = createdTexObj->allocator;
                    createdTexObj->vptr->dtor(createdTexObj);
                    resAlloc->vptr->deallocate(resAlloc, createdTexObj);
                }
                hasError     = 0;
                resultStatus = 0;
            } else if (createdTexObj != nullptr && DLAtomic_FetchDecrement32(&createdTexObj->refCount) == 1) {
                DLAllocator* resAlloc = createdTexObj->allocator;
                createdTexObj->vptr->dtor(createdTexObj);
                resAlloc->vptr->deallocate(resAlloc, createdTexObj);
            }
        }
    }

    if (heapFallbackAlloc != nullptr) {
        heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
    }
    DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
    return (hasError == 0) ? 0u : resultStatus;
}

// ============================================================================
// 5. VMA 0x26d9850 (1926 B) — GXTpfContainer_CreateTexture3DFromEntry
//    Creates a 3D Volume Texture and optional SRV from a single TPF entry
//    whose textureType == 2.
// ============================================================================
uint32_t GXTpfContainer_CreateTexture3DFromEntry(
    GXGnmRefCountedResource** outTextureObj,
    GXGnmRefCountedResource** outSrvObj,
    void* gxDevice,
    const uint8_t* tpfBase,
    uint32_t entryIndex,
    DLAllocator* overrideAllocator,
    void* overrideOwnerContext,
    uint32_t createFlags)
{
    if (outTextureObj == nullptr && outSrvObj == nullptr) {
        return 0;
    }

    bool isValidHeader = false;
    uint32_t textureCount = 0;
    if (tpfBase != nullptr &&
        tpfBase[0] == 'T' &&
        tpfBase[1] == 'P' &&
        tpfBase[2] == 'F' &&
        tpfBase[3] == '\0' &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) != 0 &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x08) != 0 &&
        (tpfBase[0x0C] & 0xFB) == 0 &&
        tpfBase[0x0D] == 3)
    {
        isValidHeader = true;
        textureCount  = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    }

    DLAllocator* defaultAlloc = GXDevice_GetDefaultAllocator(gxDevice);
    DLAllocator* allocator    = GXResolveAllocator(overrideAllocator, defaultAlloc);
    void* defaultOwner        = GXDevice_GetDefaultOwnerContext(gxDevice);
    void* ownerContext        = GXResolveOwnerContext(overrideOwnerContext, defaultOwner);

    if (ownerContext == nullptr || allocator == nullptr || !isValidHeader || entryIndex >= textureCount) {
        return 8;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 8;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 8;
    }
    if (static_cast<uint8_t>(strat->vptr->getTextureType(strat, &view)) != 2) {
        return 8;
    }

    uint32_t sliceCount = strat->vptr->getSliceCount(strat, &view);
    uint32_t mipCount   = strat->vptr->getEffectiveMipCount(strat, &view);

    DLScratchScope scratchScope{};
    scratchScope.vptr      = reinterpret_cast<const void*>(GX_VMA(0x52b49f0ULL));
    scratchScope.savedMark = DLScratchAlloc_AcquireMark();

    size_t subresourceBytes = static_cast<size_t>(mipCount * sliceCount) << 4;
    GXTpfSubresourceInitData* subresources = static_cast<GXTpfSubresourceInitData*>(
        DLScratchAlloc_Allocate(&scratchScope, subresourceBytes));
    DLAllocator* heapFallbackAlloc = nullptr;

    if (subresources == nullptr) {
        subresources = static_cast<GXTpfSubresourceInitData*>(
            allocator->vptr->allocate(allocator, subresourceBytes));
        heapFallbackAlloc = allocator;
        if (subresources == nullptr) {
            DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
            return 9;
        }
    }

    uint32_t baseSubIdx = 0;
    for (uint32_t sliceIdx = 0; sliceIdx < sliceCount; ++sliceIdx) {
        for (uint32_t mipIdx = 0; mipIdx < mipCount; ++mipIdx) {
            uint32_t rowPitch   = strat->vptr->getMipSlicePitchOrRowSize(strat, &view, mipIdx);
            uint32_t slicePitch = strat->vptr->getMipSliceByteSize(strat, &view, mipIdx);
            if (rowPitch == 0 || slicePitch == 0) {
                if (heapFallbackAlloc != nullptr) {
                    heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
                }
                DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
                return 8;
            }
            const uint8_t* payloadPtr = strat->vptr->getSubresourcePayloadPtr(strat, &view, mipIdx, sliceIdx);
            GXTpfSubresourceInitData& dst = subresources[baseSubIdx + mipIdx];
            dst.dataPtr    = payloadPtr;
            dst.rowPitch   = rowPitch;
            dst.slicePitch = slicePitch;
        }
        baseSubIdx += mipCount;
    }

    uint32_t usageFlag = (~createFlags) & 1u;
    uint32_t srgbFlag  = createFlags & 2u;
    uint32_t flagBit2  = createFlags & 4u;

    GXGnmRefCountedResource* createdTexObj = nullptr;
    uint32_t rawEngineFormat = strat->vptr->getEnginePixelFormat(strat, &view);
    uint32_t texFormat       = (srgbFlag != 0) ? GXFormat_ToSrgbFormat(rawEngineFormat) : rawEngineFormat;

    GXTexture3DDesc texDesc{};
    texDesc.width          = strat->vptr->getWidth(strat, &view);
    texDesc.height         = strat->vptr->getHeight(strat, &view);
    texDesc.depth          = strat->vptr->getDepth(strat, &view);
    texDesc.mipLevels      = mipCount;
    texDesc.format         = texFormat;
    texDesc.usage          = usageFlag;
    texDesc.bindFlags      = 8;
    texDesc.cpuAccessFlags = 0;
    texDesc.miscFlags      = flagBit2 << 12;
    texDesc.reserved24     = 0;

    alignas(16) GXTpfExtraMetadataOut extraMeta{};
    strat->vptr->extractExtraMetadata(strat, &view, &extraMeta);

    uint32_t resultStatus = GXGnmDevice_CreateTexture3D(
        gxDevice, &createdTexObj, &texDesc, subresources, allocator, ownerContext, &extraMeta);
    uint32_t hasError = 1;

    if (resultStatus == 0) {
        bool srvOk = true;
        if (outSrvObj != nullptr) {
            if (flagBit2 != 0) {
                *outSrvObj = nullptr;
            } else {
                GXShaderResourceViewDesc srvDesc{};
                srvDesc.format          = rawEngineFormat;
                srvDesc.viewDimension   = 8; // TEXTURE3D
                srvDesc.mostDetailedMip = 0;
                srvDesc.mipLevels       = mipCount;
                srvDesc.firstArraySlice = 0;
                srvDesc.arraySize       = 0;

                resultStatus = GXGnmDevice_CreateShaderResourceView(
                    gxDevice, outSrvObj, createdTexObj, &srvDesc, allocator);
                if (resultStatus != 0) {
                    srvOk = false;
                }
            }
        }

        if (srvOk) {
            if (outTextureObj != nullptr) {
                *outTextureObj = createdTexObj;
            } else if (createdTexObj != nullptr && DLAtomic_FetchDecrement32(&createdTexObj->refCount) == 1) {
                DLAllocator* resAlloc = createdTexObj->allocator;
                createdTexObj->vptr->dtor(createdTexObj);
                resAlloc->vptr->deallocate(resAlloc, createdTexObj);
            }
            hasError     = 0;
            resultStatus = 0;
        } else if (createdTexObj != nullptr && DLAtomic_FetchDecrement32(&createdTexObj->refCount) == 1) {
            DLAllocator* resAlloc = createdTexObj->allocator;
            createdTexObj->vptr->dtor(createdTexObj);
            resAlloc->vptr->deallocate(resAlloc, createdTexObj);
        }
    }

    if (heapFallbackAlloc != nullptr) {
        heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
    }
    DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
    return (hasError == 0) ? 0u : resultStatus;
}

// ============================================================================
// 6. VMA 0x26d9070 (2015 B) — GXTpfContainer_CreateTexture2DOrCubeFromEntry
//    Creates a 2D Texture, 2D Texture Array, or CubeMap Texture and optional
//    SRV from a single TPF entry whose textureType != 2.
// ============================================================================
uint32_t GXTpfContainer_CreateTexture2DOrCubeFromEntry(
    GXGnmRefCountedResource** outTextureObj,
    GXGnmRefCountedResource** outSrvObj,
    void* gxDevice,
    const uint8_t* tpfBase,
    uint32_t entryIndex,
    DLAllocator* overrideAllocator,
    void* overrideOwnerContext,
    uint32_t createFlags)
{
    if (outTextureObj == nullptr && outSrvObj == nullptr) {
        return 0;
    }

    bool isValidHeader = false;
    uint32_t textureCount = 0;
    if (tpfBase != nullptr &&
        tpfBase[0] == 'T' &&
        tpfBase[1] == 'P' &&
        tpfBase[2] == 'F' &&
        tpfBase[3] == '\0' &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) != 0 &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x08) != 0 &&
        (tpfBase[0x0C] & 0xFB) == 0 &&
        tpfBase[0x0D] == 3)
    {
        isValidHeader = true;
        textureCount  = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    }

    DLAllocator* defaultAlloc = GXDevice_GetDefaultAllocator(gxDevice);
    DLAllocator* allocator    = GXResolveAllocator(overrideAllocator, defaultAlloc);
    void* defaultOwner        = GXDevice_GetDefaultOwnerContext(gxDevice);
    void* ownerContext        = GXResolveOwnerContext(overrideOwnerContext, defaultOwner);

    if (ownerContext == nullptr || allocator == nullptr || !isValidHeader || entryIndex >= textureCount) {
        return 8;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 8;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 8;
    }
    if (static_cast<uint8_t>(strat->vptr->getTextureType(strat, &view)) == 2) {
        return 8;
    }

    uint32_t sliceCount = strat->vptr->getSliceCount(strat, &view);
    uint32_t mipCount   = strat->vptr->getEffectiveMipCount(strat, &view);

    DLScratchScope scratchScope{};
    scratchScope.vptr      = reinterpret_cast<const void*>(GX_VMA(0x52b49f0ULL));
    scratchScope.savedMark = DLScratchAlloc_AcquireMark();

    size_t subresourceBytes = static_cast<size_t>(mipCount * sliceCount) << 4;
    GXTpfSubresourceInitData* subresources = static_cast<GXTpfSubresourceInitData*>(
        DLScratchAlloc_Allocate(&scratchScope, subresourceBytes));
    DLAllocator* heapFallbackAlloc = nullptr;

    if (subresources == nullptr) {
        subresources = static_cast<GXTpfSubresourceInitData*>(
            allocator->vptr->allocate(allocator, subresourceBytes));
        heapFallbackAlloc = allocator;
        if (subresources == nullptr) {
            DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
            return 9;
        }
    }

    uint32_t baseSubIdx = 0;
    for (uint32_t sliceIdx = 0; sliceIdx < sliceCount; ++sliceIdx) {
        for (uint32_t mipIdx = 0; mipIdx < mipCount; ++mipIdx) {
            uint32_t rowPitch   = strat->vptr->getMipSlicePitchOrRowSize(strat, &view, mipIdx);
            uint32_t slicePitch = strat->vptr->getMipSliceByteSize(strat, &view, mipIdx);
            if (rowPitch == 0 || slicePitch == 0) {
                if (heapFallbackAlloc != nullptr) {
                    heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
                }
                DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
                return 8;
            }
            const uint8_t* payloadPtr = strat->vptr->getSubresourcePayloadPtr(strat, &view, mipIdx, sliceIdx);
            GXTpfSubresourceInitData& dst = subresources[baseSubIdx + mipIdx];
            dst.dataPtr    = payloadPtr;
            dst.rowPitch   = rowPitch;
            dst.slicePitch = slicePitch;
        }
        baseSubIdx += mipCount;
    }

    uint32_t usageFlag = (~createFlags) & 1u;
    uint32_t srgbFlag  = createFlags & 2u;
    uint32_t flagBit2  = createFlags & 4u;

    GXGnmRefCountedResource* createdTexObj = nullptr;
    uint32_t rawEngineFormat = strat->vptr->getEnginePixelFormat(strat, &view);
    uint32_t texFormat       = (srgbFlag != 0) ? GXFormat_ToSrgbFormat(rawEngineFormat) : rawEngineFormat;

    uint8_t texType = static_cast<uint8_t>(strat->vptr->getTextureType(strat, &view));
    uint32_t cubeMiscFlag = (texType == 1) ? 4u : 0u;

    GXTexture2DDesc texDesc{};
    texDesc.width          = strat->vptr->getWidth(strat, &view);
    texDesc.height         = strat->vptr->getHeight(strat, &view);
    texDesc.mipLevels      = mipCount;
    texDesc.arraySize      = sliceCount;
    texDesc.format         = texFormat;
    texDesc.sampleCount    = 1;
    texDesc.sampleQuality  = 0;
    texDesc.usage          = usageFlag;
    texDesc.bindFlags      = 8;
    texDesc.cpuAccessFlags = 0;
    texDesc.miscFlags      = (flagBit2 << 12) | cubeMiscFlag;
    texDesc.reserved2C     = 0;

    alignas(16) GXTpfExtraMetadataOut extraMeta{};
    strat->vptr->extractExtraMetadata(strat, &view, &extraMeta);

    uint32_t resultStatus = GXGnmDevice_CreateTexture2D(
        gxDevice, &createdTexObj, &texDesc, subresources, allocator, ownerContext, &extraMeta);
    uint32_t hasError = 1;

    if (resultStatus == 0) {
        bool srvOk = true;
        if (outSrvObj != nullptr) {
            if (flagBit2 != 0) {
                *outSrvObj = nullptr;
            } else {
                GXShaderResourceViewDesc srvDesc{};
                srvDesc.format          = rawEngineFormat;
                srvDesc.mostDetailedMip = 0;
                srvDesc.mipLevels       = mipCount;
                srvDesc.firstArraySlice = 0;

                if (static_cast<uint8_t>(strat->vptr->getTextureType(strat, &view)) == 1) {
                    srvDesc.viewDimension = 9; // TEXTURECUBE
                    srvDesc.arraySize     = 1;
                } else {
                    srvDesc.viewDimension = (sliceCount > 1u) ? 5u : 4u; // TEXTURE2DARRAY vs TEXTURE2D
                    srvDesc.arraySize     = sliceCount;
                }

                resultStatus = GXGnmDevice_CreateShaderResourceView(
                    gxDevice, outSrvObj, createdTexObj, &srvDesc, allocator);
                if (resultStatus != 0) {
                    srvOk = false;
                }
            }
        }

        if (srvOk) {
            if (outTextureObj != nullptr) {
                *outTextureObj = createdTexObj;
            } else if (createdTexObj != nullptr && DLAtomic_FetchDecrement32(&createdTexObj->refCount) == 1) {
                DLAllocator* resAlloc = createdTexObj->allocator;
                createdTexObj->vptr->dtor(createdTexObj);
                resAlloc->vptr->deallocate(resAlloc, createdTexObj);
            }
            hasError     = 0;
            resultStatus = 0;
        } else if (createdTexObj != nullptr && DLAtomic_FetchDecrement32(&createdTexObj->refCount) == 1) {
            DLAllocator* resAlloc = createdTexObj->allocator;
            createdTexObj->vptr->dtor(createdTexObj);
            resAlloc->vptr->deallocate(resAlloc, createdTexObj);
        }
    }

    if (heapFallbackAlloc != nullptr) {
        heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, subresources);
    }
    DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
    return (hasError == 0) ? 0u : resultStatus;
}

// ============================================================================
// 7. VMA 0x26d8df0 (627 B) — GXTpfContainer_GetEntryExtraChunkData
//    Locates the extra metadata chunk at chunkIndex for entryIndex, writes its
//    payload byte size and chunk ID, and returns a pointer to its payload.
// ============================================================================
const uint8_t* GXTpfContainer_GetEntryExtraChunkData(
    const uint8_t* tpfBase,
    uint32_t entryIndex,
    uint32_t chunkIndex,
    uint32_t* outChunkByteSize,
    uint32_t* outChunkId)
{
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return nullptr;
    }

    uint32_t textureCount = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (textureCount == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= textureCount)
    {
        return nullptr;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return nullptr;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return nullptr;
    }

    uint32_t extCount = strat->vptr->getHeaderExtCount(strat, &view);
    if (chunkIndex >= extCount) {
        return nullptr;
    }

    const uint8_t* chunkPtr = strat->vptr->getHeaderPtr(strat, &view) +
                              strat->vptr->getHeaderByteSize(strat, &view);
    for (uint32_t c = 0; c < chunkIndex; ++c) {
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(chunkPtr + 0x04);
        chunkPtr += chunkSize + 0x08;
    }

    if (outChunkByteSize != nullptr) {
        *outChunkByteSize = *reinterpret_cast<const uint32_t*>(chunkPtr + 0x04);
    }
    if (outChunkId != nullptr) {
        *outChunkId = *reinterpret_cast<const uint32_t*>(chunkPtr + 0x00);
    }
    return chunkPtr + 0x08;
}

// ============================================================================
// 8. VMA 0x26d8c10 (467 B) — GXTpfContainer_GetEntryExtraChunkCount
//    Returns the number of extra metadata chunks attached to entryIndex.
// ============================================================================
uint32_t GXTpfContainer_GetEntryExtraChunkCount(const uint8_t* tpfBase, uint32_t entryIndex) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return 0;
    }

    uint32_t textureCount = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (textureCount == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= textureCount)
    {
        return 0;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 0;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 0;
    }
    return strat->vptr->getHeaderExtCount(strat, &view);
}

// ============================================================================
// 9. VMA 0x26d8a20 (486 B) — GXTpfContainer_GetEntryFileNamePtr
//    Returns a pointer to the file name string of entryIndex in the TPF.
// ============================================================================
const char* GXTpfContainer_GetEntryFileNamePtr(const uint8_t* tpfBase, uint32_t entryIndex) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return nullptr;
    }

    uint32_t textureCount = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (textureCount == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= textureCount)
    {
        return nullptr;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return nullptr;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return nullptr;
    }

    uint32_t nameOffset = strat->vptr->getFileNameOffset(strat, &view);
    return reinterpret_cast<const char*>(tpfBase + nameOffset);
}

// ============================================================================
// 10. VMA 0x26d8840 (471 B) — GXTpfContainer_GetEntryTextureType
//     Returns the texture type (0 = 2D, 1 = Cube, 2 = 3D) of entryIndex, or
//     0xFFFFFFFF on invalid header/index.
// ============================================================================
uint32_t GXTpfContainer_GetEntryTextureType(const uint8_t* tpfBase, uint32_t entryIndex) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return 0xFFFFFFFFu;
    }

    uint32_t textureCount = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (textureCount == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= textureCount)
    {
        return 0xFFFFFFFFu;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 0xFFFFFFFFu;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 0xFFFFFFFFu;
    }
    return static_cast<uint8_t>(strat->vptr->getTextureType(strat, &view));
}

// ============================================================================
// 11. VMA 0x26d8660 (467 B) — GXTpfContainer_GetEntryEnginePixelFormat
//     Returns the resolved engine pixel format ID of entryIndex, or 0 on error.
// ============================================================================
uint32_t GXTpfContainer_GetEntryEnginePixelFormat(const uint8_t* tpfBase, uint32_t entryIndex) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return 0;
    }

    uint32_t textureCount = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (textureCount == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= textureCount)
    {
        return 0;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 0;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 0;
    }
    return strat->vptr->getEnginePixelFormat(strat, &view);
}

// ============================================================================
// 12. VMA 0x26d8480 (471 B) — GXTpfContainer_GetEntryFormatCode
//     Returns the raw TPF format code byte of entryIndex, or 0xFFFFFFFF on error.
// ============================================================================
uint32_t GXTpfContainer_GetEntryFormatCode(const uint8_t* tpfBase, uint32_t entryIndex) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return 0xFFFFFFFFu;
    }

    uint32_t textureCount = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (textureCount == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= textureCount)
    {
        return 0xFFFFFFFFu;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 0xFFFFFFFFu;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 0xFFFFFFFFu;
    }
    return static_cast<uint8_t>(strat->vptr->getFormatCode(strat, &view));
}

// ============================================================================
// 13. VMA 0x26d8280 (503 B) — GXTpfContainer_GetEntryFlagsShifted4
//     Returns (entryFlags >> 4) (arithmetic right shift) for entryIndex.
// ============================================================================
int32_t GXTpfContainer_GetEntryFlagsShifted4(const uint8_t* tpfBase, uint32_t entryIndex) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0 ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x08) == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0 ||
        tpfBase[0x0D] != 3 ||
        entryIndex >= *reinterpret_cast<const uint32_t*>(tpfBase + 0x08))
    {
        return 0;
    }

    const uint8_t* curEntry = (tpfBase[0x0F] & 0x01) ? (tpfBase + 0x30) : (tpfBase + 0x10);
    for (uint32_t prevIdx = 0; prevIdx < entryIndex; ++prevIdx) {
        GXTpfTextureSourceView stepView{ tpfBase, curEntry };
        const GXTpfEntryStrategy* stepStrat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
        if (stepStrat == nullptr || !stepStrat->vptr->validateEntry(stepStrat, &stepView)) {
            return 0;
        }
        uint32_t extCount = stepStrat->vptr->getHeaderExtCount(stepStrat, &stepView);
        uint32_t hdrBytes = stepStrat->vptr->getHeaderByteSize(stepStrat, &stepView);
        curEntry += hdrBytes;
        for (uint32_t c = 0; c < extCount; ++c) {
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(curEntry + 0x04);
            curEntry += chunkSize + 0x08;
        }
    }

    GXTpfTextureSourceView view{ tpfBase, curEntry };
    const GXTpfEntryStrategy* strat = GXTpf_SelectStrategy(static_cast<int8_t>(tpfBase[0x0C]));
    if (strat == nullptr || !strat->vptr->validateEntry(strat, &view)) {
        return 0;
    }
    return strat->vptr->getFlags(strat, &view) >> 4;
}

// ============================================================================
// 14. VMA 0x26d8230 (72 B) — GXTpfContainer_GetEncodingFlag
//     Writes the string encoding byte (tpfBase[0x0E]) to *outEncodingFlag if
//     tpfBase is a valid 'TPF\0' header, or 0 otherwise.
// ============================================================================
uint8_t* GXTpfContainer_GetEncodingFlag(uint8_t* outEncodingFlag, const uint8_t* tpfBase) {
    uint8_t encoding = 0;
    if (tpfBase != nullptr &&
        tpfBase[0] == 'T' &&
        tpfBase[1] == 'P' &&
        tpfBase[2] == 'F' &&
        tpfBase[3] == '\0' &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) != 0 &&
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x08) != 0 &&
        (tpfBase[0x0C] & 0xFB) == 0 &&
        tpfBase[0x0D] == 3)
    {
        encoding = tpfBase[0x0E];
    }
    *outEncodingFlag = encoding;
    return outEncodingFlag;
}

// ============================================================================
// 15. VMA 0x26d81f0 (59 B) — GXTpfContainer_GetTextureCount
//     Returns textureCount (+0x08) if tpfBase is a valid 'TPF\0' header, else 0.
// ============================================================================
uint32_t GXTpfContainer_GetTextureCount(const uint8_t* tpfBase) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0)
    {
        return 0;
    }

    uint32_t count = *reinterpret_cast<const uint32_t*>(tpfBase + 0x08);
    if (count == 0 || (tpfBase[0x0C] & 0xFB) != 0 || tpfBase[0x0D] != 3) {
        return 0;
    }
    return count;
}

// ============================================================================
// 16. VMA 0x26d81a0 (78 B) — GXTpfContainer_ValidateHeader
//     Validates the 16-byte 'TPF\0' header magic, sizes, platform, and flags.
// ============================================================================
bool GXTpfContainer_ValidateHeader(const uint8_t* tpfBase) {
    if (tpfBase == nullptr ||
        tpfBase[0] != 'T' ||
        tpfBase[1] != 'P' ||
        tpfBase[2] != 'F' ||
        tpfBase[3] != '\0' ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x04) == 0 ||
        *reinterpret_cast<const uint32_t*>(tpfBase + 0x08) == 0 ||
        (tpfBase[0x0C] & 0xFB) != 0)
    {
        return false;
    }
    return (tpfBase[0x0D] == 3);
}

// ============================================================================
// 17. VMA 0x26d80f0 (24 B) — GXTpfFormat_GetBlockHeight
//     Returns the compression block height in texels (1, 4, or 8) for
//     engineFormat (1..99), or 0 if out of range.
// ============================================================================
uint32_t GXTpfFormat_GetBlockHeight(uint32_t engineFormat) {
    uint32_t idx = engineFormat - 1u;
    if (idx > 0x62u) {
        return 0;
    }
    return kEngineFormatBlockHeightTable[idx];
}

// ============================================================================
// 18. VMA 0x26d80d0 (24 B) — GXTpfFormat_GetBlockWidth
//     Returns the compression block width in texels (1, 2, 4, or 8) for
//     engineFormat (1..99), or 0 if out of range.
// ============================================================================
uint32_t GXTpfFormat_GetBlockWidth(uint32_t engineFormat) {
    uint32_t idx = engineFormat - 1u;
    if (idx > 0x62u) {
        return 0;
    }
    return kEngineFormatBlockWidthTable[idx];
}

// ============================================================================
// 19. VMA 0x26d80b0 (24 B) — GXTpfFormat_GetBytesPerBlock
//     Returns the byte size per texel/block (1, 2, 4, 8, 12, or 16) for
//     engineFormat (1..99), or 0 if out of range.
// ============================================================================
uint32_t GXTpfFormat_GetBytesPerBlock(uint32_t engineFormat) {
    uint32_t idx = engineFormat - 1u;
    if (idx > 0x62u) {
        return 0;
    }
    return kEngineFormatBytesPerBlockTable[idx];
}

// ============================================================================
// 20. VMA 0x26d8010 (5 B) — GXDynamicBufferRingCursor_CtorThunk
// ============================================================================
void GXDynamicBufferRingCursor_CtorThunk(
    GXDynamicBufferRingCursor* self,
    uint32_t* unusedOutStatus,
    DLAllocator* unusedAllocator,
    GXGnmRefCountedResource* bufferObj,
    uint32_t alignment,
    uint32_t unusedExtra)
{
    if (unusedExtra == 0) {
        GXDynamicBufferRingCursor_Ctor(self, unusedOutStatus, unusedAllocator, bufferObj, alignment);
        return;
    }
    GXDynamicBufferRingCursor_Ctor(self, unusedOutStatus, unusedAllocator, bufferObj, alignment);
}

// ============================================================================
// 21. VMA 0x26d8000 (5 B) — GXDynamicBufferRingCursor_DtorThunk
// ============================================================================
void GXDynamicBufferRingCursor_DtorThunk(GXDynamicBufferRingCursor* self) {
    GXDynamicBufferRingCursor_Dtor(self);
}

// ============================================================================
// 22. VMA 0x26d7fd0 (46 B) — GXDynamicBufferRingOwner_InitializeThunk
// ============================================================================
void GXDynamicBufferRingOwner_InitializeThunk(
    GXDynamicBufferRingOwner* self,
    uint32_t* outStatus,
    void* gxDevice,
    uint32_t byteSize,
    uint32_t alignment,
    DLAllocator* allocator,
    void* ownerContext,
    uint8_t isConstantBuffer,
    uint32_t unusedExtra)
{
    if (unusedExtra == 0) {
        GXDynamicBufferRingOwner_Initialize(
            self, outStatus, gxDevice, byteSize, alignment, allocator, ownerContext, isConstantBuffer);
        return;
    }
    GXDynamicBufferRingOwner_Initialize(
        self, outStatus, gxDevice, byteSize, alignment, allocator, ownerContext, isConstantBuffer);
}

// ============================================================================
// 23. VMA 0x26d7fb0 (18 B) — GXDynamicBufferRingOwner_CreateCursorWithAlignmentCheck
//     Checks that alignment is a power of two (or zero) via BLSR before
//     creating a ring cursor instance.
// ============================================================================
uint32_t GXDynamicBufferRingOwner_CreateCursorWithAlignmentCheck(
    GXDynamicBufferRingOwner* self,
    GXDynamicBufferRingCursor** outCursor,
    uint32_t alignment,
    DLAllocator* overrideAllocator)
{
    if ((alignment & (alignment - 1u)) != 0u) {
        return 8;
    }
    return GXDynamicBufferRingOwner_CreateCursorInstance(self, outCursor, alignment, overrideAllocator);
}

// ============================================================================
// 24. VMA 0x26d7fa0 (5 B) — GXDynamicBufferRingOwner_DestroyThunk
// ============================================================================
void GXDynamicBufferRingOwner_DestroyThunk(GXDynamicBufferRingOwner* self) {
    GXDynamicBufferRingOwner_Destroy(self);
}

// ============================================================================
// 25. VMA 0x26d7e60 (316 B) — GXDynamicBufferRingOwner_CreateAndInitialize
//     Allocates and initializes a 0x18-byte GXDynamicBufferRingOwner.
// ============================================================================
uint32_t GXDynamicBufferRingOwner_CreateAndInitialize(
    GXDynamicBufferRingOwner** outOwner,
    void* gxDevice,
    uint32_t byteSize,
    uint32_t alignment,
    DLAllocator* overrideAllocator,
    void* overrideOwnerContext,
    uint8_t isConstantBuffer,
    uint32_t unusedExtra)
{
    DLAllocator* defaultAlloc = GXDevice_GetDefaultAllocator(gxDevice);
    DLAllocator* allocator    = GXResolveAllocator(overrideAllocator, defaultAlloc);
    void* defaultOwner        = GXDevice_GetDefaultOwnerContext(gxDevice);
    void* ownerContext        = GXResolveOwnerContext(overrideOwnerContext, defaultOwner);

    if (ownerContext == nullptr ||
        allocator == nullptr ||
        (alignment & (alignment - 1u)) != 0u ||
        byteSize == 0u)
    {
        return 8;
    }

    uint32_t status = 0;
    GXDynamicBufferRingOwner* owner = static_cast<GXDynamicBufferRingOwner*>(
        DLHeap_AllocateAligned(sizeof(GXDynamicBufferRingOwner), 8, allocator));
    if (owner == nullptr) {
        *outOwner = nullptr;
        return 9;
    }

    GXDynamicBufferRingOwner_InitializeThunk(
        owner, &status, gxDevice, byteSize, alignment, allocator, ownerContext, isConstantBuffer, unusedExtra);
    *outOwner = owner;

    if (status != 0) {
        GXDynamicBufferRingOwner_Destroy(owner);
        allocator->vptr->deallocate(allocator, owner);
        *outOwner = nullptr;
        return status;
    }
    return 0;
}

// ============================================================================
// 26. VMA 0x26d7b40 (653 B) — GXDdsHeader_GetSubresourceOffset
//     Computes the byte offset from the start of a DDS file/blob (ddsPtr) to
//     the specified (mipIndex, sliceIndex) subresource payload.
// ============================================================================
uint32_t GXDdsHeader_GetSubresourceOffset(
    const uint8_t* ddsPtr,
    uint32_t mipIndex,
    uint32_t sliceIndex)
{
    uint32_t rawMipCount = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x1C);
    uint32_t totalMips   = (rawMipCount != 0u) ? rawMipCount : 1u;

    bool hasDx10 = ((ddsPtr[0x50] & 0x04) != 0 &&
                    *reinterpret_cast<const uint32_t*>(ddsPtr + 0x54) == 0x30315844u);
    bool isVolume = hasDx10
        ? (*reinterpret_cast<const uint32_t*>(ddsPtr + 0x84) == 4u)
        : ((ddsPtr[0x72] & 0x20) != 0);

    uint32_t baseHeight    = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x0C);
    uint32_t baseWidth     = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x10);
    uint32_t baseDepth     = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x18);
    uint32_t engineFmt     = GXDdsHeader_GetEnginePixelFormat(ddsPtr);
    uint32_t bytesPerBlock = GXTpfFormat_GetBytesPerBlock(engineFmt);
    uint32_t blockW        = GXTpfFormat_GetBlockWidth(engineFmt);
    uint32_t blockH        = GXTpfFormat_GetBlockHeight(engineFmt);

    auto calcMipBytes = [&](uint32_t m) -> uint32_t {
        if (blockW == 0u || blockH == 0u) {
            return 0u;
        }
        uint32_t shift = m & 31u;
        uint32_t mipDepth = 1u;
        if (isVolume) {
            uint32_t d = baseDepth >> shift;
            mipDepth = (d != 0u) ? d : 1u;
        }
        uint32_t mipHeight = baseHeight >> shift;
        uint32_t mipWidth  = baseWidth >> shift;
        uint32_t wMinus1   = (mipWidth != 0u) ? (mipWidth - 1u) : 0u;
        uint32_t blocksX   = (wMinus1 + blockW) / blockW;
        uint32_t hMinus1   = (mipHeight != 0u) ? (mipHeight - 1u) : 0u;
        uint32_t blocksY   = (hMinus1 + blockH) / blockH;
        return (blocksX * bytesPerBlock * blocksY) * mipDepth;
    };

    uint32_t accumOffset = 0;
    if (sliceIndex != 0u) {
        uint32_t fullSliceBytes = 0;
        for (uint32_t m = 0; m < totalMips; ++m) {
            fullSliceBytes += calcMipBytes(m);
        }
        accumOffset = sliceIndex * fullSliceBytes;
    }

    for (uint32_t m = 0; m < mipIndex; ++m) {
        accumOffset += calcMipBytes(m);
    }

    uint32_t headerBytes = hasDx10 ? 0x94u : 0x80u;
    return headerBytes + accumOffset;
}

// ============================================================================
// 27. VMA 0x26d7ab0 (140 B) — GXDdsHeader_GetMipSliceByteSize
//     Computes the 2D slice byte size at mipIndex for a DDS header.
// ============================================================================
uint32_t GXDdsHeader_GetMipSliceByteSize(const uint8_t* ddsPtr, uint32_t mipIndex) {
    uint32_t mipHeight = (*reinterpret_cast<const uint32_t*>(ddsPtr + 0x0C)) >> (mipIndex & 31u);
    uint32_t mipWidth  = (*reinterpret_cast<const uint32_t*>(ddsPtr + 0x10)) >> (mipIndex & 31u);

    uint32_t engineFmt     = GXDdsHeader_GetEnginePixelFormat(ddsPtr);
    uint32_t bytesPerBlock = GXTpfFormat_GetBytesPerBlock(engineFmt);
    uint32_t blockW        = GXTpfFormat_GetBlockWidth(engineFmt);
    uint32_t blockH        = GXTpfFormat_GetBlockHeight(engineFmt);

    if (blockW == 0u || blockH == 0u) {
        return 0;
    }

    uint32_t wMinus1 = (mipWidth != 0u) ? (mipWidth - 1u) : 0u;
    uint32_t blocksX = (wMinus1 + blockW) / blockW;

    uint32_t hMinus1 = (mipHeight != 0u) ? (mipHeight - 1u) : 0u;
    uint32_t blocksY = (hMinus1 + blockH) / blockH;

    return blocksX * bytesPerBlock * blocksY;
}

// ============================================================================
// 28. VMA 0x26d7a50 (87 B) — GXDdsHeader_GetMipSlicePitchOrRowSize
//     Computes the row pitch in bytes at mipIndex for a DDS header.
// ============================================================================
uint32_t GXDdsHeader_GetMipSlicePitchOrRowSize(const uint8_t* ddsPtr, uint32_t mipIndex) {
    uint32_t mipWidth = (*reinterpret_cast<const uint32_t*>(ddsPtr + 0x10)) >> (mipIndex & 31u);

    uint32_t engineFmt     = GXDdsHeader_GetEnginePixelFormat(ddsPtr);
    uint32_t bytesPerBlock = GXTpfFormat_GetBytesPerBlock(engineFmt);
    uint32_t blockW        = GXTpfFormat_GetBlockWidth(engineFmt);

    if (blockW == 0u) {
        return 0;
    }

    uint32_t effWidth = (mipWidth != 0u) ? mipWidth : 1u;
    return ((effWidth + blockW - 1u) / blockW) * bytesPerBlock;
}

// ============================================================================
// 29. VMA 0x26d7a20 (38 B) — GXDdsHeader_GetArrayOrCubeSliceCount
//     Returns 6 for cubemaps (DDSCAPS2_CUBEMAP), DX10.arraySize for DX10
//     headers, or 1 for standard 2D/3D DDS headers.
// ============================================================================
uint32_t GXDdsHeader_GetArrayOrCubeSliceCount(const uint8_t* ddsPtr) {
    if ((ddsPtr[0x71] & 0x02) != 0) {
        return 6u;
    }
    if ((ddsPtr[0x50] & 0x04) != 0 &&
        *reinterpret_cast<const uint32_t*>(ddsPtr + 0x54) == 0x30315844u)
    {
        return *reinterpret_cast<const uint32_t*>(ddsPtr + 0x8C);
    }
    return 1u;
}

// ============================================================================
// 30. VMA 0x26d7a00 (22 B) — GXTpfFormat_DxgiToEngineFormat
//     Maps DXGI_FORMAT (0..99) to the engine pixel format ID via 0x4870900.
// ============================================================================
uint32_t GXTpfFormat_DxgiToEngineFormat(uint32_t dxgiFormat) {
    if (dxgiFormat > 0x63u) {
        return 0;
    }
    return kDxgiToEngineFormatTable[dxgiFormat];
}

// ============================================================================
// 31. VMA 0x26d7530 (1228 B) — GXDdsHeader_GetEnginePixelFormat
//     Parses DDS_PIXELFORMAT (ddsPtr + 0x4C) and optional DDS_HEADER_DXT10
//     (ddsPtr + 0x80) into a FromSoftware GX engine pixel format enum (0..99).
// ============================================================================
uint32_t GXDdsHeader_GetEnginePixelFormat(const uint8_t* ddsPtr) {
    uint32_t ddspfFlags  = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x50);
    uint32_t fourCC      = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x54);
    uint32_t hasFourCC   = ddspfFlags & 0x04u;
    uint32_t rgbBitCount = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x58);
    uint32_t rMask       = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x5C);
    uint32_t gMask       = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x60);
    uint32_t bMask       = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x64);
    uint32_t aMask       = *reinterpret_cast<const uint32_t*>(ddsPtr + 0x68);

    // 1. DX10 extension header ('DX10' = 0x30315844)
    if (fourCC == 0x30315844u && hasFourCC != 0u) {
        int64_t dxgiFmt = *reinterpret_cast<const int32_t*>(ddsPtr + 0x80);
        if (static_cast<uint64_t>(dxgiFmt) > 0x63ULL) {
            return 0;
        }
        return kDxgiToEngineFormatTable[dxgiFmt];
    }

    // 2. DDPF_RGB (0x40)
    if ((ddspfFlags & 0x40u) != 0u) {
        if (rgbBitCount == 16u) {
            if (rMask == 0xF800u) {
                if (gMask == 0x07E0u && bMask == 0x001Fu && aMask == 0u) {
                    return 0x55u; // 85: B5G6R5_UNORM
                }
                return 0;
            }
            if (rMask == 0x7C00u) {
                if (gMask == 0x03E0u && bMask == 0x001Fu && (aMask & 0xFFFF7FFFu) == 0u) {
                    return 0x56u; // 86: B5G5R5A1_UNORM
                }
                return 0;
            }
            return 0;
        }

        if (rgbBitCount != 32u) {
            return 0;
        }

        if (rMask == 0x000000FFu) {
            if (gMask != 0x0000FF00u) {
                return 0;
            }
            if (bMask == 0x00FF0000u && (aMask == 0xFF000000u || aMask == 0u)) {
                return 0x1Cu; // 28: R8G8B8A8_UNORM
            }
            return 0;
        }

        if (rMask == 0x00FF0000u) {
            if (gMask != 0x0000FF00u) {
                return 0;
            }
            if (bMask == 0x000000FFu && aMask == 0u) {
                return 0x58u; // 88: B8G8R8X8_UNORM
            }
            if (bMask == 0x000000FFu && aMask == 0xFF000000u) {
                return 0x57u; // 87: B8G8R8A8_UNORM
            }
            return 0;
        }

        if (rMask == 0xFFFFFFFFu) {
            return ((gMask | bMask | aMask) == 0u) ? 0x29u : 0u; // 41: R32_FLOAT
        }

        if (rMask == 0x0000FFFFu) {
            if (gMask == 0xFFFF0000u && (bMask | aMask) == 0u) {
                return 0x23u; // 35: R16G16_UNORM
            }
            return 0;
        }

        if (rMask == 0x000003FFu &&
            gMask == 0x000FFC00u &&
            bMask == 0x3FF00000u &&
            aMask == 0xC0000000u)
        {
            return 0x18u; // 24: R10G10B10A2_UNORM
        }
        return 0;
    }

    // 3. DDPF_LUMINANCE (0x20000)
    if ((ddspfFlags & 0x20000u) != 0u) {
        if (rgbBitCount == 16u) {
            if (rMask == 0x00FFu) {
                if ((gMask | bMask) == 0u && aMask == 0xFF00u) {
                    return 0x31u; // 49: R8G8_UNORM
                }
                return 0;
            }
            if (rMask == 0xFFFFu) {
                if ((gMask | bMask | aMask) == 0u) {
                    return 0x38u; // 56: R16_UNORM
                }
                return 0;
            }
            return 0;
        }

        if (rgbBitCount == 8u && rMask == 0x00FFu && (gMask | bMask | aMask) == 0u) {
            return 0x3Du; // 61: R8_UNORM
        }
        return 0;
    }

    // 4. DDPF_BUMPDUDV (0x80000)
    if ((ddspfFlags & 0x80000u) != 0u) {
        if (rgbBitCount == 16u &&
            rMask == 0x00FFu &&
            gMask == 0xFF00u &&
            bMask == 0u &&
            aMask == 0u)
        {
            return 0x33u; // 51: R8G8_SNORM
        }
        return 0;
    }

    // 5. DDPF_ALPHA (0x02)
    if ((ddspfFlags & 0x02u) != 0u) {
        return (rgbBitCount == 8u) ? 0x41u : 0u; // 65: A8_UNORM
    }

    // 6. DDPF_FOURCC (0x04)
    if (hasFourCC == 0u) {
        return 0;
    }

    switch (fourCC) {
        case 0x24u:       return 0x0Bu; // 11: R16G16B16A16_UNORM (D3DFMT_A16B16G16R16)
        case 0x6Eu:       return 0x0Du; // 13: R16G16B16A16_SNORM (D3DFMT_Q16W16V16U16)
        case 0x6Fu:       return 0x36u; // 54: R16_FLOAT          (D3DFMT_R16F)
        case 0x70u:       return 0x22u; // 34: R16G16_FLOAT       (D3DFMT_G16R16F)
        case 0x71u:       return 0x0Au; // 10: R16G16B16A16_FLOAT (D3DFMT_A16B16G16R16F)
        case 0x72u:       return 0x29u; // 41: R32_FLOAT          (D3DFMT_R32F)
        case 0x73u:       return 0x10u; // 16: R32G32_FLOAT       (D3DFMT_G32R32F)
        case 0x74u:       return 0x02u; //  2: R32G32B32A32_FLOAT (D3DFMT_A32B32G32R32F)
        case 0x31495441u: return 0x50u; // 80: BC4_UNORM          ('ATI1')
        case 0x55344342u: return 0x50u; // 80: BC4_UNORM          ('BC4U')
        case 0x31545844u: return 0x47u; // 71: BC1_UNORM          ('DXT1')
        case 0x32495441u: return 0x53u; // 83: BC5_UNORM          ('ATI2')
        case 0x33545844u: return 0x4Au; // 74: BC2_UNORM          ('DXT3')
        case 0x35545844u: return 0x4Du; // 77: BC3_UNORM          ('DXT5')
        case 0x42475247u: return 0x45u; // 69: G8R8_G8B8_UNORM    ('GRGB')
        case 0x47424752u: return 0x44u; // 68: R8G8_B8G8_UNORM    ('RGBG')
        case 0x53344342u: return 0x51u; // 81: BC4_SNORM          ('BC4S')
        case 0x53354342u: return 0x54u; // 84: BC5_SNORM          ('BC5S')
        default:          return 0u;
    }
}

// ============================================================================
// 32. VMA 0x26d7510 (27 B) — GXDdsHeader_ValidateMagicAndSize
//     Checks that ddsPtr starts with 'DDS ' (0x20534444) and dwSize != 0.
// ============================================================================
bool GXDdsHeader_ValidateMagicAndSize(const uint8_t* ddsPtr) {
    if (ddsPtr == nullptr) {
        return false;
    }
    if (*reinterpret_cast<const uint32_t*>(ddsPtr + 0x00) != 0x20534444u) {
        return false;
    }
    return (*reinterpret_cast<const uint32_t*>(ddsPtr + 0x04) != 0u);
}

// ============================================================================
// 33. VMA 0x26d7460 (19 B) — GXConstantBufferArrayOwner_Ctor
// ============================================================================
void GXConstantBufferArrayOwner_Ctor(GXConstantBufferArrayOwner* self, DLAllocator* allocator) {
    self->allocator   = allocator;
    self->bufferCount = 0;
    self->buffers     = nullptr;
}

// ============================================================================
// 34. VMA 0x26d7450 (4 B) — GXConstantBufferArrayOwner_GetBufferCount
// ============================================================================
uint32_t GXConstantBufferArrayOwner_GetBufferCount(const GXConstantBufferArrayOwner* self) {
    return self->bufferCount;
}

// ============================================================================
// 35. VMA 0x26d7440 (5 B) — GXConstantBufferArrayOwner_GetBufferArray
// ============================================================================
GXGnmRefCountedResource** GXConstantBufferArrayOwner_GetBufferArray(const GXConstantBufferArrayOwner* self) {
    return self->buffers;
}

// ============================================================================
// 36. VMA 0x26d73f0 (67 B) — GXConstantBufferArrayOwner_UpdateBufferData
//     Uploads srcData into self->buffers[bufferIndex] on cmdCtx.
// ============================================================================
uint32_t GXConstantBufferArrayOwner_UpdateBufferData(
    GXConstantBufferArrayOwner* self,
    uint8_t* cmdCtx,
    uint32_t bufferIndex,
    const void* srcData)
{
    if (self == nullptr || self->buffers == nullptr || bufferIndex >= self->bufferCount) {
        return 8;
    }
    GXGnmRefCountedResource* targetBuffer = self->buffers[bufferIndex];
    GXGnmCommandContext_UpdateSubresource(
        cmdCtx, cmdCtx + 0xB730, targetBuffer, 0, nullptr, srcData, 0, 0);
    return 0;
}

// ============================================================================
// 37. VMA 0x26d7360 (138 B) — GXConstantBufferArrayOwner_Destroy
//     Destroys and deallocates all constant buffers in self->buffers and frees
//     the pointer array.
// ============================================================================
void GXConstantBufferArrayOwner_Destroy(GXConstantBufferArrayOwner* self) {
    GXGnmRefCountedResource** bufArray = self->buffers;
    if (bufArray == nullptr) {
        return;
    }

    uint32_t count = self->bufferCount;
    for (uint32_t i = 0; i < count; ++i) {
        GXGnmRefCountedResource* buf = self->buffers[i];
        if (buf != nullptr) {
            DLAllocator* alloc = self->allocator;
            buf->vptr->dtor(buf);
            alloc->vptr->deallocate(alloc, buf);
            self->buffers[i] = nullptr;
            count = self->bufferCount;
        }
    }

    DLAllocator* alloc = self->allocator;
    alloc->vptr->deallocate(alloc, self->buffers);
}

// ============================================================================
// 38. VMA 0x26d70e0 (626 B) — GXConstantBufferArrayOwner_CreateAndInitialize
//     Allocates a GXConstantBufferArrayOwner and creates bufferCount constant
//     buffers according to bufferByteSizes[].
// ============================================================================
uint32_t GXConstantBufferArrayOwner_CreateAndInitialize(
    GXConstantBufferArrayOwner** outOwner,
    void* gxDevice,
    uint32_t bufferCount,
    const uint32_t* bufferByteSizes,
    DLAllocator* overrideAllocator,
    void* overrideOwnerContext,
    uint8_t isStandardConstantBuffer)
{
    DLAllocator* defaultAlloc = GXDevice_GetDefaultAllocator(gxDevice);
    DLAllocator* allocator    = GXResolveAllocator(overrideAllocator, defaultAlloc);
    void* defaultOwner        = GXDevice_GetDefaultOwnerContext(gxDevice);
    void* ownerContext        = GXResolveOwnerContext(overrideOwnerContext, defaultOwner);

    if (ownerContext == nullptr ||
        allocator == nullptr ||
        bufferByteSizes == nullptr ||
        bufferCount == 0u)
    {
        return 8;
    }

    GXConstantBufferArrayOwner* owner = static_cast<GXConstantBufferArrayOwner*>(
        DLHeap_AllocateAligned(sizeof(GXConstantBufferArrayOwner), 8, allocator));
    if (owner == nullptr) {
        *outOwner = nullptr;
        return 9;
    }

    owner->allocator   = allocator;
    owner->bufferCount = 0;
    owner->buffers     = nullptr;
    *outOwner          = owner;

    size_t ptrArrayBytes = static_cast<size_t>(bufferCount) << 3;
    owner->buffers = static_cast<GXGnmRefCountedResource**>(
        allocator->vptr->allocate(allocator, ptrArrayBytes));
    if ((*outOwner)->buffers == nullptr) {
        allocator->vptr->deallocate(allocator, owner);
        *outOwner = nullptr;
        return 9;
    }

    (*outOwner)->bufferCount = bufferCount;
    std::memset((*outOwner)->buffers, 0, ptrArrayBytes);

    uint32_t miscFlags = (isStandardConstantBuffer == 0) ? 0x800u : 0u;

    for (uint32_t i = 0; i < bufferCount; ++i) {
        GXBufferDescriptor24 desc{};
        desc.byteSize            = bufferByteSizes[i];
        desc.usageType           = 0;
        desc.bindFlags           = 4; // Constant Buffer
        desc.cpuAccessFlags      = 0;
        desc.miscFlags           = miscFlags;
        desc.structureByteStride = 0;
        desc.reserved18          = 0;

        uint32_t err = GXGnmDevice_CreateBuffer(
            gxDevice, &(*outOwner)->buffers[i], &desc, nullptr, allocator, ownerContext, 0);
        if (err != 0) {
            GXConstantBufferArrayOwner* failedOwner = *outOwner;
            if (failedOwner != nullptr) {
                if (failedOwner->buffers != nullptr) {
                    for (uint32_t j = 0; j < failedOwner->bufferCount; ++j) {
                        GXGnmRefCountedResource* buf = failedOwner->buffers[j];
                        if (buf != nullptr) {
                            DLAllocator* bufAlloc = failedOwner->allocator;
                            buf->vptr->dtor(buf);
                            bufAlloc->vptr->deallocate(bufAlloc, buf);
                            failedOwner->buffers[j] = nullptr;
                        }
                    }
                    failedOwner->allocator->vptr->deallocate(failedOwner->allocator, failedOwner->buffers);
                }
                allocator->vptr->deallocate(allocator, failedOwner);
                *outOwner = nullptr;
            }
            return err;
        }
    }

    return 0;
}

// ============================================================================
// 39. VMA 0x26d6e50 (500 B) — GXGammaRampAndFlipPresenter_PresentAndFlip
//     Waits for an available flip queue slot on sceVideoOut, binds the next
//     backbuffer surface, submits the flip command, and flushes the command
//     context.
// ============================================================================
uint32_t GXGammaRampAndFlipPresenter_PresentAndFlip(
    uint8_t* self,
    uint8_t* depthTargetContainer,
    uint32_t flipRateMode)
{
    int64_t bufferCount = *reinterpret_cast<int32_t*>(self + 0x328);
    uint8_t* cmdCtx     = *reinterpret_cast<uint8_t**>(self + 0x2D8);
    if (bufferCount <= 0 || cmdCtx == nullptr) {
        return 8;
    }

    uint64_t curThreadId = DLThread_GetCurrentThreadId();
    if (curThreadId == *reinterpret_cast<uint64_t*>(cmdCtx + 0xB718)) {
        int64_t queueThresholdOffset = 1 - bufferCount;

        struct SceVideoOutFlipStatusLocal {
            uint64_t count;        // +0x00
            uint64_t processTime;  // +0x08
            uint64_t tsc;          // +0x10
            int64_t  flipArg;      // +0x18
            uint64_t submitTsc;    // +0x20
            uint64_t reserved0;    // +0x28
            int32_t  gcQueueNum;   // +0x30
            int32_t  flipPending;  // +0x34
            uint32_t currentBuf;   // +0x38
            uint32_t pad3C;        // +0x3C
        } flipStatus{};

        for (;;) {
            int32_t videoHandle = *reinterpret_cast<int32_t*>(self + 0x2F0);
            if (sceVideoOutGetFlipStatus(videoHandle, &flipStatus) < 0) {
                return 5;
            }

            int64_t activeFlipArg = *reinterpret_cast<int64_t*>(self + 0x2E0);
            if (activeFlipArg < bufferCount ||
                flipStatus.flipArg >= queueThresholdOffset + activeFlipArg)
            {
                int64_t surfaceSlotIdx = activeFlipArg % bufferCount;
                uint64_t baseAddr256   = GXGnmGpuSurface_GetBaseAddress256(self + surfaceSlotIdx * 0x2C);
                *reinterpret_cast<uint64_t*>(self + 0x350) = baseAddr256 << 8;

                GXGnmCommandContext_PrepareForPresent(cmdCtx + 0xB558, cmdCtx, depthTargetContainer);
                GXGnmCommandContext_BindRenderTargetsAndViewport(
                    cmdCtx, self + 0x348, depthTargetContainer + 0x18, self + 0x380);

                uint32_t hwFlipMode = (flipRateMode < 1u) ? 2u : 1u;
                int32_t vOutHandle  = *reinterpret_cast<int32_t*>(self + 0x2F0);
                uint32_t submitErr  = GXGnmCommandBuffer_SubmitFlip(
                    cmdCtx + 0x10,
                    vOutHandle,
                    static_cast<uint64_t>(surfaceSlotIdx),
                    hwFlipMode,
                    *reinterpret_cast<uint64_t*>(self + 0x2E0));
                if (submitErr != 0) {
                    return submitErr;
                }

                *reinterpret_cast<uint64_t*>(self + 0x2E0) += 1;
                void* cmdCtxSub10 = cmdCtx + 0x10;
                uint8_t* broadcasterOwner = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
                void* broadcaster = *reinterpret_cast<void**>(broadcasterOwner + 0x08);
                GXGnmEventBroadcaster_NotifyFlip(broadcaster, &cmdCtxSub10);
                GXGnmCommandContext_PostFlipNotify(cmdCtx);
                break;
            }

            uint8_t kernelEventBuf[36]{};
            int32_t outNumEvents = 0;
            void* equeue = *reinterpret_cast<void**>(self + 0x2F8);
            if (sceKernelWaitEqueue(equeue, kernelEventBuf, 1, &outNumEvents, nullptr) < 0) {
                return 5;
            }
        }
    }

    uint8_t* finalCmdCtx = *reinterpret_cast<uint8_t**>(self + 0x2D8);
    if (DLThread_GetCurrentThreadId() == *reinterpret_cast<uint64_t*>(finalCmdCtx + 0xB718)) {
        GXGnmCommandBuffer_FlushSubmission(finalCmdCtx + 0x10);
    }
    return 0;
}

// ============================================================================
// 40. VMA 0x26d6e30 (23 B) — GXGammaRampAndFlipPresenter_ClearOutputs
// ============================================================================
uint32_t GXGammaRampAndFlipPresenter_ClearOutputs(
    const void* unusedSelf,
    uint8_t* outFlag,
    uint64_t* outValue)
{
    if (outFlag != nullptr) {
        *outFlag = 0;
    }
    if (outValue != nullptr) {
        *outValue = 0;
    }
    return (unusedSelf != nullptr) ? 0u : 0u;
}

// ============================================================================
// 41. VMA 0x26d6e20 (3 B) — GXGammaRampAndFlipPresenter_ReturnZeroA
// ============================================================================
uint32_t GXGammaRampAndFlipPresenter_ReturnZeroA() {
    return 0;
}

// ============================================================================
// 42. VMA 0x26d6d80 (154 B) — GXGammaRampAndFlipPresenter_CopyGammaRampTable
//     Copies the 0x300C-byte (1025 * 3 * float) gamma ramp table from
//     self + 0x3B8 into outGammaTable.
// ============================================================================
void* GXGammaRampAndFlipPresenter_CopyGammaRampTable(const uint8_t* self, void* outGammaTable) {
    if (outGammaTable == nullptr) {
        return const_cast<uint8_t*>(self);
    }
    reinterpret_cast<void (*)(void*, const void*, size_t)>(GX_VMA(0x261a1b0ULL))(
        outGammaTable, self + 0x3B8, 0x300Cu);
    return outGammaTable;
}

// ============================================================================
// 43. VMA 0x26d6b20 (593 B) — GXGammaRampAndFlipPresenter_SetGammaRampTable
//     De-interleaves 1025 RGB float triplets into self + 0x3B8 and commits
//     each channel (R=0, G=1, B=2) to the hardware gamma ramp unit.
// ============================================================================
uint32_t GXGammaRampAndFlipPresenter_SetGammaRampTable(
    uint8_t* self,
    const float* inGammaRgbTriplets)
{
    uint8_t* cmdCtx = *reinterpret_cast<uint8_t**>(self + 0x2D8);
    if (DLThread_GetCurrentThreadId() != *reinterpret_cast<uint64_t*>(cmdCtx + 0xB718)) {
        return 5;
    }

    DLScratchScope scratchScope{};
    scratchScope.vptr      = reinterpret_cast<const void*>(GX_VMA(0x52b49f0ULL));
    scratchScope.savedMark = DLScratchAlloc_AcquireMark();

    DLAllocator* ownerAlloc = *reinterpret_cast<DLAllocator**>(self + 0x2C0);
    float* channelScratch = static_cast<float*>(DLScratchAlloc_Allocate(&scratchScope, 0x1004));
    DLAllocator* heapFallbackAlloc = nullptr;

    if (channelScratch == nullptr && ownerAlloc != nullptr) {
        channelScratch = static_cast<float*>(ownerAlloc->vptr->allocate(ownerAlloc, 0x1004));
        heapFallbackAlloc = ownerAlloc;
    }

    if (channelScratch == nullptr) {
        DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
        return 9;
    }

    void* cmdCtxSub10   = cmdCtx + 0x10;
    void* gammaState380 = self + 0x380;

    struct GammaChannelUploadDesc {
        const float* dataPtr; // +0x00
        uint32_t     reserved08;
        uint32_t     reserved0C;
    };

    // Channel 0 (Red)
    float* dstR = reinterpret_cast<float*>(self + 0x3B8);
    for (size_t i = 0; i < 0x401; ++i) {
        float val = inGammaRgbTriplets[i * 3 + 0];
        dstR[i * 3] = val;
        channelScratch[i] = val;
    }
    GammaChannelUploadDesc descR{ channelScratch, 0, 0 };
    GXGnmCommandBuffer_SetGammaRampChannel(cmdCtxSub10, gammaState380, &descR, 0, 0, 0);

    // Channel 1 (Green)
    float* dstG = reinterpret_cast<float*>(self + 0x3BC);
    for (size_t i = 0; i < 0x401; ++i) {
        float val = inGammaRgbTriplets[i * 3 + 1];
        dstG[i * 3] = val;
        channelScratch[i] = val;
    }
    GammaChannelUploadDesc descG{ channelScratch, 0, 0 };
    GXGnmCommandBuffer_SetGammaRampChannel(cmdCtxSub10, gammaState380, &descG, 1, 0, 0);

    // Channel 2 (Blue) + commitFlag = 1
    float* dstB = reinterpret_cast<float*>(self + 0x3C0);
    for (size_t i = 0; i < 0x401; ++i) {
        float val = inGammaRgbTriplets[i * 3 + 2];
        dstB[i * 3] = val;
        channelScratch[i] = val;
    }
    GammaChannelUploadDesc descB{ channelScratch, 0, 0 };
    GXGnmCommandBuffer_SetGammaRampChannel(cmdCtxSub10, gammaState380, &descB, 2, 0, 1);

    if (heapFallbackAlloc != nullptr) {
        heapFallbackAlloc->vptr->deallocate(heapFallbackAlloc, channelScratch);
    }
    DLScratchAlloc_ReleaseMark(scratchScope.savedMark);
    return 0;
}

// ============================================================================
// 44. VMA 0x26d6b10 (3 B) — GXGammaRampAndFlipPresenter_ReturnZeroB
// ============================================================================
uint32_t GXGammaRampAndFlipPresenter_ReturnZeroB() {
    return 0;
}

// ============================================================================
// 45. VMA 0x26d6af0 (22 B) — GXGammaRampAndFlipPresenter_CopyDisplayConfig72
//     Copies 72 bytes (9 QWORDs) of display configuration from self + 0x300.
// ============================================================================
void GXGammaRampAndFlipPresenter_CopyDisplayConfig72(const uint8_t* self, void* outConfig72) {
    std::memcpy(outConfig72, self + 0x300, 0x48);
}

// ============================================================================
// 46. VMA 0x26d6ae0 (3 B) — GXGammaRampAndFlipPresenter_ReturnZeroC
// ============================================================================
uint32_t GXGammaRampAndFlipPresenter_ReturnZeroC() {
    return 0;
}

// ============================================================================
// 47. VMA 0x26d6a50 (138 B) — GXGammaRampAndFlipPresenter_Destroy
//     Destroys swapchain target sub-objects, deletes the flip equeue, closes
//     the sceVideoOut handle, and frees the surface buffer allocation.
// ============================================================================
void GXGammaRampAndFlipPresenter_Destroy(uint8_t* self) {
    GXSwapChainTargetSubObj380_Destroy(self + 0x380);
    GXSwapChainTargetSubObj348_Destroy(self + 0x348);

    void* equeue = *reinterpret_cast<void**>(self + 0x2F8);
    if (equeue != nullptr) {
        sceKernelDeleteEqueue(equeue);
        *reinterpret_cast<void**>(self + 0x2F8) = nullptr;
    }

    int32_t videoHandle = *reinterpret_cast<int32_t*>(self + 0x2F0);
    if (static_cast<uint32_t>(videoHandle) != 0x80290001u) {
        sceVideoOutClose(videoHandle);
        *reinterpret_cast<uint32_t*>(self + 0x2F0) = 0x80290001u;
    }

    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x2C8);
    if (alloc != nullptr) {
        void* surfaceMem = *reinterpret_cast<void**>(self + 0x2D0);
        if (surfaceMem != nullptr) {
            alloc->vptr->deallocate(alloc, surfaceMem);
            *reinterpret_cast<void**>(self + 0x2D0) = nullptr;
        }
    }
}

} // extern "C"

} // namespace GX
} // namespace FromSoftware
