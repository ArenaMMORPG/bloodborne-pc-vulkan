#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_9e1cb278

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

layout(set = 0, binding = 1) uniform texture2D fs_img0;
layout(set = 0, binding = 2) uniform texture2D fs_img1;
layout(set = 0, binding = 3) uniform sampler fs_samp0;
layout(set = 0, binding = 4) uniform sampler fs_samp1;

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    const vec4 in_attr1 = fs_in_attr1_p;
    uint t77 = bo0.x;
    uint buf0_off = bitfieldExtract(t77, int(0u), int(8u));
    uint buf0_dword_off = buf0_off >> 2u;
    float t97 = in_attr0.x;
    float t109 = in_attr0.y;
    vec2 t110 = vec2(t97, t109);
    vec4 t114 = textureLod(sampler2D(fs_img0, fs_samp0), t110, 0.0);
    float t115 = t114.x;
    float t116 = t114.y;
    float t117 = t114.z;
    float t118 = t114.w;
    float t130 = in_attr1.x;
    float t142 = in_attr1.y;
    vec2 t143 = vec2(t130, t142);
    vec4 t147 = textureLod(sampler2D(fs_img1, fs_samp1), t143, 0.0);
    float t148 = t147.x;
    float t149 = t147.y;
    float t150 = t147.z;
    uint t152 = uint(76u) + uint(buf0_dword_off);
    uint t154 = ssbo_1.data[t152];
    uint t156 = uint(77u) + uint(buf0_dword_off);
    uint t158 = ssbo_1.data[t156];
    float t159 = uintBitsToFloat(uint(t158));
    float t160 = uintBitsToFloat(uint(t154));
    float t161 = t160 * t118;
    float t162 = t161 + t159;
    float t165 = 2.0 - t162;
    float t167 = clamp(t165, 0.0, 1.0);
    float t168 = t115 - t148;
    float t169 = t116 - t149;
    float t170 = t117 - t150;
    float t171 = t167 * t168;
    float t172 = t171 + t148;
    float t173 = t167 * t169;
    float t174 = t173 + t149;
    float t175 = t167 * t170;
    float t176 = t175 + t150;
    vec4 t186 = vec4(t172, t174, t176, t118);
    frag_color0 = t186;
}
