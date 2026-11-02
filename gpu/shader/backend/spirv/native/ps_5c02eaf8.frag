#version 460
// Native GLSL replacement for 0x5c02eaf8 (Final HDR Tone-Mapping + Color Grading + Dither Pass)

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

layout(location = 0) in vec4 fs_in_attr0;
layout(location = 1) in vec4 fs_in_attr1;
layout(location = 3) in vec4 fs_in_attr2;
layout(location = 4) in vec4 fs_in_attr3;
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;
layout(set = 0, binding = 1) uniform texture2D fs_img0;
layout(set = 0, binding = 2) uniform texture2D fs_img1;
layout(set = 0, binding = 3) uniform texture2D fs_img2;
layout(set = 0, binding = 4) uniform texture2D fs_img3;
layout(set = 0, binding = 5) uniform sampler fs_samp0;
layout(set = 0, binding = 6) uniform sampler fs_samp1;
layout(set = 0, binding = 7) uniform sampler fs_samp2;
layout(set = 0, binding = 8) uniform sampler fs_samp3;

void main() {
    uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2;

    vec3 t0 = texture(sampler2D(fs_img0, fs_samp0), fs_in_attr2.xy).rgb;
    vec4 t1 = textureLod(sampler2D(fs_img1, fs_samp1), fs_in_attr0.xy, 0.0);
    vec3 t2 = textureLod(sampler2D(fs_img2, fs_samp2), fs_in_attr1.xy, 0.0).rgb;
    float t3 = textureLod(sampler2D(fs_img3, fs_samp3), fs_in_attr3.xy, 0.0).r;

    vec3 c0 = t0.bgr;
    vec3 c1 = t1.bgr;
    vec3 c2 = t2.bgr;

    float c752 = uintBitsToFloat(ssbo_1.data[b0 + 752]);
    float c753 = uintBitsToFloat(ssbo_1.data[b0 + 753]);
    float c754 = uintBitsToFloat(ssbo_1.data[b0 + 754]);
    float c755 = uintBitsToFloat(ssbo_1.data[b0 + 755]);
    float c756 = uintBitsToFloat(ssbo_1.data[b0 + 756]);

    vec3 a0 = c0 * c752 + c753;
    float avg0 = (a0.z + (a0.y + a0.x)) * 0.333333343;
    vec3 sat0 = ((a0 - avg0) * c756 + avg0) * c754 + c755;

    float c616 = uintBitsToFloat(ssbo_1.data[b0 + 616]);
    float c617 = uintBitsToFloat(ssbo_1.data[b0 + 617]);
    float c832 = uintBitsToFloat(ssbo_1.data[b0 + 832]);
    float c833 = uintBitsToFloat(ssbo_1.data[b0 + 833]);

    vec3 hdr = max(vec3(6.10351562e-05), sat0 * (c1 * c616));
    vec3 ex = exp2((-hdr * c832) * 1.44269502);
    vec3 one_minus_ex = 1.0 - ex;
    vec3 mod_ex = 1.0 - ex * c833;
    vec3 curve = clamp(mod_ex * (one_minus_ex * mod_ex), 0.0, 1.0);

    float c744 = uintBitsToFloat(ssbo_1.data[b0 + 744]);
    float c745 = uintBitsToFloat(ssbo_1.data[b0 + 745]);
    float c746 = uintBitsToFloat(ssbo_1.data[b0 + 746]);
    float c747 = uintBitsToFloat(ssbo_1.data[b0 + 747]);
    float c748 = uintBitsToFloat(ssbo_1.data[b0 + 748]);

    vec3 a1 = c0 * c744 + c745;
    float avg1 = (a1.z + (a1.y + a1.x)) * 0.333333343;
    vec3 sat1 = clamp((a1 - avg1) * c748 + avg1, 0.0, 1.0) * c746 + c747;

    vec3 bloom = clamp((c2 * sat0 + sat1) * c617, 0.0, 1.0);
    vec3 screen = (curve + bloom) - curve * bloom;

    vec4 m0 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 840]),
                   uintBitsToFloat(ssbo_1.data[b0 + 841]),
                   uintBitsToFloat(ssbo_1.data[b0 + 842]),
                   uintBitsToFloat(ssbo_1.data[b0 + 843]));
    vec4 m1 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 844]),
                   uintBitsToFloat(ssbo_1.data[b0 + 845]),
                   uintBitsToFloat(ssbo_1.data[b0 + 846]),
                   uintBitsToFloat(ssbo_1.data[b0 + 847]));
    vec4 m2 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 848]),
                   uintBitsToFloat(ssbo_1.data[b0 + 849]),
                   uintBitsToFloat(ssbo_1.data[b0 + 850]),
                   uintBitsToFloat(ssbo_1.data[b0 + 851]));

    float g0 = m0.x * screen.z + (m0.y * screen.y + (m0.z * screen.x + m0.w));
    float g1 = m1.x * screen.z + (m1.y * screen.y + (m1.z * screen.x + m1.w));
    float g2 = m2.x * screen.z + (m2.y * screen.y + (m2.z * screen.x + m2.w));

    vec3 graded = clamp(vec3(g0, g1, g2) + 2.38418579e-07, 0.0, 1.0);
    float inv_gamma = uintBitsToFloat(ssbo_1.data[b0 + 808]);
    float dither_scale = uintBitsToFloat(ssbo_1.data[b0 + 809]);
    float dither_bias = uintBitsToFloat(ssbo_1.data[b0 + 810]);

    vec3 gamma_out = exp2(log2(graded) * inv_gamma) + (t3 * dither_scale) + dither_bias;

    vec4 aw = vec4(uintBitsToFloat(ssbo_1.data[b0 + 620]),
                   uintBitsToFloat(ssbo_1.data[b0 + 621]),
                   uintBitsToFloat(ssbo_1.data[b0 + 622]),
                   uintBitsToFloat(ssbo_1.data[b0 + 623]));
    float out_a = clamp(t1.r * aw.x + (t1.g * aw.y + (t1.b * aw.z + t1.a * aw.w)), 0.0, 1.0);

    frag_color0 = vec4(gamma_out.zyx, out_a);
}
