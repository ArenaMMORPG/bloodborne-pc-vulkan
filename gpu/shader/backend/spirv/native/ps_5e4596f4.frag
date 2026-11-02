#version 460
#extension GL_EXT_samplerless_texture_functions : require
// ps_5e4596f4

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


layout(set = 0, binding = 0) uniform texture2D fs_img0;
layout(set = 0, binding = 1) uniform sampler fs_samp0;

float safe_rsq(float x) {
    return (x > 0.0) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

void main() {
    const uvec4 bo0 = push_data.buf_offsets0;
    const vec4 in_attr0 = fs_in_attr0_p;
    const vec4 in_attr1 = fs_in_attr1_p;
    float t90 = in_attr1.x;
    float t96 = in_attr1.y;
    vec2 t97 = vec2(t90, t96);
    vec4 t101 = texture(sampler2D(fs_img0, fs_samp0), t97);
    float t102 = t101.w;
    float t114 = in_attr0.w;
    float t115 = t114 * t102;
    float t127 = in_attr0.x;
    float t139 = in_attr0.y;
    float t151 = in_attr0.z;
    vec4 t163 = vec4(t151, t139, t127, t115);
    frag_color0 = t163;
}
