// SPDX-License-Identifier: GPL-2.0-or-later
// bbport textures: which images lie over each 1 MiB page of the guest's address space, as a
// two-level table (1024 x 1024 pages over 40 bits; second-level blocks made on first use).
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>

#include <boost/container/small_vector.hpp>

#include "common/types.h"
#include "textures/types.h"

namespace VideoCore {

class ImageRegistry {
public:
    static constexpr u32 PageBits = 20;
    using Ids = boost::container::small_vector<ImageId, 16>;

    /// The images over `page`, or null when no image ever was.
    Ids* Find(u64 page) {
        Block* block = blocks[(page >> BlockBits) & (NumBlocks - 1)].get();
        return block ? &(*block)[page & (PagesPerBlock - 1)] : nullptr;
    }
    const Ids* Find(u64 page) const noexcept {
        const Block* block = blocks[(page >> BlockBits) & (NumBlocks - 1)].get();
        return block ? &(*block)[page & (PagesPerBlock - 1)] : nullptr;
    }
    /// The images over `page` (its block made if needed).
    Ids& At(u64 page) {
        auto& block = blocks[(page >> BlockBits) & (NumBlocks - 1)];
        if (!block) {
            block = std::make_unique<Block>();
        }
        return (*block)[page & (PagesPerBlock - 1)];
    }

    /// Calls `func(ids)` for the pages of [address, address + size) that hold images, until it
    /// returns true.
    template <typename Func>
    void ForEachPage(VAddr address, u64 size, Func&& func) {
        const u64 last = (address + size - 1) >> PageBits;
        for (u64 page = address >> PageBits; page <= last; ++page) {
            if (Ids* ids = Find(page); ids && !ids->empty() && func(*ids)) {
                return;
            }
        }
    }
    template <typename Func>
    void ForEachPage(VAddr address, u64 size, Func&& func) const {
        const u64 last = (address + size - 1) >> PageBits;
        for (u64 page = address >> PageBits; page <= last; ++page) {
            if (const Ids* ids = Find(page); ids && !ids->empty() && func(*ids)) {
                return;
            }
        }
    }
    /// Calls `func(page)` for every page of [address, address + size).
    template <typename Func>
    static void ForEachPageIndex(VAddr address, u64 size, Func&& func) {
        const u64 last = (address + size - 1) >> PageBits;
        for (u64 page = address >> PageBits; page <= last; ++page) {
            func(page);
        }
    }

    void SetPageActive(u64 page) noexcept {
        const u64 idx = page & (TotalPages - 1ULL);
        active_pages_bitmap[idx >> 6].fetch_or(1ULL << (idx & 63ULL), std::memory_order_release);
    }

    void ClearPageActive(u64 page) noexcept {
        const u64 idx = page & (TotalPages - 1ULL);
        active_pages_bitmap[idx >> 6].fetch_and(~(1ULL << (idx & 63ULL)),
                                                std::memory_order_release);
    }

    [[nodiscard]] bool AnyPageActive(VAddr address, u64 size) const noexcept {
        if (size == 0 || address >= MaxAddressSpace) {
            return false;
        }
        const VAddr end = std::min<VAddr>(address + size, MaxAddressSpace);
        if (address >= end) {
            return false;
        }
        const u64 first_page = address >> PageBits;
        const u64 last_page = (end - 1ULL) >> PageBits;
        const u64 first_word = first_page >> 6;
        const u64 last_word = last_page >> 6;
        if (first_word == last_word) {
            const u64 lo_mask = ~0ULL << (first_page & 63ULL);
            const u64 hi_mask = ~0ULL >> (63ULL - (last_page & 63ULL));
            return (active_pages_bitmap[first_word].load(std::memory_order_acquire) &
                    (lo_mask & hi_mask)) != 0ULL;
        }
        const u64 lo_mask = ~0ULL << (first_page & 63ULL);
        if ((active_pages_bitmap[first_word].load(std::memory_order_acquire) & lo_mask) != 0ULL) {
            return true;
        }
        for (u64 w = first_word + 1; w < last_word; ++w) {
            if (active_pages_bitmap[w].load(std::memory_order_acquire) != 0ULL) {
                return true;
            }
        }
        const u64 hi_mask = ~0ULL >> (63ULL - (last_page & 63ULL));
        return (active_pages_bitmap[last_word].load(std::memory_order_acquire) & hi_mask) != 0ULL;
    }

private:
    static constexpr VAddr MaxAddressSpace = 1ull << 40;
    static constexpr u32 BlockBits = 10;
    static constexpr u64 PagesPerBlock = 1ull << BlockBits;
    static constexpr u64 NumBlocks = 1ull << (40 - PageBits - BlockBits);
    static constexpr u64 TotalPages = PagesPerBlock * NumBlocks;
    static constexpr u64 BitmapWords = TotalPages / 64ull;
    using Block = std::array<Ids, PagesPerBlock>;

    std::array<std::unique_ptr<Block>, NumBlocks> blocks{};
    std::array<std::atomic<u64>, BitmapWords> active_pages_bitmap{};
};

} // namespace VideoCore
