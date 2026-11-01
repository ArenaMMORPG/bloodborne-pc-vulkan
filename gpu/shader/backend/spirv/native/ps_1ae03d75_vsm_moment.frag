// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL replacement for ps 0x000000001ae03d75
// (VSM point-light depth-moment (d, d^2 + 0.25*(dx^2 + dy^2)), 38 passes/frame).
// Collapses 4 separate clip-distance demote branches into 1 vector check and eliminates
// 6 u32<->f32 bitcasts around subgroupQuadBroadcast.
#version 450
#extension GL_KHR_shader_subgroup_quad : require
#ifdef HAS_CLDIST
#extension GL_EXT_demote_to_helper_invocation : require
layout(location = 0) in vec4 cldist_attr0;
layout(location = 2) in float fs_in_depth;
#else
layout(location = 1) in float fs_in_depth;
#endif

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

layout(set = 0, binding = 0) readonly buffer SSBO1 {
    uint data[];
} ssbo_1;

void main() {
#ifdef HAS_CLDIST
    if (any(lessThan(cldist_attr0, vec4(0.0)))) {
        demote;
    }
#endif
    const uint d0 = (push_data.buf_offsets0.x & 0xFFu) >> 2u;
    const float d = uintBitsToFloat(ssbo_1.data[d0 + 1u]) * fs_in_depth;
    const float d_q0 = subgroupQuadBroadcast(d, 0u);
    const float dy = subgroupQuadBroadcast(d, 2u) - d_q0;
    const float dx = subgroupQuadBroadcast(d, 1u) - d_q0;
    frag_color0 = vec4(d, fma(0.25, fma(dx, dx, dy * dy), d * d), 0.0, 0.0);
}
