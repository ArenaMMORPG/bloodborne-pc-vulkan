// SPDX-License-Identifier: GPL-2.0-or-later
// Block 8, Step 8.2: Native Vulkan 1.3 GLSL replacement for ps 0x000000006b42fc89
// (3-Point-Light + VSM Cube-Array Shadow Deferred PBR Lighting pass).
#version 460
#extension GL_EXT_demote_to_helper_invocation : require

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;

layout(location = 0) out vec4 frag_color0;
layout(location = 1) out vec4 frag_color1;

layout(push_constant) uniform AuxData {
    float xoffset;
    float yoffset;
    float xscale;
    float yscale;
    uvec4 ud_regs0;
    uvec4 ud_regs1;
    uvec4 ud_regs2;
    uvec4 ud_regs3;
    uvec4 buf_offsets0;
    uvec4 buf_offsets1;
    uvec2 buf_offsets2;
    uint motion_param;
    uint scene_size;
} push_data;

layout(set = 0, binding = 0, std430) readonly buffer SSBO1 {
    uint data[];
} ssbo_1;

layout(set = 0, binding = 1, std430) readonly buffer SSBO2 {
    uint data[];
} ssbo_2;

layout(set = 0, binding = 2) uniform texture2D fs_img0;
layout(set = 0, binding = 3) uniform texture2D fs_img1;
layout(set = 0, binding = 4) uniform texture2D fs_img2;
layout(set = 0, binding = 5) uniform texture2D fs_img3;
layout(set = 0, binding = 6) uniform texture2D fs_img4;
layout(set = 0, binding = 7) uniform texture2DArray fs_img5;
layout(set = 0, binding = 8) uniform texture2DArray fs_img6;
layout(set = 0, binding = 9) uniform texture2DArray fs_img7;
layout(set = 0, binding = 10) uniform sampler fs_samp0;
layout(set = 0, binding = 11) uniform sampler fs_samp1;

float safe_inversesqrt(float x) {
    const float clamped = min(max(x, 1.17549435e-38), 3.40282347e+38);
    return (x > 0.0 && x <= 3.40282347e+38) ? inversesqrt(clamped) : 0.0;
}

float s1f(uint idx) {
    return uintBitsToFloat(ssbo_1.data[idx]);
}

float s2f(uint idx) {
    return uintBitsToFloat(ssbo_2.data[idx]);
}

vec3 cube_array_uv(vec3 d) {
    const vec3 nd = -d;
    const vec3 ad = abs(nd);
    const bool z_max = (ad.z >= ad.y) && (ad.z >= ad.x);
    const bool y_max = (ad.y >= ad.x);

    const float sel_y_neg = (nd.y < 0.0) ? d.z : nd.z;
    const float v_num = z_max ? d.y : (y_max ? sel_y_neg : d.y);

    const float sel_x_neg = (nd.x < 0.0) ? nd.z : d.z;
    const float sel_z_neg = (nd.z < 0.0) ? d.x : nd.x;
    const float u_num = z_max ? sel_z_neg : (y_max ? nd.x : sel_x_neg);

    const vec3 m2 = d * -2.0;
    const float denom = abs(z_max ? m2.z : (y_max ? m2.y : m2.x));
    const float inv_denom = 1.0 / denom;

    const float face_x = (nd.x < 0.0) ? 1.0 : 0.0;
    const float face_y = (nd.y < 0.0) ? 3.0 : 2.0;
    const float face_z = (nd.z < 0.0) ? 5.0 : 4.0;
    const float face = z_max ? face_z : (y_max ? face_y : face_x);

    const float u = (u_num * inv_denom) + 0.5;
    const float v = (v_num * inv_denom) + 0.5;
    const float layer = fma(floor(face * 0.125), -2.0, face);
    return vec3(u, v, layer);
}

void main() {
    const uint bo0 = push_data.buf_offsets0.x;
    const uint buf0_dword_off = bitfieldExtract(bo0, 0, 8) >> 2u;
    const uint buf1_dword_off = bitfieldExtract(bo0, 8, 8) >> 2u;

    const vec2 uv = fs_in_attr1_p.xy;
    const vec2 ndc = fs_in_attr0_p.xy;

    const float depth = texture(sampler2D(fs_img1, fs_samp0), uv).x;

    const vec4 col0 = vec4(s1f(0u + buf0_dword_off), s1f(4u + buf0_dword_off), s1f(8u + buf0_dword_off), s1f(12u + buf0_dword_off));
    const vec4 col1 = vec4(s1f(1u + buf0_dword_off), s1f(5u + buf0_dword_off), s1f(9u + buf0_dword_off), s1f(13u + buf0_dword_off));
    const vec4 col2 = vec4(s1f(2u + buf0_dword_off), s1f(6u + buf0_dword_off), s1f(10u + buf0_dword_off), s1f(14u + buf0_dword_off));
    const vec4 col3 = vec4(s1f(3u + buf0_dword_off), s1f(7u + buf0_dword_off), s1f(11u + buf0_dword_off), s1f(15u + buf0_dword_off));

    const vec4 clip_p = col0 * ndc.x + (col1 * ndc.y + (col2 * depth + col3));
    const float inv_w = 1.0 / clip_p.w;
    const vec3 pos = clip_p.xyz * inv_w;
    const vec3 neg_pos = (-clip_p.xyz) * inv_w;

    const vec3 l0_vec = neg_pos + vec3(s2f(0u + buf1_dword_off), s2f(1u + buf1_dword_off), s2f(2u + buf1_dword_off));
    const float l0_dist2 = dot(l0_vec, l0_vec);
    const float l0_norm2 = s2f(3u + buf1_dword_off) * l0_dist2;

    const vec3 l1_vec = neg_pos + vec3(s2f(24u + buf1_dword_off), s2f(25u + buf1_dword_off), s2f(26u + buf1_dword_off));
    const float l1_dist2 = dot(l1_vec, l1_vec);
    const float l1_norm2 = s2f(27u + buf1_dword_off) * l1_dist2;

    const vec3 l2_vec = neg_pos + vec3(s2f(48u + buf1_dword_off), s2f(49u + buf1_dword_off), s2f(50u + buf1_dword_off));
    const float l2_dist2 = dot(l2_vec, l2_vec);
    const float l2_norm2 = s2f(51u + buf1_dword_off) * l2_dist2;

    if (l0_norm2 >= 1.0 && l1_norm2 >= 1.0 && l2_norm2 >= 1.0) {
        frag_color0 = vec4(0.0);
        frag_color1 = vec4(0.0);
        demote;
        return;
    }

    const vec3 nrm_tex = texture(sampler2D(fs_img0, fs_samp0), uv).xyz;

    const vec3 n_raw = fma(vec3(2.0), nrm_tex, vec3(-1.0));
    const float n_len2 = dot(n_raw, n_raw);
    const vec3 n_vs = n_raw * safe_inversesqrt(n_len2);

    const vec3 N = vec3(
        s1f(16u + buf0_dword_off) * n_vs.x + (s1f(17u + buf0_dword_off) * n_vs.y + s1f(18u + buf0_dword_off) * n_vs.z),
        s1f(20u + buf0_dword_off) * n_vs.x + (s1f(21u + buf0_dword_off) * n_vs.y + s1f(22u + buf0_dword_off) * n_vs.z),
        s1f(24u + buf0_dword_off) * n_vs.x + (s1f(25u + buf0_dword_off) * n_vs.y + s1f(26u + buf0_dword_off) * n_vs.z)
    );

    const float pos_len2 = dot(pos, pos);
    const vec3 V = pos * safe_inversesqrt(pos_len2);

    const float ndotv_2 = dot(N, V) * 2.0;
    const vec3 R = V - N * ndotv_2;

    const float rim_dot = 1.0 + dot(N, -V);
    const float rim_clamp = max(0.0, rim_dot);
    const float rim_term = 0.25 * (rim_clamp * rim_clamp);

    const float ao_mask = texture(sampler2D(fs_img2, fs_samp1), uv).x;
    const vec3 spec_col = texture(sampler2D(fs_img3, fs_samp0), uv).xyz;
    const vec2 gbuf4 = texture(sampler2D(fs_img4, fs_samp0), uv).xy;

    const vec3 spec_sq = spec_col * spec_col;
    const float trans_k = 10.0 * gbuf4.y;
    const float gloss_exp = exp2(13.0 * gbuf4.x);
    const bool no_trans = (trans_k == 0.0);
    const float trans_denom = fma(2.0, trans_k, 1.0);
    const float gloss_scaled = gloss_exp * 0.0100021362;

    // --- Light 0 ---
    const vec3 L0 = l0_vec * safe_inversesqrt(l0_dist2);
    const float ndotl0 = dot(N, L0);
    const float att0_t = 1.0 - sqrt(clamp(l0_norm2, 0.0, 1.0));
    const float att0 = att0_t * att0_t;

    const vec3 sh0_vec = neg_pos + vec3(s2f(20u + buf1_dword_off), s2f(21u + buf1_dword_off), s2f(22u + buf1_dword_off));
    const float sh0_max = max(abs(sh0_vec.z), max(abs(sh0_vec.x), abs(sh0_vec.y)));
    const float sh0_scale = (-s2f(17u + buf1_dword_off)) * (1.0 / (s2f(16u + buf1_dword_off) - 1.0));
    const float sh0_dist = sh0_max * (1.0 / sh0_scale);

    const vec2 vsm0 = texture(sampler2DArray(fs_img5, fs_samp1), cube_array_uv(sh0_vec)).xy;
    const float vsm0_cmp = (vsm0.x >= sh0_dist) ? 1.0 : 0.0;
    const float vsm0_var = max(0.00100000005 / sh0_scale, (-vsm0.x) * vsm0.x + vsm0.y);
    const float vsm0_diff = sh0_dist - vsm0.x;
    const float vsm0_p = max(vsm0_cmp, vsm0_var / (vsm0_diff * vsm0_diff + vsm0_var));
    const float sh0 = s2f(19u + buf1_dword_off) * (vsm0_p - 1.0) + 1.0;

    const float sp0_k = s2f(7u + buf1_dword_off);
    const float sp0_pow = max(1.0, (sp0_k * gloss_exp) / (gloss_scaled + sp0_k));
    const float rdotl0 = clamp(dot(R, L0), 0.0, 1.0);
    const float sp0_val = exp2((0.25 * sp0_pow) * log2(rdotl0)) * (0.125 * sp0_pow + 0.25);

    const vec3 l0_diff_col = vec3(s2f(4u + buf1_dword_off), s2f(5u + buf1_dword_off), s2f(6u + buf1_dword_off));
    vec3 diff0;
    if (no_trans) {
        const float w0 = att0 * clamp(ndotl0, 0.0, 1.0) * sh0;
        diff0 = l0_diff_col * w0;
    } else {
        const float wrap0 = max(0.0, trans_k + ndotl0) / trans_denom;
        const float vdotl0 = clamp(dot(V, L0), 0.0, 1.0);
        const float tr0_spec = exp2((s2f(13u + buf1_dword_off) * 32.0) * log2(vdotl0)) * (s2f(12u + buf1_dword_off) * rim_term);
        const float w0 = (att0 * sh0) * wrap0 + trans_k * (att0 * tr0_spec);
        diff0 = l0_diff_col * w0;
    }
    const vec3 spec0 = vec3(s2f(8u + buf1_dword_off), s2f(9u + buf1_dword_off), s2f(10u + buf1_dword_off)) * (sp0_val * att0 * sh0);

    // --- Light 1 ---
    const vec3 sh1_vec = neg_pos + vec3(s2f(44u + buf1_dword_off), s2f(45u + buf1_dword_off), s2f(46u + buf1_dword_off));
    const vec2 vsm1 = texture(sampler2DArray(fs_img6, fs_samp1), cube_array_uv(sh1_vec)).xy;
    const float sh1_max = max(abs(sh1_vec.z), max(abs(sh1_vec.x), abs(sh1_vec.y)));
    const float sh1_scale = (-s2f(41u + buf1_dword_off)) * (1.0 / (s2f(40u + buf1_dword_off) - 1.0));
    const float sh1_dist = sh1_max * (1.0 / sh1_scale);

    const vec3 L1 = l1_vec * safe_inversesqrt(l1_dist2);
    const float ndotl1 = dot(N, L1);
    const float att1_t = 1.0 - sqrt(clamp(l1_norm2, 0.0, 1.0));
    const float att1 = att1_t * att1_t;

    const float vsm1_cmp = (vsm1.x >= sh1_dist) ? 1.0 : 0.0;
    const float vsm1_var = max(0.00100000005 / sh1_scale, (-vsm1.x) * vsm1.x + vsm1.y);
    const float vsm1_diff = sh1_dist - vsm1.x;
    const float vsm1_p = max(vsm1_cmp, vsm1_var / (vsm1_diff * vsm1_diff + vsm1_var));
    const float sh1 = s2f(43u + buf1_dword_off) * (vsm1_p - 1.0) + 1.0;

    const float sp1_k = s2f(31u + buf1_dword_off);
    const float sp1_pow = max(1.0, (sp1_k * gloss_exp) / (gloss_scaled + sp1_k));
    const float rdotl1 = clamp(dot(R, L1), 0.0, 1.0);
    const float sp1_val = exp2((0.25 * sp1_pow) * log2(rdotl1)) * (0.125 * sp1_pow + 0.25);

    const vec3 l1_diff_col = vec3(s2f(28u + buf1_dword_off), s2f(29u + buf1_dword_off), s2f(30u + buf1_dword_off));
    vec3 diff1;
    if (no_trans) {
        const float w1 = att1 * clamp(ndotl1, 0.0, 1.0) * sh1;
        diff1 = l1_diff_col * w1;
    } else {
        const float wrap1 = max(0.0, trans_k + ndotl1) / trans_denom;
        const float vdotl1 = clamp(dot(V, L1), 0.0, 1.0);
        const float tr1_spec = exp2((s2f(37u + buf1_dword_off) * 32.0) * log2(vdotl1)) * (s2f(36u + buf1_dword_off) * rim_term);
        const float w1 = (att1 * sh1) * wrap1 + trans_k * (att1 * tr1_spec);
        diff1 = l1_diff_col * w1;
    }
    const vec3 spec1 = vec3(s2f(32u + buf1_dword_off), s2f(33u + buf1_dword_off), s2f(34u + buf1_dword_off)) * (sp1_val * att1 * sh1);

    // --- Light 2 ---
    const vec3 sh2_vec = neg_pos + vec3(s2f(68u + buf1_dword_off), s2f(69u + buf1_dword_off), s2f(70u + buf1_dword_off));
    const vec2 vsm2 = texture(sampler2DArray(fs_img7, fs_samp1), cube_array_uv(sh2_vec)).xy;
    const float sh2_max = max(abs(sh2_vec.z), max(abs(sh2_vec.x), abs(sh2_vec.y)));
    const float sh2_scale = (-s2f(65u + buf1_dword_off)) * (1.0 / (s2f(64u + buf1_dword_off) - 1.0));
    const float sh2_dist = sh2_max * (1.0 / sh2_scale);

    const vec3 L2 = l2_vec * safe_inversesqrt(l2_dist2);
    const float ndotl2 = dot(N, L2);
    const float att2_t = 1.0 - sqrt(clamp(l2_norm2, 0.0, 1.0));
    const float att2 = att2_t * att2_t;

    const float vsm2_cmp = (vsm2.x >= sh2_dist) ? 1.0 : 0.0;
    const float vsm2_var = max(0.00100000005 / sh2_scale, (-vsm2.x) * vsm2.x + vsm2.y);
    const float vsm2_diff = sh2_dist - vsm2.x;
    const float vsm2_p = max(vsm2_cmp, vsm2_var / (vsm2_diff * vsm2_diff + vsm2_var));
    const float sh2 = s2f(67u + buf1_dword_off) * (vsm2_p - 1.0) + 1.0;

    const float sp2_k = s2f(55u + buf1_dword_off);
    const float sp2_pow = max(1.0, (sp2_k * gloss_exp) / (gloss_scaled + sp2_k));
    const float rdotl2 = clamp(dot(R, L2), 0.0, 1.0);
    const float sp2_val = exp2((0.25 * sp2_pow) * log2(rdotl2)) * (0.125 * sp2_pow + 0.25);

    const vec3 l2_diff_col = vec3(s2f(52u + buf1_dword_off), s2f(53u + buf1_dword_off), s2f(54u + buf1_dword_off));
    vec3 diff2;
    if (no_trans) {
        const float w2 = att2 * clamp(ndotl2, 0.0, 1.0) * sh2;
        diff2 = l2_diff_col * w2;
    } else {
        const float wrap2 = max(0.0, gbuf4.y * 10.0 + ndotl2) / trans_denom;
        const float vdotl2 = clamp(dot(V, L2), 0.0, 1.0);
        const float tr2_spec = exp2((s2f(61u + buf1_dword_off) * 32.0) * log2(vdotl2)) * (s2f(60u + buf1_dword_off) * rim_term);
        const float w2 = (att2 * sh2) * wrap2 + trans_k * (att2 * tr2_spec);
        diff2 = l2_diff_col * w2;
    }
    const vec3 spec2 = vec3(s2f(56u + buf1_dword_off), s2f(57u + buf1_dword_off), s2f(58u + buf1_dword_off)) * (sp2_val * att2 * sh2);

    const vec3 total_diff = (diff0 + diff1) + diff2;
    const vec3 total_spec = (spec0 + spec1) + spec2;

    const vec3 diff_energy = total_diff - (spec_sq * total_diff) * 0.5;
    frag_color1 = vec4(spec_sq * (ao_mask * total_spec), 1.0);
    frag_color0 = vec4(ao_mask * diff_energy, 1.0);
}
