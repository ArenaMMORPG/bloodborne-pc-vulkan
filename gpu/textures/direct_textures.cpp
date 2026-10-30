// SPDX-License-Identifier: GPL-2.0-or-later
// V3.2: Direct .tpf / .gnf Asset Texture Upload & 0-trap Free-List Retirement Queue.
#include "textures/direct_textures.h"

#include <atomic>
#include <cstring>
#include <mutex>

#include "textures/texture_cache.h"

namespace VideoCore::DirectTextures {

namespace {

std::atomic<TextureCache*> g_active_cache{nullptr};
std::atomic<u32> g_pending_count{0};
std::atomic<u32> g_pending_registers{0};
std::mutex g_queue_mutex;
std::vector<Event> g_queue;

void PushEvent(const Event& ev) {
    std::scoped_lock lock{g_queue_mutex};
    if (g_queue.capacity() < 1024) {
        g_queue.reserve(1024);
    }
    g_queue.push_back(ev);
    if (ev.kind == EventKind::RegisterAsset) {
        g_pending_registers.fetch_add(1, std::memory_order_release);
    }
    g_pending_count.fetch_add(1, std::memory_order_release);
}

} // namespace

void SetActiveCache(TextureCache* cache) noexcept {
    g_active_cache.store(cache, std::memory_order_release);
    if (!cache) {
        std::scoped_lock lock{g_queue_mutex};
        g_queue.clear();
        g_pending_registers.store(0, std::memory_order_release);
        g_pending_count.store(0, std::memory_order_release);
    }
}

bool HasPendingEvents() noexcept {
    return g_pending_count.load(std::memory_order_acquire) != 0;
}

void DrainEvents(std::vector<Event>& out) {
    out.clear();
    if (g_pending_count.load(std::memory_order_acquire) == 0) {
        return;
    }
    std::scoped_lock lock{g_queue_mutex};
    if (g_queue.empty()) {
        g_pending_registers.store(0, std::memory_order_release);
        g_pending_count.store(0, std::memory_order_release);
        return;
    }
    out.swap(g_queue);
    g_pending_registers.store(0, std::memory_order_release);
    g_pending_count.store(0, std::memory_order_release);
}

} // namespace VideoCore::DirectTextures

extern "C" void bb_direct_vk_retire_buffer_range(const void* gpu_addr, uint64_t byte_size) noexcept;

extern "C" {

void bb_direct_vk_prepare_texture_write(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    if (auto* cache =
            VideoCore::DirectTextures::g_active_cache.load(std::memory_order_acquire)) {
        cache->DisarmWatchedRange(reinterpret_cast<VAddr>(gpu_addr),
                                  static_cast<size_t>(byte_size),
                                  /*mark_dirty=*/true);
    }
}

void bb_direct_vk_unwatch_texture_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    if (auto* cache =
            VideoCore::DirectTextures::g_active_cache.load(std::memory_order_acquire)) {
        cache->DisarmWatchedRange(reinterpret_cast<VAddr>(gpu_addr),
                                  static_cast<size_t>(byte_size),
                                  /*mark_dirty=*/false);
    }
}

void bb_direct_vk_retire_gpu_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    const VAddr addr = reinterpret_cast<VAddr>(gpu_addr);
    bool has_registered_image = false;
    if (auto* cache =
            VideoCore::DirectTextures::g_active_cache.load(std::memory_order_acquire)) {
        has_registered_image =
            cache->DisarmWatchedRange(addr, static_cast<size_t>(byte_size), /*mark_dirty=*/false);
    }
    if (has_registered_image ||
        VideoCore::DirectTextures::g_pending_registers.load(std::memory_order_acquire) != 0) {
        VideoCore::DirectTextures::Event ev{};
        ev.kind = VideoCore::DirectTextures::EventKind::RetireRange;
        ev.addr = addr;
        ev.byte_size = byte_size;
        VideoCore::DirectTextures::PushEvent(ev);
    }
    bb_direct_vk_retire_buffer_range(gpu_addr, byte_size);
}

void bb_direct_vk_register_asset_texture(uint32_t resource_id,
                                         const void* tsharp_8dw,
                                         uint32_t raw_byte_size,
                                         uint32_t alloc_byte_size,
                                         uint32_t is_immutable) noexcept {
    if (!tsharp_8dw || raw_byte_size == 0) {
        return;
    }
    VideoCore::DirectTextures::Event ev{};
    ev.kind = VideoCore::DirectTextures::EventKind::RegisterAsset;
    ev.is_immutable = (is_immutable != 0);
    ev.resource_id = resource_id;
    ev.raw_byte_size = raw_byte_size;
    ev.alloc_byte_size = alloc_byte_size >= raw_byte_size ? alloc_byte_size : raw_byte_size;
    std::memcpy(ev.tsharp.data(), tsharp_8dw, sizeof(ev.tsharp));
    VideoCore::DirectTextures::PushEvent(ev);
}

} // extern "C"
