// ============================================================================
// Bloodborne (CUSA03173 v1.09) — PC Native Active Workspace Test Runner
// File: 01_PC_NATIVE_ACTIVE_WORKSPACE/tests/gx_native_workspace_runner.cpp
// Standalone cross-platform test runner for `libbb_gx_engine_native.a`
// (Linux clang++ & Windows x86_64-w64-mingw32-clang++).
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "gx_eboot_external_imports.h"
#include "gx_decomp_registry.h"

struct GXShaderManifestEntry {
    const char* bundle;            // Archive name ("gxshader", "gxgui", etc.)
    const char* name;              // Original BND4 entry name
    const char* safe_name;         // Sanitized filename stem for .asm / .spv
    const char* stage;             // Stage/Role ("Pixel(PS)", "Vertex(VS)", "LocalShader(LS)", etc.)
    const char* tess_role;         // TessRole ("None", "LsCompute", "HullTcs", "Domain")
    std::uint32_t entry_id;        // BND4 entry ID
    std::uint8_t container_type;   // ShaderFileHeader type (1=VS/DS, 2=PS, 3=GS, 4=CS, 5=ES, 6=LS, 7=HS)
    std::uint64_t shader_hash;     // OrbShdr 64-bit ShaderBinaryInfo hash
    std::uint32_t crc32;           // OrbShdr CRC32
    std::uint32_t container_bytes; // Byte size of container (.vpo/.ppo/.cpo/.dpo/.hpo/.gpo)
    std::uint32_t code_bytes;      // Byte size of GCN ISA machine code
    std::uint32_t instructions;    // Decoded GCN instruction count
    std::uint32_t spirv_words;     // Translated Vulkan 1.2 SPIR-V word count
    std::uint32_t rsrc1;           // SPI_SHADER_PGM_RSRC1 / COMPUTE_PGM_RSRC1
    std::uint32_t rsrc2;           // SPI_SHADER_PGM_RSRC2 / COMPUTE_PGM_RSRC2
    std::uint16_t vgprs;           // Allocated VGPR count
    std::uint16_t sgprs;           // Allocated SGPR count
    std::uint8_t user_sgprs;       // User data SGPR count
    std::uint8_t image_count;      // Number of T# ImageBinding entries
    std::uint8_t sampler_count;    // Number of S# SamplerBinding entries
    std::uint8_t buffer_count;     // Number of V# BufferBinding entries
    std::uint32_t max_cb_dwords;   // Maximum constant buffer (non-pointer) max_dw read by shader
    std::uint32_t max_tbl_dwords;  // Maximum resource table (pointer) max_dw read by shader
    const char* primary_cb_path;   // ResourcePath of primary constant buffer V#
    const char* primary_cb_name;   // Reflected constant buffer symbol name (if present)
    const char* primary_tex_name;  // Reflected primary texture symbol name (if present)
};

extern "C" {
uint32_t GXShaderLibrary_GetTotalShaderCount();
uint32_t GXShaderLibrary_GetBundleShaderCount(const char* bundleName);
const GXShaderManifestEntry* GXShaderLibrary_GetEntryByIndex(uint32_t index);
const GXShaderManifestEntry* GXShaderLibrary_FindByName(const char* bundleName, const char* shaderName);
const GXShaderManifestEntry* GXShaderLibrary_FindByHash(uint64_t shaderHash, uint32_t crc32);
bool GXShaderLibrary_VerifyCpuBindingLayout(
    const char* bundleName,
    const char* shaderName,
    uint32_t cpuConstantBufferBytes,
    uint32_t expectedSampledImageSlots,
    uint32_t expectedSamplerSlots);
}

int main() {
    int failures = 0;

    // 1. Verify all 2,940 decompiled CPU functions in kGXDecompiledFunctions
    uint64_t totalCodeBytes = 0;
    std::vector<uintptr_t> ptrs;
    ptrs.reserve(kGXDecompiledFunctionCount);
    for (size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
        const GXDecompiledFunctionEntry& e = kGXDecompiledFunctions[i];
        if (!e.func_ptr || e.size == 0 || !e.name || !e.module_name) {
            ++failures;
        }
        if (i > 0 && kGXDecompiledFunctions[i - 1].vma + kGXDecompiledFunctions[i - 1].size > e.vma) {
            ++failures;
        }
        totalCodeBytes += e.size;
        ptrs.push_back(reinterpret_cast<uintptr_t>(e.func_ptr));
    }
    std::sort(ptrs.begin(), ptrs.end());
    if (std::adjacent_find(ptrs.begin(), ptrs.end()) != ptrs.end()) {
        ++failures;
    }
    if (totalCodeBytes != 1040700ULL) {
        ++failures;
    }

    // 2. Verify all 752 Vulkan 1.2 SPIR-V shader modules in shaders/
    const uint32_t shaderCount = GXShaderLibrary_GetTotalShaderCount();
    if (shaderCount != 752u) {
        ++failures;
    }
    uint32_t totalInsts = 0;
    for (uint32_t i = 0; i < shaderCount; ++i) {
        const GXShaderManifestEntry* e = GXShaderLibrary_GetEntryByIndex(i);
        if (!e || e->spirv_words < 5u) {
            ++failures;
            continue;
        }
        totalInsts += e->instructions;
        std::string spvPath = std::string("shaders/") + e->bundle + "/spv/" + e->safe_name + ".spv";
        FILE* fp = std::fopen(spvPath.c_str(), "rb");
        if (!fp) {
            std::string alt = std::string("../shaders/") + e->bundle + "/spv/" + e->safe_name + ".spv";
            fp = std::fopen(alt.c_str(), "rb");
        }
        if (!fp) {
            ++failures;
            continue;
        }
        uint32_t magic = 0;
        if (std::fread(&magic, sizeof(uint32_t), 1, fp) != 1 || magic != 0x07230203u) {
            ++failures;
        }
        std::fclose(fp);
    }
    if (totalInsts != 252065u) {
        ++failures;
    }

    std::printf("[PC_NATIVE_WORKSPACE] Verified %zu CPU Functions (%llu B), %zu External Imports, %u SPIR-V Shaders (%u GCN insts) -> %d failures\n",
                kGXDecompiledFunctionCount,
                static_cast<unsigned long long>(totalCodeBytes),
                kGXEbootExternalImportCount,
                shaderCount,
                totalInsts,
                failures);
    return failures == 0 ? 0 : 1;
}
