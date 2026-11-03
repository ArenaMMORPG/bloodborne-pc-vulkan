// ============================================================================
// Bloodborne Engine Reconstruction — Module 02_GX_GraphicsFramework
// File: GXPerfDebugMenuAndSurfaceViews.cpp
// Address Range: 0x2175480 .. 0x217f430 (80 functions, 37,302 bytes)
// Reconstructed strictly Right-to-Left (High VMA -> Low VMA)
// Policy: 100% Complete Instruction-Accurate Decompilation (ZERO STUBS)
//
// Subsystems reconstructed in this translation unit:
//   1. GXPerformanceProfilerAndScreenCaptureDebugMenu (0x217aa70 .. 0x217f430)
//      - 0x860-byte debug UI controller for GPU/CPU frame timing graphs,
//        memory/draw-call counters, custom timer series, and screenshot capture.
//   2. GXGnmSurfaceAndViewInitializer (0x217a210 .. 0x217a8b0)
//      - GNM Texture, RenderTarget, DepthRenderTarget, CMask, HTile, and
//        Buffer SRV/UAV hardware descriptor initialization.
//   3. GXRefCountedCommandPacket (0x2179a70 .. 0x217a160)
//      - Reference-counted 64-byte and 80-byte GPU command/descriptor packets.
//   4. GXBitmapFontAndRichTextVertexGenerator (0x2178c60 .. 0x21798a0)
//      - UTF-16 decoder with #c[RRGGBBAA] / #a[AA] rich-text color markup,
//        glyph quad generator with horizontal/vertical alignment, and binary
//        font resource loader.
//   5. GXGnmSamplerDepthStencilBlendAndShaderHardwareState (0x2177d20 .. 0x2178ab0)
//      - Hardware state builders and extractors for Sampler, Rasterizer,
//        DepthStencil, and Blend states, plus descriptor copy helpers.
//   6. GXGnmShaderStageAndInputLayoutCompiler (0x2175480 .. 0x2177c50)
//      - 'Shdr' container loader (Pixel/Vertex/Compute), stable merge-sort for
//        vertex attributes, and input layout semantic bitmask compiler.
// ============================================================================

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <immintrin.h>
#include "gx_direct_vulkan.h"

namespace Bloodborne {
namespace GX {

// ----------------------------------------------------------------------------
// Forward Declarations, Structures & External Engine Symbols
// ----------------------------------------------------------------------------
struct IGXAllocator {
    struct VTable {
        void* m_reserved0[10];
        void* (*Allocate)(IGXAllocator* pThis, size_t size);              // +0x50
        void* (*AllocateAligned)(IGXAllocator* pThis, size_t size, size_t align); // +0x58
        void* m_reserved1[2];
        void  (*Free)(IGXAllocator* pThis, void* pMemory);                // +0x70
    };
    VTable* vptr;
};

struct GXRefCountedOwner {
    struct VTable {
        void (*OnZeroRef)(GXRefCountedOwner* pThis);                      // +0x00
    };
    VTable*          vptr;          // +0x00
    IGXAllocator*    m_pAllocator08;// +0x08
    volatile int32_t m_refCount10;  // +0x10
};

// Reference-counted UI widget node (dual 15-bit strong/weak refcounts at +0x08)
struct GXUIWidgetNode {
    struct VTable {
        void* m_reserved0[3];
        void  (*OnStrongRefZero)(GXUIWidgetNode* pThis);                  // +0x18
        void  (*OnWeakRefZero)(GXUIWidgetNode* pThis);                    // +0x20
        int   (*CanAttachChild)(GXUIWidgetNode* pThis, GXUIWidgetNode* pChild); // +0x28
    };
    VTable*         vptr;           // +0x00
    uint32_t        m_refFlags08;   // +0x08 (bits 0..14: strongRef, bits 15..29: weakRef, bit 30: aliveFlag)
    uint32_t        m_pad0C;        // +0x0C
    GXUIWidgetNode* m_pParent10;    // +0x10
    uint64_t        m_reserved18;   // +0x18
    void*           m_pLayoutOwner20;// +0x20
};

// Reference-counted UI string object
struct GXUIRefStringNode {
    uint64_t      m_reserved00;     // +0x00
    wchar_t*      m_pBuffer08;      // +0x08
    uint64_t      m_reserved10;     // +0x10
    uint64_t      m_length18;       // +0x18
    uint64_t      m_capacity20;     // +0x20
    IGXAllocator* m_pAllocator28;   // +0x28
    uint64_t      m_reserved30;     // +0x30
    int32_t       m_refCount38;     // +0x38
};

struct GXUIRefStringHandle {
    GXUIRefStringNode* m_pNode00;   // +0x00
    const wchar_t*     m_pLiteral08;// +0x08
};

struct GXCustomTimerSlotEntry {
    void*          m_pTimerHandle00; // +0x00
    uint64_t       m_reserved08;     // +0x08
    union {
        wchar_t    m_inlineBuf10[8]; // +0x10
        wchar_t*   m_pHeapBuf10;     // +0x10
    };
    uint64_t       m_strLen20;       // +0x20
    uint64_t       m_strCap28;       // +0x28
    IGXAllocator*  m_pStrAlloc30;    // +0x30
    uint8_t        m_extra38[0x39];  // +0x38 .. +0x70
    uint8_t        m_allocFlags71;   // +0x71
    uint8_t        m_pad72[6];       // +0x72 .. +0x78
};

// External globals
#undef g_pGXGlobalDeviceSingleton
#define g_pGXGlobalDeviceSingleton (*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))
#undef g_pGXDefaultHeapAllocator
#define g_pGXDefaultHeapAllocator (*reinterpret_cast<IGXAllocator**>(GX_VMA(0x5540680ULL)))
#undef g_pUIWidgetAllocator
#define g_pUIWidgetAllocator (*reinterpret_cast<IGXAllocator**>(GX_VMA(0x5540e08ULL)))
#undef g_pUIStringNodeAllocator
#define g_pUIStringNodeAllocator (*reinterpret_cast<IGXAllocator**>(GX_VMA(0x5540e10ULL)))
#undef g_pUIDebugRootManager
#define g_pUIDebugRootManager (*reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL)))
#undef g_pSystemTimeService
#define g_pSystemTimeService (*reinterpret_cast<void**>(GX_VMA(0x5540670ULL)))
#define g_TlsScratchArenaOffset (static_cast<intptr_t>(gx_guest_tls_offset(0x53e4568ULL)))         // 0x53e4568
#undef g_vtable_GXRefCountedBase
#define g_vtable_GXRefCountedBase (*reinterpret_cast<void**>(GX_VMA(0x53b6260ULL)))
#undef g_vtable_GXUICallbackFunctor
#define g_vtable_GXUICallbackFunctor (*reinterpret_cast<void**>(GX_VMA(0x53b6280ULL)))
#undef g_vtable_GXCommandPacket80
#define g_vtable_GXCommandPacket80 (*reinterpret_cast<void**>(GX_VMA(0x535f440ULL)))
#undef g_vtable_GXCommandPacket64_TypeA
#define g_vtable_GXCommandPacket64_TypeA (*reinterpret_cast<void**>(GX_VMA(0x535f460ULL)))
#undef g_vtable_GXCommandPacket64_TypeB
#define g_vtable_GXCommandPacket64_TypeB (*reinterpret_cast<void**>(GX_VMA(0x535f480ULL)))

// External functions
extern "C" void   GXShaderConstantSlotTable_CopyConstructEntry(void* pPoolBase, void* pDst, const void* pSrc); // 0x217f530
extern "C" void   operator_delete(void* pPtr); // 0x2bbe588
extern "C" void   TlsScratchArena_InitThreadLocal(); // 0x207d250
extern "C" void   DebugLog_Warning(uint32_t level, uint32_t code, const void* pMsg, ...); // 0x20b55b0
extern "C" void*  GetSystemTimeServiceSingleton(); // 0x2ba2b10
extern "C" int    QuerySystemClockTick(void* pClockCtx); // 0x2bc0908
extern "C" void   ConvertClockTickToUtc(void* pClockCtx, uint64_t* pOutTick); // 0x2bbf7e8
extern "C" bool   UnpackRtcTimestamp(void* pOutRtc, uint64_t tick); // 0x20ee200
extern "C" void   FormatPerformanceCsvExport(void* pWideStr, const wchar_t* pFmt, const wchar_t* pPrefix, uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t min, uint32_t sec); // 0x26295d0
extern "C" void   FormatScreenshotPath(void* pWideStr, const wchar_t* pFmt, const wchar_t* pBasePath, uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t min, uint32_t sec); // 0x2762d00
extern "C" void   UIString_Format(GXUIRefStringHandle* pOut, const wchar_t* pFmt, ...); // 0x2ba7d40
extern "C" void   UIString_Assign(void* pDstField, const GXUIRefStringHandle* pSrc); // 0x2ba7bc0
extern "C" void   UIString_MakeFromWideLiteral(GXUIRefStringHandle* pOut, const wchar_t* pLiteral); // 0x231d3a0
extern "C" void   UITableRow_SetColumnFormatString(void* pRowWidget, uint32_t colIndex, const GXUIRefStringHandle* pStr); // 0x2341990
extern "C" void   UIWidget_BringToFront(void* pLayoutMgr, GXUIWidgetNode* pWidget); // 0x2330a80
extern "C" void   UICustomGraphWin_Construct(GXUIWidgetNode* pThis, GXUIWidgetNode* pParent, const GXUIRefStringHandle* pTitle, const void* pRectDesc); // 0x2342700
extern "C" void   UICustomGraphWin_AddSeries(GXUIWidgetNode* pThis, const GXUIRefStringHandle* pLabel, void* pTimerHandle, uint32_t colorMask); // 0x2342b00
extern "C" void   UITimingTableWin_Construct(GXUIWidgetNode* pThis, GXUIWidgetNode* pParent, const GXUIRefStringHandle* pTitle, const void* pRectDesc); // 0x2362e90
extern "C" void   UIGpuGraphWin_Construct(GXUIWidgetNode* pThis, GXUIWidgetNode* pParent, const GXUIRefStringHandle* pTitle, uint32_t flags); // 0x2363080
extern "C" void   UIMainDebugWin_Construct(GXUIWidgetNode* pThis, const GXUIRefStringHandle* pTitle); // 0x2362cf0
extern "C" void   UIWidget_AttachToParentWithRect(GXUIWidgetNode* pChild, GXUIWidgetNode* pParent, void* pLayoutCtx, const void* pRectDesc); // 0x2328e40
extern "C" void   UITableWidget_Construct(GXUIWidgetNode* pThis, GXUIWidgetNode* pParent, const void* pTitleStr, const void* pRectDesc); // 0x2339cb0
extern "C" void   UITableWidget_ConfigureColumn(GXUIWidgetNode* pThis, const GXUIRefStringHandle* pHeader, uint32_t colIdx, uint32_t width, uint32_t flags); // 0x233b370
extern "C" void*  UITableWidget_AddRow(GXUIWidgetNode* pThis, const GXUIRefStringHandle* pRowTitle, uint32_t flags); // 0x233a810
extern "C" void   UIBuilder_Init(void* pBuilder, GXUIWidgetNode* pRootWidget); // 0x236c4d0
extern "C" void   UIBuilder_Destroy(void* pBuilder); // 0x236c720
extern "C" void*  UIBuilder_BeginGroup(void* pBuilder, const GXUIRefStringHandle* pTitle, uint32_t bExpanded); // 0x236c900
extern "C" void*  UIBuilder_AddButtonItem(void* pBuilder, const GXUIRefStringHandle* pTemp); // 0x236d620
extern "C" void   UIButtonItem_BindCallback(void* pButton, const GXUIRefStringHandle* pLabel, void* pCallbackFunctor); // 0x235e4c0
extern "C" void   UIBuilder_AddGpuProfilerControls(void* pBuilder); // 0x21b7490
extern "C" void   UIBuilder_AddDisplayConfigControls(void* pDisplayCfg, void* pBuilder); // 0x0e31990
extern "C" void   UIBuilder_BindGpuTimingGraph(void* pGpuProfiler, void* pBuilder, uint32_t flags); // 0x21ad390
extern "C" void   UIBuilder_AddMemoryStatRow(void* pBuilder, const GXUIRefStringHandle* pLabel, void* pCounterAddr); // 0x292f830
extern "C" void   UIBuilder_AddTimingStatRow(void* pBuilder, const GXUIRefStringHandle* pLabel, float* pTimingVal); // 0x2931c70
extern "C" void   UICapturePanel_Construct(GXUIWidgetNode* pThis, GXUIWidgetNode* pParent, const void* pTitle, uint32_t a4, uint32_t a5); // 0x23362f0
extern "C" void   UITextBox_Clear(void* pTextBox); // 0x23743a0
extern "C" void   UITextBox_SetDefaultText(void* pTextBox, const wchar_t* pText); // 0x2374120
extern "C" void   UITextBox_GetText(void* pTextBox, void* pOutWideStr, uint32_t flags); // 0x2374ee0
extern "C" void   UIGraphSeries_Remove(GXUIWidgetNode* pGraphWin, void* pSeriesNode); // 0x233bfc0
extern "C" const wchar_t* WidePath_HasExtension(const wchar_t* pPath); // 0x26343c0
extern "C" void   WideString_CopyCount(wchar_t* pDst, const wchar_t* pSrc, size_t count); // 0x2bbf7a8
extern "C" wchar_t* WideString_FindLastChar(wchar_t* pStr, wchar_t ch); // 0x2bbf258
extern "C" void   ExecuteScreenCaptureToDisk(void* pCaptureMgr, void* pSurface, const wchar_t* pPath, uint32_t fmt, uint32_t width, uint32_t height, uint64_t flags); // 0x21bcf40
extern "C" void   InitCharConversionTable(void* pTable, void* pScratch, size_t size); // 0x2bbe578
extern "C" size_t AnsiString_Length(const char* pStr); // 0x2bbf228
extern "C" void   sub_27cbc20(void* pStr, uint64_t newCap, uint64_t oldLen); // 0x27cbc20
extern "C" void   ConvertAnsiToWideString(void* pConvCtx, const char* pSrc, size_t len); // 0x27cba30
extern "C" IGXAllocator* SubAllocator_GetParentAllocator(void* pSubAlloc); // 0x207b720
extern "C" void   SubAllocator_Shutdown(void* pSubAlloc); // 0x26d7fa0
extern "C" int    SubAllocator_Initialize(void* pSubAlloc, void* pDev, uint32_t size, uint32_t align, IGXAllocator* pParentAlloc, uint32_t flags, uint32_t extra0, uint32_t extra1); // 0x26d7e60
extern "C" void   FlushCommandContext(void* pCmdCtx); // 0x26aab20

// GNM Surface & View external helpers
extern "C" int    GXGnmTexture_InitializeFromSurface(void* pOutTex, const void* pSurface, uint32_t fmt, uint32_t firstMip, uint32_t firstSlice, uint32_t numMips, uint32_t numSlices, uint32_t isCubemap); // 0x216d900
extern "C" int    GXGnmDepthTarget_InitializeFromSurface(void* pOutDepthTarget, const void* pSurface, uint32_t fmt, uint32_t mip, uint32_t firstSlice, uint32_t numSlices); // 0x216fa60
extern "C" int    GXGnmRenderTarget_InitializeFromSurface(void* pOutRenderTarget, const void* pSurface, uint32_t fmt, uint32_t mip, uint32_t firstSlice, uint32_t numSlices); // 0x216e1d0
extern "C" uint8_t GXGnmFormat_SupportsCMaskFastClear(uint8_t surfaceFlags, uint32_t format); // 0x26bb510
extern "C" int    GXGnmFormat_GetDataFormatDescriptor(uint32_t* pOutDataFmt, uint32_t engineFmt); // 0x26b7310
extern "C" void   sceGnmTextureSetResourceMemoryType(void* pTex, uint32_t memType, uint32_t isL1Enabled); // 0x1089760
extern "C" void   sceGnmBufferInitializeAsData(void* pBuf, uint64_t baseAddr, uint32_t dataFmt, uint32_t numElements); // 0x10707c0
extern "C" void   sceGnmBufferInitializeAsRegular(void* pBuf, uint64_t baseAddr, uint32_t stride, uint32_t numElements); // 0x10708a0
extern "C" void   sceGnmBufferInitializeAsByteBuffer(void* pBuf, uint64_t baseAddr, uint32_t byteSize); // 0x1070990
extern "C" void   sceGnmBufferSetResourceMemoryType(void* pBuf, uint32_t memType, uint32_t isL1Enabled, uint32_t isCoherent); // 0x10710c0
extern "C" void   sceGnmRenderTargetSetCmaskClearColor(void* pRt, uint32_t isLinear); // 0x1072200
extern "C" bool   sceGnmRenderTargetGetCmaskFastClearEnable(const void* pRt); // 0x1072730
extern "C" void   sceGnmRenderTargetSetCmaskFastClearEnable(void* pRt, uint32_t enable); // 0x1072840
extern "C" uint32_t sceGnmRenderTargetGetCmaskAddress256ByteBlocks(const void* pRt); // 0x1072830
extern "C" uint32_t sceGnmRenderTargetGetCmaskSliceSizeInBytes(const void* pRt); // 0x1072630
extern "C" uint32_t sceGnmRenderTargetGetBaseArraySliceIndex(const void* pRt); // 0x1072800
extern "C" uint32_t sceGnmRenderTargetGetLastArraySliceIndex(const void* pRt); // 0x1072810

// ============================================================================
// 1. GXPerformanceProfilerAndScreenCaptureDebugMenu (0x217aa70 .. 0x217f430)
// ============================================================================

struct GXPerfDebugMenu {
    void*                  m_pSubAllocator00;       // +0x000
    float                  m_gpuTimingsMs08[22];    // +0x008 .. +0x060
    uint32_t               m_reserved60[2];         // +0x060
    GXCustomTimerSlotEntry m_customTimerSlots68[16];// +0x068 .. +0x7E8 (16 * 0x78 = 0x780)
    uint64_t               m_reserved7E8;           // +0x7E8
    uint64_t               m_customTimerCount7F0;   // +0x7F0
    GXUIWidgetNode*        m_pWindow7F8;            // +0x7F8
    uint32_t               m_captureFormat800;      // +0x800
    uint32_t               m_captureWidth804;       // +0x804
    uint32_t               m_captureHeight808;      // +0x808
    uint8_t                m_bCaptureRequested80C;  // +0x80C
    uint8_t                m_bCaptureFlag80D;       // +0x80D
    uint8_t                m_bCaptureFlag80E;       // +0x80E
    uint8_t                m_bPadding80F;           // +0x80F
    GXUIWidgetNode*        m_pCapturePanel810;      // +0x810
    GXUIWidgetNode*        m_pMainWindow818;        // +0x818
    GXUIWidgetNode*        m_pSubPanel820;          // +0x820
    GXUIWidgetNode*        m_pStatsWindow828;       // +0x828
    GXUIWidgetNode*        m_pStatsSubPanel830;     // +0x830
    GXUIWidgetNode*        m_pGpuGraphWindow838;    // +0x838
    GXUIWidgetNode*        m_pTimingTableWin840;    // +0x840
    GXUIWidgetNode*        m_pTimingTableWidget848; // +0x848
    GXUIWidgetNode*        m_pCustomGraphWindow850; // +0x850
    uint32_t               m_reserved858;           // +0x858
    uint32_t               m_reserved85C;           // +0x85C
};

// Exact instruction-level helper for releasing a GXUIRefStringHandle
static inline void GXUIRefString_Release(GXUIRefStringHandle* pHandle)
{
    pHandle->m_pLiteral08 = nullptr;
    GXUIRefStringNode* pNode = pHandle->m_pNode00;
    if (pNode) {
        if (--pNode->m_refCount38 == 0) {
            IGXAllocator* pStrNodeAlloc = g_pUIStringNodeAllocator;
            if (pNode->m_capacity20 >= 8) {
                pNode->m_pAllocator28->vptr->Free(pNode->m_pAllocator28, pNode->m_pBuffer08);
            }
            pNode->m_capacity20 = 7;
            pNode->m_length18 = 0;
            pNode->m_pBuffer08 = nullptr;
            pStrNodeAlloc->vptr->Free(pStrNodeAlloc, pNode);
        }
        pHandle->m_pNode00 = nullptr;
    }
}

// Exact instruction-level helper for copying a GXUIRefStringHandle into a temporary handle
static inline void GXUIRefString_CopyInit(GXUIRefStringHandle* pDst, const GXUIRefStringHandle* pSrc)
{
    pDst->m_pNode00 = nullptr;
    pDst->m_pLiteral08 = nullptr;
    if (pSrc->m_pLiteral08) {
        pDst->m_pLiteral08 = pSrc->m_pLiteral08;
    } else if (pSrc->m_pNode00) {
        pDst->m_pNode00 = pSrc->m_pNode00;
        pSrc->m_pNode00->m_refCount38++;
    }
}

// Exact instruction-level helper for decrementing strong+weak refcounts on a GXUIWidgetNode
static inline void GXUIWidget_ReleaseRef(GXUIWidgetNode** ppSlot)
{
    GXUIWidgetNode* pNode = *ppSlot;
    if (!pNode) {
        return;
    }
    uint32_t flags = pNode->m_refFlags08;
    uint32_t decStrong = (flags + 0x7FFF) & 0x7FFF;
    uint32_t newFlags = (flags & 0xFFFF8000u) | decStrong;
    pNode->m_refFlags08 = newFlags;
    if (decStrong == 0 && (flags & 0x40000000u) != 0) {
        pNode->vptr->OnStrongRefZero(pNode);
        newFlags = pNode->m_refFlags08 & 0xBFFFFFFFu;
        pNode->m_refFlags08 = newFlags;
    }
    uint32_t decWeak = ((newFlags >> 15) + 0x7FFF) & 0x7FFF;
    pNode->m_refFlags08 = (newFlags & 0xC0007FFFu) | (decWeak << 15);
    if (decWeak == 0) {
        pNode->m_refFlags08 = newFlags | 0x3FFF8000u;
        pNode->vptr->OnWeakRefZero(pNode);
    }
    *ppSlot = nullptr;
}

// Exact instruction-level helper for assigning and retaining a GXUIWidgetNode smart pointer
static inline void GXUIWidget_AssignRef(GXUIWidgetNode** ppSlot, GXUIWidgetNode* pNewNode)
{
    if (*ppSlot == pNewNode) {
        return;
    }
    GXUIWidget_ReleaseRef(ppSlot);
    if (pNewNode) {
        *ppSlot = pNewNode;
        uint32_t flags = pNewNode->m_refFlags08;
        uint32_t incStrong = (flags + 1) & 0x7FFF;
        flags = (flags & 0xFFFF8000u) | incStrong;
        uint32_t incWeak = (flags + 0x8000u) & 0x3FFF8000u;
        pNewNode->m_refFlags08 = (flags & 0xC0007FFFu) | incWeak;
    }
}

// Helper to allocate a 0x30-byte UI button callback functor bound to pThis and pfnCallback
static inline void* GXUICallbackFunctor_Create(GXPerfDebugMenu* pMenu, void (*pfnCallback)(GXPerfDebugMenu*))
{
    uint8_t* pMem = static_cast<uint8_t*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x30, 8));
    if (!pMem) {
        return nullptr;
    }
    uint32_t* pRefFlags = reinterpret_cast<uint32_t*>(pMem + 0x08);
    *pRefFlags = (*pRefFlags & 0x80000000u) | 0x40000000u;
    *reinterpret_cast<void**>(pMem + 0x00) = &g_vtable_GXUICallbackFunctor;
    *reinterpret_cast<GXPerfDebugMenu**>(pMem + 0x10) = pMenu;
    *reinterpret_cast<uint32_t*>(pMem + 0x18) = 0;
    *reinterpret_cast<void (**)(GXPerfDebugMenu*)>(pMem + 0x20) = pfnCallback;
    *reinterpret_cast<uint64_t*>(pMem + 0x28) = 0;
    return pMem;
}

// [VMA: 0x217f430 - 0x217f431] (1 B)
extern "C" void GXPerfDebugMenu_NoOpCallback1() {}

// [VMA: 0x217f420 - 0x217f421] (1 B)
extern "C" void GXPerfDebugMenu_NoOpCallback2() {}

// [VMA: 0x217f410 - 0x217f411] (1 B)
extern "C" void GXPerfDebugMenu_NoOpCallback3() {}

// [VMA: 0x217eee0 - 0x217f40a] (1,322 B)
// Configures the empty format string (0x49d85f0) on the row widget (+0x18) and columns 1..6.
extern "C" void GXPerfDebugMenu_ConfigureGraphSeriesColumns(void* pTableRowWidget)
{
    GXUIRefStringHandle fmtStr = { nullptr, nullptr };
    UIString_Format(&fmtStr, reinterpret_cast<const wchar_t*>(GX_VMA(0x49d85f0ULL)));

    GXUIRefStringHandle tmpStr;
    GXUIRefString_CopyInit(&tmpStr, &fmtStr);
    UIString_Assign(static_cast<uint8_t*>(pTableRowWidget) + 0x18, &tmpStr);
    GXUIRefString_Release(&tmpStr);

    for (uint32_t colIdx = 1; colIdx <= 6; ++colIdx) {
        GXUIRefString_CopyInit(&tmpStr, &fmtStr);
        UITableRow_SetColumnFormatString(pTableRowWidget, colIdx, &tmpStr);
        GXUIRefString_Release(&tmpStr);
    }

    GXUIRefString_Release(&fmtStr);
}

// [VMA: 0x217ec60 - 0x217eed5] (629 B)
// Formats a timestamped performance log file path (YYYYMMDD_HHMMSS) and triggers CSV export.
extern "C" void GXPerfDebugMenu_OnSavePerformanceLogCallback(GXPerfDebugMenu* pThis)
{
    struct LocalWideString {
        uint8_t       m_header[0x18]; // -0x90 .. -0x78
        wchar_t*      m_pBuf;         // -0x50 (or -0x88)
        uint64_t      m_pad;          // -0x48
        uint64_t      m_length;       // -0x40
        uint64_t      m_capacity;     // -0x38
        IGXAllocator* m_pAllocator;   // -0x30
        uint8_t       m_bInitialized; // -0x28
    };

    uint8_t* pTlsBase = nullptr;
    pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
    uint8_t* pTlsSlot = *reinterpret_cast<uint8_t**>(pTlsBase + g_TlsScratchArenaOffset);
    if (!pTlsSlot) {
        TlsScratchArena_InitThreadLocal();
        pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
        pTlsSlot = *reinterpret_cast<uint8_t**>(pTlsBase + g_TlsScratchArenaOffset);
    }

    IGXAllocator* pScratchAlloc = reinterpret_cast<IGXAllocator*>(pTlsSlot + 0x28);
    LocalWideString strBase = {};
    reinterpret_cast<void (*)(void*, IGXAllocator*, uint32_t)>(
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pScratchAlloc->vptr) + 0x20))(
        &strBase, pScratchAlloc, 0);
    if ((strBase.m_header[0] & 0x20) == 0) {
        DebugLog_Warning(0, 0x3E, "Allocator flag check failed");
    }
    strBase.m_pAllocator = pScratchAlloc;
    strBase.m_capacity = 7;
    strBase.m_length = 0;
    strBase.m_pBuf = nullptr;
    strBase.m_bInitialized = 1;

    void* pTextBox = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pThis->m_pCapturePanel810) + 0x110);
    UITextBox_GetText(pTextBox, &strBase.m_pBuf, 1);

    if (!g_pSystemTimeService) {
        g_pSystemTimeService = GetSystemTimeServiceSingleton();
    }
    IGXAllocator* pTimeAlloc = static_cast<IGXAllocator*>(g_pSystemTimeService);
    LocalWideString clockCtx = {};
    reinterpret_cast<void (*)(void*, IGXAllocator*, uint32_t)>(
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pTimeAlloc->vptr) + 0x20))(
        &clockCtx, pTimeAlloc, 0);
    if ((clockCtx.m_header[0] & 0x20) == 0) {
        DebugLog_Warning(0, 0x3E, "Allocator flag check failed");
    }
    clockCtx.m_pAllocator = pTimeAlloc;
    clockCtx.m_capacity = 7;
    clockCtx.m_length = 0;
    clockCtx.m_pBuf = nullptr;
    clockCtx.m_bInitialized = 1;

    uint64_t unpackedRtc[2] = { 0, 0 };
    if (QuerySystemClockTick(&clockCtx) == 0) {
        uint64_t utcTick = 0;
        ConvertClockTickToUtc(&clockCtx, &utcTick);
        if (utcTick != 0 && UnpackRtcTimestamp(unpackedRtc, utcTick) && unpackedRtc[0] != 0) {
            const wchar_t* pPrefix = (strBase.m_capacity >= 8)
                ? strBase.m_pBuf
                : reinterpret_cast<const wchar_t*>(&strBase.m_pBuf);
            uint64_t packed = unpackedRtc[1];
            uint32_t year  = static_cast<uint32_t>(packed & 0xFFFu);
            uint32_t month = static_cast<uint32_t>((packed >> 22) & 0xFu);
            uint32_t day   = static_cast<uint32_t>((packed >> 29) & 0x1Fu);
            uint32_t hour  = static_cast<uint32_t>((packed >> 34) & 0x1Fu);
            uint32_t min   = static_cast<uint32_t>((packed >> 39) & 0x3Fu);
            uint32_t sec   = static_cast<uint32_t>((packed >> 45) & 0x3Fu);
            FormatPerformanceCsvExport(&strBase, L"%s_%04d%02d%02d_%02d%02d%02d.csv",
                                       pPrefix, year, month, day, hour, min, sec);
        }
    }

    if (clockCtx.m_capacity >= 8) {
        clockCtx.m_pAllocator->vptr->Free(clockCtx.m_pAllocator, clockCtx.m_pBuf);
    }
    if (strBase.m_capacity >= 8) {
        strBase.m_pAllocator->vptr->Free(strBase.m_pAllocator, strBase.m_pBuf);
    }
}

// [VMA: 0x217e810 - 0x217ec5a] (1,098 B)
// Creates or brings to front the Custom Timer Graph window (+0x850) and populates registered series.
extern "C" void GXPerfDebugMenu_OpenCustomTimerGraphWindow(GXPerfDebugMenu* pThis)
{
    if (!g_pUIDebugRootManager) {
        return;
    }

    GXUIWidgetNode* pExisting = pThis->m_pCustomGraphWindow850;
    if (pExisting) {
        uint8_t* pRaw = reinterpret_cast<uint8_t*>(pExisting);
        if ((pRaw[0x0B] & 0x40) != 0 && pRaw[0x6F] >= 2) {
            for (GXUIWidgetNode* pCur = pExisting; pCur != nullptr; pCur = pCur->m_pParent10) {
                if ((reinterpret_cast<uint8_t*>(pCur)[0x50] & 0x02) != 0) {
                    void* pLayoutOwner = pCur->m_pLayoutOwner20;
                    UIWidget_BringToFront(*reinterpret_cast<void**>(static_cast<uint8_t*>(pLayoutOwner) + 0x10), pCur);
                    return;
                }
            }
            return;
        }
        pExisting->vptr->OnStrongRefZero(pExisting);
        pRaw[0x0B] &= 0xBFu;
        GXUIWidget_ReleaseRef(&pThis->m_pCustomGraphWindow850);
    }

    struct UIWindowRectDesc {
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
        uint64_t flags;
    } rectDesc = { 0, 0, 0x168, 0xC8, 0x53502ull };

    GXUIWidgetNode* pNewWin = static_cast<GXUIWidgetNode*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x188, 8));
    if (pNewWin) {
        void* pRootMgrSub = *reinterpret_cast<void**>(static_cast<uint8_t*>(g_pUIDebugRootManager) + 0x10);
        GXUIWidgetNode* pParent = *reinterpret_cast<GXUIWidgetNode**>(static_cast<uint8_t*>(pRootMgrSub) + 0x18);
        GXUIRefStringHandle titleStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&titleStr, L"Custom Timer Graph");
        UICustomGraphWin_Construct(pNewWin, pParent, &titleStr, &rectDesc);
        GXUIWidget_AssignRef(&pThis->m_pCustomGraphWindow850, pNewWin);
        GXUIRefString_Release(&titleStr);
    } else {
        GXUIWidget_ReleaseRef(&pThis->m_pCustomGraphWindow850);
    }

    GXUIWidgetNode* pGraphWin = pThis->m_pCustomGraphWindow850;
    uint64_t totalBytes = pThis->m_customTimerCount7F0 * sizeof(GXCustomTimerSlotEntry);
    if (totalBytes != 0) {
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(&pThis->m_customTimerSlots68[0]);
        uintptr_t alignPad = (0 - baseAddr) & 7u;
        uint8_t* pSlotBase = reinterpret_cast<uint8_t*>(baseAddr + alignPad);
        uint8_t* pCurSlot = pSlotBase;
        while (true) {
            GXCustomTimerSlotEntry* pEntry = reinterpret_cast<GXCustomTimerSlotEntry*>(pCurSlot);
            const wchar_t* pLabel = (pEntry->m_strCap28 >= 8) ? pEntry->m_pHeapBuf10 : pEntry->m_inlineBuf10;
            GXUIRefStringHandle seriesStr = { nullptr, pLabel };
            UICustomGraphWin_AddSeries(pGraphWin, &seriesStr, pEntry->m_pTimerHandle00, 0xFFFFFFFFu);
            seriesStr.m_pLiteral08 = nullptr;

            pCurSlot += sizeof(GXCustomTimerSlotEntry);
            uint8_t* pEndSlot = pSlotBase + (pThis->m_customTimerCount7F0 * sizeof(GXCustomTimerSlotEntry));
            pGraphWin = pThis->m_pCustomGraphWindow850;
            if (pCurSlot == pEndSlot) {
                break;
            }
        }
    }
}

// [VMA: 0x217e800 - 0x217e801] (1 B)
extern "C" void GXPerfDebugMenu_NoOpActionCallback1() {}

// [VMA: 0x217dca0 - 0x217e7f7] (2,903 B)
// Creates or brings to front the 6-column Performance Timing Table window (+0x840) and widget (+0x848).
extern "C" void GXPerfDebugMenu_OpenTimingTableWindow(GXPerfDebugMenu* pThis)
{
    if (!g_pUIDebugRootManager) {
        return;
    }

    GXUIWidgetNode* pExisting = pThis->m_pTimingTableWin840;
    if (pExisting) {
        uint8_t* pRaw = reinterpret_cast<uint8_t*>(pExisting);
        if ((pRaw[0x0B] & 0x40) != 0 && pRaw[0x6F] >= 2) {
            for (GXUIWidgetNode* pCur = pExisting; pCur != nullptr; pCur = pCur->m_pParent10) {
                if ((reinterpret_cast<uint8_t*>(pCur)[0x50] & 0x02) != 0) {
                    void* pLayoutOwner = pCur->m_pLayoutOwner20;
                    UIWidget_BringToFront(*reinterpret_cast<void**>(static_cast<uint8_t*>(pLayoutOwner) + 0x10), pCur);
                    return;
                }
            }
            return;
        }
        GXUIWidget_ReleaseRef(&pThis->m_pTimingTableWin840);
    }

    struct UIWindowRectDesc {
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
        uint64_t flags;
    } winRect = { 0, 0, 0x17C, 0x17C, 0x51502ull };

    GXUIWidgetNode* pNewWin = static_cast<GXUIWidgetNode*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x178, 8));
    if (pNewWin) {
        void* pRootMgrSub = *reinterpret_cast<void**>(static_cast<uint8_t*>(g_pUIDebugRootManager) + 0x10);
        GXUIWidgetNode* pParent = *reinterpret_cast<GXUIWidgetNode**>(static_cast<uint8_t*>(pRootMgrSub) + 0x18);
        GXUIRefStringHandle titleStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&titleStr, reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2824ULL)));
        UITimingTableWin_Construct(pNewWin, pParent, &titleStr, &winRect);
        GXUIWidget_AssignRef(&pThis->m_pTimingTableWin840, pNewWin);
        GXUIRefString_Release(&titleStr);
    } else {
        GXUIWidget_ReleaseRef(&pThis->m_pTimingTableWin840);
    }

    uint64_t builderCtx[6] = {};
    UIBuilder_Init(builderCtx, pThis->m_pTimingTableWin840);

    UIWindowRectDesc tableRect = { 0, 0, 0x14A, 0xC8, 0x805001ull };
    GXUIWidgetNode* pNewTable = static_cast<GXUIWidgetNode*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x180, 8));
    if (pNewTable) {
        GXUIWidgetNode* pStackTopParent = *reinterpret_cast<GXUIWidgetNode**>(builderCtx[2] - 8);
        uint64_t emptyTitle[3] = { 0, 0, 0 };
        UITableWidget_Construct(pNewTable, pStackTopParent, emptyTitle, &tableRect);
        GXUIWidget_AssignRef(&pThis->m_pTimingTableWidget848, pNewTable);
    } else {
        GXUIWidget_ReleaseRef(&pThis->m_pTimingTableWidget848);
    }

    GXUIWidgetNode* pTable = pThis->m_pTimingTableWidget848;
    GXUIRefStringHandle defaultColFmt = { nullptr, reinterpret_cast<const wchar_t*>(GX_VMA(0x49d85f0ULL)) };
    uint8_t* pColBegin = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(pTable) + 0x140);
    uint8_t* pColEnd   = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(pTable) + 0x148);
    for (uint8_t* pCol = pColBegin; pCol != pColEnd; pCol += 0x18) {
        if (*reinterpret_cast<uint16_t*>(pCol + 0x12) == 0) {
            UIString_Assign(pCol, &defaultColFmt);
            break;
        }
    }
    defaultColFmt.m_pLiteral08 = nullptr;

    pColBegin = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(pTable) + 0x140);
    pColEnd   = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(pTable) + 0x148);
    for (uint8_t* pCol = pColBegin; pCol != pColEnd; pCol += 0x18) {
        if (*reinterpret_cast<uint16_t*>(pCol + 0x12) == 0) {
            *reinterpret_cast<uint16_t*>(pCol + 0x10) = 0x32;
            break;
        }
    }

    const wchar_t* kColumnHeaders[6] = {
        reinterpret_cast<const wchar_t*>(GX_VMA(0x49c284eULL)),
        reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2860ULL)),
        reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2870ULL)),
        reinterpret_cast<const wchar_t*>(GX_VMA(0x49c2882ULL)),
        reinterpret_cast<const wchar_t*>(GX_VMA(0x49c289cULL)),
        reinterpret_cast<const wchar_t*>(GX_VMA(0x49c28b8ULL)),
    };
    for (uint32_t col = 1; col <= 6; ++col) {
        GXUIRefStringHandle colHdr = { nullptr, kColumnHeaders[col - 1] };
        UITableWidget_ConfigureColumn(pTable, &colHdr, col, 0x32, 0xFFFFFFFFu);
        colHdr.m_pLiteral08 = nullptr;
    }

    for (uint32_t rowIdx = 0; rowIdx < 8; ++rowIdx) {
        GXUIRefStringHandle rowLabel = { nullptr, nullptr };
        void* pRowWidget = UITableWidget_AddRow(pTable, &rowLabel, 0xFFFFFFFFu);
        GXUIRefString_Release(&rowLabel);
        GXPerfDebugMenu_ConfigureGraphSeriesColumns(pRowWidget);
    }

    UIBuilder_Destroy(builderCtx);
}

// [VMA: 0x217d8f0 - 0x217dc98] (936 B)
// Creates or brings to front the GPU Timing Graph window (+0x838) and binds the GPU profiler.
extern "C" void GXPerfDebugMenu_OpenGpuTimingGraphWindow(GXPerfDebugMenu* pThis)
{
    if (!g_pUIDebugRootManager) {
        return;
    }
    uint8_t* pGlobalDev = static_cast<uint8_t*>(g_pGXGlobalDeviceSingleton);
    if (*reinterpret_cast<void**>(pGlobalDev + 0x650) == nullptr) {
        return;
    }

    GXUIWidgetNode* pExisting = pThis->m_pGpuGraphWindow838;
    if (pExisting) {
        uint8_t* pRaw = reinterpret_cast<uint8_t*>(pExisting);
        if ((pExisting->m_refFlags08 & 0x40000000u) != 0 && pRaw[0x6F] >= 2) {
            for (GXUIWidgetNode* pCur = pExisting; pCur != nullptr; pCur = pCur->m_pParent10) {
                if ((reinterpret_cast<uint8_t*>(pCur)[0x50] & 0x02) != 0) {
                    void* pLayoutOwner = pCur->m_pLayoutOwner20;
                    UIWidget_BringToFront(*reinterpret_cast<void**>(static_cast<uint8_t*>(pLayoutOwner) + 0x10), pCur);
                    return;
                }
            }
            return;
        }
        GXUIWidget_ReleaseRef(&pThis->m_pGpuGraphWindow838);
    }

    GXUIWidgetNode* pNewWin = static_cast<GXUIWidgetNode*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x178, 8));
    if (pNewWin) {
        void* pRootMgrSub = *reinterpret_cast<void**>(static_cast<uint8_t*>(g_pUIDebugRootManager) + 0x10);
        GXUIWidgetNode* pParent = *reinterpret_cast<GXUIWidgetNode**>(static_cast<uint8_t*>(pRootMgrSub) + 0x18);
        GXUIRefStringHandle titleStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&titleStr, L"GPU Timing Graph");
        UIGpuGraphWin_Construct(pNewWin, pParent, &titleStr, 0x51502u);
        GXUIWidget_AssignRef(&pThis->m_pGpuGraphWindow838, pNewWin);
        GXUIRefString_Release(&titleStr);
    } else {
        GXUIWidget_ReleaseRef(&pThis->m_pGpuGraphWindow838);
    }

    void* pGpuProfWrapper = *reinterpret_cast<void**>(static_cast<uint8_t*>(g_pGXGlobalDeviceSingleton) + 0x650);
    uint64_t builderCtx[6] = {};
    UIBuilder_Init(builderCtx, pThis->m_pGpuGraphWindow838);
    UIBuilder_BindGpuTimingGraph(*reinterpret_cast<void**>(pGpuProfWrapper), builderCtx, 0);
    UIBuilder_Destroy(builderCtx);
}

// [VMA: 0x217d8e0 - 0x217d8e1] (1 B)
extern "C" void GXPerfDebugMenu_NoOpActionCallback2() {}

// [VMA: 0x217d080 - 0x217d8d5] (2,133 B)
// Creates or brings to front the Memory & GPU Timing Stats window (+0x828) and binds all 19 float counters.
extern "C" void GXPerfDebugMenu_OpenMemoryAndDrawCallStatsWindow(GXPerfDebugMenu* pThis)
{
    if (!g_pUIDebugRootManager) {
        return;
    }

    GXUIWidgetNode* pExisting = pThis->m_pStatsWindow828;
    if (pExisting) {
        uint8_t* pRaw = reinterpret_cast<uint8_t*>(pExisting);
        if ((pRaw[0x0B] & 0x40) != 0 && pRaw[0x6F] >= 2) {
            for (GXUIWidgetNode* pCur = pExisting; pCur != nullptr; pCur = pCur->m_pParent10) {
                if ((reinterpret_cast<uint8_t*>(pCur)[0x50] & 0x02) != 0) {
                    void* pLayoutOwner = pCur->m_pLayoutOwner20;
                    UIWidget_BringToFront(*reinterpret_cast<void**>(static_cast<uint8_t*>(pLayoutOwner) + 0x10), pCur);
                    return;
                }
            }
            return;
        }
        GXUIWidget_ReleaseRef(&pThis->m_pStatsWindow828);
    }

    struct UIWindowRectDesc {
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
        uint64_t flags;
    } winRect = { 0, 0, 0xC8, 0x1E0, 0x51502ull };

    GXUIWidgetNode* pNewWin = static_cast<GXUIWidgetNode*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x178, 8));
    if (pNewWin) {
        void* pRootMgrSub = *reinterpret_cast<void**>(static_cast<uint8_t*>(g_pUIDebugRootManager) + 0x10);
        GXUIWidgetNode* pParent = *reinterpret_cast<GXUIWidgetNode**>(static_cast<uint8_t*>(pRootMgrSub) + 0x18);
        GXUIRefStringHandle titleStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&titleStr, L"Performance Statistics");
        UITimingTableWin_Construct(pNewWin, pParent, &titleStr, &winRect);
        GXUIWidget_AssignRef(&pThis->m_pStatsWindow828, pNewWin);
        GXUIRefString_Release(&titleStr);
    } else {
        GXUIWidget_ReleaseRef(&pThis->m_pStatsWindow828);
    }

    uint64_t builderCtx[6] = {};
    UIBuilder_Init(builderCtx, pThis->m_pStatsWindow828);

    uint8_t* pGlobalDev = static_cast<uint8_t*>(g_pGXGlobalDeviceSingleton);
    uint8_t* pPerfStatsObj = *reinterpret_cast<uint8_t**>(pGlobalDev + 0x640);
    if (pPerfStatsObj) {
        GXUIRefStringHandle memLabel = { nullptr, L"TotalAllocated" };
        UIBuilder_AddMemoryStatRow(builderCtx, &memLabel, pPerfStatsObj + 0x6A432);
        memLabel.m_pLiteral08 = nullptr;

        if (*reinterpret_cast<void**>(pGlobalDev + 0x640) != nullptr) {
            GXUIRefStringHandle groupLabel = { nullptr, L"GPU Timings (ms)" };
            void* pGroup = UIBuilder_BeginGroup(builderCtx, &groupLabel, 1);
            groupLabel.m_pLiteral08 = nullptr;

            if (pGroup) {
                static const wchar_t* const kStatLabels[19] = {
                    L"Stat00", L"Stat01", L"Stat02", L"Stat03", L"Stat04",
                    L"Stat05", L"Stat06", L"Stat07", L"Stat08", L"Stat09",
                    L"Stat10", L"Stat11", L"Stat12", L"Stat13", L"Stat14",
                    L"Stat15", L"Stat16", L"Stat17", L"Stat18"
                };
                for (uint32_t i = 0; i < 19; ++i) {
                    GXUIRefStringHandle itemLabel = { nullptr, kStatLabels[i] };
                    UIBuilder_AddTimingStatRow(builderCtx, &itemLabel, &pThis->m_gpuTimingsMs08[i]);
                    itemLabel.m_pLiteral08 = nullptr;
                }
                if (builderCtx[1] != builderCtx[2]) {
                    GXUIWidgetNode** ppStackTop = reinterpret_cast<GXUIWidgetNode**>(builderCtx[2] - 8);
                    GXUIWidget_ReleaseRef(ppStackTop);
                    builderCtx[2] -= 8;
                }
            }
        }
    }

    UIBuilder_Destroy(builderCtx);
}

// [VMA: 0x217c870 - 0x217d07b] (2,059 B)
// Populates the performance sub-menu with action buttons for GPU Graph, Timing Table, Action1, and Custom Graph.
extern "C" void GXPerfDebugMenu_PopulatePerformanceSubMenu(
    GXPerfDebugMenu* pThis,
    uint64_t* pBuilderCtx,
    bool bCreateGroup,
    bool bExpanded)
{
    bool bSkipGroupPop = true;
    if (bCreateGroup) {
        GXUIRefStringHandle groupTitle = { nullptr, L"Performance Profiler" };
        void* pGroup = UIBuilder_BeginGroup(pBuilderCtx, &groupTitle, bExpanded ? 1u : 0u);
        groupTitle.m_pLiteral08 = nullptr;
        if (!pGroup) {
            return;
        }
        bSkipGroupPop = false;
    }

    struct ButtonSpec {
        const wchar_t* pTitle;
        void (*pfnCallback)(GXPerfDebugMenu*);
    };

    // 1. GPU Timing Graph button
    {
        GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
        void* pBtn = UIBuilder_AddButtonItem(pBuilderCtx, &tmpBtn);
        tmpBtn.m_pLiteral08 = nullptr;

        GXUIRefStringHandle labelStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&labelStr, L"GPU Timing Graph");
        GXUIRefStringHandle argStr;
        GXUIRefString_CopyInit(&argStr, &labelStr);
        void* pFunctor = GXUICallbackFunctor_Create(pThis, GXPerfDebugMenu_OpenGpuTimingGraphWindow);
        UIButtonItem_BindCallback(pBtn, &argStr, pFunctor);
        GXUIRefString_Release(&argStr);
        GXUIRefString_Release(&labelStr);
    }

    // 2. Performance Timing Table button
    {
        GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
        void* pBtn = UIBuilder_AddButtonItem(pBuilderCtx, &tmpBtn);
        tmpBtn.m_pLiteral08 = nullptr;

        GXUIRefStringHandle labelStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&labelStr, L"Performance Timing Table");
        GXUIRefStringHandle argStr;
        GXUIRefString_CopyInit(&argStr, &labelStr);
        void* pFunctor = GXUICallbackFunctor_Create(pThis, GXPerfDebugMenu_OpenTimingTableWindow);
        UIButtonItem_BindCallback(pBtn, &argStr, pFunctor);
        GXUIRefString_Release(&argStr);
        GXUIRefString_Release(&labelStr);
    }

    // 3. Action 1 button + GPU Profiler & Display Config controls
    {
        GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
        void* pBtn = UIBuilder_AddButtonItem(pBuilderCtx, &tmpBtn);
        tmpBtn.m_pLiteral08 = nullptr;

        GXUIRefStringHandle labelStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&labelStr, L"Reset Counters");
        GXUIRefStringHandle argStr;
        GXUIRefString_CopyInit(&argStr, &labelStr);
        void* pFunctor = GXUICallbackFunctor_Create(
            pThis, reinterpret_cast<void (*)(GXPerfDebugMenu*)>(GXPerfDebugMenu_NoOpActionCallback1));
        UIButtonItem_BindCallback(pBtn, &argStr, pFunctor);
        GXUIRefString_Release(&argStr);
        GXUIRefString_Release(&labelStr);
    }

    UIBuilder_AddGpuProfilerControls(pBuilderCtx);
    uint8_t* pGlobalDev = static_cast<uint8_t*>(g_pGXGlobalDeviceSingleton);
    void* pDispSub = *reinterpret_cast<void**>(pGlobalDev + 0x258);
    UIBuilder_AddDisplayConfigControls(*reinterpret_cast<void**>(pDispSub), pBuilderCtx);

    // 4. Custom Timer Graph button
    {
        GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
        void* pBtn = UIBuilder_AddButtonItem(pBuilderCtx, &tmpBtn);
        tmpBtn.m_pLiteral08 = nullptr;

        GXUIRefStringHandle labelStr = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&labelStr, L"Custom Timer Graph");
        GXUIRefStringHandle argStr;
        GXUIRefString_CopyInit(&argStr, &labelStr);
        void* pFunctor = GXUICallbackFunctor_Create(pThis, GXPerfDebugMenu_OpenCustomTimerGraphWindow);
        UIButtonItem_BindCallback(pBtn, &argStr, pFunctor);
        GXUIRefString_Release(&argStr);
        GXUIRefString_Release(&labelStr);
    }

    if (!bSkipGroupPop && pBuilderCtx[1] != pBuilderCtx[2]) {
        GXUIWidgetNode** ppStackTop = reinterpret_cast<GXUIWidgetNode**>(pBuilderCtx[2] - 8);
        GXUIWidget_ReleaseRef(ppStackTop);
        pBuilderCtx[2] -= 8;
    }
}

// [VMA: 0x217bc80 - 0x217c86d] (3,053 B)
// Creates or brings to front the Main GX Debug Menu Window (+0x818) and Capture Panel (+0x810).
extern "C" GXUIWidgetNode* GXPerfDebugMenu_OpenMainDebugWindow(GXPerfDebugMenu* pThis)
{
    GXUIWidgetNode* pExisting = pThis->m_pMainWindow818;
    if (pExisting && reinterpret_cast<uint8_t*>(pExisting)[0x6F] >= 2) {
        for (GXUIWidgetNode* pCur = pExisting; pCur != nullptr; pCur = pCur->m_pParent10) {
            if ((reinterpret_cast<uint8_t*>(pCur)[0x50] & 0x02) != 0) {
                void* pLayoutOwner = pCur->m_pLayoutOwner20;
                UIWidget_BringToFront(*reinterpret_cast<void**>(static_cast<uint8_t*>(pLayoutOwner) + 0x10), pCur);
                return pThis->m_pMainWindow818;
            }
        }
        return pThis->m_pMainWindow818;
    }

    GXUIWidgetNode* pNewWin = static_cast<GXUIWidgetNode*>(
        g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x178, 8));
    if (pNewWin) {
        GXUIRefStringHandle titleStr = { nullptr, L"GX Debug Menu" };
        UIMainDebugWin_Construct(pNewWin, &titleStr);
        GXUIWidget_AssignRef(&pThis->m_pMainWindow818, pNewWin);
        titleStr.m_pLiteral08 = nullptr;
    } else {
        GXUIWidget_ReleaseRef(&pThis->m_pMainWindow818);
    }

    GXUIWidgetNode* pMainWin = pThis->m_pMainWindow818;
    void* pRootMgrSub = *reinterpret_cast<void**>(static_cast<uint8_t*>(g_pUIDebugRootManager) + 0x10);
    GXUIWidgetNode* pRootParent = *reinterpret_cast<GXUIWidgetNode**>(static_cast<uint8_t*>(pRootMgrSub) + 0x18);

    struct UIWindowRectDesc {
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
        uint64_t flags;
    } mainRect = { 0, 0, 0x12C, 0x190, 0x51502ull };

    uint8_t* pMainRaw = reinterpret_cast<uint8_t*>(pMainWin);
    uint8_t* pParentRaw = reinterpret_cast<uint8_t*>(pRootParent);
    if ((pMainRaw[0x0B] & 0x40) != 0 && pMainRaw[0x6F] == 0 && pRootParent != pMainWin &&
        pRootParent != nullptr && (pParentRaw[0x0B] & 0x40) != 0) {
        if (pRootParent->vptr->CanAttachChild(pRootParent, pMainWin) != 0) {
            UIWidget_AttachToParentWithRect(pMainWin, pRootParent, pRootParent->m_pLayoutOwner20, &mainRect);
        }
    }

    uint64_t builderCtx[6] = {};
    UIBuilder_Init(builderCtx, pThis->m_pMainWindow818);

    GXUIRefStringHandle statGroupTitle = { nullptr, L"Statistics" };
    if (UIBuilder_BeginGroup(builderCtx, &statGroupTitle, 1) != nullptr) {
        // Button 1: Performance Statistics Window
        {
            GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
            void* pBtn = UIBuilder_AddButtonItem(builderCtx, &tmpBtn);
            tmpBtn.m_pLiteral08 = nullptr;

            GXUIRefStringHandle labelStr = { nullptr, nullptr };
            UIString_MakeFromWideLiteral(&labelStr, L"Performance Statistics");
            GXUIRefStringHandle argStr;
            GXUIRefString_CopyInit(&argStr, &labelStr);
            void* pFunctor = GXUICallbackFunctor_Create(pThis, GXPerfDebugMenu_OpenMemoryAndDrawCallStatsWindow);
            UIButtonItem_BindCallback(pBtn, &argStr, pFunctor);
            GXUIRefString_Release(&argStr);
            GXUIRefString_Release(&labelStr);
        }
        // Button 2: No-op stats action
        {
            GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
            void* pBtn = UIBuilder_AddButtonItem(builderCtx, &tmpBtn);
            tmpBtn.m_pLiteral08 = nullptr;

            GXUIRefStringHandle labelStr = { nullptr, nullptr };
            UIString_MakeFromWideLiteral(&labelStr, L"Reset Statistics");
            GXUIRefStringHandle argStr;
            GXUIRefString_CopyInit(&argStr, &labelStr);
            void* pFunctor = GXUICallbackFunctor_Create(
                pThis, reinterpret_cast<void (*)(GXPerfDebugMenu*)>(GXPerfDebugMenu_NoOpActionCallback2));
            UIButtonItem_BindCallback(pBtn, &argStr, pFunctor);
            GXUIRefString_Release(&argStr);
            GXUIRefString_Release(&labelStr);
        }
        if (builderCtx[1] != builderCtx[2]) {
            GXUIWidgetNode** ppStackTop = reinterpret_cast<GXUIWidgetNode**>(builderCtx[2] - 8);
            GXUIWidget_ReleaseRef(ppStackTop);
            builderCtx[2] -= 8;
        }
    }

    GXPerfDebugMenu_PopulatePerformanceSubMenu(pThis, builderCtx, true, false);

    GXUIRefStringHandle logGroupTitle = { nullptr, L"Performance Log Export" };
    if (UIBuilder_BeginGroup(builderCtx, &logGroupTitle, 0) != nullptr) {
        GXUIWidgetNode* pCapPanel = static_cast<GXUIWidgetNode*>(
            g_pUIWidgetAllocator->vptr->AllocateAligned(g_pUIWidgetAllocator, 0x140, 8));
        if (pCapPanel) {
            GXUIWidgetNode* pStackTop = *reinterpret_cast<GXUIWidgetNode**>(builderCtx[2] - 8);
            GXUIRefStringHandle emptyStr = { nullptr, nullptr };
            UICapturePanel_Construct(pCapPanel, pStackTop, &emptyStr, 1, 0x201);
            GXUIWidget_AssignRef(&pThis->m_pCapturePanel810, pCapPanel);
        } else {
            GXUIWidget_ReleaseRef(&pThis->m_pCapturePanel810);
        }

        GXUIWidgetNode* pPanel = pThis->m_pCapturePanel810;
        void* pTextBox = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pPanel) + 0x110);
        UITextBox_Clear(pTextBox);
        UITextBox_SetDefaultText(pTextBox, L"/data/perf_log");
        uint8_t* pTbRaw = static_cast<uint8_t*>(*reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pPanel) + 0x110));
        *reinterpret_cast<uint64_t*>(pTbRaw + 0x50) = *reinterpret_cast<uint64_t*>(pTbRaw + 0x48);

        GXUIRefStringHandle tmpBtn = { nullptr, nullptr };
        void* pSaveBtn = UIBuilder_AddButtonItem(builderCtx, &tmpBtn);
        tmpBtn.m_pLiteral08 = nullptr;

        GXUIRefStringHandle saveLabel = { nullptr, nullptr };
        UIString_MakeFromWideLiteral(&saveLabel, L"Save Performance Log");
        GXUIRefStringHandle argStr;
        GXUIRefString_CopyInit(&argStr, &saveLabel);
        void* pFunctor = GXUICallbackFunctor_Create(pThis, GXPerfDebugMenu_OnSavePerformanceLogCallback);
        UIButtonItem_BindCallback(pSaveBtn, &argStr, pFunctor);
        GXUIRefString_Release(&argStr);
        GXUIRefString_Release(&saveLabel);

        if (builderCtx[1] != builderCtx[2]) {
            GXUIWidgetNode** ppStackTop = reinterpret_cast<GXUIWidgetNode**>(builderCtx[2] - 8);
            GXUIWidget_ReleaseRef(ppStackTop);
            builderCtx[2] -= 8;
        }
    }

    UIBuilder_Destroy(builderCtx);
    return pThis->m_pMainWindow818;
}

// [VMA: 0x217b870 - 0x217bc7a] (1,034 B)
// Checks m_bCaptureRequested80C (+0x80C), formats screenshot filename with RTC timestamp, and saves surface.
extern "C" void GXPerfDebugMenu_ExecutePendingScreenCapture(GXPerfDebugMenu* pThis, void* pRenderTargetSurface)
{
    if (pThis->m_bCaptureRequested80C == 0) {
        return;
    }

    struct LocalWideString {
        uint8_t       m_header[0x38];
        wchar_t*      m_pBuf;
        uint64_t      m_pad;
        uint64_t      m_length;
        uint64_t      m_capacity;
        IGXAllocator* m_pAllocator;
        uint8_t       m_bInitialized;
    };

    uint8_t* pTlsBase = nullptr;
    pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
    uint8_t* pTlsSlot = *reinterpret_cast<uint8_t**>(pTlsBase + g_TlsScratchArenaOffset);
    if (!pTlsSlot) {
        TlsScratchArena_InitThreadLocal();
        pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
        pTlsSlot = *reinterpret_cast<uint8_t**>(pTlsBase + g_TlsScratchArenaOffset);
    }
    IGXAllocator* pScratchAlloc1 = reinterpret_cast<IGXAllocator*>(pTlsSlot + 0x28);

    LocalWideString rawInputStr = {};
    reinterpret_cast<void (*)(void*, IGXAllocator*, uint32_t)>(
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pScratchAlloc1->vptr) + 0x20))(
        &rawInputStr, pScratchAlloc1, 0);
    if ((rawInputStr.m_header[0] & 0x20) == 0) {
        DebugLog_Warning(0, 0x3E, "Allocator flag check failed");
    }
    rawInputStr.m_pAllocator = pScratchAlloc1;
    rawInputStr.m_capacity = 7;
    rawInputStr.m_length = 0;
    rawInputStr.m_pBuf = nullptr;
    rawInputStr.m_bInitialized = 1;

    void* pTextBox = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pThis->m_pWindow7F8) + 0x110);
    UITextBox_GetText(pTextBox, &rawInputStr.m_pBuf, 1);

    pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
    pTlsSlot = *reinterpret_cast<uint8_t**>(pTlsBase + g_TlsScratchArenaOffset);
    if (!pTlsSlot) {
        TlsScratchArena_InitThreadLocal();
        pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
        pTlsSlot = *reinterpret_cast<uint8_t**>(pTlsBase + g_TlsScratchArenaOffset);
    }
    IGXAllocator* pScratchAlloc2 = reinterpret_cast<IGXAllocator*>(pTlsSlot + 0x28);

    LocalWideString formattedStr = {};
    uint8_t clockCtx[0x220] = {};
    reinterpret_cast<void (*)(void*, IGXAllocator*, uint32_t)>(
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(pScratchAlloc2->vptr) + 0x20))(
        clockCtx, pScratchAlloc2, 0);
    if ((clockCtx[0] & 0x20) == 0) {
        DebugLog_Warning(0, 0x3E, "Allocator flag check failed");
    }
    formattedStr.m_pAllocator = pScratchAlloc2;
    formattedStr.m_capacity = 7;
    formattedStr.m_length = 0;
    formattedStr.m_pBuf = nullptr;
    formattedStr.m_bInitialized = 1;

    const wchar_t* pFinalPath = nullptr;
    if (pThis->m_bCaptureFlag80D != 0) {
        uint64_t unpackedRtc[2] = { 0, 0 };
        if (QuerySystemClockTick(clockCtx) == 0) {
            uint64_t utcTick = 0;
            ConvertClockTickToUtc(clockCtx, &utcTick);
            if (utcTick != 0 && !UnpackRtcTimestamp(unpackedRtc, utcTick)) {
                unpackedRtc[0] = 0;
                unpackedRtc[1] = 0;
            }
        }
        const wchar_t* pInputChars = (rawInputStr.m_capacity >= 8)
            ? rawInputStr.m_pBuf
            : reinterpret_cast<const wchar_t*>(&rawInputStr.m_pBuf);

        wchar_t tempPathBuf[264] = {};
        const wchar_t* pBaseForFmt = pInputChars;
        if (WidePath_HasExtension(pInputChars) != nullptr) {
            size_t copyChars = (rawInputStr.m_length < 263u) ? rawInputStr.m_length : 263u;
            WideString_CopyCount(tempPathBuf, pInputChars, copyChars);
            tempPathBuf[copyChars] = L'\0';
            wchar_t* pLastDot   = WideString_FindLastChar(tempPathBuf, L'.');
            wchar_t* pLastSlash = WideString_FindLastChar(tempPathBuf, L'/');
            wchar_t* pLastBack  = WideString_FindLastChar(tempPathBuf, L'\\');
            if (pLastDot && pLastDot >= pLastSlash && pLastDot >= pLastBack) {
                *pLastDot = L'\0';
            }
            pBaseForFmt = tempPathBuf;
        }

        uint64_t packed = unpackedRtc[1];
        uint32_t year  = static_cast<uint32_t>(packed & 0xFFFu);
        uint32_t month = static_cast<uint32_t>((packed >> 22) & 0xFu);
        uint32_t day   = static_cast<uint32_t>((packed >> 29) & 0x1Fu);
        uint32_t hour  = static_cast<uint32_t>((packed >> 34) & 0x1Fu);
        uint32_t min   = static_cast<uint32_t>((packed >> 39) & 0x3Fu);
        uint32_t sec   = static_cast<uint32_t>((packed >> 45) & 0x3Fu);
        FormatScreenshotPath(&formattedStr, L"%s_%04d%02d%02d_%02d%02d%02d",
                             pBaseForFmt, year, month, day, hour, min, sec);
        pFinalPath = (formattedStr.m_capacity >= 8)
            ? formattedStr.m_pBuf
            : reinterpret_cast<const wchar_t*>(&formattedStr.m_pBuf);
    } else {
        pFinalPath = (rawInputStr.m_capacity >= 8)
            ? rawInputStr.m_pBuf
            : reinterpret_cast<const wchar_t*>(&rawInputStr.m_pBuf);
    }

    uint8_t* pGlobalDev = static_cast<uint8_t*>(g_pGXGlobalDeviceSingleton);
    void* pCaptureService = *reinterpret_cast<void**>(pGlobalDev + 0x660);
    if (pCaptureService) {
        uint32_t width  = (pThis->m_bCaptureFlag80E != 0) ? 0u : pThis->m_captureWidth804;
        uint32_t height = (pThis->m_bCaptureFlag80E != 0) ? 0u : pThis->m_captureHeight808;
        ExecuteScreenCaptureToDisk(pCaptureService, pRenderTargetSurface, pFinalPath,
                                   pThis->m_captureFormat800, width, height, 0);
    }

    if (formattedStr.m_capacity >= 8) {
        formattedStr.m_pAllocator->vptr->Free(formattedStr.m_pAllocator, formattedStr.m_pBuf);
    }
    if (rawInputStr.m_capacity >= 8) {
        rawInputStr.m_pAllocator->vptr->Free(rawInputStr.m_pAllocator, rawInputStr.m_pBuf);
    }
    pThis->m_bCaptureRequested80C = 0;
}

// [VMA: 0x217b6c0 - 0x217b861] (417 B)
// Unregisters a custom timer series from m_customTimerSlots68 (+0x68) and removes it from the active graph window.
extern "C" bool GXPerfDebugMenu_UnregisterCustomTimerSeries(GXPerfDebugMenu* pThis, void* pTimerSeriesHandle)
{
    uintptr_t rawBase = reinterpret_cast<uintptr_t>(&pThis->m_customTimerSlots68[0]);
    uintptr_t alignOffset = (0xFFFFFF98u - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pThis))) & 7u;
    uint8_t* pAlignedBase = reinterpret_cast<uint8_t*>(rawBase + alignOffset);
    uint64_t count = pThis->m_customTimerCount7F0;
    uint64_t totalBytes = count * sizeof(GXCustomTimerSlotEntry);
    uint8_t* pEnd = pAlignedBase + totalBytes;

    uint8_t* pFound = nullptr;
    for (uint8_t* pCur = pAlignedBase; pCur != pEnd; pCur += sizeof(GXCustomTimerSlotEntry)) {
        if (*reinterpret_cast<void**>(pCur) == pTimerSeriesHandle) {
            pFound = pCur;
            break;
        }
    }
    if (!pFound) {
        return false;
    }

    if (pFound < pAlignedBase || pFound >= pEnd) {
        DebugLog_Warning(0x2D4, 0, "Custom timer slot iterator out of bounds");
        count = pThis->m_customTimerCount7F0;
    }

    pEnd = pAlignedBase + (count * sizeof(GXCustomTimerSlotEntry));
    uint8_t* pCur = pFound;
    while (pCur < pEnd) {
        GXCustomTimerSlotEntry* pEntry = reinterpret_cast<GXCustomTimerSlotEntry*>(pCur);
        if (pEntry->m_strCap28 >= 8) {
            if ((pEntry->m_strCap28 + 1) >= 0x21) {
                DebugLog_Warning(0x120, 0, "Fixed string capacity overflow");
            }
            pEntry->m_allocFlags71 &= 0xFDu;
            count = pThis->m_customTimerCount7F0;
        }
        pEntry->m_strCap28 = 7;
        pEntry->m_strLen20 = 0;
        pEntry->m_inlineBuf10[0] = L'\0';

        uint8_t* pNext = pCur + sizeof(GXCustomTimerSlotEntry);
        pEnd = pAlignedBase + (count * sizeof(GXCustomTimerSlotEntry));
        if (pNext < pEnd) {
            GXShaderConstantSlotTable_CopyConstructEntry(
                &pThis->m_customTimerSlots68[0], pCur, pNext);
            count = pThis->m_customTimerCount7F0;
        }
        pEnd = pAlignedBase + (count * sizeof(GXCustomTimerSlotEntry));
        pCur = pNext;
    }

    pThis->m_customTimerCount7F0 = count - 1;

    GXUIWidgetNode* pGraphWin = pThis->m_pCustomGraphWindow850;
    if (pGraphWin) {
        uint8_t* pWinRaw = reinterpret_cast<uint8_t*>(pGraphWin);
        if ((pWinRaw[0x0B] & 0x40) != 0 && pWinRaw[0x6F] >= 2) {
            void** ppSeriesBegin = *reinterpret_cast<void***>(pWinRaw + 0x118);
            void** ppSeriesEnd   = *reinterpret_cast<void***>(pWinRaw + 0x120);
            for (void** ppIt = ppSeriesBegin; ppIt != ppSeriesEnd; ++ppIt) {
                void* pSeriesObj = *ppIt;
                if (*reinterpret_cast<void**>(static_cast<uint8_t*>(pSeriesObj) + 0x30) == pTimerSeriesHandle) {
                    if (pSeriesObj) {
                        UIGraphSeries_Remove(pGraphWin, pSeriesObj);
                    }
                    break;
                }
            }
        }
    }
    return true;
}

// [VMA: 0x217b440 - 0x217b6bd] (637 B)
// Registers a named custom timer series (up to 16 slots) and adds it to the active Custom Timer Graph window.
extern "C" bool GXPerfDebugMenu_RegisterCustomTimerSeries(
    GXPerfDebugMenu* pThis,
    void* pTimerSeriesHandle,
    const char* pSeriesName)
{
    if (!pTimerSeriesHandle || pThis->m_customTimerCount7F0 > 15) {
        return false;
    }

    GXCustomTimerSlotEntry tempSlot = {};
    tempSlot.m_pTimerHandle00 = pTimerSeriesHandle;
    InitCharConversionTable(&tempSlot.m_pStrAlloc30, &tempSlot.m_allocFlags71, 0x41);
    tempSlot.m_allocFlags71 = 0;
    tempSlot.m_strCap28 = 7;
    tempSlot.m_strLen20 = 0;
    tempSlot.m_inlineBuf10[0] = L'\0';
    tempSlot.m_pad72[0] = 1;

    sub_27cbc20(&tempSlot.m_reserved08, 0x1Fu, 0u);
    tempSlot.m_strLen20 = 0;
    if (tempSlot.m_strCap28 >= 8) {
        tempSlot.m_pHeapBuf10[0] = L'\0';
    } else {
        tempSlot.m_inlineBuf10[0] = L'\0';
    }

    size_t nameLen = AnsiString_Length(pSeriesName);
    if (nameLen > 0x1F) {
        nameLen = 0x1F;
    }
    ConvertAnsiToWideString(&tempSlot.m_reserved08, pSeriesName, nameLen);

    uint64_t curCount = pThis->m_customTimerCount7F0;
    if (curCount + 1 >= 17) {
        DebugLog_Warning(0x24C, 0, "Custom timer slot capacity exceeded (max 16)");
        curCount = pThis->m_customTimerCount7F0;
    }
    pThis->m_customTimerCount7F0 = curCount + 1;

    uintptr_t rawBase = reinterpret_cast<uintptr_t>(&pThis->m_customTimerSlots68[0]);
    uintptr_t alignOffset = (0 - static_cast<uint32_t>(rawBase)) & 7u;
    uint8_t* pDstSlot = reinterpret_cast<uint8_t*>(pThis) + alignOffset + 0x68 + (curCount * sizeof(GXCustomTimerSlotEntry));
    GXShaderConstantSlotTable_CopyConstructEntry(&pThis->m_customTimerSlots68[0], pDstSlot, &tempSlot);

    GXUIWidgetNode* pGraphWin = pThis->m_pCustomGraphWindow850;
    if (pGraphWin) {
        uint8_t* pWinRaw = reinterpret_cast<uint8_t*>(pGraphWin);
        if ((pWinRaw[0x0B] & 0x40) != 0 && pWinRaw[0x6F] >= 2) {
            const wchar_t* pWideName = (tempSlot.m_strCap28 >= 8) ? tempSlot.m_pHeapBuf10 : tempSlot.m_inlineBuf10;
            GXUIRefStringHandle seriesTitle = { nullptr, nullptr };
            UIString_MakeFromWideLiteral(&seriesTitle, pWideName);
            UICustomGraphWin_AddSeries(pGraphWin, &seriesTitle, tempSlot.m_pTimerHandle00, 0xFFFFFFFFu);
            GXUIRefString_Release(&seriesTitle);
        }
    }

    if (tempSlot.m_strCap28 >= 8) {
        if ((tempSlot.m_strCap28 + 1) >= 0x21) {
            DebugLog_Warning(0x120, 0, "Fixed string capacity overflow");
        }
        tempSlot.m_allocFlags71 &= 0xFDu;
    }
    return true;
}

// [VMA: 0x217b3f0 - 0x217b436] (70 B)
// Shuts down and frees the debug menu's 256 KB sub-allocator (+0x00).
extern "C" void GXPerfDebugMenu_DestroySubAllocator(GXPerfDebugMenu* pThis)
{
    void* pSubAlloc = pThis->m_pSubAllocator00;
    if (pSubAlloc) {
        IGXAllocator* pParentAlloc = SubAllocator_GetParentAllocator(pSubAlloc);
        SubAllocator_Shutdown(pSubAlloc);
        pParentAlloc->vptr->Free(pParentAlloc, pSubAlloc);
        pThis->m_pSubAllocator00 = nullptr;
    }
}

// [VMA: 0x217b3a0 - 0x217b3eb] (75 B)
// Initializes the 256 KB (0x40000), 16-byte aligned sub-allocator on the global GX device.
extern "C" bool GXPerfDebugMenu_InitializeSubAllocator(GXPerfDebugMenu* pThis)
{
    void* pDev = *static_cast<void**>(g_pGXGlobalDeviceSingleton);
    int status = SubAllocator_Initialize(
        pThis, pDev, 0x40000u, 0x10u, g_pGXDefaultHeapAllocator, 0, 0, 0);
    return (status == 0);
}

// [VMA: 0x217b2f0 - 0x217b39d] (173 B)
// Converts 19 raw GPU timing values at pGpuProfilerState + 0x6A3D8 into milliseconds (x * 1000.0f / freq).
extern "C" void GXPerfDebugMenu_UpdateGpuTimingCounters(GXPerfDebugMenu* pThis, const uint8_t* pGpuProfilerState)
{
    const float* pSrc = reinterpret_cast<const float*>(pGpuProfilerState + 0x6A3D8);
    float* pDst = &pThis->m_gpuTimingsMs08[0];

    const float kScaleMs  = 1000.0f;
    const float kGpuClock = 800000000.0f;

    // Check for pointer aliasing before running the 8-wide AVX2 loop for the first 16 floats
    uintptr_t dstBegin = reinterpret_cast<uintptr_t>(pDst);
    uintptr_t dstEnd   = reinterpret_cast<uintptr_t>(pDst + 18);
    uintptr_t srcBegin = reinterpret_cast<uintptr_t>(pSrc);
    uintptr_t srcEnd   = reinterpret_cast<uintptr_t>(pSrc + 18);

    size_t idx = 0;
    if (dstBegin > srcEnd || srcBegin > dstEnd) {
        __m256 vScale = _mm256_set1_ps(kScaleMs);
        __m256 vClock = _mm256_set1_ps(kGpuClock);
        for (; idx < 16; idx += 8) {
            __m256 vVal = _mm256_loadu_ps(pSrc + idx);
            __m256 vMs  = _mm256_div_ps(_mm256_mul_ps(vVal, vScale), vClock);
            _mm256_storeu_ps(pDst + idx, vMs);
        }
    }

    for (; idx < 19; ++idx) {
        pDst[idx] = (pSrc[idx] * kScaleMs) / kGpuClock;
    }
}

// [VMA: 0x217b2d0 - 0x217b2e6] (22 B)
// Flushes the primary command context of the global GX device singleton.
extern "C" void GXPerfDebugMenu_FlushGlobalDeviceCommandContext()
{
    uint8_t* pGlobalDev = static_cast<uint8_t*>(g_pGXGlobalDeviceSingleton);
    uint8_t* pPrimaryDev = *reinterpret_cast<uint8_t**>(pGlobalDev);
    void* pCmdCtx = *reinterpret_cast<void**>(pPrimaryDev + 0x18);
    FlushCommandContext(pCmdCtx);
}

// [VMA: 0x217ac30 - 0x217b2c7] (1,687 B)
// Destructor for GXPerfDebugMenu: releases all 10 UI window/widget references and frees custom timer strings.
extern "C" void GXPerfDebugMenu_Destructor(GXPerfDebugMenu* pThis)
{
    GXUIWidget_ReleaseRef(&pThis->m_pCustomGraphWindow850);
    GXUIWidget_ReleaseRef(&pThis->m_pTimingTableWidget848);
    GXUIWidget_ReleaseRef(&pThis->m_pTimingTableWin840);
    GXUIWidget_ReleaseRef(&pThis->m_pGpuGraphWindow838);
    GXUIWidget_ReleaseRef(&pThis->m_pStatsSubPanel830);
    GXUIWidget_ReleaseRef(&pThis->m_pStatsWindow828);
    GXUIWidget_ReleaseRef(&pThis->m_pSubPanel820);
    GXUIWidget_ReleaseRef(&pThis->m_pMainWindow818);
    GXUIWidget_ReleaseRef(&pThis->m_pCapturePanel810);
    GXUIWidget_ReleaseRef(&pThis->m_pWindow7F8);

    uintptr_t rawBase = reinterpret_cast<uintptr_t>(&pThis->m_customTimerSlots68[0]);
    uintptr_t alignOffset = (0 - static_cast<uint32_t>(rawBase)) & 7u;
    uint8_t* pAlignedBase = reinterpret_cast<uint8_t*>(rawBase + alignOffset);
    uint8_t* pEnd = pAlignedBase + (pThis->m_customTimerCount7F0 * sizeof(GXCustomTimerSlotEntry));

    for (uint8_t* pCur = pAlignedBase; pCur < pEnd; pCur += sizeof(GXCustomTimerSlotEntry)) {
        GXCustomTimerSlotEntry* pEntry = reinterpret_cast<GXCustomTimerSlotEntry*>(pCur);
        if (pEntry->m_strCap28 >= 8) {
            if ((pEntry->m_strCap28 + 1) >= 0x21) {
                DebugLog_Warning(0x120, 0, "Fixed string capacity overflow");
            }
            pEntry->m_allocFlags71 &= 0xFDu;
        }
        pEntry->m_strCap28 = 7;
        pEntry->m_strLen20 = 0;
        pEntry->m_inlineBuf10[0] = L'\0';
    }
}

// [VMA: 0x217ab10 - 0x217ac28] (280 B)
// Constructor for GXPerfDebugMenu: initializes capture resolution to 1280x720 and zeroes all UI pointers.
extern "C" void GXPerfDebugMenu_Constructor(GXPerfDebugMenu* pThis)
{
    pThis->m_pSubAllocator00 = nullptr;
    pThis->m_reserved60[0] = 0;
    pThis->m_captureFormat800 = 0;
    pThis->m_pWindow7F8 = nullptr;
    pThis->m_customTimerCount7F0 = 0;
    pThis->m_captureWidth804 = 1280;  // 0x500
    pThis->m_captureHeight808 = 720;  // 0x2D0
    pThis->m_bCaptureRequested80C = 0;
    pThis->m_bCaptureFlag80D = 1;
    pThis->m_bCaptureFlag80E = 1;
    pThis->m_pCustomGraphWindow850 = nullptr;
    pThis->m_pTimingTableWidget848 = nullptr;
    pThis->m_pTimingTableWin840 = nullptr;
    pThis->m_pGpuGraphWindow838 = nullptr;
    pThis->m_pStatsSubPanel830 = nullptr;
    pThis->m_pStatsWindow828 = nullptr;
    pThis->m_pSubPanel820 = nullptr;
    pThis->m_pMainWindow818 = nullptr;
    pThis->m_pCapturePanel810 = nullptr;
    pThis->m_reserved858 = 1;
    memset(&pThis->m_gpuTimingsMs08[0], 0, 22 * sizeof(float));
}

// [VMA: 0x217aa70 - 0x217aa71] (1 B)
extern "C" void GXPerfDebugMenu_NoOpStub() {}

// ============================================================================
// 2. GXGnmSurfaceAndViewInitializer (0x217a210 .. 0x217a8b0)
// ============================================================================

struct GXGnmViewInitDesc {
    uint32_t m_format00;      // +0x00
    uint32_t m_viewType04;    // +0x04
    uint32_t m_firstElem08;   // +0x08 (firstMip / firstElement / flags)
    uint32_t m_numElems0C;    // +0x0C (numMips / numElements / firstSlice)
    uint32_t m_firstSlice10;  // +0x10 (firstSlice / bufferFlags)
    uint32_t m_numSlices14;   // +0x14 (numSlices)
};

// Helper to compute GNM resource memory type (RO/GC/SC/WC) from surface flags at +0x39..+0x3C
static inline uint32_t GXGnmSurface_ComputeMemoryTypeCode(const uint8_t* pSurfaceObj)
{
    uint8_t  flags3B = pSurfaceObj[0x3B];
    uint32_t flags3C = *reinterpret_cast<const uint32_t*>(pSurfaceObj + 0x3C);

    if ((flags3B & 0x04) != 0) {
        return 0x6E0000u;
    }
    if ((flags3B & 0x60) != 0) {
        return 0x600001u;
    }
    if ((((flags3C << 5) | flags3B) & 0x102u) != 0) {
        return 0x6E0001u;
    }
    if ((flags3B & 0x90) != 0) {
        return 0x600001u;
    }
    if (pSurfaceObj[0x39] == 2) {
        return 0x6E0001u;
    }
    return 0x100000u;
}

static inline void RegisterDirectTextureViewFromInnerSurface(void* pOutView,
                                                             const uint8_t* pInnerSurf) noexcept {
    const uint32_t resId = *reinterpret_cast<const uint32_t*>(pInnerSurf + 0x10);
    const uint8_t dimType = pInnerSurf[0x20];
    const uint8_t usageMode = pInnerSurf[0x21];
    const uint8_t cpuAccess = pInnerSurf[0x22];
    const uint8_t bindFlags = pInnerSurf[0x23];
    const uint16_t sampleCnt =
        (dimType == 3u) ? *reinterpret_cast<const uint16_t*>(pInnerSurf + 0x34) : 1u;
    const uint32_t isImmutable =
        (usageMode == 1u && bindFlags == 8u && cpuAccess == 0u && sampleCnt <= 1u) ? 1u : 0u;
    gx_direct_vk::RegisterDirectNativeSrv(pOutView, resId, isImmutable);
}

// [VMA: 0x217a8b0 - 0x217aa63] (435 B)
// Initializes a GNM Texture or Buffer Shader Resource View (SRV) from a surface object and view descriptor.
extern "C" int GXGnmSurfaceView_InitShaderResourceView(
    uint8_t* pOutView,
    void* /*pUnused*/,
    const uint8_t* pSurfaceObj,
    const GXGnmViewInitDesc* pViewDesc)
{
    uint32_t memTypeCode = GXGnmSurface_ComputeMemoryTypeCode(pSurfaceObj);
    const uint8_t* pInnerSurf = pSurfaceObj + 0x18;
    uint32_t viewType = pViewDesc->m_viewType04;

    if ((viewType - 2u) < 5u) {
        int res = GXGnmTexture_InitializeFromSurface(
            pOutView,
            pInnerSurf,
            pViewDesc->m_format00,
            pViewDesc->m_firstElem08,
            pViewDesc->m_numElems0C,
            1u,
            pViewDesc->m_firstSlice10,
            0u);
        if (res != 0) {
            return res;
        }
        sceGnmTextureSetResourceMemoryType(pOutView, memTypeCode >> 16, 0);
        RegisterDirectTextureViewFromInnerSurface(pOutView, pInnerSurf);
        *reinterpret_cast<uint32_t*>(pOutView + 0x30) = pViewDesc->m_firstSlice10;
        *reinterpret_cast<uint64_t*>(pOutView + 0x28) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstElem08);
        *reinterpret_cast<uint64_t*>(pOutView + 0x20) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_format00);
        return 0;
    }

    if (viewType != 1u) {
        return 8;
    }

    uint16_t elemStride = *reinterpret_cast<const uint16_t*>(pInnerSurf + 0x2A);
    uint32_t flags3C    = *reinterpret_cast<const uint32_t*>(pSurfaceObj + 0x3C);
    uint64_t gpuBaseAddr = *reinterpret_cast<const uint64_t*>(pSurfaceObj + 0x20);

    if (elemStride != 0 && (flags3C & 0x20u) != 0) {
        uint64_t byteOffset = static_cast<uint64_t>(pViewDesc->m_firstElem08) * elemStride;
        sceGnmBufferInitializeAsRegular(pOutView, gpuBaseAddr + byteOffset, elemStride, pViewDesc->m_numElems0C);
    } else if ((pViewDesc->m_firstSlice10 & 0x01u) != 0 && (flags3C & 0x10u) != 0) {
        uint64_t byteOffset = static_cast<uint64_t>(pViewDesc->m_firstElem08) << 2;
        sceGnmBufferInitializeAsByteBuffer(pOutView, gpuBaseAddr + byteOffset, pViewDesc->m_numElems0C << 2);
    } else {
        uint32_t gnmDataFmt = 0;
        int fmtRes = GXGnmFormat_GetDataFormatDescriptor(&gnmDataFmt, pViewDesc->m_format00);
        if (fmtRes != 0) {
            return fmtRes;
        }
#undef g_GXGnmFormatBytesPerElementTable
#define g_GXGnmFormatBytesPerElementTable (reinterpret_cast<const uint64_t*>(GX_VMA(0x47388d0ULL)))
        uint32_t fmtIdx = pViewDesc->m_format00 - 1u;
        uint64_t bytesPerElem = (fmtIdx <= 0x62u) ? g_GXGnmFormatBytesPerElementTable[fmtIdx] : 0ull;
        uint64_t byteOffset = bytesPerElem * pViewDesc->m_firstElem08;
        sceGnmBufferInitializeAsData(pOutView, gpuBaseAddr + byteOffset, gnmDataFmt, pViewDesc->m_numElems0C);
    }

    sceGnmBufferSetResourceMemoryType(pOutView, memTypeCode >> 16, 0, memTypeCode & 1u);
    *reinterpret_cast<uint32_t*>(pOutView + 0x30) = pViewDesc->m_firstSlice10;
    *reinterpret_cast<uint64_t*>(pOutView + 0x28) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstElem08);
    *reinterpret_cast<uint64_t*>(pOutView + 0x20) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_format00);
    return 0;
}

// [VMA: 0x217a800 - 0x217a818] (24 B)
// Copies the 24-byte cached SRV/UAV view descriptor from pViewObj (+0x20..+0x38) to pOutDesc.
extern "C" void GXGnmSurfaceView_GetShaderResourceViewDesc(const uint8_t* pViewObj, uint64_t* pOutDesc)
{
    pOutDesc[2] = *reinterpret_cast<const uint64_t*>(pViewObj + 0x30);
    pOutDesc[1] = *reinterpret_cast<const uint64_t*>(pViewObj + 0x28);
    pOutDesc[0] = *reinterpret_cast<const uint64_t*>(pViewObj + 0x20);
}

// [VMA: 0x217a7f0 - 0x217a7f1] (1 B)
extern "C" void GXGnmSurfaceView_ReleaseShaderResourceView() {}

// [VMA: 0x217a5d0 - 0x217a7e4] (532 B)
// Initializes a GNM Texture, Cubemap, or Buffer Unordered Access View (UAV) from a surface object.
extern "C" int GXGnmSurfaceView_InitUnorderedAccessView(
    uint8_t* pOutView,
    void* /*pUnused*/,
    const uint8_t* pSurfaceObj,
    const GXGnmViewInitDesc* pViewDesc)
{
    uint32_t memTypeCode = GXGnmSurface_ComputeMemoryTypeCode(pSurfaceObj);
    uint32_t viewTypeIdx = pViewDesc->m_viewType04 - 1u;
    if (viewTypeIdx > 9u) {
        return 8;
    }

    const uint8_t* pInnerSurf = pSurfaceObj + 0x18;
    uint32_t firstSlice = 0;
    uint32_t numSlices  = 1;
    uint32_t isCubemap  = 0;

    switch (viewTypeIdx) {
    case 0: // Buffer UAV
    {
        uint16_t elemStride  = *reinterpret_cast<const uint16_t*>(pInnerSurf + 0x2A);
        uint32_t flags3C     = *reinterpret_cast<const uint32_t*>(pSurfaceObj + 0x3C);
        uint64_t gpuBaseAddr = *reinterpret_cast<const uint64_t*>(pSurfaceObj + 0x20);

        if (elemStride != 0 && (flags3C & 0x20u) != 0) {
            uint64_t byteOffset = static_cast<uint64_t>(pViewDesc->m_firstElem08) * elemStride;
            sceGnmBufferInitializeAsRegular(pOutView, gpuBaseAddr + byteOffset, elemStride, pViewDesc->m_numElems0C);
        } else if ((pViewDesc->m_firstSlice10 & 0x01u) != 0 && (flags3C & 0x10u) != 0) {
            sceGnmBufferInitializeAsByteBuffer(pOutView, gpuBaseAddr + pViewDesc->m_firstElem08, pViewDesc->m_numElems0C);
        } else {
            uint32_t gnmDataFmt = 0;
            int fmtRes = GXGnmFormat_GetDataFormatDescriptor(&gnmDataFmt, pViewDesc->m_format00);
            if (fmtRes != 0) {
                return fmtRes;
            }
#undef g_GXGnmFormatBytesPerElementTable
#define g_GXGnmFormatBytesPerElementTable (reinterpret_cast<const uint64_t*>(GX_VMA(0x47388d0ULL)))
            uint32_t fmtIdx = pViewDesc->m_format00 - 1u;
            uint64_t bytesPerElem = (fmtIdx <= 0x62u) ? g_GXGnmFormatBytesPerElementTable[fmtIdx] : 0ull;
            uint64_t byteOffset = bytesPerElem * pViewDesc->m_firstElem08;
            sceGnmBufferInitializeAsData(pOutView, gpuBaseAddr + byteOffset, gnmDataFmt, pViewDesc->m_numElems0C);
        }
        sceGnmBufferSetResourceMemoryType(pOutView, memTypeCode >> 16, 0, memTypeCode & 1u);
        *reinterpret_cast<uint64_t*>(pOutView + 0x30) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstSlice10);
        *reinterpret_cast<uint64_t*>(pOutView + 0x28) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstElem08);
        *reinterpret_cast<uint64_t*>(pOutView + 0x20) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_format00);
        return 0;
    }
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        firstSlice = pViewDesc->m_firstSlice10;
        numSlices  = pViewDesc->m_numSlices14;
        isCubemap  = 0;
        break;
    case 7:
        firstSlice = 0;
        numSlices  = 1;
        isCubemap  = 0;
        break;
    case 8:
    case 9:
        firstSlice = pViewDesc->m_firstSlice10 * 6u;
        numSlices  = pViewDesc->m_numSlices14 * 6u;
        isCubemap  = 1;
        break;
    }

    int res = GXGnmTexture_InitializeFromSurface(
        pOutView,
        pInnerSurf,
        pViewDesc->m_format00,
        pViewDesc->m_firstElem08,
        firstSlice,
        pViewDesc->m_numElems0C,
        numSlices,
        isCubemap);
    if (res != 0) {
        return res;
    }

    sceGnmTextureSetResourceMemoryType(pOutView, memTypeCode >> 16, 0);
    RegisterDirectTextureViewFromInnerSurface(pOutView, pInnerSurf);
    *reinterpret_cast<uint64_t*>(pOutView + 0x30) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstSlice10);
    *reinterpret_cast<uint64_t*>(pOutView + 0x28) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstElem08);
    *reinterpret_cast<uint64_t*>(pOutView + 0x20) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_format00);
    return 0;
}

// [VMA: 0x217a520 - 0x217a536] (22 B)
// Copies the 20-byte cached DepthRenderTarget view descriptor from pViewObj (+0x2C..+0x40) to pOutDesc.
extern "C" void GXGnmSurfaceView_GetDepthRenderTargetViewDesc(const uint8_t* pViewObj, uint8_t* pOutDesc)
{
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x10) = *reinterpret_cast<const uint32_t*>(pViewObj + 0x3C);
    *reinterpret_cast<uint64_t*>(pOutDesc + 0x08) = *reinterpret_cast<const uint64_t*>(pViewObj + 0x34);
    *reinterpret_cast<uint64_t*>(pOutDesc + 0x00) = *reinterpret_cast<const uint64_t*>(pViewObj + 0x2C);
}

// [VMA: 0x217a510 - 0x217a511] (1 B)
extern "C" void GXGnmSurfaceView_ReleaseDepthRenderTargetView() {}

// [VMA: 0x217a4b0 - 0x217a50b] (91 B)
// Initializes a GNM RenderTarget view from a surface object and view descriptor.
extern "C" int GXGnmSurfaceView_InitDepthRenderTargetView(
    uint8_t* pOutDepthView,
    void* /*pUnused*/,
    const uint8_t* pSurfaceObj,
    const GXGnmViewInitDesc* pViewDesc)
{
    if ((pViewDesc->m_viewType04 - 1u) > 7u) {
        return 8;
    }

    int res = GXGnmRenderTarget_InitializeFromSurface(
        pOutDepthView,
        pSurfaceObj + 0x18,
        pViewDesc->m_format00,
        pViewDesc->m_firstElem08,
        pViewDesc->m_numElems0C,
        pViewDesc->m_firstSlice10);
    if (res == 0) {
        *reinterpret_cast<uint32_t*>(pOutDepthView + 0x3C) = pViewDesc->m_firstSlice10;
        *reinterpret_cast<uint64_t*>(pOutDepthView + 0x34) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstElem08);
        *reinterpret_cast<uint64_t*>(pOutDepthView + 0x2C) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_format00);
        gx_direct_vk::RegisterDirectDepthTarget(
            pOutDepthView, nullptr, pViewDesc->m_firstElem08, pViewDesc->m_numElems0C, pViewDesc->m_firstSlice10);
    }
    return res;
}

// [VMA: 0x217a400 - 0x217a418] (24 B)
// Copies the 24-byte cached DepthRenderTarget view descriptor from pViewObj (+0x34..+0x4C) to pOutDesc.
extern "C" void GXGnmSurfaceView_GetRenderTargetViewDesc(const uint8_t* pViewObj, uint8_t* pOutDesc)
{
    *reinterpret_cast<uint64_t*>(pOutDesc + 0x10) = *reinterpret_cast<const uint64_t*>(pViewObj + 0x44);
    *reinterpret_cast<uint64_t*>(pOutDesc + 0x08) = *reinterpret_cast<const uint64_t*>(pViewObj + 0x3C);
    *reinterpret_cast<uint64_t*>(pOutDesc + 0x00) = *reinterpret_cast<const uint64_t*>(pViewObj + 0x34);
}

// [VMA: 0x217a3f0 - 0x217a3f1] (1 B)
extern "C" void GXGnmSurfaceView_ReleaseRenderTargetView() {}

// [VMA: 0x217a210 - 0x217a3e1] (465 B)
// Initializes a GNM RenderTarget view and optionally clears its CMask buffer to 0x0000000F (fast-clear state).
extern "C" int GXGnmSurfaceView_InitRenderTargetViewAndClearCMask(
    uint8_t* pOutRtView,
    void* /*pUnused*/,
    const uint8_t* pSurfaceObj,
    const GXGnmViewInitDesc* pViewDesc)
{
    if ((pViewDesc->m_viewType04 - 1u) > 5u) {
        return 8;
    }

    const uint8_t* pInnerSurf = pSurfaceObj + 0x18;
    int res = GXGnmDepthTarget_InitializeFromSurface(
        pOutRtView,
        pInnerSurf,
        pViewDesc->m_format00,
        pViewDesc->m_numElems0C,
        pViewDesc->m_firstSlice10,
        pViewDesc->m_numSlices14);
    if (res != 0) {
        return res;
    }

    *reinterpret_cast<uint64_t*>(pOutRtView + 0x44) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstSlice10);
    *reinterpret_cast<uint64_t*>(pOutRtView + 0x3C) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_firstElem08);
    *reinterpret_cast<uint64_t*>(pOutRtView + 0x34) = *reinterpret_cast<const uint64_t*>(&pViewDesc->m_format00);

    uint32_t rtFlags = pViewDesc->m_firstElem08;
    if (rtFlags == 1u) {
        *reinterpret_cast<uint32_t*>(pOutRtView + 0x4C) =
            GXGnmFormat_SupportsCMaskFastClear(pInnerSurf[0x23], pViewDesc->m_format00);
    } else if (rtFlags == 3u) {
        *reinterpret_cast<uint32_t*>(pOutRtView + 0x4C) = 1u;
    } else {
        *reinterpret_cast<uint32_t*>(pOutRtView + 0x4C) = 0u;
    }

    sceGnmRenderTargetSetCmaskClearColor(pOutRtView, ((rtFlags & 0x04u) != 0) ? 0u : 1u);
    if (!sceGnmRenderTargetGetCmaskFastClearEnable(pOutRtView)) {
        gx_direct_vk::RegisterDirectColorTarget(
            pOutRtView, nullptr, pViewDesc->m_numElems0C, pViewDesc->m_firstSlice10, pViewDesc->m_numSlices14);
        return 0;
    }
    if ((rtFlags & 0x08u) == 0) {
        gx_direct_vk::RegisterDirectColorTarget(
            pOutRtView, nullptr, pViewDesc->m_numElems0C, pViewDesc->m_firstSlice10, pViewDesc->m_numSlices14);
        return 0;
    }

    sceGnmRenderTargetSetCmaskFastClearEnable(pOutRtView, 1u);
    gx_direct_vk::RegisterDirectColorTarget(
        pOutRtView, nullptr, pViewDesc->m_numElems0C, pViewDesc->m_firstSlice10, pViewDesc->m_numSlices14);
    uintptr_t cmaskBase = static_cast<uintptr_t>(sceGnmRenderTargetGetCmaskAddress256ByteBlocks(pOutRtView)) << 8;
    uint32_t cmaskSliceBytes = sceGnmRenderTargetGetCmaskSliceSizeInBytes(pOutRtView);
    uint32_t baseSlice = sceGnmRenderTargetGetBaseArraySliceIndex(pOutRtView);
    if (baseSlice != 0) {
        uint32_t lastSlice = sceGnmRenderTargetGetLastArraySliceIndex(pOutRtView);
        uint32_t numSlices = (1u - baseSlice) + lastSlice;
        cmaskBase += static_cast<uintptr_t>((cmaskSliceBytes / numSlices) * baseSlice);
    }

    uint32_t dwordCount = (cmaskSliceBytes + 3u) >> 2;
    if (dwordCount != 0) {
        uint32_t* pCmaskDwords = reinterpret_cast<uint32_t*>(cmaskBase);
        uint32_t vecCount = dwordCount & ~0x1Fu;
        uint32_t idx = 0;
        if (vecCount != 0) {
            __m128i vClear = _mm_set1_epi32(0x0000000F);
            for (; idx < vecCount; idx += 32) {
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx +  0), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx +  4), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx +  8), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx + 12), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx + 16), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx + 20), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx + 24), vClear);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pCmaskDwords + idx + 28), vClear);
            }
        }
        for (; idx < dwordCount; ++idx) {
            pCmaskDwords[idx] = 0x0000000Fu;
        }
    }
    return 0;
}

// ============================================================================
// 3. GXRefCountedCommandPacket (0x2179a70 .. 0x217a160)
// ============================================================================

struct GXRefCountedPacketBase {
    void*             vptr;       // +0x00
    GXRefCountedOwner* m_pOwner08;// +0x08
    __m128            m_payload10[4]; // +0x10 .. +0x50
};

static inline void GXRefCountedPacketBase_ReleaseOwner(GXRefCountedPacketBase* pThis)
{
    pThis->vptr = &g_vtable_GXRefCountedBase;
    GXRefCountedOwner* pOwner = pThis->m_pOwner08;
    if (pOwner) {
        if (__sync_fetch_and_add(&pOwner->m_refCount10, -1) == 1) {
            IGXAllocator* pAlloc = pOwner->m_pAllocator08;
            pOwner->vptr->OnZeroRef(pOwner);
            pAlloc->vptr->Free(pAlloc, pOwner);
        }
    }
}

// [VMA: 0x217a160 - 0x217a178] (24 B)
extern "C" void GXRefCountedPacketBase_Construct(GXRefCountedPacketBase* pThis, GXRefCountedOwner* pOwner)
{
    pThis->vptr = &g_vtable_GXRefCountedBase;
    pThis->m_pOwner08 = pOwner;
    if (pOwner) {
        __sync_fetch_and_add(&pOwner->m_refCount10, 1);
    }
}

// [VMA: 0x217a110 - 0x217a15d] (77 B)
extern "C" void GXRefCountedPacketBase_Destructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
}

// [VMA: 0x217a0b0 - 0x217a109] (89 B)
extern "C" void GXRefCountedPacketBase_DeletingDestructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
    operator_delete(pThis);
}

// [VMA: 0x2179fd0 - 0x217a01d] (77 B)
extern "C" void GXCommandPacket64_TypeB_Destructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
}

// [VMA: 0x2179f70 - 0x2179fc9] (89 B)
extern "C" void GXCommandPacket64_TypeB_DeletingDestructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
    operator_delete(pThis);
}

// [VMA: 0x2179f20 - 0x2179f69] (73 B)
extern "C" void GXCommandPacket64_TypeB_Construct(
    GXRefCountedPacketBase* pThis,
    const __m128* pSrcPayload64,
    GXRefCountedOwner* pOwner)
{
    pThis->vptr = &g_vtable_GXRefCountedBase;
    pThis->m_pOwner08 = pOwner;
    if (pOwner) {
        __sync_fetch_and_add(&pOwner->m_refCount10, 1);
    }
    pThis->vptr = &g_vtable_GXCommandPacket64_TypeB;
    __m128 v0 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 0));
    __m128 v1 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 1));
    __m128 v2 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 2));
    __m128 v3 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 3));
    pThis->m_payload10[0] = v0;
    pThis->m_payload10[1] = v1;
    pThis->m_payload10[2] = v2;
    pThis->m_payload10[3] = v3;
}

// [VMA: 0x2179e40 - 0x2179e8d] (77 B)
extern "C" void GXCommandPacket64_TypeA_Destructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
}

// [VMA: 0x2179de0 - 0x2179e39] (89 B)
extern "C" void GXCommandPacket64_TypeA_DeletingDestructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
    operator_delete(pThis);
}

// [VMA: 0x2179d90 - 0x2179dd9] (73 B)
extern "C" void GXCommandPacket64_TypeA_Construct(
    GXRefCountedPacketBase* pThis,
    const __m128* pSrcPayload64,
    GXRefCountedOwner* pOwner)
{
    pThis->vptr = &g_vtable_GXRefCountedBase;
    pThis->m_pOwner08 = pOwner;
    if (pOwner) {
        __sync_fetch_and_add(&pOwner->m_refCount10, 1);
    }
    pThis->vptr = &g_vtable_GXCommandPacket64_TypeA;
    __m128 v0 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 0));
    __m128 v1 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 1));
    __m128 v2 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 2));
    __m128 v3 = _mm_loadu_ps(reinterpret_cast<const float*>(pSrcPayload64 + 3));
    pThis->m_payload10[0] = v0;
    pThis->m_payload10[1] = v1;
    pThis->m_payload10[2] = v2;
    pThis->m_payload10[3] = v3;
}

// [VMA: 0x2179cb0 - 0x2179cfd] (77 B)
extern "C" void GXCommandPacket80_Destructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
}

// [VMA: 0x2179c50 - 0x2179ca9] (89 B)
extern "C" void GXCommandPacket80_DeletingDestructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
    operator_delete(pThis);
}

// [VMA: 0x2179be0 - 0x2179c41] (97 B)
extern "C" void GXCommandPacket80_Construct(
    uint64_t* pThis,
    const uint64_t* pSrcPayload64,
    GXRefCountedOwner* pOwner)
{
    pThis[0] = reinterpret_cast<uint64_t>(&g_vtable_GXRefCountedBase);
    pThis[1] = reinterpret_cast<uint64_t>(pOwner);
    if (pOwner) {
        __sync_fetch_and_add(&pOwner->m_refCount10, 1);
    }
    pThis[0] = reinterpret_cast<uint64_t>(&g_vtable_GXCommandPacket80);
    pThis[9] = pSrcPayload64[7];
    pThis[8] = pSrcPayload64[6];
    pThis[7] = pSrcPayload64[5];
    pThis[6] = pSrcPayload64[4];
    pThis[5] = pSrcPayload64[3];
    pThis[4] = pSrcPayload64[2];
    pThis[3] = pSrcPayload64[1];
    pThis[2] = pSrcPayload64[0];
}

#undef g_vtable_GXCommandPacket96
#define g_vtable_GXCommandPacket96 (*reinterpret_cast<void**>(GX_VMA(0x535f420ULL)))

// [VMA: 0x2179b00 - 0x2179b4d] (77 B)
extern "C" void GXCommandPacket96_Destructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
}

// [VMA: 0x2179aa0 - 0x2179af9] (89 B)
extern "C" void GXCommandPacket96_DeletingDestructor(GXRefCountedPacketBase* pThis)
{
    GXRefCountedPacketBase_ReleaseOwner(pThis);
    operator_delete(pThis);
}

// [VMA: 0x2179a70 - 0x2179a9d] (45 B)
extern "C" void GXCommandPacket96_Construct(
    uint8_t* pThis,
    const void* pSrcPayload80,
    GXRefCountedOwner* pOwner)
{
    *reinterpret_cast<void**>(pThis + 0x00) = &g_vtable_GXRefCountedBase;
    *reinterpret_cast<GXRefCountedOwner**>(pThis + 0x08) = pOwner;
    if (pOwner) {
        __sync_fetch_and_add(&pOwner->m_refCount10, 1);
    }
    *reinterpret_cast<void**>(pThis + 0x00) = &g_vtable_GXCommandPacket96;
    memcpy(pThis + 0x10, pSrcPayload80, 0x50);
}

// ============================================================================
// 4. GXBitmapFontAndRichTextVertexGenerator (0x2178c60 .. 0x21798a0)
// ============================================================================

extern "C" const int16_t* sub_2bbec48_ctype_tolower_loc(); // 0x2bbec48

// [VMA: 0x21798a0 - 0x2179a6f] (463 B)
// Decodes the next UTF-16 codepoint (including surrogate pairs) and parses inline
// rich-text color/alpha tags: "##" -> '#', "#c[RRGGBB]" / "#c[RRGGBBAA]", "#a[AA]".
extern "C" uint32_t GXRichText_DecodeNextCodepointAndColorMarkup(
    const uint16_t** ppTextCursor,
    uint32_t* pInOutColorRGBA)
{
    const uint16_t* pCur = *ppTextCursor;
    const uint16_t* pNext = pCur + 1;
    *ppTextCursor = pNext;

    uint32_t codepoint = pCur[0];
    if ((codepoint & 0xF800u) == 0xD800u) {
        uint32_t secondUnit = pCur[1];
        uint32_t highPart   = (codepoint < 0xDC00u) ? codepoint : secondUnit;
        uint32_t lowPart    = (codepoint < 0xDC00u) ? secondUnit : codepoint;
        codepoint = (highPart << 16) | lowPart;
        pNext = pCur + 2;
        *ppTextCursor = pNext;
    }

    if (codepoint != 0x23u) { // '#'
        return codepoint;
    }

    uint16_t tagType = pNext[0];
    if (tagType != 0x63u && tagType != 0x61u) { // 'c' or 'a'
        if (tagType == 0x23u) { // "##" escaped '#'
            *ppTextCursor = pNext + 1;
        }
        return 0x23u;
    }

    if (pNext[1] != 0x5Bu) { // '['
        return 0x23u;
    }

    uint32_t hexLen = 0;
    while (true) {
        if (hexLen > 8u || pNext[2 + hexLen] == 0u) {
            return 0x23u;
        }
        if (pNext[2 + hexLen] == 0x5Du) { // ']'
            break;
        }
        ++hexLen;
    }
    if (hexLen == 0) {
        return 0x23u;
    }

    uint32_t parsedHex = 0;
    for (uint32_t i = 0; i < hexLen; ++i) {
        uint8_t rawCh = static_cast<uint8_t>(pNext[2 + i] & 0xFFu);
        const int8_t* pLowerTable = reinterpret_cast<const int8_t*>(sub_2bbec48_ctype_tolower_loc());
        uint16_t lowerCh = static_cast<uint16_t>(static_cast<int16_t>(pLowerTable[rawCh * 2]));
        uint32_t nibble = 0;
        if (static_cast<uint16_t>(lowerCh - 0x30u) <= 9u) {
            nibble = lowerCh - 0x30u;
        } else if (static_cast<uint16_t>(lowerCh - 0x61u) <= 5u) {
            nibble = lowerCh - 0x57u;
        } else {
            break;
        }
        parsedHex = (parsedHex << 4) | nibble;
    }

    uint32_t r = 0, gByte = 0, b = 0, a = 0;
    if (tagType == 0x61u) { // 'a' -> override alpha only
        uint32_t prevColor = *pInOutColorRGBA;
        r     = prevColor & 0xFFu;
        gByte = prevColor & 0xFF00u;
        b     = (prevColor >> 16) & 0xFFu;
        a     = parsedHex & 0xFFu;
    } else if (tagType == 0x63u) { // 'c' -> override RGB or RGBA
        r     = (parsedHex >> 16) & 0xFFu;
        gByte = parsedHex & 0xFF00u;
        b     = parsedHex & 0xFFu;
        a     = (hexLen == 6u) ? 0xFFu : ((parsedHex >> 24) & 0xFFu);
    }

    *pInOutColorRGBA = (a << 24) | ((b << 16) & 0xFF0000u) | (gByte & 0xFF00u) | (r & 0xFFu);
    *ppTextCursor = pNext + hexLen + 4;
    return pNext[hexLen + 3];
}

struct GXBitmapGlyphMetric {
    float    m_bearingX00;    // +0x00
    float    m_width04;       // +0x04
    float    m_rightBearing08;// +0x08
    uint32_t m_pageIndex0C;   // +0x0C
    float    m_u0_10;         // +0x10
    float    m_v0_14;         // +0x14
    float    m_u1_18;         // +0x18
    float    m_v1_1C;         // +0x1C
};

struct GXBitmapFontResource {
    uint32_t m_reserved00;    // +0x00
    uint32_t m_reserved04;    // +0x04
    uint32_t m_glyphCount08;  // +0x08
    float    m_baseHeight0C;  // +0x0C
    uint32_t m_codepoints10[1]; // +0x10 (m_glyphCount08 elements, followed by GXBitmapGlyphMetric array)
};

struct GXTextStyleDesc {
    float    m_fontHeight00;  // +0x00
    float    m_charSpacing04; // +0x04
    float    m_lineSpacing08; // +0x08
    float    m_aspectScale0C; // +0x0C
    uint8_t  m_bMonospace10;  // +0x10
};

struct GXTextLayoutBoxDesc {
    float    m_minX00;        // +0x00
    float    m_minY04;        // +0x04
    float    m_maxX08;        // +0x08
    float    m_maxY0C;        // +0x0C
    uint32_t m_hAlign10;      // +0x10 (0 = Left, 1 = Center, 2 = Right)
    uint32_t m_vAlign14;      // +0x14 (0 = Top, 1 = Middle, 2 = Bottom)
    uint8_t  m_bWordWrap18;   // +0x18
};

struct GXGlyphVertex {
    float    m_posX00;        // +0x00
    float    m_posY04;        // +0x04
    float    m_u08;           // +0x08
    float    m_v0C;           // +0x0C
    float    m_page10;        // +0x10
    uint32_t m_colorRGBA14;   // +0x14
};

// [VMA: 0x2178e50 - 0x217980a] (2,490 B)
// Generates 4-vertex glyph quads (96 bytes per glyph) and/or computes the 2D bounding box for rich text.
extern "C" void GXBitmapFont_GenerateRichTextGlyphQuads(
    const uint16_t* pText,
    const GXBitmapFontResource* pFont,
    const GXTextStyleDesc* pStyle,
    const GXTextLayoutBoxDesc* pBox,
    GXGlyphVertex* pOutVertices,
    float* pOutBounds4,
    uint16_t fallbackChar)
{
    if (!pText || (!pOutVertices && !pOutBounds4)) {
        return;
    }

    float boxMinX = 0.0f, boxMinY = 0.0f, boxMaxX = 0.0f, boxMaxY = 0.0f;
    uint32_t hAlign = 0, vAlign = 0, bWordWrap = 0;
    if (pBox) {
        boxMinX   = pBox->m_minX00;
        boxMinY   = pBox->m_minY04;
        boxMaxX   = pBox->m_maxX08;
        boxMaxY   = pBox->m_maxY0C;
        hAlign    = pBox->m_hAlign10;
        vAlign    = pBox->m_vAlign14;
        bWordWrap = pBox->m_bWordWrap18 & 1u;
    }

    uint64_t glyphCount = pFont->m_glyphCount08;
    const uint32_t* pCodepoints = &pFont->m_codepoints10[0];
    const GXBitmapGlyphMetric* pMetrics = reinterpret_cast<const GXBitmapGlyphMetric*>(pCodepoints + glyphCount);

    float fontHeight  = pStyle->m_fontHeight00;
    float charSpacing = pStyle->m_charSpacing04;
    float lineStepY   = fontHeight + pStyle->m_lineSpacing08;
    float baseScaleX  = (fontHeight / pFont->m_baseHeight0C) * pStyle->m_aspectScale0C;
    float boxWidth    = boxMaxX - boxMinX;

    GXBitmapGlyphMetric fallbackMetric = {};
    fallbackMetric.m_width04 = fontHeight;
    fallbackMetric.m_pageIndex0C = 0;

    if (fallbackChar != 0) {
        uint32_t searchKey = fallbackChar;
        if ((searchKey & 0xF800u) == 0xD800u && fallbackChar < 0xDC00u) {
            searchKey <<= 16;
        }
        int64_t count = static_cast<int64_t>(glyphCount);
        const uint32_t* pBase = pCodepoints;
        while (count > 0) {
            int64_t half = count / 2;
            if (pBase[half] < searchKey) {
                pBase += half + 1;
                count -= half + 1;
            } else {
                count = half;
            }
        }
        if (pBase != (pCodepoints + glyphCount) && *pBase == searchKey) {
            size_t idx = static_cast<size_t>(pBase - pCodepoints);
            fallbackMetric = pMetrics[idx];
        }
    }

    uint32_t curColorRGBA = 0xFFFFFFFFu;
    const uint16_t* pCursor = pText;
    uint32_t cp = GXRichText_DecodeNextCodepointAndColorMarkup(&pCursor, &curColorRGBA);

    float curX = 0.0f;
    float curY = 0.0f;
    float maxLineX = 0.0f;
    float maxLineY = 0.0f;

    GXGlyphVertex* pWriteVert = pOutVertices;
    GXGlyphVertex* pLineStartVert = pOutVertices;

    while (true) {
        const GXBitmapGlyphMetric* pGlyph = nullptr;
        float glyphAdvanceX = 0.0f;
        float scaleX = baseScaleX;

        if (cp == 0x0Au) { // '\n'
            goto handle_newline;
        }
        if (cp == 0x0Du) { // '\r'
            cp = GXRichText_DecodeNextCodepointAndColorMarkup(&pCursor, &curColorRGBA);
            continue;
        }
        if (cp == 0u) {
            break;
        }

        {
            int64_t count = static_cast<int64_t>(glyphCount);
            const uint32_t* pBase = pCodepoints;
            while (count > 0) {
                int64_t half = count / 2;
                if (pBase[half] < cp) {
                    pBase += half + 1;
                    count -= half + 1;
                } else {
                    count = half;
                }
            }
            if (pBase != (pCodepoints + glyphCount) && *pBase == cp) {
                pGlyph = &pMetrics[pBase - pCodepoints];
            } else {
                pGlyph = &fallbackMetric;
            }

            float rawGlyphSpan = pGlyph->m_bearingX00 + pGlyph->m_width04 + pGlyph->m_rightBearing08;
            if (pStyle->m_bMonospace10 != 0) {
                float monoFactor = 0.5f;
                if ((cp - 0x20u) >= 0x5Fu) {
                    monoFactor = ((cp - 0xFF61u) < 0x3Fu) ? 0.5f : 1.0f;
                }
                scaleX = ((fontHeight * monoFactor) / rawGlyphSpan) * baseScaleX;
            }

            glyphAdvanceX = rawGlyphSpan * scaleX;
            if (bWordWrap != 0 && (curX + glyphAdvanceX) > boxWidth) {
                goto handle_newline;
            }
        }

emit_glyph:
        if (pWriteVert) {
            float xBase = boxMinX + curX;
            float yTop  = boxMinY + curY;
            float yBot  = yTop + fontHeight;
            float xLeft = xBase + (pGlyph->m_bearingX00 * scaleX);
            float xRight = xBase + ((pGlyph->m_bearingX00 + pGlyph->m_width04) * scaleX);
            float pageF = static_cast<float>(pGlyph->m_pageIndex0C);

            pWriteVert[0].m_posX00 = xLeft;
            pWriteVert[0].m_posY04 = yTop;
            pWriteVert[0].m_u08    = pGlyph->m_u0_10;
            pWriteVert[0].m_v0C    = pGlyph->m_v0_14;
            pWriteVert[0].m_page10 = pageF;
            pWriteVert[0].m_colorRGBA14 = curColorRGBA;

            pWriteVert[1].m_posX00 = xRight;
            pWriteVert[1].m_posY04 = yTop;
            pWriteVert[1].m_u08    = pGlyph->m_u1_18;
            pWriteVert[1].m_v0C    = pGlyph->m_v0_14;
            pWriteVert[1].m_page10 = pageF;
            pWriteVert[1].m_colorRGBA14 = curColorRGBA;

            pWriteVert[2].m_posX00 = xRight;
            pWriteVert[2].m_posY04 = yBot;
            pWriteVert[2].m_u08    = pGlyph->m_u1_18;
            pWriteVert[2].m_v0C    = pGlyph->m_v1_1C;
            pWriteVert[2].m_page10 = pageF;
            pWriteVert[2].m_colorRGBA14 = curColorRGBA;

            pWriteVert[3].m_posX00 = xLeft;
            pWriteVert[3].m_posY04 = yBot;
            pWriteVert[3].m_u08    = pGlyph->m_u0_10;
            pWriteVert[3].m_v0C    = pGlyph->m_v1_1C;
            pWriteVert[3].m_page10 = pageF;
            pWriteVert[3].m_colorRGBA14 = curColorRGBA;

            pWriteVert += 4;
        }

        curX += charSpacing + glyphAdvanceX;
        if (curX > maxLineX) {
            maxLineX = curX;
        }
        if (curY > maxLineY) {
            maxLineY = curY;
        }
        cp = GXRichText_DecodeNextCodepointAndColorMarkup(&pCursor, &curColorRGBA);
        continue;

handle_newline:
        if (pWriteVert && pLineStartVert && pLineStartVert < pWriteVert) {
            if (hAlign == 2u) {
                float shiftX = boxWidth - curX;
                for (GXGlyphVertex* pV = pLineStartVert; pV < pWriteVert; pV += 4) {
                    pV[0].m_posX00 += shiftX;
                    pV[1].m_posX00 += shiftX;
                    pV[2].m_posX00 += shiftX;
                    pV[3].m_posX00 += shiftX;
                }
            } else if (hAlign == 1u) {
                float shiftX = (boxWidth - curX) * 0.5f;
                for (GXGlyphVertex* pV = pLineStartVert; pV < pWriteVert; pV += 4) {
                    pV[0].m_posX00 += shiftX;
                    pV[1].m_posX00 += shiftX;
                    pV[2].m_posX00 += shiftX;
                    pV[3].m_posX00 += shiftX;
                }
            }
        }
        curY += lineStepY;
        curX = 0.0f;
        pLineStartVert = pWriteVert;
        if (pGlyph != nullptr) {
            goto emit_glyph;
        }
        cp = GXRichText_DecodeNextCodepointAndColorMarkup(&pCursor, &curColorRGBA);
    }

    // Align the final line horizontally
    if (pWriteVert && pLineStartVert && pLineStartVert < pWriteVert) {
        if (hAlign == 2u) {
            float shiftX = boxWidth - curX;
            for (GXGlyphVertex* pV = pLineStartVert; pV < pWriteVert; pV += 4) {
                pV[0].m_posX00 += shiftX;
                pV[1].m_posX00 += shiftX;
                pV[2].m_posX00 += shiftX;
                pV[3].m_posX00 += shiftX;
            }
        } else if (hAlign == 1u) {
            float shiftX = (boxWidth - curX) * 0.5f;
            for (GXGlyphVertex* pV = pLineStartVert; pV < pWriteVert; pV += 4) {
                pV[0].m_posX00 += shiftX;
                pV[1].m_posX00 += shiftX;
                pV[2].m_posX00 += shiftX;
                pV[3].m_posX00 += shiftX;
            }
        }
    }

    float totalHeight = lineStepY + maxLineY;
    uint32_t totalVertices = static_cast<uint32_t>(pWriteVert - pOutVertices);
    uint32_t totalQuads    = totalVertices / 4u;

    float outMinX = boxMinX;
    float outMaxX = boxMaxX;
    if (hAlign == 2u) {
        outMinX = boxMaxX - maxLineX;
        outMaxX = boxMaxX;
    } else if (hAlign == 1u) {
        float halfPadX = (boxWidth - maxLineX) * 0.5f;
        outMinX = boxMinX + halfPadX;
        outMaxX = boxMaxX - halfPadX;
    } else if (hAlign == 0u) {
        outMaxX = boxMinX + maxLineX;
    } else {
        outMinX = 0.0f;
        outMaxX = maxLineX;
    }

    float outMinY = boxMinY;
    float outMaxY = boxMaxY;
    if (vAlign == 1u) {
        float shiftY = ((boxMaxY - boxMinY) - totalHeight) * 0.5f;
        if (pOutVertices && totalQuads != 0) {
            for (uint32_t q = 0; q < totalQuads; ++q) {
                GXGlyphVertex* pV = pOutVertices + (q * 4);
                pV[0].m_posY04 += shiftY;
                pV[1].m_posY04 += shiftY;
                pV[2].m_posY04 += shiftY;
                pV[3].m_posY04 += shiftY;
            }
        }
        outMinY = boxMinY + shiftY;
        outMaxY = boxMaxY - shiftY;
    } else if (vAlign == 2u) {
        if (pOutVertices && totalQuads != 0) {
            float shiftY = (boxMaxY - boxMinY) - totalHeight;
            for (uint32_t q = 0; q < totalQuads; ++q) {
                GXGlyphVertex* pV = pOutVertices + (q * 4);
                pV[0].m_posY04 += shiftY;
                pV[1].m_posY04 += shiftY;
                pV[2].m_posY04 += shiftY;
                pV[3].m_posY04 += shiftY;
            }
        }
        outMinY = boxMaxY - totalHeight;
        outMaxY = boxMaxY;
    } else if (vAlign == 0u) {
        outMaxY = boxMinY + totalHeight;
    } else {
        outMinY = 0.0f;
        outMaxY = totalHeight;
    }

    if (pOutBounds4) {
        pOutBounds4[0] = outMinX;
        pOutBounds4[1] = outMinY;
        pOutBounds4[2] = outMaxX;
        pOutBounds4[3] = outMaxY;
    }
}

// [VMA: 0x2178db0 - 0x2178e42] (146 B)
// Counts the number of printable glyph vertices (4 vertices per non-CR/LF codepoint) in a rich-text string.
extern "C" uint32_t GXBitmapFont_CountPrintableGlyphVertices(const uint16_t* pText)
{
    if (!pText) {
        return 0;
    }
    const uint16_t* pCursor = pText;
    uint32_t dummyColor = 0;
    uint32_t glyphCount = 0;

    uint32_t cp = GXRichText_DecodeNextCodepointAndColorMarkup(&pCursor, &dummyColor);
    while (cp != 0) {
        if (cp != 0x0Au && cp != 0x0Du) {
            ++glyphCount;
        }
        cp = GXRichText_DecodeNextCodepointAndColorMarkup(&pCursor, &dummyColor);
    }
    return glyphCount << 2;
}

// [VMA: 0x2178c60 - 0x2178daf] (335 B)
// Allocates and initializes a GXBitmapFontResource from a binary font table.
extern "C" int GXBitmapFont_CreateFromBinaryResource(
    GXBitmapFontResource** ppOutFont,
    const uint8_t* pBinData,
    IGXAllocator* pAllocator)
{
    uint32_t glyphCount = *reinterpret_cast<const uint32_t*>(pBinData + 0x10);
    size_t allocSize = static_cast<size_t>(glyphCount * 0x24u + 0x10u);

    GXBitmapFontResource* pFont = static_cast<GXBitmapFontResource*>(
        pAllocator->vptr->Allocate(pAllocator, allocSize));
    if (!pFont) {
        return 9;
    }

    memset(pFont, 0, allocSize);
    pFont->m_reserved00 = 0;
    pFont->m_reserved04 = 0;
    pFont->m_glyphCount08 = glyphCount;
    pFont->m_baseHeight0C = static_cast<float>(*reinterpret_cast<const int16_t*>(pBinData + 0x08));

    uint32_t tableOffset = *reinterpret_cast<const uint32_t*>(pBinData + 0x18);
    if (glyphCount != 0) {
        float invTexW = static_cast<float>(*reinterpret_cast<const int16_t*>(pBinData + 0x0A));
        float invTexH = static_cast<float>(*reinterpret_cast<const int16_t*>(pBinData + 0x0C));

        uint32_t* pOutCodepoints = &pFont->m_codepoints10[0];
        GXBitmapGlyphMetric* pOutMetrics = reinterpret_cast<GXBitmapGlyphMetric*>(pOutCodepoints + glyphCount);
        const uint8_t* pSrcRec = pBinData + tableOffset;

        for (uint32_t i = 0; i < glyphCount; ++i) {
            pOutCodepoints[i] = *reinterpret_cast<const uint32_t*>(pSrcRec + 0x00);

            int16_t bearingX = *reinterpret_cast<const int16_t*>(pSrcRec + 0x0A);
            int16_t width    = *reinterpret_cast<const int16_t*>(pSrcRec + 0x0C);
            int16_t advanceX = *reinterpret_cast<const int16_t*>(pSrcRec + 0x0E);
            uint16_t pageIdx = *reinterpret_cast<const uint16_t*>(pSrcRec + 0x08);
            uint32_t uvOff   = *reinterpret_cast<const uint32_t*>(pSrcRec + 0x04);

            pOutMetrics[i].m_bearingX00     = static_cast<float>(bearingX);
            pOutMetrics[i].m_width04        = static_cast<float>(width);
            pOutMetrics[i].m_rightBearing08 = static_cast<float>(advanceX - width);
            pOutMetrics[i].m_pageIndex0C    = pageIdx;

            const int16_t* pUvRect = reinterpret_cast<const int16_t*>(pBinData + uvOff);
            pOutMetrics[i].m_u0_10 = static_cast<float>(pUvRect[0]) / invTexW;
            pOutMetrics[i].m_v0_14 = static_cast<float>(pUvRect[1]) / invTexH;
            pOutMetrics[i].m_u1_18 = static_cast<float>(pUvRect[2]) / invTexW;
            pOutMetrics[i].m_v1_1C = static_cast<float>(pUvRect[3]) / invTexH;

            pSrcRec += 0x18;
        }
    }

    *ppOutFont = pFont;
    return 0;
}

// ============================================================================
// 5. GXGnmSamplerDepthStencilBlendAndShaderHardwareState (0x2177d20 .. 0x2178ab0)
// ============================================================================

// External GNM hardware state SDK & translation helpers
extern "C" void     sceGnmSamplerInit(void* pSampler); // 0x1086dd0
extern "C" void     sceGnmSamplerSetWrapMode(void* pSampler, uint32_t wrapX, uint32_t wrapY, uint32_t wrapZ); // 0x1086ef0
extern "C" void     sceGnmSamplerSetXyFilterMode(void* pSampler, uint32_t magFilter, uint32_t minFilter); // 0x1086f40
extern "C" void     sceGnmSamplerSetMipFilterMode(void* pSampler, uint32_t mipFilter); // 0x1086e10
extern "C" void     sceGnmSamplerSetZFilterMode(void* pSampler, uint32_t zFilter); // 0x1086df0
extern "C" void     sceGnmSamplerSetAnisotropyRatio(void* pSampler, uint32_t anisoRatio); // 0x1086f60
extern "C" void     sceGnmSamplerSetBorderColor(void* pSampler, uint32_t borderColorType); // 0x1086f80
extern "C" void     sceGnmSamplerSetDepthCompareFunction(void* pSampler, uint32_t depthCompare); // 0x1086fc0
extern "C" void     sceGnmSamplerSetLodBias(void* pSampler, uint32_t lodBiasFixed14, uint32_t secBias); // 0x1087020
extern "C" void     sceGnmSamplerSetLodRange(void* pSampler, uint32_t minLodFixed12, uint32_t maxLodFixed12); // 0x1086e30
extern "C" void     sceGnmSamplerSetAnisotropyThreshold(void* pSampler, uint32_t thresh); // 0x1087000
extern "C" void     sceGnmSamplerSetPerfMip(void* pSampler, uint32_t perfMip); // 0x1086e50
extern "C" void     sceGnmSamplerSetPerfZ(void* pSampler, uint32_t perfZ); // 0x1086e70
extern "C" uint32_t sceGnmSamplerGetMinFilterMode(const void* pSampler); // 0x1087070
extern "C" uint32_t sceGnmSamplerGetMagFilterMode(const void* pSampler); // 0x1087060
extern "C" uint32_t sceGnmSamplerGetMipFilterMode(const void* pSampler); // 0x1087090
extern "C" uint32_t sceGnmSamplerGetWrapModeX(const void* pSampler); // 0x1086f10
extern "C" uint32_t sceGnmSamplerGetWrapModeY(const void* pSampler); // 0x1086f20
extern "C" uint32_t sceGnmSamplerGetWrapModeZ(const void* pSampler); // 0x1086f30
extern "C" uint32_t sceGnmSamplerGetDepthCompareFunction(const void* pSampler); // 0x10870f0

extern "C" uint32_t GXTranslate_GnmFilterToEngine(uint32_t minF, uint32_t magF, uint32_t mipF); // 0x26b8100
extern "C" uint32_t GXTranslate_GnmWrapToEngine(uint32_t gnmWrap); // 0x26b81b0
extern "C" uint32_t GXTranslate_GnmCompareToEngine(uint32_t gnmCmp); // 0x26b8080
extern "C" uint32_t GXTranslate_EngineWrapToGnm(uint32_t engWrap); // 0x26b7d00
extern "C" uint32_t GXTranslate_EngineMagFilterToGnm(uint32_t engFilter); // 0x26b7c80
extern "C" uint32_t GXTranslate_EngineMinFilterToGnm(uint32_t engFilter); // 0x26b7c60
extern "C" uint32_t GXTranslate_EngineMipFilterToGnm(uint32_t engFilter); // 0x26b7ca0
extern "C" uint32_t GXTranslate_EngineZFilterToGnm(uint32_t engFilter); // 0x26b7cc0
extern "C" uint32_t GXTranslate_EngineAnisoToGnm(uint32_t maxAniso); // 0x26b7d20
extern "C" uint32_t GXTranslate_EngineDepthCompareToGnm(uint32_t engFilter, uint32_t cmpFunc); // 0x26b7ce0

// [VMA: 0x2178ab0 - 0x2178bcb] (283 B)
// Extracts an engine 56-byte Sampler descriptor from a compiled GNM Sampler state object.
extern "C" void GXGnmSamplerState_ExtractDesc(const uint8_t* pSamplerObj, uint8_t* pOutSamplerDesc)
{
    uint32_t minF = sceGnmSamplerGetMinFilterMode(pSamplerObj);
    uint32_t magF = sceGnmSamplerGetMagFilterMode(pSamplerObj);
    uint32_t mipF = sceGnmSamplerGetMipFilterMode(pSamplerObj);
    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x00) = GXTranslate_GnmFilterToEngine(minF, magF, mipF);

    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x04) = GXTranslate_GnmWrapToEngine(sceGnmSamplerGetWrapModeX(pSamplerObj));
    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x08) = GXTranslate_GnmWrapToEngine(sceGnmSamplerGetWrapModeY(pSamplerObj));
    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x0C) = GXTranslate_GnmWrapToEngine(sceGnmSamplerGetWrapModeZ(pSamplerObj));

    *reinterpret_cast<float*>(pOutSamplerDesc + 0x10)    = *reinterpret_cast<const float*>(pSamplerObj + 0x10);
    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x14) = *reinterpret_cast<const uint32_t*>(pSamplerObj + 0x14);
    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x18) = GXTranslate_GnmCompareToEngine(sceGnmSamplerGetDepthCompareFunction(pSamplerObj));

    *reinterpret_cast<float*>(pOutSamplerDesc + 0x1C) = *reinterpret_cast<const float*>(pSamplerObj + 0x18);
    *reinterpret_cast<float*>(pOutSamplerDesc + 0x20) = *reinterpret_cast<const float*>(pSamplerObj + 0x1C);
    *reinterpret_cast<float*>(pOutSamplerDesc + 0x24) = *reinterpret_cast<const float*>(pSamplerObj + 0x20);
    *reinterpret_cast<float*>(pOutSamplerDesc + 0x28) = *reinterpret_cast<const float*>(pSamplerObj + 0x24);
    *reinterpret_cast<float*>(pOutSamplerDesc + 0x2C) = *reinterpret_cast<const float*>(pSamplerObj + 0x28);
    *reinterpret_cast<float*>(pOutSamplerDesc + 0x30) = *reinterpret_cast<const float*>(pSamplerObj + 0x2C);

    uint16_t packedFlags = *reinterpret_cast<const uint16_t*>(pSamplerObj + 0x30);
    uint32_t curFlags34  = *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x34);
    curFlags34 = (curFlags34 & ~0x7FFu) | (packedFlags & 0x7FFu);
    *reinterpret_cast<uint32_t*>(pOutSamplerDesc + 0x34) = curFlags34;
}

// [VMA: 0x2178aa0 - 0x2178aa1] (1 B)
extern "C" void GXGnmSamplerState_Release() {}

// [VMA: 0x2178750 - 0x2178a99] (841 B)
// Initializes a GNM Sampler state object (+0x00..+0x34) from an engine Sampler descriptor.
extern "C" int GXGnmSamplerState_Initialize(uint8_t* pOutSamplerObj, void* /*pUnused*/, const uint8_t* pSamplerDesc)
{
    sceGnmSamplerInit(pOutSamplerObj);

    uint32_t wrapX = GXTranslate_EngineWrapToGnm(*reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x04));
    uint32_t wrapY = GXTranslate_EngineWrapToGnm(*reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x08));
    uint32_t wrapZ = GXTranslate_EngineWrapToGnm(*reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x0C));
    sceGnmSamplerSetWrapMode(pOutSamplerObj, wrapX, wrapY, wrapZ);

    uint32_t filterMode = *reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x00);
    uint32_t magF = GXTranslate_EngineMagFilterToGnm(filterMode);
    uint32_t minF = GXTranslate_EngineMinFilterToGnm(filterMode);
    sceGnmSamplerSetXyFilterMode(pOutSamplerObj, magF, minF);
    sceGnmSamplerSetMipFilterMode(pOutSamplerObj, GXTranslate_EngineMipFilterToGnm(filterMode));
    sceGnmSamplerSetZFilterMode(pOutSamplerObj, GXTranslate_EngineZFilterToGnm(filterMode));
    sceGnmSamplerSetAnisotropyRatio(pOutSamplerObj, GXTranslate_EngineAnisoToGnm(*reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x14)));

    float borderR = *reinterpret_cast<const float*>(pSamplerDesc + 0x1C);
    float borderG = *reinterpret_cast<const float*>(pSamplerDesc + 0x20);
    float borderB = *reinterpret_cast<const float*>(pSamplerDesc + 0x24);
    float borderA = *reinterpret_cast<const float*>(pSamplerDesc + 0x28);

    uint32_t borderColorType = 0;
    if (borderR == 0.0f && borderG == 0.0f && borderB == 0.0f && borderA == 1.0f) {
        borderColorType = 1u; // Opaque black
    } else if (borderR == 1.0f && borderG == 1.0f && borderB == 1.0f && borderA == 1.0f) {
        borderColorType = 2u; // Opaque white
    }
    sceGnmSamplerSetBorderColor(pOutSamplerObj, borderColorType);

    uint32_t cmpFunc = GXTranslate_EngineDepthCompareToGnm(
        filterMode, *reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x18));
    sceGnmSamplerSetDepthCompareFunction(pOutSamplerObj, cmpFunc);

    // Convert float LOD bias (+0x10) to 14-bit signed fixed-point (S5.8: * 256.0f)
    float lodBias = *reinterpret_cast<const float*>(pSamplerDesc + 0x10);
    uint32_t biasBits = 0;
    memcpy(&biasBits, &lodBias, sizeof(uint32_t));
    if ((biasBits & 0x7F800000u) == 0x7F800000u && (biasBits & 0x007FFFFFu) != 0) {
        lodBias = 0.0f;
    } else if (lodBias > 31.99609375f) {
        lodBias = 31.99609375f;
    }

    memcpy(&biasBits, &lodBias, sizeof(uint32_t));
    uint32_t lodBiasFixed14 = 0;
    if ((biasBits & 0x7F800000u) == 0x7F800000u && (biasBits & 0x007FFFFFu) != 0) {
        lodBiasFixed14 = 0;
    } else {
        int32_t fixedSigned = -8192;
        if (lodBias >= -32.0f) {
            fixedSigned = static_cast<int32_t>(lodBias * 256.0f);
            if (fixedSigned < -8192) {
                lodBiasFixed14 = 0x2000u;
                goto set_lod_bias;
            }
        }
        lodBiasFixed14 = (fixedSigned > 0x1FFF) ? 0x1FFFu : static_cast<uint32_t>(fixedSigned & 0x3FFF);
    }
set_lod_bias:
    sceGnmSamplerSetLodBias(pOutSamplerObj, lodBiasFixed14, 0);

    // Convert minLod (+0x2C) and maxLod (+0x30) to 12-bit unsigned fixed-point (U4.8: * 256.0f, 0..0xFFF)
    float minLod = *reinterpret_cast<const float*>(pSamplerDesc + 0x2C);
    uint32_t minLodBits = 0;
    memcpy(&minLodBits, &minLod, sizeof(uint32_t));
    bool minLodIsNaN = ((minLodBits & 0x7F800000u) == 0x7F800000u) && ((minLodBits & 0x007FFFFFu) != 0);
    uint32_t minLodFixed = 0;
    if (!minLodIsNaN && minLod >= 0.0f) {
        minLodFixed = static_cast<uint32_t>(static_cast<int64_t>(minLod * 256.0f));
    }
    if (minLodFixed > 0xFFFu) {
        minLodFixed = 0xFFFu;
    }

    float maxLod = *reinterpret_cast<const float*>(pSamplerDesc + 0x30);
    uint32_t maxLodBits = 0;
    memcpy(&maxLodBits, &maxLod, sizeof(uint32_t));
    bool maxLodIsNaN = ((maxLodBits & 0x7F800000u) == 0x7F800000u) && ((maxLodBits & 0x007FFFFFu) != 0);
    uint32_t maxLodFixed = 0xFFFu;
    if (!maxLodIsNaN && maxLod <= 15.99609375f) {
        if (maxLod < 0.0f) {
            maxLodFixed = 0;
            goto set_lod_range;
        }
        uint32_t conv = static_cast<uint32_t>(static_cast<int64_t>(maxLod * 256.0f));
        maxLodFixed = (conv <= 0xFFFu) ? conv : 0xFFFu;
    }
set_lod_range:
    sceGnmSamplerSetLodRange(pOutSamplerObj, minLodFixed, maxLodFixed);

    uint32_t extFlags = *reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x34);
    sceGnmSamplerSetAnisotropyThreshold(pOutSamplerObj, extFlags & 0x7u);
    sceGnmSamplerSetPerfMip(pOutSamplerObj, (extFlags >> 3) & 0xFu);
    sceGnmSamplerSetPerfZ(pOutSamplerObj, (extFlags >> 7) & 0xFu);
    gx_direct_vk::RegisterDirectNativeSampler(pOutSamplerObj);

    *reinterpret_cast<float*>(pOutSamplerObj + 0x10)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x10);
    *reinterpret_cast<uint32_t*>(pOutSamplerObj + 0x14) = *reinterpret_cast<const uint32_t*>(pSamplerDesc + 0x14);
    *reinterpret_cast<float*>(pOutSamplerObj + 0x18)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x1C);
    *reinterpret_cast<float*>(pOutSamplerObj + 0x1C)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x20);
    *reinterpret_cast<float*>(pOutSamplerObj + 0x20)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x24);
    *reinterpret_cast<float*>(pOutSamplerObj + 0x24)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x28);
    *reinterpret_cast<float*>(pOutSamplerObj + 0x28)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x2C);
    *reinterpret_cast<float*>(pOutSamplerObj + 0x2C)    = *reinterpret_cast<const float*>(pSamplerDesc + 0x30);
    *reinterpret_cast<uint32_t*>(pOutSamplerObj + 0x30) = 0;
    *reinterpret_cast<uint16_t*>(pOutSamplerObj + 0x30) = static_cast<uint16_t>(extFlags & 0x7FFu);
    return 0;
}

// External GNM PrimitiveSetup / ScanMode helpers
extern "C" void     sceGnmScanModeControlInit(void* pScanMode); // 0x10850d0
extern "C" void     sceGnmScanModeControlSetMsaa(void* pScanMode, uint32_t enable); // 0x1085170
extern "C" void     sceGnmScanModeControlSetViewportScissor(void* pScanMode, uint32_t enable, uint32_t enable2); // 0x10851f0
extern "C" void     sceGnmPrimitiveSetupInit(void* pPrimSetup); // 0x1084f40
extern "C" void     sceGnmPrimitiveSetupSetCullFace(void* pPrimSetup, uint32_t cullMode); // 0x1084f50
extern "C" void     sceGnmPrimitiveSetupSetFrontFace(void* pPrimSetup, uint32_t frontFace); // 0x1084f60
extern "C" void     sceGnmPrimitiveSetupSetPolygonMode(void* pPrimSetup, uint32_t frontMode, uint32_t backMode); // 0x1084f70
extern "C" void     sceGnmPrimitiveSetupSetPolygonOffsetEnable(void* pPrimSetup, uint32_t frontEn, uint32_t backEn); // 0x1084fa0
extern "C" uint32_t GXTranslate_EngineCullModeToGnm(uint32_t cullMode); // 0x26b7c50
extern "C" uint32_t GXTranslate_EngineFillModeToGnm(uint32_t fillMode); // 0x26b7c40

// [VMA: 0x2178670 - 0x21786b8] (72 B)
// Extracts an engine 28-byte Rasterizer descriptor from a compiled GNM Rasterizer state object.
extern "C" void GXGnmRasterizerState_ExtractDesc(const uint8_t* pRasterObj, uint8_t* pOutRasterDesc)
{
    *reinterpret_cast<uint32_t*>(pOutRasterDesc + 0x00) = *reinterpret_cast<const uint32_t*>(pRasterObj + 0x18);
    *reinterpret_cast<uint32_t*>(pOutRasterDesc + 0x04) = *reinterpret_cast<const uint32_t*>(pRasterObj + 0x1C);
    pOutRasterDesc[0x08] = pRasterObj[0x20];
    *reinterpret_cast<int32_t*>(pOutRasterDesc + 0x0C)  = static_cast<int32_t>(*reinterpret_cast<const float*>(pRasterObj + 0x08));
    *reinterpret_cast<float*>(pOutRasterDesc + 0x10)    = *reinterpret_cast<const float*>(pRasterObj + 0x0C);
    *reinterpret_cast<float*>(pOutRasterDesc + 0x14)    = *reinterpret_cast<const float*>(pRasterObj + 0x10);
    pOutRasterDesc[0x18] = pRasterObj[0x21];
    pOutRasterDesc[0x19] = (pRasterObj[0x15] != 0) ? 1u : 0u;
    pOutRasterDesc[0x1A] = (pRasterObj[0x14] != 0) ? 1u : 0u;
    pOutRasterDesc[0x1B] = 0;
}

// [VMA: 0x2178660 - 0x2178661] (1 B)
extern "C" void GXGnmRasterizerState_Release() {}

// [VMA: 0x2178550 - 0x2178654] (260 B)
// Initializes a GNM ScanModeControl + PrimitiveSetup state object (+0x00..+0x22) from an engine Rasterizer descriptor.
extern "C" int GXGnmRasterizerState_Initialize(uint8_t* pOutRasterObj, void* /*pUnused*/, const uint8_t* pRasterDesc)
{
    sceGnmScanModeControlInit(pOutRasterObj);
    sceGnmScanModeControlSetMsaa(pOutRasterObj, 1u);
    uint32_t scissorDisableInv = static_cast<uint32_t>(pRasterDesc[0x18]) ^ 1u;
    sceGnmScanModeControlSetViewportScissor(pOutRasterObj, scissorDisableInv, scissorDisableInv);

    void* pPrimSetup = pOutRasterObj + 0x04;
    sceGnmPrimitiveSetupInit(pPrimSetup);
    sceGnmPrimitiveSetupSetCullFace(pPrimSetup, GXTranslate_EngineCullModeToGnm(*reinterpret_cast<const uint32_t*>(pRasterDesc + 0x04)));
    sceGnmPrimitiveSetupSetFrontFace(pPrimSetup, static_cast<uint32_t>(pRasterDesc[0x08]) ^ 1u);

    uint32_t fillMode = *reinterpret_cast<const uint32_t*>(pRasterDesc + 0x00);
    uint32_t gnmFillFront = GXTranslate_EngineFillModeToGnm(fillMode);
    uint32_t gnmFillBack  = GXTranslate_EngineFillModeToGnm(fillMode);
    sceGnmPrimitiveSetupSetPolygonMode(pPrimSetup, gnmFillFront, gnmFillBack);

    int32_t depthBiasInt = *reinterpret_cast<const int32_t*>(pRasterDesc + 0x0C);
    float slopeScaledBias = *reinterpret_cast<const float*>(pRasterDesc + 0x14);
    uint32_t polyOffsetEn = (depthBiasInt != 0 || slopeScaledBias != 0.0f) ? 1u : 0u;
    sceGnmPrimitiveSetupSetPolygonOffsetEnable(pPrimSetup, polyOffsetEn, polyOffsetEn);

    *reinterpret_cast<float*>(pOutRasterObj + 0x08) = static_cast<float>(depthBiasInt);
    *reinterpret_cast<float*>(pOutRasterObj + 0x0C) = *reinterpret_cast<const float*>(pRasterDesc + 0x10);
    *reinterpret_cast<float*>(pOutRasterObj + 0x10) = slopeScaledBias;
    pOutRasterObj[0x14] = pRasterDesc[0x1A];
    pOutRasterObj[0x15] = pRasterDesc[0x19];
    pOutRasterObj[0x16] = 0;
    pOutRasterObj[0x17] = 0;
    *reinterpret_cast<uint32_t*>(pOutRasterObj + 0x18) = fillMode;
    *reinterpret_cast<uint32_t*>(pOutRasterObj + 0x1C) = *reinterpret_cast<const uint32_t*>(pRasterDesc + 0x04);
    pOutRasterObj[0x20] = pRasterDesc[0x08];
    pOutRasterObj[0x21] = pRasterDesc[0x18];
    return 0;
}

// External GNM DepthStencilControl + StencilOpControl helpers
extern "C" void     sceGnmDepthStencilControlInit(void* pDsCtrl); // 0x1085380
extern "C" void     sceGnmDepthStencilControlSetDepthControl(void* pDsCtrl, uint32_t depthWrite, uint32_t zFunc); // 0x1085390
extern "C" void     sceGnmDepthStencilControlSetStencilFunction(void* pDsCtrl, uint32_t stencilFunc); // 0x10853b0
extern "C" void     sceGnmDepthStencilControlSetStencilFunctionBack(void* pDsCtrl, uint32_t stencilFuncBack); // 0x10853d0
extern "C" void     sceGnmDepthStencilControlSetSeparateStencilEnable(void* pDsCtrl, uint32_t enable); // 0x1085410
extern "C" void     sceGnmDepthStencilControlSetDepthEnable(void* pDsCtrl, uint32_t enable); // 0x10853f0
extern "C" void     sceGnmDepthStencilControlSetStencilEnable(void* pDsCtrl, uint32_t enable); // 0x1085400
extern "C" void     sceGnmDepthStencilControlSetDepthBoundsEnable(void* pDsCtrl, uint32_t enable); // 0x1085430
extern "C" void     sceGnmStencilOpControlInit(void* pStencilOpCtrl); // 0x10854b0
extern "C" void     sceGnmStencilOpControlSetStencilOps(void* pStencilOpCtrl, uint32_t failOp, uint32_t passOp, uint32_t zFailOp); // 0x10854c0
extern "C" void     sceGnmStencilOpControlSetStencilOpsBack(void* pStencilOpCtrl, uint32_t failOp, uint32_t passOp, uint32_t zFailOp); // 0x10854e0
extern "C" uint32_t GXTranslate_EngineDepthWriteMaskToGnm(uint32_t writeMask); // 0x26b7bf0
extern "C" uint32_t GXTranslate_EngineCompareFuncToGnm(uint32_t cmpFunc); // 0x26b7c00
extern "C" uint32_t GXTranslate_EngineStencilOpToGnm(uint32_t stencilOp); // 0x26b7c20

// [VMA: 0x2178460 - 0x21784ba] (90 B)
// Extracts an engine 52-byte DepthStencil descriptor from a compiled GNM DepthStencil state object.
extern "C" void GXGnmDepthStencilState_ExtractDesc(const uint8_t* pDepthStencilObj, uint8_t* pOutDesc)
{
    pOutDesc[0x00] = pDepthStencilObj[0x0C];
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x04) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x10);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x08) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x14);
    pOutDesc[0x0C] = pDepthStencilObj[0x0D];
    pOutDesc[0x0D] = pDepthStencilObj[0x09];
    pOutDesc[0x0E] = pDepthStencilObj[0x0A];
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x10) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x1C);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x14) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x1C);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x18) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x20);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x1C) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x24);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x20) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x2C);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x24) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x2C);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x28) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x30);
    *reinterpret_cast<uint32_t*>(pOutDesc + 0x2C) = *reinterpret_cast<const uint32_t*>(pDepthStencilObj + 0x34);
    pOutDesc[0x30] = pDepthStencilObj[0x0E];
}

// [VMA: 0x2178450 - 0x2178451] (1 B)
extern "C" void GXGnmDepthStencilState_Release() {}

// [VMA: 0x21782b0 - 0x2178450] (416 B)
// Initializes a GNM DepthStencilControl + StencilOpControl state object (+0x00..+0x38) from an engine descriptor.
extern "C" int GXGnmDepthStencilState_Initialize(uint8_t* pOutDsObj, void* /*pUnused*/, const uint8_t* pDsDesc)
{
    sceGnmDepthStencilControlInit(pOutDsObj);

    uint32_t zWrite = GXTranslate_EngineDepthWriteMaskToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x04));
    uint32_t zFunc  = GXTranslate_EngineCompareFuncToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x08));
    sceGnmDepthStencilControlSetDepthControl(pOutDsObj, zWrite, zFunc);

    sceGnmDepthStencilControlSetStencilFunction(
        pOutDsObj, GXTranslate_EngineCompareFuncToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x1C)));
    sceGnmDepthStencilControlSetStencilFunctionBack(
        pOutDsObj, GXTranslate_EngineCompareFuncToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x2C)));

    bool bSeparateStencil = (memcmp(pDsDesc + 0x10, pDsDesc + 0x20, 0x10) != 0);
    sceGnmDepthStencilControlSetSeparateStencilEnable(pOutDsObj, bSeparateStencil ? 1u : 0u);
    sceGnmDepthStencilControlSetDepthEnable(pOutDsObj, pDsDesc[0x00]);
    sceGnmDepthStencilControlSetStencilEnable(pOutDsObj, pDsDesc[0x0C]);
    sceGnmDepthStencilControlSetDepthBoundsEnable(pOutDsObj, pDsDesc[0x30]);

    void* pStencilOp = pOutDsObj + 0x04;
    sceGnmStencilOpControlInit(pStencilOp);
    uint32_t fFail  = GXTranslate_EngineStencilOpToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x10));
    uint32_t fPass  = GXTranslate_EngineStencilOpToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x18));
    uint32_t fZFail = GXTranslate_EngineStencilOpToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x14));
    sceGnmStencilOpControlSetStencilOps(pStencilOp, fFail, fPass, fZFail);

    uint32_t bFail  = GXTranslate_EngineStencilOpToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x20));
    uint32_t bPass  = GXTranslate_EngineStencilOpToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x28));
    uint32_t bZFail = GXTranslate_EngineStencilOpToGnm(*reinterpret_cast<const uint32_t*>(pDsDesc + 0x24));
    sceGnmStencilOpControlSetStencilOpsBack(pStencilOp, bFail, bPass, bZFail);

    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x08) = 0;
    pOutDsObj[0x08] = 0;
    pOutDsObj[0x09] = pDsDesc[0x0D];
    pOutDsObj[0x0A] = pDsDesc[0x0E];
    pOutDsObj[0x0B] = 0;
    pOutDsObj[0x0C] = pDsDesc[0x00];
    pOutDsObj[0x0D] = pDsDesc[0x0C];
    pOutDsObj[0x0E] = pDsDesc[0x30];
    pOutDsObj[0x0F] = 0;

    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x10) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x04);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x14) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x08);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x18) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x10);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x1C) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x14);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x20) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x18);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x24) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x1C);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x28) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x20);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x2C) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x24);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x30) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x28);
    *reinterpret_cast<uint32_t*>(pOutDsObj + 0x34) = *reinterpret_cast<const uint32_t*>(pDsDesc + 0x2C);
    return 0;
}

// External GNM BlendControl helpers
extern "C" void     sceGnmBlendControlInit(void* pBlendCtrl); // 0x1084ea0
extern "C" void     sceGnmBlendControlSetBlendEnable(void* pBlendCtrl, uint32_t enable); // 0x1084eb0
extern "C" void     sceGnmBlendControlSetColorEquation(void* pBlendCtrl, uint32_t srcMul, uint32_t func, uint32_t dstMul); // 0x1084ed0
extern "C" void     sceGnmBlendControlSetSeparateAlphaEnable(void* pBlendCtrl, uint32_t enable); // 0x1084f20
extern "C" void     sceGnmBlendControlSetAlphaEquation(void* pBlendCtrl, uint32_t srcMul, uint32_t func, uint32_t dstMul); // 0x1084ef0
extern "C" uint32_t GXTranslate_EngineBlendFactorToGnm(uint32_t blendFactor); // 0x26b7ba0
extern "C" uint32_t GXTranslate_EngineBlendOpToGnm(uint32_t blendOp); // 0x26b7bc0
extern "C" uint32_t GXTranslate_EngineRenderTargetWriteMaskToGnm(uint32_t writeMask, uint32_t rtIndex); // 0x26b7be0

// [VMA: 0x2178210 - 0x2178211] (1 B)
extern "C" void GXGnmBlendState_GetDescStub() {}

// [VMA: 0x2178050 - 0x2178201] (433 B)
// Initializes a GNM BlendControl array (up to 8 render targets) and CB_TARGET_MASK (+0x00) from an engine Blend descriptor.
extern "C" int GXGnmBlendState_Initialize(uint8_t* pOutBlendObj, void* /*pUnused*/, const uint8_t* pBlendDesc)
{
    memcpy(pOutBlendObj + 0x24, pBlendDesc, 0x104);
    *reinterpret_cast<uint32_t*>(pOutBlendObj + 0x00) = 0;

    uint32_t numTargets = (pBlendDesc[0x01] != 0) ? 8u : 1u;
    const uint8_t* pRtDesc = pBlendDesc + 0x04;
    uint32_t* pOutBlendCtrl = reinterpret_cast<uint32_t*>(pOutBlendObj + 0x04);

    for (uint32_t rtIdx = 0; rtIdx < numTargets; ++rtIdx) {
        uint32_t srcBlend       = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x04);
        uint32_t dstBlend       = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x08);
        uint32_t blendOp        = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x0C);
        uint32_t srcBlendAlpha  = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x10);
        uint32_t dstBlendAlpha  = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x14);
        uint32_t blendOpAlpha   = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x18);
        uint32_t writeMask      = *reinterpret_cast<const uint32_t*>(pRtDesc + 0x1C);
        uint8_t  bBlendEnable   = pRtDesc[0x00];

        bool bSeparateAlpha = (srcBlend != srcBlendAlpha) || (blendOp != blendOpAlpha) || (dstBlend != dstBlendAlpha);

        sceGnmBlendControlInit(pOutBlendCtrl);
        sceGnmBlendControlSetBlendEnable(pOutBlendCtrl, bBlendEnable);
        if (bBlendEnable != 0) {
            uint32_t gnmSrcC = GXTranslate_EngineBlendFactorToGnm(srcBlend);
            uint32_t gnmOpC  = GXTranslate_EngineBlendOpToGnm(blendOp);
            uint32_t gnmDstC = GXTranslate_EngineBlendFactorToGnm(dstBlend);
            sceGnmBlendControlSetColorEquation(pOutBlendCtrl, gnmSrcC, gnmOpC, gnmDstC);

            sceGnmBlendControlSetSeparateAlphaEnable(pOutBlendCtrl, bSeparateAlpha ? 1u : 0u);
            if (bSeparateAlpha) {
                uint32_t gnmSrcA = GXTranslate_EngineBlendFactorToGnm(srcBlendAlpha);
                uint32_t gnmOpA  = GXTranslate_EngineBlendOpToGnm(blendOpAlpha);
                uint32_t gnmDstA = GXTranslate_EngineBlendFactorToGnm(dstBlendAlpha);
                sceGnmBlendControlSetAlphaEquation(pOutBlendCtrl, gnmSrcA, gnmOpA, gnmDstA);
            }
        }

        uint32_t rtMaskBits = GXTranslate_EngineRenderTargetWriteMaskToGnm(writeMask, rtIdx);
        *reinterpret_cast<uint32_t*>(pOutBlendObj + 0x00) |= rtMaskBits;

        pRtDesc += 0x20;
        ++pOutBlendCtrl;
    }

    if (pBlendDesc[0x01] == 0) {
        uint32_t ctrl0 = *reinterpret_cast<uint32_t*>(pOutBlendObj + 0x04);
        for (uint32_t i = 1; i < 8; ++i) {
            *reinterpret_cast<uint32_t*>(pOutBlendObj + 0x04 + i * 4) = ctrl0;
        }
        uint32_t mask0 = *reinterpret_cast<uint32_t*>(pOutBlendObj + 0x00);
        uint32_t nibble = mask0 & 0xFu;
        uint32_t replicated = (mask0 << 28) | mask0 | (nibble << 4) | (nibble << 8) |
                              (nibble << 12) | (nibble << 16) | (nibble << 20) | (nibble << 24);
        *reinterpret_cast<uint32_t*>(pOutBlendObj + 0x00) = replicated;
    }
    return 0;
}

// [VMA: 0x2177fb0 - 0x2177fb1] (1 B)
extern "C" void GXGnmShaderStageDesc64_Release() {}

// [VMA: 0x2177f80 - 0x2177fa7] (39 B)
extern "C" void GXGnmShaderStageDesc64_Copy(__m128* pDst, const __m128* pSrc)
{
    __m128 v0 = _mm_load_ps(reinterpret_cast<const float*>(pSrc + 0));
    __m128 v1 = _mm_load_ps(reinterpret_cast<const float*>(pSrc + 1));
    __m128 v2 = _mm_load_ps(reinterpret_cast<const float*>(pSrc + 2));
    __m128 v3 = _mm_load_ps(reinterpret_cast<const float*>(pSrc + 3));
    _mm_store_ps(reinterpret_cast<float*>(pDst + 0), v0);
    _mm_store_ps(reinterpret_cast<float*>(pDst + 1), v1);
    _mm_store_ps(reinterpret_cast<float*>(pDst + 2), v2);
    _mm_store_ps(reinterpret_cast<float*>(pDst + 3), v3);
}

// [VMA: 0x2177ee0 - 0x2177ee1] (1 B)
extern "C" void GXGnmShaderStageDesc36_Release() {}

// [VMA: 0x2177eb0 - 0x2177ed5] (37 B)
extern "C" void GXGnmShaderStageDesc36_Copy(uint8_t* pDst, const uint8_t* pSrc)
{
    *reinterpret_cast<uint32_t*>(pDst + 0x20) = *reinterpret_cast<const uint32_t*>(pSrc + 0x20);
    *reinterpret_cast<uint64_t*>(pDst + 0x18) = *reinterpret_cast<const uint64_t*>(pSrc + 0x18);
    *reinterpret_cast<uint64_t*>(pDst + 0x10) = *reinterpret_cast<const uint64_t*>(pSrc + 0x10);
    *reinterpret_cast<uint64_t*>(pDst + 0x08) = *reinterpret_cast<const uint64_t*>(pSrc + 0x08);
    *reinterpret_cast<uint64_t*>(pDst + 0x00) = *reinterpret_cast<const uint64_t*>(pSrc + 0x00);
}

// [VMA: 0x2177e10 - 0x2177e11] (1 B)
extern "C" void GXGnmShaderStageDesc56_Release() {}

// [VMA: 0x2177dd0 - 0x2177e07] (55 B)
extern "C" void GXGnmShaderStageDesc56_Copy(uint64_t* pDst, const uint64_t* pSrc)
{
    pDst[6] = pSrc[6];
    pDst[5] = pSrc[5];
    pDst[4] = pSrc[4];
    pDst[3] = pSrc[3];
    pDst[2] = pSrc[2];
    pDst[1] = pSrc[1];
    pDst[0] = pSrc[0];
}

// [VMA: 0x2177d30 - 0x2177d31] (1 B)
extern "C" void GXGnmShaderContainer_GetDescStub() {}

// [VMA: 0x2177d20 - 0x2177d30] (16 B)
extern "C" void GXGnmShaderContainer_CopyState(void* pDst, const void* pSrc)
{
    memcpy(pDst, pSrc, 0x128);
}

// ============================================================================
// 6. GXGnmShaderStageAndInputLayoutCompiler (0x2175480 .. 0x2177c50)
// ============================================================================

extern "C" int     GXGnmShdr_UnpackContainerHeader(void* pOutHeaderPtr, void** ppOutStageData, void* pDev, const void* pShdrDesc, IGXAllocator* pAlloc, uint32_t stageType); // 0x26bb570
extern "C" int     GXGnmShdr_CompileComputeStage(void* pOutShaderPtr, void* pShaderObj, void* pStageData, void* pHeader, const void* pShdrDesc, uint64_t shdrSize, uint32_t stageType, void* pShaderMem, void* pOutAux0, void* pOutAux1); // 0x26ba280
extern "C" int     GXGnmShdr_CompilePixelStage(void* pOutShaderPtr, void* pShaderObj, void* pStageData, void* pHeader, const void* pShdrDesc, uint64_t shdrSize, uint32_t stageType, void* pShaderMem, void* pOutAux0, void* pOutAux1); // 0x26b9fc0
extern "C" int     GXGnmShdr_CompileVertexStage(void* pOutShaderPtr, void* pShaderObj, void* pStageData, void* pHeader, const void* pShdrDesc, uint64_t shdrSize, uint32_t stageType, void* pShaderMem, void* pOutAux0, void* pOutAux1); // 0x26b9cf0
extern "C" uint8_t GXGnmShdr_BuildResourceSlotTable(void* pOutSlotTable, const void* pCompiledShader, uint32_t numRes0, uint32_t numRes1); // 0x26bb670

// [VMA: 0x2177c50 - 0x2177c8a] (58 B)
// Frees both the compiled GNM shader object (+0x100) via allocator (+0x110) and header (+0x108) via allocator (+0x118).
extern "C" void GXGnmShaderContainer_Destroy(uint8_t* pShaderObj)
{
    void* pCompiledShader = *reinterpret_cast<void**>(pShaderObj + 0x100);
    IGXAllocator* pShaderAlloc = *reinterpret_cast<IGXAllocator**>(pShaderObj + 0x110);
    pShaderAlloc->vptr->Free(pShaderAlloc, pCompiledShader);

    void* pHeaderData = *reinterpret_cast<void**>(pShaderObj + 0x108);
    IGXAllocator* pHeaderAlloc = *reinterpret_cast<IGXAllocator**>(pShaderObj + 0x118);
    pHeaderAlloc->vptr->Free(pHeaderAlloc, pHeaderData);
}

// [VMA: 0x2177a40 - 0x2177c4c] (524 B)
// Validates the 'Shdr' magic (0x72646853) at +0x24 and compiles a Vertex (1), Pixel (5), or Compute (6) shader stage.
extern "C" int GXGnmShaderContainer_InitializeFromShdrBinary(
    uint8_t* pOutShaderObj,
    void* pDev,
    const uint8_t* pShdrDesc,
    uint64_t shdrSize,
    IGXAllocator* pShaderAlloc,
    IGXAllocator* pHeaderAlloc)
{
    if (*reinterpret_cast<const uint32_t*>(pShdrDesc + 0x24) != 0x72646853u) { // 'Shdr'
        return 8;
    }

    uint8_t stageType = pShdrDesc[0x2C];
    void* pStageData = nullptr;
    void** ppHeaderSlot = reinterpret_cast<void**>(pOutShaderObj + 0x108);

    int unpackRes = GXGnmShdr_UnpackContainerHeader(
        ppHeaderSlot, &pStageData, pDev, pShdrDesc, pHeaderAlloc, stageType);
    if (unpackRes != 0) {
        return unpackRes;
    }

    int compileRes = 8;
    if (stageType == 6u) { // Compute Shader
        compileRes = GXGnmShdr_CompileComputeStage(
            pOutShaderObj + 0x100, pOutShaderObj, pStageData, *ppHeaderSlot,
            pShdrDesc, shdrSize, stageType, pShaderAlloc, pOutShaderObj + 0x120, pOutShaderObj + 0x124);
    } else if (stageType == 5u) { // Pixel Shader
        compileRes = GXGnmShdr_CompilePixelStage(
            pOutShaderObj + 0x100, pOutShaderObj, pStageData, *ppHeaderSlot,
            pShdrDesc, shdrSize, stageType, pShaderAlloc, pOutShaderObj + 0x120, pOutShaderObj + 0x124);
    } else if (stageType == 1u) { // Vertex Shader
        compileRes = GXGnmShdr_CompileVertexStage(
            pOutShaderObj + 0x100, pOutShaderObj, pStageData, *ppHeaderSlot,
            pShdrDesc, shdrSize, stageType, pShaderAlloc, pOutShaderObj + 0x120, pOutShaderObj + 0x124);
    } else {
        pHeaderAlloc->vptr->Free(pHeaderAlloc, *ppHeaderSlot);
        return 8;
    }

    if (compileRes != 0) {
        pHeaderAlloc->vptr->Free(pHeaderAlloc, *ppHeaderSlot);
        return compileRes;
    }

    uint8_t* pCompiled = *reinterpret_cast<uint8_t**>(pOutShaderObj + 0x100);
    pOutShaderObj[0xD2] = GXGnmShdr_BuildResourceSlotTable(
        pOutShaderObj + 0xF0,
        pCompiled,
        *reinterpret_cast<const uint32_t*>(pCompiled + 0x0C),
        *reinterpret_cast<const uint32_t*>(pCompiled + 0x08));

    *reinterpret_cast<IGXAllocator**>(pOutShaderObj + 0x110) = pShaderAlloc;
    *reinterpret_cast<IGXAllocator**>(pOutShaderObj + 0x118) = pHeaderAlloc;
    return 0;
}

// Temporary buffer state used by std::stable_sort for 4-byte vertex element descriptors
struct GXSortTempBuffer {
    uint32_t* m_pBase00;      // +0x00
    uint32_t* m_pCur08;       // +0x08
    uint32_t* m_pEnd10;       // +0x10
    int64_t   m_capacity18;   // +0x18
    GXSortTempBuffer* m_pSelf20; // +0x20
};

#undef g_pStdNothrowTag
#define g_pStdNothrowTag (*reinterpret_cast<void**>(GX_VMA(0x53e4558ULL)))
extern "C" void* operator_new_nothrow(size_t size, void* pNothrowTag); // 0x2bc08b8
extern "C" void  std_throw_bad_alloc(); // 0x2bbf188

static inline void GXSortTempBuffer_PushBack(GXSortTempBuffer* pBuf, uint32_t val)
{
    uint32_t* pCur = pBuf->m_pCur08;
    if (pCur < pBuf->m_pEnd10) {
        pBuf->m_pCur08 = pCur + 1;
        *pCur = val;
    } else {
        uint32_t* pNew = nullptr;
        if (pCur) {
            *pCur = val;
            pNew = pBuf->m_pCur08;
        }
        pNew += 1;
        pBuf->m_pCur08 = pNew;
        pBuf->m_pEnd10 = pNew;
    }
}

static inline void GXSortTempBuffer_EnsureAllocated(GXSortTempBuffer* pBuf)
{
    if (pBuf->m_pBase00 != nullptr || pBuf->m_capacity18 <= 0) {
        return;
    }
    int64_t reqCount = pBuf->m_capacity18;
    if ((static_cast<uint64_t>(reqCount) >> 62) != 0) {
        std_throw_bad_alloc();
    }
    uint32_t* pMem = nullptr;
    while (true) {
        pMem = static_cast<uint32_t*>(operator_new_nothrow(static_cast<size_t>(reqCount) * 4u, g_pStdNothrowTag));
        if (pMem != nullptr) {
            break;
        }
        int64_t half = reqCount / 2;
        if (reqCount <= 1) {
            reqCount = half;
            break;
        }
        reqCount = half;
    }
    pBuf->m_pBase00    = pMem;
    pBuf->m_pCur08     = pMem;
    pBuf->m_pEnd10     = pMem ? (pMem + reqCount) : nullptr;
    pBuf->m_capacity18 = reqCount;
}

// [VMA: 0x2177890 - 0x2177a3f] (431 B)
// Merges two sorted runs [pFirst1, pLast1) and [pFirst2, pLast2) into the temporary sort buffer by byte 0 key.
extern "C" GXSortTempBuffer* GXVertexElementSort_MergeToTempBuffer(
    GXSortTempBuffer* pOutResult,
    const uint32_t* pFirst1,
    const uint32_t* pLast1,
    const uint32_t* pFirst2,
    const uint32_t* pLast2,
    GXSortTempBuffer* pTempWrapper)
{
    GXSortTempBuffer* pTargetBuf = pTempWrapper->m_pSelf20;
    while (pFirst1 != pLast1 && pFirst2 != pLast2) {
        pTargetBuf = pTempWrapper->m_pSelf20;
        uint8_t key2 = static_cast<uint8_t>(*pFirst2 & 0xFFu);
        uint8_t key1 = static_cast<uint8_t>(*pFirst1 & 0xFFu);
        if (key2 < key1) {
            GXSortTempBuffer_PushBack(pTargetBuf, *pFirst2);
            ++pFirst2;
        } else {
            GXSortTempBuffer_PushBack(pTargetBuf, *pFirst1);
            ++pFirst1;
        }
    }

    pTargetBuf = pTempWrapper->m_pSelf20;
    while (pFirst1 != pLast1) {
        GXSortTempBuffer_PushBack(pTargetBuf, *pFirst1);
        ++pFirst1;
    }
    while (pFirst2 != pLast2) {
        GXSortTempBuffer_PushBack(pTargetBuf, *pFirst2);
        ++pFirst2;
    }

    pOutResult->m_pBase00    = nullptr;
    pOutResult->m_pCur08     = nullptr;
    pOutResult->m_pEnd10     = nullptr;
    pOutResult->m_capacity18 = 0;
    pOutResult->m_pSelf20    = pTargetBuf;
    return pOutResult;
}

// [VMA: 0x2176f70 - 0x2177887] (2,327 B)
// Recursive buffered/in-place merge (std::__inplace_merge) for 4-byte vertex element descriptors sorted by byte 0.
extern "C" void GXVertexElementSort_InPlaceMerge(
    uint32_t* pFirst,
    uint32_t* pMiddle,
    uint32_t* pLast,
    int64_t len1,
    int64_t len2,
    GXSortTempBuffer* pTempWrapper)
{
    while (true) {
        if (len1 + len2 == 2) {
            if (static_cast<uint8_t>(*pMiddle & 0xFFu) < static_cast<uint8_t>(*pFirst & 0xFFu)) {
                uint32_t tmp = *pFirst;
                *pFirst = *pMiddle;
                *pMiddle = tmp;
            }
            return;
        }

        GXSortTempBuffer* pBuf = pTempWrapper->m_pSelf20;
        if (len1 <= len2) {
            GXSortTempBuffer_EnsureAllocated(pBuf);
            if (pBuf->m_capacity18 >= len1) {
                pBuf->m_pCur08 = pBuf->m_pBase00;
                for (uint32_t* pIt = pFirst; pIt != pMiddle; ++pIt) {
                    GXSortTempBuffer_PushBack(pBuf, *pIt);
                }
                uint32_t* pBufIt  = pBuf->m_pBase00;
                uint32_t* pBufEnd = pBuf->m_pCur08;
                uint32_t* pRight  = pMiddle;
                uint32_t* pOut    = pFirst;
                while (pRight != pLast && pBufIt != pBufEnd) {
                    if (static_cast<uint8_t>(*pRight & 0xFFu) < static_cast<uint8_t>(*pBufIt & 0xFFu)) {
                        *pOut++ = *pRight++;
                    } else {
                        *pOut++ = *pBufIt++;
                    }
                }
                while (pBufIt != pBufEnd) {
                    *pOut++ = *pBufIt++;
                }
                while (pRight != pLast) {
                    *pOut++ = *pRight++;
                }
                return;
            }
        } else {
            GXSortTempBuffer_EnsureAllocated(pBuf);
            if (pBuf->m_capacity18 >= len2) {
                pBuf->m_pCur08 = pBuf->m_pBase00;
                for (uint32_t* pIt = pMiddle; pIt != pLast; ++pIt) {
                    GXSortTempBuffer_PushBack(pBuf, *pIt);
                }
                uint32_t* pBufBegin = pBuf->m_pBase00;
                uint32_t* pBufCur   = pBuf->m_pCur08;
                uint32_t* pLeftCur  = pMiddle;
                uint32_t* pOut      = pLast;
                while (pLeftCur != pFirst && pBufCur != pBufBegin) {
                    if (static_cast<uint8_t>(pBufCur[-1] & 0xFFu) < static_cast<uint8_t>(pLeftCur[-1] & 0xFFu)) {
                        *--pOut = *--pLeftCur;
                    } else {
                        *--pOut = *--pBufCur;
                    }
                }
                while (pBufCur != pBufBegin) {
                    *--pOut = *--pBufCur;
                }
                while (pLeftCur != pFirst) {
                    *--pOut = *--pLeftCur;
                }
                return;
            }
        }

        // Split and rotate middle range [pCut1, pMiddle) and [pMiddle, pCut2)
        uint32_t* pCut1 = nullptr;
        uint32_t* pCut2 = nullptr;
        int64_t len11 = 0;
        int64_t len21 = 0;

        if (len1 > len2) {
            len11 = len1 / 2;
            pCut1 = pFirst + len11;
            int64_t count = pLast - pMiddle;
            uint32_t* pBase = pMiddle;
            uint8_t pivot = static_cast<uint8_t>(*pCut1 & 0xFFu);
            while (count > 0) {
                int64_t half = count / 2;
                if (static_cast<uint8_t>(pBase[half] & 0xFFu) < pivot) {
                    pBase += half + 1;
                    count -= half + 1;
                } else {
                    count = half;
                }
            }
            pCut2 = pBase;
            len21 = pCut2 - pMiddle;
        } else {
            len21 = len2 / 2;
            pCut2 = pMiddle + len21;
            int64_t count = pMiddle - pFirst;
            uint32_t* pBase = pFirst;
            uint8_t pivot = static_cast<uint8_t>(*pCut2 & 0xFFu);
            while (count > 0) {
                int64_t half = count / 2;
                if (!(pivot < static_cast<uint8_t>(pBase[half] & 0xFFu))) {
                    pBase += half + 1;
                    count -= half + 1;
                } else {
                    count = half;
                }
            }
            pCut1 = pBase;
            len11 = pCut1 - pFirst;
        }

        int64_t len12 = len1 - len11;
        uint32_t* pNewMiddle = nullptr;
        if (len12 <= len21 && pBuf->m_capacity18 >= len12) {
            pBuf->m_pCur08 = pBuf->m_pBase00;
            for (uint32_t* pIt = pCut1; pIt != pMiddle; ++pIt) {
                GXSortTempBuffer_PushBack(pBuf, *pIt);
            }
            uint32_t* pDst = pCut1;
            for (uint32_t* pIt = pMiddle; pIt != pCut2; ++pIt) {
                *pDst++ = *pIt;
            }
            pNewMiddle = pDst;
            for (uint32_t* pIt = pBuf->m_pBase00; pIt != pBuf->m_pCur08; ++pIt) {
                *pDst++ = *pIt;
            }
        } else if (pBuf->m_capacity18 >= len21) {
            pBuf->m_pCur08 = pBuf->m_pBase00;
            for (uint32_t* pIt = pMiddle; pIt != pCut2; ++pIt) {
                GXSortTempBuffer_PushBack(pBuf, *pIt);
            }
            uint32_t* pDst = pCut2;
            uint32_t* pSrc = pMiddle;
            while (pSrc != pCut1) {
                *--pDst = *--pSrc;
            }
            pNewMiddle = pDst;
            uint32_t* pOut = pCut1;
            for (uint32_t* pIt = pBuf->m_pBase00; pIt != pBuf->m_pCur08; ++pIt) {
                *pOut++ = *pIt;
            }
        } else {
            // In-place block rotation using GCD cycle shifting
            int64_t leftSpan  = pMiddle - pCut1;
            int64_t totalSpan = pCut2 - pCut1;
            if (leftSpan != 0 && pMiddle != pCut2) {
                int64_t a = totalSpan;
                int64_t b = leftSpan;
                while (b != 0) {
                    int64_t rem = a % b;
                    a = b;
                    b = rem;
                }
                if (a < totalSpan && a > 0) {
                    for (int64_t cycle = a - 1; cycle >= 0; --cycle) {
                        uint32_t* pStart = pCut1 + cycle;
                        uint32_t* pNext  = pCut1 + cycle + leftSpan;
                        if (pNext == pCut2) {
                            pNext = pCut1;
                        }
                        uint32_t savedVal = *pStart;
                        uint32_t* pCur = pStart;
                        while (pNext != pStart) {
                            *pCur = *pNext;
                            pCur  = pNext;
                            int64_t remRight = pCut2 - pNext;
                            if (leftSpan >= remRight) {
                                pNext = pCut1 + (leftSpan - remRight);
                            } else {
                                pNext += leftSpan;
                            }
                        }
                        *pCur = savedVal;
                    }
                }
            }
            pNewMiddle = pCut1 + len21;
        }

        GXVertexElementSort_InPlaceMerge(pFirst, pCut1, pNewMiddle, len11, len21, pTempWrapper);
        pFirst  = pNewMiddle;
        pMiddle = pCut2;
        len1    = len12;
        len2   -= len21;
    }
}

// [VMA: 0x2176820 - 0x2176f64] (1,860 B)
// Bottom-up buffered merge sort (chunk size 32 insertion sort followed by ping-pong merge passes).
extern "C" void GXVertexElementSort_BufferedMergeSort(
    uint32_t* pFirst,
    uint32_t* pLast,
    int64_t count,
    GXSortTempBuffer* pTempWrapper)
{
    uint32_t* pRemFirst = pFirst;
    if (count >= 32) {
        int64_t fullBlocksCount = count & ~31ll;
        int64_t rem = count;
        uint32_t* pBlock = pFirst;
        while (rem > 31) {
            uint32_t* pBlockEnd = pBlock + 32;
            for (uint32_t* pI = pBlock + 1; pI != pBlockEnd; ++pI) {
                uint32_t val = *pI;
                uint8_t key = static_cast<uint8_t>(val & 0xFFu);
                if (key < static_cast<uint8_t>(*pBlock & 0xFFu)) {
                    for (uint32_t* pShift = pI; pShift != pBlock; --pShift) {
                        pShift[0] = pShift[-1];
                    }
                    *pBlock = val;
                } else {
                    uint32_t* pPos = pI;
                    while (key < static_cast<uint8_t>(pPos[-1] & 0xFFu)) {
                        pPos[0] = pPos[-1];
                        --pPos;
                    }
                    *pPos = val;
                }
            }
            pBlock = pBlockEnd;
            rem -= 32;
        }
        pRemFirst = pFirst + fullBlocksCount;
    }

    if (pRemFirst != pLast && (pRemFirst + 1) != pLast) {
        for (uint32_t* pI = pRemFirst + 1; pI != pLast; ++pI) {
            uint32_t val = *pI;
            uint8_t key = static_cast<uint8_t>(val & 0xFFu);
            if (key < static_cast<uint8_t>(*pRemFirst & 0xFFu)) {
                for (uint32_t* pShift = pI; pShift != pRemFirst; --pShift) {
                    pShift[0] = pShift[-1];
                }
                *pRemFirst = val;
            } else {
                uint32_t* pPos = pI;
                while (key < static_cast<uint8_t>(pPos[-1] & 0xFFu)) {
                    pPos[0] = pPos[-1];
                    --pPos;
                }
                *pPos = val;
            }
        }
    }

    int64_t stepSize = 32;
    while (stepSize < count) {
        GXSortTempBuffer* pBuf = pTempWrapper->m_pSelf20;
        pBuf->m_pCur08 = pBuf->m_pBase00;

        int64_t twoStep = stepSize * 2;
        int64_t rem = count;
        uint32_t* pSrcCur = pFirst;
        while (rem >= twoStep) {
            GXSortTempBuffer tmpIn  = {};
            tmpIn.m_pSelf20 = pBuf;
            GXSortTempBuffer tmpOut = {};
            GXVertexElementSort_MergeToTempBuffer(
                &tmpOut, pSrcCur, pSrcCur + stepSize, pSrcCur + stepSize, pSrcCur + twoStep, &tmpIn);
            pBuf = tmpOut.m_pSelf20;
            if (tmpOut.m_pBase00) {
                operator_delete(tmpOut.m_pBase00);
            }
            if (tmpIn.m_pBase00) {
                operator_delete(tmpIn.m_pBase00);
            }
            pSrcCur += twoStep;
            rem -= twoStep;
        }

        if (rem > stepSize) {
            GXSortTempBuffer tmpIn  = {};
            tmpIn.m_pSelf20 = pBuf;
            GXSortTempBuffer tmpOut = {};
            GXVertexElementSort_MergeToTempBuffer(
                &tmpOut, pSrcCur, pSrcCur + stepSize, pSrcCur + stepSize, pLast, &tmpIn);
            if (tmpOut.m_pBase00) {
                operator_delete(tmpOut.m_pBase00);
            }
            if (tmpIn.m_pBase00) {
                operator_delete(tmpIn.m_pBase00);
            }
        } else {
            for (; pSrcCur != pLast; ++pSrcCur) {
                GXSortTempBuffer_PushBack(pBuf, *pSrcCur);
            }
        }

        // Merge back from temporary buffer into [pFirst, pLast)
        stepSize = twoStep;
        uint32_t* pBufFirst = pBuf->m_pBase00;
        uint32_t* pBufLast  = pBuf->m_pCur08;
        int64_t twoStepBack = stepSize * 2;
        int64_t remBack = count;
        uint32_t* pDstCur = pFirst;

        while (remBack >= twoStepBack) {
            const uint32_t* pL     = pBufFirst;
            const uint32_t* pLEnd  = pBufFirst + stepSize;
            const uint32_t* pR     = pLEnd;
            const uint32_t* pREnd  = pBufFirst + twoStepBack;
            while (pL != pLEnd && pR != pREnd) {
                if (static_cast<uint8_t>(*pR & 0xFFu) < static_cast<uint8_t>(*pL & 0xFFu)) {
                    *pDstCur++ = *pR++;
                } else {
                    *pDstCur++ = *pL++;
                }
            }
            while (pL != pLEnd) {
                *pDstCur++ = *pL++;
            }
            while (pR != pREnd) {
                *pDstCur++ = *pR++;
            }
            pBufFirst += twoStepBack;
            remBack   -= twoStepBack;
        }

        if (remBack > stepSize) {
            const uint32_t* pL    = pBufFirst;
            const uint32_t* pLEnd = pBufFirst + stepSize;
            const uint32_t* pR    = pLEnd;
            while (pL != pLEnd && pR != pBufLast) {
                if (static_cast<uint8_t>(*pR & 0xFFu) < static_cast<uint8_t>(*pL & 0xFFu)) {
                    *pDstCur++ = *pR++;
                } else {
                    *pDstCur++ = *pL++;
                }
            }
            while (pL != pLEnd) {
                *pDstCur++ = *pL++;
            }
            while (pR != pBufLast) {
                *pDstCur++ = *pR++;
            }
        } else {
            while (pBufFirst != pBufLast) {
                *pDstCur++ = *pBufFirst++;
            }
        }

        stepSize = twoStepBack;
    }
}

// [VMA: 0x2176660 - 0x2176820] (448 B)
// Stable sort entry point (std::__stable_sort) for 4-byte vertex element descriptors sorted by byte 0.
extern "C" void GXVertexElementSort_StableSort(
    uint32_t* pFirst,
    uint32_t* pLast,
    int64_t count,
    GXSortTempBuffer* pTempWrapper)
{
    if (count <= 32) {
        if (pFirst == pLast || (pFirst + 1) == pLast) {
            return;
        }
        for (uint32_t* pI = pFirst + 1; pI != pLast; ++pI) {
            uint32_t val = *pI;
            uint8_t key = static_cast<uint8_t>(val & 0xFFu);
            if (key < static_cast<uint8_t>(*pFirst & 0xFFu)) {
                for (uint32_t* pShift = pI; pShift != pFirst; --pShift) {
                    pShift[0] = pShift[-1];
                }
                *pFirst = val;
            } else {
                uint32_t* pPos = pI;
                while (key < static_cast<uint8_t>(pPos[-1] & 0xFFu)) {
                    pPos[0] = pPos[-1];
                    --pPos;
                }
                *pPos = val;
            }
        }
        return;
    }

    GXSortTempBuffer* pBuf = pTempWrapper->m_pSelf20;
    GXSortTempBuffer_EnsureAllocated(pBuf);

    int64_t half = (count + 1) / 2;
    uint32_t* pMiddle = pFirst + half;
    if (half > pBuf->m_capacity18) {
        GXVertexElementSort_StableSort(pFirst, pMiddle, half, pTempWrapper);
        GXVertexElementSort_StableSort(pMiddle, pLast, count - half, pTempWrapper);
    } else {
        GXVertexElementSort_BufferedMergeSort(pFirst, pMiddle, half, pTempWrapper);
        GXVertexElementSort_BufferedMergeSort(pMiddle, pLast, count - half, pTempWrapper);
    }
    GXVertexElementSort_InPlaceMerge(pFirst, pMiddle, pLast, half, count - half, pTempWrapper);
}

// [VMA: 0x21765c0 - 0x21765c1] (1 B)
extern "C" void GXInputLayout_Release() {}

// [VMA: 0x21754e0 - 0x21765b4] (4,308 B)
// Compiles an engine InputLayout / VertexDeclaration object (+0x00..+0x100), sorting vertex elements
// and computing 64-bit semantic/slot bitmasks and 0xBA-byte remapping tables.
extern "C" int GXInputLayout_CompileSemanticMappingAndBitmasks(
    uint8_t* pOutLayoutObj,
    uint64_t* pOutBitmaskArray,
    uint32_t elementCount,
    uint32_t layoutFlags,
    const uint8_t* pShaderReflectDesc,
    const uint8_t* pInputElements,
    uint32_t inputElemCount,
    IGXAllocator* pFallbackAllocator)
{
    *reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x00) = 0;
    memset(pOutLayoutObj + 0x18, 0xFF, 0xBA);
    *reinterpret_cast<uint16_t*>(pOutLayoutObj + 0xD6) = 0;
    *reinterpret_cast<uint32_t*>(pOutLayoutObj + 0xD2) = 0;

    uint32_t* pPackedElements = reinterpret_cast<uint32_t*>(pOutBitmaskArray + elementCount);
    *reinterpret_cast<uint64_t**>(pOutLayoutObj + 0xD8) = pOutBitmaskArray;
    *reinterpret_cast<uint32_t**>(pOutLayoutObj + 0xE0) = pPackedElements;
    *reinterpret_cast<uint32_t*>(pOutLayoutObj + 0xE8)  = elementCount;
    *reinterpret_cast<uint32_t*>(pOutLayoutObj + 0xEC)  = layoutFlags;
    *reinterpret_cast<uint64_t*>(pOutLayoutObj + 0xF8)  = 0;
    *reinterpret_cast<uint64_t*>(pOutLayoutObj + 0xF0)  = 0;

    // Allocate 0x174 bytes from TLS scratch arena (or fallback allocator)
    uint8_t* pTlsBase = nullptr;
    pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
    uint64_t* pTlsArena = *reinterpret_cast<uint64_t**>(pTlsBase + g_TlsScratchArenaOffset);
    if (!pTlsArena) {
        TlsScratchArena_InitThreadLocal();
        pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
        pTlsArena = *reinterpret_cast<uint64_t**>(pTlsBase + g_TlsScratchArenaOffset);
    }
    uint64_t savedTlsCur = pTlsArena[2];
    uint8_t* pScratchBuf = nullptr;
    if (savedTlsCur + 0x174u <= pTlsArena[3]) {
        pTlsArena[2] = savedTlsCur + 0x174u;
        pScratchBuf = reinterpret_cast<uint8_t*>(savedTlsCur);
    }

    IGXAllocator* pUsedHeapAlloc = nullptr;
    if (!pScratchBuf && pFallbackAllocator) {
        pScratchBuf = static_cast<uint8_t*>(pFallbackAllocator->vptr->Allocate(pFallbackAllocator, 0x174u));
        pUsedHeapAlloc = pFallbackAllocator;
    }
    if (!pScratchBuf) {
        return 9;
    }

    uint8_t* pWriteElem = reinterpret_cast<uint8_t*>(pPackedElements);
    uint32_t maxDynamicSlot = 0;
    uint32_t minDynamicSlot = 0xFFFFFFFFu;

    // Pass 1: Process primary vertex/constant semantic elements (types 0..7 and 26)
    if (inputElemCount != 0) {
        const uint8_t* pIn = pInputElements;
        for (uint32_t i = 0; i < inputElemCount; ++i, pIn += 4) {
            uint8_t semType  = pIn[0];
            uint8_t semIndex = pIn[1];
            uint8_t slotIdx  = pIn[2];
            uint8_t subFlag  = pIn[3];

            auto updateDynamicRange = [&](uint32_t slot, uint32_t span) {
                uint32_t endSlot = slot + span;
                if (maxDynamicSlot <= endSlot) {
                    maxDynamicSlot = endSlot;
                }
                if (slot < minDynamicSlot) {
                    minDynamicSlot = slot;
                }
            };

            auto setBitmaskSlot = [&](uint32_t baseBitOffset, uint64_t* pMaskBase) {
                uint32_t wordIdx = semIndex >> 6;
                uint32_t bitIdx  = (baseBitOffset + semIndex) - (wordIdx << 6);
                pMaskBase[wordIdx] |= (1ull << bitIdx);
            };

            switch (semType) {
            case 0:
            {
                uint8_t flagBit = subFlag & 1u;
                uint8_t mappedCode = flagBit;
                if (slotIdx >= 16u) {
                    if (flagBit != 0) {
                        updateDynamicRange(slotIdx, 8u);
                        mappedCode = 0x0Eu;
                    } else {
                        updateDynamicRange(slotIdx, 4u);
                        mappedCode = 0x0Du;
                    }
                }
                pWriteElem[0] = mappedCode;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = semIndex;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                setBitmaskSlot(0u, reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x00));
                pScratchBuf[semIndex * 2 + 0x000] = semIndex;
                pScratchBuf[semIndex * 2 + 0x001] = mappedCode;
                break;
            }
            case 1:
            {
                uint8_t mappedCode = 0x04u;
                if (slotIdx >= 16u) {
                    updateDynamicRange(slotIdx, 4u);
                    mappedCode = 0x11u;
                }
                pWriteElem[0] = mappedCode;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = semIndex;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                setBitmaskSlot(0u, reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10));
                pScratchBuf[semIndex * 2 + 0x100] = semIndex;
                pScratchBuf[semIndex * 2 + 0x101] = mappedCode;
                break;
            }
            case 2:
            {
                uint8_t mappedCode = 0x05u;
                if (slotIdx >= 16u) {
                    updateDynamicRange(slotIdx, 4u);
                    mappedCode = 0x12u;
                }
                pWriteElem[0] = mappedCode;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = semIndex;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                setBitmaskSlot(0x10u, reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10));
                pScratchBuf[semIndex * 2 + 0x120] = semIndex;
                pScratchBuf[semIndex * 2 + 0x121] = mappedCode;
                break;
            }
            case 3:
            {
                if (slotIdx >= 16u) {
                    updateDynamicRange(slotIdx, 4u);
                }
                break;
            }
            case 4:
            {
                uint8_t flagBit = subFlag & 1u;
                uint8_t mappedCode = flagBit | 0x02u;
                if (slotIdx > 15u) {
                    if (flagBit != 0) {
                        updateDynamicRange(slotIdx, 8u);
                        mappedCode = 0x10u;
                    } else {
                        updateDynamicRange(slotIdx, 4u);
                        mappedCode = 0x0Fu;
                    }
                }
                pWriteElem[0] = mappedCode;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = semIndex;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                setBitmaskSlot(0x24u, reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10));
                pScratchBuf[semIndex * 2 + 0x148] = semIndex;
                pScratchBuf[semIndex * 2 + 0x149] = mappedCode;
                break;
            }
            case 5:
            case 6:
                break;
            case 7:
            {
                uint8_t mappedCode = 0x06u;
                if (slotIdx >= 16u) {
                    updateDynamicRange(slotIdx, 1u);
                    mappedCode = 0x13u;
                }
                pWriteElem[0] = mappedCode;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = semIndex;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                setBitmaskSlot(0x38u, reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10));
                pScratchBuf[semIndex * 2 + 0x170] = semIndex;
                pScratchBuf[semIndex * 2 + 0x171] = mappedCode;
                break;
            }
            case 26:
            {
                uint8_t mappedCode = 0x07u;
                if (slotIdx >= 16u) {
                    updateDynamicRange(slotIdx, 2u);
                    mappedCode = 0x14u;
                }
                pWriteElem[0] = mappedCode;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = semIndex;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                setBitmaskSlot(0x39u, reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10));
                pScratchBuf[semIndex * 2 + 0x172] = semIndex;
                pScratchBuf[semIndex * 2 + 0x173] = mappedCode;
                break;
            }
            default:
                break;
            }
        }
    }

    // Pass 2: Mark shader reflection resources (+0x18 count, +0x38 pointer) and samplers (+0x24 count, +0x50 pointer)
    uint32_t reflectResCount = *reinterpret_cast<const uint32_t*>(pShaderReflectDesc + 0x18);
    if (reflectResCount != 0) {
        const uint8_t* pResEntry = *reinterpret_cast<const uint8_t* const*>(pShaderReflectDesc + 0x38);
        for (uint32_t i = 0; i < reflectResCount; ++i, pResEntry += 0x18) {
            uint32_t resIndex = *reinterpret_cast<const uint32_t*>(pResEntry + 0x00);
            uint8_t  resClass = pResEntry[0x09];
            uint32_t wordIdx  = resIndex >> 6;
            if (resClass == 0) {
                uint32_t bitIdx = (resIndex + 0x24u) - (wordIdx << 6);
                uint64_t* pMask = reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10) + wordIdx;
                if ((*pMask & (1ull << bitIdx)) == 0) {
                    *pMask |= (1ull << bitIdx);
                    *reinterpret_cast<uint16_t*>(pScratchBuf + resIndex * 2 + 0x148) = 0x09FFu;
                }
            } else if (resClass == 1 || resClass == 5) {
                uint32_t bitIdx = resIndex - (wordIdx << 6);
                uint64_t* pMask = reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x00) + wordIdx;
                if ((*pMask & (1ull << bitIdx)) == 0) {
                    *pMask |= (1ull << bitIdx);
                    pScratchBuf[resIndex * 2 + 0x000] = 0xFFu;
                    pScratchBuf[resIndex * 2 + 0x001] = 0x08u;
                }
            } else if (resClass == 4) {
                uint32_t bitIdx = (resIndex + 0x10u) - (wordIdx << 6);
                uint64_t* pMask = reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10) + wordIdx;
                if ((*pMask & (1ull << bitIdx)) == 0) {
                    *pMask |= (1ull << bitIdx);
                    *reinterpret_cast<uint16_t*>(pScratchBuf + resIndex * 2 + 0x120) = 0x0BFFu;
                }
            }
        }
    }

    uint32_t reflectSampCount = *reinterpret_cast<const uint32_t*>(pShaderReflectDesc + 0x24);
    if (reflectSampCount != 0) {
        const uint8_t* pSampEntry = *reinterpret_cast<const uint8_t* const*>(pShaderReflectDesc + 0x50) + 0x08;
        for (uint32_t i = 0; i < reflectSampCount; ++i, pSampEntry += 0x10) {
            uint32_t sampIndex = *reinterpret_cast<const uint32_t*>(pSampEntry);
            uint32_t wordIdx   = sampIndex >> 6;
            uint32_t bitIdx    = sampIndex - (wordIdx << 6);
            uint64_t* pMask    = reinterpret_cast<uint64_t*>(pOutLayoutObj + 0x10) + wordIdx;
            if ((*pMask & (1ull << bitIdx)) == 0) {
                *pMask |= (1ull << bitIdx);
                *reinterpret_cast<uint16_t*>(pScratchBuf + sampIndex * 2 + 0x100) = 0x0AFFu;
            }
        }
    }

    // Pass 3: Process extended semantic elements (types 19..27)
    if (inputElemCount != 0) {
        const uint8_t* pIn = pInputElements;
        for (uint32_t i = 0; i < inputElemCount; ++i, pIn += 4) {
            uint8_t semType  = pIn[0];
            uint8_t semIndex = pIn[1];
            uint8_t slotIdx  = pIn[2];
            uint32_t extIdx  = static_cast<uint32_t>(semType) - 19u;
            if (extIdx > 8u) {
                continue;
            }

            auto updateExtSlot = [&](uint8_t baseCode, uint8_t dynCode) -> uint8_t {
                if (slotIdx < 16u) {
                    return baseCode;
                }
                uint32_t endSlot = static_cast<uint32_t>(slotIdx) + 2u;
                if (maxDynamicSlot <= endSlot) {
                    maxDynamicSlot = endSlot;
                }
                if (slotIdx < minDynamicSlot) {
                    minDynamicSlot = slotIdx;
                }
                return dynCode;
            };

            uint8_t mappedCode = 0;
            switch (extIdx) {
            case 0: // 19
                mappedCode = updateExtSlot(0x08u, 0x15u);
                break;
            case 1: // 20
            case 7: // 26
                continue;
            case 2: // 21
                mappedCode = updateExtSlot(0x0Au, 0x17u);
                break;
            case 3: // 22
                mappedCode = updateExtSlot(0x0Bu, 0x18u);
                break;
            case 4: // 23
                if (slotIdx >= 16u) {
                    uint32_t endSlot = static_cast<uint32_t>(slotIdx) + 2u;
                    if (maxDynamicSlot <= endSlot) {
                        maxDynamicSlot = endSlot;
                    }
                    if (slotIdx < minDynamicSlot) {
                        minDynamicSlot = slotIdx;
                    }
                }
                continue;
            case 5: // 24
                mappedCode = updateExtSlot(0x0Cu, 0x19u);
                break;
            case 6: // 25
                mappedCode = updateExtSlot(0x09u, 0x16u);
                break;
            case 8: // 27
                pWriteElem[0] = 0x1Au;
                pWriteElem[1] = slotIdx;
                pWriteElem[2] = 0;
                pWriteElem[3] = 0;
                pWriteElem += 4;
                continue;
            }

            pWriteElem[0] = mappedCode;
            pWriteElem[1] = slotIdx;
            pWriteElem[2] = semIndex;
            pWriteElem[3] = 0;
            pWriteElem += 4;
        }
    }

    // Stable-sort the packed element descriptors by byte 0 (mappedCode)
    uint32_t* pElemArray = *reinterpret_cast<uint32_t**>(pOutLayoutObj + 0xE0);
    if (elementCount != 0) {
        GXSortTempBuffer sortBuf = {};
        sortBuf.m_capacity18 = static_cast<int64_t>((elementCount + 1u) >> 1);
        sortBuf.m_pSelf20    = &sortBuf;
        GXVertexElementSort_StableSort(pElemArray, pElemArray + elementCount, elementCount, &sortBuf);
        if (sortBuf.m_pBase00) {
            operator_delete(sortBuf.m_pBase00);
        }
    }

    // Build lookup table of element indices by mappedCode (0..26)
    uint32_t codeToIndex[27];
    for (uint32_t i = 0; i < 27; ++i) {
        codeToIndex[i] = 0x40u;
    }
    for (uint32_t i = 0; i < elementCount; ++i) {
        uint8_t code = static_cast<uint8_t>(pElemArray[i] & 0xFFu);
        if (code < 27u) {
            codeToIndex[code] = i;
        }
    }
    if (codeToIndex[8] == 0x40u)  codeToIndex[8]  = codeToIndex[21];
    if (codeToIndex[9] == 0x40u)  codeToIndex[9]  = codeToIndex[22];
    if (codeToIndex[10] == 0x40u) codeToIndex[10] = codeToIndex[23];
    if (codeToIndex[11] == 0x40u) codeToIndex[11] = codeToIndex[24];
    if (codeToIndex[12] == 0x40u) codeToIndex[12] = codeToIndex[25];

    // Populate the 0xBA-byte slot remap table at pOutLayoutObj + 0x18
    const uint64_t* pSlotMasks = reinterpret_cast<const uint64_t*>(pOutLayoutObj);
    for (uint32_t slot = 0; slot < 0xBAu; ++slot) {
        if ((pSlotMasks[slot >> 6] & (1ull << (slot & 63u))) == 0) {
            continue;
        }
        uint8_t semIdx = pScratchBuf[slot * 2 + 0];
        uint8_t code   = pScratchBuf[slot * 2 + 1];
        if (semIdx == 0xFFu) {
            pOutLayoutObj[0x18 + slot] = static_cast<uint8_t>(codeToIndex[code]);
        } else {
            for (uint32_t i = 0; i < elementCount; ++i) {
                const uint8_t* pRec = reinterpret_cast<const uint8_t*>(&pElemArray[i]);
                if (pRec[0] == code && pRec[2] == semIdx) {
                    pOutLayoutObj[0x18 + slot] = static_cast<uint8_t>(i);
                    break;
                }
            }
        }
    }

    uint32_t rootTableIdx = codeToIndex[26];
    if (rootTableIdx <= 0x3Fu) {
        uint32_t span = maxDynamicSlot - minDynamicSlot;
        uint8_t* pRootRec = reinterpret_cast<uint8_t*>(&pElemArray[rootTableIdx]);
        pRootRec[2] = static_cast<uint8_t>(minDynamicSlot + 0xF0u);
        pRootRec[3] = static_cast<uint8_t>(span);
    }
    pOutLayoutObj[0xD3] = static_cast<uint8_t>(rootTableIdx);

    uint32_t uavTableIdx = codeToIndex[7];
    if (uavTableIdx > 0x3Fu) {
        uavTableIdx = codeToIndex[20];
    }
    pOutLayoutObj[0xD4] = (uavTableIdx <= 0x3Fu) ? static_cast<uint8_t>(uavTableIdx) : 0x40u;

    uint64_t rootMaskBit = (rootTableIdx <= 0x3Fu) ? (1ull << rootTableIdx) : 0ull;

    // Build per-element dependency bitmasks in pOutBitmaskArray[0 .. elementCount - 1]
    for (uint32_t i = 0; i < elementCount; ++i) {
        uint8_t code = static_cast<uint8_t>(pElemArray[i] & 0xFFu);
        uint64_t selfBit = (i <= 0x3Fu) ? (1ull << i) : 0ull;
        auto bitFor = [&](uint32_t idx) -> uint64_t {
            return (idx <= 0x3Fu) ? (1ull << idx) : 0ull;
        };

        uint64_t depMask = 0;
        switch (code) {
        case 0:
        case 1:
            depMask = selfBit | bitFor(codeToIndex[8]) | bitFor(codeToIndex[21]);
            break;
        case 2:
        case 3:
            depMask = selfBit | bitFor(codeToIndex[9]) | bitFor(codeToIndex[22]);
            break;
        case 4:
            depMask = selfBit | bitFor(codeToIndex[10]) | bitFor(codeToIndex[23]);
            break;
        case 5:
            depMask = selfBit | bitFor(codeToIndex[11]) | bitFor(codeToIndex[24]);
            break;
        case 6:
        case 7:
            depMask = selfBit;
            break;
        case 8:
            depMask = selfBit | bitFor(codeToIndex[21]);
            break;
        case 9:
            depMask = selfBit | bitFor(codeToIndex[22]);
            break;
        case 10:
            depMask = selfBit | bitFor(codeToIndex[23]);
            break;
        case 11:
            depMask = selfBit | bitFor(codeToIndex[24]);
            break;
        case 12:
            depMask = selfBit | bitFor(codeToIndex[25]);
            break;
        case 13:
        case 14:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[8]) | bitFor(codeToIndex[21]);
            break;
        case 15:
        case 16:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[9]) | bitFor(codeToIndex[22]);
            break;
        case 17:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[10]) | bitFor(codeToIndex[23]);
            break;
        case 18:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[11]) | bitFor(codeToIndex[24]);
            break;
        case 19:
        case 20:
            depMask = selfBit | rootMaskBit;
            break;
        case 21:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[8]);
            break;
        case 22:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[9]);
            break;
        case 23:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[10]);
            break;
        case 24:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[11]);
            break;
        case 25:
            depMask = selfBit | rootMaskBit | bitFor(codeToIndex[12]);
            break;
        case 26:
            depMask = 0;
            break;
        default:
            continue;
        }
        pOutBitmaskArray[i] = depMask;
    }

    if (pUsedHeapAlloc) {
        pUsedHeapAlloc->vptr->Free(pUsedHeapAlloc, pScratchBuf);
    }

    pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
    pTlsArena = *reinterpret_cast<uint64_t**>(pTlsBase + g_TlsScratchArenaOffset);
    if (!pTlsArena) {
        TlsScratchArena_InitThreadLocal();
        pTlsBase = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
        pTlsArena = *reinterpret_cast<uint64_t**>(pTlsBase + g_TlsScratchArenaOffset);
    }
    if (savedTlsCur < pTlsArena[0] || savedTlsCur > pTlsArena[1]) {
        auto fnAssertLog = reinterpret_cast<void (*)(uintptr_t, uint32_t, uintptr_t)>(GX_VMA(0x20b55b0ULL));
        fnAssertLog(GX_VMA(0x493c62dULL), 0x233u, GX_VMA(0x493c690ULL));
    }
    if (pTlsArena[2] > savedTlsCur) {
        pTlsArena[2] = savedTlsCur;
    }
    return 0;
}

// [VMA: 0x2175480 - 0x21754d9] (89 B)
// Validates semantic types (0..27) against bitmasks 0x0F680097 / 0x00840108 and computes scratch size (12 bytes per active element).
extern "C" int GXInputLayout_CalculateRequiredScratchSize(
    uint64_t* pOutByteSize,
    uint32_t* pOutElementCount,
    const uint8_t* pInputElements,
    uint32_t inputElemCount)
{
    uint32_t activeCount = 0;
    *pOutElementCount = 0;

    if (inputElemCount != 0) {
        const uint32_t kActiveMask   = 0x0F680097u;
        const uint32_t kPassthruMask = 0x00840108u;
        for (uint32_t i = 0; i < inputElemCount; ++i) {
            uint8_t semType = pInputElements[i * 4];
            if (semType > 0x1Bu) {
                return 8;
            }
            if ((kActiveMask & (1u << semType)) != 0) {
                ++activeCount;
                *pOutElementCount = activeCount;
            } else if ((kPassthruMask & (1u << semType)) == 0) {
                return 8;
            }
        }
    }

    *pOutByteSize = static_cast<uint64_t>(activeCount * 12u);
    return 0;
}

} // namespace GX
} // namespace Bloodborne
