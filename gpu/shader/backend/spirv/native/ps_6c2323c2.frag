#version 460
#extension GL_EXT_samplerless_texture_functions : require
// Native Vulkan 1.3 GLSL replacement for 0x6c2323c2 (Filter\GXSsr.ppo)
// Two-Phase Adaptive Screen-Space Reflections Raymarcher (max 48-64 steps + 5-step Binary Search refinement)

layout(location = 0) in vec4 fs_in_attr0_p;
layout(location = 1) in vec4 fs_in_attr1_p;

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

layout(set = 0, binding = 0, std430) readonly buffer ssbo_1_buf { uint data[]; } ssbo_1;

layout(set = 0, binding = 1) uniform texture2D fs_img0;
layout(set = 0, binding = 2) uniform texture2D fs_img1;
layout(set = 0, binding = 3) uniform texture2D fs_img2;
layout(set = 0, binding = 4) uniform sampler fs_samp0;
layout(set = 0, binding = 5) uniform sampler fs_samp1;
layout(set = 0, binding = 6) uniform sampler fs_samp2;

float safe_rsq(float x) {
    return (x > 0.0 && x <= 3.40282347e+38) ? inversesqrt(max(x, 1.17549435e-38)) : 0.0;
}

float load_cb(uint base, uint idx) {
    return uintBitsToFloat(ssbo_1.data[base + idx]);
}

void main() {
    const uint b0 = bitfieldExtract(push_data.buf_offsets0.x, 0, 8) >> 2u;
    const vec2 ndc = fs_in_attr0_p.xy;
    const vec2 uv = fs_in_attr1_p.xy;
    const vec2 uv_origin = vec2(0.5 * ndc.x + 0.5, -0.5 * ndc.y + 0.5);
    const vec2 ddx_uv = dFdx(uv_origin);
    const vec2 ddy_uv = dFdy(uv_origin);

    const float depth = textureLod(sampler2D(fs_img0, fs_samp0), uv, 0.0).r;
    if (!(1.0 > depth)) {
        frag_color0 = vec4(0.0);
        return;
    }

    const vec3 gbuf_n_raw = texture(sampler2D(fs_img1, fs_samp1), uv).rgb * 2.0 - 1.0;
    const vec3 n_ws = gbuf_n_raw * safe_rsq(dot(gbuf_n_raw, gbuf_n_raw));

    const vec3 vrow0 = vec3(load_cb(b0, 32u), load_cb(b0, 33u), load_cb(b0, 34u));
    const vec3 vrow1 = vec3(load_cb(b0, 36u), load_cb(b0, 37u), load_cb(b0, 38u));
    const vec3 vrow2 = vec3(load_cb(b0, 40u), load_cb(b0, 41u), load_cb(b0, 42u));
    const vec3 n_vs = vec3(dot(vrow0, n_ws), dot(vrow1, n_ws), dot(vrow2, n_ws));

    const vec4 clip_pos = vec4(ndc.x, ndc.y, depth, 1.0);
    const vec4 iprow0 = vec4(load_cb(b0, 0u),  load_cb(b0, 1u),  load_cb(b0, 2u),  load_cb(b0, 3u));
    const vec4 iprow1 = vec4(load_cb(b0, 4u),  load_cb(b0, 5u),  load_cb(b0, 6u),  load_cb(b0, 7u));
    const vec4 iprow2 = vec4(load_cb(b0, 8u),  load_cb(b0, 9u),  load_cb(b0, 10u), load_cb(b0, 11u));
    const vec4 iprow3 = vec4(load_cb(b0, 12u), load_cb(b0, 13u), load_cb(b0, 14u), load_cb(b0, 15u));

    const float inv_w = 1.0 / dot(iprow3, clip_pos);
    const vec3 p_vs = vec3(dot(iprow0, clip_pos), dot(iprow1, clip_pos), dot(iprow2, clip_pos)) * inv_w;
    const vec3 v_vs = p_vs * safe_rsq(dot(p_vs, p_vs));

    const vec3 r_raw = v_vs - n_vs * (2.0 * dot(v_vs, n_vs));
    const vec3 r_vs = r_raw * safe_rsq(dot(r_raw, r_raw));

    const float min_reflect_z = load_cb(b0, 64u);
    if (!(min_reflect_z <= r_vs.z)) {
        frag_color0 = vec4(0.0);
        return;
    }

    const float dir_fade_lin = clamp(r_vs.z - min_reflect_z, 0.0, 1.0);
    const float dir_fade = dir_fade_lin * dir_fade_lin;
    const vec2 centered_uv = uv_origin - vec2(0.5);
    const float edge_dist = clamp(length(centered_uv) * 2.0, 0.0, 1.0);
    const float edge_fade = 1.0 - edge_dist * edge_dist;
    const float alpha_fade = min(1.0, min(dir_fade, edge_fade));
    if (alpha_fade <= 0.0) {
        frag_color0 = vec4(0.0);
        return;
    }

    const vec3 p_end = p_vs + r_vs;
    if (!(p_end.z > 0.0)) {
        frag_color0 = vec4(0.0);
        return;
    }

    const vec4 prow0 = vec4(load_cb(b0, 16u), load_cb(b0, 17u), load_cb(b0, 18u), load_cb(b0, 19u));
    const vec4 prow1 = vec4(load_cb(b0, 20u), load_cb(b0, 21u), load_cb(b0, 22u), load_cb(b0, 23u));
    const vec4 prow2 = vec4(load_cb(b0, 24u), load_cb(b0, 25u), load_cb(b0, 26u), load_cb(b0, 27u));

    const float inv_end_z = 1.0 / p_end.z;
    const vec4 p_end4 = vec4(p_end, 1.0);
    const float end_ndc_x = dot(prow0, p_end4) * inv_end_z;
    const float end_ndc_y = dot(prow1, p_end4) * inv_end_z;
    const float end_depth = dot(prow2, p_end4) * inv_end_z;

    const vec3 d_ndc = vec3(end_ndc_x - ndc.x, end_ndc_y - ndc.y, end_depth - depth);
    const vec3 d_ndc_unit = d_ndc * safe_rsq(dot(d_ndc, d_ndc));
    const vec3 d_uvz = vec3(0.5 * d_ndc_unit.x, -0.5 * d_ndc_unit.y, d_ndc_unit.z);
    const vec3 d_uvz_unit = d_uvz * safe_rsq(dot(d_uvz, d_uvz));

    const float raw_screen_steps = load_cb(b0, 72u);
    const float uv_len = length(d_uvz_unit.xy);
    if (!(raw_screen_steps > 0.0) || !(uv_len > 1e-6)) {
        frag_color0 = vec4(0.0);
        return;
    }

    // Standard SSR passes raw_screen_steps ~= 640 (StepScale = 2.0);
    // High-Precision Ultra SSR passes raw_screen_steps ~= 2560 (StepScale = 0.5).
    // Bound coarse raymarching to 48 steps (Standard) or 64 steps (Ultra) with smooth
    // quadratic stride acceleration + 5-step Binary Search refinement upon intersection.
    const bool ultra_mode = (raw_screen_steps > 1000.0);
    const int max_steps = ultra_mode ? 64 : 48;
    const float eff_screen_steps = ultra_mode ? min(raw_screen_steps, 960.0) : min(raw_screen_steps, 640.0);

    const float min_thickness   = load_cb(b0, 76u);
    const float thickness_scale = load_cb(b0, 77u);
    const float max_ray_dist    = load_cb(b0, 78u);
    const float init_offset     = load_cb(b0, 79u);
    const float depth_param0    = load_cb(b0, 68u);
    const float depth_param1    = load_cb(b0, 69u);

    const vec3 base_step = (d_uvz_unit / (eff_screen_steps * uv_len)) * (1.0 + init_offset);

    vec3 prev_pos = vec3(uv_origin, depth);
    vec3 ray_pos = prev_pos + base_step;
    bool hit = false;
    vec3 hit_lo = prev_pos;
    vec3 hit_hi = ray_pos;
    float hit_scene_depth = depth;

    for (int i = 0; i < max_steps; ++i) {
        if (ray_pos.x < 0.0 || ray_pos.x > 1.0 || ray_pos.y < 0.0 || ray_pos.y > 1.0 || ray_pos.z >= 1.0) {
            break;
        }
        const float denom = depth_param0 - ray_pos.z;
        if (denom != 0.0) {
            const float linear_z = -depth_param1 / denom;
            if (linear_z > max_ray_dist) {
                break;
            }
        }

        const float scene_depth = textureLod(sampler2D(fs_img0, fs_samp0), ray_pos.xy, 0.0).r;
        const float depth_diff = ray_pos.z - scene_depth;
        const float step_dz = abs(ray_pos.z - prev_pos.z);
        const float step_thickness = max(min_thickness, thickness_scale * step_dz) + step_dz;

        if (depth_diff >= 0.0 && depth_diff < step_thickness) {
            hit = true;
            hit_lo = prev_pos;
            hit_hi = ray_pos;
            hit_scene_depth = scene_depth;
            break;
        }

        prev_pos = ray_pos;
        const float fi = float(i + 1);
        const float stride = 1.0 + 0.05 * fi + 0.004 * (fi * fi);
        ray_pos += base_step * stride;
    }

    if (!hit) {
        frag_color0 = vec4(0.0);
        return;
    }

    // Phase 2: 5-step Binary Search refinement across [hit_lo, hit_hi] (1/32 step sub-pixel accuracy)
    for (int b = 0; b < 5; ++b) {
        const vec3 mid_pos = 0.5 * (hit_lo + hit_hi);
        const float mid_depth = textureLod(sampler2D(fs_img0, fs_samp0), mid_pos.xy, 0.0).r;
        if (mid_pos.z >= mid_depth) {
            hit_hi = mid_pos;
            hit_scene_depth = mid_depth;
        } else {
            hit_lo = mid_pos;
        }
    }

    const float final_thickness = max(min_thickness, thickness_scale * abs(base_step.z));
    if ((hit_hi.z - hit_scene_depth) > final_thickness * 2.0) {
        frag_color0 = vec4(0.0);
        return;
    }

    const vec4 hit_clip = vec4(2.0 * hit_hi.x - 1.0, 1.0 - 2.0 * hit_hi.y, hit_scene_depth, 1.0);
    const vec4 rrow0 = vec4(load_cb(b0, 48u), load_cb(b0, 49u), load_cb(b0, 50u), load_cb(b0, 51u));
    const vec4 rrow1 = vec4(load_cb(b0, 52u), load_cb(b0, 53u), load_cb(b0, 54u), load_cb(b0, 55u));
    const vec4 rrow3 = vec4(load_cb(b0, 60u), load_cb(b0, 61u), load_cb(b0, 62u), load_cb(b0, 63u));

    const float reproj_w_rcp = 1.0 / dot(rrow3, hit_clip);
    const vec2 reproj_ndc = vec2(dot(rrow0, hit_clip), dot(rrow1, hit_clip)) * reproj_w_rcp;
    const vec2 reproj_uv = vec2(0.5 * reproj_ndc.x + 0.5, -0.5 * reproj_ndc.y + 0.5);

    const vec3 prev_rgb = textureGrad(sampler2D(fs_img2, fs_samp2), reproj_uv, ddx_uv, ddy_uv).rgb;
    if (any(isnan(prev_rgb))) {
        frag_color0 = vec4(0.0);
        return;
    }

    frag_color0 = vec4(prev_rgb * alpha_fade, alpha_fade);
}
