// SPDX-License-Identifier: GPL-2.0-or-later
// Block 8, Step 8.2: Native Vulkan 1.3 GLSL replacement for vs 0x00000000061fbbf6
// (Bilateral Blur / CSM Shadows / SSAO fullscreen quad vertex shader).
#version 460

layout(location = 0) in vec4 vs_in_attr0;
layout(location = 1) in vec4 vs_in_attr1;

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

void main() {
    vec4 pos = vs_in_attr0;
    out_attr0 = pos;
    out_attr1 = vec4(vs_in_attr1.x, vs_in_attr1.y, 0.0, 1.0);

    const uint mp = push_data.motion_param;
    if (mp != 0u) {
        const float lim_x = float(mp >> 16u) * 1.52590219e-05;
        const float lim_y = float((mp & 65534u) >> 1u) * 3.05185094e-05;
        const bool flip_y = (mp & 1u) != 0u;
        const float abs_w = abs(pos.w);
        const float sy = flip_y ? pos.y : -pos.y;
        const bool cond_y_gt = sy > (abs_w * 0.400000006);
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
