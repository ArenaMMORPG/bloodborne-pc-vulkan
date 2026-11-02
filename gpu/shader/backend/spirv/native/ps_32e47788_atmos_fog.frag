// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4 (Sub-Batch 2): Native Vulkan 1.3 GLSL replacement for ps 0x0000000032e47788
// (2560x1440 R16G16B16A16Sfloat fullscreen atmospheric radial + height fog composite).
#version 450

layout(location = 0) in vec2 fs_in_ndc;
layout(location = 1) in vec2 fs_in_uv;
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
    float data[];
} ssbo_1;

layout(set = 0, binding = 1) readonly buffer SSBO2 {
    uint data[];
} ssbo_2;

layout(set = 0, binding = 2) uniform texture2D fs_img0;
layout(set = 0, binding = 3) uniform texture2D fs_img1;
layout(set = 0, binding = 4) uniform texture2D fs_img2;
layout(set = 0, binding = 5) uniform texture2D fs_img3;
layout(set = 0, binding = 6) uniform texture2D fs_img4;
layout(set = 0, binding = 7) uniform texture2D fs_img5;
layout(set = 0, binding = 8) uniform sampler fs_samp0;

float s2f(uint idx) {
    return uintBitsToFloat(ssbo_2.data[idx]);
}

void main() {
    const uint b0 = push_data.buf_offsets0.x;
    const uint d0 = (b0 & 0xFFu) >> 2u;
    const uint d1 = ((b0 >> 8u) & 0xFFu) >> 2u;

    const vec2 uv = fs_in_uv;
    const float s0_a = texture(sampler2D(fs_img0, fs_samp0), uv).w;
    const float depth = texture(sampler2D(fs_img1, fs_samp0), uv).x;
    const float out_a = texture(sampler2D(fs_img2, fs_samp0), uv).w;
    const vec3 col3 = texture(sampler2D(fs_img3, fs_samp0), uv).rgb;
    const vec3 col4 = texture(sampler2D(fs_img4, fs_samp0), uv).rgb;
    const vec3 col5 = texture(sampler2D(fs_img5, fs_samp0), uv).rgb;

    const uint idx6 = uint(int(255.0 * s0_a)) * 6u;
    const uint o1 = (idx6 + 1u) * 4u + d0;
    const uint o2 = (idx6 + 2u) * 4u + d0;
    const uint o3 = (idx6 + 3u) * 4u + d0;
    const uint o4 = (idx6 + 4u) * 4u + d0;

    const float nx = fs_in_ndc.x;
    const float ny = fs_in_ndc.y;

    const float cw = fma(s2f(d1 + 12u), nx, fma(s2f(d1 + 13u), ny, fma(s2f(d1 + 14u), depth, s2f(d1 + 15u))));
    const float inv_w = 1.0 / cw;
    const float pz = fma(s2f(d1 + 8u), nx, fma(s2f(d1 + 9u), ny, fma(s2f(d1 + 10u), depth, s2f(d1 + 11u)))) * inv_w;
    const float py = fma(s2f(d1 + 4u), nx, fma(s2f(d1 + 5u), ny, fma(s2f(d1 + 6u), depth, s2f(d1 + 7u)))) * inv_w;
    const float px = fma(s2f(d1 + 0u), nx, fma(s2f(d1 + 1u), ny, fma(s2f(d1 + 2u), depth, s2f(d1 + 3u)))) * inv_w;

    const float vx = fma(s2f(d1 + 40u), px, fma(s2f(d1 + 41u), py, s2f(d1 + 42u) * pz));
    const float vy = fma(s2f(d1 + 32u), px, fma(s2f(d1 + 33u), py, s2f(d1 + 34u) * pz));
    const float vz = fma(s2f(d1 + 36u), px, fma(s2f(d1 + 37u), py, s2f(d1 + 38u) * pz));

    const vec3 sum_rgb = col5 + (col3 + col4);
    const float dist_xy = sqrt(vy * vy + vx * vx);
    const float luma = fma(sum_rgb.z, 0.114480004, fma(sum_rgb.x, 0.298909992, 0.586610019 * sum_rgb.y));
    const float luma_m1 = luma - 1.0;

    const float f188 = ssbo_1.data[o2 + 0u];
    const float f191 = ssbo_1.data[o2 + 1u];
    const float f194 = ssbo_1.data[o2 + 2u];
    const float fog_rad = clamp(f188 - f188 * exp2((dist_xy - f191) * (-f194)), 0.0, 1.0);

    const float scale1 = fma(ssbo_1.data[o1 + 3u], luma_m1, 1.0);
    const vec3 fcol1 = vec3(ssbo_1.data[o1 + 0u], ssbo_1.data[o1 + 1u], ssbo_1.data[o1 + 2u]);
    const vec3 mix1 = fma(vec3(fog_rad), fcol1 * scale1 - sum_rgb, sum_rgb);

    const float f210 = ssbo_1.data[o3 + 0u];
    const float f213 = ssbo_1.data[o3 + 1u];
    const float f216 = ssbo_1.data[o3 + 2u];
    const float f219 = ssbo_1.data[o3 + 3u];
    const float h_factor = clamp(f216 * ((vz + f219) - f213), 0.0, 1.0);
    const float fog_h = clamp(f210 * h_factor, 0.0, 1.0);

    const float scale2 = fma(ssbo_1.data[o4 + 3u], luma_m1, 1.0);
    const vec3 fcol2 = vec3(ssbo_1.data[o4 + 0u], ssbo_1.data[o4 + 1u], ssbo_1.data[o4 + 2u]);
    const vec3 mix2 = fma(vec3(fog_h), fcol2 * scale2 - mix1, mix1);

    frag_color0 = vec4(mix2, out_a);
}
