// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4 (Sub-Batch 2): Native Vulkan 1.3 GLSL replacement for vs 0x00000000167daba6
// (VSM shadow map instanced vertex shader, 103 draws/frame, 9.37M indices/frame in Clinic).
#version 450

layout(location = 0) in vec4 vs_in_attr0;
layout(location = 1) in uvec4 vs_in_attr1;

layout(location = 0) out vec4 out_attr0;
layout(location = 1) out vec4 out_attr1;

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

layout(set = 0, binding = 1) readonly buffer SSBO2 {
    uint data[];
} ssbo_2;

layout(set = 0, binding = 2) readonly buffer SSBO3 {
    float data[];
} ssbo_3;

layout(set = 0, binding = 3) readonly buffer SSBO4 {
    uint data[];
} ssbo_4;

float s4f(uint idx) {
    return uintBitsToFloat(ssbo_4.data[idx]);
}

void main() {
    const uint b0 = push_data.buf_offsets0.x;
    const uint d1 = ((b0 >> 8u) & 0xFFu) >> 2u;
    const uint d2 = ((b0 >> 16u) & 0xFFu) >> 2u;
    const uint d3 = ((b0 >> 24u) & 0xFFu) >> 2u;

    const uint inst = ssbo_2.data[d1 + 20u] + vs_in_attr1.w;
    const uint r2 = (inst * 3u + 2u) * 4u + d2;
    const uint r1 = (inst * 3u + 1u) * 4u + d2;
    const uint r0 = inst * 12u + d2;

    const float vx = vs_in_attr0.x;
    const float vy = vs_in_attr0.y;
    const float vz = vs_in_attr0.z;

    const float wx = fma(ssbo_3.data[r2 + 0u], vx,
                     fma(ssbo_3.data[r2 + 1u], vy,
                     fma(ssbo_3.data[r2 + 2u], vz, ssbo_3.data[r2 + 3u])));
    const float wy = fma(ssbo_3.data[r1 + 0u], vx,
                     fma(ssbo_3.data[r1 + 1u], vy,
                     fma(ssbo_3.data[r1 + 2u], vz, ssbo_3.data[r1 + 3u])));
    const float wz = fma(ssbo_3.data[r0 + 0u], vx,
                     fma(ssbo_3.data[r0 + 1u], vy,
                     fma(ssbo_3.data[r0 + 2u], vz, ssbo_3.data[r0 + 3u])));

    const float dx = wx - s4f(d3 + 191u);
    const float dy = wy - s4f(d3 + 187u);
    const float dz = wz - s4f(d3 + 183u);

    const float cx = fma(s4f(d3 + 200u), dz, fma(s4f(d3 + 201u), dy, fma(s4f(d3 + 202u), dx, s4f(d3 + 203u))));
    const float cy = fma(s4f(d3 + 204u), dz, fma(s4f(d3 + 205u), dy, fma(s4f(d3 + 206u), dx, s4f(d3 + 207u))));
    const float cz = fma(s4f(d3 + 208u), dz, fma(s4f(d3 + 209u), dy, fma(s4f(d3 + 210u), dx, s4f(d3 + 211u))));
    const float cw = fma(s4f(d3 + 212u), dz, fma(s4f(d3 + 213u), dy, fma(s4f(d3 + 214u), dx, s4f(d3 + 215u))));

    out_attr0 = vec4(0.0, 0.0, 0.0, 1.0);
    out_attr1 = vec4(cw, 0.0, 0.0, 1.0);

    vec4 pos = vec4(cx, cy, cz, cw);
    const uint mp = push_data.motion_param;
    if (mp != 0u) {
        const float lim_x = float(mp >> 16u) * 1.52590219e-05;
        const float lim_y = float((mp & 65534u) >> 1u) * 3.05185094e-05;
        const bool flip_y = (mp & 1u) != 0u;
        const float abs_w = abs(cw);
        const float sy = flip_y ? cy : -cy;
        const bool cond_y_gt = sy > (abs_w * 0.400000006);
        const float abs_y = abs(cy);
        const bool out_y = abs_y >= (abs_w * (lim_y - 0.00200000009));
        const float abs_x = abs(cx);
        const bool out_x = abs_x >= (abs_w * (lim_x - 0.00200000009));
        const bool both_out = out_x && out_y;
        const bool in_x_max = abs_x <= (abs_w * (lim_x + 0.00400000019));
        if (both_out || (cond_y_gt && out_x && in_x_max)) {
            pos.x = (cx * abs_w) / abs_x;
        }
        if (both_out) {
            pos.y = (cy * abs_w) / abs_y;
        }
    }
    gl_Position = pos;
}
