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

// N2.1: Lock-free content-deduplicated native S# table (uid = 1..4095).
struct NativeSamplerSlot {
    std::atomic<u32> state{0}; // 0 = empty, 1 = writing, 2 = ready
    u64 clean_qw[2]{0, 0};
};
std::array<NativeSamplerSlot, kMaxNativeSamplers> g_sampler_slots{};

// N2.1: Seqlock 4-way set-associative content-deduplicated native T# SRV/UAV table (uid = 1..65535).
struct NativeSrvSlot {
    std::atomic<u32> seq{0}; // 0 = empty, odd = writing, even != 0 = ready
    u64 clean_qw[4]{0, 0, 0, 0};
    u32 resource_id{0};
    u8 gen8{0};
    bool is_immutable{false};
};
constexpr u32 kSrvBucketCount = kMaxNativeSrvs / 4u; // 16384 buckets x 4 ways
std::array<NativeSrvSlot, kMaxNativeSrvs> g_srv_slots{};
std::array<u8, kSrvBucketCount> g_srv_bucket_clock{};
std::array<std::mutex, 64> g_srv_bucket_locks{};

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

u16 RegisterNativeSampler(void* ssharp_4dw) noexcept {
    if (!ssharp_4dw) {
        return 0;
    }
    u64 raw[2];
    std::memcpy(raw, ssharp_4dw, sizeof(raw));
    auto* clean_dw = reinterpret_cast<u32*>(raw);
    clean_dw[3] &= ~kSamplerTagMask;

    u64 h = (raw[0] * 0x9E3779B97F4A7C15ULL) ^ (raw[1] * 0xBF58476D1CE4E5B9ULL);
    h ^= (h >> 32);
    constexpr u32 kUsableSlots = kMaxNativeSamplers - 1u; // 1..4095
    const u32 start = static_cast<u32>(h % kUsableSlots);

    for (u32 step = 0; step < 64u; ++step) {
        const u32 uid = ((start + step) % kUsableSlots) + 1u;
        auto& s = g_sampler_slots[uid];
        const u32 st = s.state.load(std::memory_order_acquire);
        if (st == 2u) {
            if (s.clean_qw[0] == raw[0] && s.clean_qw[1] == raw[1]) {
                auto* out_dw = static_cast<u32*>(ssharp_4dw);
                out_dw[3] = (clean_dw[3] | (kSamplerTagMagic << 24) | (uid << 12));
                return static_cast<u16>(uid);
            }
            continue;
        }
        if (st == 0u) {
            u32 expected = 0u;
            if (s.state.compare_exchange_strong(expected, 1u, std::memory_order_acq_rel)) {
                s.clean_qw[0] = raw[0];
                s.clean_qw[1] = raw[1];
                s.state.store(2u, std::memory_order_release);
                auto* out_dw = static_cast<u32*>(ssharp_4dw);
                out_dw[3] = (clean_dw[3] | (kSamplerTagMagic << 24) | (uid << 12));
                return static_cast<u16>(uid);
            }
            if (expected == 2u && s.clean_qw[0] == raw[0] && s.clean_qw[1] == raw[1]) {
                auto* out_dw = static_cast<u32*>(ssharp_4dw);
                out_dw[3] = (clean_dw[3] | (kSamplerTagMagic << 24) | (uid << 12));
                return static_cast<u16>(uid);
            }
        }
    }
    return 0;
}

u16 UnpackAndCleanSampler(Gcn::Sampler& ssharp) noexcept {
    auto* dw = reinterpret_cast<u32*>(&ssharp);
    const u32 dw3 = dw[3];
    if (((dw3 >> 24) & 0x3Fu) != kSamplerTagMagic) {
        return 0;
    }
    const u32 uid = (dw3 >> 12) & 0xFFFu;
    dw[3] = dw3 & ~kSamplerTagMask;
    if (uid == 0u || uid >= kMaxNativeSamplers) {
        return 0;
    }
    const auto& s = g_sampler_slots[uid];
    if (s.state.load(std::memory_order_acquire) != 2u) {
        return 0;
    }
    u64 raw[2];
    std::memcpy(raw, &ssharp, sizeof(raw));
    if (s.clean_qw[0] != raw[0] || s.clean_qw[1] != raw[1]) {
        return 0;
    }
    return static_cast<u16>(uid);
}

u16 RegisterNativeSrv(void* tsharp_8dw, u32 resource_id, bool is_immutable) noexcept {
    if (!tsharp_8dw) {
        return 0;
    }
    u64 raw[4];
    std::memcpy(raw, tsharp_8dw, sizeof(raw));
    raw[3] &= 0x00000000FFFFFFFFULL;
    auto* out_dw = static_cast<u32*>(tsharp_8dw);
    if ((raw[0] & 0x3FFFFFFFFFULL) == 0ULL) {
        out_dw[7] = 0u;
        return 0;
    }

    u64 h = (raw[0] * 0x9E3779B97F4A7C15ULL) ^ (raw[1] * 0xBF58476D1CE4E5B9ULL) ^
            (raw[2] * 0x94D049BB133111EBULL) ^ raw[3];
    h ^= (h >> 32);
    const u32 bucket = static_cast<u32>(h) & (kSrvBucketCount - 1u);
    const u32 first_way = (bucket == 0u) ? 1u : 0u;

    std::scoped_lock lk{g_srv_bucket_locks[bucket & 63u]};
    u32 empty_uid = 0u;
    for (u32 w = first_way; w < 4u; ++w) {
        const u32 uid = (bucket << 2) | w;
        auto& s = g_srv_slots[uid];
        const u32 seq = s.seq.load(std::memory_order_relaxed);
        if (seq == 0u) {
            if (empty_uid == 0u) {
                empty_uid = uid;
            }
            continue;
        }
        if (s.clean_qw[0] == raw[0] && s.clean_qw[1] == raw[1] &&
            s.clean_qw[2] == raw[2] && s.clean_qw[3] == raw[3]) {
            out_dw[7] = (kSrvTagMagic << 24) | (static_cast<u32>(s.gen8) << 16) | uid;
            return static_cast<u16>(uid);
        }
    }

    u32 target_uid = empty_uid;
    if (target_uid == 0u) {
        const u32 clk = g_srv_bucket_clock[bucket]++;
        const u32 way = (bucket == 0u) ? (1u + (clk % 3u)) : (clk & 3u);
        target_uid = (bucket << 2) | way;
    }

    auto& s = g_srv_slots[target_uid];
    const u32 old_seq = s.seq.load(std::memory_order_relaxed);
    s.seq.store(old_seq + 1u, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    s.clean_qw[0] = raw[0];
    s.clean_qw[1] = raw[1];
    s.clean_qw[2] = raw[2];
    s.clean_qw[3] = raw[3];
    s.resource_id = resource_id;
    s.is_immutable = is_immutable;
    u8 next_gen = static_cast<u8>(s.gen8 + 1u);
    if (next_gen == 0u) {
        next_gen = 1u;
    }
    s.gen8 = next_gen;
    s.seq.store(old_seq + 2u, std::memory_order_release);

    out_dw[7] = (kSrvTagMagic << 24) | (static_cast<u32>(next_gen) << 16) | target_uid;
    return static_cast<u16>(target_uid);
}

u16 UnpackAndCleanSrv(Gcn::Image& tsharp, u8& out_gen8) noexcept {
    auto* dw = reinterpret_cast<u32*>(&tsharp);
    const u32 dw7 = dw[7];
    if ((dw7 >> 24) != kSrvTagMagic) {
        dw[7] = 0u;
        out_gen8 = 0u;
        return 0u;
    }
    const u32 uid = dw7 & 0xFFFFu;
    const u8 tag_gen = static_cast<u8>((dw7 >> 16) & 0xFFu);
    dw[7] = 0u;
    if (uid == 0u || uid >= kMaxNativeSrvs || tag_gen == 0u) {
        out_gen8 = 0u;
        return 0u;
    }
    const auto& s = g_srv_slots[uid];
    const u32 seq1 = s.seq.load(std::memory_order_acquire);
    if (seq1 == 0u || (seq1 & 1u) != 0u) {
        out_gen8 = 0u;
        return 0u;
    }
    u64 raw[4];
    std::memcpy(raw, &tsharp, sizeof(raw));
    const u8 slot_gen = s.gen8;
    const bool match = (slot_gen == tag_gen) && (s.clean_qw[0] == raw[0]) &&
                       (s.clean_qw[1] == raw[1]) && (s.clean_qw[2] == raw[2]) &&
                       (s.clean_qw[3] == raw[3]);
    std::atomic_thread_fence(std::memory_order_acquire);
    const u32 seq2 = s.seq.load(std::memory_order_relaxed);
    if (!match || seq1 != seq2) {
        out_gen8 = 0u;
        return 0u;
    }
    out_gen8 = tag_gen;
    return static_cast<u16>(uid);
}

} // namespace VideoCore::DirectTextures

extern "C" void bb_direct_vk_retire_buffer_range(const void* gpu_addr, uint64_t byte_size) noexcept;
extern "C" void bb_direct_vk_retire_shader_range(const void* gpu_addr, uint64_t byte_size) noexcept;

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
    bb_direct_vk_retire_shader_range(gpu_addr, byte_size);
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
    ev.tsharp[7] = 0u;
    VideoCore::DirectTextures::PushEvent(ev);
}

void bb_direct_vk_register_native_sampler(void* ssharp_4dw) noexcept {
    (void)VideoCore::DirectTextures::RegisterNativeSampler(ssharp_4dw);
}

void bb_direct_vk_register_native_srv(void* tsharp_8dw,
                                      uint32_t resource_id,
                                      uint32_t is_immutable) noexcept {
    (void)VideoCore::DirectTextures::RegisterNativeSrv(tsharp_8dw, resource_id, is_immutable != 0u);
}

} // extern "C"
