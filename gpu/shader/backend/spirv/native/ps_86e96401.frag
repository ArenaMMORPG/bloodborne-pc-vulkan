#version 460
#extension GL_EXT_samplerless_texture_functions : require
// Native Vulkan 1.3 GLSL replacement for 0x86e96401 (Yebis 2 HDR Resample / Copy Pass)

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform sampler fs_samp0;

void main() {
    frag_color0 = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_attr0_p.xy, 0.0);
}
