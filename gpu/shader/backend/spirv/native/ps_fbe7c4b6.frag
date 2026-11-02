#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_fbe7c4b6

layout(location = 0) in vec4 fs_in_attr0_p;

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
layout(set = 0, binding = 3) uniform texture2D fs_img2;
layout(set = 0, binding = 4) uniform sampler fs_samp0;
layout(set = 0, binding = 5) uniform sampler fs_samp1;
layout(set = 0, binding = 6) uniform sampler fs_samp2;

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    uint t84 = bo0.x;
    uint buf0_off = bitfieldExtract(t84, int(0u), int(8u));
    uint buf0_dword_off = buf0_off >> 2u;
    uint t93 = uint(1024u) + uint(buf0_dword_off);
    uint t95 = ssbo_1.data[t93];
    float t96 = uintBitsToFloat(uint(t95));
    bool t98 = (t96 == 1.0);
    bool t99 = !t98;
    float t118 = in_attr0.x;
    float t124 = in_attr0.y;
    vec2 t125 = vec2(t118, t124);
    vec4 t129 = textureLod(sampler2D(fs_img0, fs_samp0), t125, 0.0);
    float t130 = t129.x;
    float t131 = t129.y;
    float t132 = t129.z;
    uint t134 = uint(616u) + uint(buf0_dword_off);
    uint t136 = ssbo_1.data[t134];
    uint t138 = uint(617u) + uint(buf0_dword_off);
    uint t140 = ssbo_1.data[t138];
    uint t142 = uint(618u) + uint(buf0_dword_off);
    uint t144 = ssbo_1.data[t142];
    float t145 = uintBitsToFloat(uint(t136));
    float t146 = t145 * t130;
    uint t147 = floatBitsToUint(t146);
    float t148 = uintBitsToFloat(uint(t140));
    float t149 = t148 * t131;
    uint t150 = floatBitsToUint(t149);
    float t151 = uintBitsToFloat(uint(t144));
    float t152 = t151 * t132;
    uint t153 = floatBitsToUint(t152);
    uint t154 = t98 ? t150 : 0u;
    uint t155 = t98 ? t147 : 1065353216u;
    uint t156 = t98 ? t153 : 0u;
    uint t237 = t99 ? 0u : t154;
    uint t238 = t99 ? 0u : t155;
    uint t239 = t99 ? 0u : t156;
    float t177 = in_attr0.x;
    float t183 = in_attr0.y;
    float t184 = uintBitsToFloat(uint(t237));
    float t185 = uintBitsToFloat(uint(t238));
    vec2 t186 = vec2(t177, t183);
    vec4 t190 = textureLod(sampler2D(fs_img1, fs_samp1), t186, 0.0);
    float t191 = t190.x;
    uint t194 = ssbo_1.data[buf0_dword_off];
    uint t195 = uint(1u) + uint(buf0_dword_off);
    uint t197 = ssbo_1.data[t195];
    uint t199 = uint(200u) + uint(buf0_dword_off);
    uint t201 = ssbo_1.data[t199];
    uint t203 = uint(68u) + uint(buf0_dword_off);
    uint t205 = ssbo_1.data[t203];
    uint t207 = uint(69u) + uint(buf0_dword_off);
    uint t209 = ssbo_1.data[t207];
    float t210 = uintBitsToFloat(uint(t197));
    float t211 = uintBitsToFloat(uint(t194));
    float t212 = t211 * t191;
    float t213 = t212 + t210;
    float t214 = uintBitsToFloat(uint(t201));
    vec2 t215 = vec2(t213, t214);
    vec4 t219 = textureLod(sampler2D(fs_img2, fs_samp2), t215, 0.0);
    float t220 = t219.x;
    float t221 = uintBitsToFloat(uint(t209));
    float t222 = uintBitsToFloat(uint(t205));
    float t223 = t222 * t220;
    float t224 = t223 + t221;
    float t225 = uintBitsToFloat(uint(t239));
    vec4 t236 = vec4(t185, t184, t225, t224);
    frag_color0 = t236;
}
