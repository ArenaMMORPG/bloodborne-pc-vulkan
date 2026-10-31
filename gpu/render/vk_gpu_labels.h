// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <span>
#include <utility>
#include "common/types.h"
#include "vulkan/vk_common.h"

namespace Vulkan {

/// End-of-pipe label using core Vulkan only. Must be recorded outside rendering. The caller
/// supplies a coherent, GPU-visible guest buffer, a 4/8-byte aligned offset and size 4 or 8.
/// Prior device writes become visible to host readers before the low (polled) word is written.
void RecordPortableGpuLabel(vk::CommandBuffer command, vk::Buffer buffer, u64 offset,
                            u64 value, u32 num_bytes);

/// Coalesced batch of 64-bit zero end-of-pipe label writes (V2.4 ReleaseLabelsBatch):
/// emits 1 barrier pair around all high/low dword writes instead of 3 barriers per label.
void RecordPortableGpuZeroLabelsBatch(vk::CommandBuffer command,
                                      std::span<const std::pair<vk::Buffer, u64>> targets);

/// Coalesced batch of 32-bit value end-of-pipe label writes (V5.2b ReleaseU32LabelsBatch):
/// emits 1 pre-barrier and 1 post-barrier around all 32-bit updateBuffer writes.
void RecordPortableGpuU32LabelsBatch(vk::CommandBuffer command,
                                     std::span<const std::pair<vk::Buffer, u64>> targets,
                                     u32 value);

} // namespace Vulkan
