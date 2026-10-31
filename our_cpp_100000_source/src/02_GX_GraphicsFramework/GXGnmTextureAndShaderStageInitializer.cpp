#include <cstdint>
#include <cstddef>
#include <cstring>
#include "gx_direct_vulkan.h"

// ============================================================================
// Bloodborne PS4 Engine (ORBIS) - 02_GX_GraphicsFramework (Batch 11)
// Subsystem: GX Gnm Texture, RenderTarget, UAV, FetchShader & Shader Stage Initializer
// VMA Range: 0x216af50 .. 0x217547f (80 functions, 38,487 bytes)
// Quality Remediation: 100% Complete Decompilation (Zero Stubs Policy)
// ============================================================================

namespace GXGraphicsFramework {

struct GXAllocatorVTable {
    void* vfunc_00[10];
    void* (*AllocCpu)(void* self, uint64_t size);                            // +0x50
    void* (*AllocAligned)(void* self, uint64_t size, uint32_t alignment);    // +0x58
    void* vfunc_60;                                                          // +0x60
    void* vfunc_68;                                                          // +0x68
    void  (*Free)(void* self, void* ptr);                                    // +0x70
};

struct GXAllocator {
    const GXAllocatorVTable* vptr;
};

struct GXShaderProgramBase {
    uint8_t      pad_00[0xd2];
    uint8_t      hasConstantBufferSlot;  // +0xd2
    uint8_t      pad_d3[0x1d];
    uint64_t     constantBufferVSharp[2];// +0xf0
    void*        shaderHeaderCpu;        // +0x100
    void*        shaderUcodeGpu;         // +0x108
    GXAllocator* cpuAllocator;           // +0x110
    GXAllocator* gpuAllocator;           // +0x118
    uint32_t     gsMaxVertOutMode;       // +0x120
    uint32_t     copyShaderCbVSharp[4];  // +0x124
    uint8_t      hasCopyShaderCbSlot;    // +0x134
    uint8_t      streamOutEnabled;       // +0x135
    uint8_t      pad_136[2];
    uint64_t     streamOutMaskAndCfg;    // +0x138 (byte +0x138 = streamMask, dword +0x13c = vsOutCfg)
    uint16_t     streamStridesDwords[4]; // +0x140
    uint32_t     streamControl[4];       // +0x148
};

struct GXStreamOutElement28 {
    uint32_t key;       // +0x00
    uint32_t streamIdx; // +0x04
    uint32_t mask;      // +0x08
    uint32_t offset;    // +0x0c
    uint32_t stride;    // +0x10
    uint32_t semantic;  // +0x14
    uint32_t component; // +0x18
};

struct GXStreamOutVector {
    void*                 vptr_or_ctx; // +0x00
    GXStreamOutElement28* begin;       // +0x08
    GXStreamOutElement28* end;         // +0x10
    GXStreamOutElement28* cap;         // +0x18
    GXAllocator*          allocator;   // +0x20
};

// External helper declarations resolved from db/functions.json & binary targets
extern "C" {
    int32_t  CGShaderBinaryGnm_AllocateAndUploadUcode(void** outGpuPtr, uint64_t* outGpuSize, void* ctx, const void* shdrData, GXAllocator* gpuAlloc, uint32_t stageType); // 0x26bb570
    int32_t  CGShaderFactoryGnm_CreateHullShader(void** outCpuHeader, void* programObj, uint64_t gpuSize, void* gpuPtr, const void* shdrData, uint64_t shdrSize, uint32_t stageType, GXAllocator* cpuAlloc, uint64_t arg8, uint64_t arg9); // 0x26ba800
    int32_t  CGShaderFactoryGnm_CreateDomainShader(void** outCpuHeader, void* programObj, uint64_t gpuSize, void* gpuPtr, const void* shdrData, uint64_t shdrSize, uint32_t stageType, GXAllocator* cpuAlloc, uint64_t arg8, uint64_t arg9); // 0x26ba540
    int32_t  CGShaderFactoryGnm_CreateExportShader(void** outCpuHeader, void* programObj, uint64_t gpuSize, void* gpuPtr, const void* shdrData, uint64_t shdrSize, uint32_t stageType, GXAllocator* cpuAlloc, uint64_t arg8, uint64_t arg9); // 0x26baac0
    int32_t  CGShaderFactoryGnm_CreatePixelShader(void** outCpuHeader, void* programObj, uint64_t gpuSize, void* gpuPtr, const void* shdrData, uint64_t shdrSize, uint32_t stageType, GXAllocator* cpuAlloc, uint64_t arg8, uint64_t arg9); // 0x26b9fc0
    int32_t  CGShaderFactoryGnm_CreateVertexShader(void** outCpuHeader, void* programObj, uint64_t gpuSize, void* gpuPtr, const void* shdrData, uint64_t shdrSize, uint32_t stageType, GXAllocator* cpuAlloc, uint64_t arg8, uint64_t arg9); // 0x26b9cf0
    uint8_t  CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(void* outVSharp16B, const void* shaderHeader, uint32_t regLow, uint32_t regHigh); // 0x26bb670
    int32_t  GXGnmShaderStageInit_SetupGsAndCopyShader(void* programObj, void* copyShaderHeaderDst, uint64_t count, uint32_t stageType, void* shdrParser, const void* gsBinHeader, uint32_t elemCount, GXAllocator* cpuAlloc); // 0x21754e0
    void     SceShaderBinaryInfo_Init(void* info); // 0x10a3b70
    int32_t  SceShaderBinaryInfo_Parse(void* info, const void* data, uint32_t size); // 0x10a3c40
    void     SceShaderBinaryInfo_Destroy(void* info); // 0x10a3bf0
    void     SceGsShaderBinary_ExtractStagePointers(void* outGsPtrs, void* outVsCopyPtrs, const void* shdrData); // 0x109c410
    void     SceGnmVsStreamOut_SetStreamControl(void* streamCtrlArray, uint32_t streamIdx, uint32_t mask); // 0x1087d60
    uint8_t  CGStreamOutGnm_ResolveSemanticAndComponent(uint32_t* outSemanticType, const void* attrDesc); // 0x26b7ec0
    uint8_t  CGStreamOutGnm_ResolveComponentMaskAndCount(uint32_t* outCount, uint32_t* outMask, uint32_t rawMask); // 0x26b7f30
    uint8_t  CGMemoryGnm_IsCpuAccessibleAddress(const void* addr); // 0x26bad70
    uint8_t  CGMemoryGnm_IsGarlicAddress(const void* addr); // 0x26badd0
    void*    CGMemoryGnm_Memcpy(void* dst, const void* src, uint64_t size); // 0x261a1b0
    void*    CGMemoryGnm_MemsetZero(void* dst, const void* src, uint64_t size); // 0x2bbe578
    uint32_t CGResourceIdGnm_AllocateNextHandle(); // 0x26baef0
    uint8_t  CGFormatGnm_IsValidDataFormat(uint32_t fmt); // 0x26bb4b0
    uint8_t  CGFormatGnm_HasFmaskOrHtileFlag(uint32_t flags, uint32_t fmt); // 0x26bb4f0
    void     CGErrorGnm_ReportGpuAddrError(int32_t errCode); // 0x26bb4a0
    int32_t  CGFormatGnm_ConvertToGnmDataFormat(uint32_t* outDataFmt, uint32_t gxFormat); // 0x26b7310
    int32_t  CGFormatGnm_ConvertToDepthFormat(uint32_t* outZFormat, uint32_t gxFormat); // 0x26b7720
    int32_t  CGFormatGnm_ConvertToStencilFormat(uint32_t* outStencilFmt, uint32_t gxFormat); // 0x26b78a0
    uint32_t CGFormatGnm_ResolveTileModeIndex(uint32_t texDimType, uint32_t gxFormat, uint32_t usageFlags, uint32_t msaaOrBindFlags, uint32_t extraFlags); // 0x26baf10
    uint32_t CGFormatGnm_ResolveNumFragments(uint32_t sampleCount, uint32_t sampleQuality, uint32_t isEqaa); // 0x26b7ab0
    uint32_t CGFormatGnm_FragmentLog2ToCount(uint32_t fragLog2); // 0x26b7e40
    int32_t  SceGpuAddress_ComputeTileMode(uint32_t* outTileMode, uint32_t arrayModeIdx, uint32_t dataFormat, uint32_t numSamples); // 0x109cb30
    int32_t  SceGnmTexture3D_InitAndComputeSize(uint64_t* outSizeAlign, void* outGnmTex, const void* desc, const void* customTile); // 0x2ba73a0
    int32_t  SceGnmTexture2D_InitAndComputeSize(uint64_t* outSizeAlign, void* outGnmTex, const void* desc, const void* customTile); // 0x2ba6af0
    int32_t  SceGnmTexture1D_InitAndComputeSize(uint64_t* outSizeAlign, void* outGnmTex, const void* desc, const void* customTile); // 0x2ba64a0
    uint32_t SceGnmTexture_GetLastMipLevel(const void* gnmTex); // 0x1089910
    uint32_t SceGnmTexture_GetTileMode(const void* gnmTex); // 0x10898f0
    uint64_t SceGnmTexture_Init2DAndReturnSizeAlign(void* outGnmTex, uint32_t width, uint32_t height, uint32_t numSlices, uint32_t numMips, uint32_t dataFormat, uint32_t tileMode, uint32_t numFragments, uint32_t isCubemap); // 0x1087ee0
    void     SceGnmTexture_SetResourceMemoryType(void* gnmTex, uint32_t memType); // 0x1088370
    int32_t  SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(uint64_t* outOffset, uint64_t* outSize, const void* gnmTex, uint32_t mipLevel, uint32_t arraySlice); // 0x10a2c40
    void     SceGpuAddress_InitTilingParametersFromTexture(void* outTp, const void* gnmTex, uint32_t mipLevel, uint32_t arraySlice); // 0x10a1e80
    int32_t  SceGpuAddress_TileSurface(void* dstTiled, const void* srcLinear, const void* tilingParams); // 0x10a3410
    void     CGCommandContextGnm_DmaUploadTextureSubresource(void* cmdCtxPlus40, void* texResource, const void* subresourceDesc, uint32_t subresourceIdx, uint32_t flags); // 0x26b4230
    void     CGCommandContextGnm_FlushAndWaitIdle(void* cmdCtxPlus40); // 0x26b1930
    void     CGCommandContextGnm_SubmitAndReset(void* cmdCtxPlus40); // 0x26b18c0
    void     GXLogging_LogError(uint32_t channel, uint32_t line, const char* fmt, ...); // 0x20b55b0
    void     GXThreadLocalAllocator_EnsureInitialized(); // 0x207d250
    void     GXException_ThrowOutOfMemory(void* excObj, uint32_t code); // 0x2bbe978
}

// Forward declaration within this compilation unit (defined at 0x216d7f0)
extern "C" uint64_t GXGnmTexture2D_InitGnmTextureFromSurfaceInfo(const void* surfaceInfo, void* outGnmTex);

static inline void RetireDirectShaderProgramUcode(const GXShaderProgramBase* self, bool isGs = false) noexcept {
    if (!self || !self->shaderUcodeGpu) {
        return;
    }
    uint64_t span = 256ULL;
    if (self->shaderHeaderCpu) {
        const uint8_t* hdr = static_cast<const uint8_t*>(self->shaderHeaderCpu);
        span = static_cast<uint64_t>(*reinterpret_cast<const uint32_t*>(hdr) & 0x7FFFFFu) + 256ULL;
        if (isGs) {
            const uint32_t gsDwords = hdr[0x03];
            const uint64_t copyLo = *reinterpret_cast<const uint32_t*>(hdr + gsDwords * 4 + 0x40);
            const uint64_t copyHi = *reinterpret_cast<const uint32_t*>(hdr + gsDwords * 4 + 0x44);
            const uint64_t copyAddr = (copyLo << 8) | (copyHi << 40);
            if (copyAddr != 0) {
                const uint32_t copySize =
                    *reinterpret_cast<const uint32_t*>(hdr + gsDwords * 4 + 0x38) & 0x7FFFFFu;
                gx_direct_vk::RetireDirectShaderRange(
                    reinterpret_cast<const void*>(copyAddr), static_cast<uint64_t>(copySize) + 256ULL);
            }
        }
    }
    gx_direct_vk::RetireDirectShaderRange(self->shaderUcodeGpu, span);
}

// [VMA: 0x21753b0 - 0x21753ea] (58 bytes)
// Releases CPU shader header (+0x100) via CPU allocator (+0x110) and GPU ucode (+0x108) via GPU allocator (+0x118).
extern "C" void GXGnmHullShaderProgram_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2175280 - 0x21753ab] (299 bytes)
// Uploads Hull Shader ucode (stage=2), creates HS header via CGShaderFactoryGnm, and initializes constant buffer V# slot.
extern "C" int32_t GXGnmHullShaderProgram_InitializeFromBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc)
{
    uint64_t gpuSize = 0;
    int32_t err = CGShaderBinaryGnm_AllocateAndUploadUcode(&self->shaderUcodeGpu, &gpuSize, ctx, shdrData, gpuAlloc, 2u);
    if (err != 0) {
        return err;
    }

    err = CGShaderFactoryGnm_CreateHullShader(
        &self->shaderHeaderCpu,
        self,
        gpuSize,
        self->shaderUcodeGpu,
        shdrData,
        shdrSize,
        2u,
        cpuAlloc,
        0,
        0);
    if (err != 0) {
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        return err;
    }

    const uint32_t* headerWords = static_cast<const uint32_t*>(self->shaderHeaderCpu);
    self->hasConstantBufferSlot = CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
        self->constantBufferVSharp,
        self->shaderHeaderCpu,
        headerWords[3],
        headerWords[2]);
    self->cpuAllocator = cpuAlloc;
    self->gpuAllocator = gpuAlloc;
    return 0;
}

// [VMA: 0x21751b0 - 0x21751ea] (58 bytes)
// Releases Domain Shader CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmDomainShaderProgram_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2175080 - 0x21751ab] (299 bytes)
// Uploads Domain Shader ucode (stage=7), creates DS/VS header via CGShaderFactoryGnm, and initializes constant buffer V# slot.
extern "C" int32_t GXGnmDomainShaderProgram_InitializeFromBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc)
{
    uint64_t gpuSize = 0;
    int32_t err = CGShaderBinaryGnm_AllocateAndUploadUcode(&self->shaderUcodeGpu, &gpuSize, ctx, shdrData, gpuAlloc, 7u);
    if (err != 0) {
        return err;
    }

    err = CGShaderFactoryGnm_CreateDomainShader(
        &self->shaderHeaderCpu,
        self,
        gpuSize,
        self->shaderUcodeGpu,
        shdrData,
        shdrSize,
        7u,
        cpuAlloc,
        0,
        0);
    if (err != 0) {
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        return err;
    }

    const uint32_t* headerWords = static_cast<const uint32_t*>(self->shaderHeaderCpu);
    self->hasConstantBufferSlot = CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
        self->constantBufferVSharp,
        self->shaderHeaderCpu,
        headerWords[3],
        headerWords[2]);
    self->cpuAllocator = cpuAlloc;
    self->gpuAllocator = gpuAlloc;
    return 0;
}

// Helper inline for 28-byte element swap in GXStreamOutElementArray_IntrosortBySemanticKey
static inline void SwapStreamOutElement28(GXStreamOutElement28* a, GXStreamOutElement28* b) {
    GXStreamOutElement28 tmp = *a;
    *a = *b;
    *b = tmp;
}

// [VMA: 0x21740e0 - 0x217507a] (3994 bytes)
// Introsort (quicksort + heapsort fallback + final insertion sort) on an array of 28-byte GXStreamOutElement28 records sorted by key (+0x00).
extern "C" void GXStreamOutElementArray_IntrosortBySemanticKey(
    GXStreamOutElement28* first,
    GXStreamOutElement28* last,
    int64_t depthLimit)
{
    int64_t byteSpan = reinterpret_cast<uint8_t*>(last) - reinterpret_cast<uint8_t*>(first);
    bool useHeapsort = (byteSpan > 0x380);
    GXStreamOutElement28* partitionEnd = last;

    while (byteSpan >= 0x381) {
        if (depthLimit <= 0) {
            partitionEnd = last;
            break;
        }

        GXStreamOutElement28* lastMinusOne = last - 1;
        int64_t halfCount = (last - first) / 2;
        GXStreamOutElement28* mid = first + halfCount;
        int64_t spanToLastMinusOne = reinterpret_cast<uint8_t*>(lastMinusOne) - reinterpret_cast<uint8_t*>(first);

        if (spanToLastMinusOne >= 0x461) {
            // Tukey ninther median-of-medians pivot selection
            int64_t step = ((lastMinusOne - first) + 1) / 8;
            GXStreamOutElement28* p1 = first + step;
            GXStreamOutElement28* p2 = first + step * 2;
            if (p1->key < first->key) SwapStreamOutElement28(p1, first);
            if (p2->key < p1->key) {
                SwapStreamOutElement28(p2, p1);
                if (p1->key < first->key) SwapStreamOutElement28(p1, first);
            }

            GXStreamOutElement28* m0 = first + (halfCount - step);
            GXStreamOutElement28* m2 = first + (halfCount + step);
            if (mid->key < m0->key) SwapStreamOutElement28(mid, m0);
            if (m2->key < mid->key) {
                SwapStreamOutElement28(m2, mid);
                if (mid->key < m0->key) SwapStreamOutElement28(mid, m0);
            }

            GXStreamOutElement28* e0 = last - 1 - step * 2;
            GXStreamOutElement28* e1 = last - 1 - step;
            if (e1->key < e0->key) SwapStreamOutElement28(e1, e0);
            if (lastMinusOne->key < e1->key) {
                SwapStreamOutElement28(lastMinusOne, e1);
                if (e1->key < e0->key) SwapStreamOutElement28(e1, e0);
            }

            if (mid->key < p1->key) SwapStreamOutElement28(mid, p1);
            if (e1->key < mid->key) {
                SwapStreamOutElement28(e1, mid);
                if (mid->key < p1->key) SwapStreamOutElement28(mid, p1);
            }
        } else {
            // Median-of-three pivot selection
            if (mid->key < first->key) SwapStreamOutElement28(mid, first);
            if (lastMinusOne->key < mid->key) {
                SwapStreamOutElement28(lastMinusOne, mid);
                if (mid->key < first->key) SwapStreamOutElement28(mid, first);
            }
        }

        // Bentley-McIlroy 3-way / fat-pivot partition around mid
        GXStreamOutElement28* eqLeft = mid;
        while (eqLeft > first && eqLeft->key == (eqLeft - 1)->key) {
            --eqLeft;
        }
        GXStreamOutElement28* eqRight = mid + 1;
        while (eqRight < last && eqLeft->key == eqRight->key) {
            ++eqRight;
        }

        GXStreamOutElement28* curRight = eqRight;
        GXStreamOutElement28* curLeft = eqLeft;

        for (;;) {
            for (; curRight < last; ++curRight) {
                uint32_t pivotKey = eqLeft->key;
                uint32_t rightKey = curRight->key;
                if (pivotKey < rightKey) {
                    continue;
                }
                if (rightKey < pivotKey) {
                    break;
                }
                SwapStreamOutElement28(eqRight, curRight);
                ++eqRight;
            }

            for (; curLeft > first; --curLeft) {
                uint32_t prevKey = (curLeft - 1)->key;
                uint32_t pivotKey = eqLeft->key;
                if (prevKey < pivotKey) {
                    continue;
                }
                if (pivotKey < prevKey) {
                    break;
                }
                --eqLeft;
                SwapStreamOutElement28(eqLeft, curLeft - 1);
            }

            if (curLeft == first && curRight == last) {
                break;
            }

            if (curLeft == first) {
                if (eqRight != curRight) {
                    SwapStreamOutElement28(eqLeft, eqRight);
                }
                ++eqRight;
                SwapStreamOutElement28(eqLeft, curRight);
                ++eqLeft;
                ++curRight;
            } else if (curRight == last) {
                --curLeft;
                --eqLeft;
                if (curLeft != eqLeft) {
                    SwapStreamOutElement28(curLeft, eqLeft);
                }
                --eqRight;
                SwapStreamOutElement28(eqLeft, eqRight);
            } else {
                --curLeft;
                SwapStreamOutElement28(curRight, curLeft);
                ++curRight;
            }
        }

        depthLimit = (depthLimit / 2) + (depthLimit / 4);
        int64_t leftBytes = reinterpret_cast<uint8_t*>(eqLeft) - reinterpret_cast<uint8_t*>(first);
        int64_t rightBytes = reinterpret_cast<uint8_t*>(last) - reinterpret_cast<uint8_t*>(eqRight);

        if (leftBytes < rightBytes) {
            GXStreamOutElementArray_IntrosortBySemanticKey(first, eqLeft, depthLimit);
            first = eqRight;
            byteSpan = rightBytes;
            useHeapsort = (byteSpan > 0x380);
        } else {
            GXStreamOutElementArray_IntrosortBySemanticKey(eqRight, last, depthLimit);
            last = eqLeft;
            byteSpan = leftBytes;
            useHeapsort = (byteSpan > 0x380);
        }
        partitionEnd = last;
    }

    if (useHeapsort) {
        // Heapsort fallback when recursion depth limit is exhausted while span > 0x380
        int64_t n = partitionEnd - first;
        for (int64_t start = n / 2; start > 0; ) {
            --start;
            GXStreamOutElement28 val = first[start];
            int64_t hole = start;
            int64_t child = hole * 2 + 2;
            while (child < n) {
                int64_t leftChild = hole * 2 + 1;
                if (first[child].key < first[leftChild].key) {
                    child = leftChild;
                }
                first[hole] = first[child];
                hole = child;
                child = hole * 2 + 2;
            }
            if (child == n) {
                first[hole] = first[n - 1];
                hole = n - 1;
            }
            while (hole > start) {
                int64_t parent = (hole - 1) / 2;
                if (first[parent].key >= val.key) {
                    break;
                }
                first[hole] = first[parent];
                hole = parent;
            }
            first[hole] = val;
        }

        GXStreamOutElement28* heapEnd = partitionEnd;
        while ((reinterpret_cast<uint8_t*>(heapEnd) - reinterpret_cast<uint8_t*>(first)) > 0x1c) {
            --heapEnd;
            GXStreamOutElement28 val = *heapEnd;
            *heapEnd = *first;
            int64_t nRem = heapEnd - first;
            int64_t hole = 0;
            int64_t child = 2;
            while (child < nRem) {
                int64_t leftChild = child - 1;
                if (first[child].key >= first[leftChild].key) {
                    leftChild = child;
                }
                first[hole] = first[leftChild];
                hole = leftChild;
                child = hole * 2 + 2;
            }
            if (child == nRem) {
                first[hole] = first[nRem - 1];
                hole = nRem - 1;
            }
            while (hole > 0) {
                int64_t parent = (hole - 1) / 2;
                if (first[parent].key >= val.key) {
                    break;
                }
                first[hole] = first[parent];
                hole = parent;
            }
            first[hole] = val;
        }
        return;
    }

    // Final insertion sort pass when span >= 29 bytes (more than 1 element)
    if (byteSpan >= 0x1d && first != partitionEnd && (first + 1) != partitionEnd) {
        for (GXStreamOutElement28* it = first + 1; it != partitionEnd; ++it) {
            GXStreamOutElement28 val = *it;
            if (val.key < first->key) {
                for (GXStreamOutElement28* p = it; p != first; --p) {
                    *p = *(p - 1);
                }
                *first = val;
            } else {
                GXStreamOutElement28* pos = it;
                while (val.key < (pos - 1)->key) {
                    *pos = *(pos - 1);
                    --pos;
                }
                *pos = val;
            }
        }
    }
}

// [VMA: 0x2173f80 - 0x21740db] (347 bytes)
// Grows a 28-byte element vector by 1.5x (clamped to max capacity 0x924924924924924), copies existing elements, and frees old buffer.
extern "C" void GXStreamOutElementVector_GrowAndReallocate(GXStreamOutVector* vec) {
    if (vec->cap != vec->end) {
        return;
    }

    uint64_t oldCount = static_cast<uint64_t>(vec->end - vec->begin);
    uint64_t halfCount = oldCount >> 1;
    const uint64_t maxElements = 0x0924924924924924ULL;
    uint64_t newCap = 0;
    if ((maxElements - halfCount) >= oldCount) {
        newCap = oldCount + halfCount;
    }
    if (newCap < oldCount + 1) {
        newCap = oldCount + 1;
    }

    uint64_t allocBytes = newCap * sizeof(GXStreamOutElement28);
    GXStreamOutElement28* newBuf = static_cast<GXStreamOutElement28*>(
        vec->allocator->vptr->AllocAligned(vec->allocator, allocBytes, 4u));

    if (!newBuf) {
        GXLogging_LogError(0, 0x109, "Out of memory in GXStreamOutElementVector_GrowAndReallocate");
    }

    GXStreamOutElement28* src = vec->begin;
    GXStreamOutElement28* srcEnd = vec->end;
    if (src != srcEnd) {
        GXStreamOutElement28* dst = newBuf;
        for (; src != srcEnd; ++src, ++dst) {
            if (dst) {
                *dst = *src;
            }
        }
    }

    uint64_t currentCount = static_cast<uint64_t>(vec->end - vec->begin);
    if (vec->begin) {
        vec->allocator->vptr->Free(vec->allocator, vec->begin);
    }

    vec->begin = newBuf;
    vec->end = newBuf + currentCount;
    vec->cap = reinterpret_cast<GXStreamOutElement28*>(reinterpret_cast<uint8_t*>(newBuf) + allocBytes);
}

// [VMA: 0x2173eb0 - 0x2173eea] (58 bytes)
// Releases Geometry Shader with StreamOut CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmGeometryShaderWithStreamOut_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self, true);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// Forward declaration for 0x2173220
extern "C" int32_t GXGnmGeometryShaderProgram_InitializeFromShdrBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc);

struct GXStreamOutInputDecl {
    uint32_t semanticKey;  // +0x00 (low byte used as shift for stream mask)
    uint32_t mask;         // +0x04 (rbx-0x9 when rbx = decl+0xd)
    uint32_t pad_08;       // +0x08
    uint8_t  semanticIdx;  // +0x0c (rbx-0x1)
    uint8_t  componentCnt; // +0x0d (rbx+0x0)
    uint8_t  streamIndex;  // +0x0e (rbx+0x1)
    uint8_t  pad_0f;       // +0x0f
};

// [VMA: 0x2173620 - 0x2173ea4] (2180 bytes)
// Initializes a Geometry Shader with StreamOut from binary, validates sorted stream-out elements against shader reflection, and sets Gnm stream control registers.
extern "C" int32_t GXGnmGeometryShaderWithStreamOut_InitializeFromBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    const GXStreamOutInputDecl* soDecls,
    uint32_t soDeclCount,
    const uint32_t* streamStridesBytes,
    uint32_t /*numStrides*/,
    uint32_t rasterizedStream,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc)
{
    int32_t err = GXGnmGeometryShaderProgram_InitializeFromShdrBinary(
        self, ctx, shdrData, shdrSize, cpuAlloc, gpuAlloc);
    if (err != 0) {
        return err;
    }

    uint32_t streamMasks[4] = {0, 0, 0, 0};
    uint32_t streamOffsets[4] = {0, 0, 0, 0};

    uint8_t shaderInfo[0x98];
    std::memset(shaderInfo, 0, sizeof(shaderInfo));
    SceShaderBinaryInfo_Init(shaderInfo);

    int32_t parseErr = SceShaderBinaryInfo_Parse(shaderInfo, shdrData, static_cast<uint32_t>(shdrSize));
    uint32_t validationFailed = 1;

    if (parseErr == 0) {
        auto acquireTlsTempAlloc = []() -> GXAllocator* {
            uint8_t* tlsArena = *reinterpret_cast<uint8_t**>(gx_guest_tls_slot(0x53e4568ULL));
            if (!tlsArena) {
                reinterpret_cast<void (*)()>(GX_VMA(0x207d250ULL))();
                tlsArena = *reinterpret_cast<uint8_t**>(gx_guest_tls_slot(0x53e4568ULL));
            }
            GXAllocator* tlsAlloc = reinterpret_cast<GXAllocator*>(tlsArena + 0x28);
            uint8_t caps[16] = {};
            auto getCaps = reinterpret_cast<void (*)(void*, void*, uint32_t)>(tlsAlloc->vptr->vfunc_00[4]);
            getCaps(caps, tlsAlloc, 0u);
            if ((caps[0] & 0x20u) == 0u) {
                auto logAssert = reinterpret_cast<void (*)(uintptr_t, uint32_t, uintptr_t)>(GX_VMA(0x20b55b0ULL));
                logAssert(0u, 0x3Eu, GX_VMA(0x4737dc0ULL));
            }
            return tlsAlloc;
        };

        GXStreamOutVector userVec = {nullptr, nullptr, nullptr, nullptr, acquireTlsTempAlloc()};
        for (uint32_t i = 0; i < soDeclCount; ++i) {
            const GXStreamOutInputDecl& decl = soDecls[i];
            if (decl.mask != 0) {
                uint32_t sIdx = decl.streamIndex;
                GXStreamOutElement28 elem;
                elem.key = decl.semanticKey;
                elem.streamIdx = sIdx;
                elem.mask = decl.mask;
                elem.offset = streamOffsets[sIdx];
                elem.stride = streamStridesBytes[sIdx];
                elem.semantic = decl.semanticIdx;
                elem.component = decl.componentCnt;

                if (userVec.end == userVec.cap) {
                    GXStreamOutElementVector_GrowAndReallocate(&userVec);
                }
                if (userVec.end) {
                    *userVec.end = elem;
                }
                ++userVec.end;
            }

            uint32_t addBytes = static_cast<uint32_t>(decl.componentCnt) << 2;
            uint32_t sIdx = decl.streamIndex;
            streamOffsets[sIdx] += addBytes;
            uint8_t shift = static_cast<uint8_t>(decl.semanticKey & 0xFFu);
            streamMasks[sIdx] = (1u << shift);
        }

        int64_t userCount = userVec.end - userVec.begin;
        GXStreamOutElementArray_IntrosortBySemanticKey(userVec.begin, userVec.end, userCount);

        uint8_t reflElemCount = shaderInfo[0x2a]; // [rbp-0xae] where shaderInfo is at [rbp-0xd8]
        const uint8_t* reflArray = *reinterpret_cast<const uint8_t* const*>(shaderInfo + 0x68); // [rbp-0x70]

        GXStreamOutVector reflVec = {nullptr, nullptr, nullptr, nullptr, acquireTlsTempAlloc()};
        bool reflResolveOk = true;

        for (uint32_t i = 0; i < reflElemCount; ++i) {
            const uint8_t* rEntry = reflArray + i * 16;
            GXStreamOutElement28 rElem;
            rElem.key = rEntry[6];
            rElem.streamIdx = rEntry[5];
            rElem.mask = 0;
            rElem.offset = *reinterpret_cast<const uint16_t*>(rEntry + 2);
            rElem.stride = *reinterpret_cast<const uint16_t*>(rEntry + 0);
            rElem.semantic = 0;
            rElem.component = 0;

            if (!CGStreamOutGnm_ResolveSemanticAndComponent(&rElem.mask, rEntry) ||
                !CGStreamOutGnm_ResolveComponentMaskAndCount(&rElem.semantic, &rElem.component, rEntry[8]))
            {
                reflResolveOk = false;
                break;
            }

            if (reflVec.end == reflVec.cap) {
                GXStreamOutElementVector_GrowAndReallocate(&reflVec);
            }
            if (reflVec.end) {
                *reflVec.end = rElem;
            }
            ++reflVec.end;
        }

        if (reflResolveOk) {
            int64_t reflCount = reflVec.end - reflVec.begin;
            GXStreamOutElementArray_IntrosortBySemanticKey(reflVec.begin, reflVec.end, reflCount);

            if ((userVec.end - userVec.begin) == (reflVec.end - reflVec.begin)) {
                bool allMatch = true;
                int64_t count = userVec.end - userVec.begin;
                for (int64_t k = 0; k < count; ++k) {
                    const GXStreamOutElement28& u = userVec.begin[k];
                    const GXStreamOutElement28& r = reflVec.begin[k];
                    if (u.key != r.key || u.streamIdx != r.streamIdx || u.mask != r.mask ||
                        u.offset != r.offset || u.stride != r.stride || u.semantic != r.semantic ||
                        u.component != r.component)
                    {
                        allMatch = false;
                        break;
                    }
                }

                if (allMatch) {
                    validationFailed = 0;
                    self->streamOutEnabled = 1;
                    for (uint32_t i = 0; i < reflElemCount; ++i) {
                        if (userVec.begin[i].mask == 1u && userVec.begin[i].key == rasterizedStream) {
                            self->streamOutEnabled = 0;
                            break;
                        }
                    }
                }
            }
        }

        if (reflVec.begin) {
            reflVec.allocator->vptr->Free(reflVec.allocator, reflVec.begin);
        }
        if (userVec.begin) {
            userVec.allocator->vptr->Free(userVec.allocator, userVec.begin);
        }
    }

    SceShaderBinaryInfo_Destroy(shaderInfo);
    if (validationFailed != 0) {
        return 8;
    }

    uint8_t* streamMaskByte = reinterpret_cast<uint8_t*>(&self->streamOutMaskAndCfg);
    for (uint32_t s = 0; s < 4; ++s) {
        uint32_t mask = streamMasks[s];
        if ((mask & 0xFu) != 0) {
            *streamMaskByte |= static_cast<uint8_t>(1u << s);
            self->streamStridesDwords[s] = static_cast<uint16_t>(streamStridesBytes[s] >> 2);
            if (mask != 0) {
                SceGnmVsStreamOut_SetStreamControl(self->streamControl, s, mask);
            }
        }
    }

    return 0;
}

// [VMA: 0x2173220 - 0x217361b] (1019 bytes)
// Initializes a Geometry Shader program from a 'Shdr' binary (stage=3), uploading both GS and VS copy shader ucode and initializing V# constant buffer slots.
extern "C" int32_t GXGnmGeometryShaderProgram_InitializeFromShdrBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc)
{
    const uint8_t* binBytes = static_cast<const uint8_t*>(shdrData);
    if (*reinterpret_cast<const uint32_t*>(binBytes + 0x24) != 0x72646853u || binBytes[0x2c] != 3u) {
        return 8;
    }

    uint8_t shaderInfo[0x98];
    std::memset(shaderInfo, 0, sizeof(shaderInfo));
    SceShaderBinaryInfo_Init(shaderInfo);

    if (SceShaderBinaryInfo_Parse(shaderInfo, shdrData, static_cast<uint32_t>(shdrSize)) != 0) {
        SceShaderBinaryInfo_Destroy(shaderInfo);
        return 8;
    }

    struct StageBinaryPtrs {
        const uint8_t* header; // +0x00
        const void*    ucode;  // +0x08
        uint32_t       size;   // +0x10
        uint32_t       pad;    // +0x14
    };
    StageBinaryPtrs gsPtrs = {nullptr, nullptr, 0, 0};
    StageBinaryPtrs vsCopyPtrs = {nullptr, nullptr, 0, 0};
    SceGsShaderBinary_ExtractStagePointers(&gsPtrs, &vsCopyPtrs, shdrData);

    uint64_t gsAlignedSize = (static_cast<uint64_t>(gsPtrs.size) + 0xFFULL) & 0x1FFFFFF00ULL;
    uint64_t totalGpuSize = gsAlignedSize + static_cast<uint64_t>(vsCopyPtrs.size);

    uint8_t* gpuBase = static_cast<uint8_t*>(gpuAlloc->vptr->AllocAligned(gpuAlloc, totalGpuSize, 0x100u));
    if (!gpuBase) {
        SceShaderBinaryInfo_Destroy(shaderInfo);
        return 9;
    }

    self->shaderUcodeGpu = gpuBase;
    uint8_t* vsCopyGpuPtr = gpuBase + gsAlignedSize;
    if (CGMemoryGnm_IsCpuAccessibleAddress(gpuBase)) {
        if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectBuffers) && totalGpuSize > 0u) {
            gx_direct_vk::PrepareDirectBufferWrite(gpuBase, totalGpuSize);
            gx_direct_vk::NoteDirectAssetBufferWrite(gpuBase, totalGpuSize);
        }
        CGMemoryGnm_Memcpy(gpuBase, gsPtrs.ucode, gsPtrs.size);
        CGMemoryGnm_Memcpy(vsCopyGpuPtr, vsCopyPtrs.ucode, vsCopyPtrs.size);
        if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectBuffers) && totalGpuSize > 0u) {
            gx_direct_vk::NoteDirectAssetBufferWrite(gpuBase, totalGpuSize);
        }
        gx_direct_vk::RegisterDirectShaderBinary(
            gpuBase, static_cast<uint32_t>(gsAlignedSize), shdrData, static_cast<uint32_t>(shdrSize), 3u);
        gx_direct_vk::RegisterDirectShaderBinary(
            vsCopyGpuPtr, vsCopyPtrs.size, shdrData, static_cast<uint32_t>(shdrSize), 1u);
    }

    const uint8_t* gsHdr = gsPtrs.header;
    uint32_t attrCount = *reinterpret_cast<const uint32_t*>(gsHdr) >> 24;
    uint32_t validAttrCount = 0;

    for (uint32_t i = 0; i < attrCount; ++i) {
        uint8_t attrType = gsHdr[0x38 + i * 4];
        if (attrType > 0x1Bu) {
            SceShaderBinaryInfo_Destroy(shaderInfo);
            return 8;
        }
        if ((0x0F680097u & (1u << attrType)) != 0) {
            ++validAttrCount;
        } else if ((0x00840108u & (1u << attrType)) == 0) {
            SceShaderBinaryInfo_Destroy(shaderInfo);
            return 8;
        }
    }

    const uint8_t* gsHdrPlus38 = gsHdr + 0x38;
    uint32_t regCount = gsHdrPlus38[attrCount * 4 + 0x03];
    uint32_t immCount = gsHdrPlus38[attrCount * 4 + 0x24];
    uint32_t samplerCount = gsHdrPlus38[attrCount * 4 + 0x25];
    uint32_t copyHdrOffset = (((regCount + immCount) << 2) + (samplerCount * 2) + 0x2Bu) & ~3u;
    uint32_t copySectionAligned = (copyHdrOffset + attrCount * 4 + 0x3Fu) & ~7u;
    uint64_t totalCpuHeaderBytes = static_cast<uint64_t>(copySectionAligned) + static_cast<uint64_t>(validAttrCount) * 12ULL;

    uint8_t* cpuHdr = static_cast<uint8_t*>(cpuAlloc->vptr->AllocCpu(cpuAlloc, totalCpuHeaderBytes));
    self->shaderHeaderCpu = cpuHdr;
    if (!cpuHdr) {
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        SceShaderBinaryInfo_Destroy(shaderInfo);
        return 9;
    }

    CGMemoryGnm_Memcpy(cpuHdr, gsHdr, copyHdrOffset + attrCount * 4 + 0x38u);
    int32_t setupErr = GXGnmShaderStageInit_SetupGsAndCopyShader(
        self,
        cpuHdr + copySectionAligned,
        validAttrCount,
        3u,
        shaderInfo,
        gsHdrPlus38,
        attrCount,
        cpuAlloc);

    if (setupErr != 0) {
        cpuAlloc->vptr->Free(cpuAlloc, self->shaderHeaderCpu);
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        SceShaderBinaryInfo_Destroy(shaderInfo);
        return setupErr;
    }

    uintptr_t gsGpuAddr = reinterpret_cast<uintptr_t>(gpuBase);
    *reinterpret_cast<uint32_t*>(cpuHdr + 0x08) = static_cast<uint32_t>(gsGpuAddr >> 8);
    *reinterpret_cast<uint32_t*>(cpuHdr + 0x0c) = static_cast<uint32_t>(gsGpuAddr >> 40);

    uint32_t gsDwordCount = cpuHdr[0x03];
    uintptr_t vsCopyGpuAddr = reinterpret_cast<uintptr_t>(vsCopyGpuPtr);
    *reinterpret_cast<uint32_t*>(cpuHdr + gsDwordCount * 4 + 0x40) = static_cast<uint32_t>(vsCopyGpuAddr >> 8);
    *reinterpret_cast<uint32_t*>(cpuHdr + gsDwordCount * 4 + 0x44) = static_cast<uint32_t>(vsCopyGpuAddr >> 40);

    self->hasConstantBufferSlot = CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
        self->constantBufferVSharp,
        cpuHdr,
        *reinterpret_cast<const uint32_t*>(cpuHdr + 0x0c),
        *reinterpret_cast<const uint32_t*>(cpuHdr + 0x08));
    self->cpuAllocator = cpuAlloc;
    self->gpuAllocator = gpuAlloc;

    uint32_t maxVerts = *reinterpret_cast<const uint32_t*>(cpuHdr + 0x34) & 0x7FFu;
    if (maxVerts <= 0x80u) {
        self->gsMaxVertOutMode = 3u;
    } else if (maxVerts <= 0x100u) {
        self->gsMaxVertOutMode = 2u;
    } else if (maxVerts <= 0x200u) {
        self->gsMaxVertOutMode = 1u;
    } else {
        self->gsMaxVertOutMode = 0u;
    }

    uint32_t copyBaseIdx = cpuHdr[0x03];
    const uint8_t* copyShaderHdr = cpuHdr + copyBaseIdx * 4 + 0x38;
    self->streamOutEnabled = 0;
    self->streamControl[0] = 0;
    *reinterpret_cast<uint64_t*>(self->streamStridesDwords) = 0;
    self->streamOutMaskAndCfg = 0;

    uint32_t copySlotCount = cpuHdr[copyBaseIdx * 4 + 0x3b];
    const uint8_t* copySlotPtr = cpuHdr + copyBaseIdx * 4 + 0x60;
    for (uint32_t idx = 0; idx < copySlotCount; ++idx, copySlotPtr += 4) {
        if (copySlotPtr[0] == 0x18u) {
            uint32_t cfgVal = (static_cast<uint32_t>(copySlotPtr[2]) << 8) | 0x0Cu;
            *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(&self->streamOutMaskAndCfg) + 4) = cfgVal;
            break;
        }
    }

    self->hasCopyShaderCbSlot = CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
        self->copyShaderCbVSharp,
        copyShaderHdr,
        *reinterpret_cast<const uint32_t*>(cpuHdr + 0x38 + copyBaseIdx * 4 + 0x0c),
        *reinterpret_cast<const uint32_t*>(cpuHdr + 0x38 + copyBaseIdx * 4 + 0x08));

    SceShaderBinaryInfo_Destroy(shaderInfo);
    return 0;
}

// [VMA: 0x2173150 - 0x217318a] (58 bytes)
// Releases Geometry Shader CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmGeometryShaderProgram_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self, true);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172fb0 - 0x217314f] (415 bytes)
// Initializes a Vertex Shader (stage=1) or Pixel Shader (stage=5) program from a 'Shdr' binary.
extern "C" int32_t GXGnmVertexOrPixelShaderProgram_InitializeFromShdrBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc)
{
    const uint8_t* binBytes = static_cast<const uint8_t*>(shdrData);
    if (*reinterpret_cast<const uint32_t*>(binBytes + 0x24) != 0x72646853u) {
        return 8;
    }

    uint32_t stageType = binBytes[0x2c];
    uint64_t gpuSize = 0;
    int32_t err = CGShaderBinaryGnm_AllocateAndUploadUcode(
        &self->shaderUcodeGpu, &gpuSize, ctx, shdrData, gpuAlloc, stageType);
    if (err != 0) {
        return err;
    }

    if (stageType == 5u) {
        err = CGShaderFactoryGnm_CreatePixelShader(
            &self->shaderHeaderCpu,
            self,
            gpuSize,
            self->shaderUcodeGpu,
            shdrData,
            shdrSize,
            stageType,
            cpuAlloc,
            0,
            0);
    } else if (stageType == 1u) {
        err = CGShaderFactoryGnm_CreateVertexShader(
            &self->shaderHeaderCpu,
            self,
            gpuSize,
            self->shaderUcodeGpu,
            shdrData,
            shdrSize,
            stageType,
            cpuAlloc,
            0,
            0);
    } else {
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        return 8;
    }

    const uint32_t* headerWords = static_cast<const uint32_t*>(self->shaderHeaderCpu);
    self->hasConstantBufferSlot = CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
        self->constantBufferVSharp,
        self->shaderHeaderCpu,
        headerWords[3],
        headerWords[2]);

    if (err != 0) {
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        return err;
    }

    self->cpuAllocator = cpuAlloc;
    self->gpuAllocator = gpuAlloc;
    return 0;
}

// [VMA: 0x2172ee0 - 0x2172f1a] (58 bytes)
// Releases Vertex/Pixel Shader CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmVertexOrPixelShaderProgram_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172db0 - 0x2172edb] (299 bytes)
// Uploads Export/Compute Shader ucode (stage=4), creates shader header via CGShaderFactoryGnm_CreateExportShader, and initializes V# slot.
extern "C" int32_t GXGnmExportShaderProgram_InitializeFromBinary(
    GXShaderProgramBase* self,
    void* ctx,
    const void* shdrData,
    uint64_t shdrSize,
    GXAllocator* cpuAlloc,
    GXAllocator* gpuAlloc)
{
    uint64_t gpuSize = 0;
    int32_t err = CGShaderBinaryGnm_AllocateAndUploadUcode(
        &self->shaderUcodeGpu, &gpuSize, ctx, shdrData, gpuAlloc, 4u);
    if (err != 0) {
        return err;
    }

    err = CGShaderFactoryGnm_CreateExportShader(
        &self->shaderHeaderCpu,
        self,
        gpuSize,
        self->shaderUcodeGpu,
        shdrData,
        shdrSize,
        4u,
        cpuAlloc,
        0,
        0);
    if (err != 0) {
        gpuAlloc->vptr->Free(gpuAlloc, self->shaderUcodeGpu);
        return err;
    }

    const uint32_t* headerWords = static_cast<const uint32_t*>(self->shaderHeaderCpu);
    self->hasConstantBufferSlot = CGConstantBufferGnm_InitBufferVSharpFromShaderSlot(
        self->constantBufferVSharp,
        self->shaderHeaderCpu,
        headerWords[3],
        headerWords[2]);
    self->cpuAllocator = cpuAlloc;
    self->gpuAllocator = gpuAlloc;
    return 0;
}

// [VMA: 0x2172ce0 - 0x2172d1a] (58 bytes)
// Releases Export Shader CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmExportShaderProgram_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172cd0 - 0x2172cdf] (16 bytes)
// Copy-constructs a 0x128-byte shader stage object.
extern "C" void GXGnmShaderProgram_ZeroInit128(void* self, const void* src) {
    CGMemoryGnm_MemsetZero(self, src, 0x128u);
}

// [VMA: 0x2172c00 - 0x2172c3a] (58 bytes)
// Releases Shader Stage variant A CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmShaderStageVariantA_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172bf0 - 0x2172bff] (16 bytes)
// Copy-constructs a 0x120-byte shader stage variant A object.
extern "C" void GXGnmShaderStageVariantA_ZeroInit120(void* self, const void* src) {
    CGMemoryGnm_MemsetZero(self, src, 0x120u);
}

// [VMA: 0x2172b20 - 0x2172b5a] (58 bytes)
// Releases Shader Stage variant B CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmShaderStageVariantB_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172b10 - 0x2172b1f] (16 bytes)
// Copy-constructs a 0x120-byte shader stage variant B object.
extern "C" void GXGnmShaderStageVariantB_ZeroInit120(void* self, const void* src) {
    CGMemoryGnm_MemsetZero(self, src, 0x120u);
}

// [VMA: 0x2172a40 - 0x2172a7a] (58 bytes)
// Releases Geometry Shader Stage variant C CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmGeometryShaderStageVariantC_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self, true);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172a30 - 0x2172a3f] (16 bytes)
// Copy-constructs a 0x150-byte geometry shader stage object.
extern "C" void GXGnmGeometryShaderStageVariantC_ZeroInit150(void* self, const void* src) {
    CGMemoryGnm_MemsetZero(self, src, 0x150u);
}

// [VMA: 0x2172960 - 0x217299a] (58 bytes)
// Releases Shader Stage variant D CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmShaderStageVariantD_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172950 - 0x217295f] (16 bytes)
// Copy-constructs a 0x120-byte shader stage variant D object.
extern "C" void GXGnmShaderStageVariantD_ZeroInit120(void* self, const void* src) {
    CGMemoryGnm_MemsetZero(self, src, 0x120u);
}

// [VMA: 0x2172880 - 0x21728ba] (58 bytes)
// Releases Shader Stage variant E CPU header (+0x100) and GPU ucode (+0x108).
extern "C" void GXGnmShaderStageVariantE_ReleaseBinaries(GXShaderProgramBase* self) {
    RetireDirectShaderProgramUcode(self);
    self->cpuAllocator->vptr->Free(self->cpuAllocator, self->shaderHeaderCpu);
    self->gpuAllocator->vptr->Free(self->gpuAllocator, self->shaderUcodeGpu);
}

// [VMA: 0x2172870 - 0x217287f] (16 bytes)
// Copy-constructs a 0x120-byte shader stage variant E object.
extern "C" void GXGnmShaderStageVariantE_ZeroInit120(void* self, const void* src) {
    CGMemoryGnm_MemsetZero(self, src, 0x120u);
}

struct GXGnmTextureHardwareObject {
    GXAllocator* allocator;    // +0x00
    void*        gpuBaseAddr;  // +0x08
    uint32_t     resourceId;   // +0x10
    uint32_t     rawByteSize;  // +0x14
    uint32_t     allocSize;    // +0x18
    uint32_t     alignment;    // +0x1c
    uint8_t      dimType;      // +0x20 (2=1D, 3=2D, 4=3D)
    uint8_t      usageMode;    // +0x21
    uint8_t      cpuAccess;    // +0x22
    uint8_t      bindFlags;    // +0x23
    uint32_t     miscFlags;    // +0x24
    uint32_t     tileMode;     // +0x28
    uint16_t     width;        // +0x2c
    uint16_t     height;       // +0x2e
    uint16_t     depthOrArray; // +0x30
    uint8_t      mipLevels;    // +0x32
    uint8_t      format;       // +0x33
    uint16_t     sampleCount;  // +0x34
    uint16_t     sampleQuality;// +0x36
};

struct GXTexture3DDesc {
    uint32_t width;        // +0x00
    uint32_t height;       // +0x04
    uint32_t depth;        // +0x08
    uint32_t mipLevels;    // +0x0c
    uint32_t format;       // +0x10
    uint32_t usageMode;    // +0x14
    uint32_t bindFlags;    // +0x18
    uint32_t cpuAccess;    // +0x1c
    uint32_t miscFlags;    // +0x20
};

struct GXTexture2DDesc {
    uint32_t width;        // +0x00
    uint32_t height;       // +0x04
    uint32_t mipLevels;    // +0x08
    uint32_t arraySize;    // +0x0c
    uint32_t format;       // +0x10
    uint32_t sampleCount;  // +0x14
    uint32_t sampleQuality;// +0x18
    uint32_t usageMode;    // +0x1c
    uint32_t bindFlags;    // +0x20
    uint32_t cpuAccess;    // +0x24
    uint32_t miscFlags;    // +0x28
};

struct GXTexture1DDesc {
    uint32_t width;        // +0x00
    uint32_t mipLevels;    // +0x04
    uint32_t arraySize;    // +0x08
    uint32_t format;       // +0x0c
    uint32_t usageMode;    // +0x10
    uint32_t bindFlags;    // +0x14
    uint32_t cpuAccess;    // +0x18
    uint32_t miscFlags;    // +0x1c
};

struct GXTextureCustomAllocOptions {
    uint8_t  useCustomSizeAlign; // +0x00
    uint8_t  copyRawLinear;      // +0x01
    uint8_t  hasCustomTileInfo;  // +0x02
    uint8_t  skipGpuDmaUpload;   // +0x03
    uint32_t customSize;         // +0x04
    uint32_t customAlignment;    // +0x08
    uint8_t  customTileData[16]; // +0x0c
};

// [VMA: 0x21727a0 - 0x21727dd] (62 bytes)
// Extracts a 36-byte GXTexture3DDesc from a GXGnmTextureHardwareObject.
extern "C" void GXGnmTexture3D_ExtractDescriptorFromHardware(
    const GXGnmTextureHardwareObject* hwTex,
    GXTexture3DDesc* outDesc)
{
    outDesc->width     = hwTex->width;
    outDesc->height    = hwTex->height;
    outDesc->depth     = hwTex->depthOrArray;
    outDesc->mipLevels = hwTex->mipLevels;
    outDesc->format    = hwTex->format;
    outDesc->usageMode = hwTex->usageMode;
    outDesc->bindFlags = hwTex->bindFlags;
    outDesc->cpuAccess = hwTex->cpuAccess;
    outDesc->miscFlags = hwTex->miscFlags;
}

// [VMA: 0x2172770 - 0x217279e] (47 bytes)
// Releases the GPU memory allocation of a 3D texture hardware object and clears its pointer.
extern "C" void GXGnmTexture3D_ReleaseMemory(GXGnmTextureHardwareObject* hwTex) {
    GXAllocator* alloc = hwTex->allocator;
    if (alloc && hwTex->gpuBaseAddr) {
        gx_direct_vk::UnwatchDirectTextureRange(hwTex->gpuBaseAddr, hwTex->allocSize);
        alloc->vptr->Free(alloc, hwTex->gpuBaseAddr);
        hwTex->gpuBaseAddr = nullptr;
    }
}

extern "C" __attribute__((weak)) void BbGoldenEngine_NotifyGpuSurfaceWrite(void* dst, uint64_t size) {
    (void)dst;
    (void)size;
}

// [VMA: 0x21723a0 - 0x217276b] (972 bytes)
// Allocates and initializes a 3D Gnm Texture, optionally uploading initial mip subresources via DMA or CPU surface tiling.
extern "C" int32_t GXGnmTexture3D_CreateAndInitialize(
    GXGnmTextureHardwareObject* hwTex,
    uint8_t* deviceCtx,
    const GXTexture3DDesc* desc,
    const void* const* initialSubresources,
    GXAllocator* preferredAlloc,
    GXAllocator* fallbackAlloc,
    const GXTextureCustomAllocOptions* opts)
{
    GXAllocator* activeAlloc = ((desc->miscFlags & 0x4000u) != 0) ? preferredAlloc : fallbackAlloc;
    uint32_t sizeAlign[2] = {0, 0};
    const void* customTilePtr = (opts && opts->hasCustomTileInfo != 0) ? opts->customTileData : nullptr;

    uint64_t gnmTex[4] = {0, 0, 0, 0};
    int32_t err = SceGnmTexture3D_InitAndComputeSize(
        reinterpret_cast<uint64_t*>(sizeAlign),
        gnmTex,
        desc,
        customTilePtr);
    if (err != 0) {
        return err;
    }

    uint32_t rawSize = sizeAlign[0];
    uint32_t alignment = sizeAlign[1];
    uint32_t allocSize = rawSize;

    if (opts && opts->useCustomSizeAlign != 0) {
        if (opts->customSize < rawSize || opts->customAlignment < alignment) {
            return 8;
        }
        allocSize = opts->customSize;
        alignment = opts->customAlignment;
    }

    uint32_t hasCounterTail = desc->bindFlags & 0x80u;
    if (hasCounterTail != 0) {
        allocSize = (allocSize + 0x17u) & ~7u;
        if (alignment <= 8u) {
            alignment = 8u;
        }
    }

    uint32_t numMips = SceGnmTexture_GetLastMipLevel(gnmTex);
    uint8_t* gpuMem = static_cast<uint8_t*>(activeAlloc->vptr->AllocAligned(activeAlloc, allocSize, alignment));
    if (!gpuMem) {
        return 9;
    }

    ++numMips;
    if (hasCounterTail != 0) {
        *reinterpret_cast<uint64_t*>(gpuMem + allocSize - 8) = 0;
        *reinterpret_cast<uint64_t*>(gpuMem + allocSize - 16) = 0;
    }

    hwTex->allocator    = activeAlloc;
    hwTex->gpuBaseAddr  = gpuMem;
    hwTex->resourceId   = CGResourceIdGnm_AllocateNextHandle();
    hwTex->rawByteSize  = rawSize;
    hwTex->allocSize    = allocSize;
    hwTex->alignment    = alignment;
    hwTex->dimType      = 4u;
    hwTex->usageMode    = static_cast<uint8_t>(desc->usageMode);
    hwTex->cpuAccess    = static_cast<uint8_t>(desc->cpuAccess);
    hwTex->bindFlags    = static_cast<uint8_t>(desc->bindFlags);
    hwTex->miscFlags    = desc->miscFlags;
    hwTex->tileMode     = SceGnmTexture_GetTileMode(gnmTex);
    hwTex->width        = static_cast<uint16_t>(desc->width);
    hwTex->height       = static_cast<uint16_t>(desc->height);
    hwTex->depthOrArray = static_cast<uint16_t>(desc->depth);
    hwTex->mipLevels    = static_cast<uint8_t>(numMips);
    hwTex->format       = static_cast<uint8_t>(desc->format);

    bool skipGpuDma = (opts && opts->skipGpuDmaUpload != 0);
    if (!initialSubresources || !CGFormatGnm_IsValidDataFormat(desc->format)) {
        return 0;
    }

    if (opts && opts->copyRawLinear != 0) {
        gx_direct_vk::PrepareDirectTextureWrite(gpuMem, allocSize);
        CGMemoryGnm_Memcpy(gpuMem, initialSubresources[0], rawSize);
        if ((desc->bindFlags & 0x60u) == 0u) {
            gx_direct_vk::SetTSharpBaseAddress(gnmTex, gpuMem);
            const uint32_t isImmutable =
                (desc->usageMode == 1u && desc->bindFlags == 8u && desc->cpuAccess == 0u) ? 1u : 0u;
            gx_direct_vk::RegisterDirectAssetTexture(
                hwTex->resourceId, gnmTex, rawSize, allocSize, isImmutable);
        }
        return 0;
    }

    if (!skipGpuDma && CGMemoryGnm_IsGarlicAddress(hwTex->gpuBaseAddr)) {
        void* lockObj = *reinterpret_cast<void**>(deviceCtx + 0x118);
        (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(lockObj) + 0x18))(lockObj, 0xFFFFFFFFu);

        uint8_t* cmdCtxPlus40 = deviceCtx + 0x40;
        const uint8_t* subDescPtr = reinterpret_cast<const uint8_t*>(initialSubresources);
        for (uint32_t mip = 0; mip < numMips; ++mip, subDescPtr += 16) {
            CGCommandContextGnm_DmaUploadTextureSubresource(cmdCtxPlus40, hwTex, subDescPtr, mip, 0u);
        }
        CGCommandContextGnm_FlushAndWaitIdle(cmdCtxPlus40);
        CGCommandContextGnm_SubmitAndReset(cmdCtxPlus40);

        lockObj = *reinterpret_cast<void**>(deviceCtx + 0x118);
        (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(lockObj) + 0x28))(lockObj);
        return 0;
    }

    bool isCpuAccessible = (CGMemoryGnm_IsCpuAccessibleAddress(hwTex->gpuBaseAddr) != 0);
    if (numMips == 0 || !isCpuAccessible) {
        return 0;
    }

    gx_direct_vk::PrepareDirectTextureWrite(gpuMem, allocSize);
    uint8_t tilingParams[0x30];
    const uint8_t* subDescPtr = reinterpret_cast<const uint8_t*>(initialSubresources);
    for (uint32_t mip = 0; mip < numMips; ++mip, subDescPtr += 16) {
        uint64_t mipOffset = 0;
        uint64_t mipSize = 0;
        int32_t addrErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, gnmTex, mip, 0u);
        if (addrErr != 0) {
            CGErrorGnm_ReportGpuAddrError(addrErr);
            return 8;
        }
        SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mip, 0u);
        const void* srcLinear = *reinterpret_cast<const void* const*>(subDescPtr);
        BbGoldenEngine_NotifyGpuSurfaceWrite(gpuMem + mipOffset, mipSize);
        if (SceGpuAddress_TileSurface(gpuMem + mipOffset, srcLinear, tilingParams) != 0) {
            activeAlloc->vptr->Free(activeAlloc, gpuMem);
            return 8;
        }
        BbGoldenEngine_NotifyGpuSurfaceWrite(gpuMem + mipOffset, mipSize);
    }
    if ((desc->bindFlags & 0x60u) == 0u) {
        gx_direct_vk::SetTSharpBaseAddress(gnmTex, gpuMem);
        const uint32_t isImmutable =
            (desc->usageMode == 1u && desc->bindFlags == 8u && desc->cpuAccess == 0u) ? 1u : 0u;
        gx_direct_vk::RegisterDirectAssetTexture(
            hwTex->resourceId, gnmTex, rawSize, allocSize, isImmutable);
    }
    return 0;
}

// [VMA: 0x21722c0 - 0x217230b] (76 bytes)
// Extracts a 44-byte GXTexture2DDesc from a GXGnmTextureHardwareObject.
extern "C" void GXGnmTexture2D_ExtractDescriptorFromHardware(
    const GXGnmTextureHardwareObject* hwTex,
    GXTexture2DDesc* outDesc)
{
    outDesc->width         = hwTex->width;
    outDesc->height        = hwTex->height;
    outDesc->mipLevels     = hwTex->mipLevels;
    outDesc->arraySize     = hwTex->depthOrArray;
    outDesc->format        = hwTex->format;
    outDesc->sampleCount   = hwTex->sampleCount;
    outDesc->sampleQuality = hwTex->sampleQuality;
    outDesc->usageMode     = hwTex->usageMode;
    outDesc->bindFlags     = hwTex->bindFlags;
    outDesc->cpuAccess     = hwTex->cpuAccess;
    outDesc->miscFlags     = hwTex->miscFlags;
}

// [VMA: 0x2172290 - 0x21722be] (47 bytes)
// Releases the GPU memory allocation of a 2D texture hardware object and clears its pointer.
extern "C" void GXGnmTexture2D_ReleaseMemory(GXGnmTextureHardwareObject* hwTex) {
    GXAllocator* alloc = hwTex->allocator;
    if (alloc && hwTex->gpuBaseAddr) {
        gx_direct_vk::UnwatchDirectTextureRange(hwTex->gpuBaseAddr, hwTex->allocSize);
        alloc->vptr->Free(alloc, hwTex->gpuBaseAddr);
        hwTex->gpuBaseAddr = nullptr;
    }
}

// [VMA: 0x2171d60 - 0x217228c] (1325 bytes)
// Allocates and initializes a 2D / 2DArray / Cubemap Gnm Texture, uploading initial mip*slice subresources via DMA or CPU tiling.
extern "C" int32_t GXGnmTexture2D_CreateAndInitialize(
    GXGnmTextureHardwareObject* hwTex,
    uint8_t* deviceCtx,
    const GXTexture2DDesc* desc,
    const void* const* initialSubresources,
    GXAllocator* preferredAlloc,
    GXAllocator* fallbackAlloc,
    const GXTextureCustomAllocOptions* opts)
{
    if (desc->sampleCount == 16u && (desc->miscFlags & 0x400u) == 0) {
        return 8;
    }

    GXAllocator* activeAlloc = ((desc->miscFlags & 0x4000u) != 0) ? preferredAlloc : fallbackAlloc;
    uint32_t sizeAlign[2] = {0, 0};
    const void* customTilePtr = (opts && opts->hasCustomTileInfo != 0) ? opts->customTileData : nullptr;

    uint64_t gnmTex[4] = {0, 0, 0, 0};
    int32_t err = SceGnmTexture2D_InitAndComputeSize(
        reinterpret_cast<uint64_t*>(sizeAlign),
        gnmTex,
        desc,
        customTilePtr);
    if (err != 0) {
        return err;
    }

    uint32_t rawSize = sizeAlign[0];
    uint32_t alignment = sizeAlign[1];
    uint32_t allocSize = rawSize;

    if (opts && opts->useCustomSizeAlign != 0) {
        if (opts->customSize < rawSize || opts->customAlignment < alignment) {
            return 8;
        }
        allocSize = opts->customSize;
        alignment = opts->customAlignment;
    }

    uint32_t hasCounterTail = desc->bindFlags & 0x80u;
    if (hasCounterTail != 0) {
        allocSize = (allocSize + 0x17u) & ~7u;
        if (alignment <= 8u) {
            alignment = 8u;
        }
    }

    uint32_t lastMip = SceGnmTexture_GetLastMipLevel(gnmTex);
    uint8_t* gpuMem = static_cast<uint8_t*>(activeAlloc->vptr->AllocAligned(activeAlloc, allocSize, alignment));
    if (!gpuMem) {
        return 9;
    }

    uint32_t numMips = lastMip + 1;
    if (hasCounterTail != 0) {
        *reinterpret_cast<uint64_t*>(gpuMem + allocSize - 8) = 0;
        *reinterpret_cast<uint64_t*>(gpuMem + allocSize - 16) = 0;
    }

    hwTex->allocator     = activeAlloc;
    hwTex->gpuBaseAddr   = gpuMem;
    hwTex->resourceId    = CGResourceIdGnm_AllocateNextHandle();
    hwTex->rawByteSize   = rawSize;
    hwTex->allocSize     = allocSize;
    hwTex->alignment     = alignment;
    hwTex->dimType       = 3u;
    hwTex->usageMode     = static_cast<uint8_t>(desc->usageMode);
    hwTex->cpuAccess     = static_cast<uint8_t>(desc->cpuAccess);
    hwTex->bindFlags     = static_cast<uint8_t>(desc->bindFlags);
    hwTex->miscFlags     = desc->miscFlags;
    hwTex->tileMode      = SceGnmTexture_GetTileMode(gnmTex);
    hwTex->width         = static_cast<uint16_t>(desc->width);
    hwTex->height        = static_cast<uint16_t>(desc->height);
    hwTex->depthOrArray  = static_cast<uint16_t>(desc->arraySize);
    hwTex->mipLevels     = static_cast<uint8_t>(numMips);
    hwTex->format        = static_cast<uint8_t>(desc->format);
    hwTex->sampleCount   = static_cast<uint16_t>(desc->sampleCount);
    hwTex->sampleQuality = static_cast<uint16_t>(desc->sampleQuality);

    bool skipGpuDma = (opts && opts->skipGpuDmaUpload != 0);
    if (!initialSubresources || !CGFormatGnm_IsValidDataFormat(desc->format)) {
        return 0;
    }

    if (opts && opts->copyRawLinear != 0) {
        gx_direct_vk::PrepareDirectTextureWrite(gpuMem, allocSize);
        CGMemoryGnm_Memcpy(gpuMem, initialSubresources[0], rawSize);
        if ((desc->bindFlags & 0x60u) == 0u && desc->sampleCount <= 1u) {
            gx_direct_vk::SetTSharpBaseAddress(gnmTex, gpuMem);
            const uint32_t isImmutable =
                (desc->usageMode == 1u && desc->bindFlags == 8u && desc->cpuAccess == 0u) ? 1u : 0u;
            gx_direct_vk::RegisterDirectAssetTexture(
                hwTex->resourceId, gnmTex, rawSize, allocSize, isImmutable);
        }
        return 0;
    }

    if (!skipGpuDma && CGMemoryGnm_IsGarlicAddress(hwTex->gpuBaseAddr)) {
        void* lockObj = *reinterpret_cast<void**>(deviceCtx + 0x118);
        (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(lockObj) + 0x18))(lockObj, 0xFFFFFFFFu);

        uint8_t* cmdCtxPlus40 = deviceCtx + 0x40;
        if (numMips != 0) {
            uint32_t mipStride = lastMip + 1;
            const uint8_t* subBase = reinterpret_cast<const uint8_t*>(initialSubresources);
            for (uint32_t mip = 0; mip < numMips; ++mip) {
                uint32_t subIdx = mip;
                uint32_t arrayCount = desc->arraySize;
                for (uint32_t slice = 0; slice < arrayCount; ++slice, subIdx += mipStride) {
                    CGCommandContextGnm_DmaUploadTextureSubresource(
                        cmdCtxPlus40, hwTex, subBase + static_cast<uint64_t>(subIdx) * 16ULL, subIdx, 0u);
                    arrayCount = desc->arraySize;
                }
            }
        }

        CGCommandContextGnm_FlushAndWaitIdle(cmdCtxPlus40);
        CGCommandContextGnm_SubmitAndReset(cmdCtxPlus40);

        lockObj = *reinterpret_cast<void**>(deviceCtx + 0x118);
        (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(lockObj) + 0x28))(lockObj);
        return 0;
    }

    bool isCpuAccessible = (CGMemoryGnm_IsCpuAccessibleAddress(hwTex->gpuBaseAddr) != 0);
    if (numMips == 0 || !isCpuAccessible) {
        return 0;
    }

    gx_direct_vk::PrepareDirectTextureWrite(gpuMem, allocSize);
    uint8_t tilingParams[0x30];
    uint32_t mipStride = lastMip + 1;
    const uint8_t* subBase = reinterpret_cast<const uint8_t*>(initialSubresources);
    for (uint32_t mip = 0; mip < numMips; ++mip) {
        uint32_t subIdx = mip;
        uint32_t arrayCount = desc->arraySize;
        for (uint32_t slice = 0; slice < arrayCount; ++slice, subIdx += mipStride) {
            uint64_t surfOffset = 0;
            uint64_t surfSize = 0;
            int32_t addrErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&surfOffset, &surfSize, gnmTex, mip, slice);
            if (addrErr != 0) {
                CGErrorGnm_ReportGpuAddrError(addrErr);
                return 8;
            }
            SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mip, slice);
            const void* srcLinear = *reinterpret_cast<const void* const*>(subBase + static_cast<uint64_t>(subIdx) * 16ULL);
            BbGoldenEngine_NotifyGpuSurfaceWrite(gpuMem + surfOffset, surfSize);
            if (SceGpuAddress_TileSurface(gpuMem + surfOffset, srcLinear, tilingParams) != 0) {
                activeAlloc->vptr->Free(activeAlloc, gpuMem);
                return 8;
            }
            BbGoldenEngine_NotifyGpuSurfaceWrite(gpuMem + surfOffset, surfSize);
            arrayCount = desc->arraySize;
        }
    }
    if ((desc->bindFlags & 0x60u) == 0u && desc->sampleCount <= 1u) {
        gx_direct_vk::SetTSharpBaseAddress(gnmTex, gpuMem);
        const uint32_t isImmutable =
            (desc->usageMode == 1u && desc->bindFlags == 8u && desc->cpuAccess == 0u) ? 1u : 0u;
        gx_direct_vk::RegisterDirectAssetTexture(
            hwTex->resourceId, gnmTex, rawSize, allocSize, isImmutable);
    }
    return 0;
}

// [VMA: 0x2171c90 - 0x2171cc6] (55 bytes)
// Extracts a 32-byte GXTexture1DDesc from a GXGnmTextureHardwareObject.
extern "C" void GXGnmTexture1D_ExtractDescriptorFromHardware(
    const GXGnmTextureHardwareObject* hwTex,
    GXTexture1DDesc* outDesc)
{
    outDesc->width     = hwTex->width;
    outDesc->mipLevels = hwTex->mipLevels;
    outDesc->arraySize = hwTex->depthOrArray;
    outDesc->format    = hwTex->format;
    outDesc->usageMode = hwTex->usageMode;
    outDesc->bindFlags = hwTex->bindFlags;
    outDesc->cpuAccess = hwTex->cpuAccess;
    outDesc->miscFlags = hwTex->miscFlags;
}

// [VMA: 0x2171c60 - 0x2171c8e] (47 bytes)
// Releases the GPU memory allocation of a 1D texture hardware object and clears its pointer.
extern "C" void GXGnmTexture1D_ReleaseMemory(GXGnmTextureHardwareObject* hwTex) {
    GXAllocator* alloc = hwTex->allocator;
    if (alloc && hwTex->gpuBaseAddr) {
        gx_direct_vk::UnwatchDirectTextureRange(hwTex->gpuBaseAddr, hwTex->allocSize);
        alloc->vptr->Free(alloc, hwTex->gpuBaseAddr);
        hwTex->gpuBaseAddr = nullptr;
    }
}

// [VMA: 0x2171790 - 0x2171c54] (1221 bytes)
// Allocates and initializes a 1D / 1DArray Gnm Texture, uploading initial mip*slice subresources via DMA or CPU tiling.
extern "C" int32_t GXGnmTexture1D_CreateAndInitialize(
    GXGnmTextureHardwareObject* hwTex,
    uint8_t* deviceCtx,
    const GXTexture1DDesc* desc,
    const void* const* initialSubresources,
    GXAllocator* preferredAlloc,
    GXAllocator* fallbackAlloc,
    const GXTextureCustomAllocOptions* opts)
{
    GXAllocator* activeAlloc = ((desc->miscFlags & 0x4000u) != 0) ? preferredAlloc : fallbackAlloc;
    uint32_t sizeAlign[2] = {0, 0};
    const void* customTilePtr = (opts && opts->hasCustomTileInfo != 0) ? opts->customTileData : nullptr;

    uint64_t gnmTex[4] = {0, 0, 0, 0};
    int32_t err = SceGnmTexture1D_InitAndComputeSize(
        reinterpret_cast<uint64_t*>(sizeAlign),
        gnmTex,
        desc,
        customTilePtr);
    if (err != 0) {
        return err;
    }

    uint32_t rawSize = sizeAlign[0];
    uint32_t alignment = sizeAlign[1];
    uint32_t allocSize = rawSize;

    if (opts && opts->useCustomSizeAlign != 0) {
        if (opts->customSize < rawSize || opts->customAlignment < alignment) {
            return 8;
        }
        allocSize = opts->customSize;
        alignment = opts->customAlignment;
    }

    uint32_t hasCounterTail = desc->bindFlags & 0x80u;
    if (hasCounterTail != 0) {
        allocSize = (allocSize + 0x17u) & ~7u;
        if (alignment <= 8u) {
            alignment = 8u;
        }
    }

    uint32_t lastMip = SceGnmTexture_GetLastMipLevel(gnmTex);
    uint8_t* gpuMem = static_cast<uint8_t*>(activeAlloc->vptr->AllocAligned(activeAlloc, allocSize, alignment));
    if (!gpuMem) {
        return 9;
    }

    uint32_t numMips = lastMip + 1;
    if (hasCounterTail != 0) {
        *reinterpret_cast<uint64_t*>(gpuMem + allocSize - 8) = 0;
        *reinterpret_cast<uint64_t*>(gpuMem + allocSize - 16) = 0;
    }

    hwTex->allocator    = activeAlloc;
    hwTex->gpuBaseAddr  = gpuMem;
    hwTex->resourceId   = CGResourceIdGnm_AllocateNextHandle();
    hwTex->rawByteSize  = rawSize;
    hwTex->allocSize    = allocSize;
    hwTex->alignment    = alignment;
    hwTex->dimType      = 2u;
    hwTex->usageMode    = static_cast<uint8_t>(desc->usageMode);
    hwTex->cpuAccess    = static_cast<uint8_t>(desc->cpuAccess);
    hwTex->bindFlags    = static_cast<uint8_t>(desc->bindFlags);
    hwTex->miscFlags    = desc->miscFlags;
    hwTex->tileMode     = SceGnmTexture_GetTileMode(gnmTex);
    hwTex->width        = static_cast<uint16_t>(desc->width);
    hwTex->height       = 1u;
    hwTex->depthOrArray = static_cast<uint16_t>(desc->arraySize);
    hwTex->mipLevels    = static_cast<uint8_t>(numMips);
    hwTex->format       = static_cast<uint8_t>(desc->format);

    bool skipGpuDma = (opts && opts->skipGpuDmaUpload != 0);
    if (!initialSubresources || !CGFormatGnm_IsValidDataFormat(desc->format)) {
        return 0;
    }

    if (opts && opts->copyRawLinear != 0) {
        gx_direct_vk::PrepareDirectTextureWrite(gpuMem, allocSize);
        CGMemoryGnm_Memcpy(gpuMem, initialSubresources[0], rawSize);
        if ((desc->bindFlags & 0x60u) == 0u) {
            gx_direct_vk::SetTSharpBaseAddress(gnmTex, gpuMem);
            const uint32_t isImmutable =
                (desc->usageMode == 1u && desc->bindFlags == 8u && desc->cpuAccess == 0u) ? 1u : 0u;
            gx_direct_vk::RegisterDirectAssetTexture(
                hwTex->resourceId, gnmTex, rawSize, allocSize, isImmutable);
        }
        return 0;
    }

    if (!skipGpuDma && CGMemoryGnm_IsGarlicAddress(hwTex->gpuBaseAddr)) {
        void* lockObj = *reinterpret_cast<void**>(deviceCtx + 0x118);
        (*reinterpret_cast<void(**)(void*, uint32_t)>(*reinterpret_cast<uintptr_t*>(lockObj) + 0x18))(lockObj, 0xFFFFFFFFu);

        uint8_t* cmdCtxPlus40 = deviceCtx + 0x40;
        if (numMips != 0) {
            uint32_t mipStride = lastMip + 1;
            const uint8_t* subBase = reinterpret_cast<const uint8_t*>(initialSubresources);
            for (uint32_t mip = 0; mip < numMips; ++mip) {
                uint32_t subIdx = mip;
                uint32_t arrayCount = desc->arraySize;
                for (uint32_t slice = 0; slice < arrayCount; ++slice, subIdx += mipStride) {
                    CGCommandContextGnm_DmaUploadTextureSubresource(
                        cmdCtxPlus40, hwTex, subBase + static_cast<uint64_t>(subIdx) * 16ULL, subIdx, 0u);
                    arrayCount = desc->arraySize;
                }
            }
        }

        CGCommandContextGnm_FlushAndWaitIdle(cmdCtxPlus40);
        CGCommandContextGnm_SubmitAndReset(cmdCtxPlus40);

        lockObj = *reinterpret_cast<void**>(deviceCtx + 0x118);
        (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<uintptr_t*>(lockObj) + 0x28))(lockObj);
        return 0;
    }

    bool isCpuAccessible = (CGMemoryGnm_IsCpuAccessibleAddress(hwTex->gpuBaseAddr) != 0);
    if (numMips == 0 || !isCpuAccessible) {
        return 0;
    }

    gx_direct_vk::PrepareDirectTextureWrite(gpuMem, allocSize);
    uint8_t tilingParams[0x30];
    uint32_t mipStride = lastMip + 1;
    const uint8_t* subBase = reinterpret_cast<const uint8_t*>(initialSubresources);
    for (uint32_t mip = 0; mip < numMips; ++mip) {
        uint32_t subIdx = mip;
        uint32_t arrayCount = desc->arraySize;
        for (uint32_t slice = 0; slice < arrayCount; ++slice, subIdx += mipStride) {
            uint64_t surfOffset = 0;
            uint64_t surfSize = 0;
            int32_t addrErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&surfOffset, &surfSize, gnmTex, mip, slice);
            if (addrErr != 0) {
                CGErrorGnm_ReportGpuAddrError(addrErr);
                return 8;
            }
            SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mip, slice);
            const void* srcLinear = *reinterpret_cast<const void* const*>(subBase + static_cast<uint64_t>(subIdx) * 16ULL);
            BbGoldenEngine_NotifyGpuSurfaceWrite(gpuMem + surfOffset, surfSize);
            if (SceGpuAddress_TileSurface(gpuMem + surfOffset, srcLinear, tilingParams) != 0) {
                activeAlloc->vptr->Free(activeAlloc, gpuMem);
                return 8;
            }
            BbGoldenEngine_NotifyGpuSurfaceWrite(gpuMem + surfOffset, surfSize);
            arrayCount = desc->arraySize;
        }
    }
    if ((desc->bindFlags & 0x60u) == 0u) {
        gx_direct_vk::SetTSharpBaseAddress(gnmTex, gpuMem);
        const uint32_t isImmutable =
            (desc->usageMode == 1u && desc->bindFlags == 8u && desc->cpuAccess == 0u) ? 1u : 0u;
        gx_direct_vk::RegisterDirectAssetTexture(
            hwTex->resourceId, gnmTex, rawSize, allocSize, isImmutable);
    }
    return 0;
}

struct GXSurfaceFormatInfo2D {
    uint32_t width;         // +0x00
    uint32_t height;        // +0x04
    uint32_t mipLevels;     // +0x08
    uint32_t arraySize;     // +0x0c
    uint32_t format;        // +0x10
    uint32_t sampleCount;   // +0x14
    uint32_t sampleQuality; // +0x18
    uint32_t usageMode;     // +0x1c
    uint32_t bindFlags;     // +0x20
    uint32_t cpuAccess;     // +0x24
    uint32_t miscFlags;     // +0x28
    uint32_t pad_2c;        // +0x2c
    uint32_t gnmDataFormat; // +0x30
    uint32_t tileMode;      // +0x34
    uint32_t zFormat;       // +0x38
    uint32_t stencilFormat; // +0x3c
    uint8_t  isArrayOrCube; // +0x40
    uint8_t  pad_41[3];
    int32_t  status;        // +0x44
};

// [VMA: 0x2171610 - 0x217178d] (382 bytes)
// Computes aligned base surface size + optional mip level offset for a 2D texture surface info block.
extern "C" int32_t GXGnmTexture2D_ComputeMipSurfaceSizeAndAlignment(
    uint64_t* outTotalOffsetOrSize,
    const GXSurfaceFormatInfo2D* surfInfo,
    uint32_t mipLevel)
{
    uint64_t gnmTex[4] = {0, 0, 0, 0};
    uint64_t sizeAlign = GXGnmTexture2D_InitGnmTextureFromSurfaceInfo(surfInfo, gnmTex);
    uint32_t alignMask = static_cast<uint32_t>(sizeAlign >> 32);

    uint64_t tempGnmTex[4] = {0, 0, 0, 0};
    uint32_t isEqaa = ((surfInfo->miscFlags & 0x400u) != 0) ? 1u : 0u;
    uint32_t numSlices = surfInfo->arraySize;
    uint32_t isCubemap = 0u;
    if (surfInfo->isArrayOrCube != 0 && (surfInfo->miscFlags & 4u) != 0) {
        numSlices /= 6u;
        isCubemap = 1u;
    }

    uint32_t numFragments = CGFormatGnm_ResolveNumFragments(
        surfInfo->sampleCount, surfInfo->sampleQuality, isEqaa);

    uint32_t width = surfInfo->width;
    uint32_t height = surfInfo->height;
    uint32_t mipCount = surfInfo->mipLevels;
    if (mipCount == 0) {
        uint32_t lzW = width ? __builtin_clz(width) : 32u;
        uint32_t lzH = height ? __builtin_clz(height) : 32u;
        uint32_t minLz = (lzH < 31u) ? lzH : 31u;
        if (lzW < minLz) {
            minLz = lzW;
        }
        mipCount = 32u - minLz;
    }

    uint64_t baseSizeAlign = SceGnmTexture_Init2DAndReturnSizeAlign(
        tempGnmTex,
        width,
        height,
        numSlices,
        mipCount,
        surfInfo->gnmDataFormat,
        surfInfo->tileMode,
        numFragments,
        isCubemap);

    uint32_t alignedSize = (alignMask != 0u)
        ? ((alignMask + static_cast<uint32_t>(baseSizeAlign) - 1u) & (~alignMask + 1u))
        : static_cast<uint32_t>(baseSizeAlign);
    *outTotalOffsetOrSize = alignedSize;

    if (mipLevel != 0) {
        uint64_t mipOffset = 0;
        uint64_t mipSize = 0;
        int32_t err = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, gnmTex, mipLevel, 0u);
        if (err != 0) {
            CGErrorGnm_ReportGpuAddrError(err);
            return 5;
        }
        *outTotalOffsetOrSize += mipOffset;
    }
    return 0;
}

struct GXSurfaceFormatInfo1D {
    uint32_t width;         // +0x00
    uint32_t mipLevels;     // +0x04
    uint32_t arraySize;     // +0x08
    uint32_t format;        // +0x0c
    uint32_t usageMode;     // +0x10
    uint32_t bindFlags;     // +0x14
    uint32_t cpuAccess;     // +0x18
    uint32_t miscFlags;     // +0x1c
    uint32_t extra_20;      // +0x20
    uint32_t gnmDataFormat; // +0x24
    uint32_t tileMode;      // +0x28
    uint32_t zFormat;       // +0x2c
    uint32_t stencilFormat; // +0x30
    uint8_t  flag_34;       // +0x34
    uint8_t  pad_35[3];
    int32_t  status;        // +0x38
};

// [VMA: 0x2171470 - 0x217160d] (414 bytes)
// Computes aligned base surface size + optional mip level offset for a 1D texture surface info block.
extern "C" int32_t GXGnmTexture1D_ComputeMipSurfaceSizeAndAlignment(
    uint64_t* outTotalOffsetOrSize,
    const GXSurfaceFormatInfo1D* surfInfo,
    uint32_t mipLevel)
{
    uint64_t gnmTexA[4] = {0, 0, 0, 0};
    uint32_t alignMask = 0;

    if (CGFormatGnm_HasFmaskOrHtileFlag(surfInfo->bindFlags, surfInfo->format)) {
        uint32_t width = surfInfo->width;
        uint32_t mipCount = surfInfo->mipLevels;
        if (mipCount == 0) {
            uint32_t lzW = width ? __builtin_clz(width) : 32u;
            mipCount = (lzW < 31u) ? (32u - lzW) : 1u;
        }
        uint64_t sa = SceGnmTexture_Init2DAndReturnSizeAlign(
            gnmTexA, width, 1u, surfInfo->arraySize, mipCount, 0x20401u, surfInfo->tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(gnmTexA, 12u);
        alignMask = static_cast<uint32_t>(sa >> 32);
    }

    uint64_t gnmTexB[4] = {0, 0, 0, 0};
    uint32_t width = surfInfo->width;
    uint32_t mipCount = surfInfo->mipLevels;
    if (mipCount == 0) {
        uint32_t lzW = width ? __builtin_clz(width) : 32u;
        mipCount = (lzW < 31u) ? (32u - lzW) : 1u;
    }

    uint64_t baseSa = SceGnmTexture_Init2DAndReturnSizeAlign(
        gnmTexB, width, 1u, surfInfo->arraySize, mipCount, surfInfo->gnmDataFormat, surfInfo->tileMode, 0u, 0u);
    SceGnmTexture_SetResourceMemoryType(gnmTexB, 12u);

    uint32_t alignedSize = (alignMask != 0u)
        ? ((alignMask + static_cast<uint32_t>(baseSa) - 1u) & (~alignMask + 1u))
        : static_cast<uint32_t>(baseSa);
    *outTotalOffsetOrSize = alignedSize;

    if (mipLevel != 0) {
        uint64_t mipOffset = 0;
        uint64_t mipSize = 0;
        int32_t err = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, gnmTexA, mipLevel, 0u);
        if (err != 0) {
            CGErrorGnm_ReportGpuAddrError(err);
            return 5;
        }
        *outTotalOffsetOrSize += mipOffset;
    }
    return 0;
}

struct GXSurfaceFormatInfo3D {
    uint32_t width;         // +0x00
    uint32_t height;        // +0x04
    uint32_t depth;         // +0x08
    uint32_t mipLevels;     // +0x0c
    uint32_t format;        // +0x10
    uint32_t usageMode;     // +0x14
    uint32_t bindFlags;     // +0x18
    uint32_t cpuAccess;     // +0x1c
    uint32_t miscFlags;     // +0x20
    uint32_t pad_24;        // +0x24
    uint32_t gnmDataFormat; // +0x28
    uint32_t tileMode;      // +0x2c
    uint32_t zFormat;       // +0x30
    uint32_t stencilFormat; // +0x34
    uint8_t  flag_38;       // +0x38
    uint8_t  pad_39[3];
    int32_t  status;        // +0x3c
};

// [VMA: 0x2171340 - 0x217146e] (303 bytes)
// Validates and populates Gnm data format, tileMode, zFormat, and stencilFormat for a 3D texture descriptor.
extern "C" void GXGnmTexture3D_ValidateAndComputeFormatInfo(
    GXSurfaceFormatInfo3D* outInfo,
    const GXTexture3DDesc* desc,
    const uint32_t* optionalTileMode,
    uint8_t flagByte)
{
    std::memcpy(outInfo, desc, 0x28);
    outInfo->tileMode      = optionalTileMode ? *optionalTileMode : 0u;
    outInfo->zFormat       = 0u;
    outInfo->stencilFormat = 0u;
    outInfo->flag_38       = flagByte;

    if (outInfo->format == 0x16u) {
        outInfo->gnmDataFormat = 0x204704u;
    } else {
        int32_t fmtErr = CGFormatGnm_ConvertToGnmDataFormat(&outInfo->gnmDataFormat, outInfo->format);
        outInfo->status = fmtErr;
        if (fmtErr != 0) {
            return;
        }
    }

    if (!optionalTileMode) {
        uint32_t arrayModeIdx = CGFormatGnm_ResolveTileModeIndex(
            4u, outInfo->format, outInfo->usageMode, outInfo->bindFlags, outInfo->miscFlags);
        int32_t tileErr = SceGpuAddress_ComputeTileMode(
            &outInfo->tileMode, arrayModeIdx, outInfo->gnmDataFormat, 1u);
        if (tileErr != 0) {
            CGErrorGnm_ReportGpuAddrError(tileErr);
            outInfo->status = 8;
            return;
        }
    }

    uint32_t fmtMinus20 = outInfo->format - 0x14u;
    if (fmtMinus20 <= 0x23u && ((0x0802100001ULL >> fmtMinus20) & 1ULL) != 0) {
        int32_t zErr = CGFormatGnm_ConvertToDepthFormat(&outInfo->zFormat, outInfo->format);
        outInfo->status = zErr;
        if (zErr != 0) {
            return;
        }
    }

    if (CGFormatGnm_HasFmaskOrHtileFlag(outInfo->bindFlags, outInfo->format)) {
        int32_t stErr = CGFormatGnm_ConvertToStencilFormat(&outInfo->stencilFormat, outInfo->format);
        outInfo->status = stErr;
        if (stErr != 0) {
            return;
        }
    }

    outInfo->status = 0;
}

// [VMA: 0x21711d0 - 0x2171338] (361 bytes)
// Validates and populates Gnm data format, MSAA fragment count, tileMode, zFormat, and stencilFormat for a 2D texture descriptor.
extern "C" void GXGnmTexture2D_ValidateAndComputeFormatInfo(
    GXSurfaceFormatInfo2D* outInfo,
    const GXTexture2DDesc* desc,
    const uint32_t* optionalTileMode,
    uint8_t isArrayOrCube)
{
    std::memcpy(outInfo, desc, 0x30);
    outInfo->tileMode      = optionalTileMode ? *optionalTileMode : 0u;
    outInfo->zFormat       = 0u;
    outInfo->stencilFormat = 0u;
    outInfo->isArrayOrCube = isArrayOrCube;

    if (outInfo->format == 0x16u) {
        outInfo->gnmDataFormat = 0x204704u;
    } else {
        int32_t fmtErr = CGFormatGnm_ConvertToGnmDataFormat(&outInfo->gnmDataFormat, outInfo->format);
        outInfo->status = fmtErr;
        if (fmtErr != 0) {
            return;
        }
    }

    if (!optionalTileMode) {
        uint32_t arrayModeIdx = CGFormatGnm_ResolveTileModeIndex(
            3u, outInfo->format, outInfo->usageMode, outInfo->bindFlags, outInfo->miscFlags);
        uint32_t numSamples = outInfo->sampleCount;
        if ((outInfo->bindFlags & 0x60u) != 0) {
            uint32_t isEqaa = (outInfo->miscFlags & 0x400u) >> 10;
            uint32_t fragLog2 = CGFormatGnm_ResolveNumFragments(outInfo->sampleCount, outInfo->sampleQuality, isEqaa);
            numSamples = CGFormatGnm_FragmentLog2ToCount(fragLog2);
        }
        int32_t tileErr = SceGpuAddress_ComputeTileMode(
            &outInfo->tileMode, arrayModeIdx, outInfo->gnmDataFormat, numSamples);
        if (tileErr != 0) {
            CGErrorGnm_ReportGpuAddrError(tileErr);
            outInfo->status = 8;
            return;
        }
    }

    uint32_t fmtMinus20 = outInfo->format - 0x14u;
    if (fmtMinus20 <= 0x23u && ((0x0802100001ULL >> fmtMinus20) & 1ULL) != 0) {
        int32_t zErr = CGFormatGnm_ConvertToDepthFormat(&outInfo->zFormat, outInfo->format);
        outInfo->status = zErr;
        if (zErr != 0) {
            return;
        }
    }

    if (CGFormatGnm_HasFmaskOrHtileFlag(outInfo->bindFlags, outInfo->format)) {
        int32_t stErr = CGFormatGnm_ConvertToStencilFormat(&outInfo->stencilFormat, outInfo->format);
        outInfo->status = stErr;
        if (stErr != 0) {
            return;
        }
    }

    outInfo->status = 0;
}

// [VMA: 0x21710a0 - 0x21711cc] (301 bytes)
// Validates and populates Gnm data format, tileMode, zFormat, and stencilFormat for a 1D texture descriptor.
extern "C" void GXGnmTexture1D_ValidateAndComputeFormatInfo(
    GXSurfaceFormatInfo1D* outInfo,
    const GXTexture1DDesc* desc,
    const uint32_t* optionalTileMode,
    uint8_t flagByte)
{
    std::memcpy(outInfo, desc, 0x24);
    outInfo->tileMode      = optionalTileMode ? *optionalTileMode : 0u;
    outInfo->zFormat       = 0u;
    outInfo->stencilFormat = 0u;
    outInfo->flag_34       = flagByte;

    if (outInfo->format == 0x16u) {
        outInfo->gnmDataFormat = 0x204704u;
    } else {
        int32_t fmtErr = CGFormatGnm_ConvertToGnmDataFormat(&outInfo->gnmDataFormat, outInfo->format);
        outInfo->status = fmtErr;
        if (fmtErr != 0) {
            return;
        }
    }

    if (!optionalTileMode) {
        uint32_t arrayModeIdx = CGFormatGnm_ResolveTileModeIndex(
            2u, outInfo->format, outInfo->usageMode, outInfo->bindFlags, outInfo->miscFlags);
        int32_t tileErr = SceGpuAddress_ComputeTileMode(
            &outInfo->tileMode, arrayModeIdx, outInfo->gnmDataFormat, 1u);
        if (tileErr != 0) {
            CGErrorGnm_ReportGpuAddrError(tileErr);
            outInfo->status = 8;
            return;
        }
    }

    uint32_t fmtMinus20 = outInfo->format - 0x14u;
    if (fmtMinus20 <= 0x23u && ((0x0802100001ULL >> fmtMinus20) & 1ULL) != 0) {
        int32_t zErr = CGFormatGnm_ConvertToDepthFormat(&outInfo->zFormat, outInfo->format);
        outInfo->status = zErr;
        if (zErr != 0) {
            return;
        }
    }

    if (CGFormatGnm_HasFmaskOrHtileFlag(outInfo->bindFlags, outInfo->format)) {
        int32_t stErr = CGFormatGnm_ConvertToStencilFormat(&outInfo->stencilFormat, outInfo->format);
        outInfo->status = stErr;
        if (stErr != 0) {
            return;
        }
    }

    outInfo->status = 0;
}

// Additional Gnm DepthRenderTarget / RenderTarget / SRV external helpers
extern "C" {
    int32_t  SceGpuAddress_ComputeSurfaceInfo(void* outSurfInfo, const void* tilingParams); // 0x109ce10
    int32_t  SceGpuAddress_ComputeHtileSurfaceSizeAndAlignment(uint32_t* outSize, uint32_t* outAlign, uint32_t* outPitch, uint32_t* outHeight, uint32_t surfPitch, uint32_t surfHeight, uint32_t surfDepth, uint32_t flags); // 0x109e570
    int32_t  SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(uint32_t* outSize, uint32_t* outAlign, uint32_t* outPitch, uint32_t* outHeight, uint32_t surfPitch, uint32_t surfHeight, uint32_t surfDepth, uint32_t flags); // 0x109e2d0
    int32_t  SceGnmDepthRenderTarget_Init(void* outDrt, uint32_t width, uint32_t height, uint32_t numSlices, uint32_t zFormat, uint32_t stencilFormat, uint32_t tileMode, uint32_t numFragments, void* outStencilSizeAlign, void* outHtileSizeAlign); // 0x1071b70
    void     SceGnmDepthRenderTarget_SetPitchMinusOne(void* drt, uint32_t pitchDiv8Minus1); // 0x1072430
    void     SceGnmDepthRenderTarget_SetSliceSizeMinusOne(void* drt, uint32_t sliceDiv64Minus1); // 0x1072470
    void     SceGnmDepthRenderTarget_SetSliceViewRange(void* drt, uint32_t firstSlice, uint32_t lastSlice); // 0x10721d0
    uint32_t CGAddressGnm_ConvertTo256ByteBaseAddr(const void* addr); // 0x26baeb0
    void     SceGnmDepthRenderTarget_SetZReadAddress256(void* drt, uint32_t addr256); // 0x10726d0
    void     SceGnmDepthRenderTarget_SetZWriteAddress256(void* drt, uint32_t addr256); // 0x10726e0
    void     SceGnmDepthRenderTarget_SetStencilReadAddress256(void* drt, uint32_t addr256); // 0x10726f0
    void     SceGnmDepthRenderTarget_SetStencilWriteAddress256(void* drt, uint32_t addr256); // 0x1072700
    void     SceGnmDepthRenderTarget_SetHtileAddress256(void* drt, uint32_t addr256); // 0x10727e0
    void     SceGnmDepthRenderTarget_SetHtileStencilDisable(void* drt, uint32_t stencilDisable); // 0x1072410
    uint32_t CGFormatGnm_ResolveNumSamplesLog2(uint32_t sampleCount, uint32_t isEqaa); // 0x26b7a20
    uint64_t SceGnmRenderTarget_Init(void* outRt, uint32_t width, uint32_t height, uint32_t numSlices, uint32_t dataFormat, uint32_t tileMode, uint32_t numSamplesLog2, uint32_t numFragments, void* outCmaskSizeAlign, void* outFmaskSizeAlign); // 0x1085f90
    void     SceGnmRenderTarget_SetSliceViewRange(void* rt, uint32_t firstSlice, uint32_t lastSlice); // 0x1086520
    void     SceGnmRenderTarget_SetBaseAddress256ByteBlocks(void* rt, uint32_t addr256); // 0x1086c50
    void     SceGnmRenderTarget_SetCmaskAddress256ByteBlocks(void* rt, uint32_t addr256); // 0x1086c80
    void     SceGnmRenderTarget_SetCmaskFastClearEnable(void* rt, uint32_t enable); // 0x10867c0
    void     SceGnmRenderTarget_SetFmaskAddress256ByteBlocks(void* rt, uint32_t addr256); // 0x1086c90
    void     SceGnmRenderTarget_SetFmaskCompressionEnable(void* rt, uint32_t enable); // 0x10867e0
    uint64_t SceGnmTexture_Init3DAndReturnSizeAlign(void* outGnmTex, uint32_t width, uint32_t height, uint32_t depth, uint32_t numMips, uint32_t dataFormat, uint32_t tileMode); // 0x1088590
    void     SceGnmRenderTarget_SetPitchMinusOne(void* rt, uint32_t pitchDiv8Minus1); // 0x10866e0
    void     SceGnmRenderTarget_SetSliceSizeMinusOne(void* rt, uint32_t sliceDiv64Minus1); // 0x1086710
    uint32_t SceGnmRenderTarget_GetFmaskTileMode(const void* rt); // 0x10868e0
    void     SceGnmRenderTarget_SetFmaskTileMode(void* rt, uint32_t fmaskTileMode); // 0x1086760
    uint32_t SceGnmRenderTarget_GetFmaskSliceSizeMinusOne(const void* rt); // 0x1086cc0
    void     SceGnmRenderTarget_SetFmaskSliceSizeMinusOne(void* rt, uint32_t sliceMinusOne); // 0x1086800
    uint32_t SceGnmRenderTarget_GetBaseAddress256ByteBlocks(const void* rt); // 0x1086ca0
    uint8_t  CGFormatGnm_IsStencilOnlyViewFormat(uint32_t bindFlags, uint32_t gxFormat); // 0x26bb510
    void     SceGnmTexture_InitFromRenderTargetFmask(void* outGnmTex, const void* rt, uint32_t isCubemap); // 0x1088d90
    void     SceGnmTexture_SetBaseArrayViewRange(void* gnmTex, uint32_t firstSlice, uint32_t lastSlice); // 0x10884f0
    void     SceGnmTexture_SetMipLevelViewRange(void* gnmTex, uint32_t firstMip, uint32_t lastMip); // 0x1088510
    void     SceGnmTexture_SetBaseAddress256ByteBlocks(void* gnmTex, uint32_t addr256); // 0x10890c0
}

extern "C" int32_t GXGnmShaderResourceView_InitializeForSurfaceMipSlice(
    void* outGnmTex,
    const GXGnmTextureHardwareObject* hwTex,
    uint32_t viewFormat,
    uint32_t firstMip,
    uint32_t firstSlice,
    uint32_t mipCount,
    uint32_t sliceCount,
    uint8_t isArrayOrCube);

// [VMA: 0x216fa60 - 0x217100f] (5551 bytes)
// Initializes a sce::Gnm::DepthRenderTarget for a specific mip level and array slice range of a 2D or 1D depth/stencil texture, computing Z, Stencil, and HTILE base addresses and alignments.
static int32_t GXGnmDepthRenderTarget_InitializeForSurfaceMipSlice_Impl(
    void* outDrt,
    const GXGnmTextureHardwareObject* hwTex,
    uint32_t viewFormat,
    uint32_t mipLevel,
    uint32_t firstSlice,
    uint32_t sliceCount)
{
    // Reject disallowed typeless/non-depth view formats
    if (viewFormat == 1u || viewFormat == 5u || viewFormat == 9u || viewFormat == 0xFu ||
        viewFormat == 0x13u || viewFormat == 0x17u || viewFormat == 0x1Bu || viewFormat == 0x21u)
    {
        return 8;
    }
    if (viewFormat >= 0x27u && (viewFormat - 0x27u) <= 0x3Au) {
        if (((0x04A8092480204221ULL >> (viewFormat - 0x27u)) & 1ULL) != 0) {
            return 8;
        }
    }

    uint32_t fmtMinus20 = viewFormat - 0x14u;
    if (fmtMinus20 > 0x23u || ((0x0802100001ULL >> fmtMinus20) & 1ULL) == 0) {
        return 8;
    }

    if (hwTex->dimType == 3u) {
        // 2D DepthRenderTarget initialization
        GXTexture2DDesc desc2d;
        desc2d.width         = hwTex->width;
        desc2d.height        = hwTex->height;
        desc2d.mipLevels     = hwTex->mipLevels;
        desc2d.arraySize     = hwTex->depthOrArray;
        desc2d.format        = viewFormat;
        desc2d.sampleCount   = hwTex->sampleCount;
        desc2d.sampleQuality = hwTex->sampleQuality;
        desc2d.usageMode     = hwTex->usageMode;
        desc2d.bindFlags     = hwTex->bindFlags;
        desc2d.cpuAccess     = hwTex->cpuAccess;
        desc2d.miscFlags     = hwTex->miscFlags;

        GXSurfaceFormatInfo2D surf2d;
        GXGnmTexture2D_ValidateAndComputeFormatInfo(&surf2d, &desc2d, &hwTex->tileMode, 0u);
        if (surf2d.status != 0) {
            return surf2d.status;
        }
        if ((surf2d.bindFlags & 0x40u) == 0) {
            return 8;
        }

        const uint8_t* gpuBase = static_cast<const uint8_t*>(hwTex->gpuBaseAddr);
        uint32_t isEqaa = (surf2d.miscFlags >> 10) & 1u;
        uint8_t hasStencil = CGFormatGnm_HasFmaskOrHtileFlag(surf2d.bindFlags, surf2d.format);
        uint32_t hasHtile = isEqaa ? 1u : ((surf2d.miscFlags & 0x100u) >> 8);

        struct GpuSurfaceInfo {
            uint32_t pitch;          // +0x00
            uint32_t height;         // +0x04
            uint32_t depth;          // +0x08
            uint32_t pad_0c;         // +0x0c
            uint64_t surfSize;       // +0x10
            uint64_t pad_18;         // +0x18
            uint64_t pad_20;         // +0x20
            uint32_t baseAlign;      // +0x28
            uint32_t bpp;            // +0x2c
            uint32_t arrayModeOrTile;// +0x30
            uint32_t pad_34;         // +0x34
        };
        GpuSurfaceInfo surfInfo;
        std::memset(&surfInfo, 0, sizeof(surfInfo));

        uint64_t gnmTex[4] = {0, 0, 0, 0};
        uint32_t numSlices = surf2d.arraySize;
        uint32_t isCubemap = 0u;
        if (surf2d.isArrayOrCube != 0 && (surf2d.miscFlags & 4u) != 0) {
            numSlices /= 6u;
            isCubemap = 1u;
        }

        uint32_t numFrags = CGFormatGnm_ResolveNumFragments(
            surf2d.sampleCount, surf2d.sampleQuality, (surf2d.miscFlags & 0x400u) ? 1u : 0u);
        uint32_t mipCount = surf2d.mipLevels;
        if (mipCount == 0) {
            uint32_t lzW = surf2d.width ? __builtin_clz(surf2d.width) : 32u;
            uint32_t lzH = surf2d.height ? __builtin_clz(surf2d.height) : 32u;
            uint32_t minLz = (lzH < 31u) ? lzH : 31u;
            if (lzW < minLz) minLz = lzW;
            mipCount = 32u - minLz;
        }

        SceGnmTexture_Init2DAndReturnSizeAlign(
            gnmTex, surf2d.width, surf2d.height, numSlices, mipCount,
            surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);

        uint8_t tilingParams[0x30];
        SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mipLevel, 0u);
        int32_t addrErr = SceGpuAddress_ComputeSurfaceInfo(&surfInfo, tilingParams);
        if (addrErr != 0) {
            CGErrorGnm_ReportGpuAddrError(addrErr);
            return 5;
        }

        uint64_t stencilSizeAlign = 0;
        uint64_t htileSizeAlign = 0;
        void* stencilSaPtr = hasStencil ? &stencilSizeAlign : nullptr;
        void* htileSaPtr = hasHtile ? &htileSizeAlign : nullptr;

        uint32_t drtFrags = CGFormatGnm_ResolveNumFragments(surf2d.sampleCount, surf2d.sampleQuality, isEqaa);
        uint32_t mipW = surf2d.width >> mipLevel;
        if (mipW == 0) mipW = 1u;
        uint32_t mipH = surf2d.height >> mipLevel;
        if (mipH == 0) mipH = 1u;

        SceGnmDepthRenderTarget_Init(
            outDrt, mipW, mipH, surf2d.arraySize, surf2d.zFormat, surf2d.stencilFormat,
            surfInfo.arrayModeOrTile, drtFrags, stencilSaPtr, htileSaPtr);

        uint32_t fragCnt = CGFormatGnm_FragmentLog2ToCount(drtFrags);
        uint64_t sliceElements = (surfInfo.surfSize / (surfInfo.bpp >> 3)) / fragCnt;
        SceGnmDepthRenderTarget_SetPitchMinusOne(outDrt, (surfInfo.pitch >> 3) - 1u);
        SceGnmDepthRenderTarget_SetSliceSizeMinusOne(outDrt, static_cast<uint32_t>((sliceElements >> 6) - 1u));
        SceGnmDepthRenderTarget_SetSliceViewRange(outDrt, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t zMipOffset = 0;
        if (mipLevel != 0) {
            uint64_t tempTex[4] = {0, 0, 0, 0};
            SceGnmTexture_Init2DAndReturnSizeAlign(
                tempTex, surf2d.width, surf2d.height, numSlices, mipCount,
                surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);
            uint64_t mipSz = 0;
            int32_t mErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&zMipOffset, &mipSz, tempTex, mipLevel, 0u);
            if (mErr != 0) {
                CGErrorGnm_ReportGpuAddrError(mErr);
                return 5;
            }
        }

        uint32_t zAddr256 = CGAddressGnm_ConvertTo256ByteBaseAddr(gpuBase + zMipOffset);
        SceGnmDepthRenderTarget_SetZReadAddress256(outDrt, zAddr256);
        SceGnmDepthRenderTarget_SetZWriteAddress256(outDrt, zAddr256);

        if (hasStencil) {
            uint64_t stOffset = 0;
            int32_t stErr = GXGnmTexture2D_ComputeMipSurfaceSizeAndAlignment(&stOffset, &surf2d, mipLevel);
            if (stErr != 0) {
                return stErr;
            }
            uint32_t stAddr256 = CGAddressGnm_ConvertTo256ByteBaseAddr(gpuBase + stOffset);
            SceGnmDepthRenderTarget_SetStencilReadAddress256(outDrt, stAddr256);
            SceGnmDepthRenderTarget_SetStencilWriteAddress256(outDrt, stAddr256);
        }

        if (!hasHtile) {
            return 0;
        }

        uint64_t texHtileBase[4] = {0, 0, 0, 0};
        uint64_t zTotalSa = SceGnmTexture_Init2DAndReturnSizeAlign(
            texHtileBase, surf2d.width, surf2d.height, numSlices, mipCount,
            surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);

        uint64_t stTotalSize = 0;
        uint64_t stTotalAlign = 0;
        if (CGFormatGnm_HasFmaskOrHtileFlag(surf2d.bindFlags, surf2d.format)) {
            uint64_t stTempTex[4] = {0, 0, 0, 0};
            uint64_t stSa = SceGnmTexture_Init2DAndReturnSizeAlign(
                stTempTex, surf2d.width, surf2d.height, numSlices, mipCount,
                0x20401u, surf2d.tileMode, numFrags, isCubemap);
            stTotalSize = static_cast<uint32_t>(stSa);
            stTotalAlign = stSa >> 32;
        }

        uint64_t texIter[4] = {0, 0, 0, 0};
        SceGnmTexture_Init2DAndReturnSizeAlign(
            texIter, surf2d.width, surf2d.height, numSlices, mipCount,
            surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);

        uint64_t htileAccumOffset = 0;
        uint32_t maxHtileAlign = 0;
        for (uint32_t m = 0; m < mipLevel; ++m) {
            uint8_t tp[0x30];
            GpuSurfaceInfo si;
            std::memset(&si, 0, sizeof(si));
            SceGpuAddress_InitTilingParametersFromTexture(tp, texIter, m, 0u);
            int32_t e1 = SceGpuAddress_ComputeSurfaceInfo(&si, tp);
            if (e1 != 0) {
                CGErrorGnm_ReportGpuAddrError(e1);
                return 5;
            }
            uint32_t htSize = 0, htAlign = 0, htPitch = 0, htHeight = 0;
            int32_t e2 = SceGpuAddress_ComputeHtileSurfaceSizeAndAlignment(
                &htSize, &htAlign, &htPitch, &htHeight, si.pitch, si.height, si.depth, 0u);
            if (e2 != 0) {
                CGErrorGnm_ReportGpuAddrError(e2);
                return 5;
            }
            if (htAlign != 0) {
                uint32_t alignedPrev = (static_cast<uint32_t>(htileAccumOffset) + htAlign - 1u) & (~htAlign + 1u);
                htileAccumOffset = alignedPrev + htSize * surf2d.arraySize;
            }
            if (htAlign > maxHtileAlign) {
                maxHtileAlign = htAlign;
            }
        }

        uint8_t tpTarget[0x30];
        GpuSurfaceInfo siTarget;
        std::memset(&siTarget, 0, sizeof(siTarget));
        SceGpuAddress_InitTilingParametersFromTexture(tpTarget, texHtileBase, mipLevel, 0u);
        int32_t eT1 = SceGpuAddress_ComputeSurfaceInfo(&siTarget, tpTarget);
        if (eT1 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT1);
            return 5;
        }
        uint32_t curHtSize = 0, curHtAlign = 0, curHtPitch = 0, curHtHeight = 0;
        int32_t eT2 = SceGpuAddress_ComputeHtileSurfaceSizeAndAlignment(
            &curHtSize, &curHtAlign, &curHtPitch, &curHtHeight,
            siTarget.pitch, siTarget.height, siTarget.depth, 0u);
        if (eT2 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT2);
            return 5;
        }

        if (curHtAlign != 0) {
            htileAccumOffset = (static_cast<uint32_t>(htileAccumOffset) + curHtAlign - 1u) & (~curHtAlign + 1u);
        }
        uint32_t finalHtAlign = (maxHtileAlign > curHtAlign) ? maxHtileAlign : curHtAlign;
        uint64_t baseBeforeHtile = static_cast<uint32_t>(zTotalSa);
        if (stTotalAlign != 0) {
            baseBeforeHtile = ((baseBeforeHtile + stTotalAlign - 1ULL) & (~stTotalAlign + 1ULL)) + stTotalSize;
        }
        if (finalHtAlign != 0) {
            baseBeforeHtile = ((static_cast<uint32_t>(baseBeforeHtile) + finalHtAlign - 1u) & (~finalHtAlign + 1u)) +
                              static_cast<uint32_t>(htileAccumOffset);
        }

        uintptr_t htileGpuAddr = reinterpret_cast<uintptr_t>(gpuBase) + static_cast<uint32_t>(baseBeforeHtile);
        SceGnmDepthRenderTarget_SetHtileAddress256(outDrt, static_cast<uint32_t>(htileGpuAddr >> 8));
        uint32_t stencilDisable = 1u;
        if (hasStencil) {
            stencilDisable = ((surf2d.miscFlags & 0x8000u) == 0) ? 1u : 0u;
        }
        SceGnmDepthRenderTarget_SetHtileStencilDisable(outDrt, stencilDisable);
        return 0;
    }

    if (hwTex->dimType == 2u) {
        // 1D DepthRenderTarget initialization
        GXTexture1DDesc desc1d;
        desc1d.width     = hwTex->width;
        desc1d.mipLevels = hwTex->mipLevels;
        desc1d.arraySize = hwTex->depthOrArray;
        desc1d.format    = viewFormat;
        desc1d.usageMode = hwTex->usageMode;
        desc1d.bindFlags = hwTex->bindFlags;
        desc1d.cpuAccess = hwTex->cpuAccess;
        desc1d.miscFlags = hwTex->miscFlags;

        GXSurfaceFormatInfo1D surf1d;
        GXGnmTexture1D_ValidateAndComputeFormatInfo(&surf1d, &desc1d, &hwTex->tileMode, 0u);
        if (surf1d.status != 0) {
            return surf1d.status;
        }
        if ((surf1d.bindFlags & 0x40u) == 0) {
            return 8;
        }

        const uint8_t* gpuBase = static_cast<const uint8_t*>(hwTex->gpuBaseAddr);
        uint32_t isEqaa = (surf1d.miscFlags >> 10) & 1u;
        uint8_t hasStencil = CGFormatGnm_HasFmaskOrHtileFlag(surf1d.bindFlags, surf1d.format);
        uint8_t hasHtile = isEqaa ? 1u : static_cast<uint8_t>((surf1d.miscFlags >> 8) & 1u);

        struct GpuSurfaceInfo {
            uint32_t pitch;
            uint32_t height;
            uint32_t depth;
            uint32_t pad_0c;
            uint64_t surfSize;
            uint64_t pad_18;
            uint64_t pad_20;
            uint32_t baseAlign;
            uint32_t bpp;
            uint32_t arrayModeOrTile;
            uint32_t pad_34;
        };
        GpuSurfaceInfo surfInfo;
        std::memset(&surfInfo, 0, sizeof(surfInfo));

        uint64_t gnmTex[4] = {0, 0, 0, 0};
        uint32_t mipCount = surf1d.mipLevels;
        if (mipCount == 0) {
            uint32_t lzW = surf1d.width ? __builtin_clz(surf1d.width) : 32u;
            mipCount = (lzW < 31u) ? (32u - lzW) : 1u;
        }

        SceGnmTexture_Init2DAndReturnSizeAlign(
            gnmTex, surf1d.width, 1u, surf1d.arraySize, mipCount,
            surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(gnmTex, 12u);

        uint8_t tilingParams[0x30];
        SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mipLevel, 0u);
        int32_t addrErr = SceGpuAddress_ComputeSurfaceInfo(&surfInfo, tilingParams);
        if (addrErr != 0) {
            CGErrorGnm_ReportGpuAddrError(addrErr);
            return 5;
        }

        uint64_t stencilSizeAlign = 0;
        uint64_t htileSizeAlign = 0;
        void* stencilSaPtr = hasStencil ? &stencilSizeAlign : nullptr;
        void* htileSaPtr = hasHtile ? &htileSizeAlign : nullptr;

        uint32_t drtFrags = CGFormatGnm_ResolveNumFragments(1u, 0u, isEqaa);
        uint32_t mipW = surf1d.width >> mipLevel;
        if (mipW == 0) mipW = 1u;
        uint32_t mipH = 1u >> mipLevel;
        if (mipH == 0) mipH = 1u;

        SceGnmDepthRenderTarget_Init(
            outDrt, mipW, mipH, surf1d.arraySize, surf1d.zFormat, surf1d.stencilFormat,
            surfInfo.arrayModeOrTile, drtFrags, stencilSaPtr, htileSaPtr);

        uint32_t fragCnt = CGFormatGnm_FragmentLog2ToCount(drtFrags);
        uint64_t sliceElements = (surfInfo.surfSize / (surfInfo.bpp >> 3)) / fragCnt;
        SceGnmDepthRenderTarget_SetPitchMinusOne(outDrt, (surfInfo.pitch >> 3) - 1u);
        SceGnmDepthRenderTarget_SetSliceSizeMinusOne(outDrt, static_cast<uint32_t>((sliceElements >> 6) - 1u));
        SceGnmDepthRenderTarget_SetSliceViewRange(outDrt, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t zMipOffset = 0;
        if (mipLevel != 0) {
            uint64_t tempTex[4] = {0, 0, 0, 0};
            SceGnmTexture_Init2DAndReturnSizeAlign(
                tempTex, surf1d.width, 1u, surf1d.arraySize, mipCount,
                surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
            SceGnmTexture_SetResourceMemoryType(tempTex, 12u);
            uint64_t mipSz = 0;
            int32_t mErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&zMipOffset, &mipSz, tempTex, mipLevel, 0u);
            if (mErr != 0) {
                CGErrorGnm_ReportGpuAddrError(mErr);
                return 5;
            }
        }

        uint32_t zAddr256 = CGAddressGnm_ConvertTo256ByteBaseAddr(gpuBase + zMipOffset);
        SceGnmDepthRenderTarget_SetZReadAddress256(outDrt, zAddr256);
        SceGnmDepthRenderTarget_SetZWriteAddress256(outDrt, zAddr256);

        if (hasStencil) {
            uint64_t stOffset = 0;
            int32_t stErr = GXGnmTexture1D_ComputeMipSurfaceSizeAndAlignment(&stOffset, &surf1d, mipLevel);
            if (stErr != 0) {
                return stErr;
            }
            uint32_t stAddr256 = CGAddressGnm_ConvertTo256ByteBaseAddr(gpuBase + stOffset);
            SceGnmDepthRenderTarget_SetStencilReadAddress256(outDrt, stAddr256);
            SceGnmDepthRenderTarget_SetStencilWriteAddress256(outDrt, stAddr256);
        }

        if (!hasHtile) {
            return 0;
        }

        uint64_t texHtileBase[4] = {0, 0, 0, 0};
        uint64_t zTotalSa = SceGnmTexture_Init2DAndReturnSizeAlign(
            texHtileBase, surf1d.width, 1u, surf1d.arraySize, mipCount,
            surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(texHtileBase, 12u);

        uint64_t stTotalSize = 0;
        uint64_t stTotalAlign = 0;
        if (CGFormatGnm_HasFmaskOrHtileFlag(surf1d.bindFlags, surf1d.format)) {
            uint64_t stTempTex[4] = {0, 0, 0, 0};
            uint64_t stSa = SceGnmTexture_Init2DAndReturnSizeAlign(
                stTempTex, surf1d.width, 1u, surf1d.arraySize, mipCount,
                0x20401u, surf1d.tileMode, 0u, 0u);
            SceGnmTexture_SetResourceMemoryType(stTempTex, 12u);
            stTotalSize = static_cast<uint32_t>(stSa);
            stTotalAlign = stSa >> 32;
        }

        uint64_t texIter[4] = {0, 0, 0, 0};
        SceGnmTexture_Init2DAndReturnSizeAlign(
            texIter, surf1d.width, 1u, surf1d.arraySize, mipCount,
            surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(texIter, 12u);

        uint64_t htileAccumOffset = 0;
        uint32_t maxHtileAlign = 0;
        for (uint32_t m = 0; m < mipLevel; ++m) {
            uint8_t tp[0x30];
            GpuSurfaceInfo si;
            std::memset(&si, 0, sizeof(si));
            SceGpuAddress_InitTilingParametersFromTexture(tp, texIter, m, 0u);
            int32_t e1 = SceGpuAddress_ComputeSurfaceInfo(&si, tp);
            if (e1 != 0) {
                CGErrorGnm_ReportGpuAddrError(e1);
                return 5;
            }
            uint32_t htSize = 0, htAlign = 0, htPitch = 0, htHeight = 0;
            int32_t e2 = SceGpuAddress_ComputeHtileSurfaceSizeAndAlignment(
                &htSize, &htAlign, &htPitch, &htHeight, si.pitch, si.height, si.depth, 0u);
            if (e2 != 0) {
                CGErrorGnm_ReportGpuAddrError(e2);
                return 5;
            }
            if (htAlign != 0) {
                uint32_t alignedPrev = (static_cast<uint32_t>(htileAccumOffset) + htAlign - 1u) & (~htAlign + 1u);
                htileAccumOffset = alignedPrev + htSize * surf1d.arraySize;
            }
            if (htAlign > maxHtileAlign) {
                maxHtileAlign = htAlign;
            }
        }

        uint8_t tpTarget[0x30];
        GpuSurfaceInfo siTarget;
        std::memset(&siTarget, 0, sizeof(siTarget));
        SceGpuAddress_InitTilingParametersFromTexture(tpTarget, texHtileBase, mipLevel, 0u);
        int32_t eT1 = SceGpuAddress_ComputeSurfaceInfo(&siTarget, tpTarget);
        if (eT1 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT1);
            return 5;
        }
        uint32_t curHtSize = 0, curHtAlign = 0, curHtPitch = 0, curHtHeight = 0;
        int32_t eT2 = SceGpuAddress_ComputeHtileSurfaceSizeAndAlignment(
            &curHtSize, &curHtAlign, &curHtPitch, &curHtHeight,
            siTarget.pitch, siTarget.height, siTarget.depth, 0u);
        if (eT2 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT2);
            return 5;
        }

        if (curHtAlign != 0) {
            htileAccumOffset = (static_cast<uint32_t>(htileAccumOffset) + curHtAlign - 1u) & (~curHtAlign + 1u);
        }
        uint32_t finalHtAlign = (maxHtileAlign > curHtAlign) ? maxHtileAlign : curHtAlign;
        uint64_t baseBeforeHtile = static_cast<uint32_t>(zTotalSa);
        if (stTotalAlign != 0) {
            baseBeforeHtile = ((baseBeforeHtile + stTotalAlign - 1ULL) & (~stTotalAlign + 1ULL)) + stTotalSize;
        }
        if (finalHtAlign != 0) {
            baseBeforeHtile = ((static_cast<uint32_t>(baseBeforeHtile) + finalHtAlign - 1u) & (~finalHtAlign + 1u)) +
                              static_cast<uint32_t>(htileAccumOffset);
        }

        uintptr_t htileGpuAddr = reinterpret_cast<uintptr_t>(gpuBase) + static_cast<uint32_t>(baseBeforeHtile);
        SceGnmDepthRenderTarget_SetHtileAddress256(outDrt, static_cast<uint32_t>(htileGpuAddr >> 8));
        uint32_t stencilDisable = 1u;
        if (hasStencil) {
            stencilDisable = ((surf1d.miscFlags & 0x8000u) == 0) ? 1u : 0u;
        }
        SceGnmDepthRenderTarget_SetHtileStencilDisable(outDrt, stencilDisable);
        return 0;
    }

    return 8;
}

extern "C" int32_t GXGnmDepthRenderTarget_InitializeForSurfaceMipSlice(
    void* outDrt,
    const GXGnmTextureHardwareObject* hwTex,
    uint32_t viewFormat,
    uint32_t mipLevel,
    uint32_t firstSlice,
    uint32_t sliceCount)
{
    const int32_t rc = GXGnmDepthRenderTarget_InitializeForSurfaceMipSlice_Impl(
        outDrt, hwTex, viewFormat, mipLevel, firstSlice, sliceCount);
    if (rc == 0 && gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectTargets)) {
        gx_direct_vk::RegisterDirectDepthTarget(
            outDrt, nullptr, mipLevel, firstSlice, sliceCount);
    }
    return rc;
}

// [VMA: 0x216e1d0 - 0x216fa58] (6281 bytes)
// Initializes a sce::Gnm::RenderTarget for a specific mip level and array/depth slice range of a 2D, 3D, or 1D color texture, computing base, CMASK, and FMASK addresses.
static int32_t GXGnmRenderTarget_InitializeForSurfaceMipSlice_Impl(
    void* outRt,
    const GXGnmTextureHardwareObject* hwTex,
    uint32_t viewFormat,
    uint32_t mipLevel,
    uint32_t firstSlice,
    uint32_t sliceCount)
{
    struct GpuSurfaceInfo {
        uint32_t pitch;          // +0x00
        uint32_t height;         // +0x04
        uint32_t depth;          // +0x08
        uint32_t pad_0c;         // +0x0c
        uint64_t surfSize;       // +0x10
        uint64_t pad_18;         // +0x18
        uint64_t pad_20;         // +0x20
        uint32_t baseAlign;      // +0x28
        uint32_t bpp;            // +0x2c
        uint32_t arrayModeOrTile;// +0x30
        uint32_t pad_34;         // +0x34
    };

    if (hwTex->dimType == 3u) {
        // 2D RenderTarget initialization
        GXTexture2DDesc desc2d;
        desc2d.width         = hwTex->width;
        desc2d.height        = hwTex->height;
        desc2d.mipLevels     = hwTex->mipLevels;
        desc2d.arraySize     = hwTex->depthOrArray;
        desc2d.format        = viewFormat;
        desc2d.sampleCount   = hwTex->sampleCount;
        desc2d.sampleQuality = hwTex->sampleQuality;
        desc2d.usageMode     = hwTex->usageMode;
        desc2d.bindFlags     = hwTex->bindFlags;
        desc2d.cpuAccess     = hwTex->cpuAccess;
        desc2d.miscFlags     = hwTex->miscFlags;

        GXSurfaceFormatInfo2D surf2d;
        GXGnmTexture2D_ValidateAndComputeFormatInfo(&surf2d, &desc2d, &hwTex->tileMode, 0u);
        if (surf2d.status != 0) {
            return surf2d.status;
        }
        if ((surf2d.bindFlags & 0x20u) == 0) {
            return 8;
        }

        uintptr_t gpuBase = reinterpret_cast<uintptr_t>(hwTex->gpuBaseAddr);
        uint32_t isEqaa = (surf2d.miscFlags >> 10) & 1u;

        // Fast path for MSAA + EQAA (sampleCount >= 2 && isEqaa)
        if (isEqaa != 0 && surf2d.sampleCount >= 2u) {
            uint64_t cmaskSizeAlign = 0;
            uint64_t fmaskSizeAlign = 0;
            uint32_t numSamplesLog2 = CGFormatGnm_ResolveNumSamplesLog2(surf2d.sampleCount, 1u);
            uint32_t numFrags = CGFormatGnm_ResolveNumFragments(surf2d.sampleCount, surf2d.sampleQuality, 1u);
            uint64_t rtSa = SceGnmRenderTarget_Init(
                outRt, surf2d.width, surf2d.height, surf2d.arraySize, surf2d.gnmDataFormat,
                surf2d.tileMode, numSamplesLog2, numFrags, &cmaskSizeAlign, &fmaskSizeAlign);
            SceGnmRenderTarget_SetSliceViewRange(outRt, firstSlice, firstSlice + sliceCount - 1u);
            SceGnmRenderTarget_SetBaseAddress256ByteBlocks(outRt, static_cast<uint32_t>(gpuBase >> 8));

            uint32_t cmaskAlign = static_cast<uint32_t>(cmaskSizeAlign >> 32);
            uint32_t cmaskSize  = static_cast<uint32_t>(cmaskSizeAlign);
            uint32_t rtBaseSize = static_cast<uint32_t>(rtSa);
            uint32_t cmaskOffset = (rtBaseSize + cmaskAlign - 1u) & (~cmaskAlign + 1u);
            SceGnmRenderTarget_SetCmaskAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + cmaskOffset) >> 8));
            SceGnmRenderTarget_SetCmaskFastClearEnable(outRt, 1u);

            uint32_t afterCmask = rtBaseSize;
            if (cmaskAlign != 0) {
                afterCmask = cmaskOffset + cmaskSize;
            }
            uint32_t fmaskAlign = static_cast<uint32_t>(fmaskSizeAlign >> 32);
            uint32_t fmaskOffset = (afterCmask + fmaskAlign - 1u) & (~fmaskAlign + 1u);
            SceGnmRenderTarget_SetFmaskAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + fmaskOffset) >> 8));
            SceGnmRenderTarget_SetFmaskCompressionEnable(outRt, 1u);
            return 0;
        }

        uint32_t hasCmask = isEqaa ? 1u : ((surf2d.miscFlags & 0x200u) >> 9);
        GpuSurfaceInfo surfInfo;
        std::memset(&surfInfo, 0, sizeof(surfInfo));

        uint64_t gnmTex[4] = {0, 0, 0, 0};
        uint32_t numSlices = surf2d.arraySize;
        uint32_t isCubemap = 0u;
        if (surf2d.isArrayOrCube != 0 && (surf2d.miscFlags & 4u) != 0) {
            numSlices /= 6u;
            isCubemap = 1u;
        }
        uint32_t numFrags = CGFormatGnm_ResolveNumFragments(
            surf2d.sampleCount, surf2d.sampleQuality, (surf2d.miscFlags & 0x400u) ? 1u : 0u);
        uint32_t mipCount = surf2d.mipLevels;
        if (mipCount == 0) {
            uint32_t lzW = surf2d.width ? __builtin_clz(surf2d.width) : 32u;
            uint32_t lzH = surf2d.height ? __builtin_clz(surf2d.height) : 32u;
            uint32_t minLz = (lzH < 31u) ? lzH : 31u;
            if (lzW < minLz) minLz = lzW;
            mipCount = 32u - minLz;
        }

        SceGnmTexture_Init2DAndReturnSizeAlign(
            gnmTex, surf2d.width, surf2d.height, numSlices, mipCount,
            surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);

        uint8_t tilingParams[0x30];
        SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mipLevel, 0u);
        int32_t addrErr = SceGpuAddress_ComputeSurfaceInfo(&surfInfo, tilingParams);
        if (addrErr != 0) {
            CGErrorGnm_ReportGpuAddrError(addrErr);
            return 5;
        }

        uint64_t cmaskSizeAlign = 0;
        void* cmaskSaPtr = hasCmask ? &cmaskSizeAlign : nullptr;
        uint32_t numSamplesLog2 = CGFormatGnm_ResolveNumSamplesLog2(surf2d.sampleCount, isEqaa);
        uint32_t rtFrags = CGFormatGnm_ResolveNumFragments(surf2d.sampleCount, surf2d.sampleQuality, isEqaa);
        uint32_t mipW = surf2d.width >> mipLevel;
        if (mipW == 0) mipW = 1u;
        uint32_t mipH = surf2d.height >> mipLevel;
        if (mipH == 0) mipH = 1u;

        SceGnmRenderTarget_Init(
            outRt, mipW, mipH, surf2d.arraySize, surf2d.gnmDataFormat,
            surfInfo.arrayModeOrTile, numSamplesLog2, rtFrags, cmaskSaPtr, nullptr);

        uint32_t fragCnt = CGFormatGnm_FragmentLog2ToCount(rtFrags);
        uint64_t sliceElements = (surfInfo.surfSize / (surfInfo.bpp >> 3)) / fragCnt;
        SceGnmRenderTarget_SetPitchMinusOne(outRt, (surfInfo.pitch >> 3) - 1u);
        SceGnmRenderTarget_SetSliceSizeMinusOne(outRt, static_cast<uint32_t>((sliceElements >> 6) - 1u));
        SceGnmRenderTarget_SetSliceViewRange(outRt, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t colorMipOffset = 0;
        if (mipLevel != 0) {
            uint64_t tempTex[4] = {0, 0, 0, 0};
            SceGnmTexture_Init2DAndReturnSizeAlign(
                tempTex, surf2d.width, surf2d.height, numSlices, mipCount,
                surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);
            uint64_t mipSz = 0;
            int32_t mErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&colorMipOffset, &mipSz, tempTex, mipLevel, 0u);
            if (mErr != 0) {
                CGErrorGnm_ReportGpuAddrError(mErr);
                return 5;
            }
        }

        SceGnmRenderTarget_SetBaseAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + colorMipOffset) >> 8));
        if (!hasCmask) {
            return 0;
        }

        uint64_t texCmaskBase[4] = {0, 0, 0, 0};
        uint64_t colorTotalSa = SceGnmTexture_Init2DAndReturnSizeAlign(
            texCmaskBase, surf2d.width, surf2d.height, numSlices, mipCount,
            surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);

        uint64_t texIter[4] = {0, 0, 0, 0};
        SceGnmTexture_Init2DAndReturnSizeAlign(
            texIter, surf2d.width, surf2d.height, numSlices, mipCount,
            surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCubemap);

        uint64_t cmaskAccumOffset = 0;
        uint32_t maxCmaskAlign = 0;
        for (uint32_t m = 0; m < mipLevel; ++m) {
            uint8_t tp[0x30];
            GpuSurfaceInfo si;
            std::memset(&si, 0, sizeof(si));
            SceGpuAddress_InitTilingParametersFromTexture(tp, texIter, m, 0u);
            int32_t e1 = SceGpuAddress_ComputeSurfaceInfo(&si, tp);
            if (e1 != 0) {
                CGErrorGnm_ReportGpuAddrError(e1);
                return 5;
            }
            uint32_t cmSize = 0, cmAlign = 0, cmPitch = 0, cmHeight = 0;
            int32_t e2 = SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(
                &cmSize, &cmAlign, &cmPitch, &cmHeight, si.pitch, si.height, si.depth, 0u);
            if (e2 != 0) {
                CGErrorGnm_ReportGpuAddrError(e2);
                return 5;
            }
            if (cmAlign != 0) {
                uint32_t alignedPrev = (static_cast<uint32_t>(cmaskAccumOffset) + cmAlign - 1u) & (~cmAlign + 1u);
                cmaskAccumOffset = alignedPrev + cmSize * surf2d.arraySize;
            }
            if (cmAlign > maxCmaskAlign) {
                maxCmaskAlign = cmAlign;
            }
        }

        uint8_t tpTarget[0x30];
        GpuSurfaceInfo siTarget;
        std::memset(&siTarget, 0, sizeof(siTarget));
        SceGpuAddress_InitTilingParametersFromTexture(tpTarget, texCmaskBase, mipLevel, 0u);
        int32_t eT1 = SceGpuAddress_ComputeSurfaceInfo(&siTarget, tpTarget);
        if (eT1 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT1);
            return 5;
        }
        uint32_t curCmSize = 0, curCmAlign = 0, curCmPitch = 0, curCmHeight = 0;
        int32_t eT2 = SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(
            &curCmSize, &curCmAlign, &curCmPitch, &curCmHeight,
            siTarget.pitch, siTarget.height, siTarget.depth, 0u);
        if (eT2 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT2);
            return 5;
        }

        if (curCmAlign != 0) {
            cmaskAccumOffset = (static_cast<uint32_t>(cmaskAccumOffset) + curCmAlign - 1u) & (~curCmAlign + 1u);
        }
        uint32_t finalCmAlign = (maxCmaskAlign > curCmAlign) ? maxCmaskAlign : curCmAlign;
        uint32_t cmaskFinalOffset = static_cast<uint32_t>(colorTotalSa);
        if (finalCmAlign != 0) {
            cmaskFinalOffset = ((cmaskFinalOffset + finalCmAlign - 1u) & (~finalCmAlign + 1u)) +
                               static_cast<uint32_t>(cmaskAccumOffset);
        }

        SceGnmRenderTarget_SetCmaskAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + cmaskFinalOffset) >> 8));
        SceGnmRenderTarget_SetCmaskFastClearEnable(outRt, 1u);
        SceGnmRenderTarget_SetFmaskCompressionEnable(outRt, 0u);
        SceGnmRenderTarget_SetFmaskTileMode(outRt, SceGnmRenderTarget_GetFmaskTileMode(outRt));
        SceGnmRenderTarget_SetFmaskSliceSizeMinusOne(outRt, SceGnmRenderTarget_GetFmaskSliceSizeMinusOne(outRt));
        SceGnmRenderTarget_SetFmaskAddress256ByteBlocks(outRt, SceGnmRenderTarget_GetBaseAddress256ByteBlocks(outRt));
        return 0;
    }

    if (hwTex->dimType == 4u) {
        // 3D RenderTarget initialization
        GXTexture3DDesc desc3d;
        desc3d.width     = hwTex->width;
        desc3d.height    = hwTex->height;
        desc3d.depth     = hwTex->depthOrArray;
        desc3d.mipLevels = hwTex->mipLevels;
        desc3d.format    = viewFormat;
        desc3d.usageMode = hwTex->usageMode;
        desc3d.bindFlags = hwTex->bindFlags;
        desc3d.cpuAccess = hwTex->cpuAccess;
        desc3d.miscFlags = hwTex->miscFlags;

        GXSurfaceFormatInfo3D surf3d;
        GXGnmTexture3D_ValidateAndComputeFormatInfo(&surf3d, &desc3d, &hwTex->tileMode, 0u);
        if (surf3d.status != 0) {
            return surf3d.status;
        }
        if ((surf3d.bindFlags & 0x20u) == 0) {
            return 8;
        }

        uintptr_t gpuBase = reinterpret_cast<uintptr_t>(hwTex->gpuBaseAddr);
        uint32_t isEqaa = (surf3d.miscFlags >> 10) & 1u;
        uint8_t hasCmask = isEqaa ? 1u : static_cast<uint8_t>((surf3d.miscFlags >> 9) & 1u);

        GpuSurfaceInfo surfInfo;
        std::memset(&surfInfo, 0, sizeof(surfInfo));

        uint64_t gnmTex[4] = {0, 0, 0, 0};
        uint32_t mipCount = surf3d.mipLevels;
        if (mipCount == 0) {
            uint32_t lzW = surf3d.width ? __builtin_clz(surf3d.width) : 32u;
            uint32_t lzH = surf3d.height ? __builtin_clz(surf3d.height) : 32u;
            uint32_t lzD = surf3d.depth ? __builtin_clz(surf3d.depth) : 32u;
            uint32_t minLz = (lzH < lzD) ? lzH : lzD;
            if (lzW < minLz) minLz = lzW;
            mipCount = 32u - minLz;
        }

        SceGnmTexture_Init3DAndReturnSizeAlign(
            gnmTex, surf3d.width, surf3d.height, surf3d.depth, mipCount,
            surf3d.gnmDataFormat, surf3d.tileMode);

        uint8_t tilingParams[0x30];
        SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mipLevel, 0u);
        int32_t addrErr = SceGpuAddress_ComputeSurfaceInfo(&surfInfo, tilingParams);
        if (addrErr != 0) {
            CGErrorGnm_ReportGpuAddrError(addrErr);
            return 5;
        }

        uint64_t cmaskSizeAlign = 0;
        void* cmaskSaPtr = hasCmask ? &cmaskSizeAlign : nullptr;
        uint32_t numSamplesLog2 = CGFormatGnm_ResolveNumSamplesLog2(1u, isEqaa);
        uint32_t rtFrags = CGFormatGnm_ResolveNumFragments(1u, 0u, isEqaa);
        uint32_t mipW = surf3d.width >> mipLevel;
        if (mipW == 0) mipW = 1u;
        uint32_t mipH = surf3d.height >> mipLevel;
        if (mipH == 0) mipH = 1u;

        SceGnmRenderTarget_Init(
            outRt, mipW, mipH, surf3d.depth, surf3d.gnmDataFormat,
            surfInfo.arrayModeOrTile, numSamplesLog2, rtFrags, cmaskSaPtr, nullptr);

        uint32_t fragCnt = CGFormatGnm_FragmentLog2ToCount(rtFrags);
        uint64_t sliceElements = (surfInfo.surfSize / (surfInfo.bpp >> 3)) / fragCnt;
        SceGnmRenderTarget_SetPitchMinusOne(outRt, (surfInfo.pitch >> 3) - 1u);
        SceGnmRenderTarget_SetSliceSizeMinusOne(outRt, static_cast<uint32_t>((sliceElements >> 6) - 1u));
        SceGnmRenderTarget_SetSliceViewRange(outRt, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t colorMipOffset = 0;
        if (mipLevel != 0) {
            uint64_t tempTex[4] = {0, 0, 0, 0};
            SceGnmTexture_Init3DAndReturnSizeAlign(
                tempTex, surf3d.width, surf3d.height, surf3d.depth, mipCount,
                surf3d.gnmDataFormat, surf3d.tileMode);
            uint64_t mipSz = 0;
            int32_t mErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&colorMipOffset, &mipSz, tempTex, mipLevel, 0u);
            if (mErr != 0) {
                CGErrorGnm_ReportGpuAddrError(mErr);
                return 5;
            }
        }

        SceGnmRenderTarget_SetBaseAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + colorMipOffset) >> 8));
        if (!hasCmask) {
            return 0;
        }

        uint64_t texCmaskBase[4] = {0, 0, 0, 0};
        uint64_t colorTotalSa = SceGnmTexture_Init3DAndReturnSizeAlign(
            texCmaskBase, surf3d.width, surf3d.height, surf3d.depth, mipCount,
            surf3d.gnmDataFormat, surf3d.tileMode);

        uint64_t texIter[4] = {0, 0, 0, 0};
        SceGnmTexture_Init3DAndReturnSizeAlign(
            texIter, surf3d.width, surf3d.height, surf3d.depth, mipCount,
            surf3d.gnmDataFormat, surf3d.tileMode);

        uint64_t cmaskAccumOffset = 0;
        uint32_t maxCmaskAlign = 0;
        for (uint32_t m = 0; m < mipLevel; ++m) {
            uint8_t tp[0x30];
            GpuSurfaceInfo si;
            std::memset(&si, 0, sizeof(si));
            SceGpuAddress_InitTilingParametersFromTexture(tp, texIter, m, 0u);
            int32_t e1 = SceGpuAddress_ComputeSurfaceInfo(&si, tp);
            if (e1 != 0) {
                CGErrorGnm_ReportGpuAddrError(e1);
                return 5;
            }
            uint32_t cmSize = 0, cmAlign = 0, cmPitch = 0, cmHeight = 0;
            int32_t e2 = SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(
                &cmSize, &cmAlign, &cmPitch, &cmHeight, si.pitch, si.height, si.depth, 0u);
            if (e2 != 0) {
                CGErrorGnm_ReportGpuAddrError(e2);
                return 5;
            }
            if (cmAlign != 0) {
                uint32_t alignedPrev = (static_cast<uint32_t>(cmaskAccumOffset) + cmAlign - 1u) & (~cmAlign + 1u);
                cmaskAccumOffset = alignedPrev + cmSize;
            }
            if (cmAlign > maxCmaskAlign) {
                maxCmaskAlign = cmAlign;
            }
        }

        uint8_t tpTarget[0x30];
        GpuSurfaceInfo siTarget;
        std::memset(&siTarget, 0, sizeof(siTarget));
        SceGpuAddress_InitTilingParametersFromTexture(tpTarget, texCmaskBase, mipLevel, 0u);
        int32_t eT1 = SceGpuAddress_ComputeSurfaceInfo(&siTarget, tpTarget);
        if (eT1 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT1);
            return 5;
        }
        uint32_t curCmSize = 0, curCmAlign = 0, curCmPitch = 0, curCmHeight = 0;
        int32_t eT2 = SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(
            &curCmSize, &curCmAlign, &curCmPitch, &curCmHeight,
            siTarget.pitch, siTarget.height, siTarget.depth, 0u);
        if (eT2 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT2);
            return 5;
        }

        uint32_t alignedAccum = static_cast<uint32_t>(cmaskAccumOffset);
        if (curCmAlign != 0) {
            alignedAccum = (alignedAccum + curCmAlign - 1u) & (~curCmAlign + 1u);
        }
        uint32_t finalCmAlign = (maxCmaskAlign > curCmAlign) ? maxCmaskAlign : curCmAlign;
        uint32_t cmaskFinalOffset = static_cast<uint32_t>(colorTotalSa);
        if (finalCmAlign != 0) {
            cmaskFinalOffset = ((cmaskFinalOffset + finalCmAlign - 1u) & (~finalCmAlign + 1u)) + alignedAccum;
        }

        SceGnmRenderTarget_SetCmaskAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + cmaskFinalOffset) >> 8));
        SceGnmRenderTarget_SetCmaskFastClearEnable(outRt, 1u);
        SceGnmRenderTarget_SetFmaskCompressionEnable(outRt, 0u);
        SceGnmRenderTarget_SetFmaskTileMode(outRt, SceGnmRenderTarget_GetFmaskTileMode(outRt));
        SceGnmRenderTarget_SetFmaskSliceSizeMinusOne(outRt, SceGnmRenderTarget_GetFmaskSliceSizeMinusOne(outRt));
        SceGnmRenderTarget_SetFmaskAddress256ByteBlocks(outRt, SceGnmRenderTarget_GetBaseAddress256ByteBlocks(outRt));
        return 0;
    }

    if (hwTex->dimType == 2u) {
        // 1D RenderTarget initialization
        GXTexture1DDesc desc1d;
        desc1d.width     = hwTex->width;
        desc1d.mipLevels = hwTex->mipLevels;
        desc1d.arraySize = hwTex->depthOrArray;
        desc1d.format    = viewFormat;
        desc1d.usageMode = hwTex->usageMode;
        desc1d.bindFlags = hwTex->bindFlags;
        desc1d.cpuAccess = hwTex->cpuAccess;
        desc1d.miscFlags = hwTex->miscFlags;

        GXSurfaceFormatInfo1D surf1d;
        GXGnmTexture1D_ValidateAndComputeFormatInfo(&surf1d, &desc1d, &hwTex->tileMode, 0u);
        if (surf1d.status != 0) {
            return surf1d.status;
        }
        if ((surf1d.bindFlags & 0x20u) == 0) {
            return 8;
        }

        uintptr_t gpuBase = reinterpret_cast<uintptr_t>(hwTex->gpuBaseAddr);
        uint32_t isEqaa = (surf1d.miscFlags >> 10) & 1u;
        uint8_t hasCmask = isEqaa ? 1u : static_cast<uint8_t>((surf1d.miscFlags >> 9) & 1u);

        GpuSurfaceInfo surfInfo;
        std::memset(&surfInfo, 0, sizeof(surfInfo));

        uint64_t gnmTex[4] = {0, 0, 0, 0};
        uint32_t mipCount = surf1d.mipLevels;
        if (mipCount == 0) {
            uint32_t lzW = surf1d.width ? __builtin_clz(surf1d.width) : 32u;
            mipCount = (lzW < 31u) ? (32u - lzW) : 1u;
        }

        SceGnmTexture_Init2DAndReturnSizeAlign(
            gnmTex, surf1d.width, 1u, surf1d.arraySize, mipCount,
            surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(gnmTex, 12u);

        uint8_t tilingParams[0x30];
        SceGpuAddress_InitTilingParametersFromTexture(tilingParams, gnmTex, mipLevel, 0u);
        int32_t addrErr = SceGpuAddress_ComputeSurfaceInfo(&surfInfo, tilingParams);
        if (addrErr != 0) {
            CGErrorGnm_ReportGpuAddrError(addrErr);
            return 5;
        }

        uint64_t cmaskSizeAlign = 0;
        void* cmaskSaPtr = hasCmask ? &cmaskSizeAlign : nullptr;
        uint32_t numSamplesLog2 = CGFormatGnm_ResolveNumSamplesLog2(1u, isEqaa);
        uint32_t rtFrags = CGFormatGnm_ResolveNumFragments(1u, 0u, isEqaa);
        uint32_t mipW = surf1d.width >> mipLevel;
        if (mipW == 0) mipW = 1u;
        uint32_t mipH = 1u >> mipLevel;
        if (mipH == 0) mipH = 1u;

        SceGnmRenderTarget_Init(
            outRt, mipW, mipH, surf1d.arraySize, surf1d.gnmDataFormat,
            surfInfo.arrayModeOrTile, numSamplesLog2, rtFrags, cmaskSaPtr, nullptr);

        uint32_t fragCnt = CGFormatGnm_FragmentLog2ToCount(rtFrags);
        uint64_t sliceElements = (surfInfo.surfSize / (surfInfo.bpp >> 3)) / fragCnt;
        SceGnmRenderTarget_SetPitchMinusOne(outRt, (surfInfo.pitch >> 3) - 1u);
        SceGnmRenderTarget_SetSliceSizeMinusOne(outRt, static_cast<uint32_t>((sliceElements >> 6) - 1u));
        SceGnmRenderTarget_SetSliceViewRange(outRt, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t colorMipOffset = 0;
        if (mipLevel != 0) {
            uint64_t tempTex[4] = {0, 0, 0, 0};
            SceGnmTexture_Init2DAndReturnSizeAlign(
                tempTex, surf1d.width, 1u, surf1d.arraySize, mipCount,
                surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
            SceGnmTexture_SetResourceMemoryType(tempTex, 12u);
            uint64_t mipSz = 0;
            int32_t mErr = SceGpuAddress_ComputeTextureSurfaceOffsetAndSize(&colorMipOffset, &mipSz, tempTex, mipLevel, 0u);
            if (mErr != 0) {
                CGErrorGnm_ReportGpuAddrError(mErr);
                return 5;
            }
        }

        SceGnmRenderTarget_SetBaseAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + colorMipOffset) >> 8));
        if (!hasCmask) {
            return 0;
        }

        uint64_t texCmaskBase[4] = {0, 0, 0, 0};
        uint64_t colorTotalSa = SceGnmTexture_Init2DAndReturnSizeAlign(
            texCmaskBase, surf1d.width, 1u, surf1d.arraySize, mipCount,
            surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(texCmaskBase, 12u);

        uint64_t texIter[4] = {0, 0, 0, 0};
        SceGnmTexture_Init2DAndReturnSizeAlign(
            texIter, surf1d.width, 1u, surf1d.arraySize, mipCount,
            surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
        SceGnmTexture_SetResourceMemoryType(texIter, 12u);

        uint64_t cmaskAccumOffset = 0;
        uint32_t maxCmaskAlign = 0;
        for (uint32_t m = 0; m < mipLevel; ++m) {
            uint8_t tp[0x30];
            GpuSurfaceInfo si;
            std::memset(&si, 0, sizeof(si));
            SceGpuAddress_InitTilingParametersFromTexture(tp, texIter, m, 0u);
            int32_t e1 = SceGpuAddress_ComputeSurfaceInfo(&si, tp);
            if (e1 != 0) {
                CGErrorGnm_ReportGpuAddrError(e1);
                return 5;
            }
            uint32_t cmSize = 0, cmAlign = 0, cmPitch = 0, cmHeight = 0;
            int32_t e2 = SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(
                &cmSize, &cmAlign, &cmPitch, &cmHeight, si.pitch, si.height, si.depth, 0u);
            if (e2 != 0) {
                CGErrorGnm_ReportGpuAddrError(e2);
                return 5;
            }
            if (cmAlign != 0) {
                uint32_t alignedPrev = (static_cast<uint32_t>(cmaskAccumOffset) + cmAlign - 1u) & (~cmAlign + 1u);
                cmaskAccumOffset = alignedPrev + cmSize * surf1d.arraySize;
            }
            if (cmAlign > maxCmaskAlign) {
                maxCmaskAlign = cmAlign;
            }
        }

        uint8_t tpTarget[0x30];
        GpuSurfaceInfo siTarget;
        std::memset(&siTarget, 0, sizeof(siTarget));
        SceGpuAddress_InitTilingParametersFromTexture(tpTarget, texCmaskBase, mipLevel, 0u);
        int32_t eT1 = SceGpuAddress_ComputeSurfaceInfo(&siTarget, tpTarget);
        if (eT1 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT1);
            return 5;
        }
        uint32_t curCmSize = 0, curCmAlign = 0, curCmPitch = 0, curCmHeight = 0;
        int32_t eT2 = SceGpuAddress_ComputeCmaskSurfaceSizeAndAlignment(
            &curCmSize, &curCmAlign, &curCmPitch, &curCmHeight,
            siTarget.pitch, siTarget.height, siTarget.depth, 0u);
        if (eT2 != 0) {
            CGErrorGnm_ReportGpuAddrError(eT2);
            return 5;
        }

        if (curCmAlign != 0) {
            cmaskAccumOffset = (static_cast<uint32_t>(cmaskAccumOffset) + curCmAlign - 1u) & (~curCmAlign + 1u);
        }
        uint32_t finalCmAlign = (maxCmaskAlign > curCmAlign) ? maxCmaskAlign : curCmAlign;
        uint32_t cmaskFinalOffset = static_cast<uint32_t>(colorTotalSa);
        if (finalCmAlign != 0) {
            cmaskFinalOffset = ((cmaskFinalOffset + finalCmAlign - 1u) & (~finalCmAlign + 1u)) +
                               static_cast<uint32_t>(cmaskAccumOffset);
        }

        SceGnmRenderTarget_SetCmaskAddress256ByteBlocks(outRt, static_cast<uint32_t>((gpuBase + cmaskFinalOffset) >> 8));
        SceGnmRenderTarget_SetCmaskFastClearEnable(outRt, 1u);
        SceGnmRenderTarget_SetFmaskCompressionEnable(outRt, 0u);
        SceGnmRenderTarget_SetFmaskTileMode(outRt, SceGnmRenderTarget_GetFmaskTileMode(outRt));
        SceGnmRenderTarget_SetFmaskSliceSizeMinusOne(outRt, SceGnmRenderTarget_GetFmaskSliceSizeMinusOne(outRt));
        SceGnmRenderTarget_SetFmaskAddress256ByteBlocks(outRt, SceGnmRenderTarget_GetBaseAddress256ByteBlocks(outRt));
        return 0;
    }

    return 8;
}

extern "C" int32_t GXGnmRenderTarget_InitializeForSurfaceMipSlice(
    void* outRt,
    const GXGnmTextureHardwareObject* hwTex,
    uint32_t viewFormat,
    uint32_t mipLevel,
    uint32_t firstSlice,
    uint32_t sliceCount)
{
    const int32_t rc = GXGnmRenderTarget_InitializeForSurfaceMipSlice_Impl(
        outRt, hwTex, viewFormat, mipLevel, firstSlice, sliceCount);
    if (rc == 0 && gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectTargets)) {
        uint64_t parentTsharp[4] = {0, 0, 0, 0};
        const void* parentPtr = nullptr;
        if (hwTex && hwTex->sampleCount <= 1u &&
            (hwTex->mipLevels != 1u || sliceCount < hwTex->depthOrArray)) {
            uint32_t totalMips = hwTex->mipLevels;
            if (totalMips == 0u) {
                uint32_t maxDim = (hwTex->width > hwTex->height) ? hwTex->width : hwTex->height;
                if (hwTex->dimType == 4u && hwTex->depthOrArray > maxDim) {
                    maxDim = hwTex->depthOrArray;
                }
                totalMips = maxDim ? (32u - static_cast<uint32_t>(__builtin_clz(maxDim))) : 1u;
            }
            const uint8_t isArrOrCube =
                (hwTex->dimType == 3u && (hwTex->depthOrArray > 1u || (hwTex->miscFlags & 4u) != 0u))
                    ? 1u
                    : 0u;
            if (GXGnmShaderResourceView_InitializeForSurfaceMipSlice(
                    parentTsharp, hwTex, viewFormat, 0u, 0u,
                    totalMips, hwTex->depthOrArray, isArrOrCube) == 0) {
                parentPtr = parentTsharp;
            }
        }
        gx_direct_vk::RegisterDirectColorTarget(
            outRt, parentPtr, mipLevel, firstSlice, sliceCount);
    }
    return rc;
}

// [VMA: 0x216e1c0 - 0x216e1cc] (13 bytes)
// Returns the GPU address of the 8-byte append/consume counter at the end of a buffer allocation (gpuBaseAddr + allocSize - 8).
extern "C" void* GXGnmBuffer_GetAppendConsumeCounterAddress(const GXGnmTextureHardwareObject* hwBuf) {
    return static_cast<uint8_t*>(hwBuf->gpuBaseAddr) + hwBuf->allocSize - 8;
}

// [VMA: 0x216e1b0 - 0x216e1bb] (12 bytes)
// Returns the 24-bit resource handle shifted to bits [63:40] ORed with the 40-bit GPU base address.
extern "C" uint64_t GXGnmBuffer_GetPackedHandleAndAddress(const GXGnmTextureHardwareObject* hwBuf) {
    return (static_cast<uint64_t>(hwBuf->resourceId) << 40) | reinterpret_cast<uintptr_t>(hwBuf->gpuBaseAddr);
}

// [VMA: 0x216e1a0 - 0x216e1a4] (5 bytes)
// Returns the raw GPU base address of a buffer or texture hardware object (+0x08).
extern "C" void* GXGnmBuffer_GetGpuBaseAddress(const GXGnmTextureHardwareObject* hwBuf) {
    return hwBuf->gpuBaseAddr;
}

// [VMA: 0x216d900 - 0x216e198] (2201 bytes)
// Initializes a 32-byte sce::Gnm::Texture T# Shader Resource View for a specific mip range and slice range of a 2D, 3D, or 1D texture (supporting color, stencil-only, and MSAA FMASK views).
extern "C" int32_t GXGnmShaderResourceView_InitializeForSurfaceMipSlice(
    void* outGnmTex,
    const GXGnmTextureHardwareObject* hwTex,
    uint32_t viewFormat,
    uint32_t firstMip,
    uint32_t firstSlice,
    uint32_t mipCount,
    uint32_t sliceCount,
    uint8_t isArrayOrCube)
{
    if (hwTex->dimType == 3u) {
        // 2D / 2DArray / Cubemap SRV T# initialization
        GXTexture2DDesc desc2d;
        desc2d.width         = hwTex->width;
        desc2d.height        = hwTex->height;
        desc2d.mipLevels     = hwTex->mipLevels;
        desc2d.arraySize     = hwTex->depthOrArray;
        desc2d.format        = viewFormat;
        desc2d.sampleCount   = hwTex->sampleCount;
        desc2d.sampleQuality = hwTex->sampleQuality;
        desc2d.usageMode     = hwTex->usageMode;
        desc2d.bindFlags     = hwTex->bindFlags;
        desc2d.cpuAccess     = hwTex->cpuAccess;
        desc2d.miscFlags     = hwTex->miscFlags;

        GXSurfaceFormatInfo2D surf2d;
        GXGnmTexture2D_ValidateAndComputeFormatInfo(&surf2d, &desc2d, &hwTex->tileMode, isArrayOrCube);
        if (surf2d.status != 0) {
            return surf2d.status;
        }

        uintptr_t gpuBase = reinterpret_cast<uintptr_t>(hwTex->gpuBaseAddr);
        uint32_t sampleCount = surf2d.sampleCount;
        uint64_t surfOffset = 0;

        if ((surf2d.bindFlags & 0x20u) != 0 && (surf2d.miscFlags & 0x400u) != 0 && sampleCount >= 2u) {
            // MSAA EQAA RenderTarget FMASK SRV path
            uint8_t tempRt[0x40];
            std::memset(tempRt, 0, sizeof(tempRt));
            uint64_t cmaskSa = 0;
            uint64_t fmaskSa = 0;
            uint32_t numSamplesLog2 = CGFormatGnm_ResolveNumSamplesLog2(sampleCount, 1u);
            uint32_t numFrags = CGFormatGnm_ResolveNumFragments(sampleCount, surf2d.sampleQuality, 1u);
            SceGnmRenderTarget_Init(
                tempRt, surf2d.width, surf2d.height, surf2d.arraySize, surf2d.gnmDataFormat,
                surf2d.tileMode, numSamplesLog2, numFrags, &cmaskSa, &fmaskSa);
            SceGnmTexture_InitFromRenderTargetFmask(outGnmTex, tempRt, (surf2d.miscFlags & 4u) >> 2);
        } else if (CGFormatGnm_IsStencilOnlyViewFormat(surf2d.bindFlags, surf2d.format)) {
            GXGnmTexture2D_InitGnmTextureFromSurfaceInfo(&surf2d, outGnmTex);
            if (GXGnmTexture2D_ComputeMipSurfaceSizeAndAlignment(&surfOffset, &surf2d, 0u) != 0) {
                return surf2d.status;
            }
        } else {
            uint32_t numSlices = surf2d.arraySize;
            uint32_t isCube = 0u;
            if (surf2d.isArrayOrCube != 0 && (surf2d.miscFlags & 4u) != 0) {
                numSlices /= 6u;
                isCube = 1u;
            }
            uint32_t numFrags = CGFormatGnm_ResolveNumFragments(
                surf2d.sampleCount, surf2d.sampleQuality, (surf2d.miscFlags & 0x400u) ? 1u : 0u);
            uint32_t maxMips = surf2d.mipLevels;
            if (maxMips == 0) {
                uint32_t lzW = surf2d.width ? __builtin_clz(surf2d.width) : 32u;
                uint32_t lzH = surf2d.height ? __builtin_clz(surf2d.height) : 32u;
                uint32_t minLz = (lzH < 31u) ? lzH : 31u;
                if (lzW < minLz) minLz = lzW;
                maxMips = 32u - minLz;
            }
            SceGnmTexture_Init2DAndReturnSizeAlign(
                outGnmTex, surf2d.width, surf2d.height, numSlices, maxMips,
                surf2d.gnmDataFormat, surf2d.tileMode, numFrags, isCube);
        }

        SceGnmTexture_SetBaseArrayViewRange(outGnmTex, firstSlice, firstSlice + sliceCount - 1u);
        if (sampleCount <= 1u) {
            uint32_t totalMips = surf2d.mipLevels;
            if (totalMips == 0) {
                uint32_t lzW = surf2d.width ? __builtin_clz(surf2d.width) : 32u;
                uint32_t lzH = surf2d.height ? __builtin_clz(surf2d.height) : 32u;
                uint32_t minLz = (lzH < 31u) ? lzH : 31u;
                if (lzW < minLz) minLz = lzW;
                totalMips = 32u - minLz;
            }
            uint64_t reqLastMip = static_cast<uint64_t>(firstMip) + static_cast<uint64_t>(mipCount) - 1ULL;
            uint64_t maxMipIdx = static_cast<uint64_t>(totalMips);
            uint64_t clampedLastMip = reqLastMip;
            if (totalMips < firstMip) {
                if (firstMip < clampedLastMip) clampedLastMip = firstMip;
                if (maxMipIdx > clampedLastMip) clampedLastMip = maxMipIdx;
            } else {
                if (maxMipIdx < clampedLastMip) clampedLastMip = maxMipIdx;
                if (firstMip > clampedLastMip) clampedLastMip = firstMip;
            }
            SceGnmTexture_SetMipLevelViewRange(outGnmTex, firstMip, static_cast<uint32_t>(clampedLastMip));
        }

        SceGnmTexture_SetBaseAddress256ByteBlocks(outGnmTex, static_cast<uint32_t>((gpuBase + surfOffset) >> 8));
        return 0;
    }

    if (hwTex->dimType == 4u) {
        // 3D SRV T# initialization
        GXTexture3DDesc desc3d;
        desc3d.width     = hwTex->width;
        desc3d.height    = hwTex->height;
        desc3d.depth     = hwTex->depthOrArray;
        desc3d.mipLevels = hwTex->mipLevels;
        desc3d.format    = viewFormat;
        desc3d.usageMode = hwTex->usageMode;
        desc3d.bindFlags = hwTex->bindFlags;
        desc3d.cpuAccess = hwTex->cpuAccess;
        desc3d.miscFlags = hwTex->miscFlags;

        GXSurfaceFormatInfo3D surf3d;
        GXGnmTexture3D_ValidateAndComputeFormatInfo(&surf3d, &desc3d, &hwTex->tileMode, isArrayOrCube);
        if (surf3d.status != 0) {
            return surf3d.status;
        }

        uintptr_t gpuBase = reinterpret_cast<uintptr_t>(hwTex->gpuBaseAddr);
        uint32_t totalMips = surf3d.mipLevels;
        if (totalMips == 0) {
            uint32_t lzW = surf3d.width ? __builtin_clz(surf3d.width) : 32u;
            uint32_t lzH = surf3d.height ? __builtin_clz(surf3d.height) : 32u;
            uint32_t lzD = surf3d.depth ? __builtin_clz(surf3d.depth) : 32u;
            uint32_t minLz = (lzH < lzD) ? lzH : lzD;
            if (lzW < minLz) minLz = lzW;
            totalMips = 32u - minLz;
        }

        if (CGFormatGnm_IsStencilOnlyViewFormat(surf3d.bindFlags, surf3d.format)) {
            uint64_t dummyTex[4] = {0, 0, 0, 0};
            SceGnmTexture_Init3DAndReturnSizeAlign(
                dummyTex, surf3d.width, surf3d.height, surf3d.depth, totalMips,
                surf3d.gnmDataFormat, surf3d.tileMode);
        } else {
            SceGnmTexture_Init3DAndReturnSizeAlign(
                outGnmTex, surf3d.width, surf3d.height, surf3d.depth, totalMips,
                surf3d.gnmDataFormat, surf3d.tileMode);
        }

        SceGnmTexture_SetBaseArrayViewRange(outGnmTex, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t reqLastMip = static_cast<uint64_t>(firstMip) + static_cast<uint64_t>(mipCount) - 1ULL;
        uint64_t maxMipIdx = static_cast<uint64_t>(totalMips);
        uint64_t clampedLastMip = reqLastMip;
        if (totalMips < firstMip) {
            if (firstMip < clampedLastMip) clampedLastMip = firstMip;
            if (maxMipIdx > clampedLastMip) clampedLastMip = maxMipIdx;
        } else {
            if (maxMipIdx < clampedLastMip) clampedLastMip = maxMipIdx;
            if (firstMip > clampedLastMip) clampedLastMip = firstMip;
        }
        SceGnmTexture_SetMipLevelViewRange(outGnmTex, firstMip, static_cast<uint32_t>(clampedLastMip));
        SceGnmTexture_SetBaseAddress256ByteBlocks(outGnmTex, static_cast<uint32_t>(gpuBase >> 8));
        return 0;
    }

    if (hwTex->dimType == 2u) {
        // 1D / 1DArray SRV T# initialization
        GXTexture1DDesc desc1d;
        desc1d.width     = hwTex->width;
        desc1d.mipLevels = hwTex->mipLevels;
        desc1d.arraySize = hwTex->depthOrArray;
        desc1d.format    = viewFormat;
        desc1d.usageMode = hwTex->usageMode;
        desc1d.bindFlags = hwTex->bindFlags;
        desc1d.cpuAccess = hwTex->cpuAccess;
        desc1d.miscFlags = hwTex->miscFlags;

        GXSurfaceFormatInfo1D surf1d;
        GXGnmTexture1D_ValidateAndComputeFormatInfo(&surf1d, &desc1d, &hwTex->tileMode, isArrayOrCube);
        if (surf1d.status != 0) {
            return surf1d.status;
        }

        uintptr_t gpuBase = reinterpret_cast<uintptr_t>(hwTex->gpuBaseAddr);
        uint64_t surfOffset = 0;
        uint32_t totalMips = surf1d.mipLevels;
        if (totalMips == 0) {
            uint32_t lzW = surf1d.width ? __builtin_clz(surf1d.width) : 32u;
            totalMips = (lzW < 31u) ? (32u - lzW) : 1u;
        }

        if (CGFormatGnm_IsStencilOnlyViewFormat(surf1d.bindFlags, surf1d.format)) {
            if (CGFormatGnm_HasFmaskOrHtileFlag(surf1d.bindFlags, surf1d.format)) {
                SceGnmTexture_Init2DAndReturnSizeAlign(
                    outGnmTex, surf1d.width, 1u, surf1d.arraySize, totalMips,
                    0x20401u, surf1d.tileMode, 0u, 0u);
                SceGnmTexture_SetResourceMemoryType(outGnmTex, 12u);
            }
            if (GXGnmTexture1D_ComputeMipSurfaceSizeAndAlignment(&surfOffset, &surf1d, 0u) != 0) {
                return surf1d.status;
            }
        } else {
            SceGnmTexture_Init2DAndReturnSizeAlign(
                outGnmTex, surf1d.width, 1u, surf1d.arraySize, totalMips,
                surf1d.gnmDataFormat, surf1d.tileMode, 0u, 0u);
            SceGnmTexture_SetResourceMemoryType(outGnmTex, 12u);
        }

        SceGnmTexture_SetBaseArrayViewRange(outGnmTex, firstSlice, firstSlice + sliceCount - 1u);

        uint64_t reqLastMip = static_cast<uint64_t>(firstMip) + static_cast<uint64_t>(mipCount) - 1ULL;
        uint64_t maxMipIdx = static_cast<uint64_t>(totalMips);
        uint64_t clampedLastMip = reqLastMip;
        if (totalMips < firstMip) {
            if (firstMip < clampedLastMip) clampedLastMip = firstMip;
            if (maxMipIdx > clampedLastMip) clampedLastMip = maxMipIdx;
        } else {
            if (maxMipIdx < clampedLastMip) clampedLastMip = maxMipIdx;
            if (firstMip > clampedLastMip) clampedLastMip = firstMip;
        }
        SceGnmTexture_SetMipLevelViewRange(outGnmTex, firstMip, static_cast<uint32_t>(clampedLastMip));
        SceGnmTexture_SetBaseAddress256ByteBlocks(outGnmTex, static_cast<uint32_t>((gpuBase + surfOffset) >> 8));
        return 0;
    }

    return 8;
}


// ============================================================================
// Group 4: Buffer Creation, RefCounted Resource Wrappers & Fetch Shader Gen
// ============================================================================

struct GXMemoryAllocator;
struct GXMemoryAllocatorVTable {
    void* vfunc_00[10];
    void* (*Allocate)(GXMemoryAllocator* self, uint64_t size);                            // +0x50
    void* (*AllocateAligned)(GXMemoryAllocator* self, uint64_t size, uint32_t alignment); // +0x58
    void* vfunc_60;                                                                       // +0x60
    void* vfunc_68;                                                                       // +0x68
    void  (*FreeAligned)(GXMemoryAllocator* self, void* ptr);                             // +0x70
};

struct GXMemoryAllocator {
    const GXMemoryAllocatorVTable* vtable;
};

struct GXHardwareTextureResource {
    GXMemoryAllocator* allocator;    // +0x00
    void*              gpuBaseAddr;  // +0x08
    uint32_t           resourceId;   // +0x10
    uint32_t           rawByteSize;  // +0x14
    uint32_t           allocSize;    // +0x18
    uint32_t           alignment;    // +0x1c
    uint8_t            dimType;      // +0x20 (2=1D, 3=2D, 4=3D)
    uint8_t            usageMode;    // +0x21
    uint8_t            cpuAccess;    // +0x22
    uint8_t            bindFlags;    // +0x23
    uint32_t           miscFlags;    // +0x24
    uint32_t           tileMode;     // +0x28
    uint16_t           width;        // +0x2c
    uint16_t           height;       // +0x2e
    uint16_t           depthOrArray; // +0x30
    uint8_t            mipLevels;    // +0x32
    uint8_t            format;       // +0x33
    uint16_t           sampleCount;  // +0x34
    uint16_t           sampleQuality;// +0x36
};

struct GXHardwareBufferResource {
    GXMemoryAllocator* allocator;             // +0x00
    void*              gpuBaseAddr;           // +0x08
    uint32_t           resourceId;            // +0x10
    uint32_t           requestedSize;         // +0x14
    uint32_t           allocatedSize;         // +0x18
    uint32_t           alignment;             // +0x1c
    uint8_t            dimType;               // +0x20 (1=Buffer)
    uint8_t            usageType;             // +0x21
    uint8_t            cpuAccessFlags;        // +0x22
    uint8_t            bindFlags;             // +0x23
    uint32_t           miscFlags;             // +0x24
    uint8_t            isDynamicRing;         // +0x28
    uint8_t            pad29;                 // +0x29
    uint16_t           elementStride;         // +0x2a
    uint32_t           pad2c;                 // +0x2c
    GXMemoryAllocator* sysAllocator;          // +0x30
    uint16_t*          contextBoundSlotTable; // +0x38
};

struct GXPsslEmbeddedHwHeaderInfo {
    const uint8_t* hwShaderHeader; // +0x00
    const void*    hwShaderUcode;  // +0x08
    uint32_t       hwUcodeSize;    // +0x10
    uint32_t       pad14;          // +0x14
};

extern "C" {
    void    GXPsslShaderBinaryInfo_Construct(void* info); // 0x10a3b70
    int32_t GXPsslShaderBinaryInfo_ParseFromMemory(void* info, const void* data, uint32_t size); // 0x10a3c40
    void    GXPsslShaderBinaryInfo_Destroy(void* info); // 0x10a3bf0
    void    GXPsslContainer_ExtractEmbeddedHwShaderHeader(GXPsslEmbeddedHwHeaderInfo* outInfo, const void* shdrContainer); // 0x109c3b0
}

// [VMA: 0x216d8f0 - 0x216d8f5] (size: 5 bytes)
// Translates: sub_216d8f0
// Returns the hardware resource dimension type byte at +0x20.
uint8_t GXGnmResource_GetDimensionType(const GXHardwareTextureResource* hwRes) {
    return hwRes->dimType;
}

// [VMA: 0x216d7f0 - 0x216d8e7] (size: 247 bytes)
// Translates: sub_216d7f0
// Initializes a secondary SceGnmTexture (for HTILE/FMASK/CMASK companion metadata) from GXSurfaceFormatInfo2D.
extern "C" uint64_t GXGnmTexture2D_InitGnmTextureFromSurfaceInfo(
    const void* surfInfoRaw,
    void* outGnmTex)
{
    const auto* surfInfo = static_cast<const GXSurfaceFormatInfo2D*>(surfInfoRaw);
    uint32_t miscFlags = surfInfo->miscFlags;
    uint32_t isEqaa = (miscFlags & 0x400u) ? 1u : 0u;
    uint32_t numSlices = surfInfo->arraySize;
    uint8_t isCubemap = 0;

    if (surfInfo->isArrayOrCube != 0) {
        if ((miscFlags & 0x4u) != 0) {
            numSlices = static_cast<uint32_t>((static_cast<uint64_t>(numSlices) * 0xaaaaaaabULL) >> 34);
            isCubemap = 1;
        }
    }

    uint32_t numFragments = CGFormatGnm_ResolveNumFragments(
        surfInfo->sampleCount,
        surfInfo->sampleQuality,
        isEqaa);

    if (!CGFormatGnm_HasFmaskOrHtileFlag(surfInfo->bindFlags, surfInfo->format)) {
        return 0ULL;
    }

    uint32_t width = surfInfo->width;
    uint32_t height = surfInfo->height;
    uint32_t mipLevels = surfInfo->mipLevels;
    if (mipLevels == 0) {
        uint32_t lzW = width ? __builtin_clz(width) : 32u;
        uint32_t lzH = height ? __builtin_clz(height) : 32u;
        uint32_t minLz = (lzH < 31u) ? lzH : 31u;
        if (lzW < minLz) {
            minLz = lzW;
        }
        mipLevels = 32u - minLz;
    }

    uint64_t packedSizeAlign = SceGnmTexture_Init2DAndReturnSizeAlign(
        outGnmTex,
        width,
        height,
        numSlices,
        mipLevels,
        0x20401u,
        surfInfo->tileMode,
        numFragments,
        isCubemap);

    return (packedSizeAlign & 0xffffffff00000000ULL) | static_cast<uint32_t>(packedSizeAlign);
}

struct GXBufferDescriptorOut {
    uint32_t byteSize;          // +0x00 (from +0x14)
    uint32_t usageType;         // +0x04 (from byte +0x21)
    uint32_t bindFlags;         // +0x08 (from byte +0x23)
    uint32_t cpuAccessFlags;    // +0x0c (from byte +0x22)
    uint32_t miscFlags;         // +0x10 (from dword +0x24)
    uint32_t structureByteStride; // +0x14 (from word +0x2a)
};

// [VMA: 0x216d730 - 0x216d758] (size: 40 bytes)
// Translates: sub_216d730
// Extracts a 24-byte GXBufferDescriptorOut from a GXHardwareBufferResource.
void GXGnmBuffer_ExtractDescriptorFromHardware(
    const GXHardwareBufferResource* hwBuf,
    GXBufferDescriptorOut* outDesc)
{
    outDesc->byteSize = hwBuf->requestedSize;
    outDesc->usageType = hwBuf->usageType;
    outDesc->bindFlags = hwBuf->bindFlags;
    outDesc->cpuAccessFlags = hwBuf->cpuAccessFlags;
    outDesc->miscFlags = hwBuf->miscFlags;
    outDesc->structureByteStride = hwBuf->elementStride;
}

#undef g_GXGnmContextSlotTable
#define g_GXGnmContextSlotTable (reinterpret_cast<void**>(GX_VMA(0x56a63c0ULL)))
extern void CGContextGnm_UnbindConstantBufferSlot(void* constBufTable, uint32_t slotIndex); // 0x26ae900
extern uint32_t CGResourceGnm_AllocateUniqueResourceId(); // 0x26baef0
extern bool CGMemoryGnm_IsCpuReadableAddress(const void* addr); // 0x26bad70
extern void CGMemoryGnm_FastCopyGpuMemory(void* dst, const void* src, uint32_t byteSize); // 0x261a1b0
extern void OperatorDelete_Global(void* ptr); // 0x2bbe588

// [VMA: 0x216d690 - 0x216d724] (size: 148 bytes)
// Translates: sub_216d690
// Releases GPU buffer memory and unbinds any bound constant buffer slots across all 16 active contexts.
void GXGnmBuffer_ReleaseMemoryAndNotifyContexts(GXHardwareBufferResource* hwBuf) {
    if (hwBuf->allocator != nullptr && hwBuf->gpuBaseAddr != nullptr) {
        gx_direct_vk::RetireDirectBufferRange(hwBuf->gpuBaseAddr, hwBuf->allocatedSize);
        hwBuf->allocator->vtable->FreeAligned(hwBuf->allocator, hwBuf->gpuBaseAddr);
        hwBuf->gpuBaseAddr = nullptr;
    }

    if ((hwBuf->bindFlags & 0x04u) != 0) {
        for (size_t i = 0; i < 16; ++i) {
            uint8_t* ctx = static_cast<uint8_t*>(g_GXGnmContextSlotTable[i]);
            if (ctx != nullptr) {
                uint16_t slotIdx = hwBuf->contextBoundSlotTable[i];
                CGContextGnm_UnbindConstantBufferSlot(ctx + 0xb3c0, slotIdx);
            }
        }
        if (hwBuf->sysAllocator != nullptr && hwBuf->contextBoundSlotTable != nullptr) {
            hwBuf->sysAllocator->vtable->FreeAligned(hwBuf->sysAllocator, hwBuf->contextBoundSlotTable);
            hwBuf->contextBoundSlotTable = nullptr;
        }
    }
}

struct GXBufferCreationDesc {
    uint32_t byteSize;          // +0x00
    uint32_t usageType;         // +0x04
    uint32_t bindFlags;         // +0x08
    uint32_t cpuAccessFlags;    // +0x0c
    uint32_t miscFlags;         // +0x10
    uint16_t elementStride;     // +0x14
};

struct GXBufferExternalPlacementInfo {
    uint8_t  hasCustomSizeAlign; // +0x00
    uint8_t  pad01[3];           // +0x01
    uint32_t customByteSize;     // +0x04
    uint32_t customAlignment;    // +0x08
};

// [VMA: 0x216d420 - 0x216d68e] (size: 622 bytes)
// Translates: sub_216d420
// Allocates and initializes a GXHardwareBufferResource, including optionalGDS/append-consume counter padding and per-context CB slot tracking.
uint32_t GXGnmBuffer_CreateAndInitialize(
    GXHardwareBufferResource* outBuf,
    void* unusedDevice,
    const GXBufferCreationDesc* desc,
    const void* const* initialDataPtr,
    GXMemoryAllocator* sysOrOnionAllocator,
    GXMemoryAllocator* garlicAllocator,
    const GXBufferExternalPlacementInfo* extPlacement)
{
    GXMemoryAllocator* memAlloc = garlicAllocator;
    uint32_t reqSize = desc->byteSize;
    uint32_t miscFlags = desc->miscFlags;
    if ((miscFlags & 0x4000u) != 0) {
        memAlloc = sysOrOnionAllocator;
    }

    uint32_t alignment = ((miscFlags >> 1) & 0x4u) + 4u;
    uint32_t allocSize = reqSize;

    if (extPlacement != nullptr && extPlacement->hasCustomSizeAlign != 0) {
        allocSize = extPlacement->customByteSize;
        if (allocSize < reqSize) {
            return 8;
        }
        alignment = extPlacement->customAlignment;
        if (alignment < 4u) {
            return 8;
        }
    }

    allocSize = (allocSize + 3u) & ~3u;
    uint32_t hasCounterTail = desc->bindFlags & 0x80u;
    if (hasCounterTail != 0) {
        allocSize = (allocSize + 0x17u) & ~7u;
        if (alignment <= 8u) {
            alignment = 8u;
        }
    }

    void* gpuMem = memAlloc->vtable->AllocateAligned(memAlloc, allocSize, alignment);
    if (gpuMem == nullptr) {
        return 9;
    }
    gx_direct_vk::NoteDirectFreshGpuRange(gpuMem, allocSize);

    if (hasCounterTail != 0) {
        uint8_t* tailEnd = static_cast<uint8_t*>(gpuMem) + allocSize;
        gx_direct_vk::PrepareDirectBufferWrite(tailEnd - 0x10, 0x10);
        *reinterpret_cast<uint64_t*>(tailEnd - 0x08) = 0ULL;
        *reinterpret_cast<uint64_t*>(tailEnd - 0x10) = 0ULL;
    }

    outBuf->allocator = memAlloc;
    outBuf->gpuBaseAddr = gpuMem;
    outBuf->resourceId = CGResourceGnm_AllocateUniqueResourceId();
    outBuf->requestedSize = desc->byteSize;
    outBuf->allocatedSize = allocSize;
    outBuf->alignment = alignment;
    outBuf->dimType = 1;
    outBuf->usageType = static_cast<uint8_t>(desc->usageType);
    outBuf->cpuAccessFlags = static_cast<uint8_t>(desc->cpuAccessFlags);
    outBuf->bindFlags = static_cast<uint8_t>(desc->bindFlags);
    outBuf->miscFlags = desc->miscFlags;

    uint8_t isDynamicBindable = 0;
    if (desc->usageType == 2u) {
        isDynamicBindable = ((desc->bindFlags & 0x3u) != 0) ? 1u : 0u;
    }
    outBuf->isDynamicRing = isDynamicBindable;
    outBuf->elementStride = desc->elementStride;
    outBuf->sysAllocator = sysOrOnionAllocator;
    outBuf->contextBoundSlotTable = nullptr;

    if ((desc->bindFlags & 0x04u) != 0) {
        uint32_t errCode = 5;
        if (sysOrOnionAllocator == nullptr) {
            GXGnmBuffer_ReleaseMemoryAndNotifyContexts(outBuf);
            return errCode;
        }
        uint16_t* slotTable = static_cast<uint16_t*>(
            sysOrOnionAllocator->vtable->Allocate(sysOrOnionAllocator, 0x20u));
        outBuf->contextBoundSlotTable = slotTable;
        if (slotTable == nullptr) {
            errCode = 9;
            GXGnmBuffer_ReleaseMemoryAndNotifyContexts(outBuf);
            return errCode;
        }
        uint64_t* slotWords = reinterpret_cast<uint64_t*>(slotTable);
        slotWords[3] = 0ULL;
        slotWords[2] = 0ULL;
        slotWords[1] = 0ULL;
        slotWords[0] = 0ULL;
    }

    if (initialDataPtr != nullptr) {
        if (CGMemoryGnm_IsCpuReadableAddress(gpuMem)) {
            if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectBuffers)) {
                const uint32_t copySize = desc->byteSize;
                const uint32_t noteSize = (hasCounterTail == 0 && allocSize > copySize)
                                              ? allocSize
                                              : copySize;
                gx_direct_vk::PrepareDirectBufferWrite(gpuMem, noteSize);
                gx_direct_vk::NoteDirectAssetBufferWrite(gpuMem, noteSize);
                if (copySize > 0u) {
                    std::memcpy(gpuMem, *initialDataPtr, copySize);
                }
                if (noteSize > copySize) {
                    std::memset(static_cast<uint8_t*>(gpuMem) + copySize, 0, noteSize - copySize);
                }
                gx_direct_vk::NoteDirectAssetBufferWrite(gpuMem, noteSize);
                // Immutable buffers (usageType == 1, cpuAccessFlags == 0, no UAV 0x80 / SO 0x10)
                // — including 100% of combined FLVER model VB+IB buffers — get a dedicated VMA
                // DEVICE_LOCAL VkBuffer resolved in O(1) on Stage B.
                if (desc->usageType == 1u && desc->cpuAccessFlags == 0u &&
                    (desc->bindFlags & 0x90u) == 0u && copySize > 0u) {
                    gx_direct_vk::RegisterDirectImmutableBuffer(
                        outBuf->resourceId, gpuMem, copySize, allocSize, desc->bindFlags);
                }
            } else {
                CGMemoryGnm_FastCopyGpuMemory(gpuMem, *initialDataPtr, desc->byteSize);
            }
        }
    }
    return 0;
}

struct GXRefCountedTextureResource {
    void*                     vtable;      // +0x00
    void*                     parentDevice;// +0x08
    uint32_t                  refCount;    // +0x10
    uint32_t                  pad14;       // +0x14
    GXHardwareTextureResource hwResource;  // +0x18 (size 0x38: +0x18..+0x4f)
};

struct GXRefCountedBufferResource {
    void*                    vtable;      // +0x00
    void*                    parentDevice;// +0x08
    uint32_t                 refCount;    // +0x10
    uint32_t                 pad14;       // +0x14
    GXHardwareBufferResource hwResource;  // +0x18 (size 0x40: +0x18..+0x57)
};

#undef g_Vtable_GXRefCountedResourceBase
#define g_Vtable_GXRefCountedResourceBase (*reinterpret_cast<void**>(GX_VMA(0x53b6240ULL)))
#undef g_Vtable_GXRefCountedTexture3DResource
#define g_Vtable_GXRefCountedTexture3DResource (*reinterpret_cast<void**>(GX_VMA(0x535f400ULL)))
#undef g_Vtable_GXRefCountedTexture2DResource
#define g_Vtable_GXRefCountedTexture2DResource (*reinterpret_cast<void**>(GX_VMA(0x535f3e0ULL)))
#undef g_Vtable_GXRefCountedTexture1DResource
#define g_Vtable_GXRefCountedTexture1DResource (*reinterpret_cast<void**>(GX_VMA(0x535f3c0ULL)))
#undef g_Vtable_GXRefCountedBufferResource
#define g_Vtable_GXRefCountedBufferResource (*reinterpret_cast<void**>(GX_VMA(0x535f3a0ULL)))

// [VMA: 0x216d350 - 0x216d38a] (size: 58 bytes)
// Translates: sub_216d350
// Non-deleting destructor for GXRefCountedTexture3DResource.
void GXRefCountedTexture3DResource_Destructor(GXRefCountedTextureResource* self) {
    self->vtable = &g_Vtable_GXRefCountedTexture3DResource;
    GXMemoryAllocator* alloc = self->hwResource.allocator;
    if (alloc != nullptr && self->hwResource.gpuBaseAddr != nullptr) {
        gx_direct_vk::UnwatchDirectTextureRange(self->hwResource.gpuBaseAddr, self->hwResource.allocSize);
        alloc->vtable->FreeAligned(alloc, self->hwResource.gpuBaseAddr);
        self->hwResource.gpuBaseAddr = nullptr;
    }
}

// [VMA: 0x216d310 - 0x216d349] (size: 57 bytes)
// Translates: sub_216d310
// Deleting destructor for GXRefCountedTexture3DResource.
void GXRefCountedTexture3DResource_DeletingDestructor(GXRefCountedTextureResource* self) {
    self->vtable = &g_Vtable_GXRefCountedTexture3DResource;
    GXMemoryAllocator* alloc = self->hwResource.allocator;
    if (alloc != nullptr && self->hwResource.gpuBaseAddr != nullptr) {
        gx_direct_vk::UnwatchDirectTextureRange(self->hwResource.gpuBaseAddr, self->hwResource.allocSize);
        alloc->vtable->FreeAligned(alloc, self->hwResource.gpuBaseAddr);
    }
    OperatorDelete_Global(self);
}

// [VMA: 0x216d2b0 - 0x216d307] (size: 87 bytes)
// Translates: sub_216d2b0
// Constructor for GXRefCountedTexture3DResource.
void GXRefCountedTexture3DResource_Constructor(
    GXRefCountedTextureResource* self,
    const GXHardwareTextureResource* srcHwTex,
    void* parentDevice)
{
    self->vtable = &g_Vtable_GXRefCountedResourceBase;
    self->parentDevice = parentDevice;
    self->refCount = 1u;
    self->vtable = &g_Vtable_GXRefCountedTexture3DResource;
    self->hwResource = *srcHwTex;
}

// [VMA: 0x216d1e0 - 0x216d21a] (size: 58 bytes)
// Translates: sub_216d1e0
// Non-deleting destructor for GXRefCountedTexture2DResource.
void GXRefCountedTexture2DResource_Destructor(GXRefCountedTextureResource* self) {
    self->vtable = &g_Vtable_GXRefCountedTexture2DResource;
    GXMemoryAllocator* alloc = self->hwResource.allocator;
    if (alloc != nullptr && self->hwResource.gpuBaseAddr != nullptr) {
        gx_direct_vk::UnwatchDirectTextureRange(self->hwResource.gpuBaseAddr, self->hwResource.allocSize);
        alloc->vtable->FreeAligned(alloc, self->hwResource.gpuBaseAddr);
        self->hwResource.gpuBaseAddr = nullptr;
    }
}

// [VMA: 0x216d1a0 - 0x216d1d9] (size: 57 bytes)
// Translates: sub_216d1a0
// Deleting destructor for GXRefCountedTexture2DResource.
void GXRefCountedTexture2DResource_DeletingDestructor(GXRefCountedTextureResource* self) {
    self->vtable = &g_Vtable_GXRefCountedTexture2DResource;
    GXMemoryAllocator* alloc = self->hwResource.allocator;
    if (alloc != nullptr && self->hwResource.gpuBaseAddr != nullptr) {
        gx_direct_vk::UnwatchDirectTextureRange(self->hwResource.gpuBaseAddr, self->hwResource.allocSize);
        alloc->vtable->FreeAligned(alloc, self->hwResource.gpuBaseAddr);
    }
    OperatorDelete_Global(self);
}

// [VMA: 0x216d140 - 0x216d197] (size: 87 bytes)
// Translates: sub_216d140
// Constructor for GXRefCountedTexture2DResource.
void GXRefCountedTexture2DResource_Constructor(
    GXRefCountedTextureResource* self,
    const GXHardwareTextureResource* srcHwTex,
    void* parentDevice)
{
    self->vtable = &g_Vtable_GXRefCountedResourceBase;
    self->parentDevice = parentDevice;
    self->refCount = 1u;
    self->vtable = &g_Vtable_GXRefCountedTexture2DResource;
    self->hwResource = *srcHwTex;
}

// [VMA: 0x216d070 - 0x216d0aa] (size: 58 bytes)
// Translates: sub_216d070
// Non-deleting destructor for GXRefCountedTexture1DResource.
void GXRefCountedTexture1DResource_Destructor(GXRefCountedTextureResource* self) {
    self->vtable = &g_Vtable_GXRefCountedTexture1DResource;
    GXMemoryAllocator* alloc = self->hwResource.allocator;
    if (alloc != nullptr && self->hwResource.gpuBaseAddr != nullptr) {
        gx_direct_vk::UnwatchDirectTextureRange(self->hwResource.gpuBaseAddr, self->hwResource.allocSize);
        alloc->vtable->FreeAligned(alloc, self->hwResource.gpuBaseAddr);
        self->hwResource.gpuBaseAddr = nullptr;
    }
}

// [VMA: 0x216d030 - 0x216d069] (size: 57 bytes)
// Translates: sub_216d030
// Deleting destructor for GXRefCountedTexture1DResource.
void GXRefCountedTexture1DResource_DeletingDestructor(GXRefCountedTextureResource* self) {
    self->vtable = &g_Vtable_GXRefCountedTexture1DResource;
    GXMemoryAllocator* alloc = self->hwResource.allocator;
    if (alloc != nullptr && self->hwResource.gpuBaseAddr != nullptr) {
        gx_direct_vk::UnwatchDirectTextureRange(self->hwResource.gpuBaseAddr, self->hwResource.allocSize);
        alloc->vtable->FreeAligned(alloc, self->hwResource.gpuBaseAddr);
    }
    OperatorDelete_Global(self);
}

// [VMA: 0x216cfd0 - 0x216d027] (size: 87 bytes)
// Translates: sub_216cfd0
// Constructor for GXRefCountedTexture1DResource.
void GXRefCountedTexture1DResource_Constructor(
    GXRefCountedTextureResource* self,
    const GXHardwareTextureResource* srcHwTex,
    void* parentDevice)
{
    self->vtable = &g_Vtable_GXRefCountedResourceBase;
    self->parentDevice = parentDevice;
    self->refCount = 1u;
    self->vtable = &g_Vtable_GXRefCountedTexture1DResource;
    self->hwResource = *srcHwTex;
}

// [VMA: 0x216cf20 - 0x216cf36] (size: 22 bytes)
// Translates: sub_216cf20
// Base constructor for GXRefCountedResourceBase.
void GXRefCountedResourceBase_Constructor(GXRefCountedTextureResource* self, void* parentDevice) {
    self->vtable = &g_Vtable_GXRefCountedResourceBase;
    self->parentDevice = parentDevice;
    self->refCount = 1u;
}

// [VMA: 0x216cf10 - 0x216cf13] (size: 3 bytes)
// Translates: sub_216cf10
// Base virtual method returning 0.
uint32_t GXRefCountedResourceBase_GetSubTypeZero() {
    return 0;
}

// [VMA: 0x216cf00 - 0x216cf01] (size: 1 byte)
// Translates: sub_216cf00
// Base virtual no-op destructor.
void GXRefCountedResourceBase_Destructor(void* self) {
}

// [VMA: 0x216cef0 - 0x216cef5] (size: 5 bytes)
// Translates: sub_216cef0
// Base deleting destructor forwarding directly to global operator delete.
void GXRefCountedResourceBase_DeletingDestructor(void* self) {
    OperatorDelete_Global(self);
}

// [VMA: 0x216ce40 - 0x216ce53] (size: 19 bytes)
// Translates: sub_216ce40
// Non-deleting destructor for GXRefCountedBufferResource.
void GXRefCountedBufferResource_Destructor(GXRefCountedBufferResource* self) {
    self->vtable = &g_Vtable_GXRefCountedBufferResource;
    GXGnmBuffer_ReleaseMemoryAndNotifyContexts(&self->hwResource);
}

// [VMA: 0x216ce10 - 0x216ce3a] (size: 42 bytes)
// Translates: sub_216ce10
// Deleting destructor for GXRefCountedBufferResource.
void GXRefCountedBufferResource_DeletingDestructor(GXRefCountedBufferResource* self) {
    self->vtable = &g_Vtable_GXRefCountedBufferResource;
    GXGnmBuffer_ReleaseMemoryAndNotifyContexts(&self->hwResource);
    OperatorDelete_Global(self);
}

// [VMA: 0x216cda0 - 0x216ce07] (size: 103 bytes)
// Translates: sub_216cda0
// Constructor for GXRefCountedBufferResource (moves ownership of contextBoundSlotTable).
void GXRefCountedBufferResource_Constructor(
    GXRefCountedBufferResource* self,
    GXHardwareBufferResource* srcHwBuf,
    void* parentDevice)
{
    self->vtable = &g_Vtable_GXRefCountedResourceBase;
    self->parentDevice = parentDevice;
    self->refCount = 1u;
    self->vtable = &g_Vtable_GXRefCountedBufferResource;
    self->hwResource = *srcHwBuf;
    srcHwBuf->contextBoundSlotTable = nullptr;
}

// ============================================================================
// Group 5: Fetch Shader Generation & Input Table Remapping
// ============================================================================

struct GXInputElementDesc {
    uint32_t semanticType;         // +0x00
    uint32_t semanticIndex;        // +0x04
    uint32_t format;               // +0x08
    uint8_t  inputSlot;            // +0x0c
    uint8_t  pad0d[3];             // +0x0d
    uint32_t alignedByteOffset;    // +0x10
    uint32_t inputSlotClass;       // +0x14
    uint32_t instanceDataStepRate; // +0x18 (total stride 0x1c)
};

struct GXFetchShaderBuildContext {
    const uint8_t* vsOrLsEsBinaryHeader; // +0x00
    uint32_t       shaderStageType;      // +0x08 (1 = VS, 5 = LS, 6 = ES)
    uint32_t       pad0c;                // +0x0c
    uint32_t*      remappedVgprTable;    // +0x10
    uint32_t*      elementToInputMap;    // +0x18
    uint32_t*      inputToElementMap;    // +0x20
    uint32_t*      semanticRemapTable;   // +0x28
    uint32_t       numShaderInputs;      // +0x30
    uint32_t       pad34;                // +0x34
};

struct GXCompiledFetchShader {
    GXMemoryAllocator* sysAllocator;       // +0x00
    GXMemoryAllocator* garlicAllocator;    // +0x08
    void*              fetchShaderGpuMem;  // +0x10
    uint8_t*           inputTableBlock;    // +0x18
    void*              vertexAttrDescs;    // +0x20
    uint8_t*           streamSlotEntries;  // +0x28
    uint32_t*          packedStreamRanges; // +0x30
    uint32_t*          userSgprBufferMap;  // +0x38
    uint32_t           shaderModifier;     // +0x40
    uint8_t            activeStreamCount;  // +0x44
    uint8_t            numBufferSgprSlots; // +0x45
    uint8_t            numActiveElements;  // +0x46
    uint8_t            firstInstanceStream;// +0x47
    uint8_t            instanceStreamSpan; // +0x48
    uint8_t            pad49[3];           // +0x49
    uint32_t           vertexTableSgprMask;// +0x4c
    uint32_t           directFetchSlotMask;// +0x50
};

extern void SceGnm_BuildVsFetchShaderBuildState(void* outBuildState, const void* vsRegs, uint32_t numInputs, const uint32_t* instancingData, uint32_t instanceStepRate0, uint32_t instanceStepRate1) __asm__("_ZN19GXGraphicsFramework35SceGnm_BuildVsFetchShaderBuildStateEPvPKvjjS2_"); // 0x1087450
extern void SceGnm_BuildLsFetchShaderBuildState(void* outBuildState, const void* lsRegs, uint32_t numInputs, const uint32_t* instancingData, uint32_t instanceStepRate0, uint32_t instanceStepRate1) __asm__("_ZN19GXGraphicsFramework35SceGnm_BuildLsFetchShaderBuildStateEPvPKvjjS2_"); // 0x1087590
extern void SceGnm_BuildEsFetchShaderBuildState(void* outBuildState, const void* esRegs, uint32_t numInputs, const uint32_t* instancingData, uint32_t instanceStepRate0, uint32_t instanceStepRate1) __asm__("_ZN19GXGraphicsFramework35SceGnm_BuildEsFetchShaderBuildStateEPvPKvjjS2_"); // 0x1087650
extern void SceGnm_GenerateFetchShader(void* dstGpuMem, const void* buildState); // 0x10877c0
extern "C" int32_t CGFormatGnm_ConvertToGnmDataFormat(uint32_t* outGnmDataFormat, uint32_t gxFormat); // 0x26b7310
extern void SceGnmBuffer_InitAsVertexBuffer(void* outVsharp, void* baseAddr, uint32_t dataFormat, uint32_t stride, uint32_t numElements); // 0x1070650
extern uint32_t CGFormatGnm_ConvertStepRateToFetchRate(uint32_t slotClassOrStep); // 0x26b7b50
extern void StackChkFail_Abort(); // 0x2bbe4f8
#undef g_FormatByteSizeTable
#define g_FormatByteSizeTable (reinterpret_cast<uint64_t*>(GX_VMA(0x47388d0ULL)))

// [VMA: 0x216c5d0 - 0x216cd99] (size: 1993 bytes)
// Translates: sub_216c5d0
// Generates a GNM hardware fetch shader binary and builds the vertex stream/attribute remapping tables.
uint32_t GXGnmFetchShader_GenerateAndRemapInputTable(
    GXCompiledFetchShader* outFetchShader,
    const GXInputElementDesc* inputElements,
    uint32_t numInputElements,
    const GXFetchShaderBuildContext* buildCtx,
    GXMemoryAllocator* sysAllocator,
    GXMemoryAllocator* garlicAllocator)
{
    outFetchShader->sysAllocator = sysAllocator;
    outFetchShader->garlicAllocator = garlicAllocator;

    struct FetchShaderBuildState {
        uint16_t fetchShaderByteSize;  // +0x00 (at rsp+0xa0)
        uint8_t  pad02[10];            // +0x02
        uint32_t shaderModifier;       // +0x0c (at rsp+0xac)
        uint8_t  pad10[8];             // +0x10
        uint8_t  numInputLocations;    // +0x18 (at rsp+0xb8)
        uint8_t  pad19[3];             // +0x19
        uint32_t numUserSgprs;         // +0x1c (at rsp+0xbc)
        const uint8_t* sgprUsageEnd;   // +0x20 (at rsp+0xc0)
        const uint8_t* sgprUsageTable; // +0x28 (at rsp+0xc8)
        uint32_t numShaderInputs;      // +0x30 (at rsp+0xd0)
        uint32_t* semanticRemapTable;  // +0x38 (at rsp+0xd8)
    } buildState = {};

    const uint8_t* binHdr = buildCtx->vsOrLsEsBinaryHeader;
    uint32_t stageType = buildCtx->shaderStageType;
    const uint8_t* sgprUsageTable = nullptr;
    uint32_t numUserSgprs = 0;

    if (stageType == 5u) {
        SceGnm_BuildLsFetchShaderBuildState(&buildState, binHdr + 0x08, binHdr[0x18], buildCtx->remappedVgprTable, 0u, 0u);
        buildState.numInputLocations = binHdr[0x18];
        buildState.sgprUsageEnd = binHdr + 0x20 + static_cast<size_t>(binHdr[0x03]) * 4u;
        numUserSgprs = *reinterpret_cast<const uint32_t*>(binHdr) >> 24;
        sgprUsageTable = binHdr + 0x20;
        buildState.numUserSgprs = numUserSgprs;
        buildState.sgprUsageTable = sgprUsageTable;
        buildState.numShaderInputs = buildCtx->numShaderInputs;
        buildState.semanticRemapTable = buildCtx->semanticRemapTable;
    } else if (stageType == 6u) {
        SceGnm_BuildEsFetchShaderBuildState(&buildState, binHdr + 0x08, binHdr[0x18], buildCtx->remappedVgprTable, 0u, 0u);
        buildState.numInputLocations = binHdr[0x18];
        buildState.sgprUsageEnd = binHdr + 0x20 + static_cast<size_t>(binHdr[0x03]) * 4u;
        numUserSgprs = *reinterpret_cast<const uint32_t*>(binHdr) >> 24;
        sgprUsageTable = binHdr + 0x20;
        buildState.numUserSgprs = numUserSgprs;
        buildState.sgprUsageTable = sgprUsageTable;
        buildState.numShaderInputs = buildCtx->numShaderInputs;
        buildState.semanticRemapTable = buildCtx->semanticRemapTable;
    } else if (stageType == 1u) {
        SceGnm_BuildVsFetchShaderBuildState(&buildState, binHdr + 0x08, binHdr[0x24], buildCtx->remappedVgprTable, 0u, 0u);
        buildState.numInputLocations = binHdr[0x24];
        buildState.sgprUsageEnd = binHdr + 0x28 + static_cast<size_t>(binHdr[0x03]) * 4u;
        numUserSgprs = *reinterpret_cast<const uint32_t*>(binHdr) >> 24;
        sgprUsageTable = binHdr + 0x28;
        buildState.numUserSgprs = numUserSgprs;
        buildState.sgprUsageTable = sgprUsageTable;
        buildState.numShaderInputs = buildCtx->numShaderInputs;
        buildState.semanticRemapTable = buildCtx->semanticRemapTable;
    } else {
        return 8;
    }

    outFetchShader->shaderModifier = buildState.shaderModifier;

    uint32_t activeSlotMask = 0;
    if (numInputElements != 0) {
        for (uint32_t i = 0; i < numInputElements; ++i) {
            if (buildCtx->elementToInputMap[i] != 0xffffffffu) {
                activeSlotMask |= (1u << inputElements[i].inputSlot);
            }
        }
    }

    uint32_t vertexTableSgprMask = 0;
    uint32_t directFetchSlotMask = 0;
    uint32_t numBufferSgprSlots = 0;
    uint8_t hasBufferSgprSlots = 0;

    if (numUserSgprs != 0) {
        for (uint32_t i = 0; i < numUserSgprs; ++i) {
            uint8_t usageKind = sgprUsageTable[i * 4u + 0u];
            if (usageKind == 0x17u) {
                ++numBufferSgprSlots;
            } else if (usageKind == 0x03u) {
                uint8_t inputIdx = sgprUsageTable[i * 4u + 1u];
                uint32_t elemIdx = buildCtx->inputToElementMap[inputIdx];
                uint8_t slotIdx = inputElements[elemIdx].inputSlot;
                vertexTableSgprMask |= (1u << slotIdx);
            }
        }
        if (numBufferSgprSlots != 0) {
            directFetchSlotMask = activeSlotMask & ~vertexTableSgprMask;
            hasBufferSgprSlots = 1;
        }
    }

    uint32_t activeStreamCount = static_cast<uint32_t>(__builtin_popcount(activeSlotMask));
    uint32_t numActiveElements = 0;
    for (uint32_t i = 0; i < 32u; ++i) {
        if (buildCtx->inputToElementMap[i] != 0xffffffffu) {
            ++numActiveElements;
        }
    }

    size_t vsharpBytes = static_cast<size_t>(numActiveElements) * 16u;
    size_t elemEntryOffset = vsharpBytes + static_cast<size_t>(numActiveElements) * 4u;
    size_t streamRangeEnd = elemEntryOffset + static_cast<size_t>(activeStreamCount) * 4u;
    size_t totalTableBytes = streamRangeEnd + static_cast<size_t>(numBufferSgprSlots) * 4u;

    if (totalTableBytes == 0) {
        outFetchShader->userSgprBufferMap = nullptr;
        outFetchShader->packedStreamRanges = nullptr;
        outFetchShader->streamSlotEntries = nullptr;
        outFetchShader->vertexAttrDescs = nullptr;
        outFetchShader->inputTableBlock = nullptr;
        outFetchShader->fetchShaderGpuMem = nullptr;
        outFetchShader->vertexTableSgprMask = 0;
        outFetchShader->directFetchSlotMask = 0;
        outFetchShader->activeStreamCount = 0;
        outFetchShader->numBufferSgprSlots = 0;
        outFetchShader->numActiveElements = 0;
        outFetchShader->firstInstanceStream = 0;
        outFetchShader->instanceStreamSpan = 0;
        return 0;
    }

    uint8_t* tableMem = static_cast<uint8_t*>(
        sysAllocator->vtable->AllocateAligned(sysAllocator, totalTableBytes, 16u));
    if (tableMem == nullptr) {
        garlicAllocator->vtable->FreeAligned(garlicAllocator, outFetchShader->fetchShaderGpuMem);
        return 9;
    }

    std::memset(tableMem, 0, totalTableBytes);
    outFetchShader->inputTableBlock = tableMem;

    if (numActiveElements != 0) {
        outFetchShader->vertexAttrDescs = tableMem;
        outFetchShader->streamSlotEntries = tableMem + vsharpBytes;
    } else {
        outFetchShader->vertexAttrDescs = nullptr;
        outFetchShader->streamSlotEntries = nullptr;
    }

    outFetchShader->packedStreamRanges = (activeStreamCount != 0)
        ? reinterpret_cast<uint32_t*>(tableMem + elemEntryOffset)
        : nullptr;
    outFetchShader->userSgprBufferMap = (hasBufferSgprSlots != 0)
        ? reinterpret_cast<uint32_t*>(tableMem + streamRangeEnd)
        : nullptr;

    outFetchShader->activeStreamCount = static_cast<uint8_t>(activeStreamCount);
    outFetchShader->numBufferSgprSlots = static_cast<uint8_t>(numBufferSgprSlots);
    outFetchShader->numActiveElements = static_cast<uint8_t>(numActiveElements);
    outFetchShader->firstInstanceStream = 0;
    outFetchShader->instanceStreamSpan = 0;
    outFetchShader->vertexTableSgprMask = vertexTableSgprMask;
    outFetchShader->directFetchSlotMask = directFetchSlotMask;

    void* gpuFetchMem = garlicAllocator->vtable->AllocateAligned(
        garlicAllocator, buildState.fetchShaderByteSize, 4u);
    outFetchShader->fetchShaderGpuMem = gpuFetchMem;
    if (gpuFetchMem == nullptr) {
        return 9;
    }

    SceGnm_GenerateFetchShader(gpuFetchMem, &buildState);

    uint32_t remainingSlotMask = activeSlotMask;
    if (remainingSlotMask != 0) {
        if (numInputElements != 0) {
            uint8_t minDirectInput = 0xffu;
            uint32_t maxDirectInput = 0u;
            uint32_t streamIdx = 0u;
            uint8_t baseAttrIdx = 0u;

            while (remainingSlotMask != 0) {
                uint32_t slotBitIdx = static_cast<uint32_t>(__builtin_ctz(remainingSlotMask));
                uint32_t slotBitMask = 1u << slotBitIdx;
                remainingSlotMask -= slotBitMask;

                uint8_t countInStream = 0u;
                for (uint32_t elemIdx = 0; elemIdx < numInputElements; ++elemIdx) {
                    if (inputElements[elemIdx].inputSlot != slotBitIdx) {
                        continue;
                    }
                    uint32_t shaderInputIdx = buildCtx->elementToInputMap[elemIdx];
                    if (shaderInputIdx == 0xffffffffu) {
                        continue;
                    }

                    size_t flatAttrIdx = static_cast<size_t>(baseAttrIdx) + static_cast<size_t>(countInStream);
                    outFetchShader->streamSlotEntries[flatAttrIdx * 4u] = static_cast<uint8_t>(shaderInputIdx);

                    if ((outFetchShader->directFetchSlotMask & slotBitMask) != 0) {
                        uint8_t inByte = static_cast<uint8_t>(shaderInputIdx);
                        if (inByte < minDirectInput) {
                            minDirectInput = inByte;
                        }
                        if (inByte >= static_cast<uint8_t>(maxDirectInput)) {
                            maxDirectInput = shaderInputIdx;
                        }
                    }

                    uint32_t gnmFormat = 0;
                    uint32_t fmtErr = CGFormatGnm_ConvertToGnmDataFormat(&gnmFormat, inputElements[elemIdx].format);
                    if (fmtErr != 0) {
                        garlicAllocator->vtable->FreeAligned(garlicAllocator, outFetchShader->fetchShaderGpuMem);
                        sysAllocator->vtable->FreeAligned(sysAllocator, outFetchShader->inputTableBlock);
                        return fmtErr;
                    }

                    uint32_t fmtIdx = inputElements[elemIdx].format - 1u;
                    uint32_t elemByteSize = (fmtIdx <= 0x62u)
                        ? static_cast<uint32_t>(g_FormatByteSizeTable[fmtIdx])
                        : 0u;

                    uint8_t* vsharpDst = static_cast<uint8_t*>(outFetchShader->vertexAttrDescs) + (flatAttrIdx << 4);
                    SceGnmBuffer_InitAsVertexBuffer(
                        vsharpDst,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(inputElements[elemIdx].alignedByteOffset)),
                        gnmFormat,
                        elemByteSize,
                        1u);

                    ++countInStream;
                }

                uint32_t packedStreamEntry =
                    (slotBitIdx << 16) |
                    (static_cast<uint32_t>(countInStream) << 8) |
                    static_cast<uint32_t>(baseAttrIdx);
                outFetchShader->packedStreamRanges[streamIdx] = packedStreamEntry;
                ++streamIdx;
                baseAttrIdx = static_cast<uint8_t>(baseAttrIdx + countInStream);
            }

            if (minDirectInput != 0xffu) {
                outFetchShader->firstInstanceStream = minDirectInput;
                outFetchShader->instanceStreamSpan = static_cast<uint8_t>(1u - minDirectInput + static_cast<uint8_t>(maxDirectInput));
            }
        } else {
            uint32_t streamIdx = 0u;
            while (remainingSlotMask != 0) {
                uint32_t slotBitIdx = static_cast<uint32_t>(__builtin_ctz(remainingSlotMask));
                outFetchShader->packedStreamRanges[streamIdx] = (slotBitIdx << 16);
                remainingSlotMask -= (1u << slotBitIdx);
                ++streamIdx;
            }
        }
    }

    if (numUserSgprs != 0) {
        uint32_t bufSgprOutIdx = 0u;
        for (uint32_t i = 0; i < numUserSgprs; ++i) {
            uint8_t usageKind = sgprUsageTable[i * 4u + 0u];
            uint8_t sgprOffset = sgprUsageTable[i * 4u + 2u];
            if (usageKind == 0x17u) {
                uint32_t slotIndex = sgprUsageTable[i * 4u + 1u];
                outFetchShader->userSgprBufferMap[bufSgprOutIdx] = (slotIndex << 8) | static_cast<uint32_t>(sgprOffset);
                ++bufSgprOutIdx;
            } else if (usageKind == 0x03u) {
                uint8_t targetInputIdx = sgprUsageTable[i * 4u + 1u];
                for (uint32_t attrIdx = 0; attrIdx < numActiveElements; ++attrIdx) {
                    uint8_t* entry = &outFetchShader->streamSlotEntries[attrIdx * 4u];
                    if (entry[0] == targetInputIdx) {
                        entry[1] = sgprOffset;
                        break;
                    }
                }
            }
        }
    }

    return 0;
}


// [VMA: 0x216c4f0 - 0x216c535] (size: 69 bytes)
// Translates: sub_216c4f0
// Releases both the system-memory input remapping table and the Garlic GPU fetch shader binary.
void GXGnmFetchShader_ReleaseMemory(GXCompiledFetchShader* self) {
    if (self->inputTableBlock != nullptr) {
        self->sysAllocator->vtable->FreeAligned(self->sysAllocator, self->inputTableBlock);
        self->inputTableBlock = nullptr;
    }
    if (self->fetchShaderGpuMem != nullptr) {
        self->garlicAllocator->vtable->FreeAligned(self->garlicAllocator, self->fetchShaderGpuMem);
        self->fetchShaderGpuMem = nullptr;
    }
}

struct GXTlsScratchArena {
    uint8_t* basePtr; // +0x00
    uint8_t* warnPtr; // +0x08
    uint8_t* curPtr;  // +0x10
    uint8_t* endPtr;  // +0x18
};

#define g_TlsScratchArena (*reinterpret_cast<GXTlsScratchArena**>(gx_guest_tls_slot(0x53e4568ULL))) // fs:0x0 + [0x53e4568]
extern void GXTlsScratchArena_InitializeThreadLocal(); // 0x207d250
extern void GXDebug_ReportAssertFailure(const char* file, uint32_t line, const char* expr); // 0x20b55b0

struct GXShaderProgramStageInfo {
    uint8_t        pad00[0xec];
    uint32_t       shaderStageType;  // +0xec
    uint8_t        padF0[0x10];
    const uint8_t* hwBinaryHeader;   // +0x100
    uint8_t        pad108[0x18];
    uint32_t       inputTableOffset; // +0x120
    uint32_t       numShaderInputs;  // +0x124
};

// [VMA: 0x216c140 - 0x216c4ee] (size: 942 bytes)
// Translates: sub_216c140
// Matches vertex input elements against a compiled shader stage program and generates a GNM fetch shader.
uint32_t GXGnmFetchShader_CompileFromShaderProgram(
    GXCompiledFetchShader* outFetchShader,
    void* unusedDevice,
    const GXInputElementDesc* inputElements,
    uint32_t numInputElements,
    const GXShaderProgramStageInfo* shaderProg,
    GXMemoryAllocator* sysAllocator,
    GXMemoryAllocator* garlicAllocator)
{
    uint32_t numShaderInputs = shaderProg->numShaderInputs;
    uint32_t scratchByteSize = ((numShaderInputs + numInputElements) * 4u) + 0x100u;

    GXTlsScratchArena* arena = g_TlsScratchArena;
    uint8_t* savedScratchCursor = nullptr;
    if (arena != nullptr) {
        savedScratchCursor = arena->curPtr;
    } else {
        GXTlsScratchArena_InitializeThreadLocal();
        arena = g_TlsScratchArena;
        savedScratchCursor = arena->curPtr;
        if (arena == nullptr) {
            GXTlsScratchArena_InitializeThreadLocal();
            arena = g_TlsScratchArena;
        }
    }

    uint8_t* curCursor = arena->curPtr;
    uint8_t* nextCursor = curCursor + scratchByteSize;
    uint32_t* scratchBuf = nullptr;
    if (nextCursor <= arena->endPtr) {
        arena->curPtr = nextCursor;
        scratchBuf = reinterpret_cast<uint32_t*>(curCursor);
    }

    GXMemoryAllocator* heapFallbackOwner = nullptr;
    if (sysAllocator != nullptr && scratchBuf == nullptr) {
        scratchBuf = static_cast<uint32_t*>(
            sysAllocator->vtable->Allocate(sysAllocator, scratchByteSize));
        heapFallbackOwner = sysAllocator;
    }

    uint32_t resultStatus = 9u;
    if (scratchBuf != nullptr) {
        uint32_t* remappedVgprTable = scratchBuf;
        uint32_t* elementToInputMap = scratchBuf + numShaderInputs;
        if (numInputElements != 0) {
            std::memset(elementToInputMap, 0xff, static_cast<size_t>(numInputElements) * 4u);
        }

        size_t totalMapOffset = static_cast<size_t>(numShaderInputs) + static_cast<size_t>(numInputElements);
        uint32_t* semanticRemapTable = scratchBuf + totalMapOffset;
        uint32_t* inputToElementMap = scratchBuf + totalMapOffset + 0x20u;
        std::memset(semanticRemapTable, 0xff, 0x100u);

        const uint8_t* binHdr = shaderProg->hwBinaryHeader;
        uint32_t inputTableOffset = shaderProg->inputTableOffset;
        bool matchFailed = false;

        if (numShaderInputs != 0) {
            const uint8_t* shaderInputEntries = binHdr + inputTableOffset;
            for (uint32_t inIdx = 0; inIdx < numShaderInputs; ++inIdx) {
                uint32_t semType = shaderInputEntries[inIdx * 4u + 0u];
                if (semType == 0xffu) {
                    continue;
                }
                if (numInputElements == 0) {
                    matchFailed = true;
                    break;
                }

                uint32_t semIndex = shaderInputEntries[inIdx * 4u + 1u];
                uint32_t matchedElemIdx = numInputElements;
                for (uint32_t elemIdx = 0; elemIdx < numInputElements; ++elemIdx) {
                    if (inputElements[elemIdx].semanticType == semType &&
                        inputElements[elemIdx].semanticIndex == semIndex) {
                        matchedElemIdx = elemIdx;
                        break;
                    }
                }

                if (matchedElemIdx >= numInputElements) {
                    matchFailed = true;
                    break;
                }

                elementToInputMap[matchedElemIdx] = inIdx;
                semanticRemapTable[inIdx] = matchedElemIdx;
                inputToElementMap[inIdx] = shaderInputEntries[inIdx * 4u + 2u];
                remappedVgprTable[inIdx] = CGFormatGnm_ConvertStepRateToFetchRate(
                    inputElements[matchedElemIdx].inputSlotClass);
            }
        }

        if (matchFailed) {
            resultStatus = 8u;
        } else {
            GXFetchShaderBuildContext buildCtx = {};
            buildCtx.vsOrLsEsBinaryHeader = shaderProg->hwBinaryHeader;
            buildCtx.shaderStageType = shaderProg->shaderStageType;
            buildCtx.remappedVgprTable = remappedVgprTable;
            buildCtx.elementToInputMap = elementToInputMap;
            buildCtx.inputToElementMap = semanticRemapTable;
            buildCtx.semanticRemapTable = inputToElementMap;
            buildCtx.numShaderInputs = numShaderInputs;

            resultStatus = GXGnmFetchShader_GenerateAndRemapInputTable(
                outFetchShader,
                inputElements,
                numInputElements,
                &buildCtx,
                sysAllocator,
                garlicAllocator);
        }

        if (heapFallbackOwner != nullptr) {
            heapFallbackOwner->vtable->FreeAligned(heapFallbackOwner, scratchBuf);
        }
    }

    GXTlsScratchArena* endArena = g_TlsScratchArena;
    if (endArena == nullptr) {
        GXTlsScratchArena_InitializeThreadLocal();
        endArena = g_TlsScratchArena;
    }
    if (endArena->basePtr > savedScratchCursor || endArena->warnPtr < savedScratchCursor) {
        GXDebug_ReportAssertFailure("GXGnmTextureAndShaderStageInitializer.cpp", 0x233u, "tlsScratchCursorInRange");
    }
    if (endArena->curPtr > savedScratchCursor) {
        endArena->curPtr = savedScratchCursor;
    }

    return resultStatus;
}

extern bool CGSemanticGnm_ParseSemanticName(uint32_t* outSemanticType, const char* semanticName); // 0x26b7e60

// [VMA: 0x216bc70 - 0x216c131] (size: 1217 bytes)
// Translates: sub_216bc70
// Parses a raw 'Shdr' PSSL binary container, matches vertex input semantics, and compiles a GNM fetch shader.
uint32_t GXGnmFetchShader_CompileFromShdrBinary(
    GXCompiledFetchShader* outFetchShader,
    void* unusedDevice,
    const GXInputElementDesc* inputElements,
    uint32_t numInputElements,
    const uint8_t* shdrContainer,
    uint32_t shdrSize,
    GXMemoryAllocator* sysAllocator,
    GXMemoryAllocator* garlicAllocator)
{
    if (*reinterpret_cast<const uint32_t*>(shdrContainer + 0x24) != 0x72646853u) {
        return 8u;
    }

    struct PsslParsedShdrInfo {
        uint8_t  rawHeader[0x28];    // +0x00 (rbp-0xb0)
        uint8_t  numInputAttrs;      // +0x28 (rbp-0x88)
        uint8_t  pad29[0x2f];        // +0x29
        const uint8_t* inputAttrTable; // +0x58 (rbp-0x58)
        uint8_t  tail[0x20];         // +0x60
    } shdrInfo = {};

    GXPsslShaderBinaryInfo_Construct(&shdrInfo);
    uint32_t resultStatus = 8u;
    if (GXPsslShaderBinaryInfo_ParseFromMemory(&shdrInfo, shdrContainer, shdrSize) == 0) {
        uint32_t numShaderInputs = shdrInfo.numInputAttrs;
        uint32_t scratchByteSize = ((numShaderInputs + numInputElements) * 4u) + 0x100u;

        GXTlsScratchArena* arena = g_TlsScratchArena;
        uint8_t* savedScratchCursor = nullptr;
        if (arena != nullptr) {
            savedScratchCursor = arena->curPtr;
        } else {
            GXTlsScratchArena_InitializeThreadLocal();
            arena = g_TlsScratchArena;
            savedScratchCursor = arena->curPtr;
            if (arena == nullptr) {
                GXTlsScratchArena_InitializeThreadLocal();
                arena = g_TlsScratchArena;
            }
        }

        uint8_t* curCursor = arena->curPtr;
        uint8_t* nextCursor = curCursor + scratchByteSize;
        uint32_t* scratchBuf = nullptr;
        if (nextCursor <= arena->endPtr) {
            arena->curPtr = nextCursor;
            scratchBuf = reinterpret_cast<uint32_t*>(curCursor);
        }

        GXMemoryAllocator* heapFallbackOwner = nullptr;
        if (sysAllocator != nullptr && scratchBuf == nullptr) {
            scratchBuf = static_cast<uint32_t*>(
                sysAllocator->vtable->Allocate(sysAllocator, scratchByteSize));
            heapFallbackOwner = sysAllocator;
        }

        if (scratchBuf == nullptr) {
            resultStatus = 9u;
        } else {
            uint32_t* remappedVgprTable = scratchBuf;
            uint32_t* elementToInputMap = scratchBuf + numShaderInputs;
            if (numInputElements != 0) {
                std::memset(elementToInputMap, 0xff, static_cast<size_t>(numInputElements) * 4u);
            }

            size_t totalMapOffset = static_cast<size_t>(numShaderInputs) + static_cast<size_t>(numInputElements);
            uint32_t* semanticRemapTable = scratchBuf + totalMapOffset;
            uint32_t* inputToElementMap = scratchBuf + totalMapOffset + 0x20u;
            std::memset(semanticRemapTable, 0xff, 0x100u);

            bool matchFailed = false;
            if (numShaderInputs != 0) {
                const uint8_t* attrTable = shdrInfo.inputAttrTable;
                if (numInputElements != 0) {
                    for (uint32_t inIdx = 0; inIdx < numShaderInputs; ++inIdx) {
                        const uint8_t* attrEntry = attrTable + (static_cast<size_t>(inIdx) << 4);
                        uint32_t semIndex = attrEntry[0x02];
                        uint32_t nameRelOffset = *reinterpret_cast<const uint32_t*>(attrEntry + 0x0c);
                        const char* semName = reinterpret_cast<const char*>(attrEntry + 0x0c + nameRelOffset);

                        uint32_t parsedSemType = 0;
                        if (!CGSemanticGnm_ParseSemanticName(&parsedSemType, semName)) {
                            continue;
                        }

                        uint32_t matchedElemIdx = numInputElements;
                        for (uint32_t elemIdx = 0; elemIdx < numInputElements; ++elemIdx) {
                            if (inputElements[elemIdx].semanticType == parsedSemType &&
                                inputElements[elemIdx].semanticIndex == semIndex) {
                                matchedElemIdx = elemIdx;
                                break;
                            }
                        }

                        if (matchedElemIdx >= numInputElements) {
                            matchFailed = true;
                            resultStatus = 8u;
                            break;
                        }

                        elementToInputMap[matchedElemIdx] = inIdx;
                        semanticRemapTable[inIdx] = matchedElemIdx;
                        inputToElementMap[inIdx] = attrEntry[0x03];
                        remappedVgprTable[inIdx] = CGFormatGnm_ConvertStepRateToFetchRate(
                            inputElements[matchedElemIdx].inputSlotClass);
                    }
                } else {
                    for (uint32_t inIdx = 0; inIdx < numShaderInputs; ++inIdx) {
                        const uint8_t* attrEntry = attrTable + (static_cast<size_t>(inIdx) << 4);
                        uint32_t nameRelOffset = *reinterpret_cast<const uint32_t*>(attrEntry + 0x0c);
                        const char* semName = reinterpret_cast<const char*>(attrEntry + 0x0c + nameRelOffset);
                        uint32_t parsedSemType = 0;
                        if (CGSemanticGnm_ParseSemanticName(&parsedSemType, semName)) {
                            matchFailed = true;
                            resultStatus = 8u;
                            break;
                        }
                    }
                }
            }

            if (!matchFailed) {
                GXPsslEmbeddedHwHeaderInfo hwHdrInfo = {};
                GXPsslContainer_ExtractEmbeddedHwShaderHeader(&hwHdrInfo, shdrContainer);

                GXFetchShaderBuildContext buildCtx = {};
                buildCtx.vsOrLsEsBinaryHeader = hwHdrInfo.hwShaderHeader;
                buildCtx.shaderStageType = shdrContainer[0x2c];
                buildCtx.remappedVgprTable = remappedVgprTable;
                buildCtx.elementToInputMap = elementToInputMap;
                buildCtx.inputToElementMap = semanticRemapTable;
                buildCtx.semanticRemapTable = inputToElementMap;
                buildCtx.numShaderInputs = numShaderInputs;

                resultStatus = GXGnmFetchShader_GenerateAndRemapInputTable(
                    outFetchShader,
                    inputElements,
                    numInputElements,
                    &buildCtx,
                    sysAllocator,
                    garlicAllocator);
            }

            if (heapFallbackOwner != nullptr) {
                heapFallbackOwner->vtable->FreeAligned(heapFallbackOwner, scratchBuf);
            }
        }

        GXTlsScratchArena* endArena = g_TlsScratchArena;
        if (endArena == nullptr) {
            GXTlsScratchArena_InitializeThreadLocal();
            endArena = g_TlsScratchArena;
        }
        if (endArena->basePtr > savedScratchCursor || endArena->warnPtr < savedScratchCursor) {
            GXDebug_ReportAssertFailure("GXGnmTextureAndShaderStageInitializer.cpp", 0x233u, "tlsScratchCursorInRange");
        }
        if (endArena->curPtr > savedScratchCursor) {
            endArena->curPtr = savedScratchCursor;
        }
    }

    GXPsslShaderBinaryInfo_Destroy(&shdrInfo);
    return resultStatus;
}

// ============================================================================
// Group 6: Primitive Topology, Graphics Context Rings & Display Flip Queue
// ============================================================================

// [VMA: 0x216bb60 - 0x216bbd4] (size: 116 bytes)
// Translates: sub_216bb60
// Maps a GX primitive topology enum to its vertex count per primitive (1, 2, 3, 4, or 0).
uint32_t GXGnmTopology_GetVerticesPerPrimitive(
    void* unusedSelf,
    uint32_t* outVerticesPerPrim,
    uint32_t unusedArg,
    uint32_t topologyType)
{
    if (topologyType <= 15u) {
        switch (topologyType) {
            case 1u:
                *outVerticesPerPrim = 1u;
                return 0;
            case 2u:
                *outVerticesPerPrim = 2u;
                return 0;
            case 4u:
                *outVerticesPerPrim = 3u;
                return 0;
            case 8u:
                *outVerticesPerPrim = 4u;
                return 0;
            default:
                *outVerticesPerPrim = 0u;
                return 0;
        }
    }
    if (topologyType == 16u) {
        *outVerticesPerPrim = 4u;
        return 0;
    }
    *outVerticesPerPrim = 0u;
    return 0;
}

struct GXFlipQueueCoordinator {
    uint8_t  pad00[0x67];
    uint8_t  hasAlignedSubObject; // +0x67
    uint8_t  pad68[0x70];
    void*    mutexVtable;         // +0xd8
    uint8_t  scePthreadMutex[8];  // +0xe0
    uint8_t  isMutexInitialized;  // +0xe8
    uint8_t  padE9[7];            // +0xe9 (total size 0xf0)
};

struct GXRefCountedMutexObject {
    void*    vtable;             // +0x00
    uint8_t  scePthreadMutex[8]; // +0x08
    uint8_t  isInitialized;      // +0x10
    uint8_t  pad11[7];           // +0x11 (total size 0x18)
};

struct GXGnmGraphicsContext {
    void*                   dcbRingBuffer;       // +0x00
    void*                   ccbRingBuffer;       // +0x08
    void*                   cueHeapMemory;       // +0x10
    GXFlipQueueCoordinator* flipCoordinator;     // +0x18
    GXMemoryAllocator*      sysAllocator;        // +0x20
    GXMemoryAllocator*      onionAllocator;      // +0x28
    GXMemoryAllocator*      garlicAllocator;     // +0x30
    uint32_t                cueRingEntries;      // +0x38
    uint32_t                pad3c;               // +0x3c
    uint8_t                 gnmDrawCmdCtx[0xd8]; // +0x40..+0x117 (dcb at +0x80)
    GXRefCountedMutexObject* syncMutexObj;       // +0x118
    uint8_t                 isComputeDispatcherInitialized; // +0x120
    uint8_t                 isDebugMarkersEnabled;          // +0x121
    uint8_t                 pad122[6];                      // +0x122
};

// [VMA: 0x216bb40 - 0x216bb5c] (size: 28 bytes)
// Translates: sub_216bb40
// Returns true if dcbRingBuffer (+0x00), ccbRingBuffer (+0x08), and sysAllocator (+0x20) are all non-null.
bool GXGnmGraphicsContext_IsInitialized(const GXGnmGraphicsContext* self) {
    if (self->dcbRingBuffer == nullptr) {
        return false;
    }
    if (self->ccbRingBuffer == nullptr) {
        return false;
    }
    return self->sysAllocator != nullptr;
}

extern void GXGnmCommandContext_ReleaseResources(void* cmdCtx); // 0x21682c0
extern void CGComputeGnm_ShutdownGlobalDispatcher(); // 0x26b9c10
extern void CGContextGnm_FlushAndShutdownGlobalState(); // 0x26b2300
extern void CGRingGnm_ShutdownGlobalRingManager(); // 0x26d41c0
extern void CGDrawCtxGnm_DestroyCommandBuffers(void* gnmDrawCmdCtx); // 0x26b12f0
extern void CGFlipCoordGnm_ShutdownQueues(GXFlipQueueCoordinator* coord); // 0x26a9ca0
extern bool GXMutex_ShouldDestroyOSMutex(void* mutexObj); // 0x207ee80
extern int32_t ScePthreadMutex_Destroy(void* mutexHandle); // 0x2bbfee8
extern void CGSubObject_DestroyAligned(void* alignedSubObj); // 0x263bc40
extern void CGRingGnm_DestroyRingBuffer(void* ringBuf); // 0x26d3b50
#undef g_Vtable_GXMutexWrapper
#define g_Vtable_GXMutexWrapper (*reinterpret_cast<void**>(GX_VMA(0x52b6220ULL)))
#undef g_Vtable_GXMutexBase
#define g_Vtable_GXMutexBase (*reinterpret_cast<void**>(GX_VMA(0x53b2fa0ULL)))

static void DestroyInlineCoordinatorMutex(GXFlipQueueCoordinator* coord) {
    const char* kFile = reinterpret_cast<const char*>(GX_VMA(0x493c20eULL));
    const char* kMsgBusy = reinterpret_cast<const char*>(GX_VMA(0x493c275ULL));
    const char* kMsgOk = reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL));
    coord->mutexVtable = reinterpret_cast<uint8_t*>(&g_Vtable_GXMutexWrapper) + 0x10;
    if (GXMutex_ShouldDestroyOSMutex(&coord->mutexVtable)) {
        int32_t err = ScePthreadMutex_Destroy(&coord->scePthreadMutex);
        if (err != static_cast<int32_t>(0x80020010u)) {
            if (err != 0) {
                if (err == static_cast<int32_t>(0x80020016u)) {
                    GXDebug_ReportAssertFailure(kFile, 0x55u, kMsgBusy);
                }
                GXDebug_ReportAssertFailure(kFile, 0x57u, kMsgOk);
            }
            coord->isMutexInitialized = 0;
        }
    }
    coord->mutexVtable = &g_Vtable_GXMutexBase;
    if (coord->isMutexInitialized != 0) {
        int32_t err = ScePthreadMutex_Destroy(&coord->scePthreadMutex);
        if (err != static_cast<int32_t>(0x80020010u)) {
            if (err != 0) {
                if (err == static_cast<int32_t>(0x80020016u)) {
                    GXDebug_ReportAssertFailure(kFile, 0x55u, kMsgBusy);
                }
                GXDebug_ReportAssertFailure(kFile, 0x57u, kMsgOk);
            }
            coord->isMutexInitialized = 0;
        }
    }
    if (coord->hasAlignedSubObject != 0) {
        uintptr_t alignPad = (0u - static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(coord))) & 7u;
        CGSubObject_DestroyAligned(reinterpret_cast<uint8_t*>(coord) + alignPad);
        coord->hasAlignedSubObject = 0;
    }
}

// [VMA: 0x216b900 - 0x216bb3d] (size: 573 bytes)
// Translates: sub_216b900
// Shuts down a GXGnmGraphicsContext, releasing command rings, CUE heap, flip coordinator, and OS mutexes.
void GXGnmGraphicsContext_ShutdownAndReleaseRings(GXGnmGraphicsContext* self, void* cmdCtx) {
    GXGnmCommandContext_ReleaseResources(cmdCtx);
    if (self->isComputeDispatcherInitialized != 0) {
        CGComputeGnm_ShutdownGlobalDispatcher();
    }
    CGContextGnm_FlushAndShutdownGlobalState();
    CGRingGnm_ShutdownGlobalRingManager();
    CGDrawCtxGnm_DestroyCommandBuffers(&self->gnmDrawCmdCtx);

    if (self->garlicAllocator != nullptr && self->cueHeapMemory != nullptr) {
        self->garlicAllocator->vtable->FreeAligned(self->garlicAllocator, self->cueHeapMemory);
        self->cueHeapMemory = nullptr;
    }

    GXMemoryAllocator* sysAlloc = self->sysAllocator;
    if (sysAlloc != nullptr) {
        if (self->syncMutexObj != nullptr) {
            GXRefCountedMutexObject* mutexObj = self->syncMutexObj;
            void (*dtorFn)(GXRefCountedMutexObject*) =
                *reinterpret_cast<void (**)(GXRefCountedMutexObject*)>(mutexObj->vtable);
            dtorFn(mutexObj);
            sysAlloc->vtable->FreeAligned(sysAlloc, mutexObj);
            self->syncMutexObj = nullptr;
        }

        if (self->flipCoordinator != nullptr) {
            CGFlipCoordGnm_ShutdownQueues(self->flipCoordinator);
            GXFlipQueueCoordinator* coord = self->flipCoordinator;
            if (coord != nullptr) {
                GXMemoryAllocator* coordAlloc = self->sysAllocator;
                DestroyInlineCoordinatorMutex(coord);
                coordAlloc->vtable->FreeAligned(coordAlloc, coord);
                self->flipCoordinator = nullptr;
            }
        }

        if (self->ccbRingBuffer != nullptr) {
            void* ccbRing = self->ccbRingBuffer;
            GXMemoryAllocator* ringAlloc = self->sysAllocator;
            CGRingGnm_DestroyRingBuffer(ccbRing);
            ringAlloc->vtable->FreeAligned(ringAlloc, ccbRing);
            self->ccbRingBuffer = nullptr;
        }

        if (self->dcbRingBuffer != nullptr) {
            void* dcbRing = self->dcbRingBuffer;
            GXMemoryAllocator* ringAlloc = self->sysAllocator;
            CGRingGnm_DestroyRingBuffer(dcbRing);
            ringAlloc->vtable->FreeAligned(ringAlloc, dcbRing);
            self->dcbRingBuffer = nullptr;
        }
    }
}

struct GXGraphicsContextInitDesc {
    uint8_t  pad00[4];
    uint8_t  enableDebugMarkers; // +0x04
    uint8_t  pad05[0x3b];
    uint32_t dcbRingByteSize;    // +0x40
    uint32_t dcbRingSegmentCount;// +0x44
    uint32_t ccbRingByteSize;    // +0x48
    uint32_t ccbRingSegmentCount;// +0x4c
    uint32_t cueRingEntries;     // +0x50
    uint32_t cueHeapByteSize;    // +0x54
    uint8_t  pad58[8];           // +0x58
    GXMemoryAllocator* customFlipAllocator; // +0x60
};

extern uint32_t CGRingGnm_InitializeGlobalRingManager(GXMemoryAllocator* sysAlloc, GXMemoryAllocator* garlicAlloc, const GXGraphicsContextInitDesc* desc); // 0x26d3e20
extern bool CGDebugGnm_IsRazorOrDebugEnabled(); // 0x26bb8e0
extern void* CGRingGnm_CreateRingBuffer(GXMemoryAllocator* sysAlloc, GXMemoryAllocator* onionAlloc, uint32_t byteSize, uint32_t segmentCount); // 0x26d3a70
extern void CGFlipCoordGnm_Constructor(GXFlipQueueCoordinator* coord); // 0x26a9a80
extern bool CGFlipCoordGnm_InitializeQueues(GXFlipQueueCoordinator* coord, GXMemoryAllocator* flipAlloc, void* cueHeap, uint32_t cueHeapSize); // 0x26a9b50
extern uint32_t CGDrawCtxGnm_InitializeCommandBuffers(void* gnmDrawCmdCtx, uint32_t flags0, uint32_t flags1, uint32_t chunkBytes, void* dcbRing, void* flipCoord, GXMemoryAllocator* sysAlloc, GXMemoryAllocator* onionAlloc); // 0x26b1130
extern int32_t GXMutex_InitializeOSMutex(void* mutexHandle); // 0x2083fe0
extern uint32_t GXGnmCommandContext_InitializeWithRings(void* cmdCtx, uint32_t flags, GXGnmGraphicsContext* gfxCtx, GXMemoryAllocator* sysAlloc, GXMemoryAllocator* garlicAlloc); // 0x2168080
extern uint32_t CGContextGnm_InitializeGlobalHardwareTables(GXGnmGraphicsContext* gfxCtx, GXMemoryAllocator* sysAlloc, GXMemoryAllocator* garlicAlloc); // 0x26b2030
extern uint32_t CGComputeGnm_InitializeGlobalDispatcher(GXMemoryAllocator* sysAlloc); // 0x26b9c00
extern "C" __attribute__((weak)) void BbGoldenEngine_UpgradeResourceCache(void** ioHeap, uint32_t* ioSize) {
    (void)ioHeap;
    (void)ioSize;
}

// [VMA: 0x216b0f0 - 0x216b8f8] (size: 2056 bytes)
// Translates: sub_216b0f0
// Validates ring sizes and initializes DCB/CCB rings, CUE heap, flip coordinator, draw command buffers, and global compute/context state.
uint32_t GXGnmGraphicsContext_InitializeRingsAndCommandBuffers(
    GXGnmGraphicsContext* self,
    void* cmdCtx,
    void* unusedArg,
    const GXGraphicsContextInitDesc* initDesc,
    GXMemoryAllocator* sysAllocator,
    GXMemoryAllocator* onionAllocator,
    GXMemoryAllocator* garlicAllocator)
{
    if ((initDesc->dcbRingByteSize - 0x8000u) > 0x3f8000u) {
        return 8u;
    }
    if (initDesc->dcbRingSegmentCount < 3u) {
        return 8u;
    }
    if ((initDesc->ccbRingByteSize - 0x8000u) > 0x3f8000u) {
        return 8u;
    }
    if (initDesc->ccbRingSegmentCount == 0u || initDesc->cueHeapByteSize == 0u) {
        return 8u;
    }

    uint32_t ringInitErr = CGRingGnm_InitializeGlobalRingManager(sysAllocator, garlicAllocator, initDesc);
    if (ringInitErr != 0) {
        return ringInitErr;
    }

    self->sysAllocator = sysAllocator;
    self->onionAllocator = onionAllocator;
    self->garlicAllocator = garlicAllocator;
    self->cueRingEntries = initDesc->cueRingEntries;
    self->isComputeDispatcherInitialized = 0;
    self->isDebugMarkersEnabled = CGDebugGnm_IsRazorOrDebugEnabled() ? initDesc->enableDebugMarkers : 0u;

    void* dcbRing = CGRingGnm_CreateRingBuffer(
        sysAllocator, onionAllocator, initDesc->dcbRingByteSize, initDesc->dcbRingSegmentCount);
    self->dcbRingBuffer = dcbRing;
    if (dcbRing == nullptr) {
        CGRingGnm_ShutdownGlobalRingManager();
        return 9u;
    }

    void* ccbRing = CGRingGnm_CreateRingBuffer(
        sysAllocator, onionAllocator, initDesc->ccbRingByteSize, initDesc->ccbRingSegmentCount);
    self->ccbRingBuffer = ccbRing;
    if (ccbRing == nullptr) {
        if (self->dcbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->dcbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->dcbRingBuffer);
        }
        CGRingGnm_ShutdownGlobalRingManager();
        return 9u;
    }

    void* cueHeap = garlicAllocator->vtable->AllocateAligned(
        garlicAllocator, initDesc->cueHeapByteSize, 16u);
    BbGoldenEngine_UpgradeResourceCache(&cueHeap, const_cast<uint32_t*>(&initDesc->cueHeapByteSize));
    self->cueHeapMemory = cueHeap;
    if (cueHeap != nullptr) {
        gx_direct_vk::RegisterTransientCueHeap(cueHeap, initDesc->cueHeapByteSize);
    }
    if (cueHeap == nullptr) {
        if (self->ccbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->ccbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->ccbRingBuffer);
        }
        if (self->dcbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->dcbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->dcbRingBuffer);
        }
        CGRingGnm_ShutdownGlobalRingManager();
        return 9u;
    }

    GXFlipQueueCoordinator* coord = static_cast<GXFlipQueueCoordinator*>(
        sysAllocator->vtable->AllocateAligned(sysAllocator, sizeof(GXFlipQueueCoordinator), 8u));
    if (coord == nullptr) {
        self->flipCoordinator = nullptr;
        garlicAllocator->vtable->FreeAligned(garlicAllocator, self->cueHeapMemory);
        if (self->ccbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->ccbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->ccbRingBuffer);
        }
        if (self->dcbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->dcbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->dcbRingBuffer);
        }
        CGRingGnm_ShutdownGlobalRingManager();
        return 9u;
    }

    CGFlipCoordGnm_Constructor(coord);
    self->flipCoordinator = coord;

    GXMemoryAllocator* flipAlloc = (initDesc->customFlipAllocator != nullptr)
        ? initDesc->customFlipAllocator
        : onionAllocator;
    if (!CGFlipCoordGnm_InitializeQueues(coord, flipAlloc, self->cueHeapMemory, initDesc->cueHeapByteSize)) {
        if (self->flipCoordinator != nullptr) {
            DestroyInlineCoordinatorMutex(self->flipCoordinator);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->flipCoordinator);
        }
        garlicAllocator->vtable->FreeAligned(garlicAllocator, self->cueHeapMemory);
        if (self->ccbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->ccbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->ccbRingBuffer);
        }
        if (self->dcbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->dcbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->dcbRingBuffer);
        }
        CGRingGnm_ShutdownGlobalRingManager();
        return 5u;
    }

    uint32_t drawCtxErr = CGDrawCtxGnm_InitializeCommandBuffers(
        &self->gnmDrawCmdCtx, 0u, 0u, 0x1000u, self->dcbRingBuffer, self->flipCoordinator, sysAllocator, onionAllocator);
    if (drawCtxErr != 0) {
        CGFlipCoordGnm_ShutdownQueues(self->flipCoordinator);
        if (self->flipCoordinator != nullptr) {
            DestroyInlineCoordinatorMutex(self->flipCoordinator);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->flipCoordinator);
        }
        garlicAllocator->vtable->FreeAligned(garlicAllocator, self->cueHeapMemory);
        if (self->ccbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->ccbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->ccbRingBuffer);
        }
        if (self->dcbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->dcbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->dcbRingBuffer);
        }
        CGRingGnm_ShutdownGlobalRingManager();
        return drawCtxErr;
    }

    GXRefCountedMutexObject* syncMutex = static_cast<GXRefCountedMutexObject*>(
        sysAllocator->vtable->AllocateAligned(sysAllocator, sizeof(GXRefCountedMutexObject), 8u));
    if (syncMutex == nullptr) {
        self->syncMutexObj = nullptr;
        CGFlipCoordGnm_ShutdownQueues(self->flipCoordinator);
        if (self->flipCoordinator != nullptr) {
            DestroyInlineCoordinatorMutex(self->flipCoordinator);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->flipCoordinator);
        }
        garlicAllocator->vtable->FreeAligned(garlicAllocator, self->cueHeapMemory);
        CGDrawCtxGnm_DestroyCommandBuffers(&self->gnmDrawCmdCtx);
        if (self->ccbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->ccbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->ccbRingBuffer);
        }
        if (self->dcbRingBuffer != nullptr) {
            CGRingGnm_DestroyRingBuffer(self->dcbRingBuffer);
            sysAllocator->vtable->FreeAligned(sysAllocator, self->dcbRingBuffer);
        }
        CGRingGnm_ShutdownGlobalRingManager();
        return 9u;
    }

    syncMutex->isInitialized = 0;
    syncMutex->vtable = reinterpret_cast<uint8_t*>(&g_Vtable_GXMutexWrapper) + 0x10;
    if (!GXMutex_ShouldDestroyOSMutex(syncMutex)) {
        if (GXMutex_InitializeOSMutex(&syncMutex->scePthreadMutex) == 0) {
            syncMutex->isInitialized = 1;
        }
    }
    self->syncMutexObj = syncMutex;

    uint32_t cmdCtxErr = GXGnmCommandContext_InitializeWithRings(
        cmdCtx, 0u, self, sysAllocator, garlicAllocator);
    if (cmdCtxErr != 0) {
        GXGnmGraphicsContext_ShutdownAndReleaseRings(self, cmdCtx);
        return cmdCtxErr;
    }

    // Copy DCB pointer at self + 0x80 into cmdCtx + 0xb728
    *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(cmdCtx) + 0xb728) =
        *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(self) + 0x80);

    uint32_t hwTableErr = CGContextGnm_InitializeGlobalHardwareTables(self, sysAllocator, garlicAllocator);
    if (hwTableErr != 0) {
        GXGnmGraphicsContext_ShutdownAndReleaseRings(self, cmdCtx);
        return hwTableErr;
    }

    self->isComputeDispatcherInitialized =
        (CGComputeGnm_InitializeGlobalDispatcher(sysAllocator) == 0) ? 1u : 0u;
    return 0;
}

extern void CGDrawCtxGnm_ResetCommandBuffers(void* gnmDrawCmdCtx); // 0x26b10a0
extern void CGDrawCtxGnm_Constructor(void* gnmDrawCmdCtx);         // 0x26b0f10

// [VMA: 0x216b0e0 - 0x216b0e9] (size: 9 bytes)
// Translates: sub_216b0e0
// Forwards to CGDrawCtxGnm_ResetCommandBuffers on the embedded draw context at +0x40.
void GXGnmGraphicsContext_ResetCommandBuffers(GXGnmGraphicsContext* self) {
    CGDrawCtxGnm_ResetCommandBuffers(&self->gnmDrawCmdCtx);
}

// [VMA: 0x216b070 - 0x216b0d9] (size: 105 bytes)
// Translates: sub_216b070
// Initializes a GXGnmGraphicsContext structure and constructs its embedded draw command context at +0x40.
void GXGnmGraphicsContext_Constructor(GXGnmGraphicsContext* self) {
    self->cueRingEntries = 0;
    self->garlicAllocator = nullptr;
    self->onionAllocator = nullptr;
    self->sysAllocator = nullptr;
    self->flipCoordinator = nullptr;
    self->cueHeapMemory = nullptr;
    self->ccbRingBuffer = nullptr;
    self->dcbRingBuffer = nullptr;
    CGDrawCtxGnm_Constructor(&self->gnmDrawCmdCtx);
    self->syncMutexObj = nullptr;
    self->isComputeDispatcherInitialized = 0;
}

struct GXCommandListRingWithMutex {
    uint8_t pad00[0x20];
    void*   mutexVtable;        // +0x20
    uint8_t scePthreadMutex[8]; // +0x28
    uint8_t isMutexInitialized; // +0x30
};

// [VMA: 0x216af80 - 0x216b063] (size: 227 bytes)
// Translates: sub_216af80
// Destroys the embedded OS mutex inside a GXCommandListRingWithMutex at +0x20.
void GXGnmCommandListRing_DestroyMutex(GXCommandListRingWithMutex* self) {
    self->mutexVtable = reinterpret_cast<uint8_t*>(&g_Vtable_GXMutexWrapper) + 0x10;
    if (GXMutex_ShouldDestroyOSMutex(&self->mutexVtable)) {
        int32_t err = ScePthreadMutex_Destroy(&self->scePthreadMutex);
        if (err != static_cast<int32_t>(0x80020010u)) {
            if (err != 0) {
                if (err == static_cast<int32_t>(0x80020016u)) {
                    GXDebug_ReportAssertFailure("GXMutex.h", 0x55u, "err != SCE_KERNEL_ERROR_EBUSY");
                }
                GXDebug_ReportAssertFailure("GXMutex.h", 0x57u, "err == SCE_OK");
            }
            self->isMutexInitialized = 0;
        }
    }
    self->mutexVtable = &g_Vtable_GXMutexBase;
    if (self->isMutexInitialized != 0) {
        int32_t err = ScePthreadMutex_Destroy(&self->scePthreadMutex);
        if (err != static_cast<int32_t>(0x80020010u)) {
            if (err != 0) {
                if (err == static_cast<int32_t>(0x80020016u)) {
                    GXDebug_ReportAssertFailure("GXMutex.h", 0x55u, "err != SCE_KERNEL_ERROR_EBUSY");
                }
                GXDebug_ReportAssertFailure("GXMutex.h", 0x57u, "err == SCE_OK");
            }
            self->isMutexInitialized = 0;
        }
    }
}

struct GXGnmDeviceWithFlipQueue {
    void* vtable;        // +0x00
    void* flipQueueImpl; // +0x08
};

extern void CGFlipQueueGnm_WaitAndFlush(void* flipQueueImpl);         // 0x26aa6a0
extern void CGFlipQueueGnm_AllocateAndSubmit(void* flipQueueImpl);    // 0x26aa2e0
extern void CGFlipQueueGnm_WaitForCompletion(void* flipQueueImpl);    // 0x26aa5a0

// [VMA: 0x216af70 - 0x216af79] (size: 9 bytes)
// Translates: sub_216af70
// Forwards to CGFlipQueueGnm_WaitAndFlush using the internal flip queue pointer at +0x08.
void GXGnmDevice_WaitAndFlushDisplayFlipQueue(GXGnmDeviceWithFlipQueue* self) {
    CGFlipQueueGnm_WaitAndFlush(self->flipQueueImpl);
}

// [VMA: 0x216af60 - 0x216af69] (size: 9 bytes)
// Translates: sub_216af60
// Forwards to CGFlipQueueGnm_AllocateAndSubmit using the internal flip queue pointer at +0x08.
void GXGnmDevice_AllocateAndSubmitFlipCommandBuffer(GXGnmDeviceWithFlipQueue* self) {
    CGFlipQueueGnm_AllocateAndSubmit(self->flipQueueImpl);
}

// [VMA: 0x216af50 - 0x216af59] (size: 9 bytes)
// Translates: sub_216af50
// Forwards to CGFlipQueueGnm_WaitForCompletion using the internal flip queue pointer at +0x08.
void GXGnmDevice_WaitForDisplayFlipCompletion(GXGnmDeviceWithFlipQueue* self) {
    CGFlipQueueGnm_WaitForCompletion(self->flipQueueImpl);
}

} // namespace GXGraphicsFramework
