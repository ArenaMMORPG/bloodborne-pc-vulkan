// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Phase 4 GPU Shader Decompiled Library &
// Complete 6-Bundle Resource Binding Manifest
//
// File: decolied files/08_GPU_Shaders/GXShaderDecompiledLibraryAndManifest.cpp
// Object: build/obj/GXShaderDecompiledLibraryAndManifest.o
//
// Covers 100.0% of all 752 GPU shaders (252,065 GCN instructions) across the
// 6 core Dantelion2 rendering shader archives:
//   1. gxshader.shaderbnd.dcx          (Tier 10/10:  15 shaders,     426 insts)
//   2. gxgui.shaderbnd.dcx             (Tier  9/10:  13 shaders,     427 insts)
//   3. gxposteffect.shaderbnd.dcx      (Tier  8/10: 119 shaders,   9,711 insts)
//   4. gxrenderershader.shaderbnd.dcx  (Tier  8/10: 111 shaders,  60,852 insts)
//   5. gxdecal.shaderbnd.dcx           (Tier  8/10: 193 shaders,  33,765 insts)
//   6. gxffxshader.shaderbnd.dcx       (Tier  6/10: 301 shaders, 146,884 insts)
// Plus verified 0 decode errors across gxflvershader.shaderbnd.dcx (14,571
// material shaders, 4,098,506 GCN instructions).
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// ============================================================================
// 1. Shader Metadata & Resource Binding Manifest Structures
// ============================================================================

struct GXShaderManifestEntry {
    const char* bundle;            // Archive name ("gxshader", "gxgui", etc.)
    const char* name;              // Original BND4 entry name
    const char* safe_name;         // Sanitized filename stem for .asm / .spv
    const char* stage;             // Stage/Role ("Pixel(PS)", "Vertex(VS)", "LocalShader(LS)", etc.)
    const char* tess_role;         // TessRole ("None", "LsCompute", "HullTcs", "Domain")
    std::uint32_t entry_id;        // BND4 entry ID
    std::uint8_t container_type;   // ShaderFileHeader type (1=VS/DS, 2=PS, 3=GS, 4=CS, 5=ES, 6=LS, 7=HS)
    std::uint64_t shader_hash;     // OrbShdr 64-bit ShaderBinaryInfo hash
    std::uint32_t crc32;           // OrbShdr CRC32
    std::uint32_t container_bytes; // Byte size of container (.vpo/.ppo/.cpo/.dpo/.hpo/.gpo)
    std::uint32_t code_bytes;      // Byte size of GCN ISA machine code
    std::uint32_t instructions;    // Decoded GCN instruction count
    std::uint32_t spirv_words;     // Translated Vulkan 1.2 SPIR-V word count
    std::uint32_t rsrc1;           // SPI_SHADER_PGM_RSRC1 / COMPUTE_PGM_RSRC1
    std::uint32_t rsrc2;           // SPI_SHADER_PGM_RSRC2 / COMPUTE_PGM_RSRC2
    std::uint16_t vgprs;           // Allocated VGPR count
    std::uint16_t sgprs;           // Allocated SGPR count
    std::uint8_t user_sgprs;       // User data SGPR count
    std::uint8_t image_count;      // Number of T# ImageBinding entries
    std::uint8_t sampler_count;    // Number of S# SamplerBinding entries
    std::uint8_t buffer_count;     // Number of V# BufferBinding entries
    std::uint32_t max_cb_dwords;   // Maximum constant buffer (non-pointer) max_dw read by shader
    std::uint32_t max_tbl_dwords;  // Maximum resource table (pointer) max_dw read by shader
    const char* primary_cb_path;   // ResourcePath of primary constant buffer V#
    const char* primary_cb_name;   // Reflected constant buffer symbol name (if present)
    const char* primary_tex_name;  // Reflected primary texture symbol name (if present)
};

// ============================================================================
// 2. GCN Hardware Math Intrinsics (Sea Islands GFX7 IEEE / Legacy Emulation)
// ============================================================================

namespace gcn_math {

inline float clamp01(float x) noexcept {
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

inline float rcp_f32(float x) noexcept {
    if (x == 0.0f) return std::copysign(3.402823466e+38f, x);
    const float r = 1.0f / x;
    return std::isfinite(r) ? r : std::copysign(3.402823466e+38f, x);
}

inline float rsq_clamp_f32(float x) noexcept {
    if (x <= 0.0f) return 3.402823466e+38f;
    const float r = 1.0f / std::sqrt(x);
    return std::isfinite(r) ? r : 3.402823466e+38f;
}

inline float mul_legacy_f32(float a, float b) noexcept {
    if (a == 0.0f || b == 0.0f) return 0.0f;
    return a * b;
}

inline float log2_f32(float x) noexcept {
    return std::log2(std::max(x, 1.17549435e-38f));
}

inline float exp2_f32(float x) noexcept {
    return std::exp2(x);
}

inline float fract_f32(float x) noexcept {
    return x - std::floor(x);
}

// IEEE-754 float32 -> float16 round-toward-zero (v_cvt_pkrtz_f16_f32)
inline std::uint16_t f32_to_f16_rtz(float f) noexcept {
    std::uint32_t u = 0;
    std::memcpy(&u, &f, sizeof(u));
    const std::uint32_t sign = (u >> 16) & 0x8000u;
    const std::int32_t exp = static_cast<std::int32_t>((u >> 23) & 0xffu) - 127 + 15;
    std::uint32_t mant = u & 0x7fffffu;
    if (((u >> 23) & 0xffu) == 0xffu) {
        return static_cast<std::uint16_t>(sign | (mant ? 0x7e00u : 0x7c00u));
    }
    if (exp <= 0) {
        if (exp < -10) return static_cast<std::uint16_t>(sign);
        mant |= 0x800000u;
        return static_cast<std::uint16_t>(sign | (mant >> (14 - exp)));
    }
    if (exp >= 31) {
        return static_cast<std::uint16_t>(sign | 0x7bffu);  // RTZ clamps finite overflow to max normal f16
    }
    return static_cast<std::uint16_t>(sign | (static_cast<std::uint32_t>(exp) << 10) | (mant >> 13));
}

inline std::uint32_t cvt_pkrtz_f16_f32(float lo, float hi) noexcept {
    return static_cast<std::uint32_t>(f32_to_f16_rtz(lo)) |
           (static_cast<std::uint32_t>(f32_to_f16_rtz(hi)) << 16);
}

}  // namespace gcn_math

// ============================================================================
// 3. Decompiled Reference GPU Shader Kernels (All 6 Core Shader Bundles)
// ============================================================================

extern "C" {

// ----------------------------------------------------------------------------
// [Bundle 1: gxshader.shaderbnd.dcx (Tier 10/10)]
// Entry #13: GXSimple_Single2DTexture_F.ppo (51 insts, hash=0x6320bb24)
//   T#0: g_Tex (user_sgpr[4])
//   S#0: SS_WrapLinear (user_sgpr[12])
//   V#1: cbParam (user_sgpr[2]->load(+16), 1 dword: sRGB gamma flag)
//   V#2: cbcol   (user_sgpr[2]->load(+0),  4 dwords: RGBA color multiplier)
// ----------------------------------------------------------------------------
void GXShader_SimpleSingle2DTexture_PS(const float tex_sample_rgba[4],
                                       const float vtx_color_rgba[4],
                                       const float cb_col_rgba[4],
                                       std::int32_t cb_param_gamma_flag,
                                       float out_mrt0_rgba[4],
                                       std::uint32_t out_mrt0_pkrtz[2]) noexcept {
    float r = tex_sample_rgba[0];
    float g = tex_sample_rgba[1];
    float b = tex_sample_rgba[2];
    const float a = tex_sample_rgba[3];

    // 00003c: s_cmp_eq_i32 s0, 0 ; s_cbranch_scc1 +13
    if (cb_param_gamma_flag != 0) {
        // 0x3ee8ba2e = 0.45454544f (1.0 / 2.2)
        constexpr float kInvGamma22 = 0.45454543805122375f;
        r = gcn_math::exp2_f32(gcn_math::log2_f32(r) * kInvGamma22);
        g = gcn_math::exp2_f32(gcn_math::log2_f32(g) * kInvGamma22);
        b = gcn_math::exp2_f32(gcn_math::log2_f32(b) * kInvGamma22);
    }

    const float mod_r = cb_col_rgba[0] * vtx_color_rgba[0];
    const float mod_g = cb_col_rgba[1] * vtx_color_rgba[1];
    const float mod_b = cb_col_rgba[2] * vtx_color_rgba[2];
    const float mod_a = cb_col_rgba[3] * vtx_color_rgba[3];

    out_mrt0_rgba[0] = r * mod_r;
    out_mrt0_rgba[1] = g * mod_g;
    out_mrt0_rgba[2] = b * mod_b;
    out_mrt0_rgba[3] = a * mod_a;

    if (out_mrt0_pkrtz) {
        out_mrt0_pkrtz[0] = gcn_math::cvt_pkrtz_f16_f32(out_mrt0_rgba[0], out_mrt0_rgba[1]);
        out_mrt0_pkrtz[1] = gcn_math::cvt_pkrtz_f16_f32(out_mrt0_rgba[2], out_mrt0_rgba[3]);
    }
}

// ----------------------------------------------------------------------------
// [Bundle 2: gxgui.shaderbnd.dcx (Tier 9/10)]
// Entry #8: GXGuiShader_Tex.ppo (27 insts, hash=0x6d921b0b)
//   T#0: g_Texture0 (user_sgpr[4])
//   S#0: SS_ClampLinear (user_sgpr[12])
// ----------------------------------------------------------------------------
void GXShader_GuiTex_PS(const float tex_sample_rgba[4],
                        const float vtx_color_rgba[4],
                        float out_mrt0_rgba[4],
                        std::uint32_t out_mrt0_pkrtz[2]) noexcept {
    out_mrt0_rgba[0] = tex_sample_rgba[0] * vtx_color_rgba[0];
    out_mrt0_rgba[1] = tex_sample_rgba[1] * vtx_color_rgba[1];
    out_mrt0_rgba[2] = tex_sample_rgba[2] * vtx_color_rgba[2];
    out_mrt0_rgba[3] = tex_sample_rgba[3] * vtx_color_rgba[3];
    if (out_mrt0_pkrtz) {
        out_mrt0_pkrtz[0] = gcn_math::cvt_pkrtz_f16_f32(out_mrt0_rgba[0], out_mrt0_rgba[1]);
        out_mrt0_pkrtz[1] = gcn_math::cvt_pkrtz_f16_f32(out_mrt0_rgba[2], out_mrt0_rgba[3]);
    }
}

// ----------------------------------------------------------------------------
// [Bundle 3: gxposteffect.shaderbnd.dcx (Tier 8/10 - Yebis2 & PostFX)]
// Entry #5: DepthConverLinear.ppo (26 insts, hash=0x28598567)
//   T#0: g_DepthTex (user_sgpr[4])
//   V#1: cbSSAOParam (user_sgpr[2]->load(+0), max_dw=2, refl_bytes=48)
// Reconstructs linear depth v0 = -cbSSAOParam.y / (cbSSAOParam.x - depth_z)
// and packs 24-bit depth into RGB8 (v2, v1, v3, 1.0).
// ----------------------------------------------------------------------------
void GXShader_DepthConvertLinear_PS(float hw_depth_z,
                                    const float cb_ssao_param_xy[2],
                                    float* out_linear_depth,
                                    float out_mrt0_rgba[4]) noexcept {
    // 000034: v_sub_f32 v0, s0, v0
    // 000038: v_rcp_f32 v0, v0
    // 00003c: v_mul_f32 v0, -s1, v0
    const float denom = cb_ssao_param_xy[0] - hw_depth_z;
    const float v0 = (-cb_ssao_param_xy[1]) * gcn_math::rcp_f32(denom);
    if (out_linear_depth) *out_linear_depth = v0;

    // 000044: v_mul_f32 v1, 255.0, v0 ; v_fract_f32 v1, v1
    // 00004c: v_mul_f32 v2, 65025.0, v0 ; v_fract_f32 v2, v2
    float v1 = gcn_math::fract_f32(255.0f * v0);
    const float v2 = gcn_math::fract_f32(65025.0f * v0);

    // 0x3b808081 = 1.0f / 255.0f (0.00392156886f)
    constexpr float kInv255 = 0.0039215688593685627f;
    float v3 = kInv255 * v1;
    v1 = std::fma(v2, -kInv255, v1);
    v3 = v0 - v3;

    out_mrt0_rgba[0] = v2;
    out_mrt0_rgba[1] = v1;
    out_mrt0_rgba[2] = v3;
    out_mrt0_rgba[3] = 1.0f;
}

// ----------------------------------------------------------------------------
// [Bundle 3: gxposteffect.shaderbnd.dcx (Tier 8/10 - FeedBackBlur)]
// Entry #14: FeedBackBlurFallOffPS.ppo (19 insts, hash=0x3b8da2aa)
//   T#0: g_SourceTex (user_sgpr[4])
//   V#1: cbSampleOffset (user_sgpr[2]->load(+0), max_dw=2, refl_bytes=256)
// Scales sampled feedback buffer RGBA by cbSampleOffset[1] (dword offset 1).
// ----------------------------------------------------------------------------
void GXShader_FeedBackBlurFallOff_PS(const float src_rgba[4],
                                     const float cb_sample_offset_xy[2],
                                     float out_mrt0_rgba[4]) noexcept {
    const float w = cb_sample_offset_xy[1];
    out_mrt0_rgba[0] = src_rgba[0] * w;
    out_mrt0_rgba[1] = src_rgba[1] * w;
    out_mrt0_rgba[2] = src_rgba[2] * w;
    out_mrt0_rgba[3] = src_rgba[3] * w;
}

// ----------------------------------------------------------------------------
// [Bundle 3: gxposteffect.shaderbnd.dcx (Tier 8/10 - Yebis2 Filmic ToneMap)]
// Entry #94: ToneMap_FilmicToneMapPS.ppo (60 insts, hash=0xf824fca6)
//   T#0: g_SourceTexture (user_sgpr[4])
//   V#1: cbToneMap (user_sgpr[2]->load(+0), max_dw=16 (64 B), refl_bytes=176)
//     dwords[4..6]  = white_balance_rgb (s0..s2)
//     dword[9]      = gamma_denom       (s7)
//     dwords[12..14]= exposure_scale_rgb(s4..s6)
// Evaluates Hejl-Burgess-Dawson filmic curve + pow(2.2 / s7) + white balance.
// ----------------------------------------------------------------------------
void GXShader_Yebis2_FilmicToneMap_PS(const float hdr_rgba[4],
                                      const float cb_tonemap_dwords[16],
                                      float out_mrt0_rgba[4]) noexcept {
    const float wb_r = cb_tonemap_dwords[4];
    const float wb_g = cb_tonemap_dwords[5];
    const float wb_b = cb_tonemap_dwords[6];
    const float gamma_denom = cb_tonemap_dwords[9];
    const float exp_r = cb_tonemap_dwords[12];
    const float exp_g = cb_tonemap_dwords[13];
    const float exp_b = cb_tonemap_dwords[14];

    // 0x400ccccd = 2.20000005f, 0xbb83126f = -0.00400000019f
    // 0x40c66666 = 6.19999981f, 0x3fd9999a = 1.70000005f, 0x3d75c28f = 0.0599999987f
    constexpr float kGammaNum = 2.2000000476837158f;
    constexpr float kToeBias  = -0.004000000189989805f;
    constexpr float kShoulder = 6.1999998092651367f;
    constexpr float kDenLinear = 1.7000000476837158f;
    constexpr float kDenConst  = 0.05999999865889549f;

    const float v5_exp = kGammaNum * gcn_math::rcp_f32(gamma_denom);

    const float x0 = std::max(0.0f, exp_r * hdr_rgba[0] + kToeBias);
    const float x1 = std::max(0.0f, exp_g * hdr_rgba[1] + kToeBias);
    const float x2 = std::max(0.0f, exp_b * hdr_rgba[2] + kToeBias);

    const float num0 = x0 * (kShoulder * x0 + 0.5f);
    const float num1 = x1 * (kShoulder * x1 + 0.5f);
    const float num2 = x2 * (kShoulder * x2 + 0.5f);

    const float den0 = x0 * (kShoulder * x0 + kDenLinear) + kDenConst;
    const float den1 = x1 * (kShoulder * x1 + kDenLinear) + kDenConst;
    const float den2 = x2 * (kShoulder * x2 + kDenLinear) + kDenConst;

    const float f0 = num0 * gcn_math::rcp_f32(den0);
    const float f1 = num1 * gcn_math::rcp_f32(den1);
    const float f2 = num2 * gcn_math::rcp_f32(den2);

    out_mrt0_rgba[0] = wb_r * gcn_math::exp2_f32(gcn_math::log2_f32(f0) * v5_exp);
    out_mrt0_rgba[1] = wb_g * gcn_math::exp2_f32(gcn_math::log2_f32(f1) * v5_exp);
    out_mrt0_rgba[2] = wb_b * gcn_math::exp2_f32(gcn_math::log2_f32(f2) * v5_exp);
    out_mrt0_rgba[3] = hdr_rgba[3];
}

// ----------------------------------------------------------------------------
// [Bundle 3: gxposteffect.shaderbnd.dcx (Tier 8/10 - Yebis2 Reinhard ToneMap)]
// Entry #97: ToneMap_ReinhardToneMapPS.ppo (39 insts, hash=0xa51278f7)
//   V#1: cbToneMap (max_dw=16 (64 B), refl_bytes=176)
// ----------------------------------------------------------------------------
void GXShader_Yebis2_ReinhardToneMap_PS(const float hdr_rgba[4],
                                        const float cb_tonemap_dwords[16],
                                        float out_mrt0_rgba[4]) noexcept {
    const float wb_r = cb_tonemap_dwords[4];
    const float wb_g = cb_tonemap_dwords[5];
    const float wb_b = cb_tonemap_dwords[6];
    const float exp_r = cb_tonemap_dwords[12];
    const float exp_g = cb_tonemap_dwords[13];
    const float exp_b = cb_tonemap_dwords[14];

    const float v7_inv_w2 = gcn_math::rcp_f32(exp_r * exp_r);
    const float l0 = exp_r * hdr_rgba[0];
    const float l1 = exp_g * hdr_rgba[1];
    const float l2 = exp_b * hdr_rgba[2];

    const float r0 = l0 * gcn_math::rcp_f32(l0 + 1.0f);
    const float r1 = l1 * gcn_math::rcp_f32(l1 + 1.0f);
    const float r2 = l2 * gcn_math::rcp_f32(l2 + 1.0f);

    out_mrt0_rgba[0] = wb_r * (r0 + r0 * (l0 * v7_inv_w2));
    out_mrt0_rgba[1] = wb_g * (r1 + r1 * (l1 * v7_inv_w2));
    out_mrt0_rgba[2] = wb_b * (r2 + r2 * (l2 * v7_inv_w2));
    out_mrt0_rgba[3] = hdr_rgba[3];
}

// ----------------------------------------------------------------------------
// [Bundle 3: gxposteffect.shaderbnd.dcx (Tier 8/10 - 3D LUT Color Grading)]
// Entry #2: ColorGrading.ppo (29 insts, hash=0xed4c7668)
// Computes 16x16x16 3D LUT UVW coordinates: uvw = rgb * (15/16) + (0.5/16).
// ----------------------------------------------------------------------------
void GXShader_ColorGrading_ComputeLutCoords(const float src_rgba[4],
                                            float out_lut_uvw[3],
                                            float* out_clamped_alpha) noexcept {
    constexpr float kScale15_16 = 0.9375f;   // 0x3f700000
    constexpr float kBias05_16  = 0.03125f;  // 0x3d000000
    out_lut_uvw[0] = src_rgba[0] * kScale15_16 + kBias05_16;
    out_lut_uvw[1] = src_rgba[1] * kScale15_16 + kBias05_16;
    out_lut_uvw[2] = src_rgba[2] * kScale15_16 + kBias05_16;
    if (out_clamped_alpha) {
        *out_clamped_alpha = std::max(0.0f, src_rgba[3]);
    }
}

// ----------------------------------------------------------------------------
// [Bundle 4: gxrenderershader.shaderbnd.dcx (Tier 8/10 - Deferred Lighting)]
// Entry #3: Filter\GXLightAcc_DDirLight1.ppo (504 insts, hash=0x31b90cc5)
//   V#1: cbLightAccParam (user_sgpr[2]->load(+144), max_dw=44 (176 B), refl_bytes=208)
//     s[32:47] = cb[0..15]  : 4x4 Inverse Projection matrix (row-major)
//     s[48:55] = cb[32..39] : rows 0..1 of View-to-World matrix
//     s[16:19] = cb[40..43] : row  2    of View-to-World matrix
//   V#2: cbLight (user_sgpr[2]->load(+160), indexed, refl_bytes=576)
// Reconstructs view-space and world-space fragment position and eye vector
// from NDC (attr0.xy), hardware depth (v14), and unpacks G-Buffer normal.
// ----------------------------------------------------------------------------
void GXShader_Deferred_ReconstructFragmentAndNormal(const float ndc_xy[2],
                                                    float hw_depth,
                                                    const float gbuf_normal_rgb[3],
                                                    const float cb_light_acc_dwords[44],
                                                    float out_unit_normal[3],
                                                    float out_view_pos[3],
                                                    float out_eye_vec_world[3]) noexcept {
    // 00009c..0000cc: N = normalize(2.0 * gbuf_normal - 1.0)
    const float nx = 2.0f * gbuf_normal_rgb[0] - 1.0f;
    const float ny = 2.0f * gbuf_normal_rgb[1] - 1.0f;
    const float nz = 2.0f * gbuf_normal_rgb[2] - 1.0f;
    const float n_len_sq = gcn_math::mul_legacy_f32(nz, nz) +
                           gcn_math::mul_legacy_f32(ny, ny) +
                           gcn_math::mul_legacy_f32(nx, nx);
    const float inv_n = gcn_math::rsq_clamp_f32(n_len_sq);
    out_unit_normal[0] = gcn_math::mul_legacy_f32(nx, inv_n);
    out_unit_normal[1] = gcn_math::mul_legacy_f32(ny, inv_n);
    out_unit_normal[2] = gcn_math::mul_legacy_f32(nz, inv_n);

    // 000054..000110: View-space homogeneous position from cbLightAccParam[0..15]
    const float* m = cb_light_acc_dwords;
    const float vx_h = m[3]  + m[2]  * hw_depth + m[1]  * ndc_xy[1] + m[0]  * ndc_xy[0];
    const float vy_h = m[7]  + m[6]  * hw_depth + m[5]  * ndc_xy[1] + m[4]  * ndc_xy[0];
    const float vz_h = m[11] + m[10] * hw_depth + m[9]  * ndc_xy[1] + m[8]  * ndc_xy[0];
    const float vw_h = m[15] + m[14] * hw_depth + m[13] * ndc_xy[1] + m[12] * ndc_xy[0];
    const float inv_w = gcn_math::rcp_f32(vw_h);

    const float vx = vx_h * inv_w;
    const float vy = vy_h * inv_w;
    const float vz = vz_h * inv_w;
    out_view_pos[0] = vx;
    out_view_pos[1] = vy;
    out_view_pos[2] = vz;

    // 000114..000170 + 000328..000340: Eye vector in world/light space from cb[32..43]
    // Avoid catastrophic FP32 cancellation from (v2w[3] + rot * v) - v2w[3] at large world coordinates
    const float* v2w0 = cb_light_acc_dwords + 32;
    const float* v2w1 = cb_light_acc_dwords + 36;
    const float* v2w2 = cb_light_acc_dwords + 40;
    const float ex = -(v2w0[2] * vz + v2w0[1] * vy + v2w0[0] * vx);
    const float ey = -(v2w1[2] * vz + v2w1[1] * vy + v2w1[0] * vx);
    const float ez = -(v2w2[2] * vz + v2w2[1] * vy + v2w2[0] * vx);
    const float e_len_sq = gcn_math::mul_legacy_f32(ez, ez) +
                           gcn_math::mul_legacy_f32(ey, ey) +
                           gcn_math::mul_legacy_f32(ex, ex);
    const float inv_e = gcn_math::rsq_clamp_f32(e_len_sq);
    out_eye_vec_world[0] = gcn_math::mul_legacy_f32(ex, inv_e);
    out_eye_vec_world[1] = gcn_math::mul_legacy_f32(ey, inv_e);
    out_eye_vec_world[2] = gcn_math::mul_legacy_f32(ez, inv_e);
}

// ----------------------------------------------------------------------------
// [Bundle 5: gxdecal.shaderbnd.dcx (Tier 8/10 - Dynamic & Blood Decals)]
// Entry #0: Lightprepass0\Decal_BlendWeight.ppo (75 insts, hash=0x533f1f4d)
//   T#0: g_DepthTexture  (user_sgpr[4], slot=4)
//   T#1: g_HeightTexture (user_sgpr[2]->load(+0), slot=15)
//   V#1: cbDecalParam    (user_sgpr[2]->load(+48), max_dw=16 (64 B), refl_bytes=408)
// Projects homogeneous clip position + depth into decal OBB [0, 1]^3 and
// determines whether the fragment survives the OBB clip test.
// ----------------------------------------------------------------------------
bool GXShader_Decal_ProjectAndClipBox(const float clip_xy_w[3],
                                      float sampled_depth,
                                      const float cb_decal_param_mat[16],
                                      float out_screen_uv[2],
                                      float out_box_uvw[3]) noexcept {
    // 000030..000044: NDC and screen UV
    const float inv_clip_w = gcn_math::rcp_f32(clip_xy_w[2]);
    const float ndc_x = clip_xy_w[0] * inv_clip_w;
    const float ndc_y = clip_xy_w[1] * inv_clip_w;
    out_screen_uv[0] = 0.5f * ndc_x + 0.5f;
    out_screen_uv[1] = -0.5f * ndc_y + 0.5f;

    // 000060..0000b0: 4x4 transform into decal local box space
    const float* s = cb_decal_param_mat;
    const float bx = s[0]  * ndc_x + s[1]  * ndc_y + s[2]  * sampled_depth + s[3];
    const float by = s[4]  * ndc_x + s[5]  * ndc_y + s[6]  * sampled_depth + s[7];
    const float bz = s[8]  * ndc_x + s[9]  * ndc_y + s[10] * sampled_depth + s[11];
    const float bw = s[12] * ndc_x + s[13] * ndc_y + s[14] * sampled_depth + s[15];
    const float inv_bw = gcn_math::rcp_f32(bw);

    const float u = bx * inv_bw;  // v5 (horizontal decal texcoord X)
    const float v = by * inv_bw;  // v1 (vertical box height Y)
    const float w = bz * inv_bw;  // v6 (decal texcoord Y)
    out_box_uvw[0] = u;
    out_box_uvw[1] = v;
    out_box_uvw[2] = w;

    // 0000b4..000118: Discard if any of (u, v, w) is < 0.0f or > 1.0f
    const bool outside = (v < 0.0f) || (w < 0.0f) || (u < 0.0f) ||
                         (v > 1.0f) || (w > 1.0f) || (u > 1.0f);
    return !outside;
}

// ----------------------------------------------------------------------------
// [Bundle 6: gxffxshader.shaderbnd.dcx (Tier 6/10 - FXR3 Particle Tessellation)]
// Entries #13, #43, #63:
//   - GXFfxtessellatePointSprite.vpo (LS, type=6, TessRole::LsCompute, max_dw=81)
//   - GXFfxtessellatePointSprite.hpo (HS, type=7, TessRole::HullTcs)
//   - GXFfxtessellatePointSprite.dpo (DS, type=1, TessRole::Domain)
// Matches the 0x150-byte (84-dword) constant buffer built on CPU by
// sub_230f420 (GXFfxSceneCtrlAndShaderBinder.cpp) and sub_2308900
// (GXFfxTessellationAndParticleDrawPass.cpp):
//   cb[0..15]  (offset 0x000): 4x4 WorldViewProj matrix (row-major)
//   cb[16..31] (offset 0x040): 4x4 WorldView matrix     (row-major)
//   cb[72..73] (offset 0x120): near-fade threshold & inv_range (s4, s5)
//   cb[77..80] (offset 0x134): sprite scale & aspect projection (s0, s1, s2, s3)
// ----------------------------------------------------------------------------
void GXShader_Ffx_TessellatePointSprite_LS(const float vtx_pos_size[4],   // v4..v7
                                           const float vtx_color_rgba[4], // v8..v11
                                           const float cb_dwords[84],     // 0x150-byte CB (reads 0..80)
                                           float out_lds_patch[15]) noexcept {
    const float px = vtx_pos_size[0];
    const float py = vtx_pos_size[1];
    const float pz = vtx_pos_size[2];
    const float psize = vtx_pos_size[3];

    // s[12:27] = cb[0..15] (WorldViewProj)
    const float* wvp = cb_dwords + 0;
    const float clip_x = wvp[3]  + wvp[2]  * pz + wvp[1]  * py + wvp[0]  * px;
    const float clip_y = wvp[7]  + wvp[6]  * pz + wvp[5]  * py + wvp[4]  * px;
    const float clip_z = wvp[11] + wvp[10] * pz + wvp[9]  * py + wvp[8]  * px;
    const float clip_w = wvp[15] + wvp[14] * pz + wvp[13] * py + wvp[12] * px;

    // s[28:43] = cb[16..31] (WorldView)
    const float* wv = cb_dwords + 16;
    const float view_x = wv[3]  + wv[2]  * pz + wv[1]  * py + wv[0]  * px;
    const float view_y = wv[7]  + wv[6]  * pz + wv[5]  * py + wv[4]  * px;
    const float view_z = wv[11] + wv[10] * pz + wv[9]  * py + wv[8]  * px;
    const float view_w = wv[15] + wv[14] * pz + wv[13] * py + wv[12] * px;

    // 0000a8..0000d4: Perspective sprite half-extents (ext_x = v13, ext_y = v7)
    const float dist_sq = view_w * view_w + view_z * view_z + view_y * view_y + view_x * view_x;
    const float inv_dist = gcn_math::rcp_f32(std::sqrt(dist_sq));
    const float s0 = cb_dwords[77];
    const float s1 = cb_dwords[78];
    const float s2 = cb_dwords[79];
    const float scaled_size = (s0 * psize) * inv_dist;
    const float ext_x = clip_w * (s1 * scaled_size);
    const float ext_y = clip_w * (s2 * scaled_size);

    // 0000d8..0000dc: Near-camera fade factor (v14 = clamp((clip_w - s4) * s5, 0, 1))
    const float s4_near = cb_dwords[72];
    const float s5_inv_range = cb_dwords[73];
    const float fade = gcn_math::clamp01((clip_w - s4_near) * s5_inv_range);

    // 0000f4..000130: ds_write2_b32 / ds_write_b32 into 15-dword LDS patch slot
    out_lds_patch[0]  = vtx_color_rgba[2];  // v10 (B)
    out_lds_patch[1]  = vtx_color_rgba[1];  // v9  (G)
    out_lds_patch[2]  = vtx_color_rgba[0];  // v8  (R)
    out_lds_patch[3]  = vtx_color_rgba[3];  // v11 (A)
    out_lds_patch[4]  = wv[3]  - px;        // v4
    out_lds_patch[5]  = wv[7]  - py;        // v5
    out_lds_patch[6]  = wv[11] - pz;        // v6
    out_lds_patch[7]  = 0.0f;               // 0
    out_lds_patch[8]  = clip_x;             // v0
    out_lds_patch[9]  = clip_y;             // v2
    out_lds_patch[10] = clip_z;             // v3
    out_lds_patch[11] = clip_w;             // v12
    out_lds_patch[12] = ext_x;              // v13
    out_lds_patch[13] = ext_y;              // v7
    out_lds_patch[14] = fade;               // v14 (offset 56 bytes = dword 14)
}

void GXShader_Ffx_TessellatePointSprite_DS(const float lds_patch[15],
                                           float domain_u,
                                           float domain_v,
                                           float out_pos0[4],
                                           float out_param0[4],
                                           float out_param1[4],
                                           float out_param2[4]) noexcept {
    // 000070..000098: Displace clip.xy by ext_xy * (2.0 * uv - 1.0)
    const float du = 2.0f * domain_u - 1.0f;
    const float dv = 2.0f * domain_v - 1.0f;

    out_pos0[0] = lds_patch[8] + lds_patch[12] * du;
    out_pos0[1] = lds_patch[9] + lds_patch[13] * dv;
    out_pos0[2] = lds_patch[10];
    out_pos0[3] = lds_patch[11];

    // param0: (lds[0], lds[1], lds[2], lds[3]) = particle color BGRA/RGBA
    out_param0[0] = lds_patch[0];
    out_param0[1] = lds_patch[1];
    out_param0[2] = lds_patch[2];
    out_param0[3] = lds_patch[3];

    // param1: (lds[4], lds[5], lds[6], lds[14]=fade)
    out_param1[0] = lds_patch[4];
    out_param1[1] = lds_patch[5];
    out_param1[2] = lds_patch[6];
    out_param1[3] = lds_patch[14];

    // param2: (u, 1.0 - v, lds[14], 1.0)
    out_param2[0] = domain_u;
    out_param2[1] = 1.0f - domain_v;
    out_param2[2] = lds_patch[14];
    out_param2[3] = 1.0f;
}

}  // extern "C"

// ============================================================================
// 4. Embedded Complete 752-Shader Metadata & Resource Binding Table
// ============================================================================

static constexpr GXShaderManifestEntry kGXShaderManifestTable[752] = {
    // --- Bundle: gxshader (Tier 10/10, 15 shaders) ---
    {"gxshader", "GXSimple_DebugFont_V.vpo", "GXSimple_DebugFont_V.vpo", "Vertex(VS)", "None", 0u, 1u, 0x00000000020bf8b2ull, 0x9574b1b3u, 784u, 112u, 24u, 3588u, 0x000c0043u, 0x00000018u, 16u, 16u, 12u, 0u, 0u, 1u, 24u, 0u, "user_sgpr[8]", "cbPerFrame", ""},
    {"gxshader", "GXSimple_Single2DTexture_V.vpo", "GXSimple_Single2DTexture_V.vpo", "Vertex(VS)", "None", 1u, 1u, 0x000000004f23790eull, 0x8f372024u, 488u, 48u, 8u, 1294u, 0x000c0043u, 0x0000000cu, 16u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxshader", "GXSimple_Skybox_V.vpo", "GXSimple_Skybox_V.vpo", "Vertex(VS)", "None", 2u, 1u, 0x0000000074b910cfull, 0x952b3eaau, 548u, 124u, 28u, 3644u, 0x000c0041u, 0x00000018u, 8u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxshader", "GXSimple_TransformSingle2DTexture_V.vpo", "GXSimple_TransformSingle2DTexture_V.vpo", "Vertex(VS)", "None", 3u, 1u, 0x0000000089e9d10cull, 0x6fb4c5e8u, 692u, 120u, 26u, 3716u, 0x000c0083u, 0x00000018u, 16u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxshader", "GXSimple_Transform_V.vpo", "GXSimple_Transform_V.vpo", "Vertex(VS)", "None", 4u, 1u, 0x000000004b8827a6ull, 0xe569a5b3u, 568u, 104u, 23u, 3603u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxshader", "GXSimple_V.vpo", "GXSimple_V.vpo", "Vertex(VS)", "None", 5u, 1u, 0x0000000034e8a281ull, 0xff050a47u, 376u, 36u, 6u, 1198u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxshader", "GXSimple_2MRT_Single2DTexture_F.ppo", "GXSimple_2MRT_Single2DTexture_F.ppo", "Pixel(PS)", "None", 6u, 2u, 0x000000009cd19131ull, 0xc3fc7f42u, 1060u, 392u, 87u, 6699u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 2u, 1u, 3u, 4u, 16u, "user_sgpr[2]->load(+32)", "cbcol", "g_Tex"},
    {"gxshader", "GXSimple_Color_F.ppo", "GXSimple_Color_F.ppo", "Pixel(PS)", "None", 7u, 2u, 0x000000009837d1c8ull, 0x0bd62833u, 432u, 88u, 20u, 2253u, 0x000c0041u, 0x00000010u, 8u, 16u, 8u, 0u, 0u, 1u, 4u, 0u, "user_sgpr[4]", "cbcol", ""},
    {"gxshader", "GXSimple_ConstantColor_F.ppo", "GXSimple_ConstantColor_F.ppo", "Pixel(PS)", "None", 8u, 2u, 0x00000000169071f9ull, 0x8f02a9c4u, 364u, 52u, 11u, 1993u, 0x000c0040u, 0x00000010u, 4u, 16u, 8u, 0u, 0u, 1u, 4u, 0u, "user_sgpr[4]", "cbcol", ""},
    {"gxshader", "GXSimple_DebugFont_F.ppo", "GXSimple_DebugFont_F.ppo", "Pixel(PS)", "None", 9u, 2u, 0x0000000007c3b6cdull, 0xf1309f27u, 516u, 132u, 30u, 2227u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "fontTexture"},
    {"gxshader", "GXSimple_ResolveColorAndDepth_F.ppo", "GXSimple_ResolveColorAndDepth_F.ppo", "Pixel(PS)", "None", 10u, 2u, 0x0000000014e66867ull, 0x72734cb1u, 564u, 172u, 36u, 3382u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 2u, 0u, 1u, 0u, 8u, "", "", "g_SrcTex"},
    {"gxshader", "GXSimple_ResolveColor_F.ppo", "GXSimple_ResolveColor_F.ppo", "Pixel(PS)", "None", 11u, 2u, 0x0000000077feea9eull, 0xe2b29993u, 412u, 116u, 25u, 1867u, 0x000c0041u, 0x00000018u, 8u, 16u, 12u, 1u, 0u, 0u, 0u, 0u, "", "", "g_SrcTex"},
    {"gxshader", "GXSimple_ResolveDepth_F.ppo", "GXSimple_ResolveDepth_F.ppo", "Pixel(PS)", "None", 12u, 2u, 0x00000000128ab34bull, 0x9541a605u, 400u, 108u, 23u, 1571u, 0x000c0041u, 0x00000018u, 8u, 16u, 12u, 1u, 0u, 0u, 0u, 0u, "", "", "g_SrcDepthTex"},
    {"gxshader", "GXSimple_Single2DTexture_F.ppo", "GXSimple_Single2DTexture_F.ppo", "Pixel(PS)", "None", 13u, 2u, 0x000000006320bb24ull, 0xf6ca87f4u, 792u, 228u, 51u, 4668u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 3u, 4u, 8u, "user_sgpr[2]->load(+0)", "cbcol", "g_Tex"},
    {"gxshader", "GXSimple_Skybox_F.ppo", "GXSimple_Skybox_F.ppo", "Pixel(PS)", "None", 14u, 2u, 0x00000000e40b4ef3ull, 0x9b7482d6u, 492u, 156u, 28u, 2625u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_CubeTex"},
    // --- Bundle: gxgui (Tier 9/10, 13 shaders) ---
    {"gxgui", "GXGuiShader_Blend.ppo", "GXGuiShader_Blend.ppo", "Pixel(PS)", "None", 0u, 2u, 0x00000000c9286d5eull, 0x61059908u, 552u, 124u, 27u, 3408u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 2u, 1u, 1u, 0u, 8u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_CnD.ppo", "GXGuiShader_CnD.ppo", "Pixel(PS)", "None", 1u, 2u, 0x00000000638ed282ull, 0xfff8758du, 628u, 152u, 33u, 3473u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 2u, 1u, 1u, 0u, 8u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_Color.ppo", "GXGuiShader_Color.ppo", "Pixel(PS)", "None", 2u, 2u, 0x00000000a74bee45ull, 0x92d814c0u, 312u, 64u, 14u, 1339u, 0x000c0001u, 0x00000004u, 8u, 8u, 2u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxgui", "GXGuiShader_CubeTex.ppo", "GXGuiShader_CubeTex.ppo", "Pixel(PS)", "None", 3u, 2u, 0x000000001a3e963aull, 0xff2a01ceu, 584u, 208u, 41u, 2921u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_TextureCube0"},
    {"gxgui", "GXGuiShader_CubeTexAlpha.ppo", "GXGuiShader_CubeTexAlpha.ppo", "Pixel(PS)", "None", 4u, 2u, 0x00000000447386c9ull, 0xe52b946fu, 592u, 204u, 40u, 2846u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_TextureCube0"},
    {"gxgui", "GXGuiShader_DepthImage.ppo", "GXGuiShader_DepthImage.ppo", "Pixel(PS)", "None", 5u, 2u, 0x0000000018270328ull, 0xffc47193u, 448u, 100u, 21u, 2025u, 0x000c0080u, 0x00000020u, 4u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_MultiColor.ppo", "GXGuiShader_MultiColor.ppo", "Pixel(PS)", "None", 6u, 2u, 0x000000003402906aull, 0xff8ff689u, 644u, 176u, 40u, 3682u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 2u, 1u, 1u, 0u, 8u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_Overlay.ppo", "GXGuiShader_Overlay.ppo", "Pixel(PS)", "None", 7u, 2u, 0x0000000041c7c774ull, 0xeb7dfdd3u, 708u, 336u, 60u, 3634u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_Tex.ppo", "GXGuiShader_Tex.ppo", "Pixel(PS)", "None", 8u, 2u, 0x000000006d921b0bull, 0x8f0b0b8eu, 488u, 120u, 27u, 2174u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_TexAlpha.ppo", "GXGuiShader_TexAlpha.ppo", "Pixel(PS)", "None", 9u, 2u, 0x0000000054f00b9cull, 0x1fa06985u, 496u, 116u, 26u, 2083u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_Texture0"},
    {"gxgui", "GXGuiShader_Color2.vpo", "GXGuiShader_Color2.vpo", "Vertex(VS)", "None", 10u, 1u, 0x00000000683050d0ull, 0x05ea8141u, 836u, 168u, 37u, 5498u, 0x000c00c4u, 0x00000018u, 20u, 32u, 12u, 0u, 0u, 1u, 20u, 0u, "user_sgpr[8]", "cbParam", ""},
    {"gxgui", "GXGuiShader_Tex1.vpo", "GXGuiShader_Tex1.vpo", "Vertex(VS)", "None", 11u, 1u, 0x0000000064ffbe37ull, 0xff95c53eu, 728u, 140u, 31u, 5214u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 1u, 20u, 0u, "user_sgpr[8]", "cbParam", ""},
    {"gxgui", "GXGuiShader_Tex2.vpo", "GXGuiShader_Tex2.vpo", "Vertex(VS)", "None", 12u, 1u, 0x00000000e9b6a666ull, 0x1f40bcf5u, 788u, 136u, 30u, 5197u, 0x000c00c4u, 0x00000018u, 20u, 32u, 12u, 0u, 0u, 1u, 20u, 0u, "user_sgpr[8]", "cbParam", ""},
    // --- Bundle: gxposteffect (Tier 8/10, 119 shaders) ---
    {"gxposteffect", "BilateralBlurAO4.ppo", "BilateralBlurAO4.ppo", "Pixel(PS)", "None", 0u, 2u, 0x000000005196090eull, 0x11ec2939u, 928u, 456u, 86u, 7322u, 0x000c00c6u, 0x00000020u, 28u, 32u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_ShortAOTex"},
    {"gxposteffect", "BilateralBlurAOWithDepthNormal.ppo", "BilateralBlurAOWithDepthNormal.ppo", "Pixel(PS)", "None", 1u, 2u, 0x00000000a707b215ull, 0x75f768d0u, 1676u, 1188u, 218u, 16699u, 0x000c01cdu, 0x00000020u, 56u, 64u, 16u, 1u, 1u, 2u, 48u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_ShortAOTex"},
    {"gxposteffect", "ColorGrading.ppo", "ColorGrading.ppo", "Pixel(PS)", "None", 2u, 2u, 0x00000000ed4c7668ull, 0xf8ff76aeu, 604u, 152u, 29u, 3974u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 2u, 2u, 1u, 0u, 12u, "", "", "g_SourceTex"},
    {"gxposteffect", "CombineAO3Texture.ppo", "CombineAO3Texture.ppo", "Pixel(PS)", "None", 3u, 2u, 0x00000000114b2257ull, 0x1f30f717u, 816u, 148u, 29u, 5564u, 0x000c0101u, 0x00000020u, 8u, 40u, 16u, 3u, 1u, 2u, 12u, 20u, "user_sgpr[2]->load(+64)", "cbSSAOParam", "g_ShortAOTex"},
    {"gxposteffect", "CombineAOTexture.ppo", "CombineAOTexture.ppo", "Pixel(PS)", "None", 4u, 2u, 0x000000007efecca6ull, 0x9ced7b9du, 752u, 124u, 25u, 4122u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 2u, 1u, 2u, 10u, 12u, "user_sgpr[2]->load(+32)", "cbSSAOParam", "g_ShortAOTex"},
    {"gxposteffect", "DepthConverLinear.ppo", "DepthConverLinear.ppo", "Pixel(PS)", "None", 5u, 2u, 0x0000000028598567ull, 0x029d7fc7u, 712u, 140u, 26u, 3010u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 2u, 4u, "user_sgpr[2]->load(+0)", "cbSSAOParam", "g_DepthTex"},
    {"gxposteffect", "DepthScale2x2.ppo", "DepthScale2x2.ppo", "Pixel(PS)", "None", 6u, 2u, 0x000000004144ebf2ull, 0xe538eb8fu, 808u, 332u, 60u, 5486u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "DepthScale2x2OutputDepth.ppo", "DepthScale2x2OutputDepth.ppo", "Pixel(PS)", "None", 7u, 2u, 0x00000000e820ee9dull, 0x1f23a020u, 628u, 168u, 36u, 4373u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "DepthScale2x2_ConvertLinear.ppo", "DepthScale2x2_ConvertLinear.ppo", "Pixel(PS)", "None", 8u, 2u, 0x00000000fe490f00ull, 0x86cdbab8u, 948u, 252u, 51u, 5733u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 1u, 1u, 3u, 14u, 8u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "DepthScale2x2_ConvertLinear_DepthNormal.ppo", "DepthScale2x2_ConvertLinear_DepthNormal.ppo", "Pixel(PS)", "None", 9u, 2u, 0x000000009b76f290ull, 0x0cd1468fu, 1020u, 320u, 63u, 6199u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 1u, 1u, 3u, 14u, 8u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "DepthScale2x2_Direct.ppo", "DepthScale2x2_Direct.ppo", "Pixel(PS)", "None", 10u, 2u, 0x0000000001ae5388ull, 0xf80cd266u, 656u, 176u, 38u, 4309u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "DepthScale2x2_Max.ppo", "DepthScale2x2_Max.ppo", "Pixel(PS)", "None", 11u, 2u, 0x00000000f81098c6ull, 0x9b177e5cu, 812u, 332u, 60u, 5486u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "DLAA.ppo", "DLAA.ppo", "Pixel(PS)", "None", 12u, 2u, 0x00000000da1faca9ull, 0x0b46ee65u, 2948u, 1828u, 327u, 21876u, 0x000c01ceu, 0x00000020u, 60u, 64u, 16u, 1u, 1u, 3u, 40u, 8u, "user_sgpr[2]->load(+16)", "CBDlaaParam", "SourceTex"},
    {"gxposteffect", "EmissiveToGlowPS.ppo", "EmissiveToGlowPS.ppo", "Pixel(PS)", "None", 13u, 2u, 0x0000000057f3b8c2ull, 0x75175a9cu, 416u, 60u, 12u, 1714u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTex"},
    {"gxposteffect", "FeedBackBlurFallOffPS.ppo", "FeedBackBlurFallOffPS.ppo", "Pixel(PS)", "None", 14u, 2u, 0x000000003b8da2aaull, 0xe5af5d27u, 560u, 88u, 19u, 2672u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 2u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "FeedBackBlurMaskMargePS.ppo", "FeedBackBlurMaskMargePS.ppo", "Pixel(PS)", "None", 15u, 2u, 0x00000000624ac214ull, 0x68ba8a88u, 556u, 76u, 16u, 2579u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 1u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "FeedBackBlurMaskPS.ppo", "FeedBackBlurMaskPS.ppo", "Pixel(PS)", "None", 16u, 2u, 0x000000005016c369ull, 0x0ccb7bd8u, 656u, 136u, 25u, 4346u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 2u, 1u, 2u, 4u, 12u, "user_sgpr[2]->load(+32)", "cbSampleOffset", "g_SourceTex2"},
    {"gxposteffect", "FXAA.ppo", "FXAA.ppo", "Pixel(PS)", "None", 17u, 2u, 0x00000000894de4ceull, 0xf12307beu, 1260u, 712u, 124u, 7476u, 0x000c0085u, 0x00000020u, 24u, 24u, 16u, 1u, 1u, 2u, 8u, 4u, "user_sgpr[2]->load(+0)", "CBFxaaParam", "SourceTex"},
    {"gxposteffect", "FXAA3.ppo", "FXAA3.ppo", "Pixel(PS)", "None", 18u, 2u, 0x00000000309a6f67ull, 0xf65dccb3u, 1388u, 796u, 138u, 8390u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 2u, 12u, 4u, "user_sgpr[2]->load(+0)", "CBFxaaParam", "SourceTex"},
    {"gxposteffect", "FXAA3HQ.ppo", "FXAA3HQ.ppo", "Pixel(PS)", "None", 19u, 2u, 0x000000006c5edb96ull, 0xe241b99cu, 1936u, 1384u, 258u, 14980u, 0x000c00c7u, 0x00000020u, 32u, 32u, 16u, 1u, 1u, 2u, 4u, 4u, "user_sgpr[2]->load(+0)", "CBFxaaParam", "SourceTex"},
    {"gxposteffect", "LendsSimu_MakeBlurBuffer.ppo", "LendsSimu_MakeBlurBuffer.ppo", "Pixel(PS)", "None", 20u, 2u, 0x000000002c3eebe3ull, 0x6f476127u, 1320u, 492u, 97u, 6954u, 0x000c00c9u, 0x00000020u, 40u, 32u, 16u, 2u, 1u, 2u, 12u, 12u, "user_sgpr[2]->load(+32)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LendsSimu_MakeLensAndMotionBlurBuffer.ppo", "LendsSimu_MakeLensAndMotionBlurBuffer.ppo", "Pixel(PS)", "None", 21u, 2u, 0x00000000e4009b2bull, 0x7c52cb4au, 1784u, 896u, 173u, 10980u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 2u, 21u, 20u, "user_sgpr[2]->load(+64)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LendsSimu_MakeMotionBlurBuffer.ppo", "LendsSimu_MakeMotionBlurBuffer.ppo", "Pixel(PS)", "None", 22u, 2u, 0x00000000b72bb396ull, 0x2a8c078eu, 1532u, 640u, 125u, 8650u, 0x000c0108u, 0x00000020u, 36u, 40u, 16u, 3u, 1u, 2u, 21u, 20u, "user_sgpr[2]->load(+64)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LendsSimu_MakeMotionBlurBufferNoMask.ppo", "LendsSimu_MakeMotionBlurBufferNoMask.ppo", "Pixel(PS)", "None", 23u, 2u, 0x000000008adcefe7ull, 0x8febd0b5u, 1272u, 432u, 82u, 6183u, 0x000c00c6u, 0x00000020u, 28u, 32u, 16u, 2u, 1u, 2u, 21u, 12u, "user_sgpr[2]->load(+32)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_BlurCoC.ppo", "LensSimu2_BlurCoC.ppo", "Pixel(PS)", "None", 24u, 2u, 0x00000000c902266full, 0x814e148du, 1268u, 360u, 76u, 5174u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 1u, 2u, 2u, 12u, 8u, "user_sgpr[2]->load(+16)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu2_BlurVelocity.ppo", "LensSimu2_BlurVelocity.ppo", "Pixel(PS)", "None", 25u, 2u, 0x00000000514b4e45ull, 0xec87e55fu, 1188u, 308u, 59u, 4449u, 0x000c0086u, 0x00000020u, 28u, 24u, 16u, 1u, 1u, 2u, 12u, 4u, "user_sgpr[2]->load(+0)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_ComputeCoC.ppo", "LensSimu2_ComputeCoC.ppo", "Pixel(PS)", "None", 26u, 2u, 0x00000000981046d7ull, 0x0b2b570du, 1244u, 280u, 54u, 7001u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 2u, 2u, 2u, 22u, 16u, "user_sgpr[2]->load(+48)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_ComputeCoCF.ppo", "LensSimu2_ComputeCoCF.ppo", "Pixel(PS)", "None", 27u, 2u, 0x00000000a9c2becaull, 0xffec6e7bu, 1236u, 272u, 53u, 6679u, 0x000c00c0u, 0x00000020u, 4u, 32u, 16u, 2u, 2u, 2u, 22u, 16u, "user_sgpr[2]->load(+48)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_Debug.ppo", "LensSimu2_Debug.ppo", "Pixel(PS)", "None", 28u, 2u, 0x00000000f928e583ull, 0x0b3190eau, 1364u, 360u, 77u, 9094u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 4u, 1u, 2u, 33u, 28u, "user_sgpr[2]->load(+96)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu2_DilateVelocity.ppo", "LensSimu2_DilateVelocity.ppo", "Pixel(PS)", "None", 29u, 2u, 0x000000001534534aull, 0x9c108932u, 3148u, 2128u, 387u, 21456u, 0x000c014eu, 0x00000020u, 60u, 48u, 16u, 1u, 1u, 3u, 30u, 8u, "user_sgpr[2]->load(+16)", "(embedded constant buffer)", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_DownSample2x2RGBM.ppo", "LensSimu2_DownSample2x2RGBM.ppo", "Pixel(PS)", "None", 30u, 2u, 0x00000000ed1e60eeull, 0x6839c7feu, 1048u, 144u, 30u, 4371u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 2u, 1u, 2u, 26u, 12u, "user_sgpr[2]->load(+32)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu2_DownSample2x2RGBMCoC.ppo", "LensSimu2_DownSample2x2RGBMCoC.ppo", "Pixel(PS)", "None", 31u, 2u, 0x00000000f3621495ull, 0xe25b900cu, 1328u, 308u, 59u, 8428u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 2u, 2u, 22u, 24u, "user_sgpr[2]->load(+80)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_DownSample2x2RGBMCoCF.ppo", "LensSimu2_DownSample2x2RGBMCoCF.ppo", "Pixel(PS)", "None", 32u, 2u, 0x00000000b095e72full, 0xb3857094u, 1304u, 288u, 55u, 8075u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 2u, 2u, 22u, 24u, "user_sgpr[2]->load(+80)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_DownSample2x2RGBMF.ppo", "LensSimu2_DownSample2x2RGBMF.ppo", "Pixel(PS)", "None", 33u, 2u, 0x00000000ed1e60eeull, 0x6839c7feu, 1048u, 144u, 30u, 4371u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 2u, 1u, 2u, 26u, 12u, "user_sgpr[2]->load(+32)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu2_FinalComposite.ppo", "LensSimu2_FinalComposite.ppo", "Pixel(PS)", "None", 34u, 2u, 0x0000000004ef5f2full, 0x6611865du, 628u, 164u, 34u, 4770u, 0x000c0102u, 0x00000020u, 12u, 40u, 16u, 3u, 1u, 1u, 0u, 16u, "", "", "g_SourceTex2"},
    {"gxposteffect", "LensSimu2_MakeBlurBuffer.ppo", "LensSimu2_MakeBlurBuffer.ppo", "Pixel(PS)", "None", 35u, 2u, 0x00000000f0fa51feull, 0x0c934c91u, 2600u, 1280u, 244u, 21416u, 0x000c0118u, 0x00000020u, 100u, 40u, 16u, 2u, 1u, 3u, 94u, 16u, "user_sgpr[2]->load(+48)", "(embedded constant buffer)", "g_CoCTex"},
    {"gxposteffect", "LensSimu2_MakeBlurBuffer19.ppo", "LensSimu2_MakeBlurBuffer19.ppo", "Pixel(PS)", "None", 36u, 2u, 0x00000000d61c51ceull, 0x1f24cd94u, 2088u, 1056u, 203u, 18075u, 0x000c0113u, 0x00000020u, 80u, 40u, 16u, 2u, 1u, 3u, 74u, 16u, "user_sgpr[2]->load(+48)", "CBSampleOffsetParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu2_MakeLensAndMotionBlurBuffer.ppo", "LensSimu2_MakeLensAndMotionBlurBuffer.ppo", "Pixel(PS)", "None", 37u, 2u, 0x0000000094059df9ull, 0x7c2c7da6u, 3652u, 2276u, 457u, 30486u, 0x000c015au, 0x00000020u, 108u, 48u, 16u, 3u, 1u, 3u, 94u, 24u, "user_sgpr[2]->load(+80)", "(embedded constant buffer)", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_MakeMotionBlurBuffer.ppo", "LensSimu2_MakeMotionBlurBuffer.ppo", "Pixel(PS)", "None", 38u, 2u, 0x00000000b84726b7ull, 0x6854fadbu, 1492u, 508u, 93u, 7788u, 0x000c0107u, 0x00000020u, 32u, 40u, 16u, 3u, 1u, 2u, 22u, 20u, "user_sgpr[2]->load(+64)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_MakeMotionBlurBufferNoMask.ppo", "LensSimu2_MakeMotionBlurBufferNoMask.ppo", "Pixel(PS)", "None", 39u, 2u, 0x0000000023cf4ed2ull, 0x1675ded9u, 1300u, 360u, 65u, 5666u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 2u, 22u, 12u, "user_sgpr[2]->load(+32)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu2_ScaleCoCPS.ppo", "LensSimu2_ScaleCoCPS.ppo", "Pixel(PS)", "None", 40u, 2u, 0x00000000ebbf4c04ull, 0x1f5af6e9u, 1084u, 216u, 42u, 3676u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 12u, 4u, "user_sgpr[2]->load(+0)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu_BlurCoC.ppo", "LensSimu_BlurCoC.ppo", "Pixel(PS)", "None", 41u, 2u, 0x00000000164cdf00ull, 0x88f8d553u, 1104u, 332u, 63u, 4465u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 2u, 12u, 4u, "user_sgpr[2]->load(+0)", "CBDofParam", "g_CoCTex"},
    {"gxposteffect", "LensSimu_BlurVelocity.ppo", "LensSimu_BlurVelocity.ppo", "Pixel(PS)", "None", 42u, 2u, 0x000000007df9e24aull, 0x0c46b530u, 1092u, 308u, 59u, 4449u, 0x000c0086u, 0x00000020u, 28u, 24u, 16u, 1u, 1u, 2u, 12u, 4u, "user_sgpr[2]->load(+0)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu_ComputeCoC.ppo", "LensSimu_ComputeCoC.ppo", "Pixel(PS)", "None", 43u, 2u, 0x000000005850880cull, 0xff11ca0du, 1136u, 276u, 54u, 6599u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 2u, 2u, 2u, 21u, 16u, "user_sgpr[2]->load(+48)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu_ComputeCoCF.ppo", "LensSimu_ComputeCoCF.ppo", "Pixel(PS)", "None", 44u, 2u, 0x000000007f677cb7ull, 0x0c2b2d16u, 1132u, 264u, 52u, 6277u, 0x000c00c0u, 0x00000020u, 4u, 32u, 16u, 2u, 2u, 2u, 21u, 16u, "user_sgpr[2]->load(+48)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu_Debug.ppo", "LensSimu_Debug.ppo", "Pixel(PS)", "None", 45u, 2u, 0x000000003fa4d59full, 0xc415618bu, 1200u, 336u, 72u, 7622u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 3u, 1u, 2u, 25u, 20u, "user_sgpr[2]->load(+64)", "CBDofParam", "g_SourceTex"},
    {"gxposteffect", "LensSimu_DilateVelocity.ppo", "LensSimu_DilateVelocity.ppo", "Pixel(PS)", "None", 46u, 2u, 0x00000000c6ea8d61ull, 0xb3da9cefu, 3048u, 2128u, 387u, 21222u, 0x000c014eu, 0x00000020u, 60u, 48u, 16u, 1u, 1u, 3u, 30u, 8u, "user_sgpr[2]->load(+16)", "(embedded constant buffer)", "g_VelocityTex"},
    {"gxposteffect", "LensSimu_DownSample2x2RGBM.ppo", "LensSimu_DownSample2x2RGBM.ppo", "Pixel(PS)", "None", 47u, 2u, 0x00000000cfbd6ad9ull, 0x9b87d2ffu, 456u, 108u, 23u, 2093u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTex"},
    {"gxposteffect", "LensSimu_DownSample2x2RGBMCoC.ppo", "LensSimu_DownSample2x2RGBMCoC.ppo", "Pixel(PS)", "None", 48u, 2u, 0x000000002ef21350ull, 0x180b38c8u, 1260u, 340u, 67u, 8259u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 2u, 2u, 21u, 24u, "user_sgpr[2]->load(+80)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu_DownSample2x2RGBMCoCF.ppo", "LensSimu_DownSample2x2RGBMCoCF.ppo", "Pixel(PS)", "None", 49u, 2u, 0x0000000052e4f791ull, 0x9c9a8f8au, 1236u, 316u, 62u, 7872u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 2u, 2u, 21u, 24u, "user_sgpr[2]->load(+80)", "CBDofParam", "g_VelocityTex"},
    {"gxposteffect", "LensSimu_DownSample2x2RGBMF.ppo", "LensSimu_DownSample2x2RGBMF.ppo", "Pixel(PS)", "None", 50u, 2u, 0x00000000cfbd6ad9ull, 0x9b87d2ffu, 456u, 108u, 23u, 2093u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTex"},
    {"gxposteffect", "LensSimu_FinalComposite.ppo", "LensSimu_FinalComposite.ppo", "Pixel(PS)", "None", 51u, 2u, 0x000000008248f5aaull, 0xffb312a2u, 592u, 176u, 33u, 4300u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 2u, 1u, 1u, 0u, 8u, "", "", "g_SourceTex"},
    {"gxposteffect", "LineColorParaffinOverrayPS.ppo", "LineColorParaffinOverrayPS.ppo", "Pixel(PS)", "None", 52u, 2u, 0x0000000031d97e63ull, 0x6f5d5c26u, 1000u, 520u, 101u, 8135u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 22u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "LineColorParaffinPS.ppo", "LineColorParaffinPS.ppo", "Pixel(PS)", "None", 53u, 2u, 0x000000005d38bb03ull, 0xec8ea9f5u, 704u, 328u, 67u, 6013u, 0x000c0081u, 0x00000010u, 8u, 24u, 8u, 0u, 0u, 1u, 22u, 0u, "user_sgpr[4]", "cbSampleOffset", ""},
    {"gxposteffect", "MotionBlur.ppo", "MotionBlur.ppo", "Pixel(PS)", "None", 54u, 2u, 0x00000000107109bcull, 0x167b163au, 912u, 504u, 90u, 5924u, 0x000c00c9u, 0x00000020u, 40u, 32u, 16u, 2u, 1u, 1u, 0u, 8u, "", "", "g_VelocityTex"},
    {"gxposteffect", "MotionBlurDilateVelocity.ppo", "MotionBlurDilateVelocity.ppo", "Pixel(PS)", "None", 55u, 2u, 0x00000000abf92450ull, 0x9c7abaa2u, 1844u, 1080u, 163u, 9497u, 0x000c008au, 0x00000020u, 44u, 24u, 16u, 1u, 1u, 2u, 32u, 4u, "user_sgpr[2]->load(+0)", "cbFilterParam", "g_VelocityTex"},
    {"gxposteffect", "MotionBlurWriteCMBVelocity.ppo", "MotionBlurWriteCMBVelocity.ppo", "Pixel(PS)", "None", 56u, 2u, 0x0000000052b0abfdull, 0x7c369b76u, 1016u, 252u, 52u, 5813u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 34u, 4u, "user_sgpr[2]->load(+0)", "cbFilterParam", "g_DepthTex"},
    {"gxposteffect", "MotionBlurWriteCMBVelocityNoMove.ppo", "MotionBlurWriteCMBVelocityNoMove.ppo", "Pixel(PS)", "None", 57u, 2u, 0x00000000dfff41caull, 0x6fc42b45u, 912u, 140u, 29u, 3089u, 0x000c0080u, 0x00000020u, 4u, 24u, 16u, 1u, 1u, 2u, 10u, 4u, "user_sgpr[2]->load(+0)", "cbFilterParam", "g_DepthTex"},
    {"gxposteffect", "OutputColor.ppo", "OutputColor.ppo", "Pixel(PS)", "None", 58u, 2u, 0x000000009eb30215ull, 0x028efab8u, 356u, 52u, 11u, 1993u, 0x000c0040u, 0x00000010u, 4u, 16u, 8u, 0u, 0u, 1u, 4u, 0u, "user_sgpr[4]", "cbColor", ""},
    {"gxposteffect", "OutputOne.ppo", "OutputOne.ppo", "Pixel(PS)", "None", 59u, 2u, 0x000000006343bb37ull, 0xebc5cd79u, 240u, 28u, 5u, 1128u, 0x000c0000u, 0x00000004u, 4u, 8u, 2u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxposteffect", "PointSampleColor1.ppo", "PointSampleColor1.ppo", "Pixel(PS)", "None", 60u, 2u, 0x00000000f7229091ull, 0x18396d4bu, 416u, 60u, 12u, 1714u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTex"},
    {"gxposteffect", "PointSampleColorClamp1.ppo", "PointSampleColorClamp1.ppo", "Pixel(PS)", "None", 61u, 2u, 0x00000000ccbf44a6ull, 0xeb7ac862u, 440u, 76u, 16u, 1818u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTex"},
    {"gxposteffect", "RadiationColorParaffinOverrayPS.ppo", "RadiationColorParaffinOverrayPS.ppo", "Pixel(PS)", "None", 62u, 2u, 0x0000000097e01a6full, 0x81b40828u, 868u, 380u, 72u, 6409u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "RadiationColorParaffinPS.ppo", "RadiationColorParaffinPS.ppo", "Pixel(PS)", "None", 63u, 2u, 0x00000000a1eeb6a2ull, 0xc4fb7c02u, 564u, 184u, 37u, 4292u, 0x000c0081u, 0x00000010u, 8u, 24u, 8u, 0u, 0u, 1u, 14u, 0u, "user_sgpr[4]", "cbSampleOffset", ""},
    {"gxposteffect", "Sample13Weight.ppo", "Sample13Weight.ppo", "Pixel(PS)", "None", 64u, 2u, 0x000000000f61fd27ull, 0x729399eeu, 1444u, 800u, 159u, 16054u, 0x000c020du, 0x00000020u, 56u, 72u, 16u, 1u, 1u, 3u, 52u, 8u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "Sample13WeightforDeferredSnow.ppo", "Sample13WeightforDeferredSnow.ppo", "Pixel(PS)", "None", 65u, 2u, 0x000000002c9d680cull, 0x02ed6c98u, 1528u, 788u, 153u, 17275u, 0x000c028bu, 0x00000020u, 48u, 88u, 16u, 2u, 2u, 3u, 52u, 20u, "user_sgpr[2]->load(+64)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "Sample13WeightforGlow.ppo", "Sample13WeightforGlow.ppo", "Pixel(PS)", "None", 66u, 2u, 0x000000008c08444cull, 0xff88f8acu, 1536u, 780u, 154u, 16115u, 0x000c020au, 0x00000020u, 44u, 72u, 16u, 1u, 1u, 4u, 52u, 12u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "Sample13WeightforGlowKernel.ppo", "Sample13WeightforGlowKernel.ppo", "Pixel(PS)", "None", 67u, 2u, 0x00000000a61e325dull, 0xf124d55eu, 1584u, 820u, 164u, 16817u, 0x000c020du, 0x00000020u, 56u, 72u, 16u, 1u, 1u, 4u, 52u, 12u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "Sample13WeightforSnow.ppo", "Sample13WeightforSnow.ppo", "Pixel(PS)", "None", 68u, 2u, 0x000000006c96640cull, 0x9b692143u, 1528u, 788u, 153u, 17275u, 0x000c028bu, 0x00000020u, 48u, 88u, 16u, 2u, 2u, 3u, 52u, 20u, "user_sgpr[2]->load(+64)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "Sample13WeightRotation.ppo", "Sample13WeightRotation.ppo", "Pixel(PS)", "None", 69u, 2u, 0x00000000ecb1fb03ull, 0x1847d20bu, 1760u, 1008u, 195u, 19873u, 0x000c020du, 0x00000020u, 56u, 72u, 16u, 2u, 2u, 3u, 52u, 20u, "user_sgpr[2]->load(+64)", "cbSampleOffset", "g_RandomRotationTex"},
    {"gxposteffect", "SampleColor1.ppo", "SampleColor1.ppo", "Pixel(PS)", "None", 70u, 2u, 0x0000000086e96401ull, 0x9cedf186u, 412u, 60u, 12u, 1714u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTex"},
    {"gxposteffect", "SampleColor4.ppo", "SampleColor4.ppo", "Pixel(PS)", "None", 71u, 2u, 0x00000000bdb825dcull, 0x0280b161u, 692u, 224u, 49u, 4995u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "SampleWanderingVisionColorClamp1.ppo", "SampleWanderingVisionColorClamp1.ppo", "Pixel(PS)", "None", 72u, 2u, 0x000000008642d5daull, 0x9baf39f6u, 776u, 192u, 36u, 5638u, 0x000c0101u, 0x00000020u, 8u, 40u, 16u, 2u, 2u, 2u, 6u, 16u, "user_sgpr[2]->load(+48)", "cbSampleOffset", "g_RandomRotationTex"},
    {"gxposteffect", "SampleWanderingVisionFeedbackBlur.ppo", "SampleWanderingVisionFeedbackBlur.ppo", "Pixel(PS)", "None", 73u, 2u, 0x000000001718cc23ull, 0xff750a89u, 696u, 112u, 24u, 4528u, 0x000c0102u, 0x00000020u, 12u, 40u, 16u, 2u, 2u, 2u, 1u, 16u, "user_sgpr[2]->load(+48)", "cbSampleOffset", "g_SourceTex"},
    {"gxposteffect", "ScreenSpaceAO.ppo", "ScreenSpaceAO.ppo", "Pixel(PS)", "None", 74u, 2u, 0x00000000b6b33378ull, 0x81ece43du, 1504u, 820u, 140u, 12476u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 1u, 1u, 3u, 32u, 8u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "ScreenSpaceAO2.ppo", "ScreenSpaceAO2.ppo", "Pixel(PS)", "None", 75u, 2u, 0x000000007b1a6a02ull, 0x6830f2e5u, 2156u, 924u, 185u, 18175u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 2u, 2u, 4u, 65u, 24u, "user_sgpr[2]->load(+80)", "cbSSAOParam2", "g_DepthTex"},
    {"gxposteffect", "ScreenSpaceAO2_BilateralUpScale.ppo", "ScreenSpaceAO2_BilateralUpScale.ppo", "Pixel(PS)", "None", 76u, 2u, 0x00000000c45c51cbull, 0x61e23b16u, 1684u, 548u, 104u, 8515u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 2u, 2u, 3u, 20u, 20u, "user_sgpr[2]->load(+64)", "cbSSAOParam2", "g_SrcAOTex"},
    {"gxposteffect", "ScreenSpaceAO2_ReProj.ppo", "ScreenSpaceAO2_ReProj.ppo", "Pixel(PS)", "None", 77u, 2u, 0x0000000063bf8617ull, 0x0cd8ea4fu, 2528u, 1164u, 233u, 25047u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 3u, 4u, 4u, 65u, 40u, "user_sgpr[2]->load(+144)", "cbSSAOParam2", "g_DepthTex"},
    {"gxposteffect", "ScreenSpaceAO2_WithNormal.ppo", "ScreenSpaceAO2_WithNormal.ppo", "Pixel(PS)", "None", 78u, 2u, 0x00000000cbbca5dcull, 0xeceacb72u, 2404u, 1072u, 218u, 23154u, 0x000c0207u, 0x00000020u, 32u, 72u, 16u, 3u, 3u, 4u, 44u, 36u, "user_sgpr[2]->load(+128)", "cbSSAOParam2", "g_DepthTex"},
    {"gxposteffect", "ScreenSpaceAO2_WithNormalReProj.ppo", "ScreenSpaceAO2_WithNormalReProj.ppo", "Pixel(PS)", "None", 79u, 2u, 0x0000000083da8467ull, 0x92228cdcu, 2732u, 1316u, 267u, 29756u, 0x000c0247u, 0x00000020u, 32u, 80u, 16u, 4u, 4u, 4u, 64u, 48u, "user_sgpr[2]->load(+176)", "cbSSAOParam2", "g_DepthTex"},
    {"gxposteffect", "ScreenSpaceAOAndCombine.ppo", "ScreenSpaceAOAndCombine.ppo", "Pixel(PS)", "None", 80u, 2u, 0x00000000b5f8c858ull, 0xc478cc9bu, 1656u, 876u, 151u, 14750u, 0x000c01c7u, 0x00000020u, 32u, 64u, 16u, 2u, 2u, 3u, 32u, 20u, "user_sgpr[2]->load(+64)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "ScreenSpaceDirectionalAO.ppo", "ScreenSpaceDirectionalAO.ppo", "Pixel(PS)", "None", 81u, 2u, 0x00000000ca2d8d0aull, 0x81305155u, 2292u, 1596u, 281u, 17719u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 1u, 1u, 3u, 32u, 8u, "user_sgpr[2]->load(+16)", "cbSampleOffset", "g_DepthTex"},
    {"gxposteffect", "ToneMap_AdaptedLuminanceCopyPS.ppo", "ToneMap_AdaptedLuminanceCopyPS.ppo", "Pixel(PS)", "None", 82u, 2u, 0x00000000627951b0ull, 0x5d71edd5u, 1080u, 96u, 18u, 2638u, 0x000c0080u, 0x00000020u, 4u, 24u, 16u, 1u, 1u, 2u, 17u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_AdaptedLuminancePS.ppo", "ToneMap_AdaptedLuminancePS.ppo", "Pixel(PS)", "None", 83u, 2u, 0x00000000c2bb6351ull, 0xe5b58083u, 1228u, 188u, 36u, 4820u, 0x000c00c0u, 0x00000020u, 4u, 32u, 16u, 2u, 1u, 2u, 20u, 12u, "user_sgpr[2]->load(+32)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_BloomCompositePS.ppo", "ToneMap_BloomCompositePS.ppo", "Pixel(PS)", "None", 84u, 2u, 0x000000002f82498dull, 0x7bbcd203u, 1320u, 196u, 38u, 5984u, 0x000c0102u, 0x00000020u, 12u, 40u, 16u, 3u, 1u, 2u, 40u, 20u, "user_sgpr[2]->load(+64)", "cbToneMap", "g_SourceTexture2"},
    {"gxposteffect", "ToneMap_BloomCompositeSinglePS.ppo", "ToneMap_BloomCompositeSinglePS.ppo", "Pixel(PS)", "None", 85u, 2u, 0x0000000044a2ed89ull, 0x863ef977u, 1132u, 100u, 22u, 3210u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 40u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_BloomCompositeUpScalePS.ppo", "ToneMap_BloomCompositeUpScalePS.ppo", "Pixel(PS)", "None", 86u, 2u, 0x00000000714ff44eull, 0xfff6c44fu, 1132u, 100u, 22u, 2819u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 41u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_BloomGaussBlurPS.ppo", "ToneMap_BloomGaussBlurPS.ppo", "Pixel(PS)", "None", 87u, 2u, 0x00000000825dd04bull, 0x7cd69bffu, 808u, 320u, 71u, 8561u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 1u, 1u, 2u, 28u, 4u, "user_sgpr[2]->load(+0)", "cbToneMapSampleOffset", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_BrightPassAndScalePS.ppo", "ToneMap_BrightPassAndScalePS.ppo", "Pixel(PS)", "None", 88u, 2u, 0x000000001c3cbd98ull, 0x81ae3c1bu, 1724u, 584u, 115u, 8717u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 36u, 8u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_BrightPassPS.ppo", "ToneMap_BrightPassPS.ppo", "Pixel(PS)", "None", 89u, 2u, 0x00000000e93a1097ull, 0xb312ac85u, 1404u, 272u, 55u, 5759u, 0x000c00c1u, 0x00000020u, 8u, 32u, 16u, 1u, 1u, 3u, 36u, 8u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_ColorToLuminancePS.ppo", "ToneMap_ColorToLuminancePS.ppo", "Pixel(PS)", "None", 90u, 2u, 0x00000000fb10e078ull, 0x7b55a2b3u, 2452u, 1308u, 224u, 14883u, 0x000c00ccu, 0x00000020u, 52u, 32u, 16u, 1u, 1u, 3u, 62u, 8u, "user_sgpr[2]->load(+16)", "cbToneMapSampleOffset", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_DivideToneMapPS.ppo", "ToneMap_DivideToneMapPS.ppo", "Pixel(PS)", "None", 91u, 2u, 0x00000000a74e0dbcull, 0xb3fcb6e2u, 1172u, 148u, 31u, 4036u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_DownSample2x2PS.ppo", "ToneMap_DownSample2x2PS.ppo", "Pixel(PS)", "None", 92u, 2u, 0x00000000b8b0c759ull, 0x118fb1c5u, 408u, 60u, 12u, 1714u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_ExpToneMapPS.ppo", "ToneMap_ExpToneMapPS.ppo", "Pixel(PS)", "None", 93u, 2u, 0x00000000bb3593dbull, 0x9573abe9u, 1168u, 148u, 31u, 3973u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_FilmicToneMapPS.ppo", "ToneMap_FilmicToneMapPS.ppo", "Pixel(PS)", "None", 94u, 2u, 0x00000000f824fca6ull, 0xc31bd4edu, 1332u, 308u, 60u, 4960u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_GammaPS.ppo", "ToneMap_GammaPS.ppo", "Pixel(PS)", "None", 95u, 2u, 0x00000000f4baa5a2ull, 0xf62d5c31u, 1124u, 112u, 25u, 3792u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_NonePS.ppo", "ToneMap_NonePS.ppo", "Pixel(PS)", "None", 96u, 2u, 0x00000000e02eed2eull, 0xfff8f13du, 412u, 60u, 12u, 1714u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_ReinhardToneMapPS.ppo", "ToneMap_ReinhardToneMapPS.ppo", "Pixel(PS)", "None", 97u, 2u, 0x00000000a51278f7ull, 0x8ff604d3u, 1204u, 184u, 39u, 4308u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_ResetAdaptedLuminancePS.ppo", "ToneMap_ResetAdaptedLuminancePS.ppo", "Pixel(PS)", "None", 98u, 2u, 0x000000007cfe6e91ull, 0x2a014dcbu, 276u, 48u, 8u, 1184u, 0x000c0000u, 0x00000004u, 4u, 8u, 2u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxposteffect", "ToneMap_ResetSmoothAdaptedLuminancePS.ppo", "ToneMap_ResetSmoothAdaptedLuminancePS.ppo", "Pixel(PS)", "None", 99u, 2u, 0x000000003cc2c2e1ull, 0xc4e1d817u, 1168u, 172u, 31u, 3341u, 0x000c0080u, 0x00000020u, 4u, 24u, 16u, 1u, 1u, 2u, 19u, 4u, "user_sgpr[2]->load(+0)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_RGBMEncodePS.ppo", "ToneMap_RGBMEncodePS.ppo", "Pixel(PS)", "None", 100u, 2u, 0x0000000012d77885ull, 0x059a9504u, 452u, 96u, 20u, 1942u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_ScaleLuminancePS.ppo", "ToneMap_ScaleLuminancePS.ppo", "Pixel(PS)", "None", 101u, 2u, 0x00000000d7ecf71full, 0x0293ddf0u, 680u, 188u, 39u, 4327u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 2u, 14u, 4u, "user_sgpr[2]->load(+0)", "cbToneMapSampleOffset", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_TabledToneMapLUTPS.ppo", "ToneMap_TabledToneMapLUTPS.ppo", "Pixel(PS)", "None", 102u, 2u, 0x00000000b691348aull, 0xff95a63fu, 1516u, 324u, 65u, 8332u, 0x000c0102u, 0x00000020u, 12u, 40u, 16u, 4u, 1u, 2u, 29u, 28u, "user_sgpr[2]->load(+96)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_TabledToneMapPS.ppo", "ToneMap_TabledToneMapPS.ppo", "Pixel(PS)", "None", 103u, 2u, 0x000000004f4afe5cull, 0x619b9e95u, 1388u, 260u, 55u, 6868u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 3u, 1u, 2u, 29u, 20u, "user_sgpr[2]->load(+64)", "cbToneMap", "g_SourceTexture"},
    {"gxposteffect", "ToneMap_VisualizeLUTPS.ppo", "ToneMap_VisualizeLUTPS.ppo", "Pixel(PS)", "None", 104u, 2u, 0x00000000444b1983ull, 0x2afcc81du, 460u, 128u, 25u, 2011u, 0x000c0041u, 0x00000018u, 8u, 16u, 12u, 1u, 0u, 0u, 0u, 0u, "", "", "g_ColorGradingLUTTexture"},
    {"gxposteffect", "ToneMap_WriteFilmicToneMappingTablePS.ppo", "ToneMap_WriteFilmicToneMappingTablePS.ppo", "Pixel(PS)", "None", 105u, 2u, 0x00000000801791beull, 0x6f2319eau, 1088u, 152u, 29u, 2095u, 0x000c0040u, 0x00000010u, 4u, 16u, 8u, 0u, 0u, 1u, 10u, 0u, "user_sgpr[4]", "cbToneMap", ""},
    {"gxposteffect", "ToneMap_WriteSensitometricToneMappingTablePS.ppo", "ToneMap_WriteSensitometricToneMappingTablePS.ppo", "Pixel(PS)", "None", 106u, 2u, 0x00000000529f7122ull, 0x1649443au, 1068u, 132u, 27u, 2263u, 0x000c0040u, 0x00000010u, 4u, 16u, 8u, 0u, 0u, 1u, 10u, 0u, "user_sgpr[4]", "cbToneMap", ""},
    {"gxposteffect", "FXAA.vpo", "FXAA.vpo", "Vertex(VS)", "None", 107u, 1u, 0x00000000dc609f83ull, 0x72e46db5u, 712u, 116u, 18u, 2244u, 0x000c0042u, 0x00000018u, 12u, 16u, 12u, 0u, 0u, 1u, 4u, 0u, "user_sgpr[8]", "CBFxaaParam", ""},
    {"gxposteffect", "ScreenSpaceQuadShader.vpo", "ScreenSpaceQuadShader.vpo", "Vertex(VS)", "None", 108u, 1u, 0x000000000b0acf50ull, 0x05730b23u, 428u, 40u, 7u, 1215u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxposteffect", "ScreenSpaceQuadShaderZ.vpo", "ScreenSpaceQuadShaderZ.vpo", "Vertex(VS)", "None", 109u, 1u, 0x000000006a51f1d8ull, 0x0b52961fu, 540u, 52u, 10u, 1588u, 0x000c0042u, 0x00000018u, 12u, 16u, 12u, 0u, 0u, 1u, 1u, 0u, "user_sgpr[8]", "cbVtxParam", ""},
    {"gxposteffect", "ToneMapAdaptVS.vpo", "ToneMapAdaptVS.vpo", "Vertex(VS)", "None", 110u, 1u, 0x00000000bd0ee4e9ull, 0xecf0eb47u, 476u, 60u, 11u, 1573u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 0u, 0u, 0u, 0u, "", "", "g_LuminanceTexture"},
    {"gxposteffect", "ToneMapVS.vpo", "ToneMapVS.vpo", "Vertex(VS)", "None", 111u, 1u, 0x00000000360418aaull, 0x95b5d99bu, 396u, 36u, 6u, 1190u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxposteffect", "CustomDofLutCopy.cpo", "CustomDofLutCopy.cpo", "Compute(CS)", "None", 112u, 4u, 0x000000001f2b3c9eull, 0x7c2ce45cu, 760u, 264u, 57u, 8872u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 0u, 3u, 4u, 4u, "user_sgpr[12]", "", ""},
    {"gxposteffect", "LutBlend2DTo3DCs.cpo", "LutBlend2DTo3DCs.cpo", "Compute(CS)", "None", 113u, 4u, 0x000000000f6d2ebfull, 0x926e3e63u, 732u, 156u, 31u, 5651u, 0x000c0103u, 0x00000a20u, 16u, 40u, 16u, 3u, 0u, 2u, 13u, 16u, "user_sgpr[12]", "", ""},
    {"gxposteffect", "LutCopy2DTo3DCs.cpo", "LutCopy2DTo3DCs.cpo", "Compute(CS)", "None", 114u, 4u, 0x00000000200820a1ull, 0xb3e14a88u, 360u, 92u, 20u, 3787u, 0x000c00c2u, 0x00000a18u, 12u, 32u, 12u, 2u, 0u, 1u, 0u, 8u, "", "", ""},
    {"gxposteffect", "LutCopy3DTo2DCs.cpo", "LutCopy3DTo2DCs.cpo", "Compute(CS)", "None", 115u, 4u, 0x0000000000f423c6ull, 0x8ff6d847u, 360u, 92u, 20u, 3779u, 0x000c00c2u, 0x00000a18u, 12u, 32u, 12u, 2u, 0u, 1u, 0u, 8u, "", "", ""},
    {"gxposteffect", "ToneMap_Blend2Dto3DLUTCS.cpo", "ToneMap_Blend2Dto3DLUTCS.cpo", "Compute(CS)", "None", 116u, 4u, 0x00000000478c3fa1ull, 0xff923f26u, 576u, 128u, 28u, 4923u, 0x000c0103u, 0x00000a20u, 16u, 40u, 16u, 3u, 0u, 2u, 1u, 16u, "user_sgpr[12]", "", ""},
    {"gxposteffect", "ToneMap_Copy2Dto3DLUTCS.cpo", "ToneMap_Copy2Dto3DLUTCS.cpo", "Compute(CS)", "None", 117u, 4u, 0x00000000389abde0ull, 0x056ea8c6u, 344u, 68u, 14u, 3082u, 0x000c0042u, 0x00000a18u, 12u, 16u, 12u, 2u, 0u, 1u, 0u, 8u, "", "", ""},
    {"gxposteffect", "ToneMap_MakeDefaultLUTCS.cpo", "ToneMap_MakeDefaultLUTCS.cpo", "Compute(CS)", "None", 118u, 4u, 0x00000000eb0e828dull, 0x68f8487du, 296u, 84u, 16u, 1837u, 0x000c0042u, 0x00000a18u, 12u, 16u, 12u, 1u, 0u, 0u, 0u, 0u, "", "", ""},
    // --- Bundle: gxrenderershader (Tier 8/10, 111 shaders) ---
    {"gxrenderershader", "Filter\\GXDeferred_Debug.ppo", "Filter_GXDeferred_Debug.ppo", "Pixel(PS)", "None", 0u, 2u, 0x00000000458a4dd3ull, 0x7594f7b5u, 3664u, 1748u, 339u, 27436u, 0x000c0184u, 0x0000001cu, 20u, 56u, 14u, 8u, 2u, 3u, 8u, 272u, "user_sgpr[2]->load(+0)", "cbPerFrame", "g_LightPrepassAttrTexture"},
    {"gxrenderershader", "Filter\\GXEnvDecal.ppo", "Filter_GXEnvDecal.ppo", "Pixel(PS)", "None", 1u, 2u, 0x000000003029db84ull, 0xeb94ee9fu, 4072u, 936u, 184u, 17363u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 2u, 3u, 196u, 28u, "user_sgpr[2]->load(+80)", "cbSceneParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXEnvDecal_Deferred.ppo", "Filter_GXEnvDecal_Deferred.ppo", "Pixel(PS)", "None", 2u, 2u, 0x000000003b7ae589ull, 0xb396e10du, 4072u, 936u, 184u, 17363u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 2u, 3u, 196u, 28u, "user_sgpr[2]->load(+80)", "cbSceneParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DDirLight1.ppo", "Filter_GXLightAcc_DDirLight1.ppo", "Pixel(PS)", "None", 3u, 2u, 0x0000000031b90cc5ull, 0x92c2a4aau, 4324u, 2512u, 504u, 41821u, 0x000c024cu, 0x00000020u, 52u, 80u, 16u, 5u, 2u, 3u, 44u, 44u, "user_sgpr[2]->load(+144)", "cbLightAccParam", "g_LightPrepassAttrTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DDirLightShadowed1.ppo", "Filter_GXLightAcc_DDirLightShadowed1.ppo", "Pixel(PS)", "None", 4u, 2u, 0x00000000d92266b9ull, 0x72cbebb7u, 5788u, 2604u, 523u, 42897u, 0x000c014du, 0x00000020u, 56u, 48u, 16u, 5u, 2u, 4u, 80u, 48u, "user_sgpr[2]->load(+176)", "cbSceneParam", "g_GBuffer3Texture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DLineLight1.ppo", "Filter_GXLightAcc_DLineLight1.ppo", "Pixel(PS)", "None", 5u, 2u, 0x0000000002aee188ull, 0xb396efa1u, 2528u, 1164u, 244u, 22892u, 0x000c0106u, 0x00000020u, 28u, 40u, 16u, 5u, 2u, 3u, 28u, 44u, "user_sgpr[2]->load(+144)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DLineLight2.ppo", "Filter_GXLightAcc_DLineLight2.ppo", "Pixel(PS)", "None", 6u, 2u, 0x00000000c37e6323ull, 0x866f206au, 3112u, 1752u, 367u, 30362u, 0x000c0108u, 0x00000020u, 36u, 40u, 16u, 5u, 2u, 3u, 38u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DLineLight3.ppo", "Filter_GXLightAcc_DLineLight3.ppo", "Pixel(PS)", "None", 7u, 2u, 0x000000003b397883ull, 0x955c475bu, 3696u, 2332u, 488u, 37828u, 0x000c010au, 0x00000020u, 44u, 40u, 16u, 5u, 2u, 3u, 58u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DLineLight4.ppo", "Filter_GXLightAcc_DLineLight4.ppo", "Pixel(PS)", "None", 8u, 2u, 0x00000000d3143e39ull, 0x68ba1316u, 4272u, 2908u, 608u, 45222u, 0x000c010au, 0x00000020u, 44u, 40u, 16u, 5u, 2u, 3u, 78u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight1.ppo", "Filter_GXLightAcc_DPointLight1.ppo", "Pixel(PS)", "None", 9u, 2u, 0x00000000370174f7ull, 0xf892dcceu, 2384u, 1064u, 226u, 21709u, 0x000c0106u, 0x00000020u, 28u, 40u, 16u, 5u, 2u, 3u, 28u, 44u, "user_sgpr[2]->load(+144)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight10.ppo", "Filter_GXLightAcc_DPointLight10.ppo", "Pixel(PS)", "None", 10u, 2u, 0x00000000201cb43dull, 0x6f30e2ffu, 6700u, 5372u, 1150u, 78232u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 158u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight11.ppo", "Filter_GXLightAcc_DPointLight11.ppo", "Pixel(PS)", "None", 11u, 2u, 0x000000005f76eb3dull, 0xc3fcfc78u, 7180u, 5852u, 1253u, 84503u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 174u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight12.ppo", "Filter_GXLightAcc_DPointLight12.ppo", "Pixel(PS)", "None", 12u, 2u, 0x00000000799bfa32ull, 0x18a95843u, 7660u, 6332u, 1356u, 90774u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 190u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight2.ppo", "Filter_GXLightAcc_DPointLight2.ppo", "Pixel(PS)", "None", 13u, 2u, 0x00000000600f86b0ull, 0xffb382e9u, 2872u, 1552u, 331u, 28064u, 0x000c0108u, 0x00000020u, 36u, 40u, 16u, 5u, 2u, 3u, 30u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight3.ppo", "Filter_GXLightAcc_DPointLight3.ppo", "Pixel(PS)", "None", 14u, 2u, 0x0000000089f9ee3eull, 0x6f2a4893u, 3352u, 2032u, 434u, 34395u, 0x000c010au, 0x00000020u, 44u, 40u, 16u, 5u, 2u, 3u, 46u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight4.ppo", "Filter_GXLightAcc_DPointLight4.ppo", "Pixel(PS)", "None", 15u, 2u, 0x00000000149432ffull, 0x888699ceu, 3832u, 2508u, 536u, 40666u, 0x000c010au, 0x00000020u, 44u, 40u, 16u, 5u, 2u, 3u, 62u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight5.ppo", "Filter_GXLightAcc_DPointLight5.ppo", "Pixel(PS)", "None", 16u, 2u, 0x000000005de04266ull, 0x0c55e23bu, 4296u, 2972u, 635u, 46877u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 78u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight6.ppo", "Filter_GXLightAcc_DPointLight6.ppo", "Pixel(PS)", "None", 17u, 2u, 0x000000005d7150daull, 0x0c554025u, 4776u, 3452u, 738u, 53148u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 94u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight7.ppo", "Filter_GXLightAcc_DPointLight7.ppo", "Pixel(PS)", "None", 18u, 2u, 0x000000008d5387d9ull, 0x9b9d06cbu, 5256u, 3932u, 841u, 59419u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 110u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight8.ppo", "Filter_GXLightAcc_DPointLight8.ppo", "Pixel(PS)", "None", 19u, 2u, 0x00000000f81596cdull, 0x0c228674u, 5736u, 4412u, 944u, 65690u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 126u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLight9.ppo", "Filter_GXLightAcc_DPointLight9.ppo", "Pixel(PS)", "None", 20u, 2u, 0x000000000b2ace4cull, 0xebed32d4u, 6216u, 4892u, 1047u, 71961u, 0x000c010bu, 0x00000020u, 48u, 40u, 16u, 5u, 2u, 3u, 142u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLightShadowed1.ppo", "Filter_GXLightAcc_DPointLightShadowed1.ppo", "Pixel(PS)", "None", 21u, 2u, 0x00000000f40da260ull, 0x9bed8b9au, 2796u, 1336u, 271u, 26613u, 0x000c0187u, 0x0000001cu, 32u, 56u, 14u, 6u, 2u, 4u, 28u, 272u, "user_sgpr[2]->load(+0)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLightShadowed2.ppo", "Filter_GXLightAcc_DPointLightShadowed2.ppo", "Pixel(PS)", "None", 22u, 2u, 0x00000000b5b52d63ull, 0x11eb443bu, 3604u, 2092u, 421u, 37144u, 0x000c0189u, 0x0000001cu, 40u, 56u, 14u, 7u, 2u, 4u, 48u, 272u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLightShadowed3.ppo", "Filter_GXLightAcc_DPointLightShadowed3.ppo", "Pixel(PS)", "None", 23u, 2u, 0x00000000bb2ac1cbull, 0xec0422abu, 4396u, 2836u, 568u, 47539u, 0x000c018bu, 0x0000001cu, 48u, 56u, 14u, 8u, 2u, 4u, 72u, 272u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DPointLightShadowed4.ppo", "Filter_GXLightAcc_DPointLightShadowed4.ppo", "Pixel(PS)", "None", 24u, 2u, 0x00000000205c82b3ull, 0x1fd0195fu, 5156u, 3544u, 706u, 57749u, 0x000c018cu, 0x0000001cu, 52u, 56u, 14u, 9u, 2u, 4u, 96u, 272u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLight1.ppo", "Filter_GXLightAcc_DSpotLight1.ppo", "Pixel(PS)", "None", 25u, 2u, 0x00000000bae5454eull, 0x8f115503u, 3088u, 1664u, 358u, 32052u, 0x000c0189u, 0x00000020u, 40u, 56u, 16u, 5u, 2u, 3u, 32u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLight2.ppo", "Filter_GXLightAcc_DSpotLight2.ppo", "Pixel(PS)", "None", 26u, 2u, 0x00000000896e7242ull, 0x1fc37250u, 4152u, 2728u, 594u, 47690u, 0x000c018au, 0x00000020u, 44u, 56u, 16u, 5u, 2u, 3u, 64u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLight3.ppo", "Filter_GXLightAcc_DSpotLight3.ppo", "Pixel(PS)", "None", 27u, 2u, 0x0000000002cd8f9bull, 0x059de200u, 5184u, 3756u, 821u, 63381u, 0x000c018bu, 0x00000020u, 48u, 56u, 16u, 5u, 2u, 3u, 96u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLight4.ppo", "Filter_GXLightAcc_DSpotLight4.ppo", "Pixel(PS)", "None", 28u, 2u, 0x000000008ea5be99ull, 0x6f2303c3u, 6216u, 4792u, 1050u, 79078u, 0x000c018bu, 0x00000020u, 48u, 56u, 16u, 5u, 2u, 3u, 128u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLightShadowed1.ppo", "Filter_GXLightAcc_DSpotLightShadowed1.ppo", "Pixel(PS)", "None", 29u, 2u, 0x0000000082951fa5ull, 0x18bac706u, 3396u, 1840u, 391u, 35820u, 0x000c01c9u, 0x0000001cu, 40u, 64u, 14u, 6u, 3u, 4u, 33u, 272u, "user_sgpr[2]->load(+32)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLightShadowed2.ppo", "Filter_GXLightAcc_DSpotLightShadowed2.ppo", "Pixel(PS)", "None", 30u, 2u, 0x00000000294503baull, 0x184728cbu, 4684u, 3080u, 661u, 54051u, 0x000c01cau, 0x0000001cu, 44u, 64u, 14u, 7u, 3u, 4u, 69u, 272u, "user_sgpr[2]->load(+32)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLightShadowed3.ppo", "Filter_GXLightAcc_DSpotLightShadowed3.ppo", "Pixel(PS)", "None", 31u, 2u, 0x00000000761d2013ull, 0x7bc5ee93u, 5932u, 4280u, 921u, 72271u, 0x000c01cbu, 0x0000001cu, 48u, 64u, 14u, 8u, 3u, 4u, 105u, 272u, "user_sgpr[2]->load(+32)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_DSpotLightShadowed4.ppo", "Filter_GXLightAcc_DSpotLightShadowed4.ppo", "Pixel(PS)", "None", 32u, 2u, 0x00000000376e0a37ull, 0x1823b943u, 7188u, 5488u, 1183u, 90484u, 0x000c01cbu, 0x0000001cu, 48u, 64u, 14u, 9u, 3u, 4u, 141u, 272u, "user_sgpr[2]->load(+32)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight1.ppo", "Filter_GXLightAcc_LegacyDPointLight1.ppo", "Pixel(PS)", "None", 33u, 2u, 0x000000007cc2c044ull, 0x6fd05d69u, 2320u, 1000u, 212u, 20869u, 0x000c0106u, 0x00000020u, 28u, 40u, 16u, 5u, 2u, 3u, 28u, 44u, "user_sgpr[2]->load(+144)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight10.ppo", "Filter_GXLightAcc_LegacyDPointLight10.ppo", "Pixel(PS)", "None", 34u, 2u, 0x00000000505ebdbeull, 0x7510de51u, 5820u, 4496u, 959u, 68530u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 158u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight11.ppo", "Filter_GXLightAcc_LegacyDPointLight11.ppo", "Pixel(PS)", "None", 35u, 2u, 0x0000000022b357f3ull, 0xe53ff132u, 6204u, 4880u, 1041u, 73805u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 174u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight12.ppo", "Filter_GXLightAcc_LegacyDPointLight12.ppo", "Pixel(PS)", "None", 36u, 2u, 0x000000009f9ef267ull, 0x027dce5fu, 6588u, 5264u, 1123u, 79080u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 190u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight2.ppo", "Filter_GXLightAcc_LegacyDPointLight2.ppo", "Pixel(PS)", "None", 37u, 2u, 0x00000000186d69d5ull, 0x0b10b32cu, 2712u, 1388u, 294u, 26126u, 0x000c0107u, 0x00000020u, 32u, 40u, 16u, 5u, 2u, 3u, 30u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight3.ppo", "Filter_GXLightAcc_LegacyDPointLight3.ppo", "Pixel(PS)", "None", 38u, 2u, 0x00000000d22cbfd1ull, 0xc382aa46u, 3128u, 1808u, 385u, 31605u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 46u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight4.ppo", "Filter_GXLightAcc_LegacyDPointLight4.ppo", "Pixel(PS)", "None", 39u, 2u, 0x000000003105494cull, 0x68ca979cu, 3512u, 2192u, 467u, 36880u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 62u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight5.ppo", "Filter_GXLightAcc_LegacyDPointLight5.ppo", "Pixel(PS)", "None", 40u, 2u, 0x00000000d75fe2ffull, 0x95d12282u, 3896u, 2576u, 549u, 42155u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 78u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight6.ppo", "Filter_GXLightAcc_LegacyDPointLight6.ppo", "Pixel(PS)", "None", 41u, 2u, 0x0000000007d07cf3ull, 0x7c2b87beu, 4280u, 2960u, 631u, 47430u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 94u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight7.ppo", "Filter_GXLightAcc_LegacyDPointLight7.ppo", "Pixel(PS)", "None", 42u, 2u, 0x00000000f2531727ull, 0x0ba89955u, 4664u, 3344u, 713u, 52705u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 110u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight8.ppo", "Filter_GXLightAcc_LegacyDPointLight8.ppo", "Pixel(PS)", "None", 43u, 2u, 0x00000000c6edb19aull, 0x05f0c21fu, 5048u, 3728u, 795u, 57980u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 126u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLight9.ppo", "Filter_GXLightAcc_LegacyDPointLight9.ppo", "Pixel(PS)", "None", 44u, 2u, 0x00000000b59b4c4eull, 0xe51e5882u, 5432u, 4112u, 877u, 63255u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 5u, 2u, 3u, 142u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLightShadowed1.ppo", "Filter_GXLightAcc_LegacyDPointLightShadowed1.ppo", "Pixel(PS)", "None", 45u, 2u, 0x00000000e96447c3ull, 0x18a0e906u, 2716u, 1252u, 251u, 26075u, 0x000c0247u, 0x0000001cu, 32u, 80u, 14u, 6u, 2u, 4u, 28u, 272u, "user_sgpr[2]->load(+0)", "cbLightAccParam", "g_LightPrepassAttrTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLightShadowed2.ppo", "Filter_GXLightAcc_LegacyDPointLightShadowed2.ppo", "Pixel(PS)", "None", 46u, 2u, 0x00000000a22c7f71ull, 0x11f1971cu, 3452u, 1940u, 387u, 35725u, 0x000c0248u, 0x0000001cu, 36u, 80u, 14u, 7u, 2u, 4u, 48u, 272u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_LightPrepassAttrTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLightShadowed3.ppo", "Filter_GXLightAcc_LegacyDPointLightShadowed3.ppo", "Pixel(PS)", "None", 47u, 2u, 0x000000006b42fc89ull, 0x6830f2e5u, 4140u, 2580u, 511u, 45103u, 0x000c024au, 0x0000001cu, 44u, 80u, 14u, 8u, 2u, 4u, 72u, 272u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_LightPrepassAttrTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyDPointLightShadowed4.ppo", "Filter_GXLightAcc_LegacyDPointLightShadowed4.ppo", "Pixel(PS)", "None", 48u, 2u, 0x000000000ef50047ull, 0xff7bd68cu, 4804u, 3196u, 629u, 54295u, 0x000c024bu, 0x0000001cu, 48u, 80u, 14u, 9u, 2u, 4u, 96u, 272u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_LightPrepassAttrTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight1.ppo", "Filter_GXLightAcc_LegacyPointLight1.ppo", "Pixel(PS)", "None", 49u, 2u, 0x000000006ab6cecbull, 0x86bd2a91u, 2072u, 872u, 185u, 17541u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 28u, 24u, "user_sgpr[2]->load(+64)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight10.ppo", "Filter_GXLightAcc_LegacyPointLight10.ppo", "Pixel(PS)", "None", 50u, 2u, 0x00000000428a80bfull, 0xf65dd8afu, 5564u, 4364u, 931u, 65078u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 158u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight11.ppo", "Filter_GXLightAcc_LegacyPointLight11.ppo", "Pixel(PS)", "None", 51u, 2u, 0x00000000d3a7b5a0ull, 0xebb2a080u, 5956u, 4752u, 1014u, 70353u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 174u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight12.ppo", "Filter_GXLightAcc_LegacyPointLight12.ppo", "Pixel(PS)", "None", 52u, 2u, 0x00000000e4daeaaeull, 0x169ccd8fu, 6340u, 5140u, 1097u, 75628u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 190u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight2.ppo", "Filter_GXLightAcc_LegacyPointLight2.ppo", "Pixel(PS)", "None", 53u, 2u, 0x00000000ecda90f6ull, 0xff66ff95u, 2488u, 1292u, 275u, 22974u, 0x000c0106u, 0x00000020u, 28u, 40u, 16u, 3u, 1u, 3u, 30u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight3.ppo", "Filter_GXLightAcc_LegacyPointLight3.ppo", "Pixel(PS)", "None", 54u, 2u, 0x00000000d776fb27ull, 0x05f7b282u, 2880u, 1680u, 358u, 28297u, 0x000c0108u, 0x00000020u, 36u, 40u, 16u, 3u, 1u, 3u, 46u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight4.ppo", "Filter_GXLightAcc_LegacyPointLight4.ppo", "Pixel(PS)", "None", 55u, 2u, 0x00000000c023d70full, 0x81b3e148u, 3232u, 2036u, 433u, 33428u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 62u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight5.ppo", "Filter_GXLightAcc_LegacyPointLight5.ppo", "Pixel(PS)", "None", 56u, 2u, 0x00000000b47e0972ull, 0x0b3826c5u, 3624u, 2424u, 516u, 38703u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 78u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight6.ppo", "Filter_GXLightAcc_LegacyPointLight6.ppo", "Pixel(PS)", "None", 57u, 2u, 0x00000000b3c63c01ull, 0x9ce3bac8u, 4008u, 2812u, 599u, 43978u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 94u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight7.ppo", "Filter_GXLightAcc_LegacyPointLight7.ppo", "Pixel(PS)", "None", 58u, 2u, 0x0000000043d26ee3ull, 0x1847dc49u, 4400u, 3200u, 682u, 49253u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 110u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight8.ppo", "Filter_GXLightAcc_LegacyPointLight8.ppo", "Pixel(PS)", "None", 59u, 2u, 0x00000000b464a1f2ull, 0x9248ab0fu, 4784u, 3588u, 765u, 54528u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 126u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLight9.ppo", "Filter_GXLightAcc_LegacyPointLight9.ppo", "Pixel(PS)", "None", 60u, 2u, 0x00000000a422d554ull, 0x185a6515u, 5176u, 3976u, 848u, 59803u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 142u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLightShadowed1.ppo", "Filter_GXLightAcc_LegacyPointLightShadowed1.ppo", "Pixel(PS)", "None", 61u, 2u, 0x000000008aa6908eull, 0x68b321e4u, 2528u, 1140u, 230u, 22625u, 0x000c0186u, 0x00000020u, 28u, 56u, 16u, 4u, 2u, 3u, 28u, 36u, "user_sgpr[2]->load(+112)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLightShadowed2.ppo", "Filter_GXLightAcc_LegacyPointLightShadowed2.ppo", "Pixel(PS)", "None", 62u, 2u, 0x000000000a8a518bull, 0x6fa03704u, 3244u, 1800u, 358u, 31969u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 5u, 2u, 3u, 48u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLightShadowed3.ppo", "Filter_GXLightAcc_LegacyPointLightShadowed3.ppo", "Pixel(PS)", "None", 63u, 2u, 0x000000005e210809ull, 0x6f49d0e5u, 3956u, 2472u, 490u, 41947u, 0x000c0189u, 0x0000001cu, 40u, 56u, 14u, 6u, 2u, 4u, 72u, 80u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_LegacyPointLightShadowed4.ppo", "Filter_GXLightAcc_LegacyPointLightShadowed4.ppo", "Pixel(PS)", "None", 64u, 2u, 0x000000002994a496ull, 0x7ccb5c98u, 4620u, 3088u, 608u, 51139u, 0x000c018au, 0x0000001cu, 44u, 56u, 14u, 7u, 2u, 4u, 96u, 88u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight1.ppo", "Filter_GXLightAcc_PointLight1.ppo", "Pixel(PS)", "None", 65u, 2u, 0x0000000057a66dedull, 0x924f4537u, 2136u, 936u, 199u, 18381u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 28u, 24u, "user_sgpr[2]->load(+64)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight10.ppo", "Filter_GXLightAcc_PointLight10.ppo", "Pixel(PS)", "None", 66u, 2u, 0x00000000db92e75cull, 0x5df22463u, 6436u, 5236u, 1121u, 74984u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 158u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight11.ppo", "Filter_GXLightAcc_PointLight11.ppo", "Pixel(PS)", "None", 67u, 2u, 0x00000000093a99d8ull, 0x619b6a5cu, 6916u, 5712u, 1223u, 81255u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 174u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight12.ppo", "Filter_GXLightAcc_PointLight12.ppo", "Pixel(PS)", "None", 68u, 2u, 0x0000000065ff4c81ull, 0x7c2c87a7u, 7388u, 6188u, 1325u, 87526u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 190u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight2.ppo", "Filter_GXLightAcc_PointLight2.ppo", "Pixel(PS)", "None", 69u, 2u, 0x00000000dea6db05ull, 0x727d9730u, 2624u, 1424u, 304u, 24744u, 0x000c0107u, 0x00000020u, 32u, 40u, 16u, 3u, 1u, 3u, 30u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight3.ppo", "Filter_GXLightAcc_PointLight3.ppo", "Pixel(PS)", "None", 70u, 2u, 0x00000000c3eb1d3aull, 0x66f875a9u, 3104u, 1904u, 407u, 31087u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 46u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight4.ppo", "Filter_GXLightAcc_PointLight4.ppo", "Pixel(PS)", "None", 71u, 2u, 0x0000000042bd4804ull, 0x923fc875u, 3576u, 2380u, 509u, 37358u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 62u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight5.ppo", "Filter_GXLightAcc_PointLight5.ppo", "Pixel(PS)", "None", 72u, 2u, 0x00000000ba81fc0aull, 0xc41ca72eu, 4056u, 2856u, 611u, 43629u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 78u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight6.ppo", "Filter_GXLightAcc_PointLight6.ppo", "Pixel(PS)", "None", 73u, 2u, 0x000000003cd2ac34ull, 0x86bd4900u, 4528u, 3332u, 713u, 49900u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 94u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight7.ppo", "Filter_GXLightAcc_PointLight7.ppo", "Pixel(PS)", "None", 74u, 2u, 0x000000002aaa5cb1ull, 0x16612200u, 5008u, 3808u, 815u, 56171u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 110u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight8.ppo", "Filter_GXLightAcc_PointLight8.ppo", "Pixel(PS)", "None", 75u, 2u, 0x00000000e1610d5bull, 0xfffff124u, 5480u, 4284u, 917u, 62442u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 126u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLight9.ppo", "Filter_GXLightAcc_PointLight9.ppo", "Pixel(PS)", "None", 76u, 2u, 0x000000003aafbe57ull, 0x7293a2c2u, 5960u, 4760u, 1019u, 68713u, 0x000c0109u, 0x00000020u, 40u, 40u, 16u, 3u, 1u, 3u, 142u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLightShadowed1.ppo", "Filter_GXLightAcc_PointLightShadowed1.ppo", "Pixel(PS)", "None", 77u, 2u, 0x0000000098506debull, 0xff1f23cau, 2600u, 1208u, 245u, 23329u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 4u, 2u, 3u, 28u, 36u, "user_sgpr[2]->load(+112)", "cbLightAccParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLightShadowed2.ppo", "Filter_GXLightAcc_PointLightShadowed2.ppo", "Pixel(PS)", "None", 78u, 2u, 0x0000000007aa7c3eull, 0x9bf9992eu, 3420u, 1976u, 398u, 33834u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 5u, 2u, 3u, 48u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLightShadowed3.ppo", "Filter_GXLightAcc_PointLightShadowed3.ppo", "Pixel(PS)", "None", 79u, 2u, 0x00000000ea36a1b0ull, 0x6f40927au, 4196u, 2712u, 543u, 44791u, 0x000c018au, 0x0000001cu, 44u, 56u, 14u, 6u, 2u, 4u, 72u, 80u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_PointLightShadowed4.ppo", "Filter_GXLightAcc_PointLightShadowed4.ppo", "Pixel(PS)", "None", 80u, 2u, 0x0000000040727870ull, 0x7c2b1012u, 4948u, 3420u, 681u, 55001u, 0x000c018bu, 0x0000001cu, 48u, 56u, 14u, 7u, 2u, 4u, 96u, 88u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLight1.ppo", "Filter_GXLightAcc_SpotLight1.ppo", "Pixel(PS)", "None", 81u, 2u, 0x00000000bcf67c52ull, 0xeb6e98fdu, 2840u, 1536u, 331u, 28732u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 3u, 32u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLight2.ppo", "Filter_GXLightAcc_SpotLight2.ppo", "Pixel(PS)", "None", 82u, 2u, 0x00000000c12ac9e5ull, 0x8f6f7457u, 3904u, 2604u, 568u, 44403u, 0x000c0189u, 0x00000020u, 40u, 56u, 16u, 3u, 1u, 3u, 64u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLight3.ppo", "Filter_GXLightAcc_SpotLight3.ppo", "Pixel(PS)", "None", 83u, 2u, 0x0000000046323550ull, 0xff668647u, 4936u, 3632u, 795u, 60094u, 0x000c018au, 0x00000020u, 44u, 56u, 16u, 3u, 1u, 3u, 96u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLight4.ppo", "Filter_GXLightAcc_SpotLight4.ppo", "Pixel(PS)", "None", 84u, 2u, 0x000000001ac8be70ull, 0xff7c2c9au, 5968u, 4664u, 1023u, 75770u, 0x000c018au, 0x00000020u, 44u, 56u, 16u, 3u, 1u, 3u, 128u, 24u, "user_sgpr[2]->load(+80)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLightShadowed1.ppo", "Filter_GXLightAcc_SpotLightShadowed1.ppo", "Pixel(PS)", "None", 85u, 2u, 0x000000008e4d8f54ull, 0xffb46c10u, 3156u, 1716u, 366u, 31967u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 4u, 2u, 3u, 33u, 36u, "user_sgpr[2]->load(+128)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLightShadowed2.ppo", "Filter_GXLightAcc_SpotLightShadowed2.ppo", "Pixel(PS)", "None", 86u, 2u, 0x00000000e72d12aeull, 0xff758e37u, 4456u, 2960u, 637u, 50131u, 0x000c0189u, 0x00000020u, 40u, 56u, 16u, 5u, 2u, 3u, 69u, 44u, "user_sgpr[2]->load(+160)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLightShadowed3.ppo", "Filter_GXLightAcc_SpotLightShadowed3.ppo", "Pixel(PS)", "None", 87u, 2u, 0x00000000c44db5d0ull, 0x92d8282au, 5696u, 4160u, 897u, 68869u, 0x000c018au, 0x0000001cu, 44u, 56u, 14u, 6u, 2u, 4u, 105u, 80u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightAcc_SpotLightShadowed4.ppo", "Filter_GXLightAcc_SpotLightShadowed4.ppo", "Pixel(PS)", "None", 88u, 2u, 0x000000007b33e949ull, 0xff9238b8u, 6952u, 5368u, 1159u, 87082u, 0x000c018au, 0x0000001cu, 44u, 56u, 14u, 7u, 2u, 4u, 141u, 88u, "user_sgpr[2]->load(+16)", "cbLightAccLightParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXLightCmbAlbedo.ppo", "Filter_GXLightCmbAlbedo.ppo", "Pixel(PS)", "None", 89u, 2u, 0x000000008650418full, 0x05ed7585u, 524u, 100u, 21u, 3294u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 2u, 1u, 1u, 0u, 8u, "", "", "g_IrradianceTexture"},
    {"gxrenderershader", "Filter\\GXLightComposite.ppo", "Filter_GXLightComposite.ppo", "Pixel(PS)", "None", 90u, 2u, 0x0000000032e47788ull, 0xff7c36aeu, 1936u, 604u, 121u, 17512u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 6u, 1u, 3u, 44u, 48u, "user_sgpr[2]->load(+160)", "cbLightAccParam", "g_GBuffer3Texture"},
    {"gxrenderershader", "Filter\\GXLightDeferredAO.ppo", "Filter_GXLightDeferredAO.ppo", "Pixel(PS)", "None", 91u, 2u, 0x000000003fe9e964ull, 0x95385bfdu, 3384u, 820u, 162u, 21866u, 0x000c0206u, 0x0000001cu, 28u, 72u, 14u, 8u, 4u, 4u, 80u, 360u, "user_sgpr[2]->load(+48)", "cbSceneParam", "g_GBuffer1Texture"},
    {"gxrenderershader", "Filter\\GXRend_CompositeShadowMask.ppo", "Filter_GXRend_CompositeShadowMask.ppo", "Pixel(PS)", "None", 92u, 2u, 0x00000000a291c498ull, 0x81ba57e4u, 732u, 132u, 29u, 4929u, 0x000c0080u, 0x00000020u, 4u, 24u, 16u, 2u, 2u, 2u, 7u, 16u, "user_sgpr[2]->load(+48)", "cbCompositeShadowMask", "g_GBuffer1Texture"},
    {"gxrenderershader", "Filter\\GXScreenSpaceCascadedShadow_PCF4.ppo", "Filter_GXScreenSpaceCascadedShadow_PCF4.ppo", "Pixel(PS)", "None", 93u, 2u, 0x00000000d0cf07dfull, 0x6f4011bau, 2576u, 492u, 103u, 13170u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 2u, 2u, 3u, 84u, 20u, "user_sgpr[2]->load(+48)", "cbPerScene", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXScreenSpaceCascadedShadow_PCF9.ppo", "Filter_GXScreenSpaceCascadedShadow_PCF9.ppo", "Pixel(PS)", "None", 94u, 2u, 0x00000000f55643deull, 0x9ce44e1fu, 2752u, 664u, 132u, 14308u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 2u, 2u, 3u, 84u, 20u, "user_sgpr[2]->load(+48)", "cbPerScene", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXScreenSpaceCascadedShadow_Soft.ppo", "Filter_GXScreenSpaceCascadedShadow_Soft.ppo", "Pixel(PS)", "None", 95u, 2u, 0x00000000b89e316bull, 0xf888913bu, 4236u, 2064u, 337u, 23365u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 4u, 3u, 3u, 112u, 32u, "user_sgpr[2]->load(+112)", "cbSceneParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXScreenSpaceShadow_Blur.ppo", "Filter_GXScreenSpaceShadow_Blur.ppo", "Pixel(PS)", "None", 96u, 2u, 0x0000000011ffdfb5ull, 0xebeac841u, 1992u, 1460u, 260u, 18602u, 0x000c020du, 0x00000020u, 56u, 72u, 16u, 1u, 1u, 2u, 52u, 4u, "user_sgpr[2]->load(+0)", "cbBlurParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXScreenSpaceShadow_PCF4.ppo", "Filter_GXScreenSpaceShadow_PCF4.ppo", "Pixel(PS)", "None", 97u, 2u, 0x00000000fea7f9d6ull, 0x9b036b62u, 2472u, 396u, 88u, 11097u, 0x000c0101u, 0x00000020u, 8u, 40u, 16u, 2u, 2u, 3u, 74u, 20u, "user_sgpr[2]->load(+64)", "cbSceneParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXScreenSpaceShadow_PCF9.ppo", "Filter_GXScreenSpaceShadow_PCF9.ppo", "Pixel(PS)", "None", 98u, 2u, 0x00000000ed6e3d3cull, 0x05fe6028u, 2672u, 596u, 124u, 12338u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 2u, 3u, 76u, 20u, "user_sgpr[2]->load(+64)", "cbSceneParam", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXSsr.ppo", "Filter_GXSsr.ppo", "Pixel(PS)", "None", 99u, 2u, 0x000000006c2323c2ull, 0x75e3eaabu, 2660u, 1612u, 354u, 31861u, 0x000c0206u, 0x00000020u, 28u, 72u, 16u, 3u, 3u, 2u, 80u, 28u, "user_sgpr[2]->load(+96)", "cbPerScene", "g_DepthTexture"},
    {"gxrenderershader", "Filter\\GXSssBlurX.ppo", "Filter_GXSssBlurX.ppo", "Pixel(PS)", "None", 100u, 2u, 0x000000002eacb983ull, 0x021e26b9u, 4248u, 2168u, 402u, 30813u, 0x000c014cu, 0x00000020u, 52u, 48u, 16u, 3u, 2u, 3u, 72u, 28u, "user_sgpr[2]->load(+96)", "cbSceneParam", "g_SSSStrengthTexture"},
    {"gxrenderershader", "Filter\\GXSssBlurY.ppo", "Filter_GXSssBlurY.ppo", "Pixel(PS)", "None", 101u, 2u, 0x000000008c13b984ull, 0x7cdfd02bu, 4248u, 2168u, 402u, 30813u, 0x000c014cu, 0x00000020u, 52u, 48u, 16u, 3u, 2u, 3u, 72u, 28u, "user_sgpr[2]->load(+96)", "cbSceneParam", "g_SSSStrengthTexture"},
    {"gxrenderershader", "Filter\\GXEnvDecal.vpo", "Filter_GXEnvDecal.vpo", "Vertex(VS)", "None", 102u, 1u, 0x000000008ee3fd59ull, 0x1fff1bedu, 448u, 48u, 8u, 1270u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxrenderershader", "Filter\\GXLightAcc.vpo", "Filter_GXLightAcc.vpo", "Vertex(VS)", "None", 103u, 1u, 0x000000008747a367ull, 0xf12483f0u, 568u, 60u, 11u, 1639u, 0x000c0042u, 0x00000018u, 12u, 16u, 12u, 0u, 0u, 1u, 1u, 0u, "user_sgpr[8]", "cbLightAccVtxParam", ""},
    {"gxrenderershader", "Filter\\GXScreenSpaceShadow.vpo", "Filter_GXScreenSpaceShadow.vpo", "Vertex(VS)", "None", 104u, 1u, 0x00000000061fbbf6ull, 0xc48b1849u, 476u, 48u, 8u, 1270u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxrenderershader", "Filter\\GXSsr.vpo", "Filter_GXSsr.vpo", "Vertex(VS)", "None", 105u, 1u, 0x000000005a619b89ull, 0x618f1b84u, 492u, 48u, 8u, 1270u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxrenderershader", "Filter\\GXSss.vpo", "Filter_GXSss.vpo", "Vertex(VS)", "None", 106u, 1u, 0x000000008cbc58faull, 0xfff64947u, 468u, 48u, 8u, 1270u, 0x000c0042u, 0x0000000cu, 12u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxrenderershader", "Sea\\GXOceanWave.ppo", "Sea_GXOceanWave.ppo", "Pixel(PS)", "None", 107u, 2u, 0x00000000660f9ca7ull, 0x1f2a86ddu, 6668u, 2624u, 536u, 39408u, 0x000c020bu, 0x00000020u, 48u, 72u, 16u, 5u, 2u, 4u, 158u, 48u, "user_sgpr[2]->load(+144)", "cbWave", "g_BumpmapTexture2"},
    {"gxrenderershader", "Sea\\GXOceanWave.vpo", "Sea_GXOceanWave.vpo", "Vertex(VS)", "None", 108u, 1u, 0x000000008cb07506ull, 0xf1c43cb4u, 6820u, 2672u, 571u, 49214u, 0x000c024du, 0x0000001cu, 56u, 80u, 14u, 4u, 4u, 1u, 161u, 0u, "user_sgpr[6]->load(+48)", "cbWave", "g_DepthTexture"},
    {"gxrenderershader", "WaterInteraction\\GXWaterInteractionHeightMap.ppo", "WaterInteraction_GXWaterInteractionHeightMap.ppo", "Pixel(PS)", "None", 109u, 2u, 0x00000000d07810e4ull, 0xff5d7fa6u, 2332u, 1008u, 197u, 14651u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 2u, 1u, 2u, 24u, 12u, "user_sgpr[2]->load(+32)", "cbWaterHeightMap", "g_DiffuseTexture"},
    {"gxrenderershader", "WaterInteraction\\GXWaterInteractionNormalMap.ppo", "WaterInteraction_GXWaterInteractionNormalMap.ppo", "Pixel(PS)", "None", 110u, 2u, 0x000000001574e65dull, 0xf8617120u, 1016u, 360u, 74u, 5369u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 4u, 4u, "user_sgpr[2]->load(+0)", "cbWaterNormalMap", "g_BumpmapTexture"},
    // --- Bundle: gxdecal (Tier 8/10, 193 shaders) ---
    {"gxdecal", "Lightprepass0\\Decal_BlendWeight.ppo", "Lightprepass0_Decal_BlendWeight.ppo", "Pixel(PS)", "None", 0u, 2u, 0x00000000533f1f4dull, 0x184ef368u, 2036u, 376u, 75u, 10055u, 0x000c0102u, 0x00000020u, 12u, 40u, 16u, 2u, 2u, 2u, 16u, 16u, "user_sgpr[2]->load(+48)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Blood_DifSpcBump.ppo", "Lightprepass0_Decal_Blood_DifSpcBump.ppo", "Pixel(PS)", "None", 1u, 2u, 0x0000000018e91730ull, 0x7c41a756u, 5580u, 936u, 185u, 27807u, 0x000c0184u, 0x0000001cu, 20u, 56u, 14u, 10u, 5u, 4u, 100u, 120u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_Dif.ppo", "Lightprepass0_Decal_Cubic_Dif.ppo", "Pixel(PS)", "None", 2u, 2u, 0x000000008baf4fbeull, 0xe2bb3b47u, 1984u, 412u, 83u, 9591u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 60u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifBump.ppo", "Lightprepass0_Decal_Cubic_DifBump.ppo", "Pixel(PS)", "None", 3u, 2u, 0x00000000f3546520ull, 0xec9d224cu, 4744u, 696u, 135u, 18329u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifBumpE.ppo", "Lightprepass0_Decal_Cubic_DifBumpE.ppo", "Pixel(PS)", "None", 4u, 2u, 0x00000000f3546520ull, 0xec9d224cu, 4744u, 696u, 135u, 18329u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifE.ppo", "Lightprepass0_Decal_Cubic_DifE.ppo", "Pixel(PS)", "None", 5u, 2u, 0x000000008baf4fbeull, 0xe2bb3b47u, 1984u, 412u, 83u, 9591u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 60u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpc.ppo", "Lightprepass0_Decal_Cubic_DifSpc.ppo", "Pixel(PS)", "None", 6u, 2u, 0x00000000bb816d4eull, 0x02f78da8u, 4808u, 632u, 122u, 16663u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpcBump.ppo", "Lightprepass0_Decal_Cubic_DifSpcBump.ppo", "Pixel(PS)", "None", 7u, 2u, 0x00000000394274f9ull, 0x18aeb1d0u, 5008u, 740u, 145u, 20553u, 0x000c0182u, 0x00000020u, 12u, 56u, 16u, 4u, 4u, 3u, 100u, 44u, "user_sgpr[2]->load(+160)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpcBumpE.ppo", "Lightprepass0_Decal_Cubic_DifSpcBumpE.ppo", "Pixel(PS)", "None", 8u, 2u, 0x00000000394274f9ull, 0x18aeb1d0u, 5008u, 740u, 145u, 20553u, 0x000c0182u, 0x00000020u, 12u, 56u, 16u, 4u, 4u, 3u, 100u, 44u, "user_sgpr[2]->load(+160)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpcE.ppo", "Lightprepass0_Decal_Cubic_DifSpcE.ppo", "Pixel(PS)", "None", 9u, 2u, 0x00000000bb816d4eull, 0x02f78da8u, 4808u, 632u, 122u, 16663u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_Dif.ppo", "Lightprepass0_Decal_Dif.ppo", "Pixel(PS)", "None", 10u, 2u, 0x00000000ec2145aaull, 0x2a9f506bu, 1944u, 376u, 74u, 9047u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 60u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifBump.ppo", "Lightprepass0_Decal_DifBump.ppo", "Pixel(PS)", "None", 11u, 2u, 0x0000000092dc7fabull, 0xff2a6251u, 4608u, 572u, 115u, 16293u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifBumpPrx.ppo", "Lightprepass0_Decal_DifBumpPrx.ppo", "Pixel(PS)", "None", 12u, 2u, 0x00000000308e33d3ull, 0x86b4053au, 4712u, 676u, 134u, 17990u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifBumpPrxE.ppo", "Lightprepass0_Decal_DifBumpPrxE.ppo", "Pixel(PS)", "None", 13u, 2u, 0x00000000e600a8b2ull, 0x18c4fc24u, 4756u, 676u, 134u, 17990u, 0x000c0142u, 0x00000020u, 12u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifE.ppo", "Lightprepass0_Decal_DifE.ppo", "Pixel(PS)", "None", 14u, 2u, 0x00000000ec2145aaull, 0x2a9f506bu, 1944u, 376u, 74u, 9047u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 60u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifPom.ppo", "Lightprepass0_Decal_DifPom.ppo", "Pixel(PS)", "None", 15u, 2u, 0x00000000c64f7603ull, 0xc415614du, 9060u, 1616u, 338u, 36622u, 0x000c01c8u, 0x0000001cu, 36u, 64u, 14u, 5u, 5u, 5u, 100u, 112u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifPomE.ppo", "Lightprepass0_Decal_DifPomE.ppo", "Pixel(PS)", "None", 16u, 2u, 0x000000006f9cfd89ull, 0xf8f1dd26u, 9104u, 1616u, 338u, 36622u, 0x000c01c8u, 0x0000001cu, 36u, 64u, 14u, 5u, 5u, 5u, 100u, 112u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifSpc.ppo", "Lightprepass0_Decal_DifSpc.ppo", "Pixel(PS)", "None", 17u, 2u, 0x000000004f32dfe7ull, 0xffe5a6c3u, 4668u, 504u, 101u, 14612u, 0x000c0141u, 0x00000020u, 8u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcBump.ppo", "Lightprepass0_Decal_DifSpcBump.ppo", "Pixel(PS)", "None", 18u, 2u, 0x000000002017fcb5ull, 0x7b415e79u, 4864u, 616u, 125u, 18508u, 0x000c0182u, 0x00000020u, 12u, 56u, 16u, 4u, 4u, 3u, 100u, 44u, "user_sgpr[2]->load(+160)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcBumpE.ppo", "Lightprepass0_Decal_DifSpcBumpE.ppo", "Pixel(PS)", "None", 19u, 2u, 0x00000000384d7253ull, 0x18cdb917u, 4904u, 616u, 125u, 18508u, 0x000c0182u, 0x00000020u, 12u, 56u, 16u, 4u, 4u, 3u, 100u, 44u, "user_sgpr[2]->load(+160)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcE.ppo", "Lightprepass0_Decal_DifSpcE.ppo", "Pixel(PS)", "None", 20u, 2u, 0x00000000d108a0eeull, 0xe52c8777u, 4708u, 504u, 101u, 14612u, 0x000c0141u, 0x00000020u, 8u, 48u, 16u, 3u, 3u, 3u, 100u, 32u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcPom.ppo", "Lightprepass0_Decal_DifSpcPom.ppo", "Pixel(PS)", "None", 21u, 2u, 0x00000000c64f7603ull, 0xc415614du, 9060u, 1616u, 338u, 36622u, 0x000c01c8u, 0x0000001cu, 36u, 64u, 14u, 5u, 5u, 5u, 100u, 112u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcPomE.ppo", "Lightprepass0_Decal_DifSpcPomE.ppo", "Pixel(PS)", "None", 22u, 2u, 0x000000006f9cfd89ull, 0xf8f1dd26u, 9104u, 1616u, 338u, 36622u, 0x000c01c8u, 0x0000001cu, 36u, 64u, 14u, 5u, 5u, 5u, 100u, 112u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass0\\Decal_PrePass.ppo", "Lightprepass0_Decal_PrePass.ppo", "Pixel(PS)", "None", 23u, 2u, 0x000000003d2a4cf7ull, 0x61f68c41u, 2056u, 424u, 86u, 10967u, 0x000c0181u, 0x00000020u, 8u, 56u, 16u, 2u, 2u, 2u, 100u, 16u, "user_sgpr[2]->load(+48)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_BlendWeight.ppo", "Lightprepass1_Decal_BlendWeight.ppo", "Pixel(PS)", "None", 24u, 2u, 0x00000000661ebf77ull, 0xff169c54u, 1848u, 312u, 62u, 7197u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Blood_DifSpcBump.ppo", "Lightprepass1_Decal_Blood_DifSpcBump.ppo", "Pixel(PS)", "None", 25u, 2u, 0x00000000b6bb3341ull, 0xff18de3cu, 8232u, 2812u, 540u, 65447u, 0x000c02cfu, 0x0000001cu, 64u, 96u, 14u, 17u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_Dif.ppo", "Lightprepass1_Decal_Cubic_Dif.ppo", "Pixel(PS)", "None", 26u, 2u, 0x000000006c776b60ull, 0x721e8cf1u, 6188u, 1316u, 265u, 37106u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifBump.ppo", "Lightprepass1_Decal_Cubic_DifBump.ppo", "Pixel(PS)", "None", 27u, 2u, 0x000000006c776b60ull, 0x721e8cf1u, 6188u, 1316u, 265u, 37106u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifBumpE.ppo", "Lightprepass1_Decal_Cubic_DifBumpE.ppo", "Pixel(PS)", "None", 28u, 2u, 0x000000006c776b60ull, 0x721e8cf1u, 6188u, 1316u, 265u, 37106u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifE.ppo", "Lightprepass1_Decal_Cubic_DifE.ppo", "Pixel(PS)", "None", 29u, 2u, 0x000000006c776b60ull, 0x721e8cf1u, 6188u, 1316u, 265u, 37106u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpc.ppo", "Lightprepass1_Decal_Cubic_DifSpc.ppo", "Pixel(PS)", "None", 30u, 2u, 0x000000009888ce82ull, 0xffc491c5u, 7792u, 2668u, 507u, 57521u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpcBump.ppo", "Lightprepass1_Decal_Cubic_DifSpcBump.ppo", "Pixel(PS)", "None", 31u, 2u, 0x000000009888ce82ull, 0xffc491c5u, 7792u, 2668u, 507u, 57521u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpcBumpE.ppo", "Lightprepass1_Decal_Cubic_DifSpcBumpE.ppo", "Pixel(PS)", "None", 32u, 2u, 0x000000009888ce82ull, 0xffc491c5u, 7792u, 2668u, 507u, 57521u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpcE.ppo", "Lightprepass1_Decal_Cubic_DifSpcE.ppo", "Pixel(PS)", "None", 33u, 2u, 0x000000009888ce82ull, 0xffc491c5u, 7792u, 2668u, 507u, 57521u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_Dif.ppo", "Lightprepass1_Decal_Dif.ppo", "Pixel(PS)", "None", 34u, 2u, 0x00000000a80ab1c5ull, 0x7593e420u, 6024u, 1156u, 240u, 34856u, 0x000c0245u, 0x0000001cu, 24u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifBump.ppo", "Lightprepass1_Decal_DifBump.ppo", "Pixel(PS)", "None", 35u, 2u, 0x00000000a80ab1c5ull, 0x7593e420u, 6024u, 1156u, 240u, 34856u, 0x000c0245u, 0x0000001cu, 24u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifBumpPrx.ppo", "Lightprepass1_Decal_DifBumpPrx.ppo", "Pixel(PS)", "None", 36u, 2u, 0x000000006bd94174ull, 0xe2eaec72u, 6208u, 1268u, 261u, 38117u, 0x000c0245u, 0x0000001cu, 24u, 80u, 14u, 6u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifBumpPrxE.ppo", "Lightprepass1_Decal_DifBumpPrxE.ppo", "Pixel(PS)", "None", 37u, 2u, 0x00000000b1e56025ull, 0x6f7bfc23u, 6288u, 1300u, 268u, 39594u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 7u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifE.ppo", "Lightprepass1_Decal_DifE.ppo", "Pixel(PS)", "None", 38u, 2u, 0x000000006ef4761eull, 0x9ce440d7u, 6096u, 1188u, 247u, 36333u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 6u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifPom.ppo", "Lightprepass1_Decal_DifPom.ppo", "Pixel(PS)", "None", 39u, 2u, 0x000000007ada8174ull, 0x02f907e4u, 11720u, 3412u, 666u, 68875u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 11u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifPomE.ppo", "Lightprepass1_Decal_DifPomE.ppo", "Pixel(PS)", "None", 40u, 2u, 0x00000000e1981a3cull, 0x18d7f4ffu, 11796u, 3436u, 671u, 70388u, 0x000c024eu, 0x0000001cu, 60u, 80u, 14u, 12u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifSpc.ppo", "Lightprepass1_Decal_DifSpc.ppo", "Pixel(PS)", "None", 41u, 2u, 0x000000002f7c9869ull, 0x8f189455u, 7628u, 2516u, 483u, 55290u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcBump.ppo", "Lightprepass1_Decal_DifSpcBump.ppo", "Pixel(PS)", "None", 42u, 2u, 0x000000002f7c9869ull, 0x8f189455u, 7628u, 2516u, 483u, 55290u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcBumpE.ppo", "Lightprepass1_Decal_DifSpcBumpE.ppo", "Pixel(PS)", "None", 43u, 2u, 0x000000008c8a8b53ull, 0xec19f89cu, 7696u, 2540u, 488u, 56939u, 0x000c028eu, 0x0000001cu, 60u, 88u, 14u, 11u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcE.ppo", "Lightprepass1_Decal_DifSpcE.ppo", "Pixel(PS)", "None", 44u, 2u, 0x000000008c8a8b53ull, 0xec19f89cu, 7696u, 2540u, 488u, 56939u, 0x000c028eu, 0x0000001cu, 60u, 88u, 14u, 11u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcPom.ppo", "Lightprepass1_Decal_DifSpcPom.ppo", "Pixel(PS)", "None", 45u, 2u, 0x000000007ada8174ull, 0x02f907e4u, 11720u, 3412u, 666u, 68875u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 11u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcPomE.ppo", "Lightprepass1_Decal_DifSpcPomE.ppo", "Pixel(PS)", "None", 46u, 2u, 0x00000000e1981a3cull, 0x18d7f4ffu, 11796u, 3436u, 671u, 70388u, 0x000c024eu, 0x0000001cu, 60u, 80u, 14u, 12u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Lightprepass1\\Decal_PrePass.ppo", "Lightprepass1_Decal_PrePass.ppo", "Pixel(PS)", "None", 47u, 2u, 0x000000003d2a4cf7ull, 0x61f68c41u, 2056u, 424u, 86u, 10967u, 0x000c0181u, 0x00000020u, 8u, 56u, 16u, 2u, 2u, 2u, 100u, 16u, "user_sgpr[2]->load(+48)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_BlendWeight.ppo", "Singlepass_Decal_BlendWeight.ppo", "Pixel(PS)", "None", 48u, 2u, 0x00000000661ebf77ull, 0xff169c54u, 1848u, 312u, 62u, 7197u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Blood_DifSpcBump.ppo", "Singlepass_Decal_Blood_DifSpcBump.ppo", "Pixel(PS)", "None", 49u, 2u, 0x00000000c2abea69ull, 0x955c4903u, 8392u, 2896u, 553u, 68277u, 0x000c02d0u, 0x0000001cu, 68u, 96u, 14u, 18u, 6u, 7u, 100u, 304u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_Dif.ppo", "Singlepass_Decal_Cubic_Dif.ppo", "Pixel(PS)", "None", 50u, 2u, 0x000000003ab3cbbeull, 0x669c1e65u, 6308u, 1440u, 291u, 40358u, 0x000c0287u, 0x0000001cu, 32u, 88u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifBump.ppo", "Singlepass_Decal_Cubic_DifBump.ppo", "Pixel(PS)", "None", 51u, 2u, 0x00000000da7bd564ull, 0x0cb53742u, 6480u, 1532u, 310u, 42778u, 0x000c0287u, 0x0000001cu, 32u, 88u, 14u, 6u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifBumpE.ppo", "Singlepass_Decal_Cubic_DifBumpE.ppo", "Pixel(PS)", "None", 52u, 2u, 0x00000000da7bd564ull, 0x0cb53742u, 6480u, 1532u, 310u, 42778u, 0x000c0287u, 0x0000001cu, 32u, 88u, 14u, 6u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifE.ppo", "Singlepass_Decal_Cubic_DifE.ppo", "Pixel(PS)", "None", 53u, 2u, 0x000000003ab3cbbeull, 0x669c1e65u, 6308u, 1440u, 291u, 40358u, 0x000c0287u, 0x0000001cu, 32u, 88u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpc.ppo", "Singlepass_Decal_Cubic_DifSpc.ppo", "Pixel(PS)", "None", 54u, 2u, 0x00000000bb6621a9ull, 0x75672a25u, 7912u, 2792u, 533u, 60569u, 0x000c0250u, 0x0000001cu, 68u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpcBump.ppo", "Singlepass_Decal_Cubic_DifSpcBump.ppo", "Pixel(PS)", "None", 55u, 2u, 0x0000000089d8423cull, 0x8623babau, 8084u, 2876u, 550u, 63194u, 0x000c02d1u, 0x0000001cu, 72u, 96u, 14u, 11u, 6u, 7u, 100u, 304u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpcBumpE.ppo", "Singlepass_Decal_Cubic_DifSpcBumpE.ppo", "Pixel(PS)", "None", 56u, 2u, 0x0000000089d8423cull, 0x8623babau, 8084u, 2876u, 550u, 63194u, 0x000c02d1u, 0x0000001cu, 72u, 96u, 14u, 11u, 6u, 7u, 100u, 304u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpcE.ppo", "Singlepass_Decal_Cubic_DifSpcE.ppo", "Pixel(PS)", "None", 57u, 2u, 0x00000000bb6621a9ull, 0x75672a25u, 7912u, 2792u, 533u, 60569u, 0x000c0250u, 0x0000001cu, 68u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_Dif.ppo", "Singlepass_Decal_Dif.ppo", "Pixel(PS)", "None", 58u, 2u, 0x000000001c79da07ull, 0x95d13f1du, 6064u, 1200u, 247u, 35743u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 5u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifBump.ppo", "Singlepass_Decal_DifBump.ppo", "Pixel(PS)", "None", 59u, 2u, 0x00000000b2ecf57dull, 0x66f88848u, 6240u, 1300u, 268u, 39427u, 0x000c0286u, 0x0000001cu, 28u, 88u, 14u, 6u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifBumpPrx.ppo", "Singlepass_Decal_DifBumpPrx.ppo", "Pixel(PS)", "None", 60u, 2u, 0x00000000b4adebaaull, 0xe2489647u, 6352u, 1408u, 288u, 41112u, 0x000c0286u, 0x0000001cu, 28u, 88u, 14u, 6u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifBumpPrxE.ppo", "Singlepass_Decal_DifBumpPrxE.ppo", "Pixel(PS)", "None", 61u, 2u, 0x00000000199e2948ull, 0xff05edf8u, 6424u, 1440u, 295u, 42589u, 0x000c0287u, 0x0000001cu, 32u, 88u, 14u, 7u, 5u, 7u, 100u, 296u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifE.ppo", "Singlepass_Decal_DifE.ppo", "Pixel(PS)", "None", 62u, 2u, 0x00000000159ccab7ull, 0x0b25914cu, 6136u, 1232u, 254u, 37220u, 0x000c0246u, 0x0000001cu, 28u, 80u, 14u, 6u, 4u, 7u, 100u, 296u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifPom.ppo", "Singlepass_Decal_DifPom.ppo", "Pixel(PS)", "None", 63u, 2u, 0x0000000018b535a2ull, 0xc498f9c4u, 11824u, 3512u, 683u, 71846u, 0x000c028fu, 0x0000001cu, 64u, 88u, 14u, 11u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifPomE.ppo", "Singlepass_Decal_DifPomE.ppo", "Pixel(PS)", "None", 64u, 2u, 0x00000000b5b258d6ull, 0xf639d92eu, 11900u, 3540u, 689u, 73427u, 0x000c02cfu, 0x0000001cu, 64u, 96u, 14u, 12u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifSpc.ppo", "Singlepass_Decal_DifSpc.ppo", "Pixel(PS)", "None", 65u, 2u, 0x0000000094f4c8e5ull, 0x887cfc59u, 7668u, 2560u, 490u, 56193u, 0x000c024fu, 0x0000001cu, 64u, 80u, 14u, 10u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifSpcBump.ppo", "Singlepass_Decal_DifSpcBump.ppo", "Pixel(PS)", "None", 66u, 2u, 0x00000000fb1e5fbeull, 0xf6c3afc1u, 7844u, 2652u, 509u, 59934u, 0x000c02cfu, 0x0000001cu, 64u, 96u, 14u, 11u, 6u, 7u, 100u, 304u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifSpcBumpE.ppo", "Singlepass_Decal_DifSpcBumpE.ppo", "Pixel(PS)", "None", 67u, 2u, 0x0000000052cdb02dull, 0x728e5760u, 7912u, 2680u, 515u, 61379u, 0x000c02cfu, 0x0000001cu, 64u, 96u, 14u, 12u, 6u, 7u, 100u, 304u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifSpcE.ppo", "Singlepass_Decal_DifSpcE.ppo", "Pixel(PS)", "None", 68u, 2u, 0x00000000b7f25b7full, 0xff18ae35u, 7744u, 2584u, 495u, 57842u, 0x000c028fu, 0x0000001cu, 64u, 88u, 14u, 11u, 5u, 7u, 100u, 304u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifSpcPom.ppo", "Singlepass_Decal_DifSpcPom.ppo", "Pixel(PS)", "None", 69u, 2u, 0x0000000018b535a2ull, 0xc498f9c4u, 11824u, 3512u, 683u, 71846u, 0x000c028fu, 0x0000001cu, 64u, 88u, 14u, 11u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_DifSpcPomE.ppo", "Singlepass_Decal_DifSpcPomE.ppo", "Pixel(PS)", "None", 70u, 2u, 0x00000000b5b258d6ull, 0xf639d92eu, 11900u, 3540u, 689u, 73427u, 0x000c02cfu, 0x0000001cu, 64u, 96u, 14u, 12u, 6u, 9u, 100u, 304u, "user_sgpr[2]->load(+144)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Singlepass\\Decal_PrePass.ppo", "Singlepass_Decal_PrePass.ppo", "Pixel(PS)", "None", 71u, 2u, 0x00000000205c9555ull, 0x16f1746du, 2280u, 556u, 111u, 14417u, 0x000c0182u, 0x00000020u, 12u, 56u, 16u, 3u, 3u, 2u, 100u, 28u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_BlendWeight.ppo", "Deferred_Decal_BlendWeight.ppo", "Pixel(PS)", "None", 72u, 2u, 0x0000000039170484ull, 0x7b4f1c8fu, 2056u, 364u, 71u, 8169u, 0x000c0083u, 0x00000020u, 16u, 24u, 16u, 1u, 1u, 2u, 16u, 4u, "user_sgpr[2]->load(+0)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Blood_DifSpcBump.ppo", "Deferred_Decal_Blood_DifSpcBump.ppo", "Pixel(PS)", "None", 73u, 2u, 0x000000007ab7bd15ull, 0xb3347252u, 5964u, 2028u, 409u, 55054u, 0x000c0248u, 0x0000001cu, 36u, 80u, 14u, 15u, 6u, 5u, 100u, 256u, "user_sgpr[2]->load(+80)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_Dif.ppo", "Deferred_Decal_Cubic_Dif.ppo", "Pixel(PS)", "None", 74u, 2u, 0x000000009c0f2affull, 0xeb10be47u, 4064u, 828u, 168u, 23249u, 0x000c01c4u, 0x00000020u, 20u, 64u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifBump.ppo", "Deferred_Decal_Cubic_DifBump.ppo", "Pixel(PS)", "None", 75u, 2u, 0x0000000098f6ee37ull, 0x18b31c77u, 4232u, 928u, 188u, 26697u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifBumpE.ppo", "Deferred_Decal_Cubic_DifBumpE.ppo", "Pixel(PS)", "None", 76u, 2u, 0x0000000098f6ee37ull, 0x18b31c77u, 4232u, 928u, 188u, 26697u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifE.ppo", "Deferred_Decal_Cubic_DifE.ppo", "Pixel(PS)", "None", 77u, 2u, 0x000000009c0f2affull, 0xeb10be47u, 4064u, 828u, 168u, 23249u, 0x000c01c4u, 0x00000020u, 20u, 64u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpc.ppo", "Deferred_Decal_Cubic_DifSpc.ppo", "Pixel(PS)", "None", 78u, 2u, 0x000000004ab18b34ull, 0x05179de0u, 5100u, 1612u, 322u, 40700u, 0x000c0207u, 0x0000001cu, 32u, 72u, 14u, 7u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpcBump.ppo", "Deferred_Decal_Cubic_DifSpcBump.ppo", "Pixel(PS)", "None", 79u, 2u, 0x00000000be303620ull, 0x61119618u, 5260u, 1708u, 341u, 44328u, 0x000c0208u, 0x0000001cu, 36u, 72u, 14u, 7u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpcBumpE.ppo", "Deferred_Decal_Cubic_DifSpcBumpE.ppo", "Pixel(PS)", "None", 80u, 2u, 0x00000000be303620ull, 0x61119618u, 5260u, 1708u, 341u, 44328u, 0x000c0208u, 0x0000001cu, 36u, 72u, 14u, 7u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpcE.ppo", "Deferred_Decal_Cubic_DifSpcE.ppo", "Pixel(PS)", "None", 81u, 2u, 0x000000004ab18b34ull, 0x05179de0u, 5100u, 1612u, 322u, 40700u, 0x000c0207u, 0x0000001cu, 32u, 72u, 14u, 7u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_Dif.ppo", "Deferred_Decal_Dif.ppo", "Pixel(PS)", "None", 82u, 2u, 0x00000000a1648616ull, 0xe2c5e7e8u, 3932u, 700u, 147u, 21205u, 0x000c01c4u, 0x00000020u, 20u, 64u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifBump.ppo", "Deferred_Decal_DifBump.ppo", "Pixel(PS)", "None", 83u, 2u, 0x0000000017cab08full, 0x1fba2d0eu, 4088u, 796u, 166u, 24661u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifBumpPrx.ppo", "Deferred_Decal_DifBumpPrx.ppo", "Pixel(PS)", "None", 84u, 2u, 0x000000007082a43full, 0x95bceb89u, 4192u, 896u, 184u, 26401u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 3u, 4u, 100u, 36u, "user_sgpr[2]->load(+112)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifBumpPrxE.ppo", "Deferred_Decal_DifBumpPrxE.ppo", "Pixel(PS)", "None", 85u, 2u, 0x00000000101a3e12ull, 0x81543a63u, 4320u, 936u, 193u, 28618u, 0x000c01c5u, 0x00000020u, 24u, 64u, 16u, 4u, 4u, 4u, 100u, 48u, "user_sgpr[2]->load(+160)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifE.ppo", "Deferred_Decal_DifE.ppo", "Pixel(PS)", "None", 86u, 2u, 0x00000000c1a82a46ull, 0xf14e2055u, 4120u, 760u, 160u, 24003u, 0x000c01c4u, 0x00000020u, 20u, 64u, 16u, 4u, 4u, 4u, 100u, 48u, "user_sgpr[2]->load(+160)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifPom.ppo", "Deferred_Decal_DifPom.ppo", "Pixel(PS)", "None", 87u, 2u, 0x00000000761dd83full, 0xff9cfefeu, 7768u, 2392u, 483u, 54450u, 0x000c01c9u, 0x0000001cu, 40u, 64u, 14u, 8u, 6u, 6u, 100u, 256u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifPomE.ppo", "Deferred_Decal_DifPomE.ppo", "Pixel(PS)", "None", 88u, 2u, 0x00000000673ab86dull, 0x7cf9fa58u, 7844u, 2424u, 490u, 55879u, 0x000c01c9u, 0x0000001cu, 40u, 64u, 14u, 9u, 6u, 6u, 100u, 256u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifSpc.ppo", "Deferred_Decal_DifSpc.ppo", "Pixel(PS)", "None", 89u, 2u, 0x00000000133eb139ull, 0xffff7cf9u, 4968u, 1492u, 302u, 38683u, 0x000c0207u, 0x0000001cu, 32u, 72u, 14u, 7u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifSpcBump.ppo", "Deferred_Decal_DifSpcBump.ppo", "Pixel(PS)", "None", 90u, 2u, 0x00000000bd2e05e7ull, 0x2a9fbe8bu, 5124u, 1588u, 321u, 42315u, 0x000c0208u, 0x0000001cu, 36u, 72u, 14u, 7u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifSpcBumpE.ppo", "Deferred_Decal_DifSpcBumpE.ppo", "Pixel(PS)", "None", 91u, 2u, 0x00000000c4aae5f8ull, 0x02749fc8u, 5200u, 1620u, 328u, 43768u, 0x000c0209u, 0x0000001cu, 40u, 72u, 14u, 8u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifSpcE.ppo", "Deferred_Decal_DifSpcE.ppo", "Pixel(PS)", "None", 92u, 2u, 0x00000000213ffbefull, 0x66f654b0u, 5092u, 1540u, 312u, 40901u, 0x000c0208u, 0x0000001cu, 36u, 72u, 14u, 8u, 5u, 5u, 100u, 256u, "user_sgpr[2]->load(+64)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifSpcPom.ppo", "Deferred_Decal_DifSpcPom.ppo", "Pixel(PS)", "None", 93u, 2u, 0x00000000761dd83full, 0xff9cfefeu, 7768u, 2392u, 483u, 54450u, 0x000c01c9u, 0x0000001cu, 40u, 64u, 14u, 8u, 6u, 6u, 100u, 256u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_DifSpcPomE.ppo", "Deferred_Decal_DifSpcPomE.ppo", "Pixel(PS)", "None", 94u, 2u, 0x00000000673ab86dull, 0x7cf9fa58u, 7844u, 2424u, 490u, 55879u, 0x000c01c9u, 0x0000001cu, 40u, 64u, 14u, 9u, 6u, 6u, 100u, 256u, "user_sgpr[2]->load(+96)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "Deferred\\Decal_PrePass.ppo", "Deferred_Decal_PrePass.ppo", "Pixel(PS)", "None", 95u, 2u, 0x000000003d2a4cf7ull, 0x61f68c41u, 2056u, 424u, 86u, 10967u, 0x000c0181u, 0x00000020u, 8u, 56u, 16u, 2u, 2u, 2u, 100u, 16u, "user_sgpr[2]->load(+48)", "cbDecalParam", "g_DepthTexture"},
    {"gxdecal", "PostProcess\\Decal_Sub.ppo", "PostProcess_Decal_Sub.ppo", "Pixel(PS)", "None", 96u, 2u, 0x000000008823c7d1ull, 0x2a8c4c6du, 584u, 96u, 20u, 2829u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 4u, 4u, "user_sgpr[2]->load(+0)", "cbParam", "g_SourceTexture"},
    {"gxdecal", "Lightprepass0\\Decal_BlendWeight.vpo", "Lightprepass0_Decal_BlendWeight.vpo", "Vertex(VS)", "None", 97u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Blood_DifSpcBump.vpo", "Lightprepass0_Decal_Blood_DifSpcBump.vpo", "Vertex(VS)", "None", 98u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_Dif.vpo", "Lightprepass0_Decal_Cubic_Dif.vpo", "Vertex(VS)", "None", 99u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifBump.vpo", "Lightprepass0_Decal_Cubic_DifBump.vpo", "Vertex(VS)", "None", 100u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifBumpE.vpo", "Lightprepass0_Decal_Cubic_DifBumpE.vpo", "Vertex(VS)", "None", 101u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifE.vpo", "Lightprepass0_Decal_Cubic_DifE.vpo", "Vertex(VS)", "None", 102u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpc.vpo", "Lightprepass0_Decal_Cubic_DifSpc.vpo", "Vertex(VS)", "None", 103u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpcBump.vpo", "Lightprepass0_Decal_Cubic_DifSpcBump.vpo", "Vertex(VS)", "None", 104u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpcBumpE.vpo", "Lightprepass0_Decal_Cubic_DifSpcBumpE.vpo", "Vertex(VS)", "None", 105u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Cubic_DifSpcE.vpo", "Lightprepass0_Decal_Cubic_DifSpcE.vpo", "Vertex(VS)", "None", 106u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_Dif.vpo", "Lightprepass0_Decal_Dif.vpo", "Vertex(VS)", "None", 107u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifBump.vpo", "Lightprepass0_Decal_DifBump.vpo", "Vertex(VS)", "None", 108u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifBumpPrx.vpo", "Lightprepass0_Decal_DifBumpPrx.vpo", "Vertex(VS)", "None", 109u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifBumpPrxE.vpo", "Lightprepass0_Decal_DifBumpPrxE.vpo", "Vertex(VS)", "None", 110u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifE.vpo", "Lightprepass0_Decal_DifE.vpo", "Vertex(VS)", "None", 111u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifPom.vpo", "Lightprepass0_Decal_DifPom.vpo", "Vertex(VS)", "None", 112u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifPomE.vpo", "Lightprepass0_Decal_DifPomE.vpo", "Vertex(VS)", "None", 113u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifSpc.vpo", "Lightprepass0_Decal_DifSpc.vpo", "Vertex(VS)", "None", 114u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcBump.vpo", "Lightprepass0_Decal_DifSpcBump.vpo", "Vertex(VS)", "None", 115u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcBumpE.vpo", "Lightprepass0_Decal_DifSpcBumpE.vpo", "Vertex(VS)", "None", 116u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcE.vpo", "Lightprepass0_Decal_DifSpcE.vpo", "Vertex(VS)", "None", 117u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcPom.vpo", "Lightprepass0_Decal_DifSpcPom.vpo", "Vertex(VS)", "None", 118u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_DifSpcPomE.vpo", "Lightprepass0_Decal_DifSpcPomE.vpo", "Vertex(VS)", "None", 119u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass0\\Decal_PrePass.vpo", "Lightprepass0_Decal_PrePass.vpo", "Vertex(VS)", "None", 120u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_BlendWeight.vpo", "Lightprepass1_Decal_BlendWeight.vpo", "Vertex(VS)", "None", 121u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Blood_DifSpcBump.vpo", "Lightprepass1_Decal_Blood_DifSpcBump.vpo", "Vertex(VS)", "None", 122u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_Dif.vpo", "Lightprepass1_Decal_Cubic_Dif.vpo", "Vertex(VS)", "None", 123u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifBump.vpo", "Lightprepass1_Decal_Cubic_DifBump.vpo", "Vertex(VS)", "None", 124u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifBumpE.vpo", "Lightprepass1_Decal_Cubic_DifBumpE.vpo", "Vertex(VS)", "None", 125u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifE.vpo", "Lightprepass1_Decal_Cubic_DifE.vpo", "Vertex(VS)", "None", 126u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpc.vpo", "Lightprepass1_Decal_Cubic_DifSpc.vpo", "Vertex(VS)", "None", 127u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpcBump.vpo", "Lightprepass1_Decal_Cubic_DifSpcBump.vpo", "Vertex(VS)", "None", 128u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpcBumpE.vpo", "Lightprepass1_Decal_Cubic_DifSpcBumpE.vpo", "Vertex(VS)", "None", 129u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Cubic_DifSpcE.vpo", "Lightprepass1_Decal_Cubic_DifSpcE.vpo", "Vertex(VS)", "None", 130u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_Dif.vpo", "Lightprepass1_Decal_Dif.vpo", "Vertex(VS)", "None", 131u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifBump.vpo", "Lightprepass1_Decal_DifBump.vpo", "Vertex(VS)", "None", 132u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifBumpPrx.vpo", "Lightprepass1_Decal_DifBumpPrx.vpo", "Vertex(VS)", "None", 133u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifBumpPrxE.vpo", "Lightprepass1_Decal_DifBumpPrxE.vpo", "Vertex(VS)", "None", 134u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifE.vpo", "Lightprepass1_Decal_DifE.vpo", "Vertex(VS)", "None", 135u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifPom.vpo", "Lightprepass1_Decal_DifPom.vpo", "Vertex(VS)", "None", 136u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifPomE.vpo", "Lightprepass1_Decal_DifPomE.vpo", "Vertex(VS)", "None", 137u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifSpc.vpo", "Lightprepass1_Decal_DifSpc.vpo", "Vertex(VS)", "None", 138u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcBump.vpo", "Lightprepass1_Decal_DifSpcBump.vpo", "Vertex(VS)", "None", 139u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcBumpE.vpo", "Lightprepass1_Decal_DifSpcBumpE.vpo", "Vertex(VS)", "None", 140u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcE.vpo", "Lightprepass1_Decal_DifSpcE.vpo", "Vertex(VS)", "None", 141u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcPom.vpo", "Lightprepass1_Decal_DifSpcPom.vpo", "Vertex(VS)", "None", 142u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_DifSpcPomE.vpo", "Lightprepass1_Decal_DifSpcPomE.vpo", "Vertex(VS)", "None", 143u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Lightprepass1\\Decal_PrePass.vpo", "Lightprepass1_Decal_PrePass.vpo", "Vertex(VS)", "None", 144u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_BlendWeight.vpo", "Singlepass_Decal_BlendWeight.vpo", "Vertex(VS)", "None", 145u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Blood_DifSpcBump.vpo", "Singlepass_Decal_Blood_DifSpcBump.vpo", "Vertex(VS)", "None", 146u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_Dif.vpo", "Singlepass_Decal_Cubic_Dif.vpo", "Vertex(VS)", "None", 147u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifBump.vpo", "Singlepass_Decal_Cubic_DifBump.vpo", "Vertex(VS)", "None", 148u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifBumpE.vpo", "Singlepass_Decal_Cubic_DifBumpE.vpo", "Vertex(VS)", "None", 149u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifE.vpo", "Singlepass_Decal_Cubic_DifE.vpo", "Vertex(VS)", "None", 150u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpc.vpo", "Singlepass_Decal_Cubic_DifSpc.vpo", "Vertex(VS)", "None", 151u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpcBump.vpo", "Singlepass_Decal_Cubic_DifSpcBump.vpo", "Vertex(VS)", "None", 152u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpcBumpE.vpo", "Singlepass_Decal_Cubic_DifSpcBumpE.vpo", "Vertex(VS)", "None", 153u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Cubic_DifSpcE.vpo", "Singlepass_Decal_Cubic_DifSpcE.vpo", "Vertex(VS)", "None", 154u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_Dif.vpo", "Singlepass_Decal_Dif.vpo", "Vertex(VS)", "None", 155u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifBump.vpo", "Singlepass_Decal_DifBump.vpo", "Vertex(VS)", "None", 156u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifBumpPrx.vpo", "Singlepass_Decal_DifBumpPrx.vpo", "Vertex(VS)", "None", 157u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifBumpPrxE.vpo", "Singlepass_Decal_DifBumpPrxE.vpo", "Vertex(VS)", "None", 158u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifE.vpo", "Singlepass_Decal_DifE.vpo", "Vertex(VS)", "None", 159u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifPom.vpo", "Singlepass_Decal_DifPom.vpo", "Vertex(VS)", "None", 160u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifPomE.vpo", "Singlepass_Decal_DifPomE.vpo", "Vertex(VS)", "None", 161u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifSpc.vpo", "Singlepass_Decal_DifSpc.vpo", "Vertex(VS)", "None", 162u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifSpcBump.vpo", "Singlepass_Decal_DifSpcBump.vpo", "Vertex(VS)", "None", 163u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifSpcBumpE.vpo", "Singlepass_Decal_DifSpcBumpE.vpo", "Vertex(VS)", "None", 164u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifSpcE.vpo", "Singlepass_Decal_DifSpcE.vpo", "Vertex(VS)", "None", 165u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifSpcPom.vpo", "Singlepass_Decal_DifSpcPom.vpo", "Vertex(VS)", "None", 166u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_DifSpcPomE.vpo", "Singlepass_Decal_DifSpcPomE.vpo", "Vertex(VS)", "None", 167u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Singlepass\\Decal_PrePass.vpo", "Singlepass_Decal_PrePass.vpo", "Vertex(VS)", "None", 168u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_BlendWeight.vpo", "Deferred_Decal_BlendWeight.vpo", "Vertex(VS)", "None", 169u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Blood_DifSpcBump.vpo", "Deferred_Decal_Blood_DifSpcBump.vpo", "Vertex(VS)", "None", 170u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_Dif.vpo", "Deferred_Decal_Cubic_Dif.vpo", "Vertex(VS)", "None", 171u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifBump.vpo", "Deferred_Decal_Cubic_DifBump.vpo", "Vertex(VS)", "None", 172u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifBumpE.vpo", "Deferred_Decal_Cubic_DifBumpE.vpo", "Vertex(VS)", "None", 173u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifE.vpo", "Deferred_Decal_Cubic_DifE.vpo", "Vertex(VS)", "None", 174u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpc.vpo", "Deferred_Decal_Cubic_DifSpc.vpo", "Vertex(VS)", "None", 175u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpcBump.vpo", "Deferred_Decal_Cubic_DifSpcBump.vpo", "Vertex(VS)", "None", 176u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpcBumpE.vpo", "Deferred_Decal_Cubic_DifSpcBumpE.vpo", "Vertex(VS)", "None", 177u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Cubic_DifSpcE.vpo", "Deferred_Decal_Cubic_DifSpcE.vpo", "Vertex(VS)", "None", 178u, 1u, 0x0000000005fb1649ull, 0x9c87d169u, 616u, 140u, 32u, 3918u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_Dif.vpo", "Deferred_Decal_Dif.vpo", "Vertex(VS)", "None", 179u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifBump.vpo", "Deferred_Decal_DifBump.vpo", "Vertex(VS)", "None", 180u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifBumpPrx.vpo", "Deferred_Decal_DifBumpPrx.vpo", "Vertex(VS)", "None", 181u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifBumpPrxE.vpo", "Deferred_Decal_DifBumpPrxE.vpo", "Vertex(VS)", "None", 182u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifE.vpo", "Deferred_Decal_DifE.vpo", "Vertex(VS)", "None", 183u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifPom.vpo", "Deferred_Decal_DifPom.vpo", "Vertex(VS)", "None", 184u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifPomE.vpo", "Deferred_Decal_DifPomE.vpo", "Vertex(VS)", "None", 185u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifSpc.vpo", "Deferred_Decal_DifSpc.vpo", "Vertex(VS)", "None", 186u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifSpcBump.vpo", "Deferred_Decal_DifSpcBump.vpo", "Vertex(VS)", "None", 187u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifSpcBumpE.vpo", "Deferred_Decal_DifSpcBumpE.vpo", "Vertex(VS)", "None", 188u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifSpcE.vpo", "Deferred_Decal_DifSpcE.vpo", "Vertex(VS)", "None", 189u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifSpcPom.vpo", "Deferred_Decal_DifSpcPom.vpo", "Vertex(VS)", "None", 190u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_DifSpcPomE.vpo", "Deferred_Decal_DifSpcPomE.vpo", "Vertex(VS)", "None", 191u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxdecal", "Deferred\\Decal_PrePass.vpo", "Deferred_Decal_PrePass.vpo", "Vertex(VS)", "None", 192u, 1u, 0x0000000063a3f522ull, 0x9b94da4eu, 664u, 192u, 40u, 4536u, 0x000c0082u, 0x00000018u, 12u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    // --- Bundle: gxffxshader (Tier 6/10, 301 shaders) ---
    {"gxffxshader", "GXFfxbumpTexture.vpo", "GXFfxbumpTexture.vpo", "Vertex(VS)", "None", 0u, 1u, 0x000000000934da13ull, 0x812a768eu, 2784u, 536u, 107u, 9066u, 0x000c00c9u, 0x00000018u, 40u, 32u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxcolorOnly.vpo", "GXFfxcolorOnly.vpo", "Vertex(VS)", "None", 1u, 1u, 0x000000005dba4ec5ull, 0x81532e81u, 1748u, 144u, 32u, 5083u, 0x000c00c2u, 0x00000018u, 12u, 32u, 12u, 0u, 0u, 1u, 28u, 0u, "user_sgpr[8]", "VSCommonBuffer", ""},
    {"gxffxshader", "GXFfxdistortion.vpo", "GXFfxdistortion.vpo", "Vertex(VS)", "None", 2u, 1u, 0x0000000065dd13b5ull, 0xff759accu, 1888u, 236u, 48u, 6304u, 0x000c00c2u, 0x00000018u, 12u, 32u, 12u, 0u, 0u, 1u, 90u, 0u, "user_sgpr[8]", "VSCommonBuffer", ""},
    {"gxffxshader", "GXFfxLightShaft.vpo", "GXFfxLightShaft.vpo", "Vertex(VS)", "None", 3u, 1u, 0x00000000637500e0ull, 0xffe2d665u, 652u, 60u, 9u, 1459u, 0x000c0045u, 0x0000000cu, 24u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxmultiTexture.vpo", "GXFfxmultiTexture.vpo", "Vertex(VS)", "None", 4u, 1u, 0x0000000050ef54e7ull, 0x0c528f61u, 2040u, 280u, 60u, 5990u, 0x000c00c4u, 0x00000018u, 20u, 32u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxpointSprite.vpo", "GXFfxpointSprite.vpo", "ExportShader(ES)", "None", 5u, 5u, 0x0000000089be0747ull, 0xffeb60aau, 2044u, 364u, 74u, 9443u, 0x000c0182u, 0x00000018u, 12u, 56u, 12u, 0u, 0u, 1u, 0u, 12u, "", "", ""},
    {"gxffxshader", "GXFfxradialBlur.vpo", "GXFfxradialBlur.vpo", "Vertex(VS)", "None", 6u, 1u, 0x00000000f81310f4ull, 0x72221862u, 440u, 36u, 6u, 1198u, 0x000c0043u, 0x0000000cu, 16u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxsimpleTexture.vpo", "GXFfxsimpleTexture.vpo", "Vertex(VS)", "None", 7u, 1u, 0x00000000ff9ac40bull, 0x8623407au, 1884u, 240u, 53u, 6706u, 0x000c00c2u, 0x00000018u, 12u, 32u, 12u, 0u, 0u, 1u, 100u, 0u, "user_sgpr[8]", "VSCommonBuffer", ""},
    {"gxffxshader", "GXFfxsoftBumpTexture.vpo", "GXFfxsoftBumpTexture.vpo", "Vertex(VS)", "None", 8u, 1u, 0x000000007d63e82dull, 0xc48b1181u, 3056u, 724u, 145u, 10767u, 0x000c008au, 0x00000018u, 44u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxsoftMultiTexture.vpo", "GXFfxsoftMultiTexture.vpo", "Vertex(VS)", "None", 9u, 1u, 0x00000000a54ce70eull, 0xb4f28e15u, 2368u, 476u, 99u, 7768u, 0x000c0085u, 0x00000018u, 24u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxsoftTexture.vpo", "GXFfxsoftTexture.vpo", "Vertex(VS)", "None", 10u, 1u, 0x0000000041ae600full, 0x6f24f9acu, 2440u, 472u, 98u, 7742u, 0x000c0086u, 0x00000018u, 28u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture.vpo", "GXFfxtessellateBumpTexture.vpo", "LocalShader(LS)", "LsCompute", 11u, 6u, 0x000000004cd21a0full, 0xff61863du, 964u, 156u, 22u, 3904u, 0x010c0048u, 0x0000000cu, 36u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateMultiTexture.vpo", "GXFfxtessellateMultiTexture.vpo", "LocalShader(LS)", "LsCompute", 12u, 6u, 0x000000008b2c5098ull, 0x02101d8eu, 984u, 164u, 23u, 4148u, 0x010c0048u, 0x0000000cu, 36u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellatePointSprite.vpo", "GXFfxtessellatePointSprite.vpo", "LocalShader(LS)", "LsCompute", 13u, 6u, 0x00000000c863cbbcull, 0x02fe9e13u, 1960u, 316u, 68u, 10449u, 0x010c0144u, 0x00000018u, 20u, 48u, 12u, 0u, 0u, 1u, 81u, 0u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "GXFfxtessellateQuadLineColorOnly.vpo", "GXFfxtessellateQuadLineColorOnly.vpo", "LocalShader(LS)", "LsCompute", 14u, 6u, 0x00000000df0f1658ull, 0x72949774u, 688u, 108u, 15u, 2899u, 0x010c0045u, 0x0000000cu, 24u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture.vpo", "GXFfxtessellateSoftBumpTexture.vpo", "LocalShader(LS)", "LsCompute", 15u, 6u, 0x000000004cd21a0full, 0xff61863du, 964u, 156u, 22u, 3904u, 0x010c0048u, 0x0000000cu, 36u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture.vpo", "GXFfxtessellateSoftMultiTexture.vpo", "LocalShader(LS)", "LsCompute", 16u, 6u, 0x00000000d2a80d7aull, 0x1f40a6dcu, 1088u, 180u, 25u, 4325u, 0x010c0049u, 0x0000000cu, 40u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture.vpo", "GXFfxtessellateSoftTexture.vpo", "LocalShader(LS)", "LsCompute", 17u, 6u, 0x000000004cd21a0full, 0xff61863du, 964u, 156u, 22u, 3904u, 0x010c0048u, 0x0000000cu, 36u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateTexture.vpo", "GXFfxtessellateTexture.vpo", "LocalShader(LS)", "LsCompute", 18u, 6u, 0x000000004dde1a10ull, 0x6fb3bcc3u, 964u, 156u, 22u, 3904u, 0x010c0048u, 0x0000000cu, 36u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateTracerBumpTexture.vpo", "GXFfxtessellateTracerBumpTexture.vpo", "LocalShader(LS)", "LsCompute", 19u, 6u, 0x00000000e8b6cda1ull, 0x6faeebe6u, 776u, 132u, 19u, 3319u, 0x010c0046u, 0x0000000cu, 28u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateTracerTexture.vpo", "GXFfxtessellateTracerTexture.vpo", "LocalShader(LS)", "LsCompute", 20u, 6u, 0x000000002cdb6271ull, 0xe2cc5196u, 608u, 96u, 14u, 2816u, 0x010c0044u, 0x0000000cu, 20u, 16u, 6u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "GXFfxtexture.vpo", "GXFfxtexture.vpo", "Vertex(VS)", "None", 21u, 1u, 0x000000004d803411ull, 0xf15db893u, 2132u, 280u, 60u, 5982u, 0x000c00c5u, 0x00000018u, 24u, 32u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "VS_VFX_Grid.vpo", "VS_VFX_Grid.vpo", "ExportShader(ES)", "None", 22u, 5u, 0x000000001d359a82ull, 0x86302b4eu, 1896u, 228u, 40u, 4708u, 0x000c0043u, 0x00000018u, 16u, 16u, 12u, 0u, 0u, 1u, 4u, 0u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "VS_VFX_OptStandardParticle.vpo", "VS_VFX_OptStandardParticle.vpo", "Vertex(VS)", "None", 23u, 1u, 0x000000000ed5d637ull, 0x86ca249du, 1756u, 300u, 61u, 7822u, 0x000c00c5u, 0x00000018u, 24u, 32u, 12u, 0u, 0u, 1u, 68u, 0u, "user_sgpr[8]", "gcbSceneParam", ""},
    {"gxffxshader", "VS_VFX_RayCastQuad.vpo", "VS_VFX_RayCastQuad.vpo", "Vertex(VS)", "None", 24u, 1u, 0x00000000a933f656ull, 0xb3655811u, 1364u, 120u, 27u, 3911u, 0x000c0081u, 0x00000018u, 8u, 24u, 12u, 0u, 0u, 1u, 52u, 0u, "user_sgpr[8]", "fcbVolumeRenderParam", ""},
    {"gxffxshader", "VS_VFX_RayDataBack.vpo", "VS_VFX_RayDataBack.vpo", "Vertex(VS)", "None", 25u, 1u, 0x00000000fb6aa40eull, 0x812d855eu, 1352u, 112u, 25u, 3553u, 0x000c0081u, 0x00000018u, 8u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "VS_VFX_RayDataFront.vpo", "VS_VFX_RayDataFront.vpo", "Vertex(VS)", "None", 26u, 1u, 0x000000003f6c174eull, 0x8653f82fu, 1388u, 104u, 23u, 3511u, 0x000c0081u, 0x00000018u, 8u, 24u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "VS_VFX_RayEdgeDetect.vpo", "VS_VFX_RayEdgeDetect.vpo", "Vertex(VS)", "None", 27u, 1u, 0x0000000053fb178bull, 0xb3fb35b1u, 1688u, 132u, 24u, 2488u, 0x000c0042u, 0x00000018u, 12u, 16u, 12u, 0u, 0u, 1u, 2u, 0u, "user_sgpr[8]", "fcbVolumeRenderParam", ""},
    {"gxffxshader", "VS_VFX_SparkParticle.vpo", "VS_VFX_SparkParticle.vpo", "ExportShader(ES)", "None", 28u, 5u, 0x00000000073ddffcull, 0xc491b587u, 2164u, 684u, 93u, 17368u, 0x000c004eu, 0x00000018u, 60u, 16u, 12u, 0u, 0u, 3u, 4u, 12u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "VS_VFX_StandardParticle.vpo", "VS_VFX_StandardParticle.vpo", "ExportShader(ES)", "None", 29u, 5u, 0x0000000016e3fa9full, 0xf12477a7u, 2064u, 452u, 77u, 10113u, 0x000c0044u, 0x00000018u, 20u, 16u, 12u, 0u, 0u, 3u, 4u, 12u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "VS_VFX_WanderingVision.vpo", "VS_VFX_WanderingVision.vpo", "ExportShader(ES)", "None", 30u, 5u, 0x00000000ac472950ull, 0x059d9c5fu, 1912u, 248u, 37u, 6590u, 0x000c0044u, 0x00000010u, 20u, 16u, 8u, 0u, 0u, 2u, 4u, 12u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GS_VFX_GsArray.gpo", "GS_VFX_GsArray.gpo", "GeometryShader(GS)", "None", 31u, 3u, 0x00000000f7f36d8aull, 0xe2a6653au, 2048u, 1048u, 162u, 12042u, 0x000c004bu, 0x00000004u, 48u, 16u, 2u, 0u, 0u, 1u, 0u, 20u, "", "", ""},
    {"gxffxshader", "GS_VFX_GsArrayLine.gpo", "GS_VFX_GsArrayLine.gpo", "GeometryShader(GS)", "None", 32u, 3u, 0x00000000c644ae75ull, 0xf1a97834u, 1688u, 712u, 111u, 8773u, 0x000c0047u, 0x00000004u, 32u, 16u, 2u, 0u, 0u, 1u, 0u, 20u, "", "", ""},
    {"gxffxshader", "GS_VFX_SparkParticle.gpo", "GS_VFX_SparkParticle.gpo", "GeometryShader(GS)", "None", 33u, 3u, 0x000000001635154cull, 0xf8ff9653u, 10724u, 8668u, 1590u, 104589u, 0x000c0149u, 0x00000010u, 40u, 48u, 8u, 0u, 0u, 2u, 68u, 20u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GS_VFX_StandardParticle.gpo", "GS_VFX_StandardParticle.gpo", "GeometryShader(GS)", "None", 34u, 3u, 0x000000008e3f8dc4ull, 0x05036639u, 8084u, 5976u, 1078u, 76183u, 0x000c0195u, 0x00000010u, 88u, 56u, 8u, 0u, 0u, 2u, 68u, 20u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GS_VFX_StandardParticleGenerateSdw.gpo", "GS_VFX_StandardParticleGenerateSdw.gpo", "GeometryShader(GS)", "None", 35u, 3u, 0x000000008e3f8dc4ull, 0x05036639u, 8084u, 5976u, 1078u, 76183u, 0x000c0195u, 0x00000010u, 88u, 56u, 8u, 0u, 0u, 2u, 68u, 20u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GS_VFX_WanderingVision.gpo", "GS_VFX_WanderingVision.gpo", "GeometryShader(GS)", "None", 36u, 3u, 0x00000000df83b1c0ull, 0xfff1c3abu, 4780u, 2752u, 496u, 38444u, 0x000c014bu, 0x00000010u, 48u, 48u, 8u, 0u, 0u, 2u, 96u, 20u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxpointSprite.gpo", "GXFfxpointSprite.gpo", "GeometryShader(GS)", "None", 37u, 3u, 0x0000000065081c67ull, 0xc4914fe5u, 1664u, 816u, 128u, 9823u, 0x000c00c4u, 0x00000004u, 20u, 32u, 2u, 0u, 0u, 1u, 0u, 20u, "", "", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture_octed.hpo", "GXFfxtessellateBumpTexture_octed.hpo", "HullShader(HS)", "HullTcs", 38u, 7u, 0x000000008453cd1cull, 0x0c2511b1u, 1460u, 368u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture_quad.hpo", "GXFfxtessellateBumpTexture_quad.hpo", "HullShader(HS)", "HullTcs", 39u, 7u, 0x00000000bc234799ull, 0x86b37b81u, 1436u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture_quincunx.hpo", "GXFfxtessellateBumpTexture_quincunx.hpo", "HullShader(HS)", "HullTcs", 40u, 7u, 0x00000000d67db0efull, 0x9c0d2db4u, 1492u, 364u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateMultiTexture_octed.hpo", "GXFfxtessellateMultiTexture_octed.hpo", "HullShader(HS)", "HullTcs", 41u, 7u, 0x00000000a26750c1ull, 0x187bd1f2u, 1468u, 368u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateMultiTexture_quad.hpo", "GXFfxtessellateMultiTexture_quad.hpo", "HullShader(HS)", "HullTcs", 42u, 7u, 0x0000000077043ad2ull, 0x92313cb3u, 1448u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellatePointSprite.hpo", "GXFfxtessellatePointSprite.hpo", "HullShader(HS)", "HullTcs", 43u, 7u, 0x00000000bb88db5full, 0x758781f0u, 1068u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateQuadLineColorOnly.hpo", "GXFfxtessellateQuadLineColorOnly.hpo", "HullShader(HS)", "HullTcs", 44u, 7u, 0x0000000034121ac6ull, 0x6f406f66u, 1160u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture_octed.hpo", "GXFfxtessellateSoftBumpTexture_octed.hpo", "HullShader(HS)", "HullTcs", 45u, 7u, 0x000000008453cd1cull, 0x0c2511b1u, 1460u, 368u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture_quad.hpo", "GXFfxtessellateSoftBumpTexture_quad.hpo", "HullShader(HS)", "HullTcs", 46u, 7u, 0x00000000bc234799ull, 0x86b37b81u, 1436u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture_quincunx.hpo", "GXFfxtessellateSoftBumpTexture_quincunx.hpo", "HullShader(HS)", "HullTcs", 47u, 7u, 0x00000000d67db0efull, 0x9c0d2db4u, 1492u, 364u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture_octed.hpo", "GXFfxtessellateSoftMultiTexture_octed.hpo", "HullShader(HS)", "HullTcs", 48u, 7u, 0x0000000090c6fb05ull, 0xff955268u, 1560u, 368u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture_quad.hpo", "GXFfxtessellateSoftMultiTexture_quad.hpo", "HullShader(HS)", "HullTcs", 49u, 7u, 0x00000000627ac5b9ull, 0xecfe5d98u, 1536u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture_octed.hpo", "GXFfxtessellateSoftTexture_octed.hpo", "HullShader(HS)", "HullTcs", 50u, 7u, 0x000000008453cd1cull, 0x0c2511b1u, 1460u, 368u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture_quad.hpo", "GXFfxtessellateSoftTexture_quad.hpo", "HullShader(HS)", "HullTcs", 51u, 7u, 0x00000000bc234799ull, 0x86b37b81u, 1436u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture_quincunx.hpo", "GXFfxtessellateSoftTexture_quincunx.hpo", "HullShader(HS)", "HullTcs", 52u, 7u, 0x00000000d67db0efull, 0x9c0d2db4u, 1492u, 364u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateTexture_octed.hpo", "GXFfxtessellateTexture_octed.hpo", "HullShader(HS)", "HullTcs", 53u, 7u, 0x000000008453cd1cull, 0x0c2511b1u, 1460u, 368u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateTexture_quad.hpo", "GXFfxtessellateTexture_quad.hpo", "HullShader(HS)", "HullTcs", 54u, 7u, 0x00000000bc234799ull, 0x86b37b81u, 1436u, 360u, 58u, 4776u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateTexture_quincunx.hpo", "GXFfxtessellateTexture_quincunx.hpo", "HullShader(HS)", "HullTcs", 55u, 7u, 0x00000000d67db0efull, 0x9c0d2db4u, 1492u, 364u, 59u, 4795u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateTracerBumpTexture.hpo", "GXFfxtessellateTracerBumpTexture.hpo", "HullShader(HS)", "HullTcs", 56u, 7u, 0x00000000ffd53a4dull, 0x7503ea3fu, 1212u, 356u, 57u, 4756u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateTracerTexture.hpo", "GXFfxtessellateTracerTexture.hpo", "HullShader(HS)", "HullTcs", 57u, 7u, 0x00000000096172a4ull, 0xff8123adu, 1084u, 356u, 57u, 4756u, 0x000c0043u, 0x00000010u, 16u, 16u, 8u, 0u, 0u, 2u, 4u, 40u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture_octed.dpo", "GXFfxtessellateBumpTexture_octed.dpo", "DomainShader(DS)", "Domain", 58u, 1u, 0x00000000416b04b7ull, 0xec8ed043u, 2920u, 736u, 141u, 12520u, 0x020c00c7u, 0x00000018u, 32u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture_quad.dpo", "GXFfxtessellateBumpTexture_quad.dpo", "DomainShader(DS)", "Domain", 59u, 1u, 0x00000000c4d6d9d5ull, 0x2a7f714bu, 2832u, 660u, 125u, 11929u, 0x020c00c8u, 0x00000018u, 36u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateBumpTexture_quincunx.dpo", "GXFfxtessellateBumpTexture_quincunx.dpo", "DomainShader(DS)", "Domain", 60u, 1u, 0x000000006486bb33ull, 0xb3080879u, 2988u, 780u, 160u, 14976u, 0x020c00c9u, 0x00000018u, 40u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateMultiTexture_octed.dpo", "GXFfxtessellateMultiTexture_octed.dpo", "DomainShader(DS)", "Domain", 61u, 1u, 0x00000000c96b77ebull, 0x66f63e77u, 2720u, 724u, 141u, 11866u, 0x020c00c6u, 0x00000018u, 28u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateMultiTexture_quad.dpo", "GXFfxtessellateMultiTexture_quad.dpo", "DomainShader(DS)", "Domain", 62u, 1u, 0x0000000053a58ae9ull, 0x72940904u, 2648u, 656u, 131u, 11351u, 0x020c00c6u, 0x00000018u, 28u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellatePointSprite.dpo", "GXFfxtessellatePointSprite.dpo", "DomainShader(DS)", "Domain", 63u, 1u, 0x000000009071184bull, 0x6fd0cdd6u, 740u, 248u, 45u, 3638u, 0x020c0044u, 0x00000010u, 20u, 16u, 8u, 0u, 0u, 1u, 7u, 0u, "user_sgpr[4]", "", ""},
    {"gxffxshader", "GXFfxtessellateQuadLineColorOnly.dpo", "GXFfxtessellateQuadLineColorOnly.dpo", "DomainShader(DS)", "Domain", 64u, 1u, 0x00000000deb29f08ull, 0xff81deb8u, 2100u, 324u, 63u, 7362u, 0x020c00c4u, 0x00000018u, 20u, 32u, 12u, 0u, 0u, 2u, 28u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture_octed.dpo", "GXFfxtessellateSoftBumpTexture_octed.dpo", "DomainShader(DS)", "Domain", 65u, 1u, 0x0000000025e144a5ull, 0x6105136au, 3220u, 940u, 181u, 14370u, 0x020c0088u, 0x00000018u, 36u, 24u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture_quad.dpo", "GXFfxtessellateSoftBumpTexture_quad.dpo", "DomainShader(DS)", "Domain", 66u, 1u, 0x00000000248eab7bull, 0xebb24974u, 3128u, 860u, 166u, 13786u, 0x020c0088u, 0x00000018u, 36u, 24u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture_quincunx.dpo", "GXFfxtessellateSoftBumpTexture_quincunx.dpo", "DomainShader(DS)", "Domain", 67u, 1u, 0x0000000014251300ull, 0x8f406bccu, 3340u, 1028u, 207u, 17114u, 0x020c00cau, 0x00000018u, 44u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture_octed.dpo", "GXFfxtessellateSoftMultiTexture_octed.dpo", "DomainShader(DS)", "Domain", 68u, 1u, 0x00000000ecb47dc1ull, 0xffe53f95u, 3068u, 928u, 181u, 13645u, 0x020c0086u, 0x00000018u, 28u, 24u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture_quad.dpo", "GXFfxtessellateSoftMultiTexture_quad.dpo", "DomainShader(DS)", "Domain", 69u, 1u, 0x000000004b999de8ull, 0x02e4aaadu, 2980u, 852u, 167u, 13118u, 0x020c0086u, 0x00000018u, 28u, 24u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture_octed.dpo", "GXFfxtessellateSoftTexture_octed.dpo", "DomainShader(DS)", "Domain", 70u, 1u, 0x000000007f03e945ull, 0x9bfeeeaeu, 2876u, 780u, 153u, 12820u, 0x020c0085u, 0x00000018u, 24u, 24u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture_quad.dpo", "GXFfxtessellateSoftTexture_quad.dpo", "DomainShader(DS)", "Domain", 71u, 1u, 0x000000000c1888bdull, 0xffebfe27u, 2800u, 708u, 140u, 12250u, 0x020c0085u, 0x00000018u, 24u, 24u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftTexture_quincunx.dpo", "GXFfxtessellateSoftTexture_quincunx.dpo", "DomainShader(DS)", "Domain", 72u, 1u, 0x000000000c6f6c39ull, 0x61f8ef0au, 3012u, 892u, 184u, 16255u, 0x020c00c8u, 0x00000018u, 36u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateTexture_octed.dpo", "GXFfxtessellateTexture_octed.dpo", "DomainShader(DS)", "Domain", 73u, 1u, 0x00000000da35f466ull, 0xf62474d1u, 2552u, 596u, 117u, 11014u, 0x020c00c5u, 0x00000018u, 24u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateTexture_quad.dpo", "GXFfxtessellateTexture_quad.dpo", "DomainShader(DS)", "Domain", 74u, 1u, 0x0000000052d756dfull, 0x11e26207u, 2484u, 528u, 105u, 10439u, 0x020c00c5u, 0x00000018u, 24u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateTexture_quincunx.dpo", "GXFfxtessellateTexture_quincunx.dpo", "DomainShader(DS)", "Domain", 75u, 1u, 0x00000000a4e5f5a7ull, 0x1116b636u, 2712u, 696u, 144u, 14304u, 0x020c00c7u, 0x00000018u, 32u, 32u, 12u, 0u, 0u, 2u, 96u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateTracerBumpTexture.dpo", "GXFfxtessellateTracerBumpTexture.dpo", "DomainShader(DS)", "Domain", 76u, 1u, 0x0000000009c5192aull, 0x0b38dc39u, 2812u, 800u, 154u, 12775u, 0x020c00cau, 0x00000018u, 44u, 32u, 12u, 0u, 0u, 2u, 72u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateTracerTexture.dpo", "GXFfxtessellateTracerTexture.dpo", "DomainShader(DS)", "Domain", 77u, 1u, 0x000000008ceb6e7cull, 0x021e353cu, 2344u, 552u, 112u, 9864u, 0x020c00c5u, 0x00000018u, 24u, 32u, 12u, 0u, 0u, 2u, 66u, 0u, "user_sgpr[4]", "DSCommonBuffer", ""},
    {"gxffxshader", "CS_VFX_BitonicSort.cpo", "CS_VFX_BitonicSort.cpo", "Compute(CS)", "None", 78u, 4u, 0x00000000aaeca963ull, 0x6fd04e24u, 912u, 548u, 110u, 12815u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_BitonicSort1024.cpo", "CS_VFX_BitonicSort1024.cpo", "Compute(CS)", "None", 79u, 4u, 0x00000000baf640d5ull, 0xff5d4d79u, 928u, 560u, 112u, 12823u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_BitonicSort128.cpo", "CS_VFX_BitonicSort128.cpo", "Compute(CS)", "None", 80u, 4u, 0x00000000a376364full, 0xb36c6a89u, 928u, 560u, 112u, 12819u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_BitonicSort256.cpo", "CS_VFX_BitonicSort256.cpo", "Compute(CS)", "None", 81u, 4u, 0x00000000a89e37d1ull, 0xffff182au, 928u, 560u, 112u, 12823u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_BitonicSort512.cpo", "CS_VFX_BitonicSort512.cpo", "Compute(CS)", "None", 82u, 4u, 0x00000000afc83ad3ull, 0xf1241d69u, 928u, 560u, 112u, 12823u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_DrawParticle.cpo", "CS_VFX_DrawParticle.cpo", "Compute(CS)", "None", 83u, 4u, 0x00000000516411e6ull, 0x721ee694u, 3520u, 1192u, 225u, 25180u, 0x000c0109u, 0x0000101au, 40u, 40u, 13u, 0u, 0u, 3u, 48u, 8u, "user_sgpr[2]->load(+16)", "", ""},
    {"gxffxshader", "CS_VFX_GridSlidY.cpo", "CS_VFX_GridSlidY.cpo", "Compute(CS)", "None", 84u, 4u, 0x0000000015ee6ca8ull, 0x7273dccfu, 1736u, 284u, 64u, 6589u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 2u, 0u, 2u, 38u, 8u, "user_sgpr[12]", "", ""},
    {"gxffxshader", "CS_VFX_InitSort.cpo", "CS_VFX_InitSort.cpo", "Compute(CS)", "None", 85u, 4u, 0x000000009c0d2d94ull, 0x7c31de08u, 5204u, 180u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_InitSort1024.cpo", "CS_VFX_InitSort1024.cpo", "Compute(CS)", "None", 86u, 4u, 0x00000000d7003d5aull, 0x7c522227u, 5204u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_InitSort128.cpo", "CS_VFX_InitSort128.cpo", "Compute(CS)", "None", 87u, 4u, 0x000000000b2232d4ull, 0x86a71e45u, 5204u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_InitSort256.cpo", "CS_VFX_InitSort256.cpo", "Compute(CS)", "None", 88u, 4u, 0x000000000f033456ull, 0x5d1b40bfu, 5204u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_InitSort512.cpo", "CS_VFX_InitSort512.cpo", "Compute(CS)", "None", 89u, 4u, 0x0000000002f33758ull, 0xff922ca4u, 5204u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkBitonicSort.cpo", "CS_VFX_SparkBitonicSort.cpo", "Compute(CS)", "None", 90u, 4u, 0x00000000aaeca963ull, 0x6fd04e24u, 912u, 548u, 110u, 12815u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_SparkBitonicSort1024.cpo", "CS_VFX_SparkBitonicSort1024.cpo", "Compute(CS)", "None", 91u, 4u, 0x00000000baf640d5ull, 0xff5d4d79u, 928u, 560u, 112u, 12823u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_SparkBitonicSort128.cpo", "CS_VFX_SparkBitonicSort128.cpo", "Compute(CS)", "None", 92u, 4u, 0x00000000a376364full, 0xb36c6a89u, 928u, 560u, 112u, 12819u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_SparkBitonicSort256.cpo", "CS_VFX_SparkBitonicSort256.cpo", "Compute(CS)", "None", 93u, 4u, 0x00000000a89e37d1ull, 0xffff182au, 928u, 560u, 112u, 12823u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_SparkBitonicSort512.cpo", "CS_VFX_SparkBitonicSort512.cpo", "Compute(CS)", "None", 94u, 4u, 0x00000000afc83ad3ull, 0xf1241d69u, 928u, 560u, 112u, 12823u, 0x000c0083u, 0x00001020u, 16u, 24u, 16u, 0u, 0u, 4u, 1u, 4u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_SparkInitSort.cpo", "CS_VFX_SparkInitSort.cpo", "Compute(CS)", "None", 95u, 4u, 0x000000008ea6e07dull, 0x7c3f2a27u, 4800u, 180u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkInitSort1024.cpo", "CS_VFX_SparkInitSort1024.cpo", "Compute(CS)", "None", 96u, 4u, 0x000000005751f043ull, 0x618f6b70u, 4800u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkInitSort128.cpo", "CS_VFX_SparkInitSort128.cpo", "Compute(CS)", "None", 97u, 4u, 0x00000000d936e5bdull, 0xf1de859bu, 4800u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkInitSort256.cpo", "CS_VFX_SparkInitSort256.cpo", "Compute(CS)", "None", 98u, 4u, 0x00000000ac82e73full, 0x6f538ee2u, 4800u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkInitSort512.cpo", "CS_VFX_SparkInitSort512.cpo", "Compute(CS)", "None", 99u, 4u, 0x0000000040dcea41ull, 0xec311df6u, 4800u, 184u, 37u, 9085u, 0x000c00c1u, 0x00001020u, 8u, 32u, 16u, 0u, 0u, 4u, 228u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticle.cpo", "CS_VFX_SparkParticle.cpo", "Compute(CS)", "None", 100u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeHigher.cpo", "CS_VFX_SparkParticleCounterDownSizeHigher.cpo", "Compute(CS)", "None", 101u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeHighest.cpo", "CS_VFX_SparkParticleCounterDownSizeHighest.cpo", "Compute(CS)", "None", 102u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeLower.cpo", "CS_VFX_SparkParticleCounterDownSizeLower.cpo", "Compute(CS)", "None", 103u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeLowest.cpo", "CS_VFX_SparkParticleCounterDownSizeLowest.cpo", "Compute(CS)", "None", 104u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeMiddle.cpo", "CS_VFX_SparkParticleCounterDownSizeMiddle.cpo", "Compute(CS)", "None", 105u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeMiddleHigher.cpo", "CS_VFX_SparkParticleCounterDownSizeMiddleHigher.cpo", "Compute(CS)", "None", 106u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleCounterDownSizeMiddleLower.cpo", "CS_VFX_SparkParticleCounterDownSizeMiddleLower.cpo", "Compute(CS)", "None", 107u, 4u, 0x0000000081f0e75aull, 0x0b2b06c0u, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleEmit.cpo", "CS_VFX_SparkParticleEmit.cpo", "Compute(CS)", "None", 108u, 4u, 0x00000000a509af23ull, 0xf8ec00e2u, 17668u, 10516u, 2045u, 144250u, 0x000c01cdu, 0x0000109au, 56u, 64u, 13u, 0u, 0u, 4u, 272u, 16u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleHigher.cpo", "CS_VFX_SparkParticleHigher.cpo", "Compute(CS)", "None", 109u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleHighest.cpo", "CS_VFX_SparkParticleHighest.cpo", "Compute(CS)", "None", 110u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleLower.cpo", "CS_VFX_SparkParticleLower.cpo", "Compute(CS)", "None", 111u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleLowest.cpo", "CS_VFX_SparkParticleLowest.cpo", "Compute(CS)", "None", 112u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleMiddle.cpo", "CS_VFX_SparkParticleMiddle.cpo", "Compute(CS)", "None", 113u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleMiddleHigher.cpo", "CS_VFX_SparkParticleMiddleHigher.cpo", "Compute(CS)", "None", 114u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_SparkParticleMiddleLower.cpo", "CS_VFX_SparkParticleMiddleLower.cpo", "Compute(CS)", "None", 115u, 4u, 0x0000000042f2a521ull, 0xc4915cedu, 29760u, 23244u, 4957u, 331131u, 0x000c02ecu, 0x0000009au, 180u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticle.cpo", "CS_VFX_StandardParticle.cpo", "Compute(CS)", "None", 116u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeHigher.cpo", "CS_VFX_StandardParticleCounterDownSizeHigher.cpo", "Compute(CS)", "None", 117u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeHighest.cpo", "CS_VFX_StandardParticleCounterDownSizeHighest.cpo", "Compute(CS)", "None", 118u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeLower.cpo", "CS_VFX_StandardParticleCounterDownSizeLower.cpo", "Compute(CS)", "None", 119u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeLowest.cpo", "CS_VFX_StandardParticleCounterDownSizeLowest.cpo", "Compute(CS)", "None", 120u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeMiddle.cpo", "CS_VFX_StandardParticleCounterDownSizeMiddle.cpo", "Compute(CS)", "None", 121u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeMiddleHigher.cpo", "CS_VFX_StandardParticleCounterDownSizeMiddleHigher.cpo", "Compute(CS)", "None", 122u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleCounterDownSizeMiddleLower.cpo", "CS_VFX_StandardParticleCounterDownSizeMiddleLower.cpo", "Compute(CS)", "None", 123u, 4u, 0x00000000a954e79dull, 0x1824291fu, 432u, 172u, 35u, 6521u, 0x000c0042u, 0x00001018u, 12u, 16u, 12u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleEmit.cpo", "CS_VFX_StandardParticleEmit.cpo", "Compute(CS)", "None", 124u, 4u, 0x000000004ca76892ull, 0x61f64d20u, 16312u, 12304u, 2392u, 170809u, 0x000c02ceu, 0x0000109au, 60u, 96u, 13u, 0u, 0u, 3u, 180u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleGenerateSdw.cpo", "CS_VFX_StandardParticleGenerateSdw.cpo", "Compute(CS)", "None", 125u, 4u, 0x000000006962c537ull, 0x1f5a0544u, 792u, 348u, 77u, 7166u, 0x000c00c3u, 0x00000820u, 16u, 32u, 16u, 2u, 0u, 2u, 8u, 8u, "user_sgpr[12]", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleHigher.cpo", "CS_VFX_StandardParticleHigher.cpo", "Compute(CS)", "None", 126u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleHighest.cpo", "CS_VFX_StandardParticleHighest.cpo", "Compute(CS)", "None", 127u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleLower.cpo", "CS_VFX_StandardParticleLower.cpo", "Compute(CS)", "None", 128u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleLowest.cpo", "CS_VFX_StandardParticleLowest.cpo", "Compute(CS)", "None", 129u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleMiddle.cpo", "CS_VFX_StandardParticleMiddle.cpo", "Compute(CS)", "None", 130u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleMiddleHigher.cpo", "CS_VFX_StandardParticleMiddleHigher.cpo", "Compute(CS)", "None", 131u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_StandardParticleMiddleLower.cpo", "CS_VFX_StandardParticleMiddleLower.cpo", "Compute(CS)", "None", 132u, 4u, 0x000000002da7fe60ull, 0x0b384bf6u, 31088u, 23876u, 4969u, 332243u, 0x000c02ebu, 0x0000109au, 176u, 96u, 13u, 4u, 0u, 4u, 581u, 44u, "user_sgpr[2]->load(+160)", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionAddParticle.cpo", "CS_VFX_WanderingVisionAddParticle.cpo", "Compute(CS)", "None", 133u, 4u, 0x0000000040229f02ull, 0x1fd7e3bdu, 10536u, 3788u, 729u, 63072u, 0x000c024du, 0x0000009au, 56u, 80u, 13u, 0u, 0u, 5u, 581u, 12u, "user_sgpr[2]->load(+32)", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionCounterDownSize.cpo", "CS_VFX_WanderingVisionCounterDownSize.cpo", "Compute(CS)", "None", 134u, 4u, 0x000000006bbf53c0ull, 0x7bc26412u, 948u, 292u, 60u, 11537u, 0x000c0081u, 0x000010a0u, 8u, 24u, 16u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionMargeEmitter.cpo", "CS_VFX_WanderingVisionMargeEmitter.cpo", "Compute(CS)", "None", 135u, 4u, 0x0000000039a26e28ull, 0x1f117fc1u, 1016u, 176u, 35u, 5122u, 0x000c0081u, 0x000000a0u, 8u, 24u, 16u, 0u, 0u, 3u, 3u, 0u, "user_sgpr[8]", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionModelToEmitter.cpo", "CS_VFX_WanderingVisionModelToEmitter.cpo", "Compute(CS)", "None", 136u, 4u, 0x00000000f48b0163ull, 0x2ae6967eu, 4504u, 320u, 62u, 7030u, 0x000c0082u, 0x000000a0u, 12u, 24u, 16u, 0u, 0u, 3u, 37u, 4u, "user_sgpr[2]->load(+0)", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionSort.cpo", "CS_VFX_WanderingVisionSort.cpo", "Compute(CS)", "None", 137u, 4u, 0x00000000432e0ba7ull, 0x66f6c462u, 6956u, 1736u, 271u, 54696u, 0x000c0090u, 0x00001020u, 68u, 24u, 16u, 0u, 0u, 2u, 260u, 0u, "user_sgpr[12]", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionUpdateParticle.cpo", "CS_VFX_WanderingVisionUpdateParticle.cpo", "Compute(CS)", "None", 138u, 4u, 0x0000000054b91773ull, 0x925bc7c1u, 25888u, 18448u, 3909u, 246182u, 0x000c01a5u, 0x0000009au, 152u, 56u, 13u, 2u, 0u, 4u, 581u, 32u, "user_sgpr[2]->load(+112)", "", ""},
    {"gxffxshader", "CS_VFX_WanderingVisionUpdateParticle128.cpo", "CS_VFX_WanderingVisionUpdateParticle128.cpo", "Compute(CS)", "None", 139u, 4u, 0x0000000097396c30ull, 0x1fd07a4au, 25888u, 18448u, 3909u, 246207u, 0x000c01a5u, 0x0000009au, 152u, 56u, 13u, 2u, 0u, 4u, 581u, 32u, "user_sgpr[2]->load(+112)", "", ""},
    {"gxffxshader", "GXFfxbumpDistorsionTexture.ppo", "GXFfxbumpDistorsionTexture.ppo", "Pixel(PS)", "None", 140u, 2u, 0x000000009a4bfbf9ull, 0x7c2c0a52u, 5208u, 1268u, 265u, 19383u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 3u, 1u, 3u, 96u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpDistorsionTextureGlowD.ppo", "GXFfxbumpDistorsionTextureGlowD.ppo", "Pixel(PS)", "None", 141u, 2u, 0x0000000062daa321ull, 0xf6a975a2u, 5288u, 1308u, 274u, 20406u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 96u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpDistorsionTextureGlowLP.ppo", "GXFfxbumpDistorsionTextureGlowLP.ppo", "Pixel(PS)", "None", 142u, 2u, 0x0000000036146a42ull, 0x5d62e535u, 5296u, 1312u, 274u, 20414u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 96u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture.ppo", "GXFfxbumpTexture.ppo", "Pixel(PS)", "None", 143u, 2u, 0x000000009198e7a4ull, 0xb3623c00u, 5812u, 1620u, 346u, 27447u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTextureGlowD.ppo", "GXFfxbumpTextureGlowD.ppo", "Pixel(PS)", "None", 144u, 2u, 0x0000000061c674b3ull, 0xece460a2u, 5884u, 1660u, 355u, 28960u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTextureGlowLP.ppo", "GXFfxbumpTextureGlowLP.ppo", "Pixel(PS)", "None", 145u, 2u, 0x00000000907651c8ull, 0xe55b1923u, 5892u, 1664u, 355u, 28968u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT1.ppo", "GXFfxbumpTexture_PNT1.ppo", "Pixel(PS)", "None", 146u, 2u, 0x0000000048ea848full, 0x954604f8u, 5816u, 1620u, 346u, 27447u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT1GlowD.ppo", "GXFfxbumpTexture_PNT1GlowD.ppo", "Pixel(PS)", "None", 147u, 2u, 0x00000000821171caull, 0xf1c3cfe0u, 5892u, 1660u, 355u, 28960u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT1GlowLP.ppo", "GXFfxbumpTexture_PNT1GlowLP.ppo", "Pixel(PS)", "None", 148u, 2u, 0x0000000025154edfull, 0xffffb382u, 5900u, 1664u, 355u, 28968u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT2.ppo", "GXFfxbumpTexture_PNT2.ppo", "Pixel(PS)", "None", 149u, 2u, 0x000000004fea858full, 0xff6fd9e5u, 5816u, 1620u, 346u, 27447u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT2GlowD.ppo", "GXFfxbumpTexture_PNT2GlowD.ppo", "Pixel(PS)", "None", 150u, 2u, 0x00000000981173caull, 0x0c41809cu, 5892u, 1660u, 355u, 28960u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT2GlowLP.ppo", "GXFfxbumpTexture_PNT2GlowLP.ppo", "Pixel(PS)", "None", 151u, 2u, 0x000000003b1550dfull, 0x02ede1eeu, 5900u, 1664u, 355u, 28968u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT3.ppo", "GXFfxbumpTexture_PNT3.ppo", "Pixel(PS)", "None", 152u, 2u, 0x0000000056ea868full, 0xe5253f07u, 5816u, 1620u, 346u, 27447u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT3GlowD.ppo", "GXFfxbumpTexture_PNT3GlowD.ppo", "Pixel(PS)", "None", 153u, 2u, 0x00000000ae1175caull, 0x1661d13au, 5892u, 1660u, 355u, 28960u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxbumpTexture_PNT3GlowLP.ppo", "GXFfxbumpTexture_PNT3GlowLP.ppo", "Pixel(PS)", "None", 154u, 2u, 0x00000000511552dfull, 0xffffffffu, 5900u, 1664u, 355u, 28968u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxcolorOnly.ppo", "GXFfxcolorOnly.ppo", "Pixel(PS)", "None", 155u, 2u, 0x000000005ea42ad3ull, 0x0b1723eeu, 3860u, 392u, 73u, 7035u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 2u, 52u, 0u, "user_sgpr[4]", "PSCommonBuffer", ""},
    {"gxffxshader", "GXFfxcolorOnlyGlowD.ppo", "GXFfxcolorOnlyGlowD.ppo", "Pixel(PS)", "None", 156u, 2u, 0x00000000440ece4cull, 0xff758095u, 3932u, 432u, 82u, 8038u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 2u, 92u, 0u, "user_sgpr[4]", "PSCommonBuffer", ""},
    {"gxffxshader", "GXFfxcolorOnlyGlowLP.ppo", "GXFfxcolorOnlyGlowLP.ppo", "Pixel(PS)", "None", 157u, 2u, 0x00000000293fab51ull, 0xb48289b1u, 3932u, 436u, 82u, 8046u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 2u, 92u, 0u, "user_sgpr[4]", "PSCommonBuffer", ""},
    {"gxffxshader", "GXFfxdistortion.ppo", "GXFfxdistortion.ppo", "Pixel(PS)", "None", 158u, 2u, 0x0000000096b84030ull, 0xb38b58b8u, 3860u, 988u, 193u, 13098u, 0x000c0103u, 0x00000020u, 16u, 40u, 16u, 2u, 1u, 2u, 96u, 12u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_depthTexture"},
    {"gxffxshader", "GXFfxdistortionBump.ppo", "GXFfxdistortionBump.ppo", "Pixel(PS)", "None", 159u, 2u, 0x000000000150b093ull, 0x812a0f48u, 3884u, 964u, 186u, 14210u, 0x000c0103u, 0x00000020u, 16u, 40u, 16u, 3u, 1u, 2u, 96u, 20u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_normalTexture"},
    {"gxffxshader", "GXFfxdistortionBumpTexture.ppo", "GXFfxdistortionBumpTexture.ppo", "Pixel(PS)", "None", 160u, 2u, 0x0000000031c21c09ull, 0xebf9500au, 3872u, 896u, 176u, 16031u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 4u, 1u, 2u, 96u, 28u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_normalTexture"},
    {"gxffxshader", "GXFfxdistortionRotation.ppo", "GXFfxdistortionRotation.ppo", "Pixel(PS)", "None", 161u, 2u, 0x00000000666b386aull, 0x81a0ee41u, 3916u, 1036u, 201u, 13320u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 2u, 1u, 2u, 96u, 12u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_depthTexture"},
    {"gxffxshader", "GXFfxdistortionTexture.ppo", "GXFfxdistortionTexture.ppo", "Pixel(PS)", "None", 162u, 2u, 0x000000005b2d28e9ull, 0x9c7aba21u, 4024u, 1100u, 219u, 16105u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 3u, 1u, 2u, 96u, 20u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_depthTexture"},
    {"gxffxshader", "GXFfxLightShaftProcedual.ppo", "GXFfxLightShaftProcedual.ppo", "Pixel(PS)", "None", 163u, 2u, 0x000000007fdd54e1ull, 0x0cbc66a5u, 3224u, 336u, 75u, 5664u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 2u, 66u, 4u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_depthTexture"},
    {"gxffxshader", "GXFfxLightShaftProcedualMask.ppo", "GXFfxLightShaftProcedualMask.ppo", "Pixel(PS)", "None", 164u, 2u, 0x00000000e0386541ull, 0x88927fa0u, 3324u, 384u, 87u, 7374u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 2u, 1u, 2u, 66u, 12u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_depthTexture"},
    {"gxffxshader", "GXFfxLightShaftTexture.ppo", "GXFfxLightShaftTexture.ppo", "Pixel(PS)", "None", 165u, 2u, 0x00000000e55e45ebull, 0x750a69c6u, 3060u, 276u, 63u, 5884u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 2u, 70u, 4u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxmultiTexture.ppo", "GXFfxmultiTexture.ppo", "Pixel(PS)", "None", 166u, 2u, 0x00000000b970637full, 0xf1531385u, 6196u, 2452u, 479u, 35174u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxmultiTexture2Sampler.ppo", "GXFfxmultiTexture2Sampler.ppo", "Pixel(PS)", "None", 167u, 2u, 0x00000000697bdda7ull, 0x8f3715ddu, 5636u, 1968u, 388u, 28809u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 1u, 3u, 109u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxmultiTexture2SamplerGlowD.ppo", "GXFfxmultiTexture2SamplerGlowD.ppo", "Pixel(PS)", "None", 168u, 2u, 0x0000000036232d4dull, 0x18535799u, 5716u, 2012u, 398u, 29764u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 1u, 3u, 109u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxmultiTexture2SamplerGlowLP.ppo", "GXFfxmultiTexture2SamplerGlowLP.ppo", "Pixel(PS)", "None", 169u, 2u, 0x0000000003a1fc68ull, 0xe2489ff1u, 5724u, 2016u, 398u, 29772u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 1u, 3u, 109u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxmultiTextureGlowD.ppo", "GXFfxmultiTextureGlowD.ppo", "Pixel(PS)", "None", 170u, 2u, 0x00000000dee0bdfcull, 0x954f3603u, 6276u, 2496u, 489u, 36129u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxmultiTextureGlowLP.ppo", "GXFfxmultiTextureGlowLP.ppo", "Pixel(PS)", "None", 171u, 2u, 0x00000000d5308d05ull, 0x118fd25au, 6276u, 2500u, 489u, 36137u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxpointSprite.ppo", "GXFfxpointSprite.ppo", "Pixel(PS)", "None", 172u, 2u, 0x000000005a402daeull, 0xc46c5a76u, 4108u, 492u, 98u, 9259u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 3u, 52u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxpointSpriteGlowD.ppo", "GXFfxpointSpriteGlowD.ppo", "Pixel(PS)", "None", 173u, 2u, 0x0000000088ae6537ull, 0xec67a751u, 4196u, 548u, 110u, 10291u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 3u, 92u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxpointSpriteGlowLP.ppo", "GXFfxpointSpriteGlowLP.ppo", "Pixel(PS)", "None", 174u, 2u, 0x00000000e1c23440ull, 0xf14ed481u, 4196u, 552u, 110u, 10299u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 3u, 92u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxradialBlur.ppo", "GXFfxradialBlur.ppo", "Pixel(PS)", "None", 175u, 2u, 0x0000000000d506a1ull, 0x728008bfu, 3920u, 1084u, 227u, 17407u, 0x000c00ccu, 0x00000020u, 52u, 32u, 16u, 2u, 1u, 2u, 96u, 12u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_normalTexture"},
    {"gxffxshader", "GXFfxsimpleTexture.ppo", "GXFfxsimpleTexture.ppo", "Pixel(PS)", "None", 176u, 2u, 0x00000000cef49cceull, 0xf1a7a45cu, 4444u, 836u, 167u, 13937u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 116u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsimpleTextureGlowD.ppo", "GXFfxsimpleTextureGlowD.ppo", "Pixel(PS)", "None", 177u, 2u, 0x000000006b34038cull, 0x6fba8ea1u, 4516u, 880u, 177u, 14969u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 116u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsimpleTextureGlowLP.ppo", "GXFfxsimpleTextureGlowLP.ppo", "Pixel(PS)", "None", 178u, 2u, 0x000000002bacd4a6ull, 0x7b41be35u, 4524u, 884u, 177u, 14977u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 116u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture.ppo", "GXFfxsoftBumpTexture.ppo", "Pixel(PS)", "None", 179u, 2u, 0x00000000498c09f3ull, 0xf15ad5ddu, 6120u, 1804u, 384u, 31652u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTextureGlowD.ppo", "GXFfxsoftBumpTextureGlowD.ppo", "Pixel(PS)", "None", 180u, 2u, 0x00000000067d6fddull, 0xe52bf08eu, 6192u, 1848u, 394u, 33256u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTextureGlowLP.ppo", "GXFfxsoftBumpTextureGlowLP.ppo", "Pixel(PS)", "None", 181u, 2u, 0x00000000215d3ef9ull, 0x61026310u, 6200u, 1852u, 394u, 33264u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT1.ppo", "GXFfxsoftBumpTexture_PNT1.ppo", "Pixel(PS)", "None", 182u, 2u, 0x0000000022ec2ff8ull, 0x68c44b0eu, 6124u, 1808u, 385u, 31673u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT1GlowD.ppo", "GXFfxsoftBumpTexture_PNT1GlowD.ppo", "Pixel(PS)", "None", 183u, 2u, 0x00000000df10f625ull, 0xc444bf08u, 6212u, 1852u, 395u, 33277u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT1GlowLP.ppo", "GXFfxsoftBumpTexture_PNT1GlowLP.ppo", "Pixel(PS)", "None", 184u, 2u, 0x00000000ff10c52eull, 0x72ccf60au, 6212u, 1856u, 395u, 33285u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT2.ppo", "GXFfxsoftBumpTexture_PNT2.ppo", "Pixel(PS)", "None", 185u, 2u, 0x0000000028ec30f8ull, 0x7510b478u, 6124u, 1808u, 385u, 31673u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT2GlowD.ppo", "GXFfxsoftBumpTexture_PNT2GlowD.ppo", "Pixel(PS)", "None", 186u, 2u, 0x00000000f710f825ull, 0xff05178eu, 6212u, 1852u, 395u, 33277u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT2GlowLP.ppo", "GXFfxsoftBumpTexture_PNT2GlowLP.ppo", "Pixel(PS)", "None", 187u, 2u, 0x000000001711c72eull, 0x027dd478u, 6212u, 1856u, 395u, 33285u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT3.ppo", "GXFfxsoftBumpTexture_PNT3.ppo", "Pixel(PS)", "None", 188u, 2u, 0x000000002eec31f8ull, 0x0517e356u, 6124u, 1808u, 385u, 31673u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT3GlowD.ppo", "GXFfxsoftBumpTexture_PNT3GlowD.ppo", "Pixel(PS)", "None", 189u, 2u, 0x000000000f11fa25ull, 0x72198c9fu, 6212u, 1852u, 395u, 33277u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftBumpTexture_PNT3GlowLP.ppo", "GXFfxsoftBumpTexture_PNT3GlowLP.ppo", "Pixel(PS)", "None", 190u, 2u, 0x000000002f11c92eull, 0x9bfe0e8fu, 6212u, 1856u, 395u, 33285u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftMultiTexture.ppo", "GXFfxsoftMultiTexture.ppo", "Pixel(PS)", "None", 191u, 2u, 0x000000001587ddc9ull, 0x169b4ec8u, 6536u, 2672u, 523u, 39934u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 4u, 1u, 3u, 109u, 32u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxsoftMultiTexture2Sampler.ppo", "GXFfxsoftMultiTexture2Sampler.ppo", "Pixel(PS)", "None", 192u, 2u, 0x00000000fa0033daull, 0x11caf0fdu, 5948u, 2152u, 426u, 32906u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxsoftMultiTexture2SamplerGlowD.ppo", "GXFfxsoftMultiTexture2SamplerGlowD.ppo", "Pixel(PS)", "None", 193u, 2u, 0x0000000097299036ull, 0x7c5c697du, 6028u, 2196u, 436u, 33861u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxsoftMultiTexture2SamplerGlowLP.ppo", "GXFfxsoftMultiTexture2SamplerGlowLP.ppo", "Pixel(PS)", "None", 194u, 2u, 0x000000005c0b5f52ull, 0x5de63f24u, 6036u, 2200u, 436u, 33869u, 0x000c0105u, 0x00000020u, 24u, 40u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxsoftMultiTextureGlowD.ppo", "GXFfxsoftMultiTextureGlowD.ppo", "Pixel(PS)", "None", 195u, 2u, 0x00000000cf8a4518ull, 0x0cd16784u, 6616u, 2716u, 533u, 40889u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 4u, 1u, 3u, 109u, 32u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxsoftMultiTextureGlowLP.ppo", "GXFfxsoftMultiTextureGlowLP.ppo", "Pixel(PS)", "None", 196u, 2u, 0x00000000eb211421ull, 0x61f1daf6u, 6616u, 2720u, 533u, 40897u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 4u, 1u, 3u, 109u, 32u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxsoftTexture.ppo", "GXFfxsoftTexture.ppo", "Pixel(PS)", "None", 197u, 2u, 0x000000005e224cb6ull, 0xf86f591fu, 4808u, 1004u, 201u, 15139u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 3u, 96u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftTextureGlowD.ppo", "GXFfxsoftTextureGlowD.ppo", "Pixel(PS)", "None", 198u, 2u, 0x00000000898dabb3ull, 0x9c9318e2u, 4888u, 1048u, 211u, 16144u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 3u, 96u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxsoftTextureGlowLP.ppo", "GXFfxsoftTextureGlowLP.ppo", "Pixel(PS)", "None", 199u, 2u, 0x0000000025527abcull, 0x7cb26006u, 4888u, 1052u, 211u, 16152u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 3u, 96u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateBumpTexture.ppo", "GXFfxtessellateBumpTexture.ppo", "Pixel(PS)", "None", 200u, 2u, 0x000000000df37378ull, 0x6fb4e3bbu, 5740u, 1608u, 341u, 28030u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateBumpTextureGlowD.ppo", "GXFfxtessellateBumpTextureGlowD.ppo", "Pixel(PS)", "None", 201u, 2u, 0x00000000c032cf67ull, 0x7cd61174u, 5812u, 1652u, 351u, 29566u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateBumpTextureGlowLP.ppo", "GXFfxtessellateBumpTextureGlowLP.ppo", "Pixel(PS)", "None", 202u, 2u, 0x00000000d70b9e83ull, 0xf816965bu, 5820u, 1656u, 351u, 29574u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateMultiTexture.ppo", "GXFfxtessellateMultiTexture.ppo", "Pixel(PS)", "None", 203u, 2u, 0x0000000081b3639bull, 0x0c410a6eu, 6236u, 2488u, 485u, 35804u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateMultiTexture2Sampler.ppo", "GXFfxtessellateMultiTexture2Sampler.ppo", "Pixel(PS)", "None", 204u, 2u, 0x00000000b382dd8full, 0x95c2b482u, 5636u, 1968u, 388u, 28805u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 1u, 3u, 109u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateMultiTexture2SamplerGlowD.ppo", "GXFfxtessellateMultiTexture2SamplerGlowD.ppo", "Pixel(PS)", "None", 205u, 2u, 0x000000007c6a2d35ull, 0xebfead9fu, 5716u, 2012u, 398u, 29760u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 1u, 3u, 109u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateMultiTexture2SamplerGlowLP.ppo", "GXFfxtessellateMultiTexture2SamplerGlowLP.ppo", "Pixel(PS)", "None", 206u, 2u, 0x000000004988fc50ull, 0x169bc4a4u, 5724u, 2016u, 398u, 29768u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 2u, 1u, 3u, 109u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateMultiTextureGlowD.ppo", "GXFfxtessellateMultiTextureGlowD.ppo", "Pixel(PS)", "None", 207u, 2u, 0x0000000058c0be1full, 0xe5692fbcu, 6308u, 2532u, 495u, 36759u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateMultiTextureGlowLP.ppo", "GXFfxtessellateMultiTextureGlowLP.ppo", "Pixel(PS)", "None", 208u, 2u, 0x00000000cda98d3bull, 0x166103cdu, 6316u, 2536u, 495u, 36767u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellatePointSprite.ppo", "GXFfxtessellatePointSprite.ppo", "Pixel(PS)", "None", 209u, 2u, 0x000000005a402daeull, 0xc46c5a76u, 4108u, 492u, 98u, 9259u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 3u, 52u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellatePointSpriteGlowD.ppo", "GXFfxtessellatePointSpriteGlowD.ppo", "Pixel(PS)", "None", 210u, 2u, 0x0000000088ae6537ull, 0xec67a751u, 4196u, 548u, 110u, 10291u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 3u, 92u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellatePointSpriteGlowLP.ppo", "GXFfxtessellatePointSpriteGlowLP.ppo", "Pixel(PS)", "None", 211u, 2u, 0x00000000e1c23440ull, 0xf14ed481u, 4196u, 552u, 110u, 10299u, 0x000c00c3u, 0x00000020u, 16u, 32u, 16u, 1u, 1u, 3u, 92u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateQuadLineColorOnly.ppo", "GXFfxtessellateQuadLineColorOnly.ppo", "Pixel(PS)", "None", 212u, 2u, 0x000000005c792ad2ull, 0x95228fe6u, 3860u, 392u, 73u, 7035u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 2u, 52u, 0u, "user_sgpr[4]", "PSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateQuadLineColorOnlyGlowD.ppo", "GXFfxtessellateQuadLineColorOnlyGlowD.ppo", "Pixel(PS)", "None", 213u, 2u, 0x0000000041d3ce4bull, 0x92cc78a6u, 3932u, 432u, 82u, 8038u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 2u, 92u, 0u, "user_sgpr[4]", "PSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateQuadLineColorOnlyGlowLP.ppo", "GXFfxtessellateQuadLineColorOnlyGlowLP.ppo", "Pixel(PS)", "None", 214u, 2u, 0x000000002704ab50ull, 0x18a06a77u, 3932u, 436u, 82u, 8046u, 0x000c00c3u, 0x00000018u, 16u, 32u, 12u, 0u, 0u, 2u, 92u, 0u, "user_sgpr[4]", "PSCommonBuffer", ""},
    {"gxffxshader", "GXFfxtessellateSoftBumpTexture.ppo", "GXFfxtessellateSoftBumpTexture.ppo", "Pixel(PS)", "None", 215u, 2u, 0x000000002e64f5c6ull, 0xe25c1afdu, 5908u, 1652u, 350u, 28386u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateSoftBumpTextureGlowD.ppo", "GXFfxtessellateSoftBumpTextureGlowD.ppo", "Pixel(PS)", "None", 216u, 2u, 0x00000000a1538300ull, 0x9c9a8658u, 5980u, 1692u, 359u, 29895u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateSoftBumpTextureGlowLP.ppo", "GXFfxtessellateSoftBumpTextureGlowLP.ppo", "Pixel(PS)", "None", 217u, 2u, 0x000000007d946002ull, 0x68de0f6cu, 5980u, 1696u, 359u, 29903u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 3u, 1u, 4u, 96u, 28u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture.ppo", "GXFfxtessellateSoftMultiTexture.ppo", "Pixel(PS)", "None", 218u, 2u, 0x000000005ed27cb4ull, 0xc3963563u, 6520u, 2656u, 522u, 39933u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 4u, 1u, 3u, 109u, 32u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture2Sampler.ppo", "GXFfxtessellateSoftMultiTexture2Sampler.ppo", "Pixel(PS)", "None", 219u, 2u, 0x000000003a69b784ull, 0x7cbb5ba5u, 5932u, 2136u, 425u, 32829u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture2SamplerGlowD.ppo", "GXFfxtessellateSoftMultiTexture2SamplerGlowD.ppo", "Pixel(PS)", "None", 220u, 2u, 0x000000002d0913d3ull, 0x9238ac1du, 6012u, 2180u, 435u, 33784u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateSoftMultiTexture2SamplerGlowLP.ppo", "GXFfxtessellateSoftMultiTexture2SamplerGlowLP.ppo", "Pixel(PS)", "None", 221u, 2u, 0x000000003fc5e2eeull, 0xff0cb53eu, 6020u, 2184u, 435u, 33792u, 0x000c0145u, 0x00000020u, 24u, 48u, 16u, 3u, 1u, 3u, 109u, 24u, "user_sgpr[2]->load(+64)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateSoftMultiTextureGlowD.ppo", "GXFfxtessellateSoftMultiTextureGlowD.ppo", "Pixel(PS)", "None", 222u, 2u, 0x00000000eddde3f4ull, 0xff815465u, 6600u, 2700u, 532u, 40888u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 4u, 1u, 3u, 109u, 32u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateSoftMultiTextureGlowLP.ppo", "GXFfxtessellateSoftMultiTextureGlowLP.ppo", "Pixel(PS)", "None", 223u, 2u, 0x000000000961b2fdull, 0x924828dcu, 6600u, 2704u, 532u, 40896u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 4u, 1u, 3u, 109u, 32u, "user_sgpr[2]->load(+96)", "PSCommonBuffer", "g_multiTexture0"},
    {"gxffxshader", "GXFfxtessellateSoftTexture.ppo", "GXFfxtessellateSoftTexture.ppo", "Pixel(PS)", "None", 224u, 2u, 0x0000000071c96598ull, 0xff0580afu, 4780u, 1004u, 201u, 15139u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 3u, 96u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateSoftTextureGlowD.ppo", "GXFfxtessellateSoftTextureGlowD.ppo", "Pixel(PS)", "None", 225u, 2u, 0x00000000d729c469ull, 0xf805d235u, 4860u, 1048u, 211u, 16144u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 3u, 96u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateSoftTextureGlowLP.ppo", "GXFfxtessellateSoftTextureGlowLP.ppo", "Pixel(PS)", "None", 226u, 2u, 0x0000000020739372ull, 0xc3fc8cefu, 4860u, 1052u, 211u, 16152u, 0x000c00c5u, 0x00000020u, 24u, 32u, 16u, 2u, 1u, 3u, 96u, 16u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTexture.ppo", "GXFfxtessellateTexture.ppo", "Pixel(PS)", "None", 227u, 2u, 0x000000002e86d610ull, 0xf1249e7bu, 4500u, 848u, 169u, 12775u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTextureGlowD.ppo", "GXFfxtessellateTextureGlowD.ppo", "Pixel(PS)", "None", 228u, 2u, 0x0000000044a82ffbull, 0x9bf963b8u, 4580u, 892u, 179u, 13739u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTextureGlowLP.ppo", "GXFfxtessellateTextureGlowLP.ppo", "Pixel(PS)", "None", 229u, 2u, 0x00000000ca74ff03ull, 0xebe3e9c2u, 4580u, 896u, 179u, 13747u, 0x000c00c4u, 0x00000020u, 20u, 32u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTracerBumpTexture.ppo", "GXFfxtessellateTracerBumpTexture.ppo", "Pixel(PS)", "None", 230u, 2u, 0x00000000f1cc8391ull, 0x6181b088u, 5472u, 1384u, 295u, 24306u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTracerBumpTextureGlowD.ppo", "GXFfxtessellateTracerBumpTextureGlowD.ppo", "Pixel(PS)", "None", 231u, 2u, 0x00000000d2161b96ull, 0x0b2bc73fu, 5544u, 1424u, 304u, 25815u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTracerBumpTextureGlowLP.ppo", "GXFfxtessellateTracerBumpTextureGlowLP.ppo", "Pixel(PS)", "None", 232u, 2u, 0x00000000ebe8f897ull, 0x750d69c2u, 5544u, 1428u, 304u, 25823u, 0x000c0148u, 0x00000020u, 36u, 48u, 16u, 2u, 1u, 4u, 96u, 20u, "user_sgpr[2]->load(+32)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTracerTexture.ppo", "GXFfxtessellateTracerTexture.ppo", "Pixel(PS)", "None", 233u, 2u, 0x00000000b827e780ull, 0x1611c596u, 4292u, 676u, 134u, 9741u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTracerTextureGlowD.ppo", "GXFfxtessellateTracerTextureGlowD.ppo", "Pixel(PS)", "None", 234u, 2u, 0x000000003c6a7e2dull, 0x9cf773b0u, 4364u, 716u, 143u, 10696u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtessellateTracerTextureGlowLP.ppo", "GXFfxtessellateTracerTextureGlowLP.ppo", "Pixel(PS)", "None", 235u, 2u, 0x00000000a23d5930ull, 0x02d154e1u, 4364u, 720u, 143u, 10704u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtexture.ppo", "GXFfxtexture.ppo", "Pixel(PS)", "None", 236u, 2u, 0x00000000832cdf8dull, 0x1f53a93cu, 4888u, 1208u, 247u, 19369u, 0x000c0184u, 0x00000020u, 20u, 56u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtextureGlowD.ppo", "GXFfxtextureGlowD.ppo", "Pixel(PS)", "None", 237u, 2u, 0x00000000fe0f39bfull, 0xf649d7acu, 4968u, 1252u, 257u, 20324u, 0x000c0184u, 0x00000020u, 20u, 56u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "GXFfxtextureGlowLP.ppo", "GXFfxtextureGlowLP.ppo", "Pixel(PS)", "None", 238u, 2u, 0x000000004a0810c4ull, 0x057a24ebu, 4968u, 1256u, 257u, 20332u, 0x000c0184u, 0x00000020u, 20u, 56u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_diffuseTexture"},
    {"gxffxshader", "PS_VFX_Advect.ppo", "PS_VFX_Advect.ppo", "Pixel(PS)", "None", 239u, 2u, 0x00000000e81e27d0ull, 0x0525bf14u, 1980u, 272u, 60u, 8077u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 3u, 2u, 2u, 23u, 24u, "user_sgpr[2]->load(+80)", "fcbFluidParam", "g_Texture_obstacles"},
    {"gxffxshader", "PS_VFX_AdvectBfecc.ppo", "PS_VFX_AdvectBfecc.ppo", "Pixel(PS)", "None", 240u, 2u, 0x0000000050ec411bull, 0xb3159e43u, 2268u, 496u, 95u, 11716u, 0x000c0102u, 0x00000020u, 12u, 40u, 16u, 4u, 2u, 2u, 28u, 32u, "user_sgpr[2]->load(+112)", "fcbFluidParam", "g_Texture_obstacles"},
    {"gxffxshader", "PS_VFX_AdvectVel.ppo", "PS_VFX_AdvectVel.ppo", "Pixel(PS)", "None", 241u, 2u, 0x00000000226490c4ull, 0xe55c7d3cu, 1828u, 212u, 46u, 4921u, 0x000c00c2u, 0x00000020u, 12u, 32u, 16u, 1u, 2u, 2u, 23u, 8u, "user_sgpr[2]->load(+16)", "fcbFluidParam", "g_Texture_velocity0"},
    {"gxffxshader", "PS_VFX_Confinement.ppo", "PS_VFX_Confinement.ppo", "Pixel(PS)", "None", 242u, 2u, 0x0000000073204870ull, 0x86a789a5u, 2140u, 516u, 114u, 5902u, 0x000c0088u, 0x00000020u, 36u, 24u, 16u, 1u, 1u, 2u, 22u, 4u, "user_sgpr[2]->load(+0)", "fcbFluidParam", "g_Texture_tempvector"},
    {"gxffxshader", "PS_VFX_Divergence.ppo", "PS_VFX_Divergence.ppo", "Pixel(PS)", "None", 243u, 2u, 0x0000000073092c5dull, 0x167be2f3u, 1144u, 588u, 120u, 13328u, 0x000c0103u, 0x00000020u, 16u, 40u, 16u, 3u, 1u, 1u, 0u, 16u, "", "", "g_Texture_obstacles"},
    {"gxffxshader", "PS_VFX_DrawBox.ppo", "PS_VFX_DrawBox.ppo", "Pixel(PS)", "None", 244u, 2u, 0x00000000c9abcc91ull, 0x88e501bfu, 1836u, 360u, 71u, 6376u, 0x000c0081u, 0x00000010u, 8u, 24u, 8u, 0u, 0u, 1u, 36u, 0u, "user_sgpr[4]", "fcbFluidParam", ""},
    {"gxffxshader", "PS_VFX_DrawTexture.ppo", "PS_VFX_DrawTexture.ppo", "Pixel(PS)", "None", 245u, 2u, 0x00000000d1cb2b41ull, 0xf62a361bu, 1992u, 288u, 58u, 6870u, 0x000c0101u, 0x00000020u, 8u, 40u, 16u, 4u, 1u, 2u, 4u, 28u, "user_sgpr[2]->load(+96)", "fcbFluidParam", "g_Texture_color"},
    {"gxffxshader", "PS_VFX_DrawWhite.ppo", "PS_VFX_DrawWhite.ppo", "Pixel(PS)", "None", 246u, 2u, 0x00000000d8b57737ull, 0x1847423eu, 240u, 28u, 5u, 1128u, 0x000c0000u, 0x00000004u, 4u, 8u, 2u, 0u, 0u, 0u, 0u, 0u, "", "", ""},
    {"gxffxshader", "PS_VFX_EdgeDetect.ppo", "PS_VFX_EdgeDetect.ppo", "Pixel(PS)", "None", 247u, 2u, 0x000000004875fa15ull, 0xf15ab697u, 2116u, 608u, 128u, 7914u, 0x000c0082u, 0x00000020u, 12u, 24u, 16u, 1u, 1u, 2u, 20u, 4u, "user_sgpr[2]->load(+0)", "fcbVolumeRenderParam", "g_rayDataTexSmall"},
    {"gxffxshader", "PS_VFX_Gaussian.ppo", "PS_VFX_Gaussian.ppo", "Pixel(PS)", "None", 248u, 2u, 0x00000000700eb760ull, 0x7c25cc7au, 1944u, 376u, 72u, 6158u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 20u, 4u, "user_sgpr[2]->load(+0)", "fcbFluidParam", "g_Texture_obstacles"},
    {"gxffxshader", "PS_VFX_GridAddVelocity.ppo", "PS_VFX_GridAddVelocity.ppo", "Pixel(PS)", "None", 249u, 2u, 0x000000004c5f56dfull, 0x68f8225au, 1692u, 152u, 31u, 3435u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 38u, 4u, "user_sgpr[2]->load(+0)", "fcbFluidParam", "g_Texture_localVelocity"},
    {"gxffxshader", "PS_VFX_GridSlid.ppo", "PS_VFX_GridSlid.ppo", "Pixel(PS)", "None", 250u, 2u, 0x00000000f45dd6dfull, 0xe548860cu, 1680u, 152u, 35u, 3983u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 40u, 4u, "user_sgpr[2]->load(+0)", "fcbFluidParam", "g_Texture_color"},
    {"gxffxshader", "PS_VFX_Jacobi.ppo", "PS_VFX_Jacobi.ppo", "Pixel(PS)", "None", 251u, 2u, 0x0000000058d1ddbfull, 0xe5af4790u, 984u, 432u, 88u, 6025u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 3u, 1u, 1u, 0u, 16u, "", "", "g_Texture_obstacles"},
    {"gxffxshader", "PS_VFX_Project.ppo", "PS_VFX_Project.ppo", "Pixel(PS)", "None", 252u, 2u, 0x00000000b4cd2fd4ull, 0x954fdf0du, 2404u, 620u, 116u, 10895u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 4u, 1u, 2u, 9u, 28u, "user_sgpr[2]->load(+96)", "fcbFluidParam", "g_Texture_obstacles"},
    {"gxffxshader", "PS_VFX_RayCastCopyQuad.ppo", "PS_VFX_RayCastCopyQuad.ppo", "Pixel(PS)", "None", 253u, 2u, 0x00000000c008ee63ull, 0x72ea4cc8u, 2392u, 888u, 195u, 19265u, 0x000c0204u, 0x00000020u, 20u, 72u, 16u, 5u, 2u, 2u, 24u, 40u, "user_sgpr[2]->load(+144)", "fcbVolumeRenderParam", "g_edgeTex"},
    {"gxffxshader", "PS_VFX_RayCastQuad.ppo", "PS_VFX_RayCastQuad.ppo", "Pixel(PS)", "None", 254u, 2u, 0x00000000eb99f642ull, 0x7ccca60du, 2196u, 792u, 176u, 15489u, 0x000c0144u, 0x00000020u, 20u, 48u, 16u, 3u, 2u, 2u, 20u, 24u, "user_sgpr[2]->load(+80)", "fcbVolumeRenderParam", "g_rayDataTex"},
    {"gxffxshader", "PS_VFX_RayDataBack.ppo", "PS_VFX_RayDataBack.ppo", "Pixel(PS)", "None", 255u, 2u, 0x00000000ad2ac2e7ull, 0x169cb35au, 1404u, 124u, 27u, 3229u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 4u, 4u, "user_sgpr[2]->load(+0)", "fcbVolumeRenderParam", "g_sceneDepthTex"},
    {"gxffxshader", "PS_VFX_RayDataCopyQuad.ppo", "PS_VFX_RayDataCopyQuad.ppo", "Pixel(PS)", "None", 256u, 2u, 0x0000000039b42b4eull, 0x9ba8d957u, 1324u, 84u, 18u, 2938u, 0x000c0080u, 0x00000020u, 4u, 24u, 16u, 1u, 1u, 2u, 2u, 4u, "user_sgpr[2]->load(+0)", "fcbVolumeRenderParam", "g_rayDataTex"},
    {"gxffxshader", "PS_VFX_RayDataFront.ppo", "PS_VFX_RayDataFront.ppo", "Pixel(PS)", "None", 257u, 2u, 0x0000000017113cacull, 0x72044b0bu, 1516u, 208u, 45u, 4168u, 0x000c0081u, 0x00000020u, 8u, 24u, 16u, 1u, 1u, 2u, 4u, 4u, "user_sgpr[2]->load(+0)", "fcbVolumeRenderParam", "g_sceneDepthTex"},
    {"gxffxshader", "PS_VFX_SingleTexture.ppo", "PS_VFX_SingleTexture.ppo", "Pixel(PS)", "None", 258u, 2u, 0x000000000089e4b2ull, 0xb38b58b8u, 5132u, 1408u, 286u, 22125u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 1u, 1u, 4u, 96u, 12u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureBump.ppo", "PS_VFX_SingleTextureBump.ppo", "Pixel(PS)", "None", 259u, 2u, 0x00000000a96834b5ull, 0xffe52bbcu, 7028u, 2028u, 426u, 35313u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 2u, 1u, 6u, 101u, 28u, "user_sgpr[2]->load(+80)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureBumpGlowD.ppo", "PS_VFX_SingleTextureBumpGlowD.ppo", "Pixel(PS)", "None", 260u, 2u, 0x00000000faeab073ull, 0xff759a36u, 7116u, 2076u, 437u, 36917u, 0x000c01c8u, 0x00000020u, 36u, 64u, 16u, 2u, 1u, 6u, 101u, 28u, "user_sgpr[2]->load(+80)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureBumpGlowLP.ppo", "PS_VFX_SingleTextureBumpGlowLP.ppo", "Pixel(PS)", "None", 261u, 2u, 0x000000008f9c7f8full, 0xf139c0ddu, 7124u, 2080u, 437u, 36925u, 0x000c01c8u, 0x00000020u, 36u, 64u, 16u, 2u, 1u, 6u, 101u, 28u, "user_sgpr[2]->load(+80)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureCheap.ppo", "PS_VFX_SingleTextureCheap.ppo", "Pixel(PS)", "None", 262u, 2u, 0x00000000b6e87124ull, 0x9be3a4c8u, 3700u, 768u, 157u, 8807u, 0x000c0085u, 0x00000020u, 24u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureCheapBump.ppo", "PS_VFX_SingleTextureCheapBump.ppo", "Pixel(PS)", "None", 263u, 2u, 0x00000000890b525eull, 0xb396efafu, 5664u, 1428u, 308u, 23138u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 2u, 1u, 5u, 101u, 24u, "user_sgpr[2]->load(+64)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureCheapBumpGlowD.ppo", "PS_VFX_SingleTextureCheapBumpGlowD.ppo", "Pixel(PS)", "None", 264u, 2u, 0x000000006c4adfe6ull, 0xe525462fu, 5752u, 1472u, 318u, 24733u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 2u, 1u, 5u, 101u, 24u, "user_sgpr[2]->load(+64)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureCheapBumpGlowLP.ppo", "PS_VFX_SingleTextureCheapBumpGlowLP.ppo", "Pixel(PS)", "None", 265u, 2u, 0x00000000fd51aeefull, 0x1f2a0c3bu, 5752u, 1476u, 318u, 24741u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 2u, 1u, 5u, 101u, 24u, "user_sgpr[2]->load(+64)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureCheapGlowD.ppo", "PS_VFX_SingleTextureCheapGlowD.ppo", "Pixel(PS)", "None", 266u, 2u, 0x000000002fe1ed20ull, 0x922ba4fau, 3788u, 812u, 167u, 9771u, 0x000c0085u, 0x00000020u, 24u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureCheapGlowLP.ppo", "PS_VFX_SingleTextureCheapGlowLP.ppo", "Pixel(PS)", "None", 267u, 2u, 0x00000000c575bc29ull, 0x026779b6u, 3788u, 816u, 167u, 9779u, 0x000c0085u, 0x00000020u, 24u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureGenerateSdw.ppo", "PS_VFX_SingleTextureGenerateSdw.ppo", "Pixel(PS)", "None", 268u, 2u, 0x000000005120f012ull, 0x922576c3u, 2076u, 784u, 158u, 15885u, 0x000c0144u, 0x00000020u, 20u, 48u, 16u, 1u, 1u, 3u, 56u, 8u, "user_sgpr[2]->load(+0)", "cbFog", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureGlowD.ppo", "PS_VFX_SingleTextureGlowD.ppo", "Pixel(PS)", "None", 269u, 2u, 0x0000000068518439ull, 0x81543a70u, 5220u, 1452u, 296u, 23157u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 1u, 1u, 4u, 96u, 12u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureGlowLP.ppo", "PS_VFX_SingleTextureGlowLP.ppo", "Pixel(PS)", "None", 270u, 2u, 0x00000000ab4e5342ull, 0x7b5b071au, 5220u, 1456u, 296u, 23165u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 1u, 1u, 4u, 96u, 12u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SingleTextureSdw.ppo", "PS_VFX_SingleTextureSdw.ppo", "Pixel(PS)", "None", 271u, 2u, 0x000000000089e4b2ull, 0xb38b58b8u, 5132u, 1408u, 286u, 22125u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 1u, 1u, 4u, 96u, 12u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTexture.ppo", "PS_VFX_SoftTexture.ppo", "Pixel(PS)", "None", 272u, 2u, 0x00000000528b383cull, 0x92d1512fu, 5380u, 1524u, 311u, 24537u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 2u, 2u, 4u, 96u, 24u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureBump.ppo", "PS_VFX_SoftTextureBump.ppo", "Pixel(PS)", "None", 273u, 2u, 0x0000000020cee1b5ull, 0xb3ad0b67u, 7264u, 2136u, 449u, 37670u, 0x000c0188u, 0x00000020u, 36u, 56u, 16u, 3u, 2u, 6u, 101u, 40u, "user_sgpr[2]->load(+128)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureBumpGlowD.ppo", "PS_VFX_SoftTextureBumpGlowD.ppo", "Pixel(PS)", "None", 274u, 2u, 0x0000000008ecc8a8ull, 0x7bcbc64cu, 7340u, 2180u, 459u, 39274u, 0x000c01c8u, 0x00000020u, 36u, 64u, 16u, 3u, 2u, 6u, 101u, 40u, "user_sgpr[2]->load(+128)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureBumpGlowLP.ppo", "PS_VFX_SoftTextureBumpGlowLP.ppo", "Pixel(PS)", "None", 275u, 2u, 0x00000000e84797c4ull, 0x18ae45e4u, 7348u, 2184u, 459u, 39282u, 0x000c01c8u, 0x00000020u, 36u, 64u, 16u, 3u, 2u, 6u, 101u, 40u, "user_sgpr[2]->load(+128)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureCheap.ppo", "PS_VFX_SoftTextureCheap.ppo", "Pixel(PS)", "None", 276u, 2u, 0x00000000bf18ff46ull, 0xe225e775u, 3944u, 880u, 182u, 11218u, 0x000c00c6u, 0x00000020u, 28u, 32u, 16u, 2u, 2u, 3u, 96u, 20u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureCheapBump.ppo", "PS_VFX_SoftTextureCheapBump.ppo", "Pixel(PS)", "None", 277u, 2u, 0x0000000035b8197full, 0xff1fc454u, 5888u, 1532u, 330u, 25311u, 0x000c0147u, 0x00000020u, 32u, 48u, 16u, 3u, 2u, 5u, 101u, 36u, "user_sgpr[2]->load(+112)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureCheapBumpGlowD.ppo", "PS_VFX_SoftTextureCheapBumpGlowD.ppo", "Pixel(PS)", "None", 278u, 2u, 0x000000007671e55aull, 0xff727d83u, 5972u, 1576u, 340u, 26906u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 3u, 2u, 5u, 101u, 36u, "user_sgpr[2]->load(+112)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureCheapBumpGlowLP.ppo", "PS_VFX_SoftTextureCheapBumpGlowLP.ppo", "Pixel(PS)", "None", 279u, 2u, 0x00000000346eb463ull, 0xe5b503aau, 5972u, 1580u, 340u, 26914u, 0x000c0187u, 0x00000020u, 32u, 56u, 16u, 3u, 2u, 5u, 101u, 36u, "user_sgpr[2]->load(+112)", "gcbSceneParam", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureCheapGlowD.ppo", "PS_VFX_SoftTextureCheapGlowD.ppo", "Pixel(PS)", "None", 280u, 2u, 0x000000003d0ec19full, 0x05cbabdfu, 4020u, 924u, 192u, 12182u, 0x000c00c6u, 0x00000020u, 28u, 32u, 16u, 2u, 2u, 3u, 96u, 20u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureCheapGlowLP.ppo", "PS_VFX_SoftTextureCheapGlowLP.ppo", "Pixel(PS)", "None", 281u, 2u, 0x00000000cd5b96b8ull, 0x7c2cfe16u, 4028u, 928u, 192u, 12190u, 0x000c00c6u, 0x00000020u, 28u, 32u, 16u, 2u, 2u, 3u, 96u, 20u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureGenerateSdw.ppo", "PS_VFX_SoftTextureGenerateSdw.ppo", "Pixel(PS)", "None", 282u, 2u, 0x000000009dbbece7ull, 0x7cbb2264u, 5292u, 1436u, 292u, 22572u, 0x000c0146u, 0x00000020u, 28u, 48u, 16u, 2u, 2u, 4u, 96u, 24u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureGlowD.ppo", "PS_VFX_SoftTextureGlowD.ppo", "Pixel(PS)", "None", 283u, 2u, 0x00000000c5ba1e09ull, 0xe2315974u, 5456u, 1568u, 321u, 25569u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 2u, 2u, 4u, 96u, 24u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureGlowLP.ppo", "PS_VFX_SoftTextureGlowLP.ppo", "Pixel(PS)", "None", 284u, 2u, 0x00000000367eed24ull, 0x0b4f3b21u, 5464u, 1572u, 321u, 25577u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 2u, 2u, 4u, 96u, 24u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SoftTextureSdw.ppo", "PS_VFX_SoftTextureSdw.ppo", "Pixel(PS)", "None", 285u, 2u, 0x00000000528b383cull, 0x92d1512fu, 5380u, 1524u, 311u, 24537u, 0x000c01c6u, 0x00000020u, 28u, 64u, 16u, 2u, 2u, 4u, 96u, 24u, "user_sgpr[2]->load(+48)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSingleTexture.ppo", "PS_VFX_SparkSingleTexture.ppo", "Pixel(PS)", "None", 286u, 2u, 0x000000002db65555ull, 0x1f2a8f8bu, 3384u, 488u, 101u, 6740u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSingleTextureCheap.ppo", "PS_VFX_SparkSingleTextureCheap.ppo", "Pixel(PS)", "None", 287u, 2u, 0x000000002db65555ull, 0x1f2a8f8bu, 3384u, 488u, 101u, 6740u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSingleTextureCheapGlowD.ppo", "PS_VFX_SparkSingleTextureCheapGlowD.ppo", "Pixel(PS)", "None", 288u, 2u, 0x00000000ae2cd121ull, 0xff9c0a5au, 3472u, 532u, 111u, 7704u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSingleTextureCheapGlowLP.ppo", "PS_VFX_SparkSingleTextureCheapGlowLP.ppo", "Pixel(PS)", "None", 289u, 2u, 0x000000005334a02aull, 0x619b6a10u, 3472u, 536u, 111u, 7712u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSingleTextureGlowD.ppo", "PS_VFX_SparkSingleTextureGlowD.ppo", "Pixel(PS)", "None", 290u, 2u, 0x00000000ae2cd121ull, 0xff9c0a5au, 3472u, 532u, 111u, 7704u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSingleTextureGlowLP.ppo", "PS_VFX_SparkSingleTextureGlowLP.ppo", "Pixel(PS)", "None", 291u, 2u, 0x000000005334a02aull, 0x619b6a10u, 3472u, 536u, 111u, 7712u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSoftTexture.ppo", "PS_VFX_SparkSoftTexture.ppo", "Pixel(PS)", "None", 292u, 2u, 0x00000000e216e102ull, 0xff0c5b24u, 3384u, 488u, 101u, 6740u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSoftTextureCheap.ppo", "PS_VFX_SparkSoftTextureCheap.ppo", "Pixel(PS)", "None", 293u, 2u, 0x00000000e216e102ull, 0xff0c5b24u, 3384u, 488u, 101u, 6740u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSoftTextureCheapGlowD.ppo", "PS_VFX_SparkSoftTextureCheapGlowD.ppo", "Pixel(PS)", "None", 294u, 2u, 0x00000000f669e876ull, 0x05898e11u, 3468u, 532u, 111u, 7704u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSoftTextureCheapGlowLP.ppo", "PS_VFX_SparkSoftTextureCheapGlowLP.ppo", "Pixel(PS)", "None", 295u, 2u, 0x00000000fd5fb77full, 0x180b2b44u, 3468u, 536u, 111u, 7712u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSoftTextureGlowD.ppo", "PS_VFX_SparkSoftTextureGlowD.ppo", "Pixel(PS)", "None", 296u, 2u, 0x00000000f669e876ull, 0x05898e11u, 3468u, 532u, 111u, 7704u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_SparkSoftTextureGlowLP.ppo", "PS_VFX_SparkSoftTextureGlowLP.ppo", "Pixel(PS)", "None", 297u, 2u, 0x00000000fd5fb77full, 0x180b2b44u, 3468u, 536u, 111u, 7712u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 3u, 96u, 8u, "user_sgpr[2]->load(+0)", "PSCommonBuffer", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_Vorticity.ppo", "PS_VFX_Vorticity.ppo", "Pixel(PS)", "None", 298u, 2u, 0x00000000fda7b3f5ull, 0xec9dbc21u, 712u, 276u, 58u, 2820u, 0x000c0084u, 0x00000020u, 20u, 24u, 16u, 1u, 1u, 0u, 0u, 0u, "", "", "g_Texture_velocity1"},
    {"gxffxshader", "PS_VFX_WanderingVision.ppo", "PS_VFX_WanderingVision.ppo", "Pixel(PS)", "None", 299u, 2u, 0x0000000025b9324dull, 0x889ca4b8u, 2240u, 916u, 193u, 16084u, 0x000c0104u, 0x00000020u, 20u, 40u, 16u, 1u, 1u, 3u, 56u, 8u, "user_sgpr[2]->load(+0)", "cbFog", "g_texAlbedo0"},
    {"gxffxshader", "PS_VFX_WanderingVisionSoft.ppo", "PS_VFX_WanderingVisionSoft.ppo", "Pixel(PS)", "None", 300u, 2u, 0x00000000ded42897ull, 0x021035e1u, 2492u, 1040u, 217u, 18916u, 0x000c01c7u, 0x00000020u, 32u, 64u, 16u, 2u, 2u, 3u, 56u, 20u, "user_sgpr[2]->load(+48)", "cbFog", "g_texAlbedo0"},
};

// ============================================================================
// 5. Shader Registry Query & CPU<->GPU Binding Layout Verification API
// ============================================================================

extern "C" {

std::uint32_t GXShaderLibrary_GetTotalShaderCount() noexcept {
    return static_cast<std::uint32_t>(sizeof(kGXShaderManifestTable) / sizeof(kGXShaderManifestTable[0]));
}

std::uint32_t GXShaderLibrary_GetBundleShaderCount(const char* bundle_name) noexcept {
    if (!bundle_name) return 0;
    std::uint32_t n = 0;
    for (const GXShaderManifestEntry& e : kGXShaderManifestTable) {
        if (std::strcmp(e.bundle, bundle_name) == 0) ++n;
    }
    return n;
}

const GXShaderManifestEntry* GXShaderLibrary_GetEntryByIndex(std::uint32_t index) noexcept {
    if (index >= GXShaderLibrary_GetTotalShaderCount()) return nullptr;
    return &kGXShaderManifestTable[index];
}

const GXShaderManifestEntry* GXShaderLibrary_FindByName(const char* bundle_name,
                                                        const char* shader_name) noexcept {
    if (!shader_name) return nullptr;
    for (const GXShaderManifestEntry& e : kGXShaderManifestTable) {
        if (bundle_name && std::strcmp(e.bundle, bundle_name) != 0) continue;
        if (std::strcmp(e.name, shader_name) == 0 || std::strcmp(e.safe_name, shader_name) == 0) {
            return &e;
        }
    }
    return nullptr;
}

const GXShaderManifestEntry* GXShaderLibrary_FindByHash(std::uint64_t shader_hash,
                                                        std::uint32_t crc32) noexcept {
    const std::uint64_t hash_lo = shader_hash & 0xffffffffULL;
    for (const GXShaderManifestEntry& e : kGXShaderManifestTable) {
        if ((e.shader_hash == shader_hash || e.shader_hash == hash_lo) &&
            (crc32 == 0 || e.crc32 == crc32)) {
            return &e;
        }
    }
    return nullptr;
}

bool GXShaderLibrary_VerifyCpuBindingLayout(const char* bundle_name,
                                            const char* shader_name,
                                            std::uint32_t cpu_cb_bytes,
                                            std::uint32_t expected_images,
                                            std::uint32_t expected_samplers) noexcept {
    const GXShaderManifestEntry* e = GXShaderLibrary_FindByName(bundle_name, shader_name);
    if (!e) return false;
    const std::uint32_t gpu_cb_bytes = e->max_cb_dwords * 4u;
    if (cpu_cb_bytes < gpu_cb_bytes) return false;
    if (e->image_count != expected_images) return false;
    if (e->sampler_count != expected_samplers) return false;
    return true;
}

}  // extern "C"
