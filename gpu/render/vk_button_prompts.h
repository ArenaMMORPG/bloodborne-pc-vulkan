// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: the port's button prompt icons on the GPU (gpu/shim/bbport_button_prompts.h) and
// Hybrid GPU UI & Icon Super-Resolution (2x / 4x) for 1-mip 2D UI textures (title logo,
// inventory icons, runes, HUD atlases).
#pragma once

#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "vulkan/vk_common.h"
#include "textures/image.h"

namespace VideoCore {
class BufferCache;
struct ImageViewInfo;
} // namespace VideoCore

namespace Vulkan {

class Instance;
class Runtime;
class Scheduler;

class ButtonPrompts {
public:
    ButtonPrompts(const Instance& instance, Scheduler& scheduler, VideoCore::BufferCache& buffer_cache);
    ~ButtonPrompts();

    /// The icons for the device in use, uploaded when they changed (the draw recording thread;
    /// ends the render pass then, once per change of device or bindings).
    void Update();

    /// Whether any icon is the port's now.
    bool Active() const noexcept {
        return active;
    }

    /// The port's icon for the game's icon `icon` (Image::prompt_icon), viewed as `info` views
    /// the game's; null: the game's own.
    vk::ImageView View(u32 icon, const VideoCore::ImageViewInfo& info);

    /// The copy of the HUD atlas `image` (Image::prompt_atlas) with the port's icons in its
    /// cells, viewed as `info` views the game's; null: the game's own. Made (in stream order, out
    /// of a render pass) when the icons or the atlas's bytes changed.
    vk::ImageView AtlasView(Runtime& runtime, VideoCore::Image& image,
                            const VideoCore::ImageViewInfo& info);

    /// True if `image` is a 1-mip 2D block-compressed UI texture eligible for UI Super-Resolution.
    bool IsUiTexture(const VideoCore::Image& image) const noexcept;

    /// Returns the 2x/4x upscaled view of 1-mip 2D UI texture `image` (logos, item/weapon/armor
    /// icons, HUD widgets), computed once per upload on the GPU via `ui_upscale.comp` and cached
    /// in VRAM.
    vk::ImageView UiView(Runtime& runtime, VideoCore::Image& image,
                         const VideoCore::ImageViewInfo& info);

private:
    struct Atlas {
        VideoCore::HostImage image;
        vk::Image src_image{};
        vk::UniqueImageView src_view;
        vk::UniqueImageView storage_view;
        u32 upload = 0;
        u32 generation = 0;
        u32 scale = 0;
        u64 last_used_tick = 0;
        std::map<std::tuple<bool, bool, u32>, vk::UniqueImageView> views;
    };

    bool EnsureUpscaleViews(Atlas& entry, VideoCore::Image& src_image, u32 dst_w, u32 dst_h);

    const Instance& instance;
    Scheduler& scheduler;
    VideoCore::BufferCache& buffer_cache;
    VideoCore::HostImage image;
    vk::UniqueDescriptorSetLayout upscale_set_layout;
    vk::UniquePipelineLayout upscale_pipeline_layout;
    vk::UniquePipeline upscale_pipeline;
    u32 layers = 0;
    u32 generation = 0;
    bool active = false;
    std::vector<bool> drawn;
    std::vector<u32> small;  ///< the icons at 1x game size (32x32), a layer after another
    std::vector<u32> medium; ///< the icons at 2x size (64x64) for upscaled HUD atlases
    std::unordered_map<u64, Atlas> atlases;     ///< HUD prompt atlases by Image::image_uid
    std::unordered_map<u64, Atlas> ui_textures; ///< Upscaled 1-mip UI textures by Image::image_uid
    std::map<std::tuple<u32, bool, bool, u32>, vk::UniqueImageView> views;
};

} // namespace Vulkan
