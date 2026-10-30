// SPDX-License-Identifier: GPL-2.0-or-later
// bbport cp: packet work run in stream order with the draws (Rasterizer::RunInOrder). Each takes
// a copy of its packet (or a small task record) as bytes.
#pragma once

#include "common/types.h"

namespace Vulkan {
class Rasterizer;
}

namespace BbCp {

struct EventWriteEop;

namespace Tasks {

/// ZPASS_DONE (the game's occlusion query): the counters' address and the depth blocks.
struct ZpassEvent {
    VAddr address;
    u32 pairs;
};

/// V3.3: Direct VkQueryPool occlusion query packet (0 = ClearAndBegin, 1 = End, 2 = ClearOnly).
struct DirectOcclusionEvent {
    VAddr base_address;
    u32 pairs;
    u32 query_op;
};

/// COPY_DATA from memory or GDS, or of a value into GDS.
struct CopyTask {
    VAddr dst;
    VAddr src;
    u64 value;
    u32 num_bytes;
    bool dst_gds;
    bool src_gds;
    bool fill; ///< write `value` instead of copying from `src`
};

void EventWriteEopTask(Vulkan::Rasterizer& rasterizer, const u8* data); ///< EventWriteEop
void EventWriteEosTask(Vulkan::Rasterizer& rasterizer, const u8* data); ///< EventWriteEos
void MemSemaphoreSignalTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DmaDataTask(Vulkan::Rasterizer& rasterizer, const u8* data);  ///< DmaData
void ZpassDoneTask(Vulkan::Rasterizer& rasterizer, const u8* data); ///< ZpassEvent
void CopyDataTask(Vulkan::Rasterizer& rasterizer, const u8* data);  ///< CopyTask
void FlipTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void GpuClockCopyTask(Vulkan::Rasterizer& rasterizer, const u8* data); ///< VAddr
void WriteDataTask(Vulkan::Rasterizer& rasterizer, const u8* data);    ///< the whole packet
void DirectVkCopyBatchTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkFillTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkClearColorTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkCopyTextureRegionTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkDecompressDepthTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkEliminateFastClearTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkResolveColorTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkUploadTextureTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkClearDepthStencilTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkReleaseLabelsBatchTask(Vulkan::Rasterizer& rasterizer, const u8* data);
void DirectVkOcclusionQueryTask(Vulkan::Rasterizer& rasterizer, const u8* data);

/// An end-of-pipe fence now (no draw recording thread).
void SignalEop(const EventWriteEop& eop, u64 seq);

} // namespace Tasks
} // namespace BbCp
