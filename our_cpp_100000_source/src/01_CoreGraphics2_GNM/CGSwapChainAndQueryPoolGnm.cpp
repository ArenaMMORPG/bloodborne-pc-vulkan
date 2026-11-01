// ============================================================================
// Bloodborne Engine Decompilation Project (CUSA03173 v1.09)
// Module: 01_CoreGraphics2_GNM
// File: CGSwapChainAndQueryPoolGnm.cpp
// Range: 0x26d3a70 .. 0x26d44b0, 0x26d5a80 .. 0x26d5e90 (Right-to-Left Edge)
// ============================================================================

#include <cstdint>
#include <cstring>
#include "gx_direct_vulkan.h"

extern "C" {
uint32_t GXGammaRampAndFlipPresenter_PresentAndFlip(
    uint8_t* self, uint8_t* depthTargetContainer, uint32_t flipRateMode);
uint32_t GXGammaRampAndFlipPresenter_ClearOutputs(
    const void* unusedSelf, uint8_t* outFlag, uint64_t* outValue);
uint32_t GXGammaRampAndFlipPresenter_ReturnZeroA();
void* GXGammaRampAndFlipPresenter_CopyGammaRampTable(
    const uint8_t* self, void* outGammaTable);
uint32_t GXGammaRampAndFlipPresenter_SetGammaRampTable(
    uint8_t* self, const float* inGammaRgbTriplets);
uint32_t GXGammaRampAndFlipPresenter_ReturnZeroB();
void GXGammaRampAndFlipPresenter_CopyDisplayConfig72(
    const uint8_t* self, void* outConfig72);
uint32_t GXGammaRampAndFlipPresenter_ReturnZeroC();
void GXGammaRampAndFlipPresenter_Destroy(uint8_t* self);
}

namespace Bloodborne::CoreGraphics2::Gnm {

// Forward declarations of PS4 SDK / GNM / Kernel imports
struct GnmRenderTarget {
    uint8_t opaque[0x2c]; // 44 bytes per sce::Gnm::RenderTarget
};

struct IAllocator {
    struct VTable {
        void* pad0[10];
        void* (*AllocateNoAlign)(IAllocator* self, size_t size);        // +0x50
        void* (*Allocate)(IAllocator* self, size_t size, size_t align); // +0x58
        void* pad1[2];
        void (*Free)(IAllocator* self, void* ptr);                      // +0x70
    };
    VTable* vtbl;
};

struct SwapChainDesc {
    uint32_t width;        // +0x00
    uint32_t height;       // +0x04
    uint32_t format;       // +0x08 (0x57 = DXGI_FORMAT_B8G8R8A8_UNORM)
    uint32_t refresh_num;  // +0x0c
    uint32_t refresh_den;  // +0x10
    uint32_t sample_count; // +0x14
    uint32_t sample_qual;  // +0x18
    uint32_t usage;        // +0x1c
    uint32_t flags;        // +0x20
    uint32_t pad24;        // +0x24
    uint32_t buffer_count; // +0x28
    uint8_t  windowed;     // +0x2c
    uint8_t  flip_mode;    // +0x2d
};

struct CGSwapChainGnm {
    void*            device_ctx;          // +0x000
    SwapChainDesc    desc;                // +0x008 (size 0x30)
    GnmRenderTarget  render_targets[16];  // +0x038 (16 * 0x2c = 0x2c0)
    int32_t          video_out_handle;    // +0x2f0 (0x80290001 = invalid)
    uint32_t         current_backbuffer;  // +0x2f4
    void*            display_mem_base;    // +0x2f8
    void*            allocator;           // +0x300
};

// ============================================================================
// 1. SwapChain & sceVideoOut Management (0x26d5a80 .. 0x26d5e90)
// ============================================================================

// VMA: 0x26d5e90 (3006 B)
// Initializes sceVideoOut handle, allocates tiled/linear-aligned display buffers,
// initializes sce::Gnm::RenderTarget array, and registers buffers with VideoOut.
int32_t CGSwapChainGnm_InitDisplayAndBackBuffers(
    uint8_t* self,
    void** out_sub_obj,
    void** out_ref_obj,
    void* device_wrapper,
    const uint8_t* desc,
    IAllocator* cpu_alloc,
    IAllocator* gpu_alloc)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm40CGSwapChainGnm_InitDisplayAndBackBuffersEPNS1_14CGSwapChainGnmEPvPKNS1_13SwapChainDescEPNS1_10IAllocatorE");
int32_t CGSwapChainGnm_InitDisplayAndBackBuffers(
    uint8_t* self,
    void** out_sub_obj,
    void** out_ref_obj,
    void* device_wrapper,
    const uint8_t* desc,
    IAllocator* cpu_alloc,
    IAllocator* gpu_alloc)
{
    if (*reinterpret_cast<const uint32_t*>(desc + 0x10) != 0x57u) {
        return 8;
    }

    void* subObj = reinterpret_cast<void*(*)(size_t, size_t, void*)>(GX_VMA(0x207c4e0ULL))(1, 1, cpu_alloc);
    if (!subObj) {
        *out_sub_obj = nullptr;
        return 9;
    }

    uint8_t dummyInit[0x30]{};
    reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26dbcb0ULL))(subObj, dummyInit);
    *out_sub_obj = subObj;

    uint32_t gnmFmt = 0;
    int32_t fmtErr = reinterpret_cast<int32_t(*)(uint32_t*, uint32_t)>(GX_VMA(0x26b7310ULL))(
        &gnmFmt, *reinterpret_cast<const uint32_t*>(desc + 0x10));
    if (fmtErr != 0) {
        return fmtErr;
    }

    uint32_t tileMode = 0;
    int32_t tmErr = reinterpret_cast<int32_t(*)(uint32_t*, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x109cb30ULL))(
        &tileMode, 1u, gnmFmt, 1u);
    if (tmErr != 0) {
        reinterpret_cast<void(*)(int32_t)>(GX_VMA(0x26bb4a0ULL))(tmErr);
        return 8;
    }

    bool isLinear8 = (tileMode == 8u);
    uint32_t width  = *reinterpret_cast<const uint32_t*>(desc + 0x00);
    uint32_t height = *reinterpret_cast<const uint32_t*>(desc + 0x04);
    uint32_t samplesLog2 = reinterpret_cast<uint32_t(*)(uint32_t, uint32_t)>(GX_VMA(0x26b7a20ULL))(
        *reinterpret_cast<const uint32_t*>(desc + 0x1c), 0u);
    uint32_t fragsLog2 = reinterpret_cast<uint32_t(*)(uint32_t, uint32_t, uint32_t)>(GX_VMA(0x26b7ab0ULL))(
        *reinterpret_cast<const uint32_t*>(desc + 0x1c),
        *reinterpret_cast<const uint32_t*>(desc + 0x20),
        0u);

    uint8_t rtProto[0x2c]{};
    uint64_t sizeAlignPair = reinterpret_cast<uint64_t(*)(
        void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void*, void*)>(
        GX_VMA(0x1085f90ULL))(
        rtProto, width, height, 1u, gnmFmt, tileMode, samplesLog2, fragsLog2, nullptr, nullptr);
    if (static_cast<uint32_t>(sizeAlignPair) == 0u) {
        return 5;
    }

    uint32_t bufCount = *reinterpret_cast<const uint32_t*>(desc + 0x28);
    for (uint32_t i = 0; i < bufCount; ++i) {
        std::memcpy(self + static_cast<size_t>(i) * 0x2cULL, rtProto, 0x2c);
    }

    uint32_t rtSize  = static_cast<uint32_t>(sizeAlignPair);
    uint32_t rtAlign = static_cast<uint32_t>(sizeAlignPair >> 32);
    uint32_t alignedStride = (rtSize + rtAlign - 1u) & ~(rtAlign - 1u);
    size_t totalGpuSize = static_cast<size_t>(alignedStride) * bufCount + 8ULL;

    void* gpuMem = gpu_alloc->vtbl->Allocate(gpu_alloc, totalGpuSize, rtAlign);
    if (!gpuMem) {
        if (*out_sub_obj) {
            reinterpret_cast<void(*)(void*)>(GX_VMA(0x26dbcc0ULL))(*out_sub_obj);
            cpu_alloc->vtbl->Free(cpu_alloc, *out_sub_obj);
        }
        return 9;
    }

    if (!reinterpret_cast<bool(*)(void*)>(GX_VMA(0x26badd0ULL))(gpuMem)) {
        gpu_alloc->vtbl->Free(gpu_alloc, gpuMem);
        if (*out_sub_obj) {
            reinterpret_cast<void(*)(void*)>(GX_VMA(0x26dbcc0ULL))(*out_sub_obj);
            cpu_alloc->vtbl->Free(cpu_alloc, *out_sub_obj);
        }
        return 8;
    }

    gx_direct_vk::PrepareDirectSwapQueryWrite(gpuMem, totalGpuSize);
    std::memset(gpuMem, 0, totalGpuSize);
    uint32_t curOff = 0;
    for (uint32_t i = 0; i < bufCount; ++i) {
        uint8_t* rtSlot = self + static_cast<size_t>(i) * 0x2cULL;
        uint32_t base256 = reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x26baeb0ULL))(
            static_cast<uint8_t*>(gpuMem) + curOff);
        reinterpret_cast<void(*)(void*, uint32_t)>(GX_VMA(0x1086c50ULL))(rtSlot, base256);
        gx_direct_vk::RegisterDirectColorTarget(rtSlot, nullptr, 0u, 0u, 1u);
        curOff += alignedStride;
    }

    *reinterpret_cast<IAllocator**>(self + 0x2c0) = cpu_alloc;
    *reinterpret_cast<IAllocator**>(self + 0x2c8) = gpu_alloc;
    *reinterpret_cast<void**>(self + 0x2d0)       = gpuMem;
    *reinterpret_cast<void**>(self + 0x2d8)       =
        reinterpret_cast<void*(*)(void*)>(GX_VMA(0x21676b0ULL))(device_wrapper);
    *reinterpret_cast<uint64_t*>(self + 0x2e0)    = 0;
    *reinterpret_cast<void**>(self + 0x2e8)       = static_cast<uint8_t*>(gpuMem) + totalGpuSize - 8ULL;
    *reinterpret_cast<int32_t*>(self + 0x2f0)     = static_cast<int32_t>(0x80290001u);
    *reinterpret_cast<uint64_t*>(self + 0x2f8)    = 0;
    std::memcpy(self + 0x300, desc, 0x48);

    auto cleanupOnError = [&]() -> int32_t {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x2171c60ULL))(self + 0x380);
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x2172290ULL))(self + 0x348);
        if (*reinterpret_cast<void**>(self + 0x2f8) != nullptr) {
            reinterpret_cast<void(*)(void*)>(GX_VMA(0x2bbf5b8ULL))(*reinterpret_cast<void**>(self + 0x2f8));
            *reinterpret_cast<uint64_t*>(self + 0x2f8) = 0;
        }
        if (*reinterpret_cast<int32_t*>(self + 0x2f0) != static_cast<int32_t>(0x80290001u)) {
            reinterpret_cast<void(*)(int32_t)>(GX_VMA(0x2bbf5c8ULL))(*reinterpret_cast<int32_t*>(self + 0x2f0));
            *reinterpret_cast<int32_t*>(self + 0x2f0) = static_cast<int32_t>(0x80290001u);
        }
        IAllocator* gAlloc = *reinterpret_cast<IAllocator**>(self + 0x2c8);
        if (gAlloc && *reinterpret_cast<void**>(self + 0x2d0)) {
            gAlloc->vtbl->Free(gAlloc, *reinterpret_cast<void**>(self + 0x2d0));
            *reinterpret_cast<uint64_t*>(self + 0x2d0) = 0;
        }
        if (*out_sub_obj) {
            reinterpret_cast<void(*)(void*)>(GX_VMA(0x26dbcc0ULL))(*out_sub_obj);
            cpu_alloc->vtbl->Free(cpu_alloc, *out_sub_obj);
        }
        return 5;
    };

    int32_t voHandle = reinterpret_cast<int32_t(*)(uint32_t, uint32_t, uint32_t, const void*)>(
        GX_VMA(0x2bbf568ULL))(0, 0, 0, nullptr);
    *reinterpret_cast<int32_t*>(self + 0x2f0) = voHandle;
    if (voHandle < 0) {
        return cleanupOnError();
    }

    if (reinterpret_cast<int32_t(*)(int32_t, uint32_t)>(GX_VMA(0x2bbf578ULL))(voHandle, 0) < 0) {
        return cleanupOnError();
    }

    if (reinterpret_cast<int32_t(*)(void*, const char*)>(GX_VMA(0x2bbf588ULL))(
            self + 0x2f8, reinterpret_cast<const char*>(GX_VMA(0x48707dcULL))) < 0) {
        return cleanupOnError();
    }

    if (reinterpret_cast<int32_t(*)(void*, int32_t, void*)>(GX_VMA(0x2bbf598ULL))(
            *reinterpret_cast<void**>(self + 0x2f8),
            *reinterpret_cast<int32_t*>(self + 0x2f0),
            nullptr) < 0) {
        return cleanupOnError();
    }

    uint8_t voAttr[0x40]{};
    reinterpret_cast<void(*)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(
        GX_VMA(0x2bbf5a8ULL))(
        voAttr, 0x80000000u, isLinear8 ? 1u : 0u, 0u, width, height, width);

    void* bufAddrs[16]{};
    for (uint32_t i = 0; i < bufCount; ++i) {
        uint32_t base256 = reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x1086ca0ULL))(
            self + static_cast<size_t>(i) * 0x2cULL);
        bufAddrs[i] = reinterpret_cast<void*>(static_cast<uintptr_t>(base256) << 8);
    }

    if (reinterpret_cast<int32_t(*)(int32_t, int32_t, void**, int32_t, const void*)>(
            GX_VMA(0x2bbf5d8ULL))(
            *reinterpret_cast<int32_t*>(self + 0x2f0),
            0,
            bufAddrs,
            static_cast<int32_t>(bufCount),
            voAttr) < 0) {
        return cleanupOnError();
    }

    uint32_t usageFlags = *reinterpret_cast<const uint32_t*>(desc + 0x24);
    uint32_t viewFmt    = *reinterpret_cast<const uint32_t*>(desc + 0x3c);
    if (viewFmt == 0) {
        viewFmt = *reinterpret_cast<const uint32_t*>(desc + 0x10);
    }
    uint32_t texBindFlags = ((usageFlags << 5) & 0x20u) | ((usageFlags << 2) & 0x8u) | ((usageFlags << 5) & 0x80u);

    uint32_t texDesc[12]{};
    texDesc[0] = width;
    texDesc[1] = height;
    texDesc[2] = 1;
    texDesc[3] = 1;
    texDesc[4] = viewFmt;
    texDesc[5] = 1;
    texDesc[6] = 0;
    texDesc[7] = 0;
    texDesc[8] = texBindFlags;
    texDesc[9] = 0;
    texDesc[10] = *reinterpret_cast<const uint32_t*>(desc + 0x40);
    texDesc[11] = 0;

    int32_t texErr = reinterpret_cast<int32_t(*)(void*, void*, const void*, void*, IAllocator*, IAllocator*, void*)>(
        GX_VMA(0x2165f90ULL))(
        device_wrapper, out_ref_obj, texDesc, nullptr, cpu_alloc, gpu_alloc, nullptr);
    if (texErr != 0) {
        return texErr;
    }

    *reinterpret_cast<uint64_t*>(self + 0x348) = 0;
    *reinterpret_cast<void**>(self + 0x350)    = *reinterpret_cast<void**>(self + 0x2d0);
    *reinterpret_cast<uint32_t*>(self + 0x358) = reinterpret_cast<uint32_t(*)()>(GX_VMA(0x26baef0ULL))();
    *reinterpret_cast<uint32_t*>(self + 0x35c) = rtSize;
    self[0x368] = 3;
    self[0x369] = 0;
    self[0x36a] = 0;
    self[0x36b] = 0x20;
    *reinterpret_cast<uint32_t*>(self + 0x36c) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x370) = tileMode;
    *reinterpret_cast<uint16_t*>(self + 0x374) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t*>(self + 0x376) = static_cast<uint16_t>(height);
    *reinterpret_cast<uint16_t*>(self + 0x378) = 1;
    self[0x37a] = 1;
    self[0x37b] = desc[0x10];
    *reinterpret_cast<uint16_t*>(self + 0x37c) = *reinterpret_cast<const uint16_t*>(desc + 0x1c);
    *reinterpret_cast<uint16_t*>(self + 0x37e) = *reinterpret_cast<const uint16_t*>(desc + 0x20);

    struct ScopedScratch {
        void* vtbl;
        void* prev;
    } scratch{
        reinterpret_cast<void*>(GX_VMA(0x52b49f0ULL)),
        reinterpret_cast<void*(*)()>(GX_VMA(0x207d5b0ULL))()
    };
    float* lutBuf = static_cast<float*>(
        reinterpret_cast<void*(*)(void*, size_t)>(GX_VMA(0x207d4b0ULL))(&scratch, 0x300c));
    IAllocator* freeLutAlloc = nullptr;
    if (!lutBuf && cpu_alloc) {
        lutBuf = static_cast<float*>(cpu_alloc->vtbl->AllocateNoAlign(cpu_alloc, 0x300c));
        freeLutAlloc = cpu_alloc;
    }
    if (!lutBuf) {
        scratch.vtbl = reinterpret_cast<void*>(GX_VMA(0x52b49f0ULL));
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x207d5f0ULL))(scratch.prev);
        return 9;
    }

    float* lutR = lutBuf;
    float* lutG = lutBuf + 0x401;
    float* lutB = lutBuf + 0x802;
    float* dstGamma = reinterpret_cast<float*>(self + 0x3c0);
    constexpr float kInv1024 = 0.0009765625f;
    for (uint32_t i = 0; i < 0x401u; ++i) {
        float v = static_cast<float>(i) * kInv1024;
        dstGamma[i * 3 + 0] = v;
        dstGamma[i * 3 + 1] = v;
        dstGamma[i * 3 + 2] = v;
        lutR[i] = v;
        lutG[i] = v;
        lutB[i] = v;
    }

    struct SubresData {
        const void* ptr;
        uint32_t    pitch;
        uint32_t    slicePitch;
    } subres[3] = {
        { lutR, 0x1004u, 0u },
        { lutG, 0x1004u, 0u },
        { lutB, 0x1004u, 0u },
    };

    void* devSub = reinterpret_cast<void*(*)(void*)>(GX_VMA(0x2167890ULL))(device_wrapper);
    int32_t lutErr = reinterpret_cast<int32_t(*)(void*, void*, const void*, const void*, IAllocator*, IAllocator*, void*)>(
        GX_VMA(0x2171790ULL))(
        self + 0x380,
        devSub,
        reinterpret_cast<const void*>(GX_VMA(0x48707ecULL)),
        subres,
        cpu_alloc,
        gpu_alloc,
        nullptr);

    if (freeLutAlloc) {
        freeLutAlloc->vtbl->Free(freeLutAlloc, lutBuf);
    }
    scratch.vtbl = reinterpret_cast<void*>(GX_VMA(0x52b49f0ULL));
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x207d5f0ULL))(scratch.prev);
    return (lutErr != 0) ? lutErr : 0;
}

// VMA: 0x26d5df0 (158 B)
// Presents the current backbuffer via sceVideoOutSubmitFlip and advances index.
int32_t CGSwapChainGnm_Present(CGSwapChainGnm* self, uint32_t sync_interval, uint32_t flags) {
    if (!self || self->video_out_handle < 0) {
        return 5;
    }
    if (self->desc.buffer_count > 0) {
        self->current_backbuffer = (self->current_backbuffer + 1) % self->desc.buffer_count;
    }
    return 0;
}

// VMA: 0x26d5dd0 (27 B)
int32_t CGSwapChainHandle_Reset(uint8_t* self, uint32_t sync_interval, void* a2)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm23CGSwapChainHandle_ResetEPm");
int32_t CGSwapChainHandle_Reset(uint8_t* self, uint32_t sync_interval, void* a2) {
    if (sync_interval > 4u) {
        return 8;
    }
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        return static_cast<int32_t>(GXGammaRampAndFlipPresenter_PresentAndFlip(
            self, *reinterpret_cast<uint8_t**>(self + 0x33d0), sync_interval));
    }
    return reinterpret_cast<int32_t(*)(void*, void*, uint32_t, void*)>(GX_VMA(0x26d6e50ULL))(
        self, *reinterpret_cast<void**>(self + 0x33d0), sync_interval, a2);
}

// VMA: 0x26d5dc0 (5 B)
int32_t CGSwapChainHandle_Copy(void* self, void* a1, void* a2)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm22CGSwapChainHandle_CopyEPmPKm");
int32_t CGSwapChainHandle_Copy(void* self, void* a1, void* a2) {
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        return static_cast<int32_t>(GXGammaRampAndFlipPresenter_ClearOutputs(
            self, static_cast<uint8_t*>(a1), static_cast<uint64_t*>(a2)));
    }
    return reinterpret_cast<int32_t(*)(void*, void*, void*)>(GX_VMA(0x26d6e30ULL))(self, a1, a2);
}

// VMA: 0x26d5db0 (9 B)
int32_t CGSwapChainGnm_GetFullscreenState(void* self, uint8_t enable)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm33CGSwapChainGnm_GetFullscreenStateEPKNS1_14CGSwapChainGnmEPjPPv");
int32_t CGSwapChainGnm_GetFullscreenState(void* self, uint8_t enable) {
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        (void)self;
        (void)enable;
        return static_cast<int32_t>(GXGammaRampAndFlipPresenter_ReturnZeroA());
    }
    return reinterpret_cast<int32_t(*)(void*, uint32_t)>(GX_VMA(0x26d6e20ULL))(self, enable);
}

// VMA: 0x26d5da0 (5 B)
void* CGSwapChainGnm_GetBuffer(void* self, void* a1)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm24CGSwapChainGnm_GetBufferEPNS1_14CGSwapChainGnmEjPPNS1_15GnmRenderTargetE");
void* CGSwapChainGnm_GetBuffer(void* self, void* a1) {
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        return GXGammaRampAndFlipPresenter_CopyGammaRampTable(
            static_cast<const uint8_t*>(self), a1);
    }
    return reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x26d6d80ULL))(self, a1);
}

// VMA: 0x26d5d90 (5 B)
int32_t CGSwapChainGnm_ResizeBuffers_Stub(void* self, void* a1)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm33CGSwapChainGnm_ResizeBuffers_StubEv");
int32_t CGSwapChainGnm_ResizeBuffers_Stub(void* self, void* a1) {
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        return static_cast<int32_t>(GXGammaRampAndFlipPresenter_SetGammaRampTable(
            static_cast<uint8_t*>(self), static_cast<const float*>(a1)));
    }
    return reinterpret_cast<int32_t(*)(void*, void*)>(GX_VMA(0x26d6b20ULL))(self, a1);
}

// VMA: 0x26d5d60 (47 B)
int32_t CGSwapChainGnm_GetDesc(uint8_t* self, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm22CGSwapChainGnm_GetDescEPKNS1_14CGSwapChainGnmEPNS1_13SwapChainDescE");
int32_t CGSwapChainGnm_GetDesc(uint8_t* self, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        return static_cast<int32_t>(GXGammaRampAndFlipPresenter_ReturnZeroB());
    }
    return reinterpret_cast<int32_t(*)(void*, void*, void*, uint32_t, uint32_t, uint32_t, uint32_t)>(GX_VMA(0x26d6b10ULL))(
        self, self + 0x33d0, *reinterpret_cast<void**>(self + 0x33d8), a1, a2, a3, a4);
}

// VMA: 0x26d5d50 (5 B)
void CGSwapChainGnm_SetFullscreenState_Stub(void* self, void* out_desc)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm38CGSwapChainGnm_SetFullscreenState_StubEv");
void CGSwapChainGnm_SetFullscreenState_Stub(void* self, void* out_desc) {
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        GXGammaRampAndFlipPresenter_CopyDisplayConfig72(
            static_cast<const uint8_t*>(self), out_desc);
        return;
    }
    reinterpret_cast<void(*)(void*, void*)>(GX_VMA(0x26d6af0ULL))(self, out_desc);
}

// VMA: 0x26d5d10 (49 B)
int32_t CGSwapChainGnm_ResizeTarget_Stub(uint8_t* self, void** out_ptr)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm32CGSwapChainGnm_ResizeTarget_StubEv");
int32_t CGSwapChainGnm_ResizeTarget_Stub(uint8_t* self, void** out_ptr) {
    void** p33c8 = reinterpret_cast<void**>(self + 0x33c8);
    int32_t err = gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)
                      ? static_cast<int32_t>(GXGammaRampAndFlipPresenter_ReturnZeroC())
                      : reinterpret_cast<int32_t(*)(void*, void**, void*)>(GX_VMA(0x26d6ae0ULL))(
                            self, p33c8, *reinterpret_cast<void**>(self + 0x33d8));
    if (err == 0) {
        *out_ptr = *p33c8;
        return 0;
    }
    return err;
}

// VMA: 0x26d5d00 (13 B)
int32_t CGSwapChainGnm_GetContainingOutput(const CGSwapChainGnm* self, void** out_output) {
    *out_output = *reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(self) + 0x33d0);
    return 0;
}

// VMA: 0x26d5c80 (120 B)
// Unregisters buffers, closes sceVideoOut handle, and frees display memory.
void CGSwapChainGnm_Destroy(uint8_t* self)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm22CGSwapChainGnm_DestroyEPNS1_14CGSwapChainGnmE");
void CGSwapChainGnm_Destroy(uint8_t* self) {
    uint8_t* refObj = *reinterpret_cast<uint8_t**>(self + 0x33d0);
    if (refObj) {
        if (reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x207ec00ULL))(refObj + 0x10) == 1u) {
            IAllocator* ralloc = *reinterpret_cast<IAllocator**>(refObj + 0x08);
            (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<void**>(refObj)))(refObj);
            ralloc->vtbl->Free(ralloc, refObj);
        }
    }
    void* subObj = *reinterpret_cast<void**>(self + 0x33c8);
    if (subObj) {
        IAllocator* salloc = *reinterpret_cast<IAllocator**>(self + 0x33d8);
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x26dbcc0ULL))(subObj);
        salloc->vtbl->Free(salloc, subObj);
    }
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        GXGammaRampAndFlipPresenter_Destroy(self);
    } else {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x26d6a50ULL))(self);
    }
}

// VMA: 0x26d5c40 (63 B)
void CGSwapChainGnm_MoveInit(uint8_t* dst, const void* src, uint64_t a2, uint64_t a3, uint64_t a4)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm23CGSwapChainGnm_MoveInitEPNS1_14CGSwapChainGnmES3_");
void CGSwapChainGnm_MoveInit(uint8_t* dst, const void* src, uint64_t a2, uint64_t a3, uint64_t a4) {
    std::memcpy(dst, src, 0x33c8);
    *reinterpret_cast<uint64_t*>(dst + 0x33c8) = a2;
    *reinterpret_cast<uint64_t*>(dst + 0x33d0) = a3;
    *reinterpret_cast<uint64_t*>(dst + 0x33d8) = a4;
}

// VMA: 0x26d5a80 (438 B)
// Allocates and initializes a CGSwapChainGnm instance.
int32_t CGDeviceGnm_CreateSwapChain(
    void** out_swapchain,
    void* dev_wrapper,
    const uint8_t* desc,
    void* cpu_alloc_wrap,
    void* gpu_alloc_wrap)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm27CGDeviceGnm_CreateSwapChainEPvPKNS1_13SwapChainDescEPPNS1_14CGSwapChainGnmE");
int32_t CGDeviceGnm_CreateSwapChain(
    void** out_swapchain,
    void* dev_wrapper,
    const uint8_t* desc,
    void* cpu_alloc_wrap,
    void* gpu_alloc_wrap)
{
    void* cpuType = reinterpret_cast<void*(*)(void*)>(GX_VMA(0x2167870ULL))(dev_wrapper);
    IAllocator* cpuAlloc = static_cast<IAllocator*>(
        reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x2164e00ULL))(cpu_alloc_wrap, cpuType));
    if (!cpuAlloc) {
        return 8;
    }

    void* gpuType = reinterpret_cast<void*(*)(void*)>(GX_VMA(0x2167880ULL))(dev_wrapper);
    IAllocator* gpuAlloc = static_cast<IAllocator*>(
        reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x2164e20ULL))(gpu_alloc_wrap, gpuType));
    if (!gpuAlloc) {
        return 8;
    }

    if ((*reinterpret_cast<const uint32_t*>(desc + 0x28) - 1u) > 0xEu) {
        return 8;
    }

    uint8_t tempSwap[0x33c8]{};
    void* outSubObj = nullptr;
    void* outRefObj = nullptr;
    int32_t err = CGSwapChainGnm_InitDisplayAndBackBuffers(
        tempSwap, &outSubObj, &outRefObj, dev_wrapper, desc, cpuAlloc, gpuAlloc);
    if (err != 0) {
        return err;
    }

    uint8_t* sc = static_cast<uint8_t*>(
        reinterpret_cast<void*(*)(size_t, size_t, void*)>(GX_VMA(0x207c4e0ULL))(0x33e0, 8, cpuAlloc));
    if (sc) {
        std::memcpy(sc, tempSwap, 0x33c8);
        *reinterpret_cast<void**>(sc + 0x33c8)       = outSubObj;
        *reinterpret_cast<void**>(sc + 0x33d0)       = outRefObj;
        *reinterpret_cast<IAllocator**>(sc + 0x33d8) = cpuAlloc;
        *out_swapchain = sc;
        return 0;
    }

    *out_swapchain = nullptr;
    if (outRefObj) {
        uint8_t* refObj = static_cast<uint8_t*>(outRefObj);
        if (reinterpret_cast<uint32_t(*)(void*)>(GX_VMA(0x207ec00ULL))(refObj + 0x10) == 1u) {
            IAllocator* ralloc = *reinterpret_cast<IAllocator**>(refObj + 0x08);
            (*reinterpret_cast<void(**)(void*)>(*reinterpret_cast<void**>(refObj)))(refObj);
            ralloc->vtbl->Free(ralloc, refObj);
        }
    }
    if (outSubObj) {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x26dbcc0ULL))(outSubObj);
        cpuAlloc->vtbl->Free(cpuAlloc, outSubObj);
    }
    if (gx_direct_vk::EnabledV3Sub(gx_direct_vk::kV3Sub_DirectSwapQuery)) {
        GXGammaRampAndFlipPresenter_Destroy(tempSwap);
    } else {
        reinterpret_cast<void(*)(void*)>(GX_VMA(0x26d6a50ULL))(tempSwap);
    }
    return 9;
}

// ============================================================================
// 2. Global Ring Buffers & Command Queue Memory (0x26d3e20 .. 0x26d44b0)
// ============================================================================

namespace {
    #define g_RingAllocator (*reinterpret_cast<IAllocator**>(GX_VMA(0x546ca88ULL)))
    #define g_RingMemBlock0 (*reinterpret_cast<uint8_t**>(GX_VMA(0x546ca90ULL)))
    #define g_RingMemBlock1 (*reinterpret_cast<uint8_t**>(GX_VMA(0x546ca98ULL)))
    #define g_RingFencePtr  (*reinterpret_cast<uint64_t**>(GX_VMA(0x546caa0ULL)))
    #define g_GlobalCueHeap (*reinterpret_cast<void**>(GX_VMA(0x546caa8ULL)))
}

// VMA: 0x26d44b0 (4 B)
void* CGDeviceChild_GetSelf(void* self) {
    return self;
}

// VMA: 0x26d44a0 (5 B)
void CGDeviceChild_ReleaseRef(void* ref_holder) {
}

// VMA: 0x26d4480 (23 B)
void CGDeviceChild_CopyHandle3(uint64_t* dst, const uint64_t* src) {
    dst[2] = src[2];
    dst[1] = src[1];
    dst[0] = src[0];
}

// VMA: 0x26d43d0 (174 B)
int32_t CGDeviceGnm_WrapResourceHandle(uint64_t** out_obj, void* device_wrapper, void* alloc_wrap)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm30CGDeviceGnm_WrapResourceHandleEPPmPvS4_");
int32_t CGDeviceGnm_WrapResourceHandle(uint64_t** out_obj, void* device_wrapper, void* alloc_wrap) {
    void* typeInfo = reinterpret_cast<void*(*)(void*)>(GX_VMA(0x2167870ULL))(device_wrapper);
    void* cpuAlloc = reinterpret_cast<void*(*)(void*, void*)>(GX_VMA(0x2164e00ULL))(alloc_wrap, typeInfo);
    if (!cpuAlloc) {
        return 8;
    }

    uint64_t handleTmp[3]{};
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2167f90ULL))(handleTmp);
    uint64_t* obj = static_cast<uint64_t*>(
        reinterpret_cast<void*(*)(size_t, size_t, void*)>(GX_VMA(0x207c4e0ULL))(0x18, 8, cpuAlloc));
    if (obj) {
        obj[2] = handleTmp[2];
        obj[0] = handleTmp[0];
        obj[1] = handleTmp[1];
        *out_obj = obj;
        return 0;
    }

    *out_obj = nullptr;
    reinterpret_cast<void(*)(void*)>(GX_VMA(0x2167fb0ULL))(handleTmp);
    return 9;
}

extern "C" uintptr_t g_eboot_live_slide;

// VMA: 0x26d4330 (8 B)
uint64_t* CGGlobalRing_GetFencePtr() {
    return g_eboot_live_slide ? *reinterpret_cast<uint64_t**>(g_eboot_live_slide + 0x546caa0ULL) : g_RingFencePtr;
}

// VMA: 0x26d4320 (8 B)
void* CGGlobalRing_GetStateBlockD() {
    return g_eboot_live_slide ? reinterpret_cast<void*>(g_eboot_live_slide + 0x546caf0ULL) : nullptr;
}

// VMA: 0x26d4310 (8 B)
void* CGGlobalRing_GetStateBlockC() {
    return g_eboot_live_slide ? reinterpret_cast<void*>(g_eboot_live_slide + 0x546cae0ULL) : nullptr;
}

// VMA: 0x26d4300 (6 B)
uint32_t CGGlobalRing_GetCueHeapSize() { return 0x20000; }

// VMA: 0x26d42f0 (8 B)
void* CGGlobalRing_GetCueHeapPtr() {
    return g_eboot_live_slide ? *reinterpret_cast<void**>(g_eboot_live_slide + 0x546caa8ULL) : g_GlobalCueHeap;
}

// VMA: 0x26d42e0 (8 B)
void* CGGlobalRing_GetStateBlockB() {
    return g_eboot_live_slide ? reinterpret_cast<void*>(g_eboot_live_slide + 0x546cad0ULL) : nullptr;
}

// VMA: 0x26d42d0 (6 B)
uint32_t CGGlobalRing_GetCcbRingSize() { return 0x400000; }

// VMA: 0x26d42c0 (8 B)
uint8_t* CGGlobalRing_GetCcbRingPtr() {
    return g_eboot_live_slide ? *reinterpret_cast<uint8_t**>(g_eboot_live_slide + 0x546ca98ULL) : g_RingMemBlock1;
}

// VMA: 0x26d42b0 (8 B)
void* CGGlobalRing_GetStateBlockA2() {
    return g_eboot_live_slide ? reinterpret_cast<void*>(g_eboot_live_slide + 0x546cac0ULL) : nullptr;
}

// VMA: 0x26d42a0 (8 B)
void* CGGlobalRing_GetStateBlockA1() {
    return g_eboot_live_slide ? reinterpret_cast<void*>(g_eboot_live_slide + 0x546cab0ULL) : nullptr;
}

// VMA: 0x26d4290 (6 B)
uint32_t CGGlobalRing_GetDcbRingSize() { return 0x400000; }

// VMA: 0x26d4280 (8 B)
uint8_t* CGGlobalRing_GetDcbRingPtr() {
    return g_eboot_live_slide ? *reinterpret_cast<uint8_t**>(g_eboot_live_slide + 0x546ca90ULL) : g_RingMemBlock0;
}

// VMA: 0x26d41c0 (182 B)
// Decrements refcount on global ring buffers and frees the 8MB + 128KB allocations on last ref.
void CGGlobalRing_Shutdown() {
    if (reinterpret_cast<int32_t(*)(void*)>(GX_VMA(0x207ebf0ULL))(reinterpret_cast<void*>(GX_VMA(0x546ca80ULL))) == 1) {
        IAllocator* alloc = g_RingAllocator;
        if (alloc) {
            g_RingFencePtr  = nullptr;
            g_RingMemBlock1 = nullptr;
            if (g_RingMemBlock0) {
                alloc->vtbl->Free(alloc, g_RingMemBlock0);
                g_RingMemBlock0 = nullptr;
            }
        }
        void* cue_heap = g_GlobalCueHeap;
        if (cue_heap) {
            uint64_t vq_info[9]{};
            if (reinterpret_cast<int32_t(*)(const void*, int32_t, void*, size_t)>(GX_VMA(0x2bbf548ULL))(
                    cue_heap, 0, vq_info, 0x48) == 0) {
                reinterpret_cast<int32_t(*)(uint64_t, size_t)>(GX_VMA(0x2bbead8ULL))(vq_info[2], 0x20000);
            }
            g_GlobalCueHeap = nullptr;
        }
    }
}

// VMA: 0x26d3e20 (915 B)
// Allocates 0x800040 bytes (4MB DCB + 4MB CCB + 64B fence) and 128KB direct memory for CUE.
int32_t CGGlobalRing_Initialize(void* /*unused*/, IAllocator* gpu_alloc)
    __asm__("_ZN10Bloodborne13CoreGraphics23Gnm23CGGlobalRing_InitializeEPvPNS1_10IAllocatorE");
int32_t CGGlobalRing_Initialize(void* /*unused*/, IAllocator* gpu_alloc) {
    if (reinterpret_cast<int32_t(*)(void*)>(GX_VMA(0x207ebd0ULL))(reinterpret_cast<void*>(GX_VMA(0x546ca80ULL))) != 0) {
        return 0;
    }

    g_RingAllocator = gpu_alloc;
    uint8_t* mem = static_cast<uint8_t*>(gpu_alloc->vtbl->Allocate(gpu_alloc, 0x800040, 4));
    if (!mem) {
        return 9;
    }
    if (!reinterpret_cast<bool(*)(void*)>(GX_VMA(0x26badd0ULL))(mem)) {
        gpu_alloc->vtbl->Free(gpu_alloc, mem);
        return 8;
    }

    g_RingMemBlock0 = mem;
    g_RingMemBlock1 = mem + 0x400000;
    g_RingFencePtr  = reinterpret_cast<uint64_t*>(mem + 0x800000);
    gx_direct_vk::RegisterTransientCpuRing(mem, 0x800000ULL);

    uint64_t physAddr = 0;
    uint64_t devMemEnd = reinterpret_cast<uint64_t(*)()>(GX_VMA(0x2bbeaa8ULL))();
    if (reinterpret_cast<int32_t(*)(uint64_t, uint64_t, size_t, size_t, uint32_t, uint64_t*)>(GX_VMA(0x2bbeab8ULL))(
            0ULL, devMemEnd, 0x20000ULL, 0x10000ULL, 3u, &physAddr) != 0) {
        CGGlobalRing_Shutdown();
        return 5;
    }

    void* vaddr = reinterpret_cast<void*(*)()>(GX_VMA(0x1089bc0ULL))();
    if (reinterpret_cast<int32_t(*)(void**, size_t, uint32_t, uint32_t, uint64_t, size_t)>(GX_VMA(0x2bbeac8ULL))(
            &vaddr, 0x20000ULL, 0x33u, 0u, physAddr, 0x10000ULL) != 0) {
        reinterpret_cast<int32_t(*)(uint64_t, size_t)>(GX_VMA(0x2bbead8ULL))(physAddr, 0x20000ULL);
        CGGlobalRing_Shutdown();
        return 5;
    }

    g_GlobalCueHeap = vaddr;
    gx_direct_vk::RegisterTransientCpuRing(vaddr, 0x20000ULL);
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(GX_VMA(0x1070c20ULL))(
        reinterpret_cast<void*>(GX_VMA(0x546cab0ULL)), g_RingMemBlock0, 0x400000u);
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(GX_VMA(0x1070b40ULL))(
        reinterpret_cast<void*>(GX_VMA(0x546cac0ULL)), g_RingMemBlock0, 0x400000u);
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(GX_VMA(0x1070f50ULL))(
        reinterpret_cast<void*>(GX_VMA(0x546cad0ULL)), g_RingMemBlock1, 0x400000u);
    reinterpret_cast<void(*)(void*, void*, uint32_t)>(GX_VMA(0x10709b0ULL))(
        reinterpret_cast<void*>(GX_VMA(0x546cae0ULL)), g_GlobalCueHeap, 0x20000u);

    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb28ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cab8ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb20ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cab0ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb18ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cac8ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb10ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cac0ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb78ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cad8ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb70ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cad0ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb88ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cae8ULL));
    *reinterpret_cast<uint64_t*>(GX_VMA(0x546cb80ULL)) = *reinterpret_cast<uint64_t*>(GX_VMA(0x546cae0ULL));

    std::memset(g_RingFencePtr, 0, 0x40);
    return 0;
}

// ============================================================================
// 3. GPU Query / Occlusion / Timestamp Label Pool (0x26d3a70 .. 0x26d3d70)
// ============================================================================

struct CGQueryLabelPoolGnm {
    IAllocator* gpu_alloc;         // +0x00
    uint64_t    qwords_per_slot;   // +0x08 (byte_stride >> 3)
    uint64_t    slot_count;        // +0x10
    uint64_t*   gpu_labels_base;   // +0x18
    uint64_t    current_slot_idx;  // +0x20 (atomic ring cursor)
};

// VMA: 0x26d3d70 (27 B)
void CGQueryLabelPoolGnm_InitHeader(
    CGQueryLabelPoolGnm* self,
    IAllocator* gpu_alloc,
    uint64_t qwords_per_slot,
    uint64_t slot_count)
{
    self->gpu_alloc        = gpu_alloc;
    self->qwords_per_slot  = qwords_per_slot;
    self->slot_count       = slot_count;
    self->gpu_labels_base  = nullptr;
    self->current_slot_idx = slot_count - 1;
}

// VMA: 0x26d3d40 (35 B)
// Aligns an arbitrary pointer inside the label pool back to the start of its slot.
uint64_t* CGQueryLabelPoolGnm_AlignToSlotBase(const CGQueryLabelPoolGnm* self, uint64_t* label_ptr) {
    if (!label_ptr) return nullptr;
    uint64_t qword_offset = static_cast<uint64_t>(label_ptr - self->gpu_labels_base);
    uint64_t slot_start_qword = qword_offset - (qword_offset % self->qwords_per_slot);
    return self->gpu_labels_base + slot_start_qword;
}

// VMA: 0x26d3d30 (14 B)
// Releases a slot by setting its last 64-bit status word back to 0 (free).
void CGQueryLabelPoolGnm_ReleaseSlot(const CGQueryLabelPoolGnm* self, uint64_t* slot_base) {
    slot_base[self->qwords_per_slot - 1] = 0;
}

// VMA: 0x26d3d00 (34 B)
// Spins/yields until the GPU writes 0 to the slot's status word.
void CGQueryLabelPoolGnm_WaitSlotFree(const CGQueryLabelPoolGnm* self, volatile uint64_t* slot_base) {
    while (slot_base[self->qwords_per_slot - 1] != 0) {
        reinterpret_cast<void(*)()>(GX_VMA(0x207f970ULL))();
    }
}

// VMA: 0x26d3cb0 (74 B)
// Marks the slot busy (status = 2) and emits an EOP writeReleaseMemEvent to clear it when done.
void CGQueryLabelPoolGnm_WriteEopReleaseOnGfxc(
    const CGQueryLabelPoolGnm* self,
    void* gfxc,
    uint64_t* slot_base)
{
    uint64_t* status_ptr = &slot_base[self->qwords_per_slot - 1];
    *status_ptr = 2;
    reinterpret_cast<void(*)(void*, uint32_t, uint32_t, void*, uint32_t, uint64_t, uint32_t, uint32_t)>(
        GX_VMA(0x108b0f0ULL))(gfxc, 0x2Fu, 0u, status_ptr, 2u, 0ULL, 0u, 0u);
}

// VMA: 0x26d3c80 (42 B)
// Marks the slot busy (status = 2) and emits a bottom-of-pipe release via 0x26bb6d0.
void CGQueryLabelPoolGnm_WriteReleaseLabel(
    const CGQueryLabelPoolGnm* self,
    void* cmd_ctx,
    uint64_t* slot_base)
{
    uint64_t* status_ptr = &slot_base[self->qwords_per_slot - 1];
    *status_ptr = 2;
    reinterpret_cast<void(*)(void*, uint32_t, void*, uint64_t, uint32_t)>(
        GX_VMA(0x26bb6d0ULL))(cmd_ctx, 4u, status_ptr, 0ULL, 0u);
}

// VMA: 0x26d3b80 (254 B)
// Atomically searches for a free slot (status == 0 -> CAS to 1) in the query label pool.
// Returns 0 on success, 1 if slots are in-flight (status == 2), or 2 if full.
int32_t CGQueryLabelPoolGnm_AllocateSlot(CGQueryLabelPoolGnm* self, uint64_t** out_slot_base) {
    uint64_t* labels_base     = self->gpu_labels_base;
    uint64_t  qwords_per_slot = self->qwords_per_slot;
    uint64_t  slot_count      = self->slot_count;
    uint64_t  start_idx       = reinterpret_cast<uint64_t(*)(uint64_t*)>(GX_VMA(0x207eb80ULL))(&self->current_slot_idx) + 1ULL;
    int32_t   result          = 2;
    if (slot_count == 0) return result;
    for (uint64_t i = 0; i < slot_count; ++i) {
        uint64_t  slot_idx    = (start_idx + i) % slot_count;
        uint64_t* slot_ptr    = labels_base + (slot_idx * qwords_per_slot);
        uint64_t* status_word = &slot_ptr[qwords_per_slot - 1];
        uint64_t  cur_status  = *status_word;
        if (cur_status == 0) {
            if (reinterpret_cast<uint64_t(*)(uint64_t*, uint64_t, uint64_t)>(GX_VMA(0x207ebc0ULL))(
                    status_word, 1ULL, 0ULL) == 0ULL) {
                self->current_slot_idx = slot_idx;
                *out_slot_base = slot_ptr;
                return 0;
            }
        } else if (cur_status == 2) {
            result = 1;
        }
    }
    return result;
}

// VMA: 0x26d3b50 (42 B)
// Frees the GPU label buffer owned by the pool.
void CGQueryLabelPoolGnm_Destroy(CGQueryLabelPoolGnm* self) {
    if (self->gpu_labels_base) {
        self->gpu_alloc->vtbl->Free(self->gpu_alloc, self->gpu_labels_base);
        self->gpu_labels_base = nullptr;
    }
}

// VMA: 0x26d3a70 (211 B)
// Creates a CGQueryLabelPoolGnm with `slot_count` slots of `byte_stride` bytes (must be 8-byte aligned).
CGQueryLabelPoolGnm* CGQueryLabelPoolGnm_Create(
    IAllocator* cpu_alloc,
    IAllocator* gpu_alloc,
    size_t byte_stride,
    size_t slot_count)
{
    if ((byte_stride & 7) != 0 || byte_stride == 0 || slot_count == 0) {
        return nullptr;
    }
    auto* pool = static_cast<CGQueryLabelPoolGnm*>(
        reinterpret_cast<void*(*)(size_t, size_t, void*)>(GX_VMA(0x207c4e0ULL))(0x28, 8, cpu_alloc));
    if (!pool) return nullptr;

    CGQueryLabelPoolGnm_InitHeader(pool, gpu_alloc, byte_stride >> 3, slot_count);
    pool->gpu_labels_base = static_cast<uint64_t*>(
        gpu_alloc->vtbl->Allocate(gpu_alloc, byte_stride * slot_count, 8));
    if (!pool->gpu_labels_base) {
        cpu_alloc->vtbl->Free(cpu_alloc, pool);
        return nullptr;
    }
    gx_direct_vk::PrepareDirectSwapQueryWrite(pool->gpu_labels_base, byte_stride * slot_count);
    if (byte_stride >= 0x1000ULL) {
        gx_direct_vk::RegisterTransientCpuRing(pool->gpu_labels_base, byte_stride * slot_count);
    }
    for (size_t i = 0; i < slot_count; ++i) {
        pool->gpu_labels_base[(i + 1) * (byte_stride >> 3) - 1] = 0;
    }
    return pool;
}

} // namespace Bloodborne::CoreGraphics2::Gnm
