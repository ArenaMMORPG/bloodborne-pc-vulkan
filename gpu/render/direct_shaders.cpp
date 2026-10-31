// SPDX-License-Identifier: GPL-2.0-or-later
// V4.1: Direct 752-Shader Manifest Index, Offline SPIR-V / Raw Container Loader, and Lock-Free
//       O(1) pgm_base -> {shader_hash, code_dwords, Program*} Stage-A & DrawPrep Registry.
#include "render/direct_shaders.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "gcn/regs_shader.h"

extern "C" {
std::uint32_t GXShaderLibrary_GetTotalShaderCount() noexcept;
const GXShaderManifestEntry* GXShaderLibrary_GetEntryByIndex(std::uint32_t index) noexcept;
}

namespace Vulkan::DirectShaders {

namespace {

constexpr u64 kMaxGuestVa = 1ULL << 40; // 1 TiB (40-bit PS4 VA space, matches BbMem::AddressBits = 40)
constexpr u32 kPageShift64K = 16u;
constexpr size_t kBitmapWords = static_cast<size_t>((kMaxGuestVa >> kPageShift64K) / 64ULL);

// 2,048-slot O(1) open-addressed hash table for the 752 core shaders (< 37% load factor).
constexpr size_t kManifestHashSlots = 2048;
struct ManifestHashSlot {
    u64 hash{0};
    u16 index{kNoManifestIndex};
    u16 valid{0};
};
std::array<ManifestHashSlot, kManifestHashSlots> g_manifest_by_hash{};
std::filesystem::path g_shaders_root{};
std::once_flag g_manifest_once;
std::atomic<u32> g_manifest_total{0};
std::atomic<u32> g_manifest_spv_verified{0};
std::atomic<u32> g_manifest_raw_verified{0};

// 16,384-slot open-addressed atomic hash -> Program* directory for instant linking between
// PipelineCache::WarmUp / GetProgram and newly allocated guest shader binaries.
constexpr size_t kProgramDirSlots = 16384;
struct ProgramDirEntry {
    std::atomic<u64> hash{0};
    std::atomic<Program*> program{nullptr};
};
std::array<ProgramDirEntry, kProgramDirSlots> g_program_dir{};

// 4-way set-associative lock-free table keyed by (pgm_base >> 8): 4,096 sets x 4 ways = 16,384 slots.
constexpr size_t kNumSets = 4096;
constexpr size_t kWays = 4;

struct alignas(64) DirectShaderSlot {
    std::atomic<u32> seq{0}; // odd = writer active, nonzero even = valid
    std::atomic<u32> valid{0};
    VAddr pgm_base{0};
    u64 shader_hash{0};
    u32 crc32{0};
    u32 code_dwords{0};
    u32 ucode_alloc_bytes{0};
    u16 manifest_index{kNoManifestIndex};
    u8 shader_type{0};
    u8 cpu_immutable{0};
    std::atomic<Program*> cached_program{nullptr};
    std::atomic<u64> stamp{0};
};

std::array<std::array<DirectShaderSlot, kWays>, kNumSets> g_sets{};
std::array<std::atomic<u64>, kBitmapWords> g_shader_pages_64k{};
std::mutex g_writer_mutex;
std::atomic<u64> g_stamp_counter{1};
std::atomic<u32> g_max_ucode_span{0x4000u};

// Telemetry counters.
std::atomic<u64> g_registered_binaries{0};
std::atomic<u64> g_manifest_binaries_bound{0};
std::atomic<u64> g_stage_a_direct_hits{0};
std::atomic<u64> g_worker_direct_hits{0};

[[nodiscard]] constexpr u64 MixHash64(u64 x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

[[nodiscard]] constexpr size_t SetIndexForBase(VAddr pgm_base) noexcept {
    const u64 aligned = pgm_base >> 8;
    return static_cast<size_t>((aligned ^ (aligned >> 12) ^ (aligned >> 24)) & (kNumSets - 1));
}

void MarkShaderPages64k(VAddr addr, u64 size) noexcept {
    if (size == 0 || addr >= kMaxGuestVa) {
        return;
    }
    const VAddr end = std::min<VAddr>(addr + size, kMaxGuestVa);
    if (addr >= end) {
        return;
    }
    const u64 first_page = addr >> kPageShift64K;
    const u64 last_page = (end - 1ULL) >> kPageShift64K;
    for (u64 page = first_page; page <= last_page; ++page) {
        g_shader_pages_64k[page >> 6].fetch_or(1ULL << (page & 63ULL), std::memory_order_release);
    }
}

[[nodiscard]] bool AnyShaderPage64k(VAddr addr, u64 size) noexcept {
    if (size == 0 || addr >= kMaxGuestVa) {
        return false;
    }
    const VAddr end = std::min<VAddr>(addr + size, kMaxGuestVa);
    if (addr >= end) {
        return false;
    }
    const u64 first_page = addr >> kPageShift64K;
    const u64 last_page = (end - 1ULL) >> kPageShift64K;
    const u64 first_word = first_page >> 6;
    const u64 last_word = last_page >> 6;
    if (first_word == last_word) {
        const u64 lo_mask = ~0ULL << (first_page & 63ULL);
        const u64 hi_mask = ~0ULL >> (63ULL - (last_page & 63ULL));
        return (g_shader_pages_64k[first_word].load(std::memory_order_acquire) &
                (lo_mask & hi_mask)) != 0ULL;
    }
    const u64 lo_mask = ~0ULL << (first_page & 63ULL);
    if ((g_shader_pages_64k[first_word].load(std::memory_order_acquire) & lo_mask) != 0ULL) {
        return true;
    }
    for (u64 w = first_word + 1; w < last_word; ++w) {
        if (g_shader_pages_64k[w].load(std::memory_order_acquire) != 0ULL) {
            return true;
        }
    }
    const u64 hi_mask = ~0ULL >> (63ULL - (last_page & 63ULL));
    return (g_shader_pages_64k[last_word].load(std::memory_order_acquire) & hi_mask) != 0ULL;
}

void InsertManifestKey(u64 key_hash, u16 idx) noexcept {
    size_t slot = static_cast<size_t>(MixHash64(key_hash) & (kManifestHashSlots - 1));
    for (size_t probe = 0; probe < kManifestHashSlots; ++probe) {
        auto& entry = g_manifest_by_hash[slot];
        if (!entry.valid) {
            entry.hash = key_hash;
            entry.index = idx;
            entry.valid = 1u;
            return;
        }
        if (entry.hash == key_hash) {
            return; // Keep primary index for duplicate-hash aliases (e.g. Cheap variants).
        }
        slot = (slot + 1) & (kManifestHashSlots - 1);
    }
}

[[nodiscard]] std::filesystem::path DiscoverShadersRoot() {
    namespace fs = std::filesystem;
    if (const char* env = std::getenv("BB_MANIFEST_SHADERS_DIR"); env && env[0] != '\0') {
        fs::path p{env};
        std::error_code ec;
        if (fs::exists(p / "shaders_manifest.json", ec)) {
            return p;
        }
    }
    const std::array<fs::path, 4> candidates = {
        fs::path{"/home/kekw3/Documents/BloodBorne/bb_vulkan/our_cpp_100000_source/shaders"},
        fs::current_path() / "our_cpp_100000_source" / "shaders",
        fs::current_path().parent_path() / "our_cpp_100000_source" / "shaders",
        fs::current_path() / "shaders",
    };
    std::error_code ec;
    for (const auto& c : candidates) {
        if (fs::exists(c / "shaders_manifest.json", ec)) {
            return c;
        }
    }
    return {};
}

[[nodiscard]] Program* LookupProgramDir(u64 shader_hash) noexcept {
    if (shader_hash == 0) {
        return nullptr;
    }
    size_t slot = static_cast<size_t>(MixHash64(shader_hash) & (kProgramDirSlots - 1));
    for (size_t probe = 0; probe < 32; ++probe) {
        const u64 h = g_program_dir[slot].hash.load(std::memory_order_acquire);
        if (h == shader_hash) {
            return g_program_dir[slot].program.load(std::memory_order_acquire);
        }
        if (h == 0) {
            return nullptr;
        }
        slot = (slot + 1) & (kProgramDirSlots - 1);
    }
    return nullptr;
}

void InsertProgramDir(u64 shader_hash, Program* program) noexcept {
    if (shader_hash == 0 || !program) {
        return;
    }
    size_t slot = static_cast<size_t>(MixHash64(shader_hash) & (kProgramDirSlots - 1));
    for (size_t probe = 0; probe < 32; ++probe) {
        u64 expected = g_program_dir[slot].hash.load(std::memory_order_acquire);
        if (expected == shader_hash) {
            g_program_dir[slot].program.store(program, std::memory_order_release);
            return;
        }
        if (expected == 0) {
            if (g_program_dir[slot].hash.compare_exchange_strong(
                    expected, shader_hash, std::memory_order_acq_rel, std::memory_order_acquire) ||
                expected == shader_hash) {
                g_program_dir[slot].program.store(program, std::memory_order_release);
                return;
            }
        }
        slot = (slot + 1) & (kProgramDirSlots - 1);
    }
}

} // namespace

void InitializeManifestLibrary() noexcept {
    std::call_once(g_manifest_once, [] {
        const u32 count = GXShaderLibrary_GetTotalShaderCount();
        g_manifest_total.store(count, std::memory_order_release);
        g_shaders_root = DiscoverShadersRoot();

        u32 spv_ok = 0;
        u32 raw_ok = 0;
        std::error_code ec;
        const bool have_root = !g_shaders_root.empty();

        for (u32 i = 0; i < count; ++i) {
            const GXShaderManifestEntry* e = GXShaderLibrary_GetEntryByIndex(i);
            if (!e) {
                continue;
            }
            InsertManifestKey(e->shader_hash, static_cast<u16>(i));
            const u64 lo = e->shader_hash & 0xFFFFFFFFULL;
            if (lo != e->shader_hash) {
                InsertManifestKey(lo, static_cast<u16>(i));
            }
            if (have_root && e->bundle && e->safe_name) {
                const auto bundle_dir = g_shaders_root / e->bundle;
                const auto spv_path = bundle_dir / "spv" / (std::string(e->safe_name) + ".spv");
                const auto raw_path = bundle_dir / "raw" / e->safe_name;
                if (std::filesystem::exists(spv_path, ec)) {
                    ++spv_ok;
                }
                if (std::filesystem::exists(raw_path, ec)) {
                    ++raw_ok;
                }
            }
        }
        g_manifest_spv_verified.store(spv_ok, std::memory_order_release);
        g_manifest_raw_verified.store(raw_ok, std::memory_order_release);

        std::printf("[bb_vulkan] V4.1 Direct Shader Manifest: indexed %u/%u core shaders in O(1) "
                    "(archive=%s, verified %u .spv + %u raw Shdr containers)\n",
                    count, count, have_root ? g_shaders_root.c_str() : "<embedded-manifest-only>",
                    spv_ok, raw_ok);
        std::fflush(stdout);
    });
}

u16 FindManifestIndex(u64 shader_hash) noexcept {
    InitializeManifestLibrary();
    size_t slot = static_cast<size_t>(MixHash64(shader_hash) & (kManifestHashSlots - 1));
    for (size_t probe = 0; probe < kManifestHashSlots; ++probe) {
        const auto& entry = g_manifest_by_hash[slot];
        if (!entry.valid) {
            break;
        }
        if (entry.hash == shader_hash) {
            return entry.index;
        }
        slot = (slot + 1) & (kManifestHashSlots - 1);
    }
    const u64 lo = shader_hash & 0xFFFFFFFFULL;
    if (lo != shader_hash) {
        slot = static_cast<size_t>(MixHash64(lo) & (kManifestHashSlots - 1));
        for (size_t probe = 0; probe < kManifestHashSlots; ++probe) {
            const auto& entry = g_manifest_by_hash[slot];
            if (!entry.valid) {
                break;
            }
            if (entry.hash == lo) {
                return entry.index;
            }
            slot = (slot + 1) & (kManifestHashSlots - 1);
        }
    }
    return kNoManifestIndex;
}

const GXShaderManifestEntry* FindManifestEntry(u64 shader_hash, u32 crc32,
                                               u32 code_bytes) noexcept {
    const u16 idx = FindManifestIndex(shader_hash);
    if (idx == kNoManifestIndex) {
        return nullptr;
    }
    const GXShaderManifestEntry* e = GXShaderLibrary_GetEntryByIndex(idx);
    if (!e) {
        return nullptr;
    }
    if ((crc32 == 0 || e->crc32 == crc32) && (code_bytes == 0 || e->code_bytes == code_bytes)) {
        return e;
    }
    // Fallback linear check only when duplicate-hash alias differed in crc32/code_bytes.
    const u32 total = g_manifest_total.load(std::memory_order_acquire);
    const u64 lo = shader_hash & 0xFFFFFFFFULL;
    for (u32 i = 0; i < total; ++i) {
        const GXShaderManifestEntry* cand = GXShaderLibrary_GetEntryByIndex(i);
        if (!cand) {
            continue;
        }
        if ((cand->shader_hash == shader_hash || cand->shader_hash == lo) &&
            (crc32 == 0 || cand->crc32 == crc32) &&
            (code_bytes == 0 || cand->code_bytes == code_bytes)) {
            return cand;
        }
    }
    return nullptr;
}

bool LoadManifestOfflineSpv(u16 manifest_index, std::vector<u32>& out_spv) {
    out_spv.clear();
    InitializeManifestLibrary();
    if (g_shaders_root.empty()) {
        return false;
    }
    const GXShaderManifestEntry* e = GXShaderLibrary_GetEntryByIndex(manifest_index);
    if (!e || !e->bundle || !e->safe_name) {
        return false;
    }
    const auto path = g_shaders_root / e->bundle / "spv" / (std::string(e->safe_name) + ".spv");
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        return false;
    }
    const auto byte_len = f.tellg();
    if (byte_len < 20 || (static_cast<u64>(byte_len) & 3ULL) != 0ULL) {
        return false;
    }
    out_spv.resize(static_cast<size_t>(byte_len) / sizeof(u32));
    f.seekg(0, std::ios::beg);
    if (!f.read(reinterpret_cast<char*>(out_spv.data()), byte_len)) {
        out_spv.clear();
        return false;
    }
    if (out_spv[0] != 0x07230203u) {
        out_spv.clear();
        return false;
    }
    return true;
}

bool LoadManifestRawContainer(u16 manifest_index, std::vector<u8>& out_raw) {
    out_raw.clear();
    InitializeManifestLibrary();
    if (g_shaders_root.empty()) {
        return false;
    }
    const GXShaderManifestEntry* e = GXShaderLibrary_GetEntryByIndex(manifest_index);
    if (!e || !e->bundle || !e->safe_name) {
        return false;
    }
    const auto path = g_shaders_root / e->bundle / "raw" / e->safe_name;
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        return false;
    }
    const auto byte_len = f.tellg();
    if (byte_len <= 0x2c) {
        return false;
    }
    out_raw.resize(static_cast<size_t>(byte_len));
    f.seekg(0, std::ios::beg);
    if (!f.read(reinterpret_cast<char*>(out_raw.data()), byte_len)) {
        out_raw.clear();
        return false;
    }
    return true;
}

bool TryLookup(VAddr pgm_base, Snapshot& out, bool is_worker) noexcept {
    if (pgm_base == 0 || (pgm_base & 0xFFULL) != 0ULL || pgm_base >= kMaxGuestVa) {
        return false;
    }
    auto& set = g_sets[SetIndexForBase(pgm_base)];
    for (size_t w = 0; w < kWays; ++w) {
        const auto& slot = set[w];
        const u32 s1 = slot.seq.load(std::memory_order_acquire);
        if ((s1 & 1u) != 0u || s1 == 0u || slot.valid.load(std::memory_order_relaxed) == 0u) {
            continue;
        }
        if (slot.pgm_base != pgm_base) {
            continue;
        }
        Snapshot snap{};
        snap.pgm_base = slot.pgm_base;
        snap.shader_hash = slot.shader_hash;
        snap.crc32 = slot.crc32;
        snap.code_dwords = slot.code_dwords;
        snap.ucode_alloc_bytes = slot.ucode_alloc_bytes;
        snap.manifest_index = slot.manifest_index;
        snap.shader_type = slot.shader_type;
        snap.cpu_immutable = slot.cpu_immutable;
        snap.cached_program = slot.cached_program.load(std::memory_order_acquire);
        const u32 valid_now = slot.valid.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_acquire);
        const u32 s2 = slot.seq.load(std::memory_order_acquire);
        if (s1 == s2 && valid_now != 0u && snap.pgm_base == pgm_base && snap.code_dwords != 0u) {
            if (is_worker) {
                g_worker_direct_hits.fetch_add(1, std::memory_order_relaxed);
            } else {
                g_stage_a_direct_hits.fetch_add(1, std::memory_order_relaxed);
            }
            out = snap;
            return true;
        }
    }
    return false;
}

void NoteProgramCreated(u64 shader_hash, Program* program) noexcept {
    if (shader_hash == 0 || !program) {
        return;
    }
    InsertProgramDir(shader_hash, program);
    for (auto& set : g_sets) {
        for (auto& slot : set) {
            if (slot.valid.load(std::memory_order_acquire) != 0u &&
                slot.shader_hash == shader_hash) {
                slot.cached_program.store(program, std::memory_order_release);
            }
        }
    }
}

void AttachProgramToSlot(VAddr pgm_base, u64 shader_hash, Program* program) noexcept {
    if (pgm_base == 0 || shader_hash == 0 || !program) {
        return;
    }
    InsertProgramDir(shader_hash, program);
    auto& set = g_sets[SetIndexForBase(pgm_base)];
    for (auto& slot : set) {
        if (slot.valid.load(std::memory_order_acquire) != 0u && slot.pgm_base == pgm_base &&
            slot.shader_hash == shader_hash) {
            slot.cached_program.store(program, std::memory_order_release);
            return;
        }
    }
}

void ClearCachedPrograms() noexcept {
    std::scoped_lock lock{g_writer_mutex};
    for (auto& entry : g_program_dir) {
        entry.program.store(nullptr, std::memory_order_release);
        entry.hash.store(0ULL, std::memory_order_release);
    }
    for (auto& set : g_sets) {
        for (auto& slot : set) {
            slot.cached_program.store(nullptr, std::memory_order_release);
        }
    }
}

void InvalidateRange(VAddr addr, u64 byte_size) noexcept {
    if (addr == 0 || byte_size == 0 || !AnyShaderPage64k(addr, byte_size)) {
        return;
    }
    const VAddr end = addr + byte_size;
    const u64 max_span =
        std::max<u64>(g_max_ucode_span.load(std::memory_order_acquire), 256ULL);
    std::scoped_lock lock{g_writer_mutex};

    // Fast path when `[addr - max_span, end)` spans <= 256 KiB (<= 1,024 256-byte sets, < 1 us):
    // guaranteed to visit every slot whose `[s_start, s_start + ucode_alloc_bytes)` overlaps `[addr, end)`.
    if (byte_size + max_span <= (256ULL << 10)) {
        const VAddr first_base = (addr > max_span ? (addr - max_span) : 0ULL) & ~0xFFULL;
        for (VAddr base = first_base; base < end; base += 0x100ULL) {
            auto& set = g_sets[SetIndexForBase(base)];
            for (auto& slot : set) {
                if (slot.valid.load(std::memory_order_relaxed) == 0u) {
                    continue;
                }
                const VAddr s_start = slot.pgm_base;
                const VAddr s_end = s_start + std::max<u64>(slot.ucode_alloc_bytes, 256ULL);
                if (s_start < end && addr < s_end) {
                    const u32 prev = slot.seq.load(std::memory_order_relaxed);
                    slot.seq.store(prev + 1u, std::memory_order_release);
                    slot.valid.store(0u, std::memory_order_release);
                    slot.pgm_base = 0;
                    slot.cached_program.store(nullptr, std::memory_order_release);
                    slot.seq.store(prev + 2u, std::memory_order_release);
                }
            }
        }
        return;
    }

    // Large range (> 256 KiB): sweep all 4,096 sets once.
    for (auto& set : g_sets) {
        for (auto& slot : set) {
            if (slot.valid.load(std::memory_order_relaxed) == 0u) {
                continue;
            }
            const VAddr s_start = slot.pgm_base;
            const VAddr s_end = s_start + std::max<u64>(slot.ucode_alloc_bytes, 256ULL);
            if (s_start < end && addr < s_end) {
                const u32 prev = slot.seq.load(std::memory_order_relaxed);
                slot.seq.store(prev + 1u, std::memory_order_release);
                slot.valid.store(0u, std::memory_order_release);
                slot.pgm_base = 0;
                slot.cached_program.store(nullptr, std::memory_order_release);
                slot.seq.store(prev + 2u, std::memory_order_release);
            }
        }
    }
}

Telemetry ReadTelemetry() noexcept {
    return Telemetry{
        .manifest_total = g_manifest_total.load(std::memory_order_relaxed),
        .manifest_spv_verified = g_manifest_spv_verified.load(std::memory_order_relaxed),
        .manifest_raw_verified = g_manifest_raw_verified.load(std::memory_order_relaxed),
        .registered_binaries = g_registered_binaries.load(std::memory_order_relaxed),
        .manifest_binaries_bound = g_manifest_binaries_bound.load(std::memory_order_relaxed),
        .stage_a_direct_hits = g_stage_a_direct_hits.load(std::memory_order_relaxed),
        .worker_direct_hits = g_worker_direct_hits.load(std::memory_order_relaxed),
    };
}

} // namespace Vulkan::DirectShaders

extern "C" {

void bb_direct_vk_register_shader_binary(const void* gpu_ucode_addr, uint32_t ucode_alloc_bytes,
                                         const void* /*shdr_container*/,
                                         uint32_t /*container_bytes*/,
                                         uint32_t shader_type) noexcept {
    if (!gpu_ucode_addr) {
        return;
    }
    const VAddr base = reinterpret_cast<VAddr>(gpu_ucode_addr);
    if (base == 0 || (base & 0xFFULL) != 0ULL || base >= Vulkan::DirectShaders::kMaxGuestVa) {
        return;
    }
    const u32* code = static_cast<const u32*>(gpu_ucode_addr);
    const Gcn::BinaryInfo* bin = Gcn::DirectBinaryInfo(code);
    if (!bin || bin->length == 0 || (bin->length & 3u) != 0u) {
        return;
    }
    const u32 code_dwords = bin->length / sizeof(u32);
    const u32 full_span =
        std::max<u32>(ucode_alloc_bytes, static_cast<u32>(bin->length + sizeof(Gcn::BinaryInfo)));

    Vulkan::DirectShaders::InitializeManifestLibrary();
    const u16 manifest_idx = Vulkan::DirectShaders::FindManifestIndex(bin->shader_hash);
    Vulkan::Program* existing_pgm = Vulkan::DirectShaders::LookupProgramDir(bin->shader_hash);

    Vulkan::DirectShaders::MarkShaderPages64k(base, full_span);
    u32 prev_max = Vulkan::DirectShaders::g_max_ucode_span.load(std::memory_order_relaxed);
    while (full_span > prev_max &&
           !Vulkan::DirectShaders::g_max_ucode_span.compare_exchange_weak(
               prev_max, full_span, std::memory_order_release, std::memory_order_relaxed)) {
    }

    std::scoped_lock lock{Vulkan::DirectShaders::g_writer_mutex};
    auto& set = Vulkan::DirectShaders::g_sets[Vulkan::DirectShaders::SetIndexForBase(base)];

    // Reuse matching base slot or pick empty / oldest-stamp way.
    size_t chosen = 0;
    u64 oldest_stamp = ~0ULL;
    for (size_t w = 0; w < Vulkan::DirectShaders::kWays; ++w) {
        if (set[w].valid.load(std::memory_order_relaxed) != 0u && set[w].pgm_base == base) {
            chosen = w;
            if (set[w].shader_hash == bin->shader_hash && set[w].code_dwords == code_dwords) {
                if (existing_pgm && !set[w].cached_program.load(std::memory_order_relaxed)) {
                    set[w].cached_program.store(existing_pgm, std::memory_order_release);
                }
                if (ucode_alloc_bytes > set[w].ucode_alloc_bytes) {
                    set[w].ucode_alloc_bytes = full_span;
                }
                return;
            }
            break;
        }
        if (set[w].valid.load(std::memory_order_relaxed) == 0u) {
            chosen = w;
            oldest_stamp = 0;
        } else {
            const u64 st = set[w].stamp.load(std::memory_order_relaxed);
            if (st < oldest_stamp) {
                oldest_stamp = st;
                chosen = w;
            }
        }
    }

    auto& slot = set[chosen];
    const u32 prev_seq = slot.seq.load(std::memory_order_relaxed);
    const u32 odd_seq = (prev_seq | 1u) == prev_seq ? (prev_seq + 2u) : (prev_seq + 1u);
    slot.seq.store(odd_seq, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    slot.valid.store(0u, std::memory_order_relaxed);
    slot.pgm_base = base;
    slot.shader_hash = bin->shader_hash;
    slot.crc32 = bin->crc32;
    slot.code_dwords = code_dwords;
    slot.ucode_alloc_bytes = full_span;
    slot.manifest_index = manifest_idx;
    slot.shader_type = static_cast<u8>(shader_type & 0xFFu);
    slot.cpu_immutable = 1u;
    slot.cached_program.store(existing_pgm, std::memory_order_relaxed);
    slot.stamp.store(
        Vulkan::DirectShaders::g_stamp_counter.fetch_add(1, std::memory_order_relaxed),
        std::memory_order_relaxed);
    slot.valid.store(1u, std::memory_order_release);

    std::atomic_thread_fence(std::memory_order_release);
    slot.seq.store(odd_seq + 1u, std::memory_order_release);

    Vulkan::DirectShaders::g_registered_binaries.fetch_add(1, std::memory_order_relaxed);
    if (manifest_idx != Vulkan::DirectShaders::kNoManifestIndex) {
        Vulkan::DirectShaders::g_manifest_binaries_bound.fetch_add(1, std::memory_order_relaxed);
    }
}

void bb_direct_vk_retire_shader_range(const void* gpu_addr, uint64_t byte_size) noexcept {
    if (!gpu_addr || byte_size == 0) {
        return;
    }
    Vulkan::DirectShaders::InvalidateRange(reinterpret_cast<VAddr>(gpu_addr), byte_size);
}

} // extern "C"
