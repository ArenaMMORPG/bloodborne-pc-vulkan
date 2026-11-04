#version 460
// Native GLSL replacement for 0xcbbca5dc (16-Tap SSAO Generation + Depth Packing Pass)

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
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;
layout(set = 0, binding = 1) readonly buffer ssbo_2_buf { uint data[]; } ssbo_2;
layout(set = 0, binding = 2) readonly buffer ssbo_3_buf { uint data[]; } ssbo_3;
layout(set = 0, binding = 3) uniform texture2D fs_img0;
layout(set = 0, binding = 4) uniform texture2D fs_img1;
layout(set = 0, binding = 5) uniform texture2D fs_img2;
layout(set = 0, binding = 6) uniform sampler fs_samp0;
layout(set = 0, binding = 7) uniform sampler fs_samp1;
layout(set = 0, binding = 8) uniform sampler fs_samp2;

void main() {
    uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2;
    uint b1 = bitfieldExtract(push_data.buf_offsets0.x, 8, 8) >> 2;
    uint b2 = bitfieldExtract(push_data.buf_offsets0.x, 16, 8) >> 2;

    vec2 uv = fs_in_attr0.xy;
    float d = textureLod(sampler2D(fs_img0, fs_samp0), uv, 0.0).r;
    vec3 gbuf_n = texture(sampler2D(fs_img1, fs_samp1), uv).rgb;

    vec4 p0 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 0u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 1u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 2u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 3u]));
    vec4 p1 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 4u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 5u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 6u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 7u]));
    vec4 p2 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 8u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 9u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 10u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 11u]));
    vec4 p3 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 12u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 13u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 14u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 15u]));

    float ndc_x = uv.x * 2.0 - 1.0;
    float ndc_y = 1.0 - uv.y * 2.0;
    float w_rcp = 1.0 / (p3.x * ndc_x + (p3.y * ndc_y + (p3.z * d + p3.w)));
    float vx = (p0.x * ndc_x + (p0.y * ndc_y + (p0.z * d + p0.w))) * w_rcp;
    float vy = (p1.x * ndc_x + (p1.y * ndc_y + (p1.z * d + p1.w))) * w_rcp;
    float vz = (p2.x * ndc_x + (p2.y * ndc_y + (p2.z * d + p2.w))) * w_rcp;

    float ao_acc = 1.0;
    if (d != 1.0) {
        ao_acc = 0.0;
        vec2 rot_scale = vec2(uintBitsToFloat(ssbo_1.data[b0 + 16u]),
                              uintBitsToFloat(ssbo_1.data[b0 + 17u]));
        vec2 rad_scale = vec2(uintBitsToFloat(ssbo_1.data[b0 + 18u]),
                              uintBitsToFloat(ssbo_1.data[b0 + 19u]));

        vec2 noise = textureLod(sampler2D(fs_img2, fs_samp2), (uv * rot_scale) * 0.015625, 0.0).rg * 2.0 - 1.0;
        const bool enhanced_gtao = ((push_data.buf_offsets2.y >> 24u) & 0x01u) != 0u;
        if (enhanced_gtao) {
            // Break the repeating 64x64 PS4 noise tile grid with Jimenez Interleaved Gradient Noise.
            const float ign = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
            const float ang = ign * 6.28318530718;
            const vec2 ign_cs = vec2(cos(ang), sin(ang));
            noise = normalize(vec2(noise.x * ign_cs.x - noise.y * ign_cs.y,
                                   noise.x * ign_cs.y + noise.y * ign_cs.x));
        }

        vec3 n_ws = gbuf_n * 2.0 - 1.0;
        float n_len2 = dot(n_ws, n_ws);
        vec3 n_unit = (n_len2 > 0.0) ? (n_ws * inversesqrt(n_len2)) : vec3(0.0);

        vec3 m0 = vec3(uintBitsToFloat(ssbo_1.data[b0 + 20u]),
                       uintBitsToFloat(ssbo_1.data[b0 + 21u]),
                       uintBitsToFloat(ssbo_1.data[b0 + 22u]));
        vec3 m1 = vec3(uintBitsToFloat(ssbo_1.data[b0 + 24u]),
                       uintBitsToFloat(ssbo_1.data[b0 + 25u]),
                       uintBitsToFloat(ssbo_1.data[b0 + 26u]));
        vec3 m2 = vec3(uintBitsToFloat(ssbo_1.data[b0 + 28u]),
                       uintBitsToFloat(ssbo_1.data[b0 + 29u]),
                       uintBitsToFloat(ssbo_1.data[b0 + 30u]));
        vec3 n_vs = vec3(dot(m0, n_unit), dot(m1, n_unit), dot(m2, n_unit));

        float inv_vz  = 1.0 / vz;
        float r_scale = uintBitsToFloat(ssbo_1.data[b0 + 40u]);
        float r_min   = uintBitsToFloat(ssbo_1.data[b0 + 41u]);
        float r_max   = uintBitsToFloat(ssbo_1.data[b0 + 42u]);
        float radius  = max(r_min, min(r_max, r_scale * inv_vz));
        vec2 uv_rad   = rad_scale * radius;

        for (uint i = 0u; i < 8u; ++i) {
            uint k_base = b1 + i * 4u;
            vec2 k_xy = vec2(uintBitsToFloat(ssbo_2.data[k_base + 0u]),
                             uintBitsToFloat(ssbo_2.data[k_base + 1u]));
            float k_w = uintBitsToFloat(ssbo_2.data[k_base + 2u]);

            vec2 rot_k = vec2(k_xy.x * noise.y + k_xy.y * noise.x,
                              k_xy.y * noise.y - k_xy.x * noise.x);
            vec2 delta_uv = rot_k * uv_rad;
            vec2 uv_a = uv + delta_uv;
            vec2 uv_b = uv - delta_uv;

            float d_a = textureLod(sampler2D(fs_img0, fs_samp0), uv_a, 0.0).r;
            float d_b = textureLod(sampler2D(fs_img0, fs_samp0), uv_b, 0.0).r;

            vec2 ndc_a = vec2(uv_a.x * 2.0 - 1.0, 1.0 - uv_a.y * 2.0);
            vec2 ndc_b = vec2(uv_b.x * 2.0 - 1.0, 1.0 - uv_b.y * 2.0);

            float wa_rcp = 1.0 / (p3.x * ndc_a.x + (p3.y * ndc_a.y + (p3.z * d_a + p3.w)));
            float wb_rcp = 1.0 / (p3.x * ndc_b.x + (p3.y * ndc_b.y + (p3.z * d_b + p3.w)));

            vec3 pos_a = vec3(p0.x * ndc_a.x + (p0.y * ndc_a.y + (p0.z * d_a + p0.w)),
                              p1.x * ndc_a.x + (p1.y * ndc_a.y + (p1.z * d_a + p1.w)),
                              p2.x * ndc_a.x + (p2.y * ndc_a.y + (p2.z * d_a + p2.w))) * wa_rcp;
            vec3 pos_b = vec3(p0.x * ndc_b.x + (p0.y * ndc_b.y + (p0.z * d_b + p0.w)),
                              p1.x * ndc_b.x + (p1.y * ndc_b.y + (p1.z * d_b + p1.w)),
                              p2.x * ndc_b.x + (p2.y * ndc_b.y + (p2.z * d_b + p2.w))) * wb_rcp;

            vec3 diff_a = pos_a - vec3(vx, vy, vz);
            vec3 diff_b = pos_b - vec3(vx, vy, vz);
            float len2_a = dot(diff_a, diff_a);
            float len2_b = dot(diff_b, diff_b);

            vec3 dir_a = (len2_a > 0.0) ? (diff_a * inversesqrt(len2_a)) : vec3(0.0);
            vec3 dir_b = (len2_b > 0.0) ? (diff_b * inversesqrt(len2_b)) : vec3(0.0);

            float o_a = clamp(1.0 - dot(dir_a, n_vs), 0.0, 1.0);
            float o_b = clamp(1.0 - dot(dir_b, n_vs), 0.0, 1.0);

            float fall_a = clamp(((vz * 0.899999976 - pos_a.z) * inv_vz) * 10.0, 0.0, 1.0);
            float fall_b = clamp(((vz * 0.899999976 - pos_b.z) * inv_vz) * 10.0, 0.0, 1.0);

            float o_sum = o_a + o_b;
            float o_sq4 = (o_sum * o_sum) * 0.25;
            float pair_ao = max(fall_a, fall_b) * (1.0 - o_sq4) + o_sq4;
            ao_acc += k_w * pair_ao;
        }

        if (enhanced_gtao) {
            // Multi-scale fine micro-crevice GTAO pass (4 rotated inner horizon pairs at 0.32x radius).
            const vec2 micro_noise = vec2(noise.x * 0.9238795 - noise.y * 0.3826834,
                                          noise.x * 0.3826834 + noise.y * 0.9238795);
            const vec2 micro_rad = uv_rad * 0.32;
            float micro_acc = 0.0;
            float micro_w_sum = 0.0;
            for (uint i = 0u; i < 4u; ++i) {
                uint k_base = b1 + (i * 2u) * 4u;
                vec2 k_xy = vec2(uintBitsToFloat(ssbo_2.data[k_base + 0u]),
                                 uintBitsToFloat(ssbo_2.data[k_base + 1u]));
                float k_w = uintBitsToFloat(ssbo_2.data[k_base + 2u]);
                vec2 rot_k = vec2(k_xy.x * micro_noise.y + k_xy.y * micro_noise.x,
                                  k_xy.y * micro_noise.y - k_xy.x * micro_noise.x);
                vec2 delta_uv = rot_k * micro_rad;
                vec2 uv_a = uv + delta_uv;
                vec2 uv_b = uv - delta_uv;
                float d_a = textureLod(sampler2D(fs_img0, fs_samp0), uv_a, 0.0).r;
                float d_b = textureLod(sampler2D(fs_img0, fs_samp0), uv_b, 0.0).r;
                vec2 ndc_a = vec2(uv_a.x * 2.0 - 1.0, 1.0 - uv_a.y * 2.0);
                vec2 ndc_b = vec2(uv_b.x * 2.0 - 1.0, 1.0 - uv_b.y * 2.0);
                float wa_rcp = 1.0 / (p3.x * ndc_a.x + (p3.y * ndc_a.y + (p3.z * d_a + p3.w)));
                float wb_rcp = 1.0 / (p3.x * ndc_b.x + (p3.y * ndc_b.y + (p3.z * d_b + p3.w)));
                vec3 pos_a = vec3(p0.x * ndc_a.x + (p0.y * ndc_a.y + (p0.z * d_a + p0.w)),
                                  p1.x * ndc_a.x + (p1.y * ndc_a.y + (p1.z * d_a + p1.w)),
                                  p2.x * ndc_a.x + (p2.y * ndc_a.y + (p2.z * d_a + p2.w))) * wa_rcp;
                vec3 pos_b = vec3(p0.x * ndc_b.x + (p0.y * ndc_b.y + (p0.z * d_b + p0.w)),
                                  p1.x * ndc_b.x + (p1.y * ndc_b.y + (p1.z * d_b + p1.w)),
                                  p2.x * ndc_b.x + (p2.y * ndc_b.y + (p2.z * d_b + p2.w))) * wb_rcp;
                vec3 diff_a = pos_a - vec3(vx, vy, vz);
                vec3 diff_b = pos_b - vec3(vx, vy, vz);
                float len2_a = dot(diff_a, diff_a);
                float len2_b = dot(diff_b, diff_b);
                vec3 dir_a = (len2_a > 0.0) ? (diff_a * inversesqrt(len2_a)) : vec3(0.0);
                vec3 dir_b = (len2_b > 0.0) ? (diff_b * inversesqrt(len2_b)) : vec3(0.0);
                float o_a = clamp(1.0 - dot(dir_a, n_vs), 0.0, 1.0);
                float o_b = clamp(1.0 - dot(dir_b, n_vs), 0.0, 1.0);
                float fall_a = clamp(((vz * 0.965 - pos_a.z) * inv_vz) * 28.0, 0.0, 1.0);
                float fall_b = clamp(((vz * 0.965 - pos_b.z) * inv_vz) * 28.0, 0.0, 1.0);
                float o_sum = o_a + o_b;
                float o_sq4 = (o_sum * o_sum) * 0.25;
                float pair_ao = max(fall_a, fall_b) * (1.0 - o_sq4) + o_sq4;
                micro_acc += k_w * pair_ao;
                micro_w_sum += k_w;
            }
            if (micro_w_sum > 0.0) {
                float micro_ao = micro_acc / micro_w_sum;
                ao_acc = min(ao_acc, mix(ao_acc, ao_acc * micro_ao, 0.45));
            }
        }
    }

    float ao_scale = uintBitsToFloat(ssbo_1.data[b0 + 36u]);
    float ao_bias  = uintBitsToFloat(ssbo_1.data[b0 + 37u]);
    float out_r    = ao_acc * ao_scale + ao_bias;

    float z_far  = uintBitsToFloat(ssbo_3.data[b2 + 2u]);
    float z_norm = vz / z_far;
    float f65025 = fract(z_norm * 65025.0);
    float f255   = fract(z_norm * 255.0);

    frag_color0 = vec4(out_r, f65025, f255 - f65025 * 0.00392156886, z_norm - f255 * 0.00392156886);
}
