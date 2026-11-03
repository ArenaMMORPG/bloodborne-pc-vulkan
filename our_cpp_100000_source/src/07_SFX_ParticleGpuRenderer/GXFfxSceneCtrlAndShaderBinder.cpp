// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 07: SFX_ParticleGpuRenderer (Tier 6/10)
// File: GXFfxSceneCtrlAndShaderBinder.cpp
// Range: 0x2309000 .. 0x2310000 (66 functions, 27,219 bytes)
// Order: Strict Right-to-Left (0x230ffe0 down to 0x23091c0)
// ============================================================================
// Architectural Subsystem Overview:
//   1. GXFfxpointSprite Shader Binder & Input-Layout Cache (0x230dc40 .. 0x230ffe0)
//      - sub_230ffe0: Initializes the 0x3b50-byte GXFfxShaderCache object,
//        including its red-black tree of named input layouts (+0xf8..+0x118)
//        and two shader table arrays (+0x120 for 139 entries, +0x1f88 for 127 entries)
//      - sub_230ffd0 / sub_230ffc0: Indexed entry accessors for table 0 (+0x120)
//        and table 1 (+0x1f88), each stride 0x38 bytes
//      - sub_230f420: Allocates and populates the GXFfxShaderCache from
//        GXFfxShader.shaderbnd, binding "GXFfxpointSprite" geometry shaders and
//        caching vertex input layouts by shader name
//      - sub_230ee40: Destroys the GXFfxShaderCache red-black tree and frees
//        all cached input-layout wrappers
//      - sub_230ecc0 .. sub_230dc40 (12 functions): Point-sprite and particle
//        shader pipeline state binders into GXDrawContext and GNM state shadow
//   2. GXFfxGraphicsResourceManager Hash Table (0x230c640 .. 0x230d830)
//      - Dinkumware std::unordered_map<uint32_t, void*> implementation using
//        Park-Miller LCG hash (0xdeadbeef ^ key, div 127773)
//      - sub_230d830 (_Insert), sub_230d6c0 (erase), sub_230d170 (bucket vector
//        insert), sub_230cfb0 (remove by ID), sub_230cd00 (insert unique ID),
//        sub_230cc30 (find by ID), sub_230caf0 / sub_230cae0 (clear),
//        sub_230c940 (constructor), sub_230c640 (destructor)
//   3. GXFfxSceneCtrl & Particle Transform / Instance Controller (0x230a250 .. 0x230c550)
//      - sub_230c170 / sub_230b760 / sub_230b2b0: SIMD quaternion-to-3x3 + scale
//        + 4x4 affine matrix batch transformers for 48-byte particle instances
//      - sub_230ba00: 3x4 affine matrix to Scale-Quaternion-Translation (SQT)
//        decomposer with Newton-Raphson rsqrt normalization
//      - sub_230b080 / sub_230af50 / sub_230af30 / sub_230ae70: Lifecycle and
//        DLReferenceCountObject management for particle instance buffers
//      - sub_230a5a0: Main GXFfxSceneCtrl draw packet builder and dispatcher
//   4. FXR3 Containers & Virtual Thunks (0x23091c0 .. 0x2309e80)
//      - sub_2309e80 (_Hash<uint64_t>::_Insert_bucket), sub_2309d80 / sub_2309830 /
//        sub_23096e0 / sub_2309560 / sub_23094c0 / sub_2309350 (FXR3 vectors &
//        hash tables with TLS setjmp transaction guards), sub_2309340 .. sub_23091c0
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <immintrin.h>

namespace {

struct LDivResult {
    int64_t quot;
    int64_t rem;
};

struct FfxIter24 {
    uint64_t proxy0;
    uint64_t proxy1;
    uint8_t* node;
};

struct FfxInsertResult32 {
    uint64_t proxy0;
    uint64_t proxy1;
    uint8_t* node;
    uint8_t  inserted;
    uint8_t  pad[7];
};

struct FfxTempWString {
    uint64_t reserved00;
    char16_t buf[8];
    uint64_t size;
    uint64_t capacity;
    void*    allocator;
    uint8_t  flag30;
    uint8_t  pad31[7];
};

} // namespace

extern "C" {

// External engine / Orbis OS functions called by 0x2309000 .. 0x2310000
void sub_e588a0(void* pDrawObj);
void sub_ee5de0(uint32_t code);
void sub_f24980(void* pObj, void* pAllocator);
bool sub_f24bc0(void* pObj, const char16_t* pName, void* pResArg, uint32_t flags);
void* sub_207b720(void* pMemory);
void sub_207d250();
void sub_20b55b0(const char* file, uint32_t line, const char* msg, ...);
int32_t sub_21a2e60(
    void** ppOutLayoutWrapper,
    void* pGfxDevice,
    const void* pLayoutElements,
    uint32_t elementCount,
    void* pVertexShader,
    void* pAllocator,
    void* pOptionalArg);
void sub_224c940(void* pOutDrawObj, void* pSceneSub720, void* pSceneSub440, const void* pParams);
void sub_224d020(void* pDrawObj);
void sub_2250f70(void* pSceneCtrl, void* pDrawObj, void* pSubDesc, void* pParamCopy);
void sub_23101e0(void* pTree, void* pRootNode);
void sub_2310260(void* pOutIter, void* pTree, const void* pKeyWStr);
void** sub_23103c0(void* pTree, const void* pKeyWStr);
void sub_2311020(void* pSub80);
bool sub_2311180(void* pSub80, void* pResData, uint32_t flags);
void sub_2311430(void* pSub80, uint32_t index, const void* pSqtData, uint32_t flags);
void sub_2311c00(void* pSub80, uint32_t mask);
void sub_262a720(void* pWStr, const char16_t* pSrc, uint64_t len);
uint8_t* sub_2ba2b10();
void sub_2bbe588(void* pMemory);
int32_t sub_2bbe8f8(void* pJmpBuf);
[[noreturn]] void sub_2bbe978(void* pJmpBuf, uint32_t val);
[[noreturn]] void sub_2bbf1a8(const char* msg);
[[noreturn]] void sub_2bbf1b8(const char* msg);
uint64_t sub_2bbf228(const char16_t* pStr);
LDivResult sub_2bbfd68(int64_t numer, int64_t denom);

// Forward declarations of all 66 functions in 0x2309000 .. 0x2310000
void sub_230ffe0(void* pSelf, void* pAllocator);
void* sub_230ffd0(void* pSelf, uint32_t index);
void* sub_230ffc0(void* pSelf, int32_t index);
void* sub_230f420(void* pGfxDevice, void* pAllocator, void* pShaderBnd);
void sub_230ee40(void* pSelf);
void sub_230ecc0(const void* pShaderCache, void* pDrawContext);
void sub_230eb40(const void* pShaderCache, void* pDrawContext);
void sub_230e9c0(const void* pShaderCache, void* pDrawContext);
void sub_230e840(const void* pShaderCache, void* pDrawContext);
void sub_230e6c0(const void* pShaderCache, void* pDrawContext);
void sub_230e540(const void* pShaderCache, void* pDrawContext);
void sub_230e3c0(const void* pShaderCache, void* pDrawContext);
void sub_230e240(const void* pShaderCache, void* pDrawContext);
void sub_230e0c0(const void* pShaderCache, void* pDrawContext);
void sub_230df40(const void* pShaderCache, void* pDrawContext);
void sub_230ddc0(const void* pShaderCache, void* pDrawContext);
void sub_230dc40(const void* pShaderCache, void* pDrawContext);
FfxInsertResult32* sub_230d830(
    FfxInsertResult32* pOutPair,
    void* pHash,
    const uint32_t* pKey,
    FfxIter24 plistIter);
FfxIter24* sub_230d6c0(FfxIter24* pOutIter, void* pHash, uint8_t* pEraseNode);
FfxIter24* sub_230d170(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter);
void sub_230cfb0(void* pMgr, uint32_t key);
bool sub_230cd00(void* pMgr, uint32_t key, void* pValue);
void* sub_230cc30(void* pMgr, uint32_t key);
void sub_230caf0(void* pMgr);
void sub_230cae0(void* pMgr);
void sub_230c940(void* pMgr, void* pAllocator);
void sub_230c640(void* pMgr);
void sub_230c550(void* pSelf, void* pArg1, void* pArg2);
void sub_230c170(void* pSelf, void* pDrawCtx, uint32_t slotIndex);
void sub_230c160();
void sub_230ba00(void* pSelf, uint32_t index, const float* pMatrix3x4);
void sub_230b760(void* pSelf);
bool sub_230b2b0(void* pSelf, const void* pSrcObj);
void sub_230b080(void* pSelf);
void sub_230af50(void* pSelf);
void sub_230af30(void* pSelf);
void sub_230ae70(void* pSelf, void* pAllocator);
void sub_230ae10(void* pOuter, void* pArg1, void* pArg2);
void sub_230add0(void* pOuter, const float* pMatrix4x4);
void sub_230a5a0(void* pOuter, void* pSceneCtrl, const void* pDrawDesc);
uint64_t sub_230a590(void* pOuter, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
void sub_230a4b0(void* pOuter, uint32_t index, const float* pMatrix3x4);
void sub_230a440(void* pOuter);
bool sub_230a360(void* pOuter, void* pSceneCtrl, void* pResArg);
void sub_230a2f0(void* pOuter);
void sub_230a280(void* pOuter);
void sub_230a250(void* pOuter, void* pAllocator);
FfxInsertResult32* sub_2309e80(
    FfxInsertResult32* pOutPair,
    void* pHash,
    uint8_t* pWhereNode,
    uint64_t bucket,
    FfxIter24 plistIter);
void sub_2309d80(void* pVec, uint64_t newCap);
FfxIter24* sub_2309830(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter);
void sub_23096e0(void* pVec);
bool sub_2309560(void* pVec, uint64_t newCap);
bool sub_23094c0(void* pVec, uint64_t newCap);
void sub_2309350(void* pHash, void** ppAllocator);
void* sub_2309340(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
void* sub_2309330(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
void* sub_2309320(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
void* sub_2309310(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
void* sub_2309300(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
void* sub_23092f0(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
void* sub_23092e0(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5);
bool sub_23092d0();
uint32_t sub_23091f0();
bool sub_23091e0();
bool sub_23091d0();
void sub_23091c0();

} // extern "C"

namespace {

static inline void handleContainerAllocFailure() {
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    if (*reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) != 0) {
        sub_2bbe978(reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL))), 1);
    }
    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
}

static inline void validateContainerHeap(void* pHeap) {
    uint8_t flagsBuf[8] = {};
    void* vtbl = *reinterpret_cast<void**>(pHeap);
    (*reinterpret_cast<void (**)(void*, void*, uint32_t)>(static_cast<uint8_t*>(vtbl) + 0x20))(flagsBuf, pHeap, 0);
    if ((flagsBuf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
}

static inline void* getDefaultContainerHeapChecked() {
    void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    if (pHeap == nullptr) {
        pHeap = sub_2ba2b10();
        *reinterpret_cast<void**>(GX_VMA(0x5540670ULL)) = pHeap;
    }
    validateContainerHeap(pHeap);
    return pHeap;
}

static inline void initTempWString(FfxTempWString* pStr, const char16_t* pChars) {
    void* pHeap = getDefaultContainerHeapChecked();
    pStr->reserved00 = 0;
    pStr->allocator = pHeap;
    pStr->capacity = 7;
    pStr->size = 0;
    pStr->buf[0] = 0;
    uint64_t len = (pChars[0] != 0) ? sub_2bbf228(pChars) : 0;
    sub_262a720(pStr, pChars, len);
    pStr->flag30 = 1;
}

static inline void destroyTempWString(FfxTempWString* pStr) {
    if (pStr->capacity >= 8) {
        void* pHeap = pStr->allocator;
        void* pBuf = *reinterpret_cast<void**>(&pStr->buf[0]);
        void* vtbl = *reinterpret_cast<void**>(pHeap);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pHeap, pBuf);
    }
    pStr->capacity = 7;
    pStr->size = 0;
    pStr->buf[0] = 0;
}

static inline void* extractShaderHwObj(void* pShaderWrapper) {
    if (pShaderWrapper == nullptr) {
        return nullptr;
    }
    void* vtbl = *reinterpret_cast<void**>(pShaderWrapper);
    return (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl) + 0x18))(pShaderWrapper);
}

static inline void bindParticleShaderStateCommon(
    const uint8_t* pShaderCache,
    uint8_t* pDrawContext,
    bool hasGeometryShader,
    size_t psOffset,
    size_t blendStateOffset,
    size_t rasterizerStateOffset)
{
    void* pVs = *reinterpret_cast<void* const*>(pShaderCache + 0x08);
    *reinterpret_cast<void**>(pDrawContext + 0x780) = pVs;
    uint8_t* pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
    *reinterpret_cast<void**>(pShadow + 0xc1b8) = pVs;

    if (hasGeometryShader) {
        void* pGs = *reinterpret_cast<void* const*>(pShaderCache + 0x10);
        *reinterpret_cast<void**>(pDrawContext + 0xd50) = pGs;
        pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
        *reinterpret_cast<void**>(pShadow + 0xd0a0) = pGs;
    } else {
        *reinterpret_cast<void**>(pDrawContext + 0xd50) = nullptr;
        pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
        *reinterpret_cast<void**>(pShadow + 0xd0a0) = nullptr;
    }

    void* pPs = *reinterpret_cast<void* const*>(pShaderCache + psOffset);
    *reinterpret_cast<void**>(pDrawContext + 0xa68) = pPs;
    pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
    *reinterpret_cast<void**>(pShadow + 0xd598) = pPs;

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pStateRepo = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    uint64_t samp0 = *reinterpret_cast<uint64_t*>(pStateRepo + 0x2d0);
    uint64_t samp1 = *reinterpret_cast<uint64_t*>(pStateRepo + 0x2c0);
    uint64_t samp2 = *reinterpret_cast<uint64_t*>(pStateRepo + 0x2d8);

    *reinterpret_cast<uint64_t*>(pDrawContext + 0xa18) = samp0;
    *reinterpret_cast<uint64_t*>(pDrawContext + 0xa20) = samp1;
    *reinterpret_cast<uint64_t*>(pDrawContext + 0xa28) = samp2;

    uint32_t minSlot = *reinterpret_cast<uint32_t*>(pDrawContext + 0xa60);
    *reinterpret_cast<uint32_t*>(pDrawContext + 0xa60) = (minSlot < 7u) ? minSlot : 7u;

    uint32_t maxSlot = *reinterpret_cast<uint32_t*>(pDrawContext + 0xa64);
    *reinterpret_cast<uint32_t*>(pDrawContext + 0xa64) = (maxSlot > 10u) ? maxSlot : 10u;

    pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
    *reinterpret_cast<uint64_t*>(pShadow + 0x1978) |= 0x380ULL;
    *reinterpret_cast<uint64_t*>(pShadow + 0xd0e0) = samp0;
    *reinterpret_cast<uint64_t*>(pShadow + 0xd0e8) = samp1;
    *reinterpret_cast<uint64_t*>(pShadow + 0xd0f0) = samp2;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pStateRepo = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pBlendState = *reinterpret_cast<void**>(pStateRepo + blendStateOffset);
    *reinterpret_cast<void**>(pDrawContext + 0x480) = pBlendState;
    __m128 zeroVec = _mm_setzero_ps();
    _mm_storeu_ps(reinterpret_cast<float*>(pDrawContext + 0x488), zeroVec);
    *reinterpret_cast<uint32_t*>(pDrawContext + 0x498) = 0xffffffffu;

    pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
    *reinterpret_cast<void**>(pShadow + 0xbc00) = pBlendState;
    _mm_storeu_ps(reinterpret_cast<float*>(pShadow + 0xbc98), zeroVec);
    *reinterpret_cast<uint32_t*>(pShadow + 0xbcb0) = 0xffffffffu;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pStateRepo = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pRasterizerState = *reinterpret_cast<void**>(pStateRepo + rasterizerStateOffset);
    *reinterpret_cast<void**>(pDrawContext + 0x258) = pRasterizerState;
    pShadow = *reinterpret_cast<uint8_t**>(pDrawContext + 0x08);
    *reinterpret_cast<void**>(pShadow + 0xb970) = pRasterizerState;

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pStateRepo = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    void* pDepthStencilState = *reinterpret_cast<void**>(pStateRepo + 0x1d8);
    *reinterpret_cast<void**>(pDrawContext + 0x210) = pDepthStencilState;
    pDrawContext[0x218] = 0;
    *reinterpret_cast<void**>(pShadow + 0xbc08) = pDepthStencilState;
    pShadow[0xbcb4] = 0;
}

} // namespace

extern "C" {

// ============================================================================
// [1/66] sub_230ffe0 @ 0x230ffe0 (303 B)
// Initializes GXFfxShaderCache (+0x00..+0x3b50)
// ============================================================================
void sub_230ffe0(void* pSelf, void* pAllocator) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    std::memset(self, 0, 0xf8);
    validateContainerHeap(pAllocator);

    *reinterpret_cast<uint64_t*>(self + 0x108) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x100) = 0;
    *reinterpret_cast<void**>(self + 0x110) = pAllocator;

    void* vtbl = *reinterpret_cast<void**>(pAllocator);
    uint8_t* pHead = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(vtbl) + 0x58))(
            pAllocator, 0x60, 8));
    if (pHead != nullptr) {
        *reinterpret_cast<void**>(pHead + 0x00) = pHead;
    } else {
        handleContainerAllocFailure();
        return;
    }

    *reinterpret_cast<void**>(pHead + 0x08) = pHead;
    *reinterpret_cast<void**>(pHead + 0x10) = pHead;
    pHead[0x18] = 1;
    pHead[0x19] = 1;

    *reinterpret_cast<void**>(self + 0x100) = pHead;
    *reinterpret_cast<void**>(self + 0x118) = pAllocator;
    std::memset(self + 0x120, 0, 0x3a30);
}

// ============================================================================
// [2/66] sub_230ffd0 @ 0x230ffd0 (15 B)
// Returns pointer to shader table 0 entry (+0x120 + index * 0x38)
// ============================================================================
void* sub_230ffd0(void* pSelf, uint32_t index) {
    return static_cast<uint8_t*>(pSelf) + static_cast<uint64_t>(index) * 0x38ULL + 0x120ULL;
}

// ============================================================================
// [3/66] sub_230ffc0 @ 0x230ffc0 (16 B)
// Returns pointer to shader table 1 entry (+0x1f88 + index * 0x38)
// ============================================================================
void* sub_230ffc0(void* pSelf, int32_t index) {
    return static_cast<uint8_t*>(pSelf) + static_cast<int64_t>(index) * 0x38 + 0x1f88;
}

// ============================================================================
// [4/66] sub_230f420 @ 0x230f420 (2969 B)
// Creates and populates GXFfxShaderCache from GXFfxShader.shaderbnd
// ============================================================================
void* sub_230f420(void* pGfxDevice, void* pAllocator, void* pShaderBnd) {
    if (pAllocator == nullptr) {
        return nullptr;
    }

    void* allocVtbl = *reinterpret_cast<void**>(pAllocator);
    uint8_t* pCache = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAllocator, 0x3b50, 8));
    if (pCache == nullptr) {
        return nullptr;
    }

    sub_230ffe0(pCache, pAllocator);

    const char16_t* gsNames1[139];
    std::memset(gsNames1, 0, sizeof(gsNames1));
    const char16_t* kPointSpriteGsName = reinterpret_cast<const char16_t*>(GX_VMA(0x49d48feULL));
    gsNames1[11] = kPointSpriteGsName;
    gsNames1[75] = kPointSpriteGsName;
    gsNames1[76] = kPointSpriteGsName;

    if (pShaderBnd == nullptr) {
        sub_230ee40(pCache);
        void* vtbl = *reinterpret_cast<void**>(pAllocator);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pAllocator, pCache);
        return nullptr;
    }

    uint8_t* pTree = pCache + 0xf8;
    const char16_t* const* pVsTable1 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x53625d0ULL));
    const void* const* pLayoutTable1 = reinterpret_cast<const void* const*>(GX_VMA(0x53632f0ULL));
    const uint32_t* pCountTable1 = reinterpret_cast<const uint32_t*>(GX_VMA(0x47395a0ULL));
    const char16_t* const* pPsHsTable1 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x5362a30ULL));
    const char16_t* const* pDsTable1 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x5362e90ULL));

    for (size_t i = 0; i < 139; ++i) {
        void* pLayoutWrapper = nullptr;
        const char16_t* pVsName = pVsTable1[i];

        void* bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
        void* pVsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
            static_cast<uint8_t*>(bndVtbl) + 0x40))(pShaderBnd, 0, pVsName);

        void* pVsHwObj = nullptr;
        if (pVsShader != nullptr) {
            FfxTempWString keyStr;
            initTempWString(&keyStr, pVsName);
            FfxIter24 foundIter = {};
            sub_2310260(&foundIter, pTree, &keyStr);
            destroyTempWString(&keyStr);

            if (foundIter.node == *reinterpret_cast<uint8_t**>(pCache + 0x100)) {
                int32_t err = sub_21a2e60(
                    &pLayoutWrapper,
                    pGfxDevice,
                    pLayoutTable1[i],
                    pCountTable1[i],
                    pVsShader,
                    pAllocator,
                    nullptr);
                if (err != 0) {
                    sub_230ee40(pCache);
                    void* vtbl = *reinterpret_cast<void**>(pAllocator);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(
                        pAllocator, pCache);
                    if (pLayoutWrapper != nullptr) {
                        vtbl = *reinterpret_cast<void**>(pAllocator);
                        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(
                            pAllocator, pLayoutWrapper);
                    }
                    return nullptr;
                }
                FfxTempWString insStr;
                initTempWString(&insStr, pVsName);
                void** ppSlot = sub_23103c0(pTree, &insStr);
                *ppSlot = pLayoutWrapper;
                destroyTempWString(&insStr);
            } else {
                pLayoutWrapper = *reinterpret_cast<void**>(foundIter.node + 0x58);
            }
            pVsHwObj = extractShaderHwObj(pVsShader);
        }

        const char16_t* pPsHsName = pPsHsTable1[i];
        void* pHsHwObj = nullptr;
        void* pPsHwObj = nullptr;
        if (pPsHsName != nullptr) {
            bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pHsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x28))(pShaderBnd, 0, pPsHsName);
            pHsHwObj = extractShaderHwObj(pHsShader);

            bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pPsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x20))(pShaderBnd, 0, pPsHsName);
            pPsHwObj = extractShaderHwObj(pPsShader);
        }

        const char16_t* pGsName = gsNames1[i];
        void* pGsHwObj = nullptr;
        if (pGsName != nullptr) {
            bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pGsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x30))(pShaderBnd, 0, pGsName);
            pGsHwObj = extractShaderHwObj(pGsShader);
        }

        const char16_t* pDsName = pDsTable1[i];
        bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
        void* pDsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
            static_cast<uint8_t*>(bndVtbl) + 0x38))(pShaderBnd, 0, pDsName);
        void* pDsHwObj = extractShaderHwObj(pDsShader);

        void** pEntry = reinterpret_cast<void**>(pCache + 0x120 + i * 0x38);
        pEntry[0] = pLayoutWrapper;
        pEntry[1] = pVsHwObj;
        pEntry[2] = pHsHwObj;
        pEntry[3] = pPsHwObj;
        pEntry[4] = pGsHwObj;
        pEntry[5] = pDsHwObj;
    }

    const char16_t* const* pVsTable2 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x5363750ULL));
    const void* const* pLayoutTable2 = reinterpret_cast<const void* const*>(GX_VMA(0x5364750ULL));
    const uint32_t* pCountTable2 = reinterpret_cast<const uint32_t*>(GX_VMA(0x47397d0ULL));
    const char16_t* const* pGsTable2 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x5363b50ULL));
    const char16_t* const* pCsTable2 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x5364350ULL));
    const char16_t* const* pDsTable2 = reinterpret_cast<const char16_t* const*>(GX_VMA(0x5363f50ULL));

    for (uint32_t j = 0; j < 0x7fu; ++j) {
        void* pLayoutWrapper = nullptr;
        void* pVsHwObj = nullptr;
        const char16_t* pVsName = pVsTable2[j];

        if (pVsName != nullptr) {
            void* bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pVsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x40))(pShaderBnd, 0, pVsName);
            if (pVsShader != nullptr) {
                FfxTempWString keyStr;
                initTempWString(&keyStr, pVsName);
                FfxIter24 foundIter = {};
                sub_2310260(&foundIter, pTree, &keyStr);
                destroyTempWString(&keyStr);

                if (foundIter.node == *reinterpret_cast<uint8_t**>(pCache + 0x100)) {
                    int32_t err = sub_21a2e60(
                        &pLayoutWrapper,
                        pGfxDevice,
                        pLayoutTable2[j],
                        pCountTable2[j],
                        pVsShader,
                        pAllocator,
                        nullptr);
                    if (err != 0) {
                        sub_230ee40(pCache);
                        void* vtbl = *reinterpret_cast<void**>(pAllocator);
                        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(
                            pAllocator, pCache);
                        if (pLayoutWrapper != nullptr) {
                            vtbl = *reinterpret_cast<void**>(pAllocator);
                            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(
                                pAllocator, pLayoutWrapper);
                        }
                        return nullptr;
                    }
                    FfxTempWString insStr;
                    initTempWString(&insStr, pVsName);
                    void** ppSlot = sub_23103c0(pTree, &insStr);
                    *ppSlot = pLayoutWrapper;
                    destroyTempWString(&insStr);
                } else {
                    pLayoutWrapper = *reinterpret_cast<void**>(foundIter.node + 0x58);
                }
                pVsHwObj = extractShaderHwObj(pVsShader);
            }
        }

        void* pHsHwObj = nullptr;
        void* pPsHwObj = nullptr;

        void* pGsHwObj = nullptr;
        if (pGsTable2[j] != nullptr) {
            void* bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pGsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x30))(pShaderBnd, 0, pGsTable2[j]);
            pGsHwObj = extractShaderHwObj(pGsShader);
        }

        void* pCsHwObj = nullptr;
        if (pCsTable2[j] != nullptr) {
            void* bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pCsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x18))(pShaderBnd, 0, pCsTable2[j]);
            pCsHwObj = extractShaderHwObj(pCsShader);
        }

        void* pDsHwObj = nullptr;
        if (pDsTable2[j] != nullptr) {
            void* bndVtbl = *reinterpret_cast<void**>(pShaderBnd);
            void* pDsShader = (*reinterpret_cast<void* (**)(void*, uint32_t, const char16_t*)>(
                static_cast<uint8_t*>(bndVtbl) + 0x38))(pShaderBnd, 0, pDsTable2[j]);
            pDsHwObj = extractShaderHwObj(pDsShader);
        }

        void** pEntry = reinterpret_cast<void**>(pCache + 0x1f88 + static_cast<size_t>(j) * 0x38);
        pEntry[0] = pLayoutWrapper;
        pEntry[1] = pVsHwObj;
        pEntry[2] = pHsHwObj;
        pEntry[3] = pPsHwObj;
        pEntry[4] = pGsHwObj;
        pEntry[5] = pDsHwObj;
        pEntry[6] = pCsHwObj;
    }

    // Populate quick-access header slots (+0x00 .. +0xf0) from table entries
    *reinterpret_cast<uint64_t*>(pCache + 0x00) = *reinterpret_cast<uint64_t*>(pCache + 0x2d18);
    *reinterpret_cast<uint64_t*>(pCache + 0x08) = *reinterpret_cast<uint64_t*>(pCache + 0x2d20);
    *reinterpret_cast<uint64_t*>(pCache + 0x10) = *reinterpret_cast<uint64_t*>(pCache + 0x2e50);
    *reinterpret_cast<uint64_t*>(pCache + 0x18) = *reinterpret_cast<uint64_t*>(pCache + 0x2e88);
    *reinterpret_cast<uint64_t*>(pCache + 0x20) = *reinterpret_cast<uint64_t*>(pCache + 0x2ec8);
    *reinterpret_cast<uint64_t*>(pCache + 0x28) = *reinterpret_cast<uint64_t*>(pCache + 0x32c0);
    *reinterpret_cast<uint64_t*>(pCache + 0x30) = *reinterpret_cast<uint64_t*>(pCache + 0x32f0);
    *reinterpret_cast<uint64_t*>(pCache + 0x38) = *reinterpret_cast<uint64_t*>(pCache + 0x3328);
    *reinterpret_cast<uint64_t*>(pCache + 0x40) = *reinterpret_cast<uint64_t*>(pCache + 0x2f00);
    *reinterpret_cast<uint64_t*>(pCache + 0x48) = *reinterpret_cast<uint64_t*>(pCache + 0x2f38);
    *reinterpret_cast<uint64_t*>(pCache + 0x50) = *reinterpret_cast<uint64_t*>(pCache + 0x3280);
    *reinterpret_cast<uint64_t*>(pCache + 0x58) = *reinterpret_cast<uint64_t*>(pCache + 0x2f70);
    *reinterpret_cast<uint64_t*>(pCache + 0x60) = *reinterpret_cast<uint64_t*>(pCache + 0x30c0);
    uint64_t val3018 = *reinterpret_cast<uint64_t*>(pCache + 0x3018);
    *reinterpret_cast<uint64_t*>(pCache + 0x68) = val3018;
    *reinterpret_cast<uint64_t*>(pCache + 0x70) = *reinterpret_cast<uint64_t*>(pCache + 0x3050);
    *reinterpret_cast<uint64_t*>(pCache + 0x78) = *reinterpret_cast<uint64_t*>(pCache + 0x2fe0);
    *reinterpret_cast<uint64_t*>(pCache + 0x80) = *reinterpret_cast<uint64_t*>(pCache + 0x2fa8);
    *reinterpret_cast<uint64_t*>(pCache + 0x88) = *reinterpret_cast<uint64_t*>(pCache + 0x30f8);
    *reinterpret_cast<uint64_t*>(pCache + 0x90) = *reinterpret_cast<uint64_t*>(pCache + 0x3130);
    *reinterpret_cast<uint64_t*>(pCache + 0x98) = val3018;
    *reinterpret_cast<uint64_t*>(pCache + 0xf0) = *reinterpret_cast<uint64_t*>(pCache + 0x2d88);
    *reinterpret_cast<uint64_t*>(pCache + 0xa0) = *reinterpret_cast<uint64_t*>(pCache + 0x2d90);
    *reinterpret_cast<uint64_t*>(pCache + 0xa8) = *reinterpret_cast<uint64_t*>(pCache + 0x2dc8);
    *reinterpret_cast<uint64_t*>(pCache + 0xb0) = *reinterpret_cast<uint64_t*>(pCache + 0x2d58);
    *reinterpret_cast<uint64_t*>(pCache + 0xb8) = *reinterpret_cast<uint64_t*>(pCache + 0x2e00);
    *reinterpret_cast<uint64_t*>(pCache + 0xc0) = *reinterpret_cast<uint64_t*>(pCache + 0x31d8);
    *reinterpret_cast<uint64_t*>(pCache + 0xc8) = *reinterpret_cast<uint64_t*>(pCache + 0x3248);
    *reinterpret_cast<uint64_t*>(pCache + 0xd0) = *reinterpret_cast<uint64_t*>(pCache + 0x3210);
    *reinterpret_cast<uint64_t*>(pCache + 0xd8) = *reinterpret_cast<uint64_t*>(pCache + 0x31a0);
    *reinterpret_cast<uint64_t*>(pCache + 0xe0) = *reinterpret_cast<uint64_t*>(pCache + 0x3088);
    *reinterpret_cast<uint64_t*>(pCache + 0xe8) = *reinterpret_cast<uint64_t*>(pCache + 0x3168);

    return pCache;
}

// ============================================================================
// [5/66] sub_230ee40 @ 0x230ee40 (1503 B)
// Destroys GXFfxShaderCache red-black tree and cached input layouts
// ============================================================================
void sub_230ee40(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint8_t* pHead = *reinterpret_cast<uint8_t**>(self + 0x100);
    if (!pHead) {
        return;
    }
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pHead + 0x00);

    while (pCur != pHead) {
        uint8_t* pWrapper = *reinterpret_cast<uint8_t**>(pCur + 0x58);
        if (pWrapper != nullptr) {
            void* pWrapAlloc = sub_207b720(pWrapper);
            void* pBuf18 = *reinterpret_cast<void**>(pWrapper + 0x18);
            if (pBuf18 != nullptr) {
                void* pAlloc0 = *reinterpret_cast<void**>(pWrapper + 0x00);
                void* vtbl0 = *reinterpret_cast<void**>(pAlloc0);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl0) + 0x70))(pAlloc0, pBuf18);
                *reinterpret_cast<void**>(pWrapper + 0x18) = nullptr;
            }
            void* pBuf10 = *reinterpret_cast<void**>(pWrapper + 0x10);
            if (pBuf10 != nullptr) {
                void* pAlloc1 = *reinterpret_cast<void**>(pWrapper + 0x08);
                void* vtbl1 = *reinterpret_cast<void**>(pAlloc1);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl1) + 0x70))(pAlloc1, pBuf10);
                *reinterpret_cast<void**>(pWrapper + 0x10) = nullptr;
            }
            void* wrapVtbl = *reinterpret_cast<void**>(pWrapAlloc);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(wrapVtbl) + 0x70))(pWrapAlloc, pWrapper);
            *reinterpret_cast<void**>(pCur + 0x58) = nullptr;
        }

        // Advance iterator to in-order successor without per-node RB-tree rebalancing
        uint8_t* pRight = *reinterpret_cast<uint8_t**>(pCur + 0x10);
        uint8_t* pNext = nullptr;
        if (pRight[0x19] == 0) {
            uint8_t* pScan = pRight;
            do {
                pNext = pScan;
                pScan = *reinterpret_cast<uint8_t**>(pNext + 0x00);
            } while (pScan[0x19] == 0);
        } else {
            uint8_t* pScan = pCur;
            for (;;) {
                pNext = *reinterpret_cast<uint8_t**>(pScan + 0x08);
                if (pNext[0x19] != 0 || pScan != *reinterpret_cast<uint8_t**>(pNext + 0x10)) {
                    break;
                }
                pScan = pNext;
            }
        }
        pCur = pNext;
    }

    sub_23101e0(self + 0xf8, *reinterpret_cast<void**>(pHead + 0x08));
    pHead = *reinterpret_cast<uint8_t**>(self + 0x100);
    *reinterpret_cast<void**>(pHead + 0x08) = pHead;
    *reinterpret_cast<void**>(pHead + 0x00) = pHead;
    *reinterpret_cast<void**>(pHead + 0x10) = pHead;
    *reinterpret_cast<uint64_t*>(self + 0x108) = 0;

    void* pHeadToFree = *reinterpret_cast<void**>(self + 0x100);
    void* pTreeAlloc = *reinterpret_cast<void**>(self + 0x110);
    void* treeAllocVtbl = *reinterpret_cast<void**>(pTreeAlloc);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(treeAllocVtbl) + 0x70))(
        pTreeAlloc, pHeadToFree);
}

// ============================================================================
// [6/66] .. [17/66] Point-Sprite & Particle Shader Pipeline Binders
// (0x230dc40 .. 0x230ecc0)
// ============================================================================
void sub_230ecc0(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x78,
        0x00,
        0x218);
}

void sub_230eb40(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        false,
        0x98,
        0x00,
        0x1f8);
}

void sub_230e9c0(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x90,
        0x00,
        0x1f8);
}

void sub_230e840(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x88,
        0x00,
        0x1f8);
}

void sub_230e6c0(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x80,
        0x00,
        0x1f8);
}

void sub_230e540(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x60,
        0x08,
        0x1f8);
}

void sub_230e3c0(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x58,
        0x18,
        0x1f8);
}

void sub_230e240(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x50,
        0x00,
        0x1f8);
}

void sub_230e0c0(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x48,
        0x00,
        0x1f8);
}

void sub_230df40(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x30,
        0x00,
        0x1f8);
}

void sub_230ddc0(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x40,
        0x00,
        0x1f8);
}

void sub_230dc40(const void* pShaderCache, void* pDrawContext) {
    bindParticleShaderStateCommon(
        static_cast<const uint8_t*>(pShaderCache),
        static_cast<uint8_t*>(pDrawContext),
        true,
        0x20,
        0x00,
        0x1f8);
}

// ============================================================================
// [18/66] sub_230d830 @ 0x230d830 (1040 B)
// _Hash<uint32_t, void*>::_Insert(pOutPair, pHash, pKey, plistIter)
// ============================================================================
FfxInsertResult32* sub_230d830(
    FfxInsertResult32* pOutPair,
    void* pHash,
    const uint32_t* pKey,
    FfxIter24 plistIter)
{
    uint8_t* hashBytes = static_cast<uint8_t*>(pHash);
    uint64_t keyVal = *pKey;
    LDivResult dv = sub_2bbfd68(static_cast<int64_t>(keyVal ^ 0xdeadbeefULL), 0x1f31d);
    int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
    uint64_t hashVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

    uint64_t mask = *reinterpret_cast<uint64_t*>(hashBytes + 0x48);
    uint64_t bucket = hashVal & mask;
    if (*reinterpret_cast<uint64_t*>(hashBytes + 0x50) <= bucket) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(hashBytes + 0x08);
    FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(hashBytes + 0x28);
    uint8_t* pBucketFirst = pVecBegin[bucket * 2].node;
    uint8_t* pCur = pHead;

    if (pBucketFirst != pHead) {
        pCur = *reinterpret_cast<uint8_t**>(pVecBegin[bucket * 2 + 1].node + 0x00);
        while (pCur != pBucketFirst) {
            pCur = *reinterpret_cast<uint8_t**>(pCur + 0x08);
            uint32_t k = *pKey;
            uint32_t curK = *reinterpret_cast<uint32_t*>(pCur + 0x10);
            if (k < curK) {
                continue;
            }
            if (curK >= k) {
                uint8_t* pPlist = plistIter.node;
                if (pPlist != pHead) {
                    uint8_t* pNext = *reinterpret_cast<uint8_t**>(pPlist + 0x00);
                    uint8_t* pPrev = *reinterpret_cast<uint8_t**>(pPlist + 0x08);
                    *reinterpret_cast<uint8_t**>(pPrev + 0x00) = pNext;
                    *reinterpret_cast<uint8_t**>(pNext + 0x08) = pPrev;

                    void* pAlloc = *reinterpret_cast<void**>(hashBytes + 0x18);
                    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(
                        pAlloc, pPlist);
                    --(*reinterpret_cast<uint64_t*>(hashBytes + 0x10));
                }
                pOutPair->proxy0 = 0;
                pOutPair->proxy1 = 0;
                pOutPair->node = pCur;
                pOutPair->inserted = 0;
                return pOutPair;
            }
            pBucketFirst = *reinterpret_cast<uint8_t**>(pCur + 0x00);
            break;
        }
    }

    uint8_t* pWhere = pBucketFirst;
    uint8_t* pPlist = plistIter.node;
    uint8_t* pPlistNext = *reinterpret_cast<uint8_t**>(pPlist + 0x00);

    if (pWhere != pPlistNext) {
        uint8_t* pPlistPrev = *reinterpret_cast<uint8_t**>(pPlist + 0x08);
        *reinterpret_cast<uint8_t**>(pPlistPrev + 0x00) = pPlistNext;
        uint8_t* pNextPrev = *reinterpret_cast<uint8_t**>(pPlistNext + 0x08);
        *reinterpret_cast<uint8_t**>(pNextPrev + 0x00) = pWhere;
        uint8_t* pWherePrev = *reinterpret_cast<uint8_t**>(pWhere + 0x08);
        *reinterpret_cast<uint8_t**>(pWherePrev + 0x00) = pPlist;

        uint8_t* pTmp = *reinterpret_cast<uint8_t**>(pWhere + 0x08);
        *reinterpret_cast<uint8_t**>(pWhere + 0x08) = *reinterpret_cast<uint8_t**>(pPlistNext + 0x08);
        *reinterpret_cast<uint8_t**>(pPlistNext + 0x08) = *reinterpret_cast<uint8_t**>(pPlist + 0x08);
        *reinterpret_cast<uint8_t**>(pPlist + 0x08) = pTmp;

        pHead = *reinterpret_cast<uint8_t**>(hashBytes + 0x08);
        pVecBegin = *reinterpret_cast<FfxIter24**>(hashBytes + 0x28);
        pBucketFirst = pVecBegin[bucket * 2].node;
    }

    FfxIter24* pSlot0 = &pVecBegin[bucket * 2];
    if (pBucketFirst == pHead) {
        pSlot0->proxy0 = plistIter.proxy0;
        pSlot0->proxy1 = plistIter.proxy1;
        pSlot0->node = pPlist;
        pVecBegin = *reinterpret_cast<FfxIter24**>(hashBytes + 0x28);
        FfxIter24* pSlot1 = &pVecBegin[bucket * 2 + 1];
        pSlot1->proxy0 = plistIter.proxy0;
        pSlot1->proxy1 = plistIter.proxy1;
        pSlot1->node = pPlist;
    } else if (pBucketFirst == pWhere) {
        pSlot0->proxy0 = plistIter.proxy0;
        pSlot0->proxy1 = plistIter.proxy1;
        pSlot0->node = pPlist;
    } else {
        FfxIter24* pSlot1 = &pVecBegin[bucket * 2 + 1];
        uint8_t* pEndNext = *reinterpret_cast<uint8_t**>(pSlot1->node + 0x00);
        pSlot1->node = pEndNext;
        if (pEndNext != pPlist) {
            pSlot1->node = *reinterpret_cast<uint8_t**>(pEndNext + 0x08);
        }
    }

    uint64_t curSize = *reinterpret_cast<uint64_t*>(hashBytes + 0x10);
    uint64_t maxIdx = *reinterpret_cast<uint64_t*>(hashBytes + 0x50);
    float fSize = (static_cast<int64_t>(curSize) < 0)
        ? (static_cast<float>((curSize >> 1) | (curSize & 1)) * 2.0f)
        : static_cast<float>(curSize);
    float fMaxIdx = (static_cast<int64_t>(maxIdx) < 0)
        ? (static_cast<float>((maxIdx >> 1) | (maxIdx & 1)) * 2.0f)
        : static_cast<float>(maxIdx);

    if ((fSize / fMaxIdx) > *reinterpret_cast<float*>(hashBytes + 0x58)) {
        if (maxIdx <= 0x1ffULL) {
            maxIdx <<= 3;
        } else if (maxIdx < 0x555555555555555ULL) {
            maxIdx <<= 1;
        }

        FfxIter24 valIter = {0, 0, *reinterpret_cast<uint8_t**>(hashBytes + 0x08)};
        uint8_t* pVecRawBegin = *reinterpret_cast<uint8_t**>(hashBytes + 0x28);
        *reinterpret_cast<uint8_t**>(hashBytes + 0x30) = pVecRawBegin;
        FfxIter24 whereIter = {0, 0, pVecRawBegin};
        FfxIter24 outVecIter = {};
        sub_230d170(&outVecIter, hashBytes + 0x20, maxIdx * 2, &valIter, whereIter);

        *reinterpret_cast<uint64_t*>(hashBytes + 0x48) = maxIdx - 1;
        *reinterpret_cast<uint64_t*>(hashBytes + 0x50) = maxIdx;

        uint8_t* pListHead = *reinterpret_cast<uint8_t**>(hashBytes + 0x08);
        uint8_t* pFirstNode = *reinterpret_cast<uint8_t**>(pListHead + 0x00);
        if (pFirstNode != pListHead) {
            uint8_t* pLastNode = *reinterpret_cast<uint8_t**>(pListHead + 0x08);
            for (;;) {
                FfxIter24 reinsertIter = {0, 0, pFirstNode};
                FfxInsertResult32 tmpPair = {};
                sub_230d830(
                    &tmpPair,
                    hashBytes,
                    reinterpret_cast<const uint32_t*>(pFirstNode + 0x10),
                    reinsertIter);
                if (pFirstNode == pLastNode) {
                    break;
                }
                pListHead = *reinterpret_cast<uint8_t**>(hashBytes + 0x08);
                pFirstNode = *reinterpret_cast<uint8_t**>(pListHead + 0x00);
            }
        }
    }

    pOutPair->proxy0 = plistIter.proxy0;
    pOutPair->proxy1 = plistIter.proxy1;
    pOutPair->node = pPlist;
    pOutPair->inserted = 1;
    return pOutPair;
}

// ============================================================================
// [19/66] sub_230d6c0 @ 0x230d6c0 (363 B)
// _Hash<uint32_t, void*>::erase(pOutIter, pHash, pEraseNode)
// ============================================================================
FfxIter24* sub_230d6c0(FfxIter24* pOutIter, void* pHash, uint8_t* pEraseNode) {
    uint8_t* hashBytes = static_cast<uint8_t*>(pHash);
    uint64_t keyVal = *reinterpret_cast<uint32_t*>(pEraseNode + 0x10);
    LDivResult dv = sub_2bbfd68(static_cast<int64_t>(keyVal ^ 0xdeadbeefULL), 0x1f31d);
    int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
    uint64_t hashVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

    uint64_t mask = *reinterpret_cast<uint64_t*>(hashBytes + 0x48);
    uint64_t bucket = hashVal & mask;
    if (*reinterpret_cast<uint64_t*>(hashBytes + 0x50) <= bucket) {
        bucket += ~(mask >> 1);
    }

    FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(hashBytes + 0x28);
    FfxIter24* pSlot0 = &pVecBegin[bucket * 2];
    FfxIter24* pSlot1 = &pVecBegin[bucket * 2 + 1];

    if (pSlot1->node == pEraseNode) {
        if (pSlot0->node == pEraseNode) {
            uint8_t* pHead = *reinterpret_cast<uint8_t**>(hashBytes + 0x08);
            pSlot0->proxy0 = 0;
            pSlot0->proxy1 = 0;
            pSlot0->node = pHead;
            pHead = *reinterpret_cast<uint8_t**>(hashBytes + 0x08);
            pVecBegin = *reinterpret_cast<FfxIter24**>(hashBytes + 0x28);
            pSlot1 = &pVecBegin[bucket * 2 + 1];
            pSlot1->proxy0 = 0;
            pSlot1->proxy1 = 0;
            pSlot1->node = pHead;
        } else {
            pSlot1->proxy0 = 0;
            pSlot1->proxy1 = 0;
            pSlot1->node = *reinterpret_cast<uint8_t**>(pEraseNode + 0x08);
        }
    } else if (pSlot0->node == pEraseNode) {
        pSlot0->proxy0 = 0;
        pSlot0->proxy1 = 0;
        pSlot0->node = *reinterpret_cast<uint8_t**>(pEraseNode + 0x00);
    }

    uint8_t* pNext = *reinterpret_cast<uint8_t**>(pEraseNode + 0x00);
    if (*reinterpret_cast<uint8_t**>(hashBytes + 0x08) != pEraseNode) {
        uint8_t* pPrev = *reinterpret_cast<uint8_t**>(pEraseNode + 0x08);
        *reinterpret_cast<uint8_t**>(pPrev + 0x00) = pNext;
        uint8_t* pNextNode = *reinterpret_cast<uint8_t**>(pEraseNode + 0x00);
        *reinterpret_cast<uint8_t**>(pNextNode + 0x08) = *reinterpret_cast<uint8_t**>(pEraseNode + 0x08);

        void* pAlloc = *reinterpret_cast<void**>(hashBytes + 0x18);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(
            pAlloc, pEraseNode);
        --(*reinterpret_cast<uint64_t*>(hashBytes + 0x10));
    }

    pOutIter->proxy0 = 0;
    pOutIter->proxy1 = 0;
    pOutIter->node = pNext;
    return pOutIter;
}

// ============================================================================
// [20/66] sub_230d170 @ 0x230d170 (1352 B)
// vector<FfxIter24>::insert(pOutIter, pVec, count, pVal, whereIter)
// ============================================================================
FfxIter24* sub_230d170(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter)
{
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    FfxIter24* pFirst = *reinterpret_cast<FfxIter24**>(vecBytes + 0x08);
    FfxIter24* pWhere = reinterpret_cast<FfxIter24*>(whereIter.node);

    if (count != 0) {
        FfxIter24* pLast = *reinterpret_cast<FfxIter24**>(vecBytes + 0x10);
        FfxIter24* pEnd = *reinterpret_cast<FfxIter24**>(vecBytes + 0x18);
        uint64_t avail = static_cast<uint64_t>(pEnd - pLast);

        if (avail < count) {
            uint64_t curLen = static_cast<uint64_t>(pLast - pFirst);
            if (0xaaaaaaaaaaaaaaaULL - curLen < count) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            uint64_t needed = curLen + count;
            uint64_t curCap = static_cast<uint64_t>(pEnd - pFirst);
            uint64_t halfCap = curCap >> 1;
            uint64_t grownCap = (0xaaaaaaaaaaaaaaaULL - halfCap >= curCap) ? (curCap + halfCap) : 0ULL;
            if (grownCap < needed) {
                grownCap = needed;
            }

            void* pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
            FfxIter24* pNewBuf = static_cast<FfxIter24*>(
                (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
                    pAlloc, grownCap * sizeof(FfxIter24), 8));
            if (pNewBuf == nullptr) {
                handleContainerAllocFailure();
            }

            uint64_t insertIdx = static_cast<uint64_t>(pWhere - *reinterpret_cast<FfxIter24**>(vecBytes + 0x08));
            FfxIter24* pDstInsert = pNewBuf + insertIdx;
            for (uint64_t i = 0; i < count; ++i) {
                if (&pDstInsert[i] != nullptr) {
                    pDstInsert[i] = *pVal;
                }
            }

            FfxIter24* pSrcBegin = *reinterpret_cast<FfxIter24**>(vecBytes + 0x08);
            if (pSrcBegin != pWhere) {
                FfxIter24* pDst = pNewBuf;
                for (FfxIter24* pSrc = pSrcBegin; pSrc != pWhere; ++pSrc, ++pDst) {
                    if (pDst != nullptr) {
                        *pDst = *pSrc;
                    }
                }
            }

            FfxIter24* pSrcEnd = *reinterpret_cast<FfxIter24**>(vecBytes + 0x10);
            if (pWhere != pSrcEnd) {
                FfxIter24* pDst = pNewBuf + insertIdx + count;
                for (FfxIter24* pSrc = pWhere; pSrc != pSrcEnd; ++pSrc, ++pDst) {
                    if (pDst != nullptr) {
                        *pDst = *pSrc;
                    }
                }
            }

            FfxIter24* pOldFirst = *reinterpret_cast<FfxIter24**>(vecBytes + 0x08);
            uint64_t oldLen = static_cast<uint64_t>(*reinterpret_cast<FfxIter24**>(vecBytes + 0x10) - pOldFirst);
            uint64_t newLen = oldLen + count;
            if (pOldFirst != nullptr) {
                pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
                allocVtbl = *reinterpret_cast<void**>(pAlloc);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(
                    pAlloc, pOldFirst);
            }

            *reinterpret_cast<FfxIter24**>(vecBytes + 0x18) = pNewBuf + grownCap;
            *reinterpret_cast<FfxIter24**>(vecBytes + 0x10) = pNewBuf + newLen;
            *reinterpret_cast<FfxIter24**>(vecBytes + 0x08) = pNewBuf;
        } else {
            uint64_t tailElems = static_cast<uint64_t>(pLast - pWhere);
            FfxIter24 valCopy = *pVal;
            if (tailElems < count) {
                if (pWhere != pLast) {
                    for (FfxIter24* pSrc = pWhere; pSrc != pLast; ++pSrc) {
                        FfxIter24* pDst = pSrc + count;
                        if (pDst != nullptr) {
                            *pDst = *pSrc;
                        }
                    }
                    pLast = *reinterpret_cast<FfxIter24**>(vecBytes + 0x10);
                }
                uint64_t uninitFill = count - tailElems;
                FfxIter24* pFill = pLast;
                for (uint64_t i = 0; i < uninitFill; ++i, ++pFill) {
                    if (pFill != nullptr) {
                        *pFill = valCopy;
                    }
                }
                pLast = *reinterpret_cast<FfxIter24**>(vecBytes + 0x10);
                *reinterpret_cast<FfxIter24**>(vecBytes + 0x10) = pLast + count;
                for (FfxIter24* pDst = pWhere; pDst != pLast; ++pDst) {
                    *pDst = valCopy;
                }
            } else {
                FfxIter24* pMoveSrc = pLast - count;
                FfxIter24* pMoveDst = pLast;
                for (uint64_t i = 0; i < count; ++i) {
                    if (&pMoveDst[i] != nullptr) {
                        pMoveDst[i] = pMoveSrc[i];
                    }
                }
                *reinterpret_cast<FfxIter24**>(vecBytes + 0x10) = pLast + count;
                if (pMoveSrc != pWhere) {
                    FfxIter24* pDst = pLast - 1;
                    FfxIter24* pSrc = pMoveSrc - 1;
                    while (pSrc >= pWhere) {
                        *pDst = *pSrc;
                        --pDst;
                        --pSrc;
                    }
                }
                for (uint64_t i = 0; i < count; ++i) {
                    pWhere[i] = valCopy;
                }
            }
        }
    }

    uint64_t offsetIdx = static_cast<uint64_t>(pWhere - pFirst);
    pOutIter->proxy0 = 0;
    pOutIter->proxy1 = 0;
    pOutIter->node = reinterpret_cast<uint8_t*>(
        *reinterpret_cast<FfxIter24**>(vecBytes + 0x08) + offsetIdx);
    return pOutIter;
}

// ============================================================================
// [21/66] sub_230cfb0 @ 0x230cfb0 (238 B)
// GXFfxGraphicsResourceManager::RemoveById(pMgr, key)
// ============================================================================
void sub_230cfb0(void* pMgr, uint32_t key) {
    uint8_t* mgrBytes = static_cast<uint8_t*>(pMgr);
    uint64_t keyVal = key;
    LDivResult dv = sub_2bbfd68(static_cast<int64_t>(keyVal ^ 0xdeadbeefULL), 0x1f31d);
    int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
    uint64_t hashVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

    uint64_t mask = *reinterpret_cast<uint64_t*>(mgrBytes + 0x50);
    uint64_t bucket = hashVal & mask;
    if (*reinterpret_cast<uint64_t*>(mgrBytes + 0x58) <= bucket) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(mgrBytes + 0x30);
    uint8_t* pCur = pVecBegin[bucket * 2].node;
    if (pCur == pHead) {
        return;
    }

    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(pVecBegin[bucket * 2 + 1].node + 0x00);
    while (pCur != pEnd) {
        uint32_t curK = *reinterpret_cast<uint32_t*>(pCur + 0x10);
        if (curK >= key) {
            if (curK > key) {
                pCur = pHead;
            }
            if (pCur != pHead) {
                FfxIter24 outIter = {};
                sub_230d6c0(&outIter, mgrBytes + 0x08, pCur);
            }
            return;
        }
        pCur = *reinterpret_cast<uint8_t**>(pCur + 0x00);
    }
}

// ============================================================================
// [22/66] sub_230cd00 @ 0x230cd00 (684 B)
// GXFfxGraphicsResourceManager::InsertUnique(pMgr, key, pValue)
// ============================================================================
bool sub_230cd00(void* pMgr, uint32_t key, void* pValue) {
    uint8_t* mgrBytes = static_cast<uint8_t*>(pMgr);
    uint64_t xoredKey = static_cast<uint64_t>(key) ^ 0xdeadbeefULL;

    LDivResult dv = sub_2bbfd68(static_cast<int64_t>(xoredKey), 0x1f31d);
    int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
    uint64_t hashVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

    uint64_t mask = *reinterpret_cast<uint64_t*>(mgrBytes + 0x50);
    uint64_t bucket = hashVal & mask;
    if (*reinterpret_cast<uint64_t*>(mgrBytes + 0x58) <= bucket) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(mgrBytes + 0x30);
    uint8_t* pCur = pVecBegin[bucket * 2].node;
    uint8_t* pTargetNode = pHead;
    if (pCur != pHead) {
        uint8_t* pEnd = *reinterpret_cast<uint8_t**>(pVecBegin[bucket * 2 + 1].node + 0x00);
        while (pCur != pEnd) {
            uint32_t curK = *reinterpret_cast<uint32_t*>(pCur + 0x10);
            if (curK >= key) {
                if (curK == key) {
                    sub_ee5de0(6);
                    return false;
                }
                break;
            }
            pCur = *reinterpret_cast<uint8_t**>(pCur + 0x00);
        }
    }

    if (pTargetNode == pHead) {
        uint8_t* pFirst = *reinterpret_cast<uint8_t**>(pHead + 0x00);
        uint8_t* pFirstPrev = *reinterpret_cast<uint8_t**>(pFirst + 0x08);
        void* pAlloc = *reinterpret_cast<void**>(mgrBytes + 0x20);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        uint8_t* pNewNode = static_cast<uint8_t*>(
            (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
                pAlloc, 0x20, 8));
        if (pNewNode != nullptr) {
            if (pFirst == nullptr) {
                pFirstPrev = pNewNode;
            }
            *reinterpret_cast<uint8_t**>(pNewNode + 0x00) = (pFirst != nullptr) ? pFirst : pNewNode;
        } else {
            handleContainerAllocFailure();
            if (pFirst == nullptr) {
                pFirstPrev = pNewNode;
            }
        }
        *reinterpret_cast<uint8_t**>(pNewNode + 0x08) = pFirstPrev;
        *reinterpret_cast<uint32_t*>(pNewNode + 0x10) = key;
        *reinterpret_cast<void**>(pNewNode + 0x18) = nullptr;

        uint64_t curSize = *reinterpret_cast<uint64_t*>(mgrBytes + 0x18);
        if (curSize == 0x7fffffffffffffeULL) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        *reinterpret_cast<uint64_t*>(mgrBytes + 0x18) = curSize + 1;
        *reinterpret_cast<uint8_t**>(pFirst + 0x08) = pNewNode;
        *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(pNewNode + 0x08) + 0x00) = pNewNode;

        uint8_t* pFront = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(mgrBytes + 0x10) + 0x00);
        FfxIter24 plistIter = {0, 0, pFront};
        FfxInsertResult32 insPair = {};
        sub_230d830(
            &insPair,
            mgrBytes + 0x08,
            reinterpret_cast<const uint32_t*>(pFront + 0x10),
            plistIter);
        pTargetNode = insPair.node;
    }

    *reinterpret_cast<void**>(pTargetNode + 0x18) = pValue;
    return true;
}

// ============================================================================
// [23/66] sub_230cc30 @ 0x230cc30 (195 B)
// GXFfxGraphicsResourceManager::FindById(pMgr, key)
// ============================================================================
void* sub_230cc30(void* pMgr, uint32_t key) {
    uint8_t* mgrBytes = static_cast<uint8_t*>(pMgr);
    uint64_t keyVal = key;
    LDivResult dv = sub_2bbfd68(static_cast<int64_t>(keyVal ^ 0xdeadbeefULL), 0x1f31d);
    int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
    uint64_t hashVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

    uint64_t mask = *reinterpret_cast<uint64_t*>(mgrBytes + 0x50);
    uint64_t bucket = hashVal & mask;
    if (*reinterpret_cast<uint64_t*>(mgrBytes + 0x58) <= bucket) {
        bucket += ~(mask >> 1);
    }

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(mgrBytes + 0x30);
    uint8_t* pCur = pVecBegin[bucket * 2].node;
    if (pCur == pHead) {
        return nullptr;
    }

    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(pVecBegin[bucket * 2 + 1].node + 0x00);
    while (pCur != pEnd) {
        uint32_t curK = *reinterpret_cast<uint32_t*>(pCur + 0x10);
        if (curK >= key) {
            if (curK > key) {
                pCur = pHead;
            }
            return (pCur != pHead) ? *reinterpret_cast<void**>(pCur + 0x18) : nullptr;
        }
        pCur = *reinterpret_cast<uint8_t**>(pCur + 0x00);
    }
    return nullptr;
}

// ============================================================================
// [24/66] sub_230caf0 @ 0x230caf0 (310 B)
// GXFfxGraphicsResourceManager::Clear(pMgr)
// ============================================================================
void sub_230caf0(void* pMgr) {
    uint8_t* mgrBytes = static_cast<uint8_t*>(pMgr);
    uint8_t* pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pHead + 0x00);

    if (pCur != pHead) {
        do {
            FfxIter24 nextIter = {};
            sub_230d6c0(&nextIter, mgrBytes + 0x08, pCur);
            pCur = nextIter.node;
        } while (pCur != *reinterpret_cast<uint8_t**>(mgrBytes + 0x10));
        pHead = pCur;
        pCur = *reinterpret_cast<uint8_t**>(pHead + 0x00);
    }

    *reinterpret_cast<uint8_t**>(pHead + 0x00) = pHead;
    pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    *reinterpret_cast<uint8_t**>(pHead + 0x08) = pHead;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x18) = 0;

    while (pCur != *reinterpret_cast<uint8_t**>(mgrBytes + 0x10)) {
        uint8_t* pNext = *reinterpret_cast<uint8_t**>(pCur + 0x00);
        void* pAlloc = *reinterpret_cast<void**>(mgrBytes + 0x20);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pCur);
        pCur = pNext;
    }

    FfxIter24 valIter = {0, 0, pCur};
    uint8_t* pVecBegin = *reinterpret_cast<uint8_t**>(mgrBytes + 0x30);
    *reinterpret_cast<uint8_t**>(mgrBytes + 0x38) = pVecBegin;
    FfxIter24 whereIter = {0, 0, pVecBegin};
    FfxIter24 outIter = {};
    sub_230d170(&outIter, mgrBytes + 0x28, 0x10, &valIter, whereIter);

    *reinterpret_cast<uint64_t*>(mgrBytes + 0x50) = 7;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x58) = 8;
}

// ============================================================================
// [25/66] sub_230cae0 @ 0x230cae0 (5 B)
// Thunk to sub_230caf0
// ============================================================================
void sub_230cae0(void* pMgr) {
    sub_230caf0(pMgr);
}

// ============================================================================
// [26/66] sub_230c940 @ 0x230c940 (412 B)
// GXFfxGraphicsResourceManager Constructor
// ============================================================================
void sub_230c940(void* pMgr, void* pAllocator) {
    uint8_t* mgrBytes = static_cast<uint8_t*>(pMgr);
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x00) = GX_VMA(0x53b9250ULL);
    validateContainerHeap(pAllocator);

    *reinterpret_cast<uint64_t*>(mgrBytes + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x10) = 0;
    *reinterpret_cast<void**>(mgrBytes + 0x20) = pAllocator;

    void* allocVtbl = *reinterpret_cast<void**>(pAllocator);
    uint8_t* pHead = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAllocator, 0x20, 8));
    if (pHead != nullptr) {
        *reinterpret_cast<void**>(pHead + 0x00) = pHead;
    } else {
        handleContainerAllocFailure();
        return;
    }
    *reinterpret_cast<void**>(pHead + 0x08) = pHead;
    *reinterpret_cast<void**>(mgrBytes + 0x10) = pHead;

    *reinterpret_cast<uint64_t*>(mgrBytes + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x30) = 0;
    *reinterpret_cast<void**>(mgrBytes + 0x48) = pAllocator;
    *reinterpret_cast<uint32_t*>(mgrBytes + 0x60) = 0x3f800000u;

    FfxIter24 valIter = {0, 0, pHead};
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x38) = 0;
    FfxIter24 whereIter = {0, 0, nullptr};
    FfxIter24 outIter = {};
    sub_230d170(&outIter, mgrBytes + 0x28, 0x10, &valIter, whereIter);

    *reinterpret_cast<uint64_t*>(mgrBytes + 0x50) = 7;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x58) = 8;
}

// ============================================================================
// [27/66] sub_230c640 @ 0x230c640 (754 B)
// GXFfxGraphicsResourceManager Destructor
// ============================================================================
void sub_230c640(void* pMgr) {
    uint8_t* mgrBytes = static_cast<uint8_t*>(pMgr);
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x00) = GX_VMA(0x53b9250ULL);

    uint8_t* pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pHead + 0x00);
    uint8_t* pNext = pHead;

    if (pCur != pHead) {
        do {
            uint64_t keyVal = *reinterpret_cast<uint32_t*>(pCur + 0x10);
            LDivResult dv = sub_2bbfd68(static_cast<int64_t>(keyVal ^ 0xdeadbeefULL), 0x1f31d);
            int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
            uint64_t hashVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

            uint64_t mask = *reinterpret_cast<uint64_t*>(mgrBytes + 0x50);
            uint64_t bucket = hashVal & mask;
            if (*reinterpret_cast<uint64_t*>(mgrBytes + 0x58) <= bucket) {
                bucket += ~(mask >> 1);
            }

            FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(mgrBytes + 0x30);
            FfxIter24* pSlot0 = &pVecBegin[bucket * 2];
            FfxIter24* pSlot1 = &pVecBegin[bucket * 2 + 1];

            if (pSlot1->node == pCur) {
                if (pSlot0->node == pCur) {
                    uint8_t* hNode = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
                    pSlot0->proxy0 = 0;
                    pSlot0->proxy1 = 0;
                    pSlot0->node = hNode;
                    hNode = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
                    pVecBegin = *reinterpret_cast<FfxIter24**>(mgrBytes + 0x30);
                    pSlot1 = &pVecBegin[bucket * 2 + 1];
                    pSlot1->proxy0 = 0;
                    pSlot1->proxy1 = 0;
                    pSlot1->node = hNode;
                } else {
                    pSlot1->proxy0 = 0;
                    pSlot1->proxy1 = 0;
                    pSlot1->node = *reinterpret_cast<uint8_t**>(pCur + 0x08);
                }
            } else if (pSlot0->node == pCur) {
                pSlot0->proxy0 = 0;
                pSlot0->proxy1 = 0;
                pSlot0->node = *reinterpret_cast<uint8_t**>(pCur + 0x00);
            }

            pNext = *reinterpret_cast<uint8_t**>(pCur + 0x00);
            uint8_t* curHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
            if (pCur != curHead) {
                uint8_t* pPrev = *reinterpret_cast<uint8_t**>(pCur + 0x08);
                *reinterpret_cast<uint8_t**>(pPrev + 0x00) = pNext;
                uint8_t* pAfter = *reinterpret_cast<uint8_t**>(pCur + 0x00);
                *reinterpret_cast<uint8_t**>(pAfter + 0x08) = *reinterpret_cast<uint8_t**>(pCur + 0x08);

                void* pAlloc = *reinterpret_cast<void**>(mgrBytes + 0x20);
                void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pCur);
                --(*reinterpret_cast<uint64_t*>(mgrBytes + 0x18));
                curHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
            }
            pCur = pNext;
            pHead = curHead;
        } while (pNext != pHead);
        pCur = *reinterpret_cast<uint8_t**>(pNext + 0x00);
    }

    *reinterpret_cast<uint8_t**>(pNext + 0x00) = pNext;
    pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    *reinterpret_cast<uint8_t**>(pHead + 0x08) = pHead;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x18) = 0;

    while (pCur != *reinterpret_cast<uint8_t**>(mgrBytes + 0x10)) {
        uint8_t* pAfter = *reinterpret_cast<uint8_t**>(pCur + 0x00);
        void* pAlloc = *reinterpret_cast<void**>(mgrBytes + 0x20);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pCur);
        pCur = pAfter;
    }

    *reinterpret_cast<uint64_t*>(mgrBytes + 0x50) = 7;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x58) = 8;

    void* pVecBuf = *reinterpret_cast<void**>(mgrBytes + 0x30);
    if (pVecBuf != nullptr) {
        void* pVecAlloc = *reinterpret_cast<void**>(mgrBytes + 0x48);
        void* vecAllocVtbl = *reinterpret_cast<void**>(pVecAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vecAllocVtbl) + 0x70))(pVecAlloc, pVecBuf);
        *reinterpret_cast<uint64_t*>(mgrBytes + 0x40) = 0;
        *reinterpret_cast<uint64_t*>(mgrBytes + 0x38) = 0;
        *reinterpret_cast<uint64_t*>(mgrBytes + 0x30) = 0;
    }

    pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    pCur = *reinterpret_cast<uint8_t**>(pHead + 0x00);
    *reinterpret_cast<uint8_t**>(pHead + 0x00) = pHead;
    pHead = *reinterpret_cast<uint8_t**>(mgrBytes + 0x10);
    *reinterpret_cast<uint8_t**>(pHead + 0x08) = pHead;
    *reinterpret_cast<uint64_t*>(mgrBytes + 0x18) = 0;

    while (pCur != *reinterpret_cast<uint8_t**>(mgrBytes + 0x10)) {
        uint8_t* pAfter = *reinterpret_cast<uint8_t**>(pCur + 0x00);
        void* pAlloc = *reinterpret_cast<void**>(mgrBytes + 0x20);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pCur);
        pCur = pAfter;
    }

    void* pListAlloc = *reinterpret_cast<void**>(mgrBytes + 0x20);
    void* listAllocVtbl = *reinterpret_cast<void**>(pListAlloc);
    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(listAllocVtbl) + 0x70))(pListAlloc, pCur);
}

// ============================================================================
// [28/66] sub_230c550 @ 0x230c550 (84 B)
// Updates particle instance controller (+0x80) and dispatches vtable+0x40 on +0x50
// ============================================================================
void sub_230c550(void* pSelf, void* pArg1, void* pArg2) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    void* pSub50 = *reinterpret_cast<void**>(self + 0x50);
    void* vtbl50 = *reinterpret_cast<void**>(pSub50);
    auto fn40 = *reinterpret_cast<void (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl50) + 0x40);
    sub_2311c00(self + 0x80, 0xffffffffu);
    fn40(pSub50, pArg1, pArg2, *reinterpret_cast<void**>(self + 0xa8));
}

} // extern "C"

namespace {

// Shared SIMD helper: converts 48-byte SQT (pos, quat, scale) items into 3x4 matrices
// and multiplies by a 4x4 parent transform (rows r0, r1, r2, r3).
static inline void transformParticleSqtBatchTo3x4(
    uint8_t* pDst,
    const uint8_t* pSrc,
    uint32_t count,
    __m128 r0,
    __m128 r1,
    __m128 r2,
    __m128 r3,
    bool reverseLastAddOrder)
{
    const __m128i i0 = _mm_castps_si128(r0);
    const __m128i i1 = _mm_castps_si128(r1);
    const __m128i i2 = _mm_castps_si128(r2);
    const __m128i i3 = _mm_castps_si128(r3);

    const __m128 r0_xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(i0, 0x00));
    const __m128 r1_xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(i1, 0x00));
    const __m128 r2_xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(i2, 0x00));
    const __m128 r3_xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(i3, 0x00));

    const __m128 r0_yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(i0, 0x55));
    const __m128 r1_yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(i1, 0x55));
    const __m128 r2_yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(i2, 0x55));
    const __m128 r3_yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(i3, 0x55));

    const __m128 r0_zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(i0, 0xaa));
    const __m128 r1_zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(i1, 0xaa));
    const __m128 r2_zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(i2, 0xaa));
    const __m128 r3_zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(i3, 0xaa));

    const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    const __m128 maskXYZ = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    const __m128 zeroVec = _mm_setzero_ps();
    const __m128 onesXYZ = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f);
    const __m128 maskY = _mm_castsi128_ps(_mm_setr_epi32(0, -1, 0, 0));
    const __m128 maskXZW = _mm_castsi128_ps(_mm_setr_epi32(-1, 0, -1, -1));

    const __m128 baseX = _mm_mul_ps(r3_xxxx, unitW);
    const __m128 baseY = _mm_mul_ps(r3_yyyy, unitW);
    const __m128 baseZ = _mm_mul_ps(r3_zzzz, unitW);

    for (uint32_t i = 0; i < count; ++i, pSrc += 0x30, pDst += 0x30) {
        __m128 scaleXyz = _mm_and_ps(maskXYZ, _mm_load_ps(reinterpret_cast<const float*>(pSrc + 0x20)));
        __m128 scale4 = _mm_add_ps(scaleXyz, unitW);

        __m128 q = _mm_load_ps(reinterpret_cast<const float*>(pSrc + 0x10));
        __m128 q2 = _mm_add_ps(q, q);
        __m128 qq2 = _mm_mul_ps(q, q2);

        __m128 diagA = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(qq2), 0xc1));
        __m128 diagB = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(qq2), 0xda));
        __m128 diag = _mm_sub_ps(_mm_sub_ps(onesXYZ, diagA), diagB);

        __m128 crossA = _mm_mul_ps(
            _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(q), 0xd0)),
            _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(q2), 0xe6)));
        __m128 crossB = _mm_mul_ps(
            _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(q), 0xff)),
            _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(q2), 0xc9)));

        diag = _mm_and_ps(diag, maskXYZ);
        __m128 sumCross = _mm_add_ps(crossB, crossA);
        __m128 diffCross = _mm_sub_ps(crossA, crossB);

        __m128 tmp5 = _mm_move_ss(zeroVec, sumCross);
        __m128 tmp7 = _mm_shuffle_ps(diffCross, diag, 0xe8);
        tmp7 = _mm_move_ss(tmp7, zeroVec);
        tmp5 = _mm_or_ps(tmp5, tmp7);

        __m128i posI = _mm_load_si128(reinterpret_cast<const __m128i*>(pSrc + 0x00));
        __m128 tmp0 = _mm_shuffle_ps(diffCross, diag, 0x45);
        __m128 tmp1 = _mm_move_ss(sumCross, diffCross);
        __m128 tmp6 = _mm_and_ps(sumCross, maskY);
        tmp0 = _mm_and_ps(tmp0, maskXZW);
        tmp0 = _mm_or_ps(tmp6, tmp0);

        tmp6 = _mm_shuffle_ps(tmp0, tmp5, 0xc2);
        tmp0 = _mm_shuffle_ps(tmp0, tmp5, 0xdd);
        tmp1 = _mm_shuffle_ps(tmp1, tmp5, 0xe8);

        tmp5 = _mm_castsi128_ps(_mm_shuffle_epi32(posI, 0x00));
        tmp5 = _mm_shuffle_ps(tmp6, tmp5, 0xc8);
        tmp5 = _mm_shuffle_ps(tmp6, tmp5, 0xd4);

        tmp6 = _mm_movelh_ps(_mm_castsi128_ps(posI), _mm_castsi128_ps(posI));
        tmp6 = _mm_shuffle_ps(tmp0, tmp6, 0xc8);
        tmp0 = _mm_shuffle_ps(tmp0, tmp6, 0xd4);

        tmp6 = _mm_castsi128_ps(_mm_shuffle_epi32(posI, 0x80));
        tmp6 = _mm_shuffle_ps(tmp1, tmp6, 0xc8);
        tmp1 = _mm_shuffle_ps(tmp1, tmp6, 0xd4);

        tmp5 = _mm_mul_ps(scale4, tmp5);
        tmp0 = _mm_mul_ps(scale4, tmp0);
        tmp1 = _mm_mul_ps(scale4, tmp1);

        __m128 out0;
        __m128 out1;
        __m128 out2;
        if (!reverseLastAddOrder) {
            out0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r0_xxxx, tmp5), _mm_mul_ps(r1_xxxx, tmp0)), _mm_mul_ps(r2_xxxx, tmp1));
            out1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r0_yyyy, tmp5), _mm_mul_ps(r1_yyyy, tmp0)), _mm_mul_ps(r2_yyyy, tmp1));
            out2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r0_zzzz, tmp5), _mm_mul_ps(r1_zzzz, tmp0)), _mm_mul_ps(r2_zzzz, tmp1));
        } else {
            out0 = _mm_add_ps(_mm_mul_ps(r2_xxxx, tmp1), _mm_add_ps(_mm_mul_ps(r0_xxxx, tmp5), _mm_mul_ps(r1_xxxx, tmp0)));
            out1 = _mm_add_ps(_mm_mul_ps(r2_yyyy, tmp1), _mm_add_ps(_mm_mul_ps(r0_yyyy, tmp5), _mm_mul_ps(r1_yyyy, tmp0)));
            out2 = _mm_add_ps(_mm_mul_ps(r2_zzzz, tmp1), _mm_add_ps(_mm_mul_ps(r0_zzzz, tmp5), _mm_mul_ps(r1_zzzz, tmp0)));
        }

        _mm_store_ps(reinterpret_cast<float*>(pDst + 0x00), _mm_add_ps(baseX, out0));
        _mm_store_ps(reinterpret_cast<float*>(pDst + 0x10), _mm_add_ps(baseY, out1));
        _mm_store_ps(reinterpret_cast<float*>(pDst + 0x20), _mm_add_ps(baseZ, out2));
    }
}

static inline __m128 newtonRaphsonLengthVec4(__m128 v) {
    __m128 sq = _mm_mul_ps(v, v);
    __m128 sum = _mm_hadd_ps(sq, sq);
    sum = _mm_hadd_ps(sum, sum);
    __m128 y = _mm_rsqrt_ps(sum);
    __m128 zero = _mm_setzero_ps();
    __m128 isZero = _mm_or_ps(_mm_cmpeq_ps(sum, zero), _mm_cmpeq_ps(y, zero));
    __m128 half = _mm_set1_ps(0.5f);
    __m128 three = _mm_set1_ps(3.0f);

    y = _mm_mul_ps(_mm_mul_ps(y, half), _mm_sub_ps(three, _mm_mul_ps(y, _mm_mul_ps(sum, y))));
    y = _mm_mul_ps(_mm_mul_ps(y, half), _mm_sub_ps(three, _mm_mul_ps(y, _mm_mul_ps(sum, y))));
    __m128 len = _mm_mul_ps(sum, y);
    return _mm_blendv_ps(len, sum, isZero);
}

static inline void byteTableGather16(
    uint8_t* pDst16,
    const uint8_t* pIdx16,
    const uint8_t* pSrc0,
    const uint8_t* pSrc1,
    const uint8_t* pSrc2,
    uint8_t* pSrc3)
{
    __m128i idx   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pIdx16));
    __m128i loIdx = _mm_and_si128(idx, _mm_set1_epi8(0x0f));
    __m128i v0    = _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc0)), loIdx);
    __m128i v1    = _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc1)), loIdx);
    __m128i v2    = _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc2)), loIdx);
    __m128i v3    = _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc3)), loIdx);
    __m128i sel0  = _mm_slli_epi16(idx, 3);
    __m128i sel1  = _mm_slli_epi16(idx, 2);
    __m128i v01   = _mm_blendv_epi8(v0, v1, sel0);
    __m128i v23   = _mm_blendv_epi8(v2, v3, sel0);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst16), _mm_blendv_epi8(v01, v23, sel1));
}

} // namespace

extern "C" {

// ============================================================================
// [29/66] sub_230c170 @ 0x230c170 (987 B)
// Transforms particle instance buffer (camera-relative or world) and submits draw
// ============================================================================
void sub_230c170(void* pSelf, void* pDrawCtx, uint32_t slotIndex) {
    if (slotIndex > 3u) {
        return;
    }

    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint8_t* drawCtx = static_cast<uint8_t*>(pDrawCtx);
    void* pSlotObj = *reinterpret_cast<void**>(self + static_cast<size_t>(slotIndex) * 8 + 0x50);
    if (pSlotObj == nullptr) {
        return;
    }

    void* pBuf70 = *reinterpret_cast<void**>(self + 0x70);
    if (pBuf70 == nullptr) {
        return;
    }

    uint32_t count = *reinterpret_cast<uint32_t*>(self + 0x90);
    if (count == 0) {
        return;
    }

    if (drawCtx[0x44] == 0) {
        void* vtbl = *reinterpret_cast<void**>(pSlotObj);
        (*reinterpret_cast<void (**)(void*, void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x18))(
            pSlotObj, pDrawCtx, pBuf70);
        return;
    }

    __m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x10));
    __m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x20));
    __m128 r2 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x30));
    uint8_t* pSubCtx08 = *reinterpret_cast<uint8_t**>(drawCtx + 0x08);
    __m128 r3 = _mm_load_ps(reinterpret_cast<const float*>(pSubCtx08 + 0x80));

    sub_2311c00(self + 0x80, 0xffffffffu);
    const uint8_t* pSrcParticles = *reinterpret_cast<const uint8_t**>(self + 0xa8);

    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    uint8_t* pTlsHeap = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    if (pTlsHeap == nullptr) {
        sub_207d250();
        fsBase = gx_guest_tls_base();
        pTlsHeap = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
        if (pTlsHeap == nullptr) {
            sub_207d250();
            fsBase = gx_guest_tls_base();
            pTlsHeap = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
        }
    }
    if (pTlsHeap == nullptr) {
        return;
    }
    void* pSavedCursor = *reinterpret_cast<void**>(pTlsHeap + 0x10);

    void* pHeapAlloc = pTlsHeap + 0x20;
    void* heapVtbl = *reinterpret_cast<void**>(pHeapAlloc);
    uint8_t* pTempParticles = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(heapVtbl) + 0x58))(
            pHeapAlloc, static_cast<size_t>(count) * 0x30, 0x10));

    if (pTempParticles != nullptr) {
        transformParticleSqtBatchTo3x4(pTempParticles, pSrcParticles, count, r0, r1, r2, r3, false);

        void* slotVtbl = *reinterpret_cast<void**>(pSlotObj);
        (*reinterpret_cast<void (**)(void*, void*, void*)>(static_cast<uint8_t*>(slotVtbl) + 0x18))(
            pSlotObj, pDrawCtx, pTempParticles);
    }

    fsBase = gx_guest_tls_base();
    pTlsHeap = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    if (pTlsHeap == nullptr) {
        sub_207d250();
        fsBase = gx_guest_tls_base();
        pTlsHeap = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    }

    if (*reinterpret_cast<void**>(pTlsHeap + 0x00) > pSavedCursor ||
        *reinterpret_cast<void**>(pTlsHeap + 0x08) <= pSavedCursor) {
        sub_20b55b0(
            reinterpret_cast<const char*>(GX_VMA(0x493c62dULL)),
            0x233,
            reinterpret_cast<const char*>(GX_VMA(0x493c690ULL)));
    }
    if (*reinterpret_cast<void**>(pTlsHeap + 0x10) > pSavedCursor) {
        *reinterpret_cast<void**>(pTlsHeap + 0x10) = pSavedCursor;
    }
}

// ============================================================================
// [30/66] sub_230c160 @ 0x230c160 (1 B)
// Empty virtual no-op
// ============================================================================
void sub_230c160() {
}

// ============================================================================
// [31/66] sub_230ba00 @ 0x230ba00 (1875 B)
// Decomposes a 3x4 affine matrix into SQT (Scale, Quaternion, Translation)
// and forwards to sub_2311430(self + 0x80, index, &sqt, 0)
// ============================================================================
void sub_230ba00(void* pSelf, uint32_t index, const float* pMatrix3x4) {
    __m128 m0 = _mm_load_ps(pMatrix3x4 + 0);
    __m128 m1 = _mm_load_ps(pMatrix3x4 + 4);
    __m128 m2 = _mm_load_ps(pMatrix3x4 + 8);

    __m128 t3 = _mm_movelh_ps(m0, m1);
    __m128 t0 = _mm_movehl_ps(m1, m0);
    __m128 t1 = _mm_castsi128_ps(_mm_move_epi64(_mm_castps_si128(m2)));
    __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    __m128 t2 = _mm_movehl_ps(unitW, m2);

    __m128 col0 = _mm_shuffle_ps(t3, t1, 0x88);
    __m128 col1 = _mm_shuffle_ps(t3, t1, 0xdd);
    __m128 col2 = _mm_shuffle_ps(t0, t2, 0x88);
    __m128 trans = _mm_shuffle_ps(t0, t2, 0xdd);

    __m128 len0 = newtonRaphsonLengthVec4(col0);
    __m128 len1 = newtonRaphsonLengthVec4(col1);
    __m128 len2 = newtonRaphsonLengthVec4(col2);

    __m128 ones = _mm_set1_ps(1.0f);
    alignas(16) __m128 normCol0 = _mm_mul_ps(col0, _mm_div_ps(ones, len0));
    alignas(16) __m128 normCol1 = _mm_mul_ps(col1, _mm_div_ps(ones, len1));
    alignas(16) __m128 normCol2 = _mm_mul_ps(col2, _mm_div_ps(ones, len2));

    __m128 scaleVec = _mm_shuffle_ps(_mm_unpacklo_ps(len0, len1), len2, 0xe4);

    __m128 d0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(normCol0), 0x00));
    __m128 d1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(normCol1), 0x55));
    __m128 d2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(normCol2), 0xaa));

    __m128 traceSigns0 = _mm_setr_ps( 1.0f, -1.0f, -1.0f,  1.0f);
    __m128 traceSigns1 = _mm_setr_ps(-1.0f,  1.0f, -1.0f,  1.0f);
    __m128 traceSigns2 = _mm_setr_ps(-1.0f, -1.0f,  1.0f,  1.0f);

    alignas(16) __m128 traceVec = _mm_add_ps(
        _mm_mul_ps(d2, traceSigns2),
        _mm_add_ps(_mm_mul_ps(d0, traceSigns0), _mm_mul_ps(d1, traceSigns1)));

    __m128 tracePlus1 = _mm_add_ps(traceVec, ones);
    __m128 y = _mm_rsqrt_ps(tracePlus1);
    __m128 zero = _mm_setzero_ps();
    __m128 isZero = _mm_or_ps(_mm_cmpeq_ps(tracePlus1, zero), _mm_cmpeq_ps(y, zero));
    __m128 half = _mm_set1_ps(0.5f);
    __m128 three = _mm_set1_ps(3.0f);

    y = _mm_mul_ps(_mm_mul_ps(y, half), _mm_sub_ps(three, _mm_mul_ps(y, _mm_mul_ps(tracePlus1, y))));
    y = _mm_mul_ps(_mm_mul_ps(y, half), _mm_sub_ps(three, _mm_mul_ps(y, _mm_mul_ps(tracePlus1, y))));
    __m128 sqrtTrace = _mm_blendv_ps(_mm_mul_ps(tracePlus1, y), tracePlus1, isZero);
    __m128 invSqrt = _mm_div_ps(ones, sqrtTrace);

    __m128 infMask = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));
    __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    __m128 isPosInf = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(_mm_and_ps(invSqrt, infMask)), _mm_castps_si128(infMask)));
    __m128 isAnyInf = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(_mm_and_ps(invSqrt, absMask)), _mm_castps_si128(infMask)));
    __m128 useRaw = _mm_castsi128_ps(_mm_andnot_si128(_mm_castps_si128(isAnyInf), _mm_castps_si128(isPosInf)));

    __m128 mulTrace = _mm_mul_ps(tracePlus1, invSqrt);
    __m128 selTrace = _mm_or_ps(_mm_and_ps(tracePlus1, useRaw), _mm_andnot_ps(useRaw, mulTrace));

    alignas(16) __m128 halfSelTrace = _mm_mul_ps(selTrace, half);
    alignas(16) __m128 halfInvSqrt = _mm_mul_ps(invSqrt, half);

    alignas(16) static constexpr uint32_t kTbl4739550[4] = { 0x03020100u, 0x03020100u, 0x07060504u, 0x0f0e0d0cu };
    alignas(16) static constexpr uint32_t kTbl4739560[4] = { 0x07060504u, 0x0b0a0908u, 0x0b0a0908u, 0x1f1e1d1cu };
    alignas(16) static constexpr uint32_t kTbl4739570[4] = { 0x03020100u, 0x0b0a0908u, 0x13121110u, 0x17161514u };
    alignas(16) static constexpr uint32_t kTbl4739580[4] = { 0x07060504u, 0x17161514u, 0x1b1a1918u, 0x0f0e0d0cu };
    alignas(16) static constexpr uint32_t kTbl4739590[4] = { 0x13121110u, 0x1f1e1d1cu, 0x0b0a0908u, 0x0f0e0d0cu };
    alignas(16) static constexpr uint32_t kTbl4738090[4] = { 0x34000000u, 0x34000000u, 0x34000000u, 0x34000000u };

    alignas(16) uint8_t scratch90[16];
    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(kTbl4739550),
        reinterpret_cast<const uint8_t*>(&traceVec),
        reinterpret_cast<const uint8_t*>(&traceVec),
        reinterpret_cast<const uint8_t*>(kTbl4739550),
        scratch90);
    __m128 shufTraceA = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(kTbl4739560),
        reinterpret_cast<const uint8_t*>(&traceVec),
        reinterpret_cast<const uint8_t*>(kTbl4738090),
        reinterpret_cast<const uint8_t*>(kTbl4739560),
        scratch90);
    __m128 shufTraceB = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    __m128 cmpMask = _mm_cmple_ps(shufTraceB, shufTraceA);
    __m128i cmpI = _mm_castps_si128(cmpMask);
    __m128i m4 = _mm_shuffle_epi32(cmpI, 0x00);
    __m128i m6 = _mm_shuffle_epi32(cmpI, 0x55);
    __m128i m7 = _mm_shuffle_epi32(cmpI, 0xaa);
    __m128i m14 = _mm_shuffle_epi32(cmpI, 0xff);

    __m128i allOnes = _mm_set1_epi32(-1);
    __m128i cFa0 = _mm_set1_epi32(0x07060504);
    __m128i cFb0 = _mm_set1_epi32(0x0b0a0908);
    __m128i cFc0 = _mm_setr_epi32(static_cast<int32_t>(0xbf800000u), 0x3f800000, 0x3f800000, 0x3f800000);
    __m128i cFd0 = _mm_setr_epi32(0x13121110, 0x07060504, 0x17161514, 0x1b1a1918);
    __m128i cFe0 = _mm_setr_epi32(0x1b1a1918, 0x17161514, 0x0b0a0908, 0x13121110);
    __m128i cFf0 = _mm_set1_epi32(0x03020100);
    __m128i c000 = _mm_setr_epi32(0x03020100, 0x13121110, 0x1b1a1918, 0x17161514);
    __m128i c010 = _mm_set1_epi32(0x0f0e0d0c);
    __m128i c020 = _mm_setr_epi32(0x17161514, 0x1b1a1918, 0x13121110, 0x0f0e0d0c);

    __m128i x1 = _mm_or_si128(_mm_and_si128(m7, cFa0), _mm_andnot_si128(m7, cFb0));
    __m128i x11 = _mm_andnot_si128(m7, cFc0);
    __m128i x5 = _mm_or_si128(_mm_and_si128(m7, cFd0), _mm_andnot_si128(m7, cFe0));

    __m128i x12 = _mm_xor_si128(m4, allOnes);
    x1 = _mm_andnot_si128(m4, x1);
    __m128i x0 = _mm_or_si128(_mm_and_si128(m4, cFb0), x1);
    x5 = _mm_andnot_si128(m4, x5);
    __m128i x3 = _mm_or_si128(_mm_and_si128(m4, cFe0), x5);

    __m128i x13 = _mm_xor_si128(m6, allOnes);
    x0 = _mm_or_si128(_mm_and_si128(m6, cFf0), _mm_andnot_si128(m6, x0));
    __m128i x2 = _mm_or_si128(_mm_and_si128(m6, c000), _mm_andnot_si128(m6, x3));

    x0 = _mm_or_si128(_mm_and_si128(x0, m4), x1);
    x1 = _mm_or_si128(_mm_and_si128(x2, m4), x5);

    __m128i x10 = _mm_xor_si128(m14, allOnes);
    alignas(16) __m128i idxTableD0 = _mm_or_si128(_mm_and_si128(m14, c010), _mm_andnot_si128(m14, x0));
    alignas(16) __m128i idxTableE0 = _mm_or_si128(_mm_and_si128(m14, c020), _mm_andnot_si128(m14, x1));

    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(&idxTableD0),
        reinterpret_cast<const uint8_t*>(&halfInvSqrt),
        reinterpret_cast<const uint8_t*>(&halfInvSqrt),
        reinterpret_cast<const uint8_t*>(&idxTableD0),
        scratch90);
    __m128 gatheredScale = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    __m128i c030 = _mm_setr_epi32(0x3f800000, 0x3f800000, static_cast<int32_t>(0xbf800000u), 0x3f800000);
    __m128i c040 = _mm_setr_epi32(0x3f800000, static_cast<int32_t>(0xbf800000u), 0x3f800000, 0x3f800000);
    __m128i c050 = _mm_set1_epi32(static_cast<int32_t>(0xbf800000u));

    x0 = _mm_and_si128(_mm_or_si128(_mm_and_si128(m7, c030), x11), x12);
    x1 = _mm_and_si128(_mm_or_si128(_mm_and_si128(m4, cFc0), x0), x13);
    x1 = _mm_or_si128(_mm_and_si128(m6, c040), x1);
    x0 = _mm_or_si128(_mm_and_si128(x1, m4), x0);

    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(kTbl4739570),
        reinterpret_cast<const uint8_t*>(&normCol1),
        reinterpret_cast<const uint8_t*>(&normCol2),
        reinterpret_cast<const uint8_t*>(kTbl4739570),
        scratch90);
    alignas(16) __m128 partF0 = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    x0 = _mm_and_si128(x0, x10);
    x1 = _mm_and_si128(m14, c050);

    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(kTbl4739580),
        reinterpret_cast<const uint8_t*>(&normCol0),
        reinterpret_cast<const uint8_t*>(&partF0),
        reinterpret_cast<const uint8_t*>(kTbl4739580),
        scratch90);
    __m128 termA = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(kTbl4739590),
        reinterpret_cast<const uint8_t*>(&normCol0),
        reinterpret_cast<const uint8_t*>(&partF0),
        reinterpret_cast<const uint8_t*>(kTbl4739590),
        scratch90);
    __m128 termB = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    __m128 signMask = _mm_castsi128_ps(_mm_or_si128(x1, x0));
    alignas(16) __m128 offDiagQuat = _mm_mul_ps(
        gatheredScale,
        _mm_add_ps(termA, _mm_mul_ps(signMask, termB)));

    byteTableGather16(
        scratch90,
        reinterpret_cast<const uint8_t*>(&idxTableE0),
        reinterpret_cast<const uint8_t*>(&halfSelTrace),
        reinterpret_cast<const uint8_t*>(&offDiagQuat),
        reinterpret_cast<const uint8_t*>(&idxTableE0),
        scratch90);
    __m128 rawQuat = _mm_load_ps(reinterpret_cast<const float*>(scratch90));

    __m128 qSq = _mm_mul_ps(rawQuat, rawQuat);
    __m128 qHl = _mm_movehl_ps(qSq, qSq);
    __m128 qSum2 = _mm_add_ps(qSq, qHl);
    __m128 qShuf1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(qSum2), 0x40));
    __m128 qShuf2 = _mm_shuffle_ps(qHl, qShuf1, 0xf0);
    __m128 qNormSq = _mm_add_ps(qShuf1, qShuf2);
    qNormSq = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(qNormSq), 0xaa));

    __m128 qY = _mm_rsqrt_ps(qNormSq);
    __m128 qZero = _mm_setzero_ps();
    __m128 qIsZero = _mm_or_ps(_mm_cmpeq_ps(qNormSq, qZero), _mm_cmpeq_ps(qY, qZero));
    qY = _mm_mul_ps(_mm_mul_ps(qY, half), _mm_sub_ps(three, _mm_mul_ps(qY, _mm_mul_ps(qNormSq, qY))));
    qY = _mm_mul_ps(_mm_mul_ps(qY, half), _mm_sub_ps(three, _mm_mul_ps(qY, _mm_mul_ps(qNormSq, qY))));
    __m128 qLen = _mm_blendv_ps(_mm_mul_ps(qNormSq, qY), qNormSq, qIsZero);

    __m128 qLenNonZero = _mm_cmpneq_ps(qZero, qLen);
    __m128 qNormNotInf = _mm_cmpneq_ps(qNormSq, infMask);
    __m128 nanConst = _mm_castsi128_ps(_mm_set1_epi32(0x7fc00000));
    __m128 qNormalized = _mm_or_ps(
        _mm_andnot_ps(qNormNotInf, nanConst),
        _mm_and_ps(_mm_and_ps(_mm_div_ps(rawQuat, qLen), qLenNonZero), qNormNotInf));

    alignas(16) __m128 sqtPacket[3];
    sqtPacket[0] = trans;
    sqtPacket[1] = qNormalized;
    sqtPacket[2] = scaleVec;

    sub_2311430(static_cast<uint8_t*>(pSelf) + 0x80, index, sqtPacket, 0);
}

// ============================================================================
// [32/66] sub_230b760 @ 0x230b760 (664 B)
// Recomputes world-space 3x4 matrices in pSelf+0x70 when dirty flag (+0x78) is set
// ============================================================================
void sub_230b760(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    if (self[0x78] == 0) {
        return;
    }
    uint8_t* pDstBuf = *reinterpret_cast<uint8_t**>(self + 0x70);
    if (pDstBuf == nullptr) {
        return;
    }
    uint32_t count = *reinterpret_cast<uint32_t*>(self + 0x90);
    if (count == 0) {
        return;
    }

    __m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x10));
    __m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x20));
    __m128 r2 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x30));
    __m128 r3 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x40));

    sub_2311c00(self + 0x80, 0xffffffffu);

    pDstBuf = *reinterpret_cast<uint8_t**>(self + 0x70);
    const uint8_t* pSrcBuf = *reinterpret_cast<const uint8_t**>(self + 0xa8);
    transformParticleSqtBatchTo3x4(pDstBuf, pSrcBuf, count, r0, r1, r2, r3, false);

    self[0x78] = 0;
}

} // extern "C"

namespace {

static inline void releaseDlRefCountObj(uint8_t* pOwner, size_t slotOffset) {
    uint8_t* pObj = *reinterpret_cast<uint8_t**>(pOwner + slotOffset);
    if (pObj == nullptr) {
        return;
    }
    int32_t oldRef = __atomic_fetch_add(reinterpret_cast<int32_t*>(pObj + 0x08), -1, __ATOMIC_SEQ_CST);
    if (oldRef == 1) {
        void* vtbl = *reinterpret_cast<void**>(pObj);
        (*reinterpret_cast<void (**)(void*)>(vtbl))(pObj);
    } else if (oldRef <= 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4940331ULL)));
    }
    *reinterpret_cast<void**>(pOwner + slotOffset) = nullptr;
}

static inline void assignDlRefCountObj(uint8_t* pOwner, size_t slotOffset, uint8_t* pNewObj) {
    uint8_t* pCur = *reinterpret_cast<uint8_t**>(pOwner + slotOffset);
    if (pCur == pNewObj) {
        return;
    }
    if (pCur != nullptr) {
        releaseDlRefCountObj(pOwner, slotOffset);
    }
    *reinterpret_cast<uint8_t**>(pOwner + slotOffset) = pNewObj;
    if (pNewObj != nullptr) {
        __atomic_fetch_add(reinterpret_cast<int32_t*>(pNewObj + 0x08), 1, __ATOMIC_SEQ_CST);
    }
}

} // namespace

extern "C" {

// ============================================================================
// [33/66] sub_230b2b0 @ 0x230b2b0 (1199 B)
// Binds particle resource objects from pSrcObj (+0x20), allocates instance buffer,
// and performs initial SQT-to-3x4 matrix batch transform
// ============================================================================
bool sub_230b2b0(void* pSelf, const void* pSrcObj) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    const uint8_t* src = static_cast<const uint8_t*>(pSrcObj);

    sub_230b080(self);
    __m128i pair20 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + 0x20));

    uint8_t* pRes0 = reinterpret_cast<uint8_t*>(_mm_cvtsi128_si64(pair20));
    assignDlRefCountObj(self, 0x50, pRes0);

    uint8_t* pRes1 = reinterpret_cast<uint8_t*>(_mm_extract_epi64(pair20, 1));
    assignDlRefCountObj(self, 0x58, pRes1);

    if (*reinterpret_cast<void**>(self + 0x60) != nullptr) {
        releaseDlRefCountObj(self, 0x60);
        *reinterpret_cast<void**>(self + 0x60) = nullptr;
    }
    if (*reinterpret_cast<void**>(self + 0x68) != nullptr) {
        releaseDlRefCountObj(self, 0x68);
        *reinterpret_cast<void**>(self + 0x68) = nullptr;
    }

    void* pObj50 = *reinterpret_cast<void**>(self + 0x50);
    if (pObj50 == nullptr) {
        sub_ee5de0(6);
        return false;
    }

    void* vtbl50 = *reinterpret_cast<void**>(pObj50);
    uint8_t* pData48 = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vtbl50) + 0x48))(pObj50));
    if (pData48 == nullptr) {
        sub_ee5de0(6);
        return false;
    }

    uint16_t maxInstances = *reinterpret_cast<uint16_t*>(pData48 + 0x18);

    if (!sub_2311180(self + 0x80, pData48, 0)) {
        sub_ee5de0(6);
        return false;
    }

    self[0x78] = 1;
    if (maxInstances != 0) {
        void* pAlloc = *reinterpret_cast<void**>(self + 0x08);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        uint8_t* pInstBuf = static_cast<uint8_t*>(
            (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
                pAlloc, static_cast<size_t>(maxInstances) * 0x30, 0x10));
        *reinterpret_cast<uint8_t**>(self + 0x70) = pInstBuf;

        if (self[0x78] != 0 && pInstBuf != nullptr) {
            uint32_t count = *reinterpret_cast<uint32_t*>(self + 0x90);
            if (count != 0) {
                __m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x10));
                __m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x20));
                __m128 r2 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x30));
                __m128 r3 = _mm_load_ps(reinterpret_cast<const float*>(self + 0x40));

                sub_2311c00(self + 0x80, 0xffffffffu);

                pInstBuf = *reinterpret_cast<uint8_t**>(self + 0x70);
                const uint8_t* pSrcBuf = *reinterpret_cast<const uint8_t**>(self + 0xa8);
                transformParticleSqtBatchTo3x4(pInstBuf, pSrcBuf, count, r0, r1, r2, r3, true);
                self[0x78] = 0;
            }
        }
    }
    return true;
}

// ============================================================================
// [34/66] sub_230b080 @ 0x230b080 (547 B)
// Releases particle instance resources and reference-counted slots (+0x50..+0xa8)
// ============================================================================
void sub_230b080(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    releaseDlRefCountObj(self, 0x50);
    releaseDlRefCountObj(self, 0x58);
    releaseDlRefCountObj(self, 0x60);
    releaseDlRefCountObj(self, 0x68);
    releaseDlRefCountObj(self, 0x88);

    void* pBuf98 = *reinterpret_cast<void**>(self + 0x98);
    if (pBuf98 != nullptr) {
        void* pAlloc80 = *reinterpret_cast<void**>(self + 0x80);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc80);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc80, pBuf98);
        *reinterpret_cast<void**>(self + 0x98) = nullptr;
    }

    void* pBufA8 = *reinterpret_cast<void**>(self + 0xa8);
    if (pBufA8 != nullptr) {
        void* pAlloc80 = *reinterpret_cast<void**>(self + 0x80);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc80);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc80, pBufA8);
        *reinterpret_cast<void**>(self + 0xa8) = nullptr;
    }

    *reinterpret_cast<uint64_t*>(self + 0xa0) = 0;
    self[0xb0] = 0;

    void* pBuf70 = *reinterpret_cast<void**>(self + 0x70);
    if (pBuf70 != nullptr) {
        uint64_t fsBase = 0;
        fsBase = gx_guest_tls_base();
        uint8_t* pTlsSlot = reinterpret_cast<uint8_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e44a8ULL)));
        void* pHeap = *reinterpret_cast<void**>(pTlsSlot + 0x10);
        *reinterpret_cast<void**>(pTlsSlot + 0x10) = nullptr;
        if (pHeap == nullptr) {
            pHeap = sub_207b720(pBuf70);
            if (pHeap == nullptr) {
                sub_20b55b0(
                    reinterpret_cast<const char*>(GX_VMA(0x493bac3ULL)),
                    0x64,
                    reinterpret_cast<const char*>(GX_VMA(0x4940207ULL)));
            }
        }
        void* heapVtbl = *reinterpret_cast<void**>(pHeap);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(heapVtbl) + 0x70))(pHeap, pBuf70);
        *reinterpret_cast<void**>(self + 0x70) = nullptr;
    }
}

// ============================================================================
// [35/66] sub_230af50 @ 0x230af50 (299 B)
// Non-deleting destructor for particle instance sub-controller (vtable 0x53625b0)
// ============================================================================
void sub_230af50(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x53625b0ULL);
    sub_230b080(self);
    sub_2311020(self + 0x80);
    releaseDlRefCountObj(self, 0x68);
    releaseDlRefCountObj(self, 0x60);
    releaseDlRefCountObj(self, 0x58);
    releaseDlRefCountObj(self, 0x50);
}

// ============================================================================
// [36/66] sub_230af30 @ 0x230af30 (28 B)
// Deleting destructor for particle instance sub-controller
// ============================================================================
void sub_230af30(void* pSelf) {
    sub_230af50(pSelf);
    sub_2bbe588(pSelf);
}

// ============================================================================
// [37/66] sub_230ae70 @ 0x230ae70 (181 B)
// Constructor for 0xc0-byte particle instance sub-controller (vtable 0x53625b0)
// ============================================================================
void sub_230ae70(void* pSelf, void* pAllocator) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x53625b0ULL);
    *reinterpret_cast<void**>(self + 0x08) = pAllocator;

    _mm_store_ps(reinterpret_cast<float*>(self + 0x10), _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f));
    _mm_store_ps(reinterpret_cast<float*>(self + 0x20), _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f));
    _mm_store_ps(reinterpret_cast<float*>(self + 0x30), _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f));
    _mm_store_ps(reinterpret_cast<float*>(self + 0x40), _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

    *reinterpret_cast<uint64_t*>(self + 0x70) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x68) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x60) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x58) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x50) = 0;
    self[0x78] = 1;
    *reinterpret_cast<void**>(self + 0x80) = pAllocator;
    *reinterpret_cast<uint64_t*>(self + 0x88) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x90) = 0;
    *reinterpret_cast<uint16_t*>(self + 0xb0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xa8) = 0;
    *reinterpret_cast<uint64_t*>(self + 0xa0) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x98) = 0;
}

// ============================================================================
// [38/66] sub_230ae10 @ 0x230ae10 (88 B)
// Dispatches update + vtable+0x40 via outer->sub28 (+0x28)
// ============================================================================
void sub_230ae10(void* pOuter, void* pArg1, void* pArg2) {
    uint8_t* pSub28 = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pOuter) + 0x28);
    void* pSub50 = *reinterpret_cast<void**>(pSub28 + 0x50);
    void* vtbl50 = *reinterpret_cast<void**>(pSub50);
    auto fn40 = *reinterpret_cast<void (**)(void*, void*, void*, void*)>(static_cast<uint8_t*>(vtbl50) + 0x40);
    sub_2311c00(pSub28 + 0x80, 0xffffffffu);
    fn40(pSub50, pArg1, pArg2, *reinterpret_cast<void**>(pSub28 + 0xa8));
}

// ============================================================================
// [39/66] sub_230add0 @ 0x230add0 (56 B)
// Sets 4x4 matrix on outer->sub28 (+0x10..+0x40), marks dirty (+0x78), and updates
// ============================================================================
void sub_230add0(void* pOuter, const float* pMatrix4x4) {
    uint8_t* pSub28 = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pOuter) + 0x28);
    _mm_store_ps(reinterpret_cast<float*>(pSub28 + 0x10), _mm_load_ps(pMatrix4x4 + 0));
    _mm_store_ps(reinterpret_cast<float*>(pSub28 + 0x20), _mm_load_ps(pMatrix4x4 + 4));
    _mm_store_ps(reinterpret_cast<float*>(pSub28 + 0x30), _mm_load_ps(pMatrix4x4 + 8));
    _mm_store_ps(reinterpret_cast<float*>(pSub28 + 0x40), _mm_load_ps(pMatrix4x4 + 12));
    pSub28[0x78] = 1;
    pSub28 = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pOuter) + 0x28);
    sub_230b760(pSub28);
}


// ============================================================================
// [40/66] sub_230a5a0 @ 0x230a5a0 (2091 B)
// Builds draw parameter packets (paramA/paramB), initializes draw context,
// and dispatches particle draw via sub_230c170(*qword(pSelf + 0x28), drawCtx, 0)
// ============================================================================
void sub_230a5a0(void* pSelf, void* pRenderCtx, const void* pDrawDesc) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    uint8_t* renderCtx = static_cast<uint8_t*>(pRenderCtx);
    const uint8_t* desc = static_cast<const uint8_t*>(pDrawDesc);

    alignas(32) uint8_t paramA[0x160];
    std::memset(paramA, 0, sizeof(paramA));

    *reinterpret_cast<uint64_t*>(paramA + 0xd8) = 0xffffffffffffffffULL;
    *reinterpret_cast<uint64_t*>(paramA + 0xd0) = 0xffffffffffffffffULL;
    *reinterpret_cast<uint64_t*>(paramA + 0x60) = 0xffffffffffffffffULL;
    *reinterpret_cast<uint32_t*>(paramA + 0xc4) = 0xffffffffu;
    *reinterpret_cast<uint32_t*>(paramA + 0xcc) = 0xffffffffu;
    *reinterpret_cast<uint32_t*>(paramA + 0x150) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(paramA + 0x154) = 0x3f800000u;
    *reinterpret_cast<uint64_t*>(paramA + 0x88) = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4923e10ULL));
    *reinterpret_cast<uint32_t*>(paramA + 0x90) = 0xc2c80000u;
    *reinterpret_cast<uint64_t*>(paramA + 0x94) = *reinterpret_cast<const uint64_t*>(GX_VMA(0x4923e20ULL));
    *reinterpret_cast<uint32_t*>(paramA + 0x9c) = 0x42c80000u;

    _mm_store_ps(
        reinterpret_cast<float*>(paramA + 0x50),
        _mm_load_ps(reinterpret_cast<const float*>(GX_VMA(0x4738020ULL))));

    uint8_t flag30 = desc[0x30];
    paramA[0xa1] = flag30;
    paramA[0xa0] = flag30;

    _mm_store_ps(
        reinterpret_cast<float*>(paramA + 0xe0),
        _mm_load_ps(reinterpret_cast<const float*>(desc + 0x10)));

    __m128 row20 = _mm_load_ps(reinterpret_cast<const float*>(desc + 0x20));
    _mm_store_ps(reinterpret_cast<float*>(paramA + 0x00), row20);
    _mm_store_ps(reinterpret_cast<float*>(paramA + 0x10), row20);
    _mm_store_ps(reinterpret_cast<float*>(paramA + 0x20), row20);
    _mm_store_ps(reinterpret_cast<float*>(paramA + 0x30), row20);

    uint8_t* pGlobalMgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobalMgr != nullptr) {
        uint8_t* pSub238 = *reinterpret_cast<uint8_t**>(pGlobalMgr + 0x238);
        uint64_t val70 = *reinterpret_cast<uint64_t*>(pSub238 + 0x70);
        *reinterpret_cast<uint64_t*>(paramA + 0xb8) = val70;
        *reinterpret_cast<uint64_t*>(paramA + 0xb0) = val70;
        *reinterpret_cast<uint32_t*>(paramA + 0x14c) = 0;
        *reinterpret_cast<uint32_t*>(paramA + 0x154) = 0x3f800000u;
        *reinterpret_cast<uint32_t*>(paramA + 0x150) = 0x3f800000u;
    }

    uint32_t d3c = *reinterpret_cast<const uint32_t*>(desc + 0x3c);
    if (d3c != 0xffffffffu) {
        uint32_t d38 = *reinterpret_cast<const uint32_t*>(desc + 0x38);
        *reinterpret_cast<uint32_t*>(paramA + 0x114) = d3c;
        *reinterpret_cast<uint32_t*>(paramA + 0x110) = d38;
        *reinterpret_cast<uint32_t*>(paramA + 0x10c) = d3c;
        *reinterpret_cast<uint32_t*>(paramA + 0x108) = d38;
        *reinterpret_cast<uint32_t*>(paramA + 0x118) = 0;
    }

    uint32_t d44 = *reinterpret_cast<const uint32_t*>(desc + 0x44);
    if (d44 != 0xffffffffu) {
        uint32_t d40 = *reinterpret_cast<const uint32_t*>(desc + 0x40);
        *reinterpret_cast<uint32_t*>(paramA + 0x12c) = d44;
        *reinterpret_cast<uint32_t*>(paramA + 0x128) = d40;
        *reinterpret_cast<uint32_t*>(paramA + 0x124) = d44;
        *reinterpret_cast<uint32_t*>(paramA + 0x120) = d40;
        *reinterpret_cast<uint32_t*>(paramA + 0x130) = 0;
    }

    uint32_t d4c = *reinterpret_cast<const uint32_t*>(desc + 0x4c);
    if (d4c != 0xffffffffu) {
        uint32_t d48 = *reinterpret_cast<const uint32_t*>(desc + 0x48);
        *reinterpret_cast<uint32_t*>(paramA + 0x144) = d4c;
        *reinterpret_cast<uint32_t*>(paramA + 0x140) = d48;
        *reinterpret_cast<uint32_t*>(paramA + 0x13c) = d4c;
        *reinterpret_cast<uint32_t*>(paramA + 0x138) = d48;
        *reinterpret_cast<uint32_t*>(paramA + 0x148) = 0;
    }

    *reinterpret_cast<uint32_t*>(paramA + 0x14c) = 0;
    *reinterpret_cast<uint32_t*>(paramA + 0x150) = 0;
    _mm_store_ps(reinterpret_cast<float*>(paramA + 0xb0), _mm_setzero_ps());
    *reinterpret_cast<uint32_t*>(paramA + 0x154) = 0x3f800000u;

    alignas(32) uint8_t paramB[0x1b0];
    std::memset(paramB, 0, sizeof(paramB));
    *reinterpret_cast<uint32_t*>(paramB + 0x00) = 0;
    *reinterpret_cast<uint32_t*>(paramB + 0x04) = 0xc7u;
    *reinterpret_cast<uint32_t*>(paramB + 0x160) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(paramB + 0x164) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(paramB + 0x170) = 0xffffffffu;
    *reinterpret_cast<uint64_t*>(paramB + 0x190) = *reinterpret_cast<const uint64_t*>(desc + 0x98);

    std::memcpy(paramB + 0x10, paramA, 0x160);

    alignas(32) uint8_t drawCtx[0x3d0];
    std::memset(drawCtx, 0, sizeof(drawCtx));
    sub_224c940(drawCtx, renderCtx + 0x720, renderCtx + 0x440, paramA);

    alignas(16) uint8_t smallDesc[0x20];
    std::memset(smallDesc, 0, sizeof(smallDesc));
    *reinterpret_cast<uint64_t*>(smallDesc + 0x08) = *reinterpret_cast<const uint64_t*>(desc + 0x98);
    smallDesc[0x10] = 1;

    if (desc[0x01] != 0) {
        _mm_store_ps(
            reinterpret_cast<float*>(drawCtx + 0x380),
            _mm_load_ps(reinterpret_cast<const float*>(desc + 0x50)));
        _mm_store_ps(
            reinterpret_cast<float*>(drawCtx + 0x390),
            _mm_load_ps(reinterpret_cast<const float*>(desc + 0x60)));
        _mm_store_ps(
            reinterpret_cast<float*>(drawCtx + 0x3a0),
            _mm_load_ps(reinterpret_cast<const float*>(desc + 0x70)));
        _mm_store_ps(
            reinterpret_cast<float*>(drawCtx + 0x3b0),
            _mm_load_ps(reinterpret_cast<const float*>(desc + 0x80)));
        *reinterpret_cast<uint32_t*>(drawCtx + 0x3c0) = *reinterpret_cast<const uint32_t*>(desc + 0x90);
        drawCtx[0x3c4] = 1;
    }

    sub_2250f70(renderCtx, drawCtx, smallDesc, paramB);
    drawCtx[0x174] = desc[0x00];
    sub_224d020(drawCtx);
    sub_230c170(*reinterpret_cast<void**>(self + 0x28), drawCtx, 0);
    sub_e588a0(drawCtx);
}

// ============================================================================
// [41/66] sub_230a590 @ 0x230a590 (13 B)
// Forwards to sub28->vtable[2] (+0x10)
// ============================================================================
uint64_t sub_230a590(void* pOuter, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    void* pSub28 = *reinterpret_cast<void**>(static_cast<uint8_t*>(pOuter) + 0x28);
    void* vtbl = *reinterpret_cast<void**>(pSub28);
    return (*reinterpret_cast<uint64_t (**)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x10))(pSub28, a2, a3, a4, a5, a6);
}

// ============================================================================
// [42/66] sub_230a4b0 @ 0x230a4b0 (9 B)
// Thunk forwarding to sub_230ba00(*qword(pOuter + 0x28), index, pMatrix3x4)
// ============================================================================
void sub_230a4b0(void* pOuter, uint32_t index, const float* pMatrix3x4) {
    sub_230ba00(*reinterpret_cast<void**>(static_cast<uint8_t*>(pOuter) + 0x28), index, pMatrix3x4);
}

// ============================================================================
// [43/66] sub_230a440 @ 0x230a440 (102 B)
// Releases sub-objects at +0x28 and +0x20 via allocator at +0x08
// ============================================================================
void sub_230a440(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    void* pAlloc = *reinterpret_cast<void**>(self + 0x08);

    void* pSub28 = *reinterpret_cast<void**>(self + 0x28);
    if (pSub28 != nullptr) {
        void* vtbl28 = *reinterpret_cast<void**>(pSub28);
        (*reinterpret_cast<void (**)(void*)>(vtbl28))(pSub28);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pSub28);
        *reinterpret_cast<void**>(self + 0x28) = nullptr;
    }

    void* pSub20 = *reinterpret_cast<void**>(self + 0x20);
    if (pSub20 != nullptr) {
        void* vtbl20 = *reinterpret_cast<void**>(pSub20);
        (*reinterpret_cast<void (**)(void*)>(vtbl20))(pSub20);
        void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pSub20);
        *reinterpret_cast<void**>(self + 0x20) = nullptr;
    }
}

// ============================================================================
// [44/66] sub_230a360 @ 0x230a360 (222 B)
// Initializes particle resource wrapper (+0x20) and instance sub-controller (+0x28)
// ============================================================================
bool sub_230a360(void* pSelf, void* pContext, void* pResourceArg) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<void**>(self + 0x10) = pContext;
    *reinterpret_cast<void**>(self + 0x18) = pResourceArg;

    void* pAlloc = *reinterpret_cast<void**>(static_cast<uint8_t*>(pContext) + 0x10);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    void* pObj20 = (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
        pAlloc, 0x80, 8);

    if (pObj20 != nullptr) {
        void* pCtx10 = *reinterpret_cast<void**>(self + 0x10);
        sub_f24980(pObj20, *reinterpret_cast<void**>(static_cast<uint8_t*>(pCtx10) + 0x10));
        *reinterpret_cast<void**>(self + 0x20) = pObj20;
        if (!sub_f24bc0(pObj20, reinterpret_cast<const char16_t*>(GX_VMA(0x49d432cULL)), pResourceArg, 0)) {
            sub_ee5de0(6);
            void* selfVtbl = *reinterpret_cast<void**>(self);
            (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(selfVtbl) + 0x18))(self);
            return false;
        }
    } else {
        *reinterpret_cast<void**>(self + 0x20) = nullptr;
    }

    void* pCtx10 = *reinterpret_cast<void**>(self + 0x10);
    pAlloc = *reinterpret_cast<void**>(static_cast<uint8_t*>(pCtx10) + 0x10);
    allocVtbl = *reinterpret_cast<void**>(pAlloc);
    void* pObj28 = (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
        pAlloc, 0xc0, 0x10);

    if (pObj28 != nullptr) {
        pCtx10 = *reinterpret_cast<void**>(self + 0x10);
        sub_230ae70(pObj28, *reinterpret_cast<void**>(static_cast<uint8_t*>(pCtx10) + 0x10));
        *reinterpret_cast<void**>(self + 0x28) = pObj28;
        if (!sub_230b2b0(pObj28, *reinterpret_cast<void**>(self + 0x20))) {
            sub_ee5de0(6);
            void* selfVtbl = *reinterpret_cast<void**>(self);
            (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(selfVtbl) + 0x18))(self);
            return false;
        }
    } else {
        *reinterpret_cast<void**>(self + 0x28) = nullptr;
    }
    return true;
}

// ============================================================================
// [45/66] sub_230a2f0 @ 0x230a2f0 (112 B)
// Non-deleting destructor for outer particle controller (vtable 0x5362560)
// ============================================================================
void sub_230a2f0(void* pSelf) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x5362560ULL);
    sub_230a440(self);
}

// ============================================================================
// [46/66] sub_230a280 @ 0x230a280 (111 B)
// Deleting destructor for outer particle controller (vtable 0x5362560)
// ============================================================================
void sub_230a280(void* pSelf) {
    sub_230a2f0(pSelf);
    sub_2bbe588(pSelf);
}

// ============================================================================
// [47/66] sub_230a250 @ 0x230a250 (47 B)
// Constructor for outer particle controller (vtable 0x5362560)
// ============================================================================
void sub_230a250(void* pSelf, void* pAllocator) {
    uint8_t* self = static_cast<uint8_t*>(pSelf);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x5362560ULL);
    *reinterpret_cast<void**>(self + 0x08) = pAllocator;
    *reinterpret_cast<uint64_t*>(self + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
}

// ============================================================================
// [48/66] sub_2309e80 @ 0x2309e80 (976 B)
// Hash table splice/insert + load-factor check & rehash (64-bit key at +0x10)
// ============================================================================
FfxInsertResult32* sub_2309e80(
    FfxInsertResult32* pOut,
    void* pTable,
    uint8_t* pWhere,
    uint64_t bucket,
    FfxIter24 plistIter)
{
    uint8_t* tblBytes = static_cast<uint8_t*>(pTable);
    uint8_t* pNode = plistIter.node;
    uint8_t* pNodeNext = *reinterpret_cast<uint8_t**>(pNode + 0x00);

    if (pNodeNext != pWhere) {
        uint8_t* pNodePrev = *reinterpret_cast<uint8_t**>(pNode + 0x08);
        *reinterpret_cast<uint8_t**>(pNodePrev + 0x00) = pNodeNext;

        uint8_t* pNextPrev = *reinterpret_cast<uint8_t**>(pNodeNext + 0x08);
        *reinterpret_cast<uint8_t**>(pNextPrev + 0x00) = pWhere;

        uint8_t* pWherePrev = *reinterpret_cast<uint8_t**>(pWhere + 0x08);
        *reinterpret_cast<uint8_t**>(pWherePrev + 0x00) = pNode;

        pWherePrev = *reinterpret_cast<uint8_t**>(pWhere + 0x08);
        *reinterpret_cast<uint8_t**>(pWhere + 0x08) = *reinterpret_cast<uint8_t**>(pNodeNext + 0x08);
        *reinterpret_cast<uint8_t**>(pNodeNext + 0x08) = *reinterpret_cast<uint8_t**>(pNode + 0x08);
        *reinterpret_cast<uint8_t**>(pNode + 0x08) = pWherePrev;
    }

    FfxIter24* pVecBegin = *reinterpret_cast<FfxIter24**>(tblBytes + 0x28);
    FfxIter24* pSlot0 = &pVecBegin[bucket * 2];
    uint8_t* pSlot0Node = pSlot0->node;

    if (pSlot0Node == *reinterpret_cast<uint8_t**>(tblBytes + 0x08)) {
        pSlot0->proxy0 = plistIter.proxy0;
        pSlot0->proxy1 = plistIter.proxy1;
        pSlot0->node = pNode;
        pVecBegin = *reinterpret_cast<FfxIter24**>(tblBytes + 0x28);
        FfxIter24* pSlot1 = &pVecBegin[bucket * 2 + 1];
        pSlot1->proxy0 = plistIter.proxy0;
        pSlot1->proxy1 = plistIter.proxy1;
        pSlot1->node = pNode;
    } else if (pSlot0Node == pWhere) {
        pSlot0->proxy0 = plistIter.proxy0;
        pSlot0->proxy1 = plistIter.proxy1;
        pSlot0->node = pNode;
    } else {
        FfxIter24* pSlot1 = &pVecBegin[bucket * 2 + 1];
        uint8_t* pNext = *reinterpret_cast<uint8_t**>(pSlot1->node + 0x00);
        pSlot1->node = pNext;
        if (pNext != pNode) {
            pSlot1->node = *reinterpret_cast<uint8_t**>(pNext + 0x08);
        }
    }

    uint64_t elemCount = *reinterpret_cast<uint64_t*>(tblBytes + 0x10);
    uint64_t maxBuckets = *reinterpret_cast<uint64_t*>(tblBytes + 0x50);
    float fCount = (static_cast<int64_t>(elemCount) < 0)
        ? (static_cast<float>((elemCount >> 1) | (elemCount & 1)) * 2.0f)
        : static_cast<float>(elemCount);
    float fBuckets = (static_cast<int64_t>(maxBuckets) < 0)
        ? (static_cast<float>((maxBuckets >> 1) | (maxBuckets & 1)) * 2.0f)
        : static_cast<float>(maxBuckets);

    if ((fCount / fBuckets) > *reinterpret_cast<float*>(tblBytes + 0x58)) {
        if (maxBuckets <= 0x1ffULL) {
            maxBuckets <<= 3;
        } else if (maxBuckets < 0x555555555555555ULL) {
            maxBuckets <<= 1;
        }

        FfxIter24 valIter = {0, 0, *reinterpret_cast<uint8_t**>(tblBytes + 0x08)};
        uint8_t* pVecRaw = *reinterpret_cast<uint8_t**>(tblBytes + 0x28);
        *reinterpret_cast<uint8_t**>(tblBytes + 0x30) = pVecRaw;
        FfxIter24 whereIter = {0, 0, pVecRaw};
        FfxIter24 outIter = {};
        sub_2309830(&outIter, tblBytes + 0x20, maxBuckets * 2, &valIter, whereIter);

        *reinterpret_cast<uint64_t*>(tblBytes + 0x48) = maxBuckets - 1;
        *reinterpret_cast<uint64_t*>(tblBytes + 0x50) = maxBuckets;

        uint8_t* pHead = *reinterpret_cast<uint8_t**>(tblBytes + 0x08);
        uint8_t* pIter = *reinterpret_cast<uint8_t**>(pHead + 0x00);
        if (pIter != pHead) {
            uint8_t* pLastPreRehash = *reinterpret_cast<uint8_t**>(pHead + 0x08);
            for (;;) {
                uint64_t k = *reinterpret_cast<uint64_t*>(pIter + 0x10) & 0x7fffffffffffffffULL;
                LDivResult dv = sub_2bbfd68(static_cast<int64_t>(k ^ 0xdeadbeefULL), 0x1f31d);
                int64_t h = dv.rem * 0x41a7 + dv.quot * -2836;
                uint64_t hVal = (h < 0) ? static_cast<uint64_t>(h + 0x7fffffffffffffffLL) : static_cast<uint64_t>(h);

                uint64_t m = *reinterpret_cast<uint64_t*>(tblBytes + 0x48);
                uint64_t b = hVal & m;
                if (*reinterpret_cast<uint64_t*>(tblBytes + 0x50) <= b) {
                    b += ~(m >> 1);
                }

                uint8_t* curHead = *reinterpret_cast<uint8_t**>(tblBytes + 0x08);
                FfxIter24* vec = *reinterpret_cast<FfxIter24**>(tblBytes + 0x28);
                uint8_t* bFirst = vec[b * 2].node;
                uint8_t* bPos = curHead;
                bool duplicateKey = false;

                if (bFirst != curHead) {
                    bPos = *reinterpret_cast<uint8_t**>(vec[b * 2 + 1].node + 0x00);
                    while (bPos != bFirst) {
                        uint8_t* pPrev = *reinterpret_cast<uint8_t**>(bPos + 0x08);
                        uint64_t iterKey = *reinterpret_cast<uint64_t*>(pIter + 0x10);
                        uint64_t prevKey = *reinterpret_cast<uint64_t*>(pPrev + 0x10);
                        if (iterKey < prevKey) {
                            bPos = pPrev;
                        } else if (prevKey >= iterKey) {
                            duplicateKey = true;
                            break;
                        } else {
                            bPos = *reinterpret_cast<uint8_t**>(pPrev + 0x00);
                            break;
                        }
                    }
                    if (!duplicateKey && bPos == bFirst) {
                        bPos = bFirst;
                    }
                } else {
                    bPos = bFirst;
                }

                if (!duplicateKey) {
                    FfxIter24 subIter = {0, 0, pIter};
                    FfxInsertResult32 tmpRes = {};
                    sub_2309e80(&tmpRes, tblBytes, bPos, b, subIter);
                } else if (pIter != curHead) {
                    uint8_t* pNextNode = *reinterpret_cast<uint8_t**>(pIter + 0x00);
                    uint8_t* pPrevNode = *reinterpret_cast<uint8_t**>(pIter + 0x08);
                    *reinterpret_cast<uint8_t**>(pPrevNode + 0x00) = pNextNode;
                    *reinterpret_cast<uint8_t**>(pNextNode + 0x08) = pPrevNode;

                    void* pAlloc = *reinterpret_cast<void**>(tblBytes + 0x18);
                    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                    (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pIter);
                    --(*reinterpret_cast<uint64_t*>(tblBytes + 0x10));
                }

                if (pIter == pLastPreRehash) {
                    break;
                }
                pHead = *reinterpret_cast<uint8_t**>(tblBytes + 0x08);
                pIter = *reinterpret_cast<uint8_t**>(pHead + 0x00);
            }
        }
    }

    pOut->proxy0 = 0;
    pOut->proxy1 = 0;
    pOut->node = pNode;
    pOut->inserted = 1;
    return pOut;
}

// ============================================================================
// [49/66] sub_2309d80 @ 0x2309d80 (251 B)
// vector<uint64_t>::_Reallocate(pVec, newCap)
// ============================================================================
void sub_2309d80(void* pVec, uint64_t newCap) {
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    void* pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    uint64_t* pNewBuf = static_cast<uint64_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAlloc, newCap * sizeof(uint64_t), 8));
    if (pNewBuf == nullptr) {
        handleContainerAllocFailure();
    }

    uint64_t* pFirst = *reinterpret_cast<uint64_t**>(vecBytes + 0x08);
    uint64_t* pLast = *reinterpret_cast<uint64_t**>(vecBytes + 0x10);
    if (pFirst != pLast) {
        uint64_t* pDst = pNewBuf;
        for (uint64_t* pSrc = pFirst; pSrc != pLast; ++pSrc, ++pDst) {
            if (pDst != nullptr) {
                *pDst = *pSrc;
            }
        }
        pFirst = *reinterpret_cast<uint64_t**>(vecBytes + 0x08);
        pLast = *reinterpret_cast<uint64_t**>(vecBytes + 0x10);
    }

    uint64_t curLen = static_cast<uint64_t>(pLast - pFirst);
    if (pFirst != nullptr) {
        pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
        allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pFirst);
    }

    *reinterpret_cast<uint64_t**>(vecBytes + 0x18) = pNewBuf + newCap;
    *reinterpret_cast<uint64_t**>(vecBytes + 0x10) = pNewBuf + curLen;
    *reinterpret_cast<uint64_t**>(vecBytes + 0x08) = pNewBuf;
}

// ============================================================================
// [50/66] sub_2309830 @ 0x2309830 (1352 B)
// vector<FfxIter24>::insert(pOutIter, pVec, count, pVal, whereIter)
// ============================================================================
FfxIter24* sub_2309830(
    FfxIter24* pOutIter,
    void* pVec,
    uint64_t count,
    const FfxIter24* pVal,
    FfxIter24 whereIter)
{
    return sub_230d170(pOutIter, pVec, count, pVal, whereIter);
}

// ============================================================================
// [51/66] sub_23096e0 @ 0x23096e0 (325 B)
// vector<16-byte>::_Reserve_1(pVec)
// ============================================================================
void sub_23096e0(void* pVec) {
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    uint8_t* pEnd = *reinterpret_cast<uint8_t**>(vecBytes + 0x18);
    uint8_t* pLast = *reinterpret_cast<uint8_t**>(vecBytes + 0x10);
    if (pEnd != pLast) {
        return;
    }

    uint8_t* pFirst = *reinterpret_cast<uint8_t**>(vecBytes + 0x08);
    uint64_t curCap = static_cast<uint64_t>(pEnd - pFirst) >> 4;
    uint64_t halfCap = curCap >> 1;
    uint64_t grownCap = (0xfffffffffffffffULL - halfCap >= curCap) ? (curCap + halfCap) : 0ULL;
    if (grownCap < curCap + 1) {
        grownCap = curCap + 1;
    }

    void* pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    uint64_t byteCap = grownCap << 4;
    uint8_t* pNewBuf = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAlloc, byteCap, 8));
    if (pNewBuf == nullptr) {
        handleContainerAllocFailure();
    }

    pFirst = *reinterpret_cast<uint8_t**>(vecBytes + 0x08);
    pLast = *reinterpret_cast<uint8_t**>(vecBytes + 0x10);
    if (pFirst != pLast) {
        uint8_t* pDst = pNewBuf;
        for (uint8_t* pSrc = pFirst; pSrc != pLast; pSrc += 0x10, pDst += 0x10) {
            if (pDst != nullptr) {
                *reinterpret_cast<uint64_t*>(pDst + 0x00) = *reinterpret_cast<uint64_t*>(pSrc + 0x00);
                *reinterpret_cast<uint64_t*>(pDst + 0x08) = *reinterpret_cast<uint64_t*>(pSrc + 0x08);
            }
        }
        pFirst = *reinterpret_cast<uint8_t**>(vecBytes + 0x08);
        pLast = *reinterpret_cast<uint8_t**>(vecBytes + 0x10);
    }

    uint64_t byteLen = static_cast<uint64_t>(pLast - pFirst) & ~0xfull;
    if (pFirst != nullptr) {
        pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
        allocVtbl = *reinterpret_cast<void**>(pAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pFirst);
    }

    *reinterpret_cast<uint8_t**>(vecBytes + 0x18) = pNewBuf + byteCap;
    *reinterpret_cast<uint8_t**>(vecBytes + 0x10) = pNewBuf + byteLen;
    *reinterpret_cast<uint8_t**>(vecBytes + 0x08) = pNewBuf;
}

// ============================================================================
// [52/66] sub_2309560 @ 0x2309560 (381 B)
// vector<16-byte>::reserve_safe(pVec, newCap) with TLS setjmp guard
// ============================================================================
bool sub_2309560(void* pVec, uint64_t newCap) {
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();

    *reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) = 1;
    void* pJmpBuf = reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL)));

    bool ok = false;
    if (sub_2bbe8f8(pJmpBuf) == 0) {
        uint64_t curCap = static_cast<uint64_t>(
            *reinterpret_cast<uint8_t**>(vecBytes + 0x18) -
            *reinterpret_cast<uint8_t**>(vecBytes + 0x08)) >> 4;
        ok = true;
        if (curCap < newCap) {
            if ((newCap >> 60) != 0) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            void* pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
            void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
            uint64_t byteCap = newCap << 4;
            uint8_t* pNewBuf = static_cast<uint8_t*>(
                (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
                    pAlloc, byteCap, 8));
            if (pNewBuf == nullptr) {
                handleContainerAllocFailure();
            }

            uint8_t* pFirst = *reinterpret_cast<uint8_t**>(vecBytes + 0x08);
            uint8_t* pLast = *reinterpret_cast<uint8_t**>(vecBytes + 0x10);
            if (pFirst != pLast) {
                uint8_t* pDst = pNewBuf;
                for (uint8_t* pSrc = pFirst; pSrc != pLast; pSrc += 0x10, pDst += 0x10) {
                    if (pDst != nullptr) {
                        *reinterpret_cast<uint64_t*>(pDst + 0x00) = *reinterpret_cast<uint64_t*>(pSrc + 0x00);
                        *reinterpret_cast<uint64_t*>(pDst + 0x08) = *reinterpret_cast<uint64_t*>(pSrc + 0x08);
                    }
                }
                pFirst = *reinterpret_cast<uint8_t**>(vecBytes + 0x08);
                pLast = *reinterpret_cast<uint8_t**>(vecBytes + 0x10);
            }

            uint64_t byteLen = static_cast<uint64_t>(pLast - pFirst) & ~0xfull;
            if (pFirst != nullptr) {
                pAlloc = *reinterpret_cast<void**>(vecBytes + 0x20);
                allocVtbl = *reinterpret_cast<void**>(pAlloc);
                (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(pAlloc, pFirst);
            }

            *reinterpret_cast<uint8_t**>(vecBytes + 0x18) = pNewBuf + byteCap;
            *reinterpret_cast<uint8_t**>(vecBytes + 0x10) = pNewBuf + byteLen;
            *reinterpret_cast<uint8_t**>(vecBytes + 0x08) = pNewBuf;
        }
    }

    fsBase = gx_guest_tls_base();
    *reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) = 0;
    return ok;
}

// ============================================================================
// [53/66] sub_23094c0 @ 0x23094c0 (159 B)
// vector<uint64_t>::reserve_safe(pVec, newCap) with TLS setjmp guard
// ============================================================================
bool sub_23094c0(void* pVec, uint64_t newCap) {
    uint8_t* vecBytes = static_cast<uint8_t*>(pVec);
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();

    *reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) = 1;
    void* pJmpBuf = reinterpret_cast<void*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL)));

    bool ok = false;
    if (sub_2bbe8f8(pJmpBuf) == 0) {
        uint64_t curCap = static_cast<uint64_t>(
            *reinterpret_cast<uint8_t**>(vecBytes + 0x18) -
            *reinterpret_cast<uint8_t**>(vecBytes + 0x08)) >> 3;
        ok = true;
        if (curCap < newCap) {
            if ((newCap >> 61) != 0) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            sub_2309d80(vecBytes, newCap);
        }
    }

    fsBase = gx_guest_tls_base();
    *reinterpret_cast<uint32_t*>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL))) = 0;
    return ok;
}

// ============================================================================
// [54/66] sub_2309350 @ 0x2309350 (366 B)
// Hash table constructor (0x28-byte nodes, 8 initial buckets)
// ============================================================================
void sub_2309350(void* pTable, void** ppAllocator) {
    uint8_t* tblBytes = static_cast<uint8_t*>(pTable);
    *reinterpret_cast<uint64_t*>(tblBytes + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(tblBytes + 0x08) = 0;

    void* pAlloc = *ppAllocator;
    *reinterpret_cast<void**>(tblBytes + 0x18) = pAlloc;
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    uint8_t* pHead = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, size_t, size_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(
            pAlloc, 0x28, 8));
    if (pHead != nullptr) {
        *reinterpret_cast<void**>(pHead + 0x00) = pHead;
    } else {
        handleContainerAllocFailure();
        return;
    }
    *reinterpret_cast<void**>(pHead + 0x08) = pHead;
    *reinterpret_cast<void**>(tblBytes + 0x08) = pHead;

    void* pVecAlloc = *ppAllocator;
    *reinterpret_cast<uint64_t*>(tblBytes + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(tblBytes + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(tblBytes + 0x28) = 0;
    *reinterpret_cast<void**>(tblBytes + 0x40) = pVecAlloc;
    *reinterpret_cast<uint32_t*>(tblBytes + 0x58) = 0x3f800000u;

    FfxIter24 valIter = {0, 0, pHead};
    *reinterpret_cast<uint64_t*>(tblBytes + 0x30) = 0;
    FfxIter24 whereIter = {0, 0, nullptr};
    FfxIter24 outIter = {};
    sub_2309830(&outIter, tblBytes + 0x20, 0x10, &valIter, whereIter);

    *reinterpret_cast<uint64_t*>(tblBytes + 0x48) = 7;
    *reinterpret_cast<uint64_t*>(tblBytes + 0x50) = 8;
}

// ============================================================================
// [55/66] sub_2309340 @ 0x2309340 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_2309340(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [56/66] sub_2309330 @ 0x2309330 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_2309330(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [57/66] sub_2309320 @ 0x2309320 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_2309320(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [58/66] sub_2309310 @ 0x2309310 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_2309310(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [59/66] sub_2309300 @ 0x2309300 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_2309300(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [60/66] sub_23092f0 @ 0x23092f0 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_23092f0(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [61/66] sub_23092e0 @ 0x23092e0 (9 B)
// Virtual thunk to vtable[5] (+0x28)
// ============================================================================
void* sub_23092e0(void* pSelf, void* a1, void* a2, void* a3, void* a4, void* a5) {
    void* vtbl = *reinterpret_cast<void**>(pSelf);
    return (*reinterpret_cast<void* (**)(void*, void*, void*, void*, void*, void*)>(
        static_cast<uint8_t*>(vtbl) + 0x28))(pSelf, a1, a2, a3, a4, a5);
}

// ============================================================================
// [62/66] sub_23092d0 @ 0x23092d0 (3 B)
// Virtual predicate returning true (1)
// ============================================================================
bool sub_23092d0() {
    return true;
}

// ============================================================================
// [63/66] sub_23091f0 @ 0x23091f0 (3 B)
// Virtual getter returning 0
// ============================================================================
uint32_t sub_23091f0() {
    return 0;
}

// ============================================================================
// [64/66] sub_23091e0 @ 0x23091e0 (3 B)
// Virtual predicate returning true (1)
// ============================================================================
bool sub_23091e0() {
    return true;
}

// ============================================================================
// [65/66] sub_23091d0 @ 0x23091d0 (3 B)
// Virtual predicate returning true (1)
// ============================================================================
bool sub_23091d0() {
    return true;
}

// ============================================================================
// [66/66] sub_23091c0 @ 0x23091c0 (1 B)
// Empty virtual no-op
// ============================================================================
void sub_23091c0() {
}

} // extern "C"
