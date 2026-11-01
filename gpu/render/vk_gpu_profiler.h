// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: BB_GPU_PROFILE=1 — GPU time per render pass, dispatch and upscaler run. A timestamp is
// written where each of them starts (and at the frame end); the time to the next timestamp is
// charged to its label, barriers and copies recorded in between included. Results are read four
// frames later and printed every 5 s: GPU ms per frame by label.

#pragma once

#include <array>
#include <chrono>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "vulkan/vk_common.h"

namespace Vulkan {

class Instance;
class Scheduler;

class GpuProfiler {
public:
    /// The profiler when BB_GPU_PROFILE=1, else null.
    static GpuProfiler* Get() noexcept {
        return instance_ptr;
    }
    static void Init(const Instance& instance, Scheduler& scheduler);

    /// Starts a segment labelled `key`; `describe` names it the first time the key is seen.
    template <typename Describe>
    void Mark(u64 key, Describe&& describe) {
        if (!described.contains(key)) {
            described.emplace(key, describe());
        }
        WriteTimestamp(key);
    }

    /// The label of the open segment, for Resume after a nested segment (transfers).
    [[nodiscard]] u64 Current() const noexcept {
        return current;
    }
    /// Continues the segment of `key` (a label seen before) where a nested one ends.
    void Resume(u64 key) {
        WriteTimestamp(key);
    }

    /// Whether `other` is the scheduler it records into (the presenter has its own).
    [[nodiscard]] bool Records(const Scheduler* other) const noexcept {
        return other == &scheduler;
    }

    /// Block 5, Step 5.4: Zero-pass-break per-(VS, PS, WxH, MRTs) draw & vertex/index counter
    /// recorded only while GpuProfiler capture is active (exposes top shaders inside merged passes).
    void NoteDraw(u64 vs_hash, u64 ps_hash, u32 width, u32 height, u32 num_mrts, u64 vtx_instances);

    /// At the display pass: closes the frame's last segment, reads an old frame's results.
    void BeginFrame();
    static void OnFrame();

    /// V7.3: Zero-RenderPass-break submission-boundary GPU busy/gap meter for the HUD.
    static void HudBeginSubmission(const Scheduler* sched);
    static void HudEndSubmission(const Scheduler* sched, u64 tick, bool is_frame_end);

private:
    GpuProfiler(const Instance& instance, Scheduler& scheduler);
    void StartCapture();
    void StopCapture();
    void WriteTimestamp(u64 key);
    void Collect(u32 slice);
    void Print(bool to_file = false);
    [[nodiscard]] bool TryCollectHudSlice(u32 s);

    static constexpr u32 NumSlices = 4;
    static constexpr u32 SliceQueries = 4096;
    static constexpr u32 HudSlices = 8;
    static constexpr u32 HudSliceQueries = 128;
    static inline GpuProfiler* instance_ptr = nullptr;
    static inline GpuProfiler* stored_ptr = nullptr;
    bool permanent = false;
    bool hud_enabled = false;
    u64 seen_frames = 0;

    vk::Device device;
    Scheduler& scheduler;
    vk::UniqueQueryPool pool;
    vk::UniqueQueryPool hud_pool;
    double period_ns = 1.0;
    u32 slice = 0;
    std::array<std::vector<u64>, NumSlices> keys; ///< label of each timestamp but the last
    std::array<u32, NumSlices> used{};
    std::array<bool, NumSlices> pending{};
    u32 hud_slice = 0;
    bool hud_subm_open = false;
    bool hud_slice_blocked = false;
    bool hud_has_prev_end = false;
    u64 hud_prev_end_stamp = 0;
    std::array<u32, HudSlices> hud_used{};
    std::array<u64, HudSlices> hud_last_tick{};
    std::array<bool, HudSlices> hud_pending{};
    std::unordered_map<u64, std::string> described;
    struct Total {
        double ms = 0;
        u64 segments = 0;
    };
    std::unordered_map<u64, Total> totals;
    struct DrawShaderStat {
        u64 vs_hash = 0;
        u64 ps_hash = 0;
        u32 width = 0;
        u32 height = 0;
        u32 num_mrts = 0;
        u64 draws = 0;
        u64 vtx_instances = 0;
    };
    std::unordered_map<u64, DrawShaderStat> draw_stats;
    u64 draw_frames = 0;
    static constexpr u64 SubmissionGapKey = 0x5B317ull; ///< Scheduler's "between submissions"
    std::vector<double> gaps;
    u64 frames = 0;
    u64 current = 0;
    std::chrono::steady_clock::time_point window = std::chrono::steady_clock::now();
};

} // namespace Vulkan
