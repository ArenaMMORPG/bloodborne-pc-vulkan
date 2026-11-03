// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 04: SceneRenderer (Tier 8/10)
// File: GXFlverParserAndSkinningMatricesUpdater.cpp
// Range: 0x21c9a90 .. 0x21cedf0 (55 functions, 20,199 bytes)
// Direction: Right-to-Left (highest VMA 0x21cedf0 down to lowest VMA 0x21c9a90)
// ============================================================================
// Architectural Subsystems in this Translation Unit:
//   1. GXModelFlverContainer Binary Parser & Section Accessors (0x21cedf0 .. 0x21ce560):
//      - GetFaceSetHeaderAt (0x21cedf0), GetBufferLayoutMemberAt (0x21cedd0),
//        GetBufferLayoutMemberCount (0x21cedc0), GetMeshHeaderAt (0x21cedb0),
//        GetBoneNameAt (0x21ced90), GetBoneHeaderAt (0x21ced80),
//        GetMaterialTextureTypeNameAt (0x21ced40), GetTextureHeaderAt (0x21ced10),
//        GetMaterialGxListAt (0x21cecf0), GetMaterialMtdPathAt (0x21cecd0),
//        GetMaterialNameAt (0x21cecb0), GetMaterialHeaderAt (0x21ceca0),
//        GetDummyHeaderAt (0x21cec90)
//      - ParseAndValidate (0x21ce5d0, 1,725 B: full FLVER2 endianness detection
//        at header[0x06] == 'B', MOVBE byte-swapping across header, dummies,
//        materials, bones, meshes, face sets, vertex buffers, buffer layouts,
//        textures, bone index pairs, and mesh bone indices, plus version
//        validation <= 0x20014 and section pointer table population)
//      - Construct (0x21ce560, 100 B)
//   2. GXFlverTextureResourceList & RefCounted Resource Containers (0x21ce550 .. 0x21cd5b0):
//      - GXFlverDoubleBufferedSlot_GetActivePtr (0x21ce550) & Construct (0x21ce340)
//      - GXFlverNamedResourceBase_Destroy (0x21ce470) & DeletingDestructor (0x21ce430)
//      - GXRefCountedPtrVector_EraseRange (0x21ce1b0, 388 B)
//      - GXFlverTextureResourceList_InitializeFromFlver (0x21cdcd0, 1,093 B),
//        ClearAndFreeSlots (0x21cdbe0), Destroy (0x21cda20),
//        DeletingDestructor (0x21cda00), & Construct (0x21cd920)
//      - GXFlverResourceVector accessors, Clear, Destroy, & Construct (0x21cd900 .. 0x21cd5b0)
//   3. GXSkinningMatricesUpdater & Introsort (0x21cd130 .. 0x21c9a90):
//      - GXSkinningSegmentVector_CopyAssignWithRotate (0x21cd130, 1,137 B)
//      - GXSkinningEntryVector_CopyAssignWithRotate (0x21ccbc0, 1,385 B)
//      - GXSkinningEntryVector_GrowByOne (0x21cca50, 363 B)
//      - GXSkinningMatricesUpdater_RegisterDebugGuiProperties (0x21cc6c0, 700 B:
//        "MatricesUpdater", "Block Count", "Buffer  Free", "Seg     Free", "Warning Rate")
//      - GXSkinningMatricesUpdater_UnregisterSkinningTarget (0x21cc5e0, 222 B)
//      - GXSkinningMatricesUpdater_AllocateAndQueueBoneMatrices (0x21cba80, 2,912 B)
//      - GXSkinningMatricesUpdaterOwner_Destroy (0x21cba30) & Create (0x21cb9d0)
//      - GXSkinningMatricesUpdater_SubmitClearJob (0x21cb970) &
//        DispatchFrameUpdateJobs (0x21cb5d0, 925 B)
//      - GXSkinningMatricesUpdater_Destroy (0x21cad60, 2,145 B) &
//        Construct (0x21ca3d0, 2,441 B: DLSegregatedBiHeap + DLBiHeap setup)
//      - GXSkinningClearJob_Execute (0x21ca340) & GXSkinningUpdateJob_ExecuteRange (0x21ca290)
//      - GXSkinningVector_DestroyBuffer (0x21ca250)
//      - GXSortKey64_Introsort (0x21c9c00, 1,615 B)
//      - Global Scene Render Callback Setters (0x21c9aa0, 0x21c9a90)
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <immintrin.h>

namespace {

// ----------------------------------------------------------------------------
// Read-Only Constants (.rodata) verified from eboot.bin (vma + 0x4000)
// ----------------------------------------------------------------------------
constexpr float kOne_4929754          = 1.0f;                  // 0x3f800000
constexpr float kPointZeroOne_4929758 = 0.009999999776482582f; // 0x3c23d70a
constexpr float kOneHundred_492975c   = 100.0f;                // 0x42c80000

// ----------------------------------------------------------------------------
// External Engine Helper Declarations (resolved by VMA)
// ----------------------------------------------------------------------------
using FnGetDefaultStringAllocator = void* (*)();
using FnLogWarning                = void (*)(const char* fileOrNull, uint32_t line, const char* msg, ...);
using FnOperatorDelete            = void (*)(void* ptr);
using FnRefPtrVecReserve          = void (*)(void* vec, uint64_t newCap);
using FnWideToUtf8                = void (*)(const char16_t* src, char* dst, uint32_t maxBytes);
using FnStrLen                    = uint64_t (*)(const char* s);
using FnDLStringAssign            = void (*)(void* dlStr, const char* s, uint64_t len);
using FnThrowLengthError          = void (*)(const char* msg);
using FnSetErrnoError             = void (*)(void* tlsErrPtr, uint32_t code);
using FnDebugGuiBeginGroup        = void* (*)(void* guiCtx, const void* nameDesc, uint32_t flags);
using FnDebugGuiAddUint32Prop     = void (*)(void* guiCtx, const void* nameDesc, void* propAddr);
using FnDebugGuiAddFloatProp      = void (*)(void* guiCtx, const void* nameDesc, void* propAddr,
                                             float minVal, float maxVal, float stepVal, float multVal);
using FnMemcpyBoneMatrices        = void (*)(void* dst, const void* src, uint64_t byteCount);
using FnAllocTaskNode             = void* (*)(void* taskPool);
using FnSubmitTaskNode            = void (*)(void* queue, void* taskNode);
using FnMemmove                   = void* (*)(void* dst, const void* src, uint64_t size);
using FnFinalizeHeapTracker       = void (*)(void* heap);
using FnFreeBiHeapBlock           = void (*)(void* biHeap, void* block);
using FnPthreadMutexDestroy       = int32_t (*)(void* mutex);
using FnPthreadMutexInit          = int32_t (*)(void* mutex);
using FnDestroyAllocatorAdapter   = void (*)(void* adapter);
using FnGetProcessHeapId          = uint32_t (*)();
using FnGetMemoryTracker          = void* (*)();
using FnRegisterHeapRange         = void (*)(void* tracker, void* heap, void* mem, uint64_t size);
using FnInitBiHeap                = bool (*)(void* biHeap, void* parentHeap, void* mem, uint64_t size);
using FnRegisterWithGlobalDevice  = void (*)(void* globalDevice, void* updater);
using FnExecuteTargetSkinning     = void (*)(void* target, void* dst, const void* src, uint32_t flag);

inline uint64_t ReadTlsQwordAtOffset(uint64_t tlsOffsetGlobalVma) {
    return reinterpret_cast<uint64_t>(gx_guest_tls_slot(tlsOffsetGlobalVma));
}

inline uint16_t ByteSwap16SignedViaBextr(int16_t val) {
    uint32_t s = static_cast<uint32_t>(static_cast<int32_t>(val));
    uint32_t hi = (s >> 8) & 0xFFu;
    uint32_t lo = s << 8;
    return static_cast<uint16_t>(lo | hi);
}

inline uint16_t Rol16By8(uint16_t val) {
    return static_cast<uint16_t>((val << 8) | (val >> 8));
}

inline void Bswap32InPlace(uint8_t* base, uint64_t offset) {
    uint32_t v = *reinterpret_cast<uint32_t*>(base + offset);
    *reinterpret_cast<uint32_t*>(base + offset) = __builtin_bswap32(v);
}

inline void Bswap16SignedInPlace(uint8_t* base, uint64_t offset) {
    int16_t v = *reinterpret_cast<int16_t*>(base + offset);
    *reinterpret_cast<uint16_t*>(base + offset) = ByteSwap16SignedViaBextr(v);
}

} // namespace

extern "C" {

// Forward declarations for intra-unit calls
void* sub_21ce1b0(void* outIter, void* vec, void* last, uint64_t padRcx, uint64_t padR8, uint64_t padR9,
                  uint64_t stackArg0, uint64_t stackArg1, void* first,
                  uint64_t stackArg2, uint64_t stackArg3, void* stackLast);
void  sub_21cda20(void* self);
void  sub_21cd130(void* dstVec, const void* srcVec);
void  sub_21ccbc0(void* dstVec, const void* srcVec);
void  sub_21cca50(void* vec);
void  sub_21cad60(void* self);
void  sub_21ca3d0(void* self, uint32_t segHeapBlocks, uint32_t biHeapBytes);
void  sub_21c9c00(uint64_t* first, uint64_t* last, int64_t depthLimit);

// ============================================================================
// 1. 0x21cedf0 (30 B) - GXModelFlverContainer_GetFaceSetHeaderAt
// ============================================================================
void* sub_21cedf0(const void* flverContainer, uint32_t faceSetIndex) {
    auto* cont   = static_cast<const uint8_t*>(flverContainer);
    auto* hdr    = *reinterpret_cast<const uint8_t* const*>(cont + 0x00);
    uint32_t ver = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
    uint32_t stride = (ver > 0x20008u) ? 0x20u : 0x10u;
    uint32_t byteOffset = stride * faceSetIndex;
    return *reinterpret_cast<uint8_t* const*>(cont + 0x28) + byteOffset;
}

// ============================================================================
// 2. 0x21cedd0 (28 B) - GXModelFlverContainer_GetBufferLayoutMemberAt
// ============================================================================
void* sub_21cedd0(const void* flverContainer, uint32_t layoutIndex, uint32_t memberIndex) {
    auto* cont        = static_cast<const uint8_t*>(flverContainer);
    auto* layoutTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x38);
    uint32_t membersOff = *reinterpret_cast<const uint32_t*>(
        layoutTable + (static_cast<uint64_t>(layoutIndex) << 4) + 0x0c);
    uint8_t* base = *reinterpret_cast<uint8_t* const*>(cont + 0x00) + membersOff;
    uint64_t mIdx = static_cast<uint64_t>(memberIndex);
    return base + mIdx * 20ULL;
}

// ============================================================================
// 3. 0x21cedc0 (14 B) - GXModelFlverContainer_GetBufferLayoutMemberCount
// ============================================================================
uint32_t sub_21cedc0(const void* flverContainer, uint32_t layoutIndex) {
    auto* cont        = static_cast<const uint8_t*>(flverContainer);
    auto* layoutTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x38);
    return *reinterpret_cast<const uint32_t*>(layoutTable + (static_cast<uint64_t>(layoutIndex) << 4));
}

// ============================================================================
// 4. 0x21cedb0 (16 B) - GXModelFlverContainer_GetMeshHeaderAt
// ============================================================================
void* sub_21cedb0(const void* flverContainer, int32_t meshIndex) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    int64_t off = static_cast<int64_t>(meshIndex) * 48LL;
    return *reinterpret_cast<uint8_t* const*>(cont + 0x20) + off;
}

// ============================================================================
// 5. 0x21ced90 (19 B) - GXModelFlverContainer_GetBoneNameAt
// ============================================================================
const char16_t* sub_21ced90(const void* flverContainer, int32_t boneIndex) {
    auto* cont      = static_cast<const uint8_t*>(flverContainer);
    auto* boneTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x18);
    uint32_t nameOff = *reinterpret_cast<const uint32_t*>(
        boneTable + (static_cast<int64_t>(boneIndex) << 7) + 0x0c);
    return reinterpret_cast<const char16_t*>(*reinterpret_cast<const uint8_t* const*>(cont + 0x00) + nameOff);
}

// ============================================================================
// 6. 0x21ced80 (12 B) - GXModelFlverContainer_GetBoneHeaderAt
// ============================================================================
void* sub_21ced80(const void* flverContainer, int32_t boneIndex) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    return *reinterpret_cast<uint8_t* const*>(cont + 0x18) + (static_cast<int64_t>(boneIndex) << 7);
}

// ============================================================================
// 7. 0x21ced40 (52 B) - GXModelFlverContainer_GetMaterialTextureTypeNameAt
// ============================================================================
const char16_t* sub_21ced40(const void* flverContainer, int32_t materialIndex, uint32_t subTexIndex) {
    auto* cont     = static_cast<const uint8_t*>(flverContainer);
    auto* hdr      = *reinterpret_cast<const uint8_t* const*>(cont + 0x00);
    auto* matTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x10);
    uint32_t texIdx = subTexIndex + *reinterpret_cast<const uint32_t*>(
        matTable + (static_cast<int64_t>(materialIndex) << 5) + 0x0c);
    uint32_t ver    = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
    auto* texTable  = *reinterpret_cast<const uint8_t* const*>(cont + 0x40);
    uint64_t texByteOff = (ver <= 0x20002u)
        ? static_cast<uint64_t>(texIdx << 4)
        : (static_cast<uint64_t>(texIdx) << 5);
    uint32_t typeNameOff = *reinterpret_cast<const uint32_t*>(texTable + texByteOff + 0x04);
    return reinterpret_cast<const char16_t*>(hdr + typeNameOff);
}

// ============================================================================
// 8. 0x21ced10 (33 B) - GXModelFlverContainer_GetTextureHeaderAt
// ============================================================================
void* sub_21ced10(const void* flverContainer, uint32_t textureIndex) {
    auto* cont     = static_cast<const uint8_t*>(flverContainer);
    auto* hdr      = *reinterpret_cast<const uint8_t* const*>(cont + 0x00);
    auto* texTable = *reinterpret_cast<uint8_t* const*>(cont + 0x40);
    if (*reinterpret_cast<const uint32_t*>(hdr + 0x08) <= 0x20002u) {
        return texTable + static_cast<uint64_t>(textureIndex << 4);
    }
    return texTable + (static_cast<uint64_t>(textureIndex) << 5);
}

// ============================================================================
// 9. 0x21cecf0 (29 B) - GXModelFlverContainer_GetMaterialGxListAt
// ============================================================================
void* sub_21cecf0(const void* flverContainer, int32_t materialIndex) {
    auto* cont     = static_cast<const uint8_t*>(flverContainer);
    auto* matTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x10);
    uint32_t gxOff = *reinterpret_cast<const uint32_t*>(
        matTable + (static_cast<int64_t>(materialIndex) << 5) + 0x14);
    if (gxOff == 0) {
        return nullptr;
    }
    return *reinterpret_cast<uint8_t* const*>(cont + 0x00) + gxOff;
}

// ============================================================================
// 10. 0x21cecd0 (19 B) - GXModelFlverContainer_GetMaterialMtdPathAt
// ============================================================================
const char16_t* sub_21cecd0(const void* flverContainer, int32_t materialIndex) {
    auto* cont     = static_cast<const uint8_t*>(flverContainer);
    auto* matTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x10);
    uint32_t mtdOff = *reinterpret_cast<const uint32_t*>(
        matTable + (static_cast<int64_t>(materialIndex) << 5) + 0x04);
    return reinterpret_cast<const char16_t*>(*reinterpret_cast<const uint8_t* const*>(cont + 0x00) + mtdOff);
}

// ============================================================================
// 11. 0x21cecb0 (18 B) - GXModelFlverContainer_GetMaterialNameAt
// ============================================================================
const char16_t* sub_21cecb0(const void* flverContainer, int32_t materialIndex) {
    auto* cont     = static_cast<const uint8_t*>(flverContainer);
    auto* matTable = *reinterpret_cast<const uint8_t* const*>(cont + 0x10);
    uint32_t nameOff = *reinterpret_cast<const uint32_t*>(
        matTable + (static_cast<int64_t>(materialIndex) << 5));
    return reinterpret_cast<const char16_t*>(*reinterpret_cast<const uint8_t* const*>(cont + 0x00) + nameOff);
}

// ============================================================================
// 12. 0x21ceca0 (12 B) - GXModelFlverContainer_GetMaterialHeaderAt
// ============================================================================
void* sub_21ceca0(const void* flverContainer, int32_t materialIndex) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    return *reinterpret_cast<uint8_t* const*>(cont + 0x10) + (static_cast<int64_t>(materialIndex) << 5);
}

// ============================================================================
// 13. 0x21cec90 (12 B) - GXModelFlverContainer_GetDummyHeaderAt
// ============================================================================
void* sub_21cec90(const void* flverContainer, int32_t dummyIndex) {
    auto* cont = static_cast<const uint8_t*>(flverContainer);
    return *reinterpret_cast<uint8_t* const*>(cont + 0x08) + (static_cast<int64_t>(dummyIndex) << 6);
}

// ============================================================================
// 14. 0x21ce5d0 (1,725 B) - GXModelFlverContainer_ParseAndValidate
// ============================================================================
bool sub_21ce5d0(void* flverContainer, void* rawFlverData) {
    auto* cont = static_cast<uint8_t*>(flverContainer);
    auto* hdr  = static_cast<uint8_t*>(rawFlverData);
    *reinterpret_cast<uint8_t**>(cont + 0x00) = hdr;

    if (hdr[0x06] == 0x42u) { // 'B' = big-endian FLVER2
        Bswap32InPlace(hdr, 0x08);
        Bswap32InPlace(hdr, 0x0c);
        Bswap32InPlace(hdr, 0x10);
        Bswap32InPlace(hdr, 0x14);
        Bswap32InPlace(hdr, 0x18);
        Bswap32InPlace(hdr, 0x1c);
        Bswap32InPlace(hdr, 0x20);
        Bswap32InPlace(hdr, 0x24);
        Bswap32InPlace(hdr, 0x28);
        Bswap32InPlace(hdr, 0x2c);
        Bswap32InPlace(hdr, 0x30);
        Bswap32InPlace(hdr, 0x34);
        Bswap32InPlace(hdr, 0x38);
        Bswap32InPlace(hdr, 0x3c);
        Bswap32InPlace(hdr, 0x40);
        Bswap32InPlace(hdr, 0x44);
        Bswap32InPlace(hdr, 0x4c);
        Bswap32InPlace(hdr, 0x50);
        Bswap32InPlace(hdr, 0x54);
        Bswap32InPlace(hdr, 0x58);
        Bswap32InPlace(hdr, 0x60);
        Bswap32InPlace(hdr, 0x64);
        hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
        Bswap32InPlace(hdr, 0x68);
    }

    uint32_t version = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
    if (version > 0x20014u) {
        return false;
    }

    uint32_t faceSetStride = (version > 0x20008u) ? 0x20u : 0x10u;
    uint32_t textureStride = (version <= 0x20002u) ? 0x10u : 0x20u;

    uint8_t* pDummies = hdr + 0x80;
    *reinterpret_cast<uint8_t**>(cont + 0x08) = pDummies;

    uint32_t dummyCount = *reinterpret_cast<const uint32_t*>(hdr + 0x14);
    uint64_t dummyBytes = static_cast<uint64_t>(dummyCount) << 6;
    uint8_t* pMaterials = hdr + dummyBytes + 0x80;
    *reinterpret_cast<uint8_t**>(cont + 0x10) = pMaterials;

    uint32_t materialCount = *reinterpret_cast<const uint32_t*>(hdr + 0x18);
    uint8_t* pBones = pMaterials + (static_cast<uint64_t>(materialCount) << 5);
    *reinterpret_cast<uint8_t**>(cont + 0x18) = pBones;

    uint32_t boneCount = *reinterpret_cast<const uint32_t*>(hdr + 0x1c);
    uint8_t* pMeshes = pBones + (static_cast<uint64_t>(boneCount) << 7);
    *reinterpret_cast<uint8_t**>(cont + 0x20) = pMeshes;

    uint32_t meshCount = *reinterpret_cast<const uint32_t*>(hdr + 0x20);
    uint8_t* pFaceSets = pMeshes + (static_cast<uint64_t>(meshCount) * 3ULL << 4);
    *reinterpret_cast<uint8_t**>(cont + 0x28) = pFaceSets;

    uint32_t faceSetCount = *reinterpret_cast<const uint32_t*>(hdr + 0x50);
    uint8_t* pVertexBuffers = pFaceSets + static_cast<uint64_t>(faceSetStride * faceSetCount);
    *reinterpret_cast<uint8_t**>(cont + 0x30) = pVertexBuffers;

    uint32_t vbCount = *reinterpret_cast<const uint32_t*>(hdr + 0x24);
    uint8_t* pBufferLayouts = pVertexBuffers + (static_cast<uint64_t>(vbCount) << 5);
    *reinterpret_cast<uint8_t**>(cont + 0x38) = pBufferLayouts;

    uint32_t layoutCount = *reinterpret_cast<const uint32_t*>(hdr + 0x54);
    uint8_t* pTextures = pBufferLayouts + (static_cast<uint64_t>(layoutCount) << 4);
    *reinterpret_cast<uint8_t**>(cont + 0x40) = pTextures;

    uint32_t texCount = *reinterpret_cast<const uint32_t*>(hdr + 0x58);
    uint8_t* pUnkSec60 = pTextures + static_cast<uint64_t>(textureStride * texCount);
    *reinterpret_cast<uint8_t**>(cont + 0x48) = pUnkSec60;

    uint32_t unkCount60 = *reinterpret_cast<const uint32_t*>(hdr + 0x60);
    uint8_t* pBoneIndexPairs = pUnkSec60 + (static_cast<uint64_t>(unkCount60) * 3ULL << 4);
    *reinterpret_cast<uint8_t**>(cont + 0x50) = pBoneIndexPairs;

    uint32_t dataOffset = *reinterpret_cast<const uint32_t*>(hdr + 0x0c);
    *reinterpret_cast<uint8_t**>(cont + 0x58) = static_cast<uint8_t*>(rawFlverData) + dataOffset;

    if (hdr[0x06] == 0x42u) {
        // 1. Byte-swap Dummies (64 B each)
        if (dummyCount != 0) {
            uint32_t i = 0;
            do {
                uint8_t* d = pDummies + (static_cast<uint64_t>(i) << 6);
                Bswap32InPlace(d, 0x00);
                Bswap32InPlace(d, 0x04);
                Bswap32InPlace(d, 0x08);
                Bswap32InPlace(d, 0x0c);
                Bswap32InPlace(d, 0x10);
                Bswap32InPlace(d, 0x14);
                Bswap32InPlace(d, 0x18);
                Bswap16SignedInPlace(d, 0x1c);
                Bswap16SignedInPlace(d, 0x1e);
                Bswap32InPlace(d, 0x20);
                Bswap32InPlace(d, 0x24);
                Bswap32InPlace(d, 0x28);
                Bswap16SignedInPlace(d, 0x2c);
                ++i;
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x14));
            materialCount = *reinterpret_cast<const uint32_t*>(hdr + 0x18);
        }

        // 2. Byte-swap Materials (32 B each)
        if (materialCount != 0) {
            uint8_t* mPtr = hdr + dummyBytes + 0x90;
            for (uint32_t i = 0; i < materialCount; ++i, mPtr += 0x20) {
                Bswap32InPlace(mPtr - 0x10, 0x00);
                Bswap32InPlace(mPtr - 0x10, 0x04);
                Bswap32InPlace(mPtr - 0x10, 0x08);
                Bswap32InPlace(mPtr - 0x10, 0x0c);
                Bswap32InPlace(mPtr - 0x10, 0x10);
            }
        }

        // 3. Byte-swap Bones (128 B each)
        if (*reinterpret_cast<const uint32_t*>(hdr + 0x1c) != 0) {
            uint32_t i = 0;
            do {
                uint8_t* b = pBones + (static_cast<uint64_t>(i) << 7);
                Bswap32InPlace(b, 0x00);
                Bswap32InPlace(b, 0x04);
                Bswap32InPlace(b, 0x08);
                Bswap32InPlace(b, 0x0c);
                Bswap32InPlace(b, 0x10);
                Bswap32InPlace(b, 0x14);
                Bswap32InPlace(b, 0x18);
                Bswap16SignedInPlace(b, 0x1c);
                Bswap16SignedInPlace(b, 0x1e);
                Bswap32InPlace(b, 0x20);
                Bswap32InPlace(b, 0x24);
                Bswap32InPlace(b, 0x28);
                Bswap16SignedInPlace(b, 0x2c);
                Bswap16SignedInPlace(b, 0x2e);
                Bswap32InPlace(b, 0x30);
                Bswap32InPlace(b, 0x34);
                Bswap32InPlace(b, 0x38);
                Bswap32InPlace(b, 0x3c);
                Bswap32InPlace(b, 0x40);
                Bswap32InPlace(b, 0x44);
                Bswap32InPlace(b, 0x48);
                ++i;
                hdr = *reinterpret_cast<uint8_t**>(cont + 0x00);
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x1c));
        }

        // 4. Byte-swap Meshes (48 B each)
        if (*reinterpret_cast<const uint32_t*>(hdr + 0x20) != 0) {
            uint8_t* meshBase = *reinterpret_cast<uint8_t**>(cont + 0x20);
            uint32_t i = 0;
            do {
                uint8_t* m = meshBase + (static_cast<uint64_t>(i) * 3ULL << 4);
                Bswap32InPlace(m, 0x04);
                Bswap32InPlace(m, 0x10);
                Bswap32InPlace(m, 0x14);
                Bswap32InPlace(m, 0x1c);
                Bswap32InPlace(m, 0x20);
                Bswap32InPlace(m, 0x24);
                Bswap32InPlace(m, 0x28);
                Bswap32InPlace(m, 0x2c);
                ++i;
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x20));
        }

        // 5. Byte-swap FaceSets (16 B or 32 B each)
        uint32_t fsStride = (*reinterpret_cast<const uint32_t*>(hdr + 0x08) > 0x20008u) ? 0x20u : 0x10u;
        if (*reinterpret_cast<const uint32_t*>(hdr + 0x50) != 0) {
            uint8_t* fsBase = *reinterpret_cast<uint8_t**>(cont + 0x28);
            uint32_t byteOff = 0;
            uint32_t i = 0;
            do {
                uint8_t* fs = fsBase + byteOff;
                Bswap32InPlace(fs, 0x00);
                Bswap32InPlace(fs, 0x08);
                Bswap32InPlace(fs, 0x0c);
                if (*reinterpret_cast<const uint32_t*>(hdr + 0x08) >= 0x20009u) {
                    Bswap32InPlace(fs, 0x10);
                    Bswap32InPlace(fs, 0x14);
                }
                ++i;
                byteOff += fsStride;
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x50));
        }

        // 6. Byte-swap VertexBuffers (32 B each)
        if (*reinterpret_cast<const uint32_t*>(hdr + 0x24) != 0) {
            uint8_t* vbBase = *reinterpret_cast<uint8_t**>(cont + 0x30);
            uint32_t i = 0;
            do {
                uint8_t* vb = vbBase + (static_cast<uint64_t>(i) << 5);
                Bswap32InPlace(vb, 0x00);
                Bswap32InPlace(vb, 0x04);
                Bswap32InPlace(vb, 0x08);
                Bswap32InPlace(vb, 0x0c);
                Bswap32InPlace(vb, 0x10);
                Bswap32InPlace(vb, 0x14);
                Bswap32InPlace(vb, 0x18);
                Bswap32InPlace(vb, 0x1c);
                ++i;
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x24));
        }

        // 7. Byte-swap BufferLayouts (16 B each)
        if (*reinterpret_cast<const uint32_t*>(hdr + 0x54) != 0) {
            uint8_t* blBase = *reinterpret_cast<uint8_t**>(cont + 0x38);
            uint32_t i = 0;
            do {
                uint8_t* bl = blBase + (static_cast<uint64_t>(i) << 4);
                Bswap32InPlace(bl, 0x00);
                Bswap32InPlace(bl, 0x0c);
                ++i;
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x54));
        }

        // 8. Byte-swap Textures (16 B or 32 B each)
        if (*reinterpret_cast<const uint32_t*>(hdr + 0x58) != 0) {
            uint8_t* txBase = *reinterpret_cast<uint8_t**>(cont + 0x40);
            uint32_t ver = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
            uint32_t off16 = 0;
            uint32_t i = 0;
            do {
                uint64_t txOff = (ver <= 0x20002u)
                    ? static_cast<uint64_t>(off16)
                    : (static_cast<uint64_t>(i) << 5);
                uint8_t* tx = txBase + txOff;
                Bswap32InPlace(tx, 0x00);
                Bswap32InPlace(tx, 0x04);
                ver = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
                if (ver >= 0x20004u) {
                    Bswap32InPlace(tx, 0x08);
                    Bswap32InPlace(tx, 0x0c);
                    if (ver >= 0x2000au) {
                        Bswap32InPlace(tx, 0x14);
                        Bswap32InPlace(tx, 0x18);
                        Bswap32InPlace(tx, 0x1c);
                    }
                }
                ++i;
                off16 += 0x10u;
            } while (i < *reinterpret_cast<const uint32_t*>(hdr + 0x58));
        }

        // 9. Byte-swap BoneIndexPairs (+0x50) and Mesh Bone Index Table
        uint32_t pairCount = *reinterpret_cast<const uint32_t*>(hdr + 0x64);
        if (pairCount != 0) {
            auto* pairs = *reinterpret_cast<uint16_t**>(cont + 0x50);
            int32_t maxIdx = -1;
            int32_t prevMax = -1;
            uint32_t lastA = 0;
            uint32_t lastB = 0;
            for (uint32_t i = 0; i < pairCount; ++i) {
                prevMax = maxIdx;
                uint16_t a = Rol16By8(pairs[i * 2 + 0]);
                pairs[i * 2 + 0] = a;
                uint16_t b = Rol16By8(pairs[i * 2 + 1]);
                pairs[i * 2 + 1] = b;
                lastA = a;
                lastB = b;
                int32_t endIdx = static_cast<int32_t>(lastA + lastB) - 1;
                maxIdx = (prevMax >= endIdx) ? prevMax : endIdx;
            }
            if (maxIdx >= 0) {
                uint8_t* firstMesh = *reinterpret_cast<uint8_t**>(cont + 0x20);
                if (firstMesh != nullptr) {
                    uint32_t boneIndicesOff = *reinterpret_cast<const uint32_t*>(firstMesh + 0x1c);
                    if (boneIndicesOff != 0 && maxIdx != -1) {
                        uint8_t* boneIdxPtr = hdr + boneIndicesOff;
                        int32_t lastEnd = static_cast<int32_t>(lastA + lastB) - 1;
                        int32_t countMinus1 = (prevMax >= lastEnd) ? prevMax : lastEnd;
                        uint32_t totalIndices = static_cast<uint32_t>(countMinus1 + 1);
                        do {
                            Bswap32InPlace(boneIdxPtr, 0);
                            boneIdxPtr += 4;
                            --totalIndices;
                        } while (totalIndices != 0);
                    }
                }
            }
        }
    }

    int32_t rawUvFactor = static_cast<int8_t>(hdr[0x48]);
    cont[0x60] = static_cast<uint8_t>(rawUvFactor / 8);
    return true;
}

// ============================================================================
// 15. 0x21ce560 (100 B) - GXModelFlverContainer_Construct
// ============================================================================
void sub_21ce560(void* flverContainer) {
    auto* cont = static_cast<uint8_t*>(flverContainer);
    cont[0x60] = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x58) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x50) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x48) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(cont + 0x00) = 0;
}

// ============================================================================
// 16. 0x21ce550 (10 B) - GXFlverDoubleBufferedSlot_GetActivePtr
// ============================================================================
void* sub_21ce550(const void* slot) {
    auto* s = static_cast<const uint8_t*>(slot);
    int64_t activeIdx = *reinterpret_cast<const int32_t*>(s + 0x60);
    return *reinterpret_cast<void* const*>(s + activeIdx * 8 + 0x50);
}

// ============================================================================
// 17. 0x21ce470 (69 B) - GXFlverNamedResourceBase_Destroy
// ============================================================================
void sub_21ce470(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x536bdd0ULL);
    if (*reinterpret_cast<uint64_t*>(s + 0x30) >= 8ULL) {
        void* strBuf = *reinterpret_cast<void**>(s + 0x18);
        void* alloc  = *reinterpret_cast<void**>(s + 0x38);
        auto* vtbl   = *reinterpret_cast<uintptr_t**>(alloc);
        reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(alloc, strBuf);
    }
    *reinterpret_cast<uint64_t*>(s + 0x30) = 7ULL;
    *reinterpret_cast<uint64_t*>(s + 0x28) = 0ULL;
    *reinterpret_cast<uint16_t*>(s + 0x18) = 0u;
}

// ============================================================================
// 18. 0x21ce430 (54 B) - GXFlverNamedResourceBase_DeletingDestructor
// ============================================================================
void sub_21ce430(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x536bdd0ULL);
    if (*reinterpret_cast<uint64_t*>(s + 0x30) >= 8ULL) {
        void* strBuf = *reinterpret_cast<void**>(s + 0x18);
        void* alloc  = *reinterpret_cast<void**>(s + 0x38);
        auto* vtbl   = *reinterpret_cast<uintptr_t**>(alloc);
        reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(alloc, strBuf);
    }
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// 19. 0x21ce340 (235 B) - GXFlverDoubleBufferedSlot_Construct
// ============================================================================
void sub_21ce340(void* self, void* ownerAllocator, void* ptr0, void* ptr1) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint32_t*>(s + 0x08) = 0u;
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x536bdd0ULL);

    auto** pGlobalStrAlloc = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* strAlloc = *pGlobalStrAlloc;
    if (strAlloc == nullptr) {
        strAlloc = reinterpret_cast<FnGetDefaultStringAllocator>(GX_VMA(0x2ba2b10ULL))();
        *pGlobalStrAlloc = strAlloc;
    }

    uint64_t heapFlags = 0;
    auto* strAllocVtbl = *reinterpret_cast<uintptr_t**>(strAlloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(strAllocVtbl[0x20 / 8])(
        &heapFlags, strAlloc, 0u);
    if ((heapFlags & 0x20ULL) == 0) {
        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
            nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<void**>(s + 0x38)    = strAlloc;
    *reinterpret_cast<uint64_t*>(s + 0x30) = 7ULL;
    *reinterpret_cast<uint64_t*>(s + 0x28) = 0ULL;
    *reinterpret_cast<uint16_t*>(s + 0x18) = 0u;
    s[0x40]                                = 1u;
    *reinterpret_cast<void**>(s + 0x48)    = ownerAllocator;
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x535fe40ULL);
    *reinterpret_cast<uint32_t*>(s + 0x60) = 0u;
    *reinterpret_cast<uint32_t*>(s + 0x64) = 0x3f000000u; // 0.5f
    *reinterpret_cast<void**>(s + 0x50)    = ptr0;
    *reinterpret_cast<void**>(s + 0x58)    = ptr1;
}

// ============================================================================
// 20. 0x21ce1b0 (388 B) - GXRefCountedPtrVector_EraseRange
// ============================================================================
void* sub_21ce1b0(void* outIter, void* vec, void* last,
                  uint64_t /*padRcx*/, uint64_t /*padR8*/, uint64_t /*padR9*/,
                  uint64_t /*stackArg0*/, uint64_t /*stackArg1*/, void* first,
                  uint64_t /*stackArg2*/, uint64_t /*stackArg3*/, void* stackLast) {
    auto* v      = static_cast<uint8_t*>(vec);
    auto** pBegin = *reinterpret_cast<uint8_t***>(v + 0x08);
    auto** pEnd   = *reinterpret_cast<uint8_t***>(v + 0x10);
    if (pBegin == pEnd) {
        *reinterpret_cast<void**>(static_cast<uint8_t*>(outIter) + 0x10) = pBegin;
        return outIter;
    }
    auto** candLastRdx   = static_cast<uint8_t**>(last);
    auto** candLastStack = static_cast<uint8_t**>(stackLast);
    auto** pLast = (candLastRdx >= pBegin && candLastRdx <= pEnd)
        ? candLastRdx
        : ((candLastStack >= pBegin && candLastStack <= pEnd) ? candLastStack : pEnd);
    auto** candFirst = static_cast<uint8_t**>(first);
    auto** pFirst = (candFirst >= pBegin && candFirst <= pLast) ? candFirst : pBegin;

    if (pFirst == pBegin && pEnd == pLast) {
        if (pBegin != pLast) {
            for (auto** cur = pBegin; cur != pLast; ++cur) {
                uint8_t* obj = *cur;
                if (obj != nullptr) {
                    int32_t oldRef = *reinterpret_cast<int32_t*>(obj + 0x08);
                    *reinterpret_cast<int32_t*>(obj + 0x08) = oldRef - 1;
                    if (oldRef <= 1) {
                        auto* vtbl = *reinterpret_cast<uintptr_t**>(obj);
                        reinterpret_cast<void (*)(void*)>(vtbl[0])(obj);
                    }
                    *cur = nullptr;
                }
            }
            pLast = *reinterpret_cast<uint8_t***>(v + 0x08);
        }
        *reinterpret_cast<uint8_t***>(v + 0x10) = pLast;
    } else if (pFirst != pLast) {
        auto** dst = pFirst;
        if (pEnd != pLast) {
            for (auto** src = pLast; src != pEnd; ++src, ++dst) {
                uint8_t* oldObj = *dst;
                uint8_t* newObj = *src;
                if (oldObj != newObj) {
                    if (oldObj != nullptr) {
                        int32_t oldRef = *reinterpret_cast<int32_t*>(oldObj + 0x08);
                        *reinterpret_cast<int32_t*>(oldObj + 0x08) = oldRef - 1;
                        if (oldRef <= 1) {
                            auto* vtbl = *reinterpret_cast<uintptr_t**>(oldObj);
                            reinterpret_cast<void (*)(void*)>(vtbl[0])(oldObj);
                        }
                        *dst = nullptr;
                        newObj = *src;
                    }
                    *dst = newObj;
                    if (newObj != nullptr) {
                        ++(*reinterpret_cast<int32_t*>(newObj + 0x08));
                    }
                }
            }
            pEnd = *reinterpret_cast<uint8_t***>(v + 0x10);
        }
        if (dst != pEnd) {
            for (auto** cur = dst; cur != pEnd; ++cur) {
                uint8_t* obj = *cur;
                if (obj != nullptr) {
                    int32_t oldRef = *reinterpret_cast<int32_t*>(obj + 0x08);
                    *reinterpret_cast<int32_t*>(obj + 0x08) = oldRef - 1;
                    if (oldRef <= 1) {
                        auto* vtbl = *reinterpret_cast<uintptr_t**>(obj);
                        reinterpret_cast<void (*)(void*)>(vtbl[0])(obj);
                    }
                    *cur = nullptr;
                }
            }
        }
        *reinterpret_cast<uint8_t***>(v + 0x10) = dst;
    }

    *reinterpret_cast<void**>(static_cast<uint8_t*>(outIter) + 0x10) = pFirst;
    return outIter;
}

// ============================================================================
// 21. 0x21cdcd0 (1,093 B) - GXFlverTextureResourceList_InitializeFromFlver
// ============================================================================
bool sub_21cdcd0(void* self, const void* flverContainer, void* textureResolver) {
    auto* s        = static_cast<uint8_t*>(self);
    auto* flver    = static_cast<const uint8_t*>(flverContainer);
    auto* flverHdr = *reinterpret_cast<const uint8_t* const*>(flver + 0x00);
    uint32_t texCount = *reinterpret_cast<const uint32_t*>(flverHdr + 0x58);

    uint8_t* vecPtr = s + 0x10;
    uint64_t curCap = static_cast<uint64_t>(
        (*reinterpret_cast<uint8_t**>(s + 0x28) - *reinterpret_cast<uint8_t**>(s + 0x18)) >> 3);
    if (curCap < texCount) {
        reinterpret_cast<FnRefPtrVecReserve>(GX_VMA(0x1448ea0ULL))(vecPtr, texCount);
    }

    void* slotAlloc = *reinterpret_cast<void**>(s + 0x40);
    auto* slotAllocVtbl = *reinterpret_cast<uintptr_t**>(slotAlloc);
    auto* slots = static_cast<uint8_t*>(
        reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(slotAllocVtbl[0x58 / 8])(
            slotAlloc, static_cast<uint64_t>(texCount) * 0x58ULL, 8u));
    *reinterpret_cast<uint8_t**>(s + 0x50) = slots;

    if (slots != nullptr) {
        if (texCount != 0) {
            for (uint32_t i = 0; i < texCount; ++i) {
                uint8_t* slot = *reinterpret_cast<uint8_t**>(s + 0x50) + static_cast<uint64_t>(i) * 0x58ULL;
                if (slot != nullptr) {
                    void* ownerField = *reinterpret_cast<void**>(s + 0x38);
                    *reinterpret_cast<uint32_t*>(slot + 0x08) = 0u;
                    *reinterpret_cast<uintptr_t*>(slot + 0x00) = GX_VMA(0x536bdd0ULL);

                    auto** pGlobalStrAlloc = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
                    void* strAlloc = *pGlobalStrAlloc;
                    if (strAlloc == nullptr) {
                        strAlloc = reinterpret_cast<FnGetDefaultStringAllocator>(GX_VMA(0x2ba2b10ULL))();
                        *pGlobalStrAlloc = strAlloc;
                    }

                    uint64_t heapFlags = 0;
                    auto* strAllocVtbl = *reinterpret_cast<uintptr_t**>(strAlloc);
                    reinterpret_cast<void (*)(void*, void*, uint32_t)>(strAllocVtbl[0x20 / 8])(
                        &heapFlags, strAlloc, 0u);
                    if ((heapFlags & 0x20ULL) == 0) {
                        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                            nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
                    }

                    *reinterpret_cast<void**>(slot + 0x38)    = strAlloc;
                    *reinterpret_cast<uint64_t*>(slot + 0x30) = 7ULL;
                    *reinterpret_cast<uint64_t*>(slot + 0x28) = 0ULL;
                    *reinterpret_cast<uint16_t*>(slot + 0x18) = 0u;
                    slot[0x40]                                = 1u;
                    *reinterpret_cast<void**>(slot + 0x48)    = ownerField;
                    *reinterpret_cast<uintptr_t*>(slot + 0x00) = GX_VMA(0x53215a0ULL);
                    *reinterpret_cast<void**>(slot + 0x50)    = nullptr;
                }
            }
        }
        *reinterpret_cast<uint64_t*>(s + 0x48) = texCount;
    }

    if (texCount != 0) {
        uint64_t slotByteOff = 0;
        uint64_t off16 = 0;
        uint64_t off32 = 0;
        uint32_t remaining = texCount;
        char utf8Buf[128];

        do {
            const uint8_t* hdr = *reinterpret_cast<const uint8_t* const*>(flver + 0x00);
            const uint8_t* texTable = *reinterpret_cast<const uint8_t* const*>(flver + 0x40);
            uint32_t ver = *reinterpret_cast<const uint32_t*>(hdr + 0x08);
            const uint8_t* texEntry = (ver <= 0x20002u)
                ? (texTable + (off16 & 0xFFFFFFF0ULL))
                : (texTable + off32);
            uint32_t pathOff = *reinterpret_cast<const uint32_t*>(texEntry + 0x00);
            const auto* widePath = reinterpret_cast<const char16_t*>(hdr + pathOff);

            auto* resolverVtbl = *reinterpret_cast<uintptr_t**>(textureResolver);
            auto* resolvedTex = static_cast<uint8_t*>(
                reinterpret_cast<void* (*)(void*, const char16_t*)>(resolverVtbl[0x38 / 8])(
                    textureResolver, widePath));

            reinterpret_cast<FnWideToUtf8>(GX_VMA(0x21bc840ULL))(widePath, utf8Buf, 0x40u);

            uint8_t* slotBase = *reinterpret_cast<uint8_t**>(s + 0x50);
            uint8_t* slot = slotBase + slotByteOff;
            ++(*reinterpret_cast<int32_t*>(slot + 0x08));

            uint64_t strLen = 0;
            if (*reinterpret_cast<const uint16_t*>(utf8Buf) != 0) {
                strLen = reinterpret_cast<FnStrLen>(GX_VMA(0x2bbf228ULL))(utf8Buf);
            }
            reinterpret_cast<FnDLStringAssign>(GX_VMA(0x262a720ULL))(slot + 0x10, utf8Buf, strLen);

            uint8_t* oldTex = *reinterpret_cast<uint8_t**>(slot + 0x50);
            if (oldTex != resolvedTex) {
                if (oldTex != nullptr) {
                    int32_t oldRef = *reinterpret_cast<int32_t*>(oldTex + 0x08);
                    *reinterpret_cast<int32_t*>(oldTex + 0x08) = oldRef - 1;
                    if (oldRef <= 1) {
                        auto* vtbl = *reinterpret_cast<uintptr_t**>(oldTex);
                        reinterpret_cast<void (*)(void*)>(vtbl[0])(oldTex);
                    }
                    *reinterpret_cast<void**>(slot + 0x50) = nullptr;
                }
                *reinterpret_cast<uint8_t**>(slot + 0x50) = resolvedTex;
                if (resolvedTex != nullptr) {
                    ++(*reinterpret_cast<int32_t*>(resolvedTex + 0x08));
                }
            }

            uint8_t* tempSlotPtr = slot;
            if (tempSlotPtr != nullptr) {
                ++(*reinterpret_cast<int32_t*>(slot + 0x08));
            }

            auto** vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
            auto** vEnd   = *reinterpret_cast<uint8_t***>(s + 0x20);
            auto** vCap   = *reinterpret_cast<uint8_t***>(s + 0x28);
            uint8_t** addrOfTemp = &tempSlotPtr;

            if (vEnd > addrOfTemp && vBegin <= addrOfTemp) {
                int64_t relIdx = addrOfTemp - vBegin;
                if (vEnd == vCap) {
                    int64_t curSize = vEnd - vBegin;
                    uint64_t half = static_cast<uint64_t>(curSize) >> 1;
                    uint64_t newCap = (0x1FFFFFFFFFFFFFFFULL - half >= static_cast<uint64_t>(curSize))
                        ? (static_cast<uint64_t>(curSize) + half)
                        : 0ULL;
                    if (newCap < static_cast<uint64_t>(curSize + 1)) {
                        newCap = static_cast<uint64_t>(curSize + 1);
                    }
                    reinterpret_cast<FnRefPtrVecReserve>(GX_VMA(0x1448ea0ULL))(vecPtr, newCap);
                    vEnd = *reinterpret_cast<uint8_t***>(s + 0x20);
                }
                if (vEnd != nullptr) {
                    uint8_t* elem = (*reinterpret_cast<uint8_t***>(s + 0x18))[relIdx];
                    *vEnd = elem;
                    if (elem != nullptr) {
                        ++(*reinterpret_cast<int32_t*>(elem + 0x08));
                    }
                }
            } else {
                if (vEnd == vCap) {
                    int64_t curSize = vEnd - vBegin;
                    uint64_t half = static_cast<uint64_t>(curSize) >> 1;
                    uint64_t newCap = (0x1FFFFFFFFFFFFFFFULL - half >= static_cast<uint64_t>(curSize))
                        ? (static_cast<uint64_t>(curSize) + half)
                        : 0ULL;
                    if (newCap < static_cast<uint64_t>(curSize + 1)) {
                        newCap = static_cast<uint64_t>(curSize + 1);
                    }
                    reinterpret_cast<FnRefPtrVecReserve>(GX_VMA(0x1448ea0ULL))(vecPtr, newCap);
                    vEnd = *reinterpret_cast<uint8_t***>(s + 0x20);
                }
                if (vEnd != nullptr) {
                    *vEnd = tempSlotPtr;
                    if (tempSlotPtr != nullptr) {
                        ++(*reinterpret_cast<int32_t*>(tempSlotPtr + 0x08));
                    }
                }
            }
            *reinterpret_cast<uint8_t***>(s + 0x20) += 1;

            if (tempSlotPtr != nullptr) {
                int32_t oldRef = *reinterpret_cast<int32_t*>(tempSlotPtr + 0x08);
                *reinterpret_cast<int32_t*>(tempSlotPtr + 0x08) = oldRef - 1;
                if (oldRef <= 1) {
                    auto* vtbl = *reinterpret_cast<uintptr_t**>(tempSlotPtr);
                    reinterpret_cast<void (*)(void*)>(vtbl[0])(tempSlotPtr);
                }
                tempSlotPtr = nullptr;
            }

            off16 += 0x10ULL;
            off32 += 0x20ULL;
            slotByteOff += 0x58ULL;
            --remaining;
        } while (remaining != 0);
    }

    return true;
}

// ============================================================================
// 22. 0x21cdbe0 (228 B) - GXFlverTextureResourceList_ClearAndFreeSlots
// ============================================================================
void sub_21cdbe0(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    uint64_t iterOut[4] = {};
    void* vecEnd = *reinterpret_cast<void**>(s + 0x20);
    sub_21ce1b0(iterOut, s + 0x10,
                vecEnd,
                0, 0, 0, 0, 0,
                *reinterpret_cast<void**>(s + 0x18),
                0, 0, vecEnd);

    uint8_t* slots = *reinterpret_cast<uint8_t**>(s + 0x50);
    if (slots != nullptr) {
        int32_t count = static_cast<int32_t>(*reinterpret_cast<uint64_t*>(s + 0x48));
        if (count > 0) {
            int64_t idx = static_cast<int64_t>(count) - 1;
            int32_t rem = static_cast<int32_t>(idx + 1);
            uint8_t* cur = slots + idx * 0x58LL;
            do {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(cur);
                reinterpret_cast<void (*)(void*)>(vtbl[1])(cur);
                --rem;
                cur -= 0x58;
            } while (rem > 0);
            slots = *reinterpret_cast<uint8_t**>(s + 0x50);
        }
        void* alloc = *reinterpret_cast<void**>(s + 0x40);
        auto* allocVtbl = *reinterpret_cast<uintptr_t**>(alloc);
        reinterpret_cast<void (*)(void*, void*)>(allocVtbl[0x70 / 8])(alloc, slots);
        *reinterpret_cast<uint64_t*>(s + 0x50) = 0ULL;
        *reinterpret_cast<uint64_t*>(s + 0x48) = 0ULL;
    }
}

// ============================================================================
// 23. 0x21cda20 (433 B) - GXFlverTextureResourceList_Destroy
// ============================================================================
void sub_21cda20(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6f20ULL);

    uint64_t iterOut[4] = {};

    uint8_t* slots = *reinterpret_cast<uint8_t**>(s + 0x50);
    if (slots != nullptr) {
        int32_t count = static_cast<int32_t>(*reinterpret_cast<uint64_t*>(s + 0x48));
        if (count > 0) {
            int64_t idx = static_cast<int64_t>(count) - 1;
            int32_t rem = static_cast<int32_t>(idx + 1);
            uint8_t* cur = slots + idx * 0x58LL;
            do {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(cur);
                reinterpret_cast<void (*)(void*)>(vtbl[1])(cur);
                --rem;
                cur -= 0x58;
            } while (rem > 0);
            slots = *reinterpret_cast<uint8_t**>(s + 0x50);
        }
        void* alloc = *reinterpret_cast<void**>(s + 0x40);
        auto* allocVtbl = *reinterpret_cast<uintptr_t**>(alloc);
        reinterpret_cast<void (*)(void*, void*)>(allocVtbl[0x70 / 8])(alloc, slots);
        *reinterpret_cast<uint64_t*>(s + 0x50) = 0ULL;
        *reinterpret_cast<uint64_t*>(s + 0x48) = 0ULL;
    }

    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6ee0ULL);
    void* vecEnd2 = *reinterpret_cast<void**>(s + 0x20);
    sub_21ce1b0(iterOut, s + 0x10,
                vecEnd2,
                0, 0, 0, 0, 0,
                *reinterpret_cast<void**>(s + 0x18),
                0, 0, vecEnd2);

    auto** vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
    if (vBegin != nullptr) {
        auto** vEnd = *reinterpret_cast<uint8_t***>(s + 0x20);
        for (auto** cur = vBegin; cur != vEnd; ++cur) {
            uint8_t* obj = *cur;
            if (obj != nullptr) {
                int32_t oldRef = *reinterpret_cast<int32_t*>(obj + 0x08);
                *reinterpret_cast<int32_t*>(obj + 0x08) = oldRef - 1;
                if (oldRef <= 1) {
                    auto* vtbl = *reinterpret_cast<uintptr_t**>(obj);
                    reinterpret_cast<void (*)(void*)>(vtbl[0])(obj);
                }
                *cur = nullptr;
            }
        }
        vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
        void* vecAlloc = *reinterpret_cast<void**>(s + 0x30);
        auto* vecAllocVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
        reinterpret_cast<void (*)(void*, void*)>(vecAllocVtbl[0x70 / 8])(vecAlloc, vBegin);
        *reinterpret_cast<uint64_t*>(s + 0x28) = 0ULL;
        *reinterpret_cast<uint64_t*>(s + 0x20) = 0ULL;
        *reinterpret_cast<uint64_t*>(s + 0x18) = 0ULL;
    }
}

// ============================================================================
// 24. 0x21cda00 (28 B) - GXFlverTextureResourceList_DeletingDestructor
// ============================================================================
void sub_21cda00(void* self) {
    sub_21cda20(self);
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// 25. 0x21cd920 (218 B) - GXFlverTextureResourceList_Construct
// ============================================================================
void sub_21cd920(void* self, void* customAllocator) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint32_t*>(s + 0x08) = 0u;
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6ee0ULL);

    auto** pGlobalAlloc = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* vecAlloc = *pGlobalAlloc;
    if (vecAlloc == nullptr) {
        vecAlloc = reinterpret_cast<FnGetDefaultStringAllocator>(GX_VMA(0x2ba2b10ULL))();
        *pGlobalAlloc = vecAlloc;
    }

    uint64_t heapFlags = 0;
    auto* vecAllocVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(vecAllocVtbl[0x20 / 8])(
        &heapFlags, vecAlloc, 0u);
    if ((heapFlags & 0x20ULL) == 0) {
        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
            nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(s + 0x28) = 0ULL;
    *reinterpret_cast<uint64_t*>(s + 0x20) = 0ULL;
    *reinterpret_cast<uint64_t*>(s + 0x18) = 0ULL;
    *reinterpret_cast<void**>(s + 0x30)    = vecAlloc;
    *reinterpret_cast<void**>(s + 0x38)    = customAllocator;
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6f20ULL);
    *reinterpret_cast<void**>(s + 0x40)    = customAllocator;
    *reinterpret_cast<uint64_t*>(s + 0x50) = 0ULL;
    *reinterpret_cast<uint64_t*>(s + 0x48) = 0ULL;
}

// ============================================================================
// 26. 0x21cd900 (25 B) - GXFlverResourceVector_GetElementNameCStr
// ============================================================================
const char16_t* sub_21cd900(const void* self, uint32_t index) {
    auto* s    = static_cast<const uint8_t*>(self);
    auto* arr  = *reinterpret_cast<const uint8_t* const*>(s + 0x18);
    auto* elem = *reinterpret_cast<const uint8_t* const*>(arr + static_cast<uint64_t>(index) * 8ULL);
    const uint8_t* strField = elem + 0x18;
    if (*reinterpret_cast<const uint64_t*>(elem + 0x30) >= 8ULL) {
        return *reinterpret_cast<const char16_t* const*>(strField);
    }
    return reinterpret_cast<const char16_t*>(strField);
}

// ============================================================================
// 27. 0x21cd8f0 (11 B) - GXFlverResourceVector_GetElementAt
// ============================================================================
void* sub_21cd8f0(const void* self, uint32_t index) {
    auto* s   = static_cast<const uint8_t*>(self);
    auto* arr = *reinterpret_cast<void* const* const*>(s + 0x18);
    return arr[index];
}

// ============================================================================
// 28. 0x21cd8e0 (16 B) - GXFlverResourceVector_GetElementPayloadAt
// ============================================================================
void* sub_21cd8e0(const void* self, uint32_t index) {
    auto* s    = static_cast<const uint8_t*>(self);
    auto* arr  = *reinterpret_cast<void* const* const*>(s + 0x18);
    void* elem = arr[index];
    auto* vtbl = *reinterpret_cast<uintptr_t**>(elem);
    return reinterpret_cast<void* (*)(void*)>(vtbl[0x18 / 8])(elem);
}

// ============================================================================
// 29. 0x21cd8d0 (13 B) - GXFlverResourceVector_GetCount
// ============================================================================
uint64_t sub_21cd8d0(const void* self) {
    auto* s = static_cast<const uint8_t*>(self);
    return static_cast<uint64_t>(
        *reinterpret_cast<const uint8_t* const*>(s + 0x20) -
        *reinterpret_cast<const uint8_t* const*>(s + 0x18)) >> 3;
}

// ============================================================================
// 30. 0x21cd860 (112 B) - GXFlverResourceVector_Clear
// ============================================================================
void sub_21cd860(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    uint64_t iterOut[4] = {};
    void* vecEnd = *reinterpret_cast<void**>(s + 0x20);
    sub_21ce1b0(iterOut, s + 0x10,
                vecEnd,
                0, 0, 0, 0, 0,
                *reinterpret_cast<void**>(s + 0x18),
                0, 0, vecEnd);
}

// ============================================================================
// 31. 0x21cd750 (260 B) - GXFlverResourceVector_Destroy
// ============================================================================
void sub_21cd750(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6ee0ULL);
    uint64_t iterOut[4] = {};
    void* vecEnd = *reinterpret_cast<void**>(s + 0x20);
    sub_21ce1b0(iterOut, s + 0x10,
                vecEnd,
                0, 0, 0, 0, 0,
                *reinterpret_cast<void**>(s + 0x18),
                0, 0, vecEnd);

    auto** vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
    if (vBegin != nullptr) {
        auto** vEnd = *reinterpret_cast<uint8_t***>(s + 0x20);
        for (auto** cur = vBegin; cur != vEnd; ++cur) {
            uint8_t* obj = *cur;
            if (obj != nullptr) {
                int32_t oldRef = *reinterpret_cast<int32_t*>(obj + 0x08);
                *reinterpret_cast<int32_t*>(obj + 0x08) = oldRef - 1;
                if (oldRef <= 1) {
                    auto* vtbl = *reinterpret_cast<uintptr_t**>(obj);
                    reinterpret_cast<void (*)(void*)>(vtbl[0])(obj);
                }
                *cur = nullptr;
            }
        }
        vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
        void* vecAlloc = *reinterpret_cast<void**>(s + 0x30);
        auto* vecAllocVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
        reinterpret_cast<void (*)(void*, void*)>(vecAllocVtbl[0x70 / 8])(vecAlloc, vBegin);
        *reinterpret_cast<uint64_t*>(s + 0x28) = 0ULL;
        *reinterpret_cast<uint64_t*>(s + 0x20) = 0ULL;
        *reinterpret_cast<uint64_t*>(s + 0x18) = 0ULL;
    }
}

// ============================================================================
// 32. 0x21cd670 (223 B) - GXFlverResourceVector_DeletingDestructor
// ============================================================================
void sub_21cd670(void* self) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6ee0ULL);
    uint64_t iterOut[4] = {};
    void* vecEnd = *reinterpret_cast<void**>(s + 0x20);
    sub_21ce1b0(iterOut, s + 0x10,
                vecEnd,
                0, 0, 0, 0, 0,
                *reinterpret_cast<void**>(s + 0x18),
                0, 0, vecEnd);

    auto** vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
    if (vBegin != nullptr) {
        auto** vEnd = *reinterpret_cast<uint8_t***>(s + 0x20);
        for (auto** cur = vBegin; cur != vEnd; ++cur) {
            uint8_t* obj = *cur;
            if (obj != nullptr) {
                int32_t oldRef = *reinterpret_cast<int32_t*>(obj + 0x08);
                *reinterpret_cast<int32_t*>(obj + 0x08) = oldRef - 1;
                if (oldRef <= 1) {
                    auto* vtbl = *reinterpret_cast<uintptr_t**>(obj);
                    reinterpret_cast<void (*)(void*)>(vtbl[0])(obj);
                }
                *cur = nullptr;
            }
        }
        vBegin = *reinterpret_cast<uint8_t***>(s + 0x18);
        void* vecAlloc = *reinterpret_cast<void**>(s + 0x30);
        auto* vecAllocVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
        reinterpret_cast<void (*)(void*, void*)>(vecAllocVtbl[0x70 / 8])(vecAlloc, vBegin);
    }
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// 33. 0x21cd5b0 (188 B) - GXFlverResourceVector_Construct
// ============================================================================
void sub_21cd5b0(void* self, void* customAllocator) {
    auto* s = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint32_t*>(s + 0x08) = 0u;
    *reinterpret_cast<uintptr_t*>(s + 0x00) = GX_VMA(0x53b6ee0ULL);

    auto** pGlobalAlloc = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* vecAlloc = *pGlobalAlloc;
    if (vecAlloc == nullptr) {
        vecAlloc = reinterpret_cast<FnGetDefaultStringAllocator>(GX_VMA(0x2ba2b10ULL))();
        *pGlobalAlloc = vecAlloc;
    }

    uint64_t heapFlags = 0;
    auto* vecAllocVtbl = *reinterpret_cast<uintptr_t**>(vecAlloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(vecAllocVtbl[0x20 / 8])(
        &heapFlags, vecAlloc, 0u);
    if ((heapFlags & 0x20ULL) == 0) {
        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
            nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(s + 0x28) = 0ULL;
    *reinterpret_cast<uint64_t*>(s + 0x20) = 0ULL;
    *reinterpret_cast<uint64_t*>(s + 0x18) = 0ULL;
    *reinterpret_cast<void**>(s + 0x30)    = vecAlloc;
    *reinterpret_cast<void**>(s + 0x38)    = customAllocator;
}

// ============================================================================
// 34. 0x21cd130 (1,137 B) - GXSkinningSegmentVector_CopyAssignWithRotate
// ============================================================================
void sub_21cd130(void* dstVec, const void* srcVec) {
    auto* dst = static_cast<uint8_t*>(dstVec);
    auto* src = static_cast<const uint8_t*>(srcVec);
    if (dst == src) {
        return;
    }

    uint8_t* oldBegin = *reinterpret_cast<uint8_t**>(dst + 0x08);
    if (oldBegin != nullptr) {
        void* dstAlloc = *reinterpret_cast<void**>(dst + 0x20);
        auto* vtbl     = *reinterpret_cast<uintptr_t**>(dstAlloc);
        reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(dstAlloc, oldBegin);
        *reinterpret_cast<uint64_t*>(dst + 0x18) = 0ULL;
        *reinterpret_cast<uint64_t*>(dst + 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(dst + 0x08) = 0ULL;
    }

    void* dstAlloc2 = *reinterpret_cast<void**>(dst + 0x20);
    void* srcAlloc2 = *reinterpret_cast<void* const*>(src + 0x20);
    auto* vtblD2    = *reinterpret_cast<uintptr_t**>(dstAlloc2);
    uint32_t idD    = reinterpret_cast<uint32_t (*)(void*)>(vtblD2[0x10 / 8])(dstAlloc2);
    auto* vtblS2    = *reinterpret_cast<uintptr_t**>(srcAlloc2);
    uint32_t idS    = reinterpret_cast<uint32_t (*)(void*)>(vtblS2[0x10 / 8])(srcAlloc2);

    if (idD == idS) {
        auto* mutSrc = const_cast<uint8_t*>(src);
        __m128 ptrs01 = _mm_loadu_ps(reinterpret_cast<const float*>(mutSrc + 0x08));
        _mm_storeu_ps(reinterpret_cast<float*>(dst + 0x08), ptrs01);
        *reinterpret_cast<uint64_t*>(dst + 0x18)    = *reinterpret_cast<uint64_t*>(mutSrc + 0x18);
        *reinterpret_cast<uint64_t*>(mutSrc + 0x18) = 0ULL;
        *reinterpret_cast<uint64_t*>(mutSrc + 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(mutSrc + 0x08) = 0ULL;
        return;
    }

    const uint8_t* sBegin = *reinterpret_cast<const uint8_t* const*>(src + 0x08);
    const uint8_t* sEnd   = *reinterpret_cast<const uint8_t* const*>(src + 0x10);
    uint8_t* dPos         = *reinterpret_cast<uint8_t**>(dst + 0x08);
    *reinterpret_cast<uint8_t**>(dst + 0x10) = dPos;

    int64_t count = (sEnd - sBegin) >> 4;
    if (count == 0) {
        return;
    }

    int64_t availCap = (*reinterpret_cast<uint8_t**>(dst + 0x18) - dPos) >> 4;
    if (static_cast<uint64_t>(availCap) < static_cast<uint64_t>(count)) {
        if ((static_cast<uint64_t>(count) >> 60) != 0) {
            reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
        }
        uint64_t half = static_cast<uint64_t>(availCap) >> 1;
        uint64_t grown = (0x0FFFFFFFFFFFFFFFULL - half >= static_cast<uint64_t>(availCap))
            ? (static_cast<uint64_t>(availCap) + half)
            : 0ULL;
        if (grown < static_cast<uint64_t>(count)) {
            grown = static_cast<uint64_t>(count);
        }
        uint64_t newBytes = grown << 4;
        void* alloc = *reinterpret_cast<void**>(dst + 0x20);
        auto* allocVtbl = *reinterpret_cast<uintptr_t**>(alloc);
        auto* newBuf = static_cast<uint8_t*>(
            reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(allocVtbl[0x58 / 8])(
                alloc, newBytes, 8u));
        if (newBuf == nullptr) {
            auto* pIgnore = reinterpret_cast<int32_t*>(ReadTlsQwordAtOffset(0x53e4318ULL));
            if (*pIgnore != 0) {
                reinterpret_cast<FnSetErrnoError>(GX_VMA(0x2bbe978ULL))(
                    reinterpret_cast<void*>(ReadTlsQwordAtOffset(0x53e4320ULL)), 1u);
            } else {
                reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                    nullptr, 0x109u, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
            }
        }

        uint8_t* dBegin = *reinterpret_cast<uint8_t**>(dst + 0x08);
        uint8_t* writePtr = newBuf;
        if (dBegin != dPos) {
            uint64_t prefixBytes = static_cast<uint64_t>(dPos - dBegin);
            for (uint64_t off = 0; off != prefixBytes; off += 0x10ULL) {
                if (reinterpret_cast<uintptr_t>(newBuf) + off != 0) {
                    uint64_t q0 = *reinterpret_cast<const uint64_t*>(dBegin + off);
                    uint64_t q1 = *reinterpret_cast<const uint64_t*>(dBegin + off + 8);
                    *reinterpret_cast<uint64_t*>(newBuf + off + 8) = q1;
                    *reinterpret_cast<uint64_t*>(newBuf + off)     = q0;
                }
            }
            writePtr = newBuf + (((static_cast<uint64_t>(dPos - 0x10 - dBegin)) & ~0xFULL) + 0x10ULL);
        }

        if (sBegin != sEnd) {
            uint8_t* curOut = writePtr;
            for (const uint8_t* curIn = sBegin; curIn != sEnd; curIn += 0x10, curOut += 0x10) {
                if (curOut != nullptr) {
                    uint64_t q0 = *reinterpret_cast<const uint64_t*>(curIn);
                    uint64_t q1 = *reinterpret_cast<const uint64_t*>(curIn + 8);
                    *reinterpret_cast<uint64_t*>(curOut + 8) = q1;
                    *reinterpret_cast<uint64_t*>(curOut)     = q0;
                }
            }
            writePtr += (((static_cast<uint64_t>(sEnd - 0x10 - sBegin)) & ~0xFULL) + 0x10ULL);
        }

        uint8_t* dEnd = *reinterpret_cast<uint8_t**>(dst + 0x10);
        if (dPos != dEnd) {
            for (const uint8_t* curTail = dPos; curTail != dEnd; curTail += 0x10, writePtr += 0x10) {
                if (writePtr != nullptr) {
                    uint64_t q0 = *reinterpret_cast<const uint64_t*>(curTail);
                    uint64_t q1 = *reinterpret_cast<const uint64_t*>(curTail + 8);
                    *reinterpret_cast<uint64_t*>(writePtr + 8) = q1;
                    *reinterpret_cast<uint64_t*>(writePtr)     = q0;
                }
            }
            dPos = *reinterpret_cast<uint8_t**>(dst + 0x10);
        }

        uint8_t* oldBuf = *reinterpret_cast<uint8_t**>(dst + 0x08);
        int64_t newCount = ((dPos - oldBuf) >> 4) + count;
        if (oldBuf != nullptr) {
            void* a2 = *reinterpret_cast<void**>(dst + 0x20);
            auto* vt2 = *reinterpret_cast<uintptr_t**>(a2);
            reinterpret_cast<void (*)(void*, void*)>(vt2[0x70 / 8])(a2, oldBuf);
        }
        *reinterpret_cast<uint8_t**>(dst + 0x18) = newBuf + newBytes;
        *reinterpret_cast<uint8_t**>(dst + 0x10) = newBuf + (static_cast<uint64_t>(newCount) << 4);
        *reinterpret_cast<uint8_t**>(dst + 0x08) = newBuf;
        return;
    }

    uint8_t* dEnd = dPos;
    if (sBegin != sEnd) {
        uint8_t* outPtr = dPos;
        for (const uint8_t* inPtr = sBegin; inPtr != sEnd; inPtr += 0x10, outPtr += 0x10) {
            if (outPtr != nullptr) {
                uint64_t q0 = *reinterpret_cast<const uint64_t*>(inPtr);
                uint64_t q1 = *reinterpret_cast<const uint64_t*>(inPtr + 8);
                *reinterpret_cast<uint64_t*>(outPtr + 8) = q1;
                *reinterpret_cast<uint64_t*>(outPtr)     = q0;
            }
        }
        dEnd = *reinterpret_cast<uint8_t**>(dst + 0x10);
    }

    uint64_t insertBytes = static_cast<uint64_t>(count) << 4;
    if (dPos != dEnd) {
        int64_t midDist = (dEnd - dPos) >> 4;
        if (midDist != 0) {
            uint8_t* totalEnd = dEnd + insertBytes;
            int64_t totalDist = (totalEnd - dPos) >> 4;
            int64_t a = totalDist;
            int64_t b = midDist;
            int64_t gcd = b;
            do {
                gcd = b;
                b = a % gcd;
                a = gcd;
            } while (b != 0);

            if (gcd < totalDist && gcd > 0) {
                uint64_t midBytes = static_cast<uint64_t>(midDist) << 4;
                for (int64_t cycle = gcd; cycle > 0; ) {
                    --cycle;
                    uint8_t* startPtr = dPos + (static_cast<uint64_t>(cycle) << 4);
                    uint8_t* nextPtr  = dPos + (static_cast<uint64_t>(cycle + midDist) << 4);
                    if (nextPtr == totalEnd) {
                        nextPtr = dPos;
                    }
                    uint8_t* curPtr = startPtr;
                    do {
                        uint64_t tmp0 = *reinterpret_cast<uint64_t*>(curPtr);
                        uint64_t tmp1 = *reinterpret_cast<uint64_t*>(curPtr + 8);
                        *reinterpret_cast<uint64_t*>(curPtr + 8) = *reinterpret_cast<uint64_t*>(nextPtr + 8);
                        *reinterpret_cast<uint64_t*>(curPtr)     = *reinterpret_cast<uint64_t*>(nextPtr);
                        *reinterpret_cast<uint64_t*>(nextPtr + 8) = tmp1;
                        *reinterpret_cast<uint64_t*>(nextPtr)     = tmp0;

                        int64_t remToEnd = (totalEnd - nextPtr) >> 4;
                        int64_t diff = midDist - remToEnd;
                        uint8_t* stepPtr = (diff < 0)
                            ? (nextPtr + midBytes)
                            : (dPos + (static_cast<uint64_t>(diff) << 4));
                        curPtr  = nextPtr;
                        nextPtr = stepPtr;
                    } while (nextPtr != startPtr);
                }
                dEnd = *reinterpret_cast<uint8_t**>(dst + 0x10);
            }
        }
    } else {
        dEnd = dPos;
    }

    *reinterpret_cast<uint8_t**>(dst + 0x10) = dEnd + insertBytes;
}

// ============================================================================
// 35. 0x21ccbc0 (1,385 B) - GXSkinningEntryVector_CopyAssignWithRotate
// ============================================================================
void sub_21ccbc0(void* dstVec, const void* srcVec) {
    auto* dst = static_cast<uint8_t*>(dstVec);
    auto* src = static_cast<const uint8_t*>(srcVec);
    if (dst == src) {
        return;
    }

    uint8_t* oldBegin = *reinterpret_cast<uint8_t**>(dst + 0x08);
    if (oldBegin != nullptr) {
        void* dstAlloc = *reinterpret_cast<void**>(dst + 0x20);
        auto* vtbl     = *reinterpret_cast<uintptr_t**>(dstAlloc);
        reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(dstAlloc, oldBegin);
        *reinterpret_cast<uint64_t*>(dst + 0x18) = 0ULL;
        *reinterpret_cast<uint64_t*>(dst + 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(dst + 0x08) = 0ULL;
    }

    void* dstAlloc2 = *reinterpret_cast<void**>(dst + 0x20);
    void* srcAlloc2 = *reinterpret_cast<void* const*>(src + 0x20);
    auto* vtblD2    = *reinterpret_cast<uintptr_t**>(dstAlloc2);
    uint32_t idD    = reinterpret_cast<uint32_t (*)(void*)>(vtblD2[0x10 / 8])(dstAlloc2);
    auto* vtblS2    = *reinterpret_cast<uintptr_t**>(srcAlloc2);
    uint32_t idS    = reinterpret_cast<uint32_t (*)(void*)>(vtblS2[0x10 / 8])(srcAlloc2);

    if (idD == idS) {
        auto* mutSrc = const_cast<uint8_t*>(src);
        __m128 ptrs01 = _mm_loadu_ps(reinterpret_cast<const float*>(mutSrc + 0x08));
        _mm_storeu_ps(reinterpret_cast<float*>(dst + 0x08), ptrs01);
        *reinterpret_cast<uint64_t*>(dst + 0x18)    = *reinterpret_cast<uint64_t*>(mutSrc + 0x18);
        *reinterpret_cast<uint64_t*>(mutSrc + 0x18) = 0ULL;
        *reinterpret_cast<uint64_t*>(mutSrc + 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(mutSrc + 0x08) = 0ULL;
        return;
    }

    const uint8_t* sBegin = *reinterpret_cast<const uint8_t* const*>(src + 0x08);
    const uint8_t* sEnd   = *reinterpret_cast<const uint8_t* const*>(src + 0x10);
    uint8_t* dPos         = *reinterpret_cast<uint8_t**>(dst + 0x08);
    *reinterpret_cast<uint8_t**>(dst + 0x10) = dPos;

    ptrdiff_t byteDiff = sEnd - sBegin;
    if (byteDiff == 0) {
        return;
    }

    int64_t count = static_cast<int64_t>(
        static_cast<uint64_t>(byteDiff >> 3) * 0xCCCCCCCCCCCCCCCDULL);
    int64_t availCap = static_cast<int64_t>(
        static_cast<uint64_t>((*reinterpret_cast<uint8_t**>(dst + 0x18) - dPos) >> 3) *
        0xCCCCCCCCCCCCCCCDULL);

    if (static_cast<uint64_t>(availCap) < static_cast<uint64_t>(count)) {
        if (static_cast<uint64_t>(count) >= 0x0666666666666667ULL) {
            reinterpret_cast<FnThrowLengthError>(GX_VMA(0x2bbf1b8ULL))(
                reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
        }
        uint64_t half = static_cast<uint64_t>(availCap) >> 1;
        uint64_t grown = (0x0666666666666666ULL - half >= static_cast<uint64_t>(availCap))
            ? (static_cast<uint64_t>(availCap) + half)
            : 0ULL;
        if (grown < static_cast<uint64_t>(count)) {
            grown = static_cast<uint64_t>(count);
        }

        void* alloc = *reinterpret_cast<void**>(dst + 0x20);
        auto* allocVtbl = *reinterpret_cast<uintptr_t**>(alloc);
        auto* newBuf = static_cast<uint8_t*>(
            reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(allocVtbl[0x58 / 8])(
                alloc, grown * 40ULL, 8u));
        if (newBuf == nullptr) {
            auto* pIgnore = reinterpret_cast<int32_t*>(ReadTlsQwordAtOffset(0x53e4318ULL));
            if (*pIgnore != 0) {
                reinterpret_cast<FnSetErrnoError>(GX_VMA(0x2bbe978ULL))(
                    reinterpret_cast<void*>(ReadTlsQwordAtOffset(0x53e4320ULL)), 1u);
            } else {
                reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                    nullptr, 0x109u, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
            }
        }

        uint8_t* dBegin = *reinterpret_cast<uint8_t**>(dst + 0x08);
        uint8_t* writePtr = newBuf;
        if (dBegin != dPos) {
            uint64_t prefixBytes = static_cast<uint64_t>(dPos - dBegin);
            for (uint64_t off = 0; off != prefixBytes; off += 0x28ULL) {
                if (reinterpret_cast<uintptr_t>(newBuf) + off != 0) {
                    std::memcpy(newBuf + off, dBegin + off, 0x28);
                }
            }
            uint64_t prefixCountM1 = static_cast<uint64_t>(dPos - 0x28 - dBegin) / 0x28ULL;
            writePtr = newBuf + (prefixCountM1 + 1ULL) * 0x28ULL;
        }

        uint8_t* curOut = writePtr;
        for (const uint8_t* curIn = sBegin; curIn != sEnd; curIn += 0x28, curOut += 0x28) {
            if (curOut != nullptr) {
                std::memcpy(curOut, curIn, 0x28);
            }
        }

        uint8_t* dEnd = *reinterpret_cast<uint8_t**>(dst + 0x10);
        if (dPos != dEnd) {
            uint64_t copiedCountM1 = static_cast<uint64_t>(sEnd - 0x28 - sBegin) / 0x28ULL;
            uint8_t* tailOut = writePtr + (copiedCountM1 + 1ULL) * 0x28ULL;
            for (const uint8_t* curTail = dPos; curTail != dEnd; curTail += 0x28, tailOut += 0x28) {
                if (tailOut != nullptr) {
                    std::memcpy(tailOut, curTail, 0x28);
                }
            }
            dPos = *reinterpret_cast<uint8_t**>(dst + 0x10);
        }

        uint8_t* oldBuf = *reinterpret_cast<uint8_t**>(dst + 0x08);
        int64_t oldTailCount = static_cast<int64_t>(
            static_cast<uint64_t>((dPos - oldBuf) >> 3) * 0xCCCCCCCCCCCCCCCDULL);
        int64_t newCount = oldTailCount + count;
        if (oldBuf != nullptr) {
            void* a2 = *reinterpret_cast<void**>(dst + 0x20);
            auto* vt2 = *reinterpret_cast<uintptr_t**>(a2);
            reinterpret_cast<void (*)(void*, void*)>(vt2[0x70 / 8])(a2, oldBuf);
        }
        *reinterpret_cast<uint8_t**>(dst + 0x18) = newBuf + grown * 40ULL;
        *reinterpret_cast<uint8_t**>(dst + 0x10) = newBuf + static_cast<uint64_t>(newCount) * 40ULL;
        *reinterpret_cast<uint8_t**>(dst + 0x08) = newBuf;
        return;
    }

    for (uint64_t off = 0; off != static_cast<uint64_t>(byteDiff); off += 0x28ULL) {
        if (reinterpret_cast<uintptr_t>(dPos) + off != 0) {
            std::memcpy(dPos + off, sBegin + off, 0x28);
        }
    }

    uint8_t* dEnd = *reinterpret_cast<uint8_t**>(dst + 0x10);
    uint64_t insertBytes = static_cast<uint64_t>(count) * 40ULL;
    if (dPos != dEnd) {
        uint8_t* totalEnd = dEnd + insertBytes;
        int64_t midDist = static_cast<int64_t>(
            static_cast<uint64_t>((dEnd - dPos) >> 3) * 0xCCCCCCCCCCCCCCCDULL);
        int64_t totalDist = static_cast<int64_t>(
            static_cast<uint64_t>((totalEnd - dPos) >> 3) * 0xCCCCCCCCCCCCCCCDULL);

        int64_t a = totalDist;
        int64_t b = midDist;
        int64_t gcd = b;
        do {
            gcd = b;
            b = a % gcd;
            a = gcd;
        } while (b != 0);

        if (gcd < totalDist && gcd > 0) {
            uint64_t midBytes = static_cast<uint64_t>(midDist) * 40ULL;
            for (int64_t cycle = gcd; cycle > 0; ) {
                --cycle;
                uint8_t* startPtr = dPos + static_cast<uint64_t>(cycle) * 40ULL;
                uint8_t* nextPtr  = dPos + static_cast<uint64_t>(cycle + midDist) * 40ULL;
                if (nextPtr == totalEnd) {
                    nextPtr = dPos;
                }
                uint8_t* curPtr = startPtr;
                do {
                    uint64_t tmp[5];
                    std::memcpy(tmp, curPtr, 40);
                    std::memcpy(curPtr, nextPtr, 40);
                    std::memcpy(nextPtr, tmp, 40);

                    int64_t remToEnd = static_cast<int64_t>(
                        static_cast<uint64_t>((totalEnd - nextPtr) >> 3) * 0xCCCCCCCCCCCCCCCDULL);
                    int64_t diff = midDist - remToEnd;
                    uint8_t* stepPtr = (diff < 0)
                        ? (nextPtr + midBytes)
                        : (dPos + static_cast<uint64_t>(diff) * 40ULL);
                    curPtr  = nextPtr;
                    nextPtr = stepPtr;
                } while (nextPtr != startPtr);
            }
            dEnd = *reinterpret_cast<uint8_t**>(dst + 0x10);
        }
    } else {
        dEnd = dPos;
    }

    *reinterpret_cast<uint8_t**>(dst + 0x10) = dEnd + insertBytes;
}

// ============================================================================
// 36. 0x21cca50 (363 B) - GXSkinningEntryVector_GrowByOne
// ============================================================================
void sub_21cca50(void* vec) {
    auto* v      = static_cast<uint8_t*>(vec);
    uint8_t* end = *reinterpret_cast<uint8_t**>(v + 0x10);
    uint8_t* cap = *reinterpret_cast<uint8_t**>(v + 0x18);
    if (cap != end) {
        return;
    }

    uint8_t* begin = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint64_t curCount = static_cast<uint64_t>((cap - begin) >> 3) * 0xCCCCCCCCCCCCCCCDULL;
    uint64_t half     = curCount >> 1;
    uint64_t minCap   = curCount + 1ULL;
    uint64_t newCap   = (0x0666666666666666ULL - half >= curCount) ? (curCount + half) : 0ULL;
    if (newCap < minCap) {
        newCap = minCap;
    }

    void* alloc = *reinterpret_cast<void**>(v + 0x20);
    auto* allocVtbl = *reinterpret_cast<uintptr_t**>(alloc);
    auto* newBuf = static_cast<uint8_t*>(
        reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(allocVtbl[0x58 / 8])(
            alloc, newCap * 40ULL, 8u));
    if (newBuf == nullptr) {
        auto* pIgnore = reinterpret_cast<int32_t*>(ReadTlsQwordAtOffset(0x53e4318ULL));
        if (*pIgnore != 0) {
            reinterpret_cast<FnSetErrnoError>(GX_VMA(0x2bbe978ULL))(
                reinterpret_cast<void*>(ReadTlsQwordAtOffset(0x53e4320ULL)), 1u);
        } else {
            reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                nullptr, 0x109u, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
        }
    }

    uint8_t* curBegin = *reinterpret_cast<uint8_t**>(v + 0x08);
    uint8_t* curEnd   = *reinterpret_cast<uint8_t**>(v + 0x10);
    if (curBegin != curEnd) {
        uint8_t* dst = newBuf;
        for (uint8_t* src = curBegin; src != curEnd; src += 0x28, dst += 0x28) {
            if (dst != nullptr) {
                std::memcpy(dst, src, 0x28);
            }
        }
        curBegin = *reinterpret_cast<uint8_t**>(v + 0x08);
        curEnd   = *reinterpret_cast<uint8_t**>(v + 0x10);
    }

    uint64_t copiedCount = static_cast<uint64_t>((curEnd - curBegin) >> 3) * 0xCCCCCCCCCCCCCCCDULL;
    if (curBegin != nullptr) {
        void* a2 = *reinterpret_cast<void**>(v + 0x20);
        auto* vt2 = *reinterpret_cast<uintptr_t**>(a2);
        reinterpret_cast<void (*)(void*, void*)>(vt2[0x70 / 8])(a2, curBegin);
    }

    *reinterpret_cast<uint8_t**>(v + 0x18) = newBuf + newCap * 40ULL;
    *reinterpret_cast<uint8_t**>(v + 0x10) = newBuf + copiedCount * 40ULL;
    *reinterpret_cast<uint8_t**>(v + 0x08) = newBuf;
}

// ============================================================================
// 37. 0x21cca40 (5 B) - GXSkinningTaskJobB_DeletingDestructor
// ============================================================================
void sub_21cca40(void* self) {
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// 38. 0x21cca30 (1 B) - GXSkinningTaskJobB_Destroy
// ============================================================================
void sub_21cca30(void* /*self*/) {
}

// ============================================================================
// 39. 0x21cca20 (5 B) - GXSkinningTaskJobA_DeletingDestructor
// ============================================================================
void sub_21cca20(void* self) {
    reinterpret_cast<FnOperatorDelete>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// 40. 0x21cca10 (1 B) - GXSkinningTaskJobA_Destroy
// ============================================================================
void sub_21cca10(void* /*self*/) {
}

// ============================================================================
// 41. 0x21cc6c0 (700 B) - GXSkinningMatricesUpdater_RegisterDebugGuiProperties
// ============================================================================
void sub_21cc6c0(void* ownerPtr, void* debugGuiCtx) {
    auto* updater = *reinterpret_cast<uint8_t**>(ownerPtr);
    auto* gui     = static_cast<uint8_t*>(debugGuiCtx);

    struct alignas(16) WideLabelDesc {
        uint64_t zero0;
        uintptr_t wstrAddr;
    };

    WideLabelDesc groupDesc = { 0ULL, GX_VMA(0x49c3a10ULL) }; // L"MatricesUpdater"
    void* groupHandle = reinterpret_cast<FnDebugGuiBeginGroup>(GX_VMA(0x236c900ULL))(gui, &groupDesc, 0u);
    groupDesc.wstrAddr = 0ULL;
    if (groupHandle == nullptr) {
        return;
    }

    auto registerUintProp = [&](uintptr_t labelVma, uint32_t offset) {
        WideLabelDesc propDesc = { 0ULL, labelVma };
        reinterpret_cast<FnDebugGuiAddUint32Prop>(GX_VMA(0x2905e60ULL))(gui, &propDesc, updater + offset);
        propDesc.wstrAddr = 0ULL;
    };

    registerUintProp(GX_VMA(0x49c3a30ULL), 0x5f8u); // L"Block Count"
    registerUintProp(GX_VMA(0x49c3a48ULL), 0x5fcu); // L"Buffer  Free"
    registerUintProp(GX_VMA(0x49c3a7cULL), 0x600u); // L"         Max"
    registerUintProp(GX_VMA(0x49c3a96ULL), 0x604u); // L"       Total"
    registerUintProp(GX_VMA(0x49c3a62ULL), 0x608u); // L"Seg     Free"
    registerUintProp(GX_VMA(0x49c3a7cULL), 0x60cu); // L"         Max"
    registerUintProp(GX_VMA(0x49c3a96ULL), 0x610u); // L"       Total"

    WideLabelDesc warnDesc = { 0ULL, GX_VMA(0x49c3ab0ULL) }; // L"Warning Rate"
    reinterpret_cast<FnDebugGuiAddFloatProp>(GX_VMA(0x292a590ULL))(
        gui, &warnDesc, reinterpret_cast<void*>(GX_VMA(0x513ac94ULL)),
        0.0f, kOne_4929754, kPointZeroOne_4929758, kOneHundred_492975c);
    warnDesc.wstrAddr = 0ULL;

    auto** stackBegin = *reinterpret_cast<uint8_t***>(gui + 0x08);
    auto** stackEnd   = *reinterpret_cast<uint8_t***>(gui + 0x10);
    if (stackBegin != stackEnd) {
        uint8_t* node = stackEnd[-1];
        if (node != nullptr) {
            uint32_t refWord = *reinterpret_cast<uint32_t*>(node + 0x08);
            uint32_t decStrong = refWord + 0x7FFFu;
            uint32_t newStrong = decStrong & 0x7FFFu;
            uint32_t updated = (refWord & 0xFFFF8000u) | newStrong;
            *reinterpret_cast<uint32_t*>(node + 0x08) = updated;
            if (newStrong == 0 && (refWord & 0x40000000u) != 0) {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(node);
                reinterpret_cast<void (*)(void*)>(vtbl[0x18 / 8])(node);
                updated = *reinterpret_cast<uint32_t*>(node + 0x08) & 0xBFFFFFFFu;
                *reinterpret_cast<uint32_t*>(node + 0x08) = updated;
            }
            uint32_t decWeak = (updated >> 15) + 0x7FFFu;
            uint32_t newWeak = decWeak & 0x7FFFu;
            *reinterpret_cast<uint32_t*>(node + 0x08) = (updated & 0xC0007FFFu) | (newWeak << 15);
            if (newWeak == 0) {
                *reinterpret_cast<uint32_t*>(node + 0x08) = updated | 0x3FFF8000u;
                auto* vtbl = *reinterpret_cast<uintptr_t**>(node);
                reinterpret_cast<void (*)(void*)>(vtbl[0x20 / 8])(node);
            }
            stackEnd[-1] = nullptr;
            stackEnd = *reinterpret_cast<uint8_t***>(gui + 0x10);
        }
        *reinterpret_cast<uint8_t***>(gui + 0x10) = stackEnd - 1;
    }
}

// ============================================================================
// 42. 0x21cc5e0 (222 B) - GXSkinningMatricesUpdater_UnregisterSkinningTarget
// ============================================================================
namespace {
static volatile uint32_t g_skinningEntriesSpinLock = 0u;
struct SkinningEntriesLockGuard {
    SkinningEntriesLockGuard() noexcept {
        while (__atomic_exchange_n(&g_skinningEntriesSpinLock, 1u, __ATOMIC_ACQUIRE) != 0u) {
            __builtin_ia32_pause();
        }
    }
    ~SkinningEntriesLockGuard() noexcept {
        __atomic_store_n(&g_skinningEntriesSpinLock, 0u, __ATOMIC_RELEASE);
    }
};
} // namespace

void sub_21cc5e0(void* ownerPtr, void* target) {
    SkinningEntriesLockGuard lockGuard{};
    auto* updater = *reinterpret_cast<uint8_t**>(ownerPtr);
    uint8_t* cur  = *reinterpret_cast<uint8_t**>(updater + 0x10);
    uint8_t* end  = *reinterpret_cast<uint8_t**>(updater + 0x18);

    while (end != cur) {
        bool match = (*reinterpret_cast<void**>(cur) == target);
        cur += 0x28;
        if (match) {
            void* biHeap = updater + 0x508;
            auto freeBlock = [&](void* blk) {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(biHeap);
                reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(biHeap, blk);
            };
            freeBlock(*reinterpret_cast<void**>(cur - 0x08));
            freeBlock(*reinterpret_cast<void**>(cur - 0x10));
            freeBlock(*reinterpret_cast<void**>(cur - 0x18));
            freeBlock(*reinterpret_cast<void**>(cur - 0x20));

            uint8_t* newEnd = *reinterpret_cast<uint8_t**>(updater + 0x18);
            if (newEnd != cur) {
                uint8_t* dst = cur - 0x28;
                while (dst + 0x50 != newEnd + 0x28) {
                    std::memcpy(dst, dst + 0x28, 0x28);
                    dst += 0x28;
                }
                newEnd = *reinterpret_cast<uint8_t**>(updater + 0x18);
            }
            *reinterpret_cast<uint8_t**>(updater + 0x18) = newEnd - 0x28;
            return;
        }
    }
}

// ============================================================================
// 43. 0x21cba80 (2,912 B) - GXSkinningMatricesUpdater_AllocateAndQueueBoneMatrices
// ============================================================================
bool sub_21cba80(void* ownerPtr, void* target, const void* srcMatrices0, const void* srcMatrices1) {
    if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac8cULL)) == 0u) {
        return false;
    }

    auto* tgt = static_cast<uint8_t*>(target);
    uint32_t boneCount = *reinterpret_cast<const uint32_t*>(tgt + 0xa0);
    if (boneCount == 0u) {
        return true;
    }

    SkinningEntriesLockGuard lockGuard{};
    auto* updater = *reinterpret_cast<uint8_t**>(ownerPtr);
    uint8_t* entryVec = updater + 0x08;
    uint8_t* cur      = *reinterpret_cast<uint8_t**>(updater + 0x10);
    uint8_t* end      = *reinterpret_cast<uint8_t**>(updater + 0x18);

    auto allocateFromBiHeapAndUpdateStats = [&](uint64_t byteSize) -> void* {
        void* biHeap = updater + 0x508;
        auto* biVtbl = *reinterpret_cast<uintptr_t**>(biHeap);
        void* mem = reinterpret_cast<void* (*)(void*, uint64_t)>(biVtbl[0x50 / 8])(biHeap, byteSize);
        ++(*reinterpret_cast<uint32_t*>(updater + 0x614));
        return mem;
    };

    uint64_t matrixByteSize = static_cast<uint64_t>(boneCount) * 48ULL;

    while (end != cur) {
        bool match = (*reinterpret_cast<void**>(cur) == target);
        cur += 0x28;
        if (match) {
            uint8_t* foundEntry = cur; // points 0x28 past the matched entry
            if (*reinterpret_cast<const uint32_t*>(tgt + 0x9c) == 2u) {
                void* prevBuf0 = *reinterpret_cast<void**>(foundEntry - 0x20);
                void* reuseBuf0 = *reinterpret_cast<void**>(foundEntry - 0x18);
                *reinterpret_cast<void**>(foundEntry - 0x18) = prevBuf0;
                if (reuseBuf0 == nullptr) {
                    reuseBuf0 = allocateFromBiHeapAndUpdateStats(matrixByteSize);
                }
                *reinterpret_cast<void**>(foundEntry - 0x20) = reuseBuf0;

                if (srcMatrices1 != nullptr) {
                    void* prevBuf1 = *reinterpret_cast<void**>(foundEntry - 0x10);
                    void* reuseBuf1 = *reinterpret_cast<void**>(foundEntry - 0x08);
                    *reinterpret_cast<void**>(foundEntry - 0x08) = prevBuf1;
                    if (reuseBuf1 == nullptr) {
                        reuseBuf1 = allocateFromBiHeapAndUpdateStats(matrixByteSize);
                    }
                    *reinterpret_cast<void**>(foundEntry - 0x10) = reuseBuf1;
                }
            } else {
                if (srcMatrices1 != nullptr && *reinterpret_cast<void**>(foundEntry - 0x10) == nullptr) {
                    void* newBuf1 = allocateFromBiHeapAndUpdateStats(matrixByteSize);
                    *reinterpret_cast<void**>(foundEntry - 0x10) = newBuf1;
                }
            }

            reinterpret_cast<FnMemcpyBoneMatrices>(GX_VMA(0x261a1b0ULL))(
                *reinterpret_cast<void**>(foundEntry - 0x20), srcMatrices0, matrixByteSize);
            if (srcMatrices1 != nullptr) {
                reinterpret_cast<FnMemcpyBoneMatrices>(GX_VMA(0x261a1b0ULL))(
                    *reinterpret_cast<void**>(foundEntry - 0x10), srcMatrices1, matrixByteSize);
            }
            return true;
        }
    }

    // Target not found in m_entries: allocate buffers and push new 40-byte entry
    void* newDst0 = allocateFromBiHeapAndUpdateStats(matrixByteSize);
    reinterpret_cast<FnMemcpyBoneMatrices>(GX_VMA(0x261a1b0ULL))(newDst0, srcMatrices0, matrixByteSize);

    void* newDst1 = nullptr;
    if (srcMatrices1 != nullptr) {
        newDst1 = allocateFromBiHeapAndUpdateStats(matrixByteSize);
        reinterpret_cast<FnMemcpyBoneMatrices>(GX_VMA(0x261a1b0ULL))(newDst1, srcMatrices1, matrixByteSize);
    }

    struct SkinningEntry40 {
        void* target;
        void* dst0;
        void* dst0Prev;
        void* dst1;
        void* dst1Prev;
    };
    SkinningEntry40 localEntry = { target, newDst0, nullptr, newDst1, nullptr };

    uint8_t* vEnd = *reinterpret_cast<uint8_t**>(updater + 0x18);
    uint8_t* addrLocal = reinterpret_cast<uint8_t*>(&localEntry);
    if (vEnd > addrLocal && *reinterpret_cast<uint8_t**>(updater + 0x10) <= addrLocal) {
        uint8_t* vBegin = *reinterpret_cast<uint8_t**>(updater + 0x10);
        uint64_t relIdx = static_cast<uint64_t>((addrLocal - vBegin) >> 3) * 0xCCCCCCCCCCCCCCCDULL;
        if (vEnd == *reinterpret_cast<uint8_t**>(updater + 0x20)) {
            sub_21cca50(entryVec);
            vEnd = *reinterpret_cast<uint8_t**>(updater + 0x18);
        }
        if (vEnd != nullptr) {
            uint8_t* srcElem = *reinterpret_cast<uint8_t**>(updater + 0x10) + relIdx * 40ULL;
            std::memcpy(vEnd, srcElem, 0x28);
        }
    } else {
        if (vEnd == *reinterpret_cast<uint8_t**>(updater + 0x20)) {
            sub_21cca50(entryVec);
            vEnd = *reinterpret_cast<uint8_t**>(updater + 0x18);
        }
        if (vEnd != nullptr) {
            std::memcpy(vEnd, &localEntry, 0x28);
        }
    }
    *reinterpret_cast<uint8_t**>(updater + 0x18) += 0x28;
    return true;
}

// ============================================================================
// 44. 0x21cba30 (69 B) - GXSkinningMatricesUpdaterOwner_Destroy
// ============================================================================
void sub_21cba30(void* ownerPtr) {
    auto** pUpdater = static_cast<void**>(ownerPtr);
    void* updater = *pUpdater;
    if (updater != nullptr) {
        void* heapAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        sub_21cad60(updater);
        auto* vtbl = *reinterpret_cast<uintptr_t**>(heapAlloc);
        reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(heapAlloc, updater);
        *pUpdater = nullptr;
    }
}

// ============================================================================
// 45. 0x21cb9d0 (86 B) - GXSkinningMatricesUpdaterOwner_Create
// ============================================================================
void sub_21cb9d0(void* ownerPtr, uint32_t segHeapBlocks, uint32_t biHeapBytes) {
    auto** pUpdater = static_cast<void**>(ownerPtr);
    void* heapAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    auto* vtbl      = *reinterpret_cast<uintptr_t**>(heapAlloc);
    void* mem       = reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(vtbl[0x58 / 8])(
        heapAlloc, 0x618ULL, 8u);
    if (mem != nullptr) {
        sub_21ca3d0(mem, segHeapBlocks, biHeapBytes);
    }
    *pUpdater = mem;
}

// ============================================================================
// 46. 0x21cb970 (91 B) - GXSkinningMatricesUpdater_SubmitClearJob
// ============================================================================
void sub_21cb970(void* updaterPtr, void* jobScheduler) {
    auto* updater = static_cast<uint8_t*>(updaterPtr);
    auto* sched   = static_cast<uint8_t*>(jobScheduler);
    void* clearJob = *reinterpret_cast<void**>(updater + 0x60);

    void* rawTp = *reinterpret_cast<void**>(GX_VMA(0x56a6c28ULL));
    void* taskPool = (g_eboot_live_slide != 0 && rawTp != nullptr) ? *reinterpret_cast<void**>(rawTp) : rawTp;
    auto* taskNode = static_cast<uint8_t*>(
        reinterpret_cast<FnAllocTaskNode>(GX_VMA(0x21b7f60ULL))(taskPool));
    *reinterpret_cast<void**>(taskNode + 0x08)    = clearJob;
    *reinterpret_cast<uint32_t*>(taskNode + 0x10) = 0u;
    *reinterpret_cast<uint64_t*>(taskNode + 0x14) = 0x0000000100000001ULL;

    uint8_t* queueBase = *reinterpret_cast<uint8_t**>(sched + 0x00) + 0x18;
    reinterpret_cast<FnSubmitTaskNode>(GX_VMA(0xe47ab0ULL))(queueBase, taskNode);

    if (sched[0x68] != 0u) {
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1u;
    }
    sched[0x31] = 1u;
}

// ============================================================================
// 47. 0x21cb5d0 (925 B) - GXSkinningMatricesUpdater_DispatchFrameUpdateJobs
// ============================================================================
void sub_21cb5d0(void* updaterPtr, void* jobScheduler) {
    auto* updater = static_cast<uint8_t*>(updaterPtr);
    auto* sched   = static_cast<uint8_t*>(jobScheduler);

    void* biMutex = updater + 0x5d8;
    auto* bmVtbl1 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(bmVtbl1[0x18 / 8])(biMutex, 0xFFFFFFFFu);
    uint32_t biFree = *reinterpret_cast<const uint32_t*>(updater + 0x5d0);
    auto* bmVtbl2 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*)>(bmVtbl2[0x28 / 8])(biMutex);
    *reinterpret_cast<uint32_t*>(updater + 0x5fc) = biFree;

    auto* bmVtbl3 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(bmVtbl3[0x18 / 8])(biMutex, 0xFFFFFFFFu);
    uint8_t* biSentinel = updater + 0x580;
    uint8_t* biNode     = *reinterpret_cast<uint8_t**>(updater + 0x5a0);
    uint64_t maxBiBlock = 0ULL;
    while (biNode != biSentinel) {
        uint8_t* nextGeo = *reinterpret_cast<uint8_t**>(biNode + 0x10);
        if (nextGeo == nullptr) {
            reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                reinterpret_cast<const char*>(GX_VMA(0x493c8e8ULL)), 0x310u,
                reinterpret_cast<const char*>(GX_VMA(0x493cb05ULL)));
        }
        uint64_t span = static_cast<uint64_t>(
            *reinterpret_cast<uint64_t*>(nextGeo) - *reinterpret_cast<uint64_t*>(biNode));
        if (maxBiBlock < span) {
            maxBiBlock = span;
        }
        biNode = *reinterpret_cast<uint8_t**>(biNode + 0x20);
    }
    auto* bmVtbl4 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*)>(bmVtbl4[0x28 / 8])(biMutex);
    *reinterpret_cast<uint32_t*>(updater + 0x600) = static_cast<uint32_t>(maxBiBlock);

    void* segMutex = updater + 0x4d0;
    auto* smVtbl1  = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(smVtbl1[0x18 / 8])(segMutex, 0xFFFFFFFFu);
    uint32_t segFree = *reinterpret_cast<const uint32_t*>(updater + 0xa8);
    auto* smVtbl2  = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*)>(smVtbl2[0x28 / 8])(segMutex);
    *reinterpret_cast<uint32_t*>(updater + 0x608) = segFree;

    auto* smVtbl3  = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(smVtbl3[0x18 / 8])(segMutex, 0xFFFFFFFFu);
    uint64_t bitmask = *reinterpret_cast<const uint64_t*>(updater + 0x90);
    uint64_t maxSegBlock = 0ULL;
    if (bitmask != 0ULL) {
        uint64_t binIdx = 63ULL - static_cast<uint64_t>(__builtin_clzll(bitmask));
        uint8_t* binHead = updater + (binIdx << 4) + 0xa8;
        uint8_t* segNode = *reinterpret_cast<uint8_t**>(updater + (binIdx << 4) + 0xc0);
        while (segNode != binHead) {
            uint64_t rawNext = *reinterpret_cast<const uint64_t*>(segNode + 0x08) & ~3ULL;
            uint64_t segSize = (rawNext - reinterpret_cast<uintptr_t>(segNode)) - 8ULL;
            if (maxSegBlock < segSize) {
                maxSegBlock = segSize;
            }
            segNode = *reinterpret_cast<uint8_t**>(segNode + 0x18);
        }
    }
    auto* smVtbl4 = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*)>(smVtbl4[0x28 / 8])(segMutex);
    *reinterpret_cast<uint32_t*>(updater + 0x60c) = static_cast<uint32_t>(maxSegBlock);

    uint32_t frameAllocs = *reinterpret_cast<const uint32_t*>(updater + 0x614);
    *reinterpret_cast<uint32_t*>(updater + 0x5f8) = frameAllocs;
    *reinterpret_cast<uint32_t*>(updater + 0x614) = 0u;

    int32_t workerCount = *reinterpret_cast<const int32_t*>(sched + 0x08);
    uint64_t entryQwords = static_cast<uint64_t>(
        *reinterpret_cast<uint8_t**>(updater + 0x18) -
        *reinterpret_cast<uint8_t**>(updater + 0x10)) >> 3;
    int32_t totalEntries = static_cast<int32_t>(static_cast<uint32_t>(entryQwords) * 0xCCCCCCCDu);
    int32_t divisor      = workerCount * 2;
    int32_t chunkSize    = totalEntries / divisor;

    bool canParallelize = false;
    if (chunkSize >= 2) {
        uint8_t* globalDev = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        uint8_t* ctxSlot   = (globalDev[0x84] != 0u) ? (globalDev + 0x78) : (globalDev + 0x10);
        uint8_t* cmdCtx    = *reinterpret_cast<uint8_t**>(ctxSlot);
        if (cmdCtx[0x2093] == 0u && *reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac8dULL)) != 0u) {
            canParallelize = true;
        }
    }

    if (!canParallelize) {
        if (totalEntries <= 0) {
            return;
        }
        void* updateJob  = *reinterpret_cast<void**>(updater + 0x58);
        void* rawTp      = *reinterpret_cast<void**>(GX_VMA(0x56a6c28ULL));
        void* taskPool   = (g_eboot_live_slide != 0 && rawTp != nullptr) ? *reinterpret_cast<void**>(rawTp) : rawTp;
        auto* taskNode   = static_cast<uint8_t*>(
            reinterpret_cast<FnAllocTaskNode>(GX_VMA(0x21b7f60ULL))(taskPool));
        *reinterpret_cast<void**>(taskNode + 0x08)    = updateJob;
        *reinterpret_cast<int32_t*>(taskNode + 0x10)  = 0;
        *reinterpret_cast<int32_t*>(taskNode + 0x14)  = totalEntries;
        *reinterpret_cast<int32_t*>(taskNode + 0x18)  = totalEntries;

        uint8_t* queueBase = *reinterpret_cast<uint8_t**>(sched + 0x00) + 0x18;
        reinterpret_cast<FnSubmitTaskNode>(GX_VMA(0xe47ab0ULL))(queueBase, taskNode);
        if (sched[0x68] != 0u) {
            *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1u;
        }
        sched[0x31] = 1u;
        return;
    }

    int32_t dispatchedCount = 0;
    if (workerCount > 0) {
        uint64_t poolStrideOff  = 0ULL;
        uint64_t queueStrideOff = 0x18ULL;
        int32_t curStart        = 0;
        for (int32_t w = 0; w < workerCount; ++w) {
            int32_t curEnd   = curStart + chunkSize;
            void* updateJob  = *reinterpret_cast<void**>(updater + 0x58);
            uint8_t* rawTp   = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
            uint8_t* baseTp  = (g_eboot_live_slide != 0 && rawTp != nullptr) ? *reinterpret_cast<uint8_t**>(rawTp) : rawTp;
            uint8_t* poolPtr = baseTp + poolStrideOff;
            auto* taskNode   = static_cast<uint8_t*>(
                reinterpret_cast<FnAllocTaskNode>(GX_VMA(0x21b7f60ULL))(poolPtr));
            *reinterpret_cast<void**>(taskNode + 0x08)   = updateJob;
            *reinterpret_cast<int32_t*>(taskNode + 0x10) = curStart;
            *reinterpret_cast<int32_t*>(taskNode + 0x14) = curEnd;
            *reinterpret_cast<int32_t*>(taskNode + 0x18) = chunkSize;

            uint8_t* queuePtr = *reinterpret_cast<uint8_t**>(sched + 0x00) + queueStrideOff;
            reinterpret_cast<FnSubmitTaskNode>(GX_VMA(0xe47ab0ULL))(queuePtr, taskNode);
            if (sched[0x68] != 0u && w == 0) {
                *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1u;
            }
            sched[0x31] = 1u;

            curStart        = curEnd;
            queueStrideOff += 0x530ULL;
            poolStrideOff  += 0x60ULL;
        }
        dispatchedCount = workerCount * chunkSize;
    }

    int32_t maxBatchCap = *reinterpret_cast<const int32_t*>(GX_VMA(0x513ac90ULL));
    int32_t remCount    = totalEntries - dispatchedCount;
    int32_t subChunk    = (remCount > maxBatchCap) ? maxBatchCap : remCount;

    void* updateJob  = *reinterpret_cast<void**>(updater + 0x58);
    void* rawTp      = *reinterpret_cast<void**>(GX_VMA(0x56a6c28ULL));
    void* taskPool   = (g_eboot_live_slide != 0 && rawTp != nullptr) ? *reinterpret_cast<void**>(rawTp) : rawTp;
    auto* taskNode   = static_cast<uint8_t*>(
        reinterpret_cast<FnAllocTaskNode>(GX_VMA(0x21b7f60ULL))(taskPool));
    *reinterpret_cast<void**>(taskNode + 0x08)   = updateJob;
    *reinterpret_cast<int32_t*>(taskNode + 0x10) = dispatchedCount;
    *reinterpret_cast<int32_t*>(taskNode + 0x14) = totalEntries;
    *reinterpret_cast<int32_t*>(taskNode + 0x18) = subChunk;

    uint8_t* queueBase = *reinterpret_cast<uint8_t**>(sched + 0x00) + 0x18;
    reinterpret_cast<FnSubmitTaskNode>(GX_VMA(0xe47ab0ULL))(queueBase, taskNode);
    if (sched[0x68] != 0u) {
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1u;
    }
    sched[0x31] = 1u;
}

// ============================================================================
// 48. 0x21cad60 (2,145 B) - GXSkinningMatricesUpdater_Destroy
// ============================================================================
void sub_21cad60(void* self) {
    auto* u = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(u + 0x00) = GX_VMA(0x535fdc0ULL);

    // 1. Unregister from g_pGXGlobalDeviceSingleton (+0x328..+0x330)
    uint8_t* globalDev = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (globalDev != nullptr) {
        auto** cur = *reinterpret_cast<void***>(globalDev + 0x328);
        auto** end = *reinterpret_cast<void***>(globalDev + 0x330);
        while (end != cur) {
            bool match = (*cur == self);
            ++cur;
            if (match) {
                uint64_t tailBytes = static_cast<uint64_t>(
                    reinterpret_cast<uint8_t*>(end) - reinterpret_cast<uint8_t*>(cur));
                reinterpret_cast<FnMemmove>(GX_VMA(0x2bbe658ULL))(cur - 1, cur, tailBytes);
                *reinterpret_cast<uint8_t**>(globalDev + 0x330) -= 8;
                break;
            }
        }
    }

    // 2. Destroy m_pClearJob (+0x60) and m_pUpdateJob (+0x58)
    void* clearJob = *reinterpret_cast<void**>(u + 0x60);
    if (clearJob != nullptr) {
        void* heapAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        auto* jobVtbl   = *reinterpret_cast<uintptr_t**>(clearJob);
        reinterpret_cast<void (*)(void*)>(jobVtbl[0])(clearJob);
        auto* allocVtbl = *reinterpret_cast<uintptr_t**>(heapAlloc);
        reinterpret_cast<void (*)(void*, void*)>(allocVtbl[0x70 / 8])(heapAlloc, clearJob);
        *reinterpret_cast<void**>(u + 0x60) = nullptr;
    }

    void* updateJob = *reinterpret_cast<void**>(u + 0x58);
    if (updateJob != nullptr) {
        void* heapAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        auto* jobVtbl   = *reinterpret_cast<uintptr_t**>(updateJob);
        reinterpret_cast<void (*)(void*)>(jobVtbl[0])(updateJob);
        auto* allocVtbl = *reinterpret_cast<uintptr_t**>(heapAlloc);
        reinterpret_cast<void (*)(void*, void*)>(allocVtbl[0x70 / 8])(heapAlloc, updateJob);
        *reinterpret_cast<void**>(u + 0x58) = nullptr;
    }

    // 3. Swap-to-empty on m_entries (+0x08..+0x28)
    uint8_t* entryVec = u + 0x08;
    *reinterpret_cast<uint64_t*>(u + 0x18) = *reinterpret_cast<uint64_t*>(u + 0x10);
    void* entryAlloc = *reinterpret_cast<void**>(u + 0x28);
    uint64_t heapFlags = 0;
    auto* eaVtbl = *reinterpret_cast<uintptr_t**>(entryAlloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(eaVtbl[0x20 / 8])(&heapFlags, entryAlloc, 0u);
    if ((heapFlags & 0x20ULL) == 0) {
        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
            nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    struct RawVecHeader {
        uint64_t pad0;
        void*    begin;
        void*    end;
        void*    cap;
        void*    alloc;
    };
    RawVecHeader tmpVec1 = { 0ULL, nullptr, nullptr, nullptr, entryAlloc };
    if (reinterpret_cast<uint8_t*>(&tmpVec1) != entryVec) {
        void* ea2 = *reinterpret_cast<void**>(u + 0x28);
        auto* vt1 = *reinterpret_cast<uintptr_t**>(entryAlloc);
        uint32_t id1 = reinterpret_cast<uint32_t (*)(void*)>(vt1[0x10 / 8])(entryAlloc);
        auto* vt2 = *reinterpret_cast<uintptr_t**>(ea2);
        uint32_t id2 = reinterpret_cast<uint32_t (*)(void*)>(vt2[0x10 / 8])(ea2);
        if (id1 == id2) {
            void* b = *reinterpret_cast<void**>(u + 0x10);
            void* e = *reinterpret_cast<void**>(u + 0x18);
            void* c = *reinterpret_cast<void**>(u + 0x20);
            *reinterpret_cast<void**>(u + 0x10) = tmpVec1.begin;
            *reinterpret_cast<void**>(u + 0x18) = tmpVec1.end;
            *reinterpret_cast<void**>(u + 0x20) = tmpVec1.cap;
            tmpVec1.begin = b;
            tmpVec1.end   = e;
            tmpVec1.cap   = c;
        } else {
            RawVecHeader moveTmp = { 0ULL, tmpVec1.begin, tmpVec1.end, tmpVec1.cap, tmpVec1.alloc };
            tmpVec1.begin = nullptr;
            tmpVec1.end   = nullptr;
            tmpVec1.cap   = nullptr;
            sub_21ccbc0(&tmpVec1, entryVec);
            sub_21ccbc0(entryVec, &moveTmp);
            if (moveTmp.begin != nullptr) {
                auto* mvt = *reinterpret_cast<uintptr_t**>(moveTmp.alloc);
                reinterpret_cast<void (*)(void*, void*)>(mvt[0x70 / 8])(moveTmp.alloc, moveTmp.begin);
            }
        }
    }
    if (tmpVec1.begin != nullptr) {
        auto* tvt = *reinterpret_cast<uintptr_t**>(tmpVec1.alloc);
        reinterpret_cast<void (*)(void*, void*)>(tvt[0x70 / 8])(tmpVec1.alloc, tmpVec1.begin);
        tmpVec1.begin = nullptr;
        tmpVec1.end   = nullptr;
        tmpVec1.cap   = nullptr;
    }

    // 4. Swap-to-empty on m_segments (+0x30..+0x50)
    uint8_t* segVec = u + 0x30;
    *reinterpret_cast<uint64_t*>(u + 0x40) = *reinterpret_cast<uint64_t*>(u + 0x38);
    void* segAlloc = *reinterpret_cast<void**>(u + 0x50);
    uint64_t segFlags = 0;
    auto* saVtbl = *reinterpret_cast<uintptr_t**>(segAlloc);
    reinterpret_cast<void (*)(void*, void*, uint32_t)>(saVtbl[0x20 / 8])(&segFlags, segAlloc, 0u);
    if ((segFlags & 0x20ULL) == 0) {
        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
            nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    RawVecHeader tmpVec2 = { 0ULL, nullptr, nullptr, nullptr, segAlloc };
    if (reinterpret_cast<uint8_t*>(&tmpVec2) != segVec) {
        void* sa2 = *reinterpret_cast<void**>(u + 0x50);
        auto* vt1 = *reinterpret_cast<uintptr_t**>(segAlloc);
        uint32_t id1 = reinterpret_cast<uint32_t (*)(void*)>(vt1[0x10 / 8])(segAlloc);
        auto* vt2 = *reinterpret_cast<uintptr_t**>(sa2);
        uint32_t id2 = reinterpret_cast<uint32_t (*)(void*)>(vt2[0x10 / 8])(sa2);
        if (id1 == id2) {
            void* b = *reinterpret_cast<void**>(u + 0x38);
            void* e = *reinterpret_cast<void**>(u + 0x40);
            void* c = *reinterpret_cast<void**>(u + 0x48);
            *reinterpret_cast<void**>(u + 0x38) = tmpVec2.begin;
            *reinterpret_cast<void**>(u + 0x40) = tmpVec2.end;
            *reinterpret_cast<void**>(u + 0x48) = tmpVec2.cap;
            tmpVec2.begin = b;
            tmpVec2.end   = e;
            tmpVec2.cap   = c;
        } else {
            RawVecHeader moveTmp = { 0ULL, tmpVec2.begin, tmpVec2.end, tmpVec2.cap, tmpVec2.alloc };
            tmpVec2.begin = nullptr;
            tmpVec2.end   = nullptr;
            tmpVec2.cap   = nullptr;
            sub_21cd130(&tmpVec2, segVec);
            sub_21cd130(segVec, &moveTmp);
            if (moveTmp.begin != nullptr) {
                auto* mvt = *reinterpret_cast<uintptr_t**>(moveTmp.alloc);
                reinterpret_cast<void (*)(void*, void*)>(mvt[0x70 / 8])(moveTmp.alloc, moveTmp.begin);
            }
        }
    }
    if (tmpVec2.begin != nullptr) {
        auto* tvt = *reinterpret_cast<uintptr_t**>(tmpVec2.alloc);
        reinterpret_cast<void (*)(void*, void*)>(tvt[0x70 / 8])(tmpVec2.alloc, tmpVec2.begin);
        tmpVec2.begin = nullptr;
        tmpVec2.end   = nullptr;
        tmpVec2.cap   = nullptr;
    }

    // 5. Finalize DLBiHeap (+0x508..+0x5f0)
    reinterpret_cast<FnFinalizeHeapTracker>(GX_VMA(0x207b8c0ULL))(u + 0x508);
    if (u[0x57f] != 0u) {
        uint64_t alignOff = static_cast<uint64_t>(-static_cast<int32_t>(reinterpret_cast<uintptr_t>(u + 0x518))) & 7ULL;
        uint8_t* alignedBi = u + alignOff + 0x518;
        void* rootBlock = *reinterpret_cast<void**>(u + alignOff + 0x570);
        if (rootBlock != nullptr) {
            reinterpret_cast<FnFreeBiHeapBlock>(GX_VMA(0x2084ae0ULL))(alignedBi, rootBlock);
        }
        uint64_t tailOff = alignOff | 0x48ULL;
        *reinterpret_cast<uint64_t*>(alignedBi + 0x08) = 0ULL;
        *reinterpret_cast<uint64_t*>(alignedBi + 0x00) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + tailOff + 0x528) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + tailOff + 0x520) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + tailOff + 0x518) = 0ULL;
        u[0x57f] = 0u;
    }

    for (uint32_t off = 0x580u; off <= 0x5c0u; off += 8u) {
        *reinterpret_cast<uint64_t*>(u + off) = 0ULL;
    }

    void* biMutex = u + 0x5d8;
    auto* bmVtbl  = *reinterpret_cast<uintptr_t**>(biMutex);
    bool biHasLock = reinterpret_cast<bool (*)(void*)>(bmVtbl[0x10 / 8])(biMutex);
    bool skipBiFree = false;
    if (biHasLock) {
        int32_t err = reinterpret_cast<FnPthreadMutexDestroy>(GX_VMA(0x2bbfee8ULL))(u + 0x5e0);
        if (err == static_cast<int32_t>(0x80020010u)) {
            skipBiFree = true;
        } else {
            if (err != 0) {
                if (err == static_cast<int32_t>(0x80020016u)) {
                    reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x55u,
                        reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                }
                reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                    reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x57u,
                    reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
            }
            u[0x5e8] = 0u;
        }
    }
    if (!skipBiFree) {
        void* biBackingAlloc = *reinterpret_cast<void**>(u + 0x4f0);
        if (biBackingAlloc != nullptr) {
            auto* baVtbl = *reinterpret_cast<uintptr_t**>(biBackingAlloc);
            void* biMem  = *reinterpret_cast<void**>(u + 0x500);
            reinterpret_cast<void (*)(void*, void*)>(baVtbl[0x70 / 8])(biBackingAlloc, biMem);
            *reinterpret_cast<void**>(u + 0x4f0) = nullptr;
            *reinterpret_cast<void**>(u + 0x500) = nullptr;
        }
    }

    // 6. Finalize DLSegregatedBiHeap (+0x80..+0x4e8)
    auto finalizeSegHeapPass = [&]() {
        reinterpret_cast<FnFinalizeHeapTracker>(GX_VMA(0x207b8c0ULL))(u + 0x80);
        std::memset(u + 0xb8, 0, 0x400);
        *reinterpret_cast<uint64_t*>(u + 0xa0) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + 0x98) = 0ULL;

        void* segMutex = u + 0x4d0;
        auto* smVtbl   = *reinterpret_cast<uintptr_t**>(segMutex);
        bool segHasLock = reinterpret_cast<bool (*)(void*)>(smVtbl[0x10 / 8])(segMutex);
        bool skipSegFree = false;
        if (segHasLock) {
            int32_t err = reinterpret_cast<FnPthreadMutexDestroy>(GX_VMA(0x2bbfee8ULL))(u + 0x4d8);
            if (err == static_cast<int32_t>(0x80020010u)) {
                skipSegFree = true;
            } else {
                if (err != 0) {
                    if (err == static_cast<int32_t>(0x80020016u)) {
                        reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                            reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x55u,
                            reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                    }
                    reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x57u,
                        reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
                }
                u[0x4e0] = 0u;
            }
        }
        if (!skipSegFree) {
            void* segBackingAlloc = *reinterpret_cast<void**>(u + 0x68);
            if (segBackingAlloc != nullptr) {
                auto* saVtbl2 = *reinterpret_cast<uintptr_t**>(segBackingAlloc);
                void* segMem  = *reinterpret_cast<void**>(u + 0x78);
                reinterpret_cast<void (*)(void*, void*)>(saVtbl2[0x70 / 8])(segBackingAlloc, segMem);
                *reinterpret_cast<void**>(u + 0x68) = nullptr;
                *reinterpret_cast<void**>(u + 0x78) = nullptr;
            }
        }
    };

    finalizeSegHeapPass();
    reinterpret_cast<FnDestroyAllocatorAdapter>(GX_VMA(0x219d550ULL))(u + 0x4f0);

    if (*reinterpret_cast<void**>(u + 0x98) != nullptr) {
        finalizeSegHeapPass();
    }

    *reinterpret_cast<uintptr_t*>(u + 0x4d0) = GX_VMA(0x53b2fa0ULL);
    if (u[0x4e0] != 0u) {
        int32_t err = reinterpret_cast<FnPthreadMutexDestroy>(GX_VMA(0x2bbfee8ULL))(u + 0x4d8);
        if (err != static_cast<int32_t>(0x80020010u)) {
            if (err != 0) {
                if (err == static_cast<int32_t>(0x80020016u)) {
                    reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x55u,
                        reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                }
                reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                    reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)), 0x57u,
                    reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
            }
            u[0x4e0] = 0u;
        }
    }

    // 7. Free remaining vector buffers (+0x38 and +0x10)
    void* remSegBuf = *reinterpret_cast<void**>(u + 0x38);
    if (remSegBuf != nullptr) {
        void* sa3 = *reinterpret_cast<void**>(u + 0x50);
        auto* vt3 = *reinterpret_cast<uintptr_t**>(sa3);
        reinterpret_cast<void (*)(void*, void*)>(vt3[0x70 / 8])(sa3, remSegBuf);
        *reinterpret_cast<uint64_t*>(u + 0x48) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + 0x40) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + 0x38) = 0ULL;
    }

    void* remEntryBuf = *reinterpret_cast<void**>(u + 0x10);
    if (remEntryBuf != nullptr) {
        void* ea3 = *reinterpret_cast<void**>(u + 0x28);
        auto* vt3 = *reinterpret_cast<uintptr_t**>(ea3);
        reinterpret_cast<void (*)(void*, void*)>(vt3[0x70 / 8])(ea3, remEntryBuf);
        *reinterpret_cast<uint64_t*>(u + 0x20) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + 0x18) = 0ULL;
        *reinterpret_cast<uint64_t*>(u + 0x10) = 0ULL;
    }
}

// ============================================================================
// 49. 0x21ca3d0 (2,441 B) - GXSkinningMatricesUpdater_Construct
// ============================================================================
void sub_21ca3d0(void* self, uint32_t segHeapBlocks, uint32_t biHeapBytes) {
    auto* u = static_cast<uint8_t*>(self);
    *reinterpret_cast<uintptr_t*>(u + 0x00) = GX_VMA(0x535fdc0ULL);

    auto** pGlobalVecAlloc = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    auto initVecAllocator = [&]() -> void* {
        void* alloc = *pGlobalVecAlloc;
        if (alloc == nullptr) {
            alloc = reinterpret_cast<FnGetDefaultStringAllocator>(GX_VMA(0x2ba2b10ULL))();
            *pGlobalVecAlloc = alloc;
        }
        uint64_t flags = 0;
        auto* vtbl = *reinterpret_cast<uintptr_t**>(alloc);
        reinterpret_cast<void (*)(void*, void*, uint32_t)>(vtbl[0x20 / 8])(&flags, alloc, 0u);
        if ((flags & 0x20ULL) == 0) {
            reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                nullptr, 0x3eu, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }
        return alloc;
    };

    void* entryAlloc = initVecAllocator();
    *reinterpret_cast<uint64_t*>(u + 0x20) = 0ULL;
    *reinterpret_cast<uint64_t*>(u + 0x18) = 0ULL;
    *reinterpret_cast<uint64_t*>(u + 0x10) = 0ULL;
    *reinterpret_cast<void**>(u + 0x28)    = entryAlloc;

    void* segAlloc = initVecAllocator();
    *reinterpret_cast<uint64_t*>(u + 0x48) = 0ULL;
    *reinterpret_cast<uint64_t*>(u + 0x40) = 0ULL;
    *reinterpret_cast<uint64_t*>(u + 0x38) = 0ULL;
    *reinterpret_cast<void**>(u + 0x50)    = segAlloc;

    // Initialize DLSegregatedBiHeap (+0x80..+0x4e8) and DLBiHeap (+0x508..+0x5f0)
    *reinterpret_cast<uintptr_t*>(u + 0x80) = GX_VMA(0x52e70c0ULL);
    std::memset(u + 0x88, 0, 0x448);
    *reinterpret_cast<uintptr_t*>(u + 0x4d0) = GX_VMA(0x53b2fa0ULL);
    u[0x4e0]                                 = 0u;
    *reinterpret_cast<uint64_t*>(u + 0x68)   = 0ULL;
    *reinterpret_cast<uint32_t*>(u + 0x70)   = reinterpret_cast<FnGetProcessHeapId>(GX_VMA(0x207b9f0ULL))();
    *reinterpret_cast<uint64_t*>(u + 0x78)   = 0ULL;
    *reinterpret_cast<uint32_t*>(u + 0x4e8)  = 0u;
    *reinterpret_cast<void**>(u + 0x88)      = u + 0x68;

    *reinterpret_cast<uintptr_t*>(u + 0x508) = GX_VMA(0x53b6770ULL);
    *reinterpret_cast<uint64_t*>(u + 0x510)  = 0ULL;
    u[0x5d7]                                 = 0u;
    std::memset(u + 0x57f, 0, 0x58);
    *reinterpret_cast<uintptr_t*>(u + 0x5d8) = GX_VMA(0x53b2fa0ULL);
    u[0x5e8]                                 = 0u;
    *reinterpret_cast<uint64_t*>(u + 0x4f0)  = 0ULL;
    *reinterpret_cast<uint32_t*>(u + 0x4f8)  = reinterpret_cast<FnGetProcessHeapId>(GX_VMA(0x207b9f0ULL))();
    *reinterpret_cast<uint64_t*>(u + 0x500)  = 0ULL;
    *reinterpret_cast<uint32_t*>(u + 0x5f0)  = 0u;
    *reinterpret_cast<void**>(u + 0x510)     = u + 0x4f0;
    *reinterpret_cast<uint32_t*>(u + 0x610)  = 0u;
    *reinterpret_cast<uint64_t*>(u + 0x608)  = 0ULL;
    *reinterpret_cast<uint64_t*>(u + 0x600)  = 0ULL;
    *reinterpret_cast<uint64_t*>(u + 0x5f8)  = 0ULL;

    // Allocate & configure DLSegregatedBiHeap backing store (+0x78)
    uint64_t segByteSize = (segHeapBlocks != 0u)
        ? static_cast<uint64_t>(segHeapBlocks << 6)
        : 0x10000ULL;
    void* defaultHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    if (defaultHeap != nullptr) {
        auto* dhVtbl = *reinterpret_cast<uintptr_t**>(defaultHeap);
        auto* segMem = static_cast<uint8_t*>(
            reinterpret_cast<void* (*)(void*, uint64_t)>(dhVtbl[0x50 / 8])(defaultHeap, segByteSize));
        *reinterpret_cast<uint8_t**>(u + 0x78) = segMem;
        if (segMem != nullptr) {
            *reinterpret_cast<void**>(u + 0x68) = defaultHeap;
            void* segMutex = u + 0x4d0;
            auto* smVtbl   = *reinterpret_cast<uintptr_t**>(segMutex);
            bool hasLock   = reinterpret_cast<bool (*)(void*)>(smVtbl[0x10 / 8])(segMutex);
            bool mutexOk   = true;
            if (!hasLock) {
                if (reinterpret_cast<FnPthreadMutexInit>(GX_VMA(0x2083fe0ULL))(u + 0x4d8) == 0) {
                    u[0x4e0] = 1u;
                } else {
                    mutexOk = false;
                }
            }

            uintptr_t alignedStart = (reinterpret_cast<uintptr_t>(segMem) + 0x17ULL) & ~0xFULL;
            uintptr_t alignedEnd   = (reinterpret_cast<uintptr_t>(segMem) + segByteSize - 8ULL) & ~0xFULL;
            if (!mutexOk || (alignedEnd - alignedStart) < 0x40ULL) {
                if (mutexOk) {
                    std::memset(u + 0x90, 0, 0x440);
                }
                auto* dhVtbl2 = *reinterpret_cast<uintptr_t**>(defaultHeap);
                reinterpret_cast<void (*)(void*, void*)>(dhVtbl2[0x70 / 8])(
                    defaultHeap, *reinterpret_cast<void**>(u + 0x78));
                *reinterpret_cast<void**>(u + 0x78) = nullptr;
            } else {
                auto* pStart = reinterpret_cast<uint8_t*>(alignedStart);
                auto* pEnd   = reinterpret_cast<uint8_t*>(alignedEnd);
                *reinterpret_cast<uint8_t**>(u + 0x98) = pStart;
                *reinterpret_cast<uint8_t**>(u + 0xa0) = pEnd;

                uint8_t* binTable = u + 0xb8;
                for (uint64_t off = 0; off != 0x400ULL; off += 0x10ULL) {
                    uint8_t* selfHead = binTable + off - 0x10;
                    *reinterpret_cast<uint8_t**>(binTable + off + 0x08) = selfHead;
                    *reinterpret_cast<uint8_t**>(binTable + off + 0x00) = selfHead;
                }

                uint8_t* firstNode = pStart - 0x10;
                uint8_t* lastNode  = pEnd - 0x10;
                *reinterpret_cast<uintptr_t*>(pStart - 0x08) = reinterpret_cast<uintptr_t>(lastNode) | 2ULL;
                *reinterpret_cast<uint8_t**>(pEnd - 0x10)    = firstNode;
                *reinterpret_cast<uintptr_t*>(pEnd - 0x08)   = reinterpret_cast<uintptr_t>(firstNode) | 1ULL;

                *reinterpret_cast<uint8_t**>(u + 0x4b8) = firstNode;
                *reinterpret_cast<uint8_t**>(u + 0x4c0) = lastNode;
                uint64_t availSpan = static_cast<uint64_t>(lastNode - firstNode);
                *reinterpret_cast<uint64_t*>(u + 0xa8)  = availSpan;
                *reinterpret_cast<uint64_t*>(u + 0x4c8) = availSpan;
                *reinterpret_cast<uint64_t*>(u + 0xb0)  = 0ULL;
                *reinterpret_cast<uint64_t*>(u + 0x90)  = 0ULL;

                uint64_t binIdx = 63ULL - static_cast<uint64_t>(__builtin_clzll(availSpan));
                uint8_t* binHead = u + (binIdx << 4) + 0xa8;
                uint8_t* oldNext = *reinterpret_cast<uint8_t**>(u + (binIdx << 4) + 0xc0);
                *reinterpret_cast<uint8_t**>(u + (binIdx << 4) + 0xc0) = firstNode;
                *reinterpret_cast<uint8_t**>(pStart + 0x00)            = binHead;
                *reinterpret_cast<uint8_t**>(pStart + 0x08)            = oldNext;
                *reinterpret_cast<uint8_t**>(oldNext + 0x10)           = firstNode;
                if (oldNext == binHead) {
                    *reinterpret_cast<uint64_t*>(u + 0x90) |= (1ULL << binIdx);
                }
                *reinterpret_cast<uint32_t*>(u + 0x4e8) = 0u;
                *reinterpret_cast<void**>(u + 0x88)     = u + 0x68;

                void* tracker = reinterpret_cast<FnGetMemoryTracker>(GX_VMA(0x207bbf0ULL))();
                reinterpret_cast<FnRegisterHeapRange>(GX_VMA(0x207c000ULL))(
                    tracker, u + 0x80, *reinterpret_cast<void**>(u + 0x78), segByteSize);
            }
        }

        // Allocate & configure DLBiHeap backing store (+0x500)
        uint64_t biByteSize = (biHeapBytes != 0u)
            ? static_cast<uint64_t>(biHeapBytes)
            : 0x75300ULL;
        void* defaultHeap2 = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        if (defaultHeap2 != nullptr) {
            auto* dh2Vtbl = *reinterpret_cast<uintptr_t**>(defaultHeap2);
            void* biMem = reinterpret_cast<void* (*)(void*, uint64_t)>(dh2Vtbl[0x50 / 8])(
                defaultHeap2, biByteSize);
            *reinterpret_cast<void**>(u + 0x500) = biMem;
            if (biMem != nullptr) {
                *reinterpret_cast<void**>(u + 0x4f0) = defaultHeap2;
                void* biMutex = u + 0x5d8;
                auto* bmVtbl  = *reinterpret_cast<uintptr_t**>(biMutex);
                bool hasLock  = reinterpret_cast<bool (*)(void*)>(bmVtbl[0x10 / 8])(biMutex);
                bool mutexOk  = true;
                if (!hasLock) {
                    if (reinterpret_cast<FnPthreadMutexInit>(GX_VMA(0x2083fe0ULL))(u + 0x5e0) == 0) {
                        u[0x5e8] = 1u;
                    } else {
                        mutexOk = false;
                    }
                }
                if (mutexOk && reinterpret_cast<FnInitBiHeap>(GX_VMA(0x2087e50ULL))(
                        u + 0x518, u + 0x80, biMem, biByteSize)) {
                    *reinterpret_cast<uint32_t*>(u + 0x5f0) = 0u;
                    *reinterpret_cast<void**>(u + 0x510)    = u + 0x4f0;
                    void* tracker = reinterpret_cast<FnGetMemoryTracker>(GX_VMA(0x207bbf0ULL))();
                    reinterpret_cast<FnRegisterHeapRange>(GX_VMA(0x207c000ULL))(
                        tracker, u + 0x508, *reinterpret_cast<void**>(u + 0x500), biByteSize);
                } else {
                    auto* dh2VtblFree = *reinterpret_cast<uintptr_t**>(defaultHeap2);
                    reinterpret_cast<void (*)(void*, void*)>(dh2VtblFree[0x70 / 8])(
                        defaultHeap2, *reinterpret_cast<void**>(u + 0x500));
                    *reinterpret_cast<void**>(u + 0x500) = nullptr;
                }
            }
        }
    }

    // Reserve 256 entries (0x2800 B) in m_entries (+0x10..+0x20)
    uint64_t curEntryCap = static_cast<uint64_t>(
        (*reinterpret_cast<uint8_t**>(u + 0x20) - *reinterpret_cast<uint8_t**>(u + 0x10)) >> 3) *
        0xCCCCCCCCCCCCCCCDULL;
    if (curEntryCap <= 0xFFULL) {
        void* ea = *reinterpret_cast<void**>(u + 0x28);
        auto* eaVtbl = *reinterpret_cast<uintptr_t**>(ea);
        auto* newBuf = static_cast<uint8_t*>(
            reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(eaVtbl[0x58 / 8])(ea, 0x2800ULL, 8u));
        if (newBuf == nullptr) {
            auto* pIgnore = reinterpret_cast<int32_t*>(ReadTlsQwordAtOffset(0x53e4318ULL));
            if (*pIgnore != 0) {
                reinterpret_cast<FnSetErrnoError>(GX_VMA(0x2bbe978ULL))(
                    reinterpret_cast<void*>(ReadTlsQwordAtOffset(0x53e4320ULL)), 1u);
            } else {
                reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                    nullptr, 0x109u, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
            }
        }
        uint8_t* oldBegin = *reinterpret_cast<uint8_t**>(u + 0x10);
        uint8_t* oldEnd   = *reinterpret_cast<uint8_t**>(u + 0x18);
        if (oldBegin != oldEnd) {
            uint8_t* dst = newBuf;
            for (uint8_t* src = oldBegin; src != oldEnd; src += 0x28, dst += 0x28) {
                if (dst != nullptr) {
                    std::memcpy(dst, src, 0x28);
                }
            }
            oldBegin = *reinterpret_cast<uint8_t**>(u + 0x10);
            oldEnd   = *reinterpret_cast<uint8_t**>(u + 0x18);
        }
        uint64_t count = static_cast<uint64_t>((oldEnd - oldBegin) >> 3) * 0xCCCCCCCCCCCCCCCDULL;
        if (oldBegin != nullptr) {
            void* ea2 = *reinterpret_cast<void**>(u + 0x28);
            auto* vt2 = *reinterpret_cast<uintptr_t**>(ea2);
            reinterpret_cast<void (*)(void*, void*)>(vt2[0x70 / 8])(ea2, oldBegin);
        }
        *reinterpret_cast<uint8_t**>(u + 0x20) = newBuf + 0x2800ULL;
        *reinterpret_cast<uint8_t**>(u + 0x18) = newBuf + count * 40ULL;
        *reinterpret_cast<uint8_t**>(u + 0x10) = newBuf;
    }

    // Reserve 100 entries (0x640 B) in m_segments (+0x38..+0x48)
    uint64_t curSegCap = static_cast<uint64_t>(
        (*reinterpret_cast<uint8_t**>(u + 0x48) - *reinterpret_cast<uint8_t**>(u + 0x38)) >> 4);
    if (curSegCap <= 0x63ULL) {
        void* sa = *reinterpret_cast<void**>(u + 0x50);
        auto* saVtbl = *reinterpret_cast<uintptr_t**>(sa);
        auto* newBuf = static_cast<uint8_t*>(
            reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(saVtbl[0x58 / 8])(sa, 0x640ULL, 8u));
        if (newBuf == nullptr) {
            auto* pIgnore = reinterpret_cast<int32_t*>(ReadTlsQwordAtOffset(0x53e4318ULL));
            if (*pIgnore != 0) {
                reinterpret_cast<FnSetErrnoError>(GX_VMA(0x2bbe978ULL))(
                    reinterpret_cast<void*>(ReadTlsQwordAtOffset(0x53e4320ULL)), 1u);
            } else {
                reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                    nullptr, 0x109u, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
            }
        }
        uint8_t* oldBegin = *reinterpret_cast<uint8_t**>(u + 0x38);
        uint8_t* oldEnd   = *reinterpret_cast<uint8_t**>(u + 0x40);
        if (oldBegin != oldEnd) {
            uint8_t* dst = newBuf;
            for (uint8_t* src = oldBegin; src != oldEnd; src += 0x10, dst += 0x10) {
                if (dst != nullptr) {
                    std::memcpy(dst, src, 0x10);
                }
            }
            oldBegin = *reinterpret_cast<uint8_t**>(u + 0x38);
            oldEnd   = *reinterpret_cast<uint8_t**>(u + 0x40);
        }
        uint64_t count = static_cast<uint64_t>((oldEnd - oldBegin) >> 4);
        if (oldBegin != nullptr) {
            void* sa2 = *reinterpret_cast<void**>(u + 0x50);
            auto* vt2 = *reinterpret_cast<uintptr_t**>(sa2);
            reinterpret_cast<void (*)(void*, void*)>(vt2[0x70 / 8])(sa2, oldBegin);
        }
        *reinterpret_cast<uint8_t**>(u + 0x48) = newBuf + 0x640ULL;
        *reinterpret_cast<uint8_t**>(u + 0x40) = newBuf + (count << 4);
        *reinterpret_cast<uint8_t**>(u + 0x38) = newBuf;
    }

    // Allocate job functors m_pUpdateJob (+0x58) and m_pClearJob (+0x60)
    void* jobHeap1 = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    auto* jhVtbl1  = *reinterpret_cast<uintptr_t**>(jobHeap1);
    auto* job1     = static_cast<uint8_t*>(
        reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(jhVtbl1[0x58 / 8])(jobHeap1, 0x10ULL, 8u));
    if (job1 != nullptr) {
        *reinterpret_cast<uintptr_t*>(job1 + 0x00) = GX_VMA(0x535fde0ULL);
        *reinterpret_cast<void**>(job1 + 0x08)     = u;
    }
    *reinterpret_cast<uint8_t**>(u + 0x58) = job1;

    void* jobHeap2 = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    auto* jhVtbl2  = *reinterpret_cast<uintptr_t**>(jobHeap2);
    auto* job2     = static_cast<uint8_t*>(
        reinterpret_cast<void* (*)(void*, uint64_t, uint32_t)>(jhVtbl2[0x58 / 8])(jobHeap2, 0x10ULL, 8u));
    if (job2 != nullptr) {
        *reinterpret_cast<uintptr_t*>(job2 + 0x00) = GX_VMA(0x535fe10ULL);
        *reinterpret_cast<void**>(job2 + 0x08)     = u;
    }
    *reinterpret_cast<uint8_t**>(u + 0x60) = job2;

    void* globalDev = *reinterpret_cast<void**>(GX_VMA(0x55406c8ULL));
    if (globalDev != nullptr) {
        reinterpret_cast<FnRegisterWithGlobalDevice>(GX_VMA(0x2199f00ULL))(globalDev, u);
    }

    // Compute initial heap telemetry (+0x5fc..+0x614)
    void* biMutex = u + 0x5d8;
    auto* bmVtbl1 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(bmVtbl1[0x18 / 8])(biMutex, 0xFFFFFFFFu);
    uint32_t biFree = *reinterpret_cast<const uint32_t*>(u + 0x5d0);
    auto* bmVtbl2 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*)>(bmVtbl2[0x28 / 8])(biMutex);
    *reinterpret_cast<uint32_t*>(u + 0x5fc) = biFree;

    auto* bmVtbl3 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(bmVtbl3[0x18 / 8])(biMutex, 0xFFFFFFFFu);
    uint8_t* biSentinel = u + 0x580;
    uint8_t* biNode     = *reinterpret_cast<uint8_t**>(u + 0x5a0);
    uint64_t maxBiBlock = 0ULL;
    while (biNode != biSentinel) {
        uint8_t* nextGeo = *reinterpret_cast<uint8_t**>(biNode + 0x10);
        if (nextGeo == nullptr) {
            reinterpret_cast<FnLogWarning>(GX_VMA(0x20b55b0ULL))(
                reinterpret_cast<const char*>(GX_VMA(0x493c8e8ULL)), 0x310u,
                reinterpret_cast<const char*>(GX_VMA(0x493cb05ULL)));
        }
        uint64_t span = static_cast<uint64_t>(
            *reinterpret_cast<uint64_t*>(nextGeo) - *reinterpret_cast<uint64_t*>(biNode));
        if (maxBiBlock < span) {
            maxBiBlock = span;
        }
        biNode = *reinterpret_cast<uint8_t**>(biNode + 0x20);
    }
    auto* bmVtbl4 = *reinterpret_cast<uintptr_t**>(biMutex);
    reinterpret_cast<void (*)(void*)>(bmVtbl4[0x28 / 8])(biMutex);
    *reinterpret_cast<uint32_t*>(u + 0x600) = static_cast<uint32_t>(maxBiBlock);

    void* segMutex = u + 0x4d0;
    auto* smVtbl1  = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(smVtbl1[0x18 / 8])(segMutex, 0xFFFFFFFFu);
    uint32_t segFree = *reinterpret_cast<const uint32_t*>(u + 0xa8);
    auto* smVtbl2  = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*)>(smVtbl2[0x28 / 8])(segMutex);
    *reinterpret_cast<uint32_t*>(u + 0x608) = segFree;

    auto* smVtbl3  = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*, uint32_t)>(smVtbl3[0x18 / 8])(segMutex, 0xFFFFFFFFu);
    uint64_t bitmask = *reinterpret_cast<const uint64_t*>(u + 0x90);
    uint64_t maxSegBlock = 0ULL;
    if (bitmask != 0ULL) {
        uint64_t binIdx = 63ULL - static_cast<uint64_t>(__builtin_clzll(bitmask));
        uint8_t* binHead = u + (binIdx << 4) + 0xa8;
        uint8_t* segNode = *reinterpret_cast<uint8_t**>(u + (binIdx << 4) + 0xc0);
        while (segNode != binHead) {
            uint64_t rawNext = *reinterpret_cast<const uint64_t*>(segNode + 0x08) & ~3ULL;
            uint64_t segSize = (rawNext - reinterpret_cast<uintptr_t>(segNode)) - 8ULL;
            if (maxSegBlock < segSize) {
                maxSegBlock = segSize;
            }
            segNode = *reinterpret_cast<uint8_t**>(segNode + 0x18);
        }
    }
    auto* smVtbl4 = *reinterpret_cast<uintptr_t**>(segMutex);
    reinterpret_cast<void (*)(void*)>(smVtbl4[0x28 / 8])(segMutex);
    *reinterpret_cast<uint32_t*>(u + 0x60c) = static_cast<uint32_t>(maxSegBlock);

    uint32_t biTotal = *reinterpret_cast<const uint32_t*>(u + 0x5c0) -
                       *reinterpret_cast<const uint32_t*>(u + 0x5b8);
    *reinterpret_cast<uint32_t*>(u + 0x604) = biTotal;
    *reinterpret_cast<uint32_t*>(u + 0x610) = *reinterpret_cast<const uint32_t*>(u + 0x4c8);
    *reinterpret_cast<uint32_t*>(u + 0x614) = 0u;
}

// ============================================================================
// 50. 0x21ca340 (129 B) - GXSkinningClearJob_Execute
// ============================================================================
void sub_21ca340(void* jobSelf) {
    auto* job     = static_cast<uint8_t*>(jobSelf);
    auto* updater = *reinterpret_cast<uint8_t**>(job + 0x08);
    uint8_t* cur  = *reinterpret_cast<uint8_t**>(updater + 0x10);
    if (cur != *reinterpret_cast<uint8_t**>(updater + 0x18)) {
        void* biHeap = updater + 0x508;
        do {
            auto freeBlock = [&](void* blk) {
                auto* vtbl = *reinterpret_cast<uintptr_t**>(biHeap);
                reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(biHeap, blk);
            };
            freeBlock(*reinterpret_cast<void**>(cur + 0x20));
            freeBlock(*reinterpret_cast<void**>(cur + 0x18));
            freeBlock(*reinterpret_cast<void**>(cur + 0x10));
            freeBlock(*reinterpret_cast<void**>(cur + 0x08));
            cur += 0x28;
        } while (cur != *reinterpret_cast<uint8_t**>(updater + 0x18));
        cur = *reinterpret_cast<uint8_t**>(updater + 0x10);
    }
    *reinterpret_cast<uint8_t**>(updater + 0x18) = cur;
}

// ============================================================================
// 51. 0x21ca290 (166 B) - GXSkinningUpdateJob_ExecuteRange
// ============================================================================
void sub_21ca290(void* jobSelf, const int32_t* rangePair, uint32_t flag) {
    int64_t idx = static_cast<int64_t>(rangePair[0]);
    if (static_cast<int32_t>(idx) >= rangePair[1]) {
        return;
    }

    auto* job     = static_cast<uint8_t*>(jobSelf);
    auto* updater = *reinterpret_cast<uint8_t**>(job + 0x08);
    uint64_t byteOff = static_cast<uint64_t>(idx) * 40ULL + 0x20ULL;

    do {
        uint8_t* vBegin = *reinterpret_cast<uint8_t**>(updater + 0x10);
        uint8_t* vEnd   = *reinterpret_cast<uint8_t**>(updater + 0x18);
        uint64_t totalEntries = static_cast<uint64_t>((vEnd - vBegin) >> 3) * 0xCCCCCCCCCCCCCCCDULL;
        if (totalEntries <= static_cast<uint64_t>(idx)) {
            break;
        }

        uint8_t* entryOff20 = vBegin + byteOff;
        void* dst1 = *reinterpret_cast<void**>(entryOff20 - 0x10);
        if (dst1 != nullptr) {
            void* target      = *reinterpret_cast<void**>(entryOff20 - 0x20);
            const void* prev1 = *reinterpret_cast<const void**>(entryOff20 + 0x00);
            reinterpret_cast<FnExecuteTargetSkinning>(GX_VMA(0x21c8c50ULL))(target, dst1, prev1, flag);
        }

        void* target      = *reinterpret_cast<void**>(entryOff20 - 0x20);
        void* dst0        = *reinterpret_cast<void**>(entryOff20 - 0x18);
        const void* prev0 = *reinterpret_cast<const void**>(entryOff20 - 0x08);
        reinterpret_cast<FnExecuteTargetSkinning>(GX_VMA(0x21c8c50ULL))(target, dst0, prev0, flag);

        ++idx;
        byteOff += 0x28ULL;
    } while (static_cast<int32_t>(idx) < rangePair[1]);
}

// ============================================================================
// 52. 0x21ca250 (62 B) - GXSkinningVector_DestroyBuffer
// ============================================================================
void sub_21ca250(void* vecSelf) {
    auto* v      = static_cast<uint8_t*>(vecSelf);
    void* bufPtr = *reinterpret_cast<void**>(v + 0x08);
    if (bufPtr != nullptr) {
        void* alloc = *reinterpret_cast<void**>(v + 0x20);
        auto* vtbl  = *reinterpret_cast<uintptr_t**>(alloc);
        reinterpret_cast<void (*)(void*, void*)>(vtbl[0x70 / 8])(alloc, bufPtr);
        *reinterpret_cast<uint64_t*>(v + 0x18) = 0ULL;
        *reinterpret_cast<uint64_t*>(v + 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(v + 0x08) = 0ULL;
    }
}

// ============================================================================
// 53. 0x21c9c00 (1,615 B) - GXSortKey64_Introsort
// ============================================================================
void sub_21c9c00(uint64_t* first, uint64_t* last, int64_t depthLimit) {
    auto key32 = [](uint64_t v) -> uint32_t {
        return static_cast<uint32_t>(v);
    };
    auto swapIfLess = [&](uint64_t* a, uint64_t* b) {
        if (key32(*b) < key32(*a)) {
            uint64_t tmp = *a;
            *a = *b;
            *b = tmp;
        }
    };
    auto medianOf3 = [&](uint64_t* a, uint64_t* b, uint64_t* c) {
        swapIfLess(a, b);
        if (key32(*c) < key32(*b)) {
            uint64_t tmp = *c;
            *c = *b;
            *b = tmp;
            swapIfLess(a, b);
        }
    };

    ptrdiff_t byteSpan = reinterpret_cast<uint8_t*>(last) - reinterpret_cast<uint8_t*>(first);
    bool wasLarge = (byteSpan > 0x100);

    while (byteSpan >= 0x101 && depthLimit > 0) {
        uint64_t* lastMinus1 = last - 1;
        int64_t count = last - first;
        int64_t midIdx = count / 2;
        uint64_t* mid = first + midIdx;

        ptrdiff_t spanMinus8 = reinterpret_cast<uint8_t*>(lastMinus1) - reinterpret_cast<uint8_t*>(first);
        if (spanMinus8 >= 0x141) {
            int64_t step = ((spanMinus8 >> 3) + 1) / 8;
            int64_t step2 = step * 2;
            medianOf3(first, first + step, first + step2);
            medianOf3(mid - step, mid, mid + step);
            medianOf3(last - 1 - step2, last - 1 - step, lastMinus1);
            medianOf3(first + step, mid, last - 1 - step);
        } else {
            medianOf3(first, mid, lastMinus1);
        }

        // 3-way partition around [pLeft, pRight) equal to pivot key
        uint64_t* pLeft = mid;
        while (pLeft > first && key32(*pLeft) == key32(*(pLeft - 1))) {
            --pLeft;
        }
        uint64_t* pRight = mid + 1;
        while (pRight < last && key32(*pLeft) == key32(*pRight)) {
            ++pRight;
        }

        uint64_t* i = pRight;
        uint64_t* j = pLeft;
        for (;;) {
            for (; i < last; ++i) {
                uint32_t kPivot = key32(*pLeft);
                uint32_t kElem  = key32(*i);
                if (kPivot < kElem) {
                    continue;
                }
                if (kElem < kPivot) {
                    break;
                }
                uint64_t tmp = *pRight;
                *pRight = *i;
                *i = tmp;
                ++pRight;
            }
            for (; j > first; --j) {
                uint32_t kPrev  = key32(*(j - 1));
                uint32_t kPivot = key32(*pLeft);
                if (kPrev < kPivot) {
                    continue;
                }
                if (kPivot < kPrev) {
                    break;
                }
                --pLeft;
                uint64_t tmp = *pLeft;
                *pLeft = *(j - 1);
                *(j - 1) = tmp;
            }

            if (j == first && i == last) {
                break;
            }
            if (j == first) {
                if (pRight != i) {
                    uint64_t tmp = *pLeft;
                    *pLeft = *pRight;
                    *pRight = tmp;
                }
                ++pRight;
                uint64_t tmp = *pLeft;
                *pLeft = *i;
                *i = tmp;
                ++pLeft;
                ++i;
            } else if (i == last) {
                --j;
                --pLeft;
                if (pLeft != j) {
                    uint64_t tmp = *j;
                    *j = *pLeft;
                    *pLeft = tmp;
                }
                --pRight;
                uint64_t tmp = *pLeft;
                *pLeft = *pRight;
                *pRight = tmp;
            } else {
                --j;
                uint64_t tmp = *i;
                *i = *j;
                *j = tmp;
                ++i;
            }
        }

        depthLimit = (depthLimit / 2) + (depthLimit / 4);
        ptrdiff_t leftBytes  = reinterpret_cast<uint8_t*>(pLeft) - reinterpret_cast<uint8_t*>(first);
        ptrdiff_t rightBytes = reinterpret_cast<uint8_t*>(last)  - reinterpret_cast<uint8_t*>(pRight);
        if (leftBytes < rightBytes) {
            sub_21c9c00(first, pLeft, depthLimit);
            first    = pRight;
            byteSpan = rightBytes;
            wasLarge = (byteSpan > 0x100);
        } else {
            sub_21c9c00(pRight, last, depthLimit);
            last     = pLeft;
            byteSpan = leftBytes;
            wasLarge = (byteSpan > 0x100);
        }
    }

    if (wasLarge) {
        // Heapsort fallback when depthLimit reaches 0
        int64_t n = byteSpan >> 3;
        int64_t half = n / 2;
        while (half > 0) {
            --half;
            uint64_t val = first[half];
            int64_t hole = half;
            int64_t child = 2 * hole + 2;
            while (child < n) {
                if (key32(first[child]) < key32(first[child - 1])) {
                    --child;
                }
                first[hole] = first[child];
                hole = child;
                child = 2 * child + 2;
            }
            if (child == n) {
                first[hole] = first[n - 1];
                hole = n - 1;
            }
            int64_t parent = (hole - 1) / 2;
            while (hole > half && key32(first[parent]) < key32(val)) {
                first[hole] = first[parent];
                hole = parent;
                parent = (hole - 1) / 2;
            }
            first[hole] = val;
        }

        uint64_t* endHeap = last;
        while ((reinterpret_cast<uint8_t*>(endHeap) - reinterpret_cast<uint8_t*>(first)) > 8) {
            --endHeap;
            uint64_t val = *endHeap;
            *endHeap = *first;
            int64_t remCount = endHeap - first;
            int64_t hole = 0;
            int64_t child = 2;
            while (child < remCount) {
                if (key32(first[child]) < key32(first[child - 1])) {
                    --child;
                }
                first[hole] = first[child];
                hole = child;
                child = 2 * child + 2;
            }
            if (child == remCount) {
                first[hole] = first[remCount - 1];
                hole = remCount - 1;
            }
            while (hole > 0) {
                int64_t parent = (hole - 1) / 2;
                if (key32(first[parent]) >= key32(val)) {
                    break;
                }
                first[hole] = first[parent];
                hole = parent;
            }
            first[hole] = val;
        }
        return;
    }

    // Final insertion sort for small partitions (byteSpan >= 9)
    if (byteSpan >= 9 && first != last && (first + 1) != last) {
        for (uint64_t* cur = first + 1; cur != last; ++cur) {
            uint64_t val = *cur;
            if (key32(val) < key32(*first)) {
                for (uint64_t* p = cur; p != first; --p) {
                    *p = *(p - 1);
                }
                *first = val;
            } else {
                uint64_t* p = cur;
                while (key32(val) < key32(*(p - 1))) {
                    *p = *(p - 1);
                    --p;
                }
                *p = val;
            }
        }
    }
}

// ============================================================================
// 54. 0x21c9aa0 (8 B) - GXSetGlobalSceneRenderCallback_56a6cf0
// ============================================================================
void sub_21c9aa0(void* callbackPtr) {
    *reinterpret_cast<void**>(GX_VMA(0x56a6cf0ULL)) = callbackPtr;
}

// ============================================================================
// 55. 0x21c9a90 (8 B) - GXSetGlobalSceneRenderCallback_56a6ce8
// ============================================================================
void sub_21c9a90(void* callbackPtr) {
    *reinterpret_cast<void**>(GX_VMA(0x56a6ce8ULL)) = callbackPtr;
}

} // extern "C"
