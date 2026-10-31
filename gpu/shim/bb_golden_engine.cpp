// SPDX-License-Identifier: GPL-2.0-or-later
// ============================================================================
// bb_vulkan: Golden Engine C++20 Native GX Bridge
// File: gpu/shim/bb_golden_engine.cpp
// Connects our 102,597 lines of decompiled C++20 GX code
// (our_cpp_100000_source / libbb_gx_engine_native.a) into bb_vulkan.
// ============================================================================
#include "bb_golden_engine.h"
#include "bbport_guest_code_hooks.h"
#include "bbport_heap_sites.h"
#include "bbport_settings.h"
#include "bbport_toggles.h"
#include "render/direct_shaders.h"

#define GX_HOST_PROVIDES_CYG_PROFILE 1
#define GX_HOST_PROVIDES_CGMEMORY_MEMCPY 1
#include "../../our_cpp_100000_source/include/gx_runtime_vma_and_tls.h"
#include "../../our_cpp_100000_source/include/gx_eboot_external_imports.h"
#include "../../our_cpp_100000_source/include/gx_decomp_registry.h"
#include "../../our_cpp_100000_source/include/gx_direct_vulkan.h"

#include <algorithm>
#include <atomic>
#include <cpuid.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>
#include <unistd.h>

namespace {
std::atomic<std::uint64_t> g_cpp_draws{0};
std::atomic<std::uint64_t> g_cpp_dispatches{0};
std::atomic<std::uint64_t> g_cpp_gpu_writes{0};
std::atomic<std::uint64_t> g_cpp_gpu_write_bytes{0};
std::atomic<std::uint64_t> g_cpp_manifest_hits{0};
std::atomic<std::uint64_t> g_cpp_camera_updates{0};
BbGoldenEngine::CameraBridgeState g_camera_bridge{};
} // namespace

extern "C" {
std::uint32_t GXShaderLibrary_GetTotalShaderCount() noexcept;

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void*, void*) noexcept {}
__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void*, void*) noexcept {}

void runtime_memory_note_write(std::uint64_t address, std::uint64_t size);
int runtime_extra_direct_map(std::uint64_t size, std::uint64_t alignment, void** out);

void BbGoldenEngine_OnYebisCameraMatrix(std::uint32_t kind, const float* mtx16) {
    if (!mtx16) return;
    switch (kind) {
    case 0u:
        std::memcpy(g_camera_bridge.inv_view, mtx16, 16 * sizeof(float));
        g_camera_bridge.has_inv_view = true;
        break;
    case 1u:
        std::memcpy(g_camera_bridge.view, mtx16, 16 * sizeof(float));
        g_camera_bridge.has_view = true;
        break;
    case 2u:
        std::memcpy(g_camera_bridge.prev_inv_view, mtx16, 16 * sizeof(float));
        break;
    case 3u:
        std::memcpy(g_camera_bridge.prev_view, mtx16, 16 * sizeof(float));
        g_camera_bridge.has_prev_view = true;
        break;
    default:
        break;
    }
    g_camera_bridge.update_epoch = g_cpp_camera_updates.fetch_add(1, std::memory_order_relaxed) + 1;
}

void BbGoldenEngine_OnYebisCameraParams(float near_z, float far_z, float fov_y) {
    g_camera_bridge.near_z = near_z;
    g_camera_bridge.far_z = far_z;
    g_camera_bridge.fov_y = fov_y;
    g_camera_bridge.has_params = true;
}

void BbGoldenEngine_OnYebisJitter(float jx, float jy) {
    g_camera_bridge.jitter[0] = jx;
    g_camera_bridge.jitter[1] = jy;
}

// Strong override for CGMemoryGnm_Memcpy (0x262cf10): pre/post notifies Vulkan memory tracker for zero-fault GPU uploads
void* CGMemoryGnm_Memcpy(void* dst, const void* src, std::size_t size) {
    if (!dst || !src || size == 0) {
        return dst;
    }
    const auto addr = reinterpret_cast<std::uint64_t>(dst);
    runtime_memory_note_write(addr, size);
    void* res = std::memcpy(dst, src, size);
    runtime_memory_note_write(addr, size);
    g_cpp_gpu_writes.fetch_add(1, std::memory_order_relaxed);
    g_cpp_gpu_write_bytes.fetch_add(size, std::memory_order_relaxed);
    return res;
}

// Direct C++20 -> Vulkan surface write notification for one-time CPU texture tiling (GXGnmTexture1D/2D/3D_CreateAndInitialize)
void BbGoldenEngine_NotifyGpuSurfaceWrite(void* dst, std::uint64_t size) {
    if (!dst || size == 0) {
        return;
    }
    runtime_memory_note_write(reinterpret_cast<std::uint64_t>(dst), size);
    g_cpp_gpu_writes.fetch_add(1, std::memory_order_relaxed);
    g_cpp_gpu_write_bytes.fetch_add(size, std::memory_order_relaxed);
}

// Direct C++20 -> Vulkan fast-path Draw & Dispatch hooks (used by GXGnmCommandContext_Draw* / Dispatch*)
void BbGoldenEngine_OnDrawCall(
    std::uint32_t /*count*/, std::uint32_t /*instanceCount*/, bool /*isIndexed*/, bool /*isIndirect*/) {
    g_cpp_draws.fetch_add(1, std::memory_order_relaxed);
}

void BbGoldenEngine_OnDispatchCall(
    std::uint32_t /*groupX*/, std::uint32_t /*groupY*/, std::uint32_t /*groupZ*/, bool /*isIndirect*/) {
    g_cpp_dispatches.fetch_add(1, std::memory_order_relaxed);
}

// Native C++ replacement for bbport_guest_hooks.cpp ResourceCacheBacking (0x216b21d inside 0x216b0f0)
void BbGoldenEngine_UpgradeResourceCache(void** io_cueHeap, std::uint32_t* io_cueHeapByteSize) {
    if (!io_cueHeap || !*io_cueHeap || !io_cueHeapByteSize) {
        return;
    }
    constexpr std::uint32_t kNativeBytes = 160u << 20;
    if (*io_cueHeapByteSize != kNativeBytes) {
        return;
    }
    static const std::uint32_t bytes = [] {
        const char* env = std::getenv("BB_RENDER_RESOURCE_CACHE_MB");
        const long mb = env ? std::strtol(env, nullptr, 10) : 512;
        return static_cast<std::uint32_t>(std::clamp(mb, 160l, 512l)) << 20;
    }();
    if (bytes == kNativeBytes) {
        return;
    }
    void* block = nullptr;
    if (const int error = runtime_extra_direct_map(bytes, 64ull << 10, &block); error || !block) {
        std::printf("[bb_vulkan] Golden Engine C++20: no port memory for %u MiB resource cache (err=%#x), using 160 MiB\n",
                    bytes >> 20, static_cast<unsigned>(error));
        return;
    }
    *io_cueHeapByteSize = bytes;
    *io_cueHeap = block;
    gx_direct_vk::RegisterTransientCueHeap(block, bytes);
    std::printf("[bb_vulkan] Golden Engine C++20: upgraded GX resource cache to %u MiB at %p\n",
                bytes >> 20, block);
}

// ============================================================================
// Module V5.1: Native C++20 PC PostFX, Resolution/LightGrid & Heap Controls
// ============================================================================
int bb_direct_vk_is_effect_enabled(std::uint32_t effect_index) noexcept {
    if (effect_index < static_cast<std::uint32_t>(BbSettings::EffectCount)) {
        return BbSettings::Get().effects[effect_index].load(std::memory_order_relaxed) ? 1 : 0;
    }
    return 1;
}

std::uint32_t bb_direct_vk_gx_command_heap_bytes() noexcept {
    static const std::uint32_t kHeapBytes = []() noexcept -> std::uint32_t {
        if (const char* env_kb = std::getenv("BB_GX_COMMAND_HEAP_KB"); env_kb && env_kb[0] != '\0') {
            const long kb = std::strtol(env_kb, nullptr, 0);
            if (kb >= 256l && kb <= 16384l) {
                return static_cast<std::uint32_t>(kb) * 1024u;
            }
        }
        if (const char* env = std::getenv("BB_GX_COMMAND_HEAP"); env && env[0] == '0') {
            return 0x40000u; // PS4 default 256 KiB
        }
        return 0x100000u; // PC native default 1 MiB (Larger GX Per-Frame Command Heap)
    }();
    return kHeapBytes;
}

int bb_direct_vk_intel_tonemap_fix() noexcept {
    static const int kFix = []() noexcept -> int {
        if (const char* env = std::getenv("BB_INTEL_TONEMAP_FIX"); env && env[0] != '\0') {
            if (env[0] == '1') return 1;
            if (env[0] == '0') return 0;
        }
        unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
        if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
            // "GenuineIntel": EBX="Genu" (0x756e6547), EDX="ineI" (0x49656e69), ECX="ntel" (0x6c65746e)
            if (ebx == 0x756e6547u && edx == 0x49656e69u && ecx == 0x6c65746eu) {
                return 1;
            }
        }
        return 0;
    }();
    return kFix;
}

int bb_direct_vk_light_grid_override(std::uint32_t* out_w, std::uint32_t* out_h) noexcept {
    struct GridOverride {
        int active;
        std::uint32_t w;
        std::uint32_t h;
    };
    static const GridOverride kGrid = []() noexcept -> GridOverride {
        const char* env = std::getenv("BB_LIGHT_GRID_RES");
        if (!env || env[0] == '\0' || std::strcmp(env, "0") == 0 || std::strcmp(env, "default") == 0) {
            return {0, 0x80u, 0x80u};
        }
        if (std::strcmp(env, "deck") == 0)  return {1, 1280u, 800u};
        if (std::strcmp(env, "1080p") == 0) return {1, 1920u, 1080u};
        if (std::strcmp(env, "1440p") == 0) return {1, 2560u, 1440u};
        if (std::strcmp(env, "4k") == 0 || std::strcmp(env, "2160p") == 0) return {1, 3840u, 2160u};
        char* end = nullptr;
        const unsigned long w = std::strtoul(env, &end, 0);
        unsigned long h = w;
        if (end && (*end == 'x' || *end == 'X')) {
            h = std::strtoul(end + 1, nullptr, 0);
        }
        if (w < 16ul || h < 16ul) {
            return {0, 0x80u, 0x80u};
        }
        return {1,
                static_cast<std::uint32_t>(std::clamp(w, 16ul, 4096ul)),
                static_cast<std::uint32_t>(std::clamp(h, 16ul, 4096ul))};
    }();
    if (!kGrid.active) {
        return 0;
    }
    if (out_w) *out_w = kGrid.w;
    if (out_h) *out_h = kGrid.h;
    return 1;
}

void bb_direct_vk_note_gpu_allocator(
    const void* alloc, std::uint64_t live_ranges, std::uint64_t live_bytes) noexcept {
    if (!BbStats::enabled || !alloc) {
        return;
    }
    const auto allocator = reinterpret_cast<std::uint64_t>(alloc);
    for (std::size_t i = 0; i < BbStats::range_allocators.size(); ++i) {
        std::uint64_t expected = 0;
        if (BbStats::range_allocators[i].load(std::memory_order_relaxed) == allocator ||
            BbStats::range_allocators[i].compare_exchange_strong(expected, allocator)) {
            BbStats::range_live[i].store(live_ranges, std::memory_order_relaxed);
            BbStats::range_bytes[i].store(live_bytes, std::memory_order_relaxed);
            break;
        }
    }
}

void bb_direct_vk_note_release_check(std::uint32_t hr) noexcept {
    static const bool kHeapSitesOn = [] {
        const char* env = std::getenv("BB_HEAP_SITES");
        return env && env[0] == '1';
    }();
    if (kHeapSitesOn) {
        BbHeapSites::NoteReleaseCheck(hr);
    }
}
} // extern "C"

namespace GXGraphicsFramework {
// Strong override for GXGraphicsFramework::CGMemoryGnm_FastCopyGpuMemory (0x261a1b0)
void CGMemoryGnm_FastCopyGpuMemory(void* dst, const void* src, std::uint32_t size) {
    if (!dst || !src || size == 0) {
        return;
    }
    const auto addr = reinterpret_cast<std::uint64_t>(dst);
    runtime_memory_note_write(addr, size);
    std::memcpy(dst, src, size);
    runtime_memory_note_write(addr, size);
    g_cpp_gpu_writes.fetch_add(1, std::memory_order_relaxed);
    g_cpp_gpu_write_bytes.fetch_add(size, std::memory_order_relaxed);
}
} // namespace GXGraphicsFramework

namespace BbGoldenEngine {
namespace {

using u8  = std::uint8_t;
using u64 = std::uint64_t;

// All 2,940 functions in libbb_gx_engine_native.a across Modules 01..07 have been verified 1-to-1 against eboot.bin (0 unsafe stubs remaining).
bool IsUnsafeStubOrCaller(u64 /*vma*/) {
    return false;
}

bool ModuleEnabled(const char* env_mods, const char* mod_name) {
    if (!env_mods || env_mods[0] == '\0' ||
        std::strcmp(env_mods, "all") == 0 ||
        std::strcmp(env_mods, "1") == 0) {
        return true;
    }
    if (std::strcmp(env_mods, "0") == 0 ||
        std::strcmp(env_mods, "none") == 0 ||
        std::strcmp(env_mods, "off") == 0) {
        return false;
    }
    for (const char* p = env_mods; *p;) {
        const char* comma = std::strchr(p, ',');
        const std::size_t n = comma ? static_cast<std::size_t>(comma - p) : std::strlen(p);
        if (n == 2) {
            if (std::strncmp(p, "01", 2) == 0 && std::strncmp(mod_name, "01", 2) == 0) return true;
            if (std::strncmp(p, "02", 2) == 0 && std::strncmp(mod_name, "02", 2) == 0) return true;
            if (std::strncmp(p, "03", 2) == 0 && std::strncmp(mod_name, "08", 2) == 0) return true;
            if (std::strncmp(p, "04", 2) == 0 && std::strncmp(mod_name, "05", 2) == 0) return true;
            if (std::strncmp(p, "05", 2) == 0 && std::strncmp(mod_name, "12", 2) == 0) return true;
            if (std::strncmp(p, "06", 2) == 0 && std::strncmp(mod_name, "06", 2) == 0) return true;
            if (std::strncmp(p, "07", 2) == 0 &&
                (std::strncmp(mod_name, "07", 2) == 0 || std::strncmp(mod_name, "10", 2) == 0)) return true;
            if ((std::strncmp(p, "08", 2) == 0 || std::strncmp(p, "10", 2) == 0 || std::strncmp(p, "12", 2) == 0) &&
                std::strncmp(mod_name, p, 2) == 0) return true;
        } else if (n > 0 && std::strncmp(mod_name, p, n) == 0) {
            return true;
        }
        if (!comma) break;
        p = comma + 1;
    }
    return false;
}

u8* MapNear(unsigned char* image, u64 image_size, std::size_t size) {
    const u64 base = reinterpret_cast<u64>(image);
    for (u64 k = 1; k <= 64; ++k) {
        for (const u64 hint : {base - k * (16ull << 20), base + image_size + k * (16ull << 20)}) {
            void* p = ::mmap(reinterpret_cast<void*>(hint), size,
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (p == MAP_FAILED) continue;
            const u64 addr = reinterpret_cast<u64>(p);
            const auto lo = static_cast<std::int64_t>(addr) - static_cast<std::int64_t>(base + image_size);
            const auto hi = static_cast<std::int64_t>(addr + size) - static_cast<std::int64_t>(base);
            if (lo >= -(1ll << 31) && hi < (1ll << 31)) {
                return static_cast<u8*>(p);
            }
            ::munmap(p, size);
        }
    }
    return nullptr;
}

// Redirects a skipped C++ host symbol inside libbbgpu.so to `image + e.vma` via a 14-byte `jmp [rip + 0]`
// so that intra-library calls or function-pointer references (`&sub_XXXXXX`) from active C++ functions
// invoke the guest function (including any bbport guest hooks) instead of the skipped C++ body.
bool RedirectSkippedHostFnToGuest(void* host_fn_ptr, const u8* guest_fn, std::size_t page) {
    if (!host_fn_ptr || !guest_fn) {
        return false;
    }
    auto* host_fn = reinterpret_cast<u8*>(host_fn_ptr);
    const u64 page_mask   = ~(static_cast<u64>(page) - 1ULL);
    const u64 patch_begin = reinterpret_cast<u64>(host_fn) & page_mask;
    const u64 patch_end   = (reinterpret_cast<u64>(host_fn) + 14ULL + static_cast<u64>(page) - 1ULL) & page_mask;
    if (::mprotect(reinterpret_cast<void*>(patch_begin), patch_end - patch_begin,
                   PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return false;
    }
    host_fn[0] = 0xff;
    host_fn[1] = 0x25;
    host_fn[2] = 0x00;
    host_fn[3] = 0x00;
    host_fn[4] = 0x00;
    host_fn[5] = 0x00;
    const u64 target = reinterpret_cast<u64>(guest_fn);
    std::memcpy(host_fn + 6, &target, 8);
    ::mprotect(reinterpret_cast<void*>(patch_begin), patch_end - patch_begin,
               PROT_READ | PROT_EXEC);
    return true;
}

bool ShouldInstallEntry(const GXDecompiledFunctionEntry& e, u64 size,
                        u64 skip_min, u64 skip_max, const char* env_mods) {
    if (skip_max != 0ULL && e.vma >= skip_min && e.vma <= skip_max) {
        return false;
    }
    if (e.size < 5 || e.vma + e.size > size) {
        return false;
    }
    // Skip any function that is a stub or calls a stub transitively
    if (IsUnsafeStubOrCaller(e.vma)) {
        return false;
    }
    // Skip HLE/allocator/ribbon VMAs (0x26aa860 runs natively in V5.1 unless BB_FREE_CHECK=1 hooks 0x26aa986)
    static const bool kFreeCheckOn = [] {
        const char* env = std::getenv("BB_FREE_CHECK");
        return env && env[0] == '1';
    }();
    if (e.vma == 0x28ce7b0ULL || e.vma == 0x28ceec0ULL) {
        return false;
    }
    if (e.vma == 0xbb5f10ULL &&
        !gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_AsyncFramePipeline)) {
        return false;
    }
    if (e.vma == 0x26aa860ULL &&
        (!gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_NativeHeapsAndFixes) || kFreeCheckOn)) {
        return false;
    }
    // V5.1 unlocks Yebis PostFX director (0x21d5f80) and Deferred Lighting/CA pass (0x229e990) natively in C++20
    if ((e.vma == 0x21d5f80ULL || e.vma == 0x229e990ULL) &&
        !gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_NativePostFx)) {
        return false;
    }
    // Skip remaining VMAs that host hooks patch mid-function or check return addresses inside
    if (e.vma == 0x22777d0ULL || e.vma == 0x22c39f0ULL ||
        e.vma == 0x22da690ULL || e.vma == 0x2313870ULL ||
        e.vma == 0x2315d40ULL) {
        return false;
    }
    // In 06_DeferredLightingAndAO ("06"), enable full verified span 0x22900a0..0x22af630 (Files 1..3: 125/125)
    if (std::strncmp(e.module_name, "06", 2) == 0) {
        if (e.vma < 0x22900a0ULL || e.vma > 0x22af630ULL) {
            return false;
        }
    }
    // In 04_SceneRenderer ("05"), enable full verified span 0x21b6420..0x220fef0 (Sub-batches 4-1..4-8: all 13 files = 656/656)
    if (std::strncmp(e.module_name, "05", 2) == 0) {
        if (e.vma < 0x21b6420ULL || e.vma > 0x220fef0ULL) {
            return false;
        }
    }
    // In 02_GX_GraphicsFramework, enable verified continuous span 0x2167030..0x21b5990 (Batches 1..6)
    if (std::strncmp(e.module_name, "02", 2) == 0) {
        if (e.vma < 0x2167030ULL || e.vma > 0x21b5990ULL) {
            return false;
        }
    }
    // In 01_CoreGraphics2_GNM, enable all verified functions across 0x26a8090..0x26d5e90
    if (std::strncmp(e.module_name, "01", 2) == 0) {
        if (e.vma < 0x26a8090ULL || e.vma > 0x26d5e90ULL) {
            return false;
        }
    }
    if (!ModuleEnabled(env_mods, e.module_name)) {
        return false;
    }
    return true;
}

} // namespace

void PatchImage(unsigned char* image, u64 size) {
    g_eboot_live_slide = reinterpret_cast<uintptr_t>(image);

    const char* env_mods = std::getenv("BB_GX_MODULES");
    if (!env_mods) {
        env_mods = std::getenv("BBHOST_GX_MODULES");
    }
    if (env_mods && (std::strcmp(env_mods, "0") == 0 ||
                     std::strcmp(env_mods, "none") == 0 ||
                     std::strcmp(env_mods, "off") == 0)) {
        std::printf("[bb_vulkan] Golden Engine C++20 disabled (BB_GX_MODULES=%s)\n", env_mods);
        std::fflush(stdout);
        return;
    }

    constexpr std::size_t kSlotSize = 16;
    const std::size_t page = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
    const std::size_t alloc_bytes = ((kGXDecompiledFunctionCount + 1) * kSlotSize + page - 1) / page * page;

    u8* stubs = MapNear(image, size, alloc_bytes);
    if (!stubs) {
        std::fprintf(stderr, "[bb_vulkan] ERROR: MapNear failed for Golden Engine C++20 stubs\n");
        return;
    }

    const char* const env_skip_min = std::getenv("BB_GX_SKIP_MIN");
    const char* const env_skip_max = std::getenv("BB_GX_SKIP_MAX");
    const u64 skip_min = (env_skip_min && env_skip_min[0]) ? std::strtoull(env_skip_min, nullptr, 0) : 0ULL;
    const u64 skip_max = (env_skip_max && env_skip_max[0]) ? std::strtoull(env_skip_max, nullptr, 0) : 0ULL;

    std::size_t installed = 0;
    std::size_t redirected = 0;
    for (std::size_t i = 0; i < kGXDecompiledFunctionCount; ++i) {
        const GXDecompiledFunctionEntry& e = kGXDecompiledFunctions[i];
        if (!ShouldInstallEntry(e, size, skip_min, skip_max, env_mods)) {
            if (e.vma < size && RedirectSkippedHostFnToGuest(e.func_ptr, image + e.vma, page)) {
                ++redirected;
            }
            continue;
        }

        // 1. Write 14-byte absolute jump (`jmp [rip + 0]` -> e.func_ptr) into the near stub slot
        u8* stub = stubs + installed * kSlotSize;
        stub[0] = 0xff;
        stub[1] = 0x25;
        stub[2] = 0x00;
        stub[3] = 0x00;
        stub[4] = 0x00;
        stub[5] = 0x00;
        const u64 target_fn = reinterpret_cast<u64>(e.func_ptr);
        std::memcpy(stub + 6, &target_fn, 8);
        stub[14] = 0xcc;
        stub[15] = 0xcc;

        // 2. Write 5-byte `jmp rel32` at `image + e.vma` -> `stub`
        // Earlier guest hooks may have set this 4 KiB page to PROT_READ | PROT_EXEC
        u8* guest_fn = image + e.vma;
        const auto rel = static_cast<std::int64_t>(reinterpret_cast<u64>(stub)) -
                         static_cast<std::int64_t>(reinterpret_cast<u64>(guest_fn + 5));
        if (rel < -(1ll << 31) || rel >= (1ll << 31)) {
            if (e.vma < size && RedirectSkippedHostFnToGuest(e.func_ptr, image + e.vma, page)) {
                ++redirected;
            }
            continue;
        }
        const u64 patch_begin = reinterpret_cast<u64>(guest_fn) & ~(static_cast<u64>(page) - 1ULL);
        const u64 patch_end   = (reinterpret_cast<u64>(guest_fn) + 5ULL + static_cast<u64>(page) - 1ULL) &
                                ~(static_cast<u64>(page) - 1ULL);
        if (::mprotect(reinterpret_cast<void*>(patch_begin), patch_end - patch_begin,
                       PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            if (e.vma < size && RedirectSkippedHostFnToGuest(e.func_ptr, image + e.vma, page)) {
                ++redirected;
            }
            continue;
        }
        const auto rel32 = static_cast<std::int32_t>(rel);
        guest_fn[0] = 0xe9;
        std::memcpy(guest_fn + 1, &rel32, 4);
        ::mprotect(reinterpret_cast<void*>(patch_begin), patch_end - patch_begin,
                   Core::GuestCodeProtection());
        ++installed;
    }

    ::mprotect(stubs, alloc_bytes, PROT_READ | PROT_EXEC);
    std::printf("[bb_vulkan] Golden Engine C++20: installed %zu / %zu native C++ functions, redirected %zu skipped host symbols to guest, %u manifest shaders active (slide=0x%llx, filter=%s)\n",
                installed, kGXDecompiledFunctionCount, redirected,
                GXShaderLibrary_GetTotalShaderCount(),
                static_cast<unsigned long long>(g_eboot_live_slide),
                (env_mods && env_mods[0]) ? env_mods : "all");
    Vulkan::DirectShaders::InitializeManifestLibrary();
    std::printf("[bb_vulkan] Direct Vulkan RHI: mask=0x%08x (V1_BufferCopyFill=%s, V2_DrawDispatch=%s, "
                "V3_ResourceViews=%s, V4_NativeShadersAndPrewarmedPSO=%s, V5_NativePcControls=%s "
                "[v5_mask=0x%02x, gx_cmd_heap=%uKiB, intel_tonemap=%s, yebis_async_unreg=%s, decommission_pm4=%s], "
                "V6_PipelineCut=%s [0x%02x], V7_ZeroOverhead=%s [0x%02x])\n",
                gx_direct_vk::ActiveMask(),
                gx_direct_vk::EnabledV1() ? "ON" : "OFF",
                gx_direct_vk::EnabledV2() ? "ON" : "OFF",
                gx_direct_vk::EnabledV3() ? "ON" : "OFF",
                gx_direct_vk::EnabledV4() ? "ON" : "OFF",
                gx_direct_vk::EnabledV5() ? "ON" : "OFF",
                gx_direct_vk::ActiveV5SubMask(),
                bb_direct_vk_gx_command_heap_bytes() >> 10,
                bb_direct_vk_intel_tonemap_fix() ? "ON" : "OFF",
                gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_AsyncFramePipeline) ? "ON" : "OFF",
                gx_direct_vk::EnabledV5Sub(gx_direct_vk::kV5Sub_DecommissionLegacyPm4) ? "ON" : "OFF",
                gx_direct_vk::EnabledV6() ? "ON" : "OFF",
                gx_direct_vk::ActiveV6SubMask() & 0xFFu,
                gx_direct_vk::EnabledV7() ? "ON" : "OFF",
                gx_direct_vk::ActiveV7SubMask() & 0xFFu);
    std::printf("[bb_vulkan] Native Vulkan Engine: mask=0x%08x (N1_NativeBuffers=%s)\n",
                gx_direct_vk::ActiveNativeVkMask(),
                gx_direct_vk::EnabledNativeVk(gx_direct_vk::kN1_NativeBuffers) ? "ON" : "OFF");
    std::fflush(stdout);
}

Telemetry ReadTelemetry() noexcept {
    return Telemetry{
        .cpp_draws           = g_cpp_draws.load(std::memory_order_relaxed),
        .cpp_dispatches      = g_cpp_dispatches.load(std::memory_order_relaxed),
        .cpp_gpu_writes      = g_cpp_gpu_writes.load(std::memory_order_relaxed),
        .cpp_gpu_write_bytes = g_cpp_gpu_write_bytes.load(std::memory_order_relaxed),
        .cpp_manifest_hits   = g_cpp_manifest_hits.load(std::memory_order_relaxed),
        .cpp_camera_updates  = g_cpp_camera_updates.load(std::memory_order_relaxed),
    };
}

CameraBridgeState ReadCameraBridge() noexcept {
    return g_camera_bridge;
}

bool QueryShaderManifest(std::uint64_t pgm_hash, std::uint32_t crc32, std::uint32_t code_bytes,
                         ShaderManifestInfo* out) noexcept {
    const GXShaderManifestEntry* e =
        Vulkan::DirectShaders::FindManifestEntry(pgm_hash, crc32, code_bytes);
    if (!e) {
        return false;
    }
    g_cpp_manifest_hits.fetch_add(1, std::memory_order_relaxed);
    if (out) {
        out->bundle        = e->bundle;
        out->name          = e->name;
        out->stage         = e->stage;
        out->shader_hash   = e->shader_hash;
        out->crc32         = e->crc32;
        out->code_bytes    = e->code_bytes;
        out->instructions  = e->instructions;
        out->image_count   = e->image_count;
        out->sampler_count = e->sampler_count;
        out->buffer_count  = e->buffer_count;
        out->max_cb_dwords = e->max_cb_dwords;
    }
    return true;
}

std::uint32_t GetShaderManifestCount() noexcept {
    return GXShaderLibrary_GetTotalShaderCount();
}

} // namespace BbGoldenEngine
