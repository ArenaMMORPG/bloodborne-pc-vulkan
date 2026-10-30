// SPDX-License-Identifier: GPL-2.0-or-later
// bbport BB_SECTIONS=1 (diagnostics): time the draw recording thread (and, "A:" parts, the GPU
// command thread) spends per part of a draw, inclusive (nested parts count in their parent too),
// printed with the draw pipe statistics.
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <x86intrin.h>

namespace BbSections {
enum Id : std::uint32_t {
    Packet,          ///< a whole packet of the draw pipe
    Task,            ///< ordered tasks (fences, signals, DMA, flips)
    DrawRecord,
    PrepareRenderState,
    BindResources,
    BindBuffers,
    BindTextures,
    BeginRendering,
    ResolveVertexBuffers,
    ResolveIndexBuffer,
    EmitVertexBuffers,
    UpdateDynamicState,
    DispatchRecord,
    ShaderHle,
    ObtainBuffer,
    Flush,
    Motion,          ///< object motion (bone palettes, index ranges)
    PipelineBind,    ///< Pipeline::BindResources (descriptors, push constants)
    Kick,            ///< ResetBindings and the hand-over to the Vulkan recording threads
    Preupload,
    EnsureResident,
    ObtainStream,  ///< ObtainBuffer's copy of a small read-only buffer into the stream buffer
    ObtainVram,    ///< ObtainBuffer's VRAM path (SynchronizeMemory)
    LayerBind,     ///< BB_LAYER_MEMORY: BufferCache::LayerBind
    // The GPU command thread (stage A), per draw handed over.
    ADraw,         ///< Rasterizer::Draw: the pipeline selection and the hand-over
    APipeline,     ///< PipelineCache::GetGraphicsPipeline
    APrepared,     ///< its prepared-draw check (TryPreparedPipeline)
    APostDraw,     ///< packing the draw for the recording thread
    ARing,         ///< constants copied into the ring (CollectRingBindings)
    ARingChecks,   ///< its checks of each buffer (GPU-written, pending writes, VRAM copies)
    ARingCopy,     ///< its copies (memcpy and flush)
    ARingSharp,    ///< its V# reads and range clamps
    ARingNote,     ///< its pending read notes (NotePendingRead)
    ADecode,       ///< decoding a whole submission (everything of stage A)
    Count,
};
inline constexpr const char* Names[Count] = {
    "packet",       "tasks",       "draw",          "render state", "bind",
    "buffers",      "textures",    "begin render",  "vertex",       "index",
    "emit vertex",  "dynamic",     "dispatch",      "HLE shader",   "ObtainBuffer",
    "flush",        "motion",      "descriptors",   "kick",         "preupload",
    "EnsureResident", "stream copy", "VRAM path", "layer bind",
    "A: draw", "A: pipeline", "A: prepared", "A: post", "A: ring", "A: ring checks",
    "A: ring copy", "A: ring sharp", "A: ring note", "A: decode",
};
// Per-thread counters on their own cache lines: stage A and stage B run on different cores (often
// different CCDs) and a scope ends ~20 times per draw on each; a line written by both bounced
// between them on every draw (~100 ns over the Infinity Fabric each time).
struct alignas(64) Counters {
    std::array<std::atomic<std::uint64_t>, Count> v{};
    std::atomic<std::uint64_t>& operator[](std::size_t i) { return v[i]; }
    const std::atomic<std::uint64_t>& operator[](std::size_t i) const { return v[i]; }
};
struct alignas(64) PaddedCounter {
    std::atomic<std::uint64_t> value{0};
    std::uint64_t load(std::memory_order order = std::memory_order_seq_cst) const {
        return value.load(order);
    }
    void store(std::uint64_t v, std::memory_order order = std::memory_order_seq_cst) {
        value.store(v, order);
    }
    std::uint64_t fetch_add(std::uint64_t v, std::memory_order order = std::memory_order_seq_cst) {
        return value.fetch_add(v, order);
    }
};
inline Counters cycles_a{}; ///< written by the GPU command thread only
inline Counters cycles_b{}; ///< written by the draw recording thread only
/// Both stages summed (read side only).
struct CyclesSum {
    struct Ref {
        std::size_t i;
        std::uint64_t load(std::memory_order order = std::memory_order_relaxed) const {
            return cycles_a[i].load(order) + cycles_b[i].load(order);
        }
    };
    Ref operator[](std::size_t i) const { return {i}; }
};
inline constexpr CyclesSum cycles{};
/// Constant buffers stage A looked at and bytes it copied into the ring (with the sections).
inline PaddedCounter ring_buffers{}, ring_copies{}, ring_bytes{};
/// DrawPipe live counters mirrored for the in-game frame breakdown HUD.
inline PaddedCounter pipe_packets{}, pipe_drains{}, pipe_drain_cycles{}, pipe_busy_cycles{};

inline bool Enabled() {
    static const bool on = [] {
        const char* env = std::getenv("BB_SECTIONS");
        return !env || env[0] != '0';
    }();
    return on;
}
/// Set on the draw recording thread (DrawPipe::Run) and the GPU command thread: only their time
/// is counted.
inline thread_local bool recording_thread = false;
inline thread_local bool command_thread = false;

struct Scope {
    explicit Scope(Id id_)
        : id{id_}, start{Enabled() && (recording_thread || command_thread) ? __rdtsc() : 0} {}
    ~Scope() {
        if (start) {
            const std::uint64_t delta = __rdtsc() - start;
            auto& counters = recording_thread ? cycles_b : cycles_a;
            counters[id].store(counters[id].load(std::memory_order_relaxed) + delta,
                               std::memory_order_relaxed);
        }
    }
    Id id;
    std::uint64_t start;
};
} // namespace BbSections

#define BB_SECTION_JOIN2(a, b) a##b
#define BB_SECTION_JOIN(a, b) BB_SECTION_JOIN2(a, b)
#define BB_SECTION(id) BbSections::Scope BB_SECTION_JOIN(bb_section_scope_, __LINE__){BbSections::id}
