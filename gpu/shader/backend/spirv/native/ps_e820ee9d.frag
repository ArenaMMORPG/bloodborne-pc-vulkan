#version 460
#extension GL_EXT_samplerless_texture_functions : require
// Native Vulkan 1.3 GLSL replacement for 0xe820ee9d (DepthScale2x2OutputDepth.ppo)
// SSR / PostFX 2x2 Hi-Z Minimum Depth Downsample Pass (4 passes/frame)

layout(location = 0) in vec4 fs_in_attr0_p;

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
    const vec2 uv = fs_in_attr0_p.xy;

    const vec2 o0 = vec2(uintBitsToFloat(ssbo_1.data[b0 + 0u]),
                         uintBitsToFloat(ssbo_1.data[b0 + 1u]));
    const vec2 o1 = vec2(uintBitsToFloat(ssbo_1.data[b0 + 4u]),
                         uintBitsToFloat(ssbo_1.data[b0 + 5u]));
    const vec2 o2 = vec2(uintBitsToFloat(ssbo_1.data[b0 + 8u]),
                         uintBitsToFloat(ssbo_1.data[b0 + 9u]));
    const vec2 o3 = vec2(uintBitsToFloat(ssbo_1.data[b0 + 12u]),
                         uintBitsToFloat(ssbo_1.data[b0 + 13u]));

    const float d0 = textureLod(sampler2D(fs_img0, fs_samp0), uv + o0, 0.0).r;
    const float d1 = textureLod(sampler2D(fs_img0, fs_samp0), uv + o1, 0.0).r;
    const float d2 = textureLod(sampler2D(fs_img0, fs_samp0), uv + o2, 0.0).r;
    const float d3 = textureLod(sampler2D(fs_img0, fs_samp0), uv + o3, 0.0).r;

    float d_min = (d0 > d1) ? d1 : d0;
    d_min = (d_min > d2) ? d2 : d_min;
    d_min = (d_min > d3) ? d3 : d_min;
    gl_FragDepth = d_min;
}
