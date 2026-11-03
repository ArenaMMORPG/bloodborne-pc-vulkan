// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Module 02: GX Graphics Framework
// File: GXStatePresetCacheAndDebugProps.cpp
// VMA Range: 0x21ae2f0 .. 0x21b29f0 (10 functions, 18,127 bytes)
// ============================================================================
// Architectural Overview:
//   1. GX State Preset Cache (`GXStatePresetCache`, 0x21aeca0 .. 0x21b29f0)
//      - Pre-allocates and caches all built-in pipeline state objects used by
//        the GX rendering engine:
//          * 57 Blend State presets (`+0x000 .. +0x1C7`, 0x128 bytes each)
//          *  6 Depth-Stencil State presets (`+0x1C8 .. +0x1F7`, 0x44 bytes each)
//          * 24 Rasterizer State presets (`+0x1F8 .. +0x2B7`, 0x34 bytes each)
//          * 19 Sampler State presets (`+0x2B8 .. +0x34F`, 0x40 bytes each)
//          * 19 Shadow Sampler Descriptors (`+0x350 .. +0x78F`, 0x38 bytes each)
//      - `GXStatePresetCache_UpdateSamplerAnisotropyPresets` (`0x21b2870`):
//        Dynamically updates `maxAnisotropy` across the anisotropic sampler
//        presets and recreates modified `CGSamplerStateGnm` objects on the fly.
//      - `GXDebugMenu_CleanupSamplerPresetInspectors` (`0x21b29f0`):
//        Releases ref-counted debug menu property nodes and `DLWString` names
//        for the 19 sampler state presets.
//   2. `DLVector<void*>` Pointer-Vector Template Helpers (`0x21ae2f0 .. 0x21aeba0`)
//      - Six template instantiations of `DLVector<T*>::Reallocate(size_t newCap)`
//        (`0x21ae6a0`, `0x21ae7a0`, `0x21ae8a0`, `0x21ae9a0`, `0x21aeaa0`,
//        `0x21aeba0`) and `DLVector<T*>::MoveAssign` (`0x21ae2f0`).
// ============================================================================

#include <cstdint>
#include <cstring>
#include <algorithm>

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

struct GXSamplerDesc {
    uint32_t filterMode;     // +0x00
    uint32_t addressU;       // +0x04
    uint32_t addressV;       // +0x08
    uint32_t addressW;       // +0x0C
    float    mipLodBias;     // +0x10
    uint32_t maxAnisotropy;  // +0x14
    uint32_t comparisonFunc; // +0x18
    float    borderColor[4]; // +0x1C .. +0x2B
    float    minLod;         // +0x2C
    float    maxLod;         // +0x30
    uint32_t flags;          // +0x34
}; // sizeof == 0x38 (56 bytes)

struct GXStatePresetCache {
    void*         blendStates[57];        // +0x000 .. +0x1C7 (0x39 entries)
    void*         depthStencilStates[6];  // +0x1C8 .. +0x1F7 (6 entries)
    void*         rasterizerStates[24];   // +0x1F8 .. +0x2B7 (0x18 entries)
    void*         samplerStates[19];      // +0x2B8 .. +0x34F (0x13 entries)
    GXSamplerDesc samplerShadowDescs[19]; // +0x350 .. +0x78F (19 * 0x38 bytes)
};

extern "C" {
    DLAllocator* GXStateObject_GetAllocator(void* stateObj);                               // 0x207b720
    void         GXBlendState_InitFromDesc(void* outState, void* dev, const void* desc);   // 0x2178050
    void         GXDepthStencilState_InitFromDesc(void* out, void* dev, const void* d);    // 0x21782b0
    void         GXRasterizerState_InitFromDesc(void* out, void* dev, const void* d);      // 0x2178550
    void         GXSamplerState_InitFromDesc(void* out, void* dev, const GXSamplerDesc*);  // 0x2178750
    void         GXSamplerState_ExtractDesc(void* stateObj, GXSamplerDesc*);               // 0x2178ab0
}

// ============================================================================
// 1. GX State Preset Cache & Debug Menu Cleanup (0x21aeca0 .. 0x21b29f0)
// ============================================================================

struct GXRefCountedNode {
    struct VTable {
        void (*fn00)(GXRefCountedNode*);
        void (*fn08)(GXRefCountedNode*);
        void (*fn10)(GXRefCountedNode*);
        void (*OnZeroStrongRef)(GXRefCountedNode*); // +0x18
        void (*OnZeroWeakRef)(GXRefCountedNode*);   // +0x20
    };
    VTable*  vptr;      // +0x00
    uint32_t packedRef; // +0x08 (bits 0..14: strong ref, bits 15..29: weak ref, bit 30: valid)
};

static inline void GXRefCountedNode_ReleaseStrongAndWeak(GXRefCountedNode* node) {
    if (!node) {
        return;
    }
    uint32_t oldVal = node->packedRef;
    uint32_t strongDec = (oldVal + 0x7FFFu) & 0x7FFFu;
    uint32_t updated = (oldVal & 0xFFFF8000u) | strongDec;
    node->packedRef = updated;

    if (strongDec == 0 && (oldVal & 0x40000000u) != 0) {
        node->vptr->OnZeroStrongRef(node);
        updated = node->packedRef & 0xBFFFFFFFu;
        node->packedRef = updated;
    }

    uint32_t weakDec = ((updated >> 15) + 0x7FFFu) & 0x7FFFu;
    node->packedRef = (updated & 0xC0007FFFu) | (weakDec << 15);
    if (weakDec == 0) {
        node->packedRef = updated | 0x3FFF8000u;
        node->vptr->OnZeroWeakRef(node);
    }
}

struct GXDebugMenuWStrDesc {
    uint8_t*    pRefObj;      // +0x00
    const void* pStaticUtf16; // +0x08
};

static inline void GXDebugMenu_ReleaseWStrRefObj(uint8_t* obj) {
    if (!obj) {
        return;
    }
    if (--(*reinterpret_cast<uint32_t*>(obj + 0x38)) == 0u) {
        DLAllocator* strPool = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e10ULL));
        if (*reinterpret_cast<uint64_t*>(obj + 0x20) >= 8ULL) {
            void* buf = *reinterpret_cast<void**>(obj + 0x08);
            DLAllocator* bufAlloc = *reinterpret_cast<DLAllocator**>(obj + 0x28);
            bufAlloc->vptr->Free(bufAlloc, buf);
        }
        *reinterpret_cast<uint64_t*>(obj + 0x20) = 7ULL;
        *reinterpret_cast<uint64_t*>(obj + 0x18) = 0ULL;
        *reinterpret_cast<uint16_t*>(obj + 0x08) = 0u;
        strPool->vptr->Free(strPool, obj);
    }
}

static inline void GXDebugMenu_PopNodeFromStack(void* menuCtx) {
    auto** pBegin = reinterpret_cast<GXRefCountedNode***>(static_cast<uint8_t*>(menuCtx) + 0x08);
    auto** pEnd   = reinterpret_cast<GXRefCountedNode***>(static_cast<uint8_t*>(menuCtx) + 0x10);
    GXRefCountedNode** begin = *pBegin;
    GXRefCountedNode** end   = *pEnd;
    if (begin != end) {
        GXRefCountedNode* backNode = end[-1];
        if (backNode != nullptr) {
            GXRefCountedNode_ReleaseStrongAndWeak(backNode);
            end[-1] = nullptr;
            end = *pEnd;
        }
        *pEnd = end - 1;
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b29f0 (1,168 Bytes)
// Registers debug-menu inspector groups and sliders ("Sampler", "Aniso Debug",
// "  MaxAnisotropy", and per-preset "MaxAnisotropy" / "mipLODBias") for all 19
// sampler presets in `GXStatePresetCache` (%rdi = `self`, %rsi = `menuCtx`).
// Note: Parameter types `(void*, GXStatePresetCache*)` are preserved for ABI
// symbol mangling compatibility with `gx_decomp_registry.h`.
// ----------------------------------------------------------------------------
void GXDebugMenu_CleanupSamplerPresetInspectors(
    void* argRdi_self,
    GXStatePresetCache* argRsi_menuCtx)
{
    auto* self    = static_cast<GXStatePresetCache*>(argRdi_self);
    void* menuCtx = static_cast<void*>(argRsi_menuCtx);

    auto fnPushGroup = reinterpret_cast<void* (*)(void*, const void*, uint32_t)>(GX_VMA(0x236c900ULL));
    auto fnAddBool   = reinterpret_cast<void (*)(void*, const void*, void*)>(GX_VMA(0x292f830ULL));
    auto fnAddUInt32 = reinterpret_cast<void (*)(void*, const void*, void*, uint32_t, uint32_t, uint32_t, float)>(GX_VMA(0x29316a0ULL));
    auto fnAddFloat  = reinterpret_cast<void (*)(void*, const void*, void*, float, float, float, float)>(GX_VMA(0x292a590ULL));
    auto fnMakeWStr  = reinterpret_cast<void (*)(GXDebugMenuWStrDesc*, const void*)>(GX_VMA(0x231d3a0ULL));

    alignas(16) GXDebugMenuWStrDesc rootDesc{nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c3612ULL))}; // u"Sampler"
    void* rootNode = fnPushGroup(menuCtx, &rootDesc, 0u);
    rootDesc.pStaticUtf16 = nullptr;
    if (rootNode == nullptr) {
        return;
    }

    alignas(16) GXDebugMenuWStrDesc anisoDbgDesc{nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c3622ULL))}; // u"Aniso Debug"
    fnAddBool(menuCtx, &anisoDbgDesc, reinterpret_cast<void*>(GX_VMA(0x56a6c25ULL)));
    anisoDbgDesc.pStaticUtf16 = nullptr;

    alignas(16) GXDebugMenuWStrDesc globalAnisoDesc{nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c363aULL))}; // u"  MaxAnisotropy"
    const float kOneF = *reinterpret_cast<const float*>(GX_VMA(0x492971cULL));
    fnAddUInt32(menuCtx, &globalAnisoDesc, reinterpret_cast<void*>(GX_VMA(0x513ac88ULL)), 0u, 16u, 1u, kOneF);
    globalAnisoDesc.pStaticUtf16 = nullptr;

    const auto* presetNames = reinterpret_cast<const void* const*>(GX_VMA(0x535fa20ULL));
    for (size_t i = 0; i < 19; ++i) {
        alignas(16) GXDebugMenuWStrDesc tmpStr{nullptr, nullptr};
        fnMakeWStr(&tmpStr, presetNames[i]);

        alignas(16) GXDebugMenuWStrDesc subDesc{nullptr, nullptr};
        uint8_t* retainedObj = nullptr;
        if (tmpStr.pStaticUtf16 != nullptr) {
            subDesc.pStaticUtf16 = tmpStr.pStaticUtf16;
        } else if (tmpStr.pRefObj != nullptr) {
            subDesc.pRefObj = tmpStr.pRefObj;
            ++(*reinterpret_cast<uint32_t*>(tmpStr.pRefObj + 0x38));
            retainedObj = tmpStr.pRefObj;
        }

        void* subNode = fnPushGroup(menuCtx, &subDesc, 1u);
        subDesc.pStaticUtf16 = nullptr;
        if (retainedObj != nullptr) {
            GXDebugMenu_ReleaseWStrRefObj(retainedObj);
            subDesc.pRefObj = nullptr;
        }

        if (subNode != nullptr) {
            alignas(16) GXDebugMenuWStrDesc maxAnisoDesc{nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c365aULL))}; // u"MaxAnisotropy"
            fnAddUInt32(
                menuCtx,
                &maxAnisoDesc,
                &self->samplerShadowDescs[i].maxAnisotropy,
                0u,
                16u,
                1u,
                *reinterpret_cast<const float*>(GX_VMA(0x492971cULL)));
            maxAnisoDesc.pStaticUtf16 = nullptr;

            alignas(16) GXDebugMenuWStrDesc mipBiasDesc{nullptr, reinterpret_cast<const void*>(GX_VMA(0x49c3676ULL))}; // u"mipLODBias"
            fnAddFloat(
                menuCtx,
                &mipBiasDesc,
                &self->samplerShadowDescs[i].mipLodBias,
                *reinterpret_cast<const float*>(GX_VMA(0x4929720ULL)),
                *reinterpret_cast<const float*>(GX_VMA(0x4929724ULL)),
                *reinterpret_cast<const float*>(GX_VMA(0x4929728ULL)),
                *reinterpret_cast<const float*>(GX_VMA(0x492971cULL)));
            mipBiasDesc.pStaticUtf16 = nullptr;

            GXDebugMenu_PopNodeFromStack(menuCtx);
        }

        tmpStr.pStaticUtf16 = nullptr;
        if (tmpStr.pRefObj != nullptr) {
            GXDebugMenu_ReleaseWStrRefObj(tmpStr.pRefObj);
            tmpStr.pRefObj = nullptr;
        }
    }

    GXDebugMenu_PopNodeFromStack(menuCtx);
}

// ----------------------------------------------------------------------------
// VMA: 0x21b2870 (374 Bytes)
// Updates `maxAnisotropy` (`newAnisoLevel`) across the 10 anisotropic sampler
// shadow descriptors in `GXStatePresetCache` (`+0x350 .. +0x78F`), compares
// each against the live `CGSamplerStateGnm` object, and recreates modified
// sampler state objects via `GXSamplerState_InitFromDesc` (`0x2178750`).
// ----------------------------------------------------------------------------
void GXStatePresetCache_UpdateSamplerAnisotropyPresets(
    GXStatePresetCache* self,
    uint32_t newAnisoLevel,
    DLAllocator* /*stateAlloc*/)
{
    // Update maxAnisotropy (+0x14 within each 0x38-byte GXSamplerDesc) for presets 8..17
    for (int idx = -8, slot = 0; slot < 19; ++idx, ++slot) {
        if (static_cast<uint32_t>(idx) <= 9u) {
            self->samplerShadowDescs[slot].maxAnisotropy = newAnisoLevel;
        }
    }

    // Compare each shadow descriptor against the existing sampler state and update in-place if dirty
    for (size_t i = 0; i < 19; ++i) {
        if (self->samplerStates[i] == nullptr) {
            continue;
        }
        GXSamplerDesc currentDesc{};
        GXSamplerState_ExtractDesc(self->samplerStates[i], &currentDesc);

        if (std::memcmp(&currentDesc, &self->samplerShadowDescs[i], sizeof(GXSamplerDesc)) != 0) {
            GXSamplerState_InitFromDesc(self->samplerStates[i], nullptr, &self->samplerShadowDescs[i]);
        }
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21b26a0 (457 Bytes)
// Releases all 19 sampler states, 24 rasterizer states, 6 depth-stencil states,
// and 57 blend states owned by `GXStatePresetCache`.
// ----------------------------------------------------------------------------
void GXStatePresetCache_Finalize(GXStatePresetCache* self) {
    for (size_t i = 0; i < 19; ++i) {
        if (self->samplerStates[i] != nullptr) {
            DLAllocator* alloc = GXStateObject_GetAllocator(self->samplerStates[i]);
            alloc->vptr->Free(alloc, self->samplerStates[i]);
            self->samplerStates[i] = nullptr;
        }
    }

    for (size_t i = 0; i < 24; ++i) {
        if (self->rasterizerStates[i] != nullptr) {
            DLAllocator* alloc = GXStateObject_GetAllocator(self->rasterizerStates[i]);
            alloc->vptr->Free(alloc, self->rasterizerStates[i]);
            self->rasterizerStates[i] = nullptr;
        }
    }

    for (size_t i = 0; i < 6; ++i) {
        if (self->depthStencilStates[i] != nullptr) {
            DLAllocator* alloc = GXStateObject_GetAllocator(self->depthStencilStates[i]);
            alloc->vptr->Free(alloc, self->depthStencilStates[i]);
            self->depthStencilStates[i] = nullptr;
        }
    }

    for (size_t i = 0; i < 57; ++i) {
        if (self->blendStates[i] != nullptr) {
            DLAllocator* alloc = GXStateObject_GetAllocator(self->blendStates[i]);
            alloc->vptr->Free(alloc, self->blendStates[i]);
            self->blendStates[i] = nullptr;
        }
    }
}

// ----------------------------------------------------------------------------
// VMA: 0x21aeca0 (14,847 Bytes)
// Initializes all 57 Blend State presets (`0x128` B each), 6 Depth-Stencil
// State presets (`0x38` B each), 24 Rasterizer State presets (`0x24` B each),
// and 19 Sampler State presets (`0x40` B each), then snapshots all 19 sampler
// descriptors into `self->samplerShadowDescs`.
// ----------------------------------------------------------------------------
struct GXBlendAttachmentDesc {
    uint32_t blendEnable;    // +0x00 (low byte read by GXBlendState_InitFromDesc)
    uint32_t srcBlend;       // +0x04
    uint32_t destBlend;      // +0x08
    uint32_t blendOp;        // +0x0C
    uint32_t srcBlendAlpha;  // +0x10
    uint32_t destBlendAlpha; // +0x14
    uint32_t blendOpAlpha;   // +0x18
    uint32_t writeMask;      // +0x1C
}; // sizeof == 0x20

struct GXBlendStateDesc {
    uint8_t               alphaToCoverageEnable;  // +0x00
    uint8_t               independentBlendEnable; // +0x01
    uint8_t               pad02[2];               // +0x02
    GXBlendAttachmentDesc renderTargets[8];       // +0x04 .. +0x103
}; // sizeof == 0x104

struct GXDepthStencilOpDesc {
    uint32_t stencilFailOp;      // +0x00
    uint32_t stencilDepthFailOp; // +0x04
    uint32_t stencilPassOp;      // +0x08
    uint32_t stencilFunc;        // +0x0C
};

struct GXDepthStencilStateDesc {
    uint8_t              depthEnable;           // +0x00
    uint8_t              pad01[3];              // +0x01
    uint32_t             depthWriteMask;        // +0x04
    uint32_t             depthFunc;             // +0x08
    uint8_t              stencilEnable;         // +0x0C
    uint8_t              stencilReadMask;       // +0x0D
    uint8_t              stencilWriteMask;      // +0x0E
    uint8_t              pad0F;                 // +0x0F
    GXDepthStencilOpDesc frontFace;             // +0x10 .. +0x1F
    GXDepthStencilOpDesc backFace;              // +0x20 .. +0x2F
    uint8_t              depthBoundsTestEnable; // +0x30
    uint8_t              pad31[3];              // +0x31
}; // sizeof == 0x34

struct GXRasterizerStateDesc {
    uint32_t fillMode;              // +0x00 (0 = Solid, 1 = Wireframe)
    uint32_t cullMode;              // +0x04 (0 = None, 1 = Front, 2 = Back)
    uint8_t  frontCounterClockwise; // +0x08
    uint8_t  pad09[3];              // +0x09
    int32_t  depthBias;             // +0x0C
    float    depthBiasClamp;        // +0x10
    float    slopeScaledDepthBias;  // +0x14
    uint8_t  depthClipEnable;       // +0x18
    uint8_t  scissorEnable;         // +0x19
    uint8_t  multisampleEnable;     // +0x1A
    uint8_t  antialiasedLineEnable; // +0x1B
}; // sizeof == 0x1C

static inline DLAllocator* GXStatePresetCache_ResolveDeviceStateAllocator(uint8_t* pDev) {
    if (pDev == nullptr
        || *reinterpret_cast<void**>(pDev + 0x00) == nullptr
        || *reinterpret_cast<void**>(pDev + 0x08) == nullptr)
    {
        return nullptr;
    }
    DLAllocator* stateAlloc = *reinterpret_cast<DLAllocator**>(pDev + 0xDC10);
    if (stateAlloc == nullptr || *reinterpret_cast<void**>(pDev + 0x20) == nullptr) {
        return nullptr;
    }
    return stateAlloc;
}

static void GXStatePresetCache_CreateAndStoreBlendPreset(
    GXStatePresetCache* self,
    size_t slotIndex,
    const GXBlendStateDesc* desc,
    uint8_t* pDev)
{
    DLAllocator* stateAlloc = GXStatePresetCache_ResolveDeviceStateAllocator(pDev);
    if (stateAlloc == nullptr) {
        return;
    }
    alignas(16) uint8_t tempBlendState[0x128]{};
    GXBlendState_InitFromDesc(tempBlendState, nullptr, desc);

    void* mem = stateAlloc->vptr->AllocateAligned(stateAlloc, 0x128u, 4u);
    if (mem != nullptr) {
        std::memcpy(mem, tempBlendState, 0x128u);
    }
    self->blendStates[slotIndex] = mem;
}

static void GXStatePresetCache_CreateAndStoreDepthStencilPreset(
    GXStatePresetCache* self,
    size_t slotIndex,
    const GXDepthStencilStateDesc* desc,
    uint8_t* pDev)
{
    DLAllocator* stateAlloc = GXStatePresetCache_ResolveDeviceStateAllocator(pDev);
    if (stateAlloc == nullptr) {
        return;
    }
    alignas(16) uint8_t tempDsState[0x38]{};
    GXDepthStencilState_InitFromDesc(tempDsState, nullptr, desc);

    void* mem = stateAlloc->vptr->AllocateAligned(stateAlloc, 0x38u, 4u);
    if (mem != nullptr) {
        std::memcpy(mem, tempDsState, 0x38u);
    }
    self->depthStencilStates[slotIndex] = mem;
}

static void GXStatePresetCache_CreateAndStoreRasterizerPreset(
    GXStatePresetCache* self,
    size_t slotIndex,
    const GXRasterizerStateDesc* desc,
    uint8_t* pDev)
{
    DLAllocator* stateAlloc = GXStatePresetCache_ResolveDeviceStateAllocator(pDev);
    if (stateAlloc == nullptr) {
        return;
    }
    alignas(16) uint8_t tempRsState[0x24]{};
    GXRasterizerState_InitFromDesc(tempRsState, nullptr, desc);

    void* mem = stateAlloc->vptr->AllocateAligned(stateAlloc, 0x24u, 4u);
    if (mem != nullptr) {
        std::memcpy(mem, tempRsState, 0x24u);
    }
    self->rasterizerStates[slotIndex] = mem;
}

static void GXStatePresetCache_CreateAndStoreSamplerPreset(
    GXStatePresetCache* self,
    size_t slotIndex,
    const GXSamplerDesc* desc,
    uint8_t* pDev)
{
    DLAllocator* stateAlloc = GXStatePresetCache_ResolveDeviceStateAllocator(pDev);
    if (stateAlloc == nullptr) {
        return;
    }
    alignas(16) uint8_t tempSamplerState[0x40]{};
    GXSamplerState_InitFromDesc(tempSamplerState, nullptr, desc);

    void* mem = stateAlloc->vptr->AllocateAligned(stateAlloc, 0x40u, 0x10u);
    if (mem != nullptr) {
        std::memcpy(mem, tempSamplerState, 0x40u);
    }
    self->samplerStates[slotIndex] = mem;
}

void GXStatePresetCache_InitializeAllPresets(
    GXStatePresetCache* self,
    DLAllocator* /*unusedRsi*/)
{
    uint8_t** ppGlobal = *reinterpret_cast<uint8_t***>(GX_VMA(0x55406c8ULL));
    uint8_t* pDev = ppGlobal ? *ppGlobal : nullptr;

    // 1. Construct the 57 Blend State presets (slots 0..24 without A2C,
    //    slots 25..49 with A2C, and slots 50..56 specialized single/dual-alpha presets)
    GXBlendStateDesc blendDesc{};
    for (uint32_t a2cPass = 0; a2cPass < 2; ++a2cPass) {
        const uint8_t a2cEnable = static_cast<uint8_t>(a2cPass == 1);
        const uint32_t passBase = (a2cPass == 1) ? 25u : 0u;

        // First sub-group: k = 0..2 -> base offsets {0, 5, 10} (+ passBase)
        for (int k = 0; k < 3; ++k) {
            const uint32_t baseSlot = passBase + static_cast<uint32_t>(k * 5);
            const uint32_t srcBlendAlpha = (k != 2) ? 1u : 0u;
            const uint32_t opaqueMask = (k == 0) ? 0x0Fu : 0x07u;
            const uint32_t blendMask  = (k != 1) ? 0x0Fu : 0x07u;

            blendDesc.alphaToCoverageEnable = a2cEnable;
            blendDesc.independentBlendEnable = 0;

            const GXBlendAttachmentDesc opaqueRt{ 0u, 1u, 0u, 0u, srcBlendAlpha, 0u, 0u, opaqueMask };
            for (int r = 0; r < 8; ++r) {
                blendDesc.renderTargets[r] = opaqueRt;
            }
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 0u, &blendDesc, pDev);

            blendDesc.renderTargets[0] = { 1u, 4u, 5u, 0u, srcBlendAlpha, 0u, 0u, blendMask };
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 1u, &blendDesc, pDev);

            blendDesc.renderTargets[0] = { 1u, 8u, 0u, 0u, srcBlendAlpha, 0u, 0u, blendMask };
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 2u, &blendDesc, pDev);

            blendDesc.renderTargets[0] = { 1u, 4u, 1u, 0u, srcBlendAlpha, 0u, 0u, blendMask };
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 3u, &blendDesc, pDev);

            blendDesc.renderTargets[0] = { 1u, 4u, 1u, 2u, srcBlendAlpha, 0u, 0u, blendMask };
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 4u, &blendDesc, pDev);
        }

        // Second sub-group: m = 0..1 -> base offsets {15, 20} (+ passBase), independent MRT blend
        for (int m = 0; m < 2; ++m) {
            const uint32_t baseSlot = passBase + 15u + static_cast<uint32_t>(m * 5);
            const uint32_t rt0OpaqueMask = (m == 1) ? 0x00u : 0x0Fu;
            const uint32_t rt0BlendMask  = (m == 1) ? 0x00u : 0x07u;

            blendDesc.alphaToCoverageEnable = a2cEnable;
            blendDesc.independentBlendEnable = static_cast<uint8_t>(m == 1);

            for (int r = 1; r < 8; ++r) {
                blendDesc.renderTargets[r] = { 0u, 1u, 0u, 0u, 0u, 0u, 0u, 0x0Fu };
            }
            blendDesc.renderTargets[0] = { 0u, 1u, 0u, 0u, 0u, 0u, 0u, rt0OpaqueMask };
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 0u, &blendDesc, pDev);

            blendDesc.independentBlendEnable = 1;

            const GXBlendAttachmentDesc mode1Rt{ 1u, 4u, 5u, 0u, 1u, 0u, 0u, 0x07u };
            for (int r = 1; r < 8; ++r) {
                blendDesc.renderTargets[r] = mode1Rt;
            }
            blendDesc.renderTargets[0] = mode1Rt;
            blendDesc.renderTargets[0].writeMask = rt0BlendMask;
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 1u, &blendDesc, pDev);

            const GXBlendAttachmentDesc mode2Rt{ 1u, 8u, 0u, 0u, 1u, 0u, 0u, 0x07u };
            for (int r = 1; r < 8; ++r) {
                blendDesc.renderTargets[r] = mode2Rt;
            }
            blendDesc.renderTargets[0] = mode2Rt;
            blendDesc.renderTargets[0].writeMask = rt0BlendMask;
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 2u, &blendDesc, pDev);

            const GXBlendAttachmentDesc mode3Rt{ 1u, 4u, 1u, 0u, 1u, 0u, 0u, 0x07u };
            for (int r = 1; r < 8; ++r) {
                blendDesc.renderTargets[r] = mode3Rt;
            }
            blendDesc.renderTargets[0] = mode3Rt;
            blendDesc.renderTargets[0].writeMask = rt0BlendMask;
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 3u, &blendDesc, pDev);

            const GXBlendAttachmentDesc mode4Rt{ 1u, 4u, 1u, 2u, 1u, 0u, 0u, 0x07u };
            for (int r = 1; r < 8; ++r) {
                blendDesc.renderTargets[r] = mode4Rt;
            }
            blendDesc.renderTargets[0] = mode4Rt;
            blendDesc.renderTargets[0].writeMask = rt0BlendMask;
            GXStatePresetCache_CreateAndStoreBlendPreset(self, baseSlot + 4u, &blendDesc, pDev);
        }
    }

    // Specialized blend presets 50..56 (alphaToCoverageEnable = 0, independentBlendEnable = 0)
    blendDesc.alphaToCoverageEnable = 0;
    blendDesc.independentBlendEnable = 0;

    blendDesc.renderTargets[0] = { 0u, 1u, 0u, 0u, 1u, 0u, 0u, 0x00u };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 50u, &blendDesc, pDev);

    blendDesc.renderTargets[0] = { 1u, 1u, 0u, 0u, 0u, 0u, 0u, 0x0Fu };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 51u, &blendDesc, pDev);

    blendDesc.renderTargets[0] = { 1u, 4u, 5u, 0u, 0u, 5u, 0u, 0x0Fu };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 52u, &blendDesc, pDev);

    blendDesc.renderTargets[0] = { 1u, 4u, 1u, 0u, 0u, 1u, 0u, 0x0Fu };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 53u, &blendDesc, pDev);

    blendDesc.renderTargets[0] = { 1u, 8u, 0u, 0u, 0u, 5u, 0u, 0x0Fu };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 54u, &blendDesc, pDev);

    blendDesc.renderTargets[0] = { 1u, 1u, 4u, 0u, 0u, 4u, 0u, 0x0Fu };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 55u, &blendDesc, pDev);

    blendDesc.renderTargets[0] = { 1u, 1u, 0u, 2u, 1u, 1u, 2u, 0x0Fu };
    GXStatePresetCache_CreateAndStoreBlendPreset(self, 56u, &blendDesc, pDev);

    // 2. Construct the 6 Depth-Stencil State presets (+0x1C8 .. +0x1F7, 0x38 bytes each)
    GXDepthStencilStateDesc dsDesc{};
    dsDesc.depthEnable = 0;
    dsDesc.depthWriteMask = 0u;
    dsDesc.depthFunc = 7u;
    dsDesc.stencilEnable = 0;
    dsDesc.stencilReadMask = 0xFFu;
    dsDesc.stencilWriteMask = 0xFFu;
    dsDesc.frontFace = { 0u, 0u, 0u, 7u };
    dsDesc.backFace  = { 0u, 0u, 0u, 7u };
    dsDesc.depthBoundsTestEnable = 0;
    GXStatePresetCache_CreateAndStoreDepthStencilPreset(self, 0u, &dsDesc, pDev);

    dsDesc.depthEnable = 1;
    dsDesc.depthWriteMask = 1u;
    dsDesc.depthFunc = 3u;
    GXStatePresetCache_CreateAndStoreDepthStencilPreset(self, 1u, &dsDesc, pDev);

    dsDesc.depthWriteMask = 0u;
    GXStatePresetCache_CreateAndStoreDepthStencilPreset(self, 2u, &dsDesc, pDev);

    dsDesc.depthWriteMask = 0u;
    dsDesc.depthFunc = 1u;
    GXStatePresetCache_CreateAndStoreDepthStencilPreset(self, 3u, &dsDesc, pDev);

    dsDesc.depthWriteMask = 1u;
    dsDesc.depthFunc = 7u;
    GXStatePresetCache_CreateAndStoreDepthStencilPreset(self, 4u, &dsDesc, pDev);

    dsDesc.depthEnable = 0;
    dsDesc.depthWriteMask = 0u;
    dsDesc.depthFunc = 7u;
    dsDesc.stencilEnable = 1;
    dsDesc.stencilReadMask = 0x00u;
    dsDesc.stencilWriteMask = 0xFFu;
    dsDesc.frontFace.stencilPassOp = 2u;
    dsDesc.backFace.stencilPassOp  = 2u;
    GXStatePresetCache_CreateAndStoreDepthStencilPreset(self, 5u, &dsDesc, pDev);

    // 3. Construct the 24 Rasterizer State presets (+0x1F8 .. +0x2B7, 0x24 bytes each)
    static constexpr uint32_t kCullModes[3] = { 0u, 2u, 1u };
    for (uint32_t msaaPass = 0; msaaPass < 2; ++msaaPass) {
        const size_t baseSlot = (msaaPass == 1) ? 12u : 0u;
        GXRasterizerStateDesc rsDesc{};
        rsDesc.frontCounterClockwise = 1;
        rsDesc.depthBias = 0;
        rsDesc.depthBiasClamp = 0.0f;
        rsDesc.slopeScaledDepthBias = 0.0f;
        rsDesc.depthClipEnable = 1;
        rsDesc.multisampleEnable = static_cast<uint8_t>(msaaPass == 1);
        rsDesc.antialiasedLineEnable = 0;

        for (uint32_t fill = 0; fill < 2; ++fill) {
            rsDesc.fillMode = fill;
            for (uint32_t c = 0; c < 3; ++c) {
                rsDesc.cullMode = kCullModes[c];
                for (uint32_t sc = 0; sc < 2; ++sc) {
                    rsDesc.scissorEnable = static_cast<uint8_t>(sc);
                    const size_t slot = baseSlot + fill * 6u + c * 2u + sc;
                    GXStatePresetCache_CreateAndStoreRasterizerPreset(self, slot, &rsDesc, pDev);
                }
            }
        }
    }

    // 4. Construct the 19 Sampler State presets (+0x2B8 .. +0x34F, 0x40 bytes each)
    static constexpr float kFltMax = 3.402823466e+38f; // 0x7f7fffff

    GXSamplerDesc ssDesc{};
    ssDesc.filterMode = 0u;
    ssDesc.addressU = 0u;
    ssDesc.addressV = 0u;
    ssDesc.addressW = 0u;
    ssDesc.mipLodBias = 0.0f;
    ssDesc.maxAnisotropy = 1u;
    ssDesc.comparisonFunc = 7u;
    ssDesc.borderColor[0] = 0.0f;
    ssDesc.borderColor[1] = 0.0f;
    ssDesc.borderColor[2] = 0.0f;
    ssDesc.borderColor[3] = 0.0f;
    ssDesc.minLod = 0.0f;
    ssDesc.maxLod = kFltMax;
    ssDesc.flags = 0u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 0u, &ssDesc, pDev);

    ssDesc.filterMode = 7u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 1u, &ssDesc, pDev);

    ssDesc.filterMode = 6u;
    ssDesc.maxLod = 0.0f;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 2u, &ssDesc, pDev);

    ssDesc.filterMode = 0u;
    ssDesc.addressU = 2u;
    ssDesc.addressV = 2u;
    ssDesc.addressW = 2u;
    ssDesc.maxLod = kFltMax;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 3u, &ssDesc, pDev);

    ssDesc.filterMode = 7u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 4u, &ssDesc, pDev);

    ssDesc.filterMode = 6u;
    ssDesc.maxLod = 0.0f;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 5u, &ssDesc, pDev);

    GXSamplerDesc ssBorderDesc = ssDesc;
    ssBorderDesc.filterMode = 7u;
    ssBorderDesc.addressU = 3u;
    ssBorderDesc.addressV = 3u;
    ssBorderDesc.addressW = 3u;
    ssBorderDesc.borderColor[0] = 1.0f;
    ssBorderDesc.borderColor[1] = 1.0f;
    ssBorderDesc.borderColor[2] = 1.0f;
    ssBorderDesc.borderColor[3] = 1.0f;
    ssBorderDesc.maxLod = kFltMax;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 7u, &ssBorderDesc, pDev);

    ssBorderDesc.filterMode = 0u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 6u, &ssBorderDesc, pDev);

    ssDesc.filterMode = 8u;
    ssDesc.addressU = 2u;
    ssDesc.addressV = 2u;
    ssDesc.addressW = 2u;
    ssDesc.maxAnisotropy = 4u;
    ssDesc.maxLod = kFltMax;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 9u, &ssDesc, pDev);

    ssDesc.filterMode = 8u;
    ssDesc.addressU = 0u;
    ssDesc.addressV = 0u;
    ssDesc.addressW = 0u;
    ssDesc.maxAnisotropy = 4u;
    ssDesc.maxLod = kFltMax;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 8u, &ssDesc, pDev);

    ssDesc.filterMode = 8u;
    ssDesc.addressU = 3u;
    ssDesc.addressV = 3u;
    ssDesc.addressW = 3u;
    ssDesc.maxAnisotropy = 4u;
    ssDesc.borderColor[0] = 0.0f;
    ssDesc.borderColor[1] = 0.0f;
    ssDesc.borderColor[2] = 0.0f;
    ssDesc.borderColor[3] = 0.0f;
    ssDesc.maxLod = kFltMax;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 10u, &ssDesc, pDev);

    ssDesc.borderColor[0] = 0.5f;
    ssDesc.borderColor[1] = 0.5f;
    ssDesc.borderColor[2] = 0.5f;
    ssDesc.borderColor[3] = 0.0f;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 11u, &ssDesc, pDev);

    ssDesc.addressU = 0u;
    ssDesc.addressV = 0u;
    ssDesc.addressW = 0u;
    if (pDev != nullptr && *reinterpret_cast<void**>(pDev + 0x00) != nullptr) {
        GXStatePresetCache_CreateAndStoreSamplerPreset(self, 12u, &ssDesc, pDev);
        GXStatePresetCache_CreateAndStoreSamplerPreset(self, 14u, &ssDesc, pDev);
        GXStatePresetCache_CreateAndStoreSamplerPreset(self, 15u, &ssDesc, pDev);
        GXStatePresetCache_CreateAndStoreSamplerPreset(self, 17u, &ssDesc, pDev);
    }

    ssDesc.maxAnisotropy = 4u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 13u, &ssDesc, pDev);

    ssDesc.filterMode = 7u;
    ssDesc.maxAnisotropy = 0u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 16u, &ssDesc, pDev);

    // Slot 18: PCF Shadow Map Comparison Sampler (filterMode = 0x0F, address = 3, comparisonFunc = 1)
    GXSamplerDesc ssShadowCmpDesc{};
    ssShadowCmpDesc.filterMode = 0x0Fu;
    ssShadowCmpDesc.addressU = 3u;
    ssShadowCmpDesc.addressV = 3u;
    ssShadowCmpDesc.addressW = 3u;
    ssShadowCmpDesc.comparisonFunc = 1u;
    GXStatePresetCache_CreateAndStoreSamplerPreset(self, 18u, &ssShadowCmpDesc, pDev);

    // Snapshot all 19 sampler descriptors into `self->samplerShadowDescs` (+0x350 .. +0x78F)
    for (size_t i = 0; i < 19; ++i) {
        if (self->samplerStates[i] != nullptr) {
            GXSamplerState_ExtractDesc(self->samplerStates[i], &self->samplerShadowDescs[i]);
        }
    }
}

// ============================================================================
// 2. DLVector<void*> Pointer-Vector Template Instantiations (0x21ae2f0 .. 0x21aeba0)
// ============================================================================

struct DLPointerVector {
    void*        vptr;      // +0x00
    void**       begin;     // +0x08
    void**       end;       // +0x10
    void**       cap;       // +0x18
    DLAllocator* allocator; // +0x20
};

static inline void DLPointerVector_HandleAllocFailure() {
    if (*reinterpret_cast<uint32_t*>(gx_guest_tls_slot(0x53e4318ULL)) != 0u) {
        reinterpret_cast<void (*)(void*, int)>(GX_VMA(0x2bbe978ULL))(
            gx_guest_tls_slot(0x53e4320ULL), 1);
    } else {
        reinterpret_cast<void (*)(uint32_t, uint32_t, uintptr_t, ...)>(GX_VMA(0x20b55b0ULL))(
            0u, 0x109u, GX_VMA(0x4737d80ULL));
    }
}

static inline void DLPointerVector_ReallocateImpl(DLPointerVector* self, size_t newCapElements) {
    void** newBuf = static_cast<void**>(
        self->allocator->vptr->AllocateAligned(self->allocator, newCapElements * sizeof(void*), 8u));
    if (newBuf == nullptr) {
        DLPointerVector_HandleAllocFailure();
    }

    void** oldBegin = self->begin;
    void** oldEnd   = self->end;
    ptrdiff_t count = oldEnd - oldBegin;
    if (newBuf != nullptr && count > 0) {
        std::memcpy(newBuf, oldBegin, static_cast<size_t>(count) * sizeof(void*));
    }

    if (oldBegin != nullptr) {
        self->allocator->vptr->Free(self->allocator, oldBegin);
    }

    self->cap   = newBuf ? (newBuf + newCapElements) : nullptr;
    self->end   = newBuf ? (newBuf + count) : nullptr;
    self->begin = newBuf;
}

// VMA: 0x21aeba0 (251 B)
void DLVectorPtr_ReallocateType5(DLPointerVector* self, size_t newCap) {
    DLPointerVector_ReallocateImpl(self, newCap);
}

// VMA: 0x21aeaa0 (251 B)
void DLVectorPtr_ReallocateType4(DLPointerVector* self, size_t newCap) {
    DLPointerVector_ReallocateImpl(self, newCap);
}

// VMA: 0x21ae9a0 (251 B)
void DLVectorPtr_ReallocateType3(DLPointerVector* self, size_t newCap) {
    DLPointerVector_ReallocateImpl(self, newCap);
}

// VMA: 0x21ae8a0 (251 B)
void DLVectorPtr_ReallocateType2(DLPointerVector* self, size_t newCap) {
    DLPointerVector_ReallocateImpl(self, newCap);
}

// VMA: 0x21ae7a0 (251 B)
void DLVectorPtr_ReallocateType1(DLPointerVector* self, size_t newCap) {
    DLPointerVector_ReallocateImpl(self, newCap);
}

// VMA: 0x21ae6a0 (251 B)
void DLVectorPtr_ReallocateType0(DLPointerVector* self, size_t newCap) {
    DLPointerVector_ReallocateImpl(self, newCap);
}

// ----------------------------------------------------------------------------
// VMA: 0x21ae2f0 (934 Bytes)
// Move-assignment operator for `DLPointerVector` (`DLVector<T*>::operator=(DLVector&&)`).
// Frees existing storage, steals the pointer triad if both vectors share the
// same allocator heap tag (`vptr->GetHeapTag`), or deep-copies when heaps differ.
// ----------------------------------------------------------------------------
DLPointerVector* DLVectorPtr_MoveAssign(DLPointerVector* self, DLPointerVector* rhs) {
    if (self == rhs) {
        return self;
    }

    if (self->begin != nullptr) {
        self->allocator->vptr->Free(self->allocator, self->begin);
        self->cap   = nullptr;
        self->end   = nullptr;
        self->begin = nullptr;
    }

    uint32_t selfTag = self->allocator->vptr->GetHeapTag(self->allocator);
    uint32_t rhsTag  = rhs->allocator->vptr->GetHeapTag(rhs->allocator);

    if (selfTag == rhsTag) {
        self->begin = rhs->begin;
        self->end   = rhs->end;
        self->cap   = rhs->cap;
        rhs->cap    = nullptr;
        rhs->end    = nullptr;
        rhs->begin  = nullptr;
        return self;
    }

    void** rhsBegin = rhs->begin;
    void** rhsEnd   = rhs->end;
    self->end = self->begin;

    size_t rhsCount = static_cast<size_t>(rhsEnd - rhsBegin);
    if (rhsCount == 0u) {
        return self;
    }

    if ((rhsCount >> 61) != 0u) {
        reinterpret_cast<void (*)(const char*)>(GX_VMA(0x2bbf1b8ULL))(
            reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
    }

    DLPointerVector_ReallocateImpl(self, rhsCount);
    if (self->begin != nullptr) {
        for (size_t i = 0; i < rhsCount; ++i) {
            self->begin[i] = rhsBegin[i];
        }
    }
    self->end = self->begin + rhsCount;
    return self;
}

} // namespace GX
} // namespace FromSoftware
