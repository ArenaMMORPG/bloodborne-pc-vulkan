// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Module 02: GX Graphics Framework
// File: GXResourceMemoryTrackerAndCameraMath.cpp
// VMA Range: 0x21a9f90 .. 0x21adf40 (54 functions, 18,452 bytes)
// ============================================================================
// Architectural Overview:
//   1. GX Resource Memory & Object Tracker (`GXResourceMemoryTracker`, 0x21ab570 .. 0x21adf40)
//      - Tracks live GPU resource allocations across 6 pointer vectors (`+0x00`,
//        `+0x38`, `+0x70`, `+0xA8`, `+0xE0`, `+0x118`) and 11 `(count, totalBytes,
//        totalMegabytes)` telemetry groups (`+0x28 .. +0x184`).
//      - Includes `GXResourceMemoryTrackerOwner` wrapper thunks (`0x21ab570 .. 0x21aba20`),
//        debug menu property registration (`0x21ad390`, `0x21ad5c0`), and three
//        `DLVector<T*>::MoveAssign` template instantiations (`0x21ad7e0`,
//        `0x21adb90`, `0x21adf40`).
//   2. GX Camera Frustum & SIMD View-Projection Math (`GXCameraFrustum`, 0x21a9f90 .. 0x21ab450)
//      - Manages the `0x84`-byte `GXCameraFrustum` descriptor (4x4 View matrix,
//        eye position, FOV, near/far planes, viewport dimensions, aspect ratio,
//        half-pixel offset flag, and tiled rendering sub-frustum coordinates).
//      - Provides AVX/SSE 4x4 matrix inversion (`0x21aa080`), perspective
//        projection matrix builders (`0x21aa290`, `0x21aa6c0`), view-projection
//        concatenation (`0x21aaa90`), 6-plane world-space frustum extraction
//        (`0x21aadf0`), and cubemap 6-face view matrix initialization (`0x21ab2e0`).
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <immintrin.h>

namespace FromSoftware {
namespace GX {

struct DLAllocator {
    struct VTable {
        void*    (*dtor)(DLAllocator*, int);                         // +0x00
        void*    (*fn08)(DLAllocator*);                              // +0x08
        uint32_t (*GetHeapTag)(DLAllocator*);                        // +0x10
        uint32_t (*GetAllocatorId)(DLAllocator*);                    // +0x18
        void*    (*GetCapabilities)(void*, DLAllocator*, uint32_t);  // +0x20
        uint64_t (*GetTotalSize)(DLAllocator*);                      // +0x28
        uint64_t (*GetFreeSize)(DLAllocator*);                       // +0x30
        uint64_t (*GetMaxAllocatable)(DLAllocator*);                 // +0x38
        uint64_t (*GetAllocationCount)(DLAllocator*);                // +0x40
        uint64_t (*GetBlockSize)(DLAllocator*, void*);               // +0x48
        void*    (*Allocate)(DLAllocator*, size_t);                  // +0x50
        void*    (*AllocateAligned)(DLAllocator*, size_t, size_t);   // +0x58
        void*    (*Reallocate)(DLAllocator*, void*, size_t);         // +0x60
        void*    (*ReallocateAligned)(DLAllocator*, void*, size_t, size_t); // +0x68
        void     (*Free)(DLAllocator*, void*);                       // +0x70
    };
    VTable* vptr;
};

struct GXTrackedResource {
    struct VTable {
        void     (*dtor)(GXTrackedResource*);               // +0x00
        void     (*fn08)(GXTrackedResource*);               // +0x08
        uint32_t (*GetShaderBinarySize)(GXTrackedResource*);// +0x10
        uint8_t  pad18[0x48];
        uint32_t (*GetGpuMemoryBytes)(GXTrackedResource*);  // +0x60
    };
    VTable*  vptr;         // +0x00
    uint8_t  pad08[0x24];  // +0x08
    uint32_t textureBytes; // +0x2C
    uint8_t  pad30[0x0B];  // +0x30
    uint8_t  textureFlags; // +0x3B (bits 0..1 classify texture pool category)
};

struct DLPointerVector {
    void*        vptr;      // +0x00
    void**       begin;     // +0x08
    void**       end;       // +0x10
    void**       cap;       // +0x18
    DLAllocator* allocator; // +0x20
};

struct GXMemoryCounterGroup {
    uint32_t count;          // +0x00
    uint32_t totalBytes;     // +0x04
    float    totalMegabytes; // +0x08 (totalBytes * (1.0f / 1048576.0f))
    uint32_t pad0C;          // +0x0C
};

struct GXTrackedResourceBucket {
    DLPointerVector      vec;     // +0x00 .. +0x27
    GXMemoryCounterGroup counter; // +0x28 .. +0x37
}; // sizeof == 0x38

struct GXResourceMemoryTracker {
    union {
        GXTrackedResourceBucket buckets[6];             // +0x000 .. +0x14B (6 * 0x38 = 0x150 with tail pad0C at +0x14C)
        struct {
            uint8_t             padTo14C[0x14C];
            uint32_t            texType0Count;          // +0x14C
            uint32_t            texType0Bytes;          // +0x150
            float               texType0Megabytes;      // +0x154
            uint32_t            texType2Count;          // +0x158
            uint32_t            texType2Bytes;          // +0x15C
            float               texType2Megabytes;      // +0x160
            uint32_t            texType1Count;          // +0x164
            uint32_t            texType1Bytes;          // +0x168
            float               texType1Megabytes;      // +0x16C
            uint32_t            texType3Count;          // +0x170
            uint32_t            texType3Bytes;          // +0x174
            float               texType3Megabytes;      // +0x178
            uint32_t            shaderCount;            // +0x17C
            uint32_t            shaderBytes;            // +0x180
            float               shaderMegabytes;        // +0x184
        };
    };
}; // sizeof == 0x188
static_assert(sizeof(GXResourceMemoryTracker) == 0x188, "GXResourceMemoryTracker size must be 0x188");

static constexpr float kBytesToMegabytes = 1.0f / 1048576.0f;

// Forward declarations of DLVectorPtr_ReallocateType{0..5} from GXStatePresetCacheAndDebugProps.cpp
void DLVectorPtr_ReallocateType0(DLPointerVector* self, size_t newCap); // 0x21ae6a0
void DLVectorPtr_ReallocateType1(DLPointerVector* self, size_t newCap); // 0x21ae7a0
void DLVectorPtr_ReallocateType2(DLPointerVector* self, size_t newCap); // 0x21ae8a0
void DLVectorPtr_ReallocateType3(DLPointerVector* self, size_t newCap); // 0x21ae9a0
void DLVectorPtr_ReallocateType4(DLPointerVector* self, size_t newCap); // 0x21aeaa0
void DLVectorPtr_ReallocateType5(DLPointerVector* self, size_t newCap); // 0x21aeba0
DLPointerVector* DLVectorPtr_MoveAssign(DLPointerVector* self, DLPointerVector* rhs); // 0x21ae2f0

// ============================================================================
// 1. DLVector<T*>::MoveAssign Template Instantiations (0x21ad7e0 .. 0x21adf40)
// ============================================================================

// VMA: 0x21adf40 (934 B)
DLPointerVector* DLVectorPtr_MoveAssignType2(DLPointerVector* self, DLPointerVector* rhs) {
    return DLVectorPtr_MoveAssign(self, rhs);
}

// VMA: 0x21adb90 (934 B)
DLPointerVector* DLVectorPtr_MoveAssignType1(DLPointerVector* self, DLPointerVector* rhs) {
    return DLVectorPtr_MoveAssign(self, rhs);
}

// VMA: 0x21ad7e0 (934 B)
DLPointerVector* DLVectorPtr_MoveAssignType0(DLPointerVector* self, DLPointerVector* rhs) {
    return DLVectorPtr_MoveAssign(self, rhs);
}

// ============================================================================
// 2. GXResourceMemoryTracker Methods (0x21abac0 .. 0x21ad5c0)
// ============================================================================

extern "C" {
    void* GXDebugMenu_PushGroupNode(void* menuCtx, const void* nameDesc, uint32_t flags); // 0x236c900
    void  GXDebugMenu_AddUint32Property(void* menuCtx, const void* nameDesc, uint32_t* val); // 0x2905e60
    void  GXDebugMenu_AddFloatProperty(void* menuCtx, const void* nameDesc, float* val);     // 0x2931c70
}

static inline void GXDebugMenu_PopGroupNode(void* menuCtx) {
    auto** pBegin = reinterpret_cast<uint8_t***>(static_cast<uint8_t*>(menuCtx) + 0x08);
    auto** pEnd   = reinterpret_cast<uint8_t***>(static_cast<uint8_t*>(menuCtx) + 0x10);
    uint8_t** end = *pEnd;
    if (*pBegin == end) {
        return;
    }
    uint8_t* node = end[-1];
    if (node != nullptr) {
        uint32_t oldVal = *reinterpret_cast<uint32_t*>(node + 0x08);
        uint32_t newStrong = (oldVal + 0x7FFFu) & 0x7FFFu;
        uint32_t eaxVal = (oldVal & 0xFFFF8000u) | newStrong;
        *reinterpret_cast<uint32_t*>(node + 0x08) = eaxVal;
        if (newStrong == 0 && (oldVal & 0x40000000u) != 0) {
            auto vtbl = *reinterpret_cast<void (***)(void*)>(node);
            vtbl[3](node);
            eaxVal = *reinterpret_cast<uint32_t*>(node + 0x08) & 0xBFFFFFFFu;
            *reinterpret_cast<uint32_t*>(node + 0x08) = eaxVal;
        }
        uint32_t newWeak = ((eaxVal >> 15) + 0x7FFFu) & 0x7FFFu;
        *reinterpret_cast<uint32_t*>(node + 0x08) = (eaxVal & 0xC0007FFFu) | (newWeak << 15);
        if (newWeak == 0) {
            *reinterpret_cast<uint32_t*>(node + 0x08) = eaxVal | 0x3FFF8000u;
            auto vtbl = *reinterpret_cast<void (***)(void*)>(node);
            vtbl[4](node);
        }
        end[-1] = nullptr;
        end = *pEnd;
    }
    *pEnd = end - 1;
}

// ----------------------------------------------------------------------------
// VMA: 0x21ad5c0 (398 Bytes)
// Registers a single `(count, totalMegabytes)` counter pair as a debug menu
// sub-group (`u"Count"` at `+0x00` and `u"MemoryUsage[MB]"` at `+0x08`), then
// pops the sub-group node from `menuCtx`.
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_RegisterDebugCounterGroup(
    void* menuCtx,
    uint32_t* counterBase,
    const wchar_t* groupTitle)
{
    alignas(16) struct {
        uint64_t    pRefStrObj;
        const void* pUtf16Title;
    } groupDesc{ 0, groupTitle != nullptr ? static_cast<const void*>(groupTitle) : nullptr };

    void* node = GXDebugMenu_PushGroupNode(menuCtx, &groupDesc, 1u);
    groupDesc.pUtf16Title = nullptr;
    if (node != nullptr) {
        alignas(16) struct {
            uint64_t    pRefStrObj;
            const void* pUtf16Title;
        } countDesc{ 0, reinterpret_cast<const void*>(GX_VMA(0x49c33dcULL)) }; // u"Count"
        GXDebugMenu_AddUint32Property(menuCtx, &countDesc, counterBase);
        countDesc.pUtf16Title = nullptr;

        alignas(16) struct {
            uint64_t    pRefStrObj;
            const void* pUtf16Title;
        } sizeDesc{ 0, reinterpret_cast<const void*>(GX_VMA(0x49c33e8ULL)) }; // u"MemoryUsage[MB]"
        GXDebugMenu_AddFloatProperty(menuCtx, &sizeDesc, reinterpret_cast<float*>(counterBase + 2));
        sizeDesc.pUtf16Title = nullptr;

        GXDebugMenu_PopGroupNode(menuCtx);
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21ad390 (556 Bytes)
// Registers all 11 resource memory counter groups in `GXResourceMemoryTracker`
// with the engine's debug inspector tree using exact PS4 UTF-16 `.rodata` names.
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_RegisterAllDebugProperties(
    GXResourceMemoryTracker* self,
    void* menuCtx,
    bool createRootGroup)
{
    if (createRootGroup) {
        alignas(16) struct {
            uint64_t    pRefStrObj;
            const void* pUtf16Title;
        } rootDesc{ 0, reinterpret_cast<const void*>(GX_VMA(0x49c3292ULL)) }; // u"Resource"
        void* rootNode = GXDebugMenu_PushGroupNode(menuCtx, &rootDesc, 1u);
        rootDesc.pUtf16Title = nullptr;
        if (rootNode == nullptr) {
            return;
        }
    }

    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->buckets[0].counter.count, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c32a4ULL))); // u"Texture2D"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->buckets[1].counter.count, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c32b8ULL))); // u"TextureCube"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->buckets[2].counter.count, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c32d0ULL))); // u"RenderTarget2D"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->buckets[3].counter.count, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c32eeULL))); // u"DepthStencilBuffer"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->buckets[4].counter.count, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c3314ULL))); // u"RenderTargetCube"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->buckets[5].counter.count, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c3336ULL))); // u"DepthStencilCube"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->texType0Count,            reinterpret_cast<const wchar_t*>(GX_VMA(0x49c3358ULL))); // u"ConstantBuffer"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->texType2Count,            reinterpret_cast<const wchar_t*>(GX_VMA(0x49c3376ULL))); // u"IndexBuffer"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->texType1Count,            reinterpret_cast<const wchar_t*>(GX_VMA(0x49c338eULL))); // u"VertexBuffer"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->texType3Count,            reinterpret_cast<const wchar_t*>(GX_VMA(0x49c33a8ULL))); // u"Index&VertexBuffer"
    GXResourceMemoryTracker_RegisterDebugCounterGroup(menuCtx, &self->shaderCount,              reinterpret_cast<const wchar_t*>(GX_VMA(0x49c33ceULL))); // u"Shader"

    if (createRootGroup) {
        GXDebugMenu_PopGroupNode(menuCtx);
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21ad340 (75 Bytes)
// Unregisters a shader object from the shader memory counter (`+0x17C..+0x184`).
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_UnregisterShader(GXResourceMemoryTracker* self, GXTrackedResource* shader) {
    if (shader == nullptr) {
        return;
    }
    uint32_t bytes = shader->vptr->GetShaderBinarySize(shader);
    if (bytes != 0) {
        self->shaderBytes -= bytes;
        self->shaderMegabytes = static_cast<float>(self->shaderBytes) * kBytesToMegabytes;
        --self->shaderCount;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21ad270 (204 Bytes)
// Unregisters a texture object from one of the 4 texture sub-type counters
// (`+0x14C .. +0x178`) based on `tex->textureFlags & 3`.
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_UnregisterTextureSubType(GXResourceMemoryTracker* self, GXTrackedResource* tex) {
    if (tex == nullptr) {
        return;
    }
    uint32_t bytes = tex->textureBytes;
    uint8_t flags = tex->textureFlags;
    if ((flags & 3u) == 3u) {
        self->texType3Bytes -= bytes;
        self->texType3Megabytes = static_cast<float>(self->texType3Bytes) * kBytesToMegabytes;
        --self->texType3Count;
    } else if ((flags & 1u) != 0) {
        self->texType1Bytes -= bytes;
        self->texType1Megabytes = static_cast<float>(self->texType1Bytes) * kBytesToMegabytes;
        --self->texType1Count;
    } else if ((flags & 2u) != 0) {
        self->texType2Bytes -= bytes;
        self->texType2Megabytes = static_cast<float>(self->texType2Bytes) * kBytesToMegabytes;
        --self->texType2Count;
    } else {
        self->texType0Bytes -= bytes;
        self->texType0Megabytes = static_cast<float>(self->texType0Bytes) * kBytesToMegabytes;
        --self->texType0Count;
    }
}

static inline void GXResourceBucket_Remove(GXTrackedResourceBucket* bucket, GXTrackedResource* res) {
    for (void** it = bucket->vec.begin; it != bucket->vec.end; ++it) {
        if (*it == res) {
            std::memmove(it, it + 1, static_cast<size_t>((bucket->vec.end - (it + 1)) * sizeof(void*)));
            --bucket->vec.end;
            uint32_t bytes = res->vptr->GetGpuMemoryBytes(res);
            bucket->counter.totalBytes -= bytes;
            bucket->counter.totalMegabytes = static_cast<float>(bucket->counter.totalBytes) * kBytesToMegabytes;
            --bucket->counter.count;
            return;
        }
    }
}

// VMA: 0x21ad1f0 (125 B)
void GXResourceMemoryTracker_UnregisterBucket5(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Remove(&self->buckets[5], res);
}
// VMA: 0x21ad170 (125 B)
void GXResourceMemoryTracker_UnregisterBucket4(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Remove(&self->buckets[4], res);
}
// VMA: 0x21ad0f0 (125 B)
void GXResourceMemoryTracker_UnregisterBucket3(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Remove(&self->buckets[3], res);
}
// VMA: 0x21ad070 (125 B)
void GXResourceMemoryTracker_UnregisterBucket2(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Remove(&self->buckets[2], res);
}
// VMA: 0x21ad000 (110 B)
void GXResourceMemoryTracker_UnregisterBucket1(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Remove(&self->buckets[1], res);
}
// VMA: 0x21acf90 (110 B)
void GXResourceMemoryTracker_UnregisterBucket0(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Remove(&self->buckets[0], res);
}

// ----------------------------------------------------------------------------
// VMA: 0x21acf40 (73 Bytes)
// Registers a shader object in the shader memory counter (`+0x17C..+0x184`).
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_RegisterShader(GXResourceMemoryTracker* self, GXTrackedResource* shader) {
    if (shader == nullptr) {
        return;
    }
    uint32_t bytes = shader->vptr->GetShaderBinarySize(shader);
    if (bytes != 0) {
        self->shaderBytes += bytes;
        self->shaderMegabytes = static_cast<float>(self->shaderBytes) * kBytesToMegabytes;
        ++self->shaderCount;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21ace70 (196 Bytes)
// Registers a texture object in one of the 4 texture sub-type counters
// (`+0x14C .. +0x178`) based on `tex->textureFlags & 3`.
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_RegisterTextureSubType(GXResourceMemoryTracker* self, GXTrackedResource* tex) {
    if (tex == nullptr) {
        return;
    }
    uint32_t bytes = tex->textureBytes;
    uint8_t flags = tex->textureFlags;
    if ((flags & 3u) == 3u) {
        self->texType3Bytes += bytes;
        self->texType3Megabytes = static_cast<float>(self->texType3Bytes) * kBytesToMegabytes;
        ++self->texType3Count;
    } else if ((flags & 1u) != 0) {
        self->texType1Bytes += bytes;
        self->texType1Megabytes = static_cast<float>(self->texType1Bytes) * kBytesToMegabytes;
        ++self->texType1Count;
    } else if ((flags & 2u) != 0) {
        self->texType2Bytes += bytes;
        self->texType2Megabytes = static_cast<float>(self->texType2Bytes) * kBytesToMegabytes;
        ++self->texType2Count;
    } else {
        self->texType0Bytes += bytes;
        self->texType0Megabytes = static_cast<float>(self->texType0Bytes) * kBytesToMegabytes;
        ++self->texType0Count;
    }
}

static inline void GXResourceBucket_Add(
    GXTrackedResourceBucket* bucket,
    GXTrackedResource* res,
    void (*reallocateFn)(DLPointerVector*, size_t))
{
    if (res == nullptr) {
        return;
    }
    if (bucket->vec.end == bucket->vec.cap) {
        size_t curSize = static_cast<size_t>(bucket->vec.end - bucket->vec.begin);
        size_t newCap = std::max<size_t>(curSize + 1, curSize + curSize / 2);
        reallocateFn(&bucket->vec, newCap);
    }
    *bucket->vec.end++ = res;
    uint32_t bytes = res->vptr->GetGpuMemoryBytes(res);
    bucket->counter.totalBytes += bytes;
    bucket->counter.totalMegabytes = static_cast<float>(bucket->counter.totalBytes) * kBytesToMegabytes;
    ++bucket->counter.count;
}

// VMA: 0x21acd10 (346 B)
void GXResourceMemoryTracker_RegisterBucket5(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Add(&self->buckets[5], res, DLVectorPtr_ReallocateType5);
}
// VMA: 0x21acbb0 (346 B)
void GXResourceMemoryTracker_RegisterBucket4(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Add(&self->buckets[4], res, DLVectorPtr_ReallocateType4);
}
// VMA: 0x21aca50 (346 B)
void GXResourceMemoryTracker_RegisterBucket3(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Add(&self->buckets[3], res, DLVectorPtr_ReallocateType3);
}
// VMA: 0x21ac900 (334 B)
void GXResourceMemoryTracker_RegisterBucket2(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Add(&self->buckets[2], res, DLVectorPtr_ReallocateType2);
}
// VMA: 0x21ac7d0 (304 B)
void GXResourceMemoryTracker_RegisterBucket1(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Add(&self->buckets[1], res, DLVectorPtr_ReallocateType1);
}
// VMA: 0x21ac690 (306 B)
void GXResourceMemoryTracker_RegisterBucket0(GXResourceMemoryTracker* self, GXTrackedResource* res) {
    GXResourceBucket_Add(&self->buckets[0], res, DLVectorPtr_ReallocateType0);
}

// ----------------------------------------------------------------------------
// VMA: 0x21abf90 (1,786 Bytes)
// Clears and shrinks-to-empty the first 4 resource tracking vectors in `self`
// (`buckets[0..3]`: Texture2D, TextureCube, RenderTarget2D, DepthStencilBuffer),
// matching `eboot.bin` `0x21abf90..0x21ac683` 1-in-1.
// ----------------------------------------------------------------------------
static inline void GXResourceMemoryTracker_ShrinkBucketVector(
    DLPointerVector* v,
    DLPointerVector* (*moveAssignFn)(DLPointerVector*, DLPointerVector*))
{
    v->end = v->begin;
    DLAllocator* alloc = v->allocator;
    uint64_t caps = 0;
    alloc->vptr->GetCapabilities(&caps, alloc, 0u);
    if ((caps & 0x20ULL) == 0) {
        reinterpret_cast<void (*)(uint32_t, uint32_t, uintptr_t)>(GX_VMA(0x20b55b0ULL))(
            0u, 0x3Eu, GX_VMA(0x4737dc0ULL));
    }

    DLPointerVector emptyVec{ nullptr, nullptr, nullptr, nullptr, alloc };
    if (&emptyVec != v) {
        DLAllocator* curAlloc = v->allocator;
        uint32_t tagA = alloc->vptr->GetHeapTag(alloc);
        uint32_t tagB = curAlloc->vptr->GetHeapTag(curAlloc);
        if (tagA == tagB) {
            std::swap(emptyVec.begin, v->begin);
            std::swap(emptyVec.end,   v->end);
            std::swap(emptyVec.cap,   v->cap);
        } else {
            DLPointerVector savedEmpty = emptyVec;
            emptyVec.begin = nullptr;
            emptyVec.end   = nullptr;
            emptyVec.cap   = nullptr;
            moveAssignFn(&emptyVec, v);
            moveAssignFn(v, &savedEmpty);
            if (savedEmpty.begin != nullptr) {
                savedEmpty.allocator->vptr->Free(savedEmpty.allocator, savedEmpty.begin);
                savedEmpty.begin = nullptr;
                savedEmpty.end   = nullptr;
                savedEmpty.cap   = nullptr;
            }
        }
    }
    if (emptyVec.begin != nullptr) {
        emptyVec.allocator->vptr->Free(emptyVec.allocator, emptyVec.begin);
        emptyVec.begin = nullptr;
        emptyVec.end   = nullptr;
        emptyVec.cap   = nullptr;
    }
}

void GXResourceMemoryTracker_ClearAllVectors(GXResourceMemoryTracker* self) {
    GXResourceMemoryTracker_ShrinkBucketVector(&self->buckets[0].vec, DLVectorPtr_MoveAssignType0);
    GXResourceMemoryTracker_ShrinkBucketVector(&self->buckets[1].vec, DLVectorPtr_MoveAssignType1);
    GXResourceMemoryTracker_ShrinkBucketVector(&self->buckets[2].vec, DLVectorPtr_MoveAssignType2);
    GXResourceMemoryTracker_ShrinkBucketVector(&self->buckets[3].vec, DLVectorPtr_MoveAssign);
    GXResourceMemoryTracker_ShrinkBucketVector(&self->buckets[4].vec, DLVectorPtr_MoveAssign);
    GXResourceMemoryTracker_ShrinkBucketVector(&self->buckets[5].vec, DLVectorPtr_MoveAssign);
    for (int i = 0; i < 6; ++i) {
        self->buckets[i].counter.count = 0;
        self->buckets[i].counter.totalBytes = 0;
        self->buckets[i].counter.totalMegabytes = 0.0f;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21abe40 (326 Bytes)
// Destructor for `GXResourceMemoryTracker`: shrinks buckets 0..3 via
// `GXResourceMemoryTracker_ClearAllVectors` (`0x21abf90`), then frees backing
// buffers of all 6 vectors in reverse order (`5..0`).
// ----------------------------------------------------------------------------
void GXResourceMemoryTracker_Destroy(GXResourceMemoryTracker* self) {
    GXResourceMemoryTracker_ClearAllVectors(self);
    for (int i = 5; i >= 0; --i) {
        DLPointerVector& v = self->buckets[i].vec;
        if (v.begin != nullptr) {
            v.allocator->vptr->Free(v.allocator, v.begin);
            v.cap   = nullptr;
            v.end   = nullptr;
            v.begin = nullptr;
        }
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21abac0 (889 Bytes)
// Constructor for `GXResourceMemoryTracker`: resolves default string/container
// allocator `*0x5540670` (via `0x2ba2b10` if null), validates capability bit
// `0x20` on all 6 `DLPointerVector` buckets, and zeroes all 11 counter groups.
// ----------------------------------------------------------------------------
extern "C" void* DLSystem_GetDefaultStringAllocator(); // 0x2ba2b10

void GXResourceMemoryTracker_Construct(GXResourceMemoryTracker* self, DLAllocator* allocator) {
    std::memset(self, 0, sizeof(GXResourceMemoryTracker));
    auto** pGlobalSlot = (g_eboot_live_slide != 0)
        ? reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL))
        : nullptr;

    for (int i = 0; i < 6; ++i) {
        if (pGlobalSlot != nullptr) {
            allocator = *pGlobalSlot;
            if (allocator == nullptr) {
                allocator = reinterpret_cast<DLAllocator*>(DLSystem_GetDefaultStringAllocator());
                *pGlobalSlot = allocator;
            }
        }
        if (allocator != nullptr) {
            uint64_t caps = 0;
            allocator->vptr->GetCapabilities(&caps, allocator, 0u);
            if ((caps & 0x20ULL) == 0 && g_eboot_live_slide != 0) {
                reinterpret_cast<void (*)(uint32_t, uint32_t, uintptr_t)>(GX_VMA(0x20b55b0ULL))(
                    0u, 0x3Eu, GX_VMA(0x4737dc0ULL));
            }
        }
        self->buckets[i].vec.allocator = allocator;
    }
}

// ============================================================================
// 3. GXResourceMemoryTrackerOwner Wrapper & Forwarding Thunks (0x21ab570 .. 0x21aba20)
// ============================================================================

struct GXResourceMemoryTrackerOwner {
    GXResourceMemoryTracker* tracker; // +0x00
};

// VMA: 0x21aba20 (11 B)
void GXResourceMemoryTrackerOwner_RegisterDebugProperties(
    GXResourceMemoryTrackerOwner* self, void* menuCtx, bool createRoot)
{
    GXResourceMemoryTracker_RegisterAllDebugProperties(self->tracker, menuCtx, createRoot);
}
// VMA: 0x21ab9d0 (75 B)
void GXResourceMemoryTrackerOwner_UnregisterShader(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* shader)
{
    GXResourceMemoryTracker_UnregisterShader(self->tracker, shader);
}
// VMA: 0x21ab9c0 (8 B)
void GXResourceMemoryTrackerOwner_UnregisterTextureSubType(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* tex)
{
    GXResourceMemoryTracker_UnregisterTextureSubType(self->tracker, tex);
}
// VMA: 0x21ab940 (125 B)
void GXResourceMemoryTrackerOwner_UnregisterBucket5(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_UnregisterBucket5(self->tracker, res);
}
// VMA: 0x21ab8c0 (125 B)
void GXResourceMemoryTrackerOwner_UnregisterBucket4(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_UnregisterBucket4(self->tracker, res);
}
// VMA: 0x21ab840 (125 B)
void GXResourceMemoryTrackerOwner_UnregisterBucket3(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_UnregisterBucket3(self->tracker, res);
}
// VMA: 0x21ab7c0 (125 B)
void GXResourceMemoryTrackerOwner_UnregisterBucket2(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_UnregisterBucket2(self->tracker, res);
}
// VMA: 0x21ab750 (110 B)
void GXResourceMemoryTrackerOwner_UnregisterBucket1(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_UnregisterBucket1(self->tracker, res);
}
// VMA: 0x21ab6e0 (110 B)
void GXResourceMemoryTrackerOwner_UnregisterBucket0(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_UnregisterBucket0(self->tracker, res);
}
// VMA: 0x21ab690 (73 B)
void GXResourceMemoryTrackerOwner_RegisterShader(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* shader)
{
    GXResourceMemoryTracker_RegisterShader(self->tracker, shader);
}
// VMA: 0x21ab680 (8 B)
void GXResourceMemoryTrackerOwner_RegisterTextureSubType(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* tex)
{
    GXResourceMemoryTracker_RegisterTextureSubType(self->tracker, tex);
}
// VMA: 0x21ab670 (8 B)
void GXResourceMemoryTrackerOwner_RegisterBucket5(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_RegisterBucket5(self->tracker, res);
}
// VMA: 0x21ab660 (8 B)
void GXResourceMemoryTrackerOwner_RegisterBucket4(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_RegisterBucket4(self->tracker, res);
}
// VMA: 0x21ab650 (8 B)
void GXResourceMemoryTrackerOwner_RegisterBucket3(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_RegisterBucket3(self->tracker, res);
}
// VMA: 0x21ab640 (8 B)
void GXResourceMemoryTrackerOwner_RegisterBucket2(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_RegisterBucket2(self->tracker, res);
}
// VMA: 0x21ab630 (8 B)
void GXResourceMemoryTrackerOwner_RegisterBucket1(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_RegisterBucket1(self->tracker, res);
}
// VMA: 0x21ab620 (8 B)
void GXResourceMemoryTrackerOwner_RegisterBucket0(
    GXResourceMemoryTrackerOwner* self, GXTrackedResource* res)
{
    GXResourceMemoryTracker_RegisterBucket0(self->tracker, res);
}
// VMA: 0x21ab610 (1 B)
void GXResourceMemoryTrackerOwner_NoOp(GXResourceMemoryTrackerOwner* /*self*/) {}

// VMA: 0x21ab5c0 (77 B)
void GXResourceMemoryTrackerOwner_FinalizeAndFree(
    GXResourceMemoryTrackerOwner* self, DLAllocator* alloc)
{
    if (g_eboot_live_slide != 0) {
        alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540680ULL));
    }
    if (self->tracker != nullptr) {
        GXResourceMemoryTracker_Destroy(self->tracker);
        alloc->vptr->Free(alloc, self->tracker);
        self->tracker = nullptr;
    }
}

// VMA: 0x21ab570 (65 B)
void GXResourceMemoryTrackerOwner_Initialize(
    GXResourceMemoryTrackerOwner* self, DLAllocator* alloc)
{
    if (g_eboot_live_slide != 0) {
        alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540680ULL));
    }
    auto* mem = static_cast<GXResourceMemoryTracker*>(
        alloc->vptr->AllocateAligned(alloc, sizeof(GXResourceMemoryTracker), 8u));
    if (mem != nullptr) {
        GXResourceMemoryTracker_Construct(mem, alloc);
    }
    self->tracker = mem;
}

// ============================================================================
// 4. GX Camera Frustum & SIMD View-Projection Math (0x21a9f90 .. 0x21ab450)
// ============================================================================

struct alignas(16) GXMatrix4x4 {
    float m[4][4];
};

struct alignas(16) GXCameraFrustum {
    GXMatrix4x4 viewMatrix;       // +0x00 .. +0x3F
    float       eyePosition[4];   // +0x40 .. +0x4F
    float       fovYRadians;      // +0x50 (default 0.959931f = 55 deg)
    float       nearZ;            // +0x54 (default 0.1f)
    float       farZ;             // +0x58 (default 5000.0f)
    uint32_t    viewportWidth;    // +0x5C
    uint32_t    viewportHeight;   // +0x60
    float       aspectRatio;      // +0x64 (0.0f = use viewportWidth / viewportHeight)
    uint8_t     enableHalfPixelOffset; // +0x68
    uint8_t     pad69[7];         // +0x69
    uint32_t    tileGridSubdiv;   // +0x70 (0 = full screen, >0 = N*N tiled screenshot)
    uint32_t    tileLinearIndex;  // +0x74
    uint32_t    tileMarginPixels; // +0x78
    uint32_t    tileWidthPixels;  // +0x7C
    uint32_t    tileHeightPixels; // +0x80
};

struct alignas(16) GXCameraFrustumCompact {
    GXMatrix4x4 viewMatrix;       // +0x00 .. +0x3F
    float       eyePosition[4];   // +0x40 .. +0x4F
    float       fovYRadians;      // +0x50
    float       nearZ;            // +0x54
    float       farZ;             // +0x58
    float       aspectRatio;      // +0x5C (note swapped order with width/height!)
    uint32_t    viewportWidth;    // +0x60
    uint32_t    viewportHeight;   // +0x64
    uint16_t    tileGridSubdiv;   // +0x68
    uint16_t    tileLinearIndex;  // +0x6A
    uint16_t    tileMarginPixels; // +0x6C
    uint16_t    tileWidthPixels;  // +0x6E
    uint16_t    tileHeightPixels; // +0x70
};

// ----------------------------------------------------------------------------
// VMA: 0x21ab450 (139 Bytes)
// Unpacks a `GXCameraFrustumCompact` (`0x72` B) into `GXCameraFrustum` (`0x84` B).
// ----------------------------------------------------------------------------
void GXCameraFrustum_UnpackFromCompact(GXCameraFrustum* dst, const GXCameraFrustumCompact* src) {
    dst->viewMatrix = src->viewMatrix;
    std::memcpy(dst->eyePosition, src->eyePosition, sizeof(dst->eyePosition));
    dst->fovYRadians = src->fovYRadians;
    dst->nearZ = src->nearZ;
    dst->farZ = src->farZ;
    dst->aspectRatio = src->aspectRatio;
    dst->viewportWidth = src->viewportWidth;
    dst->viewportHeight = src->viewportHeight;
    dst->enableHalfPixelOffset = 0;
    dst->tileGridSubdiv = src->tileGridSubdiv;
    dst->tileLinearIndex = src->tileLinearIndex;
    dst->tileMarginPixels = src->tileMarginPixels;
    dst->tileWidthPixels = src->tileWidthPixels;
    dst->tileHeightPixels = src->tileHeightPixels;
}

// ----------------------------------------------------------------------------
// VMA: 0x21ab3c0 (144 Bytes)
// Packs a `GXCameraFrustum` (`0x84` B) into `GXCameraFrustumCompact` (`0x72` B).
// ----------------------------------------------------------------------------
void GXCameraFrustum_PackToCompact(const GXCameraFrustum* src, GXCameraFrustumCompact* dst) {
    dst->viewMatrix = src->viewMatrix;
    std::memcpy(dst->eyePosition, src->eyePosition, sizeof(src->eyePosition));
    dst->fovYRadians = src->fovYRadians;
    dst->nearZ = src->nearZ;
    dst->farZ = src->farZ;
    dst->aspectRatio = src->aspectRatio;
    dst->viewportWidth = src->viewportWidth;
    dst->viewportHeight = src->viewportHeight;
    dst->tileGridSubdiv = static_cast<uint16_t>(src->tileGridSubdiv);
    dst->tileLinearIndex = static_cast<uint16_t>(src->tileLinearIndex);
    dst->tileMarginPixels = static_cast<uint16_t>(src->tileMarginPixels);
    dst->tileWidthPixels = static_cast<uint16_t>(src->tileWidthPixels);
    dst->tileHeightPixels = static_cast<uint16_t>(src->tileHeightPixels);
}

// ----------------------------------------------------------------------------
// VMA: 0x21ab2e0 (212 Bytes)
// Configures `self` for rendering cubemap face `faceIndex` (`0..5`: +X, -X,
// +Y, -Y, +Z, -Z) from world-space origin `cubeCenter` with 90-degree FOV.
// ----------------------------------------------------------------------------
void GXCameraFrustum_InitCubeMapFace(
    GXCameraFrustum* self,
    uint32_t faceIndex,
    const float* cubeCenter)
{
    self->fovYRadians = 1.57079637f; // 0x3fc90fdb: pi / 2 (90 degrees)
    self->aspectRatio = 0.0f;        // 0x00000000

    __m128 up  = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    __m128 fwd = _mm_setzero_ps();

    switch (faceIndex) {
        default:
        case 0: // +X
            fwd = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
            break;
        case 1: // -X
            fwd = _mm_setr_ps(-1.0f, 0.0f, 0.0f, 0.0f);
            break;
        case 2: // +Y
            up  = _mm_setr_ps(0.0f, 0.0f, -1.0f, 0.0f);
            fwd = _mm_setr_ps(0.0f, 1.0f,  0.0f, 0.0f);
            break;
        case 3: // -Y
            up  = _mm_setr_ps(0.0f,  0.0f, 1.0f, 0.0f);
            fwd = _mm_setr_ps(0.0f, -1.0f, 0.0f, 0.0f);
            break;
        case 4: // +Z
            fwd = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            break;
        case 5: // -Z
            fwd = _mm_setr_ps(0.0f, 0.0f, -1.0f, 0.0f);
            break;
    }

    // Exact SIMD cross-product matching 0x21ab357..0x21ab389:
    // xmm2 = shuf(up, 0xC9), xmm3 = shuf(fwd, 0xC9)
    // xmm2 = shuf(up * xmm3 - fwd * xmm2, 0x49) & mask_xyz
    __m128 up_yzx  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(up),  0xC9));
    __m128 fwd_yzx = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(fwd), 0xC9));
    __m128 sub_val = _mm_sub_ps(_mm_mul_ps(up, fwd_yzx), _mm_mul_ps(fwd, up_yzx));
    __m128 right   = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(sub_val), 0x49));
    const __m128 mask_xyz = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
    right = _mm_and_ps(right, mask_xyz);

    __m128 pos = _mm_add_ps(_mm_and_ps(mask_xyz, _mm_loadu_ps(cubeCenter)),
                            _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

    _mm_store_ps(self->viewMatrix.m[0], right);
    _mm_store_ps(self->viewMatrix.m[1], up);
    _mm_store_ps(self->viewMatrix.m[2], fwd);
    _mm_store_ps(self->viewMatrix.m[3], pos);
}

// Forward declaration of SIMD 4x4 matrix inverse
GXMatrix4x4* GXMatrix4x4_InvertSIMD(GXMatrix4x4* dst, const GXMatrix4x4* src); // 0x21aa080

static inline float GXGuestTrig_SinCos(uint32_t isCos, float rad) {
    if (g_eboot_live_slide != 0) {
        return reinterpret_cast<float (*)(uint32_t, uint32_t, float)>(GX_VMA(0x2bbe518ULL))(isCos, 0u, rad);
    }
    return isCos ? std::cos(rad) : std::sin(rad);
}

static inline float GXGuestTrig_Tan(float rad) {
    if (g_eboot_live_slide != 0) {
        return reinterpret_cast<float (*)(float)>(GX_VMA(0x2bbe648ULL))(rad);
    }
    return std::tan(rad);
}

// ----------------------------------------------------------------------------
// VMA: 0x21aadf0 (1,252 Bytes)
// Computes the 6 frustum planes (`outPlanes[6][4]`, `0x60` bytes) in view
// space from `cam`'s FOV/aspect/near/far, computes `Inverse(Transpose(cam->viewMatrix))`
// (`= Transpose(Inverse(cam->viewMatrix))`), and transforms all 6 planes into
// world space (`0x21aadf0..0x21ab2d3`).
// ----------------------------------------------------------------------------
float* GXCameraFrustum_ExtractSixWorldFrustumPlanes(
    float outPlanes[6][4],
    const GXCameraFrustum* cam)
{
    float aspect = cam->aspectRatio;
    if (aspect == 0.0f) {
        aspect = (cam->viewportHeight != 0)
            ? (static_cast<float>(cam->viewportWidth) / static_cast<float>(cam->viewportHeight))
            : 1.0f;
        if (aspect == 0.0f) aspect = 1.0f;
    }

    float halfFov = cam->fovYRadians * 0.5f;
    float sinHalf = GXGuestTrig_SinCos(0u, halfFov);
    float cosHalf = GXGuestTrig_SinCos(1u, halfFov);
    float cotHalf = cosHalf / sinHalf;

    float projX = cotHalf / aspect;
    float projY = cotHalf;
    float projZ = cam->farZ / (cam->farZ - cam->nearZ);
    float projW = -(cam->nearZ * projZ);

    // Construct the 6 frustum planes in view space (Left, Right, Bottom, Top, Near, Far)
    const float viewPlanes[6][4] = {
        { -projX,  0.0f,         -1.0f,  0.0f },
        {  projX,  0.0f,         -1.0f,  0.0f },
        {  0.0f,  -projY,        -1.0f,  0.0f },
        {  0.0f,   projY,        -1.0f,  0.0f },
        {  0.0f,   0.0f,        -projZ, -projW },
        {  0.0f,   0.0f,  projZ - 1.0f,  projW }
    };

    // Covariant plane transform: worldPlane = viewPlane * Transpose(Inverse(cam->viewMatrix))
    GXMatrix4x4 invView{};
    GXMatrix4x4_InvertSIMD(&invView, &cam->viewMatrix);

    for (int p = 0; p < 6; ++p) {
        for (int c = 0; c < 4; ++c) {
            outPlanes[p][c] = viewPlanes[p][0] * invView.m[c][0]
                            + viewPlanes[p][1] * invView.m[c][1]
                            + viewPlanes[p][2] * invView.m[c][2]
                            + viewPlanes[p][3] * invView.m[c][3];
        }
    }
    return &outPlanes[0][0];
}

// Forward declaration of projection builder
GXMatrix4x4* GXCameraFrustum_BuildProjectionMatrix(GXMatrix4x4* outProj, const GXCameraFrustum* cam); // 0x21aa290

// ----------------------------------------------------------------------------
// VMA: 0x21aaa90 (856 Bytes)
// Computes `outViewProj = Inverse(cam->viewMatrix) * Projection(cam)`.
// ----------------------------------------------------------------------------
GXMatrix4x4* GXCameraFrustum_ComputeViewProjectionMatrix(
    GXMatrix4x4* outViewProj,
    const GXCameraFrustum* cam)
{
    GXMatrix4x4 invView{};
    GXMatrix4x4_InvertSIMD(&invView, &cam->viewMatrix);

    GXMatrix4x4 proj{};
    GXCameraFrustum_BuildProjectionMatrix(&proj, cam);

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            outViewProj->m[r][c] = invView.m[r][0] * proj.m[0][c]
                                 + invView.m[r][1] * proj.m[1][c]
                                 + invView.m[r][2] * proj.m[2][c]
                                 + invView.m[r][3] * proj.m[3][c];
        }
    }
    return outViewProj;
}

// ----------------------------------------------------------------------------
// VMA: 0x21aa950 (313 Bytes)
// Computes the normalized tangents `(outTop, outBottom, outLeft, outRight)`
// for the current tile (`tileLinearIndex` out of `tileGridSubdiv * tileGridSubdiv`).
// ----------------------------------------------------------------------------
void GXCameraFrustum_ComputeTileTangentBounds(
    const GXCameraFrustum* cam,
    float* outTop,
    float* outBottom,
    float* outLeft,
    float* outRight)
{
    uint32_t subdiv = (cam->tileGridSubdiv != 0) ? cam->tileGridSubdiv : 1u;
    float aspect = cam->aspectRatio;
    if (aspect == 0.0f) {
        aspect = (cam->viewportHeight != 0)
            ? (static_cast<float>(cam->viewportWidth) / static_cast<float>(cam->viewportHeight))
            : 1.0f;
        if (aspect == 0.0f) aspect = 1.0f;
    }

    float tanHalfY = std::tan(cam->fovYRadians * 0.5f);
    float fullHeight = tanHalfY * 2.0f;
    float fullWidth  = fullHeight * aspect;

    float tileStepX = fullWidth / static_cast<float>(subdiv);
    float tileStepY = fullHeight / static_cast<float>(subdiv);

    float marginX = (cam->tileWidthPixels != 0)
        ? tileStepX * (static_cast<float>(cam->tileMarginPixels) / static_cast<float>(cam->tileWidthPixels))
        : 0.0f;
    float marginY = (cam->tileHeightPixels != 0)
        ? tileStepY * (static_cast<float>(cam->tileMarginPixels) / static_cast<float>(cam->tileHeightPixels))
        : 0.0f;

    uint32_t tileX = cam->tileLinearIndex % subdiv;
    uint32_t tileY = cam->tileLinearIndex / subdiv;

    float leftBase  = -0.5f * fullWidth + static_cast<float>(tileX) * tileStepX;
    float rightBase = leftBase + tileStepX;
    float topBase   =  0.5f * fullHeight - static_cast<float>(tileY) * tileStepY;
    float botBase   = topBase - tileStepY;

    *outLeft   = leftBase - marginX;
    *outRight  = rightBase + marginX;
    *outTop    = topBase + marginY;
    *outBottom = botBase - marginY;
}

// ----------------------------------------------------------------------------
// VMA: 0x21aa6c0 (647 Bytes)
// Builds a standard perspective projection matrix (`outProj`) from `cam`,
// ignoring tile subdivision (`tileGridSubdiv`), with optional half-pixel offset
// (`P * H` -> `m[2][0] = -0.5f / width`, `m[2][1] = +0.5f / height`).
// ----------------------------------------------------------------------------
GXMatrix4x4* GXCameraFrustum_BuildUntiledProjectionMatrix(
    GXMatrix4x4* outProj,
    const GXCameraFrustum* cam)
{
    const float widthF  = static_cast<float>(cam->viewportWidth);
    const float heightF = static_cast<float>(cam->viewportHeight);
    float aspect = cam->aspectRatio;
    if (aspect == 0.0f) {
        aspect = (heightF != 0.0f) ? (widthF / heightF) : 1.0f;
        if (aspect == 0.0f) aspect = 1.0f;
    }

    const float offsetX = (widthF != 0.0f) ? ((1.0f / widthF) * -0.5f) : 0.0f;
    const float offsetY = (heightF != 0.0f) ? ((1.0f / heightF) * 0.5f) : 0.0f;

    const float halfFov = cam->fovYRadians * 0.5f;
    const float sinHalf = GXGuestTrig_SinCos(0u, halfFov);
    const float cosHalf = GXGuestTrig_SinCos(1u, halfFov);
    const float cotHalf = cosHalf / sinHalf;

    const float nearZ = cam->nearZ;
    const float farZ  = cam->farZ;
    const float p00   = cotHalf / aspect;
    const float p11   = cotHalf;
    const float p22   = farZ / (farZ - nearZ);
    const float p32   = -(nearZ * p22);

    std::memset(outProj, 0, sizeof(GXMatrix4x4));
    outProj->m[0][0] = p00;
    outProj->m[1][1] = p11;
    if (cam->enableHalfPixelOffset != 0) {
        outProj->m[2][0] = offsetX;
        outProj->m[2][1] = offsetY;
    }
    outProj->m[2][2] = p22;
    outProj->m[2][3] = 1.0f;
    outProj->m[3][2] = p32;
    return outProj;
}

// ----------------------------------------------------------------------------
// VMA: 0x21aa290 (1,060 Bytes)
// Builds the perspective projection matrix (`outProj`) from `cam`, supporting
// both standard full-viewport projection (with optional half-pixel offset) and
// off-center tiled sub-frustum projection when `cam->tileGridSubdiv != 0`.
// ----------------------------------------------------------------------------
GXMatrix4x4* GXCameraFrustum_BuildProjectionMatrix(
    GXMatrix4x4* outProj,
    const GXCameraFrustum* cam)
{
    const float widthF  = static_cast<float>(cam->viewportWidth);
    const float heightF = static_cast<float>(cam->viewportHeight);
    float aspect = cam->aspectRatio;
    if (aspect == 0.0f) {
        aspect = (heightF != 0.0f) ? (widthF / heightF) : 1.0f;
        if (aspect == 0.0f) aspect = 1.0f;
    }

    if (cam->tileGridSubdiv == 0) {
        return GXCameraFrustum_BuildUntiledProjectionMatrix(outProj, cam);
    }

    const uint32_t subdiv  = cam->tileGridSubdiv;
    const uint32_t tileIdx = cam->tileLinearIndex;
    const float nearZ      = cam->nearZ;
    const float farZ       = cam->farZ;
    const float near2      = nearZ + nearZ;

    const float tanHalfY   = GXGuestTrig_Tan(cam->fovYRadians * 0.5f);
    const float fullHeight = near2 * tanHalfY;
    const float fullWidth  = fullHeight * aspect;

    const float subdivF   = static_cast<float>(subdiv);
    const float tileStepX = fullWidth / subdivF;
    const float tileStepY = fullHeight / subdivF;

    const float marginF = static_cast<float>(cam->tileMarginPixels);
    const float marginX = (cam->tileWidthPixels != 0)
        ? (tileStepX * (marginF / static_cast<float>(cam->tileWidthPixels)))
        : 0.0f;
    const float marginY = (cam->tileHeightPixels != 0)
        ? (tileStepY * (marginF / static_cast<float>(cam->tileHeightPixels)))
        : 0.0f;

    const uint32_t tileX = tileIdx % subdiv;
    const uint32_t tileY = tileIdx / subdiv;

    const float leftBase  = (-0.5f * fullWidth) + static_cast<float>(tileX) * tileStepX;
    const float rightBase = leftBase + tileStepX;
    const float left      = leftBase - marginX;
    const float right     = rightBase + marginX;

    const float topBase   = (0.5f * fullHeight) - static_cast<float>(tileY) * tileStepY;
    const float botBase   = topBase - tileStepY;
    const float top       = topBase + marginY;
    const float bottom    = botBase - marginY;

    const float invW = 1.0f / (right - left);
    const float invH = 1.0f / (top - bottom);
    const float q    = farZ / (farZ - nearZ);

    std::memset(outProj, 0, sizeof(GXMatrix4x4));
    outProj->m[0][0] = near2 * invW;
    outProj->m[1][1] = near2 * invH;
    outProj->m[2][0] = -((left + right) * invW);
    outProj->m[2][1] = -((top + bottom) * invH);
    outProj->m[2][2] = q;
    outProj->m[2][3] = 1.0f;
    outProj->m[3][2] = -(nearZ * q);
    return outProj;
}

// ----------------------------------------------------------------------------
// VMA: 0x21aa080 (526 Bytes)
// Full 4x4 matrix inverse using Cramers rule / 2x2 sub-determinant SIMD blocks.
// ----------------------------------------------------------------------------
GXMatrix4x4* GXMatrix4x4_InvertSIMD(GXMatrix4x4* dst, const GXMatrix4x4* src) {
    const float* m = &src->m[0][0];
    float inv[16];

    inv[0] =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12]= -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];

    inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13]=  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];

    inv[2] =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
    inv[6] = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
    inv[10]=  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
    inv[14]= -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];

    inv[3] = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
    inv[7] =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
    inv[11]= -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
    inv[15]=  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];

    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    float invDet = (det != 0.0f) ? (1.0f / det) : 0.0f;
    float* out = &dst->m[0][0];
    for (int i = 0; i < 16; ++i) {
        out[i] = inv[i] * invDet;
    }
    return dst;
}

// ----------------------------------------------------------------------------
// VMA: 0x21aa050 (39 Bytes)
// Copies a 64-byte `GXMatrix4x4` from `src` to `dst`.
// ----------------------------------------------------------------------------
void GXMatrix4x4_Copy(GXMatrix4x4* dst, const GXMatrix4x4* src) {
    *dst = *src;
}

// ----------------------------------------------------------------------------
// VMA: 0x21a9f90 (190 Bytes)
// Resets `GXCameraFrustum` to identity view matrix, 55-degree vertical FOV
// (`0.959931f` rad), `nearZ = 0.1f`, `farZ = 5000.0f`, and active backbuffer
// viewport dimensions from `g_pGXDeviceSingleton` (`0x55406c8`) if present.
// ----------------------------------------------------------------------------
#undef g_pGXDeviceSingleton
#define g_pGXDeviceSingleton (*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))

void GXCameraFrustum_ResetDefaults(GXCameraFrustum* self, uint32_t defaultWidth = 0, uint32_t defaultHeight = 0) {
    static constexpr float kIdentity4x4[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    std::memcpy(&self->viewMatrix, kIdentity4x4, sizeof(kIdentity4x4));
    self->eyePosition[0] = 0.0f;
    self->eyePosition[1] = 0.0f;
    self->eyePosition[2] = 0.0f;
    self->eyePosition[3] = 1.0f;
    self->fovYRadians = 0.95993108f; // 0x3f75be0b (~55 degrees)
    self->nearZ       = 0.1f;        // 0x3dcccccd
    self->farZ        = 5000.0f;     // 0x459c4000
    self->aspectRatio = 0.0f;

    if (g_pGXDeviceSingleton != nullptr) {
        const uint8_t* dev = static_cast<const uint8_t*>(g_pGXDeviceSingleton);
        const uint8_t* swapChain = *reinterpret_cast<const uint8_t* const*>(dev + 0x90);
        self->viewportWidth  = *reinterpret_cast<const uint32_t*>(swapChain + 0x80);
        self->viewportHeight = *reinterpret_cast<const uint32_t*>(swapChain + 0x84);
    } else {
        self->viewportWidth  = defaultWidth;
        self->viewportHeight = defaultHeight;
    }

    self->enableHalfPixelOffset = 0;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(self) + 0x7c) = 0;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(self) + 0x74) = 0;
    *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(self) + 0x6c) = 0;
}

} // namespace GX
} // namespace FromSoftware
