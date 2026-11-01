// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: program and pipeline selection (see pipeline_cache.h).

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>

#include "bb_golden_engine.h"
#include "bbport_sections.h"
#include "bbport_settings.h"
#include "bbport_toggles.h"
#include "buffers/buffer_cache.h"
#include "common/hash.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "core/emulator_settings.h"
#include "cp/command_processor.h"
#include "game_profile.h"
#include "gcn/decoder.h"
#include "render/direct_shaders.h"
#include "render/gcn_to_vk.h"
#include "render/pipeline_cache.h"
#include "render/pipeline_store.h"
#include "render/scheduler.h"
#include "render/shader_capture.h"
#include "render/vk_draw_prep.h"
#include "shader/backend/spirv/emit_spirv.h"
#include "shader/info.h"
#include "shader/runtime_info.h"
#include "upscale/motion_history.h"
#include "vulkan/instance.h"
#include "vulkan/shader_module.h"

namespace Vulkan {

using Shader::HwStage;
using Shader::Output;
using Shader::SwStage;

// Shader/pipeline compile time on the GPU thread, reported by BB_FRAME_STATS.
std::atomic<u64> g_bb_compile_ns;
std::atomic<u32> g_bb_compiles;

namespace {

constexpr u32 SpirvVersion1_6 = 0x00010600U;

constexpr std::array DescriptorHeapSizes = {
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 512},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 8192},
    vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 8192},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1024},
    vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1024},
};

void AddCompileTime(std::chrono::steady_clock::time_point start) {
    g_bb_compile_ns += u64(std::chrono::duration_cast<std::chrono::nanoseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count());
}

struct CompileTimer {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    ~CompileTimer() {
        AddCompileTime(start);
        ++g_bb_compiles;
    }
};

/// The vertex outputs as the hardware lays them out (VS_OUT_CNTL): the misc vector, then the
/// clip/cull distance vectors that are enabled.
u32 MapOutputs(std::span<Shader::OutputMap, 3> outputs, const Gcn::VsOutputControl& ctl) {
    u32 num_outputs = 0;
    if (ctl.vs_out_misc_enable) {
        auto& misc = outputs[num_outputs++];
        misc[0] = ctl.use_vtx_point_size ? Output::PointSize : Output::None;
        misc[1] = ctl.use_vtx_edge_flag
                      ? Output::EdgeFlag
                      : (ctl.use_vtx_gs_cut_flag ? Output::GsCutFlag : Output::None);
        misc[2] = ctl.use_vtx_kill_flag
                      ? Output::KillFlag
                      : (ctl.use_vtx_render_target_idx ? Output::RenderTargetIndex : Output::None);
        misc[3] = ctl.use_vtx_viewport_idx ? Output::ViewportIndex : Output::None;
    }
    const auto distance = [&](u32 index) {
        if (ctl.IsClipDistEnabled(index)) {
            return Output(u32(Output::ClipDist0) + index);
        }
        return ctl.IsCullDistEnabled(index) ? Output(u32(Output::CullDist0) + index)
                                            : Output::None;
    };
    const std::array<bool, 2> distance_vectors = {bool(ctl.vs_out_ccdist0_enable),
                                                  bool(ctl.vs_out_ccdist1_enable)};
    for (u32 vector = 0; vector < distance_vectors.size(); ++vector) {
        if (distance_vectors[vector]) {
            auto& map = outputs[num_outputs++];
            for (u32 component = 0; component < map.size(); ++component) {
                map[component] = distance(vector * 4 + component);
            }
        }
    }
    return num_outputs;
}

/// Clip distances (guest-exported, or user clip planes the vertex stage lowers) need the
/// fragment stage to discard where the host has no clip distances (NVIDIA): it then reads them
/// at input locations after the vertex outputs.
bool NeedsClipDistanceEmulation(const Gcn::Regs& regs, const Shader::Profile& profile) {
    const bool lowers_user_clip_planes =
        regs.clipper_control.user_clip_plane_enable &&
        !regs.stage_enable.IsStageEnabled(static_cast<u32>(HwStage::Geometry));
    return ((regs.vs_output_control.clip_distance_enable &&
             !regs.stage_enable.IsStageEnabled(static_cast<u32>(HwStage::Local))) ||
            lowers_user_clip_planes) &&
           profile.needs_clip_distance_emulation;
}

bool IsDualSourceBlend(const Gcn::BlendControl& blend) {
    if (!blend.enable) {
        return false;
    }
    bool dual = GcnToVk::IsDualSourceBlendFactor(blend.color_dst_factor) ||
                GcnToVk::IsDualSourceBlendFactor(blend.color_src_factor);
    if (blend.separate_alpha_blend) {
        dual |= GcnToVk::IsDualSourceBlendFactor(blend.alpha_dst_factor) ||
                GcnToVk::IsDualSourceBlendFactor(blend.alpha_src_factor);
    }
    return dual;
}

Gcn::PrimitiveType GsInputPrimitive(const Gcn::Regs& regs) {
    if (regs.stage_enable.raw != Gcn::ShaderStageEnable::LsHsEsGs) {
        return regs.primitive_type;
    }
    switch (regs.tess_config.topology) {
    case Gcn::TessellationTopology::Point:
        return Gcn::PrimitiveType::PointList;
    case Gcn::TessellationTopology::Line:
        return Gcn::PrimitiveType::LineList;
    case Gcn::TessellationTopology::TriangleCw:
    case Gcn::TessellationTopology::TriangleCcw:
        return Gcn::PrimitiveType::TriangleList;
    default:
        UNREACHABLE();
    }
}

/// The render target a draw goes to: its first color buffer written, else its depth buffer,
/// else 0.
u64 DrawTarget(const Gcn::Regs& regs) {
    if (regs.color_control.mode != Gcn::ColorControl::OperationMode::Disable) {
        for (u32 cb = 0; cb < Gcn::NUM_COLOR_BUFFERS; ++cb) {
            if (regs.color_buffers[cb] && regs.color_target_mask.GetMask(cb)) {
                return regs.color_buffers[cb].Address();
            }
        }
    }
    return regs.depth_buffer.DepthValid() ? regs.depth_buffer.DepthAddress() : 0;
}

Shader::Profile MakeProfile(const Instance& instance, u32 sparse_page_shift) {
    const auto& vk12 = instance.GetVk12Properties();
    const auto driver = instance.GetDriverID();
    const bool nvidia = driver == vk::DriverId::eNvidiaProprietary;
    return Shader::Profile{
        .max_viewport_width = instance.GetMaxViewportWidth(),
        .max_viewport_height = instance.GetMaxViewportHeight(),
        .max_shared_memory_size = instance.MaxComputeSharedMemorySize(),
        .supported_spirv = SpirvVersion1_6,
        .subgroup_size = instance.SubgroupSize(),
        .sparse_page_shift = sparse_page_shift,
        .support_int8 = instance.IsShaderInt8Supported(),
        .support_int16 = instance.IsShaderInt16Supported(),
        .support_int64 = instance.IsShaderInt64Supported(),
        .support_float16 = instance.IsShaderFloat16Supported(),
        .support_float64 = instance.IsShaderFloat64Supported(),
        .supports_denorm_behavior_independence =
            vk12.denormBehaviorIndependence != vk::ShaderFloatControlsIndependence::eNone,
        .supports_rounding_mode_independence =
            vk12.roundingModeIndependence != vk::ShaderFloatControlsIndependence::eNone,
        .support_fp16_denorm_preserve = bool(vk12.shaderDenormPreserveFloat16),
        .support_fp16_denorm_flush = bool(vk12.shaderDenormFlushToZeroFloat16),
        .support_fp16_round_to_zero = bool(vk12.shaderRoundingModeRTZFloat16),
        .support_fp32_denorm_preserve = bool(vk12.shaderDenormPreserveFloat32),
        .support_fp32_denorm_flush = bool(vk12.shaderDenormFlushToZeroFloat32),
        .support_fp32_round_to_zero = bool(vk12.shaderRoundingModeRTZFloat32),
        .support_fp64_denorm_preserve = bool(vk12.shaderDenormPreserveFloat64),
        .support_fp64_denorm_flush = bool(vk12.shaderDenormFlushToZeroFloat64),
        .support_fp64_round_to_zero = bool(vk12.shaderRoundingModeRTZFloat64),
        .support_fp16_signed_zero_inf_nan_preserve =
            bool(vk12.shaderSignedZeroInfNanPreserveFloat16),
        .support_fp32_signed_zero_inf_nan_preserve =
            bool(vk12.shaderSignedZeroInfNanPreserveFloat32),
        .support_fp64_signed_zero_inf_nan_preserve =
            bool(vk12.shaderSignedZeroInfNanPreserveFloat64),
        .supports_image_load_store_lod = instance.IsImageLoadStoreLodSupported(),
        .supports_native_cube_calc = instance.IsAmdGcnShaderSupported(),
        .supports_trinary_minmax = instance.IsAmdShaderTrinaryMinMaxSupported(),
        .supports_buffer_fp32_atomic_min_max =
            instance.IsShaderAtomicFloatBuffer32MinMaxSupported(),
        .supports_image_fp32_atomic_min_max = instance.IsShaderAtomicFloatImage32MinMaxSupported(),
        .supports_buffer_int64_atomics = instance.IsBufferInt64AtomicsSupported(),
        .supports_shared_int64_atomics = instance.IsSharedInt64AtomicsSupported(),
        .supports_workgroup_explicit_memory_layout =
            instance.IsWorkgroupMemoryExplicitLayoutSupported(),
        .supports_amd_shader_explicit_vertex_parameter =
            instance.IsAmdShaderExplicitVertexParameterSupported(),
        .supports_fragment_shader_barycentric = instance.IsFragmentShaderBarycentricSupported(),
        .supports_shader_subgroup_clock = instance.IsShaderSubgroupClockSupported(),
        .needs_manual_interpolation = instance.IsFragmentShaderBarycentricSupported() && nvidia,
        // Older NVIDIA (Pascal) has no barycentrics; BB_INTERP_INT_FIX=0/1 overrides.
        .needs_integer_interpolation_fix = [&] {
            if (const char* env = std::getenv("BB_INTERP_INT_FIX")) {
                return env[0] == '1';
            }
            return !instance.IsFragmentShaderBarycentricSupported() && nvidia;
        }(),
        .needs_lds_barriers = nvidia || driver == vk::DriverId::eMesaKosmickrisp,
        .needs_buffer_offsets = instance.StorageMinAlignment() > 4,
        .needs_unorm_fixup = driver == vk::DriverId::eMesaKosmickrisp,
        .needs_clip_distance_emulation = nvidia,
        .supports_shader_stencil_export = instance.IsShaderStencilExportSupported(),
        // BB_LAYER_MEMORY: buffers over nearly all memory go through the page table.
        .paged_buffers = VideoCore::BufferCache::LayerPagedActive(),
        // The game's buffer copy shader, from its profile (games/).
        .buffer_copy_shader_hash = Game::BufferCopyShader(),
    };
}

} // namespace

/// BB_ASYNC_PIPELINES: worker threads creating graphics pipelines (the driver's compile,
/// vkCreateGraphicsPipelines, with its own VkPipelineCache synchronization). Jobs start in order.
class PipelineCompiler {
public:
    explicit PipelineCompiler(u32 threads) {
        for (u32 i = 0; i < threads; ++i) {
            workers.emplace_back([this] { Run(); });
        }
    }
    ~PipelineCompiler() {
        {
            std::scoped_lock lk{mutex};
            stop = true;
        }
        cv.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
    }
    void Submit(std::function<void()> job) {
        {
            std::scoped_lock lk{mutex};
            jobs.push_back(std::move(job));
        }
        cv.notify_one();
    }

private:
    void Run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock lk{mutex};
                cv.wait(lk, [&] { return stop || !jobs.empty(); });
                if (jobs.empty()) {
                    return; // stopping, every job done
                }
                job = std::move(jobs.front());
                jobs.pop_front();
            }
            job();
        }
    }
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<std::function<void()>> jobs;
    std::vector<std::thread> workers;
    bool stop = false;
};

struct PipelineCache::PendingPipeline {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::unique_ptr<GraphicsPipeline> pipeline;
    GraphicsPipeline::SerializationSupport sdata{};
};

using Gcn::RegDirty;

struct PipelineCache::FastSelect {
    struct Stage {
        Program* program = nullptr;
        HwStage hw_stage{};
        SwStage sw_stage{};
        VAddr pgm_base = 0;
        u64 shader_hash = 0;
        u32 perm_idx = 0;
        Shader::RuntimeInfo runtime_info{};
    };
    struct CachedFetch {
        std::vector<u32> words;
        Shader::Gcn::FetchShaderData data;
    };
    static constexpr u32 MaxStages = 2; ///< VS mode: fragment (optional), vertex
    static constexpr u32 MaxSigWords = 72;
    static constexpr u32 NumSets = 512;
    static constexpr u32 NumWays = 4;
    static constexpr u32 TableSize = NumSets * NumWays;

    struct Entry {
        u64 key = 0;
        u64 last_use = 0;
        const GraphicsPipeline* pipeline = nullptr;
        u16 sig_words = 0;
        u8 num_stages = 0;
        bool motion = false;
        bool motion_possible = false;
        std::array<u32, MaxSigWords> sig{};
        std::array<Stage, MaxStages> stages{};
    };

    std::array<Entry, TableSize> entries{};
    std::unordered_map<const u32*, CachedFetch> fetch_map;
    std::array<std::pair<const u32*, const CachedFetch*>, 64> fetch_mru{};
    u64 use_clock = 0;

    std::array<Stage, MaxStages> pending{};
    u32 num_pending = 0;
    bool pending_ok = true;
    bool pending_motion_possible = false;

    u64 hits = 0, full = 0, miss_regs = 0, miss_program = 0, miss_spec = 0, miss_motion = 0;
    u64 not_eligible = 0, verify_bad = 0, calls = 0;
    std::chrono::steady_clock::time_point window = std::chrono::steady_clock::now();
};

void PipelineCache::InvalidateFastSelect() {
    if (fast) {
        for (auto& e : fast->entries) {
            e.key = 0;
            e.pipeline = nullptr;
        }
        fast->fetch_map.clear();
        fast->fetch_mru.fill({});
        fast->use_clock = 0;
    }
}

PipelineCache::PipelineCache(const Instance& instance_, Scheduler& scheduler_,
                             BbCp::CommandProcessor* liverpool_, u32 sparse_page_shift)
    : instance{instance_}, scheduler{scheduler_}, liverpool{liverpool_},
      desc_heap{instance, scheduler.GetWorkSemaphore(), DescriptorHeapSizes},
      profile{MakeProfile(instance_, sparse_page_shift)} {
    fast = std::make_unique<FastSelect>();
    sel.regs = &liverpool->regs;
    WarmUp();

    const char* async_env = std::getenv("BB_ASYNC_PIPELINES");
    if (!(async_env && async_env[0] == '0') && instance.IsVertexInputDynamicState()) {
        const char* thr_env = std::getenv("BB_ASYNC_PIPELINE_THREADS");
        const u32 threads = std::max(
            thr_env && *thr_env ? u32(std::strtoul(thr_env, nullptr, 10)) : 2u,
            1u);
        compiler = std::make_unique<PipelineCompiler>(threads);
        std::printf("[bb_vulkan] V4.2 Async PipelineCompiler prewarmed with %u worker threads\n",
                    threads);
    }
}

PipelineCache::~PipelineCache() {
    compiler.reset();
    if (driver_cache_dirty.load(std::memory_order_relaxed)) {
        SaveDriverPipelineCache();
    }
    const auto tel = DirectShaders::ReadTelemetry();
    if (tel.registered_binaries != 0 || tel.stage_a_direct_hits != 0 ||
        tel.worker_direct_hits != 0) {
        std::printf("[bb_vulkan] V4.1 DirectShaders telemetry: registered=%llu "
                    "(manifest=%llu/%u), StageA_hits=%llu, DrawPrep_hits=%llu\n",
                    static_cast<unsigned long long>(tel.registered_binaries),
                    static_cast<unsigned long long>(tel.manifest_binaries_bound),
                    tel.manifest_total,
                    static_cast<unsigned long long>(tel.stage_a_direct_hits),
                    static_cast<unsigned long long>(tel.worker_direct_hits));
    }
    DirectShaders::ClearCachedPrograms();
}

const Shader::RuntimeInfo& PipelineCache::BuildRuntimeInfo(PipelineSelection& sel, HwStage stage,
                                                           SwStage l_stage) {
    auto& info = sel.runtime_infos[u32(l_stage)];
    const auto& regs = *sel.regs;
    const auto common = [&](const auto& program) {
        info.props.num_user_data = program.settings.num_user_regs;
        info.props.num_input_vgprs = program.settings.vgpr_comp_cnt;
        info.props.num_allocated_vgprs = program.NumVgprs();
        info.props.fp_denorm_mode32 = program.settings.fp_denorm_mode32;
        info.props.fp_denorm_mode16_64 = program.settings.fp_denorm_mode64;
        info.props.fp_round_mode32 = program.settings.fp_round_mode32;
        info.props.fp_round_mode16_64 = program.settings.fp_round_mode64;
    };
    info.Initialize(stage, l_stage);
    switch (stage) {
    case HwStage::Local: {
        common(regs.ls_program);
        // The hull stage is selected first: its constant buffer has the LS output stride.
        Shader::TessellationDataConstantBuffer tess_constants{};
        sel.infos[u32(SwStage::TessellationControl)]->ReadTessConstantBuffer(tess_constants);
        info.hw.ls.ls_stride = tess_constants.ls_stride;
        break;
    }
    case HwStage::Hull:
        common(regs.hs_program);
        break;
    case HwStage::Export:
        common(regs.es_program);
        info.hw.es.vertex_data_size = regs.vgt_esgs_ring_itemsize;
        break;
    case HwStage::Geometry: {
        common(regs.gs_program);
        auto& gs = info.hw.gs;
        gs.num_outputs = MapOutputs(gs.outputs, regs.vs_output_control);
        gs.output_vertices = regs.vgt_gs_max_vert_out;
        gs.num_invocations =
            regs.vgt_gs_instance_cnt.IsEnabled() ? regs.vgt_gs_instance_cnt.count : 1;
        gs.in_primitive = GsInputPrimitive(regs);
        for (u32 stream_id = 0; stream_id < Shader::GsMaxOutputStreams; ++stream_id) {
            gs.out_primitive[stream_id] = regs.vgt_gs_out_prim_type.GetPrimitiveType(stream_id);
        }
        gs.in_vertex_data_size = regs.vgt_esgs_ring_itemsize;
        gs.out_vertex_data_size = regs.vgt_gs_vert_itemsize[0];
        gs.mode = regs.vgt_gs_mode.mode;
        // The copy shader (the VS program) moves the GS ring's vertices to the outputs.
        DirectShaders::Snapshot copy_snap;
        const VAddr copy_base = regs.vs_program.Address<VAddr>();
        if (DirectShaders::TryLookup(copy_base, copy_snap, sel.worker != nullptr) &&
            copy_snap.cpu_immutable != 0) {
            gs.vs_copy = std::span<const u32>{reinterpret_cast<const u32*>(copy_base),
                                              copy_snap.code_dwords};
            gs.vs_copy_hash = copy_snap.shader_hash;
        } else {
            const auto copy = Gcn::GetParams(regs.vs_program);
            gs.vs_copy = copy.code;
            gs.vs_copy_hash = copy.hash;
        }
        DumpShader(gs.vs_copy, gs.vs_copy_hash, HwStage::Vertex, 0, "copy.bin");
        break;
    }
    case HwStage::Vertex: {
        common(regs.vs_program);
        auto& vs = info.hw.vs;
        vs.user_clip_plane_mask = regs.clipper_control.user_clip_plane_enable;
        vs.num_outputs = MapOutputs(vs.outputs, regs.vs_output_control);
        vs.emulate_depth_negative_one_to_one =
            !instance.IsDepthClipControlSupported() &&
            regs.clipper_control.clip_space == Gcn::ClipSpace::MinusWToW;
        vs.clip_disable = regs.IsClipDisabled();
        vs.motion_vectors = sel.motion;
        break;
    }
    case HwStage::Fragment: {
        common(regs.ps_program);
        auto& fs = info.hw.fs;
        fs.en_flags = regs.ps_input_ena;
        fs.addr_flags = regs.ps_input_addr;
        fs.num_inputs = regs.num_interp;
        fs.front_face_all_bits = regs.barycentric_control.front_face_all_bits;
        fs.num_samples =
            regs.ps_input_addr.sample_coverage_ena && regs.ps_input_ena.sample_coverage_ena
                ? regs.aa_config.NumSamples()
                : 1;
        fs.z_export_format = regs.z_export_format;
        fs.motion_vectors = sel.motion;
        const auto& depth = regs.depth_shader_control;
        const u8 stencil_export =
            depth.stencil_op_val_export_enable | depth.stencil_test_val_export_enable;
        fs.mrtz_mask = depth.z_export_enable | (stencil_export << 1) |
                       (depth.mask_export_enable << 2) | (depth.coverage_to_mask_enable << 3);
        fs.dual_source_blending = IsDualSourceBlend(regs.blend_control[0]);
        for (u32 i = 0; i < regs.num_interp; i++) {
            const auto& input = regs.ps_inputs[i];
            fs.inputs[i] = {
                .param_index = u8(input.input_offset),
                .is_default = bool(input.use_default),
                .is_flat = bool(input.flat_shade),
                .default_value = u8(input.default_value),
            };
        }
        for (u32 i = 0; i < Shader::MaxColorBuffers; i++) {
            fs.color_buffers[i] = sel.graphics_key.color_buffers[i];
        }
        fs.clip_distance_emulation = NeedsClipDistanceEmulation(regs, profile);
        break;
    }
    case HwStage::Compute: {
        const auto& cs = liverpool->GetCsRegs();
        info.props.num_user_data = cs.settings.num_user_regs;
        info.props.num_allocated_vgprs = cs.settings.num_vgprs * 4;
        info.props.fp_denorm_mode32 = cs.settings.fp_denorm_mode32;
        info.props.fp_denorm_mode16_64 = cs.settings.fp_denorm_mode64;
        info.props.fp_round_mode32 = cs.settings.fp_round_mode32;
        info.props.fp_round_mode16_64 = cs.settings.fp_round_mode64;
        info.hw.cs.workgroup_size = {cs.num_thread_x.full, cs.num_thread_y.full,
                                     cs.num_thread_z.full};
        info.hw.cs.tgid_enable = {cs.IsTgidEnabled(0), cs.IsTgidEnabled(1), cs.IsTgidEnabled(2)};
        info.hw.cs.shared_memory_size = cs.SharedMemSize();
        break;
    }
    default:
        break;
    }
    switch (l_stage) {
    case SwStage::Vertex:
        info.sw.vs.step_rate_0 = regs.vgt_instance_step_rate_0;
        info.sw.vs.step_rate_1 = regs.vgt_instance_step_rate_1;
        info.sw.vs.vertex_sgpr_offset = sel.draw_indirect_params.vertex_sgpr_offset;
        info.sw.vs.instance_sgpr_offset = sel.draw_indirect_params.instance_sgpr_offset;
        info.sw.vs.tess_emulated_primitive =
            regs.primitive_type == Gcn::PrimitiveType::RectList ||
            regs.primitive_type == Gcn::PrimitiveType::QuadList;
        break;
    case SwStage::TessellationControl:
        info.sw.tcs.num_input_control_points = regs.ls_hs_config.hs_input_control_points;
        info.sw.tcs.num_threads = regs.ls_hs_config.hs_output_control_points;
        info.sw.tcs.tess_type = regs.tess_config.type;
        info.sw.tcs.offchip_lds_enable = regs.hs_program.settings.oc_lds_en;
        break;
    case SwStage::TessellationEval:
        info.sw.tes.tess_type = regs.tess_config.type;
        info.sw.tes.tess_topology = regs.tess_config.topology;
        info.sw.tes.tess_partitioning = regs.tess_config.partitioning;
        break;
    default:
        break;
    }
    return info;
}

bool PipelineCache::PrepareGraphicsPipeline(PipelineSelection& worker_sel) {
    // Tessellation stages read constant buffers from memory at selection time: not prepared.
    if (worker_sel.regs->stage_enable.hs_en) {
        return false;
    }
    // Single shared_lock + single sigsetjmp across all stages of the draw (cuts cross-CCD
    // programs_mutex atomic RMWs and sigsetjmp overhead in half on bb:DrawPrep0..1).
    std::shared_lock lk{programs_mutex};
    sigjmp_buf recover;
    if (sigsetjmp(recover, 0)) {
        runtime_fault_recover = nullptr;
        worker_sel.worker->failed = true;
        return false;
    }
    runtime_fault_recover = &recover;
    const bool ok = RefreshGraphicsKey(worker_sel) && !worker_sel.worker->failed;
    runtime_fault_recover = nullptr;
    return ok;
}

namespace {
/// The fetch shader a vertex program calls (through a pointer in its user data) decodes up to
/// its s_setpc_b64: the parse asserts on anything else. Memory the game is still filling, or
/// reused, holds garbage there (another game: "illegal instruction" while loading).
bool FetchCodeDecodes(const u32* code) {
    const u32* pc = code;
    for (u32 i = 0; i < 512; ++i) {
        const auto encoding = GcnIsa::EncodingOf(*pc);
        if (encoding == GcnIsa::InstEncoding::ILLEGAL) {
            return false;
        }
        const bool vector = encoding == GcnIsa::InstEncoding::VOP1 ||
                            encoding == GcnIsa::InstEncoding::VOP2 ||
                            encoding == GcnIsa::InstEncoding::VOPC;
        if (vector && ((*pc & 0x1FF) == 0xF9 || (*pc & 0x1FF) == 0xFA)) {
            return false; // SDWA/DPP: not in a PS4 program
        }
        if (GcnIsa::Decode(pc).opcode == GcnIsa::Opcode::S_SETPC_B64) {
            return true;
        }
    }
    return false;
}
} // namespace

/// The GPU's writes before this draw, for a program that is not whole in memory (a copy into
/// place still on the GPU): at most once per address every 100 ms (thousands of draws a frame
/// use code that is simply not there; each wait is a whole GPU round trip). False: not tried.
bool PipelineCache::SyncProgramMemory(VAddr address, u64 size) {
    if (!program_memory_sync) {
        return false;
    }
    const auto now = std::chrono::steady_clock::now();
    if (const auto tried = program_sync_tried.find(address);
        tried != program_sync_tried.end() && now - tried->second < std::chrono::milliseconds(100)) {
        return false;
    }
    if (program_sync_tried.size() >= 4096) {
        program_sync_tried.clear();
    }
    program_sync_tried[address] = now;
    program_memory_sync(address, size);
    return true;
}

/// Every program the games' compilers emit starts with "s_mov_b32 vcc_hi, length" and ends in
/// an "OrbShdr" header. Anything else at a program address is not a program yet: the game copied
/// it into place with the GPU (DMA) and the CPU's view does not have it (or only its start), or
/// it freed the memory. The header search would find the next program's header (or none: an
/// assertion) and the translator would decode garbage. On the GPU command thread (`may_sync`)
/// what the GPU wrote there is read back first; still nothing: the draw is skipped. A draw
/// preparation worker gives such a draw to the GPU command thread.
bool PipelineCache::LooksLikeProgram(const u32* code, HwStage stage, bool may_sync) {
    if (may_sync && program_write_sync) {
        program_write_sync(reinterpret_cast<VAddr>(code), 256);
    }
    // Its header where its first instruction says: a header found further on (the search) may
    // be the next program's while this one is still being copied in.
    const auto complete = [code] { return Gcn::DirectBinaryInfo(code) != nullptr; };
    if (complete()) {
        return true;
    }
    const u32 first = code[0];
    if (may_sync && SyncProgramMemory(reinterpret_cast<VAddr>(code), 0x10000)) {
        if (complete()) {
            static std::atomic<u32> told{0};
            if (told.fetch_add(1, std::memory_order_relaxed) < 8) {
                LOG_INFO(Render_Vulkan, "stage {} program at {} arrived from the GPU's writes",
                         u32(stage), fmt::ptr(code));
            }
            return true;
        }
        // Read back and still no header in place: a program laid out differently (the header
        // searched for, as before), else not a program.
        if (first == 0xBEEB03FF && code[0] == 0xBEEB03FF && Gcn::FindBinaryInfo(code)) {
            static std::atomic<u32> told{0};
            if (told.fetch_add(1, std::memory_order_relaxed) < 8) {
                LOG_WARNING(Render_Vulkan,
                            "stage {} program at {}: its header is not where its first "
                            "instruction says; the next header found is used",
                            u32(stage), fmt::ptr(code));
            }
            return true;
        }
    }
    static std::atomic<u32> reported{0};
    if (reported.fetch_add(1, std::memory_order_relaxed) < 8) {
        LOG_WARNING(Render_Vulkan,
                    "stage {} program at {} is not a whole program ({:#010x} {:#010x} {:#010x} "
                    "{:#010x}, {:#010x} before a read back, header {}): draw skipped",
                    u32(stage), fmt::ptr(code), code[0], code[1], code[2], code[3], first,
                    Gcn::FindBinaryInfo(code) ? "found" : "missing");
    }
    return false;
}

/// A new program decodes whole before it is translated. A program the GPU copied into place
/// (DMA_DATA, CP DMA) may not be in the CPU's view of the memory yet: then what the GPU wrote is
/// read back once and the program decoded again. Still not decoding: the draw is skipped.
bool PipelineCache::ProgramDecodes(const Shader::ShaderParams& params, HwStage stage) {
    const auto first_illegal = [&]() -> s64 {
        const u32* pc = params.code.data();
        const u32* const end = pc + params.code.size();
        while (pc < end) {
            const auto encoding = GcnIsa::EncodingOf(*pc);
            if (encoding == GcnIsa::InstEncoding::ILLEGAL) {
                return pc - params.code.data();
            }
            // SDWA (0xF9) and DPP (0xFA) operands came with later GCN generations: in a PS4
            // program such a word is garbage, not an instruction.
            const bool vector = encoding == GcnIsa::InstEncoding::VOP1 ||
                                encoding == GcnIsa::InstEncoding::VOP2 ||
                                encoding == GcnIsa::InstEncoding::VOPC;
            if (vector && ((*pc & 0x1FF) == 0xF9 || (*pc & 0x1FF) == 0xFA)) {
                return pc - params.code.data();
            }
            GcnIsa::Decode(pc);
        }
        return -1;
    };
    s64 bad = first_illegal();
    if (bad < 0) {
        return true;
    }
    const VAddr address = reinterpret_cast<VAddr>(params.code.data());
    const u64 bytes = params.code.size_bytes();
    const u32 word = params.code[bad];
    const s64 at = bad;
    if (SyncProgramMemory(address, bytes)) {
        bad = first_illegal();
    }
    static std::atomic<u32> reported{0};
    if (reported.fetch_add(1, std::memory_order_relaxed) < 16) {
        LOG_WARNING(Render_Vulkan,
                    "stage {} program {:#x} at {:#x} ({} bytes): word {:#010x} at dword {} does "
                    "not decode; {}",
                    u32(stage), params.hash, address, bytes, word, at,
                    bad < 0 ? "decodes after reading back what the GPU wrote"
                            : "still not after a read back: draw skipped");
    }
    return bad < 0;
}

const GraphicsPipeline* PipelineCache::TryPreparedPipeline(const PreparedDraw& prepared) {
    BB_SECTION(APrepared);
    if (prepared.state.load(std::memory_order_acquire) != PreparedDraw::Ready ||
        prepared.reg_checksum != liverpool->gfx_reg_checksum) {
        return nullptr;
    }
    // Same registers; the stage programs and their flattened user data (which also covers
    // everything read from guest memory for the specialization) must match as well. This is
    // the per-stage work GetProgram does on the regular path.
    const auto& regs = liverpool->regs;
    for (u32 i = 0; i < prepared.num_stages; ++i) {
        const auto& stage = prepared.stages[i];
        const auto* pgm = regs.ProgramForStage(static_cast<u32>(stage.hw_stage));
        if (!pgm) {
            return nullptr;
        }
        const VAddr base = pgm->Address<VAddr>();
        if (base != stage.pgm_base || base == 0) {
            return nullptr;
        }
        DirectShaders::Snapshot direct_snap;
        if (DirectShaders::TryLookup(base, direct_snap, /*is_worker=*/false) &&
            direct_snap.cpu_immutable != 0) {
            if (direct_snap.shader_hash != stage.hash) {
                return nullptr;
            }
        } else {
            if (program_write_sync) {
                program_write_sync(base, 256);
            }
            const auto* bin = Gcn::DirectBinaryInfo(reinterpret_cast<const u32*>(base));
            if (!bin || bin->shader_hash != stage.hash) {
                return nullptr; // not a program or changed: the regular path skips or reselects
            }
        }
        auto& info = const_cast<Program*>(stage.program)->info;
        info.pgm_base = base;
        info.user_data = pgm->user_data;
        info.RefreshFlatBuf();
        if (info.flattened_ud_buf.size() != stage.flat_size ||
            std::memcmp(info.flattened_ud_buf.data(), stage.flat,
                        stage.flat_size * sizeof(u32)) != 0) {
            return nullptr;
        }
    }
    if (last_graphics_pipeline && last_graphics_pipeline->GetGraphicsKey() == prepared.key) {
        return last_graphics_pipeline;
    }
    const auto it = prepared.key_hash ? graphics_pipelines.find(prepared.key, prepared.key_hash)
                                      : graphics_pipelines.find(prepared.key);
    if (it != graphics_pipelines.end()) {
        last_graphics_pipeline = it->second.get();
        return last_graphics_pipeline;
    }
    return nullptr;
}

namespace {
/// Whether a G-buffer vertex shader gets the motion attachment (its bone palette buffers):
/// shared by the full selection (RefreshGraphicsKey) and the fast select.
bool VsSelectsMotion(const Shader::Info& vs) {
    for (const auto& resource : vs.buffers) {
        if (resource.IsSpecial()) {
            continue;
        }
        const auto buffer = resource.GetSharp(vs);
        if (buffer.Valid() && buffer.GetStride() == 16 &&
            Motion::ClassifyBuffer(buffer.GetSize()) != Motion::BufferRole::Other) {
            return true;
        }
    }
    return false;
}

bool AllMotionEnv() {
    // BB_OBJECT_MOTION_ALL=1: the old broad selection, for visual A/B tests.
    static const bool all_motion = [] {
        const char* value = std::getenv("BB_OBJECT_MOTION_ALL");
        return value && value[0] == '1';
    }();
    return all_motion;
}

bool FastSelectEnabled() {
    static const bool enabled = [] {
        const char* env = std::getenv("BB_FAST_SELECT");
        return !(env && env[0] == '0');
    }();
    return enabled;
}

bool FastSelectVerify() {
    static const bool verify = [] {
        const char* env = std::getenv("BB_FAST_SELECT_VERIFY");
        return env && env[0] == '1';
    }();
    return verify;
}

/// Packs every register field and global setting that `RefreshGraphicsKey` + `BuildRuntimeInfo`
/// read in `VgtStages::Vs` + `IsVertexInputDynamicState()` mode into `out` (~25-40 words).
/// Returns 0 if the draw configuration is not eligible for fast selection.
u32 BuildFastSelectSig(const Gcn::Regs& regs, const DrawIndirectParams& params,
                       bool dynamic_vertex_input,
                       std::array<u32, 72>& out) noexcept {
    if (!dynamic_vertex_input ||
        regs.stage_enable.raw != Gcn::ShaderStageEnable::VgtStages::Vs ||
        regs.num_interp > 32u) {
        return 0;
    }
    std::memcpy(&out[0], &regs.reg_array[0x2C08], 4 * sizeof(u32)); // PS address + settings
    std::memcpy(&out[4], &regs.reg_array[0x2C48], 4 * sizeof(u32)); // VS address + settings

    const bool obj_motion = BbSettings::Get().object_motion.load(std::memory_order_relaxed);
    const int upscaler = BbSettings::Get().upscaler.load(std::memory_order_relaxed);
    const bool has_pos = (Shader::MotionVectors::positions_address != 0);

    out[8] = u32(params.vertex_sgpr_offset) | (u32(params.instance_sgpr_offset) << 16);
    out[9] = u32(obj_motion) |
             (u32(has_pos) << 1) |
             (u32(upscaler & 0xFF) << 4) |
             (u32(!regs.depth_render_override.disable_viewport_clamp) << 12) |
             (u32(regs.polygon_control.provoking_vtx_last) << 13) |
             (u32(regs.polygon_control.PolyMode()) << 14) |
             (u32(regs.barycentric_control.front_face_all_bits) << 18) |
             (u32(IsDualSourceBlend(regs.blend_control[0])) << 19) |
             (u32(regs.aa_config.NumSamples()) << 20);
    out[10] = u32(regs.primitive_type);
    out[11] = regs.reg_array[0xA204]; // PA_CL_CLIP_CNTL
    out[12] = regs.reg_array[0xA207]; // PA_CL_VS_OUT_CNTL
    out[13] = u32(regs.color_control.mode) |
              (u32(regs.color_control.rop3) << 8) |
              (u32(regs.z_export_format) << 16);
    out[14] = regs.color_shader_mask.raw;
    out[15] = regs.color_target_mask.raw;
    out[16] = regs.color_export_format.raw;
    out[17] = regs.reg_array[0xA203]; // DB_SHADER_CONTROL
    out[18] = regs.reg_array[0xA1B3]; // SPI_PS_INPUT_ENA
    out[19] = regs.reg_array[0xA1B4]; // SPI_PS_INPUT_ADDR
    out[20] = regs.vgt_instance_step_rate_0;
    out[21] = regs.vgt_instance_step_rate_1;

    const auto& db = regs.depth_buffer;
    const bool db_d = db.DepthValid();
    const bool db_s = db.StencilValid();
    const bool color_disabled =
        (regs.color_control.mode == Gcn::ColorControl::OperationMode::Disable);
    u32 bound_mask = 0;
    u32 idx = 23;
    for (u32 cb = 0; cb < Gcn::NUM_COLOR_BUFFERS; ++cb) {
        if (!color_disabled && regs.color_buffers[cb] && regs.color_target_mask.GetMask(cb)) {
            bound_mask |= (1u << cb);
            const auto& col_buf = regs.color_buffers[cb];
            out[idx++] = (col_buf.info.raw & 0x00023FFCu) |
                         (u32(col_buf.attrib.num_fragments_log2) << 24) |
                         0x80000000u;
            out[idx++] = (regs.blend_control[cb].enable && !col_buf.info.blend_bypass)
                             ? regs.reg_array[0xA1E0 + cb]
                             : 0u;
        }
    }
    const u32 n_interp = regs.num_interp;
    out[22] = (db_d ? (u32(db.z_info.format) | 0x8u) : 0u) |
              ((db_s ? (u32(db.stencil_info.format) | 0x8u) : 0u) << 4) |
              (u32((db_d || db_s) ? db.NumSamples() : 1u) << 8) |
              (bound_mask << 14) |
              (n_interp << 22) |
              (u32(bool(regs.color_buffers[Shader::MotionVectors::Output])) << 28);
    if (n_interp != 0) {
        std::memcpy(&out[idx], &regs.reg_array[0xA191], n_interp * sizeof(u32));
        idx += n_interp;
    }
    return idx;
}
} // namespace


void PipelineCache::NoteFastProgram(Program* program, HwStage hw_stage, SwStage sw_stage,
                                    VAddr pgm_base, u64 shader_hash, size_t perm_idx) {
    if (!fast || sw_stage == SwStage::Compute) {
        return;
    }
    auto& f = *fast;
    if (f.num_pending >= FastSelect::MaxStages) {
        f.pending_ok = false;
        return;
    }
    auto& st = f.pending[f.num_pending++];
    st.program = program;
    st.hw_stage = hw_stage;
    st.sw_stage = sw_stage;
    st.pgm_base = pgm_base;
    st.shader_hash = shader_hash;
    st.perm_idx = static_cast<u32>(perm_idx);
}

void PipelineCache::CommitFastSelect(const GraphicsPipeline* pipeline,
                                     const DrawIndirectParams& params) {
    auto& f = *fast;
    const auto& regs = liverpool->regs;
    if (!pipeline || !f.pending_ok || f.num_pending == 0) {
        ++f.not_eligible;
        return;
    }
    std::array<u32, FastSelect::MaxSigWords> sig{};
    const u32 sig_len =
        BuildFastSelectSig(regs, params, instance.IsVertexInputDynamicState(), sig);
    if (sig_len == 0) {
        return;
    }
    const u64 sig_key = XXH3_64bits(sig.data(), sig_len * sizeof(u32)) | 1ULL;
    const size_t set_base =
        ((sig_key >> 1) & (FastSelect::NumSets - 1)) * FastSelect::NumWays;

    FastSelect::Entry* target = nullptr;
    for (u32 w = 0; w < FastSelect::NumWays; ++w) {
        auto& cand = f.entries[set_base + w];
        if (cand.key == sig_key && cand.pipeline == pipeline && cand.sig_words == sig_len &&
            std::memcmp(cand.sig.data(), sig.data(), sig_len * sizeof(u32)) == 0) {
            target = &cand;
            break;
        }
    }
    if (!target) {
        target = &f.entries[set_base];
        for (u32 w = 1; w < FastSelect::NumWays; ++w) {
            auto& cand = f.entries[set_base + w];
            if (cand.key == 0) {
                target = &cand;
                break;
            }
            if (cand.last_use < target->last_use) {
                target = &cand;
            }
        }
    }

    target->key = sig_key;
    target->last_use = ++f.use_clock;
    target->pipeline = pipeline;
    target->sig_words = static_cast<u16>(sig_len);
    target->num_stages = static_cast<u8>(f.num_pending);
    target->motion = sel.motion;
    target->motion_possible = f.pending_motion_possible;
    std::memcpy(target->sig.data(), sig.data(), sig_len * sizeof(u32));
    for (u32 i = 0; i < f.num_pending; ++i) {
        target->stages[i] = f.pending[i];
        target->stages[i].runtime_info =
            sel.runtime_infos[static_cast<u32>(f.pending[i].sw_stage)];
    }
}

const GraphicsPipeline* PipelineCache::TryFastSelect(const DrawIndirectParams& params) {
    auto& f = *fast;
    const auto& regs = liverpool->regs;
    std::array<u32, FastSelect::MaxSigWords> sig{};
    const u32 sig_len =
        BuildFastSelectSig(regs, params, instance.IsVertexInputDynamicState(), sig);
    if (sig_len == 0) {
        ++f.not_eligible;
        return nullptr;
    }
    const u64 sig_key = XXH3_64bits(sig.data(), sig_len * sizeof(u32)) | 1ULL;
    const size_t set_base =
        ((sig_key >> 1) & (FastSelect::NumSets - 1)) * FastSelect::NumWays;

    FastSelect::Entry* first_match = nullptr;
    u32 matching_ways_mask = 0;
    for (u32 w = 0; w < FastSelect::NumWays; ++w) {
        auto& cand = f.entries[set_base + w];
        if (cand.key == sig_key && cand.sig_words == sig_len &&
            std::memcmp(cand.sig.data(), sig.data(), sig_len * sizeof(u32)) == 0) {
            if (!first_match) {
                first_match = &cand;
            }
            matching_ways_mask |= (1u << w);
        }
    }
    if (!first_match) {
        ++f.miss_regs;
        return nullptr;
    }

    const auto lookup_fetch = [&](const Shader::Info& info) -> const Shader::Gcn::FetchShaderData* {
        const u32* const code =
            Shader::Gcn::GetFetchShaderCode(info, info.fetch_shader_sgpr_base);
        if (!code) {
            return nullptr;
        }
        auto& mru = f.fetch_mru[(reinterpret_cast<uintptr_t>(code) >> 4) & 63];
        if (!BbToggle::Disabled(BbToggle::FetchShaderCache) && mru.first == code && mru.second &&
            std::memcmp(code, mru.second->words.data(),
                        mru.second->words.size() * sizeof(u32)) == 0) {
            return &mru.second->data;
        }
        if (const auto it = f.fetch_map.find(code);
            !BbToggle::Disabled(BbToggle::FetchShaderCache) && it != f.fetch_map.end() &&
            std::memcmp(code, it->second.words.data(),
                        it->second.words.size() * sizeof(u32)) == 0) {
            mru = {code, &it->second};
            return &it->second.data;
        }
        if (program_write_sync) {
            program_write_sync(reinterpret_cast<VAddr>(code), 256);
        }
        if (!FetchCodeDecodes(code)) {
            return nullptr;
        }
        auto parsed = Shader::Gcn::ParseFetchShader(info);
        if (!parsed || parsed->size == 0 || (parsed->size & 3u) != 0) {
            return nullptr;
        }
        if (f.fetch_map.size() >= 4096) {
            f.fetch_map.clear();
            f.fetch_mru.fill({});
        }
        auto& slot = f.fetch_map[code];
        slot.words.assign(code, code + (parsed->size / sizeof(u32)));
        slot.data = std::move(*parsed);
        mru = {code, &slot};
        return &slot.data;
    };

    // Refresh user data & fetch shader once per stage across all candidate ways in the set.
    std::array<const Shader::Gcn::FetchShaderData*, FastSelect::MaxStages> cur_fetch{};
    const Shader::Info* vs_info = nullptr;
    for (u32 i = 0; i < first_match->num_stages; ++i) {
        const auto& st = first_match->stages[i];
        const auto* pgm = regs.ProgramForStage(static_cast<u32>(st.hw_stage));
        if (!pgm) {
            ++f.miss_program;
            return nullptr;
        }
        const VAddr base = pgm->Address<VAddr>();
        if (base != st.pgm_base || base == 0) {
            ++f.miss_program;
            return nullptr;
        }
        DirectShaders::Snapshot direct_snap;
        if (DirectShaders::TryLookup(base, direct_snap, /*is_worker=*/false) &&
            direct_snap.cpu_immutable != 0) {
            if (direct_snap.shader_hash != st.shader_hash) {
                ++f.miss_program;
                return nullptr;
            }
        } else {
            if (program_write_sync) {
                program_write_sync(base, 256);
            }
            const auto* bin = Gcn::DirectBinaryInfo(reinterpret_cast<const u32*>(base));
            if (!bin || bin->shader_hash != st.shader_hash) {
                ++f.miss_program;
                return nullptr;
            }
        }
        auto& info = st.program->info;
        info.pgm_base = base;
        info.user_data = pgm->user_data;
        info.RefreshFlatBuf();
        if (info.has_fetch_shader) {
            cur_fetch[i] = lookup_fetch(info);
            if (!cur_fetch[i]) {
                ++f.miss_program;
                return nullptr;
            }
        }
        if (st.sw_stage == SwStage::Vertex) {
            vs_info = &info;
        }
    }

    const bool cur_motion = first_match->motion_possible &&
                            (AllMotionEnv() || (vs_info && VsSelectsMotion(*vs_info)));

    bool saw_motion_mismatch = false;
    for (u32 w = 0; w < FastSelect::NumWays; ++w) {
        if (((matching_ways_mask >> w) & 1u) == 0) {
            continue;
        }
        auto& way = f.entries[set_base + w];
        if (way.num_stages != first_match->num_stages) {
            continue;
        }
        if (way.motion_possible && way.motion != cur_motion) {
            saw_motion_mismatch = true;
            continue;
        }
        bool programs_ok = true;
        std::array<const Shader::Gcn::FetchShaderData*, FastSelect::MaxStages> way_fetch =
            cur_fetch;
        for (u32 i = 0; i < way.num_stages; ++i) {
            const auto& fst = first_match->stages[i];
            const auto& wst = way.stages[i];
            if (wst.hw_stage != fst.hw_stage || wst.pgm_base != fst.pgm_base ||
                wst.shader_hash != fst.shader_hash) {
                programs_ok = false;
                break;
            }
            if (wst.program != fst.program) {
                const auto* pgm = regs.ProgramForStage(static_cast<u32>(wst.hw_stage));
                auto& winfo = wst.program->info;
                winfo.pgm_base = wst.pgm_base;
                winfo.user_data = pgm->user_data;
                winfo.RefreshFlatBuf();
                if (winfo.has_fetch_shader) {
                    way_fetch[i] = lookup_fetch(winfo);
                    if (!way_fetch[i]) {
                        programs_ok = false;
                        break;
                    }
                } else {
                    way_fetch[i] = nullptr;
                }
            }
        }
        if (!programs_ok) {
            continue;
        }
        Shader::Backend::Bindings binding{};
        bool spec_ok = true;
        for (u32 i = 0; i < way.num_stages; ++i) {
            const auto& st = way.stages[i];
            const auto& modules = st.program->modules;
            auto& info = st.program->info;
            if (st.perm_idx >= modules.size() ||
                !modules[st.perm_idx].spec.MatchesInPlace(info, st.runtime_info, profile, binding,
                                                          way_fetch[i])) {
                spec_ok = false;
                break;
            }
            info.AddBindings(binding);
        }
        if (spec_ok) {
            for (u32 i = 0; i < way.num_stages; ++i) {
                way.stages[i].program->last_used = way.stages[i].perm_idx;
            }
            way.last_use = ++f.use_clock;
            ++f.hits;
            return way.pipeline;
        }
    }
    if (saw_motion_mismatch) {
        ++f.miss_motion;
    } else {
        ++f.miss_spec;
    }
    return nullptr;
}

void PipelineCache::PrintFastSelectStats() {
    auto& f = *fast;
    static const bool stats = std::getenv("BB_FRAME_STATS") != nullptr;
    if (!stats || (++f.calls & 1023) != 0) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (now - f.window < std::chrono::seconds(5)) {
        return;
    }
    f.window = now;
    const u64 total = std::max<u64>(f.hits + f.full, 1);
    std::printf("Fast select (5 s): reused %llu (%.1f%%), full %llu; misses: registers %llu, "
                "program %llu, specialization %llu, motion %llu; not eligible %llu; verify "
                "mismatches %llu\n",
                (unsigned long long)f.hits, 100.0 * double(f.hits) / double(total),
                (unsigned long long)f.full, (unsigned long long)f.miss_regs,
                (unsigned long long)f.miss_program, (unsigned long long)f.miss_spec,
                (unsigned long long)f.miss_motion, (unsigned long long)f.not_eligible,
                (unsigned long long)f.verify_bad);
    f.hits = f.full = f.miss_regs = f.miss_program = f.miss_spec = f.miss_motion = 0;
    f.not_eligible = 0;
}

void PipelineCache::NoteDrawTarget(const Gcn::Regs& regs) {
    const u32 frame = BbStats::frame_number.load(std::memory_order_relaxed);
    const u64 target = DrawTarget(regs);
    if (target == 0 || (target == last_target && frame == last_target_frame)) {
        return;
    }
    last_target = target;
    last_target_frame = frame;
    auto& streak = target_streaks[target];
    if (streak.last_frame != frame) {
        streak.frames = streak.last_frame + 1 == frame ? streak.frames + 1 : 1;
        streak.last_frame = frame;
    }
}

const GraphicsPipeline* PipelineCache::GetGraphicsPipeline(const DrawIndirectParams params,
                                                           const PreparedDraw* prepared,
                                                           bool indirect) {
    BB_SECTION(APipeline);
    used_prepared = nullptr;
    NoteDrawTarget(liverpool->regs);
    if (prepared) {
        if (const auto* pipeline = TryPreparedPipeline(*prepared)) {
            used_prepared = prepared;
            return pipeline;
        }
    }
    const bool fast_select = FastSelectEnabled() && fast;
    const GraphicsPipeline* fast_pipeline = nullptr;
    if (fast_select) {
        PrintFastSelectStats();
        fast_pipeline = TryFastSelect(params);
        if (fast_pipeline && !FastSelectVerify()) {
            return fast_pipeline;
        }
    }
    const GraphicsPipeline* const selected = SelectGraphicsPipelineFull(params, indirect);
    if (fast_select) {
        if (fast_pipeline) {
            if (selected != fast_pipeline) {
                auto& f = *fast;
                if (f.verify_bad++ < 16) {
                    const auto* vs = selected ? selected->GetStages()[u32(SwStage::Vertex)]
                                              : nullptr;
                    std::printf("Fast select VERIFY mismatch: reused %p, full selection %p "
                                "(vs %016llx)\n",
                                static_cast<const void*>(fast_pipeline),
                                static_cast<const void*>(selected),
                                (unsigned long long)(vs ? vs->pgm_hash : 0));
                }
            }
        } else {
            ++fast->full;
            if (selected) {
                CommitFastSelect(selected, params);
            }
        }
    }
    return selected;
}

const GraphicsPipeline* PipelineCache::SelectGraphicsPipelineFull(const DrawIndirectParams params,
                                                                  bool indirect) {
    sel.draw_indirect_params = params;
    if (!RefreshGraphicsKey(sel)) {
        return nullptr;
    }
    if (const auto found = graphics_pipelines.find(sel.graphics_key);
        found != graphics_pipelines.end()) {
        return found->second.get();
    }
    const bool skippable = AsyncSkippable(sel, indirect);
    if (const auto pending = pending_graphics.find(sel.graphics_key);
        pending != pending_graphics.end()) {
        const std::shared_ptr<PendingPipeline> job = pending->second;
        {
            std::unique_lock lk{job->mutex};
            if (!job->done) {
                if (skippable) {
                    ++async_skipped;
                    return nullptr; // drawn from the frame its pipeline is ready
                }
                // A pass that is not drawn every frame needs it now: the wait is compile time.
                const auto start = std::chrono::steady_clock::now();
                job->cv.wait(lk, [&] { return job->done; });
                AddCompileTime(start);
                ++async_waited;
            }
        }
        pending_graphics.erase(sel.graphics_key);
        return FinishPipeline(sel.graphics_key, *job);
    }
    if (skippable) {
        return CompileInBackground(sel);
    }

    const auto pipeline_hash = std::hash<GraphicsPipelineKey>{}(sel.graphics_key);
    LOG_INFO(Render_Vulkan, "Compiling graphics pipeline {:#x}", pipeline_hash);
    CompileTimer timer;
    GraphicsPipeline::SerializationSupport sdata{};
    auto& slot = graphics_pipelines[sel.graphics_key];
    slot = std::make_unique<GraphicsPipeline>(instance, scheduler, desc_heap, profile,
                                              sel.graphics_key, *pipeline_cache, sel.infos,
                                              sel.runtime_infos, sel.fetch_shader, sel.modules,
                                              sdata, false);
    PipelineStore::SaveGraphics(sel.graphics_key, pipeline_hash, sdata);
    NoteDriverPipelineCacheDirty();
    sel.fetch_shader.reset();
    return slot.get();
}

const GraphicsPipeline* PipelineCache::CompileInBackground(const PipelineSelection& selection) {
    if (!compiler) {
        const char* env = std::getenv("BB_ASYNC_PIPELINE_THREADS");
        const u32 threads =
            std::max(env && *env ? u32(std::strtoul(env, nullptr, 10))
                                 : std::clamp(std::thread::hardware_concurrency() / 3, 2u, 6u),
                     1u);
        compiler = std::make_unique<PipelineCompiler>(threads);
        std::printf("GPU: new pipelines of passes drawn every frame compile on %u threads "
                    "(BB_ASYNC_PIPELINES=0: on the GPU thread, the game waits)\n",
                    threads);
    }
    auto job = std::make_shared<PendingPipeline>();
    pending_graphics.emplace(selection.graphics_key, job);
    ++async_started;
    ++g_bb_compiles;
    compiler->Submit([this, job, key = selection.graphics_key, infos = selection.infos,
                      runtime_infos = selection.runtime_infos, fetch = selection.fetch_shader,
                      modules = selection.modules]() mutable {
        GraphicsPipeline::SerializationSupport sdata{};
        auto pipeline = std::make_unique<GraphicsPipeline>(
            instance, scheduler, desc_heap, profile, key, *pipeline_cache, infos, runtime_infos,
            fetch, modules, sdata, false);
        std::scoped_lock lk{job->mutex};
        job->pipeline = std::move(pipeline);
        job->sdata = std::move(sdata);
        job->done = true;
        job->cv.notify_all();
    });
    sel.fetch_shader.reset();
    return nullptr;
}

const GraphicsPipeline* PipelineCache::FinishPipeline(const GraphicsPipelineKey& key,
                                                      PendingPipeline& job) {
    PipelineStore::SaveGraphics(key, std::hash<GraphicsPipelineKey>{}(key), job.sdata);
    NoteDriverPipelineCacheDirty();
    auto& slot = graphics_pipelines[key];
    slot = std::move(job.pipeline);
    return slot.get();
}

bool PipelineCache::AsyncSkippable(const PipelineSelection& selection, bool indirect) {
    // Only a pass drawn every frame may go without a draw for a frame or two (its render target
    // drawn to in each of the last frames, below). A one-time render compiles at once, as during
    // loading screens (a frame of fewer than 300 draws before this one). The pipeline must not
    // depend on the draw's vertex buffers (VK_EXT_vertex_input_dynamic_state). A size rule (the
    // scene's, the output's) missed most passes: many set a 16384 scissor.
    static const bool enabled = [] {
        const char* env = std::getenv("BB_ASYNC_PIPELINES");
        return !(env && env[0] == '0');
    }();
    if (!enabled || !instance.IsVertexInputDynamicState() || !selection.regs) {
        return false;
    }
    const u32 frame = BbStats::frame_number.load(std::memory_order_relaxed);
    const u64 draws = BbStats::draws.load(std::memory_order_relaxed);
    if (frame != stats_frame) {
        last_frame_draws = draws - frame_start_draws;
        frame_start_draws = draws;
        stats_frame = frame;
        const u32 second = BbStats::coarse_second.load(std::memory_order_relaxed);
        if (async_started != 0 && second - stats_printed >= 5) {
            stats_printed = second;
            std::printf("GPU: %llu pipelines compiled in the background, %llu draws went without "
                        "theirs meanwhile, %llu waited for one\n",
                        (unsigned long long)async_started, (unsigned long long)async_skipped,
                        (unsigned long long)async_waited);
            async_started = async_skipped = async_waited = 0;
        }
    }
    if (last_frame_draws < 300) {
        return false;
    }
    // A full-screen pass (one triangle or quad: lighting, fog, post-processing) waits for its
    // pipeline: going without one for a frame left whole frames grey. So does an indirect draw
    // (its size unknown here). What may go without is geometry, which shows a frame later.
    constexpr u32 FullScreenIndices = 12;
    if (indirect || selection.regs->num_indices <= FullScreenIndices) {
        return false;
    }
    // A pass drawn every frame: its render target drawn to in each of the last 8 frames (the
    // scene, the output, shadow maps, half-size and depth-only passes alike). A one-time render
    // (a face baked for a character, a texture made once) goes to a target new or used now and
    // then: compiled at once.
    constexpr u32 PerFrameStreak = 8;
    const u64 target = DrawTarget(*selection.regs);
    const auto streak = target_streaks.find(target);
    return target != 0 && streak != target_streaks.end() && streak->second.frames >= PerFrameStreak;
}

const ComputePipeline* PipelineCache::GetComputePipeline() {
    if (!RefreshComputeKey()) {
        return nullptr;
    }
    const auto [it, is_new] = compute_pipelines.try_emplace(compute_key);
    if (is_new) {
        LOG_INFO(Render_Vulkan, "Compiling compute pipeline {:#x}",
                 std::hash<ComputePipelineKey>{}(compute_key));
        CompileTimer timer;
        ComputePipeline::SerializationSupport sdata{};
        it.value() = std::make_unique<ComputePipeline>(instance, scheduler, desc_heap, profile,
                                                       *pipeline_cache, compute_key, *sel.infos[0],
                                                       sel.modules[0], sdata, false);
        PipelineStore::SaveCompute(compute_key, sdata);
        NoteDriverPipelineCacheDirty();
    }
    return it->second.get();
}

bool PipelineCache::RefreshGraphicsKey(PipelineSelection& sel) {
    std::memset(&sel.graphics_key, 0, sizeof(GraphicsPipelineKey));
    const auto& regs = *sel.regs;
    auto& key = sel.graphics_key;
    const auto& db = regs.depth_buffer;
    const bool db_enabled = db.DepthValid() || db.StencilValid();

    key.z_format = db.DepthValid() ? db.z_info.format : Gcn::DepthBuffer::ZFormat::Invalid;
    key.stencil_format =
        db.StencilValid() ? db.stencil_info.format : Gcn::DepthBuffer::StencilFormat::Invalid;
    key.depth_clamp_enable = !regs.depth_render_override.disable_viewport_clamp;
    key.depth_clip_enable = regs.clipper_control.ZclipEnable();
    key.clip_space = regs.clipper_control.clip_space;
    key.provoking_vtx_last = regs.polygon_control.provoking_vtx_last;
    key.prim_type = regs.primitive_type;
    key.polygon_mode = regs.polygon_control.PolyMode();
    key.patch_control_points =
        regs.stage_enable.hs_en ? regs.ls_hs_config.hs_input_control_points : 0;
    key.logic_op = regs.color_control.rop3;
    key.depth_samples = db_enabled ? db.NumSamples() : 1;
    key.num_samples = key.depth_samples;
    key.cb_shader_mask = regs.color_shader_mask;

    const bool color_disabled =
        regs.color_control.mode == Gcn::ColorControl::OperationMode::Disable;
    const auto bound = [&](u32 cb) {
        return !color_disabled && regs.color_buffers[cb] && regs.color_target_mask.GetMask(cb);
    };

    // The color targets the fragment stage is translated for.
    u32 num_bound = 0;
    for (u32 cb = 0; cb < Gcn::NUM_COLOR_BUFFERS; ++cb) {
        if (!bound(cb)) {
            continue;
        }
        ++num_bound;
        const auto& col_buf = regs.color_buffers[cb];
        auto& color_buffer = key.color_buffers[cb];
        color_buffer.data_format = col_buf.GetDataFmt();
        color_buffer.num_format = col_buf.GetNumberFmt();
        color_buffer.num_conversion = col_buf.GetNumberConversion();
        color_buffer.export_format = regs.color_export_format.GetFormat(cb);
        color_buffer.swizzle = col_buf.Swizzle();
        const auto& bc = regs.blend_control[cb];
        color_buffer.blend_self_scale =
            bc.enable && !col_buf.info.blend_bypass &&
            (bc.color_func == Gcn::BlendControl::BlendFunc::Min ||
             bc.color_func == Gcn::BlendControl::BlendFunc::Max) &&
            bc.color_src_factor == Gcn::BlendControl::BlendFactor::SrcColor &&
            bc.color_dst_factor == Gcn::BlendControl::BlendFactor::DstColor;
    }

    // Only potentially animated G-buffer draws may use the extra motion attachment. The
    // ordinary stage variant is needed first to inspect the vertex shader's resources.
    const bool motion_possible =
        Shader::MotionVectors::positions_address != 0 &&
        BbSettings::Get().object_motion &&
        BbSettings::Get().upscaler != BbSettings::UpscalerOff &&
        num_bound >= 5 &&
        !regs.color_buffers[Shader::MotionVectors::Output] && db.DepthValid() &&
        db.NumSamples() == 1 && !regs.IsClipDisabled() &&
        regs.stage_enable.raw == Gcn::ShaderStageEnable::VgtStages::Vs;
    sel.motion = false;
    if (!sel.worker && &sel == &this->sel && fast) {
        fast->pending_motion_possible = motion_possible;
    }

    if (!RefreshGraphicsStages(sel)) {
        return false;
    }
    if (motion_possible) {
        const auto* vs = sel.infos[static_cast<u32>(SwStage::Vertex)];
        if (vs) {
            // Shaders with a bone palette (motion_history.h). Small skeletons (weapons, props)
            // also include static world pieces: the rasterizer gives those history only while
            // their constants change.
            sel.motion = VsSelectsMotion(*vs);
        }
        // BB_MOTION_SELECT_LOG=1: each G-buffer vertex shader once, with its buffer sizes and
        // the selection, to find animated models that the size rule leaves out.
        static const bool select_log = [] {
            const char* value = std::getenv("BB_MOTION_SELECT_LOG");
            return value && value[0] == '1';
        }();
        if (select_log && vs) {
            std::string sizes;
            for (const auto& resource : vs->buffers) {
                if (resource.IsSpecial()) {
                    continue;
                }
                const auto buffer = resource.GetSharp(*vs);
                sizes += fmt::format(" {}/{}", buffer.GetSize(), buffer.GetStride());
            }
            static std::mutex log_mutex;
            static std::unordered_set<u64> logged;
            std::scoped_lock lock{log_mutex};
            if (logged.insert(vs->pgm_hash ^ std::hash<std::string>{}(sizes)).second) {
                std::printf("Motion select: vs %016llx %s, buffers (size/stride):%s\n",
                            static_cast<unsigned long long>(vs->pgm_hash),
                            sel.motion ? "ON " : "off", sizes.c_str());
            }
        }
        // BB_OBJECT_MOTION_ALL=1: the old broad selection, for visual A/B tests.
        if (AllMotionEnv()) {
            sel.motion = true;
        }
        if (sel.motion && !RefreshGraphicsStages(sel)) {
            return false;
        }
    }

    // Targets the fragment stage does not write are left out; the others get their blending,
    // write mask and sample count.
    u8 color_samples = 0;
    bool all_color_samples_same = true;
    for (u32 cb = 0; cb < key.num_color_attachments; ++cb) {
        if (!bound(cb)) {
            continue;
        }
        if ((key.mrt_mask & (1u << cb)) == 0) {
            std::memset(&key.color_buffers[cb], 0, sizeof(Shader::PsColorBuffer));
            continue;
        }
        const auto& col_buf = regs.color_buffers[cb];
        if (regs.blend_control[cb].enable && !col_buf.info.blend_bypass) {
            key.blend_controls[cb] = regs.blend_control[cb];
        }
        key.write_masks[cb] = vk::ColorComponentFlags{
            key.color_buffers[cb].swizzle.ApplyMask(regs.color_target_mask.GetMask(cb))};
        const u8 prev_color_samples = std::exchange(color_samples, col_buf.NumSamples());
        all_color_samples_same &= color_samples == prev_color_samples || prev_color_samples == 0;
        key.color_samples[cb] = color_samples;
        key.num_samples = std::max(key.num_samples, color_samples);
    }

    if (sel.motion) {
        constexpr u32 mv = Shader::MotionVectors::Output;
        key.motion_vectors = 1;
        key.mrt_mask |= 1u << mv;
        key.num_color_attachments = mv + 1;
        auto& color_buffer = key.color_buffers[mv];
        color_buffer.data_format = Gcn::DataFormat::Format32_32_32_32;
        color_buffer.num_format = Gcn::NumberFormat::Float;
        color_buffer.swizzle = Gcn::IdentityMapping;
        key.write_masks[mv] = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        key.color_samples[mv] = 1;
    }

    // Color sample counts the host cannot mix with each other or with the depth buffer's all
    // take the depth buffer's.
    if (color_samples != 0) {
        const bool depth_mismatch = db_enabled && color_samples != key.depth_samples;
        if ((!all_color_samples_same && !instance.IsMixedAnySamplesSupported()) ||
            (all_color_samples_same && depth_mismatch &&
             !instance.IsMixedDepthSamplesSupported())) {
            key.color_samples.fill(key.depth_samples);
            key.num_samples = key.depth_samples;
        }
    }
    return true;
}

bool PipelineCache::RefreshGraphicsStages(PipelineSelection& sel) {
    bool not_a_program = false;
    const auto& regs = *sel.regs;
    auto& key = sel.graphics_key;
    if (!sel.worker && &sel == &this->sel && fast) {
        fast->num_pending = 0;
        fast->pending_ok = true;
    }
    sel.fetch_shader = std::nullopt;
    sel.infos.fill(nullptr);
    sel.modules.fill(nullptr);

    Shader::Backend::Bindings binding{};
    const auto bind_stage = [&](HwStage stage_in, SwStage stage_out) -> bool {
        const auto in = static_cast<u32>(stage_in);
        const auto out = static_cast<u32>(stage_out);
        const auto* pgm = regs.stage_enable.IsStageEnabled(in) ? regs.ProgramForStage(in) : nullptr;
        // A geometry stage also runs the copy shader (the VS program).
        const auto* copy = stage_in == HwStage::Geometry && pgm ? &regs.vs_program : nullptr;
        const VAddr pgm_base = pgm ? pgm->Address<VAddr>() : 0;
        if (pgm_base == 0) {
            key.stage_hashes[out] = 0;
            sel.infos[out] = nullptr;
            return false;
        }
        Shader::ShaderParams params{.user_data = pgm->user_data};
        Program* direct_pgm = nullptr;
        DirectShaders::Snapshot direct_snap;
        if (DirectShaders::TryLookup(pgm_base, direct_snap, sel.worker != nullptr) &&
            direct_snap.cpu_immutable != 0) {
            if (copy) {
                const VAddr copy_base = copy->Address<VAddr>();
                DirectShaders::Snapshot copy_snap;
                const bool copy_ok =
                    copy_base != 0 &&
                    ((DirectShaders::TryLookup(copy_base, copy_snap, sel.worker != nullptr) &&
                      copy_snap.cpu_immutable != 0) ||
                     LooksLikeProgram(reinterpret_cast<const u32*>(copy_base), HwStage::Vertex,
                                      !sel.worker));
                if (!copy_ok) {
                    not_a_program = true;
                    key.stage_hashes[out] = 0;
                    sel.infos[out] = nullptr;
                    return false;
                }
            }
            params.code = std::span<const u32>{reinterpret_cast<const u32*>(pgm_base),
                                               direct_snap.code_dwords};
            params.hash = direct_snap.shader_hash;
            direct_pgm = direct_snap.cached_program;
        } else {
            if (!LooksLikeProgram(reinterpret_cast<const u32*>(pgm_base), stage_in, !sel.worker) ||
                (copy && (!copy->Address<u32*>() ||
                          !LooksLikeProgram(copy->Address<u32*>(), HwStage::Vertex, !sel.worker)))) {
                not_a_program = true;
                key.stage_hashes[out] = 0;
                sel.infos[out] = nullptr;
                return false;
            }
            params = Gcn::GetParams(*pgm);
            if (!sel.worker && !program_cache.contains(params.hash) &&
                !ProgramDecodes(params, stage_in)) {
                not_a_program = true;
                key.stage_hashes[out] = 0;
                sel.infos[out] = nullptr;
                return false;
            }
        }
        std::optional<Shader::Gcn::FetchShaderData> fetch_shader;
        std::tie(sel.infos[out], sel.modules[out], fetch_shader, key.stage_hashes[out]) =
            GetProgram(sel, stage_in, stage_out, params, binding, direct_pgm);
        if (!sel.infos[out]) {
            not_a_program |= !sel.worker; // its fetch shader does not decode (GetProgram)
            key.stage_hashes[out] = 0;
            return false;
        }
        if (fetch_shader) {
            sel.fetch_shader = std::move(fetch_shader);
        }
        return true;
    };

    if (!bind_stage(HwStage::Fragment, SwStage::Fragment)) {
        // No pixel shader: the clip distance discard shader may still be needed, decided from
        // this draw's registers (not a previous draw's fragment stage).
        auto& fs = sel.runtime_infos[u32(SwStage::Fragment)];
        fs.Initialize(HwStage::Fragment, SwStage::Fragment);
        fs.hw.fs.clip_distance_emulation = NeedsClipDistanceEmulation(regs, profile);
    }
    const auto* fs_info = sel.infos[static_cast<u32>(SwStage::Fragment)];
    key.mrt_mask = fs_info ? fs_info->mrt_mask : 0u;
    key.num_color_attachments = std::bit_width(key.mrt_mask);

    // The geometry stages of each hardware configuration, in selection order (the hull stage
    // before the local one: the LS output stride is in the hull stage's constants).
    // Static arrays: an initializer_list assigned here would point at a temporary array that ends
    // with its statement (clang reuses that stack: stages read as garbage).
    using Stage = std::pair<HwStage, SwStage>;
    static constexpr Stage EsGsStages[] = {{HwStage::Export, SwStage::Vertex},
                                           {HwStage::Geometry, SwStage::Geometry}};
    static constexpr Stage LsHsStages[] = {{HwStage::Hull, SwStage::TessellationControl},
                                           {HwStage::Vertex, SwStage::TessellationEval},
                                           {HwStage::Local, SwStage::Vertex}};
    static constexpr Stage LsHsEsGsStages[] = {{HwStage::Hull, SwStage::TessellationControl},
                                               {HwStage::Export, SwStage::TessellationEval},
                                               {HwStage::Local, SwStage::Vertex},
                                               {HwStage::Geometry, SwStage::Geometry}};
    using Vgt = Gcn::ShaderStageEnable::VgtStages;
    const auto mode = regs.stage_enable.raw;
    bool tessellation = false, geometry = false;
    std::span<const Stage> stages;
    switch (mode) {
    case Vgt::Vs:
        // A missing vertex program is drawn without one.
        bind_stage(HwStage::Vertex, SwStage::Vertex);
        break;
    case Vgt::EsGs:
        geometry = true;
        stages = EsGsStages;
        break;
    case Vgt::LsHs:
        tessellation = true;
        stages = LsHsStages;
        break;
    case Vgt::LsHsEsGs:
        tessellation = geometry = true;
        stages = LsHsEsGsStages;
        break;
    default:
        LOG_WARNING(Render_Vulkan, "unimplemented shader stage {}", u32(mode));
        return false;
    }
    if (tessellation && !instance.IsTessellationSupported()) {
        return false;
    }
    if (geometry) {
        if (!instance.IsGeometryStageSupported()) {
            LOG_WARNING(Render_Vulkan, "Geometry shader stage unsupported, skipping");
            return false;
        }
        if (regs.vgt_gs_mode.onchip || regs.vgt_strmout_config.raw) {
            LOG_WARNING(Render_Vulkan, "Geometry shader features unsupported, skipping");
            return false;
        }
    }
    for (const auto& [hw, sw] : stages) {
        if (!bind_stage(hw, sw)) {
            return false;
        }
    }
    if (not_a_program) {
        return false;
    }

    // Without dynamic vertex input the pipeline is specialized on the attribute formats
    // (strides stay dynamic state).
    const auto* vs_info = sel.infos[static_cast<u32>(SwStage::Vertex)];
    if (vs_info && sel.fetch_shader && !instance.IsVertexInputDynamicState()) {
        u32 vertex_binding = 0;
        for (const auto& attrib : sel.fetch_shader->attributes) {
            const auto& buffer = attrib.GetSharp(*vs_info);
            ASSERT_MSG(vertex_binding < MaxVertexBufferCount,
                       "Vertex attribute binding count exceeded limit: {} >= {}", vertex_binding,
                       MaxVertexBufferCount);
            key.vertex_buffer_formats[vertex_binding++] =
                GcnToVk::SurfaceFormat(buffer.GetDataFmt(), buffer.GetNumberFmt());
        }
    }
    return true;
}

bool PipelineCache::RefreshComputeKey() {
    Shader::Backend::Bindings binding{};
    const auto& cs = liverpool->GetCsRegs();
    const VAddr cs_base = cs.Address<VAddr>();
    if (cs_base == 0) {
        return false;
    }
    Shader::ShaderParams cs_params{.user_data = cs.user_data};
    Program* direct_pgm = nullptr;
    DirectShaders::Snapshot direct_snap;
    if (DirectShaders::TryLookup(cs_base, direct_snap, /*is_worker=*/false) &&
        direct_snap.cpu_immutable != 0) {
        cs_params.code =
            std::span<const u32>{reinterpret_cast<const u32*>(cs_base), direct_snap.code_dwords};
        cs_params.hash = direct_snap.shader_hash;
        direct_pgm = direct_snap.cached_program;
    } else {
        if (!LooksLikeProgram(reinterpret_cast<const u32*>(cs_base), HwStage::Compute, true)) {
            return false;
        }
        cs_params = Gcn::GetParams(cs);
        if (!program_cache.contains(cs_params.hash) &&
            !ProgramDecodes(cs_params, HwStage::Compute)) {
            return false;
        }
    }
    std::tie(sel.infos[0], sel.modules[0], sel.fetch_shader, compute_key.value) =
        GetProgram(sel, HwStage::Compute, SwStage::Compute, cs_params, binding, direct_pgm);
    return true;
}

vk::ShaderModule PipelineCache::CompileModule(Shader::Info& info, Shader::RuntimeInfo& runtime_info,
                                              std::span<const u32> code, size_t perm_idx,
                                              Shader::Backend::Bindings& binding) {
    LOG_INFO(Render_Vulkan, "Compiling {} shader {:#x} {}", info.hw_stage, info.pgm_hash,
             perm_idx != 0 ? "(permutation)" : "");
    DumpShader(code, info.pgm_hash, info.hw_stage, perm_idx, "bin");
    CompileTimer timer;

    // BB_SHADER_CAPTURE: the inputs as they were, for the record.
    const bool capture = BbShaderCapture::Enabled();
    std::optional<Shader::Info> in_info;
    const auto in_runtime_info = runtime_info;
    const auto in_binding = binding;
    if (capture) {
        in_info.emplace(info);
        BbShaderCapture::Begin();
    }
    const auto ir_program = Shader::TranslateProgram(code, pools, info, runtime_info, profile);
    auto spv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, ir_program, binding);
    if (capture) {
        BbShaderCapture::Write(*in_info, in_runtime_info, profile, in_binding, code, perm_idx, spv,
                               info, binding);
    }
    DumpShader(spv, info.pgm_hash, info.hw_stage, perm_idx, "spv");

    const vk::ShaderModule module = CompileSPV(spv, instance.GetDevice());
    PipelineStore::SaveShaderBinary(std::move(spv), info.pgm_hash, perm_idx);
    SetObjectName(instance.GetDevice(), module, GetShaderName(info.hw_stage, info.pgm_hash, perm_idx));
    return module;
}

PipelineCache::Result PipelineCache::LookUpProgram(PipelineSelection& sel,
                                                   const Shader::RuntimeInfo& runtime_info,
                                                   const Shader::ShaderParams& params,
                                                   Shader::Backend::Bindings& binding,
                                                   HwStage hw_stage,
                                                   const Program* direct_program) {
    // A draw-preparation worker only looks up, with its own copy of the program's Info.
    // Note: PrepareGraphicsPipeline already holds shared_lock<programs_mutex> and armed
    // runtime_fault_recover across the whole draw.
    auto& worker = *sel.worker;
    const Program* program = direct_program;
    if (!program) {
        const auto found = program_cache.find(params.hash);
        if (found == program_cache.end()) {
            worker.failed = true;
            return {};
        }
        program = found->second.get();
        DirectShaders::AttachProgramToSlot(params.Base(), params.hash,
                                           const_cast<Program*>(program));
    }
    if (!program->info_template) {
        worker.failed = true;
        return {};
    }
    auto& mru_slot = worker.info_mru[(reinterpret_cast<uintptr_t>(program) >> 6) & 63];
    PrepWorker::ProgramState* prog_state = nullptr;
    if (mru_slot.first == program && mru_slot.second != nullptr) [[likely]] {
        prog_state = mru_slot.second;
    } else {
        prog_state = &worker.infos.try_emplace(program, *program->info_template).first->second;
        mru_slot = {program, prog_state};
    }
    auto& info = prog_state->info;
    if (program->info_template->uses_paged_buffers && !info.uses_paged_buffers) [[unlikely]] {
        info = *program->info_template;
    }
    info.pgm_base = params.Base();
    info.user_data = params.user_data;
    info.RefreshFlatBuf();
    if (info.has_fetch_shader &&
        !FetchCodeDecodes(Shader::Gcn::GetFetchShaderCode(info, info.fetch_shader_sgpr_base))) {
        worker.failed = true; // the GPU thread reads back what the GPU wrote, or skips the draw
        return {};
    }
    auto spec = Shader::StageSpecialization(info, runtime_info, profile, binding);
    const auto& modules = program->modules;
    const auto it =
        (prog_state->last_perm_idx < modules.size() &&
         modules[prog_state->last_perm_idx].spec == spec)
            ? (modules.begin() + prog_state->last_perm_idx)
            : std::ranges::find(modules, spec, &Program::Module::spec);
    if (it == modules.end()) {
        worker.failed = true;
        return {};
    }
    info.AddBindings(binding);
    const size_t perm_idx = static_cast<size_t>(std::distance(modules.begin(), it));
    prog_state->last_perm_idx = perm_idx;
    worker.stages.push_back(
        {program, &info, params.hash, hw_stage, info.pgm_base, &info.flattened_ud_buf});
    return {&info, it->module, std::move(spec.fetch_shader_data),
            HashCombine(params.hash, perm_idx)};
}

PipelineCache::Result PipelineCache::GetProgram(PipelineSelection& sel, HwStage hw_stage,
                                                SwStage sw_stage,
                                                const Shader::ShaderParams& params,
                                                Shader::Backend::Bindings& binding,
                                                Program* direct_program) {
    auto runtime_info = BuildRuntimeInfo(sel, hw_stage, sw_stage);
    if (sel.worker) {
        return LookUpProgram(sel, runtime_info, params, binding, hw_stage, direct_program);
    }

    Program* resolved = direct_program;
    if (!resolved) {
        const auto found = program_cache.find(params.hash); // this thread is the only writer
        if (found == program_cache.end()) {
            auto new_program = std::make_unique<Program>(hw_stage, sw_stage, params);
            const auto start = binding;
            const auto module =
                CompileModule(new_program->info, runtime_info, params.code, 0, binding);
            auto spec =
                Shader::StageSpecialization(new_program->info, runtime_info, profile, start);
            const u64 perm_hash = HashCombine(params.hash, 0);
            PipelineStore::SaveShaderMeta(new_program->info, spec, perm_hash, 0);
            new_program->AddPermut(module, std::move(spec));
            new_program->info_template = std::make_unique<Shader::Info>(new_program->info);
            Program* program = new_program.get();
            {
                std::unique_lock lk{programs_mutex};
                program_cache.emplace(params.hash, std::move(new_program));
            }
            DirectShaders::NoteProgramCreated(params.hash, program);
            if (&sel == &this->sel) {
                NoteFastProgram(program, hw_stage, sw_stage, params.Base(), params.hash, 0);
            }
            return {&program->info, module, program->modules[0].spec.fetch_shader_data, perm_hash};
        }
        resolved = found.value().get();
        DirectShaders::AttachProgramToSlot(params.Base(), params.hash, resolved);
    }

    Program& program = *resolved;
    if (!program.info_template) {
        // Programs preloaded from the pipeline cache get their template on first use.
        std::unique_lock lk{programs_mutex};
        program.info_template = std::make_unique<Shader::Info>(program.info);
    }
    auto& info = program.info;
    info.pgm_base = params.Base(); // the inline constant buffer addresses depend on it
    info.user_data = params.user_data;
    info.RefreshFlatBuf();
    if (info.has_fetch_shader) {
        const u32* fetch = Shader::Gcn::GetFetchShaderCode(info, info.fetch_shader_sgpr_base);
        if (!sel.worker && fetch && program_write_sync) {
            program_write_sync(reinterpret_cast<VAddr>(fetch), 256);
        }
        if (!FetchCodeDecodes(fetch)) {
            SyncProgramMemory(reinterpret_cast<VAddr>(fetch), 4096);
            if (!FetchCodeDecodes(fetch)) {
                static std::atomic<u32> reported{0};
                if (reported.fetch_add(1, std::memory_order_relaxed) < 8) {
                    LOG_WARNING(Render_Vulkan,
                                "program {:#x}: its fetch shader at {} does not decode "
                                "({:#010x} {:#010x}): draw skipped",
                                params.hash, fmt::ptr(fetch), fetch[0], fetch[1]);
                }
                return {};
            }
        }
    }
    auto spec = Shader::StageSpecialization(info, runtime_info, profile, binding);

    // Consecutive draws of a program almost always use the same permutation.
    auto& modules = program.modules;
    auto it = program.last_used < modules.size() && modules[program.last_used].spec == spec
                  ? modules.begin() + program.last_used
                  : std::ranges::find(modules, spec, &Program::Module::spec);
    size_t perm_idx;
    if (it != modules.end()) {
        perm_idx = std::distance(modules.begin(), it);
        program.last_used = perm_idx;
        info.AddBindings(binding);
    } else {
        perm_idx = modules.size();
        auto new_info = Shader::Info(hw_stage, sw_stage, params);
        const auto module = CompileModule(new_info, runtime_info, params.code, perm_idx, binding);
        std::unique_lock lk{programs_mutex};
        if (new_info.uses_paged_buffers && !info.uses_paged_buffers) {
            info.uses_paged_buffers = true;
            info.buffers = new_info.buffers;
            program.info_template = std::make_unique<Shader::Info>(info);
            if (!modules.empty() && modules[0].spec.Valid()) {
                PipelineStore::SaveShaderMeta(info, modules[0].spec, HashCombine(params.hash, 0),
                                              0);
            }
        }
        spec.info = &info;
        PipelineStore::SaveShaderMeta(new_info, spec, HashCombine(params.hash, perm_idx), perm_idx);
        program.AddPermut(module, std::move(spec));
    }
    if (&sel == &this->sel) {
        NoteFastProgram(&program, hw_stage, sw_stage, params.Base(), params.hash, perm_idx);
    }
    return {&info, modules[perm_idx].module, modules[perm_idx].spec.fetch_shader_data,
            HashCombine(params.hash, perm_idx)};
}

std::string PipelineCache::GetShaderName(HwStage stage, u64 hash, std::optional<size_t> perm) {
    BbGoldenEngine::ShaderManifestInfo manifest{};
    if (BbGoldenEngine::QueryShaderManifest(hash, 0, 0, &manifest) && manifest.bundle &&
        manifest.name) {
        if (perm) {
            return fmt::format("{}_{:#018x}_{}_{}_{}", stage, hash, *perm, manifest.bundle,
                               manifest.name);
        }
        return fmt::format("{}_{:#018x}_{}_{}", stage, hash, manifest.bundle, manifest.name);
    }
    if (perm) {
        return fmt::format("{}_{:#018x}_{}", stage, hash, *perm);
    }
    return fmt::format("{}_{:#018x}", stage, hash);
}

void PipelineCache::DumpShader(std::span<const u32> code, u64 hash, HwStage stage,
                               size_t perm_idx, std::string_view ext) {
    if (!EmulatorSettings.IsDumpShaders()) {
        return;
    }
    using namespace Common::FS;
    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}.{}", GetShaderName(stage, hash, perm_idx), ext);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Create};
    file.WriteSpan(code);
}

} // namespace Vulkan
