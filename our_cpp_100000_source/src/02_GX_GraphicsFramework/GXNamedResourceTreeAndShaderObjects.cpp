// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Module 02: GX_GraphicsFramework
// File: GXNamedResourceTreeAndShaderObjects.cpp
// Address Range: 0x21a50c0 .. 0x21a9b20 (84 functions, 18,705 bytes)
//
// Contains:
//   1. GXNamedResourceMap (0x21a8680 .. 0x21a9b20, 18 functions):
//      Case-insensitive Red-Black Tree dictionary (`0xD0`-byte nodes with
//      `DLWString` keys and ref-counted `GXResourceBase*` payloads) with
//      hierarchical parent-map fallback and total byte-size tracking.
//   2. GXTextureColorScaleCBuffer (0x21a8250 .. 0x21a85e0, 7 functions):
//      16-byte GPU constant buffer wrapper for HDR texture color-scale factors
//      extracted from PS4 GNF user-data tag `0x3FC` or DDS tail chunks.
//   3. GXResourceBase, GXCubeTextureResource & GXTexture2DResource
//      (0x21a68b0 .. 0x21a8140, 27 functions):
//      High-level GX texture resource wrappers managing `CGTextureGnm*`,
//      SRV/UAV views, DXGI-to-GNM typeless format remapping (`0x21a6c00`),
//      and `GXResourceMemoryTracker` registration (buckets 0 & 1).
//   4. GXPixelShaderResource, GXComputeShaderResource, GXGeometryShaderResource,
//      GXVertexShaderResource, and GXHullOrDomainShaderResource
//      (0x21a50c0 .. 0x21a68a0, 32 functions):
//      High-level GX shader resource wrappers managing `CG*ShaderGnm*` objects,
//      optional CPU bytecode copies, `GXResourceMemoryTracker` shader telemetry
//      (`+0x17C..+0x184`), and `GXDeviceContext` stage binding.
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cwchar>
#include <wctype.h>

#include "gx_direct_vulkan.h"

namespace Bloodborne {
namespace GX {

// Forward declarations
struct GXResourceBase;
struct GXDeviceContext;
struct GXResourceMemoryTracker;

extern "C" {
    void* GXGlobalDevice_GetInstance(); // *(void**)0x55406c8
    void* CGObject_GetCreatorAllocator(void* pObj); // 0x207b720
    void  CGBufferGnm_Release(void* pBuf); // 0x2192660
    int   GnfUserData_GetChunkCount(const void* pGnf, uint32_t size); // 0x26d8c10
    const float* GnfUserData_GetChunkByIndex(const void* pGnf, uint32_t size, int idx, uint32_t* outSize, uint32_t* outTag); // 0x26d8df0
    int   GXDevice_CreateConstantBuffer(void* pDev, void** outBuf, const void* pDesc, const void* pInitData, void* a5, void* a6, void* a7); // 0x2165c10
    int   GXDevice_CreateTexture(void* pDev, void** outTex, const void* pDesc, const void* pInitData, void* a5, void* a6, void* a7); // 0x2165f90
    int   GXDevice_CreateShaderResourceView(void* pDev, void** outSrv, void* pTex, const void* pSrvDesc, void* a5); // 0x21669a0
    int   GXDevice_CreateUnorderedAccessView(void* pDev, void** outUav, void* pTex, const void* pUavDesc, void* a5); // 0x2166b60
    int   GXDevice_CreatePixelShader(void* pDev, void** outPs, const void* pCode, size_t size, void* a5, void* a6); // 0x2166d00
    int   GXDevice_CreateHullOrDomainShader(void* pDev, void** outShd, const void* pCode, size_t size, void* a5, void* a6); // 0x2166e10
    int   GXDevice_CreateVertexShader(void* pDev, void** outVs, const void* pCode, size_t size, void* a5, void* a6); // 0x2167030
    int   GXDevice_CreateGeometryShaderWithSO(void* pDev, void** outGs, const void* pCode, size_t size, void* a5, uint32_t a6, void* a7, uint32_t a8, uint32_t a9, void* a10, void* a11); // 0x2167140
    int   GXDevice_CreateComputeShader(void* pDev, void** outCs, const void* pCode, size_t size, void* a5, void* a6); // 0x21672d0
    void* sub_2bbe578(void* pDst, const void* pSrc, size_t len);
    void  sub_2bbe588(void* pMem);
    void* sub_2bbe658(void* pDst, const void* pSrc, size_t len);
    void  sub_207d250();
    void* sub_261a1b0(void* pDst, const void* pSrc, size_t len);
    void  sub_27cbc20(void* pStr, uint64_t newCap, uint64_t oldLen);
    void  sub_27cba30(void* pStr, const wchar_t* pszSrc, uint64_t len);
    void  sub_29b8ca0(void* pDstStr, const void* pSrcStr, uint64_t pos, uint64_t count);
    size_t sub_2bbf228(const wchar_t* pszStr);
    const int16_t* sub_2bbec48();
    void  sub_20b55b0(const void* pFile, uint32_t line, const void* pMsg);
    void  sub_2bbf1a8(const char* pszMsg);
    void  sub_2bbf1b8(const char* pszMsg);
    void  sub_2bbe978(void* pTlsSlot, uint32_t code);
}

// ============================================================================
// 1. GXNamedResourceMap — Case-Insensitive Red-Black Tree Dictionary
//    (0x21a8680 .. 0x21a9b20)
// ============================================================================

struct DLFixedWString31 {
    void*    m_pStrHeader00;      // +0x00 (node + 0x20)
    union {
        wchar_t  m_inlineBuf[8];
        wchar_t* m_pHeapBuf;
    } m_data08;                   // +0x08 (node + 0x28)
    uint64_t m_length18;          // +0x18 (node + 0x38)
    uint64_t m_capacity20;        // +0x20 (node + 0x40)
    uint8_t  m_fixedBuf28[0x41];  // +0x28 (node + 0x48 .. +0x88)
    uint8_t  m_inUseFlag69;       // +0x69 (node + 0x89)
    uint8_t  m_bOwnsMemory6A;     // +0x6A (node + 0x8A)
    uint8_t  m_pad6B[5];          // +0x6B (node + 0x8B .. +0x8F)
};

struct GXNamedResourcePayload {
    GXResourceBase* m_pResource;       // +0x00 (node + 0x90)
    uint32_t        m_flags;           // +0x08 (node + 0x98)
    uint32_t        m_pad0C;           // +0x0C (node + 0x9C)
    float           m_colorScale;      // +0x10 (node + 0xA0)
    uint32_t        m_unk14_lo;        // +0x14 (node + 0xA4)
    uint32_t        m_unk14_hi;        // +0x18 (node + 0xA8)
    uint32_t        m_unk1C;           // +0x1C (node + 0xAC)
    uint64_t        m_uidLow;          // +0x20 (node + 0xB0)
    uint64_t        m_uidHigh;         // +0x28 (node + 0xB8)
    void*           m_pColorScaleCBuf; // +0x30 (node + 0xC0)
    uint64_t        m_reserved38;      // +0x38 (node + 0xC8)
};
static_assert(sizeof(GXNamedResourcePayload) == 0x40, "GXNamedResourcePayload must be 0x40 bytes");

struct GXNamedResourceKeyValuePair {
    DLFixedWString31       m_key00;     // +0x00 .. +0x6F
    GXNamedResourcePayload m_payload70; // +0x70 .. +0xAF
};
static_assert(sizeof(GXNamedResourceKeyValuePair) == 0xB0, "GXNamedResourceKeyValuePair must be 0xB0 bytes");

struct GXNamedResourceTreeNode {
    GXNamedResourceTreeNode* m_pLeft;    // +0x00
    GXNamedResourceTreeNode* m_pParent;  // +0x08
    GXNamedResourceTreeNode* m_pRight;   // +0x10
    uint8_t                  m_color;    // +0x18 (0 = Red, 1 = Black)
    uint8_t                  m_isNil;    // +0x19 (1 = Sentinel)
    uint8_t                  m_pad1A[6]; // +0x1A
    DLFixedWString31         m_key;      // +0x20 .. +0x8F
    GXNamedResourcePayload   m_payload;  // +0x90 .. +0xCF (0x40 bytes)
};
static_assert(sizeof(GXNamedResourceTreeNode) == 0xD0, "GXNamedResourceTreeNode must be 0xD0 bytes");

struct GXNamedResourceTreeIter {
    void*                    m_pContainer00; // +0x00
    void*                    m_pContainer08; // +0x08
    GXNamedResourceTreeNode* m_pNode10;      // +0x10
};

struct GXNamedResourceTreeBase {
    void*                    m_pTreeAlloc00; // +0x00 (map + 0x10)
    GXNamedResourceTreeNode* m_pHead08;      // +0x08 (map + 0x18)
    uint64_t                 m_nodeCount10;  // +0x10 (map + 0x20)
    void*                    m_pNodeAlloc18; // +0x18 (map + 0x28)
};

struct GXNamedResourceMap {
    void**                   m_vptr;       // +0x00 (0x535f980)
    void**                   m_vptr08;     // +0x08 (0x535f9c0)
    void*                    m_pTreeAlloc; // +0x10
    GXNamedResourceTreeNode* m_pHead;      // +0x18 (RB-tree nil/header sentinel)
    uint64_t                 m_nodeCount;  // +0x20
    void*                    m_pNodeAlloc; // +0x28
    uint32_t                 m_totalBytes; // +0x30
    uint32_t                 m_pad34;      // +0x34
    GXNamedResourceMap*      m_pParentMap; // +0x38
    uint64_t                 m_unk40;      // +0x40
    uint64_t                 m_unk48;      // +0x48
};

static inline const wchar_t* GXFixedWString_GetPtr(const DLFixedWString31* pStr) {
    return (pStr->m_capacity20 >= 8) ? pStr->m_data08.m_pHeapBuf : pStr->m_data08.m_inlineBuf;
}

static inline wchar_t* GXFixedWString_GetMutablePtr(DLFixedWString31* pStr) {
    return (pStr->m_capacity20 >= 8) ? pStr->m_data08.m_pHeapBuf : pStr->m_data08.m_inlineBuf;
}

static inline int32_t GXCompareWideStringsCaseInsensitive(const wchar_t* pLhs, const wchar_t* pRhs) {
    const int16_t* pLowerTbl = sub_2bbec48();
    int16_t c1;
    int16_t c2;
    for (;;) {
        uint8_t b1 = *reinterpret_cast<const uint8_t*>(pLhs);
        c1 = static_cast<int8_t>(pLowerTbl[b1] & 0xFF);

        uint8_t b2 = *reinterpret_cast<const uint8_t*>(pRhs);
        c2 = static_cast<int8_t>(pLowerTbl[b2] & 0xFF);

        if (static_cast<uint8_t>(c1) == 0) {
            break;
        }
        ++pLhs;
        ++pRhs;
        if (c1 != c2) {
            break;
        }
    }
    return static_cast<int32_t>(static_cast<uint16_t>(c1)) - static_cast<int32_t>(static_cast<uint16_t>(c2));
}

static inline void GXRBTree_RotateLeft(GXNamedResourceTreeNode* pHead, GXNamedResourceTreeNode* pX) {
    GXNamedResourceTreeNode* pY = pX->m_pRight;
    pX->m_pRight = pY->m_pLeft;
    if (pY->m_pLeft->m_isNil == 0) {
        pY->m_pLeft->m_pParent = pX;
    }
    pY->m_pParent = pX->m_pParent;
    if (pHead->m_pParent == pX) {
        pHead->m_pParent = pY;
    } else if (pX->m_pParent->m_pLeft == pX) {
        pX->m_pParent->m_pLeft = pY;
    } else {
        pX->m_pParent->m_pRight = pY;
    }
    pY->m_pLeft = pX;
    pX->m_pParent = pY;
}

static inline void GXRBTree_RotateRight(GXNamedResourceTreeNode* pHead, GXNamedResourceTreeNode* pX) {
    GXNamedResourceTreeNode* pY = pX->m_pLeft;
    pX->m_pLeft = pY->m_pRight;
    if (pY->m_pRight->m_isNil == 0) {
        pY->m_pRight->m_pParent = pX;
    }
    pY->m_pParent = pX->m_pParent;
    if (pHead->m_pParent == pX) {
        pHead->m_pParent = pY;
    } else if (pX->m_pParent->m_pRight == pX) {
        pX->m_pParent->m_pRight = pY;
    } else {
        pX->m_pParent->m_pLeft = pY;
    }
    pY->m_pRight = pX;
    pX->m_pParent = pY;
}

// ----------------------------------------------------------------------------
// 0x21a9b20 (1129 B): GXNamedResourceTree_InsertNode
// Allocates a 0xD0-byte RB-tree node, copies the lowercase DLWString key and
// 64-byte GXNamedResourcePayload, links into parent `pWhere`, and rebalances.
// ----------------------------------------------------------------------------
GXNamedResourceTreeIter* GXNamedResourceTree_InsertNode(
    GXNamedResourceTreeIter* pOutIter,
    GXNamedResourceTreeBase* pTreeBase,
    bool insertLeft,
    GXNamedResourceTreeNode* pWhere,
    const GXNamedResourceKeyValuePair* pKeyPairAndPayload)
{
    if (pTreeBase->m_nodeCount10 >= 0x013B13B13B13B13AULL) {
        sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940ac6ULL)));
    }

    using AllocAlignedFn = void* (*)(void*, size_t, size_t);
    void** pAllocVtbl = *reinterpret_cast<void***>(pTreeBase->m_pNodeAlloc18);
    auto* pNewNode = static_cast<GXNamedResourceTreeNode*>(
        reinterpret_cast<AllocAlignedFn>(pAllocVtbl[0x58 / 8])(pTreeBase->m_pNodeAlloc18, 0xD0, 0x10));

    if (!pNewNode) {
        uintptr_t tlsBase = 0;
        tlsBase = gx_guest_tls_base();
        int32_t* pTlsErrFlag = reinterpret_cast<int32_t*>(tlsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4318ULL)));
        if (*pTlsErrFlag != 0) {
            void* pTlsSlot = reinterpret_cast<void*>(tlsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4320ULL)));
            sub_2bbe978(pTlsSlot, 1);
        }
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }

    GXNamedResourceTreeNode* pHead = pTreeBase->m_pHead08;
    pNewNode->m_pLeft = pHead;
    pNewNode->m_pParent = pHead;
    pNewNode->m_pRight = pHead;
    pNewNode->m_color = 0;
    pNewNode->m_isNil = 0;

    uint8_t stackScratch[0x41];
    sub_2bbe578(pNewNode->m_key.m_fixedBuf28, stackScratch, 0x41);
    pNewNode->m_key.m_inUseFlag69 = 0;
    pNewNode->m_key.m_capacity20 = 7;
    pNewNode->m_key.m_length18 = 0;
    pNewNode->m_key.m_data08.m_inlineBuf[0] = L'\0';
    pNewNode->m_key.m_bOwnsMemory6A = pKeyPairAndPayload->m_key00.m_bOwnsMemory6A;

    sub_27cbc20(&pNewNode->m_key, 0x1Fu, 0u);
    pNewNode->m_key.m_length18 = 0;
    *GXFixedWString_GetMutablePtr(&pNewNode->m_key) = L'\0';

    uint64_t copyLen = (pKeyPairAndPayload->m_key00.m_length18 < 0x1Fu)
        ? pKeyPairAndPayload->m_key00.m_length18
        : 0x1Fu;
    sub_29b8ca0(&pNewNode->m_key, &pKeyPairAndPayload->m_key00, 0u, copyLen);

    pNewNode->m_payload = pKeyPairAndPayload->m_payload70;
    pTreeBase->m_nodeCount10++;

    pNewNode->m_pParent = pWhere;
    if (pTreeBase->m_pHead08 == pWhere) {
        pWhere->m_pParent = pNewNode;
        pTreeBase->m_pHead08->m_pLeft = pNewNode;
        pTreeBase->m_pHead08->m_pRight = pNewNode;
    } else if (insertLeft) {
        pWhere->m_pLeft = pNewNode;
        if (pTreeBase->m_pHead08->m_pLeft == pWhere) {
            pTreeBase->m_pHead08->m_pLeft = pNewNode;
        }
    } else {
        pWhere->m_pRight = pNewNode;
        if (pTreeBase->m_pHead08->m_pRight == pWhere) {
            pTreeBase->m_pHead08->m_pRight = pNewNode;
        }
    }

    // Red-Black Tree Insert Rebalance
    GXNamedResourceTreeNode* pCur = pNewNode;
    GXNamedResourceTreeNode* pParent = pCur->m_pParent;
    while (pParent->m_color == 0) {
        GXNamedResourceTreeNode* pGrand = pParent->m_pParent;
        if (pParent == pGrand->m_pLeft) {
            GXNamedResourceTreeNode* pUncle = pGrand->m_pRight;
            if (pUncle->m_color == 0) {
                pParent->m_color = 1;
                pUncle->m_color = 1;
                pGrand->m_color = 0;
                pCur = pGrand;
            } else {
                if (pCur == pParent->m_pRight) {
                    GXRBTree_RotateLeft(pTreeBase->m_pHead08, pParent);
                    pCur = pParent;
                    pParent = pCur->m_pParent;
                    pGrand = pParent->m_pParent;
                }
                pParent->m_color = 1;
                pGrand->m_color = 0;
                GXRBTree_RotateRight(pTreeBase->m_pHead08, pGrand);
            }
        } else {
            GXNamedResourceTreeNode* pUncle = pGrand->m_pLeft;
            if (pUncle->m_color == 0) {
                pParent->m_color = 1;
                pUncle->m_color = 1;
                pGrand->m_color = 0;
                pCur = pGrand;
            } else {
                if (pCur == pParent->m_pLeft) {
                    GXRBTree_RotateRight(pTreeBase->m_pHead08, pParent);
                    pCur = pParent;
                    pParent = pCur->m_pParent;
                    pGrand = pParent->m_pParent;
                }
                pParent->m_color = 1;
                pGrand->m_color = 0;
                GXRBTree_RotateLeft(pTreeBase->m_pHead08, pGrand);
            }
        }
        pParent = pCur->m_pParent;
    }

    pTreeBase->m_pHead08->m_pParent->m_color = 1;
    if (pOutIter) {
        pOutIter->m_pNode10 = pNewNode;
    }
    return pOutIter;
}

// ----------------------------------------------------------------------------
// 0x21a9a50 (199 B): GXNamedResourceTree_EraseSubtreeRecursive
// Post-order traversal freeing right subtree, releasing payload constant buffer
// (`+0xC0`), freeing heap key buffer (`+0x20`), and deallocating node (`0xD0` B).
// ----------------------------------------------------------------------------
void GXNamedResourceTree_EraseSubtreeRecursive(
    GXNamedResourceTreeBase* pTreeBase,
    GXNamedResourceTreeNode* pNode)
{
    while (pNode && pNode->m_isNil == 0) {
        GXNamedResourceTree_EraseSubtreeRecursive(pTreeBase, pNode->m_pRight);
        GXNamedResourceTreeNode* pLeft = pNode->m_pLeft;
        if (pNode->m_payload.m_pColorScaleCBuf) {
            CGBufferGnm_Release(pNode->m_payload.m_pColorScaleCBuf);
            pNode->m_payload.m_pColorScaleCBuf = nullptr;
        }
        if (pNode->m_key.m_capacity20 >= 8) {
            if (pNode->m_key.m_capacity20 + 1 >= 0x21) {
                sub_20b55b0(
                    reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                    0x120,
                    reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
            }
            pNode->m_key.m_inUseFlag69 &= 0xFDu;
        }
        pNode->m_key.m_capacity20 = 7;
        pNode->m_key.m_length18 = 0;
        pNode->m_key.m_data08.m_inlineBuf[0] = L'\0';

        using FreeFn = void (*)(void*, void*);
        void** pAllocVtbl = *reinterpret_cast<void***>(pTreeBase->m_pNodeAlloc18);
        reinterpret_cast<FreeFn>(pAllocVtbl[0x70 / 8])(pTreeBase->m_pNodeAlloc18, pNode);

        pNode = pLeft;
    }
}

// ----------------------------------------------------------------------------
// 0x21a8810 (295 B): GXNamedResourceMap_Destroy
// Iterates all registered resources in-order, decrements their refCount
// (destroying if <= 1), erases the RB-tree, and frees `m_pHead`.
// ----------------------------------------------------------------------------
void GXNamedResourceMap_Destroy(GXNamedResourceMap* pThis) {
    if (!pThis) return;
    pThis->m_vptr   = reinterpret_cast<void**>(GX_VMA(0x535f980ULL));
    pThis->m_vptr08 = reinterpret_cast<void**>(GX_VMA(0x535f9c0ULL));
    if (!pThis->m_pHead) return;
    GXNamedResourceTreeNode* pCur = pThis->m_pHead->m_pLeft;
    while (pCur != pThis->m_pHead) {
        GXResourceBase* pRes = pCur->m_payload.m_pResource;
        if (pRes) {
            int32_t oldRef = *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRes) + 0x08);
            *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRes) + 0x08) = oldRef - 1;
            if (oldRef <= 1) {
                using DtorFn = void (*)(GXResourceBase*);
                void** pResVtbl = *reinterpret_cast<void***>(pRes);
                reinterpret_cast<DtorFn>(pResVtbl[0])(pRes);
            }
        }
        if (pCur->m_isNil != 0) {
            break;
        }
        if (pCur->m_pRight->m_isNil == 0) {
            pCur = pCur->m_pRight;
            while (pCur->m_pLeft->m_isNil == 0) {
                pCur = pCur->m_pLeft;
            }
        } else {
            GXNamedResourceTreeNode* pParent = pCur->m_pParent;
            while (pParent->m_isNil == 0 && pCur == pParent->m_pRight) {
                pCur = pParent;
                pParent = pParent->m_pParent;
            }
            pCur = pParent;
        }
    }
    auto* pTreeBase = reinterpret_cast<GXNamedResourceTreeBase*>(&pThis->m_pTreeAlloc);
    GXNamedResourceTree_EraseSubtreeRecursive(pTreeBase, pThis->m_pHead->m_pParent);
    pThis->m_pHead->m_pParent = pThis->m_pHead;
    pThis->m_pHead->m_pLeft   = pThis->m_pHead;
    pThis->m_pHead->m_pRight  = pThis->m_pHead;
    pThis->m_nodeCount        = 0;
    pThis->m_totalBytes       = 0;
    if (pThis->m_pNodeAlloc) {
        using FreeFn = void (*)(void*, void*);
        void** vtbl = *reinterpret_cast<void***>(pThis->m_pNodeAlloc);
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pThis->m_pNodeAlloc, pThis->m_pHead);
    }
}

// ----------------------------------------------------------------------------
// 0x21a9a00 (72 B): GXNamedResourceMap_DeletingDtor
// Deleting destructor for the 0x28-byte buffer container (vtable 0x535fa10).
// ----------------------------------------------------------------------------
void GXNamedResourceMap_DeletingDtor(GXNamedResourceMap* pThis) {
    auto* pRaw = reinterpret_cast<uint8_t*>(pThis);
    *reinterpret_cast<void***>(pRaw) = reinterpret_cast<void**>(GX_VMA(0x535fa10ULL));
    if (pRaw[0x20] != 0) {
        void* pBuf = *reinterpret_cast<void**>(pRaw + 0x10);
        if (pBuf != nullptr) {
            void* pAlloc = *reinterpret_cast<void**>(pRaw + 0x08);
            void** vtbl = *reinterpret_cast<void***>(pAlloc);
            using FreeFn = void(*)(void*, void*);
            if (pRaw[0x21] != 0) {
                reinterpret_cast<FreeFn>(vtbl[0xA0 / 8])(pAlloc, pBuf);
            } else {
                reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pAlloc, pBuf);
            }
        }
    }
    sub_2bbe588(pRaw);
}

// ----------------------------------------------------------------------------
// 0x21a99a0 (86 B): GXNamedResourceMap_BaseDtor
// Non-deleting destructor for the 0x28-byte buffer container (vtable 0x535fa10).
// ----------------------------------------------------------------------------
void GXNamedResourceMap_BaseDtor(GXNamedResourceMap* pThis) {
    auto* pRaw = reinterpret_cast<uint8_t*>(pThis);
    *reinterpret_cast<void***>(pRaw) = reinterpret_cast<void**>(GX_VMA(0x535fa10ULL));
    if (pRaw[0x20] != 0) {
        void* pBuf = *reinterpret_cast<void**>(pRaw + 0x10);
        if (pBuf != nullptr) {
            void* pAlloc = *reinterpret_cast<void**>(pRaw + 0x08);
            void** vtbl = *reinterpret_cast<void***>(pAlloc);
            using FreeFn = void(*)(void*, void*);
            if (pRaw[0x21] != 0) {
                reinterpret_cast<FreeFn>(vtbl[0xA0 / 8])(pAlloc, pBuf);
            } else {
                reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pAlloc, pBuf);
            }
        }
    }
    *reinterpret_cast<uint16_t*>(pRaw + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(pRaw + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(pRaw + 0x10) = 0;
}

// ----------------------------------------------------------------------------
// 0x21a98f0 (86 B): GXNamedResourceMap_ClearTree
// Non-deleting destructor for the 0x28-byte buffer container (vtable 0x535fa10).
// ----------------------------------------------------------------------------
void GXNamedResourceMap_ClearTree(GXNamedResourceMap* pThis) {
    GXNamedResourceMap_BaseDtor(pThis);
}

// ----------------------------------------------------------------------------
// 0x21a9950 (72 B): GXNamedResourceMap_ClearAndReset
// Deleting destructor for the 0x28-byte buffer container (vtable 0x535fa10).
// ----------------------------------------------------------------------------
void GXNamedResourceMap_ClearAndReset(GXNamedResourceMap* pThis) {
    GXNamedResourceMap_DeletingDtor(pThis);
}

// ----------------------------------------------------------------------------
// 0x21a98e0 (3 B): GXNamedResourceMap_ReturnTrueStub
// ----------------------------------------------------------------------------
bool GXNamedResourceMap_ReturnTrueStub() {
    return false;
}

// ----------------------------------------------------------------------------
// 0x21a9570 (684 B): GXNamedResourceMap_FindByNameCaseInsensitive
// Copies `name` into a stack `DLFixedWString31` (up to 31 chars), performs a
// case-insensitive RB-tree lower-bound search using `sub_2bbec48()`, and
// falls back to `m_pParentMap` (`vtable+0x28`) when not found in `pThis`.
// ----------------------------------------------------------------------------
GXResourceBase* GXNamedResourceMap_FindByNameCaseInsensitive(
    GXNamedResourceMap* pThis,
    const wchar_t* name,
    bool localOnly)
{
    DLFixedWString31 searchKey;
    uint8_t stackScratch[0x41];
    sub_2bbe578(searchKey.m_fixedBuf28, stackScratch, 0x41);
    searchKey.m_inUseFlag69 = 0;
    searchKey.m_capacity20 = 7;
    searchKey.m_length18 = 0;
    searchKey.m_data08.m_inlineBuf[0] = L'\0';
    searchKey.m_bOwnsMemory6A = 1;

    sub_27cbc20(&searchKey, 0x1Fu, 0u);
    searchKey.m_length18 = 0;
    *GXFixedWString_GetMutablePtr(&searchKey) = L'\0';

    size_t rawLen = sub_2bbf228(name);
    uint64_t copyLen = (rawLen < 0x1Fu) ? rawLen : 0x1Fu;
    sub_27cba30(&searchKey, name, copyLen);

    GXNamedResourceTreeNode* pHead = pThis->m_pHead;
    GXNamedResourceTreeNode* pBest = pHead;
    GXNamedResourceTreeNode* pCur  = pHead->m_pParent;

    while (pCur->m_isNil == 0) {
        const wchar_t* pNodeStr = GXFixedWString_GetPtr(&pCur->m_key);
        const wchar_t* pKeyStr  = GXFixedWString_GetPtr(&searchKey);
        if (GXCompareWideStringsCaseInsensitive(pNodeStr, pKeyStr) < 0) {
            pCur = pCur->m_pRight;
        } else {
            pBest = pCur;
            pCur  = pCur->m_pLeft;
        }
    }

    if (pBest != pThis->m_pHead) {
        const wchar_t* pKeyStr  = GXFixedWString_GetPtr(&searchKey);
        const wchar_t* pBestStr = GXFixedWString_GetPtr(&pBest->m_key);
        if (GXCompareWideStringsCaseInsensitive(pKeyStr, pBestStr) < 0) {
            pBest = pThis->m_pHead;
        }
    }

    if (searchKey.m_capacity20 >= 8) {
        if (searchKey.m_capacity20 + 1 >= 0x21) {
            sub_20b55b0(
                reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        searchKey.m_inUseFlag69 &= 0xFDu;
    }
    searchKey.m_capacity20 = 7;
    searchKey.m_length18 = 0;
    searchKey.m_data08.m_inlineBuf[0] = L'\0';

    if (pBest != pThis->m_pHead) {
        return pBest->m_payload.m_pResource;
    }

    if (!localOnly && pThis->m_pParentMap) {
        using FindFn = GXResourceBase*(*)(GXNamedResourceMap*, const wchar_t*);
        return reinterpret_cast<FindFn>(pThis->m_pParentMap->m_vptr[0x28 / 8])(
            pThis->m_pParentMap, name);
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// 0x21a9820 (9 B): GXNamedResourceMap_FindByNameThunk
// Non-virtual thunk adjusting `this` by -8 (secondary vtable 0x535f9c0).
// ----------------------------------------------------------------------------
GXResourceBase* GXNamedResourceMap_FindByNameThunk(
    GXNamedResourceMap* pThis,
    const wchar_t* name)
{
    auto* pAdjusted = reinterpret_cast<GXNamedResourceMap*>(reinterpret_cast<uint8_t*>(pThis) - 8);
    return GXNamedResourceMap_FindByNameCaseInsensitive(pAdjusted, name, false);
}

// ----------------------------------------------------------------------------
// 0x21a8940 (9 B): GXNamedResourceMap_FindByNameLocalOnlyThunk
// Non-virtual thunk adjusting `this` by -8 and jumping to `0x21a8810`.
// ----------------------------------------------------------------------------
GXResourceBase* GXNamedResourceMap_FindByNameLocalOnlyThunk(
    GXNamedResourceMap* pThis,
    const wchar_t* /*name*/)
{
    auto* pAdjusted = reinterpret_cast<GXNamedResourceMap*>(reinterpret_cast<uint8_t*>(pThis) - 8);
    GXNamedResourceMap_Destroy(pAdjusted);
    return nullptr;
}

// ----------------------------------------------------------------------------
// 0x21a8da0 (1986 B): GXNamedResourceMap_UnregisterAndEraseByName
// Searches the RB-tree for `name` (case-insensitive), releases the resource's
// byte-size contribution and reference count, splices and rebalances the
// red-black tree, frees the node and its color-scale constant buffer, and
// decrements `m_nodeCount`.
// ----------------------------------------------------------------------------
bool GXNamedResourceMap_UnregisterAndEraseByName(
    GXNamedResourceMap* pThis,
    const wchar_t* name)
{
    DLFixedWString31 searchKey;
    uint8_t stackScratch[0x41];
    sub_2bbe578(searchKey.m_fixedBuf28, stackScratch, 0x41);
    searchKey.m_inUseFlag69 = 0;
    searchKey.m_capacity20 = 7;
    searchKey.m_length18 = 0;
    searchKey.m_data08.m_inlineBuf[0] = L'\0';
    searchKey.m_bOwnsMemory6A = 1;

    sub_27cbc20(&searchKey, 0x1Fu, 0u);
    searchKey.m_length18 = 0;
    *GXFixedWString_GetMutablePtr(&searchKey) = L'\0';

    size_t rawLen = sub_2bbf228(name);
    uint64_t copyLen = (rawLen < 0x1Fu) ? rawLen : 0x1Fu;
    sub_27cba30(&searchKey, name, copyLen);

    GXNamedResourceTreeNode* pHead = pThis->m_pHead;
    GXNamedResourceTreeNode* pTarget = pHead;
    GXNamedResourceTreeNode* pCur = pHead->m_pParent;

    while (pCur->m_isNil == 0) {
        const wchar_t* pNodeStr = GXFixedWString_GetPtr(&pCur->m_key);
        const wchar_t* pKeyStr  = GXFixedWString_GetPtr(&searchKey);
        if (GXCompareWideStringsCaseInsensitive(pNodeStr, pKeyStr) < 0) {
            pCur = pCur->m_pRight;
        } else {
            pTarget = pCur;
            pCur    = pCur->m_pLeft;
        }
    }

    if (pTarget != pThis->m_pHead) {
        const wchar_t* pKeyStr    = GXFixedWString_GetPtr(&searchKey);
        const wchar_t* pTargetStr = GXFixedWString_GetPtr(&pTarget->m_key);
        if (GXCompareWideStringsCaseInsensitive(pKeyStr, pTargetStr) < 0) {
            pTarget = pThis->m_pHead;
        }
    }

    if (searchKey.m_capacity20 >= 8) {
        if (searchKey.m_capacity20 + 1 >= 0x21) {
            sub_20b55b0(
                reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        searchKey.m_inUseFlag69 &= 0xFDu;
    }
    searchKey.m_capacity20 = 7;
    searchKey.m_length18 = 0;
    searchKey.m_data08.m_inlineBuf[0] = L'\0';

    if (pTarget == pThis->m_pHead) {
        return false;
    }

    // Release payload resource refCount and subtract byte size
    GXResourceBase* pRes = pTarget->m_payload.m_pResource;
    if (pRes) {
        using GetByteSizeFn = uint32_t (*)(GXResourceBase*);
        void** pResVtbl = *reinterpret_cast<void***>(pRes);
        uint32_t resBytes = reinterpret_cast<GetByteSizeFn>(pResVtbl[0x68 / 8])(pRes);
        pThis->m_totalBytes -= resBytes;

        int32_t oldRef = *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRes) + 0x08);
        *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRes) + 0x08) = oldRef - 1;
        if (oldRef <= 1) {
            using DtorFn = void (*)(GXResourceBase*);
            pResVtbl = *reinterpret_cast<void***>(pRes);
            reinterpret_cast<DtorFn>(pResVtbl[0])(pRes);
        }
    }

    if (pTarget->m_isNil != 0) {
        sub_2bbf1a8(reinterpret_cast<const char*>(GX_VMA(0x4940315ULL)));
    }

    // Find RB-tree splice node `pSplice` and fixup child `pFixupChild`
    GXNamedResourceTreeNode* pRight = pTarget->m_pRight;
    uint8_t rightIsNil = pRight->m_isNil;
    GXNamedResourceTreeNode* pSplice;
    if (rightIsNil != 0) {
        GXNamedResourceTreeNode* pWalk = pTarget;
        for (;;) {
            pSplice = pWalk->m_pParent;
            if (pSplice->m_isNil != 0 || pWalk != pSplice->m_pRight) {
                break;
            }
            pWalk = pSplice;
        }
    } else {
        GXNamedResourceTreeNode* pWalk = pRight;
        do {
            pSplice = pWalk;
            pWalk = pSplice->m_pLeft;
        } while (pWalk->m_isNil == 0);
        rightIsNil = 0;
    }

    GXNamedResourceTreeNode* pLeft = pTarget->m_pLeft;
    GXNamedResourceTreeNode* pFixupChild;
    GXNamedResourceTreeNode* pFixupParent;

    if (pLeft->m_isNil != 0) {
        pFixupChild = pRight;
        goto splice_single_child;
    }
    if (rightIsNil != 0) {
        pFixupChild = pLeft;
        goto splice_single_child;
    }

    // Two non-nil children: replace `pTarget` in-place with in-order successor `pSplice`
    pFixupChild = pSplice->m_pRight;
    if (pSplice == pTarget) {
        goto splice_single_child;
    }

    pLeft->m_pParent = pSplice;
    pSplice->m_pLeft = pTarget->m_pLeft;
    if (pSplice == pTarget->m_pRight) {
        pFixupParent = pSplice;
    } else {
        pFixupParent = pSplice->m_pParent;
        if (pFixupChild->m_isNil == 0) {
            pFixupChild->m_pParent = pFixupParent;
        }
        pFixupParent->m_pLeft = pFixupChild;
        pSplice->m_pRight = pTarget->m_pRight;
        pTarget->m_pRight->m_pParent = pSplice;
    }

    if (pThis->m_pHead->m_pParent == pTarget) {
        pThis->m_pHead->m_pParent = pSplice;
    } else if (pTarget->m_pParent->m_pLeft == pTarget) {
        pTarget->m_pParent->m_pLeft = pSplice;
    } else {
        pTarget->m_pParent->m_pRight = pSplice;
    }
    pSplice->m_pParent = pTarget->m_pParent;
    {
        uint8_t tmpColor = pSplice->m_color;
        pSplice->m_color = pTarget->m_color;
        pTarget->m_color = tmpColor;
    }
    goto check_rebalance;

splice_single_child:
    pFixupParent = pTarget->m_pParent;
    if (pFixupChild->m_isNil == 0) {
        pFixupChild->m_pParent = pFixupParent;
    }
    if (pThis->m_pHead->m_pParent == pTarget) {
        pThis->m_pHead->m_pParent = pFixupChild;
    } else if (pFixupParent->m_pLeft == pTarget) {
        pFixupParent->m_pLeft = pFixupChild;
    } else {
        pFixupParent->m_pRight = pFixupChild;
    }

    if (pThis->m_pHead->m_pLeft == pTarget) {
        GXNamedResourceTreeNode* pNewMin = pFixupParent;
        if (pFixupChild->m_isNil == 0) {
            GXNamedResourceTreeNode* pW = pFixupChild;
            do {
                pNewMin = pW;
                pW = pNewMin->m_pLeft;
            } while (pW->m_isNil == 0);
        }
        pThis->m_pHead->m_pLeft = pNewMin;
    }

    if (pThis->m_pHead->m_pRight == pTarget) {
        GXNamedResourceTreeNode* pNewMax = pFixupParent;
        if (pFixupChild->m_isNil == 0) {
            GXNamedResourceTreeNode* pW = pFixupChild;
            do {
                pNewMax = pW;
                pW = pNewMax->m_pRight;
            } while (pW->m_isNil == 0);
        }
        pThis->m_pHead->m_pRight = pNewMax;
    }

check_rebalance:
    // Red-Black Tree Delete Rebalance
    if (pTarget->m_color == 1) {
        while (pFixupChild != pThis->m_pHead->m_pParent && pFixupChild->m_color == 1) {
            if (pFixupChild == pFixupParent->m_pLeft) {
                GXNamedResourceTreeNode* pSibling = pFixupParent->m_pRight;
                if (pSibling->m_color == 0) {
                    pSibling->m_color = 1;
                    pFixupParent->m_color = 0;
                    GXRBTree_RotateLeft(pThis->m_pHead, pFixupParent);
                    pSibling = pFixupParent->m_pRight;
                }
                if (pSibling->m_isNil != 0) {
                    pFixupChild = pFixupParent;
                    pFixupParent = pFixupChild->m_pParent;
                } else {
                    GXNamedResourceTreeNode* pSibLeft = pSibling->m_pLeft;
                    GXNamedResourceTreeNode* pSibRight = pSibling->m_pRight;
                    if (pSibLeft->m_color == 1 && pSibRight->m_color == 1) {
                        pSibling->m_color = 0;
                        pFixupChild = pFixupParent;
                        pFixupParent = pFixupChild->m_pParent;
                    } else {
                        if (pSibRight->m_color == 1) {
                            pSibLeft->m_color = 1;
                            pSibling->m_color = 0;
                            GXRBTree_RotateRight(pThis->m_pHead, pSibling);
                            pSibling = pFixupParent->m_pRight;
                        }
                        pSibling->m_color = pFixupParent->m_color;
                        pFixupParent->m_color = 1;
                        pSibling->m_pRight->m_color = 1;
                        GXRBTree_RotateLeft(pThis->m_pHead, pFixupParent);
                        break;
                    }
                }
            } else {
                GXNamedResourceTreeNode* pSibling = pFixupParent->m_pLeft;
                if (pSibling->m_color == 0) {
                    pSibling->m_color = 1;
                    pFixupParent->m_color = 0;
                    GXRBTree_RotateRight(pThis->m_pHead, pFixupParent);
                    pSibling = pFixupParent->m_pLeft;
                }
                if (pSibling->m_isNil != 0) {
                    pFixupChild = pFixupParent;
                    pFixupParent = pFixupChild->m_pParent;
                } else {
                    GXNamedResourceTreeNode* pSibLeft = pSibling->m_pLeft;
                    GXNamedResourceTreeNode* pSibRight = pSibling->m_pRight;
                    if (pSibRight->m_color == 1 && pSibLeft->m_color == 1) {
                        pSibling->m_color = 0;
                        pFixupChild = pFixupParent;
                        pFixupParent = pFixupChild->m_pParent;
                    } else {
                        if (pSibLeft->m_color == 1) {
                            pSibRight->m_color = 1;
                            pSibling->m_color = 0;
                            GXRBTree_RotateLeft(pThis->m_pHead, pSibling);
                            pSibling = pFixupParent->m_pLeft;
                        }
                        pSibling->m_color = pFixupParent->m_color;
                        pFixupParent->m_color = 1;
                        pSibling->m_pLeft->m_color = 1;
                        GXRBTree_RotateRight(pThis->m_pHead, pFixupParent);
                        break;
                    }
                }
            }
        }
        pFixupChild->m_color = 1;
    }

    // Destroy node payload, key, and deallocate node
    if (pTarget->m_payload.m_pColorScaleCBuf) {
        CGBufferGnm_Release(pTarget->m_payload.m_pColorScaleCBuf);
        pTarget->m_payload.m_pColorScaleCBuf = nullptr;
    }
    if (pTarget->m_key.m_capacity20 >= 8) {
        if (pTarget->m_key.m_capacity20 + 1 >= 0x21) {
            sub_20b55b0(
                reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        pTarget->m_key.m_inUseFlag69 &= 0xFDu;
    }
    pTarget->m_key.m_capacity20 = 7;
    pTarget->m_key.m_length18 = 0;
    pTarget->m_key.m_data08.m_inlineBuf[0] = L'\0';

    using FreeFn = void (*)(void*, void*);
    void** pAllocVtbl = *reinterpret_cast<void***>(pThis->m_pNodeAlloc);
    reinterpret_cast<FreeFn>(pAllocVtbl[0x70 / 8])(pThis->m_pNodeAlloc, pTarget);

    if (pThis->m_nodeCount != 0) {
        pThis->m_nodeCount--;
    }
    return true;
}

// ----------------------------------------------------------------------------
// 0x21a98d0 (5 B): GXNamedResourceMap_UnregisterByNameThunk
// ----------------------------------------------------------------------------
bool GXNamedResourceMap_UnregisterByNameThunk(
    GXNamedResourceMap* pThis,
    const wchar_t* name)
{
    return GXNamedResourceMap_UnregisterAndEraseByName(pThis, name);
}

// ----------------------------------------------------------------------------
// 0x21a8950 (1102 B): GXNamedResourceMap_RegisterResourceByName
// Constructs a `GXNamedResourceKeyValuePair` on the stack, searches the
// RB-tree for the insertion position and checks for duplicates using
// case-insensitive comparison (`sub_2bbec48()`), inserts and rebalances via
// `GXNamedResourceTree_InsertNode` (`0x21a9b20`), and updates `m_totalBytes`
// and `pResource->m_refCount`.
// ----------------------------------------------------------------------------
bool GXNamedResourceMap_RegisterResourceByName(
    GXNamedResourceMap* pThis,
    const wchar_t* name,
    GXResourceBase* pResource)
{
    if (!pResource) {
        return false;
    }

    GXNamedResourceKeyValuePair kvPair;
    uint8_t stackScratch[0x41];
    sub_2bbe578(kvPair.m_key00.m_fixedBuf28, stackScratch, 0x41);
    kvPair.m_key00.m_inUseFlag69 = 0;
    kvPair.m_key00.m_capacity20 = 7;
    kvPair.m_key00.m_length18 = 0;
    kvPair.m_key00.m_data08.m_inlineBuf[0] = L'\0';
    kvPair.m_key00.m_bOwnsMemory6A = 1;

    sub_27cbc20(&kvPair.m_key00, 0x1Fu, 0u);
    kvPair.m_key00.m_length18 = 0;
    *GXFixedWString_GetMutablePtr(&kvPair.m_key00) = L'\0';

    auto* pTreeBase = reinterpret_cast<GXNamedResourceTreeBase*>(&pThis->m_pTreeAlloc);

    size_t rawLen = sub_2bbf228(name);
    uint64_t copyLen = (rawLen < 0x1Fu) ? rawLen : 0x1Fu;
    sub_27cba30(&kvPair.m_key00, name, copyLen);

    kvPair.m_payload70.m_pResource = pResource;
    kvPair.m_payload70.m_flags = 0;
    kvPair.m_payload70.m_pad0C = 0;
    kvPair.m_payload70.m_colorScale = 1.0f;
    kvPair.m_payload70.m_unk14_lo = 0;
    kvPair.m_payload70.m_unk14_hi = 0;
    kvPair.m_payload70.m_unk1C = 0;
    const uint64_t* pDefaultUid = reinterpret_cast<const uint64_t*>(GX_VMA(0x4920860ULL));
    kvPair.m_payload70.m_uidLow = pDefaultUid[0];
    kvPair.m_payload70.m_uidHigh = pDefaultUid[1];
    kvPair.m_payload70.m_pColorScaleCBuf = nullptr;
    kvPair.m_payload70.m_reserved38 = 0;

    GXNamedResourceTreeNode* pHead = pThis->m_pHead;
    GXNamedResourceTreeNode* pCur = pHead->m_pParent;
    GXNamedResourceTreeNode* pWhere = pHead;
    int32_t lastCmp = -1;

    while (pCur->m_isNil == 0) {
        pWhere = pCur;
        const wchar_t* pKeyStr  = GXFixedWString_GetPtr(&kvPair.m_key00);
        const wchar_t* pNodeStr = GXFixedWString_GetPtr(&pCur->m_key);
        lastCmp = GXCompareWideStringsCaseInsensitive(pKeyStr, pNodeStr);
        pCur = (lastCmp < 0) ? pCur->m_pLeft : pCur->m_pRight;
    }

    GXNamedResourceTreeNode* pPred = pWhere;
    bool insertLeft = true;
    bool bInserted = false;
    GXNamedResourceTreeIter insIter = {nullptr, nullptr, nullptr};

    if (lastCmp >= 0) {
        insertLeft = false;
        goto check_pred;
    }
    if (pWhere == pHead->m_pLeft) {
        GXNamedResourceTree_InsertNode(&insIter, pTreeBase, true, pWhere, &kvPair);
        bInserted = true;
        goto cleanup_kv;
    }
    if (pWhere->m_isNil != 0) {
        pPred = pWhere->m_pRight;
    } else if (pWhere->m_pLeft->m_isNil == 0) {
        GXNamedResourceTreeNode* pW = pWhere->m_pLeft;
        do {
            pPred = pW;
            pW = pPred->m_pRight;
        } while (pW->m_isNil == 0);
    } else {
        GXNamedResourceTreeNode* pW = pWhere;
        GXNamedResourceTreeNode* pP = pWhere->m_pParent;
        while (pP->m_isNil == 0 && pW == pP->m_pLeft) {
            pW = pP;
            pP = pP->m_pParent;
        }
        if (pW->m_isNil == 0) {
            pPred = pP;
        }
    }

check_pred:
    {
        const wchar_t* pPredStr = GXFixedWString_GetPtr(&pPred->m_key);
        const wchar_t* pKeyStr  = GXFixedWString_GetPtr(&kvPair.m_key00);
        if (GXCompareWideStringsCaseInsensitive(pPredStr, pKeyStr) < 0) {
            GXNamedResourceTree_InsertNode(&insIter, pTreeBase, insertLeft, pWhere, &kvPair);
            bInserted = true;
        } else {
            bInserted = false;
        }
    }

cleanup_kv:
    if (kvPair.m_payload70.m_pColorScaleCBuf) {
        CGBufferGnm_Release(kvPair.m_payload70.m_pColorScaleCBuf);
        kvPair.m_payload70.m_pColorScaleCBuf = nullptr;
    }
    if (kvPair.m_key00.m_capacity20 >= 8) {
        if (kvPair.m_key00.m_capacity20 + 1 >= 0x21) {
            sub_20b55b0(
                reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        kvPair.m_key00.m_inUseFlag69 &= 0xFDu;
    }
    kvPair.m_key00.m_capacity20 = 7;
    kvPair.m_key00.m_length18 = 0;
    kvPair.m_key00.m_data08.m_inlineBuf[0] = L'\0';

    if (bInserted) {
        using GetByteSizeFn = uint32_t (*)(GXResourceBase*);
        void** pResVtbl = *reinterpret_cast<void***>(pResource);
        uint32_t resBytes = reinterpret_cast<GetByteSizeFn>(pResVtbl[0x68 / 8])(pResource);
        pThis->m_totalBytes += resBytes;
        (*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pResource) + 0x08))++;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// 0x21a98c0 (16 B): GXNamedResourceMap_RegisterByNameThunk
// ----------------------------------------------------------------------------
bool GXNamedResourceMap_RegisterByNameThunk(
    GXNamedResourceMap* pThis,
    const wchar_t* name,
    GXResourceBase* pResource)
{
    return GXNamedResourceMap_RegisterResourceByName(pThis, name, pResource);
}

// ----------------------------------------------------------------------------
// 0x21a87e0 (35 B): GXNamedResourceMap_GetTotalByteSize
// Deleting destructor thunk for secondary vtable (`this - 8`).
// ----------------------------------------------------------------------------
uint32_t GXNamedResourceMap_GetTotalByteSize(const GXNamedResourceMap* pThis) {
    auto* pAdjusted = reinterpret_cast<GXNamedResourceMap*>(
        const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pThis)) - 8);
    GXNamedResourceMap_Destroy(pAdjusted);
    sub_2bbe588(pAdjusted);
    return 0;
}

// ----------------------------------------------------------------------------
// 0x21a87c0 (28 B): GXNamedResourceMap_GetTotalResourceCount
// Deleting destructor for primary vtable (`this`).
// ----------------------------------------------------------------------------
uint32_t GXNamedResourceMap_GetTotalResourceCount(const GXNamedResourceMap* pThis) {
    auto* pMut = const_cast<GXNamedResourceMap*>(pThis);
    GXNamedResourceMap_Destroy(pMut);
    sub_2bbe588(pMut);
    return 0;
}

// ----------------------------------------------------------------------------
// 0x21a8680 (309 B): GXNamedResourceMap_Construct
// ----------------------------------------------------------------------------
void GXNamedResourceMap_Construct(
    GXNamedResourceMap* pThis,
    void* pAlloc,
    uint64_t unk48,
    uint64_t /*unused4*/)
{
    pThis->m_vptr   = reinterpret_cast<void**>(GX_VMA(0x535f980ULL));
    pThis->m_vptr08 = reinterpret_cast<void**>(GX_VMA(0x535f9c0ULL));

    void** pAllocVtbl = *reinterpret_cast<void***>(pAlloc);
    using GetCapsFn = void (*)(uint64_t*, void*, uint32_t);
    uint64_t allocCaps = 0;
    reinterpret_cast<GetCapsFn>(pAllocVtbl[0x20 / 8])(&allocCaps, pAlloc, 0u);
    if ((allocCaps & 0x20ULL) == 0ULL) {
        sub_20b55b0(nullptr, 0x3Eu, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }

    pThis->m_nodeCount  = 0;
    pThis->m_pHead      = nullptr;
    pThis->m_pNodeAlloc = pAlloc;

    using AllocAlignedFn = void* (*)(void*, size_t, size_t);
    pAllocVtbl = *reinterpret_cast<void***>(pAlloc);
    auto* pHead = static_cast<GXNamedResourceTreeNode*>(
        reinterpret_cast<AllocAlignedFn>(pAllocVtbl[0x58 / 8])(pAlloc, 0xD0u, 0x10u));
    if (pHead) {
        pHead->m_pLeft = pHead;
    } else {
        int32_t* pOomMode = reinterpret_cast<int32_t*>(gx_guest_tls_slot(0x53e4318ULL));
        if (*pOomMode != 0) {
            sub_2bbe978(gx_guest_tls_slot(0x53e4320ULL), 1u);
            return;
        }
        sub_20b55b0(nullptr, 0x109u, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }
    if (pHead) {
        pHead->m_pParent = pHead;
        pHead->m_pRight  = pHead;
        pHead->m_color   = 1;
        pHead->m_isNil   = 1;
    }
    pThis->m_pHead      = pHead;
    pThis->m_totalBytes = 0;
    pThis->m_pParentMap = nullptr;
    pThis->m_unk40      = reinterpret_cast<uint64_t>(pAlloc);
    pThis->m_unk48      = unk48;
}


// ============================================================================
// 2. GXTextureColorScaleCBuffer — HDR Texture Color-Scale Constant Buffer
//    (0x21a8250 .. 0x21a85e0)
// ============================================================================

struct DLLinearHeapTls {
    uint8_t* m_pBegin00; // +0x00
    uint8_t* m_pLimit08; // +0x08
    uint8_t* m_pCur10;   // +0x10
    uint8_t* m_pEnd18;   // +0x18
};

struct GXTextureColorScaleCBuffer {
    float    m_colorScale;        // +0x00 (resource + 0x10)
    uint32_t m_pad04[3];          // +0x04
    float    m_rgbaColorScale[4]; // +0x10 (resource + 0x20, init from 0x4920820..0x4920850 = {1,1,1,1})
    void*    m_pConstantBuffer;   // +0x20 (resource + 0x30)
};

// 0x21a85e0 (5 B): GXTextureColorScale_GetConstantBuffer
void* GXTextureColorScale_GetConstantBuffer(const GXTextureColorScaleCBuffer* pThis) {
    return pThis->m_pConstantBuffer;
}

// 0x21a85d0 (7 B): GXTextureColorScale_ResetToOne
void GXTextureColorScale_ResetToOne(GXTextureColorScaleCBuffer* pThis) {
    pThis->m_colorScale = 1.0f;
}

// 0x21a8350 (420 B): GXTextureColorScale_CreateConstantBuffer
void GXTextureColorScale_CreateConstantBuffer(
    GXTextureColorScaleCBuffer* pThis,
    const float rgbaScale[4])
{
    alignas(16) uint32_t cbDesc[8] = {};
    cbDesc[0] = 0x10u; // 16-byte float4 constant buffer
    cbDesc[2] = 0x04u;
    alignas(16) const void* initData[2] = { nullptr, nullptr };

    auto getTlsLinearHeap = []() -> DLLinearHeapTls* {
        auto** ppHeap = reinterpret_cast<DLLinearHeapTls**>(gx_guest_tls_slot(0x53e4568ULL));
        DLLinearHeapTls* pHeap = *ppHeap;
        if (!pHeap) {
            sub_207d250();
            ppHeap = reinterpret_cast<DLLinearHeapTls**>(gx_guest_tls_slot(0x53e4568ULL));
            pHeap = *ppHeap;
        }
        return pHeap;
    };

    DLLinearHeapTls* pHeap = getTlsLinearHeap();
    uint8_t* pMarker = pHeap->m_pCur10;

    if (!pHeap) {
        pHeap = getTlsLinearHeap();
    }
    uint8_t* pCur = pHeap->m_pCur10;
    uintptr_t alignPad = (-reinterpret_cast<uintptr_t>(pCur)) & 0xFULL;
    uint8_t* pNext = pCur + alignPad + 0x10u;
    if (pNext <= pHeap->m_pEnd18) {
        pHeap->m_pCur10 = pNext;
        uint8_t* pScratch = pCur + alignPad;
        if (pScratch != nullptr) {
            std::memcpy(pScratch, rgbaScale, 16u);
            initData[0] = pScratch;
            void* pGlobal = *reinterpret_cast<void**>(GX_VMA(0x55406c8ULL));
            void* pDev = *reinterpret_cast<void**>(pGlobal);
            GXDevice_CreateConstantBuffer(
                pDev, &pThis->m_pConstantBuffer, cbDesc, initData, nullptr, nullptr, nullptr);
            pHeap = *reinterpret_cast<DLLinearHeapTls**>(gx_guest_tls_slot(0x53e4568ULL));
        }
    }

    if (!pHeap) {
        pHeap = getTlsLinearHeap();
    }
    if (pHeap->m_pBegin00 > pMarker || pHeap->m_pLimit08 < pMarker) {
        sub_20b55b0(
            reinterpret_cast<const void*>(GX_VMA(0x493c62dULL)),
            0x233u,
            reinterpret_cast<const void*>(GX_VMA(0x493c690ULL)));
    }
    if (pHeap->m_pCur10 > pMarker) {
        pHeap->m_pCur10 = pMarker;
    }
}

// 0x21a8500 (207 B): GXTextureColorScale_InitFromGnfUserData
bool GXTextureColorScale_InitFromGnfUserData(
    GXTextureColorScaleCBuffer* pThis,
    const void* pGnfData,
    uint32_t gnfSize)
{
    if (!pGnfData) return false;
    int count = GnfUserData_GetChunkCount(pGnfData, gnfSize);
    for (int i = 0; i < count; ++i) {
        uint32_t chunkSize = 0;
        uint32_t chunkTag  = 0;
        const float* pFloats = GnfUserData_GetChunkByIndex(pGnfData, gnfSize, i, &chunkSize, &chunkTag);
        if (chunkTag == 0x3FCu) {
            alignas(16) float rgba[4] = { pFloats[0], pFloats[1], pFloats[2], 1.0f };
            GXTextureColorScale_CreateConstantBuffer(pThis, rgba);
            return true;
        }
    }
    return false;
}

// 0x21a82a0 (175 B): GXTextureColorScale_InitFromDdsTailChunk
bool GXTextureColorScale_InitFromDdsTailChunk(
    GXTextureColorScaleCBuffer* pThis,
    void* pDdsContainer)
{
    void** vtbl = *reinterpret_cast<void***>(pDdsContainer);
    using GetCountFn = uint32_t(*)(void*);
    using GetChunkFn = void(*)(void*, uint32_t, const float**, uint32_t*);
    uint32_t count = reinterpret_cast<GetCountFn>(vtbl[0x40 / 8])(pDdsContainer);
    if (count == 0) return false;

    const float* pChunkData = nullptr;
    uint32_t chunkSize = 0;
    reinterpret_cast<GetChunkFn>(vtbl[0x38 / 8])(pDdsContainer, count - 1, &pChunkData, &chunkSize);
    if (chunkSize == 4u) {
        float s = *pChunkData;
        pThis->m_colorScale = s;
        alignas(16) float rgba[4] = { s, s, s, 1.0f };
        GXTextureColorScale_CreateConstantBuffer(pThis, rgba);
    }
    return true;
}

// 0x21a8270 (38 B): GXTextureColorScale_Destroy
void GXTextureColorScale_Destroy(GXTextureColorScaleCBuffer* pThis) {
    if (pThis->m_pConstantBuffer) {
        CGBufferGnm_Release(pThis->m_pConstantBuffer);
        pThis->m_pConstantBuffer = nullptr;
    }
}

// 0x21a8250 (28 B): GXTextureColorScale_Construct
void GXTextureColorScale_Construct(GXTextureColorScaleCBuffer* pThis) {
    pThis->m_colorScale        = 1.0f;
    pThis->m_rgbaColorScale[0] = 1.0f;
    pThis->m_rgbaColorScale[1] = 1.0f;
    pThis->m_rgbaColorScale[2] = 1.0f;
    pThis->m_rgbaColorScale[3] = 1.0f;
    pThis->m_pConstantBuffer   = nullptr;
}


// ============================================================================
// 3. GXResourceBase, GXCubeTextureResource & GXTexture2DResource
//    (0x21a68b0 .. 0x21a8140)
// ============================================================================

struct CGTextureGnmRef {
    void**   m_vptr;      // +0x00
    void*    m_pAllocator;// +0x08
    int32_t  m_refCount;  // +0x10
    uint8_t  m_pad14[0x18];
    uint32_t m_byteSize;  // +0x2C
    uint8_t  m_pad30[0x1A];
    uint8_t  m_mipLevels; // +0x4A
    uint8_t  m_dxgiFormat;// +0x4B
};

struct GXResourceBase {
    void**                     m_vptr;       // +0x00 (0x53b6b00)
    uint32_t                   m_refCount;   // +0x08
    uint32_t                   m_pad0C;      // +0x0C
    GXTextureColorScaleCBuffer m_colorScale; // +0x10 .. +0x37
    uint64_t                   m_pad38;      // +0x38
};

struct GXTextureResource : GXResourceBase {
    CGTextureGnmRef* m_pGnmTexture; // +0x40
    void*            m_pSrv;        // +0x48
    void*            m_pUav;        // +0x50
    uint32_t         m_byteSize;    // +0x58
    uint32_t         m_pad5C;       // +0x5C
};

static inline void CGTextureGnmRef_ReleaseRaw(CGTextureGnmRef* pTex) {
    if (!pTex) return;
    if (__sync_fetch_and_add(&pTex->m_refCount, -1) == 1) {
        void* pAlloc = pTex->m_pAllocator;
        using DtorFn = void(*)(void*);
        using FreeFn = void(*)(void*, void*);
        reinterpret_cast<DtorFn>(pTex->m_vptr[0])(pTex);
        void** allocVtbl = *reinterpret_cast<void***>(pAlloc);
        reinterpret_cast<FreeFn>(allocVtbl[0x70 / 8])(pAlloc, pTex);
    }
}

static inline void CGTextureGnmRef_Release(CGTextureGnmRef*& pTex) {
    CGTextureGnmRef* pOld = pTex;
    if (!pOld) return;
    CGTextureGnmRef_ReleaseRaw(pOld);
    pTex = nullptr;
}

static inline void CGTextureGnmRef_Assign(CGTextureGnmRef*& pDst, CGTextureGnmRef* pSrc) {
    if (pDst != pSrc) {
        if (pDst) {
            CGTextureGnmRef_Release(pDst);
        }
        pDst = pSrc;
        if (pSrc) {
            __sync_fetch_and_add(&pSrc->m_refCount, 1);
        }
    }
}

static inline void CGViewGnm_DestroyAndFree(void*& pView) {
    if (!pView) return;
    void* pAlloc = CGObject_GetCreatorAllocator(pView);
    void** viewVtbl = *reinterpret_cast<void***>(pView);
    using DtorFn = void(*)(void*);
    using FreeFn = void(*)(void*, void*);
    reinterpret_cast<DtorFn>(viewVtbl[0])(pView);
    void** allocVtbl = *reinterpret_cast<void***>(pAlloc);
    reinterpret_cast<FreeFn>(allocVtbl[0x70 / 8])(pAlloc, pView);
    pView = nullptr;
}

static inline void GXResourceMemoryTracker_UnregisterTextureBucket(
    GXTextureResource* pThis,
    size_t vecBeginOffset,
    size_t vecEndOffset,
    size_t countOffset,
    size_t bytesOffset,
    size_t mibOffset)
{
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (!pGlobal) return;
    uint8_t** ppTracker = *reinterpret_cast<uint8_t***>(pGlobal + 0x650);
    if (!ppTracker) return;
    uint8_t* pTracker = *ppTracker;
    if (!pTracker) return;

    auto** pBeginSlot = reinterpret_cast<GXTextureResource***>(pTracker + vecBeginOffset);
    auto** pEndSlot   = reinterpret_cast<GXTextureResource***>(pTracker + vecEndOffset);
    GXTextureResource** it  = *pBeginSlot;
    GXTextureResource** end = *pEndSlot;
    while (it != end) {
        if (*it == pThis) {
            size_t tailBytes = static_cast<size_t>(
                reinterpret_cast<uint8_t*>(end) - reinterpret_cast<uint8_t*>(it + 1));
            if (tailBytes != 0) {
                sub_2bbe658(it, it + 1, tailBytes);
            }
            *pEndSlot = reinterpret_cast<GXTextureResource**>(
                reinterpret_cast<uint8_t*>(*pEndSlot) - sizeof(void*));
            using GetByteSizeFn = uint32_t(*)(GXTextureResource*);
            uint32_t texBytes = reinterpret_cast<GetByteSizeFn>(pThis->m_vptr[0x60 / 8])(pThis);
            uint32_t* pTotalBytes = reinterpret_cast<uint32_t*>(pTracker + bytesOffset);
            float*    pTotalMiB   = reinterpret_cast<float*>(pTracker + mibOffset);
            uint32_t* pCount      = reinterpret_cast<uint32_t*>(pTracker + countOffset);
            uint32_t newBytes = *pTotalBytes - texBytes;
            *pTotalBytes = newBytes;
            *pTotalMiB   = static_cast<float>(static_cast<uint64_t>(newBytes)) * (1.0f / 1048576.0f);
            *pCount -= 1u;
            break;
        }
        ++it;
    }
}

// 98-entry DXGI format -> parent typeless DXGI format table (jump table @ 0x21a70a0)
static inline uint32_t GXDxgiFormat_MakeTypeless(uint32_t fmt) {
    switch (fmt) {
        case 2: case 3: case 4:                         return 0x01u; // R32G32B32A32_TYPELESS
        case 6: case 7: case 8:                         return 0x05u; // R32G32B32_TYPELESS
        case 10: case 11: case 12: case 13: case 14:    return 0x09u; // R16G16B16A16_TYPELESS
        case 16: case 17: case 18:                      return 0x0Fu; // R32G32_TYPELESS
        case 20: case 21: case 22:                      return 0x13u; // R32G8X24_TYPELESS
        case 24: case 25:                               return 0x17u; // R10G10B10A2_TYPELESS
        case 28: case 29: case 30: case 31: case 32:    return 0x1Bu; // R8G8B8A8_TYPELESS
        case 34: case 35: case 36: case 37: case 38:    return 0x21u; // R16G16_TYPELESS
        case 40: case 41: case 42: case 43:             return 0x27u; // R32_TYPELESS
        case 45: case 46: case 47:                      return 0x2Cu; // R24G8_TYPELESS
        case 49: case 50: case 51: case 52:             return 0x30u; // R8G8_TYPELESS
        case 54: case 55: case 56: case 57: case 58: case 59: return 0x35u; // R16_TYPELESS
        case 61: case 62: case 63: case 64:             return 0x3Cu; // R8_TYPELESS
        case 71: case 72:                               return 0x46u; // BC1_TYPELESS
        case 74: case 75:                               return 0x49u; // BC2_TYPELESS
        case 77: case 78:                               return 0x4Cu; // BC3_TYPELESS
        case 80: case 81:                               return 0x4Fu; // BC4_TYPELESS
        case 83: case 84:                               return 0x52u; // BC5_TYPELESS
        case 87: case 91:                               return 0x5Au; // B8G8R8A8_TYPELESS
        case 88: case 93:                               return 0x5Cu; // B8G8R8X8_TYPELESS
        case 95: case 96:                               return 0x5Eu; // BC6H_TYPELESS
        case 98: case 99:                               return 0x61u; // BC7_TYPELESS
        default:                                        return fmt;
    }
}

static inline uint32_t GXDxgiFormat_StripSrgb(uint32_t fmt) {
    switch (fmt) {
        case 0x1Du: return 0x1Cu; // R8G8B8A8_UNORM_SRGB -> R8G8B8A8_UNORM
        case 0x48u: return 0x47u; // BC1_UNORM_SRGB      -> BC1_UNORM
        case 0x4Bu: return 0x4Au; // BC2_UNORM_SRGB      -> BC2_UNORM
        case 0x4Eu: return 0x4Du; // BC3_UNORM_SRGB      -> BC3_UNORM
        case 0x5Bu: return 0x57u; // B8G8R8A8_UNORM_SRGB -> B8G8R8A8_UNORM
        case 0x5Du: return 0x58u; // B8G8R8X8_UNORM_SRGB -> B8G8R8X8_UNORM
        case 0x63u: return 0x62u; // BC7_UNORM_SRGB      -> BC7_UNORM
        default:    return fmt;
    }
}

// 0x21a8140 (125 B): GXCubeTextureResource_RecreateInternalSrv
int GXCubeTextureResource_RecreateInternalSrv(
    GXTextureResource* pThis,
    uint32_t format,
    uint32_t /*unused*/,
    uint32_t mipLevels)
{
    if (!pThis->m_pGnmTexture) return 5;
    uint32_t srvDesc[6] = { format, 9, 0, mipLevels, 0, 6 };
    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    return GXDevice_CreateShaderResourceView(pDev, &pThis->m_pSrv, pThis->m_pGnmTexture, srvDesc, nullptr);
}

// 0x21a7c20 (313 B): GXCubeTextureResource_ReleaseGpuResources
void GXCubeTextureResource_ReleaseGpuResources(GXTextureResource* pThis) {
    GXResourceMemoryTracker_UnregisterTextureBucket(pThis, 0x40, 0x48, 0x60, 0x64, 0x68);
    CGViewGnm_DestroyAndFree(pThis->m_pSrv);
    CGViewGnm_DestroyAndFree(pThis->m_pUav);
    CGTextureGnmRef_Release(pThis->m_pGnmTexture);
    pThis->m_colorScale.m_colorScale = 1.0f;
    pThis->m_byteSize = 0;
}

// 0x21a7f20 (535 B): GXCubeTextureResource_CreateCubeTexture
bool GXCubeTextureResource_CreateCubeTexture(
    GXTextureResource* pThis,
    uint32_t edgeLength,
    uint32_t format,
    uint32_t mipLevels,
    const void* pInitData,
    void* pSrvExtra,
    void* pTexExtra)
{
    GXCubeTextureResource_ReleaseGpuResources(pThis);
    uint32_t texDesc[12] = {
        edgeLength, edgeLength, mipLevels, 6, format, 1, 0, 0, 8, 0, 4, 0
    };
    CGTextureGnmRef* pNewTex = nullptr;
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    int err = GXDevice_CreateTexture(
        pDev, reinterpret_cast<void**>(&pNewTex), texDesc, pInitData, pSrvExtra, pTexExtra, nullptr);

    CGTextureGnmRef_Assign(pThis->m_pGnmTexture, pNewTex);

    if (err == 0) {
        CGTextureGnmRef_ReleaseRaw(pNewTex);
        pThis->m_byteSize = pNewTex ? pNewTex->m_byteSize : 0;
        if (pThis->m_pGnmTexture) {
            uint32_t srvDesc[6] = { format, 9, 0, mipLevels, 0, 6 };
            pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
            pDev = *reinterpret_cast<void**>(pGlobal);
            err = GXDevice_CreateShaderResourceView(
                pDev, &pThis->m_pSrv, pThis->m_pGnmTexture, srvDesc, pSrvExtra);
        } else {
            err = 5;
        }
    }

    pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal) {
        auto** ppTracker = *reinterpret_cast<GXResourceMemoryTracker***>(pGlobal + 0x650);
        if (ppTracker) {
            using RegCubeFn = void(*)(GXResourceMemoryTracker*, void*);
            reinterpret_cast<RegCubeFn>(GX_VMA(0x21ac7d0ULL))(*ppTracker, pThis);
        }
    }
    return err == 0;
}

// 0x21a7d70 (422 B): GXCubeTextureResource_LoadFromMemoryContainer
bool GXCubeTextureResource_LoadFromMemoryContainer(
    GXTextureResource* pThis,
    void* pContainer)
{
    GXCubeTextureResource_ReleaseGpuResources(pThis);
    void** vtbl = *reinterpret_cast<void***>(pContainer);
    using LoadFn = int(*)(void*, CGTextureGnmRef**, void**, uint32_t);
    CGTextureGnmRef* pNewTex = nullptr;
    void* pNewSrv = nullptr;
    int err = reinterpret_cast<LoadFn>(vtbl[0x58 / 8])(pContainer, &pNewTex, &pNewSrv, 0);
    if (err == 0) {
        if (pNewTex) {
            uint8_t fmt = pNewTex->m_dxgiFormat;
            if (static_cast<uint8_t>(fmt - 0x46u) <= 8u && ((0x1C7u >> (fmt - 0x46u)) & 1u)) {
                GXTextureColorScale_InitFromDdsTailChunk(&pThis->m_colorScale, pContainer);
            }
        }
        pThis->m_byteSize = pNewTex ? pNewTex->m_byteSize : 0;
    }
    CGTextureGnmRef_Assign(pThis->m_pGnmTexture, pNewTex);
    pThis->m_pSrv = pNewSrv;
    if (pNewTex) {
        CGTextureGnmRef_ReleaseRaw(pNewTex);
    }
    return err == 0;
}

// 0x21a7d60 (11 B): GXCubeTextureResource_GetGnmTextureByteSizeSafe
uint32_t GXCubeTextureResource_GetGnmTextureByteSizeSafe(const CGTextureGnmRef* pTex) {
    return pTex ? pTex->m_byteSize : 0;
}

// 0x21a7ba0 (128 B): GXCubeTextureResource_BaseDtor
void GXCubeTextureResource_BaseDtor(GXTextureResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6ba0ULL));
    GXCubeTextureResource_ReleaseGpuResources(pThis);
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    GXTextureColorScale_Destroy(&pThis->m_colorScale);
}

// 0x21a7b20 (127 B): GXCubeTextureResource_DeletingDtor
void GXCubeTextureResource_DeletingDtor(GXTextureResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6ba0ULL));
    GXCubeTextureResource_ReleaseGpuResources(pThis);
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    if (pThis->m_colorScale.m_pConstantBuffer) {
        CGBufferGnm_Release(pThis->m_colorScale.m_pConstantBuffer);
    }
    sub_2bbe588(pThis);
}

// 0x21a7ad0 (77 B): GXCubeTextureResource_Construct
void GXCubeTextureResource_Construct(GXTextureResource* pThis) {
    pThis->m_vptr        = reinterpret_cast<void**>(GX_VMA(0x53b6ba0ULL));
    pThis->m_refCount    = 0;
    GXTextureColorScale_Construct(&pThis->m_colorScale);
    pThis->m_pGnmTexture = nullptr;
    pThis->m_pSrv        = nullptr;
    pThis->m_pUav        = nullptr;
    pThis->m_byteSize    = 0;
}

// 0x21a7a30 (5 B): GXResourceBase_GetColorScaleCBuffer
void* GXResourceBase_GetColorScaleCBuffer(const GXResourceBase* pThis) {
    return pThis->m_colorScale.m_pConstantBuffer;
}

// 0x21a7a00 (38 B): GXResourceBase_EnsureInitializedDoubleCheck
uint32_t GXResourceBase_EnsureInitializedDoubleCheck(GXResourceBase* pThis) {
    using VFn28 = void*(*)(GXResourceBase*);
    if (reinterpret_cast<VFn28>(pThis->m_vptr[0x28 / 8])(pThis) != nullptr) {
        reinterpret_cast<VFn28>(pThis->m_vptr[0x28 / 8])(pThis);
    }
    return 0;
}

// 0x21a79f0 (6 B): GXResourceBase_CallVirtual28Thunk
void* GXResourceBase_CallVirtual28Thunk(GXResourceBase* pThis) {
    using VFn28 = void*(*)(GXResourceBase*);
    return reinterpret_cast<VFn28>(pThis->m_vptr[0x28 / 8])(pThis);
}

// 0x21a79c0 (48 B): GXResourceBase_BaseDtor
void GXResourceBase_BaseDtor(GXResourceBase* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    GXTextureColorScale_Destroy(&pThis->m_colorScale);
}

// 0x21a7990 (47 B): GXResourceBase_DeletingDtor
void GXResourceBase_DeletingDtor(GXResourceBase* pThis) {
    GXResourceBase_BaseDtor(pThis);
    sub_2bbe588(pThis);
}

// 0x21a7960 (46 B): GXResourceBase_Construct
void GXResourceBase_Construct(GXResourceBase* pThis) {
    pThis->m_vptr     = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    pThis->m_refCount = 0;
    GXTextureColorScale_Construct(&pThis->m_colorScale);
}

// 0x21a7850 (120 B): GXTexture2DResource_CreateCustomSrv
int GXTexture2DResource_CreateCustomSrv(
    void* /*pThisUnused*/,
    void** outSrv,
    void* pGnmTex,
    uint32_t format,
    const uint32_t* pSampleCount,
    uint32_t mipLevels,
    void* pExtra)
{
    uint32_t viewDim = (*pSampleCount > 1) ? 6u : 4u;
    uint32_t srvDesc[6] = { format, viewDim, 0, mipLevels, 0, 1 };
    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    return GXDevice_CreateShaderResourceView(pDev, outSrv, pGnmTex, srvDesc, pExtra);
}

// 0x21a7800 (70 B): GXTexture2DResource_OrFlagsAndLoadFromMemory
bool GXTexture2DResource_OrFlagsAndLoadFromMemory(
    GXTextureResource* pThis,
    void* pContainer,
    uint32_t extraFlags)
{
    void** vtbl = *reinterpret_cast<void***>(pContainer);
    using GetFlagsFn = uint32_t(*)(void*);
    using SetFlagsFn = void(*)(void*, uint32_t);
    uint32_t cur = reinterpret_cast<GetFlagsFn>(vtbl[0x50 / 8])(pContainer);
    reinterpret_cast<SetFlagsFn>(vtbl[0x48 / 8])(pContainer, cur | extraFlags);
    using LoadFn = bool(*)(GXTextureResource*, void*);
    return reinterpret_cast<LoadFn>(pThis->m_vptr[0xB8 / 8])(pThis, pContainer);
}

// 0x21a75a0 (602 B): GXTexture2DResource_LoadFromStreamContainer
bool GXTexture2DResource_LoadFromStreamContainer(
    GXTextureResource* pThis,
    void* pContainer)
{
    using ReleaseFn = void(*)(GXTextureResource*);
    reinterpret_cast<ReleaseFn>(pThis->m_vptr[0x90 / 8])(pThis);

    void** vtbl = *reinterpret_cast<void***>(pContainer);
    using LoadFn = int(*)(void*, CGTextureGnmRef**, void**, uint32_t);
    CGTextureGnmRef* pNewTex = nullptr;
    void* pNewSrv = nullptr;
    int err = reinterpret_cast<LoadFn>(vtbl[0x68 / 8])(pContainer, &pNewTex, &pNewSrv, 0);
    if (err == 0) {
        if (pNewTex) {
            uint8_t fmt = pNewTex->m_dxgiFormat;
            if (static_cast<uint8_t>(fmt - 0x46u) <= 8u && ((0x1C7u >> (fmt - 0x46u)) & 1u)) {
                GXTextureColorScale_InitFromDdsTailChunk(&pThis->m_colorScale, pContainer);
            } else {
                using GetDataFn = const void*(*)(void*);
                using GetSizeFn = uint32_t(*)(void*);
                const void* pData = reinterpret_cast<GetDataFn>(vtbl[0x70 / 8])(pContainer);
                uint32_t dataSize = reinterpret_cast<GetSizeFn>(vtbl[0x18 / 8])(pContainer);
                GXTextureColorScale_InitFromGnfUserData(&pThis->m_colorScale, pData, dataSize);
            }
        }
        pThis->m_byteSize = pNewTex ? pNewTex->m_byteSize : 0;
    }
    CGTextureGnmRef_Assign(pThis->m_pGnmTexture, pNewTex);
    pThis->m_pSrv = pNewSrv;
    if (pNewTex) {
        CGTextureGnmRef_ReleaseRaw(pNewTex);
    }
    return err == 0;
}

// 0x21a7550 (70 B): GXTexture2DResource_OrFlagsAndLoadFromStream
bool GXTexture2DResource_OrFlagsAndLoadFromStream(
    GXTextureResource* pThis,
    void* pContainer,
    uint32_t extraFlags)
{
    void** vtbl = *reinterpret_cast<void***>(pContainer);
    using GetFlagsFn = uint32_t(*)(void*);
    using SetFlagsFn = void(*)(void*, uint32_t);
    uint32_t cur = reinterpret_cast<GetFlagsFn>(vtbl[0x50 / 8])(pContainer);
    reinterpret_cast<SetFlagsFn>(vtbl[0x48 / 8])(pContainer, cur | extraFlags);
    using LoadFn = bool(*)(GXTextureResource*, void*);
    return reinterpret_cast<LoadFn>(pThis->m_vptr[0xA8 / 8])(pThis, pContainer);
}

// 0x21a72c0 (652 B): GXTexture2DResource_LoadFromMemoryContainer
bool GXTexture2DResource_LoadFromMemoryContainer(
    GXTextureResource* pThis,
    void* pContainer)
{
    using ReleaseFn = void(*)(GXTextureResource*);
    reinterpret_cast<ReleaseFn>(pThis->m_vptr[0x90 / 8])(pThis);

    void** vtbl = *reinterpret_cast<void***>(pContainer);
    using LoadFn = int(*)(void*, CGTextureGnmRef**, void**, uint32_t);
    CGTextureGnmRef* pNewTex = nullptr;
    void* pNewSrv = nullptr;
    int err = reinterpret_cast<LoadFn>(vtbl[0x58 / 8])(pContainer, &pNewTex, &pNewSrv, 0);
    if (err == 0) {
        if (pNewTex) {
            uint8_t fmt = pNewTex->m_dxgiFormat;
            if (static_cast<uint8_t>(fmt - 0x46u) <= 8u && ((0x1C7u >> (fmt - 0x46u)) & 1u)) {
                GXTextureColorScale_InitFromDdsTailChunk(&pThis->m_colorScale, pContainer);
            } else {
                using GetDataFn = const void*(*)(void*);
                using GetSizeFn = uint32_t(*)(void*);
                const void* pData = reinterpret_cast<GetDataFn>(vtbl[0x70 / 8])(pContainer);
                uint32_t dataSize = reinterpret_cast<GetSizeFn>(vtbl[0x18 / 8])(pContainer);
                GXTextureColorScale_InitFromGnfUserData(&pThis->m_colorScale, pData, dataSize);
            }
        }
        pThis->m_byteSize = pNewTex ? pNewTex->m_byteSize : 0;
        CGTextureGnmRef_Assign(pThis->m_pGnmTexture, pNewTex);
        pThis->m_pSrv = pNewSrv;
        CGTextureGnmRef_ReleaseRaw(pNewTex);
    }
    return err == 0;
}

// 0x21a7230 (140 B): GXTexture2DResource_RecreateInternalSrv
int GXTexture2DResource_RecreateInternalSrv(
    GXTextureResource* pThis,
    uint32_t format,
    const uint32_t* pSampleCount,
    uint32_t mipLevels,
    void* pExtra)
{
    if (!pThis->m_pGnmTexture) return 5;
    uint32_t viewDim = (*pSampleCount > 1) ? 6u : 4u;
    uint32_t srvDesc[6] = { format, viewDim, 0, mipLevels, 0, 1 };
    void* pGlobal = GXGlobalDevice_GetInstance();
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    return GXDevice_CreateShaderResourceView(
        pDev, &pThis->m_pSrv, pThis->m_pGnmTexture, srvDesc, pExtra);
}

// 0x21a6c00 (1576 B): GXTexture2DResource_Create2DTexture
// Remaps `0x2E` (R24_UNORM_X8_TYPELESS) -> `0x2C` (R24G8_TYPELESS) and any sRGB
// format (`StripSrgb(fmt) != fmt`) to its parent typeless format (`MakeTypeless(fmt)`),
// creates the 2D `CGTextureGnm`, computes mip levels, creates SRV + optional UAV,
// and registers with `GXResourceMemoryTracker` (`0x21ac690`).
bool GXTexture2DResource_Create2DTexture(
    GXTextureResource* pThis,
    uint32_t width,
    uint32_t height,
    uint32_t format,
    uint32_t mipLevels,
    const void* pInitData,
    bool enableUav,
    uint32_t usageFlags,
    void* pSrvExtra,
    void* pTexExtra,
    bool useMultiSampleDesc)
{
    using ReleaseFn = void(*)(GXTextureResource*);
    reinterpret_cast<ReleaseFn>(pThis->m_vptr[0x90 / 8])(pThis);

    uint32_t texDescFormat = format;
    if (format == 0x2Eu) {
        texDescFormat = 0x2Cu;
    } else if (GXDxgiFormat_StripSrgb(format) != format) {
        texDescFormat = GXDxgiFormat_MakeTypeless(format);
    }

    uint32_t bindFlags = enableUav ? 0x88u : 0x08u;
    uint32_t texDesc[12] = {
        width, height, mipLevels, 1, texDescFormat, 1, 0, usageFlags, bindFlags, 0, 0, 0
    };
    alignas(16) uint8_t msaaDesc[16] = {};
    if (useMultiSampleDesc) {
        msaaDesc[0] = 0;
        msaaDesc[1] = 1;
        msaaDesc[2] = 1;
        *reinterpret_cast<uint32_t*>(&msaaDesc[12]) = 8;
    }

    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGTextureGnmRef* pNewTex = nullptr;
    int err = GXDevice_CreateTexture(
        pDev,
        reinterpret_cast<void**>(&pNewTex),
        texDesc,
        pInitData,
        pSrvExtra,
        pTexExtra,
        useMultiSampleDesc ? msaaDesc : nullptr);

    CGTextureGnmRef_Assign(pThis->m_pGnmTexture, pNewTex);

    if (err == 0) {
        CGTextureGnmRef_ReleaseRaw(pNewTex);
        pThis->m_byteSize = pNewTex ? pNewTex->m_byteSize : 0;
        uint32_t actualMips = mipLevels;
        if (actualMips == 0) {
            actualMips = pNewTex ? pNewTex->m_mipLevels : 0;
            if (actualMips == 0) {
                uint32_t w = width, h = height;
                if (h != 0 && w != 0) {
                    do {
                        ++actualMips;
                        h >>= 1;
                        if (h == 0) break;
                        w >>= 1;
                    } while (w != 0);
                }
            }
        }
        if (!pThis->m_pGnmTexture) {
            err = 5;
        } else {
            uint32_t srvDesc[6] = { format, 4, 0, actualMips, 0, 1 };
            pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
            pDev = *reinterpret_cast<void**>(pGlobal);
            err = GXDevice_CreateShaderResourceView(
                pDev, &pThis->m_pSrv, pThis->m_pGnmTexture, srvDesc, pSrvExtra);
            if (err == 0 && enableUav) {
                uint32_t uavDesc[5] = { texDescFormat, 4, actualMips - 1, 0, 1 };
                pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
                pDev = *reinterpret_cast<void**>(pGlobal);
                err = GXDevice_CreateUnorderedAccessView(
                    pDev, &pThis->m_pUav, pThis->m_pGnmTexture, uavDesc, nullptr);
            }
        }
    }

    pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal) {
        auto** ppTracker = *reinterpret_cast<GXResourceMemoryTracker***>(pGlobal + 0x650);
        if (ppTracker) {
            using RegTex2DFn = void(*)(GXResourceMemoryTracker*, void*);
            reinterpret_cast<RegTex2DFn>(GX_VMA(0x21ac690ULL))(*ppTracker, pThis);
        }
    }
    return err == 0;
}

// 0x21a6bf0 (11 B): GXTexture2DResource_GetGnmTextureByteSizeSafe
uint32_t GXTexture2DResource_GetGnmTextureByteSizeSafe(const CGTextureGnmRef* pTex) {
    return pTex ? pTex->m_byteSize : 0;
}

// 0x21a6b40 (161 B): GXTexture2DResource_AttachExistingGnmTextureAndSrv
bool GXTexture2DResource_AttachExistingGnmTextureAndSrv(
    GXTextureResource* pThis,
    CGTextureGnmRef* pTex,
    void* pSrv)
{
    using ReleaseFn = void(*)(GXTextureResource*);
    reinterpret_cast<ReleaseFn>(pThis->m_vptr[0x90 / 8])(pThis);
    CGTextureGnmRef_Assign(pThis->m_pGnmTexture, pTex);
    pThis->m_pSrv     = pSrv;
    pThis->m_byteSize = pTex ? pTex->m_byteSize : 0;
    return true;
}

// 0x21a6a00 (306 B): GXTexture2DResource_ReleaseGpuResources
void GXTexture2DResource_ReleaseGpuResources(GXTextureResource* pThis) {
    GXResourceMemoryTracker_UnregisterTextureBucket(pThis, 0x08, 0x10, 0x28, 0x2C, 0x30);
    CGViewGnm_DestroyAndFree(pThis->m_pSrv);
    CGViewGnm_DestroyAndFree(pThis->m_pUav);
    CGTextureGnmRef_Release(pThis->m_pGnmTexture);
    pThis->m_byteSize = 0;
    pThis->m_colorScale.m_colorScale = 1.0f;
}

// 0x21a6980 (128 B): GXTexture2DResource_BaseDtor
void GXTexture2DResource_BaseDtor(GXTextureResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6a20ULL));
    GXTexture2DResource_ReleaseGpuResources(pThis);
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    GXTextureColorScale_Destroy(&pThis->m_colorScale);
}

// 0x21a6900 (127 B): GXTexture2DResource_DeletingDtor
void GXTexture2DResource_DeletingDtor(GXTextureResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6a20ULL));
    GXTexture2DResource_ReleaseGpuResources(pThis);
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x53b6b00ULL));
    if (pThis->m_colorScale.m_pConstantBuffer) {
        CGBufferGnm_Release(pThis->m_colorScale.m_pConstantBuffer);
    }
    sub_2bbe588(pThis);
}

// 0x21a68b0 (77 B): GXTexture2DResource_Construct
void GXTexture2DResource_Construct(GXTextureResource* pThis) {
    pThis->m_vptr        = reinterpret_cast<void**>(GX_VMA(0x53b6a20ULL));
    pThis->m_refCount    = 0;
    GXTextureColorScale_Construct(&pThis->m_colorScale);
    pThis->m_pGnmTexture = nullptr;
    pThis->m_pSrv        = nullptr;
    pThis->m_pUav        = nullptr;
    pThis->m_byteSize    = 0;
}


// ============================================================================
// 4. GXShaderResource Hierarchy — Pixel, Compute, Geometry, Vertex, Hull/Domain
//    (0x21a50c0 .. 0x21a68a0)
// ============================================================================

struct CGShaderGnm {
    uint8_t   m_pad00[0x100];
    uint32_t* m_pShaderBinary;      // +0x100 (low 23 bits of [0] = GPU binary byte size)
    void*     m_pShaderHeader;      // +0x108
    void*     m_pBinaryAllocator;   // +0x110
    void*     m_pHeaderAllocator;   // +0x118
};

struct GXShaderResource {
    void**       m_vptr;            // +0x00
    uint32_t     m_refCount;        // +0x08
    uint32_t     m_bytecodeSize;    // +0x0C
    CGShaderGnm* m_pGnmShader;      // +0x10
    void*        m_pCpuBytecodeCopy;// +0x18
    uint64_t     m_cpuBytecodeSize; // +0x20
};

static void GXShaderResource_FreeGnmShader(CGShaderGnm*& pShd) {
    if (!pShd) return;
    void* pObjAlloc = CGObject_GetCreatorAllocator(pShd);
    using FreeFn = void(*)(void*, void*);

    if (pShd->m_pShaderHeader) {
        const uint64_t gpuUcodeSpan =
            (pShd->m_pShaderBinary ? static_cast<uint64_t>((*pShd->m_pShaderBinary) & 0x7FFFFFu) : 0ULL) + 256ULL;
        gx_direct_vk::RetireDirectShaderRange(pShd->m_pShaderHeader, gpuUcodeSpan);
    }
    if (pShd->m_pBinaryAllocator) {
        void** vtbl = *reinterpret_cast<void***>(pShd->m_pBinaryAllocator);
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pShd->m_pBinaryAllocator, pShd->m_pShaderBinary);
    }
    if (pShd->m_pHeaderAllocator) {
        void** vtbl = *reinterpret_cast<void***>(pShd->m_pHeaderAllocator);
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pShd->m_pHeaderAllocator, pShd->m_pShaderHeader);
    }
    if (pObjAlloc) {
        void** vtbl = *reinterpret_cast<void***>(pObjAlloc);
        reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pObjAlloc, pShd);
    }
    pShd = nullptr;
}

static inline void GXShaderResource_TrackerUnregisterVirtual(GXShaderResource* pThis) {
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (!pGlobal) return;
    uint8_t** ppTracker = *reinterpret_cast<uint8_t***>(pGlobal + 0x650);
    if (!ppTracker || !pThis) return;
    uint8_t* pTracker = *ppTracker;
    using GetByteSizeFn = uint32_t(*)(GXShaderResource*);
    uint32_t sz = reinterpret_cast<GetByteSizeFn>(pThis->m_vptr[0x10 / 8])(pThis);
    if (sz != 0) {
        uint32_t* pBytes = reinterpret_cast<uint32_t*>(pTracker + 0x180);
        float*    pMiB   = reinterpret_cast<float*>(pTracker + 0x184);
        uint32_t* pCount = reinterpret_cast<uint32_t*>(pTracker + 0x17C);
        uint32_t newBytes = *pBytes - sz;
        *pBytes = newBytes;
        *pMiB   = static_cast<float>(static_cast<uint64_t>(newBytes)) * (1.0f / 1048576.0f);
        *pCount -= 1u;
    }
}

static inline void GXShaderResource_TrackerUnregisterDirect(
    GXShaderResource* pThis,
    uint32_t(*fnGetByteSize)(const GXShaderResource*))
{
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (!pGlobal) return;
    uint8_t** ppTracker = *reinterpret_cast<uint8_t***>(pGlobal + 0x650);
    if (!ppTracker || !pThis) return;
    uint8_t* pTracker = *ppTracker;
    uint32_t sz = fnGetByteSize(pThis);
    if (sz != 0) {
        uint32_t* pBytes = reinterpret_cast<uint32_t*>(pTracker + 0x180);
        float*    pMiB   = reinterpret_cast<float*>(pTracker + 0x184);
        uint32_t* pCount = reinterpret_cast<uint32_t*>(pTracker + 0x17C);
        uint32_t newBytes = *pBytes - sz;
        *pBytes = newBytes;
        *pMiB   = static_cast<float>(static_cast<uint64_t>(newBytes)) * (1.0f / 1048576.0f);
        *pCount -= 1u;
    }
}

static inline void GXShaderResource_TrackerRegisterVirtual(GXShaderResource* pThis) {
    if (!pThis || !pThis->m_pGnmShader) return;
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (!pGlobal) return;
    uint8_t** ppTracker = *reinterpret_cast<uint8_t***>(pGlobal + 0x650);
    if (!ppTracker) return;
    uint8_t* pTracker = *ppTracker;
    using GetByteSizeFn = uint32_t(*)(GXShaderResource*);
    uint32_t sz = reinterpret_cast<GetByteSizeFn>(pThis->m_vptr[0x10 / 8])(pThis);
    if (sz != 0) {
        uint32_t* pBytes = reinterpret_cast<uint32_t*>(pTracker + 0x180);
        float*    pMiB   = reinterpret_cast<float*>(pTracker + 0x184);
        uint32_t* pCount = reinterpret_cast<uint32_t*>(pTracker + 0x17C);
        uint32_t newBytes = *pBytes + sz;
        *pBytes = newBytes;
        *pMiB   = static_cast<float>(static_cast<uint64_t>(newBytes)) * (1.0f / 1048576.0f);
        *pCount += 1u;
    }
}

static inline void GXShaderResource_FreeVsGsCpuBytecodeRaw(void* pCopy) {
    if (!pCopy) return;
    void** pTlsHeapSlot = reinterpret_cast<void**>(gx_guest_tls_slot(0x53e44a8ULL) + 0x10);
    void* pHeap = *pTlsHeapSlot;
    *pTlsHeapSlot = nullptr;
    if (!pHeap) {
        pHeap = CGObject_GetCreatorAllocator(pCopy);
        if (!pHeap) {
            sub_20b55b0(
                reinterpret_cast<const void*>(GX_VMA(0x493bac3ULL)),
                0x64u,
                reinterpret_cast<const void*>(GX_VMA(0x4940207ULL)));
        }
    }
    using FreeFn = void(*)(void*, void*);
    void** vtbl = *reinterpret_cast<void***>(pHeap);
    reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pHeap, pCopy);
}

static inline void GXShaderResource_FreePsCpuBytecodeRaw(void* pCopy) {
    if (!pCopy) return;
    void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    using FreeFn = void(*)(void*, void*);
    void** vtbl = *reinterpret_cast<void***>(pAlloc);
    reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pAlloc, pCopy);
}

// 0x21a68a0 (5 B): GXShaderResource_GetCpuBytecodeSize
uint64_t GXShaderResource_GetCpuBytecodeSize(const GXShaderResource* pThis) {
    return pThis->m_cpuBytecodeSize;
}

// 0x21a6890 (5 B): GXShaderResource_GetCpuBytecodePtr
void* GXShaderResource_GetCpuBytecodePtr(const GXShaderResource* pThis) {
    return pThis->m_pCpuBytecodeCopy;
}

// 0x21a6880 (5 B): GXPixelShaderResource_GetGnmShader
CGShaderGnm* GXPixelShaderResource_GetGnmShader(const GXShaderResource* pThis) {
    return pThis->m_pGnmShader;
}

// 0x21a6870 (5 B): GXComputeShaderResource_GetGnmShader
CGShaderGnm* GXComputeShaderResource_GetGnmShader(const GXShaderResource* pThis) {
    return pThis->m_pGnmShader;
}

// 0x21a6860 (5 B): GXGeometryShaderResource_GetGnmShader
CGShaderGnm* GXGeometryShaderResource_GetGnmShader(const GXShaderResource* pThis) {
    return pThis->m_pGnmShader;
}

// 0x21a6850 (5 B): GXVertexShaderResource_GetGnmShader
CGShaderGnm* GXVertexShaderResource_GetGnmShader(const GXShaderResource* pThis) {
    return pThis->m_pGnmShader;
}

// 0x21a6840 (5 B): GXHullShaderResource_GetGnmShader
CGShaderGnm* GXHullShaderResource_GetGnmShader(const GXShaderResource* pThis) {
    return pThis->m_pGnmShader;
}

// 0x21a6830 (5 B): GXDomainShaderResource_GetGnmShader
CGShaderGnm* GXDomainShaderResource_GetGnmShader(const GXShaderResource* pThis) {
    return pThis->m_pGnmShader;
}

// 0x21a6780 (28 B): GXPixelShaderResource_GetByteSize
uint32_t GXPixelShaderResource_GetByteSize(const GXShaderResource* pThis) {
    if (pThis->m_pGnmShader && pThis->m_pGnmShader->m_pShaderBinary) {
        return (*pThis->m_pGnmShader->m_pShaderBinary) & 0x7FFFFFu;
    }
    return pThis->m_bytecodeSize;
}

// 0x21a6760 (28 B): GXPixelShaderResource_BindToDeviceContext
void GXPixelShaderResource_BindToDeviceContext(const GXShaderResource* pThis, uint8_t* pDevCtx) {
    if (pThis->m_pGnmShader) {
        *reinterpret_cast<CGShaderGnm**>(pDevCtx + 0x780) = pThis->m_pGnmShader;
        uint8_t* pCmdCtx = *reinterpret_cast<uint8_t**>(pDevCtx + 0x08);
        *reinterpret_cast<CGShaderGnm**>(pCmdCtx + 0xC1B8) = pThis->m_pGnmShader;
    }
}

// 0x21a6400 (268 B): GXPixelShaderResource_ReleaseGpuShader
void GXPixelShaderResource_ReleaseGpuShader(GXShaderResource* pThis) {
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterVirtual(pThis);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    if (pThis->m_pCpuBytecodeCopy) {
        GXShaderResource_FreePsCpuBytecodeRaw(pThis->m_pCpuBytecodeCopy);
        pThis->m_pCpuBytecodeCopy = nullptr;
    }
    pThis->m_cpuBytecodeSize  = 0;
    pThis->m_pCpuBytecodeCopy = nullptr;
    pThis->m_pGnmShader       = nullptr;
}

// 0x21a6510 (588 B): GXPixelShaderResource_CreateFromShaderBinary
bool GXPixelShaderResource_CreateFromShaderBinary(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* a4,
    void* a5)
{
    GXPixelShaderResource_ReleaseGpuShader(pThis);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGShaderGnm* pNewPs = nullptr;
    int err = GXDevice_CreatePixelShader(
        pDev, reinterpret_cast<void**>(&pNewPs), pCode, codeSize, a4, a5);
    if (err != 0) return false;
    pThis->m_pGnmShader   = pNewPs;
    pThis->m_bytecodeSize = static_cast<uint32_t>(codeSize);
    if (pNewPs) {
        GXShaderResource_TrackerRegisterVirtual(pThis);
    }
    if (*reinterpret_cast<const uint8_t*>(GX_VMA(0x513ac87ULL)) == 0) {
        void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        using AllocFn = void*(*)(void*, size_t);
        void** vtbl = *reinterpret_cast<void***>(pAlloc);
        void* pBuf = reinterpret_cast<AllocFn>(vtbl[0x50 / 8])(pAlloc, codeSize);
        pThis->m_pCpuBytecodeCopy = pBuf;
        if (!pBuf) return false;
        pThis->m_cpuBytecodeSize = codeSize;
        sub_261a1b0(pBuf, pCode, codeSize);
    }
    return pThis->m_pGnmShader != nullptr;
}

// 0x21a62e0 (277 B): GXPixelShaderResource_BaseDtor
void GXPixelShaderResource_BaseDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f940ULL));
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterDirect(pThis, &GXPixelShaderResource_GetByteSize);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    if (pThis->m_pCpuBytecodeCopy) {
        GXShaderResource_FreePsCpuBytecodeRaw(pThis->m_pCpuBytecodeCopy);
        pThis->m_pCpuBytecodeCopy = nullptr;
    }
    pThis->m_cpuBytecodeSize  = 0;
    pThis->m_pCpuBytecodeCopy = nullptr;
    pThis->m_pGnmShader       = nullptr;
}

// 0x21a61e0 (247 B): GXPixelShaderResource_DeletingDtor
void GXPixelShaderResource_DeletingDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f940ULL));
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterDirect(pThis, &GXPixelShaderResource_GetByteSize);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    if (pThis->m_pCpuBytecodeCopy) {
        GXShaderResource_FreePsCpuBytecodeRaw(pThis->m_pCpuBytecodeCopy);
    }
    sub_2bbe588(pThis);
}

// 0x21a61a0 (49 B): GXPixelShaderResource_Construct
void GXPixelShaderResource_Construct(GXShaderResource* pThis) {
    pThis->m_vptr             = reinterpret_cast<void**>(GX_VMA(0x535f940ULL));
    pThis->m_refCount         = 0;
    pThis->m_bytecodeSize     = 0;
    pThis->m_pGnmShader       = nullptr;
    pThis->m_pCpuBytecodeCopy = nullptr;
    pThis->m_cpuBytecodeSize  = 0;
}

// 0x21a6180 (28 B): GXComputeShaderResource_GetByteSize
uint32_t GXComputeShaderResource_GetByteSize(const GXShaderResource* pThis) {
    return GXPixelShaderResource_GetByteSize(pThis);
}

// 0x21a6160 (28 B): GXComputeShaderResource_BindToDeviceContext
void GXComputeShaderResource_BindToDeviceContext(const GXShaderResource* pThis, uint8_t* pDevCtx) {
    if (pThis->m_pGnmShader) {
        *reinterpret_cast<CGShaderGnm**>(pDevCtx + 0xA68) = pThis->m_pGnmShader;
        uint8_t* pCmdCtx = *reinterpret_cast<uint8_t**>(pDevCtx + 0x08);
        *reinterpret_cast<CGShaderGnm**>(pCmdCtx + 0xD598) = pThis->m_pGnmShader;
    }
}

// 0x21a5e90 (214 B): GXComputeShaderResource_ReleaseGpuShader
void GXComputeShaderResource_ReleaseGpuShader(GXShaderResource* pThis) {
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterVirtual(pThis);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    pThis->m_pGnmShader = nullptr;
}

// 0x21a5f70 (495 B): GXComputeShaderResource_CreateFromShaderBinary
bool GXComputeShaderResource_CreateFromShaderBinary(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* a4,
    void* a5)
{
    GXComputeShaderResource_ReleaseGpuShader(pThis);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGShaderGnm* pNewCs = nullptr;
    int err = GXDevice_CreateComputeShader(
        pDev, reinterpret_cast<void**>(&pNewCs), pCode, codeSize, a4, a5);
    if (err != 0) return false;
    pThis->m_pGnmShader   = pNewCs;
    pThis->m_bytecodeSize = static_cast<uint32_t>(codeSize);
    if (pNewCs) {
        GXShaderResource_TrackerRegisterVirtual(pThis);
    }
    return pThis->m_pGnmShader != nullptr;
}

// 0x21a5db0 (223 B): GXComputeShaderResource_BaseDtor
void GXComputeShaderResource_BaseDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f910ULL));
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterDirect(pThis, &GXComputeShaderResource_GetByteSize);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    pThis->m_pGnmShader = nullptr;
}

// 0x21a5cd0 (214 B): GXComputeShaderResource_DeletingDtor
void GXComputeShaderResource_DeletingDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f910ULL));
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterDirect(pThis, &GXComputeShaderResource_GetByteSize);
        if (pThis->m_pGnmShader) {
            void* pObjAlloc = CGObject_GetCreatorAllocator(pThis->m_pGnmShader);
            using FreeFn = void(*)(void*, void*);
            if (pThis->m_pGnmShader->m_pShaderHeader) {
                const uint64_t gpuUcodeSpan =
                    (pThis->m_pGnmShader->m_pShaderBinary
                         ? static_cast<uint64_t>((*pThis->m_pGnmShader->m_pShaderBinary) & 0x7FFFFFu)
                         : 0ULL) +
                    256ULL;
                gx_direct_vk::RetireDirectShaderRange(pThis->m_pGnmShader->m_pShaderHeader, gpuUcodeSpan);
            }
            if (pThis->m_pGnmShader->m_pBinaryAllocator) {
                void** vtbl = *reinterpret_cast<void***>(pThis->m_pGnmShader->m_pBinaryAllocator);
                reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pThis->m_pGnmShader->m_pBinaryAllocator, pThis->m_pGnmShader->m_pShaderBinary);
            }
            if (pThis->m_pGnmShader->m_pHeaderAllocator) {
                void** vtbl = *reinterpret_cast<void***>(pThis->m_pGnmShader->m_pHeaderAllocator);
                reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pThis->m_pGnmShader->m_pHeaderAllocator, pThis->m_pGnmShader->m_pShaderHeader);
            }
            if (pObjAlloc) {
                void** vtbl = *reinterpret_cast<void***>(pObjAlloc);
                reinterpret_cast<FreeFn>(vtbl[0x70 / 8])(pObjAlloc, pThis->m_pGnmShader);
            }
        }
    }
    sub_2bbe588(pThis);
}

// 0x21a5ca0 (33 B): GXComputeShaderResource_Construct
void GXComputeShaderResource_Construct(GXShaderResource* pThis) {
    pThis->m_vptr         = reinterpret_cast<void**>(GX_VMA(0x535f910ULL));
    pThis->m_refCount     = 0;
    pThis->m_bytecodeSize = 0;
    pThis->m_pGnmShader   = nullptr;
}

// 0x21a5c80 (28 B): GXVertexOrGeometryShaderResource_GetByteSize
uint32_t GXVertexOrGeometryShaderResource_GetByteSize(const GXShaderResource* pThis) {
    return GXPixelShaderResource_GetByteSize(pThis);
}

// 0x21a5690 (341 B): GXVertexShaderResource_ReleaseGpuShader
void GXVertexShaderResource_ReleaseGpuShader(GXShaderResource* pThis) {
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterVirtual(pThis);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    if (pThis->m_pCpuBytecodeCopy) {
        GXShaderResource_FreeVsGsCpuBytecodeRaw(pThis->m_pCpuBytecodeCopy);
        pThis->m_pCpuBytecodeCopy = nullptr;
    }
    pThis->m_cpuBytecodeSize  = 0;
    pThis->m_pCpuBytecodeCopy = nullptr;
    pThis->m_pGnmShader       = nullptr;
}

// 0x21a5a70 (516 B): GXGeometryShaderResource_CreateWithStreamOut
bool GXGeometryShaderResource_CreateWithStreamOut(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* pSoDecl,
    uint32_t numEntries,
    void* pBufferStrides,
    uint32_t numStrides,
    uint32_t rasterizedStream)
{
    GXVertexShaderResource_ReleaseGpuShader(pThis);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    void* pShaderAlloc = *reinterpret_cast<void**>(GX_VMA(0x55406a0ULL));
    CGShaderGnm* pNewGs = nullptr;
    int err = GXDevice_CreateGeometryShaderWithSO(
        pDev,
        reinterpret_cast<void**>(&pNewGs),
        pCode,
        codeSize,
        pSoDecl,
        numEntries,
        pBufferStrides,
        numStrides,
        rasterizedStream,
        pShaderAlloc,
        nullptr);
    if (err != 0) return false;
    pThis->m_pGnmShader = pNewGs;
    return pNewGs != nullptr;
}

// 0x21a57f0 (631 B): GXVertexShaderResource_CreateFromShaderBinary
bool GXVertexShaderResource_CreateFromShaderBinary(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* a4,
    void* a5)
{
    GXVertexShaderResource_ReleaseGpuShader(pThis);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGShaderGnm* pNewVs = nullptr;
    int err = GXDevice_CreateVertexShader(
        pDev, reinterpret_cast<void**>(&pNewVs), pCode, codeSize, a4, a5);
    if (err != 0) return false;
    pThis->m_pGnmShader   = pNewVs;
    pThis->m_bytecodeSize = static_cast<uint32_t>(codeSize);
    if (pNewVs) {
        GXShaderResource_TrackerRegisterVirtual(pThis);
    }
    void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    using AllocFn = void*(*)(void*, size_t);
    void** vtbl = *reinterpret_cast<void***>(pAlloc);
    void* pBuf = reinterpret_cast<AllocFn>(vtbl[0x50 / 8])(pAlloc, codeSize);
    pThis->m_pCpuBytecodeCopy = pBuf;
    if (!pBuf) return false;
    pThis->m_cpuBytecodeSize = codeSize;
    sub_261a1b0(pBuf, pCode, codeSize);
    return pThis->m_pGnmShader != nullptr;
}

// 0x21a5530 (350 B): GXVertexShaderResource_BaseDtor
void GXVertexShaderResource_BaseDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f8e0ULL));
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterDirect(pThis, &GXVertexOrGeometryShaderResource_GetByteSize);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    if (pThis->m_pCpuBytecodeCopy) {
        GXShaderResource_FreeVsGsCpuBytecodeRaw(pThis->m_pCpuBytecodeCopy);
        pThis->m_pCpuBytecodeCopy = nullptr;
    }
    pThis->m_cpuBytecodeSize  = 0;
    pThis->m_pCpuBytecodeCopy = nullptr;
    pThis->m_pGnmShader       = nullptr;
}

// 0x21a53f0 (320 B): GXVertexShaderResource_DeletingDtor
void GXVertexShaderResource_DeletingDtor(GXShaderResource* pThis) {
    pThis->m_vptr = reinterpret_cast<void**>(GX_VMA(0x535f8e0ULL));
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterDirect(pThis, &GXVertexOrGeometryShaderResource_GetByteSize);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    if (pThis->m_pCpuBytecodeCopy) {
        GXShaderResource_FreeVsGsCpuBytecodeRaw(pThis->m_pCpuBytecodeCopy);
    }
    sub_2bbe588(pThis);
}

// 0x21a53b0 (49 B): GXVertexShaderResource_Construct
void GXVertexShaderResource_Construct(GXShaderResource* pThis) {
    pThis->m_vptr             = reinterpret_cast<void**>(GX_VMA(0x535f8e0ULL));
    pThis->m_refCount         = 0;
    pThis->m_bytecodeSize     = 0;
    pThis->m_pGnmShader       = nullptr;
    pThis->m_pCpuBytecodeCopy = nullptr;
    pThis->m_cpuBytecodeSize  = 0;
}

// 0x21a5390 (28 B): GXHullOrDomainShaderResource_GetByteSize
uint32_t GXHullOrDomainShaderResource_GetByteSize(const GXShaderResource* pThis) {
    return GXPixelShaderResource_GetByteSize(pThis);
}

// 0x21a50c0 (214 B): GXHullOrDomainShaderResource_ReleaseGpuShader
void GXHullOrDomainShaderResource_ReleaseGpuShader(GXShaderResource* pThis) {
    if (pThis->m_pGnmShader) {
        GXShaderResource_TrackerUnregisterVirtual(pThis);
        GXShaderResource_FreeGnmShader(pThis->m_pGnmShader);
    }
    pThis->m_pGnmShader = nullptr;
}

// 0x21a51a0 (495 B): GXHullOrDomainShaderResource_CreateFromShaderBinary
bool GXHullOrDomainShaderResource_CreateFromShaderBinary(
    GXShaderResource* pThis,
    const void* pCode,
    size_t codeSize,
    void* a4,
    void* a5)
{
    GXHullOrDomainShaderResource_ReleaseGpuShader(pThis);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    void* pDev = *reinterpret_cast<void**>(pGlobal);
    CGShaderGnm* pNewShd = nullptr;
    int err = GXDevice_CreateHullOrDomainShader(
        pDev, reinterpret_cast<void**>(&pNewShd), pCode, codeSize, a4, a5);
    if (err != 0) return false;
    pThis->m_pGnmShader   = pNewShd;
    pThis->m_bytecodeSize = static_cast<uint32_t>(codeSize);
    if (pNewShd) {
        GXShaderResource_TrackerRegisterVirtual(pThis);
    }
    return pThis->m_pGnmShader != nullptr;
}

} // namespace GX
} // namespace Bloodborne
