// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 04: SceneRenderer (Tier 8/10 CPU)
// File: GXFlverMaterialShaderBinderAndModelContext.cpp
// Range: 0x21c09e0 .. 0x21c4b40 (55 functions, 15,448 bytes)
// Direction: Right-to-Left (0x21c4b40 -> 0x21c09e0)
// ============================================================================
// Architectural Subsystems Reconstructed in this Translation Unit:
// 1. GXFlverMaterialTable & GXFlverMaterial:
//    - Shader (.spx / MTD) parameter binder, sampler & constant buffer layout
//    - Material flags detection (u"RNM", u"DOL", u"GXFlver_Col", u"GI", u"Emi")
//    - Debug inspector property binder (u"Mat Name", u"Spx Name", u"Disable Deferred Decal")
// 2. GXTextureFormatUtils & MipmapGenerator:
//    - DXGI_FORMAT BPP/block-size query, typeless/sRGB/UNORM conversion, BC check
//    - Hardware quad-draw mipmap chain generator (sub_21c2100)
// 3. GXGpuTimestampProfiler:
//    - 15-frame ring buffer of 8 GPU queue timestamp query pools (0x7138 B/frame)
// 4. BND4 / DCX Deflate Archive Reader:
//    - Binder4 container entry lookup, deflate stream decompression, raw copy
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <immintrin.h>

namespace Bloodborne::SceneRenderer {

namespace {

template <typename T>
inline T& MemRef(void* base, std::ptrdiff_t offset) noexcept {
    return *reinterpret_cast<T*>(static_cast<std::uint8_t*>(base) + offset);
}

template <typename T>
inline const T& MemRef(const void* base, std::ptrdiff_t offset) noexcept {
    return *reinterpret_cast<const T*>(static_cast<const std::uint8_t*>(base) + offset);
}

template <typename FnPtr>
inline FnPtr VCall(void* obj, std::ptrdiff_t vtable_offset) noexcept {
    void* vtable = *reinterpret_cast<void**>(obj);
    return *reinterpret_cast<FnPtr*>(static_cast<std::uint8_t*>(vtable) + vtable_offset);
}

inline void FreeWStringInPlace(void* str_base) noexcept {
    if (MemRef<std::uint64_t>(str_base, 0x18) >= 8) {
        void* buf = MemRef<void*>(str_base, 0x00);
        void* alloc = MemRef<void*>(str_base, 0x20);
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, buf);
    }
    MemRef<std::uint64_t>(str_base, 0x18) = 7;
    MemRef<std::uint64_t>(str_base, 0x10) = 0;
    MemRef<std::uint16_t>(str_base, 0x00) = 0;
}

inline void ReleaseRefCounted(void* obj) noexcept {
    if (!obj) {
        return;
    }
    std::int32_t prev = __atomic_fetch_sub(&MemRef<std::int32_t>(obj, 0x08), 1, __ATOMIC_ACQ_REL);
    if (prev <= 1) {
        VCall<void(*)(void*)>(obj, 0x00)(obj);
    }
}

inline void ReleaseWeakRefObject(void* obj) noexcept {
    if (!obj) {
        return;
    }
    std::uint32_t ecx = MemRef<std::uint32_t>(obj, 0x08);
    std::uint32_t edx = ecx + 0x7fffU;
    std::uint32_t esi = edx & 0x7fffU;
    std::uint32_t eax = (ecx & 0xffff8000U) | esi;
    MemRef<std::uint32_t>(obj, 0x08) = eax;
    if ((edx & 0x7fffU) == 0 && (ecx & 0x40000000U) != 0) {
        VCall<void(*)(void*)>(obj, 0x18)(obj);
        eax = MemRef<std::uint32_t>(obj, 0x08) & 0xbfffffffU;
        MemRef<std::uint32_t>(obj, 0x08) = eax;
    }
    std::uint32_t weak_ecx = (eax >> 15) + 0x7fffU;
    std::uint32_t weak_edx = (weak_ecx & 0x7fffU) << 15;
    std::uint32_t weak_esi = (eax & 0xc0007fffU) | weak_edx;
    MemRef<std::uint32_t>(obj, 0x08) = weak_esi;
    if ((weak_ecx & 0x7fffU) == 0) {
        MemRef<std::uint32_t>(obj, 0x08) = eax | 0x3fff8000U;
        VCall<void(*)(void*)>(obj, 0x20)(obj);
    }
}

} // namespace

// External engine symbols & runtime helpers
extern "C" {
#undef g_vtable_535fda0
#define g_vtable_535fda0 (reinterpret_cast<void**>(GX_VMA(0x535fda0ULL)))
#undef g_vtable_535fd70
#define g_vtable_535fd70 (reinterpret_cast<void**>(GX_VMA(0x535fd70ULL)))
#undef g_vtable_535f520
#define g_vtable_535f520 (reinterpret_cast<void**>(GX_VMA(0x535f520ULL)))
#undef g_vtable_535fcd0
#define g_vtable_535fcd0 (reinterpret_cast<void**>(GX_VMA(0x535fcd0ULL)))
#undef g_vtable_52b49e0
#define g_vtable_52b49e0 (reinterpret_cast<void**>(GX_VMA(0x52b49e0ULL)))
#undef g_vtable_53b2be0
#define g_vtable_53b2be0 (reinterpret_cast<void**>(GX_VMA(0x53b2be0ULL)))

#undef g_allocator_5540680
#define g_allocator_5540680 (*reinterpret_cast<void**>(GX_VMA(0x5540680ULL)))
#undef g_allocator_5540670
#define g_allocator_5540670 (*reinterpret_cast<void**>(GX_VMA(0x5540670ULL)))
#undef g_gx_manager_55406c8
#define g_gx_manager_55406c8 (*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))
#undef g_gx_context_55406b8
#define g_gx_context_55406b8 (*reinterpret_cast<void**>(GX_VMA(0x55406b8ULL)))
#undef g_qword_5540660
#define g_qword_5540660 (*reinterpret_cast<std::uint64_t*>(GX_VMA(0x5540660ULL)))
#undef g_byte_55406d1
#define g_byte_55406d1 (*reinterpret_cast<std::uint8_t*>(GX_VMA(0x55406d1ULL)))

#undef g_profiler_alloc_56a6ca8
#define g_profiler_alloc_56a6ca8 (*reinterpret_cast<void**>(GX_VMA(0x56a6ca8ULL)))
#undef g_profiler_array_56a6cb0
#define g_profiler_array_56a6cb0 (*reinterpret_cast<void***>(GX_VMA(0x56a6cb0ULL)))
#undef g_profiler_flag_56a6cb8
#define g_profiler_flag_56a6cb8 (*reinterpret_cast<std::uint8_t*>(GX_VMA(0x56a6cb8ULL)))
#undef g_profiler_state_56a6cb9
#define g_profiler_state_56a6cb9 (*reinterpret_cast<std::uint8_t*>(GX_VMA(0x56a6cb9ULL)))
#undef g_profiler_active_56a6cba
#define g_profiler_active_56a6cba (*reinterpret_cast<std::uint8_t*>(GX_VMA(0x56a6cbaULL)))

    #define g_tls_alloc_err_53e4318 (*reinterpret_cast<std::int32_t*>(gx_guest_tls_slot(0x53e4318ULL)))
    #define g_tls_jmpbuf_53e4320 (reinterpret_cast<std::uint8_t*>(gx_guest_tls_slot(0x53e4320ULL)))
    #define g_tls_linear_heap_53e4568 (*reinterpret_cast<void**>(gx_guest_tls_slot(0x53e4568ULL)))
    #define g_tls_render_thread_ctx_53e4658 (*reinterpret_cast<void**>(gx_guest_tls_slot(0x53e4658ULL)))

    void sub_21c4cd0(void* vec, const char16_t* mat_name, std::uint32_t param_id, const float* val4, float scalar_val);
    void sub_2bbe588(void* ptr);
    void* sub_2ba2b10();
    void sub_20b55b0(void* file, std::uint32_t line, const char* msg, ...);
    void sub_2bbe978(void* jmpbuf, std::int32_t status);
    void* sub_207b720(void* obj);
    void sub_293f750(void* ui_ctx, void* label_desc, void* wstring_prop);
    void sub_292f830(void* ui_ctx, void* label_desc, void* bool_prop);
    std::int32_t sub_2bbf3b8(const char16_t* s1, const char16_t* s2);
    void* sub_207d250();
    void sub_e4b380(void* cmd_ctx, void* alloc_desc, void* shader_state);
    void sub_e4b0c0(void* target, void* dev_ctx, std::uint32_t mask1, std::uint32_t slot, std::uint32_t mask2);
    void sub_e4b590(void* cmd_ctx);
    void sub_e4b4d0(void* cmd_ctx);

    void sub_26dc2e0(void* mtd_parser);
    bool sub_26dc7c0(void* mtd_parser, const void* data, std::uint32_t size);
    bool sub_26dcbf0(void* mtd_parser, void* out_spx_path);
    void sub_21bc980(const char16_t* path, void* out_wstring);
    void sub_26279a0(void* dst_wstring, const void* src_wstring, std::uint64_t pos, std::uint64_t count);
    std::uint32_t sub_26dd290(void* mtd_parser);
    void sub_26dd750(void* out_param, void* mtd_parser, std::uint32_t index);
    void sub_26de9e0(void* param);
    void sub_2165c10(void* dev, void* cb_obj, const void* desc, std::uint32_t a4, std::uint32_t a5, void* ctx, void* a7);
    void sub_e4ad70(void* state_obj);
    void sub_e4aed0(void* state_obj, std::uint32_t count, void* alloc, void* cb_obj, std::uint32_t flags);
    std::uint32_t sub_26dd420(void* mtd_parser);
    void sub_26dd890(void* out_sampler, void* mtd_parser, std::uint32_t index);
    void sub_26e0460(void* sampler);
    const char16_t* sub_2bbfd48(const char16_t* haystack, const char16_t* needle);
    std::size_t sub_2bbf228(const char16_t* str);
    void sub_26dc2f0(void* mtd_parser);
    void sub_2192660(void* buffer_res);
    void sub_262a720(void* wstring_obj, const char16_t* str, std::size_t len);

    void sub_22958d0(void* inst, void* alloc);
    bool sub_22979b0(void* inst, const void* a2, void* a3);
    bool sub_21915a0(void* mgr, void* res);
    std::uint64_t sub_2194580(void* a1, void* a2, void* a3);

    void* sub_26db050(void* mgr);
    void sub_2191c30(void* mgr, void* ctx, std::uint32_t mode, std::uint32_t flags);
    void sub_2168880(void* dev_ctx, void* dst_tex, void* src_tex, std::uint32_t dst_mip,
                     std::uint32_t a5, std::uint32_t a6, std::int32_t a7, void* src_surf,
                     std::uint32_t a9, std::uint64_t a10);
    void* sub_e31ed0(void* pool, std::uint32_t width, std::uint32_t height, std::uint32_t format, std::uint32_t flags);
    void sub_e46f40(void* rt_state, void* surf, std::uint32_t idx, std::uint32_t count);
    void* sub_e47840(void* rt_state);
    void sub_21a1c50(void* render_ctx);
    void sub_219fcf0(void* render_ctx);
    void sub_e47290(void* rt_state, std::uint32_t count);

    void sub_26a9690(void* query_obj);
    std::int32_t sub_216a2f0(void* dev_ctx, void* a2, void* query_obj, void* out_timestamp,
                             std::uint32_t a5, std::uint32_t a6);
    std::uint8_t sub_1089c30(void* a1);
    void sub_216a070(void* dev_ctx, void* a2, void* query_obj);
    std::uint64_t sub_2bbfe78();
    void sub_2bbe728(void* timeval_out, void* tz);
    void sub_2169ec0(void* dev_ctx, void* a2, void* query_obj);
    void sub_2bbfe68(std::uint32_t ms);

    void sub_20f1c20(void* stream_state);
    std::int32_t sub_261a1b0(void* dst, const void* src, std::size_t size);
    std::int32_t sub_20f3680(void* bnd_header, std::uint32_t entry_id, void* out_entry_state);
    void sub_20f1920(void* out_entry_state, const void* bnd_data, std::uint32_t index);
    std::uint32_t sub_20f6650(void* bnd_header);
    void sub_21c08b0(void* reader);
    std::int32_t sub_20f1fe0(void* bnd_header, const void* data, std::uint32_t a3, std::uint32_t a4, std::uint32_t a5);
    void* sub_20625a0(const char* config_str, void* alloc);
}

// Forward declarations within this range
extern "C" {
    void sub_21c4730(void* vec, std::uint64_t new_cap);
    void sub_21c4630(void* vec, std::uint64_t new_cap);
    void sub_21c4550(void* vec, const void* elem_ptr);
    void sub_21c3290(void* self);
    void sub_21c1f80(void* frame_slot);
    void sub_21c1ea0(void* queue_slot, void* dev_wrapper, std::uint64_t* out_accum);
    void sub_21c1440(void* frame_slot);
    std::int64_t sub_21c0d30(void* self, void* dst_buf, std::uint64_t max_size);
}

// ============================================================================
// 1. 0x21c4b40 (9 B) - sub_21c4b40
// Thunk forwarding [rdi + 0x08] to sub_21c4cd0
// ============================================================================
extern "C" void sub_21c4b40(void* self, const char16_t* mat_name, std::uint32_t param_id, const float* val4, float scalar_val) {
    sub_21c4cd0(MemRef<void*>(self, 0x08), mat_name, param_id, val4, scalar_val);
}

// ============================================================================
// 2. 0x21c4a30 (272 B) - sub_21c4a30
// Non-deleting destructor for GXFlverMaterialTable container
// ============================================================================
extern "C" void sub_21c4a30(void* self) {
    MemRef<void*>(self, 0x00) = g_vtable_535fda0;
    void* inner = MemRef<void*>(self, 0x08);
    if (!inner) {
        return;
    }

    void* outer_alloc = g_allocator_5540680;
    std::uint8_t* begin = MemRef<std::uint8_t*>(inner, 0x08);
    if (begin) {
        std::uint8_t* end = MemRef<std::uint8_t*>(inner, 0x10);
        for (std::uint8_t* cur = begin; cur != end; cur += 0x90) {
            FreeWStringInPlace(cur + 0x58);
            FreeWStringInPlace(cur + 0x20);
        }
        begin = MemRef<std::uint8_t*>(inner, 0x08);
        void* vec_alloc = MemRef<void*>(inner, 0x20);
        VCall<void(*)(void*, void*)>(vec_alloc, 0x70)(vec_alloc, begin);
        MemRef<std::uint64_t>(inner, 0x18) = 0;
        MemRef<std::uint64_t>(inner, 0x10) = 0;
        MemRef<std::uint64_t>(inner, 0x08) = 0;
    }

    VCall<void(*)(void*, void*)>(outer_alloc, 0x70)(outer_alloc, inner);
    MemRef<void*>(self, 0x08) = nullptr;
}

// ============================================================================
// 3. 0x21c4920 (271 B) - sub_21c4920
// Deleting destructor for GXFlverMaterialTable container
// ============================================================================
extern "C" void sub_21c4920(void* self) {
    MemRef<void*>(self, 0x00) = g_vtable_535fda0;
    void* inner = MemRef<void*>(self, 0x08);
    if (inner) {
        void* outer_alloc = g_allocator_5540680;
        std::uint8_t* begin = MemRef<std::uint8_t*>(inner, 0x08);
        if (begin) {
            std::uint8_t* end = MemRef<std::uint8_t*>(inner, 0x10);
            for (std::uint8_t* cur = begin; cur != end; cur += 0x90) {
                FreeWStringInPlace(cur + 0x58);
                FreeWStringInPlace(cur + 0x20);
            }
            begin = MemRef<std::uint8_t*>(inner, 0x08);
            void* vec_alloc = MemRef<void*>(inner, 0x20);
            VCall<void(*)(void*, void*)>(vec_alloc, 0x70)(vec_alloc, begin);
            MemRef<std::uint64_t>(inner, 0x18) = 0;
            MemRef<std::uint64_t>(inner, 0x10) = 0;
            MemRef<std::uint64_t>(inner, 0x08) = 0;
        }
        VCall<void(*)(void*, void*)>(outer_alloc, 0x70)(outer_alloc, inner);
    }
    sub_2bbe588(self);
}

// ============================================================================
// 4. 0x21c4830 (225 B) - sub_21c4830
// Constructor for GXFlverMaterialTable container
// ============================================================================
extern "C" void sub_21c4830(void* self) {
    MemRef<void*>(self, 0x00) = g_vtable_535fda0;
    MemRef<void*>(self, 0x08) = nullptr;

    void* outer_alloc = g_allocator_5540680;
    void* inner = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(outer_alloc, 0x58)(outer_alloc, 0x28, 0x08);
    if (inner) {
        void* vec_alloc = g_allocator_5540670;
        if (!vec_alloc) {
            vec_alloc = sub_2ba2b10();
            g_allocator_5540670 = vec_alloc;
        }
        std::uint8_t flags[16] = {0x20};
        if (vec_alloc && *reinterpret_cast<void**>(vec_alloc) && VCall<void*(*)(void*, void*, std::uint32_t)>(vec_alloc, 0x20)) {
            VCall<void(*)(void*, void*, std::uint32_t)>(vec_alloc, 0x20)(flags, vec_alloc, 0);
        }
        if ((flags[0] & 0x20) == 0) {
            sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }
        MemRef<std::uint64_t>(inner, 0x18) = 0;
        MemRef<std::uint64_t>(inner, 0x10) = 0;
        MemRef<std::uint64_t>(inner, 0x08) = 0;
        MemRef<void*>(inner, 0x20) = vec_alloc;
    }
    MemRef<void*>(self, 0x08) = inner;
}

// ============================================================================
// 5. 0x21c4730 (251 B) - sub_21c4730
// Reallocate pointer vector buffer (variant A) to new_cap elements
// ============================================================================
extern "C" void sub_21c4730(void* vec, std::uint64_t new_cap) {
    void* alloc = MemRef<void*>(vec, 0x20);
    std::uint64_t* new_buf = static_cast<std::uint64_t*>(
        VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(alloc, 0x58)(alloc, new_cap * 8, 8));
    if (!new_buf) {
        if (g_tls_alloc_err_53e4318 != 0) {
            sub_2bbe978(g_tls_jmpbuf_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    std::uint64_t* old_begin = MemRef<std::uint64_t*>(vec, 0x08);
    std::uint64_t* old_end = MemRef<std::uint64_t*>(vec, 0x10);
    std::ptrdiff_t count = old_end - old_begin;
    if (new_buf && count > 0) {
        std::memcpy(new_buf, old_begin, static_cast<std::size_t>(count) * sizeof(std::uint64_t));
    }
    if (old_begin) {
        void* free_alloc = MemRef<void*>(vec, 0x20);
        VCall<void(*)(void*, void*)>(free_alloc, 0x70)(free_alloc, old_begin);
    }
    MemRef<std::uint64_t*>(vec, 0x18) = new_buf + new_cap;
    MemRef<std::uint64_t*>(vec, 0x10) = new_buf + count;
    MemRef<std::uint64_t*>(vec, 0x08) = new_buf;
}

// ============================================================================
// 6. 0x21c4630 (251 B) - sub_21c4630
// Reallocate pointer vector buffer (variant B) to new_cap elements
// ============================================================================
extern "C" void sub_21c4630(void* vec, std::uint64_t new_cap) {
    void* alloc = MemRef<void*>(vec, 0x20);
    std::uint64_t* new_buf = static_cast<std::uint64_t*>(
        VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(alloc, 0x58)(alloc, new_cap * 8, 8));
    if (!new_buf) {
        if (g_tls_alloc_err_53e4318 != 0) {
            sub_2bbe978(g_tls_jmpbuf_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    }

    std::uint64_t* old_begin = MemRef<std::uint64_t*>(vec, 0x08);
    std::uint64_t* old_end = MemRef<std::uint64_t*>(vec, 0x10);
    std::ptrdiff_t count = old_end - old_begin;
    if (new_buf && count > 0) {
        std::memcpy(new_buf, old_begin, static_cast<std::size_t>(count) * sizeof(std::uint64_t));
    }
    if (old_begin) {
        void* free_alloc = MemRef<void*>(vec, 0x20);
        VCall<void(*)(void*, void*)>(free_alloc, 0x70)(free_alloc, old_begin);
    }
    MemRef<std::uint64_t*>(vec, 0x18) = new_buf + new_cap;
    MemRef<std::uint64_t*>(vec, 0x10) = new_buf + count;
    MemRef<std::uint64_t*>(vec, 0x08) = new_buf;
}

// ============================================================================
// 7. 0x21c4550 (218 B) - sub_21c4550
// push_back pointer into allocator-backed vector (handles aliasing inside [begin, end))
// ============================================================================
extern "C" void sub_21c4550(void* vec, const void* elem_ptr) {
    const auto* val_ptr = static_cast<const std::uint64_t*>(elem_ptr);
    std::uint64_t* end = MemRef<std::uint64_t*>(vec, 0x10);
    std::uint64_t* begin = MemRef<std::uint64_t*>(vec, 0x08);

    if (val_ptr < end && begin <= val_ptr) {
        std::ptrdiff_t idx = val_ptr - begin;
        if (end == MemRef<std::uint64_t*>(vec, 0x18)) {
            std::uint64_t cur_len = static_cast<std::uint64_t>(end - begin);
            std::uint64_t half = cur_len >> 1;
            std::uint64_t max_cap = 0x1fffffffffffffffULL;
            std::uint64_t grown = (max_cap - half >= cur_len) ? (cur_len + half) : 0ULL;
            std::uint64_t min_cap = cur_len + 1;
            if (grown < min_cap) {
                grown = min_cap;
            }
            sub_21c4630(vec, grown);
            end = MemRef<std::uint64_t*>(vec, 0x10);
        }
        if (end) {
            *end = MemRef<std::uint64_t*>(vec, 0x08)[idx];
        }
    } else {
        if (end == MemRef<std::uint64_t*>(vec, 0x18)) {
            std::uint64_t cur_len = static_cast<std::uint64_t>(end - MemRef<std::uint64_t*>(vec, 0x08));
            std::uint64_t half = cur_len >> 1;
            std::uint64_t max_cap = 0x1fffffffffffffffULL;
            std::uint64_t grown = (max_cap - half >= cur_len) ? (cur_len + half) : 0ULL;
            std::uint64_t min_cap = cur_len + 1;
            if (grown < min_cap) {
                grown = min_cap;
            }
            sub_21c4630(vec, grown);
            end = MemRef<std::uint64_t*>(vec, 0x10);
        }
        if (end) {
            *end = *val_ptr;
        }
    }
    MemRef<std::uint64_t*>(vec, 0x10) += 1;
}

// ============================================================================
// 8. 0x21c4510 (59 B) - sub_21c4510
// Destroy object via vtable+0x08 and free its memory via its owner allocator
// ============================================================================
extern "C" void sub_21c4510(void* obj) {
    if (!obj) {
        return;
    }
    void* alloc = sub_207b720(obj);
    VCall<void(*)(void*)>(obj, 0x08)(obj);
    VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, obj);
}

// ============================================================================
// 9. 0x21c4380 (254 B) - sub_21c4380
// Register GXFlverMaterial properties in debug inspector UI
// ============================================================================
extern "C" void sub_21c4380(void* self, void* ui_ctx) {
    alignas(16) struct {
        std::uint64_t pad0 = 0;
        const char16_t* label = nullptr;
    } desc;

    desc.pad0 = 0;
    desc.label = reinterpret_cast<const char16_t*>(GX_VMA(0x49c38f2ULL));
    sub_293f750(ui_ctx, &desc, static_cast<std::uint8_t*>(self) + 0x18);
    desc.label = nullptr;

    desc.pad0 = 0;
    desc.label = reinterpret_cast<const char16_t*>(GX_VMA(0x49c3904ULL));
    sub_293f750(ui_ctx, &desc, static_cast<std::uint8_t*>(self) + 0x50);
    desc.label = nullptr;

    void** it = MemRef<void**>(self, 0xb8);
    void** end = MemRef<void**>(self, 0xc0);
    for (; it != end; ++it) {
        void* param = *it;
        VCall<void(*)(void*, void*, void*)>(param, 0x88)(param, ui_ctx, self);
    }

    alignas(16) struct {
        std::uint64_t pad0 = 0;
        const char16_t* label = nullptr;
    } bool_desc;
    bool_desc.pad0 = 0;
    bool_desc.label = reinterpret_cast<const char16_t*>(GX_VMA(0x49c3916ULL));
    sub_292f830(ui_ctx, &bool_desc, static_cast<std::uint8_t*>(self) + 0x90);
    bool_desc.label = nullptr;
}

// ============================================================================
// 10. 0x21c4310 (99 B) - sub_21c4310
// Compute maximum float parameter value across [0xb8..0xc0) and store at +0x94
// ============================================================================
extern "C" void sub_21c4310(void* self) {
    float max_val = 0.0f;
    void** it = MemRef<void**>(self, 0xb8);
    for (; it != MemRef<void**>(self, 0xc0); ++it) {
        void* param = *it;
        float val = VCall<float(*)(void*)>(param, 0x80)(param);
        if (val > max_val) {
            max_val = val;
        }
    }
    MemRef<float>(self, 0x94) = max_val;
}

// ============================================================================
// 11. 0x21c42c0 (79 B) - sub_21c42c0
// Compute maximum uint32 parameter value across [0xb8..0xc0) and store at +0x8c
// ============================================================================
extern "C" void sub_21c42c0(void* self) {
    std::uint32_t max_val = 0;
    void** it = MemRef<void**>(self, 0xb8);
    for (; it != MemRef<void**>(self, 0xc0); ++it) {
        void* param = *it;
        std::uint32_t val = VCall<std::uint32_t(*)(void*)>(param, 0x38)(param);
        if (max_val <= val) {
            max_val = val;
        }
    }
    MemRef<std::uint32_t>(self, 0x8c) = max_val;
}

// ============================================================================
// 12. 0x21c4260 (84 B) - sub_21c4260
// Find material parameter by wide-string name in [0xb8..0xc0)
// ============================================================================
extern "C" void* sub_21c4260(void* self, const char16_t* name) {
    void** it = MemRef<void**>(self, 0xb8);
    while (it != MemRef<void**>(self, 0xc0)) {
        void* param = *it;
        void* name_holder = MemRef<void*>(param, 0x08);
        const char16_t* param_name = MemRef<const char16_t*>(name_holder, 0x00);
        ++it;
        if (sub_2bbf3b8(param_name, name) == 0) {
            return *(it - 1);
        }
    }
    return nullptr;
}

// ============================================================================
// 13. 0x21c4250 (7 B) - sub_21c4250
// Return material byte flag at +0x148
// ============================================================================
extern "C" std::uint8_t sub_21c4250(const void* self) {
    return MemRef<std::uint8_t>(self, 0x148);
}

// ============================================================================
// 14. 0x21c4200 (71 B) - sub_21c4200
// Return true if any parameter in [0xb8..0xc0) returns non-zero from vtable+0x40
// ============================================================================
extern "C" bool sub_21c4200(void* self) {
    void** it = MemRef<void**>(self, 0xb8);
    while (it != MemRef<void**>(self, 0xc0)) {
        void* param = *it;
        std::uint8_t res = VCall<std::uint8_t(*)(void*)>(param, 0x40)(param);
        ++it;
        if (res != 0) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// 15. 0x21c41a0 (82 B) - sub_21c41a0
// Reverse-iterate parameters in [0xb8..0xc0) and invoke vtable+0x20
// ============================================================================
extern "C" void sub_21c41a0(void* self, void* arg2, void* arg3) {
    void** begin = MemRef<void**>(self, 0xb8);
    void** cur = MemRef<void**>(self, 0xc0);
    while (cur != begin) {
        void* param = *(cur - 1);
        --cur;
        VCall<void(*)(void*, void*, void*)>(param, 0x20)(param, arg2, arg3);
    }
}

// ============================================================================
// 16. 0x21c3f60 (566 B) - sub_21c3f60
// Bind GXFlverMaterial parameters & constant buffers to command context
// ============================================================================
extern "C" void sub_21c3f60(void* self, void* dev_wrapper, void* draw_ctx) {
    void* list_holder = MemRef<void*>(draw_ctx, 0x08);
    bool has_instances = false;
    if (list_holder) {
        void* vec_ptr = MemRef<void*>(list_holder, 0x00);
        if (vec_ptr) {
            std::uint64_t byte_diff = MemRef<std::uint64_t>(vec_ptr, 0x10) - MemRef<std::uint64_t>(vec_ptr, 0x08);
            std::uint32_t count = static_cast<std::uint32_t>(byte_diff >> 4) * 0x38e38e39U;
            has_instances = (count != 0);
        }
    }

    if (has_instances) {
        void* vtbl_base = reinterpret_cast<std::uint8_t*>(g_vtable_52b49e0) + 0x10;
        void* linear_heap = g_tls_linear_heap_53e4568;
        if (!linear_heap) {
            sub_207d250();
            linear_heap = g_tls_linear_heap_53e4568;
        }
        std::uint64_t saved_marker = MemRef<std::uint64_t>(linear_heap, 0x10);

        struct {
            void* vtbl;
            std::uint64_t marker;
        } alloc_desc{vtbl_base, saved_marker};

        alignas(8) std::uint8_t local_cb_state[0x30]{};
        sub_e4b380(local_cb_state, &alloc_desc, MemRef<void*>(self, 0x128));

        for (void** it = MemRef<void**>(self, 0xb8); it != MemRef<void**>(self, 0xc0); ++it) {
            void* param = *it;
            VCall<void(*)(void*, void*, void*)>(param, 0x18)(param, dev_wrapper, draw_ctx);
            VCall<void(*)(void*, void*, void*)>(param, 0x10)(param, local_cb_state, draw_ctx);
        }

        void* dev_ctx = VCall<void*(*)(void*)>(dev_wrapper, 0x10)(dev_wrapper);
        sub_e4b0c0(local_cb_state, dev_ctx, 0xffffffffU, MemRef<std::uint32_t>(self, 0x138), 0xffffffffU);
        sub_e4b590(local_cb_state);
        sub_e4b4d0(local_cb_state);

        alloc_desc.vtbl = vtbl_base;
        void* heap_after = g_tls_linear_heap_53e4568;
        if (!heap_after) {
            sub_207d250();
            heap_after = g_tls_linear_heap_53e4568;
        }
        if (MemRef<std::uint64_t>(heap_after, 0x00) > saved_marker ||
            MemRef<std::uint64_t>(heap_after, 0x08) < saved_marker) {
            sub_20b55b0(reinterpret_cast<void*>(GX_VMA(0x493c62dULL)),
                        0x233,
                        reinterpret_cast<const char*>(GX_VMA(0x493c690ULL)));
        }
        if (MemRef<std::uint64_t>(heap_after, 0x10) > saved_marker) {
            MemRef<std::uint64_t>(heap_after, 0x10) = saved_marker;
        }
    } else {
        for (void** it = MemRef<void**>(self, 0xb8); it != MemRef<void**>(self, 0xc0); ++it) {
            void* param = *it;
            VCall<void(*)(void*, void*, void*)>(param, 0x18)(param, dev_wrapper, draw_ctx);
            VCall<void(*)(void*, void*, void*)>(param, 0x10)(param, MemRef<void*>(self, 0x128), draw_ctx);
        }
        void* state_obj = MemRef<void*>(self, 0x128);
        void* dev_ctx = VCall<void*(*)(void*)>(dev_wrapper, 0x10)(dev_wrapper);
        sub_e4b0c0(state_obj, dev_ctx, 0xffffffffU, MemRef<std::uint32_t>(self, 0x138), 0xffffffffU);
    }
}

// ============================================================================
// 17. 0x21c36a0 (2239 B) - sub_21c36a0
// Parse MTD/SPX material definition, build parameters/samplers/CBs, classify shader flags
// ============================================================================
extern "C" bool sub_21c36a0(void* self, const void* mtd_data, std::uint32_t mtd_size, void* binder_factory) {
    alignas(8) std::uint8_t mtd_parser[0x38]{};
    sub_26dc2e0(mtd_parser);

    bool ok = false;
    if (sub_26dc7c0(mtd_parser, mtd_data, mtd_size)) {
        VCall<void(*)(void*)>(binder_factory, 0x30)(binder_factory);

        void* tls_heap = g_tls_linear_heap_53e4568;
        if (!tls_heap) {
            sub_207d250();
            tls_heap = g_tls_linear_heap_53e4568;
        }
        void* str_alloc1 = static_cast<std::uint8_t*>(tls_heap) + 0x28;
        std::uint8_t flags1[16] = {0x20};
        if (str_alloc1 && *reinterpret_cast<void**>(str_alloc1) && VCall<void*(*)(void*, void*, std::uint32_t)>(str_alloc1, 0x20)) {
            VCall<void(*)(void*, void*, std::uint32_t)>(str_alloc1, 0x20)(flags1, str_alloc1, 0);
        }
        if ((flags1[0] & 0x20) == 0) {
            sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }

        struct WString {
            void* vtbl;
            union {
                char16_t inline_buf[8];
                char16_t* ptr;
            };
            std::uint64_t len;
            std::uint64_t cap;
            void* alloc;
            std::uint8_t owning;
            std::uint8_t pad[7];
        };

        WString spx_raw{};
        spx_raw.inline_buf[0] = 0;
        spx_raw.len = 0;
        spx_raw.cap = 7;
        spx_raw.alloc = str_alloc1;
        spx_raw.owning = 1;

        if (sub_26dcbf0(mtd_parser, &spx_raw)) {
            void* tls_heap2 = g_tls_linear_heap_53e4568;
            if (!tls_heap2) {
                sub_207d250();
                tls_heap2 = g_tls_linear_heap_53e4568;
            }
            void* str_alloc2 = static_cast<std::uint8_t*>(tls_heap2) + 0x28;
            std::uint8_t flags2[16] = {0x20};
            if (str_alloc2 && *reinterpret_cast<void**>(str_alloc2) && VCall<void*(*)(void*, void*, std::uint32_t)>(str_alloc2, 0x20)) {
                VCall<void(*)(void*, void*, std::uint32_t)>(str_alloc2, 0x20)(flags2, str_alloc2, 0);
            }
            if ((flags2[0] & 0x20) == 0) {
                sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }

            WString spx_stem{};
            spx_stem.inline_buf[0] = 0;
            spx_stem.len = 0;
            spx_stem.cap = 7;
            spx_stem.alloc = str_alloc2;
            spx_stem.owning = 1;

            const char16_t* raw_cstr = (spx_raw.cap >= 8) ? spx_raw.ptr : spx_raw.inline_buf;
            sub_21bc980(raw_cstr, &spx_stem);

            void* self_spx_str = static_cast<std::uint8_t*>(self) + 0x50;
            MemRef<std::uint8_t>(self, 0x80) = spx_stem.owning;
            if (self_spx_str != static_cast<void*>(&spx_stem)) {
                void* dst_alloc = MemRef<void*>(self, 0x78);
                VCall<void(*)(void*)>(dst_alloc, 0x10)(dst_alloc);
                VCall<void(*)(void*)>(spx_stem.alloc, 0x10)(spx_stem.alloc);
                sub_26279a0(self_spx_str, &spx_stem, 0, 0xffffffffffffffffULL);
            }

            const char16_t* self_spx_cstr = (MemRef<std::uint64_t>(self, 0x70) >= 8)
                ? MemRef<const char16_t*>(self, 0x58)
                : reinterpret_cast<const char16_t*>(static_cast<std::uint8_t*>(self) + 0x58);
            VCall<void(*)(void*, const char16_t*)>(binder_factory, 0x38)(binder_factory, self_spx_cstr);

            if (spx_stem.cap >= 8) {
                VCall<void(*)(void*, void*)>(spx_stem.alloc, 0x70)(spx_stem.alloc, spx_stem.ptr);
            }
        }

        std::uint32_t param_count = sub_26dd290(mtd_parser);
        if (param_count != 0) {
            void* vec_active = static_cast<std::uint8_t*>(self) + 0xb0;
            void* vec_inactive = static_cast<std::uint8_t*>(self) + 0xd8;
            void* vec_aux = static_cast<std::uint8_t*>(self) + 0x100;
            for (std::uint32_t i = 0; i < param_count; ++i) {
                alignas(8) std::uint8_t param_desc[0x18]{};
                sub_26dd750(param_desc, mtd_parser, i);
                void* aux_out = nullptr;
                void* param_obj = VCall<void*(*)(void*, void*, void*, void*, void**)>(binder_factory, 0x00)(
                    binder_factory, param_desc, mtd_parser, MemRef<void*>(self, 0x10), &aux_out);
                if (param_obj) {
                    bool is_inactive = VCall<bool(*)(void*)>(param_obj, 0x30)(param_obj);
                    sub_21c4550(is_inactive ? vec_inactive : vec_active, &param_obj);
                    if (aux_out) {
                        void* aux_val = aux_out;
                        void** end = MemRef<void**>(self, 0x110);
                        void** begin = MemRef<void**>(self, 0x108);
                        if (&aux_val < end && begin <= &aux_val) {
                            std::ptrdiff_t idx = &aux_val - begin;
                            if (end == MemRef<void**>(self, 0x118)) {
                                std::uint64_t cur_len = static_cast<std::uint64_t>(end - begin);
                                std::uint64_t half = cur_len >> 1;
                                std::uint64_t grown = (0x1fffffffffffffffULL - half >= cur_len) ? (cur_len + half) : 0ULL;
                                if (grown < cur_len + 1) grown = cur_len + 1;
                                sub_21c4730(vec_aux, grown);
                                end = MemRef<void**>(self, 0x110);
                            }
                            if (end) {
                                *end = MemRef<void**>(self, 0x108)[idx];
                            }
                        } else {
                            if (end == MemRef<void**>(self, 0x118)) {
                                std::uint64_t cur_len = static_cast<std::uint64_t>(end - MemRef<void**>(self, 0x108));
                                std::uint64_t half = cur_len >> 1;
                                std::uint64_t grown = (0x1fffffffffffffffULL - half >= cur_len) ? (cur_len + half) : 0ULL;
                                if (grown < cur_len + 1) grown = cur_len + 1;
                                sub_21c4730(vec_aux, grown);
                                end = MemRef<void**>(self, 0x110);
                            }
                            if (end) {
                                *end = aux_val;
                            }
                        }
                        MemRef<void**>(self, 0x110) += 1;
                    }
                }
                sub_26de9e0(param_desc);
            }
        }

        std::uint32_t reg_count = VCall<std::uint32_t(*)(void*, std::uint32_t)>(binder_factory, 0x10)(binder_factory, 0) + 1;
        void* self_alloc = MemRef<void*>(self, 0x10);
        void* cb_obj = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(self_alloc, 0x58)(self_alloc, 0x10, 0x08);
        if (cb_obj) {
            MemRef<std::uint64_t>(cb_obj, 0x08) = 0;
            MemRef<std::uint64_t>(cb_obj, 0x00) = 0;
        }
        MemRef<void*>(self, 0x130) = cb_obj;
        if (!cb_obj) {
            sub_26dc2f0(mtd_parser);
            return false;
        }

        std::uint32_t byte_stride = reg_count << 4;
        std::uint32_t cb_size = byte_stride;
        if (byte_stride >= 0x10001U) {
            cb_size = 0x10000U - (0x10000U % byte_stride);
            if (byte_stride < cb_size) {
                cb_size = byte_stride;
            }
        }
        MemRef<std::uint32_t>(cb_obj, 0x08) = cb_size;
        MemRef<std::uint32_t>(cb_obj, 0x0c) = byte_stride;

        if (cb_size != 0) {
            struct alignas(16) CBDesc {
                std::uint32_t size;
                std::uint32_t pad1;
                std::uint32_t flags;
                std::uint32_t pad2;
                std::uint32_t pad3;
                std::uint32_t pad4;
                std::uint32_t pad5;
                std::uint32_t pad6;
            } cb_desc{};
            cb_desc.size = cb_size;
            cb_desc.flags = 4;
            void* gx_dev = MemRef<void*>(g_gx_manager_55406c8, 0x00);
            sub_2165c10(gx_dev, cb_obj, &cb_desc, 0, 0, g_gx_context_55406b8, nullptr);
        }

        void* state_alloc = MemRef<void*>(self, 0x10);
        void* state_obj = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(state_alloc, 0x58)(state_alloc, 0x28, 0x08);
        if (state_obj) {
            sub_e4ad70(state_obj);
        }
        MemRef<void*>(self, 0x128) = state_obj;
        sub_e4aed0(state_obj, reg_count, MemRef<void*>(self, 0x10), MemRef<void*>(self, 0x130), 0);

        MemRef<std::uint32_t>(self, 0x138) = VCall<std::uint32_t(*)(void*, std::uint32_t)>(binder_factory, 0x18)(binder_factory, 0);
        MemRef<std::uint32_t>(self, 0x140) = VCall<std::uint32_t(*)(void*, std::uint32_t)>(binder_factory, 0x10)(binder_factory, 1);
        std::int32_t slot1_check = VCall<std::int32_t(*)(void*, std::uint32_t)>(binder_factory, 0x10)(binder_factory, 1);
        if (slot1_check >= 0) {
            MemRef<std::uint32_t>(self, 0x13c) = VCall<std::uint32_t(*)(void*, std::uint32_t)>(binder_factory, 0x18)(binder_factory, 1);
        } else {
            MemRef<std::uint32_t>(self, 0x13c) = 0xffffffffU;
        }
        MemRef<std::uint32_t>(self, 0x144) = VCall<std::uint32_t(*)(void*)>(binder_factory, 0x20)(binder_factory);

        void* mat_info = VCall<void*(*)(void*)>(binder_factory, 0x28)(binder_factory);
        MemRef<std::uint16_t>(self, 0xa8) = MemRef<std::uint16_t>(mat_info, 0x20);
        MemRef<std::uint64_t>(self, 0xa0) = MemRef<std::uint64_t>(mat_info, 0x18);
        MemRef<std::uint64_t>(self, 0x98) = MemRef<std::uint64_t>(mat_info, 0x10);
        MemRef<std::uint64_t>(self, 0x90) = MemRef<std::uint64_t>(mat_info, 0x08);
        MemRef<std::uint64_t>(self, 0x88) = MemRef<std::uint64_t>(mat_info, 0x00);

        std::uint32_t sampler_count = sub_26dd420(mtd_parser);
        if (sampler_count != 0) {
            void* vec_active = static_cast<std::uint8_t*>(self) + 0xb0;
            for (std::uint32_t i = 0; i < sampler_count; ++i) {
                alignas(8) std::uint8_t sampler_desc[0x20]{};
                sub_26dd890(sampler_desc, mtd_parser, i);
                void* sampler_param = VCall<void*(*)(void*, void*, void*, void*)>(binder_factory, 0x08)(
                    binder_factory, sampler_desc, mtd_parser, MemRef<void*>(self, 0x10));
                if (sampler_param) {
                    sub_21c4550(vec_active, &sampler_param);
                }
                sub_26e0460(sampler_desc);
            }
        }

        const auto get_spx_cstr = [self]() -> const char16_t* {
            return (MemRef<std::uint64_t>(self, 0x70) >= 8)
                ? MemRef<const char16_t*>(self, 0x58)
                : reinterpret_cast<const char16_t*>(static_cast<std::uint8_t*>(self) + 0x58);
        };

        if (sub_2bbfd48(get_spx_cstr(), reinterpret_cast<const char16_t*>(GX_VMA(0x49c38bcULL))) != nullptr) {
            MemRef<std::uint8_t>(self, 0x148) = MemRef<std::uint8_t>(self, 0xa4);
            MemRef<std::uint8_t>(self, 0x98) = 1;
        } else if (sub_2bbfd48(get_spx_cstr(), reinterpret_cast<const char16_t*>(GX_VMA(0x49c38c4ULL))) != nullptr) {
            MemRef<std::uint8_t>(self, 0xa2) = 1;
            MemRef<std::uint8_t>(self, 0x148) = MemRef<std::uint8_t>(self, 0xa4);
            MemRef<std::uint8_t>(self, 0x98) = 1;
        } else {
            MemRef<std::uint8_t>(self, 0x148) = MemRef<std::uint8_t>(self, 0xa4);
            std::size_t spx_len = MemRef<std::uint64_t>(self, 0x68);
            std::size_t col_len = sub_2bbf228(reinterpret_cast<const char16_t*>(GX_VMA(0x49c38ccULL)));
            if (spx_len <= col_len && sub_2bbfd48(get_spx_cstr(), reinterpret_cast<const char16_t*>(GX_VMA(0x49c38ccULL))) != nullptr) {
                MemRef<std::uint8_t>(self, 0x148) = 1;
                MemRef<std::uint8_t>(self, 0xa4) = 1;
                MemRef<std::uint8_t>(self, 0x98) = 1;
            } else {
                const char16_t* gi_hit = sub_2bbfd48(get_spx_cstr(), reinterpret_cast<const char16_t*>(GX_VMA(0x49c38e4ULL)));
                if (gi_hit != nullptr && gi_hit[2] == 0) {
                    MemRef<std::uint8_t>(self, 0x98) = 1;
                } else if (sub_2bbfd48(get_spx_cstr(), reinterpret_cast<const char16_t*>(GX_VMA(0x49c38eaULL))) != nullptr) {
                    MemRef<std::uint8_t>(self, 0xa7) = 1;
                }
            }
        }

        for (void** it = MemRef<void**>(self, 0xb8); it != MemRef<void**>(self, 0xc0); ++it) {
            void* param = *it;
            VCall<void(*)(void*, void*)>(param, 0x68)(param, self);
            VCall<void(*)(void*, void*)>(param, 0x78)(param, self);
        }

        if (spx_raw.cap >= 8) {
            VCall<void(*)(void*, void*)>(spx_raw.alloc, 0x70)(spx_raw.alloc, spx_raw.ptr);
        }
        ok = true;
    }

    sub_26dc2f0(mtd_parser);
    return ok;
}

// ============================================================================
// 18. 0x21c3290 (1037 B) - sub_21c3290
// Non-deleting destructor for GXFlverMaterial
// ============================================================================
extern "C" void sub_21c3290(void* self) {
    MemRef<void*>(self, 0x00) = g_vtable_535fd70;

    void* ref_obj = MemRef<void*>(self, 0x150);
    if (ref_obj) {
        VCall<void(*)(void*)>(ref_obj, 0x18)(ref_obj);
        MemRef<std::uint8_t>(ref_obj, 0x0b) &= 0xbfU;
        void* ref_obj2 = MemRef<void*>(self, 0x150);
        if (ref_obj2) {
            ReleaseWeakRefObject(ref_obj2);
            MemRef<void*>(self, 0x150) = nullptr;
        }
    }

    void* state_obj = MemRef<void*>(self, 0x128);
    if (state_obj) {
        void* alloc = MemRef<void*>(self, 0x10);
        VCall<void(*)(void*)>(state_obj, 0x00)(state_obj);
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, state_obj);
        MemRef<void*>(self, 0x128) = nullptr;
    }

    void* cb_obj = MemRef<void*>(self, 0x130);
    if (cb_obj) {
        void* alloc = MemRef<void*>(self, 0x10);
        void* res = MemRef<void*>(cb_obj, 0x00);
        if (res) {
            sub_2192660(res);
            MemRef<void*>(cb_obj, 0x00) = nullptr;
        }
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, cb_obj);
        MemRef<void*>(self, 0x130) = nullptr;
    }

    for (void** it = MemRef<void**>(self, 0xb8); it != MemRef<void**>(self, 0xc0); ++it) {
        void* param = *it;
        if (param) {
            void* alloc = sub_207b720(param);
            VCall<void(*)(void*)>(param, 0x00)(param);
            VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, param);
        }
    }
    MemRef<void**>(self, 0xc0) = MemRef<void**>(self, 0xb8);

    for (void** it = MemRef<void**>(self, 0xe0); it != MemRef<void**>(self, 0xe8); ++it) {
        void* param = *it;
        if (param) {
            void* alloc = sub_207b720(param);
            VCall<void(*)(void*)>(param, 0x00)(param);
            VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, param);
        }
    }
    MemRef<void**>(self, 0xe8) = MemRef<void**>(self, 0xe0);

    for (void** it = MemRef<void**>(self, 0x108); it != MemRef<void**>(self, 0x110); ++it) {
        void* aux = *it;
        if (aux) {
            void* alloc = sub_207b720(aux);
            VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, aux);
        }
    }
    MemRef<void**>(self, 0x110) = MemRef<void**>(self, 0x108);

    const char16_t* empty_wstr = reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL));
    sub_262a720(static_cast<std::uint8_t*>(self) + 0x18, empty_wstr, 0);
    sub_262a720(static_cast<std::uint8_t*>(self) + 0x50, empty_wstr, 0);

    void* tail_ref = MemRef<void*>(self, 0x150);
    if (tail_ref) {
        ReleaseWeakRefObject(tail_ref);
        MemRef<void*>(self, 0x150) = nullptr;
    }

    if (void* buf3 = MemRef<void*>(self, 0x108)) {
        void* alloc = MemRef<void*>(self, 0x120);
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, buf3);
        MemRef<std::uint64_t>(self, 0x118) = 0;
        MemRef<std::uint64_t>(self, 0x110) = 0;
        MemRef<std::uint64_t>(self, 0x108) = 0;
    }

    if (void* buf2 = MemRef<void*>(self, 0xe0)) {
        void* alloc = MemRef<void*>(self, 0xf8);
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, buf2);
        MemRef<std::uint64_t>(self, 0xf0) = 0;
        MemRef<std::uint64_t>(self, 0xe8) = 0;
        MemRef<std::uint64_t>(self, 0xe0) = 0;
    }

    if (void* buf1 = MemRef<void*>(self, 0xb8)) {
        void* alloc = MemRef<void*>(self, 0xd0);
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, buf1);
        MemRef<std::uint64_t>(self, 0xc8) = 0;
        MemRef<std::uint64_t>(self, 0xc0) = 0;
        MemRef<std::uint64_t>(self, 0xb8) = 0;
    }

    FreeWStringInPlace(static_cast<std::uint8_t*>(self) + 0x58);
    FreeWStringInPlace(static_cast<std::uint8_t*>(self) + 0x20);
}

// ============================================================================
// 19. 0x21c3270 (28 B) - sub_21c3270
// Deleting destructor for GXFlverMaterial
// ============================================================================
extern "C" void sub_21c3270(void* self) {
    sub_21c3290(self);
    sub_2bbe588(self);
}

// ============================================================================
// 20. 0x21c2fe0 (643 B) - sub_21c2fe0
// Constructor for GXFlverMaterial
// ============================================================================
extern "C" void sub_21c2fe0(void* self, void* alloc) {
    MemRef<std::uint32_t>(self, 0x08) = 0;
    MemRef<void*>(self, 0x00) = g_vtable_535fd70;
    MemRef<void*>(self, 0x10) = alloc;

    const auto check_alloc = [](void* a) {
        std::uint8_t flags[16] = {0x20};
        if (a && *reinterpret_cast<void**>(a) && VCall<void*(*)(void*, void*, std::uint32_t)>(a, 0x20)) {
            VCall<void(*)(void*, void*, std::uint32_t)>(a, 0x20)(flags, a, 0);
        }
        if ((flags[0] & 0x20) == 0) {
            sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }
    };

    check_alloc(alloc);
    MemRef<void*>(self, 0x40) = alloc;
    MemRef<std::uint64_t>(self, 0x38) = 7;
    MemRef<std::uint64_t>(self, 0x30) = 0;
    MemRef<std::uint16_t>(self, 0x20) = 0;
    MemRef<std::uint8_t>(self, 0x48) = 1;

    check_alloc(alloc);
    MemRef<void*>(self, 0x78) = alloc;
    MemRef<std::uint64_t>(self, 0x70) = 7;
    MemRef<std::uint64_t>(self, 0x68) = 0;
    MemRef<std::uint16_t>(self, 0x58) = 0;
    MemRef<std::uint8_t>(self, 0x80) = 1;

    MemRef<std::uint32_t>(self, 0x88) = 0;
    MemRef<std::uint32_t>(self, 0x8c) = 0;
    MemRef<std::uint32_t>(self, 0x94) = 0x40600000U; // 3.5f
    MemRef<std::uint16_t>(self, 0x90) = 0;
    MemRef<std::uint16_t>(self, 0xa8) = 0;
    MemRef<std::uint64_t>(self, 0xa0) = 0;
    MemRef<std::uint64_t>(self, 0x98) = 0;

    check_alloc(alloc);
    MemRef<std::uint64_t>(self, 0xc8) = 0;
    MemRef<std::uint64_t>(self, 0xc0) = 0;
    MemRef<std::uint64_t>(self, 0xb8) = 0;
    MemRef<void*>(self, 0xd0) = alloc;

    check_alloc(alloc);
    MemRef<std::uint64_t>(self, 0xf0) = 0;
    MemRef<std::uint64_t>(self, 0xe8) = 0;
    MemRef<std::uint64_t>(self, 0xe0) = 0;
    MemRef<void*>(self, 0xf8) = alloc;

    void* global_alloc = g_allocator_5540670;
    if (!global_alloc) {
        global_alloc = sub_2ba2b10();
        g_allocator_5540670 = global_alloc;
    }
    check_alloc(global_alloc);
    MemRef<std::uint64_t>(self, 0x118) = 0;
    MemRef<std::uint64_t>(self, 0x110) = 0;
    MemRef<std::uint64_t>(self, 0x108) = 0;
    MemRef<void*>(self, 0x120) = global_alloc;

    MemRef<std::uint64_t>(self, 0x138) = 0;
    MemRef<std::uint64_t>(self, 0x130) = 0;
    MemRef<std::uint64_t>(self, 0x128) = 0;
    MemRef<std::uint32_t>(self, 0x140) = 0xffffffffU;
    MemRef<std::uint32_t>(self, 0x144) = 0;
    MemRef<std::uint8_t>(self, 0x148) = 0;
    MemRef<std::uint64_t>(self, 0x150) = 0;
}

// ============================================================================
// 21. 0x21c2ef0 (91 B) - sub_21c2ef0
// Allocate and initialize 0x7b20-byte GXModelContext object
// ============================================================================
extern "C" bool sub_21c2ef0(const void* arg1, void* arg2) {
    void* alloc = g_allocator_5540680;
    void* obj = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(alloc, 0x58)(alloc, 0x7b20, 0x10);
    if (!obj) {
        return false;
    }
    sub_22958d0(obj, g_allocator_5540680);
    return sub_22979b0(obj, arg1, arg2);
}

// ============================================================================
// 22. 0x21c2eb0 (51 B) - sub_21c2eb0
// Register resource with GX manager +0x388 if not disabled
// ============================================================================
extern "C" bool sub_21c2eb0(void* res) {
    void* mgr = g_gx_manager_55406c8;
    if (!mgr) {
        return false;
    }
    void* sub_mgr = MemRef<void*>(mgr, 0x388);
    if (MemRef<std::uint8_t>(sub_mgr, 0x08) != 0) {
        return true;
    }
    return sub_21915a0(sub_mgr, res);
}

// ============================================================================
// 23. 0x21c2ea0 (5 B) - sub_21c2ea0
// Tail-call thunk to sub_2194580
// ============================================================================
extern "C" std::uint64_t sub_21c2ea0(void* a1, void* a2, void* a3) {
    return sub_2194580(a1, a2, a3);
}

// ============================================================================
// 24. 0x21c2d60 (176 B) - sub_21c2d60
// Return bits-per-pixel / block byte size for DXGI_FORMAT enum
// ============================================================================
extern "C" std::uint32_t sub_21c2d60(std::uint32_t fmt) {
    std::uint32_t c1 = fmt - 0x46U;
    if (c1 <= 0x1dU) {
        if ((0x3f007000U >> c1) & 1U) {
            return 8U;
        }
        if ((0x00000e07U >> c1) & 1U) {
            return 4U;
        }
        if ((0x000001f8U >> c1) & 1U) {
            return 8U;
        }
    }
    if ((fmt - 0x3cU) < 6U) {
        return 8U;
    }
    if ((fmt - 0x30U) < 12U) {
        return 16U;
    }
    if ((fmt - 0x17U) < 25U) {
        return 32U;
    }
    if ((fmt - 0x09U) < 14U) {
        return 64U;
    }
    if ((fmt - 0x05U) < 4U) {
        return 96U;
    }
    if ((fmt - 0x01U) < 4U) {
        return 128U;
    }
    if (fmt == 0x43U) {
        return 32U;
    }
    std::uint32_t c2 = fmt - 0x44U;
    if (c2 <= 0x12U && ((0x00060003U >> c2) & 1U)) {
        return 16U;
    }
    return ((fmt - 0x57U) < 7U) ? 32U : 0U;
}

// ============================================================================
// 25. 0x21c2d20 (53 B) - sub_21c2d20
// Convert depth DXGI_FORMAT to corresponding depth-stencil view format
// ============================================================================
extern "C" std::uint32_t sub_21c2d20(std::int32_t fmt) {
    if (fmt <= 0x26) {
        if (fmt == 0x15) {
            return 0x14U;
        }
        return static_cast<std::uint32_t>(fmt);
    }
    if (fmt == 0x35) {
        return 0x37U;
    }
    if (fmt == 0x2c) {
        return 0x2dU;
    }
    if (fmt == 0x27) {
        return 0x28U;
    }
    return static_cast<std::uint32_t>(fmt);
}

// ============================================================================
// 26. 0x21c2af0 (552 B) - sub_21c2af0
// Map typed DXGI_FORMAT (2..99) to its corresponding _TYPELESS base format
// ============================================================================
extern "C" std::uint32_t sub_21c2af0(std::uint32_t fmt) {
    switch (fmt) {
        case 2: case 3: case 4:
            return 1U;
        case 6: case 7: case 8:
            return 5U;
        case 10: case 11: case 12: case 13: case 14:
            return 9U;
        case 16: case 17: case 18:
            return 15U;
        case 20: case 21: case 22:
            return 19U;
        case 24: case 25:
            return 23U;
        case 28: case 29: case 30: case 31: case 32:
            return 27U;
        case 34: case 35: case 36: case 37: case 38:
            return 33U;
        case 40: case 41: case 42: case 43:
            return 39U;
        case 45: case 46: case 47:
            return 44U;
        case 49: case 50: case 51: case 52:
            return 48U;
        case 54: case 55: case 56: case 57: case 58: case 59:
            return 53U;
        case 61: case 62: case 63: case 64:
            return 60U;
        case 71: case 72:
            return 70U;
        case 74: case 75:
            return 73U;
        case 77: case 78:
            return 76U;
        case 80: case 81:
            return 79U;
        case 83: case 84:
            return 82U;
        case 87: case 91:
            return 90U;
        case 88: case 93:
            return 92U;
        case 95: case 96:
            return 94U;
        case 98: case 99:
            return 97U;
        default:
            return fmt;
    }
}

// ============================================================================
// 27. 0x21c2a80 (107 B) - sub_21c2a80
// Return true if DXGI_FORMAT is an _SRGB format
// ============================================================================
extern "C" bool sub_21c2a80(std::int32_t fmt) {
    std::int32_t non_srgb = fmt;
    if (fmt <= 0x47) {
        if (fmt == 0x1d) {
            non_srgb = 0x1c;
        }
    } else if (fmt <= 0x62) {
        if (fmt <= 0x5a) {
            if (fmt == 0x48) {
                non_srgb = 0x47;
            } else if (fmt == 0x4b) {
                non_srgb = 0x4a;
            } else if (fmt == 0x4e) {
                non_srgb = 0x4d;
            }
        } else if (fmt == 0x5b) {
            non_srgb = 0x57;
        } else if (fmt == 0x5d) {
            non_srgb = 0x58;
        }
    } else if (fmt == 0x63) {
        non_srgb = 0x62;
    }
    return non_srgb != fmt;
}

// ============================================================================
// 28. 0x21c2a60 (20 B) - sub_21c2a60
// Return true if DXGI_FORMAT is block-compressed (BC1..BC5 or BC6H..BC7)
// ============================================================================
extern "C" bool sub_21c2a60(std::uint32_t fmt) {
    if ((fmt - 0x46U) < 0x0fU) {
        return true;
    }
    return (fmt - 0x5eU) < 0x06U;
}

// ============================================================================
// 29. 0x21c29d0 (139 B) - sub_21c29d0
// Return true if texture resource wrapper uses an _SRGB DXGI_FORMAT
// ============================================================================
extern "C" bool sub_21c29d0(void* tex_wrapper) {
    if (!tex_wrapper) {
        return false;
    }
    void* desc = VCall<void*(*)(void*)>(tex_wrapper, 0x18)(tex_wrapper);
    if (!desc) {
        return false;
    }
    std::int32_t fmt = MemRef<std::int32_t>(desc, 0x30);
    return sub_21c2a80(fmt);
}

// ============================================================================
// 30. 0x21c29b0 (27 B) - sub_21c29b0
// Compute full mipmap chain level count for (width, height)
// ============================================================================
extern "C" std::uint32_t sub_21c29b0(std::uint32_t width, std::uint32_t height) {
    std::uint32_t mips = 0;
    if (height == 0 || width == 0) {
        return 0;
    }
    while (true) {
        ++mips;
        height >>= 1;
        if (height == 0) {
            break;
        }
        width >>= 1;
        if (width == 0) {
            break;
        }
    }
    return mips;
}

// ============================================================================
// 31. 0x21c2100 (2214 B) - sub_21c2100
// Hardware mipmap chain generator using full-screen quad draw calls
// ============================================================================
extern "C" bool sub_21c2100(void* self, void* src_tex, void* dst_tex) {
    if (!dst_tex) {
        return false;
    }

    void* render_ctx = MemRef<void*>(self, 0x00);
    struct {
        void* vtbl;
        void* owner;
        void* shader_pass;
    } draw_helper{g_vtable_535f520, self, nullptr};

    void* gx_mgr = g_gx_manager_55406c8;
    if (MemRef<std::uint8_t>(render_ctx, 0x2093) != 0) {
        draw_helper.shader_pass = sub_26db050(MemRef<void*>(gx_mgr, 0x390));
        gx_mgr = g_gx_manager_55406c8;
    } else {
        std::uint32_t idx = MemRef<std::uint32_t>(render_ctx, 0x2098);
        void** table = MemRef<void**>(gx_mgr, 0x3a0);
        draw_helper.shader_pass = table[idx];
    }

    void* rt_pool = MemRef<void*>(gx_mgr, 0x260);
    sub_2191c30(MemRef<void*>(gx_mgr, 0x388), render_ctx, 2, 0);

    void* state_mgr = MemRef<void*>(g_gx_manager_55406c8, 0x250);
    void* sampler_state = MemRef<void*>(state_mgr, 0x2e0);
    MemRef<void*>(render_ctx, 0x9e0) = sampler_state;
    MemRef<std::uint32_t>(render_ctx, 0xa60) = 0;
    if (MemRef<std::uint32_t>(render_ctx, 0xa64) <= 1U) {
        MemRef<std::uint32_t>(render_ctx, 0xa64) = 1U;
    }
    void* cmd_buf = MemRef<void*>(render_ctx, 0x08);
    MemRef<std::uint64_t>(cmd_buf, 0x1978) |= 1ULL;
    MemRef<void*>(cmd_buf, 0xd0a8) = sampler_state;

    state_mgr = MemRef<void*>(g_gx_manager_55406c8, 0x250);
    void* blend_state = MemRef<void*>(state_mgr, 0x00);
    MemRef<void*>(render_ctx, 0x480) = blend_state;
    MemRef<std::uint32_t>(render_ctx, 0x498) = 0xffffffffU;
    cmd_buf = MemRef<void*>(render_ctx, 0x08);
    MemRef<void*>(cmd_buf, 0xbc00) = blend_state;
    MemRef<std::uint64_t>(cmd_buf, 0xbca0) = 0;
    MemRef<std::uint64_t>(cmd_buf, 0xbc98) = 0;
    MemRef<std::uint32_t>(cmd_buf, 0xbcb0) = 0xffffffffU;

    state_mgr = MemRef<void*>(g_gx_manager_55406c8, 0x250);
    void* depth_state = MemRef<void*>(state_mgr, 0x1c8);
    MemRef<void*>(render_ctx, 0x210) = depth_state;
    MemRef<std::uint8_t>(render_ctx, 0x218) = 0xffU;
    cmd_buf = MemRef<void*>(render_ctx, 0x08);
    MemRef<void*>(cmd_buf, 0xbc08) = depth_state;
    MemRef<std::uint8_t>(cmd_buf, 0xbcb4) = 0xffU;

    state_mgr = MemRef<void*>(g_gx_manager_55406c8, 0x250);
    void* rast_state = MemRef<void*>(state_mgr, 0x1f8);
    MemRef<void*>(render_ctx, 0x258) = rast_state;
    MemRef<void*>(cmd_buf, 0xb970) = rast_state;

    void* query_tex = src_tex ? src_tex : dst_tex;
    std::uint32_t width = VCall<std::uint32_t(*)(void*, std::uint32_t)>(query_tex, 0x48)(query_tex, 0);
    std::uint32_t height = VCall<std::uint32_t(*)(void*, std::uint32_t)>(query_tex, 0x50)(query_tex, 0);
    std::uint32_t mip_levels = VCall<std::uint32_t(*)(void*)>(query_tex, 0x40)(query_tex);
    if (mip_levels == 0) {
        mip_levels = sub_21c29b0(width, height);
    }

    void* fmt_holder = MemRef<void*>(query_tex, 0x48);
    std::uint32_t format = MemRef<std::uint32_t>(fmt_holder, 0x30);

    MemRef<std::int32_t>(dst_tex, 0x08) += 1;
    if (src_tex) {
        void* src_res = VCall<void*(*)(void*)>(src_tex, 0x28)(src_tex);
        void* dst_res = VCall<void*(*)(void*)>(dst_tex, 0x28)(dst_tex);
        sub_2168880(MemRef<void*>(render_ctx, 0x08), nullptr, src_res, 0, 0, 0, 0, dst_res, 0, 0);
    }

    // 4 vertices * 0x30 (48) bytes = 48 floats (pos.xyzw, color.rgba = 1,1,1,1, uv.xy, pad.zw = 0,0)
    // Matches eboot.bin 0x21c2525..0x21c25d7 (.rodata 0x4920b50..0x4920b80)
    static constexpr float kQuadVertices[48] = {
        -1.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 0.0f, 0.0f,
    };

    void* cur_src = dst_tex;
    // When src_tex == nullptr (SSR glossy mip-pyramid in sub_22b1530), Deferred Light PBR PS
    // (ps_3fe9e964) clamps the SSR reflection mip LOD to min(5.0, 6.5 - gloss * 6.5), so mips 6..11
    // (40x22 down to 1x1) are never sampled. Capping at 6 levels (mips 0..5, down to 80x45),
    // copying temp_rt into dst_tex mips 1..4 via EmitCopyTextureRegion (vkCmdCopyImage), and
    // rendering mip 5 directly from mip 4's temp_rt into mip_surfs[5] reduces the SSR mip-pyramid
    // from 22 RenderPasses down to 5 RenderPasses/frame.
    const std::uint32_t loop_mip_levels =
        (src_tex == nullptr && mip_levels > 6U) ? 6U : mip_levels;
    if (loop_mip_levels >= 2U) {
        void* rt_state = static_cast<std::uint8_t*>(render_ctx) + 0x1728;
        void* temp_rt = nullptr;
        const std::uint32_t dst_mip_cap =
            (src_tex == nullptr && MemRef<void*>(dst_tex, 0x60) != nullptr)
                ? VCall<std::uint32_t(*)(void*)>(dst_tex, 0x40)(dst_tex)
                : 0U;

        for (std::uint32_t mip = 1U; mip != loop_mip_levels; ++mip) {
            width >>= 1;
            if (width == 0) width = 1U;
            height >>= 1;
            if (height == 0) height = 1U;

            if (src_tex == nullptr && mip > 1U && (mip + 1U) == loop_mip_levels &&
                dst_mip_cap > mip && cur_src != dst_tex) {
                void** mip_surfs = MemRef<void**>(dst_tex, 0x60);
                sub_e46f40(rt_state, mip_surfs[mip], 0, 1);
                MemRef<void*>(render_ctx, 0x1ec0) = sub_e47840(rt_state);
                sub_21a1c50(render_ctx);

                void* srv_obj = MemRef<void*>(cur_src, 0x48);
                MemRef<void*>(render_ctx, 0x788) = srv_obj;
                MemRef<std::uint32_t>(render_ctx, 0x960) = 0;
                if (MemRef<std::uint32_t>(render_ctx, 0x964) <= 1U) {
                    MemRef<std::uint32_t>(render_ctx, 0x964) = 1U;
                }
                void* cb = MemRef<void*>(render_ctx, 0x08);
                MemRef<std::uint64_t>(cb, 0x1968) |= 1ULL;
                MemRef<void*>(cb, 0xd128) = srv_obj;

                alignas(16) float quad_verts[48];
                std::memcpy(quad_verts, kQuadVertices, sizeof(quad_verts));
                VCall<void(*)(void*, std::uint32_t, std::uint32_t, const void*, std::uint32_t)>(&draw_helper, 0x00)(
                    &draw_helper, 5, 4, quad_verts, 0x30);

                MemRef<void*>(render_ctx, 0x788) = nullptr;
                MemRef<std::uint32_t>(render_ctx, 0x960) = 0;
                if (MemRef<std::uint32_t>(render_ctx, 0x964) <= 1U) {
                    MemRef<std::uint32_t>(render_ctx, 0x964) = 1U;
                }
                cb = MemRef<void*>(render_ctx, 0x08);
                MemRef<std::uint64_t>(cb, 0x1968) |= 1ULL;
                MemRef<void*>(cb, 0xd128) = nullptr;

                sub_219fcf0(render_ctx);
                sub_e47290(rt_state, 1);
                MemRef<void*>(render_ctx, 0x1ec0) = sub_e47840(rt_state);
                MemRef<std::uint8_t>(render_ctx, 0x1670) &= 0xfeU;
                break;
            }

            void* new_rt = sub_e31ed0(rt_pool, width, height, format, 0);
            if (temp_rt != new_rt) {
                ReleaseRefCounted(temp_rt);
                temp_rt = nullptr;
                if (new_rt) {
                    MemRef<std::int32_t>(new_rt, 0x08) += 1;
                    temp_rt = new_rt;
                }
            }

            void* rt_surf = nullptr;
            if (MemRef<void*>(temp_rt, 0x40) != nullptr && MemRef<void*>(temp_rt, 0x60) != nullptr) {
                if (VCall<std::uint32_t(*)(void*)>(temp_rt, 0x40)(temp_rt) != 0) {
                    void** surf_arr = MemRef<void**>(temp_rt, 0x60);
                    rt_surf = surf_arr[0];
                }
            }

            sub_e46f40(rt_state, rt_surf, 0, 1);
            MemRef<void*>(render_ctx, 0x1ec0) = sub_e47840(rt_state);
            sub_21a1c50(render_ctx);

            void* srv_obj = MemRef<void*>(cur_src, 0x48);
            MemRef<void*>(render_ctx, 0x788) = srv_obj;
            MemRef<std::uint32_t>(render_ctx, 0x960) = 0;
            if (MemRef<std::uint32_t>(render_ctx, 0x964) <= 1U) {
                MemRef<std::uint32_t>(render_ctx, 0x964) = 1U;
            }
            void* cb = MemRef<void*>(render_ctx, 0x08);
            MemRef<std::uint64_t>(cb, 0x1968) |= 1ULL;
            MemRef<void*>(cb, 0xd128) = srv_obj;

            alignas(16) float quad_verts[48];
            std::memcpy(quad_verts, kQuadVertices, sizeof(quad_verts));
            VCall<void(*)(void*, std::uint32_t, std::uint32_t, const void*, std::uint32_t)>(&draw_helper, 0x00)(
                &draw_helper, 5, 4, quad_verts, 0x30);

            MemRef<void*>(render_ctx, 0x788) = nullptr;
            MemRef<std::uint32_t>(render_ctx, 0x960) = 0;
            if (MemRef<std::uint32_t>(render_ctx, 0x964) <= 1U) {
                MemRef<std::uint32_t>(render_ctx, 0x964) = 1U;
            }
            cb = MemRef<void*>(render_ctx, 0x08);
            MemRef<std::uint64_t>(cb, 0x1968) |= 1ULL;
            MemRef<void*>(cb, 0xd128) = nullptr;

            sub_219fcf0(render_ctx);
            sub_e47290(rt_state, 1);
            MemRef<void*>(render_ctx, 0x1ec0) = sub_e47840(rt_state);
            MemRef<std::uint8_t>(render_ctx, 0x1670) &= 0xfeU;

            if (src_tex) {
                void* dst_res = VCall<void*(*)(void*)>(src_tex, 0x28)(src_tex);
                void* tmp_res = VCall<void*(*)(void*)>(temp_rt, 0x28)(temp_rt);
                std::uint32_t target_mip = (mip_levels != 0) ? mip : 0U;
                sub_2168880(MemRef<void*>(render_ctx, 0x08), nullptr, dst_res, target_mip, 0, 0, 0, tmp_res, 0, 0);
            } else if (dst_mip_cap > mip) {
                void* dst_res = VCall<void*(*)(void*)>(dst_tex, 0x28)(dst_tex);
                void* tmp_res = VCall<void*(*)(void*)>(temp_rt, 0x28)(temp_rt);
                sub_2168880(MemRef<void*>(render_ctx, 0x08), nullptr, dst_res, mip, 0, 0, 0, tmp_res, 0, 0);
            }

            if (cur_src != temp_rt) {
                ReleaseRefCounted(cur_src);
                cur_src = nullptr;
                if (temp_rt) {
                    MemRef<std::int32_t>(temp_rt, 0x08) += 1;
                    cur_src = temp_rt;
                }
            }
        }

        ReleaseRefCounted(temp_rt);
    }

    ReleaseRefCounted(cur_src);
    return true;
}

// ============================================================================
// 31b. 0xe5b220 (176 B) - sub_e5b220
// Enqueues half/quarter-res depth pyramid downsample pass (ps_e820ee9d).
// When SSR is enabled (owning GXSceneRenderer flag +0x2974, i.e. self + 0x47c != 0),
// stage 0x0c (0x22a3808) has already downsampled scene depth into self (+0x24f8),
// so the duplicate call at stage 0x1b (0x22ab09c) is skipped, saving 2 RenderPasses/frame.
// ============================================================================
extern "C" void sub_e5b220(void* self, void* cmd_list, std::uint32_t stage_id,
                           std::uint32_t sub_id, void* src_depth) {
    if (MemRef<std::uint8_t>(self, 0xb8) == 0) {
        return;
    }
    if (stage_id == 0x1bU && MemRef<std::uint8_t>(self, 0x47c) != 0) {
        return;
    }
    auto fn_alloc = reinterpret_cast<void*(*)(void*, std::uint32_t, std::uint32_t)>(GX_VMA(0x2289db0ULL));
    auto fn_begin = reinterpret_cast<void(*)(void*, std::uint32_t, std::uint32_t)>(GX_VMA(0x228e430ULL));
    auto fn_push  = reinterpret_cast<void(*)(void*, void*, const void*)>(GX_VMA(0x228e6a0ULL));
    auto fn_end   = reinterpret_cast<void(*)(void*)>(GX_VMA(0x228e3c0ULL));

    void** slot = static_cast<void**>(fn_alloc(MemRef<void*>(cmd_list, 0x450), 8U, 8U));
    *slot = src_depth;
    fn_begin(cmd_list, stage_id, sub_id);
    struct {
        void* ptr;
        std::uint64_t zero;
    } pkt{slot, 0ULL};
    fn_push(cmd_list, self, &pkt);
    fn_end(cmd_list);
}

// ============================================================================
// 32. 0x21c1f80 (227 B) - sub_21c1f80
// Destroy all 8 GPU timestamp query pools in a single profiler frame slot
// ============================================================================
extern "C" void sub_21c1f80(void* frame_slot) {
    sub_21c1440(frame_slot);
}

// ============================================================================
// 33. 0x21c1ea0 (213 B) - sub_21c1ea0
// Accumulate GPU timestamp deltas for a single queue slot into out_accum[0..18]
// ============================================================================
extern "C" void sub_21c1ea0(void* queue_slot, void* dev_wrapper, std::uint64_t* out_accum) {
    std::int32_t count = MemRef<std::int32_t>(queue_slot, 0xc00);
    if (count <= 0) {
        return;
    }

    std::uint8_t* entry = static_cast<std::uint8_t*>(queue_slot) + 0x10;
    for (std::int32_t i = 0; i < count; ++i, entry += 0x18) {
        std::uint32_t bucket = MemRef<std::uint32_t>(entry, -0x10);
        if (bucket <= 0x12U) {
            std::uint64_t t_start = 0;
            std::uint64_t t_end = 0;
            void* dev_ctx = MemRef<void*>(dev_wrapper, 0x08);
            std::int32_t err0 = sub_216a2f0(dev_ctx, nullptr, MemRef<void*>(entry, -0x08), &t_start, 0, 0);
            std::int32_t err1 = sub_216a2f0(MemRef<void*>(dev_wrapper, 0x08), nullptr, MemRef<void*>(entry, 0x00), &t_end, 0, 0);
            if ((err0 | err1) == 0 && t_end > t_start) {
                out_accum[bucket] += (t_end - t_start);
            }
        }
    }
}

// ============================================================================
// 34. 0x21c1e90 (13 B) - sub_21c1e90
// Latch profiler state byte 0x56a6cb9 into active flag 0x56a6cba
// ============================================================================
extern "C" void sub_21c1e90() {
    g_profiler_active_56a6cba = g_profiler_state_56a6cb9;
}

// ============================================================================
// 35. 0x21c1e60 (42 B) - sub_21c1e60
// Update profiler enable flags from main profiler manager +0x6a432
// ============================================================================
extern "C" void sub_21c1e60(void* self) {
    std::uint8_t res = sub_1089c30(self);
    g_byte_55406d1 |= res;
    g_profiler_state_56a6cb9 = MemRef<std::uint8_t>(self, 0x6a432);
}

// ============================================================================
// 36. 0x21c1da0 (178 B) - sub_21c1da0
// End current GPU timestamp scope and pop scope stack
// ============================================================================
extern "C" void sub_21c1da0(void* self, void* dev_wrapper) {
    std::uint32_t queue_idx = 0;
    std::uint32_t frame_idx = 0;
    if (MemRef<std::uint8_t>(dev_wrapper, 0x2093) != 0) {
        frame_idx = MemRef<std::uint32_t>(self, 0x00);
    } else {
        void* tls_ctx = g_tls_render_thread_ctx_53e4658;
        if (tls_ctx) {
            queue_idx = MemRef<std::uint32_t>(tls_ctx, 0xa0);
        }
        frame_idx = MemRef<std::uint32_t>(self, 0x04);
    }

    std::uint8_t* frame_base = static_cast<std::uint8_t*>(self) + static_cast<std::size_t>(frame_idx) * 0x7138ULL;
    std::uint8_t* queue_base = frame_base + static_cast<std::size_t>(queue_idx) * 0x0e10ULL;

    std::int32_t active_slot = MemRef<std::int32_t>(queue_base, 0xcc4);
    if (active_slot >= 0 && active_slot < 0x80) {
        void* end_query = MemRef<void*>(queue_base, static_cast<std::ptrdiff_t>(active_slot) * 24 + 0xd0);
        sub_216a070(MemRef<void*>(dev_wrapper, 0x08), nullptr, end_query);
    }

    std::int32_t stack_depth = MemRef<std::int32_t>(queue_base, 0xcc8);
    if (stack_depth > 0) {
        --stack_depth;
        MemRef<std::int32_t>(queue_base, 0xcc8) = stack_depth;
        MemRef<std::int32_t>(queue_base, 0xcc4) =
            MemRef<std::int32_t>(queue_base, static_cast<std::ptrdiff_t>(stack_depth) * 4 + 0xccc);
    }
}

// ============================================================================
// 37. 0x21c1cb0 (228 B) - sub_21c1cb0
// Begin new GPU timestamp scope (push active scope, issue start timestamp query)
// ============================================================================
extern "C" void sub_21c1cb0(void* self, void* dev_wrapper, std::uint32_t bucket_id) {
    std::uint32_t queue_idx = 0;
    std::uint32_t frame_idx = 0;
    if (MemRef<std::uint8_t>(dev_wrapper, 0x2093) != 0) {
        frame_idx = MemRef<std::uint32_t>(self, 0x00);
    } else {
        void* tls_ctx = g_tls_render_thread_ctx_53e4658;
        if (tls_ctx) {
            queue_idx = MemRef<std::uint32_t>(tls_ctx, 0xa0);
        }
        frame_idx = MemRef<std::uint32_t>(self, 0x04);
    }

    std::uint8_t* frame_base = static_cast<std::uint8_t*>(self) + static_cast<std::size_t>(frame_idx) * 0x7138ULL;
    std::uint8_t* queue_base = frame_base + static_cast<std::size_t>(queue_idx) * 0x0e10ULL;

    std::uint32_t next_slot = MemRef<std::uint32_t>(queue_base, 0xcc0);
    if (next_slot > 0x7fU) {
        return;
    }

    std::int32_t cur_active = MemRef<std::int32_t>(queue_base, 0xcc4);
    if (cur_active >= 0) {
        std::int32_t depth = MemRef<std::int32_t>(queue_base, 0xcc8);
        MemRef<std::int32_t>(queue_base, static_cast<std::ptrdiff_t>(depth) * 4 + 0xccc) = cur_active;
        MemRef<std::int32_t>(queue_base, 0xcc8) = depth + 1;
        next_slot = MemRef<std::uint32_t>(queue_base, 0xcc0);
    }

    MemRef<std::uint32_t>(queue_base, 0xcc4) = next_slot;
    MemRef<std::uint32_t>(queue_base, 0xcc0) = next_slot + 1U;

    std::ptrdiff_t entry_off = static_cast<std::ptrdiff_t>(next_slot) * 24;
    MemRef<std::uint32_t>(queue_base, entry_off + 0xc0) = bucket_id;
    MemRef<std::uint8_t>(queue_base, entry_off + 0xc4) = 1;
    void* start_query = MemRef<void*>(queue_base, entry_off + 0xc8);
    sub_216a070(MemRef<void*>(dev_wrapper, 0x08), nullptr, start_query);
}

// ============================================================================
// 38. 0x21c1bd0 (222 B) - sub_21c1bd0
// End frame GPU timestamp profiling and advance 15-frame ring index
// ============================================================================
extern "C" void sub_21c1bd0(void* self, void* dev_wrapper) {
    if (MemRef<std::uint8_t>(dev_wrapper, 0x2093) == 0) {
        return;
    }

    std::uint64_t tid = sub_2bbfe78();
    void* mgr = g_gx_manager_55406c8;
    void* thread_info = MemRef<void*>(MemRef<void*>(mgr, 0x348), 0x08);
    std::uint8_t enabled = (tid == MemRef<std::uint64_t>(thread_info, 0x18))
        ? g_profiler_active_56a6cba
        : g_profiler_state_56a6cb9;

    if (enabled != 0) {
        std::uint32_t frame_idx = MemRef<std::uint32_t>(self, 0x00);
        std::uint8_t* frame_base = static_cast<std::uint8_t*>(self) + static_cast<std::size_t>(frame_idx) * 0x7138ULL;
        std::int32_t active_slot = MemRef<std::int32_t>(frame_base, 0xcc4);
        if (active_slot >= 0 && active_slot < 0x80) {
            void* end_query = MemRef<void*>(frame_base, static_cast<std::ptrdiff_t>(active_slot) * 24 + 0xd0);
            sub_216a070(MemRef<void*>(dev_wrapper, 0x08), nullptr, end_query);
        }

        std::int32_t depth = MemRef<std::int32_t>(frame_base, 0xcc8);
        if (depth > 0) {
            --depth;
            MemRef<std::int32_t>(frame_base, 0xcc8) = depth;
            MemRef<std::int32_t>(frame_base, 0xcc4) =
                MemRef<std::int32_t>(frame_base, static_cast<std::ptrdiff_t>(depth) * 4 + 0xccc);
        }

        sub_216a070(MemRef<void*>(dev_wrapper, 0x08), nullptr, MemRef<void*>(frame_base, 0x08));
    }

    std::uint32_t cur = MemRef<std::uint32_t>(self, 0x00);
    MemRef<std::uint32_t>(self, 0x00) = (cur + 1U) % 15U;
}

// ============================================================================
// 39. 0x21c1b90 (57 B) - sub_21c1b90
// Query whether GPU timestamp profiler is enabled for the calling thread
// ============================================================================
extern "C" bool sub_21c1b90() {
    std::uint64_t tid = sub_2bbfe78();
    void* mgr = g_gx_manager_55406c8;
    void* thread_info = MemRef<void*>(MemRef<void*>(mgr, 0x348), 0x08);
    std::uint8_t enabled = (tid == MemRef<std::uint64_t>(thread_info, 0x18))
        ? g_profiler_active_56a6cba
        : g_profiler_state_56a6cb9;
    return enabled != 0;
}

// ============================================================================
// 40. 0x21c17b0 (977 B) - sub_21c17b0
// Begin frame GPU timestamp profiling and compute averaged millisecond stats
// ============================================================================
extern "C" void sub_21c17b0(void* self, void* dev_wrapper) {
    if (MemRef<std::uint8_t>(dev_wrapper, 0x2093) == 0) {
        return;
    }
    if (!sub_21c1b90()) {
        return;
    }

    std::uint32_t frame_idx = MemRef<std::uint32_t>(self, 0x00);
    std::uint8_t* frame_base = static_cast<std::uint8_t*>(self) + static_cast<std::size_t>(frame_idx) * 0x7138ULL;
    std::uint64_t freq = MemRef<std::uint64_t>(frame_base, 0x10);

    if ((MemRef<std::uint8_t>(frame_base, 0x18) & 1U) == 0) {
        auto* accum = reinterpret_cast<std::uint64_t*>(static_cast<std::uint8_t*>(self) + 0x6a258);
        const auto* src = reinterpret_cast<const std::uint64_t*>(frame_base + 0x28);
        for (std::size_t i = 0; i < 19; ++i) {
            accum[i] += src[i];
        }
        MemRef<std::int32_t>(self, 0x6a250) += 1;
    }

    struct {
        std::int64_t tv_sec;
        std::int64_t tv_usec;
    } tv{};
    sub_2bbe728(&tv, nullptr);
    std::uint64_t now_us = static_cast<std::uint64_t>(tv.tv_sec) * 1000000ULL +
                           static_cast<std::uint64_t>(tv.tv_usec) - g_qword_5540660;

    // Unsigned 64-bit to double conversion using magic constants at 0x4920b30 / 0x4920b40
    auto u64_to_double = [](std::uint64_t v) noexcept -> double {
        return static_cast<double>(v);
    };

    float elapsed_sec = static_cast<float>(u64_to_double(now_us) / 1000000.0);
    if ((elapsed_sec - MemRef<float>(self, 0x6a434)) >= 1.0f) {
        std::int64_t sample_count = MemRef<std::int32_t>(self, 0x6a250);
        if (sample_count > 0) {
            MemRef<std::uint64_t>(self, 0x6a438) = freq;
            const auto* accum = reinterpret_cast<const std::uint64_t*>(static_cast<const std::uint8_t*>(self) + 0x6a258);
            auto* snapshot = reinterpret_cast<std::uint64_t*>(static_cast<std::uint8_t*>(self) + 0x6a340);
            std::memcpy(snapshot, accum, 0xe8);

            unsigned __int128 prod = static_cast<unsigned __int128>(static_cast<std::uint64_t>(sample_count) * freq) *
                                     0x8888888888888889ULL;
            std::uint64_t denom_ticks = static_cast<std::uint64_t>(prod >> 69);
            double denom = u64_to_double(denom_ticks);

            auto* out_ms = reinterpret_cast<float*>(static_cast<std::uint8_t*>(self) + 0x6a3d8);
            for (std::size_t i = 0; i < 19; ++i) {
                out_ms[i] = static_cast<float>(u64_to_double(snapshot[i]) / denom);
            }

            MemRef<std::int32_t>(self, 0x6a250) = 0;
            std::memset( static_cast<std::uint8_t*>(self) + 0x6a258, 0, 0xe4);
            MemRef<float>(self, 0x6a434) = elapsed_sec;
        }
    }

    frame_idx = MemRef<std::uint32_t>(self, 0x00);
    frame_base = static_cast<std::uint8_t*>(self) + static_cast<std::size_t>(frame_idx) * 0x7138ULL;
    sub_2169ec0(MemRef<void*>(dev_wrapper, 0x08), nullptr, MemRef<void*>(frame_base, 0x08));

    std::uint32_t next_slot = MemRef<std::uint32_t>(frame_base, 0xcc0);
    if (next_slot <= 0x7fU) {
        std::int32_t cur_active = MemRef<std::int32_t>(frame_base, 0xcc4);
        if (cur_active >= 0) {
            std::int32_t depth = MemRef<std::int32_t>(frame_base, 0xcc8);
            MemRef<std::int32_t>(frame_base, static_cast<std::ptrdiff_t>(depth) * 4 + 0xccc) = cur_active;
            MemRef<std::int32_t>(frame_base, 0xcc8) = depth + 1;
            next_slot = MemRef<std::uint32_t>(frame_base, 0xcc0);
        }
        MemRef<std::uint32_t>(frame_base, 0xcc4) = next_slot;
        MemRef<std::uint32_t>(frame_base, 0xcc0) = next_slot + 1U;

        std::ptrdiff_t entry_off = static_cast<std::ptrdiff_t>(next_slot) * 24;
        MemRef<std::uint32_t>(frame_base, entry_off + 0xc0) = 0;
        MemRef<std::uint8_t>(frame_base, entry_off + 0xc4) = 1;
        sub_216a070(MemRef<void*>(dev_wrapper, 0x08), nullptr, MemRef<void*>(frame_base, entry_off + 0xc8));
    }
    MemRef<std::uint8_t>(frame_base, 0x20) = 1;
}

// ============================================================================
// 41. 0x21c1540 (611 B) - sub_21c1540
// Collect completed GPU timestamp queries for the oldest frame in the 15-frame ring
// ============================================================================
extern "C" void sub_21c1540(void* self, void* dev_wrapper) {
    std::uint32_t cur_frame = MemRef<std::uint32_t>(self, 0x00);
    std::uint32_t read_frame = (cur_frame + 1U) % 15U;
    MemRef<std::uint32_t>(self, 0x04) = read_frame;

    std::uint8_t* frame_base = static_cast<std::uint8_t*>(self) + static_cast<std::size_t>(read_frame) * 0x7138ULL;
    if (MemRef<std::uint8_t>(frame_base, 0x20) == 0) {
        return;
    }

    while (sub_216a2f0(MemRef<void*>(dev_wrapper, 0x08), nullptr, MemRef<void*>(frame_base, 0x08), nullptr, 0, 0) != 0) {
        __builtin_ia32_pause();
    }

    sub_216a2f0(MemRef<void*>(dev_wrapper, 0x08), nullptr, MemRef<void*>(frame_base, 0x08), frame_base + 0x10, 0, 0);

    auto* accum = reinterpret_cast<std::uint64_t*>(frame_base + 0x28);
    std::memset(accum, 0, 0x98);

    constexpr std::uint64_t kResetActive = 0xffffffff00000000ULL;
    for (std::size_t q = 0; q < 8; ++q) {
        std::uint8_t* q_base = frame_base + 0xc0 + q * 0x0e10ULL;
        sub_21c1ea0(q_base, dev_wrapper, accum);
        if (q < 7) {
            MemRef<std::uint64_t>(q_base, 0xc00) = kResetActive;
            MemRef<std::uint32_t>(q_base, 0xc08) = 0;
        } else {
            MemRef<std::uint32_t>(q_base, 0xc00) = 0;
            MemRef<std::uint32_t>(q_base, 0xc04) = 0xffffffffU;
            MemRef<std::uint32_t>(q_base, 0xc08) = 0;
        }
    }

    MemRef<std::uint64_t>(frame_base, 0xb8) = 0;
    std::uint64_t rem = accum[0];
    std::uint64_t last_valid = 0;
    for (std::size_t idx = 6; (idx - 5) < 0x13; ++idx) {
        last_valid = 0;
        std::uint64_t val = MemRef<std::uint64_t>(frame_base, idx * 8);
        if (rem < val) {
            break;
        }
        rem -= val;
        last_valid = rem;
    }
    MemRef<std::uint64_t>(frame_base, 0xb8) = last_valid;
    MemRef<std::uint8_t>(frame_base, 0x20) = 0;
}

// ============================================================================
// 42. 0x21c1440 (244 B) - sub_21c1440
// Release all GPU query objects across all 8 queues of a single frame slot
// ============================================================================
extern "C" void sub_21c1440(void* frame_slot) {
    std::uint8_t* q_ptr = static_cast<std::uint8_t*>(frame_slot) + 0xc8;
    for (std::uint32_t q = 0; q < 8U; ++q, q_ptr += 0x0e10) {
        std::uint8_t* entry = q_ptr;
        for (std::uint32_t i = 0; i < 0x80U; ++i, entry += 0x18) {
            if (void* q0 = MemRef<void*>(entry, -0x08)) {
                void* alloc = sub_207b720(q0);
                sub_26a9690(q0);
                VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, q0);
                MemRef<void*>(entry, -0x08) = nullptr;
            }
            if (void* q1 = MemRef<void*>(entry, 0x00)) {
                void* alloc = sub_207b720(q1);
                sub_26a9690(q1);
                VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, q1);
                MemRef<void*>(entry, 0x00) = nullptr;
            }
        }
    }

    if (void* freq_q = MemRef<void*>(frame_slot, 0x00)) {
        void* alloc = sub_207b720(freq_q);
        sub_26a9690(freq_q);
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, freq_q);
        MemRef<void*>(frame_slot, 0x00) = nullptr;
    }
}

// ============================================================================
// 43. 0x21c12b0 (396 B) - sub_21c12b0
// Destructor for 15-frame GXGpuTimestampProfiler
// ============================================================================
extern "C" void sub_21c12b0(void* self) {
    std::uint8_t* base = static_cast<std::uint8_t*>(self);
    for (std::size_t i = 0; i < 15; ++i) {
        sub_21c1440(base + 0x08 + i * 0x7138ULL);
    }
    for (std::size_t i = 15; i > 0; --i) {
        sub_21c1f80(base + 0x08 + (i - 1) * 0x7138ULL);
    }
}

// ============================================================================
// 44. 0x21c1160 (189 B) - sub_21c1160
// Shutdown global 17-entry shader parameter binder factory table
// ============================================================================
extern "C" void sub_21c1160() {
    if (!g_profiler_alloc_56a6ca8) {
        return;
    }
    if (g_profiler_flag_56a6cb8 != 0) {
        std::int32_t limit = (g_profiler_flag_56a6cb8 != 0) ? 17 : 0;
        for (std::int32_t i = 0; i < limit; ++i) {
            void* entry = g_profiler_array_56a6cb0[i];
            if (entry) {
                void* alloc = g_profiler_alloc_56a6ca8;
                VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, entry);
                g_profiler_array_56a6cb0[i] = nullptr;
                limit = (g_profiler_flag_56a6cb8 != 0) ? 17 : 0;
            }
        }
    }
    if (g_profiler_array_56a6cb0) {
        void* alloc = g_profiler_alloc_56a6ca8;
        VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, g_profiler_array_56a6cb0);
        g_profiler_array_56a6cb0 = nullptr;
    }
    g_profiler_flag_56a6cb8 = 0;
    g_profiler_alloc_56a6ca8 = nullptr;
}

// ============================================================================
// 45. 0x21c10a0 (188 B) - sub_21c10a0
// Initialize global 17-entry shader parameter binder factory table
// ============================================================================
extern "C" void sub_21c10a0(void* alloc) {
    g_profiler_alloc_56a6ca8 = alloc;
    g_profiler_flag_56a6cb8 = 1;
    g_profiler_array_56a6cb0 = static_cast<void**>(
        VCall<void*(*)(void*, std::uint64_t)>(alloc, 0x50)(alloc, 0x88));

    if (g_profiler_flag_56a6cb8 != 0) {
        void** vtbl_table = g_vtable_535fcd0;
        std::int32_t i = 0;
        std::int32_t limit = 17;
        do {
            void* cur_alloc = g_profiler_alloc_56a6ca8;
            void* obj = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(cur_alloc, 0x58)(cur_alloc, 0x18, 0x08);
            if (obj) {
                MemRef<void*>(obj, 0x00) = *vtbl_table;
                MemRef<std::uint64_t>(obj, 0x10) = 0;
                MemRef<std::uint64_t>(obj, 0x08) = 0;
            }
            g_profiler_array_56a6cb0[i] = obj;
            ++i;
            ++vtbl_table;
            limit = (g_profiler_flag_56a6cb8 != 0) ? 17 : 0;
        } while (i < limit);
    }
}

// ============================================================================
// 46. 0x21c0ff0 (17 B) - sub_21c0ff0
// Reset BND4 entry iterator if current entry is active
// ============================================================================
extern "C" void sub_21c0ff0(void* self) {
    if (MemRef<void*>(self, 0x20) != nullptr) {
        sub_20f1c20(static_cast<std::uint8_t*>(self) + 0x18);
    }
}

// ============================================================================
// 47. 0x21c0f90 (81 B) - sub_21c0f90
// Get wide-string name pointer of the currently selected BND4 archive entry
// ============================================================================
extern "C" const void* sub_21c0f90(const void* self) {
    const std::uint8_t* entry = MemRef<const std::uint8_t*>(self, 0x20);
    if (!entry) {
        return nullptr;
    }
    std::uint8_t flags = MemRef<std::uint8_t>(self, 0x34);
    if ((flags & 0x12U) != 0x12U) {
        return nullptr;
    }
    const std::uint8_t* hdr = MemRef<const std::uint8_t*>(self, 0x18);
    if (!hdr) {
        return nullptr;
    }
    std::uint32_t off = (MemRef<std::uint16_t>(hdr, 0x31) & 4U) * 2U;
    off += (flags & 1U) ? 0x18U : 0x14U;
    if ((flags & 4U) != 0) {
        off += 4U;
    }
    std::uint32_t name_rva = MemRef<std::uint32_t>(entry, static_cast<std::ptrdiff_t>(off));
    return hdr + name_rva;
}

// ============================================================================
// 48. 0x21c0ea0 (230 B) - sub_21c0ea0
// Allocate aligned buffer for current BND4 entry and extract/decompress its payload
// ============================================================================
extern "C" std::int64_t sub_21c0ea0(void* self, void* out_buf_holder, std::uint64_t alignment) {
    const std::uint8_t* entry = MemRef<const std::uint8_t*>(self, 0x20);
    if (!entry) {
        return 0;
    }

    const std::uint8_t* size_ptr = nullptr;
    if (static_cast<std::int8_t>(entry[0]) >= 0) {
        size_ptr = entry + 0x08;
    } else {
        std::ptrdiff_t off = (MemRef<void*>(self, 0x18) != nullptr) ? 0x10 : 0x00;
        size_ptr = entry + off;
    }
    if (!size_ptr) {
        return 0;
    }

    std::uint64_t unpacked_size = MemRef<std::uint64_t>(size_ptr, 0x00);
    if (unpacked_size == 0) {
        return 0;
    }

    if (MemRef<std::uint8_t>(out_buf_holder, 0x20) != 0) {
        void* old_ptr = MemRef<void*>(out_buf_holder, 0x10);
        if (old_ptr) {
            void* alloc = MemRef<void*>(out_buf_holder, 0x08);
            if (MemRef<std::uint8_t>(out_buf_holder, 0x21) != 0) {
                VCall<void(*)(void*, void*)>(alloc, 0xa0)(alloc, old_ptr);
            } else {
                VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, old_ptr);
            }
        }
    }
    MemRef<std::uint16_t>(out_buf_holder, 0x20) = 0;
    MemRef<std::uint64_t>(out_buf_holder, 0x18) = 0;
    MemRef<std::uint64_t>(out_buf_holder, 0x10) = 0;

    void* alloc = MemRef<void*>(out_buf_holder, 0x08);
    void* new_ptr = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(alloc, 0x58)(alloc, unpacked_size, alignment);
    MemRef<void*>(out_buf_holder, 0x10) = new_ptr;
    MemRef<std::uint8_t>(out_buf_holder, 0x21) = 0;
    if (!new_ptr) {
        return 0;
    }

    MemRef<std::uint64_t>(out_buf_holder, 0x18) = unpacked_size;
    MemRef<std::uint8_t>(out_buf_holder, 0x20) = 1;
    return sub_21c0d30(self, new_ptr, unpacked_size);
}

// ============================================================================
// 49. 0x21c0d30 (356 B) - sub_21c0d30
// Extract or deflate-decompress current BND4 entry into caller-supplied dst_buf
// ============================================================================
extern "C" std::int64_t sub_21c0d30(void* self, void* dst_buf, std::uint64_t max_size) {
    const std::uint8_t* entry = MemRef<const std::uint8_t*>(self, 0x20);
    if (!entry || max_size == 0 || !dst_buf) {
        return 0;
    }

    const std::uint8_t* hdr = MemRef<const std::uint8_t*>(self, 0x18);
    std::ptrdiff_t data_off_rva = 0;
    if (hdr) {
        data_off_rva = static_cast<std::ptrdiff_t>((MemRef<std::uint16_t>(hdr, 0x31) & 4U) * 2U + 0x10U);
    }
    const std::uint8_t* rva_ptr = entry + data_off_rva;
    std::uint32_t data_offset = rva_ptr ? MemRef<std::uint32_t>(rva_ptr, 0x00) : 0U;
    std::uint64_t comp_size = MemRef<std::uint64_t>(entry, 0x08);

    std::uint64_t end_pos = static_cast<std::uint32_t>(comp_size) + data_offset;
    if (MemRef<std::uint64_t>(self, 0x40) < end_pos) {
        return 0;
    }

    if (static_cast<std::int8_t>(entry[0]) >= 0) {
        std::uint64_t copy_len = static_cast<std::uint32_t>(comp_size);
        if (copy_len < max_size) {
            max_size = copy_len;
        }
        const void* src_ptr = MemRef<const std::uint8_t*>(self, 0x38) + data_offset;
        sub_261a1b0(dst_buf, src_ptr, max_size);
        return static_cast<std::int64_t>(max_size);
    }

    std::ptrdiff_t unp_off = (hdr != nullptr) ? 0x10 : 0x00;
    const std::uint8_t* unp_ptr = entry + unp_off;
    std::uint64_t unpacked_size = unp_ptr ? MemRef<std::uint64_t>(unp_ptr, 0x00) : 0ULL;

    struct StreamDesc {
        void* vtbl;
        std::uint64_t size;
        const void* data;
        std::uint64_t pos;
        std::uint32_t flags0;
        std::uint8_t flag1;
        std::uint8_t pad[3];
        std::uint32_t flags2;
    } stream_desc{};

    stream_desc.vtbl = g_vtable_53b2be0;
    stream_desc.size = static_cast<std::uint32_t>(comp_size);
    stream_desc.data = MemRef<const std::uint8_t*>(self, 0x38) + data_offset;
    stream_desc.pos = 0;
    stream_desc.flags0 = 0;
    stream_desc.flag1 = 1;
    stream_desc.flags2 = 0;

    void* decomp = MemRef<void*>(self, 0x08);
    if (!VCall<bool(*)(void*, void*, std::uint32_t)>(decomp, 0x88)(decomp, &stream_desc, 0)) {
        return 0;
    }

    if (unpacked_size < max_size) {
        max_size = unpacked_size;
    }
    decomp = MemRef<void*>(self, 0x08);
    std::int32_t read_bytes = VCall<std::int32_t(*)(void*, void*, std::uint64_t)>(decomp, 0x20)(decomp, dst_buf, max_size);
    decomp = MemRef<void*>(self, 0x08);
    VCall<void(*)(void*)>(decomp, 0x40)(decomp);
    return static_cast<std::int64_t>(read_bytes);
}

// ============================================================================
// 50. 0x21c0c50 (209 B) - sub_21c0c50
// Allocate unaligned buffer for current BND4 entry and extract/decompress its payload
// ============================================================================
extern "C" std::int64_t sub_21c0c50(void* self, void* out_buf_holder) {
    const std::uint8_t* entry = MemRef<const std::uint8_t*>(self, 0x20);
    if (!entry) {
        return 0;
    }

    const std::uint8_t* size_ptr = nullptr;
    if (static_cast<std::int8_t>(entry[0]) >= 0) {
        size_ptr = entry + 0x08;
    } else {
        std::ptrdiff_t off = (MemRef<void*>(self, 0x18) != nullptr) ? 0x10 : 0x00;
        size_ptr = entry + off;
    }
    if (!size_ptr) {
        return 0;
    }

    std::uint64_t unpacked_size = MemRef<std::uint64_t>(size_ptr, 0x00);
    if (unpacked_size == 0) {
        return 0;
    }

    if (MemRef<std::uint8_t>(out_buf_holder, 0x20) != 0) {
        void* old_ptr = MemRef<void*>(out_buf_holder, 0x10);
        if (old_ptr) {
            void* alloc = MemRef<void*>(out_buf_holder, 0x08);
            if (MemRef<std::uint8_t>(out_buf_holder, 0x21) != 0) {
                VCall<void(*)(void*, void*)>(alloc, 0xa0)(alloc, old_ptr);
            } else {
                VCall<void(*)(void*, void*)>(alloc, 0x70)(alloc, old_ptr);
            }
        }
    }
    MemRef<std::uint16_t>(out_buf_holder, 0x20) = 0;
    MemRef<std::uint64_t>(out_buf_holder, 0x18) = 0;
    MemRef<std::uint64_t>(out_buf_holder, 0x10) = 0;

    void* alloc = MemRef<void*>(out_buf_holder, 0x08);
    void* new_ptr = VCall<void*(*)(void*, std::uint64_t)>(alloc, 0x50)(alloc, unpacked_size);
    MemRef<void*>(out_buf_holder, 0x10) = new_ptr;
    if (!new_ptr) {
        return 0;
    }

    MemRef<std::uint64_t>(out_buf_holder, 0x18) = unpacked_size;
    MemRef<std::uint8_t>(out_buf_holder, 0x20) = 1;
    return sub_21c0d30(self, new_ptr, unpacked_size);
}

// ============================================================================
// 51. 0x21c0b60 (235 B) - sub_21c0b60
// Find BND4 entry by ID, allocate buffer, and extract/decompress its payload
// ============================================================================
extern "C" std::int64_t sub_21c0b60(void* self, void* out_buf_holder, std::uint32_t entry_id) {
    void* bnd_hdr = MemRef<void*>(self, 0x10);
    if (!bnd_hdr) {
        return 0;
    }
    if (sub_20f3680(bnd_hdr, entry_id, static_cast<std::uint8_t*>(self) + 0x18) != 0) {
        return 0;
    }
    return sub_21c0c50(self, out_buf_holder);
}

// ============================================================================
// 52. 0x21c0b20 (55 B) - sub_21c0b20
// Select BND4 entry by index
// ============================================================================
extern "C" bool sub_21c0b20(void* self, std::uint32_t index) {
    void* bnd_hdr = MemRef<void*>(self, 0x10);
    if (!bnd_hdr) {
        return false;
    }
    void* raw_bnd = MemRef<void*>(bnd_hdr, 0x00);
    if (!raw_bnd) {
        return false;
    }
    if (MemRef<std::uint32_t>(raw_bnd, 0x0c) <= index) {
        return false;
    }
    sub_20f1920(static_cast<std::uint8_t*>(self) + 0x18, raw_bnd, index);
    return true;
}

// ============================================================================
// 53. 0x21c0b00 (24 B) - sub_21c0b00
// Query BND4 header version/flags via sub_20f6650
// ============================================================================
extern "C" std::uint32_t sub_21c0b00(void* self) {
    void* bnd_hdr = MemRef<void*>(self, 0x10);
    if (!bnd_hdr) {
        return 0;
    }
    return sub_20f6650(bnd_hdr);
}

// ============================================================================
// 54. 0x21c0ae0 (23 B) - sub_21c0ae0
// Return number of entries in the mounted BND4 archive
// ============================================================================
extern "C" std::uint32_t sub_21c0ae0(const void* self) {
    void* bnd_hdr = MemRef<void*>(self, 0x10);
    if (!bnd_hdr) {
        return 0;
    }
    void* raw_bnd = MemRef<void*>(bnd_hdr, 0x00);
    if (!raw_bnd) {
        return 0;
    }
    return MemRef<std::uint32_t>(raw_bnd, 0x0c);
}

// ============================================================================
// 55. 0x21c09e0 (253 B) - sub_21c09e0
// Mount BND4 archive in memory and initialize deflate decompressor codec
// ============================================================================
extern "C" bool sub_21c09e0(void* self, const void* data, std::uint64_t size) {
    sub_21c08b0(self);

    void* alloc = MemRef<void*>(self, 0x00);
    void* bnd_hdr = VCall<void*(*)(void*, std::uint64_t, std::uint64_t)>(alloc, 0x58)(alloc, 0x50, 0x08);
    if (bnd_hdr) {
        MemRef<std::uint64_t>(bnd_hdr, 0x30) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x28) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x20) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x18) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x10) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x08) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x00) = 0;

        void* global_alloc = g_allocator_5540670;
        if (!global_alloc) {
            global_alloc = sub_2ba2b10();
            g_allocator_5540670 = global_alloc;
        }
        MemRef<void*>(bnd_hdr, 0x30) = global_alloc;
        MemRef<std::uint64_t>(bnd_hdr, 0x48) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x40) = 0;
        MemRef<std::uint64_t>(bnd_hdr, 0x38) = 0;
    }

    MemRef<void*>(self, 0x10) = bnd_hdr;
    if (sub_20f1fe0(bnd_hdr, data, 0, 0, 0) != 0) {
        sub_21c08b0(self);
        return false;
    }

    MemRef<const void*>(self, 0x38) = data;
    MemRef<std::uint64_t>(self, 0x40) = size;
    MemRef<void*>(self, 0x08) = sub_20625a0(reinterpret_cast<const char*>(GX_VMA(0x4940254ULL)), MemRef<void*>(self, 0x00));
    return true;
}

} // namespace Bloodborne::SceneRenderer
