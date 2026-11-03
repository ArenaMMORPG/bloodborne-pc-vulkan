// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 04: SceneRenderer (Tier 8/10 CPU)
// File: GXSkinningBufferUploaderAndFlverModelInstance.cpp
// Range: 0x21c4b50 .. 0x21c9a70 (55 functions, 19,267 bytes)
// Architecture: x86_64 PS4 (Orbis OS / FreeBSD 9.0) -> Native C++20
// Decompilation Order: Strict Right-to-Left (high VMA -> low VMA)
// Policy: 100% Bitwise-Accurate, Zero Stubs, Complete Structural Reconstruction
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <atomic>
#include <immintrin.h>

namespace Bloodborne::SceneRenderer {

// ----------------------------------------------------------------------------
// External Engine Entry Points & Runtime Globals
// ----------------------------------------------------------------------------
extern "C" {
    // System / Memory / TLS / Exception / Logging
    void*    sub_2ba2b10();                                                    // 0x2ba2b10: DLKR::GetDefaultHeapAllocator()
    void     sub_20b55b0(void* ctx, uint32_t code, const char* msg, ...);      // 0x20b55b0: DLUT::ReportFatalError
    void     sub_2bbe4f8();                                                    // 0x2bbe4f8: __stack_chk_fail
    void     sub_2bbe588(void* ptr);                                           // 0x2bbe588: operator delete(void*)
    void*    sub_2bbe688(void* dst, int val, size_t len);                      // 0x2bbe688: memset
    void     sub_2bbe978(void* tls_exc, uint32_t flag);                        // 0x2bbe978: ThrowBadAlloc / TransactionFailed
    size_t   sub_2bbf228(const char16_t* str);                                 // 0x2bbf228: wcslen (UTF-16LE)
    int      sub_2bbf268(const char16_t* s1, const char16_t* s2, size_t n);    // 0x2bbf268: wmemcmp (UTF-16LE)
    void*    sub_261a1b0(void* dst, const void* src, size_t len);              // 0x261a1b0: memcpy

    // GX Constant Buffer / Command Context / Heap Helpers
    void*    sub_207b720(void* ptr);                                           // 0x207b720: DLKR::FindHeapForBlock(void*)
    void     sub_207d250();                                                    // 0x207d250: InitializeThreadLocalTempHeap()
    void*    sub_e30f30(void* custom_alloc, uint32_t byte_size);               // 0x0e30f30: AllocateSkinningConstantBuffer
    void     sub_e31100(void* custom_alloc, void* buf);                        // 0x0e31100: FreeSkinningConstantBuffer
    int32_t  sub_2165c10(void* dev, void** out_buf, const void* buf_desc,
                         const void* init_data, uint32_t a5, uint32_t a6, void* a7); // 0x2165c10: GXDevice::CreateConstantBuffer
    void     sub_2168a20(void* cmd_ctx, void* sub_ctx, void* cb_handle,
                         uint32_t offset, uint32_t size, const void* src_data,
                         uint32_t flag0, uint32_t flag1);                      // 0x2168a20: GXCommandContext::UpdateConstantBuffer
    uint8_t  sub_21cba80(void* batch_mgr, void* skin_target,
                         const void* matrices, const void* prev_matrices);     // 0x21cba80: GXSkinningBatchManager_EnqueueTarget
    void     sub_21c9c00(void* begin, void* end, int64_t count);               // 0x21c9c00: DestroySkinningTargetRange
    void     sub_292dcb0(void* vec, size_t new_cap);                           // 0x292dcb0: std::vector<void*>::reserve
    void     sub_292ccf0(void* vec, void** elem_ptr);                          // 0x292ccf0: std::vector<void*>::push_back

    // String & Inspector Helpers
    void     sub_231d3a0(void* out_wstr, const char* utf8_str);                // 0x231d3a0: ConvertUtf8ToWideString
    void*    sub_236c900(void* inspector, const void* wstr_name, uint32_t flg);// 0x236c900: GXDebugInspector::BeginGroupNode
    void     sub_29055d0(void* inspector, const void* wstr_name, void* val);   // 0x29055d0: GXDebugInspector::AddInt32Property
    void     sub_2905e60(void* inspector, const void* wstr_name, void* val);   // 0x2905e60: GXDebugInspector::AddUint32Property
    void     sub_219db10(void* inspector, const void* wstr_name, void* val);   // 0x219db10: GXDebugInspector::AddBoolProperty
    void     sub_26279a0(void* dst_wstr, const void* src_wstr,
                         size_t pos, size_t count);                            // 0x26279a0: std::wstring::assign(const std::wstring&, pos, n)
    void     sub_262a720(void* dst_wstr, const char16_t* str, size_t len);     // 0x262a720: std::wstring::assign(const char16_t*, len)

    // FLVER Model Resource Subobject Constructors / Destructors / Resets
    void     sub_e51e90(void* subobj, void* alloc);                            // 0x0e51e90: ConstructFlverMeshContainer
    void     sub_e52310(void* subobj);                                         // 0x0e52310: ClearFlverMeshContainer
    void     sub_e52280(void* subobj);                                         // 0x0e52280: DestroyFlverMeshContainer
    void     sub_e523b0(void* subobj, void* inspector);                        // 0x0e523b0: InspectFlverMeshContainer
    void     sub_e51210(void* subobj, void* alloc);                            // 0x0e51210: ConstructFlverMatrixArray
    void     sub_e51640(void* subobj);                                         // 0x0e51640: ClearFlverMatrixArray
    void     sub_e515f0(void* subobj);                                         // 0x0e515f0: DestroyFlverMatrixArray
    void     sub_e512b0(void* subobj, int32_t count);                          // 0x0e512b0: ResizeFlverMatrixArray
    void     sub_e516e0(void* dst_arr, const void* src_arr, const void* mesh); // 0x0e516e0: ComputeFlverBindPoseMatrices
    void     sub_e4ddc0(void* subobj, void* alloc);                            // 0x0e4ddc0: ConstructFlverBoneHierarchy
    void     sub_e4e140(void* subobj);                                         // 0x0e4e140: ClearFlverBoneHierarchy
    void     sub_e4e100(void* subobj);                                         // 0x0e4e100: DestroyFlverBoneHierarchy
    void     sub_e502c0(void* subobj, void* alloc);                            // 0x0e502c0: ConstructFlverPrimitiveTable
    void     sub_e50cc0(void* subobj);                                         // 0x0e50cc0: ClearFlverPrimitiveTable
    void     sub_e50c20(void* subobj);                                         // 0x0e50c20: DestroyFlverPrimitiveTable
    void     sub_e51070(void* prim_tbl, const void* submesh_tbl);              // 0x0e51070: BindFlverPrimitivesToSubmeshes
    void     sub_e50d20(void* prim_tbl, const void* vdecl_tbl,
                        const void* vb_tbl, const void* mat_list, void* ctx);  // 0x0e50d20: FinalizeFlverPrimitiveBindings
    void     sub_e4edb0(void* subobj, void* alloc);                            // 0x0e4edb0: ConstructFlverSubmeshTable
    void     sub_e4f570(void* subobj);                                         // 0x0e4f570: ClearFlverSubmeshTable
    void     sub_e4f3a0(void* subobj);                                         // 0x0e4f3a0: DestroyFlverSubmeshTable
    void     sub_e528e0(void* subobj, void* alloc);                            // 0x0e528e0: ConstructFlverVertexBufferTable
    void     sub_e52e50(void* subobj);                                         // 0x0e52e50: ClearFlverVertexBufferTable
    void     sub_e52e30(void* subobj);                                         // 0x0e52e30: DestroyFlverVertexBufferTable
    void     sub_e4f810(void* subobj, void* alloc);                            // 0x0e4f810: ConstructFlverVertexDeclTable
    void     sub_e4f960(void* subobj);                                         // 0x0e4f960: ClearFlverVertexDeclTable
    void     sub_e4f900(void* subobj);                                         // 0x0e4f900: DestroyFlverVertexDeclTable
    void*    sub_21ce1b0(void* out_iter, void* vec, void* last,
                         uint64_t u3, uint64_t u4, uint64_t u5,
                         uint64_t s0, uint64_t s1, void* first,
                         uint64_t s2, uint64_t s3, void* stack_last);          // 0x21ce1b0: GXRefCountedPtrVector_EraseRange
    void     sub_21cda20(void* tree_holder);                                   // 0x21cda20: DestroyRedBlackTreeHolder

    // Forward declarations within this translation unit
    void     sub_21c8c50(void* self, const float* bone_matrices,
                         const float* prev_bone_matrices, uint32_t ctx_idx);   // 0x21c8c50
    void     sub_21c7ff0(void* self);                                          // 0x21c7ff0
    void     sub_21c7e80(void* vec, size_t new_cap);                           // 0x21c7e80
    void     sub_21c7d80(void* vec, size_t new_cap);                           // 0x21c7d80
    void     sub_21c7890(void* mat_list, void* inspector);                     // 0x21c7890
    void     sub_21c6410(void* self);                                          // 0x21c6410
    void     sub_21c5e80(void* vec, size_t new_cap);                           // 0x21c5e80
    void*    sub_21c5cf0(void* out_iter, void* vec,
                         uint64_t pad_rdx, uint64_t pad_rcx,
                         uint64_t pad_r8, uint64_t pad_r9,
                         void* arg_s0, void* arg_s8, void* erase_pos);         // 0x21c5cf0
    void     sub_21c5a70(void* vec, const void* elem);                         // 0x21c5a70
    void     sub_21c5960(void* elem);                                          // 0x21c5960
    void*    sub_21c56a0(void* vec, const char16_t* name, uint32_t crc);       // 0x21c56a0
    void*    sub_21c5550(void* vec, const char16_t* name, uint32_t param_id);  // 0x21c5550
    void     sub_21c54b0(void* vec);                                           // 0x21c54b0
    void     sub_21c4f10(void* vec, const char16_t* mat_name,
                         const char16_t* param_name, const float* val4,
                         float scalar_val);                                    // 0x21c4f10

    // Global Singletons & Tables
#undef g_stack_cookie_ptr_53e3be0
#define g_stack_cookie_ptr_53e3be0 (*reinterpret_cast<uint64_t**>(GX_VMA(0x53e3be0ULL)))
#undef g_default_heap_cache_5540670
#define g_default_heap_cache_5540670 (*reinterpret_cast<void**>(GX_VMA(0x5540670ULL)))
#undef g_default_allocator_5540680
#define g_default_allocator_5540680 (*reinterpret_cast<void**>(GX_VMA(0x5540680ULL)))
#undef g_fallback_frame_alloc_5540688
#define g_fallback_frame_alloc_5540688 (*reinterpret_cast<void**>(GX_VMA(0x5540688ULL)))
#undef g_gx_device_singleton_55406c8
#define g_gx_device_singleton_55406c8 (*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))
#undef g_wstr_allocator_5540e10
#define g_wstr_allocator_5540e10 (*reinterpret_cast<void**>(GX_VMA(0x5540e10ULL)))
#undef g_skin_target_vec_begin_56a6cc8
#define g_skin_target_vec_begin_56a6cc8 (*reinterpret_cast<void**>(GX_VMA(0x56a6cc8ULL)))
#undef g_skin_target_vec_end_56a6cd0
#define g_skin_target_vec_end_56a6cd0 (*reinterpret_cast<void**>(GX_VMA(0x56a6cd0ULL)))
#undef g_custom_cb_allocator_56a6ce8
#define g_custom_cb_allocator_56a6ce8 (*reinterpret_cast<void**>(GX_VMA(0x56a6ce8ULL)))
#undef g_skinning_batch_mgr_56a6cf0
#define g_skinning_batch_mgr_56a6cf0 (*reinterpret_cast<void**>(GX_VMA(0x56a6cf0ULL)))
#undef g_vtable_53b6e70
#define g_vtable_53b6e70 (reinterpret_cast<uint64_t*>(GX_VMA(0x53b6e70ULL)))
#undef g_vtable_53b6eb0
#define g_vtable_53b6eb0 (reinterpret_cast<uint64_t*>(GX_VMA(0x53b6eb0ULL)))
#undef g_vtable_53b6ee0
#define g_vtable_53b6ee0 (reinterpret_cast<uint64_t*>(GX_VMA(0x53b6ee0ULL)))
#undef g_vtable_53b6f20
#define g_vtable_53b6f20 (reinterpret_cast<uint64_t*>(GX_VMA(0x53b6f20ULL)))
}

namespace {

// Thread-local storage emulations matching PS4 fs:0x0 + offset
#define g_tls_frame_heap_53e4658 (*reinterpret_cast<void**>(gx_guest_tls_slot(0x53e4658ULL)))                    // 0x53e4658
#define g_tls_release_heap_53e44a8 (reinterpret_cast<void*>(gx_guest_tls_slot(0x53e44a8ULL)))                    // 0x53e44a8
#define g_tls_temp_heap_53e4568 (*reinterpret_cast<void**>(gx_guest_tls_slot(0x53e4568ULL)))                    // 0x53e4568
#define g_tls_tx_active_53e4318 (*reinterpret_cast<int32_t*>(gx_guest_tls_slot(0x53e4318ULL)))                          // 0x53e4318
#define g_tls_tx_ctx_53e4320 (*reinterpret_cast<uint64_t*>(gx_guest_tls_slot(0x53e4320ULL)))                          // 0x53e4320

// .rodata SIMD Constants (0x4920ba0 .. 0x4920ca0) & Identity 3x4 (0x5128a10)
alignas(16) constexpr float kSimd0001_4920ba0[4]     = { 0.0f, 0.0f, 0.0f, 1.0f };
alignas(16) constexpr float kSimd0101_4920bb0[4]     = { 0.0f, 1.0f, 0.0f, 1.0f };
alignas(16) constexpr float kSimd0010_4920bc0[4]     = { 0.0f, 0.0f, 1.0f, 0.0f };
alignas(16) constexpr float kSimd01n01_4920bd0[4]    = { 0.0f, 1.0f, -0.0f, 1.0f };
alignas(16) constexpr float kSimd1111_4920be0[4]     = { 1.0f, 1.0f, 1.0f, 1.0f };
alignas(16) constexpr float kSimdNegMax_4920bf0[4]   = { -3.402823466e+38f, -3.402823466e+38f, -3.402823466e+38f, -3.402823466e+38f };
alignas(16) constexpr float kSimdPosMax_4920c00[4]   = {  3.402823466e+38f,  3.402823466e+38f,  3.402823466e+38f,  3.402823466e+38f };
alignas(16) constexpr uint32_t kMaskXYZ_4920c10[4]   = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u };
alignas(16) constexpr float kSimd0001_4920c20[4]     = { 0.0f, 0.0f, 0.0f, 1.0f };
alignas(16) constexpr float kSimd0001_4920c50[4]     = { 0.0f, 0.0f, 0.0f, 1.0f };
alignas(16) constexpr float kSimd0101_4920c60[4]     = { 0.0f, 1.0f, 0.0f, 1.0f };
alignas(16) constexpr float kSimd0010_4920c70[4]     = { 0.0f, 0.0f, 1.0f, 0.0f };
alignas(16) constexpr float kSimd01n01_4920c80[4]    = { 0.0f, 1.0f, -0.0f, 1.0f };
alignas(16) constexpr float kSimd1111_4920c90[4]     = { 1.0f, 1.0f, 1.0f, 1.0f };
alignas(16) constexpr float kSimd0001_4920ca0[4]     = { 0.0f, 0.0f, 0.0f, 1.0f };
#define kIdentity3x4_5128a10      (reinterpret_cast<const float*>(GX_VMA(0x5128a10ULL)))
#define kCrc32Table_52b74f0       (reinterpret_cast<const uint32_t*>(GX_VMA(0x52b74f0ULL)))
#define kErrTxFailed_4737d80      (reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)))
#define kErrIncompatHeap_4737dc0  (reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)))
#define kErrFile_493bac3          (reinterpret_cast<const char*>(GX_VMA(0x493bac3ULL)))
#define kErrHeapNotFound_4940207  (reinterpret_cast<const char*>(GX_VMA(0x4940207ULL)))
#define kEmptyWStr_49d85f0        (reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL)))

} // anonymous namespace

extern "C" {

// ============================================================================
// 1. 0x21c9a70 (31 B) - sub_21c9a70
// Gets the current ring-buffered CPU shadow bone matrix buffer at +0x20.
// ============================================================================
void* sub_21c9a70(const void* self) {
    const auto* s = static_cast<const uint8_t*>(self);
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    if (model[0x200] == 0) {
        return nullptr;
    }
    int64_t idx = *reinterpret_cast<const int32_t*>(s + 0x98);
    return *reinterpret_cast<void* const*>(s + idx * 8 + 0x20);
}

// ============================================================================
// 2. 0x21c9a30 (49 B) - sub_21c9a30
// Gets the previous frame's ring-buffered secondary bone buffer from +0x18.
// ============================================================================
void* sub_21c9a30(const void* self) {
    const auto* s = static_cast<const uint8_t*>(self);
    int32_t cur = *reinterpret_cast<const int32_t*>(s + 0x98);
    int32_t prev = cur - 1;
    if (cur <= 0) {
        prev += *reinterpret_cast<const int32_t*>(s + 0x9c);
    }
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    if (model[0x200] == 0) {
        return nullptr;
    }
    void* const* arr = *reinterpret_cast<void* const* const*>(s + 0x18);
    return arr[static_cast<int64_t>(prev)];
}

// ============================================================================
// 3. 0x21c99f0 (49 B) - sub_21c99f0
// Gets the previous frame's ring-buffered velocity/motion bone buffer (+0x10).
// ============================================================================
void* sub_21c99f0(const void* self) {
    const auto* s = static_cast<const uint8_t*>(self);
    int32_t cur = *reinterpret_cast<const int32_t*>(s + 0x98);
    int32_t prev = cur - 1;
    if (cur <= 0) {
        prev += *reinterpret_cast<const int32_t*>(s + 0x9c);
    }
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    if (model[0x200] == 0) {
        return nullptr;
    }
    void* const* arr = *reinterpret_cast<void* const* const*>(s + 0x10);
    return arr[static_cast<int64_t>(prev)];
}

// ============================================================================
// 4. 0x21c99c0 (34 B) - sub_21c99c0
// Gets the current frame's ring-buffered secondary bone buffer from +0x18.
// ============================================================================
void* sub_21c99c0(const void* self) {
    const auto* s = static_cast<const uint8_t*>(self);
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    if (model[0x200] == 0) {
        return nullptr;
    }
    int64_t idx = *reinterpret_cast<const int32_t*>(s + 0x98);
    void* const* arr = *reinterpret_cast<void* const* const*>(s + 0x18);
    return arr[idx];
}

// ============================================================================
// 5. 0x21c9990 (34 B) - sub_21c9990
// Gets the current frame's ring-buffered velocity/motion bone buffer (+0x10).
// ============================================================================
void* sub_21c9990(const void* self) {
    const auto* s = static_cast<const uint8_t*>(self);
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    if (model[0x200] == 0) {
        return nullptr;
    }
    int64_t idx = *reinterpret_cast<const int32_t*>(s + 0x98);
    void* const* arr = *reinterpret_cast<void* const* const*>(s + 0x10);
    return arr[idx];
}

// ============================================================================
// 6. 0x21c9940 (75 B) - sub_21c9940
// Gets the previous frame's primary constant buffer (unified or per-submesh).
// ============================================================================
void* sub_21c9940(const void* self, int32_t submesh_idx) {
    const auto* s = static_cast<const uint8_t*>(self);
    int32_t cur = *reinterpret_cast<const int32_t*>(s + 0x98);
    int32_t prev = cur - 1;
    if (cur <= 0) {
        prev += *reinterpret_cast<const int32_t*>(s + 0x9c);
    }
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    int64_t ring_idx = static_cast<int64_t>(prev);
    if (model[0x200] != 0) {
        void* const* arr = *reinterpret_cast<void* const* const*>(s + 0x08);
        return arr[ring_idx];
    }
    const auto* vec_array = *reinterpret_cast<const uint8_t* const*>(s + 0x00);
    void* const* sub_arr = *reinterpret_cast<void* const* const*>(vec_array + ring_idx * 40 + 0x08);
    return sub_arr[static_cast<int64_t>(submesh_idx)];
}

// ============================================================================
// 7. 0x21c9900 (61 B) - sub_21c9900
// Gets the current frame's primary constant buffer (unified or per-submesh).
// ============================================================================
void* sub_21c9900(const void* self, int32_t submesh_idx) {
    const auto* s = static_cast<const uint8_t*>(self);
    const auto* model = *reinterpret_cast<const uint8_t* const*>(s + 0xb0);
    int64_t ring_idx = *reinterpret_cast<const int32_t*>(s + 0x98);
    if (model[0x200] != 0) {
        void* const* arr = *reinterpret_cast<void* const* const*>(s + 0x08);
        return arr[ring_idx];
    }
    const auto* vec_array = *reinterpret_cast<const uint8_t* const*>(s + 0x00);
    void* const* sub_arr = *reinterpret_cast<void* const* const*>(vec_array + ring_idx * 40 + 0x08);
    return sub_arr[static_cast<int64_t>(submesh_idx)];
}

// ============================================================================
// 8. 0x21c98a0 (87 B) - sub_21c98a0
// Enqueues or immediately uploads bone matrices with motion blur matrices.
// ============================================================================
void sub_21c98a0(void* self, const float* bone_matrices,
                 const float* prev_bone_matrices, uint32_t ctx_idx) {
    void* batch_mgr = g_skinning_batch_mgr_56a6cf0;
    if (batch_mgr != nullptr) {
        if (sub_21cba80(batch_mgr, self, bone_matrices, prev_bone_matrices) != 0) {
            return;
        }
    }
    sub_21c8c50(self, bone_matrices, prev_bone_matrices, ctx_idx);
}

// ============================================================================
// 9. 0x21c8c50 (3142 B) - sub_21c8c50
// Ring-buffered SIMD 3x4 bone matrix inverse-bind & motion-blur transformer
// and GPU constant buffer uploader.
// ============================================================================
void sub_21c8c50(void* self, const float* bone_matrices,
                 const float* prev_bone_matrices, uint32_t ctx_idx) {
    auto* s = static_cast<uint8_t*>(self);

    // Advance ring buffer index: (++idx >= count) ? (idx -= count)
    int32_t next_idx = *reinterpret_cast<int32_t*>(s + 0x98) + 1;
    *reinterpret_cast<int32_t*>(s + 0x98) = next_idx;
    int32_t wrapped = next_idx - *reinterpret_cast<int32_t*>(s + 0x9c);
    if (wrapped >= 0) {
        *reinterpret_cast<int32_t*>(s + 0x98) = wrapped;
        next_idx = wrapped;
    }
    s[0xa4] = 0;

    // Resolve command context from GXDevice singleton (0x55406c8)
    auto* dev = static_cast<uint8_t*>(g_gx_device_singleton_55406c8);
    auto* ctx_slot = *reinterpret_cast<uint8_t**>(dev + ((dev[0x84] != 0) ? 0x78 : 0x10));
    if (ctx_slot[0x2093] == 0) {
        auto* ctx_table = *reinterpret_cast<uint8_t**>(dev + 0x68);
        ctx_slot = *reinterpret_cast<uint8_t**>(ctx_table + static_cast<uint64_t>(ctx_idx) * 8);
    }
    auto* cmd_ctx = *reinterpret_cast<uint8_t**>(ctx_slot + 0x08);

    auto* model = *reinterpret_cast<uint8_t**>(s + 0xb0);
    if (model[0x200] != 0) {
        // Unified skinning buffer mode
        int64_t bone_count = *reinterpret_cast<int32_t*>(model + 0x1c);
        size_t alloc_bytes = static_cast<size_t>(bone_count) * 48;

        auto* tls_base = static_cast<uint8_t*>(g_tls_frame_heap_53e4658);
        void* alloc_obj = (tls_base != nullptr) ? static_cast<void*>(tls_base + 0xc8) : g_fallback_frame_alloc_5540688;
        auto* vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
        using AllocFn = void* (*)(void*, size_t, size_t);
        void* temp_buf = reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(alloc_obj, alloc_bytes, 16);
        if (temp_buf == nullptr && alloc_obj != g_fallback_frame_alloc_5540688) {
            alloc_obj = g_fallback_frame_alloc_5540688;
            vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
            temp_buf = reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(alloc_obj, alloc_bytes, 16);
        }
        if (temp_buf == nullptr) {
            return;
        }

        if (*reinterpret_cast<void**>(s + 0x18) != nullptr) {
            auto* cur_model = *reinterpret_cast<uint8_t**>(s + 0xb0);
            if (*reinterpret_cast<int32_t*>(cur_model + 0x1ec) != 0) {
                if (prev_bone_matrices != nullptr) {
                    if (bone_count > 0) {
                        auto* dst_ptr = static_cast<float*>(temp_buf) + 8;
                        const float* cur_ptr = bone_matrices + 8;
                        const float* prv_ptr = prev_bone_matrices + 8;

                        const __m128i sign_hi32 = _mm_set_epi32(0, static_cast<int>(0x80000000u), 0, 0);
                        const __m128  sign_lo0  = _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, static_cast<int>(0x80000000u)));
                        const __m128i sign_lo1  = _mm_set_epi32(0, 0, static_cast<int>(0x80000000u), 0);
                        const __m128  sign_hi1  = _mm_castsi128_ps(_mm_set_epi32(static_cast<int>(0x80000000u), 0, 0, 0));
                        const __m128  c_0001    = _mm_load_ps(kSimd0001_4920ca0);

                        for (int32_t i = 0; i < static_cast<int32_t>(bone_count); ++i) {
                            // Load 3x4 previous matrix rows (xmm7, xmm1, xmm4) and invert 3x4 -> multiply by cur_ptr
                            __m128 r0 = _mm_loadu_ps(prv_ptr - 8);
                            __m128 r1 = _mm_loadu_ps(prv_ptr - 4);
                            __m128 r2 = _mm_loadu_ps(prv_ptr);

                            __m128 xmm2  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2), 0x50));
                            __m128 xmm3  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r0), 0x50));
                            __m128 xmm8  = _mm_movehl_ps(r1, r1);
                            __m128 xmm9  = _mm_shuffle_ps(r2, r0, 0x88);
                            __m128 xmm0  = _mm_load_ps(kSimd0001_4920c50);
                            __m128 xmm10 = _mm_shuffle_ps(xmm0, r1, 0xdd);
                            xmm2 = _mm_mul_ps(xmm2, _mm_load_ps(kSimd0101_4920c60));
                            xmm3 = _mm_mul_ps(xmm3, xmm8);
                            xmm8 = _mm_mul_ps(xmm9, xmm10);

                            __m128 xmm6  = _mm_movehl_ps(r2, r2);
                            xmm10 = _mm_movehl_ps(r0, r0);
                            __m128 xmm5  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1), 0x50));
                            xmm9  = _mm_shuffle_ps(r2, r0, 0xdd);
                            __m128 xmm15 = _mm_setzero_ps();
                            xmm0  = _mm_shuffle_ps(xmm15, r1, 0x88);
                            xmm6  = _mm_mul_ps(xmm6, xmm15);
                            xmm2  = _mm_sub_ps(xmm2, xmm6);
                            xmm5  = _mm_mul_ps(xmm10, xmm5);
                            xmm3  = _mm_sub_ps(xmm3, xmm5);
                            xmm0  = _mm_mul_ps(xmm9, xmm0);
                            xmm0  = _mm_sub_ps(xmm8, xmm0);

                            xmm8  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1), 0x26));
                            xmm10 = _mm_shuffle_ps(xmm0, xmm2, 0xd4);
                            xmm9  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r0), 0xc8));
                            xmm5  = _mm_shuffle_ps(xmm2, xmm0, 0x1e);
                            xmm6  = _mm_shuffle_ps(xmm0, xmm3, 0xde);
                            __m128 xmm14 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2), 0xc8));
                            xmm8  = _mm_mul_ps(xmm8, xmm10);
                            xmm10 = _mm_shuffle_ps(xmm3, xmm0, 0xbe);
                            xmm5  = _mm_mul_ps(xmm9, xmm5);
                            xmm15 = _mm_mul_ps(xmm6, xmm15);
                            xmm9  = _mm_mul_ps(xmm14, xmm10);

                            xmm10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1), 0x79));
                            xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x4c));
                            xmm6  = _mm_mul_ps(xmm10, xmm6);
                            xmm10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r0), 0x6d));
                            xmm8  = _mm_sub_ps(xmm8, xmm6);

                            xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x58));
                            xmm6  = _mm_mul_ps(xmm10, xmm6);
                            xmm10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm3), 0x4c));
                            xmm14 = _mm_sub_ps(xmm5, xmm6);

                            xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2), 0x6d));
                            xmm5  = _mm_mul_ps(xmm10, _mm_load_ps(kSimd0010_4920c70));
                            xmm10 = _mm_sub_ps(xmm15, xmm5);

                            xmm5  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm3), 0x58));
                            xmm5  = _mm_mul_ps(xmm6, xmm5);
                            xmm9  = _mm_sub_ps(xmm9, xmm5);

                            xmm6  = _mm_shuffle_ps(xmm2, xmm0, 0x1a);
                            xmm2  = _mm_shuffle_ps(xmm0, xmm2, 0xc4);
                            xmm5  = _mm_shuffle_ps(xmm3, xmm0, 0xba);
                            xmm0  = _mm_shuffle_ps(xmm0, xmm3, 0xce);

                            __m128 xmm1 = _mm_castsi128_ps(_mm_xor_si128(_mm_shuffle_epi32(_mm_castps_si128(r1), 0xcc), sign_hi32));
                            xmm1 = _mm_add_ps(_mm_mul_ps(xmm1, xmm6), xmm8);

                            xmm3 = _mm_castsi128_ps(_mm_xor_si128(_mm_shuffle_epi32(_mm_castps_si128(r0), 0x36), sign_lo1));
                            xmm2 = _mm_add_ps(_mm_mul_ps(xmm2, xmm3), xmm14);

                            xmm3 = _mm_add_ps(_mm_mul_ps(xmm5, _mm_load_ps(kSimd01n01_4920c80)), xmm10);

                            __m128 xmm4 = _mm_castsi128_ps(_mm_xor_si128(_mm_shuffle_epi32(_mm_castps_si128(r2), 0x36), sign_lo1));
                            xmm0 = _mm_add_ps(_mm_mul_ps(xmm0, xmm4), xmm9);

                            xmm1 = _mm_xor_ps(xmm1, sign_lo0);
                            xmm2 = _mm_xor_ps(xmm2, sign_hi1);
                            xmm3 = _mm_xor_ps(xmm3, sign_lo0);
                            xmm0 = _mm_xor_ps(xmm0, sign_hi1);

                            xmm1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x39));
                            xmm2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x39));
                            xmm3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm3), 0x39));
                            xmm0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x39));

                            xmm4 = _mm_mul_ps(r0, xmm1);
                            xmm4 = _mm_hadd_ps(xmm4, xmm4);
                            xmm4 = _mm_hadd_ps(xmm4, xmm4);
                            xmm4 = _mm_div_ps(_mm_load_ps(kSimd1111_4920c90), xmm4);

                            xmm1 = _mm_mul_ps(xmm4, xmm1);
                            xmm2 = _mm_mul_ps(xmm4, xmm2);
                            xmm3 = _mm_mul_ps(xmm4, xmm3);
                            xmm0 = _mm_mul_ps(xmm4, xmm0);

                            xmm4 = _mm_movelh_ps(xmm1, xmm2);
                            xmm1 = _mm_movehl_ps(xmm2, xmm1);
                            xmm2 = _mm_movelh_ps(xmm3, xmm0);
                            xmm0 = _mm_movehl_ps(xmm0, xmm3);

                            xmm3 = _mm_shuffle_ps(xmm4, xmm2, 0x88);
                            xmm2 = _mm_shuffle_ps(xmm4, xmm2, 0xdd);
                            xmm0 = _mm_shuffle_ps(xmm1, xmm0, 0x88);

                            // Multiply current bone matrix by inverted previous matrix
                            __m128 c0 = _mm_loadu_ps(cur_ptr - 8);
                            __m128 c1 = _mm_loadu_ps(cur_ptr - 4);
                            __m128 c2 = _mm_loadu_ps(cur_ptr);

                            auto mul_row = [&](const __m128& row) -> __m128 {
                                __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                                __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                                __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                                __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                                __m128 xy = _mm_add_ps(_mm_mul_ps(x, xmm3), _mm_mul_ps(y, xmm2));
                                __m128 xyz = _mm_add_ps(_mm_mul_ps(z, xmm0), xy);
                                return _mm_add_ps(_mm_mul_ps(w, c_0001), xyz);
                            };

                            _mm_storeu_ps(dst_ptr - 8, mul_row(c0));
                            _mm_storeu_ps(dst_ptr - 4, mul_row(c1));
                            _mm_storeu_ps(dst_ptr,     mul_row(c2));

                            dst_ptr += 12;
                            cur_ptr += 12;
                            prv_ptr += 12;
                        }
                    }

                    int64_t r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                    void* cb_vel = (*reinterpret_cast<void***>(s + 0x10))[r_idx];
                    sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_vel, 0, 0, temp_buf, 0, 0);

                    if (bone_count > 0) {
                        const __m128 c_0001 = _mm_load_ps(kSimd0001_4920ca0);
                        for (int64_t b = 0; b < bone_count; ++b) {
                            const auto* inv_bind = *reinterpret_cast<const float* const*>(model + 0x1f0) + b * 12;
                            const float* src_m = prev_bone_matrices + b * 12;
                            float* dst_m = static_cast<float*>(temp_buf) + b * 12;

                            __m128 ib0 = _mm_loadu_ps(inv_bind);
                            __m128 ib1 = _mm_loadu_ps(inv_bind + 4);
                            __m128 ib2 = _mm_loadu_ps(inv_bind + 8);

                            for (int r = 0; r < 3; ++r) {
                                __m128 row = _mm_loadu_ps(src_m + r * 4);
                                __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                                __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                                __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                                __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                                __m128 res = _mm_add_ps(_mm_mul_ps(ib0, x), _mm_mul_ps(y, ib1));
                                res = _mm_add_ps(_mm_mul_ps(z, ib2), res);
                                res = _mm_add_ps(_mm_mul_ps(w, c_0001), res);
                                _mm_storeu_ps(dst_m + r * 4, res);
                            }
                        }
                    }

                    r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                    void* cb_prv = (*reinterpret_cast<void***>(s + 0x18))[r_idx];
                    sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_prv, 0, 0, temp_buf, 0, 0);
                    s[0xa4] = 1;
                } else {
                    // prev_bone_matrices == nullptr, but model[0x1ec] != 0:
                    // 1) Upload identity to cb_prv first, then compute bone_matrices * inv_bind once
                    //    and reuse it for both cb_vel and cb_main / shadow_buf.
                    auto* dst_id = static_cast<float*>(temp_buf);
                    for (int32_t b = 0; b < static_cast<int32_t>(bone_count); ++b, dst_id += 12) {
                        _mm_storeu_ps(dst_id,     _mm_load_ps(kIdentity3x4_5128a10));
                        _mm_storeu_ps(dst_id + 4, _mm_load_ps(kIdentity3x4_5128a10 + 4));
                        _mm_storeu_ps(dst_id + 8, _mm_load_ps(kIdentity3x4_5128a10 + 8));
                    }
                    int64_t r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                    void* cb_prv = (*reinterpret_cast<void***>(s + 0x18))[r_idx];
                    sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_prv, 0, 0, temp_buf, 0, 0);

                    if (bone_count > 0) {
                        const __m128 c_0001 = _mm_load_ps(kSimd0001_4920ca0);
                        for (int64_t b = 0; b < bone_count; ++b) {
                            const auto* inv_bind = *reinterpret_cast<const float* const*>(model + 0x1f0) + b * 12;
                            const float* src_m = bone_matrices + b * 12;
                            float* dst_m = static_cast<float*>(temp_buf) + b * 12;

                            __m128 ib0 = _mm_loadu_ps(inv_bind);
                            __m128 ib1 = _mm_loadu_ps(inv_bind + 4);
                            __m128 ib2 = _mm_loadu_ps(inv_bind + 8);

                            for (int r = 0; r < 3; ++r) {
                                __m128 row = _mm_loadu_ps(src_m + r * 4);
                                __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                                __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                                __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                                __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                                __m128 res = _mm_add_ps(_mm_mul_ps(ib0, x), _mm_mul_ps(y, ib1));
                                res = _mm_add_ps(_mm_mul_ps(z, ib2), res);
                                res = _mm_add_ps(_mm_mul_ps(w, c_0001), res);
                                _mm_storeu_ps(dst_m + r * 4, res);
                            }
                        }
                    }
                    r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                    void* cb_vel = (*reinterpret_cast<void***>(s + 0x10))[r_idx];
                    sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_vel, 0, 0, temp_buf, 0, 0);

                    r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                    void* shadow_buf = *reinterpret_cast<void**>(s + r_idx * 8 + 0x20);
                    if (shadow_buf != nullptr) {
                        sub_261a1b0(shadow_buf, temp_buf, alloc_bytes);
                        r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                    }
                    void* cb_main = (*reinterpret_cast<void***>(s + 0x08))[r_idx];
                    sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_main, 0, 0, temp_buf, 0, 0);

                    auto* free_vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
                    using FreeFn = void (*)(void*, void*);
                    reinterpret_cast<FreeFn>(free_vtbl[0x70 / 8])(alloc_obj, temp_buf);
                    return;
                }
            } else {
                // model[0x1ec] == 0
                auto* dst_id = static_cast<float*>(temp_buf);
                for (int32_t b = 0; b < static_cast<int32_t>(bone_count); ++b, dst_id += 12) {
                    _mm_storeu_ps(dst_id,     _mm_load_ps(kIdentity3x4_5128a10));
                    _mm_storeu_ps(dst_id + 4, _mm_load_ps(kIdentity3x4_5128a10 + 4));
                    _mm_storeu_ps(dst_id + 8, _mm_load_ps(kIdentity3x4_5128a10 + 8));
                }
                int64_t r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
                void* cb_prv = (*reinterpret_cast<void***>(s + 0x18))[r_idx];
                sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_prv, 0, 0, temp_buf, 0, 0);
            }
        }

        // Upload current frame primary bone matrices (transformed by inverse bind if 0x1ec != 0)
        auto* cur_model2 = *reinterpret_cast<uint8_t**>(s + 0xb0);
        if (*reinterpret_cast<int32_t*>(cur_model2 + 0x1ec) != 0) {
            if (bone_count > 0) {
                const __m128 c_0001 = _mm_load_ps(kSimd0001_4920ca0);
                for (int64_t b = 0; b < bone_count; ++b) {
                    const auto* inv_bind = *reinterpret_cast<const float* const*>(model + 0x1f0) + b * 12;
                    const float* src_m = bone_matrices + b * 12;
                    float* dst_m = static_cast<float*>(temp_buf) + b * 12;

                    __m128 ib0 = _mm_loadu_ps(inv_bind);
                    __m128 ib1 = _mm_loadu_ps(inv_bind + 4);
                    __m128 ib2 = _mm_loadu_ps(inv_bind + 8);

                    for (int r = 0; r < 3; ++r) {
                        __m128 row = _mm_loadu_ps(src_m + r * 4);
                        __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                        __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                        __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                        __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                        __m128 res = _mm_add_ps(_mm_mul_ps(ib0, x), _mm_mul_ps(y, ib1));
                        res = _mm_add_ps(_mm_mul_ps(z, ib2), res);
                        res = _mm_add_ps(_mm_mul_ps(w, c_0001), res);
                        _mm_storeu_ps(dst_m + r * 4, res);
                    }
                }
            }
        } else {
            sub_261a1b0(temp_buf, bone_matrices, alloc_bytes);
        }

        int64_t r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
        void* shadow_buf = *reinterpret_cast<void**>(s + r_idx * 8 + 0x20);
        if (shadow_buf != nullptr) {
            sub_261a1b0(shadow_buf, temp_buf, alloc_bytes);
            r_idx = *reinterpret_cast<int32_t*>(s + 0x98);
        }
        void* cb_main = (*reinterpret_cast<void***>(s + 0x08))[r_idx];
        sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_main, 0, 0, temp_buf, 0, 0);

        auto* free_vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(free_vtbl[0x70 / 8])(alloc_obj, temp_buf);
        return;
    }

    // Per-submesh palette mode (model[0x200] == 0): 28 bones max per submesh (0x540 bytes)
    int64_t ring_idx = next_idx;
    auto* vec_array = *reinterpret_cast<uint8_t**>(s + 0x00);
    auto* tls_base = static_cast<uint8_t*>(g_tls_frame_heap_53e4658);
    void* alloc_obj = (tls_base != nullptr) ? static_cast<void*>(tls_base + 0xc8) : g_fallback_frame_alloc_5540688;
    auto* vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
    using AllocFn = void* (*)(void*, size_t, size_t);
    void* palette_buf = reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(alloc_obj, 0x540, 16);
    if (palette_buf == nullptr && alloc_obj != g_fallback_frame_alloc_5540688) {
        alloc_obj = g_fallback_frame_alloc_5540688;
        vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
        palette_buf = reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(alloc_obj, 0x540, 16);
    }
    if (palette_buf == nullptr) {
        return;
    }

    auto* vec_entry = vec_array + ring_idx * 40;
    void** cb_begin = *reinterpret_cast<void***>(vec_entry + 0x08);
    void** cb_end   = *reinterpret_cast<void***>(vec_entry + 0x10);
    if (cb_end != cb_begin) {
        const __m128 c_0001 = _mm_load_ps(kSimd0001_4920ca0);
        size_t sub_idx = 0;
        while (true) {
            void* cb_handle = cb_begin[sub_idx];
            if (cb_handle != nullptr) {
                auto* submesh_info = (*reinterpret_cast<uint8_t***>(s + 0x90))[sub_idx];
                uint16_t pal_count = *reinterpret_cast<uint16_t*>(submesh_info + 0x18);
                const void* upload_src = palette_buf;

                if (submesh_info[0x04] == 0) {
                    if (pal_count != 0) {
                        const auto* bone_indices = *reinterpret_cast<const int32_t* const*>(submesh_info + 0x20);
                        auto* dst_m = static_cast<float*>(palette_buf);
                        for (uint16_t j = 0; j < pal_count; ++j, dst_m += 12) {
                            int64_t b_idx = bone_indices[j];
                            if (b_idx < 0) break;
                            const float* src_m = bone_matrices + b_idx * 12;
                            _mm_storeu_ps(dst_m,     _mm_loadu_ps(src_m));
                            _mm_storeu_ps(dst_m + 4, _mm_loadu_ps(src_m + 4));
                            _mm_storeu_ps(dst_m + 8, _mm_loadu_ps(src_m + 8));
                        }
                    } else {
                        uint16_t single_bone = *reinterpret_cast<uint16_t*>(submesh_info + 0x16);
                        upload_src = bone_matrices + static_cast<size_t>(single_bone) * 12;
                    }
                } else {
                    if (pal_count != 0) {
                        const auto* bone_indices = *reinterpret_cast<const int32_t* const*>(submesh_info + 0x20);
                        auto* dst_m = static_cast<float*>(palette_buf);
                        for (uint16_t j = 0; j < pal_count; ++j, dst_m += 12) {
                            int64_t b_idx = bone_indices[j];
                            if (b_idx < 0) break;
                            const auto* inv_bind = *reinterpret_cast<const float* const*>(model + 0x1f0) + b_idx * 12;
                            const float* src_m = bone_matrices + b_idx * 12;

                            __m128 ib0 = _mm_loadu_ps(inv_bind);
                            __m128 ib1 = _mm_loadu_ps(inv_bind + 4);
                            __m128 ib2 = _mm_loadu_ps(inv_bind + 8);

                            for (int r = 0; r < 3; ++r) {
                                __m128 row = _mm_loadu_ps(src_m + r * 4);
                                __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                                __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                                __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                                __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                                __m128 res = _mm_add_ps(_mm_mul_ps(ib0, x), _mm_mul_ps(y, ib1));
                                res = _mm_add_ps(_mm_mul_ps(z, ib2), res);
                                res = _mm_add_ps(_mm_mul_ps(w, c_0001), res);
                                _mm_storeu_ps(dst_m + r * 4, res);
                            }
                        }
                    } else {
                        uint16_t single_bone = *reinterpret_cast<uint16_t*>(submesh_info + 0x16);
                        const auto* inv_bind = *reinterpret_cast<const float* const*>(model + 0x1f0) + static_cast<size_t>(single_bone) * 12;
                        const float* src_m = bone_matrices + static_cast<size_t>(single_bone) * 12;
                        auto* dst_m = static_cast<float*>(palette_buf);

                        __m128 ib0 = _mm_loadu_ps(inv_bind);
                        __m128 ib1 = _mm_loadu_ps(inv_bind + 4);
                        __m128 ib2 = _mm_loadu_ps(inv_bind + 8);

                        for (int r = 0; r < 3; ++r) {
                            __m128 row = _mm_loadu_ps(src_m + r * 4);
                            __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                            __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                            __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                            __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                            __m128 res = _mm_add_ps(_mm_mul_ps(ib0, x), _mm_mul_ps(y, ib1));
                            res = _mm_add_ps(_mm_mul_ps(z, ib2), res);
                            res = _mm_add_ps(_mm_mul_ps(w, c_0001), res);
                            _mm_storeu_ps(dst_m + r * 4, res);
                        }
                    }
                }

                cb_begin = *reinterpret_cast<void***>(vec_entry + 0x08);
                sub_2168a20(cmd_ctx, cmd_ctx + 0xb730, cb_begin[sub_idx], 0, 0, upload_src, 0, 0);
            }
            ++sub_idx;
            cb_begin = *reinterpret_cast<void***>(vec_entry + 0x08);
            cb_end   = *reinterpret_cast<void***>(vec_entry + 0x10);
            if (sub_idx >= static_cast<size_t>(cb_end - cb_begin)) {
                break;
            }
        }
    }

    auto* free_vtbl = *reinterpret_cast<uintptr_t**>(alloc_obj);
    using FreeFn = void (*)(void*, void*);
    reinterpret_cast<FreeFn>(free_vtbl[0x70 / 8])(alloc_obj, palette_buf);
}

// ============================================================================
// 10. 0x21c8bf0 (85 B) - sub_21c8bf0
// Enqueues or immediately uploads bone matrices without previous-frame motion.
// ============================================================================
void sub_21c8bf0(void* self, const float* bone_matrices, uint32_t ctx_idx) {
    void* batch_mgr = g_skinning_batch_mgr_56a6cf0;
    if (batch_mgr != nullptr) {
        if (sub_21cba80(batch_mgr, self, bone_matrices, nullptr) != 0) {
            return;
        }
    }
    sub_21c8c50(self, bone_matrices, nullptr, ctx_idx);
}

// ============================================================================
// 11. 0x21c8570 (1662 B) - sub_21c8570
// Initializes GXSkinningTarget ring buffers, constant buffers, and shadow RAM.
// ============================================================================
uint8_t sub_21c8570(void* self, void* model_res, uint8_t extra_ring_frames) {
    auto* s = static_cast<uint8_t*>(self);
    auto* model = static_cast<uint8_t*>(model_res);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    uint32_t prim_count = *reinterpret_cast<uint32_t*>(model + 0xa0);
    void* alloc = *reinterpret_cast<void**>(s + 0xa8);
    if (alloc == nullptr) {
        alloc = g_default_allocator_5540680;
    }

    *reinterpret_cast<void**>(s + 0xb0) = model;
    uint32_t ring_count = static_cast<uint32_t>(extra_ring_frames) + 1u;
    if (ring_count > 14u) {
        ring_count = 14u;
    }
    *reinterpret_cast<uint32_t*>(s + 0x9c) = ring_count;
    *reinterpret_cast<uint32_t*>(s + 0xa0) = *reinterpret_cast<uint32_t*>(model + 0x1c);

    // Allocate vector-of-vectors header (ring_count * 40 + 16 bytes)
    auto* alloc_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
    using AllocFn = void* (*)(void*, size_t, size_t);
    auto* raw_vec_arr = static_cast<uint8_t*>(
        reinterpret_cast<AllocFn>(alloc_vtbl[0x58 / 8])(alloc, static_cast<size_t>(ring_count) * 40 + 16, 16));

    uint8_t* vec_arr_ptr = nullptr;
    if (raw_vec_arr != nullptr) {
        vec_arr_ptr = raw_vec_arr + 16;
        uint8_t* cur_elem = raw_vec_arr + 24;
        for (uint32_t i = 0; i < ring_count; ++i, cur_elem += 40) {
            if (cur_elem != reinterpret_cast<uint8_t*>(0x08)) {
                void* heap = g_default_heap_cache_5540670;
                if (heap == nullptr) {
                    heap = sub_2ba2b10();
                    g_default_heap_cache_5540670 = heap;
                }
                uint8_t flags_buf[8] = {};
                auto* heap_vtbl = *reinterpret_cast<uintptr_t**>(heap);
                using CheckFn = void (*)(void*, void*, uint32_t);
                reinterpret_cast<CheckFn>(heap_vtbl[0x20 / 8])(flags_buf, heap, 0);
                if ((flags_buf[0] & 0x20) == 0) {
                    sub_20b55b0(nullptr, 0x3e, kErrIncompatHeap_4737dc0);
                }
                *reinterpret_cast<uint64_t*>(cur_elem + 0x10) = 0;
                *reinterpret_cast<uint64_t*>(cur_elem + 0x08) = 0;
                *reinterpret_cast<uint64_t*>(cur_elem + 0x00) = 0;
                *reinterpret_cast<void**>(cur_elem + 0x18) = heap;
            }
        }
        *reinterpret_cast<uint64_t*>(raw_vec_arr + 0x08) = ring_count;
        *reinterpret_cast<void**>(raw_vec_arr + 0x00) = raw_vec_arr;
    }
    *reinterpret_cast<uint8_t**>(s + 0x00) = vec_arr_ptr;

    auto alloc_ptr_array = [&](uint32_t count) -> void** {
        auto* v = *reinterpret_cast<uintptr_t**>(alloc);
        auto* raw = static_cast<uint8_t*>(
            reinterpret_cast<AllocFn>(v[0x58 / 8])(alloc, static_cast<size_t>(count) * 8 + 16, 16));
        if (raw == nullptr) return nullptr;
        auto** arr = reinterpret_cast<void**>(raw + 16);
        for (uint32_t i = 0; i < count; ++i) {
            arr[i] = nullptr;
        }
        *reinterpret_cast<uint64_t*>(raw + 0x08) = count;
        *reinterpret_cast<void**>(raw + 0x00) = raw;
        return arr;
    };

    if (model[0x200] != 0) {
        int32_t rc = *reinterpret_cast<int32_t*>(s + 0x9c);
        *reinterpret_cast<void***>(s + 0x08) = alloc_ptr_array(static_cast<uint32_t>(rc));
        if (model[0x25a] != 0) {
            rc = *reinterpret_cast<int32_t*>(s + 0x9c);
            *reinterpret_cast<void***>(s + 0x10) = alloc_ptr_array(static_cast<uint32_t>(rc));
            rc = *reinterpret_cast<int32_t*>(s + 0x9c);
            *reinterpret_cast<void***>(s + 0x18) = alloc_ptr_array(static_cast<uint32_t>(rc));
        }
    }

    *reinterpret_cast<void***>(s + 0x90) = alloc_ptr_array(prim_count);

    uint8_t ok = 1;
    int32_t total_ring = *reinterpret_cast<int32_t*>(s + 0x9c);
    for (int32_t r = 0; r < total_ring; ++r) {
        if (model[0x200] == 0) {
            auto* vec_entry = *reinterpret_cast<uint8_t**>(s + 0x00) + static_cast<int64_t>(r) * 40;
            size_t cap = static_cast<size_t>(
                (*reinterpret_cast<uint8_t**>(vec_entry + 0x18) - *reinterpret_cast<uint8_t**>(vec_entry + 0x08)) >> 3);
            if (cap < prim_count) {
                sub_292dcb0(vec_entry, prim_count);
            }
            if (prim_count != 0) {
                if (r == 0) {
                    for (uint32_t p = 0; p < prim_count; ++p) {
                        auto* prim_base = *reinterpret_cast<uint8_t**>(model + 0xa8);
                        void* submesh_ptr = prim_base + static_cast<size_t>(p) * 72;
                        uint32_t cb_desc[6] = { 0x540u, 0u, 4u, 0u, 0u, 0u };
                        void* cb_handle = nullptr;
                        if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                            cb_handle = sub_e30f30(g_custom_cb_allocator_56a6ce8, 0x540);
                            if (cb_handle == nullptr) ok = 0;
                        } else {
                            void* dev = *reinterpret_cast<void**>(g_gx_device_singleton_55406c8);
                            if (sub_2165c10(dev, &cb_handle, cb_desc, nullptr, 0, 0, nullptr) != 0) {
                                ok = 0;
                            }
                        }
                        sub_292ccf0(*reinterpret_cast<uint8_t**>(s + 0x00) + static_cast<int64_t>(r) * 40, &cb_handle);
                        (*reinterpret_cast<void***>(s + 0x90))[p] = submesh_ptr;
                    }
                } else {
                    for (uint32_t p = 0; p < prim_count; ++p) {
                        uint32_t cb_desc[6] = { 0x540u, 0u, 4u, 0u, 0u, 0u };
                        void* cb_handle = nullptr;
                        if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                            cb_handle = sub_e30f30(g_custom_cb_allocator_56a6ce8, 0x540);
                            if (cb_handle == nullptr) ok = 0;
                        } else {
                            void* dev = *reinterpret_cast<void**>(g_gx_device_singleton_55406c8);
                            if (sub_2165c10(dev, &cb_handle, cb_desc, nullptr, 0, 0, nullptr) != 0) {
                                ok = 0;
                            }
                        }
                        sub_292ccf0(*reinterpret_cast<uint8_t**>(s + 0x00) + static_cast<int64_t>(r) * 40, &cb_handle);
                    }
                }
            }
        }

        auto* cur_model = *reinterpret_cast<uint8_t**>(s + 0xb0);
        if (cur_model[0x200] != 0) {
            uint32_t bones = *reinterpret_cast<uint32_t*>(s + 0xa0);
            uint32_t byte_sz = (bones < 2u) ? 0x60u : (bones * 0x30u);
            uint32_t cb_desc[6] = { byte_sz, 0u, 4u, 0u, 0u, 0u };

            void** arr08 = *reinterpret_cast<void***>(s + 0x08);
            arr08[r] = nullptr;
            if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                arr08[r] = sub_e30f30(g_custom_cb_allocator_56a6ce8, byte_sz);
            } else {
                void* dev = *reinterpret_cast<void**>(g_gx_device_singleton_55406c8);
                sub_2165c10(dev, &arr08[r], cb_desc, nullptr, 0, 0, nullptr);
            }

            if (model[0x25a] != 0) {
                void** arr10 = *reinterpret_cast<void***>(s + 0x10);
                arr10[r] = nullptr;
                if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                    arr10[r] = sub_e30f30(g_custom_cb_allocator_56a6ce8, byte_sz);
                } else {
                    void* dev = *reinterpret_cast<void**>(g_gx_device_singleton_55406c8);
                    sub_2165c10(dev, &arr10[r], cb_desc, nullptr, 0, 0, nullptr);
                }

                void** arr18 = *reinterpret_cast<void***>(s + 0x18);
                arr18[r] = nullptr;
                if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                    arr18[r] = sub_e30f30(g_custom_cb_allocator_56a6ce8, byte_sz);
                } else {
                    void* dev = *reinterpret_cast<void**>(g_gx_device_singleton_55406c8);
                    sub_2165c10(dev, &arr18[r], cb_desc, nullptr, 0, 0, nullptr);
                }
            }

            auto* v = *reinterpret_cast<uintptr_t**>(alloc);
            *reinterpret_cast<void**>(s + static_cast<size_t>(r) * 8 + 0x20) =
                reinterpret_cast<AllocFn>(v[0x58 / 8])(alloc, byte_sz, 16);
        }
    }

    *reinterpret_cast<uint32_t*>(s + 0x98) = 0;
    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return ok & 1u;
}

// ============================================================================
// 12. 0x21c7ff0 (1404 B) - sub_21c7ff0
// Finalizes and releases all GPU constant buffers and ring arrays in GXSkinningTarget.
// ============================================================================
void sub_21c7ff0(void* self) {
    auto* s = static_cast<uint8_t*>(self);

    // 1. Remove pending entries for `self` from GXSkinningBatchManager queue (0x56a6cf0)
    if (g_skinning_batch_mgr_56a6cf0 != nullptr) {
        auto* mgr = *reinterpret_cast<uint8_t**>(g_skinning_batch_mgr_56a6cf0);
        auto* q_cur = *reinterpret_cast<uint8_t**>(mgr + 0x10);
        auto* q_end = *reinterpret_cast<uint8_t**>(mgr + 0x18);
        while (q_cur != q_end) {
            uint8_t* next_entry = q_cur + 0x28;
            if (*reinterpret_cast<void**>(q_cur) == self) {
                void* q_alloc = mgr + 0x508;
                using FreeFn = void (*)(void*, void*);
                for (int k = 4; k >= 1; --k) {
                    void* ptr = *reinterpret_cast<void**>(q_cur + k * 8);
                    auto* vtbl = *reinterpret_cast<uintptr_t**>(q_alloc);
                    reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(q_alloc, ptr);
                }
                q_end = *reinterpret_cast<uint8_t**>(mgr + 0x18);
                if (next_entry != q_end) {
                    uint8_t* dst = q_cur;
                    uint8_t* src = next_entry;
                    while (src != q_end) {
                        *reinterpret_cast<uint64_t*>(dst + 0x20) = *reinterpret_cast<uint64_t*>(src + 0x20);
                        *reinterpret_cast<uint64_t*>(dst + 0x18) = *reinterpret_cast<uint64_t*>(src + 0x18);
                        *reinterpret_cast<uint64_t*>(dst + 0x10) = *reinterpret_cast<uint64_t*>(src + 0x10);
                        *reinterpret_cast<uint64_t*>(dst + 0x08) = *reinterpret_cast<uint64_t*>(src + 0x08);
                        *reinterpret_cast<uint64_t*>(dst + 0x00) = *reinterpret_cast<uint64_t*>(src + 0x00);
                        dst += 0x28;
                        src += 0x28;
                    }
                    q_end = *reinterpret_cast<uint8_t**>(mgr + 0x18);
                }
                *reinterpret_cast<uint8_t**>(mgr + 0x18) = q_end - 0x28;
                break;
            }
            q_cur = next_entry;
        }
    }

    using FreeFn = void (*)(void*, void*);
    using DtorFn = void (*)(void*);

    auto destroy_cb_array = [&](size_t field_offset, bool also_free_shadow_20) {
        void** arr = *reinterpret_cast<void***>(s + field_offset);
        if (arr == nullptr) return;
        void* alloc = *reinterpret_cast<void**>(s + 0xa8);
        if (alloc == nullptr) alloc = g_default_allocator_5540680;

        int32_t rc = *reinterpret_cast<int32_t*>(s + 0x9c);
        for (int32_t i = 0; i < rc; ++i) {
            arr = *reinterpret_cast<void***>(s + field_offset);
            void* cb = arr[i];
            if (cb != nullptr) {
                if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                    sub_e31100(g_custom_cb_allocator_56a6ce8, cb);
                } else {
                    void* heap = sub_207b720(cb);
                    auto* cb_vtbl = *reinterpret_cast<uintptr_t**>(cb);
                    reinterpret_cast<DtorFn>(cb_vtbl[0])(cb);
                    auto* h_vtbl = *reinterpret_cast<uintptr_t**>(heap);
                    reinterpret_cast<FreeFn>(h_vtbl[0x70 / 8])(heap, cb);
                    arr = *reinterpret_cast<void***>(s + field_offset);
                    arr[i] = nullptr;
                }
            }
            if (also_free_shadow_20) {
                void* shadow = *reinterpret_cast<void**>(s + static_cast<size_t>(i) * 8 + 0x20);
                if (shadow != nullptr) {
                    auto* tls_slot = reinterpret_cast<void**>(
                        static_cast<uint8_t*>(g_tls_release_heap_53e44a8) + 0x10);
                    void* heap = (g_tls_release_heap_53e44a8 != nullptr) ? *tls_slot : nullptr;
                    if (g_tls_release_heap_53e44a8 != nullptr) *tls_slot = nullptr;
                    if (heap == nullptr) {
                        heap = sub_207b720(shadow);
                        if (heap == nullptr) {
                            sub_20b55b0(const_cast<char*>(kErrFile_493bac3), 0x64, kErrHeapNotFound_4940207);
                        }
                    }
                    auto* h_vtbl = *reinterpret_cast<uintptr_t**>(heap);
                    reinterpret_cast<FreeFn>(h_vtbl[0x70 / 8])(heap, shadow);
                    *reinterpret_cast<void**>(s + static_cast<size_t>(i) * 8 + 0x20) = nullptr;
                }
            }
        }
        arr = *reinterpret_cast<void***>(s + field_offset);
        if (arr != nullptr) {
            void* raw_hdr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(arr) - 0x10);
            auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
            reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, raw_hdr);
            *reinterpret_cast<void**>(s + field_offset) = nullptr;
        }
    };

    // 2. Release ring arrays +0x18, +0x10, +0x08 (with shadow buffers at +0x20)
    destroy_cb_array(0x18, false);
    destroy_cb_array(0x10, false);
    destroy_cb_array(0x08, true);

    // 3. Release per-submesh vector-of-vectors at +0x00 and submesh pointer array at +0x90
    auto* vec_arr = *reinterpret_cast<uint8_t**>(s + 0x00);
    if (vec_arr != nullptr) {
        void* alloc = *reinterpret_cast<void**>(s + 0xa8);
        if (alloc == nullptr) alloc = g_default_allocator_5540680;

        int32_t rc = *reinterpret_cast<int32_t*>(s + 0x9c);
        for (int32_t r = 0; r < rc; ++r) {
            vec_arr = *reinterpret_cast<uint8_t**>(s + 0x00);
            auto* vec_entry = vec_arr + static_cast<size_t>(r) * 40;
            void** cb_begin = *reinterpret_cast<void***>(vec_entry + 0x08);
            void** cb_end   = *reinterpret_cast<void***>(vec_entry + 0x10);
            size_t count = static_cast<size_t>(cb_end - cb_begin);
            for (size_t j = 0; j < count; ++j) {
                vec_arr = *reinterpret_cast<uint8_t**>(s + 0x00);
                cb_begin = *reinterpret_cast<void***>(vec_arr + static_cast<size_t>(r) * 40 + 0x08);
                void* cb = cb_begin[j];
                if (g_custom_cb_allocator_56a6ce8 != nullptr) {
                    sub_e31100(g_custom_cb_allocator_56a6ce8, cb);
                } else if (cb != nullptr) {
                    void* heap = sub_207b720(cb);
                    auto* cb_vtbl = *reinterpret_cast<uintptr_t**>(cb);
                    reinterpret_cast<DtorFn>(cb_vtbl[0])(cb);
                    auto* h_vtbl = *reinterpret_cast<uintptr_t**>(heap);
                    reinterpret_cast<FreeFn>(h_vtbl[0x70 / 8])(heap, cb);
                    vec_arr = *reinterpret_cast<uint8_t**>(s + 0x00);
                    cb_begin = *reinterpret_cast<void***>(vec_arr + static_cast<size_t>(r) * 40 + 0x08);
                    cb_begin[j] = nullptr;
                }
            }
        }

        vec_arr = *reinterpret_cast<uint8_t**>(s + 0x00);
        if (vec_arr != nullptr) {
            uint64_t elem_cnt = *reinterpret_cast<uint64_t*>(vec_arr - 0x08);
            uint8_t* elem = vec_arr + 0x08;
            for (uint64_t i = 0; i < elem_cnt; ++i, elem += 40) {
                void* buf = *reinterpret_cast<void**>(elem);
                if (buf != nullptr) {
                    void* v_alloc = *reinterpret_cast<void**>(elem + 0x18);
                    auto* v_vtbl = *reinterpret_cast<uintptr_t**>(v_alloc);
                    reinterpret_cast<FreeFn>(v_vtbl[0x70 / 8])(v_alloc, buf);
                    *reinterpret_cast<uint64_t*>(elem + 0x10) = 0;
                    *reinterpret_cast<uint64_t*>(elem + 0x08) = 0;
                    *reinterpret_cast<uint64_t*>(elem + 0x00) = 0;
                }
            }
            void* raw_hdr = *reinterpret_cast<void**>(vec_arr - 0x10);
            auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
            reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, raw_hdr);
            *reinterpret_cast<void**>(s + 0x00) = nullptr;
        }

        auto* arr90 = *reinterpret_cast<uint8_t**>(s + 0x90);
        if (arr90 != nullptr) {
            void* raw_hdr = *reinterpret_cast<void**>(arr90 - 0x10);
            auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
            reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, raw_hdr);
            *reinterpret_cast<void**>(s + 0x90) = nullptr;
        }
    }
}

// ============================================================================
// 13. 0x21c7fe0 (5 B) - sub_21c7fe0
// Thunk to sub_21c7ff0 (GXSkinningTarget destructor / finalizer).
// ============================================================================
void sub_21c7fe0(void* self) {
    sub_21c7ff0(self);
}

// ============================================================================
// 14. 0x21c7fa0 (50 B) - sub_21c7fa0
// Constructs a zeroed GXSkinningTarget (0xb8 bytes).
// ============================================================================
void sub_21c7fa0(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    sub_2bbe688(s, 0, 0xa5);
    *reinterpret_cast<uint64_t*>(s + 0xb0) = 0;
    *reinterpret_cast<uint64_t*>(s + 0xa8) = 0;
}

// ============================================================================
// 15. 0x21c7f80 (29 B) - sub_21c7f80
// Destroys the global GXSkinningTarget vector range (0x56a6cc8 .. 0x56a6cd0).
// ============================================================================
void sub_21c7f80() {
    auto* begin = static_cast<uint8_t*>(g_skin_target_vec_begin_56a6cc8);
    auto* end   = static_cast<uint8_t*>(g_skin_target_vec_end_56a6cd0);
    int64_t count = (end - begin) >> 3;
    sub_21c9c00(begin, end, count);
}

// ============================================================================
// 16. 0x21c7e80 (251 B) - sub_21c7e80
// Reallocates a pointer vector (0x08..0x20 layout) to `new_cap` elements.
// ============================================================================
void sub_21c7e80(void* vec, size_t new_cap) {
    auto* v = static_cast<uint8_t*>(vec);
    void* alloc = *reinterpret_cast<void**>(v + 0x20);
    auto* vtbl = *reinterpret_cast<uintptr_t**>(alloc);
    using AllocFn = void* (*)(void*, size_t, size_t);
    auto** new_buf = static_cast<void**>(
        reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(alloc, new_cap * 8, 8));

    if (new_buf == nullptr) {
        if (g_tls_tx_active_53e4318 != 0) {
            sub_2bbe978(&g_tls_tx_ctx_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, kErrTxFailed_4737d80);
    }

    void** old_begin = *reinterpret_cast<void***>(v + 0x08);
    void** old_end   = *reinterpret_cast<void***>(v + 0x10);
    void** free_ptr  = old_begin;
    void** end_ptr   = old_begin;
    if (old_begin != old_end) {
        void** dst = new_buf;
        for (void** cur = old_begin; cur != old_end; ++cur, ++dst) {
            if (dst != nullptr) {
                *dst = *cur;
            }
        }
        free_ptr = *reinterpret_cast<void***>(v + 0x08);
        end_ptr  = *reinterpret_cast<void***>(v + 0x10);
    }

    ptrdiff_t count = end_ptr - free_ptr;
    if (free_ptr != nullptr) {
        void* a2 = *reinterpret_cast<void**>(v + 0x20);
        auto* v2 = *reinterpret_cast<uintptr_t**>(a2);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(v2[0x70 / 8])(a2, free_ptr);
    }

    *reinterpret_cast<void***>(v + 0x18) = new_buf + new_cap;
    *reinterpret_cast<void***>(v + 0x10) = new_buf + count;
    *reinterpret_cast<void***>(v + 0x08) = new_buf;
}

// ============================================================================
// 17. 0x21c7d80 (251 B) - sub_21c7d80
// Reallocates a material pointer vector (0x08..0x20 layout) to `new_cap`.
// ============================================================================
void sub_21c7d80(void* vec, size_t new_cap) {
    auto* v = static_cast<uint8_t*>(vec);
    void* alloc = *reinterpret_cast<void**>(v + 0x20);
    auto* vtbl = *reinterpret_cast<uintptr_t**>(alloc);
    using AllocFn = void* (*)(void*, size_t, size_t);
    auto** new_buf = static_cast<void**>(
        reinterpret_cast<AllocFn>(vtbl[0x58 / 8])(alloc, new_cap * 8, 8));

    if (new_buf == nullptr) {
        if (g_tls_tx_active_53e4318 != 0) {
            sub_2bbe978(&g_tls_tx_ctx_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, kErrTxFailed_4737d80);
    }

    void** old_begin = *reinterpret_cast<void***>(v + 0x08);
    void** old_end   = *reinterpret_cast<void***>(v + 0x10);
    void** free_ptr  = old_begin;
    void** end_ptr   = old_begin;
    if (old_begin != old_end) {
        void** dst = new_buf;
        for (void** cur = old_begin; cur != old_end; ++cur, ++dst) {
            if (dst != nullptr) {
                *dst = *cur;
            }
        }
        free_ptr = *reinterpret_cast<void***>(v + 0x08);
        end_ptr  = *reinterpret_cast<void***>(v + 0x10);
    }

    ptrdiff_t count = end_ptr - free_ptr;
    if (free_ptr != nullptr) {
        void* a2 = *reinterpret_cast<void**>(v + 0x20);
        auto* v2 = *reinterpret_cast<uintptr_t**>(a2);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(v2[0x70 / 8])(a2, free_ptr);
    }

    *reinterpret_cast<void***>(v + 0x18) = new_buf + new_cap;
    *reinterpret_cast<void***>(v + 0x10) = new_buf + count;
    *reinterpret_cast<void***>(v + 0x08) = new_buf;
}

// ============================================================================
// 18. 0x21c7c30 (189 B) - sub_21c7c30
// Accumulates bitwise OR of material render flags (+0x18) into container +0x38.
// ============================================================================
void sub_21c7c30(void* mat_list) {
    auto* m = static_cast<uint8_t*>(mat_list);
    *reinterpret_cast<uint32_t*>(m + 0x38) = 0;

    auto** cur = *reinterpret_cast<uint8_t***>(m + 0x10);
    auto** end = *reinterpret_cast<uint8_t***>(m + 0x18);
    if (cur == end) {
        return;
    }

    size_t count = static_cast<size_t>(end - cur);
    size_t vec_n = count & ~static_cast<size_t>(3);
    size_t idx = 0;

    uint32_t acc0 = 0;
    uint32_t acc1 = 0;
    uint32_t acc2 = 0;
    uint32_t acc3 = 0;

    if (vec_n != 0) {
        for (; idx < vec_n; idx += 4) {
            acc0 |= *reinterpret_cast<const uint32_t*>(cur[idx + 0] + 0x18);
            acc1 |= *reinterpret_cast<const uint32_t*>(cur[idx + 1] + 0x18);
            acc2 |= *reinterpret_cast<const uint32_t*>(cur[idx + 2] + 0x18);
            acc3 |= *reinterpret_cast<const uint32_t*>(cur[idx + 3] + 0x18);
        }
        cur += vec_n;
    }

    uint32_t total_flags = acc0 | acc1 | acc2 | acc3;
    for (; cur != end; ++cur) {
        total_flags |= *reinterpret_cast<const uint32_t*>(*cur + 0x18);
    }
    *reinterpret_cast<uint32_t*>(m + 0x38) = total_flags;
}

// ============================================================================
// 19. 0x21c7890 (914 B) - sub_21c7890
// Populates debug inspector UI nodes for unique materials in GXFlverMaterialList.
// ============================================================================
void sub_21c7890(void* mat_list, void* inspector) {
    auto* m = static_cast<uint8_t*>(mat_list);
    auto* insp = static_cast<uint8_t*>(inspector);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    void** begin = *reinterpret_cast<void***>(m + 0x10);
    void** end   = *reinterpret_cast<void***>(m + 0x18);
    if (begin != end) {
        for (void** it = begin; it != end; ++it) {
            void** scan_begin = *reinterpret_cast<void***>(m + 0x10);
            void** found = scan_begin;
            while (found != it) {
                if (*found == *it) {
                    break;
                }
                ++found;
            }
            if (found != it) {
                continue;
            }

            auto* mat_obj = static_cast<uint8_t*>(*it);
            const char* name_utf8 = (*reinterpret_cast<uint64_t*>(mat_obj + 0x58) >= 8)
                ? *reinterpret_cast<const char* const*>(mat_obj + 0x40)
                : reinterpret_cast<const char*>(mat_obj + 0x40);

            alignas(16) uint8_t wstr_holder[16] = {};
            sub_231d3a0(wstr_holder, name_utf8);
            void* node_ok = sub_236c900(insp, wstr_holder, 0);

            auto* wstr_obj = *reinterpret_cast<uint8_t**>(wstr_holder);
            if (wstr_obj != nullptr) {
                if (--(*reinterpret_cast<int32_t*>(wstr_obj + 0x38)) == 0) {
                    void* wstr_alloc = g_wstr_allocator_5540e10;
                    if (*reinterpret_cast<uint64_t*>(wstr_obj + 0x20) >= 8) {
                        void* str_buf = *reinterpret_cast<void**>(wstr_obj + 0x08);
                        void* str_heap = *reinterpret_cast<void**>(wstr_obj + 0x28);
                        auto* sh_vtbl = *reinterpret_cast<uintptr_t**>(str_heap);
                        using FreeFn = void (*)(void*, void*);
                        reinterpret_cast<FreeFn>(sh_vtbl[0x70 / 8])(str_heap, str_buf);
                    }
                    *reinterpret_cast<uint64_t*>(wstr_obj + 0x20) = 7;
                    *reinterpret_cast<uint64_t*>(wstr_obj + 0x18) = 0;
                    *reinterpret_cast<uint16_t*>(wstr_obj + 0x08) = 0;
                    auto* wa_vtbl = *reinterpret_cast<uintptr_t**>(wstr_alloc);
                    using FreeFn = void (*)(void*, void*);
                    reinterpret_cast<FreeFn>(wa_vtbl[0x70 / 8])(wstr_alloc, wstr_obj);
                }
            }

            if (node_ok != nullptr) {
                void* cur_mat = *it;
                auto* m_vtbl = *reinterpret_cast<uintptr_t**>(cur_mat);
                using InspectMatFn = void (*)(void*, void*);
                reinterpret_cast<InspectMatFn>(m_vtbl[0x40 / 8])(cur_mat, insp);

                // Pop and release inspector stack node at insp + 0x08..0x10
                void** stk_begin = *reinterpret_cast<void***>(insp + 0x08);
                void** stk_end   = *reinterpret_cast<void***>(insp + 0x10);
                if (stk_begin != stk_end) {
                    auto* ref_obj = static_cast<uint8_t*>(stk_end[-1]);
                    if (ref_obj != nullptr) {
                        uint32_t rc_word = *reinterpret_cast<uint32_t*>(ref_obj + 0x08);
                        uint32_t strong_dec = (rc_word + 0x7FFFu) & 0x7FFFu;
                        uint32_t new_word = (rc_word & 0xFFFF8000u) | strong_dec;
                        *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = new_word;
                        if (strong_dec == 0 && (rc_word & 0x40000000u) != 0) {
                            auto* r_vtbl = *reinterpret_cast<uintptr_t**>(ref_obj);
                            reinterpret_cast<void (*)(void*)>(r_vtbl[0x18 / 8])(ref_obj);
                            new_word = *reinterpret_cast<uint32_t*>(ref_obj + 0x08) & 0xBFFFFFFFu;
                            *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = new_word;
                        }
                        uint32_t weak_dec = ((new_word >> 15) + 0x7FFFu) & 0x7FFFu;
                        uint32_t final_word = (new_word & 0xC0007FFFu) | (weak_dec << 15);
                        *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = final_word;
                        if (weak_dec == 0) {
                            *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = new_word | 0x3FFF8000u;
                            auto* r_vtbl = *reinterpret_cast<uintptr_t**>(ref_obj);
                            reinterpret_cast<void (*)(void*)>(r_vtbl[0x20 / 8])(ref_obj);
                        }
                        stk_end[-1] = nullptr;
                        stk_end = *reinterpret_cast<void***>(insp + 0x10);
                    }
                    *reinterpret_cast<void***>(insp + 0x10) = stk_end - 1;
                }
            }
        }
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 20. 0x21c7880 (11 B) - sub_21c7880
// Gets material pointer at `idx` from GXFlverMaterialList (+0x10).
// ============================================================================
void* sub_21c7880(const void* mat_list, uint32_t idx) {
    const auto* m = static_cast<const uint8_t*>(mat_list);
    void* const* arr = *reinterpret_cast<void* const* const*>(m + 0x10);
    return arr[idx];
}

// ============================================================================
// 21. 0x21c7870 (13 B) - sub_21c7870
// Gets number of materials in GXFlverMaterialList ((+0x18 - +0x10) >> 3).
// ============================================================================
uint64_t sub_21c7870(const void* mat_list) {
    const auto* m = static_cast<const uint8_t*>(mat_list);
    const auto* end   = *reinterpret_cast<const uint8_t* const*>(m + 0x18);
    const auto* begin = *reinterpret_cast<const uint8_t* const*>(m + 0x10);
    return static_cast<uint64_t>(end - begin) >> 3;
}

// ============================================================================
// 22. 0x21c7660 (515 B) - sub_21c7660
// Builds GXFlverMaterialList from FLVER resource material descriptors.
// ============================================================================
uint8_t sub_21c7660(void* mat_list, void* flver_res_ptr, void* ctx_arg, void* factory) {
    auto* m = static_cast<uint8_t*>(mat_list);
    auto* flver_holder = static_cast<uint8_t**>(flver_res_ptr);
    auto* fac = static_cast<uint8_t*>(factory);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    void* vec_ptr = m + 0x08;
    uint32_t mat_count = *reinterpret_cast<uint32_t*>(*flver_holder + 0x18);
    size_t cur_cap = static_cast<size_t>(
        (*reinterpret_cast<uint8_t**>(m + 0x20) - *reinterpret_cast<uint8_t**>(m + 0x10)) >> 3);
    if (cur_cap < mat_count) {
        sub_21c7d80(vec_ptr, mat_count);
        mat_count = *reinterpret_cast<uint32_t*>(*flver_holder + 0x18);
    }

    if (mat_count != 0) {
        for (uint32_t i = 0; i < mat_count; ++i) {
            auto* f_vtbl = *reinterpret_cast<uintptr_t**>(fac);
            using CreateMatFn = uint8_t* (*)(void*, void*, uint32_t, void*);
            uint8_t* new_mat = reinterpret_cast<CreateMatFn>(f_vtbl[0x30 / 8])(fac, flver_holder, i, ctx_arg);
            if (new_mat != nullptr) {
                *reinterpret_cast<void**>(new_mat + 0x78) = m;
            }

            void** end_ptr = *reinterpret_cast<void***>(m + 0x18);
            void** cap_ptr = *reinterpret_cast<void***>(m + 0x20);
            if (end_ptr == cap_ptr) {
                void** begin_ptr = *reinterpret_cast<void***>(m + 0x10);
                size_t sz = static_cast<size_t>(end_ptr - begin_ptr);
                size_t half = sz >> 1;
                size_t new_cap = (0x1FFFFFFFFFFFFFFFull - half < sz) ? 0 : (sz + half);
                if (new_cap < sz + 1) new_cap = sz + 1;
                sub_21c7d80(vec_ptr, new_cap);
                end_ptr = *reinterpret_cast<void***>(m + 0x18);
            }
            if (end_ptr != nullptr) {
                *end_ptr = new_mat;
            }
            *reinterpret_cast<void***>(m + 0x18) = end_ptr + 1;
        }
    }

    void** begin_ptr = *reinterpret_cast<void***>(m + 0x10);
    void** end_ptr   = *reinterpret_cast<void***>(m + 0x18);
    *reinterpret_cast<uint32_t*>(m + 0x50) = static_cast<uint32_t>(end_ptr - begin_ptr);

    if (begin_ptr != end_ptr) {
        uint32_t flags38 = *reinterpret_cast<uint32_t*>(m + 0x38);
        __m128 mask3c = _mm_loadu_ps(reinterpret_cast<const float*>(m + 0x3c));
        uint8_t byte4c = m[0x4c];
        uint8_t byte4d = m[0x4d];

        for (void** it = begin_ptr; it != end_ptr; ++it) {
            const auto* mat_elem = static_cast<const uint8_t*>(*it);
            flags38 |= *reinterpret_cast<const uint32_t*>(mat_elem + 0x18);
            mask3c = _mm_or_ps(mask3c, _mm_loadu_ps(reinterpret_cast<const float*>(mat_elem + 0x1e)));
            _mm_storeu_ps(reinterpret_cast<float*>(m + 0x3c), mask3c);
            byte4c |= mat_elem[0x2e];
            m[0x4c] = byte4c;
            byte4d |= mat_elem[0x2f];
            m[0x4d] = byte4d;
        }
        *reinterpret_cast<uint32_t*>(m + 0x38) = flags38;
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return 1;
}

// ============================================================================
// 23. 0x21c7620 (64 B) - sub_21c7620
// Releases all material elements in GXFlverMaterialList and resets end pointer.
// ============================================================================
void sub_21c7620(void* mat_list) {
    auto* m = static_cast<uint8_t*>(mat_list);
    void** cur = *reinterpret_cast<void***>(m + 0x10);
    void** end = *reinterpret_cast<void***>(m + 0x18);
    if (cur != end) {
        for (; cur != *reinterpret_cast<void***>(m + 0x18); ++cur) {
            void* elem = *cur;
            auto* vtbl = *reinterpret_cast<uintptr_t**>(elem);
            reinterpret_cast<void (*)(void*)>(vtbl[0x48 / 8])(elem);
        }
        cur = *reinterpret_cast<void***>(m + 0x10);
    }
    *reinterpret_cast<void***>(m + 0x18) = cur;
}

// ============================================================================
// 24. 0x21c75a0 (126 B) - sub_21c75a0
// Non-deleting destructor for GXFlverMaterialList (vtable 0x53b6eb0).
// ============================================================================
void sub_21c75a0(void* mat_list) {
    auto* m = static_cast<uint8_t*>(mat_list);
    *reinterpret_cast<void**>(m + 0x00) = g_vtable_53b6eb0;

    void** cur = *reinterpret_cast<void***>(m + 0x10);
    if (cur != *reinterpret_cast<void***>(m + 0x18)) {
        for (; cur != *reinterpret_cast<void***>(m + 0x18); ++cur) {
            void* elem = *cur;
            auto* vtbl = *reinterpret_cast<uintptr_t**>(elem);
            reinterpret_cast<void (*)(void*)>(vtbl[0x48 / 8])(elem);
        }
        cur = *reinterpret_cast<void***>(m + 0x10);
    }
    *reinterpret_cast<void***>(m + 0x18) = cur;

    if (cur != nullptr) {
        void* alloc = *reinterpret_cast<void**>(m + 0x28);
        auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, cur);
        *reinterpret_cast<uint64_t*>(m + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(m + 0x18) = 0;
        *reinterpret_cast<uint64_t*>(m + 0x10) = 0;
    }
}

// ============================================================================
// 25. 0x21c7540 (89 B) - sub_21c7540
// Deleting destructor for GXFlverMaterialList (vtable 0x53b6eb0).
// ============================================================================
void sub_21c7540(void* mat_list) {
    auto* m = static_cast<uint8_t*>(mat_list);
    *reinterpret_cast<void**>(m + 0x00) = g_vtable_53b6eb0;

    void** cur = *reinterpret_cast<void***>(m + 0x10);
    if (cur != *reinterpret_cast<void***>(m + 0x18)) {
        for (; cur != *reinterpret_cast<void***>(m + 0x18); ++cur) {
            void* elem = *cur;
            auto* vtbl = *reinterpret_cast<uintptr_t**>(elem);
            reinterpret_cast<void (*)(void*)>(vtbl[0x48 / 8])(elem);
        }
        cur = *reinterpret_cast<void***>(m + 0x10);
    }
    *reinterpret_cast<void***>(m + 0x18) = cur;

    if (cur != nullptr) {
        void* alloc = *reinterpret_cast<void**>(m + 0x28);
        auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, cur);
    }
    sub_2bbe588(m);
}

// ============================================================================
// 26. 0x21c7460 (217 B) - sub_21c7460
// Constructor for GXFlverMaterialList (0x58 bytes, vtable 0x53b6eb0).
// ============================================================================
void sub_21c7460(void* mat_list, void* owner_ctx) {
    auto* m = static_cast<uint8_t*>(mat_list);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    *reinterpret_cast<void**>(m + 0x00) = g_vtable_53b6eb0;
    void* heap = g_default_heap_cache_5540670;
    if (heap == nullptr) {
        heap = sub_2ba2b10();
        g_default_heap_cache_5540670 = heap;
    }

    uint8_t flags_buf[8] = {};
    auto* h_vtbl = *reinterpret_cast<uintptr_t**>(heap);
    using CheckFn = void (*)(void*, void*, uint32_t);
    reinterpret_cast<CheckFn>(h_vtbl[0x20 / 8])(flags_buf, heap, 0);
    if ((flags_buf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, kErrIncompatHeap_4737dc0);
    }

    *reinterpret_cast<uint64_t*>(m + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(m + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(m + 0x10) = 0;
    *reinterpret_cast<void**>(m + 0x28)    = heap;
    *reinterpret_cast<void**>(m + 0x30)    = owner_ctx;
    *reinterpret_cast<uint32_t*>(m + 0x50) = 0;
    *reinterpret_cast<uint16_t*>(m + 0x4c) = 0;
    *reinterpret_cast<uint32_t*>(m + 0x48) = 0;
    *reinterpret_cast<uint64_t*>(m + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(m + 0x38) = 0;

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 27. 0x21c7450 (3 B) - sub_21c7450
// Virtual stub returning 0 (false / null) on GXFlverModelResource.
// ============================================================================
uint32_t sub_21c7450() {
    return 0;
}

// ============================================================================
// 28. 0x21c7010 (942 B) - sub_21c7010
// Populates debug inspector UI properties for GXFlverModelResource ("ObjNum",
// "UsingObjNum", "PrimNum", "MaterialNum", "LodLevel", "UseResetIndex",
// "DisableObjId", "PrimAABB", "Objects", "Materials").
// ============================================================================
void sub_21c7010(void* self, void* inspector) {
    auto* s = static_cast<uint8_t*>(self);
    auto* insp = static_cast<uint8_t*>(inspector);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    struct WStrProp {
        uint64_t        zero0;
        const char16_t* name;
    };

    auto add_int32 = [&](const char16_t* name, size_t off) {
        WStrProp p = { 0, name };
        sub_29055d0(insp, &p, s + off);
    };
    auto add_uint32 = [&](const char16_t* name, size_t off) {
        WStrProp p = { 0, name };
        sub_2905e60(insp, &p, s + off);
    };
    auto add_bool = [&](const char16_t* name, size_t off) {
        WStrProp p = { 0, name };
        sub_219db10(insp, &p, s + off);
    };

    add_int32(reinterpret_cast<const char16_t*>(GX_VMA(0x49c3944ULL)), 0x18);
    add_int32(reinterpret_cast<const char16_t*>(GX_VMA(0x49c3952ULL)), 0x1c);
    add_int32(reinterpret_cast<const char16_t*>(GX_VMA(0x49c396aULL)), 0xa0);
    add_uint32(reinterpret_cast<const char16_t*>(GX_VMA(0x49c397aULL)), 0x260);
    add_uint32(reinterpret_cast<const char16_t*>(GX_VMA(0x49c3992ULL)), 0x204);
    add_bool(reinterpret_cast<const char16_t*>(GX_VMA(0x49c39a4ULL)), 0x1f8);
    add_bool(reinterpret_cast<const char16_t*>(GX_VMA(0x49c39c0ULL)), 0x200);
    if (s[0x201] != 0) {
        add_bool(reinterpret_cast<const char16_t*>(GX_VMA(0x49c39daULL)), 0x201);
    }

    auto pop_inspector_node = [&]() {
        void** stk_begin = *reinterpret_cast<void***>(insp + 0x08);
        void** stk_end   = *reinterpret_cast<void***>(insp + 0x10);
        if (stk_begin != stk_end) {
            auto* ref_obj = static_cast<uint8_t*>(stk_end[-1]);
            if (ref_obj != nullptr) {
                uint32_t rc_word = *reinterpret_cast<uint32_t*>(ref_obj + 0x08);
                uint32_t strong_dec = (rc_word + 0x7FFFu) & 0x7FFFu;
                uint32_t new_word = (rc_word & 0xFFFF8000u) | strong_dec;
                *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = new_word;
                if (strong_dec == 0 && (rc_word & 0x40000000u) != 0) {
                    auto* r_vtbl = *reinterpret_cast<uintptr_t**>(ref_obj);
                    reinterpret_cast<void (*)(void*)>(r_vtbl[0x18 / 8])(ref_obj);
                    new_word = *reinterpret_cast<uint32_t*>(ref_obj + 0x08) & 0xBFFFFFFFu;
                    *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = new_word;
                }
                uint32_t weak_dec = ((new_word >> 15) + 0x7FFFu) & 0x7FFFu;
                uint32_t final_word = (new_word & 0xC0007FFFu) | (weak_dec << 15);
                *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = final_word;
                if (weak_dec == 0) {
                    *reinterpret_cast<uint32_t*>(ref_obj + 0x08) = new_word | 0x3FFF8000u;
                    auto* r_vtbl = *reinterpret_cast<uintptr_t**>(ref_obj);
                    reinterpret_cast<void (*)(void*)>(r_vtbl[0x20 / 8])(ref_obj);
                }
                stk_end[-1] = nullptr;
                stk_end = *reinterpret_cast<void***>(insp + 0x10);
            }
            *reinterpret_cast<void***>(insp + 0x10) = stk_end - 1;
        }
    };

    WStrProp obj_grp = { 0, reinterpret_cast<const char16_t*>(GX_VMA(0x49c39ecULL)) };
    if (sub_236c900(insp, &obj_grp, 0) != nullptr) {
        sub_e523b0(s + 0x10, insp);
        pop_inspector_node();
    }

    WStrProp mat_grp = { 0, reinterpret_cast<const char16_t*>(GX_VMA(0x49c39fcULL)) };
    if (sub_236c900(insp, &mat_grp, 0) != nullptr) {
        sub_21c7890(s + 0x210, insp);
        pop_inspector_node();
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 29. 0x21c6ca0 (873 B) - sub_21c6ca0
// Computes world-space merged AABB (min at out_aabb+0x00, max at out_aabb+0x10)
// across all valid submesh local AABBs transformed by bone + world matrices.
// ============================================================================
void sub_21c6ca0(const void* self, float* out_aabb,
                 const float* world_mtx, void* bone_provider) {
    const auto* s = static_cast<const uint8_t*>(self);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    int32_t obj_count = *reinterpret_cast<const int32_t*>(s + 0x18);
    __m128 aabb_max = _mm_load_ps(kSimdNegMax_4920bf0);
    __m128 aabb_min = _mm_load_ps(kSimdPosMax_4920c00);

    if (obj_count > 0) {
        __m128 wm0 = _mm_loadu_ps(world_mtx + 0);
        __m128 wm1 = _mm_loadu_ps(world_mtx + 4);
        __m128 wm2 = _mm_loadu_ps(world_mtx + 8);
        __m128 wm3 = _mm_loadu_ps(world_mtx + 12);

        const __m128 mask_xyz = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(kMaskXYZ_4920c10)));
        const __m128 c_0001   = _mm_load_ps(kSimd0001_4920c20);
        const __m128i zero_i  = _mm_setzero_si128();

        for (int32_t i = 0; i < obj_count; ++i) {
            const auto* obj_base = *reinterpret_cast<const uint8_t* const*>(s + 0x20) + static_cast<size_t>(i) * 0x30;
            __m128 b_min = _mm_loadu_ps(reinterpret_cast<const float*>(obj_base));
            __m128 b_max = _mm_loadu_ps(reinterpret_cast<const float*>(obj_base + 0x10));

            // Check if b_max < b_min in any XYZ component (invalid/empty AABB)
            __m128 cmp = _mm_cmplt_ps(b_max, b_min);
            __m128i shuf = _mm_shuffle_epi32(_mm_castps_si128(cmp), 0x24);
            __m128 eq = _mm_castsi128_ps(_mm_cmpeq_epi32(shuf, zero_i));
            if (_mm_movemask_ps(eq) == 0x0F) {
                alignas(16) float local_mtx[16];
                auto* bp_vtbl = *reinterpret_cast<uintptr_t**>(bone_provider);
                using GetBoneMtxFn = void (*)(void*, int32_t, float*);
                reinterpret_cast<GetBoneMtxFn>(bp_vtbl[0])(bone_provider, i, local_mtx);

                auto mul_wm = [&](const __m128& row) -> __m128 {
                    __m128 x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x00));
                    __m128 y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0x55));
                    __m128 z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xaa));
                    __m128 w = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row), 0xff));
                    return _mm_add_ps(_mm_add_ps(_mm_mul_ps(wm0, x), _mm_mul_ps(wm2, z)),
                                      _mm_add_ps(_mm_mul_ps(wm1, y), _mm_mul_ps(wm3, w)));
                };

                __m128 tm0 = mul_wm(_mm_load_ps(local_mtx + 0));
                __m128 tm1 = mul_wm(_mm_load_ps(local_mtx + 4));
                __m128 tm2 = mul_wm(_mm_load_ps(local_mtx + 8));
                __m128 tm3 = mul_wm(_mm_load_ps(local_mtx + 12));

                __m128 min_xyz1 = _mm_or_ps(_mm_and_ps(b_min, mask_xyz), c_0001);
                __m128 extent   = _mm_sub_ps(b_max, b_min);

                __m128 ex = _mm_mul_ps(tm0, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(extent), 0x00)));
                __m128 ey = _mm_mul_ps(tm1, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(extent), 0x55)));
                __m128 ez = _mm_mul_ps(tm2, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(extent), 0xaa)));

                __m128 origin = _mm_add_ps(
                    _mm_add_ps(_mm_mul_ps(tm0, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(min_xyz1), 0x00))),
                               _mm_mul_ps(tm1, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(min_xyz1), 0x55)))),
                    _mm_add_ps(_mm_mul_ps(tm2, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(min_xyz1), 0xaa))),
                               _mm_mul_ps(tm3, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(min_xyz1), 0xff)))));

                __m128 zero_f = _mm_setzero_ps();
                __m128 mx_lt = _mm_cmplt_ps(ex, zero_f);
                __m128 my_lt = _mm_cmplt_ps(ey, zero_f);
                __m128 mz_lt = _mm_cmplt_ps(ez, zero_f);

                __m128 neg_sum = _mm_add_ps(_mm_add_ps(_mm_and_ps(ex, mx_lt), _mm_and_ps(ey, my_lt)), _mm_and_ps(ez, mz_lt));
                __m128 pos_sum = _mm_add_ps(_mm_add_ps(_mm_andnot_ps(mx_lt, ex), _mm_andnot_ps(my_lt, ey)), _mm_andnot_ps(mz_lt, ez));

                __m128 w_min = _mm_add_ps(origin, neg_sum);
                __m128 w_max = _mm_add_ps(origin, pos_sum);
                __m128 orig_xyz1 = _mm_or_ps(_mm_and_ps(origin, mask_xyz), c_0001);

                w_min = _mm_min_ps(orig_xyz1, w_min);
                w_max = _mm_max_ps(orig_xyz1, w_max);

                w_min = _mm_or_ps(_mm_and_ps(w_min, mask_xyz), c_0001);
                aabb_min = _mm_min_ps(w_min, aabb_min);

                w_max = _mm_or_ps(_mm_and_ps(w_max, mask_xyz), c_0001);
                aabb_max = _mm_max_ps(w_max, aabb_max);
            }
        }
    }

    _mm_storeu_ps(out_aabb + 0, aabb_min);
    _mm_storeu_ps(out_aabb + 4, aabb_max);

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 30. 0x21c6ae0 (437 B) - sub_21c6ae0
// Clears and resets all sub-containers of GXFlverModelResource.
// ============================================================================
void sub_21c6ae0(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    sub_e52310(s + 0x10);
    sub_e51640(s + 0x38);
    sub_e4e140(s + 0x78);
    sub_e50cc0(s + 0x98);
    sub_e4f570(s + 0xf8);
    sub_e52e50(s + 0x160);
    sub_e4f960(s + 0x188);
    sub_e51640(s + 0x1d8);

    s[0x1f8] = 0;
    *reinterpret_cast<uint32_t*>(s + 0x1fc) = 0xFFFFFFFFu;
    *reinterpret_cast<uint32_t*>(s + 0x204) = 0;
    *reinterpret_cast<uint32_t*>(s + 0x208) = 0;

    void** mat_cur = *reinterpret_cast<void***>(s + 0x220);
    if (mat_cur != *reinterpret_cast<void***>(s + 0x228)) {
        for (; mat_cur != *reinterpret_cast<void***>(s + 0x228); ++mat_cur) {
            void* elem = *mat_cur;
            auto* vtbl = *reinterpret_cast<uintptr_t**>(elem);
            reinterpret_cast<void (*)(void*)>(vtbl[0x48 / 8])(elem);
        }
        mat_cur = *reinterpret_cast<void***>(s + 0x220);
    }
    *reinterpret_cast<void***>(s + 0x228) = mat_cur;

    uint8_t out_iter[24] = {};
    void* vec_end   = *reinterpret_cast<void**>(s + 0x288);
    void* vec_begin = *reinterpret_cast<void**>(s + 0x280);
    sub_21ce1b0(out_iter, s + 0x278, vec_end, 0, 0, 0, 0, 0, vec_begin, 0, 0, vec_end);

    auto* lod_arr = *reinterpret_cast<uint8_t**>(s + 0x2b8);
    if (lod_arr != nullptr) {
        int32_t lod_cnt = *reinterpret_cast<int32_t*>(s + 0x2b0);
        if (lod_cnt > 0) {
            uint8_t* p = lod_arr + static_cast<size_t>(lod_cnt - 1) * 0x58;
            for (int32_t i = lod_cnt; i > 0; --i, p -= 0x58) {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(p);
                reinterpret_cast<void (*)(void*)>(vtbl[0x08 / 8])(p);
            }
            lod_arr = *reinterpret_cast<uint8_t**>(s + 0x2b8);
        }
        void* alloc = *reinterpret_cast<void**>(s + 0x2a8);
        auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, lod_arr);
        *reinterpret_cast<uint64_t*>(s + 0x2b8) = 0;
        *reinterpret_cast<uint64_t*>(s + 0x2b0) = 0;
    }

    s[0x2cc] = 0;
    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 31. 0x21c67a0 (819 B) - sub_21c67a0
// Computes inverse bind pose 3x4 matrices, extracts LOD level mask (+0x204/+0x208),
// and binds primitives to submeshes, vertex declarations, vertex buffers, and materials.
// ============================================================================
void sub_21c67a0(void* self, void* bind_ctx) {
    auto* s = static_cast<uint8_t*>(self);

    if (s[0xb0] != 0) {
        int32_t bone_count = *reinterpret_cast<int32_t*>(s + 0x4c);
        sub_e512b0(s + 0x1d8, bone_count);
        sub_e516e0(s + 0x1d8, s + 0x38, s + 0x10);

        int32_t total_bones = *reinterpret_cast<int32_t*>(s + 0x4c);
        if (total_bones > 0) {
            const __m128i sign_hi32 = _mm_set_epi32(0, static_cast<int>(0x80000000u), 0, 0);
            const __m128  sign_lo0  = _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, static_cast<int>(0x80000000u)));
            const __m128i sign_lo1  = _mm_set_epi32(0, 0, static_cast<int>(0x80000000u), 0);
            const __m128  sign_hi1  = _mm_castsi128_ps(_mm_set_epi32(static_cast<int>(0x80000000u), 0, 0, 0));

            for (int32_t i = 0; i < total_bones; ++i) {
                auto* mtx_base = *reinterpret_cast<float**>(s + 0x1f0) + static_cast<size_t>(i) * 12;
                __m128 r0 = _mm_loadu_ps(mtx_base + 0);
                __m128 r1 = _mm_loadu_ps(mtx_base + 4);
                __m128 r2 = _mm_loadu_ps(mtx_base + 8);

                __m128 xmm4  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2), 0x50));
                __m128 xmm2  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r0), 0x50));
                __m128 xmm5  = _mm_movehl_ps(r1, r1);
                __m128 xmm8  = _mm_shuffle_ps(r2, r0, 0x88);
                __m128 xmm0  = _mm_load_ps(kSimd0001_4920ba0);
                __m128 xmm9  = _mm_shuffle_ps(xmm0, r1, 0xdd);
                xmm4 = _mm_mul_ps(xmm4, _mm_load_ps(kSimd0101_4920bb0));
                xmm2 = _mm_mul_ps(xmm2, xmm5);
                xmm8 = _mm_mul_ps(xmm8, xmm9);

                __m128 xmm6  = _mm_movehl_ps(r2, r2);
                xmm0 = _mm_movehl_ps(r0, r0);
                xmm5 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1), 0x50));
                xmm9 = _mm_shuffle_ps(r2, r0, 0xdd);
                __m128 xmm14 = _mm_setzero_ps();
                __m128 xmm12 = _mm_shuffle_ps(xmm14, r1, 0x88);
                xmm6 = _mm_mul_ps(xmm6, xmm14);
                xmm4 = _mm_sub_ps(xmm4, xmm6);
                xmm0 = _mm_sub_ps(xmm2, _mm_mul_ps(xmm0, xmm5));
                xmm2 = _mm_sub_ps(xmm8, _mm_mul_ps(xmm9, xmm12));

                xmm8  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1), 0x26));
                xmm12 = _mm_shuffle_ps(xmm2, xmm4, 0xd4);
                xmm9  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r0), 0xc8));
                xmm5  = _mm_shuffle_ps(xmm4, xmm2, 0x1e);
                xmm6  = _mm_shuffle_ps(xmm2, xmm0, 0xde);
                __m128 xmm13 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2), 0xc8));
                xmm8  = _mm_mul_ps(xmm8, xmm12);
                xmm12 = _mm_shuffle_ps(xmm0, xmm2, 0xbe);
                xmm5  = _mm_mul_ps(xmm9, xmm5);
                xmm14 = _mm_mul_ps(xmm6, xmm14);
                xmm9  = _mm_mul_ps(xmm13, xmm12);

                xmm12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1), 0x79));
                xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x4c));
                xmm6  = _mm_mul_ps(xmm12, xmm6);
                xmm12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r0), 0x6d));
                xmm8  = _mm_sub_ps(xmm8, xmm6);

                xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x58));
                xmm6  = _mm_mul_ps(xmm12, xmm6);
                xmm12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x4c));
                xmm13 = _mm_sub_ps(xmm5, xmm6);

                xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2), 0x6d));
                xmm5  = _mm_mul_ps(xmm12, _mm_load_ps(kSimd0010_4920bc0));
                xmm12 = _mm_sub_ps(xmm14, xmm5);

                xmm5  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x58));
                xmm5  = _mm_mul_ps(xmm6, xmm5);
                xmm9  = _mm_sub_ps(xmm9, xmm5);

                xmm6  = _mm_shuffle_ps(xmm4, xmm2, 0x1a);
                xmm4  = _mm_shuffle_ps(xmm2, xmm4, 0xc4);
                xmm5  = _mm_shuffle_ps(xmm0, xmm2, 0xba);
                xmm0  = _mm_shuffle_ps(xmm2, xmm0, 0xce);

                xmm2  = _mm_castsi128_ps(_mm_xor_si128(_mm_shuffle_epi32(_mm_castps_si128(r1), 0xcc), sign_hi32));
                xmm2  = _mm_add_ps(_mm_mul_ps(xmm2, xmm6), xmm8);

                xmm6  = _mm_castsi128_ps(_mm_xor_si128(_mm_shuffle_epi32(_mm_castps_si128(r0), 0x36), sign_lo1));
                xmm4  = _mm_add_ps(_mm_mul_ps(xmm4, xmm6), xmm13);

                xmm5  = _mm_add_ps(_mm_mul_ps(xmm5, _mm_load_ps(kSimd01n01_4920bd0)), xmm12);

                __m128 xmm1 = _mm_castsi128_ps(_mm_xor_si128(_mm_shuffle_epi32(_mm_castps_si128(r2), 0x36), sign_lo1));
                xmm0  = _mm_add_ps(_mm_mul_ps(xmm0, xmm1), xmm9);

                xmm1  = _mm_xor_ps(xmm2, sign_lo0);
                xmm2  = _mm_xor_ps(xmm4, sign_hi1);
                xmm4  = _mm_xor_ps(xmm5, sign_lo0);
                xmm0  = _mm_xor_ps(xmm0, sign_hi1);

                xmm1  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x39));
                xmm2  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x39));
                xmm4  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x39));
                xmm0  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x39));

                __m128 xmm3 = _mm_mul_ps(r0, xmm1);
                xmm3 = _mm_hadd_ps(xmm3, xmm3);
                xmm3 = _mm_hadd_ps(xmm3, xmm3);
                xmm3 = _mm_div_ps(_mm_load_ps(kSimd1111_4920be0), xmm3);

                xmm1 = _mm_mul_ps(xmm3, xmm1);
                xmm2 = _mm_mul_ps(xmm3, xmm2);
                xmm4 = _mm_mul_ps(xmm3, xmm4);
                xmm0 = _mm_mul_ps(xmm3, xmm0);

                xmm3 = _mm_movelh_ps(xmm1, xmm2);
                xmm1 = _mm_movehl_ps(xmm2, xmm1);
                xmm2 = _mm_movelh_ps(xmm4, xmm0);
                xmm0 = _mm_movehl_ps(xmm0, xmm4);

                xmm4 = _mm_shuffle_ps(xmm3, xmm2, 0x88);
                xmm2 = _mm_shuffle_ps(xmm3, xmm2, 0xdd);
                xmm0 = _mm_shuffle_ps(xmm1, xmm0, 0x88);

                _mm_storeu_ps(mtx_base + 0, xmm4);
                _mm_storeu_ps(mtx_base + 4, xmm2);
                _mm_storeu_ps(mtx_base + 8, xmm0);
            }
        }
    }

    *reinterpret_cast<uint64_t*>(s + 0x204) = 0;
    const auto* sub_begin = *reinterpret_cast<const uint8_t* const*>(s + 0x138);
    const auto* sub_end   = *reinterpret_cast<const uint8_t* const*>(s + 0x140);
    uint32_t sub_count = static_cast<uint32_t>((sub_end - sub_begin) >> 5);
    uint32_t max_lod = 1;
    if (sub_count != 0) {
        uint32_t lod_mask = 0;
        max_lod = 0;
        for (uint32_t i = 0; i < sub_count; ++i) {
            uint32_t flags = *reinterpret_cast<const uint32_t*>(sub_begin + static_cast<size_t>(i) * 32);
            uint32_t lod_val = _bextr_u32(flags, 24, 4); // 0x418: start=24, len=4
            if (lod_val > max_lod) {
                max_lod = lod_val;
            }
            if (static_cast<int32_t>(flags) < 0) {
                lod_mask |= (1u << lod_val);
                *reinterpret_cast<uint32_t*>(s + 0x208) = lod_mask;
            }
        }
        *reinterpret_cast<uint32_t*>(s + 0x204) = max_lod;
        ++max_lod;
    }

    *reinterpret_cast<uint32_t*>(s + 0x204) = max_lod;
    sub_e51070(s + 0x98, s + 0xf8);
    sub_e50d20(s + 0x98, s + 0x188, s + 0x160, s + 0x210, bind_ctx);
}

// ============================================================================
// 32. 0x21c6790 (6 B) - sub_21c6790
// Virtual dispatch thunk jumping to vtable[+0x20] (e.g. ReleaseRef / Destroy).
// ============================================================================
void sub_21c6790(void* self) {
    auto* vtbl = *reinterpret_cast<uintptr_t**>(self);
    reinterpret_cast<void (*)(void*)>(vtbl[0x20 / 8])(self);
}

// ============================================================================
// 33. 0x21c6410 (887 B) - sub_21c6410
// Non-deleting destructor for GXFlverModelResource (0x308 bytes, vtable 0x53b6e70).
// ============================================================================
void sub_21c6410(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    *reinterpret_cast<void**>(s + 0x00) = g_vtable_53b6e70;
    sub_e52310(s + 0x10);
    sub_e51640(s + 0x38);
    sub_e4e140(s + 0x78);
    sub_e50cc0(s + 0x98);
    sub_e4f570(s + 0xf8);
    sub_e52e50(s + 0x160);
    sub_e4f960(s + 0x188);
    sub_e51640(s + 0x1d8);

    s[0x1f8] = 0;
    *reinterpret_cast<uint32_t*>(s + 0x1fc) = 0xFFFFFFFFu;
    *reinterpret_cast<uint32_t*>(s + 0x204) = 0;
    *reinterpret_cast<uint32_t*>(s + 0x208) = 0;

    uint8_t out_iter[24] = {};
    void* vec_end   = *reinterpret_cast<void**>(s + 0x288);
    void* vec_begin = *reinterpret_cast<void**>(s + 0x280);
    sub_21ce1b0(out_iter, s + 0x278, vec_end, 0, 0, 0, 0, 0, vec_begin, 0, 0, vec_end);

    auto* lod_arr = *reinterpret_cast<uint8_t**>(s + 0x2b8);
    if (lod_arr != nullptr) {
        int32_t lod_cnt = *reinterpret_cast<int32_t*>(s + 0x2b0);
        if (lod_cnt > 0) {
            uint8_t* p = lod_arr + static_cast<size_t>(lod_cnt - 1) * 0x58;
            for (int32_t i = lod_cnt; i > 0; --i, p -= 0x58) {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(p);
                reinterpret_cast<void (*)(void*)>(vtbl[0x08 / 8])(p);
            }
            lod_arr = *reinterpret_cast<uint8_t**>(s + 0x2b8);
        }
        void* alloc = *reinterpret_cast<void**>(s + 0x2a8);
        auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(a_vtbl[0x70 / 8])(alloc, lod_arr);
        *reinterpret_cast<uint64_t*>(s + 0x2b8) = 0;
        *reinterpret_cast<uint64_t*>(s + 0x2b0) = 0;
    }

    s[0x2cc] = 0;
    if (*reinterpret_cast<uint64_t*>(s + 0x2f0) >= 8) {
        void* wstr_buf = *reinterpret_cast<void**>(s + 0x2d8);
        void* wstr_alloc = *reinterpret_cast<void**>(s + 0x2f8);
        auto* wa_vtbl = *reinterpret_cast<uintptr_t**>(wstr_alloc);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(wa_vtbl[0x70 / 8])(wstr_alloc, wstr_buf);
    }
    *reinterpret_cast<uint64_t*>(s + 0x2f0) = 7;
    *reinterpret_cast<uint64_t*>(s + 0x2e8) = 0;
    *reinterpret_cast<uint16_t*>(s + 0x2d8) = 0;

    sub_21cda20(s + 0x268);
    *reinterpret_cast<void**>(s + 0x210) = g_vtable_53b6eb0;

    void** mat_cur = *reinterpret_cast<void***>(s + 0x220);
    if (mat_cur != *reinterpret_cast<void***>(s + 0x228)) {
        for (; mat_cur != *reinterpret_cast<void***>(s + 0x228); ++mat_cur) {
            void* elem = *mat_cur;
            auto* vtbl = *reinterpret_cast<uintptr_t**>(elem);
            reinterpret_cast<void (*)(void*)>(vtbl[0x48 / 8])(elem);
        }
        mat_cur = *reinterpret_cast<void***>(s + 0x220);
    }
    *reinterpret_cast<void***>(s + 0x228) = mat_cur;
    if (mat_cur != nullptr) {
        void* mat_alloc = *reinterpret_cast<void**>(s + 0x238);
        auto* ma_vtbl = *reinterpret_cast<uintptr_t**>(mat_alloc);
        using FreeFn = void (*)(void*, void*);
        reinterpret_cast<FreeFn>(ma_vtbl[0x70 / 8])(mat_alloc, mat_cur);
        *reinterpret_cast<uint64_t*>(s + 0x230) = 0;
        *reinterpret_cast<uint64_t*>(s + 0x228) = 0;
        *reinterpret_cast<uint64_t*>(s + 0x220) = 0;
    }

    sub_e515f0(s + 0x1d8);
    sub_e4f900(s + 0x188);
    sub_e52e30(s + 0x160);
    sub_e4f3a0(s + 0xf8);

    auto* ref_res = *reinterpret_cast<uint8_t**>(s + 0xf0);
    if (ref_res != nullptr) {
        auto* atomic_rc = reinterpret_cast<std::atomic<int32_t>*>(ref_res + 0x10);
        if (atomic_rc->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            void* res_alloc = *reinterpret_cast<void**>(ref_res + 0x08);
            auto* r_vtbl = *reinterpret_cast<uintptr_t**>(ref_res);
            reinterpret_cast<void (*)(void*)>(r_vtbl[0])(ref_res);
            auto* ra_vtbl = *reinterpret_cast<uintptr_t**>(res_alloc);
            using FreeFn = void (*)(void*, void*);
            reinterpret_cast<FreeFn>(ra_vtbl[0x70 / 8])(res_alloc, ref_res);
        }
        *reinterpret_cast<void**>(s + 0xf0) = nullptr;
    }

    sub_e50c20(s + 0x98);
    sub_e4e100(s + 0x78);
    sub_e515f0(s + 0x58);
    sub_e515f0(s + 0x38);

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    sub_e52280(s + 0x10);
}

// ============================================================================
// 34. 0x21c63f0 (28 B) - sub_21c63f0
// Deleting destructor for GXFlverModelResource.
// ============================================================================
void sub_21c63f0(void* self) {
    sub_21c6410(self);
    sub_2bbe588(self);
}

// ============================================================================
// 35. 0x21c60f0 (759 B) - sub_21c60f0
// Constructor for GXFlverModelResource (0x308 bytes, vtable 0x53b6e70).
// ============================================================================
void sub_21c60f0(void* self, void* alloc) {
    auto* s = static_cast<uint8_t*>(self);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    *reinterpret_cast<uint32_t*>(s + 0x08) = 0;
    *reinterpret_cast<void**>(s + 0x00)    = g_vtable_53b6e70;

    sub_e51e90(s + 0x10, alloc);
    sub_e51210(s + 0x38, alloc);
    sub_e51210(s + 0x58, alloc);
    sub_e4ddc0(s + 0x78, alloc);
    sub_e502c0(s + 0x98, alloc);
    *reinterpret_cast<uint64_t*>(s + 0xf0) = 0;
    sub_e4edb0(s + 0xf8, alloc);
    sub_e528e0(s + 0x160, alloc);
    sub_e4f810(s + 0x188, alloc);
    sub_e51210(s + 0x1d8, alloc);

    s[0x1f8] = 0;
    *reinterpret_cast<uint32_t*>(s + 0x1fc) = 0xFFFFFFFFu;
    s[0x200] = 0;
    s[0x201] = 0;
    *reinterpret_cast<uint32_t*>(s + 0x204) = 0;
    *reinterpret_cast<uint32_t*>(s + 0x208) = 0;
    *reinterpret_cast<void**>(s + 0x210)    = g_vtable_53b6eb0;

    auto verify_default_heap = [&]() -> void* {
        void* heap = g_default_heap_cache_5540670;
        if (heap == nullptr) {
            heap = sub_2ba2b10();
            g_default_heap_cache_5540670 = heap;
        }
        uint8_t flags_buf[8] = {};
        auto* h_vtbl = *reinterpret_cast<uintptr_t**>(heap);
        using CheckFn = void (*)(void*, void*, uint32_t);
        reinterpret_cast<CheckFn>(h_vtbl[0x20 / 8])(flags_buf, heap, 0);
        if ((flags_buf[0] & 0x20) == 0) {
            sub_20b55b0(nullptr, 0x3e, kErrIncompatHeap_4737dc0);
        }
        return heap;
    };

    void* heap1 = verify_default_heap();
    *reinterpret_cast<uint64_t*>(s + 0x230) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x228) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x220) = 0;
    *reinterpret_cast<void**>(s + 0x238)    = heap1;
    *reinterpret_cast<void**>(s + 0x240)    = alloc;
    *reinterpret_cast<uint32_t*>(s + 0x260) = 0;
    *reinterpret_cast<uint16_t*>(s + 0x25c) = 0;
    *reinterpret_cast<uint32_t*>(s + 0x258) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x250) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x248) = 0;
    *reinterpret_cast<uint32_t*>(s + 0x270) = 0;
    *reinterpret_cast<void**>(s + 0x268)    = g_vtable_53b6ee0;

    void* heap2 = verify_default_heap();
    *reinterpret_cast<uint64_t*>(s + 0x290) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x288) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x280) = 0;
    *reinterpret_cast<void**>(s + 0x298)    = heap2;
    *reinterpret_cast<void**>(s + 0x2a0)    = alloc;
    *reinterpret_cast<void**>(s + 0x268)    = g_vtable_53b6f20;
    *reinterpret_cast<void**>(s + 0x2a8)    = alloc;
    *reinterpret_cast<uint64_t*>(s + 0x2b8) = 0;
    *reinterpret_cast<uint64_t*>(s + 0x2b0) = 0;
    *reinterpret_cast<void**>(s + 0x2c0)    = alloc;
    *reinterpret_cast<uint32_t*>(s + 0x2c8) = 0;
    s[0x2cc] = 0;

    void* heap3 = verify_default_heap();
    *reinterpret_cast<void**>(s + 0x2f8)    = heap3;
    *reinterpret_cast<uint64_t*>(s + 0x2f0) = 7;
    *reinterpret_cast<uint64_t*>(s + 0x2e8) = 0;
    *reinterpret_cast<uint16_t*>(s + 0x2d8) = 0;
    s[0x300] = 1;

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 36. 0x21c5e80 (618 B) - sub_21c5e80
// Reallocates GXFlverMaterialParamOverride vector (0x90-byte elements) to `new_cap`.
// ============================================================================
void sub_21c5e80(void* vec, size_t new_cap) {
    auto* v = static_cast<uint8_t*>(vec);
    void* alloc = *reinterpret_cast<void**>(v + 0x20);
    auto* a_vtbl = *reinterpret_cast<uintptr_t**>(alloc);
    using AllocFn = void* (*)(void*, size_t, size_t);
    auto* new_buf = static_cast<uint8_t*>(
        reinterpret_cast<AllocFn>(a_vtbl[0x58 / 8])(alloc, new_cap * 0x90, 16));

    if (new_buf == nullptr) {
        if (g_tls_tx_active_53e4318 != 0) {
            sub_2bbe978(&g_tls_tx_ctx_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, kErrTxFailed_4737d80);
    }

    auto* old_begin = *reinterpret_cast<uint8_t**>(v + 0x08);
    auto* old_end   = *reinterpret_cast<uint8_t**>(v + 0x10);
    uint8_t* free_begin = old_begin;
    uint8_t* free_end   = old_begin;

    if (old_begin != old_end) {
        size_t byte_len = static_cast<size_t>(old_end - old_begin);
        for (size_t off = 0; off != byte_len; off += 0x90) {
            uint8_t* dst = new_buf + off;
            uint8_t* src = old_begin + off;
            if (dst != nullptr) {
                *reinterpret_cast<uint64_t*>(dst + 0x10) = *reinterpret_cast<uint64_t*>(src + 0x10);
                _mm_storeu_ps(reinterpret_cast<float*>(dst), _mm_loadu_ps(reinterpret_cast<const float*>(src)));

                *reinterpret_cast<void**>(dst + 0x40)    = *reinterpret_cast<void**>(src + 0x40);
                *reinterpret_cast<uint64_t*>(dst + 0x38) = 7;
                *reinterpret_cast<uint64_t*>(dst + 0x30) = 0;
                *reinterpret_cast<uint16_t*>(dst + 0x20) = 0;
                sub_26279a0(dst + 0x18, src + 0x18, 0, ~static_cast<size_t>(0));
                dst[0x48] = src[0x48];

                *reinterpret_cast<void**>(dst + 0x78)    = *reinterpret_cast<void**>(src + 0x78);
                *reinterpret_cast<uint64_t*>(dst + 0x70) = 7;
                *reinterpret_cast<uint64_t*>(dst + 0x68) = 0;
                *reinterpret_cast<uint16_t*>(dst + 0x58) = 0;
                sub_26279a0(dst + 0x50, src + 0x50, 0, ~static_cast<size_t>(0));
                dst[0x80] = src[0x80];
                dst[0x8c] = src[0x8c];
                *reinterpret_cast<uint32_t*>(dst + 0x88) = *reinterpret_cast<uint32_t*>(src + 0x88);
            }
        }
        free_begin = *reinterpret_cast<uint8_t**>(v + 0x08);
        free_end   = *reinterpret_cast<uint8_t**>(v + 0x10);
    }

    using FreeFn = void (*)(void*, void*);
    if (free_begin != nullptr) {
        if (free_begin != free_end) {
            for (uint8_t* p = free_begin; p != free_end; p += 0x90) {
                if (*reinterpret_cast<uint64_t*>(p + 0x70) >= 8) {
                    void* s_buf   = *reinterpret_cast<void**>(p + 0x58);
                    void* s_alloc = *reinterpret_cast<void**>(p + 0x78);
                    auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
                    reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
                }
                *reinterpret_cast<uint64_t*>(p + 0x70) = 7;
                *reinterpret_cast<uint64_t*>(p + 0x68) = 0;
                *reinterpret_cast<uint16_t*>(p + 0x58) = 0;

                if (*reinterpret_cast<uint64_t*>(p + 0x38) >= 8) {
                    void* s_buf   = *reinterpret_cast<void**>(p + 0x20);
                    void* s_alloc = *reinterpret_cast<void**>(p + 0x40);
                    auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
                    reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
                }
                *reinterpret_cast<uint64_t*>(p + 0x38) = 7;
                *reinterpret_cast<uint64_t*>(p + 0x30) = 0;
                *reinterpret_cast<uint16_t*>(p + 0x20) = 0;
            }
            free_begin = *reinterpret_cast<uint8_t**>(v + 0x08);
        }
        void* v_alloc = *reinterpret_cast<void**>(v + 0x20);
        auto* vv = *reinterpret_cast<uintptr_t**>(v_alloc);
        reinterpret_cast<FreeFn>(vv[0x70 / 8])(v_alloc, free_begin);
    }

    int64_t elem_count = ((free_end - free_begin) >> 4) * static_cast<int64_t>(0x8E38E38E38E38E39ull);
    *reinterpret_cast<uint8_t**>(v + 0x18) = new_buf + new_cap * 0x90;
    *reinterpret_cast<uint8_t**>(v + 0x10) = new_buf + elem_count * 0x90;
    *reinterpret_cast<uint8_t**>(v + 0x08) = new_buf;
}

// ============================================================================
// 37. 0x21c5cf0 (392 B) - sub_21c5cf0
// Erases one 0x90-byte element at `erase_pos` from GXFlverMaterialParamOverride vector.
// ============================================================================
void* sub_21c5cf0(void* out_iter, void* vec,
                  uint64_t /*pad_rdx*/, uint64_t /*pad_rcx*/,
                  uint64_t pad_r8, uint64_t /*pad_r9*/,
                  void* arg_s0, void* arg_s8, void* erase_pos) {
    auto* v = static_cast<uint8_t*>(vec);
    auto* begin = *reinterpret_cast<uint8_t**>(v + 0x08);
    auto* end = *reinterpret_cast<uint8_t**>(v + 0x10);
    auto* cur = static_cast<uint8_t*>(erase_pos);
    if (cur < begin || cur >= end) {
        cur = reinterpret_cast<uint8_t*>(pad_r8);
    }
    auto* saved_pos = cur;
    auto* next = cur + 0x90;

    using CopyStatusFn = void (*)(void*);
    if (next != end) {
        while (true) {
            uint8_t* step_next = cur + 0x90;
            *reinterpret_cast<uint64_t*>(cur + 0x10) = *reinterpret_cast<uint64_t*>(cur + 0xa0);
            _mm_storeu_ps(reinterpret_cast<float*>(cur),
                          _mm_loadu_ps(reinterpret_cast<const float*>(cur + 0x90)));
            cur[0x48] = cur[0xd8];

            void* a1 = *reinterpret_cast<void**>(cur + 0x40);
            void* a2 = *reinterpret_cast<void**>(cur + 0xd0);
            reinterpret_cast<CopyStatusFn>((*reinterpret_cast<uintptr_t**>(a1))[0x10 / 8])(a1);
            reinterpret_cast<CopyStatusFn>((*reinterpret_cast<uintptr_t**>(a2))[0x10 / 8])(a2);
            sub_26279a0(cur + 0x18, cur + 0xa8, 0, ~static_cast<size_t>(0));

            cur[0x80] = cur[0x110];
            void* b1 = *reinterpret_cast<void**>(cur + 0x78);
            void* b2 = *reinterpret_cast<void**>(cur + 0x108);
            reinterpret_cast<CopyStatusFn>((*reinterpret_cast<uintptr_t**>(b1))[0x10 / 8])(b1);
            reinterpret_cast<CopyStatusFn>((*reinterpret_cast<uintptr_t**>(b2))[0x10 / 8])(b2);
            sub_26279a0(cur + 0x50, cur + 0xe0, 0, ~static_cast<size_t>(0));

            cur[0x8c] = cur[0x11c];
            *reinterpret_cast<uint32_t*>(cur + 0x88) = *reinterpret_cast<uint32_t*>(cur + 0x118);

            if (cur + 0x120 == end) {
                break;
            }
            cur = step_next;
        }
        next = *reinterpret_cast<uint8_t**>(v + 0x10);
    }

    using FreeFn = void (*)(void*, void*);
    if (*reinterpret_cast<uint64_t*>(next - 0x20) >= 8) {
        void* s_buf   = *reinterpret_cast<void**>(next - 0x38);
        void* s_alloc = *reinterpret_cast<void**>(next - 0x18);
        auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
        reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
    }
    *reinterpret_cast<uint64_t*>(next - 0x20) = 7;
    *reinterpret_cast<uint64_t*>(next - 0x28) = 0;
    *reinterpret_cast<uint16_t*>(next - 0x38) = 0;

    if (*reinterpret_cast<uint64_t*>(next - 0x58) >= 8) {
        void* s_buf   = *reinterpret_cast<void**>(next - 0x70);
        void* s_alloc = *reinterpret_cast<void**>(next - 0x50);
        auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
        reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
    }
    *reinterpret_cast<uint64_t*>(next - 0x58) = 7;
    *reinterpret_cast<uint64_t*>(next - 0x60) = 0;
    *reinterpret_cast<uint16_t*>(next - 0x70) = 0;

    *reinterpret_cast<uint8_t**>(v + 0x10) -= 0x90;
    *reinterpret_cast<void**>(static_cast<uint8_t*>(out_iter) + 0x00) = arg_s0;
    *reinterpret_cast<void**>(static_cast<uint8_t*>(out_iter) + 0x08) = arg_s8;
    *reinterpret_cast<void**>(static_cast<uint8_t*>(out_iter) + 0x10) = saved_pos;
    return out_iter;
}

// ============================================================================
// 38. 0x21c5a70 (627 B) - sub_21c5a70
// Appends a 0x90-byte GXFlverMaterialParamOverride element to `vec`.
// ============================================================================
void sub_21c5a70(void* vec, const void* elem) {
    auto* v = static_cast<uint8_t*>(vec);
    const auto* e = static_cast<const uint8_t*>(elem);

    uint8_t* end   = *reinterpret_cast<uint8_t**>(v + 0x10);
    uint8_t* begin = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* cap   = *reinterpret_cast<uint8_t**>(v + 0x18);

    auto grow_vec = [&]() {
        uint64_t cur_sz = static_cast<uint64_t>((end - begin) >> 4) * 0x8E38E38E38E38E39ull;
        uint64_t half = cur_sz >> 1;
        uint64_t new_cap = (0x01C71C71C71C71C7ull - half < cur_sz) ? 0 : (cur_sz + half);
        if (new_cap < cur_sz + 1) new_cap = cur_sz + 1;
        sub_21c5e80(v, new_cap);
    };

    auto copy_construct_at = [&](uint8_t* dst, const uint8_t* src) {
        *reinterpret_cast<uint64_t*>(dst + 0x10) = *reinterpret_cast<const uint64_t*>(src + 0x10);
        _mm_storeu_ps(reinterpret_cast<float*>(dst), _mm_loadu_ps(reinterpret_cast<const float*>(src)));

        *reinterpret_cast<void**>(dst + 0x40)    = *reinterpret_cast<void* const*>(src + 0x40);
        *reinterpret_cast<uint64_t*>(dst + 0x38) = 7;
        *reinterpret_cast<uint64_t*>(dst + 0x30) = 0;
        *reinterpret_cast<uint16_t*>(dst + 0x20) = 0;
        sub_26279a0(dst + 0x18, src + 0x18, 0, ~static_cast<size_t>(0));
        dst[0x48] = src[0x48];

        *reinterpret_cast<void**>(dst + 0x78)    = *reinterpret_cast<void* const*>(src + 0x78);
        *reinterpret_cast<uint64_t*>(dst + 0x70) = 7;
        *reinterpret_cast<uint64_t*>(dst + 0x68) = 0;
        *reinterpret_cast<uint16_t*>(dst + 0x58) = 0;
        sub_26279a0(dst + 0x50, src + 0x50, 0, ~static_cast<size_t>(0));
        dst[0x80] = src[0x80];
        dst[0x8c] = src[0x8c];
        *reinterpret_cast<uint32_t*>(dst + 0x88) = *reinterpret_cast<const uint32_t*>(src + 0x88);
    };

    if (e < end && begin <= e) {
        size_t idx = static_cast<size_t>((e - begin) >> 4) * 0x8E38E38E38E38E39ull;
        if (end == cap) {
            grow_vec();
            begin = *reinterpret_cast<uint8_t**>(v + 0x08);
            end   = *reinterpret_cast<uint8_t**>(v + 0x10);
        }
        if (end != nullptr) {
            copy_construct_at(end, begin + idx * 0x90);
        }
    } else {
        if (end == cap) {
            grow_vec();
            end = *reinterpret_cast<uint8_t**>(v + 0x10);
        }
        if (end != nullptr) {
            copy_construct_at(end, e);
        }
    }
    *reinterpret_cast<uint8_t**>(v + 0x10) += 0x90;
}

// ============================================================================
// 39. 0x21c5960 (260 B) - sub_21c5960
// Default-constructs a 0x90-byte GXFlverMaterialParamOverride entry.
// ============================================================================
void sub_21c5960(void* elem) {
    auto* e = static_cast<uint8_t*>(elem);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    auto init_wstr_heap = [&]() -> void* {
        void* heap = g_default_heap_cache_5540670;
        if (heap == nullptr) {
            heap = sub_2ba2b10();
            g_default_heap_cache_5540670 = heap;
        }
        uint8_t flags_buf[8] = {};
        auto* h_vtbl = *reinterpret_cast<uintptr_t**>(heap);
        using CheckFn = void (*)(void*, void*, uint32_t);
        reinterpret_cast<CheckFn>(h_vtbl[0x20 / 8])(flags_buf, heap, 0);
        if ((flags_buf[0] & 0x20) == 0) {
            sub_20b55b0(nullptr, 0x3e, kErrIncompatHeap_4737dc0);
        }
        return heap;
    };

    *reinterpret_cast<void**>(e + 0x40)    = init_wstr_heap();
    *reinterpret_cast<uint64_t*>(e + 0x38) = 7;
    *reinterpret_cast<uint64_t*>(e + 0x30) = 0;
    *reinterpret_cast<uint16_t*>(e + 0x20) = 0;
    e[0x48] = 1;

    *reinterpret_cast<void**>(e + 0x78)    = init_wstr_heap();
    *reinterpret_cast<uint64_t*>(e + 0x70) = 7;
    *reinterpret_cast<uint64_t*>(e + 0x68) = 0;
    *reinterpret_cast<uint16_t*>(e + 0x58) = 0;
    e[0x80] = 1;

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 40. 0x21c58a0 (36 B) - sub_21c58a0
// Dispatches parameter override lookup by param_id (!= 0) or CRC32 (== 0).
// ============================================================================
void* sub_21c58a0(const void* lookup_ctx, uint32_t param_id) {
    const auto* ctx = static_cast<const uint8_t*>(lookup_ctx);
    void* vec = *reinterpret_cast<void* const*>(ctx + 0x00);
    if (vec == nullptr) {
        return nullptr;
    }
    const auto* mat_name = *reinterpret_cast<const char16_t* const*>(ctx + 0x08);
    if (param_id != 0) {
        return sub_21c5550(vec, mat_name, param_id);
    }
    return sub_21c56a0(vec, mat_name, 0);
}

// ============================================================================
// 41. 0x21c5880 (29 B) - sub_21c5880
// Returns the number of 0x90-byte override entries in `[lookup_ctx]`.
// ============================================================================
uint32_t sub_21c5880(const void* lookup_ctx) {
    const auto* vec = *reinterpret_cast<const uint8_t* const*>(lookup_ctx);
    if (vec == nullptr) {
        return 0;
    }
    uint64_t diff = static_cast<uint64_t>(
        *reinterpret_cast<const uint8_t* const*>(vec + 0x10) -
        *reinterpret_cast<const uint8_t* const*>(vec + 0x08));
    return static_cast<uint32_t>(diff >> 4) * 0x38E38E39u;
}

// ============================================================================
// 42. 0x21c5860 (26 B) - sub_21c5860
// Initializes a 16-byte material parameter override lookup context.
// ============================================================================
void sub_21c5860(void* out_ctx, const void* holder, const char16_t* mat_name) {
    auto* c = static_cast<uint8_t*>(out_ctx);
    *reinterpret_cast<void**>(c + 0x00) = nullptr;
    *reinterpret_cast<const char16_t**>(c + 0x08) = mat_name;
    void* vec = (holder != nullptr)
        ? *reinterpret_cast<void* const*>(static_cast<const uint8_t*>(holder) + 0x08)
        : nullptr;
    *reinterpret_cast<void**>(c + 0x00) = vec;
}

// ============================================================================
// 43. 0x21c57f0 (109 B) - sub_21c57f0
// Hashes wide string `param_name` with IEEE CRC-32 and looks up override entry.
// ============================================================================
void* sub_21c57f0(void* vec, const char16_t* mat_name, const char16_t* param_name) {
    size_t wlen = sub_2bbf228(param_name);
    uint32_t crc = 0;
    if ((wlen & 0x7FFFFFFFFFFFFFFFull) != 0) {
        size_t byte_len = wlen * 2;
        crc = 0xFFFFFFFFu;
        const auto* bytes = reinterpret_cast<const uint8_t*>(param_name);
        for (size_t i = 0; i < byte_len; ++i) {
            uint32_t idx = (crc ^ bytes[i]) & 0xFFu;
            crc = (crc >> 8) ^ kCrc32Table_52b74f0[idx];
        }
        crc = ~crc;
    }
    return sub_21c56a0(vec, mat_name, crc);
}

// ============================================================================
// 44. 0x21c56a0 (327 B) - sub_21c56a0
// Finds material parameter override entry by material name and CRC-32 (+0x88).
// ============================================================================
void* sub_21c56a0(void* vec, const char16_t* mat_name, uint32_t crc) {
    auto* v = static_cast<uint8_t*>(vec);
    uint8_t* begin = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end   = *reinterpret_cast<uint8_t**>(v + 0x10);

    if (mat_name != nullptr) {
        for (uint8_t* cur = begin; cur != end; cur = *reinterpret_cast<uint8_t**>(v + 0x10), cur += 0) {
            // Note: reload end after string compare in case of side effects
            for (uint8_t* it = begin; it != *reinterpret_cast<uint8_t**>(v + 0x10); it += 0x90) {
                if (it[0x8c] == 0 && *reinterpret_cast<uint32_t*>(it + 0x88) == crc) {
                    size_t entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                    size_t query_len = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
                    entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                    const char16_t* entry_str = (*reinterpret_cast<uint64_t*>(it + 0x38) >= 8)
                        ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
                        : reinterpret_cast<const char16_t*>(it + 0x20);
                    size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
                    int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, mat_name, cmp_len) : 0;
                    if (cmp == 0 && entry_len == query_len) {
                        return it;
                    }
                }
            }
            break;
        }
        end = *reinterpret_cast<uint8_t**>(v + 0x10);
    }

    for (uint8_t* it = *reinterpret_cast<uint8_t**>(v + 0x08); it != end; it += 0x90) {
        if (it[0x8c] != 0 && *reinterpret_cast<uint32_t*>(it + 0x88) == crc) {
            return it;
        }
    }
    return nullptr;
}

// ============================================================================
// 45. 0x21c5550 (324 B) - sub_21c5550
// Finds material parameter override entry by material name and param_id (+0x10).
// ============================================================================
void* sub_21c5550(void* vec, const char16_t* mat_name, uint32_t param_id) {
    auto* v = static_cast<uint8_t*>(vec);
    uint8_t* begin = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end   = *reinterpret_cast<uint8_t**>(v + 0x10);

    if (mat_name != nullptr) {
        for (uint8_t* it = begin; it != *reinterpret_cast<uint8_t**>(v + 0x10); it += 0x90) {
            if (it[0x8c] == 0 && *reinterpret_cast<uint32_t*>(it + 0x10) == param_id) {
                size_t query_len = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
                size_t entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                const char16_t* entry_str = (*reinterpret_cast<uint64_t*>(it + 0x38) >= 8)
                    ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
                    : reinterpret_cast<const char16_t*>(it + 0x20);
                size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
                int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, mat_name, cmp_len) : 0;
                if (cmp == 0 && entry_len == query_len) {
                    return it;
                }
            }
        }
        end = *reinterpret_cast<uint8_t**>(v + 0x10);
    }

    for (uint8_t* it = *reinterpret_cast<uint8_t**>(v + 0x08); it != end; it += 0x90) {
        if (it[0x8c] != 0 && *reinterpret_cast<uint32_t*>(it + 0x10) == param_id) {
            return it;
        }
    }
    return nullptr;
}

// ============================================================================
// 46. 0x21c54b0 (149 B) - sub_21c54b0
// Clears all 0x90-byte elements in GXFlverMaterialParamOverride vector.
// ============================================================================
void sub_21c54b0(void* vec) {
    auto* v = static_cast<uint8_t*>(vec);
    uint8_t* cur = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end = *reinterpret_cast<uint8_t**>(v + 0x10);

    using FreeFn = void (*)(void*, void*);
    if (cur != end) {
        for (; cur != end; cur += 0x90) {
            if (*reinterpret_cast<uint64_t*>(cur + 0x70) >= 8) {
                void* s_buf   = *reinterpret_cast<void**>(cur + 0x58);
                void* s_alloc = *reinterpret_cast<void**>(cur + 0x78);
                auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
                reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
            }
            *reinterpret_cast<uint64_t*>(cur + 0x70) = 7;
            *reinterpret_cast<uint64_t*>(cur + 0x68) = 0;
            *reinterpret_cast<uint16_t*>(cur + 0x58) = 0;

            if (*reinterpret_cast<uint64_t*>(cur + 0x38) >= 8) {
                void* s_buf   = *reinterpret_cast<void**>(cur + 0x20);
                void* s_alloc = *reinterpret_cast<void**>(cur + 0x40);
                auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
                reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
            }
            *reinterpret_cast<uint64_t*>(cur + 0x38) = 7;
            *reinterpret_cast<uint64_t*>(cur + 0x30) = 0;
            *reinterpret_cast<uint16_t*>(cur + 0x20) = 0;
        }
        cur = *reinterpret_cast<uint8_t**>(v + 0x08);
    }
    *reinterpret_cast<uint8_t**>(v + 0x10) = cur;
}

// ============================================================================
// 47. 0x21c5300 (420 B) - sub_21c5300
// Removes a material parameter override matching `mat_name` and `param_name` (CRC32).
// ============================================================================
uint8_t sub_21c5300(void* vec, const char16_t* mat_name, const char16_t* param_name) {
    auto* v = static_cast<uint8_t*>(vec);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    size_t wlen = sub_2bbf228(param_name);
    uint32_t crc = 0;
    if ((wlen & 0x7FFFFFFFFFFFFFFFull) != 0) {
        size_t byte_len = wlen * 2;
        crc = 0xFFFFFFFFu;
        const auto* bytes = reinterpret_cast<const uint8_t*>(param_name);
        for (size_t i = 0; i < byte_len; ++i) {
            uint32_t idx = (crc ^ bytes[i]) & 0xFFu;
            crc = (crc >> 8) ^ kCrc32Table_52b74f0[idx];
        }
        crc = ~crc;
    }

    uint8_t result = 0;
    uint8_t* it  = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end = *reinterpret_cast<uint8_t**>(v + 0x10);
    while (it != end) {
        if (*reinterpret_cast<uint32_t*>(it + 0x88) == crc) {
            if (mat_name == nullptr) {
                if (it[0x8c] != 0) {
                    uint8_t out_iter[24] = {};
                    sub_21c5cf0(out_iter, v, 0, 0, 0, 0, nullptr, nullptr, it);
                    result = 1;
                    break;
                }
            } else {
                size_t query_len = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
                size_t entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                const char16_t* entry_str = (*reinterpret_cast<uint64_t*>(it + 0x38) >= 8)
                    ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
                    : reinterpret_cast<const char16_t*>(it + 0x20);
                size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
                int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, mat_name, cmp_len) : 0;
                if (cmp == 0 && entry_len == query_len) {
                    uint8_t out_iter[24] = {};
                    sub_21c5cf0(out_iter, v, 0, 0, 0, 0, nullptr, nullptr, it);
                    result = 1;
                    break;
                }
                end = *reinterpret_cast<uint8_t**>(v + 0x10);
            }
        }
        it += 0x90;
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return result;
}

// ============================================================================
// 48. 0x21c5190 (358 B) - sub_21c5190
// Removes a material parameter override matching `mat_name` and `param_id`.
// ============================================================================
uint8_t sub_21c5190(void* vec, const char16_t* mat_name, uint32_t param_id) {
    auto* v = static_cast<uint8_t*>(vec);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    uint8_t result = 0;
    uint8_t* it  = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end = *reinterpret_cast<uint8_t**>(v + 0x10);
    while (it != end) {
        if (*reinterpret_cast<uint32_t*>(it + 0x10) == param_id) {
            if (mat_name == nullptr) {
                if (it[0x8c] != 0) {
                    uint8_t out_iter[24] = {};
                    sub_21c5cf0(out_iter, v, 0, 0, 0, 0, nullptr, nullptr, it);
                    result = 1;
                    break;
                }
            } else {
                size_t query_len = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
                size_t entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                const char16_t* entry_str = (*reinterpret_cast<uint64_t*>(it + 0x38) >= 8)
                    ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
                    : reinterpret_cast<const char16_t*>(it + 0x20);
                size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
                int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, mat_name, cmp_len) : 0;
                if (cmp == 0 && entry_len == query_len) {
                    uint8_t out_iter[24] = {};
                    sub_21c5cf0(out_iter, v, 0, 0, 0, 0, nullptr, nullptr, it);
                    result = 1;
                    break;
                }
                end = *reinterpret_cast<uint8_t**>(v + 0x10);
            }
        }
        it += 0x90;
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return result;
}

// ============================================================================
// 49. 0x21c5180 (5 B) - sub_21c5180
// Thunk jumping directly to sub_21c4f10.
// ============================================================================
void sub_21c5180(void* vec, const char16_t* mat_name,
                 const char16_t* param_name, const float* val4, float scalar_val) {
    sub_21c4f10(vec, mat_name, param_name, val4, scalar_val);
}

// ============================================================================
// 50. 0x21c4f10 (615 B) - sub_21c4f10
// Updates or inserts a material parameter override by `mat_name` & `param_name` (CRC32).
// ============================================================================
void sub_21c4f10(void* vec, const char16_t* mat_name,
                 const char16_t* param_name, const float* val4, float scalar_val) {
    auto* v = static_cast<uint8_t*>(vec);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    size_t wlen = sub_2bbf228(param_name);
    uint32_t crc = 0;
    if ((wlen & 0x7FFFFFFFFFFFFFFFull) != 0) {
        size_t byte_len = wlen * 2;
        crc = 0xFFFFFFFFu;
        const auto* bytes = reinterpret_cast<const uint8_t*>(param_name);
        for (size_t i = 0; i < byte_len; ++i) {
            uint32_t idx = (crc ^ bytes[i]) & 0xFFu;
            crc = (crc >> 8) ^ kCrc32Table_52b74f0[idx];
        }
        crc = ~crc;
    }

    alignas(16) uint8_t temp_elem[0x90] = {};
    sub_21c5960(temp_elem);

    uint8_t* found = nullptr;
    uint8_t* it  = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end = *reinterpret_cast<uint8_t**>(v + 0x10);
    while (it != end) {
        if (*reinterpret_cast<uint32_t*>(it + 0x88) == crc) {
            if (mat_name == nullptr) {
                if (it[0x8c] != 0) {
                    found = it;
                    break;
                }
            } else {
                size_t query_len = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
                size_t entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                const char16_t* entry_str = (*reinterpret_cast<uint64_t*>(it + 0x38) >= 8)
                    ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
                    : reinterpret_cast<const char16_t*>(it + 0x20);
                size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
                int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, mat_name, cmp_len) : 0;
                if (cmp == 0 && entry_len == query_len) {
                    found = it;
                    break;
                }
                end = *reinterpret_cast<uint8_t**>(v + 0x10);
            }
        }
        it += 0x90;
    }

    if (found != nullptr) {
        _mm_storeu_ps(reinterpret_cast<float*>(found), _mm_loadu_ps(val4));
        *reinterpret_cast<float*>(found + 0x14) = scalar_val;
    } else {
        _mm_store_ps(reinterpret_cast<float*>(temp_elem), _mm_loadu_ps(val4));
        if (mat_name != nullptr) {
            size_t mlen = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
            sub_262a720(temp_elem + 0x18, mat_name, mlen);
        }
        temp_elem[0x8c] = (mat_name == nullptr) ? 1 : 0;
        *reinterpret_cast<float*>(temp_elem + 0x14)    = scalar_val;
        *reinterpret_cast<uint32_t*>(temp_elem + 0x88) = crc;
        *reinterpret_cast<uint32_t*>(temp_elem + 0x10) = 0;
        sub_21c5a70(v, temp_elem);
    }

    using FreeFn = void (*)(void*, void*);
    if (*reinterpret_cast<uint64_t*>(temp_elem + 0x70) >= 8) {
        void* s_buf   = *reinterpret_cast<void**>(temp_elem + 0x58);
        void* s_alloc = *reinterpret_cast<void**>(temp_elem + 0x78);
        auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
        reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
    }
    *reinterpret_cast<uint64_t*>(temp_elem + 0x70) = 7;
    *reinterpret_cast<uint64_t*>(temp_elem + 0x68) = 0;
    *reinterpret_cast<uint16_t*>(temp_elem + 0x58) = 0;

    if (*reinterpret_cast<uint64_t*>(temp_elem + 0x38) >= 8) {
        void* s_buf   = *reinterpret_cast<void**>(temp_elem + 0x20);
        void* s_alloc = *reinterpret_cast<void**>(temp_elem + 0x40);
        auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
        reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 51. 0x21c4cd0 (562 B) - sub_21c4cd0
// Updates or inserts a material parameter override by `mat_name` & `param_id`.
// ============================================================================
void sub_21c4cd0(void* vec, const char16_t* mat_name,
                 uint32_t param_id, const float* val4, float scalar_val) {
    auto* v = static_cast<uint8_t*>(vec);
    uint64_t cookie = *g_stack_cookie_ptr_53e3be0;

    alignas(16) uint8_t temp_elem[0x90] = {};
    sub_21c5960(temp_elem);

    uint8_t* found = nullptr;
    uint8_t* it  = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* end = *reinterpret_cast<uint8_t**>(v + 0x10);
    if (it != end) {
        if (mat_name != nullptr) {
            for (; it != end; it += 0x90) {
                if (*reinterpret_cast<uint32_t*>(it + 0x10) == param_id) {
                    size_t query_len = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
                    size_t entry_len = *reinterpret_cast<size_t*>(it + 0x30);
                    const char16_t* entry_str = (*reinterpret_cast<uint64_t*>(it + 0x38) >= 8)
                        ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
                        : reinterpret_cast<const char16_t*>(it + 0x20);
                    size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
                    int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, mat_name, cmp_len) : 0;
                    if (cmp == 0 && entry_len == query_len) {
                        found = it;
                        break;
                    }
                    end = *reinterpret_cast<uint8_t**>(v + 0x10);
                }
            }
        } else {
            for (; it != end; it += 0x90) {
                if (*reinterpret_cast<uint32_t*>(it + 0x10) == param_id && it[0x8c] != 0) {
                    found = it;
                    break;
                }
            }
        }
    }

    if (found != nullptr) {
        _mm_storeu_ps(reinterpret_cast<float*>(found), _mm_loadu_ps(val4));
        *reinterpret_cast<float*>(found + 0x14) = scalar_val;
    } else {
        *reinterpret_cast<uint32_t*>(temp_elem + 0x10) = param_id;
        _mm_store_ps(reinterpret_cast<float*>(temp_elem), _mm_loadu_ps(val4));
        if (mat_name != nullptr) {
            size_t mlen = (mat_name[0] != 0) ? sub_2bbf228(mat_name) : 0;
            sub_262a720(temp_elem + 0x18, mat_name, mlen);
        }
        temp_elem[0x8c] = (mat_name == nullptr) ? 1 : 0;
        *reinterpret_cast<float*>(temp_elem + 0x14) = scalar_val;
        sub_21c5a70(v, temp_elem);
    }

    using FreeFn = void (*)(void*, void*);
    if (*reinterpret_cast<uint64_t*>(temp_elem + 0x70) >= 8) {
        void* s_buf   = *reinterpret_cast<void**>(temp_elem + 0x58);
        void* s_alloc = *reinterpret_cast<void**>(temp_elem + 0x78);
        auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
        reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
    }
    *reinterpret_cast<uint64_t*>(temp_elem + 0x70) = 7;
    *reinterpret_cast<uint64_t*>(temp_elem + 0x68) = 0;
    *reinterpret_cast<uint16_t*>(temp_elem + 0x58) = 0;

    if (*reinterpret_cast<uint64_t*>(temp_elem + 0x38) >= 8) {
        void* s_buf   = *reinterpret_cast<void**>(temp_elem + 0x20);
        void* s_alloc = *reinterpret_cast<void**>(temp_elem + 0x40);
        auto* sv = *reinterpret_cast<uintptr_t**>(s_alloc);
        reinterpret_cast<FreeFn>(sv[0x70 / 8])(s_alloc, s_buf);
    }

    if (*g_stack_cookie_ptr_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 52. 0x21c4c10 (192 B) - sub_21c4c10
// Returns 1 if any material parameter override entry matches `mat_name`.
// ============================================================================
uint8_t sub_21c4c10(const void* vec, const char16_t* mat_name) {
    const auto* v = static_cast<const uint8_t*>(vec);
    const char16_t* target = (mat_name != nullptr) ? mat_name : kEmptyWStr_49d85f0;

    const uint8_t* it = *reinterpret_cast<const uint8_t* const*>(v + 0x08);
    while (it != *reinterpret_cast<const uint8_t* const*>(v + 0x10)) {
        size_t query_len = (target[0] != 0) ? sub_2bbf228(target) : 0;
        size_t entry_len = *reinterpret_cast<const size_t*>(it + 0x30);
        const char16_t* entry_str = (*reinterpret_cast<const uint64_t*>(it + 0x38) >= 8)
            ? *reinterpret_cast<const char16_t* const*>(it + 0x20)
            : reinterpret_cast<const char16_t*>(it + 0x20);
        size_t cmp_len = (entry_len < query_len) ? entry_len : query_len;
        int cmp = (cmp_len != 0) ? sub_2bbf268(entry_str, target, cmp_len) : 0;
        if (cmp == 0 && entry_len == query_len) {
            return 1;
        }
        it += 0x90;
    }
    return 0;
}

// ============================================================================
// 53. 0x21c4b70 (9 B) - sub_21c4b70
// Clears all material parameter overrides on holder `[self + 0x08]`.
// ============================================================================
void sub_21c4b70(void* holder) {
    void* vec = *reinterpret_cast<void**>(static_cast<uint8_t*>(holder) + 0x08);
    sub_21c54b0(vec);
}

// ============================================================================
// 54. 0x21c4b60 (9 B) - sub_21c4b60
// Sets/inserts a float4+scalar material parameter override on holder `[self + 0x08]`.
// ============================================================================
void sub_21c4b60(void* holder, const char16_t* mat_name,
                 const char16_t* param_name, const float* val4, float scalar_val) {
    void* vec = *reinterpret_cast<void**>(static_cast<uint8_t*>(holder) + 0x08);
    sub_21c4f10(vec, mat_name, param_name, val4, scalar_val);
}

// ============================================================================
// 55. 0x21c4b50 (9 B) - sub_21c4b50
// Sets/inserts a float4+scalar material parameter override on holder `[self + 0x08]`.
// ============================================================================
void sub_21c4b50(void* holder, const char16_t* mat_name,
                 const char16_t* param_name, const float* val4, float scalar_val) {
    void* vec = *reinterpret_cast<void**>(static_cast<uint8_t*>(holder) + 0x08);
    sub_21c4f10(vec, mat_name, param_name, val4, scalar_val);
}

} // extern "C"
} // namespace Bloodborne::SceneRenderer
