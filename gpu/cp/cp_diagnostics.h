// SPDX-License-Identifier: GPL-2.0-or-later
// bbport cp: diagnostics of the command stream. BB_DCB_STATS=1: the structure of the graphics
// command buffers every 5 s (input for decoding them on several threads); bad packets reported
// with their surroundings; BB_FREE_CHECK: submissions the guest changed before they were decoded.
#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "common/types.h"

namespace BbCp::Diagnostics {

struct SubmittedCopy;
/// BB_GNM_WRITES_TRACE: retain bounded recent snapshots to identify reused command storage.
void RememberSubmittedCopy(std::shared_ptr<const SubmittedCopy> copy);

/// A type-3 opcode's name (its number in hex if unknown).
std::string OpName(u32 op);

bool DcbStatsEnabled();
/// A graphics buffer about to be decoded (`depth` > 0: an indirect buffer).
void ScanDcb(std::span<const u32> dcb, int depth);
/// WAIT_REG_MEM outcomes: 0 display buffer label, 1 pending fence, 2 met at once, 3 waited.
void NoteWait(int outcome, double ms = 0);

/// A packet that cannot be decoded: where, in which buffer, what surrounds it.
void ReportBadPacket(uintptr_t base, std::size_t dwords, const u32* at, u64 seq, int depth,
                     const SubmittedCopy* copy = nullptr);

/// Where a label for the guest is (a fence, an end-of-pipe or release event's value).
enum class LabelStep : u8 {
    ComputeDecoded, ///< RELEASE_MEM decoded (compute)
    ComputeOnGpu,   ///< RELEASE_MEM recorded as a GPU write
    ComputeWritten, ///< RELEASE_MEM written by a CPU thread
    GraphicsOnGpu,  ///< EOP/EOS recorded as a GPU write
    GraphicsWritten ///< EOP/EOS written by a CPU thread
};
/// Kept in a ring of the last 256 (any thread; a few nanoseconds).
void NoteLabel(LabelStep step, u64 address, u64 value, u32 bytes);
/// The last `count` labels (oldest first), each with its age and what its memory holds now.
std::string RecentLabels(size_t count);
/// Guest memory read through the backing view (no fault, any protection); false when unmapped.
bool ReadGuest(const void* address, void* out, u32 bytes);

/// Uninitialized word buffer for SubmittedCopy (avoids 1.2 MB/frame memset(0) before memcpy).
struct UninitWords {
    std::unique_ptr<u32[]> ptr;
    std::size_t len = 0;
    std::size_t cap = 0;

    void resize_uninit(std::size_t n) {
        if (n > cap) {
            const std::size_t new_cap = std::max(n, cap + (cap >> 1));
            ptr = std::make_unique_for_overwrite<u32[]>(new_cap);
            cap = new_cap;
        }
        len = n;
    }
    [[nodiscard]] u32* data() noexcept { return ptr.get(); }
    [[nodiscard]] const u32* data() const noexcept { return ptr.get(); }
    [[nodiscard]] std::size_t size() const noexcept { return len; }
    [[nodiscard]] std::size_t capacity() const noexcept { return cap; }
    u32& operator[](std::size_t i) noexcept { return ptr[i]; }
    const u32& operator[](std::size_t i) const noexcept { return ptr[i]; }
};

/// A submission's command buffers as copied when submitted (BB_COPY_GPU_BUFFERS).
struct SubmittedCopy {
    UninitWords data; ///< dcb, then ccb
    const u32* guest_dcb{};
    std::size_t dcb_dwords{};
    const u32* guest_ccb{};
    std::size_t ccb_dwords{};
    std::chrono::steady_clock::time_point submitted;
    std::size_t bad_dcb_at_submit = ~std::size_t(0);
    bool inspected = false;
};
/// Acquires a pooled SubmittedCopy sized for `dwords` without zero-filling.
std::shared_ptr<SubmittedCopy> AcquireSubmittedCopy(std::size_t dwords);
/// Whether eager submit-time PM4 preflight inspection is enabled.
bool PreflightInspectEnabled() noexcept;
/// Record whether the submitted snapshot already contains a malformed top-level packet.
void InspectSubmittedCopy(SubmittedCopy& copy);
/// After it was decoded: whether the guest changed its command buffers since.
void CheckSubmittedCopy(const SubmittedCopy& copy, u64 seq);

} // namespace BbCp::Diagnostics
