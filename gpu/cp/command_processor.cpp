// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include <bit>
#include <chrono>
#include <filesystem>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>

#include <immintrin.h>
#include <pthread.h>
#include <sys/resource.h>
#ifndef _WIN32
#include <sys/syscall.h>
#endif
#include <csignal>
#include <dlfcn.h>
#include <sys/uio.h>
#include <unistd.h>

#include <fmt/format.h>

#include "bbport_ce_stats.h"
#include "bbport_free_check.h"
#include "bbport_threads.h"
#include "bbport_timeline.h"
#include "bbport_toggles.h"
#include "bbport_write_log.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"
#include "core/emulator_settings.h"
#include "bbport_sections.h"
#include "cp/command_processor.h"
#include "cp/cp_diagnostics.h"
#include "cp/cp_packets.h"
#include "cp/cp_tasks.h"
#include "cp/pm4_selftest.h"
#include "platform/interrupts.h"
#include "platform/videoout/videoout.h"
#include "render/vk_draw_pipe.h"
#include "render/vk_draw_prep.h"
#include "render/shader_hle.h"
#include "buffers/buffer_cache.h"
#include "buffers/direct_buffers.h"
#include "render/rasterizer.h"
#include "textures/direct_textures.h"

namespace BbCp {

std::array<u8, 48 * 1024> CommandProcessor::constant_ram;


namespace {

using Vulkan::DrawPipe;

constexpr u32 ComputeShOffset = 0x200; ///< COMPUTE_* in the SH bank
constexpr u32 ComputeWords = sizeof(ComputeProgram) / 4;

bool InComputeRange(u32 sh_offset) {
    return sh_offset >= ComputeShOffset && sh_offset <= ComputeShOffset + ComputeWords;
}

u32 BodyWords(u32 header) {
    return ((header >> 16) & 0x3FFF) + 1;
}

} // namespace

// ---- Registers -------------------------------------------------------------------------------

u64 CommandProcessor::HashRegisterPacket(u64 checksum, const u32* words, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        checksum = (checksum ^ words[i]) * 0x9E3779B97F4A7C15ull;
        checksum ^= checksum >> 29;
    }
    return checksum;
}

void CommandProcessor::ApplyGraphicsRegisterPacket(Regs& regs, const u32* packet, u64& checksum,
                                                   RegDirty* dirty) {
    const u32 header = packet[0];
    const u32 body_words = BodyWords(header);
    const u32* body = packet + 1;
    const auto mark = [&](u32 index, u32 count) {
        if (dirty) {
            dirty->Mark(index, count);
        }
    };
    const auto set = [&](u32 bank) {
        const u32 index = bank + (body[0] & 0xFFFF);
        const u32 count = body_words - 1;
        std::memcpy(&regs.reg_array[index], body + 1, count * sizeof(u32));
        mark(index, count);
    };
    const auto write = [&](u32 index, u32 value) {
        regs.reg_array[index] = value;
        mark(index, 1);
    };
    switch (Header{header}.Opcode()) {
    case Op::ClearState:
        regs.SetDefaults();
        if (dirty) {
            dirty->Clear();
            dirty->reset = true;
        }
        break;
    case Op::SetConfigReg:
        set(Bank::Config);
        break;
    case Op::SetContextReg:
        set(Bank::Context);
        break;
    case Op::SetShReg:
        // The compute program goes to the queue's compute state, not the file.
        if (InComputeRange(body[0] & 0xFFFF)) {
            return;
        }
        set(Bank::Sh);
        break;
    case Op::SetUconfigReg:
        set(Bank::Uconfig);
        break;
    case Op::IndexType:
        write(Reg::VgtDmaIndexType, body[0]);
        break;
    case Op::DrawIndex2:
        // max size, base low, base high, index count, draw initiator
        write(Reg::VgtDmaMaxSize, body[0]);
        write(Reg::VgtDmaBase, body[1]);
        write(Reg::VgtDmaBaseHi, (regs.reg_array[Reg::VgtDmaBaseHi] & ~0xFFu) | (body[2] & 0xFF));
        write(Reg::VgtNumIndices, body[3]);
        write(Reg::VgtDrawInitiator, body[4]);
        break;
    case Op::DrawIndexOffset2:
        // max size, index offset, index count, draw initiator
        write(Reg::VgtDmaMaxSize, body[0]);
        write(Reg::VgtNumIndices, body[2]);
        write(Reg::VgtDrawInitiator, body[3]);
        break;
    case Op::DrawIndexAuto:
        write(Reg::VgtNumIndices, body[0]);
        write(Reg::VgtDrawInitiator, body[1]);
        break;
    case Op::NumInstances:
        write(Reg::VgtNumInstances, body[0]);
        break;
    case Op::IndexBase:
        write(Reg::VgtDmaBase, body[0]);
        write(Reg::VgtDmaBaseHi, (regs.reg_array[Reg::VgtDmaBaseHi] & ~0xFFu) | (body[1] & 0xFF));
        break;
    case Op::IndexBufferSize:
        write(Reg::VgtNumIndices, body[0]);
        break;
    case Op::EventWrite:
        // A streamout flush: the offsets are updated at once (no streamout here).
        if ((body[0] & 0x3F) != EventWrite::SoVgtStreamoutFlush) {
            return;
        }
        write(Reg::CpStrmoutCntl, regs.reg_array[Reg::CpStrmoutCntl] | 1);
        break;
    case Op::Nop: {
        if (body_words < 2 || body[0] != NopPayload::DirectVulkan) {
            return;
        }
        const auto vk_op = DirectVk::Op(body[1] & 0xFFFFu);
        if (vk_op == DirectVk::Op::SetIndexState && body_words == 6u) {
            DirectVk::SetIndexStateArgs a{};
            std::memcpy(&a, body + 2u, sizeof(a));
            if (a.index_and_flags & 0x100u) {
                write(Reg::VgtPrimitiveType, a.prim_type & 0x3Fu);
            }
            if (a.index_and_flags & 0x200u) {
                write(Reg::VgtDmaBase, u32(a.ib_va) & ~1u);
                write(Reg::VgtDmaBaseHi,
                      (regs.reg_array[Reg::VgtDmaBaseHi] & ~0xFFu) | (u32(a.ib_va >> 32) & 0xFFu));
            }
            if (a.index_and_flags & 0x400u) {
                write(Reg::VgtDmaIndexType, a.index_and_flags & 0xFFu);
            }
            break;
        }
        if (vk_op == DirectVk::Op::DrawIndexed && body_words == 6u) {
            DirectVk::DrawIndexedArgs a{};
            std::memcpy(&a, body + 2u, sizeof(a));
            write(Reg::VgtIndxOffset, static_cast<u32>(a.base_vertex));
            write(Reg::VgtNumInstances, a.instance_count);
            write(Reg::VgtDmaMaxSize, a.index_count);
            write(Reg::VgtNumIndices, a.index_count);
            write(Reg::VgtDrawInitiator, 0u);
            break;
        }
        if (vk_op == DirectVk::Op::DrawIndexAddr && body_words == 6u) {
            DirectVk::DrawIndexAddrArgs a{};
            std::memcpy(&a, body + 2u, sizeof(a));
            if (a.prim_and_size & 0x100u) {
                write(Reg::VgtPrimitiveType, a.prim_and_size & 0x3Fu);
            }
            if (a.prim_and_size & 0x200u) {
                write(Reg::VgtDmaIndexType, (a.prim_and_size >> 16) & 0xFFu);
            }
            write(Reg::VgtDmaMaxSize, a.index_count);
            write(Reg::VgtDmaBase, u32(a.ib_va) & ~1u);
            write(Reg::VgtDmaBaseHi,
                  (regs.reg_array[Reg::VgtDmaBaseHi] & ~0xFFu) | (u32(a.ib_va >> 32) & 0xFFu));
            write(Reg::VgtNumIndices, a.index_count);
            write(Reg::VgtDrawInitiator, 0u);
            break;
        }
        if (vk_op == DirectVk::Op::DrawAuto && body_words == 5u) {
            DirectVk::DrawAutoArgs a{};
            std::memcpy(&a, body + 2u, sizeof(a));
            write(Reg::VgtIndxOffset, static_cast<u32>(a.start_vertex));
            write(Reg::VgtNumInstances, a.instance_count);
            write(Reg::VgtNumIndices, a.vertex_count);
            write(Reg::VgtDrawInitiator, 2u);
            break;
        }
        if (vk_op == DirectVk::Op::SetStageUserData && body_words >= 3u) {
            const u32 sh_offset = body[2] & 0xFFFFu;
            if (InComputeRange(sh_offset)) {
                return;
            }
            u32 mask = body[2] >> 16;
            if (static_cast<u32>(std::popcount(mask)) != body_words - 3u) {
                return;
            }
            u32 src = 3u;
            while (mask != 0u) {
                const u32 bit = static_cast<u32>(std::countr_zero(mask));
                mask &= (mask - 1u);
                write(Bank::Sh + sh_offset + bit, body[src++]);
            }
            break;
        }
        if (vk_op == DirectVk::Op::SetContextRegsPacked && body_words >= 4u) {
            const u32 num_spans = body[1] >> 16;
            u32 idx = 2u;
            for (u32 s = 0u; s < num_spans; ++s) {
                if (idx >= body_words) {
                    return;
                }
                const u32 span_hdr = body[idx++];
                const u32 ctx_off = span_hdr & 0xFFFFu;
                const u32 count = span_hdr >> 16;
                if (count == 0u || idx + count > body_words ||
                    Bank::Context + ctx_off + count > std::size(regs.reg_array)) {
                    return;
                }
                const u32 dst_reg = Bank::Context + ctx_off;
                std::memcpy(&regs.reg_array[dst_reg], &body[idx], count * sizeof(u32));
                mark(dst_reg, count);
                idx += count;
            }
            if (idx != body_words) {
                return;
            }
            break;
        }
        if (vk_op == DirectVk::Op::SetRenderTarget) {
            const u32 slot = (body[1] >> 16) & 0x7u;
            const bool valid = ((body[1] >> 19) & 1u) != 0u;
            if (valid && body_words == 14u) {
                const u32 dst_reg = Bank::Context + 0x318u + slot * 0x0Fu;
                std::memcpy(&regs.reg_array[dst_reg], body + 2u, 11u * sizeof(u32));
                mark(dst_reg, 11u);
                break;
            }
            if (!valid && body_words == 2u) {
                write(Bank::Context + 0x31Cu + slot * 0x0Fu, 0u);
                break;
            }
            return;
        }
        if (vk_op == DirectVk::Op::SetDepthRenderTarget) {
            const bool valid = ((body[1] >> 16) & 1u) != 0u;
            if (valid && body_words == 15u) {
                const u32* dt = body + 2u;
                std::memcpy(&regs.reg_array[Bank::Context + 0x010u], dt, 8u * sizeof(u32));
                mark(Bank::Context + 0x010u, 8u);
                write(Bank::Context + 0x00Fu, dt[11]);
                write(Bank::Context + 0x002u, dt[8]);
                write(Bank::Context + 0x005u, dt[9]);
                write(Bank::Context + 0x2AFu, dt[10]);
                break;
            }
            if (!valid && body_words == 2u) {
                write(Bank::Context + 0x010u, 0u);
                write(Bank::Context + 0x011u, 0u);
                break;
            }
            return;
        }
        return;
    }
    default:
        return;
    }
    checksum = HashRegisterPacket(checksum, packet, body_words + 1);
}

// ---- Thread ----------------------------------------------------------------------------------

CommandProcessor::CommandProcessor()
    : guest_markers_enabled{EmulatorSettings.IsVkGuestMarkersEnabled()} {
    thread = std::jthread{[this](std::stop_token stop) { Run(stop); }};
    if (const char* env = std::getenv("BB_STALL_REPORT"); !(env && env[0] == '0')) {
        stall_watch = std::jthread{[this](std::stop_token stop) { StallWatch(stop); }};
    }
}

CommandProcessor::~CommandProcessor() {
    if (stall_watch.joinable()) {
        stall_watch.request_stop();
        stall_watch.join();
    }
    thread.request_stop();
    {
        std::scoped_lock lock{submit_mutex};
        submit_cv.notify_all();
    }
    thread.join();
}

void CommandProcessor::SubmitDone(u64 frame) noexcept {
    std::scoped_lock lock{submit_mutex};
    if (frame != 0) {
        frame_ends.emplace_back(submissions_total, frame);
        has_pending_frame_end.store(true, std::memory_order_release);
    } else {
        ++submissions_total;
        work_retired = false;
    }
    submit_done = true;
    submit_cv.notify_one();
}

void CommandProcessor::WaitGpuIdle() noexcept {
    std::unique_lock lock{submit_mutex};
    submit_cv.wait(lock, [this] { return num_submits == 0; });
}

bool CommandProcessor::IsGpuIdle() const {
    static const bool use_retired = [] {
        const char* env = std::getenv("BB_WORK_RETIRED");
        return !(env && env[0] == '0');
    }();
    // BB_SUBMIT_LOCK=decode with honest labels: decoded is enough.
    return num_submits == 0 && (work_retired || !use_retired || BbGnm::SubmitLockOnDecode());
}

void CommandProcessor::RunCommands() {
    while (num_commands) {
        // Commands touch the caches and record into the scheduler: the draw recording thread
        // must be idle, for each one (one arriving between two checks ran beside it before).
        if (rasterizer) {
            rasterizer->DrainDrawPipe(DrawPipe::ReasonCommands);
        }
        Common::UniqueFunction<void> command;
        {
            std::scoped_lock lock{submit_mutex};
            command = std::move(commands.front());
            commands.pop_front();
            --num_commands;
        }
        command();
    }
}

void CommandProcessor::WaitForWork(std::stop_token stop) {
    const auto has_work = [this] { return num_commands || num_submits || submit_done; };
    const auto idle_start = std::chrono::steady_clock::now();
    u64 flush_ns = 0;
    // BB_HONEST_LABELS: fences (and GPU idle) wait for work not submitted yet; it goes out once
    // this thread has had nothing to do for BB_HONEST_IDLE_US (0.2 ms): the guest may be waiting.
    if (rasterizer && Vulkan::Rasterizer::HonestLabels() &&
        (rasterizer->HasUnsubmittedSignals() || !rasterizer->DrawPipeIdle())) {
        static const auto idle_wait = std::chrono::microseconds([] {
            const char* env = std::getenv("BB_HONEST_IDLE_US");
            return env ? std::strtoll(env, nullptr, 10) : 200ll;
        }());
        std::unique_lock lock{submit_mutex};
        if (!submit_cv.wait_for(lock, idle_wait, has_work)) {
            lock.unlock();
            const auto flush_start = std::chrono::steady_clock::now();
            if (rasterizer->PipeSignals()) {
                // The recording thread submits once it gets there (waiting for it to run dry,
                // tens of ms in heavy scenes, held up the guest's next submission).
                rasterizer->RequestSignalFlush();
            } else {
                rasterizer->Flush();
            }
            flush_ns = u64(std::chrono::duration_cast<std::chrono::nanoseconds>(
                               std::chrono::steady_clock::now() - flush_start)
                               .count());
        }
    }
    BbTimeline::Note(BbTimeline::DecoderIdleBegin);
    // A frame comes in ~20 submissions: waking from the condition variable for each took tens of
    // microseconds. Spin first, with cores to spare (BB_GPU_SPIN_US, default 200 with 12+
    // hardware threads, else 0).
    static const auto spin_time = std::chrono::microseconds([] {
        if (const char* env = std::getenv("BB_GPU_SPIN_US")) {
            return std::max(0, std::atoi(env));
        }
        return BbThreads::Available() >= 12 ? 200 : 0;
    }());
    if (spin_time.count() > 0) {
        const auto spin_until = std::chrono::steady_clock::now() + spin_time;
        for (u32 spins = 1; !has_work() && !stop.stop_requested(); ++spins) {
            _mm_pause();
            if (!(spins & 255) && std::chrono::steady_clock::now() >= spin_until) {
                break;
            }
        }
    }
    {
        std::unique_lock lock{submit_mutex};
        Common::CondvarWait(submit_cv, lock, stop, has_work);
    }
    const u64 waited_ns = u64(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  std::chrono::steady_clock::now() - idle_start)
                                  .count());
    BbStats::gpu_idle_ns.fetch_add(waited_ns > flush_ns ? waited_ns - flush_ns : 0,
                                   std::memory_order_relaxed);
    BbTimeline::Note(BbTimeline::DecoderIdleEnd);
}

void CommandProcessor::StallWatch(std::stop_token stop) {
    Common::SetCurrentThreadName("bb:StallWatch");
    using Clock = std::chrono::steady_clock;
    constexpr auto Limit = std::chrono::seconds(5);
    u64 last_submitted = ~0ull, last_decoded = ~0ull;
    auto submitted_at = Clock::now(), decoded_at = Clock::now();
    bool reported = false;
    while (!stop.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        u64 submitted, decoded, pending;
        {
            std::scoped_lock lock{submit_mutex};
            submitted = submissions_total;
            decoded = decoded_total;
            pending = num_submits;
        }
        const auto now = Clock::now();
        if (submitted != last_submitted) {
            last_submitted = submitted;
            submitted_at = now;
            reported = false;
        }
        if (decoded != last_decoded || pending == 0) {
            last_decoded = decoded;
            decoded_at = now;
        }
        if (reported || submitted == 0) {
            continue;
        }
        if (now - decoded_at >= Limit) {
            reported = true;
            ReportStall("the queues have made no progress for 5 s");
        } else if (now - submitted_at >= Limit) {
            reported = true;
            ReportStall("the guest has submitted nothing for 5 s");
        }
    }
}

void CommandProcessor::ReportStall(const char* why) {
    std::string out;
    {
        std::scoped_lock lock{submit_mutex};
        out = fmt::format("GPU stall report: {} (submitted {}, decoded {}, waiting {}, commands "
                          "{})\n",
                          why, submissions_total, decoded_total, num_submits.load(),
                          num_commands.load());
    }
    // What each queue is at (read beside the command thread: a snapshot, maybe torn).
    for (u32 q = 0; q < num_mapped_queues && q < NumTotalQueues; ++q) {
        Queue& queue = queues[q];
        std::scoped_lock lock{queue.mutex};
        if (q != GfxQueueId && asc_queues.is_allocated(Common::SlotId{q - 1})) {
            const auto& ring = asc_queues[Common::SlotId{q - 1}];
            u32 read = 0;
            const bool readable = Diagnostics::ReadGuest(ring.read_addr, &read, sizeof(read));
            out += fmt::format("  compute queue {} (pipe {}): ring {:#x}, {} dwords, read pointer "
                               "{}{}, {} submissions queued\n",
                               q, ring.pipe_id, ring.map_addr, ring.ring_size_dw,
                               readable ? fmt::format("{}", read) : std::string("?"),
                               ring.tmp_dwords ? fmt::format(" ({} dwords of a split packet)",
                                                             ring.tmp_dwords)
                                               : std::string{},
                               queue.works.size());
        } else if (!queue.works.empty()) {
            out += fmt::format("  queue {}: {} submissions queued\n", q, queue.works.size());
        }
        if (queue.works.empty()) {
            continue;
        }
        const Work& work = *queue.works.front();
        if (!work.started || work.walk.top < 0) {
            out += "    first one not started\n";
            continue;
        }
        const Level& level = work.walk.levels[work.walk.top];
        u32 header = 0;
        const bool readable =
            level.at < level.end && Diagnostics::ReadGuest(level.at, &header, sizeof(header));
        out += fmt::format("    first one at depth {}: {:#x}, dword {} of {}, packet {}{}\n",
                           work.walk.top, reinterpret_cast<uintptr_t>(level.at),
                           level.at - level.begin, level.end - level.begin,
                           readable && (header >> 30) == 3
                               ? Diagnostics::OpName(u32(Header{header}.Opcode()))
                               : fmt::format("{:#x}", header),
                           work.waiting_at ? " (waiting)" : "");
    }
    if (rasterizer) {
        out += rasterizer->DescribeSignals();
    }
    out += "  last labels written for the guest (oldest first):\n";
    out += Diagnostics::RecentLabels(40);
    // The command thread's frames (the runtime's SIGUSR2 handler prints its frame-pointer chain
    // to stderr; addresses minus this library's base give the source lines).
    Dl_info self{};
    if (dladdr(reinterpret_cast<void*>(&BodyWords), &self)) {
        out += fmt::format("  command thread {} stack follows ({} at {:#x}):\n", thread_tid,
                           self.dli_fname ? self.dli_fname : "?",
                           reinterpret_cast<uintptr_t>(self.dli_fbase));
    }
    std::fputs(out.c_str(), stdout);
    std::fflush(stdout);
#ifndef _WIN32
    if (thread_tid != 0) {
        syscall(SYS_tgkill, getpid(), thread_tid, SIGUSR2);
    }
#endif
#ifndef _WIN32
    // Every thread of the process prints its frames and registers (the runtime's SIGUSR2 handler):
    // when the guest itself stops (its job system, a lock), they say what each one waits for.
    // BB_STALL_THREADS=0: the command thread only.
    static const bool all_threads = [] {
        const char* env = std::getenv("BB_STALL_THREADS");
        return !(env && env[0] == '0');
    }();
    if (all_threads && !std::getenv("BB_STALL_THREADS_DONE")) {
        setenv("BB_STALL_THREADS_DONE", "1", 1); // once per run
        std::fputs("  all threads follow (frames: image-relative, then registers and stack):\n", stdout);
        std::fflush(stdout);
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator("/proc/self/task", ec)) {
            const long tid = std::strtol(entry.path().filename().c_str(), nullptr, 10);
            if (tid > 0 && tid != long(thread_tid)) {
                syscall(SYS_tgkill, getpid(), tid, SIGUSR2);
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
        }
    }
#endif
}

void CommandProcessor::Run(std::stop_token stop) {
    Common::SetCurrentThreadName("bb:GpuComm");
    BbSections::command_thread = true;
    BbThreads::PinToGpuDomain();
    if (clockid_t clock; pthread_getcpuclockid(pthread_self(), &clock) == 0) {
        BbStats::gpu_thread_clock.store(int(clock));
    }
    thread_id = std::this_thread::get_id();
    thread_tid = u32(gettid());

    while (!stop.stop_requested()) {
        WaitForWork(stop);
        if (stop.stop_requested()) {
            break;
        }
        // The queues in turn: each one's front submission runs until it is done or waits.
        current_queue = -1;
        while (num_submits || num_commands) {
            RunCommands();
            current_queue = int((current_queue + 1) % num_mapped_queues);
            Queue& queue = queues[current_queue];
            Work* work;
            {
                std::scoped_lock lock{queue.mutex};
                if (queue.works.empty()) {
                    continue;
                }
                work = queue.works.front().get();
            }
            if (work != last_decoded) {
                if (has_pending_frame_end.load(std::memory_order_acquire)) {
                    SignalDecodedFrames();
                }
                last_decoded = work;
                BbTimeline::Note(BbTimeline::DecodeBegin, u64(current_queue), decoded_total);
                if (rasterizer && (VideoCore::DirectTextures::HasPendingEvents() ||
                                   VideoCore::DirectBuffers::HasPendingEvents())) {
                    if (rasterizer->PipeSignals() ||
                        (rasterizer->PipeOrder() &&
                         !BbToggle::Disabled(BbToggle::PipelinedTasks))) {
                        static constexpr u8 none = 0;
                        rasterizer->RunInOrder(
                            [](Vulkan::Rasterizer& r, const u8*) {
                                r.ProcessDirectTextureEvents();
                            },
                            &none, 0, BbToggle::PipelinedTasks, false);
                    } else {
                        rasterizer->DrainDrawPipe();
                        rasterizer->ProcessDirectTextureEvents();
                    }
                }
            }
            Status status;
            {
                BB_SECTION(ADecode);
                status = current_queue == GfxQueueId ? RunGraphics(*work) : RunCompute(*work);
            }
            if (status == Status::Wait) {
                _mm_pause(); // the other queues' turn
                continue;
            }
            BbTimeline::Note(BbTimeline::DecodeEnd, u64(current_queue), decoded_total + 1);
            bool frame_end;
            {
                std::scoped_lock lock{queue.mutex};
                queue.works.pop_front();
                last_decoded = nullptr;
                --num_submits;
                std::scoped_lock submit_lock{submit_mutex};
                ++decoded_total;
                frame_end = !frame_ends.empty() && frame_ends.front().first <= decoded_total;
                submit_cv.notify_all();
            }
            if (frame_end) {
                SignalDecodedFrames(); // BB_SUBMIT_LOCK=frame: a frame's last submission decoded
            }
        }
        AfterBatch();
    }
}

void CommandProcessor::AfterBatch() {
    SignalDecodedFrames();
    const bool pipe_signals = rasterizer && rasterizer->PipeSignals();
    const bool pipe_order =
        rasterizer && rasterizer->PipeOrder() &&
        !BbToggle::Disabled(BbToggle::PipelinedTasks);
    const bool was_submit_done = submit_done.exchange(false, std::memory_order_acq_rel);
    if (was_submit_done) {
        if (pipe_signals || pipe_order) {
            // BB_PIPE_SIGNALS: the frame's cache work runs on the recording thread after the
            // frame (this thread decodes the next one meanwhile).
            static constexpr u8 none = 0;
            rasterizer->RunInOrder(
                [](Vulkan::Rasterizer& r, const u8*) {
                    r.OnSubmit();
                    if (r.HasPendingWork()) {
                        r.Flush();
                    }
                    // BB_EXP_FINISH_FRAMES=1 (experiment): the GPU finishes each frame before the next.
                    static const bool finish = std::getenv("BB_EXP_FINISH_FRAMES") != nullptr;
                    if (finish) {
                        r.Finish();
                    }
                },
                &none, 0, BbToggle::PipelinedTasks, false);
        } else if (rasterizer) {
            rasterizer->OnSubmit();
            if (rasterizer->HasPendingWork()) {
                rasterizer->Flush();
            }
        }
    }

    // The guest takes GPU idle (sceGnmSubmitDone) as its work being done and frees the objects
    // that hold its fence labels: they must be written before, by the draw recording thread and
    // the fences it deferred. (BB_PIPE_SIGNALS: that thread does both in order, below.)
    if (rasterizer && !pipe_signals && !pipe_order) {
        rasterizer->DrainDrawPipe(DrawPipe::ReasonSubmissionEnd);
        rasterizer->WaitDeferredSignals();
    }
    // GPU idle only if nothing was submitted meanwhile: the drain and the deferred fences take
    // milliseconds, and a submission plus sceGnmSubmitDone in that time would release the lock for
    // work not even decoded (late fences in the guest's heap, command buffers reused as garbage).
    if (rasterizer && Vulkan::Rasterizer::HonestLabels()) {
        u64 generation;
        bool decoded;
        {
            std::scoped_lock lock{submit_mutex};
            generation = submissions_total;
            decoded = num_submits == 0;
        }
        // BB_SUBMIT_LOCK=decode: everything is decoded; the next submission need not wait for
        // the GPU (the idle interrupt still does).
        if (BbGnm::SubmitLockOnDecode() && decoded) {
            BbGnm::ReleaseSubmissionLock();
        }
        if (pipe_signals) {
            struct IdleSignal {
                CommandProcessor* self;
                u64 generation;
            } const signal{this, generation};
            rasterizer->RunInOrder(
                [](Vulkan::Rasterizer& r, const u8* data) {
                    IdleSignal idle;
                    std::memcpy(&idle, data, sizeof(idle));
                    r.WaitDeferredSignals();
                    r.SignalAfterGpu([idle] { idle.self->SignalGpuIdle(idle.generation); });
                    r.SubmitForSignals(); // at once if the GPU has run dry
                    r.WaitPendingFrame();
                },
                &signal, sizeof(signal), BbToggle::PipelinedTasks, false);
            return;
        }
        rasterizer->SignalAfterGpu([this, generation] { SignalGpuIdle(generation); });
        rasterizer->WaitPendingFrame();
        return;
    }
    if (pipe_order) {
        u64 generation;
        {
            std::scoped_lock lock{submit_mutex};
            generation = submissions_total;
        }
        struct IdleSignal {
            CommandProcessor* self;
            u64 generation;
            bool wait_frame;
        } const signal{this, generation, was_submit_done};
        rasterizer->RunInOrder(
            [](Vulkan::Rasterizer& r, const u8* data) {
                IdleSignal idle;
                std::memcpy(&idle, data, sizeof(idle));
                r.WaitDeferredSignals();
                idle.self->SignalGpuIdle(idle.generation);
                if (idle.wait_frame) {
                    r.WaitPendingFrame();
                }
            },
            &signal, sizeof(signal), BbToggle::PipelinedTasks, false);
        return;
    }
    bool idle;
    {
        std::scoped_lock lock{submit_mutex};
        idle = num_submits == 0;
        if (idle) {
            work_retired = true;
        }
    }
    if (idle) {
        BbIrq::Raise(BbIrq::Line::GpuIdle);
    }
    if (rasterizer && was_submit_done) {
        rasterizer->WaitPendingFrame();
    }
}

void CommandProcessor::SignalDecodedFrames() {
    u64 frame = 0;
    {
        std::scoped_lock lock{submit_mutex};
        while (!frame_ends.empty() && frame_ends.front().first <= decoded_total) {
            frame = frame_ends.front().second;
            frame_ends.pop_front();
        }
        if (frame_ends.empty()) {
            has_pending_frame_end.store(false, std::memory_order_release);
        }
    }
    if (frame == 0) {
        return;
    }
    const bool do_on_submit = submit_done.exchange(false, std::memory_order_acq_rel);
    if (!rasterizer) {
        BbGnm::NoteFramesRetired(frame);
        return;
    }
    struct FrameSignal {
        u64 retired;
        bool on_submit;
    } const sig{frame, do_on_submit};
    // Everything of the frame recorded, then submitted: finished once the GPU has executed it
    // (HonestLabels), or once Stage B has recorded/flushed all commands and EoP labels of frame F
    // in the 0.3 VRAM model (where all transient guest data is copied out into ConstantRing /
    // stream_buffer / native VkBuffers before Flush, while WaitPendingFrame bounds GPU to 2-FIF).
    if (rasterizer->PipeSignals()) {
        rasterizer->RunInOrder(
            [](Vulkan::Rasterizer& r, const u8* data) {
                FrameSignal s;
                std::memcpy(&s, data, sizeof(s));
                if (s.on_submit) {
                    r.OnSubmit();
                }
                BbTimeline::Note(BbTimeline::PipeTask, 7, r.CurrentTick());
                r.SignalAfterGpu([retired = s.retired] { BbGnm::NoteFramesRetired(retired); });
                r.Flush();
            },
            &sig, sizeof(sig), BbToggle::PipelinedTasks, false);
        return;
    }
    if (!Vulkan::Rasterizer::HonestLabels()) {
        if (rasterizer->PipeOrder() && !BbToggle::Disabled(BbToggle::PipelinedTasks)) {
            rasterizer->RunInOrder(
                [](Vulkan::Rasterizer& r, const u8* data) {
                    FrameSignal s;
                    std::memcpy(&s, data, sizeof(s));
                    if (s.on_submit) {
                        r.OnSubmit();
                    }
                    if (r.HasPendingWork()) {
                        r.Flush();
                    }
                    BbTimeline::Note(BbTimeline::PipeTask, 7, r.CurrentTick());
                    r.WaitDeferredSignals();
                    BbGnm::NoteFramesRetired(s.retired);
                    if (s.on_submit) {
                        r.WaitPendingFrame();
                    }
                },
                &sig, sizeof(sig), BbToggle::PipelinedTasks, false);
            return;
        }
        rasterizer->DrainDrawPipe(DrawPipe::ReasonSubmissionEnd);
        if (do_on_submit) {
            rasterizer->OnSubmit();
            if (rasterizer->HasPendingWork()) {
                rasterizer->Flush();
            }
        }
        rasterizer->WaitDeferredSignals();
        BbGnm::NoteFramesRetired(frame);
        if (do_on_submit) {
            rasterizer->WaitPendingFrame();
        }
        return;
    }
    rasterizer->DrainDrawPipe(DrawPipe::ReasonSubmissionEnd);
    if (do_on_submit) {
        rasterizer->OnSubmit();
    }
    rasterizer->SignalAfterGpu([frame] { BbGnm::NoteFramesRetired(frame); });
    rasterizer->Flush();
}

void CommandProcessor::SignalGpuIdle(u64 generation) {
    bool idle;
    {
        std::scoped_lock lock{submit_mutex};
        idle = num_submits == 0 && submissions_total == generation;
        if (idle) {
            work_retired = true;
        }
    }
    if (idle) {
        BbIrq::Raise(BbIrq::Line::GpuIdle);
    }
}

// ---- Submissions -----------------------------------------------------------------------------

void CommandProcessor::SubmitGfx(std::span<const u32> dcb, std::span<const u32> ccb, bool force_copy) {
    Queue& queue = queues[GfxQueueId];
    auto work = std::make_unique<Work>();
    work->queue = GfxQueueId;
    // BB_COPY_GPU_BUFFERS: one copy per submission, alive until it is decoded (the guest reused
    // the memory before this thread got to it: "PM4 type 0" while an area loads).
    if (force_copy || EmulatorSettings.IsCopyGpuBuffers()) {
        auto copy = std::make_shared<SubmittedCopy>();
        copy->data.resize(dcb.size() + ccb.size());
        std::memcpy(copy->data.data(), dcb.data(), dcb.size_bytes());
        std::memcpy(copy->data.data() + dcb.size(), ccb.data(), ccb.size_bytes());
        copy->guest_dcb = force_copy ? copy->data.data() : dcb.data();
        copy->dcb_dwords = dcb.size();
        copy->guest_ccb = force_copy ? copy->data.data() + dcb.size() : ccb.data();
        copy->ccb_dwords = ccb.size();
        copy->submitted = std::chrono::steady_clock::now();
        Diagnostics::InspectSubmittedCopy(*copy);
        Diagnostics::RememberSubmittedCopy(copy);
        dcb = {copy->data.data(), copy->dcb_dwords};
        ccb = {copy->data.data() + copy->dcb_dwords, copy->ccb_dwords};
        work->copy = std::move(copy);
    }
    work->dcb = dcb;
    work->ccb = ccb;
    // The draw preparation workers' copy is made before the queue lock, which this thread needs.
    auto* draw_prep = rasterizer ? &rasterizer->GetDrawPreparation() : nullptr;
    auto prepared = draw_prep ? draw_prep->Build(dcb) : nullptr;
    {
        // Numbering, enqueueing and queueing under one lock keep the three orders the same.
        std::scoped_lock lock{queue.mutex};
        if (draw_prep) {
            work->seq = gfx_submit_seq++;
            draw_prep->Enqueue(work->seq, std::move(prepared));
        }
        BbFreeCheck::NoteSubmit(work->seq, work->copy ? work->copy->guest_dcb : dcb.data(),
                                dcb.size_bytes());
        queue.works.push_back(std::move(work));
    }
    std::scoped_lock lock{submit_mutex};
    ++num_submits;
    ++submissions_total;
    BbTimeline::Note(BbTimeline::GuestSubmit, 0, submissions_total);
    work_retired = false;
    submit_cv.notify_one();
}

void CommandProcessor::SubmitAsc(u32 gnm_vqid, std::span<const u32> acb) {
    ASSERT_MSG(gnm_vqid > 0 && gnm_vqid < NumTotalQueues, "bad compute queue {}", gnm_vqid);
    BbStats::asc_submits.fetch_add(1, std::memory_order_relaxed);
    Queue& queue = queues[gnm_vqid];
    auto work = std::make_unique<Work>();
    work->queue = gnm_vqid;
    work->dcb = acb;
    {
        std::scoped_lock lock{queue.mutex};
        queue.works.push_back(std::move(work));
    }
    std::scoped_lock lock{submit_mutex};
    num_mapped_queues = std::max(num_mapped_queues, gnm_vqid + 1);
    ++num_submits;
    ++submissions_total;
    work_retired = false;
    submit_cv.notify_one();
}

} // namespace BbCp
