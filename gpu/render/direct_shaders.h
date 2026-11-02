// SPDX-License-Identifier: GPL-2.0-or-later
// V4.1: Direct 752-Shader Manifest Index, Offline SPIR-V / Raw Container Loader, and Lock-Free
//       O(1) pgm_base -> {shader_hash, code_dwords, Program*} Stage-A & DrawPrep Registry.
#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "common/types.h"

struct GXShaderManifestEntry {
    const char* bundle;
    const char* name;
    const char* safe_name;
    const char* stage;
    const char* tess_role;
    u32 entry_id;
    u8 container_type;
    u64 shader_hash;
    u32 crc32;
    u32 container_bytes;
    u32 code_bytes;
    u32 instructions;
    u32 spirv_words;
    u32 rsrc1;
    u32 rsrc2;
    u16 vgprs;
    u16 sgprs;
    u8 user_sgprs;
    u8 image_count;
    u8 sampler_count;
    u8 buffer_count;
    u32 max_cb_dwords;
    u32 max_tbl_dwords;
    const char* primary_cb_path;
    const char* primary_cb_name;
    const char* primary_tex_name;
};

namespace Vulkan {

struct Program;

namespace DirectShaders {

inline constexpr u16 kNoManifestIndex = 0xFFFFu;

struct Snapshot {
    VAddr pgm_base{0};
    u64 shader_hash{0};
    u32 crc32{0};
    u32 code_dwords{0};
    u32 ucode_alloc_bytes{0};
    u16 manifest_index{kNoManifestIndex};
    u8 shader_type{0};
    u8 cpu_immutable{0};
    Program* cached_program{nullptr};
};

struct Telemetry {
    u32 manifest_total{0};
    u32 manifest_spv_verified{0};
    u32 manifest_raw_verified{0};
    u64 registered_binaries{0};
    u64 manifest_binaries_bound{0};
    u64 stage_a_direct_hits{0};
    u64 worker_direct_hits{0};
};

/// Initializes the O(1) 752-shader manifest hash table and verifies the 6-bundle shader archive
/// (`our_cpp_100000_source/shaders/{gxshader,gxgui,gxposteffect,gxrenderershader,gxdecal,gxffxshader}`).
void InitializeManifestLibrary() noexcept;

/// O(1) lookup into the 752-shader manifest by 64-bit OrbShdr hash (with optional crc32/code_bytes
/// verification). Replaces the O(N) linear scan in GXShaderLibrary_FindByHash.
[[nodiscard]] const GXShaderManifestEntry* FindManifestEntry(u64 shader_hash, u32 crc32 = 0,
                                                             u32 code_bytes = 0) noexcept;

/// O(1) manifest index lookup (`0..751`, or `kNoManifestIndex` if material shader).
[[nodiscard]] u16 FindManifestIndex(u64 shader_hash) noexcept;

/// Loads the offline-compiled `.spv` binary for a manifest entry (`0..751`) into `out_spv`.
[[nodiscard]] bool LoadManifestOfflineSpv(u16 manifest_index, std::vector<u32>& out_spv);

/// Loads the raw `"Shdr"` container binary (`.vpo/.ppo/.cpo/...`) for a manifest entry (`0..751`).
[[nodiscard]] bool LoadManifestRawContainer(u16 manifest_index, std::vector<u8>& out_raw);

/// Lock-free (~2 ns) Stage-A & DrawPrep worker lookup by 256-byte aligned guest shader base VA.
/// When true and `out.cpu_immutable != 0`, callers skip `program_write_sync`, `DirectBinaryInfo`,
/// `SearchBinaryInfo`, `ProgramDecodes`, and (if `out.cached_program != nullptr`) `program_cache`
/// hash-map probing.
[[nodiscard]] bool TryLookup(VAddr pgm_base, Snapshot& out, bool is_worker = false) noexcept;

/// Publishes a newly loaded or compiled `Program*` into the global hash -> Program* directory and
/// attaches it directly to any live `DirectShaderSlot` with `shader_hash`.
void NoteProgramCreated(u64 shader_hash, Program* program) noexcept;

/// Attaches `program` directly to the live slot at `pgm_base` if its hash still matches.
void AttachProgramToSlot(VAddr pgm_base, u64 shader_hash, Program* program) noexcept;

/// Clears all `Program*` pointers when `PipelineCache` drops preloaded modules or shuts down.
void ClearCachedPrograms() noexcept;

/// Invalidates any registered direct shader overlapping `[addr, addr + byte_size)` (called on
/// shader destruction, GPU heap block retirement, or GPU memory writes). Fast ~1 ns no-op when
/// `[addr, addr + byte_size)` touches no 64 KiB page that ever held shader ucode.
void InvalidateRange(VAddr addr, u64 byte_size) noexcept;

/// Monotonic generation incremented whenever any registered shader binary slot is invalidated or
/// overwritten in-place. Allows `FastSelect` to verify all stage `(pgm_base, shader_hash)` pairs
/// in a single O(1) load when `pgm_base` is already matched in the register signature.
[[nodiscard]] u64 InvalidationEpoch() noexcept;

[[nodiscard]] Telemetry ReadTelemetry() noexcept;

} // namespace DirectShaders
} // namespace Vulkan
