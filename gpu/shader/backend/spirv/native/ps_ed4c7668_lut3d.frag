// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL replacement for ps 0x00000000ed4c7668
// (2560x1440 16x16x16 3D Color-Grading LUT pass).
#version 450

layout(location = 0) in vec2 fs_in_uv;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform texture3D fs_img1;
layout(set = 0, binding = 2) uniform sampler fs_samp0;
layout(set = 0, binding = 3) uniform sampler fs_samp1;

void main() {
    const vec4 src = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_uv, 0.0);
    const vec3 lut_uv = fma(src.rgb, vec3(0.9375), vec3(0.03125));
    const vec3 graded = textureLod(sampler3D(fs_img1, fs_samp1), lut_uv, 0.0).rgb;
    frag_color0 = max(vec4(graded.bgr, src.a), vec4(0.0));
}
