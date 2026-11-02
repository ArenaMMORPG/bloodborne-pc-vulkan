#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_006c079b

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;


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

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    const vec4 in_attr1 = fs_in_attr1_p;
    uint t76 = bo0.x;
    uint buf0_off = bitfieldExtract(t76, int(0u), int(8u));
    uint buf0_dword_off = buf0_off >> 2u;
    uint buf1_off = bitfieldExtract(t76, int(8u), int(8u));
    uint buf1_dword_off = buf1_off >> 2u;
    uint t90 = ssbo_1.data[buf0_dword_off];
    uint t91 = uint(1u) + uint(buf0_dword_off);
    uint t93 = ssbo_1.data[t91];
    uint t94 = uint(2u) + uint(buf0_dword_off);
    uint t96 = ssbo_1.data[t94];
    uint t97 = uint(3u) + uint(buf0_dword_off);
    uint t99 = ssbo_1.data[t97];
    float t118 = in_attr1.z;
    float t124 = in_attr1.w;
    float t125 = uintBitsToFloat(uint(t96));
    float t126 = uintBitsToFloat(uint(t99));
    float t127 = uintBitsToFloat(uint(t90));
    float t128 = t118 * t127;
    float t129 = t128 + t125;
    float t130 = uintBitsToFloat(uint(t93));
    float t131 = t124 * t130;
    float t132 = t131 + t126;
    vec2 t133 = vec2(t129, t132);
    vec4 t137 = texture(sampler2D(fs_img0, fs_samp0), t133);
    float t138 = t137.w;
    uint t140 = uint(20u) + uint(buf1_dword_off);
    uint t142 = ssbo_2.data[t140];
    uint t144 = uint(21u) + uint(buf1_dword_off);
    uint t146 = ssbo_2.data[t144];
    float t158 = in_attr0.w;
    float t159 = t138 * t158;
    float t160 = uintBitsToFloat(uint(t146));
    float t162 = t160 * -0.5;
    float t163 = t162 + t159;
    float t164 = uintBitsToFloat(uint(t142));
    bool t165 = (t164 > t163);
    bool t166 = (t159 == 0.0);
    bool t167 = (t165 || t166);
    if (t167) { discard; }
}
