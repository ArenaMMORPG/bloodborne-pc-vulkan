// SPDX-License-Identifier: GPL-2.0-or-later
// V3.1: Direct RenderTarget & DepthTarget Registration and O(1) Lookup Table.
#pragma once

#include <array>
#include <cstdint>
#include "common/types.h"

namespace Vulkan::DirectTargets {

inline constexpr u32 kMaxTargets = 4096u; // UID 0 = unregistered, 1..4095 = registered slots

struct TargetSnapshot {
    u32 epoch{0};
    bool is_depth{false};
    bool has_parent_tsharp{false};
    u16 mip_level{0};
    u16 first_slice{0};
    u16 slice_count{1};
    std::array<u32, 13> raw_regs{};
    std::array<u32, 8> parent_tsharp{};
};

/// Thread-safe lock-free read of a registered DirectTarget slot by 12-bit UID (1..4095).
bool ReadTargetSlot(u32 uid, TargetSnapshot& out) noexcept;

/// Register a Color RenderTarget (11 dwords: CB_COLORn_BASE..CLEAR_WORD1, with extent in [6])
/// and optional 8-dword parent Gcn::Image T# (for multi-mip color surfaces).
u32 RegisterColorTarget(const u32* gnm_rt_11dw, const u32* parent_tsharp_8dw,
                        u32 mip_level, u32 first_slice, u32 slice_count) noexcept;

/// Register a Depth RenderTarget (13 dwords: DB_DEPTH_INFO..DB_EXTENT)
u32 RegisterDepthTarget(const u32* gnm_dt_13dw, const u32* parent_tsharp_8dw,
                        u32 mip_level, u32 first_slice, u32 slice_count) noexcept;

/// Lock-free O(1) L1 lookup of Color RenderTarget UID at bind time (EmitSetRenderTarget).
u32 LookupColorTargetUid(const u32* gnm_rt_11dw) noexcept;

/// Lock-free O(1) L1 lookup of Depth RenderTarget UID at bind time (EmitSetDepthRenderTarget).
u32 LookupDepthTargetUid(const u32* gnm_dt_13dw) noexcept;

} // namespace Vulkan::DirectTargets

extern "C" {
uint32_t bb_direct_vk_register_color_target(const void* gnm_rt_11dw,
                                            const void* parent_tsharp_8dw,
                                            uint32_t mip_level,
                                            uint32_t first_slice,
                                            uint32_t slice_count) noexcept;

uint32_t bb_direct_vk_register_depth_target(const void* gnm_dt_13dw,
                                            const void* parent_tsharp_8dw,
                                            uint32_t mip_level,
                                            uint32_t first_slice,
                                            uint32_t slice_count) noexcept;

uint32_t bb_direct_vk_lookup_color_target_uid(const void* gnm_rt_11dw) noexcept;
uint32_t bb_direct_vk_lookup_depth_target_uid(const void* gnm_dt_13dw) noexcept;
}
