// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4 (Sub-Batch 2): Native Vulkan 1.3 GLSL replacement for ps 0x000000005e224cb6
// (2560x1440 R16G16B16A16Sfloat soft-particle flipbook VFX + radial/height fog, blend_t = fs_in_attr4_p.w).
#version 450

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;
layout(location = 2) in vec4 fs_in_attr2_p;
layout(location = 3) in vec4 fs_in_attr3_p;
layout(location = 4) in vec4 fs_in_attr4_p;
layout(location = 5) in vec4 fs_in_attr5_p;

layout(location = 0) out vec4 frag_color0;

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

layout(set = 0, binding = 0) readonly buffer SSBO1 {
    uint data[];
} ssbo_1;

layout(set = 0, binding = 1) readonly buffer SSBO2 {
    uint data[];
} ssbo_2;

layout(set = 0, binding = 2) uniform texture2D fs_img0;
layout(set = 0, binding = 3) uniform texture2D fs_img1;
layout(set = 0, binding = 4) uniform sampler fs_samp0;

float s1f(uint idx) {
    return uintBitsToFloat(ssbo_1.data[idx]);
}

float s2f(uint idx) {
    return uintBitsToFloat(ssbo_2.data[idx]);
}

void main() {
    const uint b0 = push_data.buf_offsets0.x;
    const uint d0 = (b0 & 0xFFu) >> 2u;
    const uint d1 = ((b0 >> 8u) & 0xFFu) >> 2u;

    const vec4 s_hi = texture(sampler2D(fs_img0, fs_samp0), fs_in_attr5_p.xy);
    const vec4 s_lo = texture(sampler2D(fs_img0, fs_samp0), fs_in_attr4_p.xy);

    const float inv_w = 1.0 / fs_in_attr2_p.w;
    const vec2 screen_uv = vec2(fma(0.5, fs_in_attr2_p.x * inv_w, 0.5),
                                fma(-0.5, fs_in_attr2_p.y * inv_w, 0.5));

    const vec3 hi_sq = s_hi.rgb * s_hi.rgb;
    const vec3 lo_sq = s_lo.rgb * s_lo.rgb;
    const float blend_t = fs_in_attr4_p.w;

    const vec3 col = fma(vec3(blend_t), hi_sq - lo_sq, lo_sq);
    const float alpha = fma(blend_t, s_hi.a - s_lo.a, s_lo.a);

    const vec4 tint = vec4(s1f(d0 + 36u), s1f(d0 + 37u), s1f(d0 + 38u), s1f(d0 + 39u)) * fs_in_attr0_p;
    vec3 rgb_out;
    const float a_out = alpha * tint.w;
    if (ssbo_1.data[d0 + 95u] == 2u) {
        const vec3 ca = col * alpha;
        const vec3 lo_br = (tint.rgb * ca) * 2.0;
        const vec3 inv_t = vec3(1.0) - tint.rgb;
        const vec3 hi_br = vec3(1.0) - 2.0 * (inv_t - alpha * (col * inv_t));
        rgb_out = vec3((ca.x < 0.5) ? lo_br.x : hi_br.x,
                       (ca.y < 0.5) ? lo_br.y : hi_br.y,
                       (0.5 > ca.z) ? lo_br.z : hi_br.z);
    } else {
        rgb_out = tint.rgb * col;
    }

    const float scene_d = texture(sampler2D(fs_img1, fs_samp0), screen_uv).x;
    const float p16 = s1f(d0 + 16u);
    const float p17 = s1f(d0 + 17u);
    const float p18 = s1f(d0 + 18u);
    const float scene_z = (-p16) / (p17 * scene_d - p18);
    const float soft_fade = max(0.0, min(fs_in_attr1_p.w, scene_z - fs_in_attr1_p.z));
    const float final_a = a_out * soft_fade;

    if (final_a > 0.0) {
        const vec3 k_rgb = vec3(s1f(d0 + 43u), s1f(d0 + 47u), s1f(d0 + 51u));
        const vec3 scaled_rgb = k_rgb * rgb_out;
        const float luma = fma(scaled_rgb.z, 0.114480004,
                           fma(scaled_rgb.x, 0.298909992, 0.586610019 * scaled_rgb.y));
        const float luma_m1 = luma - 1.0;

        const float dist_xz = sqrt(fs_in_attr3_p.x * fs_in_attr3_p.x + fs_in_attr3_p.z * fs_in_attr3_p.z);
        const float b8 = s2f(d1 + 8u);
        const float b9 = s2f(d1 + 9u);
        const float b10 = s2f(d1 + 10u);
        const float fog_rad = clamp(b8 - b8 * exp2(-(b10 * (dist_xz - b9))), 0.0, 1.0);

        const vec3 fcol1 = vec3(s2f(d1 + 4u), s2f(d1 + 5u), s2f(d1 + 6u));
        const float fscale1 = fma(s2f(d1 + 7u), luma_m1, 1.0);
        const vec3 mix1 = fma(vec3(fog_rad), fcol1 * fscale1 - scaled_rgb, scaled_rgb);

        const float b12 = s2f(d1 + 12u);
        const float b13 = s2f(d1 + 13u);
        const float b14 = s2f(d1 + 14u);
        const float b15 = s2f(d1 + 15u);
        const float h_factor = clamp(b14 * ((b15 - fs_in_attr3_p.y) - b13), 0.0, 1.0);
        const float fog_h = clamp(b12 * h_factor, 0.0, 1.0);

        const vec3 fcol2 = vec3(s2f(d1 + 16u), s2f(d1 + 17u), s2f(d1 + 18u));
        const float fscale2 = fma(s2f(d1 + 19u), luma_m1, 1.0);
        rgb_out = fma(vec3(fog_h), fcol2 * fscale2 - mix1, mix1);
    }

    frag_color0 = max(vec4(0.0), vec4(rgb_out, final_a));
}
