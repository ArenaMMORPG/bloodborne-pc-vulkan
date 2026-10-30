#include <cstdint>
#include <cstring>
#include <immintrin.h>

// ============================================================================
// Module 05: Yebis 2 PostFX Middleware Core (05_Yebis2_PostFX)
// File: GXYebisPostEffectContextSettersAndInit.cpp
// Range: 0x0bb4000 .. 0x0bb7500 (191 functions, 12,610 bytes)
// ============================================================================
// Architectural Overview:
//   Implements the upper half of Silicon Studio's Yebis 2 (PPFX) public API
//   and Orbis GNM platform context shim (`ppfx.cpp`, `ppfxInterface.cpp`,
//   `ppfxMemoryAllocator.cpp`, `ppfxRenderToTexture.cpp`, and
//   `ppfxPlatformContextGnmShim.h`):
//   - `IPfxBaseRenderToTexture` lifecycle & initialization (`sub_bb6f90` .. `sub_bb71e0`)
//   - `IPfxMemoryAllocator` & `SetMemoryAllocator` (`sub_bb6e60` .. `sub_bb6f80`)
//   - `IPfxInterface` refcount & base interface (`sub_bb6c70` .. `sub_bb6e50`)
//   - Global `ppfxInitialize`, `ppfxUninitialize`, `ppfxBindFramebuffer`,
//     `ppfxApplyEffects`, camera/frustum jitter & exposure math (`sub_bb5ff0` .. `sub_bb6b90`)
//   - `IPfxContext_Gnm` GNM texture/render-target registry search & source
//     setters (`SetEffectMaskAlphaSource`, `SetEffectMaskColorSource`,
//     `SetGlareModulatorSource`, `SetDepthOfFieldDepthRemapLutSource`,
//     `SetColorGradingLutSource`, `SetMotionBlurVelocitySource`,
//     `SetTonemapDestination`, `ResolveMsaa`, `SetDepthFactorSource`,
//     `SetEffectSource`, `SetActiveCommandBuffers`, `SetActiveBackBuffer`,
//     `IPfxContext_Gnm::Initialize`, `IPfxBaseContext::UninitializeDeviceResources`,
//     `0xbb4710` .. `0xbb5f10`)
//   - 112 `IPfxBaseContext` -> `s_pSinglePostEffect` (`*(void**)0x5465ed0`)
//     forwarding methods (`0xbb4000` .. `0xbb4700`).
// ============================================================================

namespace {

inline void* g_pSinglePostEffect() {
    return *reinterpret_cast<void**>(GX_VMA(0x5465ed0ULL));
}

inline void set_pSinglePostEffect(void* p) {
    *reinterpret_cast<void**>(GX_VMA(0x5465ed0ULL)) = p;
}

inline void* g_pMemoryAllocator() {
    return *reinterpret_cast<void**>(GX_VMA(0x52b3618ULL));
}

inline int32_t g_logEnabled() {
    return *reinterpret_cast<int32_t*>(GX_VMA(0x5119a68ULL));
}

inline int32_t g_logWarnEnabled() {
    return *reinterpret_cast<int32_t*>(GX_VMA(0x5119a6cULL));
}

inline int32_t g_logErrorEnabled() {
    return *reinterpret_cast<int32_t*>(GX_VMA(0x5119a70ULL));
}

inline int32_t g_logInfoEnabled() {
    return *reinterpret_cast<int32_t*>(GX_VMA(0x5465eb0ULL));
}

inline float bits_to_f32(uint32_t u) {
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

} // namespace

struct PfxTreeIter24 {
    uint64_t w0;
    uint64_t w1;
    void* node;
};

struct GnmResolveTextureSpec48 {
    uint64_t w0;
    uint64_t w1;
    uint64_t w2;
    uint64_t w3;
    uint64_t w4;
    uint32_t w5;
    uint32_t pad;
};

extern "C" {

// External / internal declarations
void sub_bb18b0(void* this_ctx);
void sub_bb1a90(const char* fmt, ...);
int32_t sub_bb1c20(void* this_ctx, void* initDesc, uint32_t a3, uint32_t a4, uint32_t a5, void* a6);
void sub_bb2310(void* this_ctx);
void sub_bb6d20(void* this_if);
void sub_bb6da0(void* this_if);
void* sub_bb6e40(void* this_if);
void* sub_bb6f20(void* pAlloc, uint64_t size, const char* file, int32_t line, const char* func);
void sub_bb6f50(void* pAlloc, void* ptr);
void sub_bb6fe0(void* this_rtt);
void sub_bb48d0(void* this_ctx);
uint8_t sub_bb6740();

void sub_bb9250(void* outMat, const void* inMat, uint32_t bTranspose, __m128 v0, __m128 v1, __m128 v2, __m128 v3);
uint64_t sub_bb9360(void* a1, uint32_t a2, float f0, float f1, float f2, float f3);
uint64_t sub_bb9510(void* a1, uint32_t a2, float f0, float f1, float f2, float f3);

uint64_t sub_bc69f0(void* a1, uint32_t a2, void* a3);
void sub_bc6a20(void* pPostEffect);
void sub_bc70c0(void* pPostEffect);
int32_t sub_bc7290(void* pPostEffect, uint32_t flags);
uint64_t sub_bc9430(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_bc94b0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_bc9530(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
int32_t sub_bc98b0(void* pPostEffect, void* a2, uint32_t a3, uint32_t a4, uint32_t a5, void* a6);

uint64_t sub_bd27a0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd27f0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
float sub_bd2880(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd46b0(void* pPostEffect, uint32_t a2);
uint64_t sub_bd46f0(void* pPostEffect, uint32_t a2, float f0);
uint64_t sub_bd4720(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd47d0(void* pPostEffect, uint32_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd4800(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd4840(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
void sub_bd48d0(void* pPostEffect, float f0, float f1, float f2, float f3, float f4);
void sub_bd4b60(void* pPostEffect, uint32_t a2, float f0, float f1, float f2);
uint64_t sub_bd4c20(void* pPostEffect, uint32_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd4c40(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd4c50(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
void sub_bd4d40(void* pPostEffect, int32_t a2, int32_t a3, float f0, float f1, float f2, float f3);
void sub_bd4f70(void* pPostEffect, float f0, float f1);
uint64_t sub_bd63d0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd6850(void* pPostEffect, uint32_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd6cb0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
int32_t sub_bd7bf0(void* pPostEffect);
uint64_t sub_bd7c10(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
int32_t sub_bd82f0(void* pPostEffect, uint32_t a2, void* a3, uint32_t a4, int32_t a5);
uint64_t sub_bd99a0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bd99b0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
float sub_bd99d0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
int32_t sub_bda460(void* pPostEffect, uint32_t a2, void* a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, void* a8, uint32_t a9);
void sub_bdc470(void* pPostEffect, uint32_t a2, float f0, float f1, float f2, float f3, float f4);
uint64_t sub_bdc710(void* pPostEffect, uint32_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bdc740(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bdc7a0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bdc800(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);

int32_t sub_be95e0(void* pPostEffect, uint32_t a2, uint32_t a3);
int32_t sub_befb50(void* pPostEffect, uint32_t a2);
int32_t sub_bf05f0(void* pPostEffect, void* pRtObj);
int32_t sub_bf0790(void* pPostEffect, void* pTexObj);
uint64_t sub_bf3070(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bf30a0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
int32_t sub_bf30d0(void* pPostEffect, void* a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6);
int32_t sub_bf3280(void* pPostEffect, void* a2, uint32_t a3, uint32_t a4);
int32_t sub_bf3430(void* pPostEffect, uint32_t a2, void* a3, uint32_t a4, uint32_t a5, uint32_t a6);
int32_t sub_bf3610(void* pPostEffect, uint32_t a2, void* a3, uint32_t a4, uint32_t a5, uint32_t a6);
int32_t sub_bf37f0(void* pPostEffect, uint32_t a2, void* a3, uint32_t a4, uint32_t a5, uint32_t a6);
int32_t sub_bf39c0(void* pPostEffect, void* a2, uint32_t a3, uint32_t a4, uint32_t a5);
int32_t sub_bf3b70(void* pPostEffect, void* a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7);

void sub_c09d10(void* pRtt, const char* name);
int32_t sub_c0a090(void* pRtt, const void* pDesc);
void sub_c0c530(void* pRtt, uint32_t flag);
const char* sub_c4ede0(int32_t hr);
void sub_c4f5d0(uint32_t fmt, uint32_t msaa, uint32_t* outFmt0, uint32_t* outFmt1);

uint32_t sub_1086540(void* pGnmRt, uint32_t fmt);
uint32_t sub_1086cf0(void* pGnmRt);
uint32_t sub_10883f0(void* pGnmTex, uint32_t fmt);
uint32_t sub_1089960(void* pGnmTex);
int32_t sub_11d6680(uint32_t fmt, uint32_t* outPfxFmt);
void sub_11d6860(void* pConstBuf);
void sub_11d7030(void* pDrawCmdBuf);
void sub_11d7140(void* pDrawCmdBuf, GnmResolveTextureSpec48 srcSpec, GnmResolveTextureSpec48 dstSpec);
void sub_11d7190(void* pDrawCmdBuf);
void sub_11d71a0(void* pDrawCmdBuf, void* pBuf0, void* pBuf1);
void sub_11d71d0(void* a1, void* a2);
void sub_11d71e0(void* pDrawCmdBuf);

int32_t sub_2610000(void* pShim, void* a2, uint32_t a3, void* a4, void* a5, uint32_t a6);
void sub_2610250(void* pShim);
void sub_2610300(void* pShim);
int32_t sub_2610430(void* pShim, void* pGnmTex, void* outPair);
int32_t sub_26105f0(void* pShim, void* pGnmRt, void* outPair);
int32_t sub_2610760(void* pShim, void* pGnmRt);
int32_t sub_2610840(void* pShim, void* pGnmTex);
void sub_2610920(float* outAperture, float* outTime, const float* f0, int32_t mode, const float* f1, const float* f2, const float* f3, const float* f4, const float* f5, const float* f6, const float* f7);
__m128 sub_2610c30(uint32_t sampleIdx);
void sub_2611020(void* outIter, void* pTree, PfxTreeIter24 first, PfxTreeIter24 last);
void sub_2611880(void* outIter, void* pTree, PfxTreeIter24 first, PfxTreeIter24 last);
void sub_26128b0(void* pShim);

void sub_2bbe588(void* ptr);
int32_t sub_2bbe618(const char* fmt, int32_t line, const char* msg, void* ap);
float sub_2bbe648(float x);
float sub_2bbe948(float x, float y);
void sub_2bbeae8(void* ptr);
void sub_2bbeaf8(void* ptr);
float sub_2bbf128(float x);
void sub_2bbf178(const char* expr, const char* func);

// 0xbb71e0 | 945 B | IPfxBaseRenderToTexture::Initialize
int32_t sub_bb71e0(
    void* this_rtt,
    void* pContext,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    uint32_t mipLevels,
    uint32_t format,
    uint32_t msaaSamples,
    uint32_t flags) {
    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fed3aULL)), 0x87);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47ff05eULL)));
    }
    if (!pContext) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47ff08fULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47ff0f4ULL)));
        return static_cast<int32_t>(0x80000003u);
    }

    void** ppInternal = reinterpret_cast<void**>(sub_bb6e40(this_rtt));
    if (*ppInternal != nullptr) {
        void** ppCtxInternal = reinterpret_cast<void**>(sub_bb6e40(pContext));
        uint8_t* pCtxObj = reinterpret_cast<uint8_t*>(*ppCtxInternal);
        if (!pCtxObj) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47ff0ffULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47ff0f4ULL)));
        }
        if (pCtxObj[0] != 0) {
            if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
                sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fee42ULL)), 0x9f);
                sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47ff164ULL)));
            }
            if (pCtxObj[0] != 0) {
                sub_2bbf178(
                    reinterpret_cast<const char*>(GX_VMA(0x47ff1fdULL)),
                    reinterpret_cast<const char*>(GX_VMA(0x47ff0f4ULL)));
            }
        }
        uint8_t* pOldRtt = reinterpret_cast<uint8_t*>(*ppInternal);
        if (pOldRtt != nullptr) {
            int32_t oldRef = *reinterpret_cast<int32_t*>(pOldRtt + 0x08);
            int32_t newRef = oldRef - 1;
            *reinterpret_cast<int32_t*>(pOldRtt + 0x08) = newRef;
            if (oldRef <= 0) {
                sub_2bbf178(
                    reinterpret_cast<const char*>(GX_VMA(0x47ff673ULL)),
                    reinterpret_cast<const char*>(GX_VMA(0x47ff6bdULL)));
                newRef = *reinterpret_cast<int32_t*>(pOldRtt + 0x08);
            }
            if (newRef == 0) {
                void* pAlloc = g_pMemoryAllocator();
                uint64_t vtbl = *reinterpret_cast<uint64_t*>(pOldRtt);
                reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x20))(pOldRtt);
                sub_bb6f50(pAlloc, pOldRtt);
            }
            *ppInternal = nullptr;
        }
    }

    const char* kClassName = reinterpret_cast<const char*>(GX_VMA(0x47ff275ULL));
    void* pNewRtt = sub_bb6f20(
        g_pMemoryAllocator(),
        0x118,
        reinterpret_cast<const char*>(GX_VMA(0x47feffcULL)),
        0xaa,
        kClassName);
    sub_c09d10(pNewRtt, kClassName);
    *ppInternal = pNewRtt;
    *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(this_rtt) + 0x18) = pContext;

    uint8_t* pCtxObj = *reinterpret_cast<uint8_t**>(sub_bb6e40(pContext));
    void* pDevice = *reinterpret_cast<void**>(pCtxObj + 0x18);
    if (!pDevice) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47ff28dULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47ff0f4ULL)));
        return static_cast<int32_t>(0x80000003u);
    }

    uint32_t outFmt0 = 0;
    uint32_t outFmt1 = 0;
    sub_c4f5d0(format, msaaSamples, &outFmt0, &outFmt1);

    uint32_t desc[8];
    desc[0] = 0;
    desc[1] = width;
    desc[2] = height;
    desc[3] = mipLevels;
    desc[4] = depth;
    desc[5] = outFmt0;
    desc[6] = outFmt1;
    desc[7] = flags;

    int32_t hr = sub_c0a090(*ppInternal, desc);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fee42ULL)), 0xd2);
            const char* errStr = sub_c4ede0(hr);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47ff2f1ULL)), hr, errStr);
        }
        return hr;
    }

    *reinterpret_cast<uint8_t*>(GX_VMA(0x5119ab8ULL)) = 0;
    void* pRttObj = *ppInternal;
    uint64_t rttVtbl = *reinterpret_cast<uint64_t*>(pRttObj);
    hr = reinterpret_cast<int32_t (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(rttVtbl + 0x00))(pRttObj, pDevice);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fee42ULL)), 0xdf);
            const char* errStr = sub_c4ede0(hr);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47ff349ULL)), hr, errStr);
        }
        uint64_t selfVtbl = *reinterpret_cast<uint64_t*>(this_rtt);
        reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(selfVtbl + 0x38))(this_rtt);
        return hr;
    }

    sub_c0c530(*ppInternal, 0);
    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fed3aULL)), 0xf2);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47ff3a7ULL)));
    }
    return 0;
}

// 0xbb7170 | 104 B | IPfxRenderToTexture::Instantiate
void* sub_bb7170() {
    uint8_t* pObj = reinterpret_cast<uint8_t*>(sub_bb6f20(
        g_pMemoryAllocator(),
        0x50,
        reinterpret_cast<const char*>(GX_VMA(0x47feffcULL)),
        0x72,
        reinterpret_cast<const char*>(GX_VMA(0x47ff03bULL))));
    sub_bb6d20(pObj);
    *reinterpret_cast<uint64_t*>(pObj + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(pObj + 0x00) = GX_VMA(0x52b36c0ULL);
    *reinterpret_cast<uint64_t*>(pObj + 0x20) = 0;
    *reinterpret_cast<void**>(pObj + 0x28) = pObj + 0x30;
    return pObj;
}

// 0xbb6fe0 | 398 B | IPfxBaseRenderToTexture::~IPfxBaseRenderToTexture
void sub_bb6fe0(void* this_rtt) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_rtt);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x52b3670ULL);

    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fed3aULL)), 0x3a);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fed86ULL)));
    }

    void* pContext = *reinterpret_cast<void**>(self + 0x18);
    uint8_t* pCtxObj = nullptr;
    if (pContext != nullptr) {
        pCtxObj = *reinterpret_cast<uint8_t**>(sub_bb6e40(pContext));
    }

    void** ppInternal = reinterpret_cast<void**>(sub_bb6e40(self));
    if (*ppInternal != nullptr) {
        if (!pCtxObj) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fedc5ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fee29ULL)));
        }
        if (pCtxObj[0] != 0) {
            if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
                sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fee42ULL)), 0x45);
                sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fee94ULL)));
            }
            if (pCtxObj[0] != 0) {
                sub_2bbf178(
                    reinterpret_cast<const char*>(GX_VMA(0x47fef3bULL)),
                    reinterpret_cast<const char*>(GX_VMA(0x47fee29ULL)));
            }
        }
        uint8_t* pOldRtt = reinterpret_cast<uint8_t*>(*ppInternal);
        if (pOldRtt != nullptr) {
            int32_t oldRef = *reinterpret_cast<int32_t*>(pOldRtt + 0x08);
            int32_t newRef = oldRef - 1;
            *reinterpret_cast<int32_t*>(pOldRtt + 0x08) = newRef;
            if (oldRef <= 0) {
                sub_2bbf178(
                    reinterpret_cast<const char*>(GX_VMA(0x47ff673ULL)),
                    reinterpret_cast<const char*>(GX_VMA(0x47ff6bdULL)));
                newRef = *reinterpret_cast<int32_t*>(pOldRtt + 0x08);
            }
            if (newRef == 0) {
                void* pAlloc = g_pMemoryAllocator();
                uint64_t vtbl = *reinterpret_cast<uint64_t*>(pOldRtt);
                reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x20))(pOldRtt);
                sub_bb6f50(pAlloc, pOldRtt);
            }
            *ppInternal = nullptr;
        }
    }

    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fed3aULL)), 0x69);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fefb2ULL)));
    }
    sub_bb6da0(self);
}

// 0xbb6fc0 | 28 B | IPfxBaseRenderToTexture deleting destructor
void sub_bb6fc0(void* this_rtt) {
    sub_bb6fe0(this_rtt);
    sub_2bbe588(this_rtt);
}

// 0xbb6f90 | 43 B | IPfxBaseRenderToTexture::IPfxBaseRenderToTexture
void sub_bb6f90(void* this_rtt) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_rtt);
    sub_bb6d20(self);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x52b3670ULL);
    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
}

// 0xbb6f80 | 1 B | IPfxDefaultMemoryAllocator no-op 1
void sub_bb6f80() {}

// 0xbb6f70 | 1 B | IPfxDefaultMemoryAllocator no-op 2
void sub_bb6f70() {}

// 0xbb6f60 | 8 B | IPfxDefaultMemoryAllocator::Free
void sub_bb6f60(void* /*this_alloc*/, void* ptr) {
    sub_2bbeaf8(ptr);
}

// 0xbb6f50 | 15 B | PfxMemoryFreeWrapper
void sub_bb6f50(void* pAlloc, void* ptr) {
    --(*reinterpret_cast<int32_t*>(GX_VMA(0x5466700ULL)));
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pAlloc);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x20))(pAlloc, ptr);
}

// 0xbb6f40 | 8 B | IPfxDefaultMemoryAllocator::Allocate
void sub_bb6f40(void* /*this_alloc*/, void* sizeOrPtr) {
    sub_2bbeae8(sizeOrPtr);
}

// 0xbb6f20 | 18 B | PfxMemoryAllocateWrapper
void* sub_bb6f20(void* pAlloc, uint64_t size, const char* file, int32_t line, const char* func) {
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pAlloc);
    void* res = reinterpret_cast<void* (*)(void*, uint64_t, const char*, int32_t, const char*)>(
        *reinterpret_cast<uint64_t*>(vtbl + 0x18))(pAlloc, size, file, line, func);
    ++(*reinterpret_cast<int32_t*>(GX_VMA(0x5466700ULL)));
    return res;
}

// 0xbb6f10 | 15 B | IPfxMemoryAllocator::IPfxMemoryAllocator
void sub_bb6f10(void* this_alloc) {
    *reinterpret_cast<uint64_t*>(this_alloc) = GX_VMA(0x52b3630ULL);
}

// 0xbb6f00 | 1 B | IPfxMemoryAllocator::~IPfxMemoryAllocator
void sub_bb6f00() {}

// 0xbb6ef0 | 1 B | IPfxMemoryAllocator virtual no-op
void sub_bb6ef0() {}

// 0xbb6e60 | 139 B | PPFX::SetMemoryAllocator
int32_t sub_bb6e60(void* pCustomAllocator) {
    if (*reinterpret_cast<int32_t*>(GX_VMA(0x5466700ULL)) != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fec19ULL)), 0x62);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fec6bULL)));
            if (*reinterpret_cast<int32_t*>(GX_VMA(0x5466700ULL)) == 0) {
                return static_cast<int32_t>(0x80004005u);
            }
        }
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fecb7ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fed27ULL)));
        return static_cast<int32_t>(0x80004005u);
    }

    void* pDefaultAlloc = reinterpret_cast<void*>(GX_VMA(0x52b3610ULL));
    *reinterpret_cast<void**>(GX_VMA(0x52b3618ULL)) = pCustomAllocator ? pCustomAllocator : pDefaultAlloc;
    return 0;
}

// 0xbb6e50 | 5 B | IPfxInterface::GetInternalPtrConst
void* sub_bb6e50(void* this_if) {
    return reinterpret_cast<uint8_t*>(this_if) + 0x10;
}

// 0xbb6e40 | 5 B | IPfxInterface::GetInternalPtr
void* sub_bb6e40(void* this_if) {
    return reinterpret_cast<uint8_t*>(this_if) + 0x10;
}

// 0xbb6e20 | 17 B | IPfxInterface::IsDeviceInitialized
uint8_t sub_bb6e20(void* this_if) {
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(this_if);
    int32_t res = reinterpret_cast<int32_t (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x28))(this_if);
    return (res > 0) ? 1u : 0u;
}

// 0xbb6e10 | 12 B | IPfxInterface::IsInitialized
uint32_t sub_bb6e10(void* this_if) {
    return (*reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(this_if) + 0x10) != 0) ? 1u : 0u;
}

// 0xbb6e00 | 4 B | IPfxInterface::GetReferenceCount
int32_t sub_bb6e00(void* this_if) {
    return *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(this_if) + 0x08);
}

// 0xbb6da0 | 87 B | IPfxInterface::~IPfxInterface
void sub_bb6da0(void* this_if) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_if);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x52b35e0ULL);
    if (*reinterpret_cast<int32_t*>(self + 0x08) != 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47feb4bULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47febabULL)));
    }
    if (*reinterpret_cast<uint64_t*>(self + 0x10) != 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47febbaULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47febabULL)));
    }
}

// 0xbb6d40 | 88 B | IPfxInterface deleting destructor
void sub_bb6d40(void* this_if) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_if);
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x52b35e0ULL);
    if (*reinterpret_cast<int32_t*>(self + 0x08) != 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47feb4bULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47febabULL)));
    }
    if (*reinterpret_cast<uint64_t*>(self + 0x10) != 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47febbaULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47febabULL)));
    }
    sub_2bbe588(self);
}

// 0xbb6d20 | 30 B | IPfxInterface::IPfxInterface
void sub_bb6d20(void* this_if) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_if);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x52b35e0ULL);
    *reinterpret_cast<int32_t*>(self + 0x08) = 1;
    *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
}

// 0xbb6ce0 | 54 B | IPfxInterface::DestroyInstance
void sub_bb6ce0(void* this_if) {
    if (this_if != nullptr) {
        uint64_t vtbl = *reinterpret_cast<uint64_t*>(this_if);
        reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x18))(this_if);
        sub_bb6f50(g_pMemoryAllocator(), this_if);
    }
}

// 0xbb6c80 | 90 B | IPfxInterface::Release
int32_t sub_bb6c80(void* this_if) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_if);
    int32_t refCount = *reinterpret_cast<int32_t*>(self + 0x08);
    if (refCount == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47feae0ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47feb43ULL)));
        refCount = *reinterpret_cast<int32_t*>(self + 0x08);
    }
    --refCount;
    *reinterpret_cast<int32_t*>(self + 0x08) = refCount;
    if (refCount == 0) {
        if (self != nullptr) {
            uint64_t vtbl = *reinterpret_cast<uint64_t*>(self);
            reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x18))(self);
            sub_bb6f50(g_pMemoryAllocator(), self);
        }
        return 0;
    }
    return refCount;
}

// 0xbb6c70 | 9 B | IPfxInterface::AddRef
int32_t sub_bb6c70(void* this_if) {
    int32_t next = *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(this_if) + 0x08) + 1;
    *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(this_if) + 0x08) = next;
    return next;
}

// 0xbb6b90 | 167 B | ppfxFormatLogVariadic
int32_t sub_bb6b90(const char* fmt, int32_t line, const char* msg, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, msg);
    int32_t res = sub_2bbe618(fmt, line, msg, ap);
    __builtin_va_end(ap);
    return res;
}

// 0xbb6b80 | 15 B | ppfxGetSubContext128
void* sub_bb6b80() {
    uint8_t* p = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    return *reinterpret_cast<void**>(p + 0x128);
}

// 0xbb6b70 | 15 B | ppfxGetSubContext120
void* sub_bb6b70() {
    uint8_t* p = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    return *reinterpret_cast<void**>(p + 0x120);
}

// 0xbb6b60 | 15 B | ppfxGetSubContext118
void* sub_bb6b60() {
    uint8_t* p = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    return *reinterpret_cast<void**>(p + 0x118);
}

// 0xbb6b50 | 8 B | ppfxGetSinglePostEffect
void* sub_bb6b50() {
    return g_pSinglePostEffect();
}

// 0xbb6b00 | 80 B | ppfxAcquireDeviceObjectAddRef
void* sub_bb6b00() {
    uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    void* pDev = *reinterpret_cast<void**>(pPost + 0x18);
    void* pOutObj = nullptr;
    uint64_t devVtbl = *reinterpret_cast<uint64_t*>(pDev);
    reinterpret_cast<void (*)(void*, void**)>(*reinterpret_cast<uint64_t*>(devVtbl + 0x60))(pDev, &pOutObj);
    uint64_t objVtbl = *reinterpret_cast<uint64_t*>(pOutObj);
    reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(objVtbl + 0x08))(pOutObj);
    return pOutObj;
}

// 0xbb6af0 | 12 B | ppfxGetDeviceContextPtr
void* sub_bb6af0() {
    uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    return *reinterpret_cast<void**>(pPost + 0x18);
}

// 0xbb6ad0 | 30 B | ppfxSetDefaultVignetteParams
uint8_t sub_bb6ad0(float f0, float f1) {
    sub_bd4b60(g_pSinglePostEffect(), 0, f0, f1, 1.0f);
    return 1;
}

// 0xbb6a70 | 95 B | ppfxSetDepthOfFieldAndFocusParams
uint8_t sub_bb6a70(
    uint8_t bUseCustomFocalLength,
    int32_t a2,
    uint8_t bEnableBokehScale,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5) {
    if (!bUseCustomFocalLength) {
        f1 = 1.0f;
    }
    float ratio = f2 / f3;
    float bokehScale = bEnableBokehScale ? bits_to_f32(0x3b808081u) : 0.0f;
    sub_bd4d40(g_pSinglePostEffect(), a2, static_cast<int32_t>(bEnableBokehScale), f0, f1, ratio, bokehScale);
    sub_bd4f70(g_pSinglePostEffect(), f4, f5);
    return 1;
}

// 0xbb6a40 | 42 B | ppfxSetGlareAnamorphicParams
uint8_t sub_bb6a40(float f0, float f1, float f2) {
    sub_bdc470(
        g_pSinglePostEffect(),
        0,
        f0,
        f1,
        bits_to_f32(0x3f490fdbu),
        1.0f,
        f2);
    return 1;
}

// 0xbb6a10 | 36 B | ppfxSetExposureAdaptationParams
uint8_t sub_bb6a10(uint32_t /*a2*/, float f0, float f1, float f2) {
    sub_bd48d0(
        g_pSinglePostEffect(),
        f0,
        f1,
        f2,
        bits_to_f32(0x3c888889u),
        1.0f);
    return 1;
}

// 0xbb6980 | 131 B | ppfxApplyEffects
uint8_t sub_bb6980() {
    int32_t hr = sub_be95e0(g_pSinglePostEffect(), 0, 0);
    if (hr >= 0) {
        return 1;
    }
    if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1abb);
        const char* errStr = sub_c4ede0(hr);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe401ULL)), hr, errStr);
    }
    sub_2bbf178(
        reinterpret_cast<const char*>(GX_VMA(0x47fe450ULL)),
        reinterpret_cast<const char*>(GX_VMA(0x47fe4aaULL)));
    return 0;
}

// 0xbb68f0 | 135 B | ppfxBindFramebuffer
uint8_t sub_bb68f0(uint32_t rtIndex, void* pRenderTarget, uint32_t flags) {
    int32_t hr = sub_bd82f0(g_pSinglePostEffect(), rtIndex, pRenderTarget, flags, -1);
    if (hr >= 0) {
        return 1;
    }
    if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1aa6);
        const char* errStr = sub_c4ede0(hr);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe3afULL)), hr, errStr);
    }
    return 0;
}

// 0xbb6740 | 421 B | ppfxUninitialize
uint8_t sub_bb6740() {
    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1a75);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe265ULL)));
    }

    void* pPost = g_pSinglePostEffect();
    if (!pPost) {
        if (g_logEnabled() != 0 && g_logWarnEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe283ULL)), 0x1a79);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe2c8ULL)));
        }
        return 1;
    }

    int32_t hr = sub_bc7290(pPost, 0);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1a81);
            const char* errStr = sub_c4ede0(hr);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe2f6ULL)), hr, errStr);
        }
    } else if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1a85);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe342ULL)));
    }

    void* pToFree = g_pSinglePostEffect();
    if (pToFree != nullptr) {
        void* pAlloc = g_pMemoryAllocator();
        sub_bc70c0(pToFree);
        sub_bb6f50(pAlloc, pToFree);
    }
    set_pSinglePostEffect(nullptr);

    if (hr < 0) {
        return 0;
    }
    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1a90);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe386ULL)));
    }
    return 1;
}

// 0xbb6570 | 456 B | ppfxInitialize
uint8_t sub_bb6570(void* pDevice, uint32_t width, uint32_t height, uint32_t flags, void* pInitParam) {
    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1a44);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe1a5ULL)));
    }

    if (g_pSinglePostEffect() != nullptr) {
        sub_bb6740();
    }

    void* pNewPost = sub_bb6f20(
        g_pMemoryAllocator(),
        0xca8,
        reinterpret_cast<const char*>(GX_VMA(0x47fc72aULL)),
        0x1a4a,
        nullptr);
    sub_bc6a20(pNewPost);
    set_pSinglePostEffect(pNewPost);

    int32_t hr = sub_bc98b0(pNewPost, pDevice, width, height, flags, pInitParam);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1a5d);
            const char* errStr = sub_c4ede0(hr);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe1c1ULL)), hr, errStr);
        }
        return 0;
    }

    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
        uint8_t* pDevCtx = *reinterpret_cast<uint8_t**>(pPost + 0x18);
        int32_t gpuBytes = *reinterpret_cast<int32_t*>(pDevCtx + 0x9d0);
        float kb = static_cast<float>(gpuBytes) * 0.0009765625f;
        float mb = kb * 0.0009765625f;
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1a69);
        sub_bb1a90(
            reinterpret_cast<const char*>(GX_VMA(0x47fe209ULL)),
            gpuBytes,
            static_cast<double>(kb),
            static_cast<double>(mb));
    }

    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1a6e);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fe23eULL)));
    }
    return 1;
}

// 0xbb6550 | 25 B | ppfxGetEffectPerfTime2
uint64_t sub_bb6550() {
    extern uint64_t sub_bf99c0(void* pPostEffect, uint32_t flag);
    void* p = g_pSinglePostEffect();
    return p ? sub_bf99c0(p, 1) : 0;
}

// 0xbb6530 | 30 B | ppfxGetEffectPerfName2
const char* sub_bb6530() {
    extern const char* sub_bf98e0(void* pPostEffect, uint32_t flag);
    void* p = g_pSinglePostEffect();
    return p ? sub_bf98e0(p, 1) : reinterpret_cast<const char*>(GX_VMA(0x47fe1a4ULL));
}

// 0xbb6510 | 25 B | ppfxGetEffectPerfTime1
uint64_t sub_bb6510() {
    extern uint64_t sub_bf9800(void* pPostEffect, uint32_t flag);
    void* p = g_pSinglePostEffect();
    return p ? sub_bf9800(p, 1) : 0;
}

// 0xbb64f0 | 30 B | ppfxGetEffectPerfName1
const char* sub_bb64f0() {
    extern const char* sub_bf9720(void* pPostEffect, uint32_t flag);
    void* p = g_pSinglePostEffect();
    return p ? sub_bf9720(p, 1) : reinterpret_cast<const char*>(GX_VMA(0x47fe1a4ULL));
}

// 0xbb64d0 | 25 B | ppfxGetEffectPerfTime0
uint64_t sub_bb64d0() {
    extern uint64_t sub_bf9640(void* pPostEffect, uint32_t flag);
    void* p = g_pSinglePostEffect();
    return p ? sub_bf9640(p, 1) : 0;
}

// 0xbb64b0 | 30 B | ppfxGetEffectPerfName0
const char* sub_bb64b0() {
    extern const char* sub_bf9560(void* pPostEffect, uint32_t flag);
    void* p = g_pSinglePostEffect();
    return p ? sub_bf9560(p, 1) : reinterpret_cast<const char*>(GX_VMA(0x47fe1a4ULL));
}

// 0xbb6410 | 147 B | PPFX::GetMatrixFrustumJittered (by sample index)
void* sub_bb6410(
    const void* pfMatrixProj,
    uint32_t bColumnMajor,
    uint32_t iSample,
    void* pfOutMatrix,
    float fWidth,
    float fHeight) {
    if (!pfMatrixProj) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe0f2ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe07bULL)));
    }
    if (!pfOutMatrix) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe146ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe07bULL)));
    }
    __m128 jitter = sub_2610c30(iSample);
    __m128 jitterY = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(jitter), 1));
    sub_bb9250(
        pfOutMatrix,
        pfMatrixProj,
        bColumnMajor & 0xffu,
        _mm_set_ss(fWidth),
        _mm_set_ss(fHeight),
        jitter,
        jitterY);
    return pfOutMatrix;
}

// 0xbb6350 | 179 B | PPFX::GetMatrixFrustumJittered (by explicit offset)
void* sub_bb6350(
    const void* pfMatrixProj,
    uint32_t bColumnMajor,
    void* pfOutMatrix,
    float fWidth,
    float fHeight,
    float fJitterX,
    float fJitterY) {
    if (!pfMatrixProj) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe027ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe07bULL)));
    }
    if (!pfOutMatrix) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe094ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe07bULL)));
    }
    sub_bb9250(
        pfOutMatrix,
        pfMatrixProj,
        bColumnMajor & 0xffu,
        _mm_set_ss(fWidth),
        _mm_set_ss(fHeight),
        _mm_set_ss(fJitterX),
        _mm_set_ss(fJitterY));
    return pfOutMatrix;
}

extern "C" __attribute__((weak)) void BbGoldenEngine_OnYebisJitter(float jx, float jy);

// 0xbb6310 | 58 B | PPFX::GetJitteredSubpixel
void sub_bb6310(uint32_t iSample, void* afOutOffset) {
    if (!afOutOffset) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fdfbcULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe013ULL)));
    }
    __m128 jitter = sub_2610c30(iSample);
    _mm_storel_epi64(reinterpret_cast<__m128i*>(afOutOffset), _mm_castps_si128(jitter));
    if (BbGoldenEngine_OnYebisJitter) {
        const float* j = static_cast<const float*>(afOutOffset);
        BbGoldenEngine_OnYebisJitter(j[0], j[1]);
    }
}

// 0xbb6270 | 150 B | PPFX::GetApertureTimeValueFromExposureValue
void sub_bb6270(
    float* pfOutAperture,
    float* pfOutTimeValue,
    int32_t eAutoExposureMode,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    sub_2610920(pfOutAperture, pfOutTimeValue, &f0, eAutoExposureMode, &f1, &f2, &f3, &f4, &f5, &f6, &f7);
}

// 0xbb6120 | 326 B | GPUMath_ApertureTimeValueFromExposureValue
void sub_bb6120(
    float* pfOutAperture,
    float* pfOutTimeValue,
    int32_t eAutoExposureMode,
    float fExposureValue,
    float fPriorityValue,
    float fSensitivityBias,
    float fProgramWeight,
    float fMinAperture,
    float fMaxAperture,
    float fMinTime,
    float fMaxTime) {
    float f5 = 0.0f;
    float f1 = 0.0f;
    if (eAutoExposureMode == 2) {
        f5 = fExposureValue - fPriorityValue;
        f1 = fPriorityValue;
    } else if (eAutoExposureMode == 1) {
        f5 = fPriorityValue;
        f1 = fExposureValue - fPriorityValue;
    } else if (eAutoExposureMode == 0) {
        f5 = fExposureValue * fProgramWeight;
        f1 = (1.0f - fProgramWeight) * fExposureValue;
    } else {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe4bbULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe577ULL)));
        f5 = 0.0f;
        f1 = 0.0f;
    }

    float f3 = f5 - fSensitivityBias;
    float f2 = f1 + fSensitivityBias;

    float out0 = (fMinAperture > f3) ? fMinAperture : ((fMaxAperture < f3) ? fMaxAperture : f3);
    float out1 = (fMinTime > f2) ? fMinTime : ((fMaxTime < f2) ? fMaxTime : f2);

    if (eAutoExposureMode == 0) {
        if (f3 != out0) {
            out1 = (fExposureValue - out0) + 0.0f;
            out1 = (fMinTime > out1) ? fMinTime : ((fMaxTime < out1) ? fMaxTime : out1);
        } else if (f2 != out1) {
            out0 = fExposureValue - out1;
            out0 = (fMinAperture > out0) ? fMinAperture : ((fMaxAperture < out0) ? fMaxAperture : out0);
        }
    }

    if (pfOutAperture) {
        *pfOutAperture = out0;
    }
    if (pfOutTimeValue) {
        *pfOutTimeValue = out1;
    }
}

// 0xbb6110 | 9 B | PPFX::ConvertProjectionMatrixToPerspectiveParams
uint64_t sub_bb6110(void* a1, uint32_t a2, float f0, float f1, float f2, float f3) {
    return sub_bb9360(a1, a2 & 0xffu, f0, f1, f2, f3);
}

// 0xbb6100 | 9 B | PPFX::ConvertProjectionMatrixToOrthographicParams
uint64_t sub_bb6100(void* a1, uint32_t a2, float f0, float f1, float f2, float f3) {
    return sub_bb9510(a1, a2 & 0xffu, f0, f1, f2, f3);
}

// 0xbb60a0 | 81 B | PPFX::ComputeFNumberFromCoCAndLensParams
float sub_bb60a0(float fCoC, float fFocusDist, float fLensPower, float fSensorSize) {
    if (fFocusDist <= 0.0f || fLensPower == 0.0f) {
        return 1.0f;
    }
    float focalLen = fSensorSize / (fCoC + fCoC);
    float num = focalLen * fFocusDist;
    float denom = fFocusDist - focalLen;
    float ratio = (num / denom) / focalLen;
    float p = sub_2bbe948(ratio, fLensPower);
    return 1.0f / p;
}

// 0xbb5ff0 | 166 B | PPFX::ComputeFieldOfViewFromFocusAndAperture
float sub_bb5ff0(float fFovHalfRad, float fFocusDist, float fLensPower, float fSensorSize) {
    if (fFocusDist <= 0.0f || fLensPower == 0.0f) {
        return fFovHalfRad;
    }
    float tanHalf = sub_2bbe648(fFovHalfRad * 0.5f);
    float focalLen = fSensorSize / (tanHalf + tanHalf);
    if (focalLen >= fFocusDist) {
        fFocusDist = focalLen * bits_to_f32(0x3f8147aeu);
    }
    float num = focalLen * fFocusDist;
    float denom = fFocusDist - focalLen;
    float ratio = (num / denom) / focalLen;
    float p = sub_2bbe948(ratio, fLensPower);
    float effFocal = focalLen * p;
    float atanArg = (fSensorSize * 0.5f) / effFocal;
    float halfAngle = sub_2bbf128(atanArg);
    return halfAngle + halfAngle;
}

// 0xbb5fe0 | 8 B | PPFX::SetCustomLogCallback
void sub_bb5fe0(void* pCallback) {
    *reinterpret_cast<void**>(GX_VMA(0x5465ef0ULL)) = pCallback;
}

// 0xbb5f10 | 194 B | ppfxPlatformContextGnmShim::searchGnmTextureRegistry (and unregister)
int32_t sub_bb5f10(void* this_ctx, void* pGnmTexture) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    uint8_t* head = *reinterpret_cast<uint8_t**>(self + 0x20);
    uint8_t* cur = *reinterpret_cast<uint8_t**>(head + 0x08);
    uint8_t* candidate = head;
    while (cur[0x19] == 0) {
        if (*reinterpret_cast<void**>(cur + 0x20) < pGnmTexture) {
            cur = *reinterpret_cast<uint8_t**>(cur + 0x10);
        } else {
            candidate = cur;
            cur = *reinterpret_cast<uint8_t**>(cur + 0x00);
        }
    }
    if (candidate == head || pGnmTexture < *reinterpret_cast<void**>(candidate + 0x20)) {
        return static_cast<int32_t>(0x80004005u);
    }
    void* pInternalTex = *reinterpret_cast<void**>(candidate + 0x38);
    if (*reinterpret_cast<void**>(candidate + 0x28) != pGnmTexture ||
        *reinterpret_cast<void**>(candidate + 0x30) == nullptr ||
        pInternalTex == nullptr) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe5beULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe648ULL)));
    }
    int32_t hr = sub_bf0790(g_pSinglePostEffect(), pInternalTex);
    if (hr < 0) {
        return hr;
    }
    return sub_2610840(self + 0x18, pGnmTexture);
}

// 0xbb5e40 | 194 B | ppfxPlatformContextGnmShim::searchGnmRenderTargetRegistry (and unregister)
int32_t sub_bb5e40(void* this_ctx, void* pGnmRenderTarget) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    uint8_t* head = *reinterpret_cast<uint8_t**>(self + 0x38);
    uint8_t* cur = *reinterpret_cast<uint8_t**>(head + 0x08);
    uint8_t* candidate = head;
    while (cur[0x19] == 0) {
        if (*reinterpret_cast<void**>(cur + 0x20) < pGnmRenderTarget) {
            cur = *reinterpret_cast<uint8_t**>(cur + 0x10);
        } else {
            candidate = cur;
            cur = *reinterpret_cast<uint8_t**>(cur + 0x00);
        }
    }
    if (candidate == head || pGnmRenderTarget < *reinterpret_cast<void**>(candidate + 0x20)) {
        return static_cast<int32_t>(0x80004005u);
    }
    void* pInternalRt = *reinterpret_cast<void**>(candidate + 0x38);
    if (*reinterpret_cast<void**>(candidate + 0x28) != pGnmRenderTarget ||
        *reinterpret_cast<void**>(candidate + 0x30) == nullptr ||
        pInternalRt == nullptr) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe661ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe6e6ULL)));
    }
    int32_t hr = sub_bf05f0(g_pSinglePostEffect(), pInternalRt);
    if (hr < 0) {
        return hr;
    }
    return sub_2610760(self + 0x18, pGnmRenderTarget);
}

// 0xbb5c90 | 422 B | IPfxContext_Gnm::SetEffectMaskAlphaSource
int32_t sub_bb5c90(
    void* this_ctx,
    uint32_t maskIndex,
    void* pGnmTexture,
    uint32_t a4,
    uint32_t a5,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fde7dULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fdee4ULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x17d5);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fdefdULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fdf27ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fdee4ULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x17ec);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fdf81ULL)));
        }
        return hr;
    }

    uint8_t dummyDesc[48];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, dummyDesc);
    return sub_bf37f0(g_pSinglePostEffect(), maskIndex, regPair[2], a4, a5, pfxFmt);
}

// 0xbb5ae0 | 422 B | IPfxContext_Gnm::SetEffectMaskColorSource
int32_t sub_bb5ae0(
    void* this_ctx,
    uint32_t maskIndex,
    void* pGnmTexture,
    uint32_t a4,
    uint32_t a5,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fdd3eULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fdda5ULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x17ac);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fddbeULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fdde8ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fdda5ULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x17c3);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fde42ULL)));
        }
        return hr;
    }

    uint8_t dummyDesc[48];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, dummyDesc);
    return sub_bf3610(g_pSinglePostEffect(), maskIndex, regPair[2], a4, a5, pfxFmt);
}

// 0xbb5930 | 422 B | IPfxContext_Gnm::SetGlareModulatorSource
int32_t sub_bb5930(
    void* this_ctx,
    uint32_t modIndex,
    void* pGnmTexture,
    uint32_t a4,
    uint32_t a5,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fdc02ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fdc69ULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1783);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fdc81ULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fdcaaULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fdc69ULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x179a);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fdd04ULL)));
        }
        return hr;
    }

    uint8_t dummyDesc[48];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, dummyDesc);
    return sub_bf3430(g_pSinglePostEffect(), modIndex, regPair[2], a4, a5, pfxFmt);
}

// 0xbb5790 | 414 B | IPfxContext_Gnm::SetDepthOfFieldDepthRemapLutSource
int32_t sub_bb5790(
    void* this_ctx,
    void* pGnmTexture,
    uint32_t a3,
    uint32_t a4,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fdaa5ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fdb0cULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x175b);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fdb2fULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fdb63ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fdb0cULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1771);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fdbbdULL)));
        }
        return hr;
    }

    uint8_t dummyDesc[48];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, dummyDesc);
    return sub_bf39c0(g_pSinglePostEffect(), regPair[2], a3, a4, pfxFmt);
}

// 0xbb5600 | 395 B | IPfxContext_Gnm::SetColorGradingLutSource
int32_t sub_bb5600(
    void* this_ctx,
    void* pGnmTexture,
    uint32_t a3,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd966ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd9cdULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1735);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd9e6ULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fda10ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fd9cdULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x174a);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fda6aULL)));
        }
        return hr;
    }

    uint8_t dummyDesc[48];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, dummyDesc);
    return sub_bf3280(g_pSinglePostEffect(), regPair[2], a3, pfxFmt);
}

// 0xbb5430 | 455 B | IPfxContext_Gnm::SetMotionBlurVelocitySource
int32_t sub_bb5430(
    void* this_ctx,
    uint32_t a2,
    void* pGnmTexture,
    uint32_t a4,
    uint32_t a5,
    uint32_t gnmFormat,
    uint32_t a7,
    void* a8,
    uint8_t a9) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd81eULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd885ULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x170b);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd8a1ULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fd8ceULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fd885ULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1725);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd928ULL)));
        }
        return hr;
    }

    uint8_t dummyDesc[48];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, dummyDesc);
    return sub_bda460(
        g_pSinglePostEffect(),
        a2,
        regPair[2],
        a4,
        a5,
        pfxFmt,
        a7,
        a8,
        static_cast<uint32_t>(a9));
}

// 0xbb5290 | 409 B | IPfxContext_Gnm::SetTonemapDestination
int32_t sub_bb5290(
    void* this_ctx,
    void* pGnmRenderTarget,
    uint32_t /*a3*/,
    uint32_t /*a4*/,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd6e8ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd74fULL)));
    }
    uint32_t pfxFmt = 0;
    int32_t hr = sub_11d6680(gnmFormat, &pfxFmt);
    if (hr != 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x16e3);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd765ULL)));
        }
        if (hr < 0) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fd78cULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fd74fULL)));
        }
        return hr;
    }

    uint32_t oldFmt = sub_1086cf0(pGnmRenderTarget);
    sub_1086540(pGnmRenderTarget, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    hr = sub_26105f0(self + 0x18, pGnmRenderTarget, regPair);
    sub_1086540(pGnmRenderTarget, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x16f6);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd7e6ULL)));
        }
        return hr;
    }

    uint32_t rtDesc[12];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, rtDesc);
    return sub_bf3b70(g_pSinglePostEffect(), regPair[2], rtDesc[0], rtDesc[1], rtDesc[4], 1, 0);
}

// 0xbb51e0 | 174 B | IPfxContext_Gnm::ResolveMsaa
int32_t sub_bb51e0(
    void* this_ctx,
    GnmResolveTextureSpec48 srcSpec,
    GnmResolveTextureSpec48 dstSpec) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd675ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd6dcULL)));
    }
    void* pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
    if (!pDrawCmdBuf) {
        return static_cast<int32_t>(0x80004005u);
    }
    sub_11d7140(pDrawCmdBuf, srcSpec, dstSpec);
    return 0;
}

// 0xbb5080 | 346 B | IPfxContext_Gnm::SetDepthFactorSource
int32_t sub_bb5080(
    void* this_ctx,
    uint32_t depthType,
    void* pGnmTexture,
    uint32_t /*a4*/,
    uint32_t /*a5*/,
    uint32_t gnmFormat,
    float fNear,
    float fFar,
    uint32_t flags) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd500ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd567ULL)));
    }
    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    int32_t hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x16bd);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd57cULL)));
        }
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd5b3ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd567ULL)));
        return hr;
    }

    uint32_t texDesc[12];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, texDesc);

    extern int32_t sub_bf8c20(
        void* pPostEffect,
        uint32_t depthType,
        void* pTexObj,
        uint32_t width,
        uint32_t height,
        uint32_t fmt,
        float fNear,
        float fFar,
        uint32_t flags);
    return sub_bf8c20(
        g_pSinglePostEffect(),
        depthType,
        regPair[2],
        texDesc[0],
        texDesc[1],
        texDesc[4],
        fNear,
        fFar,
        flags);
}

// 0xbb4f60 | 280 B | IPfxContext_Gnm::SetEffectSource
int32_t sub_bb4f60(
    void* this_ctx,
    void* pGnmTexture,
    uint32_t /*a3*/,
    uint32_t /*a4*/,
    uint32_t gnmFormat) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd457ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd4beULL)));
    }
    uint32_t oldFmt = sub_1089960(pGnmTexture);
    sub_10883f0(pGnmTexture, gnmFormat);
    void* regPair[3] = {nullptr, nullptr, nullptr};
    int32_t hr = sub_2610430(self + 0x18, pGnmTexture, regPair);
    sub_10883f0(pGnmTexture, oldFmt);
    if (hr < 0) {
        if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x169e);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd4ceULL)));
        }
        return hr;
    }

    uint32_t texDesc[12];
    void* pWrapper = regPair[1];
    uint64_t vtbl = *reinterpret_cast<uint64_t*>(pWrapper);
    reinterpret_cast<void (*)(void*, void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x50))(pWrapper, texDesc);
    return sub_bf30d0(g_pSinglePostEffect(), regPair[2], texDesc[0], texDesc[1], texDesc[4], 0);
}

// 0xbb4f10 | 78 B | IPfxContext_Gnm::RenderEffectsWithStateGuard
int32_t sub_bb4f10(void* this_ctx, uint32_t flags) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] != 0) {
        sub_11d7190(*reinterpret_cast<void**>(self + 0x58));
    }
    int32_t hr = sub_befb50(g_pSinglePostEffect(), flags);
    if (self[0x48] != 0) {
        sub_11d71e0(*reinterpret_cast<void**>(self + 0x58));
        sub_11d6860(*reinterpret_cast<void**>(self + 0x50));
    }
    return hr;
}

// 0xbb4ed0 | 61 B | IPfxContext_Gnm::PostApply
int32_t sub_bb4ed0(void* this_ctx) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd604ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd66bULL)));
    }
    sub_11d71e0(*reinterpret_cast<void**>(self + 0x58));
    sub_11d6860(*reinterpret_cast<void**>(self + 0x50));
    return 0;
}

// 0xbb4e80 | 76 B | IPfxContext_Gnm::ApplyEffectsAndPostApply
int32_t sub_bb4e80(void* this_ctx, uint32_t a2, uint32_t a3) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    int32_t hr = sub_be95e0(g_pSinglePostEffect(), a2, a3);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd604ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd66bULL)));
    }
    sub_11d71e0(*reinterpret_cast<void**>(self + 0x58));
    sub_11d6860(*reinterpret_cast<void**>(self + 0x50));
    return hr;
}

// 0xbb4de0 | 154 B | IPfxContext_Gnm::SetActiveCommandBuffers
int32_t sub_bb4de0(void* this_ctx, int32_t iCommandBuffer) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd233ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd29aULL)));
    }
    if (iCommandBuffer < 0 || iCommandBuffer >= *reinterpret_cast<int32_t*>(self + 0x174)) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd2b2ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd29aULL)));
    }
    *reinterpret_cast<int32_t*>(self + 0x170) = iCommandBuffer;
    void* pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
    if (!pDrawCmdBuf) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd352ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd29aULL)));
        pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
        iCommandBuffer = *reinterpret_cast<int32_t*>(self + 0x170);
    }
    void** ppDrawBufs = *reinterpret_cast<void***>(self + 0x160);
    void** ppConstBufs = *reinterpret_cast<void***>(self + 0x168);
    sub_11d71a0(pDrawCmdBuf, ppDrawBufs[iCommandBuffer], ppConstBufs[iCommandBuffer]);
    return 0;
}

// 0xbb4d70 | 109 B | IPfxContext_Gnm::SetActiveBackBuffer
int32_t sub_bb4d70(void* this_ctx, int32_t iBackBuffer) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] == 0) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd11eULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd185ULL)));
    }
    if (iBackBuffer < 0 || iBackBuffer >= *reinterpret_cast<int32_t*>(self + 0x70)) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd199ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd185ULL)));
    }
    if (iBackBuffer >= 0 && iBackBuffer <= *reinterpret_cast<int32_t*>(self + 0x70)) {
        *reinterpret_cast<int32_t*>(self + 0x88) = iBackBuffer;
        void** ppBackBuffers = *reinterpret_cast<void***>(self + 0x80);
        *reinterpret_cast<void**>(self + 0x78) = ppBackBuffers[iBackBuffer];
    }
    return 0;
}

// 0xbb4d30 | 64 B | IPfxContext_Gnm::Uninitialize
int32_t sub_bb4d30(void* this_ctx) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    if (self[0x48] != 0) {
        void* pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
        if (pDrawCmdBuf != nullptr) {
            sub_11d7030(pDrawCmdBuf);
        }
    }
    sub_bb2310(self);
    if (self[0x48] != 0) {
        sub_2610300(self + 0x18);
    }
    return 0;
}

// 0xbb4ce0 | 75 B | IPfxContext_Gnm::SetDrawAndConstantCommandBuffers
int32_t sub_bb4ce0(void* this_ctx, void* pDrawBuf, void* pConstBuf) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    void* pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
    if (!pDrawCmdBuf) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fd3c4ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fd436ULL)));
        pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
    }
    sub_11d71a0(pDrawCmdBuf, pDrawBuf, pConstBuf);
    return 0;
}

// 0xbb4b40 | 412 B | IPfxContext_Gnm::Initialize
int32_t sub_bb4b40(
    void* this_ctx,
    void* pInitConfig,
    uint32_t width,
    uint32_t height,
    uint32_t flags,
    void* pExtraParam) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    uint8_t* cfg = reinterpret_cast<uint8_t*>(pInitConfig);

    uint64_t localDesc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    void* pDev0 = *reinterpret_cast<void**>(cfg + 0x30);
    void* pDev1 = *reinterpret_cast<void**>(cfg + 0x38);

    if (pDev0 != nullptr && pDev1 != nullptr) {
        if (*reinterpret_cast<void**>(cfg + 0x00) != nullptr ||
            *reinterpret_cast<void**>(cfg + 0x08) != nullptr) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fd098ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fc803ULL)));
            pDev0 = *reinterpret_cast<void**>(cfg + 0x30);
            pDev1 = *reinterpret_cast<void**>(cfg + 0x38);
        }
        localDesc[6] = reinterpret_cast<uint64_t>(pDev0);
        localDesc[7] = reinterpret_cast<uint64_t>(pDev1);
    } else {
        sub_11d71d0(*reinterpret_cast<void**>(cfg + 0x00), *reinterpret_cast<void**>(cfg + 0x08));
        int32_t hr = sub_2610000(
            self + 0x18,
            *reinterpret_cast<void**>(cfg + 0x10),
            *reinterpret_cast<uint32_t*>(cfg + 0x28),
            *reinterpret_cast<void**>(cfg + 0x18),
            *reinterpret_cast<void**>(cfg + 0x20),
            *reinterpret_cast<uint32_t*>(cfg + 0x2c));
        if (hr != 0) {
            return hr;
        }
        int32_t activeIdx = *reinterpret_cast<int32_t*>(self + 0x170);
        void* pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
        void* pDrawBuf = (*reinterpret_cast<void***>(self + 0x160))[activeIdx];
        void* pConstBuf = (*reinterpret_cast<void***>(self + 0x168))[activeIdx];
        if (!pDrawCmdBuf) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fd3c4ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fd436ULL)));
            pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
        }
        sub_11d71a0(pDrawCmdBuf, pDrawBuf, pConstBuf);
        localDesc[6] = *reinterpret_cast<uint64_t*>(self + 0x58);
        localDesc[7] = *reinterpret_cast<uint64_t*>(self + 0x60);
    }

    int32_t hr = sub_bb1c20(self, localDesc, width, height, flags, pExtraParam);
    if (self[0x48] != 0) {
        sub_2610250(self + 0x18);
    }
    return hr;
}

// 0xbb4b20 | 19 B | IPfxContext_Gnm::SetCommandBuffersStatic
int32_t sub_bb4b20(void* /*this_ctx*/, void* a2, void* a3) {
    sub_11d71d0(a2, a3);
    return 0;
}

// 0xbb4ac0 | 89 B | IPfxContext::Instantiate
void* sub_bb4ac0() {
    uint8_t* pObj = reinterpret_cast<uint8_t*>(sub_bb6f20(
        g_pMemoryAllocator(),
        0x178,
        reinterpret_cast<const char*>(GX_VMA(0x47fc72aULL)),
        0x275,
        reinterpret_cast<const char*>(GX_VMA(0x47fc75aULL))));
    sub_bb6d20(pObj);
    *reinterpret_cast<uint64_t*>(pObj + 0x00) = GX_VMA(0x52b3580ULL);
    sub_26128b0(pObj + 0x18);
    return pObj;
}

// 0xbb4ab0 | 12 B | IPfxBaseContext::GetActiveBackBufferHeight
uint64_t sub_bb4ab0(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bc9430(g_pSinglePostEffect(), a2, a3, a4, a5, a6);
}

// 0xbb4aa0 | 12 B | IPfxBaseContext::GetActiveBackBufferFormat
uint64_t sub_bb4aa0(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bc94b0(g_pSinglePostEffect(), a2, a3, a4, a5, a6);
}

// 0xbb4a90 | 12 B | IPfxBaseContext::OnResetDeviceResources
uint64_t sub_bb4a90(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bd7c10(g_pSinglePostEffect(), a2, a3, a4, a5, a6);
}

// 0xbb48d0 | 436 B | IPfxContext_Gnm::~IPfxContext_Gnm
void sub_bb48d0(void* this_ctx) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x52b3580ULL);

    if (self[0x48] != 0) {
        void* pDrawCmdBuf = *reinterpret_cast<void**>(self + 0x58);
        if (pDrawCmdBuf != nullptr) {
            sub_11d7030(pDrawCmdBuf);
        }
    }
    sub_bb2310(self);
    if (self[0x48] != 0) {
        sub_2610300(self + 0x18);
    }

    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x15ed);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd05cULL)));
    }

    PfxTreeIter24 outIter;
    void* pRtHead = *reinterpret_cast<void**>(self + 0x38);
    void* pRtBegin = *reinterpret_cast<void**>(pRtHead);
    sub_2611880(
        &outIter,
        self + 0x30,
        PfxTreeIter24{0, 0, pRtBegin},
        PfxTreeIter24{0, 0, pRtHead});
    sub_2bbe588(*reinterpret_cast<void**>(self + 0x38));

    void* pTexHead = *reinterpret_cast<void**>(self + 0x20);
    void* pTexBegin = *reinterpret_cast<void**>(pTexHead);
    sub_2611020(
        &outIter,
        self + 0x18,
        PfxTreeIter24{0, 0, pTexBegin},
        PfxTreeIter24{0, 0, pTexHead});
    sub_2bbe588(*reinterpret_cast<void**>(self + 0x20));

    sub_bb18b0(self);
}

// 0xbb48b0 | 28 B | IPfxContext_Gnm deleting destructor
void sub_bb48b0(void* this_ctx) {
    sub_bb48d0(this_ctx);
    sub_2bbe588(this_ctx);
}

// 0xbb4880 | 46 B | IPfxContext_Gnm::IPfxContext_Gnm
void sub_bb4880(void* this_ctx) {
    uint8_t* self = reinterpret_cast<uint8_t*>(this_ctx);
    sub_bb6d20(self);
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x52b3580ULL);
    sub_26128b0(self + 0x18);
}

// ============================================================================
// IPfxBaseContext::UninitializeDeviceResources & Singleton Forwarding Methods
// (0xbb4000 .. 0xbb4870)
// ============================================================================

#define PFX_DECLARE_FORWARD(target_fn) \
    uint64_t target_fn( \
        void* pPostEffect, \
        uint64_t a2, \
        uint64_t a3, \
        uint64_t a4, \
        uint64_t a5, \
        uint64_t a6, \
        float f0, \
        float f1, \
        float f2, \
        float f3, \
        float f4, \
        float f5, \
        float f6, \
        float f7)

#define PFX_DEFINE_FORWARD(fn_name, target_fn) \
    PFX_DECLARE_FORWARD(target_fn); \
    uint64_t fn_name( \
        void* /*this_ctx*/, \
        uint64_t a2, \
        uint64_t a3, \
        uint64_t a4, \
        uint64_t a5, \
        uint64_t a6, \
        float f0, \
        float f1, \
        float f2, \
        float f3, \
        float f4, \
        float f5, \
        float f6, \
        float f7) { \
        return target_fn(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7); \
    }

#define PFX_DEFINE_FORWARD_U8_ARG3(fn_name, target_fn) \
    uint64_t target_fn( \
        void* pPostEffect, \
        uint64_t a2, \
        uint32_t a3, \
        uint64_t a4, \
        uint64_t a5, \
        uint64_t a6, \
        float f0, \
        float f1, \
        float f2, \
        float f3, \
        float f4, \
        float f5, \
        float f6, \
        float f7); \
    uint64_t fn_name( \
        void* /*this_ctx*/, \
        uint64_t a2, \
        uint32_t a3, \
        uint64_t a4, \
        uint64_t a5, \
        uint64_t a6, \
        float f0, \
        float f1, \
        float f2, \
        float f3, \
        float f4, \
        float f5, \
        float f6, \
        float f7) { \
        return target_fn( \
            g_pSinglePostEffect(), \
            a2, \
            a3 & 0xffu, \
            a4, \
            a5, \
            a6, \
            f0, \
            f1, \
            f2, \
            f3, \
            f4, \
            f5, \
            f6, \
            f7); \
    }

#define PFX_DEFINE_FORWARD_U8_ARG2(fn_name, target_fn) \
    uint64_t fn_name( \
        void* /*this_ctx*/, \
        uint32_t a2, \
        uint64_t a3, \
        uint64_t a4, \
        uint64_t a5, \
        uint64_t a6, \
        float f0, \
        float f1, \
        float f2, \
        float f3, \
        float f4, \
        float f5, \
        float f6, \
        float f7) { \
        return target_fn( \
            g_pSinglePostEffect(), \
            a2 & 0xffu, \
            a3, \
            a4, \
            a5, \
            a6, \
            f0, \
            f1, \
            f2, \
            f3, \
            f4, \
            f5, \
            f6, \
            f7); \
    }

// 0xbb4870 | 5 B | PPFX::SetShaderPackageSourceConfig
uint64_t sub_bb4870(void* a1, uint32_t a2, void* a3) {
    return sub_bc69f0(a1, a2, a3);
}

// 0xbb4710 | 341 B | IPfxBaseContext::UninitializeDeviceResources
int32_t sub_bb4710() {
    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1467);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fcf15ULL)));
    }

    int32_t hr = 0;
    if (g_pSinglePostEffect() != nullptr) {
        void* pRes0 = *reinterpret_cast<void**>(GX_VMA(0x5465ed8ULL));
        if (pRes0 != nullptr) {
            uint64_t vtbl = *reinterpret_cast<uint64_t*>(pRes0);
            reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x08))(pRes0);
            *reinterpret_cast<void**>(GX_VMA(0x5465ed8ULL)) = nullptr;
        }
        void* pRes1 = *reinterpret_cast<void**>(GX_VMA(0x5465ee0ULL));
        if (pRes1 != nullptr) {
            uint64_t vtbl = *reinterpret_cast<uint64_t*>(pRes1);
            reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uint64_t*>(vtbl + 0x08))(pRes1);
            *reinterpret_cast<void**>(GX_VMA(0x5465ee0ULL)) = nullptr;
        }

        hr = sub_bd7bf0(g_pSinglePostEffect());
        if (hr < 0) {
            if (g_logEnabled() != 0 && g_logErrorEnabled() != 0) {
                sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc59dULL)), 0x1475);
                const char* errStr = sub_c4ede0(hr);
                sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fcf4fULL)), hr, errStr);
            }
            return hr;
        }
        if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1479);
            sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fcfb7ULL)));
        }
    }

    if (g_logEnabled() != 0 && g_logInfoEnabled() != 0) {
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fc520ULL)), 0x1482);
        sub_bb1a90(reinterpret_cast<const char*>(GX_VMA(0x47fd017ULL)));
    }
    return 0;
}

PFX_DEFINE_FORWARD(sub_bb4700, sub_bf9500)
PFX_DEFINE_FORWARD(sub_bb46f0, sub_bf94a0)
PFX_DEFINE_FORWARD(sub_bb46e0, sub_bf9440)
PFX_DEFINE_FORWARD(sub_bb46d0, sub_bf93e0)
PFX_DEFINE_FORWARD(sub_bb46c0, sub_bf9380)
PFX_DEFINE_FORWARD(sub_bb46b0, sub_bf9340)
PFX_DEFINE_FORWARD(sub_bb46a0, sub_bf9300)
PFX_DEFINE_FORWARD(sub_bb4690, sub_bf92c0)

// 0xbb4680 | 12 B | IPfxBaseContext::GetBackBufferDesc
uint64_t sub_bb4680(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bc9530(g_pSinglePostEffect(), a2, a3, a4, a5, a6);
}

// 0xbb4670 | 12 B | IPfxBaseContext::GetBackBufferFormat2
uint64_t sub_bb4670(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bc94b0(g_pSinglePostEffect(), a2, a3, a4, a5, a6);
}

PFX_DEFINE_FORWARD(sub_bb4660, sub_bf92b0)

// 0xbb4650 | 12 B | IPfxBaseContext::GetLastExposureValue (returns float)
float sub_bb4650(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bd2880(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

PFX_DEFINE_FORWARD(sub_bb4640, sub_bf30a0)
PFX_DEFINE_FORWARD(sub_bb4630, sub_bf3070)
PFX_DEFINE_FORWARD(sub_bb4620, sub_bd27a0)
PFX_DEFINE_FORWARD(sub_bb4610, sub_bd27f0)
PFX_DEFINE_FORWARD(sub_bb4600, sub_bf92a0)
PFX_DEFINE_FORWARD(sub_bb45f0, sub_bf9230)
PFX_DEFINE_FORWARD(sub_bb45e0, sub_bf9290)
PFX_DEFINE_FORWARD(sub_bb45d0, sub_bf8bb0)
PFX_DEFINE_FORWARD(sub_bb45c0, sub_bf8a40)

// 0xbb45a0 | 21 B | IPfxBaseContext::TestEffectFlagMaskF0
uint8_t sub_bb45a0(void* /*this_ctx*/, uint32_t mask) {
    uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    uint32_t flags = *reinterpret_cast<uint32_t*>(pPost + 0x21c);
    return (((mask & flags) & 0xf0u) != 0) ? 1u : 0u;
}

// 0xbb4580 | 19 B | IPfxBaseContext::GetEffectFlagMaskF0
uint32_t sub_bb4580(void* /*this_ctx*/) {
    uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    return *reinterpret_cast<uint32_t*>(pPost + 0x21c) & 0xf0u;
}

// 0xbb4560 | 22 B | IPfxBaseContext::TestEffectFlagMask10F
uint8_t sub_bb4560(void* /*this_ctx*/, uint32_t mask) {
    uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    uint32_t flags = *reinterpret_cast<uint32_t*>(pPost + 0x21c);
    return (((mask & flags) & 0x10fu) != 0) ? 1u : 0u;
}

// 0xbb4540 | 19 B | IPfxBaseContext::GetEffectFlagMask10F
uint32_t sub_bb4540(void* /*this_ctx*/) {
    uint8_t* pPost = reinterpret_cast<uint8_t*>(g_pSinglePostEffect());
    return *reinterpret_cast<uint32_t*>(pPost + 0x21c) & 0x10fu;
}

PFX_DEFINE_FORWARD(sub_bb4530, sub_bf6570)
PFX_DEFINE_FORWARD(sub_bb4520, sub_bf6250)
PFX_DEFINE_FORWARD(sub_bb4510, sub_bf63e0)
PFX_DEFINE_FORWARD(sub_bb4500, sub_bf60f0)
PFX_DEFINE_FORWARD(sub_bb44f0, sub_bf5fb0)
PFX_DEFINE_FORWARD(sub_bb44e0, sub_bf5e50)
PFX_DEFINE_FORWARD(sub_bb44d0, sub_bf5cf0)
PFX_DEFINE_FORWARD(sub_bb44c0, sub_bf5b90)
PFX_DEFINE_FORWARD(sub_bb44b0, sub_bf8850)
PFX_DEFINE_FORWARD(sub_bb44a0, sub_bf8660)
PFX_DEFINE_FORWARD(sub_bb4490, sub_bf8310)
PFX_DEFINE_FORWARD(sub_bb4480, sub_bf82d0)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb4470, sub_bf8190)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb4460, sub_bf8040)
PFX_DEFINE_FORWARD(sub_bb4450, sub_bf7d50)
PFX_DEFINE_FORWARD(sub_bb4440, sub_bf7d30)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb4430, sub_bf7870)
PFX_DEFINE_FORWARD(sub_bb4420, sub_bf7c00)
PFX_DEFINE_FORWARD(sub_bb4410, sub_bf7550)
PFX_DEFINE_FORWARD(sub_bb4400, sub_bf76e0)
PFX_DEFINE_FORWARD(sub_bb43f0, sub_bf7420)
PFX_DEFINE_FORWARD(sub_bb43e0, sub_bf6aa0)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb43d0, sub_bf6fe0)
PFX_DEFINE_FORWARD(sub_bb43c0, sub_bf6e50)
PFX_DEFINE_FORWARD(sub_bb43b0, sub_bf7290)
PFX_DEFINE_FORWARD(sub_bb43a0, sub_bf6d20)
PFX_DEFINE_FORWARD(sub_bb4390, sub_bf6be0)
PFX_DEFINE_FORWARD(sub_bb4380, sub_bf6960)
PFX_DEFINE_FORWARD(sub_bb4370, sub_bf6830)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb4360, sub_bf6700)
PFX_DEFINE_FORWARD(sub_bb4350, sub_bf5a30)

// 0xbb4330 | 31 B | IPfxBaseContext::GetCameraMatrixByValue
void* sub_bb4330(void* outMatrixRet, void* /*this_ctx*/, uint32_t matrixType) {
    extern void sub_bf5910(void* outMatrix, void* pPostEffect, uint32_t matrixType);
    sub_bf5910(outMatrixRet, g_pSinglePostEffect(), matrixType);
    return outMatrixRet;
}

PFX_DEFINE_FORWARD(sub_bb4320, sub_bf5850)
PFX_DEFINE_FORWARD(sub_bb4310, sub_bf5780)
PFX_DEFINE_FORWARD(sub_bb4300, sub_bd6cb0)
PFX_DEFINE_FORWARD(sub_bb42f0, sub_bf54f0)
PFX_DEFINE_FORWARD(sub_bb42e0, sub_bf54b0)
PFX_DEFINE_FORWARD(sub_bb42d0, sub_bf51b0)
PFX_DEFINE_FORWARD(sub_bb42c0, sub_bf5190)
PFX_DEFINE_FORWARD(sub_bb42b0, sub_bf4f20)
PFX_DEFINE_FORWARD(sub_bb42a0, sub_bf4c60)
PFX_DEFINE_FORWARD(sub_bb4290, sub_bf4dc0)
PFX_DEFINE_FORWARD(sub_bb4280, sub_bf4b50)
PFX_DEFINE_FORWARD(sub_bb4270, sub_bf4550)
PFX_DEFINE_FORWARD(sub_bb4260, sub_bf48b0)
PFX_DEFINE_FORWARD(sub_bb4250, sub_bf4a00)
PFX_DEFINE_FORWARD(sub_bb4240, sub_bf4790)
PFX_DEFINE_FORWARD(sub_bb4230, sub_bf4670)
PFX_DEFINE_FORWARD(sub_bb4220, sub_bf4430)
PFX_DEFINE_FORWARD(sub_bb4210, sub_bf4310)
PFX_DEFINE_FORWARD(sub_bb4200, sub_bf4020)
PFX_DEFINE_FORWARD(sub_bb41f0, sub_bf3ef0)
PFX_DEFINE_FORWARD(sub_bb41e0, sub_bf3e60)
PFX_DEFINE_FORWARD(sub_bb41d0, sub_bf3dd0)
PFX_DEFINE_FORWARD(sub_bb41c0, sub_bf3d50)

// 0xbb41b0 | 12 B | IPfxBaseContext::GetLightShaftThreshold (returns float)
float sub_bb41b0(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bd99d0(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

PFX_DEFINE_FORWARD(sub_bb41a0, sub_bd99b0)
PFX_DEFINE_FORWARD(sub_bb4190, sub_bd4c50)
PFX_DEFINE_FORWARD(sub_bb4180, sub_bd4c40)
PFX_DEFINE_FORWARD(sub_bb4170, sub_bd99a0)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb4160, sub_bd4c20)

// 0xbb4140 | 19 B | IPfxBaseContext::SetAutoExposureFlags
uint64_t sub_bb4140(
    void* /*this_ctx*/,
    uint32_t a2,
    uint32_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bd6850(
        g_pSinglePostEffect(),
        a2 & 0xffu,
        a3 & 0xffu,
        a4,
        a5,
        a6,
        f0,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7);
}

PFX_DEFINE_FORWARD(sub_bb4130, sub_bf91f0)
PFX_DEFINE_FORWARD(sub_bb4120, sub_bf91c0)
PFX_DEFINE_FORWARD(sub_bb4110, sub_bf9190)
PFX_DEFINE_FORWARD(sub_bb4100, sub_bd63d0)
PFX_DEFINE_FORWARD(sub_bb40f0, sub_bf9130)
PFX_DEFINE_FORWARD(sub_bb40e0, sub_bf9030)
PFX_DEFINE_FORWARD(sub_bb40d0, sub_bf9110)
PFX_DEFINE_FORWARD(sub_bb40c0, sub_bf90f0)
PFX_DEFINE_FORWARD(sub_bb40b0, sub_bd4800)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb40a0, sub_bd47d0)
PFX_DEFINE_FORWARD(sub_bb4090, sub_bd4720)

// 0xbb4050 | 50 B | IPfxBaseContext::SetLensVignetteTypeAndRadius
uint64_t sub_bb4050(void* /*this_ctx*/, uint32_t a2, uint32_t a3, float f0) {
    sub_bd46b0(g_pSinglePostEffect(), a2);
    return sub_bd46f0(g_pSinglePostEffect(), a3, f0);
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb4040, sub_bdc710)
PFX_DEFINE_FORWARD(sub_bb4030, sub_bdc800)
PFX_DEFINE_FORWARD(sub_bb4020, sub_bdc7a0)
PFX_DEFINE_FORWARD(sub_bb4010, sub_bd4840)
PFX_DEFINE_FORWARD(sub_bb4000, sub_bdc740)

} // extern "C"
