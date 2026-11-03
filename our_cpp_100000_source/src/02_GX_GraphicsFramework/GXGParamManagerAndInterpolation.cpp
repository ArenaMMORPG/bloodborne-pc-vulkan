// ============================================================================
// Bloodborne Engine Decompilation Project
// Module: 02_GX_GraphicsFramework
// File: GXGParamManagerAndInterpolation.cpp
// Range: 0x217f4d0 .. 0x2189fff (80 functions, 42,848 bytes)
// Reconstruction: Strictly Right-to-Left (High VMA -> Low VMA), 100% Zero-Stub
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cwchar>

#include "gx_direct_vulkan.h"

namespace GX {

// ============================================================================
// Forward Declarations & Core Dantelion2 / GParam Runtime Structures
// ============================================================================

struct DLAllocator {
    struct VTable {
        void* pfn00;
        void* pfn08;
        union {
            void (*NotifyAllocatorRef)(DLAllocator* pThis);                                    // +0x10
            void (*GetAllocatorId)(DLAllocator* pThis);
        };
        void* pfn18;
        void  (*GetMemoryCapabilities)(void* pOutCaps, DLAllocator* pThis, uint32_t flags); // +0x20
        void* pfn28[5];
        void* (*Allocate)(DLAllocator* pThis, size_t size);                                 // +0x50
        void* (*AllocateAligned)(DLAllocator* pThis, size_t size, size_t alignment);        // +0x58
        void* pfn60;
        void* pfn68;
        void  (*Free)(DLAllocator* pThis, void* pMem);                                      // +0x70
    };
    VTable* m_pVTable;
};

struct DLString {
    void*        m_pReserved00;   // +0x00
    union {
        char     m_inlineBuf[16];
        char*    m_pHeapBuf;
    } m_data;                     // +0x08
    uint64_t     m_length;        // +0x18
    uint64_t     m_capacity;      // +0x20
    DLAllocator* m_pAllocator;    // +0x28
    uint8_t      m_bOwnsMemory;   // +0x30
    uint8_t      m_pad31[7];      // +0x31
}; // 0x38 bytes

struct DLWString {
    void*        m_pReserved00;   // +0x00
    union {
        wchar_t  m_inlineBuf[8];
        wchar_t* m_pHeapBuf;
    } m_data;                     // +0x08
    uint64_t     m_length;        // +0x18
    uint64_t     m_capacity;      // +0x20
    DLAllocator* m_pAllocator;    // +0x28
    uint8_t      m_bOwnsMemory;   // +0x30
    uint8_t      m_pad31[7];      // +0x31
}; // 0x38 bytes

template <size_t N>
struct DLFixedWString {
    void*    m_pReserved00;       // +0x00
    union {
        wchar_t  m_inlineBuf[8];
        wchar_t* m_pHeapBuf;
    } m_data;                     // +0x08
    uint64_t m_length;            // +0x18
    uint64_t m_capacity;          // +0x20
    wchar_t  m_fixedStorage[N];   // +0x28 .. +0x28 + N*2
    uint8_t  m_padA8;             // +0xA8 (for N=64)
    uint8_t  m_flagsA9;           // +0xA9 (for N=64)
    uint8_t  m_padAA[6];          // +0xAA .. +0xB0
}; // 0xB0 bytes for N=64

struct GParamBucketIterator {
    void* m_pListContainer00;     // +0x00
    void* m_pReserved08;          // +0x08
    void* m_pNode10;              // +0x10
}; // 0x18 bytes

template <typename T>
struct DLVector {
    T*           m_pBegin;        // +0x00
    T*           m_pEnd;          // +0x08
    union {
        T*       m_pCapacity;     // +0x10
        T*       m_pCap;
    };
    DLAllocator* m_pAllocator;    // +0x18
}; // 0x20 bytes

using GParamBucketVector = DLVector<GParamBucketIterator>;

struct GParamWStringVectorOwner {
    void*        m_pReserved00;   // +0x00
    DLWString*   m_pBegin08;      // +0x08
    DLWString*   m_pEnd10;        // +0x10
    DLWString*   m_pCap18;        // +0x18
    DLAllocator* m_pAllocator20;  // +0x20
};

struct GXGParamGroupLookupTable {
    void*               m_pReserved00[6]; // +0x00 .. +0x30
    DLVector<DLWString> m_vec30;          // +0x30 .. +0x50
    void*               m_pReserved50;    // +0x50
    DLVector<DLWString> m_vec58;          // +0x58 .. +0x78
    void*               m_pReserved78[2]; // +0x78 .. +0x88
}; // 0x88 bytes

using GParamEffectContainer = GXGParamGroupLookupTable;

struct GXGParamBinaryReader;

// 0x420-byte entry inside GParamLoadedFileNode (+0x18)
struct GParamLoadedFileEntry {
    void*                     m_pRawFileData00;         // +0x00 (node+0x18)
    uint32_t                  m_fileDataSize08;         // +0x08 (node+0x20)
    uint32_t                  m_pad0C;                  // +0x0C (node+0x24)
    DLWString                 m_fileName10;             // +0x10 (node+0x28)
    DLWString                 m_secondaryName48;        // +0x48 (node+0x60)
    DLFixedWString<64>        m_slotPath0_80;           // +0x80 (node+0x98)
    DLFixedWString<64>        m_slotPath1_130;          // +0x130 (node+0x148)
    DLFixedWString<64>        m_slotPath2_1E0;          // +0x1E0 (node+0x1F8)
    DLFixedWString<64>        m_slotPath3_290;          // +0x290 (node+0x2A8)
    bool                      m_bHasCustomPath340;      // +0x340 (node+0x358)
    uint8_t                   m_pad341[7];              // +0x341
    DLFixedWString<64>        m_customOverridePath348;  // +0x348 (node+0x360)
    GXGParamGroupLookupTable* m_pGroupLookup3F8;        // +0x3F8 (node+0x410)
    void*                     m_pParamContainer400;     // +0x400 (node+0x418)
    void*                     m_pParamBinding408;       // +0x408 (node+0x420)
    GXGParamBinaryReader*     m_pBinaryReader410;       // +0x410 (node+0x428)
    void*                     m_pSubObject418;          // +0x418 (node+0x430)
}; // 0x420 bytes

struct GParamLoadedFileNode {
    GParamLoadedFileNode* m_pNext;                      // +0x00
    GParamLoadedFileNode* m_pPrev;                      // +0x08
    uint64_t              m_fileKey;                    // +0x10
    GParamLoadedFileEntry m_entry;                      // +0x18 (0x420 bytes -> total 0x438)
};

struct GParamHashTable {
    void*                          m_pReserved00;       // +0x00
    void*                          m_pHeadNode08;       // +0x08
    uint64_t                       m_nodeCount10;       // +0x10
    union {
        DLAllocator*               m_pNodeAlloc18;      // +0x18
        DLAllocator*               m_pAllocator18;
    };
    void*                          m_pBucketReserved20; // +0x20
    DLVector<GParamBucketIterator> m_buckets28;         // +0x28 .. +0x48
    uint64_t                       m_bucketMask48;      // +0x48
    uint64_t                       m_bucketCount50;     // +0x50
    float                          m_maxLoadFactor58;   // +0x58
    uint32_t                       m_pad5C;             // +0x5C
}; // 0x60 bytes

struct GParamBlendSlotState {
    uint64_t m_targetFileKey;   // +0x00 (-1 = none)
    bool     m_bInterpolating;  // +0x08
    uint8_t  m_pad09[3];        // +0x09
    float    m_elapsedTime;     // +0x0C
    float    m_blendDuration;   // +0x10
    uint32_t m_pad14;           // +0x14
}; // 0x18 bytes

struct GParamBlendSubGroupNode {
    GParamBlendSubGroupNode* m_pNext;                   // +0x00
    GParamBlendSubGroupNode* m_pPrev;                   // +0x08
    uint32_t                 m_subGroupId;              // +0x10
    uint32_t                 m_pad14;                   // +0x14
    GParamBlendSlotState     m_slots[17];               // +0x18 .. +0x1B0 (17 * 0x18 = 0x198)
    uint8_t                  m_bSlotEnabled[17];        // +0x1B0 .. +0x1C1
    uint8_t                  m_pad1C1[7];               // +0x1C1 .. +0x1C8
}; // 0x1C8 bytes

struct GParamBlendGroupNode {
    GParamBlendGroupNode* m_pNext;                      // +0x00
    GParamBlendGroupNode* m_pPrev;                      // +0x08
    uint32_t              m_groupId;                    // +0x10
    uint32_t              m_pad14;                      // +0x14
    GParamHashTable       m_subGroupMap18;              // +0x18 .. +0x78
}; // 0x78 bytes

struct GParamOverridePropertyBinding {
    uint32_t m_reserved00;       // +0x00
    uint32_t m_pad04;            // +0x04
    uint32_t m_paramIndex08;     // +0x08
    uint32_t m_pad0C;            // +0x0C
    void*    m_pLiveValuePtr10;  // +0x10
    void*    m_pBackupValuePtr18;// +0x18
    uint32_t m_valueByteSize20;  // +0x20
    bool     m_bEnabled24;       // +0x24
    uint8_t  m_pad25[3];         // +0x25
}; // 0x28 bytes

struct GParamActiveOverrideTarget {
    uint8_t                        m_flags00[9];             // +0x00
    bool                           m_bIsBlending09;          // +0x09
    uint8_t                        m_pad0A[2];               // +0x0A
    float                          m_blendElapsed0C;         // +0x0C
    float                          m_blendDuration10;        // +0x10
    bool                           m_bUseFixedBlendFactor14; // +0x14
    uint8_t                        m_pad15[3];               // +0x15
    float                          m_fixedBlendFactor18;     // +0x18
    uint32_t                       m_groupId1C;              // +0x1C
    uint32_t                       m_subGroupId20;           // +0x20
    uint32_t                       m_instanceParamId24;      // +0x24
    bool                           m_bActive28;              // +0x28
    uint8_t                        m_pad29[3];               // +0x29
    int32_t                        m_propertyCount2C;        // +0x2C
    GParamOverridePropertyBinding* m_pProperties30;          // +0x30
    bool                           m_bNeedsImmediateSync38;  // +0x38
    uint8_t                        m_pad39[3];               // +0x39
    uint32_t                       m_lastEvaluatedSlot3C;    // +0x3C
    bool                           m_bDirty40;               // +0x40
    bool                           m_bApplied41;             // +0x41
};

struct GParamKeyValueNode {
    GParamKeyValueNode* m_pNext;  // +0x00
    GParamKeyValueNode* m_pPrev;  // +0x08
    uint64_t            m_key10;  // +0x10
    void*               m_pVal18; // +0x18
}; // 0x20 bytes

struct GParamInterpolationWorkerJob {
    void**                                m_pVTable00;        // +0x00 (0x535f4f0)
    void*                                 m_pReserved08;      // +0x08
    DLVector<GParamActiveOverrideTarget*> m_targets10;        // +0x10 .. +0x30
    GParamHashTable*                      m_pBlendGroupMap30; // +0x30
    GParamHashTable*                      m_pLoadedFileMap38; // +0x38
    float                                 m_deltaTime40;      // +0x40
    uint32_t                              m_pad44;            // +0x44
};

struct GParamManager {
    DLAllocator*        m_pAllocator00;              // +0x00
    union {
        struct {
            uint64_t    m_selectedMenuFileKeys08[2]; // +0x08..+0x18
            uint64_t    m_reserved18[2];             // +0x18..+0x28
        };
        struct {
            uint64_t    m_qword08;                   // +0x08
            uint32_t    m_dword10;                   // +0x10
            uint32_t    m_pad14_alt;                 // +0x14
            uint64_t    m_qword18;                   // +0x18
            uint32_t    m_dword20;                   // +0x20
            uint32_t    m_pad24_alt;                 // +0x24
        };
    };
    void*               m_pSubObject28;              // +0x28
    union {
        struct {
            uint64_t    m_slotFileKeys30[11];        // +0x30..+0x88
            uint32_t    m_slotSentinel88;            // +0x88
            uint32_t    m_pad8C;                     // +0x8C
        };
        int32_t         m_activeBlendIds30[24];      // +0x30..+0x90
    };
    union {
        DLWString       m_defaultDirPath90;          // +0x90..+0xC8
        DLWString       m_defaultDirStr90;
    };
    union {
        DLWString       m_defaultSearchPatternC8;    // +0xC8..+0x100
        DLWString       m_defaultSubDirStrC8;
    };
    union {
        struct {
            uint64_t            m_reserved100;       // +0x100
            DLVector<DLWString> m_stringList108;     // +0x108..+0x128
        };
        GParamWStringVectorOwner m_searchPathVec100; // +0x100..+0x128
    };
    union {
        bool            m_bEnableMultiThread128;     // +0x128
        uint8_t         m_byte128;
    };
    uint8_t             m_pad129[7];                 // +0x129
    union {
        uint8_t               m_groupLookup130[0x88];// +0x130..+0x1B8
        GParamEffectContainer m_effectContainer130;
    };
    union {
        uint8_t         m_paramContainer1B8[0xF0];   // +0x1B8..+0x2A8
        uint8_t         m_interpolatorState1B8[0xF0];
    };
    union {
        uint8_t         m_paramBinding2A8[0x18];     // +0x2A8..+0x2C0
        uint8_t         m_subSystem2A8[0x18];
    };
    union {
        void*           m_binaryReaderVTable2C0;     // +0x2C0
        void*           m_pVTable2C0;
    };
    union {
        uint8_t         m_bReaderBigEndian2C8;       // +0x2C8
        uint8_t         m_bEnabled2C8;
    };
    uint8_t             m_pad2C9[3];                 // +0x2C9
    union {
        uint32_t        m_readerCursor2CC;           // +0x2CC
        uint32_t        m_flags2CC;
    };
    union {
        void*           m_pReaderData2D0;            // +0x2D0
        uint64_t        m_qword2D0;
    };
    GParamHashTable     m_loadedFileMap2D8;          // +0x2D8..+0x338
    GParamHashTable     m_blendGroupMap338;          // +0x338..+0x398
    union {
        GParamHashTable m_activeOverrideMap398;      // +0x398..+0x3F8
        GParamHashTable m_blendSubGroupLookup398;
    };
    union {
        GParamHashTable m_secondaryOverrideMap3F8;   // +0x3F8..+0x458
        GParamHashTable m_blendMap400;
    };
    union {
        uint32_t        m_nextFileSerial458;         // +0x458
        uint32_t        m_dword458;
    };
    uint32_t            m_pad45C;                    // +0x45C
    GParamHashTable     m_fileParamEditorMap460;     // +0x460..+0x4C0
    GParamHashTable     m_groupParamEditorMap4C0;    // +0x4C0..+0x520
    union {
        uint8_t         m_editorContext520[0x68];    // +0x520..+0x588
        uint8_t         m_subSystem520[0x68];
    };
    uint32_t            m_editorSlotSubGroup588[23]; // +0x588..+0x5E4
    int32_t             m_editorSlotIndex5E4[23];    // +0x5E4..+0x640
}; // 0x640 bytes

// External Engine Functions
extern "C" {
    size_t   sub_2bbe608(const char* pszStr);                                                 // strlen
    void     sub_2bbe578(void* pDst, const void* pSrc, size_t size);                          // memcpy
    void     sub_2bbe588(void* pMem);                                                         // operator delete
    void     sub_2bbe978(void* pExObj, uint32_t flag);                                        // _Throw_bad_alloc
    void     sub_2bbf1a8(const char* pszMsg);                                                 // std::_Xout_of_range
    void     sub_2bbf1b8(const char* pszMsg);                                                 // std::_Xlength_error
    uint64_t sub_20e2ff0(const char* pNarrow, uint64_t len, uint8_t* pFlag);                  // CountWideChars
    void     sub_2639420(DLWString* pDst, uint64_t pos, uint64_t count, wchar_t ch);          // DLWString::insert_fill
    void     sub_20e2a80(uint8_t* pFlag, wchar_t* pDst, uint64_t wideLen, const char* pSrc, uint64_t* pSrcLen);
    void     sub_20b55b0(uint32_t level, uint32_t line, const void* pFmt, ...);               // DebugLog
    void     sub_218ea00(GXGParamGroupLookupTable* pThis);                                    // ClearIndexLists
    GParamBucketIterator* sub_218b740(GParamBucketIterator* pOut, void* pVecContainer20, uint64_t count, const GParamBucketIterator* pVal, GParamBucketIterator posIter);
    GParamBucketIterator* sub_218b1f0(GParamBucketIterator* pOut, void* pVecContainer20, uint64_t count, const GParamBucketIterator* pVal, GParamBucketIterator posIter);
    void**   sub_e43610(void* pParamContainer, uint32_t groupId, uint8_t* pOutCount);
    void*    sub_236c900(void* pMenuNode, const void* pLabelDesc, uint32_t flags);
    void     sub_292f830(void* pMenuNode, const void* pLabelDesc, void* pBoolTarget);
    void     sub_e34680(void* pParamEditor);
    void     sub_e34e60(void* pParamEditor, void* pSubObj28, void* pBinding2A8, void* pEditorCtx520, void* pReader2C0, void* pGroupLookup130, void* pContainer1B8, const wchar_t* pDir, const wchar_t* pPat, DLAllocator* pAlloc, void* pParentMenu);
    void*    sub_236d620(void* pMenuNode, const void* pDesc);
    void     sub_235e4c0(void* pMenuNode, const void* pLabelDesc, void* pCallbackDelegate);
    void     sub_e35150(void* pParamEditor, void* pGroupPropEntry);
    void**   sub_218afd0(GParamHashTable* pMap, const uint64_t* pKey);
    void     sub_2362cf0(void* pDialogWindow, const void* pTitleDesc);
    void     sub_2328e40(void* pDialogWindow, void* pParentWindow, void* pCtx, const void* pRectDesc);
    void     sub_236c4d0(void* pScopedMenuBuilder, void* pDialogWindow);
    void     sub_231d3a0(void* pOutFormattedLabel, const wchar_t* pszFileName);
    void     sub_236c720(void* pScopedMenuBuilder);
    void     sub_e35450(void* pParamEditor, uint32_t groupId, void* pArg);
    void     sub_e36650(void* pParamEditor, uint32_t groupId, uint32_t slotIdx);
    void     sub_e36600(void* pParamEditor, uint32_t groupId, uint32_t slotIdx);
    void     sub_218cf50(void* pOutPair, GParamHashTable* pMap, const uint64_t* pKey, GParamBucketIterator nodeIter);
    void     sub_261a1b0(void* pDst, const void* pSrc, uint32_t size);
    DLAllocator* sub_2ba2b10();
    void     sub_218a630(GParamHashTable* pThis, DLAllocator** ppAlloc);
    void     sub_218a7a0(void* pSlotsThis);
    void*    sub_218a9f0(GParamHashTable* pMap, const uint32_t* pSubGroupId);
    void     sub_218ac30(GParamHashTable* pDst, const GParamHashTable* pSrc);
    void     sub_218ca90(void* pOutPair, GParamHashTable* pMap, const uint32_t* pKey, GParamBucketIterator nodeIter);
    uint64_t sub_e42e90(void* pContainer1B8, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
    uint64_t sub_e42b80(void* pContainer1B8, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
    uint64_t sub_e327d0(void* pSubObj28, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint32_t one, uint32_t flag);
    uint64_t sub_e33600(void* pSubObj28, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
    uint64_t sub_e333e0(void* pSubObj28, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint32_t one);
    void     sub_218e2b0(void* pGroupLookup130, void* pSubObj28, void* pReader);
    void     sub_e41f00(void* pContainer1B8, void* pSubObj28, uint32_t zero, uint32_t a2, uint32_t a3, float a4);
    uint64_t sub_e32510(void* pSubObj28, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);
    void     sub_e42b00(void* pContainer, uint32_t* pOutVal, uint32_t groupId, uint32_t paramId);
    void*    sub_e42aa0(void* pContainer, uint32_t groupId);
}

inline uint8_t* GetThreadLocalErrorFlag(uintptr_t addr) { return gx_guest_tls_slot(addr); }
inline void* GetThreadLocalSlot(uintptr_t addr) { return reinterpret_cast<void*>(gx_guest_tls_slot(addr)); }

// Park-Miller LCG hash helper used across all GParamManager hash maps
static inline uint64_t GParamComputeBucketIndex(uint64_t key, uint64_t bucketMask, uint64_t bucketCount) {
    int64_t masked = static_cast<int64_t>((key & 0x7FFFFFFFFFFFFFFFULL) ^ 0xDEADBEEFULL);
    int64_t q = masked / 0x1F31D;
    int64_t r = masked % 0x1F31D;
    int64_t lcg = r * 0x41A7 + q * -0xB14;
    if (lcg < 0) {
        lcg += 0x7FFFFFFFFFFFFFFFULL;
    }
    uint64_t idx = static_cast<uint64_t>(lcg) & bucketMask;
    if (bucketCount <= idx) {
        idx += ~(bucketMask >> 1);
    }
    return idx;
}

// Shared implementation for the three 1,352-byte DLVector<GParamBucketIterator>::insert_fill instantiations
static GParamBucketIterator* GParamBucketVector_InsertFill_Impl(
    GParamBucketIterator* pOutIter,
    uint8_t* pVecContainerWithOffset08,
    GParamBucketIterator* pPosNode,
    uint64_t count,
    const GParamBucketIterator* pVal)
{
    auto* pVec = reinterpret_cast<DLVector<GParamBucketIterator>*>(pVecContainerWithOffset08 + 0x08);
    GParamBucketIterator* pBegin = pVec->m_pBegin;

    if (count != 0) {
        GParamBucketIterator* pEnd = pVec->m_pEnd;
        GParamBucketIterator* pCap = pVec->m_pCapacity;
        uint64_t availCap = static_cast<uint64_t>(pCap - pEnd);

        if (availCap < count) {
            uint64_t curSize = static_cast<uint64_t>(pEnd - pBegin);
            const uint64_t kMaxSize = 0xAAAAAAAAAAAAAAAULL;
            if (kMaxSize - curSize < count) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            uint64_t reqSize = curSize + count;
            uint64_t oldCap = static_cast<uint64_t>(pCap - pBegin);
            uint64_t grownCap = (kMaxSize - (oldCap >> 1) >= oldCap) ? (oldCap + (oldCap >> 1)) : 0;
            if (grownCap < reqSize) {
                grownCap = reqSize;
            }

            DLAllocator* pAlloc = pVec->m_pAllocator;
            auto* pNewBuf = static_cast<GParamBucketIterator*>(
                pAlloc->m_pVTable->AllocateAligned(pAlloc, grownCap * sizeof(GParamBucketIterator), 8));
            if (!pNewBuf) {
                if (*GetThreadLocalErrorFlag(0x53e4318ULL) != 0) {
                    sub_2bbe978(GetThreadLocalSlot(0x53e4320ULL), 1);
                }
                sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
            }

            uint64_t prefixCount = static_cast<uint64_t>(pPosNode - pBegin);
            GParamBucketIterator* pFillDst = pNewBuf + prefixCount;
            for (uint64_t i = 0; i < count; ++i) {
                if (pFillDst) {
                    *pFillDst = *pVal;
                }
                ++pFillDst;
            }

            GParamBucketIterator* pCurSrc = pVec->m_pBegin;
            GParamBucketIterator* pCurDst = pNewBuf;
            while (pCurSrc != pPosNode) {
                if (pCurDst) {
                    *pCurDst = *pCurSrc;
                }
                ++pCurDst;
                ++pCurSrc;
            }

            GParamBucketIterator* pSuffixDst = pNewBuf + prefixCount + count;
            while (pCurSrc != pVec->m_pEnd) {
                if (pSuffixDst) {
                    *pSuffixDst = *pCurSrc;
                }
                ++pSuffixDst;
                ++pCurSrc;
            }

            uint64_t newSize = static_cast<uint64_t>(pVec->m_pEnd - pVec->m_pBegin) + count;
            if (pVec->m_pBegin) {
                pAlloc->m_pVTable->Free(pAlloc, pVec->m_pBegin);
            }
            pVec->m_pBegin = pNewBuf;
            pVec->m_pEnd = pNewBuf + newSize;
            pVec->m_pCapacity = pNewBuf + grownCap;
        } else {
            uint64_t elemsAfterPos = static_cast<uint64_t>(pEnd - pPosNode);
            GParamBucketIterator valCopy = *pVal;

            if (elemsAfterPos < count) {
                for (GParamBucketIterator* pSrc = pPosNode; pSrc != pEnd; ++pSrc) {
                    GParamBucketIterator* pDst = pSrc + count;
                    if (pDst) {
                        *pDst = *pSrc;
                    }
                }
                uint64_t uninitFillCount = count - elemsAfterPos;
                GParamBucketIterator* pUninitDst = pVec->m_pEnd;
                for (uint64_t i = 0; i < uninitFillCount; ++i) {
                    if (pUninitDst) {
                        *pUninitDst = valCopy;
                    }
                    ++pUninitDst;
                }
                GParamBucketIterator* pOldEnd = pVec->m_pEnd;
                pVec->m_pEnd = pOldEnd + count;
                for (GParamBucketIterator* pCur = pPosNode; pCur != pOldEnd; ++pCur) {
                    *pCur = valCopy;
                }
            } else {
                GParamBucketIterator* pTailSrc = pEnd - count;
                for (uint64_t i = 0; i < count; ++i) {
                    if (pEnd + i) {
                        pEnd[i] = pTailSrc[i];
                    }
                }
                pVec->m_pEnd = pEnd + count;
                for (GParamBucketIterator* pDst = pEnd - 1; pTailSrc != pPosNode; --pDst) {
                    --pTailSrc;
                    *pDst = *pTailSrc;
                }
                for (uint64_t i = 0; i < count; ++i) {
                    pPosNode[i] = valCopy;
                }
            }
        }
    }

    uint64_t offsetIdx = static_cast<uint64_t>(pPosNode - pBegin);
    pOutIter->m_pListContainer00 = nullptr;
    pOutIter->m_pReserved08 = nullptr;
    pOutIter->m_pNode10 = pVec->m_pBegin + offsetIdx;
    return pOutIter;
}

// ============================================================================
// 1. 0x2189f30 (224 B) - GParamWString_AppendConvertedAsciiString
// Converts a narrow DLString (`pSrc`) to wide characters and appends to `pDst`.
// ============================================================================
void GParamWString_AppendConvertedAsciiString(DLWString* pDst, const DLString* pSrc) {
    const char* pNarrowChars = (pSrc->m_capacity >= 16)
        ? pSrc->m_data.m_pHeapBuf
        : pSrc->m_data.m_inlineBuf;
    uint64_t narrowLen = pSrc->m_length;
    uint8_t modeFlag = pSrc->m_bOwnsMemory;
    uint64_t oldDstLen = pDst->m_length;

    if (narrowLen == 0xFFFFFFFFFFFFFFFFULL) {
        narrowLen = sub_2bbe608(pNarrowChars);
    }
    if (narrowLen != 0) {
        if (modeFlag == 1) {
            modeFlag = pDst->m_bOwnsMemory;
        }
        uint8_t countFlag = modeFlag;
        uint64_t wideCount = sub_20e2ff0(pNarrowChars, narrowLen, &countFlag);
        sub_2639420(pDst, oldDstLen, wideCount, L'\0');

        wchar_t* pDstBuf = (pDst->m_capacity >= 8)
            ? pDst->m_data.m_pHeapBuf
            : pDst->m_data.m_inlineBuf;
        uint8_t convFlag = modeFlag;
        uint64_t srcLenCopy = narrowLen;
        sub_20e2a80(&convFlag, pDstBuf + oldDstLen, wideCount, pNarrowChars, &srcLenCopy);
    }
}

// ============================================================================
// 2. 0x21899e0 (1,352 B) - GParamBucketVector_InsertFill_3B8
// Inserts `count` copies of `pVal` (24-byte GParamBucketIterator) into m_buckets.
// ============================================================================
GParamBucketIterator* GParamBucketVector_InsertFill_3B8(
    GParamBucketIterator* pOutIter,
    uint8_t* pVecContainer,
    uint64_t count,
    const GParamBucketIterator* pVal,
    GParamBucketIterator posIter)
{
    return GParamBucketVector_InsertFill_Impl(
        pOutIter,
        pVecContainer,
        static_cast<GParamBucketIterator*>(posIter.m_pNode10),
        count,
        pVal);
}

// ============================================================================
// 3. 0x2189490 (1,352 B) - GParamBucketVector_InsertFill_2F8
// Inserts `count` copies of `pVal` (24-byte GParamBucketIterator) into m_buckets.
// ============================================================================
GParamBucketIterator* GParamBucketVector_InsertFill_2F8(
    GParamBucketIterator* pOutIter,
    uint8_t* pVecContainer,
    uint64_t count,
    const GParamBucketIterator* pVal,
    GParamBucketIterator posIter)
{
    return GParamBucketVector_InsertFill_Impl(
        pOutIter,
        pVecContainer,
        static_cast<GParamBucketIterator*>(posIter.m_pNode10),
        count,
        pVal);
}

// ============================================================================
// 4. 0x2189310 (379 B) - GParamLoadedFileMap_EraseNode
// Unlinks a 0x438-byte GParamLoadedFileNode from the bucket table, destroys
// its embedded GParamLoadedFileEntry (+0x18), and frees the node.
// ============================================================================
void GParamLoadedFileEntry_Destroy(GParamLoadedFileEntry* pThis); // 0x2188a80

GParamBucketIterator* GParamLoadedFileMap_EraseNode(
    GParamBucketIterator* pOutNext,
    GParamHashTable* pMap,
    GParamLoadedFileNode* pNode)
{
    uint64_t bucketIdx = GParamComputeBucketIndex(
        pNode->m_fileKey,
        pMap->m_bucketMask48,
        pMap->m_bucketCount50);

    GParamBucketIterator* pBuckets = pMap->m_buckets28.m_pBegin;
    GParamBucketIterator* pBucketBegin = &pBuckets[bucketIdx * 2];
    GParamBucketIterator* pBucketEnd = &pBuckets[bucketIdx * 2 + 1];

    if (pBucketEnd->m_pNode10 == pNode) {
        if (pBucketBegin->m_pNode10 == pNode) {
            pBucketBegin->m_pNode10 = pMap->m_pHeadNode08;
            pBucketEnd->m_pNode10 = pMap->m_pHeadNode08;
        } else {
            pBucketEnd->m_pNode10 = pNode->m_pPrev;
        }
    } else if (pBucketBegin->m_pNode10 == pNode) {
        pBucketBegin->m_pNode10 = pNode->m_pNext;
    }

    GParamLoadedFileNode* pNext = pNode->m_pNext;
    if (pNode != static_cast<GParamLoadedFileNode*>(pMap->m_pHeadNode08)) {
        pNode->m_pPrev->m_pNext = pNext;
        pNext->m_pPrev = pNode->m_pPrev;
        GParamLoadedFileEntry_Destroy(&pNode->m_entry);
        pMap->m_pNodeAlloc18->m_pVTable->Free(pMap->m_pNodeAlloc18, pNode);
        pMap->m_nodeCount10--;
    }

    pOutNext->m_pNode10 = pNext;
    return pOutNext;
}

// ============================================================================
// 5. 0x2188dc0 (1,352 B) - GParamBucketVector_InsertFill_480
// Inserts `count` copies of `pVal` (24-byte GParamBucketIterator) into m_buckets.
// ============================================================================
GParamBucketIterator* GParamBucketVector_InsertFill_480(
    GParamBucketIterator* pOutIter,
    uint8_t* pVecContainer,
    uint64_t count,
    const GParamBucketIterator* pVal,
    GParamBucketIterator posIter)
{
    return GParamBucketVector_InsertFill_Impl(
        pOutIter,
        pVecContainer,
        static_cast<GParamBucketIterator*>(posIter.m_pNode10),
        count,
        pVal);
}

// ============================================================================
// 6. 0x2188ca0 (276 B) - GXGParamGroupLookupTable_Destroy
// Clears parameter value index lists via 0x218ea00 and frees the two
// DLVector<DLWString> arrays at +0x58 and +0x30.
// ============================================================================
void GXGParamGroupLookupTable_Destroy(GXGParamGroupLookupTable* pThis) {
    sub_218ea00(pThis);

    if (pThis->m_vec58.m_pBegin) {
        for (DLWString* pCur = pThis->m_vec58.m_pBegin; pCur != pThis->m_vec58.m_pEnd; ++pCur) {
            if (pCur->m_capacity >= 8) {
                pCur->m_pAllocator->m_pVTable->Free(pCur->m_pAllocator, pCur->m_data.m_pHeapBuf);
            }
            pCur->m_capacity = 7;
            pCur->m_length = 0;
            pCur->m_data.m_inlineBuf[0] = L'\0';
        }
        pThis->m_vec58.m_pAllocator->m_pVTable->Free(
            pThis->m_vec58.m_pAllocator,
            pThis->m_vec58.m_pBegin);
        pThis->m_vec58.m_pCapacity = nullptr;
        pThis->m_vec58.m_pEnd = nullptr;
        pThis->m_vec58.m_pBegin = nullptr;
    }

    if (pThis->m_vec30.m_pBegin) {
        for (DLWString* pCur = pThis->m_vec30.m_pBegin; pCur != pThis->m_vec30.m_pEnd; ++pCur) {
            if (pCur->m_capacity >= 8) {
                pCur->m_pAllocator->m_pVTable->Free(pCur->m_pAllocator, pCur->m_data.m_pHeapBuf);
            }
            pCur->m_capacity = 7;
            pCur->m_length = 0;
            pCur->m_data.m_inlineBuf[0] = L'\0';
        }
        pThis->m_vec30.m_pAllocator->m_pVTable->Free(
            pThis->m_vec30.m_pAllocator,
            pThis->m_vec30.m_pBegin);
        pThis->m_vec30.m_pCapacity = nullptr;
        pThis->m_vec30.m_pEnd = nullptr;
        pThis->m_vec30.m_pBegin = nullptr;
    }
}

// ============================================================================
// 7. 0x2188a80 (532 B) - GParamLoadedFileEntry_Destroy
// Destroys the 5 DLFixedWString<64> paths (+0x348, +0x290, +0x1E0, +0x130, +0x80)
// and 2 DLWStrings (+0x48, +0x10) inside GParamLoadedFileEntry.
// ============================================================================
static inline void DestroyFixedWString64(DLFixedWString<64>* pStr) {
    if (pStr->m_capacity >= 8) {
        if (pStr->m_capacity + 1 >= 0x41) {
            sub_20b55b0(GX_VMA(0x4940298ULL), 0x120, reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        pStr->m_flagsA9 &= 0xFDu;
    }
    pStr->m_capacity = 7;
    pStr->m_length = 0;
    pStr->m_data.m_inlineBuf[0] = L'\0';
}

void GParamLoadedFileEntry_Destroy(GParamLoadedFileEntry* pThis) {
    DestroyFixedWString64(&pThis->m_customOverridePath348);
    DestroyFixedWString64(&pThis->m_slotPath3_290);
    DestroyFixedWString64(&pThis->m_slotPath2_1E0);
    DestroyFixedWString64(&pThis->m_slotPath1_130);
    DestroyFixedWString64(&pThis->m_slotPath0_80);

    if (pThis->m_secondaryName48.m_capacity >= 8) {
        pThis->m_secondaryName48.m_pAllocator->m_pVTable->Free(
            pThis->m_secondaryName48.m_pAllocator,
            pThis->m_secondaryName48.m_data.m_pHeapBuf);
    }
    pThis->m_secondaryName48.m_capacity = 7;
    pThis->m_secondaryName48.m_length = 0;
    pThis->m_secondaryName48.m_data.m_inlineBuf[0] = L'\0';

    if (pThis->m_fileName10.m_capacity >= 8) {
        pThis->m_fileName10.m_pAllocator->m_pVTable->Free(
            pThis->m_fileName10.m_pAllocator,
            pThis->m_fileName10.m_data.m_pHeapBuf);
    }
    pThis->m_fileName10.m_capacity = 7;
    pThis->m_fileName10.m_length = 0;
    pThis->m_fileName10.m_data.m_inlineBuf[0] = L'\0';
}

// ============================================================================
// 8. 0x2188910 (366 B) - GParamHashTable_Construct_4C0
// Initializes GParamHashTable at +0x4C0 with sentinel node and 16 bucket slots.
// ============================================================================
void GParamHashTable_Construct_4C0(GParamHashTable* pThis, DLAllocator** ppAlloc) {
    pThis->m_nodeCount10 = 0;
    pThis->m_pHeadNode08 = nullptr;
    DLAllocator* pAlloc = *ppAlloc;
    pThis->m_pNodeAlloc18 = pAlloc;

    auto* pHead = static_cast<GParamKeyValueNode*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x20, 8));
    if (pHead) {
        pHead->m_pNext = pHead;
    } else {
        if (*GetThreadLocalErrorFlag(0x53e4318ULL) != 0) {
            sub_2bbe978(GetThreadLocalSlot(0x53e4320ULL), 1);
        }
        sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }
    pHead->m_pPrev = pHead;
    pThis->m_pHeadNode08 = pHead;

    pThis->m_buckets28.m_pCapacity = nullptr;
    pThis->m_buckets28.m_pEnd = nullptr;
    pThis->m_buckets28.m_pBegin = nullptr;
    pThis->m_buckets28.m_pAllocator = *ppAlloc;
    pThis->m_maxLoadFactor58 = 1.0f;

    GParamBucketIterator fillVal = {nullptr, nullptr, pHead};
    GParamBucketIterator posIter = {nullptr, nullptr, nullptr};
    GParamBucketIterator outIter;
    sub_218b740(&outIter, reinterpret_cast<uint8_t*>(pThis) + 0x20, 0x10, &fillVal, posIter);

    pThis->m_bucketMask48 = 7;
    pThis->m_bucketCount50 = 8;
}

// ============================================================================
// 9. 0x21887a0 (366 B) - GParamHashTable_Construct_460
// Initializes GParamHashTable at +0x460 with sentinel node and 16 bucket slots.
// ============================================================================
void GParamHashTable_Construct_460(GParamHashTable* pThis, DLAllocator** ppAlloc) {
    pThis->m_nodeCount10 = 0;
    pThis->m_pHeadNode08 = nullptr;
    DLAllocator* pAlloc = *ppAlloc;
    pThis->m_pNodeAlloc18 = pAlloc;

    auto* pHead = static_cast<GParamKeyValueNode*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x20, 8));
    if (pHead) {
        pHead->m_pNext = pHead;
    } else {
        if (*GetThreadLocalErrorFlag(0x53e4318ULL) != 0) {
            sub_2bbe978(GetThreadLocalSlot(0x53e4320ULL), 1);
        }
        sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }
    pHead->m_pPrev = pHead;
    pThis->m_pHeadNode08 = pHead;

    pThis->m_buckets28.m_pCapacity = nullptr;
    pThis->m_buckets28.m_pEnd = nullptr;
    pThis->m_buckets28.m_pBegin = nullptr;
    pThis->m_buckets28.m_pAllocator = *ppAlloc;
    pThis->m_maxLoadFactor58 = 1.0f;

    GParamBucketIterator fillVal = {nullptr, nullptr, pHead};
    GParamBucketIterator posIter = {nullptr, nullptr, nullptr};
    GParamBucketIterator outIter;
    GParamBucketVector_InsertFill_480(&outIter, reinterpret_cast<uint8_t*>(pThis) + 0x20, 0x10, &fillVal, posIter);

    pThis->m_bucketMask48 = 7;
    pThis->m_bucketCount50 = 8;
}

// ============================================================================
// 10. 0x2188630 (366 B) - GParamHashTable_Construct_398
// Initializes GParamHashTable at +0x398 / +0x3F8 with sentinel node and 16 slots.
// ============================================================================
void GParamHashTable_Construct_398(GParamHashTable* pThis, DLAllocator** ppAlloc) {
    pThis->m_nodeCount10 = 0;
    pThis->m_pHeadNode08 = nullptr;
    DLAllocator* pAlloc = *ppAlloc;
    pThis->m_pNodeAlloc18 = pAlloc;

    auto* pHead = static_cast<GParamKeyValueNode*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x20, 8));
    if (pHead) {
        pHead->m_pNext = pHead;
    } else {
        if (*GetThreadLocalErrorFlag(0x53e4318ULL) != 0) {
            sub_2bbe978(GetThreadLocalSlot(0x53e4320ULL), 1);
        }
        sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }
    pHead->m_pPrev = pHead;
    pThis->m_pHeadNode08 = pHead;

    pThis->m_buckets28.m_pCapacity = nullptr;
    pThis->m_buckets28.m_pEnd = nullptr;
    pThis->m_buckets28.m_pBegin = nullptr;
    pThis->m_buckets28.m_pAllocator = *ppAlloc;
    pThis->m_maxLoadFactor58 = 1.0f;

    GParamBucketIterator fillVal = {nullptr, nullptr, pHead};
    GParamBucketIterator posIter = {nullptr, nullptr, nullptr};
    GParamBucketIterator outIter;
    GParamBucketVector_InsertFill_3B8(&outIter, reinterpret_cast<uint8_t*>(pThis) + 0x20, 0x10, &fillVal, posIter);

    pThis->m_bucketMask48 = 7;
    pThis->m_bucketCount50 = 8;
}

// ============================================================================
// 11. 0x21880a0 (1,409 B) - GParamInterpolationWorkerJob_ExecuteRange
// Worker task callback that steps interpolation timers and blends parameter
// values across all 17 override slots for targets in [pRange[0], pRange[1]).
// ============================================================================
void GParamInterpolationWorkerJob_ExecuteRange(
    GParamInterpolationWorkerJob* pThis,
    const int32_t* pRange)
{
    int64_t idx = pRange[0];
    int32_t endIdx = pRange[1];
    if (idx >= endIdx) {
        return;
    }

    for (; idx < endIdx; ++idx) {
        uint64_t vecCount = static_cast<uint64_t>(pThis->m_targets10.m_pEnd - pThis->m_targets10.m_pBegin);
        if (static_cast<uint64_t>(idx) >= vecCount) {
            sub_2bbf1a8("vector<T> too long");
        }

        GParamActiveOverrideTarget* pTarget = pThis->m_targets10.m_pBegin[idx];
        if (!pTarget->m_bActive28) {
            continue;
        }

        float targetBlendFactor = 1.0f;
        float duration = pTarget->m_blendDuration10;
        if (pTarget->m_bIsBlending09) {
            float elapsed = pTarget->m_blendElapsed0C + pThis->m_deltaTime40;
            pTarget->m_blendElapsed0C = elapsed;
            if (elapsed > duration) {
                pTarget->m_blendElapsed0C = 0.0f;
                pTarget->m_blendDuration10 = 0.0f;
                pTarget->m_bIsBlending09 = false;
                targetBlendFactor = 1.0f;
                goto check_fixed_blend;
            }
        }
        if (duration > 0.0f) {
            float ratio = pTarget->m_blendElapsed0C / duration;
            if (ratio >= 0.0f) {
                targetBlendFactor = (ratio < 1.0f) ? ratio : 1.0f;
            } else {
                targetBlendFactor = 0.0f;
            }
        }

    check_fixed_blend:
        if (pTarget->m_bUseFixedBlendFactor14) {
            targetBlendFactor = pTarget->m_fixedBlendFactor18;
        }

        uint32_t groupId = pTarget->m_groupId1C;
        uint32_t instanceParamId = pTarget->m_instanceParamId24;
        GParamHashTable* pGroupMap = pThis->m_pBlendGroupMap30;
        uint64_t groupBucketIdx = GParamComputeBucketIndex(groupId, pGroupMap->m_bucketMask48, pGroupMap->m_bucketCount50);

        auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pGroupMap->m_pHeadNode08);
        GParamBucketIterator* pGroupBuckets = pGroupMap->m_buckets28.m_pBegin;
        auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucketIdx * 2].m_pNode10);
        GParamBlendGroupNode* pGroupNode = pGroupHead;

        if (pGroupCur != pGroupHead) {
            auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucketIdx * 2 + 1].m_pNode10)->m_pNext;
            while (pGroupCur != pGroupEnd) {
                if (pGroupCur->m_groupId >= groupId) {
                    if (groupId >= pGroupCur->m_groupId) {
                        pGroupNode = pGroupCur;
                    }
                    break;
                }
                pGroupCur = pGroupCur->m_pNext;
            }
        }

        if (pGroupNode == static_cast<GParamBlendGroupNode*>(pThis->m_pBlendGroupMap30->m_pHeadNode08)) {
            continue;
        }

        uint32_t subGroupId = pTarget->m_subGroupId20;
        GParamHashTable* pSubMap = &pGroupNode->m_subGroupMap18;
        uint64_t subBucketIdx = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);

        auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
        GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
        auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucketIdx * 2].m_pNode10);
        if (pSubCur != pSubHead) {
            auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucketIdx * 2 + 1].m_pNode10)->m_pNext;
            while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
                pSubCur = pSubCur->m_pNext;
            }
            if (pSubCur == pSubEnd || subGroupId < pSubCur->m_subGroupId) {
                pSubCur = pSubHead;
            }
        }
        if (pSubCur == pSubHead) {
            continue;
        }

        uint64_t anyAppliedFlag = 0;
        for (uint64_t slotIdx = 0; slotIdx < 17; ++slotIdx) {
            const GParamBlendSlotState& slotState = pSubCur->m_slots[slotIdx];
            uint64_t fileKey = slotState.m_targetFileKey;
            if (fileKey == 0xFFFFFFFFFFFFFFFFULL) {
                continue;
            }

            bool bSlotInterpolating = slotState.m_bInterpolating;
            float slotBlendFactor = 1.0f;
            if (slotState.m_blendDuration > 0.0f) {
                float ratio = slotState.m_elapsedTime / slotState.m_blendDuration;
                if (ratio >= 0.0f) {
                    slotBlendFactor = (ratio < 1.0f) ? ratio : 1.0f;
                } else {
                    slotBlendFactor = 0.0f;
                }
            }

            GParamHashTable* pFileMap = pThis->m_pLoadedFileMap38;
            uint64_t fileBucketIdx = GParamComputeBucketIndex(fileKey, pFileMap->m_bucketMask48, pFileMap->m_bucketCount50);

            auto* pFileHead = static_cast<GParamLoadedFileNode*>(pFileMap->m_pHeadNode08);
            GParamBucketIterator* pFileBuckets = pFileMap->m_buckets28.m_pBegin;
            auto* pFileCur = static_cast<GParamLoadedFileNode*>(pFileBuckets[fileBucketIdx * 2].m_pNode10);
            GParamLoadedFileNode* pFileNode = pFileHead;

            if (pFileCur != pFileHead) {
                auto* pFileEnd = static_cast<GParamLoadedFileNode*>(pFileBuckets[fileBucketIdx * 2 + 1].m_pNode10)->m_pNext;
                while (pFileCur != pFileEnd) {
                    if (pFileCur->m_fileKey >= fileKey) {
                        if (fileKey >= pFileCur->m_fileKey) {
                            pFileNode = pFileCur;
                        }
                        break;
                    }
                    pFileCur = pFileCur->m_pNext;
                }
            }

            if (pFileNode == static_cast<GParamLoadedFileNode*>(pThis->m_pLoadedFileMap38->m_pHeadNode08)) {
                continue;
            }

            uint8_t handlerCount = 0;
            void** ppHandlers = sub_e43610(pFileNode->m_entry.m_pParamContainer400, pTarget->m_groupId1C, &handlerCount);
            if (!ppHandlers) {
                continue;
            }

            if (pTarget->m_bNeedsImmediateSync38) {
                int32_t propCount = pTarget->m_propertyCount2C;
                for (int32_t p = 0; p < propCount; ++p) {
                    GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
                    if (prop.m_pBackupValuePtr18 && prop.m_pLiveValuePtr10 && prop.m_bEnabled24 && prop.m_paramIndex08 < handlerCount) {
                        void* pHandler = ppHandlers[prop.m_paramIndex08];
                        if (pHandler) {
                            using SyncFn = void(*)(void*, uint32_t, void*);
                            auto pfnSync = *reinterpret_cast<SyncFn*>(*reinterpret_cast<uintptr_t*>(pHandler) + 0x68);
                            pfnSync(pHandler, instanceParamId, prop.m_pLiveValuePtr10);
                            anyAppliedFlag = 1;
                        }
                    }
                }
            }

            for (uint32_t p = 0; p < static_cast<uint32_t>(pTarget->m_propertyCount2C); ++p) {
                GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
                if (prop.m_pBackupValuePtr18 && prop.m_pLiveValuePtr10 && prop.m_bEnabled24 && prop.m_paramIndex08 < handlerCount) {
                    void* pHandler = ppHandlers[prop.m_paramIndex08];
                    if (pHandler) {
                        using BlendFn = void(*)(void*, uint32_t, void*, void*, float);
                        auto pfnBlend = *reinterpret_cast<BlendFn*>(*reinterpret_cast<uintptr_t*>(pHandler) + 0x88);
                        if (!bSlotInterpolating) {
                            pfnBlend(pHandler, instanceParamId, prop.m_pBackupValuePtr18, prop.m_pLiveValuePtr10, targetBlendFactor);
                        } else {
                            pfnBlend(pHandler, instanceParamId, prop.m_pBackupValuePtr18, prop.m_pLiveValuePtr10, slotBlendFactor);
                            anyAppliedFlag = 1;
                        }
                    }
                }
            }

            pTarget->m_bDirty40 = (anyAppliedFlag & 1) != 0;
            pTarget->m_bNeedsImmediateSync38 = false;
        }
    }
}

// ============================================================================
// 12. 0x2188060 (52 B) - GParamInterpolationWorkerJob_DeletingDestructor
// Frees the target pointer vector buffer and deletes `pThis`.
// ============================================================================
void GParamInterpolationWorkerJob_DeletingDestructor(GParamInterpolationWorkerJob* pThis) {
    pThis->m_pVTable00 = reinterpret_cast<void**>(GX_VMA(0x535f4f0ULL));
    if (pThis->m_targets10.m_pBegin) {
        pThis->m_targets10.m_pAllocator->m_pVTable->Free(
            pThis->m_targets10.m_pAllocator,
            pThis->m_targets10.m_pBegin);
    }
    sub_2bbe588(pThis);
}

// ============================================================================
// 13. 0x2188010 (72 B) - GParamInterpolationWorkerJob_Destructor
// Non-deleting destructor for GParamInterpolationWorkerJob.
// ============================================================================
void GParamInterpolationWorkerJob_Destructor(GParamInterpolationWorkerJob* pThis) {
    pThis->m_pVTable00 = reinterpret_cast<void**>(GX_VMA(0x535f4f0ULL));
    if (pThis->m_targets10.m_pBegin) {
        pThis->m_targets10.m_pAllocator->m_pVTable->Free(
            pThis->m_targets10.m_pAllocator,
            pThis->m_targets10.m_pBegin);
        pThis->m_targets10.m_pCapacity = nullptr;
        pThis->m_targets10.m_pEnd = nullptr;
        pThis->m_targets10.m_pBegin = nullptr;
    }
}

// ============================================================================
// 14..19. 0x2187f30 .. 0x2187fc0 - GParamMemberCallback delegates
// ============================================================================
struct GParamMemberCallback3 {
    void*    m_pVTable00;     // +0x00 (0x53b63b0 / 0x53b93b0)
    uint32_t m_flags08;       // +0x08
    uint32_t m_pad0C;         // +0x0C
    void*    m_pTargetObj10;  // +0x10
    uint64_t m_arg18;         // +0x18
    uint64_t m_arg20;         // +0x20
    uint64_t m_arg28;         // +0x28
    uint64_t m_pfnMember30;   // +0x30
    int64_t  m_thisDelta38;   // +0x38
}; // 0x40 bytes

struct GParamMemberCallback1 {
    void*    m_pVTable00;     // +0x00 (0x53b6370 / 0x53b93b0)
    uint32_t m_flags08;       // +0x08
    uint32_t m_pad0C;         // +0x0C
    void*    m_pTargetObj10;  // +0x10
    uint32_t m_arg18;         // +0x18
    uint32_t m_pad1C;         // +0x1C
    uint64_t m_pfnMember20;   // +0x20
    int64_t  m_thisDelta28;   // +0x28
}; // 0x30 bytes

// 14. 0x2187fc0 (76 B) - GParamMemberCallback3_Invoke
void GParamMemberCallback3_Invoke(GParamMemberCallback3* pThis, void* /*pMenuEventArg*/) {
    if ((pThis->m_flags08 & 0x40000000u) != 0) {
        uint8_t* pAdjustedThis = static_cast<uint8_t*>(pThis->m_pTargetObj10) + pThis->m_thisDelta38;
        uint64_t fnAddr = pThis->m_pfnMember30;
        if ((fnAddr & 1ULL) != 0) {
            uintptr_t vtableAddr = *reinterpret_cast<uintptr_t*>(pAdjustedThis);
            fnAddr = *reinterpret_cast<uint64_t*>(vtableAddr + fnAddr - 1);
        }
        struct ByValPayload24 {
            uint64_t a0;
            uint64_t a1;
            uint64_t a2;
        };
        using FnType = void(*)(void*, ByValPayload24);
        reinterpret_cast<FnType>(fnAddr)(
            pAdjustedThis, ByValPayload24{pThis->m_arg18, pThis->m_arg20, pThis->m_arg28});
    }
}

// 15. 0x2187fb0 (5 B) - GParamMemberCallback3_DeletingDestructor
void GParamMemberCallback3_DeletingDestructor(GParamMemberCallback3* pThis) {
    sub_2bbe588(pThis);
}

// 16. 0x2187f90 (22 B) - GParamMemberCallback3_Destructor
void GParamMemberCallback3_Destructor(GParamMemberCallback3* pThis) {
    uint32_t newFlags = pThis->m_flags08 & 0xBFFFFFFFu;
    pThis->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b93b0ULL));
    pThis->m_flags08 = newFlags;
}

// 17. 0x2187f60 (40 B) - GParamMemberCallback1_Invoke
void GParamMemberCallback1_Invoke(GParamMemberCallback1* pThis) {
    if ((pThis->m_flags08 & 0x40000000u) != 0) {
        uint8_t* pAdjustedThis = static_cast<uint8_t*>(pThis->m_pTargetObj10) + pThis->m_thisDelta28;
        uint64_t fnAddr = pThis->m_pfnMember20;
        if ((fnAddr & 1ULL) != 0) {
            uintptr_t vtableAddr = *reinterpret_cast<uintptr_t*>(pAdjustedThis);
            fnAddr = *reinterpret_cast<uint64_t*>(vtableAddr + fnAddr - 1);
        }
        using FnType = void(*)(void*, uint32_t);
        reinterpret_cast<FnType>(fnAddr)(pAdjustedThis, pThis->m_arg18);
    }
}

// 18. 0x2187f50 (5 B) - GParamMemberCallback1_DeletingDestructor
void GParamMemberCallback1_DeletingDestructor(GParamMemberCallback1* pThis) {
    sub_2bbe588(pThis);
}

// 19. 0x2187f30 (22 B) - GParamMemberCallback1_Destructor
void GParamMemberCallback1_Destructor(GParamMemberCallback1* pThis) {
    uint32_t newFlags = pThis->m_flags08 & 0xBFFFFFFFu;
    pThis->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b93b0ULL));
    pThis->m_flags08 = newFlags;
}

// ============================================================================
// 20. 0x2187d10 (394 B) - GParamManager_RegisterRootDebugMenuProperties
// Registers "Open ParamEditor List", "Enable MultiThread", and "Enable ForceUpdate"
// properties on the debug GUI tree node, then releases the top menu stack ref.
// ============================================================================
struct GXDebugMenuLabelDesc {
    uint64_t       m_refObj00;   // +0x00
    const wchar_t* m_pszText08;  // +0x08
};

struct GXDebugMenuRefCountedItem {
    void (**m_pVTable00)(GXDebugMenuRefCountedItem*); // +0x00
    uint32_t m_refFlags08;                            // +0x08
};

struct GXDebugMenuContextStack {
    void*                       m_pVTable00; // +0x00
    GXDebugMenuRefCountedItem** m_ppBegin08; // +0x08
    GXDebugMenuRefCountedItem** m_ppEnd10;   // +0x10
};

void GParamManager_RegisterRootDebugMenuProperties(
    GParamManager* pThis,
    GXDebugMenuContextStack* pMenuStack)
{
    GXDebugMenuLabelDesc groupDesc = {0, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2b42ULL))};
    void* pSubGroup = sub_236c900(pMenuStack, &groupDesc, 0);
    groupDesc.m_pszText08 = nullptr;

    if (pSubGroup) {
        GXDebugMenuLabelDesc mtDesc = {0, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2b5eULL))};
        sub_292f830(pMenuStack, &mtDesc, &pThis->m_bEnableMultiThread128);
        mtDesc.m_pszText08 = nullptr;

        GXDebugMenuLabelDesc forceDesc = {0, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2b84ULL))};
        sub_292f830(pMenuStack, &forceDesc, reinterpret_cast<void*>(GX_VMA(0x56a6bc0ULL)));
        forceDesc.m_pszText08 = nullptr;

        if (pMenuStack->m_ppBegin08 != pMenuStack->m_ppEnd10) {
            GXDebugMenuRefCountedItem** ppTop = pMenuStack->m_ppEnd10 - 1;
            GXDebugMenuRefCountedItem* pItem = *ppTop;
            if (pItem) {
                uint32_t oldFlags = pItem->m_refFlags08;
                uint32_t newStrong = (oldFlags + 0x7FFFu) & 0x7FFFu;
                uint32_t updatedFlags = (oldFlags & 0xFFFF8000u) | newStrong;
                pItem->m_refFlags08 = updatedFlags;
                if (newStrong == 0 && (oldFlags & 0x40000000u) != 0) {
                    pItem->m_pVTable00[3](pItem); // vtable +0x18: OnLastStrongRef
                    updatedFlags = pItem->m_refFlags08 & 0xBFFFFFFFu;
                    pItem->m_refFlags08 = updatedFlags;
                }
                uint32_t newWeak = ((updatedFlags >> 15) + 0x7FFFu) & 0x7FFFu;
                pItem->m_refFlags08 = (updatedFlags & 0xC0007FFFu) | (newWeak << 15);
                if (newWeak == 0) {
                    pItem->m_refFlags08 = updatedFlags | 0x3FFF8000u;
                    pItem->m_pVTable00[4](pItem); // vtable +0x20: DestroySelf
                }
                *ppTop = nullptr;
                ppTop = pMenuStack->m_ppEnd10 - 1;
            }
            pMenuStack->m_ppEnd10 = ppTop;
        }
    }
}

// ============================================================================
// 21. 0x21877f0 (1,305 B) - GParamManager_OpenParamEditorForFileAndGroup
// Creates or focuses the 0x140-byte ParamEditor dialog for `(groupIndex, subId)`
// and registers the `"Select GParam File"` callback button (`0x2186f10`).
// ============================================================================
void GParamManager_BuildFileSelectionDialogMenu(GParamManager* pThis, uint32_t groupIndex); // 0x2186f10

void GParamManager_OpenParamEditorForFileAndGroup(
    GParamManager* pThis,
    GXDebugMenuContextStack* pMenuStack,
    uint32_t groupIndex,
    uint32_t subId)
{
    uint64_t combinedKey = (static_cast<uint64_t>(groupIndex) << 32) | static_cast<uint64_t>(subId);
    uint64_t bucketIdx = GParamComputeBucketIndex(
        combinedKey,
        pThis->m_fileParamEditorMap460.m_bucketMask48,
        pThis->m_fileParamEditorMap460.m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    GParamBucketIterator* pBuckets = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    GParamKeyValueNode* pFound = pHead;

    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd) {
            if (pCur->m_key10 >= combinedKey) {
                if (combinedKey >= pCur->m_key10) {
                    pFound = pCur;
                }
                break;
            }
            pCur = pCur->m_pNext;
        }
    }

    const wchar_t* pszDir = (pThis->m_defaultDirPath90.m_capacity >= 8)
        ? pThis->m_defaultDirPath90.m_data.m_pHeapBuf
        : pThis->m_defaultDirPath90.m_data.m_inlineBuf;
    const wchar_t* pszPat = (pThis->m_defaultSearchPatternC8.m_capacity >= 8)
        ? pThis->m_defaultSearchPatternC8.m_data.m_pHeapBuf
        : pThis->m_defaultSearchPatternC8.m_data.m_inlineBuf;

    uint8_t* pParentMenu = *reinterpret_cast<uint8_t**>(pMenuStack->m_ppEnd10 - 1);
    while (pParentMenu && (pParentMenu[0x50] & 0x02) == 0) {
        pParentMenu = *reinterpret_cast<uint8_t**>(pParentMenu + 0x10);
    }

    if (pFound != pHead) {
        sub_e34e60(
            pFound->m_pVal18,
            pThis->m_pSubObject28,
            pThis->m_paramBinding2A8,
            pThis->m_editorContext520,
            &pThis->m_binaryReaderVTable2C0,
            pThis->m_groupLookup130,
            pThis->m_paramContainer1B8,
            pszDir,
            pszPat,
            pThis->m_pAllocator00,
            pParentMenu);
    } else {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        void* pNewEditor = pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x140, 8);
        if (!pNewEditor) {
            return;
        }
        sub_e34680(pNewEditor);
        sub_e34e60(
            pNewEditor,
            pThis->m_pSubObject28,
            pThis->m_paramBinding2A8,
            pThis->m_editorContext520,
            &pThis->m_binaryReaderVTable2C0,
            pThis->m_groupLookup130,
            pThis->m_paramContainer1B8,
            pszDir,
            pszPat,
            pThis->m_pAllocator00,
            pParentMenu);

        uint64_t groupBucket = GParamComputeBucketIndex(
            groupIndex,
            pThis->m_groupParamEditorMap4C0.m_bucketMask48,
            pThis->m_groupParamEditorMap4C0.m_bucketCount50);
        auto* pGroupHead = static_cast<GParamKeyValueNode*>(pThis->m_groupParamEditorMap4C0.m_pHeadNode08);
        GParamBucketIterator* pGroupBuckets = pThis->m_groupParamEditorMap4C0.m_buckets28.m_pBegin;
        auto* pGroupCur = static_cast<GParamKeyValueNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
        if (pGroupCur != pGroupHead) {
            auto* pGroupEnd = static_cast<GParamKeyValueNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
            while (pGroupCur != pGroupEnd && pGroupCur->m_key10 < groupIndex) {
                pGroupCur = pGroupCur->m_pNext;
            }
            if (pGroupCur != pGroupEnd && groupIndex >= pGroupCur->m_key10) {
                auto* pPropVec = reinterpret_cast<DLVector<uint8_t>*>(static_cast<uint8_t*>(pGroupCur->m_pVal18) + 0x08);
                uint64_t elemCount = static_cast<uint64_t>(pPropVec->m_pEnd - pPropVec->m_pBegin) / 0x38;
                for (uint64_t i = 0; i < elemCount; ++i) {
                    sub_e35150(pNewEditor, pPropVec->m_pBegin + i * 0x38);
                }
            }
        }

        void** ppSlot = sub_218afd0(&pThis->m_fileParamEditorMap460, &combinedKey);
        *ppSlot = pNewEditor;
    }

    GXDebugMenuLabelDesc emptyDesc = {0, nullptr};
    void* pButtonRow = sub_236d620(pMenuStack, &emptyDesc);
    GXDebugMenuLabelDesc selectFileLabel = {0, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2b16ULL))};

    DLAllocator* pGuiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    auto* pCb = static_cast<GParamMemberCallback1*>(pGuiAlloc->m_pVTable->AllocateAligned(pGuiAlloc, 0x30, 8));
    if (pCb) {
        pCb->m_flags08 = (pCb->m_flags08 & 0x80000000u) | 0x40000000u;
        pCb->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b6370ULL));
        pCb->m_pTargetObj10 = pThis;
        pCb->m_arg18 = groupIndex;
        pCb->m_pfnMember20 = reinterpret_cast<uint64_t>(&GParamManager_BuildFileSelectionDialogMenu);
        pCb->m_thisDelta28 = 0;
    }
    sub_235e4c0(pButtonRow, &selectFileLabel, pCb);
}

// ============================================================================
// 22. 0x21877d0 (21 B) - GParamManager_GetParamTypeNameW
// Maps GParam data type enum (0..9) to its wide string display name at 0x535f490.
// ============================================================================
const wchar_t* GParamManager_GetParamTypeNameW(uint32_t paramType) {
    if (paramType > 9) {
        return nullptr;
    }
    const wchar_t* const* ppTable = reinterpret_cast<const wchar_t* const*>(GX_VMA(0x535f490ULL));
    return ppTable[paramType];
}

// ============================================================================
// 23. 0x21874c0 (772 B) - GParamManager_EnsureGroupParamEditorCreated
// Ensures the group-level ParamEditor entry in `m_fileParamEditorMap460` exists
// for `(groupId << 32) | 0xFFFFFFFFULL`.
// ============================================================================
void GParamManager_EnsureGroupParamEditorCreated(GParamManager* pThis, uint32_t groupId) {
    void* pGlobalMenuMgr = *reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL));
    if (!pGlobalMenuMgr) {
        return;
    }

    uint64_t combinedKey = (static_cast<uint64_t>(groupId) << 32) | 0xFFFFFFFFULL;
    uint64_t bucketIdx = GParamComputeBucketIndex(
        combinedKey,
        pThis->m_fileParamEditorMap460.m_bucketMask48,
        pThis->m_fileParamEditorMap460.m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    GParamBucketIterator* pBuckets = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < combinedKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && combinedKey >= pCur->m_key10) {
            return;
        }
    }

    DLAllocator* pAlloc = pThis->m_pAllocator00;
    uint8_t* pNewEditor = static_cast<uint8_t*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x140, 8));
    if (!pNewEditor) {
        return;
    }

    sub_e34680(pNewEditor);
    const wchar_t* pszDir = (pThis->m_defaultDirPath90.m_capacity >= 8)
        ? pThis->m_defaultDirPath90.m_data.m_pHeapBuf
        : pThis->m_defaultDirPath90.m_data.m_inlineBuf;
    const wchar_t* pszPat = (pThis->m_defaultSearchPatternC8.m_capacity >= 8)
        ? pThis->m_defaultSearchPatternC8.m_data.m_pHeapBuf
        : pThis->m_defaultSearchPatternC8.m_data.m_inlineBuf;

    void* pRootWindow = *reinterpret_cast<void**>(
        static_cast<uint8_t*>(*reinterpret_cast<void**>(static_cast<uint8_t*>(pGlobalMenuMgr) + 0x10)) + 0x18);

    sub_e34e60(
        pNewEditor,
        pThis->m_pSubObject28,
        pThis->m_paramBinding2A8,
        pThis->m_editorContext520,
        &pThis->m_binaryReaderVTable2C0,
        pThis->m_groupLookup130,
        pThis->m_paramContainer1B8,
        pszDir,
        pszPat,
        pAlloc,
        pRootWindow);

    *reinterpret_cast<uint32_t*>(pNewEditor + 0x138) = groupId;

    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_groupParamEditorMap4C0.m_bucketMask48,
        pThis->m_groupParamEditorMap4C0.m_bucketCount50);
    auto* pGroupHead = static_cast<GParamKeyValueNode*>(pThis->m_groupParamEditorMap4C0.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_groupParamEditorMap4C0.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamKeyValueNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    if (pGroupCur != pGroupHead) {
        auto* pGroupEnd = static_cast<GParamKeyValueNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
        while (pGroupCur != pGroupEnd && pGroupCur->m_key10 < groupId) {
            pGroupCur = pGroupCur->m_pNext;
        }
        if (pGroupCur != pGroupEnd && groupId >= pGroupCur->m_key10) {
            auto* pPropVec = reinterpret_cast<DLVector<uint8_t>*>(static_cast<uint8_t*>(pGroupCur->m_pVal18) + 0x08);
            uint64_t elemCount = static_cast<uint64_t>(pPropVec->m_pEnd - pPropVec->m_pBegin) / 0x38;
            for (uint64_t i = 0; i < elemCount; ++i) {
                sub_e35150(pNewEditor, pPropVec->m_pBegin + i * 0x38);
            }
        }
    }

    void** ppSlot = sub_218afd0(&pThis->m_fileParamEditorMap460, &combinedKey);
    *ppSlot = pNewEditor;
}

// ============================================================================
// 24. 0x2186f10 (1,455 B) - GParamManager_BuildFileSelectionDialogMenu
// Creates a 0x178-byte debug dialog window listing all loaded `.gparam` files
// and binds each menu item to `0x2186ef0`.
// ============================================================================
void GParamManager_OnSelectGParamFileMenuCallbackThunk(
    GParamManager* pThis,
    void* pMenuEventArg,
    uint64_t packedGroupAndSubGroup,
    uint32_t slotIdx,
    uint64_t fileKey); // 0x2186ef0

struct GXDialogRectDesc {
    uint16_t m_x00;
    uint16_t m_y02;
    uint16_t m_width04;
    uint16_t m_height06;
    uint64_t m_flags08;
};

struct GXRefCountedWStringLabel {
    DLWString m_str00;      // +0x00 .. +0x38
    int32_t   m_refCount38; // +0x38
};

void GParamManager_BuildFileSelectionDialogMenu(GParamManager* pThis, uint32_t groupId) {
    DLAllocator* pGuiAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    uint8_t* pDialog = static_cast<uint8_t*>(pGuiAlloc->m_pVTable->AllocateAligned(pGuiAlloc, 0x178, 8));
    if (pDialog) {
        GXDebugMenuLabelDesc titleDesc = {0, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2a2eULL))};
        sub_2362cf0(pDialog, &titleDesc);
    }

    void* pGlobalMenuMgr = *reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL));
    uint8_t* pRootWindow = *reinterpret_cast<uint8_t**>(
        static_cast<uint8_t*>(*reinterpret_cast<void**>(static_cast<uint8_t*>(pGlobalMenuMgr) + 0x10)) + 0x18);

    GXDialogRectDesc rectDesc = {0, 0, 300, 400, 0x51502ULL};
    if (pDialog && (pDialog[0x0B] & 0x40) != 0 && pDialog[0x6F] == 0 &&
        pRootWindow != pDialog && pRootWindow && (pRootWindow[0x0B] & 0x40) != 0)
    {
        using CanAttachFn = uint8_t(*)(void*, void*);
        auto pfnCanAttach = *reinterpret_cast<CanAttachFn*>(*reinterpret_cast<uintptr_t*>(pRootWindow) + 0x28);
        if (pfnCanAttach(pRootWindow, pDialog)) {
            sub_2328e40(pDialog, pRootWindow, *reinterpret_cast<void**>(pRootWindow + 0x20), &rectDesc);
        }
    }

    uint8_t scopedBuilder[0x40];
    sub_236c4d0(scopedBuilder, pDialog);

    GXDebugMenuLabelDesc emptyDesc = {0, nullptr};
    void* pNoneRow = sub_236d620(scopedBuilder, &emptyDesc);
    GXDebugMenuLabelDesc noneLabel = {0, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2a4eULL))};

    auto* pNoneCb = static_cast<GParamMemberCallback3*>(pGuiAlloc->m_pVTable->AllocateAligned(pGuiAlloc, 0x40, 8));
    if (pNoneCb) {
        pNoneCb->m_flags08 = (pNoneCb->m_flags08 & 0x80000000u) | 0x40000000u;
        pNoneCb->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b63b0ULL));
        pNoneCb->m_pTargetObj10 = pThis;
        pNoneCb->m_arg18 = (static_cast<uint64_t>(0xFFFFFFFFu) << 32) | groupId;
        pNoneCb->m_arg20 = 0;
        pNoneCb->m_arg28 = 0xFFFFFFFFFFFFFFFFULL;
        pNoneCb->m_pfnMember30 = reinterpret_cast<uint64_t>(&GParamManager_OnSelectGParamFileMenuCallbackThunk);
        pNoneCb->m_thisDelta38 = 0;
    }
    sub_235e4c0(pNoneRow, &noneLabel, pNoneCb);

    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    GParamBlendGroupNode* pFoundGroup = pGroupHead;

    if (pGroupCur != pGroupHead) {
        auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
        while (pGroupCur != pGroupEnd) {
            if (pGroupCur->m_groupId >= groupId) {
                if (pGroupCur->m_groupId <= groupId) {
                    pFoundGroup = pGroupCur;
                }
                break;
            }
            pGroupCur = pGroupCur->m_pNext;
        }
    }

    auto* pFileHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    for (GParamLoadedFileNode* pFileNode = pFileHead->m_pNext; pFileNode != pFileHead; pFileNode = pFileNode->m_pNext) {
        uint32_t matchedSubGroupId = 0;
        auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pFoundGroup->m_subGroupMap18.m_pHeadNode08);
        for (GParamBlendSubGroupNode* pSub = pSubHead->m_pNext; pSub != pSubHead; pSub = pSub->m_pNext) {
            for (uint32_t s = 0; s < 17; ++s) {
                if (pSub->m_slots[s].m_targetFileKey == pFileNode->m_fileKey) {
                    matchedSubGroupId = pSub->m_subGroupId;
                    goto found_subgroup;
                }
            }
        }
    found_subgroup:
        const wchar_t* pszFileName = (pFileNode->m_entry.m_fileName10.m_capacity >= 8)
            ? pFileNode->m_entry.m_fileName10.m_data.m_pHeapBuf
            : pFileNode->m_entry.m_fileName10.m_data.m_inlineBuf;

        struct FormattedLabelHolder {
            GXRefCountedWStringLabel* m_pRefLabel00;
            const wchar_t*            m_pszStatic08;
        } formatted = {nullptr, nullptr};
        sub_231d3a0(&formatted, pszFileName);

        uint64_t fileKey = pFileNode->m_fileKey;
        GXDebugMenuLabelDesc rowDesc = {0, nullptr};
        void* pFileRow = sub_236d620(scopedBuilder, &rowDesc);

        GXDebugMenuLabelDesc itemLabel = {0, nullptr};
        GXRefCountedWStringLabel* pRefLabel = nullptr;
        if (formatted.m_pszStatic08) {
            itemLabel.m_pszText08 = formatted.m_pszStatic08;
        } else if (formatted.m_pRefLabel00) {
            pRefLabel = formatted.m_pRefLabel00;
            pRefLabel->m_refCount38++;
            itemLabel.m_refObj00 = reinterpret_cast<uint64_t>(pRefLabel);
            pRefLabel->m_refCount38++;
        }

        auto* pFileCb = static_cast<GParamMemberCallback3*>(pGuiAlloc->m_pVTable->AllocateAligned(pGuiAlloc, 0x40, 8));
        if (pFileCb) {
            pFileCb->m_flags08 = (pFileCb->m_flags08 & 0x80000000u) | 0x40000000u;
            pFileCb->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b63b0ULL));
            pFileCb->m_pTargetObj10 = pThis;
            pFileCb->m_arg18 = (static_cast<uint64_t>(matchedSubGroupId) << 32) | groupId;
            pFileCb->m_arg20 = 0;
            pFileCb->m_arg28 = fileKey;
            pFileCb->m_pfnMember30 = reinterpret_cast<uint64_t>(&GParamManager_OnSelectGParamFileMenuCallbackThunk);
            pFileCb->m_thisDelta38 = 0;
        }
        sub_235e4c0(pFileRow, &itemLabel, pFileCb);

        if (pRefLabel) {
            if (--pRefLabel->m_refCount38 == 0) {
                DLAllocator* pStrAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e10ULL));
                if (pRefLabel->m_str00.m_capacity >= 8) {
                    pRefLabel->m_str00.m_pAllocator->m_pVTable->Free(
                        pRefLabel->m_str00.m_pAllocator,
                        pRefLabel->m_str00.m_data.m_pHeapBuf);
                }
                pRefLabel->m_str00.m_capacity = 7;
                pRefLabel->m_str00.m_length = 0;
                pRefLabel->m_str00.m_data.m_inlineBuf[0] = L'\0';
                pStrAlloc->m_pVTable->Free(pStrAlloc, pRefLabel);
            }
        }
        if (formatted.m_pRefLabel00) {
            if (--formatted.m_pRefLabel00->m_refCount38 == 0) {
                DLAllocator* pStrAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e10ULL));
                if (formatted.m_pRefLabel00->m_str00.m_capacity >= 8) {
                    formatted.m_pRefLabel00->m_str00.m_pAllocator->m_pVTable->Free(
                        formatted.m_pRefLabel00->m_str00.m_pAllocator,
                        formatted.m_pRefLabel00->m_str00.m_data.m_pHeapBuf);
                }
                formatted.m_pRefLabel00->m_str00.m_capacity = 7;
                formatted.m_pRefLabel00->m_str00.m_length = 0;
                formatted.m_pRefLabel00->m_str00.m_data.m_inlineBuf[0] = L'\0';
                pStrAlloc->m_pVTable->Free(pStrAlloc, formatted.m_pRefLabel00);
            }
        }
    }

    sub_236c720(scopedBuilder);
}

// ============================================================================
// 25. 0x2186ef0 (26 B) - GParamManager_OnSelectGParamFileMenuCallbackThunk
// Unpacks `(groupId, subGroupId, slotIdx, fileKey)` from stack args and forwards
// to `GParamManager_SelectGParamSlotAndRefreshEditor` (`0x2186da0`).
// ============================================================================
void GParamManager_SelectGParamSlotAndRefreshEditor(
    GParamManager* pThis,
    uint32_t groupId,
    uint32_t subGroupId,
    int32_t slotIdx,
    uint64_t fileKey); // 0x2186da0

void GParamManager_OnSelectGParamFileMenuCallbackThunk(
    GParamManager* pThis,
    void* /*pMenuEventArg*/,
    uint64_t packedGroupAndSubGroup,
    uint32_t slotIdx,
    uint64_t fileKey)
{
    uint32_t groupId = static_cast<uint32_t>(packedGroupAndSubGroup);
    uint32_t subGroupId = static_cast<uint32_t>(packedGroupAndSubGroup >> 32);
    GParamManager_SelectGParamSlotAndRefreshEditor(pThis, groupId, subGroupId, static_cast<int32_t>(slotIdx), fileKey);
}

// ============================================================================
// 26. 0x2186da0 (327 B) - GParamManager_SelectGParamSlotAndRefreshEditor
// Updates `m_editorSlotSubGroup588[groupId]` and `m_editorSlotIndex5E4[groupId]`,
// validates via `0x2186bd0`, and notifies the open ParamEditor via `0xe35450`.
// ============================================================================
bool GParamManager_HasValidOverrideTargetInSlot(GParamManager* pThis, uint32_t groupId); // 0x2186bd0

void GParamManager_SelectGParamSlotAndRefreshEditor(
    GParamManager* pThis,
    uint32_t groupId,
    uint32_t subGroupId,
    int32_t slotIdx,
    uint64_t fileKey)
{
    pThis->m_editorSlotSubGroup588[groupId] = subGroupId;
    pThis->m_editorSlotIndex5E4[groupId] = slotIdx;
    if (!GParamManager_HasValidOverrideTargetInSlot(pThis, groupId)) {
        pThis->m_editorSlotSubGroup588[groupId] = 0xFFFFFFFFu;
        pThis->m_editorSlotIndex5E4[groupId] = 0;
    }

    uint64_t editorKey = (static_cast<uint64_t>(groupId) << 32) | 0xFFFFFFFFULL;
    uint64_t bucketIdx = GParamComputeBucketIndex(
        editorKey,
        pThis->m_fileParamEditorMap460.m_bucketMask48,
        pThis->m_fileParamEditorMap460.m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    GParamBucketIterator* pBuckets = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < editorKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && editorKey >= pCur->m_key10) {
            sub_e35450(pCur->m_pVal18, groupId, reinterpret_cast<void*>(fileKey));
        }
    }
}

// ============================================================================
// 27. 0x2186bd0 (458 B) - GParamManager_HasValidOverrideTargetInSlot
// Returns true if the editor's currently selected `(subGroupId, slotIdx)` for
// `groupId` has a valid (non `-1`) target `fileKey` in `m_blendGroupMap338`.
// ============================================================================
bool GParamManager_HasValidOverrideTargetInSlot(GParamManager* pThis, uint32_t groupId) {
    int64_t slotIdx = pThis->m_editorSlotIndex5E4[groupId];
    if (slotIdx < 0) {
        return false;
    }
    uint32_t subGroupId = pThis->m_editorSlotSubGroup588[groupId];

    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    if (pGroupCur == pGroupHead) {
        return false;
    }
    auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
        pGroupCur = pGroupCur->m_pNext;
    }
    if (pGroupCur == pGroupEnd || pGroupCur->m_groupId > groupId) {
        return false;
    }

    GParamHashTable* pSubMap = &pGroupCur->m_subGroupMap18;
    uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
    auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
    GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
    auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
    if (pSubCur == pSubHead) {
        return false;
    }
    auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
        pSubCur = pSubCur->m_pNext;
    }
    if (pSubCur == pSubEnd || subGroupId < pSubCur->m_subGroupId) {
        return false;
    }

    return pSubCur->m_slots[slotIdx].m_targetFileKey != 0xFFFFFFFFFFFFFFFFULL;
}

// ============================================================================
// 28. 0x2186a80 (327 B) - GParamManager_SetSlotTargetAndRefreshEditor
// Sets `(subGroupId, slotIdx)` for `groupId`, validates via `0x2186bd0`, and
// refreshes the open ParamEditor via `0xe36650`.
// ============================================================================
void GParamManager_SetSlotTargetAndRefreshEditor(
    GParamManager* pThis,
    uint32_t editorArg,
    uint32_t groupId,
    uint32_t subGroupId,
    int32_t slotIdx)
{
    pThis->m_editorSlotSubGroup588[groupId] = subGroupId;
    pThis->m_editorSlotIndex5E4[groupId] = slotIdx;
    if (!GParamManager_HasValidOverrideTargetInSlot(pThis, groupId)) {
        pThis->m_editorSlotSubGroup588[groupId] = 0xFFFFFFFFu;
        pThis->m_editorSlotIndex5E4[groupId] = 0;
    }

    uint64_t editorKey = (static_cast<uint64_t>(groupId) << 32) | 0xFFFFFFFFULL;
    uint64_t bucketIdx = GParamComputeBucketIndex(
        editorKey,
        pThis->m_fileParamEditorMap460.m_bucketMask48,
        pThis->m_fileParamEditorMap460.m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    GParamBucketIterator* pBuckets = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < editorKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && editorKey >= pCur->m_key10) {
            sub_e36650(pCur->m_pVal18, groupId, editorArg);
        }
    }
}

// ============================================================================
// 29. 0x2186970 (267 B) - GParamManager_SetSlotTargetUncheckedAndRefreshEditor
// Unchecked variant that sets `(subGroupId, slotIdx)` for `groupId` and calls
// `0xe36600` on the open ParamEditor.
// ============================================================================
void GParamManager_SetSlotTargetUncheckedAndRefreshEditor(
    GParamManager* pThis,
    uint32_t editorArg,
    uint32_t groupId,
    uint32_t subGroupId,
    int32_t slotIdx)
{
    pThis->m_editorSlotSubGroup588[groupId] = subGroupId;
    pThis->m_editorSlotIndex5E4[groupId] = slotIdx;

    uint64_t editorKey = (static_cast<uint64_t>(groupId) << 32) | 0xFFFFFFFFULL;
    uint64_t bucketIdx = GParamComputeBucketIndex(
        editorKey,
        pThis->m_fileParamEditorMap460.m_bucketMask48,
        pThis->m_fileParamEditorMap460.m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    GParamBucketIterator* pBuckets = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < editorKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && editorKey >= pCur->m_key10) {
            sub_e36600(pCur->m_pVal18, groupId, editorArg);
        }
    }
}

// ============================================================================
// 30. 0x2186730 (575 B) - GParamManager_UnregisterFileKeyFromMap398
// Removes an active override target (`targetKey`) from `m_activeOverrideMap398`.
// ============================================================================
void GParamManager_UnregisterFileKeyFromMap398(GParamManager* pThis, uint64_t targetKey) {
    GParamHashTable* pMap = &pThis->m_activeOverrideMap398;
    uint64_t bucketIdx = GParamComputeBucketIndex(targetKey, pMap->m_bucketMask48, pMap->m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pMap->m_pHeadNode08);
    GParamBucketIterator* pBuckets = pMap->m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur == pHead) {
        return;
    }

    auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
    while (pCur != pEnd && pCur->m_key10 < targetKey) {
        pCur = pCur->m_pNext;
    }
    if (pCur == pEnd || targetKey < pCur->m_key10) {
        return;
    }

    uint64_t eraseBucketIdx = GParamComputeBucketIndex(pCur->m_key10, pMap->m_bucketMask48, pMap->m_bucketCount50);
    GParamBucketIterator* pEraseBegin = &pBuckets[eraseBucketIdx * 2];
    GParamBucketIterator* pEraseEnd = &pBuckets[eraseBucketIdx * 2 + 1];

    if (pEraseEnd->m_pNode10 == pCur) {
        if (pEraseBegin->m_pNode10 == pCur) {
            pEraseBegin->m_pNode10 = pHead;
            pEraseEnd->m_pNode10 = pHead;
        } else {
            pEraseEnd->m_pNode10 = pCur->m_pPrev;
        }
    } else if (pEraseBegin->m_pNode10 == pCur) {
        pEraseBegin->m_pNode10 = pCur->m_pNext;
    }

    if (pCur != pHead) {
        pCur->m_pPrev->m_pNext = pCur->m_pNext;
        pCur->m_pNext->m_pPrev = pCur->m_pPrev;
        pMap->m_pNodeAlloc18->m_pVTable->Free(pMap->m_pNodeAlloc18, pCur);
        pMap->m_nodeCount10--;
    }
}

// ============================================================================
// 31. 0x2186450 (734 B) - GParamManager_RegisterFileKeyInMap398
// Registers an active override target `pTarget` in `m_activeOverrideMap398`.
// Returns false if already present, true if newly inserted.
// ============================================================================
bool GParamManager_RegisterFileKeyInMap398(
    GParamManager* pThis,
    GParamActiveOverrideTarget* pTarget)
{
    uint64_t targetKey = reinterpret_cast<uint64_t>(pTarget);
    GParamHashTable* pMap = &pThis->m_activeOverrideMap398;
    uint64_t bucketIdx = GParamComputeBucketIndex(targetKey, pMap->m_bucketMask48, pMap->m_bucketCount50);

    auto* pHead = static_cast<GParamKeyValueNode*>(pMap->m_pHeadNode08);
    GParamBucketIterator* pBuckets = pMap->m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < targetKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && targetKey >= pCur->m_key10) {
            return false;
        }
    }

    GParamKeyValueNode* pFound = pHead;
    pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < targetKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && targetKey >= pCur->m_key10) {
            pFound = pCur;
        }
    }

    if (pFound == pHead) {
        GParamKeyValueNode* pFirst = pHead->m_pNext;
        GParamKeyValueNode* pPrev = pFirst->m_pPrev;
        DLAllocator* pAlloc = pMap->m_pNodeAlloc18;
        auto* pNewNode = static_cast<GParamKeyValueNode*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x20, 8));
        if (!pNewNode) {
            sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
        }
        pNewNode->m_pNext = pFirst ? pFirst : pNewNode;
        pNewNode->m_pPrev = pFirst ? pPrev : pNewNode;
        pNewNode->m_key10 = targetKey;
        pNewNode->m_pVal18 = nullptr;

        if (pMap->m_nodeCount10 == 0x7FFFFFFFFFFFFFEULL) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        pMap->m_nodeCount10++;
        pFirst->m_pPrev = pNewNode;
        pNewNode->m_pPrev->m_pNext = pNewNode;

        struct InsertResult {
            void*               m_pReserved00;
            void*               m_pReserved08;
            GParamKeyValueNode* m_pNode10;
            uint8_t             m_bInserted18;
        } res;
        GParamBucketIterator nodeIter = {nullptr, nullptr, pHead->m_pNext};
        sub_218cf50(&res, pMap, &pHead->m_pNext->m_key10, nodeIter);
        pFound = res.m_pNode10;
    }

    pFound->m_pVal18 = pTarget;
    return true;
}

// ============================================================================
// 32. 0x2186360 (226 B) - GParamManager_FindLoadedFileEntryByKey
// Looks up `fileKey` in `m_loadedFileMap2D8` and returns `&pNode->m_entry`
// (`pNode + 0x18`), or `nullptr` if not found.
// ============================================================================
GParamLoadedFileEntry* GParamManager_FindLoadedFileEntryByKey(GParamManager* pThis, uint64_t fileKey) {
    uint64_t bucketIdx = GParamComputeBucketIndex(
        fileKey,
        pThis->m_loadedFileMap2D8.m_bucketMask48,
        pThis->m_loadedFileMap2D8.m_bucketCount50);

    GParamBucketIterator* pBuckets = pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin;
    auto* pHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    auto* pCur = static_cast<GParamLoadedFileNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    GParamLoadedFileNode* pFound = pHead;

    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamLoadedFileNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd) {
            if (pCur->m_fileKey >= fileKey) {
                if (pCur->m_fileKey <= fileKey) {
                    pFound = pCur;
                }
                break;
            }
            pCur = pCur->m_pNext;
        }
    }
    return (pFound != pHead) ? &pFound->m_entry : nullptr;
}

// ============================================================================
// 33. 0x21860e0 (633 B) - GParamManager_FindActiveSlotLoadedFileEntry
// Resolves the editor's active `(subGroupId, slotIdx)` for `groupId` to its
// `fileKey` and returns the corresponding `GParamLoadedFileEntry*`.
// ============================================================================
GParamLoadedFileEntry* GParamManager_FindActiveSlotLoadedFileEntry(
    GParamManager* pThis,
    uint32_t groupId)
{
    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);

    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    GParamBlendGroupNode* pGroupFound = pGroupHead;

    if (pGroupCur != pGroupHead) {
        auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
        while (pGroupCur != pGroupEnd) {
            if (pGroupCur->m_groupId >= groupId) {
                if (pGroupCur->m_groupId <= groupId) {
                    pGroupFound = pGroupCur;
                }
                break;
            }
            pGroupCur = pGroupCur->m_pNext;
        }
    }

    int64_t slotIdx = pThis->m_editorSlotIndex5E4[groupId];
    uint32_t subGroupId = pThis->m_editorSlotSubGroup588[groupId];
    if (slotIdx < 0 || pGroupFound == pGroupHead) {
        return nullptr;
    }

    GParamHashTable* pSubMap = &pGroupFound->m_subGroupMap18;
    uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
    auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
    GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
    auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
    if (pSubCur == pSubHead) {
        return nullptr;
    }
    auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
        pSubCur = pSubCur->m_pNext;
    }
    if (pSubCur == pSubEnd || subGroupId < pSubCur->m_subGroupId) {
        return nullptr;
    }

    uint64_t fileKey = pSubCur->m_slots[slotIdx].m_targetFileKey;
    return GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
}

// ============================================================================
// 34. 0x2185e00 (734 B) - GParamManager_HasActiveParamInAnySlot
// Checks across all 17 blend slots whether `(groupId, paramId, subGroupId)` is
// present and active in any bound `.gparam` file.
// ============================================================================
bool GParamManager_HasActiveParamInAnySlot(
    GParamManager* pThis,
    uint32_t groupId,
    uint32_t paramId,
    uint32_t subGroupId)
{
    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    if (pGroupCur == pGroupHead) {
        return false;
    }
    auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
        pGroupCur = pGroupCur->m_pNext;
    }
    if (pGroupCur == pGroupEnd || pGroupCur->m_groupId > groupId) {
        return false;
    }

    GParamHashTable* pSubMap = &pGroupCur->m_subGroupMap18;
    uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
    auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
    GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
    auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
    if (pSubCur == pSubHead) {
        return false;
    }
    auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
        pSubCur = pSubCur->m_pNext;
    }
    if (pSubCur == pSubEnd || pSubCur->m_subGroupId > subGroupId) {
        return false;
    }

    for (uint32_t slotIdx = 0; slotIdx < 17; ++slotIdx) {
        uint64_t fileKey = pSubCur->m_slots[slotIdx].m_targetFileKey;
        GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
        if (!pEntry) {
            continue;
        }
        uint8_t handlerCount = 0;
        void** ppHandlers = sub_e43610(pEntry->m_pParamContainer400, groupId, &handlerCount);
        if (!ppHandlers || handlerCount == 0) {
            continue;
        }
        for (uint32_t h = 0; h < handlerCount; ++h) {
            void* pHandler = ppHandlers[h];
            if (pHandler) {
                using HasParamFn = bool(*)(void*, uint32_t);
                auto pfnHasParam = *reinterpret_cast<HasParamFn*>(*reinterpret_cast<uintptr_t*>(pHandler) + 0x60);
                if (pfnHasParam(pHandler, paramId)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// ============================================================================
// 35. 0x2185b50 (685 B) - GParamManager_ClearOverrideSlotTargetWithBlendTime
// Clears `slotIndex` in `(groupId, subGroupId)` and optionally triggers a blend
// transition of duration `blendTime` back to the highest-priority lower slot,
// restoring backup property values on all matching active targets.
// ============================================================================
void GParamManager_ClearOverrideSlotTargetWithBlendTime(
    GParamManager* pThis,
    uint32_t groupId,
    uint32_t subGroupId,
    uint32_t slotIndex,
    float blendTime)
{
    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    if (pGroupCur == pGroupHead) {
        return;
    }
    auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
        pGroupCur = pGroupCur->m_pNext;
    }
    if (pGroupCur == pGroupEnd || pGroupCur->m_groupId > groupId) {
        return;
    }

    GParamHashTable* pSubMap = &pGroupCur->m_subGroupMap18;
    uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
    auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
    GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
    auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
    if (pSubCur == pSubHead) {
        return;
    }
    auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
        pSubCur = pSubCur->m_pNext;
    }
    if (pSubCur == pSubEnd || pSubCur->m_subGroupId > subGroupId) {
        return;
    }

    GParamBlendSlotState& slot = pSubCur->m_slots[slotIndex];
    slot.m_targetFileKey = 0xFFFFFFFFFFFFFFFFULL;
    slot.m_elapsedTime = 0.0f;
    slot.m_blendDuration = 0.0f;
    slot.m_bInterpolating = false;

    if (blendTime > 0.0f) {
        for (int32_t prevSlot = static_cast<int32_t>(slotIndex) - 1; prevSlot >= 0; --prevSlot) {
            if (pSubCur->m_slots[prevSlot].m_targetFileKey != 0xFFFFFFFFFFFFFFFFULL) {
                pSubCur->m_slots[prevSlot].m_bInterpolating = true;
                pSubCur->m_slots[prevSlot].m_elapsedTime = 0.0f;
                pSubCur->m_slots[prevSlot].m_blendDuration = blendTime;
                break;
            }
        }

        auto* pTargetHead = static_cast<GParamKeyValueNode*>(pThis->m_activeOverrideMap398.m_pHeadNode08);
        for (GParamKeyValueNode* pNode = pTargetHead->m_pNext; pNode != pTargetHead; pNode = pNode->m_pNext) {
            auto* pTarget = static_cast<GParamActiveOverrideTarget*>(pNode->m_pVal18);
            if (!pTarget->m_bActive28 || pTarget->m_groupId1C != groupId) {
                continue;
            }
            pTarget->m_bIsBlending09 = false;
            pTarget->m_blendElapsed0C = 0.0f;
            pTarget->m_blendDuration10 = 0.0f;
            pTarget->m_bUseFixedBlendFactor14 = false;
            pTarget->m_fixedBlendFactor18 = 0.0f;

            for (int32_t p = 0; p < pTarget->m_propertyCount2C; ++p) {
                GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
                if (prop.m_pLiveValuePtr10) {
                    sub_261a1b0(prop.m_pBackupValuePtr18, prop.m_pLiveValuePtr10, prop.m_valueByteSize20);
                }
            }
        }
    }

    pSubCur->m_bSlotEnabled[slotIndex] = 0;
}

// ============================================================================
// 36. 0x2184ff0 (2,899 B) - GParamManager_SetOverrideSlotTargetWithBlendTime
// Binds `fileKey` to `(groupId, subGroupId, slotIndex)` with interpolation duration
// `blendTime`, creating the group/subGroup blend nodes if they do not exist.
// ============================================================================
static void GParamManager_ResetAndBackupGroupTargets(GParamManager* pThis, uint32_t groupId) {
    auto* pTargetHead = static_cast<GParamKeyValueNode*>(pThis->m_activeOverrideMap398.m_pHeadNode08);
    for (GParamKeyValueNode* pNode = pTargetHead->m_pNext; pNode != pTargetHead; pNode = pNode->m_pNext) {
        auto* pTarget = static_cast<GParamActiveOverrideTarget*>(pNode->m_pVal18);
        if (!pTarget->m_bActive28 || pTarget->m_groupId1C != groupId) {
            continue;
        }
        pTarget->m_bIsBlending09 = false;
        pTarget->m_blendElapsed0C = 0.0f;
        pTarget->m_blendDuration10 = 0.0f;
        pTarget->m_bUseFixedBlendFactor14 = false;
        pTarget->m_fixedBlendFactor18 = 0.0f;

        for (int32_t p = 0; p < pTarget->m_propertyCount2C; ++p) {
            GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
            if (prop.m_pLiveValuePtr10) {
                sub_261a1b0(prop.m_pBackupValuePtr18, prop.m_pLiveValuePtr10, prop.m_valueByteSize20);
            }
        }
    }
}

void GParamManager_SetOverrideSlotTargetWithBlendTime(
    GParamManager* pThis,
    uint32_t groupId,
    uint32_t subGroupId,
    uint32_t slotIndex,
    uint64_t fileKey,
    float blendTime)
{
    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);

    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    GParamBlendGroupNode* pGroupNode = pGroupHead;

    if (pGroupCur != pGroupHead) {
        auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
        while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
            pGroupCur = pGroupCur->m_pNext;
        }
        if (pGroupCur != pGroupEnd && pGroupCur->m_groupId <= groupId) {
            pGroupNode = pGroupCur;
        }
    }

    if (pGroupNode != pGroupHead) {
        GParamHashTable* pSubMap = &pGroupNode->m_subGroupMap18;
        uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
        auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
        GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
        auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
        GParamBlendSubGroupNode* pSubNode = pSubHead;

        if (pSubCur != pSubHead) {
            auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
            while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
                pSubCur = pSubCur->m_pNext;
            }
            if (pSubCur != pSubEnd && pSubCur->m_subGroupId <= subGroupId) {
                pSubNode = pSubCur;
            }
        }

        if (pSubNode != pSubHead) {
            GParamBlendSlotState& slot = pSubNode->m_slots[slotIndex];
            slot.m_targetFileKey = fileKey;
            slot.m_elapsedTime = 0.0f;
            slot.m_blendDuration = blendTime;
            if (blendTime > 0.0f) {
                slot.m_bInterpolating = true;
                GParamManager_ResetAndBackupGroupTargets(pThis, groupId);
            } else {
                slot.m_bInterpolating = false;
            }
            pSubNode->m_bSlotEnabled[slotIndex] = 1;
            return;
        }

        GParamBlendSubGroupNode tempSub;
        sub_218a7a0(&tempSub.m_slots[0]);
        tempSub.m_slots[slotIndex].m_targetFileKey = fileKey;
        tempSub.m_slots[slotIndex].m_elapsedTime = 0.0f;
        tempSub.m_slots[slotIndex].m_blendDuration = blendTime;
        if (blendTime > 0.0f) {
            tempSub.m_slots[slotIndex].m_bInterpolating = true;
            GParamManager_ResetAndBackupGroupTargets(pThis, groupId);
        } else {
            tempSub.m_slots[slotIndex].m_bInterpolating = false;
        }
        tempSub.m_bSlotEnabled[slotIndex] = 1;

        void* pDestPayload = sub_218a9f0(&pGroupNode->m_subGroupMap18, &subGroupId);
        sub_2bbe578(pDestPayload, &tempSub.m_slots[0], 0x1A9);
        return;
    }

    DLAllocator* pDefAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL));
    if (!pDefAlloc) {
        pDefAlloc = sub_2ba2b10();
        *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL)) = pDefAlloc;
    }
    uint8_t allocCaps[0x40];
    pDefAlloc->m_pVTable->GetMemoryCapabilities(allocCaps, pDefAlloc, 0);
    if ((allocCaps[0] & 0x20) == 0) {
        sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }

    GParamHashTable tempSubMap;
    sub_218a630(&tempSubMap, &pDefAlloc);

    GParamBlendSubGroupNode tempSub;
    sub_218a7a0(&tempSub.m_slots[0]);
    tempSub.m_slots[slotIndex].m_targetFileKey = fileKey;
    tempSub.m_slots[slotIndex].m_elapsedTime = 0.0f;
    tempSub.m_slots[slotIndex].m_blendDuration = blendTime;
    if (blendTime > 0.0f) {
        tempSub.m_slots[slotIndex].m_bInterpolating = true;
        GParamManager_ResetAndBackupGroupTargets(pThis, groupId);
    } else {
        tempSub.m_slots[slotIndex].m_bInterpolating = false;
    }
    tempSub.m_bSlotEnabled[slotIndex] = 1;

    void* pTempDestPayload = sub_218a9f0(&tempSubMap, &subGroupId);
    sub_2bbe578(pTempDestPayload, &tempSub.m_slots[0], 0x1A9);

    GParamBlendGroupNode* pTargetGroupNode = pGroupHead;
    pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    if (pGroupCur != pGroupHead) {
        auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
        while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
            pGroupCur = pGroupCur->m_pNext;
        }
        if (pGroupCur != pGroupEnd && pGroupCur->m_groupId <= groupId) {
            pTargetGroupNode = pGroupCur;
        }
    }

    if (pTargetGroupNode == pGroupHead) {
        DLAllocator* pGroupAlloc = pThis->m_blendGroupMap338.m_pNodeAlloc18;
        GParamBlendGroupNode* pFirst = pGroupHead->m_pNext;
        GParamBlendGroupNode* pPrev = pFirst->m_pPrev;
        auto* pNewGroupNode = static_cast<GParamBlendGroupNode*>(
            pGroupAlloc->m_pVTable->AllocateAligned(pGroupAlloc, sizeof(GParamBlendGroupNode), 8));
        if (!pNewGroupNode) {
            sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
        }
        pNewGroupNode->m_pNext = pFirst ? pFirst : pNewGroupNode;
        pNewGroupNode->m_pPrev = pFirst ? pPrev : pNewGroupNode;
        pNewGroupNode->m_groupId = groupId;

        DLAllocator* pSubNodeAlloc = tempSubMap.m_pNodeAlloc18;
        pNewGroupNode->m_subGroupMap18.m_nodeCount10 = 0;
        pNewGroupNode->m_subGroupMap18.m_pHeadNode08 = nullptr;
        pNewGroupNode->m_subGroupMap18.m_pNodeAlloc18 = pSubNodeAlloc;

        auto* pSubSentinel = static_cast<GParamBlendSubGroupNode*>(
            pSubNodeAlloc->m_pVTable->AllocateAligned(pSubNodeAlloc, sizeof(GParamBlendSubGroupNode), 8));
        if (!pSubSentinel) {
            sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
        }
        pSubSentinel->m_pNext = pSubSentinel;
        pSubSentinel->m_pPrev = pSubSentinel;
        pNewGroupNode->m_subGroupMap18.m_pHeadNode08 = pSubSentinel;
        pNewGroupNode->m_subGroupMap18.m_buckets28 = {nullptr, nullptr, {nullptr}, pSubNodeAlloc};
        pNewGroupNode->m_subGroupMap18.m_maxLoadFactor58 = 1.0f;

        sub_218ac30(&pNewGroupNode->m_subGroupMap18, &tempSubMap);

        if (pThis->m_blendGroupMap338.m_nodeCount10 == 0x222222222222221ULL) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        pThis->m_blendGroupMap338.m_nodeCount10++;
        pFirst->m_pPrev = pNewGroupNode;
        pNewGroupNode->m_pPrev->m_pNext = pNewGroupNode;

        struct GroupInsertResult {
            void*                 m_pReserved00;
            void*                 m_pReserved08;
            GParamBlendGroupNode* m_pNode10;
            uint8_t               m_bInserted18;
        } insRes;
        GParamBucketIterator groupNodeIter = {nullptr, nullptr, pGroupHead->m_pNext};
        sub_218ca90(&insRes, &pThis->m_blendGroupMap338, &pGroupHead->m_pNext->m_groupId, groupNodeIter);
        pTargetGroupNode = insRes.m_pNode10;
    }

    if (&pTargetGroupNode->m_subGroupMap18 != &tempSubMap) {
        sub_218ac30(&pTargetGroupNode->m_subGroupMap18, &tempSubMap);
    }

    if (tempSubMap.m_buckets28.m_pBegin) {
        tempSubMap.m_buckets28.m_pAllocator->m_pVTable->Free(
            tempSubMap.m_buckets28.m_pAllocator,
            tempSubMap.m_buckets28.m_pBegin);
        tempSubMap.m_buckets28.m_pCapacity = nullptr;
        tempSubMap.m_buckets28.m_pEnd = nullptr;
        tempSubMap.m_buckets28.m_pBegin = nullptr;
    }

    auto* pTempHead = static_cast<GParamBlendSubGroupNode*>(tempSubMap.m_pHeadNode08);
    GParamBlendSubGroupNode* pTempCur = pTempHead->m_pNext;
    pTempHead->m_pNext = pTempHead;
    pTempHead->m_pPrev = pTempHead;
    tempSubMap.m_nodeCount10 = 0;
    while (pTempCur != pTempHead) {
        GParamBlendSubGroupNode* pNext = pTempCur->m_pNext;
        tempSubMap.m_pNodeAlloc18->m_pVTable->Free(tempSubMap.m_pNodeAlloc18, pTempCur);
        pTempCur = pNext;
    }
    tempSubMap.m_pNodeAlloc18->m_pVTable->Free(tempSubMap.m_pNodeAlloc18, pTempHead);
}

// ============================================================================
// 37..43. 0x2184f10 .. 0x2184fe0 - GParamManager sub-object forwarding thunks
// ============================================================================
uint64_t GParamManager_ForwardToContainer1B8_E42E90(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
    __asm__("_ZN2GX42GParamManager_ForwardToContainer1B8_E42E90EPNS_13GParamManagerE");

uint64_t GParamManager_ForwardToContainer1B8_E42B80(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
    __asm__("_ZN2GX42GParamManager_ForwardToContainer1B8_E42B80EPNS_13GParamManagerE");

uint64_t GParamManager_ForwardToSubObject28_E33600(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
    __asm__("_ZN2GX41GParamManager_ForwardToSubObject28_E33600EPNS_13GParamManagerE");

uint64_t GParamManager_ForwardToSubObject28_E32510(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
    __asm__("_ZN2GX41GParamManager_ForwardToSubObject28_E32510EPNS_13GParamManagerE");

// 37. 0x2184fe0 (12 B)
uint64_t GParamManager_ForwardToContainer1B8_E42E90(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    return sub_e42e90(pThis->m_paramContainer1B8, a2, a3, a4, a5, a6);
}

// 38. 0x2184fd0 (12 B)
uint64_t GParamManager_ForwardToContainer1B8_E42B80(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    return sub_e42b80(pThis->m_paramContainer1B8, a2, a3, a4, a5, a6);
}

// 39. 0x2184fa0 (36 B)
uint64_t GParamManager_ForwardToSubObject28_E327D0(
    GParamManager* pThis,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint8_t a6)
{
    return sub_e327d0(pThis->m_pSubObject28, a2, a3, a4, a5, 1u, static_cast<uint32_t>(a6));
}

// 40. 0x2184f90 (9 B)
uint64_t GParamManager_ForwardToSubObject28_E33600(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    return sub_e33600(pThis->m_pSubObject28, a2, a3, a4, a5, a6);
}

// 41. 0x2184f80 (15 B)
uint64_t GParamManager_ForwardToSubObject28_E333E0(
    GParamManager* pThis,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5)
{
    return sub_e333e0(pThis->m_pSubObject28, a2, a3, a4, a5, 1u);
}

// 42. 0x2184f20 (84 B)
void GParamManager_InitializeDefaultGroupAndParamContainer(
    GParamManager* pThis,
    uint32_t a2,
    uint32_t a3,
    float a4)
{
    sub_218e2b0(pThis->m_groupLookup130, pThis->m_pSubObject28, nullptr);
    sub_e41f00(pThis->m_paramContainer1B8, pThis->m_pSubObject28, 0u, a2, a3, a4);
}

// 43. 0x2184f10 (9 B)
uint64_t GParamManager_ForwardToSubObject28_E32510(
    GParamManager* pThis, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    return sub_e32510(pThis->m_pSubObject28, a2, a3, a4, a5, a6);
}

// ============================================================================
// 44. 0x2184de0 (295 B) - GParamManager_QueryParamValueInt
// Queries an integer parameter value `(groupId, paramId)` from `fileKey` (or the
// default container at `+0x1B8` if `fileKey` is not found).
// ============================================================================
uint32_t GParamManager_QueryParamValueInt(
    GParamManager* pThis,
    uint32_t groupId,
    uint64_t fileKey,
    uint32_t paramId)
{
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    void* pContainer = pEntry ? pEntry->m_pParamContainer400 : pThis->m_paramContainer1B8;
    uint32_t outValue = 0;
    sub_e42b00(pContainer, &outValue, groupId, paramId);
    return outValue;
}

// ============================================================================
// 45. 0x2184cf0 (234 B) - GParamManager_QueryParamContainerEntry
// Queries a group container entry for `groupId` from `fileKey` (or `+0x1B8`).
// ============================================================================
void* GParamManager_QueryParamContainerEntry(
    GParamManager* pThis,
    uint32_t groupId,
    uint64_t fileKey)
{
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    void* pContainer = pEntry ? pEntry->m_pParamContainer400 : pThis->m_paramContainer1B8;
    return sub_e42aa0(pContainer, groupId);
}

// ============================================================================
// 46. 0x21842b0 (2,622 B) - GParamManager_UpdateAndStepInterpolation
// Per-frame update of GParamManager: steps all container timers, advances the
// 17 slot blend timers per subGroup, and either dispatches multi-threaded
// GParamInterpolationWorkerJob tasks or runs single-threaded parameter blending.
// ============================================================================
extern "C" {
    void sub_e43260(void* pParamContainer, const float* pDeltaTime, uint8_t forceFlag);
    void sub_218c2c0(void* pVecWithHeader, uint64_t newCap);
    void sub_21b6f10(GParamInterpolationWorkerJob* pJob, const int32_t* pRangeAndGrain, uint32_t flags, uint32_t sync);
}

void GParamManager_UpdateAndStepInterpolation(GParamManager* pThis, float deltaTime) {
    float dt = deltaTime;
    uint8_t* pGlobalForceUpdate = reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL));
    sub_e43260(pThis->m_paramContainer1B8, &dt, *pGlobalForceUpdate);

    auto* pFileHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    for (GParamLoadedFileNode* pFileNode = pFileHead->m_pNext; pFileNode != pFileHead; pFileNode = pFileNode->m_pNext) {
        if (pFileNode->m_entry.m_pParamContainer400) {
            sub_e43260(pFileNode->m_entry.m_pParamContainer400, &dt, *pGlobalForceUpdate);
        }
    }

    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    for (GParamBlendGroupNode* pGroup = pGroupHead->m_pNext; pGroup != pGroupHead; pGroup = pGroup->m_pNext) {
        auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pGroup->m_subGroupMap18.m_pHeadNode08);
        for (GParamBlendSubGroupNode* pSub = pSubHead->m_pNext; pSub != pSubHead; pSub = pSub->m_pNext) {
            for (uint32_t s = 0; s < 17; ++s) {
                GParamBlendSlotState& slot = pSub->m_slots[s];
                if (slot.m_bInterpolating) {
                    slot.m_elapsedTime += dt;
                    if (slot.m_elapsedTime >= slot.m_blendDuration) {
                        slot.m_bInterpolating = false;
                        slot.m_elapsedTime = 0.0f;
                        slot.m_blendDuration = 0.0f;
                    }
                }
            }
        }
    }

    if (pThis->m_bEnableMultiThread128 || gx_direct_vk::IntelTonemapFixEnabled()) {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        uint8_t allocCaps[0x20];
        pAlloc->m_pVTable->GetMemoryCapabilities(allocCaps, pAlloc, 0);
        if ((allocCaps[0] & 0x20) == 0) {
            sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
        }

        GParamInterpolationWorkerJob job;
        job.m_pVTable00 = reinterpret_cast<void**>(GX_VMA(0x535f4f0ULL));
        job.m_pReserved08 = nullptr;
        job.m_targets10 = {nullptr, nullptr, {nullptr}, pAlloc};
        job.m_pBlendGroupMap30 = &pThis->m_blendGroupMap338;
        job.m_pLoadedFileMap38 = &pThis->m_loadedFileMap2D8;
        job.m_deltaTime40 = dt;

        int32_t targetCount = static_cast<int32_t>(pThis->m_activeOverrideMap398.m_nodeCount10);
        if (targetCount < 0) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
        }
        if (targetCount > 0) {
            auto* pNewBuf = static_cast<GParamActiveOverrideTarget**>(
                pAlloc->m_pVTable->AllocateAligned(pAlloc, static_cast<size_t>(targetCount) * sizeof(void*), 8));
            if (!pNewBuf) {
                if (*GetThreadLocalErrorFlag(0x53e4318ULL) != 0) {
                    sub_2bbe978(GetThreadLocalSlot(0x53e4320ULL), 1);
                }
                sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
            }
            job.m_targets10.m_pBegin = pNewBuf;
            job.m_targets10.m_pEnd = pNewBuf;
            job.m_targets10.m_pCapacity = pNewBuf + targetCount;
        }

        auto* pTargetHead = static_cast<GParamKeyValueNode*>(pThis->m_activeOverrideMap398.m_pHeadNode08);
        for (GParamKeyValueNode* pNode = pTargetHead->m_pNext; pNode != pTargetHead; pNode = pNode->m_pNext) {
            auto* pTarget = static_cast<GParamActiveOverrideTarget*>(pNode->m_pVal18);
            if (job.m_targets10.m_pEnd == job.m_targets10.m_pCapacity) {
                uint64_t curSize = static_cast<uint64_t>(job.m_targets10.m_pEnd - job.m_targets10.m_pBegin);
                const uint64_t kMaxPtrVec = 0x1FFFFFFFFFFFFFFFULL;
                uint64_t grown = (kMaxPtrVec - (curSize >> 1) >= curSize) ? (curSize + (curSize >> 1)) : 0;
                if (grown < curSize + 1) {
                    grown = curSize + 1;
                }
                sub_218c2c0(&job.m_pReserved08, grown);
            }
            if (job.m_targets10.m_pEnd) {
                *job.m_targets10.m_pEnd = pTarget;
            }
            job.m_targets10.m_pEnd++;
        }

        int32_t rangeAndGrain[3];
        rangeAndGrain[0] = 0;
        rangeAndGrain[1] = targetCount;
        rangeAndGrain[2] = (targetCount <= 5) ? targetCount : 5;
        sub_21b6f10(&job, rangeAndGrain, 0, 1);

        if (job.m_targets10.m_pBegin) {
            pAlloc->m_pVTable->Free(pAlloc, job.m_targets10.m_pBegin);
            job.m_targets10.m_pCapacity = nullptr;
            job.m_targets10.m_pEnd = nullptr;
            job.m_targets10.m_pBegin = nullptr;
        }
    } else {
        auto* pTargetHead = static_cast<GParamKeyValueNode*>(pThis->m_activeOverrideMap398.m_pHeadNode08);
        for (GParamKeyValueNode* pNode = pTargetHead->m_pNext; pNode != pTargetHead; pNode = pNode->m_pNext) {
            auto* pTarget = static_cast<GParamActiveOverrideTarget*>(pNode->m_pVal18);
            if ((*pGlobalForceUpdate == 0 && !pTarget->m_bDirty40) || !pTarget->m_bActive28) {
                continue;
            }

            float targetBlendFactor = 1.0f;
            float duration = pTarget->m_blendDuration10;
            if (pTarget->m_bIsBlending09) {
                float elapsed = pTarget->m_blendElapsed0C + dt;
                pTarget->m_blendElapsed0C = elapsed;
                if (elapsed > duration) {
                    pTarget->m_blendElapsed0C = 0.0f;
                    pTarget->m_blendDuration10 = 0.0f;
                    pTarget->m_bIsBlending09 = false;
                    targetBlendFactor = 1.0f;
                    goto check_single_fixed;
                }
            }
            if (duration > 0.0f) {
                float ratio = pTarget->m_blendElapsed0C / duration;
                if (ratio >= 0.0f) {
                    targetBlendFactor = (ratio < 1.0f) ? ratio : 1.0f;
                } else {
                    targetBlendFactor = 0.0f;
                }
            }

        check_single_fixed:
            if (pTarget->m_bUseFixedBlendFactor14) {
                targetBlendFactor = pTarget->m_fixedBlendFactor18;
            }

            uint32_t groupId = pTarget->m_groupId1C;
            uint32_t instanceParamId = pTarget->m_instanceParamId24;
            uint64_t groupBucket = GParamComputeBucketIndex(
                groupId,
                pThis->m_blendGroupMap338.m_bucketMask48,
                pThis->m_blendGroupMap338.m_bucketCount50);
            GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
            auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
            if (pGroupCur == pGroupHead) {
                continue;
            }
            auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
            while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
                pGroupCur = pGroupCur->m_pNext;
            }
            if (pGroupCur == pGroupEnd || groupId < pGroupCur->m_groupId) {
                continue;
            }

            uint32_t subGroupId = pTarget->m_subGroupId20;
            GParamHashTable* pSubMap = &pGroupCur->m_subGroupMap18;
            uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
            auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
            GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
            auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
            if (pSubCur == pSubHead) {
                continue;
            }
            auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
            while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
                pSubCur = pSubCur->m_pNext;
            }
            if (pSubCur == pSubEnd || subGroupId < pSubCur->m_subGroupId) {
                continue;
            }

            uint64_t anyAppliedFlag = 0;
            for (uint32_t slotIdx = 0; slotIdx < 17; ++slotIdx) {
                const GParamBlendSlotState& slotState = pSubCur->m_slots[slotIdx];
                if (slotState.m_targetFileKey == 0xFFFFFFFFFFFFFFFFULL) {
                    continue;
                }
                bool bSlotInterpolating = slotState.m_bInterpolating;
                float slotBlendFactor = 1.0f;
                if (slotState.m_blendDuration > 0.0f) {
                    float ratio = slotState.m_elapsedTime / slotState.m_blendDuration;
                    if (ratio >= 0.0f) {
                        slotBlendFactor = (ratio < 1.0f) ? ratio : 1.0f;
                    } else {
                        slotBlendFactor = 0.0f;
                    }
                }

                GParamLoadedFileEntry* pFileEntry = GParamManager_FindLoadedFileEntryByKey(pThis, slotState.m_targetFileKey);
                if (!pFileEntry) {
                    continue;
                }

                uint8_t handlerCount = 0;
                void** ppHandlers = sub_e43610(pFileEntry->m_pParamContainer400, pTarget->m_groupId1C, &handlerCount);
                if (!ppHandlers) {
                    continue;
                }

                if (pTarget->m_bNeedsImmediateSync38) {
                    for (int32_t p = 0; p < pTarget->m_propertyCount2C; ++p) {
                        GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
                        if (prop.m_pBackupValuePtr18 && prop.m_pLiveValuePtr10 && prop.m_bEnabled24 && prop.m_paramIndex08 < handlerCount) {
                            void* pHandler = ppHandlers[prop.m_paramIndex08];
                            if (pHandler) {
                                using SyncFn = void(*)(void*, uint32_t, void*);
                                auto pfnSync = *reinterpret_cast<SyncFn*>(*reinterpret_cast<uintptr_t*>(pHandler) + 0x68);
                                pfnSync(pHandler, instanceParamId, prop.m_pLiveValuePtr10);
                                anyAppliedFlag = 1;
                            }
                        }
                    }
                }

                pTarget->m_lastEvaluatedSlot3C = slotIdx;
                for (uint32_t p = 0; p < static_cast<uint32_t>(pTarget->m_propertyCount2C); ++p) {
                    GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
                    if (prop.m_pBackupValuePtr18 && prop.m_pLiveValuePtr10 && prop.m_bEnabled24 && prop.m_paramIndex08 < handlerCount) {
                        void* pHandler = ppHandlers[prop.m_paramIndex08];
                        if (pHandler) {
                            using BlendFn = void(*)(void*, uint32_t, void*, void*, float);
                            auto pfnBlend = *reinterpret_cast<BlendFn*>(*reinterpret_cast<uintptr_t*>(pHandler) + 0x88);
                            if (!bSlotInterpolating) {
                                pfnBlend(pHandler, instanceParamId, prop.m_pBackupValuePtr18, prop.m_pLiveValuePtr10, targetBlendFactor);
                            } else {
                                pfnBlend(pHandler, instanceParamId, prop.m_pBackupValuePtr18, prop.m_pLiveValuePtr10, slotBlendFactor);
                                anyAppliedFlag = 1;
                            }
                        }
                    }
                }
            }

            bool bApplied = (anyAppliedFlag & 1) != 0;
            pTarget->m_bApplied41 = bApplied;
            pTarget->m_bDirty40 = bApplied;
            pTarget->m_bNeedsImmediateSync38 = false;
        }
    }

    *pGlobalForceUpdate = 0;
}

// ============================================================================
// 47. 0x2183ec0 (1,007 B) - GParamManager_ApplySlotOverrideToTargetImmediate
// Immediately synchronizes the parameter values of `targetKey` from `slotIndex`
// if `pTarget->m_bNeedsImmediateSync38` is set.
// ============================================================================
void GParamManager_ApplySlotOverrideToTargetImmediate(
    GParamManager* pThis,
    uint64_t targetKey,
    uint32_t slotIndex)
{
    uint64_t targetBucket = GParamComputeBucketIndex(
        targetKey,
        pThis->m_activeOverrideMap398.m_bucketMask48,
        pThis->m_activeOverrideMap398.m_bucketCount50);
    auto* pTargetHead = static_cast<GParamKeyValueNode*>(pThis->m_activeOverrideMap398.m_pHeadNode08);
    GParamBucketIterator* pTargetBuckets = pThis->m_activeOverrideMap398.m_buckets28.m_pBegin;
    auto* pTargetCur = static_cast<GParamKeyValueNode*>(pTargetBuckets[targetBucket * 2].m_pNode10);
    if (pTargetCur == pTargetHead) {
        return;
    }
    auto* pTargetEnd = static_cast<GParamKeyValueNode*>(pTargetBuckets[targetBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pTargetCur != pTargetEnd && pTargetCur->m_key10 < targetKey) {
        pTargetCur = pTargetCur->m_pNext;
    }
    if (pTargetCur == pTargetEnd || targetKey < pTargetCur->m_key10) {
        return;
    }

    auto* pTarget = static_cast<GParamActiveOverrideTarget*>(pTargetCur->m_pVal18);
    uint32_t groupId = pTarget->m_groupId1C;

    uint64_t groupBucket = GParamComputeBucketIndex(
        groupId,
        pThis->m_blendGroupMap338.m_bucketMask48,
        pThis->m_blendGroupMap338.m_bucketCount50);
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBucketIterator* pGroupBuckets = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    auto* pGroupCur = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2].m_pNode10);
    if (pGroupCur == pGroupHead) {
        return;
    }
    auto* pGroupEnd = static_cast<GParamBlendGroupNode*>(pGroupBuckets[groupBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pGroupCur != pGroupEnd && pGroupCur->m_groupId < groupId) {
        pGroupCur = pGroupCur->m_pNext;
    }
    if (pGroupCur == pGroupEnd || groupId < pGroupCur->m_groupId) {
        return;
    }

    uint32_t subGroupId = pTarget->m_subGroupId20;
    GParamHashTable* pSubMap = &pGroupCur->m_subGroupMap18;
    uint64_t subBucket = GParamComputeBucketIndex(subGroupId, pSubMap->m_bucketMask48, pSubMap->m_bucketCount50);
    auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pSubMap->m_pHeadNode08);
    GParamBucketIterator* pSubBuckets = pSubMap->m_buckets28.m_pBegin;
    auto* pSubCur = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2].m_pNode10);
    if (pSubCur == pSubHead) {
        return;
    }
    auto* pSubEnd = static_cast<GParamBlendSubGroupNode*>(pSubBuckets[subBucket * 2 + 1].m_pNode10)->m_pNext;
    while (pSubCur != pSubEnd && pSubCur->m_subGroupId < subGroupId) {
        pSubCur = pSubCur->m_pNext;
    }
    if (pSubCur == pSubEnd || subGroupId < pSubCur->m_subGroupId) {
        return;
    }

    uint64_t fileKey = pSubCur->m_slots[slotIndex].m_targetFileKey;
    GParamLoadedFileEntry* pFileEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (!pFileEntry) {
        return;
    }

    uint8_t handlerCount = 0;
    void** ppHandlers = sub_e43610(pFileEntry->m_pParamContainer400, pTarget->m_groupId1C, &handlerCount);
    if (!ppHandlers || !pTarget->m_bNeedsImmediateSync38) {
        return;
    }

    uint32_t instanceParamId = pTarget->m_instanceParamId24;
    for (int32_t p = 0; p < pTarget->m_propertyCount2C; ++p) {
        GParamOverridePropertyBinding& prop = pTarget->m_pProperties30[p];
        if (prop.m_pBackupValuePtr18 && prop.m_pLiveValuePtr10 && prop.m_bEnabled24 && prop.m_paramIndex08 < handlerCount) {
            void* pHandler = ppHandlers[prop.m_paramIndex08];
            if (pHandler) {
                using SyncFn = void(*)(void*, uint32_t, void*);
                auto pfnSync = *reinterpret_cast<SyncFn*>(*reinterpret_cast<uintptr_t*>(pHandler) + 0x68);
                pfnSync(pHandler, instanceParamId, prop.m_pLiveValuePtr10);
            }
        }
    }
    pTarget->m_bNeedsImmediateSync38 = false;
}

// ============================================================================
// 48. 0x2183dd0 (236 B) - GParamManager_HasCustomFilePathOverride
// Returns true if the loaded file `fileKey` has a custom override path (`+0x358`).
// ============================================================================
bool GParamManager_HasCustomFilePathOverride(GParamManager* pThis, uint64_t fileKey) {
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    return pEntry ? pEntry->m_bHasCustomPath340 : false;
}

// ============================================================================
// 49. 0x2183cc0 (261 B) - GParamManager_SetCustomFilePathOverride
// Enables `m_bHasCustomPath340` on `fileKey` and assigns `pszCustomPath`.
// ============================================================================
extern "C" {
    size_t sub_2bbf228(const wchar_t* pszStr); // wcslen
    void   sub_29354e0(DLFixedWString<64>* pDst, const wchar_t* pszSrc, size_t len);
}

void GParamManager_SetCustomFilePathOverride(
    GParamManager* pThis,
    uint64_t fileKey,
    const wchar_t* pszCustomPath)
{
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (pEntry) {
        pEntry->m_bHasCustomPath340 = true;
        size_t len = (*pszCustomPath != L'\0') ? sub_2bbf228(pszCustomPath) : 0;
        sub_29354e0(&pEntry->m_customOverridePath348, pszCustomPath, len);
    }
}

// ============================================================================
// 50..53. 0x2183880 .. 0x2183bb0 - GParamManager_GetEffectiveFilePathSlot0..3W
// Returns the custom override path if enabled, or the slot-specific path 0..3.
// ============================================================================
static inline const wchar_t* GetFixedWStringChars(const DLFixedWString<64>* pStr) {
    return (pStr->m_capacity >= 8) ? pStr->m_data.m_pHeapBuf : pStr->m_data.m_inlineBuf;
}

// 50. 0x2183bb0 (262 B)
const wchar_t* GParamManager_GetEffectiveFilePathSlot1W(GParamManager* pThis, uint64_t fileKey) {
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (!pEntry) {
        return nullptr;
    }
    return pEntry->m_bHasCustomPath340
        ? GetFixedWStringChars(&pEntry->m_customOverridePath348)
        : GetFixedWStringChars(&pEntry->m_slotPath1_130);
}

// 51. 0x2183aa0 (262 B)
const wchar_t* GParamManager_GetEffectiveFilePathSlot3W(GParamManager* pThis, uint64_t fileKey) {
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (!pEntry) {
        return nullptr;
    }
    return pEntry->m_bHasCustomPath340
        ? GetFixedWStringChars(&pEntry->m_customOverridePath348)
        : GetFixedWStringChars(&pEntry->m_slotPath3_290);
}

// 52. 0x2183990 (262 B)
const wchar_t* GParamManager_GetEffectiveFilePathSlot2W(GParamManager* pThis, uint64_t fileKey) {
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (!pEntry) {
        return nullptr;
    }
    return pEntry->m_bHasCustomPath340
        ? GetFixedWStringChars(&pEntry->m_customOverridePath348)
        : GetFixedWStringChars(&pEntry->m_slotPath2_1E0);
}

// 53. 0x2183880 (262 B)
const wchar_t* GParamManager_GetEffectiveFilePathSlot0W(GParamManager* pThis, uint64_t fileKey) {
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (!pEntry) {
        return nullptr;
    }
    return pEntry->m_bHasCustomPath340
        ? GetFixedWStringChars(&pEntry->m_customOverridePath348)
        : GetFixedWStringChars(&pEntry->m_slotPath0_80);
}

// ============================================================================
// 54. 0x2183730 (335 B) - GParamManager_UnloadFileByKey
// Unloads `fileKey`, notifies open ParamEditor dialogs (`0xe366a0`), releases
// sub-objects (`0x21835e0`), and erases the node from `m_loadedFileMap2D8`.
// ============================================================================
extern "C" void sub_e366a0(void* pParamEditor);
void GParamManager_ReleaseLoadedFileSubObjects(GParamManager* pThis, GParamLoadedFileEntry* pEntry); // 0x21835e0

void GParamManager_UnloadFileByKey(GParamManager* pThis, uint64_t fileKey) {
    uint64_t bucketIdx = GParamComputeBucketIndex(
        fileKey,
        pThis->m_loadedFileMap2D8.m_bucketMask48,
        pThis->m_loadedFileMap2D8.m_bucketCount50);

    auto* pHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    GParamBucketIterator* pBuckets = pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamLoadedFileNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    if (pCur == pHead) {
        return;
    }
    auto* pEnd = static_cast<GParamLoadedFileNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
    while (pCur != pEnd && pCur->m_fileKey < fileKey) {
        pCur = pCur->m_pNext;
    }
    if (pCur == pEnd || pCur->m_fileKey > fileKey) {
        return;
    }

    auto* pEditorHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    for (GParamKeyValueNode* pEd = pEditorHead->m_pNext; pEd != pEditorHead; pEd = pEd->m_pNext) {
        sub_e366a0(pEd->m_pVal18);
    }

    GParamManager_ReleaseLoadedFileSubObjects(pThis, &pCur->m_entry);
    GParamBucketIterator nextIter;
    GParamLoadedFileMap_EraseNode(&nextIter, &pThis->m_loadedFileMap2D8, pCur);
}

// ============================================================================
// 55. 0x21835e0 (321 B) - GParamManager_ReleaseLoadedFileSubObjects
// Destroys and deallocates the raw file buffer and all 5 heap sub-objects inside
// `GParamLoadedFileEntry`.
// ============================================================================
extern "C" {
    void sub_e3e740(void* pParamContainer);
    void sub_e34020(void* pParamBinding);
    void sub_e3e610(void* pParamContainer);
    void sub_e3aaf0(void* pSubObj418);
}

struct GXGParamBinaryReader {
    struct VTable {
        void  (*Initialize)(GXGParamBinaryReader* pThis, const void* pData, uint32_t size); // +0x00
        void  (*Reset)(GXGParamBinaryReader* pThis);                                        // +0x08
    };
    VTable*  m_pVTable00;      // +0x00 (0x53b63f0)
    uint8_t  m_bBigEndian08;   // +0x08
    uint8_t  m_pad09[3];       // +0x09
    uint32_t m_cursor0C;       // +0x0C
    const void* m_pData10;     // +0x10
}; // 0x18 bytes

void GParamManager_ReleaseLoadedFileSubObjects(
    GParamManager* pThis,
    GParamLoadedFileEntry* pEntry)
{
    if (pEntry->m_pRawFileData00) {
        pThis->m_pAllocator00->m_pVTable->Free(pThis->m_pAllocator00, pEntry->m_pRawFileData00);
        pEntry->m_pRawFileData00 = nullptr;
    }
    if (pEntry->m_pParamContainer400) {
        sub_e3e740(pEntry->m_pParamContainer400);
    }
    if (pEntry->m_pGroupLookup3F8) {
        sub_218ea00(pEntry->m_pGroupLookup3F8);
    }
    if (pEntry->m_pBinaryReader410) {
        pEntry->m_pBinaryReader410->m_pVTable00->Reset(pEntry->m_pBinaryReader410);
    }

    if (pEntry->m_pParamBinding408) {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        sub_e34020(pEntry->m_pParamBinding408);
        pAlloc->m_pVTable->Free(pAlloc, pEntry->m_pParamBinding408);
        pEntry->m_pParamBinding408 = nullptr;
    }
    if (pEntry->m_pParamContainer400) {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        sub_e3e610(pEntry->m_pParamContainer400);
        pAlloc->m_pVTable->Free(pAlloc, pEntry->m_pParamContainer400);
        pEntry->m_pParamContainer400 = nullptr;
    }
    if (pEntry->m_pGroupLookup3F8) {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        GXGParamGroupLookupTable_Destroy(pEntry->m_pGroupLookup3F8);
        pAlloc->m_pVTable->Free(pAlloc, pEntry->m_pGroupLookup3F8);
        pEntry->m_pGroupLookup3F8 = nullptr;
    }
    if (pEntry->m_pBinaryReader410) {
        pThis->m_pAllocator00->m_pVTable->Free(pThis->m_pAllocator00, pEntry->m_pBinaryReader410);
        pEntry->m_pBinaryReader410 = nullptr;
    }
    if (pEntry->m_pSubObject418) {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        sub_e3aaf0(pEntry->m_pSubObject418);
        pAlloc->m_pVTable->Free(pAlloc, pEntry->m_pSubObject418);
        pEntry->m_pSubObject418 = nullptr;
    }
}

// ============================================================================
// 56. 0x2182d20 (2,239 B) - GParamManager_LoadGParamFileFromMemory
// Parses a `.gparam` binary image in memory, creates its lookup tables and value
// containers, inserts a `GParamLoadedFileNode` into `m_loadedFileMap2D8`, and
// returns the assigned 64-bit `fileKey` (or `0xFFFFFFFFFFFFFFFFULL` on failure).
// ============================================================================
extern "C" {
    void sub_218a220(GParamLoadedFileEntry* pThis);
    void sub_262a720(DLWString* pThis, const wchar_t* pszStr, size_t len);
    void sub_218e1b0(GXGParamGroupLookupTable* pThis, DLAllocator* pAlloc);
    void sub_e3e3b0(void* pParamContainer, DLAllocator* pAlloc);
    void sub_e3aa20(void* pSubObj418, DLAllocator* pAlloc);
    void sub_e3e9c0(void* pParamContainer, void* pSubObj28, GXGParamBinaryReader* pReader);
    void sub_e34010(void* pParamBinding, GXGParamGroupLookupTable* pLookup, void* pContainer);
    void sub_e3ab60(void* pSubObj418, void* pContainer, GXGParamGroupLookupTable* pLookup, void* pSubObj28);
    void sub_218be20(GParamLoadedFileEntry* pDst, const GParamLoadedFileEntry* pSrc);
    void sub_218dab0(void* pOutPair, GParamHashTable* pMap, const uint64_t* pKey, GParamBucketIterator nodeIter);
    void sub_26279a0(DLWString* pDst, const DLWString* pSrc, uint64_t pos, uint64_t count);
    void sub_2935150(DLFixedWString<64>* pDst, const DLFixedWString<64>* pSrc, uint64_t pos, uint64_t count);
}

uint64_t GParamManager_LoadGParamFileFromMemory(
    GParamManager* pThis,
    const void* pFileData,
    uint32_t fileSize)
{
    uint32_t serial = pThis->m_nextFileSerial458;
    pThis->m_nextFileSerial458 = serial + 1;
    uint64_t fileKey = (static_cast<uint64_t>(serial) << 32) | static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFileData));

    if (GParamManager_FindLoadedFileEntryByKey(pThis, fileKey) != nullptr) {
        return fileKey;
    }

    GParamLoadedFileEntry tempEntry;
    sub_218a220(&tempEntry);

    DLAllocator* pAlloc = pThis->m_pAllocator00;
    void* pRawCopy = pAlloc->m_pVTable->AllocateAligned(pAlloc, fileSize, 4);
    tempEntry.m_pRawFileData00 = pRawCopy;
    if (!pRawCopy) {
        GParamLoadedFileEntry_Destroy(&tempEntry);
        return 0xFFFFFFFFFFFFFFFFULL;
    }

    sub_261a1b0(pRawCopy, pFileData, fileSize);
    tempEntry.m_fileDataSize08 = fileSize;

    const wchar_t* pszEmpty = reinterpret_cast<const wchar_t*>(GX_VMA(0x49d85f0ULL));
    sub_262a720(&tempEntry.m_fileName10, pszEmpty, 0);
    sub_262a720(&tempEntry.m_secondaryName48, pszEmpty, 0);

    const wchar_t* pszSlot0Default = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c574eULL));
    sub_29354e0(&tempEntry.m_slotPath0_80, pszSlot0Default, sub_2bbf228(pszSlot0Default));

    const wchar_t* pszSlot1Default = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2a0eULL));
    sub_29354e0(&tempEntry.m_slotPath1_130, pszSlot1Default, sub_2bbf228(pszSlot1Default));

    const wchar_t* pszSlot2Default = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2a1eULL));
    sub_29354e0(&tempEntry.m_slotPath2_1E0, pszSlot2Default, sub_2bbf228(pszSlot2Default));
    sub_29354e0(&tempEntry.m_customOverridePath348, pszSlot2Default, sub_2bbf228(pszSlot2Default));
    tempEntry.m_bHasCustomPath340 = false;

    auto* pLookup = static_cast<GXGParamGroupLookupTable*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x88, 8));
    if (!pLookup) {
        tempEntry.m_pGroupLookup3F8 = nullptr;
        GParamManager_ReleaseLoadedFileSubObjects(pThis, &tempEntry);
        GParamLoadedFileEntry_Destroy(&tempEntry);
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    sub_218e1b0(pLookup, pThis->m_pAllocator00);
    tempEntry.m_pGroupLookup3F8 = pLookup;

    pAlloc = pThis->m_pAllocator00;
    void* pContainer = pAlloc->m_pVTable->AllocateAligned(pAlloc, 0xF0, 8);
    if (!pContainer) {
        tempEntry.m_pParamContainer400 = nullptr;
        GParamManager_ReleaseLoadedFileSubObjects(pThis, &tempEntry);
        GParamLoadedFileEntry_Destroy(&tempEntry);
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    sub_e3e3b0(pContainer, pThis->m_pAllocator00);
    tempEntry.m_pParamContainer400 = pContainer;

    pAlloc = pThis->m_pAllocator00;
    auto* pReader = static_cast<GXGParamBinaryReader*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x18, 8));
    if (!pReader) {
        tempEntry.m_pBinaryReader410 = nullptr;
        GParamManager_ReleaseLoadedFileSubObjects(pThis, &tempEntry);
        GParamLoadedFileEntry_Destroy(&tempEntry);
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    pReader->m_bBigEndian08 = 0;
    pReader->m_cursor0C = 0;
    pReader->m_pData10 = nullptr;
    pReader->m_pVTable00 = reinterpret_cast<GXGParamBinaryReader::VTable*>(GX_VMA(0x53b63f0ULL));
    tempEntry.m_pBinaryReader410 = pReader;

    pAlloc = pThis->m_pAllocator00;
    void* pSubObj = pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x68, 8);
    if (!pSubObj) {
        tempEntry.m_pSubObject418 = nullptr;
        GParamManager_ReleaseLoadedFileSubObjects(pThis, &tempEntry);
        GParamLoadedFileEntry_Destroy(&tempEntry);
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    sub_e3aa20(pSubObj, pThis->m_pAllocator00);
    tempEntry.m_pSubObject418 = pSubObj;

    pReader->m_pVTable00->Initialize(pReader, tempEntry.m_pRawFileData00, fileSize);
    sub_218e2b0(tempEntry.m_pGroupLookup3F8, pThis->m_pSubObject28, pReader);
    sub_e3e9c0(tempEntry.m_pParamContainer400, pThis->m_pSubObject28, pReader);

    pAlloc = pThis->m_pAllocator00;
    void* pBinding = pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x18, 8);
    if (!pBinding) {
        tempEntry.m_pParamBinding408 = nullptr;
        GParamManager_ReleaseLoadedFileSubObjects(pThis, &tempEntry);
        GParamLoadedFileEntry_Destroy(&tempEntry);
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    sub_e34010(pBinding, tempEntry.m_pGroupLookup3F8, tempEntry.m_pParamContainer400);
    tempEntry.m_pParamBinding408 = pBinding;

    sub_e3ab60(tempEntry.m_pSubObject418, tempEntry.m_pParamContainer400, tempEntry.m_pGroupLookup3F8, pThis->m_pSubObject28);

    GParamHashTable* pFileMap = &pThis->m_loadedFileMap2D8;
    uint64_t bucketIdx = GParamComputeBucketIndex(fileKey, pFileMap->m_bucketMask48, pFileMap->m_bucketCount50);
    auto* pHead = static_cast<GParamLoadedFileNode*>(pFileMap->m_pHeadNode08);
    GParamBucketIterator* pBuckets = pFileMap->m_buckets28.m_pBegin;
    auto* pCur = static_cast<GParamLoadedFileNode*>(pBuckets[bucketIdx * 2].m_pNode10);
    GParamLoadedFileNode* pTargetNode = pHead;

    if (pCur != pHead) {
        auto* pEnd = static_cast<GParamLoadedFileNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_fileKey < fileKey) {
            pCur = pCur->m_pNext;
        }
        if (pCur != pEnd && fileKey >= pCur->m_fileKey) {
            pTargetNode = pCur;
        }
    }

    if (pTargetNode == pHead) {
        GParamLoadedFileEntry defaultEntry;
        sub_218a220(&defaultEntry);
        GParamLoadedFileEntry copyEntry;
        sub_218be20(&copyEntry, &defaultEntry);

        DLAllocator* pNodeAlloc = pFileMap->m_pNodeAlloc18;
        GParamLoadedFileNode* pFirst = pHead->m_pNext;
        GParamLoadedFileNode* pPrev = pFirst->m_pPrev;
        auto* pNewNode = static_cast<GParamLoadedFileNode*>(
            pNodeAlloc->m_pVTable->AllocateAligned(pNodeAlloc, sizeof(GParamLoadedFileNode), 8));
        if (!pNewNode) {
            sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
        }
        pNewNode->m_pNext = pFirst ? pFirst : pNewNode;
        pNewNode->m_pPrev = pFirst ? pPrev : pNewNode;
        pNewNode->m_fileKey = fileKey;
        sub_218be20(&pNewNode->m_entry, &copyEntry);

        if (pFileMap->m_nodeCount10 == 0x3CAE759203CAE6ULL) {
            sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
        }
        pFileMap->m_nodeCount10++;
        pFirst->m_pPrev = pNewNode;
        pNewNode->m_pPrev->m_pNext = pNewNode;

        struct FileInsertResult {
            void*                 m_pReserved00;
            void*                 m_pReserved08;
            GParamLoadedFileNode* m_pNode10;
            uint8_t               m_bInserted18;
        } insRes;
        GParamBucketIterator fileNodeIter = {nullptr, nullptr, pHead->m_pNext};
        sub_218dab0(&insRes, pFileMap, &pHead->m_pNext->m_fileKey, fileNodeIter);
        pTargetNode = insRes.m_pNode10;

        GParamLoadedFileEntry_Destroy(&copyEntry);
        GParamLoadedFileEntry_Destroy(&defaultEntry);
    }

    GParamLoadedFileEntry* pDst = &pTargetNode->m_entry;
    pDst->m_fileDataSize08 = tempEntry.m_fileDataSize08;
    pDst->m_pRawFileData00 = tempEntry.m_pRawFileData00;
    pDst->m_fileName10.m_bOwnsMemory = tempEntry.m_fileName10.m_bOwnsMemory;

    if (pDst != &tempEntry) {
        pDst->m_fileName10.m_pAllocator->m_pVTable->NotifyAllocatorRef(pDst->m_fileName10.m_pAllocator);
        tempEntry.m_fileName10.m_pAllocator->m_pVTable->NotifyAllocatorRef(tempEntry.m_fileName10.m_pAllocator);
        sub_26279a0(&pDst->m_fileName10, &tempEntry.m_fileName10, 0, 0xFFFFFFFFFFFFFFFFULL);

        pDst->m_secondaryName48.m_bOwnsMemory = tempEntry.m_secondaryName48.m_bOwnsMemory;
        pDst->m_secondaryName48.m_pAllocator->m_pVTable->NotifyAllocatorRef(pDst->m_secondaryName48.m_pAllocator);
        tempEntry.m_secondaryName48.m_pAllocator->m_pVTable->NotifyAllocatorRef(tempEntry.m_secondaryName48.m_pAllocator);
        sub_26279a0(&pDst->m_secondaryName48, &tempEntry.m_secondaryName48, 0, 0xFFFFFFFFFFFFFFFFULL);

        pDst->m_slotPath0_80.m_padAA[0] = tempEntry.m_slotPath0_80.m_padAA[0];
        sub_2935150(&pDst->m_slotPath0_80, &tempEntry.m_slotPath0_80, 0, 0xFFFFFFFFFFFFFFFFULL);

        pDst->m_slotPath1_130.m_padAA[0] = tempEntry.m_slotPath1_130.m_padAA[0];
        sub_2935150(&pDst->m_slotPath1_130, &tempEntry.m_slotPath1_130, 0, 0xFFFFFFFFFFFFFFFFULL);

        pDst->m_slotPath2_1E0.m_padAA[0] = tempEntry.m_slotPath2_1E0.m_padAA[0];
        sub_2935150(&pDst->m_slotPath2_1E0, &tempEntry.m_slotPath2_1E0, 0, 0xFFFFFFFFFFFFFFFFULL);

        pDst->m_slotPath3_290.m_padAA[0] = tempEntry.m_slotPath3_290.m_padAA[0];
        sub_2935150(&pDst->m_slotPath3_290, &tempEntry.m_slotPath3_290, 0, 0xFFFFFFFFFFFFFFFFULL);
    }

    pDst->m_bHasCustomPath340 = tempEntry.m_bHasCustomPath340;
    pDst->m_customOverridePath348.m_padAA[0] = tempEntry.m_customOverridePath348.m_padAA[0];
    if (pDst != &tempEntry) {
        sub_2935150(&pDst->m_customOverridePath348, &tempEntry.m_customOverridePath348, 0, 0xFFFFFFFFFFFFFFFFULL);
    }

    pDst->m_pGroupLookup3F8 = tempEntry.m_pGroupLookup3F8;
    pDst->m_pParamContainer400 = tempEntry.m_pParamContainer400;
    pDst->m_pParamBinding408 = tempEntry.m_pParamBinding408;
    pDst->m_pBinaryReader410 = tempEntry.m_pBinaryReader410;
    pDst->m_pSubObject418 = tempEntry.m_pSubObject418;

    GParamLoadedFileEntry_Destroy(&tempEntry);
    return fileKey;
}

// ============================================================================
// 57. 0x2182480 (2,205 B) - GParamManager_RegisterLoadedFileInDebugMenus
// Appends `":/*.gparam"` (10 chars at `0x49400c3`) to `pszBaseDir`, notifies
// all open ParamEditor dialogs via `0xe35150`, and appends the search path to
// all 23 group entries (`0..22`) in `m_groupParamEditorMap4C0` via `0x27c1a20`.
// ============================================================================
extern "C" {
    void* sub_207d250();
    void  sub_262a310(DLWString* pThis, const wchar_t* pszStr, size_t len);
    void  sub_267b640(DLString* pThis, const char* pszStr, size_t len);
    void** sub_218a010(GParamHashTable* pMap, const uint64_t* pKey);
    void  sub_27c1a20(void* pGroupPropVectorOwner, const DLWString* pPathStr);
}

static inline DLAllocator* GetThreadLocalStringAllocator() {
    uintptr_t tlsBase = 0;
    tlsBase = gx_guest_tls_base();
    uint64_t tlsOffset = static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL));
    uint8_t* pTlsObj = *reinterpret_cast<uint8_t**>(tlsBase + tlsOffset);
    if (!pTlsObj) {
        sub_207d250();
        pTlsObj = *reinterpret_cast<uint8_t**>(tlsBase + tlsOffset);
    }
    auto* pAlloc = reinterpret_cast<DLAllocator*>(pTlsObj + 0x28);
    uint8_t caps[0x20];
    pAlloc->m_pVTable->GetMemoryCapabilities(caps, pAlloc, 0);
    if ((caps[0] & 0x20) == 0) {
        sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }
    return pAlloc;
}

void GParamManager_RegisterLoadedFileInDebugMenus(
    GParamManager* pThis,
    const wchar_t* pszBaseDir)
{
    auto* pEditorHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    for (GParamKeyValueNode* pEd = pEditorHead->m_pNext; pEd != pEditorHead; pEd = pEd->m_pNext) {
        DLWString tempWStr;
        tempWStr.m_pReserved00 = nullptr;
        tempWStr.m_data.m_inlineBuf[0] = L'\0';
        tempWStr.m_length = 0;
        tempWStr.m_capacity = 7;
        tempWStr.m_pAllocator = GetThreadLocalStringAllocator();
        size_t baseLen = (*pszBaseDir != L'\0') ? sub_2bbf228(pszBaseDir) : 0;
        sub_262a310(&tempWStr, pszBaseDir, baseLen);
        tempWStr.m_bOwnsMemory = 1;

        DLString suffixAStr;
        suffixAStr.m_pReserved00 = nullptr;
        suffixAStr.m_data.m_inlineBuf[0] = '\0';
        suffixAStr.m_length = 0;
        suffixAStr.m_capacity = 15;
        suffixAStr.m_pAllocator = GetThreadLocalStringAllocator();
        sub_267b640(&suffixAStr, reinterpret_cast<const char*>(GX_VMA(0x49400c3ULL)), 10);
        suffixAStr.m_bOwnsMemory = 0;

        GParamWString_AppendConvertedAsciiString(&tempWStr, &suffixAStr);
        if (suffixAStr.m_capacity >= 16) {
            suffixAStr.m_pAllocator->m_pVTable->Free(suffixAStr.m_pAllocator, suffixAStr.m_data.m_pHeapBuf);
        }

        const wchar_t* pszCombined = (tempWStr.m_capacity > 7)
            ? tempWStr.m_data.m_pHeapBuf
            : tempWStr.m_data.m_inlineBuf;

        DLAllocator* pDefAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL));
        if (!pDefAlloc) {
            pDefAlloc = sub_2ba2b10();
            *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL)) = pDefAlloc;
        }
        uint8_t caps[0x20];
        pDefAlloc->m_pVTable->GetMemoryCapabilities(caps, pDefAlloc, 0);
        if ((caps[0] & 0x20) == 0) {
            sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
        }

        DLWString editorArgStr;
        editorArgStr.m_pReserved00 = nullptr;
        editorArgStr.m_data.m_inlineBuf[0] = L'\0';
        editorArgStr.m_length = 0;
        editorArgStr.m_capacity = 7;
        editorArgStr.m_pAllocator = pDefAlloc;
        sub_262a720(&editorArgStr, pszCombined, tempWStr.m_length);
        editorArgStr.m_bOwnsMemory = tempWStr.m_bOwnsMemory;

        sub_e35150(pEd->m_pVal18, &editorArgStr);

        if (editorArgStr.m_capacity >= 8) {
            editorArgStr.m_pAllocator->m_pVTable->Free(editorArgStr.m_pAllocator, editorArgStr.m_data.m_pHeapBuf);
        }
        if (tempWStr.m_capacity >= 8) {
            tempWStr.m_pAllocator->m_pVTable->Free(tempWStr.m_pAllocator, tempWStr.m_data.m_pHeapBuf);
        }
    }

    DLWString mainWStr;
    mainWStr.m_pReserved00 = nullptr;
    mainWStr.m_data.m_inlineBuf[0] = L'\0';
    mainWStr.m_length = 0;
    mainWStr.m_capacity = 7;
    mainWStr.m_pAllocator = GetThreadLocalStringAllocator();
    size_t mainBaseLen = (*pszBaseDir != L'\0') ? sub_2bbf228(pszBaseDir) : 0;
    sub_262a310(&mainWStr, pszBaseDir, mainBaseLen);
    mainWStr.m_bOwnsMemory = 1;

    DLString mainSuffixAStr;
    mainSuffixAStr.m_pReserved00 = nullptr;
    mainSuffixAStr.m_data.m_inlineBuf[0] = '\0';
    mainSuffixAStr.m_length = 0;
    mainSuffixAStr.m_capacity = 15;
    mainSuffixAStr.m_pAllocator = GetThreadLocalStringAllocator();
    sub_267b640(&mainSuffixAStr, reinterpret_cast<const char*>(GX_VMA(0x49400c3ULL)), 10);
    mainSuffixAStr.m_bOwnsMemory = 0;

    GParamWString_AppendConvertedAsciiString(&mainWStr, &mainSuffixAStr);
    if (mainSuffixAStr.m_capacity >= 16) {
        mainSuffixAStr.m_pAllocator->m_pVTable->Free(mainSuffixAStr.m_pAllocator, mainSuffixAStr.m_data.m_pHeapBuf);
    }

    for (uint64_t groupIdx = 0; groupIdx < 23; ++groupIdx) {
        uint64_t bucketIdx = GParamComputeBucketIndex(
            groupIdx,
            pThis->m_groupParamEditorMap4C0.m_bucketMask48,
            pThis->m_groupParamEditorMap4C0.m_bucketCount50);
        auto* pGroupHead = static_cast<GParamKeyValueNode*>(pThis->m_groupParamEditorMap4C0.m_pHeadNode08);
        GParamBucketIterator* pBuckets = pThis->m_groupParamEditorMap4C0.m_buckets28.m_pBegin;
        auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
        GParamKeyValueNode* pFound = pGroupHead;

        if (pCur != pGroupHead) {
            auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
            while (pCur != pEnd && pCur->m_key10 < groupIdx) {
                pCur = pCur->m_pNext;
            }
            if (pCur != pEnd && groupIdx >= pCur->m_key10) {
                pFound = pCur;
            }
        }

        if (pFound == pGroupHead) {
            DLAllocator* pAlloc = pThis->m_pAllocator00;
            auto* pVecOwner = static_cast<uint64_t*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x28, 8));
            if (!pVecOwner) {
                break;
            }
            uint8_t caps[0x20];
            pAlloc->m_pVTable->GetMemoryCapabilities(caps, pAlloc, 0);
            if ((caps[0] & 0x20) == 0) {
                sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
            }
            pVecOwner[1] = 0;
            pVecOwner[2] = 0;
            pVecOwner[3] = 0;
            pVecOwner[4] = reinterpret_cast<uint64_t>(pAlloc);
            *sub_218a010(&pThis->m_groupParamEditorMap4C0, &groupIdx) = pVecOwner;
        }

        void* pTargetVecOwner = *sub_218a010(&pThis->m_groupParamEditorMap4C0, &groupIdx);
        const wchar_t* pszCombined = (mainWStr.m_capacity > 7)
            ? mainWStr.m_data.m_pHeapBuf
            : mainWStr.m_data.m_inlineBuf;

        DLAllocator* pDefAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL));
        if (!pDefAlloc) {
            pDefAlloc = sub_2ba2b10();
            *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL)) = pDefAlloc;
        }
        uint8_t caps[0x20];
        pDefAlloc->m_pVTable->GetMemoryCapabilities(caps, pDefAlloc, 0);
        if ((caps[0] & 0x20) == 0) {
            sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
        }

        DLWString groupPathStr;
        groupPathStr.m_pReserved00 = nullptr;
        groupPathStr.m_data.m_inlineBuf[0] = L'\0';
        groupPathStr.m_length = 0;
        groupPathStr.m_capacity = 7;
        groupPathStr.m_pAllocator = pDefAlloc;
        sub_262a720(&groupPathStr, pszCombined, mainWStr.m_length);
        groupPathStr.m_bOwnsMemory = mainWStr.m_bOwnsMemory;

        sub_27c1a20(pTargetVecOwner, &groupPathStr);

        if (groupPathStr.m_capacity >= 8) {
            groupPathStr.m_pAllocator->m_pVTable->Free(groupPathStr.m_pAllocator, groupPathStr.m_data.m_pHeapBuf);
        }
    }

    if (mainWStr.m_capacity >= 8) {
        mainWStr.m_pAllocator->m_pVTable->Free(mainWStr.m_pAllocator, mainWStr.m_data.m_pHeapBuf);
    }
}

// ============================================================================
// 58. 0x2182360 (280 B) - GParamManager_SetLoadedFileNameWithExtension
// Assigns `pszBaseName` + `L".gparam"` (`0x49c29f8`) to the loaded file entry
// for `fileKey`.
// ============================================================================
extern "C" void sub_26503b0(DLWString* pDst, const wchar_t* pszSuffix, size_t len);

void GParamManager_SetLoadedFileNameWithExtension(
    GParamManager* pThis,
    uint64_t fileKey,
    const wchar_t* pszBaseName)
{
    GParamLoadedFileEntry* pEntry = GParamManager_FindLoadedFileEntryByKey(pThis, fileKey);
    if (pEntry) {
        size_t baseLen = (*pszBaseName != L'\0') ? sub_2bbf228(pszBaseName) : 0;
        sub_262a720(&pEntry->m_fileName10, pszBaseName, baseLen);
        const wchar_t* pszExt = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c29f8ULL));
        sub_26503b0(&pEntry->m_fileName10, pszExt, sub_2bbf228(pszExt));
    }
}


// ============================================================================
// 59. 0x2181970 (2541 B) - GParamManager_CleanupAllResourcesAndMaps
// Releases all active file/group ParamEditors, resets interpolator and effect
// containers, clears loaded GParam files and blend groups/subgroups, and
// destroys `m_pSubObject28`.
// ============================================================================
extern "C" void sub_e34b70(void* pParamEditor);
extern "C" void sub_e34970(void* pParamEditor);
extern "C" void sub_e3e740(void* pInterpolatorState);
extern "C" void sub_e322d0(void* pSubObj);
extern "C" DLAllocator* sub_207b720(const void* pObj);

void GParamManager_CleanupAllResourcesAndMaps(GParamManager* pThis) {
    // 1. Clean up all file ParamEditors in m_fileParamEditorMap460 (+0x460)
    auto* pFileEdHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    GParamKeyValueNode* pFileEdNode = pFileEdHead->m_pNext;
    while (pFileEdNode != pFileEdHead) {
        sub_e34b70(pFileEdNode->m_pVal18);
        void* pEditor = pFileEdNode->m_pVal18;
        if (pEditor) {
            DLAllocator* pAlloc = pThis->m_pAllocator00;
            sub_e34970(pEditor);
            pAlloc->m_pVTable->Free(pAlloc, pEditor);
            pFileEdNode->m_pVal18 = nullptr;
        }

        uint64_t bucketIdx = GParamComputeBucketIndex(
            pFileEdNode->m_key10,
            pThis->m_fileParamEditorMap460.m_bucketMask48,
            pThis->m_fileParamEditorMap460.m_bucketCount50);
        GParamBucketIterator* pBuckets = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
        uint64_t bFirst = bucketIdx * 2;
        uint64_t bLast = bucketIdx * 2 + 1;
        if (pBuckets[bLast].m_pNode10 == pFileEdNode) {
            if (pBuckets[bFirst].m_pNode10 == pFileEdNode) {
                pBuckets[bFirst].m_pNode10 = pThis->m_fileParamEditorMap460.m_pHeadNode08;
                pBuckets[bLast].m_pNode10 = pThis->m_fileParamEditorMap460.m_pHeadNode08;
            } else {
                pBuckets[bLast].m_pNode10 = pFileEdNode->m_pPrev;
            }
        } else if (pBuckets[bFirst].m_pNode10 == pFileEdNode) {
            pBuckets[bFirst].m_pNode10 = pFileEdNode->m_pNext;
        }

        GParamKeyValueNode* pNext = pFileEdNode->m_pNext;
        if (pFileEdNode != pThis->m_fileParamEditorMap460.m_pHeadNode08) {
            pFileEdNode->m_pPrev->m_pNext = pNext;
            pFileEdNode->m_pNext->m_pPrev = pFileEdNode->m_pPrev;
            DLAllocator* pMapAlloc = pThis->m_fileParamEditorMap460.m_pAllocator18;
            pMapAlloc->m_pVTable->Free(pMapAlloc, pFileEdNode);
            pThis->m_fileParamEditorMap460.m_nodeCount10--;
        }
        pFileEdHead = static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
        pFileEdNode = pNext;
    }

    GParamKeyValueNode* pRemFileEd = pFileEdHead->m_pNext;
    pFileEdHead->m_pNext = pFileEdHead;
    static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08)->m_pPrev =
        static_cast<GParamKeyValueNode*>(pThis->m_fileParamEditorMap460.m_pHeadNode08);
    pThis->m_fileParamEditorMap460.m_nodeCount10 = 0;
    while (pRemFileEd != pThis->m_fileParamEditorMap460.m_pHeadNode08) {
        GParamKeyValueNode* pNext = pRemFileEd->m_pNext;
        DLAllocator* pMapAlloc = pThis->m_fileParamEditorMap460.m_pAllocator18;
        pMapAlloc->m_pVTable->Free(pMapAlloc, pRemFileEd);
        pRemFileEd = pNext;
    }

    pThis->m_fileParamEditorMap460.m_buckets28.m_pEnd = pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin;
    GParamBucketIterator emptyFileEdIter = {nullptr, nullptr, pThis->m_fileParamEditorMap460.m_pHeadNode08};
    GParamBucketIterator posFileEdIter = {nullptr, nullptr, pThis->m_fileParamEditorMap460.m_buckets28.m_pBegin};
    GParamBucketIterator dummyOut;
    GParamBucketVector_InsertFill_480(&dummyOut, reinterpret_cast<uint8_t*>(&pThis->m_fileParamEditorMap460) + 0x20, 16, &emptyFileEdIter, posFileEdIter);
    pThis->m_fileParamEditorMap460.m_bucketMask48 = 7;
    pThis->m_fileParamEditorMap460.m_bucketCount50 = 8;

    // 2. Clean up all 23 group ParamEditor string vectors in m_groupParamEditorMap4C0 (+0x4c0)
    for (uint64_t groupId = 0; groupId < 23; ++groupId) {
        uint64_t bucketIdx = GParamComputeBucketIndex(
            groupId,
            pThis->m_groupParamEditorMap4C0.m_bucketMask48,
            pThis->m_groupParamEditorMap4C0.m_bucketCount50);
        auto* pGroupEdHead = static_cast<GParamKeyValueNode*>(pThis->m_groupParamEditorMap4C0.m_pHeadNode08);
        GParamBucketIterator* pBuckets = pThis->m_groupParamEditorMap4C0.m_buckets28.m_pBegin;
        auto* pCur = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
        if (pCur == pGroupEdHead) {
            continue;
        }
        auto* pEnd = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        while (pCur != pEnd && pCur->m_key10 < groupId) {
            pCur = pCur->m_pNext;
        }
        if (pCur == pEnd || groupId < pCur->m_key10) {
            continue;
        }

        auto* pVecOwner = static_cast<GParamWStringVectorOwner*>(pCur->m_pVal18);
        if (pVecOwner) {
            DLAllocator* pOwnerAlloc = sub_207b720(pVecOwner);
            if (pVecOwner->m_pBegin08) {
                for (DLWString* pIt = pVecOwner->m_pBegin08; pIt != pVecOwner->m_pEnd10; ++pIt) {
                    if (pIt->m_capacity >= 8) {
                        pIt->m_pAllocator->m_pVTable->Free(pIt->m_pAllocator, pIt->m_data.m_pHeapBuf);
                    }
                    pIt->m_capacity = 7;
                    pIt->m_length = 0;
                    pIt->m_data.m_inlineBuf[0] = L'\0';
                }
                pVecOwner->m_pAllocator20->m_pVTable->Free(pVecOwner->m_pAllocator20, pVecOwner->m_pBegin08);
                pVecOwner->m_pBegin08 = nullptr;
                pVecOwner->m_pEnd10 = nullptr;
                pVecOwner->m_pCap18 = nullptr;
            }
            pOwnerAlloc->m_pVTable->Free(pOwnerAlloc, pVecOwner);
            pCur->m_pVal18 = nullptr;
        }

        uint64_t bIdx = GParamComputeBucketIndex(
            pCur->m_key10,
            pThis->m_groupParamEditorMap4C0.m_bucketMask48,
            pThis->m_groupParamEditorMap4C0.m_bucketCount50);
        GParamBucketIterator* pGBuckets = pThis->m_groupParamEditorMap4C0.m_buckets28.m_pBegin;
        uint64_t bFirst = bIdx * 2;
        uint64_t bLast = bIdx * 2 + 1;
        if (pGBuckets[bLast].m_pNode10 == pCur) {
            if (pGBuckets[bFirst].m_pNode10 == pCur) {
                pGBuckets[bFirst].m_pNode10 = pThis->m_groupParamEditorMap4C0.m_pHeadNode08;
                pGBuckets[bLast].m_pNode10 = pThis->m_groupParamEditorMap4C0.m_pHeadNode08;
            } else {
                pGBuckets[bLast].m_pNode10 = pCur->m_pPrev;
            }
        } else if (pGBuckets[bFirst].m_pNode10 == pCur) {
            pGBuckets[bFirst].m_pNode10 = pCur->m_pNext;
        }

        if (pCur != pThis->m_groupParamEditorMap4C0.m_pHeadNode08) {
            pCur->m_pPrev->m_pNext = pCur->m_pNext;
            pCur->m_pNext->m_pPrev = pCur->m_pPrev;
            DLAllocator* pMapAlloc = pThis->m_groupParamEditorMap4C0.m_pAllocator18;
            pMapAlloc->m_pVTable->Free(pMapAlloc, pCur);
            pThis->m_groupParamEditorMap4C0.m_nodeCount10--;
        }
    }

    // 3. Reset interpolator state and effect container
    sub_e3e740(&pThis->m_interpolatorState1B8);
    sub_218ea00(&pThis->m_effectContainer130);
    pThis->m_bEnabled2C8 = 0;
    pThis->m_flags2CC = 0;
    pThis->m_qword2D0 = 0;
    pThis->m_qword08 = 0;
    pThis->m_dword10 = 0;
    pThis->m_qword18 = 0;
    pThis->m_dword20 = 0;

    // 4. Clean up all loaded files in m_loadedFileMap2D8 (+0x2d8)
    auto* pFileHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    GParamLoadedFileNode* pFileCur = pFileHead->m_pNext;
    while (pFileCur != pThis->m_loadedFileMap2D8.m_pHeadNode08) {
        GParamManager_ReleaseLoadedFileSubObjects(pThis, &pFileCur->m_entry);
        GParamBucketIterator nextIt = {nullptr, nullptr, nullptr};
        GParamLoadedFileMap_EraseNode(&nextIt, &pThis->m_loadedFileMap2D8, pFileCur);
        pFileCur = static_cast<GParamLoadedFileNode*>(nextIt.m_pNode10);
    }

    pFileHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    GParamLoadedFileNode* pRemFile = pFileHead->m_pNext;
    pFileHead->m_pNext = pFileHead;
    pFileHead->m_pPrev = pFileHead;
    pThis->m_loadedFileMap2D8.m_nodeCount10 = 0;
    while (pRemFile != pThis->m_loadedFileMap2D8.m_pHeadNode08) {
        GParamLoadedFileNode* pNext = pRemFile->m_pNext;
        GParamLoadedFileEntry_Destroy(&pRemFile->m_entry);
        pThis->m_loadedFileMap2D8.m_pAllocator18->m_pVTable->Free(
            pThis->m_loadedFileMap2D8.m_pAllocator18,
            pRemFile);
        pRemFile = pNext;
    }

    pThis->m_loadedFileMap2D8.m_buckets28.m_pEnd = pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin;
    GParamBucketIterator emptyFileIter = {nullptr, nullptr, pThis->m_loadedFileMap2D8.m_pHeadNode08};
    GParamBucketIterator posFileIter = {nullptr, nullptr, pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin};
    GParamBucketVector_InsertFill_2F8(&dummyOut, reinterpret_cast<uint8_t*>(&pThis->m_loadedFileMap2D8) + 0x20, 16, &emptyFileIter, posFileIter);
    pThis->m_loadedFileMap2D8.m_bucketMask48 = 7;
    pThis->m_loadedFileMap2D8.m_bucketCount50 = 8;

    // 5. Clear m_blendSubGroupLookup398 (+0x398)
    auto* pLookupHead = static_cast<GParamKeyValueNode*>(pThis->m_blendSubGroupLookup398.m_pHeadNode08);
    GParamKeyValueNode* pLookupCur = pLookupHead->m_pNext;
    pLookupHead->m_pNext = pLookupHead;
    pLookupHead->m_pPrev = pLookupHead;
    pThis->m_blendSubGroupLookup398.m_nodeCount10 = 0;
    while (pLookupCur != pThis->m_blendSubGroupLookup398.m_pHeadNode08) {
        GParamKeyValueNode* pNext = pLookupCur->m_pNext;
        pThis->m_blendSubGroupLookup398.m_pAllocator18->m_pVTable->Free(
            pThis->m_blendSubGroupLookup398.m_pAllocator18,
            pLookupCur);
        pLookupCur = pNext;
    }
    pThis->m_blendSubGroupLookup398.m_buckets28.m_pEnd = pThis->m_blendSubGroupLookup398.m_buckets28.m_pBegin;
    GParamBucketIterator emptyLookupIter = {nullptr, nullptr, pThis->m_blendSubGroupLookup398.m_pHeadNode08};
    GParamBucketIterator posLookupIter = {nullptr, nullptr, pThis->m_blendSubGroupLookup398.m_buckets28.m_pBegin};
    GParamBucketVector_InsertFill_3B8(&dummyOut, reinterpret_cast<uint8_t*>(&pThis->m_blendSubGroupLookup398) + 0x20, 16, &emptyLookupIter, posLookupIter);
    pThis->m_blendSubGroupLookup398.m_bucketMask48 = 7;
    pThis->m_blendSubGroupLookup398.m_bucketCount50 = 8;

    // 6. Clear m_blendGroupMap338 (+0x338) and nested subgroup maps
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBlendGroupNode* pGroupCur = pGroupHead->m_pNext;
    pGroupHead->m_pNext = pGroupHead;
    pGroupHead->m_pPrev = pGroupHead;
    pThis->m_blendGroupMap338.m_nodeCount10 = 0;
    while (pGroupCur != pThis->m_blendGroupMap338.m_pHeadNode08) {
        GParamBlendGroupNode* pNextGroup = pGroupCur->m_pNext;
        if (pGroupCur->m_subGroupMap18.m_buckets28.m_pBegin) {
            pGroupCur->m_subGroupMap18.m_buckets28.m_pAllocator->m_pVTable->Free(
                pGroupCur->m_subGroupMap18.m_buckets28.m_pAllocator,
                pGroupCur->m_subGroupMap18.m_buckets28.m_pBegin);
            pGroupCur->m_subGroupMap18.m_buckets28.m_pBegin = nullptr;
            pGroupCur->m_subGroupMap18.m_buckets28.m_pEnd = nullptr;
            pGroupCur->m_subGroupMap18.m_buckets28.m_pCap = nullptr;
        }
        auto* pSubHead = static_cast<GParamBlendSubGroupNode*>(pGroupCur->m_subGroupMap18.m_pHeadNode08);
        GParamBlendSubGroupNode* pSubCur = pSubHead->m_pNext;
        pSubHead->m_pNext = pSubHead;
        pSubHead->m_pPrev = pSubHead;
        pGroupCur->m_subGroupMap18.m_nodeCount10 = 0;
        while (pSubCur != pGroupCur->m_subGroupMap18.m_pHeadNode08) {
            GParamBlendSubGroupNode* pNextSub = pSubCur->m_pNext;
            pGroupCur->m_subGroupMap18.m_pAllocator18->m_pVTable->Free(
                pGroupCur->m_subGroupMap18.m_pAllocator18,
                pSubCur);
            pSubCur = pNextSub;
        }
        pGroupCur->m_subGroupMap18.m_pAllocator18->m_pVTable->Free(
            pGroupCur->m_subGroupMap18.m_pAllocator18,
            pGroupCur->m_subGroupMap18.m_pHeadNode08);
        pThis->m_blendGroupMap338.m_pAllocator18->m_pVTable->Free(
            pThis->m_blendGroupMap338.m_pAllocator18,
            pGroupCur);
        pGroupCur = pNextGroup;
    }

    pThis->m_blendGroupMap338.m_buckets28.m_pEnd = pThis->m_blendGroupMap338.m_buckets28.m_pBegin;
    GParamBucketIterator emptyGroupIter = {nullptr, nullptr, pThis->m_blendGroupMap338.m_pHeadNode08};
    GParamBucketIterator posGroupIter = {nullptr, nullptr, pThis->m_blendGroupMap338.m_buckets28.m_pBegin};
    sub_218b1f0(&dummyOut, reinterpret_cast<uint8_t*>(&pThis->m_blendGroupMap338) + 0x20, 16, &emptyGroupIter, posGroupIter);
    pThis->m_blendGroupMap338.m_bucketMask48 = 7;
    pThis->m_blendGroupMap338.m_bucketCount50 = 8;

    // 7. Destroy and free m_pSubObject28 (+0x28)
    if (pThis->m_pSubObject28) {
        DLAllocator* pAlloc = pThis->m_pAllocator00;
        sub_e322d0(pThis->m_pSubObject28);
        pAlloc->m_pVTable->Free(pAlloc, pThis->m_pSubObject28);
        pThis->m_pSubObject28 = nullptr;
    }
}

// ============================================================================
// 60. 0x21813d0 (1433 B) - GParamManager_Destructor
// Full destructor for `GParamManager`: calls `0x2181970`, destroys all
// internal hash tables, subsystems, effect container, search path vector, and
// default directory strings.
// ============================================================================
extern "C" void sub_e3aaf0(void* pSubSystem520);
extern "C" void sub_e34020(void* pSubSystem2A8);
extern "C" void sub_e3e610(void* pInterpolatorState1B8);

static void GParamDestroySimpleHashTable(GParamHashTable* pTable) {
    if (pTable->m_buckets28.m_pBegin) {
        pTable->m_buckets28.m_pAllocator->m_pVTable->Free(
            pTable->m_buckets28.m_pAllocator,
            pTable->m_buckets28.m_pBegin);
        pTable->m_buckets28.m_pBegin = nullptr;
        pTable->m_buckets28.m_pEnd = nullptr;
        pTable->m_buckets28.m_pCap = nullptr;
    }
    auto* pHead = static_cast<GParamKeyValueNode*>(pTable->m_pHeadNode08);
    GParamKeyValueNode* pCur = pHead->m_pNext;
    pHead->m_pNext = pHead;
    pHead->m_pPrev = pHead;
    pTable->m_nodeCount10 = 0;
    while (pCur != pTable->m_pHeadNode08) {
        GParamKeyValueNode* pNext = pCur->m_pNext;
        pTable->m_pAllocator18->m_pVTable->Free(pTable->m_pAllocator18, pCur);
        pCur = pNext;
    }
    pTable->m_pAllocator18->m_pVTable->Free(pTable->m_pAllocator18, pTable->m_pHeadNode08);
}

void GParamManager_Destructor(GParamManager* pThis) {
    GParamManager_CleanupAllResourcesAndMaps(pThis);
    sub_e3aaf0(&pThis->m_subSystem520);

    GParamDestroySimpleHashTable(&pThis->m_groupParamEditorMap4C0);
    GParamDestroySimpleHashTable(&pThis->m_fileParamEditorMap460);
    GParamDestroySimpleHashTable(&pThis->m_blendMap400);
    GParamDestroySimpleHashTable(&pThis->m_blendSubGroupLookup398);

    // Destroy m_blendGroupMap338 (+0x338)
    if (pThis->m_blendGroupMap338.m_buckets28.m_pBegin) {
        pThis->m_blendGroupMap338.m_buckets28.m_pAllocator->m_pVTable->Free(
            pThis->m_blendGroupMap338.m_buckets28.m_pAllocator,
            pThis->m_blendGroupMap338.m_buckets28.m_pBegin);
        pThis->m_blendGroupMap338.m_buckets28.m_pBegin = nullptr;
        pThis->m_blendGroupMap338.m_buckets28.m_pEnd = nullptr;
        pThis->m_blendGroupMap338.m_buckets28.m_pCap = nullptr;
    }
    auto* pGroupHead = static_cast<GParamBlendGroupNode*>(pThis->m_blendGroupMap338.m_pHeadNode08);
    GParamBlendGroupNode* pGroupCur = pGroupHead->m_pNext;
    pGroupHead->m_pNext = pGroupHead;
    pGroupHead->m_pPrev = pGroupHead;
    pThis->m_blendGroupMap338.m_nodeCount10 = 0;
    while (pGroupCur != pThis->m_blendGroupMap338.m_pHeadNode08) {
        GParamBlendGroupNode* pNextGroup = pGroupCur->m_pNext;
        GParamDestroySimpleHashTable(&pGroupCur->m_subGroupMap18);
        pThis->m_blendGroupMap338.m_pAllocator18->m_pVTable->Free(
            pThis->m_blendGroupMap338.m_pAllocator18,
            pGroupCur);
        pGroupCur = pNextGroup;
    }
    pThis->m_blendGroupMap338.m_pAllocator18->m_pVTable->Free(
        pThis->m_blendGroupMap338.m_pAllocator18,
        pThis->m_blendGroupMap338.m_pHeadNode08);

    // Destroy m_loadedFileMap2D8 (+0x2d8)
    if (pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin) {
        pThis->m_loadedFileMap2D8.m_buckets28.m_pAllocator->m_pVTable->Free(
            pThis->m_loadedFileMap2D8.m_buckets28.m_pAllocator,
            pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin);
        pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin = nullptr;
        pThis->m_loadedFileMap2D8.m_buckets28.m_pEnd = nullptr;
        pThis->m_loadedFileMap2D8.m_buckets28.m_pCap = nullptr;
    }
    auto* pFileHead = static_cast<GParamLoadedFileNode*>(pThis->m_loadedFileMap2D8.m_pHeadNode08);
    GParamLoadedFileNode* pFileCur = pFileHead->m_pNext;
    pFileHead->m_pNext = pFileHead;
    pFileHead->m_pPrev = pFileHead;
    pThis->m_loadedFileMap2D8.m_nodeCount10 = 0;
    while (pFileCur != pThis->m_loadedFileMap2D8.m_pHeadNode08) {
        GParamLoadedFileNode* pNextFile = pFileCur->m_pNext;
        GParamLoadedFileEntry_Destroy(&pFileCur->m_entry);
        pThis->m_loadedFileMap2D8.m_pAllocator18->m_pVTable->Free(
            pThis->m_loadedFileMap2D8.m_pAllocator18,
            pFileCur);
        pFileCur = pNextFile;
    }
    pThis->m_loadedFileMap2D8.m_pAllocator18->m_pVTable->Free(
        pThis->m_loadedFileMap2D8.m_pAllocator18,
        pThis->m_loadedFileMap2D8.m_pHeadNode08);

    sub_e34020(&pThis->m_subSystem2A8);
    sub_e3e610(&pThis->m_interpolatorState1B8);
    GXGParamGroupLookupTable_Destroy(&pThis->m_effectContainer130);

    // Destroy m_searchPathVec100 (+0x100)
    if (pThis->m_searchPathVec100.m_pBegin08) {
        for (DLWString* pIt = pThis->m_searchPathVec100.m_pBegin08; pIt != pThis->m_searchPathVec100.m_pEnd10; ++pIt) {
            if (pIt->m_capacity >= 8) {
                pIt->m_pAllocator->m_pVTable->Free(pIt->m_pAllocator, pIt->m_data.m_pHeapBuf);
            }
            pIt->m_capacity = 7;
            pIt->m_length = 0;
            pIt->m_data.m_inlineBuf[0] = L'\0';
        }
        pThis->m_searchPathVec100.m_pAllocator20->m_pVTable->Free(
            pThis->m_searchPathVec100.m_pAllocator20,
            pThis->m_searchPathVec100.m_pBegin08);
        pThis->m_searchPathVec100.m_pBegin08 = nullptr;
        pThis->m_searchPathVec100.m_pEnd10 = nullptr;
        pThis->m_searchPathVec100.m_pCap18 = nullptr;
    }

    if (pThis->m_defaultSubDirStrC8.m_capacity >= 8) {
        pThis->m_defaultSubDirStrC8.m_pAllocator->m_pVTable->Free(
            pThis->m_defaultSubDirStrC8.m_pAllocator,
            pThis->m_defaultSubDirStrC8.m_data.m_pHeapBuf);
    }
    pThis->m_defaultSubDirStrC8.m_capacity = 7;
    pThis->m_defaultSubDirStrC8.m_length = 0;
    pThis->m_defaultSubDirStrC8.m_data.m_inlineBuf[0] = L'\0';

    if (pThis->m_defaultDirStr90.m_capacity >= 8) {
        pThis->m_defaultDirStr90.m_pAllocator->m_pVTable->Free(
            pThis->m_defaultDirStr90.m_pAllocator,
            pThis->m_defaultDirStr90.m_data.m_pHeapBuf);
    }
    pThis->m_defaultDirStr90.m_capacity = 7;
    pThis->m_defaultDirStr90.m_length = 0;
    pThis->m_defaultDirStr90.m_data.m_inlineBuf[0] = L'\0';
}

// ============================================================================
// 61. 0x2180750 (3193 B) - GParamManager_ResetAndInitializeDefaultState
// Cleans up all resources, recreates `m_pSubObject28`, resets editor maps,
// slot indices, search paths, effect container, and default directory strings.
// ============================================================================
extern "C" void sub_e32240(void* pSubObj, DLAllocator* pAlloc);
extern "C" void sub_218d4f0(
    void* pOut,
    GParamHashTable* pTable,
    void* pBucketHint,
    uint64_t bucketIdx,
    GParamBucketIterator nodeIter);
extern "C" void sub_218d7d0(
    void* pOut,
    GParamHashTable* pTable,
    void* pBucketHint,
    uint64_t bucketIdx,
    GParamBucketIterator nodeIter);
extern "C" void sub_272b310(
    void* pIterOut,
    GParamWStringVectorOwner* pVec,
    GParamBucketIterator firstIter,
    GParamBucketIterator lastIter);
extern "C" void sub_e3ab60(void* pSubSystem520, void* pInterp1B8, GParamEffectContainer* pEff130, void* pSubObj28);
extern "C" void sub_e3abb0(void* pSubSystem520, const void* pStr1, const void* pStr2);

static void GParamCopyAssignHashTable(
    GParamHashTable* pDst,
    GParamHashTable* pSrc,
    bool bIsGroupMap)
{
    if (pDst == pSrc) {
        return;
    }
    pDst->m_bucketMask48 = pSrc->m_bucketMask48;
    pDst->m_bucketCount50 = pSrc->m_bucketCount50;
    pDst->m_maxLoadFactor58 = pSrc->m_maxLoadFactor58;

    auto* pDstHead = static_cast<GParamKeyValueNode*>(pDst->m_pHeadNode08);
    GParamKeyValueNode* pOldCur = pDstHead->m_pNext;
    pDstHead->m_pNext = pDstHead;
    pDstHead->m_pPrev = pDstHead;
    pDst->m_nodeCount10 = 0;
    while (pOldCur != pDst->m_pHeadNode08) {
        GParamKeyValueNode* pNext = pOldCur->m_pNext;
        pDst->m_pAllocator18->m_pVTable->Free(pDst->m_pAllocator18, pOldCur);
        pOldCur = pNext;
    }

    pDst->m_pAllocator18->m_pVTable->GetAllocatorId(pDst->m_pAllocator18);
    pSrc->m_pAllocator18->m_pVTable->GetAllocatorId(pSrc->m_pAllocator18);

    uint64_t srcBucketElemCount = static_cast<uint64_t>(pSrc->m_buckets28.m_pEnd - pSrc->m_buckets28.m_pBegin);
    pDst->m_buckets28.m_pEnd = pDst->m_buckets28.m_pBegin;
    GParamBucketIterator emptyIt = {nullptr, nullptr, pDst->m_pHeadNode08};
    GParamBucketIterator posDstIt = {nullptr, nullptr, pDst->m_buckets28.m_pBegin};
    GParamBucketIterator dummyIt;
    if (!bIsGroupMap) {
        GParamBucketVector_InsertFill_480(&dummyIt, reinterpret_cast<uint8_t*>(pDst) + 0x20, srcBucketElemCount, &emptyIt, posDstIt);
    } else {
        sub_218b740(&dummyIt, reinterpret_cast<uint8_t*>(pDst) + 0x20, srcBucketElemCount, &emptyIt, posDstIt);
    }

    auto* pSrcHead = static_cast<GParamKeyValueNode*>(pSrc->m_pHeadNode08);
    for (GParamKeyValueNode* pSrcNode = pSrcHead->m_pNext; pSrcNode != pSrcHead; pSrcNode = pSrcNode->m_pNext) {
        uint64_t bucketIdx = GParamComputeBucketIndex(pSrcNode->m_key10, pDst->m_bucketMask48, pDst->m_bucketCount50);
        auto* pHead = static_cast<GParamKeyValueNode*>(pDst->m_pHeadNode08);
        GParamBucketIterator* pBuckets = pDst->m_buckets28.m_pBegin;
        auto* pBucketHint = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2].m_pNode10);
        GParamKeyValueNode* pScan = pHead;
        if (pBucketHint != pHead) {
            pScan = static_cast<GParamKeyValueNode*>(pBuckets[bucketIdx * 2 + 1].m_pNode10)->m_pNext;
        }
        while (pScan != pBucketHint) {
            pScan = pScan->m_pPrev;
            if (pSrcNode->m_key10 >= pScan->m_key10) {
                if (pScan->m_key10 >= pSrcNode->m_key10) {
                    goto next_src_node;
                }
                pBucketHint = pScan->m_pNext;
                break;
            }
        }

        {
            GParamKeyValueNode* pInsertPos = pHead->m_pNext;
            GParamKeyValueNode* pPrevNode = pInsertPos->m_pPrev;
            auto* pNewNode = static_cast<GParamKeyValueNode*>(
                pDst->m_pAllocator18->m_pVTable->AllocateAligned(pDst->m_pAllocator18, 0x20, 8));
            if (!pNewNode) {
                if (*GetThreadLocalErrorFlag(0x53e4318) != 0) {
                    sub_2bbe978(GetThreadLocalSlot(0x53e4320), 1);
                }
                sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
            }
            pNewNode->m_pNext = pInsertPos ? pInsertPos : pNewNode;
            pNewNode->m_pPrev = pInsertPos ? pPrevNode : pNewNode;
            pNewNode->m_key10 = pSrcNode->m_key10;
            pNewNode->m_pVal18 = pSrcNode->m_pVal18;

            if (pDst->m_nodeCount10 == 0x07FFFFFFFFFFFFFEULL) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            pDst->m_nodeCount10++;
            pInsertPos->m_pPrev = pNewNode;
            pNewNode->m_pPrev->m_pNext = pNewNode;

            uint8_t insOut[0x20];
            GParamBucketIterator firstNodeIt = {nullptr, nullptr, static_cast<GParamKeyValueNode*>(pDst->m_pHeadNode08)->m_pNext};
            if (!bIsGroupMap) {
                sub_218d4f0(insOut, pDst, pBucketHint, bucketIdx, firstNodeIt);
            } else {
                sub_218d7d0(insOut, pDst, pBucketHint, bucketIdx, firstNodeIt);
            }
        }
    next_src_node:;
    }
}

bool GParamManager_ResetAndInitializeDefaultState(GParamManager* pThis) {
    GParamManager_CleanupAllResourcesAndMaps(pThis);

    DLAllocator* pAlloc = pThis->m_pAllocator00;
    void* pSubObj = pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x30, 8);
    if (pSubObj) {
        sub_e32240(pSubObj, pThis->m_pAllocator00);
    }
    pThis->m_pSubObject28 = pSubObj;
    pThis->m_qword08 = 0;
    pThis->m_dword10 = 0;
    pThis->m_qword18 = 0;
    pThis->m_dword20 = 0;

    // Reinitialize m_fileParamEditorMap460 (+0x460)
    DLAllocator* pMapAlloc = pThis->m_pAllocator00;
    uint8_t caps[0x20];
    pMapAlloc->m_pVTable->GetMemoryCapabilities(caps, pMapAlloc, 0);
    if ((caps[0] & 0x20) == 0) {
        sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }
    GParamHashTable tmpFileEdMap;
    GParamHashTable_Construct_460(&tmpFileEdMap, &pMapAlloc);
    GParamCopyAssignHashTable(&pThis->m_fileParamEditorMap460, &tmpFileEdMap, false);
    GParamDestroySimpleHashTable(&tmpFileEdMap);

    // Reinitialize m_groupParamEditorMap4C0 (+0x4c0)
    pMapAlloc = pThis->m_pAllocator00;
    pMapAlloc->m_pVTable->GetMemoryCapabilities(caps, pMapAlloc, 0);
    if ((caps[0] & 0x20) == 0) {
        sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }
    GParamHashTable tmpGroupEdMap;
    GParamHashTable_Construct_4C0(&tmpGroupEdMap, &pMapAlloc);
    GParamCopyAssignHashTable(&pThis->m_groupParamEditorMap4C0, &tmpGroupEdMap, true);
    GParamDestroySimpleHashTable(&tmpGroupEdMap);

    // Reset editor slot arrays (+0x588..+0x63F) and active blend IDs (+0x30..+0x8B)
    for (uint32_t i = 0; i < 23; ++i) {
        pThis->m_editorSlotIndex5E4[i] = -1;
        pThis->m_editorSlotSubGroup588[i] = 0;
        pThis->m_activeBlendIds30[i] = -1;
    }

    // Clear m_searchPathVec100 (+0x100)
    uint8_t dummyIter[0x20];
    GParamBucketIterator firstPathIt = {nullptr, nullptr, pThis->m_searchPathVec100.m_pBegin08};
    GParamBucketIterator lastPathIt = {nullptr, nullptr, pThis->m_searchPathVec100.m_pEnd10};
    sub_272b310(
        dummyIter,
        &pThis->m_searchPathVec100,
        firstPathIt,
        lastPathIt);

    // Reinitialize effect container and subsystem 520
    sub_218e2b0(&pThis->m_effectContainer130, pThis->m_pSubObject28, nullptr);
    sub_e3ab60(
        &pThis->m_subSystem520,
        &pThis->m_interpolatorState1B8,
        &pThis->m_effectContainer130,
        pThis->m_pSubObject28);
    sub_e3abb0(
        &pThis->m_subSystem520,
        reinterpret_cast<const void*>(GX_VMA(0x49c2996ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c29a8ULL)));

    // Reset default directory strings (+0x90 and +0xC8)
    const wchar_t* pszPrefix = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c29d8ULL));
    const wchar_t* pszDirSuffix = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c29baULL));
    const wchar_t* pszSubDirSuffix = reinterpret_cast<const wchar_t*>(GX_VMA(0x49c29e0ULL));

    sub_262a720(&pThis->m_defaultDirStr90, pszPrefix, sub_2bbf228(pszPrefix));
    sub_26503b0(&pThis->m_defaultDirStr90, pszDirSuffix, sub_2bbf228(pszDirSuffix));

    sub_262a720(&pThis->m_defaultSubDirStrC8, pszPrefix, sub_2bbf228(pszPrefix));
    sub_26503b0(&pThis->m_defaultSubDirStrC8, pszSubDirSuffix, sub_2bbf228(pszSubDirSuffix));

    return true;
}

// ============================================================================
// 62. 0x2180130 (1566 B) - GParamManager_Construct
// Full constructor for `GParamManager`: initializes all strings, vectors,
// effect container, interpolator state, hash tables, and calls `0x2180750`.
// ============================================================================
extern "C" void sub_e3e3b0(void* pInterpolatorState1B8, DLAllocator* pAlloc);
extern "C" void sub_e34010(void* pSubSystem2A8, GParamEffectContainer* pEff130, void* pInterp1B8);
extern "C" void sub_e3aa20(void* pSubSystem520, DLAllocator* pAlloc);

static DLAllocator* GParamGetCheckedDefaultAllocator() {
    DLAllocator* pAlloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL));
    if (!pAlloc) {
        pAlloc = sub_2ba2b10();
        *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL)) = pAlloc;
    }
    uint8_t caps[0x20];
    pAlloc->m_pVTable->GetMemoryCapabilities(caps, pAlloc, 0);
    if ((caps[0] & 0x20) == 0) {
        sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }
    return pAlloc;
}

static void GParamVerifyAllocatorCapability(DLAllocator* pAlloc) {
    uint8_t caps[0x20];
    pAlloc->m_pVTable->GetMemoryCapabilities(caps, pAlloc, 0);
    if ((caps[0] & 0x20) == 0) {
        sub_20b55b0(0, 0x3E, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }
}

void GParamManager_Construct(GParamManager* pThis, DLAllocator* pAlloc) {
    pThis->m_pAllocator00 = pAlloc;
    pThis->m_pSubObject28 = nullptr;

    // Initialize m_defaultDirStr90 (+0x90)
    DLAllocator* pDefAlloc1 = GParamGetCheckedDefaultAllocator();
    pThis->m_defaultDirStr90.m_data.m_inlineBuf[0] = L'\0';
    pThis->m_defaultDirStr90.m_length = 0;
    pThis->m_defaultDirStr90.m_capacity = 7;
    pThis->m_defaultDirStr90.m_pAllocator = pDefAlloc1;
    pThis->m_defaultDirStr90.m_bOwnsMemory = 1;

    // Initialize m_defaultSubDirStrC8 (+0xc8)
    DLAllocator* pDefAlloc2 = GParamGetCheckedDefaultAllocator();
    pThis->m_defaultSubDirStrC8.m_data.m_inlineBuf[0] = L'\0';
    pThis->m_defaultSubDirStrC8.m_length = 0;
    pThis->m_defaultSubDirStrC8.m_capacity = 7;
    pThis->m_defaultSubDirStrC8.m_pAllocator = pDefAlloc2;
    pThis->m_defaultSubDirStrC8.m_bOwnsMemory = 1;

    // Initialize m_searchPathVec100 (+0x100)
    DLAllocator* pDefAlloc3 = GParamGetCheckedDefaultAllocator();
    pThis->m_searchPathVec100.m_pBegin08 = nullptr;
    pThis->m_searchPathVec100.m_pEnd10 = nullptr;
    pThis->m_searchPathVec100.m_pCap18 = nullptr;
    pThis->m_searchPathVec100.m_pAllocator20 = pDefAlloc3;

    // Construct m_effectContainer130 (+0x130), m_interpolatorState1B8 (+0x1b8), m_subSystem2A8 (+0x2a8)
    sub_218e1b0(&pThis->m_effectContainer130, pAlloc);
    sub_e3e3b0(&pThis->m_interpolatorState1B8, pAlloc);
    sub_e34010(&pThis->m_subSystem2A8, &pThis->m_effectContainer130, &pThis->m_interpolatorState1B8);

    pThis->m_bEnabled2C8 = 0;
    pThis->m_flags2CC = 0;
    pThis->m_qword2D0 = 0;
    pThis->m_pVTable2C0 = reinterpret_cast<void*>(GX_VMA(0x53b63f0ULL));

    // Construct m_loadedFileMap2D8 (+0x2d8, sentinel 0x438 B)
    GParamVerifyAllocatorCapability(pAlloc);
    pThis->m_loadedFileMap2D8.m_pHeadNode08 = nullptr;
    pThis->m_loadedFileMap2D8.m_nodeCount10 = 0;
    pThis->m_loadedFileMap2D8.m_pAllocator18 = pAlloc;
    auto* pFileSentinel = static_cast<GParamLoadedFileNode*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x438, 8));
    if (pFileSentinel) {
        pFileSentinel->m_pNext = pFileSentinel;
    } else {
        if (*GetThreadLocalErrorFlag(0x53e4318) != 0) {
            sub_2bbe978(GetThreadLocalSlot(0x53e4320), 1);
        }
        sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }
    pFileSentinel->m_pPrev = pFileSentinel;
    pThis->m_loadedFileMap2D8.m_pHeadNode08 = pFileSentinel;
    pThis->m_loadedFileMap2D8.m_buckets28.m_pBegin = nullptr;
    pThis->m_loadedFileMap2D8.m_buckets28.m_pEnd = nullptr;
    pThis->m_loadedFileMap2D8.m_buckets28.m_pCap = nullptr;
    pThis->m_loadedFileMap2D8.m_buckets28.m_pAllocator = pAlloc;
    pThis->m_loadedFileMap2D8.m_maxLoadFactor58 = 1.0f;
    GParamBucketIterator fileEmptyIt = {nullptr, nullptr, pFileSentinel};
    GParamBucketIterator filePosIt = {nullptr, nullptr, nullptr};
    GParamBucketIterator dummyIt;
    GParamBucketVector_InsertFill_2F8(&dummyIt, reinterpret_cast<uint8_t*>(&pThis->m_loadedFileMap2D8) + 0x20, 16, &fileEmptyIt, filePosIt);
    pThis->m_loadedFileMap2D8.m_bucketMask48 = 7;
    pThis->m_loadedFileMap2D8.m_bucketCount50 = 8;

    // Construct m_blendGroupMap338 (+0x338, sentinel 0x78 B)
    GParamVerifyAllocatorCapability(pAlloc);
    pThis->m_blendGroupMap338.m_pHeadNode08 = nullptr;
    pThis->m_blendGroupMap338.m_nodeCount10 = 0;
    pThis->m_blendGroupMap338.m_pAllocator18 = pAlloc;
    auto* pGroupSentinel = static_cast<GParamBlendGroupNode*>(pAlloc->m_pVTable->AllocateAligned(pAlloc, 0x78, 8));
    if (pGroupSentinel) {
        pGroupSentinel->m_pNext = pGroupSentinel;
    } else {
        if (*GetThreadLocalErrorFlag(0x53e4318) != 0) {
            sub_2bbe978(GetThreadLocalSlot(0x53e4320), 1);
        }
        sub_20b55b0(0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
    }
    pGroupSentinel->m_pPrev = pGroupSentinel;
    pThis->m_blendGroupMap338.m_pHeadNode08 = pGroupSentinel;
    pThis->m_blendGroupMap338.m_buckets28.m_pBegin = nullptr;
    pThis->m_blendGroupMap338.m_buckets28.m_pEnd = nullptr;
    pThis->m_blendGroupMap338.m_buckets28.m_pCap = nullptr;
    pThis->m_blendGroupMap338.m_buckets28.m_pAllocator = pAlloc;
    pThis->m_blendGroupMap338.m_maxLoadFactor58 = 1.0f;
    GParamBucketIterator groupEmptyIt = {nullptr, nullptr, pGroupSentinel};
    GParamBucketIterator groupPosIt = {nullptr, nullptr, nullptr};
    sub_218b1f0(&dummyIt, reinterpret_cast<uint8_t*>(&pThis->m_blendGroupMap338) + 0x20, 16, &groupEmptyIt, groupPosIt);
    pThis->m_blendGroupMap338.m_bucketMask48 = 7;
    pThis->m_blendGroupMap338.m_bucketCount50 = 8;

    // Construct m_blendSubGroupLookup398 (+0x398) and m_blendMap400 (+0x3f8)
    DLAllocator* pSubAlloc = pAlloc;
    GParamVerifyAllocatorCapability(pSubAlloc);
    GParamHashTable_Construct_398(&pThis->m_blendSubGroupLookup398, &pSubAlloc);

    pSubAlloc = pAlloc;
    GParamVerifyAllocatorCapability(pSubAlloc);
    GParamHashTable_Construct_398(&pThis->m_blendMap400, &pSubAlloc);
    pThis->m_dword458 = 0;

    // Construct m_fileParamEditorMap460 (+0x460) and m_groupParamEditorMap4C0 (+0x4c0)
    DLAllocator* pDefMapAlloc = GParamGetCheckedDefaultAllocator();
    GParamHashTable_Construct_460(&pThis->m_fileParamEditorMap460, &pDefMapAlloc);

    pDefMapAlloc = GParamGetCheckedDefaultAllocator();
    GParamHashTable_Construct_4C0(&pThis->m_groupParamEditorMap4C0, &pDefMapAlloc);

    // Construct m_subSystem520 (+0x520) and initialize default state
    sub_e3aa20(&pThis->m_subSystem520, pAlloc);
    pThis->m_byte128 = 0;
    GParamManager_ResetAndInitializeDefaultState(pThis);
}

// ============================================================================
// GXSurfaceTexture & GXStructuredBuffer Structures
// ============================================================================

struct GXRefCountedSurface {
    void (**m_pVTable00)(GXRefCountedSurface*); // +0x00
    DLAllocator* m_pAllocator08;                // +0x08
    volatile int32_t m_refCount10;              // +0x10
    uint32_t m_pad14;                           // +0x14
    uint8_t  m_pad18[0x14];                     // +0x18
    uint32_t m_pitchOrSize2C;                   // +0x2C
    uint8_t  m_pad30[0x1B];                     // +0x30
    uint8_t  m_surfaceFormat4B;                 // +0x4B
};

struct GXTextureContainerVTable {
    void* m_pReserved00[7];
    void (*GetDimensionInfo38)(void* pContainer, uint32_t mipIndex, void** ppDataOut, uint32_t* pSizeOut); // +0x38
    uint32_t (*GetMipCount40)(void* pContainer);                                                           // +0x40
    void* m_pReserved48[3];
    int32_t (*AcquireSurfaceAndSrv60)(void* pContainer, GXRefCountedSurface** ppSurfOut, void** ppSrvOut, uint32_t flags); // +0x60
};

struct GXTextureContainer {
    GXTextureContainerVTable* m_pVTable00;
};

struct GXSurfaceTextureVTable {
    uint8_t m_pad00[0x90];
    void (*ReleaseViewsAndSurface90)(struct GXSurfaceTexture* pThis); // +0x90
};

struct GXSurfaceTexture {
    void*                m_pVTable00;      // +0x00 (0x53b62c0, base 0x53b6b00)
    uint32_t             m_flags08;        // +0x08
    uint32_t             m_pad0C;          // +0x0C
    float                m_clearColor10[4];// +0x10
    float                m_defaultVec20[4];// +0x20 (initialized from 0x49205c0)
    void*                m_pResource30;    // +0x30
    uint64_t             m_pad38;          // +0x38
    GXRefCountedSurface* m_pSurface40;     // +0x40
    void*                m_pSrvView48;     // +0x48
    void*                m_pRtvOrDsvView50;// +0x50
    uint32_t             m_pitchOrSize58;  // +0x58
    uint32_t             m_pad5C;          // +0x5C
};

struct GXStructuredBuffer {
    void*    m_pBufferResource00; // +0x00
    uint32_t m_totalByteSize08;   // +0x08
    uint32_t m_elementStride0C;   // +0x0C
};

extern "C" void sub_21a8350(float* pDstVec4, const float* pSrcVec4);
extern "C" int32_t sub_21669a0(void* pDevice, void** ppSrvOut, GXRefCountedSurface* pSurface, const void* pSrvDesc, void* pArg5);
extern "C" int32_t sub_2166300(void* pDevice, GXRefCountedSurface** ppSurfOut, const void* pTexDesc, void* pInitData, void* pArg5, void* pArg6, void* pArg7);
extern "C" int32_t sub_2166b60(void* pDevice, void** ppRtvDsvOut, GXRefCountedSurface* pSurface, const void* pViewDesc, uint32_t flags);
extern "C" void sub_2192660(void* pResource);
extern "C" void sub_2168a20(void* pCmdCtx, void* pUploadRing, void* pBufferRes, uint32_t offset, uint32_t flags, const void* pSrcData, uint32_t zero0, uint32_t zero1);
extern "C" int32_t sub_2165c10(void* pDevice, GXStructuredBuffer* pBufOut, const void* pBufDesc, const void* pInitDataDesc, uint32_t flags, void* pAlloc, void* pZero);

// ============================================================================
// 63. 0x217fef0 (426 B) - GXSurfaceTexture_InitializeFromTextureContainer
// Releases existing views/surface (`vtable+0x90`), acquires a surface and SRV
// from `pContainer` (`vtable+0x60`), extracts border/clear float data for
// formats `0x46..0x4E` (bitmask `0x1C7`), and updates reference counts.
// ============================================================================
bool GXSurfaceTexture_InitializeFromTextureContainer(
    GXSurfaceTexture* pThis,
    GXTextureContainer* pContainer)
{
    reinterpret_cast<GXSurfaceTextureVTable*>(pThis->m_pVTable00)->ReleaseViewsAndSurface90(pThis);

    GXRefCountedSurface* pNewSurf = nullptr;
    void* pNewSrv = nullptr;
    int32_t hr = pContainer->m_pVTable00->AcquireSurfaceAndSrv60(pContainer, &pNewSurf, &pNewSrv, 0);

    GXRefCountedSurface* pOldSurf = pThis->m_pSurface40;
    if (pOldSurf != pNewSurf) {
        if (pOldSurf) {
            if (__sync_fetch_and_add(&pOldSurf->m_refCount10, -1) == 1) {
                DLAllocator* pAlloc = pOldSurf->m_pAllocator08;
                pOldSurf->m_pVTable00[0](pOldSurf);
                pAlloc->m_pVTable->Free(pAlloc, pOldSurf);
            }
            pThis->m_pSurface40 = nullptr;
        }
        pThis->m_pSurface40 = pNewSurf;
        if (pNewSurf) {
            __sync_fetch_and_add(&pNewSurf->m_refCount10, 1);
        }
    }

    if (hr == 0 && pNewSurf != nullptr) {
        uint32_t fmtOffset = static_cast<uint32_t>(pNewSurf->m_surfaceFormat4B) - 0x46u;
        if (fmtOffset <= 8u && ((0x1C7u >> fmtOffset) & 1u) != 0) {
            uint32_t mipCount = pContainer->m_pVTable00->GetMipCount40(pContainer);
            if (mipCount != 0) {
                uint32_t dataSize = 0;
                float* pMipData = nullptr;
                pContainer->m_pVTable00->GetDimensionInfo38(
                    pContainer,
                    mipCount - 1,
                    reinterpret_cast<void**>(&pMipData),
                    &dataSize);
                if (dataSize == 4 && pMipData != nullptr) {
                    float val = pMipData[0];
                    pThis->m_clearColor10[0] = val;
                    alignas(16) float vec4[4] = {
                        val,
                        val,
                        val,
                        *reinterpret_cast<const float*>(GX_VMA(0x4929594ULL))
                    };
                    sub_21a8350(pThis->m_clearColor10, vec4);
                }
            }
        }
        pThis->m_pitchOrSize58 = pNewSurf->m_pitchOrSize2C;
    }

    if (pNewSurf != nullptr) {
        if (__sync_fetch_and_add(&pNewSurf->m_refCount10, -1) == 1) {
            DLAllocator* pAlloc = pNewSurf->m_pAllocator08;
            pNewSurf->m_pVTable00[0](pNewSurf);
            pAlloc->m_pVTable->Free(pAlloc, pNewSurf);
        }
    }

    pThis->m_pSrvView48 = pNewSrv;
    return (hr == 0);
}

// ============================================================================
// 64. 0x217fe60 (135 B) - GXSurfaceTexture_CreateShaderResourceView
// Creates a Shader Resource View (`m_pSrvView48`) for `m_pSurface40` via
// `sub_21669a0` using view type `8`.
// ============================================================================
int32_t GXSurfaceTexture_CreateShaderResourceView(
    GXSurfaceTexture* pThis,
    uint32_t format,
    uint32_t mipLevels,
    void* pExtraArg)
{
    if (!pThis->m_pSurface40) {
        return 5;
    }
    struct SrvDesc {
        uint32_t m_format00;
        uint32_t m_viewType04;
        uint32_t m_mostDetailedMip08;
        uint32_t m_mipLevels0C;
        uint64_t m_reserved10;
    } desc = {format, 8u, 0u, mipLevels, 0ULL};

    void* pDevice = **reinterpret_cast<void***>(GX_VMA(0x55406c8ULL));
    return sub_21669a0(pDevice, &pThis->m_pSrvView48, pThis->m_pSurface40, &desc, pExtraArg);
}

// ============================================================================
// 65. 0x217fba0 (702 B) - GXSurfaceTexture_CreateTexture2DAndViews
// Releases existing views, creates a 2D surface via `sub_2166300`, creates its
// SRV via `sub_21669a0`, and optionally creates a depth/render view (`+0x50`)
// via `sub_2166b60` when `bCreateTargetView != 0`.
// ============================================================================
bool GXSurfaceTexture_CreateTexture2DAndViews(
    GXSurfaceTexture* pThis,
    uint32_t width,
    uint32_t height,
    uint32_t arraySizeOrDepth,
    uint32_t format,
    uint32_t mipLevels,
    void* pInitData,
    bool bCreateTargetView,
    void* pCustomArg1,
    void* pCustomArg2)
{
    reinterpret_cast<GXSurfaceTextureVTable*>(pThis->m_pVTable00)->ReleaseViewsAndSurface90(pThis);

    uint32_t surfaceFormat = (format == 0x2Eu) ? 0x2Cu : format;
    uint32_t bindFlags = bCreateTargetView ? 0x88u : 0x08u;

    struct Tex2DDesc {
        uint32_t m_width00;
        uint32_t m_height04;
        uint32_t m_depthOrArray08;
        uint32_t m_mipLevels0C;
        uint32_t m_format10;
        uint32_t m_sampleCount14;
        uint32_t m_bindFlags18;
        uint32_t m_cpuAccess1C;
        uint64_t m_miscFlags20;
    } texDesc = {
        width,
        height,
        arraySizeOrDepth,
        mipLevels,
        surfaceFormat,
        0u,
        bindFlags,
        0u,
        0ULL
    };

    void* pDevice = **reinterpret_cast<void***>(GX_VMA(0x55406c8ULL));
    GXRefCountedSurface* pNewSurf = nullptr;
    int32_t hr = sub_2166300(pDevice, &pNewSurf, &texDesc, pInitData, pCustomArg1, pCustomArg2, nullptr);

    GXRefCountedSurface* pOldSurf = pThis->m_pSurface40;
    if (pOldSurf != pNewSurf) {
        if (pOldSurf) {
            if (__sync_fetch_and_add(&pOldSurf->m_refCount10, -1) == 1) {
                DLAllocator* pAlloc = pOldSurf->m_pAllocator08;
                pOldSurf->m_pVTable00[0](pOldSurf);
                pAlloc->m_pVTable->Free(pAlloc, pOldSurf);
            }
            pThis->m_pSurface40 = nullptr;
        }
        pThis->m_pSurface40 = pNewSurf;
        if (pNewSurf) {
            __sync_fetch_and_add(&pNewSurf->m_refCount10, 1);
        }
    }

    if (hr == 0) {
        if (__sync_fetch_and_add(&pNewSurf->m_refCount10, -1) == 1 && pNewSurf != nullptr) {
            DLAllocator* pAlloc = pNewSurf->m_pAllocator08;
            pNewSurf->m_pVTable00[0](pNewSurf);
            pAlloc->m_pVTable->Free(pAlloc, pNewSurf);
        }
        pThis->m_pitchOrSize58 = pNewSurf ? pNewSurf->m_pitchOrSize2C : 0;

        if (!pThis->m_pSurface40) {
            hr = 5;
        } else {
            struct SrvDesc {
                uint32_t m_format00;
                uint32_t m_viewType04;
                uint32_t m_mostDetailedMip08;
                uint32_t m_mipLevels0C;
                uint64_t m_reserved10;
            } srvDesc = {format, 8u, 0u, mipLevels, 0ULL};

            pDevice = **reinterpret_cast<void***>(GX_VMA(0x55406c8ULL));
            hr = sub_21669a0(pDevice, &pThis->m_pSrvView48, pThis->m_pSurface40, &srvDesc, pCustomArg1);
            if (hr == 0 && bCreateTargetView) {
                struct TargetViewDesc {
                    uint32_t m_format00;
                    uint32_t m_viewType04;
                    uint32_t m_mipSlice08;
                    uint32_t m_firstArraySlice0C;
                    uint32_t m_arraySize10;
                } tvDesc = {surfaceFormat, 6u, mipLevels - 1u, 0u, arraySizeOrDepth};

                pDevice = **reinterpret_cast<void***>(GX_VMA(0x55406c8ULL));
                hr = sub_2166b60(pDevice, &pThis->m_pRtvOrDsvView50, pThis->m_pSurface40, &tvDesc, 0);
            }
        }
    }

    return (hr == 0);
}

// ============================================================================
// 66. 0x217fb90 (11 B) - GXSurfaceTexture_GetSurfacePitchOrSize2C
// Null-safe accessor returning `pSurf ? pSurf->m_pitchOrSize2C : 0`.
// ============================================================================
uint32_t GXSurfaceTexture_GetSurfacePitchOrSize2C(const GXRefCountedSurface* pSurf) {
    return pSurf ? pSurf->m_pitchOrSize2C : 0u;
}

// ============================================================================
// 67. 0x217fac0 (201 B) - GXSurfaceTexture_ReleaseViewsAndSurface
// Virtual method (`+0x90`): destroys and frees `m_pSrvView48` (`+0x48`),
// `m_pRtvOrDsvView50` (`+0x50`), releases `m_pSurface40` (`+0x40`), resets
// `m_pitchOrSize58 = 0` and `m_clearColor10[0] = 1.0f`.
// ============================================================================
void GXSurfaceTexture_ReleaseViewsAndSurface(GXSurfaceTexture* pThis) {
    if (pThis->m_pSrvView48) {
        auto* pView = static_cast<void (**)(void*)>(pThis->m_pSrvView48);
        DLAllocator* pAlloc = sub_207b720(pView);
        (*pView)(pView);
        pAlloc->m_pVTable->Free(pAlloc, pView);
        pThis->m_pSrvView48 = nullptr;
    }
    if (pThis->m_pRtvOrDsvView50) {
        auto* pView = static_cast<void (**)(void*)>(pThis->m_pRtvOrDsvView50);
        DLAllocator* pAlloc = sub_207b720(pView);
        (*pView)(pView);
        pAlloc->m_pVTable->Free(pAlloc, pView);
        pThis->m_pRtvOrDsvView50 = nullptr;
    }
    GXRefCountedSurface* pSurf = pThis->m_pSurface40;
    if (pSurf) {
        if (__sync_fetch_and_add(&pSurf->m_refCount10, -1) == 1) {
            DLAllocator* pAlloc = pSurf->m_pAllocator08;
            pSurf->m_pVTable00[0](pSurf);
            pAlloc->m_pVTable->Free(pAlloc, pSurf);
        }
        pThis->m_pSurface40 = nullptr;
        pThis->m_pitchOrSize58 = 0;
    }
    pThis->m_pitchOrSize58 = 0;
    pThis->m_clearColor10[0] = 1.0f;
}

// ============================================================================
// 68. 0x217f990 (300 B) - GXSurfaceTexture_Destructor
// Non-deleting destructor for `GXSurfaceTexture`: releases views/surface,
// sets base vtable `0x53b6b00`, and releases `m_pResource30` via `0x2192660`.
// ============================================================================
void GXSurfaceTexture_Destructor(GXSurfaceTexture* pThis) {
    pThis->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b62c0ULL));
    GXSurfaceTexture_ReleaseViewsAndSurface(pThis);

    pThis->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b6b00ULL));
    if (pThis->m_pResource30) {
        sub_2192660(pThis->m_pResource30);
        pThis->m_pResource30 = nullptr;
    }
}

// ============================================================================
// 69. 0x217f970 (28 B) - GXSurfaceTexture_DeletingDestructor
// Calls `GXSurfaceTexture_Destructor` (`0x217f990`) and frees `pThis` via
// `sub_2bbe588`.
// ============================================================================
void GXSurfaceTexture_DeletingDestructor(GXSurfaceTexture* pThis) {
    GXSurfaceTexture_Destructor(pThis);
    sub_2bbe588(pThis);
}

// ============================================================================
// 70. 0x217f920 (77 B) - GXSurfaceTexture_Construct
// Initializes `GXSurfaceTexture` fields and sets vtable `0x53b62c0`.
// ============================================================================
void GXSurfaceTexture_Construct(GXSurfaceTexture* pThis) {
    pThis->m_flags08 = 0;
    const float* pDefaultVec = reinterpret_cast<const float*>(GX_VMA(0x49205c0ULL));
    pThis->m_defaultVec20[0] = pDefaultVec[0];
    pThis->m_defaultVec20[1] = pDefaultVec[1];
    pThis->m_defaultVec20[2] = pDefaultVec[2];
    pThis->m_defaultVec20[3] = pDefaultVec[3];
    pThis->m_pResource30 = nullptr;
    pThis->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b62c0ULL));
    pThis->m_pitchOrSize58 = 0;
    pThis->m_pRtvOrDsvView50 = nullptr;
    pThis->m_pSrvView48 = nullptr;
    pThis->m_pSurface40 = nullptr;
    pThis->m_clearColor10[0] = 1.0f;
}

// ============================================================================
// 71. 0x217f840 (69 B) - GXStructuredBuffer_UploadDataToContext
// Uploads `pSrcData` into `pThis->m_pBufferResource00` using the command
// context at `*reinterpret_cast<uint8_t**>(pContextHolder + 8)` (`+0xb730`
// upload ring) via `sub_2168a20`.
// ============================================================================
void GXStructuredBuffer_UploadDataToContext(
    GXStructuredBuffer* pThis,
    void* pContextHolder,
    const void* pSrcData)
{
    if (pSrcData && pThis->m_pBufferResource00) {
        auto* pCtx = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(pContextHolder) + 0x08);
        sub_2168a20(pCtx, pCtx + 0xB730, pThis->m_pBufferResource00, 0, 0, pSrcData, 0, 0);
    }
}

// ============================================================================
// 72. 0x217f7e0 (85 B) - GXStructuredBuffer_UploadDataImmediate
// Uploads `pSrcData` into `pThis->m_pBufferResource00` using the global device
// immediate context (`0x55406c8`, `+0x130` ctx, `+0xb860` upload ring) via
// `sub_2168a20`.
// ============================================================================
void GXStructuredBuffer_UploadDataImmediate(
    GXStructuredBuffer* pThis,
    const void* pSrcData)
{
    if (pSrcData && pThis->m_pBufferResource00) {
        auto* pDev = **reinterpret_cast<uint8_t***>(GX_VMA(0x55406c8ULL));
        sub_2168a20(pDev + 0x130, pDev + 0xB860, pThis->m_pBufferResource00, 0, 0, pSrcData, 0, 0);
    }
}

// ============================================================================
// 73. 0x217f6c0 (278 B) - GXStructuredBuffer_CreateBuffer
// Computes clamped buffer byte size (`<= 0x10000` aligned to `stride`), fills
// the buffer descriptor, and creates the buffer via `sub_2165c10`.
// ============================================================================
bool GXStructuredBuffer_CreateBuffer(
    GXStructuredBuffer* pThis,
    uint32_t stride,
    uint32_t elementCount,
    const void* pInitialData,
    uint8_t usageFlag)
{
    uint32_t totalBytes = elementCount * stride;
    if (totalBytes >= 0x10001u) {
        uint32_t maxAligned = 0x10000u - (0x10000u % stride);
        if (totalBytes >= maxAligned) {
            totalBytes = maxAligned;
        }
    }
    pThis->m_totalByteSize08 = totalBytes;
    pThis->m_elementStride0C = stride;
    if (totalBytes == 0) {
        return false;
    }

    struct BufferDesc {
        uint32_t m_byteWidth00;
        uint32_t m_usage04;
        uint32_t m_bindFlags08;
        uint32_t m_cpuAccess0C;
        uint32_t m_miscFlags10;
        uint32_t m_structureByteStride14;
        uint32_t m_reserved18;
    } bufDesc = {
        totalBytes,
        0u,
        4u,
        0u,
        static_cast<uint32_t>(usageFlag) << 11,
        0u,
        0u
    };

    struct InitDataDesc {
        const void* m_pSysMem00;
        uint64_t    m_reserved08;
    } initDesc = {pInitialData, 0ULL};

    void* pDevice = **reinterpret_cast<void***>(GX_VMA(0x55406c8ULL));
    void* pAlloc = *reinterpret_cast<void**>(GX_VMA(0x55406b8ULL));
    int32_t hr = sub_2165c10(
        pDevice,
        pThis,
        &bufDesc,
        pInitialData ? &initDesc : nullptr,
        0,
        pAlloc,
        nullptr);
    return (hr == 0);
}

// ============================================================================
// 74. 0x217f690 (36 B) - GXStructuredBuffer_ReleaseBuffer
// Releases `pThis->m_pBufferResource00` via `sub_2192660` and clears pointer.
// ============================================================================
void GXStructuredBuffer_ReleaseBuffer(GXStructuredBuffer* pThis) {
    if (pThis->m_pBufferResource00) {
        sub_2192660(pThis->m_pBufferResource00);
        pThis->m_pBufferResource00 = nullptr;
    }
}

// ============================================================================
// 75. 0x217f660 (36 B) - GXStructuredBuffer_Destructor
// Destructor for `GXStructuredBuffer`: releases `m_pBufferResource00` via
// `sub_2192660` and clears pointer.
// ============================================================================
void GXStructuredBuffer_Destructor(GXStructuredBuffer* pThis) {
    if (pThis->m_pBufferResource00) {
        sub_2192660(pThis->m_pBufferResource00);
        pThis->m_pBufferResource00 = nullptr;
    }
}

// ============================================================================
// 76. 0x217f650 (16 B) - GXStructuredBuffer_Construct
// Default constructor for `GXStructuredBuffer`: zeroes resource pointer and
// size/stride fields.
// ============================================================================
void GXStructuredBuffer_Construct(GXStructuredBuffer* pThis) {
    *reinterpret_cast<uint64_t*>(&pThis->m_totalByteSize08) = 0;
    pThis->m_pBufferResource00 = nullptr;
}

// ============================================================================
// 77. 0x217f530 (282 B) - GXShaderConstantSlotTable_CopyConstructEntry
// Bounds-checks `pDstEntry` against `pTable` capacity (`+0x788` * `0x78`),
// then copy-constructs a `0x78`-byte shader constant slot entry (including its
// embedded short string at `+0x08` up to `0x1F` chars and `0x41`-byte payload
// at `+0x30`).
// ============================================================================
struct GXShaderConstantSlotEntry {
    uint64_t m_header00;          // +0x00
    void*    m_pStrAlloc08;       // +0x08
    char     m_inlineBuf10[0x10]; // +0x10
    uint64_t m_strLen20;          // +0x20
    uint64_t m_strCap28;          // +0x28
    uint8_t  m_payload30[0x41];   // +0x30
    uint8_t  m_zero71;            // +0x71
    uint8_t  m_flags72;           // +0x72
    uint8_t  m_pad73[5];          // +0x73
};

extern "C" void sub_27cbc20(void* pStr, uint64_t newCap, uint64_t oldLen);
extern "C" void sub_29b8ca0(void* pDstStr, const void* pSrcStr, uint64_t pos, uint64_t count);

void GXShaderConstantSlotTable_CopyConstructEntry(
    const void* pTableBase,
    GXShaderConstantSlotEntry* pDstEntry,
    const GXShaderConstantSlotEntry* pSrcEntry)
{
    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(pTableBase);
    uintptr_t alignedBase = baseAddr + ((-static_cast<intptr_t>(baseAddr)) & 7);
    uint64_t maxEntries = *reinterpret_cast<const uint64_t*>(static_cast<const uint8_t*>(pTableBase) + 0x788);
    uintptr_t endAddr = alignedBase + maxEntries * 0x78u;

    if (endAddr <= reinterpret_cast<uintptr_t>(pDstEntry)) {
        sub_20b55b0(
            0u,
            0x27,
            reinterpret_cast<const void*>(GX_VMA(0x4940099ULL)));
    }

    if (pDstEntry) {
        pDstEntry->m_header00 = pSrcEntry->m_header00;
        uint8_t stackScratch[0x41];
        sub_2bbe578(pDstEntry->m_payload30, stackScratch, 0x41);
        pDstEntry->m_zero71 = 0;
        pDstEntry->m_strCap28 = 7;
        pDstEntry->m_strLen20 = 0;
        *reinterpret_cast<uint16_t*>(pDstEntry->m_inlineBuf10) = 0;
        pDstEntry->m_flags72 = pSrcEntry->m_flags72;

        sub_27cbc20(&pDstEntry->m_pStrAlloc08, 0x1Fu, 0u);
        bool bHeap = (pDstEntry->m_strCap28 >= 8u);
        pDstEntry->m_strLen20 = 0;
        char* pStrBuf = bHeap ? *reinterpret_cast<char**>(pDstEntry->m_inlineBuf10) : pDstEntry->m_inlineBuf10;
        *reinterpret_cast<uint16_t*>(pStrBuf) = 0;

        uint64_t copyLen = (pSrcEntry->m_strLen20 < 0x1Fu) ? pSrcEntry->m_strLen20 : 0x1Fu;
        sub_29b8ca0(&pDstEntry->m_pStrAlloc08, &pSrcEntry->m_pStrAlloc08, 0u, copyLen);
        pDstEntry->m_header00 = pSrcEntry->m_header00;
    }
}

// ============================================================================
// 78. 0x217f500 (40 B) - GXMemberFunctionDelegate_InvokeWithStoredIntArg
// Checks bit 6 of byte `+0x0B` (`0x40000000` in `+0x08`) and invokes the
// C++ member function pointer at `+0x20`/`+0x28` on `m_pTargetObj10` with
// `m_intArg18` (`+0x18`).
// ============================================================================
struct GXMemberFunctionDelegate {
    void*    m_pVTable00;    // +0x00 (base: 0x53b93b0)
    uint32_t m_flags08;      // +0x08
    uint32_t m_pad0C;        // +0x0C
    void*    m_pTargetObj10; // +0x10
    uint32_t m_intArg18;     // +0x18
    uint32_t m_pad1C;        // +0x1C
    uint64_t m_pfnMember20;  // +0x20
    int64_t  m_thisDelta28;  // +0x28
};

void GXMemberFunctionDelegate_InvokeWithStoredIntArg(GXMemberFunctionDelegate* pThis) {
    if ((pThis->m_flags08 & 0x40000000u) != 0) {
        auto* pTarget = static_cast<uint8_t*>(pThis->m_pTargetObj10) + pThis->m_thisDelta28;
        uint64_t fnOrVtableOff = pThis->m_pfnMember20;
        if ((fnOrVtableOff & 1ULL) != 0) {
            auto* pVTable = *reinterpret_cast<uint8_t**>(pTarget);
            fnOrVtableOff = *reinterpret_cast<uint64_t*>(pVTable + fnOrVtableOff - 1);
        }
        using MemberFn = void (*)(void*, uint32_t);
        reinterpret_cast<MemberFn>(fnOrVtableOff)(pTarget, pThis->m_intArg18);
    }
}

// ============================================================================
// 79. 0x217f4f0 (5 B) - GXMemberFunctionDelegate_DeletingDestructor
// Tail-jumps to `sub_2bbe588(pThis)`.
// ============================================================================
void GXMemberFunctionDelegate_DeletingDestructor(GXMemberFunctionDelegate* pThis) {
    sub_2bbe588(pThis);
}

// ============================================================================
// 80. 0x217f4d0 (22 B) - GXMemberFunctionDelegate_Destructor
// Clears bit 30 (`0x40000000`) in `m_flags08` and sets base vtable `0x53b93b0`.
// ============================================================================
void GXMemberFunctionDelegate_Destructor(GXMemberFunctionDelegate* pThis) {
    pThis->m_flags08 &= 0xBFFFFFFFu;
    pThis->m_pVTable00 = reinterpret_cast<void*>(GX_VMA(0x53b93b0ULL));
}

} // namespace GX
