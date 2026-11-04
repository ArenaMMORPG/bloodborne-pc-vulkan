#version 460
#extension GL_EXT_samplerless_texture_functions : require
// Native Vulkan 1.3 GLSL replacement for 0x6320bb24 (GXSimple_Single2DTexture_F.ppo)
// SSR Glossy Mip-Pyramid Blur / Simple 2D Texture Composite Pass (22 passes/frame)

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;

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

layout(set = 0, binding = 0, std430) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;
layout(set = 0, binding = 1, std430) readonly buffer ssbo_2_buf { uint data[]; } ssbo_2;

layout(set = 0, binding = 2) uniform texture2D fs_img0;
layout(set = 0, binding = 3) uniform sampler fs_samp0;

void main() {
    const uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2u;
    const uint b1 = bitfieldExtract(push_data.buf_offsets0.x, 8, 8) >> 2u;

    vec4 tex = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_attr1_p.xy, 0.0);
    if (ssbo_1.data[b0] != 0u) {
        tex.rgb = exp2(log2(tex.rgb) * 0.454545438);
    }

    const vec4 cb_col = vec4(uintBitsToFloat(ssbo_2.data[b1 + 0u]),
                             uintBitsToFloat(ssbo_2.data[b1 + 1u]),
                             uintBitsToFloat(ssbo_2.data[b1 + 2u]),
                             uintBitsToFloat(ssbo_2.data[b1 + 3u]));
    frag_color0 = tex * (cb_col * fs_in_attr0_p);
}
