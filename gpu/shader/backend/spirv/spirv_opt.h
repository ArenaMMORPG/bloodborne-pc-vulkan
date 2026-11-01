// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.1: In-process SPIRV-Tools optimization pass for translated GCN shaders.
#pragma once

#include <vector>

#include "common/types.h"

namespace Shader::Backend::SPIRV {

struct SpirvOptStats {
    u32 modules_optimized = 0;
    u32 modules_failed = 0;
    u64 words_before = 0;
    u64 words_after = 0;
    u64 total_ns = 0;
    u64 nocontract_stripped = 0;
    u64 legacy_mul_folded = 0;
    u32 inf_nan_preserve_stripped = 0;
};

/// Controlled by BB_SPIRV_OPT (default 1 = enabled, 0 = raw EmitSPIRV output).
[[nodiscard]] bool SpirvOptEnabled() noexcept;

/// Block 5, Step 5.2: Controlled by BB_SHADER_FAST_FP (default 1 = unlock hardware FFMA
/// contraction, fold exact V_MUL_LEGACY_F32 squares/constants, and strip SignedZeroInfNanPreserve
/// on NaN-safe modules; 0 = strict GCN float controls).
[[nodiscard]] bool FastFpEnabled() noexcept;

/// Runs SPIRV-Tools RegisterPerformancePasses() (-O) targeting Vulkan 1.3.
/// On failure, logs a warning and leaves `spv` unmodified.
bool OptimizeSPIRV(std::vector<u32>& spv, u64 pgm_hash = 0);

/// Atomically snapshots and resets accumulated optimization counters.
[[nodiscard]] SpirvOptStats GetAndResetSpirvOptStats() noexcept;

} // namespace Shader::Backend::SPIRV
