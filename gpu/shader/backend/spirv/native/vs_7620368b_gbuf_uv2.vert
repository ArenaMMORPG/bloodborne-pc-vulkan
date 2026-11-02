#version 460
// Native Vulkan 1.3 GLSL adaptation of VS 0x000000007620368b (G-Buffer 2-UV/blend mesh VS, 59 draws/frame)

layout(location = 0) in vec4 vs_in_attr0;
layout(location = 1) in uvec4 vs_in_attr1;
layout(location = 2) in uvec4 vs_in_attr2;
layout(location = 3) in vec4 vs_in_attr3;
layout(location = 4) in ivec4 vs_in_attr4;
layout(location = 5) in ivec4 vs_in_attr5;

layout(location = 0) out vec4 out_attr0;
layout(location = 1) out vec4 out_attr1;
layout(location = 2) out vec4 out_attr2;
layout(location = 3) out vec4 out_attr3;
layout(location = 4) out vec4 out_attr4;
layout(location = 5) out vec4 out_attr5;

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

layout(set = 0, binding = 0, std430) readonly buffer ssbo_8_buf { uint data[]; } ssbo_8;
layout(set = 0, binding = 1, std430) readonly buffer ssbo_9_buf { uint data[]; } ssbo_9;
layout(set = 0, binding = 2, std430) readonly buffer ssbo_10_buf { float data[]; } ssbo_10;
layout(set = 0, binding = 3, std430) readonly buffer ssbo_11_buf { uint data[]; } ssbo_11;

void main() {
    const vec4 pos_in = vs_in_attr0;
    const uvec4 nrm_u = vs_in_attr1;
    const uvec4 tan_u = vs_in_attr2;
    const vec4 col_in = vs_in_attr3;
    const vec4 uv_in = vec4(vs_in_attr4) * 0.00048828125;
    const vec2 uv2_in = vec2(vs_in_attr5.xy) * 0.00048828125;

    const vec3 n_in = fma(vec3(nrm_u.xyz), vec3(0.00784313772), vec3(-1.0));
    const vec4 t_in = fma(vec4(tan_u), vec4(0.00784313772), vec4(-1.0));

    const uvec4 bo0 = push_data.buf_offsets0;
    const uint d7 = (bo0.y >> 24u) >> 2u;
    const uint d8 = (bo0.z & 0xFFu) >> 2u;
    const uint d9 = ((bo0.z >> 8u) & 0xFFu) >> 2u;
    const uint d10 = ((bo0.z >> 16u) & 0xFFu) >> 2u;

    const uint inst = ssbo_8.data[d7 + 20u] + nrm_u.w;
    const uint r0 = inst * 12u + d9;
    const uint r1 = (inst * 3u + 1u) * 4u + d9;
    const uint r2 = (inst * 3u + 2u) * 4u + d9;

    const vec4 m0 = vec4(ssbo_10.data[r0 + 0u], ssbo_10.data[r0 + 1u], ssbo_10.data[r0 + 2u], ssbo_10.data[r0 + 3u]);
    const vec4 m1 = vec4(ssbo_10.data[r1 + 0u], ssbo_10.data[r1 + 1u], ssbo_10.data[r1 + 2u], ssbo_10.data[r1 + 3u]);
    const vec4 m2 = vec4(ssbo_10.data[r2 + 0u], ssbo_10.data[r2 + 1u], ssbo_10.data[r2 + 2u], ssbo_10.data[r2 + 3u]);

    const float wx = ((pos_in.x * m0.x) + (pos_in.y * m0.y)) + (pos_in.z * m0.z) + m0.w;
    const float wy = ((pos_in.x * m1.x) + (pos_in.y * m1.y)) + (pos_in.z * m1.z) + m1.w;
    const float wz = ((pos_in.x * m2.x) + (pos_in.y * m2.y)) + (pos_in.z * m2.z) + m2.w;

    const float nx = (n_in.x * m0.x) + (n_in.y * m0.y) + (n_in.z * m0.z);
    const float ny = (n_in.x * m1.x) + (n_in.y * m1.y) + (n_in.z * m1.z);
    const float nz = (n_in.x * m2.x) + (n_in.y * m2.y) + (n_in.z * m2.z);

    const float tx = (t_in.x * m0.x) + (t_in.y * m0.y) + (t_in.z * m0.z);
    const float ty = (t_in.x * m1.x) + (t_in.y * m1.y) + (t_in.z * m1.z);
    const float tz = (t_in.x * m2.x) + (t_in.y * m2.y) + (t_in.z * m2.z);

    const float rx = wx - uintBitsToFloat(ssbo_9.data[d8 + 183u]);
    const float ry = wy - uintBitsToFloat(ssbo_9.data[d8 + 187u]);
    const float rz = wz - uintBitsToFloat(ssbo_9.data[d8 + 191u]);

    const float cx = (rx * uintBitsToFloat(ssbo_9.data[d8 + 200u])) + ((ry * uintBitsToFloat(ssbo_9.data[d8 + 201u])) + ((rz * uintBitsToFloat(ssbo_9.data[d8 + 202u])) + uintBitsToFloat(ssbo_9.data[d8 + 203u])));
    const float cy = (rx * uintBitsToFloat(ssbo_9.data[d8 + 204u])) + ((ry * uintBitsToFloat(ssbo_9.data[d8 + 205u])) + ((rz * uintBitsToFloat(ssbo_9.data[d8 + 206u])) + uintBitsToFloat(ssbo_9.data[d8 + 207u])));
    const float cz = (rx * uintBitsToFloat(ssbo_9.data[d8 + 208u])) + ((ry * uintBitsToFloat(ssbo_9.data[d8 + 209u])) + ((rz * uintBitsToFloat(ssbo_9.data[d8 + 210u])) + uintBitsToFloat(ssbo_9.data[d8 + 211u])));
    const float cw = (rx * uintBitsToFloat(ssbo_9.data[d8 + 212u])) + ((ry * uintBitsToFloat(ssbo_9.data[d8 + 213u])) + ((rz * uintBitsToFloat(ssbo_9.data[d8 + 214u])) + uintBitsToFloat(ssbo_9.data[d8 + 215u])));

    vec4 clip_pos = vec4(cx, cy, cz, cw);

    const float u0 = (uv_in.x * uintBitsToFloat(ssbo_11.data[d10 + 0u])) + uintBitsToFloat(ssbo_11.data[d10 + 2u]);
    const float v0 = (uv_in.y * uintBitsToFloat(ssbo_11.data[d10 + 1u])) + uintBitsToFloat(ssbo_11.data[d10 + 3u]);
    const float u1 = (uv_in.z * uintBitsToFloat(ssbo_8.data[d7 + 12u])) + uintBitsToFloat(ssbo_8.data[d7 + 14u]);
    const float v1 = (uv_in.w * uintBitsToFloat(ssbo_8.data[d7 + 13u])) + uintBitsToFloat(ssbo_8.data[d7 + 15u]);
    const float u2 = (uv2_in.x * uintBitsToFloat(ssbo_11.data[d10 + 12u])) + uintBitsToFloat(ssbo_11.data[d10 + 14u]);
    const float v2 = (uv2_in.y * uintBitsToFloat(ssbo_11.data[d10 + 13u])) + uintBitsToFloat(ssbo_11.data[d10 + 15u]);

    out_attr0 = vec4(nx, ny, nz, 0.0);
    out_attr1 = vec4(tx, ty, tz, t_in.w);
    out_attr2 = col_in;
    out_attr3 = vec4(u0, v0, u2, v2);
    out_attr4 = clip_pos;
    out_attr5 = vec4(u1, v1, 0.0, 1.0);

    const uint mp = (push_data.motion_param != 0u) ? push_data.buf_offsets2.x : 0u;
    if (mp != 0u) {
        const vec2 mp_lo = unpackHalf2x16(mp);
        const vec2 mp_hi = unpackHalf2x16(push_data.scene_size);
        const float px = clamp(pos_in.x, -1.0, 1.0);
        const float py = clamp(pos_in.y, -1.0, 1.0);
        const bool is_quad = (abs(pos_in.x - px) < 0.0001) && (abs(pos_in.y - py) < 0.0001) && (abs(pos_in.z) < 0.0001) &&
                             (abs(clip_pos.x - px) < 0.01) && (abs(clip_pos.y - py) < 0.01) && (abs(clip_pos.w - 1.0) < 0.0001);
        if (!is_quad) {
            const float inv_w = (abs(clip_pos.w) > 9.99999975e-06) ? (1.0 / clip_pos.w) : 0.0;
            const float ndc_x = clip_pos.x * inv_w;
            const float ndc_y = clip_pos.y * inv_w;
            const float sx = ndc_x * mp_lo.x + mp_hi.x * (1.0 - mp_lo.x);
            const float sy = ndc_y * mp_lo.x + (-mp_hi.y) * (1.0 - mp_lo.x);
            const float z_lin = 1.0 - (clip_pos.z * inv_w) * mp_lo.y;
            const float inv_z = 1.0 / max(z_lin, 0.0001);
            clip_pos.x = sx * clip_pos.w;
            clip_pos.y = sy * clip_pos.w;
            clip_pos.z = ((1.0 - (1.0 - mp_lo.y) * inv_z) * clip_pos.w) * float(mp_lo.y > 0.0) + clip_pos.z * float(!(mp_lo.y > 0.0));
        }
    }
    gl_Position = clip_pos;
}
