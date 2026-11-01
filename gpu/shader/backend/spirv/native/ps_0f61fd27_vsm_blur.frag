// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL replacement for ps 0x000000000f61fd27
// (13-tap VSM Gaussian blur, 36 passes/frame across 32x32..256x256).
// Streams taps sequentially instead of preloading 13 vec4 samples + 39 SSBO scalars (~91 VGPRs -> ~12 VGPRs),
// preserving the original GCN tap summation order (1, 0, 2..12).
#version 450

layout(location = 0) in vec2 fs_in_uv;
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
layout(set = 0, binding = 3) uniform sampler fs_samp0;

vec4 sample_tap(uint base, vec2 uv0, vec2 scale) {
    const vec2 off = vec2(uintBitsToFloat(ssbo_1.data[base]),
                          uintBitsToFloat(ssbo_1.data[base + 1u]));
    const float w = uintBitsToFloat(ssbo_1.data[base + 2u]);
    return w * textureLod(sampler2D(fs_img0, fs_samp0), fma(scale, off, uv0), 0.0);
}

void main() {
    const uint d0 = (push_data.buf_offsets0.x & 0xFFu) >> 2u;
    const uint d1 = ((push_data.buf_offsets0.x >> 8u) & 0xFFu) >> 2u;
    const vec2 scale = vec2(uintBitsToFloat(ssbo_2.data[d1 + 4u]),
                            uintBitsToFloat(ssbo_2.data[d1 + 5u]));
    const vec2 uv0 = fs_in_uv;

    vec4 acc = sample_tap(d0 + 4u, uv0, scale);
    acc += sample_tap(d0 + 0u, uv0, scale);
    for (uint k = 2u; k < 13u; ++k) {
        acc += sample_tap(d0 + (k << 2u), uv0, scale);
    }
    frag_color0 = acc;
}
