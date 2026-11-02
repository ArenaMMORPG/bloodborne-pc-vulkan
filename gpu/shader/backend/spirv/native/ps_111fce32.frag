#version 460
// Native GLSL replacement for 0x111fce32 (Fullscreen DoF / Blur Lerp Pass)

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

layout(location = 0) in vec4 fs_in_attr0;
layout(location = 1) in vec4 fs_in_attr1;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;
layout(set = 0, binding = 1) uniform texture2D fs_img0;
layout(set = 0, binding = 2) uniform texture2D fs_img1;
layout(set = 0, binding = 3) uniform sampler fs_samp0;
layout(set = 0, binding = 4) uniform sampler fs_samp1;

void main() {
    uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2;

    vec4 s0 = textureLod(sampler2D(fs_img0, fs_samp0), fs_in_attr0.xy, 0.0);
    vec3 s1 = textureLod(sampler2D(fs_img1, fs_samp1), fs_in_attr1.xy, 0.0).rgb;

    float scale = uintBitsToFloat(ssbo_1.data[b0 + 76]);
    float bias  = uintBitsToFloat(ssbo_1.data[b0 + 77]);
    float t = clamp(s0.a * scale + bias, 0.0, 1.0);

    frag_color0 = vec4(s0.rgb + (s1 - s0.rgb) * t, s0.a);
}
