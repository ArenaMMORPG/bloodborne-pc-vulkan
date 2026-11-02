#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_a291c498

layout(location = 1) in vec4 fs_in_attr0_p;

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
layout(set = 0, binding = 2) uniform texture2D fs_img1;
layout(set = 0, binding = 3) uniform sampler fs_samp0;
layout(set = 0, binding = 4) uniform sampler fs_samp1;

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr1 = fs_in_attr0_p;
    uint t76 = bo0.x;
    uint buf0_off = bitfieldExtract(t76, int(0u), int(8u));
    uint buf0_dword_off = buf0_off >> 2u;
    float t102 = in_attr1.x;
    float t108 = in_attr1.y;
    vec2 t109 = vec2(t102, t108);
    vec4 t113 = texture(sampler2D(fs_img0, fs_samp0), t109);
    float t114 = t113.z;
    uint t116 = uint(5u) + uint(buf0_dword_off);
    uint t118 = ssbo_1.data[t116];
    uint t120 = uint(6u) + uint(buf0_dword_off);
    uint t122 = ssbo_1.data[t120];
    uint t123 = uint(1u) + uint(buf0_dword_off);
    uint t125 = ssbo_1.data[t123];
    uint t126 = uint(2u) + uint(buf0_dword_off);
    uint t128 = ssbo_1.data[t126];
    vec4 t133 = texture(sampler2D(fs_img1, fs_samp1), t109);
    float t134 = t133.x;
    float t135 = uintBitsToFloat(uint(t118));
    float t136 = uintBitsToFloat(uint(t122));
    float t137 = uintBitsToFloat(uint(t125));
    float t138 = t137 * t114;
    float t139 = t138 + t135;
    float t140 = uintBitsToFloat(uint(t128));
    float t141 = t140 * t114;
    float t142 = t141 + t136;
    vec4 t151 = vec4(t134, t139, t142, 1.0);
    frag_color0 = t151;
}
