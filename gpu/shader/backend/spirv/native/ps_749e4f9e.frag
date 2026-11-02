#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_749e4f9e

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;
layout(location = 3) in vec4 fs_in_attr2_p;

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

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    const vec4 in_attr1 = fs_in_attr1_p;
    const vec4 in_attr3 = fs_in_attr2_p;
    uint t79 = bo0.x;
    uint buf0_off = bitfieldExtract(t79, int(0u), int(8u));
    uint buf0_dword_off = buf0_off >> 2u;
    uint buf1_off = bitfieldExtract(t79, int(8u), int(8u));
    uint buf1_dword_off = buf1_off >> 2u;
    vec3 t87 = vec3(0.0);
    float t88 = t87.y;
    float t90 = t87.z;
    uint t93 = ssbo_1.data[buf0_dword_off];
    uint t94 = uint(1u) + uint(buf0_dword_off);
    uint t96 = ssbo_1.data[t94];
    uint t97 = uint(2u) + uint(buf0_dword_off);
    uint t99 = ssbo_1.data[t97];
    uint t100 = uint(3u) + uint(buf0_dword_off);
    uint t102 = ssbo_1.data[t100];
    float t121 = in_attr3.x;
    float t127 = in_attr3.y;
    float t128 = uintBitsToFloat(uint(t99));
    float t129 = uintBitsToFloat(uint(t102));
    float t130 = uintBitsToFloat(uint(t93));
    float t131 = t130 * t121;
    float t132 = t131 + t128;
    float t133 = uintBitsToFloat(uint(t96));
    float t134 = t133 * t127;
    float t135 = t134 + t129;
    vec2 t136 = vec2(t132, t135);
    vec4 t140 = texture(sampler2D(fs_img0, fs_samp0), t136);
    float t141 = t140.w;
    bool t143 = (0.00392156886 > t141);
    bool t144 = !t143;
    if (t143) { discard; }
    uint t147 = ssbo_2.data[buf1_dword_off];
    uint t148 = uint(1u) + uint(buf1_dword_off);
    uint t150 = ssbo_2.data[t148];
    float t174 = in_attr1.w;
    float t192 = in_attr0.w;
    float t204 = in_attr1.x;
    float t210 = in_attr1.y;
    float t212 = 1.0 / t174;
    float t218 = in_attr0.x;
    float t224 = in_attr0.y;
    float t225 = 1.0 / t192;
    float t226 = t204 * t212;
    float t227 = t210 * t212;
    float t229 = t218 * t225;
    float t230 = t229 - t226;
    float t232 = t224 * t225;
    float t233 = t232 - t227;
    float t235 = 10.0 + t192;
    float t236 = uintBitsToFloat(uint(t147));
    float t237 = t236 * t230;
    float t238 = uintBitsToFloat(uint(t150));
    float t239 = t238 * t233;
    float t240 = 1.0 / t235;
    float t242 = t237 * 0.5;
    float t243 = t242 + 0.5;
    float t244 = clamp(t243, 0.0, 1.0);
    float t245 = t239 * 0.5;
    float t246 = t245 + 0.5;
    float t247 = clamp(t246, 0.0, 1.0);
    float t248 = t192 * t240;
    float t249 = t247;
    float t250 = t244;
    float t251 = t248;
    vec4 t267 = vec4(t250, t249, t251, 1.0);
    frag_color0 = t267;
}
