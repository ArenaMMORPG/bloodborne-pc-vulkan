#version 460
// Native GLSL replacement for 0xf55643de (Screen-Space Cascaded Shadow + Depth Packing Pass)

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
layout(location = 0) out vec4 frag_color0;

layout(set = 0, binding = 0) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;
layout(set = 0, binding = 1) readonly buffer ssbo_2_buf { uint data[]; } ssbo_2;
layout(set = 0, binding = 2) uniform texture2D fs_img0;
layout(set = 0, binding = 3) uniform texture2D fs_img1;
layout(set = 0, binding = 4) uniform sampler fs_samp0;
layout(set = 0, binding = 5) uniform samplerShadow fs_samp1;

void main() {
    uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2;
    uint b1 = bitfieldExtract(push_data.buf_offsets0.x, 8, 8) >> 2;

    float d = texture(sampler2D(fs_img0, fs_samp0), fs_in_attr1.xy).r;
    float in_x = fs_in_attr0.x;
    float in_y = fs_in_attr0.y;

    vec4 r0 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 0u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 1u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 2u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 3u]));
    vec4 r1 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 4u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 5u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 6u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 7u]));
    vec4 r2 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 8u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 9u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 10u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 11u]));
    vec4 r3 = vec4(uintBitsToFloat(ssbo_1.data[b0 + 12u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 13u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 14u]),
                   uintBitsToFloat(ssbo_1.data[b0 + 15u]));

    float inv_w = 1.0 / (r3.x * in_x + (r3.y * in_y + (r3.z * d + r3.w)));
    float vz = (r2.x * in_x + (r2.y * in_y + (r2.z * d + r2.w))) * inv_w;

    float split0 = uintBitsToFloat(ssbo_1.data[b0 + 80u]);
    float split1 = uintBitsToFloat(ssbo_1.data[b0 + 81u]);
    float split2 = uintBitsToFloat(ssbo_1.data[b0 + 82u]);
    uint cascade = (split0 < vz ? 1u : 0u) + (vz > split1 ? 1u : 0u) + (vz > split2 ? 1u : 0u);

    uint c_base = b0 + ((cascade << 2u) + 4u) * 4u;
    vec4 sm0 = vec4(uintBitsToFloat(ssbo_1.data[c_base + 0u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 1u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 2u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 3u]));
    vec4 sm1 = vec4(uintBitsToFloat(ssbo_1.data[c_base + 4u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 5u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 6u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 7u]));
    vec4 sm2 = vec4(uintBitsToFloat(ssbo_1.data[c_base + 8u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 9u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 10u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 11u]));
    vec4 sm3 = vec4(uintBitsToFloat(ssbo_1.data[c_base + 12u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 13u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 14u]),
                    uintBitsToFloat(ssbo_1.data[c_base + 15u]));

    float sx = sm0.x * in_x + (sm0.y * in_y + (sm0.z * d + sm0.w));
    float sy = sm1.x * in_x + (sm1.y * in_y + (sm1.z * d + sm1.w));
    float sz = sm2.x * in_x + (sm2.y * in_y + (sm2.z * d + sm2.w));
    float sw = sm3.x * in_x + (sm3.y * in_y + (sm3.z * d + sm3.w));

    float inv_sw = 1.0 / sw;
    float dref = sz * inv_sw;
    float da = sw * uintBitsToFloat(ssbo_2.data[b1 + 74u]);
    float db = sw * uintBitsToFloat(ssbo_2.data[b1 + 75u]);

    float s0 = textureLod(sampler2DShadow(fs_img1, fs_samp1), vec3((sx + db) * inv_sw, (sy - da) * inv_sw, dref), 0.0);
    float s1 = textureLod(sampler2DShadow(fs_img1, fs_samp1), vec3((sx + da) * inv_sw, (sy + db) * inv_sw, dref), 0.0);
    float s2 = textureLod(sampler2DShadow(fs_img1, fs_samp1), vec3((sx - da) * inv_sw, (sy - db) * inv_sw, dref), 0.0);
    float s3 = textureLod(sampler2DShadow(fs_img1, fs_samp1), vec3((sx - db) * inv_sw, (sy + da) * inv_sw, dref), 0.0);

    float vx = (r0.x * in_x + (r0.y * in_y + (r0.z * d + r0.w))) * inv_w;
    float vy = (r1.x * in_x + (r1.y * in_y + (r1.z * d + r1.w))) * inv_w;
    float dist = sqrt(vx * vx + (vy * vy + vz * vz));

    float fade_end   = uintBitsToFloat(ssbo_2.data[b1 + 72u]);
    float fade_scale = uintBitsToFloat(ssbo_2.data[b1 + 73u]);
    float fade = clamp((fade_end - dist) * fade_scale, 0.0, 1.0);
    float pcf_avg = (s0 + s1 + s2 + s3) * 0.25;

    const bool enhanced_shadows = ((push_data.buf_offsets2.y >> 24u) & 0x02u) != 0u;
    if (enhanced_shadows && d < 0.9999) {
        const float ign = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
        const float base_ang = ign * 6.28318530718;
        const float rad_step = max(abs(da), abs(db)) * 1.35;
        if (rad_step > 0.0) {
            float vogel_sum = pcf_avg * 4.0;
            for (uint i = 0u; i < 8u; ++i) {
                float r = sqrt((float(i) + 0.5) * 0.125) * rad_step;
                float theta = float(i) * 2.39996323 + base_ang;
                vec2 off = vec2(cos(theta), sin(theta)) * r;
                vogel_sum += textureLod(sampler2DShadow(fs_img1, fs_samp1),
                                        vec3((sx + off.x) * inv_sw, (sy + off.y) * inv_sw, dref), 0.0);
            }
            pcf_avg = vogel_sum * (1.0 / 12.0);
        }

        // Screen-space depth-raymarched contact micro-shadows along the exact directional light vector
        // L_screen = cross(sm0.xyz, sm1.xyz) where delta_sx == 0, delta_sy == 0, and delta_sz < 0.
        if (cascade <= 1u && pcf_avg > 0.01) {
            vec3 l_ray = cross(sm0.xyz, sm1.xyz);
            float l_dz = dot(l_ray, sm2.xyz);
            if (l_dz > 0.0) {
                l_ray = -l_ray;
                l_dz = -l_dz;
            }
            // Convert NDC step (in_x, in_y) to UV step (in_x * 0.5, -in_y * 0.5).
            vec2 uv_dir = vec2(l_ray.x * 0.5, -l_ray.y * 0.5);
            float uv_len = length(uv_dir);
            if (uv_len > 1e-6 && abs(l_dz) > 1e-7) {
                // March ~16 pixels across 8 steps (2.0 px/step at 1080p) with IGN phase jitter.
                const float max_uv_dist = 0.012;
                const float scale = (max_uv_dist / 8.0) / uv_len;
                const vec2 step_uv = uv_dir * scale;
                const float step_d = l_ray.z * scale;
                // Depth thickness proportional to per-step depth change and view distance.
                const float thickness = max(abs(step_d) * 3.5, 0.00035 / max(abs(vz), 1.0));
                float contact_vis = 1.0;
                for (uint s = 1u; s <= 8u; ++s) {
                    float t = float(s) - 0.65 + ign * 0.65;
                    vec2 sample_uv = fs_in_attr1.xy + step_uv * t;
                    float ray_d = d + step_d * t;
                    float scene_d = textureLod(sampler2D(fs_img0, fs_samp0), sample_uv, 0.0).r;
                    float diff = ray_d - scene_d;
                    if (diff > 0.000015 && diff < thickness) {
                        float w = 1.0 - (float(s - 1u) * 0.11);
                        contact_vis = min(contact_vis, 1.0 - 0.78 * w);
                    }
                }
                pcf_avg *= contact_vis;
            }
        }
    }

    float out_a = 1.0 - (fade + fade * (-pcf_avg));

    float f255   = fract(d * 255.0);
    float f65025 = fract(d * 65025.0);
    frag_color0 = vec4(f65025, f255 - f65025 * 0.00392156886, d - f255 * 0.00392156886, out_a);
}
