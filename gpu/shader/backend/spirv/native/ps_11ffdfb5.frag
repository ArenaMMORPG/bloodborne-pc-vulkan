#version 460
// Native GLSL replacement for 0x11ffdfb5 (12-Tap Bilateral Depth-Aware Blur Pass)

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

float unpack_depth(vec3 rgb) {
    return rgb.b + (rgb.g * 0.00392156886 + rgb.r * 1.53787005e-05);
}

void main() {
    uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2;
    vec2 uv = fs_in_attr0.xy;

    vec4 s0 = texture(sampler2D(fs_img0, fs_samp0), uv);
    float z_a = uintBitsToFloat(ssbo_1.data[b0 + 0]);
    float z_b = -uintBitsToFloat(ssbo_1.data[b0 + 1]);

    float z0 = z_b / (z_a - unpack_depth(s0.rgb));
    float inv_z0_sq = 144.269501 / (z0 * z0);

    float w_sum = 1.0;
    float val_sum = s0.a;

    for (uint k = 0u; k < 12u; ++k) {
        uint base = b0 + 4u + k * 4u;
        vec2 off_uv = vec2(uintBitsToFloat(ssbo_1.data[base + 0u]),
                           uintBitsToFloat(ssbo_1.data[base + 1u]));
        float kw = uintBitsToFloat(ssbo_1.data[base + 2u]);

        vec4 sk = texture(sampler2D(fs_img0, fs_samp0), uv + off_uv);
        float zk = z_b / (z_a - unpack_depth(sk.rgb));
        float dz = z0 - zk;
        float wk = kw * exp2((-dz * dz) * inv_z0_sq);

        w_sum += wk;
        val_sum += sk.a * wk;
    }

    frag_color0 = vec4(s0.rgb, val_sum / w_sum);
}
