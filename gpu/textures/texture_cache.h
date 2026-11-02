// SPDX-License-Identifier: GPL-2.0-or-later
// bbport textures: the cache of guest images on the host. Images are found by address and
// description (FindImage: an exact match, else one resolved from the overlapping images: a
// subresource view, an expanded or recreated image, or a new one), kept up to date with guest
// memory (their pages are watched; a write marks them dirty, the next use uploads them, detiled
// on the GPU), written back where the guest reads what the GPU rendered, and evicted when VRAM
// runs short (least recently used first).
//
// One mutex guards the registry; the fast paths every binding takes (UpdateImage of a clean image,
// a remembered FindImage result) avoid it.
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>

#include <boost/container/small_vector.hpp>
#include <tsl/robin_map.h>

#include "bbport_toggles.h"
#include "common/lru_cache.h"
#include "common/slot_vector.h"
#include "shader/resource.h"
#include "textures/detiler.h"
#include "textures/image.h"
#include "textures/image_registry.h"
#include "textures/image_view.h"
#include "textures/sampler.h"

namespace BbCp {
class CommandProcessor;
}
namespace BbMem {
class PageWatch;
}
namespace BbVideoOut {
struct BufferAttributeGroup;
}
namespace Core {
class MemoryManager;
}
namespace Vulkan {
class Instance;
class Scheduler;
class Runtime;
} // namespace Vulkan

namespace VideoCore {

class BufferCache;

class TextureCache {
public:
    enum class BindingType : u32 {
        Texture,
        Storage,
        RenderTarget,
        DepthTarget,
        VideoOut,
    };

    /// What a binding asks for: the image and the view of it.
    struct ImageDesc {
        ImageInfo info;
        ImageViewInfo view_info;
        BindingType type{BindingType::Texture};

        ImageDesc() = default;
        ImageDesc(const Gcn::Image& image, const Shader::ImageResource& desc)
            : info{image, desc}, view_info{image, desc},
              type{desc.is_written ? BindingType::Storage : BindingType::Texture} {}
        ImageDesc(const Gcn::ColorBuffer& buffer, Gcn::CbDbExtent hint)
            : info{buffer, hint}, view_info{buffer}, type{BindingType::RenderTarget} {}
        ImageDesc(const Gcn::DepthBuffer& buffer, Gcn::DepthView view, Gcn::DepthControl control,
                  VAddr htile_address, Gcn::CbDbExtent hint, bool write_buffer = false)
            : info{buffer, view.NumSlices(), htile_address, hint, write_buffer},
              view_info{buffer, view, control}, type{BindingType::DepthTarget} {}
        ImageDesc(const BbVideoOut::BufferAttributeGroup& group, VAddr cpu_address)
            : info{group, cpu_address}, type{BindingType::VideoOut} {}
    };

    /// A FindTexture result remembered by the caller, for the same image and view.
    struct ViewMemo {
        ImageId image_id{};
        const void* backing = nullptr;
        ImageViewId view_id{};
    };

    enum class MetaType {
        CMask,
        FMask,
        HTile,
    };

    TextureCache(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                 Vulkan::Runtime& runtime, BbCp::CommandProcessor* command_processor,
                 BufferCache& buffer_cache, BbMem::PageWatch& page_watch);
    ~TextureCache();

    // Lookup.

    /// The image the description asks for (found, resolved from overlaps, or created); a view
    /// into a larger image puts its level and layer into `desc.view_info`. `exact_fmt`: only an
    /// image of the very format.
    [[nodiscard]] ImageId FindImage(ImageDesc& desc, bool exact_fmt = false);
    /// The image starting at `address` (`ensure_valid`: written by the GPU and clean).
    [[nodiscard]] ImageId FindImageFromRange(VAddr address, size_t size, bool ensure_valid = true,
                                             size_t max_tail = 0);
    /// Single-level array/cubemap slice covering `[address, address + size)`: `(image_id, layer)`.
    [[nodiscard]] std::pair<ImageId, s32> FindImageSliceFromRange(VAddr address, size_t size);
    /// The view a texture or storage binding uses; `refresh` false: an image found out of date is
    /// used as is (the caller checked it).
    [[nodiscard]] ImageView& FindTexture(ImageId image_id, const ImageDesc& desc,
                                         ViewMemo* memo = nullptr, bool refresh = true);
    [[nodiscard]] ImageView& FindRenderTarget(ImageId image_id, const ImageDesc& desc,
                                              ViewMemo* memo = nullptr);
    [[nodiscard]] ImageView& FindDepthTarget(ImageId image_id, const ImageDesc& desc,
                                             ViewMemo* memo = nullptr);
    /// The sampler of an S#.
    [[nodiscard]] vk::Sampler GetSampler(const Gcn::Sampler& sampler,
                                         Gcn::BorderColorBuffer border_color_base, bool is_depth,
                                         float extra_lod_bias = 0.0f);

    Image* TryGetImage(ImageId id, u64 uid) {
        if (!slot_images.is_allocated(id)) {
            return nullptr;
        }
        Image& image = slot_images[id];
        return !uid || image.image_uid == uid ? &image : nullptr;
    }
    [[nodiscard]] Image& GetImage(ImageId id) {
        Image& image = slot_images[id];
        if (image.lru_touched_tick > gc_tick || (gc_tick - image.lru_touched_tick) >= 8) [[unlikely]] {
            TouchImage(image);
        }
        return image;
    }
    /// V3.2: O(1) validation for DirectImmutable asset textures that survives unrelated
    /// registry_generation bumps when other .tpf/.gnf textures stream in.
    [[nodiscard]] bool IsRegisteredImmutable(ImageId id, u64 uid) const noexcept {
        if (!id || uid == 0 || !slot_images.is_allocated(id)) {
            return false;
        }
        const Image& image = slot_images[id];
        constexpr u32 Required =
            u32(ImageFlagBits::Registered) | u32(ImageFlagBits::DirectImmutable);
        return image.image_uid == uid && (u32(image.flags) & Required) == Required;
    }
    [[nodiscard]] Image& GetImageUntouched(ImageId id) {
        return slot_images[id];
    }
    [[nodiscard]] ImageView& GetImageView(ImageViewId id) {
        return slot_image_views[id];
    }
    /// The depth image a stencil image belongs to, while it still lives.
    ImageId GetAssociatedDepth(Image& image);

    /// Changes whenever an image is registered or unregistered.
    [[nodiscard]] u64 RegistryGeneration() const noexcept {
        return registry_generation.load(std::memory_order_acquire);
    }
    [[nodiscard]] u64 GcTick() const noexcept {
        return gc_tick;
    }
    [[nodiscard]] u64 SamplerEvictGeneration() const noexcept {
        return sampler_evict_gen.load(std::memory_order_relaxed);
    }
    /// N2.1: Marks a GPU-DMA uploaded asset VkImage as DirectImmutable (disarming any page traps,
    /// registering direct_assets_by_addr, and enabling O(1) lock-free IsUpToDate).
    void MarkDirectImmutableAsset(ImageId image_id);
    /// A memoized lookup found the image: its access tick (FindImage's), the LRU touch comes
    /// with UpdateImage.
    void MarkFound(ImageId image_id) {
        slot_images[image_id].tick_accessed_last = CurrentTick();
    }

    // Keeping images up to date.

    /// Whether UpdateImage has nothing to do: clean, registered, watched whole, touched this
    /// collection period.
    [[nodiscard]] bool IsUpToDate(ImageId image_id) const {
        return IsUpToDate(slot_images[image_id]);
    }
    /// Uploads what the guest changed; every binding comes here (lock-free when clean).
    void UpdateImage(ImageId image_id) {
        if (!BbToggle::Disabled(BbToggle::UpdateImageFastPath) &&
            IsUpToDate(slot_images[image_id])) {
            return;
        }
        std::scoped_lock lock{mutex};
        Image& image = slot_images[image_id];
        if (True(image.flags & ImageFlagBits::SharedHotPage)) {
            if (True(image.flags & ImageFlagBits::GpuModified)) {
                image.track_addr = 0;
                image.track_addr_end = 0;
                image.flags &= ~ImageFlagBits::SharedHotPage;
            } else if (False(image.flags & ImageFlagBits::Dirty) &&
                       image.hot_check_tick != gc_tick) {
                const u64 h = ComputeSmallOrSampleHash(image.info);
                image.hot_check_tick = gc_tick;
                if (h != image.hash) {
                    image.hash = h;
                    image.flags |= ImageFlagBits::CpuDirty;
                }
            }
        }
        TrackImage(image_id);
        WatchImage(image, "update image (slow path)");
        TouchImage(image);
        RefreshImage(image);
    }
    /// Uploads the image if it is dirty (detiled on the GPU).
    void RefreshImage(Image& image);
    /// BB_IMAGE_WATCH=addr (diagnostics): what happens to the image at that address.
    void WatchImage(const Image& image, const char* what);

    // Guest memory events.

    /// The guest wrote [addr, addr + size): the images there are dirty (or maybe dirty).
    void InvalidateMemory(VAddr addr, size_t size);
    /// The GPU wrote the memory of the image starting at `address` as a buffer.
    /// GPU buffer write: refresh images starting here and wholly covered by max_size.
    void InvalidateMemoryFromGPU(VAddr address, size_t max_size);
    /// Lock-free check whether any registered 1 MiB image page overlaps [addr, addr + size).
    [[nodiscard]] bool HasActiveImagePages(VAddr addr, size_t size) const noexcept {
        return size != 0 && registry.AnyPageActive(addr, size);
    }
    /// Exact byte-range check whether any registered image overlaps [addr, addr + size).
    /// Fast lock-free O(1) exit when the 1 MiB page has no images; otherwise verifies exact
    /// [guest_address, guest_address + guest_size) overlap so buffers sharing a 1 MiB slab with
    /// unrelated textures are never misclassified as images.
    [[nodiscard]] bool HasOverlappingImage(VAddr addr, size_t size) const noexcept {
        if (size == 0 || !registry.AnyPageActive(addr, size)) {
            return false;
        }
        std::scoped_lock lock{mutex};
        bool found = false;
        registry.ForEachPage(addr, size, [&](const ImageRegistry::Ids& ids) {
            for (const ImageId id : ids) {
                if (slot_images[id].Overlaps(addr, size)) {
                    found = true;
                    return true;
                }
            }
            return false;
        });
        return found;
    }
    /// The range was unmapped: its images go.
    void UnmapMemory(VAddr cpu_addr, size_t size);
    /// Writes back the images queued for it (BB_READBACK_LINEAR_IMAGES).
    void ProcessDownloadImages();
    /// V3.2 Phase A (guest thread): proactively disarms watched image traps in user space before
    /// memcpy / tiling / block free so guest threads never fault into SIGSEGV -> DrainDrawPipe().
    /// Returns true if any registered image overlapped [addr, addr + size).
    bool DisarmWatchedRange(VAddr addr, size_t size, bool mark_dirty);
    /// V3.2 Phase B (Stage B stream order): retires freed blocks and uploads .tpf/.gnf assets
    /// directly into DEVICE_LOCAL VkImages transitioned to eShaderReadOnlyOptimal.
    void ProcessDirectTextureEvents(Core::MemoryManager* memory);

    // Surface metadata (CMASK, FMASK, HTILE) of bound targets.

    std::optional<MetaType> IsMeta(VAddr address) const {
        const auto it = surface_metas.find(address);
        return it != surface_metas.end() ? std::optional{it->second.type} : std::nullopt;
    }
    [[nodiscard]] bool HasMetaInRange(VAddr addr, size_t size) const noexcept {
        if (size == 0 || surface_metas.empty()) {
            return false;
        }
        const VAddr end = addr + size;
        for (const auto& [maddr, _] : surface_metas) {
            if (maddr >= addr && maddr < end) {
                return true;
            }
        }
        return false;
    }
    static constexpr u64 kSharedHotPageMaxBytes = 32768;
    static constexpr u64 kHotGranuleShift = 16;
    static constexpr size_t kHotGranuleCount = static_cast<size_t>((80ULL << 30) >> kHotGranuleShift);
    [[nodiscard]] static u64 ComputeSmallOrSampleHash(const ImageInfo& info) noexcept;
    bool IsMetaCleared(VAddr address, u32 slice) const {
        const auto it = surface_metas.find(address);
        return it != surface_metas.end() && (it->second.clear_mask & (1u << slice));
    }
    bool ClearMeta(VAddr address) {
        const auto it = surface_metas.find(address);
        if (it == surface_metas.end()) {
            return false;
        }
        it.value().clear_mask = -1;
        return true;
    }
    bool TouchMeta(VAddr address, u32 slice, bool is_clear) {
        const auto it = surface_metas.find(address);
        if (it == surface_metas.end()) {
            return false;
        }
        if (is_clear) {
            it.value().clear_mask |= 1u << slice;
        } else {
            it.value().clear_mask &= ~(1u << slice);
        }
        return true;
    }

    // Memory.

    /// Once per guest submission: evicts unused images and samplers when memory runs short.
    void RunGarbageCollector();
    /// VRAM ran out (any thread): for the next 30 s the collector evicts by submissions again
    /// instead of keeping textures unused for seconds.
    static void NoteVramShort();
    static bool VramShort();

    Detiler& GetTileManager() noexcept {
        return detiler;
    }
    /// Diagnostics: the images over [addr, addr + size), briefly.
    std::string DescribeImagesIn(VAddr addr, u64 size);

    /// Calls `func(id, image)` once for each image overlapping [addr, addr + size); a `func`
    /// returning bool stops at true.
    /// ForEachImageInRegion under the registry mutex (diagnostics from other threads).
    template <typename Func>
    void ForEachImageInRegionLocked(VAddr addr, size_t size, Func&& func) {
        std::scoped_lock lock{mutex};
        ForEachImageInRegion(addr, size, std::forward<Func>(func));
    }

    template <typename Func>
    void ForEachImageInRegion(VAddr addr, size_t size, Func&& func) {
        using Result = std::invoke_result_t<Func, ImageId, Image&>;
        boost::container::small_vector<ImageId, 16> seen;
        registry.ForEachPage(addr, size, [&](const ImageRegistry::Ids& ids) {
            for (const ImageId id : ids) {
                Image& image = slot_images[id];
                if (True(image.flags & ImageFlagBits::Picked) || !image.Overlaps(addr, size)) {
                    continue;
                }
                image.flags |= ImageFlagBits::Picked;
                seen.push_back(id);
                if constexpr (std::is_same_v<Result, bool>) {
                    if (func(id, image)) {
                        return true;
                    }
                } else {
                    func(id, image);
                }
            }
            return false;
        });
        for (const ImageId id : seen) {
            slot_images[id].flags &= ~ImageFlagBits::Picked;
        }
    }

private:
    using ImageIds = boost::container::small_vector<ImageId, 16>;

    bool IsUpToDate(const Image& image) const {
        // The flags are read atomically: a racing invalidation races the same way with the
        // locked path.
        const u32 flags =
            __atomic_load_n(reinterpret_cast<const u32*>(&image.flags), __ATOMIC_ACQUIRE);
        constexpr u32 Dirty = u32(ImageFlagBits::Dirty);
        constexpr u32 Registered = u32(ImageFlagBits::Registered);
        constexpr u32 SharedHot = u32(ImageFlagBits::SharedHotPage);
        constexpr u32 DirectImm = u32(ImageFlagBits::DirectImmutable);
        const bool lru_ok =
            ((flags & DirectImm) != 0 && total_used_memory < trigger_gc_memory) ||
            (image.lru_touched_tick <= gc_tick && (gc_tick - image.lru_touched_tick) < 8);
        return (flags & (Dirty | Registered)) == Registered &&
               ((flags & SharedHot) == 0 || image.hot_check_tick == gc_tick) &&
               image.track_addr == image.guest_begin && image.track_addr_end == image.guest_end &&
               lru_ok;
    }
    u64 CurrentTick() const;

    // Lookup (texture_cache.cpp).
    ImageId InsertImage(const ImageInfo& info);
    std::tuple<ImageId, int, int> ResolveOverlap(const ImageInfo& info, BindingType binding,
                                                 ImageId cache_id, ImageId merged_id);
    ImageId ResolveDepthOverlap(const ImageInfo& info, BindingType binding, ImageId cache_id);
    ImageId ExpandImage(const ImageInfo& info, ImageId image_id);
    void QueueDownload(ImageId image_id);

    // Registry and page watching (texture_cache_track.cpp).
    void RegisterImage(ImageId image_id);
    void UnregisterImage(ImageId image_id);
    void TrackImage(ImageId image_id);
    void TrackImageHead(ImageId image_id);
    void TrackImageTail(ImageId image_id);
    void UntrackImage(ImageId image_id);
    void UntrackImageHead(ImageId image_id);
    void UntrackImageTail(ImageId image_id);
    void MarkAsMaybeDirty(ImageId image_id, Image& image);
    void NoteHotImageHit(Image& image) noexcept;
    [[nodiscard]] bool IsHotGranuleRange(VAddr addr, u64 size) const noexcept;
    void TouchImage(const Image& image);
    void DeleteImage(ImageId image_id);
    void FreeImage(ImageId image_id) {
        UntrackImage(image_id);
        UnregisterImage(image_id);
        DeleteImage(image_id);
    }

    // Memory (texture_cache_memory.cpp).
    void DownloadImageMemory(ImageId image_id, bool sync);
    void GarbageCollectImages();
    void GarbageCollectSamplers();
    void UpdateMemoryBudget();

    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    Vulkan::Runtime& runtime;
    BbCp::CommandProcessor* command_processor;
    BufferCache& buffer_cache;
    BbMem::PageWatch& page_watch;
    Detiler detiler;
    Common::SlotVector<Image> slot_images;
    Common::SlotVector<ImageView> slot_image_views;
    ImageRegistry registry;
    mutable std::mutex mutex;
    std::atomic<u64> registry_generation{0};
    const bool readback_linear_images;

    /// FindImage results while the registry is unchanged (under `mutex`).
    struct FoundImage {
        VAddr address = 0;
        u64 size = 0;
        Extent3D extent{};
        vk::Format format{};
        Gcn::ImageType type{};
        bool exact_fmt = false;
        BindingType binding{};
        u32 levels = 0;
        u32 layers = 0;
        u64 generation = ~0ull;
        ImageId image_id{};
        int view_mip = -1;
        int view_slice = -1;
    };
    std::array<FoundImage, 1024> found_images{};

    // Samplers.
    std::mutex samplers_mutex;
    tsl::robin_map<u64, Sampler> samplers;
    Common::LeastRecentlyUsedCache<u64, u64> sampler_lru;
    u64 trigger_gc_samplers = 0;
    u64 pressure_gc_samplers = 0;
    u64 critical_gc_samplers = 0;
    std::atomic<u64> sampler_evict_gen{0};

    // Write-backs.
    std::mutex download_mutex;
    std::unordered_set<ImageId> download_images;
    std::atomic<bool> downloads_queued{false}; ///< checked without the lock

    // Garbage collection.
    Common::LeastRecentlyUsedCache<ImageId, u64> lru_cache;
    u64 gc_tick = 0;
    u64 total_used_memory = 0;
    u64 trigger_gc_memory = 0;
    u64 pressure_gc_memory = 0;
    u64 critical_gc_memory = 0;
    u64 gc_second = 0;
    std::array<u64, 64> gc_tick_at_second{}; ///< gc_tick at each of the last 64 seconds
    u64 gc_evictions = 0, gc_downloads = 0;  ///< since the last pressure report
    std::chrono::steady_clock::time_point gc_report_time{};
    static inline std::atomic<u64> vram_short_until{0}; ///< steady seconds

    struct MetaInfo {
        MetaType type;
        s32 clear_mask = -1;
    };
    tsl::robin_map<VAddr, MetaInfo> surface_metas;
    tsl::robin_map<VAddr, ImageId> direct_assets_by_addr; ///< V3.2: O(1) VAddr -> ImageId lookup
    std::unique_ptr<u8[]> hot_granule_hits; ///< Sticky 64 KiB hot-page counters for rotated CPU LUT rings
};

} // namespace VideoCore
