#version 460
// Native GLSL replacement for 0xe55e45eb (Radial Light Shafts / God-Rays Pass)

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

layout(location = 1) in vec4 fs_in_attr0;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;
layout(set = 0, binding = 1) uniform texture2D fs_img0;
layout(set = 0, binding = 2) uniform sampler fs_samp0;

void main() {
    uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2;

    int num_samples = int(ssbo_1.data[b0 + 66]);
    float decay     = uintBitsToFloat(ssbo_1.data[b0 + 60]);
    float weight    = uintBitsToFloat(ssbo_1.data[b0 + 61]);
    float density   = uintBitsToFloat(ssbo_1.data[b0 + 62]);
    float exposure  = uintBitsToFloat(ssbo_1.data[b0 + 63]);
    vec2 light_pos  = vec2(uintBitsToFloat(ssbo_1.data[b0 + 68]),
                           uintBitsToFloat(ssbo_1.data[b0 + 69]));

    vec2 uv = fs_in_attr0.xy;
    vec2 step_uv = (uv - light_pos) * (density / float(num_samples));

    float cur_decay = 1.0;
    vec4 acc = vec4(0.0);
    for (int i = 0; i < num_samples; ++i) {
        uv = clamp(uv - step_uv, vec2(0.0), vec2(1.0));
        vec4 s = textureLod(sampler2D(fs_img0, fs_samp0), uv, 0.0);
        float w = weight * cur_decay;
        cur_decay *= decay;
        acc += vec4(s.rgb * s.rgb, s.a) * w;
    }

    frag_color0 = vec4(max(acc.rgb, vec3(0.0)) * exposure, clamp(acc.a, 0.0, 1.0));
}
