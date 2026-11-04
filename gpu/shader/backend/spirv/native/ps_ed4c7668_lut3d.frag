// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL replacement for ps 0x00000000ed4c7668
// (2560x1440 16x16x16 3D Color-Grading LUT pass).
#version 450

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

layout(location = 0) in vec2 fs_in_uv;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform texture3D fs_img1;
layout(set = 0, binding = 2) uniform sampler fs_samp0;
layout(set = 0, binding = 3) uniform sampler fs_samp1;

void main() {
    const vec4 src = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_uv, 0.0);
    const vec3 lut_uv = fma(src.rgb, vec3(0.9375), vec3(0.03125));
    vec3 c = max(textureLod(sampler3D(fs_img1, fs_samp1), lut_uv, 0.0).bgr, vec3(0.0));
    if (((push_data.buf_offsets2.y >> 24u) & 0x08u) != 0u) {
        // True Black / OLED Shadow De-Haze + Filmic S-Contrast + Skin/Blood-Safe Vibrance:
        // Removes FromSoftware's 2015 LCD black-floor lift (~0.012) in dark shadows while
        // preserving midtone/highlight luminance and enriching gothic color saturation.
        const float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
        const float shadow_weight = 1.0 - smoothstep(0.0, 0.22, luma);
        const float toe = 0.011 * shadow_weight;
        c = max(c - vec3(toe), vec3(0.0)) / max(1.0 - toe, 1e-4);
        const vec3 s_curve = c * c * (3.0 - 2.0 * c);
        c = mix(c, s_curve, 0.12);
        const float luma2 = dot(c, vec3(0.2126, 0.7152, 0.0722));
        const float max_c = max(c.r, max(c.g, c.b));
        const float min_c = min(c.r, min(c.g, c.b));
        const float sat = max_c - min_c;
        c = clamp(mix(vec3(luma2), c, 1.0 + 0.14 * (1.0 - sat)), 0.0, 1.0);
    }
    frag_color0 = vec4(c, src.a);
}
