// SPDX-License-Identifier: GPL-2.0-or-later
#include "render/vk_button_prompts.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include "bbgpu/shaders/ui_upscale_comp.h"
#include "bbport_button_prompts.h"
#include "bbport_settings.h"
#include "buffers/buffer_cache.h"
#include "vulkan/instance.h"
#include "vulkan/shader_module.h"
#include "render/runtime.h"
#include "render/scheduler.h"
#include "textures/image_view.h"

namespace Vulkan {
namespace {

struct UpscalePush {
    s32 src_w;
    s32 src_h;
    s32 dst_w;
    s32 dst_h;
    s32 scale_factor;
    s32 is_block;
};

bool IsSrgb(vk::Format format) {
    switch (format) {
    case vk::Format::eBc1RgbaSrgbBlock:
    case vk::Format::eBc1RgbSrgbBlock:
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc7SrgbBlock:
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eA8B8G8R8SrgbPack32:
        return true;
    default:
        return false;
    }
}

vk::Format UnormFormat(vk::Format format) {
    switch (format) {
    case vk::Format::eBc1RgbSrgbBlock:
        return vk::Format::eBc1RgbUnormBlock;
    case vk::Format::eBc1RgbaSrgbBlock:
        return vk::Format::eBc1RgbaUnormBlock;
    case vk::Format::eBc2SrgbBlock:
        return vk::Format::eBc2UnormBlock;
    case vk::Format::eBc3SrgbBlock:
        return vk::Format::eBc3UnormBlock;
    case vk::Format::eBc7SrgbBlock:
        return vk::Format::eBc7UnormBlock;
    case vk::Format::eR8G8B8A8Srgb:
        return vk::Format::eR8G8B8A8Unorm;
    case vk::Format::eB8G8R8A8Srgb:
        return vk::Format::eB8G8R8A8Unorm;
    case vk::Format::eA8B8G8R8SrgbPack32:
        return vk::Format::eA8B8G8R8UnormPack32;
    default:
        return format;
    }
}

bool IsSupportedUiFormat(vk::Format format) {
    switch (format) {
    case vk::Format::eBc1RgbUnormBlock:
    case vk::Format::eBc1RgbSrgbBlock:
    case vk::Format::eBc1RgbaUnormBlock:
    case vk::Format::eBc1RgbaSrgbBlock:
    case vk::Format::eBc2UnormBlock:
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc3UnormBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc4UnormBlock:
    case vk::Format::eBc7UnormBlock:
    case vk::Format::eBc7SrgbBlock:
        return true;
    default:
        return false;
    }
}

void DownsampleIcons(const std::vector<u32>& src, u32 layers, u32 dst_dim, std::vector<u32>& dst) {
    const u32 factor = BbPrompts::IconSize / dst_dim;
    dst.assign(std::size_t(layers) * dst_dim * dst_dim, 0);
    for (u32 layer = 0; layer < layers; ++layer) {
        const u32* icon = src.data() + std::size_t(layer) * BbPrompts::IconSize * BbPrompts::IconSize;
        for (u32 y = 0; y < dst_dim; ++y) {
            for (u32 x = 0; x < dst_dim; ++x) {
                u32 sum[4]{};
                for (u32 dy = 0; dy < factor; ++dy) {
                    for (u32 dx = 0; dx < factor; ++dx) {
                        const u32 texel = icon[(y * factor + dy) * BbPrompts::IconSize + x * factor + dx];
                        for (u32 c = 0; c < 4; ++c) {
                            sum[c] += (texel >> (8 * c)) & 0xff;
                        }
                    }
                }
                u32 out = 0;
                for (u32 c = 0; c < 4; ++c) {
                    out |= (sum[c] + factor * factor / 2) / (factor * factor) << (8 * c);
                }
                dst[(std::size_t(layer) * dst_dim + y) * dst_dim + x] = out;
            }
        }
    }
}

} // namespace

ButtonPrompts::ButtonPrompts(const Instance& instance_, Scheduler& scheduler_,
                             VideoCore::BufferCache& buffer_cache_)
    : instance{instance_}, scheduler{scheduler_}, buffer_cache{buffer_cache_},
      image{instance_.GetDevice(), instance_.GetAllocator()} {
    BbPrompts::Start();

    const auto device = instance.GetDevice();
    const std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{
        {.binding = 0,
         .descriptorType = vk::DescriptorType::eSampledImage,
         .descriptorCount = 1,
         .stageFlags = vk::ShaderStageFlagBits::eCompute},
        {.binding = 1,
         .descriptorType = vk::DescriptorType::eStorageImage,
         .descriptorCount = 1,
         .stageFlags = vk::ShaderStageFlagBits::eCompute},
    }};
    upscale_set_layout = Check(device.createDescriptorSetLayoutUnique({
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    }));
    const vk::PushConstantRange push_range{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(UpscalePush),
    };
    upscale_pipeline_layout = Check(device.createPipelineLayoutUnique({
        .setLayoutCount = 1,
        .pSetLayouts = &*upscale_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    }));
    const auto module = CompileSPV(UI_UPSCALE_COMP, device);
    upscale_pipeline = Check(device.createComputePipelineUnique(
        {}, vk::ComputePipelineCreateInfo{
                .stage = {.stage = vk::ShaderStageFlagBits::eCompute,
                          .module = module,
                          .pName = "main"},
                .layout = *upscale_pipeline_layout,
            }));
    device.destroyShaderModule(module);
}

ButtonPrompts::~ButtonPrompts() = default;

void ButtonPrompts::Update() {
    BbPrompts::Set set;
    if (!BbPrompts::Update(generation, set)) {
        return;
    }
    generation = set.generation;
    drawn = set.drawn;
    active = std::find(drawn.begin(), drawn.end(), true) != drawn.end();
    if (!active) {
        return; // the game's icons: nothing to upload
    }
    if (!image) {
        layers = set.count;
        image.Create(vk::ImageCreateInfo{
            .flags = vk::ImageCreateFlagBits::eMutableFormat,
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = {BbPrompts::IconSize, BbPrompts::IconSize, 1},
            .mipLevels = 1,
            .arrayLayers = layers,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        });
    }
    if (set.count != layers) {
        active = false;
        return;
    }
    // Precompute 1x (32x32) and 2x (64x64) icons for HUD atlases.
    DownsampleIcons(set.pixels, layers, BbPrompts::GameIconSize, small);
    DownsampleIcons(set.pixels, layers, BbPrompts::GameIconSize * 2, medium);

    const u64 bytes = u64(set.pixels.size()) * sizeof(u32);
    auto& stream = buffer_cache.GetStreamBuffer();
    const u64 offset = stream.Copy(set.pixels.data(), bytes, 16);
    // In stream order, ahead of the draws that sample it; the draws before it are done reading it
    // (the barrier's source stages).
    scheduler.EndRendering();
    scheduler.Record([image = vk::Image{image.image}, buffer = stream.Handle(), offset,
                      layers = layers](vk::CommandBuffer cmdbuf) {
        const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, layers};
        const vk::ImageMemoryBarrier2 to_copy{
            .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
            .srcAccessMask = vk::AccessFlagBits2::eShaderRead,
            .dstStageMask = vk::PipelineStageFlagBits2::eCopy,
            .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .image = image,
            .subresourceRange = range,
        };
        cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                   .pImageMemoryBarriers = &to_copy});
        const vk::BufferImageCopy copy{
            .bufferOffset = offset,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, layers},
            .imageOffset = {0, 0, 0},
            .imageExtent = {BbPrompts::IconSize, BbPrompts::IconSize, 1},
        };
        cmdbuf.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, copy);
        const vk::ImageMemoryBarrier2 to_sample{
            .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
            .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .image = image,
            .subresourceRange = range,
        };
        cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                   .pImageMemoryBarriers = &to_sample});
    });
}

vk::ImageView ButtonPrompts::View(u32 icon, const VideoCore::ImageViewInfo& info) {
    if (!active || icon == 0 || icon > drawn.size() || !drawn[icon - 1]) {
        return {};
    }
    const bool array = info.type == Gcn::ImageType::Color2DArray;
    if (!array && info.type != Gcn::ImageType::Color2D) {
        return {};
    }
    const bool srgb = IsSrgb(info.format);
    const u32 mapping = u32(info.mapping.r) | u32(info.mapping.g) << 8 | u32(info.mapping.b) << 16 |
                        u32(info.mapping.a) << 24;
    auto& view = views[{icon, srgb, array, mapping}];
    if (!view) {
        auto [result, created] = instance.GetDevice().createImageViewUnique(vk::ImageViewCreateInfo{
            .image = image.image,
            .viewType = array ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D,
            .format = srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm,
            .components = info.mapping,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, icon - 1, 1},
        });
        if (result != vk::Result::eSuccess) {
            std::printf("Button prompts: no image view (%s)\n", vk::to_string(result).c_str());
            return {};
        }
        view = std::move(created);
    }
    return *view;
}

bool ButtonPrompts::EnsureUpscaleViews(Atlas& entry, VideoCore::Image& src_image,
                                       u32 dst_w, u32 dst_h) {
    const auto device = instance.GetDevice();
    if (!entry.image) {
        entry.image = VideoCore::HostImage{device, instance.GetAllocator()};
        entry.image.Create(vk::ImageCreateInfo{
            .flags = vk::ImageCreateFlagBits::eMutableFormat,
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = {dst_w, dst_h, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        });
    }
    if (!entry.storage_view) {
        auto [res, created] = device.createImageViewUnique(vk::ImageViewCreateInfo{
            .image = entry.image.image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        });
        if (res != vk::Result::eSuccess) {
            return false;
        }
        entry.storage_view = std::move(created);
    }
    if (entry.src_image != src_image.GetImage() || !entry.src_view) {
        entry.src_image = src_image.GetImage();
        const vk::Format raw_fmt = UnormFormat(src_image.backing->image.image_ci.format);
        auto [res, created] = device.createImageViewUnique(vk::ImageViewCreateInfo{
            .image = entry.src_image,
            .viewType = vk::ImageViewType::e2D,
            .format = raw_fmt,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        });
        if (res != vk::Result::eSuccess) {
            entry.src_view.reset();
            return false;
        }
        entry.src_view = std::move(created);
    }
    return true;
}

vk::ImageView ButtonPrompts::AtlasView(Runtime& runtime, VideoCore::Image& image,
                                       const VideoCore::ImageViewInfo& info) {
    if (!active || image.prompt_atlas == 0 || info.type != Gcn::ImageType::Color2D || !image.backing) {
        return {};
    }
    const auto cells = BbPrompts::AtlasCells(image.prompt_atlas);
    const auto is_drawn = [&](const BbPrompts::AtlasCell& cell) {
        return cell.icon <= drawn.size() && drawn[cell.icon - 1] && cell.icon <= layers;
    };
    if (std::none_of(cells.begin(), cells.end(), is_drawn)) {
        return {};
    }
    const vk::Format source_format = image.backing->image.image_ci.format;
    if (source_format != vk::Format::eBc7UnormBlock && source_format != vk::Format::eBc7SrgbBlock) {
        return {};
    }
    const u32 width = image.info.size.width, height = image.info.size.height;
    const bool upscale_on = BbSettings::Get().ui_upscale.load(std::memory_order_relaxed);
    const u32 desired_scale = (upscale_on && width <= 2048 && height <= 2048) ? 2u : 1u;
    const u32 dst_w = width * desired_scale, dst_h = height * desired_scale;

    Atlas& atlas = atlases[image.image_uid];
    if (atlas.scale != desired_scale) {
        if (atlas.image && atlas.last_used_tick != 0 && !scheduler.IsFree(atlas.last_used_tick)) {
            scheduler.Wait(atlas.last_used_tick);
        }
        atlas.views.clear();
        atlas.storage_view.reset();
        atlas.image.Destroy();
        atlas.scale = desired_scale;
        atlas.upload = 0;
        atlas.generation = 0;
    }
    if (!EnsureUpscaleViews(atlas, image, dst_w, dst_h)) {
        return {};
    }
    if (atlas.upload != image.prompt_upload || atlas.generation != generation) {
        atlas.upload = image.prompt_upload;
        atlas.generation = generation;
        const u32 cell_dim = BbPrompts::GameIconSize * desired_scale;
        const auto& icon_pool = (desired_scale == 2u) ? medium : small;
        std::vector<vk::BufferImageCopy> copies;
        std::vector<u32> pixels;
        for (const auto& cell : cells) {
            if (!is_drawn(cell) || cell.x + BbPrompts::GameIconSize > width ||
                cell.y + BbPrompts::GameIconSize > height) {
                continue;
            }
            copies.push_back(vk::BufferImageCopy{
                .bufferOffset = pixels.size() * sizeof(u32),
                .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .imageOffset = {s32(cell.x * desired_scale), s32(cell.y * desired_scale), 0},
                .imageExtent = {cell_dim, cell_dim, 1},
            });
            const u32* icon = icon_pool.data() + std::size_t(cell.icon - 1) * cell_dim * cell_dim;
            pixels.insert(pixels.end(), icon, icon + cell_dim * cell_dim);
        }
        auto& stream = buffer_cache.GetStreamBuffer();
        const u64 offset = stream.Copy(pixels.data(), pixels.size() * sizeof(u32), 16);
        for (auto& copy : copies) {
            copy.bufferOffset += offset;
        }
        scheduler.EndRendering();
        runtime.Transit(&image, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::PipelineStageFlagBits2::eComputeShader |
                            vk::PipelineStageFlagBits2::eAllGraphics,
                        vk::AccessFlagBits2::eShaderRead);
        runtime.FlushBarriers();
        scheduler.BumpDescriptorStateEpoch();

        const UpscalePush push{
            .src_w = s32(width),
            .src_h = s32(height),
            .dst_w = s32(dst_w),
            .dst_h = s32(dst_h),
            .scale_factor = s32(desired_scale),
            .is_block = 1,
        };
        scheduler.Record([pipeline = *upscale_pipeline, layout = *upscale_pipeline_layout,
                          src_view = *atlas.src_view, dst_view = *atlas.storage_view,
                          target = vk::Image{atlas.image.image}, buffer = stream.Handle(),
                          copies = std::move(copies), push](vk::CommandBuffer cmdbuf) {
            const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
            const vk::ImageMemoryBarrier2 to_storage{
                .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                .srcAccessMask = vk::AccessFlagBits2::eShaderRead,
                .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eGeneral,
                .image = target,
                .subresourceRange = range,
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &to_storage});

            const vk::DescriptorImageInfo src_info{
                .sampler = VK_NULL_HANDLE,
                .imageView = src_view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };
            const vk::DescriptorImageInfo dst_info{
                .sampler = VK_NULL_HANDLE,
                .imageView = dst_view,
                .imageLayout = vk::ImageLayout::eGeneral,
            };
            const std::array<vk::WriteDescriptorSet, 2> writes{{
                {.dstBinding = 0,
                 .descriptorCount = 1,
                 .descriptorType = vk::DescriptorType::eSampledImage,
                 .pImageInfo = &src_info},
                {.dstBinding = 1,
                 .descriptorCount = 1,
                 .descriptorType = vk::DescriptorType::eStorageImage,
                 .pImageInfo = &dst_info},
            }};
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
            cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, layout, 0, writes);
            cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(push), &push);
            cmdbuf.dispatch(u32(push.dst_w + 15) / 16, u32(push.dst_h + 15) / 16, 1);

            const vk::ImageMemoryBarrier2 to_copy{
                .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eCopy,
                .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .image = target,
                .subresourceRange = range,
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &to_copy});
            cmdbuf.copyBufferToImage(buffer, target, vk::ImageLayout::eTransferDstOptimal, copies);
            const vk::ImageMemoryBarrier2 to_sample{
                .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .image = target,
                .subresourceRange = range,
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &to_sample});
        });
    }
    atlas.last_used_tick = scheduler.CurrentTick();
    const bool srgb = IsSrgb(info.format);
    const u32 mapping = u32(info.mapping.r) | u32(info.mapping.g) << 8 | u32(info.mapping.b) << 16 |
                        u32(info.mapping.a) << 24;
    auto& view = atlas.views[{srgb, false, mapping}];
    if (!view) {
        auto [result, created] = instance.GetDevice().createImageViewUnique(vk::ImageViewCreateInfo{
            .image = atlas.image.image,
            .viewType = vk::ImageViewType::e2D,
            .format = srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm,
            .components = info.mapping,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        });
        if (result != vk::Result::eSuccess) {
            std::printf("Button prompts: no atlas view (%s)\n", vk::to_string(result).c_str());
            return {};
        }
        view = std::move(created);
    }
    return *view;
}

bool ButtonPrompts::IsUiTexture(const VideoCore::Image& image) const noexcept {
    if (!image.backing || !image.info.props.is_block || image.info.props.is_depth ||
        image.info.type != Gcn::ImageType::Color2D ||
        image.info.resources.levels != 1 || image.info.resources.layers != 1 ||
        image.binding.is_target || image.scene_proxy || image.prompt_icon != 0 ||
        VideoCore::True(image.flags & VideoCore::ImageFlagBits::GpuModified)) {
        return false;
    }
    if (!IsSupportedUiFormat(image.backing->image.image_ci.format)) {
        return false;
    }
    const u32 w = image.info.size.width;
    const u32 h = image.info.size.height;
    return w >= 32 && h >= 32 && w <= 2048 && h <= 2048;
}

vk::ImageView ButtonPrompts::UiView(Runtime& runtime, VideoCore::Image& image,
                                    const VideoCore::ImageViewInfo& info) {
    if (info.type != Gcn::ImageType::Color2D || !IsUiTexture(image)) {
        return {};
    }
    const u32 width = image.info.size.width;
    const u32 height = image.info.size.height;
    // 4x Super-Resolution for logos (1024x256, 1024x512) and individual icons/widgets (<= 512K px);
    // 2x Super-Resolution for large 1024x1024 / 2048x2048 multi-icon sheets (up to 4096x4096).
    const u32 scale = (width * height <= 1024u * 512u && width <= 1024u && height <= 1024u) ? 4u : 2u;
    const u32 dst_w = width * scale;
    const u32 dst_h = height * scale;

    auto it = ui_textures.find(image.image_uid);
    if (it == ui_textures.end() && ui_textures.size() >= 96) {
        // Evict the oldest GPU-retired UI texture so VRAM usage stays strictly bounded.
        auto victim = ui_textures.end();
        for (auto cand = ui_textures.begin(); cand != ui_textures.end(); ++cand) {
            if (scheduler.IsFree(cand->second.last_used_tick)) {
                if (victim == ui_textures.end() ||
                    cand->second.last_used_tick < victim->second.last_used_tick) {
                    victim = cand;
                }
            }
        }
        if (victim != ui_textures.end()) {
            ui_textures.erase(victim);
        }
    }

    Atlas& entry = ui_textures[image.image_uid];
    entry.scale = scale;
    if (!EnsureUpscaleViews(entry, image, dst_w, dst_h)) {
        return {};
    }

    const u32 cur_upload = image.prompt_upload ? image.prompt_upload : 1u;
    if (entry.upload != cur_upload) {
        entry.upload = cur_upload;
        scheduler.EndRendering();
        runtime.Transit(&image, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::PipelineStageFlagBits2::eComputeShader |
                            vk::PipelineStageFlagBits2::eAllGraphics,
                        vk::AccessFlagBits2::eShaderRead);
        runtime.FlushBarriers();
        scheduler.BumpDescriptorStateEpoch();

        const UpscalePush push{
            .src_w = s32(width),
            .src_h = s32(height),
            .dst_w = s32(dst_w),
            .dst_h = s32(dst_h),
            .scale_factor = s32(scale),
            .is_block = 1,
        };
        scheduler.Record([pipeline = *upscale_pipeline, layout = *upscale_pipeline_layout,
                          src_view = *entry.src_view, dst_view = *entry.storage_view,
                          target = vk::Image{entry.image.image}, push](vk::CommandBuffer cmdbuf) {
            const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
            const vk::ImageMemoryBarrier2 to_storage{
                .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                .srcAccessMask = vk::AccessFlagBits2::eShaderRead,
                .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eGeneral,
                .image = target,
                .subresourceRange = range,
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &to_storage});

            const vk::DescriptorImageInfo src_info{
                .sampler = VK_NULL_HANDLE,
                .imageView = src_view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };
            const vk::DescriptorImageInfo dst_info{
                .sampler = VK_NULL_HANDLE,
                .imageView = dst_view,
                .imageLayout = vk::ImageLayout::eGeneral,
            };
            const std::array<vk::WriteDescriptorSet, 2> writes{{
                {.dstBinding = 0,
                 .descriptorCount = 1,
                 .descriptorType = vk::DescriptorType::eSampledImage,
                 .pImageInfo = &src_info},
                {.dstBinding = 1,
                 .descriptorCount = 1,
                 .descriptorType = vk::DescriptorType::eStorageImage,
                 .pImageInfo = &dst_info},
            }};
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
            cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, layout, 0, writes);
            cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(push), &push);
            cmdbuf.dispatch(u32(push.dst_w + 15) / 16, u32(push.dst_h + 15) / 16, 1);

            const vk::ImageMemoryBarrier2 to_sample{
                .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .image = target,
                .subresourceRange = range,
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &to_sample});
        });
    }

    entry.last_used_tick = scheduler.CurrentTick();
    const bool srgb = IsSrgb(info.format);
    const u32 mapping = u32(info.mapping.r) | u32(info.mapping.g) << 8 | u32(info.mapping.b) << 16 |
                        u32(info.mapping.a) << 24;
    auto& view = entry.views[{srgb, false, mapping}];
    if (!view) {
        auto [result, created] = instance.GetDevice().createImageViewUnique(vk::ImageViewCreateInfo{
            .image = entry.image.image,
            .viewType = vk::ImageViewType::e2D,
            .format = srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm,
            .components = info.mapping,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        });
        if (result != vk::Result::eSuccess) {
            std::printf("UI upscale: no image view (%s)\n", vk::to_string(result).c_str());
            return {};
        }
        view = std::move(created);
    }
    return *view;
}

} // namespace Vulkan
