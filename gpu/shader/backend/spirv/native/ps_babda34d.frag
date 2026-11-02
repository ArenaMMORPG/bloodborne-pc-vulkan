#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_babda34d

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;
layout(location = 2) in vec4 fs_in_attr2_p;
layout(location = 3) in vec4 fs_in_attr3_p;

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


layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform sampler fs_samp0;

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    const vec4 in_attr1 = fs_in_attr1_p;
    const vec4 in_attr2 = fs_in_attr2_p;
    const vec4 in_attr3 = fs_in_attr3_p;
    float t92 = in_attr3.x;
    float t98 = in_attr3.y;
    vec2 t99 = vec2(t92, t98);
    vec4 t103 = texture(sampler2D(fs_img0, fs_samp0), t99);
    float t104 = t103.x;
    float t105 = t103.y;
    float t106 = t103.z;
    float t107 = t103.w;
    float t143 = in_attr1.w;
    float t167 = in_attr2.w;
    float t179 = in_attr1.x;
    float t185 = in_attr1.y;
    float t191 = in_attr1.z;
    float t197 = in_attr2.x;
    float t203 = in_attr2.y;
    float t209 = in_attr2.z;
    float t215 = in_attr0.w;
    float t216 = t167 * t107;
    float t217 = t216 + t143;
    float t218 = t197 * t104;
    float t219 = t218 + t179;
    float t220 = t203 * t105;
    float t221 = t220 + t185;
    float t222 = t209 * t106;
    float t223 = t222 + t191;
    float t224 = t217 * t215;
    vec4 t241 = vec4(t223, t221, t219, t224);
    frag_color0 = t241;
}
