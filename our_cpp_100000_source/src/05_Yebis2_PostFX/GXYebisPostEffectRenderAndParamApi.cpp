#include <cstdint>
#include <cstring>
#include <immintrin.h>

// ============================================================================
// Module 05: Yebis 2 PostFX Middleware Core (05_Yebis2_PostFX)
// File: GXYebisPostEffectRenderAndParamApi.cpp
// Range: 0x0bb2600 .. 0x0bb3fff (242 functions, 5,607 bytes)
// ============================================================================
// Architectural Overview:
//   Implements the lower half of Silicon Studio's Yebis 2 (PPFX) public
//   `IPfxBaseContext` API (`ppfx.cpp`), completing the 433-function public
//   entry bridge (`0xbb2600 .. 0xbb7500`):
//   - Main render / post-processing execution entry `sub_bb3c50` (`ApplyEffects`
//     -> `CPostEffect::Render` `sub_bd82f0`)
//   - `IPfxInterface` resource unwrapping forwarders (`sub_bb6e50`) for LUTs,
//     masks, glare modulators, and velocity/depth textures (`sub_bb3ee0`,
//     `sub_bb3e90`, `sub_bb3e40`, `sub_bb3dc0`, `sub_bb3d90`, `sub_bb3d10`,
//     `sub_bb3cb0`, `sub_bb39f0`, `sub_bb3910`, `sub_bb3620`)
//   - Multi-view (`GetEffectiveViewIndex`) parameter inspectors (`sub_bb33e0`,
//     `sub_bb3360`, `sub_bb3250`, `sub_bb31d0`)
//   - Depth factor conversion & scale/offset solvers (`ConvertDepthValue`
//     `sub_bb2dc0`, `GetDepthConversionFactor` `sub_bb2c20`,
//     `GetDepthConversionFactorValue` `sub_bb2a90`,
//     `GetDepthFactorScaleOffset` `sub_bb2a40`,
//     `GetDepthOfFieldFactorScaleOffset` `sub_bb29f0`,
//     `GetMotionBlurInternalVelocityTexture` `sub_bb2610`)
//   - Direct `CPostEffect` (`s_pSinglePostEffect` at `0x5465ed0`) field
//     accessors and tail-call forwarding methods (`0xbb2600` .. `0xbb3ff0`).
// ============================================================================

namespace {

inline void* g_pSinglePostEffect() {
    return *reinterpret_cast<void**>(GX_VMA(0x5465ed0ULL));
}

inline uint8_t* g_pSinglePostEffectBytes() {
    return *reinterpret_cast<uint8_t**>(GX_VMA(0x5465ed0ULL));
}

} // namespace

struct PfxDepthFactorVec4 {
    float x;
    float y;
    float z;
    float w;
};

extern "C" {

// External helper declarations
void* sub_bb6e50(void* pResourceIf);
void sub_2bbf178(const char* exprMsg, const char* funcName);
int32_t sub_bdbe90(
    void* pPostEffect,
    float* outFactor4,
    uint32_t srcType,
    uint32_t dstType,
    float scaleInit,
    float offsetInit);

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
        return target_fn( \
            g_pSinglePostEffect(), \
            a2, \
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

#define PFX_DEFINE_FORWARD_F32(fn_name, target_fn) \
    float target_fn( \
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
        float f7); \
    float fn_name( \
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
        return target_fn( \
            g_pSinglePostEffect(), \
            a2, \
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

#define PFX_DEFINE_FORWARD_U8_ARG2(fn_name, target_fn) \
    uint64_t target_fn( \
        void* pPostEffect, \
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
        float f7); \
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

// ============================================================================
// Upper Range of Step 2: 0xbb3350 .. 0xbb3ff0
// ============================================================================

PFX_DEFINE_FORWARD(sub_bb3ff0, sub_bd4890)
PFX_DEFINE_FORWARD(sub_bb3fe0, sub_bf8f80)
PFX_DEFINE_FORWARD(sub_bb3fd0, sub_bf2590)
PFX_DEFINE_FORWARD(sub_bb3fc0, sub_bf90c0)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3fb0, sub_bf90a0)

// 0xbb3fa0 | 16 B
float sub_bb3fa0(void* /*this_ctx*/) {
    return *reinterpret_cast<float*>(g_pSinglePostEffectBytes() + 0x6f0);
}

PFX_DEFINE_FORWARD(sub_bb3f90, sub_bd4cd0)
PFX_DEFINE_FORWARD(sub_bb3f80, sub_bf9080)
PFX_DEFINE_FORWARD(sub_bb3f70, sub_bf0790)
PFX_DEFINE_FORWARD(sub_bb3f60, sub_bf05f0)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb3f50, sub_bf2bf0)
PFX_DEFINE_FORWARD(sub_bb3f40, sub_bf29a0)
PFX_DEFINE_FORWARD(sub_bb3f30, sub_bd4520)
PFX_DEFINE_FORWARD(sub_bb3f20, sub_bf37f0)

// 0xbb3ee0 | 49 B
uint64_t sub_bd4420(void* pPostEffect, uint32_t a2, void* rawRes);
uint64_t sub_bb3ee0(void* /*this_ctx*/, uint32_t a2, void* pResourceIf) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd4420(g_pSinglePostEffect(), a2, rawRes);
}

PFX_DEFINE_FORWARD(sub_bb3ed0, sub_bf3610)

// 0xbb3e90 | 49 B
uint64_t sub_bd4320(void* pPostEffect, uint32_t a2, void* rawRes);
uint64_t sub_bb3e90(void* /*this_ctx*/, uint32_t a2, void* pResourceIf) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd4320(g_pSinglePostEffect(), a2, rawRes);
}

PFX_DEFINE_FORWARD(sub_bb3e80, sub_bf3430)

// 0xbb3e40 | 49 B
uint64_t sub_bd4220(void* pPostEffect, uint32_t a2, void* rawRes);
uint64_t sub_bb3e40(void* /*this_ctx*/, uint32_t a2, void* pResourceIf) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd4220(g_pSinglePostEffect(), a2, rawRes);
}

PFX_DEFINE_FORWARD(sub_bb3e30, sub_bf8c20)

// 0xbb3e20 | 12 B (also jumps to sub_bf8c20)
uint64_t sub_bb3e20(
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
    return sub_bf8c20(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

// 0xbb3dc0 | 84 B
uint64_t sub_bd68e0(
    void* pPostEffect,
    uint32_t a2,
    void* rawRes,
    uint32_t zeroArg,
    uint32_t a4,
    float f0,
    float f1);
uint64_t sub_bb3dc0(
    void* /*this_ctx*/,
    uint32_t a2,
    void* pResourceIf,
    uint32_t a4,
    float f0,
    float f1) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd68e0(g_pSinglePostEffect(), a2, rawRes, 0, a4, f0, f1);
}

// 0xbb3d90 | 40 B
uint64_t sub_bd4210(void* pPostEffect, void* rawRes, uint32_t zeroArg);
uint64_t sub_bb3d90(void* /*this_ctx*/, void* pResourceIf) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd4210(g_pSinglePostEffect(), rawRes, 0);
}

// 0xbb3d80 | 15 B
uint64_t sub_bf30d0(
    void* pPostEffect,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint32_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7);
uint64_t sub_bb3d80(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bf30d0(g_pSinglePostEffect(), a2, a3, a4, a5, 0, f0, f1, f2, f3, f4, f5, f6, f7);
}

// 0xbb3d70 | 15 B
uint64_t sub_bb3d70(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bf30d0(g_pSinglePostEffect(), a2, a3, a4, a5, 0, f0, f1, f2, f3, f4, f5, f6, f7);
}

// 0xbb3d60 | 16 B
int32_t sub_bb3d60(void* /*this_ctx*/, uint32_t val) {
    *reinterpret_cast<uint32_t*>(g_pSinglePostEffectBytes() + 0x954) = val;
    return 0;
}

PFX_DEFINE_FORWARD(sub_bb3d50, sub_bf3b70)

// 0xbb3d40 | 12 B (also jumps to sub_bf3b70)
uint64_t sub_bb3d40(
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
    return sub_bf3b70(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

// 0xbb3d10 | 38 B
uint64_t sub_bf27c0(void* pPostEffect, void* rawRes);
uint64_t sub_bb3d10(void* /*this_ctx*/, void* pResourceIf) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bf27c0(g_pSinglePostEffect(), rawRes);
}

PFX_DEFINE_FORWARD(sub_bb3d00, sub_bd67f0)

// 0xbb3cf0 | 14 B
uint64_t sub_bf25c0(void* pPostEffect, uint32_t zeroArg, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_bb3cf0(void* /*this_ctx*/, uint64_t /*a2*/, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bf25c0(g_pSinglePostEffect(), 0, a3, a4, a5, a6);
}

PFX_DEFINE_FORWARD(sub_bb3ce0, sub_bd8270)

// 0xbb3cb0 | 44 B
uint64_t sub_bd80d0(void* pPostEffect, void* rawRes, uint32_t a3, void* a4);
uint64_t sub_bb3cb0(void* /*this_ctx*/, void* pResourceIf, uint32_t a3, void* a4) {
    void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
    return sub_bd80d0(g_pSinglePostEffect(), *pp, a3, a4);
}

// 0xbb3ca0 | 16 B
uint64_t sub_bf12a0(
    void* pPostEffect,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint32_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7);
uint64_t sub_bb3ca0(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint32_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bf12a0(
        g_pSinglePostEffect(),
        a2,
        a3,
        a4,
        a5,
        a6 & 0xffu,
        f0,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7);
}

// 0xbb3c90 | 16 B
uint64_t sub_bf1f20(
    void* pPostEffect,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint32_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7);
uint64_t sub_bb3c90(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint32_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bf1f20(
        g_pSinglePostEffect(),
        a2,
        a3,
        a4,
        a5,
        a6 & 0xffu,
        f0,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7);
}

// 0xbb3c80 | 16 B
uint64_t sub_bf0c20(
    void* pPostEffect,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint32_t a5,
    uint64_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7);
uint64_t sub_bb3c80(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint32_t a5,
    uint64_t a6,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bf0c20(
        g_pSinglePostEffect(),
        a2,
        a3,
        a4,
        a5 & 0xffu,
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

PFX_DEFINE_FORWARD(sub_bb3c70, sub_befb50)
PFX_DEFINE_FORWARD(sub_bb3c60, sub_be95e0)

// 0xbb3c50 | 12 B | IPfxBaseContext::ApplyEffects (Main PostEffect Render Entry)
PFX_DEFINE_FORWARD(sub_bb3c50, sub_bd82f0)

// 0xbb3c30 | 18 B
int32_t sub_bb3c30(void* /*this_ctx*/, float val) {
    *reinterpret_cast<float*>(g_pSinglePostEffectBytes() + 0xc40) = val;
    return 0;
}

// 0xbb3c10 | 18 B
int32_t sub_bb3c10(void* /*this_ctx*/, float val) {
    *reinterpret_cast<float*>(g_pSinglePostEffectBytes() + 0xc3c) = val;
    return 0;
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3c00, sub_bd3a20)
PFX_DEFINE_FORWARD(sub_bb3bf0, sub_bd3a00)

// 0xbb3bd0 | 25 B
int32_t sub_bb3bd0(void* /*this_ctx*/, uint8_t flag, float val) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    pPost[0xaa4] = flag;
    *reinterpret_cast<float*>(pPost + 0xab4) = val;
    return 0;
}

// 0xbb3ba0 | 34 B
int32_t sub_bb3ba0(void* /*this_ctx*/, float f0, float f1, float f2) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    *reinterpret_cast<float*>(pPost + 0xaa8) = f0;
    *reinterpret_cast<float*>(pPost + 0xaac) = f1;
    *reinterpret_cast<float*>(pPost + 0xab0) = f2;
    return 0;
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3b90, sub_bd3ad0)

// 0xbb3b70 | 32 B
int32_t sub_bb3b70(void* /*this_ctx*/, uint32_t mode, float f0, float f1) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    *reinterpret_cast<uint32_t*>(pPost + 0xa98) = mode;
    *reinterpret_cast<float*>(pPost + 0xa9c) = f0;
    *reinterpret_cast<float*>(pPost + 0xaa0) = f1;
    return 0;
}

// 0xbb3b20 | 76 B
int32_t sub_bb3b20(
    void* /*this_ctx*/,
    const uint64_t* a2,
    const uint64_t* a3,
    const uint8_t* a4,
    float f0,
    float f1) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    *reinterpret_cast<float*>(pPost + 0xa6c) = f0;
    *reinterpret_cast<float*>(pPost + 0xa70) = f1;
    uint64_t v2_0 = a2[0];
    uint64_t v2_1 = a2[1];
    *reinterpret_cast<uint64_t*>(pPost + 0xa7c) = v2_1;
    *reinterpret_cast<uint64_t*>(pPost + 0xa74) = v2_0;
    *reinterpret_cast<uint64_t*>(pPost + 0xa84) = a3[0];
    *reinterpret_cast<uint32_t*>(pPost + 0xa94) = *reinterpret_cast<const uint32_t*>(a4 + 0x8);
    *reinterpret_cast<uint64_t*>(pPost + 0xa8c) = *reinterpret_cast<const uint64_t*>(a4 + 0x0);
    return 0;
}

// 0xbb3b00 | 29 B
int32_t sub_bb3b00(void* /*this_ctx*/, uint32_t a2, uint64_t a3, uint8_t a4) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    *reinterpret_cast<uint32_t*>(pPost + 0xa5c) = a2;
    *reinterpret_cast<uint64_t*>(pPost + 0xa60) = a3;
    pPost[0xa68] = a4;
    return 0;
}

// 0xbb3af0 | 14 B
uint8_t sub_bb3af0(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0xa58];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3ae0, sub_bd3a50)
PFX_DEFINE_FORWARD(sub_bb3ad0, sub_bd2ef0)
PFX_DEFINE_FORWARD(sub_bb3ac0, sub_bd2ec0)
PFX_DEFINE_FORWARD(sub_bb3ab0, sub_bd2d20)

// 0xbb3aa0 | 14 B
uint8_t sub_bb3aa0(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0xb8c];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3a90, sub_bd2f00)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3a80, sub_bd2cb0)

// 0xbb3a40 | 64 B
uint64_t sub_bdac30(
    void* pPostEffect,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    uint32_t a7,
    uint32_t a8,
    uint32_t a9,
    uint64_t a10,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7);
uint64_t sub_bb3a40(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    uint32_t a7,
    uint8_t a8,
    uint32_t a9,
    uint64_t a10,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bdac30(
        g_pSinglePostEffect(),
        a2,
        a3,
        a4,
        a5,
        a6,
        a7,
        static_cast<uint32_t>(a8),
        a9,
        a10,
        f0,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7);
}

// 0xbb39f0 | 75 B
uint64_t sub_bd3e20(
    void* pPostEffect,
    uint32_t a2,
    void* rawRes,
    uint32_t a4,
    uint32_t a5,
    uint32_t a6);
uint64_t sub_bb39f0(
    void* /*this_ctx*/,
    uint32_t a2,
    void* pResourceIf,
    uint32_t a4,
    uint32_t a5,
    uint32_t a6) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd3e20(g_pSinglePostEffect(), a2, rawRes, a4, a5 & 0xffu, a6);
}

PFX_DEFINE_FORWARD(sub_bb39e0, sub_bd3060)
PFX_DEFINE_FORWARD(sub_bb39d0, sub_bd2f80)

// 0xbb39c0 | 14 B
uint8_t sub_bb39c0(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0xb60];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb39b0, sub_bd2f10)
PFX_DEFINE_FORWARD(sub_bb39a0, sub_bd3c30)

// 0xbb3960 | 53 B
uint64_t sub_bda460(
    void* pPostEffect,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    uint32_t a7,
    uint64_t a8,
    uint32_t a9,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7);
uint64_t sub_bb3960(
    void* /*this_ctx*/,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    uint32_t a7,
    uint64_t a8,
    uint8_t a9,
    float f0,
    float f1,
    float f2,
    float f3,
    float f4,
    float f5,
    float f6,
    float f7) {
    return sub_bda460(
        g_pSinglePostEffect(),
        a2,
        a3,
        a4,
        a5,
        a6,
        a7,
        a8,
        static_cast<uint32_t>(a9),
        f0,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7);
}

// 0xbb3910 | 75 B
uint64_t sub_bd3c50(
    void* pPostEffect,
    uint32_t a2,
    void* rawRes,
    uint32_t a4,
    void* a5,
    uint32_t a6);
uint64_t sub_bb3910(
    void* /*this_ctx*/,
    uint32_t a2,
    void* pResourceIf,
    uint32_t a4,
    void* a5,
    uint32_t a6) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd3c50(g_pSinglePostEffect(), a2, rawRes, a4, a5, a6 & 0xffu);
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3900, sub_bd3be0)
PFX_DEFINE_FORWARD(sub_bb38f0, sub_bd3b90)
PFX_DEFINE_FORWARD(sub_bb38e0, sub_bd3c10)

// 0xbb38d0 | 14 B
uint8_t sub_bb38d0(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x9e8];
}

// 0xbb38b0 | 17 B
int32_t sub_bb38b0(void* /*this_ctx*/, uint8_t val) {
    g_pSinglePostEffectBytes()[0x9e8] = val;
    return 0;
}

// 0xbb38a0 | 14 B
uint32_t sub_bb38a0(void* /*this_ctx*/) {
    return *reinterpret_cast<uint32_t*>(g_pSinglePostEffectBytes() + 0x9e4);
}

PFX_DEFINE_FORWARD(sub_bb3890, sub_bd3b80)

// 0xbb3880 | 14 B
uint8_t sub_bb3880(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x9e1];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3870, sub_bd3b00)
PFX_DEFINE_FORWARD(sub_bb3860, sub_bd31b0)
PFX_DEFINE_FORWARD(sub_bb3850, sub_bd3190)

// 0xbb3840 | 14 B
uint8_t sub_bb3840(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x9c6];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3830, sub_bd3120)

// 0xbb3820 | 14 B
uint64_t sub_bd3270(void* pPostEffect, uint32_t zeroArg, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_bb3820(void* /*this_ctx*/, uint64_t /*a2*/, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bd3270(g_pSinglePostEffect(), 0, a3, a4, a5, a6);
}

// 0xbb3810 | 14 B
uint8_t sub_bb3810(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0xbbc];
}

// 0xbb37f0 | 18 B
uint64_t sub_bd31f0(void* pPostEffect, uint32_t a2, uint32_t zeroArg);
uint64_t sub_bb37f0(void* /*this_ctx*/, uint32_t a2) {
    return sub_bd31f0(g_pSinglePostEffect(), a2 & 0xffu, 0);
}

// 0xbb37d0 | 23 B
uint64_t sub_bd33a0(void* pPostEffect, uint64_t a2, uint32_t zeroArg, uint64_t a3, uint64_t a4);
uint64_t sub_bb37d0(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4) {
    return sub_bd33a0(g_pSinglePostEffect(), a2, 0, a3, a4);
}

// 0xbb37c0 | 14 B
uint8_t sub_bb37c0(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x9a0];
}

// 0xbb37a0 | 18 B
uint64_t sub_bd3320(void* pPostEffect, uint32_t a2, uint32_t zeroArg);
uint64_t sub_bb37a0(void* /*this_ctx*/, uint32_t a2) {
    return sub_bd3320(g_pSinglePostEffect(), a2 & 0xffu, 0);
}

// 0xbb3780 | 21 B
uint64_t sub_bd2c90(void* pPostEffect, uint32_t a2, uint32_t oneArg);
uint64_t sub_bb3780(void* /*this_ctx*/, uint32_t a2) {
    return sub_bd2c90(g_pSinglePostEffect(), a2 & 0xffu, 1);
}

// 0xbb3770 | 14 B
uint8_t sub_bb3770(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x9bd];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3760, sub_bd2c20)
PFX_DEFINE_FORWARD(sub_bb3750, sub_bd2bf0)
PFX_DEFINE_FORWARD(sub_bb3740, sub_bda0a0)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3730, sub_bd2b80)
PFX_DEFINE_FORWARD(sub_bb3720, sub_bda150)
PFX_DEFINE_FORWARD(sub_bb3710, sub_bda130)
PFX_DEFINE_FORWARD(sub_bb3700, sub_bda0f0)
PFX_DEFINE_FORWARD(sub_bb36f0, sub_bda0d0)
PFX_DEFINE_FORWARD(sub_bb36e0, sub_bda0b0)
PFX_DEFINE_FORWARD(sub_bb36d0, sub_bda090)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb36c0, sub_bd2ab0)

// 0xbb36a0 | 19 B
uint64_t sub_bd9470(void* pPostEffect, uint32_t zeroArg, uint32_t oneArg);
uint64_t sub_bb36a0(void* /*this_ctx*/) {
    return sub_bd9470(g_pSinglePostEffect(), 0, 1);
}

// 0xbb3680 | 21 B
uint64_t sub_bd93e0(void* pPostEffect, uint32_t a2, uint32_t a3, uint32_t zeroArg);
uint64_t sub_bb3680(void* /*this_ctx*/, uint32_t a2, uint32_t a3) {
    return sub_bd93e0(g_pSinglePostEffect(), a2 & 0xffu, a3 & 0xffu, 0);
}

PFX_DEFINE_FORWARD(sub_bb3670, sub_bd4f70)
PFX_DEFINE_FORWARD(sub_bb3660, sub_bd4670)
PFX_DEFINE_FORWARD(sub_bb3650, sub_bf39c0)

// 0xbb3620 | 38 B
uint64_t sub_bd4660(void* pPostEffect, void* rawRes);
uint64_t sub_bb3620(void* /*this_ctx*/, void* pResourceIf) {
    void* rawRes = nullptr;
    if (pResourceIf != nullptr) {
        void** pp = reinterpret_cast<void**>(sub_bb6e50(pResourceIf));
        rawRes = *pp;
    }
    return sub_bd4660(g_pSinglePostEffect(), rawRes);
}

// 0xbb3610 | 11 B
uint8_t sub_bb3610(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x70];
}

PFX_DEFINE_FORWARD_U8_ARG2(sub_bb3600, sub_bd4650)

// 0xbb35f0 | 14 B
uint64_t sub_bd4640(void* pPostEffect, uint32_t zeroArg);
uint64_t sub_bb35f0(void* /*this_ctx*/) {
    return sub_bd4640(g_pSinglePostEffect(), 0);
}

PFX_DEFINE_FORWARD(sub_bb35e0, sub_bf3280)

// 0xbb35d0 | 14 B
uint8_t sub_bb35d0(void* /*this_ctx*/) {
    return g_pSinglePostEffectBytes()[0x108];
}

// 0xbb35b0 | 18 B
uint64_t sub_bd45c0(void* pPostEffect, uint32_t a2, uint32_t zeroArg);
uint64_t sub_bb35b0(void* /*this_ctx*/, uint32_t a2) {
    return sub_bd45c0(g_pSinglePostEffect(), a2 & 0xffu, 0);
}

PFX_DEFINE_FORWARD(sub_bb35a0, sub_bd4e90)
PFX_DEFINE_FORWARD(sub_bb3590, sub_bde480)
PFX_DEFINE_FORWARD(sub_bb3580, sub_bd4f60)
PFX_DEFINE_FORWARD(sub_bb3570, sub_bd4f50)
PFX_DEFINE_FORWARD(sub_bb3560, sub_bd4e60)
PFX_DEFINE_FORWARD(sub_bb3550, sub_bd4d40)
PFX_DEFINE_FORWARD(sub_bb3540, sub_bd4ec0)
PFX_DEFINE_FORWARD(sub_bb3530, sub_bde7a0)
PFX_DEFINE_FORWARD(sub_bb3520, sub_bde4b0)
PFX_DEFINE_FORWARD(sub_bb3510, sub_bde430)
PFX_DEFINE_FORWARD(sub_bb3500, sub_bd4180)

// 0xbb34e0 | 18 B
int32_t sub_bb34e0(void* /*this_ctx*/, float val) {
    *reinterpret_cast<float*>(g_pSinglePostEffectBytes() + 0x950) = val;
    return 0;
}

PFX_DEFINE_FORWARD(sub_bb34d0, sub_bd4150)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb34c0, sub_bd40e0)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb34b0, sub_bd5d50)
PFX_DEFINE_FORWARD(sub_bb34a0, sub_bd4d20)
PFX_DEFINE_FORWARD(sub_bb3490, sub_bd6250)
PFX_DEFINE_FORWARD(sub_bb3480, sub_bd60a0)
PFX_DEFINE_FORWARD(sub_bb3470, sub_bd5f10)
PFX_DEFINE_FORWARD_F32(sub_bb3460, sub_bdecb0)

// 0xbb33e0 | 119 B | IPfxBaseContext::GetViewParamFloat34 (uses GetEffectiveViewIndex)
float sub_bb33e0(void* /*this_ctx*/, int32_t iEffectViewIndex) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    if (iEffectViewIndex == 0x7fffffff) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe8b5ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        iEffectViewIndex = 0x7fffffff;
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
    } else {
        if (iEffectViewIndex == -1) {
            iEffectViewIndex = *reinterpret_cast<int32_t*>(pPost + 0x200);
        }
        if (iEffectViewIndex < 0 || iEffectViewIndex >= *reinterpret_cast<int32_t*>(pPost + 0x320)) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        }
    }
    uint8_t* pViews = *reinterpret_cast<uint8_t**>(pPost + 0x210);
    return *reinterpret_cast<float*>(pViews + static_cast<int64_t>(iEffectViewIndex) * 72 + 0x34);
}

// 0xbb3360 | 116 B | IPfxBaseContext::GetViewEnabledFlag (uses GetEffectiveViewIndex)
uint8_t sub_bb3360(void* /*this_ctx*/, int32_t iEffectViewIndex) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    if (iEffectViewIndex == 0x7fffffff) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe8b5ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        iEffectViewIndex = 0x7fffffff;
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
    } else {
        if (iEffectViewIndex == -1) {
            iEffectViewIndex = *reinterpret_cast<int32_t*>(pPost + 0x200);
        }
        if (iEffectViewIndex < 0 || iEffectViewIndex >= *reinterpret_cast<int32_t*>(pPost + 0x320)) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        }
    }
    uint8_t* pViews = *reinterpret_cast<uint8_t**>(pPost + 0x210);
    return pViews[static_cast<int64_t>(iEffectViewIndex) * 72];
}

PFX_DEFINE_FORWARD_U8_ARG3(sub_bb3350, sub_bd5be0)

// ============================================================================
// Lower Range of Step 2: 0xbb2600 .. 0xbb3340
// ============================================================================

PFX_DEFINE_FORWARD(sub_bb3340, sub_bdee90)
PFX_DEFINE_FORWARD(sub_bb3330, sub_bd7700)
PFX_DEFINE_FORWARD(sub_bb3320, sub_bd5130)
PFX_DEFINE_FORWARD(sub_bb3310, sub_bd56f0)
PFX_DEFINE_FORWARD(sub_bb3300, sub_bd5870)
PFX_DEFINE_FORWARD(sub_bb32f0, sub_bd5550)
PFX_DEFINE_FORWARD(sub_bb32e0, sub_bd53c0)
PFX_DEFINE_FORWARD_F32(sub_bb32d0, sub_bdec30)

// 0xbb3250 | 119 B | IPfxBaseContext::GetViewSubParamFloat54 (uses GetEffectiveViewIndex)
float sub_bb3250(void* /*this_ctx*/, int32_t iEffectViewIndex) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    if (iEffectViewIndex == 0x7fffffff) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe8b5ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        iEffectViewIndex = 0x7fffffff;
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
    } else {
        if (iEffectViewIndex == -1) {
            iEffectViewIndex = *reinterpret_cast<int32_t*>(pPost + 0x200);
        }
        if (iEffectViewIndex < 0 || iEffectViewIndex >= *reinterpret_cast<int32_t*>(pPost + 0x320)) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        }
    }
    uint8_t* pViewArray = *reinterpret_cast<uint8_t**>(pPost + 0x208);
    return *reinterpret_cast<float*>(pViewArray + static_cast<int64_t>(iEffectViewIndex) * 0x68 + 0x54);
}

// 0xbb31d0 | 116 B | IPfxBaseContext::GetViewSubParamEnabledFlag (uses GetEffectiveViewIndex)
uint8_t sub_bb31d0(void* /*this_ctx*/, int32_t iEffectViewIndex) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    if (iEffectViewIndex == 0x7fffffff) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe8b5ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        iEffectViewIndex = 0x7fffffff;
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
    } else {
        if (iEffectViewIndex == -1) {
            iEffectViewIndex = *reinterpret_cast<int32_t*>(pPost + 0x200);
        }
        if (iEffectViewIndex < 0 || iEffectViewIndex >= *reinterpret_cast<int32_t*>(pPost + 0x320)) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe965ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fe94fULL)));
        }
    }
    uint8_t* pViewArray = *reinterpret_cast<uint8_t**>(pPost + 0x208);
    return pViewArray[static_cast<int64_t>(iEffectViewIndex) * 0x68];
}

PFX_DEFINE_FORWARD_U8_ARG3(sub_bb31c0, sub_bd4fc0)
PFX_DEFINE_FORWARD(sub_bb31b0, sub_bded30)
PFX_DEFINE_FORWARD(sub_bb31a0, sub_bd74d0)
PFX_DEFINE_FORWARD(sub_bb3190, sub_bc66d0)
PFX_DEFINE_FORWARD(sub_bb3180, sub_bdd080)
PFX_DEFINE_FORWARD(sub_bb3170, sub_bdd050)
PFX_DEFINE_FORWARD_F32(sub_bb3160, sub_bdcff0)
PFX_DEFINE_FORWARD_F32(sub_bb3150, sub_bdcf90)
PFX_DEFINE_FORWARD_F32(sub_bb3140, sub_bdcf00)
PFX_DEFINE_FORWARD_F32(sub_bb3130, sub_bdcec0)
PFX_DEFINE_FORWARD_F32(sub_bb3120, sub_bdce80)
PFX_DEFINE_FORWARD_F32(sub_bb3110, sub_bdcf40)
PFX_DEFINE_FORWARD_F32(sub_bb3100, sub_bdce40)
PFX_DEFINE_FORWARD_F32(sub_bb30f0, sub_bdcdc0)
PFX_DEFINE_FORWARD_F32(sub_bb30e0, sub_bdcd80)
PFX_DEFINE_FORWARD_F32(sub_bb30d0, sub_bdcd40)
PFX_DEFINE_FORWARD_F32(sub_bb30c0, sub_bdcc80)
PFX_DEFINE_FORWARD_F32(sub_bb30b0, sub_bdcc40)
PFX_DEFINE_FORWARD_F32(sub_bb30a0, sub_bdcd00)
PFX_DEFINE_FORWARD_F32(sub_bb3090, sub_bdccc0)
PFX_DEFINE_FORWARD_F32(sub_bb3080, sub_bdcc00)
PFX_DEFINE_FORWARD_F32(sub_bb3070, sub_bdcbc0)
PFX_DEFINE_FORWARD_F32(sub_bb3060, sub_bdcb80)
PFX_DEFINE_FORWARD_F32(sub_bb3050, sub_bdcb10)
PFX_DEFINE_FORWARD_F32(sub_bb3040, sub_bdcaa0)
PFX_DEFINE_FORWARD(sub_bb3030, sub_bdc2e0)
PFX_DEFINE_FORWARD(sub_bb3020, sub_bdca20)
PFX_DEFINE_FORWARD(sub_bb3010, sub_bdc9e0)
PFX_DEFINE_FORWARD(sub_bb3000, sub_bdc9a0)
PFX_DEFINE_FORWARD(sub_bb2ff0, sub_bdc960)
PFX_DEFINE_FORWARD(sub_bb2fe0, sub_bdc920)
PFX_DEFINE_FORWARD(sub_bb2fd0, sub_bdc8e0)
PFX_DEFINE_FORWARD(sub_bb2fc0, sub_bdc860)

// 0xbb2dc0 | 511 B | IPfxBaseContext::ConvertDepthValue
float sub_bb2dc0(void* /*this_ctx*/, uint32_t srcType, uint32_t dstType, float depthVal) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    if (srcType == 7) {
        float nearZ = *reinterpret_cast<float*>(pPost + 0x57c);
        float farZ = *reinterpret_cast<float*>(pPost + 0x580);
        depthVal = ((farZ * (depthVal - nearZ)) / (farZ - nearZ)) / depthVal;
        srcType = 0;
    }

    uint32_t effDstType = (dstType != 7) ? dstType : 0;
    float invScaleOff[2];
    float fwdScaleOff[2];

    if (*reinterpret_cast<void**>(pPost + 0x448) != nullptr ||
        *reinterpret_cast<void**>(pPost + 0x50) != nullptr) {
        if (*reinterpret_cast<uint32_t*>(pPost + 0x450) == 8) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe9d8ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fea3fULL)));
            pPost = g_pSinglePostEffectBytes();
        }
        std::memcpy(invScaleOff, pPost + 0x458, sizeof(invScaleOff));
        std::memcpy(fwdScaleOff, pPost + 0x460, sizeof(fwdScaleOff));
    } else {
        fwdScaleOff[0] = 1.0f;
        fwdScaleOff[1] = 0.0f;
        invScaleOff[0] = 1.0f;
        invScaleOff[1] = 0.0f;
    }

    float conv[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int32_t hr = sub_bdbe90(pPost, conv, srcType, effDstType, 1.0f, 0.0f);
    if (hr >= 0) {
        if (srcType == 6 && (fwdScaleOff[0] != 1.0f || fwdScaleOff[1] != 0.0f)) {
            float c0 = conv[0];
            conv[1] = fwdScaleOff[1] * c0 + conv[1];
            conv[0] = fwdScaleOff[0] * c0;
        }
        if (effDstType == 6 && (invScaleOff[0] != 1.0f || invScaleOff[1] != 0.0f)) {
            float new0 = invScaleOff[0] * conv[0];
            conv[1] = invScaleOff[1] + invScaleOff[0] * conv[1];
            conv[0] = new0;
        }
    }

    float result = depthVal * conv[0] + conv[1];
    if (dstType == 7) {
        uint8_t* pPostAfter = g_pSinglePostEffectBytes();
        float nearZ = *reinterpret_cast<float*>(pPostAfter + 0x57c);
        float farZ = *reinterpret_cast<float*>(pPostAfter + 0x580);
        float negNearFar = -(nearZ * farZ);
        result = negNearFar / (result * (farZ - nearZ) - farZ);
    }
    return result;
}

// 0xbb2c20 | 407 B | IPfxBaseContext::GetDepthConversionFactor
int32_t sub_bb2c20(
    void* /*this_ctx*/,
    float* afOutDepthConversionFactor,
    uint32_t srcType,
    uint32_t dstType) {
    if (afOutDepthConversionFactor == nullptr) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fce96ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fcefcULL)));
    }

    uint8_t* pPost = g_pSinglePostEffectBytes();
    float invScaleOff[2];
    float fwdScaleOff[2];

    if (*reinterpret_cast<void**>(pPost + 0x448) != nullptr ||
        *reinterpret_cast<void**>(pPost + 0x50) != nullptr) {
        if (*reinterpret_cast<uint32_t*>(pPost + 0x450) == 8) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe9d8ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fea3fULL)));
            pPost = g_pSinglePostEffectBytes();
        }
        std::memcpy(invScaleOff, pPost + 0x458, sizeof(invScaleOff));
        std::memcpy(fwdScaleOff, pPost + 0x460, sizeof(fwdScaleOff));
    } else {
        fwdScaleOff[0] = 1.0f;
        fwdScaleOff[1] = 0.0f;
        invScaleOff[0] = 1.0f;
        invScaleOff[1] = 0.0f;
    }

    float conv[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int32_t hr = sub_bdbe90(pPost, conv, srcType, dstType, 1.0f, 0.0f);
    if (hr >= 0) {
        if (srcType == 6 && (fwdScaleOff[0] != 1.0f || fwdScaleOff[1] != 0.0f)) {
            float c0 = conv[0];
            conv[1] = fwdScaleOff[1] * c0 + conv[1];
            conv[0] = fwdScaleOff[0] * c0;
        }
        if (dstType == 6 && (invScaleOff[0] != 1.0f || invScaleOff[1] != 0.0f)) {
            float new0 = invScaleOff[0] * conv[0];
            conv[1] = invScaleOff[1] + invScaleOff[0] * conv[1];
            conv[0] = new0;
        }
        std::memcpy(afOutDepthConversionFactor, conv, 16);
    }
    return hr;
}

// 0xbb2a90 | 387 B | IPfxBaseContext::GetDepthConversionFactorValue
PfxDepthFactorVec4 sub_bb2a90(void* /*this_ctx*/, uint32_t srcType, uint32_t dstType) {
    uint8_t* pPost = g_pSinglePostEffectBytes();
    float invScaleOff[2];
    float fwdScaleOff[2];

    if (*reinterpret_cast<void**>(pPost + 0x448) != nullptr ||
        *reinterpret_cast<void**>(pPost + 0x50) != nullptr) {
        if (*reinterpret_cast<uint32_t*>(pPost + 0x450) == 8) {
            sub_2bbf178(
                reinterpret_cast<const char*>(GX_VMA(0x47fe9d8ULL)),
                reinterpret_cast<const char*>(GX_VMA(0x47fea3fULL)));
            pPost = g_pSinglePostEffectBytes();
        }
        std::memcpy(invScaleOff, pPost + 0x458, sizeof(invScaleOff));
        std::memcpy(fwdScaleOff, pPost + 0x460, sizeof(fwdScaleOff));
    } else {
        fwdScaleOff[0] = 1.0f;
        fwdScaleOff[1] = 0.0f;
        invScaleOff[0] = 1.0f;
        invScaleOff[1] = 0.0f;
    }

    PfxDepthFactorVec4 out = {0.0f, 0.0f, 0.0f, 0.0f};
    int32_t hr = sub_bdbe90(pPost, &out.x, srcType, dstType, 1.0f, 0.0f);
    if (hr >= 0) {
        if (srcType == 6 && (fwdScaleOff[0] != 1.0f || fwdScaleOff[1] != 0.0f)) {
            float c0 = out.x;
            out.y = fwdScaleOff[1] * c0 + out.y;
            out.x = fwdScaleOff[0] * c0;
        }
        if (dstType == 6 && (invScaleOff[0] != 1.0f || invScaleOff[1] != 0.0f)) {
            float new0 = invScaleOff[0] * out.x;
            out.y = invScaleOff[1] + invScaleOff[0] * out.y;
            out.x = new0;
        }
    }
    return out;
}

PFX_DEFINE_FORWARD_F32(sub_bb2a80, sub_bdc2b0)

// 0xbb2a40 | 54 B | IPfxBaseContext::GetDepthFactorScaleOffset
uint64_t sub_bdbd40(void* pPostEffect, float* afOutDepthFactorScaleOffset);
uint64_t sub_bb2a40(void* /*this_ctx*/, float* afOutDepthFactorScaleOffset) {
    if (afOutDepthFactorScaleOffset == nullptr) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fce15ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fce7cULL)));
    }
    return sub_bdbd40(g_pSinglePostEffect(), afOutDepthFactorScaleOffset);
}

PFX_DEFINE_FORWARD(sub_bb2a30, sub_bdc430)

// 0xbb29f0 | 54 B | IPfxBaseContext::GetDepthOfFieldFactorScaleOffset
uint64_t sub_bdb000(void* pPostEffect, float* afOutDepthOfFieldFactorScaleOffset);
uint64_t sub_bb29f0(void* /*this_ctx*/, float* afOutDepthOfFieldFactorScaleOffset) {
    if (afOutDepthOfFieldFactorScaleOffset == nullptr) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fcd86ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fcdf4ULL)));
    }
    return sub_bdb000(g_pSinglePostEffect(), afOutDepthOfFieldFactorScaleOffset);
}

PFX_DEFINE_FORWARD(sub_bb29e0, sub_bdbd00)
PFX_DEFINE_FORWARD(sub_bb29d0, sub_bdc6f0)
PFX_DEFINE_FORWARD(sub_bb29c0, sub_bdc5e0)

// 0xbb29b0 | 12 B (also jumps to sub_bdc5e0)
uint64_t sub_bb29b0(
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
    return sub_bdc5e0(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

PFX_DEFINE_FORWARD(sub_bb29a0, sub_bdc570)
PFX_DEFINE_FORWARD(sub_bb2990, sub_bdc500)
PFX_DEFINE_FORWARD(sub_bb2980, sub_bd4c60)
PFX_DEFINE_FORWARD(sub_bb2970, sub_bdc470)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb2960, sub_bd2a90)
PFX_DEFINE_FORWARD(sub_bb2950, sub_bd6740)
PFX_DEFINE_FORWARD(sub_bb2940, sub_bd9f00)
PFX_DEFINE_FORWARD(sub_bb2930, sub_bd3980)
PFX_DEFINE_FORWARD(sub_bb2920, sub_bd9dc0)
PFX_DEFINE_FORWARD(sub_bb2910, sub_bd37c0)
PFX_DEFINE_FORWARD(sub_bb2900, sub_bd9c50)
PFX_DEFINE_FORWARD(sub_bb28f0, sub_bd3520)
PFX_DEFINE_FORWARD(sub_bb28e0, sub_bd9c00)
PFX_DEFINE_FORWARD_U8_ARG3(sub_bb28d0, sub_bd9ac0)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb28c0, sub_bd3420)

// 0xbb28b0 | 14 B
uint32_t sub_bb28b0(void* /*this_ctx*/) {
    return *reinterpret_cast<uint32_t*>(g_pSinglePostEffectBytes() + 0x6d0);
}

PFX_DEFINE_FORWARD(sub_bb28a0, sub_bd33d0)

// 0xbb2890 | 14 B
uint32_t sub_bb2890(void* /*this_ctx*/) {
    return *reinterpret_cast<uint32_t*>(g_pSinglePostEffectBytes() + 0x390);
}

PFX_DEFINE_FORWARD(sub_bb2880, sub_bd63f0)
PFX_DEFINE_FORWARD(sub_bb2870, sub_bd9500)
PFX_DEFINE_FORWARD(sub_bb2860, sub_bd94d0)
PFX_DEFINE_FORWARD(sub_bb2850, sub_bd4c10)
PFX_DEFINE_FORWARD(sub_bb2840, sub_bd4b60)
PFX_DEFINE_FORWARD(sub_bb2830, sub_bd4b10)
PFX_DEFINE_FORWARD(sub_bb2820, sub_bd86a0)

// 0xbb2800 | 18 B
int32_t sub_bb2800(void* /*this_ctx*/, float val) {
    *reinterpret_cast<float*>(g_pSinglePostEffectBytes() + 0x6ac) = val;
    return 0;
}

extern "C" __attribute__((weak)) void BbGoldenEngine_OnYebisCameraMatrix(uint32_t kind, const float* mtx16);
extern "C" __attribute__((weak)) void BbGoldenEngine_OnYebisCameraParams(float near_z, float far_z, float fov_y);

uint64_t sub_bd90f0(void* pPostEffect, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bb27f0(void* /*this_ctx*/, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7) {
    if (a2 && BbGoldenEngine_OnYebisCameraMatrix) BbGoldenEngine_OnYebisCameraMatrix(3u, reinterpret_cast<const float*>(a2));
    return sub_bd90f0(g_pSinglePostEffect(), a2, a3 & 0xffu, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

uint64_t sub_bd8eb0(void* pPostEffect, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bb27e0(void* /*this_ctx*/, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7) {
    if (a2 && BbGoldenEngine_OnYebisCameraMatrix) BbGoldenEngine_OnYebisCameraMatrix(2u, reinterpret_cast<const float*>(a2));
    return sub_bd8eb0(g_pSinglePostEffect(), a2, a3 & 0xffu, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

uint64_t sub_bd8cb0(void* pPostEffect, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bb27d0(void* /*this_ctx*/, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7) {
    if (a2 && BbGoldenEngine_OnYebisCameraMatrix) BbGoldenEngine_OnYebisCameraMatrix(1u, reinterpret_cast<const float*>(a2));
    return sub_bd8cb0(g_pSinglePostEffect(), a2, a3 & 0xffu, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

uint64_t sub_bd8a70(void* pPostEffect, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bb27c0(void* /*this_ctx*/, uint64_t a2, uint32_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7) {
    if (a2 && BbGoldenEngine_OnYebisCameraMatrix) BbGoldenEngine_OnYebisCameraMatrix(0u, reinterpret_cast<const float*>(a2));
    return sub_bd8a70(g_pSinglePostEffect(), a2, a3 & 0xffu, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

PFX_DEFINE_FORWARD_F32(sub_bb27b0, sub_bd8920)
PFX_DEFINE_FORWARD_F32(sub_bb27a0, sub_bd8910)
PFX_DEFINE_FORWARD_F32(sub_bb2790, sub_bd8900)
PFX_DEFINE_FORWARD_F32(sub_bb2780, sub_bd88f0)
PFX_DEFINE_FORWARD_F32(sub_bb2770, sub_bd88e0)
PFX_DEFINE_FORWARD_F32(sub_bb2760, sub_bd88d0)
PFX_DEFINE_FORWARD(sub_bb2750, sub_bd8800)
PFX_DEFINE_FORWARD(sub_bb2740, sub_bd4a40)
PFX_DEFINE_FORWARD(sub_bb2730, sub_bd8730)

uint64_t sub_bd85d0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bb2720(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7) {
    if (BbGoldenEngine_OnYebisCameraParams) BbGoldenEngine_OnYebisCameraParams(f0, f1, f2);
    return sub_bd85d0(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

uint64_t sub_bd48d0(void* pPostEffect, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
uint64_t sub_bb2710(void* /*this_ctx*/, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7) {
    if (BbGoldenEngine_OnYebisCameraParams) BbGoldenEngine_OnYebisCameraParams(f0, f1, f2);
    return sub_bd48d0(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}
PFX_DEFINE_FORWARD(sub_bb2700, sub_bd79e0)
PFX_DEFINE_FORWARD(sub_bb26f0, sub_bd7e20)
PFX_DEFINE_FORWARD(sub_bb26e0, sub_bd8040)
PFX_DEFINE_FORWARD(sub_bb26d0, sub_bd7d70)

// 0xbb26c0 | 12 B (also jumps to sub_bd7d70)
uint64_t sub_bb26c0(
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
    return sub_bd7d70(g_pSinglePostEffect(), a2, a3, a4, a5, a6, f0, f1, f2, f3, f4, f5, f6, f7);
}

// 0xbb26a0 | 17 B
uint64_t sub_bd98d0(void* pPostEffect, uint32_t oneArg, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
uint64_t sub_bb26a0(void* /*this_ctx*/, uint64_t /*a2*/, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    return sub_bd98d0(g_pSinglePostEffect(), 1, a3, a4, a5, a6);
}

PFX_DEFINE_FORWARD(sub_bb2690, sub_bd66c0)
PFX_DEFINE_FORWARD_U8_ARG2(sub_bb2680, sub_bd97a0)
PFX_DEFINE_FORWARD(sub_bb2670, sub_bd6450)

// 0xbb2660 | 14 B | IPfxBaseContext::GetNumViews
uint32_t sub_bb2660(void* /*this_ctx*/) {
    return *reinterpret_cast<uint32_t*>(g_pSinglePostEffectBytes() + 0x320);
}

// 0xbb2650 | 14 B | IPfxBaseContext::GetCurrentViewIndex
uint32_t sub_bb2650(void* /*this_ctx*/) {
    return *reinterpret_cast<uint32_t*>(g_pSinglePostEffectBytes() + 0x200);
}

PFX_DEFINE_FORWARD(sub_bb2640, sub_bd2990)

// 0xbb2610 | 42 B | IPfxBaseContext::GetMotionBlurInternalVelocityTexture
void* sub_bb2610(void* /*this_ctx*/) {
    if (g_pSinglePostEffect() == nullptr) {
        sub_2bbf178(
            reinterpret_cast<const char*>(GX_VMA(0x47fcd02ULL)),
            reinterpret_cast<const char*>(GX_VMA(0x47fcd61ULL)));
    }
    return *reinterpret_cast<void**>(GX_VMA(0x5465ee0ULL));
}

PFX_DEFINE_FORWARD(sub_bb2600, sub_bc8ad0)

} // extern "C"
