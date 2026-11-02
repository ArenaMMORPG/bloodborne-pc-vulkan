// SPDX-License-Identifier: GPL-2.0-or-later
// Block 8, Step 8.2: Native Vulkan 1.3 GLSL replacement for ps 0x0000000038d65b32
// (Final 1DArray Display Gamma LUT + BGRA swizzle pass).
#version 460
#extension GL_EXT_samplerless_texture_functions : require

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

layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform texture1DArray fs_img1;
layout(set = 0, binding = 2) uniform sampler fs_samp0;

void main() {
    const uint raw_w = push_data.scene_size & 65535u;
    const uint w = (raw_w == 0u) ? 1920u : raw_w;
    const float sx = 1920.0 / float(w);

    const uint raw_h = push_data.scene_size >> 16u;
    const uint h = (raw_h == 0u) ? 1080u : raw_h;
    const float sy = 1080.0 / float(h);

    const ivec2 coord = ivec2(int(floor(gl_FragCoord.x * sx)), int(floor(gl_FragCoord.y * sy)));
    const vec4 src = texelFetch(fs_img0, coord, 0);

    const float lut_r = texture(sampler1DArray(fs_img1, fs_samp0), vec2(src.x, 0.0)).x;
    const float lut_g = texture(sampler1DArray(fs_img1, fs_samp0), vec2(src.y, 1.0)).x;
    const float lut_b = texture(sampler1DArray(fs_img1, fs_samp0), vec2(src.z, 2.0)).x;

    frag_color0 = vec4(lut_b, lut_g, lut_r, src.w);
}
