#version 460
#extension GL_EXT_samplerless_texture_functions : require
// Native Vulkan 1.3 GLSL replacement for 0x3c2b7974 (Yebis 2 3-Tap Gaussian Filter Pass)

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;
layout(location = 2) in vec4 fs_in_attr2_p;

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
layout(set = 0, binding = 1) uniform texture2D fs_img0;
layout(set = 0, binding = 2) uniform sampler fs_samp0;

void main() {
    const uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2u;
    const vec4 s1 = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_attr1_p.xy, 0.0);
    const vec4 s0 = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_attr0_p.xy, 0.0);
    const vec4 s2 = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_attr2_p.xy, 0.0);

    const vec4 w0 = vec4(uintBitsToFloat(ssbo_1.data[616u + b0]),
                         uintBitsToFloat(ssbo_1.data[617u + b0]),
                         uintBitsToFloat(ssbo_1.data[618u + b0]),
                         uintBitsToFloat(ssbo_1.data[619u + b0]));
    const vec4 w1 = vec4(uintBitsToFloat(ssbo_1.data[620u + b0]),
                         uintBitsToFloat(ssbo_1.data[621u + b0]),
                         uintBitsToFloat(ssbo_1.data[622u + b0]),
                         uintBitsToFloat(ssbo_1.data[623u + b0]));
    const vec4 w2 = vec4(uintBitsToFloat(ssbo_1.data[624u + b0]),
                         uintBitsToFloat(ssbo_1.data[625u + b0]),
                         uintBitsToFloat(ssbo_1.data[626u + b0]),
                         uintBitsToFloat(ssbo_1.data[627u + b0]));

    frag_color0 = (w0 * s0 + w1 * s1) + w2 * s2;
}
