// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: Rasterizer, render targets: special passes (fast clear elimination, resolve, depth
// copies), target lookup, the render pass (BeginRendering) and the UI placement.

#include "render/direct_targets.h"
#include "render/raster_internal.h"

namespace Vulkan {

namespace {

/// What a draw is: a draw, nothing, or a pass the color/depth control registers ask for.
enum class DrawPass {
    Draw,
    Skip,               ///< FMask decompression (nothing to do), or no primitive type
    EliminateFastClear, ///< CB_COLOR_CONTROL.MODE: writes the fast clear color
    Resolve,            ///< MRT0 resolved into MRT1
    DepthStencilCopy,   ///< no color, forced dirty/valid depth or stencil to another surface
};

struct DrawPassInfo {
    DrawPass pass;
    bool depth_copy = false;
    bool stencil_copy = false;
};

DrawPassInfo DrawPassOf(const Gcn::Regs& regs) {
    using Mode = Gcn::ColorControl::OperationMode;
    switch (regs.color_control.mode) {
    case Mode::EliminateFastClear:
        return {DrawPass::EliminateFastClear};
    case Mode::FmaskDecompress:
        return {DrawPass::Skip};
    case Mode::Resolve:
        return {DrawPass::Resolve};
    default:
        break;
    }
    if (regs.primitive_type == Gcn::PrimitiveType::None) {
        return {DrawPass::Skip};
    }
    // Games copy one depth/stencil surface to another with color off and the forced dirty and
    // valid bits, without a pixel shader: the copy is done here.
    const auto& db = regs.depth_buffer;
    const auto& force = regs.depth_render_override;
    const bool depth_copy = force.force_z_dirty && force.force_z_valid && db.DepthValid() &&
                            db.DepthWriteValid() && db.DepthAddress() != db.DepthWriteAddress();
    const bool stencil_copy = force.force_stencil_dirty && force.force_stencil_valid &&
                              db.StencilValid() && db.StencilWriteValid() &&
                              db.StencilAddress() != db.StencilWriteAddress();
    if (regs.color_control.mode == Mode::Disable && (depth_copy || stencil_copy)) {
        return {DrawPass::DepthStencilCopy, depth_copy, stencil_copy};
    }
    return {DrawPass::Draw};
}

} // namespace

bool Rasterizer::FilterDrawPasses() const {
    return DrawPassOf(Regs()).pass == DrawPass::Draw;
}

bool Rasterizer::FilterDraw() {
    const auto info = DrawPassOf(Regs());
    switch (info.pass) {
    case DrawPass::Draw:
        return true;
    case DrawPass::Skip:
        break;
    case DrawPass::EliminateFastClear:
        EliminateFastClear();
        break;
    case DrawPass::Resolve:
        Resolve();
        break;
    case DrawPass::DepthStencilCopy:
        DepthStencilCopy(info.depth_copy, info.stencil_copy);
        break;
    }
    return false;
}

template <typename... Parts>
VideoCore::ImageId Rasterizer::FindTargetMemoized(VideoCore::TextureCache::ImageDesc& desc,
                                                  LastTarget& last, auto&& make_desc,
                                                  const Parts&... parts) {
    static_assert((sizeof(Parts) + ...) <= 256, "target memo key too large");
    std::array<u8, 256> key{};
    u32 key_size = 0;
    const auto append = [&](const auto& part) {
        std::memcpy(key.data() + key_size, &part, sizeof(part));
        key_size += sizeof(part);
    };
    (append(parts), ...);
    const u64 generation = texture_cache.RegistryGeneration();
    const bool memo_enabled = !BbToggle::Disabled(BbToggle::TextureBindingMemo);
    if (memo_enabled && last.generation == generation && last.key_size == key_size &&
        std::memcmp(last.key.data(), key.data(), key_size) == 0) {
        last.changed = false;
        texture_cache.MarkFound(last.image_id);
        return last.image_id;
    }
    last.changed = true;
    const auto remember = [&](VideoCore::ImageId image_id, u32 slot_idx) {
        last.key = key;
        last.key_size = key_size;
        last.memo_slot = slot_idx;
        last.generation = generation;
        last.image_id = image_id;
    };
    u64 hash = 0xCBF29CE484222325ull;
    for (u32 i = 0; i < key_size; i += 8) {
        u64 word;
        std::memcpy(&word, key.data() + i, 8);
        hash = (hash ^ word) * 0x100000001B3ull;
        hash ^= hash >> 29;
    }
    const u32 slot_idx = static_cast<u32>(hash % target_memo.size());
    auto& memo = target_memo[slot_idx];
    if (memo.generation == generation && memo.key_size == key_size &&
        std::memcmp(memo.key.data(), key.data(), key_size) == 0 && memo_enabled) {
        desc = memo.desc;
        texture_cache.MarkFound(memo.image_id);
        remember(memo.image_id, slot_idx);
        return memo.image_id;
    }
    make_desc();
    const VideoCore::ImageId image_id = texture_cache.FindImage(desc);
    if (generation == texture_cache.RegistryGeneration()) {
        memo.key = key;
        memo.key_size = key_size;
        memo.generation = generation;
        memo.image_id = image_id;
        memo.desc = desc;
        memo.color_view_memo = {};
        memo.depth_view_memo = {};
        remember(image_id, slot_idx);
    } else {
        last.memo_slot = ~0u;
        last.generation = ~0ULL;
    }
    return image_id;
}

VideoCore::ImageId Rasterizer::ResolveDirectColorTarget(
    u16 uid, const Gcn::ColorBuffer& col_buf, Gcn::CbDbExtent hint,
    VideoCore::TextureCache::ImageDesc& out_desc) {
    if (uid == 0u || uid >= DirectTargets::kMaxTargets) {
        return {};
    }
    DirectTargets::TargetSnapshot snap{};
    if (!DirectTargets::ReadTargetSlot(uid, snap) || snap.is_depth) {
        return {};
    }
    const auto* raw = reinterpret_cast<const u32*>(&col_buf);
    constexpr u32 kColorInfoIdentityMask = 0x80001FFFu;
    if (raw[0] != snap.raw_regs[0] || raw[1] != snap.raw_regs[1] ||
        raw[2] != snap.raw_regs[2] || raw[3] != snap.raw_regs[3] ||
        ((raw[4] ^ snap.raw_regs[4]) & kColorInfoIdentityMask) != 0u ||
        raw[5] != snap.raw_regs[5] || hint.raw != snap.raw_regs[6] ||
        raw[7] != snap.raw_regs[7] || raw[9] != snap.raw_regs[9]) {
        return {};
    }

    auto& cached = direct_target_cache[uid];
    const u64 generation = texture_cache.RegistryGeneration();
    const bool memo_enabled = !BbToggle::Disabled(BbToggle::TextureBindingMemo);
    if (memo_enabled && cached.slot_epoch == snap.epoch &&
        cached.registry_generation == generation && cached.image_id) {
        out_desc = cached.desc;
        texture_cache.MarkFound(cached.image_id);
        return cached.image_id;
    }

    if (snap.has_parent_tsharp && (cached.slot_epoch != snap.epoch || !cached.parent_ensured)) {
        Gcn::Image parent_img{};
        std::memcpy(&parent_img, snap.parent_tsharp.data(), sizeof(parent_img));
        if (parent_img.Valid()) {
            Shader::ImageResource parent_res{};
            const auto img_type = parent_img.GetType();
            parent_res.is_array = (img_type == Gcn::ImageType::Color1DArray ||
                                   img_type == Gcn::ImageType::Color2DArray ||
                                   img_type == Gcn::ImageType::Color2DMsaaArray);
            VideoCore::TextureCache::ImageDesc parent_desc(parent_img, parent_res);
            if (parent_desc.info.guest_address != 0 &&
                parent_desc.info.pixel_format != vk::Format::eUndefined) {
                (void)texture_cache.FindImage(parent_desc);
            }
        }
        cached.parent_ensured = true;
    }

    std::construct_at(&cached.desc, col_buf, hint);
    const u64 gen_before = texture_cache.RegistryGeneration();
    const VideoCore::ImageId id = texture_cache.FindImage(cached.desc);
    cached.image_id = id;
    cached.slot_epoch = snap.epoch;
    cached.color_view_memo = {};
    cached.registry_generation =
        (gen_before == texture_cache.RegistryGeneration()) ? gen_before : ~0ULL;
    out_desc = cached.desc;
    return id;
}

VideoCore::ImageId Rasterizer::ResolveDirectDepthTarget(
    u16 uid, const Gcn::DepthBuffer& depth_buf, Gcn::DepthView depth_view,
    Gcn::DepthControl depth_ctrl, VAddr htile_address, Gcn::CbDbExtent hint,
    VideoCore::TextureCache::ImageDesc& out_desc) {
    if (uid == 0u || uid >= DirectTargets::kMaxTargets) {
        return {};
    }
    DirectTargets::TargetSnapshot snap{};
    if (!DirectTargets::ReadTargetSlot(uid, snap) || !snap.is_depth) {
        return {};
    }
    const auto* raw = reinterpret_cast<const u32*>(&depth_buf);
    const u32 view_raw = std::bit_cast<u32>(depth_view);
    if (raw[0] != snap.raw_regs[0] || raw[1] != snap.raw_regs[1] ||
        raw[2] != snap.raw_regs[2] || raw[3] != snap.raw_regs[3] ||
        raw[6] != snap.raw_regs[6] || raw[7] != snap.raw_regs[7] ||
        view_raw != snap.raw_regs[8] || hint.raw != snap.raw_regs[12] ||
        (VAddr(snap.raw_regs[9]) << 8) != htile_address) {
        return {};
    }

    auto& cached = direct_target_cache[uid];
    const u64 generation = texture_cache.RegistryGeneration();
    const bool memo_enabled = !BbToggle::Disabled(BbToggle::TextureBindingMemo);
    if (memo_enabled && cached.slot_epoch == snap.epoch &&
        cached.registry_generation == generation && cached.image_id) {
        out_desc = cached.desc;
        out_desc.view_info.is_storage = bool(depth_ctrl.depth_write_enable);
        texture_cache.MarkFound(cached.image_id);
        return cached.image_id;
    }

    std::construct_at(&cached.desc, depth_buf, depth_view, depth_ctrl, htile_address, hint);
    const u64 gen_before = texture_cache.RegistryGeneration();
    const VideoCore::ImageId id = texture_cache.FindImage(cached.desc);
    cached.image_id = id;
    cached.slot_epoch = snap.epoch;
    cached.depth_view_memo = {};
    cached.registry_generation =
        (gen_before == texture_cache.RegistryGeneration()) ? gen_before : ~0ULL;
    out_desc = cached.desc;
    return id;
}

void Rasterizer::PrepareRenderState(const GraphicsPipeline* pipeline) {
    BB_SECTION(PrepareRenderState);
    // Prefetch render targets to handle overlaps with bound textures (e.g. mipgen)
    const auto& key = pipeline->GetGraphicsKey();
    const auto& regs = Regs();
    if (regs.color_control.degamma_enable) {
        LOG_WARNING(Render_Vulkan, "Color buffers require gamma correction");
    }

    const bool skip_cb_binding =
        regs.color_control.mode == Gcn::ColorControl::OperationMode::Disable;
    const s32 num_cb = std::bit_width(key.mrt_mask);
    const bool want_depth =
        (regs.depth_control.depth_enable && regs.depth_buffer.DepthValid()) ||
        (regs.depth_control.stencil_enable && regs.depth_buffer.StencilValid());
    const VAddr htile_address = want_depth ? regs.depth_htile_data_base.GetAddress() : 0;
    const u64 cur_gen = texture_cache.RegistryGeneration();

    u8 active_cb_mask = 0;
    if (!skip_cb_binding) {
        for (s32 cb = 0; cb < num_cb; ++cb) {
            if ((key.mrt_mask & (1u << cb)) != 0 && regs.color_buffers[cb] &&
                regs.color_target_mask.GetMask(cb) != 0) {
                active_cb_mask |= static_cast<u8>(1u << cb);
            }
        }
    }

    // V3.1 Per-Pass Fast Path (~95% of draws): inline L1 register signature match with zero
    // 256-byte stack zeroing or FNV-1a hashing across consecutive draws in the same render pass.
    bool fast_hit = !BbToggle::Disabled(BbToggle::TextureBindingMemo) &&
                    prepared_pass.valid &&
                    prepared_pass.generation == cur_gen &&
                    prepared_pass.mrt_mask == key.mrt_mask &&
                    prepared_pass.skip_cb == skip_cb_binding &&
                    prepared_pass.active_cb_mask == active_cb_mask &&
                    prepared_pass.has_depth == want_depth;
    if (fast_hit) {
        constexpr u32 kColorInfoIdentityMask = 0x80001FFFu;
        u8 m = active_cb_mask;
        while (m != 0 && fast_hit) {
            const u32 cb = static_cast<u32>(std::countr_zero(m));
            m &= static_cast<u8>(m - 1u);
            const auto& cur = regs.color_buffers[cb];
            const auto& prev = prepared_pass.cb_buf[cb];
            const auto id = cb_descs[cb].first;
            fast_hit = bool(id) &&
                       !texture_cache.GetImage(id).binding.needs_rebind &&
                       CbExtent(cb).raw == prepared_pass.cb_extent[cb].raw &&
                       cur.base_address == prev.base_address &&
                       std::memcmp(&cur, &prev, 4 * sizeof(u32)) == 0 &&
                       ((cur.info.raw ^ prev.info.raw) & kColorInfoIdentityMask) == 0u &&
                       cur.attrib.raw == prev.attrib.raw &&
                       cur.cmask_base_address == prev.cmask_base_address &&
                       cur.fmask_base_address == prev.fmask_base_address;
        }
        if (fast_hit && want_depth) {
            const auto id = db_desc.first;
            fast_hit = bool(id) &&
                       !texture_cache.GetImage(id).binding.needs_rebind &&
                       DbExtent().raw == prepared_pass.db_extent.raw &&
                       htile_address == prepared_pass.htile_addr &&
                       std::bit_cast<u32>(regs.depth_view) == prepared_pass.depth_view_raw &&
                       std::memcmp(&regs.depth_buffer, &prepared_pass.depth_buf,
                                   sizeof(Gcn::DepthBuffer)) == 0;
        }
    }

    if (fast_hit) {
        for (s32 cb = 0; cb < num_cb; ++cb) {
            if (const auto id = cb_descs[cb].first) {
                bound_images.emplace_back(id);
                texture_cache.MarkFound(id);
                texture_cache.GetImage(id).binding.is_target = 1u;
            }
        }
        if (want_depth) {
            db_desc.second.view_info.is_storage = bool(regs.depth_control.depth_write_enable);
            const auto id = db_desc.first;
            bound_images.emplace_back(id);
            texture_cache.MarkFound(id);
            texture_cache.GetImage(id).binding.is_target = 1u;
        }
    } else {
        for (s32 cb = 0; cb < num_cb; ++cb) {
            auto& [image_id, desc] = cb_descs[cb];
            if ((active_cb_mask & (1u << cb)) == 0) {
                image_id = {};
                active_cb_uid[cb] = 0;
                continue;
            }
            const auto& col_buf = regs.color_buffers[cb];
            const auto& hint = CbExtent(cb);
            const u16 uid = CbUid(cb);
            VideoCore::ImageId resolved = ResolveDirectColorTarget(uid, col_buf, hint, desc);
            if (resolved) {
                active_cb_uid[cb] = uid;
            } else {
                active_cb_uid[cb] = 0;
                const u32 tag = 1;
                resolved = FindTargetMemoized(
                    desc, last_targets[cb], [&] { std::construct_at(&desc, col_buf, hint); }, tag,
                    col_buf, hint);
                if (last_targets[cb].changed) {
                    fallback_cb_view_memo[cb] = {};
                }
            }
            image_id = bound_images.emplace_back(resolved);
            auto& image = texture_cache.GetImage(image_id);
            image.binding.is_target = 1u;
            prepared_pass.cb_buf[cb] = col_buf;
            prepared_pass.cb_extent[cb] = hint;
        }

        if (want_depth) {
            const auto& hint = DbExtent();
            auto& [image_id, desc] = db_desc;
            const u16 uid = DbUid();
            VideoCore::ImageId resolved = ResolveDirectDepthTarget(
                uid, regs.depth_buffer, regs.depth_view, regs.depth_control, htile_address, hint,
                desc);
            if (resolved) {
                active_db_uid = uid;
            } else {
                active_db_uid = 0;
                const u32 tag = 2;
                resolved = FindTargetMemoized(
                    desc, last_targets[Gcn::NUM_COLOR_BUFFERS],
                    [&] {
                        std::construct_at(&desc, regs.depth_buffer, regs.depth_view,
                                          regs.depth_control, htile_address, hint);
                    },
                    tag, regs.depth_buffer, regs.depth_view, regs.depth_control, htile_address,
                    hint);
                if (last_targets[Gcn::NUM_COLOR_BUFFERS].changed) {
                    fallback_db_view_memo = {};
                }
            }
            image_id = bound_images.emplace_back(resolved);
            auto& image = texture_cache.GetImage(image_id);
            image.binding.is_target = 1u;
            prepared_pass.depth_buf = regs.depth_buffer;
            prepared_pass.depth_view_raw = std::bit_cast<u32>(regs.depth_view);
            prepared_pass.htile_addr = htile_address;
            prepared_pass.db_extent = hint;
        } else {
            db_desc.first = {};
            active_db_uid = 0;
        }

        prepared_pass.mrt_mask = key.mrt_mask;
        prepared_pass.skip_cb = skip_cb_binding;
        prepared_pass.active_cb_mask = active_cb_mask;
        prepared_pass.has_depth = want_depth;
        prepared_pass.generation = texture_cache.RegistryGeneration();
        prepared_pass.valid = (cur_gen == prepared_pass.generation);
    }
    // bbport: the G-buffer pass (5+ color targets) holds the scene depth, and its constants the
    // main camera (shadow passes bind the same layout with the light's camera).
    gbuffer_draw = camera_motion->Enabled() && std::popcount(key.mrt_mask) >= 5 && db_desc.first;
    if (gbuffer_draw) {
        const auto& vp = regs.viewports[0];
        camera_motion->OnGBufferPass(db_desc.first, vp.xscale < 0.0f ? -1.0f : 1.0f,
                                     vp.yscale < 0.0f ? -1.0f : 1.0f);
    }
    if (upscaler->Enabled() && cb_descs[0].first) {
        upscaler->OnColorTarget(cb_descs[0].first);
    }
    // bbport: scene color: a full-size RGBA16F target drawn with the scene depth.
    if (upscaler->Enabled() && db_desc.first && db_desc.first == camera_motion->Depth() &&
        cb_descs[0].first && std::popcount(key.mrt_mask) <= 2) {
        const auto& color = texture_cache.GetImage(cb_descs[0].first);
        const auto& depth = texture_cache.GetImage(db_desc.first);
        if (color.info.pixel_format == vk::Format::eR16G16B16A16Sfloat &&
            color.info.size.width == depth.info.size.width &&
            color.info.size.height == depth.info.size.height) {
            upscaler->OnSceneColor(cb_descs[0].first);
            // Blended geometry without depth writes (not full-screen passes): transparents and
            // effects start. Blended layers writing depth (decals, wet/blood ground) are
            // surfaces and stay in the history.
            if (regs.blend_control[0].enable && !regs.color_buffers[0].info.blend_bypass &&
                !regs.depth_control.depth_write_enable &&
                (regs.num_indices > 6 || regs.num_instances.NumInstances() > 1)) {
                upscaler->OnBlendedSceneDraw();
            } else if (regs.num_indices <= 6 && regs.num_instances.NumInstances() <= 1) {
                // A full-screen pass over the scene (the fog composite): the transparents are in.
                upscaler->OnSceneComposite();
            }
        }
    }
}

void Rasterizer::EliminateFastClear() {
    // The fast clear color is written where the CMASK says the target is still cleared.
    const auto& col_buf = Regs().color_buffers[0];
    if (!col_buf || !col_buf.info.fast_clear) {
        return;
    }
    VideoCore::TextureCache::ImageDesc desc(col_buf, CbExtent(0));
    const auto image_id = texture_cache.FindImage(desc);
    texture_cache.FindRenderTarget(image_id, desc);
    const VAddr cmask = col_buf.CmaskAddress();
    if (!texture_cache.IsMetaCleared(cmask, col_buf.view.slice_start)) {
        return;
    }
    for (u32 slice = col_buf.view.slice_start; slice <= col_buf.view.slice_max; ++slice) {
        texture_cache.TouchMeta(cmask, slice, false);
    }
    const VAddr target = col_buf.Address();
    ScopeMarker("EliminateFastClear:MRT={:#x}:M={:#x}", fmt::make_format_args(target, cmask), [&] {
                    runtime.ClearImage(&texture_cache.GetImage(image_id), desc.view_info.range,
                                       GcnToVk::ColorBufferClearValue(col_buf));
                });
}

Rasterizer::BeginSignature Rasterizer::MakeBeginSignature(const GraphicsPipeline* pipeline) const {
    const auto& key = pipeline->GetGraphicsKey();
    const auto& regs = Regs();
    BeginSignature sig;
    const u32 num_color = std::bit_width(key.mrt_mask);
    for (u32 cb = 0; cb < num_color && cb < Gcn::NUM_COLOR_BUFFERS; ++cb) {
        sig.ids[cb] = cb_descs[cb].first;
        if (cb_descs[cb].first) {
            sig.views[cb] = cb_descs[cb].second.view_info;
        }
        sig.color_samples[cb] = key.color_samples[cb];
    }
    sig.ids[Gcn::NUM_COLOR_BUFFERS] = db_desc.first;
    if (db_desc.first) {
        sig.views[Gcn::NUM_COLOR_BUFFERS] = db_desc.second.view_info;
    }
    sig.mrt_mask = key.mrt_mask;
    sig.num_samples = key.num_samples;
    sig.motion = key.motion_vectors;
    sig.scene_started = scene_started;
    sig.raster_scaling = upscaler->RasterScaling();
    sig.upscaler_state = upscaler->RedirectState();
    sig.generation = texture_cache.RegistryGeneration();
    std::memcpy(&sig.depth_control, &regs.depth_control, sizeof(u32));
    sig.depth_valid = regs.depth_buffer.DepthValid();
    sig.stencil_valid = regs.depth_buffer.StencilValid();
    sig.stencil_write = regs.WritesStencil();
    sig.native_viewport = UiComposition::NativeViewport(regs.viewports[0].xscale * 2,
                                                        regs.viewports[0].yscale * 2) ||
                          (regs.IsClipDisabled() && !regs.viewport_control.xscale_enable &&
                           !regs.viewport_control.yscale_enable);
    return sig;
}

RenderState Rasterizer::BeginRendering(const GraphicsPipeline* pipeline) {
    BB_SECTION(BeginRendering);
    // bbport: RenderStateMemo (see BeginMemo).
    const bool memo_on = !BbToggle::Disabled(BbToggle::RenderStateMemo) && !FrameCapture::Active();
    static const bool verbose_stats = [] {
        const char* env = std::getenv("BB_VERBOSE_STATS");
        return env && env[0] == '1';
    }();
    if (verbose_stats && BbStats::enabled &&
        ((begin_memo_hits + begin_memo_misses) & 0x3FFFF) == 0x3FFFF) {
        std::printf("Render state memo: %llu hits, %llu misses; texture sets: %llu hits, %llu "
                    "misses\n",
                    static_cast<unsigned long long>(begin_memo_hits),
                    static_cast<unsigned long long>(begin_memo_misses),
                    static_cast<unsigned long long>(texture_set_hits),
                    static_cast<unsigned long long>(texture_set_misses));
        std::printf("  texture set misses: other key %llu, generation %llu, image check %llu, "
                    "other %llu\n",
                    (unsigned long long)texture_set_why[0], (unsigned long long)texture_set_why[1],
                    (unsigned long long)texture_set_why[2], (unsigned long long)texture_set_why[3]);
        std::printf("  scene proxies sampled directly: %llu bindings\n",
                    static_cast<unsigned long long>(proxy_samples));
        proxy_samples = 0;
        texture_set_why = {};
        begin_memo_hits = begin_memo_misses = texture_set_hits = texture_set_misses = 0;
    }
    BeginSignature signature;
    if (memo_on) {
        signature = MakeBeginSignature(pipeline);
        const auto& regs = Regs();
        bool usable = begin_memo.valid && scheduler.IsRenderingWith(begin_memo.state) &&
                      !regs.depth_render_control.depth_clear_enable &&
                      !regs.depth_render_control.stencil_clear_enable &&
                      signature == begin_memo.signature;
        for (u32 i = 0; i < signature.ids.size(); ++i) {
            const auto id = signature.ids[i];
            if (usable && id) {
                const auto& image = texture_cache.GetImage(id);
                const bool is_readonly_depth =
                    (i == Gcn::NUM_COLOR_BUFFERS) && !db_desc.second.view_info.is_storage &&
                    !(image.info.props.has_stencil && regs.WritesStencil());
                usable = (!image.binding.is_bound || is_readonly_depth) &&
                         !image.binding.needs_rebind;
            }
        }
        if (usable) {
            ++begin_memo_hits;
            attachment_feedback_loop = false;
            push_data.scene_size = begin_memo.scene_size;
            target_scale = begin_memo.target_scale;
            ui_placement = begin_memo.ui_placement;
            return begin_memo.state;
        }
        ++begin_memo_misses;
    }
    begin_memo.valid = false;
    RenderState state = BeginRenderingFull(pipeline);
    if (memo_on && !attachment_feedback_loop) {
        const auto& regs = Regs();
        const bool reg_clear = regs.depth_render_control.depth_clear_enable ||
                               regs.depth_render_control.stencil_clear_enable;
        const bool depth_layout_shifted =
            (state.depth_stencil_attachment.depth_clear && !db_desc.second.view_info.is_storage) ||
            (state.depth_stencil_attachment.stencil_clear && !db_desc.second.view_info.is_storage &&
             !regs.WritesStencil());
        if (!reg_clear && !depth_layout_shifted) {
            RenderState memo_state = state;
            for (u32 i = 0; i < memo_state.num_color_attachments; ++i) {
                memo_state.color_attachments[i].is_clear = 0;
                memo_state.color_attachments[i].clear_value = {};
            }
            memo_state.depth_stencil_attachment.depth_clear = false;
            memo_state.depth_stencil_attachment.stencil_clear = false;
            memo_state.depth_stencil_attachment.clear_value = {};
            // Recomputed after the full path: it may have started the scene (scene_started).
            begin_memo = {true, MakeBeginSignature(pipeline), memo_state, push_data.scene_size,
                          target_scale, ui_placement};
        }
    }
    return state;
}

bool Rasterizer::ReducedPass(const GraphicsPipeline* pipeline,
                             std::optional<SceneResolution::Size>& pass_size) {
    const auto& key = pipeline->GetGraphicsKey();
    // A proxy attachment cannot represent MSAA or a feedback loop that reads the same image
    // through the guest's native descriptor during this draw.
    bool reduced = scene_started && upscaler->RasterScaling() && key.num_samples == 1;
    // BB_SCENE_DEBUG=<file>: once the file exists, why the passes of one frame keep the native
    // size (see NoteFrameStart).
    const bool debug_pass = scene_debug_frame && scene_started;
    std::string why;
    if (debug_pass) {
        why = fmt::format("pass vs {:08x} ps {:08x} mrt {:#x} samples {} raster_scaling {}:",
                          pipeline->GetStage(Shader::SwStage::Vertex).pgm_hash,
                          key.mrt_mask ? pipeline->GetStage(Shader::SwStage::Fragment).pgm_hash
                                       : 0,
                          key.mrt_mask, u32(key.num_samples), upscaler->RasterScaling());
    }
    // Only this pass's attachments: slots past the mask's width keep earlier passes' targets,
    // often the G-buffer images the lighting passes sample (bound), which kept those native.
    const u32 num_attachments = BbToggle::Disabled(BbToggle::SceneAttachmentsOnly)
                                    ? u32(cb_descs.size())
                                    : u32(std::bit_width(key.mrt_mask));
    // All attachments of a reduced pass share one proxy size (scene or half resolution).
    const auto same_size = [&](const VideoCore::Image& image, u32 level) {
        const auto proxy = scene_targets->ProxySize(image, level);
        if (!pass_size) {
            pass_size = proxy;
        }
        return *pass_size == proxy;
    };
    for (u32 cb = 0; cb < num_attachments; ++cb) {
        const auto& [id, desc] = cb_descs[cb];
        if (!id) {
            continue;
        }
        const auto& image = texture_cache.GetImage(id);
        const auto& range = desc.view_info.range;
        // Mip levels of eligible chains have their own proxies (SceneTargets::Get).
        if (!scene_targets->Eligible(image) || image.binding.is_bound ||
            image.binding.needs_rebind || range.base.layer ||
            !same_size(image, range.base.level)) {
            reduced = false;
        }
        if (debug_pass) {
            why += fmt::format(" [{} {}x{} eligible {} bound {} rebind {} level {} layer {}]",
                               vk::to_string(image.info.pixel_format), image.info.size.width,
                               image.info.size.height, scene_targets->Eligible(image),
                               u32(image.binding.is_bound), u32(image.binding.needs_rebind),
                               range.base.level, range.base.layer);
        }
    }
    if (db_desc.first && (!scene_targets->Eligible(texture_cache.GetImage(db_desc.first)) ||
                          !same_size(texture_cache.GetImage(db_desc.first), 0))) {
        reduced = false;
    }
    if (debug_pass) {
        if (db_desc.first) {
            const auto& depth = texture_cache.GetImage(db_desc.first);
            why += fmt::format(" depth [{} {}x{} eligible {}]",
                               vk::to_string(depth.info.pixel_format), depth.info.size.width,
                               depth.info.size.height, scene_targets->Eligible(depth));
        }
        std::printf("Scene pass: reduced %d %s\n", reduced, why.c_str());
    }
    if (BbStats::enabled) {
        BbStats::reduced_draws.fetch_add(reduced, std::memory_order_relaxed);
        BbStats::scene_draws.fetch_add(scene_started, std::memory_order_relaxed);
    }
    return reduced;
}

void Rasterizer::BindColorAttachments(RenderState& state, const GraphicsPipelineKey& key,
                                      bool reduced, TargetRedirect& redirect) {
    const auto& regs = Regs();
    state.num_color_attachments = std::bit_width(key.mrt_mask);
    for (u32 cb = 0; cb < state.num_color_attachments; ++cb) {
        auto& [image_id, desc] = cb_descs[cb];
        auto& attachment = state.color_attachments[cb];
        if (!image_id) {
            attachment = {};
            continue;
        }
        auto* image = &texture_cache.GetImage(image_id);
        if (image->binding.needs_rebind) {
            last_targets[cb].generation = ~0ULL; // FindImage may rewrite the description
            last_targets[cb].memo_slot = ~0u;
            fallback_cb_view_memo[cb] = {};
            prepared_pass.valid = false;
            if (active_cb_uid[cb] != 0 && active_cb_uid[cb] < direct_target_cache.size()) {
                direct_target_cache[active_cb_uid[cb]].registry_generation = ~0ULL;
            }
            image_id = bound_images.emplace_back(texture_cache.FindImage(desc));
            image = &texture_cache.GetImage(image_id);
        }
        texture_cache.UpdateImage(image_id);
        runtime.SetBackingSamples(image, key.color_samples[cb]);
        VideoCore::TextureCache::ViewMemo* view_memo = nullptr;
        if (active_cb_uid[cb] != 0 && active_cb_uid[cb] < direct_target_cache.size()) {
            view_memo = &direct_target_cache[active_cb_uid[cb]].color_view_memo;
        } else {
            const auto& last = last_targets[cb];
            if (last.memo_slot < target_memo.size()) {
                auto& tm = target_memo[last.memo_slot];
                if (tm.generation == last.generation && tm.key_size == last.key_size &&
                    std::memcmp(tm.key.data(), last.key.data(), last.key_size) == 0) {
                    view_memo = &tm.color_view_memo;
                }
            }
            if (!view_memo) {
                view_memo = &fallback_cb_view_memo[cb];
            }
        }
        const auto& view = texture_cache.FindRenderTarget(image_id, desc, view_memo);
        const u32 slice = view.info.range.base.layer;
        const u32 mip = view.info.range.base.level;
        const auto& col_buf = regs.color_buffers[cb];
        const bool is_clear = texture_cache.IsMetaCleared(col_buf.CmaskAddress(), slice);
        texture_cache.TouchMeta(col_buf.CmaskAddress(), slice, false);

        constexpr auto ColorAccess =
            vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
        if (reduced) {
            // The proxy is drawn; the native image is resolved when something reads it.
        } else if (image->binding.is_bound) {
            // Also sampled by this draw: a feedback loop.
            ASSERT_MSG(!image->binding.force_general,
                       "Having image both as storage and render target is unsupported");
            runtime.FlushBarriers();
            needs_barrier |= runtime.Transit(
                image,
                instance.IsAttachmentFeedbackLoopLayoutSupported()
                    ? vk::ImageLayout::eAttachmentFeedbackLoopOptimalEXT
                    : vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput, ColorAccess);
            attachment_feedback_loop = true;
        } else {
            needs_barrier |= runtime.Transit(image, vk::ImageLayout::eColorAttachmentOptimal,
                                             vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                             ColorAccess, desc.view_info.range);
        }
        state.width = std::min<u32>(state.width, std::max(image->info.size.width >> mip, 1u));
        state.height = std::min<u32>(state.height, std::max(image->info.size.height >> mip, 1u));
        state.num_layers = std::min<u32>(state.num_layers, view.info.range.extent.layers);

        const auto clear_value =
            is_clear ? GcnToVk::ColorBufferClearValue(col_buf) : vk::ClearValue{};
        attachment.image_view = *view.image_view;
        attachment.image_layout = image->backing->state.layout;
        attachment.clear_value = clear_value.color.uint32;
        attachment.is_clear = is_clear;
        if (reduced) {
            const auto target = scene_targets->Attachment(image_id, desc.view_info);
            attachment.image_view = target.view;
            attachment.image_layout = target.layout;
        }
        image->usage.render_target = 1u;
        if (cb == 0 && upscaler->Enabled()) {
            redirect.color = upscaler->RedirectColor(image_id, desc.view_info, redirect.target);
            if (redirect.color) {
                redirect.guest_extent = {image->info.size.width, image->info.size.height};
                redirect.original_color = {attachment.image_view, attachment.image_layout};
                attachment.image_view = redirect.target.view;
                attachment.image_layout = redirect.target.layout;
            }
        }
    }
    for (u32 cb = state.num_color_attachments; cb < state.color_attachments.size(); ++cb) {
        state.color_attachments[cb] = {};
    }
}

void Rasterizer::BindDepthAttachment(RenderState& state, bool reduced, TargetRedirect& redirect) {
    auto& attachment = state.depth_stencil_attachment;
    const auto image_id = db_desc.first;
    if (!image_id) {
        attachment = {};
        return;
    }
    const auto& regs = Regs();
    const auto& desc = db_desc.second;
    const auto htile_address = regs.depth_htile_data_base.GetAddress();
    const u32 depth_memo_idx = desc.view_info.is_storage ? 1u : 0u;
    VideoCore::TextureCache::ViewMemo* view_memo = nullptr;
    if (active_db_uid != 0 && active_db_uid < direct_target_cache.size()) {
        view_memo = &direct_target_cache[active_db_uid].depth_view_memo[depth_memo_idx];
    } else {
        const auto& last = last_targets[Gcn::NUM_COLOR_BUFFERS];
        if (last.memo_slot < target_memo.size()) {
            auto& tm = target_memo[last.memo_slot];
            if (tm.generation == last.generation && tm.key_size == last.key_size &&
                std::memcmp(tm.key.data(), last.key.data(), last.key_size) == 0) {
                view_memo = &tm.depth_view_memo[depth_memo_idx];
            }
        }
        if (!view_memo) {
            view_memo = &fallback_db_view_memo[depth_memo_idx];
        }
    }
    const auto& view = texture_cache.FindDepthTarget(image_id, desc, view_memo);
    auto& image = texture_cache.GetImage(image_id);
    const u32 slice = view.info.range.base.layer;
    const bool is_depth_clear =
        (regs.depth_render_control.depth_clear_enable && regs.depth_control.depth_enable &&
         regs.depth_control.depth_write_enable) ||
        texture_cache.IsMetaCleared(htile_address, slice);
    const bool is_stencil_clear = regs.depth_render_control.stencil_clear_enable;
    texture_cache.TouchMeta(htile_address, slice, false);
    ASSERT(desc.view_info.range.extent.levels == 1 && !image.binding.needs_rebind);

    // Read-only depth layouts unless it is also written as storage; stencil writes can be on
    // while depth writes are off. Pure read-only stencil tests (ops == Keep or write_mask == 0)
    // stay in eDepthStencilReadOnlyOptimal so deferred lighting draws sampling depth while
    // testing stencil never ping-pong layouts or split the render pass.
    const bool has_stencil = image.info.props.has_stencil;
    const bool storage = desc.view_info.is_storage || is_depth_clear;
    const bool stencil_write =
        has_stencil && !storage && (regs.WritesStencil() || is_stencil_clear);
    const bool readonly_depth = !storage && !stencil_write;
    const auto layout = storage ? (has_stencil ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                               : vk::ImageLayout::eDepthAttachmentOptimal)
                        : stencil_write ? vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
                                        : vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    const auto dst_stage =
        readonly_depth
            ? (vk::PipelineStageFlagBits2::eAllGraphics |
               vk::PipelineStageFlagBits2::eComputeShader)
            : (vk::PipelineStageFlagBits2::eEarlyFragmentTests |
               vk::PipelineStageFlagBits2::eLateFragmentTests);
    const auto dst_access =
        readonly_depth
            ? (vk::AccessFlagBits2::eDepthStencilAttachmentRead |
               vk::AccessFlagBits2::eShaderRead)
            : (vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
               vk::AccessFlagBits2::eDepthStencilAttachmentRead);
    if (!reduced) {
        needs_barrier |= runtime.Transit(&image, layout, dst_stage, dst_access,
                                         desc.view_info.range);
    }
    state.width = std::min<u32>(state.width, image.info.size.width);
    state.height = std::min<u32>(state.height, image.info.size.height);
    state.num_layers = std::min<u32>(state.num_layers, view.info.range.extent.layers);

    attachment.image_view = *view.image_view;
    attachment.image_layout = image.backing->state.layout;
    attachment.clear_value = {};
    attachment.is_clear = 0;
    if (regs.depth_buffer.DepthValid()) {
        attachment.clear_value[0] = is_depth_clear ? std::bit_cast<u32>(regs.depth_clear) : 0u;
        attachment.has_depth = true;
        attachment.depth_clear = is_depth_clear;
    }
    if (regs.depth_buffer.StencilValid()) {
        attachment.clear_value[1] = is_stencil_clear ? regs.stencil_clear : 0u;
        attachment.has_stencil = true;
        attachment.stencil_clear = is_stencil_clear;
    }
    if (reduced) {
        const auto target = scene_targets->Attachment(image_id, desc.view_info);
        attachment.image_view = target.view;
        attachment.image_layout = target.layout;
    }
    image.usage.depth_target = true;
    if (upscaler->Enabled()) {
        TemporalUpscaler::Target depth_target;
        redirect.depth = upscaler->RedirectDepth(image_id, depth_target);
        if (redirect.depth) {
            redirect.guest_extent = {image.info.size.width, image.info.size.height};
            attachment.image_view = depth_target.view;
            attachment.image_layout = depth_target.layout;
            redirect.target = depth_target;
        }
    }
}

void Rasterizer::ApplyTargetRedirect(RenderState& state, const TargetRedirect& redirect) {
    // A pass drawn into the upscaler's output-size images (UI, display pass): every attachment
    // must be redirected; viewports and scissors are scaled.
    target_scale = {1.0f, 1.0f};
    ui_placement = {};
    if (!redirect.color && !redirect.depth) {
        return;
    }
    u32 color_targets = 0;
    for (u32 cb = 0; cb < state.num_color_attachments; ++cb) {
        color_targets += state.color_attachments[cb].image_view ? 1 : 0;
    }
    const bool has_depth = bool(db_desc.first);
    if ((color_targets > 0 && !redirect.color) || (has_depth && !redirect.depth) ||
        color_targets > 1) {
        static u32 warned = 0;
        if (warned < 8) {
            ++warned;
            const auto describe = [&](VideoCore::ImageId id) {
                if (!id) {
                    return std::string{"-"};
                }
                const auto& info = texture_cache.GetImage(id).info;
                return fmt::format("{:#x} {}x{} {}", info.guest_address, info.size.width,
                                   info.size.height, vk::to_string(info.pixel_format));
            };
            std::printf("Upscaler: pass not redirected (color %s: %s, depth %s: %s, %u "
                        "targets)\n",
                        redirect.color ? "yes" : "no", describe(cb_descs[0].first).c_str(),
                        redirect.depth ? "yes" : "no", describe(db_desc.first).c_str(),
                        state.num_color_attachments);
        }
        // Drawn at the game's size after all.
        if (redirect.color) {
            state.color_attachments[0].image_view = redirect.original_color.first;
            state.color_attachments[0].image_layout = redirect.original_color.second;
        }
        if (redirect.depth) {
            auto& depth_image = texture_cache.GetImage(db_desc.first);
            state.depth_stencil_attachment.image_view =
                *texture_cache.FindDepthTarget(db_desc.first, db_desc.second).image_view;
            state.depth_stencil_attachment.image_layout = depth_image.backing->state.layout;
        }
        return;
    }
    const auto& regs = Regs();
    state.width = redirect.target.width;
    state.height = redirect.target.height;
    // The UI movie viewport is 1920x1080 even when the scene is 960x540: a native viewport (or
    // native window-space UI vertices) must not be doubled again. The final display copy still
    // uses the guest render-size coordinates.
    const auto& viewport = regs.viewports[0];
    const bool native_coordinates =
        redirect.target.native_ui &&
        (UiComposition::NativeViewport(viewport.xscale * 2, viewport.yscale * 2) ||
         (regs.IsClipDisabled() && !regs.viewport_control.xscale_enable &&
          !regs.viewport_control.yscale_enable));
    target_scale = UiComposition::Scale(redirect.guest_extent.first, redirect.guest_extent.second,
                                        redirect.target.width, redirect.target.height,
                                        native_coordinates);
    if (const auto edges = UiComposition::ScreenEdges(redirect.target.width, redirect.target.height);
        native_coordinates && (edges[0] < 1.0f || edges[1] < 1.0f)) {
        ui_placement = {edges[0], edges[1]};
    }
}

RenderState Rasterizer::BeginRenderingFull(const GraphicsPipeline* pipeline) {
    attachment_feedback_loop = false;
    const auto& key = pipeline->GetGraphicsKey();
    // The G-buffer pass of an eligible scene starts the scene (reduced to the proxies from
    // here on, BB_SCENE_RESOLUTION).
    if (std::popcount(key.mrt_mask & 0x7f) >= 5 && db_desc.first &&
        scene_targets->EligibleScene(texture_cache.GetImage(db_desc.first))) {
        scene_started = true;
    }
    std::optional<SceneResolution::Size> pass_size;
    const bool reduced = ReducedPass(pipeline, pass_size);
    push_data.scene_size = reduced ? SceneResolution::Pack(scene_targets->Size()) : 0;

    RenderState state;
    state.width = instance.GetMaxFramebufferWidth();
    state.height = instance.GetMaxFramebufferHeight();
    state.num_layers = std::numeric_limits<u16>::max();
    TargetRedirect redirect{};
    BindColorAttachments(state, key, reduced, redirect);
    BindDepthAttachment(state, reduced, redirect);
    ApplyTargetRedirect(state, redirect);
    if (state.num_layers == std::numeric_limits<u16>::max()) {
        state.num_layers = 1;
    }
    if (state.num_color_attachments == 0 && !state.depth_stencil_attachment.has_depth &&
        !state.depth_stencil_attachment.has_stencil) {
        const auto size = scene_targets ? scene_targets->Size() : SceneResolution::Size{1920, 1080};
        state.width = size.width ? size.width : 1920;
        state.height = size.height ? size.height : 1080;
    }
    if (reduced) {
        // Half-resolution passes keep the scene's factor (proxy / native = scene / 1920).
        const auto size = scene_targets->Size();
        const auto proxy = pass_size.value_or(size);
        state.width = proxy.width;
        state.height = proxy.height;
        target_scale = {float(size.width) / 1920.0f, float(size.height) / 1080.0f};
    }
    if (FrameCapture::Active()) {
        std::array<const VideoCore::ImageInfo*, Gcn::NUM_COLOR_BUFFERS> colors{};
        for (u32 cb = 0; cb < state.num_color_attachments; ++cb) {
            if (cb_descs[cb].first) {
                colors[cb] = &texture_cache.GetImage(cb_descs[cb].first).info;
            }
        }
        const auto* depth = db_desc.first ? &texture_cache.GetImage(db_desc.first).info : nullptr;
        FrameCapture::BeginPass(colors.data(), state.num_color_attachments, depth);
    }
    if (key.motion_vectors && (upscaler->Enabled() || camera_motion->DebugOverlay())) {
        object_motion->Attach(state, state.width, state.height); // the G-buffer's motion target
    }
    return state;
}

void Rasterizer::UiEdgeStretch(const GraphicsPipeline* pipeline) {
    // BB_UI_DRAWS=<file>: while it exists, one frame's UI draws on another aspect ratio are
    // logged (then it is removed).
    static const char* trace_file = std::getenv("BB_UI_DRAWS");
    if (trace_file && ui_placement.edge_x != 0.0f) {
        static u64 traced_frame = ~0ull;
        const u64 frame = BbStats::frame_number.load(std::memory_order_relaxed);
        static bool tracing = false;
        if (frame != traced_frame) {
            if (tracing) {
                std::remove(trace_file);
            }
            tracing = false;
            if (std::FILE* f = std::fopen(trace_file, "r")) {
                std::fclose(f);
                tracing = true;
            }
            traced_frame = frame;
        }
        if (tracing) {
            const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
            const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
            const auto& vp = Regs().viewports[0];
            u32 tw = 0, th = 0;
            if (ps && !ps->images.empty()) {
                const auto tex = ps->images[0].GetSharp(*ps);
                tw = u32(tex.width) + 1;
                th = u32(tex.height) + 1;
            }
            const auto& sc = Regs().screen_scissor;
            const auto& wsc = Regs().window_scissor;
            const auto& vsc = Regs().viewport_scissors[0];
            std::printf("UI draw: vs %016llx ps %016llx images %zu (%ux%u) clip %d indices %u viewport "
                        "%.0fx%.0f at %.0f,%.0f scissors screen %d,%d-%d,%d window %d,%d-%d,%d vp %d,%d-%d,%d "
                        "vport_scissor %d\n", (unsigned long long)vs.pgm_hash,
                        (unsigned long long)(ps ? ps->pgm_hash : 0), ps ? ps->images.size() : 0, tw, th,
                        Regs().IsClipDisabled() ? 0 : 1, Regs().num_indices, vp.xscale * 2, vp.yscale * 2,
                        vp.xoffset, vp.yoffset, sc.top_left_x, sc.top_left_y, sc.bottom_right_x,
                        sc.bottom_right_y, wsc.top_left_x, wsc.top_left_y, wsc.bottom_right_x,
                        wsc.bottom_right_y, vsc.top_left_x, vsc.top_left_y, vsc.bottom_right_x,
                        vsc.bottom_right_y, Regs().mode_control.vport_scissor_enable ? 1 : 0);
        }
    }
    // Every vertex shader but the motion vectors' has the corner snap (EmitUiEdgeStretch). A
    // picture over the whole screen (the title's: 2048x1024) keeps its proportions, and so does
    // text (its glyph sheet is 2048x2048). Masked content (Scaleform's masks are stencil tests:
    // the HUD's item icons slide in a strip much taller than their slot) and the masks stay.
    const auto& regs = Regs();
    if (ui_placement.edge_x == 0.0f || regs.IsClipDisabled() ||
        pipeline->GetGraphicsKey().motion_vectors ||
        (regs.depth_control.stencil_enable && regs.depth_buffer.StencilValid())) {
        return;
    }
    const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
    // Experiment bit 32 (BB_TOGGLE_FILE, while the game runs): big textures stretched too.
    if (ps && !ps->images.empty() && !BbToggle::Experiment(32)) {
        const auto texture = ps->images[0].GetSharp(*ps);
        if (u32(texture.width) + 1 >= 2048 || u32(texture.height) + 1 >= 2048) {
            return;
        }
    }
    push_data.motion_param = UiComposition::EdgeParam(ui_placement.edge_x, ui_placement.edge_y,
                                                       regs.viewports[0].yscale > 0.0f);
}

void Rasterizer::Resolve() {
    const auto& regs = Regs();
    VideoCore::TextureCache::ImageDesc src_desc{regs.color_buffers[0], CbExtent(0)};
    VideoCore::TextureCache::ImageDesc dst_desc{regs.color_buffers[1], CbExtent(1)};
    auto& src = texture_cache.GetImage(texture_cache.FindImage(src_desc, true));
    auto& dst = texture_cache.GetImage(texture_cache.FindImage(dst_desc, true));
    const VAddr src_address = regs.color_buffers[0].Address();
    const VAddr dst_address = regs.color_buffers[1].Address();
    ScopeMarker("Resolve:MRT0={:#x}:MRT1={:#x}", fmt::make_format_args(src_address, dst_address),
                [&] {
                    runtime.ResolveImage(&src, &dst, src_desc.view_info.range,
                                         dst_desc.view_info.range);
                });
}

void Rasterizer::DepthStencilCopy(bool is_depth, bool is_stencil) {
    // From the depth/stencil surface read to the one written (DB_*_READ_BASE vs WRITE_BASE).
    const auto& regs = Regs();
    const auto& db = regs.depth_buffer;
    const auto desc = [&](bool write) {
        return VideoCore::TextureCache::ImageDesc(db, regs.depth_view, regs.depth_control,
                                                  regs.depth_htile_data_base.GetAddress(),
                                                  DbExtent(), write);
    };
    auto read_desc = desc(false);
    auto write_desc = desc(true);
    auto& read_image = texture_cache.GetImage(texture_cache.FindImage(read_desc));
    auto& write_image = texture_cache.GetImage(texture_cache.FindImage(write_desc));
    VideoCore::SubresourceRange range;
    range.base.layer = regs.depth_view.slice_start;
    range.extent.layers = regs.depth_view.NumSlices() - range.base.layer;
    const VAddr depth_read = db.DepthAddress(), stencil_read = db.StencilAddress();
    const VAddr depth_write = db.DepthWriteAddress(), stencil_write = db.StencilWriteAddress();
    ScopeMarker("DepthStencilCopy:DR={:#x}:SR={:#x}:DW={:#x}:SW={:#x}",
                fmt::make_format_args(depth_read, stencil_read, depth_write, stencil_write),
                [&] { runtime.CopyDepthStencil(&read_image, &write_image, range); });
}

} // namespace Vulkan
