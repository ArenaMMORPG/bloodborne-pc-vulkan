// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 04: SceneRenderer (Tier 8/10)
// File: GXYebisDirectorLifecycleAndSkinningPaletteSubmitter.cpp
// Range: 0x21cee10 .. 0x21d5520 (55 functions, 25,515 bytes)
// Direction: Right-to-Left (highest VMA 0x21d5520 down to lowest VMA 0x21cee10)
// ============================================================================
// Architectural Subsystems in this Translation Unit:
//   1. GXYebisPostProcessDirector External Parameter Appliers, .filterparam
//      Stream Serializer/Deserializer, & Lifecycle (0x21d5520 .. 0x21d1e10):
//      - ApplyExternalAntiAliasParams (0x21d5520)
//      - ApplyExternalDepthOfFieldParams (0x21d53a0)
//      - ApplyExternalLightShaftParams (0x21d52b0)
//      - ApplyExternalBloomParams (0x21d5050)
//      - ApplyExternalToneMapParams (0x21d4ee0)
//      - ApplyExternalSettingInfoParams (0x21d4d40) & SetGlobalFilterParamField2BC (0x21d4d30)
//      - ExecutePostProcessRenderPassWithCameraPair (0x21d4860)
//      - LoadFilterParamFileFromDebugGui (0x21d44f0) & SaveFilterParamFileFromDebugGui (0x21d4240)
//      - ResetFilterParamToDefault (0x21d4230) & SyncFeedBackBlurEnable (0x21d4210)
//      - InitializeContextAndLutResources (0x21d2dd0, 5,184 B: TLSF 2-level segregated fit
//        allocator initialization with lzcnt bin indexing, Yebis2 PPFX context creation,
//        double-buffered GPU command buffers/fences, 3D/2D LUT & CustomDof UAV creation)
//      - FinalizeContextAndLutResources (0x21d2830), Destructor (0x21d24e0 / 0x21d24c0),
//        Constructor (0x21d1f80), & Yebis2 TLSF Custom Heap Callbacks (0x21d1f00 / 0x21d1e40)
//   2. GXModelMeshDrawAndSkinningPaletteSubmitter (0x21d1d30 .. 0x21cee10):
//      - Global DoF FocusDistance / Enable override setters (0x21d1d30 .. 0x21d1cb0)
//      - UploadPreviousFrameSkinningBonePalette (0x21d1650, 1,488 B: SIMD 3x4 affine bone
//        matrix multiplication in thread-local DLLinearHeap with FreeUntil mark check)
//      - UploadCurrentFrameSkinningBonePalette (0x21d1080, 1,488 B: SIMD 3x4 affine bone
//        matrix multiplication in thread-local DLLinearHeap with FreeUntil mark check)
//      - SubmitMeshGroupDrawElements (0x21d0640, 2,621 B: motion-blur previous/current
//        bone binding, vertex stream binding, index buffer setup, & DrawIndexed dispatch)
//      - CullSubmeshBoundingBoxesAgainstFrustum (0x21d0330, 774 B)
//      - DispatchModelPassDrawList (0x21cfb30, 1,640 B) & wrappers (0x21d02f0 .. 0x21cfa50)
//      - InitializeModelMeshGpuBuffersAndStreams (0x21cf3c0, 1,200 B) & CB setup (0x21cf1c0)
//      - Model Mesh Resource Lifecycle & Stream Accessors (0x21cf130 .. 0x21cee10)
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include "gx_direct_vulkan.h"

namespace {

// ----------------------------------------------------------------------------
// Read-Only Constants (.rodata) verified from eboot.bin (vma + 0x4000)
// ----------------------------------------------------------------------------
constexpr float kDegToRad_492978c       = 0.01745329238474369f;  // 0x3c8efa35 (pi / 180)
constexpr float kMinGlareLength_4929784 = 0.0020000000949949026f;// 0x3b03126f
constexpr float kMaxGlareLength_4929788 = 0.10000000149011612f;  // 0x3dcccccd
constexpr float kYebisParam100_4929760  = 100.0f;                // 0x42c80000
constexpr float kYebisKeyVal_4929764    = 0.18000000715255737f;  // 0x3e3851ec
constexpr float kOne_4929768            = 1.0f;                  // 0x3f800000
constexpr float kFifteen_492976c        = 15.0f;                 // 0x41700000
constexpr float kTwoFiftyFive_4929770   = 255.0f;                // 0x437f0000
constexpr float kMinusOne_4929774       = -1.0f;                 // 0xbf800000
constexpr float kNegZero_4929778        = -0.0f;                 // 0x80000000
constexpr float kSixteen_492977c        = 16.0f;                 // 0x41800000
constexpr float kMinus256_4929780       = -256.0f;               // 0xc3800000

alignas(16) constexpr uint32_t kMaskXYZ_4920cc0[4] = {
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u
};
alignas(16) constexpr float kUnitW_4920cd0[4] = {
    0.0f, 0.0f, 0.0f, 1.0f
};
alignas(16) constexpr float kNegPoint05W1_4920ce0[4] = {
    -0.05000000074505806f, -0.05000000074505806f, -0.05000000074505806f, 1.0f
};
alignas(16) constexpr float kPosPoint05W1_4920cf0[4] = {
    0.05000000074505806f, 0.05000000074505806f, 0.05000000074505806f, 1.0f
};
alignas(16) constexpr float kAllOnes_4920d50[4] = {
    1.0f, 1.0f, 1.0f, 1.0f
};
alignas(16) constexpr float kIdentityMatrix4x4_5128a40[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

// Table @ 0x4738fc4..0x4738fe0 (28 bytes) for quad vertex buffer creation
struct QuadVbDesc_4738fc4 {
    uint64_t q0; // 0x0000000200000060ULL
    uint64_t q1; // 0x0000000100000001ULL
    uint64_t q2; // 0x0000000000000000ULL
    uint32_t d3; // 0x00000000U
};
constexpr QuadVbDesc_4738fc4 kQuadVbDesc_4738fc4 = {
    0x0000000200000060ULL,
    0x0000000100000001ULL,
    0x0000000000000000ULL,
    0x00000000U
};

// Cull-mode / winding remap table @ 0x4738fa0 (3x3 uint32_t)
constexpr uint32_t kCullModeRemap_4738fa0[3][3] = {
    { 0u, 1u, 2u },
    { 0u, 2u, 1u },
    { 0u, 0u, 0u }
};

#define kErrIncompatibleHeap_4737dc0   (reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)))
#define kKernelSourcePath_493bac3      (reinterpret_cast<const char*>(GX_VMA(0x493bac3ULL)))
#define kPthreadMutexPath_493c20e      (reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)))
#define kLinearHeapPath_493c62d        (reinterpret_cast<const char*>(GX_VMA(0x493c62dULL)))
#define kErrHeapNotFound_4940207       (reinterpret_cast<const char*>(GX_VMA(0x4940207ULL)))
#define kErrInvalidAttr_493c275        (reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)))
#define kErrUnknown_493c2ae            (reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)))
#define kErrLinearHeapMark_493c690     (reinterpret_cast<const char*>(GX_VMA(0x493c690ULL)))
#define kColorGradingLutPrefix_49c574e (reinterpret_cast<const char16_t*>(GX_VMA(0x49c574eULL)))

} // namespace

// ----------------------------------------------------------------------------
// External Engine Globals & Cross-Module Declarations
// ----------------------------------------------------------------------------
extern "C" {

#undef g_GlobalYebisFilterParam_55406e0
#define g_GlobalYebisFilterParam_55406e0 (reinterpret_cast<uint8_t*>(GX_VMA(0x55406e0ULL)))
#undef g_GlobalYebisSavedFilterParam_5540a50
#define g_GlobalYebisSavedFilterParam_5540a50 (reinterpret_cast<uint8_t*>(GX_VMA(0x5540a50ULL)))
#undef g_GlobalDofDisableOverride_5540dd0
#define g_GlobalDofDisableOverride_5540dd0 (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540dd0ULL)))
#undef g_GlobalMeshPassFlag_5540dd5
#define g_GlobalMeshPassFlag_5540dd5 (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540dd5ULL)))
#undef g_GlobalSceneRendererContext_55406c8
#define g_GlobalSceneRendererContext_55406c8 (*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))
#undef g_GlobalStringHeap_5540670
#define g_GlobalStringHeap_5540670 (*reinterpret_cast<void**>(GX_VMA(0x5540670ULL)))
#undef g_GlobalHeapAllocator_5540698
#define g_GlobalHeapAllocator_5540698 (*reinterpret_cast<void**>(GX_VMA(0x5540698ULL)))
#undef g_GlobalGfxAllocator_55406a8
#define g_GlobalGfxAllocator_55406a8 (*reinterpret_cast<void**>(GX_VMA(0x55406a8ULL)))
#undef g_GlobalRenderSystem_5540dd8
#define g_GlobalRenderSystem_5540dd8 (*reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL)))
#undef g_GlobalOverrideCallbackA_5540dc0
#define g_GlobalOverrideCallbackA_5540dc0 (*reinterpret_cast<void**>(GX_VMA(0x5540dc0ULL)))
#undef g_GlobalOverrideCallbackB_5540dc8
#define g_GlobalOverrideCallbackB_5540dc8 (*reinterpret_cast<void**>(GX_VMA(0x5540dc8ULL)))

// Yebis2 static context & double-buffered GNM ring/fence globals
#undef g_YebisStaticLock_56a6d00
#define g_YebisStaticLock_56a6d00 (reinterpret_cast<uint8_t*>(GX_VMA(0x56a6d00ULL)))
#undef g_YebisStaticHeap_56a6d08
#define g_YebisStaticHeap_56a6d08 (*reinterpret_cast<void**>(GX_VMA(0x56a6d08ULL)))
#undef g_YebisStaticInitFlags_56a6d10
#define g_YebisStaticInitFlags_56a6d10 (*reinterpret_cast<uint16_t*>(GX_VMA(0x56a6d10ULL)))
#undef g_ActiveYebisDirector_56a6d18
#define g_ActiveYebisDirector_56a6d18 (*reinterpret_cast<void**>(GX_VMA(0x56a6d18ULL)))
#undef g_YebisContextSlotIdx_56a6d20
#define g_YebisContextSlotIdx_56a6d20 (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6d20ULL)))
#undef g_YebisGnmCtx0_56a6d28
#define g_YebisGnmCtx0_56a6d28 (reinterpret_cast<uint8_t*>(GX_VMA(0x56a6d28ULL)))
#undef g_YebisGnmSubCtx0_56a6da8
#define g_YebisGnmSubCtx0_56a6da8 (reinterpret_cast<uint8_t*>(GX_VMA(0x56a6da8ULL)))
#undef g_YebisGnmCtx1_56b54e0
#define g_YebisGnmCtx1_56b54e0 (reinterpret_cast<uint8_t*>(GX_VMA(0x56b54e0ULL)))
#undef g_YebisGnmSubCtx1_56b5560
#define g_YebisGnmSubCtx1_56b5560 (reinterpret_cast<uint8_t*>(GX_VMA(0x56b5560ULL)))

#undef g_YebisCtxPtr0_56c3c98
#define g_YebisCtxPtr0_56c3c98 (*reinterpret_cast<void**>(GX_VMA(0x56c3c98ULL)))
#undef g_YebisCtxPtr1_56c3ca0
#define g_YebisCtxPtr1_56c3ca0 (*reinterpret_cast<void**>(GX_VMA(0x56c3ca0ULL)))
#undef g_YebisSubCtxPtr0_56c3ca8
#define g_YebisSubCtxPtr0_56c3ca8 (*reinterpret_cast<void**>(GX_VMA(0x56c3ca8ULL)))
#undef g_YebisSubCtxPtr1_56c3cb0
#define g_YebisSubCtxPtr1_56c3cb0 (*reinterpret_cast<void**>(GX_VMA(0x56c3cb0ULL)))
#undef g_YebisActiveRingBase_56c3cb8
#define g_YebisActiveRingBase_56c3cb8 (*reinterpret_cast<void**>(GX_VMA(0x56c3cb8ULL)))
#undef g_YebisActiveRingCopy_56c3cc0
#define g_YebisActiveRingCopy_56c3cc0 (*reinterpret_cast<void**>(GX_VMA(0x56c3cc0ULL)))
#undef g_YebisActiveRingPlus80_56c3cc8
#define g_YebisActiveRingPlus80_56c3cc8 (*reinterpret_cast<void**>(GX_VMA(0x56c3cc8ULL)))
#undef g_YebisActiveRingQword0_56c3cd0
#define g_YebisActiveRingQword0_56c3cd0 (*reinterpret_cast<void**>(GX_VMA(0x56c3cd0ULL)))
#undef g_YebisActiveRingQword80_56c3cd8
#define g_YebisActiveRingQword80_56c3cd8 (*reinterpret_cast<void**>(GX_VMA(0x56c3cd8ULL)))
#undef g_YebisRingBuffersAllocated_56c3ce0
#define g_YebisRingBuffersAllocated_56c3ce0 (*reinterpret_cast<uint8_t*>(GX_VMA(0x56c3ce0ULL)))
#undef g_YebisFenceCounter_56c3ce8
#define g_YebisFenceCounter_56c3ce8 (*reinterpret_cast<uint64_t*>(GX_VMA(0x56c3ce8ULL)))
#undef g_YebisFenceLabel0_56c3cf0
#define g_YebisFenceLabel0_56c3cf0 (*reinterpret_cast<void**>(GX_VMA(0x56c3cf0ULL)))
#undef g_YebisFenceLabel1_56c3cf8
#define g_YebisFenceLabel1_56c3cf8 (*reinterpret_cast<void**>(GX_VMA(0x56c3cf8ULL)))
#undef g_YebisRingBuf0_A_56c3d00
#define g_YebisRingBuf0_A_56c3d00 (*reinterpret_cast<void**>(GX_VMA(0x56c3d00ULL)))
#undef g_YebisRingBuf1_A_56c3d08
#define g_YebisRingBuf1_A_56c3d08 (*reinterpret_cast<void**>(GX_VMA(0x56c3d08ULL)))
#undef g_YebisRingBuf0_B_56c3d10
#define g_YebisRingBuf0_B_56c3d10 (*reinterpret_cast<void**>(GX_VMA(0x56c3d10ULL)))
#undef g_YebisRingBuf1_B_56c3d18
#define g_YebisRingBuf1_B_56c3d18 (*reinterpret_cast<void**>(GX_VMA(0x56c3d18ULL)))
#undef g_YebisRingBuf0_C_56c3d20
#define g_YebisRingBuf0_C_56c3d20 (*reinterpret_cast<void**>(GX_VMA(0x56c3d20ULL)))
#undef g_YebisRingBuf1_C_56c3d28
#define g_YebisRingBuf1_C_56c3d28 (*reinterpret_cast<void**>(GX_VMA(0x56c3d28ULL)))
#undef g_YebisRingBuf0_D_56c3d30
#define g_YebisRingBuf0_D_56c3d30 (*reinterpret_cast<void**>(GX_VMA(0x56c3d30ULL)))
#undef g_YebisRingBuf1_D_56c3d38
#define g_YebisRingBuf1_D_56c3d38 (*reinterpret_cast<void**>(GX_VMA(0x56c3d38ULL)))

#undef g_VtableFileStreamReader_53b29d0
#define g_VtableFileStreamReader_53b29d0 (reinterpret_cast<void**>(GX_VMA(0x53b29d0ULL)))
#undef g_VtableMutexLock_53b2fa0
#define g_VtableMutexLock_53b2fa0 (reinterpret_cast<void**>(GX_VMA(0x53b2fa0ULL)))
#undef g_VtableTlsfAllocator_52e70b0
#define g_VtableTlsfAllocator_52e70b0 (reinterpret_cast<void**>(GX_VMA(0x52e70b0ULL)))
#undef g_VtableYebisDirector_535fe90
#define g_VtableYebisDirector_535fe90 (reinterpret_cast<void**>(GX_VMA(0x535fe90ULL)))
#undef g_VtableMeshPassCallback_535f520
#define g_VtableMeshPassCallback_535f520 (reinterpret_cast<void**>(GX_VMA(0x535f520ULL)))
#undef g_VtableMeshDrawSubmitter_535fe70
#define g_VtableMeshDrawSubmitter_535fe70 (reinterpret_cast<void**>(GX_VMA(0x535fe70ULL)))
#undef g_VtableModelMeshResource_53b6f60
#define g_VtableModelMeshResource_53b6f60 (reinterpret_cast<void**>(GX_VMA(0x53b6f60ULL)))

// Thread-local heap pointers resolved via TLS
void* sub_207d250();
#define g_TlsLinearHeap_53e4568 (*reinterpret_cast<void**>(gx_guest_tls_slot(0x53e4568ULL)))
#define g_TlsHeapCache_53e44a8 (reinterpret_cast<uint8_t*>(gx_guest_tls_slot(0x53e44a8ULL)))

// External engine helpers
void* sub_e45560(void* allocator, uint64_t size, uint64_t align);
void  sub_228e6a0(void* renderer, void* passCtx, void* cameraPairDesc);
void  sub_2289bb0(void* targetMgr, uint32_t mask, uint32_t a3, uint32_t a4);
void  sub_20b55b0(const void* file, uint32_t line, const char* msg, ...);
void  sub_2374ee0(void* guiCtrl, void* outWString, uint32_t flag);
void* sub_2ba2b10();
void  sub_2079230(void* pathObj, const void* wstrPtr, uint32_t a3, uint32_t a4, uint32_t a5);
void  sub_20761a0(void* readerObj, void* pathObj, uint32_t mode);
void  sub_2076420(void* readerObj, void* dstBuf, uint64_t byteCount);
void  sub_2079910(void* pathObj);
void  sub_2076cb0(void* writerObj, void* pathObj, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6);
void  sub_20770d0(void* writerObj, const void* srcBuf, uint64_t byteCount);
void  sub_2076f70(void* writerObj);
void  sub_21df510(void* filterParam);
void  sub_bb6e60(void* staticLock);
int32_t sub_2083fe0(void* mutexBody);
void* sub_207bbf0();
void  sub_207c000(void* heapReg, void* tlsfHeap, void* memBase, uint64_t memSize);
void* sub_bb4ac0();
uint32_t sub_1091210();
uint32_t sub_10911a0(uint32_t arg);
void  sub_10945f0(void* gnmCtx, void* bufA, void* bufB, uint32_t numBuffers, void* bufC, uint32_t sizeC, void* bufD, uint32_t sizeD);
void  sub_10946d0(void* ringBase);
void  sub_bb4d70(void* yebisCtx, uint32_t arg);
void  sub_bb4de0(void* yebisCtx, uint32_t slotIdx);
void  sub_bb4b40(void* yebisCtx, void* initCallbacks, uint32_t width, uint32_t height, uint32_t flags, void* initParams);
void  sub_bb26e0(void* yebisCtx, uint32_t arg);
void  sub_bb26f0(void* yebisCtx, uint32_t arg);
void  sub_bb3330(void* yebisCtx, uint32_t mask, float val);
void  sub_bb31a0(void* yebisCtx, uint32_t mask, float keyVal, float oneVal);
int32_t sub_2165c10(void* dev, void* outBuffer, const void* bufDesc, const void* initData, uint32_t a5, uint32_t a6, void* a7);
int32_t sub_2166300(void* dev, void* outTex3D, const void* tex3dDesc, const void* initData, uint32_t a5, uint32_t a6, void* a7);
int32_t sub_2165f90(void* dev, void* outTex2D, const void* tex2dDesc, const void* initData, uint32_t a5, uint32_t a6, void* a7);
int32_t sub_21669a0(void* dev, void* outSrv, void* resource, const void* srvDesc, uint32_t a5);
int32_t sub_2166b60(void* dev, void* outUav, void* resource, const void* uavDesc, uint32_t a5);
void* sub_207b720(void* ptr);
void  sub_207b8c0(void* tlsfHeap);
uint32_t sub_2bbfee8(void* mutexBody);
void  sub_2bbe588(void* ptr);
uint32_t sub_207b9f0();
uint64_t sub_2bbf228(const char16_t* str);
void  sub_262a720(void* wstrObj, const char16_t* str, uint64_t len);
void* sub_2085a60(void* tlsfBody, uint64_t size, uint64_t align);
void* sub_21d1e40(uint32_t size, uint32_t align);
void  sub_21d1f00(void* ptr);
void  sub_21d2830(void* self);
void  sub_21d24e0(void* self);
void  sub_21d2dd0(void* self);
void  sub_21d1080(void* modelRes, void* drawContext, void* meshGroup, const float* bonePalette);
void  sub_21d1650(void* modelRes, void* drawContext, void* meshGroup, const float* prevBonePalette);
void  sub_21d0640(void* modelRes, void* drawContext, void* meshEntry, void* materialEntry, void* skinningInfo, uint32_t instanceCount);
uint32_t sub_21cfb30(void* modelRes, void* materialResolver, void* drawContext, uint32_t passCount, void** passArray, uint8_t skipPrePass);
uint8_t sub_21cf1c0(void* modelRes, void* flverContainer);
void  sub_2169780(void* cmdList, void* drawArgs, uint32_t indexCount, uint32_t startIndex, uint32_t baseVertex);
void  sub_2169900(void* cmdList, void* drawArgs, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, uint32_t baseVertex);
void* sub_26db050(void* arg);
uint8_t sub_2191c30(void* cullMgr, void* a2, uint32_t a3, uint32_t a4);
void  sub_2192170(void* cullMgr, void* a2, void* frustumPlanes, uint32_t a4);
void  sub_21900a0(void* cullCtx, const void* worldMatCols, const void* aabbMinMax, uint32_t a4, uint32_t a5, uint32_t a6);
void  sub_21ad270(void* mgr, void* bufferRes);
void  sub_21ace70(void* mgr, void* bufferRes);
void  sub_21c6ae0(void* self);
void  sub_21c6410(void* self);
void  sub_21c60f0(void* self, void* alloc);
void  sub_21c67a0(void* self, void* arg);
uint8_t sub_21ce5d0(void* stackDesc, void* rawFlverData);
void  sub_e51f00(void* dst, void* src);
void  sub_e51350(void* dst, void* src);
void  sub_e512b0(void* dst, uint32_t count);
void  sub_e516e0(void* dst, void* a2, void* a3);
void  sub_21cdcd0(void* dst, void* a2, void* a3);
void  sub_21c7660(void* dst, void* a2, void* a3, void* a4);
void  sub_e50490(void* dst, void* src);
void  sub_e4de20(void* dst, void* a2, void* a3, void* a4);
void  sub_e4efb0(void* dst, void* a2, void* a3, uint32_t a4, uint32_t a5);
void  sub_e52950(void* dst, void* a2, void* a3, uint32_t a4, uint32_t a5);
void  sub_e4faa0(void* dst, void* a2, void* a3);
void  sub_261a1b0(void* dst, const void* src, uint64_t size);
uint8_t sub_e4f630(void* dst, void* a2, void* a3);
uint8_t sub_e52fe0(void* dst, void* a2, void* a3);

// ============================================================================
// 1. 0x21d5520 (56 B) - GXYebisPostProcessDirector_ApplyExternalAntiAliasParams
// ============================================================================
void sub_21d5520(void* self, const uint32_t* aaParams) {
    auto* dir = static_cast<uint8_t*>(self);
    if (dir[0x2710] != 0) {
        return;
    }
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    if (aaParams[0] != 0 && aaParams[1] == 0) {
        *reinterpret_cast<uint32_t*>(fp + 0x220) = 1u;
    } else {
        *reinterpret_cast<uint32_t*>(fp + 0x220) = 0u;
    }
}

// ============================================================================
// 2. 0x21d53a0 (369 B) - GXYebisPostProcessDirector_ApplyExternalDepthOfFieldParams
// ============================================================================
void sub_21d53a0(void* self, const uint8_t* dofParams) {
    auto* dir = static_cast<uint8_t*>(self);
    if (dir[0x2710] != 0) {
        return;
    }
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    uint32_t enableFlag = 0;
    if (dir[0x2712] != 0) {
        enableFlag = *reinterpret_cast<const uint32_t*>(dofParams + 0x60);
        *reinterpret_cast<uint32_t*>(fp + 0x188) = enableFlag;
    } else {
        enableFlag = *reinterpret_cast<const uint32_t*>(fp + 0x188);
    }

    *reinterpret_cast<uint32_t*>(fp + 0x18c) = *reinterpret_cast<const uint32_t*>(dofParams + 0x64);
    std::memcpy(fp + 0x190, dofParams + 0x68, 16);
    *reinterpret_cast<float*>(fp + 0x1a0) = *reinterpret_cast<const float*>(dofParams + 0x78);
    *reinterpret_cast<float*>(fp + 0x1a4) = *reinterpret_cast<const float*>(dofParams + 0x7c);
    *reinterpret_cast<float*>(fp + 0x1a8) = *reinterpret_cast<const float*>(dofParams + 0x80);
    *reinterpret_cast<uint32_t*>(fp + 0x1ac) = *reinterpret_cast<const uint32_t*>(dofParams + 0x00);
    *reinterpret_cast<float*>(fp + 0x1c0) = *reinterpret_cast<const float*>(dofParams + 0x0c);
    *reinterpret_cast<float*>(fp + 0x1b4) = *reinterpret_cast<const float*>(dofParams + 0x04);

    if (enableFlag == 0 && dir[0x2711] != 0) {
        *reinterpret_cast<float*>(fp + 0x1b0) = *reinterpret_cast<const float*>(dofParams + 0x08);
    }

    *reinterpret_cast<float*>(fp + 0x1b8) = *reinterpret_cast<const float*>(dofParams + 0x10);
    *reinterpret_cast<float*>(fp + 0x1bc) = *reinterpret_cast<const float*>(dofParams + 0x14);
    *reinterpret_cast<float*>(fp + 0x1c4) = *reinterpret_cast<const float*>(dofParams + 0x18);
    *reinterpret_cast<float*>(fp + 0x1c8) = *reinterpret_cast<const float*>(dofParams + 0x1c);
    *reinterpret_cast<uint32_t*>(fp + 0x1cc) = *reinterpret_cast<const uint32_t*>(dofParams + 0x28);
    *reinterpret_cast<uint32_t*>(fp + 0x1d0) = *reinterpret_cast<const uint32_t*>(dofParams + 0x20);
    *reinterpret_cast<uint32_t*>(fp + 0x1d4) = *reinterpret_cast<const uint32_t*>(dofParams + 0x24);
    *reinterpret_cast<uint32_t*>(fp + 0x240) = *reinterpret_cast<const uint32_t*>(dofParams + 0x2c);

    uint32_t customDofEnable = 0;
    if (g_GlobalDofDisableOverride_5540dd0 == 0) {
        customDofEnable = *reinterpret_cast<const uint32_t*>(dofParams + 0x30);
    }
    *reinterpret_cast<uint32_t*>(fp + 0x2d0) = customDofEnable;
    *reinterpret_cast<uint32_t*>(fp + 0x2d4) = *reinterpret_cast<const uint32_t*>(dofParams + 0x34);
    *reinterpret_cast<uint32_t*>(fp + 0x2d8) = *reinterpret_cast<const uint32_t*>(dofParams + 0x48);
    std::memcpy(fp + 0x2dc, dofParams + 0x38, 16);
    std::memcpy(fp + 0x2ec, dofParams + 0x4c, 16);
    *reinterpret_cast<float*>(fp + 0x2fc) = *reinterpret_cast<const float*>(dofParams + 0x5c);
    *reinterpret_cast<float*>(fp + 0x1d8) = *reinterpret_cast<const float*>(dofParams + 0x84);
}

// ============================================================================
// 3. 0x21d52b0 (237 B) - GXYebisPostProcessDirector_ApplyExternalLightShaftParams
// ============================================================================
void sub_21d52b0(void* self, const uint8_t* lsParams) {
    auto* dir = static_cast<uint8_t*>(self);
    if (dir[0x2710] != 0) {
        return;
    }
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    *reinterpret_cast<uint32_t*>(fp + 0x130) = *reinterpret_cast<const uint32_t*>(lsParams + 0x00);
    *reinterpret_cast<float*>(fp + 0x144) = *reinterpret_cast<const float*>(lsParams + 0x20);
    *reinterpret_cast<float*>(fp + 0x148) = *reinterpret_cast<const float*>(lsParams + 0x2c);
    *reinterpret_cast<float*>(fp + 0x14c) = *reinterpret_cast<const float*>(lsParams + 0x28);
    *reinterpret_cast<uint32_t*>(fp + 0x150) = *reinterpret_cast<const uint32_t*>(lsParams + 0x24);
    std::memcpy(fp + 0x154, lsParams + 0x30, 16);
    *reinterpret_cast<float*>(fp + 0x164) = *reinterpret_cast<const float*>(lsParams + 0x40);
    *reinterpret_cast<float*>(fp + 0x24c) = *reinterpret_cast<const float*>(lsParams + 0x44);
    std::memcpy(fp + 0x168, lsParams + 0x48, 16);

    const float* colA = reinterpret_cast<const float*>(lsParams + 0x10);
    const float* colB = reinterpret_cast<const float*>(lsParams + 0x60);
    *reinterpret_cast<float*>(fp + 0x134) = colA[0];
    *reinterpret_cast<float*>(fp + 0x138) = colA[1];
    *reinterpret_cast<float*>(fp + 0x13c) = colA[2];
    *reinterpret_cast<float*>(fp + 0x140) = colA[3];
    std::memcpy(fp + 0x178, colB, 16);

    std::memcpy(dir + 0x2680, colA, 16);
    std::memcpy(dir + 0x2690, colA, 16);
    std::memcpy(dir + 0x26a0, colB, 16);
    std::memcpy(dir + 0x26b0, colB, 16);
}

// ============================================================================
// 4. 0x21d5050 (606 B) - GXYebisPostProcessDirector_ApplyExternalBloomParams
// ============================================================================
void sub_21d5050(void* self, const uint8_t* bloomParams) {
    auto* dir = static_cast<uint8_t*>(self);
    if (dir[0x2710] != 0) {
        return;
    }
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    *reinterpret_cast<uint32_t*>(fp + 0x0d8) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x20);
    *reinterpret_cast<float*>(fp + 0x0dc) = *reinterpret_cast<const float*>(bloomParams + 0x00);
    *reinterpret_cast<float*>(fp + 0x0e0) = *reinterpret_cast<const float*>(bloomParams + 0x04);
    *reinterpret_cast<uint32_t*>(fp + 0x0e4) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x24);
    *reinterpret_cast<float*>(fp + 0x0e8) = *reinterpret_cast<const float*>(bloomParams + 0x10);
    *reinterpret_cast<float*>(fp + 0x0ec) = *reinterpret_cast<const float*>(bloomParams + 0x18);
    *reinterpret_cast<float*>(fp + 0x0f0) = *reinterpret_cast<const float*>(bloomParams + 0x14);
    *reinterpret_cast<float*>(fp + 0x0f4) = *reinterpret_cast<const float*>(bloomParams + 0x1c);
    *reinterpret_cast<float*>(fp + 0x0f8) = *reinterpret_cast<const float*>(bloomParams + 0x28);
    *reinterpret_cast<float*>(fp + 0x0fc) = *reinterpret_cast<const float*>(bloomParams + 0x2c);
    *reinterpret_cast<float*>(fp + 0x100) = *reinterpret_cast<const float*>(bloomParams + 0x30);
    *reinterpret_cast<uint32_t*>(fp + 0x104) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x34);
    *reinterpret_cast<float*>(fp + 0x108) = *reinterpret_cast<const float*>(bloomParams + 0x38);
    *reinterpret_cast<float*>(fp + 0x10c) = *reinterpret_cast<const float*>(bloomParams + 0x3c);

    const float* tint0 = reinterpret_cast<const float*>(bloomParams + 0x40);
    const float* tint1 = reinterpret_cast<const float*>(bloomParams + 0x50);
    *reinterpret_cast<float*>(fp + 0x110) = tint0[0];
    *reinterpret_cast<float*>(fp + 0x120) = tint1[0];
    *reinterpret_cast<float*>(fp + 0x114) = tint0[1];
    *reinterpret_cast<float*>(fp + 0x124) = tint1[1];
    *reinterpret_cast<float*>(fp + 0x118) = tint0[2];
    *reinterpret_cast<float*>(fp + 0x128) = tint1[2];
    *reinterpret_cast<float*>(fp + 0x11c) = tint0[3];
    *reinterpret_cast<float*>(fp + 0x12c) = tint1[3];

    *reinterpret_cast<float*>(fp + 0x244) = *reinterpret_cast<const float*>(bloomParams + 0x0c);
    *reinterpret_cast<float*>(fp + 0x304) = *reinterpret_cast<const float*>(bloomParams + 0x08);

    std::memcpy(dir + 0x26d0, tint0, 16);
    std::memcpy(dir + 0x26f0, tint1, 16);
    std::memcpy(dir + 0x26c0, tint0, 16);
    std::memcpy(dir + 0x26e0, tint1, 16);

    *reinterpret_cast<float*>(fp + 0x308) = *reinterpret_cast<const float*>(bloomParams + 0x60);
    *reinterpret_cast<float*>(fp + 0x30c) = *reinterpret_cast<const float*>(bloomParams + 0x64);
    *reinterpret_cast<float*>(fp + 0x310) = *reinterpret_cast<const float*>(bloomParams + 0x68);
    *reinterpret_cast<uint32_t*>(fp + 0x314) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x6c);
    *reinterpret_cast<float*>(fp + 0x318) = *reinterpret_cast<const float*>(bloomParams + 0x70);
    *reinterpret_cast<float*>(fp + 0x31c) = *reinterpret_cast<const float*>(bloomParams + 0x74);
    *reinterpret_cast<float*>(fp + 0x320) = *reinterpret_cast<const float*>(bloomParams + 0x78);
    *reinterpret_cast<uint32_t*>(fp + 0x324) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x7c);
    *reinterpret_cast<float*>(fp + 0x328) = *reinterpret_cast<const float*>(bloomParams + 0x80);
    *reinterpret_cast<uint32_t*>(fp + 0x32c) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x84);
    *reinterpret_cast<float*>(fp + 0x330) = *reinterpret_cast<const float*>(bloomParams + 0x88);
    *reinterpret_cast<float*>(fp + 0x334) = *reinterpret_cast<const float*>(bloomParams + 0x8c);
    *reinterpret_cast<uint32_t*>(fp + 0x338) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x90);
    *reinterpret_cast<float*>(fp + 0x33c) =
        (*reinterpret_cast<const float*>(bloomParams + 0x94)) * kDegToRad_492978c;
    *reinterpret_cast<float*>(fp + 0x340) = *reinterpret_cast<const float*>(bloomParams + 0x98);
    *reinterpret_cast<uint32_t*>(fp + 0x344) = *reinterpret_cast<const uint32_t*>(bloomParams + 0x9c);
    *reinterpret_cast<float*>(fp + 0x348) = *reinterpret_cast<const float*>(bloomParams + 0xa0);
    *reinterpret_cast<float*>(fp + 0x34c) = *reinterpret_cast<const float*>(bloomParams + 0xa4);
}

// ============================================================================
// 5. 0x21d4ee0 (366 B) - GXYebisPostProcessDirector_ApplyExternalToneMapParams
// ============================================================================
void sub_21d4ee0(void* self, const uint8_t* tmParams) {
    auto* dir = static_cast<uint8_t*>(self);
    if (dir[0x2710] != 0) {
        return;
    }
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    const uint32_t autoExpEnable = *reinterpret_cast<const uint32_t*>(tmParams + 0x24);
    *reinterpret_cast<uint32_t*>(fp + 0x50) = autoExpEnable;
    std::memcpy(fp + 0x64, tmParams + 0x3c, 16);
    *reinterpret_cast<float*>(fp + 0x74) = *reinterpret_cast<const float*>(tmParams + 0x28);
    std::memcpy(fp + 0x78, tmParams + 0x4c, 16);
    *reinterpret_cast<float*>(fp + 0x88) = *reinterpret_cast<const float*>(tmParams + 0x5c);
    *reinterpret_cast<float*>(fp + 0x8c) = *reinterpret_cast<const float*>(tmParams + 0x60);
    *reinterpret_cast<float*>(fp + 0x90) = *reinterpret_cast<const float*>(tmParams + 0x64);

    if (autoExpEnable == 0) {
        *reinterpret_cast<float*>(fp + 0x98) = *reinterpret_cast<const float*>(tmParams + 0x00);
    }

    *reinterpret_cast<float*>(fp + 0x9c) = *reinterpret_cast<const float*>(tmParams + 0x04);
    *reinterpret_cast<uint32_t*>(fp + 0xa0) = *reinterpret_cast<const uint32_t*>(tmParams + 0x08);
    *reinterpret_cast<float*>(fp + 0xa4) = *reinterpret_cast<const float*>(tmParams + 0x0c);
    *reinterpret_cast<float*>(fp + 0xa8) = *reinterpret_cast<const float*>(tmParams + 0x10);
    *reinterpret_cast<uint32_t*>(fp + 0xac) = *reinterpret_cast<const uint32_t*>(tmParams + 0x70);
    *reinterpret_cast<uint32_t*>(fp + 0xb0) = *reinterpret_cast<const uint32_t*>(tmParams + 0x74);
    *reinterpret_cast<float*>(fp + 0xb4) = *reinterpret_cast<const float*>(tmParams + 0x78);
    *reinterpret_cast<float*>(fp + 0xbc) = *reinterpret_cast<const float*>(tmParams + 0x80);
    *reinterpret_cast<float*>(fp + 0xb8) = *reinterpret_cast<const float*>(tmParams + 0x7c);
    *reinterpret_cast<float*>(fp + 0xc0) = *reinterpret_cast<const float*>(tmParams + 0x84);
    *reinterpret_cast<float*>(fp + 0xc4) = *reinterpret_cast<const float*>(tmParams + 0x14);
    *reinterpret_cast<float*>(fp + 0xc8) = *reinterpret_cast<const float*>(tmParams + 0x18);
    *reinterpret_cast<float*>(fp + 0xcc) = *reinterpret_cast<const float*>(tmParams + 0x1c);
    *reinterpret_cast<float*>(fp + 0x248) = *reinterpret_cast<const float*>(tmParams + 0x20);
    *reinterpret_cast<float*>(fp + 0x54) = *reinterpret_cast<const float*>(tmParams + 0x2c);
    *reinterpret_cast<float*>(fp + 0x58) = *reinterpret_cast<const float*>(tmParams + 0x30);
    *reinterpret_cast<float*>(fp + 0x5c) = *reinterpret_cast<const float*>(tmParams + 0x34);
    *reinterpret_cast<float*>(fp + 0x60) = *reinterpret_cast<const float*>(tmParams + 0x38);
    *reinterpret_cast<float*>(fp + 0xd0) = *reinterpret_cast<const float*>(tmParams + 0x68);
    *reinterpret_cast<float*>(fp + 0xd4) = *reinterpret_cast<const float*>(tmParams + 0x6c);
    *reinterpret_cast<uint32_t*>(fp + 0x250) = *reinterpret_cast<const uint32_t*>(tmParams + 0x88);
}

// ============================================================================
// 6. 0x21d4d40 (410 B) - GXYebisPostProcessDirector_ApplyExternalSettingInfoParams
// ============================================================================
void sub_21d4d40(void* self, const uint8_t* settingParams) {
    auto* dir = static_cast<uint8_t*>(self);
    if (dir[0x2710] != 0) {
        return;
    }
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    uint8_t dirty = 0;

    auto updateU32 = [&](uint32_t fpOffset, uint32_t srcOffset) {
        const uint32_t val = *reinterpret_cast<const uint32_t*>(settingParams + srcOffset);
        uint32_t* dst = reinterpret_cast<uint32_t*>(fp + fpOffset);
        if (*dst != val) {
            *dst = val;
            dirty = 1;
        }
    };

    updateU32(0x08, 0x00);
    updateU32(0x0c, 0x04);
    updateU32(0x10, 0x08);
    updateU32(0x14, 0x0c);
    updateU32(0x18, 0x10);
    updateU32(0x1c, 0x14);
    updateU32(0x20, 0x18);
    updateU32(0x24, 0x1c);
    updateU32(0x28, 0x20);
    updateU32(0x30, 0x24);
    updateU32(0x34, 0x28);
    updateU32(0x38, 0x2c);
    updateU32(0x40, 0x30);
    updateU32(0x44, 0x34);
    updateU32(0x2b4, 0x38);

    *reinterpret_cast<float*>(fp + 0x94) = *reinterpret_cast<const float*>(settingParams + 0x50);

    updateU32(0x300, 0x3c);
    updateU32(0x48, 0x40);
    updateU32(0x4c, 0x44);

    dir[0x2718] = dirty;
    dir[0x20f8] = dirty;
}

// ============================================================================
// 7. 0x21d4d30 (14 B) - GXYebisPostProcessDirector_SetGlobalFilterParamField2BC
// ============================================================================
void sub_21d4d30(void* self, uint32_t val) {
    if (self != nullptr) {
        *reinterpret_cast<uint32_t*>(g_GlobalYebisFilterParam_55406e0 + 0x2bc) = val;
    } else {
        *reinterpret_cast<uint32_t*>(g_GlobalYebisFilterParam_55406e0 + 0x2bc) = val;
    }
}

// ============================================================================
// 8. 0x21d4860 (1,229 B) - GXYebisPostProcessDirector_ExecutePostProcessRenderPass
// ============================================================================
void sub_21d4860(
    void* passContext,
    void* sceneRenderer,
    uint32_t passFlagA,
    uint32_t passFlagB,
    void* resA,
    void* resB,
    void* resC,
    void* resD,
    void* resE,
    const uint8_t* prevCameraData)
{
    if (*reinterpret_cast<const uint32_t*>(g_GlobalYebisFilterParam_55406e0) == 0) {
        return;
    }

    auto* rendererBytes = static_cast<uint8_t*>(sceneRenderer);
    void* linearAlloc = *reinterpret_cast<void**>(rendererBytes + 0x450);
    uint8_t* packet = static_cast<uint8_t*>(
        sub_e45560(static_cast<uint8_t*>(linearAlloc) + 0x38, 0x4d0, 0x10));
    if (packet == nullptr) {
        return;
    }
    packet[0x408] = 0;
    packet[0x4a8] = 0;

    std::memcpy(packet, g_GlobalYebisFilterParam_55406e0, 0x370);
    *reinterpret_cast<void**>(packet + 0x370) = resA;
    *reinterpret_cast<void**>(packet + 0x390) = resB;

    const auto* curCam = *reinterpret_cast<const uint8_t* const*>(rendererBytes + 0x3f8);
    std::memcpy(packet + 0x3a0, curCam + 0x00, 80);
    *reinterpret_cast<float*>(packet + 0x3f0) = *reinterpret_cast<const float*>(curCam + 0x50);
    *reinterpret_cast<float*>(packet + 0x3f4) = *reinterpret_cast<const float*>(curCam + 0x54);
    *reinterpret_cast<float*>(packet + 0x3f8) = *reinterpret_cast<const float*>(curCam + 0x58);
    *reinterpret_cast<uint32_t*>(packet + 0x3fc) = *reinterpret_cast<const uint32_t*>(curCam + 0x5c);
    *reinterpret_cast<uint32_t*>(packet + 0x400) = *reinterpret_cast<const uint32_t*>(curCam + 0x60);
    *reinterpret_cast<float*>(packet + 0x404) = *reinterpret_cast<const float*>(curCam + 0x64);
    *reinterpret_cast<float*>(packet + 0x40c) = *reinterpret_cast<const float*>(curCam + 0x6c);
    std::memcpy(packet + 0x410, curCam + 0x70, 16);
    *reinterpret_cast<uint32_t*>(packet + 0x420) = *reinterpret_cast<const uint32_t*>(curCam + 0x80);

    *reinterpret_cast<void**>(packet + 0x380) = resC;
    *reinterpret_cast<void**>(packet + 0x388) = resD;
    *reinterpret_cast<void**>(packet + 0x430) = resE;

    std::memcpy(packet + 0x440, prevCameraData + 0x00, 80);
    *reinterpret_cast<float*>(packet + 0x490) = *reinterpret_cast<const float*>(prevCameraData + 0x50);
    *reinterpret_cast<float*>(packet + 0x494) = *reinterpret_cast<const float*>(prevCameraData + 0x54);
    *reinterpret_cast<float*>(packet + 0x498) = *reinterpret_cast<const float*>(prevCameraData + 0x58);
    *reinterpret_cast<uint32_t*>(packet + 0x49c) = *reinterpret_cast<const uint32_t*>(prevCameraData + 0x5c);
    *reinterpret_cast<uint32_t*>(packet + 0x4a0) = *reinterpret_cast<const uint32_t*>(prevCameraData + 0x60);
    *reinterpret_cast<float*>(packet + 0x4a4) = *reinterpret_cast<const float*>(prevCameraData + 0x64);
    *reinterpret_cast<float*>(packet + 0x4ac) = *reinterpret_cast<const float*>(prevCameraData + 0x6c);
    std::memcpy(packet + 0x4b0, prevCameraData + 0x70, 16);
    *reinterpret_cast<uint32_t*>(packet + 0x4c0) = *reinterpret_cast<const uint32_t*>(prevCameraData + 0x80);

    rendererBytes[0x400] = 1;
    rendererBytes[0x401] = 1;
    rendererBytes[0x402] = 1;
    *reinterpret_cast<uint32_t*>(rendererBytes + 0x118) = passFlagA;
    *reinterpret_cast<uint32_t*>(rendererBytes + 0x11c) = passFlagB;

    void* packetPair[2] = { packet, nullptr };
    sub_228e6a0(sceneRenderer, passContext, packetPair);

    rendererBytes[0x400] = 0;
    rendererBytes[0x403] = 0;
    rendererBytes[0x402] = 0;
    if (rendererBytes[0x401] == 0) {
        sub_2289bb0(*reinterpret_cast<void**>(rendererBytes + 0x450), 0xffu, 0u, 4u);
    }
    *reinterpret_cast<uint32_t*>(rendererBytes + 0x11c) = 0u;
}

// ============================================================================
// 9. 0x21d4850 (5 B) - GXYebisPostProcessDirector_ShutdownRedirect
// ============================================================================
void sub_21d4850(void* self) {
    sub_21d2830(self);
}

// ============================================================================
// 10. 0x21d44f0 (863 B) - GXYebisPostProcessDirector_LoadFilterParamFileFromDebugGui
// ============================================================================
void sub_21d44f0(void* self, int /*arg*/) {
    auto* dir = static_cast<uint8_t*>(self);
    uint8_t* tlsHeap = static_cast<uint8_t*>(g_TlsLinearHeap_53e4568);
    if (tlsHeap == nullptr) {
        sub_207d250();
        tlsHeap = static_cast<uint8_t*>(g_TlsLinearHeap_53e4568);
    }
    void* strHeap = tlsHeap + 0x28;
    void* strHeapVtable = *reinterpret_cast<void**>(strHeap);
    uint8_t heapInfo[16] = {};
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(
        *reinterpret_cast<void**>(static_cast<uint8_t*>(strHeapVtable) + 0x20))(heapInfo, strHeap, 0);
    if ((heapInfo[0] & 0x20u) == 0) {
        sub_20b55b0(nullptr, 0x3eu, kErrIncompatibleHeap_4737dc0);
    }

    struct WStringStack {
        void*    vtableOrPad;
        union {
            void*    heapPtr;
            uint16_t buf[8];
        };
        uint64_t size;
        uint64_t capacity;
        void*    allocator;
        uint8_t  flag;
    } wstr = {};
    wstr.capacity = 7;
    wstr.size = 0;
    wstr.buf[0] = 0;
    wstr.allocator = strHeap;
    wstr.flag = 1;

    auto* guiOwner = *reinterpret_cast<uint8_t**>(dir + 0x18);
    sub_2374ee0(*reinterpret_cast<void**>(guiOwner + 0x110), &wstr, 1u);

    const void* pathChars = (wstr.capacity >= 8)
        ? static_cast<const void*>(wstr.heapPtr)
        : static_cast<const void*>(&wstr.buf[0]);

    if (g_GlobalStringHeap_5540670 == nullptr) {
        g_GlobalStringHeap_5540670 = sub_2ba2b10();
    }

    struct PathObj {
        void*    heap;
        uint64_t field8;
        uint64_t field10;
        uint32_t field18;
        uint32_t pad1C;
    } pathObj = { g_GlobalStringHeap_5540670, 0, 0, 0, 0 };

    sub_2079230(&pathObj, pathChars, 0, 0, 0);

    struct FileReaderObj {
        void*    vtable;
        uint8_t* streamHolder;
        uint32_t status;
        uint32_t pad;
    } reader = { g_VtableFileStreamReader_53b29d0, nullptr, 0, 0 };

    sub_20761a0(&reader, &pathObj, 0);

    auto resolveStream = [](uint8_t* holder) -> void* {
        if (holder == nullptr) {
            return nullptr;
        }
        uint8_t* slot = holder + 0x08;
        if (*reinterpret_cast<uint64_t*>(holder + 0x10) != 0) {
            uint8_t flags = holder[0x18];
            uint8_t useAlt = 1;
            if ((flags & 1u) != 0) {
                useAlt = static_cast<uint8_t>((flags & 2u) >> 1);
            }
            if (useAlt != 0) {
                slot = holder + 0x10;
            }
        }
        return *reinterpret_cast<void**>(slot);
    };

    void* stream = resolveStream(reader.streamHolder);
    if (stream != nullptr) {
        auto* streamVt = *reinterpret_cast<void***>(stream);
        uint8_t isOpen = reinterpret_cast<uint8_t(*)(void*)>(streamVt[0xa0 / 8])(stream);
        if (isOpen != 0) {
            auto* fp = g_GlobalYebisFilterParam_55406e0;
            sub_2076420(&reader, fp, 0x370);
            float glareLen = *reinterpret_cast<float*>(fp + 0x304);
            if (kMinGlareLength_4929784 > glareLen) {
                glareLen = kMinGlareLength_4929784;
            } else if (glareLen > kMaxGlareLength_4929788) {
                glareLen = kMaxGlareLength_4929788;
            }
            *reinterpret_cast<float*>(fp + 0x304) = glareLen;

            std::memcpy(g_GlobalYebisSavedFilterParam_5540a50, fp, 0x370);
            std::memcpy(dir + 0x2680, fp + 0x134, 16);
            std::memcpy(dir + 0x2690, dir + 0x2680, 16);
            std::memcpy(dir + 0x26a0, fp + 0x178, 16);
            std::memcpy(dir + 0x26b0, dir + 0x26a0, 16);
            std::memcpy(dir + 0x26c0, fp + 0x110, 16);
            std::memcpy(dir + 0x26d0, dir + 0x26c0, 16);
            std::memcpy(dir + 0x26e0, fp + 0x120, 16);
            std::memcpy(dir + 0x26f0, dir + 0x26e0, 16);
            std::memcpy(g_GlobalYebisSavedFilterParam_5540a50, fp, 0x370);
        }
    }

    dir[0x2718] = 1;
    reader.vtable = g_VtableFileStreamReader_53b29d0;
    void* streamToClose = resolveStream(reader.streamHolder);
    if (streamToClose != nullptr) {
        auto* streamVt = *reinterpret_cast<void***>(streamToClose);
        reinterpret_cast<void(*)(void*)>(streamVt[0xb0 / 8])(streamToClose);
    }
    reader.streamHolder = nullptr;
    reinterpret_cast<void(*)(void*, uint32_t)>(
        reinterpret_cast<void**>(reader.vtable)[0])(&reader, 0);

    sub_2079910(&pathObj);
    if (wstr.capacity >= 8) {
        void* alloc = wstr.allocator;
        auto* allocVt = *reinterpret_cast<void***>(alloc);
        reinterpret_cast<void(*)(void*, void*)>(allocVt[0x70 / 8])(
            alloc, wstr.heapPtr);
    }
}

// ============================================================================
// 11. 0x21d4240 (674 B) - GXYebisPostProcessDirector_SaveFilterParamFileFromDebugGui
// ============================================================================
void sub_21d4240(void* self, int /*arg*/) {
    auto* dir = static_cast<uint8_t*>(self);
    uint8_t* tlsHeap = static_cast<uint8_t*>(g_TlsLinearHeap_53e4568);
    if (tlsHeap == nullptr) {
        sub_207d250();
        tlsHeap = static_cast<uint8_t*>(g_TlsLinearHeap_53e4568);
    }
    void* strHeap = tlsHeap + 0x28;
    void* strHeapVtable = *reinterpret_cast<void**>(strHeap);
    uint8_t heapInfo[16] = {};
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(
        *reinterpret_cast<void**>(static_cast<uint8_t*>(strHeapVtable) + 0x20))(heapInfo, strHeap, 0);
    if ((heapInfo[0] & 0x20u) == 0) {
        sub_20b55b0(nullptr, 0x3eu, kErrIncompatibleHeap_4737dc0);
    }

    struct WStringStack {
        void*    vtableOrPad;
        union {
            void*    heapPtr;
            uint16_t buf[8];
        };
        uint64_t size;
        uint64_t capacity;
        void*    allocator;
        uint8_t  flag;
    } wstr = {};
    wstr.capacity = 7;
    wstr.size = 0;
    wstr.buf[0] = 0;
    wstr.allocator = strHeap;
    wstr.flag = 1;

    auto* guiOwner = *reinterpret_cast<uint8_t**>(dir + 0x18);
    sub_2374ee0(*reinterpret_cast<void**>(guiOwner + 0x110), &wstr, 1u);

    const void* pathChars = (wstr.capacity >= 8)
        ? static_cast<const void*>(wstr.heapPtr)
        : static_cast<const void*>(&wstr.buf[0]);

    if (g_GlobalStringHeap_5540670 == nullptr) {
        g_GlobalStringHeap_5540670 = sub_2ba2b10();
    }

    struct PathObj {
        void*    heap;
        uint64_t field8;
        uint64_t field10;
        uint32_t field18;
        uint32_t pad1C;
    } pathObj = { g_GlobalStringHeap_5540670, 0, 0, 0, 0 };

    sub_2079230(&pathObj, pathChars, 0, 0, 0);

    struct FileWriterObj {
        void*    vtable;
        uint8_t* streamHolder;
        uint64_t pad10;
    } writer = {};
    sub_2076cb0(&writer, &pathObj, 0, 0, 1u, 1u);

    uint8_t* holder = writer.streamHolder;
    if (holder != nullptr) {
        uint8_t* checkSlot = holder + 0x08;
        uint64_t altPtr = *reinterpret_cast<uint64_t*>(holder + 0x10);
        if (altPtr != 0) {
            uint8_t flags = holder[0x18];
            uint8_t useAlt = 1;
            if ((flags & 1u) != 0) {
                useAlt = static_cast<uint8_t>((flags & 2u) >> 1);
            }
            if (useAlt != 0) {
                checkSlot = holder + 0x10;
            }
        }
        if (*reinterpret_cast<void**>(checkSlot) != nullptr) {
            uint8_t* callSlot = holder + 0x08;
            if (altPtr != 0) {
                uint8_t flags = holder[0x18];
                if ((flags & 1u) == 0 || (flags & 2u) != 0) {
                    callSlot = holder + 0x10;
                }
            }
            void* stream = *reinterpret_cast<void**>(callSlot);
            auto* streamVt = *reinterpret_cast<void***>(stream);
            uint8_t isOpen = reinterpret_cast<uint8_t(*)(void*)>(streamVt[0xa0 / 8])(stream);
            if (isOpen != 0) {
                auto* fp = g_GlobalYebisFilterParam_55406e0;
                std::memcpy(fp + 0x134, dir + 0x2680, 16);
                std::memcpy(dir + 0x2690, dir + 0x2680, 16);
                std::memcpy(fp + 0x178, dir + 0x26a0, 16);
                std::memcpy(dir + 0x26b0, dir + 0x26a0, 16);
                std::memcpy(fp + 0x110, dir + 0x26c0, 16);
                std::memcpy(dir + 0x26d0, dir + 0x26c0, 16);
                std::memcpy(fp + 0x120, dir + 0x26e0, 16);
                std::memcpy(dir + 0x26f0, dir + 0x26e0, 16);
                sub_20770d0(&writer, fp, 0x370);
            }
        }
    }

    sub_2076f70(&writer);
    sub_2079910(&pathObj);
    if (wstr.capacity >= 8) {
        void* alloc = wstr.allocator;
        auto* allocVt = *reinterpret_cast<void***>(alloc);
        reinterpret_cast<void(*)(void*, void*)>(allocVt[0x70 / 8])(
            alloc, wstr.heapPtr);
    }
}

// ============================================================================
// 12. 0x21d4230 (12 B) - GXYebisPostProcessDirector_ResetFilterParamToDefault
// ============================================================================
void sub_21d4230(void* /*self*/, int /*arg*/) {
    sub_21df510(g_GlobalYebisFilterParam_55406e0);
}

// ============================================================================
// 13. 0x21d4210 (20 B) - GXYebisPostProcessDirector_SyncFeedBackBlurEnable
// ============================================================================
void sub_21d4210(void* /*self*/) {
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    *reinterpret_cast<uint32_t*>(fp + 0x21c) = *reinterpret_cast<const uint32_t*>(fp + 0x208);
}

// ============================================================================
// 14. 0x21d2dd0 (5,184 B) - GXYebisPostProcessDirector_InitializeContextAndLutResources
// ============================================================================
void sub_21d2dd0(void* self) {
    auto* dir = static_cast<uint8_t*>(self);
    dir[0x2718] = 0;
    dir[0x20] = 0;

    void* yebisCtx = *reinterpret_cast<void**>(dir + 0x2670);
    uint32_t alreadyHadContext = 1u;

    if (yebisCtx == nullptr) {
        uint16_t staticFlags = g_YebisStaticInitFlags_56a6d10;
        if ((staticFlags & 0xFFu) == 0) {
            g_YebisStaticHeap_56a6d08 = *reinterpret_cast<void**>(dir + 0x08);
            if (staticFlags <= 0xFFu) {
                sub_bb6e60(g_YebisStaticLock_56a6d00);
            }
            *reinterpret_cast<uint8_t*>(&g_YebisStaticInitFlags_56a6d10) = 1u;
        }

        if (*reinterpret_cast<void**>(dir + 0x2648) == nullptr) {
            int64_t poolSize = *reinterpret_cast<int32_t*>(dir + 0x2714);
            if (poolSize > 0) {
                void* mainAlloc = g_GlobalHeapAllocator_5540698;
                auto* mainAllocVt = *reinterpret_cast<void***>(mainAlloc);
                void* poolMem = reinterpret_cast<void*(*)(void*, int64_t, uint64_t)>(
                    mainAllocVt[0x58 / 8])(mainAlloc, poolSize, 0x200000ULL);
                *reinterpret_cast<void**>(dir + 0x2648) = poolMem;
                if (poolMem != nullptr) {
                    std::memset(poolMem, 0xFF, static_cast<std::size_t>(*reinterpret_cast<int32_t*>(dir + 0x2714)));
                }

                uint8_t* rawBase = *reinterpret_cast<uint8_t**>(dir + 0x2648);
                if (rawBase != nullptr) {
                    int64_t rawSize = *reinterpret_cast<int32_t*>(dir + 0x2714);
                    void* mutexObj = dir + 0x2628;
                    auto* mutexVt = *reinterpret_cast<void***>(mutexObj);
                    uint8_t mutexReady = reinterpret_cast<uint8_t(*)(void*)>(mutexVt[0x10 / 8])(mutexObj);
                    bool lockOk = true;
                    if (mutexReady == 0) {
                        if (sub_2083fe0(dir + 0x2630) != 0) {
                            lockOk = false;
                        } else {
                            dir[0x2638] = 1;
                        }
                    }

                    if (lockOk) {
                        uintptr_t alignedStart = (reinterpret_cast<uintptr_t>(rawBase) + 0x17ULL) & ~0xFULL;
                        uintptr_t alignedEnd   = (reinterpret_cast<uintptr_t>(rawBase) + static_cast<uintptr_t>(rawSize) - 0x8ULL) & ~0xFULL;
                        uintptr_t usableSpan   = alignedEnd - alignedStart;

                        if (usableSpan < 0x40ULL) {
                            std::memset(dir + 0x21e8, 0, 0x440);
                        } else {
                            *reinterpret_cast<uintptr_t*>(dir + 0x21f0) = alignedStart;
                            *reinterpret_cast<uintptr_t*>(dir + 0x21f8) = alignedEnd;

                            uint8_t* binBase = dir + 0x2210;
                            for (uint64_t off = 0; off < 0x400; off += 0x10) {
                                void* selfNode = binBase + off - 0x10;
                                *reinterpret_cast<void**>(binBase + off + 0x08) = selfNode;
                                *reinterpret_cast<void**>(binBase + off + 0x00) = selfNode;
                            }

                            auto* firstHdr = reinterpret_cast<uint8_t*>(alignedStart - 0x10ULL);
                            auto* endHdr   = reinterpret_cast<uint8_t*>(alignedEnd - 0x10ULL);

                            *reinterpret_cast<uintptr_t*>(alignedStart - 0x08ULL) =
                                reinterpret_cast<uintptr_t>(endHdr) | 0x2ULL;
                            *reinterpret_cast<void**>(alignedEnd - 0x10ULL) = firstHdr;
                            *reinterpret_cast<uintptr_t*>(alignedEnd - 0x08ULL) =
                                reinterpret_cast<uintptr_t>(firstHdr) | 0x1ULL;

                            *reinterpret_cast<void**>(dir + 0x2610) = firstHdr;
                            *reinterpret_cast<void**>(dir + 0x2618) = endHdr;

                            uint64_t blockBytes = static_cast<uint64_t>(endHdr - firstHdr);
                            *reinterpret_cast<uint64_t*>(dir + 0x2200) = blockBytes;
                            *reinterpret_cast<uint64_t*>(dir + 0x2620) = blockBytes;
                            *reinterpret_cast<uint64_t*>(dir + 0x2208) = 0ULL;
                            *reinterpret_cast<uint64_t*>(dir + 0x21e8) = 0ULL;

                            uint32_t lz = static_cast<uint32_t>(__builtin_clzll(blockBytes));
                            uint32_t binIdx = 63u - lz;
                            uint64_t binOffset = static_cast<uint64_t>(binIdx) << 4;
                            uint8_t* sentinel = dir + binOffset + 0x2200;
                            uint8_t* oldHead  = *reinterpret_cast<uint8_t**>(dir + binOffset + 0x2218);

                            *reinterpret_cast<void**>(dir + binOffset + 0x2218) = firstHdr;
                            *reinterpret_cast<void**>(alignedStart + 0x00ULL) = sentinel;
                            *reinterpret_cast<void**>(alignedStart + 0x08ULL) = oldHead;
                            *reinterpret_cast<void**>(oldHead + 0x10) = firstHdr;

                            if (oldHead == sentinel) {
                                *reinterpret_cast<uint64_t*>(dir + 0x21e8) |= (1ULL << binIdx);
                            }

                            *reinterpret_cast<void**>(dir + 0x21d0) = rawBase;
                            *reinterpret_cast<uint32_t*>(dir + 0x2640) = 0u;
                            *reinterpret_cast<void**>(dir + 0x21e0) = dir + 0x21c0;

                            void* heapReg = sub_207bbf0();
                            sub_207c000(heapReg, dir + 0x21d8, rawBase, static_cast<uint64_t>(rawSize));
                        }
                    }
                }
            }
        }

        yebisCtx = sub_bb4ac0();
        *reinterpret_cast<void**>(dir + 0x2670) = yebisCtx;
        alreadyHadContext = 0u;
    }

    // Build Yebis2 PPFX Initialization Config Struct on stack (0x100 bytes @ rsp+0x300)
    alignas(16) uint8_t initCfg[0x100] = {};
    auto* fp = g_GlobalYebisFilterParam_55406e0;
    *reinterpret_cast<uint32_t*>(initCfg + 0x00) = 1u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x04) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x08) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x0c) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x28) = 0xFFFFFFFFu;
    *reinterpret_cast<uint32_t*>(initCfg + 0x2c) = 1u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x30) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x4c) = 0x10u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x58) = 0x3f400000u; // 0.75f
    *reinterpret_cast<uint32_t*>(initCfg + 0x5c) = 0u;
    initCfg[0x60] = 1u;
    initCfg[0x61] = 1u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x64) = 8u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x68) = 0x3e800000u; // 0.25f
    *reinterpret_cast<uint32_t*>(initCfg + 0x6c) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x70) = 1u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x7c) = 7u;
    *reinterpret_cast<uint32_t*>(initCfg + 0x84) = 0xFFFFFFFFu;
    *reinterpret_cast<uint32_t*>(initCfg + 0x88) = 0xFFFFFFFFu;
    std::memset(initCfg + 0x8c, 0xFF, 0x38);
    *reinterpret_cast<uint32_t*>(initCfg + 0xc4) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0xc8) = *reinterpret_cast<const uint32_t*>(fp + 0x300);
    *reinterpret_cast<uint32_t*>(initCfg + 0xcc) = 0x3f59999au; // 0.85f
    *reinterpret_cast<uint32_t*>(initCfg + 0xd0) = 0x3f400000u; // 0.75f
    *reinterpret_cast<uint32_t*>(initCfg + 0xd4) = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0xd8) = 0u;
    initCfg[0xdc] = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0xf4) = 0x100u;
    initCfg[0xf8] = 0u;
    *reinterpret_cast<uint32_t*>(initCfg + 0xfc) = 0u;

    dir[0x21aa] = 1u;

    const uint32_t flagHdr        = *reinterpret_cast<const uint32_t*>(fp + 0x0c);
    const uint32_t bit14          = (*reinterpret_cast<const uint32_t*>(fp + 0x14) != 0) ? 0x004c0000u : 0u;
    const uint32_t bit18          = (*reinterpret_cast<const uint32_t*>(fp + 0x18) != 0) ? 0x00040000u : 0u;
    const uint32_t bit20          = (*reinterpret_cast<const uint32_t*>(fp + 0x20) != 0) ? 0x00240000u : 0u;
    const uint32_t bit28          = (*reinterpret_cast<const uint32_t*>(fp + 0x28) != 0) ? 0x00800000u : 0u;
    const uint32_t bit30          = (*reinterpret_cast<const uint32_t*>(fp + 0x30) != 0) ? 0x08040000u : 0u;
    const uint32_t bit38          = (*reinterpret_cast<const uint32_t*>(fp + 0x38) != 0) ? 0x00040000u : 0u;
    const uint64_t pair40         = *reinterpret_cast<const uint64_t*>(fp + 0x40);
    const uint32_t bit40Lo        = (static_cast<uint32_t>(pair40) != 0) ? (1u << 29) : 0u;
    const uint32_t bit40Hi        = (static_cast<uint32_t>(pair40 >> 32) != 0) ? (1u << 30) : 0u;
    uint32_t featureFlags         = (flagHdr != 0) ? 0x10060000u : 0x10020000u;

    if (*reinterpret_cast<void**>(dir + 0x2650) == nullptr) {
        void* gfxAlloc = g_GlobalGfxAllocator_55406a8;
        auto* gfxAllocVt = *reinterpret_cast<void***>(gfxAlloc);
        auto* smallBuf = static_cast<uint8_t*>(
            reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(gfxAllocVt[0x58 / 8])(gfxAlloc, 0x0cULL, 4ULL));
        *reinterpret_cast<void**>(dir + 0x2650) = smallBuf;
        gx_direct_vk::PrepareDirectBufferWrite(smallBuf, 0x0cULL);
        *reinterpret_cast<uint64_t*>(smallBuf + 0x00) = 0x0003000100020000ULL;
        *reinterpret_cast<uint32_t*>(smallBuf + 0x08) = 0x00020001u;
        gx_direct_vk::NoteDirectAssetBufferWrite(smallBuf, 0x0cULL);
        gx_direct_vk::RegisterDirectImmutableBuffer(0u, smallBuf, 0x0cu, 0x0cu, 2u);
    }

    if (alreadyHadContext == 0 && g_YebisRingBuffersAllocated_56c3ce0 == 0) {
        g_YebisRingBuffersAllocated_56c3ce0 = 1u;
        uint64_t sizeA = sub_1091210();
        uint64_t sizeB = sub_10911a0(0x10u);

        auto allocGfx = [](uint64_t sz, uint64_t al) -> void* {
            void* ga = g_GlobalGfxAllocator_55406a8;
            auto* vt = *reinterpret_cast<void***>(ga);
            return reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(vt[0x58 / 8])(ga, sz, al);
        };

        g_YebisRingBuf0_A_56c3d00 = allocGfx(sizeA, 4ULL);
        g_YebisRingBuf0_B_56c3d10 = allocGfx(sizeB, 4ULL);
        g_YebisRingBuf0_C_56c3d20 = allocGfx(0x3ffffcULL, 4ULL);
        g_YebisRingBuf0_D_56c3d30 = allocGfx(0x3ffffcULL, 4ULL);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf0_A_56c3d00, sizeA);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf0_B_56c3d10, sizeB);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf0_C_56c3d20, 0x3ffffcULL);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf0_D_56c3d30, 0x3ffffcULL);

        sub_10945f0(
            g_YebisGnmCtx0_56a6d28,
            g_YebisRingBuf0_A_56c3d00,
            g_YebisRingBuf0_B_56c3d10,
            0x10u,
            g_YebisRingBuf0_C_56c3d20,
            0x3ffffcu,
            g_YebisRingBuf0_D_56c3d30,
            0x3ffffcu);
        g_YebisCtxPtr0_56c3c98    = g_YebisGnmCtx0_56a6d28;
        g_YebisSubCtxPtr0_56c3ca8 = g_YebisGnmSubCtx0_56a6da8;

        g_YebisRingBuf1_A_56c3d08 = allocGfx(sizeA, 4ULL);
        g_YebisRingBuf1_B_56c3d18 = allocGfx(sizeB, 4ULL);
        g_YebisRingBuf1_C_56c3d28 = allocGfx(0x3ffffcULL, 4ULL);
        g_YebisRingBuf1_D_56c3d38 = allocGfx(0x3ffffcULL, 4ULL);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf1_A_56c3d08, sizeA);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf1_B_56c3d18, sizeB);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf1_C_56c3d28, 0x3ffffcULL);
        gx_direct_vk::RegisterTransientCpuRing(g_YebisRingBuf1_D_56c3d38, 0x3ffffcULL);

        sub_10945f0(
            g_YebisGnmCtx1_56b54e0,
            g_YebisRingBuf1_A_56c3d08,
            g_YebisRingBuf1_B_56c3d18,
            0x10u,
            g_YebisRingBuf1_C_56c3d28,
            0x3ffffcu,
            g_YebisRingBuf1_D_56c3d38,
            0x3ffffcu);
        g_YebisCtxPtr1_56c3ca0    = g_YebisGnmCtx1_56b54e0;
        g_YebisSubCtxPtr1_56c3cb0 = g_YebisGnmSubCtx1_56b5560;

        g_YebisContextSlotIdx_56a6d20 = 0u;
        uint64_t* fenceMem = static_cast<uint64_t*>(allocGfx(0x10ULL, 8ULL));
        gx_direct_vk::RegisterTransientCpuRing(fenceMem, 0x10ULL);
        g_YebisFenceLabel0_56c3cf0 = fenceMem;
        g_YebisFenceLabel1_56c3cf8 = fenceMem;
        fenceMem[0] = 0ULL;
        fenceMem[1] = 0ULL;
        g_YebisFenceCounter_56c3ce8 = 1ULL;

        uint8_t* activeRing = &g_YebisContextSlotIdx_56a6d20 +
            static_cast<uint64_t>(g_YebisContextSlotIdx_56a6d20) * 0xe7b8ULL + 0x08ULL;
        g_YebisActiveRingBase_56c3cb8 = activeRing;
        sub_10946d0(activeRing);

        uint8_t* ringBase = static_cast<uint8_t*>(g_YebisActiveRingBase_56c3cb8);
        g_YebisActiveRingCopy_56c3cc0    = ringBase;
        g_YebisActiveRingPlus80_56c3cc8  = ringBase + 0x80;
        g_YebisActiveRingQword0_56c3cd0  = *reinterpret_cast<void**>(ringBase + 0x00);
        g_YebisActiveRingQword80_56c3cd8 = *reinterpret_cast<void**>(ringBase + 0x80);
    }

    featureFlags |= bit14 | bit18 | bit20 | bit28 | bit30 | bit38 | bit40Lo | bit40Hi;
    g_ActiveYebisDirector_56a6d18 = dir;

    auto* renderSys = static_cast<uint8_t*>(g_GlobalRenderSystem_5540dd8);
    void* devPtr = static_cast<uint8_t*>(*reinterpret_cast<void**>(renderSys + 0x1c0)) + 0x10;

    struct YebisCallbackTable {
        void*    allocFunc;
        void*    freeFunc;
        void**   devPtrRef;
        void*    ctxArray;
        void*    subCtxArray;
        uint32_t flagOne;
        uint32_t numContexts;
        uint64_t pad30;
        uint64_t pad38;
    } cbTable = {
        reinterpret_cast<void*>(&sub_21d1e40),
        reinterpret_cast<void*>(&sub_21d1f00),
        &devPtr,
        &g_YebisCtxPtr0_56c3c98,
        &g_YebisSubCtxPtr0_56c3ca8,
        1u,
        2u,
        0ULL,
        0ULL
    };

    const uint64_t resPair = *reinterpret_cast<const uint64_t*>(fp + 0x48);
    const uint32_t resW = static_cast<uint32_t>(resPair);
    const uint32_t resH = static_cast<uint32_t>(resPair >> 32);

    if (alreadyHadContext != 0) {
        if (dir[0x20f8] != 0) {
            sub_bb4d70(yebisCtx, 0u);
            sub_bb4de0(yebisCtx, g_YebisContextSlotIdx_56a6d20);
            sub_bb4b40(yebisCtx, &cbTable, resW, resH, featureFlags, initCfg);
            sub_bb26e0(yebisCtx, 0u);
            sub_bb26f0(yebisCtx, 0u);
            dir[0x20f8] = 0;
        }
    } else {
        auto* sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);
        auto* syncTarget = *reinterpret_cast<uint8_t**>(sceneCtx + 0x348);
        void* syncObj = syncTarget + 0x68;
        auto* syncVt = *reinterpret_cast<void***>(syncObj);
        reinterpret_cast<void(*)(void*, uint32_t)>(syncVt[0x18 / 8])(syncObj, 0xFFFFFFFFu);

        int64_t frameSlot = *reinterpret_cast<int32_t*>(sceneCtx + 0x68c);
        *reinterpret_cast<uint32_t*>(sceneCtx + (frameSlot << 4) + 0x670) = 0u;

        sub_bb4b40(yebisCtx, &cbTable, resW, resH, featureFlags, initCfg);
        dir[0x20f8] = 0;
    }

    sub_bb3330(yebisCtx, 0xFFFFFFFFu, kYebisParam100_4929760);
    sub_bb31a0(yebisCtx, 0xFFFFFFFFu, kYebisKeyVal_4929764, kOne_4929768);

    // Allocate 3D Color-Grading LUT (16x16x16) and 2D LUT textures if not yet created
    if (*reinterpret_cast<void**>(dir + 0x28) == nullptr) {
        uint32_t cbDesc[6] = { 0x40u, 0u, 4u, 0u, 0u, 0u };
        void* dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        sub_2165c10(dev, dir + 0x70, cbDesc, nullptr, 0u, 0u, nullptr);

        uint32_t tex3dDesc[10] = {
            0x10u, 0x10u, 0x10u, 0u, 2u, 0u, 0x88u, 0u, 0u, 0u
        };
        *reinterpret_cast<uint32_t*>(dir + 0x98) = 0x10u;
        *reinterpret_cast<uint32_t*>(dir + 0x9c) = 0x10u;
        *reinterpret_cast<uint32_t*>(dir + 0xa0) = 0x10u;
        *reinterpret_cast<uint32_t*>(dir + 0xa4) = 0x10u;

        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        if (sub_2166300(dev, dir + 0x28, tex3dDesc, nullptr, 0u, 0u, nullptr) == 0) {
            uint32_t srv3dDesc[6] = { 2u, 8u, 0u, 1u, 0u, 0u };
            dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
            if (sub_21669a0(dev, dir + 0x30, *reinterpret_cast<void**>(dir + 0x28), srv3dDesc, 0u) == 0) {
                auto* tex3dObj = *reinterpret_cast<const uint8_t* const*>(dir + 0x28);
                uint32_t depthSlices = *reinterpret_cast<const uint16_t*>(tex3dObj + 0x48);
                uint32_t uav3dDesc[6] = { 2u, 6u, 0u, 0u, depthSlices, 0u };
                dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
                sub_2166b60(dev, dir + 0x38, *reinterpret_cast<void**>(dir + 0x28), uav3dDesc, 0u);
            }
        }

        uint32_t tex2dDesc[12] = {
            0x10u, 0x100u, 1u, 1u, 2u, 1u, 0u, 0u, 0x88u, 0u, 0u, 0u
        };
        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        sub_2165f90(dev, dir + 0x40, tex2dDesc, nullptr, 0u, 0u, nullptr);

        uint32_t srv2dDesc[6] = { 2u, 4u, 0u, 1u, 0u, 1u };
        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        if (sub_21669a0(dev, dir + 0x48, *reinterpret_cast<void**>(dir + 0x40), srv2dDesc, 0u) == 0) {
            uint32_t uav2dDesc[6] = { 2u, 4u, 0u, 0u, 1u, 0u };
            dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
            sub_2166b60(dev, dir + 0x50, *reinterpret_cast<void**>(dir + 0x40), uav2dDesc, 0u);
        }

        // Generate default 16x256 neutral identity 3D-unwrapped-to-2D RGBA32F LUT image (0x10000 bytes)
        void* dirHeap = *reinterpret_cast<void**>(dir + 0x08);
        auto* dirHeapVt = *reinterpret_cast<void***>(dirHeap);
        float* lutPixels = static_cast<float*>(
            reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(dirHeapVt[0x58 / 8])(dirHeap, 0x10000ULL, 0x10ULL));

        for (int32_t y = 0; y < 256; ++y) {
            const float g = static_cast<float>(y & 0xF) / kFifteen_492976c;
            const float b = static_cast<float>(y) / kTwoFiftyFive_4929770;
            float* row = lutPixels + static_cast<std::size_t>(y) * 64u;
            for (int32_t x = 0; x < 16; ++x) {
                row[x * 4 + 0] = static_cast<float>(x) / kFifteen_492976c;
                row[x * 4 + 1] = g;
                row[x * 4 + 2] = b;
                row[x * 4 + 3] = kOne_4929768;
            }
        }

        struct SubresourceInitData {
            const void* data;
            uint32_t    pitch;
            uint32_t    slicePitch;
        } initLutData = { lutPixels, 0x100u, 0u };

        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        sub_2165f90(dev, dir + 0x60, tex2dDesc, &initLutData, 0u, 0u, nullptr);

        if (lutPixels != nullptr) {
            void* cachedHeap = *reinterpret_cast<void**>(g_TlsHeapCache_53e44a8 + 0x10);
            *reinterpret_cast<void**>(g_TlsHeapCache_53e44a8 + 0x10) = nullptr;
            if (cachedHeap == nullptr) {
                cachedHeap = sub_207b720(lutPixels);
                if (cachedHeap == nullptr) {
                    sub_20b55b0(kKernelSourcePath_493bac3, 0x64u, kErrHeapNotFound_4940207);
                }
            }
            auto* chVt = *reinterpret_cast<void***>(cachedHeap);
            reinterpret_cast<void(*)(void*, void*)>(chVt[0x70 / 8])(cachedHeap, lutPixels);
        }

        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        sub_21669a0(dev, dir + 0x68, *reinterpret_cast<void**>(dir + 0x60), srv2dDesc, 0u);
    }

    // Create full-screen/LUT quad vertex buffer at dir+0x58
    if (*reinterpret_cast<void**>(dir + 0x58) == nullptr) {
        auto* sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);
        const float vpW = static_cast<float>(*reinterpret_cast<const int32_t*>(sceneCtx + 0x180));
        const float vpH = static_cast<float>(*reinterpret_cast<const int32_t*>(sceneCtx + 0x184));

        const float x0 = (0.0f / vpW) * 2.0f + kMinusOne_4929774;
        const float y0 = (kNegZero_4929778 / vpH) * 2.0f + kOne_4929768;
        const float x1 = (kSixteen_492977c / vpW) * 2.0f + kMinusOne_4929774;
        const float y1 = (kMinus256_4929780 / vpH) * 2.0f + kOne_4929768;

        alignas(16) float quadVerts[24] = {
            x0, y0, 1.0f, 1.0f, 0.0f, 0.0f,
            x1, y0, 1.0f, 1.0f, 1.0f, 0.0f,
            x0, y1, 1.0f, 1.0f, 0.0f, 1.0f,
            x1, y1, 1.0f, 1.0f, 1.0f, 1.0f
        };

        struct VbInitData {
            const void* data;
            uint32_t    pitch;
            uint32_t    slicePitch;
        } vbData = { quadVerts, 0u, 0u };

        QuadVbDesc_4738fc4 vbDesc = kQuadVbDesc_4738fc4;
        void* dev = *reinterpret_cast<void**>(sceneCtx);
        sub_2165c10(dev, dir + 0x58, &vbDesc, &vbData, 0u, 0u, nullptr);
    }

    // Create CustomDof LUT buffer & 2048x2 texture at dir+0xb8 / dir+0x20d0
    if (*reinterpret_cast<void**>(dir + 0x20d0) == nullptr) {
        uint32_t dofCbDesc[6] = { 0x2010u, 0u, 4u, 0u, 0u, 0u };
        void* dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        sub_2165c10(dev, dir + 0xb8, dofCbDesc, nullptr, 0u, 0u, nullptr);

        uint32_t dofTexDesc[12] = {
            0x800u, 2u, 1u, 1u, 0x29u, 1u, 0u, 0u, 0x88u, 0u, 0u, 0u
        };
        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        sub_2165f90(dev, dir + 0x20d0, dofTexDesc, nullptr, 0u, 0u, nullptr);

        uint32_t dofSrvDesc[6] = { 0x29u, 4u, 0u, 1u, 0u, 1u };
        dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
        if (sub_21669a0(dev, dir + 0x20d8, *reinterpret_cast<void**>(dir + 0x20d0), dofSrvDesc, 0u) == 0) {
            uint32_t dofUavDesc[6] = { 0x29u, 4u, 0u, 0u, 1u, 0u };
            dev = *reinterpret_cast<void**>(g_GlobalSceneRendererContext_55406c8);
            sub_2166b60(dev, dir + 0x20e0, *reinterpret_cast<void**>(dir + 0x20d0), dofUavDesc, 0u);
        }
    }

    std::memcpy(g_GlobalYebisSavedFilterParam_5540a50, g_GlobalYebisFilterParam_55406e0, 0x370);
}

// ============================================================================
// 15. 0x21d2da0 (40 B) - GXYebisPostProcessDirector_SetupAndEnsureInitialized
// ============================================================================
uint8_t sub_21d2da0(void* self, void* ownerContext, uint32_t poolByteSize) {
    auto* dir = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint32_t*>(dir + 0x2714) = poolByteSize;
    if (*reinterpret_cast<void**>(dir + 0x10) == nullptr) {
        *reinterpret_cast<void**>(dir + 0x10) = ownerContext;
    }
    if (*reinterpret_cast<void**>(dir + 0x2670) == nullptr) {
        sub_21d2dd0(self);
    }
    return 1u;
}

// ============================================================================
// 16. 0x21d2d90 (16 B) - GXYebisPostProcessDirector_GetGlobalExposureValue
// ============================================================================
float sub_21d2d90() {
    return *reinterpret_cast<const float*>(g_GlobalYebisFilterParam_55406e0 + 0x98);
}

// ============================================================================
// 17. 0x21d2830 (1,375 B) - GXYebisPostProcessDirector_FinalizeContextAndLutResources
// ============================================================================
void sub_21d2830(void* self) {
    auto* dir = static_cast<uint8_t*>(self);

    void* smallBuf = *reinterpret_cast<void**>(dir + 0x2650);
    if (smallBuf != nullptr) {
        gx_direct_vk::RetireDirectBufferRange(smallBuf, 0x0cULL);
        void* gfxAlloc = g_GlobalGfxAllocator_55406a8;
        auto* vt = *reinterpret_cast<void***>(gfxAlloc);
        reinterpret_cast<void(*)(void*, void*)>(vt[0x70 / 8])(gfxAlloc, smallBuf);
        *reinterpret_cast<void**>(dir + 0x2650) = nullptr;
    }

    if (*reinterpret_cast<void**>(dir + 0x2648) != nullptr) {
        sub_207b8c0(dir + 0x21d8);
        std::memset(dir + 0x2210, 0, 0x400);
        *reinterpret_cast<uint64_t*>(dir + 0x21f8) = 0ULL;
        *reinterpret_cast<uint64_t*>(dir + 0x21f0) = 0ULL;

        void* mutexObj = dir + 0x2628;
        auto* mutexVt = *reinterpret_cast<void***>(mutexObj);
        bool skipSubHeapRelease = false;
        if (reinterpret_cast<uint8_t(*)(void*)>(mutexVt[0x10 / 8])(mutexObj) != 0) {
            uint32_t err = sub_2bbfee8(dir + 0x2630);
            if (err == 0x80020010u) {
                skipSubHeapRelease = true;
            } else {
                if (err != 0u) {
                    if (err == 0x80020016u) {
                        sub_20b55b0(kPthreadMutexPath_493c20e, 0x55u, kErrInvalidAttr_493c275);
                    }
                    sub_20b55b0(kPthreadMutexPath_493c20e, 0x57u, kErrUnknown_493c2ae);
                }
                dir[0x2638] = 0;
            }
        }

        if (!skipSubHeapRelease) {
            void* subHeap = *reinterpret_cast<void**>(dir + 0x21c0);
            if (subHeap != nullptr) {
                auto* shVt = *reinterpret_cast<void***>(subHeap);
                reinterpret_cast<void(*)(void*, void*)>(shVt[0x70 / 8])(
                    subHeap, *reinterpret_cast<void**>(dir + 0x21d0));
                *reinterpret_cast<void**>(dir + 0x21c0) = nullptr;
                *reinterpret_cast<void**>(dir + 0x21d0) = nullptr;
            }
        }

        void* poolMem = *reinterpret_cast<void**>(dir + 0x2648);
        if (poolMem != nullptr) {
            void* mainAlloc = g_GlobalHeapAllocator_5540698;
            auto* vt = *reinterpret_cast<void***>(mainAlloc);
            reinterpret_cast<void(*)(void*, void*)>(vt[0x70 / 8])(mainAlloc, poolMem);
            *reinterpret_cast<void**>(dir + 0x2648) = nullptr;
        }
    }

    auto destroyViewObj = [&](uint32_t offset) {
        void* obj = *reinterpret_cast<void**>(dir + offset);
        if (obj != nullptr) {
            void* heap = sub_207b720(obj);
            auto* objVt = *reinterpret_cast<void***>(obj);
            reinterpret_cast<void(*)(void*)>(objVt[0])(obj);
            auto* heapVt = *reinterpret_cast<void***>(heap);
            reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, obj);
            *reinterpret_cast<void**>(dir + offset) = nullptr;
        }
    };

    auto releaseRefCountedRes = [&](uint32_t offset) {
        auto* res = *reinterpret_cast<uint8_t**>(dir + offset);
        if (res != nullptr) {
            int32_t prev = __sync_fetch_and_add(reinterpret_cast<int32_t*>(res + 0x10), -1);
            if (prev == 1) {
                auto* resVt = *reinterpret_cast<void***>(res);
                void* heap = *reinterpret_cast<void**>(res + 0x08);
                reinterpret_cast<void(*)(void*)>(resVt[0])(res);
                auto* heapVt = *reinterpret_cast<void***>(heap);
                reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, res);
            }
            *reinterpret_cast<void**>(dir + offset) = nullptr;
        }
    };

    destroyViewObj(0x38);
    destroyViewObj(0x30);
    releaseRefCountedRes(0x28);
    destroyViewObj(0x50);
    destroyViewObj(0x48);
    releaseRefCountedRes(0x40);
    destroyViewObj(0x20e0);
    destroyViewObj(0x20d8);
    releaseRefCountedRes(0x20d0);
    releaseRefCountedRes(0xb8);
    releaseRefCountedRes(0x58);
    destroyViewObj(0x68);
    releaseRefCountedRes(0x60);
    releaseRefCountedRes(0x70);

    void* yebisCtx = *reinterpret_cast<void**>(dir + 0x2670);
    if (yebisCtx != nullptr) {
        auto* yVt = *reinterpret_cast<void***>(yebisCtx);
        int32_t refCount = reinterpret_cast<int32_t(*)(void*)>(yVt[0x10 / 8])(yebisCtx);
        while (refCount > 0) {
            yVt = *reinterpret_cast<void***>(yebisCtx);
            reinterpret_cast<void(*)(void*)>(yVt[0x08 / 8])(yebisCtx);
            --refCount;
        }
        *reinterpret_cast<void**>(dir + 0x2670) = nullptr;
    }

    *reinterpret_cast<void**>(dir + 0x10) = nullptr;
    if (g_YebisRingBuffersAllocated_56c3ce0 != 0u) {
        g_YebisRingBuffersAllocated_56c3ce0 = 0u;

        auto freeGfx = [](void*& ptr) {
            if (ptr != nullptr) {
                void* ga = g_GlobalGfxAllocator_55406a8;
                auto* vt = *reinterpret_cast<void***>(ga);
                reinterpret_cast<void(*)(void*, void*)>(vt[0x70 / 8])(ga, ptr);
                ptr = nullptr;
            }
        };

        freeGfx(g_YebisRingBuf0_A_56c3d00);
        freeGfx(g_YebisRingBuf0_B_56c3d10);
        freeGfx(g_YebisRingBuf0_C_56c3d20);
        freeGfx(g_YebisRingBuf0_D_56c3d30);
        freeGfx(g_YebisRingBuf1_A_56c3d08);
        freeGfx(g_YebisRingBuf1_B_56c3d18);
        freeGfx(g_YebisRingBuf1_C_56c3d28);
        freeGfx(g_YebisRingBuf1_D_56c3d38);
        void* fencePtr = g_YebisFenceLabel0_56c3cf0;
        freeGfx(fencePtr);
        g_YebisFenceLabel0_56c3cf0 = nullptr;
    }

    g_YebisFenceLabel1_56c3cf8    = nullptr;
    g_YebisActiveRingBase_56c3cb8 = nullptr;
    reinterpret_cast<uint8_t*>(&g_YebisStaticInitFlags_56a6d10)[0] = 0u;
    reinterpret_cast<uint8_t*>(&g_YebisStaticInitFlags_56a6d10)[1] = 1u;
}

// ============================================================================
// 18. 0x21d24e0 (846 B) - GXYebisPostProcessDirector_Destructor
// ============================================================================
void sub_21d24e0(void* self) {
    auto* dir = static_cast<uint8_t*>(self);
    *reinterpret_cast<void**>(dir + 0x00) = g_VtableYebisDirector_535fe90;

    sub_21d2830(self);

    // Release weak/packed handle at dir+0x2728
    auto* handle2728 = *reinterpret_cast<uint8_t**>(dir + 0x2728);
    if (handle2728 != nullptr) {
        uint32_t rawWord = *reinterpret_cast<uint32_t*>(handle2728 + 0x08);
        uint32_t decWeak = ((rawWord >> 15) + 0x7fffu) & 0x7fffu;
        uint32_t updated = (rawWord & 0xc0007fffu) | (decWeak << 15);
        *reinterpret_cast<uint32_t*>(handle2728 + 0x08) = updated;
        if (decWeak == 0u) {
            *reinterpret_cast<uint32_t*>(handle2728 + 0x08) = rawWord | 0x3fff8000u;
            auto* vt = *reinterpret_cast<void***>(handle2728);
            reinterpret_cast<void(*)(void*)>(vt[0x20 / 8])(handle2728);
        }
        *reinterpret_cast<void**>(dir + 0x2728) = nullptr;
    }

    // Release ref-counted object at dir+0x2660
    auto* obj2660 = *reinterpret_cast<uint8_t**>(dir + 0x2660);
    if (obj2660 != nullptr) {
        int32_t oldRef = *reinterpret_cast<int32_t*>(obj2660 + 0x08);
        *reinterpret_cast<int32_t*>(obj2660 + 0x08) = oldRef - 1;
        if (oldRef <= 1) {
            auto* vt = *reinterpret_cast<void***>(obj2660);
            reinterpret_cast<void(*)(void*)>(vt[0])(obj2660);
        }
        *reinterpret_cast<void**>(dir + 0x2660) = nullptr;
    }

    // Tear down TLSF heap if dir+0x21f0 is still active
    if (*reinterpret_cast<void**>(dir + 0x21f0) != nullptr) {
        sub_207b8c0(dir + 0x21d8);
        std::memset(dir + 0x2210, 0, 0x400);
        *reinterpret_cast<uint64_t*>(dir + 0x21f8) = 0ULL;
        *reinterpret_cast<uint64_t*>(dir + 0x21f0) = 0ULL;

        void* mutexObj = dir + 0x2628;
        auto* mutexVt = *reinterpret_cast<void***>(mutexObj);
        bool skipSubHeap = false;
        if (reinterpret_cast<uint8_t(*)(void*)>(mutexVt[0x10 / 8])(mutexObj) != 0) {
            uint32_t err = sub_2bbfee8(dir + 0x2630);
            if (err == 0x80020010u) {
                skipSubHeap = true;
            } else {
                if (err != 0u) {
                    if (err == 0x80020016u) {
                        sub_20b55b0(kPthreadMutexPath_493c20e, 0x55u, kErrInvalidAttr_493c275);
                    }
                    sub_20b55b0(kPthreadMutexPath_493c20e, 0x57u, kErrUnknown_493c2ae);
                }
                dir[0x2638] = 0;
            }
        }
        if (!skipSubHeap) {
            void* subHeap = *reinterpret_cast<void**>(dir + 0x21c0);
            if (subHeap != nullptr) {
                auto* shVt = *reinterpret_cast<void***>(subHeap);
                reinterpret_cast<void(*)(void*, void*)>(shVt[0x70 / 8])(
                    subHeap, *reinterpret_cast<void**>(dir + 0x21d0));
                *reinterpret_cast<void**>(dir + 0x21c0) = nullptr;
                *reinterpret_cast<void**>(dir + 0x21d0) = nullptr;
            }
        }
    }

    *reinterpret_cast<void**>(dir + 0x2628) = g_VtableMutexLock_53b2fa0;
    if (dir[0x2638] != 0) {
        uint32_t err = sub_2bbfee8(dir + 0x2630);
        if (err != 0x80020010u) {
            if (err != 0u) {
                if (err == 0x80020016u) {
                    sub_20b55b0(kPthreadMutexPath_493c20e, 0x55u, kErrInvalidAttr_493c275);
                }
                sub_20b55b0(kPthreadMutexPath_493c20e, 0x57u, kErrUnknown_493c2ae);
            }
            dir[0x2638] = 0;
        }
    }

    auto destroyWStringField = [&](uint32_t baseOff) {
        if (*reinterpret_cast<uint64_t*>(dir + baseOff + 0x20) >= 8ULL) {
            void* alloc = *reinterpret_cast<void**>(dir + baseOff + 0x28);
            auto* vt = *reinterpret_cast<void***>(alloc);
            reinterpret_cast<void(*)(void*, void*)>(vt[0x70 / 8])(
                alloc, *reinterpret_cast<void**>(dir + baseOff + 0x08));
        }
        *reinterpret_cast<uint64_t*>(dir + baseOff + 0x20) = 7ULL;
        *reinterpret_cast<uint64_t*>(dir + baseOff + 0x18) = 0ULL;
        *reinterpret_cast<uint16_t*>(dir + baseOff + 0x08) = 0u;
    };

    destroyWStringField(0x2170);
    destroyWStringField(0x2138);
    destroyWStringField(0x2100);

    // Release strong+weak packed handle at dir+0x18
    auto* guiHandle = *reinterpret_cast<uint8_t**>(dir + 0x18);
    if (guiHandle != nullptr) {
        uint32_t rawWord = *reinterpret_cast<uint32_t*>(guiHandle + 0x08);
        uint32_t decStrong = (rawWord + 0x7fffu) & 0x7fffu;
        uint32_t afterStrong = (rawWord & 0xffff8000u) | decStrong;
        *reinterpret_cast<uint32_t*>(guiHandle + 0x08) = afterStrong;
        if (decStrong == 0u && (rawWord & 0x40000000u) != 0u) {
            auto* vt = *reinterpret_cast<void***>(guiHandle);
            reinterpret_cast<void(*)(void*)>(vt[0x18 / 8])(guiHandle);
            afterStrong = *reinterpret_cast<uint32_t*>(guiHandle + 0x08) & 0xbfffffffu;
            *reinterpret_cast<uint32_t*>(guiHandle + 0x08) = afterStrong;
        }
        uint32_t decWeak = ((afterStrong >> 15) + 0x7fffu) & 0x7fffu;
        uint32_t afterWeak = (afterStrong & 0xc0007fffu) | (decWeak << 15);
        *reinterpret_cast<uint32_t*>(guiHandle + 0x08) = afterWeak;
        if (decWeak == 0u) {
            *reinterpret_cast<uint32_t*>(guiHandle + 0x08) = afterStrong | 0x3fff8000u;
            auto* vt = *reinterpret_cast<void***>(guiHandle);
            reinterpret_cast<void(*)(void*)>(vt[0x20 / 8])(guiHandle);
        }
        *reinterpret_cast<void**>(dir + 0x18) = nullptr;
    }
}

// ============================================================================
// 19. 0x21d24c0 (28 B) - GXYebisPostProcessDirector_DeletingDestructor
// ============================================================================
void sub_21d24c0(void* self) {
    sub_21d24e0(self);
    sub_2bbe588(self);
}

// ============================================================================
// 20. 0x21d1f80 (1,333 B) - GXYebisPostProcessDirector_Constructor
// ============================================================================
void sub_21d1f80(void* self, void* heapAllocator) {
    auto* dir = static_cast<uint8_t*>(self);
    *reinterpret_cast<void**>(dir + 0x00) = g_VtableYebisDirector_535fe90;
    *reinterpret_cast<void**>(dir + 0x08) = heapAllocator;
    dir[0x20] = 0;
    *reinterpret_cast<void**>(dir + 0x18) = nullptr;
    *reinterpret_cast<void**>(dir + 0x10) = nullptr;

    std::memset(dir + 0x28, 0, 0x50); // +0x28 .. +0x70

    for (uint32_t off = 0x78; off <= 0xa4; off += 4) {
        *reinterpret_cast<uint32_t*>(dir + off) = 0x40u;
    }

    *reinterpret_cast<uint64_t*>(dir + 0xa8) = 0ULL;
    *reinterpret_cast<uint64_t*>(dir + 0xb0) = 0ULL;
    *reinterpret_cast<uint64_t*>(dir + 0xb8) = 0ULL;
    *reinterpret_cast<uint32_t*>(dir + 0xc0) = 0x800u;
    *reinterpret_cast<uint64_t*>(dir + 0xc4) = 2ULL;
    *reinterpret_cast<uint32_t*>(dir + 0xcc) = 0u;

    std::memset(dir + 0xd0, 0, 0x2000);

    *reinterpret_cast<void**>(dir + 0x20d0) = nullptr;
    *reinterpret_cast<void**>(dir + 0x20d8) = nullptr;
    *reinterpret_cast<void**>(dir + 0x20e0) = nullptr;

    auto initWStringWithHeap = [&](uint32_t baseOff) {
        void* strHeap = g_GlobalStringHeap_5540670;
        if (strHeap == nullptr) {
            strHeap = sub_2ba2b10();
            g_GlobalStringHeap_5540670 = strHeap;
        }
        auto* vt = *reinterpret_cast<void***>(strHeap);
        uint8_t info[16] = {};
        reinterpret_cast<void(*)(void*, void*, uint32_t)>(vt[0x20 / 8])(info, strHeap, 0u);
        if ((info[0] & 0x20u) == 0) {
            sub_20b55b0(nullptr, 0x3eu, kErrIncompatibleHeap_4737dc0);
        }
        *reinterpret_cast<void**>(dir + baseOff + 0x28)    = strHeap;
        *reinterpret_cast<uint64_t*>(dir + baseOff + 0x20) = 7ULL;
        *reinterpret_cast<uint64_t*>(dir + baseOff + 0x18) = 0ULL;
        *reinterpret_cast<uint16_t*>(dir + baseOff + 0x08) = 0u;
        dir[baseOff + 0x30] = 1u;
    };

    initWStringWithHeap(0x2100);
    initWStringWithHeap(0x2138);
    initWStringWithHeap(0x2170);

    *reinterpret_cast<void**>(dir + 0x21d8) =
        reinterpret_cast<uint8_t*>(g_VtableTlsfAllocator_52e70b0) + 0x10;
    std::memset(dir + 0x21e0, 0, 0x448);
    *reinterpret_cast<void**>(dir + 0x2628) = g_VtableMutexLock_53b2fa0;
    dir[0x2638] = 0;
    *reinterpret_cast<void**>(dir + 0x21c0) = nullptr;
    *reinterpret_cast<uint32_t*>(dir + 0x21c8) = sub_207b9f0();
    *reinterpret_cast<void**>(dir + 0x21d0) = nullptr;
    *reinterpret_cast<uint32_t*>(dir + 0x2640) = 0u;
    *reinterpret_cast<void**>(dir + 0x21e0) = dir + 0x21c0;

    dir[0x2710] = 0;
    *reinterpret_cast<uint32_t*>(dir + 0x271c) = 0u;
    *reinterpret_cast<void**>(dir + 0x2728) = nullptr;
    dir[0x20f8] = 0;
    *reinterpret_cast<uint32_t*>(dir + 0x2720) = 0u;
    dir[0x2724] = 0;
    dir[0x2718] = 0;
    *reinterpret_cast<uint32_t*>(dir + 0x2714) = 0u;
    *reinterpret_cast<void**>(dir + 0x2670) = nullptr;
    dir[0x2668] = 0;
    *reinterpret_cast<void**>(dir + 0x2660) = nullptr;
    *reinterpret_cast<void**>(dir + 0x2658) = nullptr;
    dir[0x2711] = 1u;
    dir[0x2712] = 1u;

    for (uint32_t off = 0x2680; off <= 0x26f0; off += 0x10) {
        std::memcpy(dir + off, kAllOnes_4920d50, 16);
    }
    for (uint32_t off = 0x78; off <= 0xa0; off += 8) {
        *reinterpret_cast<uint64_t*>(dir + off) = 0x0000004000000040ULL;
    }

    *reinterpret_cast<uint64_t*>(dir + 0xb0) = 0ULL;
    *reinterpret_cast<uint64_t*>(dir + 0xa8) = 0ULL;

    uint64_t prefixLen = sub_2bbf228(kColorGradingLutPrefix_49c574e);
    sub_262a720(dir + 0x2100, kColorGradingLutPrefix_49c574e, prefixLen);
    prefixLen = sub_2bbf228(kColorGradingLutPrefix_49c574e);
    sub_262a720(dir + 0x2138, kColorGradingLutPrefix_49c574e, prefixLen);
    prefixLen = sub_2bbf228(kColorGradingLutPrefix_49c574e);
    sub_262a720(dir + 0x2170, kColorGradingLutPrefix_49c574e, prefixLen);

    dir[0x21a8] = 0;
    dir[0x21a9] = 0;
    dir[0x21aa] = 0;
    std::memcpy(dir + 0x21b0, kUnitW_4920cd0, 16);
    *reinterpret_cast<uint64_t*>(dir + 0x20f0) = 0ULL;
    *reinterpret_cast<uint64_t*>(dir + 0x20e8) = 0ULL;

    sub_21df510(g_GlobalYebisFilterParam_55406e0);
    sub_21df510(g_GlobalYebisSavedFilterParam_5540a50);
}

// ============================================================================
// 21. 0x21d1f00 (116 B) - GXYebisPostProcessDirector_TlsfFreeCallback
// ============================================================================
void sub_21d1f00(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    void* heap = *reinterpret_cast<void**>(g_TlsHeapCache_53e44a8 + 0x10);
    *reinterpret_cast<void**>(g_TlsHeapCache_53e44a8 + 0x10) = nullptr;
    if (heap == nullptr) {
        heap = sub_207b720(ptr);
        if (heap == nullptr) {
            sub_20b55b0(kKernelSourcePath_493bac3, 0x64u, kErrHeapNotFound_4940207);
        }
    }
    auto* vt = *reinterpret_cast<void***>(heap);
    reinterpret_cast<void(*)(void*, void*)>(vt[0x70 / 8])(heap, ptr);
}

// ============================================================================
// 22. 0x21d1e40 (192 B) - GXYebisPostProcessDirector_TlsfAllocCallback
// ============================================================================
void* sub_21d1e40(uint32_t size, uint32_t align) {
    auto* dir = static_cast<uint8_t*>(g_ActiveYebisDirector_56a6d18);
    uint64_t paddedSize = static_cast<uint64_t>(size + 0x40u);
    uint64_t alignVal   = static_cast<uint64_t>(align);

    if (*reinterpret_cast<int32_t*>(dir + 0x2714) > 0) {
        void* mutexObj = dir + 0x2628;
        auto* mutexVt = *reinterpret_cast<void***>(mutexObj);
        reinterpret_cast<void(*)(void*, uint32_t)>(mutexVt[0x18 / 8])(mutexObj, 0xFFFFFFFFu);

        void* res = sub_2085a60(dir + 0x21e8, paddedSize, alignVal);

        mutexVt = *reinterpret_cast<void***>(mutexObj);
        reinterpret_cast<void(*)(void*)>(mutexVt[0x28 / 8])(mutexObj);

        if (res != nullptr) {
            return res;
        }
    }

    void* fallbackAlloc = g_GlobalHeapAllocator_5540698;
    auto* vt = *reinterpret_cast<void***>(fallbackAlloc);
    return reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(
        vt[0x58 / 8])(fallbackAlloc, paddedSize, alignVal);
}

// ============================================================================
// 23. 0x21d1e30 (13 B) - GXYebisPostProcessDirector_FreeToSubHeap
// ============================================================================
void sub_21d1e30(void* self, void* ptr) {
    void* subHeap = *reinterpret_cast<void**>(static_cast<uint8_t*>(self) + 0x08);
    auto* vt = *reinterpret_cast<void***>(subHeap);
    reinterpret_cast<void(*)(void*, void*)>(vt[0x70 / 8])(subHeap, ptr);
}

// ============================================================================
// 24. 0x21d1e10 (25 B) - GXYebisPostProcessDirector_AllocFromSubHeapPlus1024
// ============================================================================
void* sub_21d1e10(void* self, uint64_t size) {
    void* subHeap = *reinterpret_cast<void**>(static_cast<uint8_t*>(self) + 0x08);
    auto* vt = *reinterpret_cast<void***>(subHeap);
    return reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(
        vt[0x58 / 8])(subHeap, size + 0x400ULL, 0x10ULL);
}

// ============================================================================
// 25. 0x21d1d30 (65 B) - GXSceneRenderer_SetDofFocusDistanceOverride
// ============================================================================
void sub_21d1d30(uint8_t lockOverride, float focusDistance) {
    auto* sys = static_cast<uint8_t*>(g_GlobalRenderSystem_5540dd8);
    auto* mgr = *reinterpret_cast<uint8_t**>(sys + 0x6078);
    if (mgr != nullptr) {
        mgr[0x3201] = lockOverride;
        if (lockOverride == 0) {
            *reinterpret_cast<float*>(g_GlobalYebisFilterParam_55406e0 + 0x1b0) = focusDistance;
            *reinterpret_cast<float*>(g_GlobalYebisSavedFilterParam_5540a50 + 0x1b0) = focusDistance;
        }
    }
}

// ============================================================================
// 26. 0x21d1ce0 (65 B) - GXSceneRenderer_SetDofEnableOverride
// ============================================================================
void sub_21d1ce0(uint8_t enableDof, uint8_t lockOverride) {
    auto* sys = static_cast<uint8_t*>(g_GlobalRenderSystem_5540dd8);
    auto* mgr = *reinterpret_cast<uint8_t**>(sys + 0x6078);
    if (mgr != nullptr) {
        mgr[0x3202] = lockOverride;
        if (lockOverride == 0) {
            uint32_t val = static_cast<uint32_t>(enableDof);
            *reinterpret_cast<uint32_t*>(g_GlobalYebisFilterParam_55406e0 + 0x188) = val;
            *reinterpret_cast<uint32_t*>(g_GlobalYebisSavedFilterParam_5540a50 + 0x188) = val;
        }
    }
}

// ============================================================================
// 27. 0x21d1cc0 (31 B) - GXSceneRenderer_SetGlobalOverrideCallbackA
// ============================================================================
void sub_21d1cc0(void* callbackPtr) {
    auto* sys = static_cast<uint8_t*>(g_GlobalRenderSystem_5540dd8);
    if (*reinterpret_cast<void**>(sys + 0x6078) != nullptr) {
        g_GlobalOverrideCallbackA_5540dc0 = callbackPtr;
    }
}

// ============================================================================
// 28. 0x21d1cb0 (11 B) - GXSceneRenderer_SetGlobalOverrideCallbackB
// ============================================================================
void sub_21d1cb0(void* callbackPtr) {
    g_GlobalOverrideCallbackB_5540dc8 = callbackPtr;
}

// ============================================================================
// Internal Helper: Multiply 3x4 Bone Matrix by 3x4 Inverse Bind Matrix (SIMD)
// Matches exact instruction sequence in 0x21d1650 and 0x21d1080
// ============================================================================
namespace {

inline void MultiplyBone3x4Simd(
    float* dst3x4,
    const float* bone3x4,
    const float* invBind3x4)
{
    const __m128i bRow0i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bone3x4 + 0));
    const __m128i bRow1i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bone3x4 + 4));
    const __m128i bRow2i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bone3x4 + 8));

    const __m128 iRow0 = _mm_loadu_ps(invBind3x4 + 0);
    const __m128 iRow1 = _mm_loadu_ps(invBind3x4 + 4);
    const __m128 iRow2 = _mm_loadu_ps(invBind3x4 + 8);
    const __m128 unitW = _mm_load_ps(kUnitW_4920cd0);

    auto mulRow = [&](__m128i rowInt) -> __m128 {
        __m128 xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0x00));
        __m128 yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0x55));
        __m128 zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0xAA));
        __m128 wwww = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0xFF));

        __m128 sum = _mm_add_ps(_mm_mul_ps(iRow0, xxxx), _mm_mul_ps(yyyy, iRow1));
        sum = _mm_add_ps(_mm_mul_ps(zzzz, iRow2), sum);
        return _mm_add_ps(_mm_mul_ps(wwww, unitW), sum);
    };

    _mm_storeu_ps(dst3x4 + 0, mulRow(bRow0i));
    _mm_storeu_ps(dst3x4 + 4, mulRow(bRow1i));
    _mm_storeu_ps(dst3x4 + 8, mulRow(bRow2i));
}

inline uint8_t* GetTlsLinearHeap() {
    uint8_t* heap = static_cast<uint8_t*>(g_TlsLinearHeap_53e4568);
    if (heap == nullptr) {
        sub_207d250();
        heap = static_cast<uint8_t*>(g_TlsLinearHeap_53e4568);
    }
    return heap;
}

inline void FreeUntilTlsLinearHeap(uintptr_t mark) {
    uint8_t* heap = GetTlsLinearHeap();
    uintptr_t start = *reinterpret_cast<uintptr_t*>(heap + 0x00);
    uintptr_t end   = *reinterpret_cast<uintptr_t*>(heap + 0x08);
    if (start > mark || end < mark) {
        sub_20b55b0(kLinearHeapPath_493c62d, 0x233u, kErrLinearHeapMark_493c690);
    }
    if (*reinterpret_cast<uintptr_t*>(heap + 0x10) > mark) {
        *reinterpret_cast<uintptr_t*>(heap + 0x10) = mark;
    }
}

} // namespace

// ============================================================================
// 29. 0x21d1650 (1,488 B) - GXModelMeshDrawSubmitter_UploadPreviousFrameSkinningBonePalette
// ============================================================================
void sub_21d1650(
    void* modelRes,
    void* drawContext,
    void* meshGroup,
    const float* prevBonePalette)
{
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* group = static_cast<uint8_t*>(meshGroup);
    auto* ctxVt = *reinterpret_cast<void***>(drawContext);

    if (model[0x200] != 0) {
        int64_t boneCount = *reinterpret_cast<int32_t*>(model + 0x1c);
        if (boneCount <= 0) {
            return;
        }
        if (group[0x04] == 0) {
            reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
                ctxVt[0x28 / 8])(drawContext, 0u, static_cast<uint32_t>(boneCount), prevBonePalette);
            return;
        }

        uint8_t* linearHeap = GetTlsLinearHeap();
        uintptr_t mark = *reinterpret_cast<uintptr_t*>(linearHeap + 0x10);
        void* allocObj = linearHeap + 0x20;
        auto* allocVt  = *reinterpret_cast<void***>(allocObj);
        float* outMatrices = static_cast<float*>(
            reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(allocVt[0x58 / 8])(
                allocObj, static_cast<uint64_t>(boneCount) * 48ULL, 0x10ULL));

        const float* invBindBase = *reinterpret_cast<const float* const*>(model + 0x1f0);
        for (int64_t i = 0; i < boneCount; ++i) {
            MultiplyBone3x4Simd(outMatrices + i * 12, prevBonePalette + i * 12, invBindBase + i * 12);
        }

        ctxVt = *reinterpret_cast<void***>(drawContext);
        reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
            ctxVt[0x28 / 8])(drawContext, 0u, static_cast<uint32_t>(boneCount), outMatrices);

        FreeUntilTlsLinearHeap(mark);
        return;
    }

    uint16_t paletteCount = *reinterpret_cast<uint16_t*>(group + 0x18);
    if (group[0x04] != 0) {
        uint8_t* linearHeap = GetTlsLinearHeap();
        uintptr_t mark = *reinterpret_cast<uintptr_t*>(linearHeap + 0x10);
        void* allocObj = linearHeap + 0x20;
        auto* allocVt  = *reinterpret_cast<void***>(allocObj);
        float* outMatrices = static_cast<float*>(
            reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(allocVt[0x58 / 8])(
                allocObj, static_cast<uint64_t>(paletteCount) * 48ULL, 0x10ULL));

        if (paletteCount != 0) {
            const int32_t* boneIndices = *reinterpret_cast<const int32_t* const*>(group + 0x20);
            const float* invBindBase   = *reinterpret_cast<const float* const*>(model + 0x1f0);
            for (uint32_t i = 0; i < paletteCount; ++i) {
                int64_t boneIdx = boneIndices[i];
                if (boneIdx < 0) {
                    break;
                }
                MultiplyBone3x4Simd(
                    outMatrices + static_cast<std::size_t>(i) * 12u,
                    prevBonePalette + static_cast<std::size_t>(boneIdx) * 12u,
                    invBindBase + static_cast<std::size_t>(boneIdx) * 12u);
            }
        }

        ctxVt = *reinterpret_cast<void***>(drawContext);
        reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
            ctxVt[0x28 / 8])(drawContext, 0u, paletteCount, outMatrices);

        FreeUntilTlsLinearHeap(mark);
        return;
    }

    if (paletteCount == 0) {
        uint16_t singleBoneIdx = *reinterpret_cast<uint16_t*>(group + 0x16);
        reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
            ctxVt[0x28 / 8])(
                drawContext,
                0u,
                1u,
                prevBonePalette + static_cast<std::size_t>(singleBoneIdx) * 12u);
        return;
    }

    uint8_t* linearHeap = GetTlsLinearHeap();
    uintptr_t mark = *reinterpret_cast<uintptr_t*>(linearHeap + 0x10);
    void* allocObj = linearHeap + 0x20;
    auto* allocVt  = *reinterpret_cast<void***>(allocObj);
    float* outMatrices = static_cast<float*>(
        reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(allocVt[0x58 / 8])(
            allocObj, static_cast<uint64_t>(paletteCount) * 48ULL, 0x10ULL));

    const int32_t* boneIndices = *reinterpret_cast<const int32_t* const*>(group + 0x20);
    for (uint32_t i = 0; i < paletteCount; ++i) {
        int64_t boneIdx = boneIndices[i];
        if (boneIdx < 0) {
            break;
        }
        std::memcpy(
            outMatrices + static_cast<std::size_t>(i) * 12u,
            prevBonePalette + static_cast<std::size_t>(boneIdx) * 12u,
            48);
    }

    ctxVt = *reinterpret_cast<void***>(drawContext);
    reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
        ctxVt[0x28 / 8])(drawContext, 0u, paletteCount, outMatrices);

    FreeUntilTlsLinearHeap(mark);
}

// ============================================================================
// 30. 0x21d1080 (1,488 B) - GXModelMeshDrawSubmitter_UploadCurrentFrameSkinningBonePalette
// ============================================================================
void sub_21d1080(
    void* modelRes,
    void* drawContext,
    void* meshGroup,
    const float* bonePalette)
{
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* group = static_cast<uint8_t*>(meshGroup);
    auto* ctxVt = *reinterpret_cast<void***>(drawContext);

    if (model[0x200] != 0) {
        int64_t boneCount = *reinterpret_cast<int32_t*>(model + 0x1c);
        if (boneCount <= 0) {
            return;
        }
        if (group[0x04] == 0) {
            reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
                ctxVt[0x20 / 8])(drawContext, 0u, static_cast<uint32_t>(boneCount), bonePalette);
            return;
        }

        uint8_t* linearHeap = GetTlsLinearHeap();
        uintptr_t mark = *reinterpret_cast<uintptr_t*>(linearHeap + 0x10);
        void* allocObj = linearHeap + 0x20;
        auto* allocVt  = *reinterpret_cast<void***>(allocObj);
        float* outMatrices = static_cast<float*>(
            reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(allocVt[0x58 / 8])(
                allocObj, static_cast<uint64_t>(boneCount) * 48ULL, 0x10ULL));

        const float* invBindBase = *reinterpret_cast<const float* const*>(model + 0x1f0);
        for (int64_t i = 0; i < boneCount; ++i) {
            MultiplyBone3x4Simd(outMatrices + i * 12, bonePalette + i * 12, invBindBase + i * 12);
        }

        ctxVt = *reinterpret_cast<void***>(drawContext);
        reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
            ctxVt[0x20 / 8])(drawContext, 0u, static_cast<uint32_t>(boneCount), outMatrices);

        FreeUntilTlsLinearHeap(mark);
        return;
    }

    uint16_t paletteCount = *reinterpret_cast<uint16_t*>(group + 0x18);
    if (group[0x04] != 0) {
        uint8_t* linearHeap = GetTlsLinearHeap();
        uintptr_t mark = *reinterpret_cast<uintptr_t*>(linearHeap + 0x10);
        void* allocObj = linearHeap + 0x20;
        auto* allocVt  = *reinterpret_cast<void***>(allocObj);
        float* outMatrices = static_cast<float*>(
            reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(allocVt[0x58 / 8])(
                allocObj, static_cast<uint64_t>(paletteCount) * 48ULL, 0x10ULL));

        if (paletteCount != 0) {
            const int32_t* boneIndices = *reinterpret_cast<const int32_t* const*>(group + 0x20);
            const float* invBindBase   = *reinterpret_cast<const float* const*>(model + 0x1f0);
            for (uint32_t i = 0; i < paletteCount; ++i) {
                int64_t boneIdx = boneIndices[i];
                if (boneIdx < 0) {
                    break;
                }
                MultiplyBone3x4Simd(
                    outMatrices + static_cast<std::size_t>(i) * 12u,
                    bonePalette + static_cast<std::size_t>(boneIdx) * 12u,
                    invBindBase + static_cast<std::size_t>(boneIdx) * 12u);
            }
        }

        ctxVt = *reinterpret_cast<void***>(drawContext);
        reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
            ctxVt[0x20 / 8])(drawContext, 0u, paletteCount, outMatrices);

        FreeUntilTlsLinearHeap(mark);
        return;
    }

    if (paletteCount == 0) {
        uint16_t singleBoneIdx = *reinterpret_cast<uint16_t*>(group + 0x16);
        reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
            ctxVt[0x20 / 8])(
                drawContext,
                0u,
                1u,
                bonePalette + static_cast<std::size_t>(singleBoneIdx) * 12u);
        return;
    }

    uint8_t* linearHeap = GetTlsLinearHeap();
    uintptr_t mark = *reinterpret_cast<uintptr_t*>(linearHeap + 0x10);
    void* allocObj = linearHeap + 0x20;
    auto* allocVt  = *reinterpret_cast<void***>(allocObj);
    float* outMatrices = static_cast<float*>(
        reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(allocVt[0x58 / 8])(
            allocObj, static_cast<uint64_t>(paletteCount) * 48ULL, 0x10ULL));

    const int32_t* boneIndices = *reinterpret_cast<const int32_t* const*>(group + 0x20);
    for (uint32_t i = 0; i < paletteCount; ++i) {
        int64_t boneIdx = boneIndices[i];
        if (boneIdx < 0) {
            break;
        }
        std::memcpy(
            outMatrices + static_cast<std::size_t>(i) * 12u,
            bonePalette + static_cast<std::size_t>(boneIdx) * 12u,
            48);
    }

    ctxVt = *reinterpret_cast<void***>(drawContext);
    reinterpret_cast<void(*)(void*, uint32_t, uint32_t, const float*)>(
        ctxVt[0x20 / 8])(drawContext, 0u, paletteCount, outMatrices);

    FreeUntilTlsLinearHeap(mark);
}

// ============================================================================
// 31. 0x21d0640 (2,621 B) - GXModelMeshDrawSubmitter_SubmitMeshGroupDrawElements
// ============================================================================
void sub_21d0640(
    void* modelRes,
    void* drawContext,
    void* meshEntry,
    void* materialEntry,
    void* skinningInfo,
    uint32_t instanceCount)
{
    auto* model   = static_cast<uint8_t*>(modelRes);
    auto* ctx     = static_cast<uint8_t*>(drawContext);
    auto* mesh    = static_cast<uint8_t*>(meshEntry);
    auto* mat     = static_cast<uint8_t*>(materialEntry);
    auto* skin    = static_cast<uint8_t*>(skinningInfo);

    auto* stateHolder = *reinterpret_cast<uint8_t**>(ctx + 0xf8);
    uint32_t flags170 = *reinterpret_cast<uint32_t*>(ctx + 0x170);
    uint64_t cullTableRow = ((flags170 & 0x200u) != 0u) ? 2ULL : static_cast<uint64_t>((flags170 >> 7) & 1u);
    uint32_t flagBit1 = flags170 & 2u;
    uint32_t flagBit2 = flags170 & 4u;
    uint32_t lodIndex = *reinterpret_cast<uint32_t*>(ctx + 0x18);
    auto* cmdWrapper  = **reinterpret_cast<uint8_t***>(ctx + 0x08);
    uint32_t flagBit3 = flags170 & 8u;

    if ((mat[0x18] & 8u) != 0u) {
        stateHolder[0x18] |= 8u;
    } else {
        stateHolder[0x18] &= ~8u;
    }
    stateHolder[0x30] |= 1u;

    auto* skinCache = *reinterpret_cast<uint8_t**>(skin + 0x08);
    bool useDualBufferCallback = (skinCache != nullptr && mat[0x2c] != 0);
    if (!useDualBufferCallback && (ctx[0x171] & 1u) != 0u && mat[0x1d] != 0) {
        if (skinCache == nullptr) {
            sub_21d1080(model, ctx, mesh, *reinterpret_cast<const float* const*>(skin + 0x00));
            const float* prevPal = *reinterpret_cast<const float* const*>(ctx + 0x180);
            if (prevPal != nullptr) {
                sub_21d1650(model, ctx, mesh, prevPal);
            }
            goto after_skinning_bind;
        }
        if (skinCache[0xa4] != 0) {
            useDualBufferCallback = true;
        }
    }

    if (useDualBufferCallback) {
        auto* ctxVt = *reinterpret_cast<void***>(ctx);
        auto* ownerModel = *reinterpret_cast<uint8_t**>(skinCache + 0xb0);
        void* curArgA = nullptr;
        void* curArgB = nullptr;
        if (ownerModel[0x200] != 0) {
            int64_t curSlot = *reinterpret_cast<int32_t*>(skinCache + 0x98);
            void** arrA = *reinterpret_cast<void***>(skinCache + 0x10);
            void** arrB = *reinterpret_cast<void***>(skinCache + 0x18);
            curArgA = arrA[curSlot];
            curArgB = arrB[curSlot];
        }
        reinterpret_cast<void(*)(void*, void*, void*)>(ctxVt[0x40 / 8])(ctx, curArgA, curArgB);

        ctxVt = *reinterpret_cast<void***>(ctx);
        skinCache = *reinterpret_cast<uint8_t**>(skin + 0x08);
        int32_t curSlot = *reinterpret_cast<int32_t*>(skinCache + 0x98);
        int32_t prevSlotA = curSlot - 1;
        if (curSlot <= 0) {
            prevSlotA += *reinterpret_cast<int32_t*>(skinCache + 0x9c);
        }
        ownerModel = *reinterpret_cast<uint8_t**>(skinCache + 0xb0);
        uint8_t isGlobalBone = ownerModel[0x200];
        void* prevArgA = nullptr;
        if (isGlobalBone != 0) {
            void** arrA = *reinterpret_cast<void***>(skinCache + 0x10);
            prevArgA = arrA[prevSlotA];
        }
        int32_t prevSlotB = curSlot - 1;
        if (curSlot <= 0) {
            prevSlotB += *reinterpret_cast<int32_t*>(skinCache + 0x9c);
        }
        void* prevArgB = nullptr;
        if (isGlobalBone != 0) {
            void** arrB = *reinterpret_cast<void***>(skinCache + 0x18);
            prevArgB = arrB[prevSlotB];
        }
        reinterpret_cast<void(*)(void*, void*, void*)>(ctxVt[0x48 / 8])(ctx, prevArgA, prevArgB);
    } else if (skinCache != nullptr) {
        auto* ctxVt = *reinterpret_cast<void***>(ctx);
        auto* ownerModel = *reinterpret_cast<uint8_t**>(skinCache + 0xb0);
        int64_t curSlot = *reinterpret_cast<int32_t*>(skinCache + 0x98);
        void** curPtrSlot = nullptr;
        if (ownerModel[0x200] != 0) {
            curPtrSlot = *reinterpret_cast<void***>(skinCache + 0x08) + curSlot;
        } else {
            auto* subArr = *reinterpret_cast<uint8_t**>(skinCache + 0x00);
            uint16_t groupSubIdx = *reinterpret_cast<uint16_t*>(mesh + 0x02);
            curPtrSlot = *reinterpret_cast<void***>(subArr + (curSlot * 5) * 8 + 0x08) + groupSubIdx;
        }
        reinterpret_cast<void(*)(void*, void*)>(ctxVt[0x30 / 8])(ctx, *curPtrSlot);

        ctxVt = *reinterpret_cast<void***>(ctx);
        skinCache = *reinterpret_cast<uint8_t**>(skin + 0x08);
        uint16_t groupSubIdx = *reinterpret_cast<uint16_t*>(mesh + 0x02);
        int32_t curSlot2 = *reinterpret_cast<int32_t*>(skinCache + 0x98);
        int32_t prevSlot = curSlot2 - 1;
        if (curSlot2 <= 0) {
            prevSlot += *reinterpret_cast<int32_t*>(skinCache + 0x9c);
        }
        ownerModel = *reinterpret_cast<uint8_t**>(skinCache + 0xb0);
        void** prevPtrSlot = nullptr;
        if (ownerModel[0x200] != 0) {
            prevPtrSlot = *reinterpret_cast<void***>(skinCache + 0x08) + prevSlot;
        } else {
            auto* subArr = *reinterpret_cast<uint8_t**>(skinCache + 0x00);
            prevPtrSlot = *reinterpret_cast<void***>(subArr + (static_cast<int64_t>(prevSlot) * 5) * 8 + 0x08) + groupSubIdx;
        }
        reinterpret_cast<void(*)(void*, void*)>(ctxVt[0x38 / 8])(ctx, *prevPtrSlot);
    } else {
        sub_21d1080(model, ctx, mesh, *reinterpret_cast<const float* const*>(skin + 0x00));
        const float* prevPal = *reinterpret_cast<const float* const*>(ctx + 0x180);
        if (prevPal != nullptr) {
            sub_21d1650(model, ctx, mesh, prevPal);
        }
    }

after_skinning_bind:
    if (flagBit3 != 0u) {
        stateHolder[0x18] |= 1u;
    } else {
        stateHolder[0x18] &= ~1u;
    }
    stateHolder[0x30] |= 1u;

    const int32_t* vbIndices = *reinterpret_cast<const int32_t* const*>(mesh + 0x38);
    const uint8_t* vbTable   = *reinterpret_cast<const uint8_t* const*>(model + 0x170);
    int64_t firstVbIdx       = vbIndices[0];
    uint32_t firstVertexOff  = *reinterpret_cast<const uint32_t*>(vbTable + firstVbIdx * 0x90 + 0x6c);
    uint16_t numVbStreams    = *reinterpret_cast<uint16_t*>(mesh + 0x2a);

    uint32_t uniformBaseVertexFlag = 1u;
    uint32_t sharedBaseVertex      = firstVertexOff;
    for (uint32_t v = 1u; v < numVbStreams; ++v) {
        int64_t vbIdx = vbIndices[v];
        if (*reinterpret_cast<const uint32_t*>(vbTable + vbIdx * 0x90 + 0x6c) != firstVertexOff) {
            uniformBaseVertexFlag = 0u;
            sharedBaseVertex      = 0u;
            break;
        }
    }

    uint16_t numIndexSets = *reinterpret_cast<uint16_t*>(mesh + 0x28);
    if (numIndexSets == 0u) {
        return;
    }

    uint8_t* drawStateClearPtr = cmdWrapper + 0x1670;
    const int32_t* ibIndices = *reinterpret_cast<const int32_t* const*>(mesh + 0x30);
    uint8_t* ibTable         = *reinterpret_cast<uint8_t**>(model + 0x138);
    uint8_t* vbt             = *reinterpret_cast<uint8_t**>(model + 0x170);
    void* customHook         = *reinterpret_cast<void**>(skin + 0x18);
    for (uint32_t idxSet = 0u; idxSet < numIndexSets; ++idxSet) {
        int64_t ibIdx            = ibIndices[idxSet];
        uint8_t* ibEntry         = ibTable + (ibIdx << 5);
        int32_t ibFlags          = *reinterpret_cast<int32_t*>(ibEntry + 0x00);

        if (ibFlags < 0 && flagBit1 == 0u) {
            continue;
        }
        if (ibFlags >= 0 && flagBit2 == 0u) {
            continue;
        }
        uint32_t entryLod = (static_cast<uint32_t>(ibFlags) >> 24) & 0xFu;
        if (entryLod != lodIndex) {
            continue;
        }

        uint32_t cullModeIdx = *reinterpret_cast<uint32_t*>(ibEntry + 0x0c);
        uint8_t mappedCull = static_cast<uint8_t>(kCullModeRemap_4738fa0[cullTableRow][cullModeIdx]);
        stateHolder[0x18] = static_cast<uint8_t>((stateHolder[0x18] & 0xF9u) | ((mappedCull << 1) & 0x06u));
        stateHolder[0x30] |= 1u;

        void* declProvider = *reinterpret_cast<void**>(mesh + 0x40);
        auto* declVt = *reinterpret_cast<void***>(declProvider);
        void* vertexDecl = reinterpret_cast<void*(*)(void*, uint32_t, uint32_t, uint32_t, uint32_t)>(
            declVt[0x10 / 8])(
                declProvider,
                *reinterpret_cast<uint32_t*>(ctx + 0x100),
                *reinterpret_cast<uint32_t*>(ctx + 0x20),
                *reinterpret_cast<uint32_t*>(ctx + 0x104),
                *reinterpret_cast<uint32_t*>(ctx + 0x108));
        if (vertexDecl == nullptr) {
            continue;
        }

        *reinterpret_cast<void**>(cmdWrapper + 0x260) = vertexDecl;
        auto* gnmState = *reinterpret_cast<uint8_t**>(cmdWrapper + 0x08);
        *reinterpret_cast<void**>(gnmState + 0xb730) = vertexDecl;

        auto* ibBufferWrapper = *reinterpret_cast<uint8_t**>(ibEntry + 0x18);
        void* ibGpuObj = *reinterpret_cast<void**>(ibBufferWrapper + 0x10);
        uint32_t ibFormat = *reinterpret_cast<uint32_t*>(ibEntry + 0x08);

        *reinterpret_cast<void**>(cmdWrapper + 0x270) = ibGpuObj;
        *reinterpret_cast<uint32_t*>(cmdWrapper + 0x278) = ibFormat;
        *reinterpret_cast<uint32_t*>(cmdWrapper + 0x27c) = 0u;
        *reinterpret_cast<void**>(gnmState + 0xb740) = ibGpuObj;
        *reinterpret_cast<uint32_t*>(gnmState + 0xb748) = ibFormat;
        *reinterpret_cast<uint32_t*>(gnmState + 0xb74c) = 0u;

        uint16_t curNumVbs = *reinterpret_cast<uint16_t*>(mesh + 0x2a);
        if (curNumVbs != 0u) {
            if ((uniformBaseVertexFlag & 0xFFu) != 0u) {
                for (uint32_t s = 0u; s < curNumVbs; ++s) {
                    int64_t sIdx = vbIndices[s];
                    uint8_t* vbEntry = vbt + sIdx * 0x90;
                    uint32_t byteOff = *reinterpret_cast<uint32_t*>(vbEntry + 0x78);

                    bool handledByHook = false;
                    if (customHook != nullptr) {
                        struct StreamHookArgs {
                            uint32_t streamSlot;
                            uint32_t byteOffset;
                            void*    vbEntryPtr;
                            void*    meshPtr;
                            void*    matPtr;
                        } hookArgs = { s, byteOff, vbEntry, mesh, mat };
                        auto* hookVt = *reinterpret_cast<void***>(customHook);
                        handledByHook = (reinterpret_cast<uint8_t(*)(void*, void*, void*)>(
                            hookVt[0x10 / 8])(customHook, ctx, &hookArgs) != 0);
                    }
                    if (!handledByHook) {
                        void* vbGpuObj = *reinterpret_cast<void**>(vbEntry + 0x70);
                        uint32_t stride = *reinterpret_cast<uint32_t*>(vbEntry + 0x68);
                        *reinterpret_cast<void**>(cmdWrapper + s * 8 + 0x280) = vbGpuObj;
                        *reinterpret_cast<uint32_t*>(cmdWrapper + s * 4 + 0x380) = stride;
                        *reinterpret_cast<uint32_t*>(cmdWrapper + s * 4 + 0x400) = byteOff;

                        auto* gs = *reinterpret_cast<uint8_t**>(cmdWrapper + 0x08);
                        *reinterpret_cast<uint32_t*>(gs + 0x25f0) |= (1u << s);
                        *reinterpret_cast<void**>(gs + s * 8 + 0xb750) = vbGpuObj;
                        *reinterpret_cast<uint32_t*>(gs + s * 4 + 0xb850) = stride;
                        *reinterpret_cast<uint32_t*>(gs + s * 4 + 0xb8d0) = byteOff;
                    }
                }
            } else {
                for (uint32_t s = 0u; s < curNumVbs; ++s) {
                    int64_t sIdx = vbIndices[s];
                    uint8_t* vbEntry = vbt + sIdx * 0x90;
                    uint32_t stride  = *reinterpret_cast<uint32_t*>(vbEntry + 0x68);
                    uint32_t vertOff = *reinterpret_cast<uint32_t*>(vbEntry + 0x6c);
                    uint32_t byteOff = vertOff * stride + *reinterpret_cast<uint32_t*>(vbEntry + 0x78);

                    bool handledByHook = false;
                    if (customHook != nullptr) {
                        struct StreamHookArgs {
                            uint32_t streamSlot;
                            uint32_t byteOffset;
                            void*    vbEntryPtr;
                            void*    meshPtr;
                            void*    matPtr;
                        } hookArgs = { s, byteOff, vbEntry, mesh, mat };
                        auto* hookVt = *reinterpret_cast<void***>(customHook);
                        handledByHook = (reinterpret_cast<uint8_t(*)(void*, void*, void*)>(
                            hookVt[0x10 / 8])(customHook, ctx, &hookArgs) != 0);
                    }
                    if (!handledByHook) {
                        void* vbGpuObj = *reinterpret_cast<void**>(vbEntry + 0x70);
                        *reinterpret_cast<void**>(cmdWrapper + s * 8 + 0x280) = vbGpuObj;
                        *reinterpret_cast<uint32_t*>(cmdWrapper + s * 4 + 0x380) = stride;
                        *reinterpret_cast<uint32_t*>(cmdWrapper + s * 4 + 0x400) = byteOff;

                        auto* gs = *reinterpret_cast<uint8_t**>(cmdWrapper + 0x08);
                        *reinterpret_cast<uint32_t*>(gs + 0x25f0) |= (1u << s);
                        *reinterpret_cast<void**>(gs + s * 8 + 0xb750) = vbGpuObj;
                        *reinterpret_cast<uint32_t*>(gs + s * 4 + 0xb850) = stride;
                        *reinterpret_cast<uint32_t*>(gs + s * 4 + 0xb8d0) = byteOff;
                    }
                }
            }
        }

        void* postVbHook = *reinterpret_cast<void**>(ctx + 0x1b8);
        if (postVbHook != nullptr) {
            const int32_t* streamIndices = *reinterpret_cast<const int32_t* const*>(mesh + 0x38);
            int64_t firstIdx = streamIndices[0];
            uint8_t* firstVbEntry = *reinterpret_cast<uint8_t**>(model + 0x170) + firstIdx * 0x90;
            struct PostHookArgs {
                uint32_t flagOne;
                uint32_t zeroPad;
                void*    vbEntryPtr;
                void*    meshPtr;
                void*    matPtr;
            } postArgs = { 1u, 0u, firstVbEntry, mesh, mat };
            auto* postVt = *reinterpret_cast<void***>(postVbHook);
            reinterpret_cast<void(*)(void*, void*, void*)>(postVt[0x10 / 8])(postVbHook, ctx, &postArgs);
        }

        uint32_t primTopology = (*reinterpret_cast<uint32_t*>(ctx + 0x20) != 0u)
            ? 0x0cu
            : *reinterpret_cast<uint32_t*>(ibEntry + 0x04);
        *reinterpret_cast<uint32_t*>(cmdWrapper + 0x268) = primTopology;
        auto* gs = *reinterpret_cast<uint8_t**>(cmdWrapper + 0x08);
        *reinterpret_cast<uint32_t*>(gs + 0xb738) = primTopology;

        uint8_t dirtyFlags = stateHolder[0x30];
        if ((dirtyFlags & 1u) != 0u) {
            auto* stateTarget = *reinterpret_cast<uint8_t**>(stateHolder + 0x10);
            auto* shaderSub   = *reinterpret_cast<uint8_t**>(stateTarget + 0x18);
            if (shaderSub != nullptr) {
                if (*reinterpret_cast<uint32_t*>(shaderSub + 0x40) == 6u) {
                    stateHolder[0x18] |= 0x80u;
                } else {
                    stateHolder[0x18] &= 0x7Fu;
                }
            }
            auto* renderSys = static_cast<uint8_t*>(g_GlobalRenderSystem_5540dd8);
            uint32_t rasterIdx = *reinterpret_cast<uint32_t*>(stateHolder + 0x18);
            void* rasterState = *reinterpret_cast<void**>(renderSys + static_cast<uint64_t>(rasterIdx) * 8ULL + 0x6e70);
            *reinterpret_cast<void**>(stateTarget + 0x258) = rasterState;
            auto* targetGnm = *reinterpret_cast<uint8_t**>(stateTarget + 0x08);
            *reinterpret_cast<void**>(targetGnm + 0xb970) = rasterState;
            dirtyFlags = static_cast<uint8_t>(stateHolder[0x30] & ~1u);
            stateHolder[0x30] = dirtyFlags;
        }

        if ((dirtyFlags & 2u) != 0u) {
            auto* stateTarget = *reinterpret_cast<uint8_t**>(stateHolder + 0x10);
            void* depthState  = *reinterpret_cast<void**>(stateHolder + 0x20);
            uint8_t stencilRef = stateHolder[0x28];
            *reinterpret_cast<void**>(stateTarget + 0x210) = depthState;
            stateTarget[0x218] = stencilRef;
            auto* targetGnm = *reinterpret_cast<uint8_t**>(stateTarget + 0x08);
            *reinterpret_cast<void**>(targetGnm + 0xbc08) = depthState;
            targetGnm[0xbcb4] = stencilRef;
            stateHolder[0x30] &= ~2u;
        }

        uint32_t indexCount = *reinterpret_cast<uint32_t*>(ibEntry + 0x10);
        auto* ibWrapper2    = *reinterpret_cast<uint8_t**>(ibEntry + 0x18);
        uint32_t startIndex = *reinterpret_cast<uint32_t*>(ibWrapper2 + 0x08) +
                              *reinterpret_cast<uint32_t*>(ibEntry + 0x14);

        std::memset(drawStateClearPtr, 0, 0xb8);
        *reinterpret_cast<uint64_t*>(cmdWrapper + 0x1680) = 0x0eULL;
        *reinterpret_cast<uint64_t*>(cmdWrapper + 0x1688) = 0x08ULL;

        auto* gnmCmd = *reinterpret_cast<uint8_t**>(cmdWrapper + 0x08);
        if (instanceCount == 1u) {
            sub_2169780(gnmCmd, gnmCmd + 0xb730, indexCount, startIndex, sharedBaseVertex);
        } else {
            sub_2169900(gnmCmd, gnmCmd + 0xb730, indexCount, instanceCount, startIndex, sharedBaseVertex);
        }
    }
}

// ============================================================================
// 32. 0x21d0330 (774 B) - GXModelMeshDrawSubmitter_CullSubmeshBoundingBoxesAgainstFrustum
// ============================================================================
void sub_21d0330(
    void* submitter,
    void* modelRes,
    void* passCtxHolder,
    const float* worldMatrix3x4,
    void* boneTransformProvider)
{
    if (submitter == nullptr) {
        // Note: submitter is unused in binary (rdi not read), kept for ABI fidelity
    }
    auto* model  = static_cast<uint8_t*>(modelRes);
    auto* holder = static_cast<uint8_t*>(passCtxHolder);

    struct CullContext {
        void*    selfRefPtr;
        uint8_t  frustumEnableFlag;
        uint8_t  pad[7];
        void*    vtableHolderPtr;
        void*    passHolder;
        void*    vtable;
        void*    passHolderCopy;
        void*    frustumData;
    } cullCtx = {};

    cullCtx.vtable         = g_VtableMeshPassCallback_535f520;
    cullCtx.passHolderCopy = holder;

    auto* viewState = *reinterpret_cast<uint8_t**>(holder + 0x00);
    auto* sceneCtx  = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);

    if (viewState[0x2093] != 0) {
        cullCtx.frustumData = sub_26db050(*reinterpret_cast<void**>(sceneCtx + 0x390));
        sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);
    } else {
        uint32_t camIdx = *reinterpret_cast<uint32_t*>(viewState + 0x2098);
        void** camTable = *reinterpret_cast<void***>(sceneCtx + 0x3a0);
        cullCtx.frustumData = camTable[camIdx];
    }

    cullCtx.passHolder      = holder;
    cullCtx.vtableHolderPtr = &cullCtx.vtable;
    cullCtx.selfRefPtr      = &cullCtx.passHolder;

    void* cullMgr = *reinterpret_cast<void**>(sceneCtx + 0x388);
    cullCtx.frustumEnableFlag = sub_2191c30(cullMgr, viewState, 3u, 0u);
    sub_2192170(cullMgr, *reinterpret_cast<void**>(holder + 0x00), holder + 0xd0, 0u);

    uint32_t numSubmeshAabbs = *reinterpret_cast<uint32_t*>(model + 0x18);
    if (numSubmeshAabbs == 0u) {
        return;
    }

    const __m128 maskXYZ = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(kMaskXYZ_4920cc0)));
    const __m128 unitW   = _mm_load_ps(kUnitW_4920cd0);
    const __m128 zeroVec = _mm_setzero_ps();

    for (uint32_t idx = 0u; idx < numSubmeshAabbs; ++idx) {
        const uint8_t* aabbTable = *reinterpret_cast<const uint8_t* const*>(model + 0x20);
        const float* aabbEntry   = reinterpret_cast<const float*>(aabbTable + static_cast<std::size_t>(idx) * 48u);

        __m128 aabbMin = _mm_or_ps(_mm_and_ps(maskXYZ, _mm_loadu_ps(aabbEntry + 0)), unitW);
        __m128 aabbMax = _mm_or_ps(_mm_and_ps(maskXYZ, _mm_loadu_ps(aabbEntry + 4)), unitW);

        __m128 cmpLt    = _mm_cmplt_ps(aabbMax, aabbMin);
        __m128i shufCmp = _mm_shuffle_epi32(_mm_castps_si128(cmpLt), 0x24);
        __m128i eqZero  = _mm_cmpeq_epi32(shufCmp, _mm_castps_si128(zeroVec));
        int maskBits    = _mm_movemask_ps(_mm_castsi128_ps(eqZero));

        alignas(16) float localAabb[8];
        if (maskBits != 0xF) {
            _mm_store_ps(localAabb + 0, _mm_load_ps(kNegPoint05W1_4920ce0));
            _mm_store_ps(localAabb + 4, _mm_load_ps(kPosPoint05W1_4920cf0));
        } else {
            _mm_store_ps(localAabb + 0, aabbMin);
            _mm_store_ps(localAabb + 4, aabbMax);
        }

        alignas(16) float boneMat[12] = {};
        auto* provVt = *reinterpret_cast<void***>(boneTransformProvider);
        reinterpret_cast<void(*)(void*, uint32_t, float*)>(
            provVt[0x08 / 8])(boneTransformProvider, idx, boneMat);

        const __m128i wRow0i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(worldMatrix3x4 + 0));
        const __m128i wRow1i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(worldMatrix3x4 + 4));
        const __m128i wRow2i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(worldMatrix3x4 + 8));

        const __m128 bRow0 = _mm_load_ps(boneMat + 0);
        const __m128 bRow1 = _mm_load_ps(boneMat + 4);
        const __m128 bRow2 = _mm_load_ps(boneMat + 8);

        auto mulRow = [&](__m128i rowInt) -> __m128 {
            __m128 xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0x00));
            __m128 yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0x55));
            __m128 zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0xAA));
            __m128 wwww = _mm_castsi128_ps(_mm_shuffle_epi32(rowInt, 0xFF));
            __m128 sum  = _mm_add_ps(_mm_mul_ps(bRow0, xxxx), _mm_mul_ps(yyyy, bRow1));
            sum = _mm_add_ps(_mm_mul_ps(zzzz, bRow2), sum);
            return _mm_add_ps(_mm_mul_ps(wwww, unitW), sum);
        };

        __m128 mRow0 = mulRow(wRow0i);
        __m128 mRow1 = mulRow(wRow1i);
        __m128 mRow2 = mulRow(wRow2i);

        // Transpose 3x4 rows (mRow0, mRow1, mRow2, unitW) into 4 columns in worldMatCols[16]
        __m128 t0 = _mm_movelh_ps(mRow0, mRow1);
        __m128 t1 = _mm_movehl_ps(mRow1, mRow0);
        __m128 t2 = _mm_castsi128_ps(_mm_move_epi64(_mm_castps_si128(mRow2)));
        __m128 t3 = _mm_movehl_ps(unitW, mRow2);

        alignas(16) float worldMatCols[16];
        _mm_store_ps(worldMatCols + 0,  _mm_shuffle_ps(t0, t2, 0x88));
        _mm_store_ps(worldMatCols + 4,  _mm_shuffle_ps(t0, t2, 0xDD));
        _mm_store_ps(worldMatCols + 8,  _mm_shuffle_ps(t1, t3, 0x88));
        _mm_store_ps(worldMatCols + 12, _mm_shuffle_ps(t1, t3, 0xDD));

        sub_21900a0(&cullCtx, worldMatCols, localAabb, 0xFFFFFFFFu, 1u, 0u);
    }
}

// ============================================================================
// 33. 0x21d02f0 (54 B) - GXModelMeshDrawSubmitter_SubmitModelPassListDirect
// ============================================================================
uint32_t sub_21d02f0(
    void* submitter,
    void* modelRes,
    void* drawContext,
    uint32_t passCount,
    void** passArray)
{
    if (submitter == nullptr) {
        // Unused self pointer
    }
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* ctx   = static_cast<uint8_t*>(drawContext);
    if (model[0x2cc] != 0) {
        uint32_t maxLod = *reinterpret_cast<uint32_t*>(model + 0x204);
        if (maxLod <= *reinterpret_cast<uint32_t*>(ctx + 0x18)) {
            *reinterpret_cast<uint32_t*>(ctx + 0x18) = maxLod - 1u;
        }
        sub_21cfb30(model, model + 0x210, ctx, passCount, passArray, 0u);
    }
    return 0u;
}

// ============================================================================
// 34. 0x21d0280 (106 B) - GXModelMeshDrawSubmitter_SubmitModelSinglePass
// ============================================================================
uint32_t sub_21d0280(
    void* submitter,
    void* modelRes,
    void* drawContext,
    void* singlePass)
{
    if (submitter == nullptr) {
        // Unused self pointer
    }
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* ctx   = static_cast<uint8_t*>(drawContext);
    if (model[0x2cc] != 0) {
        uint32_t maxLod = *reinterpret_cast<uint32_t*>(model + 0x204);
        if (maxLod <= *reinterpret_cast<uint32_t*>(ctx + 0x18)) {
            *reinterpret_cast<uint32_t*>(ctx + 0x18) = maxLod - 1u;
        }
        void* passArr[1] = { singlePass };
        sub_21cfb30(model, model + 0x210, ctx, 1u, passArr, 0u);
    }
    return 0u;
}

// ============================================================================
// 35. 0x21d01a0 (211 B) - GXModelMeshDrawSubmitter_SubmitModelWithSkinningOverrideB
// ============================================================================
uint32_t sub_21d01a0(
    void* submitter,
    void* modelRes,
    void* drawContext,
    void* skinCacheOverride)
{
    if (submitter == nullptr) {
        // Unused self pointer
    }
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* ctx   = static_cast<uint8_t*>(drawContext);
    if (model[0x2cc] != 0) {
        uint32_t maxLod = *reinterpret_cast<uint32_t*>(model + 0x204);
        if (maxLod <= *reinterpret_cast<uint32_t*>(ctx + 0x18)) {
            *reinterpret_cast<uint32_t*>(ctx + 0x18) = maxLod - 1u;
        }

        struct LocalPassEntry {
            void*    bonePalette;
            void*    skinCache;
            void*    prePassHook;
            void*    customStreamHook;
            uint64_t maskLo;
            uint64_t maskHi;
            uint64_t groupMask0;
            uint64_t groupMask1;
        } localPass = {};

        localPass.bonePalette      = nullptr;
        localPass.skinCache        = skinCacheOverride;
        localPass.prePassHook      = nullptr;
        localPass.customStreamHook = *reinterpret_cast<void**>(ctx + 0x1b0);
        localPass.maskLo           = *reinterpret_cast<uint64_t*>(ctx + 0x30);
        localPass.maskHi           = *reinterpret_cast<uint64_t*>(ctx + 0x38);
        localPass.groupMask0       = 0xFFFFFFFFFFFFFFFFULL;
        localPass.groupMask1       = 0xFFFFFFFFFFFFFFFFULL;

        void* matResolver = *reinterpret_cast<void**>(ctx + 0x190);
        if (matResolver == nullptr) {
            matResolver = model + 0x210;
        }
        void* passArr[1] = { &localPass };
        sub_21cfb30(model, matResolver, ctx, 1u, passArr, 1u);
    }
    return 0u;
}

// ============================================================================
// 36. 0x21cfb30 (1,640 B) - GXModelMeshDrawSubmitter_DispatchModelPassDrawList
// ============================================================================
uint32_t sub_21cfb30(
    void* modelRes,
    void* materialResolver,
    void* drawContext,
    uint32_t passCount,
    void** passArray,
    uint8_t skipPrePass)
{
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* ctx   = static_cast<uint8_t*>(drawContext);

    uint32_t flags170   = *reinterpret_cast<uint32_t*>(ctx + 0x170);
    uint32_t reqMask    = (flags170 & 2u) | ((flags170 >> 2) & 1u);
    uint32_t origLod    = *reinterpret_cast<uint32_t*>(ctx + 0x18);
    __m128i ctxMask128  = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ctx + 0x30));
    uint32_t ctxMask40  = *reinterpret_cast<uint32_t*>(ctx + 0x40);

    bool multiPassNoInstFlag = (passCount >= 2u) && ((ctx[0x179] & 1u) == 0u);
    if (!multiPassNoInstFlag && skipPrePass == 0u) {
        *reinterpret_cast<uint32_t*>(ctx + 0x108) =
            (*reinterpret_cast<uint32_t*>(ctx + 0x178) >> 8) & 1u;
        auto* firstPass = static_cast<uint8_t*>(passArray[0]);
        void* preHook   = *reinterpret_cast<void**>(firstPass + 0x10);
        if (preHook != nullptr) {
            auto* vt = *reinterpret_cast<void***>(preHook);
            reinterpret_cast<void(*)(void*, void*, uint32_t)>(vt[0x10 / 8])(preHook, ctx, 0u);
        }
    }

    if (origLod != 0u) {
        uint32_t lodBitmask = *reinterpret_cast<uint32_t*>(model + 0xb4);
        if (((lodBitmask >> origLod) & 1u) != 0u) {
            uint32_t probe = origLod - 1u;
            uint32_t resolvedLod = 0u;
            while (probe != 0u) {
                uint32_t nextProbe = probe - 1u;
                bool bitSet = (((lodBitmask >> probe) & 1u) != 0u);
                probe = nextProbe;
                if (bitSet) {
                    resolvedLod = nextProbe + 1u;
                    break;
                }
            }
            *reinterpret_cast<uint32_t*>(ctx + 0x18) = resolvedLod;
        }
    }

    int32_t meshEntryCount = *reinterpret_cast<int32_t*>(model + 0xa0);
    if (meshEntryCount <= 0) {
        *reinterpret_cast<uint32_t*>(ctx + 0x18) = origLod;
        return 0u;
    }

    uint32_t modeFlag = (*reinterpret_cast<uint32_t*>(ctx + 0x178) >> 8) & 1u;
    bool multiPassMode = multiPassNoInstFlag && (passCount > 1u);
    void* activeMaterial = nullptr;
    uint32_t activeSubMatId = 0xFFFFFFFFu;

    for (int32_t meshIdx = 0; meshIdx < meshEntryCount; ++meshIdx) {
        uint8_t* meshTable = *reinterpret_cast<uint8_t**>(model + 0xa8);
        uint8_t* meshEntry = meshTable + static_cast<std::size_t>(meshIdx) * 0x48u;

        uint32_t entryFlags = *reinterpret_cast<uint32_t*>(meshEntry + 0x10);
        if ((entryFlags & reqMask) != reqMask) {
            continue;
        }

        uint16_t entryLodMask = *reinterpret_cast<uint16_t*>(meshEntry + 0x14);
        uint32_t curLod       = *reinterpret_cast<uint32_t*>(ctx + 0x18);
        if (((entryLodMask >> curLod) & 1u) == 0u) {
            continue;
        }

        auto* resolverVt = *reinterpret_cast<void***>(materialResolver);
        uint16_t matIndex = *reinterpret_cast<uint16_t*>(meshEntry + 0x00);
        auto* resolvedMat = static_cast<uint8_t*>(
            reinterpret_cast<void*(*)(void*, uint32_t)>(resolverVt[0x18 / 8])(materialResolver, matIndex));
        if (resolvedMat == nullptr) {
            continue;
        }

        if ((*reinterpret_cast<uint32_t*>(resolvedMat + 0x18) & ctxMask40) == 0u) {
            continue;
        }

        __m128i matMask128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(resolvedMat + 0x08));
        if (_mm_testz_si128(ctxMask128, matMask128) != 0) {
            continue;
        }

        uint32_t subMatId = *reinterpret_cast<uint32_t*>(meshEntry + 0x08);
        if (activeMaterial != resolvedMat || activeSubMatId != subMatId) {
            if (activeMaterial != nullptr) {
                void* listenerA = *reinterpret_cast<void**>(ctx + 0x1a0);
                if (listenerA != nullptr) {
                    void* matArg = activeMaterial;
                    auto* vt = *reinterpret_cast<void***>(listenerA);
                    reinterpret_cast<void(*)(void*, void*, void**)>(vt[0x20 / 8])(listenerA, ctx, &matArg);
                }
                void* listenerB = *reinterpret_cast<void**>(ctx + 0x1a8);
                if (listenerB != nullptr) {
                    void* matArg = activeMaterial;
                    auto* vt = *reinterpret_cast<void***>(listenerB);
                    reinterpret_cast<void(*)(void*, void*, void**)>(vt[0x20 / 8])(listenerB, ctx, &matArg);
                }
                auto* actVt = *reinterpret_cast<void***>(activeMaterial);
                reinterpret_cast<void(*)(void*, void*)>(actVt[0x20 / 8])(activeMaterial, ctx);
            }

            auto* matVt = *reinterpret_cast<void***>(resolvedMat);
            if (reinterpret_cast<uint8_t(*)(void*, uint32_t, void*)>(
                    matVt[0x10 / 8])(resolvedMat, subMatId, ctx) == 0u)
            {
                activeMaterial = nullptr;
                continue;
            }

            void* listenerA = *reinterpret_cast<void**>(ctx + 0x1a0);
            if (listenerA != nullptr) {
                void* pairArgs[2] = { resolvedMat, meshEntry };
                auto* vt = *reinterpret_cast<void***>(listenerA);
                reinterpret_cast<void(*)(void*, void*, void**)>(vt[0x18 / 8])(listenerA, ctx, pairArgs);
            }
            void* listenerB = *reinterpret_cast<void**>(ctx + 0x1a8);
            if (listenerB != nullptr) {
                void* pairArgs[2] = { resolvedMat, meshEntry };
                auto* vt = *reinterpret_cast<void***>(listenerB);
                reinterpret_cast<void(*)(void*, void*, void**)>(vt[0x18 / 8])(listenerB, ctx, pairArgs);
            }

            activeSubMatId = *reinterpret_cast<uint32_t*>(meshEntry + 0x08);
            activeMaterial = resolvedMat;
        }

        if (modeFlag == 0u) {
            if (*reinterpret_cast<uint32_t*>(ctx + 0x108) == 1u) {
                auto* firstPass = static_cast<uint8_t*>(passArray[0]);
                void* preHook   = *reinterpret_cast<void**>(firstPass + 0x10);
                if (preHook != nullptr) {
                    auto* vt = *reinterpret_cast<void***>(preHook);
                    reinterpret_cast<void(*)(void*, void*, uint32_t)>(vt[0x10 / 8])(preHook, ctx, 0u);
                }
            }
        }

        uint32_t curMode108 = *reinterpret_cast<uint32_t*>(ctx + 0x108);
        uint32_t callPreHookPerPass = 0u;
        if (curMode108 == 0u && multiPassMode) {
            modeFlag = 0u;
            callPreHookPerPass = 1u;
        } else if (curMode108 == 1u) {
            callPreHookPerPass = (g_GlobalMeshPassFlag_5540dd5 != 0u) ? 1u : 0u;
            modeFlag = 1u;
        } else {
            modeFlag = curMode108;
            callPreHookPerPass = 0u;
        }

        if (static_cast<int32_t>(passCount) > 0) {
            uint64_t hiBit = (static_cast<uint32_t>(meshIdx) > 0x3Fu)
                ? (1ULL << (static_cast<uint32_t>(meshIdx) - 0x40u))
                : 0ULL;
            uint64_t loBit = (static_cast<uint32_t>(meshIdx) <= 0x3Fu)
                ? (1ULL << static_cast<uint32_t>(meshIdx))
                : 0ULL;

            for (uint32_t p = 0u; p < passCount; ++p) {
                auto* passDesc = static_cast<uint8_t*>(passArray[p]);
                if (modeFlag == 0u) {
                    __m128i m0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(resolvedMat + 0x08));
                    __m128i p0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(passDesc + 0x20));
                    if (_mm_testz_si128(m0, p0) != 0) {
                        continue;
                    }
                    uint64_t maskLo = *reinterpret_cast<uint64_t*>(passDesc + 0x30) & loBit;
                    uint64_t maskHi = *reinterpret_cast<uint64_t*>(passDesc + 0x38) & hiBit;
                    if ((maskLo | maskHi) == 0ULL) {
                        continue;
                    }
                }

                void* passHook = *reinterpret_cast<void**>(passDesc + 0x10);
                if (passHook != nullptr) {
                    if (callPreHookPerPass != 0u) {
                        auto* vt = *reinterpret_cast<void***>(passHook);
                        reinterpret_cast<void(*)(void*, void*, uint32_t)>(vt[0x10 / 8])(passHook, ctx, p);
                        passHook = *reinterpret_cast<void**>(passDesc + 0x10);
                    }
                    auto* vt = *reinterpret_cast<void***>(passHook);
                    uint16_t mIdx = *reinterpret_cast<uint16_t*>(meshEntry + 0x00);
                    reinterpret_cast<void(*)(void*, void*, uint32_t, void*)>(
                        vt[0x18 / 8])(passHook, ctx, mIdx, resolvedMat);
                }

                bool doSubmitSingle = false;
                if (passCount == 1u) {
                    doSubmitSingle = true;
                } else {
                    uint32_t m108 = *reinterpret_cast<uint32_t*>(ctx + 0x108);
                    if (m108 == 0u) {
                        doSubmitSingle = true;
                    } else if (m108 == 1u && g_GlobalMeshPassFlag_5540dd5 != 0u) {
                        doSubmitSingle = true;
                    }
                }

                if (doSubmitSingle) {
                    sub_21d0640(model, ctx, meshEntry, resolvedMat, passDesc, 1u);
                }
            }
        }

        if (passCount >= 2u && *reinterpret_cast<uint32_t*>(ctx + 0x108) != 0u && g_GlobalMeshPassFlag_5540dd5 == 0u) {
            sub_21d0640(model, ctx, meshEntry, resolvedMat, passArray[0], passCount);
        }

        modeFlag = *reinterpret_cast<uint32_t*>(ctx + 0x108);
    }

    *reinterpret_cast<uint32_t*>(ctx + 0x18) = origLod;
    if (activeMaterial != nullptr) {
        void* listenerA = *reinterpret_cast<void**>(ctx + 0x1a0);
        if (listenerA != nullptr) {
            void* matArg = activeMaterial;
            auto* vt = *reinterpret_cast<void***>(listenerA);
            reinterpret_cast<void(*)(void*, void*, void**)>(vt[0x20 / 8])(listenerA, ctx, &matArg);
        }
        void* listenerB = *reinterpret_cast<void**>(ctx + 0x1a8);
        if (listenerB != nullptr) {
            void* matArg = activeMaterial;
            auto* vt = *reinterpret_cast<void***>(listenerB);
            reinterpret_cast<void(*)(void*, void*, void**)>(vt[0x20 / 8])(listenerB, ctx, &matArg);
        }
        auto* actVt = *reinterpret_cast<void***>(activeMaterial);
        reinterpret_cast<void(*)(void*, void*)>(actVt[0x20 / 8])(activeMaterial, ctx);
    }

    return 0u;
}

// ============================================================================
// 37. 0x21cfa50 (211 B) - GXModelMeshDrawSubmitter_SubmitModelWithSkinningOverrideA
// ============================================================================
uint32_t sub_21cfa50(
    void* submitter,
    void* modelRes,
    void* drawContext,
    void* bonePaletteOverride)
{
    if (submitter == nullptr) {
        // Unused self pointer
    }
    auto* model = static_cast<uint8_t*>(modelRes);
    auto* ctx   = static_cast<uint8_t*>(drawContext);
    if (model[0x2cc] != 0) {
        uint32_t maxLod = *reinterpret_cast<uint32_t*>(model + 0x204);
        if (maxLod <= *reinterpret_cast<uint32_t*>(ctx + 0x18)) {
            *reinterpret_cast<uint32_t*>(ctx + 0x18) = maxLod - 1u;
        }

        struct LocalPassEntry {
            void*    bonePalette;
            void*    skinCache;
            void*    prePassHook;
            void*    customStreamHook;
            uint64_t maskLo;
            uint64_t maskHi;
            uint64_t groupMask0;
            uint64_t groupMask1;
        } localPass = {};

        localPass.bonePalette      = bonePaletteOverride;
        localPass.skinCache        = nullptr;
        localPass.prePassHook      = nullptr;
        localPass.customStreamHook = *reinterpret_cast<void**>(ctx + 0x1b0);
        localPass.maskLo           = *reinterpret_cast<uint64_t*>(ctx + 0x30);
        localPass.maskHi           = *reinterpret_cast<uint64_t*>(ctx + 0x38);
        localPass.groupMask0       = 0xFFFFFFFFFFFFFFFFULL;
        localPass.groupMask1       = 0xFFFFFFFFFFFFFFFFULL;

        void* matResolver = *reinterpret_cast<void**>(ctx + 0x190);
        if (matResolver == nullptr) {
            matResolver = model + 0x210;
        }
        void* passArr[1] = { &localPass };
        sub_21cfb30(model, matResolver, ctx, 1u, passArr, 1u);
    }
    return 0u;
}

// ============================================================================
// 38. 0x21cfa40 (1 B) - GXModelMeshDrawSubmitter_NoOp
// ============================================================================
void sub_21cfa40() {
}

// ============================================================================
// 39. 0x21cfa30 (5 B) - GXModelMeshDrawSubmitter_DeletingDestructor
// ============================================================================
void sub_21cfa30(void* self) {
    sub_2bbe588(self);
}

// ============================================================================
// 40. 0x21cfa20 (11 B) - GXModelMeshDrawSubmitter_Constructor
// ============================================================================
void sub_21cfa20(void* self) {
    *reinterpret_cast<void**>(self) = g_VtableMeshDrawSubmitter_535fe70;
}

// ============================================================================
// 41. 0x21cf900 (130 B) - GXModelMeshResource_ReleaseConstantBufferAndCleanup
// ============================================================================
void sub_21cf900(void* self) {
    auto* model    = static_cast<uint8_t*>(self);
    auto* sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);
    void* cbMgr    = *reinterpret_cast<void**>(sceneCtx + 0x650);
    if (cbMgr != nullptr) {
        sub_21ad270(*reinterpret_cast<void**>(cbMgr), *reinterpret_cast<void**>(model + 0xf0));
    }

    auto* cbRes = *reinterpret_cast<uint8_t**>(model + 0xf0);
    if (cbRes != nullptr) {
        int32_t prev = __sync_fetch_and_add(reinterpret_cast<int32_t*>(cbRes + 0x10), -1);
        if (prev == 1) {
            auto* resVt = *reinterpret_cast<void***>(cbRes);
            void* heap  = *reinterpret_cast<void**>(cbRes + 0x08);
            reinterpret_cast<void(*)(void*)>(resVt[0])(cbRes);
            auto* heapVt = *reinterpret_cast<void***>(heap);
            reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, cbRes);
        }
        *reinterpret_cast<void**>(model + 0xf0) = nullptr;
    }

    sub_21c6ae0(self);
}

// ============================================================================
// 42. 0x21cf870 (135 B) - GXModelMeshResource_InitializeFromDefaultDesc
// ============================================================================
uint8_t sub_21cf870(void* self, void* rawFlverData, void* /*unusedRdx*/, void* ownerArg) {
    alignas(32) uint8_t stackDesc[0x68] = {};
    if (sub_21ce5d0(stackDesc, rawFlverData) == 0u) {
        return 0u;
    }
    auto* vt = *reinterpret_cast<void***>(self);
    return reinterpret_cast<uint8_t(*)(void*, void*, void*, uint32_t, uint32_t)>(
        vt[0x28 / 8])(self, stackDesc, ownerArg, 0u, 0u);
}

// ============================================================================
// 43. 0x21cf3c0 (1,200 B) - GXModelMeshResource_InitializeGpuBuffersAndStreams
// ============================================================================
uint8_t sub_21cf3c0(
    void* self,
    void* flverContainer,
    void* resourceContext,
    uint8_t deferBufferUpload,
    uint32_t streamMaskFlags)
{
    auto* model = static_cast<uint8_t*>(self);
    auto* cont  = static_cast<uint8_t*>(flverContainer);
    auto* hdr   = *reinterpret_cast<uint8_t**>(cont + 0x00);

    if (*reinterpret_cast<uint32_t*>(hdr + 0x08) < 0x20000u) {
        return 0u;
    }
    if (hdr[0x49] == 0u) {
        return 0u;
    }

    sub_e51f00(model + 0x10, cont);
    sub_e51350(model + 0x38, cont);

    hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
    sub_e512b0(model + 0x58, *reinterpret_cast<uint32_t*>(hdr + 0x1c));
    sub_e516e0(model + 0x58, model + 0x38, model + 0x10);

    sub_21cdcd0(model + 0x268, cont, resourceContext);
    sub_21c7660(model + 0x210, cont, model + 0x268, resourceContext);
    sub_e50490(model + 0x98, cont);

    hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
    model[0x200] = static_cast<uint8_t>((hdr[0x68] >> 2) & 1u);
    model[0x201] = (*reinterpret_cast<uint64_t*>(model + 0xd8) != *reinterpret_cast<uint64_t*>(model + 0xd0)) ? 1u : 0u;

    sub_e4de20(model + 0x78, cont, model + 0x38, model + 0x10);

    if (deferBufferUpload == 0u) {
        sub_21cf1c0(model, cont);
    }

    uint32_t ibMaskBits = streamMaskFlags & 0x0Cu;
    uint32_t ibFlagEqC  = (ibMaskBits == 0x0Cu) ? 1u : 0u;
    sub_e4efb0(model + 0xf8, cont, *reinterpret_cast<void**>(model + 0xf0), ibFlagEqC, deferBufferUpload);

    uint32_t vbMaskBits = streamMaskFlags & 0x03u;
    uint32_t vbFlagEq3  = (vbMaskBits == 0x03u) ? 1u : 0u;
    sub_e52950(model + 0x160, cont, *reinterpret_cast<void**>(model + 0xf0), vbFlagEq3, deferBufferUpload);

    sub_e4faa0(model + 0x188, cont, resourceContext);

    hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
    model[0x1f8] = (hdr[0x4a] != 0u) ? 1u : 0u;
    *reinterpret_cast<uint32_t*>(model + 0x1fc) = *reinterpret_cast<uint32_t*>(hdr + 0x4c);

    if ((streamMaskFlags & 0x0Fu) != 0x0Fu) {
        uint32_t ibPending = (ibMaskBits == 0x0Cu) ? 0u : ibMaskBits;
        uint32_t vbPending = (vbMaskBits == 0x03u) ? 0u : vbMaskBits;
        if ((ibPending | vbPending) != 0u && *reinterpret_cast<int32_t*>(model + 0xa0) > 0) {
            uint32_t maskNonSkinned = streamMaskFlags & 0x0Au;
            uint32_t maskSkinned    = streamMaskFlags & 0x05u;
            int32_t meshCount       = *reinterpret_cast<int32_t*>(model + 0xa0);

            for (int32_t m = 0; m < meshCount; ++m) {
                uint8_t* meshTable = *reinterpret_cast<uint8_t**>(model + 0xa8);
                void**   matTable  = *reinterpret_cast<void***>(model + 0x220);
                uint8_t* meshEntry = meshTable + static_cast<std::size_t>(m) * 0x48u;
                uint16_t matIdx    = *reinterpret_cast<uint16_t*>(meshEntry + 0x00);
                auto*    matDesc   = static_cast<uint8_t*>(matTable[matIdx]);

                if (matDesc[0x1d] == 0u) {
                    if (maskNonSkinned == 0u) {
                        continue;
                    }
                } else {
                    if (maskSkinned == 0u) {
                        continue;
                    }
                }

                // Allocate CPU shadow copies of index buffers if ibPending != 0
                if (ibPending != 0u) {
                    uint16_t numIbs = *reinterpret_cast<uint16_t*>(meshEntry + 0x28);
                    const int32_t* ibIdxArr = *reinterpret_cast<const int32_t* const*>(meshEntry + 0x30);
                    for (uint32_t i = 0u; i < numIbs; ++i) {
                        int64_t ibIdx = ibIdxArr[i];
                        uint8_t* ibTable = *reinterpret_cast<uint8_t**>(model + 0x138);
                        uint8_t* ibSlot  = ibTable + (ibIdx << 5);
                        uint8_t* ibObj   = *reinterpret_cast<uint8_t**>(ibSlot + 0x18);
                        if (*reinterpret_cast<void**>(ibObj + 0x20) == nullptr) {
                            auto* flverHdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
                            auto* flverIbTable = *reinterpret_cast<uint8_t**>(cont + 0x28);
                            uint32_t ver = *reinterpret_cast<uint32_t*>(flverHdr + 0x08);
                            uint8_t* dataSection = flverHdr + *reinterpret_cast<uint32_t*>(flverHdr + 0x0c);
                            uint32_t ibHeaderStride = (ver > 0x20008u) ? 0x20u : 0x10u;
                            uint8_t* flverIb = flverIbTable + static_cast<uint32_t>(ibIdx) * ibHeaderStride;
                            const uint8_t* srcData = dataSection + *reinterpret_cast<uint32_t*>(flverIb + 0x0c);

                            uint32_t byteSize = 0u;
                            if (ver >= 0x20009u) {
                                byteSize = *reinterpret_cast<uint32_t*>(flverIb + 0x10);
                            } else {
                                uint32_t idxCount = *reinterpret_cast<uint32_t*>(flverIb + 0x08);
                                int32_t bitsPerIdx = static_cast<int8_t>(flverHdr[0x48]);
                                if (bitsPerIdx == 0) {
                                    bitsPerIdx = 16;
                                }
                                byteSize = (static_cast<uint32_t>(bitsPerIdx) * idxCount) >> 3;
                            }

                            void* memAlloc = *reinterpret_cast<void**>(model + 0x2c0);
                            auto* memVt = *reinterpret_cast<void***>(memAlloc);
                            void* dstBuf = reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(
                                memVt[0x58 / 8])(memAlloc, byteSize, 0x80ULL);
                            *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(ibSlot + 0x18) + 0x20) = dstBuf;
                            if (dstBuf == nullptr) {
                                goto finish_buffer_init;
                            }
                            sub_261a1b0(dstBuf, srcData, byteSize);
                            numIbs   = *reinterpret_cast<uint16_t*>(meshEntry + 0x28);
                            ibIdxArr = *reinterpret_cast<const int32_t* const*>(meshEntry + 0x30);
                        }
                    }
                }

                // Allocate CPU shadow copies of vertex buffers if vbPending != 0
                if (vbPending != 0u) {
                    uint16_t numVbs = *reinterpret_cast<uint16_t*>(meshEntry + 0x2a);
                    const int32_t* vbIdxArr = *reinterpret_cast<const int32_t* const*>(meshEntry + 0x38);
                    for (uint32_t v = 0u; v < numVbs; ++v) {
                        int64_t vbIdx = vbIdxArr[v];
                        uint8_t* vbTable = *reinterpret_cast<uint8_t**>(model + 0x170);
                        uint8_t* vbSlot  = vbTable + vbIdx * 0x90;
                        if (*reinterpret_cast<void**>(vbSlot + 0x80) == nullptr) {
                            auto* flverHdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
                            auto* flverVbTable = *reinterpret_cast<uint8_t**>(cont + 0x30);
                            uint32_t ver = *reinterpret_cast<uint32_t*>(flverHdr + 0x08);
                            uint8_t* dataSection = flverHdr + *reinterpret_cast<uint32_t*>(flverHdr + 0x0c);
                            uint8_t* flverVb = flverVbTable + (static_cast<uint64_t>(static_cast<uint32_t>(vbIdx)) << 5);
                            const uint8_t* srcData = dataSection + *reinterpret_cast<uint32_t*>(flverVb + 0x1c);

                            uint32_t byteSize = (ver >= 0x20008u)
                                ? *reinterpret_cast<uint32_t*>(flverVb + 0x18)
                                : (*reinterpret_cast<uint32_t*>(flverVb + 0x0c) *
                                   *reinterpret_cast<uint32_t*>(flverVb + 0x08));

                            void* memAlloc = *reinterpret_cast<void**>(model + 0x2c0);
                            auto* memVt = *reinterpret_cast<void***>(memAlloc);
                            void* dstBuf = reinterpret_cast<void*(*)(void*, uint64_t, uint64_t)>(
                                memVt[0x58 / 8])(memAlloc, byteSize, 0x80ULL);
                            *reinterpret_cast<void**>(vbSlot + 0x80) = dstBuf;
                            if (dstBuf == nullptr) {
                                goto finish_buffer_init;
                            }
                            sub_261a1b0(dstBuf, srcData, byteSize);
                            numVbs   = *reinterpret_cast<uint16_t*>(meshEntry + 0x2a);
                            vbIdxArr = *reinterpret_cast<const int32_t* const*>(meshEntry + 0x38);
                        }
                    }
                }
            }
        }
    }

finish_buffer_init:
    hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
    *reinterpret_cast<uint32_t*>(model + 0x2c8) =
        (*reinterpret_cast<uint32_t*>(hdr + 0x10) + 3u) & ~3u;
    if (deferBufferUpload == 0u) {
        model[0x2cc] = 1u;
    }
    return 1u;
}

// ============================================================================
// 44. 0x21cf1c0 (497 B) - GXModelMeshResource_CreateAndRegisterConstantBuffer
// ============================================================================
uint8_t sub_21cf1c0(void* modelRes, void* flverContainer) {
    auto* model    = static_cast<uint8_t*>(modelRes);
    auto* cont     = static_cast<uint8_t*>(flverContainer);
    auto* sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);

    void* cbMgr = *reinterpret_cast<void**>(sceneCtx + 0x650);
    if (cbMgr != nullptr) {
        sub_21ad270(*reinterpret_cast<void**>(cbMgr), *reinterpret_cast<void**>(model + 0xf0));
    }

    auto* oldCb = *reinterpret_cast<uint8_t**>(model + 0xf0);
    if (oldCb != nullptr) {
        int32_t prev = __sync_fetch_and_add(reinterpret_cast<int32_t*>(oldCb + 0x10), -1);
        if (prev == 1) {
            auto* resVt = *reinterpret_cast<void***>(oldCb);
            void* heap  = *reinterpret_cast<void**>(oldCb + 0x08);
            reinterpret_cast<void(*)(void*)>(resVt[0])(oldCb);
            auto* heapVt = *reinterpret_cast<void***>(heap);
            reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, oldCb);
        }
        *reinterpret_cast<void**>(model + 0xf0) = nullptr;
    }

    auto* hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
    uint32_t rawDataSize = *reinterpret_cast<uint32_t*>(hdr + 0x10);
    if (rawDataSize == 0u) {
        return 1u;
    }

    void* dev = *reinterpret_cast<void**>(sceneCtx + 0x00);
    uint32_t alignedSize = (rawDataSize + 3u) & ~3u;
    uint32_t bufDesc[6] = { alignedSize, 1u, 3u, 0u, 0u, 0u };
    struct BufInitData {
        void*    data;
        uint32_t pitch;
        uint32_t slicePitch;
    } initData = { *reinterpret_cast<void**>(cont + 0x58), 0u, 0u };

    uint8_t* createdBuf = nullptr;
    if (sub_2165c10(dev, &createdBuf, bufDesc, &initData, 0u, 0u, nullptr) != 0) {
        return 0u;
    }

    sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);
    cbMgr    = *reinterpret_cast<void**>(sceneCtx + 0x650);
    if (cbMgr != nullptr) {
        sub_21ace70(*reinterpret_cast<void**>(cbMgr), createdBuf);
    }

    auto* curCb = *reinterpret_cast<uint8_t**>(model + 0xf0);
    if (curCb != createdBuf) {
        if (curCb != nullptr) {
            int32_t prev = __sync_fetch_and_add(reinterpret_cast<int32_t*>(curCb + 0x10), -1);
            if (prev == 1) {
                auto* resVt = *reinterpret_cast<void***>(curCb);
                void* heap  = *reinterpret_cast<void**>(curCb + 0x08);
                reinterpret_cast<void(*)(void*)>(resVt[0])(curCb);
                auto* heapVt = *reinterpret_cast<void***>(heap);
                reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, curCb);
            }
            *reinterpret_cast<void**>(model + 0xf0) = nullptr;
        }
        *reinterpret_cast<void**>(model + 0xf0) = createdBuf;
        if (createdBuf != nullptr) {
            __sync_fetch_and_add(reinterpret_cast<int32_t*>(createdBuf + 0x10), 1);
            curCb = *reinterpret_cast<uint8_t**>(model + 0xf0);
        } else {
            curCb = nullptr;
        }
    } else {
        curCb = createdBuf;
    }

    if (curCb != nullptr) {
        int32_t prev = __sync_fetch_and_add(reinterpret_cast<int32_t*>(curCb + 0x10), -1);
        if (prev == 1) {
            void* heap  = *reinterpret_cast<void**>(curCb + 0x08);
            auto* resVt = *reinterpret_cast<void***>(curCb);
            reinterpret_cast<void(*)(void*)>(resVt[0])(curCb);
            auto* heapVt = *reinterpret_cast<void***>(heap);
            reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, curCb);
        }
    }

    return 1u;
}

// ============================================================================
// 45. 0x21cf130 (131 B) - GXModelMeshResource_EnsureBuffersInitialized
// ============================================================================
uint8_t sub_21cf130(void* self, void* flverContainer) {
    auto* model = static_cast<uint8_t*>(self);
    if (model[0x2cc] != 0u) {
        return 1u;
    }
    if (sub_21cf1c0(model, flverContainer) == 0u) {
        return 0u;
    }
    if (sub_e4f630(model + 0xf8, flverContainer, *reinterpret_cast<void**>(model + 0xf0)) == 0u) {
        return 0u;
    }
    if (sub_e52fe0(model + 0x160, flverContainer, *reinterpret_cast<void**>(model + 0xf0)) == 0u) {
        return 0u;
    }
    model[0x2cc] = 1u;
    return 1u;
}

// ============================================================================
// 46. 0x21cf0d0 (94 B) - GXModelMeshResource_ReloadAndSetup
// ============================================================================
uint8_t sub_21cf0d0(
    void* self,
    void* flverContainer,
    void* resourceContext,
    uint8_t deferFlag,
    uint32_t streamMaskFlags)
{
    auto* vt = *reinterpret_cast<void***>(self);
    reinterpret_cast<void(*)(void*)>(vt[0x20 / 8])(self);

    vt = *reinterpret_cast<void***>(self);
    uint8_t ok = reinterpret_cast<uint8_t(*)(void*, void*, void*, uint32_t, uint32_t)>(
        vt[0x28 / 8])(self, flverContainer, resourceContext, deferFlag, streamMaskFlags);
    if (ok != 0u) {
        sub_21c67a0(self, resourceContext);
    }
    return ok;
}

// ============================================================================
// 47. 0x21cf030 (149 B) - GXModelMeshResource_Destructor
// ============================================================================
void sub_21cf030(void* self) {
    auto* model = static_cast<uint8_t*>(self);
    *reinterpret_cast<void**>(model + 0x00) = g_VtableModelMeshResource_53b6f60;

    auto* sceneCtx = static_cast<uint8_t*>(g_GlobalSceneRendererContext_55406c8);
    void* cbMgr    = *reinterpret_cast<void**>(sceneCtx + 0x650);
    if (cbMgr != nullptr) {
        sub_21ad270(*reinterpret_cast<void**>(cbMgr), *reinterpret_cast<void**>(model + 0xf0));
    }

    auto* cbRes = *reinterpret_cast<uint8_t**>(model + 0xf0);
    if (cbRes != nullptr) {
        int32_t prev = __sync_fetch_and_add(reinterpret_cast<int32_t*>(cbRes + 0x10), -1);
        if (prev == 1) {
            auto* resVt = *reinterpret_cast<void***>(cbRes);
            void* heap  = *reinterpret_cast<void**>(cbRes + 0x08);
            reinterpret_cast<void(*)(void*)>(resVt[0])(cbRes);
            auto* heapVt = *reinterpret_cast<void***>(heap);
            reinterpret_cast<void(*)(void*, void*)>(heapVt[0x70 / 8])(heap, cbRes);
        }
        *reinterpret_cast<void**>(model + 0xf0) = nullptr;
    }

    sub_21c6ae0(self);
    sub_21c6410(self);
}

// ============================================================================
// 48. 0x21cef90 (157 B) - GXModelMeshResource_DeletingDestructor
// ============================================================================
void sub_21cef90(void* self) {
    sub_21cf030(self);
    sub_2bbe588(self);
}

// ============================================================================
// 49. 0x21cef70 (31 B) - GXModelMeshResource_Constructor
// ============================================================================
void sub_21cef70(void* self, void* alloc) {
    sub_21c60f0(self, alloc);
    *reinterpret_cast<void**>(self) = g_VtableModelMeshResource_53b6f60;
}

// ============================================================================
// 50. 0x21ceed0 (11 B) - GXModelFlverContainer_GetElement50At
// ============================================================================
void* sub_21ceed0(const void* flverContainer, uint32_t index) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    return *reinterpret_cast<uint8_t* const*>(cont + 0x50) + (static_cast<uint64_t>(index) << 2);
}

// ============================================================================
// 51. 0x21ceec0 (15 B) - GXModelFlverContainer_GetBoneMatrix3x4At
// ============================================================================
void* sub_21ceec0(const void* flverContainer, uint32_t index) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    return *reinterpret_cast<uint8_t* const*>(cont + 0x48) + static_cast<uint64_t>(index) * 48ULL;
}

// ============================================================================
// 52. 0x21cee70 (71 B) - GXModelFlverContainer_GetIndexBufferByteSize
// ============================================================================
uint32_t sub_21cee70(const void* flverContainer, uint32_t ibIndex) {
    auto* cont     = static_cast<const uint8_t*>(flverContainer);
    auto* hdr      = *reinterpret_cast<const uint8_t* const*>(cont + 0x00);
    auto* ibTable  = *reinterpret_cast<const uint8_t* const*>(cont + 0x28);
    uint32_t ver   = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
    uint32_t stride = (ver > 0x20008u) ? 0x20u : 0x10u;
    const uint8_t* ibHdr = ibTable + ibIndex * stride;

    if (ver >= 0x20009u) {
        return *reinterpret_cast<const uint32_t*>(ibHdr + 0x10);
    }

    uint32_t indexCount = *reinterpret_cast<const uint32_t*>(ibHdr + 0x08);
    int32_t bitsPerIndex = static_cast<int8_t>(hdr[0x48]);
    if (bitsPerIndex == 0) {
        bitsPerIndex = 16;
    }
    return (static_cast<uint32_t>(bitsPerIndex) * indexCount) >> 3;
}

// ============================================================================
// 53. 0x21cee40 (45 B) - GXModelFlverContainer_GetIndexBufferDataPtr
// ============================================================================
const void* sub_21cee40(const void* flverContainer, uint32_t ibIndex) {
    auto* cont       = static_cast<const uint8_t*>(flverContainer);
    auto* hdr        = *reinterpret_cast<const uint8_t* const*>(cont + 0x00);
    uint32_t ver     = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
    const uint8_t* dataBase = hdr + *reinterpret_cast<const uint32_t*>(hdr + 0x0c);
    uint32_t stride  = (ver > 0x20008u) ? 0x20u : 0x10u;
    auto* ibTable    = *reinterpret_cast<const uint8_t* const*>(cont + 0x28);
    uint32_t dataOff = *reinterpret_cast<const uint32_t*>(ibTable + ibIndex * stride + 0x0c);
    return dataBase + dataOff;
}

// ============================================================================
// 54. 0x21cee20 (27 B) - GXModelFlverContainer_GetVertexBufferDataPtr
// ============================================================================
const void* sub_21cee20(const void* flverContainer, uint32_t vbIndex) {
    auto* cont       = static_cast<const uint8_t*>(flverContainer);
    auto* hdr        = *reinterpret_cast<const uint8_t* const*>(cont + 0x00);
    auto* vbTable    = *reinterpret_cast<const uint8_t* const*>(cont + 0x30);
    const uint8_t* dataBase = hdr + *reinterpret_cast<const uint32_t*>(hdr + 0x0c);
    uint32_t dataOff = *reinterpret_cast<const uint32_t*>(vbTable + (static_cast<uint64_t>(vbIndex) << 5) + 0x1c);
    return dataBase + dataOff;
}

// ============================================================================
// 55. 0x21cee10 (11 B) - GXModelFlverContainer_GetVertexBufferHeaderAt
// ============================================================================
void* sub_21cee10(const void* flverContainer, uint32_t vbIndex) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    return *reinterpret_cast<uint8_t* const*>(cont + 0x30) + (static_cast<uint64_t>(vbIndex) << 5);
}

} // extern "C"
