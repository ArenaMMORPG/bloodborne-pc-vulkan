// SPDX-License-Identifier: GPL-2.0-or-later
// The texture cache: construction, lookup, overlap resolution, bindings and samplers.
#include "textures/texture_cache.h"

#include <xxhash.h>

#include "common/assert.h"
#include "common/hash.h"
#include "core/emulator_settings.h"
#include "core/memory.h"
#include "render/gcn_to_vk.h"
#include "textures/direct_textures.h"
#include "textures/format_class.h"
#include "buffers/buffer_cache.h"
#include "vulkan/instance.h"
#include "render/runtime.h"
#include "render/scheduler.h"

namespace VideoCore {

namespace {

/// Submissions an image must have been unused before an overlap may evict it.
constexpr u64 TicksBeforeRemoval = 32;

/// The slot of a FindImage result.
u64 FoundSlot(const ImageInfo& info, TextureCache::BindingType type, bool exact_fmt) {
    u64 key = info.guest_address ^ u64(info.guest_size) << 17 ^ u64(info.pixel_format) << 40 ^
              u64(info.size.width) << 24 ^ info.size.height ^ u64(type) << 58 ^ u64(exact_fmt) << 63;
    key = (key ^ key >> 33) * 0xFF51AFD7ED558CCDull;
    key = (key ^ key >> 33) * 0xC4CEB9FE1A85EC53ull;
    return key ^ key >> 33;
}

} // namespace

TextureCache::TextureCache(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
                           Vulkan::Runtime& runtime_, BbCp::CommandProcessor* command_processor_,
                           BufferCache& buffer_cache_, BbMem::PageWatch& page_watch_)
    : instance{instance_}, scheduler{scheduler_}, runtime{runtime_},
      command_processor{command_processor_}, buffer_cache{buffer_cache_},
      page_watch{page_watch_}, detiler{instance, scheduler, runtime, buffer_cache.GetStreamBuffer()},
      readback_linear_images{EmulatorSettings.IsReadbackLinearImagesEnabled()} {
    const u32 max_samplers = instance.GetMaxSamplerAllocationCount();
    trigger_gc_samplers = max_samplers * 3 / 4;
    pressure_gc_samplers = max_samplers * 7 / 8;
    critical_gc_samplers = max_samplers * 15 / 16;
    UpdateMemoryBudget();
    DirectTextures::SetActiveCache(this);
}

TextureCache::~TextureCache() {
    DirectTextures::SetActiveCache(nullptr);
}

u64 TextureCache::CurrentTick() const {
    return scheduler.CurrentTick();
}

std::string TextureCache::DescribeImagesIn(VAddr addr, u64 size) {
    std::unique_lock lock{mutex, std::try_to_lock}; // the caller may hold it already
    if (!lock.owns_lock()) {
        return " [images: not looked up]";
    }
    std::string out;
    int count = 0;
    ForEachImageInRegion(addr, size, [&](ImageId, Image& image) {
        if (count++ < 3) {
            out += fmt::format(" [image {:#x}+{:#x} {}x{} fmt {} tile {}{}{}]",
                               image.info.guest_address, image.info.guest_size,
                               image.info.size.width, image.info.size.height,
                               u32(image.info.pixel_format), u32(image.info.tile_mode),
                               image.usage.render_target ? " RT" : "",
                               True(image.flags & ImageFlagBits::GpuModified) ? " gpu" : "");
        }
    });
    if (count > 3) {
        out += fmt::format(" +{} more", count - 3);
    }
    return out;
}

ImageId TextureCache::InsertImage(const ImageInfo& info) {
    const ImageId id = slot_images.insert(instance, runtime, slot_image_views, info);
    RegisterImage(id);
    return id;
}

ImageId TextureCache::FindImage(ImageDesc& desc, bool exact_fmt) {
    const ImageInfo& info = desc.info;
    ASSERT(info.guest_address != 0);
    std::scoped_lock lock{mutex};

    // A remembered result: an exact match stays valid while that image is registered with the
    // same description, a resolved overlap (a view into another image) until any registration
    // changes.
    FoundImage& found = found_images[FoundSlot(info, desc.type, exact_fmt) % found_images.size()];
    const auto still_exact = [&] {
        if (found.view_mip >= 0 || found.view_slice >= 0 || !found.image_id) {
            return false;
        }
        const Image& image = slot_images[found.image_id];
        return True(image.flags & ImageFlagBits::Registered) &&
               image.info.guest_address == info.guest_address &&
               image.info.guest_size == info.guest_size && image.info.size == info.size &&
               image.info.pixel_format == info.pixel_format;
    };
    if ((found.generation == registry_generation.load(std::memory_order_relaxed) ||
         still_exact()) &&
        found.address == info.guest_address && found.size == info.guest_size &&
        found.extent == info.size && found.format == info.pixel_format &&
        found.type == info.type && found.exact_fmt == exact_fmt && found.binding == desc.type &&
        found.levels == info.resources.levels && found.layers == info.resources.layers &&
        !BbToggle::Disabled(BbToggle::FindImageCache)) {
        Image& image = slot_images[found.image_id];
        image.tick_accessed_last = CurrentTick();
        TouchImage(image);
        if (found.view_mip > 0) {
            desc.view_info.range.base.level = found.view_mip;
        }
        if (found.view_slice > 0) {
            desc.view_info.range.base.layer = found.view_slice;
        }
        return found.image_id;
    }

    // V3.2: O(1) lookup for pre-uploaded .tpf/.gnf asset textures without walking ImageRegistry pages.
    if (const auto it = direct_assets_by_addr.find(info.guest_address);
        it != direct_assets_by_addr.end() && slot_images.is_allocated(it->second)) {
        const ImageId cand_id = it->second;
        Image& cand = slot_images[cand_id];
        const ImageInfo& cached = cand.info;
        if (True(cand.flags & ImageFlagBits::Registered) &&
            cached.guest_address == info.guest_address &&
            cached.guest_size == info.guest_size && cached.size == info.size &&
            cached.resources == info.resources &&
            cached.props.is_depth == info.props.is_depth &&
            IsVulkanFormatCompatible(cached.pixel_format, info.pixel_format) &&
            (cached.type == info.type || info.size == Extent3D{1, 1, 1}) &&
            (!exact_fmt || cached.pixel_format == info.pixel_format)) {
            cand.tick_accessed_last = CurrentTick();
            TouchImage(cand);
            found = FoundImage{
                .address = info.guest_address,
                .size = info.guest_size,
                .extent = info.size,
                .format = info.pixel_format,
                .type = info.type,
                .exact_fmt = exact_fmt,
                .binding = desc.type,
                .levels = info.resources.levels,
                .layers = info.resources.layers,
                .generation = registry_generation.load(std::memory_order_relaxed),
                .image_id = cand_id,
                .view_mip = -1,
                .view_slice = -1,
            };
            return cand_id;
        }
    }

    ImageIds overlapping;
    ForEachImageInRegion(info.guest_address, info.guest_size,
                         [&](ImageId id, Image&) { overlapping.push_back(id); });

    // An exact match: same place, size and extent, a compatible format and the type (but for
    // 1x1x1 images); the last one wins.
    ImageId image_id{};
    for (const ImageId id : overlapping) {
        const ImageInfo& cached = slot_images[id].info;
        if (cached.guest_address == info.guest_address && cached.guest_size == info.guest_size &&
            cached.size == info.size &&
            IsVulkanFormatCompatible(cached.pixel_format, info.pixel_format) &&
            (cached.type == info.type || info.size == Extent3D{1, 1, 1}) &&
            (!exact_fmt || cached.pixel_format == info.pixel_format)) {
            image_id = id;
        }
    }

    // Else resolve the overlaps one by one, carrying the merged result.
    int view_mip = -1;
    int view_slice = -1;
    if (!image_id) {
        for (const ImageId id : overlapping) {
            view_mip = -1;
            view_slice = -1;
            const ImageInfo& merged = image_id ? slot_images[image_id].info : info;
            const auto [resolved, mip, slice] = ResolveOverlap(merged, desc.type, id, image_id);
            if (resolved) {
                image_id = resolved;
                view_mip = mip;
                view_slice = slice;
            }
        }
    }
    if (image_id) {
        const Image& resolved = slot_images[image_id];
        if (exact_fmt && info.pixel_format != resolved.info.pixel_format) {
            image_id = {}; // the very format is needed
        } else if (resolved.info.resources < info.resources) {
            FreeImage(image_id); // picked up wrongly
            image_id = {};
            LOG_WARNING(Render_Vulkan, "Image overlap resolve failed");
        }
    }
    if (!image_id) {
        image_id = InsertImage(info);
    }

    Image& image = slot_images[image_id];
    image.tick_accessed_last = CurrentTick();
    TouchImage(image);
    // A subresource of a larger image: where the view starts.
    if (view_mip > 0) {
        desc.view_info.range.base.level = view_mip;
    }
    if (view_slice > 0) {
        desc.view_info.range.base.layer = view_slice;
    }
    found = FoundImage{
        .address = info.guest_address,
        .size = info.guest_size,
        .extent = info.size,
        .format = info.pixel_format,
        .type = info.type,
        .exact_fmt = exact_fmt,
        .binding = desc.type,
        .levels = info.resources.levels,
        .layers = info.resources.layers,
        .generation = registry_generation.load(std::memory_order_relaxed),
        .image_id = image_id,
        .view_mip = view_mip,
        .view_slice = view_slice,
    };
    return image_id;
}

ImageId TextureCache::ResolveDepthOverlap(const ImageInfo& requested, BindingType binding,
                                          ImageId cache_id) {
    Image& cached = slot_images[cache_id];
    if (!cached.info.props.is_depth && !requested.props.is_depth) {
        return {};
    }
    // Recreated with the other aspect or more subresources; else the view handles it.
    bool recreate = cached.info.resources < requested.resources;
    switch (binding) {
    case BindingType::Texture:
        // A depth texture where the cache holds a color image of the same bits.
        recreate |= requested.props.is_depth && !cached.info.props.is_depth;
        break;
    case BindingType::Storage:
        recreate |= cached.info.props.is_depth;
        break;
    case BindingType::RenderTarget:
        ASSERT(!requested.props.is_depth);
        recreate |= cached.info.props.is_depth;
        break;
    case BindingType::DepthTarget:
        // A color image becomes a depth image; a depth image needs the same aspects and bits.
        recreate |= !cached.info.props.is_depth;
        recreate |= cached.info.props.is_depth &&
                    (requested.props.has_stencil != cached.info.props.has_stencil ||
                     requested.num_bits != cached.info.num_bits);
        break;
    default:
        break;
    }
    if (!recreate) {
        return cache_id;
    }

    ImageInfo info = requested;
    info.resources = std::max(requested.resources, cached.info.resources);
    info.UpdateSize();
    const ImageId new_id = InsertImage(info);
    Image& image = slot_images[new_id];
    image.usage = cached.usage;
    image.flags &= ~ImageFlagBits::Dirty;
    // A depth buffer made from another image is not cleared on first use.
    image.info.meta_info.htile_clear_mask = 0;
    runtime.CopyColorAndDepth(&cached, &image);
    FreeImage(cache_id);
    return new_id;
}

std::tuple<ImageId, int, int> TextureCache::ResolveOverlap(const ImageInfo& info,
                                                           BindingType binding, ImageId cache_id,
                                                           ImageId merged_id) {
    Image& cached = slot_images[cache_id];
    const ImageInfo& old = cached.info;
    const bool stale = CurrentTick() - cached.tick_accessed_last > TicksBeforeRemoval;

    if (info.guest_address == old.guest_address) {
        // Different block layouts at one address: most likely memory reused from a pool.
        if (info.BlockDim() != old.BlockDim() ||
            info.num_bits * info.num_samples != old.num_bits * old.num_samples) {
            if (stale) {
                FreeImage(cache_id);
            }
            return {merged_id, -1, -1};
        }
        if (const ImageId depth_id = ResolveDepthOverlap(info, binding, cache_id)) {
            return {depth_id, -1, -1};
        }
        // A compressed view of an uncompressed image of the same block size, or either of them
        // 3D at the same size: a new image with the old contents.
        if ((info.props.is_block && !old.props.is_block) ||
            (info.guest_size == old.guest_size &&
             (info.type == Gcn::ImageType::Color3D || old.type == Gcn::ImageType::Color3D))) {
            return {ExpandImage(info, cache_id), -1, -1};
        }
        const bool only_pow2_padding =
            info.props.is_pow2 != old.props.is_pow2 && info.tile_mode == old.tile_mode &&
            info.size == old.size && info.pitch == old.pitch && info.resources.levels == 1 &&
            old.resources.levels == 1 && info.resources.layers == 1 && old.resources.layers == 1;
        // Another format, or not larger: a view of the image.
        if (info.pixel_format != old.pixel_format || info.guest_size <= old.guest_size ||
            only_pow2_padding) {
            const ImageId result = merged_id ? merged_id : cache_id;
            const bool fits =
                IsVulkanFormatCompatible(slot_images[result].info.pixel_format, info.pixel_format);
            return {fits ? result : ImageId{}, -1, -1};
        }
        // More levels or layers: expanded.
        if (info.type == old.type && info.resources > old.resources) {
            return {ExpandImage(info, cache_id), -1, -1};
        }
        // Larger only by its tiling: the address reused for another image.
        if (info.tile_mode != old.tile_mode) {
            if (stale) {
                FreeImage(cache_id);
            }
            return {merged_id, -1, -1};
        }
        LOG_ERROR(Render_Vulkan,
                  "Unresolvable image overlap at {:#x}: cached {}x{}x{} {} type {} pitch {} "
                  "L{} x{} S{} tile {} bits {} block {} size {:#x} pow2 {} alt {} (unused {} "
                  "ticks); requested {}x{}x{} {} type {} pitch {} L{} x{} S{} tile {} bits {} "
                  "block {} size {:#x} pow2 {} alt {}; binding {}, merged {}",
                  info.guest_address, old.size.width, old.size.height, old.size.depth,
                  vk::to_string(old.pixel_format), u64(old.type), old.pitch, old.resources.levels,
                  old.resources.layers, old.num_samples, u32(old.tile_mode), old.num_bits,
                  bool(old.props.is_block), old.guest_size, bool(old.props.is_pow2),
                  old.alt_tile, CurrentTick() - cached.tick_accessed_last, info.size.width,
                  info.size.height, info.size.depth, vk::to_string(info.pixel_format),
                  u64(info.type), info.pitch, info.resources.levels, info.resources.layers,
                  info.num_samples, u32(info.tile_mode), info.num_bits, bool(info.props.is_block),
                  info.guest_size, bool(info.props.is_pow2), info.alt_tile, u32(binding),
                  merged_id.index);
        UNREACHABLE_MSG("unresolvable image overlap at the same address");
    }

    if (info.guest_address > old.guest_address) {
        // The requested image may be a subresource of the cached one.
        if (const s32 mip = info.MipOf(old); mip >= 0) {
            if (const s32 slice = info.SliceOf(old, mip); slice >= 0) {
                return {cache_id, mip, slice};
            }
        }
        // A chance overlap.
        if (stale) {
            FreeImage(cache_id);
        }
        return {{}, -1, -1};
    }

    // The cached image may be a subresource of the requested one.
    if (const s32 mip = old.MipOf(info); mip >= 0) {
        if (const s32 slice = old.SliceOf(info, mip); slice >= 0) {
            if (cached.binding.is_target) {
                // A part of it bound as a render target: rebound to the larger image.
                cached.binding.needs_rebind = 1;
                if (merged_id) {
                    GetImage(merged_id).binding.is_target = 1;
                }
                FreeImage(cache_id);
                return {merged_id, -1, -1};
            }
            // Copied into the larger image once there is one.
            if (merged_id) {
                runtime.CopyMip(&cached, &slot_images[merged_id], mip, slice);
                FreeImage(cache_id);
            }
        }
    }
    return {merged_id, -1, -1};
}

ImageId TextureCache::ExpandImage(const ImageInfo& info, ImageId image_id) {
    const ImageId new_id = InsertImage(info);
    Image& source = slot_images[image_id];
    Image& image = slot_images[new_id];
    RefreshImage(image);
    runtime.CopyImage(&source, &image);
    if (source.binding.is_bound || source.binding.is_target) {
        source.binding.needs_rebind = 1;
    }
    FreeImage(image_id);
    TrackImage(new_id);
    return new_id;
}

ImageId TextureCache::FindImageFromRange(VAddr address, size_t size, bool ensure_valid,
                                         size_t max_tail) {
    ImageIds candidates;
    ForEachImageInRegion(address, size, [&](ImageId id, Image& image) {
        if (image.info.guest_address == address && (!ensure_valid || image.SafeToDownload())) {
            candidates.push_back(id);
        }
    });
    // One candidate is taken as is (its size may differ from the buffer's a little).
    if (candidates.size() == 1) {
        return candidates.back();
    }
    {
        ImageId exact{};
        u64 best_tick = 0;
        for (const ImageId id : candidates) {
            const Image& img = slot_images[id];
            if (img.info.guest_size == size && (!exact || img.tick_accessed_last >= best_tick)) {
                exact = id;
                best_tick = img.tick_accessed_last;
            }
        }
        if (exact) {
            return exact;
        }
    }
    const size_t effective_max_tail = max_tail != 0 ? max_tail : (512ull << 10);
    {
        ImageId best{};
        size_t best_size = 0;
        u64 best_tick = 0;
        for (const ImageId id : candidates) {
            const Image& img = slot_images[id];
            const size_t guest_size = img.info.guest_size;
            if (guest_size <= size && size - guest_size <= effective_max_tail &&
                (guest_size > best_size ||
                 (guest_size == best_size && img.tick_accessed_last >= best_tick))) {
                best = id;
                best_size = guest_size;
                best_tick = img.tick_accessed_last;
            }
        }
        if (best) {
            return best;
        }
    }
    {
        ImageId sub_best{};
        u64 best_tick = 0;
        for (const ImageId id : candidates) {
            const Image& img = slot_images[id];
            bool matches_sub = false;
            if (img.info.resources.layers > 1) {
                const size_t slice_size = img.info.guest_size / img.info.resources.layers;
                if (slice_size <= size && size - slice_size <= effective_max_tail) {
                    matches_sub = true;
                }
            }
            if (!matches_sub && img.info.resources.levels > 1 && !img.info.mips_layout.empty()) {
                const size_t mip0_size = img.info.mips_layout[0].size;
                if (mip0_size <= size && size - mip0_size <= effective_max_tail) {
                    matches_sub = true;
                }
            }
            if (matches_sub && (!sub_best || img.tick_accessed_last >= best_tick)) {
                sub_best = id;
                best_tick = img.tick_accessed_last;
            }
        }
        if (sub_best) {
            return sub_best;
        }
    }
    if (!candidates.empty() && ensure_valid) {
        LOG_WARNING(Render_Vulkan, "No image of exactly {:#x}+{:#x}", address, size);
    }
    return {};
}

std::pair<ImageId, s32> TextureCache::FindImageSliceFromRange(VAddr address, size_t size) {
    constexpr size_t MaxSliceTail = 512ull << 10;
    std::pair<ImageId, s32> found{{}, -1};
    u64 best_tick = 0;
    bool best_exact = false;
    ForEachImageInRegion(address, size, [&](ImageId id, Image& image) {
        if (image.info.resources.levels == 1 && image.info.resources.layers > 1) {
            const size_t slice_size = image.info.guest_size / image.info.resources.layers;
            if (slice_size > 0 && slice_size <= size && size - slice_size <= MaxSliceTail &&
                address >= image.info.guest_address) {
                const u64 offset = address - image.info.guest_address;
                if (offset % slice_size == 0) {
                    const u64 layer = offset / slice_size;
                    if (layer < image.info.resources.layers) {
                        const bool is_exact = (slice_size == size);
                        if (found.second < 0 || (is_exact && !best_exact) ||
                            (is_exact == best_exact && image.tick_accessed_last >= best_tick)) {
                            found = {id, static_cast<s32>(layer)};
                            best_tick = image.tick_accessed_last;
                            best_exact = is_exact;
                        }
                    }
                }
            }
        }
    });
    return found;
}

void TextureCache::QueueDownload(ImageId image_id) {
    std::scoped_lock lk{download_mutex};
    download_images.emplace(image_id);
    downloads_queued.store(true, std::memory_order_release);
}

ImageView& TextureCache::FindTexture(ImageId image_id, const ImageDesc& desc, ViewMemo* memo,
                                     bool refresh) {
    Image& image = slot_images[image_id];
    WatchImage(image, refresh ? "texture (refresh)" : "texture (no refresh)");
    if (desc.type == BindingType::Storage) {
        image.flags |= ImageFlagBits::GpuModified;
        image.gpu_write_tick = CurrentTick();
        if (readback_linear_images && (!image.info.props.is_tiled || image.info.size.width <= 8) &&
            image.info.guest_address != 0) {
            QueueDownload(image_id);
        }
    }
    if (refresh) {
        UpdateImage(image_id);
    }
    if (!memo || BbToggle::Disabled(BbToggle::TextureViewMemo)) {
        return image.FindView(desc.view_info);
    }
    if (memo->image_id == image_id && memo->backing == image.backing && memo->view_id) {
        return slot_image_views[memo->view_id];
    }
    ImageView& view = image.FindView(desc.view_info);
    const auto& ids = image.backing->image_view_ids;
    const u32 last = image.backing->last_view;
    memo->image_id = image_id;
    memo->backing = image.backing;
    memo->view_id =
        last < ids.size() && &slot_image_views[ids[last]] == &view ? ids[last] : ids.back();
    return view;
}

ImageView& TextureCache::FindRenderTarget(ImageId image_id, const ImageDesc& desc, ViewMemo* memo) {
    Image& image = slot_images[image_id];
    WatchImage(image, "render target");
    image.flags |= ImageFlagBits::GpuModified;
    image.gpu_write_tick = CurrentTick();
    if (readback_linear_images && (!image.info.props.is_tiled || image.info.size.width <= 8)) {
        QueueDownload(image_id);
    }
    image.usage.render_target = 1;
    UpdateImage(image_id);
    // Its metadata surfaces.
    if (const VAddr cmask = desc.info.meta_info.cmask_addr) {
        surface_metas.emplace(cmask, MetaInfo{.type = MetaType::CMask});
        image.info.meta_info.cmask_addr = cmask;
    }
    if (const VAddr fmask = desc.info.meta_info.fmask_addr) {
        surface_metas.emplace(fmask, MetaInfo{.type = MetaType::FMask});
        image.info.meta_info.fmask_addr = fmask;
    }
    if (!memo || BbToggle::Disabled(BbToggle::TextureViewMemo)) {
        return image.FindView(desc.view_info, false);
    }
    if (memo->image_id == image_id && memo->backing == image.backing && memo->view_id) {
        return slot_image_views[memo->view_id];
    }
    ImageView& view = image.FindView(desc.view_info, false);
    const auto& ids = image.backing->image_view_ids;
    const u32 last = image.backing->last_view;
    memo->image_id = image_id;
    memo->backing = image.backing;
    memo->view_id =
        last < ids.size() && &slot_image_views[ids[last]] == &view ? ids[last] : ids.back();
    return view;
}

ImageView& TextureCache::FindDepthTarget(ImageId image_id, const ImageDesc& desc, ViewMemo* memo) {
    Image& image = slot_images[image_id];
    WatchImage(image, "depth target");
    image.flags |= ImageFlagBits::GpuModified;
    image.gpu_write_tick = CurrentTick();
    image.usage.depth_target = 1;
    UpdateImage(image_id);
    if (const VAddr htile = desc.info.meta_info.htile_addr) {
        surface_metas.emplace(htile, MetaInfo{.type = MetaType::HTile,
                                              .clear_mask = image.info.meta_info.htile_clear_mask});
        image.info.meta_info.htile_addr = htile;
    }
    // The stencil surface: an image of its own (a placeholder of no format) linked to the depth.
    // V3.1: Cache stencil_id + stencil_uid on the depth image so we avoid scanning 900+ pages
    // of the 2560x1440 stencil buffer via ForEachImageInRegion on every depth pass.
    if (desc.info.stencil_addr != 0) {
        ImageId stencil_id{};
        if (image.stencil_id && slot_images.is_allocated(image.stencil_id)) {
            const Image& cached_st = slot_images[image.stencil_id];
            if (cached_st.image_uid == image.stencil_uid &&
                True(cached_st.flags & ImageFlagBits::Registered) &&
                cached_st.info.guest_address == desc.info.stencil_addr) {
                stencil_id = image.stencil_id;
            }
        }
        if (!stencil_id) {
            ForEachImageInRegion(desc.info.stencil_addr, desc.info.stencil_size,
                                 [&](ImageId id, Image& candidate) {
                                     if (candidate.info.guest_address == desc.info.stencil_addr) {
                                         stencil_id = id;
                                     }
                                 });
            if (!stencil_id) {
                ImageInfo info{};
                info.guest_address = desc.info.stencil_addr;
                info.guest_size = desc.info.stencil_size;
                info.size = desc.info.size;
                stencil_id = InsertImage(info);
            }
            image.stencil_id = stencil_id;
            image.stencil_uid = slot_images[stencil_id].image_uid;
        }
        Image& stencil = slot_images[stencil_id];
        TouchImage(stencil);
        stencil.AssociateDepth(image_id, image.image_uid);
    }
    if (!memo || BbToggle::Disabled(BbToggle::TextureViewMemo)) {
        return image.FindView(desc.view_info, false);
    }
    if (memo->image_id == image_id && memo->backing == image.backing && memo->view_id) {
        return slot_image_views[memo->view_id];
    }
    ImageView& view = image.FindView(desc.view_info, false);
    const auto& ids = image.backing->image_view_ids;
    const u32 last = image.backing->last_view;
    memo->image_id = image_id;
    memo->backing = image.backing;
    memo->view_id =
        last < ids.size() && &slot_image_views[ids[last]] == &view ? ids[last] : ids.back();
    return view;
}

ImageId TextureCache::GetAssociatedDepth(Image& image) {
    if (!image.depth_id) {
        return {};
    }
    if (slot_images.is_allocated(image.depth_id)) {
        const Image& depth = slot_images[image.depth_id];
        if (depth.image_uid == image.depth_uid && True(depth.flags & ImageFlagBits::Registered)) {
            return image.depth_id;
        }
    }
    image.DisassociateDepth(); // that depth image is gone
    return {};
}

vk::Sampler TextureCache::GetSampler(const Gcn::Sampler& sampler,
                                     Gcn::BorderColorBuffer border_color_base, bool is_depth,
                                     float extra_lod_bias) {
    struct SamplerMemo {
        Gcn::Sampler sampler{};
        u32 is_depth = ~0u;
        u32 lod_bias_bits = 0;
        u64 tick = ~0ull;
        vk::Sampler handle{};
    };
    static constexpr size_t kMemoSize = 256;
    thread_local std::array<SamplerMemo, kMemoSize> tls_memo{};
    const u32 depth_flag = u32(is_depth);
    const u32 bias_bits = std::bit_cast<u32>(extra_lod_bias);
    u64 raw[2];
    std::memcpy(raw, &sampler, sizeof(raw));
    const u64 fast_hash = (raw[0] ^ (raw[1] * 0x9E3779B97F4A7C15ull)) ^
                          ((u64(depth_flag) << 32) | bias_bits) * 0xBF58476D1CE4E5B9ull;
    auto& memo = tls_memo[(fast_hash ^ (fast_hash >> 32)) & (kMemoSize - 1)];
    if (memo.tick == gc_tick && memo.is_depth == depth_flag && memo.lod_bias_bits == bias_bits &&
        memo.sampler == sampler && memo.handle) [[likely]] {
        return memo.handle;
    }

    // Compare and plain uses of one S# need samplers of their own, so do other LOD biases.
    const u64 key = HashCombine(HashCombine(XXH3_64bits(&sampler, sizeof(sampler)), is_depth),
                                u64(bias_bits));
    std::scoped_lock lock{samplers_mutex};
    const auto [it, created] =
        samplers.try_emplace(key, instance, sampler, border_color_base, is_depth, extra_lod_bias);
    if (created) {
        it.value().lru_id = sampler_lru.Insert(key, gc_tick);
    } else {
        sampler_lru.Touch(it->second.lru_id, gc_tick);
    }
    const vk::Sampler handle = it->second.Handle();
    memo.sampler = sampler;
    memo.is_depth = depth_flag;
    memo.lod_bias_bits = bias_bits;
    memo.tick = gc_tick;
    memo.handle = handle;
    return handle;
}

void TextureCache::ProcessDirectTextureEvents(Core::MemoryManager* memory) {
    if (!DirectTextures::HasPendingEvents()) {
        return;
    }
    static thread_local std::vector<DirectTextures::Event> events;
    DirectTextures::DrainEvents(events);
    if (events.empty()) {
        return;
    }

    u64 uploaded_bytes_since_flush = 0;
    bool needs_barrier_flush = false;
    std::unique_lock lock{mutex};

    for (const auto& ev : events) {
        if (ev.kind == DirectTextures::EventKind::RetireRange) {
            if (ev.addr == 0 || ev.byte_size == 0) {
                continue;
            }
            ImageIds stale;
            ForEachImageInRegion(ev.addr, static_cast<size_t>(ev.byte_size),
                                 [&](ImageId id, Image& img) {
                                     if (!img.binding.is_bound && !img.binding.is_target) {
                                         stale.push_back(id);
                                     } else {
                                         img.flags &= ~ImageFlagBits::DirectImmutable;
                                         UntrackImage(id);
                                     }
                                 });
            for (const ImageId id : stale) {
                FreeImage(id);
            }
            continue;
        }

        Gcn::Image tsharp{};
        std::memcpy(&tsharp, ev.tsharp.data(), sizeof(tsharp));
        if (!tsharp.Valid() || tsharp.Address() == 0 || tsharp.IsMsaa()) {
            continue;
        }
        const auto valid_swizzle = [](u64 sel) {
            return sel <= u64(Gcn::CompSwizzle::Alpha) && sel != 2 && sel != 3;
        };
        if (!valid_swizzle(tsharp.dst_sel_x) || !valid_swizzle(tsharp.dst_sel_y) ||
            !valid_swizzle(tsharp.dst_sel_z) || !valid_swizzle(tsharp.dst_sel_w)) {
            continue;
        }
        if (Vulkan::GcnToVk::TrySurfaceFormat(tsharp.GetDataFmt(), tsharp.GetNumberFmt()) ==
            vk::Format::eUndefined) {
            continue;
        }

        Shader::ImageResource res{};
        res.is_array = (tsharp.GetType() == Gcn::ImageType::Color1DArray ||
                        tsharp.GetType() == Gcn::ImageType::Color2DArray || tsharp.IsCube());
        ImageDesc desc{tsharp, res};
        if (desc.info.guest_address == 0 || desc.info.guest_size == 0) {
            continue;
        }
        if (memory && !memory->IsValidGpuMapping(desc.info.guest_address, desc.info.guest_size)) {
            continue;
        }

        const size_t clear_span = std::max<size_t>(desc.info.guest_size, ev.alloc_byte_size);
        ImageIds overlapping;
        ForEachImageInRegion(desc.info.guest_address, clear_span, [&](ImageId id, Image& img) {
            if (!img.binding.is_bound && !img.binding.is_target) {
                overlapping.push_back(id);
            } else {
                img.flags &= ~ImageFlagBits::DirectImmutable;
                UntrackImage(id);
            }
        });
        for (const ImageId id : overlapping) {
            FreeImage(id);
        }

        buffer_cache.NotePreciseUpload(desc.info.guest_address, desc.info.guest_size);

        const ImageId image_id = InsertImage(desc.info);
        Image& image = slot_images[image_id];
        if (ev.is_immutable) {
            image.flags |= ImageFlagBits::DirectImmutable;
        }
        image.usage.texture = 1u;
        image.tick_accessed_last = CurrentTick();
        direct_assets_by_addr[desc.info.guest_address] = image_id;
        TrackImage(image_id);
        TouchImage(image);
        RefreshImage(image);

        constexpr auto ShaderStages = vk::PipelineStageFlagBits2::eAllGraphics |
                                      vk::PipelineStageFlagBits2::eComputeShader;
        if (runtime.Transit(&image, vk::ImageLayout::eShaderReadOnlyOptimal, ShaderStages,
                            vk::AccessFlagBits2::eShaderRead)) {
            needs_barrier_flush = true;
        }
        (void)image.FindView(desc.view_info);

        uploaded_bytes_since_flush += desc.info.guest_size;
        if (uploaded_bytes_since_flush >= (32ull << 20)) {
            if (needs_barrier_flush) {
                runtime.FlushBarriers();
                needs_barrier_flush = false;
            }
            uploaded_bytes_since_flush = 0;
            lock.unlock();
            scheduler.Flush();
            lock.lock();
        }
    }

    if (needs_barrier_flush) {
        runtime.FlushBarriers();
    }
}

} // namespace VideoCore
