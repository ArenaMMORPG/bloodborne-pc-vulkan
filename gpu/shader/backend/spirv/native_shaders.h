// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL -> SPIR-V shader replacement registry.
#pragma once

#include <vector>

#include "common/types.h"

namespace Shader::Backend::SPIRV {

/// Controlled by BB_NATIVE_SHADERS (default 1 = replace verified hot shaders with hand-crafted
/// native Vulkan 1.3 SPIR-V modules after verifying exact Location & Descriptor interface match;
/// 0 = always use GCN-translated SPIR-V).
[[nodiscard]] bool NativeShadersEnabled() noexcept;
[[nodiscard]] bool NativeShadersCustomRange() noexcept;
[[nodiscard]] u32 NativeShadersMinIdx() noexcept;
[[nodiscard]] u32 NativeShadersMaxIdx() noexcept;

/// Looks up `pgm_hash` in the native shader registry, verifies that all Input/Output Locations,
/// interpolation modes, and DescriptorSet/Binding types match `ref_spv` (post-HwInterp), rebases
/// descriptor bindings if needed, and writes the native SPIR-V into `out_spv`.
[[nodiscard]] bool TryReplaceWithNativeShader(u64 pgm_hash, const std::vector<u32>& ref_spv,
                                              std::vector<u32>& out_spv);

} // namespace Shader::Backend::SPIRV
