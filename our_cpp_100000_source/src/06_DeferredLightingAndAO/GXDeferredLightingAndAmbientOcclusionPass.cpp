#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <immintrin.h>

#include "gx_direct_vulkan.h"

// ============================================================================
// Module 06: Deferred Lighting and Ambient Occlusion (Step 2)
// Range: 0x2297000 .. 0x22a0000 (20 functions, 36,817 bytes)
// Target: decolied files/06_DeferredLightingAndAO/GXDeferredLightingAndAmbientOcclusionPass.cpp
// ============================================================================

namespace {

struct CmdPacketHeader {
    uint32_t packedWord;
    uint32_t pad04;
    void*    pPayload;
    uint8_t  bFlag0;
    uint8_t  bFlag1;
    uint8_t  pad12[6];
};

static inline uint32_t packCmdQueueHeader(uint64_t w0, uint64_t w1) {
    uint32_t a = (static_cast<uint32_t>(w0) << 28) | (static_cast<uint32_t>(w0 >> 10) & 0x0fc00000u);
    uint32_t b = (static_cast<uint32_t>(w1) & 0x3fu) << 16;
    uint32_t c = static_cast<uint32_t>(static_cast<uint16_t>(w1 >> 32));
    return a | b | c;
}

struct InlineDLWString {
    void*    pAllocator;
    union {
        void*    pStrData;
        uint16_t inlineBuf[8];
    };
    uint64_t length;
    uint64_t capacity;
    uint64_t pad28;
};

extern "C" void sub_20b55b0(const char* file, uint32_t line, const char* fmt, ...);

static inline void resetInlineDLWString(uint8_t* base, size_t strOffset, size_t flagOffset, bool resetFields) {
    uint64_t cap = *reinterpret_cast<uint64_t*>(base + strOffset + 0x18);
    if (cap >= 8) {
        if (cap + 1 >= 0x41) {
            sub_20b55b0(reinterpret_cast<const char*>(GX_VMA(0x4940298ULL)),
                        0x120,
                        reinterpret_cast<const char*>(GX_VMA(0x4940300ULL)));
        }
        base[flagOffset] &= 0xfdu;
    }
    if (resetFields) {
        *reinterpret_cast<uint64_t*>(base + strOffset + 0x18) = 7;
        *reinterpret_cast<uint64_t*>(base + strOffset + 0x10) = 0;
        *reinterpret_cast<uint16_t*>(base + strOffset) = 0;
    }
}

} // namespace

extern "C" {

// External engine symbols used across 0x2297000 .. 0x22a0000
void sub_22d4d30(void* pCtx, void* pRenderDev, uint32_t viewIdx, uint32_t passId,
                 void* pCamera, void* pAltCamera, uint32_t width, uint32_t height,
                 uint32_t flag0, uint32_t flag1);
void sub_22d5480(void* pCtx);
void* sub_2289bb0(void* pCmdQueue, uint32_t type, uint32_t size, uint32_t align);
void sub_2289ca0(void* pCmdQueue, const void* pPacketInfo);
void sub_221c270(void* pArg);
void sub_22919c0(void* pCullVol);
void sub_2291c30(void* pCullVol, void* pCamera);
void sub_ea56d0(void* pQuery, void* pSceneMgr, void* pCullVol, uint32_t mask0, uint32_t mask1, void* pUserArg);
void sub_ea57c0(void* pQuery);
void sub_22a0a30(void* pDispatchArgs);

void* sub_e7dad0(void* pParamRepo, const void* pId);
void sub_e7ee50(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_e5f040(void* pTarget, const void* pParams);
void sub_e5d1d0(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass,
                void* pTexA, void* pTexB, void* pArg7, void* pArg8, uint32_t flag9);
void sub_e63630(void* pTarget, const void* pParams);
void sub_e60a20(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass,
                void* pTexA, void* pTexB, void* pArg7, void* pArg8, void* pArg9, uint32_t flag10);

void* sub_22044a0(void* pRepo, const void* pId);
void sub_e8a710(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_eba090(void* pTarget, const void* pParams);
void* sub_21ea1e0(void* pRepo, const void* pId);
void sub_29356b0(void* pOutParams);
void sub_e713b0(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_eba1a0(void* pTarget, const void* pParams);
void sub_20b55b0(const char* file, uint32_t line, const char* fmt, ...);
void* sub_21e62d0(void* pRepo, const void* pId);
void sub_e6d670(void* pOutParams);
void sub_e6d790(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_eba150(void* pTarget, const void* pParams);
void sub_eb5340(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass, void* pTexA, void* pTexB);

void* sub_21fe970(void* pRepo, const void* pId);
void sub_e83620(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_21d4d40(void* pTarget, const void* pParams);
void sub_21d5050(void* pTarget, const void* pParams);
void* sub_21f68b0(void* pRepo, const void* pId);
void sub_e809e0(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void* sub_21ef2e0(void* pRepo, const void* pId);
void sub_e75b50(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void* sub_21e14c0(void* pRepo, const void* pId);
void sub_e6c5d0(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void* sub_21fa460(void* pRepo, const void* pId);
void sub_e82290(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void* sub_21f2b90(void* pRepo, const void* pId);
void sub_e788f0(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_21d5660(void* pTarget, const void* pParams);
void sub_21d4860(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass,
                 void* pTexA, void* pTexB, void* pArg7, void* pArg8, void* pArg9, void* pArg10);

void sub_e596f0(void* pTarget, const void* pParams);
void sub_e58c50(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass, void* pArg5);
void* sub_e74bd0(void* pRepo, const void* pId);
void sub_e72620(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_e5ab20(void* pTarget, const void* pParams);
void sub_e5a010(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass, void* pArg5);
void* sub_e7b3b0(void* pRepo, const void* pId);
void sub_e79650(void* pOutParams, const void* pSrcA, const void* pSrcB, float weight);
void sub_e5c680(void* pTarget, const void* pParams);
void sub_e5bb70(void* pTarget, void* pCtx, uint32_t passType, uint32_t subPass,
                void* pTexA, uint8_t bFlag, void* pArg7, uint32_t mode8);

void sub_22b0490(void* pArg);
void sub_228e7a0(void* pTaskQueue, void* pCallback, uint32_t arg2, uint32_t arg3);

// ============================================================================
// 0x229e990: sub_229e990 (8,349 bytes)
// Main Scene View & Post-Process Parameter Evaluation / Pass Orchestrator
// ============================================================================
__attribute__((noinline)) void sub_229e990(void* pSelf, void* pRenderDev, void* pCamera, void* pArg4,
                 void* pArg5, void* pViewCtx, uint32_t viewIdx, uint32_t passId,
                 void* pUserArg) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* viewBytes = static_cast<uint8_t*>(pViewCtx);

    alignas(16) uint8_t passCtx[0x870];
    std::memset(passCtx, 0, sizeof(passCtx));

    sub_22d4d30(passCtx, pRenderDev, viewIdx, passId, pCamera, nullptr, 0, 0, 0, 0);

    passCtx[0x400] = 1;
    passCtx[0x401] = 1;
    passCtx[0x403] = 1;
    *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0;
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 5;

    void* pCmdQueue = *reinterpret_cast<void**>(passCtx + 0x450);
    void* pPkt0 = sub_2289bb0(pCmdQueue, 0xf8, 8, 4);
    uint8_t bFlag401 = passCtx[0x401];
    if (pPkt0 != nullptr) {
        if (bFlag401 != 0) {
            CmdPacketHeader hdr{};
            hdr.packedWord = packCmdQueueHeader(*reinterpret_cast<uint64_t*>(passCtx + 0x110),
                                                *reinterpret_cast<uint64_t*>(passCtx + 0x118));
            uint8_t bFlag403 = passCtx[0x403];
            hdr.bFlag0 = (bFlag403 != 0) ? 0 : (passCtx[0x402] != 0 ? 1 : 0);
            hdr.bFlag1 = bFlag403;
            hdr.pPayload = pPkt0;
            sub_2289ca0(*reinterpret_cast<void**>(passCtx + 0x450), &hdr);
            passCtx[0x401] = 0;
        }
        passCtx[0x400] = 0;
        passCtx[0x403] = 0;
        passCtx[0x402] = 0;
        sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
    } else {
        passCtx[0x400] = 0;
        passCtx[0x403] = 0;
        passCtx[0x402] = 0;
        if (bFlag401 == 0) {
            sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
        }
    }

    passCtx[0x400] = 1;
    passCtx[0x401] = 1;
    passCtx[0x403] = 0;
    *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0;
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 5;

    uint8_t* pPkt1 = static_cast<uint8_t*>(sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 4, 0x18, 4));
    uint8_t bFlag1 = passCtx[0x401];
    bool bCommit1 = (bFlag1 == 0);
    uint8_t* pWriteSlot = nullptr;
    if (pPkt1 != nullptr) {
        if (bFlag1 != 0) {
            CmdPacketHeader hdr{};
            hdr.packedWord = packCmdQueueHeader(*reinterpret_cast<uint64_t*>(passCtx + 0x110),
                                                *reinterpret_cast<uint64_t*>(passCtx + 0x118));
            uint8_t bFlag403 = passCtx[0x403];
            hdr.bFlag0 = (bFlag403 != 0) ? 0 : (passCtx[0x402] != 0 ? 1 : 0);
            hdr.bFlag1 = bFlag403;
            hdr.pPayload = pPkt1;
            sub_2289ca0(*reinterpret_cast<void**>(passCtx + 0x450), &hdr);
            passCtx[0x401] = 0;
        }
        pWriteSlot = pPkt1 + pPkt1[1] + 4;
        bCommit1 = true;
    }
    if (pWriteSlot != nullptr) {
        *reinterpret_cast<uint64_t*>(pWriteSlot) = GX_VMA(0x221c270ULL);
        *reinterpret_cast<void**>(pWriteSlot + 0x08) = selfBytes + 0x5860;
        *reinterpret_cast<uint64_t*>(pWriteSlot + 0x10) = 0;
    }
    passCtx[0x400] = 0;
    passCtx[0x403] = 0;
    passCtx[0x402] = 0;
    if (bCommit1) {
        sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
    }
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;
    sub_22d5480(passCtx);

    alignas(16) uint8_t cullVol[0x510];
    sub_22919c0(cullVol);
    sub_2291c30(cullVol, pCamera);

    alignas(16) uint8_t sceneQuery[0x50];
    void* pSceneMgr = *reinterpret_cast<void**>(GX_VMA(0x5540688ULL));
    sub_ea56d0(sceneQuery, pSceneMgr, cullVol, 0x7fffffff, 0xffffffffu, pUserArg);

    if (viewBytes[0x296d] != 0) {
        uint8_t* pListContainer = *reinterpret_cast<uint8_t**>(selfBytes + 0x5870);
        uint8_t** it = *reinterpret_cast<uint8_t***>(pListContainer + 0x18);
        uint8_t** end = *reinterpret_cast<uint8_t***>(pListContainer + 0x20);
        if (it != end) {
            uint32_t queryMask = *reinterpret_cast<uint32_t*>(sceneQuery + 0x08);
            while (it != end) {
                uint8_t* pItem = *it;
                if (pItem[0xb0] != 0 && (*reinterpret_cast<uint32_t*>(pItem + 0xa0) & queryMask) != 0) {
                    void* vtbl = *reinterpret_cast<void**>(sceneQuery);
                    auto fnVisit = *reinterpret_cast<bool (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x10);
                    if (!fnVisit(sceneQuery, pItem)) {
                        break;
                    }
                    end = *reinterpret_cast<uint8_t***>(pListContainer + 0x20);
                }
                ++it;
            }
        }
    }

    struct PassDispatchBlock {
        void*    pSelf;
        void*    pRenderDev;
        void*    pCamera;
        void*    pArg4;
        void*    pArg5;
        void*    pViewCtx;
        uint32_t viewIdx;
        uint32_t passId;
        void*    pSceneQuery;
    } dispatchArgs = {
        pSelf, pRenderDev, pCamera, pArg4, pArg5, pViewCtx, viewIdx, passId, sceneQuery
    };
    sub_22a0a30(&dispatchArgs);

    if (passId == 0x3e) {
        uint8_t* pGlobalPostCfg = reinterpret_cast<uint8_t*>(GX_VMA(0x55406e0ULL));
        bool bRunStandardEnv = true;
        if (*reinterpret_cast<uint32_t*>(pGlobalPostCfg) != 0) {
            bRunStandardEnv = ((viewBytes[0x49] & 0x04) == 0);
        }

        uint32_t viewWidth = *reinterpret_cast<uint32_t*>(viewBytes + 0x18);
        uint32_t viewHeight = *reinterpret_cast<uint32_t*>(viewBytes + 0x1c);
        sub_22d4d30(passCtx, pRenderDev, viewIdx, 0x3e, pCamera, nullptr, viewWidth, viewHeight, 0, 0);

        alignas(16) uint8_t evalBuf[0x240];

        if (bRunStandardEnv) {
            if (viewBytes[0x2970] != 0) {
                float w = *reinterpret_cast<float*>(viewBytes + 0x2b78);
                uint8_t* pA = static_cast<uint8_t*>(sub_e7dad0(*reinterpret_cast<void**>(selfBytes + 0x59c8), viewBytes + 0x2ad8));
                uint8_t* pB = static_cast<uint8_t*>(sub_e7dad0(*reinterpret_cast<void**>(selfBytes + 0x59c8), viewBytes + 0x2b40));
                sub_e7ee50(evalBuf, pA + 8, pB + 8, w);
                if (viewBytes[0x2bc5] != 0) {
                    *reinterpret_cast<float*>(evalBuf + 0x10) = *reinterpret_cast<float*>(viewBytes + 0x2bc8);
                    evalBuf[0x4c] = 0;
                }
                sub_e5f040(viewBytes + 0x25c0, evalBuf);
                uint8_t* pRtSet = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0) ? (viewBytes + 0x5a8) : (viewBytes + 0x188);
                void* pDepthOrMask = nullptr;
                if (viewBytes[0x2969] != 0 && selfBytes[0x78b0] == 0) {
                    pDepthOrMask = *reinterpret_cast<void**>(viewBytes + 0xcf8);
                }
                sub_e5d1d0(viewBytes + 0x25c0, passCtx, 0x31, 0,
                           *reinterpret_cast<void**>(pRtSet + 0x18),
                           *reinterpret_cast<void**>(viewBytes + 0x2520),
                           *reinterpret_cast<void**>(viewBytes + 0x2578),
                           pDepthOrMask, 1);
            }

            if (viewBytes[0x2971] != 0) {
                float w = *reinterpret_cast<float*>(viewBytes + 0x2b78);
                uint8_t* pA = static_cast<uint8_t*>(sub_e7dad0(*reinterpret_cast<void**>(selfBytes + 0x59c8), viewBytes + 0x2ad8));
                uint8_t* pB = static_cast<uint8_t*>(sub_e7dad0(*reinterpret_cast<void**>(selfBytes + 0x59c8), viewBytes + 0x2b40));
                sub_e7ee50(evalBuf, pA + 8, pB + 8, w);
                if (viewBytes[0x2bc5] != 0) {
                    *reinterpret_cast<float*>(evalBuf + 0x10) = *reinterpret_cast<float*>(viewBytes + 0x2bc8);
                    evalBuf[0x4c] = 0;
                }
                sub_e63630(viewBytes + 0x26f0, evalBuf);
                uint8_t* pRtSet = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0) ? (viewBytes + 0x5a8) : (viewBytes + 0x188);
                void* pDepthOrMask = nullptr;
                if (viewBytes[0x2969] != 0 && selfBytes[0x78b0] == 0) {
                    pDepthOrMask = *reinterpret_cast<void**>(viewBytes + 0xcf8);
                }
                sub_e60a20(viewBytes + 0x26f0, passCtx, 0x31, 0,
                           *reinterpret_cast<void**>(pRtSet + 0x18),
                           *reinterpret_cast<void**>(viewBytes + 0x2520),
                           *reinterpret_cast<void**>(viewBytes + 0x2578),
                           pDepthOrMask, viewBytes + 0x28b0, 1);
            }

            void* pEnvTarget = viewBytes + 0x1da0;
            float wTone = *reinterpret_cast<float*>(viewBytes + 0x2b4c);
            uint8_t* pToneA = static_cast<uint8_t*>(sub_22044a0(*reinterpret_cast<void**>(selfBytes + 0x5978), viewBytes + 0x2a80));
            uint8_t* pToneB = static_cast<uint8_t*>(sub_22044a0(*reinterpret_cast<void**>(selfBytes + 0x5978), viewBytes + 0x2ae8));
            if (*reinterpret_cast<float*>(pToneA + 0xe0) != *reinterpret_cast<float*>(pToneA + 0x94)) {
                *reinterpret_cast<float*>(pToneA + 0xe0) = *reinterpret_cast<float*>(pToneA + 0x94);
            }
            if (*reinterpret_cast<float*>(pToneB + 0xe0) != *reinterpret_cast<float*>(pToneB + 0x94)) {
                *reinterpret_cast<float*>(pToneB + 0xe0) = *reinterpret_cast<float*>(pToneB + 0x94);
            }

            *reinterpret_cast<uint32_t*>(evalBuf + 0x00) = 0x40800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x04) = 0x400ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x08) = 2u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x0c) = 0x42000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x10) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x14) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x18) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x1c) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x20) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x24) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x28) = 0x3e4ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x2c) = 0x3e3851ecu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x30) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x34) = 0x3dcccccd;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x38) = 0x41200000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x3c) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x40) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x44) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x48) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x4c) = 0x3f4ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x50) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x54) = 0x42700000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x58) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x5c) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x60) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x64) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x68) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x6c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x70) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x74) = 3u;
            *reinterpret_cast<uint64_t*>(evalBuf + 0x78) = 0ULL;
            *reinterpret_cast<uint64_t*>(evalBuf + 0x80) = 0ULL;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x88) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x8c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x90) = 0x3fe66666u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x94) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x98) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x9c) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xa0) = 0x41200000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xa4) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xa8) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xac) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xb0) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xb4) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xb8) = 0x400ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xbc) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xc0) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xc4) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xc8) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xcc) = 0xc0000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xd0) = 0x40800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xd4) = 0x40800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xd8) = 0u;

            sub_e8a710(evalBuf, pToneA + 8, pToneB + 8, wTone);
            sub_eba090(pEnvTarget, evalBuf);

            float wLut = *reinterpret_cast<float*>(viewBytes + 0x2b58);
            uint8_t* pLutA = static_cast<uint8_t*>(sub_21ea1e0(*reinterpret_cast<void**>(selfBytes + 0x59b0), viewBytes + 0x2a98));
            uint8_t* pLutB = static_cast<uint8_t*>(sub_21ea1e0(*reinterpret_cast<void**>(selfBytes + 0x59b0), viewBytes + 0x2b00));
            sub_29356b0(evalBuf);
            sub_e713b0(evalBuf, pLutA + 8, pLutB + 8, wLut);
            sub_eba1a0(pEnvTarget, evalBuf);

            resetInlineDLWString(evalBuf, 0x180, 0x221, true);
            resetInlineDLWString(evalBuf, 0x0d0, 0x171, true);
            resetInlineDLWString(evalBuf, 0x020, 0x0c1, false);

            float wBloom = *reinterpret_cast<float*>(viewBytes + 0x2b54);
            uint8_t* pBloomA = static_cast<uint8_t*>(sub_21e62d0(*reinterpret_cast<void**>(selfBytes + 0x5980), viewBytes + 0x2a90));
            uint8_t* pBloomB = static_cast<uint8_t*>(sub_21e62d0(*reinterpret_cast<void**>(selfBytes + 0x5980), viewBytes + 0x2af8));
            sub_e6d670(evalBuf);
            sub_e6d790(evalBuf, pBloomA + 0x10, pBloomB + 0x10, wBloom);
            sub_eba150(pEnvTarget, evalBuf);

            uint8_t* pRtSet = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0) ? (viewBytes + 0x5a8) : (viewBytes + 0x188);
            sub_eb5340(pEnvTarget, passCtx, 0x34, 8,
                       *reinterpret_cast<void**>(pRtSet + 0x28),
                       *reinterpret_cast<void**>(viewBytes + 0x160));
        }

        sub_22d5480(passCtx);

        if (*reinterpret_cast<uint32_t*>(pGlobalPostCfg) != 0 && (viewBytes[0x49] & 0x04) != 0) {
            void* pAltCam = (viewBytes[0x297e] != 0) ? static_cast<void*>(viewBytes + 0x2980) : pCamera;
            sub_22d4d30(passCtx, pRenderDev, viewIdx, 0x3e, pCamera, pAltCam,
                        *reinterpret_cast<uint32_t*>(viewBytes + 0x18),
                        *reinterpret_cast<uint32_t*>(viewBytes + 0x1c), 1, 0);

            uint8_t* pYebisState = *reinterpret_cast<uint8_t**>(selfBytes + 0x6078);
            uint8_t* pYebisSub = pYebisState + 0xaf0;

            float w0 = *reinterpret_cast<float*>(viewBytes + 0x2b48);
            uint8_t* p0A = static_cast<uint8_t*>(sub_21fe970(*reinterpret_cast<void**>(selfBytes + 0x5970), viewBytes + 0x2a78));
            uint8_t* p0B = static_cast<uint8_t*>(sub_21fe970(*reinterpret_cast<void**>(selfBytes + 0x5970), viewBytes + 0x2ae0));

            *reinterpret_cast<uint32_t*>(evalBuf + 0x00) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x04) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x08) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x0c) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x10) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x14) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x18) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x1c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x20) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x24) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x28) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x2c) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x30) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x34) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x38) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x3c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x40) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x44) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x48) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x4c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x50) = 0x400ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x54) = 0xbf800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x58) = 0x41200000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x5c) = 0x41200000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x60) = 0x43020000u;
            evalBuf[0x64] = 0;
            evalBuf[0x65] = 0;

            sub_e83620(evalBuf, p0A + 8, p0B + 8, w0);
            sub_21d4d40(pYebisSub, evalBuf);

            float w1 = *reinterpret_cast<float*>(viewBytes + 0x2b4c);
            uint8_t* p1A = static_cast<uint8_t*>(sub_22044a0(*reinterpret_cast<void**>(selfBytes + 0x5978), viewBytes + 0x2a80));
            uint8_t* p1B = static_cast<uint8_t*>(sub_22044a0(*reinterpret_cast<void**>(selfBytes + 0x5978), viewBytes + 0x2ae8));
            float globalRef = *reinterpret_cast<float*>(pGlobalPostCfg + 0x98);
            if (*reinterpret_cast<float*>(p1A + 0xdc) != globalRef) {
                *reinterpret_cast<float*>(p1A + 0xdc) = globalRef;
            }
            if (*reinterpret_cast<float*>(p1B + 0xdc) != globalRef) {
                *reinterpret_cast<float*>(p1B + 0xdc) = globalRef;
            }

            *reinterpret_cast<uint32_t*>(evalBuf + 0x00) = 0x40800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x04) = 0x400ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x08) = 2u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x0c) = 0x42000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x10) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x14) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x18) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x1c) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x20) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x24) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x28) = 0x3e4ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x2c) = 0x3e3851ecu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x30) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x34) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x38) = 0x41200000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x3c) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x40) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x44) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x48) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x4c) = 0x3f4ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x50) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x54) = 0x42700000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x58) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x5c) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x60) = 0x3dcccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x64) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x68) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x6c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x70) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x74) = 3u;
            *reinterpret_cast<uint64_t*>(evalBuf + 0x78) = 0ULL;
            *reinterpret_cast<uint64_t*>(evalBuf + 0x80) = 0ULL;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x88) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x8c) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x90) = 0x3fe66666u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x94) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x98) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x9c) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xa0) = 0x41200000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xa4) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xa8) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xac) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xb0) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xb4) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xb8) = 0x400ccccdu;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xbc) = 1u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xc0) = 0u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xc4) = 0x3f800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xc8) = 0x3f000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xcc) = 0xc0000000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xd0) = 0x40800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xd4) = 0x40800000u;
            *reinterpret_cast<uint32_t*>(evalBuf + 0xd8) = 0u;

            sub_e8a710(evalBuf, p1A + 8, p1B + 8, w1);
            if (pYebisState[0x3200] == 0) {
                uint32_t mode24 = *reinterpret_cast<uint32_t*>(evalBuf + 0x24);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x50) = mode24;
                std::memcpy(pGlobalPostCfg + 0x64, evalBuf + 0x3c, 16);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x74) = *reinterpret_cast<uint32_t*>(evalBuf + 0x28);
                std::memcpy(pGlobalPostCfg + 0x78, evalBuf + 0x4c, 16);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x88) = *reinterpret_cast<uint32_t*>(evalBuf + 0x5c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x8c) = *reinterpret_cast<uint32_t*>(evalBuf + 0x60);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x90) = *reinterpret_cast<uint32_t*>(evalBuf + 0x64);
                if (mode24 == 0) {
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x98) = *reinterpret_cast<uint32_t*>(evalBuf + 0x00);
                }
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x9c) = *reinterpret_cast<uint32_t*>(evalBuf + 0x04);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xa0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x08);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xa4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x0c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xa8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x10);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xac) =
                    gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectChromaticAberration)
                        ? *reinterpret_cast<uint32_t*>(evalBuf + 0x70)
                        : 0u;
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xb0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x74);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xb4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x78);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xbc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x80);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xb8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x7c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xc0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x84);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xc4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x14);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xc8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x18);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xcc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x1c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x248) = *reinterpret_cast<uint32_t*>(evalBuf + 0x20);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x54) = *reinterpret_cast<uint32_t*>(evalBuf + 0x2c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x58) = *reinterpret_cast<uint32_t*>(evalBuf + 0x30);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x5c) = *reinterpret_cast<uint32_t*>(evalBuf + 0x34);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x60) = *reinterpret_cast<uint32_t*>(evalBuf + 0x38);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xd0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x68);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0xd4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x6c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x250) = *reinterpret_cast<uint32_t*>(evalBuf + 0x88);
            }

            float w2 = *reinterpret_cast<float*>(viewBytes + 0x2b54);
            uint8_t* p2A = static_cast<uint8_t*>(sub_21e62d0(*reinterpret_cast<void**>(selfBytes + 0x5980), viewBytes + 0x2a90));
            uint8_t* p2B = static_cast<uint8_t*>(sub_21e62d0(*reinterpret_cast<void**>(selfBytes + 0x5980), viewBytes + 0x2af8));
            sub_e6d670(evalBuf);
            sub_e6d790(evalBuf, p2A + 0x10, p2B + 0x10, w2);
            sub_21d5050(pYebisSub, evalBuf);

            float w3 = *reinterpret_cast<float*>(viewBytes + 0x2b6c);
            uint8_t* p3A = static_cast<uint8_t*>(sub_21f68b0(*reinterpret_cast<void**>(selfBytes + 0x5988), viewBytes + 0x2ac0));
            uint8_t* p3B = static_cast<uint8_t*>(sub_21f68b0(*reinterpret_cast<void**>(selfBytes + 0x5988), viewBytes + 0x2b28));
            sub_e809e0(evalBuf, p3A + 0x10, p3B + 0x10, w3);
            if (pYebisState[0x3200] == 0) {
                const bool bloomOn = gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectBloom);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x130) =
                    bloomOn ? *reinterpret_cast<uint32_t*>(evalBuf + 0x00) : 0u;
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x144) = *reinterpret_cast<uint32_t*>(evalBuf + 0x20);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x148) = *reinterpret_cast<uint32_t*>(evalBuf + 0x2c);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x14c) = *reinterpret_cast<uint32_t*>(evalBuf + 0x28);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x150) = *reinterpret_cast<uint32_t*>(evalBuf + 0x24);
                std::memcpy(pGlobalPostCfg + 0x154, evalBuf + 0x30, 16);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x164) = *reinterpret_cast<uint32_t*>(evalBuf + 0x40);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x24c) = *reinterpret_cast<uint32_t*>(evalBuf + 0x44);
                std::memcpy(pGlobalPostCfg + 0x168, evalBuf + 0x48, 16);
                std::memcpy(pGlobalPostCfg + 0x134, evalBuf + 0x10, 16);
                std::memcpy(pGlobalPostCfg + 0x178, evalBuf + 0x60, 16);
                std::memcpy(pYebisState + 0x3170, evalBuf + 0x10, 16);
                std::memcpy(pYebisState + 0x3180, evalBuf + 0x10, 16);
                std::memcpy(pYebisState + 0x3190, evalBuf + 0x60, 16);
                std::memcpy(pYebisState + 0x31a0, evalBuf + 0x60, 16);
            }

            if (gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectDof)) {
                float w4 = *reinterpret_cast<float*>(viewBytes + 0x2b50);
                uint8_t* p4A = static_cast<uint8_t*>(sub_21ef2e0(*reinterpret_cast<void**>(selfBytes + 0x5990), viewBytes + 0x2a88));
                uint8_t* p4B = static_cast<uint8_t*>(sub_21ef2e0(*reinterpret_cast<void**>(selfBytes + 0x5990), viewBytes + 0x2af0));
                *reinterpret_cast<uint32_t*>(evalBuf + 0x00) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x04) = 0x40b33333u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x08) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x0c) = 0x422d1168u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x10) = 0x3f000000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x14) = 0x42340000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x18) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x1c) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x20) = 3u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x24) = 3u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x28) = 1u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x2c) = 0xffffffffu;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x30) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x34) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x38) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x3c) = 0x40000000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x40) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x44) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x48) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x4c) = 0x42c80000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x50) = 0x43160000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x54) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x58) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x5c) = 0x461c4000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x60) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x64) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x68) = 0x3f000000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x6c) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x70) = 0x3f800000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x74) = 0u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x78) = 0x459c4000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x7c) = 0x3f000000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x80) = 0x3f000000u;
                *reinterpret_cast<uint32_t*>(evalBuf + 0x84) = 0x3dcccccdu;

                sub_e75b50(evalBuf, p4A + 8, p4B + 8, w4);
                if (viewBytes[0x2bc5] != 0) {
                    *reinterpret_cast<float*>(evalBuf + 0x08) = *reinterpret_cast<float*>(viewBytes + 0x2bc8);
                    *reinterpret_cast<uint32_t*>(evalBuf + 0x30) = 0u;
                    *reinterpret_cast<uint32_t*>(evalBuf + 0x60) = 0u;
                }
                if (pYebisState[0x3200] == 0) {
                    uint32_t dofMode;
                    if (pYebisState[0x3202] != 0) {
                        dofMode = *reinterpret_cast<uint32_t*>(evalBuf + 0x60);
                        *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x188) = dofMode;
                    } else {
                        dofMode = *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x188);
                    }
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x18c) = *reinterpret_cast<uint32_t*>(evalBuf + 0x64);
                    std::memcpy(pGlobalPostCfg + 0x190, evalBuf + 0x68, 16);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1a0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x78);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1a4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x7c);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1a8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x80);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1ac) = *reinterpret_cast<uint32_t*>(evalBuf + 0x00);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1c0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x0c);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1b4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x04);
                    if (dofMode == 0 && pYebisState[0x3201] != 0) {
                        *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1b0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x08);
                    }
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1b8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x10);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1bc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x14);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1c4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x18);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1c8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x1c);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1cc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x28);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1d0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x20);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1d4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x24);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x240) = *reinterpret_cast<uint32_t*>(evalBuf + 0x2c);
                    uint32_t val30 = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540dd0ULL)) != 0) ? 0u : *reinterpret_cast<uint32_t*>(evalBuf + 0x30);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2d0) = val30;
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2d4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x34);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2d8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x48);
                    std::memcpy(pGlobalPostCfg + 0x2dc, evalBuf + 0x38, 16);
                    std::memcpy(pGlobalPostCfg + 0x2ec, evalBuf + 0x4c, 16);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2fc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x5c);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1d8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x84);
                }
            } else if (pYebisState[0x3200] == 0) {
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x188) = 0u;
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1ac) = 0u;
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2d0) = 0u;
            }

            float w5 = *reinterpret_cast<float*>(viewBytes + 0x2b64);
            uint8_t* p5A = static_cast<uint8_t*>(sub_21e14c0(*reinterpret_cast<void**>(selfBytes + 0x5998), viewBytes + 0x2ab0));
            uint8_t* p5B = static_cast<uint8_t*>(sub_21e14c0(*reinterpret_cast<void**>(selfBytes + 0x5998), viewBytes + 0x2b18));
            sub_e6c5d0(evalBuf, p5A + 8, p5B + 8, w5);
            if (pYebisState[0x3200] == 0) {
                uint32_t v0 = *reinterpret_cast<uint32_t*>(evalBuf + 0x00);
                uint32_t v4 = *reinterpret_cast<uint32_t*>(evalBuf + 0x04);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x220) =
                    (gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectGameAa) && v0 != 0 && v4 == 0) ? 1u : 0u;
            }

            if (gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectMotionBlur)) {
                float w6 = *reinterpret_cast<float*>(viewBytes + 0x2b60);
                uint8_t* p6A = static_cast<uint8_t*>(sub_21fa460(*reinterpret_cast<void**>(selfBytes + 0x59a0), viewBytes + 0x2aa8));
                uint8_t* p6B = static_cast<uint8_t*>(sub_21fa460(*reinterpret_cast<void**>(selfBytes + 0x59a0), viewBytes + 0x2b10));
                sub_e82290(evalBuf, p6A + 8, p6B + 8, w6);
                if (pYebisState[0x3200] == 0) {
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1dc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x00);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1e0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x04);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1e4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x08);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1e8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x0c);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x350) = *reinterpret_cast<uint32_t*>(evalBuf + 0x30);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1ec) = *reinterpret_cast<uint32_t*>(evalBuf + 0x10);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1f0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x14);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1f4) = *reinterpret_cast<uint32_t*>(evalBuf + 0x18);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1f8) = *reinterpret_cast<uint32_t*>(evalBuf + 0x1c);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1fc) = *reinterpret_cast<uint32_t*>(evalBuf + 0x20);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x200) = *reinterpret_cast<uint32_t*>(evalBuf + 0x24);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2ac) = *reinterpret_cast<uint32_t*>(evalBuf + 0x28);
                    *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x2b0) = *reinterpret_cast<uint32_t*>(evalBuf + 0x2c);
                    pGlobalPostCfg[0x204] = (*reinterpret_cast<uint32_t*>(evalBuf + 0x34) != 0) ? 1 : 0;
                }
            } else if (pYebisState[0x3200] == 0) {
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x1dc) = 0u;
                pGlobalPostCfg[0x204] = 0;
            }

            float w7 = *reinterpret_cast<float*>(viewBytes + 0x2b5c);
            uint8_t* p7A = static_cast<uint8_t*>(sub_21f2b90(*reinterpret_cast<void**>(selfBytes + 0x59a8), viewBytes + 0x2aa0));
            uint8_t* p7B = static_cast<uint8_t*>(sub_21f2b90(*reinterpret_cast<void**>(selfBytes + 0x59a8), viewBytes + 0x2b08));
            uint32_t vigBuf[8] = {
                0u, 0u, 0x3f000000u, 0u, 0u, 0x3f4ccccdu, 0x3f666666u, 0x40900000u
            };
            sub_e788f0(vigBuf, p7A + 8, p7B + 8, w7);
            if (pYebisState[0x3200] == 0) {
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x21c) = *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x208);
                *reinterpret_cast<uint32_t*>(pGlobalPostCfg + 0x208) = vigBuf[0];
                std::memcpy(pGlobalPostCfg + 0x20c, &vigBuf[1], 16);
            }

            float w8 = *reinterpret_cast<float*>(viewBytes + 0x2b58);
            uint8_t* p8A = static_cast<uint8_t*>(sub_21ea1e0(*reinterpret_cast<void**>(selfBytes + 0x59b0), viewBytes + 0x2a98));
            uint8_t* p8B = static_cast<uint8_t*>(sub_21ea1e0(*reinterpret_cast<void**>(selfBytes + 0x59b0), viewBytes + 0x2b00));
            sub_29356b0(evalBuf);
            sub_e713b0(evalBuf, p8A + 8, p8B + 8, w8);
            sub_21d5660(pYebisSub, evalBuf);

            resetInlineDLWString(evalBuf, 0x180, 0x221, true);
            resetInlineDLWString(evalBuf, 0x0d0, 0x171, true);
            resetInlineDLWString(evalBuf, 0x020, 0x0c1, false);

            void* pCtxSurf = *reinterpret_cast<void**>(passCtx + 0x508);
            if (pCtxSurf == nullptr) {
                pCtxSurf = *reinterpret_cast<void**>(passCtx + 0x3f8);
            }
            uint8_t* pRtSet = (*reinterpret_cast<uint8_t*>(GX_VMA(0x5540de0ULL)) != 0) ? (viewBytes + 0x5a8) : (viewBytes + 0x188);
            void* pOptData = nullptr;
            if (viewBytes[0x2969] != 0 && selfBytes[0x78b0] == 0) {
                pOptData = viewBytes + 0xce0;
            }
            sub_21d4860(pYebisSub, passCtx, 0x33, 8,
                        *reinterpret_cast<void**>(pRtSet + 0x28),
                        *reinterpret_cast<void**>(viewBytes + 0x160),
                        *reinterpret_cast<void**>(viewBytes + 0xf48),
                        pOptData, pYebisSub, pCtxSurf);
            sub_22d5480(passCtx);
        }

        void* pSubBlock130 = viewBytes + 0x130;
        sub_22d4d30(passCtx, pRenderDev, viewIdx, 0x3f, pCamera, nullptr,
                    *reinterpret_cast<uint32_t*>(viewBytes + 0x18),
                    *reinterpret_cast<uint32_t*>(viewBytes + 0x1c), 0, 0);

        passCtx[0x400] = 1;
        passCtx[0x401] = 1;
        passCtx[0x403] = 1;
        *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0x34;
        *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 5;

        void* pPkt2 = sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xf8, 8, 4);
        uint8_t bFlag2 = passCtx[0x401];
        if (pPkt2 != nullptr) {
            if (bFlag2 != 0) {
                CmdPacketHeader hdr{};
                hdr.packedWord = packCmdQueueHeader(*reinterpret_cast<uint64_t*>(passCtx + 0x110),
                                                    *reinterpret_cast<uint64_t*>(passCtx + 0x118));
                uint8_t bFlag403 = passCtx[0x403];
                hdr.bFlag0 = (bFlag403 != 0) ? 0 : (passCtx[0x402] != 0 ? 1 : 0);
                hdr.bFlag1 = bFlag403;
                hdr.pPayload = pPkt2;
                sub_2289ca0(*reinterpret_cast<void**>(passCtx + 0x450), &hdr);
                passCtx[0x401] = 0;
            }
            passCtx[0x400] = 0;
            passCtx[0x403] = 0;
            passCtx[0x402] = 0;
            sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
        } else {
            passCtx[0x400] = 0;
            passCtx[0x403] = 0;
            passCtx[0x402] = 0;
            if (bFlag2 == 0) {
                sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
            }
        }
        *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;

        uint8_t bHasPassA = 0;
        if (gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectGameAa)) {
            float wA = *reinterpret_cast<float*>(viewBytes + 0x2b64);
            uint8_t* pAA = static_cast<uint8_t*>(sub_21e14c0(*reinterpret_cast<void**>(selfBytes + 0x5998), viewBytes + 0x2ab0));
            uint8_t* pAB = static_cast<uint8_t*>(sub_21e14c0(*reinterpret_cast<void**>(selfBytes + 0x5998), viewBytes + 0x2b18));
            sub_e6c5d0(evalBuf, pAA + 8, pAB + 8, wA);
            if ((viewBytes[0x49] & 0x04) != 0) {
                uint32_t v4 = *reinterpret_cast<uint32_t*>(evalBuf + 0x04);
                sub_e596f0(viewBytes + 0x1bc8, evalBuf);
                if (v4 != 0) {
                    sub_e58c50(viewBytes + 0x1bc8, passCtx, 0x34, 9, pSubBlock130);
                }
            } else {
                sub_e596f0(viewBytes + 0x1bc8, evalBuf);
                sub_e58c50(viewBytes + 0x1bc8, passCtx, 0x34, 9, pSubBlock130);
            }
            bHasPassA = (viewBytes[0x1bd1] != 0) ? 1 : 0;
        }

        if (viewBytes[0x296b] != 0 && gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectBloom)) {
            float wB = *reinterpret_cast<float*>(viewBytes + 0x2b74);
            uint8_t* pBA = static_cast<uint8_t*>(sub_e74bd0(*reinterpret_cast<void**>(selfBytes + 0x59c0), viewBytes + 0x2ad0));
            uint8_t* pBB = static_cast<uint8_t*>(sub_e74bd0(*reinterpret_cast<void**>(selfBytes + 0x59c0), viewBytes + 0x2b38));
            std::memset(evalBuf, 0, 0x60);
            *reinterpret_cast<uint32_t*>(evalBuf + 0x20) = 0x3e19999au;
            *reinterpret_cast<uint32_t*>(evalBuf + 0x50) = 0x3f100000u;
            sub_e72620(evalBuf, pBA + 0x10, pBB + 0x10, wB);
            sub_e5ab20(viewBytes + 0x2430, evalBuf);
            sub_e5a010(viewBytes + 0x2430, passCtx, 0x34, 0x0a, pSubBlock130);
        }

        if (viewBytes[0x2979] != 0 && gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectBloom)) {
            float wC = *reinterpret_cast<float*>(viewBytes + 0x2b70);
            uint8_t* pCA = static_cast<uint8_t*>(sub_e7b3b0(*reinterpret_cast<void**>(selfBytes + 0x59b8), viewBytes + 0x2ac8));
            uint8_t* pCB = static_cast<uint8_t*>(sub_e7b3b0(*reinterpret_cast<void**>(selfBytes + 0x59b8), viewBytes + 0x2b30));
            uint32_t cBuf[4] = { 0u, 0x41a00000u, 0x40400000u, 0x3f800000u };
            sub_e79650(cBuf, pCA + 8, pCB + 8, wC);
            sub_e5c680(viewBytes + 0x1c20, cBuf);
            uint32_t mode68 = *reinterpret_cast<uint32_t*>(viewBytes + 0x68);
            uint8_t* pSlotTex = (mode68 == 2) ? (viewBytes + 0x578) : (viewBytes + 0x3c0);
            sub_e5bb70(viewBytes + 0x1c20, passCtx, 0x34, 0x0b,
                       *reinterpret_cast<void**>(pSlotTex), bHasPassA, pSubBlock130, mode68);
        }

        sub_22d5480(passCtx);
    }

    sub_ea57c0(sceneQuery);
}

// ============================================================================
// 0x229e970: sub_229e970 (22 bytes)
// Enqueues sub_22b0490 task callback into the render worker queue
// ============================================================================
void sub_229e970(void* /*pSelf*/, void* pTaskQueue) {
    sub_228e7a0(pTaskQueue, reinterpret_cast<void*>(GX_VMA(0x22b0490ULL)), 0, 0);
}

} // extern "C"

namespace {

static inline void invertMatrix4x4SIMD(const __m128* src, __m128* dst) {
    __m128 r0 = src[0];
    __m128 r1 = src[1];
    __m128 r2 = src[2];
    __m128 r3 = src[3];

    __m128 t0 = _mm_movelh_ps(r0, r1);
    __m128 t1 = _mm_movehl_ps(r1, r0);
    __m128 t2 = _mm_movelh_ps(r2, r3);
    __m128 t3 = _mm_movehl_ps(r3, r2);

    __m128 c0 = _mm_shuffle_ps(t0, t2, 0x88);
    __m128 c1 = _mm_shuffle_ps(t0, t2, 0xdd);
    __m128 c2 = _mm_shuffle_ps(t1, t3, 0x88);
    __m128 c3 = _mm_shuffle_ps(t1, t3, 0xdd);

    __m128 p0 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x50), _mm_movehl_ps(c3, c3)),
        _mm_mul_ps(_mm_movehl_ps(c2, c2), _mm_shuffle_ps(c3, c3, 0x50)));
    __m128 p1 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x50), _mm_movehl_ps(c1, c1)),
        _mm_mul_ps(_mm_movehl_ps(c0, c0), _mm_shuffle_ps(c1, c1, 0x50)));
    __m128 p2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c2, c0, 0x88), _mm_shuffle_ps(c3, c1, 0xdd)),
        _mm_mul_ps(_mm_shuffle_ps(c2, c0, 0xdd), _mm_shuffle_ps(c3, c1, 0x88)));

    __m128 s0 = _mm_shuffle_ps(p0, p2, 0x5d);
    __m128 s1 = _mm_shuffle_ps(p1, p2, 0xfd);
    __m128 s2 = _mm_shuffle_ps(p0, p2, 0x04);
    __m128 s3 = _mm_shuffle_ps(p1, p2, 0xa4);

    __m128 m0 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c1, c1, 0x49), _mm_shuffle_ps(s0, p0, 0x32)),
        _mm_mul_ps(_mm_shuffle_ps(c1, c1, 0x9e), _mm_shuffle_ps(p0, s2, 0x93)));
    __m128 m1 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x12), _mm_shuffle_ps(s0, p0, 0x99)),
        _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x7b), _mm_shuffle_ps(p0, s2, 0x26)));
    __m128 m2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c3, c3, 0x49), _mm_shuffle_ps(s1, p1, 0x32)),
        _mm_mul_ps(_mm_shuffle_ps(c3, c3, 0x9e), _mm_shuffle_ps(p1, s3, 0x93)));
    __m128 m3 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x12), _mm_shuffle_ps(s1, p1, 0x99)),
        _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x7b), _mm_shuffle_ps(p1, s3, 0x26)));

    __m128 u0 = _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(p2), _mm_castps_si128(p0), 8));
    __m128 u1 = _mm_shuffle_ps(p0, p2, 0x4c);
    __m128 u2 = _mm_movehl_ps(p2, p1);
    __m128 u3 = _mm_shuffle_ps(p1, p2, 0xec);

    __m128 q0 = _mm_mul_ps(_mm_shuffle_ps(c1, c1, 0x33), _mm_shuffle_ps(u0, u0, 0x2c));
    __m128 q1 = _mm_mul_ps(_mm_shuffle_ps(c0, c0, 0x8d), _mm_shuffle_ps(u1, u1, 0x93));
    __m128 q2 = _mm_mul_ps(_mm_shuffle_ps(c3, c3, 0x33), _mm_shuffle_ps(u2, u2, 0x2c));
    __m128 q3 = _mm_mul_ps(_mm_shuffle_ps(c2, c2, 0x8d), _mm_shuffle_ps(u3, u3, 0x93));

    __m128 a0 = _mm_shuffle_ps(_mm_add_ps(q0, m0), _mm_sub_ps(m0, q0), 0xd8);
    __m128 a1 = _mm_shuffle_ps(_mm_sub_ps(m1, q1), _mm_add_ps(q1, m1), 0xd8);
    __m128 a2 = _mm_shuffle_ps(_mm_add_ps(q2, m2), _mm_sub_ps(m2, q2), 0xd8);
    __m128 a3 = _mm_shuffle_ps(_mm_sub_ps(m3, q3), _mm_add_ps(q3, m3), 0xd8);

    a0 = _mm_shuffle_ps(a0, a0, 0xd8);
    a1 = _mm_shuffle_ps(a1, a1, 0xd8);
    a2 = _mm_shuffle_ps(a2, a2, 0xd8);
    a3 = _mm_shuffle_ps(a3, a3, 0xd8);

    __m128 det = _mm_mul_ps(c0, a0);
    det = _mm_hadd_ps(det, det);
    det = _mm_hadd_ps(det, det);
    __m128 invDet = _mm_div_ps(_mm_set1_ps(1.0f), det);

    a0 = _mm_mul_ps(invDet, a0);
    a1 = _mm_mul_ps(invDet, a1);
    a2 = _mm_mul_ps(invDet, a2);
    a3 = _mm_mul_ps(invDet, a3);

    dst[0] = a0;
    dst[1] = a1;
    dst[2] = a2;
    dst[3] = a3;
}

static inline void releaseRefCountedSlot(uint8_t* pOwner, size_t slotOffset) {
    uint8_t* pObj = *reinterpret_cast<uint8_t**>(pOwner + slotOffset);
    if (pObj == nullptr) {
        return;
    }
    void* vtbl0 = *reinterpret_cast<void**>(pObj);
    (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl0) + 0x18))(pObj);
    pObj[0x0b] &= 0xbfu;

    pObj = *reinterpret_cast<uint8_t**>(pOwner + slotOffset);
    if (pObj == nullptr) {
        return;
    }
    uint32_t oldRef = *reinterpret_cast<uint32_t*>(pObj + 0x08);
    uint32_t decStrong = oldRef + 0x7fffu;
    uint32_t newStrong = (oldRef & 0xffff8000u) | (decStrong & 0x7fffu);
    *reinterpret_cast<uint32_t*>(pObj + 0x08) = newStrong;
    if ((decStrong & 0x7fffu) == 0 && (oldRef & 0x40000000u) != 0) {
        void* vtbl1 = *reinterpret_cast<void**>(pObj);
        (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl1) + 0x18))(pObj);
        newStrong = *reinterpret_cast<uint32_t*>(pObj + 0x08) & 0xbfffffffu;
        *reinterpret_cast<uint32_t*>(pObj + 0x08) = newStrong;
    }
    uint32_t decWeak = (newStrong >> 15) + 0x7fffu;
    uint32_t newWeak = (newStrong & 0xc0007fffu) | ((decWeak & 0x7fffu) << 15);
    *reinterpret_cast<uint32_t*>(pObj + 0x08) = newWeak;
    if ((decWeak & 0x7fffu) == 0) {
        *reinterpret_cast<uint32_t*>(pObj + 0x08) = newStrong | 0x3fff8000u;
        void* vtbl2 = *reinterpret_cast<void**>(pObj);
        (*reinterpret_cast<void (**)(void*)>(static_cast<uint8_t*>(vtbl2) + 0x20))(pObj);
    }
    *reinterpret_cast<void**>(pOwner + slotOffset) = nullptr;
}

} // namespace

extern "C" {

void sub_229d2a0(void* pSelf, uint32_t maxSlotIndex);
void sub_21aaa90(void* pScope, void* pMatSrc);
void sub_2168a20(void* pDrawCtxImpl, void* pStateSlot, void* pCBufferObj,
                 uint32_t arg4, uint32_t arg5, const void* pData,
                 uint32_t arg7, uint32_t arg8);
void sub_22900a0(void* pPassCtx, uint32_t clearFlags, const void* pClearColor);
void* sub_226a3a0(void* pRepo, const void* pId);
void sub_226c600(void* pOutShadowCfg, const void* pSrcA, const void* pSrcB, float weight);
void* sub_22e6740(void* pLightRepo, const void* pId);
float sub_2bbe518(uint32_t trigMode, uint32_t unusedFlag, float angleRad);
void sub_eac210(void* pCsmMgr, const void* pShadowCfg);
void sub_eac540(void* pCsmMgr, void* pOutCascadeCam, const void* pBaseCam,
                const void* pLightDir, uint32_t cascadeIdx);
void sub_22d4fd0(void* pPassCtx, void* pRenderDev, void* pCmdQueue,
                 uint32_t viewIdx, uint32_t passIndex1Based, void* pCascadeCam,
                 void* pAltCam, uint32_t w, uint32_t h, uint32_t f0, uint32_t f1);
void sub_eac680(void* pCsmMgr, void* pPassCtx, const void* pLightDir, uint32_t cascadeIdx);
void sub_eaaeb0(void* pMgr, void* pSelf, void* pArg3, void* pArg4,
                void* pArg5, void* pArg6, void* pArg7);
void sub_22bf750(void* pVecBase, uint64_t newCap);
void sub_228b870(void* pObj, void* pAllocator);
void sub_e464d0(void* pSubObj);
void sub_221b8c0(void* pSubObj, float dt);
void sub_e30ec0(void* pSubObj);
void sub_22c5a60(void* pTarget, uint32_t flag);
void sub_22c58e0(void* pTarget, const void* pDesc, void* pOwner, uint32_t flag);
void sub_21d2830(void* pYebisSub);
void sub_21d2dd0(void* pYebisSub);
void* sub_207b720(void* pAllocHolder);
void sub_229a680(void* pSelf, void* pArg2);

// ============================================================================
// 0x229e480: sub_229e480 (1,249 bytes)
// Transforms plane/vector at pSrc+0x2a30 by inv(pSelf+0x10) and inv(pSelf+0x50)
// into pSelf+0x710
// ============================================================================
void sub_229e480(void* pSelf, const void* pSrc) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    const uint8_t* srcBytes = static_cast<const uint8_t*>(pSrc);

    __m128 m1[2] = {
        _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x50)),
        _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x60))
    };
    __m128 m1_hi[2] = {
        _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x70)),
        _mm_load_ps(reinterpret_cast<const float*>(selfBytes + 0x80))
    };

    __m128 inv0[4];
    invertMatrix4x4SIMD(reinterpret_cast<const __m128*>(selfBytes + 0x10), inv0);

    __m128 v = _mm_load_ps(reinterpret_cast<const float*>(srcBytes + 0x2a30));
    __m128 v0 = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), inv0[0]),
                   _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), inv0[1])),
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xaa), inv0[2]),
                   _mm_mul_ps(_mm_shuffle_ps(v, v, 0xff), inv0[3])));

    __m128 src1[4] = { m1[0], m1[1], m1_hi[0], m1_hi[1] };
    __m128 inv1[4];
    invertMatrix4x4SIMD(src1, inv1);

    __m128 v1 = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v0, v0, 0x00), inv1[0]),
                   _mm_mul_ps(_mm_shuffle_ps(v0, v0, 0x55), inv1[1])),
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v0, v0, 0xaa), inv1[2]),
                   _mm_mul_ps(_mm_shuffle_ps(v0, v0, 0xff), inv1[3])));

    _mm_store_ps(reinterpret_cast<float*>(selfBytes + 0x710), v1);
}

// ============================================================================
// 0x229e0c0: sub_229e0c0 (946 bytes)
// Uploads camera matrix and its 4x4 inverse to constant buffer slot 11
// ============================================================================
void sub_229e0c0(void* pCtx) {
    uint8_t* ctxBytes = static_cast<uint8_t*>(pCtx);
    uint8_t* pGlobal = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* pMatSrc = *reinterpret_cast<uint8_t**>(ctxBytes + 0x3f8);
    uint8_t* pDrawCtx = *reinterpret_cast<uint8_t**>(ctxBytes + 0x430);

    alignas(16) uint8_t localScope[0x48];
    sub_21aaa90(localScope, pMatSrc);

    uint8_t* pSub6070 = *reinterpret_cast<uint8_t**>(pGlobal + 0x6070);
    uint8_t* pCBufferObj = *reinterpret_cast<uint8_t**>(pSub6070 + 0x230);

    alignas(16) __m128 cbufData[8];
    cbufData[0] = _mm_load_ps(reinterpret_cast<const float*>(pMatSrc + 0x00));
    cbufData[1] = _mm_load_ps(reinterpret_cast<const float*>(pMatSrc + 0x10));
    cbufData[2] = _mm_load_ps(reinterpret_cast<const float*>(pMatSrc + 0x20));
    cbufData[3] = _mm_load_ps(reinterpret_cast<const float*>(pMatSrc + 0x30));
    invertMatrix4x4SIMD(cbufData, &cbufData[4]);

    uint8_t* pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x08);
    sub_2168a20(pDrawImpl, pDrawImpl + 0xb730, pCBufferObj, 0, 0, cbufData, 0, 0);

    *reinterpret_cast<void**>(pDrawCtx + 0x6d8) = pCBufferObj;
    uint32_t curMin = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f0) = (curMin < 11u) ? curMin : 11u;
    uint32_t curMax = *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4);
    *reinterpret_cast<uint32_t*>(pDrawCtx + 0x6f4) = (curMax > 12u) ? curMax : 12u;

    pDrawImpl = *reinterpret_cast<uint8_t**>(pDrawCtx + 0x08);
    *reinterpret_cast<uint64_t*>(pDrawImpl + 0x78) |= 0x8000000ULL;

    uint8_t* pRes = static_cast<uint8_t*>(pCBufferObj);
    if (pRes != nullptr) {
        const uint32_t reqMask = 0x10800u;
        uint32_t curFlags = *reinterpret_cast<uint32_t*>(pRes + 0x44);
        if ((curFlags & reqMask) != reqMask) {
            __sync_fetch_and_or(reinterpret_cast<uint32_t*>(pRes + 0x44), reqMask);
        }
    }
    *reinterpret_cast<void**>(pDrawImpl + 0xc1a0) = pCBufferObj;
}

// ============================================================================
// 0x229d2b0: sub_229d2b0 (3,597 bytes)
// Planar Reflection Camera Mirroring and Pass 0x35 Submission
// ============================================================================
void sub_229d2b0(void* pSelf, void* pRenderDev, const void* pCamera,
                 void* pArg4, void* pViewCtx, uint32_t viewIdx,
                 uint32_t /*unusedStackArg0*/, void* pUserArg) {
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8[0x3a9] != 0) {
        return;
    }
    uint8_t* viewBytes = static_cast<uint8_t*>(pViewCtx);
    uint8_t* pReflView = *reinterpret_cast<uint8_t**>(viewBytes + 0x120);
    if (pReflView == nullptr || pReflView[0x2a1c] == 0) {
        return;
    }

    __m128 reflPlane = _mm_load_ps(reinterpret_cast<const float*>(pReflView + 0x2a30));

    alignas(16) uint8_t reflCam[0x90];
    const float* pDefaultMat = reinterpret_cast<const float*>(GX_VMA(0x5128a40ULL));
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x00), _mm_load_ps(pDefaultMat + 0));
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x10), _mm_load_ps(pDefaultMat + 4));
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x20), _mm_load_ps(pDefaultMat + 8));
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x30), _mm_load_ps(pDefaultMat + 12));

    *reinterpret_cast<uint32_t*>(reflCam + 0x50) = 0x3f75be0bu;
    *reinterpret_cast<uint32_t*>(reflCam + 0x54) = 0x3dcccccdu;
    *reinterpret_cast<uint32_t*>(reflCam + 0x58) = 0x459c4000u;
    *reinterpret_cast<uint32_t*>(reflCam + 0x64) = 0u;
    if (pGlobal6c8 != nullptr) {
        uint8_t* pScreenCfg = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x90);
        *reinterpret_cast<uint32_t*>(reflCam + 0x5c) = *reinterpret_cast<uint32_t*>(pScreenCfg + 0x80);
        *reinterpret_cast<uint32_t*>(reflCam + 0x60) = *reinterpret_cast<uint32_t*>(pScreenCfg + 0x84);
    } else {
        *reinterpret_cast<uint32_t*>(reflCam + 0x5c) = 0u;
        *reinterpret_cast<uint32_t*>(reflCam + 0x60) = 0u;
    }
    reflCam[0x68] = 0;
    *reinterpret_cast<uint64_t*>(reflCam + 0x6c) = 0ULL;
    *reinterpret_cast<uint64_t*>(reflCam + 0x74) = 0ULL;
    *reinterpret_cast<uint64_t*>(reflCam + 0x7c) = 0ULL;

    const uint8_t* camBytes = static_cast<const uint8_t*>(pCamera);
    __m128 camCols[4];
    invertMatrix4x4SIMD(reinterpret_cast<const __m128*>(camBytes), camCols);

    __m128 invCol0 = camCols[0];
    __m128 invCol1 = camCols[1];
    __m128 invCol2 = camCols[2];
    __m128 invCol3 = camCols[3];

    // Normalize 3D normal of reflection plane
    __m128 sq = _mm_mul_ps(reflPlane, reflPlane);
    __m128 lenSq = _mm_add_ss(_mm_movehl_ps(sq, sq), _mm_add_ss(sq, _mm_shuffle_ps(sq, sq, 0x01)));
    lenSq = _mm_shuffle_ps(lenSq, lenSq, 0x00);
    __m128 rsqrt = _mm_rsqrt_ps(lenSq);
    __m128 zero = _mm_setzero_ps();
    __m128 isZero = _mm_or_ps(_mm_cmpeq_ps(lenSq, zero), _mm_cmpeq_ps(rsqrt, zero));
    __m128 half = _mm_set1_ps(0.5f);
    __m128 three = _mm_set1_ps(3.0f);
    rsqrt = _mm_mul_ps(_mm_mul_ps(rsqrt, half), _mm_sub_ps(three, _mm_mul_ps(rsqrt, _mm_mul_ps(rsqrt, lenSq))));
    rsqrt = _mm_mul_ps(_mm_mul_ps(rsqrt, half), _mm_sub_ps(three, _mm_mul_ps(rsqrt, _mm_mul_ps(lenSq, rsqrt))));
    __m128 len = _mm_blendv_ps(_mm_mul_ps(lenSq, rsqrt), lenSq, isZero);
    __m128 notInf = _mm_cmpneq_ps(lenSq, _mm_castsi128_ps(_mm_set1_epi32(0x7f800000)));
    __m128 normPlane = _mm_and_ps(_mm_div_ps(reflPlane, len), notInf);

    __m128 neg2Plane = _mm_mul_ps(normPlane, _mm_setr_ps(-2.0f, -2.0f, -2.0f, 0.0f));
    __m128 rRow0 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normPlane, normPlane, 0x00), neg2Plane),
                              _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f));
    __m128 rRow1 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normPlane, normPlane, 0x55), neg2Plane),
                              _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f));
    __m128 rRow2 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normPlane, normPlane, 0xaa), neg2Plane),
                              _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f));
    __m128 rRow3 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normPlane, normPlane, 0xff), neg2Plane),
                              _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

    auto mulCols = [&](__m128 vRow) -> __m128 {
        return _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(invCol0, _mm_shuffle_ps(vRow, vRow, 0x00)),
                       _mm_mul_ps(invCol2, _mm_shuffle_ps(vRow, vRow, 0xaa))),
            _mm_add_ps(_mm_mul_ps(invCol1, _mm_shuffle_ps(vRow, vRow, 0x55)),
                       _mm_mul_ps(invCol3, _mm_shuffle_ps(vRow, vRow, 0xff))));
    };

    __m128 mRow0 = mulCols(rRow0);
    __m128 mRow1 = mulCols(rRow1);
    __m128 mRow2 = mulCols(rRow2);
    __m128 mRow3 = mulCols(rRow3);

    __m128 axisX = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
    __m128 axisNegY = _mm_setr_ps(0.0f, -1.0f, 0.0f, 0.0f);
    __m128 axisZ = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    __m128 axisW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    auto flipY = [&](__m128 vRow) -> __m128 {
        return _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vRow, vRow, 0x00), axisX),
                       _mm_mul_ps(_mm_shuffle_ps(vRow, vRow, 0xaa), axisZ)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vRow, vRow, 0x55), axisNegY),
                       _mm_mul_ps(_mm_shuffle_ps(vRow, vRow, 0xff), axisW)));
    };

    __m128 fMat[4] = { flipY(mRow0), flipY(mRow1), flipY(mRow2), flipY(mRow3) };
    __m128 fInvTrans[4];
    invertMatrix4x4SIMD(fMat, fInvTrans);

    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x40),
                 _mm_load_ps(reinterpret_cast<const float*>(camBytes + 0x40)));
    *reinterpret_cast<float*>(reflCam + 0x50) = *reinterpret_cast<const float*>(camBytes + 0x50);
    *reinterpret_cast<float*>(reflCam + 0x54) = *reinterpret_cast<const float*>(camBytes + 0x54);
    *reinterpret_cast<float*>(reflCam + 0x58) = *reinterpret_cast<const float*>(camBytes + 0x58);
    *reinterpret_cast<uint32_t*>(reflCam + 0x5c) = *reinterpret_cast<const uint32_t*>(camBytes + 0x5c);
    *reinterpret_cast<uint32_t*>(reflCam + 0x60) = *reinterpret_cast<const uint32_t*>(camBytes + 0x60);
    *reinterpret_cast<float*>(reflCam + 0x64) = *reinterpret_cast<const float*>(camBytes + 0x64);
    *reinterpret_cast<float*>(reflCam + 0x6c) = *reinterpret_cast<const float*>(camBytes + 0x6c);
    std::memcpy(reflCam + 0x70, camBytes + 0x70, 16);
    *reinterpret_cast<uint32_t*>(reflCam + 0x80) = *reinterpret_cast<const uint32_t*>(camBytes + 0x80);

    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x00), fInvTrans[0]);
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x10), fInvTrans[1]);
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x20), fInvTrans[2]);
    _mm_store_ps(reinterpret_cast<float*>(reflCam + 0x30), fInvTrans[3]);

    alignas(16) uint8_t passCtx[0x870];
    std::memset(passCtx, 0, sizeof(passCtx));
    sub_22d4d30(passCtx, pRenderDev, viewIdx, 0x35, reflCam, nullptr, 0, 0, 0, 0);

    passCtx[0x400] = 1;
    passCtx[0x401] = 1;
    passCtx[0x403] = 1;
    *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0;
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;

    auto allocCallbackSlot = [&]() -> uint8_t* {
        uint8_t* pPkt = static_cast<uint8_t*>(sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 4, 0x18, 4));
        if (pPkt == nullptr) {
            return nullptr;
        }
        if (passCtx[0x401] != 0) {
            CmdPacketHeader hdr{};
            hdr.packedWord = packCmdQueueHeader(*reinterpret_cast<uint64_t*>(passCtx + 0x110),
                                                *reinterpret_cast<uint64_t*>(passCtx + 0x118));
            uint8_t bFlag403 = passCtx[0x403];
            hdr.bFlag0 = (bFlag403 != 0) ? 0 : (passCtx[0x402] != 0 ? 1 : 0);
            hdr.bFlag1 = bFlag403;
            hdr.pPayload = pPkt;
            sub_2289ca0(*reinterpret_cast<void**>(passCtx + 0x450), &hdr);
            passCtx[0x401] = 0;
        }
        return pPkt + pPkt[1] + 4;
    };

    uint8_t* pSlot0 = allocCallbackSlot();
    if (pSlot0 != nullptr) {
        *reinterpret_cast<uint64_t*>(pSlot0) = GX_VMA(0x229e0c0ULL);
        *reinterpret_cast<void**>(pSlot0 + 0x08) = pReflView;
        *reinterpret_cast<uint64_t*>(pSlot0 + 0x10) = 0;
    }

    uint8_t* pSlot1 = allocCallbackSlot();
    if (pSlot1 != nullptr) {
        *reinterpret_cast<uint64_t*>(pSlot1) = GX_VMA(0x229e480ULL);
        *reinterpret_cast<void**>(pSlot1 + 0x08) = pReflView;
        *reinterpret_cast<uint64_t*>(pSlot1 + 0x10) = 0;
    }

    uintptr_t val70 = *reinterpret_cast<uint32_t*>(pReflView + 0x70);
    uint8_t* pSlot2 = allocCallbackSlot();
    if (pSlot2 != nullptr) {
        *reinterpret_cast<uint64_t*>(pSlot2) = GX_VMA(0x229d2a0ULL);
        *reinterpret_cast<uintptr_t*>(pSlot2 + 0x08) = val70;
        *reinterpret_cast<uint64_t*>(pSlot2 + 0x10) = 0;
    }

    bool bWasUncommitted = (passCtx[0x401] == 0);
    uint8_t* pSlot3 = allocCallbackSlot();
    if (pSlot3 != nullptr) {
        *reinterpret_cast<uint64_t*>(pSlot3) = GX_VMA(0x22b0490ULL);
        *reinterpret_cast<uint64_t*>(pSlot3 + 0x08) = 0;
        *reinterpret_cast<uint64_t*>(pSlot3 + 0x10) = 0;
        bWasUncommitted = true;
    }
    passCtx[0x400] = 0;
    passCtx[0x403] = 0;
    passCtx[0x402] = 0;
    if (bWasUncommitted) {
        sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
    }

    passCtx[0x400] = 1;
    passCtx[0x401] = 1;
    passCtx[0x403] = 1;
    *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0;
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 5;

    void* pPktF8 = sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xf8, 8, 4);
    uint8_t bFlagF8 = passCtx[0x401];
    if (pPktF8 != nullptr) {
        if (bFlagF8 != 0) {
            CmdPacketHeader hdr{};
            hdr.packedWord = packCmdQueueHeader(*reinterpret_cast<uint64_t*>(passCtx + 0x110),
                                                *reinterpret_cast<uint64_t*>(passCtx + 0x118));
            uint8_t bFlag403 = passCtx[0x403];
            hdr.bFlag0 = (bFlag403 != 0) ? 0 : (passCtx[0x402] != 0 ? 1 : 0);
            hdr.bFlag1 = bFlag403;
            hdr.pPayload = pPktF8;
            sub_2289ca0(*reinterpret_cast<void**>(passCtx + 0x450), &hdr);
            passCtx[0x401] = 0;
        }
        passCtx[0x400] = 0;
        passCtx[0x403] = 0;
        passCtx[0x402] = 0;
        sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
    } else {
        passCtx[0x400] = 0;
        passCtx[0x403] = 0;
        passCtx[0x402] = 0;
        if (bFlagF8 == 0) {
            sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
        }
    }

    passCtx[0x400] = 1;
    passCtx[0x401] = 1;
    passCtx[0x403] = 1;
    *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0x3f;
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;

    alignas(16) float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    sub_22900a0(passCtx, 0, clearColor);

    passCtx[0x400] = 0;
    passCtx[0x403] = 0;
    passCtx[0x402] = 0;
    if (passCtx[0x401] == 0) {
        sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
    }
    *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;

    sub_229e990(pSelf, pRenderDev, reflCam, reflCam, pArg4, pReflView, viewIdx, 0x35, pUserArg);
    sub_22d5480(passCtx);
}

// ============================================================================
// 0x229d2a0: sub_229d2a0 (7 bytes)
// Sets max constant buffer slot index at pSelf + 0x6f4
// ============================================================================
void sub_229d2a0(void* pSelf, uint32_t val) {
    *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(pSelf) + 0x6f4) = val;
}

// ============================================================================
// 0x229cb70: sub_229cb70 (1,829 bytes)
// 4-Cascade Directional Shadow Map (CSM) Evaluation & Rendering Loop
// ============================================================================
void sub_229cb70(void* pSelf, void* pRenderDevHolder, const void* pCamera,
                 void* pArg4, void* pViewCtx, uint32_t viewIdx) {
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (pGlobal6c8[0x3a9] != 0) {
        return;
    }
    uint8_t* viewBytes = static_cast<uint8_t*>(pViewCtx);
    if (viewBytes[0x296c] == 0) {
        return;
    }

    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    alignas(16) __m128 lightDir = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);

    uint8_t* pShadowCfg = viewBytes + 0x80;
    if (viewBytes[0x297c] == 0) {
        uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
        void* pRepo = *reinterpret_cast<void**>(pGlobalDd8 + 0x59d0);
        uint8_t* pA = static_cast<uint8_t*>(sub_226a3a0(pRepo, viewBytes + 0x2a48));
        uint8_t* pB = static_cast<uint8_t*>(sub_226a3a0(pRepo, viewBytes + 0x2a50));
        sub_226c600(pShadowCfg, pA + 0x10, pB + 0x10, *reinterpret_cast<float*>(viewBytes + 0x2a58));
    }

    uint64_t idA = *reinterpret_cast<uint64_t*>(pShadowCfg + 0x20);
    uint64_t idB = *reinterpret_cast<uint64_t*>(pShadowCfg + 0x28);
    const uint64_t kInvalidLimit = 0xfffffffeffffffffULL;

    if (idA > kInvalidLimit || static_cast<uint32_t>(idA) == 0xffffffffu) {
        lightDir = _mm_load_ps(reinterpret_cast<const float*>(pShadowCfg + 0x10));
    } else {
        uint8_t* pLightA = static_cast<uint8_t*>(sub_22e6740(*reinterpret_cast<void**>(selfBytes + 0x5880), &idA));
        if (idB > kInvalidLimit || static_cast<uint32_t>(idB) == 0xffffffffu) {
            lightDir = _mm_load_ps(reinterpret_cast<const float*>(pLightA + 0x40));
        } else {
            float pitchA = *reinterpret_cast<float*>(pLightA + 0x70);
            float yawA = *reinterpret_cast<float*>(pLightA + 0x74);
            uint8_t* pLightB = static_cast<uint8_t*>(sub_22e6740(*reinterpret_cast<void**>(selfBytes + 0x5880), &idB));
            float pitchB = *reinterpret_cast<float*>(pLightB + 0x70);
            float yawB = *reinterpret_cast<float*>(pLightB + 0x74);
            float t = *reinterpret_cast<float*>(pShadowCfg + 0x30);

            const float kPi = 3.1415927f;
            const float kNegHalfPi = -1.5707964f;
            const float kTwoPi = 6.2831855f;
            const float kNegPi = -3.1415927f;

            if (kNegHalfPi > pitchA) pitchA += kPi;
            if (kNegHalfPi > pitchB) pitchB += kPi;
            if (std::fabs(pitchA - pitchB) >= kPi) {
                if (pitchB > pitchA) pitchB -= kPi;
                else pitchB += kPi;
            }

            if (kNegPi > yawA) yawA += kTwoPi;
            if (kNegPi > yawB) yawB += kTwoPi;
            if (std::fabs(yawA - yawB) >= kTwoPi) {
                if (yawB > yawA) yawB -= kTwoPi;
                else yawB += kTwoPi;
            }

            float pitch = pitchA + t * (pitchB - pitchA);
            float yaw = yawA + t * (yawB - yawA);

            float sinPitch = sub_2bbe518(0, 0, pitch);
            float cosPitch = sub_2bbe518(1, 0, pitch);
            __m128 vPitch = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(cosPitch), _mm_set_ss(sinPitch), 0xc3),
                _mm_setr_ps(1.0f, 1.0f, -1.0f, 1.0f));

            float sinYaw = sub_2bbe518(0, 0, yaw);
            float cosYaw = sub_2bbe518(1, 0, yaw);
            __m128 vYaw = _mm_shuffle_ps(_mm_set_ss(sinYaw), _mm_set_ss(cosYaw), 0xcc);
            __m128 vYawNeg = _mm_shuffle_ps(
                _mm_mul_ps(vYaw, _mm_setr_ps(-1.0f, 1.0f, 1.0f, 1.0f)),
                _mm_mul_ps(vYaw, _mm_setr_ps(-1.0f, 1.0f, 1.0f, 1.0f)),
                0xc6);

            __m128 rawDir = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vPitch, vPitch, 0xaa), _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f)),
                           _mm_mul_ps(_mm_shuffle_ps(vPitch, vPitch, 0xff), _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f))),
                _mm_add_ps(_mm_mul_ps(vYaw, _mm_shuffle_ps(vPitch, vPitch, 0x55)),
                           _mm_mul_ps(_mm_shuffle_ps(vPitch, vPitch, 0x00), vYawNeg)));

            rawDir = _mm_and_ps(rawDir, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
            __m128 sq = _mm_mul_ps(rawDir, rawDir);
            __m128 hi = _mm_movehl_ps(sq, sq);
            __m128 sum = _mm_add_ps(sq, hi);
            sum = _mm_shuffle_ps(sum, sum, 0x40);
            sum = _mm_add_ps(sum, _mm_shuffle_ps(hi, sum, 0xf0));
            sum = _mm_shuffle_ps(sum, sum, 0xaa);

            __m128 rsqrt = _mm_rsqrt_ps(sum);
            __m128 zero = _mm_setzero_ps();
            __m128 isZero = _mm_or_ps(_mm_cmpeq_ps(sum, zero), _mm_cmpeq_ps(rsqrt, zero));
            __m128 half = _mm_set1_ps(0.5f);
            __m128 three = _mm_set1_ps(3.0f);
            rsqrt = _mm_mul_ps(_mm_mul_ps(rsqrt, half), _mm_sub_ps(three, _mm_mul_ps(rsqrt, _mm_mul_ps(rsqrt, sum))));
            rsqrt = _mm_mul_ps(_mm_mul_ps(rsqrt, half), _mm_sub_ps(three, _mm_mul_ps(rsqrt, _mm_mul_ps(sum, rsqrt))));
            __m128 len = _mm_blendv_ps(_mm_mul_ps(sum, rsqrt), sum, isZero);
            __m128 nonZeroLen = _mm_cmpneq_ps(zero, len);
            __m128 notInf = _mm_cmpneq_ps(sum, _mm_castsi128_ps(_mm_set1_epi32(0x7f800000)));
            __m128 nanMask = _mm_andnot_ps(notInf, _mm_castsi128_ps(_mm_set1_epi32(0x7fc00000)));
            lightDir = _mm_or_ps(nanMask, _mm_and_ps(_mm_and_ps(_mm_div_ps(rawDir, len), nonZeroLen), notInf));
        }
    }

    void* pCsmMgr = viewBytes + 0x1320;
    sub_eac210(pCsmMgr, pShadowCfg);

    if (*reinterpret_cast<uint64_t*>(viewBytes + 0x1430) != 0 &&
        *reinterpret_cast<uint64_t*>(viewBytes + 0x1430) != 0x2a2a2a2a2a2a2a2aULL) {
        const uint8_t* camBytes = static_cast<const uint8_t*>(pCamera);
        alignas(16) uint8_t cascadeCam[0x90];
        alignas(16) uint8_t passCtx[0x870];

        if (pRenderDevHolder == nullptr ||
            reinterpret_cast<uintptr_t>(pRenderDevHolder) == 0x2a2a2a2a2a2a2a2aULL ||
            !gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectDynamicShadows)) {
            return;
        }

        std::memset(passCtx, 0, sizeof(passCtx));
        for (uint32_t cascadeIdx = 0; cascadeIdx < 4; ++cascadeIdx) {
            std::memcpy(cascadeCam, camBytes, 0x84);
            sub_eac540(pCsmMgr, cascadeCam, camBytes, &lightDir, cascadeIdx);

            uint32_t nextIdx = cascadeIdx + 1;
            sub_22d4fd0(passCtx, *reinterpret_cast<void**>(pRenderDevHolder),
                        pRenderDevHolder, viewIdx, nextIdx, cascadeCam, nullptr, 0, 0, 0, 0);

            passCtx[0x400] = 1;
            passCtx[0x401] = 1;
            passCtx[0x403] = 1;
            *reinterpret_cast<uint32_t*>(passCtx + 0x118) = 0;
            *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;

            uint8_t* pPkt = static_cast<uint8_t*>(sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 4, 0x18, 4));
            uint8_t bFlag = passCtx[0x401];
            if (pPkt != nullptr) {
                if (bFlag != 0) {
                    CmdPacketHeader hdr{};
                    hdr.packedWord = packCmdQueueHeader(*reinterpret_cast<uint64_t*>(passCtx + 0x110),
                                                        *reinterpret_cast<uint64_t*>(passCtx + 0x118));
                    uint8_t bFlag403 = passCtx[0x403];
                    hdr.bFlag0 = (bFlag403 != 0) ? 0 : (passCtx[0x402] != 0 ? 1 : 0);
                    hdr.bFlag1 = bFlag403;
                    hdr.pPayload = pPkt;
                    sub_2289ca0(*reinterpret_cast<void**>(passCtx + 0x450), &hdr);
                    passCtx[0x401] = 0;
                }
                uint8_t* pSlot = pPkt + pPkt[1] + 4;
                *reinterpret_cast<uint64_t*>(pSlot) = GX_VMA(0x229d2a0ULL);
                *reinterpret_cast<uint64_t*>(pSlot + 0x08) = 0;
                *reinterpret_cast<uint64_t*>(pSlot + 0x10) = 0;
                passCtx[0x400] = 0;
                passCtx[0x403] = 0;
                passCtx[0x402] = 0;
                sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
            } else {
                passCtx[0x400] = 0;
                passCtx[0x403] = 0;
                passCtx[0x402] = 0;
                if (bFlag == 0) {
                    sub_2289bb0(*reinterpret_cast<void**>(passCtx + 0x450), 0xff, 0, 4);
                }
            }
            *reinterpret_cast<uint32_t*>(passCtx + 0x11c) = 0;

            sub_eac680(pCsmMgr, passCtx, &lightDir, cascadeIdx);
            void* vtbl = *reinterpret_cast<void**>(pArg4);
            (*reinterpret_cast<void (**)(void*, void*)>(vtbl))(pArg4, passCtx);
            sub_22d5480(passCtx);
        }
    }
}

// ============================================================================
// 0x229cb60: sub_229cb60 (1 byte)
// Empty virtual/callback stub
// ============================================================================
void sub_229cb60() {
}

// ============================================================================
// 0x229cb10: sub_229cb10 (70 bytes)
// Forwards render target handles from pSrc to sub_eaaeb0
// ============================================================================
void sub_229cb10(void* pSelf, const void* pSrc) {
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    void* pMgr = *reinterpret_cast<void**>(pGlobalDd8 + 0x6088);
    const uint8_t* srcBytes = static_cast<const uint8_t*>(pSrc);
    void* pTex6c8 = *reinterpret_cast<void* const*>(srcBytes + 0x6c8);
    void* pTex200 = *reinterpret_cast<void* const*>(srcBytes + 0x200);
    void* pTex2a8 = *reinterpret_cast<void* const*>(srcBytes + 0x2a8);
    sub_eaaeb0(pMgr, pSelf, pTex6c8, pTex6c8, pTex200, pTex2a8, pTex200);
}

// ============================================================================
// 0x229cb00: sub_229cb00 (11 bytes)
// Returns pointer at pSelf + idx * 8 + 0x6e70
// ============================================================================
void* sub_229cb00(const void* pSelf, uint32_t idx) {
    return *reinterpret_cast<void* const*>(static_cast<const uint8_t*>(pSelf) + static_cast<uint64_t>(idx) * 8 + 0x6e70);
}

// ============================================================================
// 0x229cab0: sub_229cab0 (75 bytes)
// Selects primary or fallback render target slot based on boolean flags
// ============================================================================
void* sub_229cab0(const void* pSelf, uint8_t bSecondary, uint8_t bUseLocal) {
    if (bUseLocal != 0) {
        const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
        return *reinterpret_cast<void* const*>(selfBytes + (bSecondary != 0 ? 0x65a0 : 0x6598));
    }
    const uint8_t* pGlobal6c8 = *reinterpret_cast<const uint8_t**>(GX_VMA(0x55406c8ULL));
    const uint8_t* pSub250 = *reinterpret_cast<const uint8_t* const*>(pGlobal6c8 + 0x250);
    return *reinterpret_cast<void* const*>(pSub250 + (bSecondary != 0 ? 0x1d8 : 0x1d0));
}

// ============================================================================
// 0x229ca80: sub_229ca80 (42 bytes)
// Returns G-Buffer / light accumulation surface handle by index (0, 1, 2)
// ============================================================================
void* sub_229ca80(const void* pSelf, uint32_t idx) {
    const uint8_t* selfBytes = static_cast<const uint8_t*>(pSelf);
    if (idx == 2) {
        return *reinterpret_cast<void* const*>(selfBytes + 0x6600);
    }
    if (idx == 1) {
        return *reinterpret_cast<void* const*>(selfBytes + 0x65f8);
    }
    if (idx == 0) {
        return *reinterpret_cast<void* const*>(selfBytes + 0x65f0);
    }
    return nullptr;
}

// ============================================================================
// 0x229c970: sub_229c970 (267 bytes)
// Appends a non-null pointer to the pool vector at pSelf + 0x28
// ============================================================================
void sub_229c970(void* pSelf, void* pItem) {
    if (pItem == nullptr) {
        return;
    }
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void* localItem = pItem;
    void** pBegin = *reinterpret_cast<void***>(selfBytes + 0x30);
    void** pEnd = *reinterpret_cast<void***>(selfBytes + 0x38);
    void** pCap = *reinterpret_cast<void***>(selfBytes + 0x40);

    auto growIfFull = [&]() {
        if (pEnd == pCap) {
            uint64_t count = static_cast<uint64_t>(pEnd - pBegin);
            uint64_t half = count >> 1;
            uint64_t maxAdd = 0x1fffffffffffffffULL - half;
            uint64_t newCap = (maxAdd >= count) ? (count + half) : 0ULL;
            if (newCap < count + 1) {
                newCap = count + 1;
            }
            sub_22bf750(selfBytes + 0x28, newCap);
            pBegin = *reinterpret_cast<void***>(selfBytes + 0x30);
            pEnd = *reinterpret_cast<void***>(selfBytes + 0x38);
        }
    };

    if (&localItem < pEnd && pBegin <= &localItem) {
        ptrdiff_t idx = &localItem - pBegin;
        growIfFull();
        if (pEnd != nullptr) {
            *pEnd = pBegin[idx];
        }
    } else {
        growIfFull();
        if (pEnd != nullptr) {
            *pEnd = localItem;
        }
    }
    *reinterpret_cast<void***>(selfBytes + 0x38) = pEnd + 1;
}

// ============================================================================
// 0x229c900: sub_229c900 (103 bytes)
// Acquires a pooled 0x1f0-byte render node or allocates a new one
// ============================================================================
void* sub_229c900(void* pSelf) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    void** pBegin = *reinterpret_cast<void***>(selfBytes + 0x30);
    void** pEnd = *reinterpret_cast<void***>(selfBytes + 0x38);
    if (pEnd != pBegin) {
        void* pObj = pEnd[-1];
        *reinterpret_cast<void***>(selfBytes + 0x38) = pEnd - 1;
        return pObj;
    }
    void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* vtbl = *reinterpret_cast<void**>(pHeap);
    uint8_t* pNewObj = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(vtbl) + 0x58))(pHeap, 0x1f0, 8));
    sub_228b870(pNewObj, *reinterpret_cast<void**>(GX_VMA(0x5540680ULL)));
    uint8_t* pOwner20 = *reinterpret_cast<uint8_t**>(selfBytes + 0x20);
    *reinterpret_cast<void**>(pNewObj + 0x58) = *reinterpret_cast<void**>(pOwner20 + 0x58);
    return pNewObj;
}

// ============================================================================
// 0x229c8a0: sub_229c8a0 (85 bytes)
// Updates scene renderer sub-managers per frame with delta time
// ============================================================================
void sub_229c8a0(void* pSelf, float dt) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    sub_e464d0(selfBytes + 0x78c8);
    sub_221b8c0(selfBytes + 0x5860, dt);
    uint8_t* pSub6070 = *reinterpret_cast<uint8_t**>(selfBytes + 0x6070);
    if (pSub6070 != nullptr) {
        void* pSub440 = *reinterpret_cast<void**>(pSub6070 + 0x440);
        if (pSub440 != nullptr) {
            sub_e30ec0(pSub440);
        }
    }
}

// ============================================================================
// 0x229c890: sub_229c890 (9 bytes)
// Non-virtual thunk to sub_229c200 (adjusts this by -8)
// ============================================================================
void sub_229c200(void* pSelf, uint32_t width, uint32_t height);

void sub_229c890(void* pSelf, uint32_t width, uint32_t height) {
    sub_229c200(static_cast<uint8_t*>(pSelf) - 8, width, height);
}

// ============================================================================
// 0x229c200: sub_229c200 (1,673 bytes)
// Releases 7 reference-counted targets and reinitializes viewport/Yebis state
// ============================================================================
void sub_229c200(void* pSelf, uint32_t width, uint32_t height) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);

    releaseRefCountedSlot(selfBytes, 0x7970);
    releaseRefCountedSlot(selfBytes, 0x7978);
    releaseRefCountedSlot(selfBytes, 0x7948);
    releaseRefCountedSlot(selfBytes, 0x7950);
    releaseRefCountedSlot(selfBytes, 0x7958);
    releaseRefCountedSlot(selfBytes, 0x7960);
    releaseRefCountedSlot(selfBytes, 0x7968);

    if (selfBytes[0xb8] != 0) {
        void* pSub60 = selfBytes + 0x60;
        if (selfBytes[0xb0] != 0 || selfBytes[0xb2] != 0 || selfBytes[0xb3] != 0) {
            uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
            uint8_t* pVecHolder = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x5f8);
            if (pVecHolder != nullptr) {
                void* targetPtr = selfBytes + 0x68;
                void** it = *reinterpret_cast<void***>(pVecHolder + 0x08);
                void** end = *reinterpret_cast<void***>(pVecHolder + 0x10);
                while (it != end) {
                    if (*it == targetPtr) {
                        ++it;
                        std::memmove(it - 1, it, reinterpret_cast<uint8_t*>(end) - reinterpret_cast<uint8_t*>(it));
                        *reinterpret_cast<uint8_t**>(pVecHolder + 0x10) -= 8;
                        break;
                    }
                    ++it;
                }
            }
        }
        sub_22c5a60(pSub60, 1);

        uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        uint8_t* pDev90 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x90);
        alignas(32) uint8_t initDesc[0x40];
        std::memset(initDesc, 0, sizeof(initDesc));
        *reinterpret_cast<uint32_t*>(initDesc + 0x00) = width;
        *reinterpret_cast<uint32_t*>(initDesc + 0x04) = height;
        *reinterpret_cast<uint32_t*>(initDesc + 0x08) = (width > 0x800u) ? width : 0x800u;
        *reinterpret_cast<uint32_t*>(initDesc + 0x0c) = (height > 0x800u) ? height : 0x800u;
        *reinterpret_cast<uint64_t*>(initDesc + 0x20) = *reinterpret_cast<uint64_t*>(pDev90 + 0x68);
        *reinterpret_cast<uint64_t*>(initDesc + 0x28) = *reinterpret_cast<uint64_t*>(pDev90 + 0x70);
        *reinterpret_cast<uint64_t*>(initDesc + 0x30) = 0;
        sub_22c58e0(pSub60, initDesc, selfBytes, 0);

        uint8_t* pYebis = *reinterpret_cast<uint8_t**>(selfBytes + 0x6078);
        if (pYebis != nullptr) {
            sub_21d2830(pYebis + 0xaf0);
            pYebis = *reinterpret_cast<uint8_t**>(selfBytes + 0x6078);
            *reinterpret_cast<uint32_t*>(pYebis + 0x3204) = *reinterpret_cast<uint32_t*>(selfBytes + 0x792c);
            if (*reinterpret_cast<void**>(pYebis + 0xb00) == nullptr) {
                *reinterpret_cast<void**>(pYebis + 0xb00) = pYebis;
            }
            if (*reinterpret_cast<void**>(pYebis + 0x3160) == nullptr) {
                sub_21d2dd0(pYebis + 0xaf0);
            }
        }
    }
}

// ============================================================================
// 0x229c140: sub_229c140 (177 bytes)
// Releases deferred lighting shader/resource holder at +0x60a8 and re-inits
// ============================================================================
void sub_229c140(void* pSelf, void* pArg2) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    uint8_t* pHolder = *reinterpret_cast<uint8_t**>(selfBytes + 0x60a8);
    if (pHolder != nullptr) {
        void* pMainAlloc = sub_207b720(pHolder);
        void* pPtr18 = *reinterpret_cast<void**>(pHolder + 0x18);
        if (pPtr18 != nullptr) {
            void* pAlloc0 = *reinterpret_cast<void**>(pHolder + 0x00);
            void* vtbl0 = *reinterpret_cast<void**>(pAlloc0);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl0) + 0x70))(pAlloc0, pPtr18);
            *reinterpret_cast<void**>(pHolder + 0x18) = nullptr;
        }
        void* pPtr10 = *reinterpret_cast<void**>(pHolder + 0x10);
        if (pPtr10 != nullptr) {
            void* pAlloc8 = *reinterpret_cast<void**>(pHolder + 0x08);
            void* vtbl8 = *reinterpret_cast<void**>(pAlloc8);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl8) + 0x70))(pAlloc8, pPtr10);
            *reinterpret_cast<void**>(pHolder + 0x10) = nullptr;
        }
        void* vtblMain = *reinterpret_cast<void**>(pMainAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtblMain) + 0x70))(pMainAlloc, pHolder);
        *reinterpret_cast<void**>(selfBytes + 0x60a8) = nullptr;
    }
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6570) = 0;
    *reinterpret_cast<uint64_t*>(selfBytes + 0x6568) = 0;
    std::memset(selfBytes + 0x60a0, 0, 0x4b8);
    sub_229a680(pSelf, pArg2);
}

void sub_21a2ce0(void* pOutInputLayout, const void* pVertexElements, uint32_t elementCount,
                 void* pVertexShaderHolder, uint32_t arg5, uint32_t arg6);
void sub_207d250();
void sub_2762d00(void* pWString, const char16_t* fmt, ...);
void sub_ea8d80(void* pSub6080, void* pShaderProvider);
void sub_eaad00(void* pSub6088, void* pShaderProvider);

} // extern "C"

namespace {

static inline uint8_t* getThreadLocalStringAllocator() {
    uint64_t fsBase = 0;
    fsBase = gx_guest_tls_base();
    uint64_t tlsOffset = static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL));
    uint8_t* pTlsBlock = *reinterpret_cast<uint8_t**>(fsBase + tlsOffset);
    if (pTlsBlock == nullptr) {
        sub_207d250();
        fsBase = gx_guest_tls_base();
        pTlsBlock = *reinterpret_cast<uint8_t**>(fsBase + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    }
    uint8_t* pAlloc = pTlsBlock + 0x28;
    void* vtbl = *reinterpret_cast<void**>(pAlloc);
    uint8_t flagsBuf[8] = {};
    (*reinterpret_cast<void (**)(void*, void*, uint32_t)>(static_cast<uint8_t*>(vtbl) + 0x20))(flagsBuf, pAlloc, 0);
    if ((flagsBuf[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }
    return pAlloc;
}

static inline void* lookupPixelShader(void* pProvider, const char16_t* category, const char16_t* name) {
    void* vtbl = *reinterpret_cast<void**>(pProvider);
    void* pHolder = (*reinterpret_cast<void* (**)(void*, const char16_t*, const char16_t*)>(
        static_cast<uint8_t*>(vtbl) + 0x20))(pProvider, category, name);
    if (pHolder == nullptr) {
        return nullptr;
    }
    void* holderVtbl = *reinterpret_cast<void**>(pHolder);
    return (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(holderVtbl) + 0x18))(pHolder);
}

static inline void registerIndexedPixelShaderSeries(uint8_t* selfBytes, void* pProvider,
                                                    const char16_t* category, const char16_t* fmt,
                                                    size_t baseOffset, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        uint8_t* pAlloc = getThreadLocalStringAllocator();
        struct FormattedWString {
            union {
                char16_t* pData;
                char16_t  inlineBuf[8];
            };
            uint64_t length;
            uint64_t capacity;
            void*    pAllocator;
            uint8_t  bOwnsHeap;
            uint8_t  pad29[7];
        };
        struct WStringWrapper {
            uint64_t         pad0;
            FormattedWString str;
        } wrapper{};
        wrapper.str.inlineBuf[0] = 0;
        wrapper.str.length = 0;
        wrapper.str.capacity = 7;
        wrapper.str.pAllocator = pAlloc;
        wrapper.str.bOwnsHeap = 1;

        sub_2762d00(&wrapper, fmt, i + 1);
        const char16_t* cstr = (wrapper.str.capacity > 7) ? wrapper.str.pData : wrapper.str.inlineBuf;
        *reinterpret_cast<void**>(selfBytes + baseOffset + static_cast<size_t>(i) * 8) =
            lookupPixelShader(pProvider, category, cstr);

        if (wrapper.str.capacity >= 8) {
            void* allocVtbl = *reinterpret_cast<void**>(wrapper.str.pAllocator);
            (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(allocVtbl) + 0x70))(
                wrapper.str.pAllocator, wrapper.str.pData);
        }
    }
}

} // namespace

extern "C" {

// ============================================================================
// 0x229a680: sub_229a680 (6,841 bytes)
// Initializes all Deferred Lighting, SSAO, Point/Spot/Line Light, Shadow Mask,
// and Environment Decal Shaders & Input Layouts
// ============================================================================
void sub_229a680(void* pSelf, void* pShaderProvider) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    const char16_t* kFilterCategory = reinterpret_cast<const char16_t*>(GX_VMA(0x49cedf4ULL));

    void* provVtbl = *reinterpret_cast<void**>(pShaderProvider);
    void* pLightAccVsHolder = (*reinterpret_cast<void* (**)(void*, const char16_t*, const char16_t*)>(
        static_cast<uint8_t*>(provVtbl) + 0x28))(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce6ecULL)));
    void* pLightAccVs = nullptr;
    if (pLightAccVsHolder != nullptr) {
        void* vsVtbl = *reinterpret_cast<void**>(pLightAccVsHolder);
        pLightAccVs = (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vsVtbl) + 0x18))(pLightAccVsHolder);
    }
    *reinterpret_cast<void**>(selfBytes + 0x60a0) = pLightAccVs;

    *reinterpret_cast<void**>(selfBytes + 0x6550) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce702ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6558) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce724ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6560) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce746ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6530) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce76aULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6538) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce796ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6540) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce7d2ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6548) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce802ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6568) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce842ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6570) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce864ULL)));
    *reinterpret_cast<void**>(selfBytes + 0x6578) = lookupPixelShader(pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce88aULL)));

    if (*reinterpret_cast<void**>(selfBytes + 0x6078) != nullptr) {
        sub_21a2ce0(selfBytes + 0x60a8, reinterpret_cast<const void*>(GX_VMA(0x513ace0ULL)), 2, pLightAccVsHolder, 0, 0);
    }

    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce8c0ULL)), 0x60b0, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce8f0ULL)), 0x6110,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce930ULL)), 0x6130, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce962ULL)), 0x6190,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce9a4ULL)), 0x6230, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ce9d6ULL)), 0x6290,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cea18ULL)), 0x6330, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cea4eULL)), 0x6390,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cea94ULL)), 0x61b0, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cead0ULL)), 0x6210,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ceb1cULL)), 0x62b0, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ceb5aULL)), 0x6310,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ceba8ULL)), 0x63b0, 12);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cebeaULL)), 0x6410,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cec3cULL)), 0x6430,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cec6aULL)), 0x6450,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ceca8ULL)), 0x6470,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cecd8ULL)), 0x6490,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ced18ULL)), 0x64b0,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ced4cULL)), 0x64d0,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49ced90ULL)), 0x64f0,  4);
    registerIndexedPixelShaderSeries(selfBytes, pShaderProvider, kFilterCategory, reinterpret_cast<const char16_t*>(GX_VMA(0x49cedc0ULL)), 0x6510,  4);

    const char16_t* kEnvDecal = reinterpret_cast<const char16_t*>(GX_VMA(0x49cee02ULL));
    *reinterpret_cast<void**>(selfBytes + 0x6580) = lookupPixelShader(pShaderProvider, kFilterCategory, kEnvDecal);

    provVtbl = *reinterpret_cast<void**>(pShaderProvider);
    void* pEnvDecalVsHolder = (*reinterpret_cast<void* (**)(void*, const char16_t*, const char16_t*)>(
        static_cast<uint8_t*>(provVtbl) + 0x28))(pShaderProvider, kFilterCategory, kEnvDecal);
    if (pEnvDecalVsHolder != nullptr) {
        void* vsVtbl = *reinterpret_cast<void**>(pEnvDecalVsHolder);
        *reinterpret_cast<void**>(selfBytes + 0x6588) =
            (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(vsVtbl) + 0x18))(pEnvDecalVsHolder);
        if (*reinterpret_cast<void**>(selfBytes + 0x6078) != nullptr) {
            sub_21a2ce0(selfBytes + 0x6590, reinterpret_cast<const void*>(GX_VMA(0x513ace0ULL)), 2, pEnvDecalVsHolder, 0, 0);
        }
    } else {
        *reinterpret_cast<void**>(selfBytes + 0x6588) = nullptr;
    }

    void* pSub6080 = *reinterpret_cast<void**>(selfBytes + 0x6080);
    if (pSub6080 != nullptr) {
        sub_ea8d80(pSub6080, pShaderProvider);
    }
    void* pSub6088 = *reinterpret_cast<void**>(selfBytes + 0x6088);
    if (pSub6088 != nullptr) {
        sub_eaad00(pSub6088, pShaderProvider);
    }
}

} // extern "C"

extern "C" {

void sub_e45200(void* pTarget, void* pAllocator, void* pArg3);
void sub_22c39f0(void* pTarget, const void* pDesc, void* pOwner, uint32_t flag);
void sub_21a7f20(void* pTarget, uint32_t size, uint32_t count, uint32_t arg4,
                 uint32_t arg5, uint32_t arg6, void* pArg7);
void sub_221a8c0(void* pTarget, const void* pDesc);
void sub_21ca3d0(void* pTarget, uint32_t arg2, uint32_t arg3);
void sub_21782b0(void* pOutDsState, void* pDevice, const void* pDsDesc);
void sub_2178050(void* pOutBlendState, void* pDevice, const void* pBlendDesc);
void sub_2178550(void* pOutRsState, void* pDevice, const void* pRsDesc);
void sub_2165c10(void* pDeviceRoot, void* pCBufferWrapper, const void* pBufDesc,
                 uint32_t arg4, uint32_t arg5, void* pAllocator, void* pArg7);
void sub_ea8cc0(void* pObj);
void sub_eaaa90(void* pObj);
void sub_22f2060(void* pSub6070, uint32_t arg2, uint32_t arg3);
void sub_22f03d0(void* pSub6070, void* pSelf);
void sub_2199d40(void* pGlobal6c8, void* pSelfPlus8);

} // extern "C"

namespace {

static inline void* createDepthStencilStateObject(const void* pDsDesc) {
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pRoot = *reinterpret_cast<uint8_t**>(pGlobal6c8);
    if (*reinterpret_cast<void**>(pRoot + 0x00) == nullptr ||
        *reinterpret_cast<void**>(pRoot + 0x08) == nullptr) {
        return nullptr;
    }
    void* pAlloc = *reinterpret_cast<void**>(pRoot + 0xdc10);
    void* pDev20 = *reinterpret_cast<void**>(pRoot + 0x20);
    if (pAlloc == nullptr || pDev20 == nullptr) {
        return nullptr;
    }
    uint64_t tmpState[7] = {};
    sub_21782b0(tmpState, pDev20, pDsDesc);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    uint64_t* pOut = static_cast<uint64_t*>(
        (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(allocVtbl) + 0x58))(pAlloc, 0x38, 4));
    if (pOut == nullptr) {
        return nullptr;
    }
    for (int i = 0; i < 7; ++i) {
        pOut[i] = tmpState[i];
    }
    return pOut;
}

static inline void* createBlendStateObject(const void* pBlendDesc) {
    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pRoot = *reinterpret_cast<uint8_t**>(pGlobal6c8);
    if (*reinterpret_cast<void**>(pRoot + 0x00) == nullptr ||
        *reinterpret_cast<void**>(pRoot + 0x08) == nullptr) {
        return nullptr;
    }
    void* pAlloc = *reinterpret_cast<void**>(pRoot + 0xdc10);
    void* pDev20 = *reinterpret_cast<void**>(pRoot + 0x20);
    if (pAlloc == nullptr || pDev20 == nullptr) {
        return nullptr;
    }
    uint8_t tmpBlend[0x128] = {};
    sub_2178050(tmpBlend, pDev20, pBlendDesc);
    void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
    void* pOut = (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(
        static_cast<uint8_t*>(allocVtbl) + 0x58))(pAlloc, 0x128, 4);
    if (pOut == nullptr) {
        return nullptr;
    }
    std::memcpy(pOut, tmpBlend, 0x128);
    return pOut;
}

static inline void replicateRenderTargetBlendDesc0To1Through7(uint8_t* blendDesc) {
    uint64_t w0 = *reinterpret_cast<uint64_t*>(blendDesc + 0x24);
    uint64_t w1 = *reinterpret_cast<uint64_t*>(blendDesc + 0x2c);
    uint64_t w2 = *reinterpret_cast<uint64_t*>(blendDesc + 0x34);
    uint64_t w3 = *reinterpret_cast<uint64_t*>(blendDesc + 0x3c);
    for (size_t rt = 2; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = w0;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = w1;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = w2;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = w3;
    }
}

static inline void replicateRenderTargetBlendDesc1To2Through7(uint8_t* blendDesc) {
    uint64_t w0 = *reinterpret_cast<uint64_t*>(blendDesc + 0x24);
    uint64_t w1 = *reinterpret_cast<uint64_t*>(blendDesc + 0x2c);
    uint64_t w2 = *reinterpret_cast<uint64_t*>(blendDesc + 0x34);
    uint64_t w3 = *reinterpret_cast<uint64_t*>(blendDesc + 0x3c);
    for (size_t rt = 3; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = w0;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = w1;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = w2;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = w3;
    }
}

static inline void createConstantBufferSlot(uint8_t* selfBytes, size_t slotOffset, uint32_t byteSize) {
    void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* heapVtbl = *reinterpret_cast<void**>(pHeap);
    uint8_t* pWrapper = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 0x10, 8));
    if (pWrapper != nullptr) {
        *reinterpret_cast<uint64_t*>(pWrapper + 0x00) = 0;
        *reinterpret_cast<uint64_t*>(pWrapper + 0x08) = 0;
    }
    *reinterpret_cast<void**>(selfBytes + slotOffset) = pWrapper;

    uint32_t allocSize = byteSize;
    if (byteSize >= 0x10001u) {
        uint32_t rem = 0x10000u % byteSize;
        uint32_t aligned = 0x10000u - rem;
        if (byteSize >= aligned) {
            allocSize = aligned;
        }
    }
    *reinterpret_cast<uint32_t*>(pWrapper + 0x08) = allocSize;
    *reinterpret_cast<uint32_t*>(pWrapper + 0x0c) = byteSize;
    if (allocSize != 0) {
        alignas(16) uint32_t bufDesc[7] = {};
        bufDesc[0] = allocSize;
        bufDesc[2] = 4;
        bufDesc[4] = 0x800;
        uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        sub_2165c10(*reinterpret_cast<void**>(pGlobal6c8), pWrapper, bufDesc,
                    0, 0, *reinterpret_cast<void**>(GX_VMA(0x55406b8ULL)), nullptr);
    }
}

} // namespace

extern "C" {

// ============================================================================
// 0x22979b0: sub_22979b0 (11,464 bytes)
// Master Deferred Lighting, Depth-Stencil, Blend, Rasterizer, Constant Buffer,
// and Bayer Dither Texture Initializer
// ============================================================================
bool sub_22979b0(void* pSelf, const void* pInitDesc, void* pFactory) {
    uint8_t* selfBytes = static_cast<uint8_t*>(pSelf);
    const uint8_t* descBytes = static_cast<const uint8_t*>(pInitDesc);

    *reinterpret_cast<uint64_t*>(selfBytes + 0x7940) = *reinterpret_cast<const uint64_t*>(descBytes + 0x30);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7938) = *reinterpret_cast<const uint64_t*>(descBytes + 0x28);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7930) = *reinterpret_cast<const uint64_t*>(descBytes + 0x20);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7928) = *reinterpret_cast<const uint64_t*>(descBytes + 0x18);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7920) = *reinterpret_cast<const uint64_t*>(descBytes + 0x10);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7918) = *reinterpret_cast<const uint64_t*>(descBytes + 0x08);
    *reinterpret_cast<uint64_t*>(selfBytes + 0x7910) = *reinterpret_cast<const uint64_t*>(descBytes + 0x00);

    void* pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* heapVtbl = *reinterpret_cast<void**>(pHeap);
    uint8_t* pNode = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 0x1f0, 8));
    if (pNode != nullptr) {
        sub_228b870(pNode, *reinterpret_cast<void**>(GX_VMA(0x5540680ULL)));
    }
    *reinterpret_cast<void**>(selfBytes + 0x20) = pNode;
    sub_e45200(pNode + 0x08, *reinterpret_cast<void**>(GX_VMA(0x5540680ULL)), *reinterpret_cast<void* const*>(descBytes));
    *reinterpret_cast<void**>(pNode + 0x58) = pNode + 0x08;

    void* pDesc0 = *reinterpret_cast<void* const*>(descBytes);
    *reinterpret_cast<void**>(GX_VMA(0x56c3e78ULL)) = pDesc0;
    *reinterpret_cast<void**>(GX_VMA(0x56c3e80ULL)) = pDesc0;

    uint8_t* pNode20 = *reinterpret_cast<uint8_t**>(selfBytes + 0x20);
    void* pSub58 = *reinterpret_cast<void**>(pNode20 + 0x58);
    void* sub58Vtbl = *reinterpret_cast<void**>(pSub58);
    *reinterpret_cast<void**>(GX_VMA(0x56c3e88ULL)) =
        (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(sub58Vtbl) + 0x28))(pSub58);

    void* facVtbl = *reinterpret_cast<void**>(pFactory);
    *reinterpret_cast<void**>(selfBytes + 0x6070) =
        (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(facVtbl) + 0x00))(pFactory);
    facVtbl = *reinterpret_cast<void**>(pFactory);
    *reinterpret_cast<void**>(selfBytes + 0x6078) =
        (*reinterpret_cast<void* (**)(void*)>(static_cast<uint8_t*>(facVtbl) + 0x08))(pFactory);

    uint8_t* pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pDev90 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x90);
    alignas(32) uint8_t rtDesc[0x40];
    std::memset(rtDesc, 0, sizeof(rtDesc));
    const uint32_t initW = *reinterpret_cast<uint32_t*>(pDev90 + 0x80);
    const uint32_t initH = *reinterpret_cast<uint32_t*>(pDev90 + 0x84);
    *reinterpret_cast<uint32_t*>(rtDesc + 0x00) = initW;
    *reinterpret_cast<uint32_t*>(rtDesc + 0x04) = initH;
    *reinterpret_cast<uint32_t*>(rtDesc + 0x08) = (initW > 0x800u) ? initW : 0x800u;
    *reinterpret_cast<uint32_t*>(rtDesc + 0x0c) = (initH > 0x800u) ? initH : 0x800u;
    *reinterpret_cast<uint64_t*>(rtDesc + 0x20) = *reinterpret_cast<uint64_t*>(pDev90 + 0x68);
    *reinterpret_cast<uint64_t*>(rtDesc + 0x28) = *reinterpret_cast<uint64_t*>(pDev90 + 0x70);
    *reinterpret_cast<uint64_t*>(rtDesc + 0x30) = 0;
    rtDesc[0x3c] = 0;
    rtDesc[0x3d] = 0;
    sub_22c39f0(selfBytes + 0x60, rtDesc, selfBytes, 0);

    std::memset(rtDesc, 0, sizeof(rtDesc));
    *reinterpret_cast<uint32_t*>(rtDesc + 0x00) = 0x80;
    *reinterpret_cast<uint32_t*>(rtDesc + 0x04) = 0x80;
    *reinterpret_cast<uint32_t*>(rtDesc + 0x08) = 0x800;
    *reinterpret_cast<uint32_t*>(rtDesc + 0x0c) = 0x800;
    *reinterpret_cast<uint64_t*>(rtDesc + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(rtDesc + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(rtDesc + 0x30) = 0x2207;
    sub_22c39f0(selfBytes + 0x2c60, rtDesc, selfBytes, 0);
    *reinterpret_cast<uint32_t*>(selfBytes + 0x2cc8) = 2;

    sub_21a7f20(selfBytes + 0x76b0, 0x80, 10, 1, 0, 0, nullptr);

    struct Sub5860Desc {
        uint32_t val00;
        uint32_t val04;
        uint64_t val08;
        uint32_t val10;
        uint32_t val14;
    } desc5860 = { 0x100, 0x200, 0, 0x1000000, 0 };
    uint32_t v14 = *reinterpret_cast<const uint32_t*>(descBytes + 0x14);
    if (v14 != 0) desc5860.val00 = v14;
    uint32_t v18 = *reinterpret_cast<const uint32_t*>(descBytes + 0x18);
    if (v18 != 0) desc5860.val04 = v18;
    desc5860.val10 = *reinterpret_cast<const uint32_t*>(descBytes + 0x30);
    desc5860.val08 = *reinterpret_cast<const uint64_t*>(descBytes + 0x28);
    desc5860.val14 = *reinterpret_cast<const uint32_t*>(descBytes + 0x34);
    sub_221a8c0(selfBytes + 0x5860, &desc5860);

    pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    heapVtbl = *reinterpret_cast<void**>(pHeap);
    void** pMgrHolder = static_cast<void**>(
        (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 8, 8));
    if (pMgrHolder != nullptr) {
        uint32_t arg20 = *reinterpret_cast<const uint32_t*>(descBytes + 0x20);
        uint32_t arg24 = *reinterpret_cast<const uint32_t*>(descBytes + 0x24);
        pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        heapVtbl = *reinterpret_cast<void**>(pHeap);
        void* pMgr = (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(
            static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 0x618, 8);
        if (pMgr != nullptr) {
            sub_21ca3d0(pMgr, arg24, arg20);
        }
        *pMgrHolder = pMgr;
    }
    *reinterpret_cast<void***>(selfBytes + 0x6090) = pMgrHolder;
    *reinterpret_cast<void***>(GX_VMA(0x56a6cf0ULL)) = pMgrHolder;

    // 8 Depth-Stencil State Objects
    alignas(16) uint8_t dsDesc[0x38] = {};
    dsDesc[0x00] = 1;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x04) = 0;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x08) = 3;
    dsDesc[0x0c] = 1;
    dsDesc[0x0d] = 0xff;
    dsDesc[0x0e] = 0xff;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x10) = 0;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x14) = 2;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x18) = 1;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x1c) = 7;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x20) = 0;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x24) = 0;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x28) = 0;
    *reinterpret_cast<uint32_t*>(dsDesc + 0x2c) = 7;
    dsDesc[0x30] = 0;
    *reinterpret_cast<void**>(selfBytes + 0x65c0) = createDepthStencilStateObject(dsDesc);

    *reinterpret_cast<uint64_t*>(dsDesc + 0x10) = 0ULL;
    *reinterpret_cast<uint64_t*>(dsDesc + 0x18) = 0x200000000ULL;
    *reinterpret_cast<void**>(selfBytes + 0x65c8) = createDepthStencilStateObject(dsDesc);

    auto loadDsFromTemplate = [&](uint8_t* pTpl, bool bFullStencil, uint8_t readMask, uint8_t writeMask,
                                  uint32_t depthFailOp, uint32_t funcOp, uint8_t flag30) {
        dsDesc[0x00] = pTpl[0x0c];
        *reinterpret_cast<uint32_t*>(dsDesc + 0x04) = *reinterpret_cast<uint32_t*>(pTpl + 0x10);
        *reinterpret_cast<uint32_t*>(dsDesc + 0x08) = *reinterpret_cast<uint32_t*>(pTpl + 0x14);
        if (bFullStencil) {
            dsDesc[0x0c] = pTpl[0x0d];
            dsDesc[0x0d] = pTpl[0x09];
            dsDesc[0x0e] = pTpl[0x0a];
            uint32_t fOp = *reinterpret_cast<uint32_t*>(pTpl + 0x1c);
            *reinterpret_cast<uint32_t*>(dsDesc + 0x10) = fOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x14) = fOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x18) = *reinterpret_cast<uint32_t*>(pTpl + 0x20);
            *reinterpret_cast<uint32_t*>(dsDesc + 0x1c) = *reinterpret_cast<uint32_t*>(pTpl + 0x24);
            uint32_t bOp = *reinterpret_cast<uint32_t*>(pTpl + 0x2c);
            *reinterpret_cast<uint32_t*>(dsDesc + 0x20) = bOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x24) = bOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x28) = *reinterpret_cast<uint32_t*>(pTpl + 0x30);
            *reinterpret_cast<uint32_t*>(dsDesc + 0x2c) = *reinterpret_cast<uint32_t*>(pTpl + 0x34);
        } else {
            dsDesc[0x0c] = 1;
            dsDesc[0x0d] = readMask;
            dsDesc[0x0e] = writeMask;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x10) = 0;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x14) = 0;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x18) = depthFailOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x1c) = funcOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x20) = 0;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x24) = 0;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x28) = depthFailOp;
            *reinterpret_cast<uint32_t*>(dsDesc + 0x2c) = funcOp;
        }
        dsDesc[0x30] = flag30;
    };

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* pSub250 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    loadDsFromTemplate(*reinterpret_cast<uint8_t**>(pSub250 + 0x1d8), true, 0, 0, 0, 0, 1);
    *reinterpret_cast<void**>(selfBytes + 0x65d0) = createDepthStencilStateObject(dsDesc);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pSub250 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    loadDsFromTemplate(*reinterpret_cast<uint8_t**>(pSub250 + 0x1d0), false, 0x00, 0xff, 2, 7, 0);
    *reinterpret_cast<void**>(selfBytes + 0x6598) = createDepthStencilStateObject(dsDesc);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pSub250 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    loadDsFromTemplate(*reinterpret_cast<uint8_t**>(pSub250 + 0x1d8), false, 0x00, 0xff, 2, 7, 0);
    *reinterpret_cast<void**>(selfBytes + 0x65a0) = createDepthStencilStateObject(dsDesc);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pSub250 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    loadDsFromTemplate(*reinterpret_cast<uint8_t**>(pSub250 + 0x1c8), false, 0x02, 0x00, 0, 2, 0);
    *reinterpret_cast<void**>(selfBytes + 0x65a8) = createDepthStencilStateObject(dsDesc);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pSub250 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    loadDsFromTemplate(*reinterpret_cast<uint8_t**>(pSub250 + 0x1c8), false, 0x10, 0x00, 0, 2, 0);
    *reinterpret_cast<void**>(selfBytes + 0x65b0) = createDepthStencilStateObject(dsDesc);

    pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    pSub250 = *reinterpret_cast<uint8_t**>(pGlobal6c8 + 0x250);
    loadDsFromTemplate(*reinterpret_cast<uint8_t**>(pSub250 + 0x1c8), false, 0x08, 0x00, 0, 2, 0);
    *reinterpret_cast<void**>(selfBytes + 0x65b8) = createDepthStencilStateObject(dsDesc);

    // 8 Blend State Objects
    alignas(16) uint8_t blendDesc[0x108] = {};

    // Blend State 1 -> +0x65d8
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 1;
    blendDesc[0x04] = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x08) = 4;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x0c) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x10) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x14) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x18) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x1c) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 0x0f;
    blendDesc[0x24] = 0;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x28) = 1ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x30) = 0x100000000ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x38) = 0ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x40) = 0x0f;
    replicateRenderTargetBlendDesc0To1Through7(blendDesc);
    *reinterpret_cast<void**>(selfBytes + 0x65d8) = createBlendStateObject(blendDesc);

    // Blend State 2 -> +0x65e0
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 1;
    blendDesc[0x04] = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x08) = 4;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x0c) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x10) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x14) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x18) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x1c) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 0x0f;
    blendDesc[0x24] = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x28) = 4;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x2c) = 5;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x30) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x34) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x38) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x3c) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x40) = 7;
    blendDesc[0x44] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x48) = 0x500000001ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x50) = 0ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x58) = 1ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x60) = 7;
    replicateRenderTargetBlendDesc1To2Through7(blendDesc);
    *reinterpret_cast<void**>(selfBytes + 0x65e0) = createBlendStateObject(blendDesc);

    // Blend State 3 -> +0x65e8
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 1;
    blendDesc[0x04] = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x08) = 4;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x0c) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x10) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x14) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x18) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x1c) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 0x0f;
    blendDesc[0x24] = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x28) = 4;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x2c) = 5;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x30) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x34) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x38) = 1;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x3c) = 0;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x40) = 7;
    blendDesc[0x44] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x48) = 0x500000008ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x50) = 0ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x58) = 1ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x60) = 7;
    replicateRenderTargetBlendDesc1To2Through7(blendDesc);
    *reinterpret_cast<void**>(selfBytes + 0x65e8) = createBlendStateObject(blendDesc);

    // Blend State 4 -> +0x65f0
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 1;
    blendDesc[0x04] = 0;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x08) = 1ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x10) = 0x100000000ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x18) = 0ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 7;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x3c) = *reinterpret_cast<uint32_t*>(blendDesc + 0x1c);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x34) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x24) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x2c) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
    *reinterpret_cast<uint32_t*>(blendDesc + 0x40) = 3;
    for (size_t rt = 2; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = *reinterpret_cast<uint64_t*>(blendDesc + 0x1c);
    }
    *reinterpret_cast<void**>(selfBytes + 0x65f0) = createBlendStateObject(blendDesc);

    // Blend State 5 -> +0x65f8
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 1;
    blendDesc[0x04] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x08) = 0x500000004ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x10) = 0ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x18) = 1ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 7;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x3c) = *reinterpret_cast<uint32_t*>(blendDesc + 0x1c);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x34) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x24) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x2c) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
    *reinterpret_cast<uint32_t*>(blendDesc + 0x40) = 3;
    blendDesc[0x44] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x48) = 0x500000001ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x50) = 0ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x58) = 1ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x60) = 7;
    for (size_t rt = 3; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = *reinterpret_cast<uint64_t*>(blendDesc + 0x1c);
    }
    *reinterpret_cast<void**>(selfBytes + 0x65f8) = createBlendStateObject(blendDesc);

    // Blend State 6 -> +0x6600
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 1;
    blendDesc[0x04] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x08) = 0x500000004ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x10) = 0ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x18) = 1ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 7;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x3c) = *reinterpret_cast<uint32_t*>(blendDesc + 0x1c);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x34) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x24) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
    *reinterpret_cast<uint64_t*>(blendDesc + 0x2c) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
    *reinterpret_cast<uint32_t*>(blendDesc + 0x40) = 3;
    blendDesc[0x44] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x48) = 0x500000008ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x50) = 0ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x58) = 1ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x60) = 7;
    for (size_t rt = 3; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = *reinterpret_cast<uint64_t*>(blendDesc + 0x1c);
    }
    *reinterpret_cast<void**>(selfBytes + 0x6600) = createBlendStateObject(blendDesc);

    // Blend State 7 -> +0x6608
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 0;
    blendDesc[0x04] = 1;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x08) = 0x400000000ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x10) = 0x100000000ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x18) = 0ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 7;
    for (size_t rt = 1; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = *reinterpret_cast<uint64_t*>(blendDesc + 0x1c);
    }
    *reinterpret_cast<void**>(selfBytes + 0x6608) = createBlendStateObject(blendDesc);

    // Blend State 8 -> +0x6610
    blendDesc[0x00] = 0;
    blendDesc[0x01] = 0;
    blendDesc[0x04] = 0;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x08) = 1ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x10) = 0x100000000ULL;
    *reinterpret_cast<uint64_t*>(blendDesc + 0x18) = 0ULL;
    *reinterpret_cast<uint32_t*>(blendDesc + 0x20) = 8;
    for (size_t rt = 1; rt < 8; ++rt) {
        size_t off = 0x04 + rt * 0x20;
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x00) = *reinterpret_cast<uint64_t*>(blendDesc + 0x04);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x08) = *reinterpret_cast<uint64_t*>(blendDesc + 0x0c);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x10) = *reinterpret_cast<uint64_t*>(blendDesc + 0x14);
        *reinterpret_cast<uint64_t*>(blendDesc + off + 0x18) = *reinterpret_cast<uint64_t*>(blendDesc + 0x1c);
    }
    *reinterpret_cast<void**>(selfBytes + 0x6610) = createBlendStateObject(blendDesc);

    // Constant Buffers
    createConstantBufferSlot(selfBytes, 0x6678, 0xd0);
    createConstantBufferSlot(selfBytes, 0x6680, 0x10);
    for (uint32_t i = 0; i < 12; ++i) {
        createConstantBufferSlot(selfBytes, (0xcc3 + i) * 8, (i + 1) * 0x90);
    }
    for (uint32_t i = 0; i < 12; ++i) {
        createConstantBufferSlot(selfBytes, (0xcd1 + i) * 8, (i + 1) * 0x10);
    }
    createConstantBufferSlot(selfBytes, 0x66e8, 0x20);

    // 256-entry Rasterizer State Table at +0x6e70
    for (uint32_t idx = 0; idx < 0x100; ++idx) {
        uint32_t cullMode = (idx >> 1) & 0x03u;
        if (cullMode == 3u) {
            continue;
        }
        uint32_t biasMode = (idx >> 5) & 0x03u;
        alignas(16) uint8_t rsDesc[0x20] = {};
        *reinterpret_cast<uint32_t*>(rsDesc + 0x00) = idx & 1u;
        *reinterpret_cast<uint32_t*>(rsDesc + 0x04) = cullMode;
        rsDesc[0x08] = 1;
        if (biasMode == 2u) {
            *reinterpret_cast<uint32_t*>(rsDesc + 0x0c) = *reinterpret_cast<uint32_t*>(selfBytes + 0x7674);
            *reinterpret_cast<float*>(rsDesc + 0x10) = *reinterpret_cast<float*>(selfBytes + 0x7678);
            *reinterpret_cast<float*>(rsDesc + 0x14) = *reinterpret_cast<float*>(selfBytes + 0x767c);
        } else {
            *reinterpret_cast<uint32_t*>(rsDesc + 0x0c) = 0;
            *reinterpret_cast<uint32_t*>(rsDesc + 0x10) = 0;
            if (biasMode == 1u) {
                *reinterpret_cast<uint32_t*>(rsDesc + 0x14) = 0x40400000u; // 3.0f
            } else if ((idx & 0x08u) == 0) {
                *reinterpret_cast<uint32_t*>(rsDesc + 0x14) = 0u;
            } else {
                *reinterpret_cast<uint32_t*>(rsDesc + 0x14) = 0xc0400000u; // -3.0f
            }
        }
        rsDesc[0x18] = 1;
        rsDesc[0x19] = static_cast<uint8_t>((idx >> 4) & 1u);
        rsDesc[0x1a] = static_cast<uint8_t>((idx >> 7) & 1u);
        rsDesc[0x1b] = 0;

        pGlobal6c8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        uint8_t* pRoot = *reinterpret_cast<uint8_t**>(pGlobal6c8);
        if (*reinterpret_cast<void**>(pRoot + 0x00) != nullptr &&
            *reinterpret_cast<void**>(pRoot + 0x08) != nullptr) {
            void* pAlloc = *reinterpret_cast<void**>(pRoot + 0xdc10);
            void* pDev20 = *reinterpret_cast<void**>(pRoot + 0x20);
            if (pAlloc != nullptr && pDev20 != nullptr) {
                uint8_t tmpRs[0x28] = {};
                sub_2178550(tmpRs, pDev20, rsDesc);
                void* allocVtbl = *reinterpret_cast<void**>(pAlloc);
                uint8_t* pOutRs = static_cast<uint8_t*>(
                    (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(
                        static_cast<uint8_t*>(allocVtbl) + 0x58))(pAlloc, 0x24, 4));
                if (pOutRs != nullptr) {
                    *reinterpret_cast<uint32_t*>(pOutRs + 0x20) = *reinterpret_cast<uint32_t*>(tmpRs + 0x20);
                    *reinterpret_cast<uint64_t*>(pOutRs + 0x18) = *reinterpret_cast<uint64_t*>(tmpRs + 0x18);
                    *reinterpret_cast<uint64_t*>(pOutRs + 0x10) = *reinterpret_cast<uint64_t*>(tmpRs + 0x10);
                    *reinterpret_cast<uint64_t*>(pOutRs + 0x08) = *reinterpret_cast<uint64_t*>(tmpRs + 0x08);
                    *reinterpret_cast<uint64_t*>(pOutRs + 0x00) = *reinterpret_cast<uint64_t*>(tmpRs + 0x00);
                }
                *reinterpret_cast<void**>(selfBytes + 0x6e70 + static_cast<size_t>(idx) * 8) = pOutRs;
            }
        }
    }

    // 4x4 Bayer Dither Textures at +0x7720
    void* pDitherAlloc = *reinterpret_cast<void**>(GX_VMA(0x5540688ULL));
    void* ditherAllocVtbl = *reinterpret_cast<void**>(pDitherAlloc);
    uint8_t* pDitherPixels = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(
            static_cast<uint8_t*>(ditherAllocVtbl) + 0x58))(pDitherAlloc, 0x40, 0x10));
    bool bAllocatedPixels = (pDitherPixels != nullptr);

    struct SubResourceData {
        void*    pSysMem;
        uint32_t sysMemPitch;
        uint32_t sysMemSlicePitch;
    } ditherSubRes = { pDitherPixels, 0x10, 0 };

    static const uint32_t kBayer4x4[16] = {
        0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5
    };

    for (uint32_t layer = 0; layer < 4; ++layer) {
        uint32_t offset = layer * 5;
        for (uint32_t k = 0; k < 16; ++k) {
            uint32_t v = kBayer4x4[k] + offset;
            if (v > 0x0fu) {
                v -= 0x10u;
            }
            uint64_t scaled = static_cast<uint64_t>(v * 0xffu) * 0x88888889ULL;
            pDitherPixels[k] = static_cast<uint8_t>(scaled >> 35);
        }
        uint8_t* pTexObj = selfBytes + 0x7720 + static_cast<size_t>(layer) * 0x60;
        void* texVtbl = *reinterpret_cast<void**>(pTexObj);
        (*reinterpret_cast<void (**)(void*, uint32_t, uint32_t, uint32_t, uint32_t, const void*,
                                     uint32_t, uint32_t, void*, void*, uint32_t)>(
            static_cast<uint8_t*>(texVtbl) + 0xa0))(
            pTexObj, 4, 4, 0x41, 1, &ditherSubRes, 0, 0, nullptr, nullptr, 0);
    }

    if (bAllocatedPixels && pDitherPixels != nullptr) {
        void* vtbl = *reinterpret_cast<void**>(pDitherAlloc);
        (*reinterpret_cast<void (**)(void*, void*)>(static_cast<uint8_t*>(vtbl) + 0x70))(pDitherAlloc, pDitherPixels);
    }

    pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    heapVtbl = *reinterpret_cast<void**>(pHeap);
    void* pObj6080 = (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(
        static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 0x30, 8);
    if (pObj6080 != nullptr) {
        sub_ea8cc0(pObj6080);
    }
    *reinterpret_cast<void**>(selfBytes + 0x6080) = pObj6080;

    pHeap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    heapVtbl = *reinterpret_cast<void**>(pHeap);
    void* pObj6088 = (*reinterpret_cast<void* (**)(void*, uint64_t, uint64_t)>(
        static_cast<uint8_t*>(heapVtbl) + 0x58))(pHeap, 0x40, 8);
    if (pObj6088 != nullptr) {
        sub_eaaa90(pObj6088);
    }
    *reinterpret_cast<void**>(selfBytes + 0x6088) = pObj6088;

    sub_22f2060(*reinterpret_cast<void**>(selfBytes + 0x6070),
                *reinterpret_cast<const uint32_t*>(descBytes + 0x0c),
                *reinterpret_cast<const uint32_t*>(descBytes + 0x08));
    sub_229a680(selfBytes, *reinterpret_cast<void**>(selfBytes + 0x6070));
    sub_22f03d0(*reinterpret_cast<void**>(selfBytes + 0x6070), selfBytes);

    uint8_t* pYebis = *reinterpret_cast<uint8_t**>(selfBytes + 0x6078);
    uint8_t* pGlobalDd8 = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    *reinterpret_cast<uint32_t*>(pYebis + 0x3204) = *reinterpret_cast<uint32_t*>(pGlobalDd8 + 0x792c);
    if (*reinterpret_cast<void**>(pYebis + 0xb00) == nullptr) {
        *reinterpret_cast<void**>(pYebis + 0xb00) = pYebis;
    }
    if (*reinterpret_cast<void**>(pYebis + 0x3160) == nullptr) {
        sub_21d2dd0(pYebis + 0xaf0);
    }

    sub_2199d40(*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)), selfBytes + 0x08);
    return true;
}

} // extern "C"
