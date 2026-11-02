// SPDX-License-Identifier: GPL-2.0-or-later
// Block 8, Step 8.2: Native Vulkan 1.3 GLSL replacement for vs 0x000000001ea5a07e
// (Final HDR Tone-Mapping fullscreen vertex shader).
#version 460

layout(location = 0) in vec4 vs_in_attr0;
layout(location = 1) in vec4 vs_in_attr1;
layout(location = 2) in vec4 vs_in_attr2;
layout(location = 3) in vec4 vs_in_attr3;
layout(location = 4) in vec4 vs_in_attr4;
layout(location = 5) in vec4 vs_in_attr5;

layout(location = 0) out vec4 out_attr0;
layout(location = 1) out vec4 out_attr1;
layout(location = 2) out vec4 out_attr2;
layout(location = 3) out vec4 out_attr3;
layout(location = 4) out vec4 out_attr4;

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

layout(set = 0, binding = 0, std430) readonly buffer SSBO2 {
    uint data[];
} ssbo_2;

void main() {
    const uint buf1_dword_off = bitfieldExtract(push_data.buf_offsets0.x, 8, 8) >> 2u;
    const float sx = uintBitsToFloat(ssbo_2.data[20u + buf1_dword_off]);
    const float sy = uintBitsToFloat(ssbo_2.data[21u + buf1_dword_off]);

    vec4 pos = vec4(sx * vs_in_attr0.x, sy * vs_in_attr0.y, 0.100000001, 1.0);
    out_attr0 = vec4(vs_in_attr1.x, vs_in_attr1.y, 0.0, 1.0);
    out_attr1 = vec4(vs_in_attr2.x, vs_in_attr2.y, 0.0, 1.0);
    out_attr2 = vec4(vs_in_attr3.x, vs_in_attr3.y, 0.0, 1.0);
    out_attr3 = vec4(vs_in_attr4.x, vs_in_attr4.y, 0.0, 1.0);
    out_attr4 = vec4(vs_in_attr5.x, vs_in_attr5.y, 0.0, 1.0);

    const uint mp = push_data.motion_param;
    if (mp != 0u) {
        const float lim_x = float(mp >> 16u) * 1.52590219e-05;
        const float lim_y = float((mp & 65534u) >> 1u) * 3.05185094e-05;
        const bool flip_y = (mp & 1u) != 0u;
        const float abs_w = abs(pos.w);
        const float sy_pos = flip_y ? pos.y : -pos.y;
        const bool cond_y_gt = sy_pos > (abs_w * 0.400000006);
        const float abs_y = abs(pos.y);
        const bool out_y = abs_y >= (abs_w * (lim_y - 0.00200000009));
        const float abs_x = abs(pos.x);
        const bool out_x = abs_x >= (abs_w * (lim_x - 0.00200000009));
        const bool both_out = out_x && out_y;
        const bool in_x_max = abs_x <= (abs_w * (lim_x + 0.00400000019));
        if (both_out || (cond_y_gt && out_x && in_x_max)) {
            pos.x = (pos.x * abs_w) / abs_x;
        }
        if (both_out) {
            pos.y = (pos.y * abs_w) / abs_y;
        }
    }
    gl_Position = pos;
}
