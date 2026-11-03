// ============================================================================
// Bloodborne Cousin Engine (CUSA03173 v1.09) - Module 04: SceneRenderer
// File: GXBoundingVolumeCullingAndPathStringUtils.cpp
// Range: 0x21b9f90 .. 0x21bd230 (55 functions, 10,828 bytes)
// Architecture: x86-64 System V ABI / PS4 Orbis -> Native C++20 / AVX
// Zero-stub bitwise reconstruction in Right-to-Left (RTL) order.
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <immintrin.h>

extern "C" {

// External engine functions resolved from eboot.bin
uint8_t  sub_e48ec0(void* task_obj, void* out_result);
void     sub_e48280(void* task_obj, uint32_t frame_count, void* user_arg);
void     sub_e488a0(void* task_obj, const void* a1, const void* a2, uint32_t a3, uint32_t a4, uint32_t a5);
void     sub_21bd480(void* vec_self, uint64_t new_capacity);
void*    sub_2ba2b10();
void     sub_20b55b0(const void* file_or_null, uint32_t line_or_code, const char* msg, ...);
uint8_t  sub_207ee80(void* mutex_obj);
int32_t  sub_2083fe0(void* mutex_inner);
int32_t  sub_2bbf228(const char16_t* str);
char16_t* sub_2bbf258(const char16_t* str, int32_t ch);
char16_t* sub_2bbf7a8(char16_t* dst, const char16_t* src, size_t count);
void     sub_262a720(void* dl_string, const char16_t* src, size_t len);
int32_t  sub_2165f90(void* dev, void** out_tex, const void* desc, uint32_t c, uint32_t r8, uint32_t r9, void* stack0);
void     sub_2168660(void* ctx, void* out_tex_ptr_slot, void* tex, const void* gnf_hdr);
int32_t  sub_2165150(void* tex, uint32_t zero_esi, uint32_t flags_edx);
void     sub_2169c30(void* ctx, uint32_t zero_esi, void* tex, uint32_t slice_idx, uint32_t zero_r8, void* out_map_info);
void*    sub_207b720(void* obj);
void     sub_21a2580(void* cmd_ctx, uint32_t zero_esi, uint64_t target_ptr);
void*    sub_26db050(void* shader_mgr);
void     sub_2191c30(void* state_mgr, void* cmd_ctx, uint32_t pass_type, uint32_t zero_ecx);
void     sub_21a2650(void* cmd_ctx);
void     sub_261a1b0(void* dst, const void* src, size_t bytes);
void     sub_2192170(void* state_mgr, void* cmd_ctx, const void* mat4x4, uint32_t zero_ecx);
void     sub_207d250();
void     sub_2686110(void* prng_state, uint32_t seed);
int32_t  sub_2bbe728(void* timeval_out, void* tz_null);
void     sub_2686170(void* prng_state);
uint8_t  sub_29add90(void* prng_state);
void     sub_217fba0(void* tex3d, uint32_t w, uint32_t h, uint32_t d, uint32_t fmt, uint32_t mips,
                     void* data_desc, uint32_t zero_flag, void* alloc1, void* alloc2);
void     sub_2686260(void* prng_state);
int32_t  sub_2bbe538(void* guard_var);
void     sub_2bbe558(void* guard_var);
void*    sub_207bbf0();
void     sub_20b5a60(void* obj);
void     sub_2bbe588(void* ptr);
int32_t  sub_21669a0(void* dev, void** out_view, void* base_tex, const void* view_desc, uint32_t zero_r8);

} // extern "C"

namespace {

alignas(16) constexpr float kVec4One[4]         = {1.0f, 1.0f, 1.0f, 1.0f};          // 0x49209d0, 0x49209e0, 0x4920a00, 0x4920ac0
alignas(16) constexpr float kQuadPos0[4]        = {-1.0f, 1.0f, 1.0f, 1.0f};         // 0x49209f0
alignas(16) constexpr float kQuadPos2[4]        = {-1.0f, -1.0f, 1.0f, 1.0f};        // 0x4920a10
alignas(16) constexpr float kQuadPos3[4]        = {1.0f, -1.0f, 1.0f, 1.0f};         // 0x4920a20
alignas(16) constexpr float kAxisPosY[4]        = {0.0f, 1.0f, 0.0f, 0.0f};          // 0x4920a30
alignas(16) constexpr float kAxisPosZ[4]        = {0.0f, 0.0f, 1.0f, 0.0f};          // 0x4920a40
alignas(16) constexpr float kAxisNegY[4]        = {0.0f, -1.0f, 0.0f, 0.0f};         // 0x4920a50
alignas(16) constexpr float kAxisNegZ[4]        = {0.0f, 0.0f, -1.0f, 0.0f};         // 0x4920a60
alignas(16) constexpr uint32_t kMaskXYZ[4]      = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u}; // 0x4920a70
alignas(16) constexpr float kUnitW[4]           = {0.0f, 0.0f, 0.0f, 1.0f};          // 0x4920a80, 0x4738070
alignas(16) constexpr float kVec4Half[4]        = {0.5f, 0.5f, 0.5f, 0.5f};          // 0x4920a90, 0x4920af0
alignas(16) constexpr uint32_t kMaskY[4]        = {0x00000000u, 0xFFFFFFFFu, 0x00000000u, 0x00000000u}; // 0x4920aa0
alignas(16) constexpr uint32_t kMaskZ[4]        = {0x00000000u, 0x00000000u, 0xFFFFFFFFu, 0x00000000u}; // 0x4920ab0
alignas(16) constexpr float kVec4Quarter[4]     = {0.25f, 0.25f, 0.25f, 0.25f};      // 0x4920ad0
alignas(16) constexpr uint32_t kVec4OneSixth[4] = {0x3e2aaaabu, 0x3e2aaaabu, 0x3e2aaaabu, 0x3e2aaaabu}; // 0x4920ae0

// Helper to decode a single IEEE-754 binary16 (half-float) value identically to 0x21bbee6..0x21bbf4d
static inline uint32_t DecodeHalfToFloatBits(uint16_t h) noexcept {
    uint32_t eax = h;
    uint32_t ebx = eax & 0x3ffu;
    uint32_t ecx = 0;
    if ((eax & 0x7c00u) != 0u) {
        ecx = (eax >> 10) & 0x1fu;
    } else {
        ecx = 1u;
        if (ebx != 0u) {
            do {
                --ecx;
                ebx += ebx;
            } while ((ebx & 0x0400u) == 0u);
            ebx &= 0x3feu;
        } else {
            ecx = 0xffffff90u;
            ebx = 0u;
        }
    }
    uint32_t sign = (eax << 16) & 0x80000000u;
    ecx = ((ecx << 23) + 0x38000000u) | sign;
    ebx = (ebx << 13) | ecx;
    return ebx;
}

} // namespace

extern "C" {

// ============================================================================
// 1. 0x21bd230 (8 B) - sub_21bd230
// ============================================================================
bool sub_21bd230(const void* self) {
    const uint32_t state = *reinterpret_cast<const uint32_t*>(
        reinterpret_cast<const uint8_t*>(self) + 0x40);
    return state > 1u;
}

// ============================================================================
// 2. 0x21bd170 (179 B) - sub_21bd170
// ============================================================================
bool sub_21bd170(void* self, uint32_t mode, uint32_t count, uint8_t flag) {
    auto* base = static_cast<uint8_t*>(self);
    if (*reinterpret_cast<uint32_t*>(base + 0x40) > 1u) {
        return false;
    }

    *reinterpret_cast<uint32_t*>(base + 0x40) = mode;
    base[0x4c] = flag;
    const uint32_t extra = (flag != 0u) ? count : 0u;
    *reinterpret_cast<uint32_t*>(base + 0x48) = extra;
    *reinterpret_cast<uint32_t*>(base + 0x44) = 0u;

    uint8_t* const mgr = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    uint8_t* const disp = *reinterpret_cast<uint8_t* const*>(mgr + 0x90);
    uint32_t width  = *reinterpret_cast<const uint32_t*>(disp + 0x80);
    uint32_t height = *reinterpret_cast<const uint32_t*>(disp + 0x84);

    if (flag != 0u) {
        width  += count * 2u;
        height += count * 2u;
    } else {
        width  *= mode;
        height *= mode;
    }

    uint8_t* list_container = *reinterpret_cast<uint8_t* const*>(mgr + 0x5f8);
    if (list_container != nullptr) {
        void** it = *reinterpret_cast<void***>(list_container + 0x08);
        while (it != *reinterpret_cast<void***>(list_container + 0x10)) {
            void* listener = *it;
            auto* vtable = *reinterpret_cast<void (***)(void*, uint32_t, uint32_t)>(listener);
            vtable[3](listener, width, height);
            ++it;
            list_container = *reinterpret_cast<uint8_t* const*>(mgr + 0x5f8);
        }
    }
    return true;
}

// ============================================================================
// 3. 0x21bd140 (48 B) - sub_21bd140
// ============================================================================
bool sub_21bd140(const void* self, uint32_t task_id) {
    const auto* base = static_cast<const uint8_t*>(self);
    void* const* it  = *reinterpret_cast<void* const* const*>(base + 0x08);
    void* const* end = *reinterpret_cast<void* const* const*>(base + 0x10);

    while (it != end) {
        const uint8_t* item = static_cast<const uint8_t*>(*it);
        ++it;
        if (item == nullptr) {
            continue;
        }
        if (*reinterpret_cast<const uint32_t*>(item + 0x88) == task_id) {
            return *reinterpret_cast<const uint32_t*>(item) > 2u;
        }
    }
    return true;
}

// ============================================================================
// 4. 0x21bd100 (56 B) - sub_21bd100
// ============================================================================
uint8_t sub_21bd100(const void* self, void* out_result, uint32_t task_id) {
    const auto* base = static_cast<const uint8_t*>(self);
    void* const* it  = *reinterpret_cast<void* const* const*>(base + 0x08);
    void* const* end = *reinterpret_cast<void* const* const*>(base + 0x10);

    while (it != end) {
        void* item = *it;
        ++it;
        if (item == nullptr) {
            continue;
        }
        if (*reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(item) + 0x88) == task_id) {
            return sub_e48ec0(item, out_result);
        }
    }
    return 0u;
}

// ============================================================================
// 5. 0x21bcf40 (447 B) - sub_21bcf40
// ============================================================================
uint32_t sub_21bcf40(void* self, const void* a1, const void* a2,
                     uint32_t a3, uint32_t a4, uint32_t a5, void* stack_arg6) {
    auto* base = static_cast<uint8_t*>(self);
    void* mutex_obj = base + 0x28;
    {
        auto* mtx_vtbl = *reinterpret_cast<void (***)(void*, uint32_t)>(mutex_obj);
        mtx_vtbl[3](mutex_obj, 0xFFFFFFFFu);
    }

    uint32_t frame_cnt = 1u;
    if (base[0x4c] != 0u) {
        frame_cnt = *reinterpret_cast<const uint32_t*>(base + 0x40);
    }

    void* allocator = *reinterpret_cast<void* const*>(GX_VMA(0x5540680ULL));
    auto* alloc_vtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(allocator);
    void* raw_mem = alloc_vtbl[11](allocator, 0x90u, 0x08u);

    void* task_obj = nullptr;
    if (raw_mem != nullptr) {
        sub_e48280(raw_mem, frame_cnt, stack_arg6);
        task_obj = raw_mem;
    }
    if (task_obj == nullptr) {
        auto* mtx_vtbl = *reinterpret_cast<void (***)(void*)>(mutex_obj);
        mtx_vtbl[5](mutex_obj);
        return 0u;
    }

    const uint32_t next_id = ++(*reinterpret_cast<uint32_t*>(base + 0x50));
    *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(task_obj) + 0x88) = next_id;

    void** vec_begin = *reinterpret_cast<void***>(base + 0x08);
    void** vec_end   = *reinterpret_cast<void***>(base + 0x10);
    void** vec_cap   = *reinterpret_cast<void***>(base + 0x18);
    void** slot_addr = &task_obj;

    if (vec_end <= slot_addr || vec_begin > slot_addr) {
        if (vec_end == vec_cap) {
            const uint64_t cur_len = static_cast<uint64_t>(vec_end - vec_begin);
            const uint64_t half    = cur_len >> 1;
            const uint64_t max_lim = 0x1FFFFFFFFFFFFFFFULL - half;
            uint64_t new_cap       = (max_lim >= cur_len) ? (cur_len + half) : 0ULL;
            if (new_cap < cur_len + 1ULL) {
                new_cap = cur_len + 1ULL;
            }
            sub_21bd480(self, new_cap);
            vec_end = *reinterpret_cast<void***>(base + 0x10);
        }
        if (vec_end != nullptr) {
            *vec_end = task_obj;
        }
    } else {
        const ptrdiff_t idx = slot_addr - vec_begin;
        if (vec_end == vec_cap) {
            const uint64_t cur_len = static_cast<uint64_t>(vec_end - vec_begin);
            const uint64_t half    = cur_len >> 1;
            const uint64_t max_lim = 0x1FFFFFFFFFFFFFFFULL - half;
            uint64_t new_cap       = (max_lim >= cur_len) ? (cur_len + half) : 0ULL;
            if (new_cap < cur_len + 1ULL) {
                new_cap = cur_len + 1ULL;
            }
            sub_21bd480(self, new_cap);
            vec_end = *reinterpret_cast<void***>(base + 0x10);
        }
        if (vec_end != nullptr) {
            void** new_begin = *reinterpret_cast<void***>(base + 0x08);
            *vec_end = new_begin[idx];
        }
    }

    *reinterpret_cast<void***>(base + 0x10) = vec_end + 1;

    sub_e488a0(task_obj, a1, a2, a3, a4, a5);
    const uint32_t assigned_id = *reinterpret_cast<const uint32_t*>(
        static_cast<const uint8_t*>(task_obj) + 0x88);

    {
        auto* mtx_vtbl = *reinterpret_cast<void (***)(void*)>(mutex_obj);
        mtx_vtbl[5](mutex_obj);
    }
    return assigned_id;
}

// ============================================================================
// 6. 0x21bce40 (241 B) - sub_21bce40
// ============================================================================
void sub_21bce40(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    void** global_heap_slot = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* heap = *global_heap_slot;
    if (heap == nullptr) {
        heap = sub_2ba2b10();
        *global_heap_slot = heap;
    }

    uint64_t heap_info = 0;
    {
        auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(heap);
        heap_vtbl[4](&heap_info, heap, 0u);
    }
    if ((heap_info & 0x20ULL) == 0ULL) {
        sub_20b55b0(nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(base + 0x18) = 0ULL;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0ULL;
    *reinterpret_cast<uint64_t*>(base + 0x08) = 0ULL;
    *reinterpret_cast<void**>(base + 0x20)    = heap;

    base[0x38] = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x28) = GX_VMA(0x52b6230ULL);
    if (sub_207ee80(base + 0x28) == 0u) {
        if (sub_2083fe0(base + 0x30) == 0) {
            base[0x38] = 1u;
        }
    }

    *reinterpret_cast<uint32_t*>(base + 0x50) = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x58) = 0ULL;
    base[0x4c] = 0u;
    *reinterpret_cast<uint32_t*>(base + 0x48) = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x40) = 0ULL;
}

// ============================================================================
// 7. 0x21bcd80 (33 B) - sub_21bcd80
// ============================================================================
bool sub_21bcd80() {
    const uint8_t* root = *reinterpret_cast<const uint8_t* const*>(GX_VMA(0x55406c8ULL));
    const uint8_t* queue = *reinterpret_cast<const uint8_t* const*>(root + 0x660);
    if (queue == nullptr) {
        return false;
    }
    return *reinterpret_cast<const uint32_t*>(queue + 0x40) > 1u;
}

// ============================================================================
// 8. 0x21bccb0 (194 B) - sub_21bccb0
// ============================================================================
bool sub_21bccb0(uint32_t mode, uint32_t count, uint8_t flag) {
    uint8_t* const root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    uint8_t* const queue = *reinterpret_cast<uint8_t* const*>(root + 0x660);
    if (queue == nullptr || *reinterpret_cast<const uint32_t*>(queue + 0x40) > 1u) {
        return false;
    }

    *reinterpret_cast<uint32_t*>(queue + 0x40) = mode;
    queue[0x4c] = flag;
    const uint32_t extra = (flag != 0u) ? count : 0u;
    *reinterpret_cast<uint32_t*>(queue + 0x48) = extra;
    *reinterpret_cast<uint32_t*>(queue + 0x44) = 0u;

    const uint8_t* disp = *reinterpret_cast<const uint8_t* const*>(root + 0x90);
    uint32_t width  = *reinterpret_cast<const uint32_t*>(disp + 0x80);
    uint32_t height = *reinterpret_cast<const uint32_t*>(disp + 0x84);
    if (flag != 0u) {
        width  += count * 2u;
        height += count * 2u;
    } else {
        width  *= mode;
        height *= mode;
    }

    uint8_t* list_container = *reinterpret_cast<uint8_t* const*>(root + 0x5f8);
    if (list_container != nullptr) {
        void** it = *reinterpret_cast<void***>(list_container + 0x08);
        while (it != *reinterpret_cast<void***>(list_container + 0x10)) {
            void* listener = *it;
            auto* vtable = *reinterpret_cast<void (***)(void*, uint32_t, uint32_t)>(listener);
            vtable[3](listener, width, height);
            ++it;
            list_container = *reinterpret_cast<uint8_t* const*>(root + 0x5f8);
        }
    }
    return true;
}

// ============================================================================
// 9. 0x21bcc70 (64 B) - sub_21bcc70
// ============================================================================
bool sub_21bcc70(uint32_t task_id) {
    const uint8_t* root  = *reinterpret_cast<const uint8_t* const*>(GX_VMA(0x55406c8ULL));
    const uint8_t* queue = *reinterpret_cast<const uint8_t* const*>(root + 0x660);
    if (queue == nullptr) {
        return true;
    }
    return sub_21bd140(queue, task_id);
}

// ============================================================================
// 10. 0x21bcc10 (95 B) - sub_21bcc10
// ============================================================================
uint8_t sub_21bcc10(void* out_result, uint32_t task_id) {
    const uint8_t* root  = *reinterpret_cast<const uint8_t* const*>(GX_VMA(0x55406c8ULL));
    const uint8_t* queue = *reinterpret_cast<const uint8_t* const*>(root + 0x660);
    if (queue == nullptr) {
        return 0u;
    }
    return sub_21bd100(queue, out_result, task_id);
}

// ============================================================================
// 11. 0x21bcbc0 (68 B) - sub_21bcbc0
// ============================================================================
uint32_t sub_21bcbc0(const void* a1, const void* a2, uint32_t a3,
                     uint32_t a4, uint32_t a5, void* a6) {
    uint8_t* root  = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    void*    queue = *reinterpret_cast<void* const*>(root + 0x660);
    if (queue == nullptr) {
        return 0u;
    }
    return sub_21bcf40(queue, a1, a2, a3, a4, a5, a6);
}

// ============================================================================
// 12. 0x21bcb70 (73 B) - sub_21bcb70
// ============================================================================
uint32_t sub_21bcb70(const void* a1, const void* a2, uint32_t a3, void* a4) {
    uint8_t* root  = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    void*    queue = *reinterpret_cast<void* const*>(root + 0x660);
    if (queue == nullptr) {
        return 0u;
    }
    return sub_21bcf40(queue, a1, a2, a3, 0u, 0u, a4);
}

// ============================================================================
// 13. 0x21bca90 (65 B) - sub_21bca90
// ============================================================================
void sub_21bca90(char16_t* dst, const char16_t* src, char16_t sep, size_t max_len) {
    size_t i = 0;
    while (i < max_len) {
        const char16_t ch = src[i];
        if (ch == u'/' || ch == u'\\') {
            dst[i] = sep;
        } else {
            dst[i] = ch;
        }
        if (src[i] == u'\0') {
            break;
        }
        ++i;
    }
}

// ============================================================================
// 14. 0x21bc980 (271 B) - sub_21bc980
// ============================================================================
void sub_21bc980(const char16_t* path, void* out_dl_string) {
    const char16_t* cur = path;
    const int32_t len = sub_2bbf228(cur);
    if (len > 0) {
        int64_t byte_off = (static_cast<int64_t>(len) - 1) * 2;
        int64_t scan_off = byte_off;
        char16_t ch = u'\0';
        while (true) {
            scan_off = byte_off;
            ch = *reinterpret_cast<const char16_t*>(
                reinterpret_cast<const uint8_t*>(cur) + scan_off);
            if (ch != u'/' && ch != u'\\') {
                break;
            }
            byte_off = scan_off - 2;
            const auto* prev_ptr = reinterpret_cast<const char16_t*>(
                reinterpret_cast<const uint8_t*>(cur) + scan_off);
            if (prev_ptr <= cur) {
                break;
            }
        }
        if (scan_off != 0) {
            const auto* p = reinterpret_cast<const char16_t*>(
                reinterpret_cast<const uint8_t*>(cur) + scan_off);
            while (ch != u'/' && ch != u'\\') {
                if (p <= cur) {
                    break;
                }
                ch = *(p - 1);
                --p;
            }
            if (ch == u'/' || ch == u'\\') {
                ++p;
            }
            cur = p;
        }
    }

    const char16_t* dot   = sub_2bbf258(cur, u'.');
    const char16_t* slash = sub_2bbf258(cur, u'/');
    const char16_t* bslsh = sub_2bbf258(cur, u'\\');

    size_t copy_len = 0;
    if (dot != nullptr && dot >= slash && dot >= bslsh) {
        copy_len = static_cast<size_t>(dot - cur);
    } else if (*cur != u'\0') {
        copy_len = static_cast<size_t>(sub_2bbf228(cur));
    }
    sub_262a720(out_dl_string, cur, copy_len);
}

// ============================================================================
// 15. 0x21bc840 (309 B) - sub_21bc840
// ============================================================================
void sub_21bc840(const char16_t* path, char16_t* out_buf, size_t max_chars) {
    if (max_chars == 0u) {
        return;
    }

    const char16_t* cur = path;
    const int32_t len = sub_2bbf228(cur);
    if (len > 0) {
        int64_t byte_off = (static_cast<int64_t>(len) - 1) * 2;
        int64_t scan_off = byte_off;
        char16_t ch = u'\0';
        while (true) {
            scan_off = byte_off;
            ch = *reinterpret_cast<const char16_t*>(
                reinterpret_cast<const uint8_t*>(cur) + scan_off);
            if (ch != u'/' && ch != u'\\') {
                break;
            }
            byte_off = scan_off - 2;
            const auto* prev_ptr = reinterpret_cast<const char16_t*>(
                reinterpret_cast<const uint8_t*>(cur) + scan_off);
            if (prev_ptr <= cur) {
                break;
            }
        }
        if (scan_off != 0) {
            const auto* p = reinterpret_cast<const char16_t*>(
                reinterpret_cast<const uint8_t*>(cur) + scan_off);
            while (ch != u'/' && ch != u'\\') {
                if (p <= cur) {
                    break;
                }
                ch = *(p - 1);
                --p;
            }
            if (ch == u'/' || ch == u'\\') {
                ++p;
            }
            cur = p;
        }
    }

    const char16_t* dot   = sub_2bbf258(cur, u'.');
    const char16_t* slash = sub_2bbf258(cur, u'/');
    const char16_t* bslsh = sub_2bbf258(cur, u'\\');

    if (dot != nullptr && dot >= slash && dot >= bslsh) {
        const size_t stem_len = static_cast<size_t>(dot - cur);
        size_t n = max_chars - 1u;
        if (n >= stem_len) {
            n = stem_len;
        }
        sub_2bbf7a8(out_buf, cur, n);
        out_buf[n] = u'\0';
    } else {
        sub_2bbf7a8(out_buf, cur, max_chars - 1u);
        out_buf[max_chars - 1u] = u'\0';
    }
}

// ============================================================================
// 16. 0x21bc7b0 (139 B) - sub_21bc7b0
// ============================================================================
bool sub_21bc7b0(const char16_t* path, void* out_dl_string) {
    const char16_t* dot   = sub_2bbf258(path, u'.');
    const char16_t* slash = sub_2bbf258(path, u'/');
    const char16_t* bslsh = sub_2bbf258(path, u'\\');

    if (dot != nullptr && dot >= slash && dot >= bslsh) {
        const size_t stem_len = static_cast<size_t>(dot - path);
        sub_262a720(out_dl_string, path, stem_len);
        return true;
    }

    size_t full_len = 0;
    if (*path != u'\0') {
        full_len = static_cast<size_t>(sub_2bbf228(path));
    }
    sub_262a720(out_dl_string, path, full_len);
    return false;
}

// ============================================================================
// 17. 0x21bc750 (94 B) - sub_21bc750
// ============================================================================
bool sub_21bc750(char16_t* path) {
    char16_t* dot   = sub_2bbf258(path, u'.');
    char16_t* slash = sub_2bbf258(path, u'/');
    char16_t* bslsh = sub_2bbf258(path, u'\\');

    if (dot != nullptr && dot >= slash && dot >= bslsh) {
        *dot = u'\0';
        return true;
    }
    return false;
}

// ============================================================================
// 18. 0x21bc6f0 (90 B) - sub_21bc6f0
// ============================================================================
char16_t* sub_21bc6f0(const char16_t* path) {
    char16_t* dot   = sub_2bbf258(path, u'.');
    char16_t* slash = sub_2bbf258(path, u'/');
    char16_t* bslsh = sub_2bbf258(path, u'\\');

    if (dot != nullptr && dot >= slash && dot >= bslsh) {
        return dot;
    }
    return nullptr;
}

// ============================================================================
// 19. 0x21bc5d0 (134 B) - sub_21bc5d0
// ============================================================================
void sub_21bc5d0(void* self, size_t size, size_t align) {
    auto* base = static_cast<uint8_t*>(self);
    if (base[0x20] != 0u) {
        void* ptr = *reinterpret_cast<void**>(base + 0x10);
        if (ptr != nullptr) {
            void* alloc = *reinterpret_cast<void**>(base + 0x08);
            auto* vtbl  = *reinterpret_cast<void (***)(void*, void*)>(alloc);
            if (base[0x21] != 0u) {
                vtbl[20](alloc, ptr);
            } else {
                vtbl[14](alloc, ptr);
            }
        }
    }

    *reinterpret_cast<uint16_t*>(base + 0x20) = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0ULL;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0ULL;

    void* alloc = *reinterpret_cast<void**>(base + 0x08);
    auto* vtbl  = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(alloc);
    void* new_ptr = vtbl[11](alloc, size, align);
    *reinterpret_cast<void**>(base + 0x10) = new_ptr;
    base[0x21] = 0u;
    if (new_ptr != nullptr) {
        *reinterpret_cast<size_t*>(base + 0x18) = size;
        base[0x20] = 1u;
    }
}

// ============================================================================
// 20. 0x21bc580 (76 B) - sub_21bc580
// ============================================================================
void sub_21bc580(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    if (base[0x20] != 0u) {
        void* ptr = *reinterpret_cast<void**>(base + 0x10);
        if (ptr != nullptr) {
            void* alloc = *reinterpret_cast<void**>(base + 0x08);
            auto* vtbl  = *reinterpret_cast<void (***)(void*, void*)>(alloc);
            if (base[0x21] != 0u) {
                vtbl[20](alloc, ptr);
            } else {
                vtbl[14](alloc, ptr);
            }
        }
    }
    *reinterpret_cast<uint16_t*>(base + 0x20) = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0ULL;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0ULL;
}

// ============================================================================
// 21. 0x21bc500 (121 B) - sub_21bc500
// ============================================================================
void sub_21bc500(void* self, size_t size) {
    auto* base = static_cast<uint8_t*>(self);
    if (base[0x20] != 0u) {
        void* ptr = *reinterpret_cast<void**>(base + 0x10);
        if (ptr != nullptr) {
            void* alloc = *reinterpret_cast<void**>(base + 0x08);
            auto* vtbl  = *reinterpret_cast<void (***)(void*, void*)>(alloc);
            if (base[0x21] != 0u) {
                vtbl[20](alloc, ptr);
            } else {
                vtbl[14](alloc, ptr);
            }
        }
    }

    *reinterpret_cast<uint16_t*>(base + 0x20) = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0ULL;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0ULL;

    void* alloc = *reinterpret_cast<void**>(base + 0x08);
    auto* vtbl  = *reinterpret_cast<void* (***)(void*, size_t)>(alloc);
    void* new_ptr = vtbl[10](alloc, size);
    *reinterpret_cast<void**>(base + 0x10) = new_ptr;
    if (new_ptr != nullptr) {
        *reinterpret_cast<size_t*>(base + 0x18) = size;
        base[0x20] = 1u;
    }
}

// ============================================================================
// 22. 0x21bc480 (120 B) - sub_21bc480
// ============================================================================
void sub_21bc480(void* self, uint32_t heap_type) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(base) = GX_VMA(0x535fa10ULL);

    void* alloc = nullptr;
    switch (heap_type) {
        case 0u:
        case 3u:
            alloc = *reinterpret_cast<void* const*>(GX_VMA(0x5540680ULL));
            break;
        case 1u:
            alloc = *reinterpret_cast<void* const*>(GX_VMA(0x5540688ULL));
            break;
        case 2u:
            alloc = *reinterpret_cast<void* const*>(GX_VMA(0x5540690ULL));
            break;
        case 4u:
            alloc = *reinterpret_cast<void* const*>(GX_VMA(0x55406a8ULL));
            break;
        default:
            alloc = nullptr;
            break;
    }

    *reinterpret_cast<void**>(base + 0x08)    = alloc;
    *reinterpret_cast<uint16_t*>(base + 0x20) = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0ULL;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0ULL;
}

// ============================================================================
// 23. 0x21bc2a0 (321 B) - sub_21bc2a0
// ============================================================================
void sub_21bc2a0(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    base[0x00] = 0u;
    base[0x01] = 0u;
    base[0x04] = 0u;
    *reinterpret_cast<uint64_t*>(base + 0x08) = 1ULL;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0x100000000ULL;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0ULL;
    *reinterpret_cast<uint32_t*>(base + 0x20) = 0x0fu;

    for (size_t slot = 1; slot < 8; ++slot) {
        const size_t dst_off = slot * 0x20u;
        const uint64_t w1c = *reinterpret_cast<const uint64_t*>(base + 0x1c);
        const uint64_t w14 = *reinterpret_cast<const uint64_t*>(base + 0x14);
        const uint64_t w04 = *reinterpret_cast<const uint64_t*>(base + 0x04);
        const uint64_t w0c = *reinterpret_cast<const uint64_t*>(base + 0x0c);
        *reinterpret_cast<uint64_t*>(base + dst_off + 0x1c) = w1c;
        *reinterpret_cast<uint64_t*>(base + dst_off + 0x14) = w14;
        *reinterpret_cast<uint64_t*>(base + dst_off + 0x0c) = w0c;
        *reinterpret_cast<uint64_t*>(base + dst_off + 0x04) = w04;
    }
}

// ============================================================================
// 24. 0x21bbaf0 (1964 B) - sub_21bbaf0
// ============================================================================
bool sub_21bbaf0(void* dev_ctx, const void* gnf_hdr, uint32_t mip_level, __m128* out_basis3) {
    const auto* hdr = static_cast<const uint8_t*>(gnf_hdr);
    if (hdr[0x38] != 3u) {
        return false;
    }
    const uint32_t flags3c = *reinterpret_cast<const uint32_t*>(hdr + 0x3c);
    if ((flags3c & 0x04u) == 0u) {
        return false;
    }

    uint32_t width     = *reinterpret_cast<const uint16_t*>(hdr + 0x44);
    const uint32_t h   = *reinterpret_cast<const uint16_t*>(hdr + 0x46);
    const uint32_t mip = hdr[0x4a];
    const uint32_t d   = *reinterpret_cast<const uint16_t*>(hdr + 0x48);
    const uint32_t fmt = hdr[0x4b];
    const uint32_t f4c = *reinterpret_cast<const uint16_t*>(hdr + 0x4c);
    const uint32_t f4e = *reinterpret_cast<const uint16_t*>(hdr + 0x4e);

    if (fmt != 0x0au && fmt != 0x1cu) {
        return false;
    }

    struct TempTexDesc {
        uint32_t width;
        uint32_t height;
        uint32_t mip_levels;
        uint32_t depth;
        uint32_t format;
        uint32_t f4c;
        uint32_t f4e;
        uint32_t type_3;
        uint32_t zero_a8;
        uint32_t two_a4;
        uint32_t flags_a0;
    } desc{};

    desc.width      = width;
    width          >>= (mip_level & 31u);
    desc.height     = h;
    desc.mip_levels = mip;
    desc.depth      = d;
    desc.format     = fmt;
    desc.f4c        = f4c;
    desc.f4e        = f4e;
    desc.type_3     = 3u;
    desc.zero_a8    = 0u;
    desc.two_a4     = 2u;
    desc.flags_a0   = flags3c;

    void* staging_tex = nullptr;
    alignas(16) __m128 face_accum[6]{};

    if (sub_2165f90(dev_ctx, &staging_tex, &desc, 0u, 0u, 0u, nullptr) == 0) {
        uint8_t* root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
        uint8_t* dev_wrapper = *reinterpret_cast<uint8_t* const*>(root + 0x10);
        uint8_t* gnm_ctx = *reinterpret_cast<uint8_t* const*>(dev_wrapper + 0x08);

        sub_2168660(gnm_ctx, &staging_tex, staging_tex, gnf_hdr);

        if (fmt == 0x1cu) {
            const uint32_t texel_count = width * width;
            __m128 inv_count = _mm_cvtsi32_ss(_mm_setzero_ps(), static_cast<int32_t>(texel_count));
            inv_count = _mm_shuffle_ps(inv_count, inv_count, 0x00);

            if (width != 0u) {
                const __m128 one_vec = _mm_load_ps(kVec4One);
                inv_count = _mm_div_ps(one_vec, inv_count);
                const __m128 quarter_vec = _mm_load_ps(kVec4Quarter);
                const __m128 inv255 = _mm_set1_ps(1.0f / 255.0f);

                struct MapInfo {
                    uint8_t* base_ptr;
                    uint32_t row_pitch_bytes;
                    uint32_t pad;
                } map_info{};

                for (uint64_t face = 0; face < 6u; ++face) {
                    const uint32_t sync_flag = *reinterpret_cast<const uint32_t*>(gnm_ctx + 0xb510);
                    if (sub_2165150(staging_tex, 0u, sync_flag) == 0) {
                        uint32_t slice_idx = 0u;
                        if (static_cast<uint8_t>(mip) != 0u) {
                            slice_idx = mip * static_cast<uint32_t>(face) + mip_level;
                        }
                        sub_2169c30(gnm_ctx, 0u, staging_tex, slice_idx, 0u, &map_info);
                    }

                    __m128 sum = _mm_setzero_ps();
                    for (uint64_t x = 0; x < width; ++x) {
                        const uint8_t* col_ptr = map_info.base_ptr + ((x & 0x3fffffffULL) * 4ULL);
                        uint32_t y_byte_off = 0u;
                        for (uint32_t rem = width; rem != 0u; --rem) {
                            const uint32_t rgba = *reinterpret_cast<const uint32_t*>(col_ptr + y_byte_off);
                            const uint32_t r_u8 = rgba & 0xffu;
                            const uint32_t g_u8 = (rgba >> 8) & 0xffu;
                            const uint32_t b_u8 = (rgba >> 16) & 0xffu;
                            const uint32_t a_u8 = rgba >> 24;

                            if (a_u8 != 0u) {
                                __m128 px = _mm_cvtepi32_ps(_mm_setr_epi32(
                                    static_cast<int32_t>(r_u8),
                                    static_cast<int32_t>(g_u8),
                                    static_cast<int32_t>(b_u8),
                                    static_cast<int32_t>(a_u8)));
                                px = _mm_mul_ps(px, inv255);
                                __m128 a_xxxx = _mm_shuffle_ps(px, px, 0xff);
                                __m128 inv_a  = _mm_div_ps(one_vec, a_xxxx);
                                px = _mm_mul_ps(px, inv_a);
                                px = _mm_mul_ps(px, quarter_vec);
                                sum = _mm_add_ps(sum, px);
                            }

                            y_byte_off += map_info.row_pitch_bytes;
                        }
                    }
                    face_accum[face] = _mm_mul_ps(sum, inv_count);
                }
            } else {
                inv_count = _mm_setzero_ps();
                struct MapInfo {
                    uint8_t* base_ptr;
                    uint32_t row_pitch_bytes;
                    uint32_t pad;
                } map_info{};
                uint32_t cur_slice = mip_level;
                for (uint64_t face = 0; face < 6u; ++face) {
                    const uint32_t sync_flag = *reinterpret_cast<const uint32_t*>(gnm_ctx + 0xb510);
                    if (sub_2165150(staging_tex, 0u, sync_flag) == 0) {
                        const uint32_t slice_idx = (static_cast<uint8_t>(mip) != 0u) ? cur_slice : 0u;
                        sub_2169c30(gnm_ctx, 0u, staging_tex, slice_idx, 0u, &map_info);
                    }
                    face_accum[face] = inv_count;
                    cur_slice += mip;
                }
            }
        } else if (fmt == 0x0au) {
            const uint32_t texel_count = width * width;
            __m128 inv_count = _mm_setzero_ps();
            if (width != 0u) {
                inv_count = _mm_cvtsi32_ss(_mm_setzero_ps(), static_cast<int32_t>(texel_count));
                inv_count = _mm_shuffle_ps(inv_count, inv_count, 0x00);
                inv_count = _mm_div_ps(_mm_load_ps(kVec4One), inv_count);
            }

            struct MapInfo {
                uint8_t* base_ptr;
                uint32_t row_pitch_bytes;
                uint32_t pad;
            } map_info{};

            for (uint64_t face = 0; face < 6u; ++face) {
                const uint32_t sync_flag = *reinterpret_cast<const uint32_t*>(gnm_ctx + 0xb510);
                if (sub_2165150(staging_tex, 0u, sync_flag) == 0) {
                    uint32_t slice_idx = 0u;
                    if (static_cast<uint8_t>(mip) != 0u) {
                        slice_idx = mip * static_cast<uint32_t>(face) + mip_level;
                    }
                    sub_2169c30(gnm_ctx, 0u, staging_tex, slice_idx, 0u, &map_info);
                }

                __m128 sum = _mm_setzero_ps();
                if (width != 0u) {
                    for (uint64_t x = 0; x < width; ++x) {
                        const uint8_t* col_base = map_info.base_ptr + x * 8ULL;
                        for (uint64_t y = 0; y < width; ++y) {
                            const uint32_t byte_off = static_cast<uint32_t>(y) * map_info.row_pitch_bytes;
                            const auto* half_px = reinterpret_cast<const uint16_t*>(col_base + byte_off);

                            const uint32_t f0 = DecodeHalfToFloatBits(half_px[0]);
                            const uint32_t f1 = DecodeHalfToFloatBits(half_px[1]);
                            const uint32_t f2 = DecodeHalfToFloatBits(half_px[2]);
                            const uint32_t f3 = DecodeHalfToFloatBits(half_px[3]);

                            __m128 c0 = _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(f0)));
                            __m128 c1 = _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(f1)));
                            __m128 c2 = _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(f2)));
                            __m128 c3 = _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(f3)));

                            __m128 px = _mm_insert_ps(c0, c1, 0x10);
                            px = _mm_insert_ps(px, c2, 0x20);
                            px = _mm_insert_ps(px, c3, 0x30);
                            sum = _mm_add_ps(sum, px);
                        }
                    }
                }
                face_accum[face] = _mm_mul_ps(sum, inv_count);
            }
        }
    }

    if (staging_tex != nullptr) {
        void* owner_heap = sub_207b720(staging_tex);
        {
            auto* tex_vtbl = *reinterpret_cast<void (***)(void*)>(staging_tex);
            tex_vtbl[0](staging_tex);
        }
        {
            auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
            heap_vtbl[14](owner_heap, staging_tex);
        }
    }

    const __m128 f0 = face_accum[0];
    const __m128 f1 = face_accum[1];
    const __m128 f2 = face_accum[2];
    const __m128 f3 = face_accum[3];
    const __m128 f4 = face_accum[4];
    const __m128 f5 = face_accum[5];

    __m128 avg = _mm_add_ps(f0, _mm_setzero_ps());
    avg = _mm_add_ps(avg, f1);
    avg = _mm_add_ps(avg, f2);
    avg = _mm_add_ps(avg, f3);
    avg = _mm_add_ps(avg, f4);
    avg = _mm_add_ps(avg, f5);
    avg = _mm_mul_ps(avg, _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(kVec4OneSixth))));

    const __m128 half_vec = _mm_load_ps(kVec4Half);
    const __m128 dx = _mm_mul_ps(_mm_sub_ps(f0, f1), half_vec);
    const __m128 dy = _mm_mul_ps(_mm_sub_ps(f2, f3), half_vec);
    const __m128 dz = _mm_mul_ps(_mm_sub_ps(f4, f5), half_vec);

    __m128 row0 = _mm_unpacklo_ps(dx, dy);
    row0 = _mm_movelh_ps(row0, dz);
    __m128 tmp5 = _mm_move_ss(row0, avg);
    row0 = _mm_shuffle_ps(row0, tmp5, 0x24);
    out_basis3[0] = row0;

    __m128 row1 = _mm_shuffle_ps(dx, dy, 0x11);
    row1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x08));
    row1 = _mm_shuffle_ps(row1, dz, 0x14);
    tmp5 = _mm_shuffle_ps(avg, row1, 0x21);
    row1 = _mm_shuffle_ps(row1, tmp5, 0x24);
    out_basis3[1] = row1;

    __m128 row2 = _mm_unpackhi_ps(dx, dy);
    row2 = _mm_shuffle_ps(row2, dz, 0x24);
    __m128 hi_avg = _mm_movehl_ps(row2, avg);
    row2 = _mm_shuffle_ps(row2, hi_avg, 0x24);
    out_basis3[2] = row2;

    return true;
}

// ============================================================================
// 25. 0x21bba50 (147 B) - sub_21bba50
// ============================================================================
bool sub_21bba50(void* tex_wrapper, __m128* out_basis3) {
    if (tex_wrapper != nullptr) {
        auto* vtbl = *reinterpret_cast<uintptr_t**>(tex_wrapper);
        auto fn_get_gnf = reinterpret_cast<const void* (*)(void*)>(vtbl[5]);
        if (fn_get_gnf(tex_wrapper) != nullptr) {
            auto fn_get_type = reinterpret_cast<uint32_t (*)(void*)>(vtbl[6]);
            if (fn_get_type(tex_wrapper) == 1u) {
                auto fn_get_mips = reinterpret_cast<uint32_t (*)(void*)>(vtbl[8]);
                const uint32_t mips = fn_get_mips(tex_wrapper);
                void* const* root = *reinterpret_cast<void* const* const*>(GX_VMA(0x55406c8ULL));
                void* dev_ctx = *root;
                const void* gnf_hdr = fn_get_gnf(tex_wrapper);
                return sub_21bbaf0(dev_ctx, gnf_hdr, mips - 1u, out_basis3);
            }
        }
    }

    const __m128 unit_w = _mm_load_ps(kUnitW);
    _mm_storeu_ps(reinterpret_cast<float*>(out_basis3 + 0), unit_w);
    _mm_storeu_ps(reinterpret_cast<float*>(out_basis3 + 1), unit_w);
    _mm_storeu_ps(reinterpret_cast<float*>(out_basis3 + 2), unit_w);
    return false;
}

// ============================================================================
// 26. 0x21bba30 (19 B) - sub_21bba30
// ============================================================================
__m128 sub_21bba30(const __m128* v) {
    __m128 x0 = _mm_loadu_ps(reinterpret_cast<const float*>(v));
    __m128 x1 = _mm_mul_ps(x0, x0);
    x0 = _mm_shuffle_ps(x1, x0, 0xc8);
    x0 = _mm_shuffle_ps(x1, x0, 0xd4);
    return x0;
}

// ============================================================================
// 27. 0x21bb9c0 (98 B) - sub_21bb9c0
// ============================================================================
__m128* sub_21bb9c0(__m128* out_mat4, const __m128* aabb_min_max, float scale, float margin) {
    const __m128 bmin = _mm_loadu_ps(reinterpret_cast<const float*>(aabb_min_max + 0));
    const __m128 bmax = _mm_loadu_ps(reinterpret_cast<const float*>(aabb_min_max + 1));
    const __m128 half = _mm_load_ps(kVec4Half);

    const __m128 half_ext = _mm_mul_ps(_mm_sub_ps(bmax, bmin), half);
    const __m128 marg_vec = _mm_set1_ps(margin);
    const __m128 scl_vec  = _mm_set1_ps(scale);
    const __m128 diag     = _mm_mul_ps(scl_vec, _mm_add_ps(marg_vec, half_ext));

    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 0), _mm_move_ss(_mm_setzero_ps(), diag));
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 1), _mm_and_ps(diag, _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(kMaskY)))));
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 2), _mm_and_ps(diag, _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(kMaskZ)))));
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 3), _mm_mul_ps(_mm_add_ps(bmin, bmax), half));
    return out_mat4;
}

// ============================================================================
// 28. 0x21bb8f0 (200 B) - sub_21bb8f0
// ============================================================================
__m128* sub_21bb8f0(__m128* out_mat4, uint32_t face_id, const __m128* pos) {
    __m128 col1 = _mm_load_ps(kAxisPosY);
    __m128 col2;

    switch (face_id - 1u) {
        case 0u:
            col2 = _mm_set_ss(-1.0f);
            break;
        case 1u:
            col1 = _mm_load_ps(kAxisNegZ);
            col2 = _mm_load_ps(kAxisPosY);
            break;
        case 2u:
            col1 = _mm_load_ps(kAxisPosZ);
            col2 = _mm_load_ps(kAxisNegY);
            break;
        case 3u:
            col2 = _mm_load_ps(kAxisPosZ);
            break;
        case 4u:
            col2 = _mm_load_ps(kAxisNegZ);
            break;
        default:
            col2 = _mm_set_ss(1.0f);
            break;
    }

    const __m128 c1_yzx = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(col1), 0xc9));
    const __m128 c2_yzx = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(col2), 0xc9));
    __m128 cross = _mm_sub_ps(_mm_mul_ps(col1, c2_yzx), _mm_mul_ps(col2, c1_yzx));
    cross = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(cross), 0x49));

    const __m128 mask_xyz = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(kMaskXYZ)));
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 0), _mm_and_ps(cross, mask_xyz));
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 1), col1);
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 2), col2);
    _mm_storeu_ps(reinterpret_cast<float*>(out_mat4 + 3), _mm_add_ps(_mm_and_ps(mask_xyz, _mm_loadu_ps(reinterpret_cast<const float*>(pos))), _mm_load_ps(kUnitW)));
    return out_mat4;
}

// ============================================================================
// 29. 0x21bb550 (914 B) - sub_21bb550
// ============================================================================
void sub_21bb550(void* pass_ctx, void* target_view, void* param_block) {
    uint8_t* root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    uint8_t* state_mgr = *reinterpret_cast<uint8_t* const*>(root + 0x388);

    uint64_t* shader_slot = *reinterpret_cast<uint64_t* const*>(state_mgr + 0x100);
    if (shader_slot == nullptr || *shader_slot == 0ULL) {
        return;
    }

    void* res_110 = *reinterpret_cast<void* const*>(state_mgr + 0x110);
    if (res_110 == nullptr) {
        return;
    }
    {
        auto* vtbl = *reinterpret_cast<void* (***)(void*)>(res_110);
        if (vtbl[3](res_110) == nullptr) {
            return;
        }
    }

    void* res_108 = *reinterpret_cast<void* const*>(state_mgr + 0x108);
    if (res_108 == nullptr) {
        return;
    }
    {
        auto* vtbl = *reinterpret_cast<void* (***)(void*)>(res_108);
        if (vtbl[3](res_108) == nullptr) {
            return;
        }
    }

    uint8_t* cmd_ctx = *reinterpret_cast<uint8_t**>(pass_ctx);
    uint64_t rt_handle = 0ULL;
    if (*reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(target_view) + 0x60) != 0ULL) {
        auto* tv_vtbl = *reinterpret_cast<uint32_t (***)(void*)>(target_view);
        if (tv_vtbl[8](target_view) != 0u) {
            const uint64_t* ptr60 = *reinterpret_cast<const uint64_t* const*>(
                static_cast<const uint8_t*>(target_view) + 0x60);
            rt_handle = *ptr60;
        }
    }

    sub_21a2580(cmd_ctx, 0u, rt_handle);

    struct DrawHelperObj {
        uint64_t vtable_ptr;
        void*    pass_ctx_ptr;
        void*    shader_ctx_ptr;
    } draw_helper{};

    draw_helper.vtable_ptr   = GX_VMA(0x535f520ULL);
    draw_helper.pass_ctx_ptr = pass_ctx;

    uint8_t* cur_cmd = *reinterpret_cast<uint8_t**>(pass_ctx);
    uint8_t* cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    if (cur_cmd[0x2093] != 0u) {
        void* sh_mgr = *reinterpret_cast<void* const*>(cur_root + 0x390);
        draw_helper.shader_ctx_ptr = sub_26db050(sh_mgr);
        cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    } else {
        const uint32_t idx = *reinterpret_cast<const uint32_t*>(cur_cmd + 0x2098);
        void* const* table = *reinterpret_cast<void* const* const*>(cur_root + 0x3a0);
        draw_helper.shader_ctx_ptr = table[idx];
    }

    sub_2191c30(*reinterpret_cast<void* const*>(cur_root + 0x388), cmd_ctx, 0x0au, 0u);

    const uint64_t param_48 = *reinterpret_cast<const uint64_t*>(
        static_cast<const uint8_t*>(param_block) + 0x48);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x788) = 0ULL;
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x790) = param_48;
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x960) = 0u;

    uint32_t v964 = *reinterpret_cast<const uint32_t*>(cmd_ctx + 0x964);
    if (v964 <= 2u) {
        v964 = 2u;
    }
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x964) = v964;

    uint8_t* gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0x1968) |= 3ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xd128)  = 0ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xd130)  = param_48;

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    uint8_t* presets = *reinterpret_cast<uint8_t* const*>(cur_root + 0x250);
    uint64_t depth_st = *reinterpret_cast<const uint64_t*>(presets + 0x190);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x480) = depth_st;
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x498) = 0xFFFFFFFFu;

    gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbc00) = depth_st;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbca0) = 0ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbc98) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnm_state + 0xbcb0) = 0xFFFFFFFFu;

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    presets  = *reinterpret_cast<uint8_t* const*>(cur_root + 0x250);
    uint64_t blend_st = *reinterpret_cast<const uint64_t*>(presets + 0x1e8);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x210) = blend_st;
    cmd_ctx[0x218] = 0xffu;

    gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbc08) = blend_st;
    gnm_state[0xbcb4] = 0xffu;

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    presets  = *reinterpret_cast<uint8_t* const*>(cur_root + 0x250);
    uint64_t rast_st = *reinterpret_cast<const uint64_t*>(presets + 0x1f8);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x258)    = rast_st;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xb970) = rast_st;

    struct alignas(16) QuadVertex48 {
        __m128   pos;
        __m128   col;
        float    u;
        float    v;
        uint32_t pad0;
        uint32_t pad1;
    };
    alignas(16) QuadVertex48 verts[4]{};
    const __m128 one = _mm_load_ps(kVec4One);

    verts[0].pos = _mm_load_ps(kQuadPos0);
    verts[0].col = one;
    verts[0].u   = 0.0f;
    verts[0].v   = 0.0f;

    verts[1].pos = one;
    verts[1].col = one;
    verts[1].u   = 1.0f;
    verts[1].v   = 0.0f;

    verts[2].pos = _mm_load_ps(kQuadPos2);
    verts[2].col = one;
    verts[2].u   = 0.0f;
    verts[2].v   = 1.0f;

    verts[3].pos = _mm_load_ps(kQuadPos3);
    verts[3].col = one;
    verts[3].u   = 1.0f;
    verts[3].v   = 1.0f;

    {
        auto* vtbl = *reinterpret_cast<void (***)(void*, uint32_t, uint32_t, const void*, uint32_t)>(&draw_helper);
        vtbl[0](&draw_helper, 5u, 4u, verts, 0x30u);
    }

    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x790) = 0ULL;
    const uint32_t nz = (*reinterpret_cast<const uint32_t*>(cmd_ctx + 0x960) != 0u) ? 1u : 0u;
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x960) = nz;

    uint32_t v964_post = *reinterpret_cast<const uint32_t*>(cmd_ctx + 0x964);
    if (v964_post <= 2u) {
        v964_post = 2u;
    }
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x964) = v964_post;

    gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0x1968) |= 2ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xd130)  = 0ULL;

    sub_21a2650(cmd_ctx);
}

// ============================================================================
// 30. 0x21baff0 (1371 B) - sub_21baff0
// ============================================================================
void sub_21baff0(void* pass_ctx, void* resource_view) {
    uint8_t* root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    uint8_t* state_mgr = *reinterpret_cast<uint8_t* const*>(root + 0x388);

    uint64_t* shader_slot = *reinterpret_cast<uint64_t* const*>(state_mgr + 0xb8);
    if (shader_slot == nullptr || *shader_slot == 0ULL) {
        return;
    }

    void* res_c8 = *reinterpret_cast<void* const*>(state_mgr + 0xc8);
    if (res_c8 == nullptr) {
        return;
    }
    {
        auto* vtbl = *reinterpret_cast<void* (***)(void*)>(res_c8);
        if (vtbl[3](res_c8) == nullptr) {
            return;
        }
    }

    void* res_c0 = *reinterpret_cast<void* const*>(state_mgr + 0xc0);
    if (res_c0 == nullptr) {
        return;
    }
    {
        auto* vtbl = *reinterpret_cast<void* (***)(void*)>(res_c0);
        if (vtbl[3](res_c0) == nullptr) {
            return;
        }
    }

    uint8_t* cmd_ctx = *reinterpret_cast<uint8_t**>(pass_ctx);
    const uint32_t count_c8 = *reinterpret_cast<const uint32_t*>(cmd_ctx + 0xc8);
    size_t copy_bytes = static_cast<size_t>(count_c8) * 24u;

    alignas(16) float viewport_buf[48]{};
    if (copy_bytes > sizeof(viewport_buf)) {
        copy_bytes = sizeof(viewport_buf);
    }
    sub_261a1b0(viewport_buf, cmd_ctx + 0xcc, copy_bytes);

    const float inv_w = 1.0f / viewport_buf[2];
    const float inv_h = 1.0f / viewport_buf[3];
    const float neg1_minus_w = -1.0f - inv_w;
    const float neg1_minus_h = -1.0f - inv_h;
    const float pos1_plus_w  =  1.0f + inv_w;
    const float pos1_plus_h  =  1.0f + inv_h;

    alignas(16) __m128 quad_verts[4];
    quad_verts[0] = _mm_setr_ps(pos1_plus_w,  pos1_plus_h,  1.0f, 1.0f);
    quad_verts[1] = _mm_setr_ps(pos1_plus_w,  neg1_minus_h, 1.0f, 1.0f);
    quad_verts[2] = _mm_setr_ps(neg1_minus_w, pos1_plus_h,  1.0f, 1.0f);
    quad_verts[3] = _mm_setr_ps(neg1_minus_w, neg1_minus_h, 1.0f, 1.0f);

    struct DrawHelperObj {
        uint64_t vtable_ptr;
        void*    pass_ctx_ptr;
        void*    shader_ctx_ptr;
    } draw_helper{};

    draw_helper.vtable_ptr   = GX_VMA(0x535f520ULL);
    draw_helper.pass_ctx_ptr = pass_ctx;

    uint8_t* cur_cmd  = *reinterpret_cast<uint8_t**>(pass_ctx);
    uint8_t* cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    if (cur_cmd[0x2093] != 0u) {
        void* sh_mgr = *reinterpret_cast<void* const*>(cur_root + 0x390);
        draw_helper.shader_ctx_ptr = sub_26db050(sh_mgr);
        cur_cmd  = *reinterpret_cast<uint8_t**>(pass_ctx);
        cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    } else {
        const uint32_t idx = *reinterpret_cast<const uint32_t*>(cur_cmd + 0x2098);
        void* const* table = *reinterpret_cast<void* const* const*>(cur_root + 0x3a0);
        draw_helper.shader_ctx_ptr = table[idx];
    }

    sub_2191c30(*reinterpret_cast<void* const*>(cur_root + 0x388), cur_cmd, 7u, 0u);

    uint64_t srv_handle = 0ULL;
    if (resource_view != nullptr) {
        auto* rv_vtbl = *reinterpret_cast<uint64_t (***)(void*)>(resource_view);
        srv_handle = rv_vtbl[3](resource_view);
    }

    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x788) = srv_handle;
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x960) = 0u;
    uint32_t v964 = *reinterpret_cast<const uint32_t*>(cmd_ctx + 0x964);
    if (v964 <= 1u) {
        v964 = 1u;
    }
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x964) = v964;

    uint8_t* gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0x1968) |= 1ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xd128)  = srv_handle;

    // 4x4 matrix inverse of pass_ctx[0xd0 .. 0x100] identically to 0x21bb1fb..0x21bb43f
    const auto* pass_bytes = static_cast<const uint8_t*>(pass_ctx);
    __m128 x0 = _mm_load_ps(reinterpret_cast<const float*>(pass_bytes + 0xd0));
    __m128 x1 = _mm_load_ps(reinterpret_cast<const float*>(pass_bytes + 0xe0));
    __m128 x2 = _mm_load_ps(reinterpret_cast<const float*>(pass_bytes + 0xf0));
    __m128 x3 = _mm_load_ps(reinterpret_cast<const float*>(pass_bytes + 0x100));

    __m128 x4 = _mm_movelh_ps(x0, x1);
    x0 = _mm_movehl_ps(x1, x0);
    x1 = _mm_movelh_ps(x2, x3);
    x2 = _mm_movehl_ps(x3, x2);

    __m128 x15 = _mm_shuffle_ps(x4, x1, 0x88);
    x1 = _mm_shuffle_ps(x4, x1, 0xdd);
    x4 = _mm_shuffle_ps(x0, x2, 0x88);
    x0 = _mm_shuffle_ps(x0, x2, 0xdd);

    __m128 x8  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x4), 0x50));
    __m128 x9  = _mm_movehl_ps(x0, x0);
    __m128 x6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x15), 0x50));
    __m128 x7  = _mm_movehl_ps(x1, x1);
    x2 = _mm_shuffle_ps(x4, x15, 0x88);
    __m128 x5  = _mm_shuffle_ps(x0, x1, 0xdd);

    x8 = _mm_mul_ps(x8, x9);
    __m128 x10 = _mm_mul_ps(x6, x7);
    x9 = _mm_mul_ps(x2, x5);

    __m128 x11 = _mm_movehl_ps(x4, x4);
    x7 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x0), 0x50));
    __m128 x12 = _mm_movehl_ps(x15, x15);
    x6 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x1), 0x50));
    x5 = _mm_shuffle_ps(x4, x15, 0xdd);
    x2 = _mm_shuffle_ps(x0, x1, 0x88);

    x7 = _mm_mul_ps(x11, x7);
    x6 = _mm_mul_ps(x12, x6);
    x2 = _mm_mul_ps(x5, x2);

    x5 = _mm_sub_ps(x8, x7);
    x6 = _mm_sub_ps(x10, x6);
    x2 = _mm_sub_ps(x9, x2);

    x7  = _mm_shuffle_ps(x5, x2, 0x5d);
    x8  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x1), 0x49));
    x9  = _mm_shuffle_ps(x7, x5, 0x32);
    x10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x15), 0x12));
    x11 = _mm_shuffle_ps(x7, x5, 0x99);
    x7  = _mm_shuffle_ps(x6, x2, 0xfd);
    x12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x0), 0x49));
    __m128 x13 = _mm_shuffle_ps(x7, x6, 0x32);
    __m128 x14 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x4), 0x12));
    x7  = _mm_shuffle_ps(x7, x6, 0x99);

    x8  = _mm_mul_ps(x8, x9);
    x9  = _mm_mul_ps(x10, x11);
    x10 = _mm_mul_ps(x12, x13);
    x11 = _mm_mul_ps(x14, x7);

    x7  = _mm_shuffle_ps(x5, x2, 0x04);
    x12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x1), 0x9e));
    x13 = _mm_shuffle_ps(x5, x7, 0x93);
    x14 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x15), 0x7b));
    x3  = _mm_shuffle_ps(x5, x7, 0x26);
    x7  = _mm_shuffle_ps(x6, x2, 0xa4);

    x12 = _mm_mul_ps(x12, x13);
    x13 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x0), 0x9e));
    x14 = _mm_mul_ps(x14, x3);
    x3  = _mm_shuffle_ps(x6, x7, 0x93);
    x13 = _mm_mul_ps(x13, x3);
    x3  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x4), 0x7b));
    x7  = _mm_shuffle_ps(x6, x7, 0x26);
    x3  = _mm_mul_ps(x3, x7);

    x7  = _mm_sub_ps(x8, x12);
    x8  = _mm_sub_ps(x9, x14);
    x9  = _mm_sub_ps(x10, x13);
    x12 = _mm_sub_ps(x11, x3);

    x10 = _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(x2), _mm_castps_si128(x5), 8));
    x11 = _mm_shuffle_ps(x5, x2, 0x4c);
    x5  = _mm_movehl_ps(x2, x6);
    x2  = _mm_shuffle_ps(x6, x2, 0xec);

    x1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x1), 0x33));
    x6 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x10), 0x2c));
    x1 = _mm_mul_ps(x1, x6);

    x6 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x15), 0x8d));
    x3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x11), 0x93));
    x3 = _mm_mul_ps(x6, x3);

    x0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x0), 0x33));
    x5 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x5), 0x2c));
    x0 = _mm_mul_ps(x0, x5);

    x4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x4), 0x8d));
    x2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x2), 0x93));
    x2 = _mm_mul_ps(x4, x2);

    x4 = _mm_sub_ps(x7, x1);
    x1 = _mm_add_ps(x1, x7);
    x5 = _mm_add_ps(x3, x8);
    x3 = _mm_sub_ps(x8, x3);
    x6 = _mm_sub_ps(x9, x0);
    x0 = _mm_add_ps(x0, x9);
    x7 = _mm_add_ps(x2, x12);
    x2 = _mm_sub_ps(x12, x2);

    x1 = _mm_shuffle_ps(x1, x4, 0xd8);
    x3 = _mm_shuffle_ps(x3, x5, 0xd8);
    x0 = _mm_shuffle_ps(x0, x6, 0xd8);
    x2 = _mm_shuffle_ps(x2, x7, 0xd8);

    x1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x1), 0xd8));
    x4 = _mm_mul_ps(x15, x1);
    x4 = _mm_hadd_ps(x4, x4);
    x4 = _mm_hadd_ps(x4, x4);
    x4 = _mm_div_ps(_mm_load_ps(kVec4One), x4);

    x3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x3), 0xd8));
    x0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x0), 0xd8));
    x2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(x2), 0xd8));

    x1 = _mm_mul_ps(x4, x1);
    x3 = _mm_mul_ps(x4, x3);
    x0 = _mm_mul_ps(x4, x0);
    x2 = _mm_mul_ps(x4, x2);

    alignas(16) __m128 inv_mat[4];
    inv_mat[0] = x1;
    inv_mat[1] = x3;
    inv_mat[2] = x0;
    inv_mat[3] = x2;

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    sub_2192170(*reinterpret_cast<void* const*>(cur_root + 0x388), cur_cmd, inv_mat, 0u);

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    uint8_t* presets = *reinterpret_cast<uint8_t* const*>(cur_root + 0x250);
    uint64_t depth_st = *reinterpret_cast<const uint64_t*>(presets);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x480) = depth_st;
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x498) = 0xFFFFFFFFu;

    gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbc00) = depth_st;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbca0) = 0ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbc98) = 0ULL;
    *reinterpret_cast<uint32_t*>(gnm_state + 0xbcb0) = 0xFFFFFFFFu;

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    presets  = *reinterpret_cast<uint8_t* const*>(cur_root + 0x250);
    uint64_t blend_st = *reinterpret_cast<const uint64_t*>(presets + 0x1d8);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x210) = blend_st;
    cmd_ctx[0x218] = 0xffu;

    gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0xbc08) = blend_st;
    gnm_state[0xbcb4] = 0xffu;

    cur_root = *reinterpret_cast<uint8_t* const*>(GX_VMA(0x55406c8ULL));
    presets  = *reinterpret_cast<uint8_t* const*>(cur_root + 0x250);
    uint64_t rast_st = *reinterpret_cast<const uint64_t*>(presets + 0x1f8);
    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x258)    = rast_st;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xb970) = rast_st;

    {
        auto* vtbl = *reinterpret_cast<void (***)(void*, uint32_t, uint32_t, const void*, uint32_t)>(&draw_helper);
        vtbl[0](&draw_helper, 5u, 4u, quad_verts, 0x10u);
    }

    *reinterpret_cast<uint64_t*>(cmd_ctx + 0x788) = 0ULL;
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x960) = 0u;
    uint32_t v964_post = *reinterpret_cast<const uint32_t*>(cmd_ctx + 0x964);
    if (v964_post <= 1u) {
        v964_post = 1u;
    }
    *reinterpret_cast<uint32_t*>(cmd_ctx + 0x964) = v964_post;
    gnm_state = *reinterpret_cast<uint8_t**>(cmd_ctx + 0x08);
    *reinterpret_cast<uint64_t*>(gnm_state + 0x1968) |= 1ULL;
    *reinterpret_cast<uint64_t*>(gnm_state + 0xd128)  = 0ULL;
}

// ============================================================================
// 31. 0x21bac20 (972 B) - sub_21bac20
// ============================================================================
bool sub_21bac20(void** out_tex, void* unused_rsi, void* custom_alloc, void* custom_heap2) {
    void* alloc1 = custom_alloc;
    if (alloc1 == nullptr) {
        alloc1 = *reinterpret_cast<void* const*>(GX_VMA(0x5540680ULL));
    }
    void* default_heap2 = *reinterpret_cast<void* const*>(GX_VMA(0x5540698ULL));

    // Access TLS linear heap at fs:0x0 + *(uint64_t*)0x53e4568
    uint64_t tls_base = 0ULL;
    tls_base = gx_guest_tls_base();
    const uint64_t tls_off = static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL));
    uint8_t* linear_heap = *reinterpret_cast<uint8_t**>(tls_base + tls_off);
    if (linear_heap == nullptr) {
        sub_207d250();
        tls_base = gx_guest_tls_base();
        linear_heap = *reinterpret_cast<uint8_t**>(tls_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
        if (linear_heap == nullptr) {
            sub_207d250();
            tls_base = gx_guest_tls_base();
            linear_heap = *reinterpret_cast<uint8_t**>(tls_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
        }
    }

    uint8_t* saved_mark = *reinterpret_cast<uint8_t**>(linear_heap + 0x10);
    const uintptr_t cur_addr = reinterpret_cast<uintptr_t>(saved_mark);
    const uintptr_t align_pad = (0ULL - cur_addr) & 0x0fULL;
    uint8_t* next_mark = saved_mark + align_pad + 0x1000ULL;
    uint8_t* noise_buf = nullptr;
    if (next_mark <= *reinterpret_cast<uint8_t**>(linear_heap + 0x18)) {
        noise_buf = saved_mark + align_pad;
        *reinterpret_cast<uint8_t**>(linear_heap + 0x10) = next_mark;
    }

    struct DataSliceDesc {
        uint8_t* ptr;
        uint32_t row_pitch;
        uint32_t slice_pitch;
    } slice_desc{noise_buf, 0x10u, 0x100u};

    struct TimeVal {
        int64_t tv_sec;
        int64_t tv_usec;
    } tv{};

    struct PrngBlock {
        uint32_t seed_val;
        alignas(8) uint8_t state[0x9c8];
    } prng{};
    prng.seed_val = 0x1571u;

    sub_2686110(prng.state, 0x1571u);
    sub_2bbe728(&tv, nullptr);
    const uint32_t usec_seed = static_cast<uint32_t>(tv.tv_sec) * 1000000u + static_cast<uint32_t>(tv.tv_usec);
    prng.seed_val = usec_seed;
    sub_2686170(prng.state);

    if (noise_buf != nullptr) {
        uint8_t* slice_ptr = noise_buf + 0x0f;
        for (uint64_t z = 0; z < 16u; ++z) {
            uint8_t* row_ptr = slice_ptr;
            for (uint64_t y = 0; y < 16u; ++y) {
                row_ptr[-15] = sub_29add90(prng.state);
                row_ptr[-14] = sub_29add90(prng.state);
                row_ptr[-13] = sub_29add90(prng.state);
                row_ptr[-12] = sub_29add90(prng.state);
                row_ptr[-11] = sub_29add90(prng.state);
                row_ptr[-10] = sub_29add90(prng.state);
                row_ptr[-9]  = sub_29add90(prng.state);
                row_ptr[-8]  = sub_29add90(prng.state);
                row_ptr[-7]  = sub_29add90(prng.state);
                row_ptr[-6]  = sub_29add90(prng.state);
                row_ptr[-5]  = sub_29add90(prng.state);
                row_ptr[-4]  = sub_29add90(prng.state);
                row_ptr[-3]  = sub_29add90(prng.state);
                row_ptr[-2]  = sub_29add90(prng.state);
                row_ptr[-1]  = sub_29add90(prng.state);
                row_ptr[0]   = sub_29add90(prng.state);
                row_ptr += 16;
            }
            slice_ptr += 0x100;
        }
    }

    auto* alloc_vtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(alloc1);
    uint8_t* tex_obj = static_cast<uint8_t*>(alloc_vtbl[11](alloc1, 0x60u, 0x10u));
    bool ok = false;
    void* result_tex = nullptr;

    if (tex_obj != nullptr) {
        void* heap2 = (custom_heap2 != nullptr) ? custom_heap2 : default_heap2;
        *reinterpret_cast<uint32_t*>(tex_obj + 0x08) = 0u;
        _mm_storeu_ps(reinterpret_cast<float*>(tex_obj + 0x20), _mm_load_ps(kVec4One));
        *reinterpret_cast<uint64_t*>(tex_obj + 0x30) = 0ULL;
        *reinterpret_cast<uint64_t*>(tex_obj + 0x00) = GX_VMA(0x53b62c0ULL);
        *reinterpret_cast<uint32_t*>(tex_obj + 0x58) = 0u;
        *reinterpret_cast<uint64_t*>(tex_obj + 0x50) = 0ULL;
        *reinterpret_cast<uint64_t*>(tex_obj + 0x48) = 0ULL;
        *reinterpret_cast<uint64_t*>(tex_obj + 0x40) = 0ULL;
        *reinterpret_cast<uint32_t*>(tex_obj + 0x10) = 0x3f800000u;

        sub_217fba0(tex_obj, 0x10u, 0x10u, 0x10u, 0x3du, 1u, &slice_desc, 0u, alloc1, heap2);
        ok = true;
        result_tex = tex_obj;
    }

    *out_tex = result_tex;
    sub_2686260(prng.state);

    tls_base = gx_guest_tls_base();
    linear_heap = *reinterpret_cast<uint8_t**>(tls_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    if (linear_heap == nullptr) {
        sub_207d250();
        tls_base = gx_guest_tls_base();
        linear_heap = *reinterpret_cast<uint8_t**>(tls_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    }

    uint8_t* heap_start = *reinterpret_cast<uint8_t**>(linear_heap + 0x00);
    uint8_t* heap_end   = *reinterpret_cast<uint8_t**>(linear_heap + 0x08);
    if (heap_start > saved_mark || heap_end < saved_mark) {
        sub_20b55b0(
            reinterpret_cast<const char*>(GX_VMA(0x493c62dULL)),
            0x233u,
            reinterpret_cast<const char*>(GX_VMA(0x493c690ULL)));
    }
    if (*reinterpret_cast<uint8_t**>(linear_heap + 0x10) > saved_mark) {
        *reinterpret_cast<uint8_t**>(linear_heap + 0x10) = saved_mark;
    }

    return ok;
}

// ============================================================================
// 32. 0x21bac10 (6 B) - sub_21bac10
// ============================================================================
uint32_t sub_21bac10() {
    return 0x28u;
}

// ============================================================================
// 33. 0x21ba9a0 (610 B) - sub_21ba9a0
// ============================================================================
void sub_21ba9a0(void* self_unused, void* handle_pair, void* custom_heap) {
    auto* pair_ptrs = static_cast<void**>(handle_pair);
    void* target_obj = pair_ptrs[1];
    if (target_obj == nullptr) {
        return;
    }

    void* list_node = pair_ptrs[0];
    void* singleton = *reinterpret_cast<void* const*>(GX_VMA(0x56a6c38ULL));

    if (singleton == nullptr) {
        uint8_t* guard1 = reinterpret_cast<uint8_t*>(GX_VMA(0x56a5740ULL));
        if (*guard1 == 0u) {
            if (sub_2bbe538(guard1) != 0) {
                *reinterpret_cast<uint8_t*>(GX_VMA(0x56a573fULL)) = 0u;
                sub_2bbe558(guard1);
            }
        }
        if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x56a573fULL)) == 0u) {
            const uintptr_t raw_mtx = GX_VMA(0x56a5720ULL);
            const uintptr_t align1  = (0ULL - raw_mtx) & 7ULL;
            auto* mtx_base = reinterpret_cast<uint8_t*>(raw_mtx + align1);
            mtx_base[0x10] = 0u;
            *reinterpret_cast<uint64_t*>(mtx_base) = GX_VMA(0x52b6230ULL);
            if (sub_207ee80(mtx_base) == 0u) {
                if (sub_2083fe0(mtx_base + 0x08) == 0) {
                    mtx_base[0x10] = 1u;
                }
            }
            *reinterpret_cast<uint8_t*>(GX_VMA(0x56a573fULL)) = 1u;
        }

        singleton = *reinterpret_cast<void* const*>(GX_VMA(0x56a6c38ULL));
        if (singleton == nullptr) {
            uint8_t* guard2 = reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c98ULL));
            if (*guard2 == 0u) {
                if (sub_2bbe538(guard2) != 0) {
                    sub_2bbe558(guard2);
                }
            }

            const uintptr_t raw_inst = GX_VMA(0x56a6c40ULL);
            const uintptr_t align2   = (0ULL - raw_inst) & 7ULL;
            auto* inst_base = reinterpret_cast<uint8_t*>(raw_inst + align2);
            *reinterpret_cast<uint64_t*>(inst_base + 0x00) = GX_VMA(0x53b3ba0ULL);
            *reinterpret_cast<uint64_t*>(inst_base + 0x10) = 0ULL;
            *reinterpret_cast<uint64_t*>(inst_base + 0x08) = 0ULL;

            void** global_heap_slot = reinterpret_cast<void**>(GX_VMA(0x5540668ULL));
            void* heap = *global_heap_slot;
            if (heap == nullptr) {
                heap = static_cast<uint8_t*>(sub_207bbf0()) + 0x458;
                *global_heap_slot = heap;
            }

            uint64_t heap_flags = 0ULL;
            {
                auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(heap);
                h_vtbl[4](&heap_flags, heap, 0u);
            }
            if ((heap_flags & 0x20ULL) == 0ULL) {
                sub_20b55b0(nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
            }

            *reinterpret_cast<uint64_t*>(inst_base + 0x30) = 0ULL;
            *reinterpret_cast<uint64_t*>(inst_base + 0x28) = 0ULL;
            *reinterpret_cast<uint64_t*>(inst_base + 0x20) = 0ULL;
            *reinterpret_cast<void**>(inst_base + 0x38)    = heap;
            *reinterpret_cast<uint64_t*>(inst_base + 0x00) = GX_VMA(0x53b6d00ULL);
            *reinterpret_cast<uint64_t*>(inst_base + 0x48) = 0ULL;
            *reinterpret_cast<uint64_t*>(inst_base + 0x40) = 0ULL;

            singleton = inst_base;
            *reinterpret_cast<void**>(GX_VMA(0x56a6c38ULL)) = singleton;
        }
    }

    void* matched_obj = nullptr;
    for (void* cur = list_node; cur != nullptr;
         cur = *reinterpret_cast<void**>(static_cast<uint8_t*>(cur) + 0x08)) {
        if (cur == singleton) {
            matched_obj = target_obj;
            break;
        }
    }

    if (matched_obj != nullptr) {
        void* heap_to_use = custom_heap;
        if (heap_to_use == nullptr) {
            heap_to_use = sub_207b720(matched_obj);
        }
        {
            auto* obj_vtbl = *reinterpret_cast<void (***)(void*)>(matched_obj);
            obj_vtbl[1](matched_obj);
        }
        {
            auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(heap_to_use);
            heap_vtbl[14](heap_to_use, matched_obj);
        }
    }

    pair_ptrs[1] = nullptr;
}

// ============================================================================
// 34. 0x21ba990 (3 B) - sub_21ba990
// ============================================================================
uint32_t sub_21ba990() {
    return 0u;
}

// ============================================================================
// 35. 0x21ba980 (8 B) - sub_21ba980
// ============================================================================
void* sub_21ba980() {
    return reinterpret_cast<void*>(GX_VMA(0x56a6ca0ULL));
}

// ============================================================================
// 36. 0x21ba970 (8 B) - sub_21ba970
// ============================================================================
void* sub_21ba970() {
    return reinterpret_cast<void*>(GX_VMA(0x56a6ca1ULL));
}

// ============================================================================
// 37. 0x21ba960 (8 B) - sub_21ba960
// ============================================================================
void* sub_21ba960() {
    return reinterpret_cast<void*>(GX_VMA(0x56a6ca2ULL));
}

// ============================================================================
// 38. 0x21ba950 (8 B) - sub_21ba950
// ============================================================================
void* sub_21ba950() {
    return reinterpret_cast<void*>(GX_VMA(0x56a6ca3ULL));
}

// ============================================================================
// 39. 0x21ba940 (5 B) - sub_21ba940
// ============================================================================
void* sub_21ba940(const void* self) {
    return *reinterpret_cast<void* const*>(static_cast<const uint8_t*>(self) + 0x48);
}

// ============================================================================
// 40. 0x21ba930 (5 B) - sub_21ba930
// ============================================================================
void* sub_21ba930(const void* self) {
    return *reinterpret_cast<void* const*>(static_cast<const uint8_t*>(self) + 0x40);
}

// ============================================================================
// 41. 0x21ba910 (28 B) - sub_21ba910
// ============================================================================
void sub_21ba910(void* self) {
    sub_20b5a60(self);
    sub_2bbe588(self);
}

// ============================================================================
// 42. 0x21ba900 (5 B) - sub_21ba900
// ============================================================================
void sub_21ba900(void* self) {
    sub_20b5a60(self);
}

// ============================================================================
// 43. 0x21ba8f0 (5 B) - sub_21ba8f0
// ============================================================================
void* sub_21ba8f0(const void* self) {
    return *reinterpret_cast<void* const*>(static_cast<const uint8_t*>(self) + 0x10);
}

// ============================================================================
// 44. 0x21ba440 (61 B) - sub_21ba440
// ============================================================================
void sub_21ba300(void* self);

void* sub_21ba440(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    auto* vtbl = *reinterpret_cast<void* (***)(void*)>(self);
    void* inner_tex = vtbl[12](self);
    if (inner_tex == nullptr) {
        return nullptr;
    }

    sub_21ba300(self);
    void* alias_view = *reinterpret_cast<void**>(base + 0x18);
    if (alias_view != nullptr) {
        return alias_view;
    }

    auto* inner_vtbl = *reinterpret_cast<void* (***)(void*)>(inner_tex);
    return inner_vtbl[3](inner_tex);
}

// ============================================================================
// 45. 0x21ba300 (319 B) - sub_21ba300
// ============================================================================
void sub_21ba300(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    if (base[0x21] != 0u) {
        return;
    }

    auto* vtbl = *reinterpret_cast<void* (***)(void*)>(self);
    void* inner_tex = vtbl[12](self);
    if (inner_tex == nullptr) {
        return;
    }

    auto* inner_vtbl = *reinterpret_cast< uintptr_t** >(inner_tex);
    auto fn_get_gnm = reinterpret_cast<uint8_t* (*)(void*)>(inner_vtbl[3]);
    if (fn_get_gnm(inner_tex) == nullptr) {
        return;
    }

    uint8_t* gnm_tex = fn_get_gnm(inner_tex);
    struct ViewDesc {
        uint64_t q30;
        uint64_t q38;
        uint64_t q40;
    } desc{};
    desc.q30 = *reinterpret_cast<const uint64_t*>(gnm_tex + 0x30);
    desc.q38 = *reinterpret_cast<const uint64_t*>(gnm_tex + 0x38);
    desc.q40 = *reinterpret_cast<const uint64_t*>(gnm_tex + 0x40);

    const int32_t fmt = static_cast<int32_t>(desc.q30 & 0xFFFFFFFFULL);
    int32_t linear_fmt = fmt;
    if (fmt <= 0x47) {
        if (fmt == 0x1d) {
            linear_fmt = 0x1c;
        }
    } else if (fmt <= 0x62) {
        if (fmt <= 0x5a) {
            if (fmt == 0x48) {
                linear_fmt = 0x47;
            } else if (fmt == 0x4b) {
                linear_fmt = 0x4a;
            } else if (fmt == 0x4e) {
                linear_fmt = 0x4d;
            }
        } else {
            if (fmt == 0x5b) {
                linear_fmt = 0x57;
            } else if (fmt == 0x5d) {
                linear_fmt = 0x58;
            }
        }
    } else if (fmt == 0x63) {
        linear_fmt = 0x62;
    }

    if (fmt != linear_fmt) {
        desc.q30 = (desc.q30 & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(linear_fmt);
        void* const* root = *reinterpret_cast<void* const* const*>(GX_VMA(0x55406c8ULL));
        void* dev_ctx = *root;
        auto fn_get_gnf = reinterpret_cast<void* (*)(void*)>(inner_vtbl[5]);
        void* base_res = fn_get_gnf(inner_tex);

        void* created_view = nullptr;
        if (sub_21669a0(dev_ctx, &created_view, base_res, &desc, 0u) == 0) {
            *reinterpret_cast<void**>(base + 0x18) = created_view;
            base[0x20] = 1u;
        }
    }
    base[0x21] = 1u;
}

// ============================================================================
// 46. 0x21ba280 (126 B) - sub_21ba280
// ============================================================================
void sub_21ba280(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    if (base[0x20] != 0u) {
        void* alias_view = *reinterpret_cast<void**>(base + 0x18);
        if (alias_view != nullptr) {
            void* owner_heap = sub_207b720(alias_view);
            {
                auto* vtbl = *reinterpret_cast<void (***)(void*)>(alias_view);
                vtbl[0](alias_view);
            }
            {
                auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
                heap_vtbl[14](owner_heap, alias_view);
            }
            *reinterpret_cast<void**>(base + 0x18) = nullptr;
        }
        base[0x20] = 0u;
    }

    uint8_t* ref_obj = *reinterpret_cast<uint8_t**>(base + 0x10);
    if (ref_obj != nullptr) {
        const int32_t refs = __atomic_load_n(reinterpret_cast<const int32_t*>(ref_obj + 0x08), __ATOMIC_RELAXED);
        if (refs != 0) {
            const int32_t prev = __atomic_fetch_sub(reinterpret_cast<int32_t*>(ref_obj + 0x08), 1, __ATOMIC_ACQ_REL);
            if (prev <= 1) {
                auto* vtbl = *reinterpret_cast<void (***)(void*)>(ref_obj);
                vtbl[0](ref_obj);
            }
        }
        *reinterpret_cast<void**>(base + 0x10) = nullptr;
    }
    base[0x21] = 0u;
}

// ============================================================================
// 47. 0x21ba240 (56 B) - sub_21ba240
// ============================================================================
uint32_t sub_21ba240(void* self, uint32_t slot, void* target_ctx) {
    auto* vtbl = *reinterpret_cast<void* (***)(void*)>(self);
    void* res = vtbl[13](self);
    if (res == nullptr) {
        return 0u;
    }
    auto* ctx_vtbl = *reinterpret_cast<void (***)(void*, uint32_t, void*)>(target_ctx);
    ctx_vtbl[31](target_ctx, slot, res);
    return 1u;
}

// ============================================================================
// 48. 0x21ba200 (52 B) - sub_21ba200
// ============================================================================
uint32_t sub_21ba200(void* self) {
    auto* vtbl = *reinterpret_cast<uintptr_t**>(self);
    auto fn_is_valid = reinterpret_cast<uint32_t (*)(void*)>(vtbl[9]);
    if (fn_is_valid(self) == 0u) {
        return 0u;
    }
    auto fn_get_inner = reinterpret_cast<void* (*)(void*)>(vtbl[12]);
    void* inner = fn_get_inner(self);
    auto* inner_vtbl = *reinterpret_cast<uint32_t (***)(void*)>(inner);
    return inner_vtbl[8](inner);
}

// ============================================================================
// 49. 0x21ba1c0 (63 B) - sub_21ba1c0
// ============================================================================
uint32_t sub_21ba1c0(void* self) {
    auto* vtbl = *reinterpret_cast<void* (***)(void*)>(self);
    void* inner = vtbl[12](self);
    if (inner == nullptr) {
        return 0u;
    }
    auto* inner_vtbl = *reinterpret_cast<uintptr_t**>(inner);
    auto fn_check = reinterpret_cast<uint8_t (*)(void*)>(inner_vtbl[14]);
    if (fn_check(inner) == 0u) {
        return 0u;
    }
    auto fn_get_gnm = reinterpret_cast<const uint8_t* (*)(void*)>(inner_vtbl[3]);
    const uint8_t* gnm = fn_get_gnm(inner);
    if (gnm == nullptr) {
        return 0u;
    }
    return *reinterpret_cast<const uint32_t*>(gnm + 0x30);
}

// ============================================================================
// 50. 0x21ba180 (59 B) - sub_21ba180
// ============================================================================
int32_t sub_21ba180(void* self) {
    auto* vtbl = *reinterpret_cast<void* (***)(void*)>(self);
    void* inner = vtbl[12](self);
    if (inner == nullptr) {
        return -1;
    }
    auto* inner_vtbl = *reinterpret_cast<int32_t (***)(void*)>(inner);
    const int32_t kind = inner_vtbl[6](inner);
    if (kind == 0) {
        return 0;
    }
    if (kind == 1) {
        return 2;
    }
    return -1;
}

// ============================================================================
// 51. 0x21ba140 (58 B) - sub_21ba140
// ============================================================================
uint32_t sub_21ba140(void* self, uint32_t level) {
    auto* vtbl = *reinterpret_cast<uintptr_t**>(self);
    auto fn_is_valid = reinterpret_cast<uint32_t (*)(void*)>(vtbl[9]);
    if (fn_is_valid(self) == 0u) {
        return 0u;
    }
    auto fn_get_inner = reinterpret_cast<void* (*)(void*)>(vtbl[12]);
    void* inner = fn_get_inner(self);
    auto* inner_vtbl = *reinterpret_cast<uint32_t (***)(void*, uint32_t)>(inner);
    return inner_vtbl[11](inner, level);
}

// ============================================================================
// 52. 0x21ba100 (58 B) - sub_21ba100
// ============================================================================
uint32_t sub_21ba100(void* self, uint32_t level) {
    auto* vtbl = *reinterpret_cast<uintptr_t**>(self);
    auto fn_is_valid = reinterpret_cast<uint32_t (*)(void*)>(vtbl[9]);
    if (fn_is_valid(self) == 0u) {
        return 0u;
    }
    auto fn_get_inner = reinterpret_cast<void* (*)(void*)>(vtbl[12]);
    void* inner = fn_get_inner(self);
    auto* inner_vtbl = *reinterpret_cast<uint32_t (***)(void*, uint32_t)>(inner);
    return inner_vtbl[10](inner, level);
}

// ============================================================================
// 53. 0x21ba0c0 (58 B) - sub_21ba0c0
// ============================================================================
uint32_t sub_21ba0c0(void* self, uint32_t level) {
    auto* vtbl = *reinterpret_cast<uintptr_t**>(self);
    auto fn_is_valid = reinterpret_cast<uint32_t (*)(void*)>(vtbl[9]);
    if (fn_is_valid(self) == 0u) {
        return 0u;
    }
    auto fn_get_inner = reinterpret_cast<void* (*)(void*)>(vtbl[12]);
    void* inner = fn_get_inner(self);
    auto* inner_vtbl = *reinterpret_cast<uint32_t (***)(void*, uint32_t)>(inner);
    return inner_vtbl[9](inner, level);
}

// ============================================================================
// 54. 0x21ba020 (157 B) - sub_21ba020
// ============================================================================
void sub_21ba020(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(base) = GX_VMA(0x53b6c80ULL);

    if (base[0x20] != 0u) {
        void* alias_view = *reinterpret_cast<void**>(base + 0x18);
        if (alias_view != nullptr) {
            void* owner_heap = sub_207b720(alias_view);
            {
                auto* vtbl = *reinterpret_cast<void (***)(void*)>(alias_view);
                vtbl[0](alias_view);
            }
            {
                auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
                heap_vtbl[14](owner_heap, alias_view);
            }
            *reinterpret_cast<void**>(base + 0x18) = nullptr;
        }
        base[0x20] = 0u;
    }

    uint8_t* ref_obj = *reinterpret_cast<uint8_t**>(base + 0x10);
    if (ref_obj != nullptr) {
        const int32_t refs = __atomic_load_n(reinterpret_cast<const int32_t*>(ref_obj + 0x08), __ATOMIC_RELAXED);
        if (refs != 0) {
            const int32_t prev = __atomic_fetch_sub(reinterpret_cast<int32_t*>(ref_obj + 0x08), 1, __ATOMIC_ACQ_REL);
            if (prev <= 1) {
                auto* vtbl = *reinterpret_cast<void (***)(void*)>(ref_obj);
                vtbl[0](ref_obj);
            }
        }
        *reinterpret_cast<void**>(base + 0x10) = nullptr;
    }
    base[0x21] = 0u;

    const uint32_t flags = *reinterpret_cast<const uint32_t*>(base + 0x08) & 0xBFFFFFFFu;
    *reinterpret_cast<uint64_t*>(base) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(base + 0x08) = flags;
}

// ============================================================================
// 55. 0x21b9f90 (131 B) - sub_21b9f90
// ============================================================================
void sub_21b9f90(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(base) = GX_VMA(0x53b6c80ULL);

    if (base[0x20] != 0u) {
        void* alias_view = *reinterpret_cast<void**>(base + 0x18);
        if (alias_view != nullptr) {
            void* owner_heap = sub_207b720(alias_view);
            {
                auto* vtbl = *reinterpret_cast<void (***)(void*)>(alias_view);
                vtbl[0](alias_view);
            }
            {
                auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
                heap_vtbl[14](owner_heap, alias_view);
            }
            *reinterpret_cast<void**>(base + 0x18) = nullptr;
        }
        base[0x20] = 0u;
    }

    uint8_t* ref_obj = *reinterpret_cast<uint8_t**>(base + 0x10);
    if (ref_obj != nullptr) {
        const int32_t refs = *reinterpret_cast<const int32_t*>(ref_obj + 0x08);
        if (refs != 0) {
            *reinterpret_cast<int32_t*>(ref_obj + 0x08) = refs - 1;
            if (refs <= 1) {
                auto* vtbl = *reinterpret_cast<void (***)(void*)>(ref_obj);
                vtbl[0](ref_obj);
            }
        }
    }

    sub_2bbe588(self);
}

} // extern "C"
