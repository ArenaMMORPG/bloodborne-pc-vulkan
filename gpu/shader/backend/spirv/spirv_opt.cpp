// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Steps 5.1, 5.2 & 5.3: In-process SPIR-V hardware-interpolation rewrite,
// fast-FP rewrite, and SPIRV-Tools (-O) optimizer.

#include "shader/backend/spirv/spirv_opt.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

#include <spirv-tools/libspirv.h>
#include <spirv-tools/optimizer.hpp>

#include "common/logging/log.h"

namespace Shader::Backend::SPIRV {

namespace {

constexpr u32 kSpirvMagic = 0x07230203u;

// SPIR-V core opcodes
constexpr u32 kOpName = 5u;
constexpr u32 kOpExtension = 10u;
constexpr u32 kOpExtInstImport = 11u;
constexpr u32 kOpExtInst = 12u;
constexpr u32 kOpEntryPoint = 15u;
constexpr u32 kOpExecutionMode = 16u;
constexpr u32 kOpCapability = 17u;
constexpr u32 kOpTypeInt = 21u;
constexpr u32 kOpTypeFloat = 22u;
constexpr u32 kOpTypeArray = 28u;
constexpr u32 kOpTypePointer = 32u;
constexpr u32 kOpConstant = 43u;
constexpr u32 kOpConstantNull = 46u;
constexpr u32 kOpFunction = 54u;
constexpr u32 kOpFunctionEnd = 56u;
constexpr u32 kOpVariable = 59u;
constexpr u32 kOpLoad = 61u;
constexpr u32 kOpAccessChain = 65u;
constexpr u32 kOpDecorate = 71u;
constexpr u32 kOpCompositeExtract = 81u;
constexpr u32 kOpCopyObject = 83u;
constexpr u32 kOpConvertFToU = 109u;
constexpr u32 kOpConvertFToS = 110u;
constexpr u32 kOpFSub = 131u;
constexpr u32 kOpFMul = 133u;
constexpr u32 kOpIsNan = 156u;
constexpr u32 kOpIsInf = 157u;
constexpr u32 kOpLogicalOr = 166u;
constexpr u32 kOpSelect = 169u;
constexpr u32 kOpFOrdEqual = 180u;

// SPIR-V storage classes, decorations, capabilities, builtins & execution modes
constexpr u32 kStorageClassInput = 1u;
constexpr u32 kDecoBuiltIn = 11u;
constexpr u32 kDecoNoPerspective = 13u;
constexpr u32 kDecoFlat = 14u;
constexpr u32 kDecoSample = 17u;
constexpr u32 kDecoNoContraction = 42u;
constexpr u32 kDecoPerVertexKHR = 5285u;

constexpr u32 kBuiltInBaryCoordKHR = 5286u;
constexpr u32 kBuiltInBaryCoordNoPerspKHR = 5287u;

constexpr u32 kCapInterpolationFunction = 52u;
constexpr u32 kCapSignedZeroInfNanPreserve = 4466u;
constexpr u32 kCapFragmentBarycentricKHR = 5284u;

constexpr u32 kExecModeSignedZeroInfNanPreserve = 4461u;

// GLSL.std.450 extended instruction opcodes
constexpr u32 kGlslTrunc = 3u;
constexpr u32 kGlslFloor = 8u;
constexpr u32 kGlslFMin = 37u;
constexpr u32 kGlslFMax = 40u;
constexpr u32 kGlslFma = 50u;
constexpr u32 kGlslInterpolateAtCentroid = 76u;
constexpr u32 kGlslInterpolateAtSample = 77u;
constexpr u32 kGlslInterpolateAtOffset = 78u;
constexpr u32 kGlslNMin = 79u;
constexpr u32 kGlslNMax = 80u;

// 1.0f + 1.0f / 65536.0f (exact IEEE-754 f32 bits: 0x3f800080), matching InterpolatedIntegerPass.
constexpr u32 kF32InterpIntNudgeBits = 0x3f800080u;

std::atomic<u32> g_opt_modules{0};
std::atomic<u32> g_opt_failed{0};
std::atomic<u64> g_opt_words_before{0};
std::atomic<u64> g_opt_words_after{0};
std::atomic<u64> g_opt_total_ns{0};
std::atomic<u64> g_opt_nocontract_stripped{0};
std::atomic<u64> g_opt_legacy_mul_folded{0};
std::atomic<u32> g_opt_inf_nan_stripped{0};
std::atomic<u32> g_opt_hw_interp_modules{0};
std::atomic<u64> g_opt_hw_interp_varyings{0};
std::atomic<u64> g_opt_hw_interp_channels{0};
std::atomic<u64> g_opt_hw_interp_int_nudges{0};
std::atomic<u64> g_opt_self_minmax_folded{0};

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

bool MatchSpvLiteralString(const u32* words, u32 num_words, const char* target) noexcept {
    if (num_words == 0) {
        return false;
    }
    const char* bytes = reinterpret_cast<const char*>(words);
    const size_t max_bytes = static_cast<size_t>(num_words) * sizeof(u32);
    const void* nul = std::memchr(bytes, '\0', max_bytes);
    if (!nul) {
        return false;
    }
    return std::strcmp(bytes, target) == 0;
}

enum class InterpMode : u8 {
    Keep = 0,
    Smooth = 1,
    NoPerspective = 2,
    Flat = 3,
};

struct ParsedSpvInst {
    u32 offset = 0;
    u16 wc = 0;
    u16 opcode = 0;
    u32 type_id = 0;
    u32 result_id = 0;
    u32 id_op_begin = 0;
    u16 id_op_count = 0;
    bool in_func = false;
};

struct PerVertexAc {
    u32 var_id = 0;
    u32 vtx_idx = 0xFFFFFFFFu;
    u32 comp_const_id = 0;
    u32 elem_ptr_type = 0;
    bool has_comp = false;
};

struct SubPair {
    u32 hi_ld = 0; // vtx 1 or vtx 2 load
    u32 lo_ld = 0; // vtx 0 load
    u32 var_id = 0;
    u32 vtx_hi = 0;
    u32 comp_const_id = 0;
    u32 elem_ptr_type = 0;
    bool has_comp = false;
};

struct P1FmaInfo {
    u32 var_id = 0;
    u32 comp_const_id = 0;
    u32 elem_ptr_type = 0;
    bool has_comp = false;
    InterpMode mode = InterpMode::Keep;
};

struct P2FmaInfo {
    u32 var_id = 0;
    u32 comp_const_id = 0;
    u32 elem_ptr_type = 0;
    bool has_comp = false;
    InterpMode mode = InterpMode::Keep;
};

struct BaryCompInfo {
    InterpMode mode = InterpMode::Keep;
    u8 axis = 0; // 1 = I (bary.y), 2 = J (bary.z)
};

/// Block 5, Step 5.3 pre-pass over SPIR-V words:
/// Converts FragmentBarycentricKHR / PerVertexKHR software 3-vertex barycentric interpolation
/// (V_INTERP_P1_F32 + V_INTERP_P2_F32 -> 4x OpAccessChain + 4x OpLoad + 2x OpFSub + 2x Fma per
/// scalar channel) into fixed-function hardware rasterizer IPA interpolation (Smooth /
/// NoPerspective), converts vertex-0-only V_INTERP_MOV_F32 PerVertexKHR inputs into Flat inputs,
/// and applies InterpolatedIntegerPass's 1 + 2^-16 scaling nudge before integer truncations of
/// smooth/nopersp interpolated channels so instanced object IDs truncate identically on NVIDIA.
bool ApplyHwInterpSpirvPass(const std::vector<u32>& in, std::vector<u32>& out) {
    if (in.size() < 5 || in[0] != kSpirvMagic) {
        return false;
    }
    const u32 bound = in[3];
    if (bound == 0 || bound > 2'000'000u) {
        return false;
    }

    // Fast header scan: skip non-FragmentBarycentricKHR modules immediately (~0.1 us).
    bool has_bary_cap = false;
    for (size_t pos = 5; pos < in.size();) {
        const u32 wc = in[pos] >> 16;
        const u32 op = in[pos] & 0xFFFFu;
        if (wc == 0 || pos + wc > in.size()) {
            return false;
        }
        if (op == kOpCapability) {
            if (wc == 2 && in[pos + 1] == kCapFragmentBarycentricKHR) {
                has_bary_cap = true;
                break;
            }
        } else if (op == kOpFunction) {
            break;
        }
        pos += wc;
    }
    if (!has_bary_cap) {
        return false;
    }

    // Parse instruction stream once with SPIRV-Tools so SPV_OPERAND_TYPE_ID operands are
    // distinguished cleanly from literal numbers (e.g. GLSLstd450Fma = 50 vs %50 = BaryCoordKHR).
    struct ParseState {
        const u32* base = nullptr;
        u32 bound = 0;
        bool in_func = false;
        std::vector<ParsedSpvInst> insts;
        std::vector<u32> id_operands;
        std::vector<u32> total_uses;
    } ps;
    ps.base = in.data();
    ps.bound = bound;
    ps.insts.reserve(in.size() / 4);
    ps.id_operands.reserve(in.size() / 2);
    ps.total_uses.assign(bound, 0u);

    static const spv_context spv_ctx = spvContextCreate(SPV_ENV_VULKAN_1_3);
    const spv_result_t parse_res = spvBinaryParse(
        spv_ctx, &ps, in.data(), in.size(), nullptr,
        [](void* user_data, const spv_parsed_instruction_t* inst) -> spv_result_t {
            auto& st = *static_cast<ParseState*>(user_data);
            if (inst->opcode == kOpFunction) {
                st.in_func = true;
            }
            ParsedSpvInst pi{};
            pi.offset = static_cast<u32>(inst->words - st.base);
            pi.wc = inst->num_words;
            pi.opcode = inst->opcode;
            pi.type_id = inst->type_id;
            pi.result_id = inst->result_id;
            pi.id_op_begin = static_cast<u32>(st.id_operands.size());
            pi.in_func = st.in_func && inst->opcode != kOpFunction && inst->opcode != kOpFunctionEnd;

            u16 id_cnt = 0;
            for (u16 i = 0; i < inst->num_operands; ++i) {
                const auto& op = inst->operands[i];
                if (op.type == SPV_OPERAND_TYPE_ID ||
                    op.type == SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID ||
                    op.type == SPV_OPERAND_TYPE_SCOPE_ID) {
                    const u32 id = inst->words[op.offset];
                    st.id_operands.push_back(id);
                    ++id_cnt;
                    if (pi.in_func && id < st.bound) {
                        ++st.total_uses[id];
                    }
                }
            }
            pi.id_op_count = id_cnt;
            st.insts.push_back(pi);

            if (inst->opcode == kOpFunctionEnd) {
                st.in_func = false;
            }
            return SPV_SUCCESS;
        },
        nullptr);
    if (parse_res != SPV_SUCCESS) {
        return false;
    }

    u32 glsl_set_id = 0;
    u32 f32_type_id = 0;
    u32 f32_type_inst_offset = 0;
    u32 u32_type_id = 0;
    u32 existing_nudge_const_id = 0;
    bool keep_interp_func_cap = false;

    std::vector<u8> has_u32_const(bound, 0u);
    std::vector<u32> u32_const_val(bound, 0u);
    std::vector<u32> arr3_elem_type(bound, 0u);
    std::vector<u32> ptr_input_pointee(bound, 0u);
    std::vector<u32> input_ptr_for_pointee(bound, 0u);
    std::vector<u8> is_per_vertex_var(bound, 0u);
    std::vector<InterpMode> bary_var_mode(bound, InterpMode::Keep);
    std::vector<u32> var_elem_type(bound, 0u);
    std::vector<u32> var_old_ptr_type(bound, 0u);
    std::vector<u32> ptr_type_inst_offset(bound, 0u);

    std::vector<u32> per_vertex_var_list;
    std::vector<u32> bary_var_list;

    // Pass 1: Gather types, constants, decorations, and Input variables.
    for (const auto& pi : ps.insts) {
        const u32* w = &in[pi.offset];
        switch (pi.opcode) {
        case kOpExtInstImport:
            if (pi.wc >= 3 && MatchSpvLiteralString(w + 2, pi.wc - 2, "GLSL.std.450")) {
                glsl_set_id = w[1];
            }
            break;
        case kOpTypeFloat:
            if (pi.wc == 3 && w[2] == 32u && w[1] < bound) {
                f32_type_id = w[1];
                f32_type_inst_offset = pi.offset;
            }
            break;
        case kOpTypeInt:
            if (pi.wc == 4 && w[2] == 32u && w[3] == 0u && w[1] < bound) {
                u32_type_id = w[1];
            }
            break;
        case kOpConstant:
            if (pi.wc == 4 && w[2] < bound) {
                if (u32_type_id != 0 && w[1] == u32_type_id) {
                    has_u32_const[w[2]] = 1u;
                    u32_const_val[w[2]] = w[3];
                } else if (f32_type_id != 0 && w[1] == f32_type_id &&
                           w[3] == kF32InterpIntNudgeBits) {
                    existing_nudge_const_id = w[2];
                }
            }
            break;
        case kOpConstantNull:
            if (pi.wc == 3 && u32_type_id != 0 && w[1] == u32_type_id && w[2] < bound) {
                has_u32_const[w[2]] = 1u;
                u32_const_val[w[2]] = 0u;
            }
            break;
        case kOpTypeArray:
            if (pi.wc == 4 && w[1] < bound && w[2] < bound && w[3] < bound &&
                has_u32_const[w[3]] && u32_const_val[w[3]] == 3u) {
                arr3_elem_type[w[1]] = w[2];
            }
            break;
        case kOpTypePointer:
            if (pi.wc == 4 && w[2] == kStorageClassInput && w[1] < bound && w[3] < bound) {
                ptr_input_pointee[w[1]] = w[3];
                ptr_type_inst_offset[w[1]] = pi.offset;
                if (input_ptr_for_pointee[w[3]] == 0) {
                    input_ptr_for_pointee[w[3]] = w[1];
                }
            }
            break;
        case kOpDecorate:
            if (pi.wc >= 3 && w[1] < bound) {
                if (w[2] == kDecoPerVertexKHR) {
                    if (!is_per_vertex_var[w[1]]) {
                        is_per_vertex_var[w[1]] = 1u;
                        per_vertex_var_list.push_back(w[1]);
                    }
                } else if (w[2] == kDecoBuiltIn && pi.wc >= 4) {
                    if (w[3] == kBuiltInBaryCoordKHR) {
                        if (bary_var_mode[w[1]] == InterpMode::Keep) {
                            bary_var_list.push_back(w[1]);
                        }
                        bary_var_mode[w[1]] = InterpMode::Smooth;
                    } else if (w[3] == kBuiltInBaryCoordNoPerspKHR) {
                        if (bary_var_mode[w[1]] == InterpMode::Keep) {
                            bary_var_list.push_back(w[1]);
                        }
                        bary_var_mode[w[1]] = InterpMode::NoPerspective;
                    }
                } else if (w[2] == kDecoSample) {
                    keep_interp_func_cap = true;
                }
            }
            break;
        case kOpVariable:
            if (pi.wc == 4 && w[3] == kStorageClassInput && w[1] < bound && w[2] < bound) {
                const u32 pointee = ptr_input_pointee[w[1]];
                if (pointee < bound && arr3_elem_type[pointee] != 0) {
                    var_elem_type[w[2]] = arr3_elem_type[pointee];
                    var_old_ptr_type[w[2]] = w[1];
                }
            }
            break;
        case kOpExtInst:
            if (pi.wc >= 5 && glsl_set_id != 0 && w[3] == glsl_set_id &&
                (w[4] == kGlslInterpolateAtCentroid || w[4] == kGlslInterpolateAtSample ||
                 w[4] == kGlslInterpolateAtOffset)) {
                keep_interp_func_cap = true;
            }
            break;
        default:
            break;
        }
    }

    if (per_vertex_var_list.empty()) {
        // Fullscreen RectList shaders sometimes declare OpCapability FragmentBarycentricKHR with
        // zero PerVertexKHR inputs and zero BaryCoordKHR variables; strip the unused capability.
        if (!bary_var_list.empty()) {
            return false;
        }
        out.clear();
        out.reserve(in.size());
        out.insert(out.end(), in.begin(), in.begin() + 5);
        for (const auto& pi : ps.insts) {
            const u32* w = &in[pi.offset];
            if (pi.opcode == kOpCapability && pi.wc == 2) {
                if (w[1] == kCapFragmentBarycentricKHR) {
                    continue;
                }
                if (!keep_interp_func_cap && w[1] == kCapInterpolationFunction) {
                    continue;
                }
            }
            if (pi.opcode == kOpExtension && pi.wc >= 2 &&
                MatchSpvLiteralString(w + 1, pi.wc - 1, "SPV_KHR_fragment_shader_barycentric")) {
                continue;
            }
            out.insert(out.end(), w, w + pi.wc);
        }
        g_opt_hw_interp_modules.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    if (f32_type_id == 0 || f32_type_inst_offset == 0) {
        return false;
    }

    // Pass 2: Match barycentric I/J extractions and PerVertexKHR P1/P2 FSub+Fma trees inside functions.
    std::vector<InterpMode> bary_vec_ld_mode(bound, InterpMode::Keep);
    std::vector<BaryCompInfo> bary_ac_info(bound);
    std::vector<BaryCompInfo> bary_comp(bound);
    std::vector<PerVertexAc> pv_ac(bound);
    std::vector<u32> pv_ld_to_ac(bound, 0u);
    std::vector<SubPair> pv_sub(bound);
    std::vector<P1FmaInfo> p1_fma(bound);
    std::vector<P2FmaInfo> p2_fma(bound);
    std::vector<u32> var_p2_count(bound, 0u);
    std::vector<u8> var_mode_mask(bound, 0u);

    for (const auto& pi : ps.insts) {
        if (!pi.in_func || pi.result_id == 0 || pi.result_id >= bound) {
            continue;
        }
        const u32* w = &in[pi.offset];
        const u32 res = pi.result_id;

        switch (pi.opcode) {
        case kOpAccessChain:
            if (pi.wc >= 5 && w[3] < bound) {
                const u32 base = w[3];
                if (bary_var_mode[base] != InterpMode::Keep && pi.wc == 5 && w[4] < bound &&
                    has_u32_const[w[4]]) {
                    const u32 idx = u32_const_val[w[4]];
                    if (idx == 1u || idx == 2u) {
                        bary_ac_info[res] = {bary_var_mode[base], static_cast<u8>(idx)};
                    }
                } else if (is_per_vertex_var[base] && var_elem_type[base] != 0 && w[4] < bound &&
                           has_u32_const[w[4]]) {
                    const u32 vtx = u32_const_val[w[4]];
                    if (vtx < 3u && (pi.wc == 5 || pi.wc == 6)) {
                        PerVertexAc ac{};
                        ac.var_id = base;
                        ac.vtx_idx = vtx;
                        ac.elem_ptr_type = w[1];
                        if (pi.wc == 6) {
                            ac.has_comp = true;
                            ac.comp_const_id = w[5];
                        }
                        pv_ac[res] = ac;
                    }
                }
            }
            break;
        case kOpLoad:
            if (pi.wc == 4 && w[3] < bound) {
                const u32 ptr = w[3];
                if (bary_var_mode[ptr] != InterpMode::Keep) {
                    bary_vec_ld_mode[res] = bary_var_mode[ptr];
                } else if (bary_ac_info[ptr].mode != InterpMode::Keep && w[1] == f32_type_id) {
                    bary_comp[res] = bary_ac_info[ptr];
                } else if (pv_ac[ptr].var_id != 0 && w[1] == f32_type_id) {
                    pv_ld_to_ac[res] = ptr;
                }
            }
            break;
        case kOpCompositeExtract:
            if (pi.wc == 5 && w[1] == f32_type_id && w[3] < bound &&
                bary_vec_ld_mode[w[3]] != InterpMode::Keep && (w[4] == 1u || w[4] == 2u)) {
                bary_comp[res] = {bary_vec_ld_mode[w[3]], static_cast<u8>(w[4])};
            }
            break;
        case kOpFSub:
            if (pi.wc == 5 && w[1] == f32_type_id && w[3] < bound && w[4] < bound) {
                const u32 ac_hi_id = pv_ld_to_ac[w[3]];
                const u32 ac_lo_id = pv_ld_to_ac[w[4]];
                if (ac_hi_id != 0 && ac_lo_id != 0) {
                    const auto& hi = pv_ac[ac_hi_id];
                    const auto& lo = pv_ac[ac_lo_id];
                    if (hi.var_id == lo.var_id && hi.has_comp == lo.has_comp &&
                        hi.comp_const_id == lo.comp_const_id &&
                        hi.elem_ptr_type == lo.elem_ptr_type && lo.vtx_idx == 0u &&
                        (hi.vtx_idx == 1u || hi.vtx_idx == 2u)) {
                        pv_sub[res] = {w[3],           w[4],             hi.var_id,
                                       hi.vtx_idx,     hi.comp_const_id, hi.elem_ptr_type,
                                       hi.has_comp};
                    }
                }
            }
            break;
        case kOpExtInst:
            if (pi.wc == 8 && w[1] == f32_type_id && glsl_set_id != 0 && w[3] == glsl_set_id &&
                w[4] == kGlslFma && w[5] < bound && w[6] < bound && w[7] < bound) {
                const u32 a = w[5];
                const u32 b = w[6];
                const u32 c = w[7];
                const auto& sub = pv_sub[a];
                const auto& bc = bary_comp[b];
                if (sub.var_id != 0 && bc.mode != InterpMode::Keep) {
                    if (sub.vtx_hi == 1u && bc.axis == 1u && pv_ld_to_ac[c] != 0) {
                        const auto& c_ac = pv_ac[pv_ld_to_ac[c]];
                        if (c_ac.var_id == sub.var_id && c_ac.vtx_idx == 0u &&
                            c_ac.has_comp == sub.has_comp &&
                            c_ac.comp_const_id == sub.comp_const_id) {
                            p1_fma[res] = {sub.var_id, sub.comp_const_id, sub.elem_ptr_type,
                                           sub.has_comp, bc.mode};
                        }
                    } else if (sub.vtx_hi == 2u && bc.axis == 2u &&
                               p1_fma[c].mode != InterpMode::Keep) {
                        const auto& p1 = p1_fma[c];
                        if (p1.var_id == sub.var_id && p1.mode == bc.mode &&
                            p1.has_comp == sub.has_comp &&
                            p1.comp_const_id == sub.comp_const_id &&
                            p1.elem_ptr_type == sub.elem_ptr_type) {
                            p2_fma[res] = {sub.var_id, sub.comp_const_id, sub.elem_ptr_type,
                                           sub.has_comp, bc.mode};
                            ++var_p2_count[sub.var_id];
                            var_mode_mask[sub.var_id] |=
                                (bc.mode == InterpMode::Smooth) ? 0x1u : 0x2u;
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    // Helper: given candidate Smooth/NoPerspective variables, run reverse SSA use-release from
    // their P2 Fma roots to determine which intermediate instructions and variables have 100% of
    // their uses accounted for.
    std::vector<u8> candidate_interp_var(bound, 0u);
    for (const u32 v : per_vertex_var_list) {
        if (var_elem_type[v] != 0 && var_p2_count[v] > 0 &&
            (var_mode_mask[v] == 0x1u || var_mode_mask[v] == 0x2u)) {
            candidate_interp_var[v] = 1u;
        }
    }

    std::vector<u32> released_uses(bound, 0u);
    std::vector<u8> dead_id(bound, 0u);

    const auto run_reverse_release = [&]() {
        std::fill(released_uses.begin(), released_uses.end(), 0u);
        std::fill(dead_id.begin(), dead_id.end(), 0u);

        for (size_t idx = ps.insts.size(); idx-- > 0;) {
            const auto& pi = ps.insts[idx];
            if (!pi.in_func || pi.result_id == 0 || pi.result_id >= bound) {
                continue;
            }
            const u32 res = pi.result_id;
            bool release_operands = false;

            if (p2_fma[res].mode != InterpMode::Keep &&
                candidate_interp_var[p2_fma[res].var_id]) {
                // P2 Fma is replaced in-place by OpAccessChain + OpLoad on var_id, releasing its
                // old Fma operands (p20_sub, bary_j, p1_fma).
                release_operands = true;
            } else {
                const bool is_chain_node =
                    (p1_fma[res].mode != InterpMode::Keep &&
                     candidate_interp_var[p1_fma[res].var_id]) ||
                    (pv_sub[res].var_id != 0 && candidate_interp_var[pv_sub[res].var_id]) ||
                    (pv_ld_to_ac[res] != 0 &&
                     candidate_interp_var[pv_ac[pv_ld_to_ac[res]].var_id]) ||
                    (pv_ac[res].var_id != 0 && candidate_interp_var[pv_ac[res].var_id]) ||
                    bary_comp[res].mode != InterpMode::Keep ||
                    bary_vec_ld_mode[res] != InterpMode::Keep ||
                    bary_ac_info[res].mode != InterpMode::Keep;

                if (is_chain_node && ps.total_uses[res] > 0 &&
                    released_uses[res] == ps.total_uses[res]) {
                    dead_id[res] = 1u;
                    release_operands = true;
                }
            }

            if (release_operands) {
                for (u16 k = 0; k < pi.id_op_count; ++k) {
                    const u32 op_id = ps.id_operands[pi.id_op_begin + k];
                    if (op_id < bound) {
                        ++released_uses[op_id];
                    }
                }
            }
        }
    };

    run_reverse_release();

    // Validate each PerVertexKHR variable: either 100% Smooth/NoPerspective, 100% Flat (vertex 0),
    // or unused (Smooth). If any candidate Smooth/NoPerspective variable had stray non-P2 uses,
    // drop it from candidate_interp_var and re-run reverse release so its instructions stay intact.
    std::vector<InterpMode> final_var_mode(bound, InterpMode::Keep);
    bool dropped_any_candidate = false;
    for (const u32 v : per_vertex_var_list) {
        if (var_elem_type[v] == 0) {
            continue;
        }
        if (candidate_interp_var[v]) {
            if (released_uses[v] == ps.total_uses[v]) {
                final_var_mode[v] =
                    (var_mode_mask[v] == 0x1u) ? InterpMode::Smooth : InterpMode::NoPerspective;
            } else {
                candidate_interp_var[v] = 0u;
                dropped_any_candidate = true;
            }
        } else if (var_p2_count[v] == 0) {
            if (ps.total_uses[v] == 0) {
                final_var_mode[v] = InterpMode::Smooth;
            } else {
                // Check for V_INTERP_MOV_F32 provoking-vertex (vertex 0) Flat loads: every use of
                // `v` in functions must be an OpAccessChain with vtx_idx == 0.
                u32 vtx0_ac_uses = 0;
                for (const auto& pi : ps.insts) {
                    if (pi.in_func && pi.opcode == kOpAccessChain && pi.result_id < bound &&
                        pv_ac[pi.result_id].var_id == v && pv_ac[pi.result_id].vtx_idx == 0u) {
                        ++vtx0_ac_uses;
                    }
                }
                if (vtx0_ac_uses == ps.total_uses[v]) {
                    final_var_mode[v] = InterpMode::Flat;
                }
            }
        }
    }

    if (dropped_any_candidate) {
        run_reverse_release();
    }

    u64 converted_varyings = 0;
    bool all_pv_converted = true;
    for (const u32 v : per_vertex_var_list) {
        if (final_var_mode[v] != InterpMode::Keep) {
            ++converted_varyings;
        } else {
            all_pv_converted = false;
        }
    }
    if (converted_varyings == 0) {
        return false;
    }

    bool all_bary_dead = true;
    for (const u32 bv : bary_var_list) {
        if (released_uses[bv] == ps.total_uses[bv]) {
            dead_id[bv] = 1u;
        } else {
            all_bary_dead = false;
        }
    }
    const bool strip_bary_ext = all_pv_converted && all_bary_dead;

    // Mark active P2 Fma outputs and check whether any integer-truncation nudges are required.
    std::vector<u8> active_p2(bound, 0u);
    u64 converted_channels = 0;
    for (u32 id = 1; id < bound; ++id) {
        if (p2_fma[id].mode != InterpMode::Keep &&
            (final_var_mode[p2_fma[id].var_id] == InterpMode::Smooth ||
             final_var_mode[p2_fma[id].var_id] == InterpMode::NoPerspective)) {
            active_p2[id] = 1u;
            ++converted_channels;
        }
    }

    bool needs_nudge_const = false;
    for (const auto& pi : ps.insts) {
        if (!pi.in_func) {
            continue;
        }
        const u32* w = &in[pi.offset];
        if ((pi.opcode == kOpConvertFToU || pi.opcode == kOpConvertFToS) && pi.wc == 4 &&
            w[3] < bound && active_p2[w[3]]) {
            needs_nudge_const = true;
            break;
        }
        if (pi.opcode == kOpExtInst && pi.wc == 6 && glsl_set_id != 0 && w[3] == glsl_set_id &&
            (w[4] == kGlslTrunc || w[4] == kGlslFloor) && w[5] < bound && active_p2[w[5]]) {
            needs_nudge_const = true;
            break;
        }
    }

    u32 next_id = bound;
    u32 nudge_const_id = existing_nudge_const_id;
    bool emit_nudge_const = false;
    if (needs_nudge_const && nudge_const_id == 0) {
        nudge_const_id = next_id++;
        emit_nudge_const = true;
    }

    // Ensure every converted PerVertexKHR variable has an OpTypePointer Input %elem_type.
    std::vector<u32> emit_new_ptr_at_offset(in.size(), 0u);
    std::vector<u32> emit_new_ptr_elem(in.size(), 0u);
    for (const u32 v : per_vertex_var_list) {
        if (final_var_mode[v] == InterpMode::Keep) {
            continue;
        }
        const u32 elem_t = var_elem_type[v];
        if (input_ptr_for_pointee[elem_t] == 0) {
            const u32 new_ptr_id = next_id++;
            input_ptr_for_pointee[elem_t] = new_ptr_id;
            const u32 old_ptr_off = ptr_type_inst_offset[var_old_ptr_type[v]];
            emit_new_ptr_at_offset[old_ptr_off] = new_ptr_id;
            emit_new_ptr_elem[old_ptr_off] = elem_t;
        }
    }

    // Pass 3: Emit rewritten SPIR-V module.
    out.clear();
    out.reserve(in.size());
    out.insert(out.end(), in.begin(), in.begin() + 5);

    u64 int_nudges = 0;

    for (const auto& pi : ps.insts) {
        const u32* w = &in[pi.offset];

        if (strip_bary_ext) {
            if (pi.opcode == kOpCapability && pi.wc == 2) {
                if (w[1] == kCapFragmentBarycentricKHR) {
                    continue;
                }
                if (!keep_interp_func_cap && w[1] == kCapInterpolationFunction) {
                    continue;
                }
            }
            if (pi.opcode == kOpExtension && pi.wc >= 2 &&
                MatchSpvLiteralString(w + 1, pi.wc - 1, "SPV_KHR_fragment_shader_barycentric")) {
                continue;
            }
        }

        // Remove dead BaryCoordKHR variables from OpEntryPoint interface list.
        if (pi.opcode == kOpEntryPoint && pi.wc >= 4) {
            // Word 1: ExecutionModel, Word 2: EntryPoint <id>, Words 3..k: Name string.
            u32 iface_start = 3u;
            while (iface_start < pi.wc) {
                const u32 sw = w[iface_start++];
                if ((sw & 0xFFu) == 0u || (sw & 0xFF00u) == 0u || (sw & 0xFF0000u) == 0u ||
                    (sw & 0xFF000000u) == 0u) {
                    break;
                }
            }
            const size_t ep_pos = out.size();
            out.insert(out.end(), w, w + iface_start);
            for (u32 i = iface_start; i < pi.wc; ++i) {
                if (w[i] >= bound || !dead_id[w[i]]) {
                    out.push_back(w[i]);
                }
            }
            const u32 new_wc = static_cast<u32>(out.size() - ep_pos);
            out[ep_pos] = (new_wc << 16) | kOpEntryPoint;
            continue;
        }

        // Strip OpName / OpDecorate targeting deleted instructions or variables, and rewrite
        // PerVertexKHR decorations on converted Input variables.
        if (pi.opcode == kOpName && pi.wc >= 3 && w[1] < bound && dead_id[w[1]]) {
            continue;
        }
        if (pi.opcode == kOpDecorate && pi.wc >= 3 && w[1] < bound) {
            if (dead_id[w[1]]) {
                continue;
            }
            if (w[2] == kDecoPerVertexKHR && final_var_mode[w[1]] != InterpMode::Keep) {
                if (final_var_mode[w[1]] == InterpMode::NoPerspective) {
                    out.push_back((3u << 16) | kOpDecorate);
                    out.push_back(w[1]);
                    out.push_back(kDecoNoPerspective);
                } else if (final_var_mode[w[1]] == InterpMode::Flat) {
                    out.push_back((3u << 16) | kOpDecorate);
                    out.push_back(w[1]);
                    out.push_back(kDecoFlat);
                }
                // InterpMode::Smooth is Vulkan's default perspective-correct interpolation.
                continue;
            }
        }

        // Delete dead variables and intermediate barycentric SSA instructions.
        if (pi.result_id != 0 && pi.result_id < bound && dead_id[pi.result_id]) {
            continue;
        }

        // Rewrite converted Input OpVariable pointer types from [3] array to element type.
        if (pi.opcode == kOpVariable && pi.wc == 4 && w[3] == kStorageClassInput &&
            w[2] < bound && final_var_mode[w[2]] != InterpMode::Keep) {
            out.push_back((4u << 16) | kOpVariable);
            out.push_back(input_ptr_for_pointee[var_elem_type[w[2]]]);
            out.push_back(w[2]);
            out.push_back(kStorageClassInput);
            continue;
        }

        // Replace P2 Fma with direct hardware-interpolated OpAccessChain + OpLoad.
        if (pi.in_func && pi.result_id != 0 && pi.result_id < bound && active_p2[pi.result_id]) {
            const auto& p2 = p2_fma[pi.result_id];
            if (p2.has_comp) {
                const u32 ac_id = next_id++;
                out.push_back((5u << 16) | kOpAccessChain);
                out.push_back(p2.elem_ptr_type);
                out.push_back(ac_id);
                out.push_back(p2.var_id);
                out.push_back(p2.comp_const_id);

                out.push_back((4u << 16) | kOpLoad);
                out.push_back(f32_type_id);
                out.push_back(pi.result_id);
                out.push_back(ac_id);
            } else {
                out.push_back((4u << 16) | kOpLoad);
                out.push_back(f32_type_id);
                out.push_back(pi.result_id);
                out.push_back(p2.var_id);
            }
            continue;
        }

        // Rewrite Flat Input OpAccessChain: strip leading vertex-0 index (var, 0, comp) -> (var, comp).
        if (pi.in_func && pi.opcode == kOpAccessChain && pi.wc >= 5 && w[3] < bound &&
            final_var_mode[w[3]] == InterpMode::Flat) {
            if (pi.wc > 5) {
                out.push_back((static_cast<u32>(pi.wc - 1u) << 16) | kOpAccessChain);
                out.push_back(w[1]);
                out.push_back(w[2]);
                out.push_back(w[3]);
                for (u16 i = 5; i < pi.wc; ++i) {
                    out.push_back(w[i]);
                }
            } else {
                out.push_back((4u << 16) | kOpCopyObject);
                out.push_back(w[1]);
                out.push_back(w[2]);
                out.push_back(w[3]);
            }
            continue;
        }

        // Insert 1 + 2^-16 scaling nudge before integer truncations of Smooth/NoPerspective inputs.
        if (pi.in_func && (pi.opcode == kOpConvertFToU || pi.opcode == kOpConvertFToS) &&
            pi.wc == 4 && w[3] < bound && active_p2[w[3]]) {
            const u32 nudged_id = next_id++;
            out.push_back((5u << 16) | kOpFMul);
            out.push_back(f32_type_id);
            out.push_back(nudged_id);
            out.push_back(w[3]);
            out.push_back(nudge_const_id);

            out.push_back((4u << 16) | pi.opcode);
            out.push_back(w[1]);
            out.push_back(w[2]);
            out.push_back(nudged_id);
            ++int_nudges;
            continue;
        }
        if (pi.in_func && pi.opcode == kOpExtInst && pi.wc == 6 && glsl_set_id != 0 &&
            w[3] == glsl_set_id && (w[4] == kGlslTrunc || w[4] == kGlslFloor) && w[5] < bound &&
            active_p2[w[5]]) {
            const u32 nudged_id = next_id++;
            out.push_back((5u << 16) | kOpFMul);
            out.push_back(f32_type_id);
            out.push_back(nudged_id);
            out.push_back(w[5]);
            out.push_back(nudge_const_id);

            out.push_back((6u << 16) | kOpExtInst);
            out.push_back(w[1]);
            out.push_back(w[2]);
            out.push_back(w[3]);
            out.push_back(w[4]);
            out.push_back(nudged_id);
            ++int_nudges;
            continue;
        }

        // Copy unmodified instruction and emit any newly synthesized constant / pointer type
        // immediately after its dependency in the Type-Constant section.
        out.insert(out.end(), w, w + pi.wc);

        if (emit_nudge_const && pi.offset == f32_type_inst_offset) {
            out.push_back((4u << 16) | kOpConstant);
            out.push_back(f32_type_id);
            out.push_back(nudge_const_id);
            out.push_back(kF32InterpIntNudgeBits);
            emit_nudge_const = false;
        }
        if (pi.offset < emit_new_ptr_at_offset.size() && emit_new_ptr_at_offset[pi.offset] != 0) {
            out.push_back((4u << 16) | kOpTypePointer);
            out.push_back(emit_new_ptr_at_offset[pi.offset]);
            out.push_back(kStorageClassInput);
            out.push_back(emit_new_ptr_elem[pi.offset]);
        }
    }

    out[3] = next_id;

    g_opt_hw_interp_modules.fetch_add(1, std::memory_order_relaxed);
    g_opt_hw_interp_varyings.fetch_add(converted_varyings, std::memory_order_relaxed);
    g_opt_hw_interp_channels.fetch_add(converted_channels, std::memory_order_relaxed);
    g_opt_hw_interp_int_nudges.fetch_add(int_nudges, std::memory_order_relaxed);
    return true;
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

/// Block 5, Steps 5.2 & 5.3 fast-FP pre-pass over SPIR-V words:
/// 1. Strips OpDecorate %id NoContraction so NVIDIA's SASS backend fuses FMUL+FADD into FFMA.
/// 2. Folds provably exact V_MUL_LEGACY_F32 selects ((a==0 || b==0) ? +0.0 : a*b) whenever
///    a == b (x*x) or either operand is an f32 constant (zero or finite non-zero).
/// 3. Folds redundant GCN canonicalization FMax(x, x) / FMin(x, x) / NMax(x, x) / NMin(x, x) -> x.
/// 4. Strips SignedZeroInfNanPreserve when the module has no OpIsNan/OpIsInf and no remaining
///    dynamic V_MUL_LEGACY_F32 select (preserving 0 * rsqrt(0) == 0 safety where needed).
bool ApplyFastFpSpirvPass(const std::vector<u32>& in, std::vector<u32>& out) {
    if (in.size() < 5 || in[0] != kSpirvMagic) {
        return false;
    }
    const u32 bound = in[3];
    if (bound == 0 || bound > 2'000'000u) {
        return false;
    }

    u32 glsl_set_id = 0;
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
        case kOpExtInstImport:
            if (wc >= 3 && MatchSpvLiteralString(&in[pos + 2], wc - 2, "GLSL.std.450")) {
                glsl_set_id = in[pos + 1];
            }
            break;
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
    u64 self_minmax = 0;
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
        if (op == kOpExtInst && wc == 7 && glsl_set_id != 0 && in[pos + 3] == glsl_set_id &&
            (in[pos + 4] == kGlslFMin || in[pos + 4] == kGlslFMax ||
             in[pos + 4] == kGlslNMin || in[pos + 4] == kGlslNMax) &&
            in[pos + 5] == in[pos + 6]) {
            out.push_back((4u << 16) | kOpCopyObject);
            out.push_back(in[pos + 1]);
            out.push_back(in[pos + 2]);
            out.push_back(in[pos + 5]);
            ++self_minmax;
            pos += wc;
            continue;
        }

        out.insert(out.end(), in.begin() + static_cast<std::ptrdiff_t>(pos),
                   in.begin() + static_cast<std::ptrdiff_t>(pos + wc));
        pos += wc;
    }

    g_opt_nocontract_stripped.fetch_add(nocontract, std::memory_order_relaxed);
    g_opt_legacy_mul_folded.fetch_add(folded, std::memory_order_relaxed);
    g_opt_self_minmax_folded.fetch_add(self_minmax, std::memory_order_relaxed);
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

bool HwInterpEnabled() noexcept {
    static const bool enabled = ParseEnvFlag("BB_HW_INTERP", true);
    return enabled;
}

bool OptimizeSPIRV(std::vector<u32>& spv, u64 pgm_hash) {
    if (!SpirvOptEnabled() || spv.size() < 5 || spv[0] != kSpirvMagic) {
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();
    const size_t initial_words = spv.size();
    const bool run_val = SpirvValEnabled();

    std::vector<u32> hw_interp_buf;
    std::vector<u32> fast_fp_buf;
    const std::vector<u32>* input = &spv;
    if (HwInterpEnabled() && ApplyHwInterpSpirvPass(*input, hw_interp_buf)) {
        input = &hw_interp_buf;
    }
    if (FastFpEnabled() && ApplyFastFpSpirvPass(*input, fast_fp_buf)) {
        input = &fast_fp_buf;
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
    s.hw_interp_modules = g_opt_hw_interp_modules.exchange(0, std::memory_order_relaxed);
    s.hw_interp_varyings = g_opt_hw_interp_varyings.exchange(0, std::memory_order_relaxed);
    s.hw_interp_channels = g_opt_hw_interp_channels.exchange(0, std::memory_order_relaxed);
    s.hw_interp_int_nudges = g_opt_hw_interp_int_nudges.exchange(0, std::memory_order_relaxed);
    s.self_minmax_folded = g_opt_self_minmax_folded.exchange(0, std::memory_order_relaxed);
    return s;
}

} // namespace Shader::Backend::SPIRV
