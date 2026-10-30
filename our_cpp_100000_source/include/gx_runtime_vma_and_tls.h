// ============================================================================
// Bloodborne (CUSA03173 v1.09) — Runtime VMA Slide & Guest TLS Bridge
// File: include/gx_runtime_vma_and_tls.h
//
// Provides unified runtime resolution for:
//   1. eboot.bin VMA addresses (`GX_VMA(vma)` -> `g_eboot_live_slide + vma`,
//      where `g_eboot_live_slide = 0x400000` in live `bbhost` / `eboot_kit`).
//   2. Orbis Guest TLS base (`gx_guest_tls_base()`, reading `gs:[0x1660]` in
//      `bbhost` GS-rewrite mode) and GOT TPOFF64 slots (`0x53e4318`,
//      `0x53e4320`, `0x53e44a8`, `0x53e4568`, `0x53e4658`).
// ============================================================================
#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {
__attribute__((visibility("hidden"))) extern uintptr_t g_eboot_live_slide;
__attribute__((visibility("hidden"))) extern uint8_t   g_gx_offline_scratch[0x10000];
__attribute__((visibility("hidden"))) extern uint8_t   g_gx_offline_tls_area[0x4000];
#if !defined(GX_HOST_PROVIDES_CYG_PROFILE)
__attribute__((weak, no_instrument_function)) void __cyg_profile_func_enter(void*, void*) noexcept {}
__attribute__((weak, no_instrument_function)) void __cyg_profile_func_exit(void*, void*) noexcept {}
#endif
}

__attribute__((no_instrument_function)) static inline uintptr_t gx_resolve_vma(uintptr_t vma) noexcept {
    const uintptr_t slide = g_eboot_live_slide;
    if (__builtin_expect(slide != 0ULL, 1)) {
        // Idempotent guard: only add `slide` if `vma` is an unslid eboot.bin VMA
        if (vma >= 0x4000ULL && vma <= 0x56d4000ULL) {
            return slide + vma;
        }
        return vma;
    }
    if (vma >= reinterpret_cast<uintptr_t>(g_gx_offline_scratch) &&
        vma < reinterpret_cast<uintptr_t>(g_gx_offline_scratch + sizeof(g_gx_offline_scratch))) {
        return vma;
    }
    return reinterpret_cast<uintptr_t>(g_gx_offline_scratch + (vma & 0xfff0ULL));
}

#define GX_VMA(vma) (gx_resolve_vma(static_cast<uintptr_t>(vma)))

__attribute__((no_instrument_function)) static inline uintptr_t gx_guest_tls_base() noexcept {
    if (__builtin_expect(g_eboot_live_slide != 0ULL, 1)) {
#if defined(__x86_64__) && !defined(_WIN32)
        uintptr_t tcb = 0;
        // In bb_vulkan / bbport (g_eboot_live_slide >= 0x10000000ULL, e.g. 0x800000000ULL),
        // ARCH_SET_GS points GS directly at the guest TCB (where gs:[0x0] == tcb_self).
        if (__builtin_expect(g_eboot_live_slide >= 0x10000000ULL, 1)) {
            __asm__ volatile("movq %%gs:0x0, %0" : "=r"(tcb));
            return tcb;
        }
        // In bbhost / eboot_kit GS mode (kTebTlsSlots + 8 * 60 = 0x1660),
        // rewritten `mov rax, fs:[0]` instructions read `gs:[0x1660]`.
        __asm__ volatile("movq %%gs:0x1660, %0" : "=r"(tcb));
        if (__builtin_expect(tcb != 0ULL, 1)) {
            return tcb;
        }
        __asm__ volatile("movq %%fs:0, %0" : "=r"(tcb));
        return tcb;
#elif defined(_WIN32)
        uintptr_t tcb = 0;
        __asm__ volatile("movq %%gs:0x1480, %0" : "=r"(tcb));
        return tcb ? tcb : reinterpret_cast<uintptr_t>(g_gx_offline_tls_area + 0x2000);
#endif
    }
    return reinterpret_cast<uintptr_t>(g_gx_offline_tls_area + 0x2000);
}

__attribute__((no_instrument_function)) static inline int64_t gx_guest_tls_offset(uintptr_t tpoff_vma) noexcept {
    const uintptr_t slide = g_eboot_live_slide;
    if (__builtin_expect(slide != 0ULL, 1)) {
        const uintptr_t addr = (tpoff_vma >= 0x53e4000ULL && tpoff_vma <= 0x53e5000ULL)
            ? (slide + tpoff_vma)
            : tpoff_vma;
        return *reinterpret_cast<const int64_t*>(addr);
    }
    switch (tpoff_vma & 0xfffULL) {
        case 0x318ULL: return -1696LL; // 0x53e4318 -> -0x6a0 (alloc error mode flag)
        case 0x320ULL: return -1792LL; // 0x53e4320 -> -0x700 (alloc OOM jmp_buf / exception context)
        case 0x4a8ULL: return -1824LL; // 0x53e44a8 -> -0x720 (thread-local override heap cache)
        case 0x568ULL: return -1840LL; // 0x53e4568 -> -0x730 (thread-local linear scratch arena)
        case 0x658ULL: return -1688LL; // 0x53e4658 -> -0x698 (render thread / frame heap context)
        default:       return -1024LL;
    }
}

__attribute__((no_instrument_function)) static inline uint8_t* gx_guest_tls_slot(uintptr_t tpoff_vma) noexcept {
    return reinterpret_cast<uint8_t*>(
        static_cast<intptr_t>(gx_guest_tls_base()) + gx_guest_tls_offset(tpoff_vma));
}
