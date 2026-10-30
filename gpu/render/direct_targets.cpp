// SPDX-License-Identifier: GPL-2.0-or-later
// V3.1: Direct RenderTarget & DepthTarget Registration and O(1) Lookup Table.

#include "render/direct_targets.h"

#include <atomic>
#include <cstring>
#include <mutex>

namespace Vulkan::DirectTargets {

namespace {

// Mask of CB_COLORn_INFO bits that define the physical image & view format (excludes dynamic
// fast_clear / blend_bypass / round_mode flags that can toggle without changing the VkImage).
constexpr u32 kColorInfoIdentityMask = 0x80001FFFu;

struct alignas(64) SlotStorage {
    std::atomic<u32> seq{0}; // seqlock: odd = writing, even > 0 = valid
    u8 is_depth{0};
    u8 has_parent_tsharp{0};
    u16 mip_level{0};
    u16 first_slice{0};
    u16 slice_count{1};
    std::array<u32, 13> raw_regs{};
    std::array<u32, 8> parent_tsharp{};
};

constexpr u32 kLookupBuckets = 4096u; // 4096 * 2-way = 8192 pointer cache entries (64 KB)

struct LookupBucket {
    std::atomic<u64> way0{0};
    std::atomic<u64> way1{0};
};

struct RegistryState {
    std::mutex write_mutex;
    u32 next_cursor{0};
    std::array<SlotStorage, kMaxTargets> slots{};
    std::array<LookupBucket, kLookupBuckets> color_lookup{};
    std::array<LookupBucket, kLookupBuckets> depth_lookup{};
};

RegistryState& State() noexcept {
    static RegistryState state{};
    return state;
}

inline u64 PackPtrAndUid(const void* ptr, u32 uid) noexcept {
    const u64 addr_shifted = (reinterpret_cast<uintptr_t>(ptr) >> 2) << 16;
    return addr_shifted | (uid & 0xFFFu);
}

inline u64 PackPtrPrefix(const void* ptr) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) >> 2) << 16;
}

inline u32 HashPtrBucket(const void* ptr) noexcept {
    const u64 v = reinterpret_cast<uintptr_t>(ptr);
    return static_cast<u32>(((v * 0x9E3779B97F4A7C15ULL) >> 52) & (kLookupBuckets - 1u));
}

inline bool MatchesColorRegs(const SlotStorage& slot, const u32* rtv) noexcept {
    return rtv[0] == slot.raw_regs[0] &&
           rtv[1] == slot.raw_regs[1] &&
           rtv[2] == slot.raw_regs[2] &&
           rtv[3] == slot.raw_regs[3] &&
           ((rtv[4] ^ slot.raw_regs[4]) & kColorInfoIdentityMask) == 0u &&
           rtv[5] == slot.raw_regs[5] &&
           rtv[6] == slot.raw_regs[6] &&
           rtv[7] == slot.raw_regs[7] &&
           rtv[9] == slot.raw_regs[9];
}

inline bool MatchesDepthRegs(const SlotStorage& slot, const u32* dt) noexcept {
    return dt[0] == slot.raw_regs[0] &&
           dt[1] == slot.raw_regs[1] &&
           dt[2] == slot.raw_regs[2] &&
           dt[3] == slot.raw_regs[3] &&
           dt[6] == slot.raw_regs[6] &&
           dt[7] == slot.raw_regs[7] &&
           dt[8] == slot.raw_regs[8] &&
           dt[9] == slot.raw_regs[9] &&
           dt[12] == slot.raw_regs[12];
}

u32 AllocateNextUid(RegistryState& st) noexcept {
    const u32 idx = (st.next_cursor++ % (kMaxTargets - 1u)) + 1u;
    return idx;
}

void PublishLookup(LookupBucket& bucket, u64 packed) noexcept {
    const u64 prefix = packed & ~0xFFFFULL;
    const u64 cur0 = bucket.way0.load(std::memory_order_relaxed);
    if (cur0 == 0ULL || (cur0 & ~0xFFFFULL) == prefix) {
        bucket.way0.store(packed, std::memory_order_release);
        return;
    }
    const u64 cur1 = bucket.way1.load(std::memory_order_relaxed);
    if (cur1 == 0ULL || (cur1 & ~0xFFFFULL) == prefix) {
        bucket.way1.store(packed, std::memory_order_release);
        return;
    }
    bucket.way1.store(cur0, std::memory_order_release);
    bucket.way0.store(packed, std::memory_order_release);
}

} // namespace

bool ReadTargetSlot(u32 uid, TargetSnapshot& out) noexcept {
    if (uid == 0u || uid >= kMaxTargets) {
        return false;
    }
    const SlotStorage& slot = State().slots[uid];
    for (u32 attempt = 0; attempt < 4u; ++attempt) {
        const u32 s1 = slot.seq.load(std::memory_order_acquire);
        if (s1 == 0u || (s1 & 1u) != 0u) {
            return false;
        }
        out.epoch = s1;
        out.is_depth = slot.is_depth != 0u;
        out.has_parent_tsharp = slot.has_parent_tsharp != 0u;
        out.mip_level = slot.mip_level;
        out.first_slice = slot.first_slice;
        out.slice_count = slot.slice_count;
        out.raw_regs = slot.raw_regs;
        if (out.has_parent_tsharp) {
            out.parent_tsharp = slot.parent_tsharp;
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        const u32 s2 = slot.seq.load(std::memory_order_relaxed);
        if (s1 == s2) {
            return true;
        }
    }
    return false;
}

u32 LookupColorTargetUid(const u32* gnm_rt_11dw) noexcept {
    if (!gnm_rt_11dw || gnm_rt_11dw[0] == 0u) {
        return 0u;
    }
    auto& st = State();
    const LookupBucket& bucket = st.color_lookup[HashPtrBucket(gnm_rt_11dw)];
    const u64 prefix = PackPtrPrefix(gnm_rt_11dw);

    const auto check_way = [&](u64 packed) noexcept -> u32 {
        if ((packed & ~0xFFFFULL) != prefix) {
            return 0u;
        }
        const u32 uid = static_cast<u32>(packed & 0xFFFu);
        if (uid == 0u || uid >= kMaxTargets) {
            return 0u;
        }
        const SlotStorage& slot = st.slots[uid];
        const u32 s1 = slot.seq.load(std::memory_order_acquire);
        if (s1 == 0u || (s1 & 1u) != 0u || slot.is_depth != 0u) {
            return 0u;
        }
        if (!MatchesColorRegs(slot, gnm_rt_11dw)) {
            return 0u;
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        return (slot.seq.load(std::memory_order_relaxed) == s1) ? uid : 0u;
    };

    if (const u32 uid0 = check_way(bucket.way0.load(std::memory_order_acquire))) {
        return uid0;
    }
    return check_way(bucket.way1.load(std::memory_order_acquire));
}

u32 LookupDepthTargetUid(const u32* gnm_dt_13dw) noexcept {
    if (!gnm_dt_13dw || (gnm_dt_13dw[2] == 0u && gnm_dt_13dw[3] == 0u)) {
        return 0u;
    }
    auto& st = State();
    const LookupBucket& bucket = st.depth_lookup[HashPtrBucket(gnm_dt_13dw)];
    const u64 prefix = PackPtrPrefix(gnm_dt_13dw);

    const auto check_way = [&](u64 packed) noexcept -> u32 {
        if ((packed & ~0xFFFFULL) != prefix) {
            return 0u;
        }
        const u32 uid = static_cast<u32>(packed & 0xFFFu);
        if (uid == 0u || uid >= kMaxTargets) {
            return 0u;
        }
        const SlotStorage& slot = st.slots[uid];
        const u32 s1 = slot.seq.load(std::memory_order_acquire);
        if (s1 == 0u || (s1 & 1u) != 0u || slot.is_depth == 0u) {
            return 0u;
        }
        if (!MatchesDepthRegs(slot, gnm_dt_13dw)) {
            return 0u;
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        return (slot.seq.load(std::memory_order_relaxed) == s1) ? uid : 0u;
    };

    if (const u32 uid0 = check_way(bucket.way0.load(std::memory_order_acquire))) {
        return uid0;
    }
    return check_way(bucket.way1.load(std::memory_order_acquire));
}

u32 RegisterColorTarget(const u32* gnm_rt_11dw, const u32* parent_tsharp_8dw,
                        u32 mip_level, u32 first_slice, u32 slice_count) noexcept {
    if (!gnm_rt_11dw || gnm_rt_11dw[0] == 0u) {
        return 0u;
    }
    if (const u32 existing = LookupColorTargetUid(gnm_rt_11dw)) {
        if (!parent_tsharp_8dw || State().slots[existing].has_parent_tsharp != 0u) {
            return existing;
        }
    }
    auto& st = State();
    std::scoped_lock lk{st.write_mutex};

    // Check if this exact guest pointer already owns a slot we can update in-place.
    LookupBucket& bucket = st.color_lookup[HashPtrBucket(gnm_rt_11dw)];
    const u64 prefix = PackPtrPrefix(gnm_rt_11dw);
    u32 uid = 0u;
    const u64 w0 = bucket.way0.load(std::memory_order_relaxed);
    const u64 w1 = bucket.way1.load(std::memory_order_relaxed);
    if ((w0 & ~0xFFFFULL) == prefix) {
        uid = static_cast<u32>(w0 & 0xFFFu);
    } else if ((w1 & ~0xFFFFULL) == prefix) {
        uid = static_cast<u32>(w1 & 0xFFFu);
    }
    if (uid == 0u || uid >= kMaxTargets) {
        uid = AllocateNextUid(st);
    }

    SlotStorage& slot = st.slots[uid];
    const u32 prev_seq = slot.seq.load(std::memory_order_relaxed);
    const u32 odd_seq = (prev_seq & ~1u) + 1u;
    slot.seq.store(odd_seq, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_acq_rel);

    slot.is_depth = 0u;
    slot.has_parent_tsharp = parent_tsharp_8dw ? 1u : 0u;
    slot.mip_level = static_cast<u16>(mip_level);
    slot.first_slice = static_cast<u16>(first_slice);
    slot.slice_count = static_cast<u16>(slice_count ? slice_count : 1u);
    slot.raw_regs.fill(0u);
    std::memcpy(slot.raw_regs.data(), gnm_rt_11dw, 11u * sizeof(u32));
    if (parent_tsharp_8dw) {
        std::memcpy(slot.parent_tsharp.data(), parent_tsharp_8dw, 8u * sizeof(u32));
    } else {
        slot.parent_tsharp.fill(0u);
    }

    slot.seq.store(odd_seq + 1u, std::memory_order_release);
    PublishLookup(bucket, PackPtrAndUid(gnm_rt_11dw, uid));
    return uid;
}

u32 RegisterDepthTarget(const u32* gnm_dt_13dw, const u32* parent_tsharp_8dw,
                        u32 mip_level, u32 first_slice, u32 slice_count) noexcept {
    if (!gnm_dt_13dw || (gnm_dt_13dw[2] == 0u && gnm_dt_13dw[3] == 0u)) {
        return 0u;
    }
    if (const u32 existing = LookupDepthTargetUid(gnm_dt_13dw)) {
        if (!parent_tsharp_8dw || State().slots[existing].has_parent_tsharp != 0u) {
            return existing;
        }
    }
    auto& st = State();
    std::scoped_lock lk{st.write_mutex};

    LookupBucket& bucket = st.depth_lookup[HashPtrBucket(gnm_dt_13dw)];
    const u64 prefix = PackPtrPrefix(gnm_dt_13dw);
    u32 uid = 0u;
    const u64 w0 = bucket.way0.load(std::memory_order_relaxed);
    const u64 w1 = bucket.way1.load(std::memory_order_relaxed);
    if ((w0 & ~0xFFFFULL) == prefix) {
        uid = static_cast<u32>(w0 & 0xFFFu);
    } else if ((w1 & ~0xFFFFULL) == prefix) {
        uid = static_cast<u32>(w1 & 0xFFFu);
    }
    if (uid == 0u || uid >= kMaxTargets) {
        uid = AllocateNextUid(st);
    }

    SlotStorage& slot = st.slots[uid];
    const u32 prev_seq = slot.seq.load(std::memory_order_relaxed);
    const u32 odd_seq = (prev_seq & ~1u) + 1u;
    slot.seq.store(odd_seq, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_acq_rel);

    slot.is_depth = 1u;
    slot.has_parent_tsharp = parent_tsharp_8dw ? 1u : 0u;
    slot.mip_level = static_cast<u16>(mip_level);
    slot.first_slice = static_cast<u16>(first_slice);
    slot.slice_count = static_cast<u16>(slice_count ? slice_count : 1u);
    std::memcpy(slot.raw_regs.data(), gnm_dt_13dw, 13u * sizeof(u32));
    if (parent_tsharp_8dw) {
        std::memcpy(slot.parent_tsharp.data(), parent_tsharp_8dw, 8u * sizeof(u32));
    } else {
        slot.parent_tsharp.fill(0u);
    }

    slot.seq.store(odd_seq + 1u, std::memory_order_release);
    PublishLookup(bucket, PackPtrAndUid(gnm_dt_13dw, uid));
    return uid;
}

} // namespace Vulkan::DirectTargets

extern "C" {

uint32_t bb_direct_vk_register_color_target(const void* gnm_rt_11dw,
                                            const void* parent_tsharp_8dw,
                                            uint32_t mip_level,
                                            uint32_t first_slice,
                                            uint32_t slice_count) noexcept {
    return Vulkan::DirectTargets::RegisterColorTarget(
        static_cast<const u32*>(gnm_rt_11dw),
        static_cast<const u32*>(parent_tsharp_8dw),
        mip_level, first_slice, slice_count);
}

uint32_t bb_direct_vk_register_depth_target(const void* gnm_dt_13dw,
                                            const void* parent_tsharp_8dw,
                                            uint32_t mip_level,
                                            uint32_t first_slice,
                                            uint32_t slice_count) noexcept {
    return Vulkan::DirectTargets::RegisterDepthTarget(
        static_cast<const u32*>(gnm_dt_13dw),
        static_cast<const u32*>(parent_tsharp_8dw),
        mip_level, first_slice, slice_count);
}

uint32_t bb_direct_vk_lookup_color_target_uid(const void* gnm_rt_11dw) noexcept {
    return Vulkan::DirectTargets::LookupColorTargetUid(static_cast<const u32*>(gnm_rt_11dw));
}

uint32_t bb_direct_vk_lookup_depth_target_uid(const void* gnm_dt_13dw) noexcept {
    return Vulkan::DirectTargets::LookupDepthTargetUid(static_cast<const u32*>(gnm_dt_13dw));
}

} // extern "C"
