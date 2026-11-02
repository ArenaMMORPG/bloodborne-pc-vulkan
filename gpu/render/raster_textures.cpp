// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: Rasterizer, texture and sampler binding with the memoized descriptions, lookups and texture
// sets.

#include <atomic>
#include <cstring>
#include <unordered_map>
#include <chrono>
#include "bbport_button_prompts.h"
#include <unordered_set>
#include <mutex>
#include "render/raster_internal.h"
#include "textures/direct_textures.h"
#include "../../our_cpp_100000_source/include/gx_direct_vulkan.h"

namespace Vulkan {

Rasterizer::ImageDescCacheEntry& Rasterizer::CachedImageDescEntry(const Gcn::Image& raw_sharp,
                                                                   const Shader::ImageResource& res,
                                                                   u64 hash) {
    Gcn::Image sharp = raw_sharp;
    reinterpret_cast<u32*>(&sharp)[7] = 0u;
    std::array<u64, 4> key;
    std::memcpy(key.data(), &sharp, sizeof(key));
    const u32 flags = u32(res.is_written) | u32(res.is_depth) << 1 | u32(res.is_array) << 2;
    // Two-way set associative: a frame binds a few thousand distinct T#s.
    auto* set = &image_desc_cache[(hash % (image_desc_cache.size() / 2)) * 2];
    const auto matches = [&](const ImageDescCacheEntry& e) {
        return e.flags == flags && e.sharp == key;
    };
    const bool disabled = BbToggle::Disabled(BbToggle::ImageDescCache);
    ImageDescCacheEntry* slot = nullptr;
    if (!disabled && matches(set[0])) {
        slot = &set[0];
    } else if (!disabled && matches(set[1])) {
        slot = &set[1];
    } else {
        // Replace the least recently used way that no binding of this call points at.
        slot = set[0].last_use <= set[1].last_use ? &set[0] : &set[1];
        if (slot->pinned == bind_epoch) {
            slot = slot == &set[0] ? &set[1] : &set[0];
        }
        if (slot->pinned == bind_epoch) {
            slot = &image_desc_overflow.emplace_back();
        }
    }
    slot->last_use = ++desc_use_counter;
    auto& entry = *slot;
    if (entry.flags != flags || entry.sharp != key ||
        BbToggle::Disabled(BbToggle::ImageDescCache)) {
        entry.desc = VideoCore::TextureCache::ImageDesc{sharp, res};
        entry.sharp = key;
        entry.flags = flags;
        entry.srv_gen8 = 0;
        entry.found_direct_immutable = false;
        entry.found_generation = ~0ULL;
        entry.found_uid = 0;
        entry.view_memo = {};
    }
    return entry;
}

Rasterizer::ImageDescCacheEntry& Rasterizer::ResolveImageDescEntry(
    const Gcn::Image& clean_sharp, u16 srv_uid, u8 srv_gen8, const Shader::ImageResource& res,
    const u64* prepared_hash) {
    if (srv_uid != 0 && srv_gen8 != 0 && native_srv_cache &&
        !BbToggle::Disabled(BbToggle::ImageDescCache)) {
        const u32 flags = u32(res.is_written) | (u32(res.is_depth) << 1) | (u32(res.is_array) << 2);
        auto& entry = native_srv_cache[srv_uid];
        if (entry.srv_gen8 == srv_gen8 && entry.flags == flags) [[likely]] {
            return entry;
        }
        if (entry.pinned != bind_epoch || bind_epoch == 0) {
            entry.desc = VideoCore::TextureCache::ImageDesc{clean_sharp, res};
            std::memcpy(entry.sharp.data(), &clean_sharp, sizeof(entry.sharp));
            entry.flags = flags;
            entry.srv_gen8 = srv_gen8;
            entry.found_direct_immutable = false;
            entry.found_generation = ~0ULL;
            entry.found_uid = 0;
            entry.view_memo = {};
            return entry;
        }
    }
    return prepared_hash ? CachedImageDescEntry(clean_sharp, res, *prepared_hash)
                         : CachedImageDescEntry(clean_sharp, res);
}

bool Rasterizer::TransitSampled(VideoCore::Image& image,
                                const VideoCore::TextureCache::ImageDesc& desc,
                                const VideoCore::Image::Binding& binding, bool is_storage) {
    constexpr auto ShaderStages =
        vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
    // Also a storage image of this draw (its own descriptor) or a render target (a feedback
    // loop): the whole image in the general or feedback loop layout. Depth images are never
    // storage, and a feedback loop makes no sense for them.
    if ((binding.force_general || binding.is_target) && !image.info.props.is_depth) {
        if (instance.IsAttachmentFeedbackLoopLayoutSupported() && image.binding.is_target) {
            return runtime.Transit(&image, vk::ImageLayout::eAttachmentFeedbackLoopOptimalEXT,
                                   vk::PipelineStageFlagBits2::eAllGraphics,
                                   vk::AccessFlagBits2::eShaderRead);
        }
        return runtime.Transit(&image, vk::ImageLayout::eGeneral,
                               ShaderStages,
                               vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);
    }
    if (is_storage) {
        return runtime.Transit(&image, vk::ImageLayout::eGeneral,
                               ShaderStages,
                               vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                               desc.view_info.range);
    }
    const auto read_access =
        image.info.props.is_depth
            ? (vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eShaderRead)
            : vk::AccessFlagBits2::eShaderRead;
    return runtime.Transit(&image,
                           image.info.props.is_depth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                                     : vk::ImageLayout::eShaderReadOnlyOptimal,
                           ShaderStages,
                           read_access, desc.view_info.range);
}

void Rasterizer::BindTextures(const Shader::Info& stage, const PreparedStage* prepared,
                              Shader::Backend::Bindings& binding, u32& write_index,
                              bool& barrier, bool on_helper) {
    BB_SECTION(BindTextures);
    const u32 first_image_idx = image_infos.size();
    TextureSet* set_slot = nullptr;
    u64 set_key = 0;
    std::array<Gcn::Image, TextureSet::MaxImages> local_image_sharps{};
    std::array<u64, TextureSet::MaxImages> local_image_hashes{};
    bool have_local_sharps = false;
    const u32* const stage_flat = stage.FlatUserData().data();
    if (!on_helper && !prepared && !stage.images.empty() &&
        stage.images.size() <= TextureSet::MaxImages &&
        !BbToggle::Disabled(BbToggle::TextureSetMemo) && !FrameCapture::Active()) {
        const u32 cnt = static_cast<u32>(stage.images.size());
        for (u32 i = 0; i < cnt; ++i) {
            const auto& res = stage.images[i];
            local_image_sharps[i] = res.GetSharpDirect(stage_flat);
            local_image_hashes[i] = ImageDescHash(local_image_sharps[i], res);
        }
        have_local_sharps = true;
    }
    const Gcn::Image* const eff_image_sharps =
        prepared ? prepared->image_sharps : (have_local_sharps ? local_image_sharps.data() : nullptr);
    const u64* const eff_image_hashes =
        prepared ? prepared->image_hashes : (have_local_sharps ? local_image_hashes.data() : nullptr);
    const u64 eff_precomputed_key = prepared ? prepared->image_set_key : 0;
    if (!on_helper &&
        BindTexturesFromSet(stage, eff_image_hashes, eff_precomputed_key, first_image_idx, barrier,
                            set_slot, set_key)) {
        // Descriptor writes: one per image (no mip arrays in a memoized set).
        for (u32 i = 0; i < stage.images.size(); ++i) {
            set_writes[write_index++] = vk::WriteDescriptorSet{
                .dstBinding = binding.unified++,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &image_infos[first_image_idx + i],
            };
        }
        BindSamplers(stage, prepared, binding, write_index);
        return;
    }
    image_bindings.clear();
    image_binding_entries.clear();
    image_desc_overflow.clear();
    image_desc_storage.clear();
    ++bind_epoch;
    // To emulate storing to explicit mip levels, build a descriptor array with each mip level.
    boost::container::small_vector<u32, 8> image_descriptor_array_sizes;
    // TextureSetMemo: what this call resolves, to remember when the set qualifies.
    std::array<TextureSetEntry, TextureSet::MaxImages> resolved{};
    bool set_ok = (set_key != 0);
    // SampleSceneProxies: bindings that may read a reduced scene proxy instead of the native
    // image (normalized sampling only; see Shader::ImageResource::needs_native).
    boost::container::small_vector<bool, 16> binding_proxy_ok;
    const bool sample_proxies = !BbToggle::Disabled(BbToggle::SampleSceneProxies) &&
                                !on_helper && scene_targets->Reduced();
    // An invalid T# binds a null descriptor.
    const auto bind_null = [&] {
        image_bindings.emplace_back(VideoCore::ImageId{}, &image_desc_storage.emplace_back());
        image_binding_entries.push_back(nullptr);
        binding_proxy_ok.push_back(false);
        image_descriptor_array_sizes.push_back(1);
    };

    u64 local_native_hits = 0;
    u64 local_native_misses = 0;
    u32 resolved_native_count = 0;

    for (u32 image_index = 0; image_index < stage.images.size(); ++image_index) {
        const auto& image_desc = stage.images[image_index];
        auto tsharp = eff_image_sharps
                          ? eff_image_sharps[image_index]
                          : image_desc.GetSharpDirect(stage_flat);
        u8 srv_gen8 = 0u;
        u16 srv_uid = VideoCore::DirectTextures::UnpackAndCleanSrv(tsharp, srv_gen8);
        // bbport: a hash lookup per texture per draw for a diagnostic only.
        static const bool warn_meta = std::getenv("BB_WARN_META_TEXTURE") != nullptr;
        if (warn_meta && texture_cache.IsMeta(tsharp.Address())) {
            LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a shader (texture)");
        }

        const auto data_fmt = tsharp.GetDataFmt();
        const auto num_fmt = tsharp.GetNumberFmt();
        if (tsharp.Address() == 0 || data_fmt == Gcn::DataFormat::FormatInvalid) {
            bind_null();
            continue;
        }

        if (!memory->IsValidGpuMapping(tsharp.Address(), 0) ||
            !IsKnownFormat(data_fmt, num_fmt)) {
            LOG_WARNING(Render_Vulkan,
                        "Rejecting invalid T# address={:#x}, pitch={}, width={}, "
                        "data_format={}, num_format={}",
                        tsharp.Address(), static_cast<u32>(tsharp.pitch),
                        static_cast<u32>(tsharp.width), static_cast<u32>(data_fmt),
                        static_cast<u32>(num_fmt));
            bind_null();
            continue;
        }

        static const bool n2_native_tex =
            gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN2_NativeTextures);
        if (srv_uid == 0 && n2_native_tex) {
            if (VideoCore::DirectTextures::RegisterNativeSrv(&tsharp, 0u, false) != 0) {
                srv_uid = VideoCore::DirectTextures::UnpackAndCleanSrv(tsharp, srv_gen8);
            }
        }

        if (srv_uid != 0) {
            ++local_native_hits;
            if (image_index < TextureSet::MaxImages) {
                ++resolved_native_count;
            }
        } else {
            ++local_native_misses;
        }

        const Shader::MipStorageFallbackMode mip_fallback_mode = image_desc.mip_fallback_mode;
        // Same as image_desc.NumBindings(stage), without fetching the T# again.
        const u32 num_bindings =
            mip_fallback_mode == Shader::MipStorageFallbackMode::DynamicIndex
                ? static_cast<u32>(tsharp.last_level - tsharp.base_level + 1)
                : 1u;
        const bool proxy_candidate = sample_proxies && num_bindings == 1 &&
                                     !image_desc.needs_native && !image_desc.is_written &&
                                     mip_fallback_mode == Shader::MipStorageFallbackMode::None;

        auto& desc_entry = ResolveImageDescEntry(
            tsharp, srv_uid, srv_gen8, image_desc,
            eff_image_hashes ? &eff_image_hashes[image_index] : nullptr);
        // BB_SCENE_DEBUG: proxied scene targets this binding reads at the native size, and why.
        const auto debug_native = [&](const VideoCore::Image& image) {
            if (!scene_debug_frame || proxy_candidate || !image.scene_proxy) return;
            std::printf("Scene native read: %s %016llx %s %ux%u needs_native %d written %d "
                        "bindings %u mip_fallback %d\n",
                        stage.sw_stage == Shader::SwStage::Compute ? "cs" : "gfx",
                        (unsigned long long)stage.pgm_hash,
                        vk::to_string(image.info.pixel_format).c_str(), image.info.size.width,
                        image.info.size.height, int(image_desc.needs_native),
                        int(image_desc.is_written), num_bindings, int(mip_fallback_mode));
        };
        // The image bound: a stencil image with an associated depth image is read through the
        // depth-stencil image; one bound twice and written is used in the general layout.
        const auto mark_bound = [&](VideoCore::ImageId& image_id) {
            auto* image = &texture_cache.GetImage(image_id);
            if (const auto depth_image_id = texture_cache.GetAssociatedDepth(*image)) {
                image_id = depth_image_id;
                image = &texture_cache.GetImage(image_id);
            }
            debug_native(*image);
            if (image->binding.is_bound) {
                image->binding.force_general |= image_desc.is_written;
            }
            image->binding.is_bound = 1u;
        };
        for (u32 i = 0; i < num_bindings; i++) {
            // bbport: a plain binding (no mip override) of the same T# resolves to the same image
            // while no image was registered or unregistered (or V3.2: while its DirectImmutable
            // asset slot remains live).
            const u64 current_gen = texture_cache.RegistryGeneration();
            const bool memo_valid =
                desc_entry.found_generation == current_gen ||
                (desc_entry.found_direct_immutable &&
                 texture_cache.IsRegisteredImmutable(desc_entry.found_id, desc_entry.found_uid));
            if (mip_fallback_mode == Shader::MipStorageFallbackMode::None && memo_valid &&
                !BbToggle::Disabled(BbToggle::TextureBindingMemo)) {
                desc_entry.found_generation = current_gen;
                desc_entry.pinned = bind_epoch;
                auto& [image_id, _] =
                    image_bindings.emplace_back(desc_entry.found_id, &desc_entry.found_desc);
                image_binding_entries.push_back(&desc_entry);
                binding_proxy_ok.push_back(proxy_candidate);
                texture_cache.MarkFound(image_id);
                mark_bound(image_id);
                continue;
            }
            auto& desc = image_desc_storage.emplace_back(desc_entry.desc);
            auto& [image_id, _] = image_bindings.emplace_back(VideoCore::ImageId{}, &desc);
            image_binding_entries.push_back(nullptr);
            binding_proxy_ok.push_back(proxy_candidate);

            if (mip_fallback_mode == Shader::MipStorageFallbackMode::ConstantIndex) {
                ASSERT(num_bindings == 1);
                desc.view_info.range.base.level += image_desc.constant_mip_index;
                desc.view_info.range.extent.levels = 1;
            } else if (mip_fallback_mode == Shader::MipStorageFallbackMode::DynamicIndex) {
                desc.view_info.range.base.level += i;
                desc.view_info.range.extent.levels = 1;
            }

            const u64 generation = texture_cache.RegistryGeneration();
            image_id = texture_cache.FindImage(desc);
            if (mip_fallback_mode == Shader::MipStorageFallbackMode::None &&
                generation == texture_cache.RegistryGeneration() &&
                desc_entry.pinned != bind_epoch) {
                auto& resolved_img = texture_cache.GetImageUntouched(image_id);
                desc_entry.found_generation = generation;
                desc_entry.found_id = image_id;
                desc_entry.found_uid = resolved_img.image_uid;
                desc_entry.found_direct_immutable =
                    VideoCore::True(resolved_img.flags & VideoCore::ImageFlagBits::DirectImmutable) &&
                    !texture_cache.GetAssociatedDepth(resolved_img);
                desc_entry.found_desc = desc;
                desc_entry.view_memo = {};
            }
            mark_bound(image_id);
        }

        image_descriptor_array_sizes.push_back(num_bindings);
    }

    // Second pass to re-bind images that were updated after binding
    for (u32 binding_index = 0; binding_index < image_bindings.size(); ++binding_index) {
        auto& [image_id, desc_ptr] = image_bindings[binding_index];
        auto* memo_entry = image_binding_entries[binding_index];
        const auto& desc = *desc_ptr;
        bool is_storage = desc.type == VideoCore::TextureCache::BindingType::Storage;
        if (!image_id) {
            image_infos.emplace_back(VK_NULL_HANDLE, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
        } else {
            if (auto& old_image = texture_cache.GetImage(image_id);
                old_image.binding.needs_rebind) {
                old_image.binding = {};
                auto& rebind_desc = image_desc_storage.emplace_back(desc);
                desc_ptr = &rebind_desc;
                image_id = texture_cache.FindImage(rebind_desc);
                memo_entry = nullptr;
                set_ok = false;
            }

            bound_images.emplace_back(image_id);

            auto& image = texture_cache.GetImage(image_id);
            auto& image_view = texture_cache.FindTexture(
                image_id, desc, memo_entry ? &memo_entry->view_memo : nullptr, !on_helper);
            const auto binding = image.binding;

            // BB_PROMPT_TRACE=1: a 32x32 texture holding a key guide icon's bytes, and the image
            // the cache found for it (once per address).
            if (static const bool trace = std::getenv("BB_PROMPT_TRACE") != nullptr;
                trace && desc.info.size.width <= 128 && desc.info.size.height <= 128) {
                static std::mutex traced_mutex;
                static std::unordered_set<VAddr> traced;
                const VAddr address = desc.info.guest_address;
                std::scoped_lock lock{traced_mutex};
                if (traced.insert(address).second) {
                    // Uncompressed ones are saved as they are in memory (out/present/tex_*.raw).
                    if (!desc.info.props.is_block && desc.info.num_bits == 32 &&
                        desc.info.size.width <= 64 && desc.info.size.height <= 64) {
                        char path[256];
                        std::snprintf(path, sizeof(path), "out/present/tex_%llx_%ux%u_t%u.raw",
                                      (unsigned long long)address, desc.info.size.width,
                                      desc.info.size.height, u32(desc.info.tile_mode));
                        if (FILE* f = std::fopen(path, "wb")) {
                            std::fwrite(reinterpret_cast<const void*>(address), 1,
                                        std::min<u32>(desc.info.guest_size, 64 * 64 * 4), f);
                            std::fclose(f);
                        }
                    }
                    std::printf("Prompt trace: bound %#llx %ux%u %s (icon %u) -> image %#llx %ux%u "
                                "levels %u flag %u at %llu ms\n",
                                (unsigned long long)address, desc.info.size.width,
                                desc.info.size.height, vk::to_string(desc.info.pixel_format).c_str(),
                                desc.info.guest_size >= 1024 ?
                                BbPrompts::IconOf(reinterpret_cast<const void*>(address), 1024) : 0,
                                (unsigned long long)image.info.guest_address, image.info.size.width,
                                image.info.size.height, image.info.resources.levels, u32(image.prompt_icon),
                                (unsigned long long)(std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now().time_since_epoch()).count()));
                }
            }
            // BB_UI_TEXTURE_LOG=<file>: when the file appears, every texture the UI draws of the
            // next 2 s bind (once each), for finding what a HUD element samples.
            if (static const char* ui_log = std::getenv("BB_UI_TEXTURE_LOG");
                ui_log && upscaler->Enabled() && upscaler->InUiPhase()) {
                static std::atomic<u64> until_ms{0};
                static std::mutex ui_mutex;
                static std::unordered_set<VAddr> ui_seen;
                const u64 now_ms = u64(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
                if (until_ms.load() == 0 && std::remove(ui_log) == 0) {
                    until_ms = now_ms + 2000;
                }
                if (now_ms < until_ms.load()) {
                    std::scoped_lock lock{ui_mutex};
                    if (ui_seen.insert(desc.info.guest_address).second) {
                        // BC7 atlases' contents as the GPU holds them (out/present).
                        if (desc.info.pixel_format == vk::Format::eBc7UnormBlock &&
                            image.info.size.width >= 512) {
                            static int dumped = 0;
                            scheduler.EndRendering();
                            const vk::Image vk_image = image.GetImage();
                            const u32 w = image.info.size.width, h = image.info.size.height;
                            const int index = dumped++;
                            scheduler.Record([this, vk_image, w, h, index](vk::CommandBuffer cmdbuf) {
                                Vulkan::DumpRaw(instance, scheduler, cmdbuf, vk_image, w, h, 1, index,
                                                "uiatlas", "bc7");
                            });
                        }
                        std::printf("UI texture: %#llx %ux%u levels %u %s tile %u -> image %#llx %ux%u\n",
                                    (unsigned long long)desc.info.guest_address, desc.info.size.width,
                                    desc.info.size.height, desc.info.resources.levels,
                                    vk::to_string(desc.info.pixel_format).c_str(),
                                    u32(desc.info.tile_mode),
                                    (unsigned long long)image.info.guest_address,
                                    image.info.size.width, image.info.size.height);
                    }
                }
            }
            // BB_PROMPT_TRACE=1: larger block-compressed textures rescanned for an icon's micro
            // tile (the game may copy icons into an atlas on the GPU), every 5 s per address.
            if (static const bool trace_atlas = std::getenv("BB_PROMPT_TRACE") != nullptr;
                trace_atlas && desc.info.props.is_block) {
                static std::mutex scan_mutex;
                static std::unordered_map<VAddr, u64> scanned;
                const VAddr address = desc.info.guest_address;
                const u64 now_ms = u64(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
                bool scan = false;
                {
                    std::scoped_lock lock{scan_mutex};
                    auto& last = scanned[address];
                    if (now_ms - last > 5000) {
                        last = now_ms;
                        scan = true;
                    }
                }
                if (scan) {
                    const auto* bytes = reinterpret_cast<const u8*>(address);
                    // Any 16-byte block of the cross icon (KG_Cancel, 2) with colour in it: blocks
                    // stay whole whatever the layout.
                    if (const u8* cancel = BbPrompts::IconBytes(2)) {
                        u32 hits = 0, first = 0;
                        for (u32 offset = 0; offset + 16 <= desc.info.guest_size; offset += 16) {
                            for (u32 b : {27u, 28u, 35u, 36u}) {
                                if (std::memcmp(bytes + offset, cancel + b * 16, 16) == 0) {
                                    if (!hits++) first = offset;
                                }
                            }
                        }
                        if (hits) {
                            std::printf("Prompt trace: %u cross blocks in bound %#llx %ux%u %s from %#x\n",
                                        hits, (unsigned long long)address, desc.info.size.width,
                                        desc.info.size.height,
                                        vk::to_string(desc.info.pixel_format).c_str(), first);
                        }
                    }
                    for (u32 offset = 0; offset + 1024 <= desc.info.guest_size; offset += 1024) {
                        if (const u32 icon = BbPrompts::IconOf(bytes + offset, 1024)) {
                            std::printf("Prompt trace: icon %u in bound %#llx %ux%u %s at %#x\n", icon,
                                        (unsigned long long)address, desc.info.size.width,
                                        desc.info.size.height,
                                        vk::to_string(desc.info.pixel_format).c_str(), offset);
                        }
                    }
                }
            }
            // bbport: one of the game's key guide icons, drawn by the port for the device in use
            // (vk_button_prompts.h). Not remembered in the texture set: the device may change.
            // A copy the GPU made (DMA) is recognised here, by its bytes in memory.
            if (image.MaybePromptIcon() && prompts->Active()) {
                image.prompt_icon =
                    u8(BbPrompts::IconOf(reinterpret_cast<const void*>(image.info.guest_address), 1024));
            }
            if (image.prompt_atlas && !is_storage && prompts->Active()) {
                if (const vk::ImageView atlas = prompts->AtlasView(runtime, image, desc.view_info)) {
                    image.usage.texture = 1u;
                    image_infos.emplace_back(VK_NULL_HANDLE, atlas, vk::ImageLayout::eShaderReadOnlyOptimal);
                    set_ok = false;
                    continue;
                }
            }
            if (image.prompt_icon && !is_storage && prompts->Active()) {
                if (const vk::ImageView icon = prompts->View(image.prompt_icon, desc.view_info)) {
                    image.usage.texture = 1u;
                    image_infos.emplace_back(VK_NULL_HANDLE, icon, vk::ImageLayout::eShaderReadOnlyOptimal);
                    set_ok = false;
                    continue;
                }
            }

            // bbport: a proxied scene image sampled with normalized coordinates reads the proxy:
            // no resample to the native size (the native image is not touched).
            if (binding_proxy_ok[binding_index] && !is_storage && !binding.force_general &&
                !binding.is_target &&
                !(upscaler->Enabled() && upscaler->RedirectsSampled(image_id))) {
                if (const auto proxy = scene_targets->SampleProxy(image, desc.view_info)) {
                    image.usage.texture = 1u;
                    image_infos.emplace_back(VK_NULL_HANDLE, proxy->view, proxy->layout);
                    if (set_ok && binding_index < resolved.size()) {
                        resolved[binding_index] = {image_id, true, false, proxy->view,
                                                   image.backing, image.image_uid,
                                                   desc.view_info.range};
                    }
                    ++proxy_samples;
                    continue;
                }
            }
            barrier |= TransitSampled(image, desc, binding, is_storage);
            image.usage.storage |= is_storage;
            image.usage.texture |= !is_storage;
            if (FrameCapture::Active()) {
                FrameCapture::Sampled(image.info, is_storage);
            }

            vk::ImageView view = *image_view.image_view;
            vk::ImageLayout layout = image.backing->state.layout;
            if (upscaler->Enabled()) {
                // bbport: the display pass reads the upscaled frame (scaled presets).
                upscaler->RedirectSampled(image_id, image_view.info, view, layout);
            }
            image_infos.emplace_back(VK_NULL_HANDLE, view, layout);
            if (set_ok && binding_index < resolved.size()) {
                const bool imm_ro =
                    !is_storage && !image.info.props.is_depth && !image.scene_proxy &&
                    !image.MaybePromptIcon() && !image.prompt_atlas &&
                    !VideoCore::True(image.flags & VideoCore::ImageFlagBits::GpuModified) &&
                    texture_cache.IsRegisteredImmutable(image_id, image.image_uid);
                resolved[binding_index] = {image_id, false, imm_ro, *image_view.image_view,
                                           image.backing, image.image_uid, desc.view_info.range};
                set_ok = !is_storage && !binding.force_general && !binding.is_target;
            }
        }
    }
    if (!on_helper && !bind_helper.Available()) {
        if (local_native_hits != 0) {
            BbStats::native_tex_hits.store(
                BbStats::native_tex_hits.load(std::memory_order_relaxed) + local_native_hits,
                std::memory_order_relaxed);
        }
        if (local_native_misses != 0) {
            BbStats::native_tex_misses.store(
                BbStats::native_tex_misses.load(std::memory_order_relaxed) + local_native_misses,
                std::memory_order_relaxed);
        }
    } else {
        if (local_native_hits != 0) {
            BbStats::native_tex_hits.fetch_add(local_native_hits, std::memory_order_relaxed);
        }
        if (local_native_misses != 0) {
            BbStats::native_tex_misses.fetch_add(local_native_misses, std::memory_order_relaxed);
        }
    }

    if (set_ok && image_bindings.size() == stage.images.size()) {
        if (!set_slot) {
            static_assert(((std::tuple_size_v<decltype(texture_sets)> / 2) &
                           ((std::tuple_size_v<decltype(texture_sets)> / 2) - 1)) == 0);
            auto* pair = &texture_sets[((set_key >> 1) & ((texture_sets.size() / 2) - 1)) * 2];
            TextureSet& victim =
                pair[0].key == 0
                    ? pair[0]
                    : (pair[1].key == 0 ? pair[1]
                                        : (pair[0].last_use <= pair[1].last_use ? pair[0] : pair[1]));
            ++texture_set_why[victim.key != 0 ? 0 : 3];
            ++texture_set_misses;
            victim.key = set_key;
            victim.stage = &stage;
            if (eff_image_hashes) {
                std::copy_n(eff_image_hashes, stage.images.size(), victim.hashes.begin());
            }
            set_slot = &victim;
        }
        // Remember the set: the same T#s resolve the same way while no image is (un)registered,
        // or across unrelated streaming registrations when all_direct_immutable holds.
        set_slot->last_use = ++texture_set_use_counter;
        set_slot->generation = texture_cache.RegistryGeneration();
        set_slot->scene_generation = scene_targets->Generation();
        set_slot->count = static_cast<u32>(stage.images.size());
        set_slot->native_count = static_cast<u8>(resolved_native_count);
        bool all_imm = true;
        u8 valid_cnt = 0;
        for (u32 k = 0; k < set_slot->count; ++k) {
            set_slot->entries[k] = resolved[k];
            if (resolved[k].id) {
                ++valid_cnt;
                if (resolved[k].proxy ||
                    !texture_cache.IsRegisteredImmutable(resolved[k].id, resolved[k].image_uid)) {
                    all_imm = false;
                }
            }
        }
        set_slot->valid_entries = valid_cnt;
        set_slot->all_direct_immutable = all_imm;
    } else if (set_slot) {
        set_slot->key = 0;
        set_slot->last_use = 0;
        set_slot->count = 0;
        set_slot->native_count = 0;
        set_slot->valid_entries = 0;
        set_slot->all_direct_immutable = false;
    }

    // One descriptor write per image resource: an array of single-level views for dynamic mip
    // indices.
    u32 info_index = first_image_idx;
    u32 binding_index = 0;
    for (const u32 array_size : image_descriptor_array_sizes) {
        const bool is_storage = image_bindings[binding_index].second->type ==
                                VideoCore::TextureCache::BindingType::Storage;
        set_writes[write_index++] = vk::WriteDescriptorSet{
            .dstBinding = binding.unified,
            .descriptorCount = array_size,
            .descriptorType =
                is_storage ? vk::DescriptorType::eStorageImage : vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_infos[info_index],
        };
        info_index += array_size;
        binding_index += array_size;
        binding.unified += array_size;
    }

    BindSamplers(stage, prepared, binding, write_index);
}

// bbport: the PS4 build samples most scene textures with little or no anisotropic filtering.
// Behind the temporal upscaler the output shows the detail of the output's mip level, and
// surfaces seen at a grazing angle (stairs, floors, walls along the view) stay blurred.
// Mipmapped linear samplers of graphics draws get BB_ANISO (16; 0 = the game's) times.
void Rasterizer::ForceAnisotropy(Gcn::Sampler& sharp, bool is_depth) const {
    static const int forced = [] {
        const char* value = std::getenv("BB_ANISO");
        return value ? std::atoi(value) : 16;
    }();
    if (!is_depth && !pipeline_is_compute) {
        // The game's filters, once per kind: min filter, mip filter, ratio.
        static std::atomic<u64> seen{0};
        const u32 kind = u32(sharp.xy_min_filter) * 3 * 5 + u32(sharp.mip_filter) * 5 +
                         u32(sharp.max_aniso);
        if (kind < 64 && !(seen.load(std::memory_order_relaxed) & (1ull << kind)) &&
            !(seen.fetch_or(1ull << kind, std::memory_order_relaxed) & (1ull << kind))) {
            std::printf("Sampler: game uses min filter %u, mip filter %u, anisotropy %.0fx\n",
                        u32(sharp.xy_min_filter), u32(sharp.mip_filter),
                        sharp.MaxAniso());
        }
    }
    if (forced <= 1 || is_depth || pipeline_is_compute || BbToggle::Disabled(BbToggle::ForcedAniso) ||
        sharp.mip_filter == Gcn::MipFilter::None ||
        (sharp.xy_min_filter != Gcn::Filter::Bilinear &&
         sharp.xy_min_filter != Gcn::Filter::AnisoLinear)) {
        return;
    }
    const auto ratio = forced >= 16 ? Gcn::AnisoRatio::Sixteen
        : forced >= 8 ? Gcn::AnisoRatio::Eight
        : forced >= 4 ? Gcn::AnisoRatio::Four : Gcn::AnisoRatio::Two;
    if (Gcn::IsAnisoFilter(sharp.xy_min_filter) && sharp.MaxAniso() >= forced) {
        return;
    }
    sharp.xy_min_filter = Gcn::Filter::AnisoLinear;
    if (sharp.xy_mag_filter == Gcn::Filter::Bilinear) {
        sharp.xy_mag_filter = Gcn::Filter::AnisoLinear;
    }
    sharp.max_aniso = ratio;
}

void Rasterizer::BindSamplers(const Shader::Info& stage, const PreparedStage* prepared,
                              Shader::Backend::Bindings& binding, u32& write_index) {
    if (stage.samplers.empty()) {
        return;
    }
    const auto bc_base = Regs().ta_bc_base;
    const u32 color_bias_bits = std::bit_cast<u32>(sampler_lod_bias);
    const bool aniso_off = BbToggle::Disabled(BbToggle::ForcedAniso);
    const u64 cur_gc_tick = texture_cache.GcTick();
    u64 local_native_hits = 0;
    u64 local_native_misses = 0;

    const u32* const stage_flat = stage.FlatUserData().data();
    for (u32 sampler_index = 0; sampler_index < stage.samplers.size(); ++sampler_index) {
        const auto& sampler = stage.samplers[sampler_index];
        auto ssharp = prepared ? prepared->sampler_sharps[sampler_index]
                               : sampler.GetSharpDirect(stage_flat);
        u16 smp_uid = VideoCore::DirectTextures::UnpackAndCleanSampler(ssharp);
        static const bool n2_native_tex =
            gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN2_NativeTextures);
        if (smp_uid == 0 && n2_native_tex) {
            if (VideoCore::DirectTextures::RegisterNativeSampler(&ssharp) != 0) {
                smp_uid = VideoCore::DirectTextures::UnpackAndCleanSampler(ssharp);
            }
        }
        vk::Sampler vk_sampler{};
        if (smp_uid != 0 && native_sampler_cache) {
            ++local_native_hits;
            auto& sc = native_sampler_cache[smp_uid];
            const u32 variant = sampler.is_depth ? 2u : (pipeline_is_compute ? 1u : 0u);
            if (!(sc.clean_sharp == ssharp) || sc.forced_aniso_off != aniso_off ||
                std::memcmp(&sc.bc_base, &bc_base, sizeof(bc_base)) != 0) {
                sc.clean_sharp = ssharp;
                sc.bias_bits = color_bias_bits;
                sc.bc_base = bc_base;
                sc.forced_aniso_off = aniso_off;
                sc.touched_ticks = {~0ULL, ~0ULL, ~0ULL};
                sc.handles = {};
            } else if (variant != 2u && sc.bias_bits != color_bias_bits) {
                sc.bias_bits = color_bias_bits;
                sc.touched_ticks[0] = ~0ULL;
                sc.touched_ticks[1] = ~0ULL;
                sc.handles[0] = VK_NULL_HANDLE;
                sc.handles[1] = VK_NULL_HANDLE;
            }
            const u64 last_tick = sc.touched_ticks[variant];
            if (sc.handles[variant] && last_tick <= cur_gc_tick && (cur_gc_tick - last_tick) < 8)
                [[likely]] {
                vk_sampler = sc.handles[variant];
            } else {
                auto eff_sharp = ssharp;
                ForceAnisotropy(eff_sharp, sampler.is_depth);
                vk_sampler = texture_cache.GetSampler(
                    eff_sharp, bc_base, sampler.is_depth,
                    sampler.is_depth ? 0.0f : sampler_lod_bias);
                sc.handles[variant] = vk_sampler;
                sc.touched_ticks[variant] = cur_gc_tick;
            }
        } else {
            ++local_native_misses;
            ForceAnisotropy(ssharp, sampler.is_depth);
            vk_sampler = texture_cache.GetSampler(
                ssharp, bc_base, sampler.is_depth, sampler.is_depth ? 0.0f : sampler_lod_bias);
        }
        image_infos.emplace_back(vk_sampler, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
        auto& set_write = set_writes[write_index++];
        set_write.dstSet = VK_NULL_HANDLE;
        set_write.dstBinding = binding.unified++;
        set_write.dstArrayElement = 0;
        set_write.descriptorCount = 1;
        set_write.descriptorType = vk::DescriptorType::eSampler;
        set_write.pImageInfo = &image_infos.back();
    }

    if (!bind_helper.Available()) {
        if (local_native_hits != 0) {
            BbStats::native_tex_hits.store(
                BbStats::native_tex_hits.load(std::memory_order_relaxed) + local_native_hits,
                std::memory_order_relaxed);
        }
        if (local_native_misses != 0) {
            BbStats::native_tex_misses.store(
                BbStats::native_tex_misses.load(std::memory_order_relaxed) + local_native_misses,
                std::memory_order_relaxed);
        }
    } else {
        if (local_native_hits != 0) {
            BbStats::native_tex_hits.fetch_add(local_native_hits, std::memory_order_relaxed);
        }
        if (local_native_misses != 0) {
            BbStats::native_tex_misses.fetch_add(local_native_misses, std::memory_order_relaxed);
        }
    }
}

bool Rasterizer::BindTexturesFromSet(const Shader::Info& stage, const u64* image_hashes,
                                     u64 precomputed_key, u32 first_image_idx, bool& barrier,
                                     TextureSet*& slot, u64& out_key) {
    out_key = 0;
    const u32 count = static_cast<u32>(stage.images.size());
    if (!image_hashes || count == 0 || count > TextureSet::MaxImages ||
        BbToggle::Disabled(BbToggle::TextureSetMemo) || FrameCapture::Active()) {
        return false;
    }
    u64 key = precomputed_key;
    if (key == 0) {
        key = XXH3_64bits_withSeed(image_hashes, count * sizeof(u64),
                                   reinterpret_cast<u64>(&stage)) |
              1ULL;
    }
    static_assert(((std::tuple_size_v<decltype(texture_sets)> / 2) &
                   ((std::tuple_size_v<decltype(texture_sets)> / 2) - 1)) == 0);
    // `key` always has bit 0 forced to 1 (`| 1ULL`), so shift right by 1 before masking so all
    // 8192 buckets (16384 ways) are utilized instead of only odd buckets.
    auto* pair = &texture_sets[((key >> 1) & ((texture_sets.size() / 2) - 1)) * 2];
    const u64 generation = texture_cache.RegistryGeneration();
    const u64 scene_gen = scene_targets->Generation();

    const auto key_matches = [&](const TextureSet& cand) {
        return cand.key == key && cand.stage == &stage &&
               std::equal(cand.hashes.begin(), cand.hashes.begin() + count, image_hashes);
    };
    TextureSet* matched = nullptr;
    if (key_matches(pair[0])) {
        matched = &pair[0];
    } else if (key_matches(pair[1])) {
        matched = &pair[1];
    }

    if (!matched) {
        for (const auto& image : stage.images) {
            if (image.is_written ||
                image.mip_fallback_mode != Shader::MipStorageFallbackMode::None) {
                return false;
            }
        }
        // Defer victim allocation until BindTextures confirms `set_ok` so uncacheable draws
        // (prompt icons/atlas, feedback targets, mip arrays) never evict live cached texture sets.
        out_key = key;
        slot = nullptr;
        return false;
    }

    out_key = key;
    TextureSet& set = *matched;
    set.last_use = ++texture_set_use_counter;
    slot = &set;

    bool gen_ok = (set.generation == generation);
    if (!gen_ok && set.count == count && set.all_direct_immutable) {
        bool all_live = true;
        for (u32 i = 0; i < count; ++i) {
            const auto& entry = set.entries[i];
            if (entry.id && !texture_cache.IsRegisteredImmutable(entry.id, entry.image_uid)) {
                all_live = false;
                break;
            }
        }
        if (all_live) {
            set.generation = generation;
            gen_ok = true;
        }
    }

    if (set.count != count || !gen_ok || set.scene_generation != scene_gen) {
        ++texture_set_why[set.count == 0 ? 3 : 1];
        ++texture_set_misses;
        set.count = 0;
        set.valid_entries = 0;
        set.generation = ~0ull;
        set.all_direct_immutable = false;
        return false;
    }

    const bool prompts_on = prompts->Active();
    const bool upscaler_on = upscaler->Enabled();
    const auto record_set_stats = [&]() noexcept {
        ++texture_set_hits;
        const u32 valid_entries = set.valid_entries;
        const u32 set_nat = std::min<u32>(set.native_count, valid_entries);
        if (!bind_helper.Available()) {
            if (set_nat != 0) {
                BbStats::native_tex_hits.store(
                    BbStats::native_tex_hits.load(std::memory_order_relaxed) + set_nat,
                    std::memory_order_relaxed);
            }
            if (valid_entries > set_nat) {
                BbStats::native_tex_misses.store(
                    BbStats::native_tex_misses.load(std::memory_order_relaxed) +
                        (valid_entries - set_nat),
                    std::memory_order_relaxed);
            }
        } else {
            if (set_nat != 0) {
                BbStats::native_tex_hits.fetch_add(set_nat, std::memory_order_relaxed);
            }
            if (valid_entries > set_nat) {
                BbStats::native_tex_misses.fetch_add(valid_entries - set_nat,
                                                     std::memory_order_relaxed);
            }
        }
        slot = nullptr;
    };

    // N3.2: Single-pass zero-write fast path when every non-null texture in the set is a settled
    // read-only DirectImmutable asset in ShaderReadOnlyOptimal (no LRU churn, no bound_images
    // write+clear, no Image cache-line dirtying, and no Runtime::Transit / GetBarriers calls).
    if (set.all_direct_immutable && !prompts_on && !upscaler_on) {
        bool fast_ok = true;
        for (u32 i = 0; i < count; ++i) {
            const auto& entry = set.entries[i];
            if (!entry.id) {
                image_infos.emplace_back(VK_NULL_HANDLE, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
                continue;
            }
            if (!entry.immutable_ro) {
                fast_ok = false;
                break;
            }
            const auto& image = texture_cache.GetImageUntouched(entry.id);
            if (image.backing != entry.backing || image.binding.needs_rebind ||
                image.binding.is_target ||
                VideoCore::True(image.flags &
                                (VideoCore::ImageFlagBits::Dirty |
                                 VideoCore::ImageFlagBits::GpuModified |
                                 VideoCore::ImageFlagBits::SharedHotPage)) ||
                !image.backing->subresource_states.empty() ||
                image.backing->state.layout != vk::ImageLayout::eShaderReadOnlyOptimal ||
                image.backing->state.access_mask != vk::AccessFlagBits2::eShaderRead) {
                fast_ok = false;
                break;
            }
            image_infos.emplace_back(VK_NULL_HANDLE, entry.view,
                                     vk::ImageLayout::eShaderReadOnlyOptimal);
        }
        if (fast_ok) {
            record_set_stats();
            return true;
        }
        image_infos.resize(first_image_idx);
    }

    const bool proxies_on = !BbToggle::Disabled(BbToggle::SampleSceneProxies);
    // Every image still has the backing its view belongs to and needs no refresh.
    for (u32 i = 0; i < count; ++i) {
        const auto& entry = set.entries[i];
        if (!entry.id) {
            continue;
        }
        auto& image = texture_cache.GetImageUntouched(entry.id);
        // A key guide icon the GPU copied in, recognised by its bytes (then not from the set).
        if (prompts_on && image.MaybePromptIcon()) {
            image.prompt_icon =
                u8(BbPrompts::IconOf(reinterpret_cast<const void*>(image.info.guest_address), 1024));
        }
        // Proxy entries need a current proxy; native entries of a proxied image go through
        // BindTextures, which may sample the proxy instead.
        if (image.backing != entry.backing || image.binding.needs_rebind ||
            image.binding.is_target || !texture_cache.IsUpToDate(entry.id) ||
            (prompts_on && (image.prompt_icon || image.prompt_atlas)) ||
            (entry.proxy ? !proxies_on ||
                               !scene_targets->ProxyCurrent(image, entry.range.base.level,
                                                            entry.range.extent.levels)
                         : image.scene_proxy && proxies_on) ||
            (upscaler_on && upscaler->RedirectsSampled(entry.id))) {
            ++texture_set_why[2];
            ++texture_set_misses;
            set.generation = ~0ull;
            return false;
        }
    }
    record_set_stats();
    for (u32 i = 0; i < count; ++i) {
        const auto& entry = set.entries[i];
        if (!entry.id) {
            image_infos.emplace_back(VK_NULL_HANDLE, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
            continue;
        }
        auto& image = texture_cache.GetImageUntouched(entry.id);
        if (entry.immutable_ro && !prompts_on && !upscaler_on &&
            image.backing->subresource_states.empty() &&
            image.backing->state.layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
            image.backing->state.access_mask == vk::AccessFlagBits2::eShaderRead) {
            image_infos.emplace_back(VK_NULL_HANDLE, entry.view,
                                     vk::ImageLayout::eShaderReadOnlyOptimal);
            continue;
        }
        texture_cache.MarkFound(entry.id);
        image.binding.is_bound = 1u;
        bound_images.emplace_back(entry.id);
        image.usage.texture = 1u;
        if (entry.proxy) {
            image_infos.emplace_back(VK_NULL_HANDLE, entry.view,
                                     scene_targets->PrepareSample(image, entry.range.base.level));
            ++proxy_samples;
            continue;
        }
        const auto new_layout = image.info.props.is_depth
                                    ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                    : vk::ImageLayout::eShaderReadOnlyOptimal;
        constexpr auto ShaderStages =
            vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
        const auto read_access =
            image.info.props.is_depth
                ? (vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                   vk::AccessFlagBits2::eShaderRead)
                : vk::AccessFlagBits2::eShaderRead;
        barrier |= runtime.Transit(&image, new_layout, ShaderStages,
                                   read_access, entry.range);
        image_infos.emplace_back(VK_NULL_HANDLE, entry.view, image.backing->state.layout);
    }
    return true;
}

} // namespace Vulkan
