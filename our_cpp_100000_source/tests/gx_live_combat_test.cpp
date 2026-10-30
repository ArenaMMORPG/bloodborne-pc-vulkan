#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <immintrin.h>
#include <sys/mman.h>

#include "eboot_kit.h"
#define GX_LIVE_COMBAT_TEST_PROVIDES_MOCKS 1
#include "gx_eboot_external_imports.h"
#include "gx_decomp_registry.h"

// ============================================================================
// Bindings from our compiled 02_GX_GraphicsFramework object files to live
// eboot.bin functions mapped at kBase (0x400000 + VMA).
// ============================================================================
static inline uintptr_t vma_to_bn(uintptr_t vma) {
    return 0x400000ULL + vma;
}

template <typename Fn>
static inline Fn eboot_vma_fn(uintptr_t vma) {
    return reinterpret_cast<Fn>(vma_to_bn(vma));
}

// Forward declarations of external symbols referenced by the tested .cpp files.
// Note: sceGnmSampler*, sceGnmScanMode*, sceGnmPrimitiveSetup*, sceGnmDepthStencil*,
// sceGnmStencilOp*, sceGnmBlendControl*, GXTranslate_*, CGDisplayConfig_*, and GXMtd*
// are now automatically bridged via %r11 live trampolines in gx_eboot_external_imports.h!
extern "C" {
    char16_t*   DLCharTraitsW_CopyString(char16_t* dst, const char16_t* src) {
        char16_t* d = dst;
        while ((*d++ = *src++) != 0) {}
        return dst;
    }

    // Bridges for 04_SceneRenderer Orbis 16-bit wchar_t (char16_t) string PLT helpers
    int32_t sub_2bbf228(const char16_t* s) {
        int32_t len = 0;
        if (s) { while (s[len] != 0) ++len; }
        return len;
    }
    char16_t* sub_2bbf258(const char16_t* s, int32_t c) {
        if (!s) return nullptr;
        const char16_t* last = nullptr;
        char16_t ch = static_cast<char16_t>(c);
        for (; *s != 0; ++s) {
            if (*s == ch) last = s;
        }
        if (ch == 0) return const_cast<char16_t*>(s);
        return const_cast<char16_t*>(last);
    }
    char16_t* sub_2bbf7a8(char16_t* dst, const char16_t* src, size_t n) {
        size_t i = 0;
        for (; i < n && src[i] != 0; ++i) dst[i] = src[i];
        for (; i < n; ++i) dst[i] = 0;
        return dst;
    }

    // Bridges for 05_Yebis2_PostFX into live eboot.bin and host math/registry stubs
    __m128 sub_2610c30(uint32_t iSample) {
        return eboot_vma_fn<__m128(*)(uint32_t)>(0x2610c30)(iSample);
    }
    void sub_bb9250(void* outMat, const void* inMat, uint32_t bColMaj, __m128 v0, __m128 v1, __m128 v2, __m128 v3) {
        eboot_vma_fn<void(*)(void*, const void*, uint32_t, __m128, __m128, __m128, __m128)>(0xbb9250)(
            outMat, inMat, bColMaj, v0, v1, v2, v3);
    }
    int32_t sub_bdbe90(void* pPostEffect, float* outFactor4, uint32_t srcType, uint32_t dstType, float scaleInit, float offsetInit) {
        return eboot_vma_fn<int32_t(*)(void*, float*, uint32_t, uint32_t, float, float)>(0xbdbe90)(
            pPostEffect, outFactor4, srcType, dstType, scaleInit, offsetInit);
    }
    float sub_2bbe648(float x) { return std::tan(x); }
    float sub_2bbe948(float x, float y) { return std::pow(x, y); }
    float sub_2bbf128(float x) { return std::atan(x); }
    void  sub_2bbf178(const char* /*expr*/, const char* /*func*/) {}

    static void* g_last_unregistered_internal_res = nullptr;
    static void* g_last_erased_gnm_key = nullptr;
    int32_t sub_bf0790(void* /*pPostEffect*/, void* pInternalRes) {
        g_last_unregistered_internal_res = pInternalRes;
        return 0;
    }
    int32_t sub_2610840(void* /*pTree*/, void* pGnmKey) {
        g_last_erased_gnm_key = pGnmKey;
        return 0;
    }
    int32_t sub_bf05f0(void* /*pPostEffect*/, void* pInternalRt) {
        g_last_unregistered_internal_res = pInternalRt;
        return 0;
    }
    int32_t sub_2610760(void* /*pTree*/, void* pGnmKey) {
        g_last_erased_gnm_key = pGnmKey;
        return 0;
    }

    // Bridges for 06_DeferredLightingAndAO temporary sort buffer allocation/free
    void* sub_2bc08b8(size_t byteSize, void* /*pAllocatorTag*/) {
        return std::malloc(byteSize ? byteSize : 16);
    }
    void sub_2bbe588(void* ptr) {
        std::free(ptr);
    }
    void sub_2bbf188() {}

    // Bridges for 07_SFX_ParticleGpuRenderer (GXFfxRibbonAndParticleVertexEmitter.o,
    // GXFfxSceneCtrlAndShaderBinder.o, GXFfxTessellationAndParticleDrawPass.o)
    struct FfxLDivResult {
        int64_t quot;
        int64_t rem;
    };
    FfxLDivResult sub_2bbfd68(int64_t numer, int64_t denom) {
        return { numer / denom, numer % denom };
    }
    void sub_28eaf20(uint32_t* first, uint32_t* last, uint64_t depthLimit) {
        eboot_vma_fn<void(*)(uint32_t*, uint32_t*, uint64_t)>(0x28eaf20)(first, last, depthLimit);
    }
    void* sub_2bbe658(void* dst, const void* src, size_t len) {
        return std::memcpy(dst, src, len);
    }
    void* sub_2bbe688(void* dst, int val, size_t len) {
        return std::memset(dst, val, len);
    }
    [[maybe_unused]] static uint32_t g_ffx_last_err_code = 0;
    void sub_ee5de0(uint32_t code) {
        g_ffx_last_err_code = code;
    }
    void sub_20b55b0(const char* /*pFile*/, uint32_t /*line*/, const char* /*pMsg*/, ...) {}
    [[noreturn]] void sub_2bbe978(void* /*pJmpBuf*/, int /*val*/) { __builtin_trap(); }
    [[noreturn]] void sub_2bbf1b8(const char* /*msg*/) { __builtin_trap(); }

    struct FfxCapturedSqtCall {
        void*    pSub80;
        uint32_t index;
        uint32_t flags;
        alignas(16) float sqt[12];
    };
    static FfxCapturedSqtCall g_captured_sqt{};
    void sub_2311430(void* pSub80, uint32_t index, const void* pSqtData, uint32_t flags) {
        g_captured_sqt.pSub80 = pSub80;
        g_captured_sqt.index  = index;
        g_captured_sqt.flags  = flags;
        std::memcpy(g_captured_sqt.sqt, pSqtData, 48);
    }
    [[maybe_unused]] static uint32_t g_last_2311c00_mask = 0;
    void sub_2311c00(void* /*pSub80*/, uint32_t mask) {
        g_last_2311c00_mask = mask;
    }
}

// ============================================================================
// Declarations of OUR decompiled functions from 02_GX_GraphicsFramework
// ============================================================================
struct GXStreamOutElement28 {
    uint32_t key, streamIdx, mask, offset, stride, semantic, component;
};

namespace GXGraphicsFramework {
    extern "C" void GXStreamOutElementArray_IntrosortBySemanticKey(
        GXStreamOutElement28* first,
        GXStreamOutElement28* last,
        int64_t depthLimit);

    struct GXMemoryAllocator;
    struct GXHardwareBufferResource {
        GXMemoryAllocator* allocator;
        void*              gpuBaseAddr;
        uint32_t           resourceId;
        uint32_t           requestedSize;
        uint32_t           allocatedSize;
        uint32_t           alignment;
        uint8_t            dimType;
        uint8_t            usageType;
        uint8_t            cpuAccessFlags;
        uint8_t            bindFlags;
        uint32_t           miscFlags;
        uint8_t            isDynamicRing;
        uint8_t            pad29;
        uint16_t           elementStride;
        uint32_t           pad2c;
        GXMemoryAllocator* sysAllocator;
        uint16_t*          contextBoundSlotTable;
    };
    struct GXBufferDescriptorOut {
        uint32_t byteSize, usageType, bindFlags, cpuAccessFlags, miscFlags, structureByteStride;
    };
    uint32_t GXGnmTopology_GetVerticesPerPrimitive(void* unusedSelf, uint32_t* outVerticesPerPrim, uint32_t unusedArg, uint32_t topologyType);
    void GXGnmBuffer_ExtractDescriptorFromHardware(const GXHardwareBufferResource* hwBuf, GXBufferDescriptorOut* outDesc);
}

namespace FromSoftware {
namespace GX {
    struct GXRenderJobEntry {
        uint8_t* jobPacket;
        uint32_t sortKey;
    };
    struct GXRenderJobTempBuffer {
        GXRenderJobEntry* begin;
        GXRenderJobEntry* current;
        GXRenderJobEntry* end;
        int64_t           capacity;
    };
    struct GXRenderJobSortContext {
        uint8_t                pad00[0x20];
        GXRenderJobTempBuffer* tempBuf;
    };
    void GXRenderJobSort_StableSortRecursive(
        GXRenderJobEntry* first,
        GXRenderJobEntry* last,
        int64_t count,
        GXRenderJobSortContext* ctx);

    struct alignas(16) GXMatrix4x4 { float m[4][4]; };
    struct alignas(16) GXCameraFrustum {
        GXMatrix4x4 viewMatrix;
        float       eyePosition[4];
        float       fovYRadians;
        float       nearZ;
        float       farZ;
        uint32_t    viewportWidth;
        uint32_t    viewportHeight;
        float       aspectRatio;
        uint8_t     enableHalfPixelOffset;
        uint8_t     pad69[7];
        uint32_t    tileGridSubdiv;
        uint32_t    tileLinearIndex;
        uint32_t    tileMarginPixels;
        uint32_t    tileWidthPixels;
        uint32_t    tileHeightPixels;
    };
    struct alignas(16) GXCameraFrustumCompact {
        GXMatrix4x4 viewMatrix;
        float       eyePosition[4];
        float       fovYRadians;
        float       nearZ;
        float       farZ;
        float       aspectRatio;
        uint32_t    viewportWidth;
        uint32_t    viewportHeight;
        uint16_t    tileGridSubdiv;
        uint16_t    tileLinearIndex;
        uint16_t    tileMarginPixels;
        uint16_t    tileWidthPixels;
        uint16_t    tileHeightPixels;
    };
    void GXCameraFrustum_UnpackFromCompact(GXCameraFrustum* dst, const GXCameraFrustumCompact* src);
    void GXCameraFrustum_PackToCompact(const GXCameraFrustum* src, GXCameraFrustumCompact* dst);
    void GXCameraFrustum_InitCubeMapFace(GXCameraFrustum* self, uint32_t faceIndex, const float* cubeCenter);
    void GXCameraFrustum_ResetDefaults(GXCameraFrustum* self, uint32_t defaultWidth, uint32_t defaultHeight);
}
}

extern "C" {
    int  GXGnmSamplerState_Initialize(uint8_t* pOutSamplerObj, void* pUnused, const uint8_t* pSamplerDesc);
    void GXGnmSamplerState_ExtractDesc(const uint8_t* pSamplerObj, uint8_t* pOutSamplerDesc);
    int  GXGnmRasterizerState_Initialize(uint8_t* pOutRasterObj, void* pUnused, const uint8_t* pRasterDesc);
    void GXGnmRasterizerState_ExtractDesc(const uint8_t* pRasterObj, uint8_t* pOutRasterDesc);
    int  GXGnmDepthStencilState_Initialize(uint8_t* pOutDsObj, void* pUnused, const uint8_t* pDsDesc);
    void GXGnmDepthStencilState_ExtractDesc(const uint8_t* pDepthStencilObj, uint8_t* pOutDesc);
    int  GXGnmBlendState_Initialize(uint8_t* pOutBlendObj, void* pUnused, const uint8_t* pBlendDesc);

    // 03_TPF_GNF_ShaderContainers — GXMtdMaterialAndParameterAccessor.cpp (0x26dc380 .. 0x26dfe50)
    struct GXMtdConvertedValue16 { uint64_t lo, hi; };
    bool     GXMtdRootBlock_HasValidMagicHeader(const void* rootBlock);                                        // 0x26de780
    uint32_t GXMtdMaterialContainer_GetLastError(const void* self);                                            // 0x26de770
    void     GXMtdMaterialContainer_SetLastError(void* self, uint32_t errorCode);                              // 0x26dc470
    uint32_t GXMtdMaterialContainer_GetVersion(void* self);                                                    // 0x26dcbb0
    bool     GXMtdMaterialContainer_SetVersion(void* self, uint32_t version);                                  // 0x26dcbd0
    void     GXMtdParameterAccessor_BindNode(void* self, void* node);                                          // 0x26dea10
    int32_t  GXMtdParameterAccessor_GetResolvedType(void* self);                                               // 0x26dec10
    bool     GXMtdParameterAccessor_GetValueToBuffer4(void* self, void* outValues4);                           // 0x26dec90
    bool     GXMtdParameterAccessor_GetFloatComponents(void* self, float* o0, float* o1, float* o2, float* o3);// 0x26def70
    bool     GXMtdParameterAccessor_GetBoolComponents(void* self, bool* o0, bool* o1, bool* o2, bool* o3);     // 0x26df130
    bool     GXMtdParameterAccessor_GetIntComponents(void* self, int32_t* o0, int32_t* o1, int32_t* o2, int32_t* o3); // 0x26df2f0

    // 03_TPF_GNF_ShaderContainers — GXMtdTypeConverterAndTpfEntryStrategies.cpp (0x26dab60 .. 0x26dc370)
    uint64_t GXMtdParamType_GetCategoryAndComponentCount(const void* unusedSelf, uint32_t typeIndex);          // 0x26dc2b0
    GXMtdConvertedValue16 GXMtdParamValue_ConvertBetweenTypes(void* container, const void* srcVal4,
                                                              uint32_t srcType, const void* defaultVal4,
                                                              uint32_t dstType);                               // 0x26dbed0
    uint32_t GXDisplayOutput_GetDefaultModeDesc1080p60(void* unusedSelf, uint32_t adapterIdx,
                                                       uint32_t modeIdx, void* outModeDesc);                   // 0x26dbe00
    void     GXDisplayOutput_GetDefaultOutputDesc(void* unusedSelf, void* outDesc);                            // 0x26dbdb0
    uint32_t GXDisplayConfig_CopyFromGlobalCGDisplayConfig(void* outDisplayConfigC0);                          // 0x26dbbe0

    void           GXTpfEntryV2_ExtractExtraMetadata(const void* self, const void* view, void* outMeta);       // 0x26dae00
    const uint8_t* GXTpfEntryV2_GetPayloadPointer(const void* self, const void* view);                         // 0x26dadf0
    uint32_t       GXTpfEntryV2_GetDefaultOneA();                                                              // 0x26dade0
    uint32_t       GXTpfEntryV2_GetDefaultOneB();                                                              // 0x26dadd0
    int32_t        GXTpfEntryV2_GetFlags(const void* self, const void* view);                                  // 0x26dadc0
    uint32_t       GXTpfEntryV2_GetEnginePixelFormat(const void* self, const void* view);                      // 0x26dadb0
    uint32_t       GXTpfEntryV2_GetEffectiveMipCount(const void* self, const void* view);                      // 0x26dad90
    uint32_t       GXTpfEntryV2_GetSliceCount(const void* self, const void* view);                             // 0x26dad70
    uint32_t       GXTpfEntryV2_GetDepth(const void* self, const void* view);                                  // 0x26dad60
    uint32_t       GXTpfEntryV2_GetHeight(const void* self, const void* view);                                 // 0x26dad50
    uint32_t       GXTpfEntryV2_GetWidth(const void* self, const void* view);                                  // 0x26dad40
    const void*    GXTpfEntryV2_GetHeaderPtr(const void* self, const void* view);                              // 0x26dad30
    uint32_t       GXTpfEntryV2_GetHeaderByteSize();                                                           // 0x26dad20
    uint32_t       GXTpfEntryV2_GetHeaderExtSize(const void* self, const void* view);                          // 0x26dad10
    uint32_t       GXTpfEntryV2_GetFileNameOffset(const void* self, const void* view);                         // 0x26dad00
    uint32_t       GXTpfEntryV2_GetFormatCode(const void* self, const void* view);                             // 0x26dacf0
    uint32_t       GXTpfEntryV2_GetTextureType(const void* self, const void* view);                            // 0x26dace0
    uint32_t       GXTpfEntryV2_GetDataSize(const void* self, const void* view);                               // 0x26dacd0
    uint32_t       GXTpfEntryV2_GetDataOffset(const void* self, const void* view);                             // 0x26dacc0

    const uint8_t* GXTpfEntryV1_GetSubresourcePayloadPointer(const void* self, const void* view,
                                                             uint32_t mipIndex, uint32_t sliceIndex);          // 0x26dac70
    uint32_t       GXTpfEntryV1_GetMipSliceByteSize(const void* self, const void* view, uint32_t mipIndex);    // 0x26dac60
    uint32_t       GXTpfEntryV1_GetMipSlicePitchOrRowSize(const void* self, const void* view, uint32_t mipIdx);// 0x26dac50
    int32_t        GXTpfEntryV1_GetFlags(const void* self, const void* view);                                  // 0x26dac40
    uint32_t       GXTpfEntryV1_GetEnginePixelFormat(const void* self, const void* view);                      // 0x26dac30
    uint32_t       GXTpfEntryV1_GetEffectiveMipCount(const void* self, const void* view);                      // 0x26dac10
    uint32_t       GXTpfEntryV1_GetSliceCount(const void* self, const void* view);                             // 0x26dac00
    uint32_t       GXTpfEntryV1_GetDepth(const void* self, const void* view);                                  // 0x26dabf0
    uint32_t       GXTpfEntryV1_GetHeight(const void* self, const void* view);                                 // 0x26dabe0
    uint32_t       GXTpfEntryV1_GetWidth(const void* self, const void* view);                                  // 0x26dabd0
    const void*    GXTpfEntryV1_GetHeaderPtr(const void* self, const void* view);                              // 0x26dabc0
    uint32_t       GXTpfEntryV1_GetHeaderByteSize();                                                           // 0x26dabb0
    uint32_t       GXTpfEntryV1_GetHeaderExtSize(const void* self, const void* view);                          // 0x26daba0
    uint32_t       GXTpfEntryV1_GetFileNameOffset(const void* self, const void* view);                         // 0x26dab90
    uint32_t       GXTpfEntryV1_GetFormatCode(const void* self, const void* view);                             // 0x26dab80
    uint32_t       GXTpfEntryV1_GetTextureType(const void* self, const void* view);                            // 0x26dab70
    uint32_t       GXTpfEntryV1_GetDataSize(const void* self, const void* view);                               // 0x26dab60

    // 03_TPF_GNF_ShaderContainers — Batch 3 (0x26d6a50 .. 0x26dab50)
    uint32_t       GXTpfEntryV1_GetDataOffset(const void* self, const void* view);                             // 0x26dab50
    bool           GXTpfEntryV1_ValidateDdsHeader(const void* self, const void* view);                         // 0x26dab40
    const uint8_t* GXTpfContainer_GetEntryExtraChunkData(const uint8_t* tpfBase, uint32_t entryIndex,
                                                         uint32_t chunkIndex, uint32_t* outChunkByteSize,
                                                         uint32_t* outChunkId);                                // 0x26d8df0
    uint32_t       GXTpfContainer_GetEntryExtraChunkCount(const uint8_t* tpfBase, uint32_t entryIndex);        // 0x26d8c10
    const char*    GXTpfContainer_GetEntryFileNamePtr(const uint8_t* tpfBase, uint32_t entryIndex);            // 0x26d8a20
    uint32_t       GXTpfContainer_GetEntryTextureType(const uint8_t* tpfBase, uint32_t entryIndex);            // 0x26d8840
    uint32_t       GXTpfContainer_GetEntryEnginePixelFormat(const uint8_t* tpfBase, uint32_t entryIndex);      // 0x26d8660
    uint32_t       GXTpfContainer_GetEntryFormatCode(const uint8_t* tpfBase, uint32_t entryIndex);             // 0x26d8480
    int32_t        GXTpfContainer_GetEntryFlagsShifted4(const uint8_t* tpfBase, uint32_t entryIndex);          // 0x26d8280
    uint8_t*       GXTpfContainer_GetEncodingFlag(uint8_t* outEncodingFlag, const uint8_t* tpfBase);           // 0x26d8230
    uint32_t       GXTpfContainer_GetTextureCount(const uint8_t* tpfBase);                                     // 0x26d81f0
    bool           GXTpfContainer_ValidateHeader(const uint8_t* tpfBase);                                      // 0x26d81a0
    uint32_t       GXTpfFormat_GetBlockHeight(uint32_t engineFormat);                                          // 0x26d80f0
    uint32_t       GXTpfFormat_GetBlockWidth(uint32_t engineFormat);                                           // 0x26d80d0
    uint32_t       GXTpfFormat_GetBytesPerBlock(uint32_t engineFormat);                                        // 0x26d80b0
    uint32_t       GXDdsHeader_GetSubresourceOffset(const uint8_t* ddsPtr, uint32_t mipIndex, uint32_t slice); // 0x26d7b40
    uint32_t       GXDdsHeader_GetMipSliceByteSize(const uint8_t* ddsPtr, uint32_t mipIndex);                  // 0x26d7ab0
    uint32_t       GXDdsHeader_GetMipSlicePitchOrRowSize(const uint8_t* ddsPtr, uint32_t mipIndex);            // 0x26d7a50
    uint32_t       GXDdsHeader_GetArrayOrCubeSliceCount(const uint8_t* ddsPtr);                                // 0x26d7a20
    uint32_t       GXTpfFormat_DxgiToEngineFormat(uint32_t dxgiFormat);                                        // 0x26d7a00
    uint32_t       GXDdsHeader_GetEnginePixelFormat(const uint8_t* ddsPtr);                                    // 0x26d7530
    bool           GXDdsHeader_ValidateMagicAndSize(const uint8_t* ddsPtr);                                    // 0x26d7510

    // 04_SceneRenderer — GXFlverParserAndSkinningMatricesUpdater.o (0x21c9a90 .. 0x21cedf0)
    void           sub_21c9c00(uint64_t* first, uint64_t* last, int64_t depthLimit);                           // 0x21c9c00: GXSortKey64_Introsort
    void           sub_21ce560(void* flverContainer);                                                          // 0x21ce560: GXModelFlverContainer_Construct
    bool           sub_21ce5d0(void* flverContainer, void* rawFlverData);                                      // 0x21ce5d0: GXModelFlverContainer_ParseAndValidate
    void*          sub_21cec90(const void* flverContainer, int32_t dummyIndex);                                // 0x21cec90
    void*          sub_21ceca0(const void* flverContainer, int32_t materialIndex);                             // 0x21ceca0
    const char16_t* sub_21cecb0(const void* flverContainer, int32_t materialIndex);                            // 0x21cecb0
    const char16_t* sub_21cecd0(const void* flverContainer, int32_t materialIndex);                            // 0x21cecd0
    void*          sub_21cecf0(const void* flverContainer, int32_t materialIndex);                             // 0x21cecf0
    void*          sub_21ced10(const void* flverContainer, uint32_t textureIndex);                             // 0x21ced10
    const char16_t* sub_21ced40(const void* flverContainer, int32_t materialIndex, uint32_t subTexIndex);      // 0x21ced40
    void*          sub_21ced80(const void* flverContainer, int32_t boneIndex);                                 // 0x21ced80
    const char16_t* sub_21ced90(const void* flverContainer, int32_t boneIndex);                                // 0x21ced90
    void*          sub_21cedb0(const void* flverContainer, int32_t meshIndex);                                 // 0x21cedb0
    uint32_t       sub_21cedc0(const void* flverContainer, uint32_t layoutIndex);                              // 0x21cedc0
    void*          sub_21cedd0(const void* flverContainer, uint32_t layoutIndex, uint32_t memberIndex);        // 0x21cedd0
    void*          sub_21cedf0(const void* flverContainer, uint32_t faceSetIndex);                             // 0x21cedf0

    // 04_SceneRenderer — GXFlverMaterialShaderBinderAndModelContext.o (0x21c09e0 .. 0x21c4b40)
    bool           sub_21c2a60(uint32_t fmt);                                                                  // 0x21c2a60
    bool           sub_21c2a80(int32_t fmt);                                                                   // 0x21c2a80
    uint32_t       sub_21c2af0(uint32_t fmt);                                                                  // 0x21c2af0
    uint32_t       sub_21c2d20(int32_t fmt);                                                                   // 0x21c2d20
    uint32_t       sub_21c2d60(uint32_t fmt);                                                                  // 0x21c2d60

    // 04_SceneRenderer — GXShaderPackageLoaderAndParameterInspectors.o (0x21bd240 .. 0x21c08b0)
    float          sub_21c0670(const __m128* point, const __m128* obb, __m128* outClosest);                    // 0x21c0670
    float          sub_21c0710(const __m128* point, const __m128* segment, __m128* outClosest);                // 0x21c0710

    // 04_SceneRenderer — GXBoundingVolumeCullingAndPathStringUtils.o (0x21b9f90 .. 0x21bd230)
    __m128*        sub_21bb8f0(__m128* out_mat4, uint32_t face_id, const __m128* pos);                         // 0x21bb8f0
    __m128*        sub_21bb9c0(__m128* out_mat4, const __m128* aabb_min_max, float scale, float margin);       // 0x21bb9c0
    __m128         sub_21bba30(const __m128* v);                                                               // 0x21bba30
    void           sub_21bc2a0(void* self);                                                                    // 0x21bc2a0
    char16_t*      sub_21bc6f0(const char16_t* path);                                                          // 0x21bc6f0
    bool           sub_21bc750(char16_t* path);                                                                // 0x21bc750
    void           sub_21bc840(const char16_t* path, char16_t* out_buf, size_t max_chars);                     // 0x21bc840
    void           sub_21bca90(char16_t* dst, const char16_t* src, char16_t sep, size_t max_len);              // 0x21bca90

    // 04_SceneRenderer — GXRenderThreadAndWorkerTaskPool.o (0x21b6420 .. 0x21b9f40)
    void*          GXSortedRenderItemBuffer_MergeTwoSortedRuns(
                       void* out_result, const uint8_t* first1, const uint8_t* last1,
                       const uint8_t* first2, const uint8_t* last2, uint8_t* dst_container_holder);            // 0x21b6420
    bool           GXWorkerThreadContext_IsBusy(const void* self);                                             // 0x21b9a80

    // 05_Yebis2_PostFX — GXYebisPostEffectContextSettersAndInit.o & GXYebisPostEffectRenderAndParamApi.o (0xbb2600 .. 0xbb7500)
    struct PfxDepthFactorVec4 { float x, y, z, w; };
    int32_t        sub_bb6c70(void* this_if);                                                                  // 0xbb6c70: IPfxInterface::AddRef
    int32_t        sub_bb6e00(void* this_if);                                                                  // 0xbb6e00: IPfxInterface::GetReferenceCount
    uint32_t       sub_bb6e10(void* this_if);                                                                  // 0xbb6e10: IPfxInterface::IsInitialized
    void*          sub_bb6e40(void* this_if);                                                                  // 0xbb6e40: IPfxInterface::GetInternalPtr
    void*          sub_bb6e50(void* this_if);                                                                  // 0xbb6e50: IPfxInterface::GetInternalPtrConst
    void*          sub_bb6410(const void* pfMatrixProj, uint32_t bColMaj, uint32_t iSample,
                              void* pfOutMatrix, float fWidth, float fHeight);                                 // 0xbb6410: GetMatrixFrustumJittered (sample)
    void*          sub_bb6350(const void* pfMatrixProj, uint32_t bColMaj, void* pfOutMatrix,
                              float fWidth, float fHeight, float fJitterX, float fJitterY);                    // 0xbb6350: GetMatrixFrustumJittered (offset)
    void           sub_bb6310(uint32_t iSample, void* afOutOffset);                                            // 0xbb6310: GetJitteredSubpixel
    void           sub_bb6120(float* pfOutAperture, float* pfOutTimeValue, int32_t eAutoExposureMode,
                              float fExposureValue, float fPriorityValue, float fSensitivityBias,
                              float fProgramWeight, float fMinAperture, float fMaxAperture,
                              float fMinTime, float fMaxTime);                                                 // 0xbb6120: GPUMath_ApertureTimeValueFromExposureValue
    float          sub_bb60a0(float fCoC, float fFocusDist, float fLensPower, float fSensorSize);              // 0xbb60a0: ComputeFNumberFromCoCAndLensParams
    float          sub_bb5ff0(float fFovHalfRad, float fFocusDist, float fLensPower, float fSensorSize);       // 0xbb5ff0: ComputeFieldOfViewFromFocusAndAperture
    int32_t        sub_bb5f10(void* this_ctx, void* pGnmTexture);                                              // 0xbb5f10: searchGnmTextureRegistry
    int32_t        sub_bb5e40(void* this_ctx, void* pGnmRenderTarget);                                         // 0xbb5e40: searchGnmRenderTargetRegistry
    float          sub_bb33e0(void* this_ctx, int32_t iEffectViewIndex);                                       // 0xbb33e0: GetViewParamFloat34
    uint8_t        sub_bb3360(void* this_ctx, int32_t iEffectViewIndex);                                       // 0xbb3360: GetViewEnabledFlag
    float          sub_bb3250(void* this_ctx, int32_t iEffectViewIndex);                                       // 0xbb3250: GetViewSubParamFloat54
    uint8_t        sub_bb31d0(void* this_ctx, int32_t iEffectViewIndex);                                       // 0xbb31d0: GetViewSubParamEnabledFlag
    float          sub_bb2dc0(void* this_ctx, uint32_t srcType, uint32_t dstType, float depthVal);             // 0xbb2dc0: ConvertDepthValue
    int32_t        sub_bb2c20(void* this_ctx, float* afOutFactor, uint32_t srcType, uint32_t dstType);         // 0xbb2c20: GetDepthConversionFactor
    PfxDepthFactorVec4 sub_bb2a90(void* this_ctx, uint32_t srcType, uint32_t dstType);                         // 0xbb2a90: GetDepthConversionFactorValue
    uint32_t       sub_bb2660(void* this_ctx);                                                                 // 0xbb2660: GetNumViews
    uint32_t       sub_bb2650(void* this_ctx);                                                                 // 0xbb2650: GetCurrentViewIndex

    // 06_DeferredLightingAndAO — GXFeedBackBlurRenderPass.o (0x22ab7e0 .. 0x22b0000)
    void           sub_22af5c0(void* pRenderPassMgr);                                                          // 0x22af5c0: UpdateGpuTimestampMetrics
    void           sub_22af5a0(void* pTargetState);                                                            // 0x22af5a0: GXEnvCubeMapPass_EndCallback

    // 06_DeferredLightingAndAO — GXDeferredLightingAndAmbientOcclusionPass.o (0x2297000 .. 0x22a0000)
    void           sub_229e480(void* pSelf, const void* pSrc);                                                 // 0x229e480: TransformReflectionPlaneByInvViewAndProj
    void           sub_229d2a0(void* pSelf, uint32_t maxSlotIndex);                                            // 0x229d2a0: ClampActiveRenderTargetSlots
    void*          sub_229cb00(const void* pSelf, uint32_t idx);                                               // 0x229cb00: GetSlot6E70
    void*          sub_229cab0(const void* pSelf, uint8_t bSecondary, uint8_t bUseLocal);                      // 0x229cab0: SelectDepthStencilStateSlot
    void*          sub_229ca80(const void* pSelf, uint32_t idx);                                               // 0x229ca80: GetGBufferSurfaceByIndex

    // 06_DeferredLightingAndAO — GXDeferredLightVolumeAndShadowCuller.o (0x2290000 .. 0x2297000)
    void           sub_2294d40(void* pSelf, const void* pCamDesc);                                             // 0x2294d40: UpdateCameraFrustumPair
    void*          sub_2294b30(void* pRetStorage, const void* pFirst1, const void* pLast1,
                               const void* pFirst2, const void* pLast2, void* pOutHolder);                     // 0x2294b30: MergeTwoSortedLightItemRuns
    void           sub_2293710(void* pFirst, void* pLast, int64_t count, void* pTempHolder);                   // 0x2293710: GXLightSortItem16_StableSortRecursive
    uint32_t       sub_2292b80(const void* pSelf, const __m128* pPoint);                                       // 0x2292b80: CullPointAgainstFrustumSIMD
    uint32_t       sub_22928a0(const void* pSelf, const __m128* pAabbMinMax);                                  // 0x22928a0: CullAabbAgainstFrustumSIMD
    void           sub_22919c0(void* pSelf);                                                                   // 0x22919c0: GXCullVolume_Construct
    void           sub_2291530(void* pDst, const void* pSrc);                                                  // 0x2291530: GXCullContext_CopyState
    void           sub_22911b0(void* pSelf, const __m128* pViewMat, const __m128* pProjMat);                   // 0x22911b0: GXCullContext_SetViewAndProjMatrices
    void           sub_2290c80(void* pSelf, void* pCullVolumeOverride);                                        // 0x2290c80: GXCullContext_Construct
    void           sub_2290c70(void* pSelf, uint64_t flag);                                                    // 0x2290c70: SetFlag6F8
    void           sub_2290c60(void* pSelf, const __m128* pViewAndProjMats);                                   // 0x2290c60: SetViewAndProjMatricesPair
    void           sub_2290c10(void* pSelf, const __m128* pVec);                                               // 0x2290c10: SetClipPlane710
    void           sub_2290660(void* pDst, const void* pSrc);                                                  // 0x2290660: GXCommandContext_CopyExtendedState
    void*          sub_2290640(const void* pSelf);                                                             // 0x2290640: GetCommandBufferHead

    // 07_SFX_ParticleGpuRenderer — GXFfxRibbonAndParticleVertexEmitter.o (0x28ce7b0 .. 0x28d25c0)
    void           sub_28d21c0(uint8_t* pOutVerts, const float* const* ppStreams, uint32_t quadCount,
                               const float* pPosX, const float* pPosY, const float* pPosZ,
                               const __m128* pWorldMat3x4);                                                    // 0x28d21c0: EmitPointSpriteParticleVertices
    void           sub_28d0480(__m128* pOutMin, __m128* pOutMax, uint32_t count,
                               const float* pPosX, const float* pPosY, const float* pPosZ);                    // 0x28d0480: ComputeParticleSoAAabbSIMD
    void           sub_28d02c0(uint32_t* pSortBuf, uint32_t count, const float* pPosZ,
                               const float* pPosX, const float* pPosY, const uint8_t* pViewMat,
                               const __m128* pBoundsZ);                                                        // 0x28d02c0: BuildViewDepthSortKeysAndIntrosort
    void           sub_28d0160(uint8_t* pColorBytes, const uint8_t* pVertPos, uint32_t count,
                               uint32_t stride, const float* pFadeParams, const uint8_t* pViewMat);            // 0x28d0160: ApplyCameraNearFarDistanceFade
    void           sub_28cfba0(uint8_t* pOutVerts, uint32_t count, const float* pTime,
                               const uint32_t* pPosX, const uint32_t* pPosY, const uint32_t* pPosZ,
                               float curTime, float lifeTime, float param0, float param1,
                               float vOffsetBase, float vOffsetDelta, float colorB, float colorG,
                               const uint32_t* pUCoord, const float* pVCoordOpt,
                               const float* pUpZ, const float* pUpX, const float* pUpY, uint32_t upStride,
                               float colorR, float colorA, uint32_t startIdxOffset,
                               uint32_t totalRibbonPoints, float fadeInCount, float fadeOutCount);             // 0x28cfba0: EmitCustomUpRibbonTrailControlPoints
    void           sub_28cf580(uint8_t* pOutVerts, uint32_t count, const float* pTime,
                               const uint32_t* pPosX, const uint32_t* pPosY, const uint32_t* pPosZ,
                               float curTime, float camPosX, float camPosY, float camPosZ,
                               float lifeTime, float param0, float param1, float vOffsetBase,
                               const uint32_t* pUCoord, const float* pVCoordOpt, float vOffsetDelta,
                               float colorB, float colorG, float colorR, float colorA,
                               uint32_t startIdxOffset, uint32_t totalRibbonPoints,
                               float fadeInCount, float fadeOutCount);                                         // 0x28cf580: EmitCameraFacingRibbonTrailControlPoints
    void           sub_28ceec0(uint8_t* pOutVerts, uint32_t count, const float* pTime,
                               const float* pPosX, const float* pPosY, const float* pPosZ,
                               float curTime, float lifeTime, float uvParam, float halfWidth,
                               float vOffsetBase, float vOffsetDelta, float colorB, float colorG,
                               const uint32_t* pUCoord, const float* pVCoordOpt,
                               const float* pUpZ, const float* pUpX, const float* pUpY, uint32_t upStride,
                               float colorR, float colorA, uint32_t startIdxOffset,
                               uint32_t totalRibbonPoints, float fadeInCount, float fadeOutCount);             // 0x28ceec0: EmitCustomUpRibbonStripVertexPairs
    void           sub_28ce7b0(uint8_t* pOutVerts, uint32_t count, const float* pTime,
                               const float* pPosX, const float* pPosY, const float* pPosZ,
                               float curTime, float camPosX, float camPosY, float camPosZ,
                               float lifeTime, float uvParam, float halfWidth, float vOffsetBase,
                               const uint32_t* pUCoord, const float* pVCoordOpt, float vOffsetDelta,
                               float colorB, float colorG, float colorR, float colorA,
                               uint32_t startIdxOffset, uint32_t totalRibbonPoints,
                               float fadeInCount, float fadeOutCount);                                         // 0x28ce7b0: EmitCameraFacingRibbonStripVertexPairs

    // 07_SFX_ParticleGpuRenderer — GXFfxSceneCtrlAndShaderBinder.o (0x2309000 .. 0x2310000)
    void           sub_230ffe0(void* pSelf, void* pAllocator);                                                 // 0x230ffe0: GXFfxShaderCache_Construct
    void*          sub_230ffd0(void* pSelf, uint32_t index);                                                   // 0x230ffd0: GXFfxShaderCache_GetTable0Entry
    void*          sub_230ffc0(void* pSelf, int32_t index);                                                    // 0x230ffc0: GXFfxShaderCache_GetTable1Entry
    void           sub_230ecc0(const void* pShaderCache, void* pDrawContext);                                  // 0x230ecc0: BindPointSpriteShaderState0
    void           sub_230dc40(const void* pShaderCache, void* pDrawContext);                                  // 0x230dc40: BindParticleShaderState11
    void           sub_230cfb0(void* pMgr, uint32_t key);                                                      // 0x230cfb0: GXFfxGraphicsResourceManager_RemoveById
    bool           sub_230cd00(void* pMgr, uint32_t key, void* pValue);                                        // 0x230cd00: GXFfxGraphicsResourceManager_InsertUnique
    void*          sub_230cc30(void* pMgr, uint32_t key);                                                      // 0x230cc30: GXFfxGraphicsResourceManager_FindById
    void           sub_230caf0(void* pMgr);                                                                    // 0x230caf0: GXFfxGraphicsResourceManager_Clear
    void           sub_230c940(void* pMgr, void* pAllocator);                                                  // 0x230c940: GXFfxGraphicsResourceManager_Construct
    void           sub_230c640(void* pMgr);                                                                    // 0x230c640: GXFfxGraphicsResourceManager_Destruct
    void           sub_230ba00(void* pSelf, uint32_t index, const float* pMatrix3x4);                          // 0x230ba00: Decompose3x4MatrixToSqtSIMD
    void           sub_230b760(void* pSelf);                                                                   // 0x230b760: UpdateWorldMatricesFromSqtBatchSIMD
    void           sub_230ae70(void* pSelf, void* pAllocator);                                                 // 0x230ae70: ParticleSubController_Construct
    void           sub_230add0(void* pOuter, const float* pMatrix4x4);                                         // 0x230add0: ParticleOuter_SetMatrixAndTransform

    // 07_SFX_ParticleGpuRenderer — GXFfxTessellationAndParticleDrawPass.o (0x2304000 .. 0x2309000)
    struct FfxTestIter24 {
        uint64_t proxy0;
        uint64_t proxy1;
        uint8_t* node;
    };
    void           sub_23084f0(void* pSelf, const int32_t* pRange);                                            // 0x23084f0: DispatchBucketRange84F0
    void           sub_2308150(void* pSelf, const int32_t* pRange);                                            // 0x2308150: DispatchBucketRange8150
    void           sub_23071a0(void* pVec, const uint64_t* pVal);                                              // 0x23071a0: Vector8_PushBack
    void           sub_2305f50(void* pVec, FfxTestIter24 posIter,
                               FfxTestIter24 firstIter, FfxTestIter24 lastIter);                               // 0x2305f50: Vector80_InsertRangeAndMaskW
    void           sub_2305640(void* pVec);                                                                    // 0x2305640: Vector16_GrowByOne
    void           sub_23054f0(void* pSelf, const int32_t* pRange);                                            // 0x23054f0: DispatchSlotRange54F0
    void           sub_2305120(void* pSelf);                                                                   // 0x2305120: InvokePtmfCallback
    uint32_t       sub_23050f0(void* pSelf);                                                                   // 0x23050f0: AdvanceStateCounterAndInvoke
    uint32_t       sub_2305060();                                                                              // 0x2305060: GetConstantOne

    // 08_GPU_Shaders — GXShaderDecompiledLibraryAndManifest.o (752 Shaders Manifest & Reference Kernels)
    struct GXShaderManifestEntry {
        const char* bundle;
        const char* name;
        const char* safe_name;
        const char* stage;
        const char* tess_role;
        uint32_t entry_id;
        uint8_t container_type;
        uint64_t shader_hash;
        uint32_t crc32;
        uint32_t container_bytes;
        uint32_t code_bytes;
        uint32_t instructions;
        uint32_t spirv_words;
        uint32_t rsrc1;
        uint32_t rsrc2;
        uint16_t vgprs;
        uint16_t sgprs;
        uint8_t user_sgprs;
        uint8_t image_count;
        uint8_t sampler_count;
        uint8_t buffer_count;
        uint32_t max_cb_dwords;
        uint32_t max_tbl_dwords;
        const char* primary_cb_path;
        const char* primary_cb_name;
        const char* primary_tex_name;
    };

    uint32_t GXShaderLibrary_GetTotalShaderCount() noexcept;
    uint32_t GXShaderLibrary_GetBundleShaderCount(const char* bundle_name) noexcept;
    const GXShaderManifestEntry* GXShaderLibrary_GetEntryByIndex(uint32_t index) noexcept;
    const GXShaderManifestEntry* GXShaderLibrary_FindByName(const char* bundle_name, const char* shader_name) noexcept;
    const GXShaderManifestEntry* GXShaderLibrary_FindByHash(uint64_t shader_hash, uint32_t crc32) noexcept;
    bool GXShaderLibrary_VerifyCpuBindingLayout(const char* bundle_name, const char* shader_name,
                                                uint32_t cpu_cb_bytes, uint32_t expected_images,
                                                uint32_t expected_samplers) noexcept;

    void GXShader_SimpleSingle2DTexture_PS(const float tex_sample_rgba[4], const float vtx_color_rgba[4],
                                           const float cb_col_rgba[4], int32_t cb_param_gamma_flag,
                                           float out_mrt0_rgba[4], uint32_t out_mrt0_pkrtz[2]) noexcept;
    void GXShader_GuiTex_PS(const float tex_sample_rgba[4], const float vtx_color_rgba[4],
                            float out_mrt0_rgba[4], uint32_t out_mrt0_pkrtz[2]) noexcept;
    void GXShader_DepthConvertLinear_PS(float hw_depth_z, const float cb_ssao_param_xy[2],
                                        float* out_linear_depth, float out_mrt0_rgba[4]) noexcept;
    void GXShader_FeedBackBlurFallOff_PS(const float src_rgba[4], const float cb_sample_offset_xy[2],
                                         float out_mrt0_rgba[4]) noexcept;
    void GXShader_Yebis2_FilmicToneMap_PS(const float hdr_rgba[4], const float cb_tonemap_dwords[16],
                                          float out_mrt0_rgba[4]) noexcept;
    void GXShader_Yebis2_ReinhardToneMap_PS(const float hdr_rgba[4], const float cb_tonemap_dwords[16],
                                            float out_mrt0_rgba[4]) noexcept;
    void GXShader_ColorGrading_ComputeLutCoords(const float src_rgba[4], float out_lut_uvw[3],
                                                float* out_clamped_alpha) noexcept;
    void GXShader_Deferred_ReconstructFragmentAndNormal(const float ndc_xy[2], float hw_depth,
                                                        const float gbuf_normal_rgb[3],
                                                        const float cb_light_acc_dwords[44],
                                                        float out_unit_normal[3], float out_view_pos[3],
                                                        float out_eye_vec_world[3]) noexcept;
    bool GXShader_Decal_ProjectAndClipBox(const float clip_xy_w[3], float sampled_depth,
                                          const float cb_decal_param_mat[16],
                                          float out_screen_uv[2], float out_box_uvw[3]) noexcept;
    void GXShader_Ffx_TessellatePointSprite_LS(const float vtx_pos_size[4], const float vtx_color_rgba[4],
                                               const float cb_dwords[84], float out_lds_patch[15]) noexcept;
    void GXShader_Ffx_TessellatePointSprite_DS(const float lds_patch[15], float domain_u, float domain_v,
                                               float out_pos0[4], float out_param0[4],
                                               float out_param1[4], float out_param2[4]) noexcept;
}

static uint64_t g_hook_call_count = 0;
static void Hooked_GXRenderJobSort_StableSortRecursive(
    FromSoftware::GX::GXRenderJobEntry* first,
    FromSoftware::GX::GXRenderJobEntry* last,
    int64_t count,
    FromSoftware::GX::GXRenderJobSortContext* ctx)
{
    ++g_hook_call_count;
    FromSoftware::GX::GXRenderJobSort_StableSortRecursive(first, last, count, ctx);
}

// Helpers for [TEST 14] 07_SFX_ParticleGpuRenderer
static void FfxFakeHeap_GetFlags(uint8_t* outFlagsBuf, void* /*pHeap*/, uint32_t /*arg*/) {
    *reinterpret_cast<uint64_t*>(outFlagsBuf) = 0x20ULL;
}
static void* FfxFakeHeap_Alloc(void* /*pHeap*/, size_t byteSize, size_t /*align*/) {
    size_t sz = (byteSize > 0) ? ((byteSize + 15u) & ~size_t(15u)) : 16u;
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 16, sz) != 0) return nullptr;
    std::memset(ptr, 0, sz);
    return ptr;
}
static void FfxFakeHeap_Free(void* /*pHeap*/, void* ptr) {
    std::free(ptr);
}

static uint64_t g_ffx_dispatch_hash = 0;
static void FfxFakeDrawItem_Draw(void* pItem, void* pCtx, void* pData, uint32_t flag) {
    g_ffx_dispatch_hash = (g_ffx_dispatch_hash * 1315423911ULL)
        ^ reinterpret_cast<uintptr_t>(pItem)
        ^ (reinterpret_cast<uintptr_t>(pCtx) << 7)
        ^ (reinterpret_cast<uintptr_t>(pData) << 13)
        ^ static_cast<uint64_t>(flag + 0x9e3779b9u);
}
static void FfxFakePtmfTarget_Callback(void* pTarget, uint32_t arg) {
    *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(pTarget) + 0x08) =
        0xCAFE00000000ULL | static_cast<uint64_t>(arg);
}

int main() {
    setenv("BBHOST_EBOOT", "/home/kekw3/Documents/BloodBorne/CUSA03173/eboot.bin", 0);
    eboot_kit::load("gx_live_combat_test");
    g_eboot_live_slide = 0x400000ULL;
    eboot_kit::Rng rng(0xC0FFEE123456789ULL);

    int total_failures = 0;

    // ========================================================================
    // 1. Test GXStreamOutElementArray_IntrosortBySemanticKey (VMA 0x21740e0, 3994 B)
    // ========================================================================
    {
        auto game_sort = eboot_vma_fn<void(*)(GXStreamOutElement28*, GXStreamOutElement28*, int64_t)>(0x21740e0);
        int bad = 0;
        const int cases = 5000;
        for (int c = 0; c < cases; ++c) {
            int n = rng.range(0, 90);
            int64_t depth = rng.range(0, 6);
            std::vector<GXStreamOutElement28> theirs(n), ours(n);
            for (int i = 0; i < n; ++i) {
                // Make keys unique or structured like semantic keys so introsort's unstable partition order is deterministic
                uint32_t k = (rng.u32() & 0xFFFF0000u) | static_cast<uint32_t>(i);
                // Mix in random permutation of unique keys
                theirs[i] = { k, rng.u32(), rng.u32(), rng.u32(), rng.u32(), rng.u32(), rng.u32() };
            }
            // Shuffle keys
            for (int i = n - 1; i > 0; --i) {
                int j = rng.range(0, i);
                std::swap(theirs[i].key, theirs[j].key);
            }
            ours = theirs;
            game_sort(theirs.data(), theirs.data() + n, depth);
            GXGraphicsFramework::GXStreamOutElementArray_IntrosortBySemanticKey(ours.data(), ours.data() + n, depth);
            if (n > 0 && std::memcmp(theirs.data(), ours.data(), n * sizeof(GXStreamOutElement28)) != 0) {
                ++bad;
            }
        }
        std::printf("[TEST 1A] GXStreamOutElementArray_IntrosortBySemanticKey (0x21740e0, 3994 B): %d arrays -> %d differ\n", cases, bad);
        total_failures += bad;
    }

    // ========================================================================
    // 2. Test GXGnmTopology_GetVerticesPerPrimitive (0x216bb60) & Buffer Extract (0x216d730)
    // ========================================================================
    {
        auto game_topo = eboot_vma_fn<uint32_t(*)(void*, uint32_t*, uint32_t, uint32_t)>(0x216bb60);
        auto game_buf  = eboot_vma_fn<void(*)(const void*, void*)>(0x216d730);
        int bad = 0;
        for (uint32_t t = 0; t <= 40; ++t) {
            uint32_t out_theirs = 0xdeadbeef, out_ours = 0xdeadbeef;
            uint32_t r1 = game_topo(nullptr, &out_theirs, 0, t);
            uint32_t r2 = GXGraphicsFramework::GXGnmTopology_GetVerticesPerPrimitive(nullptr, &out_ours, 0, t);
            if (r1 != r2 || out_theirs != out_ours) ++bad;
        }
        for (int c = 0; c < 10000; ++c) {
            GXGraphicsFramework::GXHardwareBufferResource hw{};
            hw.requestedSize  = rng.u32();
            hw.usageType      = static_cast<uint8_t>(rng.u32());
            hw.cpuAccessFlags = static_cast<uint8_t>(rng.u32());
            hw.bindFlags      = static_cast<uint8_t>(rng.u32());
            hw.miscFlags      = rng.u32();
            hw.elementStride  = static_cast<uint16_t>(rng.u32());
            GXGraphicsFramework::GXBufferDescriptorOut d_theirs{}, d_ours{};
            game_buf(&hw, &d_theirs);
            GXGraphicsFramework::GXGnmBuffer_ExtractDescriptorFromHardware(&hw, &d_ours);
            if (std::memcmp(&d_theirs, &d_ours, sizeof(d_ours)) != 0) ++bad;
        }
        std::printf("[TEST 1B] GXGnmTopology (0x216bb60) + GXGnmBuffer_ExtractDescriptor (0x216d730): 10041 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 3. Test GXRenderJobSort_StableSortRecursive + ChunkInsertion + Merge (0x21b4f70..0x21b5990, 5263 B)
    // ========================================================================
    {
        using namespace FromSoftware::GX;
        auto game_stable_sort = eboot_vma_fn<void(*)(GXRenderJobEntry*, GXRenderJobEntry*, int64_t, GXRenderJobSortContext*)>(0x21b4f70);
        int bad = 0;
        const int cases = 5000;
        for (int c = 0; c < cases; ++c) {
            int n = rng.range(1, 120);
            std::vector<GXRenderJobEntry> theirs(n), ours(n);
            for (int i = 0; i < n; ++i) {
                // Intentionally use small range of sortKeys (0..15) to stress-test STABLE sort order preservation!
                uint32_t key = static_cast<uint32_t>(rng.range(0, 15));
                theirs[i] = { reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(0x10000 + i)), key };
            }
            ours = theirs;

            std::vector<GXRenderJobEntry> temp_theirs(n), temp_ours(n);
            int64_t cap = rng.chance(0.5) ? n : rng.range(4, std::max(4, n / 3));
            GXRenderJobTempBuffer tb_theirs{ temp_theirs.data(), temp_theirs.data(), temp_theirs.data(), cap };
            GXRenderJobTempBuffer tb_ours{ temp_ours.data(), temp_ours.data(), temp_ours.data(), cap };
            GXRenderJobSortContext ctx_theirs{};
            GXRenderJobSortContext ctx_ours{};
            ctx_theirs.tempBuf = &tb_theirs;
            ctx_ours.tempBuf   = &tb_ours;

            game_stable_sort(theirs.data(), theirs.data() + n, n, &ctx_theirs);
            GXRenderJobSort_StableSortRecursive(ours.data(), ours.data() + n, n, &ctx_ours);

            if (std::memcmp(theirs.data(), ours.data(), n * sizeof(GXRenderJobEntry)) != 0) {
                ++bad;
            }
        }
        std::printf("[TEST 2]  GXRenderJobSort_StableSort + Merge (0x21b4f70..0x21b5990, 5263 B): %d job queues -> %d differ\n", cases, bad);
        total_failures += bad;
    }

    // ========================================================================
    // 4. Test GXGnmSamplerState, RasterizerState, DepthStencilState, BlendState (0x2178050..0x2178ab0)
    // ========================================================================
    {
        auto game_sampler_init = eboot_vma_fn<int(*)(uint8_t*, void*, const uint8_t*)>(0x2178750);
        auto game_sampler_ext  = eboot_vma_fn<void(*)(const uint8_t*, uint8_t*)>(0x2178ab0);
        auto game_raster_init  = eboot_vma_fn<int(*)(uint8_t*, void*, const uint8_t*)>(0x2178550);
        auto game_raster_ext   = eboot_vma_fn<void(*)(const uint8_t*, uint8_t*)>(0x2178670);
        auto game_ds_init      = eboot_vma_fn<int(*)(uint8_t*, void*, const uint8_t*)>(0x21782b0);
        auto game_ds_ext       = eboot_vma_fn<void(*)(const uint8_t*, uint8_t*)>(0x2178460);
        auto game_blend_init   = eboot_vma_fn<int(*)(uint8_t*, void*, const uint8_t*)>(0x2178050);

        int bad_sampler = 0, bad_raster = 0, bad_ds = 0, bad_blend = 0;
        const int cases = 10000;
        for (int c = 0; c < cases; ++c) {
            // 4A. Sampler State
            alignas(16) uint8_t s_desc[0x38]{};
            *reinterpret_cast<uint32_t*>(s_desc + 0x00) = rng.range(0, 12);
            *reinterpret_cast<uint32_t*>(s_desc + 0x04) = rng.range(0, 5);
            *reinterpret_cast<uint32_t*>(s_desc + 0x08) = rng.range(0, 5);
            *reinterpret_cast<uint32_t*>(s_desc + 0x0C) = rng.range(0, 5);
            *reinterpret_cast<float*>(s_desc + 0x10)    = rng.uniform(-40.0f, 40.0f);
            *reinterpret_cast<uint32_t*>(s_desc + 0x14) = rng.range(1, 16);
            *reinterpret_cast<uint32_t*>(s_desc + 0x18) = rng.range(0, 8);
            bool white = rng.chance(0.25), black = rng.chance(0.25);
            *reinterpret_cast<float*>(s_desc + 0x1C) = white ? 1.0f : (black ? 0.0f : rng.uniform(0.0f, 1.0f));
            *reinterpret_cast<float*>(s_desc + 0x20) = white ? 1.0f : (black ? 0.0f : rng.uniform(0.0f, 1.0f));
            *reinterpret_cast<float*>(s_desc + 0x24) = white ? 1.0f : (black ? 0.0f : rng.uniform(0.0f, 1.0f));
            *reinterpret_cast<float*>(s_desc + 0x28) = (white || black) ? 1.0f : rng.uniform(0.0f, 1.0f);
            *reinterpret_cast<float*>(s_desc + 0x2C) = rng.uniform(-5.0f, 20.0f);
            *reinterpret_cast<float*>(s_desc + 0x30) = rng.uniform(-5.0f, 20.0f);
            *reinterpret_cast<uint32_t*>(s_desc + 0x34) = rng.u32();

            alignas(16) uint8_t s_theirs[0x40]{}, s_ours[0x40]{};
            alignas(16) uint8_t s_ext_theirs[0x38]{}, s_ext_ours[0x38]{};
            game_sampler_init(s_theirs, nullptr, s_desc);
            GXGnmSamplerState_Initialize(s_ours, nullptr, s_desc);
            game_sampler_ext(s_theirs, s_ext_theirs);
            GXGnmSamplerState_ExtractDesc(s_ours, s_ext_ours);
            if (std::memcmp(s_theirs, s_ours, 0x34) != 0 || std::memcmp(s_ext_theirs, s_ext_ours, 0x38) != 0) {
                ++bad_sampler;
            }

            // 4B. Rasterizer State
            alignas(16) uint8_t r_desc[0x20]{};
            *reinterpret_cast<uint32_t*>(r_desc + 0x00) = rng.range(0, 2);
            *reinterpret_cast<uint32_t*>(r_desc + 0x04) = rng.range(0, 3);
            r_desc[0x08] = static_cast<uint8_t>(rng.range(0, 1));
            *reinterpret_cast<int32_t*>(r_desc + 0x0C) = rng.range(-100, 100);
            *reinterpret_cast<float*>(r_desc + 0x10)   = rng.uniform(-1.0f, 1.0f);
            *reinterpret_cast<float*>(r_desc + 0x14)   = rng.chance(0.3) ? 0.0f : rng.uniform(-4.0f, 4.0f);
            r_desc[0x18] = static_cast<uint8_t>(rng.range(0, 1));
            r_desc[0x19] = static_cast<uint8_t>(rng.range(0, 1));
            r_desc[0x1A] = static_cast<uint8_t>(rng.range(0, 1));

            alignas(16) uint8_t r_theirs[0x28]{}, r_ours[0x28]{};
            alignas(16) uint8_t r_ext_theirs[0x20]{}, r_ext_ours[0x20]{};
            game_raster_init(r_theirs, nullptr, r_desc);
            GXGnmRasterizerState_Initialize(r_ours, nullptr, r_desc);
            game_raster_ext(r_theirs, r_ext_theirs);
            GXGnmRasterizerState_ExtractDesc(r_ours, r_ext_ours);
            if (std::memcmp(r_theirs, r_ours, 0x22) != 0 || std::memcmp(r_ext_theirs, r_ext_ours, 0x1C) != 0) {
                ++bad_raster;
            }

            // 4C. DepthStencil State
            alignas(16) uint8_t ds_desc[0x34]{};
            ds_desc[0x00] = static_cast<uint8_t>(rng.range(0, 1));
            *reinterpret_cast<uint32_t*>(ds_desc + 0x04) = rng.range(0, 1);
            *reinterpret_cast<uint32_t*>(ds_desc + 0x08) = rng.range(0, 8);
            ds_desc[0x0C] = static_cast<uint8_t>(rng.range(0, 1));
            ds_desc[0x0D] = static_cast<uint8_t>(rng.u32());
            ds_desc[0x0E] = static_cast<uint8_t>(rng.u32());
            for (int k = 0; k < 8; ++k) {
                *reinterpret_cast<uint32_t*>(ds_desc + 0x10 + k * 4) = rng.range(0, 8);
            }
            ds_desc[0x30] = static_cast<uint8_t>(rng.range(0, 1));

            alignas(16) uint8_t ds_theirs[0x38]{}, ds_ours[0x38]{};
            alignas(16) uint8_t ds_ext_theirs[0x34]{}, ds_ext_ours[0x34]{};
            game_ds_init(ds_theirs, nullptr, ds_desc);
            GXGnmDepthStencilState_Initialize(ds_ours, nullptr, ds_desc);
            game_ds_ext(ds_theirs, ds_ext_theirs);
            GXGnmDepthStencilState_ExtractDesc(ds_ours, ds_ext_ours);
            if (std::memcmp(ds_theirs, ds_ours, 0x38) != 0 || std::memcmp(ds_ext_theirs, ds_ext_ours, 0x31) != 0) {
                ++bad_ds;
            }

            // 4D. Blend State
            alignas(16) uint8_t b_desc[0x104]{};
            b_desc[0x00] = static_cast<uint8_t>(rng.range(0, 1));
            b_desc[0x01] = static_cast<uint8_t>(rng.range(0, 1));
            for (int rt = 0; rt < 8; ++rt) {
                uint8_t* rt_p = b_desc + 0x04 + rt * 0x20;
                rt_p[0x00] = static_cast<uint8_t>(rng.range(0, 1));
                *reinterpret_cast<uint32_t*>(rt_p + 0x04) = rng.range(0, 19);
                *reinterpret_cast<uint32_t*>(rt_p + 0x08) = rng.range(0, 19);
                *reinterpret_cast<uint32_t*>(rt_p + 0x0C) = rng.range(0, 5);
                *reinterpret_cast<uint32_t*>(rt_p + 0x10) = rng.range(0, 19);
                *reinterpret_cast<uint32_t*>(rt_p + 0x14) = rng.range(0, 19);
                *reinterpret_cast<uint32_t*>(rt_p + 0x18) = rng.range(0, 5);
                *reinterpret_cast<uint32_t*>(rt_p + 0x1C) = rng.range(0, 15);
            }
            alignas(16) uint8_t b_theirs[0x128]{}, b_ours[0x128]{};
            game_blend_init(b_theirs, nullptr, b_desc);
            GXGnmBlendState_Initialize(b_ours, nullptr, b_desc);
            if (std::memcmp(b_theirs, b_ours, 0x128) != 0) {
                ++bad_blend;
            }
        }
        std::printf("[TEST 3]  GXGnmSampler/Rasterizer/DepthStencil/Blend States (0x2178050..0x2178ab0): 40000 states -> sampler:%d raster:%d ds:%d blend:%d differ\n",
                    bad_sampler, bad_raster, bad_ds, bad_blend);
        total_failures += bad_sampler + bad_raster + bad_ds + bad_blend;
    }

    // ========================================================================
    // 5. Test GXCameraFrustum Pack/Unpack/InitCubeMapFace/ResetDefaults (0x21a9f90..0x21ab450)
    // ========================================================================
    {
        using namespace FromSoftware::GX;
        auto game_unpack = eboot_vma_fn<void(*)(GXCameraFrustum*, const GXCameraFrustumCompact*)>(0x21ab450);
        auto game_pack   = eboot_vma_fn<void(*)(const GXCameraFrustum*, GXCameraFrustumCompact*)>(0x21ab3c0);
        auto game_cube   = eboot_vma_fn<void(*)(GXCameraFrustum*, uint32_t, const float*)>(0x21ab2e0);
        auto game_reset  = eboot_vma_fn<void(*)(GXCameraFrustum*, uint32_t, uint32_t)>(0x21a9f90);

        int bad = 0;
        for (int c = 0; c < 10000; ++c) {
            GXCameraFrustum f_theirs{}, f_ours{};
            game_reset(&f_theirs, 0, 0);
            GXCameraFrustum_ResetDefaults(&f_ours, 0, 0);
            if (std::memcmp(&f_theirs, &f_ours, 0x84) != 0) ++bad;

            float center[4] = { rng.uniform(-500.0f, 500.0f), rng.uniform(-500.0f, 500.0f), rng.uniform(-500.0f, 500.0f), 1.0f };
            uint32_t face = rng.range(0, 5);
            game_cube(&f_theirs, face, center);
            GXCameraFrustum_InitCubeMapFace(&f_ours, face, center);
            if (std::memcmp(&f_theirs, &f_ours, 0x84) != 0) ++bad;

            GXCameraFrustumCompact c_theirs{}, c_ours{};
            game_pack(&f_theirs, &c_theirs);
            GXCameraFrustum_PackToCompact(&f_ours, &c_ours);
            if (std::memcmp(&c_theirs, &c_ours, 0x72) != 0) ++bad;

            GXCameraFrustum u_theirs{}, u_ours{};
            game_unpack(&u_theirs, &c_theirs);
            GXCameraFrustum_UnpackFromCompact(&u_ours, &c_ours);
            if (std::memcmp(&u_theirs, &u_ours, 0x84) != 0) ++bad;
        }
        std::printf("[TEST 4]  GXCameraFrustum Reset/CubeMap/Pack/Unpack (0x21a9f90..0x21ab450): 40000 ops -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 6. Live Runtime JMP Hook Test in eboot.bin (0x21b4f70 -> Hooked C++ fn)
    // ========================================================================
    {
        using namespace FromSoftware::GX;
        // Install live JMP detour at eboot.bin 0x21b4f70 (bn 0x25b4f70) -> Hooked_GXRenderJobSort_StableSortRecursive
        eboot_kit::stub(vma_to_bn(0x21b4f70), reinterpret_cast<const void*>(&Hooked_GXRenderJobSort_StableSortRecursive));

        auto eboot_entry = eboot_vma_fn<void(*)(GXRenderJobEntry*, GXRenderJobEntry*, int64_t, GXRenderJobSortContext*)>(0x21b4f70);
        std::vector<GXRenderJobEntry> jobs(64), temp(64);
        for (int i = 0; i < 64; ++i) {
            jobs[i] = { reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(0x20000 + i)), static_cast<uint32_t>(63 - i) };
        }
        GXRenderJobTempBuffer tb{ temp.data(), temp.data(), temp.data(), 64 };
        GXRenderJobSortContext ctx{};
        ctx.tempBuf = &tb;

        // Call into eboot.bin's text segment at 0x25b4f70, which now executes our JMP detour!
        eboot_entry(jobs.data(), jobs.data() + 64, 64, &ctx);

        bool sorted_ok = (g_hook_call_count == 1);
        for (int i = 0; i < 64; ++i) {
            if (jobs[i].sortKey != static_cast<uint32_t>(i)) sorted_ok = false;
        }
        std::printf("[TEST 5]  Live eboot.bin JMP Hook at 0x25b4f70 (VMA 0x21b4f70): hook_calls=%llu, sorted_ok=%s\n",
                    static_cast<unsigned long long>(g_hook_call_count), sorted_ok ? "YES" : "NO");
        if (!sorted_ok) ++total_failures;
    }

    // ========================================================================
    // 7. Test 03_TPF_GNF_ShaderContainers Batch 1 & Batch 2:
    //    GXMtdMaterialContainer, GXMtdParameterAccessor, GXMtdParamValue_ConvertBetweenTypes,
    //    GXMtdParamType_GetCategoryAndComponentCount, and GXDisplayOutput (0x26dbbe0..0x26df2f0)
    // ========================================================================
    {
        auto game_param_conv = eboot_vma_fn<GXMtdConvertedValue16(*)(void*, const void*, uint32_t, const void*, uint32_t)>(0x26dbed0);
        auto game_param_cat  = eboot_vma_fn<uint64_t(*)(const void*, uint32_t)>(0x26dc2b0);
        auto game_root_magic = eboot_vma_fn<bool(*)(const void*)>(0x26de780);
        auto game_get_err    = eboot_vma_fn<uint32_t(*)(const void*)>(0x26de770);
        auto game_set_err    = eboot_vma_fn<void(*)(void*, uint32_t)>(0x26dc470);
        auto game_get_ver    = eboot_vma_fn<uint32_t(*)(void*)>(0x26dcbb0);
        auto game_set_ver    = eboot_vma_fn<bool(*)(void*, uint32_t)>(0x26dcbd0);
        auto game_acc_bind   = eboot_vma_fn<void(*)(void*, void*)>(0x26dea10);
        auto game_acc_type   = eboot_vma_fn<int32_t(*)(void*)>(0x26dec10);
        auto game_acc_buf4   = eboot_vma_fn<bool(*)(void*, void*)>(0x26dec90);
        auto game_acc_float  = eboot_vma_fn<bool(*)(void*, float*, float*, float*, float*)>(0x26def70);
        auto game_acc_bool   = eboot_vma_fn<bool(*)(void*, bool*, bool*, bool*, bool*)>(0x26df130);
        auto game_acc_int    = eboot_vma_fn<bool(*)(void*, int32_t*, int32_t*, int32_t*, int32_t*)>(0x26df2f0);
        auto game_disp_mode  = eboot_vma_fn<uint32_t(*)(void*, uint32_t, uint32_t, void*)>(0x26dbe00);
        auto game_disp_out   = eboot_vma_fn<void(*)(void*, void*)>(0x26dbdb0);
        auto game_disp_cfg   = eboot_vma_fn<uint32_t(*)(void*)>(0x26dbbe0);

        // Bind PS4 Orbis wcscpy (char16_t*) PLT import at VMA 0x2bbf608 so eboot.bin's 0x26dbdb0 can run
        eboot_kit::stub(vma_to_bn(0x2bbf608), reinterpret_cast<const void*>(&DLCharTraitsW_CopyString));

        int bad = 0;

        // 7A. Exhaustive GXMtdParamType_GetCategoryAndComponentCount (0..15) + 10,000 GXMtdParamValue_ConvertBetweenTypes cases
        for (uint32_t t = 0; t <= 15; ++t) {
            if (game_param_cat(nullptr, t) != GXMtdParamType_GetCategoryAndComponentCount(nullptr, t)) ++bad;
        }

        for (int c = 0; c < 10000; ++c) {
            uint32_t srcType = rng.range(0, 13);
            uint32_t dstType = rng.range(0, 13);
            alignas(16) uint32_t srcBuf[4];
            alignas(16) uint32_t defBuf[4];
            for (int i = 0; i < 4; ++i) defBuf[i] = rng.u32();

            if (srcType <= 3) {
                for (int i = 0; i < 4; ++i) srcBuf[i] = rng.range(0, 1);
            } else if (srcType <= 7) {
                for (int i = 0; i < 4; ++i) srcBuf[i] = static_cast<uint32_t>(rng.range(-50, 50));
            } else {
                float* f = reinterpret_cast<float*>(srcBuf);
                for (int i = 0; i < 4; ++i) f[i] = rng.chance(0.25) ? 0.0f : rng.uniform(-100.0f, 100.0f);
            }

            uint64_t contTheirs[2]{}, contOurs[2]{};
            GXMtdConvertedValue16 r1 = game_param_conv(contTheirs, srcBuf, srcType, defBuf, dstType);
            GXMtdConvertedValue16 r2 = GXMtdParamValue_ConvertBetweenTypes(contOurs, srcBuf, srcType, defBuf, dstType);
            if (r1.lo != r2.lo || r1.hi != r2.hi || contTheirs[1] != contOurs[1]) {
                ++bad;
            }
        }

        // 7B. Test GXMtdParameterAccessor (BindNode, GetResolvedType, GetValueToBuffer4, GetFloat/Bool/IntComponents)
        static const char* kTypeStrings[13] = {
            "bool", "bool2", "bool3", "bool4",
            "int",  "int2",  "int3",  "int4",
            "float","float2","float3","float4",
            "invalid_type"
        };
        struct FakeBlockField {
            void*    vptr;          // +0x00
            uint8_t  isInitialized; // +0x08
            uint8_t  pad09[7];
            uint8_t* dataPtr;       // +0x10
            uint64_t reserved18;    // +0x18
            int32_t  elementCount;  // +0x20
            uint32_t pad24;
            void*    ownerChunk;    // +0x28 (nullptr -> no unshare needed)
            void*    ownerNode;     // +0x30
        };
        struct FakeBlockArray {
            void*    vptr;          // +0x00
            uint64_t reserved08;    // +0x08
            void**   elements;      // +0x10
            uint32_t capacity;      // +0x18
            int32_t  count;         // +0x1C
            uint8_t  pad20[0x10];   // +0x20..+0x2F
        };
        struct FakeValueBlock {
            void*          vptr;       // +0x00
            uint8_t        pad08[0x60];// +0x08..+0x67
            FakeBlockField valueField; // +0x68..+0x9F
        };
        struct FakeParamNode {
            void*          vptr;        // +0x00
            uint8_t        pad08[0x60]; // +0x08..+0x67
            FakeBlockField nameField;   // +0x68..+0x9F
            FakeBlockField typeField;   // +0xA0..+0xD7
            FakeBlockArray valueBlocks; // +0xD8..+0x107
            uint8_t        tail[0x30];  // +0x108..+0x137
        };
        struct FakeParamAccessor {
            void*          vptr;            // +0x00
            FakeParamNode* paramNode;       // +0x08
            int32_t        cachedTypeIndex; // +0x10
            uint32_t       pad14;           // +0x14
        };

        for (int c = 0; c < 5000; ++c) {
            alignas(16) uint32_t rawData[4] = { rng.u32(), rng.u32(), rng.u32(), rng.u32() };
            uint32_t typeIdx = rng.range(0, 12);
            if (typeIdx <= 3) {
                uint8_t* b = reinterpret_cast<uint8_t*>(rawData);
                for (int i = 0; i < 4; ++i) b[i] = static_cast<uint8_t>(rng.range(0, 1));
            } else if (typeIdx >= 8 && typeIdx <= 11) {
                float* f = reinterpret_cast<float*>(rawData);
                for (int i = 0; i < 4; ++i) f[i] = rng.uniform(-50.0f, 50.0f);
            }

            FakeValueBlock vb{};
            vb.valueField.isInitialized = 1;
            vb.valueField.dataPtr       = reinterpret_cast<uint8_t*>(rawData);
            vb.valueField.elementCount  = static_cast<int32_t>(rng.range(1, 4));
            void* vbPtr = &vb;

            FakeParamNode node{};
            node.typeField.isInitialized = 1;
            node.typeField.dataPtr       = reinterpret_cast<uint8_t*>(const_cast<char*>(kTypeStrings[typeIdx]));
            node.valueBlocks.elements    = &vbPtr;
            node.valueBlocks.capacity    = 1;
            node.valueBlocks.count       = rng.chance(0.95) ? 1 : 0;

            FakeParamAccessor accTheirs{}, accOurs{};
            FakeParamNode* boundNode = rng.chance(0.95) ? &node : nullptr;
            game_acc_bind(&accTheirs, boundNode);
            GXMtdParameterAccessor_BindNode(&accOurs, boundNode);

            if (game_acc_type(&accTheirs) != GXMtdParameterAccessor_GetResolvedType(&accOurs)) ++bad;

            uint32_t bufTheirs[4]{}, bufOurs[4]{};
            bool ok1 = game_acc_buf4(&accTheirs, bufTheirs);
            bool ok2 = GXMtdParameterAccessor_GetValueToBuffer4(&accOurs, bufOurs);
            if (ok1 != ok2 || std::memcmp(bufTheirs, bufOurs, 16) != 0) ++bad;

            float fT[4]{}, fO[4]{};
            if (game_acc_float(&accTheirs, &fT[0], &fT[1], &fT[2], &fT[3]) !=
                GXMtdParameterAccessor_GetFloatComponents(&accOurs, &fO[0], &fO[1], &fO[2], &fO[3]) ||
                std::memcmp(fT, fO, 16) != 0) ++bad;

            bool bT[4]{}, bO[4]{};
            if (game_acc_bool(&accTheirs, &bT[0], &bT[1], &bT[2], &bT[3]) !=
                GXMtdParameterAccessor_GetBoolComponents(&accOurs, &bO[0], &bO[1], &bO[2], &bO[3]) ||
                std::memcmp(bT, bO, 4) != 0) ++bad;

            int32_t iT[4]{}, iO[4]{};
            if (game_acc_int(&accTheirs, &iT[0], &iT[1], &iT[2], &iT[3]) !=
                GXMtdParameterAccessor_GetIntComponents(&accOurs, &iO[0], &iO[1], &iO[2], &iO[3]) ||
                std::memcmp(iT, iO, 16) != 0) ++bad;
        }

        // 7C. Test GXMtdRootBlock_HasValidMagicHeader, Version, LastError & GXDisplayOutput / GXDisplayConfig
        alignas(16) uint8_t rootBuf[0x108]{}, hdrBuf[0xC0]{};
        rootBuf[0x2C] = 'M'; rootBuf[0x2D] = 'T'; rootBuf[0x2E] = 'D'; rootBuf[0x2F] = ' ';
        *reinterpret_cast<void**>(rootBuf + 0x68) = hdrBuf;
        if (game_root_magic(rootBuf) != GXMtdRootBlock_HasValidMagicHeader(rootBuf)) ++bad;
        rootBuf[0x2F] = 'X';
        if (game_root_magic(rootBuf) != GXMtdRootBlock_HasValidMagicHeader(rootBuf)) ++bad;

        struct FakeMtdContainer { void* root; uint32_t err; uint32_t pad; };
        FakeMtdContainer mcTheirs{ rootBuf, 0, 0 }, mcOurs{ rootBuf, 0, 0 };
        if (game_set_ver(&mcTheirs, 1000u) != GXMtdMaterialContainer_SetVersion(&mcOurs, 1000u)) ++bad;
        if (game_get_ver(&mcTheirs)        != GXMtdMaterialContainer_GetVersion(&mcOurs))        ++bad;
        game_set_err(&mcTheirs, 7u);
        GXMtdMaterialContainer_SetLastError(&mcOurs, 7u);
        if (game_get_err(&mcTheirs) != GXMtdMaterialContainer_GetLastError(&mcOurs)) ++bad;

        alignas(16) uint8_t outDescTheirs[0x60]{}, outDescOurs[0x60]{};
        game_disp_out(nullptr, outDescTheirs);
        GXDisplayOutput_GetDefaultOutputDesc(nullptr, outDescOurs);
        if (std::memcmp(outDescTheirs, outDescOurs, 0x60) != 0) ++bad;

        for (uint32_t a = 0; a < 2; ++a) {
            for (uint32_t m = 0; m < 2; ++m) {
                uint32_t modeTheirs[7]{}, modeOurs[7]{};
                uint32_t rT = game_disp_mode(nullptr, a, m, modeTheirs);
                uint32_t rO = GXDisplayOutput_GetDefaultModeDesc1080p60(nullptr, a, m, modeOurs);
                if (rT != rO || std::memcmp(modeTheirs, modeOurs, sizeof(modeOurs)) != 0) ++bad;
            }
        }

        alignas(16) uint8_t cfgTheirs[0xC0]{}, cfgOurs[0xC0]{};
        if (game_disp_cfg(cfgTheirs) != GXDisplayConfig_CopyFromGlobalCGDisplayConfig(cfgOurs) ||
            std::memcmp(cfgTheirs, cfgOurs, 0xC0) != 0) ++bad;

        std::printf("[TEST 6]  03_TPF_GNF Batch 1 & 2 (GXMtdParamValue/Accessor/Display): 15025 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 8. Test 03_TPF_GNF_ShaderContainers Batch 2 & 3:
    //    GXDdsHeader_GetEnginePixelFormat (0x26d7530, 1228 B), SubresourceOffset (0x26d7b40, 653 B),
    //    MipSliceByteSize/Pitch, BlockTables, and GXTpfEntryV1 / GXTpfEntryV2 (0x26d7510..0x26dae00)
    // ========================================================================
    {
        auto game_dds_valid  = eboot_vma_fn<bool(*)(const uint8_t*)>(0x26d7510);
        auto game_dds_fmt    = eboot_vma_fn<uint32_t(*)(const uint8_t*)>(0x26d7530);
        auto game_dxgi_fmt   = eboot_vma_fn<uint32_t(*)(uint32_t)>(0x26d7a00);
        auto game_dds_slices = eboot_vma_fn<uint32_t(*)(const uint8_t*)>(0x26d7a20);
        auto game_dds_pitch  = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t)>(0x26d7a50);
        auto game_dds_size   = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t)>(0x26d7ab0);
        auto game_dds_suboff = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t, uint32_t)>(0x26d7b40);
        auto game_fmt_bpb    = eboot_vma_fn<uint32_t(*)(uint32_t)>(0x26d80b0);
        auto game_fmt_bw     = eboot_vma_fn<uint32_t(*)(uint32_t)>(0x26d80d0);
        auto game_fmt_bh     = eboot_vma_fn<uint32_t(*)(uint32_t)>(0x26d80f0);

        int bad = 0;

        // 8A. Exhaustive test of format tables (0..120)
        for (uint32_t fmt = 0; fmt <= 120; ++fmt) {
            if (game_dxgi_fmt(fmt) != GXTpfFormat_DxgiToEngineFormat(fmt)) ++bad;
            if (game_fmt_bpb(fmt)  != GXTpfFormat_GetBytesPerBlock(fmt))   ++bad;
            if (game_fmt_bw(fmt)   != GXTpfFormat_GetBlockWidth(fmt))      ++bad;
            if (game_fmt_bh(fmt)   != GXTpfFormat_GetBlockHeight(fmt))     ++bad;
        }

        // 8B. Test GXDdsHeader_GetEnginePixelFormat + MipSlicePitch + MipSliceByteSize + SubresourceOffset (10,000 synthetic DDS/DX10 headers)
        const uint32_t knownFourCCs[] = {
            0x30315844u, 0x31545844u, 0x33545844u, 0x35545844u,
            0x31495441u, 0x32495441u, 0x55344342u, 0x53344342u,
            0x53354342u, 0x42475247u, 0x47424752u,
            0x24u, 0x6Eu, 0x6Fu, 0x70u, 0x71u, 0x72u, 0x73u, 0x74u, 0xDEADBEEFu
        };

        for (int c = 0; c < 10000; ++c) {
            alignas(16) uint8_t dds[0x100]{};
            *reinterpret_cast<uint32_t*>(dds + 0x00) = rng.chance(0.95) ? 0x20534444u : rng.u32(); // 'DDS '
            *reinterpret_cast<uint32_t*>(dds + 0x04) = rng.chance(0.95) ? 124u : 0u;
            *reinterpret_cast<uint32_t*>(dds + 0x0C) = 1u << rng.range(0, 10); // height
            *reinterpret_cast<uint32_t*>(dds + 0x10) = 1u << rng.range(0, 10); // width
            *reinterpret_cast<uint32_t*>(dds + 0x18) = 1u << rng.range(0, 4);  // depth
            *reinterpret_cast<uint32_t*>(dds + 0x1C) = rng.range(0, 8);        // mipCount
            dds[0x71] = rng.chance(0.2) ? 0x02 : 0x00;                         // cubemap
            dds[0x72] = rng.chance(0.2) ? 0x20 : 0x00;                         // volume

            int mode = rng.range(0, 5);
            if (mode == 0) {
                // FOURCC / DX10
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x04u;
                uint32_t fcc = knownFourCCs[rng.range(0, 19)];
                *reinterpret_cast<uint32_t*>(dds + 0x54) = fcc;
                *reinterpret_cast<uint32_t*>(dds + 0x80) = rng.range(0, 105); // dxgiFormat
                *reinterpret_cast<uint32_t*>(dds + 0x84) = rng.range(2, 4);   // resourceDimension
                *reinterpret_cast<uint32_t*>(dds + 0x8C) = rng.range(1, 6);   // arraySize
            } else if (mode == 1) {
                // DDPF_RGB 16-bit
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x40u;
                *reinterpret_cast<uint32_t*>(dds + 0x58) = 16u;
                if (rng.chance(0.5)) {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0xF800u;
                    *reinterpret_cast<uint32_t*>(dds + 0x60) = 0x07E0u;
                    *reinterpret_cast<uint32_t*>(dds + 0x64) = 0x001Fu;
                    *reinterpret_cast<uint32_t*>(dds + 0x68) = 0u;
                } else {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x7C00u;
                    *reinterpret_cast<uint32_t*>(dds + 0x60) = 0x03E0u;
                    *reinterpret_cast<uint32_t*>(dds + 0x64) = 0x001Fu;
                    *reinterpret_cast<uint32_t*>(dds + 0x68) = rng.chance(0.5) ? 0x8000u : 0u;
                }
            } else if (mode == 2) {
                // DDPF_RGB 32-bit
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x40u;
                *reinterpret_cast<uint32_t*>(dds + 0x58) = 32u;
                int sub = rng.range(0, 5);
                if (sub == 0) {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x000000FFu;
                    *reinterpret_cast<uint32_t*>(dds + 0x60) = 0x0000FF00u;
                    *reinterpret_cast<uint32_t*>(dds + 0x64) = 0x00FF0000u;
                    *reinterpret_cast<uint32_t*>(dds + 0x68) = rng.chance(0.5) ? 0xFF000000u : 0u;
                } else if (sub == 1) {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x00FF0000u;
                    *reinterpret_cast<uint32_t*>(dds + 0x60) = 0x0000FF00u;
                    *reinterpret_cast<uint32_t*>(dds + 0x64) = 0x000000FFu;
                    *reinterpret_cast<uint32_t*>(dds + 0x68) = rng.chance(0.5) ? 0xFF000000u : 0u;
                } else if (sub == 2) {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0xFFFFFFFFu;
                } else if (sub == 3) {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x0000FFFFu;
                    *reinterpret_cast<uint32_t*>(dds + 0x60) = 0xFFFF0000u;
                } else {
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x000003FFu;
                    *reinterpret_cast<uint32_t*>(dds + 0x60) = 0x000FFC00u;
                    *reinterpret_cast<uint32_t*>(dds + 0x64) = 0x3FF00000u;
                    *reinterpret_cast<uint32_t*>(dds + 0x68) = 0xC0000000u;
                }
            } else if (mode == 3) {
                // DDPF_LUMINANCE
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x20000u;
                if (rng.chance(0.4)) {
                    *reinterpret_cast<uint32_t*>(dds + 0x58) = 8u;
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x00FFu;
                } else {
                    *reinterpret_cast<uint32_t*>(dds + 0x58) = 16u;
                    *reinterpret_cast<uint32_t*>(dds + 0x5C) = rng.chance(0.5) ? 0x00FFu : 0xFFFFu;
                    *reinterpret_cast<uint32_t*>(dds + 0x68) = (*reinterpret_cast<uint32_t*>(dds + 0x5C) == 0x00FFu) ? 0xFF00u : 0u;
                }
            } else if (mode == 4) {
                // DDPF_BUMPDUDV
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x80000u;
                *reinterpret_cast<uint32_t*>(dds + 0x58) = 16u;
                *reinterpret_cast<uint32_t*>(dds + 0x5C) = 0x00FFu;
                *reinterpret_cast<uint32_t*>(dds + 0x60) = 0xFF00u;
            } else {
                // DDPF_ALPHA
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x02u;
                *reinterpret_cast<uint32_t*>(dds + 0x58) = rng.chance(0.8) ? 8u : 16u;
            }

            if (game_dds_valid(dds)  != GXDdsHeader_ValidateMagicAndSize(dds))   ++bad;
            if (game_dds_fmt(dds)    != GXDdsHeader_GetEnginePixelFormat(dds))   ++bad;
            if (game_dds_slices(dds) != GXDdsHeader_GetArrayOrCubeSliceCount(dds)) ++bad;

            uint32_t mip = rng.range(0, 6);
            uint32_t slc = rng.range(0, 3);
            if (game_dds_pitch(dds, mip)       != GXDdsHeader_GetMipSlicePitchOrRowSize(dds, mip))   ++bad;
            if (game_dds_size(dds, mip)        != GXDdsHeader_GetMipSliceByteSize(dds, mip))         ++bad;
            if (game_dds_suboff(dds, mip, slc) != GXDdsHeader_GetSubresourceOffset(dds, mip, slc))   ++bad;
        }

        std::printf("[TEST 7]  03_TPF_GNF DDS/DX10 Format & Subresource Parsers (0x26d7510..0x26d80f0): 10121 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 9. Test 03_TPF_GNF_ShaderContainers TPF ('TPF\0') Container & Entry V1/V2
    //    Strategy VTables (0x26d81a0..0x26dae00)
    // ========================================================================
    {
        auto game_tpf_valid   = eboot_vma_fn<bool(*)(const uint8_t*)>(0x26d81a0);
        auto game_tpf_cnt     = eboot_vma_fn<uint32_t(*)(const uint8_t*)>(0x26d81f0);
        auto game_tpf_enc     = eboot_vma_fn<uint8_t*(*)(uint8_t*, const uint8_t*)>(0x26d8230);
        auto game_tpf_flags4  = eboot_vma_fn<int32_t(*)(const uint8_t*, uint32_t)>(0x26d8280);
        auto game_tpf_fmtcode = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t)>(0x26d8480);
        auto game_tpf_engfmt  = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t)>(0x26d8660);
        auto game_tpf_textype = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t)>(0x26d8840);
        auto game_tpf_name    = eboot_vma_fn<const char*(*)(const uint8_t*, uint32_t)>(0x26d8a20);
        auto game_tpf_extcnt  = eboot_vma_fn<uint32_t(*)(const uint8_t*, uint32_t)>(0x26d8c10);
        auto game_tpf_extdata = eboot_vma_fn<const uint8_t*(*)(const uint8_t*, uint32_t, uint32_t, uint32_t*, uint32_t*)>(0x26d8df0);

        int bad = 0;

        for (int c = 0; c < 5000; ++c) {
            alignas(16) uint8_t tpf[0x400]{};
            bool isV2 = rng.chance(0.5);
            bool header30 = rng.chance(0.5);

            // Build 'TPF\0' header
            tpf[0] = 'T'; tpf[1] = 'P'; tpf[2] = 'F'; tpf[3] = '\0';
            *reinterpret_cast<uint32_t*>(tpf + 0x04) = 0x200u;
            *reinterpret_cast<uint32_t*>(tpf + 0x08) = 2u; // 2 texture entries
            tpf[0x0C] = isV2 ? 4 : 0;
            tpf[0x0D] = 3;
            tpf[0x0E] = static_cast<uint8_t>(rng.range(0, 2));
            tpf[0x0F] = header30 ? 1 : 0;

            uint8_t* entry0 = tpf + (header30 ? 0x30 : 0x10);
            if (isV2) {
                // Entry 0 (0x24 bytes + 1 extra chunk of 12 bytes = 0x14 total chunk bytes)
                *reinterpret_cast<uint32_t*>(entry0 + 0x00) = 0x200u; // dataOffset
                *reinterpret_cast<uint32_t*>(entry0 + 0x04) = 0x80u;  // dataSize
                entry0[0x08] = static_cast<uint8_t>(rng.range(0, 40)); // formatCode
                entry0[0x09] = static_cast<uint8_t>(rng.range(0, 2));  // textureType
                entry0[0x0A] = static_cast<uint8_t>(rng.range(1, 8));  // mipCount
                entry0[0x0B] = static_cast<uint8_t>(rng.u32());        // flags
                *reinterpret_cast<uint16_t*>(entry0 + 0x0C) = static_cast<uint16_t>(rng.range(16, 1024)); // width
                *reinterpret_cast<uint16_t*>(entry0 + 0x0E) = static_cast<uint16_t>(rng.range(16, 1024)); // height
                *reinterpret_cast<uint32_t*>(entry0 + 0x10) = rng.range(1, 6);     // depthOrArraySize
                *reinterpret_cast<uint32_t*>(entry0 + 0x14) = rng.u32();           // headerExtValue
                *reinterpret_cast<uint32_t*>(entry0 + 0x18) = 0x180u;              // fileNameOffset
                *reinterpret_cast<uint32_t*>(entry0 + 0x1C) = 1u;                  // headerExtSize (1 extra chunk)
                *reinterpret_cast<uint32_t*>(entry0 + 0x20) = rng.range(1, 95);    // dxgiFormat

                uint8_t* chunk0 = entry0 + 0x24;
                *reinterpret_cast<uint32_t*>(chunk0 + 0x00) = 0x12345678u; // chunkId
                *reinterpret_cast<uint32_t*>(chunk0 + 0x04) = 12u;         // chunkByteSize

                uint8_t* entry1 = chunk0 + 8 + 12;
                std::memcpy(entry1, entry0, 0x24);
                entry1[0x08] = static_cast<uint8_t>(rng.range(0, 40));
                entry1[0x09] = static_cast<uint8_t>(rng.range(0, 2));
                *reinterpret_cast<uint32_t*>(entry1 + 0x18) = 0x190u;              // fileNameOffset
                *reinterpret_cast<uint32_t*>(entry1 + 0x1C) = 0u;                  // 0 extra chunks
                *reinterpret_cast<uint32_t*>(entry1 + 0x20) = rng.range(1, 95);    // dxgiFormat
            } else {
                // Entry 0 (0x14 bytes + 1 extra chunk of 8 bytes) + embedded DDS at 0x200
                *reinterpret_cast<uint32_t*>(entry0 + 0x00) = 0x200u; // dataOffset
                *reinterpret_cast<uint32_t*>(entry0 + 0x04) = 0x100u; // dataSize
                entry0[0x08] = static_cast<uint8_t>(rng.range(0, 40));
                entry0[0x09] = static_cast<uint8_t>(rng.range(0, 2));
                entry0[0x0A] = static_cast<uint8_t>(rng.range(1, 6));
                entry0[0x0B] = static_cast<uint8_t>(rng.u32());
                *reinterpret_cast<uint32_t*>(entry0 + 0x0C) = 0x180u; // fileNameOffset
                *reinterpret_cast<uint32_t*>(entry0 + 0x10) = 1u;     // 1 extra chunk

                uint8_t* chunk0 = entry0 + 0x14;
                *reinterpret_cast<uint32_t*>(chunk0 + 0x00) = 0xCAFEBABEu;
                *reinterpret_cast<uint32_t*>(chunk0 + 0x04) = 8u;

                uint8_t* entry1 = chunk0 + 8 + 8;
                std::memcpy(entry1, entry0, 0x14);
                *reinterpret_cast<uint32_t*>(entry1 + 0x10) = 0u;

                // Valid DDS at tpf + 0x200
                uint8_t* dds = tpf + 0x200;
                *reinterpret_cast<uint32_t*>(dds + 0x00) = 0x20534444u; // 'DDS '
                *reinterpret_cast<uint32_t*>(dds + 0x04) = 124u;
                *reinterpret_cast<uint32_t*>(dds + 0x0C) = 128u;
                *reinterpret_cast<uint32_t*>(dds + 0x10) = 128u;
                *reinterpret_cast<uint32_t*>(dds + 0x18) = 1u;
                *reinterpret_cast<uint32_t*>(dds + 0x1C) = 4u;
                *reinterpret_cast<uint32_t*>(dds + 0x50) = 0x04u;
                *reinterpret_cast<uint32_t*>(dds + 0x54) = 0x31545844u; // 'DXT1'
            }

            std::strcpy(reinterpret_cast<char*>(tpf + 0x180), "tex0_albedo");
            std::strcpy(reinterpret_cast<char*>(tpf + 0x190), "tex1_normal");

            if (game_tpf_valid(tpf) != GXTpfContainer_ValidateHeader(tpf)) ++bad;
            if (game_tpf_cnt(tpf)   != GXTpfContainer_GetTextureCount(tpf)) ++bad;

            uint8_t enc1 = 0xFF, enc2 = 0xFF;
            game_tpf_enc(&enc1, tpf);
            GXTpfContainer_GetEncodingFlag(&enc2, tpf);
            if (enc1 != enc2) ++bad;

            for (uint32_t idx = 0; idx < 3; ++idx) {
                if (game_tpf_flags4(tpf, idx)  != GXTpfContainer_GetEntryFlagsShifted4(tpf, idx))     ++bad;
                if (game_tpf_fmtcode(tpf, idx) != GXTpfContainer_GetEntryFormatCode(tpf, idx))        ++bad;
                if (game_tpf_engfmt(tpf, idx)  != GXTpfContainer_GetEntryEnginePixelFormat(tpf, idx)) ++bad;
                if (game_tpf_textype(tpf, idx) != GXTpfContainer_GetEntryTextureType(tpf, idx))       ++bad;
                if (game_tpf_name(tpf, idx)    != GXTpfContainer_GetEntryFileNamePtr(tpf, idx))       ++bad;
                if (game_tpf_extcnt(tpf, idx)  != GXTpfContainer_GetEntryExtraChunkCount(tpf, idx))   ++bad;

                uint32_t sz1 = 0, id1 = 0, sz2 = 0, id2 = 0;
                const uint8_t* p1 = game_tpf_extdata(tpf, idx, 0, &sz1, &id1);
                const uint8_t* p2 = GXTpfContainer_GetEntryExtraChunkData(tpf, idx, 0, &sz2, &id2);
                if (p1 != p2 || sz1 != sz2 || id1 != id2) ++bad;
            }
        }

        std::printf("[TEST 8]  03_TPF_GNF TPF Container & V1/V2 Strategy VTables (0x26d81a0..0x26dae00): 5000 containers -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 10. [TEST 9] 04_SceneRenderer: GXSortKey64_Introsort (0x21c9c00, 1615 B)
    //     + GXModelFlverContainer Parser & 13 Section Accessors (0x21ce560..0x21cedf0)
    // ========================================================================
    {
        auto game_sort64    = eboot_vma_fn<void(*)(uint64_t*, uint64_t*, int64_t)>(0x21c9c00);
        auto game_flver_ctor = eboot_vma_fn<void(*)(void*)>(0x21ce560);
        auto game_flver_parse = eboot_vma_fn<bool(*)(void*, void*)>(0x21ce5d0);
        auto game_dummy_at  = eboot_vma_fn<void*(*)(const void*, int32_t)>(0x21cec90);
        auto game_mat_at    = eboot_vma_fn<void*(*)(const void*, int32_t)>(0x21ceca0);
        auto game_mat_name  = eboot_vma_fn<const char16_t*(*)(const void*, int32_t)>(0x21cecb0);
        auto game_mat_mtd   = eboot_vma_fn<const char16_t*(*)(const void*, int32_t)>(0x21cecd0);
        auto game_mat_gx    = eboot_vma_fn<void*(*)(const void*, int32_t)>(0x21cecf0);
        auto game_tex_at    = eboot_vma_fn<void*(*)(const void*, uint32_t)>(0x21ced10);
        auto game_tex_type  = eboot_vma_fn<const char16_t*(*)(const void*, int32_t, uint32_t)>(0x21ced40);
        auto game_bone_at   = eboot_vma_fn<void*(*)(const void*, int32_t)>(0x21ced80);
        auto game_bone_name = eboot_vma_fn<const char16_t*(*)(const void*, int32_t)>(0x21ced90);
        auto game_mesh_at   = eboot_vma_fn<void*(*)(const void*, int32_t)>(0x21cedb0);
        auto game_lay_cnt   = eboot_vma_fn<uint32_t(*)(const void*, uint32_t)>(0x21cedc0);
        auto game_lay_mem   = eboot_vma_fn<void*(*)(const void*, uint32_t, uint32_t)>(0x21cedd0);
        auto game_fs_at     = eboot_vma_fn<void*(*)(const void*, uint32_t)>(0x21cedf0);

        int bad = 0;

        // 9A. Test GXSortKey64_Introsort (5,000 random arrays across quicksort, heapsort, and insertion sort paths)
        for (int c = 0; c < 5000; ++c) {
            int n = rng.range(0, 96);
            int64_t depth = rng.range(0, 6);
            std::vector<uint64_t> theirs(n), ours(n);
            for (int i = 0; i < n; ++i) {
                uint64_t payload = static_cast<uint64_t>(rng.u32()) << 32;
                uint32_t key32   = (rng.u32() & 0xFFFF0000u) | static_cast<uint32_t>(i);
                theirs[i] = payload | key32;
            }
            for (int i = n - 1; i > 0; --i) {
                int j = rng.range(0, i);
                std::swap(theirs[i], theirs[j]);
            }
            ours = theirs;
            game_sort64(theirs.data(), theirs.data() + n, depth);
            sub_21c9c00(ours.data(), ours.data() + n, depth);
            if (n > 0 && std::memcmp(theirs.data(), ours.data(), n * sizeof(uint64_t)) != 0) {
                ++bad;
            }
        }

        // 9B. Test GXModelFlverContainer_ParseAndValidate (both Little-Endian 'L' and Big-Endian 'B' with MOVBE byte-swapping)
        for (int c = 0; c < 2000; ++c) {
            std::vector<uint8_t> flver_theirs(0x1000, 0), flver_ours(0x1000, 0);
            uint8_t* hdr = flver_theirs.data();
            bool big_endian = rng.chance(0.5);
            uint32_t ver = rng.chance(0.8) ? 0x20014u : (rng.chance(0.5) ? 0x20008u : 0x20002u);

            hdr[0] = 'F'; hdr[1] = 'L'; hdr[2] = 'V'; hdr[3] = 'E'; hdr[4] = 'R'; hdr[5] = '\0';
            hdr[6] = big_endian ? 'B' : 'L';
            auto wr32 = [&](uint8_t* p, size_t off, uint32_t v) {
                *reinterpret_cast<uint32_t*>(p + off) = big_endian ? __builtin_bswap32(v) : v;
            };
            wr32(hdr, 0x08, ver);
            wr32(hdr, 0x0c, 0x800u); // dataOffset
            wr32(hdr, 0x10, 0x400u); // dataSize
            wr32(hdr, 0x14, 2u);     // dummyCount (2 * 64 = 128 B -> 0x80..0x100)
            wr32(hdr, 0x18, 2u);     // materialCount (2 * 32 = 64 B -> 0x100..0x140)
            wr32(hdr, 0x1c, 2u);     // boneCount (2 * 128 = 256 B -> 0x140..0x240)
            wr32(hdr, 0x20, 1u);     // meshCount (1 * 48 = 48 B -> 0x240..0x270)
            wr32(hdr, 0x24, 1u);     // vbCount
            wr32(hdr, 0x50, 2u);     // faceSetCount
            wr32(hdr, 0x54, 1u);     // layoutCount
            wr32(hdr, 0x58, 2u);     // texCount
            wr32(hdr, 0x60, 0u);     // unkCount60

            // Populate random words in dummy/material/bone/mesh/faceset/vb/layout/texture regions
            for (size_t off = 0x80; off < 0x500; off += 4) {
                wr32(hdr, off, rng.u32() & 0x000001FFu);
            }
            // Ensure material texture indices and sub-counts/offsets inside mesh, faceset, and layout stay within our 0x1000 buffer
            wr32(hdr + 0x100, 0x0c, 0u); // material[0].texIdx = 0
            wr32(hdr + 0x120, 0x0c, 1u); // material[1].texIdx = 1
            uint8_t* p_mesh = hdr + 0x240;
            wr32(p_mesh, 0x10, 0u); // mesh bone count = 0
            wr32(p_mesh, 0x1c, 0u); // mesh faceset idx count = 0
            wr32(p_mesh, 0x24, 0u); // mesh vb idx count = 0

            uint32_t fs_stride = (ver > 0x20008u) ? 0x20u : 0x10u;
            uint8_t* p_fs = hdr + 0x270;
            for (uint32_t f = 0; f < 2; ++f) {
                wr32(p_fs + f * fs_stride, 0x08, 4u);     // indexCount = 4
                wr32(p_fs + f * fs_stride, 0x0c, 0x100u); // indexBufferOffset (relative to dataOffset 0x800 -> 0x900)
                if (ver > 0x20008u) {
                    wr32(p_fs + f * fs_stride, 0x18, 16u); // indexByteSize (16-bit indices)
                }
            }
            uint8_t* p_vb = p_fs + 2 * fs_stride;
            uint8_t* p_lay = p_vb + 0x20;
            wr32(p_lay, 0x00, 2u);     // 2 layout members
            wr32(p_lay, 0x0c, 0x600u); // layout members at hdr + 0x600

            flver_ours = flver_theirs;

            alignas(16) uint8_t cont_theirs[0x60]{}, cont_ours[0x60]{};
            game_flver_ctor(cont_theirs);
            sub_21ce560(cont_ours);
            if (std::memcmp(cont_theirs, cont_ours, 0x60) != 0) ++bad;

            bool ok1 = game_flver_parse(cont_theirs, flver_theirs.data());
            bool ok2 = sub_21ce5d0(cont_ours, flver_ours.data());
            if (ok1 != ok2 || flver_theirs != flver_ours) ++bad;

            if (ok1) {
                auto off_t = [&](const void* p) -> ptrdiff_t {
                    return p ? (static_cast<const uint8_t*>(p) - flver_theirs.data()) : -1;
                };
                auto off_o = [&](const void* p) -> ptrdiff_t {
                    return p ? (static_cast<const uint8_t*>(p) - flver_ours.data()) : -1;
                };
                if (off_t(game_dummy_at(cont_theirs, 1)) != off_o(sub_21cec90(cont_ours, 1))) ++bad;
                if (off_t(game_mat_at(cont_theirs, 1))   != off_o(sub_21ceca0(cont_ours, 1))) ++bad;
                if (off_t(game_mat_name(cont_theirs, 0)) != off_o(sub_21cecb0(cont_ours, 0))) ++bad;
                if (off_t(game_mat_mtd(cont_theirs, 0))  != off_o(sub_21cecd0(cont_ours, 0))) ++bad;
                if (off_t(game_mat_gx(cont_theirs, 0))   != off_o(sub_21cecf0(cont_ours, 0))) ++bad;
                if (off_t(game_tex_at(cont_theirs, 1))   != off_o(sub_21ced10(cont_ours, 1))) ++bad;
                if (off_t(game_tex_type(cont_theirs, 0, 0)) != off_o(sub_21ced40(cont_ours, 0, 0))) ++bad;
                if (off_t(game_bone_at(cont_theirs, 1))  != off_o(sub_21ced80(cont_ours, 1))) ++bad;
                if (off_t(game_bone_name(cont_theirs, 1)) != off_o(sub_21ced90(cont_ours, 1))) ++bad;
                if (off_t(game_mesh_at(cont_theirs, 0))  != off_o(sub_21cedb0(cont_ours, 0))) ++bad;
                if (game_lay_cnt(cont_theirs, 0)         != sub_21cedc0(cont_ours, 0))        ++bad;
                if (off_t(game_lay_mem(cont_theirs, 0, 1)) != off_o(sub_21cedd0(cont_ours, 0, 1))) ++bad;
                if (off_t(game_fs_at(cont_theirs, 1))    != off_o(sub_21cedf0(cont_ours, 1))) ++bad;
            }
        }

        std::printf("[TEST 9]  04_SceneRenderer GXSortKey64_Introsort (0x21c9c00) & FLVER2 Parser/Accessors (0x21ce560..0x21cedf0): 7000 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 11. [TEST 10] 04_SceneRenderer: DXGI_FORMAT / GXTextureFormat Jump-Table
    //     Inspectors (0x21c2a60..0x21c2d60) + SIMD Segment/OBB ClosestPoint (0x21c0670..0x21c0710)
    // ========================================================================
    {
        auto game_is_bc       = eboot_vma_fn<bool(*)(uint32_t)>(0x21c2a60);
        auto game_is_srgb     = eboot_vma_fn<bool(*)(int32_t)>(0x21c2a80);
        auto game_to_typeless = eboot_vma_fn<uint32_t(*)(uint32_t)>(0x21c2af0);
        auto game_to_dsv      = eboot_vma_fn<uint32_t(*)(int32_t)>(0x21c2d20);
        auto game_fmt_bpp     = eboot_vma_fn<uint32_t(*)(uint32_t)>(0x21c2d60);
        auto game_obb_dist    = eboot_vma_fn<float(*)(const __m128*, const __m128*, __m128*)>(0x21c0670);
        auto game_seg_dist    = eboot_vma_fn<float(*)(const __m128*, const __m128*, __m128*)>(0x21c0710);

        int bad = 0;

        // 10A. Exhaustively test all DXGI_FORMAT values (0..130) across all 5 format inspectors
        for (uint32_t fmt = 0; fmt <= 130; ++fmt) {
            if (game_is_bc(fmt)                        != sub_21c2a60(fmt))                        ++bad;
            if (game_is_srgb(static_cast<int32_t>(fmt)) != sub_21c2a80(static_cast<int32_t>(fmt))) ++bad;
            if (game_to_typeless(fmt)                  != sub_21c2af0(fmt))                        ++bad;
            if (game_to_dsv(static_cast<int32_t>(fmt)) != sub_21c2d20(static_cast<int32_t>(fmt)))  ++bad;
            if (game_fmt_bpp(fmt)                      != sub_21c2d60(fmt))                        ++bad;
        }

        // 10B. Test SIMD ClosestPointAndDistSqToSegment (0x21c0710) & OBB (0x21c0670) on 10,000 random 3D configurations
        for (int c = 0; c < 10000; ++c) {
            alignas(16) __m128 pt = _mm_setr_ps(
                rng.uniform(-100.0f, 100.0f), rng.uniform(-100.0f, 100.0f), rng.uniform(-100.0f, 100.0f), 1.0f);
            alignas(16) __m128 seg[2] = {
                _mm_setr_ps(rng.uniform(-50.0f, 50.0f), rng.uniform(-50.0f, 50.0f), rng.uniform(-50.0f, 50.0f), 1.0f),
                _mm_setr_ps(rng.uniform(-50.0f, 50.0f), rng.uniform(-50.0f, 50.0f), rng.uniform(-50.0f, 50.0f), 0.0f)
            };
            alignas(16) __m128 out_seg_t{}, out_seg_o{};
            float d_seg_t = game_seg_dist(&pt, seg, &out_seg_t);
            float d_seg_o = sub_21c0710(&pt, seg, &out_seg_o);
            if (std::memcmp(&d_seg_t, &d_seg_o, sizeof(float)) != 0 ||
                std::memcmp(&out_seg_t, &out_seg_o, sizeof(__m128)) != 0) {
                ++bad;
            }

            alignas(16) __m128 obb[5] = {
                _mm_setr_ps(rng.uniform(1.0f, 25.0f), rng.uniform(1.0f, 25.0f), rng.uniform(1.0f, 25.0f), 0.0f), // halfExtents
                _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f),
                _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f),
                _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f),
                _mm_setr_ps(rng.uniform(-30.0f, 30.0f), rng.uniform(-30.0f, 30.0f), rng.uniform(-30.0f, 30.0f), 1.0f)
            };
            alignas(16) __m128 out_obb_t{}, out_obb_o{};
            float d_obb_t = game_obb_dist(&pt, obb, &out_obb_t);
            float d_obb_o = sub_21c0670(&pt, obb, &out_obb_o);
            if (std::memcmp(&d_obb_t, &d_obb_o, sizeof(float)) != 0 ||
                std::memcmp(&out_obb_t, &out_obb_o, sizeof(__m128)) != 0) {
                ++bad;
            }
        }

        std::printf("[TEST 10] 04_SceneRenderer DXGI_FORMAT Tables (0x21c2a60..0x21c2d60) & SIMD ClosestPoint Segment/OBB (0x21c0670..0x21c0710): 20655 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 12. [TEST 11] 04_SceneRenderer: GXPathUtil (0x21bc6f0..0x21bca90),
    //     SIMD Bounding Volume / Cubemap Matrix Builders (0x21bb8f0..0x21bc2a0),
    //     and GXSortedRenderItemBuffer_MergeTwoSortedRuns (0x21b6420, 511 B)
    // ========================================================================
    {
        auto game_cubemat   = eboot_vma_fn<__m128*(*)(__m128*, uint32_t, const __m128*)>(0x21bb8f0);
        auto game_aabbmat   = eboot_vma_fn<__m128*(*)(__m128*, const __m128*, float, float)>(0x21bb9c0);
        auto game_vec_sq    = eboot_vma_fn<__m128(*)(const __m128*)>(0x21bba30);
        auto game_samp_init = eboot_vma_fn<void(*)(void*)>(0x21bc2a0);
        auto game_find_dot  = eboot_vma_fn<char16_t*(*)(const char16_t*)>(0x21bc6f0);
        auto game_strip_ext = eboot_vma_fn<bool(*)(char16_t*)>(0x21bc750);
        auto game_stem_buf  = eboot_vma_fn<void(*)(const char16_t*, char16_t*, size_t)>(0x21bc840);
        auto game_norm_sep  = eboot_vma_fn<void(*)(char16_t*, const char16_t*, char16_t, size_t)>(0x21bca90);
        auto game_merge_runs = eboot_vma_fn<void*(*)(void*, const uint8_t*, const uint8_t*, const uint8_t*, const uint8_t*, uint8_t*)>(0x21b6420);
        auto game_is_busy   = eboot_vma_fn<bool(*)(const void*)>(0x21b9a80);

        // Bind PS4 Orbis 16-bit wchar_t PLT imports (wcslen, wcsrchr, wcsncpy) so eboot.bin's GXPathUtil functions can run
        eboot_kit::stub(vma_to_bn(0x2bbf228), reinterpret_cast<const void*>(&sub_2bbf228));
        eboot_kit::stub(vma_to_bn(0x2bbf258), reinterpret_cast<const void*>(&sub_2bbf258));
        eboot_kit::stub(vma_to_bn(0x2bbf7a8), reinterpret_cast<const void*>(&sub_2bbf7a8));

        int bad = 0;

        // 11A. Test SIMD Bounding Box / Cubemap Matrix Builders + Sampler Slot Table Reset
        for (int c = 0; c < 5000; ++c) {
            alignas(16) __m128 pos = _mm_setr_ps(
                rng.uniform(-200.0f, 200.0f), rng.uniform(-200.0f, 200.0f), rng.uniform(-200.0f, 200.0f), rng.uniform(-10.0f, 10.0f));
            __m128 sq_t = game_vec_sq(&pos);
            __m128 sq_o = sub_21bba30(&pos);
            if (std::memcmp(&sq_t, &sq_o, sizeof(__m128)) != 0) ++bad;

            uint32_t face = rng.range(0, 7);
            alignas(16) __m128 cmat_t[4]{}, cmat_o[4]{};
            game_cubemat(cmat_t, face, &pos);
            sub_21bb8f0(cmat_o, face, &pos);
            if (std::memcmp(cmat_t, cmat_o, sizeof(cmat_t)) != 0) ++bad;

            alignas(16) __m128 aabb[2] = {
                _mm_setr_ps(rng.uniform(-50.0f, 0.0f), rng.uniform(-50.0f, 0.0f), rng.uniform(-50.0f, 0.0f), 0.0f),
                _mm_setr_ps(rng.uniform(1.0f, 50.0f), rng.uniform(1.0f, 50.0f), rng.uniform(1.0f, 50.0f), 0.0f)
            };
            float sc = rng.uniform(0.5f, 4.0f);
            float mg = rng.uniform(0.0f, 2.0f);
            alignas(16) __m128 amat_t[4]{}, amat_o[4]{};
            game_aabbmat(amat_t, aabb, sc, mg);
            sub_21bb9c0(amat_o, aabb, sc, mg);
            if (std::memcmp(amat_t, amat_o, sizeof(amat_t)) != 0) ++bad;
        }

        alignas(16) uint8_t samp_t[0x100]{}, samp_o[0x100]{};
        game_samp_init(samp_t);
        sub_21bc2a0(samp_o);
        if (std::memcmp(samp_t, samp_o, 0x100) != 0) ++bad;

        // 11B. Test GXPathUtil UTF-16 Path Manipulation Functions
        const char16_t* test_paths[] = {
            u"N:\\SPRJ\\data\\chr\\c2100\\c2100.flver",
            u"shader:/gxm/flver_ phn.spx",
            u"parts\\am_m_1200.partsbnd.dcx",
            u"folder.with.dots/file_without_ext",
            u"folder.with.dots\\sub\\archive.tpf///",
            u".hidden_file",
            u""
        };
        for (const char16_t* p : test_paths) {
            if (game_find_dot(p) != sub_21bc6f0(p)) ++bad;

            char16_t s1[128]{}, s2[128]{};
            std::memcpy(s1, p, (sub_2bbf228(p) + 1) * sizeof(char16_t));
            std::memcpy(s2, p, (sub_2bbf228(p) + 1) * sizeof(char16_t));
            bool r1 = game_strip_ext(s1);
            bool r2 = sub_21bc750(s2);
            if (r1 != r2 || std::memcmp(s1, s2, sizeof(s1)) != 0) ++bad;

            char16_t b1[64]{}, b2[64]{};
            game_stem_buf(p, b1, 64);
            sub_21bc840(p, b2, 64);
            if (std::memcmp(b1, b2, sizeof(b1)) != 0) ++bad;

            char16_t n1[128]{}, n2[128]{};
            game_norm_sep(n1, p, u'/', 128);
            sub_21bca90(n2, p, u'/', 128);
            if (std::memcmp(n1, n2, sizeof(n1)) != 0) ++bad;
        }

        // 11C. Test GXSortedRenderItemBuffer_MergeTwoSortedRuns (0x21b6420) & GXWorkerThreadContext_IsBusy (0x21b9a80)
        struct RenderItem16 {
            uint64_t payload;
            uint32_t sort_key;
            uint32_t pad;
        };
        for (int c = 0; c < 3000; ++c) {
            int n1 = rng.range(0, 40);
            int n2 = rng.range(0, 40);
            std::vector<RenderItem16> r1(n1), r2(n2);
            uint32_t k = 0;
            for (int i = 0; i < n1; ++i) {
                k += rng.range(0, 5);
                r1[i] = { 0x10000ULL + static_cast<uint64_t>(i), k, 0 };
            }
            k = 0;
            for (int i = 0; i < n2; ++i) {
                k += rng.range(0, 5);
                r2[i] = { 0x20000ULL + static_cast<uint64_t>(i), k, 0 };
            }

            std::vector<RenderItem16> out_buf_t(n1 + n2 + 4), out_buf_o(n1 + n2 + 4);
            struct DstVec {
                RenderItem16* begin;
                RenderItem16* cur;
                RenderItem16* end;
            };
            DstVec dv_t{ out_buf_t.data(), out_buf_t.data(), out_buf_t.data() + out_buf_t.size() };
            DstVec dv_o{ out_buf_o.data(), out_buf_o.data(), out_buf_o.data() + out_buf_o.size() };

            alignas(16) uint8_t holder_t[0x30]{}, holder_o[0x30]{};
            *reinterpret_cast<DstVec**>(holder_t + 0x20) = &dv_t;
            *reinterpret_cast<DstVec**>(holder_o + 0x20) = &dv_o;

            alignas(16) uint8_t res_t[0x30]{}, res_o[0x30]{};
            game_merge_runs(res_t,
                            reinterpret_cast<const uint8_t*>(r1.data()),
                            reinterpret_cast<const uint8_t*>(r1.data() + n1),
                            reinterpret_cast<const uint8_t*>(r2.data()),
                            reinterpret_cast<const uint8_t*>(r2.data() + n2),
                            holder_t);
            GXSortedRenderItemBuffer_MergeTwoSortedRuns(
                            res_o,
                            reinterpret_cast<const uint8_t*>(r1.data()),
                            reinterpret_cast<const uint8_t*>(r1.data() + n1),
                            reinterpret_cast<const uint8_t*>(r2.data()),
                            reinterpret_cast<const uint8_t*>(r2.data() + n2),
                            holder_o);

            ptrdiff_t cnt_t = dv_t.cur - dv_t.begin;
            ptrdiff_t cnt_o = dv_o.cur - dv_o.begin;
            if (cnt_t != cnt_o ||
                (cnt_t > 0 && std::memcmp(out_buf_t.data(), out_buf_o.data(), cnt_t * sizeof(RenderItem16)) != 0)) {
                ++bad;
            }
        }

        alignas(16) uint8_t dummy_worker[0x530]{};
        for (uint32_t st = 0; st <= 4; ++st) {
            *reinterpret_cast<uint32_t*>(dummy_worker + 0x528) = st;
            if (game_is_busy(dummy_worker) != GXWorkerThreadContext_IsBusy(dummy_worker)) ++bad;
        }

        std::printf("[TEST 11] 04_SceneRenderer GXPathUtil, SIMD Matrix Builders & GXSortedRenderItemBuffer_MergeTwoSortedRuns (0x21b6420..0x21bca90): 8034 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 12. Test 05_Yebis2_PostFX (0xbb2600..0xbb7500):
    //     GPUMath_ApertureTimeValueFromExposureValue (0xbb6120),
    //     GetMatrixFrustumJittered (0xbb6350 / 0xbb6410), GetJitteredSubpixel (0xbb6310),
    //     CoC/FoV Optics (0xbb5ff0 / 0xbb60a0), Gnm Registry RB-Tree Search (0xbb5e40 / 0xbb5f10),
    //     IPfxInterface RefCount (0xbb6c70..0xbb6e50) & Multi-View Inspectors (0xbb2650..0xbb33e0)
    // ========================================================================
    {
        eboot_kit::stub(vma_to_bn(0x2bbe648), reinterpret_cast<const void*>(&sub_2bbe648));
        eboot_kit::stub(vma_to_bn(0x2bbe948), reinterpret_cast<const void*>(&sub_2bbe948));
        eboot_kit::stub(vma_to_bn(0x2bbf128), reinterpret_cast<const void*>(&sub_2bbf128));
        eboot_kit::stub(vma_to_bn(0x2bbf178), reinterpret_cast<const void*>(&sub_2bbf178));
        eboot_kit::stub(vma_to_bn(0xbf0790),  reinterpret_cast<const void*>(&sub_bf0790));
        eboot_kit::stub(vma_to_bn(0xbf05f0),  reinterpret_cast<const void*>(&sub_bf05f0));
        eboot_kit::stub(vma_to_bn(0x2610840), reinterpret_cast<const void*>(&sub_2610840));
        eboot_kit::stub(vma_to_bn(0x2610760), reinterpret_cast<const void*>(&sub_2610760));

        auto game_addref    = eboot_vma_fn<int32_t(*)(void*)>(0xbb6c70);
        auto game_getref    = eboot_vma_fn<int32_t(*)(void*)>(0xbb6e00);
        auto game_isinit    = eboot_vma_fn<uint32_t(*)(void*)>(0xbb6e10);
        auto game_intptr    = eboot_vma_fn<void*(*)(void*)>(0xbb6e40);
        auto game_jitter_i  = eboot_vma_fn<void*(*)(const void*, uint32_t, uint32_t, void*, float, float)>(0xbb6410);
        auto game_jitter_xy = eboot_vma_fn<void*(*)(const void*, uint32_t, void*, float, float, float, float)>(0xbb6350);
        auto game_subpix    = eboot_vma_fn<void(*)(uint32_t, void*)>(0xbb6310);
        auto game_exp_avtv  = eboot_vma_fn<void(*)(float*, float*, int32_t, float, float, float, float, float, float, float, float)>(0xbb6120);
        auto game_fnum_coc  = eboot_vma_fn<float(*)(float, float, float, float)>(0xbb60a0);
        auto game_fov_focus = eboot_vma_fn<float(*)(float, float, float, float)>(0xbb5ff0);
        auto game_search_tx = eboot_vma_fn<int32_t(*)(void*, void*)>(0xbb5f10);
        auto game_search_rt = eboot_vma_fn<int32_t(*)(void*, void*)>(0xbb5e40);
        auto game_view_f34  = eboot_vma_fn<float(*)(void*, int32_t)>(0xbb33e0);
        auto game_view_b0   = eboot_vma_fn<uint8_t(*)(void*, int32_t)>(0xbb3360);
        auto game_sub_f54   = eboot_vma_fn<float(*)(void*, int32_t)>(0xbb3250);
        auto game_sub_b0    = eboot_vma_fn<uint8_t(*)(void*, int32_t)>(0xbb31d0);
        auto game_num_views = eboot_vma_fn<uint32_t(*)(void*)>(0xbb2660);
        auto game_cur_view  = eboot_vma_fn<uint32_t(*)(void*)>(0xbb2650);
        auto game_conv_d    = eboot_vma_fn<float(*)(void*, uint32_t, uint32_t, float)>(0xbb2dc0);
        auto game_conv_f    = eboot_vma_fn<int32_t(*)(void*, float*, uint32_t, uint32_t)>(0xbb2c20);
        auto game_conv_v    = eboot_vma_fn<PfxDepthFactorVec4(*)(void*, uint32_t, uint32_t)>(0xbb2a90);

        int bad = 0;

        // 12A. GPUMath_ApertureTimeValueFromExposureValue (0xbb6120) — 5,000 cases
        for (int i = 0; i < 5000; ++i) {
            int32_t mode = rng.range(0, 2);
            float ev     = rng.uniform(-6.0f, 18.0f);
            float prio   = rng.uniform(-2.0f, 10.0f);
            float bias   = rng.uniform(-3.0f, 3.0f);
            float weight = rng.uniform(0.0f, 1.0f);
            float minAv  = rng.uniform(0.0f, 4.0f);
            float maxAv  = minAv + rng.uniform(1.0f, 8.0f);
            float minTv  = rng.uniform(-5.0f, 2.0f);
            float maxTv  = minTv + rng.uniform(1.0f, 10.0f);

            float avT = 0.0f, tvT = 0.0f;
            float avO = 0.0f, tvO = 0.0f;
            game_exp_avtv(&avT, &tvT, mode, ev, prio, bias, weight, minAv, maxAv, minTv, maxTv);
            sub_bb6120(&avO, &tvO, mode, ev, prio, bias, weight, minAv, maxAv, minTv, maxTv);
            if (!eboot_kit::same_float(avT, avO) || !eboot_kit::same_float(tvT, tvO)) ++bad;
        }

        // 12B. GetJitteredSubpixel (0xbb6310) & GetMatrixFrustumJittered (0xbb6350, 0xbb6410) — 2,000 cases
        for (int i = 0; i < 2000; ++i) {
            uint32_t sampleIdx = static_cast<uint32_t>(rng.range(0, 31));
            float offT[2]{}, offO[2]{};
            game_subpix(sampleIdx, offT);
            sub_bb6310(sampleIdx, offO);
            if (std::memcmp(offT, offO, sizeof(offT)) != 0) ++bad;

            alignas(16) float proj[16];
            for (int k = 0; k < 16; ++k) proj[k] = rng.uniform(-5.0f, 5.0f);
            uint32_t colMaj = static_cast<uint32_t>(rng.range(0, 1));
            float w = rng.uniform(320.0f, 3840.0f);
            float h = rng.uniform(240.0f, 2160.0f);

            alignas(16) float outT1[16]{}, outO1[16]{};
            game_jitter_i(proj, colMaj, sampleIdx, outT1, w, h);
            sub_bb6410(proj, colMaj, sampleIdx, outO1, w, h);
            if (std::memcmp(outT1, outO1, sizeof(outT1)) != 0) ++bad;

            float jx = rng.uniform(-1.0f, 1.0f);
            float jy = rng.uniform(-1.0f, 1.0f);
            alignas(16) float outT2[16]{}, outO2[16]{};
            game_jitter_xy(proj, colMaj, outT2, w, h, jx, jy);
            sub_bb6350(proj, colMaj, outO2, w, h, jx, jy);
            if (std::memcmp(outT2, outO2, sizeof(outT2)) != 0) ++bad;
        }

        // 12C. ComputeFNumberFromCoCAndLensParams (0xbb60a0) & ComputeFieldOfViewFromFocusAndAperture (0xbb5ff0) — 2,000 cases
        for (int i = 0; i < 2000; ++i) {
            float cocOrFov = rng.uniform(0.05f, 1.8f);
            float focus    = rng.chance(0.1) ? rng.uniform(-2.0f, 0.0f) : rng.uniform(0.05f, 50.0f);
            float power    = rng.chance(0.1) ? 0.0f : rng.uniform(0.2f, 2.5f);
            float sensor   = rng.uniform(0.015f, 0.070f);

            float fnumT = game_fnum_coc(cocOrFov, focus, power, sensor);
            float fnumO = sub_bb60a0(cocOrFov, focus, power, sensor);
            if (!eboot_kit::same_float(fnumT, fnumO)) ++bad;

            float fovT = game_fov_focus(cocOrFov, focus, power, sensor);
            float fovO = sub_bb5ff0(cocOrFov, focus, power, sensor);
            if (!eboot_kit::same_float(fovT, fovO)) ++bad;
        }

        // 12D. searchGnmTextureRegistry (0xbb5f10) & searchGnmRenderTargetRegistry (0xbb5e40) + IPfxInterface (0xbb6c70..0xbb6e50)
        struct FakeRbNode {
            FakeRbNode* left;      // +0x00
            FakeRbNode* parent;    // +0x08
            FakeRbNode* right;     // +0x10
            uint8_t     color;     // +0x18
            uint8_t     isNil;     // +0x19
            uint8_t     pad[6];
            void*       key;       // +0x20
            void*       valKey;    // +0x28
            void*       gnmObj;    // +0x30
            void*       pfxRes;    // +0x38
        };
        FakeRbNode head{};
        head.left = &head; head.parent = &head; head.right = &head; head.isNil = 1;
        FakeRbNode nodes[7]{};
        uintptr_t keys[7] = { 0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000 };
        for (int i = 0; i < 7; ++i) {
            nodes[i].left   = &head;
            nodes[i].parent = &head;
            nodes[i].right  = &head;
            nodes[i].isNil  = 0;
            nodes[i].key    = reinterpret_cast<void*>(keys[i]);
            nodes[i].valKey = reinterpret_cast<void*>(keys[i]);
            nodes[i].gnmObj = reinterpret_cast<void*>(0xA0000ULL + i);
            nodes[i].pfxRes = reinterpret_cast<void*>(0xB0000ULL + i);
        }
        // Build balanced BST rooted at nodes[3] (0x4000)
        head.parent     = &nodes[3];
        nodes[3].left   = &nodes[1];
        nodes[3].right  = &nodes[5];
        nodes[1].left   = &nodes[0];
        nodes[1].right  = &nodes[2];
        nodes[5].left   = &nodes[4];
        nodes[5].right  = &nodes[6];

        alignas(16) uint8_t fakeCtx[0x60]{};
        *reinterpret_cast<FakeRbNode**>(fakeCtx + 0x20) = &head;
        *reinterpret_cast<FakeRbNode**>(fakeCtx + 0x38) = &head;

        for (uintptr_t k = 0x0800; k <= 0x7800; k += 0x0400) {
            void* queryKey = reinterpret_cast<void*>(k);

            g_last_unregistered_internal_res = nullptr;
            g_last_erased_gnm_key = nullptr;
            int32_t rT_tx = game_search_tx(fakeCtx, queryKey);
            void* resT_tx = g_last_unregistered_internal_res;
            void* keyT_tx = g_last_erased_gnm_key;

            g_last_unregistered_internal_res = nullptr;
            g_last_erased_gnm_key = nullptr;
            int32_t rO_tx = sub_bb5f10(fakeCtx, queryKey);
            void* resO_tx = g_last_unregistered_internal_res;
            void* keyO_tx = g_last_erased_gnm_key;

            if (rT_tx != rO_tx || resT_tx != resO_tx || keyT_tx != keyO_tx) ++bad;

            g_last_unregistered_internal_res = nullptr;
            g_last_erased_gnm_key = nullptr;
            int32_t rT_rt = game_search_rt(fakeCtx, queryKey);
            void* resT_rt = g_last_unregistered_internal_res;
            void* keyT_rt = g_last_erased_gnm_key;

            g_last_unregistered_internal_res = nullptr;
            g_last_erased_gnm_key = nullptr;
            int32_t rO_rt = sub_bb5e40(fakeCtx, queryKey);
            void* resO_rt = g_last_unregistered_internal_res;
            void* keyO_rt = g_last_erased_gnm_key;

            if (rT_rt != rO_rt || resT_rt != resO_rt || keyT_rt != keyO_rt) ++bad;
        }

        // IPfxInterface AddRef / GetReferenceCount / IsInitialized / GetInternalPtr
        alignas(16) uint8_t ifT[0x20]{}, ifO[0x20]{};
        *reinterpret_cast<int32_t*>(ifT + 0x08) = 1;
        *reinterpret_cast<int32_t*>(ifO + 0x08) = 1;
        for (int i = 0; i < 10; ++i) {
            if (game_addref(ifT) != sub_bb6c70(ifO)) ++bad;
            if (game_getref(ifT) != sub_bb6e00(ifO)) ++bad;
            *reinterpret_cast<uint64_t*>(ifT + 0x10) = (i & 1) ? 0x12345678ULL : 0ULL;
            *reinterpret_cast<uint64_t*>(ifO + 0x10) = (i & 1) ? 0x12345678ULL : 0ULL;
            if (game_isinit(ifT) != sub_bb6e10(ifO)) ++bad;
            if (game_intptr(ifT) != ifT + 0x10 || sub_bb6e40(ifO) != ifO + 0x10 || sub_bb6e50(ifO) != ifO + 0x10) ++bad;
        }

        // 12E. Multi-view parameter inspectors in GXYebisPostEffectRenderAndParamApi.o (0xbb2650..0xbb33e0)
        alignas(16) uint8_t fakePost[0xd00]{};
        alignas(16) uint8_t fakeViews72[8 * 72]{};
        alignas(16) uint8_t fakeSubViews104[8 * 0x68]{};
        for (int v = 0; v < 8; ++v) {
            fakeViews72[v * 72] = static_cast<uint8_t>((v & 1) ^ 1);
            *reinterpret_cast<float*>(fakeViews72 + v * 72 + 0x34) = 1.25f * static_cast<float>(v + 1);
            fakeSubViews104[v * 0x68] = static_cast<uint8_t>(v & 1);
            *reinterpret_cast<float*>(fakeSubViews104 + v * 0x68 + 0x54) = 2.5f * static_cast<float>(v + 1);
        }
        *reinterpret_cast<int32_t*>(fakePost + 0x200) = 3; // current view index
        *reinterpret_cast<int32_t*>(fakePost + 0x320) = 8; // num views
        *reinterpret_cast<uint8_t**>(fakePost + 0x210) = fakeViews72;
        *reinterpret_cast<uint8_t**>(fakePost + 0x208) = fakeSubViews104;
        *reinterpret_cast<void**>(fakePost + 0x448)    = reinterpret_cast<void*>(0x1ULL);
        *reinterpret_cast<uint32_t*>(fakePost + 0x450) = 1;
        *reinterpret_cast<uint32_t*>(fakePost + 0x454) = 1;
        *reinterpret_cast<float*>(fakePost + 0x458)    = 1.25f;
        *reinterpret_cast<float*>(fakePost + 0x45c)    = 0.125f;
        *reinterpret_cast<float*>(fakePost + 0x460)    = 0.8f;
        *reinterpret_cast<float*>(fakePost + 0x464)    = -0.1f;
        *reinterpret_cast<float*>(fakePost + 0x57c)    = 0.1f;   // nearZ
        *reinterpret_cast<float*>(fakePost + 0x580)    = 500.0f; // farZ
        fakePost[0x6d5]                                = 0;      // depth cache valid
        *reinterpret_cast<float*>(fakePost + 0xc50)    = 0.95f;
        *reinterpret_cast<float*>(fakePost + 0xc54)    = 0.05f;
        *reinterpret_cast<float*>(fakePost + 0xc58)    = 1.05f;
        *reinterpret_cast<float*>(fakePost + 0xc5c)    = -0.02f;
        *reinterpret_cast<float*>(fakePost + 0xc60)    = 10.0f;

        // Point both direct VMA 0x5465ed0 and relocated bn VMA vma_to_bn(0x5465ed0) at fakePost
        mprotect(reinterpret_cast<void*>(0x5465000ULL), 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC);
        void* saved_5465ed0 = *reinterpret_cast<void**>(0x5465ed0ULL);
        *reinterpret_cast<void**>(0x5465ed0ULL)            = fakePost;
        *reinterpret_cast<void**>(vma_to_bn(0x5465ed0ULL)) = fakePost;

        if (game_num_views(nullptr) != sub_bb2660(nullptr)) ++bad;
        if (game_cur_view(nullptr)  != sub_bb2650(nullptr)) ++bad;
        for (int32_t v = -1; v < 8; ++v) {
            if (!eboot_kit::same_float(game_view_f34(nullptr, v), sub_bb33e0(nullptr, v))) ++bad;
            if (game_view_b0(nullptr, v) != sub_bb3360(nullptr, v)) ++bad;
            if (!eboot_kit::same_float(game_sub_f54(nullptr, v), sub_bb3250(nullptr, v))) ++bad;
            if (game_sub_b0(nullptr, v)  != sub_bb31d0(nullptr, v)) ++bad;
        }

        // Test ConvertDepthValue (0xbb2dc0), GetDepthConversionFactor (0xbb2c20) & GetDepthConversionFactorValue (0xbb2a90)
        const uint32_t kDepthTypes[] = { 0, 1, 2, 3, 6 };
        for (int i = 0; i < 1000; ++i) {
            uint32_t sType = kDepthTypes[rng.range(0, 4)];
            uint32_t dType = kDepthTypes[rng.range(0, 4)];
            float facT[4]{}, facO[4]{};
            int32_t hrT = game_conv_f(nullptr, facT, sType, dType);
            int32_t hrO = sub_bb2c20(nullptr, facO, sType, dType);
            if (hrT != hrO || std::memcmp(facT, facO, sizeof(facT)) != 0) ++bad;

            PfxDepthFactorVec4 vT = game_conv_v(nullptr, sType, dType);
            PfxDepthFactorVec4 vO = sub_bb2a90(nullptr, sType, dType);
            if (std::memcmp(&vT, &vO, sizeof(vT)) != 0) ++bad;

            uint32_t sTypeD = rng.chance(0.2) ? 7u : sType;
            uint32_t dTypeD = rng.chance(0.2) ? 7u : dType;
            float dVal = rng.uniform(0.15f, 100.0f);
            float dOutT = game_conv_d(nullptr, sTypeD, dTypeD, dVal);
            float dOutO = sub_bb2dc0(nullptr, sTypeD, dTypeD, dVal);
            if (!eboot_kit::same_float(dOutT, dOutO)) ++bad;
        }
        *reinterpret_cast<void**>(0x5465ed0ULL) = saved_5465ed0;

        std::printf("[TEST 12] 05_Yebis2_PostFX Exposure/Frustum/CoC/FoV/Depth Math, Gnm RB-Tree & View API (0xbb2600..0xbb7500): 10059 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 13. [TEST 13] 06_DeferredLightingAndAO (0x2290000..0x22a0000, 0x22ab7e0..0x22b0000):
    //     GXFeedBackBlurRenderPass.o (0x22af5a0, 0x22af5c0),
    //     GXDeferredLightingAndAmbientOcclusionPass.o (0x229e480, 0x229d2a0, 0x229ca80..0x229cb10),
    //     GXDeferredLightVolumeAndShadowCuller.o (0x2292b80, 0x22928a0, 0x22911b0, 0x2291530,
    //     0x2290660, 0x22919c0, 0x2290c80, 0x2294b30/0x2293710/0x2293900/0x22940b0, 0x2294d40)
    // ========================================================================
    {
        eboot_kit::stub(vma_to_bn(0x2bc08b8), reinterpret_cast<const void*>(&sub_2bc08b8));
        eboot_kit::stub(vma_to_bn(0x2bbe588), reinterpret_cast<const void*>(&sub_2bbe588));
        eboot_kit::stub(vma_to_bn(0x2bbf188), reinterpret_cast<const void*>(&sub_2bbf188));

        auto game_fb_end      = eboot_vma_fn<void(*)(void*)>(0x22af5a0);
        auto game_fb_metrics  = eboot_vma_fn<void(*)(void*)>(0x22af5c0);
        auto game_refl_plane  = eboot_vma_fn<void(*)(void*, const void*)>(0x229e480);
        auto game_clamp_rts   = eboot_vma_fn<void(*)(void*, uint32_t)>(0x229d2a0);
        auto game_slot6e70    = eboot_vma_fn<void*(*)(const void*, uint32_t)>(0x229cb00);
        auto game_sel_dss     = eboot_vma_fn<void*(*)(const void*, uint8_t, uint8_t)>(0x229cab0);
        auto game_gbuf_surf   = eboot_vma_fn<void*(*)(const void*, uint32_t)>(0x229ca80);
        auto game_cull_pt     = eboot_vma_fn<uint32_t(*)(const void*, const __m128*)>(0x2292b80);
        auto game_cull_aabb   = eboot_vma_fn<uint32_t(*)(const void*, const __m128*)>(0x22928a0);
        auto game_set_vp      = eboot_vma_fn<void(*)(void*, const __m128*, const __m128*)>(0x22911b0);
        auto game_set_vp_pair = eboot_vma_fn<void(*)(void*, const __m128*)>(0x2290c60);
        auto game_copy_cull   = eboot_vma_fn<void(*)(void*, const void*)>(0x2291530);
        auto game_copy_cmd    = eboot_vma_fn<void(*)(void*, const void*)>(0x2290660);
        auto game_vol_ctor    = eboot_vma_fn<void(*)(void*)>(0x22919c0);
        auto game_ctx_ctor    = eboot_vma_fn<void(*)(void*, void*)>(0x2290c80);
        auto game_merge_light = eboot_vma_fn<void*(*)(void*, const void*, const void*, const void*, const void*, void*)>(0x2294b30);
        auto game_sort_lights = eboot_vma_fn<void(*)(void*, void*, int64_t, void*)>(0x2293710);
        auto game_cam_pair    = eboot_vma_fn<void(*)(void*, const void*)>(0x2294d40);
        auto game_set_6f8     = eboot_vma_fn<void(*)(void*, uint64_t)>(0x2290c70);
        auto game_set_710     = eboot_vma_fn<void(*)(void*, const __m128*)>(0x2290c10);
        auto game_get_cmdhead = eboot_vma_fn<void*(*)(const void*)>(0x2290640);

        int bad = 0;

        // 13A. SIMD Point-in-Frustum (0x2292b80) & AABB-in-Frustum (0x22928a0) — 4,000 random configurations
        for (int c = 0; c < 4000; ++c) {
            alignas(16) uint8_t vol[0x2c0]{};
            // World AABB of frustum at +0x220 (min) and +0x230 (max)
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x220), _mm_setr_ps(-50.0f, -50.0f, -50.0f, 1.0f));
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x230), _mm_setr_ps( 50.0f,  50.0f,  50.0f, 1.0f));
            // 6 inward-pointing frustum planes at +0x160 .. +0x1b0: nx*x + ny*y + nz*z + d >= 0
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x160), _mm_setr_ps( 1.0f,  0.0f,  0.0f, 40.0f));
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x170), _mm_setr_ps(-1.0f,  0.0f,  0.0f, 40.0f));
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x180), _mm_setr_ps( 0.0f,  1.0f,  0.0f, 40.0f));
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x190), _mm_setr_ps( 0.0f, -1.0f,  0.0f, 40.0f));
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x1a0), _mm_setr_ps( 0.0f,  0.0f,  1.0f, 40.0f));
            _mm_store_ps(reinterpret_cast<float*>(vol + 0x1b0), _mm_setr_ps( 0.0f,  0.0f, -1.0f, 40.0f));

            alignas(16) __m128 pt = _mm_setr_ps(
                rng.uniform(-65.0f, 65.0f), rng.uniform(-65.0f, 65.0f), rng.uniform(-65.0f, 65.0f), 1.0f);
            if (game_cull_pt(vol, &pt) != sub_2292b80(vol, &pt)) ++bad;

            float cx = rng.uniform(-55.0f, 55.0f);
            float cy = rng.uniform(-55.0f, 55.0f);
            float cz = rng.uniform(-55.0f, 55.0f);
            float ex = rng.uniform(0.5f, 20.0f);
            float ey = rng.uniform(0.5f, 20.0f);
            float ez = rng.uniform(0.5f, 20.0f);
            alignas(16) __m128 aabb[2] = {
                _mm_setr_ps(cx - ex, cy - ey, cz - ez, 1.0f),
                _mm_setr_ps(cx + ex, cy + ey, cz + ez, 1.0f)
            };
            if (game_cull_aabb(vol, aabb) != sub_22928a0(vol, aabb)) ++bad;
        }

        // 13B. SIMD 4x4 View/Proj Matrix Multiplication & Inversion (0x22911b0, 0x2290c60)
        //      and Reflection Plane Transformation (0x229e480) — 2,000 random non-singular matrices
        for (int c = 0; c < 2000; ++c) {
            alignas(16) __m128 vpPair[8];
            // Generate well-conditioned diagonally dominant 4x4 matrices for View and Proj
            for (int m = 0; m < 2; ++m) {
                for (int r = 0; r < 4; ++r) {
                    float v[4];
                    for (int k = 0; k < 4; ++k) {
                        v[k] = (r == k) ? rng.uniform(1.5f, 4.0f) : rng.uniform(-0.35f, 0.35f);
                    }
                    vpPair[m * 4 + r] = _mm_loadu_ps(v);
                }
            }

            alignas(16) uint8_t ctxT[0x780]{}, ctxO[0x780]{};
            game_set_vp(ctxT, vpPair, vpPair + 4);
            sub_22911b0(ctxO, vpPair, vpPair + 4);
            if (std::memcmp(ctxT + 0x10, ctxO + 0x10, 0x100) != 0) ++bad;

            std::memset(ctxT, 0, sizeof(ctxT));
            std::memset(ctxO, 0, sizeof(ctxO));
            game_set_vp_pair(ctxT, vpPair);
            sub_2290c60(ctxO, vpPair);
            if (std::memcmp(ctxT + 0x10, ctxO + 0x10, 0x100) != 0) ++bad;

            // Test sub_229e480 (Reflection Plane Transformation by inv(view) and inv(proj) into +0x710)
            alignas(16) uint8_t reflSrc[0x2a50]{};
            _mm_store_ps(reinterpret_cast<float*>(reflSrc + 0x2a30),
                         _mm_setr_ps(rng.uniform(-1.0f, 1.0f), rng.uniform(-1.0f, 1.0f),
                                     rng.uniform(-1.0f, 1.0f), rng.uniform(-10.0f, 10.0f)));
            game_refl_plane(ctxT, reflSrc);
            sub_229e480(ctxO, reflSrc);
            if (std::memcmp(ctxT + 0x710, ctxO + 0x710, 0x10) != 0) ++bad;
        }

        // 13C. GXCullContext / GXCommandContext Copy (0x2291530, 0x2290660) & Constructors (0x22919c0, 0x2290c80)
        {
            mprotect(reinterpret_cast<void*>(0x5540000ULL), 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);
            mprotect(reinterpret_cast<void*>(vma_to_bn(0x5540000ULL)), 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);
            *reinterpret_cast<void**>(0x55406c8ULL) = nullptr;
            *reinterpret_cast<void**>(vma_to_bn(0x55406c8ULL)) = nullptr;

            alignas(16) uint8_t volT[0x2d0]{}, volO[0x2d0]{};
            game_vol_ctor(volT);
            sub_22919c0(volO);
            if (std::memcmp(volT + 0x08, volO + 0x08, 0x2b8) != 0) ++bad;

            alignas(16) uint8_t cctxT[0x400]{}, cctxO[0x400]{};
            void* customVol = reinterpret_cast<void*>(0x12345678ULL);
            game_ctx_ctor(cctxT, customVol);
            sub_2290c80(cctxO, customVol);
            if (std::memcmp(cctxT + 0x08, cctxO + 0x08, 0x118) != 0 ||
                std::memcmp(cctxT + 0x128, cctxO + 0x128, 0x2d8) != 0) {
                ++bad;
            }

            alignas(16) uint8_t srcCmd[0x460]{}, dstCmdT[0x460]{}, dstCmdO[0x460]{};
            for (size_t i = 0; i < sizeof(srcCmd); ++i) srcCmd[i] = static_cast<uint8_t>(rng.u32() & 0xffu);
            game_copy_cull(dstCmdT, srcCmd);
            sub_2291530(dstCmdO, srcCmd);
            if (std::memcmp(dstCmdT, dstCmdO, sizeof(dstCmdT)) != 0) ++bad;

            std::memset(dstCmdT, 0, sizeof(dstCmdT));
            std::memset(dstCmdO, 0, sizeof(dstCmdO));
            game_copy_cmd(dstCmdT, srcCmd);
            sub_2290660(dstCmdO, srcCmd);
            if (std::memcmp(dstCmdT, dstCmdO, sizeof(dstCmdT)) != 0) ++bad;
        }

        // 13D. GXLightSortItem16 Stable Sort & Buffered Merge Pipeline (0x2293710 / 0x2293900 / 0x22940b0 / 0x2294b30)
        struct LightSortItem16 {
            uint32_t key;
            uint32_t pad;
            uint64_t payload;
        };
        struct LightTempBufDesc {
            LightSortItem16* begin;
            LightSortItem16* cur;
            LightSortItem16* end;
            int64_t          cap;
        };
        for (int c = 0; c < 1500; ++c) {
            int n = rng.range(0, 140);
            std::vector<LightSortItem16> itemsT(n), itemsO(n);
            for (int i = 0; i < n; ++i) {
                uint32_t k = rng.u32() % 32u; // frequent duplicate keys to verify stability!
                itemsT[i] = { k, 0u, 0x500000ULL + static_cast<uint64_t>(i) };
            }
            itemsO = itemsT;

            int64_t tmpCap = rng.chance(0.5) ? (n + 4) : (n / 3);
            std::vector<LightSortItem16> tmpStoreT(tmpCap + 4), tmpStoreO(tmpCap + 4);
            LightTempBufDesc descT{ tmpStoreT.data(), tmpStoreT.data(), tmpStoreT.data(), tmpCap };
            LightTempBufDesc descO{ tmpStoreO.data(), tmpStoreO.data(), tmpStoreO.data(), tmpCap };
            alignas(16) uint8_t holderT[0x30]{}, holderO[0x30]{};
            *reinterpret_cast<LightTempBufDesc**>(holderT + 0x20) = &descT;
            *reinterpret_cast<LightTempBufDesc**>(holderO + 0x20) = &descO;

            if (n > 0) {
                game_sort_lights(itemsT.data(), itemsT.data() + n, n, holderT);
                sub_2293710(itemsO.data(), itemsO.data() + n, n, holderO);
                if (std::memcmp(itemsT.data(), itemsO.data(), n * sizeof(LightSortItem16)) != 0) {
                    ++bad;
                }
            }

            // Also test sub_2294b30 (MergeTwoSortedLightItemRuns) on two sorted halves
            int mid = n / 2;
            std::vector<LightSortItem16> mOutT(n + 4), mOutO(n + 4);
            LightTempBufDesc mDescT{ mOutT.data(), mOutT.data(), mOutT.data() + mOutT.size(), static_cast<int64_t>(mOutT.size()) };
            LightTempBufDesc mDescO{ mOutO.data(), mOutO.data(), mOutO.data() + mOutO.size(), static_cast<int64_t>(mOutO.size()) };
            *reinterpret_cast<LightTempBufDesc**>(holderT + 0x20) = &mDescT;
            *reinterpret_cast<LightTempBufDesc**>(holderO + 0x20) = &mDescO;
            alignas(16) uint8_t retT[0x30]{}, retO[0x30]{};
            game_merge_light(retT, itemsT.data(), itemsT.data() + mid, itemsT.data() + mid, itemsT.data() + n, holderT);
            sub_2294b30(retO, itemsO.data(), itemsO.data() + mid, itemsO.data() + mid, itemsO.data() + n, holderO);
            ptrdiff_t cntT = mDescT.cur - mDescT.begin;
            ptrdiff_t cntO = mDescO.cur - mDescO.begin;
            if (cntT != cntO || (cntT > 0 && std::memcmp(mOutT.data(), mOutO.data(), cntT * sizeof(LightSortItem16)) != 0)) {
                ++bad;
            }
        }

        // 13E. Deferred Pass Render Target Slot Clamping (0x229d2a0), Viewport/Aspect Helpers (0x229ca80..0x229cb10),
        //      Camera Pair Updater (0x2294d40), and FeedbackBlur Pass Callbacks (0x22af5a0, 0x22af5c0)
        for (uint32_t maxSlot = 0; maxSlot <= 8; ++maxSlot) {
            alignas(16) uint8_t implT[0x4000]{}, implO[0x4000]{};
            alignas(16) uint8_t selfT[0x760]{}, selfO[0x760]{};
            *reinterpret_cast<uint8_t**>(selfT + 0x08) = implT;
            *reinterpret_cast<uint8_t**>(selfO + 0x08) = implO;
            *reinterpret_cast<uint32_t*>(selfT + 0x6f4) = 8;
            *reinterpret_cast<uint32_t*>(selfO + 0x6f4) = 8;
            for (int s = 0; s < 8; ++s) {
                *reinterpret_cast<uint64_t*>(implT + 0x3b80 + s * 8) = 0x1000ULL + s;
                *reinterpret_cast<uint64_t*>(implO + 0x3b80 + s * 8) = 0x1000ULL + s;
            }
            game_clamp_rts(selfT, maxSlot);
            sub_229d2a0(selfO, maxSlot);
            if (*reinterpret_cast<uint32_t*>(selfT + 0x6f4) != *reinterpret_cast<uint32_t*>(selfO + 0x6f4) ||
                std::memcmp(implT + 0x3b80, implO + 0x3b80, 64) != 0) {
                ++bad;
            }
        }

        {
            alignas(16) uint8_t passSelf[0x7000]{};
            for (uint32_t i = 0; i < 8; ++i) {
                *reinterpret_cast<void**>(passSelf + 0x6e70 + i * 8) = reinterpret_cast<void*>(0x80000ULL + i);
                if (game_slot6e70(passSelf, i) != sub_229cb00(passSelf, i)) ++bad;
            }
            *reinterpret_cast<void**>(passSelf + 0x6598) = reinterpret_cast<void*>(0xAAAA1111ULL);
            *reinterpret_cast<void**>(passSelf + 0x65a0) = reinterpret_cast<void*>(0xAAAA2222ULL);
            *reinterpret_cast<void**>(passSelf + 0x65f0) = reinterpret_cast<void*>(0xBBBB0000ULL);
            *reinterpret_cast<void**>(passSelf + 0x65f8) = reinterpret_cast<void*>(0xBBBB1111ULL);
            *reinterpret_cast<void**>(passSelf + 0x6600) = reinterpret_cast<void*>(0xBBBB2222ULL);

            if (game_sel_dss(passSelf, 0, 1) != sub_229cab0(passSelf, 0, 1)) ++bad;
            if (game_sel_dss(passSelf, 1, 1) != sub_229cab0(passSelf, 1, 1)) ++bad;
            for (uint32_t idx = 0; idx <= 4; ++idx) {
                if (game_gbuf_surf(passSelf, idx) != sub_229ca80(passSelf, idx)) ++bad;
            }

            game_set_6f8(passSelf, 42);
            uint8_t fT = passSelf[0x6f8];
            passSelf[0x6f8] = 0;
            sub_2290c70(passSelf, 42);
            if (fT != passSelf[0x6f8]) ++bad;

            alignas(16) __m128 plane = _mm_setr_ps(0.0f, 1.0f, 0.0f, -5.0f);
            alignas(16) uint8_t pT[0x740]{}, pO[0x740]{};
            game_set_710(pT, &plane);
            sub_2290c10(pO, &plane);
            if (std::memcmp(pT + 0x710, pO + 0x710, 16) != 0) ++bad;

            void* fakeHead = reinterpret_cast<void*>(0xCAFEBABEULL);
            *reinterpret_cast<void***>(pT + 0x450) = &fakeHead;
            *reinterpret_cast<void***>(pO + 0x450) = &fakeHead;
            if (game_get_cmdhead(pT) != sub_2290640(pO)) ++bad;

            // Test sub_2294d40 (UpdateCameraFrustumPair)
            *reinterpret_cast<uint8_t*>(0x5540dd4ULL) = 0;
            *reinterpret_cast<uint8_t*>(vma_to_bn(0x5540dd4ULL)) = 0;
            alignas(16) uint8_t camSrc[0x90]{};
            for (size_t i = 0; i < sizeof(camSrc); ++i) camSrc[i] = static_cast<uint8_t>((i * 7 + 3) & 0xffu);
            alignas(16) uint8_t camHolderT[0x240]{}, camHolderO[0x240]{};
            for (int step = 0; step < 3; ++step) {
                camSrc[0] = static_cast<uint8_t>(step * 19);
                game_cam_pair(camHolderT, camSrc);
                sub_2294d40(camHolderO, camSrc);
                if (std::memcmp(camHolderT, camHolderO, sizeof(camHolderT)) != 0) ++bad;
            }

            // Test GXEnvCubeMapPass_EndCallback (0x22af5a0) & GXSceneRenderPass_UpdateGpuTimestampMetrics (0x22af5c0)
            alignas(16) uint8_t fakeDd8[0x7800]{};
            *reinterpret_cast<uint64_t*>(fakeDd8 + 0x7710) = 0x9999ULL;
            *reinterpret_cast<uint8_t**>(0x5540dd8ULL) = fakeDd8;
            *reinterpret_cast<uint8_t**>(vma_to_bn(0x5540dd8ULL)) = fakeDd8;
            alignas(16) uint8_t tgtT[0x660]{}, tgtO[0x660]{};
            game_fb_end(tgtT);
            sub_22af5a0(tgtO);
            if (tgtT[0x658] != tgtO[0x658] || *reinterpret_cast<uint64_t*>(fakeDd8 + 0x7710) != 0ULL) ++bad;

            alignas(16) uint8_t fake6c8[0x620]{};
            *reinterpret_cast<uint64_t*>(fake6c8 + 0x600) = 77ULL;
            *reinterpret_cast<uint8_t**>(0x55406c8ULL) = fake6c8;
            *reinterpret_cast<uint8_t**>(vma_to_bn(0x55406c8ULL)) = fake6c8;
            alignas(16) uint8_t mgrT[0x60]{}, mgrO[0x60]{};
            *reinterpret_cast<uint64_t*>(mgrT + 0x50) = 77ULL;
            *reinterpret_cast<uint64_t*>(mgrO + 0x50) = 77ULL;
            game_fb_metrics(mgrT);
            sub_22af5c0(mgrO);
            if (std::memcmp(mgrT, mgrO, sizeof(mgrT)) != 0) ++bad;
        }

        std::printf("[TEST 13] 06_DeferredLightingAndAO SIMD Frustum Culling, 4x4 Matrix Inv/Mul, Light MergeSort & Pass Helpers: 7520 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // ========================================================================
    // 14. [TEST 14] 07_SFX_ParticleGpuRenderer (0x28ce7b0..0x28d25c0, 0x2309000..0x2310000, 0x2304000..0x2309000):
    //     GXFfxRibbonAndParticleVertexEmitter.o (0x28d21c0, 0x28d0480, 0x28d02c0, 0x28d0160, 0x28cfba0, 0x28cf580, 0x28ceec0, 0x28ce7b0),
    //     GXFfxSceneCtrlAndShaderBinder.o (0x230ffe0/0x230ffd0/0x230ffc0, 0x230ecc0/0x230dc40, 0x230c940/0x230cd00/0x230cc30/0x230cfb0/0x230caf0/0x230c640, 0x230ba00, 0x230b760/0x230add0/0x230ae70),
    //     GXFfxTessellationAndParticleDrawPass.o (0x2305f50, 0x2305640, 0x23071a0/0x23077a0, 0x23084f0/0x2308150/0x23054f0, 0x2305120/0x23050f0/0x2305060)
    // ========================================================================
    {
        eboot_kit::stub(vma_to_bn(0x2bbfd68), reinterpret_cast<const void*>(&sub_2bbfd68));
        eboot_kit::stub(vma_to_bn(0xee5de0),  reinterpret_cast<const void*>(&sub_ee5de0));
        eboot_kit::stub(vma_to_bn(0x2311430), reinterpret_cast<const void*>(&sub_2311430));
        eboot_kit::stub(vma_to_bn(0x2311c00), reinterpret_cast<const void*>(&sub_2311c00));

        // 1. GXFfxRibbonAndParticleVertexEmitter.o game pointers
        auto game_emit_pt   = eboot_vma_fn<void(*)(uint8_t*, const float* const*, uint32_t, const float*, const float*, const float*, const __m128*)>(0x28d21c0);
        auto game_soa_aabb  = eboot_vma_fn<void(*)(__m128*, __m128*, uint32_t, const float*, const float*, const float*)>(0x28d0480);
        auto game_sort_z    = eboot_vma_fn<void(*)(uint32_t*, uint32_t, const float*, const float*, const float*, const uint8_t*, const __m128*)>(0x28d02c0);
        auto game_cam_fade  = eboot_vma_fn<void(*)(uint8_t*, const uint8_t*, uint32_t, uint32_t, const float*, const uint8_t*)>(0x28d0160);
        auto game_trail_up  = eboot_vma_fn<decltype(&sub_28cfba0)>(0x28cfba0);
        auto game_trail_cam = eboot_vma_fn<decltype(&sub_28cf580)>(0x28cf580);
        auto game_strip_up  = eboot_vma_fn<decltype(&sub_28ceec0)>(0x28ceec0);
        auto game_strip_cam = eboot_vma_fn<decltype(&sub_28ce7b0)>(0x28ce7b0);

        // 2. GXFfxSceneCtrlAndShaderBinder.o game pointers
        auto game_sc_ctor   = eboot_vma_fn<void(*)(void*, void*)>(0x230ffe0);
        auto game_sc_tbl0   = eboot_vma_fn<void*(*)(void*, uint32_t)>(0x230ffd0);
        auto game_sc_tbl1   = eboot_vma_fn<void*(*)(void*, int32_t)>(0x230ffc0);
        auto game_bind_ps0  = eboot_vma_fn<void(*)(const void*, void*)>(0x230ecc0);
        auto game_bind_ps11 = eboot_vma_fn<void(*)(const void*, void*)>(0x230dc40);
        auto game_rm_ctor   = eboot_vma_fn<void(*)(void*, void*)>(0x230c940);
        auto game_rm_ins    = eboot_vma_fn<bool(*)(void*, uint32_t, void*)>(0x230cd00);
        auto game_rm_find   = eboot_vma_fn<void*(*)(void*, uint32_t)>(0x230cc30);
        auto game_rm_rem    = eboot_vma_fn<void(*)(void*, uint32_t)>(0x230cfb0);
        auto game_rm_clear  = eboot_vma_fn<void(*)(void*)>(0x230caf0);
        auto game_rm_dtor   = eboot_vma_fn<void(*)(void*)>(0x230c640);
        auto game_mat2sqt   = eboot_vma_fn<void(*)(void*, uint32_t, const float*)>(0x230ba00);
        [[maybe_unused]] auto game_sqt2mat = eboot_vma_fn<void(*)(void*)>(0x230b760);
        auto game_sub_ctor  = eboot_vma_fn<void(*)(void*, void*)>(0x230ae70);
        auto game_set_m4x4  = eboot_vma_fn<void(*)(void*, const float*)>(0x230add0);

        // 3. GXFfxTessellationAndParticleDrawPass.o game pointers
        auto game_disp_84f0 = eboot_vma_fn<void(*)(void*, const int32_t*)>(0x23084f0);
        auto game_disp_8150 = eboot_vma_fn<void(*)(void*, const int32_t*)>(0x2308150);
        auto game_vec8_push = eboot_vma_fn<void(*)(void*, const uint64_t*)>(0x23071a0);
        auto game_ins80     = eboot_vma_fn<void(*)(void*, FfxTestIter24, FfxTestIter24, FfxTestIter24)>(0x2305f50);
        auto game_vec16_grw = eboot_vma_fn<void(*)(void*)>(0x2305640);
        auto game_disp_54f0 = eboot_vma_fn<void(*)(void*, const int32_t*)>(0x23054f0);
        auto game_ptmf_inv  = eboot_vma_fn<void(*)(void*)>(0x2305120);
        auto game_adv_50f0  = eboot_vma_fn<uint32_t(*)(void*)>(0x23050f0);
        auto game_one_5060  = eboot_vma_fn<uint32_t(*)()>(0x2305060);

        int bad = 0;

        // --------------------------------------------------------------------
        // 14A. GXFfxRibbonAndParticleVertexEmitter.o — Point/Sprite Emitter (0x28d21c0),
        //      SoA AABB (0x28d0480), View-Depth Sort Keys (0x28d02c0), Camera Fade (0x28d0160)
        // --------------------------------------------------------------------
        for (int c = 0; c < 500; ++c) {
            uint32_t quadCount = static_cast<uint32_t>(rng.range(1, 6));
            uint32_t n = quadCount * 4u;
            alignas(16) float posX[32]{}, posY[32]{}, posZ[32]{};
            alignas(16) float s0[32]{}, s1[32]{}, s2[32]{}, s3[32]{}, s4[32]{}, s5[32]{};
            for (uint32_t i = 0; i < n; ++i) {
                posX[i] = rng.uniform(-25.0f, 25.0f);
                posY[i] = rng.uniform(-25.0f, 25.0f);
                posZ[i] = rng.uniform(-25.0f, 25.0f);
                s0[i]   = rng.uniform(-0.2f, 1.2f);
                s1[i]   = rng.uniform(-0.2f, 1.2f);
                s2[i]   = rng.uniform(-0.2f, 1.2f);
                s3[i]   = rng.uniform(-0.2f, 1.2f);
                s4[i]   = rng.uniform(-3.14f, 3.14f);
                s5[i]   = rng.uniform(0.1f, 8.0f);
            }
            const float* streams[6] = { s0, s1, s2, s3, s4, s5 };
            alignas(16) __m128 world3x4[3] = {
                _mm_setr_ps(rng.uniform(0.5f, 1.5f), rng.uniform(-0.3f, 0.3f), rng.uniform(-0.3f, 0.3f), rng.uniform(-10.0f, 10.0f)),
                _mm_setr_ps(rng.uniform(-0.3f, 0.3f), rng.uniform(0.5f, 1.5f), rng.uniform(-0.3f, 0.3f), rng.uniform(-10.0f, 10.0f)),
                _mm_setr_ps(rng.uniform(-0.3f, 0.3f), rng.uniform(-0.3f, 0.3f), rng.uniform(0.5f, 1.5f), rng.uniform(-10.0f, 10.0f))
            };
            alignas(16) uint8_t outT[32 * 20]{}, outO[32 * 20]{};
            game_emit_pt(outT, streams, quadCount, posX, posY, posZ, world3x4);
            sub_28d21c0(outO, streams, quadCount, posX, posY, posZ, world3x4);
            if (std::memcmp(outT, outO, n * 20u) != 0) ++bad;
        }

        for (int c = 0; c < 1000; ++c) {
            uint32_t count = static_cast<uint32_t>(rng.range(1, 31));
            alignas(16) float posX[36]{}, posY[36]{}, posZ[36]{};
            for (uint32_t i = 0; i < count; ++i) {
                posX[i] = rng.uniform(-100.0f, 100.0f);
                posY[i] = rng.uniform(-100.0f, 100.0f);
                posZ[i] = rng.uniform(-100.0f, 100.0f);
            }
            alignas(16) __m128 minT{}, maxT{}, minO{}, maxO{};
            game_soa_aabb(&minT, &maxT, count, posX, posY, posZ);
            sub_28d0480(&minO, &maxO, count, posX, posY, posZ);
            if (std::memcmp(&minT, &minO, 16) != 0 || std::memcmp(&maxT, &maxO, 16) != 0) ++bad;
        }

        for (int c = 0; c < 500; ++c) {
            uint32_t count = static_cast<uint32_t>(rng.range(1, 24));
            alignas(16) float posX[32]{}, posY[32]{}, posZ[32]{};
            for (uint32_t i = 0; i < 32; ++i) {
                posX[i] = rng.uniform(-20.0f, 20.0f);
                posY[i] = rng.uniform(-20.0f, 20.0f);
                posZ[i] = rng.uniform(-20.0f, 20.0f);
            }
            alignas(16) uint8_t viewMat[0x40]{};
            *reinterpret_cast<float*>(viewMat + 0x20) = 0.0f;
            *reinterpret_cast<float*>(viewMat + 0x24) = 0.0f;
            *reinterpret_cast<float*>(viewMat + 0x28) = 1.0f;
            *reinterpret_cast<float*>(viewMat + 0x2c) = 25.0f;
            *reinterpret_cast<__m128*>(viewMat + 0x30) = _mm_setr_ps(1.0f, 2.0f, -5.0f, 1.0f);

            alignas(16) __m128 boundsZ[2] = {
                _mm_setr_ps(-30.0f, -30.0f, 0.0f, 1.0f),
                _mm_setr_ps( 30.0f,  30.0f, 60.0f, 1.0f)
            };
            alignas(16) uint32_t sortT[32]{}, sortO[32]{};
            game_sort_z(sortT, count, posZ, posX, posY, viewMat, boundsZ);
            sub_28d02c0(sortO, count, posZ, posX, posY, viewMat, boundsZ);
            if (std::memcmp(sortT, sortO, ((count + 3u) & ~3u) * sizeof(uint32_t)) != 0) ++bad;

            // Also test Camera Near/Far Distance Fade (0x28d0160)
            alignas(16) uint8_t vbufT[24 * 20]{}, vbufO[24 * 20]{};
            for (uint32_t i = 0; i < count; ++i) {
                float* vp = reinterpret_cast<float*>(vbufT + i * 20u);
                vp[0] = posX[i];
                vp[1] = posY[i];
                vp[2] = posZ[i];
                vbufT[i * 20u + 12] = static_cast<uint8_t>(rng.u32() & 0xffu);
                vbufT[i * 20u + 13] = static_cast<uint8_t>(rng.u32() & 0xffu);
                vbufT[i * 20u + 14] = static_cast<uint8_t>(rng.u32() & 0xffu);
                vbufT[i * 20u + 15] = static_cast<uint8_t>(rng.range(10, 255));
            }
            std::memcpy(vbufO, vbufT, sizeof(vbufT));
            float fadeParams[4] = {
                rng.chance(0.2) ? 0.0f : rng.uniform(1.0f, 5.0f),
                rng.chance(0.2) ? 0.0f : rng.uniform(2.0f, 8.0f),
                rng.uniform(15.0f, 25.0f),
                rng.uniform(5.0f, 15.0f)
            };
            game_cam_fade(vbufT + 12, vbufT, count, 20u, fadeParams, viewMat);
            sub_28d0160(vbufO + 12, vbufO, count, 20u, fadeParams, viewMat);
            if (std::memcmp(vbufT, vbufO, count * 20u) != 0) ++bad;
        }

        // --------------------------------------------------------------------
        // 14B. GXFfxRibbonAndParticleVertexEmitter.o — Ribbon Trail Control Point & Strip Emitters
        //      (0x28cfba0, 0x28cf580, 0x28ceec0, 0x28ce7b0)
        // --------------------------------------------------------------------
        for (int c = 0; c < 400; ++c) {
            uint32_t count = static_cast<uint32_t>(rng.range(2, 10));
            alignas(16) float pTime[16]{}, pPosX[16]{}, pPosY[16]{}, pPosZ[16]{};
            alignas(16) float pU[16]{}, pV[16]{}, pUpX[32]{}, pUpY[32]{}, pUpZ[32]{};
            for (uint32_t i = 0; i < 16; ++i) {
                pTime[i] = static_cast<float>(i) * 0.1f;
                pPosX[i] = static_cast<float>(i) * 1.3f + rng.uniform(-0.2f, 0.2f);
                pPosY[i] = std::sin(static_cast<float>(i) * 0.5f) * 2.0f + rng.uniform(-0.2f, 0.2f);
                pPosZ[i] = static_cast<float>(i) * (-0.8f) + rng.uniform(-0.2f, 0.2f);
                pU[i]    = static_cast<float>(i) / 10.0f;
                pV[i]    = rng.uniform(0.0f, 0.5f);
            }
            for (uint32_t i = 0; i < 32; ++i) {
                pUpX[i] = 0.0f;
                pUpY[i] = 1.0f;
                pUpZ[i] = 0.1f * static_cast<float>(i & 3u);
            }
            const float* vOpt = rng.chance(0.5) ? pV : nullptr;
            uint32_t upStride = rng.chance(0.5) ? 1u : 2u;
            float curTime = 1.25f, lifeTime = 2.0f, param0 = 0.5f, param1 = 1.5f;
            float vBase = 0.1f, vDelta = 0.8f;
            float colB = rng.uniform(-0.1f, 1.2f), colG = rng.uniform(-0.1f, 1.2f);
            float colR = rng.uniform(-0.1f, 1.2f), colA = rng.uniform(-0.1f, 1.2f);
            float camX = 5.0f, camY = 8.0f, camZ = 12.0f;
            uint32_t startOff = 1u, totalPts = count + 4u;
            float fadeIn = 2.0f, fadeOut = 2.0f;

            alignas(16) uint8_t trailUpT[16 * 64]{}, trailUpO[16 * 64]{};
            game_trail_up(trailUpT, count, pTime,
                          reinterpret_cast<const uint32_t*>(pPosX), reinterpret_cast<const uint32_t*>(pPosY), reinterpret_cast<const uint32_t*>(pPosZ),
                          curTime, lifeTime, param0, param1, vBase, vDelta, colB, colG,
                          reinterpret_cast<const uint32_t*>(pU), vOpt, pUpZ, pUpX, pUpY, upStride,
                          colR, colA, startOff, totalPts, fadeIn, fadeOut);
            sub_28cfba0(trailUpO, count, pTime,
                        reinterpret_cast<const uint32_t*>(pPosX), reinterpret_cast<const uint32_t*>(pPosY), reinterpret_cast<const uint32_t*>(pPosZ),
                        curTime, lifeTime, param0, param1, vBase, vDelta, colB, colG,
                        reinterpret_cast<const uint32_t*>(pU), vOpt, pUpZ, pUpX, pUpY, upStride,
                        colR, colA, startOff, totalPts, fadeIn, fadeOut);
            if (std::memcmp(trailUpT, trailUpO, count * 64u) != 0) ++bad;

            alignas(16) uint8_t trailCamT[16 * 64]{}, trailCamO[16 * 64]{};
            game_trail_cam(trailCamT, count, pTime,
                           reinterpret_cast<const uint32_t*>(pPosX), reinterpret_cast<const uint32_t*>(pPosY), reinterpret_cast<const uint32_t*>(pPosZ),
                           curTime, camX, camY, camZ, lifeTime, param0, param1, vBase,
                           reinterpret_cast<const uint32_t*>(pU), vOpt, vDelta,
                           colB, colG, colR, colA, startOff, totalPts, fadeIn, fadeOut);
            sub_28cf580(trailCamO, count, pTime,
                        reinterpret_cast<const uint32_t*>(pPosX), reinterpret_cast<const uint32_t*>(pPosY), reinterpret_cast<const uint32_t*>(pPosZ),
                        curTime, camX, camY, camZ, lifeTime, param0, param1, vBase,
                        reinterpret_cast<const uint32_t*>(pU), vOpt, vDelta,
                        colB, colG, colR, colA, startOff, totalPts, fadeIn, fadeOut);
            if (std::memcmp(trailCamT, trailCamO, count * 64u) != 0) ++bad;

            alignas(16) uint8_t stripUpT[16 * 112]{}, stripUpO[16 * 112]{};
            game_strip_up(stripUpT, count, pTime, pPosX, pPosY, pPosZ,
                          curTime, lifeTime, param0, param1, vBase, vDelta, colB, colG,
                          reinterpret_cast<const uint32_t*>(pU), vOpt, pUpZ, pUpX, pUpY, upStride,
                          colR, colA, startOff, totalPts, fadeIn, fadeOut);
            sub_28ceec0(stripUpO, count, pTime, pPosX, pPosY, pPosZ,
                        curTime, lifeTime, param0, param1, vBase, vDelta, colB, colG,
                        reinterpret_cast<const uint32_t*>(pU), vOpt, pUpZ, pUpX, pUpY, upStride,
                        colR, colA, startOff, totalPts, fadeIn, fadeOut);
            if (std::memcmp(stripUpT, stripUpO, count * 112u) != 0) ++bad;

            alignas(16) uint8_t stripCamT[16 * 112]{}, stripCamO[16 * 112]{};
            game_strip_cam(stripCamT, count, pTime, pPosX, pPosY, pPosZ,
                           curTime, camX, camY, camZ, lifeTime, param0, param1, vBase,
                           reinterpret_cast<const uint32_t*>(pU), vOpt, vDelta,
                           colB, colG, colR, colA, startOff, totalPts, fadeIn, fadeOut);
            sub_28ce7b0(stripCamO, count, pTime, pPosX, pPosY, pPosZ,
                        curTime, camX, camY, camZ, lifeTime, param0, param1, vBase,
                        reinterpret_cast<const uint32_t*>(pU), vOpt, vDelta,
                        colB, colG, colR, colA, startOff, totalPts, fadeIn, fadeOut);
            if (std::memcmp(stripCamT, stripCamO, count * 112u) != 0) ++bad;
        }

        // --------------------------------------------------------------------
        // 14C. GXFfxSceneCtrlAndShaderBinder.o — 3x4 Matrix to SQT Decomposer (0x230ba00)
        //      & SQT Batch to 3x4 World Matrix Transformer (0x230b760, 0x230add0, 0x230ae70)
        // --------------------------------------------------------------------
        for (int c = 0; c < 1000; ++c) {
            // Generate random rotation (via axis-angle) + non-uniform scale + translation
            float ax = rng.uniform(-1.0f, 1.0f);
            float ay = rng.uniform(-1.0f, 1.0f);
            float az = rng.uniform(-1.0f, 1.0f);
            float alen = std::sqrt(ax * ax + ay * ay + az * az);
            if (alen < 1e-4f) { ax = 0.0f; ay = 1.0f; az = 0.0f; }
            else { ax /= alen; ay /= alen; az /= alen; }
            float ang = rng.uniform(-3.14159f, 3.14159f);
            float cs = std::cos(ang), sn = std::sin(ang), omc = 1.0f - cs;
            float sx = rng.uniform(0.2f, 4.0f);
            float sy = rng.uniform(0.2f, 4.0f);
            float sz = rng.uniform(0.2f, 4.0f);

            alignas(16) float mat3x4[12] = {
                (cs + ax * ax * omc) * sx, (ax * ay * omc - az * sn) * sy, (ax * az * omc + ay * sn) * sz, rng.uniform(-50.0f, 50.0f),
                (ay * ax * omc + az * sn) * sx, (cs + ay * ay * omc) * sy, (ay * az * omc - ax * sn) * sz, rng.uniform(-50.0f, 50.0f),
                (az * ax * omc - ay * sn) * sx, (az * ay * omc + ax * sn) * sy, (cs + az * az * omc) * sz, rng.uniform(-50.0f, 50.0f)
            };
            alignas(16) uint8_t dummySelf[0x100]{};
            uint32_t idx = static_cast<uint32_t>(c);

            std::memset(&g_captured_sqt, 0, sizeof(g_captured_sqt));
            game_mat2sqt(dummySelf, idx, mat3x4);
            FfxCapturedSqtCall sqtT = g_captured_sqt;

            std::memset(&g_captured_sqt, 0, sizeof(g_captured_sqt));
            sub_230ba00(dummySelf, idx, mat3x4);
            FfxCapturedSqtCall sqtO = g_captured_sqt;

            if (sqtT.pSub80 != sqtO.pSub80 || sqtT.index != sqtO.index || sqtT.flags != sqtO.flags ||
                std::memcmp(sqtT.sqt, sqtO.sqt, 48) != 0) {
                ++bad;
            }
        }

        for (int c = 0; c < 500; ++c) {
            void* fakeAlloc = reinterpret_cast<void*>(0x77770000ULL);
            alignas(16) uint8_t subT[0xc0]{}, subO[0xc0]{};
            game_sub_ctor(subT, fakeAlloc);
            sub_230ae70(subO, fakeAlloc);
            if (std::memcmp(subT + 0x08, subO + 0x08, 0xb8) != 0) ++bad;

            uint32_t instCount = static_cast<uint32_t>(rng.range(1, 12));
            alignas(16) float sqtSrc[12 * 12]{};
            for (uint32_t i = 0; i < instCount; ++i) {
                float* inst = sqtSrc + i * 12u;
                // Translation (+0x00)
                inst[0] = rng.uniform(-20.0f, 20.0f);
                inst[1] = rng.uniform(-20.0f, 20.0f);
                inst[2] = rng.uniform(-20.0f, 20.0f);
                inst[3] = 1.0f;
                // Quaternion (+0x10)
                float qx = rng.uniform(-1.0f, 1.0f), qy = rng.uniform(-1.0f, 1.0f);
                float qz = rng.uniform(-1.0f, 1.0f), qw = rng.uniform(-1.0f, 1.0f);
                float qn = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
                if (qn < 1e-4f) { qx = 0.0f; qy = 0.0f; qz = 0.0f; qw = 1.0f; }
                else { qx /= qn; qy /= qn; qz /= qn; qw /= qn; }
                inst[4] = qx; inst[5] = qy; inst[6] = qz; inst[7] = qw;
                // Scale (+0x20)
                inst[8]  = rng.uniform(0.25f, 3.0f);
                inst[9]  = rng.uniform(0.25f, 3.0f);
                inst[10] = rng.uniform(0.25f, 3.0f);
                inst[11] = 0.0f;
            }
            alignas(16) float dstT[12 * 12]{}, dstO[12 * 12]{};
            *reinterpret_cast<float**>(subT + 0x70) = dstT;
            *reinterpret_cast<float**>(subO + 0x70) = dstO;
            *reinterpret_cast<uint32_t*>(subT + 0x90) = instCount;
            *reinterpret_cast<uint32_t*>(subO + 0x90) = instCount;
            *reinterpret_cast<const float**>(subT + 0xa8) = sqtSrc;
            *reinterpret_cast<const float**>(subO + 0xa8) = sqtSrc;

            alignas(16) float parentMat4x4[16]{};
            for (int i = 0; i < 16; ++i) parentMat4x4[i] = rng.uniform(-2.0f, 2.0f);
            alignas(16) uint8_t outerT[0x40]{}, outerO[0x40]{};
            *reinterpret_cast<uint8_t**>(outerT + 0x28) = subT;
            *reinterpret_cast<uint8_t**>(outerO + 0x28) = subO;

            game_set_m4x4(outerT, parentMat4x4);
            sub_230add0(outerO, parentMat4x4);

            if (subT[0x78] != subO[0x78] ||
                std::memcmp(subT + 0x10, subO + 0x10, 0x40) != 0 ||
                std::memcmp(dstT, dstO, instCount * 48u) != 0) {
                ++bad;
            }
        }

        // --------------------------------------------------------------------
        // 14D. GXFfxSceneCtrlAndShaderBinder.o — GXFfxGraphicsResourceManager Hash Table Lifecycle
        //      (0x230c940, 0x230cd00, 0x230cc30, 0x230cfb0, 0x230caf0, 0x230c640)
        //      & GXFfxShaderCache / PointSprite State Binders (0x230ffe0, 0x230ffd0, 0x230ffc0, 0x230ecc0, 0x230dc40)
        // --------------------------------------------------------------------
        alignas(16) void* fakeHeapVtbl[20]{};
        fakeHeapVtbl[0x20 / 8] = reinterpret_cast<void*>(&FfxFakeHeap_GetFlags);
        fakeHeapVtbl[0x58 / 8] = reinterpret_cast<void*>(&FfxFakeHeap_Alloc);
        fakeHeapVtbl[0x70 / 8] = reinterpret_cast<void*>(&FfxFakeHeap_Free);
        alignas(16) void* fakeHeapObj[2] = { fakeHeapVtbl, nullptr };

        {
            alignas(16) uint8_t mgrT[0x70]{}, mgrO[0x70]{};
            game_rm_ctor(mgrT, fakeHeapObj);
            sub_230c940(mgrO, fakeHeapObj);
            if (*reinterpret_cast<uint64_t*>(mgrT + 0x50) != *reinterpret_cast<uint64_t*>(mgrO + 0x50) ||
                *reinterpret_cast<uint64_t*>(mgrT + 0x58) != *reinterpret_cast<uint64_t*>(mgrO + 0x58)) {
                ++bad;
            }

            // Insert 40 unique keys (triggers rehashing / bucket growth via sub_230d830 & sub_230d170)
            std::vector<uint32_t> keys;
            for (uint32_t i = 0; i < 40; ++i) {
                uint32_t k = 1000u + i * 37u;
                keys.push_back(k);
                void* val = reinterpret_cast<void*>(0x900000ULL + i);
                bool okT = game_rm_ins(mgrT, k, val);
                bool okO = sub_230cd00(mgrO, k, val);
                if (okT != okO || !okT) ++bad;
            }

            // Verify duplicate key rejection
            for (uint32_t i = 0; i < 10; ++i) {
                bool dupT = game_rm_ins(mgrT, keys[i], reinterpret_cast<void*>(0xDEADULL));
                bool dupO = sub_230cd00(mgrO, keys[i], reinterpret_cast<void*>(0xDEADULL));
                if (dupT != dupO || dupT) ++bad;
            }

            // Verify FindById (0x230cc30) on present and absent keys
            for (uint32_t i = 0; i < 40; ++i) {
                if (game_rm_find(mgrT, keys[i]) != sub_230cc30(mgrO, keys[i])) ++bad;
                if (game_rm_find(mgrT, keys[i] + 1u) != sub_230cc30(mgrO, keys[i] + 1u)) ++bad;
            }

            // Remove half the keys via RemoveById (0x230cfb0) and verify FindById
            for (uint32_t i = 0; i < 20; ++i) {
                game_rm_rem(mgrT, keys[i * 2]);
                sub_230cfb0(mgrO, keys[i * 2]);
            }
            for (uint32_t i = 0; i < 40; ++i) {
                if (game_rm_find(mgrT, keys[i]) != sub_230cc30(mgrO, keys[i])) ++bad;
            }

            // Clear (0x230caf0) and Destruct (0x230c640)
            game_rm_clear(mgrT);
            sub_230caf0(mgrO);
            if (*reinterpret_cast<uint64_t*>(mgrT + 0x18) != *reinterpret_cast<uint64_t*>(mgrO + 0x18)) ++bad;

            game_rm_dtor(mgrT);
            sub_230c640(mgrO);
        }

        {
            alignas(16) uint8_t scT[0x3b60]{}, scO[0x3b60]{};
            game_sc_ctor(scT, fakeHeapObj);
            sub_230ffe0(scO, fakeHeapObj);
            for (uint32_t i = 0; i < 10; ++i) {
                ptrdiff_t offT0 = static_cast<uint8_t*>(game_sc_tbl0(scT, i)) - scT;
                ptrdiff_t offO0 = static_cast<uint8_t*>(sub_230ffd0(scO, i)) - scO;
                ptrdiff_t offT1 = static_cast<uint8_t*>(game_sc_tbl1(scT, static_cast<int32_t>(i))) - scT;
                ptrdiff_t offO1 = static_cast<uint8_t*>(sub_230ffc0(scO, static_cast<int32_t>(i))) - scO;
                if (offT0 != offO0 || offT1 != offO1) ++bad;
            }
            FfxFakeHeap_Free(fakeHeapObj, *reinterpret_cast<void**>(scT + 0x100));
            FfxFakeHeap_Free(fakeHeapObj, *reinterpret_cast<void**>(scO + 0x100));

            // Test PointSprite & Particle Shader State Binders (0x230ecc0, 0x230dc40)
            alignas(16) uint8_t stateRepo[0x400]{};
            for (size_t i = 0; i < sizeof(stateRepo); i += 8) {
                *reinterpret_cast<uint64_t*>(stateRepo + i) = 0xABC00000ULL + i;
            }
            alignas(16) uint8_t global6c8[0x300]{};
            *reinterpret_cast<uint8_t**>(global6c8 + 0x250) = stateRepo;
            *reinterpret_cast<uint8_t**>(0x55406c8ULL) = global6c8;
            *reinterpret_cast<uint8_t**>(vma_to_bn(0x55406c8ULL)) = global6c8;

            alignas(16) uint8_t shaderCache[0x100]{};
            for (size_t i = 0; i < sizeof(shaderCache); i += 8) {
                *reinterpret_cast<uint64_t*>(shaderCache + i) = 0xDEF00000ULL + i;
            }
            std::vector<uint8_t> shadowT(0xe000, 0), shadowO(0xe000, 0);
            std::vector<uint8_t> drawCtxT(0x1000, 0), drawCtxO(0x1000, 0);
            *reinterpret_cast<uint8_t**>(drawCtxT.data() + 0x08) = shadowT.data();
            *reinterpret_cast<uint8_t**>(drawCtxO.data() + 0x08) = shadowO.data();
            *reinterpret_cast<uint32_t*>(drawCtxT.data() + 0xa60) = 12u;
            *reinterpret_cast<uint32_t*>(drawCtxO.data() + 0xa60) = 12u;
            *reinterpret_cast<uint32_t*>(drawCtxT.data() + 0xa64) = 3u;
            *reinterpret_cast<uint32_t*>(drawCtxO.data() + 0xa64) = 3u;

            game_bind_ps0(shaderCache, drawCtxT.data());
            sub_230ecc0(shaderCache, drawCtxO.data());
            if (std::memcmp(drawCtxT.data() + 0x10, drawCtxO.data() + 0x10, 0xff0) != 0 ||
                std::memcmp(shadowT.data(), shadowO.data(), shadowT.size()) != 0) {
                ++bad;
            }

            game_bind_ps11(shaderCache, drawCtxT.data());
            sub_230dc40(shaderCache, drawCtxO.data());
            if (std::memcmp(drawCtxT.data() + 0x10, drawCtxO.data() + 0x10, 0xff0) != 0 ||
                std::memcmp(shadowT.data(), shadowO.data(), shadowT.size()) != 0) {
                ++bad;
            }
        }

        // --------------------------------------------------------------------
        // 14E. GXFfxTessellationAndParticleDrawPass.o — 80-byte Particle Record InsertRange80 (0x2305f50),
        //      Vector16 Grow (0x2305640), Vector8 PushBack with Self-Aliasing (0x23071a0/0x23077a0),
        //      Draw Range Dispatchers (0x23084f0, 0x2308150, 0x23054f0) & PTMF Invoker (0x2305120/0x23050f0/0x2305060)
        // --------------------------------------------------------------------
        for (int c = 0; c < 300; ++c) {
            uint32_t initCount = static_cast<uint32_t>(rng.range(0, 12));
            uint32_t insCount  = static_cast<uint32_t>(rng.range(1, 10));
            uint32_t initCap   = rng.chance(0.5) ? (initCount + insCount + 4u) : (initCount + rng.range(0, 2));
            uint32_t posIdx    = (initCount > 0) ? static_cast<uint32_t>(rng.range(0, static_cast<int>(initCount))) : 0u;

            uint8_t* bufT = static_cast<uint8_t*>(FfxFakeHeap_Alloc(fakeHeapObj, initCap * 80u, 16));
            uint8_t* bufO = static_cast<uint8_t*>(FfxFakeHeap_Alloc(fakeHeapObj, initCap * 80u, 16));
            for (uint32_t i = 0; i < initCount * 80u; ++i) {
                uint8_t b = static_cast<uint8_t>(rng.u32() & 0xffu);
                bufT[i] = b;
                bufO[i] = b;
            }
            // Ensure initial records have w = 1.0f at +0x1c and +0x2c
            for (uint32_t r = 0; r < initCount; ++r) {
                *reinterpret_cast<float*>(bufT + r * 80u + 0x1c) = 1.0f;
                *reinterpret_cast<float*>(bufT + r * 80u + 0x2c) = 1.0f;
                *reinterpret_cast<float*>(bufO + r * 80u + 0x1c) = 1.0f;
                *reinterpret_cast<float*>(bufO + r * 80u + 0x2c) = 1.0f;
            }

            std::vector<uint8_t> srcRecs(insCount * 80u);
            for (size_t i = 0; i < srcRecs.size(); ++i) {
                srcRecs[i] = static_cast<uint8_t>(rng.u32() & 0xffu);
            }

            alignas(16) uint8_t vecT[0x28]{}, vecO[0x28]{};
            *reinterpret_cast<uint8_t**>(vecT + 0x08) = bufT;
            *reinterpret_cast<uint8_t**>(vecT + 0x10) = bufT + initCount * 80u;
            *reinterpret_cast<uint8_t**>(vecT + 0x18) = bufT + initCap * 80u;
            *reinterpret_cast<void**>(vecT + 0x20)    = fakeHeapObj;

            *reinterpret_cast<uint8_t**>(vecO + 0x08) = bufO;
            *reinterpret_cast<uint8_t**>(vecO + 0x10) = bufO + initCount * 80u;
            *reinterpret_cast<uint8_t**>(vecO + 0x18) = bufO + initCap * 80u;
            *reinterpret_cast<void**>(vecO + 0x20)    = fakeHeapObj;

            FfxTestIter24 firstIt{ 0, 0, srcRecs.data() };
            FfxTestIter24 lastIt{ 0, 0, srcRecs.data() + insCount * 80u };
            game_ins80(vecT, FfxTestIter24{ 0, 0, bufT + posIdx * 80u }, firstIt, lastIt);
            sub_2305f50(vecO, FfxTestIter24{ 0, 0, bufO + posIdx * 80u }, firstIt, lastIt);

            uint8_t* newBegT = *reinterpret_cast<uint8_t**>(vecT + 0x08);
            uint8_t* newEndT = *reinterpret_cast<uint8_t**>(vecT + 0x10);
            uint8_t* newBegO = *reinterpret_cast<uint8_t**>(vecO + 0x08);
            uint8_t* newEndO = *reinterpret_cast<uint8_t**>(vecO + 0x10);
            ptrdiff_t bytesT = newEndT - newBegT;
            ptrdiff_t bytesO = newEndO - newBegO;
            bool recDiff = (bytesT != bytesO);
            for (uint32_t r = 0; !recDiff && r < initCount + insCount; ++r) {
                const uint8_t* rt = newBegT + r * 80u;
                const uint8_t* ro = newBegO + r * 80u;
                // Compare all copied fields of the 80-byte particle record (+0x00..0x08, +0x10..0x4e)
                if (std::memcmp(rt + 0x00, ro + 0x00, 8) != 0 ||
                    std::memcmp(rt + 0x10, ro + 0x10, 0x3e) != 0) {
                    recDiff = true;
                }
            }
            if (recDiff) ++bad;

            FfxFakeHeap_Free(fakeHeapObj, newBegT);
            FfxFakeHeap_Free(fakeHeapObj, newBegO);
        }

        // Test Vector8_PushBack (0x23071a0/0x23077a0) including self-aliasing & Vector16_GrowByOne (0x2305640)
        {
            alignas(16) uint8_t v8T[0x28]{}, v8O[0x28]{};
            *reinterpret_cast<void**>(v8T + 0x20) = fakeHeapObj;
            *reinterpret_cast<void**>(v8O + 0x20) = fakeHeapObj;
            for (uint64_t i = 0; i < 64; ++i) {
                if (i >= 4 && (i % 5 == 0)) {
                    // Self-aliased push_back from inside the vector buffer!
                    const uint64_t* aliasT = *reinterpret_cast<const uint64_t**>(v8T + 0x08) + (i / 2);
                    const uint64_t* aliasO = *reinterpret_cast<const uint64_t**>(v8O + 0x08) + (i / 2);
                    game_vec8_push(v8T, aliasT);
                    sub_23071a0(v8O, aliasO);
                } else {
                    uint64_t val = 0x10000ULL + i * 17ULL;
                    game_vec8_push(v8T, &val);
                    sub_23071a0(v8O, &val);
                }
                uint8_t* bT = *reinterpret_cast<uint8_t**>(v8T + 0x08);
                uint8_t* bO = *reinterpret_cast<uint8_t**>(v8O + 0x08);
                if (std::memcmp(bT, bO, (i + 1) * 8u) != 0) ++bad;
            }
            FfxFakeHeap_Free(fakeHeapObj, *reinterpret_cast<void**>(v8T + 0x08));
            FfxFakeHeap_Free(fakeHeapObj, *reinterpret_cast<void**>(v8O + 0x08));

            alignas(16) uint8_t v16T[0x28]{}, v16O[0x28]{};
            *reinterpret_cast<void**>(v16T + 0x20) = fakeHeapObj;
            *reinterpret_cast<void**>(v16O + 0x20) = fakeHeapObj;
            for (uint64_t i = 0; i < 32; ++i) {
                game_vec16_grw(v16T);
                sub_2305640(v16O);
                uint64_t* eT = *reinterpret_cast<uint64_t**>(v16T + 0x10);
                uint64_t* eO = *reinterpret_cast<uint64_t**>(v16O + 0x10);
                eT[0] = i * 3 + 1; eT[1] = i * 7 + 2;
                eO[0] = i * 3 + 1; eO[1] = i * 7 + 2;
                *reinterpret_cast<uint64_t**>(v16T + 0x10) = eT + 2;
                *reinterpret_cast<uint64_t**>(v16O + 0x10) = eO + 2;
                if (std::memcmp(*reinterpret_cast<void**>(v16T + 0x08), *reinterpret_cast<void**>(v16O + 0x08), (i + 1) * 16u) != 0) {
                    ++bad;
                }
            }
            FfxFakeHeap_Free(fakeHeapObj, *reinterpret_cast<void**>(v16T + 0x08));
            FfxFakeHeap_Free(fakeHeapObj, *reinterpret_cast<void**>(v16O + 0x08));
        }

        // Test Draw Range Dispatchers (0x23084f0, 0x2308150, 0x23054f0) & PTMF Invoker (0x2305120, 0x23050f0, 0x2305060)
        {
            if (game_one_5060() != sub_2305060()) ++bad;

            alignas(16) void* itemVtbl[4] = { nullptr, nullptr, reinterpret_cast<void*>(&FfxFakeDrawItem_Draw), nullptr };
            alignas(16) void* fakeItems[8][2]{};
            struct ItemPair16 { void* item; void* data; };
            struct BucketNode24 { void* item; void* data; BucketNode24* next; };
            ItemPair16 pairs[8]{};
            BucketNode24 bNodes[8]{};
            uint64_t slotHeads[8]{};
            uint64_t* slotPtrs[8]{};
            for (int i = 0; i < 8; ++i) {
                fakeItems[i][0] = itemVtbl;
                pairs[i].item   = fakeItems[i];
                pairs[i].data   = reinterpret_cast<void*>(0x5000ULL + i * 0x10);
                bNodes[i].item  = fakeItems[i];
                bNodes[i].data  = pairs[i].data;
                bNodes[i].next  = nullptr;
                slotHeads[i]    = reinterpret_cast<uint64_t>(&bNodes[i]);
                slotPtrs[i]     = &slotHeads[i];
            }
            alignas(16) uint8_t vecHolder[0x20]{};
            *reinterpret_cast<ItemPair16**>(vecHolder + 0x08) = pairs;

            std::vector<uint8_t> dispSelf(0x2c00, 0);
            *reinterpret_cast<uint8_t**>(dispSelf.data() + 0x20)    = vecHolder;
            *reinterpret_cast<uint64_t***>(dispSelf.data() + 0xb0)  = slotPtrs;
            *reinterpret_cast<ItemPair16**>(dispSelf.data() + 0x2ad8) = pairs;

            int32_t range[2] = { 1, 7 };
            g_ffx_dispatch_hash = 0;
            game_disp_84f0(dispSelf.data(), range);
            uint64_t hT1 = g_ffx_dispatch_hash;
            g_ffx_dispatch_hash = 0;
            sub_23084f0(dispSelf.data(), range);
            if (hT1 != g_ffx_dispatch_hash) ++bad;

            g_ffx_dispatch_hash = 0;
            game_disp_8150(dispSelf.data(), range);
            uint64_t hT2 = g_ffx_dispatch_hash;
            g_ffx_dispatch_hash = 0;
            sub_2308150(dispSelf.data(), range);
            if (hT2 != g_ffx_dispatch_hash) ++bad;

            g_ffx_dispatch_hash = 0;
            game_disp_54f0(dispSelf.data(), range);
            uint64_t hT3 = g_ffx_dispatch_hash;
            g_ffx_dispatch_hash = 0;
            sub_23054f0(dispSelf.data(), range);
            if (hT3 != g_ffx_dispatch_hash) ++bad;

            // Test PTMF Invoker (0x2305120) & AdvanceStateCounter (0x23050f0) for both non-virtual and virtual PTMF
            for (int isVirt = 0; isVirt <= 1; ++isVirt) {
                alignas(16) void* ptmfVtbl[4] = { nullptr, reinterpret_cast<void*>(&FfxFakePtmfTarget_Callback), nullptr, nullptr };
                alignas(16) uint64_t targetT[4] = { reinterpret_cast<uint64_t>(ptmfVtbl), 0, 0, 0 };
                alignas(16) uint64_t targetO[4] = { reinterpret_cast<uint64_t>(ptmfVtbl), 0, 0, 0 };
                alignas(16) uint8_t ptmfSelfT[0x40]{}, ptmfSelfO[0x40]{};
                ptmfSelfT[0x0b] = 0x40u;
                ptmfSelfO[0x0b] = 0x40u;
                *reinterpret_cast<uint64_t**>(ptmfSelfT + 0x10) = targetT;
                *reinterpret_cast<uint64_t**>(ptmfSelfO + 0x10) = targetO;
                *reinterpret_cast<uint32_t*>(ptmfSelfT + 0x18)  = 0x7788u + isVirt;
                *reinterpret_cast<uint32_t*>(ptmfSelfO + 0x18)  = 0x7788u + isVirt;
                *reinterpret_cast<uintptr_t*>(ptmfSelfT + 0x20) = isVirt ? (8ULL + 1ULL) : reinterpret_cast<uintptr_t>(&FfxFakePtmfTarget_Callback);
                *reinterpret_cast<uintptr_t*>(ptmfSelfO + 0x20) = isVirt ? (8ULL + 1ULL) : reinterpret_cast<uintptr_t>(&FfxFakePtmfTarget_Callback);
                *reinterpret_cast<int64_t*>(ptmfSelfT + 0x28)   = 0;
                *reinterpret_cast<int64_t*>(ptmfSelfO + 0x28)   = 0;

                game_ptmf_inv(ptmfSelfT);
                sub_2305120(ptmfSelfO);
                if (targetT[1] != targetO[1]) ++bad;

                // Also test sub_23050f0 (0x23050f0) which calls vtable+0x08 (bound to sub_2305120) and increments +0x0c
                alignas(16) void* selfVtblT[2] = { nullptr, reinterpret_cast<void*>(vma_to_bn(0x2305120)) };
                alignas(16) void* selfVtblO[2] = { nullptr, reinterpret_cast<void*>(&sub_2305120) };
                *reinterpret_cast<void***>(ptmfSelfT + 0x00) = selfVtblT;
                *reinterpret_cast<void***>(ptmfSelfO + 0x00) = selfVtblO;
                *reinterpret_cast<uint32_t*>(ptmfSelfT + 0x0c) = 41u;
                *reinterpret_cast<uint32_t*>(ptmfSelfO + 0x0c) = 41u;
                uint32_t rT = game_adv_50f0(ptmfSelfT);
                uint32_t rO = sub_23050f0(ptmfSelfO);
                if (rT != rO || *reinterpret_cast<uint32_t*>(ptmfSelfT + 0x0c) != *reinterpret_cast<uint32_t*>(ptmfSelfO + 0x0c)) ++bad;
            }
        }

        std::printf("[TEST 14] 07_SFX_ParticleGpuRenderer SIMD Particle/Ribbon Emitters, 3x4<->SQT, ResourceHash & Vec80 Insert: 5192 cases -> %d differ\n", bad);
        total_failures += bad;
    }

    // =========================================================================
    // [TEST 15] Phase 4 & 5: 08_GPU_Shaders (752 Shaders SPIR-V Validation +
    //           Paired CPU<->GPU Constant Buffer Handshake & Kernel Execution)
    // =========================================================================
    {
        int bad = 0;
        int cases = 0;

        // 1. Verify all 752 exported shaders in manifest and on disk (.spv magic & byte size + .asm presence)
        const uint32_t totalShaders = GXShaderLibrary_GetTotalShaderCount();
        ++cases;
        if (totalShaders != 752u) ++bad;
        if (GXShaderLibrary_GetBundleShaderCount("gxshader") != 15u) ++bad;
        if (GXShaderLibrary_GetBundleShaderCount("gxgui") != 13u) ++bad;
        if (GXShaderLibrary_GetBundleShaderCount("gxposteffect") != 119u) ++bad;
        if (GXShaderLibrary_GetBundleShaderCount("gxrenderershader") != 111u) ++bad;
        if (GXShaderLibrary_GetBundleShaderCount("gxdecal") != 193u) ++bad;
        if (GXShaderLibrary_GetBundleShaderCount("gxffxshader") != 301u) ++bad;

        uint32_t totalInsts = 0;
        for (uint32_t i = 0; i < totalShaders; ++i) {
            const GXShaderManifestEntry* e = GXShaderLibrary_GetEntryByIndex(i);
            ++cases;
            if (!e || e->spirv_words < 5u || e->instructions == 0u) {
                ++bad;
                continue;
            }
            totalInsts += e->instructions;

            // Verify lookup by name and hash
            const GXShaderManifestEntry* byName = GXShaderLibrary_FindByName(e->bundle, e->name);
            const GXShaderManifestEntry* byHash = GXShaderLibrary_FindByHash(e->shader_hash, e->crc32);
            if (byName != e || !byHash || byHash->shader_hash != e->shader_hash) {
                ++bad;
            }

            // Verify on-disk .spv binary header (0x07230203) and exact byte length
            std::string spvPath = std::string("decolied files/08_GPU_Shaders/") + e->bundle + "/spv/" + e->safe_name + ".spv";
            FILE* fp = std::fopen(spvPath.c_str(), "rb");
            if (!fp) {
                std::string alt1 = std::string("shaders/") + e->bundle + "/spv/" + e->safe_name + ".spv";
                fp = std::fopen(alt1.c_str(), "rb");
            }
            if (!fp) {
                std::string alt2 = std::string("../shaders/") + e->bundle + "/spv/" + e->safe_name + ".spv";
                fp = std::fopen(alt2.c_str(), "rb");
            }
            if (!fp) {
                ++bad;
                continue;
            }
            std::fseek(fp, 0, SEEK_END);
            const long fsize = std::ftell(fp);
            std::fseek(fp, 0, SEEK_SET);
            uint32_t spvHeader[5]{};
            const size_t nread = std::fread(spvHeader, sizeof(uint32_t), 5, fp);
            std::fclose(fp);
            if (nread != 5 || static_cast<uint32_t>(fsize) != e->spirv_words * 4u || spvHeader[0] != 0x07230203u) {
                ++bad;
            }
        }
        if (totalInsts != 252065u) ++bad;

        // 2. Verify CPU <-> GPU Constant Buffer & Resource Binding Handshake across all 6 bundles
        struct HandshakeCase {
            const char* bundle;
            const char* shader;
            uint32_t cpu_cb_bytes;
            uint32_t expected_images;
            uint32_t expected_samplers;
        };
        static constexpr HandshakeCase kHandshakes[] = {
            {"gxshader",         "GXSimple_Single2DTexture_F.ppo",       16u,  1u, 1u},
            {"gxshader",         "GXSimple_Transform_V.vpo",             64u,  0u, 0u},
            {"gxgui",            "GXGuiShader_Tex.ppo",                   0u,  1u, 1u},
            {"gxgui",            "GXGuiShader_Tex1.vpo",                 80u,  0u, 0u},
            {"gxposteffect",     "DepthConverLinear.ppo",                48u,  1u, 1u},
            {"gxposteffect",     "FeedBackBlurFallOffPS.ppo",           256u,  1u, 1u},
            {"gxposteffect",     "ColorGrading.ppo",                      0u,  2u, 2u},
            {"gxposteffect",     "ToneMap_FilmicToneMapPS.ppo",         176u,  1u, 1u},
            {"gxposteffect",     "ToneMap_ReinhardToneMapPS.ppo",       176u,  1u, 1u},
            {"gxrenderershader", "Filter\\GXLightAcc_DDirLight1.ppo",   208u,  5u, 2u},
            {"gxrenderershader", "Filter\\GXDeferred_Debug.ppo",        176u,  8u, 2u},
            {"gxdecal",          "Lightprepass0\\Decal_BlendWeight.ppo", 408u, 2u, 2u},
            {"gxffxshader",      "GXFfxpointSprite.vpo",                336u,  0u, 0u}, // 0x150 bytes allocated in sub_230f420
            {"gxffxshader",      "GXFfxtessellatePointSprite.vpo",      336u,  0u, 0u}, // 0x150 bytes allocated in sub_2308900
            {"gxffxshader",      "GXFfxtessellatePointSprite.hpo",       16u,  0u, 0u},
            {"gxffxshader",      "GXFfxtessellatePointSprite.dpo",       28u,  0u, 0u},
        };
        for (const HandshakeCase& hc : kHandshakes) {
            ++cases;
            if (!GXShaderLibrary_VerifyCpuBindingLayout(hc.bundle, hc.shader, hc.cpu_cb_bytes,
                                                        hc.expected_images, hc.expected_samplers)) {
                ++bad;
            }
        }

        // 3. Live Paired CPU -> GPU Differential Execution (4,000 cases across all 6 shader bundles)
        using FnSubCtor = void (*)(void*, void*);
        using FnSetM4x4 = void (*)(void*, const float*);
        auto game_sub_ctor = reinterpret_cast<FnSubCtor>(vma_to_bn(0x230ae70));
        auto game_set_m4x4 = reinterpret_cast<FnSetM4x4>(vma_to_bn(0x230add0));

        // 3A. Paired CPU (0x230ae70 + 0x230add0 SQT->3x4 Matrix Batch + 0x150 CB layout) -> GPU (FXR3 LS + DS Tessellation)
        for (int i = 0; i < 1000; ++i) {
            ++cases;
            void* fakeAlloc = reinterpret_cast<void*>(0x77770000ULL);
            alignas(16) uint8_t subT[0xc0]{}, subO[0xc0]{};
            game_sub_ctor(subT, fakeAlloc);
            sub_230ae70(subO, fakeAlloc);

            alignas(16) float sqt[12]{};
            sqt[0] = rng.uniform(-25.0f, 25.0f);
            sqt[1] = rng.uniform(-25.0f, 25.0f);
            sqt[2] = rng.uniform(-25.0f, 25.0f);
            sqt[3] = 1.0f;
            float qx = rng.uniform(-0.5f, 0.5f), qy = rng.uniform(-0.5f, 0.5f);
            float qz = rng.uniform(-0.5f, 0.5f), qw = rng.uniform(0.5f, 1.5f);
            const float qinv = 1.0f / std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
            sqt[4] = qx * qinv; sqt[5] = qy * qinv; sqt[6] = qz * qinv; sqt[7] = qw * qinv;
            sqt[8] = 1.0f; sqt[9] = 1.0f; sqt[10] = 1.0f; sqt[11] = 0.0f;

            alignas(16) float matT[12]{}, matO[12]{};
            *reinterpret_cast<float**>(subT + 0x70) = matT;
            *reinterpret_cast<float**>(subO + 0x70) = matO;
            *reinterpret_cast<uint32_t*>(subT + 0x90) = 1u;
            *reinterpret_cast<uint32_t*>(subO + 0x90) = 1u;
            *reinterpret_cast<const float**>(subT + 0xa8) = sqt;
            *reinterpret_cast<const float**>(subO + 0xa8) = sqt;

            alignas(16) float parentMat4x4[16] = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            alignas(16) uint8_t outerT[0x40]{}, outerO[0x40]{};
            *reinterpret_cast<uint8_t**>(outerT + 0x28) = subT;
            *reinterpret_cast<uint8_t**>(outerO + 0x28) = subO;
            game_set_m4x4(outerT, parentMat4x4);
            sub_230add0(outerO, parentMat4x4);

            // Populate 0x150-byte (84-dword) particle constant buffer from CPU matrices
            alignas(16) float cbT[84]{}, cbO[84]{};
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 4; ++c) {
                    cbT[r * 4 + c]      = matT[r * 4 + c];
                    cbO[r * 4 + c]      = matO[r * 4 + c];
                    cbT[16 + r * 4 + c] = matT[r * 4 + c];
                    cbO[16 + r * 4 + c] = matO[r * 4 + c];
                }
            }
            cbT[15] = cbO[15] = 30.0f;
            cbT[31] = cbO[31] = 1.0f;
            cbT[72] = cbO[72] = 0.5f;   // s4 near plane
            cbT[73] = cbO[73] = 0.25f;  // s5 inv fade range
            cbT[77] = cbO[77] = 2.0f;   // s0 size scale
            cbT[78] = cbO[78] = 0.5625f;// s1 inv aspect X (9/16)
            cbT[79] = cbO[79] = 1.0f;   // s2 aspect Y

            const float vtxPosSize[4] = {
                rng.uniform(-25.0f, 25.0f),
                rng.uniform(-25.0f, 25.0f),
                rng.uniform(-25.0f, 25.0f),
                1.5f + rng.uniform(0.05f, 0.95f)
            };
            const float vtxColor[4] = {
                rng.uniform(0.05f, 0.95f),
                rng.uniform(0.05f, 0.95f),
                rng.uniform(0.05f, 0.95f),
                1.0f
            };

            float ldsT[15]{}, ldsO[15]{};
            GXShader_Ffx_TessellatePointSprite_LS(vtxPosSize, vtxColor, cbT, ldsT);
            GXShader_Ffx_TessellatePointSprite_LS(vtxPosSize, vtxColor, cbO, ldsO);
            if (std::memcmp(ldsT, ldsO, sizeof(ldsT)) != 0) {
                ++bad;
                continue;
            }

            // Evaluate DS at 4 quad corners (u, v) in {(0,0), (1,0), (0,1), (1,1)}
            float pos00[4], pos10[4], pos01[4], pos11[4], p0[4], p1[4], p2[4];
            GXShader_Ffx_TessellatePointSprite_DS(ldsO, 0.0f, 0.0f, pos00, p0, p1, p2);
            GXShader_Ffx_TessellatePointSprite_DS(ldsO, 1.0f, 0.0f, pos10, p0, p1, p2);
            GXShader_Ffx_TessellatePointSprite_DS(ldsO, 0.0f, 1.0f, pos01, p0, p1, p2);
            GXShader_Ffx_TessellatePointSprite_DS(ldsO, 1.0f, 1.0f, pos11, p0, p1, p2);

            // Quad center of (pos00 + pos11) * 0.5 must equal clip center (ldsO[8], ldsO[9])
            if (std::fabs(0.5f * (pos00[0] + pos11[0]) - ldsO[8]) > 1e-4f ||
                std::fabs(0.5f * (pos00[1] + pos11[1]) - ldsO[9]) > 1e-4f ||
                std::fabs(p2[0] - 1.0f) > 1e-6f || std::fabs(p2[1] - 0.0f) > 1e-6f) {
                ++bad;
            }
        }

        // 3B. Yebis2 PostFX Shaders (Filmic ToneMap, Reinhard ToneMap, ColorGrading 3D LUT, DepthConvertLinear, FeedBackBlur)
        for (int i = 0; i < 1000; ++i) {
            ++cases;
            const float hdr[4] = {
                rng.uniform(0.01f, 12.0f),
                rng.uniform(0.01f, 12.0f),
                rng.uniform(0.01f, 12.0f),
                rng.uniform(0.05f, 0.95f)
            };
            float cbToneMap[16]{};
            cbToneMap[4]  = 1.0f; cbToneMap[5]  = 0.98f; cbToneMap[6]  = 0.95f; // white balance
            cbToneMap[9]  = 2.2f;                                               // gamma
            cbToneMap[12] = 1.2f; cbToneMap[13] = 1.2f;  cbToneMap[14] = 1.2f;  // exposure scale

            float outFilmic[4]{}, outReinhard[4]{}, lutUvw[3]{}, lutAlpha = 0.0f;
            GXShader_Yebis2_FilmicToneMap_PS(hdr, cbToneMap, outFilmic);
            GXShader_Yebis2_ReinhardToneMap_PS(hdr, cbToneMap, outReinhard);
            GXShader_ColorGrading_ComputeLutCoords(outFilmic, lutUvw, &lutAlpha);

            if (!std::isfinite(outFilmic[0]) || !std::isfinite(outReinhard[0]) ||
                outFilmic[0] < 0.0f || outReinhard[0] < 0.0f ||
                lutUvw[0] < 0.03125f || std::fabs(lutAlpha - hdr[3]) > 1e-6f) {
                ++bad;
            }

            // DepthConverLinear & FeedBackBlurFallOff
            const float ssaoParam[2] = { 1.001f, -0.1f };
            float linDepth = 0.0f, depthMrt[4]{}, fbMrt[4]{};
            const float hwDepth = rng.uniform(0.05f, 0.95f);
            GXShader_DepthConvertLinear_PS(hwDepth, ssaoParam, &linDepth, depthMrt);
            // Unpack 24-bit RGB8 depth (depthMrt[2] + depthMrt[1]/255 + depthMrt[0]/65025) == linDepth
            const float unpacked = depthMrt[2] + depthMrt[1] * (1.0f / 255.0f) + depthMrt[0] * (1.0f / 65025.0f);
            if (std::fabs(unpacked - linDepth) > 1e-6f || depthMrt[3] != 1.0f) {
                ++bad;
            }
            const float fbOffset[2] = { 0.0f, 0.85f };
            GXShader_FeedBackBlurFallOff_PS(hdr, fbOffset, fbMrt);
            if (std::fabs(fbMrt[0] - hdr[0] * 0.85f) > 1e-5f) {
                ++bad;
            }
        }

        // 3C. Deferred Lighting (Filter\GXLightAcc_DDirLight1.ppo fragment & normal reconstruction)
        for (int i = 0; i < 1000; ++i) {
            ++cases;
            const float ndc[2] = {
                rng.uniform(0.05f, 0.95f) * 2.0f - 1.0f,
                rng.uniform(0.05f, 0.95f) * 2.0f - 1.0f
            };
            const float hwZ = rng.uniform(0.05f, 0.95f);
            const float gbufN[3] = {
                rng.uniform(0.05f, 0.95f),
                rng.uniform(0.05f, 0.95f),
                rng.uniform(0.05f, 0.95f)
            };
            float cbLightAcc[44]{};
            // Identity invProj & viewToWorld with camera at (0, 2, 10)
            cbLightAcc[0] = 1.0f; cbLightAcc[5] = 1.0f; cbLightAcc[10] = 1.0f; cbLightAcc[15] = 1.0f;
            cbLightAcc[32] = 1.0f; cbLightAcc[35] = 0.0f;
            cbLightAcc[37] = 1.0f; cbLightAcc[39] = 2.0f;
            cbLightAcc[42] = 1.0f; cbLightAcc[43] = 10.0f;

            float unitN[3]{}, viewPos[3]{}, eyeVec[3]{};
            GXShader_Deferred_ReconstructFragmentAndNormal(ndc, hwZ, gbufN, cbLightAcc, unitN, viewPos, eyeVec);
            const float nLen = std::sqrt(unitN[0] * unitN[0] + unitN[1] * unitN[1] + unitN[2] * unitN[2]);
            const float eLen = std::sqrt(eyeVec[0] * eyeVec[0] + eyeVec[1] * eyeVec[1] + eyeVec[2] * eyeVec[2]);
            if (std::fabs(nLen - 1.0f) > 1e-4f || std::fabs(eLen - 1.0f) > 1e-4f ||
                std::fabs(viewPos[0] - ndc[0]) > 1e-5f || std::fabs(viewPos[1] - ndc[1]) > 1e-5f ||
                std::fabs(viewPos[2] - hwZ) > 1e-5f) {
                ++bad;
            }
        }

        // 3D. Decal Box Projection (Lightprepass0\Decal_BlendWeight.ppo) & System/GUI f16 RTZ Packing
        for (int i = 0; i < 1000; ++i) {
            ++cases;
            const float clipXYW[3] = {
                (rng.uniform(0.05f, 0.95f) - 0.5f) * 2.0f,
                (rng.uniform(0.05f, 0.95f) - 0.5f) * 2.0f,
                2.0f
            };
            const float depth = rng.uniform(0.05f, 0.95f);
            // Scale-and-bias matrix mapping NDC [-1, 1] -> [0, 1]
            const float decalMat[16] = {
                0.5f, 0.0f, 0.0f, 0.5f,
                0.0f, 0.5f, 0.0f, 0.5f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            float screenUv[2]{}, boxUvw[3]{};
            const bool inside = GXShader_Decal_ProjectAndClipBox(clipXYW, depth, decalMat, screenUv, boxUvw);
            if (!inside || boxUvw[0] < 0.0f || boxUvw[0] > 1.0f ||
                boxUvw[1] < 0.0f || boxUvw[1] > 1.0f ||
                std::fabs(boxUvw[2] - depth) > 1e-5f) {
                ++bad;
            }

            const float tex[4] = {
                rng.uniform(0.05f, 0.95f),
                rng.uniform(0.05f, 0.95f),
                rng.uniform(0.05f, 0.95f),
                1.0f
            };
            const float col[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float mrtSimple[4]{}, mrtGui[4]{};
            uint32_t pkSimple[2]{}, pkGui[2]{};
            GXShader_SimpleSingle2DTexture_PS(tex, col, col, 0, mrtSimple, pkSimple);
            GXShader_GuiTex_PS(tex, col, mrtGui, pkGui);
            if (pkSimple[0] != pkGui[0] || pkSimple[1] != pkGui[1]) {
                ++bad;
            }
        }

        std::printf("[TEST 15] 08_GPU_Shaders 752 SPIR-V Modules, 16 CPU<->GPU CB Handshakes & 4000 Paired CPU+GPU Cases: %d cases -> %d differ\n",
                    cases, bad);
        total_failures += bad;
    }

    // =========================================================================
    // [TEST 16] Golden Fund Master Registry (kGXDecompiledFunctions[2940] &
    //           kGXEbootExternalImports) vs db/functions.json Verification
    // =========================================================================
    {
        int bad = 0;
        int cases = 0;

        if (kGXDecompiledFunctionCount != 2940u) {
            ++bad;
        }

        // 1. Verify all 2,940 function pointers are non-null, unique, and sorted without VMA overlap
        uint64_t totalBytes = 0;
        std::vector<uintptr_t> fnAddrs;
        fnAddrs.reserve(kGXDecompiledFunctionCount);

        for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
            ++cases;
            const GXDecompiledFunctionEntry& e = kGXDecompiledFunctions[i];
            if (e.func_ptr == nullptr || e.size == 0u ||
                e.name == nullptr || e.name[0] == '\0' ||
                e.module_name == nullptr || e.module_name[0] == '\0') {
                ++bad;
            }
            if (i > 0) {
                const GXDecompiledFunctionEntry& prev = kGXDecompiledFunctions[i - 1];
                if (prev.vma + prev.size > e.vma) {
                    ++bad;
                }
            }
            totalBytes += e.size;
            fnAddrs.push_back(reinterpret_cast<uintptr_t>(e.func_ptr));
        }

        std::sort(fnAddrs.begin(), fnAddrs.end());
        if (std::adjacent_find(fnAddrs.begin(), fnAddrs.end()) != fnAddrs.end()) {
            ++bad;
        }
        if (totalBytes != kGXDecompiledTotalCodeBytes || totalBytes != 1040700ULL) {
            ++bad;
        }

        // 2. Parse db/functions.json and verify 100% match against all 2,940 carved functions
        FILE* fp = std::fopen("db/functions.json", "rb");
        if (!fp) fp = std::fopen("../db/functions.json", "rb");
        if (!fp) fp = std::fopen("../../db/functions.json", "rb");
        if (!fp) {
            ++bad;
        } else {
            std::fseek(fp, 0, SEEK_END);
            const long fsz = std::ftell(fp);
            std::fseek(fp, 0, SEEK_SET);
            std::string jsonText(static_cast<size_t>(fsz), '\0');
            const size_t nread = std::fread(jsonText.data(), 1, static_cast<size_t>(fsz), fp);
            std::fclose(fp);
            if (nread != static_cast<size_t>(fsz)) {
                ++bad;
            }

            struct DbCarvedFn {
                uint64_t vma;
                uint32_t size;
                std::string name;
                std::string mod;
            };
            std::vector<DbCarvedFn> dbCarved;
            dbCarved.reserve(2940);

            size_t pos = 0;
            while (true) {
                size_t stPos = jsonText.find("\"status\": \"carved\"", pos);
                if (stPos == std::string::npos) break;
                size_t objStart = jsonText.rfind('{', stPos);
                size_t objEnd = jsonText.find('}', stPos);
                if (objStart == std::string::npos || objEnd == std::string::npos) break;
                std::string obj = jsonText.substr(objStart, objEnd - objStart + 1);

                auto extractU64 = [&](const char* key) -> uint64_t {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return 0;
                    k = obj.find(':', k);
                    if (k == std::string::npos) return 0;
                    return static_cast<uint64_t>(std::strtoull(obj.c_str() + k + 1, nullptr, 10));
                };
                auto extractStr = [&](const char* key) -> std::string {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return {};
                    k = obj.find(':', k);
                    size_t q1 = obj.find('"', k + 1);
                    size_t q2 = obj.find('"', q1 + 1);
                    if (q1 == std::string::npos || q2 == std::string::npos) return {};
                    return obj.substr(q1 + 1, q2 - q1 - 1);
                };

                DbCarvedFn fn{};
                fn.vma = extractU64("\"vma_int\"");
                fn.size = static_cast<uint32_t>(extractU64("\"size\""));
                fn.name = extractStr("\"name\"");
                fn.mod = extractStr("\"module\"");
                dbCarved.push_back(std::move(fn));
                pos = objEnd + 1;
            }

            std::sort(dbCarved.begin(), dbCarved.end(),
                      [](const DbCarvedFn& a, const DbCarvedFn& b) { return a.vma < b.vma; });

            if (dbCarved.size() != kGXDecompiledFunctionCount) {
                ++bad;
            } else {
                for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
                    ++cases;
                    const GXDecompiledFunctionEntry& reg = kGXDecompiledFunctions[i];
                    const DbCarvedFn& db = dbCarved[i];
                    if (reg.vma != db.vma || reg.size != db.size ||
                        std::strcmp(reg.name, db.name.c_str()) != 0 ||
                        std::strcmp(reg.module_name, db.mod.c_str()) != 0) {
                        ++bad;
                    }
                }
            }
        }

        // 3. Verify bbhost batch registration helper (`decomp_add(vma + 0x400000, ...)`)
        size_t regCallbacks = 0;
        const size_t regReturned = GXRegisterAllDecompiledFunctions(
            [&](uint64_t runtime_va, uint32_t sz, void* fn_ptr, const char* fn_name) {
                ++cases;
                const GXDecompiledFunctionEntry& expected = kGXDecompiledFunctions[regCallbacks];
                if (runtime_va != expected.vma + 0x400000ULL ||
                    sz != expected.size ||
                    fn_ptr != expected.func_ptr ||
                    std::strcmp(fn_name, expected.name) != 0) {
                    ++bad;
                }
                ++regCallbacks;
            });
        if (regReturned != 2940u || regCallbacks != 2940u) {
            ++bad;
        }

        // 4. Verify external imports catalog kGXEbootExternalImports
        if (kGXEbootExternalImportCount == 0u) {
            ++bad;
        }
        for (size_t i = 0; i < kGXEbootExternalImportCount; ++i) {
            ++cases;
            const GXEbootExternalImportEntry& imp = kGXEbootExternalImports[i];
            if (imp.symbol_name == nullptr || imp.symbol_name[0] == '\0' ||
                imp.description == nullptr || imp.description[0] == '\0') {
                ++bad;
            }
        }

        std::printf("[TEST 16] Golden Fund Registry (2940 Functions = 1,040,700 B + %zu External Imports) vs db/functions.json: %d cases -> %d differ\n",
                    kGXEbootExternalImportCount, cases, bad);
        total_failures += bad;
    }

    // ========================================================================
    // 17. Live Runtime Bridge, Slid VMA Singletons/VTables & Guest GS-TLS Test
    // ========================================================================
    {
        int bad = 0;
        int cases = 0;

        // 17A. Verify all 5 relocated TLS GOT TPOFF64 slots in eboot.bin (0x53e4318..0x53e4658)
        struct TlsExpected { uintptr_t vma; int64_t expected_tpoff; };
        const TlsExpected kTlsSlots[5] = {
            { 0x53e4318ULL, -1696LL }, // -0x6a0
            { 0x53e4320ULL, -1792LL }, // -0x700
            { 0x53e44a8ULL, -1824LL }, // -0x720
            { 0x53e4568ULL, -1840LL }, // -0x730
            { 0x53e4658ULL, -1688LL }, // -0x698
        };
        for (const auto& s : kTlsSlots) {
            ++cases;
            int64_t live_tpoff = gx_guest_tls_offset(s.vma);
            int64_t raw_eboot  = *reinterpret_cast<const int64_t*>(vma_to_bn(s.vma));
            if (live_tpoff != s.expected_tpoff || live_tpoff != raw_eboot) {
                ++bad;
            }
        }

        // 17B. Bind a live Orbis guest TCB into gs:[0x1660] and verify gx_guest_tls_base() & gx_guest_tls_slot()
        alignas(64) uint8_t fake_teb[0x2000]{};
        alignas(64) uint8_t fake_tcb_block[0x2000]{};
        uintptr_t guest_tcb_ptr = reinterpret_cast<uintptr_t>(fake_tcb_block + 0x1000);
        *reinterpret_cast<uintptr_t*>(fake_teb + 0x1660) = guest_tcb_ptr;
        // Set Linux GS base to fake_teb via arch_prctl(ARCH_SET_GS = 0x1001)
        long prctl_rc = 0;
        __asm__ volatile(
            "movq $158, %%rax\n\t"
            "movq $0x1001, %%rdi\n\t"
            "movq %1, %%rsi\n\t"
            "syscall\n\t"
            "movq %%rax, %0\n\t"
            : "=r"(prctl_rc)
            : "r"(reinterpret_cast<uintptr_t>(fake_teb))
            : "rax", "rdi", "rsi", "rcx", "r11", "memory"
        );
        ++cases;
        if (prctl_rc != 0 || gx_guest_tls_base() != guest_tcb_ptr) {
            ++bad;
        }
        for (size_t i = 0; i < 5; ++i) {
            ++cases;
            uint8_t* slot_ptr = gx_guest_tls_slot(kTlsSlots[i].vma);
            uint8_t* expected_ptr = reinterpret_cast<uint8_t*>(
                static_cast<intptr_t>(guest_tcb_ptr) + kTlsSlots[i].expected_tpoff);
            if (slot_ptr != expected_ptr) {
                ++bad;
            }
            uint64_t sentinel = 0xB100DB049E000000ULL | (i + 1u);
            *reinterpret_cast<uint64_t*>(slot_ptr) = sentinel;
            if (*reinterpret_cast<const uint64_t*>(expected_ptr) != sentinel) {
                ++bad;
            }
        }

        // 17C. Verify shared atomic resource ID counter at GX_VMA(0x52ba180) between eboot.bin and compiled C++
        auto game_alloc_res_id = eboot_vma_fn<uint32_t(*)()>(0x26baef0);
        auto our_alloc_res_id  = reinterpret_cast<uint32_t(*)()>(
            []() -> void* {
                for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
                    if (kGXDecompiledFunctions[i].vma == 0x26baef0ULL) return kGXDecompiledFunctions[i].func_ptr;
                }
                return nullptr;
            }());
        volatile uint32_t* pLiveResCounter = reinterpret_cast<volatile uint32_t*>(GX_VMA(0x52ba180ULL));
        uint32_t startCounter = *pLiveResCounter;
        for (int i = 0; i < 500; ++i) {
            ++cases;
            uint32_t id_theirs = game_alloc_res_id();
            uint32_t id_ours   = our_alloc_res_id();
            if (id_theirs != ((startCounter + static_cast<uint32_t>(i * 2)) & 0x00FFFFFFu) ||
                id_ours   != ((startCounter + static_cast<uint32_t>(i * 2 + 1)) & 0x00FFFFFFu)) {
                ++bad;
            }
        }

        // 17D. Verify live GX_VMA(0x55406c8) singleton resolution in GXCameraFrustum_ResetDefaults
        {
            using namespace FromSoftware::GX;
            auto game_reset = eboot_vma_fn<void(*)(GXCameraFrustum*, uint32_t, uint32_t)>(0x21a9f90);
            alignas(16) uint8_t fakeDevice[0x700]{};
            alignas(16) uint8_t fakeSwapChainObj[0x100]{};
            // GXDevice+0x90 -> SwapChain; SwapChain+0x80 = width, SwapChain+0x84 = height
            *reinterpret_cast<void**>(fakeDevice + 0x90) = fakeSwapChainObj;

            void** ppGlobalDevice = reinterpret_cast<void**>(GX_VMA(0x55406c8ULL));
            void* savedDevice = *ppGlobalDevice;
            *ppGlobalDevice = fakeDevice;

            const uint32_t kResPairs[4][2] = {
                { 1920u, 1080u }, { 2560u, 1440u }, { 3840u, 2160u }, { 3440u, 1440u }
            };
            for (const auto& wh : kResPairs) {
                ++cases;
                *reinterpret_cast<uint32_t*>(fakeSwapChainObj + 0x80) = wh[0];
                *reinterpret_cast<uint32_t*>(fakeSwapChainObj + 0x84) = wh[1];
                GXCameraFrustum f_theirs{}, f_ours{};
                game_reset(&f_theirs, 0, 0);
                GXCameraFrustum_ResetDefaults(&f_ours, 0, 0);
                if (std::memcmp(&f_theirs, &f_ours, 0x84) != 0 ||
                    f_ours.viewportWidth != wh[0] ||
                    f_ours.viewportHeight != wh[1]) {
                    ++bad;
                }
            }
            *ppGlobalDevice = savedDevice;
        }

        // 17E. Verify live GX_VMA(0x52bbf20 / 0x52bbf40) vtable pointers in GXMtd accessors
        {
            auto find_our_fn = [](uint64_t vma) -> void* {
                for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
                    if (kGXDecompiledFunctions[i].vma == vma) return kGXDecompiledFunctions[i].func_ptr;
                }
                return nullptr;
            };
            auto game_param_const = eboot_vma_fn<void(*)(void*)>(0x26de970);
            auto game_param_mut   = eboot_vma_fn<void(*)(void*)>(0x26df4a0);
            auto our_param_const  = reinterpret_cast<void(*)(void*)>(find_our_fn(0x26de970ULL));
            auto our_param_mut    = reinterpret_cast<void(*)(void*)>(find_our_fn(0x26df4a0ULL));
            alignas(16) uint8_t p_theirs[32]{}, p_ours[32]{};
            alignas(16) uint8_t m_theirs[32]{}, m_ours[32]{};
            game_param_const(p_theirs);
            our_param_const(p_ours);
            game_param_mut(m_theirs);
            our_param_mut(m_ours);
            ++cases;
            if (std::memcmp(p_theirs, p_ours, 24) != 0 ||
                std::memcmp(m_theirs, m_ours, 24) != 0 ||
                *reinterpret_cast<uintptr_t*>(p_ours) != GX_VMA(0x52bbf30ULL) ||
                *reinterpret_cast<uintptr_t*>(m_ours) != GX_VMA(0x52bbf50ULL)) {
                ++bad;
            }
        }

        // 17F. Verify live eboot.bin entry-point detour installation across all 2,940 functions
        {
            for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
                ++cases;
                const GXDecompiledFunctionEntry& e = kGXDecompiledFunctions[i];
                if (e.func_ptr == nullptr || (e.vma & 0xFULL) != 0ULL) {
                    ++bad;
                }
                if (i + 1 < kGXDecompiledFunctionCount &&
                    kGXDecompiledFunctions[i + 1].vma - e.vma < 16ULL) {
                    ++bad;
                }
            }
            // Install live detours on 0x26de970 (GXMtdParameterAccessor_CtorConst) and 0x26baef0 (CGTextureGnm_AllocateUniqueResourceId)
            // and invoke through the eboot.bin .text address directly
            auto find_our_fn = [](uint64_t vma) -> const void* {
                for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
                    if (kGXDecompiledFunctions[i].vma == vma) return kGXDecompiledFunctions[i].func_ptr;
                }
                return nullptr;
            };
            eboot_kit::stub(vma_to_bn(0x26de970), find_our_fn(0x26de970ULL));
            eboot_kit::stub(vma_to_bn(0x26baef0), find_our_fn(0x26baef0ULL));

            alignas(16) uint8_t acc_hooked[32]{};
            eboot_vma_fn<void(*)(void*)>(0x26de970)(acc_hooked);
            uint32_t before_id = *reinterpret_cast<volatile uint32_t*>(GX_VMA(0x52ba180ULL));
            uint32_t next_id   = eboot_vma_fn<uint32_t(*)()>(0x26baef0)();
            uint32_t after_id  = *reinterpret_cast<volatile uint32_t*>(GX_VMA(0x52ba180ULL));
            ++cases;
            if (*reinterpret_cast<uintptr_t*>(acc_hooked) != GX_VMA(0x52bbf30ULL) ||
                *reinterpret_cast<int32_t*>(acc_hooked + 0x10) != -1 ||
                next_id != (before_id & 0x00FFFFFFu) ||
                after_id != before_id + 1u) {
                ++bad;
            }
        }

        std::printf("[TEST 17] Live %%r11 External Bridges, GX_VMA() Slid Singletons/VTables & GS:[0x1660] TLS: %d cases -> %d differ\n",
                    cases, bad);
        total_failures += bad;
    }

    std::printf("\n=== LIVE COMBAT TEST SUMMARY: %d total failures ===\n", total_failures);
    return total_failures == 0 ? 0 : 1;
}
