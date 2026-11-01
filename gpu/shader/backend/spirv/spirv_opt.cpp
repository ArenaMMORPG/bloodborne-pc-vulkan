// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.1: In-process SPIRV-Tools optimization pass for translated GCN shaders.

#include "shader/backend/spirv/spirv_opt.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <utility>

#include <spirv-tools/optimizer.hpp>

#include "common/logging/log.h"

namespace Shader::Backend::SPIRV {

namespace {

constexpr u32 kSpirvMagic = 0x07230203u;

std::atomic<u32> g_opt_modules{0};
std::atomic<u32> g_opt_failed{0};
std::atomic<u64> g_opt_words_before{0};
std::atomic<u64> g_opt_words_after{0};
std::atomic<u64> g_opt_total_ns{0};

bool ParseEnvFlag(const char* name, bool default_value) noexcept {
    const char* v = std::getenv(name);
    if (!v || !*v) {
        return default_value;
    }
    return !(v[0] == '0' && v[1] == '\0');
}

bool SpirvValEnabled() noexcept {
    static const bool enabled = ParseEnvFlag("BB_SPIRV_VAL", false);
    return enabled;
}

} // namespace

bool SpirvOptEnabled() noexcept {
    static const bool enabled = ParseEnvFlag("BB_SPIRV_OPT", true);
    return enabled;
}

bool OptimizeSPIRV(std::vector<u32>& spv, u64 pgm_hash) {
    if (!SpirvOptEnabled() || spv.size() < 5 || spv[0] != kSpirvMagic) {
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();
    const bool run_val = SpirvValEnabled();

    spvtools::Optimizer opt{SPV_ENV_VULKAN_1_3};
    opt.SetMessageConsumer([pgm_hash, run_val](spv_message_level_t level, const char*,
                                               const spv_position_t& pos, const char* msg) {
        if (level <= SPV_MSG_ERROR || (run_val && level <= SPV_MSG_WARNING)) {
            LOG_WARNING(Render_Vulkan, "spirv-opt ({:#x} line {}): {}", pgm_hash, pos.line,
                        msg ? msg : "");
        }
    });
    opt.RegisterPerformancePasses();

    spvtools::OptimizerOptions options;
    options.set_run_validator(run_val);

    std::vector<u32> optimized;
    optimized.reserve(spv.size());
    const bool ok = opt.Run(spv.data(), spv.size(), &optimized, options);
    const auto elapsed_ns = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0)
            .count());
    g_opt_total_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);

    if (!ok || optimized.size() < 5 || optimized[0] != kSpirvMagic) {
        g_opt_failed.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING(Render_Vulkan,
                    "SPIR-V optimization failed for shader {:#x}, keeping raw SPIR-V ({} words)",
                    pgm_hash, spv.size());
        return false;
    }

    g_opt_modules.fetch_add(1, std::memory_order_relaxed);
    g_opt_words_before.fetch_add(spv.size(), std::memory_order_relaxed);
    g_opt_words_after.fetch_add(optimized.size(), std::memory_order_relaxed);
    spv = std::move(optimized);
    return true;
}

SpirvOptStats GetAndResetSpirvOptStats() noexcept {
    SpirvOptStats s{};
    s.modules_optimized = g_opt_modules.exchange(0, std::memory_order_relaxed);
    s.modules_failed = g_opt_failed.exchange(0, std::memory_order_relaxed);
    s.words_before = g_opt_words_before.exchange(0, std::memory_order_relaxed);
    s.words_after = g_opt_words_after.exchange(0, std::memory_order_relaxed);
    s.total_ns = g_opt_total_ns.exchange(0, std::memory_order_relaxed);
    return s;
}

} // namespace Shader::Backend::SPIRV
