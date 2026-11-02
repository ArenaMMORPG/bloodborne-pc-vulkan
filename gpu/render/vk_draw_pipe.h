// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: two-stage draw pipeline (docs/parallel_gpu.md, "Two-stage draw pipeline").
//
// The GPU command thread (stage A) decodes PM4, keeps the register file and selects pipelines;
// for a direct draw it writes a packet (the register blocks changed since the previous packet,
// the stages' user data, the draw parameters) into this ring and goes on decoding. The draw
// recording thread (stage B) owns the caches, the barriers and the command recording while
// packets are in flight: it applies each packet to its own copy of the registers and runs the
// rest of the draw. Anything else stage A does on that state waits for stage B to run dry
// (Drain) and then runs on stage A as before.

#pragma once

#include <cstdlib>
#include "bbport_threads.h"
#include "bbport_sections.h"
#include "bbport_timeline.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <cstdio>
#include <thread>
#include <vector>
#include <x86intrin.h>

#include "common/assert.h"
#include "common/thread.h"
#include "common/types.h"

namespace Vulkan {

class DrawPipe {
public:
    /// Runs a packet on stage B.
    using Handler = void (*)(void* context, const u8* packet, u32 size);

    DrawPipe(Handler handler_, void* context_) : handler{handler_}, context{context_} {
        ring = std::make_unique<u8[]>(Capacity);
        thread = std::jthread([this](std::stop_token stop) { Run(stop); });
    }

    ~DrawPipe() {
        Drain();
        for (auto& worker : resolvers) {
            worker.request_stop();
        }
        resolve_wake.fetch_add(1, std::memory_order_seq_cst);
        resolve_wake.notify_all();
        resolvers.clear(); // joined before the ring goes
        thread.request_stop();
        published.fetch_add(0, std::memory_order_seq_cst);
        wake.fetch_add(1, std::memory_order_seq_cst);
        wake.notify_one();
    }

    /// True on the draw recording thread.
    [[nodiscard]] static bool OnStageB() noexcept {
        return on_stage_b;
    }

    [[nodiscard]] u32 StageBThreadId() const noexcept {
        return stage_b_tid.load(std::memory_order_acquire);
    }

    /// Stage A: space for a packet of `size` bytes (8-byte aligned), valid until Commit().
    u8* Begin(u32 size) {
        size = Align(size + sizeof(Header));
        ASSERT(size <= Capacity / 4);
        u64 at = head;
        const u64 offset = at % Capacity;
        if (offset + size > Capacity) {
            // Wrap: the rest of the ring is skipped (a header with zero size marks it).
            WaitForSpace(at, Capacity - offset);
            reinterpret_cast<Header*>(ring.get() + offset)->size = 0;
            at += Capacity - offset;
            head = at;
            published.store(at, std::memory_order_release);
        }
        WaitForSpace(at, size);
        pending_size = size;
        auto* header = reinterpret_cast<Header*>(ring.get() + at % Capacity);
        header->size = size;
        return reinterpret_cast<u8*>(header + 1);
    }

    /// Stage A: hands the packet from Begin() to stage B. `resolve`: a resolve worker completes
    /// it first (StartResolvers; without workers the flag is ignored).
    void Commit(u32 payload_size, bool resolve = false) {
        reinterpret_cast<Header*>(ring.get() + head % Capacity)->payload = payload_size;
        const bool has_resolvers = (num_resolvers != 0);
        if (has_resolvers) {
            auto& slot = slots[(packets + 1) & SlotMask];
            slot.at = head;
            slot.state.store(resolve ? SlotPending : SlotDone, std::memory_order_release);
        }
        head += pending_size;
        published.store(head, std::memory_order_release);
        ++packets;
        BbSections::pipe_packets.store(packets, std::memory_order_relaxed);
        if (has_resolvers) {
            published_packets.store(packets, std::memory_order_release);
        }
        // Single StoreLoad barrier pairing with sleeping/resolvers_sleeping seq_cst stores instead
        // of 4 separate `memory_order_seq_cst` XCHG instructions per draw packet.
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if (has_resolvers && resolvers_sleeping.load(std::memory_order_relaxed)) [[unlikely]] {
            const u64 backlog_pkts = packets - consumed_packets.load(std::memory_order_relaxed);
            if (!resolve ? false : (backlog_pkts <= 4 || (packets & 15u) == 0)) {
                resolve_wake.fetch_add(1, std::memory_order_release);
                resolve_wake.notify_all();
            }
        }
        if (sleeping.load(std::memory_order_relaxed)) [[unlikely]] {
            wake.fetch_add(1, std::memory_order_release);
            wake.notify_one();
        }
    }

    void WakeResolversIfSleeping() noexcept {
        if (num_resolvers != 0 && resolvers_sleeping.load(std::memory_order_acquire)) [[unlikely]] {
            resolve_wake.fetch_add(1, std::memory_order_release);
            resolve_wake.notify_all();
        }
    }

    /// Stage 2 (docs/pipeline_4stage.ru.md): `count` threads complete the packets committed with
    /// `resolve` in packet order of claim (each worker sees its own packets in increasing order),
    /// before stage B runs them. Called once, before any packet.
    using Resolver = void (*)(void* context, u8* packet, u32 size, u64 number, u32 worker);
    void StartResolvers(Resolver resolver_, u32 count) {
        resolver = resolver_;
        num_resolvers = count;
        slots = std::make_unique<Slot[]>(SlotMask + 1);
        for (u32 i = 0; i < count; ++i) {
            resolvers.emplace_back([this, i](std::stop_token stop) { ResolverRun(stop, i); });
        }
    }
    [[nodiscard]] u32 ResolverCount() const noexcept {
        return static_cast<u32>(resolvers.size());
    }
    /// Stage A: waits until every packet up to `number` is resolved.
    void WaitResolved(u64 number) {
        if (resolvers.empty()) {
            return;
        }
        number = std::min(number, packets);
        if (resolved_prefix < number) {
            WakeResolversIfSleeping();
        }
        while (resolved_prefix < number) {
            if (slots[(resolved_prefix + 1) & SlotMask].state.load(std::memory_order_acquire) ==
                SlotDone) {
                ++resolved_prefix;
            } else {
                WakeResolversIfSleeping();
                ++resolve_waits;
                __builtin_ia32_pause();
            }
        }
    }
    u64 resolve_waits = 0; ///< statistics (stage A): spins waiting for resolve workers

    /// Why stage A drained (statistics): a PM4 opcode, or one of these.
    enum Reason : u32 {
        ReasonConstRam = 256,
        ReasonCommands,
        ReasonCompute,
        ReasonSubmissionEnd,
        ReasonDraw,
        ReasonRasterizer,
        NumReasons,
    };
    std::array<u64, NumReasons> drains_by_reason{};
    std::array<u64, NumReasons> cycles_by_reason{};

    /// Stage A: waits until stage B has run every committed packet.
    void Drain(u32 reason = ReasonRasterizer) {
        if (consumed.load(std::memory_order_acquire) == head) {
            return;
        }
        WakeResolversIfSleeping();
        ++drains;
        const u32 slot = reason < NumReasons ? reason : ReasonRasterizer;
        ++drains_by_reason[slot];
        const u64 start = __rdtsc();
        for (u32 spins = 0; consumed.load(std::memory_order_acquire) != head; ++spins) {
            if (spins < 4096) {
                __builtin_ia32_pause();
            } else {
                std::this_thread::yield();
            }
        }
        const u64 waited = __rdtsc() - start;
        drain_cycles += waited;
        cycles_by_reason[slot] += waited;
        BbSections::pipe_drains.store(drains, std::memory_order_relaxed);
        BbSections::pipe_drain_cycles.store(drain_cycles, std::memory_order_relaxed);
    }

    /// Stage A: the number the next committed packet gets (packets are numbered from 1), and
    /// whether stage B has run the packet with that number.
    [[nodiscard]] u64 NextPacket() const noexcept {
        return packets + 1;
    }
    [[nodiscard]] bool ReachedPacket(u64 number) const noexcept {
        return consumed_packets.load(std::memory_order_acquire) >= number;
    }
    [[nodiscard]] u64 ConsumedPackets() const noexcept {
        return consumed_packets.load(std::memory_order_acquire);
    }
    /// Stage B: the number of the packet it runs.
    [[nodiscard]] u64 RunningPacket() const noexcept {
        return consumed_packets.load(std::memory_order_relaxed) + 1;
    }
    /// Stage A: waits until stage B has run the packet `number` (Drain: all of them).
    void WaitForPacket(u64 number, u32 reason) {
        if (ReachedPacket(number)) {
            return;
        }
        WakeResolversIfSleeping();
        ++drains;
        const u32 slot = reason < NumReasons ? reason : ReasonRasterizer;
        ++drains_by_reason[slot];
        const u64 start = __rdtsc();
        for (u32 spins = 0; !ReachedPacket(number); ++spins) {
            if (spins < 4096) {
                __builtin_ia32_pause();
            } else {
                std::this_thread::yield();
            }
        }
        const u64 waited = __rdtsc() - start;
        drain_cycles += waited;
        cycles_by_reason[slot] += waited;
        BbSections::pipe_drains.store(drains, std::memory_order_relaxed);
        BbSections::pipe_drain_cycles.store(drain_cycles, std::memory_order_relaxed);
    }

    /// Stage A: position after the last committed packet; Reached(position) once B ran it.
    [[nodiscard]] u64 Head() const noexcept {
        return head;
    }
    [[nodiscard]] bool Reached(u64 position) const noexcept {
        return consumed.load(std::memory_order_acquire) >= position;
    }

    [[nodiscard]] bool Idle() const noexcept {
        return consumed.load(std::memory_order_acquire) == head;
    }
    /// Stage B: the position of the packet it runs (packets before it have run).
    [[nodiscard]] u64 Consumed() const noexcept {
        return consumed.load(std::memory_order_acquire);
    }
    /// Stage B: bytes of packets committed and not run yet (the work queued for this thread).
    [[nodiscard]] u64 Backlog() const noexcept {
        return published.load(std::memory_order_acquire) - consumed.load(std::memory_order_relaxed);
    }
    /// Stage B (inside `handler`): returns the next already-published packet's payload if ready,
    /// or nullptr if no packet is queued after the currently running one.
    [[nodiscard]] const u8* PeekNext(u32& out_payload_size) const noexcept {
        const u64 avail = published.load(std::memory_order_acquire);
        u64 next_at = stage_b_next_at;
        if (next_at >= avail) {
            return nullptr;
        }
        const auto* hdr = reinterpret_cast<const Header*>(ring.get() + next_at % Capacity);
        if (hdr->size == 0) {
            next_at += Capacity - next_at % Capacity;
            if (next_at >= avail) {
                return nullptr;
            }
            hdr = reinterpret_cast<const Header*>(ring.get() + next_at % Capacity);
            if (hdr->size == 0) {
                return nullptr;
            }
        }
        out_payload_size = hdr->payload;
        return reinterpret_cast<const u8*>(hdr + 1);
    }

    /// Statistics (stage A): packets, drains that had to wait, cycles waited, cycles stage B
    /// spent running packets.
    u64 packets = 0, drains = 0, drain_cycles = 0;
    std::atomic<u64> busy_cycles{0};

private:
    struct Header {
        u32 size;    ///< bytes to the next packet (0: wrap to the ring start)
        u32 payload; ///< bytes written by the producer
    };
    static constexpr u64 Capacity = 16ull << 20;

    static constexpr u32 Align(u32 size) {
        return (size + 63) & ~63u;
    }

    void WaitForSpace(u64 at, u64 size) {
        while (at + size - consumed.load(std::memory_order_acquire) > Capacity) {
            __builtin_ia32_pause();
        }
    }

    void Run(std::stop_token stop) {
        Common::SetCurrentThreadName("bb:DrawRec");
        BbThreads::PinToGpuDomain();
        on_stage_b = true;
        BbSections::recording_thread = true;
        stage_b_tid.store(static_cast<u32>(gettid()), std::memory_order_release);
        u64 at = 0;
        while (true) {
            // Packets follow each other within microseconds while a frame is decoded: spin,
            // and sleep only after a longer pause (between frames).
            u64 available = published.load(std::memory_order_acquire);
            if (available == at) {
                BbTimeline::Note(BbTimeline::RecorderIdleBegin);
                // With few hardware threads (Steam Deck: 8) a long spin takes time from the
                // game's own threads. BB_PIPE_SPIN_US overrides.
                static const auto spin_time = std::chrono::microseconds([] {
                    if (const char* env = std::getenv("BB_PIPE_SPIN_US")) {
                        return std::max(0, std::atoi(env));
                    }
                    return BbThreads::Available() >= 12 ? 25 : 15;
                }());
                const auto spin_until = std::chrono::steady_clock::now() + spin_time;
                for (u32 spins = 1; available == at; ++spins) {
                    if (stop.stop_requested()) {
                        return;
                    }
                    __builtin_ia32_pause();
                    if (!(spins & 255) && std::chrono::steady_clock::now() >= spin_until) {
                        const u32 seen = wake.load(std::memory_order_seq_cst);
                        sleeping.store(true, std::memory_order_seq_cst);
                        if (published.load(std::memory_order_seq_cst) == at &&
                            !stop.stop_requested()) {
                            wake.wait(seen, std::memory_order_seq_cst);
                        }
                        sleeping.store(false, std::memory_order_relaxed);
                    }
                    available = published.load(std::memory_order_acquire);
                }
                BbTimeline::Note(BbTimeline::RecorderIdleEnd);
            }
            const auto* header = reinterpret_cast<const Header*>(ring.get() + at % Capacity);
            if (header->size == 0) {
                at += Capacity - at % Capacity;
                consumed.store(at, std::memory_order_release);
                continue;
            }
            if (!resolvers.empty()) {
                // Its resolve worker may still be completing it.
                const auto& slot = slots[(consumed_packets.load(std::memory_order_relaxed) + 1) &
                                         SlotMask];
                if (slot.state.load(std::memory_order_acquire) != SlotDone) {
                    WakeResolversIfSleeping();
                    for (u32 spins = 0; slot.state.load(std::memory_order_acquire) != SlotDone;
                         ++spins) {
                        if ((spins & 63u) == 0) {
                            WakeResolversIfSleeping();
                        }
                        if (spins < 4096) {
                            __builtin_ia32_pause();
                        } else {
                            std::this_thread::yield();
                        }
                    }
                }
            }
            stage_b_next_at = at + header->size;
            const u64 start = __rdtsc();
            handler(context, reinterpret_cast<const u8*>(header + 1), header->payload);
            const u64 delta = __rdtsc() - start;
            const u64 total_busy = busy_cycles.fetch_add(delta, std::memory_order_relaxed) + delta;
            BbSections::pipe_busy_cycles.store(total_busy, std::memory_order_relaxed);
            at += header->size;
            consumed_packets.fetch_add(1, std::memory_order_release);
            consumed.store(at, std::memory_order_release);
        }
    }

    void ResolverRun(std::stop_token stop, u32 worker) {
        char name[16];
        std::snprintf(name, sizeof(name), "bb:Resolve%u", worker);
        Common::SetCurrentThreadName(name);
        BbThreads::PinToGpuDomain();
        static const int spin_us = [] {
            if (const char* env = std::getenv("BB_RESOLVE_SPIN_US")) {
                return std::max(0, std::atoi(env));
            }
            return BbThreads::Available() >= 12 ? 20 : 10;
        }();
        static const auto spin_time = std::chrono::microseconds(spin_us);
        const bool single_worker = (num_resolvers == 1);
        u64 single_next = 1;
        while (!stop.stop_requested()) {
            u64 number = single_worker ? single_next : next_claim.load(std::memory_order_relaxed);
            if (number > published_packets.load(std::memory_order_acquire)) {
                const auto spin_until =
                    spin_us > 0 ? (std::chrono::steady_clock::now() + spin_time)
                                : std::chrono::steady_clock::time_point{};
                for (u32 spins = 1;
                     number > published_packets.load(std::memory_order_acquire); ++spins) {
                    if (stop.stop_requested()) {
                        return;
                    }
                    __builtin_ia32_pause();
                    const bool should_sleep =
                        spin_us == 0 ? ((spins & 7u) == 0)
                                     : (!(spins & 255u) &&
                                        std::chrono::steady_clock::now() >= spin_until);
                    if (should_sleep) {
                        const u32 seen = resolve_wake.load(std::memory_order_seq_cst);
                        resolvers_sleeping.fetch_add(1, std::memory_order_seq_cst);
                        if (number > published_packets.load(std::memory_order_seq_cst) &&
                            !stop.stop_requested()) {
                            resolve_wake.wait(seen, std::memory_order_seq_cst);
                        }
                        resolvers_sleeping.fetch_sub(1, std::memory_order_relaxed);
                    }
                    if (!single_worker) {
                        number = next_claim.load(std::memory_order_relaxed);
                    }
                }
                continue;
            }
            if (single_worker) {
                ++single_next;
            } else if (!next_claim.compare_exchange_weak(number, number + 1,
                                                         std::memory_order_acq_rel)) {
                continue;
            }
            auto& slot = slots[number & SlotMask];
            if (slot.state.load(std::memory_order_acquire) == SlotPending) {
                auto* header = reinterpret_cast<Header*>(ring.get() + slot.at % Capacity);
                resolver(context, reinterpret_cast<u8*>(header + 1), header->payload, number,
                         worker);
                slot.state.store(SlotDone, std::memory_order_release);
            }
        }
    }

    // Packet slots of the resolve stage: as many as packets fit in the ring (64-byte minimum).
    static constexpr u64 SlotMask = (Capacity / 64) - 1;
    enum : u32 { SlotPending = 1, SlotDone = 2 };
    struct Slot {
        u64 at = 0; ///< the packet's header position
        std::atomic<u32> state{0};
    };
    std::unique_ptr<Slot[]> slots;
    Resolver resolver = nullptr;
    u32 num_resolvers = 0;
    u64 resolved_prefix = 0; ///< stage A
    alignas(64) std::atomic<u64> published_packets{0};
    alignas(64) std::atomic<u64> next_claim{1};
    alignas(64) std::atomic<u32> resolve_wake{0};
    std::atomic<u32> resolvers_sleeping{0};
    std::vector<std::jthread> resolvers;

    static inline thread_local bool on_stage_b = false;
    Handler handler;
    void* context;
    std::unique_ptr<u8[]> ring;
    u64 head = 0; ///< stage A's write position
    u32 pending_size = 0;
    u64 stage_b_next_at = 0; ///< stage B's next packet offset while running current packet
    alignas(64) std::atomic<u64> published{0};
    alignas(64) std::atomic<u64> consumed{0};
    std::atomic<u64> consumed_packets{0}; ///< packets stage B has run (NextPacket numbers)
    alignas(64) std::atomic<u32> wake{0};
    std::atomic<bool> sleeping{false};
    std::atomic<u32> stage_b_tid{0};
    std::jthread thread;
};

} // namespace Vulkan
