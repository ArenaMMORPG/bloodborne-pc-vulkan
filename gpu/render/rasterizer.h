// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: the Rasterizer turns the command processor's draws, dispatches and DMA into
// Vulkan work: pipeline selection, resource binding, render passes and dynamic state, recorded on
// the GPU command thread or on the draw recording thread (the two-stage draw pipeline). It also
// writes the fences and labels the guest observes, in GPU order. Its parts are in
// render/raster_*.cpp (raster_internal.h).
#pragma once

#include <string>

namespace BbCp {
class CommandProcessor;
}

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <boost/icl/interval_set.hpp>
#include <thread>
#include "common/recursive_lock.h"
#include "common/shared_first_mutex.h"
#include "buffers/buffer_cache.h"
#include "buffers/direct_buffers.h"
#include "memory/page_watch.h"
#include "render/vk_bind_helper.h"
#include "upscale/vk_camera_motion.h"
#include "render/vk_constant_ring.h"
#include "upscale/vk_scene_resolution.h"
#include "upscale/vk_object_motion.h"
#include "render/vk_draw_pipe.h"
#include "render/vk_draw_prep.h"
#include "upscale/vk_temporal_upscaler.h"
#include "render/pipeline_cache.h"
#include "render/scheduler.h"
#include "textures/texture_cache.h"


namespace Core {
class MemoryManager;
}

namespace Vulkan {

class GraphicsPipeline;
class Runtime;

class Rasterizer {
public:
    explicit Rasterizer(const Instance& instance, Scheduler& scheduler, Runtime& runtime,
                        BbCp::CommandProcessor* liverpool);
    ~Rasterizer();

    [[nodiscard]] Runtime& GetRuntime() noexcept {
        return runtime;
    }

    [[nodiscard]] VideoCore::BufferCache& GetBufferCache() noexcept {
        return buffer_cache;
    }

    [[nodiscard]] VideoCore::DirectBuffers::Manager& GetDirectBuffers() noexcept {
        return direct_buffers;
    }

    [[nodiscard]] TemporalUpscaler& GetUpscaler() noexcept {
        return *upscaler;
    }

    [[nodiscard]] VideoCore::TextureCache& GetTextureCache() noexcept {
        return texture_cache;
    }

    void Draw(bool is_indexed, u32 index_offset = 0, const PreparedDraw* prepared = nullptr);

    /// bbport: draw preparation workers (vk_draw_prep.h), fed and consumed by Liverpool.
    DrawPreparation& GetDrawPreparation() {
        return *draw_prep;
    }
    void DrawIndirect(bool is_indexed, VAddr arg_address, u32 offset, u32 size, u32 max_count,
                      VAddr count_address, u16 vertex_sgpr_offset, u16 instance_sgpr_offset);

    void DispatchDirect();
    void DispatchIndirect(VAddr address, u32 offset, u32 size);

    [[nodiscard]] bool HostMarkersEnabled() const noexcept {
        return host_markers_enabled;
    }

    void ScopeMarker(fmt::string_view fmt, fmt::format_args args, auto&& func) {
        if (host_markers_enabled) {
            ScopeMarkerBegin(fmt::vformat(fmt, args));
            func();
            ScopeMarkerEnd();
        } else {
            func();
        }
    }

    void ScopeMarkerBegin(const std::string_view& str, bool from_guest = false);
    void ScopeMarkerEnd(bool from_guest = false);
    void ScopedMarkerInsert(const std::string_view& str, bool from_guest = false);
    void ScopedMarkerInsertColor(const std::string_view& str, const u32 color,
                                 bool from_guest = false);

    void FillBuffer(VAddr address, u32 num_bytes, u32 value, bool is_gds);
    void CopyBuffer(VAddr dst, VAddr src, u32 num_bytes, bool dst_gds, bool src_gds);
    struct DirectCopyRegion {
        VAddr dst_va;
        VAddr src_va;
        u32 byte_size;
        u32 reserved;
    };
    struct DirectClearColorArgs {
        VAddr dst_va;
        u32 byte_size;
        u32 num_bits;
        u32 clear_rgba[4];
        u64 tsharp[4];
    };
    struct DirectCopyTextureRegionArgs {
        u64 srv_tsharp[4];
        u64 uav_tsharp[4];
        VAddr dst_va;
        VAddr src_va;
        u32 dst_byte_size;
        u32 src_byte_size;
        u32 src_x;
        u32 src_y;
        u32 src_z;
        u32 dst_x;
        u32 dst_y;
        u32 dst_z;
        u32 copy_w;
        u32 copy_h;
        u32 copy_d;
        u32 reserved;
    };
    struct DirectDecompressDepthArgs {
        VAddr dst_va;
        u32 dst_byte_size;
        u32 db_view;
        u32 db_regs[8];
        u32 htile_data_base;
        u32 htile_surface;
        u32 cb_db_extent;
        u32 first_slice;
        u32 slice_count;
        u32 reserved;
    };
    struct DirectEliminateFastClearArgs {
        VAddr dst_va;
        u32 dst_byte_size;
        u32 first_slice;
        u32 slice_count;
        u32 cb_regs[11];
    };
    struct DirectResolveColorArgs {
        VAddr dst_va;
        VAddr src_va;
        u32 dst_byte_size;
        u32 src_byte_size;
        u32 src_cb_regs[11];
        u32 dst_cb_regs[11];
    };
    struct DirectUploadTextureArgs {
        u64 real_tsharp[4];
        VAddr src_stage_va;
        VAddr dst_va;
        u32 src_byte_size;
        u32 dst_byte_size;
        u32 offset_x;
        u32 offset_y;
        u32 offset_z;
        u32 extent_w;
        u32 extent_h;
        u32 extent_d;
        u32 buf_row_len;
        u32 buf_img_height;
        u32 reserved0;
        u32 reserved1;
    };
    struct DirectClearDepthStencilArgs {
        VAddr dst_va;
        u32 dst_byte_size;
        u32 db_view;
        u32 db_regs[8];
        u32 htile_data_base;
        u32 cb_db_extent;
        u32 first_slice;
        u32 slice_count;
        float depth_clear;
        u32 stencil_and_flags;
    };
    void DirectCopyBuffers(std::span<const DirectCopyRegion> regions);
    void DirectFillBuffer(VAddr dst, u32 num_bytes, u32 value);
    void DirectClearColorImage(const DirectClearColorArgs& args);
    void DirectCopyTextureRegion(const DirectCopyTextureRegionArgs& args);
    void DirectDecompressDepth(const DirectDecompressDepthArgs& args);
    void DirectEliminateFastClear(const DirectEliminateFastClearArgs& args);
    void DirectResolveColor(const DirectResolveColorArgs& args);
    void DirectUploadTextureSubresource(const DirectUploadTextureArgs& args);
    void DirectClearDepthStencil(const DirectClearDepthStencilArgs& args);
    /// bbport: whether FillBuffer/CopyBuffer into guest memory at `dst` may write it on the CPU
    /// now (otherwise the GPU writes it, in stream order).
    [[nodiscard]] bool DmaMayWriteOnCpu(VAddr dst, u32 num_bytes);
    u32 ReadDataFromGds(u32 gsd_offset);
    bool InvalidateMemory(VAddr addr, u64 size, bool assume_locks = false);
    /// bbport: data written into GPU memory by a path we hear of (file reads, the game's resource
    /// loaders, DMA on the CPU): invalidated, and an asset for the buffer cache.
    void NoteAssetWrite(VAddr addr, u64 size);
    /// bbport: the CPU has written [addr, addr + size): invalidated, its bytes kept over GPU data that
    /// is read back first.
    void InvalidateAfterWrite(VAddr addr, u64 size);
    /// bbport (any thread): the command processor wrote these bytes outside the command stream
    /// (labels, timestamps, occlusion results): VRAM copies of them get them too.
    void NoteLateCommandWrite(VAddr addr, const void* data, u64 size) {
        buffer_cache.NoteLateCommandWrite(addr, data, size);
    }
    /// bbport: the command processor wrote `size` bytes of `data` at addr with the CPU in decode order
    /// (WriteData, a constant RAM dump; the decoder reads them there). Where the GPU's own data there
    /// is in VRAM the bytes go into it in stream order, else the caches are invalidated (no write
    /// tracking). `recording_thread`: the draw recording thread, which owns the caches (no drain).
    void NoteCommandWrite(VAddr addr, const void* data, u64 size, bool recording_thread);
    /// bbport (stage A, BB_CONSTRAM_PIPE, default on): before the command processor writes guest
    /// memory with the CPU (a constant RAM dump), waits until the recording thread has run the
    /// queued packets that read those bytes (their buffer bindings, DMA), not all of them.
    void WaitForPendingReads(VAddr address, u64 size);
    /// V2.2: marks guest pages written by ConstantEngine DUMP_CONST_RAM for synchronous streaming.
    void NoteConstRamWrite(VAddr addr, u64 size) noexcept {
        buffer_cache.NoteConstRamWrite(addr, size);
    }
    /// Stage A, the game's memory in place: waits until the GPU has run `packet` (drawn reading in
    /// place what a CPU write is about to replace), when it has not yet.
    void WaitForInPlaceReads(u64 packet);
    /// NoteCommandWrite for a write made on stage A: handed to the recording thread in order (a copy
    /// of the bytes) instead of draining it first.
    void NoteCommandWriteInOrder(VAddr addr, const void* data, u64 size);
    /// bbport: a guest thread wrote [addr, addr + size) with a libc import (memcpy, memset, memmove)
    /// over pages the caches watch (no write tracking): what a write fault did.
    void OnCpuWrite(VAddr addr, u64 size);
    /// GPU thread, before a write the guest can observe (see Scheduler::WaitHostCopies).
    void WaitHostCopies() {
        DrainDrawPipe();
        scheduler.WaitHostCopies();
    }
    /// Before a write into [address, address + size): waits only when a host copy may still read
    /// it (Scheduler::WaitHostCopiesFor). BB_HOST_COPY_WAITS=all: always (the old behaviour).
    void WaitHostCopiesFor(VAddr address, u64 size) {
        DrainDrawPipe();
        static const bool all = [] {
            const char* env = std::getenv("BB_HOST_COPY_WAITS");
            return env && std::string_view{env} == "all";
        }();
        if (all) {
            scheduler.WaitHostCopies();
        } else {
            scheduler.WaitHostCopiesFor(address, size);
        }
    }
    /// Notes the guest memory a host copy issued now reads (see Scheduler::NoteHostCopySource).
    void NoteHostCopySource(VAddr address, u64 size) {
        scheduler.NoteHostCopySource(address, size);
    }

    /// bbport: GPU command thread: waits until the draw recording thread has recorded every
    /// draw handed to it (vk_draw_pipe.h); no-op on other threads.
    /// `line`/`function`: the caller, for the statistics of where stage A waits.
    void DrainDrawPipe(u32 reason = DrawPipe::ReasonRasterizer, u32 line = __builtin_LINE(),
                       const char* function = __builtin_FUNCTION());
    /// Runs `task(rasterizer, copy of data)` in order with the draws: on the draw recording
    /// thread while the draw pipeline is in use (PipelinedTasks), else here after a drain.
    using OrderedTask = void (*)(Rasterizer& rasterizer, const u8* data);
    /// Returns true when the task was handed to the recording thread (not run yet).
    /// `reads_guest_memory`: the task may read guest memory (a CPU write on stage A over memory
    /// waits for it, WaitForPendingReads); false for fences, flips and signals.
    bool RunInOrder(OrderedTask task, const void* data, u32 size,
                    u64 toggle = BbToggle::PipelinedTasks, bool reads_guest_memory = true);
    /// Stage A: the draw pipe position after the last handed-over packet, and whether the
    /// recording thread has run everything before a position.
    [[nodiscard]] u64 DrawPipeHead() const {
        return draw_pipe ? draw_pipe->Head() : 0;
    }
    [[nodiscard]] bool DrawPipeReached(u64 position) const {
        return !draw_pipe || draw_pipe->Reached(position);
    }
    /// Stage A: no draws waiting for the recording thread.
    [[nodiscard]] bool DrawPipeIdle() const {
        return !draw_pipe || draw_pipe->Idle();
    }
    /// Stage A, end of a submission: its prepared draws stay alive until the recording thread
    /// has recorded them (instead of a drain).
    void RetireSubmission();
    /// Stage A: guest memory the recording thread will write for work handed to it (storage
    /// buffers, DMA, WriteData, fences); constants overlapping it are bound there, not copied here.
    void NotePendingGpuWrite(VAddr address, u64 size);
    /// GPU command thread: a copy into guest memory by the command processor (DMA_DATA), kept
    /// until the GPU has run it for the program checks (SyncProgramWrites).
    void NoteCopyIntoMemory(VAddr address, u64 size);
    /// Stage A: guest memory the recording thread will read for the next packet (buffer bindings).
    /// `gpu_in_place`: true if the GPU may read guest memory in place after Stage B records it.
    void NotePendingRead(VAddr address, u64 size, bool gpu_in_place = true);
    /// Before a guest-visible write: fences deferred earlier are written first.
    void WaitDeferredSignals() {
        scheduler.WaitDeferredSignals();
    }
    /// Runs `signal` after the guest memory copies issued so far, without waiting here.
    void SignalAfterHostCopies(std::function<void()> signal, bool force_kick = true) {
        scheduler.SignalAfterHostCopies(std::move(signal), force_kick);
    }
    /// bbport BB_HONEST_LABELS=1: runs `signal` once the GPU has finished every command recorded
    /// before this call (the submission of the current tick), in call order, on a thread that
    /// waits on the work semaphore: fences the guest sees when the work is done, not when it is
    /// recorded. `label` and `value`: the fence it writes (PendingSignalValue).
    void SignalAfterGpu(std::function<void()> signal, VAddr label = 0, u64 value = 0);
    /// Whether signals wait for work not submitted yet (the GPU command thread submits it when
    /// it runs out of work, or the guest could wait for them forever).
    bool HasUnsubmittedSignals();
    /// For a stall report: the signals waiting for the GPU, the ticks and the draw pipe.
    std::string DescribeSignals();
    /// bbport BB_GUEST_IN_PLACE (draw recording thread): `data` written into the game's memory by the
    /// GPU in stream order, like the command processor's writes; false when the range is not
    /// bound in place (the caller writes it with the CPU).
    bool WriteGuestMemory(VAddr address, const void* data, u32 size);
    /// bbport BB_GUEST_IN_PLACE (in stream order: the recording thread): a WRITE_DATA performed by
    /// the GPU, as the command processor does, instead of a CPU store now (ahead of the GPU work
    /// recorded before it). Values of up to 8 bytes are registered for WAIT_REG_MEM until the GPU
    /// has written them. False: not in place (or BB_GPU_COMMAND_WRITES=0), the caller stores it.
    bool WriteDataOnGpu(VAddr address, const void* data, u32 size);
    /// bbport BB_GUEST_IN_PLACE (in stream order: the recording thread): an end-of-pipe label
    /// (EVENT_WRITE_EOP/EOS) written by the GPU itself once the work recorded before it is done,
    /// as the command processor does: a buffer marker into the game's memory, where the CPU sees
    /// it. Registered for WAIT_REG_MEM like the CPU-written ones. False (the caller signals it
    /// from the CPU): not in GPU-visible guest memory, no VK_AMD_buffer_marker, BB_GPU_LABELS=0.
    bool WriteLabelOnGpu(VAddr address, u64 value, u32 num_bytes);
    bool WriteZeroLabelsBatchOnGpu(std::span<const VAddr> addresses);
    bool WriteU32LabelsBatchOnGpu(std::span<const VAddr> addresses, u32 value);
    /// bbport: a ZPASS_DONE event (the game's occlusion query) at `address` for `pairs` depth
    /// blocks, in stream order (vk_occlusion.h). False when it is left to the caller (the old
    /// memory model).
    bool OcclusionEvent(VAddr address, u32 pairs);
    void DirectOcclusionQuery(VAddr base_addr, u32 pairs, u32 query_op);
    /// Whether ZPASS_DONE events go to OcclusionEvent (the new memory model).
    static bool OcclusionTranslated();
    /// ZPASS_DONE events translated so far.
    u64 OcclusionEvents() const;
    /// bbport BB_GUEST_IN_PLACE (in stream order): the GPU clock (PS4: 100 MHz) written as 64 bits
    /// at `address` by the GPU, at the end of the pipe or when it gets there (vk_timestamps.h).
    /// False (the caller writes it with the CPU): BB_GPU_TIMESTAMPS=0, no timestamps, the old model.
    bool WriteTimestampOnGpu(VAddr address, bool end_of_pipe);
    /// A fault on a page of occlusion counters watched for the game's first access
    /// (BB_OCCLUSION_READ_TRACE); false: not such a page.
    bool OnOcclusionPageAccess(VAddr addr, u64 rip, bool write, bool gpu_thread);
    /// A CPU access hit a page whose newest data the GPU wrote into a VRAM copy only
    /// (BB_LAYER_READ_TRAPS): copied back first. False: not such a page.
    bool OnVramDataAccess(VAddr addr, bool gpu_thread);
    /// BB_LAYER_PROBE (diagnostics): a CPU access to a probed block (BufferCache::LayerProbeHit).
    bool OnProbeAccess(VAddr addr, u64 rip, bool write, bool gpu_thread) {
        return buffer_cache.LayerProbeHit(addr, rip, write, gpu_thread);
    }
    /// End of a guest submission: submits the work recorded so far when signals wait for it and
    /// the last submission is BB_HONEST_FLUSH_US (1000) old: the GPU starts on it as the hardware
    /// would, instead of at the end of the frame, and the guest's mid-frame waits end sooner.
    void SubmitForSignals();
    /// bbport BB_PIPE_SIGNALS (default on with honest labels; 0: off): on the GPU command thread with
    /// the draw pipe, fences, the GPU idle and frame signals and the submissions for them are
    /// handed to the draw recording thread as ordered tasks instead of that thread being drained
    /// first (the GPU command thread waited for it ~15 times a frame and the GPU ran dry meanwhile).
    bool PipeSignals() const;
    bool PipeOrder() const {
        return UseDrawPipe() && OnStageA();
    }
    /// Stage A: the fences the recording thread ran up to `position` are in the signal queue (or
    /// written): a fence it ran but holds still (PublishSignals) is not visible there yet.
    bool SignalsPublished(u64 position) const;
    /// Stage A (BB_PIPE_SIGNALS): the recording thread hands over its signals and submits the work
    /// they wait for, in order, when it gets there (one request in flight at a time).
    void RequestSignalFlush();
    u64 CurrentTick() const {
        return scheduler.CurrentTick();
    }
    /// The newest queued value of the fence at `address` (a WaitRegMem in the same stream is met
    /// by it in stream order).
    bool PendingSignalValue(VAddr address, u64& value);
    static bool HonestLabels();
    /// The GPU command thread or the draw recording thread (fault handling runs inline there).
    bool IsGpuSideThread() const;
    bool IsGpuSideThreadId(u32 tid) const;
    /// A guest write hit a protected page.
    /// `guest_rip`: the guest instruction that wrote (0: unknown), see BufferCache::NoteCpuWrite.
    bool OnWriteFault(VAddr addr, bool assume_locks, u64 guest_rip = 0);
    bool ReadMemory(VAddr addr, u64 size, bool assume_locks = false);
    void ProcessDownloadImages();
    void ProcessDirectTextureEvents();
    bool IsMapped(VAddr addr, u64 size);
    void MapMemory(VAddr addr, u64 size);
    void RegisterMemory(VAddr addr, u64 size);
    void UnmapMemory(VAddr addr, u64 size);

    u64 Flush();
    bool HasPendingWork();
    void Finish();
    void WaitPendingFrame() {
        scheduler.WaitPendingFrame();
    }
    /// BB_PASS_DUMP (diagnostics): rows of chosen render targets at each display pass.
    void DumpPassImages(char tag);
    /// BB_TRACE_GPU_INPUTS (diagnostics): the traced draw's buffer ranges, dumped at the display pass.
    void DumpTracedInputs();
    struct TracedRange {
        vk::Buffer buffer;
        vk::DeviceSize offset, size;
    };
    std::vector<TracedRange> traced_inputs;
    u64 traced_inputs_frame = 0;
    /// BB_EXP_FLIP_LAG (recording thread, at a flip): waits for the GPU to finish the frame before.
    void WaitPreviousFrame();
    void OnSubmit();

    PipelineCache& GetPipelineCache() {
        return pipeline_cache;
    }

    template <typename Func>
    void ForEachMappedRangeInRange(VAddr addr, u64 size, Func&& func) {
        const auto range = decltype(mapped_ranges)::interval_type::right_open(addr, addr + size);
        Common::RecursiveSharedLock lock{mapped_ranges_mutex};
        for (const auto& mapped_range : (mapped_ranges & range)) {
            func(mapped_range);
        }
    }

    std::thread::id GetGpuCommandProcessorThread();
#ifdef __linux__
    u32 GetGpuCommandProcessorThreadId();
#endif

private:
    void PrepareRenderState(const GraphicsPipeline* pipeline);
    RenderState BeginRendering(const GraphicsPipeline* pipeline);
    RenderState BeginRenderingFull(const GraphicsPipeline* pipeline);
    /// The upscaler's redirection of the pass's targets to its output-size images.
    struct TargetRedirect {
        TemporalUpscaler::Target target{};
        bool color = false, depth = false;
        std::pair<u32, u32> guest_extent{1, 1};
        std::pair<vk::ImageView, vk::ImageLayout> original_color{};
    };
    /// Whether the pass draws into the reduced scene proxies (BB_SCENE_RESOLUTION); `pass_size`:
    /// the proxies' common size.
    bool ReducedPass(const GraphicsPipeline* pipeline,
                     std::optional<SceneResolution::Size>& pass_size);
    void BindColorAttachments(RenderState& state, const GraphicsPipelineKey& key, bool reduced,
                              TargetRedirect& redirect);
    void BindDepthAttachment(RenderState& state, bool reduced, TargetRedirect& redirect);
    void ApplyTargetRedirect(RenderState& state, const TargetRedirect& redirect);
    /// bbport: a draw continuing the open render pass with the same inputs gets the same render
    /// state (RenderStateMemo); nothing can have broken the pass in between (barriers, copies and
    /// dispatches end it).
    struct BeginSignature {
        std::array<VideoCore::ImageId, Gcn::NUM_COLOR_BUFFERS + 1> ids{};
        std::array<VideoCore::ImageViewInfo, Gcn::NUM_COLOR_BUFFERS + 1> views{};
        u32 mrt_mask = 0;
        u32 num_samples = 0;
        std::array<u8, Gcn::NUM_COLOR_BUFFERS> color_samples{};
        bool motion = false;
        bool scene_started = false;
        bool raster_scaling = false;
        u64 upscaler_state = 0;
        u64 generation = 0;
        u32 depth_control = 0;
        bool depth_valid = false;
        bool stencil_valid = false;
        bool stencil_write = false;
        bool native_viewport = false; ///< the UI composition's placement depends on it
        bool operator==(const BeginSignature&) const = default;
    };
    BeginSignature MakeBeginSignature(const GraphicsPipeline* pipeline) const;
    /// The game's UI in an output of another aspect ratio: where its 16:9 screen ends
    /// (UiComposition::ScreenEdges), 0: not such a pass.
    struct UiPlacement {
        float edge_x = 0.0f, edge_y = 0.0f;
    };
    struct BeginMemo {
        bool valid = false;
        BeginSignature signature;
        RenderState state;
        u32 scene_size = 0;
        std::array<float, 2> target_scale{};
        UiPlacement ui_placement;
    } begin_memo;
    u64 begin_memo_hits = 0, begin_memo_misses = 0;
    void Resolve();
    void DepthStencilCopy(bool is_depth, bool is_stencil);
    void EliminateFastClear();

    void UpdateDynamicState(const GraphicsPipeline* pipeline, bool is_indexed) const;
    void UpdateViewportScissorState() const;
    void UpdateDepthStencilState() const;
    void UpdatePrimitiveState(bool is_indexed) const;
    void UpdateRasterizationState() const;
    void UpdateColorBlendingState(const GraphicsPipeline* pipeline) const;

    bool FilterDraw();
    bool FilterDrawPasses() const;
    /// Parts of DrawRecord and DrawIndirectRecord: the upscaler's view of the draw, the push
    /// data scaled to the target drawn into, and the pipeline bind, dynamic state and render
    /// pass (`jitter_eligible`: not a full-screen pass).
    void NoteUpscalerDraw(const GraphicsPipeline* pipeline);
    void ScaleToTarget(const GraphicsPipeline* pipeline);
    u8 ComputeFullscreenOverwriteMask(const GraphicsPipeline* pipeline, const RenderState& state,
                                      const PreparedDraw* draw_prepared, bool is_indexed,
                                      u32 index_offset);
    void BeginDraw(const GraphicsPipeline* pipeline, const RenderState& state, bool is_indexed,
                   bool jitter_eligible, const PreparedDraw* draw_prepared = nullptr,
                   u32 index_offset = 0, bool allow_dont_care = false);
    /// Everything of a direct draw after the pipeline selection (GPU thread or stage B).
    void DrawRecord(const GraphicsPipeline* pipeline, const PreparedDraw* used_prepared,
                    bool is_indexed, u32 index_offset);
    static bool DrawPipeWanted();
    bool UseDrawPipe() const;
    bool OnStageA() const;
    /// Arguments of an indirect draw: `args` holds `max_count` commands `stride` apart.
    struct IndirectDraw {
        VAddr args;
        VAddr count; ///< the draw count, or 0 for max_count draws
        u32 stride;
        u32 max_count;
    };
    /// Everything of an indirect draw after the pipeline selection (GPU thread or stage B).
    void DrawIndirectRecord(const GraphicsPipeline* pipeline, bool is_indexed,
                            const IndirectDraw& indirect);
    /// Everything of an indirect dispatch after the pipeline selection.
    void DispatchIndirectRecord(const ComputePipeline* pipeline, VAddr args, u32 size);
    /// Hands a draw (or, with `cs`, a dispatch) to the recording thread.
    void PostDraw(const Pipeline* pipeline, const PreparedDraw* used_prepared, bool is_indexed,
                  u32 index_offset, const Gcn::ComputeProgram* cs = nullptr,
                  const IndirectDraw* indirect = nullptr);
    /// Everything of a direct dispatch after the pipeline selection (GPU thread or stage B).
    void DispatchRecord(const ComputePipeline* pipeline);
    /// The compute registers of the dispatch being recorded.
    const Gcn::ComputeProgram& CsRegs() const;
    static void RunDrawPacket(void* rasterizer, const u8* packet, u32 size);
    struct RingBinding;
    void CollectRingBindings(const Shader::Info& stage, const PreparedDraw* prepared,
                             boost::container::static_vector<RingBinding, Shader::NUM_BUFFERS>& out,
                             const Gcn::ComputeProgram* cs = nullptr);
    void PrintPipeStats();
    /// The registers of the draw being recorded: stage B's copy there, else Liverpool's.
    const Gcn::Regs& Regs() const;
    Gcn::CbDbExtent CbExtent(u32 index) const;
    Gcn::CbDbExtent DbExtent() const;
    u16 CbUid(u32 index) const;
    u16 DbUid() const;

    void BindBuffers(const Shader::Info& stage, const PreparedStage* prepared,
                     Shader::Backend::Bindings& binding,
                     Shader::PushData& push_data, u32& write_index);
    /// A buffer the translator adds (GDS, flattened user data, clip planes, page table, fault
    /// buffer, LDS) / a guest V#, at buffer slot `slot`: into buffer_infos.
    void BindSpecialBuffer(const Shader::BufferResource& desc, const Shader::Info& stage, u32 slot,
                           u64 alignment);
    void BindGuestBuffer(const Shader::BufferResource& desc, const Gcn::Buffer& vsharp,
                         const Shader::Info& stage, u32 slot, Shader::PushData& push_data,
                         u64 alignment);
    /// `on_helper`: run by the texture binding helper; out-of-date images are not refreshed.
    void BindTextures(const Shader::Info& stage, const PreparedStage* prepared,
                      Shader::Backend::Bindings& binding, u32& write_index, bool& barrier,
                      bool on_helper = false);
    bool BindResources(const Pipeline* pipeline);
    /// The layout transition of an image a shader reads (or writes, `is_storage`); true when
    /// it records a barrier.
    bool TransitSampled(VideoCore::Image& image, const VideoCore::TextureCache::ImageDesc& desc,
                        const VideoCore::Image::Binding& binding, bool is_storage);
    void ForceAnisotropy(Gcn::Sampler& sharp, bool is_depth) const;
    void BindSamplers(const Shader::Info& stage, const PreparedStage* prepared,
                      Shader::Backend::Bindings& binding, u32& write_index);
    /// bbport: a stage's resolved textures remembered by its prepared T# hashes
    /// (TextureSetMemo): a hit only redoes the per-draw effects (binding flags, transitions).
    struct TextureSetEntry {
        VideoCore::ImageId id{}; ///< after the depth redirect; null descriptor when invalid
        bool proxy = false; ///< sampled from the scene proxy (view: SampleProxy on each use)
        vk::ImageView view;
        const void* backing = nullptr;
        u64 image_uid = 0;
        VideoCore::SubresourceRange range;
    };
    struct TextureSet {
        u64 key = 0;
        const Shader::Info* stage = nullptr;
        u64 generation = ~0ull;
        u64 scene_generation = 0; ///< SceneTargets::Generation of the proxy views
        u64 last_use = 0;
        u32 count = 0;
        u8 native_count = 0;
        bool all_direct_immutable = false;
        static constexpr u32 MaxImages = 24; ///< larger sets fall back to ImageDescCacheEntry
        std::array<u64, MaxImages> hashes{};
        std::array<TextureSetEntry, MaxImages> entries{};
    };
    /// V3.2: 2-way set-associative (8192 sets x 2 ways = 16384 slots, ~22 MB, fits in L3 cache).
    std::array<TextureSet, 16384> texture_sets{};
    u64 texture_set_use_counter = 0;
    u64 texture_set_hits = 0, texture_set_misses = 0;
    std::array<u64, 4> texture_set_why{}; ///< misses: other key, generation, image check, new
    /// Returns true when the stage's images were bound from the memo.
    bool BindTexturesFromSet(const Shader::Info& stage, const PreparedStage* prepared,
                             u32 first_image_idx, bool& barrier, TextureSet*& slot);
    /// The prepared sharps of `stage` in the current prepared draw, if they fit it.
    const PreparedStage* FindPreparedStage(const Shader::Info& stage) const;
    /// Whether the helper may bind textures for this pipeline at all (fixed descriptor layout,
    /// no storage writes that invalidate cached images).
    bool HelperEligible(const Pipeline* pipeline) const;
    /// Whether every texture of `stage` takes the memoized path without refreshing, creating or
    /// redirecting anything that records commands or touches the buffer cache.
    bool TexturesBindableOnHelper(const Shader::Info& stage, const PreparedStage* prepared);
    static void RunTextureTask(void* rasterizer);
    static void JoinBindHelper(void* rasterizer);

    /// Display pass: counts the frame (BbStats::gpu_frames) and, with BB_BUFFER_STATS, the lag.
    void NoteFrameStart();
    /// BB_GPU_PROFILE: a timestamp where a render pass starts (vk_gpu_profiler.h).
    void MarkPass(const GraphicsPipeline* pipeline, const RenderState& state);
    void BindVertexBuffers(const GraphicsPipeline* pipeline,
                           const PreparedDraw* prepared = nullptr);
    void BindIndexBuffer(u32 index_offset = 0);
    /// bbport: BindVertexBuffers/BindIndexBuffer in two halves: obtaining the buffers
    /// (vertex_binds, index_bind) and recording the binds.
    void ResolveVertexBuffers(const GraphicsPipeline* pipeline, const PreparedDraw* prepared);
    void EmitVertexBuffers();
    void ResolveIndexBuffer(u32 index_offset);
    void EmitIndexBuffer();

    void ResetBindings(bool is_compute);
    /// bbport BB_DRAW_WATCH=1 (diagnostics): a draw whose viewport, targets or textures differ from
    /// what the same shaders and target had before, for one frame.
    void WatchDraw(const GraphicsPipeline* pipeline, const RenderState& state);

    bool IsComputeMetaClear(const Pipeline* pipeline);
    bool IsComputeImageCopy(const Pipeline* pipeline, bool dry_run = false);
    bool IsComputeImageClear(const Pipeline* pipeline);

private:
    friend class VideoCore::BufferCache;

    const Instance& instance;
    Scheduler& scheduler;
    Runtime& runtime;
    BbMem::PageWatch page_watch;
    VideoCore::BufferCache buffer_cache;
    VideoCore::TextureCache texture_cache;
    BbCp::CommandProcessor* liverpool;
    Core::MemoryManager* memory;
    boost::icl::interval_set<VAddr> mapped_ranges;
    Common::SharedFirstMutex mapped_ranges_mutex;
    PipelineCache pipeline_cache;
    std::unique_ptr<DrawPreparation> draw_prep;
    std::unique_ptr<CameraMotion> camera_motion; // bbport: motion vectors (docs/upscaler.md)
    std::unique_ptr<SceneTargets> scene_targets;
    bool scene_started = false;
    std::unique_ptr<ObjectMotion> object_motion;
    bool motion_draw = false;
    u64 motion_geometry{};    ///< vertex-stream identity of the current direct draw
    bool gbuffer_draw = false;
    std::unique_ptr<TemporalUpscaler> upscaler; // bbport: FSR (docs/upscaler.md)
    std::array<float, 2> draw_jitter{};         ///< viewport offset of the current draw, pixels
    std::array<float, 2> target_scale{1.0f, 1.0f}; ///< pass drawn into the upscaler's output-size images
    UiPlacement ui_placement;
    /// A solid fill of the UI on the edges of the game's screen reaches the output's edges.
    void UiEdgeStretch(const GraphicsPipeline* pipeline);
    const bool host_markers_enabled;
    const bool guest_markers_enabled;

    using RenderTargetInfo = std::pair<VideoCore::ImageId, VideoCore::TextureCache::ImageDesc>;
    std::array<RenderTargetInfo, Gcn::NUM_COLOR_BUFFERS> cb_descs;
    std::pair<VideoCore::ImageId, VideoCore::TextureCache::ImageDesc> db_desc;
    boost::container::static_vector<vk::DescriptorImageInfo, Shader::NUM_IMAGES> image_infos;
    boost::container::static_vector<vk::DescriptorBufferInfo, Shader::NUM_BUFFERS> buffer_infos;
    boost::container::static_vector<VideoCore::ImageId, Shader::NUM_IMAGES> bound_images;
    struct BoundBuffer {
        const VideoCore::Buffer* buffer;
        u64 offset;
        u64 size;
        bool is_written;
    };
    boost::container::small_vector<BoundBuffer, Shader::NUM_BUFFERS> bound_buffers;

    u32 set_write_index{};
    Pipeline::DescriptorWrites set_writes;
    Shader::PushData push_data;

    // bbport: bindings point at their description instead of copying it (a hot spot): into
    // image_desc_cache for memoized lookups (pinned for the current BindTextures call), else
    // into image_desc_storage.
    using ImageBindingInfo = std::pair<VideoCore::ImageId, const VideoCore::TextureCache::ImageDesc*>;
    // bbport: texture descriptions depend only on the T# and three resource flags; building
    // them (mip layout sizes) for every binding of every draw was a hot spot.
    struct ImageDescCacheEntry {
        std::array<u64, 4> sharp{};
        u32 flags = ~0u;
        u8 srv_gen8 = 0;
        bool found_direct_immutable = false;
        VideoCore::TextureCache::ImageDesc desc;
        // Memoized FindImage for bindings without mip overrides (TextureBindingMemo).
        u64 found_generation = ~0ULL;
        u64 found_uid = 0;
        VideoCore::ImageId found_id{};
        VideoCore::TextureCache::ImageDesc found_desc;
        u64 pinned = 0; ///< bind_epoch of the BindTextures call referencing found_desc
        u64 last_use = 0;
        // Memoized FindView for found_id (after the depth redirect): valid while the entry's
        // FindImage memo holds and the image keeps this backing (TextureViewMemo).
        VideoCore::TextureCache::ViewMemo view_memo;
    };
    std::array<ImageDescCacheEntry, 4096> image_desc_cache{};
    /// N2.1: Direct O(1) 65,536-slot Stage-B SRV/UAV cache indexed by T#.dw[7] uid16 (0 hash evictions).
    std::unique_ptr<ImageDescCacheEntry[]> native_srv_cache;
    /// N2.1: Direct O(1) 4,096-slot Stage-B VkSampler cache indexed by S#.dw[3] uid12 (0 mutex/XXH3).
    struct NativeSamplerStageBEntry {
        Gcn::Sampler clean_sharp{};
        u32 bias_bits = 0;
        Gcn::BorderColorBuffer bc_base{};
        bool forced_aniso_off = false;
        std::array<u64, 3> touched_ticks{~0ULL, ~0ULL, ~0ULL}; // [0]=gfx_color, [1]=cs_color, [2]=depth
        std::array<vk::Sampler, 3> handles{};
    };
    std::unique_ptr<NativeSamplerStageBEntry[]> native_sampler_cache;
    u64 desc_use_counter = 0;
    /// Entries replacing pinned cache slots during one BindTextures call.
    boost::container::static_vector<ImageDescCacheEntry, Shader::NUM_IMAGES> image_desc_overflow;
    boost::container::static_vector<VideoCore::TextureCache::ImageDesc, Shader::NUM_IMAGES * 2>
        image_desc_storage;
    u64 bind_epoch = 0;
    // bbport: render/depth target lookups memoized by their raw register bytes while image
    // registrations are unchanged (the descriptions depend only on those registers).
    struct TargetMemo {
        std::array<u8, 256> key{};
        u32 key_size = 0;
        u64 generation = ~0ULL;
        VideoCore::ImageId image_id{};
        VideoCore::TextureCache::ImageDesc desc;
        VideoCore::TextureCache::ViewMemo color_view_memo{};
        std::array<VideoCore::TextureCache::ViewMemo, 2> depth_view_memo{};
    };
    std::array<TargetMemo, 64> target_memo{};
    // bbport: consecutive draws mostly keep their targets; the slot's description (cb_descs,
    // db_desc) is then still the right one and is neither looked up nor copied.
    struct LastTarget {
        std::array<u8, 256> key{};
        u32 key_size = 0;
        u32 memo_slot = ~0u;
        bool changed = true;
        u64 generation = ~0ULL;
        VideoCore::ImageId image_id{};
    };
    std::array<LastTarget, Gcn::NUM_COLOR_BUFFERS + 1> last_targets{}; ///< CBs, then DB
    template <typename... Parts>
    VideoCore::ImageId FindTargetMemoized(VideoCore::TextureCache::ImageDesc& desc,
                                          LastTarget& last, auto&& make_desc,
                                          const Parts&... parts);

    // V3.1: Direct RenderTarget & DepthTarget O(1) Stage-B cache + per-pass fast signature.
    struct DirectTargetStageBCache {
        u32 slot_epoch = 0;
        u64 registry_generation = ~0ULL;
        bool parent_ensured = false;
        VideoCore::ImageId image_id{};
        VideoCore::TextureCache::ImageDesc desc{};
        VideoCore::TextureCache::ViewMemo color_view_memo{};
        std::array<VideoCore::TextureCache::ViewMemo, 2> depth_view_memo{};
    };
    std::array<DirectTargetStageBCache, 4096> direct_target_cache{};
    std::array<u16, Gcn::NUM_COLOR_BUFFERS> active_cb_uid{};
    u16 active_db_uid = 0;
    std::array<VideoCore::TextureCache::ViewMemo, Gcn::NUM_COLOR_BUFFERS> fallback_cb_view_memo{};
    std::array<VideoCore::TextureCache::ViewMemo, 2> fallback_db_view_memo{};

    struct PreparedPassMemo {
        bool valid = false;
        bool skip_cb = false;
        bool has_depth = false;
        u8 mrt_mask = 0;
        u8 active_cb_mask = 0;
        u64 generation = ~0ULL;
        std::array<Gcn::ColorBuffer, Gcn::NUM_COLOR_BUFFERS> cb_buf{};
        std::array<Gcn::CbDbExtent, Gcn::NUM_COLOR_BUFFERS> cb_extent{};
        Gcn::DepthBuffer depth_buf{};
        u32 depth_view_raw = 0;
        VAddr htile_addr = 0;
        Gcn::CbDbExtent db_extent{};
    };
    PreparedPassMemo prepared_pass{};

    VideoCore::ImageId ResolveDirectColorTarget(u16 uid, const Gcn::ColorBuffer& col_buf,
                                                Gcn::CbDbExtent hint,
                                                VideoCore::TextureCache::ImageDesc& out_desc);
    VideoCore::ImageId ResolveDirectDepthTarget(u16 uid, const Gcn::DepthBuffer& depth_buf,
                                                Gcn::DepthView depth_view,
                                                Gcn::DepthControl depth_ctrl,
                                                VAddr htile_address, Gcn::CbDbExtent hint,
                                                VideoCore::TextureCache::ImageDesc& out_desc);
    /// The prepared draw whose pipeline the current Draw uses (its sharps are valid), or null.
    const PreparedDraw* bind_prepared = nullptr;
    /// `hash` is ImageDescHash(sharp, res), computed by a draw-preparation worker or here.
    ImageDescCacheEntry& CachedImageDescEntry(const Gcn::Image& sharp,
                                              const Shader::ImageResource& res, u64 hash);
    ImageDescCacheEntry& CachedImageDescEntry(const Gcn::Image& sharp,
                                              const Shader::ImageResource& res) {
        return CachedImageDescEntry(sharp, res, ImageDescHash(sharp, res));
    }
    ImageDescCacheEntry& ResolveImageDescEntry(const Gcn::Image& clean_sharp, u16 srv_uid,
                                               u8 srv_gen8, const Shader::ImageResource& res,
                                               const u64* prepared_hash);
    const VideoCore::TextureCache::ImageDesc& CachedImageDesc(const Gcn::Image& sharp,
                                                              const Shader::ImageResource& res) {
        return CachedImageDescEntry(sharp, res).desc;
    }
    boost::container::static_vector<ImageBindingInfo, Shader::NUM_IMAGES> image_bindings;
    struct VertexBinds {
        VertexInputs<vk::VertexInputAttributeDescription2EXT> attributes;
        VertexInputs<vk::VertexInputBindingDescription2EXT> bindings;
        VertexInputs<vk::Buffer> host_buffers;
        VertexInputs<vk::DeviceSize> host_offsets;
        VertexInputs<vk::DeviceSize> host_sizes;
        VertexInputs<vk::DeviceSize> host_strides;
        u32 num_buffers = 0;
    } vertex_binds;
    struct IndexBind {
        vk::Buffer handle;
        u64 offset;
        vk::IndexType type;
    } index_bind{};
    /// The direct draw whose vertex/index buffers BindResources may resolve early.
    struct DrawInputs {
        const GraphicsPipeline* pipeline;
        const PreparedDraw* prepared;
        u32 index_offset;
        bool is_indexed;
        bool pending;
        bool resolved;
    } draw_inputs{};
    // bbport: textures of a draw bound on the helper thread while this thread binds buffers.
    static bool BindHelperWanted();
    BindHelper bind_helper{BindHelperWanted()};
    struct TextureTask {
        struct Stage {
            const Shader::Info* info;
            const PreparedStage* prepared;
            Shader::Backend::Bindings binding;
            u32 write_index;
        };
        std::array<Stage, Shader::MaxStageTypes> stages;
        u32 count = 0;
        u32 completed = 0; ///< stages the helper bound; the rest are bound after the join
        bool barrier = false;
    } texture_task;
    /// Draws whose textures the helper bound completely, partly or not at all (BB_FRAME_STATS).
    u64 helper_full = 0, helper_partial = 0, helper_serial = 0;
    /// The memoized cache entry of each image binding (FindImage memo taken), else null.
    boost::container::static_vector<ImageDescCacheEntry*, Shader::NUM_IMAGES> image_binding_entries;
    bool fault_process_pending{};
    bool attachment_feedback_loop{};
    bool needs_barrier{};
    // bbport: two-stage draw pipeline (stage B state; the thread is the last member so it
    // stops first).
    static inline thread_local const Gcn::Regs* stage_regs = nullptr;
    static inline thread_local const Gcn::ComputeProgram* stage_cs = nullptr;
    Gcn::Regs pipe_regs{};
    bool pipe_synced = false;
    std::array<Gcn::CbDbExtent, Gcn::NUM_COLOR_BUFFERS> pipe_cb_extent{};
    Gcn::CbDbExtent pipe_db_extent{};
    std::array<u16, Gcn::NUM_COLOR_BUFFERS> pipe_cb_uid{};
    u16 pipe_db_uid = 0;
    /// Stage A: small read-only guest buffers copied into the constant ring for a packet.
    struct RingBinding {
        u32 index;  ///< buffer resource of the stage
        u32 size;
        u64 offset; ///< in the ring; RingPending / RingNotCopied with resolve workers
        VAddr address;
        u32 ring;   ///< 0: stage A's constant_ring, 1 + n: resolve worker n's
        u32 always_note; ///< resolve workers: note the read even when copied (HLE compute)
    };
    static constexpr u64 RingPending = ~0ull;    ///< stage A left the copy to a resolve worker
    static constexpr u64 RingNotCopied = ~1ull;  ///< the worker left it to stage B (ObtainBuffer)
    /// Stage 2 (resolve workers): checks and copies the constants stage A left pending in a packet.
    static void ResolvePacket(void* context, u8* packet, u32 size, u64 number, u32 worker);
    std::vector<std::unique_ptr<ConstantRing>> worker_rings; ///< one per resolve worker
    [[nodiscard]] vk::Buffer RingHandle(u32 ring) const {
        return ring == 0 ? constant_ring->Handle() : worker_rings[ring - 1]->Handle();
    }
    /// Stage A: the newest draw pipe packet that reads guest memory, hashed per granule with a
    /// 24-bit granule tag + active-collision bit so unrelated vertex/index buffers never cause
    /// false-positive ConstRam GPU waits while preserving 0% false negatives.
    struct PendingReads {
        static constexpr u64 Shifts[4] = {8, 12, 16, 19};
        static constexpr u64 Limits[3] = {4_KB, 32_KB, 256_KB};
        static constexpr u32 Bits = 16;
        static constexpr u64 kPktBits = 39;
        static constexpr u64 kPktMask = (u64{1} << kPktBits) - 1;
        static constexpr u64 kTagMask = 0xFFFFFFull;
        static constexpr u64 kCollidedBit = u64{1} << 63;

        // Atomic: resolve workers note reads and look up writes while stage A notes them.
        std::array<std::vector<std::atomic<u64>>, 4> tables{
            std::vector<std::atomic<u64>>(1u << Bits), std::vector<std::atomic<u64>>(1u << Bits),
            std::vector<std::atomic<u64>>(1u << Bits), std::vector<std::atomic<u64>>(1u << Bits)};
        static u64 Slot(u64 granule, u32 level) {
            return ((granule * 4 + level) * 0x9E3779B97F4A7C15ull) >> (64 - Bits);
        }
        static u64 Tag(u64 granule) {
            return ((granule * 0xBF58476D1CE4E5B9ull) >> 40) & kTagMask;
        }
        static u64 MergeEntry(u64 seen, u64 tag, u64 pkt, u64 retired) {
            pkt &= kPktMask;
            const u64 cur_pkt = seen & kPktMask;
            if (cur_pkt <= retired) {
                return (tag << kPktBits) | pkt;
            }
            const u64 max_pkt = std::max(cur_pkt, pkt);
            if ((seen & kCollidedBit) == 0 && ((seen >> kPktBits) & kTagMask) == tag) {
                return (tag << kPktBits) | max_pkt;
            }
            return kCollidedBit | max_pkt;
        }
        /// Exact interval cache for ranges past Limits[2] (> 256 KiB); evicted entries spill into
        /// level 3 (512 KiB granules) only if still unretired. Written by stage A only.
        struct Big {
            std::atomic<VAddr> begin{0}, end{0};
            std::atomic<u64> packet{0};
        };
        std::array<Big, 128> big{};
        u32 big_next = 0;
        std::atomic<u64> total_max{0};
        std::atomic<u64> big_max{0};
        std::array<std::atomic<u64>, 4> level_max{};
        std::atomic<VAddr> active_min_addr{~VAddr{0}};
        std::atomic<VAddr> active_max_addr{0};
        static void BumpMax(std::atomic<u64>& target, u64 packet, bool single_writer) {
            u64 seen = target.load(std::memory_order_relaxed);
            if (seen >= packet) {
                return;
            }
            if (single_writer) {
                target.store(packet, std::memory_order_relaxed);
                return;
            }
            while (seen < packet &&
                   !target.compare_exchange_weak(seen, packet, std::memory_order_relaxed)) {
            }
        }
        void UpdateAddrBounds(VAddr address, u64 size, u64 prev_max, u64 retired,
                              bool single_writer) {
            const VAddr end = address + size;
            if (single_writer && prev_max <= retired) {
                active_min_addr.store(address, std::memory_order_relaxed);
                active_max_addr.store(end, std::memory_order_relaxed);
                return;
            }
            VAddr cur_min = active_min_addr.load(std::memory_order_relaxed);
            if (address < cur_min) {
                if (single_writer) {
                    active_min_addr.store(address, std::memory_order_relaxed);
                } else {
                    while (address < cur_min && !active_min_addr.compare_exchange_weak(
                                                    cur_min, address, std::memory_order_relaxed)) {
                    }
                }
            }
            VAddr cur_max = active_max_addr.load(std::memory_order_relaxed);
            if (end > cur_max) {
                if (single_writer) {
                    active_max_addr.store(end, std::memory_order_relaxed);
                } else {
                    while (end > cur_max && !active_max_addr.compare_exchange_weak(
                                                cur_max, end, std::memory_order_relaxed)) {
                    }
                }
            }
        }
        void Note(VAddr address, u64 size, u64 packet, bool single_writer = false,
                  u64 retired = 0) {
            if (size == 0 || packet <= retired) {
                return;
            }
            const u64 prev_max = total_max.load(std::memory_order_relaxed);
            UpdateAddrBounds(address, size, prev_max, retired, single_writer);
            BumpMax(total_max, packet, single_writer);
            if (size > Limits[2]) {
                BumpMax(big_max, packet, single_writer);
                u32 free_idx = ~0u;
                for (u32 i = 0; i < big.size(); ++i) {
                    auto& b = big[i];
                    const VAddr b_end = b.end.load(std::memory_order_relaxed);
                    if (b_end == 0) {
                        if (free_idx == ~0u) {
                            free_idx = i;
                        }
                        continue;
                    }
                    const u64 b_pkt = b.packet.load(std::memory_order_relaxed);
                    if (b.begin.load(std::memory_order_relaxed) == address &&
                        b_end == address + size) {
                        if (b_pkt < packet) {
                            b.packet.store(packet, std::memory_order_relaxed);
                        }
                        return;
                    }
                    if (free_idx == ~0u && b_pkt <= retired) {
                        free_idx = i;
                    }
                }
                auto& out = free_idx != ~0u ? big[free_idx] : big[big_next++ % big.size()];
                if (const VAddr end = out.end.load(std::memory_order_relaxed); end != 0) {
                    const u64 old_pkt = out.packet.load(std::memory_order_relaxed);
                    if (old_pkt > retired) {
                        const VAddr begin = out.begin.load(std::memory_order_relaxed);
                        NoteTables(begin, end - begin, old_pkt, single_writer, retired);
                    }
                    out.end.store(0, std::memory_order_release);
                }
                out.packet.store(packet, std::memory_order_relaxed);
                out.begin.store(address, std::memory_order_relaxed);
                out.end.store(address + size, std::memory_order_release);
                return;
            }
            NoteTables(address, size, packet, single_writer, retired);
        }
        void NoteTables(VAddr address, u64 size, u64 packet, bool single_writer = false,
                        u64 retired = 0) {
            if (size == 0 || packet <= retired) {
                return;
            }
            const u64 prev_max = total_max.load(std::memory_order_relaxed);
            UpdateAddrBounds(address, size, prev_max, retired, single_writer);
            BumpMax(total_max, packet, single_writer);
            const u32 level = size <= Limits[0] ? 0 : size <= Limits[1] ? 1 : size <= Limits[2] ? 2 : 3;
            BumpMax(level_max[level], packet, single_writer);
            auto& table = tables[level];
            if (single_writer) {
                for (u64 g = address >> Shifts[level]; g <= (address + size - 1) >> Shifts[level];
                     ++g) {
                    auto& slot = table[Slot(g, level)];
                    const u64 seen = slot.load(std::memory_order_relaxed);
                    const u64 desired = MergeEntry(seen, Tag(g), packet, retired);
                    if (desired != seen) {
                        slot.store(desired, std::memory_order_relaxed);
                    }
                }
                return;
            }
            for (u64 g = address >> Shifts[level]; g <= (address + size - 1) >> Shifts[level];
                 ++g) {
                const u64 tag = Tag(g);
                auto& slot = table[Slot(g, level)];
                u64 seen = slot.load(std::memory_order_relaxed);
                for (;;) {
                    const u64 desired = MergeEntry(seen, tag, packet, retired);
                    if (desired == seen ||
                        slot.compare_exchange_weak(seen, desired, std::memory_order_relaxed)) {
                        break;
                    }
                }
            }
        }
        u64 Newest(VAddr address, u64 size, u64 min_needed = 1) const {
            if (size == 0 || total_max.load(std::memory_order_relaxed) < min_needed) {
                return 0;
            }
            if (address + size <= active_min_addr.load(std::memory_order_relaxed) ||
                address >= active_max_addr.load(std::memory_order_relaxed)) {
                return 0;
            }
            u64 newest = 0;
            if (big_max.load(std::memory_order_relaxed) >= min_needed) {
                for (const auto& b : big) {
                    const u64 bpkt = b.packet.load(std::memory_order_relaxed);
                    if (bpkt < min_needed || bpkt <= newest) {
                        continue;
                    }
                    const VAddr end = b.end.load(std::memory_order_acquire);
                    if (end > address && b.begin.load(std::memory_order_relaxed) < address + size) {
                        newest = bpkt;
                    }
                }
            }
            for (u32 level = 0; level < 4; ++level) {
                if (level_max[level].load(std::memory_order_relaxed) <
                    std::max(min_needed, newest + 1)) {
                    continue;
                }
                for (u64 g = address >> Shifts[level]; g <= (address + size - 1) >> Shifts[level];
                     ++g) {
                    const u64 entry = tables[level][Slot(g, level)].load(std::memory_order_relaxed);
                    const u64 pkt = entry & kPktMask;
                    if (pkt < min_needed || pkt <= newest) {
                        continue;
                    }
                    if ((entry & kCollidedBit) == 0 &&
                        ((entry >> kPktBits) & kTagMask) != Tag(g)) {
                        continue;
                    }
                    newest = pkt;
                }
            }
            return newest;
        }
    };
    PendingReads pending_reads;
    PendingReads pending_gpu_reads;
    std::atomic<u64> gpu_retired_packet{0};
    u64 gpu_retired_min_tick = 1;
    u64 AdvanceGpuRetiredPacket();
    /// Stage A: packets whose constants a resolve worker copies, by the bytes they read (a
    /// constant RAM dump over them waits for the workers first: WaitForPendingReads).
    PendingReads pending_resolve;

    /// V2.4: Stage-A-exclusive L1D write-combining cache (256 slots = 8 KiB) coalescing repeated
    /// (address, size, mask) buffer read notes across draws before touching the 6 MiB atomic hash
    /// tables. Flushed automatically before every WaitForPendingReads / DrainDrawPipe / RetireSubmission.
    static constexpr u8 kCoalesceResolve = 1u << 0;
    static constexpr u8 kCoalesceStageB  = 1u << 1;
    static constexpr u8 kCoalesceGpuInPl = 1u << 2;
    struct CoalescedReadEntry {
        VAddr addr = 0;
        u64 size = 0;
        u64 packet = 0;
        u8 mask = 0;
    };
    static constexpr size_t kStageACoalescerSlots = 256;
    std::array<CoalescedReadEntry, kStageACoalescerSlots> stage_a_read_coalescer{};
    u64 stage_a_cached_gpu_retired = 0;
    u64 stage_a_last_gpu_retired_pkt = 0;
    bool stage_a_coalescer_dirty = false;
    void NoteStageACoalescedRead(VAddr address, u64 size, u64 packet, u8 mask);
    void FlushStageAReadCoalescer();
    /// Stage B: the scheduler tick each recent packet was recorded in (packet number & mask).
    struct PacketTick {
        std::atomic<u64> packet{0}, tick{0};
    };
    std::unique_ptr<std::array<PacketTick, 1u << 16>> packet_ticks =
        std::make_unique<std::array<PacketTick, 1u << 16>>();
    PendingReads pending_write_table; ///< the same per granule for GPU writes queued (NotePendingGpuWrite)
    /// Copies into guest memory by the command processor since the GPU last ran everything
    /// (a ring; `copies_begin`..`copies_end` bounds them all, empty when begin >= end).
    struct CopyIntoMemory {
        VAddr begin, end;
    };
    std::array<CopyIntoMemory, 64> copies_into_memory{};
    u32 num_copies_into_memory = 0;
    VAddr copies_begin = ~VAddr{0}, copies_end = 0;
    u64 copies_mask = 0;
    void SyncProgramWrites(VAddr address, u64 size);
    u64 untracked_reads_packet = 0; ///< newest packet reading guest memory it does not list
    u64 constram_waits_skipped = 0; ///< statistics: CPU writes that did not have to wait
    u64 constram_gpu_waits = 0;       ///< statistics: dumps that waited for the GPU (WaitForInPlaceReads)
    u64 constram_gpu_wait_cycles = 0;
    u64 proxy_samples = 0; ///< texture bindings that read a scene proxy (statistics)
    float sampler_lod_bias = 0.0f; ///< bbport: extra bias of this draw's samplers
    bool pipeline_is_compute = false; ///< bbport: the bound pipeline of BindResources
    bool scene_debug_frame = false; ///< BB_SCENE_DEBUG: this frame's passes are printed
    u32 preupload_frame = ~0u; ///< bbport: frame of the last background pre-upload
    bool PendingWriteOverlaps(VAddr address, u64 size);
    /// Stage B: the ring bindings of the stages of the packet being recorded.
    struct RingStage {
        const Shader::Info* info;
        const RingBinding* bindings;
        u32 count;
    };
    std::array<RingStage, Shader::MaxStageTypes> ring_stages{};
    u32 num_ring_stages = 0;
    /// bbport: the vertex stream V#s of the packet being recorded (stage B; empty elsewhere).
    std::span<const Gcn::Buffer> packet_vsharps;
    const RingBinding* FindRingBinding(const Shader::Info& stage, u32 index) const {
        for (u32 i = 0; i < num_ring_stages; ++i) {
            if (ring_stages[i].info == &stage) {
                for (u32 j = 0; j < ring_stages[i].count; ++j) {
                    if (ring_stages[i].bindings[j].index == index) {
                        // Left to stage B by its resolve worker: bound as without a ring.
                        const auto& binding = ring_stages[i].bindings[j];
                        return binding.offset >= RingNotCopied ? nullptr : &binding;
                    }
                }
                return nullptr;
            }
        }
        return nullptr;
    }
    VideoCore::DirectBuffers::Manager direct_buffers{instance, scheduler, runtime};
    std::unique_ptr<ConstantRing> constant_ring;
    std::unique_ptr<class OcclusionQueries> occlusion;
    std::unique_ptr<class ButtonPrompts> prompts; ///< bbport: the key guide icons for the device in use
    std::unique_ptr<class GpuTimestamps> timestamps;
    std::unique_ptr<class IndirectGuard> indirect_guard; ///< stage B only (lazily)
    /// Submissions (prepared draws) kept alive until stage B reaches the position.
    std::deque<std::pair<u64, std::shared_ptr<const void>>> pipe_keepalive;
    std::unique_ptr<DrawPipe> draw_pipe;

    /// BB_HONEST_LABELS (SignalAfterGpu). Last: its thread stops before the scheduler goes.
    struct GpuSignal {
        u64 tick;
        std::function<void()> signal;
        VAddr label;
        u64 value;
    };
    void GpuSignalLoop(std::stop_token token);
    /// bbport: GpuSignalLoop has waited 10 s for `tick`: prints whether the GPU or the port is stuck.
    void GpuWatchdog(u64 tick);
    std::mutex gpu_signals_mutex;
    std::condition_variable_any gpu_signals_cv;
    std::deque<GpuSignal> gpu_signals;
    /// bbport: signals registered on the draw recording thread since it last handed them over
    /// (PublishSignals: at the end of each guest submission and at every flush), one lock per
    /// batch instead of per fence (~450 fences a frame).
    std::vector<GpuSignal> local_signals;
    std::atomic<u32> local_signal_count{0};
    void PublishSignals();
    /// Draw pipe position up to which the recording thread's signals are handed over: fences
    /// handed to it before are in gpu_signals (or written).
    std::atomic<u64> signals_published_upto{0};
    std::atomic<bool> signal_flush_requested{false};
    std::jthread gpu_signal_thread;
    void SubmitForSignalsNow();
    /// steady_clock nanoseconds of the last submission (written by both pipeline stages).
    std::atomic<s64> last_flush_ns{0};
};

} // namespace Vulkan
