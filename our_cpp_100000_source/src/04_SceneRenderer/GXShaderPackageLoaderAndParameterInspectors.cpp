#include <cstdint>
#include <cstddef>
#include <cstring>
#include <immintrin.h>

namespace Bloodborne::SceneRenderer {

// ============================================================================
// External Engine / Kernel / Container Symbols (Resolved at Link / Loader Time)
// ============================================================================
extern "C" {

// Memory & Kernel Allocator Helpers
void* sub_207b720(void* ptr);                                                    // 0x207b720: DLKR::FindAllocatorForPointer
void* sub_207d250();                                                             // 0x207d250: DLKR::GetOrInitThreadLinearHeap
void  sub_20b55b0(const void* file, uint32_t line, const void* msg, ...);        // 0x20b55b0: DLUT::ReportAssertionFailure
void  sub_2bbe4f8();                                                             // 0x2bbe4f8: __stack_chk_fail
void  sub_2bbe588(void* ptr);                                                    // 0x2bbe588: operator delete(void*)
void* sub_2bbe658(void* dst, const void* src, size_t size);                      // 0x2bbe658: memmove
void  sub_2bbe978(void* excObj, uint32_t code);                                  // 0x2bbe978: KernelThrowOutOfMemory
const uint16_t* sub_2bbec48();                                                   // 0x2bbec48: GetToLowerCaseTable
size_t sub_2bbf228(const char16_t* wstr);                                        // 0x2bbf228: wcslen (16-bit)
const char16_t* sub_2bbf258(const char16_t* wstr, char16_t ch);                  // 0x2bbf258: wcsrchr (16-bit)
void* sub_2ba2b10();                                                             // 0x2ba2b10: GetDefaultStringHeapAllocator

// String & Path Helpers
void  sub_20f1920(void* bndIter, const void* bndHeader, uint32_t index);         // 0x20f1920: BND4::GetEntryAt
void  sub_20f1c20(const void* bndIter, void* outWString);                        // 0x20f1c20: BND4::GetEntryNameW
void  sub_20f7c10(void* obj);                                                    // 0x20f7c10: BND4::CleanupInternalReaderState
void  sub_21bc7b0(const char16_t* path, void* outBaseNameWString);               // 0x21bc7b0: ExtractFileBaseNameW
bool  sub_21c09e0(void* bndReader, const void* bndData, uint64_t bndSize);        // 0x21c09e0: BND4Reader::Mount
void  sub_21c0ea0(void* bndReader, void* outBufferDesc, uint32_t align);         // 0x21c0ea0: BND4Reader::ReadCurrentEntryBuffer
void  sub_26279a0(void* dstWString, const void* srcWString, size_t pos, size_t n); // 0x26279a0: DLWString::AssignSubstr
void  sub_26295d0(void* dstWString, const char16_t* fmt, ...);                   // 0x26295d0: DLWString::Format
void  sub_262a720(void* dstWString, const char16_t* src, size_t len);            // 0x262a720: DLWString::Assign

// TPF / DDS Container Parsers (03_TPF_GNF_ShaderContainers)
bool     sub_26d7510(const void* ddsData);                                       // 0x26d7510: DDS::ValidateMagicAndHeader
uint32_t sub_26d7530(const void* ddsData);                                       // 0x26d7530: DDS::GetGXPixelFormat
uint32_t sub_26d7a20(const void* ddsData);                                       // 0x26d7a20: DDS::GetArrayOrFaceCount
uint32_t sub_26d7a50(const void* ddsData, uint32_t mipIndex);                    // 0x26d7a50: DDS::GetMipPitchOrLinearSize
uint32_t sub_26d7ab0(const void* ddsData, uint32_t mipIndex);                    // 0x26d7ab0: DDS::GetMipSliceSizeBytes
uint32_t sub_26d7b40(const void* ddsData, uint32_t mipIndex, uint32_t sliceIdx); // 0x26d7b40: DDS::GetMipSliceByteOffset
uint32_t sub_26d81f0(const void* tpfData);                                       // 0x26d81f0: TPF::GetTextureCount
void     sub_26d8230(void* outState, const void* tpfData);                       // 0x26d8230: TPF::InitStateInfo
uint32_t sub_26d8280(const void* tpfData, uint32_t index);                       // 0x26d8280: TPF::GetTextureFlags
int32_t  sub_26d8660(const void* tpfData, uint32_t index);                       // 0x26d8660: TPF::GetTextureFormatCode
uint32_t sub_26d8840(const void* tpfData, uint32_t index);                       // 0x26d8840: TPF::GetTextureDimensionType
uint32_t sub_26d8a20(const void* tpfData, uint32_t index);                       // 0x26d8a20: TPF::GetTextureDataSize
const char* sub_26d8c10(const void* tpfData, uint32_t index);                    // 0x26d8c10: TPF::GetTextureName
void*    sub_26d8df0(const void* tpfData, uint32_t index, uint32_t subIdx, void* arg4, uint32_t arg5); // 0x26d8df0: TPF::GetTextureDataPtr
int32_t  sub_26d9070(void* outTex, void* outView, void* dev, const void* tpfData, uint32_t index, void* alloc1, void* alloc2, uint32_t flags); // 0x26d9070: TPF::Create2DTexture
int32_t  sub_26d9850(void* outTex, void* outView, void* dev, const void* tpfData, uint32_t index, void* alloc1, void* alloc2, uint32_t flags); // 0x26d9850: TPF::CreateCubeTexture
int32_t  sub_26d9fe0(void* outTex, void* outView, void* dev, const void* tpfData, void* alloc1, void* alloc2, uint32_t flags); // 0x26d9fe0: TPF::CreateVolumeTexture

// Core GX Texture & Shader Creation (02_GX_GraphicsFramework)
int32_t sub_2165f90(void* dev, void* outTex, const void* desc, const void* mipDescs, void* alloc1, void* alloc2, void* ext); // 0x2165f90: GXDevice::CreateCubeTexture
int32_t sub_2166300(void* dev, void* outTex, const void* desc, const void* mipDescs, void* alloc1, void* alloc2, void* ext); // 0x2166300: GXDevice::Create2DOr3DTexture
int32_t sub_21669a0(void* dev, void* outView, void* texObj, const void* viewDesc, uint32_t flags);                           // 0x21669a0: GXDevice::CreateShaderResourceView
void    sub_21a47e0(void* csObj, const void* binData, size_t binSize, uint32_t arg4, uint32_t arg5);                         // 0x21a47e0: GXComputeShader::InitializeFromPSSL
void    sub_21a4cc0(void* dsObj, const void* binData, size_t binSize, uint32_t arg4, uint32_t arg5);                         // 0x21a4cc0: GXDomainShader::InitializeFromPSSL
void    sub_21a51a0(void* hsObj, const void* binData, size_t binSize, uint32_t arg4, uint32_t arg5);                         // 0x21a51a0: GXHullShader::InitializeFromPSSL
void    sub_21a57f0(void* gsObj, const void* binData, size_t binSize, uint32_t arg4, uint32_t arg5);                         // 0x21a57f0: GXGeometryShader::InitializeFromPSSL
void    sub_21a5f70(void* psObj, const void* binData, size_t binSize, uint32_t arg4, uint32_t arg5);                         // 0x21a5f70: GXPixelShader::InitializeFromPSSL
void    sub_21a6510(void* vsObj, const void* binData, size_t binSize, uint32_t arg4, uint32_t arg5);                         // 0x21a6510: GXVertexShader::InitializeFromPSSL

// Deferred Resource Release Callbacks
void sub_e483a0(void* resObj);                                                   // 0xe483a0: ResourceObject::Destroy
void sub_e48400(void* resObj, void* ctx);                                        // 0xe48400: ResourceObject::ProcessReleaseTick

} // extern "C"

// ============================================================================
// Global Engine Pointers & VTables (.data / .bss / .rodata)
// ============================================================================
namespace {

// VTables & Global Singletons (addressed via RIP-relative in original binary)
#define g_vtable_53b6de0 (reinterpret_cast<uintptr_t*>(GX_VMA(0x53b6de0ULL)))
#define g_vtable_535fc50 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535fc50ULL)))
#define g_vtable_53b6d80 (reinterpret_cast<uintptr_t*>(GX_VMA(0x53b6d80ULL)))
#define g_vtable_535f9f0 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f9f0ULL)))
#define g_vtable_535fa10 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535fa10ULL)))
#define g_vtable_535f910 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f910ULL)))
#define g_vtable_535f940 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f940ULL)))
#define g_vtable_535f8e0 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f8e0ULL)))
#define g_vtable_535f880 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f880ULL)))
#define g_vtable_535f8b0 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f8b0ULL)))
#define g_vtable_535f850 (reinterpret_cast<uintptr_t*>(GX_VMA(0x535f850ULL)))

#define g_pStackCookie_53e3be0 (*reinterpret_cast<uint64_t**>(GX_VMA(0x53e3be0ULL)))

#define g_pStringHeap_5540670   (*reinterpret_cast<void**>(GX_VMA(0x5540670ULL)))
#define g_pShaderHeap_5540680   (reinterpret_cast<void**>(GX_VMA(0x5540680ULL)))
#define g_pDefaultHeap_5540688  (reinterpret_cast<void**>(GX_VMA(0x5540688ULL)))
#define g_pGXContextSys_55406c8 (reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))

// Thread-Local Storage simulation matching fs:0x0 + offset
struct TlsOverrideSlot {
    uint64_t reserved0;
    uint64_t reserved8;
    void*    overrideHeap; // +0x10
};
#define g_tlsSlot_53e44a8 (*reinterpret_cast<TlsOverrideSlot*>(gx_guest_tls_slot(0x53e44a8ULL)))
#define g_tlsLinearHeap_53e4568 (*reinterpret_cast<void**>(gx_guest_tls_slot(0x53e4568ULL)))
#define g_tlsOomMode_53e4318 (*reinterpret_cast<int32_t*>(gx_guest_tls_slot(0x53e4318ULL)))
#define g_tlsOomExc_53e4320 (*reinterpret_cast<uint64_t*>(gx_guest_tls_slot(0x53e4320ULL)))

// .rodata Constants (Exact Bitwise Values Extracted from eboot.bin)
alignas(16) constexpr uint32_t kMaskXYZ_4920b00[4] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u};
alignas(16) constexpr uint32_t kMaskXYZ_4920b10[4] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u};
alignas(16) constexpr uint32_t kUnitW_4920b20[4]   = {0x00000000u, 0x00000000u, 0x00000000u, 0x3F800000u};
constexpr float kFloatOne_492974c = 1.0f;

#define kStr_DLNewCpp_493bac3            (reinterpret_cast<const char*>(GX_VMA(0x493bac3ULL)))
#define kStr_HeapNotFound_4940207        (reinterpret_cast<const char*>(GX_VMA(0x4940207ULL)))
#define kStr_DLLinearHeapCpp_493c62d     (reinterpret_cast<const char*>(GX_VMA(0x493c62dULL)))
#define kStr_FreeUntilInvalidMark_493c690 (reinterpret_cast<const char*>(GX_VMA(0x493c690ULL)))
#define kStr_TransactionFailed_4737d80   (reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)))
#define kStr_IncompatibleHeap_4737dc0    (reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)))

#define kWStr_FormatPair_49d2c82 (reinterpret_cast<const char16_t*>(GX_VMA(0x49d2c82ULL)))
#define kWStr_ExtPpo_49d282c     (reinterpret_cast<const char16_t*>(GX_VMA(0x49d282cULL)))
#define kWStr_ExtVpo_49d2836     (reinterpret_cast<const char16_t*>(GX_VMA(0x49d2836ULL)))
#define kWStr_ExtHpo_49d2840     (reinterpret_cast<const char16_t*>(GX_VMA(0x49d2840ULL)))
#define kWStr_ExtDpo_49d284a     (reinterpret_cast<const char16_t*>(GX_VMA(0x49d284aULL)))
#define kWStr_ExtGpo_49d2854     (reinterpret_cast<const char16_t*>(GX_VMA(0x49d2854ULL)))
#define kWStr_ExtCpo_49d2862     (reinterpret_cast<const char16_t*>(GX_VMA(0x49d2862ULL)))

} // anonymous namespace

extern "C" {

// ============================================================================
// 1. sub_21c08b0 (0x21c08b0, 290 B) - BND4Reader::UnmountAndCleanup
// ============================================================================
void sub_21c08b0(void* self) {
    uint8_t* r15 = static_cast<uint8_t*>(self);
    uint8_t* r14 = *reinterpret_cast<uint8_t**>(r15 + 0x10);
    if (r14 != nullptr) {
        void* rbx = sub_207b720(r14);
        void* r12 = *reinterpret_cast<void**>(r14 + 0x8);
        if (r12 != nullptr) {
            void* r13 = g_tlsSlot_53e44a8.overrideHeap;
            g_tlsSlot_53e44a8.overrideHeap = nullptr;
            if (r13 == nullptr) {
                r13 = sub_207b720(r12);
                if (r13 == nullptr) {
                    sub_20b55b0(kStr_DLNewCpp_493bac3, 0x64, kStr_HeapNotFound_4940207);
                }
            }
            uintptr_t* vtbl13 = *reinterpret_cast<uintptr_t**>(r13);
            reinterpret_cast<void(*)(void*, void*)>(vtbl13[0x70 / 8])(r13, r12);
            *reinterpret_cast<void**>(r14 + 0x8) = nullptr;
        }
        sub_20f7c10(r14 + 0x28);
        uintptr_t* vtblBx = *reinterpret_cast<uintptr_t**>(rbx);
        reinterpret_cast<void(*)(void*, void*)>(vtblBx[0x70 / 8])(rbx, r14);
        *reinterpret_cast<void**>(r15 + 0x10) = nullptr;
    }

    void* rbxStream = *reinterpret_cast<void**>(r15 + 0x8);
    if (rbxStream != nullptr) {
        void* r14Heap = sub_207b720(rbxStream);
        uintptr_t* vtblStream = *reinterpret_cast<uintptr_t**>(rbxStream);
        reinterpret_cast<void(*)(void*)>(vtblStream[0x8 / 8])(rbxStream);
        uintptr_t* vtblHeap = *reinterpret_cast<uintptr_t**>(r14Heap);
        reinterpret_cast<void(*)(void*, void*)>(vtblHeap[0x70 / 8])(r14Heap, rbxStream);
        *reinterpret_cast<void**>(r15 + 0x8) = nullptr;
    }

    *reinterpret_cast<uint64_t*>(r15 + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(r15 + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(r15 + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(r15 + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(r15 + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(r15 + 0x18) = 0;
}

// ============================================================================
// 2. sub_21c08a0 (0x21c08a0, 5 B) - BND4Reader::~BND4Reader (Thunk)
// ============================================================================
void sub_21c08a0(void* self) {
    sub_21c08b0(self);
}

// ============================================================================
// 3. sub_21c0850 (0x21c0850, 68 B) - BND4Reader::BND4Reader
// ============================================================================
void sub_21c0850(void* self, void* allocator) {
    uint8_t* p = static_cast<uint8_t*>(self);
    *reinterpret_cast<void**>(p + 0x00) = allocator;
    *reinterpret_cast<uint64_t*>(p + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x08) = 0;
}

// ============================================================================
// 4. sub_21c0710 (0x21c0710, 167 B) - GXClosestPointAndDistSqToSegment
// ============================================================================
float sub_21c0710(const __m128* point, const __m128* segment, __m128* outClosest) {
    __m128 xmm0 = _mm_load_ps(reinterpret_cast<const float*>(point));
    __m128 xmm1 = _mm_load_ps(reinterpret_cast<const float*>(segment));
    __m128 xmm2 = _mm_load_ps(reinterpret_cast<const float*>(segment + 1));

    __m128 xmm3 = _mm_mul_ps(_mm_sub_ps(xmm0, xmm1), xmm2);
    __m128 xmm4 = _mm_hadd_ps(xmm3, xmm3);
    xmm3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm3), 0xAA));
    xmm4 = _mm_add_ps(xmm4, xmm3);

    __m128 zero = _mm_setzero_ps();
    float dotNumer = _mm_cvtss_f32(xmm4);
    if (0.0f < dotNumer) {
        __m128 sq2 = _mm_mul_ps(xmm2, xmm2);
        __m128 xmm5 = _mm_hadd_ps(sq2, sq2);
        __m128 sq2Z = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(sq2), 0xAA));
        __m128 lenSqV = _mm_add_ps(sq2Z, xmm5);
        float lenSq = _mm_cvtss_f32(lenSqV);
        if (dotNumer < lenSq) {
            xmm3 = _mm_div_ss(xmm4, lenSqV);
            if (_mm_cvtss_f32(xmm3) < 0.0f) {
                xmm4 = zero;
            } else {
                xmm4 = _mm_min_ss(_mm_set_ss(kFloatOne_492974c), xmm3);
            }
        } else {
            xmm4 = _mm_set_ss(kFloatOne_492974c);
        }
    } else {
        xmm4 = zero;
    }

    xmm3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x00));
    xmm2 = _mm_mul_ps(xmm2, xmm3);
    xmm1 = _mm_add_ps(xmm1, xmm2);
    xmm1 = _mm_and_ps(xmm1, _mm_load_ps(reinterpret_cast<const float*>(kMaskXYZ_4920b10)));
    xmm1 = _mm_or_ps(xmm1, _mm_load_ps(reinterpret_cast<const float*>(kUnitW_4920b20)));

    if (outClosest != nullptr) {
        _mm_store_ps(reinterpret_cast<float*>(outClosest), xmm1);
        xmm0 = _mm_load_ps(reinterpret_cast<const float*>(point));
    }

    xmm0 = _mm_sub_ps(xmm0, xmm1);
    xmm0 = _mm_mul_ps(xmm0, xmm0);
    xmm1 = _mm_hadd_ps(xmm0, xmm0);
    xmm0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0xAA));
    xmm0 = _mm_add_ps(xmm1, xmm0);
    return _mm_cvtss_f32(xmm0);
}

// ============================================================================
// 5. sub_21c0670 (0x21c0670, 148 B) - GXClosestPointAndDistSqToOBB
// ============================================================================
float sub_21c0670(const __m128* point, const __m128* obb, __m128* outClosest) {
    __m128 xmm0 = _mm_load_ps(reinterpret_cast<const float*>(point));
    __m128 xmm1 = _mm_sub_ps(xmm0, _mm_load_ps(reinterpret_cast<const float*>(obb + 4)));
    xmm1 = _mm_and_ps(xmm1, _mm_load_ps(reinterpret_cast<const float*>(kMaskXYZ_4920b00)));

    __m128 xmm2 = _mm_mul_ps(xmm1, _mm_load_ps(reinterpret_cast<const float*>(obb + 1)));
    xmm2 = _mm_hadd_ps(xmm2, xmm2);
    xmm2 = _mm_hadd_ps(xmm2, xmm2);

    __m128 xmm3 = _mm_mul_ps(xmm1, _mm_load_ps(reinterpret_cast<const float*>(obb + 2)));
    xmm3 = _mm_hadd_ps(xmm3, xmm3);
    xmm3 = _mm_hadd_ps(xmm3, xmm3);

    xmm1 = _mm_mul_ps(xmm1, _mm_load_ps(reinterpret_cast<const float*>(obb + 3)));
    xmm1 = _mm_hadd_ps(xmm1, xmm1);
    xmm1 = _mm_hadd_ps(xmm1, xmm1);

    xmm2 = _mm_shuffle_ps(xmm2, xmm3, 0x10);
    xmm2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x08));
    xmm1 = _mm_shuffle_ps(xmm2, xmm1, 0xE4);

    xmm2 = _mm_load_ps(reinterpret_cast<const float*>(obb));
    xmm3 = _mm_cmplt_ps(xmm2, xmm1);
    __m128 xmm4 = _mm_sub_ps(_mm_setzero_ps(), xmm2);
    xmm4 = _mm_cmplt_ps(xmm1, xmm4);

    __m128 xmm5 = _mm_sub_ps(xmm1, xmm2);
    xmm3 = _mm_and_ps(xmm5, xmm3);
    xmm1 = _mm_add_ps(xmm1, xmm2);
    xmm1 = _mm_and_ps(xmm1, xmm4);
    xmm2 = _mm_add_ps(xmm0, xmm3);
    xmm1 = _mm_add_ps(xmm2, xmm1);

    xmm0 = _mm_sub_ps(xmm0, xmm1);
    xmm0 = _mm_mul_ps(xmm0, xmm0);
    xmm2 = _mm_hadd_ps(xmm0, xmm0);
    xmm0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0xAA));
    xmm0 = _mm_add_ps(xmm2, xmm0);

    if (outClosest != nullptr) {
        _mm_store_ps(reinterpret_cast<float*>(outClosest), xmm1);
    }
    return _mm_cvtss_f32(xmm0);
}

// ============================================================================
// 6. sub_21c0660 (0x21c0660, 5 B) - GXTPFTextureArchive::GetTPFDataPtr
// ============================================================================
void* sub_21c0660(const void* self) {
    return *reinterpret_cast<void* const*>(static_cast<const uint8_t*>(self) + 0x08);
}

// ============================================================================
// 7. sub_21c0640 (0x21c0640, 19 B) - GXTPFTextureArchive::GetCurrentEntryFlag
// ============================================================================
int32_t sub_21c0640(const void* self) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    const int8_t* flagsArray = *reinterpret_cast<const int8_t* const*>(p + 0x28);
    if (flagsArray == nullptr) {
        return 0;
    }
    uint32_t idx = *reinterpret_cast<const uint32_t*>(p + 0x14);
    return static_cast<int32_t>(flagsArray[idx]);
}

// ============================================================================
// 8. sub_21c0620 (0x21c0620, 17 B) - GXTPFTextureArchive::SetCurrentEntryFlag
// ============================================================================
void sub_21c0620(void* self, uint8_t flagVal) {
    uint8_t* p = static_cast<uint8_t*>(self);
    uint8_t* flagsArray = *reinterpret_cast<uint8_t**>(p + 0x28);
    if (flagsArray != nullptr) {
        uint32_t idx = *reinterpret_cast<const uint32_t*>(p + 0x14);
        flagsArray[idx] = flagVal;
    }
}

// ============================================================================
// 9. sub_21c0610 (0x21c0610, 4 B) - GXTPFTextureArchive::GetCurrentIndex
// ============================================================================
uint32_t sub_21c0610(const void* self) {
    return *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(self) + 0x14);
}

// ============================================================================
// 10. sub_21c04c0 (0x21c04c0, 184 B) - GXTPFTextureArchive::CreateVolumeTexture
// ============================================================================
int32_t sub_21c04c0(void* self, void* outTex, void* outView, uint32_t extraFlags) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    uint8_t* rbx = static_cast<uint8_t*>(self);
    uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(rbx);
    uint32_t r12d = reinterpret_cast<uint32_t(*)(void*)>(vtbl[0x50 / 8])(rbx);

    const void* tpfData = *reinterpret_cast<const void**>(rbx + 0x08);
    uint32_t curIdx = *reinterpret_cast<uint32_t*>(rbx + 0x14);
    uint32_t dimType = sub_26d8840(tpfData, curIdx);
    int32_t result = 0x0C;
    if (dimType == 0) {
        r12d = ((r12d | extraFlags) & 0x06u) ^ 0x02u;
        void* gxSys = *reinterpret_cast<void**>(g_pGXContextSys_55406c8);
        void* dev = *reinterpret_cast<void**>(gxSys);
        void* alloc1 = *reinterpret_cast<void**>(rbx + 0x18);
        void* alloc2 = *reinterpret_cast<void**>(rbx + 0x20);
        int32_t err = sub_26d9fe0(outTex, outView, dev, *reinterpret_cast<const void**>(rbx + 0x08), alloc1, alloc2, r12d);
        result = 0;
        if (err != 0) {
            uint64_t dummyState = 0;
            sub_26d8230(&dummyState, *reinterpret_cast<const void**>(rbx + 0x08));
            result = err;
        }
    }
    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return result;
}

// ============================================================================
// 11. sub_21c03f0 (0x21c03f0, 199 B) - GXTPFTextureArchive::CreateCubeTexture
// ============================================================================
int32_t sub_21c03f0(void* self, void* outTex, void* outView, uint32_t extraFlags) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    uint8_t* rbx = static_cast<uint8_t*>(self);
    uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(rbx);
    uint32_t r12d = reinterpret_cast<uint32_t(*)(void*)>(vtbl[0x50 / 8])(rbx);

    const void* tpfData = *reinterpret_cast<const void**>(rbx + 0x08);
    uint32_t curIdx = *reinterpret_cast<uint32_t*>(rbx + 0x14);
    uint32_t dimType = sub_26d8840(tpfData, curIdx);
    int32_t result = 0x0F;
    if (dimType == 2) {
        r12d = ((r12d | extraFlags) & 0x06u) ^ 0x02u;
        void* gxSys = *reinterpret_cast<void**>(g_pGXContextSys_55406c8);
        void* dev = *reinterpret_cast<void**>(gxSys);
        void* alloc1 = *reinterpret_cast<void**>(rbx + 0x18);
        void* alloc2 = *reinterpret_cast<void**>(rbx + 0x20);
        int32_t err = sub_26d9850(
            outTex,
            outView,
            dev,
            *reinterpret_cast<const void**>(rbx + 0x08),
            *reinterpret_cast<uint32_t*>(rbx + 0x14),
            alloc1,
            alloc2,
            r12d);
        result = 0;
        if (err != 0) {
            uint64_t dummyState = 0;
            sub_26d8230(&dummyState, *reinterpret_cast<const void**>(rbx + 0x08));
            result = err;
        }
    }
    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return result;
}

// ============================================================================
// 12. sub_21c0320 (0x21c0320, 201 B) - GXTPFTextureArchive::Create2DTexture
// ============================================================================
int32_t sub_21c0320(void* self, void* outTex, void* outView, uint32_t extraFlags) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    uint8_t* r13 = static_cast<uint8_t*>(self);
    uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(r13);
    uint32_t ebx = reinterpret_cast<uint32_t(*)(void*)>(vtbl[0x50 / 8])(r13);
    ebx = ((ebx | extraFlags) & 0x06u) ^ 0x02u;

    uint32_t dimType = sub_26d8840(
        *reinterpret_cast<const void**>(r13 + 0x08),
        *reinterpret_cast<uint32_t*>(r13 + 0x14));
    int32_t r14d = 0;
    if (dimType == 0 || dimType == 1 || dimType == 3) {
        void* gxSys = *reinterpret_cast<void**>(g_pGXContextSys_55406c8);
        void* dev = *reinterpret_cast<void**>(gxSys);
        int32_t err = sub_26d9070(
            outTex,
            outView,
            dev,
            *reinterpret_cast<const void**>(r13 + 0x08),
            *reinterpret_cast<uint32_t*>(r13 + 0x14),
            *reinterpret_cast<void**>(r13 + 0x18),
            *reinterpret_cast<void**>(r13 + 0x20),
            ebx);
        if (err != 0) {
            uint64_t dummyState = 0;
            sub_26d8230(&dummyState, *reinterpret_cast<const void**>(r13 + 0x08));
            r14d = err;
        }
    }
    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return r14d;
}

// ============================================================================
// 13. sub_21c0310 (0x21c0310, 15 B) - GXTPFTextureArchive::GetCurrentDataSize
// ============================================================================
uint32_t sub_21c0310(const void* self) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    return sub_26d8a20(
        *reinterpret_cast<const void* const*>(p + 0x08),
        *reinterpret_cast<const uint32_t*>(p + 0x14));
}

// ============================================================================
// 14. sub_21c02f0 (0x21c02f0, 28 B) - GXTPFTextureArchive::GetArchiveState
// ============================================================================
void* sub_21c02f0(void* outState, const void* self) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    sub_26d8230(outState, *reinterpret_cast<const void* const*>(p + 0x08));
    return outState;
}

// ============================================================================
// 15. sub_21c02e0 (0x21c02e0, 15 B) - GXTPFTextureArchive::GetCurrentTextureName
// ============================================================================
const char* sub_21c02e0(const void* self) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    return sub_26d8c10(
        *reinterpret_cast<const void* const*>(p + 0x08),
        *reinterpret_cast<const uint32_t*>(p + 0x14));
}

// ============================================================================
// 16. sub_21c02b0 (0x21c02b0, 47 B) - GXTPFTextureArchive::TryGetTextureDataPtr
// ============================================================================
bool sub_21c02b0(const void* self, uint32_t subIndex, void** outPtr) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    void* ptr = sub_26d8df0(
        *reinterpret_cast<const void* const*>(p + 0x08),
        *reinterpret_cast<const uint32_t*>(p + 0x14),
        subIndex,
        outPtr,
        0);
    *outPtr = ptr;
    return ptr != nullptr;
}

// ============================================================================
// 17. sub_21c0290 (0x21c0290, 19 B) - GXTPFTextureArchive::GetPlatformByte
// ============================================================================
int32_t sub_21c0290(const void* self) {
    const uint8_t* tpfData = *reinterpret_cast<const uint8_t* const*>(static_cast<const uint8_t*>(self) + 0x08);
    if (tpfData == nullptr) {
        return -1;
    }
    return static_cast<int32_t>(*reinterpret_cast<const int8_t*>(tpfData + 0x0C));
}

// ============================================================================
// 18. sub_21c0280 (0x21c0280, 15 B) - GXTPFTextureArchive::GetCurrentDimType
// ============================================================================
uint32_t sub_21c0280(const void* self) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    return sub_26d8840(
        *reinterpret_cast<const void* const*>(p + 0x08),
        *reinterpret_cast<const uint32_t*>(p + 0x14));
}

// ============================================================================
// 19. sub_21c0270 (0x21c0270, 9 B) - GXTPFTextureArchive::GetTextureCount
// ============================================================================
uint32_t sub_21c0270(const void* self) {
    const uint8_t* p = static_cast<const uint8_t*>(self);
    return sub_26d81f0(*reinterpret_cast<const void* const*>(p + 0x08));
}

// ============================================================================
// 20. sub_21c0240 (0x21c0240, 34 B) - GXTPFTextureArchive::SetCurrentIndex
// ============================================================================
void sub_21c0240(void* self, uint32_t newIndex) {
    uint8_t* rbx = static_cast<uint8_t*>(self);
    uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(rbx);
    uint32_t count = reinterpret_cast<uint32_t(*)(void*)>(vtbl[0x20 / 8])(rbx);
    if (count > newIndex) {
        count = newIndex;
    }
    *reinterpret_cast<uint32_t*>(rbx + 0x14) = count;
}

// ============================================================================
// 21. sub_21c0200 (0x21c0200, 63 B) - GXTPFTextureArchive::~GXTPFTextureArchive
// ============================================================================
void sub_21c0200(void* self) {
    uint8_t* r14 = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(r14) = g_vtable_53b6de0;
    uint8_t* rbx = *reinterpret_cast<uint8_t**>(r14 + 0x28);
    if (rbx != nullptr) {
        void* heap = sub_207b720(rbx);
        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(heap);
        void* baseAlloc = *reinterpret_cast<void**>(rbx - 0x10);
        reinterpret_cast<void(*)(void*, void*)>(vtbl[0x70 / 8])(heap, baseAlloc);
        *reinterpret_cast<void**>(r14 + 0x28) = nullptr;
    }
}

// ============================================================================
// 22. sub_21c01c0 (0x21c01c0, 62 B) - GXTPFTextureArchive::DeletingDtor
// ============================================================================
void sub_21c01c0(void* self) {
    uint8_t* r14 = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(r14) = g_vtable_53b6de0;
    uint8_t* rbx = *reinterpret_cast<uint8_t**>(r14 + 0x28);
    if (rbx != nullptr) {
        void* heap = sub_207b720(rbx);
        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(heap);
        void* baseAlloc = *reinterpret_cast<void**>(rbx - 0x10);
        reinterpret_cast<void(*)(void*, void*)>(vtbl[0x70 / 8])(heap, baseAlloc);
    }
    sub_2bbe588(r14);
}

// ============================================================================
// 23. sub_21c0060 (0x21c0060, 346 B) - GXTPFTextureArchive::GXTPFTextureArchive
// ============================================================================
void sub_21c0060(void* self, const void* tpfData, uint32_t dataSize, void* alloc1, void* alloc2) {
    uint8_t* r14 = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(r14 + 0x00) = g_vtable_53b6de0;
    *reinterpret_cast<const void**>(r14 + 0x08) = tpfData;
    *reinterpret_cast<uint32_t*>(r14 + 0x10) = dataSize;
    *reinterpret_cast<uint32_t*>(r14 + 0x14) = 0;
    *reinterpret_cast<void**>(r14 + 0x18) = alloc1;
    *reinterpret_cast<void**>(r14 + 0x20) = alloc2;
    *reinterpret_cast<void**>(r14 + 0x28) = nullptr;

    uint32_t texCount = sub_26d81f0(tpfData);
    if (texCount == 0) {
        return;
    }

    if (*reinterpret_cast<void**>(r14 + 0x28) == nullptr) {
        void* heap = *reinterpret_cast<void**>(g_pDefaultHeap_5540688);
        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(heap);
        size_t allocBytes = static_cast<size_t>(texCount) + 0x10;
        uint8_t* block = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(vtbl[0x58 / 8])(heap, allocBytes, 0x10);
        if (block == nullptr) {
            *reinterpret_cast<void**>(r14 + 0x28) = nullptr;
            return;
        }
        *reinterpret_cast<uint64_t*>(block + 0x08) = texCount;
        *reinterpret_cast<void**>(block + 0x00) = block;
        *reinterpret_cast<void**>(r14 + 0x28) = block + 0x10;
    }

    for (uint32_t i = 0; i < texCount; ++i) {
        const void* curTpf = *reinterpret_cast<const void**>(r14 + 0x08);
        uint32_t rawFlags = sub_26d8280(curTpf, i);
        int32_t fmtCode = sub_26d8660(curTpf, i);
        int32_t mappedFmt = fmtCode;
        if (fmtCode <= 0x47) {
            if (fmtCode == 0x1D) {
                mappedFmt = 0x1C;
            }
        } else if (fmtCode <= 0x62) {
            if (fmtCode > 0x5A) {
                if (fmtCode == 0x5B) {
                    mappedFmt = 0x57;
                } else if (fmtCode == 0x5D) {
                    mappedFmt = 0x58;
                }
            } else {
                if (fmtCode == 0x48) {
                    mappedFmt = 0x47;
                } else if (fmtCode == 0x4B) {
                    mappedFmt = 0x4A;
                } else if (fmtCode == 0x4E) {
                    mappedFmt = 0x4D;
                }
            }
        } else if (fmtCode == 0x63) {
            mappedFmt = 0x62;
        }

        uint32_t edx = (mappedFmt == fmtCode) ? rawFlags : (rawFlags | 0x02u);
        uint8_t* flagsOut = *reinterpret_cast<uint8_t**>(r14 + 0x28);
        flagsOut[i] = static_cast<uint8_t>(((edx & 0x06u) ^ 0x02u));
    }
}

// ============================================================================
// 24..33. Virtual Stubs / Accessors on GXDDSTextureArchive (0x21bffc0..0x21c0050)
// ============================================================================
uint32_t sub_21c0050(const void* self) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self) & 0u);
}

uint32_t sub_21c0040(const void* self) {
    return (self != nullptr) ? 0x0Cu : 0x0Cu;
}

uint32_t sub_21c0030(const void* self) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self) & 0u);
}

void sub_21c0020(void* self) {
    if (self == nullptr) {
        return;
    }
}

const char* sub_21c0010(const void* self) {
    return (self != nullptr) ? nullptr : nullptr;
}

bool sub_21c0000(const void* self) {
    return (reinterpret_cast<uintptr_t>(self) & 0u) != 0u;
}

int32_t sub_21bfff0(const void* self) {
    return (self != nullptr) ? -1 : -1;
}

uint32_t sub_21bffe0(const void* self) {
    return (self != nullptr) ? 1u : 1u;
}

uint32_t sub_21bffd0(const void* self) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self) & 0u);
}

void sub_21bffc0(void* self) {
    if (self == nullptr) {
        return;
    }
}

// ============================================================================
// 34. sub_21bf990 (0x21bf990, 1439 B) - GXDDSTextureArchive::Create2DOrVolumeTexture
// ============================================================================
int32_t sub_21bf990(void* self, void* outTex, void* outView, uint32_t sliceMode, uint32_t depthDivisor) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    void* gxSys = *reinterpret_cast<void**>(g_pGXContextSys_55406c8);
    void* dev = *reinterpret_cast<void**>(gxSys);

    const uint8_t* pSelf = static_cast<const uint8_t*>(self);
    const uint8_t* r12 = *reinterpret_cast<const uint8_t* const*>(pSelf + 0x08);
    void* alloc1 = *reinterpret_cast<void* const*>(pSelf + 0x18);
    void* alloc2 = *reinterpret_cast<void* const*>(pSelf + 0x20);

    int32_t finalResult = 0x08;
    if (sub_26d7510(r12)) {
        uint32_t arrayCount = depthDivisor;
        if (sliceMode == 0) {
            arrayCount = sub_26d7a20(r12);
        }
        uint32_t rawMipCount = *reinterpret_cast<const uint32_t*>(r12 + 0x1C);
        uint32_t mipCount = (rawMipCount != 0) ? rawMipCount : 1u;

        uint8_t* linearHeap = static_cast<uint8_t*>(g_tlsLinearHeap_53e4568);
        if (linearHeap == nullptr) {
            linearHeap = static_cast<uint8_t*>(sub_207d250());
            g_tlsLinearHeap_53e4568 = linearHeap;
        }
        uint8_t* savedMark = *reinterpret_cast<uint8_t**>(linearHeap + 0x10);
        size_t bytesNeeded = static_cast<size_t>(mipCount * arrayCount) << 4;
        uint8_t* newCur = savedMark + bytesNeeded;
        uint8_t* endLimit = *reinterpret_cast<uint8_t**>(linearHeap + 0x18);

        struct MipSliceDesc {
            const void* dataPtr;
            uint32_t    pitch;
            uint32_t    sliceSize;
        };

        uint32_t errFlag = 0;
        uint32_t r15Err = 0;

        if (newCur > endLimit || savedMark == nullptr) {
            r15Err = 0x09;
            errFlag = 1;
        } else {
            *reinterpret_cast<uint8_t**>(linearHeap + 0x10) = newCur;
            MipSliceDesc* mipDescs = reinterpret_cast<MipSliceDesc*>(savedMark);
            uint32_t r15Div = (sliceMode == 0) ? *reinterpret_cast<const uint32_t*>(r12 + 0x18) : depthDivisor;
            if (r15Div == 0u) {
                r15Div = 1u;
            }

            if (arrayCount != 0) {
                uint32_t mipStride = (rawMipCount > 1u) ? rawMipCount : 1u;
                uint32_t baseOffsetIdx = 0;
                for (uint32_t sliceIdx = 0; sliceIdx < arrayCount; ++sliceIdx, baseOffsetIdx += mipStride) {
                    if (sliceMode == 0) {
                        for (uint32_t m = 0; m < mipCount; ++m) {
                            uint32_t off = sub_26d7b40(r12, m, sliceIdx);
                            uint32_t dstIdx = baseOffsetIdx + m;
                            mipDescs[dstIdx].dataPtr = r12 + off;
                            mipDescs[dstIdx].pitch = sub_26d7a50(r12, m);
                            mipDescs[dstIdx].sliceSize = sub_26d7ab0(r12, m) / r15Div;
                        }
                    } else if (sliceMode == 1) {
                        for (uint32_t m = 0; m < mipCount; ++m) {
                            uint32_t slicePart = (sub_26d7ab0(r12, m) / r15Div) * sliceIdx;
                            int64_t off = static_cast<int32_t>(sub_26d7b40(r12, m, 0));
                            uint32_t dstIdx = baseOffsetIdx + m;
                            mipDescs[dstIdx].dataPtr = r12 + off + static_cast<int32_t>(slicePart);
                            mipDescs[dstIdx].pitch = sub_26d7a50(r12, m) / r15Div;
                            mipDescs[dstIdx].sliceSize = sub_26d7ab0(r12, m) / r15Div;
                        }
                    } else {
                        for (uint32_t m = 0; m < mipCount; ++m) {
                            uint32_t slicePart = (sub_26d7ab0(r12, m) / r15Div) * sliceIdx;
                            int64_t off = static_cast<int32_t>(sub_26d7b40(r12, m, 0));
                            uint32_t dstIdx = baseOffsetIdx + m;
                            mipDescs[dstIdx].dataPtr = r12 + off + static_cast<int32_t>(slicePart);
                            mipDescs[dstIdx].pitch = sub_26d7a50(r12, m);
                            mipDescs[dstIdx].sliceSize = sub_26d7ab0(r12, m) / r15Div;
                        }
                    }
                }
            }

            uint32_t gxFormat = sub_26d7530(r12);
            uint32_t width = *reinterpret_cast<const uint32_t*>(r12 + 0x10);
            uint32_t height = *reinterpret_cast<const uint32_t*>(r12 + 0x0C);
            if (sliceMode == 1) {
                width /= r15Div;
            } else if (sliceMode == 2) {
                height /= r15Div;
            }

            struct GXTex2D3DDesc {
                uint32_t width;      // rbp-0x58
                uint32_t height;     // rbp-0x54
                uint32_t depth;      // rbp-0x50
                uint32_t pad0;       // rbp-0x4c
                uint32_t format;     // rbp-0x48
                uint32_t pad1;       // rbp-0x44
                uint32_t usageFlags; // rbp-0x40 (0x8)
                uint32_t pad2;       // rbp-0x3c
                uint32_t pad3;       // rbp-0x38
                uint32_t pad4;       // rbp-0x34
            } texDesc = {width, height, r15Div, 0, gxFormat, 0, 0x08, 0, 0, 0};

            int32_t createErr = sub_2166300(dev, outTex, &texDesc, mipDescs, alloc1, alloc2, nullptr);
            if (createErr != 0) {
                r15Err = static_cast<uint32_t>(createErr);
                errFlag = 1;
            } else {
                struct GXSRVDesc {
                    uint32_t format;   // rbp-0x70
                    uint32_t viewType; // rbp-0x6c (0x8)
                    uint32_t firstMip; // rbp-0x68 (0)
                    uint32_t mipCount; // rbp-0x64
                    uint64_t reserved; // rbp-0x60 (0)
                } srvDesc = {gxFormat, 0x08, 0, mipCount, 0};

                void* curDev = *reinterpret_cast<void**>(*reinterpret_cast<void**>(g_pGXContextSys_55406c8));
                void* createdTexObj = *reinterpret_cast<void**>(outTex);
                int32_t srvErr = sub_21669a0(curDev, outView, createdTexObj, &srvDesc, 0);
                if (srvErr != 0) {
                    uint8_t* texRef = *reinterpret_cast<uint8_t**>(outTex);
                    if (texRef != nullptr) {
                        int32_t oldRef = __sync_fetch_and_add(reinterpret_cast<int32_t*>(texRef + 0x10), -1);
                        if (oldRef == 1) {
                            void* texHeap = *reinterpret_cast<void**>(texRef + 0x08);
                            uintptr_t* texVtbl = *reinterpret_cast<uintptr_t**>(texRef);
                            reinterpret_cast<void(*)(void*)>(texVtbl[0])(texRef);
                            uintptr_t* heapVtbl = *reinterpret_cast<uintptr_t**>(texHeap);
                            reinterpret_cast<void(*)(void*, void*)>(heapVtbl[0x70 / 8])(texHeap, texRef);
                        }
                        *reinterpret_cast<void**>(outTex) = nullptr;
                    }
                    errFlag = 1;
                    r15Err = static_cast<uint32_t>(srvErr);
                }
            }
        }

        // FreeUntil(savedMark) on linear heap
        uint8_t* curHeap = static_cast<uint8_t*>(g_tlsLinearHeap_53e4568);
        if (curHeap == nullptr) {
            curHeap = static_cast<uint8_t*>(sub_207d250());
            g_tlsLinearHeap_53e4568 = curHeap;
        }
        uint8_t* heapStart = *reinterpret_cast<uint8_t**>(curHeap + 0x00);
        uint8_t* heapEnd   = *reinterpret_cast<uint8_t**>(curHeap + 0x08);
        if (heapStart > savedMark || heapEnd < savedMark) {
            sub_20b55b0(kStr_DLLinearHeapCpp_493c62d, 0x233, kStr_FreeUntilInvalidMark_493c690);
        }
        if (*reinterpret_cast<uint8_t**>(curHeap + 0x10) > savedMark) {
            *reinterpret_cast<uint8_t**>(curHeap + 0x10) = savedMark;
        }
        finalResult = (errFlag != 0) ? static_cast<int32_t>(r15Err) : 0;
    }

    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return finalResult;
}

// ============================================================================
// 35. sub_21bf980 (0x21bf980, 13 B) - GXDDSTextureArchive::Create2DTextureDefault
// ============================================================================
int32_t sub_21bf980(void* self, void* outTex, void* outView) {
    return sub_21bf990(self, outTex, outView, 0, 1);
}

// ============================================================================
// 36. sub_21bf550 (0x21bf550, 1065 B) - GXDDSTextureArchive::CreateCubeTexture
// ============================================================================
int32_t sub_21bf550(void* self, void* outTex, void* outView) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    void* gxSys = *reinterpret_cast<void**>(g_pGXContextSys_55406c8);
    void* dev = *reinterpret_cast<void**>(gxSys);

    const uint8_t* pSelf = static_cast<const uint8_t*>(self);
    const uint8_t* rbx = *reinterpret_cast<const uint8_t* const*>(pSelf + 0x08);
    void* alloc1 = *reinterpret_cast<void* const*>(pSelf + 0x18);
    void* alloc2 = *reinterpret_cast<void* const*>(pSelf + 0x20);

    uint32_t r15Err = 0x08;
    if (sub_26d7510(rbx)) {
        uint32_t faceOrArrayCount = sub_26d7a20(rbx);
        uint32_t rawMipCount = *reinterpret_cast<const uint32_t*>(rbx + 0x1C);
        uint32_t mipCount = (rawMipCount != 0) ? rawMipCount : 1u;

        uint8_t* linearHeap = static_cast<uint8_t*>(g_tlsLinearHeap_53e4568);
        if (linearHeap == nullptr) {
            linearHeap = static_cast<uint8_t*>(sub_207d250());
            g_tlsLinearHeap_53e4568 = linearHeap;
        }
        uint8_t* savedMark = *reinterpret_cast<uint8_t**>(linearHeap + 0x10);
        size_t bytesNeeded = static_cast<size_t>(mipCount * faceOrArrayCount) << 4;
        uint8_t* newCur = savedMark + bytesNeeded;
        uint8_t* endLimit = *reinterpret_cast<uint8_t**>(linearHeap + 0x18);

        struct MipSliceDesc {
            const void* dataPtr;
            uint32_t    pitch;
            uint32_t    sliceSize;
        };

        uint32_t errFlag = 0;
        r15Err = 0x09;

        if (newCur > endLimit || savedMark == nullptr) {
            errFlag = 1;
        } else {
            *reinterpret_cast<uint8_t**>(linearHeap + 0x10) = newCur;
            MipSliceDesc* mipDescs = reinterpret_cast<MipSliceDesc*>(savedMark);

            if (faceOrArrayCount != 0) {
                uint32_t mipStride = (rawMipCount > 1u) ? rawMipCount : 1u;
                uint32_t baseOffsetIdx = 0;
                for (uint32_t faceIdx = 0; faceIdx < faceOrArrayCount; ++faceIdx, baseOffsetIdx += mipStride) {
                    for (uint32_t m = 0; m < mipCount; ++m) {
                        uint32_t off = sub_26d7b40(rbx, m, faceIdx);
                        uint32_t dstIdx = baseOffsetIdx + m;
                        mipDescs[dstIdx].dataPtr = rbx + off;
                        mipDescs[dstIdx].pitch = sub_26d7a50(rbx, m);
                        mipDescs[dstIdx].sliceSize = sub_26d7ab0(rbx, m);
                    }
                }
            }

            uint32_t gxFormat = sub_26d7530(rbx);
            uint32_t cubeFlags = (*reinterpret_cast<const uint32_t*>(rbx + 0x70) != 0) ? 0x04u : 0x00u;
            uint32_t width  = *reinterpret_cast<const uint32_t*>(rbx + 0x10);
            uint32_t height = *reinterpret_cast<const uint32_t*>(rbx + 0x0C);

            struct GXTexCubeDesc {
                uint32_t width;      // rbp-0x60
                uint32_t height;     // rbp-0x5c
                uint32_t mipCount;   // rbp-0x58
                uint32_t arrayCount; // rbp-0x54
                uint32_t format;     // rbp-0x50
                uint32_t unk1;       // rbp-0x4c (1)
                uint32_t unk0;       // rbp-0x48 (0)
                uint32_t pad0;       // rbp-0x44 (0)
                uint32_t usageFlags; // rbp-0x40 (0x8)
                uint32_t pad1;       // rbp-0x3c (0)
                uint32_t cubeFlags;  // rbp-0x38 (0 or 4)
                uint32_t pad2;       // rbp-0x34 (0)
            } cubeDesc = {width, height, mipCount, faceOrArrayCount, gxFormat, 1, 0, 0, 0x08, 0, cubeFlags, 0};

            int32_t createErr = sub_2165f90(dev, outTex, &cubeDesc, mipDescs, alloc1, alloc2, nullptr);
            r15Err = static_cast<uint32_t>(createErr);
            if (createErr != 0) {
                errFlag = 1;
            } else {
                uint32_t srvViewType = (*reinterpret_cast<const uint32_t*>(rbx + 0x70) != 0) ? 0x09u : 0x04u;
                struct GXSRVDesc {
                    uint32_t format;    // rbp-0x78
                    uint32_t viewType;  // rbp-0x74
                    uint32_t firstMip;  // rbp-0x70 (0)
                    uint32_t mipCount;  // rbp-0x6c
                    uint32_t firstElem; // rbp-0x68 (0)
                    uint32_t numElems;  // rbp-0x64 (1)
                } srvDesc = {gxFormat, srvViewType, 0, mipCount, 0, 1};

                void* curDev = *reinterpret_cast<void**>(*reinterpret_cast<void**>(g_pGXContextSys_55406c8));
                void* createdTexObj = *reinterpret_cast<void**>(outTex);
                int32_t srvErr = sub_21669a0(curDev, outView, createdTexObj, &srvDesc, 0);
                r15Err = 0;
                errFlag = 0;
                if (srvErr != 0) {
                    uint8_t* texRef = *reinterpret_cast<uint8_t**>(outTex);
                    if (texRef != nullptr) {
                        int32_t oldRef = __sync_fetch_and_add(reinterpret_cast<int32_t*>(texRef + 0x10), -1);
                        if (oldRef == 1) {
                            void* texHeap = *reinterpret_cast<void**>(texRef + 0x08);
                            uintptr_t* texVtbl = *reinterpret_cast<uintptr_t**>(texRef);
                            reinterpret_cast<void(*)(void*)>(texVtbl[0])(texRef);
                            uintptr_t* heapVtbl = *reinterpret_cast<uintptr_t**>(texHeap);
                            reinterpret_cast<void(*)(void*, void*)>(heapVtbl[0x70 / 8])(texHeap, texRef);
                        }
                        *reinterpret_cast<void**>(outTex) = nullptr;
                    }
                    errFlag = 1;
                    r15Err = static_cast<uint32_t>(srvErr);
                }
            }
        }

        // FreeUntil(savedMark) on linear heap
        uint8_t* curHeap = static_cast<uint8_t*>(g_tlsLinearHeap_53e4568);
        if (curHeap == nullptr) {
            curHeap = static_cast<uint8_t*>(sub_207d250());
            g_tlsLinearHeap_53e4568 = curHeap;
        }
        uint8_t* heapStart = *reinterpret_cast<uint8_t**>(curHeap + 0x00);
        uint8_t* heapEnd   = *reinterpret_cast<uint8_t**>(curHeap + 0x08);
        if (heapStart > savedMark || heapEnd < savedMark) {
            sub_20b55b0(kStr_DLLinearHeapCpp_493c62d, 0x233, kStr_FreeUntilInvalidMark_493c690);
        }
        if (*reinterpret_cast<uint8_t**>(curHeap + 0x10) > savedMark) {
            *reinterpret_cast<uint8_t**>(curHeap + 0x10) = savedMark;
        }
        if (errFlag == 0) {
            r15Err = 0;
        }
    }

    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return static_cast<int32_t>(r15Err);
}

// ============================================================================
// 37. sub_21bf540 (0x21bf540, 15 B) - GXDDSTextureArchive::IsCubeMap
// ============================================================================
uint32_t sub_21bf540(const void* self) {
    const uint8_t* ddsData = *reinterpret_cast<const uint8_t* const*>(static_cast<const uint8_t*>(self) + 0x08);
    return (*reinterpret_cast<const uint32_t*>(ddsData + 0x70) != 0) ? 1u : 0u;
}

// ============================================================================
// 38. sub_21bf530 (0x21bf530, 1 B) - GXDDSTextureArchive::~GXDDSTextureArchive
// ============================================================================
void sub_21bf530(void* self) {
    if (self == nullptr) {
        return;
    }
}

// ============================================================================
// 39. sub_21bf520 (0x21bf520, 5 B) - GXDDSTextureArchive::DeletingDtor
// ============================================================================
void sub_21bf520(void* self) {
    sub_2bbe588(self);
}

// ============================================================================
// 40. sub_21bf500 (0x21bf500, 26 B) - GXDDSTextureArchive::GXDDSTextureArchive
// ============================================================================
void sub_21bf500(void* self, const void* ddsData, uint32_t dataSize, void* alloc1, void* alloc2) {
    uint8_t* p = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(p + 0x00) = g_vtable_535fc50;
    *reinterpret_cast<const void**>(p + 0x08) = ddsData;
    *reinterpret_cast<uint32_t*>(p + 0x10) = dataSize;
    *reinterpret_cast<void**>(p + 0x18) = alloc1;
    *reinterpret_cast<void**>(p + 0x20) = alloc2;
}

// ============================================================================
// Internal Structure for GXShaderPackageLoader Entries (sizeof == 0x48 = 72 B)
// ============================================================================
struct GXShaderPackageEntry {
    void*     shaderObj;       // +0x00
    uint32_t  shaderStage;     // +0x08 (0=CS, 1=DS, 2=HS, 3=GS, 4=PS, 5=VS)
    uint32_t  pad0C;           // +0x0C
    uint64_t  wstrBoxHdr;      // +0x10 (DLWString start at +0x10)
    union {
        char16_t inlineBuf[8]; // +0x18
        char16_t* heapPtr;     // +0x18
    };
    uint64_t  wstrLength;      // +0x28
    uint64_t  wstrCapacity;    // +0x30
    void*     wstrAllocator;   // +0x38
    uint8_t   wstrFlag;        // +0x40
    uint8_t   pad41[7];        // +0x41..0x47
};

// ============================================================================
// 41. sub_21bf340 (0x21bf340, 438 B) - GXShaderPackageLoader::ReallocateVector
// ============================================================================
void sub_21bf340(void* vecSelf, size_t newCapacity) {
    uint8_t* rbx = static_cast<uint8_t*>(vecSelf);
    void* allocator = *reinterpret_cast<void**>(rbx + 0x20);
    uintptr_t* allocVtbl = *reinterpret_cast<uintptr_t**>(allocator);
    size_t allocBytes = newCapacity * 0x48;
    uint8_t* r15 = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(allocVtbl[0x58 / 8])(allocator, allocBytes, 0x08);
    if (r15 == nullptr) {
        if (g_tlsOomMode_53e4318 != 0) {
            sub_2bbe978(&g_tlsOomExc_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, kStr_TransactionFailed_4737d80);
    }

    uint8_t* r12 = *reinterpret_cast<uint8_t**>(rbx + 0x08);
    uint8_t* r13 = *reinterpret_cast<uint8_t**>(rbx + 0x10);
    if (r12 != r13) {
        size_t totalBytes = static_cast<size_t>(r13 - r12);
        for (size_t off = 0; off != totalBytes; off += 0x48) {
            uint8_t* dst = r15 + off;
            const uint8_t* src = r12 + off;
            if (dst != nullptr) {
                *reinterpret_cast<uint32_t*>(dst + 0x08) = *reinterpret_cast<const uint32_t*>(src + 0x08);
                *reinterpret_cast<void**>(dst + 0x00)     = *reinterpret_cast<void* const*>(src + 0x00);
                *reinterpret_cast<void**>(dst + 0x38)     = *reinterpret_cast<void* const*>(src + 0x38);
                *reinterpret_cast<uint64_t*>(dst + 0x30)  = 7;
                *reinterpret_cast<uint64_t*>(dst + 0x28)  = 0;
                *reinterpret_cast<uint16_t*>(dst + 0x18)  = 0;
                sub_26279a0(dst + 0x10, src + 0x10, 0, ~static_cast<size_t>(0));
                dst[0x40] = src[0x40];
            }
        }
        r13 = *reinterpret_cast<uint8_t**>(rbx + 0x08);
        r12 = *reinterpret_cast<uint8_t**>(rbx + 0x10);
    } else {
        r13 = r12;
    }

    if (r13 != nullptr) {
        for (uint8_t* cur = r13; cur != r12; cur += 0x48) {
            if (*reinterpret_cast<uint64_t*>(cur + 0x30) >= 8) {
                void* strBuf = *reinterpret_cast<void**>(cur + 0x18);
                void* strAlloc = *reinterpret_cast<void**>(cur + 0x38);
                uintptr_t* strVtbl = *reinterpret_cast<uintptr_t**>(strAlloc);
                reinterpret_cast<void(*)(void*, void*)>(strVtbl[0x70 / 8])(strAlloc, strBuf);
            }
            *reinterpret_cast<uint64_t*>(cur + 0x30) = 7;
            *reinterpret_cast<uint64_t*>(cur + 0x28) = 0;
            *reinterpret_cast<uint16_t*>(cur + 0x18) = 0;
        }
        void* vecAlloc = *reinterpret_cast<void**>(rbx + 0x20);
        uintptr_t* vecVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
        reinterpret_cast<void(*)(void*, void*)>(vecVtbl[0x70 / 8])(vecAlloc, *reinterpret_cast<void**>(rbx + 0x08));
    }

    size_t oldCount = static_cast<size_t>(r12 - r13) / 0x48;
    *reinterpret_cast<uint8_t**>(rbx + 0x18) = r15 + newCapacity * 0x48;
    *reinterpret_cast<uint8_t**>(rbx + 0x10) = r15 + oldCount * 0x48;
    *reinterpret_cast<uint8_t**>(rbx + 0x08) = r15;
}

// ============================================================================
// Internal Helper for Stage-Specific Shader Lookup (Used by 0x21be710..0x21bf0c0)
// ============================================================================
namespace {

inline void* LookupShaderByStageAndName(
    const void* self,
    const char16_t* prefixWStr,
    const char16_t* nameWStr,
    uint32_t targetStage)
{
    uint64_t cookie = *g_pStackCookie_53e3be0;
    void* strHeap = g_pStringHeap_5540670;
    if (strHeap == nullptr) {
        strHeap = sub_2ba2b10();
        g_pStringHeap_5540670 = strHeap;
    }

    uint8_t heapCaps[16] = {};
    uintptr_t* heapVtbl = *reinterpret_cast<uintptr_t**>(strHeap);
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(heapVtbl[0x20 / 8])(heapCaps, strHeap, 0);
    if ((heapCaps[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3E, kStr_IncompatibleHeap_4737dc0);
    }

    struct TempDLWString {
        uint64_t  hdr;       // rbp-0x68
        union {
            char16_t  buf[8]; // rbp-0x60
            char16_t* ptr;    // rbp-0x60
        };
        uint64_t  length;    // rbp-0x50
        uint64_t  capacity;  // rbp-0x48
        void*     allocator; // rbp-0x40
        uint8_t   flag;      // rbp-0x38
        uint8_t   pad[7];
    } queryStr = {};

    queryStr.allocator = strHeap;
    queryStr.capacity  = 7;
    queryStr.length    = 0;
    queryStr.buf[0]    = 0;
    queryStr.flag      = 1;

    if (prefixWStr != nullptr) {
        sub_26295d0(&queryStr, kWStr_FormatPair_49d2c82, prefixWStr, nameWStr);
    } else {
        size_t len = (nameWStr[0] != 0) ? sub_2bbf228(nameWStr) : 0;
        sub_262a720(&queryStr, nameWStr, len);
    }

    const uint8_t* r12 = static_cast<const uint8_t*>(self);
    const uint8_t* cur = *reinterpret_cast<const uint8_t* const*>(r12 + 0x10);
    const uint8_t* end = *reinterpret_cast<const uint8_t* const*>(r12 + 0x18);
    void* foundShader  = nullptr;

    if (cur != end) {
        const uint16_t* lowerTbl = sub_2bbec48();
        const char16_t* qStrBase = (queryStr.capacity > 7) ? queryStr.ptr : queryStr.buf;
        for (; cur != end; cur += 0x48) {
            if (*reinterpret_cast<const uint32_t*>(cur + 0x08) != targetStage) {
                continue;
            }
            const char16_t* entryStr = reinterpret_cast<const char16_t*>(cur + 0x18);
            if (*reinterpret_cast<const uint64_t*>(cur + 0x30) >= 8) {
                entryStr = *reinterpret_cast<const char16_t* const*>(cur + 0x18);
            }
            const char16_t* qStr = qStrBase;

            int16_t c1 = 0;
            int16_t c2 = 0;
            for (;;) {
                uint8_t b1 = static_cast<uint8_t>(*entryStr & 0xFFu);
                uint16_t low1 = lowerTbl[b1];

                uint8_t b2 = static_cast<uint8_t>(*qStr & 0xFFu);
                c2 = static_cast<int8_t>(lowerTbl[b2] & 0xFFu);

                if (static_cast<uint8_t>(low1 & 0xFFu) == 0) {
                    c1 = 0;
                    break;
                }
                c1 = static_cast<int8_t>(low1 & 0xFFu);
                ++qStr;
                ++entryStr;
                if (c1 != c2) {
                    break;
                }
            }
            if (c1 == c2) {
                break;
            }
        }
        if (cur != *reinterpret_cast<const uint8_t* const*>(r12 + 0x18)) {
            foundShader = *reinterpret_cast<void* const*>(cur + 0x00);
        }
    }

    if (queryStr.capacity >= 8) {
        uintptr_t* aVtbl = *reinterpret_cast<uintptr_t**>(queryStr.allocator);
        reinterpret_cast<void(*)(void*, void*)>(aVtbl[0x70 / 8])(queryStr.allocator, queryStr.ptr);
    }

    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return foundShader;
}

} // anonymous namespace

// ============================================================================
// 42. sub_21bf0c0 (0x21bf0c0, 495 B) - GXShaderPackageLoader::FindVertexShader (Stage 5)
// ============================================================================
void* sub_21bf0c0(const void* self, const char16_t* prefixWStr, const char16_t* nameWStr) {
    return LookupShaderByStageAndName(self, prefixWStr, nameWStr, 5u);
}

// ============================================================================
// 43. sub_21beed0 (0x21beed0, 495 B) - GXShaderPackageLoader::FindPixelShader (Stage 4)
// ============================================================================
void* sub_21beed0(const void* self, const char16_t* prefixWStr, const char16_t* nameWStr) {
    return LookupShaderByStageAndName(self, prefixWStr, nameWStr, 4u);
}

// ============================================================================
// 44. sub_21bece0 (0x21bece0, 495 B) - GXShaderPackageLoader::FindGeometryShader (Stage 3)
// ============================================================================
void* sub_21bece0(const void* self, const char16_t* prefixWStr, const char16_t* nameWStr) {
    return LookupShaderByStageAndName(self, prefixWStr, nameWStr, 3u);
}

// ============================================================================
// 45. sub_21beaf0 (0x21beaf0, 495 B) - GXShaderPackageLoader::FindHullShader (Stage 2)
// ============================================================================
void* sub_21beaf0(const void* self, const char16_t* prefixWStr, const char16_t* nameWStr) {
    return LookupShaderByStageAndName(self, prefixWStr, nameWStr, 2u);
}

// ============================================================================
// 46. sub_21be900 (0x21be900, 495 B) - GXShaderPackageLoader::FindDomainShader (Stage 1)
// ============================================================================
void* sub_21be900(const void* self, const char16_t* prefixWStr, const char16_t* nameWStr) {
    return LookupShaderByStageAndName(self, prefixWStr, nameWStr, 1u);
}

// ============================================================================
// 47. sub_21be710 (0x21be710, 495 B) - GXShaderPackageLoader::FindComputeShader (Stage 0)
// ============================================================================
void* sub_21be710(const void* self, const char16_t* prefixWStr, const char16_t* nameWStr) {
    return LookupShaderByStageAndName(self, prefixWStr, nameWStr, 0u);
}

// ============================================================================
// 48. sub_21bd870 (0x21bd870, 3739 B) - GXShaderPackageLoader::LoadFromBND4
// ============================================================================
bool sub_21bd870(void* self, const void* bndData, uint64_t bndSize) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    uint8_t* r14Self = static_cast<uint8_t*>(self);

    struct BND4ReaderState {
        void*    allocator; // rbp-0x428
        void*    stream;    // rbp-0x420
        void**   headerPtr; // rbp-0x418
        uint64_t curEntry0; // rbp-0x410
        uint64_t curEntry1; // rbp-0x408
        uint64_t curEntry2; // rbp-0x400
        uint64_t curEntry3; // rbp-0x3f8
        uint64_t curEntry4; // rbp-0x3f0
        uint64_t curEntry5; // rbp-0x3e8
    } bndReader = {};
    bndReader.allocator = *reinterpret_cast<void**>(g_pDefaultHeap_5540688);

    bool success = false;
    if (sub_21c09e0(&bndReader, bndData, bndSize)) {
        success = true;
        if (bndReader.headerPtr != nullptr && *bndReader.headerPtr != nullptr) {
            const uint8_t* bndHdr = static_cast<const uint8_t*>(*bndReader.headerPtr);
            uint32_t fileCount = *reinterpret_cast<const uint32_t*>(bndHdr + 0x0C);
            if (fileCount != 0) {
                {
                    uint8_t* vecBegin = *reinterpret_cast<uint8_t**>(r14Self + 0x10);
                    uint8_t* vecCap   = *reinterpret_cast<uint8_t**>(r14Self + 0x20);
                    size_t curCap = static_cast<size_t>(vecCap - vecBegin) / 0x48;
                    if (curCap < fileCount) {
                        sub_21bf340(r14Self + 0x08, fileCount);
                    }
                }
                for (uint32_t idx = 0; idx < fileCount; ++idx) {
                    if (bndReader.headerPtr == nullptr || *bndReader.headerPtr == nullptr) {
                        continue;
                    }
                    const uint8_t* curHdr = static_cast<const uint8_t*>(*bndReader.headerPtr);
                    if (*reinterpret_cast<const uint32_t*>(curHdr + 0x0C) <= idx) {
                        continue;
                    }
                    sub_20f1920(&bndReader.curEntry0, curHdr, idx);

                    uint8_t* tlsHeap = static_cast<uint8_t*>(g_tlsLinearHeap_53e4568);
                    if (tlsHeap == nullptr) {
                        tlsHeap = static_cast<uint8_t*>(sub_207d250());
                        g_tlsLinearHeap_53e4568 = tlsHeap;
                    }
                    void* linearAllocObj = tlsHeap + 0x28;
                    uint8_t capsBuf[16] = {};
                    uintptr_t* linVtbl = *reinterpret_cast<uintptr_t**>(linearAllocObj);
                    reinterpret_cast<void(*)(void*, void*, uint32_t)>(linVtbl[0x20 / 8])(capsBuf, linearAllocObj, 0);
                    if ((capsBuf[0] & 0x20) == 0) {
                        sub_20b55b0(nullptr, 0x3E, kStr_IncompatibleHeap_4737dc0);
                    }

                    struct LocalDLWString {
                        uint64_t  hdr;
                        union {
                            char16_t  buf[8];
                            char16_t* ptr;
                        };
                        uint64_t  length;
                        uint64_t  capacity;
                        void*     allocator;
                        uint8_t   flag;
                        uint8_t   pad[7];
                    };

                    LocalDLWString entryPathW = {};
                    entryPathW.allocator = linearAllocObj;
                    entryPathW.capacity  = 7;
                    entryPathW.length    = 0;
                    entryPathW.buf[0]    = 0;
                    entryPathW.flag      = 1;

                    if (bndReader.curEntry1 != 0) {
                        sub_20f1c20(&bndReader.curEntry0, &entryPathW);
                    }

                    void* strHeap = g_pStringHeap_5540670;
                    if (strHeap == nullptr) {
                        strHeap = sub_2ba2b10();
                        g_pStringHeap_5540670 = strHeap;
                    }
                    uintptr_t* strHeapVtbl = *reinterpret_cast<uintptr_t**>(strHeap);
                    reinterpret_cast<void(*)(void*, void*, uint32_t)>(strHeapVtbl[0x20 / 8])(capsBuf, strHeap, 0);
                    if ((capsBuf[0] & 0x20) == 0) {
                        sub_20b55b0(nullptr, 0x3E, kStr_IncompatibleHeap_4737dc0);
                    }

                    GXShaderPackageEntry stagingEntry = {};
                    stagingEntry.shaderObj     = nullptr;
                    stagingEntry.wstrAllocator = strHeap;
                    stagingEntry.wstrCapacity  = 7;
                    stagingEntry.wstrLength    = 0;
                    stagingEntry.inlineBuf[0]  = 0;
                    stagingEntry.wstrFlag      = 1;

                    const char16_t* rawPath = (entryPathW.capacity > 7) ? entryPathW.ptr : entryPathW.buf;
                    const char16_t* lastDot   = sub_2bbf258(rawPath, u'.');
                    const char16_t* lastSlash = sub_2bbf258(rawPath, u'/');
                    const char16_t* lastBack  = sub_2bbf258(rawPath, u'\\');

                    const char16_t* extPtr = nullptr;
                    if (lastDot != nullptr && lastDot >= lastSlash && lastDot >= lastBack) {
                        extPtr = lastDot;
                    }

                    uint8_t* tlsHeap2 = static_cast<uint8_t*>(g_tlsLinearHeap_53e4568);
                    if (tlsHeap2 == nullptr) {
                        tlsHeap2 = static_cast<uint8_t*>(sub_207d250());
                        g_tlsLinearHeap_53e4568 = tlsHeap2;
                    }
                    void* linearAllocObj2 = tlsHeap2 + 0x28;
                    uintptr_t* linVtbl2 = *reinterpret_cast<uintptr_t**>(linearAllocObj2);
                    reinterpret_cast<void(*)(void*, void*, uint32_t)>(linVtbl2[0x20 / 8])(capsBuf, linearAllocObj2, 0);
                    if ((capsBuf[0] & 0x20) == 0) {
                        sub_20b55b0(nullptr, 0x3E, kStr_IncompatibleHeap_4737dc0);
                    }

                    LocalDLWString baseNameW = {};
                    baseNameW.allocator = linearAllocObj2;
                    baseNameW.capacity  = 7;
                    baseNameW.length    = 0;
                    baseNameW.buf[0]    = 0;
                    baseNameW.flag      = 1;

                    const char16_t* pathForBase = (entryPathW.capacity > 7) ? entryPathW.ptr : entryPathW.buf;
                    sub_21bc7b0(pathForBase, &baseNameW);

                    if (extPtr != nullptr) {
                        const uint16_t* lowerTbl = sub_2bbec48();
                        auto matchExtCI = [lowerTbl](const char16_t* cand, const char16_t* targetExt) -> bool {
                            size_t i = 0;
                            for (;;) {
                                uint8_t bT = static_cast<uint8_t>(targetExt[i] & 0xFFu);
                                uint16_t lowT = lowerTbl[bT];

                                uint8_t bC = static_cast<uint8_t>(cand[i] & 0xFFu);
                                int16_t lowC = static_cast<int8_t>(lowerTbl[bC] & 0xFFu);

                                if (static_cast<uint8_t>(lowT & 0xFFu) == 0) {
                                    return lowC == 0;
                                }
                                int16_t cT = static_cast<int8_t>(lowT & 0xFFu);
                                ++i;
                                if (cT != lowC) {
                                    return false;
                                }
                            }
                        };

                        struct BND4EntryDataBuffer {
                            uintptr_t* vtable;    // +0x00
                            void*      allocator; // +0x08
                            void*      dataPtr;   // +0x10
                            size_t     dataSize;  // +0x18
                            uint16_t   flags;     // +0x20
                            uint16_t   pad[3];
                        };

                        uint32_t shaderCodeSize = 0;
                        BND4EntryDataBuffer entryBuf = {};
                        bool matchedAny = false;

                        auto initEntryBufAndRead = [&]() {
                            entryBuf.allocator = *reinterpret_cast<void**>(g_pDefaultHeap_5540688);
                            entryBuf.dataPtr   = nullptr;
                            entryBuf.dataSize  = 0;
                            entryBuf.flags     = 0;
                            entryBuf.vtable    = g_vtable_535f9f0;
                            sub_21c0ea0(&bndReader, &entryBuf, 4);
                        };

                        void* shaderHeap = *reinterpret_cast<void**>(g_pShaderHeap_5540680);
                        uintptr_t* shHeapVtbl = *reinterpret_cast<uintptr_t**>(shaderHeap);

                        if (matchExtCI(extPtr, kWStr_ExtPpo_49d282c)) {
                            // .ppo -> Stage 4 (GXPixelShader, 0x18 B)
                            initEntryBufAndRead();
                            uint8_t* shObj = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(shHeapVtbl[0x58 / 8])(shaderHeap, 0x18, 8);
                            if (shObj != nullptr) {
                                *reinterpret_cast<uint32_t*>(shObj + 0x08) = 0;
                                *reinterpret_cast<uint32_t*>(shObj + 0x0C) = 0;
                                *reinterpret_cast<uintptr_t**>(shObj + 0x00) = g_vtable_535f910;
                                *reinterpret_cast<void**>(shObj + 0x10) = nullptr;
                                sub_21a5f70(shObj, entryBuf.dataPtr, entryBuf.dataSize, 0, 0);
                                stagingEntry.shaderObj   = shObj;
                                stagingEntry.shaderStage = 4;
                                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shObj);
                                shaderCodeSize = reinterpret_cast<uint32_t(*)(void*)>(shVtbl[0x10 / 8])(shObj);
                            }
                            matchedAny = true;
                        } else if (matchExtCI(extPtr, kWStr_ExtVpo_49d2836)) {
                            // .vpo -> Stage 5 (GXVertexShader, 0x28 B)
                            initEntryBufAndRead();
                            uint8_t* shObj = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(shHeapVtbl[0x58 / 8])(shaderHeap, 0x28, 8);
                            if (shObj != nullptr) {
                                *reinterpret_cast<uint32_t*>(shObj + 0x08) = 0;
                                *reinterpret_cast<uint32_t*>(shObj + 0x0C) = 0;
                                *reinterpret_cast<uintptr_t**>(shObj + 0x00) = g_vtable_535f940;
                                *reinterpret_cast<void**>(shObj + 0x20) = nullptr;
                                *reinterpret_cast<void**>(shObj + 0x18) = nullptr;
                                *reinterpret_cast<void**>(shObj + 0x10) = nullptr;
                                sub_21a6510(shObj, entryBuf.dataPtr, entryBuf.dataSize, 0, 0);
                                stagingEntry.shaderObj   = shObj;
                                stagingEntry.shaderStage = 5;
                                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shObj);
                                shaderCodeSize = reinterpret_cast<uint32_t(*)(void*)>(shVtbl[0x10 / 8])(shObj);
                            }
                            matchedAny = true;
                        } else if (matchExtCI(extPtr, kWStr_ExtGpo_49d2854)) {
                            // .gpo -> Stage 3 (GXGeometryShader, 0x28 B)
                            initEntryBufAndRead();
                            uint8_t* shObj = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(shHeapVtbl[0x58 / 8])(shaderHeap, 0x28, 8);
                            if (shObj != nullptr) {
                                *reinterpret_cast<uint32_t*>(shObj + 0x08) = 0;
                                *reinterpret_cast<uint32_t*>(shObj + 0x0C) = 0;
                                *reinterpret_cast<uintptr_t**>(shObj + 0x00) = g_vtable_535f8e0;
                                *reinterpret_cast<void**>(shObj + 0x20) = nullptr;
                                *reinterpret_cast<void**>(shObj + 0x18) = nullptr;
                                *reinterpret_cast<void**>(shObj + 0x10) = nullptr;
                                sub_21a57f0(shObj, entryBuf.dataPtr, entryBuf.dataSize, 0, 0);
                                stagingEntry.shaderObj   = shObj;
                                stagingEntry.shaderStage = 3;
                                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shObj);
                                shaderCodeSize = reinterpret_cast<uint32_t(*)(void*)>(shVtbl[0x10 / 8])(shObj);
                            }
                            matchedAny = true;
                        } else if (matchExtCI(extPtr, kWStr_ExtDpo_49d284a)) {
                            // .dpo -> Stage 1 (GXDomainShader, 0x18 B)
                            initEntryBufAndRead();
                            uint8_t* shObj = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(shHeapVtbl[0x58 / 8])(shaderHeap, 0x18, 8);
                            if (shObj != nullptr) {
                                *reinterpret_cast<uint32_t*>(shObj + 0x08) = 0;
                                *reinterpret_cast<uint32_t*>(shObj + 0x0C) = 0;
                                *reinterpret_cast<uintptr_t**>(shObj + 0x00) = g_vtable_535f880;
                                *reinterpret_cast<void**>(shObj + 0x10) = nullptr;
                                sub_21a4cc0(shObj, entryBuf.dataPtr, entryBuf.dataSize, 0, 0);
                                stagingEntry.shaderObj   = shObj;
                                stagingEntry.shaderStage = 1;
                                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shObj);
                                shaderCodeSize = reinterpret_cast<uint32_t(*)(void*)>(shVtbl[0x10 / 8])(shObj);
                            }
                            matchedAny = true;
                        } else if (matchExtCI(extPtr, kWStr_ExtHpo_49d2840)) {
                            // .hpo -> Stage 2 (GXHullShader, 0x18 B)
                            initEntryBufAndRead();
                            uint8_t* shObj = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(shHeapVtbl[0x58 / 8])(shaderHeap, 0x18, 8);
                            if (shObj != nullptr) {
                                *reinterpret_cast<uint32_t*>(shObj + 0x08) = 0;
                                *reinterpret_cast<uint32_t*>(shObj + 0x0C) = 0;
                                *reinterpret_cast<uintptr_t**>(shObj + 0x00) = g_vtable_535f8b0;
                                *reinterpret_cast<void**>(shObj + 0x10) = nullptr;
                                sub_21a51a0(shObj, entryBuf.dataPtr, entryBuf.dataSize, 0, 0);
                                stagingEntry.shaderObj   = shObj;
                                stagingEntry.shaderStage = 2;
                                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shObj);
                                shaderCodeSize = reinterpret_cast<uint32_t(*)(void*)>(shVtbl[0x10 / 8])(shObj);
                            }
                            matchedAny = true;
                        } else if (matchExtCI(extPtr, kWStr_ExtCpo_49d2862)) {
                            // .cpo -> Stage 0 (GXComputeShader, 0x18 B)
                            initEntryBufAndRead();
                            uint8_t* shObj = reinterpret_cast<uint8_t*(*)(void*, size_t, uint32_t)>(shHeapVtbl[0x58 / 8])(shaderHeap, 0x18, 8);
                            if (shObj != nullptr) {
                                *reinterpret_cast<uint32_t*>(shObj + 0x08) = 0;
                                *reinterpret_cast<uint32_t*>(shObj + 0x0C) = 0;
                                *reinterpret_cast<uintptr_t**>(shObj + 0x00) = g_vtable_535f850;
                                *reinterpret_cast<void**>(shObj + 0x10) = nullptr;
                                sub_21a47e0(shObj, entryBuf.dataPtr, entryBuf.dataSize, 0, 0);
                                stagingEntry.shaderObj   = shObj;
                                stagingEntry.shaderStage = 0;
                                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shObj);
                                shaderCodeSize = reinterpret_cast<uint32_t(*)(void*)>(shVtbl[0x10 / 8])(shObj);
                            }
                            matchedAny = true;
                        }

                        if (matchedAny) {
                            entryBuf.vtable = g_vtable_535fa10;
                            if ((entryBuf.flags & 0xFFu) != 0 && entryBuf.dataPtr != nullptr) {
                                uintptr_t* bVtbl = *reinterpret_cast<uintptr_t**>(entryBuf.allocator);
                                if (entryBuf.flags < 0x0100u) {
                                    reinterpret_cast<void(*)(void*, void*)>(bVtbl[0x70 / 8])(entryBuf.allocator, entryBuf.dataPtr);
                                } else {
                                    reinterpret_cast<void(*)(void*, void*)>(bVtbl[0xA0 / 8])(entryBuf.allocator, entryBuf.dataPtr);
                                }
                            }
                        }

                        if (stagingEntry.shaderObj != nullptr) {
                            const char16_t* srcBase = (baseNameW.capacity > 7) ? baseNameW.ptr : baseNameW.buf;
                            char16_t normalizedBuf[264] = {};
                            for (size_t cIdx = 0; cIdx <= 0x103; ++cIdx) {
                                char16_t ch = srcBase[cIdx];
                                normalizedBuf[cIdx] = (ch == u'/' || ch == u'\\') ? u'_' : ch;
                                if (srcBase[cIdx] == 0) {
                                    break;
                                }
                            }
                            size_t normLen = (normalizedBuf[0] != 0) ? sub_2bbf228(normalizedBuf) : 0;
                            sub_262a720(&stagingEntry.wstrBoxHdr, normalizedBuf, normLen);

                            uint8_t* vecBegin = *reinterpret_cast<uint8_t**>(r14Self + 0x10);
                            uint8_t* vecEnd   = *reinterpret_cast<uint8_t**>(r14Self + 0x18);
                            uint8_t* vecCap   = *reinterpret_cast<uint8_t**>(r14Self + 0x20);
                            uint8_t* stageAddr = reinterpret_cast<uint8_t*>(&stagingEntry);

                            if (vecEnd > stageAddr && vecBegin <= stageAddr) {
                                size_t aliasIdx = static_cast<size_t>(stageAddr - vecBegin) / 0x48;
                                if (vecEnd == vecCap) {
                                    size_t curLen = static_cast<size_t>(vecEnd - vecBegin) / 0x48;
                                    size_t maxCap = 0x038E38E38E38E38EULL;
                                    size_t grow   = curLen + (curLen >> 1);
                                    size_t newCap = (maxCap - (curLen >> 1) >= curLen) ? grow : 0;
                                    if (newCap < curLen + 1) {
                                        newCap = curLen + 1;
                                    }
                                    sub_21bf340(r14Self + 0x08, newCap);
                                    vecBegin = *reinterpret_cast<uint8_t**>(r14Self + 0x10);
                                    vecEnd   = *reinterpret_cast<uint8_t**>(r14Self + 0x18);
                                }
                                if (vecEnd != nullptr) {
                                    const uint8_t* srcElem = vecBegin + aliasIdx * 0x48;
                                    *reinterpret_cast<uint32_t*>(vecEnd + 0x08) = *reinterpret_cast<const uint32_t*>(srcElem + 0x08);
                                    *reinterpret_cast<void**>(vecEnd + 0x00)     = *reinterpret_cast<void* const*>(srcElem + 0x00);
                                    *reinterpret_cast<void**>(vecEnd + 0x38)     = *reinterpret_cast<void* const*>(srcElem + 0x38);
                                    *reinterpret_cast<uint64_t*>(vecEnd + 0x30)  = 7;
                                    *reinterpret_cast<uint64_t*>(vecEnd + 0x28)  = 0;
                                    *reinterpret_cast<uint16_t*>(vecEnd + 0x18)  = 0;
                                    sub_26279a0(vecEnd + 0x10, srcElem + 0x10, 0, ~static_cast<size_t>(0));
                                    vecEnd[0x40] = srcElem[0x40];
                                }
                            } else {
                                if (vecEnd == vecCap) {
                                    size_t curLen = static_cast<size_t>(vecEnd - vecBegin) / 0x48;
                                    size_t maxCap = 0x038E38E38E38E38EULL;
                                    size_t grow   = curLen + (curLen >> 1);
                                    size_t newCap = (maxCap - (curLen >> 1) >= curLen) ? grow : 0;
                                    if (newCap < curLen + 1) {
                                        newCap = curLen + 1;
                                    }
                                    sub_21bf340(r14Self + 0x08, newCap);
                                    vecEnd = *reinterpret_cast<uint8_t**>(r14Self + 0x18);
                                }
                                if (vecEnd != nullptr) {
                                    *reinterpret_cast<uint32_t*>(vecEnd + 0x08) = stagingEntry.shaderStage;
                                    *reinterpret_cast<void**>(vecEnd + 0x00)     = stagingEntry.shaderObj;
                                    *reinterpret_cast<void**>(vecEnd + 0x38)     = stagingEntry.wstrAllocator;
                                    *reinterpret_cast<uint64_t*>(vecEnd + 0x30)  = 7;
                                    *reinterpret_cast<uint64_t*>(vecEnd + 0x28)  = 0;
                                    *reinterpret_cast<uint16_t*>(vecEnd + 0x18)  = 0;
                                    sub_26279a0(vecEnd + 0x10, &stagingEntry.wstrBoxHdr, 0, ~static_cast<size_t>(0));
                                    vecEnd[0x40] = stagingEntry.wstrFlag;
                                }
                            }
                            *reinterpret_cast<uint8_t**>(r14Self + 0x18) += 0x48;
                            *reinterpret_cast<uint32_t*>(r14Self + 0x30) += shaderCodeSize;
                        }
                    }

                    if (baseNameW.capacity >= 8) {
                        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(baseNameW.allocator);
                        reinterpret_cast<void(*)(void*, void*)>(vtbl[0x70 / 8])(baseNameW.allocator, baseNameW.ptr);
                    }
                    if (stagingEntry.wstrCapacity >= 8) {
                        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(stagingEntry.wstrAllocator);
                        reinterpret_cast<void(*)(void*, void*)>(vtbl[0x70 / 8])(stagingEntry.wstrAllocator, stagingEntry.heapPtr);
                    }
                    if (entryPathW.capacity >= 8) {
                        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(entryPathW.allocator);
                        reinterpret_cast<void(*)(void*, void*)>(vtbl[0x70 / 8])(entryPathW.allocator, entryPathW.ptr);
                    }
                }
            }
        }
    }

    sub_21c08b0(&bndReader);
    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
    return success;
}

// ============================================================================
// 49. sub_21bd770 (0x21bd770, 248 B) - GXShaderPackageLoader::ClearEntries
// ============================================================================
void sub_21bd770(void* self) {
    uint8_t* r14 = static_cast<uint8_t*>(self);
    uint8_t* cur = *reinterpret_cast<uint8_t**>(r14 + 0x10);
    uint8_t* end = *reinterpret_cast<uint8_t**>(r14 + 0x18);
    if (cur == end) {
        return;
    }

    uint8_t* r13 = cur;
    for (; r13 != *reinterpret_cast<uint8_t**>(r14 + 0x18); r13 += 0x48) {
        uint32_t stage = *reinterpret_cast<uint32_t*>(r13 + 0x08);
        // Jump table 0x21bd850 cases 0..5 all dispatch to 0x21bd7b6
        if (stage <= 5u) {
            void* r12Shader = *reinterpret_cast<void**>(r13 + 0x00);
            if (r12Shader != nullptr) {
                void* r15Heap = sub_207b720(r12Shader);
                uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(r12Shader);
                reinterpret_cast<void(*)(void*)>(shVtbl[0])(r12Shader);
                uintptr_t* hpVtbl = *reinterpret_cast<uintptr_t**>(r15Heap);
                reinterpret_cast<void(*)(void*, void*)>(hpVtbl[0x70 / 8])(r15Heap, r12Shader);
                *reinterpret_cast<void**>(r13 + 0x00) = nullptr;
            }
        }
    }

    uint8_t* beginPtr = *reinterpret_cast<uint8_t**>(r14 + 0x10);
    uint8_t* afterLast = r13;
    if (afterLast != beginPtr) {
        for (uint8_t* p = beginPtr; p != afterLast; p += 0x48) {
            if (*reinterpret_cast<uint64_t*>(p + 0x30) >= 8) {
                void* strBuf = *reinterpret_cast<void**>(p + 0x18);
                void* strAlloc = *reinterpret_cast<void**>(p + 0x38);
                uintptr_t* strVtbl = *reinterpret_cast<uintptr_t**>(strAlloc);
                reinterpret_cast<void(*)(void*, void*)>(strVtbl[0x70 / 8])(strAlloc, strBuf);
            }
            *reinterpret_cast<uint64_t*>(p + 0x30) = 7;
            *reinterpret_cast<uint64_t*>(p + 0x28) = 0;
            *reinterpret_cast<uint16_t*>(p + 0x18) = 0;
        }
        beginPtr = *reinterpret_cast<uint8_t**>(r14 + 0x10);
    }
    *reinterpret_cast<uint8_t**>(r14 + 0x18) = beginPtr;
}

// ============================================================================
// 50. sub_21bd6c0 (0x21bd6c0, 164 B) - GXShaderPackageLoader::~GXShaderPackageLoader
// ============================================================================
void sub_21bd6c0(void* self) {
    uint8_t* r14 = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(r14) = g_vtable_53b6d80;
    sub_21bd770(r14);

    uint8_t* rbx = *reinterpret_cast<uint8_t**>(r14 + 0x10);
    if (rbx != nullptr) {
        void* vecAlloc = *reinterpret_cast<void**>(r14 + 0x28);
        uintptr_t* vecVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
        reinterpret_cast<void(*)(void*, void*)>(vecVtbl[0x70 / 8])(vecAlloc, rbx);
        *reinterpret_cast<void**>(r14 + 0x20) = nullptr;
        *reinterpret_cast<void**>(r14 + 0x18) = nullptr;
        *reinterpret_cast<void**>(r14 + 0x10) = nullptr;
    }
}

// ============================================================================
// 51. sub_21bd630 (0x21bd630, 135 B) - GXShaderPackageLoader::DeletingDtor
// ============================================================================
void sub_21bd630(void* self) {
    uint8_t* r14 = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(r14) = g_vtable_53b6d80;
    sub_21bd770(r14);

    uint8_t* rbx = *reinterpret_cast<uint8_t**>(r14 + 0x10);
    if (rbx != nullptr) {
        void* vecAlloc = *reinterpret_cast<void**>(r14 + 0x28);
        uintptr_t* vecVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
        reinterpret_cast<void(*)(void*, void*)>(vecVtbl[0x70 / 8])(vecAlloc, rbx);
    }
    sub_2bbe588(r14);
}

// ============================================================================
// 52. sub_21bd580 (0x21bd580, 175 B) - GXShaderPackageLoader::GXShaderPackageLoader
// ============================================================================
void sub_21bd580(void* self) {
    uint64_t cookie = *g_pStackCookie_53e3be0;
    uint8_t* rbx = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t**>(rbx + 0x00) = g_vtable_53b6d80;

    void* r14Heap = g_pStringHeap_5540670;
    if (r14Heap == nullptr) {
        r14Heap = sub_2ba2b10();
        g_pStringHeap_5540670 = r14Heap;
    }

    uint8_t capsBuf[16] = {};
    uintptr_t* heapVtbl = *reinterpret_cast<uintptr_t**>(r14Heap);
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(heapVtbl[0x20 / 8])(capsBuf, r14Heap, 0);
    if ((capsBuf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3E, kStr_IncompatibleHeap_4737dc0);
    }

    *reinterpret_cast<void**>(rbx + 0x20) = nullptr;
    *reinterpret_cast<void**>(rbx + 0x18) = nullptr;
    *reinterpret_cast<void**>(rbx + 0x10) = nullptr;
    *reinterpret_cast<void**>(rbx + 0x28) = r14Heap;
    *reinterpret_cast<uint32_t*>(rbx + 0x30) = 0;

    if (*g_pStackCookie_53e3be0 != cookie) {
        sub_2bbe4f8();
    }
}

// ============================================================================
// 53. sub_21bd480 (0x21bd480, 251 B) - GXDeferredReleaseQueue::ReallocateVector
// ============================================================================
void sub_21bd480(void* vecSelf, size_t newCapacity) {
    uint8_t* r12 = static_cast<uint8_t*>(vecSelf);
    void* allocator = *reinterpret_cast<void**>(r12 + 0x20);
    uintptr_t* allocVtbl = *reinterpret_cast<uintptr_t**>(allocator);
    void** r15 = reinterpret_cast<void**(*)(void*, size_t, uint32_t)>(allocVtbl[0x58 / 8])(allocator, newCapacity * sizeof(void*), 0x08);
    if (r15 == nullptr) {
        if (g_tlsOomMode_53e4318 != 0) {
            sub_2bbe978(&g_tlsOomExc_53e4320, 1);
        }
        sub_20b55b0(nullptr, 0x109, kStr_TransactionFailed_4737d80);
    }

    void** rbx = *reinterpret_cast<void***>(r12 + 0x08);
    void** end = *reinterpret_cast<void***>(r12 + 0x10);
    void** oldBegin = rbx;
    if (rbx != end) {
        void** dst = r15;
        for (; rbx != end; ++rbx, ++dst) {
            if (dst != nullptr) {
                *dst = *rbx;
            }
        }
        oldBegin = *reinterpret_cast<void***>(r12 + 0x08);
        rbx      = *reinterpret_cast<void***>(r12 + 0x10);
    }
    ptrdiff_t count = rbx - oldBegin;
    if (oldBegin != nullptr) {
        void* curAlloc = *reinterpret_cast<void**>(r12 + 0x20);
        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(curAlloc);
        reinterpret_cast<void(*)(void*, void*)>(vtbl[0x70 / 8])(curAlloc, oldBegin);
    }

    *reinterpret_cast<void***>(r12 + 0x18) = r15 + newCapacity;
    *reinterpret_cast<void***>(r12 + 0x10) = r15 + count;
    *reinterpret_cast<void***>(r12 + 0x08) = r15;
}

// ============================================================================
// 54. sub_21bd250 (0x21bd250, 415 B) - GXDeferredReleaseQueue::ProcessFrameTick
// ============================================================================
void sub_21bd250(void* self, void* context) {
    uint8_t* r12 = static_cast<uint8_t*>(self);
    if (*reinterpret_cast<void**>(r12 + 0x10) != *reinterpret_cast<void**>(r12 + 0x08)) {
        void* lockObj = r12 + 0x28;
        uintptr_t* lockVtbl = *reinterpret_cast<uintptr_t**>(lockObj);
        reinterpret_cast<void(*)(void*, uint32_t)>(lockVtbl[0x18 / 8])(lockObj, 0xFFFFFFFFu);

        void** rbx = *reinterpret_cast<void***>(r12 + 0x08);
        if (rbx != *reinterpret_cast<void***>(r12 + 0x10)) {
            void** r13 = rbx + 1;
            for (;;) {
                sub_e48400(*rbx, context);
                uint32_t* r15Item = static_cast<uint32_t*>(*rbx);
                if (r15Item == nullptr || r15Item[0] >= 3u) {
                    if (r15Item != nullptr) {
                        void* shHeap = *reinterpret_cast<void**>(g_pShaderHeap_5540680);
                        sub_e483a0(r15Item);
                        uintptr_t* shVtbl = *reinterpret_cast<uintptr_t**>(shHeap);
                        reinterpret_cast<void(*)(void*, void*)>(shVtbl[0x70 / 8])(shHeap, r15Item);
                    }
                    void** curEnd = *reinterpret_cast<void***>(r12 + 0x10);
                    size_t bytesToMove = static_cast<size_t>(reinterpret_cast<uint8_t*>(curEnd) - reinterpret_cast<uint8_t*>(r13));
                    sub_2bbe658(rbx, r13, bytesToMove);
                    void** newEnd = *reinterpret_cast<void***>(r12 + 0x10) - 1;
                    *reinterpret_cast<void***>(r12 + 0x10) = newEnd;
                    if (rbx == newEnd) {
                        break;
                    }
                } else {
                    rbx = r13;
                    if (r13 == *reinterpret_cast<void***>(r12 + 0x10)) {
                        break;
                    }
                    r13 = rbx + 1;
                }
            }
        }

        lockVtbl = *reinterpret_cast<uintptr_t**>(lockObj);
        reinterpret_cast<void(*)(void*)>(lockVtbl[0x28 / 8])(lockObj);
    }

    *reinterpret_cast<uint64_t*>(r12 + 0x58) = 0;
    uint32_t targetCount = *reinterpret_cast<uint32_t*>(r12 + 0x40);
    if (targetCount != 0) {
        uint32_t limit = (r12[0x4C] != 0) ? (targetCount * targetCount) : 1u;
        uint32_t curStep = *reinterpret_cast<uint32_t*>(r12 + 0x44) + 1u;
        *reinterpret_cast<uint32_t*>(r12 + 0x44) = curStep;
        if (curStep == limit) {
            uint8_t* stateBlock = r12 + 0x40;
            stateBlock[0x0C] = 0;
            *reinterpret_cast<uint32_t*>(stateBlock + 0x08) = 0;
            *reinterpret_cast<uint64_t*>(stateBlock + 0x00) = 0;

            uint8_t* r15Sys = *reinterpret_cast<uint8_t**>(g_pGXContextSys_55406c8);
            uint8_t* listenerVec = *reinterpret_cast<uint8_t**>(r15Sys + 0x5F8);
            if (listenerVec != nullptr) {
                void** curListener = *reinterpret_cast<void***>(listenerVec + 0x08);
                void** endListener = *reinterpret_cast<void***>(listenerVec + 0x10);
                if (curListener != endListener) {
                    const uint8_t* dispInfo = *reinterpret_cast<const uint8_t* const*>(r15Sys + 0x90);
                    uint32_t argWidth  = *reinterpret_cast<const uint32_t*>(dispInfo + 0x80);
                    uint32_t argHeight = *reinterpret_cast<const uint32_t*>(dispInfo + 0x84);
                    for (; curListener != *reinterpret_cast<void***>(*reinterpret_cast<uint8_t**>(r15Sys + 0x5F8) + 0x10); ++curListener) {
                        void* listenerObj = *curListener;
                        uintptr_t* lVtbl = *reinterpret_cast<uintptr_t**>(listenerObj);
                        reinterpret_cast<void(*)(void*, uint32_t, uint32_t)>(lVtbl[0x18 / 8])(listenerObj, argWidth, argHeight);
                    }
                }
            }
        }
    }
}

// ============================================================================
// 55. sub_21bd240 (0x21bd240, 4 B) - GXDeferredReleaseQueue::GetSquareCountFlag
// ============================================================================
uint8_t sub_21bd240(const void* self) {
    return static_cast<const uint8_t*>(self)[0x4C];
}

} // extern "C"

} // namespace Bloodborne::SceneRenderer
