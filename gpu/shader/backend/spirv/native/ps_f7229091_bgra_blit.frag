// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL replacement for ps 0x00000000f7229091
// (1280x720 BGRA -> RGBA downsample blit, 4 passes/frame).
#version 450

layout(location = 0) in vec2 fs_in_uv;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform sampler fs_samp0;

void main() {
    frag_color0 = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_uv, 0.0).bgra;
}
