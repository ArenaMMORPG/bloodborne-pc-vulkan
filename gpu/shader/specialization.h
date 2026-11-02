// SPDX-License-Identifier: GPL-2.0-or-later
// bbport shader translator: what a compiled shader depends on beyond its program and runtime
// info: the bound resources' types and formats, vertex input types and divisors. The key of
// shader permutations (a module is recompiled when it differs).
#pragma once

#include <bitset>
#include <optional>

#include <boost/container/small_vector.hpp>

#include "common/types.h"
#include "shader/backend/bindings.h"
#include "shader/frontend/fetch_shader.h"
#include "shader/info.h"
#include "shader/profile.h"

namespace Shader {

struct VsAttribSpecialization {
    u32 divisor{};
    ::Gcn::NumberClass num_class{};
    ::Gcn::CompMapping dst_select{};

    bool operator==(const VsAttribSpecialization&) const = default;
};

struct BufferSpecialization {
    u32 stride : 14;
    u32 is_formatted : 1;
    u32 swizzle_enable : 1;
    u32 data_format : 6;
    u32 num_format : 4;
    u32 index_stride : 2;
    u32 element_size : 2;
    u32 paged : 1; ///< BB_LAYER_MEMORY: through the page table (IsPagedBuffer)
    ::Gcn::CompMapping dst_select{};
    ::Gcn::NumberConversion num_conversion{};

    bool operator==(const BufferSpecialization& other) const {
        return stride == other.stride && is_formatted == other.is_formatted &&
               paged == other.paged &&
               swizzle_enable == other.swizzle_enable &&
               (!is_formatted ||
                (data_format == other.data_format && num_format == other.num_format &&
                 dst_select == other.dst_select && num_conversion == other.num_conversion)) &&
               (!swizzle_enable ||
                (index_stride == other.index_stride && element_size == other.element_size));
    }
};

struct ImageSpecialization {
    ::Gcn::ImageType type = ::Gcn::ImageType::Color2D;
    bool is_integer = false;
    bool is_storage = false;
    bool is_cube = false;
    bool is_srgb = false;
    ::Gcn::CompMapping dst_select{};
    ::Gcn::NumberConversion num_conversion{};
    u32 num_bindings = 0; ///< views bound (mip fallback arrays)

    bool operator==(const ImageSpecialization&) const = default;
};

struct FMaskSpecialization {
    u32 width;
    u32 height;

    bool operator==(const FMaskSpecialization&) const = default;
};

struct SamplerSpecialization {
    u8 force_unnormalized : 1;
    u8 force_degamma : 1;

    bool operator==(const SamplerSpecialization&) const = default;
};

/// The specialization of one stage. Separate from the runtime info: the resource layout is
/// known only once the module has been compiled.
struct StageSpecialization {
    static constexpr size_t MaxStageResources = 128;

    const Info* info{};
    RuntimeInfo runtime_info{};
    std::bitset<MaxStageResources> bitset{};
    std::optional<Gcn::FetchShaderData> fetch_shader_data{};
    boost::container::small_vector<VsAttribSpecialization, 32> vs_attribs;
    boost::container::small_vector<BufferSpecialization, 16> buffers;
    boost::container::small_vector<ImageSpecialization, 16> images;
    boost::container::small_vector<FMaskSpecialization, 8> fmasks;
    boost::container::small_vector<SamplerSpecialization, 16> samplers;
    Backend::Bindings start{};

    StageSpecialization() = default;
    StageSpecialization(const Info& info_, RuntimeInfo runtime_info_, const Profile& profile_,
                        Backend::Bindings start_)
        : info{&info_}, runtime_info{runtime_info_}, start{start_} {
        fetch_shader_data = Gcn::ParseFetchShader(info_);
        if (info_.sw_stage == SwStage::Vertex && fetch_shader_data) {
            // Vertex input number types (Vulkan requires matching shader types).
            ForEachSharp(vs_attribs, fetch_shader_data->attributes,
                         [this](auto& spec, const auto& desc, ::Gcn::Buffer sharp) {
                             using InstanceIdType = Shader::Gcn::VertexAttribute::InstanceIdType;
                             if (const auto step_rate = desc.GetStepRate();
                                 step_rate != InstanceIdType::None) {
                                 spec.divisor = step_rate == InstanceIdType::OverStepRate0
                                                    ? runtime_info.sw.vs.step_rate_0
                                                    : (step_rate == InstanceIdType::OverStepRate1
                                                           ? runtime_info.sw.vs.step_rate_1
                                                           : 1);
                             }
                             spec.num_class = ::Gcn::GetNumberClass(sharp.GetNumberFmt());
                             spec.dst_select = sharp.DstSelect();
                         });
        }
        u32 binding{};
        ForEachSharp(binding, buffers, info->buffers,
                     [&profile_, &info_](auto& spec, const auto& desc, ::Gcn::Buffer sharp) {
                         spec.stride = sharp.GetStride();
                         spec.paged = IsPagedBuffer(profile_.paged_buffers, sharp, info_.pgm_hash,
                                                    profile_.buffer_copy_shader_hash);
                         spec.is_formatted = desc.is_formatted;
                         spec.swizzle_enable = sharp.swizzle_enable;
                         if (spec.is_formatted) {
                             spec.data_format = static_cast<u32>(sharp.GetDataFmt());
                             spec.num_format = static_cast<u32>(sharp.GetNumberFmt());
                             spec.dst_select = sharp.DstSelect();
                             spec.num_conversion = sharp.GetNumberConversion();
                         }
                         if (spec.swizzle_enable) {
                             spec.index_stride = sharp.index_stride;
                             spec.element_size = sharp.element_size;
                         }
                     });
        ForEachSharp(binding, images, info->images,
                     [&](auto& spec, const auto& desc, ::Gcn::Image sharp) {
                         spec.type = sharp.GetViewType(desc.is_array);
                         spec.is_integer = ::Gcn::IsInteger(sharp.GetNumberFmt());
                         spec.is_storage = desc.is_written;
                         spec.is_cube = sharp.IsCube();
                         if (spec.is_storage) {
                             spec.dst_select = sharp.DstSelect();
                         } else {
                             spec.is_srgb = sharp.GetNumberFmt() == ::Gcn::NumberFormat::Srgb;
                         }
                         spec.num_conversion = sharp.GetNumberConversion();
                         spec.num_bindings = desc.NumBindings(*info);
                     });
        ForEachSharp(binding, fmasks, info->fmasks,
                     [](auto& spec, const auto& desc, ::Gcn::Image sharp) {
                         spec.width = sharp.width;
                         spec.height = sharp.height;
                     });
        ForEachSharp(samplers, info->samplers,
                     [](auto& spec, const auto& desc, ::Gcn::Sampler sharp) {
                         spec.force_unnormalized = sharp.force_unnormalized;
                         spec.force_degamma = sharp.force_degamma;
                     });

        // Runtime info found by the tessellation passes.
        if (info->sw_stage == SwStage::TessellationControl ||
            info->sw_stage == SwStage::TessellationEval) {
            TessellationDataConstantBuffer tess_constants{};
            info->ReadTessConstantBuffer(tess_constants);
            runtime_info.InitFromTessConstants(tess_constants);
        }
    }

    void ForEachSharp(auto& spec_list, auto& desc_list, auto&& func) {
        for (const auto& desc : desc_list) {
            auto& spec = spec_list.emplace_back();
            const auto sharp = desc.GetSharp(*info);
            if (!sharp) {
                continue;
            }
            func(spec, desc, sharp);
        }
    }

    void ForEachSharp(u32& binding, auto& spec_list, auto& desc_list, auto&& func) {
        for (const auto& desc : desc_list) {
            auto& spec = spec_list.emplace_back();
            const auto sharp = desc.GetSharp(*info);
            if (!sharp) {
                binding++;
                continue;
            }
            bitset.set(binding++);
            func(spec, desc, sharp);
        }
    }

    [[nodiscard]] bool Valid() const {
        return info != nullptr;
    }

    bool operator==(const StageSpecialization& other) const {
        if (!Valid()) {
            return false;
        }

        if (vs_attribs != other.vs_attribs) {
            return false;
        }

        if (runtime_info != other.runtime_info) {
            return false;
        }

        if (fetch_shader_data != other.fetch_shader_data) {
            return false;
        }

        if (fmasks != other.fmasks) {
            return false;
        }

        // Without bindings the start bindings (which follow the other stages) do not matter.
        if (bitset.none() && other.bitset.none()) {
            return true;
        }

        if (start != other.start) {
            return false;
        }

        if (buffers.size() != other.buffers.size() || images.size() != other.images.size() ||
            samplers.size() != other.samplers.size()) {
            return false;
        }

        u32 binding{};
        for (u32 i = 0; i < buffers.size(); i++) {
            if (other.bitset[binding++] && buffers[i] != other.buffers[i]) {
                return false;
            }
        }
        for (u32 i = 0; i < images.size(); i++) {
            if (other.bitset[binding++] && images[i] != other.images[i]) {
                return false;
            }
        }

        for (u32 i = 0; i < samplers.size(); i++) {
            if (samplers[i] != other.samplers[i]) {
                return false;
            }
        }
        return true;
    }

    /// Zero-allocation exact specialization match against live `cur_info` user data (`*this ==
    /// StageSpecialization(cur_info, cur_rt, profile, cur_start)`), using pre-parsed `cur_fetch`.
    [[nodiscard]] bool MatchesInPlace(
        const Info& cur_info, const RuntimeInfo& cur_rt, const Profile& profile,
        Backend::Bindings cur_start, const Gcn::FetchShaderData* cur_fetch,
        bool skip_rt_and_fetch_compare = false,
        boost::container::static_vector<::Gcn::Buffer, NUM_BUFFERS>* out_vsharps = nullptr) const {
        if (!Valid()) {
            return false;
        }
        if (cur_info.sw_stage == SwStage::TessellationControl ||
            cur_info.sw_stage == SwStage::TessellationEval) {
            return false;
        }
        if (!skip_rt_and_fetch_compare && runtime_info != cur_rt) {
            return false;
        }
        if (fetch_shader_data.has_value() != (cur_fetch != nullptr)) {
            return false;
        }
        const auto* const snap = cur_info.Snapshot();
        const u32* const flat = snap ? snap->flat : cur_info.flattened_ud_buf.data();
        const VAddr pgm_base = snap ? snap->pgm_base : cur_info.pgm_base;
        const std::span<const u32> ud =
            snap ? std::span<const u32>{snap->user_data, snap->user_data_size} : cur_info.user_data;

        if (cur_fetch) {
            if (!skip_rt_and_fetch_compare && !(*fetch_shader_data == *cur_fetch)) {
                return false;
            }
            if (cur_info.sw_stage == SwStage::Vertex) {
                if (vs_attribs.size() != cur_fetch->attributes.size()) {
                    return false;
                }
                if (out_vsharps) {
                    out_vsharps->clear();
                }
                for (size_t i = 0; i < cur_fetch->attributes.size(); ++i) {
                    VsAttribSpecialization aspec{};
                    const auto& desc = cur_fetch->attributes[i];
                    const auto sharp = desc.GetSharpDirect(ud);
                    if (out_vsharps && out_vsharps->size() < out_vsharps->capacity()) {
                        out_vsharps->push_back(sharp);
                    }
                    if (sharp) {
                        using InstanceIdType = Shader::Gcn::VertexAttribute::InstanceIdType;
                        if (const auto step_rate = desc.GetStepRate();
                            step_rate != InstanceIdType::None) {
                            aspec.divisor = step_rate == InstanceIdType::OverStepRate0
                                                ? cur_rt.sw.vs.step_rate_0
                                                : (step_rate == InstanceIdType::OverStepRate1
                                                       ? cur_rt.sw.vs.step_rate_1
                                                       : 1);
                        }
                        aspec.num_class = ::Gcn::GetNumberClass(sharp.GetNumberFmt());
                        aspec.dst_select = sharp.DstSelect();
                    }
                    if (vs_attribs[i] != aspec) {
                        if (out_vsharps) {
                            out_vsharps->clear();
                        }
                        return false;
                    }
                }
            } else if (!vs_attribs.empty()) {
                return false;
            }
        } else if (!vs_attribs.empty()) {
            return false;
        }

        if (fmasks.size() != cur_info.fmasks.size()) {
            return false;
        }
        bool cur_any_bit = false;
        for (size_t i = 0; i < cur_info.fmasks.size(); ++i) {
            FMaskSpecialization fspec{};
            const auto sharp = cur_info.fmasks[i].GetSharpDirect(flat);
            if (sharp) {
                cur_any_bit = true;
                fspec.width = sharp.width;
                fspec.height = sharp.height;
            }
            if (fmasks[i] != fspec) {
                return false;
            }
        }

        const bool buffers_match_size = (buffers.size() == cur_info.buffers.size());
        const bool images_match_size = (images.size() == cur_info.images.size());
        const bool samplers_match_size = (samplers.size() == cur_info.samplers.size());

        for (size_t i = 0; i < cur_info.buffers.size(); ++i) {
            const auto& desc = cur_info.buffers[i];
            const auto sharp = desc.GetSharpDirect(flat, pgm_base);
            if (!sharp) {
                continue;
            }
            cur_any_bit = true;
            if (!buffers_match_size || !(start == cur_start)) {
                return false;
            }
            BufferSpecialization bspec{};
            bspec.stride = sharp.GetStride();
            bspec.paged = IsPagedBuffer(profile.paged_buffers, sharp, cur_info.pgm_hash,
                                        profile.buffer_copy_shader_hash);
            bspec.is_formatted = desc.is_formatted;
            bspec.swizzle_enable = sharp.swizzle_enable;
            if (bspec.is_formatted) {
                bspec.data_format = static_cast<u32>(sharp.GetDataFmt());
                bspec.num_format = static_cast<u32>(sharp.GetNumberFmt());
                bspec.dst_select = sharp.DstSelect();
                bspec.num_conversion = sharp.GetNumberConversion();
            }
            if (bspec.swizzle_enable) {
                bspec.index_stride = sharp.index_stride;
                bspec.element_size = sharp.element_size;
            }
            if (buffers[i] != bspec) {
                return false;
            }
        }

        for (size_t i = 0; i < cur_info.images.size(); ++i) {
            const auto& desc = cur_info.images[i];
            const auto sharp = desc.GetSharpDirect(flat);
            if (!sharp) {
                continue;
            }
            cur_any_bit = true;
            if (!images_match_size || !(start == cur_start)) {
                return false;
            }
            ImageSpecialization ispec{};
            ispec.type = sharp.GetViewType(desc.is_array);
            ispec.is_integer = ::Gcn::IsInteger(sharp.GetNumberFmt());
            ispec.is_storage = desc.is_written;
            ispec.is_cube = sharp.IsCube();
            if (ispec.is_storage) {
                ispec.dst_select = sharp.DstSelect();
            } else {
                ispec.is_srgb = sharp.GetNumberFmt() == ::Gcn::NumberFormat::Srgb;
            }
            ispec.num_conversion = sharp.GetNumberConversion();
            ispec.num_bindings = desc.NumBindingsDirect(sharp);
            if (images[i] != ispec) {
                return false;
            }
        }

        if (bitset.none() && !cur_any_bit) {
            return true;
        }
        if (!(start == cur_start) || !buffers_match_size || !images_match_size ||
            !samplers_match_size) {
            return false;
        }
        for (size_t i = 0; i < cur_info.samplers.size(); ++i) {
            SamplerSpecialization sspec{};
            const auto sharp = cur_info.samplers[i].GetSharpDirect(flat);
            if (sharp) {
                sspec.force_unnormalized = sharp.force_unnormalized;
                sspec.force_degamma = sharp.force_degamma;
            }
            if (samplers[i] != sspec) {
                return false;
            }
        }
        return true;
    }

    void Serialize(Serialization::Archive& ar) const;
    bool Deserialize(Serialization::Archive& ar);
};

} // namespace Shader
