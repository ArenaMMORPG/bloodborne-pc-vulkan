// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Steps 5.1 & 5.2: In-process SPIR-V fast-FP rewrite + SPIRV-Tools (-O) optimizer.

#include "shader/backend/spirv/spirv_opt.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <utility>
#include <vector>

#include <spirv-tools/optimizer.hpp>

#include "common/logging/log.h"

namespace Shader::Backend::SPIRV {

namespace {

constexpr u32 kSpirvMagic = 0x07230203u;

constexpr u32 kOpExecutionMode = 16u;
constexpr u32 kOpCapability = 17u;
constexpr u32 kOpTypeFloat = 22u;
constexpr u32 kOpConstant = 43u;
constexpr u32 kOpConstantNull = 46u;
constexpr u32 kOpDecorate = 71u;
constexpr u32 kOpCopyObject = 83u;
constexpr u32 kOpFMul = 133u;
constexpr u32 kOpIsNan = 156u;
constexpr u32 kOpIsInf = 157u;
constexpr u32 kOpLogicalOr = 166u;
constexpr u32 kOpSelect = 169u;
constexpr u32 kOpFOrdEqual = 180u;

constexpr u32 kDecoNoContraction = 42u;
constexpr u32 kExecModeSignedZeroInfNanPreserve = 4461u;
constexpr u32 kCapSignedZeroInfNanPreserve = 4466u;

std::atomic<u32> g_opt_modules{0};
std::atomic<u32> g_opt_failed{0};
std::atomic<u64> g_opt_words_before{0};
std::atomic<u64> g_opt_words_after{0};
std::atomic<u64> g_opt_total_ns{0};
std::atomic<u64> g_opt_nocontract_stripped{0};
std::atomic<u64> g_opt_legacy_mul_folded{0};
std::atomic<u32> g_opt_inf_nan_stripped{0};

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

enum class F32ConstKind : u8 {
    NotConst = 0,
    PosZero = 1,
    NegZero = 2,
    FiniteNonZero = 3,
    NonFinite = 4,
};

struct IdPair {
    u32 a = 0;
    u32 b = 0;
};

/// Block 5, Step 5.2 pre-pass over SPIR-V words:
/// 1. Strips OpDecorate %id NoContraction so NVIDIA's SASS backend fuses FMUL+FADD into FFMA.
/// 2. Folds provably exact V_MUL_LEGACY_F32 selects ((a==0 || b==0) ? +0.0 : a*b) whenever
///    a == b (x*x) or either operand is an f32 constant (zero or finite non-zero).
/// 3. Strips SignedZeroInfNanPreserve when the module has no OpIsNan/OpIsInf and no remaining
///    dynamic V_MUL_LEGACY_F32 select (preserving 0 * rsqrt(0) == 0 safety where needed).
bool ApplyFastFpSpirvPass(const std::vector<u32>& in, std::vector<u32>& out) {
    if (in.size() < 5 || in[0] != kSpirvMagic) {
        return false;
    }
    const u32 bound = in[3];
    if (bound == 0 || bound > 2'000'000u) {
        return false;
    }

    u32 f32_type_id = 0;
    std::vector<F32ConstKind> const_kind(bound, F32ConstKind::NotConst);
    std::vector<IdPair> fmul_ops(bound);
    std::vector<u32> feq_zero_arg(bound, 0u);
    std::vector<IdPair> lor_ops(bound);
    std::vector<u32> select_replace(bound, 0u);

    bool has_nan_or_inf_op = false;
    bool has_dynamic_legacy_mul = false;

    for (size_t pos = 5; pos < in.size();) {
        const u32 wc = in[pos] >> 16;
        const u32 op = in[pos] & 0xFFFFu;
        if (wc == 0 || pos + wc > in.size()) {
            return false;
        }

        switch (op) {
        case kOpTypeFloat:
            if (wc == 3 && in[pos + 2] == 32u && in[pos + 1] < bound) {
                f32_type_id = in[pos + 1];
            }
            break;
        case kOpConstant:
            if (wc == 4 && f32_type_id != 0 && in[pos + 1] == f32_type_id && in[pos + 2] < bound) {
                const u32 id = in[pos + 2];
                const u32 bits = in[pos + 3];
                if (bits == 0u) {
                    const_kind[id] = F32ConstKind::PosZero;
                } else if (bits == 0x80000000u) {
                    const_kind[id] = F32ConstKind::NegZero;
                } else if ((bits & 0x7f800000u) != 0x7f800000u) {
                    const_kind[id] = F32ConstKind::FiniteNonZero;
                } else {
                    const_kind[id] = F32ConstKind::NonFinite;
                }
            }
            break;
        case kOpConstantNull:
            if (wc == 3 && f32_type_id != 0 && in[pos + 1] == f32_type_id && in[pos + 2] < bound) {
                const_kind[in[pos + 2]] = F32ConstKind::PosZero;
            }
            break;
        case kOpFMul:
            if (wc == 5 && f32_type_id != 0 && in[pos + 1] == f32_type_id && in[pos + 2] < bound) {
                fmul_ops[in[pos + 2]] = {in[pos + 3], in[pos + 4]};
            }
            break;
        case kOpFOrdEqual:
            if (wc == 5 && in[pos + 2] < bound) {
                const u32 lhs = in[pos + 3];
                const u32 rhs = in[pos + 4];
                const auto is_zero = [&](u32 cid) {
                    return cid < bound && (const_kind[cid] == F32ConstKind::PosZero ||
                                           const_kind[cid] == F32ConstKind::NegZero);
                };
                if (is_zero(rhs)) {
                    feq_zero_arg[in[pos + 2]] = lhs;
                } else if (is_zero(lhs)) {
                    feq_zero_arg[in[pos + 2]] = rhs;
                }
            }
            break;
        case kOpLogicalOr:
            if (wc == 5 && in[pos + 2] < bound) {
                lor_ops[in[pos + 2]] = {in[pos + 3], in[pos + 4]};
            }
            break;
        case kOpIsNan:
        case kOpIsInf:
            has_nan_or_inf_op = true;
            break;
        case kOpSelect:
            if (wc == 6 && f32_type_id != 0 && in[pos + 1] == f32_type_id && in[pos + 2] < bound) {
                const u32 res = in[pos + 2];
                const u32 cond = in[pos + 3];
                const u32 t_id = in[pos + 4];
                const u32 f_id = in[pos + 5];
                if (cond < bound && t_id < bound && f_id < bound &&
                    const_kind[t_id] == F32ConstKind::PosZero && fmul_ops[f_id].a != 0 &&
                    lor_ops[cond].a < bound && lor_ops[cond].b < bound) {
                    const u32 z0 = feq_zero_arg[lor_ops[cond].a];
                    const u32 z1 = feq_zero_arg[lor_ops[cond].b];
                    const u32 ma = fmul_ops[f_id].a;
                    const u32 mb = fmul_ops[f_id].b;
                    if (z0 != 0 && z1 != 0 &&
                        ((z0 == ma && z1 == mb) || (z0 == mb && z1 == ma)) &&
                        ma < bound && mb < bound) {
                        const auto ka = const_kind[ma];
                        const auto kb = const_kind[mb];
                        if (ka == F32ConstKind::PosZero || ka == F32ConstKind::NegZero ||
                            kb == F32ConstKind::PosZero || kb == F32ConstKind::NegZero) {
                            select_replace[res] = t_id;
                        } else if (ma == mb || ka == F32ConstKind::FiniteNonZero ||
                                   kb == F32ConstKind::FiniteNonZero) {
                            select_replace[res] = f_id;
                        } else {
                            has_dynamic_legacy_mul = true;
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
        pos += wc;
    }

    const bool strip_inf_nan = !has_nan_or_inf_op && !has_dynamic_legacy_mul;

    out.clear();
    out.reserve(in.size());
    out.insert(out.end(), in.begin(), in.begin() + 5);

    u64 nocontract = 0;
    u64 folded = 0;
    bool stripped_mode = false;

    for (size_t pos = 5; pos < in.size();) {
        const u32 wc = in[pos] >> 16;
        const u32 op = in[pos] & 0xFFFFu;

        if (op == kOpDecorate && wc == 3 && in[pos + 2] == kDecoNoContraction) {
            ++nocontract;
            pos += wc;
            continue;
        }
        if (strip_inf_nan) {
            if (op == kOpCapability && wc == 2 && in[pos + 1] == kCapSignedZeroInfNanPreserve) {
                stripped_mode = true;
                pos += wc;
                continue;
            }
            if (op == kOpExecutionMode && wc == 4 &&
                in[pos + 2] == kExecModeSignedZeroInfNanPreserve) {
                stripped_mode = true;
                pos += wc;
                continue;
            }
        }
        if (op == kOpSelect && wc == 6 && in[pos + 2] < bound &&
            select_replace[in[pos + 2]] != 0) {
            out.push_back((4u << 16) | kOpCopyObject);
            out.push_back(in[pos + 1]);
            out.push_back(in[pos + 2]);
            out.push_back(select_replace[in[pos + 2]]);
            ++folded;
            pos += wc;
            continue;
        }

        out.insert(out.end(), in.begin() + static_cast<std::ptrdiff_t>(pos),
                   in.begin() + static_cast<std::ptrdiff_t>(pos + wc));
        pos += wc;
    }

    g_opt_nocontract_stripped.fetch_add(nocontract, std::memory_order_relaxed);
    g_opt_legacy_mul_folded.fetch_add(folded, std::memory_order_relaxed);
    if (stripped_mode) {
        g_opt_inf_nan_stripped.fetch_add(1, std::memory_order_relaxed);
    }
    return true;
}

} // namespace

bool SpirvOptEnabled() noexcept {
    static const bool enabled = ParseEnvFlag("BB_SPIRV_OPT", true);
    return enabled;
}

bool FastFpEnabled() noexcept {
    static const bool enabled = ParseEnvFlag("BB_SHADER_FAST_FP", true);
    return enabled;
}

bool OptimizeSPIRV(std::vector<u32>& spv, u64 pgm_hash) {
    if (!SpirvOptEnabled() || spv.size() < 5 || spv[0] != kSpirvMagic) {
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();
    const size_t initial_words = spv.size();
    const bool run_val = SpirvValEnabled();

    std::vector<u32> pre_pass;
    const std::vector<u32>* input = &spv;
    if (FastFpEnabled() && ApplyFastFpSpirvPass(spv, pre_pass)) {
        input = &pre_pass;
    }

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
    optimized.reserve(input->size());
    const bool ok = opt.Run(input->data(), input->size(), &optimized, options);
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
    g_opt_words_before.fetch_add(initial_words, std::memory_order_relaxed);
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
    s.nocontract_stripped = g_opt_nocontract_stripped.exchange(0, std::memory_order_relaxed);
    s.legacy_mul_folded = g_opt_legacy_mul_folded.exchange(0, std::memory_order_relaxed);
    s.inf_nan_preserve_stripped = g_opt_inf_nan_stripped.exchange(0, std::memory_order_relaxed);
    return s;
}

} // namespace Shader::Backend::SPIRV
