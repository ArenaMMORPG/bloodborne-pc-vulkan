// SPDX-License-Identifier: GPL-2.0-or-later
// bbport render: Rasterizer, DMA (fills, copies, GDS reads) and the guest memory events the
// caches follow: CPU writes, write faults, readbacks, GPU mappings.
//
// DMA into memory the GPU uses in place, or whose newest data is in a VRAM copy, is done by the
// GPU in stream order (BB_GUEST_IN_PLACE): on the CPU now it would land before earlier draws read
// the old data. Elsewhere (unbound memory: shader code, descriptors the command processor reads)
// the CPU does it in decode order.

#include "render/raster_internal.h"
#include "textures/direct_textures.h"
#include "../../our_cpp_100000_source/include/gx_direct_vulkan.h"

#include <xxhash.h>

namespace Vulkan {

namespace {
using BufferSlice = std::pair<const VideoCore::Buffer*, u64>;

bool GpuBarrierCoalesce() noexcept {
    static const bool on =
        gx_direct_vk::EnabledV6Sub(gx_direct_vk::kV6Sub_GpuBarrierCoalesce);
    return on;
}
} // namespace

/// Whether the GPU's view of [address, address + size) is not the game's memory alone: used in
/// place, written by the GPU, or mirrored in VRAM (BB_LAYER_MEMORY).
static bool GpuHolds(VideoCore::BufferCache& cache, VAddr address, u64 size) {
    return cache.IsAnyInPlace(address, size) || cache.IsRegionGpuModified(address, size) ||
           VideoCore::BufferCache::LayerMirrored(address, size);
}

void Rasterizer::FillBuffer(VAddr address, u32 num_bytes, u32 value, bool is_gds) {
    DrainDrawPipe();
    if (!is_gds && HasPendingHleCopies() && PendingHleCopiesConflict(address, num_bytes, true)) {
        FlushPendingHleCopies(*this);
    }
    if (!is_gds && direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }
    ASSERT_MSG(address % 4 == 0 && num_bytes % 4 == 0,
               "FillBuffer address and size must be a multiple of 4 bytes");
    static const bool n1_native_bufs =
        gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers);
    if (is_gds) {
        const u64 first_dw = address / 4;
        const u64 end_dw = (address + num_bytes + 3) / 4;
        for (u64 dw = first_dw; dw < end_dw && dw < gds_dword_mirror.size(); ++dw) {
            gds_dword_mirror[dw] = 0;
        }
    } else {
        InvalidateGdsMirrorRange(address, num_bytes);
        if (texture_cache.ClearMeta(address)) {
            begin_memo.valid = false;
        }
        if (n1_native_bufs && !VideoCore::GuestInPlace() &&
            !VideoCore::BufferCache::LayerActive()) {
            const auto [nat_buf, nat_off] =
                direct_buffers.TryResolveWrite(address, num_bytes, /*full_overwrite=*/true,
                                               /*track_access=*/true, &texture_cache);
            if (nat_buf) {
                buffer_cache.NoteExternalGpuModified(address, num_bytes);
                runtime.FillBuffer(nat_buf, nat_off, num_bytes, value);
                return;
            }
        }
        direct_buffers.OnGpuWrite(address, num_bytes);
        if (!GpuHolds(buffer_cache, address, num_bytes)) {
            BbFreeCheck::Check(address, num_bytes, &value, BbFreeCheck::DmaFill);
            u32* words = std::bit_cast<u32*>(address);
            std::fill(words, words + num_bytes / sizeof(u32), value);
            BbWriteLog::Note(address, words, num_bytes, BbWriteLog::Dma);
            if (!VideoCore::WriteTracking()) {
                NoteAssetWrite(address, num_bytes);
            }
            return;
        }
    }
    const auto [buffer, offset] =
        is_gds ? BufferSlice{buffer_cache.GetGdsBuffer(), address}
               : buffer_cache.ObtainBuffer(address, num_bytes, true, false, GpuBarrierCoalesce());
    runtime.FillBuffer(buffer, offset, num_bytes, value);
}

bool Rasterizer::DmaMayWriteOnCpu(VAddr dst, u32 num_bytes) {
    // FillBuffer's and CopyBuffer's test (without the new memory model DMA is always the CPU's).
    return !VideoCore::GuestInPlace() || !GpuHolds(buffer_cache, dst, num_bytes);
}

void Rasterizer::CopyBuffer(VAddr dst, VAddr src, u32 num_bytes, bool dst_gds, bool src_gds) {
    DrainDrawPipe();
    if (dst_gds && !src_gds) {
        if (num_bytes == 4 && (dst % 4) == 0 && (dst / 4) < gds_dword_mirror.size() && src != 0) {
            if (gds_dword_mirror[dst / 4] == src) {
                return;
            }
            gds_dword_mirror[dst / 4] = src;
        } else {
            const u64 first_dw = dst / 4;
            const u64 end_dw = (dst + num_bytes + 3) / 4;
            for (u64 dw = first_dw; dw < end_dw && dw < gds_dword_mirror.size(); ++dw) {
                gds_dword_mirror[dw] = 0;
            }
        }
    } else if (!dst_gds && src_gds) {
        InvalidateGdsMirrorRange(dst, num_bytes);
        if (num_bytes == 4 && (src % 4) == 0 && (src / 4) < gds_dword_mirror.size() && dst != 0) {
            gds_dword_mirror[src / 4] = dst;
        }
    } else if (!dst_gds && !src_gds) {
        InvalidateGdsMirrorRange(dst, num_bytes);
    } else {
        InvalidateGdsMirror();
    }
    if (HasPendingHleCopies() &&
        ((!dst_gds && PendingHleCopiesConflict(dst, num_bytes, true)) ||
         (!src_gds && PendingHleCopiesConflict(src, num_bytes, false)))) {
        FlushPendingHleCopies(*this);
    }
    if (direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }
    static const bool n1_native_bufs =
        gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers);
    if (n1_native_bufs && !dst_gds && !VideoCore::GuestInPlace() &&
        !VideoCore::BufferCache::LayerActive() &&
        (src_gds || num_bytes < 4096 || !texture_cache.FindImageFromRange(src, num_bytes))) {
        const auto [nat_dst, nat_dst_off] =
            direct_buffers.TryResolveWrite(dst, num_bytes, /*full_overwrite=*/true,
                                           /*track_access=*/true, &texture_cache);
        if (nat_dst) {
            const VideoCore::Buffer* src_buffer = nullptr;
            u64 src_offset = 0;
            if (src_gds) {
                src_buffer = buffer_cache.GetGdsBuffer();
                src_offset = src;
            } else {
                u64 src_sz = num_bytes;
                std::tie(src_buffer, src_offset) =
                    direct_buffers.TryResolveReadClamped(src, src_sz);
                if (!src_buffer || src_sz < num_bytes) {
                    std::tie(src_buffer, src_offset) =
                        buffer_cache.ObtainBuffer(src, num_bytes, false, true);
                }
            }
            buffer_cache.NoteExternalGpuModified(dst, num_bytes);
            const vk::BufferCopy copy{
                .srcOffset = src_offset, .dstOffset = nat_dst_off, .size = num_bytes};
            runtime.CopyBuffer(src_buffer, nat_dst, std::span{&copy, 1});
            return;
        }
    }
    if (!dst_gds) {
        direct_buffers.OnGpuWrite(dst, num_bytes);
    }
    // On the CPU when neither side is the GPU's: the source not newer in VRAM (BB_LAYER_MEMORY)
    // nor an image's memory.
    if (!dst_gds && !src_gds && !GpuHolds(buffer_cache, dst, num_bytes) &&
        !buffer_cache.IsRegionGpuModified(src, num_bytes) &&
        !VideoCore::BufferCache::LayerMirrored(src, num_bytes) &&
        !texture_cache.FindImageFromRange(src, num_bytes)) {
        BbFreeCheck::Check(dst, num_bytes, std::bit_cast<const void*>(src), BbFreeCheck::DmaCopy);
        std::memcpy(std::bit_cast<void*>(dst), std::bit_cast<void*>(src), num_bytes);
        BbWriteLog::Note(dst, std::bit_cast<const void*>(dst), num_bytes, BbWriteLog::Dma);
        if (!VideoCore::WriteTracking()) {
            NoteAssetWrite(dst, num_bytes);
        }
        return;
    }
    texture_cache.InvalidateMemoryFromGPU(dst, num_bytes);
    const auto* gds = buffer_cache.GetGdsBuffer();
    const VideoCore::Buffer* src_buffer = nullptr;
    u64 src_offset = 0;
    if (src_gds) {
        src_buffer = gds;
        src_offset = src;
    } else if (n1_native_bufs) {
        u64 src_sz = num_bytes;
        std::tie(src_buffer, src_offset) = direct_buffers.TryResolveReadClamped(src, src_sz);
        if (!src_buffer || src_sz < num_bytes) {
            std::tie(src_buffer, src_offset) =
                buffer_cache.ObtainBuffer(src, num_bytes, false, true);
        }
    } else {
        std::tie(src_buffer, src_offset) = buffer_cache.ObtainBuffer(src, num_bytes, false, true);
    }
    const auto [dst_buffer, dst_offset] =
        dst_gds ? BufferSlice{gds, dst}
                : buffer_cache.ObtainBuffer(dst, num_bytes, true, true, GpuBarrierCoalesce());
    const vk::BufferCopy copy{.srcOffset = src_offset, .dstOffset = dst_offset, .size = num_bytes};
    runtime.CopyBuffer(src_buffer, dst_buffer, std::span{&copy, 1});
}

void Rasterizer::DirectCopyBuffers(std::span<const DirectCopyRegion> regions) {
    if (regions.empty()) {
        return;
    }
    for (const auto& reg : regions) {
        if (reg.byte_size != 0 && reg.dst_va != 0) {
            InvalidateGdsMirrorRange(reg.dst_va, reg.byte_size);
        }
    }
    if (HasPendingHleCopies()) {
        for (const auto& reg : regions) {
            if (reg.byte_size == 0) {
                continue;
            }
            if (PendingHleCopiesConflict(reg.dst_va, reg.byte_size, true) ||
                PendingHleCopiesConflict(reg.src_va, reg.byte_size, false)) {
                FlushPendingHleCopies(*this);
                break;
            }
        }
    }

    if (direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }

    if (regions.size() == 1) {
        const auto& r0 = regions[0];
        if (r0.byte_size >= 4096 && r0.dst_va && r0.src_va &&
            r0.reserved != gx_direct_vk::kCopyFlagKnownCpuSource) {
            const u64 cur_tex_gen = texture_cache.RegistryGeneration() + 1ULL;
            const size_t slot = static_cast<size_t>(
                (((r0.src_va >> 12) * 0x9E3779B97F4A7C15ULL) ^
                 ((r0.dst_va >> 12) * 0xBF58476D1CE4E5B9ULL)) &
                (kDirectBufImgCopyMemoSlots - 1));
            const auto& m = direct_buf_img_copy_memo[slot];
            if (m.tex_gen == cur_tex_gen && m.src_va == r0.src_va && m.dst_va == r0.dst_va &&
                m.byte_size == r0.byte_size && m.src_id && m.dst_id &&
                !VideoCore::DirectBuffers::OverlapsNativeOwnedRange(r0.dst_va, r0.byte_size)) {
                auto& src_img = texture_cache.GetImage(m.src_id);
                auto& dst_img = texture_cache.GetImage(m.dst_id);
                runtime.CopyColorAndDepth(&src_img, &dst_img);
                dst_img.gpu_write_tick = scheduler.CurrentTick();
                return;
            }
        }
    }

    static const bool v7_inpass_rename =
        gx_direct_vk::EnabledV7Sub(gx_direct_vk::kV7Sub_StageAFastConstAndPrep);
    static const bool n1_native_bufs =
        gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers);

    // N1.2b: Zero-RenderPass-break native VkBuffer renaming for FlushBufferUpdates_A/B
    // (`DEFAULT` ConstantBuffers and `DEFAULT` buffers updated from CPU / cueHeap).
    static thread_local std::vector<u8> native_handled;
    static thread_local std::vector<VideoCore::DirectBuffers::NativeCopyRegion> nat_regs;
    native_handled.assign(regions.size(), 0u);
    if (n1_native_bufs && !VideoCore::GuestInPlace() && !VideoCore::BufferCache::LayerActive()) {
        nat_regs.resize(regions.size());
        size_t active_count = 0;
        for (size_t idx = 0; idx < regions.size(); ++idx) {
            const auto& reg = regions[idx];
            const u32 sz = (reg.dst_va && reg.src_va) ? reg.byte_size : 0u;
            nat_regs[idx] = {
                .dst_va = reg.dst_va,
                .src_va = reg.src_va,
                .byte_size = sz,
                .flags = reg.reserved,
            };
            if (sz != 0) {
                ++active_count;
            }
        }
        if (active_count == 0) {
            return;
        }
        direct_buffers.StageNativeCpuCopyBatch(
            std::span<const VideoCore::DirectBuffers::NativeCopyRegion>{nat_regs.data(),
                                                                        nat_regs.size()},
            std::span<u8>{native_handled.data(), native_handled.size()}, buffer_cache,
            texture_cache, memory);
        size_t handled_count = 0;
        for (size_t idx = 0; idx < regions.size(); ++idx) {
            if (nat_regs[idx].byte_size != 0 && native_handled[idx] != 0u) {
                ++handled_count;
            }
        }
        if (handled_count == active_count) {
            return;
        }
    }

    const auto is_native_handled = [](size_t idx) noexcept {
        return idx < native_handled.size() && native_handled[idx] != 0u;
    };

    struct ResolvedBufCopy {
        const VideoCore::Buffer* src_buf;
        const VideoCore::Buffer* dst_buf;
        u64 src_off;
        u64 dst_off;
        u32 size;
    };
    static thread_local std::vector<ResolvedBufCopy> resolved_copies;
    resolved_copies.clear();
    if (resolved_copies.capacity() < regions.size()) {
        resolved_copies.reserve(regions.size());
    }

    const auto ranges_overlap = [](u64 a_off, u64 a_sz, u64 b_off, u64 b_sz) noexcept {
        return a_off < (b_off + b_sz) && b_off < (a_off + a_sz);
    };

    // V7.2: While inside an active RenderPass (e.g. 2560x1440 6-MRT G-Buffer or Lighting),
    // rename remaining non-native transient `cueHeap -> dst_va` UpdateSubresource buffer copies into
    // `stream_buffer` without calling `scheduler.EndRendering()`, coalescing the `arena` writeback
    // once when the RenderPass naturally ends.
    if (v7_inpass_rename && scheduler.IsRendering() && !VideoCore::GuestInPlace() &&
        !VideoCore::BufferCache::LayerActive()) {
        static thread_local std::vector<VideoCore::BufferCache::InPassCopySlice> inpass_slices;
        inpass_slices.clear();
        const u64 max_slice_bytes = n1_native_bufs ? 512_KB : 64_KB;
        const u64 max_batch_bytes = n1_native_bufs ? 4_MB : 512_KB;
        u64 total_inpass_bytes = 0;
        bool all_eligible = true;
        for (size_t idx = 0; idx < regions.size(); ++idx) {
            if (is_native_handled(idx)) {
                continue;
            }
            const auto& reg = regions[idx];
            if (!reg.dst_va || !reg.src_va || reg.byte_size == 0) {
                continue;
            }
            const bool known_cpu_src =
                (reg.reserved == gx_direct_vk::kCopyFlagKnownCpuSource) &&
                !VideoCore::DirectBuffers::OverlapsNativeOwnedRange(reg.src_va, reg.byte_size) &&
                !direct_buffers.OverlapsLiveEntry(reg.src_va, reg.byte_size);
            const bool valid_cpu_src =
                VideoCore::DirectBuffers::IsTransientCueHeap(reg.src_va, reg.byte_size) ||
                (n1_native_bufs && memory->ClampRangeSize(reg.src_va, reg.byte_size) == reg.byte_size);
            if (reg.byte_size > max_slice_bytes || !valid_cpu_src ||
                (n1_native_bufs &&
                 (VideoCore::DirectBuffers::OverlapsNativeOwnedRange(reg.dst_va, reg.byte_size) ||
                  direct_buffers.OverlapsLiveEntry(reg.dst_va, reg.byte_size))) ||
                (!known_cpu_src &&
                 (buffer_cache.IsRegionGpuModified(reg.src_va, reg.byte_size) ||
                  texture_cache.HasOverlappingImage(reg.src_va, reg.byte_size))) ||
                texture_cache.HasOverlappingImage(reg.dst_va, reg.byte_size) ||
                texture_cache.IsMeta(reg.dst_va)) {
                all_eligible = false;
                break;
            }
            for (size_t prev = 0; prev < idx; ++prev) {
                const auto& pr = regions[prev];
                if (pr.dst_va && pr.byte_size != 0 &&
                    ranges_overlap(reg.src_va, reg.byte_size, pr.dst_va, pr.byte_size)) {
                    all_eligible = false;
                    break;
                }
            }
            if (!all_eligible) {
                break;
            }
            const u64 aligned_off = (total_inpass_bytes + 15ULL) & ~15ULL;
            const u64 padded_slice = (static_cast<u64>(reg.byte_size) + 15ULL) & ~15ULL;
            if (aligned_off + padded_slice > max_batch_bytes) {
                all_eligible = false;
                break;
            }
            inpass_slices.push_back(VideoCore::BufferCache::InPassCopySlice{
                .src_va = reg.src_va,
                .dst_va = reg.dst_va,
                .byte_size = reg.byte_size,
                .rel_offset = static_cast<u32>(aligned_off),
            });
            total_inpass_bytes = aligned_off + padded_slice;
        }
        if (all_eligible && !inpass_slices.empty()) {
            for (const auto& s : inpass_slices) {
                direct_buffers.OnGpuWrite(s.dst_va, s.byte_size);
            }
            if (buffer_cache.StageInPassCueCopies(inpass_slices, total_inpass_bytes)) {
                return;
            }
        }
    }

    static thread_local std::vector<VideoCore::BufferCache::TransientHostSlice> transient_slices;
    static thread_local std::vector<s32> transient_index_by_reg;
    transient_slices.clear();
    transient_index_by_reg.assign(regions.size(), -1);
    const VideoCore::Buffer* transient_buf = nullptr;
    u64 transient_base_off = 0;

    if (!VideoCore::GuestInPlace()) {
        u64 total_transient_bytes = 0;
        for (size_t idx = 0; idx < regions.size(); ++idx) {
            if (is_native_handled(idx)) {
                continue;
            }
            const auto& reg = regions[idx];
            if (!reg.dst_va || !reg.src_va || reg.byte_size == 0) {
                continue;
            }
            const bool known_cpu_src =
                (reg.reserved == gx_direct_vk::kCopyFlagKnownCpuSource) &&
                !VideoCore::DirectBuffers::OverlapsNativeOwnedRange(reg.src_va, reg.byte_size) &&
                !direct_buffers.OverlapsLiveEntry(reg.src_va, reg.byte_size);
            if (!VideoCore::DirectBuffers::IsTransientCueHeap(reg.src_va, reg.byte_size) ||
                (!known_cpu_src && buffer_cache.IsRegionGpuModified(reg.src_va, reg.byte_size)) ||
                buffer_cache.HasCueStreamMemo(reg.src_va, reg.byte_size)) {
                continue;
            }
            bool written_earlier_in_batch = false;
            for (size_t prev = 0; prev < idx; ++prev) {
                const auto& pr = regions[prev];
                if (pr.dst_va && pr.byte_size != 0 &&
                    ranges_overlap(reg.src_va, reg.byte_size, pr.dst_va, pr.byte_size)) {
                    written_earlier_in_batch = true;
                    break;
                }
            }
            if (written_earlier_in_batch) {
                continue;
            }
            const u64 aligned_off = (total_transient_bytes + 15ULL) & ~15ULL;
            if (aligned_off + reg.byte_size > std::numeric_limits<u32>::max()) {
                break;
            }
            transient_index_by_reg[idx] = static_cast<s32>(transient_slices.size());
            transient_slices.push_back(VideoCore::BufferCache::TransientHostSlice{
                .src_va = reg.src_va,
                .byte_size = reg.byte_size,
                .staging_rel_offset = static_cast<u32>(aligned_off),
            });
            total_transient_bytes = aligned_off + reg.byte_size;
        }
        if (!transient_slices.empty()) {
            std::tie(transient_buf, transient_base_off) =
                buffer_cache.StageTransientHostBatch(transient_slices, total_transient_bytes);
        }
    }

    const bool full_ov = GpuBarrierCoalesce();
    bool wrote_native_buf = false;
    const auto flush_resolved = [&] {
        if (resolved_copies.empty()) {
            return;
        }
        const VideoCore::Buffer* batch_src = nullptr;
        const VideoCore::Buffer* batch_dst = nullptr;
        std::array<vk::BufferCopy, 128> batch_copies{};
        u32 batch_count = 0;
        const auto flush_batch = [&] {
            if (batch_count != 0) {
                runtime.CopyBuffer(batch_src, batch_dst,
                                   std::span{batch_copies.data(), batch_count});
                batch_count = 0;
            }
        };
        for (const auto& item : resolved_copies) {
            bool can_coalesce =
                (batch_count > 0 && item.src_buf == batch_src && item.dst_buf == batch_dst);
            if (item.src_buf == item.dst_buf &&
                ranges_overlap(item.src_off, item.size, item.dst_off, item.size)) {
                can_coalesce = false;
            }
            if (can_coalesce) {
                for (u32 i = 0; i < batch_count; ++i) {
                    const auto& prev = batch_copies[i];
                    if (ranges_overlap(item.dst_off, item.size, prev.dstOffset, prev.size) ||
                        (item.src_buf == batch_dst &&
                         ranges_overlap(item.src_off, item.size, prev.dstOffset, prev.size)) ||
                        (item.dst_buf == batch_src &&
                         ranges_overlap(item.dst_off, item.size, prev.srcOffset, prev.size))) {
                        can_coalesce = false;
                        break;
                    }
                }
            }
            if (can_coalesce && full_ov) {
                auto& last = batch_copies[batch_count - 1];
                if (last.srcOffset + last.size == item.src_off &&
                    last.dstOffset + last.size == item.dst_off) {
                    last.size += item.size;
                    continue;
                }
            }
            if (!can_coalesce || batch_count >= batch_copies.size()) {
                flush_batch();
                batch_src = item.src_buf;
                batch_dst = item.dst_buf;
            }
            batch_copies[batch_count++] = vk::BufferCopy{
                .srcOffset = item.src_off,
                .dstOffset = item.dst_off,
                .size = item.size,
            };
        }
        flush_batch();
        resolved_copies.clear();
    };

    for (size_t idx = 0; idx < regions.size(); ++idx) {
        if (is_native_handled(idx)) {
            continue;
        }
        const auto& reg = regions[idx];
        if (!reg.dst_va || !reg.src_va || reg.byte_size == 0) {
            continue;
        }

        const s32 transient_idx = transient_index_by_reg[idx];
        if (n1_native_bufs && !VideoCore::GuestInPlace() &&
            !VideoCore::BufferCache::LayerActive() &&
            (reg.byte_size < 4096 ||
             !texture_cache.FindImageFromRange(reg.src_va, reg.byte_size, true, 4_MB))) {
            const auto [nat_dst, nat_dst_off] =
                direct_buffers.TryResolveWrite(reg.dst_va, reg.byte_size, /*full_overwrite=*/true,
                                               /*track_access=*/true, &texture_cache);
            if (nat_dst) {
                const VideoCore::Buffer* src_buf = nullptr;
                u64 src_off = 0;
                if (transient_buf && transient_idx >= 0) {
                    const auto& slice = transient_slices[static_cast<size_t>(transient_idx)];
                    src_buf = transient_buf;
                    src_off = transient_base_off + slice.staging_rel_offset;
                } else {
                    u64 src_sz = reg.byte_size;
                    std::tie(src_buf, src_off) =
                        direct_buffers.TryResolveReadClamped(reg.src_va, src_sz);
                    if (!src_buf || src_sz < reg.byte_size) {
                        std::tie(src_buf, src_off) =
                            buffer_cache.ObtainBuffer(reg.src_va, reg.byte_size, false);
                    }
                }
                if (src_buf) {
                    buffer_cache.NoteExternalGpuModified(reg.dst_va, reg.byte_size);
                    resolved_copies.push_back(ResolvedBufCopy{
                        src_buf, nat_dst, src_off, nat_dst_off, reg.byte_size});
                    wrote_native_buf = true;
                }
                continue;
            }
        }

        direct_buffers.OnGpuWrite(reg.dst_va, reg.byte_size);

        if (transient_buf && transient_idx >= 0) {
            const auto& slice = transient_slices[static_cast<size_t>(transient_idx)];
            texture_cache.InvalidateMemoryFromGPU(reg.dst_va, reg.byte_size);
            buffer_cache.force_writes_in_place = true;
            const auto [dst_buf, dst_off] =
                buffer_cache.ObtainBuffer(reg.dst_va, reg.byte_size, true, false, full_ov);
            buffer_cache.force_writes_in_place = false;
            if (!dst_buf) {
                continue;
            }
            resolved_copies.push_back(ResolvedBufCopy{
                transient_buf,
                dst_buf,
                transient_base_off + slice.staging_rel_offset,
                dst_off,
                reg.byte_size,
            });
            continue;
        }

        // Direct VkImage -> VkImage copy when the game copies render-target / proxy texture
        // memory through its 1D compute buffer copy helpers (0x26b2d30 / 0x26b2670).
        if (reg.byte_size >= 4096) {
            constexpr u64 MaxTail = 4_MB;
            auto src_img_id =
                texture_cache.FindImageFromRange(reg.src_va, reg.byte_size, true, MaxTail);
            auto dst_img_id =
                texture_cache.FindImageFromRange(reg.dst_va, reg.byte_size, false, MaxTail);
            if (src_img_id && !dst_img_id) {
                const auto& src_img = texture_cache.GetImage(src_img_id);
                if (reg.byte_size >= src_img.info.guest_size &&
                    reg.byte_size - src_img.info.guest_size <= MaxTail &&
                    !src_img.info.props.is_depth) {
                    VideoCore::TextureCache::ImageDesc dst_desc{};
                    dst_desc.info = src_img.info;
                    dst_desc.info.guest_address = reg.dst_va;
                    dst_desc.info.meta_info = {};
                    dst_desc.info.stencil_addr = 0;
                    dst_desc.info.stencil_size = 0;
                    dst_desc.type = VideoCore::TextureCache::BindingType::RenderTarget;
                    dst_img_id = texture_cache.FindImage(dst_desc, true);
                }
            }
            if (src_img_id && dst_img_id && src_img_id != dst_img_id) {
                auto& src_img = texture_cache.GetImage(src_img_id);
                auto& dst_img = texture_cache.GetImage(dst_img_id);
                const u64 image_size = src_img.info.guest_size;
                if (image_size == dst_img.info.guest_size &&
                    src_img.info.pitch == dst_img.info.pitch && reg.byte_size >= image_size &&
                    reg.byte_size - image_size <= MaxTail &&
                    src_img.info.num_bits == dst_img.info.num_bits) {
                    flush_resolved();
                    runtime.CopyColorAndDepth(&src_img, &dst_img);
                    dst_img.gpu_write_tick = scheduler.CurrentTick();
                    if (const u64 tail = reg.byte_size - image_size) {
                        const VAddr src_tail = src_img.info.guest_address + image_size;
                        const VAddr dst_tail = dst_img.info.guest_address + image_size;
                        if (texture_cache.FindImageFromRange(dst_tail, tail, false)) {
                            const auto [tail_src_buf, tail_src_off] =
                                buffer_cache.ObtainBuffer(src_tail, u32(tail), false);
                            const auto [tail_dst_buf, tail_dst_off] =
                                buffer_cache.ObtainBuffer(dst_tail, u32(tail), true, false, full_ov);
                            const vk::BufferCopy tcopy{tail_src_off, tail_dst_off, tail};
                            runtime.CopyBuffer(tail_src_buf, tail_dst_buf, std::span{&tcopy, 1});
                            texture_cache.InvalidateMemoryFromGPU(dst_tail, tail);
                        }
                    } else if (reg.reserved != gx_direct_vk::kCopyFlagKnownCpuSource &&
                               !VideoCore::DirectBuffers::OverlapsNativeOwnedRange(reg.dst_va,
                                                                                   reg.byte_size)) {
                        const size_t slot = static_cast<size_t>(
                            (((reg.src_va >> 12) * 0x9E3779B97F4A7C15ULL) ^
                             ((reg.dst_va >> 12) * 0xBF58476D1CE4E5B9ULL)) &
                            (kDirectBufImgCopyMemoSlots - 1));
                        direct_buf_img_copy_memo[slot] = DirectBufImgCopyMemo{
                            .src_va = reg.src_va,
                            .dst_va = reg.dst_va,
                            .byte_size = reg.byte_size,
                            .src_id = src_img_id,
                            .dst_id = dst_img_id,
                            .tex_gen = texture_cache.RegistryGeneration() + 1ULL,
                        };
                    }
                    continue;
                }
            }
        }

        texture_cache.InvalidateMemoryFromGPU(reg.dst_va, reg.byte_size);
        const VideoCore::Buffer* src_buf = nullptr;
        u64 src_off = 0;
        if (n1_native_bufs) {
            u64 src_sz = reg.byte_size;
            std::tie(src_buf, src_off) = direct_buffers.TryResolveReadClamped(reg.src_va, src_sz);
            if (!src_buf || src_sz < reg.byte_size) {
                src_buf = nullptr;
            }
        }
        buffer_cache.force_writes_in_place = true;
        if (!src_buf) {
            std::tie(src_buf, src_off) = buffer_cache.ObtainBuffer(reg.src_va, reg.byte_size, false);
        }
        const auto [dst_buf, dst_off] =
            buffer_cache.ObtainBuffer(reg.dst_va, reg.byte_size, true, false, full_ov);
        buffer_cache.force_writes_in_place = false;
        if (!src_buf || !dst_buf) {
            continue;
        }
        resolved_copies.push_back(
            ResolvedBufCopy{src_buf, dst_buf, src_off, dst_off, reg.byte_size});
    }
    flush_resolved();
    (void)wrote_native_buf;
}

void Rasterizer::DirectFillBuffer(VAddr dst, u32 num_bytes, u32 value) {
    if (!dst || num_bytes == 0 || ((dst | num_bytes) % 4) != 0) {
        return;
    }
    InvalidateGdsMirrorRange(dst, num_bytes);
    if (HasPendingHleCopies() && PendingHleCopiesConflict(dst, num_bytes, true)) {
        FlushPendingHleCopies(*this);
    }

    // 1. Metadata (CMASK / HTILE) fast clear: updates tracked state without touching buffers.
    if (texture_cache.ClearMeta(dst)) {
        begin_memo.valid = false;
        direct_buffers.OnGpuWrite(dst, num_bytes);
        return;
    }

    // 2. Direct VkImage / slice clear when 0x26b5380 clears a tiled color render target in VRAM.
    if (num_bytes >= 4096) {
        constexpr u64 MaxClearTail = 512_KB;
        constexpr u32 kVec4Bits = 128u;
        const u32 equiv_groups = u32((num_bytes + 2047u) / 2048u);
        thread_local boost::container::static_vector<VideoCore::ImageInfo, 8> clear_templates;
        s32 slice_layer = -1;
        auto image_id = texture_cache.FindImageFromRange(dst, num_bytes, false, MaxClearTail);
        if (image_id) {
            const auto& cand = texture_cache.GetImage(image_id);
            if (num_bytes < cand.info.guest_size) {
                image_id = {};
            }
        }
        if (!image_id) {
            std::tie(image_id, slice_layer) =
                texture_cache.FindImageSliceFromRange(dst, num_bytes);
        }
        if (!image_id && equiv_groups >= 4096) {
            for (const auto& tmpl : clear_templates) {
                if (tmpl.guest_size == num_bytes && (value == 0u || tmpl.num_bits == kVec4Bits)) {
                    VideoCore::TextureCache::ImageDesc dst_desc{};
                    dst_desc.info = tmpl;
                    dst_desc.info.guest_address = dst;
                    dst_desc.info.meta_info = {};
                    dst_desc.info.stencil_addr = 0;
                    dst_desc.info.stencil_size = 0;
                    dst_desc.type = VideoCore::TextureCache::BindingType::RenderTarget;
                    image_id = texture_cache.FindImage(dst_desc, true);
                    break;
                }
            }
        }
        if (image_id) {
            auto& image = texture_cache.GetImage(image_id);
            const bool size_ok =
                slice_layer >= 0 ||
                (num_bytes >= image.info.guest_size &&
                 num_bytes - image.info.guest_size <= MaxClearTail);
            const bool fmt_ok =
                !image.info.props.is_depth && (value == 0u || image.info.num_bits == kVec4Bits);
            if (size_ok && fmt_ok) {
                direct_buffers.OnGpuWrite(dst, num_bytes);
                if (slice_layer < 0 && equiv_groups >= 4096 &&
                    image.info.resources.levels == 1 && image.info.resources.layers == 1 &&
                    image.info.guest_size == num_bytes) {
                    bool known = false;
                    for (auto& tmpl : clear_templates) {
                        if (tmpl.guest_size == image.info.guest_size &&
                            tmpl.num_bits == image.info.num_bits) {
                            tmpl = image.info;
                            known = true;
                            break;
                        }
                    }
                    if (!known && clear_templates.size() < clear_templates.capacity()) {
                        clear_templates.push_back(image.info);
                    }
                }
                const float f = std::bit_cast<float>(value);
                const vk::ClearValue clear{
                    .color = {.float32 = std::array<float, 4>{f, f, f, f}},
                };
                const VideoCore::SubresourceRange range =
                    slice_layer >= 0
                        ? VideoCore::SubresourceRange{
                              .base = {.level = 0, .layer = u32(slice_layer)},
                              .extent = {.levels = 1, .layers = 1}}
                        : VideoCore::SubresourceRange{
                              .base = {.level = 0, .layer = 0},
                              .extent = image.info.resources};
                runtime.ClearImage(&image, range, clear);
                image.gpu_write_tick = scheduler.CurrentTick();
                return;
            }
        }
    }

    if (direct_buffers.NeedsProcessEvents(scheduler.IsRendering())) [[unlikely]] {
        direct_buffers.ProcessEvents(memory);
    }

    static const bool n1_native_bufs =
        gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers);
    if (n1_native_bufs && !VideoCore::GuestInPlace() && !VideoCore::BufferCache::LayerActive()) {
        const auto [nat_dst, nat_off] =
            direct_buffers.TryResolveWrite(dst, num_bytes, /*full_overwrite=*/true,
                                           /*track_access=*/true, &texture_cache);
        if (nat_dst) {
            buffer_cache.NoteExternalGpuModified(dst, num_bytes);
            runtime.FillBuffer(nat_dst, nat_off, num_bytes, value);
            return;
        }
    }

    direct_buffers.OnGpuWrite(dst, num_bytes);

    // 3. Regular GPU buffer clear (marks gpu_modified_ranges via ObtainBuffer(..., true)).
    const auto [dst_buf, dst_off] =
        buffer_cache.ObtainBuffer(dst, num_bytes, true, false, GpuBarrierCoalesce());
    if (dst_buf) {
        runtime.FillBuffer(dst_buf, dst_off, num_bytes, value);
    }
}

void Rasterizer::DirectClearColorImage(const DirectClearColorArgs& args) {
    if (!args.dst_va || args.byte_size == 0 || args.num_bits == 0) {
        return;
    }
    if (HasPendingHleCopies() && PendingHleCopiesConflict(args.dst_va, args.byte_size, true)) {
        FlushPendingHleCopies(*this);
    }

    vk::ClearValue clear{};
    std::memcpy(clear.color.uint32.data(), args.clear_rgba, sizeof(args.clear_rgba));

    constexpr u64 MaxClearTail = 512_KB;
    const u32 bytes_per_elem = std::max(args.num_bits / 8u, 1u);
    const u32 equiv_groups = u32((args.byte_size + 128u * bytes_per_elem - 1u) / (128u * bytes_per_elem));
    thread_local boost::container::static_vector<VideoCore::ImageInfo, 8> clear_templates;

    s32 slice_layer = -1;
    auto image_id = texture_cache.FindImageFromRange(args.dst_va, args.byte_size, false, MaxClearTail);
    if (image_id) {
        const auto& cand = texture_cache.GetImage(image_id);
        if (args.byte_size < cand.info.guest_size) {
            image_id = {};
        }
    }
    if (!image_id) {
        std::tie(image_id, slice_layer) =
            texture_cache.FindImageSliceFromRange(args.dst_va, args.byte_size);
    }
    if (!image_id && equiv_groups >= 4096) {
        for (const auto& tmpl : clear_templates) {
            if (tmpl.guest_size == args.byte_size && tmpl.num_bits == args.num_bits) {
                VideoCore::TextureCache::ImageDesc dst_desc{};
                dst_desc.info = tmpl;
                dst_desc.info.guest_address = args.dst_va;
                dst_desc.info.meta_info = {};
                dst_desc.info.stencil_addr = 0;
                dst_desc.info.stencil_size = 0;
                dst_desc.type = VideoCore::TextureCache::BindingType::RenderTarget;
                image_id = texture_cache.FindImage(dst_desc, true);
                break;
            }
        }
    }
    if (image_id) {
        auto& image = texture_cache.GetImage(image_id);
        const bool size_ok =
            slice_layer >= 0 ||
            (args.byte_size >= image.info.guest_size &&
             args.byte_size - image.info.guest_size <= MaxClearTail);
        const bool fmt_ok = !image.info.props.is_depth && image.info.num_bits == args.num_bits;
        if (size_ok && fmt_ok) {
            if (slice_layer < 0 && equiv_groups >= 4096 &&
                image.info.resources.levels == 1 && image.info.resources.layers == 1 &&
                image.info.guest_size == args.byte_size) {
                bool known = false;
                for (auto& tmpl : clear_templates) {
                    if (tmpl.guest_size == image.info.guest_size &&
                        tmpl.num_bits == image.info.num_bits) {
                        tmpl = image.info;
                        known = true;
                        break;
                    }
                }
                if (!known && clear_templates.size() < clear_templates.capacity()) {
                    clear_templates.push_back(image.info);
                }
            }
            const VideoCore::SubresourceRange range =
                slice_layer >= 0
                    ? VideoCore::SubresourceRange{
                          .base = {.level = 0, .layer = u32(slice_layer)},
                          .extent = {.levels = 1, .layers = 1}}
                    : VideoCore::SubresourceRange{
                          .base = {.level = 0, .layer = 0},
                          .extent = image.info.resources};
            runtime.ClearImage(&image, range, clear);
            image.gpu_write_tick = scheduler.CurrentTick();
            return;
        }
    }

    // Exact Gcn::Image T# fallback when the surface has not been bound as an attachment yet.
    Gcn::Image gcn_img{};
    std::memcpy(&gcn_img, args.tsharp, sizeof(gcn_img));
    if (gcn_img.Valid() && !Gcn::IsBlockCoded(gcn_img.GetDataFmt())) {
        Shader::ImageResource res_desc{};
        res_desc.is_written = true;
        res_desc.needs_native = true;
        VideoCore::TextureCache::ImageDesc img_desc{gcn_img, res_desc};
        const auto resolved_id = texture_cache.FindImage(img_desc);
        if (resolved_id) {
            auto& image = texture_cache.GetImage(resolved_id);
            if (!image.info.props.is_depth) {
                const u32 mip = std::min(img_desc.view_info.range.base.level,
                                         image.info.resources.levels - 1);
                const u32 layer = std::min(img_desc.view_info.range.base.layer,
                                           image.info.resources.layers - 1);
                const VideoCore::SubresourceRange range{
                    .base = {.level = mip, .layer = layer},
                    .extent = {.levels = 1, .layers = 1},
                };
                runtime.ClearImage(&image, range, clear);
                image.gpu_write_tick = scheduler.CurrentTick();
                return;
            }
        }
    }

    if ((args.clear_rgba[0] | args.clear_rgba[1] | args.clear_rgba[2] | args.clear_rgba[3]) == 0u &&
        ((args.dst_va | args.byte_size) % 4) == 0) {
        DirectFillBuffer(args.dst_va, args.byte_size, 0u);
    }
}

void Rasterizer::DirectCopyTextureRegion(const DirectCopyTextureRegionArgs& args) {
    if (args.copy_w == 0 || args.copy_h == 0 || args.copy_d == 0) {
        return;
    }
    pending_tex_copies.push_back(args);
    if (pending_tex_copies.size() < 16 && NextPipePacketIsTextureRegionCopy()) {
        return;
    }
    FlushPendingTextureRegionCopies();
}

void Rasterizer::FlushPendingTextureRegionCopies() {
    if (pending_tex_copies.empty()) {
        return;
    }
    if (HasPendingHleCopies()) {
        for (const auto& args : pending_tex_copies) {
            if ((args.dst_va && args.dst_byte_size &&
                 PendingHleCopiesConflict(args.dst_va, args.dst_byte_size, true)) ||
                (args.src_va && args.src_byte_size &&
                 PendingHleCopiesConflict(args.src_va, args.src_byte_size, false))) {
                FlushPendingHleCopies(*this);
                break;
            }
        }
    }

    const auto resolve_copy_image = [&](const u64 raw_words[4], bool is_written,
                                        VideoCore::ImageId& out_id,
                                        VideoCore::SubresourceRange& out_view_range) -> bool {
        Gcn::Image sharp{};
        std::memcpy(&sharp, raw_words, sizeof(sharp));
        u8 srv_gen8 = 0u;
        const u16 srv_uid = VideoCore::DirectTextures::UnpackAndCleanSrv(sharp, srv_gen8);
        if (!sharp.Valid()) {
            return false;
        }
        Shader::ImageResource res{};
        res.is_written = is_written;
        res.needs_native = true;
        auto& entry = ResolveImageDescEntry(sharp, srv_uid, srv_gen8, res, nullptr);
        const u64 current_gen = texture_cache.RegistryGeneration();
        if ((entry.found_generation == current_gen ||
             (entry.found_direct_immutable &&
              texture_cache.IsRegisteredImmutable(entry.found_id, entry.found_uid))) &&
            entry.found_id && !BbToggle::Disabled(BbToggle::TextureBindingMemo)) {
            entry.found_generation = current_gen;
            out_id = entry.found_id;
            out_view_range = entry.found_desc.view_info.range;
            texture_cache.MarkFound(out_id);
            texture_cache.UpdateImage(out_id);
            return true;
        }
        const u64 gen_before = texture_cache.RegistryGeneration();
        out_id = texture_cache.FindImage(entry.desc);
        if (!out_id) {
            return false;
        }
        out_view_range = entry.desc.view_info.range;
        if (gen_before == texture_cache.RegistryGeneration()) {
            auto& img = texture_cache.GetImageUntouched(out_id);
            entry.found_generation = gen_before;
            entry.found_id = out_id;
            entry.found_uid = img.image_uid;
            entry.found_direct_immutable =
                True(img.flags & VideoCore::ImageFlagBits::DirectImmutable) &&
                !texture_cache.GetAssociatedDepth(img);
            entry.found_desc = entry.desc;
            entry.view_memo = {};
        }
        texture_cache.UpdateImage(out_id);
        return true;
    };

    VideoCore::ImageId group_src_id{};
    VideoCore::ImageId group_dst_id{};
    boost::container::small_vector<VideoCore::SubresourceRange, 16> group_src_ranges;
    boost::container::small_vector<VideoCore::SubresourceRange, 16> group_dst_ranges;
    boost::container::small_vector<vk::ImageCopy, 16> group_regions;

    const auto flush_group = [&]() {
        if (group_regions.empty()) {
            return;
        }
        auto& src_img = texture_cache.GetImage(group_src_id);
        auto& dst_img = texture_cache.GetImage(group_dst_id);
        runtime.CopyImageSubresources(&src_img, &dst_img, group_src_ranges, group_dst_ranges,
                                      group_regions);
        dst_img.gpu_write_tick = scheduler.CurrentTick();
        group_src_ranges.clear();
        group_dst_ranges.clear();
        group_regions.clear();
    };

    for (const auto& args : pending_tex_copies) {
        VideoCore::ImageId src_id{};
        VideoCore::SubresourceRange src_view_range{};
        if (!resolve_copy_image(args.srv_tsharp, false, src_id, src_view_range)) {
            continue;
        }

        VideoCore::ImageId dst_id{};
        VideoCore::SubresourceRange dst_view_range{};
        if (!resolve_copy_image(args.uav_tsharp, true, dst_id, dst_view_range)) {
            continue;
        }

        auto& src_img = texture_cache.GetImage(src_id);
        auto& dst_img = texture_cache.GetImage(dst_id);
        if (src_img.info.props.is_depth || dst_img.info.props.is_depth ||
            src_img.info.props.is_block || dst_img.info.props.is_block ||
            src_img.info.num_samples != dst_img.info.num_samples ||
            src_img.info.num_bits != dst_img.info.num_bits) {
            continue;
        }

        const u32 src_mip = std::min(src_view_range.base.level, src_img.info.resources.levels - 1);
        const u32 dst_mip = std::min(dst_view_range.base.level, dst_img.info.resources.levels - 1);
        const bool src_3d = src_img.info.type == Gcn::ImageType::Color3D;
        const bool dst_3d = dst_img.info.type == Gcn::ImageType::Color3D;
        const u32 src_layer =
            src_3d ? 0u : std::min(src_view_range.base.layer, src_img.info.resources.layers - 1);
        const u32 dst_layer =
            dst_3d ? 0u : std::min(dst_view_range.base.layer, dst_img.info.resources.layers - 1);

        const u32 src_mip_w = std::max(src_img.info.size.width >> src_mip, 1u);
        const u32 src_mip_h = std::max(src_img.info.size.height >> src_mip, 1u);
        const u32 src_mip_d = src_3d ? std::max(src_img.info.size.depth >> src_mip, 1u) : 1u;

        const u32 dst_mip_w = std::max(dst_img.info.size.width >> dst_mip, 1u);
        const u32 dst_mip_h = std::max(dst_img.info.size.height >> dst_mip, 1u);
        const u32 dst_mip_d = dst_3d ? std::max(dst_img.info.size.depth >> dst_mip, 1u) : 1u;

        const u32 sz_off = src_3d ? args.src_z : 0u;
        const u32 dz_off = dst_3d ? args.dst_z : 0u;
        if (args.src_x >= src_mip_w || args.src_y >= src_mip_h || sz_off >= src_mip_d ||
            args.dst_x >= dst_mip_w || args.dst_y >= dst_mip_h || dz_off >= dst_mip_d) {
            continue;
        }

        const u32 ext_w = std::min({args.copy_w, src_mip_w - args.src_x, dst_mip_w - args.dst_x});
        const u32 ext_h = std::min({args.copy_h, src_mip_h - args.src_y, dst_mip_h - args.dst_y});
        const u32 ext_d =
            (src_3d && dst_3d)
                ? std::min({args.copy_d, src_mip_d - sz_off, dst_mip_d - dz_off})
                : 1u;
        if (ext_w == 0 || ext_h == 0 || ext_d == 0) {
            continue;
        }

        const VideoCore::SubresourceRange src_range{
            .base = {.level = src_mip, .layer = src_layer},
            .extent = {.levels = 1, .layers = 1},
        };
        const VideoCore::SubresourceRange dst_range{
            .base = {.level = dst_mip, .layer = dst_layer},
            .extent = {.levels = 1, .layers = 1},
        };
        const vk::ImageCopy region{
            .srcSubresource = {src_img.aspect_mask & ~vk::ImageAspectFlagBits::eStencil, src_mip,
                               src_layer, 1},
            .srcOffset = {s32(args.src_x), s32(args.src_y), s32(sz_off)},
            .dstSubresource = {dst_img.aspect_mask & ~vk::ImageAspectFlagBits::eStencil, dst_mip,
                               dst_layer, 1},
            .dstOffset = {s32(args.dst_x), s32(args.dst_y), s32(dz_off)},
            .extent = {ext_w, ext_h, ext_d},
        };

        const bool same_subresource =
            (src_id == dst_id && src_mip == dst_mip && src_layer == dst_layer);
        if (same_subresource) {
            flush_group();
            runtime.CopyImageSubresource(&src_img, &dst_img, src_range, dst_range, region);
            dst_img.gpu_write_tick = scheduler.CurrentTick();
            continue;
        }
        if (!group_regions.empty() &&
            (src_id != group_src_id || dst_id != group_dst_id)) {
            flush_group();
        }
        group_src_id = src_id;
        group_dst_id = dst_id;
        group_src_ranges.push_back(src_range);
        group_dst_ranges.push_back(dst_range);
        group_regions.push_back(region);
    }
    flush_group();
    pending_tex_copies.clear();
}

void Rasterizer::DirectDecompressDepth(const DirectDecompressDepthArgs& args) {
    if (args.slice_count == 0) {
        return;
    }
    if (HasPendingHleCopies() && args.dst_va && args.dst_byte_size &&
        PendingHleCopiesConflict(args.dst_va, args.dst_byte_size, true)) {
        FlushPendingHleCopies(*this);
    }

    Gcn::DepthBuffer db{};
    std::memcpy(&db, args.db_regs, sizeof(db));
    if (!db.DepthValid() && !db.StencilValid()) {
        return;
    }
    Gcn::DepthView db_view = std::bit_cast<Gcn::DepthView>(args.db_view);
    db_view.slice_start = args.first_slice;
    db_view.slice_max = args.first_slice + args.slice_count - 1u;

    Gcn::DepthControl db_ctl{};
    db_ctl.depth_enable = db.DepthValid() ? 1u : 0u;
    db_ctl.stencil_enable = db.StencilValid() ? 1u : 0u;

    const VAddr htile_address = VAddr(args.htile_data_base) << 8;
    const Gcn::CbDbExtent hint = std::bit_cast<Gcn::CbDbExtent>(args.cb_db_extent);

    VideoCore::TextureCache::ImageDesc desc{db, db_view, db_ctl, htile_address, hint};
    const auto image_id = texture_cache.FindImage(desc);
    if (!image_id) {
        return;
    }
    (void)texture_cache.FindDepthTarget(image_id, desc);
    auto& image = texture_cache.GetImage(image_id);

    if (htile_address != 0 && (image.aspect_mask & vk::ImageAspectFlagBits::eDepth)) {
        const float clear_z = std::bit_cast<float>(args.reserved);
        const u32 max_layer = std::min(args.first_slice + args.slice_count,
                                       image.info.resources.layers);
        for (u32 slice = args.first_slice; slice < max_layer; ++slice) {
            if (texture_cache.IsMetaCleared(htile_address, slice)) {
                texture_cache.TouchMeta(htile_address, slice, false);
                const VideoCore::SubresourceRange slice_range{
                    .base = {.level = 0, .layer = slice},
                    .extent = {.levels = 1, .layers = 1},
                };
                runtime.ClearDepthStencilImage(&image, slice_range,
                                               vk::ImageAspectFlagBits::eDepth,
                                               vk::ClearDepthStencilValue{clear_z, 0u});
                image.gpu_write_tick = scheduler.CurrentTick();
            }
        }
    }
}

void Rasterizer::DirectEliminateFastClear(const DirectEliminateFastClearArgs& args) {
    if (args.slice_count == 0) {
        return;
    }
    if (HasPendingHleCopies() && args.dst_va && args.dst_byte_size &&
        PendingHleCopiesConflict(args.dst_va, args.dst_byte_size, true)) {
        FlushPendingHleCopies(*this);
    }

    Gcn::ColorBuffer col_buf{};
    std::memcpy(&col_buf, args.cb_regs, sizeof(args.cb_regs));
    if (!col_buf || !col_buf.info.fast_clear) {
        return;
    }
    col_buf.view.slice_start = args.first_slice;
    col_buf.view.slice_max = args.first_slice + args.slice_count - 1u;
    const Gcn::CbDbExtent hint = std::bit_cast<Gcn::CbDbExtent>(args.cb_regs[6]);

    VideoCore::TextureCache::ImageDesc desc{col_buf, hint};
    const auto image_id = texture_cache.FindImage(desc);
    if (!image_id) {
        return;
    }
    (void)texture_cache.FindRenderTarget(image_id, desc);
    auto& image = texture_cache.GetImage(image_id);

    const VAddr cmask = col_buf.CmaskAddress();
    if (!cmask) {
        return;
    }
    const u32 max_layer = std::min(args.first_slice + args.slice_count,
                                   image.info.resources.layers);
    const auto clear_val = GcnToVk::ColorBufferClearValue(col_buf);
    for (u32 slice = args.first_slice; slice < max_layer; ++slice) {
        if (texture_cache.IsMetaCleared(cmask, slice)) {
            texture_cache.TouchMeta(cmask, slice, false);
            const VideoCore::SubresourceRange slice_range{
                .base = {.level = desc.view_info.range.base.level, .layer = slice},
                .extent = {.levels = 1, .layers = 1},
            };
            runtime.ClearImage(&image, slice_range, clear_val);
            image.gpu_write_tick = scheduler.CurrentTick();
        }
    }
}

void Rasterizer::DirectResolveColor(const DirectResolveColorArgs& args) {
    if (!args.dst_va || !args.src_va) {
        return;
    }
    if (HasPendingHleCopies()) {
        if ((args.dst_byte_size &&
             PendingHleCopiesConflict(args.dst_va, args.dst_byte_size, true)) ||
            (args.src_byte_size &&
             PendingHleCopiesConflict(args.src_va, args.src_byte_size, false))) {
            FlushPendingHleCopies(*this);
        }
    }

    Gcn::ColorBuffer src_cb{};
    Gcn::ColorBuffer dst_cb{};
    std::memcpy(&src_cb, args.src_cb_regs, sizeof(args.src_cb_regs));
    std::memcpy(&dst_cb, args.dst_cb_regs, sizeof(args.dst_cb_regs));
    if (!src_cb || !dst_cb) {
        return;
    }
    const Gcn::CbDbExtent src_hint = std::bit_cast<Gcn::CbDbExtent>(args.src_cb_regs[6]);
    const Gcn::CbDbExtent dst_hint = std::bit_cast<Gcn::CbDbExtent>(args.dst_cb_regs[6]);

    VideoCore::TextureCache::ImageDesc src_desc{src_cb, src_hint};
    VideoCore::TextureCache::ImageDesc dst_desc{dst_cb, dst_hint};
    const auto src_id = texture_cache.FindImage(src_desc, true);
    const auto dst_id = texture_cache.FindImage(dst_desc, true);
    if (!src_id || !dst_id) {
        return;
    }
    auto& src = texture_cache.GetImage(src_id);
    auto& dst = texture_cache.GetImage(dst_id);

    if (src_cb.info.fast_clear && src_cb.CmaskAddress() != 0 &&
        texture_cache.IsMetaCleared(src_cb.CmaskAddress(), src_cb.view.slice_start)) {
        for (u32 s = src_cb.view.slice_start; s <= src_cb.view.slice_max; ++s) {
            texture_cache.TouchMeta(src_cb.CmaskAddress(), s, false);
        }
        runtime.ClearImage(&src, src_desc.view_info.range,
                           GcnToVk::ColorBufferClearValue(src_cb));
    }
    if (dst_cb.info.fast_clear && dst_cb.CmaskAddress() != 0) {
        for (u32 s = dst_cb.view.slice_start; s <= dst_cb.view.slice_max; ++s) {
            texture_cache.TouchMeta(dst_cb.CmaskAddress(), s, false);
        }
    }

    runtime.ResolveImage(&src, &dst, src_desc.view_info.range, dst_desc.view_info.range);
    dst.gpu_write_tick = scheduler.CurrentTick();
}

void Rasterizer::DirectUploadTextureSubresource(const DirectUploadTextureArgs& args) {
    if (!args.src_stage_va || args.src_byte_size == 0 ||
        args.extent_w == 0 || args.extent_h == 0 || args.extent_d == 0) {
        return;
    }
    if (HasPendingHleCopies()) {
        if ((args.dst_va && args.dst_byte_size &&
             PendingHleCopiesConflict(args.dst_va, args.dst_byte_size, true)) ||
            PendingHleCopiesConflict(args.src_stage_va, args.src_byte_size, false)) {
            FlushPendingHleCopies(*this);
        }
    }

    if (VideoCore::DirectTextures::HasPendingEvents()) {
        ProcessDirectTextureEvents();
    }

    Gcn::Image gcn_img{};
    std::memcpy(&gcn_img, args.real_tsharp, sizeof(gcn_img));
    reinterpret_cast<u32*>(&gcn_img)[7] = 0u;
    if (!gcn_img.Valid()) {
        return;
    }

    Shader::ImageResource dst_res{};
    dst_res.is_written = true;
    dst_res.needs_native = true;
    VideoCore::TextureCache::ImageDesc dst_desc{gcn_img, dst_res};
    const auto dst_id = texture_cache.FindImage(dst_desc);
    if (!dst_id) {
        return;
    }

    auto& dst_img = texture_cache.GetImage(dst_id);
    if (dst_img.info.props.is_depth || dst_img.info.num_samples > 1) {
        return;
    }

    const bool is_immutable_asset = (args.reserved0 & 0x80000000u) != 0u;
    const u32 raw_mip = args.reserved0 & 0x7FFFFFFFu;
    const u32 mip = std::min(raw_mip, dst_img.info.resources.levels - 1);
    const bool is_3d = dst_img.info.type == Gcn::ImageType::Color3D;
    const u32 layer = is_3d ? 0u : std::min(args.reserved1, dst_img.info.resources.layers - 1);

    const u32 dst_mip_w = std::max(dst_img.info.size.width >> mip, 1u);
    const u32 dst_mip_h = std::max(dst_img.info.size.height >> mip, 1u);
    const u32 dst_mip_d = is_3d ? std::max(dst_img.info.size.depth >> mip, 1u) : 1u;
    const u32 z_off = is_3d ? args.offset_z : 0u;
    if (args.offset_x >= dst_mip_w || args.offset_y >= dst_mip_h || z_off >= dst_mip_d) {
        return;
    }

    const u32 ext_w = std::min(args.extent_w, dst_mip_w - args.offset_x);
    const u32 ext_h = std::min(args.extent_h, dst_mip_h - args.offset_y);
    const u32 ext_d = is_3d ? std::min(args.extent_d, dst_mip_d - z_off) : 1u;
    if (ext_w == 0 || ext_h == 0 || ext_d == 0) {
        return;
    }

    const bool full_single_subres =
        dst_img.info.resources.levels == 1 && dst_img.info.resources.layers == 1 &&
        args.offset_x == 0 && args.offset_y == 0 && z_off == 0 &&
        ext_w == dst_mip_w && ext_h == dst_mip_h && ext_d == dst_mip_d;
    if (!full_single_subres && True(dst_img.flags & VideoCore::ImageFlagBits::Dirty)) {
        texture_cache.UpdateImage(dst_id);
    }

    // Copy directly from the guest's fence-protected CPU staging ring in host RAM into a Vulkan
    // HostUncached staging slice so BB_LAYER_MEMORY VRAM mirrors cannot serve stale bytes.
    const auto staging = runtime.GetStagingPool().Request(
        args.src_byte_size, VideoCore::MemoryType::HostUncached, 16);
    std::memcpy(staging.mapped, std::bit_cast<const void*>(args.src_stage_va), args.src_byte_size);
    staging.Flush();

    const VideoCore::SubresourceRange dst_range{
        .base = {.level = mip, .layer = layer},
        .extent = {.levels = 1, .layers = 1},
    };
    const vk::BufferImageCopy copy{
        .bufferOffset = staging.offset,
        .bufferRowLength = args.buf_row_len,
        .bufferImageHeight = args.buf_img_height,
        .imageSubresource = {dst_img.aspect_mask & ~vk::ImageAspectFlagBits::eStencil, mip, layer,
                             1},
        .imageOffset = {s32(args.offset_x), s32(args.offset_y), s32(z_off)},
        .imageExtent = {ext_w, ext_h, ext_d},
    };
    runtime.CopyBufferToImageSubresource(&dst_img, staging.buffer, dst_range, copy);

    dst_img.flags =
        (dst_img.flags | VideoCore::ImageFlagBits::GpuModified) & ~VideoCore::ImageFlagBits::Dirty;
    dst_img.gpu_write_tick = scheduler.CurrentTick();

    if (is_immutable_asset) {
        texture_cache.MarkDirectImmutableAsset(dst_id);
        return;
    }

    // Seed the 8x8 page-watch hash and mip hash of the backing guest region so a subsequent
    // MaybeCpuDirty write elsewhere in the same page never re-uploads zeroed guest tiled memory
    // over the GPU-uploaded VkImage.
    const auto& info = dst_img.info;
    if (info.guest_address != 0 && info.guest_size != 0) {
        const u32 w8 = std::min(info.size.width, 8u);
        const u32 h8 = std::min(info.size.height, 8u);
        const u32 bytes8 = (info.props.is_block ? (w8 + 3) / 4 : w8) *
                           (info.props.is_block ? (h8 + 3) / 4 : h8) * (info.num_bits / 8);
        if (bytes8 <= info.guest_size) {
            dst_img.hash = XXH3_64bits(std::bit_cast<const u8*>(info.guest_address), bytes8);
        }
        if (dst_img.mip_hashes.size() < info.resources.levels) {
            dst_img.mip_hashes.resize(info.resources.levels, 0);
        }
        if (mip < info.mips_layout.size() && mip < dst_img.mip_hashes.size()) {
            const auto& layout = info.mips_layout[mip];
            if (layout.offset + layout.size <= info.guest_size) {
                dst_img.mip_hashes[mip] = XXH3_64bits(
                    std::bit_cast<const u8*>(info.guest_address) + layout.offset, layout.size);
            }
        }
    }
}

void Rasterizer::DirectClearDepthStencil(const DirectClearDepthStencilArgs& args) {
    if (args.slice_count == 0) {
        return;
    }
    const bool clear_z = (args.stencil_and_flags & 0x100u) != 0u;
    const bool clear_s = (args.stencil_and_flags & 0x200u) != 0u;
    if (!clear_z && !clear_s) {
        return;
    }
    if (HasPendingHleCopies() && args.dst_va && args.dst_byte_size &&
        PendingHleCopiesConflict(args.dst_va, args.dst_byte_size, true)) {
        FlushPendingHleCopies(*this);
    }

    Gcn::DepthBuffer db{};
    std::memcpy(&db, args.db_regs, sizeof(db));
    if (!db.DepthValid() && !db.StencilValid()) {
        return;
    }
    Gcn::DepthView db_view = std::bit_cast<Gcn::DepthView>(args.db_view);
    db_view.slice_start = args.first_slice;
    db_view.slice_max = args.first_slice + args.slice_count - 1u;

    Gcn::DepthControl db_ctl{};
    db_ctl.depth_enable = db.DepthValid() ? 1u : 0u;
    db_ctl.depth_write_enable = clear_z ? 1u : 0u;
    db_ctl.stencil_enable = db.StencilValid() ? 1u : 0u;

    const VAddr htile_address = VAddr(args.htile_data_base) << 8;
    const Gcn::CbDbExtent hint = std::bit_cast<Gcn::CbDbExtent>(args.cb_db_extent);

    VideoCore::TextureCache::ImageDesc desc{db, db_view, db_ctl, htile_address, hint};
    const auto image_id = texture_cache.FindImage(desc);
    if (!image_id) {
        return;
    }
    (void)texture_cache.FindDepthTarget(image_id, desc);
    auto& image = texture_cache.GetImage(image_id);

    vk::ImageAspectFlags aspect{};
    if (clear_z && (image.aspect_mask & vk::ImageAspectFlagBits::eDepth)) {
        aspect |= vk::ImageAspectFlagBits::eDepth;
    }
    if (clear_s && (image.aspect_mask & vk::ImageAspectFlagBits::eStencil)) {
        aspect |= vk::ImageAspectFlagBits::eStencil;
    }
    if (!aspect) {
        return;
    }

    const u32 max_layer = std::min(args.first_slice + args.slice_count,
                                   image.info.resources.layers);
    if (max_layer <= args.first_slice) {
        return;
    }
    if (clear_z && htile_address != 0) {
        for (u32 slice = args.first_slice; slice < max_layer; ++slice) {
            texture_cache.TouchMeta(htile_address, slice, false);
        }
    }

    const VideoCore::SubresourceRange range{
        .base = {.level = 0, .layer = args.first_slice},
        .extent = {.levels = 1, .layers = max_layer - args.first_slice},
    };
    const u32 stencil_val = args.stencil_and_flags & 0xFFu;
    runtime.ClearDepthStencilImage(&image, range, aspect,
                                   vk::ClearDepthStencilValue{args.depth_clear, stencil_val});
    image.gpu_write_tick = scheduler.CurrentTick();
}

u32 Rasterizer::ReadDataFromGds(u32 gds_offset) {
    DrainDrawPipe();
    u32 value;
    std::memcpy(&value, buffer_cache.GetGdsBuffer()->mapped_data.data() + gds_offset,
                sizeof(value));
    return value;
}

bool Rasterizer::InvalidateMemory(VAddr addr, u64 size, bool assume_locks) {
    DrainDrawPipe();
    InvalidateGdsMirrorRange(addr, size);
    if (!IsMapped(addr, size)) {
        return false; // the GPU does not use it
    }
    if (HasPendingHleCopies() && PendingHleCopiesConflict(addr, size, true)) {
        FlushPendingHleCopies(*this);
    }
    buffer_cache.InvalidateMemory(addr, size, assume_locks);
    texture_cache.InvalidateMemory(addr, size);
    return true;
}

void Rasterizer::NoteAssetWrite(VAddr addr, u64 size) {
    InvalidateAfterWrite(addr, size);
    buffer_cache.NoteAssetWrite(addr, size);
}

void Rasterizer::InvalidateAfterWrite(VAddr addr, u64 size) {
    InvalidateGdsMirrorRange(addr, size);
    if (VideoCore::WriteTracking() || !buffer_cache.IsRegionGpuModified(addr, size)) {
        InvalidateMemory(addr, size);
        return;
    }
    // GPU data beside the bytes the CPU wrote lives in VRAM: an invalidation would read it back
    // (a 512 KiB window, whole pages) and upload pages over what the GPU writes meanwhile.
    // Exactly the CPU's bytes go into VRAM instead; images over them are refreshed.
    DrainDrawPipe();
    if (HasPendingHleCopies() && PendingHleCopiesConflict(addr, size, true)) {
        FlushPendingHleCopies(*this);
    }
    buffer_cache.NotePreciseUpload(addr, size);
    texture_cache.InvalidateMemory(addr, size);
}

void Rasterizer::NoteCommandWrite(VAddr addr, const void* data, u64 size, bool recording_thread) {
    InvalidateGdsMirrorRange(addr, size);
    if (VideoCore::WriteTracking() || !IsMapped(addr, size)) {
        return; // tracking catches it, or the GPU does not use it
    }
    if (!recording_thread) {
        DrainDrawPipe();
    }
    if (HasPendingHleCopies() && PendingHleCopiesConflict(addr, size, true)) {
        FlushPendingHleCopies(*this);
    }
    buffer_cache.DropShadows(addr, size);
    // An invalidation would read the GPU's data back over these bytes first (a readback).
    if (!buffer_cache.UpdateGpuWritten(addr, std::span{static_cast<const u8*>(data), size})) {
        buffer_cache.InvalidateMemory(addr, size, recording_thread);
    }
    texture_cache.InvalidateMemory(addr, size);
}

void Rasterizer::OnCpuWrite(VAddr addr, u64 size) {
    BbStats::cpu_write_notes.fetch_add(1, std::memory_order_relaxed);
    BbStats::cpu_write_note_bytes.fetch_add(size, std::memory_order_relaxed);
    if (IsMapped(addr, size)) {
        InvalidateAfterWrite(addr, size);
        buffer_cache.NoteCpuWriteRange(addr, size);
    }
}

bool Rasterizer::OnWriteFault(VAddr addr, bool assume_locks, u64 guest_rip) {
    DrainDrawPipe();
    if (!InvalidateMemory(addr, 8, assume_locks)) {
        return false;
    }
    buffer_cache.ExtendWriteFault(addr, guest_rip);
    return true;
}

bool Rasterizer::ReadMemory(VAddr addr, u64 size, bool assume_locks) {
    DrainDrawPipe();
    if (!IsMapped(addr, size)) {
        return false; // the GPU does not use it
    }
    if (HasPendingHleCopies() && PendingHleCopiesConflict(addr, size, false)) {
        FlushPendingHleCopies(*this);
    }
    buffer_cache.ReadMemory(addr, size, false, assume_locks);
    return true;
}

void Rasterizer::ProcessDownloadImages() {
    DrainDrawPipe();
    buffer_cache.NoteTransientSubmitBoundary();
    texture_cache.ProcessDownloadImages();
}

bool Rasterizer::IsMapped(VAddr addr, u64 size) {
    // Empty ranges and ranges wrapping the address space are never mapped.
    if (size == 0 || addr > std::numeric_limits<u64>::max() - size) {
        return false;
    }
    const auto range = decltype(mapped_ranges)::interval_type::right_open(addr, addr + size);
    Common::RecursiveSharedLock lock{mapped_ranges_mutex};
    return boost::icl::contains(mapped_ranges, range);
}

void Rasterizer::MapMemory(VAddr addr, u64 size) {
    {
        std::scoped_lock lock{mapped_ranges_mutex};
        mapped_ranges += decltype(mapped_ranges)::interval_type::right_open(addr, addr + size);
    }
    buffer_cache.NotePreuploadMapping(addr, size, true);
}

void Rasterizer::RegisterMemory(VAddr addr, u64 size) {
    page_watch.OnGpuMap(addr, size);
}

void Rasterizer::UnmapMemory(VAddr addr, u64 size) {
    buffer_cache.NotePreuploadMapping(addr, size, false);
    buffer_cache.UnmapInPlace(addr, size);
    buffer_cache.DropTwins(addr, size);
    DrainDrawPipe();
    if (HasPendingHleCopies()) {
        FlushPendingHleCopies(*this);
    }
    buffer_cache.InvalidateMemory(addr, size);
    texture_cache.UnmapMemory(addr, size);
    std::scoped_lock lock{mapped_ranges_mutex};
    mapped_ranges -= decltype(mapped_ranges)::interval_type::right_open(addr, addr + size);
}

} // namespace Vulkan
