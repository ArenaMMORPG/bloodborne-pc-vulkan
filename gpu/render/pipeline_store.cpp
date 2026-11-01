// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: saving to and preloading from the pipeline cache on disk (pipeline_store.h).

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <unordered_set>
#include <vector>

#include "common/serdes.h"
#include "core/emulator_settings.h"
#include "render/blob_store.h"
#include "render/direct_shaders.h"
#include "render/pipeline_cache.h"
#include "render/pipeline_store.h"
#include "shader/info.h"
#include "shader/specialization.h"
#include "vulkan/instance.h"

namespace Vulkan {

namespace {

BbCache::BlobStore& Store() {
    return BbCache::BlobStore::Instance();
}

std::string MetaName(u64 perm_hash) {
    return fmt::format("{:#018x}", perm_hash);
}

std::string BinaryName(u64 pgm_hash, size_t perm_idx) {
    return fmt::format("{:#018x}_{}", pgm_hash, perm_idx);
}

/// Pipeline entries: the version, graphics (0) or compute (1), the key, the support data.
template <typename Key, typename Support>
void SavePipeline(const Key& key, const Support& sdata, u32 is_compute, const std::string& name) {
    if (!Store().IsOpened()) {
        return;
    }
    Serialization::Archive ar{};
    Serialization::Writer writer{ar};
    writer.Write(PipelineStore::PipelineKeyVersion);
    writer.Write(is_compute);
    key.Serialize(ar);
    sdata.Serialize(ar);
    Store().Save(BbCache::BlobType::PipelineKey, name, ar.TakeOff());
}

/// A permutation's metadata. False when it is from another build, or a motion vertex shader:
/// those embed session-local buffer device addresses and are translated again.
bool LoadShaderMeta(Serialization::Archive& ar, Shader::Info& info,
                    Shader::StageSpecialization& spec, size_t& perm_idx) {
    Serialization::Reader reader{ar};
    u32 meta_version{}, binary_version{};
    reader.Read(meta_version);
    if (meta_version != PipelineStore::ShaderMetaVersion) {
        return false;
    }
    reader.Read(binary_version);
    if (binary_version != PipelineStore::ShaderBinaryVersion) {
        return false;
    }
    u64 perm_hash{};
    reader.Read(perm_hash);
    reader.Read(perm_idx);
    spec.Deserialize(ar);
    info.Deserialize(ar);
    return !(info.hw_stage == Shader::HwStage::Vertex && spec.runtime_info.hw.vs.motion_vectors);
}

} // namespace

namespace PipelineStore {

void SaveCompute(const ComputePipelineKey& key, const ComputePipeline::SerializationSupport& sdata) {
    SavePipeline(key, sdata, 1, fmt::format("c_{:#018x}", key.value));
}

void SaveGraphics(const GraphicsPipelineKey& key, u64 hash,
                  const GraphicsPipeline::SerializationSupport& sdata) {
    SavePipeline(key, sdata, 0, fmt::format("g_{:#018x}", hash));
}

void SaveShaderMeta(const Shader::Info& info, const Shader::StageSpecialization& spec,
                    u64 perm_hash, size_t perm_idx) {
    if (!Store().IsOpened()) {
        return;
    }
    Serialization::Archive ar;
    Serialization::Writer writer{ar};
    writer.Write(ShaderMetaVersion);
    writer.Write(ShaderBinaryVersion);
    writer.Write(perm_hash);
    writer.Write(perm_idx);
    spec.Serialize(ar);
    info.Serialize(ar);
    Store().Save(BbCache::BlobType::ShaderMeta, MetaName(perm_hash), ar.TakeOff());
}

void SaveShaderBinary(std::vector<u32>&& spv, u64 pgm_hash, size_t perm_idx) {
    if (!Store().IsOpened()) {
        return;
    }
    Store().Save(BbCache::BlobType::ShaderBinary, BinaryName(pgm_hash, perm_idx), std::move(spv));
}

} // namespace PipelineStore

void ComputePipelineKey::Serialize(Serialization::Archive& ar) const {
    Serialization::Writer{ar}.Write(value);
}

bool ComputePipelineKey::Deserialize(Serialization::Archive& ar) {
    Serialization::Reader{ar}.Read(value);
    return true;
}

void ComputePipeline::SerializationSupport::Serialize(Serialization::Archive&) const {}

bool ComputePipeline::SerializationSupport::Deserialize(Serialization::Archive&) {
    return true;
}

void GraphicsPipelineKey::Serialize(Serialization::Archive& ar) const {
    Serialization::Writer{ar}.Write(this, sizeof(*this));
}

bool GraphicsPipelineKey::Deserialize(Serialization::Archive& ar) {
    Serialization::Reader{ar}.Read(this, sizeof(*this));
    return true;
}

void GraphicsPipeline::SerializationSupport::Serialize(Serialization::Archive& ar) const {
    Serialization::Writer writer{ar};
    writer.Write(&vertex_attributes, sizeof(vertex_attributes));
    writer.Write(&vertex_bindings, sizeof(vertex_bindings));
    writer.Write(&divisors, sizeof(divisors));
    writer.Write(multisampling);
    writer.Write(tcs);
    writer.Write(tes);
    writer.Write(fragment);
}

bool GraphicsPipeline::SerializationSupport::Deserialize(Serialization::Archive& ar) {
    Serialization::Reader reader{ar};
    reader.Read(&vertex_attributes, sizeof(vertex_attributes));
    reader.Read(&vertex_bindings, sizeof(vertex_bindings));
    reader.Read(&divisors, sizeof(divisors));
    reader.Read(multisampling);
    reader.Read(tcs);
    reader.Read(tes);
    // The clip distance discard shader is stored since 2026-10-13; older entries end before it
    // (their pipelines are recreated without it, as they were before).
    if (!ar.IsEoS()) {
        reader.Read(fragment);
    }
    return true;
}

bool PipelineCache::InitDriverPipelineCache(bool load_from_store, size_t* loaded_bytes) {
    if (loaded_bytes) {
        *loaded_bytes = 0;
    }
    if (load_from_store && Store().IsOpened()) {
        std::vector<u8> driver_blob;
        Store().Load(BbCache::BlobType::ShaderProfile, "vk_driver_cache", driver_blob);
        if (driver_blob.size() >= sizeof(VkPipelineCacheHeaderVersionOne)) {
            VkPipelineCacheHeaderVersionOne hdr{};
            std::memcpy(&hdr, driver_blob.data(), sizeof(hdr));
            const auto props = instance.GetPhysicalDevice().getProperties();
            const bool valid =
                hdr.headerSize >= sizeof(VkPipelineCacheHeaderVersionOne) &&
                hdr.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
                hdr.vendorID == props.vendorID &&
                hdr.deviceID == props.deviceID &&
                std::memcmp(hdr.pipelineCacheUUID, props.pipelineCacheUUID.data(), VK_UUID_SIZE) ==
                    0;
            if (valid) {
                auto [res, cache] = instance.GetDevice().createPipelineCacheUnique({
                    .initialDataSize = driver_blob.size(),
                    .pInitialData = driver_blob.data(),
                });
                if (res == vk::Result::eSuccess) {
                    pipeline_cache = std::move(cache);
                    if (loaded_bytes) {
                        *loaded_bytes = driver_blob.size();
                    }
                    return true;
                }
                LOG_WARNING(Render_Vulkan,
                            "Driver rejected persisted VkPipelineCache ({}), creating fresh cache",
                            vk::to_string(res));
            } else {
                LOG_INFO(Render_Vulkan,
                         "Persisted VkPipelineCache UUID/device mismatch, recreating driver cache");
            }
        }
    }
    auto [cache_result, cache] = instance.GetDevice().createPipelineCacheUnique({});
    ASSERT_MSG(cache_result == vk::Result::eSuccess, "Failed to create pipeline cache: {}",
               vk::to_string(cache_result));
    pipeline_cache = std::move(cache);
    return false;
}

void PipelineCache::SaveDriverPipelineCache() {
    if (!pipeline_cache || !Store().IsOpened()) {
        return;
    }
    auto [res, bytes] = instance.GetDevice().getPipelineCacheData(*pipeline_cache);
    if (res == vk::Result::eSuccess && bytes.size() >= sizeof(VkPipelineCacheHeaderVersionOne)) {
        Store().Save(BbCache::BlobType::ShaderProfile, "vk_driver_cache", std::move(bytes));
        driver_cache_dirty.store(false, std::memory_order_relaxed);
        last_driver_cache_save_sec = static_cast<u32>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
    }
}

void PipelineCache::NoteDriverPipelineCacheDirty() {
    driver_cache_dirty.store(true, std::memory_order_relaxed);
    if (!Store().IsOpened()) {
        return;
    }
    const auto now_sec = static_cast<u32>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    if (now_sec - last_driver_cache_save_sec >= 15) {
        SaveDriverPipelineCache();
    }
}

bool PipelineCache::PreloadStage(Serialization::Archive& ar, const Shader::Info*& out_info,
                                 vk::ShaderModule& out_module, Shader::RuntimeInfo& out_runtime,
                                 std::optional<Shader::Gcn::FetchShaderData>& out_fetch) {
    auto program = std::make_unique<Program>();
    Shader::StageSpecialization spec{};
    spec.info = &program->info;
    size_t perm_idx{};
    if (!LoadShaderMeta(ar, program->info, spec, perm_idx)) {
        return false;
    }
    if (profile.paged_buffers && program->info.pgm_hash == profile.buffer_copy_shader_hash &&
        (!program->info.uses_paged_buffers || perm_idx != 0 || spec.buffers.size() < 3 ||
         !spec.buffers[1].paged || !spec.buffers[2].paged)) {
        return false;
    }
    if (spec.fetch_shader_data) {
        out_fetch = spec.fetch_shader_data; // the vertex stage's (as when selected)
    }
    out_runtime = spec.runtime_info;

    std::vector<u32> spv{};
    Store().Load(BbCache::BlobType::ShaderBinary, BinaryName(program->info.pgm_hash, perm_idx),
                 spv);
    // SPIR-V starts with its magic number and a 5-word header; anything else is a damaged file
    // (a crash or power loss while it was written) the driver must not see.
    if (spv.size() < 5 || spv[0] != 0x07230203u) {
        if (!spv.empty()) {
            throw Serialization::CorruptData{"damaged SPIR-V in the shader cache"};
        }
        return false;
    }
    // A module the driver rejects is a damaged entry (the cache is rebuilt), not a fatal error.
    const auto compile = [&] {
        auto [result, created] = instance.GetDevice().createShaderModule(
            {.codeSize = spv.size() * sizeof(u32), .pCode = spv.data()});
        if (result != vk::Result::eSuccess) {
            throw Serialization::CorruptData{"cached SPIR-V rejected by the driver"};
        }
        return created;
    };

    vk::ShaderModule module{};
    auto [it_pgm, new_program] = program_cache.try_emplace(program->info.pgm_hash);
    if (new_program) {
        module = compile();
        it_pgm.value() = std::move(program);
        spec.info = &it_pgm.value()->info;
        it_pgm.value()->info_template = std::make_unique<Shader::Info>(it_pgm.value()->info);
        DirectShaders::NoteProgramCreated(it_pgm.value()->info.pgm_hash, it_pgm.value().get());
    } else {
        auto& existing = *it_pgm.value();
        // Merge paged-buffer layout when a later permutation upgraded uses_paged_buffers while
        // permutation 0's on-disk .meta predated the upgrade.
        if (program->info.uses_paged_buffers && !existing.info.uses_paged_buffers) {
            existing.info.uses_paged_buffers = true;
            existing.info.buffers = program->info.buffers;
            existing.info_template = std::make_unique<Shader::Info>(existing.info);
        }
        // Always rebind spec.info to the persistent Program in program_cache so it never dangles
        // after the local `program` unique_ptr goes out of scope.
        spec.info = &existing.info;

        auto& modules = existing.modules;
        if (perm_idx < modules.size() && modules[perm_idx].module) {
            module = modules[perm_idx].module;
        } else {
            const auto it = std::ranges::find(modules, spec, &Program::Module::spec);
            if (it == modules.end()) {
                module = compile();
            } else {
                const size_t idx = std::distance(modules.begin(), it);
                if (perm_idx != idx) {
                    LOG_WARNING(Render_Vulkan,
                                "Cached permutation {} of {}_{:x} conflicts with index {}, "
                                "skipping preload",
                                perm_idx, program->info.hw_stage, program->info.pgm_hash, idx);
                    return false;
                }
                module = it->module;
            }
        }
    }
    it_pgm.value()->InsertPermut(module, std::move(spec), perm_idx);
    out_info = &it_pgm.value()->info;
    out_module = module;
    return true;
}

namespace {
/// The cache's header: a tag and the versions; the device profile follows.
constexpr std::array<u32, 4> CacheVersions{0x42425043u, PipelineStore::ShaderBinaryVersion,
                                           PipelineStore::ShaderMetaVersion,
                                           PipelineStore::PipelineKeyVersion};
constexpr size_t HeaderSize = sizeof(CacheVersions);

struct PreloadComputeItem {
    ComputePipelineKey key{};
    ComputePipeline::SerializationSupport sdata{};
    const Shader::Info* info = nullptr;
    vk::ShaderModule module{};
    std::unique_ptr<ComputePipeline> built;
};

struct PreloadGraphicsItem {
    GraphicsPipelineKey key{};
    GraphicsPipeline::SerializationSupport sdata{};
    std::array<const Shader::Info*, MaxShaderStages> infos{};
    std::array<Shader::RuntimeInfo, MaxShaderStages> runtime_infos{};
    std::optional<Shader::Gcn::FetchShaderData> fetch_shader{};
    std::array<vk::ShaderModule, MaxShaderStages> modules{};
    std::unique_ptr<GraphicsPipeline> built;
};
} // namespace

void PipelineCache::SaveProfile() {
    std::vector<u8> data(HeaderSize + sizeof(profile));
    std::memcpy(data.data(), CacheVersions.data(), HeaderSize);
    std::memcpy(data.data() + HeaderSize, &profile, sizeof(profile));
    Store().Save(BbCache::BlobType::ShaderProfile, "profile", std::move(data));
}

void PipelineCache::DropPreloaded() {
    // Modules of a damaged entry may sit in programs that later lookups would reuse: nothing
    // preloaded is kept.
    InvalidateFastSelect();
    graphics_pipelines.clear();
    compute_pipelines.clear();
    std::unordered_set<VkShaderModule> modules;
    for (const auto& [_, program] : program_cache) {
        if (!program) {
            continue;
        }
        for (const auto& permutation : program->modules) {
            if (permutation.module) {
                modules.insert(VkShaderModule(permutation.module));
            }
        }
    }
    for (const VkShaderModule module : modules) {
        instance.GetDevice().destroyShaderModule(vk::ShaderModule{module});
    }
    program_cache.clear();
    DirectShaders::ClearCachedPrograms();
}

void PipelineCache::WarmUp() {
    const auto t0 = std::chrono::steady_clock::now();
    DirectShaders::InitializeManifestLibrary();
    if (!EmulatorSettings.IsPipelineCacheEnabled()) {
        InitDriverPipelineCache(false);
        return;
    }
    Store().Open();
    const auto rebuild = [&] {
        InitDriverPipelineCache(false);
        Store().Clear();
        Store().FinishPreload();
        SaveProfile();
    };

    std::vector<u8> header{};
    Store().Load(BbCache::BlobType::ShaderProfile, "profile", header);
    if (header.empty()) {
        rebuild();
        return;
    }
    bool compatible = header.size() == HeaderSize + sizeof(Shader::Profile) &&
                      std::memcmp(header.data(), CacheVersions.data(), HeaderSize) == 0;
    if (compatible) {
        Shader::Profile cached{};
        std::memcpy(&cached, header.data() + HeaderSize, sizeof(cached));
        compatible = cached == profile;
    }
    if (!compatible) {
        LOG_WARNING(Render,
                    "Pipeline cache isn't compatible with current compiler/system: rebuilding it");
        rebuild();
        return;
    }

    size_t loaded_driver_bytes = 0;
    const bool had_driver_cache = InitDriverPipelineCache(true, &loaded_driver_bytes);

    // Phase 1: Deserialize pipeline keys, deduplicate SPIR-V VkShaderModules into program_cache,
    // and merge all multi-permutation Shader::Info layouts on the main thread so Program::info
    // pointers are stable and immutable during Phase 2 parallel PSO creation.
    std::vector<PreloadComputeItem> compute_items;
    std::vector<PreloadGraphicsItem> graphics_items;
    std::unordered_set<ComputePipelineKey> seen_compute;
    std::unordered_set<GraphicsPipelineKey> seen_graphics;
    u32 num_total = 0, num_damaged = 0;

    Store().ForEachBlob(BbCache::BlobType::PipelineKey, [&](std::vector<u8>&& data) {
        ++num_total;
        try {
            Serialization::Archive ar{std::move(data)};
            Serialization::Reader reader{ar};
            u32 version{}, is_compute{};
            reader.Read(version);
            if (version != PipelineStore::PipelineKeyVersion) {
                return;
            }
            reader.Read(is_compute);
            if (is_compute) {
                PreloadComputeItem item{};
                item.key.Deserialize(ar);
                item.sdata.Deserialize(ar);
                if (seen_compute.contains(item.key)) {
                    return;
                }
                std::vector<u8> meta;
                Store().Load(BbCache::BlobType::ShaderMeta, MetaName(item.key.value), meta);
                if (meta.empty()) {
                    return;
                }
                Serialization::Archive meta_ar{std::move(meta)};
                Shader::RuntimeInfo rt{};
                std::optional<Shader::Gcn::FetchShaderData> fetch{};
                if (!PreloadStage(meta_ar, item.info, item.module, rt, fetch)) {
                    return;
                }
                seen_compute.insert(item.key);
                compute_items.push_back(std::move(item));
            } else {
                PreloadGraphicsItem item{};
                item.key.Deserialize(ar);
                item.sdata.Deserialize(ar);
                if (seen_graphics.contains(item.key)) {
                    return;
                }
                for (u32 stage = 0; stage < MaxShaderStages; ++stage) {
                    const u64 hash = item.key.stage_hashes[stage];
                    if (!hash) {
                        continue;
                    }
                    std::vector<u8> meta;
                    Store().Load(BbCache::BlobType::ShaderMeta, MetaName(hash), meta);
                    if (meta.empty()) {
                        return;
                    }
                    Serialization::Archive meta_ar{std::move(meta)};
                    if (!PreloadStage(meta_ar, item.infos[stage], item.modules[stage],
                                      item.runtime_infos[stage], item.fetch_shader)) {
                        return;
                    }
                }
                seen_graphics.insert(item.key);
                graphics_items.push_back(std::move(item));
            }
        } catch (const std::exception& e) {
            if (num_damaged++ == 0) {
                LOG_WARNING(Render, "Pipeline cache: damaged entry ({})", e.what());
            }
        }
    });

    if (num_damaged) {
        LOG_WARNING(Render, "Pipeline cache: {} damaged entries, rebuilding it", num_damaged);
        DropPreloaded();
        rebuild();
        return;
    }

    // Phase 2: Parallel Vulkan graphics & compute PSO compilation against thread-safe
    // VkPipelineCache across worker threads.
    const size_t num_compute = compute_items.size();
    const size_t num_graphics = graphics_items.size();
    const size_t total_items = num_compute + num_graphics;
    const u32 hw_threads = std::max(1u, std::thread::hardware_concurrency());
    const u32 num_workers =
        total_items < 8
            ? 1u
            : std::min<u32>(std::clamp(hw_threads, 2u, 12u), static_cast<u32>(total_items));

    std::atomic<size_t> next_idx{0};
    std::atomic<u32> parallel_damaged{0};
    const auto worker_fn = [&] {
        for (;;) {
            if (parallel_damaged.load(std::memory_order_relaxed) != 0) {
                return;
            }
            const size_t idx = next_idx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= total_items) {
                return;
            }
            try {
                if (idx < num_compute) {
                    auto& item = compute_items[idx];
                    item.built = std::make_unique<ComputePipeline>(
                        instance, scheduler, desc_heap, profile, *pipeline_cache, item.key,
                        *item.info, item.module, item.sdata, true);
                } else {
                    auto& item = graphics_items[idx - num_compute];
                    item.built = std::make_unique<GraphicsPipeline>(
                        instance, scheduler, desc_heap, profile, item.key, *pipeline_cache,
                        item.infos, item.runtime_infos, item.fetch_shader, item.modules,
                        item.sdata, true);
                }
            } catch (const std::exception& e) {
                if (parallel_damaged.fetch_add(1, std::memory_order_relaxed) == 0) {
                    LOG_WARNING(Render, "Pipeline cache: damaged PSO during parallel warmup ({})",
                                e.what());
                }
                return;
            }
        }
    };

    if (num_workers <= 1) {
        worker_fn();
    } else {
        std::vector<std::thread> workers;
        workers.reserve(num_workers);
        for (u32 t = 0; t < num_workers; ++t) {
            workers.emplace_back(worker_fn);
        }
        for (auto& w : workers) {
            w.join();
        }
    }

    if (parallel_damaged.load(std::memory_order_relaxed) != 0) {
        LOG_WARNING(Render, "Pipeline cache: {} damaged PSOs, rebuilding cache",
                    parallel_damaged.load());
        compute_items.clear();
        graphics_items.clear();
        DropPreloaded();
        rebuild();
        return;
    }

    // Phase 3: Commit prewarmed PSOs into PipelineCache maps and persist driver VkPipelineCache.
    compute_pipelines.reserve(num_compute);
    for (auto& item : compute_items) {
        if (item.built) {
            compute_pipelines.emplace(item.key, std::move(item.built));
        }
    }
    graphics_pipelines.reserve(num_graphics);
    for (auto& item : graphics_items) {
        if (item.built) {
            graphics_pipelines.emplace(item.key, std::move(item.built));
        }
    }

    if (!had_driver_cache && total_items > 0) {
        SaveDriverPipelineCache();
    }

    const u32 num_pipelines =
        static_cast<u32>(compute_pipelines.size() + graphics_pipelines.size());
    const double elapsed_ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("[bb_vulkan] V4.2 Prewarmed %zu graphics + %zu compute PSOs (%zu programs) across "
                "%u threads in %.1f ms (VkPipelineCache=%s, %.2f MiB)\n",
                graphics_pipelines.size(), compute_pipelines.size(), program_cache.size(),
                num_workers, elapsed_ms, had_driver_cache ? "HIT" : "SEEDED",
                double(loaded_driver_bytes) / (1024.0 * 1024.0));
    LOG_INFO(Render, "Preloaded {} pipelines ({} graphics, {} compute) in {:.1f} ms",
             num_pipelines, graphics_pipelines.size(), compute_pipelines.size(), elapsed_ms);
    if (num_total > num_pipelines) {
        LOG_WARNING(Render, "{} stale pipelines were found. Consider re-generating the cache",
                    num_total - num_pipelines);
    }
    Store().FinishPreload();
}

void PipelineCache::Sync() {
    if (driver_cache_dirty.load(std::memory_order_relaxed)) {
        SaveDriverPipelineCache();
    }
    Store().Close();
}

} // namespace Vulkan
