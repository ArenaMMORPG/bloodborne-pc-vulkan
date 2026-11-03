// ============================================================================
// Bloodborne (CUSA03173 v1.09) — Decompiled Engine Source
// Module:  01_CoreGraphics2_GNM (0x26a8000 .. 0x26d6000)
// Original Path: N:/SPRJ/Source/Library/Dantelion2/dist_ps4/source/ApplicationLibrary/
//                CoreGraphics2/Source/Detail/gnm/CGResourceSyncContext_gnm.cpp
// ============================================================================

#include <cstdint>
#include <cstring>
#include <algorithm>
#include "gx_direct_vulkan.h"

namespace Dantelion2::CoreGraphics2::Gnm {

struct GpuRangeNode {
    uint64_t vtable_or_color; // +0x00: State (0=free, 2=allocated) or used bytes in StagingPool64K
    uint64_t start_va;        // +0x08: Start virtual address in PS4 GPU memory
    GpuRangeNode* prev_node;  // +0x10: Previous node in address-ordered doubly-linked list
    GpuRangeNode* next_node;  // +0x18: Next node in address-ordered list (next_node->start_va == end_va)
    GpuRangeNode* prev_free;  // +0x20: Previous node in doubly-linked free list
    GpuRangeNode* next_free;  // +0x28: Next node in free list / staging bucket list
};

struct GpuRangeAllocator {
    void** vtable;            // +0x00
    uint8_t pad_08[0xc0];
    uint64_t live_ranges;     // +0xc8: Count of active allocations
    uint64_t free_bytes;      // +0xd0: Total free bytes remaining (decremented on alloc)
    void** mutex_vtable;      // +0xd8: Embedded lock object (vtable +0x18 Lock, +0x28 Unlock)
};

struct StagingPool64K {
    GpuRangeAllocator* gpu_heap;  // +0x00
    GpuRangeNode* buckets[16];    // +0x08 .. +0x87: Free sub-chunk buckets by log2 remaining size
    GpuRangeNode* full_chunks;    // +0x88: Fully consumed 64KB chunks awaiting EOP reclaim
    uint32_t non_empty_mask;      // +0x90: Bitmask of non-empty buckets[0..15]
    uint32_t reserved_94;         // +0x94
};

struct BufferCore {
    void** vtable;            // +0x00
    uint64_t gpu_va;          // +0x08: GPU virtual address
    uint32_t flags_10;        // +0x10
    uint32_t byte_size;       // +0x14: Size in bytes
};

struct PendingUpdateQueueA {
    void* allocator;          // +0x00
    uint64_t** chunk_table;   // +0x08: Array of 128-entry (2KB) chunk pointers
    uint32_t count;           // +0x10: Total active entries
    uint32_t capacity_chunks; // +0x14
    uint16_t fast_slot_lut_idx; // +0x18
};

struct PendingUpdateQueueB {
    void** allocator;         // +0x00
    uint8_t* head_chunk;      // +0x08: Linked list of chunk blocks (24 bytes * stride)
    uint8_t* tail_chunk;      // +0x10
    uint32_t entries_per_chunk; // +0x18
    uint32_t tail_used;       // +0x1C
};

struct CopyRecord {
    uint64_t dst_va;          // +0x00: Destination GPU buffer virtual address
    uint64_t src_va;          // +0x08: Source staging buffer virtual address
    uint32_t byte_size;       // +0x10: Copy byte size (multiple of 4)
    uint32_t reserved;        // +0x14
};

struct GnmxContextCore {
    uint32_t* dcb_cmdptr;      // +0x00
    uint32_t* dcb_endptr;      // +0x08
    uint32_t* dcb_beginptr;    // +0x10
};

// ----------------------------------------------------------------------------
// VMA: 0x26aa070 (Size: 500 bytes)
// Allocates a GPU virtual address range from the doubly-linked free list (`alloc + 0x68`).
// ----------------------------------------------------------------------------
GpuRangeNode* GpuHeap_AllocateRange(GpuRangeAllocator* alloc, uint64_t size, uint64_t alignment) {
    const uint64_t aligned_size = (size + 0x0full) & ~0x0full;
    auto* sentinel = reinterpret_cast<GpuRangeNode*>(reinterpret_cast<uint8_t*>(alloc) + 0x68);
    GpuRangeNode** link_ptr = &sentinel->prev_free; // alloc + 0x88
    const uint64_t align_mask = alignment - 1ull;
    const uint64_t neg_align = 0ull - alignment;

    for (;;) {
        GpuRangeNode* cand = *link_ptr;
        if (cand == sentinel) {
            return nullptr;
        }
        const uint64_t start_va = cand->start_va;
        const uint64_t end_va = cand->next_node->start_va;
        const uint64_t aligned_start = (start_va + align_mask) & neg_align;

        uint64_t front_pad = 0;
        uint64_t usable_size = 0;
        if (end_va >= aligned_start) {
            usable_size = end_va - aligned_start;
            front_pad = aligned_start - start_va;
        }
        link_ptr = &cand->prev_free;

        if (usable_size < aligned_size) {
            continue;
        }

        alloc->live_ranges += 1;
        if (front_pad != 0 || usable_size > aligned_size) {
            const uintptr_t alloc_addr = reinterpret_cast<uintptr_t>(alloc);
            void* node_pool = reinterpret_cast<uint8_t*>(alloc) + ((0ull - alloc_addr) & 7ull);
            using AllocNodeFn = GpuRangeNode* (*)(void*);
            GpuRangeNode* new_node = reinterpret_cast<AllocNodeFn>(GX_VMA(0x0263b8b0ULL))(node_pool);
            if (!new_node) {
                cand->start_va = (cand->start_va + align_mask) & neg_align;
            } else {
                GpuRangeNode* next_addr_node = cand->next_node;
                cand->next_node = new_node;
                new_node->prev_node = cand;
                new_node->next_node = next_addr_node;
                next_addr_node->prev_node = new_node;
                new_node->vtable_or_color = 0;
                new_node->next_free = nullptr;
                new_node->prev_free = nullptr;

                const uint64_t back_pad = (next_addr_node->start_va - aligned_size) & align_mask;
                if (back_pad != 0) {
                    GpuRangeNode* tail_node = reinterpret_cast<AllocNodeFn>(GX_VMA(0x0263b8b0ULL))(node_pool);
                    if (tail_node) {
                        new_node->next_node = tail_node;
                        tail_node->prev_node = new_node;
                        tail_node->next_node = next_addr_node;
                        next_addr_node->prev_node = tail_node;
                        tail_node->vtable_or_color = 0;
                        tail_node->next_free = nullptr;
                        tail_node->prev_free = nullptr;
                        tail_node->start_va = next_addr_node->start_va - back_pad;

                        GpuRangeNode* old_next_free = cand->next_free;
                        cand->next_free = tail_node;
                        tail_node->next_free = old_next_free;
                        tail_node->prev_free = cand;
                        old_next_free->prev_free = tail_node;
                    }
                }

                const uint64_t new_start = next_addr_node->start_va - (back_pad + aligned_size);
                new_node->start_va = new_start;
                const uint64_t new_bytes = new_node->next_node->start_va - new_start;
                alloc->free_bytes += (new_start - new_node->next_node->start_va);
                new_node->next_free = nullptr;
                new_node->prev_free = nullptr;
                new_node->vtable_or_color = 2;
                gx_direct_vk::NoteDirectFreshGpuRange(
                    reinterpret_cast<const void*>(new_start), new_bytes);
                return new_node;
            }
        }

        // Consume `cand` directly from the free list
        GpuRangeNode* pf = cand->prev_free;
        GpuRangeNode* nf = cand->next_free;
        pf->next_free = nf;
        nf->prev_free = pf;
        const uint64_t cand_bytes = cand->next_node->start_va - cand->start_va;
        alloc->free_bytes += (cand->start_va - cand->next_node->start_va);
        cand->next_free = nullptr;
        cand->prev_free = nullptr;
        cand->vtable_or_color = 2;
        gx_direct_vk::NoteDirectFreshGpuRange(
            reinterpret_cast<const void*>(cand->start_va), cand_bytes);
        return cand;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x26aa010 (Size: 87 bytes)
// Allocates GPU memory with default 16-byte alignment (0x10), locking `alloc + 0xd8`.
// ----------------------------------------------------------------------------
GpuRangeNode* GpuHeap_AllocDefaultAlign16(GpuRangeAllocator* alloc, uint64_t size) {
    void* mutex_obj = reinterpret_cast<uint8_t*>(alloc) + 0xd8;
    auto** vtbl = *reinterpret_cast<void***>(mutex_obj);
    reinterpret_cast<void (*)(void*, uint32_t)>(vtbl[3])(mutex_obj, 0xFFFFFFFFu);

    GpuRangeNode* res = GpuHeap_AllocateRange(alloc, size, 0x10ull);

    vtbl = *reinterpret_cast<void***>(mutex_obj);
    reinterpret_cast<void (*)(void*)>(vtbl[5])(mutex_obj);
    return res;
}

// ----------------------------------------------------------------------------
// VMA: 0x26aa270 (Size: 108 bytes)
// Thread-safe wrapper validating power-of-two alignment (`blsr`) and locking
// `alloc + 0xd8` before calling `GpuHeap_AllocateRange` (`0x26aa070`).
// ----------------------------------------------------------------------------
GpuRangeNode* GpuRangeHeap_AllocateFromTree(GpuRangeAllocator* alloc, uint64_t size, uint64_t alignment) {
    if ((alignment & (alignment - 1ull)) != 0ull) {
        return nullptr;
    }
    void* mutex_obj = reinterpret_cast<uint8_t*>(alloc) + 0xd8;
    auto** vtbl = *reinterpret_cast<void***>(mutex_obj);
    reinterpret_cast<void (*)(void*, uint32_t)>(vtbl[3])(mutex_obj, 0xFFFFFFFFu);

    const uint64_t clamped_align = (alignment > 0x10ull) ? alignment : 0x10ull;
    GpuRangeNode* node = GpuHeap_AllocateRange(alloc, size, clamped_align);

    vtbl = *reinterpret_cast<void***>(mutex_obj);
    reinterpret_cast<void (*)(void*)>(vtbl[5])(mutex_obj);
    return node;
}

// ----------------------------------------------------------------------------
// VMA: 0x26addc0 (Size: 393 bytes)
// Suballocates `size` bytes (<= 64 KB) from `StagingPool64K` (`ctx + 0xb3e0`),
// maintaining 16 power-of-two remainder buckets (`+0x08..+0x87`) and bitmask (`+0x90`).
// ----------------------------------------------------------------------------
uint8_t* StagingPool_Suballocate64K(StagingPool64K* pool, void* fence_ctx, uint64_t size) {
    if (size == 0ull || size > 0x10000ull) {
        return nullptr;
    }
    const uint32_t req = static_cast<uint32_t>(size);
    const uint32_t req_minus_1 = req - 1u;
    const uint32_t lz = (req_minus_1 == 0u) ? 32u : static_cast<uint32_t>(__builtin_clz(req_minus_1));
    const uint32_t shift = (32u - lz) & 31u;
    const uint32_t eligible_mask = (0xFFFFFFFFu << shift) & pool->non_empty_mask;
    const uint32_t bucket = (eligible_mask == 0u) ? 32u : static_cast<uint32_t>(__builtin_ctz(eligible_mask));

    if (bucket < 16u) {
        GpuRangeNode* node = pool->buckets[bucket];
        const uint32_t used_before = static_cast<uint32_t>(node->vtable_or_color);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(node->start_va + used_before);
        const uint32_t used_after = used_before + req;
        node->vtable_or_color = used_after;

        if (used_after == 0x10000u) {
            GpuRangeNode* next = node->next_free;
            pool->buckets[bucket] = next;
            if (!next) {
                pool->non_empty_mask &= ~(1u << bucket);
            }
            node->next_free = pool->full_chunks;
            pool->full_chunks = node;
        } else {
            const uint32_t rem = 0x10000u - used_after;
            const uint32_t new_bucket = 31u - static_cast<uint32_t>(__builtin_clz(rem));
            if (new_bucket != bucket) {
                GpuRangeNode* next = node->next_free;
                pool->buckets[bucket] = next;
                node->next_free = pool->buckets[new_bucket];
                pool->buckets[new_bucket] = node;
                uint32_t mask = pool->non_empty_mask;
                if (!next) {
                    mask &= ~(1u << bucket);
                }
                pool->non_empty_mask = mask | (1u << new_bucket);
            }
        }
        return ptr;
    }

    using Alloc64KFn = GpuRangeNode* (*)(GpuRangeAllocator*, uint64_t);
    using ReclaimFn = void (*)(GpuRangeAllocator*, void*);
    GpuRangeNode* chunk = reinterpret_cast<Alloc64KFn>(GX_VMA(0x026a9d50ULL))(pool->gpu_heap, 0x10000ull);
    if (!chunk) {
        reinterpret_cast<ReclaimFn>(GX_VMA(0x026aa860ULL))(pool->gpu_heap, fence_ctx);
        chunk = reinterpret_cast<Alloc64KFn>(GX_VMA(0x026a9d50ULL))(pool->gpu_heap, 0x10000ull);
        if (!chunk) {
            return nullptr;
        }
    }

    uint8_t* result = reinterpret_cast<uint8_t*>(chunk->start_va);
    if (req == 0x10000u) {
        chunk->next_free = pool->full_chunks;
        pool->full_chunks = chunk;
    } else {
        const uint32_t rem = 0x10000u - req;
        const uint32_t new_bucket = 31u - static_cast<uint32_t>(__builtin_clz(rem));
        chunk->vtable_or_color = req;
        chunk->next_free = pool->buckets[new_bucket];
        pool->buckets[new_bucket] = chunk;
        pool->non_empty_mask |= (1u << new_bucket);
    }
    return result;
}

void FlushBufferUpdates_A(PendingUpdateQueueA* queue_a, GnmxContextCore* ctx);

// ----------------------------------------------------------------------------
// VMA: 0x26adb90 (Size: 558 bytes)
// Suballocates a staging slice in `ctx + 0xb3e0`, copies `src_data` (`*(buf+0x2c)`
// bytes), and records or coalesces the pending buffer update in `PendingUpdateQueueA`.
// ----------------------------------------------------------------------------
int32_t CopyToStagingAndEnqueue_A(
    PendingUpdateQueueA* queue,
    GnmxContextCore* ctx,
    void* allocator_ctx,
    BufferCore* target_buf,
    const void* src_data)
{
    auto* ctx_bytes = reinterpret_cast<uint8_t*>(ctx);
    auto* buf_bytes = reinterpret_cast<uint8_t*>(target_buf);
    void* fence_ptr = ctx_bytes + 0x10;
    const uint32_t copy_bytes = *reinterpret_cast<uint32_t*>(buf_bytes + 0x2c);

    uint8_t* staging_dst = StagingPool_Suballocate64K(
        reinterpret_cast<StagingPool64K*>(ctx_bytes + 0xb3e0),
        &fence_ptr,
        copy_bytes);
    if (!staging_dst) {
        return 0;
    }

    uint8_t* buf_core = buf_bytes + 0x18;
    std::memcpy(staging_dst, src_data, copy_bytes);

    const uint16_t q_slot = queue->fast_slot_lut_idx;
    uint64_t* found_entry = nullptr;

    if (q_slot > 0x0Fu) {
        uint64_t** chunk_table = queue->chunk_table;
        const uint32_t total = queue->count;
        const uint32_t tail_count = (total == 0u) ? 0u : (((total + 0x7Fu) & 0x7Fu) + 1u);
        if (chunk_table) {
            for (uint64_t** cp = chunk_table; *cp != nullptr; ++cp) {
                uint64_t* chunk = *cp;
                const uint32_t limit = (cp[1] != nullptr) ? 0x80u : tail_count;
                for (uint32_t i = 0; i < limit; ++i) {
                    if (chunk[i * 2u] == reinterpret_cast<uint64_t>(buf_core)) {
                        found_entry = &chunk[i * 2u];
                        break;
                    }
                }
                if (found_entry) {
                    break;
                }
            }
        }
    } else {
        uint64_t** chunk_table = queue->chunk_table;
        if (chunk_table) {
            uint16_t* lut = *reinterpret_cast<uint16_t**>(buf_bytes + 0x50);
            const uint32_t idx = static_cast<uint32_t>(lut[q_slot]) - 1u;
            if (idx < queue->count) {
                uint64_t* cand = chunk_table[idx >> 7] + ((idx & 0x7Fu) * 2u);
                if (cand && cand[0] == reinterpret_cast<uint64_t>(buf_core)) {
                    found_entry = cand;
                }
            }
        }
    }

    if (!found_entry) {
        if (queue->count == 0xFFFFu) {
            FlushBufferUpdates_A(queue, ctx);
        }
        using AllocEntryFn = uint64_t* (*)(PendingUpdateQueueA*);
        found_entry = reinterpret_cast<AllocEntryFn>(GX_VMA(0x026ae700ULL))(queue);
        if (!found_entry) {
            return 0;
        }
        found_entry[0] = reinterpret_cast<uint64_t>(buf_core);
        const uint16_t slot2 = queue->fast_slot_lut_idx;
        if (slot2 <= 0x0Fu) {
            uint16_t* lut = *reinterpret_cast<uint16_t**>(buf_bytes + 0x50);
            lut[slot2] = static_cast<uint16_t>(queue->count);
        }
    }

    found_entry[1] = reinterpret_cast<uint64_t>(staging_dst);

    const uint32_t mask32 = *reinterpret_cast<uint32_t*>(buf_core + 0x2c);
    uint64_t stage_mask = mask32 >> 16;
    if (stage_mask != 0) {
        const uint64_t slot_mask = mask32 & 0xFFFFu;
        auto* binder_bytes = reinterpret_cast<uint8_t*>(allocator_ctx);
        while (stage_mask != 0) {
            const uint64_t st = static_cast<uint64_t>(__builtin_ctzll(stage_mask));
            stage_mask -= (1ull << st);
            auto* ctx_dirty = reinterpret_cast<uint64_t*>(ctx_bytes + 0x78 + st * 0x640);
            uint8_t* binder_stage = binder_bytes + st * 0x4f8;
            uint64_t rem_slots = slot_mask;
            while (rem_slots != 0) {
                const uint64_t sl = static_cast<uint64_t>(__builtin_ctzll(rem_slots));
                rem_slots -= (1ull << sl);
                if (*reinterpret_cast<uint8_t**>(binder_stage + 0xa18 + sl * 8) == buf_bytes) {
                    *ctx_dirty |= (1ull << (sl + 16));
                }
            }
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
// VMA: 0x26ae900 (Size: 50 bytes)
// Clears a 16-byte entry `(target_buffer, staging_src_va)` at 1-based index `idx`
// inside the chunked `PendingUpdateQueueA` table (`+0x08`, 128 entries per chunk).
// ----------------------------------------------------------------------------
extern "C" void sub_26ae900(PendingUpdateQueueA* queue, uint32_t one_based_idx) {
    if (!queue || !queue->chunk_table) {
        return;
    }
    const uint32_t idx = one_based_idx - 1u;
    if (queue->count <= idx) {
        return;
    }
    uint64_t* chunk = queue->chunk_table[idx >> 7];
    if (!chunk) {
        return;
    }
    uint64_t* entry = chunk + ((idx & 0x7Fu) * 2u);
    entry[1] = 0;
    entry[0] = 0;
}

// ----------------------------------------------------------------------------
// VMA: 0x26b2670 (Size: 406 bytes)
// Fallback single-buffer GPU->GPU copy via compute shader (`0x546b8c8`) when
// the scatter-gather control ring is full.
// ----------------------------------------------------------------------------
void GpuUtil_SingleCopy_Impl(
    GnmxContextCore* ctx,
    uint64_t dst_va,
    uint32_t dst_pol,
    uint64_t src_va,
    uint32_t src_pol,
    uint64_t byte_size) __asm__("_ZN10Dantelion213CoreGraphics23Gnm18GpuUtil_SingleCopyEPNS1_15GnmxContextCoreEmjmjj");

void GpuUtil_SingleCopy_Impl(
    GnmxContextCore* ctx,
    uint64_t dst_va,
    uint32_t dst_pol,
    uint64_t src_va,
    uint32_t src_pol,
    uint64_t byte_size)
{
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_SingleCopy)) {
        if (byte_size != 0 && gx_direct_vk::EmitCopyBuffer(ctx, dst_va, src_va, byte_size)) {
            return;
        }
        if (byte_size == 0) {
            return;
        }
    }
    auto* gfxc = reinterpret_cast<uint8_t*>(ctx);
    uint8_t* binder = gfxc + 0x40;
    const uint32_t dwords = static_cast<uint32_t>(byte_size >> 2);
    const void* cs_copy = reinterpret_cast<const void*>(GX_VMA(0x0546b8c8ULL));

    using BindCsFn = void (*)(void*, void*, const void*);
    using InitRegBufFn = void (*)(void*, const void*, uint32_t, uint32_t);
    using SetMemTypeFn = void (*)(void*, int32_t, uint32_t, uint32_t);
    using PreDispatchCsFn = void (*)(void*, void*, const void*);
    using DispatchFn = void (*)(void*, uint32_t, uint32_t, uint32_t);
    using PostDispatchCsFn = void (*)(void*, void*);

    reinterpret_cast<BindCsFn>(GX_VMA(0x026cd400ULL))(binder, gfxc, cs_copy);

    uint64_t srv_desc[2] = {0, 0};
    reinterpret_cast<InitRegBufFn>(GX_VMA(0x010707c0ULL))(
        srv_desc, reinterpret_cast<const void*>(src_va), 0x4404u, dwords);
    reinterpret_cast<SetMemTypeFn>(GX_VMA(0x010710c0ULL))(
        srv_desc,
        static_cast<int32_t>(src_pol) >> 16,
        (src_pol >> 8) & 0xFFu,
        src_pol & 0xFFu);

    *reinterpret_cast<uint64_t*>(gfxc + 0x7920) = srv_desc[0];
    *reinterpret_cast<uint64_t*>(gfxc + 0x7928) = srv_desc[1];
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fb0) |= *reinterpret_cast<uint64_t*>(gfxc + 0x1ff8);
    *reinterpret_cast<uint8_t*>(gfxc + 0x1f98) |= 0x01u;

    uint64_t uav_desc[2] = {0, 0};
    reinterpret_cast<InitRegBufFn>(GX_VMA(0x010707c0ULL))(
        uav_desc, reinterpret_cast<const void*>(dst_va), 0x4404u, dwords);
    reinterpret_cast<SetMemTypeFn>(GX_VMA(0x010710c0ULL))(
        uav_desc,
        static_cast<int32_t>(dst_pol) >> 16,
        (dst_pol >> 8) & 0xFFu,
        dst_pol & 0xFFu);

    *reinterpret_cast<uint64_t*>(gfxc + 0x9320) = uav_desc[0];
    *reinterpret_cast<uint64_t*>(gfxc + 0x9328) = uav_desc[1];
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fa8) |= 0x1000000000ull;
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fb0) |= *reinterpret_cast<uint64_t*>(gfxc + 0x2518);

    const uint32_t groups_x = static_cast<uint32_t>((static_cast<uint64_t>(dwords) + 0x3Full) >> 6);
    reinterpret_cast<PreDispatchCsFn>(GX_VMA(0x026cd5d0ULL))(binder, gfxc, cs_copy);
    reinterpret_cast<DispatchFn>(GX_VMA(0x01074c50ULL))(gfxc, groups_x, 1u, 1u);
    reinterpret_cast<PostDispatchCsFn>(GX_VMA(0x026ce110ULL))(binder, gfxc);

    *reinterpret_cast<uint8_t*>(gfxc + 0x1f98) |= 0x01u;
    *reinterpret_cast<uint8_t*>(gfxc + 0x1fac) |= 0x10u;
}

// ----------------------------------------------------------------------------
// VMA: 0x26b2810 (Size: 1308 bytes)
// Batched scatter-gather buffer copy via compute shader `0x546b9f0`.
// ----------------------------------------------------------------------------
void GpuUtil_ScatterGatherCopy(
    GnmxContextCore* ctx,        // rdi
    uint32_t dst_cache_policy,   // esi
    uint32_t src_cache_policy,   // edx
    const CopyRecord* records,   // rcx
    uint32_t record_count)       // r8d
{
    if (gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_ScatterGather)) {
        if (record_count == 0) {
            return;
        }
        if (gx_direct_vk::EmitCopyBufferBatch(
                ctx,
                reinterpret_cast<const gx_direct_vk::CopyRegion*>(records),
                record_count)) {
            return;
        }
    }
    auto* gfxc = reinterpret_cast<uint8_t*>(ctx);
    uint64_t min_dst = UINT64_MAX;
    uint64_t max_dst = 0;
    uint64_t min_src = UINT64_MAX;
    uint64_t max_src = 0;
    uint64_t total_groups = 0;

    for (uint32_t i = 0; i < record_count; ++i) {
        const uint64_t dst_lo = records[i].dst_va;
        const uint64_t src_lo = records[i].src_va;
        const uint64_t sz = records[i].byte_size;
        const uint64_t dst_hi = dst_lo + sz;
        const uint64_t src_hi = src_lo + sz;

        min_dst = std::min(min_dst, dst_lo);
        min_src = std::min(min_src, src_lo);
        max_dst = std::max(max_dst, dst_hi);
        max_src = std::max(max_src, src_hi);
        total_groups += (sz + 0xFFull) >> 8;
    }

    const uint32_t ctrl_bytes = static_cast<uint32_t>(total_groups) * 12u;
    uint8_t* outer_ctx = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);
    void* dyn_heap = *reinterpret_cast<void**>(outer_ctx + 0x08);
    auto** heap_vtbl = *reinterpret_cast<void***>(dyn_heap);
    using HeapAllocFn = GpuRangeNode* (*)(void*, uint64_t, uint64_t);
    GpuRangeNode* ctrl_node = reinterpret_cast<HeapAllocFn>(heap_vtbl[3])(dyn_heap, ctrl_bytes, 4ull);
    if (!ctrl_node) {
        return;
    }

    uint64_t ctrl_va = ctrl_node->start_va;
    if (total_groups != 0) {
        auto* out_ctrl = reinterpret_cast<uint32_t*>(ctrl_va);
        size_t rec_idx = 0;
        uint32_t dword_off = 0;
        for (uint64_t g = 0; g < total_groups; ++g) {
            const uint32_t dst_dw = ((static_cast<uint32_t>(records[rec_idx].dst_va) - static_cast<uint32_t>(min_dst)) >> 2) + dword_off;
            const uint32_t src_dw = ((static_cast<uint32_t>(records[rec_idx].src_va) - static_cast<uint32_t>(min_src)) >> 2) + dword_off;
            out_ctrl[0] = dst_dw;
            out_ctrl[1] = src_dw;
            out_ctrl[2] = 0x3Fu;

            const uint32_t rec_dwords = records[rec_idx].byte_size >> 2;
            uint32_t next_off = dword_off + 0x40u;
            if (rec_dwords <= next_off) {
                out_ctrl[2] = rec_dwords + (~dword_off);
                ++rec_idx;
                next_off = 0;
            }
            out_ctrl += 3;
            dword_off = next_off;
        }
    }

    uint8_t* binder = gfxc + 0x40;
    const void* cs_sg = reinterpret_cast<const void*>(GX_VMA(0x0546b9f0ULL));

    using BindCsFn = void (*)(void*, void*, const void*);
    using InitRegBufFn = void (*)(void*, const void*, uint32_t, uint32_t);
    using SetMemTypeFn = void (*)(void*, int32_t, uint32_t, uint32_t);
    using PreDispatchCsFn = void (*)(void*, void*, const void*);
    using DispatchFn = void (*)(void*, uint32_t, uint32_t, uint32_t);
    using PostDispatchCsFn = void (*)(void*, void*);

    reinterpret_cast<BindCsFn>(GX_VMA(0x026cd400ULL))(binder, gfxc, cs_sg);

    const uint32_t src_dwords = static_cast<uint32_t>(max_src - min_src) >> 2;
    uint64_t srv0_desc[2] = {0, 0};
    reinterpret_cast<InitRegBufFn>(GX_VMA(0x010707c0ULL))(
        srv0_desc, reinterpret_cast<const void*>(min_src), 0x4404u, src_dwords);
    reinterpret_cast<SetMemTypeFn>(GX_VMA(0x010710c0ULL))(
        srv0_desc,
        static_cast<int32_t>(src_cache_policy) >> 16,
        (src_cache_policy >> 8) & 0xFFu,
        src_cache_policy & 0xFFu);

    *reinterpret_cast<uint64_t*>(gfxc + 0x7920) = srv0_desc[0];
    *reinterpret_cast<uint64_t*>(gfxc + 0x7928) = srv0_desc[1];
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fb0) |= *reinterpret_cast<uint64_t*>(gfxc + 0x1ff8);
    *reinterpret_cast<uint8_t*>(gfxc + 0x1f98) |= 0x01u;

    uint64_t srv1_desc[2] = {0, 0};
    reinterpret_cast<InitRegBufFn>(GX_VMA(0x010707c0ULL))(
        srv1_desc, reinterpret_cast<const void*>(ctrl_va), 0x1ac40du, static_cast<uint32_t>(total_groups));
    reinterpret_cast<SetMemTypeFn>(GX_VMA(0x010710c0ULL))(srv1_desc, 0x6e, 0u, 1u);

    *reinterpret_cast<uint64_t*>(gfxc + 0x7940) = srv1_desc[0];
    *reinterpret_cast<uint64_t*>(gfxc + 0x7948) = srv1_desc[1];
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fb0) |= *reinterpret_cast<uint64_t*>(gfxc + 0x2000);
    *reinterpret_cast<uint8_t*>(gfxc + 0x1f98) |= 0x02u;

    const uint32_t dst_dwords = static_cast<uint32_t>(max_dst - min_dst) >> 2;
    uint64_t uav0_desc[2] = {0, 0};
    reinterpret_cast<InitRegBufFn>(GX_VMA(0x010707c0ULL))(
        uav0_desc, reinterpret_cast<const void*>(min_dst), 0x4404u, dst_dwords);
    reinterpret_cast<SetMemTypeFn>(GX_VMA(0x010710c0ULL))(
        uav0_desc,
        static_cast<int32_t>(dst_cache_policy) >> 16,
        (dst_cache_policy >> 8) & 0xFFu,
        dst_cache_policy & 0xFFu);

    *reinterpret_cast<uint64_t*>(gfxc + 0x9320) = uav0_desc[0];
    *reinterpret_cast<uint64_t*>(gfxc + 0x9328) = uav0_desc[1];
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fa8) |= 0x1000000000ull;
    *reinterpret_cast<uint64_t*>(gfxc + 0x1fb0) |= *reinterpret_cast<uint64_t*>(gfxc + 0x2518);

    reinterpret_cast<PreDispatchCsFn>(GX_VMA(0x026cd5d0ULL))(binder, gfxc, cs_sg);
    reinterpret_cast<DispatchFn>(GX_VMA(0x01074c50ULL))(gfxc, static_cast<uint32_t>(total_groups), 1u, 1u);
    reinterpret_cast<PostDispatchCsFn>(GX_VMA(0x026ce110ULL))(binder, gfxc);

    *reinterpret_cast<uint8_t*>(gfxc + 0x1f98) |= 0x03u;
    *reinterpret_cast<uint8_t*>(gfxc + 0x1fac) |= 0x10u;

    uint8_t* outer_ctx2 = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);
    void* dyn_heap2 = *reinterpret_cast<void**>(outer_ctx2 + 0x08);
    auto** heap_vtbl2 = *reinterpret_cast<void***>(dyn_heap2);
    void* fence_ptr = gfxc;
    using HeapFreeFn = void (*)(void*, void*, GpuRangeNode*);
    reinterpret_cast<HeapFreeFn>(heap_vtbl2[4])(dyn_heap2, &fence_ptr, ctrl_node);
}

// ----------------------------------------------------------------------------
// VMA: 0x26adf50 (Size: 1963 bytes) — FlushBufferUpdates_A
// Gathers all deferred constant/dynamic buffer updates from `queue_a`
// into `CopyRecord[]`, invokes `GpuUtil_ScatterGatherCopy` (`0x26b2810`), and
// marks dirty shader stage constant-buffer bitmasks (`ctx + 0x78 + stage * 0x640`).
// ----------------------------------------------------------------------------
void FlushBufferUpdates_A(PendingUpdateQueueA* queue_a, GnmxContextCore* ctx) {
    auto* ctx_bytes = reinterpret_cast<uint8_t*>(ctx);
    const uint32_t initial_count = queue_a->count;
    const uint32_t initial_tail_count = (initial_count == 0u) ? 0u : (((initial_count + 0x7Fu) & 0x7Fu) + 1u);
    uint64_t** initial_chunks = queue_a->chunk_table;

    if (initial_chunks && initial_chunks[0]) {
        const uint32_t filter_flag = *reinterpret_cast<uint32_t*>(ctx_bytes + 0xb510);

        using TempGetMarkerFn = void* (*)();
        using TempAllocFn = void* (*)(void*, uint64_t, uint64_t);
        using TempFreeToMarkerFn = void (*)(void*);
        using SyncTrackerFlushFn = void (*)(void*, void*);

        void* temp_alloc[2];
        temp_alloc[0] = reinterpret_cast<void*>(GX_VMA(0x052b49f0ULL));
        temp_alloc[1] = reinterpret_cast<TempGetMarkerFn>(GX_VMA(0x0207d5b0ULL))();

        void* fallback_alloc = *reinterpret_cast<void**>(ctx_bytes);
        const uint64_t alloc_bytes = static_cast<uint64_t>(initial_count) * 24ull;
        auto* records = static_cast<CopyRecord*>(
            reinterpret_cast<TempAllocFn>(GX_VMA(0x0207d510ULL))(temp_alloc, alloc_bytes, 8ull));

        void* used_fallback_alloc = nullptr;
        if (fallback_alloc && !records) {
            auto** vtbl = *reinterpret_cast<void***>(fallback_alloc);
            using VtblAllocFn = void* (*)(void*, uint64_t, uint64_t);
            records = static_cast<CopyRecord*>(
                reinterpret_cast<VtblAllocFn>(vtbl[11])(fallback_alloc, alloc_bytes, 8ull));
            if (records) {
                used_fallback_alloc = fallback_alloc;
            }
        }

        uint8_t* gfxc = ctx_bytes + 0x10;
        uint8_t* sync_tracker = ctx_bytes + 0xb478;
        bool used_single_copy_fallback = false;

        if (records) {
            uint64_t** cp = queue_a->chunk_table;
            const uint32_t total = queue_a->count;
            const uint32_t tail_count = (total == 0u) ? 0u : (((total + 0x7Fu) & 0x7Fu) + 1u);
            uint32_t batch_count = 0;

            for (; cp && *cp; ++cp) {
                uint64_t* chunk = *cp;
                const uint32_t limit = (cp[1] != nullptr) ? 0x80u : tail_count;
                for (uint32_t i = 0; i < limit; ++i) {
                    uint64_t* entry = chunk + i * 2u;
                    auto* buf_core = reinterpret_cast<uint8_t*>(entry[0]);
                    if (!buf_core) {
                        continue;
                    }
                    if (filter_flag == 0u || (buf_core[0x25] & 0x08u) == 0u) {
                        if (batch_count == 0u) {
                            ctx_bytes[0xb498] |= 0x01u;
                            reinterpret_cast<SyncTrackerFlushFn>(GX_VMA(0x026b9850ULL))(sync_tracker, gfxc);
                            buf_core = reinterpret_cast<uint8_t*>(entry[0]);
                        }
                        records[batch_count].dst_va = *reinterpret_cast<uint64_t*>(buf_core + 0x08);
                        records[batch_count].src_va = entry[1];
                        records[batch_count].byte_size = *reinterpret_cast<uint32_t*>(buf_core + 0x14);
                        records[batch_count].reserved = gx_direct_vk::kCopyFlagKnownCpuSource;
                        ++batch_count;

                        const uint32_t mask32 = *reinterpret_cast<uint32_t*>(buf_core + 0x2c);
                        uint64_t stage_mask = mask32 >> 16;
                        if (stage_mask != 0) {
                            const uint64_t slot_mask = mask32 & 0xFFFFu;
                            while (stage_mask != 0) {
                                const uint64_t st = static_cast<uint64_t>(__builtin_ctzll(stage_mask));
                                stage_mask -= (1ull << st);
                                auto* ctx_dirty = reinterpret_cast<uint64_t*>(ctx_bytes + 0x78 + st * 0x640);
                                uint64_t val = *ctx_dirty;
                                uint64_t rem = slot_mask;
                                while (rem != 0) {
                                    const uint64_t sl = static_cast<uint64_t>(__builtin_ctzll(rem));
                                    rem -= (1ull << sl);
                                    val |= (1ull << (sl + 16));
                                }
                                *ctx_dirty = val;
                            }
                        }
                    }
                    const uint16_t q_slot = queue_a->fast_slot_lut_idx;
                    if (q_slot <= 0x0Fu) {
                        auto* bc = reinterpret_cast<uint8_t*>(entry[0]);
                        uint16_t* lut = *reinterpret_cast<uint16_t**>(bc + 0x38);
                        lut[q_slot] = 0;
                    }
                }
            }

            if (batch_count != 0u) {
                GpuUtil_ScatterGatherCopy(
                    reinterpret_cast<GnmxContextCore*>(gfxc),
                    0x6e0000u,
                    0x6e0000u,
                    records,
                    batch_count);
                ctx_bytes[0xb498] |= 0x01u;
                ctx_bytes[0xb478] |= 0x01u;
                ctx_bytes[0xb47f] |= 0x08u;
            }
        } else {
            used_single_copy_fallback = true;
            bool synced = false;
            for (uint64_t** cp = initial_chunks; cp && *cp; ++cp) {
                uint64_t* chunk = *cp;
                const uint32_t limit = (cp[1] != nullptr) ? 0x80u : initial_tail_count;
                for (uint32_t i = 0; i < limit; ++i) {
                    uint64_t* entry = chunk + i * 2u;
                    auto* buf_core = reinterpret_cast<uint8_t*>(entry[0]);
                    if (!buf_core) {
                        continue;
                    }
                    if (filter_flag == 0u || (buf_core[0x25] & 0x08u) == 0u) {
                        if (!synced) {
                            ctx_bytes[0xb498] |= 0x01u;
                            reinterpret_cast<SyncTrackerFlushFn>(GX_VMA(0x026b9850ULL))(sync_tracker, gfxc);
                            buf_core = reinterpret_cast<uint8_t*>(entry[0]);
                            synced = true;
                        }
                        const uint64_t single_dst_va = *reinterpret_cast<uint64_t*>(buf_core + 0x08);
                        const uint64_t single_src_va = entry[1];
                        const uint32_t single_bytes = *reinterpret_cast<uint32_t*>(buf_core + 0x14);
                        if (!(gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_SingleCopy) &&
                              (single_bytes == 0u ||
                               gx_direct_vk::EmitCopyBufferFromCpu(
                                   gfxc, single_dst_va, single_src_va, single_bytes)))) {
                            GpuUtil_SingleCopy_Impl(
                                reinterpret_cast<GnmxContextCore*>(gfxc),
                                single_dst_va,
                                0x6e0000u,
                                single_src_va,
                                0x6e0000u,
                                single_bytes);
                        }

                        buf_core = reinterpret_cast<uint8_t*>(entry[0]);
                        const uint32_t mask32 = *reinterpret_cast<uint32_t*>(buf_core + 0x2c);
                        uint64_t stage_mask = mask32 >> 16;
                        if (stage_mask != 0) {
                            const uint64_t slot_mask = mask32 & 0xFFFFu;
                            while (stage_mask != 0) {
                                const uint64_t st = static_cast<uint64_t>(__builtin_ctzll(stage_mask));
                                stage_mask -= (1ull << st);
                                auto* ctx_dirty = reinterpret_cast<uint64_t*>(ctx_bytes + 0x78 + st * 0x640);
                                uint64_t val = *ctx_dirty;
                                uint64_t rem = slot_mask;
                                while (rem != 0) {
                                    const uint64_t sl = static_cast<uint64_t>(__builtin_ctzll(rem));
                                    rem -= (1ull << sl);
                                    val |= (1ull << (sl + 16));
                                }
                                *ctx_dirty = val;
                            }
                        }
                    }
                    const uint16_t q_slot = queue_a->fast_slot_lut_idx;
                    if (q_slot <= 0x0Fu) {
                        auto* bc = reinterpret_cast<uint8_t*>(entry[0]);
                        uint16_t* lut = *reinterpret_cast<uint16_t**>(bc + 0x38);
                        lut[q_slot] = 0;
                    }
                }
            }
            if (synced) {
                ctx_bytes[0xb498] |= 0x01u;
                ctx_bytes[0xb478] |= 0x01u;
                ctx_bytes[0xb47f] |= 0x08u;
            }
        }

        if (queue_a->allocator && queue_a->count != 0u) {
            const uint32_t num_chunks = (queue_a->count + 0x7Fu) >> 7;
            using VtblFreeFn = void (*)(void*, void*);
            for (uint32_t c = 0; c < num_chunks; ++c) {
                void* alloc = queue_a->allocator;
                auto** vtbl = *reinterpret_cast<void***>(alloc);
                reinterpret_cast<VtblFreeFn>(vtbl[14])(alloc, queue_a->chunk_table[c]);
            }
            if (queue_a->chunk_table) {
                void* alloc = queue_a->allocator;
                auto** vtbl = *reinterpret_cast<void***>(alloc);
                reinterpret_cast<VtblFreeFn>(vtbl[14])(alloc, queue_a->chunk_table);
                queue_a->chunk_table = nullptr;
            }
            queue_a->count = 0;
        }

        if (used_fallback_alloc && !used_single_copy_fallback) {
            auto** vtbl = *reinterpret_cast<void***>(used_fallback_alloc);
            using VtblFreeFn = void (*)(void*, void*);
            reinterpret_cast<VtblFreeFn>(vtbl[14])(used_fallback_alloc, records);
        }

        reinterpret_cast<TempFreeToMarkerFn>(GX_VMA(0x0207d5f0ULL))(temp_alloc[1]);
    }

    if (gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_DecommissionLegacyPm4)) {
        gx_direct_vk::ReleaseU32LabelBatch u32Batch(ctx_bytes + 0x10, 4u);
        GpuRangeNode* chain_head = nullptr;
        GpuRangeNode* chain_tail = nullptr;
        auto enqueue_node = [&](GpuRangeNode* node) noexcept {
            node->vtable_or_color = 2ULL;
            if (!chain_head) {
                chain_head = node;
            } else {
                chain_tail->next_free = node;
            }
            chain_tail = node;
            u32Batch.Add(reinterpret_cast<uint64_t>(node));
        };

        uint32_t bucket_mask = *reinterpret_cast<uint32_t*>(ctx_bytes + 0xb470);
        if (bucket_mask != 0u) {
            while (bucket_mask != 0u) {
                const uint32_t b = static_cast<uint32_t>(__builtin_ctz(bucket_mask));
                bucket_mask -= (1u << b);
                auto** bucket_slot = reinterpret_cast<GpuRangeNode**>(
                    ctx_bytes + 0xb3e8 + static_cast<size_t>(b) * 8u);
                GpuRangeNode* node = *bucket_slot;
                while (node) {
                    GpuRangeNode* next = node->next_free;
                    enqueue_node(node);
                    node = next;
                }
                *bucket_slot = nullptr;
            }
            *reinterpret_cast<uint32_t*>(ctx_bytes + 0xb470) = 0u;
        }

        auto** full_slot = reinterpret_cast<GpuRangeNode**>(ctx_bytes + 0xb468);
        GpuRangeNode* full_node = *full_slot;
        while (full_node) {
            GpuRangeNode* next = full_node->next_free;
            enqueue_node(full_node);
            full_node = next;
        }
        *full_slot = nullptr;

        if (chain_head) {
            auto* gpu_heap = *reinterpret_cast<uint8_t**>(ctx_bytes + 0xb3e0);
            auto** head_slot = reinterpret_cast<GpuRangeNode**>(gpu_heap + 0x98);
            GpuRangeNode* old_head = __atomic_load_n(head_slot, __ATOMIC_RELAXED);
            do {
                chain_tail->next_free = old_head;
            } while (!__atomic_compare_exchange_n(
                head_slot, &old_head, chain_head, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
            u32Batch.Flush();
        }
    } else {
        void* fence_ptr = ctx_bytes + 0x10;
        using RetireNodeFn = void (*)(void*, void*, GpuRangeNode*);
        uint32_t bucket_mask = *reinterpret_cast<uint32_t*>(ctx_bytes + 0xb470);
        if (bucket_mask != 0u) {
            while (bucket_mask != 0u) {
                const uint32_t b = static_cast<uint32_t>(__builtin_ctz(bucket_mask));
                bucket_mask -= (1u << b);
                auto** bucket_slot = reinterpret_cast<GpuRangeNode**>(ctx_bytes + 0xb3e8 + static_cast<size_t>(b) * 8u);
                GpuRangeNode* node = *bucket_slot;
                while (node) {
                    GpuRangeNode* next = node->next_free;
                    void* gpu_heap = *reinterpret_cast<void**>(ctx_bytes + 0xb3e0);
                    reinterpret_cast<RetireNodeFn>(GX_VMA(0x026aa5a0ULL))(gpu_heap, &fence_ptr, node);
                    node = next;
                }
                *bucket_slot = nullptr;
            }
            *reinterpret_cast<uint32_t*>(ctx_bytes + 0xb470) = 0u;
        }

        auto** full_slot = reinterpret_cast<GpuRangeNode**>(ctx_bytes + 0xb468);
        GpuRangeNode* full_node = *full_slot;
        while (full_node) {
            GpuRangeNode* next = full_node->next_free;
            void* gpu_heap = *reinterpret_cast<void**>(ctx_bytes + 0xb3e0);
            reinterpret_cast<RetireNodeFn>(GX_VMA(0x026aa5a0ULL))(gpu_heap, &fence_ptr, full_node);
            full_node = next;
        }
        *full_slot = nullptr;
    }
}

void FlushBufferUpdates_B(PendingUpdateQueueB* queue_b, GnmxContextCore* ctx);

// ----------------------------------------------------------------------------
// VMA: 0x26afd30 (Size: 988 bytes) — FlushAndPrepareSubmit
// Flushes `queue_a` (`0x26adf50`), device `queue_b` (`0x26d1090`), surface sync
// barriers, and packages the current DCB command chunk into `out_submit_list`.
// ----------------------------------------------------------------------------
int32_t FlushAndPrepareSubmit(GnmxContextCore* ctx, PendingUpdateQueueB* out_submit_list) {
    auto* gfxc = reinterpret_cast<uint8_t*>(ctx);
    auto* outer_ctx = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);

    using SyncTrackerFlushFn = void (*)(void*, void*);
    using Sub26af700Fn = void (*)(void*);
    using Sub26cd3f0Fn = void (*)(void*, void*);
    using QueryChunkStateFn = uint64_t (*)(void*, uint64_t);
    using ApplyChunkStateFn = void (*)(void*, void*, uint64_t);

    if (outer_ctx) {
        FlushBufferUpdates_A(
            reinterpret_cast<PendingUpdateQueueA*>(gfxc + 0xb3b0),
            reinterpret_cast<GnmxContextCore*>(outer_ctx));
        outer_ctx = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);
        FlushBufferUpdates_B(
            reinterpret_cast<PendingUpdateQueueB*>(outer_ctx + 0xb6f8),
            ctx);
        reinterpret_cast<SyncTrackerFlushFn>(GX_VMA(0x026b9850ULL))(gfxc + 0xb468, gfxc);
    }

    reinterpret_cast<Sub26af700Fn>(GX_VMA(0x026af700ULL))(gfxc);
    reinterpret_cast<Sub26cd3f0Fn>(GX_VMA(0x026cd3f0ULL))(gfxc + 0x40, gfxc);

    uint8_t* chunk = *reinterpret_cast<uint8_t**>(gfxc + 0xb4a0);
    uint64_t last_state = 0;
    if (chunk) {
        const uint32_t cap = *reinterpret_cast<uint32_t*>(gfxc + 0xb4b0);
        const uint64_t next_off = static_cast<uint64_t>(cap) << 6;
        const bool batch_chunks =
            gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb);
        gx_direct_vk::ReleaseLabelBatch chunk_batch(gfxc);
        do {
            uint8_t* next_chunk = *reinterpret_cast<uint8_t**>(chunk + next_off);
            const uint32_t count = next_chunk ? cap : *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4);
            for (uint32_t i = 0; i < count; ++i) {
                const uint64_t cmd_ptr = *reinterpret_cast<uint64_t*>(chunk + (static_cast<uint64_t>(i) << 6));
                void* mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
                const uint64_t st = reinterpret_cast<QueryChunkStateFn>(GX_VMA(0x026d3d40ULL))(mgr, cmd_ptr);
                if (st != last_state) {
                    last_state = 0;
                    if (st != 0) {
                        mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
                        if (batch_chunks) {
                            chunk_batch.AddPoolSlot(mgr, reinterpret_cast<uint64_t*>(st));
                        } else {
                            reinterpret_cast<ApplyChunkStateFn>(GX_VMA(0x026d3c80ULL))(mgr, gfxc, st);
                        }
                        last_state = st;
                    }
                }
            }
            chunk = next_chunk;
        } while (chunk);
        if (batch_chunks) {
            chunk_batch.Flush();
        }
    }

    const uint64_t dcb_cmdptr = *reinterpret_cast<uint64_t*>(gfxc + 0x00);
    const uint32_t diff_lo = static_cast<uint32_t>(*reinterpret_cast<uint64_t*>(gfxc + 0x10)) - static_cast<uint32_t>(dcb_cmdptr);
    uint64_t expected_end = 0;
    if (dcb_cmdptr != 0) {
        auto* mgr = *reinterpret_cast<uint8_t**>(gfxc + 0xb4c8);
        const uint64_t qwords = *reinterpret_cast<uint64_t*>(mgr + 0x08);
        expected_end = dcb_cmdptr + qwords * 8ull - 8ull;
    }

    if (diff_lo != 0u || expected_end != *reinterpret_cast<uint64_t*>(gfxc + 0x08)) {
        const bool batch_tail =
            gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_NativeSyncAndDcb);
        void* mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
        const uint64_t st1 = reinterpret_cast<QueryChunkStateFn>(GX_VMA(0x026d3d40ULL))(mgr, dcb_cmdptr);
        if (st1 != last_state && st1 != 0) {
            mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
            if (batch_tail) {
                gx_direct_vk::ReleaseLabelBatch b1(gfxc);
                b1.AddPoolSlot(mgr, reinterpret_cast<uint64_t*>(st1));
                b1.Flush();
            } else {
                reinterpret_cast<ApplyChunkStateFn>(GX_VMA(0x026d3c80ULL))(mgr, gfxc, st1);
            }
        }

        mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
        const uint64_t st2 = reinterpret_cast<QueryChunkStateFn>(GX_VMA(0x026d3d40ULL))(
            mgr, *reinterpret_cast<uint64_t*>(gfxc + 0x00));
        if (st2 != st1) {
            mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
            if (batch_tail) {
                gx_direct_vk::ReleaseLabelBatch b2(gfxc);
                b2.AddPoolSlot(mgr, reinterpret_cast<uint64_t*>(st2));
                b2.Flush();
            } else {
                reinterpret_cast<ApplyChunkStateFn>(GX_VMA(0x026d3c80ULL))(mgr, gfxc, st2);
            }
            mgr = *reinterpret_cast<void**>(gfxc + 0xb4c8);
            const uint64_t st3 = reinterpret_cast<QueryChunkStateFn>(GX_VMA(0x026d3d40ULL))(
                mgr, *reinterpret_cast<uint64_t*>(gfxc + 0x00));
            if (st3 != st2) {
                return 9;
            }
        }

        const uint32_t cap = *reinterpret_cast<uint32_t*>(gfxc + 0xb4b0);
        const uint64_t next_off = static_cast<uint64_t>(cap) << 6;
        const uint64_t alloc_sz = next_off + 8ull;
        using VtblAllocFn = uint8_t* (*)(void*, uint64_t, uint64_t);

        uint8_t* tail_chunk = nullptr;
        uint32_t used = 0;
        if (*reinterpret_cast<uint8_t**>(gfxc + 0xb4a0) == nullptr) {
            void* alloc = *reinterpret_cast<void**>(gfxc + 0xb498);
            auto** vtbl = *reinterpret_cast<void***>(alloc);
            tail_chunk = reinterpret_cast<VtblAllocFn>(vtbl[11])(alloc, alloc_sz, 8ull);
            if (!tail_chunk) {
                return 9;
            }
            *reinterpret_cast<uint64_t*>(tail_chunk + next_off) = 0;
            *reinterpret_cast<uint8_t**>(gfxc + 0xb4a0) = tail_chunk;
            *reinterpret_cast<uint8_t**>(gfxc + 0xb4a8) = tail_chunk;
            *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4) = 0;
            used = 0;
        } else {
            used = *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4);
            if (used == cap) {
                void* alloc = *reinterpret_cast<void**>(gfxc + 0xb498);
                auto** vtbl = *reinterpret_cast<void***>(alloc);
                tail_chunk = reinterpret_cast<VtblAllocFn>(vtbl[11])(alloc, alloc_sz, 8ull);
                if (!tail_chunk) {
                    return 9;
                }
                *reinterpret_cast<uint64_t*>(tail_chunk + next_off) = 0;
                uint8_t* prev_tail = *reinterpret_cast<uint8_t**>(gfxc + 0xb4a8);
                *reinterpret_cast<uint8_t**>(prev_tail + next_off) = tail_chunk;
                *reinterpret_cast<uint8_t**>(gfxc + 0xb4a8) = tail_chunk;
                *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4) = 0;
                used = 0;
            } else {
                tail_chunk = *reinterpret_cast<uint8_t**>(gfxc + 0xb4a8);
            }
        }

        uint8_t* dst_slot = tail_chunk + (static_cast<uint64_t>(used) << 6);
        *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4) = used + 1u;
        if (!dst_slot) {
            return 9;
        }
        std::memcpy(dst_slot, gfxc, 0x40);
    }

    std::memset(gfxc, 0, 0x18);
    *reinterpret_cast<uint64_t*>(gfxc + 0xb4b8) = 0;

    if (out_submit_list->allocator) {
        const uint32_t old_cap = out_submit_list->entries_per_chunk;
        const uint64_t old_next_off = static_cast<uint64_t>(old_cap) << 6;
        uint8_t* c = out_submit_list->head_chunk;
        using VtblFreeFn = void (*)(void*, void*);
        while (c) {
            uint8_t* next_c = *reinterpret_cast<uint8_t**>(c + old_next_off);
            void* alloc = out_submit_list->allocator;
            auto** vtbl = *reinterpret_cast<void***>(alloc);
            reinterpret_cast<VtblFreeFn>(vtbl[14])(alloc, c);
            c = next_c;
        }
        out_submit_list->tail_used = 0;
        out_submit_list->tail_chunk = nullptr;
        out_submit_list->head_chunk = nullptr;
    }

    out_submit_list->allocator = *reinterpret_cast<void***>(gfxc + 0xb498);
    out_submit_list->head_chunk = *reinterpret_cast<uint8_t**>(gfxc + 0xb4a0);
    out_submit_list->tail_chunk = *reinterpret_cast<uint8_t**>(gfxc + 0xb4a8);
    out_submit_list->entries_per_chunk = *reinterpret_cast<uint32_t*>(gfxc + 0xb4b0);
    out_submit_list->tail_used = *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4);

    *reinterpret_cast<uint32_t*>(gfxc + 0xb4b4) = 0;
    *reinterpret_cast<uint8_t**>(gfxc + 0xb4a8) = nullptr;
    *reinterpret_cast<uint8_t**>(gfxc + 0xb4a0) = nullptr;
    return 0;
}

// ----------------------------------------------------------------------------
// VMA: 0x26d0d30 (Size: 560 bytes) — MapDynamicBuffer_B
// Allocates a dynamic staging node for `target_buf`, records a 24-byte update
// entry `(byte_size, cache_flag, dst_gpu_va, staging_node)` into `PendingUpdateQueueB`,
// and writes the mapped CPU pointer to `*out_mapped_ptr`.
// ----------------------------------------------------------------------------
int32_t MapDynamicBuffer_B_Impl(
    PendingUpdateQueueB* queue_b,
    uint8_t* gfxc,
    uint64_t* out_mapped_ptr,
    uint8_t* target_buf) __asm__("_ZN10Dantelion213CoreGraphics23Gnm18MapDynamicBuffer_BEPNS1_19PendingUpdateQueueBEPmPNS1_10BufferCoreEj");

int32_t MapDynamicBuffer_B_Impl(
    PendingUpdateQueueB* queue_b,
    uint8_t* gfxc,
    uint64_t* out_mapped_ptr,
    uint8_t* target_buf)
{
    const uint32_t byte_size = *reinterpret_cast<uint32_t*>(target_buf + 0x14);
    uint8_t* outer_ctx = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);
    void* dyn_heap = *reinterpret_cast<void**>(outer_ctx + 0x08);

    using HeapAllocFn = GpuRangeNode* (*)(void*, uint64_t, uint64_t);
    using HeapFreeFn = void (*)(void*, void*, GpuRangeNode*);
    using ReclaimFn = void (*)(void*, void*);
    using GetBufGpuVaFn = uint64_t (*)(void*);

    auto** heap_vtbl = *reinterpret_cast<void***>(dyn_heap);
    GpuRangeNode* node = reinterpret_cast<HeapAllocFn>(heap_vtbl[3])(dyn_heap, byte_size, 0x10ull);
    if (!node) {
        void* fence_ptr = gfxc;
        void* inner_heap = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(dyn_heap) + 0x08);
        reinterpret_cast<ReclaimFn>(GX_VMA(0x026aa860ULL))(inner_heap, &fence_ptr);
        heap_vtbl = *reinterpret_cast<void***>(dyn_heap);
        node = reinterpret_cast<HeapAllocFn>(heap_vtbl[3])(dyn_heap, byte_size, 0x10ull);
        if (!node) {
            return 9;
        }
    }

    uint8_t* head = queue_b->head_chunk;
    const uint32_t cap = queue_b->entries_per_chunk;
    const uint64_t target_va = reinterpret_cast<GetBufGpuVaFn>(GX_VMA(0x0216e1b0ULL))(target_buf);

    uint8_t* found_entry = nullptr;
    if (head) {
        const uint64_t next_off = static_cast<uint64_t>(cap) * 24ull;
        for (uint8_t* c = head; c != nullptr;) {
            uint8_t* next_c = *reinterpret_cast<uint8_t**>(c + next_off);
            const uint32_t limit = next_c ? cap : queue_b->tail_used;
            for (uint32_t i = 0; i < limit; ++i) {
                uint8_t* e = c + static_cast<uint64_t>(i) * 24ull;
                if (*reinterpret_cast<uint64_t*>(e + 0x08) == target_va) {
                    found_entry = e;
                    break;
                }
            }
            if (found_entry) {
                break;
            }
            c = next_c;
        }
    }

    if (found_entry) {
        void* fence_ptr = gfxc;
        heap_vtbl = *reinterpret_cast<void***>(dyn_heap);
        GpuRangeNode* old_node = *reinterpret_cast<GpuRangeNode**>(found_entry + 0x10);
        reinterpret_cast<HeapFreeFn>(heap_vtbl[4])(dyn_heap, &fence_ptr, old_node);
        *reinterpret_cast<GpuRangeNode**>(found_entry + 0x10) = node;
        *out_mapped_ptr = node->start_va;
        return 0;
    }

    const uint32_t cap2 = queue_b->entries_per_chunk;
    const uint64_t next_off2 = static_cast<uint64_t>(cap2) * 24ull;
    const uint64_t alloc_sz = next_off2 + 8ull;
    using VtblAllocFn = uint8_t* (*)(void*, uint64_t, uint64_t);

    uint8_t* tail_chunk = nullptr;
    uint32_t used = 0;
    if (!queue_b->head_chunk) {
        void* alloc = queue_b->allocator;
        auto** vtbl = *reinterpret_cast<void***>(alloc);
        tail_chunk = reinterpret_cast<VtblAllocFn>(vtbl[11])(alloc, alloc_sz, 8ull);
        if (!tail_chunk) {
            return 9;
        }
        *reinterpret_cast<uint64_t*>(tail_chunk + next_off2) = 0;
        queue_b->head_chunk = tail_chunk;
        queue_b->tail_chunk = tail_chunk;
        queue_b->tail_used = 0;
        used = 0;
    } else {
        used = queue_b->tail_used;
        if (used == cap2) {
            void* alloc = queue_b->allocator;
            auto** vtbl = *reinterpret_cast<void***>(alloc);
            tail_chunk = reinterpret_cast<VtblAllocFn>(vtbl[11])(alloc, alloc_sz, 8ull);
            if (!tail_chunk) {
                return 9;
            }
            *reinterpret_cast<uint64_t*>(tail_chunk + next_off2) = 0;
            *reinterpret_cast<uint8_t**>(queue_b->tail_chunk + next_off2) = tail_chunk;
            queue_b->tail_chunk = tail_chunk;
            queue_b->tail_used = 0;
            used = 0;
        } else {
            tail_chunk = queue_b->tail_chunk;
        }
    }

    uint8_t* new_entry = tail_chunk + static_cast<uint64_t>(used) * 24ull;
    queue_b->tail_used = used + 1u;
    if (!new_entry) {
        return 9;
    }

    *reinterpret_cast<uint32_t*>(new_entry + 0x00) = byte_size;
    const uint32_t buf_flags = *reinterpret_cast<uint32_t*>(target_buf + 0x24);
    *reinterpret_cast<uint32_t*>(new_entry + 0x04) = (~(buf_flags >> 11)) & 1u;
    *reinterpret_cast<uint64_t*>(new_entry + 0x08) = reinterpret_cast<GetBufGpuVaFn>(GX_VMA(0x0216e1b0ULL))(target_buf);
    *reinterpret_cast<GpuRangeNode**>(new_entry + 0x10) = node;
    *out_mapped_ptr = node->start_va;
    return 0;
}

// ----------------------------------------------------------------------------
// VMA: 0x26d1090 (Size: 1526 bytes) — FlushBufferUpdates_B
// Iterates all 24-byte entries across the linked chunks of `queue_b`, coalesces
// non-filtered updates into `CopyRecord[]`, and dispatches `GpuUtil_ScatterGatherCopy` (`0x26b2810`).
// ----------------------------------------------------------------------------
void FlushBufferUpdates_B(PendingUpdateQueueB* queue_b, GnmxContextCore* ctx) {
    if (!queue_b->head_chunk) {
        return;
    }
    auto* gfxc = reinterpret_cast<uint8_t*>(ctx);
    uint8_t* outer_ctx = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);
    void* dyn_heap = *reinterpret_cast<void**>(outer_ctx + 0x08);

    uint32_t total = 0;
    if (queue_b->head_chunk) {
        const uint32_t cap = queue_b->entries_per_chunk;
        const uint64_t next_off = static_cast<uint64_t>(cap) * 24ull;
        for (uint8_t* c = queue_b->head_chunk; c != nullptr; c = *reinterpret_cast<uint8_t**>(c + next_off)) {
            total += cap;
        }
    }
    total += queue_b->tail_used;

    const uint32_t filter_flag = *reinterpret_cast<uint32_t*>(outer_ctx + 0xb510);

    using TempGetMarkerFn = void* (*)();
    using TempAllocFn = void* (*)(void*, uint64_t, uint64_t);
    using TempFreeToMarkerFn = void (*)(void*);
    using SyncTrackerFlushFn = void (*)(void*, void*);
    using RetireNodeVtblFn = uint64_t (*)(void*, void*, GpuRangeNode*, uint64_t);

    void* temp_alloc[2];
    temp_alloc[0] = reinterpret_cast<void*>(GX_VMA(0x052b49f0ULL));
    temp_alloc[1] = reinterpret_cast<TempGetMarkerFn>(GX_VMA(0x0207d5b0ULL))();

    uint8_t* outer_ctx2 = *reinterpret_cast<uint8_t**>(gfxc + 0xb490);
    void* fallback_alloc = *reinterpret_cast<void**>(outer_ctx2);
    const uint64_t alloc_bytes = static_cast<uint64_t>(total) * 24ull;
    auto* records = static_cast<CopyRecord*>(
        reinterpret_cast<TempAllocFn>(GX_VMA(0x0207d510ULL))(temp_alloc, alloc_bytes, 8ull));

    void* used_fallback_alloc = nullptr;
    if (fallback_alloc && !records) {
        auto** vtbl = *reinterpret_cast<void***>(fallback_alloc);
        using VtblAllocFn = void* (*)(void*, uint64_t, uint64_t);
        records = static_cast<CopyRecord*>(
            reinterpret_cast<VtblAllocFn>(vtbl[11])(fallback_alloc, alloc_bytes, 8ull));
        if (records) {
            used_fallback_alloc = fallback_alloc;
        }
    }

    if (records) {
        uint8_t* c = queue_b->head_chunk;
        if (c) {
            const uint32_t cap = queue_b->entries_per_chunk;
            const uint64_t next_off = static_cast<uint64_t>(cap) * 24ull;
            uint8_t* sync_tracker = gfxc + 0xb468;
            uint32_t batch_count = 0;

            for (; c != nullptr;) {
                uint8_t* next_c = *reinterpret_cast<uint8_t**>(c + next_off);
                const uint32_t limit = next_c ? cap : queue_b->tail_used;
                for (uint32_t i = 0; i < limit; ++i) {
                    uint8_t* e = c + static_cast<uint64_t>(i) * 24ull;
                    if (filter_flag != 0u && *reinterpret_cast<uint32_t*>(e + 0x04) == 0u) {
                        continue;
                    }
                    auto* node = *reinterpret_cast<GpuRangeNode**>(e + 0x10);
                    const uint64_t src_va = node->start_va;
                    if (batch_count == 0u) {
                        gfxc[0xb488] |= 0x01u;
                        reinterpret_cast<SyncTrackerFlushFn>(GX_VMA(0x026b9850ULL))(sync_tracker, gfxc);
                    }
                    records[batch_count].dst_va = *reinterpret_cast<uint64_t*>(e + 0x08) & 0xFFFFFFFFFFull;
                    records[batch_count].src_va = src_va;
                    records[batch_count].byte_size = *reinterpret_cast<uint32_t*>(e + 0x00);
                    records[batch_count].reserved = gx_direct_vk::kCopyFlagKnownCpuSource;
                    ++batch_count;
                }
                c = next_c;
            }

            if (batch_count != 0u) {
                GpuUtil_ScatterGatherCopy(ctx, 0x6e0001u, 0x6e0001u, records, batch_count);
                gfxc[0xb488] |= 0x01u;
                gfxc[0xb468] |= 0x01u;
            }

            uint64_t prev_ret = 0;
            void* fence_ptr = gfxc;
            for (uint8_t* ch = queue_b->head_chunk; ch != nullptr;) {
                uint8_t* next_ch = *reinterpret_cast<uint8_t**>(ch + next_off);
                const uint32_t limit = next_ch ? cap : queue_b->tail_used;
                for (uint32_t i = 0; i < limit; ++i) {
                    uint8_t* e = ch + static_cast<uint64_t>(i) * 24ull;
                    auto* node = *reinterpret_cast<GpuRangeNode**>(e + 0x10);
                    auto** heap_vtbl = *reinterpret_cast<void***>(dyn_heap);
                    prev_ret = reinterpret_cast<RetireNodeVtblFn>(heap_vtbl[6])(
                        dyn_heap, &fence_ptr, node, prev_ret);
                }
                ch = next_ch;
            }
        }
    } else if (queue_b->head_chunk) {
        const uint32_t cap = queue_b->entries_per_chunk;
        const uint64_t next_off = static_cast<uint64_t>(cap) * 24ull;
        uint8_t* sync_tracker = gfxc + 0xb468;
        bool synced = false;
        uint64_t prev_ret = 0;
        void* fence_ptr = gfxc;

        for (uint8_t* ch = queue_b->head_chunk; ch != nullptr;) {
            uint8_t* next_ch = *reinterpret_cast<uint8_t**>(ch + next_off);
            const uint32_t limit = next_ch ? cap : queue_b->tail_used;
            for (uint32_t i = 0; i < limit; ++i) {
                uint8_t* e = ch + static_cast<uint64_t>(i) * 24ull;
                auto* node = *reinterpret_cast<GpuRangeNode**>(e + 0x10);
                if (filter_flag == 0u || *reinterpret_cast<uint32_t*>(e + 0x04) != 0u) {
                    const uint64_t src_va = node->start_va;
                    if (!synced) {
                        gfxc[0xb488] |= 0x01u;
                        reinterpret_cast<SyncTrackerFlushFn>(GX_VMA(0x026b9850ULL))(sync_tracker, gfxc);
                        synced = true;
                    }
                    const uint64_t single_dst_va = *reinterpret_cast<uint64_t*>(e + 0x08) & 0xFFFFFFFFFFull;
                    const uint32_t single_bytes = *reinterpret_cast<uint32_t*>(e + 0x00);
                    if (!(gx_direct_vk::EnabledV1Sub(gx_direct_vk::kV1Sub_SingleCopy) &&
                          (single_bytes == 0u ||
                           gx_direct_vk::EmitCopyBufferFromCpu(
                               ctx, single_dst_va, src_va, single_bytes)))) {
                        GpuUtil_SingleCopy_Impl(
                            ctx,
                            single_dst_va,
                            0x6e0001u,
                            src_va,
                            0x6e0001u,
                            single_bytes);
                    }
                }
                auto** heap_vtbl = *reinterpret_cast<void***>(dyn_heap);
                prev_ret = reinterpret_cast<RetireNodeVtblFn>(heap_vtbl[6])(dyn_heap, &fence_ptr, node, prev_ret);
            }
            ch = next_ch;
        }
        if (synced) {
            gfxc[0xb488] |= 0x01u;
            gfxc[0xb468] |= 0x01u;
        }
    }

    if (queue_b->allocator && queue_b->head_chunk) {
        const uint32_t cap = queue_b->entries_per_chunk;
        const uint64_t next_off = static_cast<uint64_t>(cap) * 24ull;
        using VtblFreeFn = void (*)(void*, void*);
        for (uint8_t* ch = queue_b->head_chunk; ch != nullptr;) {
            uint8_t* next_ch = *reinterpret_cast<uint8_t**>(ch + next_off);
            void* alloc = queue_b->allocator;
            auto** vtbl = *reinterpret_cast<void***>(alloc);
            reinterpret_cast<VtblFreeFn>(vtbl[14])(alloc, ch);
            ch = next_ch;
        }
    }
    queue_b->tail_used = 0;
    queue_b->head_chunk = nullptr;
    queue_b->tail_chunk = nullptr;

    if (used_fallback_alloc && records) {
        auto** vtbl = *reinterpret_cast<void***>(used_fallback_alloc);
        using VtblFreeFn = void (*)(void*, void*);
        reinterpret_cast<VtblFreeFn>(vtbl[14])(used_fallback_alloc, records);
    }

    reinterpret_cast<TempFreeToMarkerFn>(GX_VMA(0x0207d5f0ULL))(temp_alloc[1]);
}

} // namespace Dantelion2::CoreGraphics2::Gnm
