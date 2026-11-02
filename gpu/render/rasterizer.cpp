// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: Rasterizer, the draw and dispatch entry points: pipeline selection, then recording here or on the
// draw recording thread (raster_pipe.cpp), and the per-frame bookkeeping.

#include <mutex>
#include <set>
#include <string>
#include <filesystem>
#include "render/raster_internal.h"
#include "render/shader_hle.h"
#include "textures/direct_textures.h"
#include <vk_mem_alloc.h>

namespace Vulkan {

namespace {
thread_local u32 tls_draws_in_frame = 0;
thread_local bool tls_mid_frame_flushed = false;
} // namespace

Rasterizer::Rasterizer(const Instance& instance_, Scheduler& scheduler_, Runtime& runtime_,
                       BbCp::CommandProcessor* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, runtime{runtime_}, page_watch{this},
      buffer_cache{instance, scheduler, runtime, liverpool_, texture_cache, page_watch},
      texture_cache{instance, scheduler, runtime, liverpool_, buffer_cache, page_watch},
      liverpool{liverpool_}, memory{Core::Memory::Instance()},
      pipeline_cache{instance, scheduler, liverpool, buffer_cache.GetSparsePageShift()},
      host_markers_enabled{EmulatorSettings.IsVkHostMarkersEnabled()},
      guest_markers_enabled{EmulatorSettings.IsVkGuestMarkersEnabled()} {
    // Before the rasterizer is bound: Liverpool enqueues buffers only once it sees it.
    native_srv_cache =
        std::make_unique<ImageDescCacheEntry[]>(VideoCore::DirectTextures::kMaxNativeSrvs);
    native_sampler_cache =
        std::make_unique<NativeSamplerStageBEntry[]>(VideoCore::DirectTextures::kMaxNativeSamplers);
    draw_prep = std::make_unique<DrawPreparation>(pipeline_cache);
    // A program (or fetch shader) that does not look like one yet: the game may have copied it
    // into place with the GPU (DMA) in work recorded but not run yet (another game: all-zero
    // programs, the load hung). Called on the GPU command thread, inside a draw: the work
    // handed to the recording thread and recorded before it runs, so a
    // program the GPU copies into place (in place in guest memory) is there. No read back of
    // GPU-modified ranges here: downloading 512 KiB windows at draws overwrote what the CPU had
    // written there with older GPU data (another game: shader code zeroed, characters missing,
    // 5 FPS), and asking this very thread for one waited forever.
    pipeline_cache.SetProgramWriteSync(
        [this](VAddr address, u64 size) { SyncProgramWrites(address, size); });
    pipeline_cache.SetProgramMemorySync([this](VAddr, u64) {
        DrainDrawPipe(); // a copy into place may still be a packet for the recording thread
        Finish();
    });
    scene_targets = std::make_unique<SceneTargets>(instance, scheduler, runtime, texture_cache);
    // Object motion first: it fixes the buffer addresses the motion shader variants embed.
    object_motion = std::make_unique<ObjectMotion>(instance, scheduler);
    camera_motion = std::make_unique<CameraMotion>(instance, scheduler, texture_cache, runtime);
    camera_motion->SetObjectMotion(object_motion.get());
    upscaler = std::make_unique<TemporalUpscaler>(instance, scheduler, texture_cache, runtime,
                                                  *camera_motion, *scene_targets);
    prompts = std::make_unique<ButtonPrompts>(instance, scheduler, buffer_cache);
    if (!EmulatorSettings.IsNullGPU()) {
        liverpool->BindRasterizer(this);
    }
    BbGuestMemory::Install(instance); // bbport: before the runtime maps direct memory
    memory->SetRasterizer(this);
    buffer_cache.SetDirectBuffersManager(&direct_buffers);
    VideoCore::DirectBuffers::SetActiveCaches(&buffer_cache, &texture_cache);

    GpuProfiler::Init(instance, scheduler);
    if (DrawPipeWanted()) {
        constant_ring = std::make_unique<ConstantRing>(instance, scheduler);
        draw_pipe = std::make_unique<DrawPipe>(&RunDrawPacket, this);
        // Keep 1 constant resolve worker by default (`spin_us = 0` in `vk_draw_pipe.h` so it
        // never burns idle spin CPU): offloading `XXH3` + Write-Combined ReBAR `memcpy` from
        // Stage A keeps `ConstRing + Drain` at `0.30 ms/frame` instead of `1.14 ms/frame` on the
        // serial Stage A critical path (`20261102_085454.log`).
        static const u32 resolvers = [] {
            if (const char* env = std::getenv("BB_RESOLVE_WORKERS")) {
                return u32(std::clamp(std::atoi(env), 0, 4));
            }
            return BbThreads::Available() >= 8 ? 1u : 0u;
        }();
        if (resolvers != 0) {
            for (u32 i = 0; i < resolvers; ++i) {
                worker_rings.push_back(std::make_unique<ConstantRing>(instance, scheduler));
            }
            draw_pipe->StartResolvers(&ResolvePacket, resolvers);
            std::printf("GPU: %u constant resolve workers (BB_RESOLVE_WORKERS)\n", resolvers);
        }
    }
    // bbport: this thread joins the texture binding helper before it changes image state.
    runtime.SetImageAccessHook(&JoinBindHelper, this);
    scheduler.SetSubmitCallback([this](Vulkan::SubmitInfo& info) {
        if (buffer_cache.HasPendingInPassShadows()) {
            buffer_cache.FlushInPassShadows();
        }
        direct_buffers.FlushPendingWritebacks();
        runtime.FlushBarriers();
        buffer_cache.SubmitPendingArenaBinds(info);
    });
}

Rasterizer::~Rasterizer() {
    buffer_cache.SetDirectBuffersManager(nullptr);
    VideoCore::DirectBuffers::SetActiveCaches(nullptr, nullptr);
}

namespace {
/// BB_PASS_DUMP=addr,addr (hex guest addresses of render targets) with BB_PASS_DUMP_TRIGGER=<file>
/// (diagnostics, the flicker): while the file exists, every 8th row of those images is saved at
/// each display pass (BB_DUMP_DIR/pNNNNN_<addr>_<w>x<h>_<bits>.raw), little enough not to change
/// the frame's timing much.
std::vector<VAddr> PassDumpAddresses() {
    std::vector<VAddr> out;
    if (const char* env = std::getenv("BB_PASS_DUMP")) {
        for (const char* at = env; *at;) {
            char* end = nullptr;
            out.push_back(std::strtoull(at, &end, 16));
            at = *end ? end + 1 : end;
        }
    }
    return out;
}
} // namespace

void Rasterizer::DumpTracedInputs() {
    if (traced_inputs.empty()) {
        return;
    }
    static const std::string dir = [] {
        const char* env = std::getenv("BB_DUMP_DIR");
        return std::string{env && env[0] ? env : "out/dump"};
    }();
    VkDeviceSize total = 0;
    for (const auto& r : traced_inputs) total += r.size;
    const VkBufferCreateInfo buffer_ci{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                       .size = total,
                                       .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT};
    const VmaAllocationCreateInfo alloc_ci{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST};
    VkBuffer buffer{};
    VmaAllocation allocation{};
    VmaAllocationInfo info{};
    if (vmaCreateBuffer(instance.GetAllocator(), &buffer_ci, &alloc_ci, &buffer, &allocation,
                        &info) != VK_SUCCESS) {
        traced_inputs.clear();
        return;
    }
    scheduler.EndRendering();
    auto ranges = traced_inputs;
    scheduler.Record([ranges, buffer](vk::CommandBuffer cmd) {
        const vk::MemoryBarrier2 before{
            .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
            .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
        };
        cmd.pipelineBarrier2({.memoryBarrierCount = 1, .pMemoryBarriers = &before});
        VkDeviceSize at = 0;
        for (const auto& r : ranges) {
            cmd.copyBuffer(r.buffer, buffer, vk::BufferCopy{r.offset, at, r.size});
            at += r.size;
        }
    });
    std::string header;
    for (const auto& r : traced_inputs) {
        header += fmt::format("{:x}+{:x}:{} ", (u64)(VkBuffer)r.buffer, r.offset, r.size);
    }
    char path[512];
    std::snprintf(path, sizeof(path), "%s/g%06llu.bin", dir.c_str(),
                  (unsigned long long)traced_inputs_frame);
    scheduler.DeferPriorityOperation([allocator = instance.GetAllocator(), buffer, allocation, info,
                                      total, file = std::string{path}, header] {
        vmaInvalidateAllocation(allocator, allocation, 0, VK_WHOLE_SIZE);
        if (FILE* f = std::fopen(file.c_str(), "wb")) {
            const u32 n = u32(header.size());
            std::fwrite(&n, 4, 1, f);
            std::fwrite(header.data(), 1, n, f);
            std::fwrite(info.pMappedData, 1, total, f);
            std::fclose(f);
        }
        vmaDestroyBuffer(allocator, buffer, allocation);
    });
    traced_inputs.clear();
}

void Rasterizer::DumpPassImages(char tag) {
    static const std::vector<VAddr> addresses = PassDumpAddresses();
    static const char* trigger = std::getenv("BB_PASS_DUMP_TRIGGER");
    if (addresses.empty() || !trigger || !std::filesystem::exists(trigger)) {
        return;
    }
    static const std::string dir = [] {
        const char* env = std::getenv("BB_DUMP_DIR");
        return std::string{env && env[0] ? env : "out/dump"};
    }();
    static u32 indices[2] = {};
    const u32 n = indices[tag == 'p' ? 0 : 1]++;
    std::printf("Pass dump %c %u: GPU frame %llu\n", tag, n,
                (unsigned long long)BbStats::gpu_frames.load(std::memory_order_relaxed));
    scheduler.EndRendering();
    for (const VAddr address : addresses) {
        VideoCore::Image* found = nullptr;
        texture_cache.ForEachImageInRegion(address, 1, [&](VideoCore::ImageId, VideoCore::Image& image) {
            if (image.info.guest_address == address &&
                (!found || image.info.size.width > found->info.size.width)) {
                found = &image;
            }
        });
        if (!found || found->info.num_bits == 0) {
            continue;
        }
        const u32 width = found->info.size.width, height = found->info.size.height;
        const u32 bytes = found->info.num_bits / 8;
        static const u32 step = [] {
            const char* env = std::getenv("BB_PASS_DUMP_STEP");
            return env ? std::max(1, std::atoi(env)) : 8;
        }();
        const u32 rows = (height + step - 1) / step;
        const VkDeviceSize size = VkDeviceSize(width) * rows * bytes;
        const VkBufferCreateInfo buffer_ci{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                           .size = size,
                                           .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT};
        const VmaAllocationCreateInfo alloc_ci{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST};
        VkBuffer buffer{};
        VmaAllocation allocation{};
        VmaAllocationInfo info{};
        if (vmaCreateBuffer(instance.GetAllocator(), &buffer_ci, &alloc_ci, &buffer, &allocation,
                            &info) != VK_SUCCESS) {
            continue;
        }
        runtime.Transit(found, vk::ImageLayout::eTransferSrcOptimal,
                        vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead);
        runtime.FlushBarriers();
        const vk::Image image = found->GetImage();
        const vk::ImageAspectFlags aspect = found->aspect_mask & ~vk::ImageAspectFlagBits::eStencil;
        scheduler.Record([=](vk::CommandBuffer cmd) {
            std::vector<vk::BufferImageCopy> regions;
            regions.reserve(rows);
            for (u32 r = 0; r < rows; ++r) {
                regions.push_back({
                    .bufferOffset = VkDeviceSize(r) * width * bytes,
                    .imageSubresource = {aspect, 0, 0, 1},
                    .imageOffset = {0, s32(r * step), 0},
                    .imageExtent = {width, 1, 1},
                });
            }
            cmd.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, buffer, regions);
        });
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%c%05u_%llx_%ux%u_%u.raw", dir.c_str(), tag, n,
                      (unsigned long long)address, width, rows, found->info.num_bits);
        scheduler.DeferPriorityOperation([allocator = instance.GetAllocator(), buffer, allocation,
                                          info, size, file = std::string{path}] {
            vmaInvalidateAllocation(allocator, allocation, 0, VK_WHOLE_SIZE);
            if (FILE* f = std::fopen(file.c_str(), "wb")) {
                std::fwrite(info.pMappedData, 1, size, f);
                std::fclose(f);
            }
            vmaDestroyBuffer(allocator, buffer, allocation);
        });
    }
}


namespace {
// bbport: the pipeline's own type (optional<const ...>): the other one made each draw copy it,
// its attribute list included (a malloc per draw).
std::pair<u32, u32> GetDrawOffsets(
    const Gcn::Regs& regs, const Shader::Info& info,
    const std::optional<const Shader::Gcn::FetchShaderData>& fetch_shader) {
    u32 vertex_offset = regs.index_offset;
    u32 instance_offset = 0;
    if (fetch_shader) {
        if (vertex_offset == 0 && fetch_shader->vertex_offset_sgpr != -1) {
            vertex_offset = info.UserData()[fetch_shader->vertex_offset_sgpr];
        }
        if (fetch_shader->instance_offset_sgpr != -1) {
            instance_offset = info.UserData()[fetch_shader->instance_offset_sgpr];
        }
    }
    return {vertex_offset, instance_offset};
}
} // namespace

void Rasterizer::Draw(bool is_indexed, u32 index_offset, const PreparedDraw* prepared) {
    BB_SECTION(ADraw);
    BbStats::draws.fetch_add(1, std::memory_order_relaxed);

    // bbport: with the draw pipeline this thread only selects the pipeline and hands the draw
    // to the recording thread (DrawRecord there); draws FilterDraw handles itself run here.
    const bool pipelined = UseDrawPipe();
    if (pipelined && !FilterDrawPasses() &&
        !BbToggle::Disabled(BbToggle::PipelinedMemoryWrites)) {
        PostDraw(nullptr, nullptr, false, 0);
        return;
    }
    if (!pipelined || !FilterDrawPasses()) {
        DrainDrawPipe(DrawPipe::ReasonDraw);
        FrameCapture::Poll();
        scheduler.PopPendingOperations();
        if (!FilterDraw()) {
            return;
        }
    }
    const u32 prep_state =
        prepared ? prepared->state.load(std::memory_order_relaxed) : PreparedDraw::Pending;
    const PreparedDraw* ready_prepared =
        (prep_state == PreparedDraw::Ready) ? prepared : nullptr;
    const GraphicsPipeline* pipeline = pipeline_cache.GetGraphicsPipeline({}, ready_prepared);
    const PreparedDraw* used_prepared = pipeline_cache.UsedPrepared();
    if (prepared) {
        draw_prep->Count(used_prepared != nullptr,
                         prep_state == PreparedDraw::Pending       ? DrawPreparation::NotReady
                         : prep_state == PreparedDraw::Unavailable ? DrawPreparation::Unavailable
                         : prepared->reg_checksum != liverpool->gfx_reg_checksum
                             ? DrawPreparation::OtherRegisters
                             : DrawPreparation::OtherPrograms);
    }
    if (!pipeline) {
        return;
    }
    if (pipelined && FilterDrawPasses()) {
        PostDraw(pipeline, used_prepared, is_indexed, index_offset);
        return;
    }
    DrawRecord(pipeline, used_prepared, is_indexed, index_offset);
}

/// The guest memory a draw reads, in its breadcrumb: a GPU stuck in the draw is reported with it
/// (a stalled read of the game's memory on NVIDIA, whose hangs show no fault).
static void NoteCrumbReads(Breadcrumbs::Crumb& crumb, const GraphicsPipeline* pipeline,
                           const auto& regs, bool is_indexed, u32 index_offset) {
    u32 n = 0;
    const auto add = [&](char kind, u64 address, u32 size) {
        if (address && n < Breadcrumbs::Crumb::MaxReads) {
            crumb.read_kinds[n] = kind;
            crumb.reads[n] = address;
            crumb.read_sizes[n++] = size;
        }
    };
    if (is_indexed) {
        const u32 index_size = regs.index_buffer_type.index_type == Gcn::IndexType::Index16 ? 2u : 4u;
        add('i', regs.index_base_address.template Address<VAddr>() + u64(index_offset) * index_size,
            regs.num_indices * index_size);
    }
    const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
    if (const auto& fetch = pipeline->GetFetchShader()) {
        for (const auto& attribute : fetch->attributes) {
            const auto sharp = attribute.GetSharp(vs);
            add('v', sharp.base_address, sharp.GetSize());
        }
    }
    for (const auto* stage : {&vs, pipeline->GetStages()[u32(Shader::SwStage::Fragment)]}) {
        if (!stage) {
            continue;
        }
        for (const auto& buffer : stage->buffers) {
            if (!buffer.IsSpecial()) {
                const auto sharp = buffer.GetSharp(*stage);
                add('b', sharp.base_address, sharp.GetSize());
            }
        }
        for (const auto& image : stage->images) {
            add('t', image.GetSharp(*stage).Address(), 0);
        }
    }
}

void Rasterizer::DrawRecord(const GraphicsPipeline* pipeline, const PreparedDraw* used_prepared,
                            bool is_indexed, u32 index_offset) {
    BB_SECTION(DrawRecord);
    buffer_cache.NewPacket();
    if (DrawPipe::OnStageB()) {
        FrameCapture::Poll();
        scheduler.PopPendingOperations();
    }
    // bbport (BB_PREUPLOAD): once a frame, guest GPU memory the CPU wrote and then left alone is
    // uploaded ahead of its use, BB_PREUPLOAD_MB (16) at most per frame.
    if (const u32 frame = BbStats::frame_number.load(std::memory_order_relaxed);
        frame != preupload_frame) {
        preupload_frame = frame;
        BB_SECTION(Preupload);
        static const u64 budget = [] {
            const char* env = std::getenv("BB_PREUPLOAD_MB");
            return (env ? std::strtoull(env, nullptr, 10) : 16ull) << 20;
        }();
        buffer_cache.Preupload(budget);
        prompts->Update();
    }
    const auto& regs = Regs();
    // bbport: the pass copying a finished frame to a display buffer; the previous draw's
    // target is that frame.
    if (camera_motion->Enabled() && regs.color_buffers[0] &&
        FrameCapture::IsDisplayBuffer(regs.color_buffers[0].Address())) {
        scene_started = false;
        DumpTracedInputs();
        DumpPassImages('p');
        upscaler->OnDisplayPassStart(regs.color_buffers[0].Address());
        camera_motion->OnDisplayPass(cb_descs[0].first);
        if (upscaler->OnFrameStart()) {
            object_motion->InvalidateHistory();
            camera_motion->InvalidateHistory();
        }
        camera_motion->SetJitter(upscaler->Jitter());
        object_motion->OnFrameStart();
        NoteFrameStart();
        static const char* scene_debug = std::getenv("BB_SCENE_DEBUG");
        scene_debug_frame = scene_debug && std::remove(scene_debug) == 0;
        scene_targets->debug = scene_debug_frame;
    }
    // BB_SKIP_PS=hash[,hash...] (diagnostics): draws with these pixel shaders are left out.
    static const std::vector<u64> skipped_ps = [] {
        std::vector<u64> hashes;
        if (const char* env = std::getenv("BB_SKIP_PS")) {
            for (const char* at = env; *at;) {
                char* end = nullptr;
                hashes.push_back(std::strtoull(at, &end, 16));
                at = *end ? end + 1 : end;
            }
        }
        return hashes;
    }();
    if (!skipped_ps.empty()) {
        const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
        if (ps && std::ranges::find(skipped_ps, ps->pgm_hash) != skipped_ps.end()) {
            return;
        }
    }
    const auto is_attachmentless_noop = [&] {
        if (pipeline->GetStages()[u32(Shader::SwStage::Fragment)] != nullptr) {
            return false;
        }
        const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
        if (vs.pgm_hash != 0x89cda9e4ull) {
            return false;
        }
        const auto& key = pipeline->GetGraphicsKey();
        if (regs.color_control.mode != Gcn::ColorControl::OperationMode::Disable) {
            for (s32 cb = 0; cb < std::bit_width(key.mrt_mask); ++cb) {
                if (regs.color_buffers[cb] && regs.color_target_mask.GetMask(cb) &&
                    (key.mrt_mask & (1 << cb)) != 0) {
                    return false;
                }
            }
        }
        if ((regs.depth_control.depth_enable && regs.depth_buffer.DepthValid()) ||
            (regs.depth_control.stencil_enable && regs.depth_buffer.StencilValid())) {
            return false;
        }
        return !vs.uses_dma &&
               !std::ranges::any_of(vs.buffers, [](const auto& b) { return b.is_written; }) &&
               !std::ranges::any_of(vs.images, [](const auto& i) { return i.is_written; });
    }();
    if (is_attachmentless_noop) {
        return;
    }
    bind_prepared = used_prepared;
    motion_draw = pipeline->GetGraphicsKey().motion_vectors;
    motion_geometry = 0;

    // BB_EXP_FINISH_BEFORE_TARGET=addr (experiment, the flicker): the GPU runs everything before
    // a draw into that color target before the draw is recorded.
    static const VAddr finish_target = [] {
        const char* env = std::getenv("BB_EXP_FINISH_BEFORE_TARGET");
        return env ? VAddr(std::strtoull(env, nullptr, 16)) : VAddr{0};
    }();
    if (finish_target && Regs().color_buffers[0] &&
        Regs().color_buffers[0].Address() == finish_target) {
        scheduler.Finish();
    }
    // BB_PASS_DUMP_AT=addr (diagnostics): BB_PASS_DUMP's images also before the first draw of a
    // pass into that color target ('q' files).
    static const VAddr dump_at = [] {
        const char* env = std::getenv("BB_PASS_DUMP_AT");
        return env ? VAddr(std::strtoull(env, nullptr, 16)) : VAddr{0};
    }();
    if (dump_at && Regs().color_buffers[0] && Regs().color_buffers[0].Address() == dump_at &&
        !scheduler.IsRendering()) {
        const auto* q_ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
        std::printf("Pass dump q stages vs %08x ps %08x\n",
                    u32(pipeline->GetStage(Shader::SwStage::Vertex).pgm_hash), u32(q_ps ? q_ps->pgm_hash : 0));
        DumpPassImages('q');
    }
    // BB_EXP_TRANSIT_TARGET=addr (experiment): the target image goes through TransferSrcOptimal
    // before the first draw of a pass into it (what BB_PASS_DUMP_AT does, without the copy).
    static const VAddr transit_target = [] {
        const char* env = std::getenv("BB_EXP_TRANSIT_TARGET");
        return env ? VAddr(std::strtoull(env, nullptr, 16)) : VAddr{0};
    }();
    if (transit_target && Regs().color_buffers[0] &&
        Regs().color_buffers[0].Address() == transit_target && !scheduler.IsRendering()) {
        VideoCore::Image* found = nullptr;
        texture_cache.ForEachImageInRegion(transit_target, 1, [&](VideoCore::ImageId, VideoCore::Image& image) {
            if (image.info.guest_address == transit_target) found = &image;
        });
        if (found) {
            runtime.Transit(found, vk::ImageLayout::eTransferSrcOptimal,
                            vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead);
            runtime.FlushBarriers();
        }
    }
    // BB_EXP_BARRIER_TARGET=addr (experiment): a full memory barrier before each draw into that
    // color target (its render pass is ended first).
    static const VAddr barrier_target = [] {
        const char* env = std::getenv("BB_EXP_BARRIER_TARGET");
        return env ? VAddr(std::strtoull(env, nullptr, 16)) : VAddr{0};
    }();
    if (barrier_target && Regs().color_buffers[0] &&
        Regs().color_buffers[0].Address() == barrier_target) {
        scheduler.EndRendering();
        scheduler.Record([](vk::CommandBuffer cmd) {
            const vk::MemoryBarrier2 all{
                .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                .dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
            };
            cmd.pipelineBarrier2({.memoryBarrierCount = 1, .pMemoryBarriers = &all});
        });
    }
    PrepareRenderState(pipeline);
    NoteUpscalerDraw(pipeline);
    const PreparedDraw* draw_prepared = bind_prepared;
    // bbport: vertex and index buffers are resolved while the helper binds textures (their
    // commands are recorded after BeginRendering, as before).
    draw_inputs = {pipeline, draw_prepared, index_offset, is_indexed, true, false};
    const bool bound = BindResources(pipeline);
    bind_prepared = nullptr; // indirect draws and dispatches bind without prepared sharps
    const bool inputs_resolved = draw_inputs.resolved;
    draw_inputs.pending = false;
    if (!bound) {
        draw_inputs.pipeline = nullptr;
        return;
    }
    const auto state = BeginRendering(pipeline);

    if (!inputs_resolved) {
        ResolveVertexBuffers(pipeline, draw_prepared);
        if (is_indexed) {
            ResolveIndexBuffer(index_offset);
        }
    }
    draw_inputs.pipeline = nullptr;
    if (HasPendingHleCopies() && scheduler.WillBeginRendering(state)) {
        FlushPendingHleCopies(*this);
    }
    ++tls_draws_in_frame;
    static const u32 mid_flush_draws = [] {
        if (const char* env = std::getenv("BB_MID_FLUSH_DRAWS")) {
            return static_cast<u32>(std::max(0, std::atoi(env)));
        }
        return 900u;
    }();
    if (!HonestLabels() && mid_flush_draws != 0 && !tls_mid_frame_flushed &&
        tls_draws_in_frame >= mid_flush_draws && scheduler.WillBeginRendering(state)) {
        tls_mid_frame_flushed = true;
        SubmitInfo info{};
        scheduler.Flush(info);
    }
    EmitVertexBuffers();
    if (is_indexed) {
        EmitIndexBuffer();
    }

    if (needs_barrier || (scheduler.WillBeginRendering(state) && runtime.HasPendingBarriers())) {
        runtime.FlushBarriers();
    }

    ScaleToTarget(pipeline);
    if (motion_draw && motion_geometry) {
        push_data.motion_param = 0;
        if (upscaler->Enabled() || camera_motion->DebugOverlay()) {
            BB_SECTION(Motion);
            const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
            const auto [base_vertex, first_instance] =
                GetDrawOffsets(regs, vs, pipeline->GetFetchShader());
            const u32 index_size =
                regs.index_buffer_type.index_type == Gcn::IndexType::Index16 ? 2u : 4u;
            const VAddr index_address = is_indexed
                ? regs.index_base_address.Address<VAddr>() + u64(index_offset) * index_size : 0;
            // Without a character-size skeleton the draw is gated on its bone palette changing
            // since last frame (model constants follow the camera, the palette does not). The
            // check comes first: most gated draws are static world pieces with long index lists.
            static const bool all_motion = [] {
                const char* value = std::getenv("BB_OBJECT_MOTION_ALL");
                return value && value[0] == '1';
            }();
            bool gated = !all_motion;
            u64 palette = 0;
            const PreparedStage* prep_vs = nullptr;
            if (draw_prepared && !BbToggle::Disabled(BbToggle::PreparedResources)) {
                for (u32 i = 0; i < draw_prepared->num_stages; ++i) {
                    const auto& candidate = draw_prepared->stages[i];
                    if (&candidate.program->info == &vs &&
                        candidate.num_buffers == vs.buffers.size()) {
                        prep_vs = &candidate;
                        break;
                    }
                }
            }
            for (u32 b_idx = 0; b_idx < vs.buffers.size(); ++b_idx) {
                const auto& resource = vs.buffers[b_idx];
                if (resource.IsSpecial()) continue;
                const auto buffer = prep_vs ? prep_vs->buffer_sharps[b_idx] : resource.GetSharp(vs);
                const u32 size = buffer.GetSize();
                if (!buffer.Valid() || buffer.GetStride() != 16) continue;
                const auto role = Motion::ClassifyBuffer(size);
                if (role == Motion::BufferRole::Skeleton) {
                    gated = false;
                    break;
                }
                const VAddr address = buffer.base_address;
                if (role == Motion::BufferRole::SmallSkeleton &&
                    memory->ClampRangeSize(address, size) == size) {
                    palette = XXH3_64bits_withSeed(reinterpret_cast<const void*>(address), size,
                                                   palette);
                }
            }
            if (!gated || object_motion->Moving({.shader = vs.pgm_hash,
                                                 .geometry = motion_geometry,
                                                 .indices = index_address,
                                                 .index_count = regs.num_indices,
                                                 .instances = regs.num_instances.NumInstances(),
                                                 .first_instance = first_instance},
                                                palette)) {
                Motion::VertexRange range{base_vertex, regs.num_indices};
                u64 topology = 0;
                if (is_indexed) {
                    range = {};
                    const u64 bytes = u64(regs.num_indices) * index_size;
                    if (index_address && memory->ClampRangeSize(index_address, bytes) == bytes) {
                        const bool restart = regs.enable_primitive_restart != 0;
                        const u32 count = regs.num_indices;
                        const auto scanned = object_motion->IndexRange(
                            {index_address, count, index_size, s32(base_vertex), restart}, [&] {
                                const auto* data = reinterpret_cast<const void*>(index_address);
                                const auto scan_range =
                                    index_size == 2
                                        ? Motion::IndexedRange(
                                              std::span(static_cast<const u16*>(data), count),
                                              s32(base_vertex), restart)
                                        : Motion::IndexedRange(
                                              std::span(static_cast<const u32*>(data), count),
                                              s32(base_vertex), restart);
                                return Motion::IndexRangeCache::Result{scan_range,
                                                                       XXH3_64bits(data, bytes)};
                            });
                        range = scanned.range;
                        topology = scanned.topology;
                    }
                }
                push_data.motion_param = object_motion->PrepareDraw({
                    .shader = vs.pgm_hash,
                    .geometry = motion_geometry,
                    .indices = index_address,
                    .topology = topology,
                    .index_count = regs.num_indices,
                    .instances = regs.num_instances.NumInstances(),
                    .first_instance = first_instance,
                    .vertices = range,
                });
            }
        }
    }
    // BB_TRACE_SHADER with BB_GPU_PROFILE: a mark before the traced vertex shader's draws too.
    if (auto* profiler = GpuProfiler::Get()) {
        static const char* traced = std::getenv("BB_TRACE_SHADER");
        const u64 vs_hash = pipeline->GetStage(Shader::SwStage::Vertex).pgm_hash;
        if (traced && std::strtoull(traced, nullptr, 16) == vs_hash) {
            profiler->Mark(vs_hash ^ 0xBEF0'0000ull, [&] {
                return fmt::format("the draw of vs {:016x} (from before its pass begins)", vs_hash);
            });
        }
    }
    // Geometry drawn with the scene depth is jittered, full-screen passes are not (a shifted
    // full-screen quad leaves an edge column unwritten).
    // BB_TRACE_GPU_INPUTS=vs hash (diagnostics, the flicker): the buffer ranges this draw reads
    // (descriptors and vertex streams), copied by the GPU at the next display pass (DumpPassImages).
    if (static const u64 trace_inputs = [] {
            const char* env = std::getenv("BB_TRACE_GPU_INPUTS");
            return env ? std::strtoull(env, nullptr, 16) : 0ull;
        }();
        trace_inputs && pipeline->GetStage(Shader::SwStage::Vertex).pgm_hash == trace_inputs) {
        traced_inputs.clear();
        for (const auto& info : buffer_infos) {
            if (info.buffer && info.range != VK_WHOLE_SIZE && info.range <= 65536) {
                traced_inputs.push_back({info.buffer, info.offset, info.range});
            }
        }
        for (u32 i = 0; i < vertex_binds.num_buffers; ++i) {
            if (vertex_binds.host_buffers[i] && vertex_binds.host_sizes[i] <= 65536) {
                traced_inputs.push_back({vertex_binds.host_buffers[i], vertex_binds.host_offsets[i],
                                         vertex_binds.host_sizes[i]});
            }
        }
        traced_inputs_frame = BbStats::gpu_frames.load(std::memory_order_relaxed);
    }
    BeginDraw(pipeline, state, is_indexed,
              regs.num_indices > 6 || regs.num_instances.NumInstances() > 1, draw_prepared,
              index_offset, true);

    const auto& vs_info = pipeline->GetStage(Shader::SwStage::Vertex);
    const auto& fetch_shader = pipeline->GetFetchShader();
    const auto [vertex_offset, instance_offset] = GetDrawOffsets(regs, vs_info, fetch_shader);

    const vk::Pipeline handle = pipeline->Handle();
    const u32 num_indices = regs.num_indices;
    // BB_TRACE_PIPELINE=vs hash (diagnostics, the flicker): its pipeline handle and push data,
    // printed when they differ from its previous draw.
    if (static const u64 traced_pipeline = [] {
            const char* env = std::getenv("BB_TRACE_PIPELINE");
            return env ? std::strtoull(env, nullptr, 16) : 0ull;
        }();
        traced_pipeline && vs_info.pgm_hash == traced_pipeline) {
        static VkPipeline last_handle{};
        static std::array<float, 4> last_push{};
        const std::array<float, 4> push{push_data.xscale, push_data.xoffset, push_data.yscale,
                                        push_data.yoffset};
        static u32 prints = 0;
        if ((VkPipeline(handle) != last_handle || push != last_push) && prints++ < 2000) {
            std::printf("Pipeline trace: gpu frame %llu vs %016llx handle %p push %g %g %g %g "
                        "clipoff %d vte %#x\n",
                        (unsigned long long)BbStats::gpu_frames.load(std::memory_order_relaxed),
                        (unsigned long long)vs_info.pgm_hash, (void*)VkPipeline(handle), push[0],
                        push[1], push[2], push[3], int(regs.IsClipDisabled()),
                        std::bit_cast<u32>(regs.viewport_control));
            last_handle = handle;
            last_push = push;
        }
    }
    const u32 num_instances = regs.num_instances.NumInstances();
    const u32 first_vertex = vertex_offset;
    const u32 first_instance = instance_offset;
    if (!Breadcrumbs::Enabled() && !DynCheck::Enabled()) [[likely]] {
        const u64 cur_epoch = scheduler.DescriptorStateEpoch();
        const bool need_bind_pipeline =
            (bound_graphics_epoch != cur_epoch) || (bound_graphics_pipeline != handle);
        bound_graphics_pipeline = handle;
        bound_graphics_epoch = cur_epoch;
        if (is_indexed) {
            if (need_bind_pipeline) {
                scheduler.Record([handle, num_indices, num_instances, first_vertex,
                                  first_instance](vk::CommandBuffer cmdbuf) {
                    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, handle);
                    cmdbuf.drawIndexed(num_indices, num_instances, 0, s32(first_vertex),
                                       first_instance);
                });
            } else {
                scheduler.Record([num_indices, num_instances, first_vertex,
                                  first_instance](vk::CommandBuffer cmdbuf) {
                    cmdbuf.drawIndexed(num_indices, num_instances, 0, s32(first_vertex),
                                       first_instance);
                });
            }
        } else {
            if (need_bind_pipeline) {
                scheduler.Record([handle, num_indices, num_instances, first_vertex,
                                  first_instance](vk::CommandBuffer cmdbuf) {
                    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, handle);
                    cmdbuf.draw(num_indices, num_instances, first_vertex, first_instance);
                });
            } else {
                scheduler.Record([num_indices, num_instances, first_vertex,
                                  first_instance](vk::CommandBuffer cmdbuf) {
                    cmdbuf.draw(num_indices, num_instances, first_vertex, first_instance);
                });
            }
        }
    } else {
        bound_graphics_pipeline = handle;
        bound_graphics_epoch = scheduler.DescriptorStateEpoch();
        const auto* ps_info = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
        Breadcrumbs::Crumb crumb{
            .kind = is_indexed ? Breadcrumbs::Kind::DrawIndexed : Breadcrumbs::Kind::Draw,
            .hash = {vs_info.pgm_hash, ps_info ? ps_info->pgm_hash : 0},
            .program = {vs_info.ProgramBase(), ps_info ? ps_info->ProgramBase() : 0},
            .count = {num_indices, num_instances, 0},
        };
        static const bool crumb_reads = [&] {
            const char* env = std::getenv("BB_BREADCRUMB_READS");
            return env ? env[0] == '1' : instance.GetVendorID() == 0x10de;
        }();
        if (crumb_reads && Breadcrumbs::Enabled()) {
            NoteCrumbReads(crumb, pipeline, regs, is_indexed, index_offset);
        }
        const Scissors check_scissors =
            DynCheck::Enabled() ? scheduler.GetDynamicState().CurrentScissors() : Scissors{};
        const Viewports check_viewports =
            DynCheck::Enabled() ? scheduler.GetDynamicState().CurrentViewports() : Viewports{};
        const u64 check_vs = vs_info.pgm_hash, check_ps = ps_info ? ps_info->pgm_hash : 0;
        scheduler.RecordCrumb(crumb, [=](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, handle);
            if (DynCheck::Enabled()) {
                DynCheck::Draw(cmdbuf, check_viewports, check_scissors, check_vs, check_ps);
            }
            if (is_indexed) {
                cmdbuf.drawIndexed(num_indices, num_instances, 0, s32(first_vertex),
                                   first_instance);
            } else {
                cmdbuf.draw(num_indices, num_instances, first_vertex, first_instance);
            }
        });
    }
    // BB_TRACE_SHADER with BB_GPU_PROFILE: a mark right after the traced vertex shader's draws,
    // so the profile splits their pass into the draw and what follows it unlabeled.
    if (auto* profiler = GpuProfiler::Get()) {
        static const char* traced = std::getenv("BB_TRACE_SHADER");
        if (traced && std::strtoull(traced, nullptr, 16) == vs_info.pgm_hash) {
            profiler->Mark(vs_info.pgm_hash ^ 0xAF7E'0000ull, [&] {
                return fmt::format("after the draw of vs {:016x}", vs_info.pgm_hash);
            });
        }
    }
    if (FrameCapture::Active()) {
        const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
        FrameCapture::Draw(vs_info.pgm_hash, ps ? ps->pgm_hash : 0, num_indices, num_instances);
        if (const auto& bc = regs.blend_control[0]; bc.enable && regs.color_buffers[0]) {
            char note[128];
            std::snprintf(note, sizeof(note), "\n  blend ps %08x idx %u: src %u dst %u func %u z %u%u",
                          u32(ps ? ps->pgm_hash : 0), num_indices, u32(bc.color_src_factor),
                          u32(bc.color_dst_factor), u32(bc.color_func),
                          u32(regs.depth_control.depth_enable),
                          u32(regs.depth_control.depth_write_enable));
            FrameCapture::Note(note);
        }
    }

    static const bool watch = [] {
        const char* env = std::getenv("BB_DRAW_WATCH");
        return env && env[0] == '1';
    }();
    if (watch) {
        WatchDraw(pipeline, state);
    }

    BB_SECTION(Kick);
    ResetBindings(false);
    scheduler.KickRecording();
}

void Rasterizer::NoteUpscalerDraw(const GraphicsPipeline* pipeline) {
    // Single-target passes: the upscaler looks for the scene's and the UI's (UiComposition).
    if (upscaler->Enabled() && std::popcount(pipeline->GetGraphicsKey().mrt_mask) == 1) {
        const auto& viewport = Regs().viewports[0];
        upscaler->OnDraw(pipeline->GetStage(Shader::SwStage::Vertex).pgm_hash,
                         cb_descs[0].first, db_desc.first,
                         UiComposition::NativeViewport(viewport.xscale * 2, viewport.yscale * 2));
    }
}

void Rasterizer::ScaleToTarget(const GraphicsPipeline* pipeline) {
    UiEdgeStretch(pipeline);
    // Screen-space (clip disabled) draws into the upscaler's output-size images.
    push_data.xscale *= target_scale[0];
    push_data.xoffset *= target_scale[0];
    push_data.yscale *= target_scale[1];
    push_data.yoffset *= target_scale[1];
}

namespace {

constexpr u32 RequiredFormatWriteMask(vk::Format fmt) noexcept {
    switch (fmt) {
    case vk::Format::eR8Unorm:
    case vk::Format::eR8Snorm:
    case vk::Format::eR8Uint:
    case vk::Format::eR8Sint:
    case vk::Format::eR8Srgb:
    case vk::Format::eR16Unorm:
    case vk::Format::eR16Snorm:
    case vk::Format::eR16Uint:
    case vk::Format::eR16Sint:
    case vk::Format::eR16Sfloat:
    case vk::Format::eR32Uint:
    case vk::Format::eR32Sint:
    case vk::Format::eR32Sfloat:
        return 0x1u;
    case vk::Format::eR8G8Unorm:
    case vk::Format::eR8G8Snorm:
    case vk::Format::eR8G8Uint:
    case vk::Format::eR8G8Sint:
    case vk::Format::eR8G8Srgb:
    case vk::Format::eR16G16Unorm:
    case vk::Format::eR16G16Snorm:
    case vk::Format::eR16G16Uint:
    case vk::Format::eR16G16Sint:
    case vk::Format::eR16G16Sfloat:
    case vk::Format::eR32G32Uint:
    case vk::Format::eR32G32Sint:
    case vk::Format::eR32G32Sfloat:
        return 0x3u;
    case vk::Format::eB10G11R11UfloatPack32:
    case vk::Format::eR5G6B5UnormPack16:
    case vk::Format::eB5G6R5UnormPack16:
        return 0x7u;
    default:
        return 0xFu;
    }
}

float HalfToFloat(u16 h) noexcept {
    const u32 sign = u32(h & 0x8000u) << 16;
    const u32 exp = (h >> 10) & 0x1Fu;
    const u32 mant = h & 0x03FFu;
    if (exp == 0) {
        if (mant == 0) {
            return std::bit_cast<float>(sign);
        }
        const float mag = float(mant) * (1.0f / float(1u << 24));
        return (sign != 0) ? -mag : mag;
    }
    if (exp == 31) {
        return std::bit_cast<float>(sign | 0x7F800000u | (mant << 13));
    }
    return std::bit_cast<float>(sign | ((exp + 112u) << 23) | (mant << 13));
}

bool DecodeFloatVertexAttr(const Gcn::Buffer& vb, const u8* ptr, u64 avail_bytes,
                           std::array<float, 4>& out_xyzw) noexcept {
    const auto dfmt = vb.GetDataFmt();
    const auto nfmt = vb.GetNumberFmt();
    std::array<float, 4> raw{0.0f, 0.0f, 0.0f, 1.0f};
    if (nfmt == Gcn::NumberFormat::Float) {
        switch (dfmt) {
        case Gcn::DataFormat::Format32_32_32_32:
            if (avail_bytes < 16) return false;
            std::memcpy(raw.data(), ptr, 16);
            break;
        case Gcn::DataFormat::Format32_32_32:
            if (avail_bytes < 12) return false;
            std::memcpy(raw.data(), ptr, 12);
            break;
        case Gcn::DataFormat::Format32_32:
            if (avail_bytes < 8) return false;
            std::memcpy(raw.data(), ptr, 8);
            break;
        case Gcn::DataFormat::Format32:
            if (avail_bytes < 4) return false;
            std::memcpy(raw.data(), ptr, 4);
            break;
        case Gcn::DataFormat::Format16_16_16_16: {
            if (avail_bytes < 8) return false;
            u16 h[4];
            std::memcpy(h, ptr, 8);
            for (u32 i = 0; i < 4; ++i) raw[i] = HalfToFloat(h[i]);
            break;
        }
        case Gcn::DataFormat::Format16_16: {
            if (avail_bytes < 4) return false;
            u16 h[2];
            std::memcpy(h, ptr, 4);
            raw[0] = HalfToFloat(h[0]);
            raw[1] = HalfToFloat(h[1]);
            break;
        }
        default:
            return false;
        }
    } else if (nfmt == Gcn::NumberFormat::Snorm && dfmt == Gcn::DataFormat::Format16_16_16_16) {
        if (avail_bytes < 8) return false;
        s16 s[4];
        std::memcpy(s, ptr, 8);
        for (u32 i = 0; i < 4; ++i) {
            raw[i] = std::max(-1.0f, float(s[i]) * (1.0f / 32767.0f));
        }
    } else if (nfmt == Gcn::NumberFormat::Snorm && dfmt == Gcn::DataFormat::Format16_16) {
        if (avail_bytes < 4) return false;
        s16 s[2];
        std::memcpy(s, ptr, 4);
        raw[0] = std::max(-1.0f, float(s[0]) * (1.0f / 32767.0f));
        raw[1] = std::max(-1.0f, float(s[1]) * (1.0f / 32767.0f));
    } else {
        return false;
    }
    out_xyzw = vb.DstSelect().Apply(raw);
    return true;
}

struct WinPt {
    float x = 0.0f;
    float y = 0.0f;
};

float Cross2D(WinPt a, WinPt b, WinPt p) noexcept {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

bool SameWinPt(WinPt a, WinPt b) noexcept {
    return std::abs(a.x - b.x) <= 2e-2f && std::abs(a.y - b.y) <= 2e-2f;
}

bool ConvexPolyContainsRect(std::span<const WinPt> poly, float orient_sign, float w,
                            float h) noexcept {
    const std::array<WinPt, 4> corners{{{0.0f, 0.0f}, {w, 0.0f}, {0.0f, h}, {w, h}}};
    const size_t n = poly.size();
    for (size_t i = 0; i < n; ++i) {
        const WinPt a = poly[i];
        const WinPt b = poly[(i + 1) % n];
        const float edge_l1 = std::abs(b.x - a.x) + std::abs(b.y - a.y);
        if (edge_l1 <= 1e-3f) {
            return false;
        }
        const float tol = 2e-2f * edge_l1;
        for (const WinPt c : corners) {
            if (orient_sign * Cross2D(a, b, c) < -tol) {
                return false;
            }
        }
    }
    return true;
}

struct WinTri {
    std::array<WinPt, 3> v{};
    float det = 0.0f;
};

bool TriangleCoversRect(const WinTri& t, float w, float h) noexcept {
    return ConvexPolyContainsRect(t.v, t.det > 0.0f ? 1.0f : -1.0f, w, h);
}

bool TwoTrianglesCoverRect(const WinTri& t0, const WinTri& t1, float w, float h) noexcept {
    if (TriangleCoversRect(t0, w, h) || TriangleCoversRect(t1, w, h)) {
        return true;
    }
    int m0_a = -1, m0_b = -1, m1_a = -1, m1_b = -1;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (SameWinPt(t0.v[i], t1.v[j])) {
                if (m0_a == -1) {
                    m0_a = i;
                    m1_a = j;
                } else if (i != m0_a && j != m1_a) {
                    m0_b = i;
                    m1_b = j;
                }
            }
        }
    }
    if (m0_a == -1 || m0_b == -1) {
        return false;
    }
    const int p_idx = 3 - m0_a - m0_b;
    const int q_idx = 3 - m1_a - m1_b;
    const WinPt u = t0.v[m0_a];
    const WinPt v = t0.v[m0_b];
    const WinPt p = t0.v[p_idx];
    const WinPt q = t1.v[q_idx];
    const float cp_u_v_p = Cross2D(u, v, p);
    const float cp_u_v_q = Cross2D(u, v, q);
    // P and Q must lie strictly on opposite sides of shared diagonal UV.
    if (!((cp_u_v_p > 1e-2f && cp_u_v_q < -1e-2f) || (cp_u_v_p < -1e-2f && cp_u_v_q > 1e-2f))) {
        return false;
    }
    const float cp_p_q_u = Cross2D(p, q, u);
    const float cp_p_q_v = Cross2D(p, q, v);
    // U and V must lie strictly on opposite sides of PQ so U -> P -> V -> Q is strictly convex.
    if (!((cp_p_q_u > 1e-2f && cp_p_q_v < -1e-2f) || (cp_p_q_u < -1e-2f && cp_p_q_v > 1e-2f))) {
        return false;
    }
    const std::array<WinPt, 4> quad{u, p, v, q};
    // Along U -> P -> V, point V lies on the side sign(Cross2D(u, p, v)) = sign(-cp_u_v_p).
    const float orient_sign = (-cp_u_v_p > 0.0f) ? 1.0f : -1.0f;
    return ConvexPolyContainsRect(quad, orient_sign, w, h);
}

} // namespace

u8 Rasterizer::ComputeFullscreenOverwriteMask(const GraphicsPipeline* pipeline,
                                              const RenderState& state,
                                              const PreparedDraw* draw_prepared, bool is_indexed,
                                              u32 index_offset) {
    static const bool dont_care_off = [] {
        const char* env = std::getenv("BB_DONT_CARE_OFF");
        return env && env[0] == '1';
    }();
    if (dont_care_off || !pipeline) {
        return 0;
    }
    if (state.num_color_attachments == 0 || state.num_color_attachments > Gcn::NUM_COLOR_BUFFERS ||
        state.width == 0 || state.height == 0 || state.num_layers != 1) {
        return 0;
    }
    const auto& regs = Regs();
    const auto& key = pipeline->GetGraphicsKey();
    if (key.num_samples != 1 || attachment_feedback_loop ||
        ui_placement.edge_x != 0.0f || ui_placement.edge_y != 0.0f) {
        return 0;
    }
    if (regs.color_control.mode != Gcn::ColorControl::OperationMode::Normal ||
        key.logic_op != Gcn::ColorControl::LogicOp::Copy ||
        regs.clipper_control.dx_rasterization_kill ||
        regs.clipper_control.user_clip_plane_enable != 0 ||
        regs.vs_output_control.clip_distance_enable != 0 ||
        regs.vs_output_control.cull_distance_enable != 0 ||
        regs.vs_output_control.use_vtx_kill_flag ||
        regs.stage_enable.raw != Gcn::ShaderStageEnable::VgtStages::Vs ||
        key.polygon_mode != Gcn::PolygonMode::Fill) {
        return 0;
    }
    if (regs.depth_control.depth_enable || regs.depth_control.stencil_enable ||
        regs.depth_control.depth_bounds_enable) {
        return 0;
    }
    const u32 count = regs.num_indices;
    if (regs.num_instances.NumInstances() == 0 || count < 3 || count > 6) {
        return 0;
    }
    const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
    if (!ps || ps->has_discard) {
        return 0;
    }

    const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
    enum class VsKind : u8 {
        None,
        Procedural89cda9e4,
        PassThroughXYZW,
        PassThroughXYZ1,
        PassThroughXYCb0Z_W,
        YebisCb20_21ScaleXY,
    };
    VsKind vs_kind = VsKind::None;
    switch (vs.pgm_hash) {
    case 0x89cda9e4ull:
        vs_kind = VsKind::Procedural89cda9e4;
        break;
    case 0x0b0acf50ull:
    case 0x061fbbf6ull:
        vs_kind = VsKind::PassThroughXYZW;
        break;
    case 0x637500e0ull:
        vs_kind = VsKind::PassThroughXYZ1;
        break;
    case 0x8747a367ull:
        vs_kind = VsKind::PassThroughXYCb0Z_W;
        break;
    case 0x7d668276ull:
    case 0x1ea5a07eull:
    case 0xd3ca03f3ull:
        vs_kind = VsKind::YebisCb20_21ScaleXY;
        break;
    default:
        return 0;
    }

    u8 candidate_mask = 0;
    for (u32 cb = 0; cb < state.num_color_attachments; ++cb) {
        const auto& att = state.color_attachments[cb];
        if (att.is_clear || !att.image_view || (key.mrt_mask & (1u << cb)) == 0 ||
            !cb_descs[cb].first) {
            continue;
        }
        if (key.blend_controls[cb].enable) {
            continue;
        }
        const auto& img = texture_cache.GetImage(cb_descs[cb].first);
        const u32 req_mask = RequiredFormatWriteMask(img.info.pixel_format);
        if ((u32(key.write_masks[cb]) & req_mask) != req_mask) {
            continue;
        }
        candidate_mask |= u8(1u << cb);
    }
    if (candidate_mask == 0) {
        return 0;
    }

    const auto& ds = scheduler.GetDynamicState();
    const auto& scs = ds.CurrentScissors();
    const auto& vps = ds.CurrentViewports();
    if (scs.empty() || vps.empty()) {
        return 0;
    }
    if (scs[0].offset.x > 0 || scs[0].offset.y > 0 ||
        s64(scs[0].offset.x) + s64(scs[0].extent.width) < s64(state.width) ||
        s64(scs[0].offset.y) + s64(scs[0].extent.height) < s64(state.height)) {
        return 0;
    }
    const float w_f = float(state.width);
    const float h_f = float(state.height);
    const bool clip_disabled = regs.IsClipDisabled();
    const vk::Viewport& vp = vps[0];
    if (!clip_disabled) {
        const float vp_x0 = std::min(vp.x, vp.x + vp.width);
        const float vp_x1 = std::max(vp.x, vp.x + vp.width);
        const float vp_y0 = std::min(vp.y, vp.y + vp.height);
        const float vp_y1 = std::max(vp.y, vp.y + vp.height);
        if (vp_x0 > 1e-2f || vp_x1 < w_f - 1e-2f || vp_y0 > 1e-2f || vp_y1 < h_f - 1e-2f) {
            return 0;
        }
    } else if (vp.x > 1e-2f || vp.y > 1e-2f || vp.width < w_f - 1e-2f || vp.height < h_f - 1e-2f) {
        return 0;
    }

    const auto resolve_cpu_ptr = [&](VAddr addr, u64 bytes) -> const u8* {
        if (addr == 0 || bytes == 0) {
            return nullptr;
        }
        if (HasPendingHleCopies() && PendingHleCopiesConflict(addr, bytes, false)) {
            return nullptr;
        }
        if (const u8* mirror = direct_buffers.TryGetCpuMirrorPtr(addr, bytes)) {
            return mirror;
        }
        if (VideoCore::DirectBuffers::OverlapsNativeOwnedRange(addr, bytes)) {
            return nullptr;
        }
        const bool gpu_clean =
            VideoCore::DirectBuffers::IsTransientCueHeap(addr, bytes)
                ? (!VideoCore::DirectBuffers::IsCueHeapEverGpuModified() ||
                   !buffer_cache.IsRegionGpuModified(addr, bytes))
                : !buffer_cache.IsRegionGpuModified(addr, bytes);
        if (!gpu_clean || memory->ClampRangeSize(addr, bytes) != bytes) {
            return nullptr;
        }
        return reinterpret_cast<const u8*>(addr);
    };

    const auto& fetch = pipeline->GetFetchShader();
    const auto [vertex_offset, instance_offset] = GetDrawOffsets(regs, vs, fetch);
    (void)instance_offset;

    std::array<u32, 6> vtx_indices{};
    if (!is_indexed) {
        for (u32 i = 0; i < count; ++i) {
            vtx_indices[i] = vertex_offset + i;
        }
    } else {
        const bool index16 = regs.index_buffer_type.index_type == Gcn::IndexType::Index16;
        const u32 idx_size = index16 ? sizeof(u16) : sizeof(u32);
        const VAddr ib_addr =
            regs.index_base_address.Address<VAddr>() + u64(index_offset) * idx_size;
        const u64 ib_bytes = u64(count) * idx_size;
        const u8* ib_ptr = resolve_cpu_ptr(ib_addr, ib_bytes);
        if (!ib_ptr) {
            return 0;
        }
        const bool prim_restart = (regs.enable_primitive_restart & 1) != 0;
        const s64 v_off = static_cast<s64>(static_cast<s32>(vertex_offset));
        for (u32 i = 0; i < count; ++i) {
            u32 raw_idx = 0;
            if (index16) {
                u16 v16 = 0;
                std::memcpy(&v16, ib_ptr + i * sizeof(u16), sizeof(u16));
                if (prim_restart && v16 == u16(regs.primitive_restart_index & 0xFFFFu)) {
                    return 0;
                }
                raw_idx = v16;
            } else {
                std::memcpy(&raw_idx, ib_ptr + i * sizeof(u32), sizeof(u32));
                if (prim_restart && raw_idx == regs.primitive_restart_index) {
                    return 0;
                }
            }
            const s64 eff = v_off + static_cast<s64>(raw_idx);
            if (eff < 0 || eff > 0x7FFFFFFFLL) {
                return 0;
            }
            vtx_indices[i] = static_cast<u32>(eff);
        }
    }

    float cb_scale_x = 1.0f;
    float cb_scale_y = 1.0f;
    float cb0_z = 0.0f;
    if (vs_kind == VsKind::PassThroughXYCb0Z_W || vs_kind == VsKind::YebisCb20_21ScaleXY) {
        u32 cb_idx = ~0u;
        for (u32 i = 0; i < vs.buffers.size(); ++i) {
            if (!vs.buffers[i].IsSpecial()) {
                if (cb_idx != ~0u) {
                    return 0;
                }
                cb_idx = i;
            }
        }
        if (cb_idx == ~0u) {
            return 0;
        }
        const u64 needed_cb_bytes = (vs_kind == VsKind::YebisCb20_21ScaleXY) ? 88ULL : 4ULL;
        const u8* cb_ptr = nullptr;
        if (const auto* ring = FindRingBinding(vs, cb_idx)) {
            if (ring->size >= needed_cb_bytes) {
                if (ring->address != 0 &&
                    !buffer_cache.IsConstRamRange(ring->address, needed_cb_bytes) &&
                    memory->ClampRangeSize(ring->address, needed_cb_bytes) == needed_cb_bytes) {
                    cb_ptr = reinterpret_cast<const u8*>(ring->address);
                } else {
                    cb_ptr = (ring->ring == 0) ? constant_ring->Data(ring->offset)
                                               : worker_rings[ring->ring - 1]->Data(ring->offset);
                }
            }
        } else {
            const PreparedStage* prep_vs = nullptr;
            if (draw_prepared && !BbToggle::Disabled(BbToggle::PreparedResources)) {
                for (u32 i = 0; i < draw_prepared->num_stages; ++i) {
                    const auto& cand = draw_prepared->stages[i];
                    if (&cand.program->info == &vs && cand.num_buffers == vs.buffers.size()) {
                        prep_vs = &cand;
                        break;
                    }
                }
            }
            const Gcn::Buffer cb_sharp =
                prep_vs ? prep_vs->buffer_sharps[cb_idx] : vs.buffers[cb_idx].GetSharp(vs);
            if (!cb_sharp.Valid() || cb_sharp.GetSize() < needed_cb_bytes) {
                return 0;
            }
            cb_ptr = resolve_cpu_ptr(cb_sharp.base_address, needed_cb_bytes);
        }
        if (!cb_ptr) {
            return 0;
        }
        if (vs_kind == VsKind::YebisCb20_21ScaleXY) {
            std::memcpy(&cb_scale_x, cb_ptr + 20 * sizeof(u32), sizeof(float));
            std::memcpy(&cb_scale_y, cb_ptr + 21 * sizeof(u32), sizeof(float));
        } else {
            std::memcpy(&cb0_z, cb_ptr, sizeof(float));
        }
    }

    Gcn::Buffer vb_attr0{};
    if (vs_kind != VsKind::Procedural89cda9e4) {
        if (!fetch) {
            return 0;
        }
        bool found_attr0 = false;
        for (u32 i = 0; i < fetch->attributes.size(); ++i) {
            const auto& attrib = fetch->attributes[i];
            if (attrib.semantic != 0) {
                continue;
            }
            if (attrib.GetStepRate() != Shader::Gcn::VertexAttribute::InstanceIdType::None) {
                return 0;
            }
            if (draw_prepared && draw_prepared->vertex.valid && i < draw_prepared->vertex.count &&
                !BbToggle::Disabled(BbToggle::PreparedResources)) {
                vb_attr0 = draw_prepared->vertex.buffers[i];
            } else if (packet_vsharps.size() == fetch->attributes.size()) {
                vb_attr0 = packet_vsharps[i];
            } else {
                vb_attr0 = attrib.GetSharp(vs);
            }
            found_attr0 = true;
            break;
        }
        if (!found_attr0 || !vb_attr0.Valid() || vb_attr0.base_address == 0) {
            return 0;
        }
    }

    std::array<WinPt, 6> win_pts{};
    const u64 vb_size = (vs_kind != VsKind::Procedural89cda9e4) ? vb_attr0.GetSize() : 0ULL;
    const u64 vb_stride = (vs_kind != VsKind::Procedural89cda9e4) ? vb_attr0.GetStride() : 0ULL;
    for (u32 i = 0; i < count; ++i) {
        float px = 0.0f, py = 0.0f, pz = 0.0f, pw = 1.0f;
        const u32 vid = vtx_indices[i];
        if (vs_kind == VsKind::Procedural89cda9e4) {
            px = float(static_cast<s32>(((vid & 1u) << 1) - 1u));
            py = float(static_cast<s32>((vid & ~1u) - 1u));
            pz = 0.0f;
            pw = 1.0f;
        } else {
            if (vb_attr0.num_records != 0 && vb_stride > 0 && vid >= vb_attr0.num_records) {
                return 0;
            }
            const u64 elem_off = u64(vid) * vb_stride;
            if (elem_off >= vb_size) {
                return 0;
            }
            const u64 avail = std::min<u64>(vb_size - elem_off, 16ULL);
            const u8* elem_ptr = resolve_cpu_ptr(vb_attr0.base_address + elem_off, avail);
            if (!elem_ptr) {
                return 0;
            }
            std::array<float, 4> attr0{};
            if (!DecodeFloatVertexAttr(vb_attr0, elem_ptr, avail, attr0)) {
                return 0;
            }
            switch (vs_kind) {
            case VsKind::PassThroughXYZW:
                px = attr0[0];
                py = attr0[1];
                pz = attr0[2];
                pw = attr0[3];
                break;
            case VsKind::PassThroughXYZ1:
                px = attr0[0];
                py = attr0[1];
                pz = attr0[2];
                pw = 1.0f;
                break;
            case VsKind::PassThroughXYCb0Z_W:
                px = attr0[0];
                py = attr0[1];
                pz = cb0_z;
                pw = attr0[3];
                break;
            case VsKind::YebisCb20_21ScaleXY:
                px = cb_scale_x * attr0[0];
                py = cb_scale_y * attr0[1];
                pz = 0.1f;
                pw = 1.0f;
                break;
            default:
                return 0;
            }
        }
        if (!std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz) || !std::isfinite(pw) ||
            pw <= 0.0f) {
            return 0;
        }
        if (!clip_disabled) {
            if (key.depth_clip_enable) {
                const float z_min =
                    (key.clip_space == Gcn::ClipSpace::MinusWToW) ? -pw - 1e-4f : -1e-4f;
                if (pz < z_min || pz > pw + 1e-4f) {
                    return 0;
                }
            }
            const float nx = px / pw;
            const float ny = py / pw;
            win_pts[i].x = (vp.x + 0.5f * vp.width) + (0.5f * vp.width) * nx;
            win_pts[i].y = (vp.y + 0.5f * vp.height) + (0.5f * vp.height) * ny;
        } else {
            if (std::abs(pw - 1.0f) > 1e-4f) {
                return 0;
            }
            win_pts[i].x = px * push_data.xscale + push_data.xoffset;
            win_pts[i].y = py * push_data.yscale + push_data.yoffset;
        }
    }

    const auto prim = regs.primitive_type;
    if (prim == Gcn::PrimitiveType::RectList) {
        // GCN RectList: 3 corners of an axis-aligned screen rectangle (never face-culled).
        const WinPt p0 = win_pts[0], p1 = win_pts[1], p2 = win_pts[2];
        const bool ortho0 = (std::abs(p1.x - p0.x) <= 2e-2f && std::abs(p2.y - p0.y) <= 2e-2f) ||
                            (std::abs(p1.y - p0.y) <= 2e-2f && std::abs(p2.x - p0.x) <= 2e-2f);
        const bool ortho1 = (std::abs(p0.x - p1.x) <= 2e-2f && std::abs(p2.y - p1.y) <= 2e-2f) ||
                            (std::abs(p0.y - p1.y) <= 2e-2f && std::abs(p2.x - p1.x) <= 2e-2f);
        const bool ortho2 = (std::abs(p0.x - p2.x) <= 2e-2f && std::abs(p1.y - p2.y) <= 2e-2f) ||
                            (std::abs(p0.y - p2.y) <= 2e-2f && std::abs(p1.x - p2.x) <= 2e-2f);
        if (!ortho0 && !ortho1 && !ortho2) {
            return 0;
        }
        const float rx0 = std::min({p0.x, p1.x, p2.x});
        const float rx1 = std::max({p0.x, p1.x, p2.x});
        const float ry0 = std::min({p0.y, p1.y, p2.y});
        const float ry1 = std::max({p0.y, p1.y, p2.y});
        return (rx0 <= 2e-2f && rx1 >= w_f - 2e-2f && ry0 <= 2e-2f && ry1 >= h_f - 2e-2f)
                   ? candidate_mask
                   : 0;
    }

    std::array<WinTri, 2> tris{};
    u32 num_tris = 0;
    const vk::CullModeFlags cull_mode =
        GcnToVk::IsPrimitiveCulled(prim) ? GcnToVk::CullMode(regs.polygon_control.CullingMode())
                                         : vk::CullModeFlagBits::eNone;
    const vk::FrontFace front_face = GcnToVk::FrontFace(regs.polygon_control.front_face);
    if (cull_mode == vk::CullModeFlagBits::eFrontAndBack) {
        return 0;
    }
    const auto push_tri = [&](WinPt a, WinPt b, WinPt c) {
        const float det = Cross2D(a, b, c);
        if (std::abs(det) <= 1e-2f) {
            return;
        }
        if (cull_mode != vk::CullModeFlagBits::eNone) {
            const bool is_front =
                (front_face == vk::FrontFace::eCounterClockwise) ? (det < 0.0f) : (det > 0.0f);
            if ((cull_mode == vk::CullModeFlagBits::eFront && is_front) ||
                (cull_mode == vk::CullModeFlagBits::eBack && !is_front)) {
                return;
            }
        }
        if (num_tris < tris.size()) {
            tris[num_tris++] = WinTri{{a, b, c}, det};
        }
    };

    switch (prim) {
    case Gcn::PrimitiveType::TriangleList:
        if (count == 3) {
            push_tri(win_pts[0], win_pts[1], win_pts[2]);
        } else if (count == 6) {
            push_tri(win_pts[0], win_pts[1], win_pts[2]);
            push_tri(win_pts[3], win_pts[4], win_pts[5]);
        } else {
            return 0;
        }
        break;
    case Gcn::PrimitiveType::TriangleStrip:
        if (count == 3) {
            push_tri(win_pts[0], win_pts[1], win_pts[2]);
        } else if (count == 4) {
            push_tri(win_pts[0], win_pts[1], win_pts[2]);
            push_tri(win_pts[2], win_pts[1], win_pts[3]);
        } else {
            return 0;
        }
        break;
    case Gcn::PrimitiveType::TriangleFan:
    case Gcn::PrimitiveType::QuadList:
        if (count == 4) {
            push_tri(win_pts[0], win_pts[1], win_pts[2]);
            push_tri(win_pts[0], win_pts[2], win_pts[3]);
        } else {
            return 0;
        }
        break;
    default:
        return 0;
    }

    bool covered = false;
    if (num_tris == 1) {
        covered = TriangleCoversRect(tris[0], w_f, h_f);
    } else if (num_tris == 2) {
        covered = TwoTrianglesCoverRect(tris[0], tris[1], w_f, h_f);
    }
    return covered ? candidate_mask : 0;
}

void Rasterizer::BeginDraw(const GraphicsPipeline* pipeline, const RenderState& state,
                           bool is_indexed, bool jitter_eligible,
                           const PreparedDraw* draw_prepared, u32 index_offset,
                           bool allow_dont_care) {
    // The upscaler's sub-pixel jitter, for geometry drawn with the scene depth.
    draw_jitter = {};
    if (jitter_eligible && upscaler->Enabled() && db_desc.first &&
        db_desc.first == camera_motion->Depth()) {
        draw_jitter = upscaler->Jitter();
    }
    // A new render pass sets its viewports and scissors again, even when the state tracker has
    // them already: single frames were drawn with the 960x540 viewport of the passes before (the
    // final picture in the top-left quarter, black around it; docs/Flicker_video_2026-10-17.ru.md).
    // BB_DYNAMIC_STATE_ALWAYS=1 (diagnostics): all dynamic state again at every draw.
    static const bool dynamic_always = [] {
        const char* env = std::getenv("BB_DYNAMIC_STATE_ALWAYS");
        return env && env[0] == '1';
    }();
    if (dynamic_always) {
        scheduler.GetDynamicState().Invalidate();
    } else if (scheduler.WillBeginRendering(state)) {
        scheduler.GetDynamicState().InvalidateViewports();
    }
    UpdateDynamicState(pipeline, is_indexed);
    // BB_DISPLAY_WATCH=1 (diagnostics): every draw into a display buffer, with its sizes and state.
    static const bool display_watch = std::getenv("BB_DISPLAY_WATCH") != nullptr;
    // BB_DISPLAY_WATCH_TARGET=addr: also every draw into that color target.
    static const VAddr watch_target = [] {
        const char* env = std::getenv("BB_DISPLAY_WATCH_TARGET");
        return env ? VAddr(std::strtoull(env, nullptr, 16)) : VAddr{0};
    }();
    if (display_watch && Regs().color_buffers[0] &&
        (FrameCapture::IsDisplayBuffer(Regs().color_buffers[0].Address()) ||
         Regs().color_buffers[0].Address() == watch_target)) {
        const auto& rvp = Regs().viewports[0];
        std::printf("Display watch regs: vp0 %g %g %g %g vte %#x jitter %g,%g new pass %d\n", rvp.xscale,
                    rvp.xoffset, rvp.yscale, rvp.yoffset, std::bit_cast<u32>(Regs().viewport_control),
                    draw_jitter[0], draw_jitter[1], int(scheduler.WillBeginRendering(state)));
        const auto& ds = scheduler.GetDynamicState();
        const auto& vps = ds.CurrentViewports();
        const auto& scs = ds.CurrentScissors();
        const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
        const auto& image = texture_cache.GetImage(cb_descs[0].first).info;
        std::printf("Display watch: gpu frame %llu flips %llu vs %08x ps %08x pass %ux%u image %ux%u scale %g,%g "
                    "vp %g,%g %gx%g sc %d,%d %ux%u clipoff %d push %g,%g,%g,%g\n",
                    (unsigned long long)BbStats::gpu_frames.load(std::memory_order_relaxed),
                    (unsigned long long)FrameCapture::Flips(),
                    u32(pipeline->GetStage(Shader::SwStage::Vertex).pgm_hash), u32(ps ? ps->pgm_hash : 0),
                    state.width, state.height, image.size.width, image.size.height, target_scale[0],
                    target_scale[1], vps.empty() ? 0.f : vps[0].x, vps.empty() ? 0.f : vps[0].y,
                    vps.empty() ? 0.f : vps[0].width, vps.empty() ? 0.f : vps[0].height,
                    scs.empty() ? 0 : scs[0].offset.x, scs.empty() ? 0 : scs[0].offset.y,
                    scs.empty() ? 0u : scs[0].extent.width, scs.empty() ? 0u : scs[0].extent.height,
                    int(Regs().IsClipDisabled()), push_data.xscale, push_data.xoffset,
                    push_data.yscale, push_data.yoffset);
    }
    MarkPass(pipeline, state);
    if (occlusion) {
        occlusion->SetPassScale(target_scale[0] * target_scale[1]); // samples at the game's size
    }
    const u8 color_dont_care_mask =
        (allow_dont_care && scheduler.WillBeginRendering(state))
            ? ComputeFullscreenOverwriteMask(pipeline, state, draw_prepared, is_indexed,
                                             index_offset)
            : 0;
    scheduler.BeginRendering(state, color_dont_care_mask);
    // Push constants after the pass begins: beginning it may record the occlusion results' compute
    // dispatch (OcclusionQueries::Suspend), and the driver keeps one set of push constants for
    // compute and graphics. Its 8 bytes over xoffset/yoffset drew single frames shifted by half the
    // screen (docs/Flicker_2026-10-18.ru.md).
    {
        BB_SECTION(PipelineBind);
        pipeline->BindResources(set_writes, push_data, {image_infos.data(), image_infos.size()},
                                {buffer_infos.data(), buffer_infos.size()}, true);
    }
}

static u64 CopyRangeMask(VAddr begin, VAddr end) {
    constexpr u64 Shift = 16;
    const u64 first = begin >> Shift;
    const u64 last = (end - 1) >> Shift;
    if (last - first >= 63) {
        return ~0ull;
    }
    u64 mask = 0;
    for (u64 p = first; p <= last; ++p) {
        mask |= 1ull << ((p * 0x9E3779B97F4A7C15ull) >> 58);
    }
    return mask;
}

void Rasterizer::NoteCopyIntoMemory(VAddr address, u64 size) {
    if (size == 0 || !OnStageA()) {
        return;
    }
    copies_into_memory[num_copies_into_memory++ % copies_into_memory.size()] = {address,
                                                                               address + size};
    if (num_copies_into_memory % copies_into_memory.size() == 0) {
        // Once around the ring: the bounds of the copies kept (the ones pushed out no longer).
        copies_begin = ~VAddr{0};
        copies_end = 0;
        copies_mask = 0;
        for (const auto& copy : copies_into_memory) {
            copies_begin = std::min(copies_begin, copy.begin);
            copies_end = std::max(copies_end, copy.end);
            copies_mask |= CopyRangeMask(copy.begin, copy.end);
        }
        return;
    }
    copies_begin = std::min(copies_begin, address);
    copies_end = std::max(copies_end, address + size);
    copies_mask |= CopyRangeMask(address, address + size);
}

// Program code the GPU copies into place (DMA in the stream before the draw): this thread decodes
// the draw ahead of the copy and read the code that was there before (another program: a
// character's vertices stretched into streaks in another game; or zeros). Code under a copy noted
// since the GPU last ran everything: everything before it runs first. Rare (shader uploads); the
// last 64 copies are kept (a game copies far more a frame: a wait for each program check after
// the ring filled cost a quarter of this thread).
void Rasterizer::SyncProgramWrites(VAddr address, u64 size) {
    if (address + size <= copies_begin || address >= copies_end ||
        (copies_mask & CopyRangeMask(address, address + size)) == 0 || !OnStageA()) {
        return;
    }
    bool overlaps = false;
    const u32 kept = std::min<u32>(num_copies_into_memory, u32(copies_into_memory.size()));
    for (u32 i = 0; !overlaps && i < kept; ++i) {
        const auto& copy = copies_into_memory[i];
        overlaps = copy.begin < address + size && address < copy.end;
    }
    if (!overlaps) {
        return;
    }
    DrainDrawPipe();
    Finish();
    num_copies_into_memory = 0;
    copies_begin = ~VAddr{0};
    copies_end = 0;
    copies_mask = 0;
}

void Rasterizer::DrawIndirect(bool is_indexed, VAddr arg_address, u32 offset, u32 stride,
                              u32 max_count, VAddr count_address, u16 vertex_sgpr_offset,
                              u16 instance_sgpr_offset) {
    // bbport: like direct draws, handed to the recording thread after the pipeline selection
    // (else the GPU thread waited here for a whole frame of queued draws).
    const bool pipelined = UseDrawPipe() && FilterDrawPasses() &&
                           !BbToggle::Disabled(BbToggle::PipelinedIndirectDraws);
    if (!pipelined) {
        DrainDrawPipe();
        scheduler.PopPendingOperations();
        if (!FilterDraw()) {
            return;
        }
    }

    const DrawIndirectParams params = {
        .vertex_sgpr_offset = vertex_sgpr_offset,
        .instance_sgpr_offset = instance_sgpr_offset,
    };
    const GraphicsPipeline* pipeline = pipeline_cache.GetGraphicsPipeline(params, nullptr, true);
    if (!pipeline) {
        return;
    }
    const IndirectDraw indirect = {arg_address + offset, count_address, stride, max_count};
    if (pipelined) {
        PostDraw(pipeline, nullptr, is_indexed, 0, nullptr, &indirect);
        return;
    }
    DrawIndirectRecord(pipeline, is_indexed, indirect);
}

void Rasterizer::DrawIndirectRecord(const GraphicsPipeline* pipeline, bool is_indexed,
                                    const IndirectDraw& indirect) {
    buffer_cache.NewPacket();
    if (DrawPipe::OnStageB()) {
        FrameCapture::Poll();
        scheduler.PopPendingOperations();
    }
    const VAddr count_address = indirect.count;
    const u32 stride = indirect.stride;
    const u32 max_count = indirect.max_count;

    // Indirect arguments may be GPU-written: leave these draws on camera fallback.
    motion_draw = false;
    motion_geometry = 0;
    {
        const auto& regs = Regs();
        const auto& key = pipeline->GetGraphicsKey();
        bool has_color = false;
        if (regs.color_control.mode != Gcn::ColorControl::OperationMode::Disable) {
            for (s32 cb = 0; cb < std::bit_width(key.mrt_mask); ++cb) {
                if (regs.color_buffers[cb] && regs.color_target_mask.GetMask(cb) &&
                    (key.mrt_mask & (1 << cb)) != 0) {
                    has_color = true;
                    break;
                }
            }
        }
        const bool has_depth_stencil =
            (regs.depth_control.depth_enable && regs.depth_buffer.DepthValid()) ||
            (regs.depth_control.stencil_enable && regs.depth_buffer.StencilValid());
        if (!has_color && !has_depth_stencil &&
            pipeline->GetStages()[u32(Shader::SwStage::Fragment)] == nullptr) {
            const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
            if (vs.pgm_hash == 0x89cda9e4ull && !vs.uses_dma &&
                !std::ranges::any_of(vs.buffers, [](const auto& b) { return b.is_written; }) &&
                !std::ranges::any_of(vs.images, [](const auto& i) { return i.is_written; })) {
                return;
            }
        }
    }
    PrepareRenderState(pipeline);
    NoteUpscalerDraw(pipeline);
    if (!BindResources(pipeline)) {
        return;
    }
    if (HasPendingHleCopies() && PendingHleCopiesConflict(indirect.args, stride * max_count)) {
        FlushPendingHleCopies(*this);
    }
    if (direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }
    const VideoCore::Buffer* buffer = nullptr;
    u64 base = 0;
    bool is_native_indirect_buf = false;
    {
        u64 req_bytes = static_cast<u64>(stride) * max_count;
        std::tie(buffer, base) = direct_buffers.TryResolveReadClamped(indirect.args, req_bytes);
        if (buffer && req_bytes >= static_cast<u64>(stride) * max_count) {
            is_native_indirect_buf = true;
        } else {
            std::tie(buffer, base) =
                buffer_cache.ObtainBuffer(indirect.args, stride * max_count, false);
        }
    }
    needs_barrier |= runtime.IsBufferAccessed(buffer, base, stride * max_count);
    static const bool force_indirect_guard = [] {
        const char* env = std::getenv("BB_FORCE_INDIRECT_GUARD");
        return env && env[0] == '1';
    }();
    // The arguments checked on the GPU first (vk_indirect_guard.h): a dispatch, outside the
    // render pass. Native DirectBuffers are zero-initialized in VRAM and never carry garbage RAM.
    vk::Buffer checked_args = buffer->Handle();
    u64 checked_offset = base;
    if ((!is_native_indirect_buf || force_indirect_guard) && IndirectGuard::DrawEnabled()) {
        if (!indirect_guard) {
            indirect_guard = std::make_unique<IndirectGuard>(instance, scheduler);
        }
        if (needs_barrier) {
            runtime.FlushBarriers();
            needs_barrier = false;
        }
        scheduler.EndRendering();
        std::tie(checked_args, checked_offset) =
            indirect_guard->CheckDrawArgs(*buffer, base, stride, max_count, is_indexed);
    }
    const auto state = BeginRendering(pipeline);

    BindVertexBuffers(pipeline);
    if (is_indexed) {
        BindIndexBuffer();
    }

    const VideoCore::Buffer* count_buffer = nullptr;
    u64 count_offset = 0;
    if (count_address != 0) {
        if (HasPendingHleCopies() && PendingHleCopiesConflict(count_address, 4)) {
            FlushPendingHleCopies(*this);
        }
        u64 cnt_bytes = 4;
        std::tie(count_buffer, count_offset) =
            direct_buffers.TryResolveReadClamped(count_address, cnt_bytes);
        if (!count_buffer || cnt_bytes < 4) {
            std::tie(count_buffer, count_offset) =
                buffer_cache.ObtainBuffer(count_address, 4, false);
        }
        needs_barrier |= runtime.IsBufferAccessed(count_buffer, count_offset, 4);
    }
    if (HasPendingHleCopies() && scheduler.WillBeginRendering(state)) {
        FlushPendingHleCopies(*this);
    }

    if (needs_barrier || (scheduler.WillBeginRendering(state) && runtime.HasPendingBarriers())) {
        runtime.FlushBarriers();
    }

    ScaleToTarget(pipeline);
    BeginDraw(pipeline, state, is_indexed, true);

    ASSERT(stride == (is_indexed ? sizeof(VkDrawIndexedIndirectCommand)
                                 : sizeof(VkDrawIndirectCommand)));
    const vk::Pipeline handle = pipeline->Handle();
    bound_graphics_pipeline = handle;
    bound_graphics_epoch = scheduler.DescriptorStateEpoch();
    const vk::Buffer args = checked_args;
    const u64 args_offset = checked_offset;
    const vk::Buffer counts = count_address != 0 ? count_buffer->Handle() : vk::Buffer{};
    const u64 counts_offset = count_address != 0 ? count_offset : 0;
    if (!Breadcrumbs::Enabled()) [[likely]] {
        scheduler.Record([handle, is_indexed, counts, args, args_offset, counts_offset, max_count,
                          stride](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, handle);
            if (is_indexed) {
                if (counts) {
                    cmdbuf.drawIndexedIndirectCount(args, args_offset, counts, counts_offset,
                                                    max_count, stride);
                } else {
                    cmdbuf.drawIndexedIndirect(args, args_offset, max_count, stride);
                }
            } else if (counts) {
                cmdbuf.drawIndirectCount(args, args_offset, counts, counts_offset, max_count,
                                         stride);
            } else {
                cmdbuf.drawIndirect(args, args_offset, max_count, stride);
            }
        });
    } else {
        const auto* vs_crumb = pipeline->GetStages()[u32(Shader::SwStage::Vertex)];
        const auto* ps_crumb = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
        const Breadcrumbs::Crumb crumb{
            .kind = Breadcrumbs::Kind::DrawIndirect,
            .hash = {vs_crumb ? vs_crumb->pgm_hash : 0, ps_crumb ? ps_crumb->pgm_hash : 0},
            .program = {vs_crumb ? vs_crumb->ProgramBase() : 0,
                        ps_crumb ? ps_crumb->ProgramBase() : 0},
            .address = indirect.args,
            .count = {max_count, stride, 0},
        };
        scheduler.RecordCrumb(crumb, [=](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, handle);
            if (is_indexed) {
                if (counts) {
                    cmdbuf.drawIndexedIndirectCount(args, args_offset, counts, counts_offset,
                                                    max_count, stride);
                } else {
                    cmdbuf.drawIndexedIndirect(args, args_offset, max_count, stride);
                }
            } else if (counts) {
                cmdbuf.drawIndirectCount(args, args_offset, counts, counts_offset, max_count,
                                         stride);
            } else {
                cmdbuf.drawIndirect(args, args_offset, max_count, stride);
            }
        });
    }

    ResetBindings(false);
    runtime.AccessBuffer(buffer, base, static_cast<u64>(stride) * max_count,
                         vk::PipelineStageFlagBits2::eDrawIndirect,
                         vk::AccessFlagBits2::eIndirectCommandRead);
    if (count_buffer) {
        runtime.AccessBuffer(count_buffer, count_offset, 4,
                             vk::PipelineStageFlagBits2::eDrawIndirect,
                             vk::AccessFlagBits2::eIndirectCommandRead);
    }
    scheduler.KickRecording();
}

void Rasterizer::DispatchDirect() {
    BbStats::dispatches.fetch_add(1, std::memory_order_relaxed);
    // bbport: like draws, handed to the draw recording thread after the pipeline selection.
    const bool pipelined = UseDrawPipe() && !BbToggle::Disabled(BbToggle::PipelinedDispatch);
    if (!pipelined) {
        DrainDrawPipe();
    }
    const ComputePipeline* pipeline = pipeline_cache.GetComputePipeline();
    if (!pipeline) {
        return;
    }
    // BB_DISPATCH_START_LOG=1 (diagnostics, the flicker): dispatches with a nonzero thread
    // group start (COMPUTE_START_X/Y/Z), which the translation does not apply.
    if (static const bool start_log = std::getenv("BB_DISPATCH_START_LOG") != nullptr; start_log) {
        const auto& cs = CsRegs();
        static std::atomic<u32> printed{0};
        if ((cs.start_x | cs.start_y | cs.start_z) != 0 && printed.fetch_add(1) < 5000) {
            std::printf("Dispatch start: gpu frame %llu cs %016llx start %u,%u,%u dims %u,%u,%u\n",
                        (unsigned long long)BbStats::gpu_frames.load(std::memory_order_relaxed),
                        (unsigned long long)pipeline->GetStage(Shader::SwStage::Compute).pgm_hash,
                        cs.start_x, cs.start_y, cs.start_z, cs.dim_x, cs.dim_y, cs.dim_z);
        }
    }
    if (pipelined) {
        PostDraw(pipeline, nullptr, false, 0, &CsRegs());
        return;
    }
    DispatchRecord(pipeline);
}

void Rasterizer::DispatchRecord(const ComputePipeline* pipeline) {
    BB_SECTION(DispatchRecord);
    buffer_cache.NewPacket();
    FrameCapture::Poll();
    gbuffer_draw = false;

    scheduler.PopPendingOperations();

    const auto& cs_program = CsRegs();

    const auto& cs = pipeline->GetStage(Shader::SwStage::Compute);
    // After the resource binding: its transfers (image uploads, downloads) mark themselves.
    const auto mark = [&] {
        if (auto* profiler = GpuProfiler::Get()) {
            profiler->Mark(cs.pgm_hash ^ 0xD15Aull, [&] {
                return fmt::format("dispatch cs {:016x} ({}x{}x{} groups)", cs.pgm_hash,
                                   cs_program.dim_x, cs_program.dim_y, cs_program.dim_z);
            });
        }
    };
    if (upscaler->Enabled()) {
        upscaler->OnDispatch(cs.pgm_hash);
    }
    bool hle;
    {
        BB_SECTION(ShaderHle);
        hle = ExecuteShaderHLE(cs, Regs(), cs_program, *this);
    }
    if (hle) {
        return;
    }
    if (HasPendingHleCopies()) {
        FlushPendingHleCopies(*this);
    }
    // bbport: the game's copy shader running as itself (BB_COPY_SHADER_NATIVE): its destinations stay in
    // the game's memory. They hold data our translator reads on the CPU (next to shader code the
    // game uploads this way); a VRAM copy of them showed it a stale copy (a black scene). The
    // copy list read on the CPU (vk_shader_hle.cpp) kept them in place too.
    buffer_cache.force_writes_in_place =
        Game::BufferCopyShader() != 0 && cs.pgm_hash == Game::BufferCopyShader();
    // What it writes, from its control list: GPU-written marks on those bytes only (the bound
    // destination spans tens of MiB, marked whole it kept the constants there in place).
    thread_local std::vector<std::pair<unsigned long long, unsigned long long>> copy_writes;
    thread_local std::vector<std::pair<unsigned long long, unsigned long long>> copy_reads;
    copy_writes.clear();
    copy_reads.clear();
    if (buffer_cache.force_writes_in_place && CopyDestinationsPrecise() &&
        CopyShaderDestinations(cs, cs_program.dim_x, CopyDestinationGap(), copy_writes,
                               &copy_reads)) {
        buffer_cache.precise_writes = &copy_writes;
        buffer_cache.precise_reads = &copy_reads;
    }
    const bool bound = BindResources(pipeline);
    buffer_cache.force_writes_in_place = false;
    buffer_cache.precise_writes = nullptr;
    buffer_cache.precise_reads = nullptr;
    if (!bound) {
        return;
    }

    scheduler.EndRendering();
    if (needs_barrier || runtime.HasPendingComputeBarriers()) {
        runtime.FlushBarriers(false);
    }
    mark();
    // BB_DISPATCH_LOG=1 (diagnostics): the shape of each kind of real compute dispatch, once: the
    // buffers (size, formatted, written), the groups, and what the image clear detector thinks.
    {
        static const bool log = [] {
            const char* env = std::getenv("BB_DISPATCH_LOG");
            return env && env[0] == '1';
        }();
        if (log) {
            static std::mutex mutex;
            static std::set<std::string> told;
            std::string key = fmt::format("{:016x} x{} y{} z{}", cs.pgm_hash, cs_program.dim_x,
                                          cs_program.dim_y, cs_program.dim_z);
            for (const auto& b : cs.buffers) {
                key += fmt::format(" [{}{}{} {}]", b.is_formatted ? "F" : "R", b.is_written ? "W" : "r",
                                   b.is_formatted ? "" : "", b.GetSharp(cs).GetSize());
            }
            if (cs.buffers.size() >= 2 && cs.buffers[1].is_formatted && cs.buffers[1].is_written) {
                const auto target = cs.buffers[1].GetSharp(cs);
                const auto id = texture_cache.FindImageFromRange(target.base_address, target.GetSize(), false);
                if (id) {
                    const auto& image = texture_cache.GetImage(id);
                    key += fmt::format(" image(size {} bits {} depth {} fmt {})", image.info.guest_size,
                                       image.info.num_bits, image.info.props.is_depth ? 1 : 0,
                                       Gcn::NumBitsPerBlock(target.GetDataFmt()));
                } else {
                    key += fmt::format(" NO-IMAGE at {:#x}", target.base_address);
                }
            }
            std::scoped_lock lock{mutex};
            if (told.size() < 400 && told.insert(key).second) {
                std::printf("Dispatch shape: %s\n", key.c_str());
            }
        }
    }
    pipeline->BindResources(set_writes, push_data, {image_infos.data(), image_infos.size()},
                            {buffer_infos.data(), buffer_infos.size()}, true);

    const vk::Pipeline handle = pipeline->Handle();
    const u32 dim_x = cs_program.dim_x, dim_y = cs_program.dim_y, dim_z = cs_program.dim_z;
    if (FrameCapture::Active()) {
        FrameCapture::Dispatch(cs.pgm_hash, dim_x, dim_y, dim_z);
    }
    if (!Breadcrumbs::Enabled()) [[likely]] {
        scheduler.Record([handle, dim_x, dim_y, dim_z](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, handle);
            cmdbuf.dispatch(dim_x, dim_y, dim_z);
        });
    } else {
        const Breadcrumbs::Crumb crumb{
            .kind = Breadcrumbs::Kind::Dispatch,
            .hash = {cs.pgm_hash, 0},
            .program = {cs.ProgramBase(), 0},
            .count = {dim_x, dim_y, dim_z},
        };
        scheduler.RecordCrumb(crumb, [=](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, handle);
            cmdbuf.dispatch(dim_x, dim_y, dim_z);
        });
    }
    // BB_TRACE_SHADER with BB_GPU_PROFILE: a mark right after the traced compute shader's
    // dispatch, so the profile splits its label into the dispatch itself and what follows it
    // (barriers, unlabeled commands) up to the next mark.
    if (auto* profiler = GpuProfiler::Get()) {
        static const char* traced = std::getenv("BB_TRACE_SHADER");
        if (traced && std::strtoull(traced, nullptr, 16) == cs.pgm_hash) {
            profiler->Mark(cs.pgm_hash ^ 0xAF7D'0000ull, [&] {
                return fmt::format("after the dispatch of cs {:016x}", cs.pgm_hash);
            });
        }
    }

    ResetBindings(true);
    scheduler.KickRecording();
}

void Rasterizer::DispatchIndirect(VAddr address, u32 offset, u32 size) {
    // bbport: handed to the recording thread like direct dispatches.
    const bool pipelined = UseDrawPipe() && !BbToggle::Disabled(BbToggle::PipelinedIndirectDraws);
    if (!pipelined) {
        DrainDrawPipe();
    }
    const ComputePipeline* pipeline = pipeline_cache.GetComputePipeline();
    if (!pipeline) {
        return;
    }
    if (pipelined) {
        const IndirectDraw indirect = {address + offset, 0, size, 1};
        PostDraw(pipeline, nullptr, false, 0, &CsRegs(), &indirect);
        return;
    }
    DispatchIndirectRecord(pipeline, address + offset, size);
}

void Rasterizer::DispatchIndirectRecord(const ComputePipeline* pipeline, VAddr address,
                                        u32 size) {
    if (HasPendingHleCopies()) {
        FlushPendingHleCopies(*this);
    }
    buffer_cache.NewPacket();
    scheduler.PopPendingOperations();
    if (!BindResources(pipeline)) {
        return;
    }
    if (direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }
    const VideoCore::Buffer* buffer = nullptr;
    u64 base = 0;
    bool is_native_indirect_buf = false;
    {
        u64 req_bytes = size;
        std::tie(buffer, base) = direct_buffers.TryResolveReadClamped(address, req_bytes);
        if (buffer && req_bytes >= size) {
            is_native_indirect_buf = true;
        } else {
            std::tie(buffer, base) = buffer_cache.ObtainBuffer(address, size, false);
        }
    }
    needs_barrier |= runtime.IsBufferAccessed(buffer, base, size);

    scheduler.EndRendering();
    if (needs_barrier || runtime.HasPendingComputeBarriers()) {
        runtime.FlushBarriers(false);
    }
    const auto& cs_crumb = pipeline->GetStage(Shader::SwStage::Compute);
    if (auto* profiler = GpuProfiler::Get()) {
        profiler->Mark(cs_crumb.pgm_hash ^ 0xD15B0000ull, [&] {
            return fmt::format("dispatch indirect cs {:016x}", cs_crumb.pgm_hash);
        });
    }
    static const bool force_indirect_guard = [] {
        const char* env = std::getenv("BB_FORCE_INDIRECT_GUARD");
        return env && env[0] == '1';
    }();
    // Group counts checked on the GPU first (vk_indirect_guard.h); recorded before the
    // pipeline's bindings, which the check's own pipeline would disturb. Skipped for native
    // zero-initialized DirectBuffers (eliminating 37+ extra compute dispatches and 75+ full-chip
    // barriers per frame).
    vk::Buffer args = buffer->Handle();
    u64 args_offset = base;
    if ((!is_native_indirect_buf || force_indirect_guard) && IndirectGuard::DispatchEnabled()) {
        if (!indirect_guard) {
            indirect_guard = std::make_unique<IndirectGuard>(instance, scheduler);
        }
        std::tie(args, args_offset) = indirect_guard->CheckArgs(*buffer, base);
    }
    pipeline->BindResources(set_writes, push_data, {image_infos.data(), image_infos.size()},
                            {buffer_infos.data(), buffer_infos.size()}, true);

    const vk::Pipeline handle = pipeline->Handle();
    if (!Breadcrumbs::Enabled()) [[likely]] {
        scheduler.Record([handle, args, args_offset](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, handle);
            cmdbuf.dispatchIndirect(args, args_offset);
        });
    } else {
        const vk::Buffer raw_args = buffer->Handle();
        const u64 raw_offset = base;
        const u32 args_slot = Breadcrumbs::ArgsSlot();
        const Breadcrumbs::Crumb crumb{
            .kind = Breadcrumbs::Kind::DispatchIndirect,
            .hash = {cs_crumb.pgm_hash, 0},
            .program = {cs_crumb.ProgramBase(), 0},
            .address = address,
            .count = {buffer_cache.IsInPlace(address, size) ? 1u : 0u, 0, 0},
            .args_slot = args_slot,
        };
        scheduler.RecordCrumb(crumb, [=](vk::CommandBuffer cmdbuf) {
            Breadcrumbs::CopyArgs(cmdbuf, raw_args, raw_offset, args_slot);
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, handle);
            cmdbuf.dispatchIndirect(args, args_offset);
        });
    }

    ResetBindings(true);
    runtime.AccessBuffer(buffer, base, size, vk::PipelineStageFlagBits2::eDrawIndirect,
                         vk::AccessFlagBits2::eIndirectCommandRead);
    scheduler.KickRecording();
}

u64 Rasterizer::Flush() {
    BB_SECTION(Flush);
    DrainDrawPipe();
    if (HasPendingHleCopies()) {
        FlushPendingHleCopies(*this);
    }
    if (buffer_cache.HasPendingInPassShadows()) {
        buffer_cache.FlushInPassShadows();
    }
    direct_buffers.FlushPendingWritebacks();
    if (direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }
    PublishSignals(); // the recording thread's (it is idle here, or this is it)
    last_flush_ns.store(std::chrono::steady_clock::now().time_since_epoch().count(),
                        std::memory_order_relaxed);
    const u64 current_tick = scheduler.CurrentTick();
    SubmitInfo info{};
    scheduler.Flush(info);
    BbTimeline::Note(BbTimeline::VulkanSubmit, current_tick, DrawPipe::OnStageB() ? 1 : 0);
    return current_tick;
}

bool Rasterizer::HasPendingWork() {
    return scheduler.HasPendingWork() || HasPendingHleCopies() || HasUnsubmittedSignals() ||
           direct_buffers.HasPendingWritebacks() || buffer_cache.HasPendingInPassShadows();
}

void Rasterizer::Finish() {
    DrainDrawPipe();
    if (HasPendingHleCopies()) {
        FlushPendingHleCopies(*this);
    }
    if (buffer_cache.HasPendingInPassShadows()) {
        buffer_cache.FlushInPassShadows();
    }
    direct_buffers.FlushPendingWritebacks();
    PublishSignals();
    scheduler.Finish();
}

void Rasterizer::ProcessDirectTextureEvents() {
    texture_cache.ProcessDirectTextureEvents(memory);
    direct_buffers.ProcessEvents(memory);
}

void Rasterizer::OnSubmit() {
    DrainDrawPipe();
    buffer_cache.NoteTransientSubmitBoundary();
    if (buffer_cache.HasPendingInPassShadows()) {
        buffer_cache.FlushInPassShadows();
    }
    direct_buffers.FlushPendingWritebacks();
    ProcessDirectTextureEvents();
    if (fault_process_pending) {
        fault_process_pending = false;
        buffer_cache.ProcessFaultBuffer();
    }
    texture_cache.ProcessDownloadImages();
    texture_cache.RunGarbageCollector();
    runtime.TickFrame();
}

void Rasterizer::MarkPass(const GraphicsPipeline* pipeline, const RenderState& state) {
    auto* profiler = GpuProfiler::Get();
    if (!profiler) {
        return;
    }
    const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
    const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
    if (profiler->Records(&scheduler)) {
        const u64 idx_inst =
            u64(Regs().num_indices) * u64(std::max<u32>(1u, Regs().num_instances.NumInstances()));
        profiler->NoteDraw(vs.pgm_hash, ps ? ps->pgm_hash : 0, state.width, state.height,
                           state.num_color_attachments, idx_inst);
    }
    if (!scheduler.WillBeginRendering(state)) {
        return;
    }
    std::array<u64, Gcn::NUM_COLOR_BUFFERS + 6> parts{};
    parts[0] = state.width | u64(state.height) << 32;
    parts[1] = vs.pgm_hash;
    parts[2] = ps ? ps->pgm_hash : 0;
    parts[3] = db_desc.first ? u64(texture_cache.GetImage(db_desc.first).info.pixel_format) : 0;
    for (u32 cb = 0; cb < state.num_color_attachments && cb < Gcn::NUM_COLOR_BUFFERS; ++cb) {
        parts[4 + cb] = cb_descs[cb].first
                            ? u64(texture_cache.GetImage(cb_descs[cb].first).info.pixel_format) + 1
                            : 0;
    }
    const u64 key = XXH3_64bits(parts.data(), sizeof(parts));
    profiler->Mark(key, [&] {
        std::string targets;
        for (u32 cb = 0; cb < state.num_color_attachments; ++cb) {
            targets += cb_descs[cb].first
                           ? vk::to_string(texture_cache.GetImage(cb_descs[cb].first).info.pixel_format)
                           : std::string{"-"};
            targets += ' ';
        }
        return fmt::format("pass {}x{} [{}] depth {} vs {:016x} ps {:016x}", state.width,
                           state.height, targets,
                           db_desc.first ? vk::to_string(
                                               texture_cache.GetImage(db_desc.first).info.pixel_format)
                                         : std::string{"-"},
                           vs.pgm_hash, ps ? ps->pgm_hash : 0);
    });
}

void Rasterizer::NoteFrameStart() {
    tls_draws_in_frame = 0;
    tls_mid_frame_flushed = false;
    InvalidateGdsMirror();
    if (HasPendingHleCopies()) {
        FlushPendingHleCopies(*this);
    }
    GpuProfiler::OnFrame();
    const u64 frame = BbStats::gpu_frames.fetch_add(1, std::memory_order_relaxed) + 1;
    // BB_BUFFER_STATS=1: how many earlier frames the GPU has not finished when the GPU thread
    // starts a frame — the lag that decides whether guest memory can be read in place.
    static const bool stats = [] {
        const char* value = std::getenv("BB_BUFFER_STATS");
        return value && value[0] == '1';
    }();
    if (!stats) {
        return;
    }
    static std::array<u64, 16> frame_ticks{};
    static std::array<u64, 17> lag_histogram{};
    u32 lag = 0;
    for (u32 back = 1; back < frame_ticks.size() && back < frame; ++back) {
        const u64 tick = frame_ticks[(frame - back) % frame_ticks.size()];
        if (tick && !scheduler.IsFree(tick)) {
            lag = back;
        }
    }
    ++lag_histogram[lag];
    frame_ticks[frame % frame_ticks.size()] = scheduler.CurrentTick();
    if (frame % 600 == 0) {
        std::printf("GPU lag at frame start (frames not finished: count):");
        for (u32 i = 0; i < lag_histogram.size(); ++i) {
            if (lag_histogram[i]) {
                std::printf(" %u:%llu", i, static_cast<unsigned long long>(lag_histogram[i]));
            }
        }
        std::printf("\n");
        lag_histogram = {};
    }
}

} // namespace Vulkan

namespace Vulkan {

void Rasterizer::WatchDraw(const GraphicsPipeline* pipeline, const RenderState& state) {
    // GPU thread only. Per (vs, ps, first target): the last fields seen and how many draws in a
    // row had them. A change after a stable run is printed with the frame number, and so is the
    // change back, which marks a one-frame anomaly (docs/Flicker_video_2026-10-17.ru.md).
    const auto& regs = Regs();
    const auto& vs = pipeline->GetStage(Shader::SwStage::Vertex);
    const auto* ps = pipeline->GetStages()[u32(Shader::SwStage::Fragment)];
    const VAddr target = cb_descs[0].first ? texture_cache.GetImage(cb_descs[0].first).info.guest_address : 0;
    const u64 key = vs.pgm_hash * 0x9E3779B97F4A7C15ull ^ (ps ? ps->pgm_hash : 0) ^ target;
    const auto bits = [](float f) { return u64(std::bit_cast<u32>(f)); };
    std::vector<u64> fields;
    const auto& vp = regs.viewports[0];
    fields.push_back(bits(vp.xscale) << 32 | bits(vp.xoffset));
    fields.push_back(bits(vp.yscale) << 32 | bits(vp.yoffset));
    fields.push_back(u64(std::bit_cast<u32>(regs.viewport_control)) << 32 | u64(regs.IsClipDisabled()));
    const auto& sc = regs.screen_scissor;
    fields.push_back(u64(u16(sc.top_left_x)) << 48 | u64(u16(sc.top_left_y)) << 32 |
                     u64(u16(sc.bottom_right_x)) << 16 | u16(sc.bottom_right_y));
    fields.push_back(bits(target_scale[0]) << 32 | bits(target_scale[1]));
    fields.push_back(bits(push_data.xoffset) << 32 | bits(push_data.yoffset));
    fields.push_back(u64(state.width) << 32 | state.height);
    static const bool textures = std::getenv("BB_DRAW_WATCH_TEXTURES") != nullptr;
    for (const auto id : textures ? bound_images : decltype(bound_images){}) {
        const auto& info = texture_cache.GetImage(id).info;
        fields.push_back(info.guest_address);
        fields.push_back(u64(info.size.width) << 32 | info.size.height);
    }
    struct Seen {
        std::vector<u64> fields;
        std::vector<u64> stable_fields;
        u32 run = 0;
        u64 changed_frame = 0;
    };
    static std::unordered_map<u64, Seen> seen;
    static u32 printed = 0;
    auto& s = seen[key];
    const u64 frame = FrameCapture::Flips();
    if (s.fields == fields) {
        ++s.run;
        return;
    }
    const auto print = [&](const char* what) {
        if (printed++ > 4000) {
            return;
        }
        std::string text = fmt::format("Draw watch: frame {} {} vs {:016x} ps {:016x} target {:#x}:",
                                       frame, what, vs.pgm_hash, ps ? ps->pgm_hash : 0, target);
        const size_t n = std::max(fields.size(), s.fields.size());
        for (size_t i = 0; i < n; ++i) {
            const u64 now = i < fields.size() ? fields[i] : 0;
            const u64 was = i < s.fields.size() ? s.fields[i] : 0;
            if (now != was) {
                text += fmt::format(" [{}] {:#x}->{:#x}", i, was, now);
            }
        }
        std::printf("%s\n", text.c_str());
    };
    if (s.run >= 8) {
        // A stable run broke.
        s.stable_fields = s.fields;
        s.changed_frame = frame;
        print("changed");
    } else if (!s.stable_fields.empty() && fields == s.stable_fields && frame - s.changed_frame <= 2) {
        print("back after a short change");
        s.stable_fields.clear();
    }
    s.fields = std::move(fields);
    s.run = 0;
}

} // namespace Vulkan
