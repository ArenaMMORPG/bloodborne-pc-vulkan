#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_ccbf44a6

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


layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform sampler fs_samp0;

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    float t83 = in_attr0.x;
    float t95 = in_attr0.y;
    vec2 t96 = vec2(t83, t95);
    vec4 t100 = textureLod(sampler2D(fs_img0, fs_samp0), t96, 0.0);
    float t101 = t100.x;
    float t102 = t100.y;
    float t103 = t100.z;
    float t104 = t100.w;
    float t105 = max(0.0, t101);
    float t106 = max(0.0, t102);
    float t107 = max(0.0, t103);
    float t108 = max(0.0, t104);
    vec4 t116 = vec4(t105, t106, t107, t108);
    frag_color0 = t116;
}
