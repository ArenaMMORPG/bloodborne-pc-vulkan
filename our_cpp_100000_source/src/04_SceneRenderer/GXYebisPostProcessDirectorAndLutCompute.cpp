// ============================================================================
// GXYebisPostProcessDirectorAndLutCompute.cpp
// Bloodborne (CUSA03173 v1.09) - Module 04: SceneRenderer (Tier 8/10)
// Range: 0x21d5560 .. 0x21dfc40 (20 functions, 42,409 bytes)
// Subsystem: Master Yebis2 Post-Process Director, Debug GUI Builder,
//            Default Filter Parameter Initializer, and 3D LUT / CustomDof
//            Compute Shader Dispatch (LutCopy2DTo3DCs.cpo, LutBlend2DTo3DCs.cpo,
//            LutCopy3DTo2DCs.cpo, CustomDofLutCopy.cpo).
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>
#include <immintrin.h>

#include "gx_direct_vulkan.h"

namespace {

inline float BitsToFloat(uint32_t bits) noexcept {
    float f;
    std::memcpy(&f, &bits, sizeof(float));
    return f;
}

inline uint32_t FloatToBits(float f) noexcept {
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(uint32_t));
    return bits;
}

// ============================================================================
// External Engine / Yebis2 / GNM Function Declarations
// ============================================================================
extern "C" {

struct DLWStrIter24 {
    uint64_t t0;
    uint64_t t1;
    const char16_t* ptr;
};

// Dantelion2 Heap / Memory / Runtime
void sub_2bbe588(void* ptr);                                                     // 0x2bbe588 (operator delete)
void* sub_2bbe578(void* dst, const void* src, size_t size);                      // 0x2bbe578 (memcpy)
void* sub_2bbe688(void* dst, int val, size_t size);                              // 0x2bbe688 (memset)
float sub_2bbe518(int mode, int flags, float rad);                               // 0x2bbe518 (sincosf helper: 0=sin, 1=cos)
void* sub_e45560(void* linearAlloc, uint64_t size, uint64_t align);              // 0xe45560 (DLLinearAllocator::Allocate)
void* sub_2724ff0(void* dstWString,
                  DLWStrIter24 dstBeg,
                  DLWStrIter24 dstEnd,
                  DLWStrIter24 srcBeg,
                  DLWStrIter24 srcEnd);                                          // 0x2724ff0 (DLWString::replace)
void sub_262a720(void* dstWString, const char16_t* srcChars, size_t len);        // 0x262a720 (DLWString::assign(chars, len))

// Yebis2 Parameter Bank Registration Helpers (called from Master Debug Menu)
void sub_21fec50(void* settingBank, void* ui);                                   // 0x21fec50
void sub_2204780(void* toneMapBank, void* ui);                                   // 0x2204780
void sub_21e65b0(void* bloomBank, void* ui);                                     // 0x21e65b0
void sub_21f6b90(void* lightShaftBank, void* ui);                                // 0x21f6b90
void sub_21ef5c0(void* dofBank, void* ui);                                       // 0x21ef5c0
void sub_21e17a0(void* antiAliasBank, void* ui);                                 // 0x21e17a0
void sub_21fa740(void* motionBlurBank, void* ui);                                // 0x21fa740
void sub_21f2e70(void* feedBackBlurBank, void* ui);                              // 0x21f2e70
void sub_21ea4c0(void* colorGradingBank, void* ui);                              // 0x21ea4c0

// Yebis2 Director Action Callbacks (defined in lower SceneRenderer range)
void sub_21d4230(void* director, int arg);                                       // 0x21d4230 (InitParam callback)
void sub_21d4240(void* director, int arg);                                       // 0x21d4240 (Save callback)
void sub_21d44f0(void* director, int arg);                                       // 0x21d44f0 (Load callback)
void sub_21d4210(void* director);                                                // 0x21d4210 (FeedBackBlur Enable callback)
void sub_21d2dd0(void* director);                                                // 0x21d2dd0 (Reinitialize Yebis context)
void sub_21b4b00(void* jobQueue, void* fnCallback, void* arg0, void* arg1, uint32_t flags); // 0x21b4b00

// Debug UI Widget Creation Helpers
void* sub_236c900(void* ui, const void* labelDesc, uint32_t flags);              // 0x236c900 (BeginGroup / TreePush)
void* sub_236d620(void* ui, const void* labelDesc);                              // 0x236d620 (AddSubGroup)
void* sub_235e4c0(void* group, const void* labelDesc, void* callbackObj);        // 0x235e4c0 (AddButtonCallback)
void* sub_23362f0(void* fileWidget, void* parentNode, const void* desc, uint32_t a, uint32_t b); // 0x23362f0
void sub_23743a0(void* fileCtrl);                                                // 0x23743a0
void sub_2374120(void* fileCtrl, const char16_t* filename);                      // 0x2374120
void* sub_292f830(void* ui, const void* labelDesc, bool* pVal);                  // 0x292f830 (AddBoolCheckbox)
void* sub_292a730(void* ui, const void* labelDesc, int32_t* pVal);               // 0x292a730 (AddIntBoolToggle)
void* sub_292a590(void* ui, const void* labelDesc, float* pVal, float minV, float maxV, float stepV, float defV); // 0x292a590
void* sub_2930840(void* ui, const void* labelDesc, uint32_t* pVal, uint32_t count, const void* table); // 0x2930840
void* sub_2934780(void* ui, const void* labelDesc, int32_t* pVal, int32_t minV, int32_t maxV, int32_t stepV, float defV); // 0x2934780
void* sub_2934560(void* ui, const void* labelDesc, int32_t* pVal, uint32_t count, const void* table); // 0x2934560
void* sub_29316a0(void* ui, const void* labelDesc, uint32_t* pVal, uint32_t minV, uint32_t maxV, uint32_t stepV, float defV); // 0x29316a0
void* sub_2934910(void* ui, const void* labelDesc, int32_t* pVal);               // 0x2934910 (AddIntCheckbox)
void* sub_236cf00(void* ui, const void* labelDesc, void* rgbaPtr, uint32_t flags); // 0x236cf00 (AddColorPicker)
void* sub_2937a00(void* ui, const void* labelDesc, float* pVal, uint32_t count, const void* table); // 0x2937a00 (AddFloatEnumCombo)
void* sub_293f750(void* ui, const void* labelDesc, void* wstrPtr);               // 0x293f750 (AddWStringEdit)
void* sub_219db10(void* ui, const void* labelDesc, uint8_t* pReadOnlyByte);      // 0x219db10 (AddReadOnlyByteDisplay)

// Yebis2 Low-Level Library (PPFX / PostProcess Effects Context @ 0xbb26e0..0xbb5f10)
void sub_bb5f10(void* ppfxCtx, void* gnmTexturePtr);                             // 0xbb5f10 (ReleaseTextureRef)
void sub_bb4d70(void* ppfxCtx, int32_t arg);                                     // 0xbb4d70
void sub_bb4de0(void* ppfxCtx, uint8_t bufferIndex);                             // 0xbb4de0
void sub_bb26e0(void* ppfxCtx, int32_t arg);                                     // 0xbb26e0
void sub_bb2710(void* ppfxCtx, float f0, float f1, float f2, float dt, float scale); // 0xbb2710
void sub_bb2720(void* ppfxCtx, float f0, float f1, float f2);                    // 0xbb2720
void sub_bb2740(void* ppfxCtx, float f0, float f1, float f2);                    // 0xbb2740
void sub_bb27d0(void* ppfxCtx, const void* invProjMtx, int32_t flag);            // 0xbb27d0
void sub_bb27f0(void* ppfxCtx, const void* invPrevProjMtx, int32_t flag);        // 0xbb27f0
void sub_bb3fb0(void* ppfxCtx, uint8_t debugDofFlag);                            // 0xbb3fb0
void sub_bb5080(void* ppfxCtx, int32_t slot, void* gnmTex, uint32_t w, uint32_t h, uint32_t fmt, float f0, float f1, int32_t mask); // 0xbb5080
void sub_bb31c0(void* ppfxCtx, int32_t slot, uint8_t enableAutoExp);             // 0xbb31c0
float sub_bb3250(void* ppfxCtx, int32_t slot);                                   // 0xbb3250 (GetAdaptedExposure)
void sub_bb32f0(void* ppfxCtx, int32_t slot, float middleGray, float toneInfByGlare); // 0xbb32f0
void sub_bb32e0(void* ppfxCtx, int32_t slot, float minExp, float maxExp);        // 0xbb32e0
void sub_bb3300(void* ppfxCtx, int32_t slot, float mw, float mh, float ox, float oy); // 0xbb3300
void sub_bb3310(void* ppfxCtx, int32_t slot, int32_t flag, float delayFrame);    // 0xbb3310
void sub_bb3320(void* ppfxCtx, int32_t slot, float aSens, float aScale, float aSpeed, float dSens, float dScale, float dSpeed, float ratio); // 0xbb3320
void sub_bb3550(void* ppfxCtx, int32_t toneType, float exp, float gamma, float factor, float negMax); // 0xbb3550
void sub_bb3560(void* ppfxCtx, float filmicContrast);                            // 0xbb3560
void sub_bb37a0(void* ppfxCtx, uint8_t enableChromaticAberration);               // 0xbb37a0
void sub_bb37d0(void* ppfxCtx, int32_t waveSample, const float* latDisp, const float* uniDisp); // 0xbb37d0
void sub_bb4300(void* ppfxCtx, int32_t slot);                                    // 0xbb4300 (ResetColorCorrection)
void sub_bb4360(void* ppfxCtx, int32_t slot, int32_t flag, float hueRad);        // 0xbb4360 (SetColorHue)
void sub_bb4390(void* ppfxCtx, int32_t slot, float sat);                         // 0xbb4390 (SetColorSaturation)
void sub_bb43b0(void* ppfxCtx, int32_t slot, float val);                         // 0xbb43b0 (SetColorValue)
void sub_bb43e0(void* ppfxCtx, int32_t slot, float contrast);                    // 0xbb43e0 (SetColorContrast)
void sub_bb3670(void* ppfxCtx, float vigPower, float vigFovDep);                 // 0xbb3670 (SetVignette)
void sub_bb2840(void* ppfxCtx, int32_t flag, float glareLum, float glareThresh, float remapFactor); // 0xbb2840
void sub_bb2860(void* ppfxCtx, const void* customGlareStruct);                   // 0xbb2860 (SetCustomGlareShape)
void sub_bb2850(void* ppfxCtx, int32_t glareShapePreset);                        // 0xbb2850 (SetGlareShapePreset)
void sub_bb40e0(void* ppfxCtx, int32_t glareNumLevels);                          // 0xbb40e0
void sub_bb3c10(void* ppfxCtx, float fVal);                                      // 0xbb3c10
void sub_bb4130(void* ppfxCtx, float starFilterThresh);                          // 0xbb4130
void sub_bb4100(void* ppfxCtx, float starLenFovDep);                             // 0xbb4100
void sub_bb40f0(void* ppfxCtx, float starSoftness);                              // 0xbb40f0
void sub_bb4120(void* ppfxCtx, float samplingThresh);                            // 0xbb4120
void sub_bb4110(void* ppfxCtx, float bloomGaussRadScale);                        // 0xbb4110
void sub_bb3c30(void* ppfxCtx, float bloomLumGamma);                             // 0xbb3c30
void sub_bb41a0(void* ppfxCtx, float ghostConcentricDist);                       // 0xbb41a0
void sub_bb4160(void* ppfxCtx, uint8_t enableAnamorphic);                        // 0xbb4160
void sub_bb4180(void* ppfxCtx, float anamorphicFlareLum, float anamorphicGhostScale); // 0xbb4180
void sub_bb4190(void* ppfxCtx, const float* flareColor, const float* ghostColor);// 0xbb4190
void sub_bb28a0(void* ppfxCtx, int32_t slot);                                    // 0xbb28a0
void sub_bb28d0(void* ppfxCtx, int32_t slot, uint8_t enableLightShaft);          // 0xbb28d0
void sub_bb2940(void* ppfxCtx, int32_t slot, float screenX, float screenY);      // 0xbb2940
void sub_bb2900(void* ppfxCtx, int32_t slot, const float* lsColor, int32_t flag, float len, float glareRatio, float att, float angleAtt, float noiseMask, float noiseFreq); // 0xbb2900
void sub_bb2920(void* ppfxCtx, int32_t slot, const float* ringColor, float diffRing, float diffRad, float diffAtt, float diffSpecOrder); // 0xbb2920
void sub_bb2960(void* ppfxCtx, uint8_t enableDof);                               // 0xbb2960
void sub_bb3350(void* ppfxCtx, int32_t slot, int32_t enableAutoFocus);           // 0xbb3350
void sub_bb34b0(void* ppfxCtx, int32_t slot, uint8_t doNotPassOver, float sens, float speedLimit); // 0xbb34b0
void sub_bb3490(void* ppfxCtx, int32_t slot, int32_t flag, float focusDelay);    // 0xbb3490
void sub_bb3470(void* ppfxCtx, int32_t slot, float rangeMin, float rangeMax);    // 0xbb3470
void sub_bb3480(void* ppfxCtx, int32_t slot, float screenPosX, float screenPosY);// 0xbb3480
float sub_bb33e0(void* ppfxCtx, int32_t slot);                                   // 0xbb33e0 (GetAutoFocusDistance)
void sub_bb3340(void* ppfxCtx, int32_t slot, float manualFocusDist);             // 0xbb3340
void sub_bb3f80(void* ppfxCtx, float bgMaskThreshold);                           // 0xbb3f80
void sub_bb34a0(void* ppfxCtx, int32_t diaphragmRotateType, float rotScale, float rotOffsetRad); // 0xbb34a0
void sub_bb3fd0(void* ppfxCtx, int32_t frontLevels, int32_t backLevels);         // 0xbb3fd0
void sub_bb2970(void* ppfxCtx, int32_t slot, float focusDist, float apertureF, float adaptiveFactor, float adaptiveBaseFovRad, float sensorHeightM); // 0xbb2970
int32_t sub_bb45c0(void* ppfxCtx, int32_t dofEdgeQuality);                       // 0xbb45c0
void sub_bb3600(void* ppfxCtx, uint8_t enableColorGrading);                      // 0xbb3600
void sub_bb3660(void* ppfxCtx, int32_t slot, int32_t mode, int32_t arg, float fVal); // 0xbb3660
void sub_bb5790(void* ppfxCtx, void* lutGnmTex, uint32_t w, uint32_t h, uint32_t fmt); // 0xbb5790
void sub_bb3870(void* ppfxCtx, uint8_t enableMotionBlur);                        // 0xbb3870
void sub_bb38b0(void* ppfxCtx, uint8_t bidirectionalPerm);                       // 0xbb38b0
void sub_bb3890(void* ppfxCtx, int32_t motionBlurType);                          // 0xbb3890
void sub_bb38e0(void* ppfxCtx, float falloffMin, float falloffMax);              // 0xbb38e0
void sub_bb27c0(void* ppfxCtx, const void* viewProjMtx, int32_t flag);           // 0xbb27c0
void sub_bb27e0(void* ppfxCtx, const void* prevViewProjMtx, int32_t flag);       // 0xbb27e0
void sub_bb2800(void* ppfxCtx, float deltaTime);                                 // 0xbb2800
int32_t sub_bb5430(void* ppfxCtx, int32_t slot, void* velGnmTex, uint32_t w, uint32_t h, uint32_t fmt, int32_t flag0, const void* scalePair, int32_t flag2); // 0xbb5430
void sub_bb38f0(void* ppfxCtx, int32_t numBaseSamples, int32_t maxRecurrences, float blurTimeRatio, float maxBlurLen, float sampleInterleaved); // 0xbb38f0
void sub_bb3900(void* ppfxCtx, uint8_t flag, float camRotPerspThresh, float camTransThresh); // 0xbb3900
void sub_bb34c0(void* ppfxCtx, uint8_t enableFeedbackBlur);                      // 0xbb34c0
void sub_bb34d0(void* ppfxCtx, float fbWeight0, float fbWeight1, float fbUnitTime); // 0xbb34d0
void sub_bb34e0(void* ppfxCtx, float fbAspectRatio);                             // 0xbb34e0
void sub_bb37f0(void* ppfxCtx, uint8_t enableAntiAlias);                         // 0xbb37f0
void sub_bb3c50(void* ppfxCtx, uint32_t flags, const float* color, int32_t a, int32_t b, float negMax); // 0xbb3c50
float sub_bb2dc0(void* ppfxCtx, int32_t slot, int32_t queryType, float normDepth); // 0xbb2dc0

// GNM /CoreGraphics2 / GX Helpers called during Compute & Post-Process Dispatch
void sub_10945c0(void* yebisGpuCtx);                                             // 0x10945c0
void sub_10946d0(void* yebisGpuCtx);                                             // 0x10946d0
void sub_2168a20(void* gnmCtx, void* stageBinder, void* uavDesc, int32_t slot, int32_t count, const void* extDim, int32_t f0, int32_t f1); // 0x2168a20
void* sub_2274e20(void* shdrContainer, const char16_t* cpoName);                 // 0x2274e20 (FindComputeShaderByName)
void sub_26af700(void* gfxc);                                                    // 0x26af700 (Gnm::WaitForGraphicsWrites / Stall)
void sub_26afa00(void* gfxc);                                                    // 0x26afa00
void sub_26cbe50(void* dispatchHelper, void* gnmCtx, void* stageBinder);         // 0x26cbe50 (BindComputePipelineState)
void sub_1074c50(void* gfxc, uint32_t tgX, uint32_t tgY, uint32_t tgZ);          // 0x1074c50 (Gnm::DrawCommandBuffer::dispatch)
void sub_26ccf20(void* dispatchHelper, void* gnmCtx, void* stageBinder);         // 0x26ccf20 (UnbindComputePipelineState)
void sub_219fcf0(void* renderCmdCtx);                                            // 0x219fcf0 (FlushDirtyRenderState)
void sub_e46f40(void* rtBinder, void* rtDesc, int32_t a, int32_t b);             // 0xe46f40
void sub_e47290(void* rtBinder, int32_t flag);                                   // 0xe47290
void* sub_e47840(void* rtBinder);                                                // 0xe47840
void sub_21a1a00(void* renderCmdCtx);                                            // 0x21a1a00
void sub_21a1c50(void* renderCmdCtx);                                            // 0x21a1c50
void sub_26d3070(void* rtState, void* gnmCtx, const void* clearDesc);            // 0x26d3070
void sub_26b9850(void* syncState, void* gfxc);                                   // 0x26b9850
void sub_26cd190(void* pipeState, void* gnmCtx);                                 // 0x26cd190
void sub_26bba10(void* depthState);                                              // 0x26bba10
void sub_26d3300(void* viewportState);                                           // 0x26d3300
void sub_26d2d20(void* rtState);                                                 // 0x26d2d20
void sub_108c520(void* yebisGpuCtx, int32_t slot, void* rtGnmDesc);              // 0x108c520
bool sub_26b9a50(void* syncState);                                               // 0x26b9a50
void sub_26b9a40(void* syncState);                                               // 0x26b9a40
uint32_t sub_1089960(void* gnmTexture);                                          // 0x1089960 (GetGnmTextureFormatCode)
void sub_21aaa90(void* outViewProjMtx, const void* sceneView);                   // 0x21aaa90
void sub_21aa290(void* outProjParams, const void* sceneView);                    // 0x21aa290
void sub_10850d0(void* depthStencilControl);                                     // 0x10850d0
void sub_1085130(void* depthStencilControl, int32_t enable);                     // 0x1085130
void sub_1076b30(void* yebisGpuCtx, uint32_t dsCtrlReg);                         // 0x1076b30
void sub_1084f40(void* primitiveSetup);                                          // 0x1084f40
void sub_1084f50(void* primitiveSetup, int32_t cullFace);                        // 0x1084f50
void sub_1084f60(void* primitiveSetup, int32_t frontFace);                       // 0x1084f60
void sub_1084f70(void* primitiveSetup, int32_t polyModeFront, int32_t polyModeBack); // 0x1084f70
void sub_1076e00(void* yebisGpuCtx, uint32_t primSetupReg);                      // 0x1076e00
void sub_1072fd0(void* yebisGpuCtx, uint32_t renderTargetMask);                  // 0x1072fd0
void sub_1076030(void* yebisGpuCtx, int32_t clipControl);                        // 0x1076030
void sub_1085380(void* blendControl);                                            // 0x1085380
void sub_1085390(void* blendControl, int32_t enable, int32_t mask);              // 0x1085390
void sub_10853f0(void* blendControl, int32_t separateAlpha);                     // 0x10853f0
void sub_10733a0(void* yebisGpuCtx, uint32_t blendCtrlReg);                      // 0x10733a0
void sub_1096f60(void* yebisGpuCtx, int32_t x, int32_t y, uint32_t w, uint32_t h, float minZ, float maxZ); // 0x1096f60
void sub_1077150(void* yebisGpuCtx, int32_t a, int32_t b);                       // 0x1077150
void sub_26b7310(void* outElemFormat, int32_t fmtEnum);                          // 0x26b7310
void sub_1070650(void* outGnmBuffer, void* baseAddr, uint32_t fmt, uint32_t stride, uint32_t numElems); // 0x1070650
void sub_108f330(void* yebisShaderCtx, int32_t stage, int32_t startSlot, int32_t count, const void* gnmBuffer); // 0x108f330
void sub_1091bc0(void* fetchShaderMem, const void* shdrHeader, uint8_t flags);   // 0x1091bc0
void sub_108fbb0(void* yebisShaderCtx, void* vsBin, uint32_t vsReg, void* fetchMem, void* cacheSlot); // 0x108fbb0
void sub_108fdd0(void* yebisShaderCtx, void* psBin, void* cacheSlot);            // 0x108fdd0
void sub_108d3f0(void* yebisShaderCtx, int32_t stage, int32_t slot, const void* gnmTex128); // 0x108d3f0
void sub_1086dd0(void* gnmSampler);                                              // 0x1086dd0
void sub_1086e10(void* gnmSampler, int32_t wrapMode);                            // 0x1086e10
void sub_108dfe0(void* yebisShaderCtx, int32_t stage, int32_t slot, const void* gnmSampler); // 0x108dfe0
void sub_1073fe0(void* yebisGpuCtx, int32_t primType);                           // 0x1073fe0
void sub_1075270(void* yebisGpuCtx, int32_t indexOffset);                        // 0x1075270
void sub_108ca70(void* yebisShaderCtx, void* yebisGpuCtx, void* subCtx);         // 0x108ca70
void sub_1074360(void* yebisGpuCtx, uint32_t indexCount, void* indexAddr);       // 0x1074360 (drawIndexAuto / drawIndex)
void sub_108ca80(void* yebisShaderCtx, void* yebisGpuCtx, void* subCtx);         // 0x108ca80
void sub_10778f0(void* gfxc, int32_t eopEvent, int32_t cacheAction, void* dstGpuAddr, int32_t dataSel, uint64_t immVal, int32_t intrSel, int32_t intrType); // 0x10778f0
void sub_1077b30(void* gfxc, void* gpuAddr, uint32_t mask, int32_t cmpOp, uint32_t refVal); // 0x1077b30 (waitOnAddress)
void sub_1089a10(int32_t flag, void* pStart0, void* pSize0, void* pStart1, void* pSize1); // 0x1089a10
void sub_2168660(void* gnmCtx, void* stateCtx, void* dstSurf, void* srcSurf);    // 0x2168660 (CopySurface)
void sub_21696c0(void* gnmCtx, void* stageBinder, int32_t primType, int32_t vertexCount); // 0x21696c0 (DrawVertices)
int32_t sub_2165150(void* vtxStreamObj, int32_t a, uint32_t frameIdx);           // 0x2165150
int32_t sub_26d0d30(void* cbAlloc, void* gfxc, void** outCpuPtr, void* streamDesc); // 0x26d0d30
void sub_2192d20(void* localRenderCtx, void* srcViewDesc);                       // 0x2192d20

} // extern "C"

// ============================================================================
// Global Engine Singletons & Data Tables (VMA-Annotated)
// ============================================================================
struct DLAllocatorVtable {
    void* fn00;
    void* fn08;
    void* fn10;
    void (*dtor_18)(void* self);
    void (*destroy_20)(void* self);
    void* fn28;
    void* fn30;
    void (*finish_38)(void* self, int32_t a, int32_t b);
    void* fn40;
    uint32_t (*getWidth_48)(void* self, int32_t mip);
    uint32_t (*getHeight_50)(void* self, int32_t mip);
    void* (*allocate_58)(void* self, size_t size, size_t align);
    void* fn60;
    void* fn68;
    void (*free_70)(void* self, void* ptr);
};

struct DLAllocator {
    DLAllocatorVtable* vtable;
};

struct DLRefCountedObj {
    DLAllocatorVtable* vtable;
    uint32_t refFlags; // +0x08
    uint32_t pad0C;
};

struct DLUiLabelDesc {
    uint64_t reserved0; // +0x00
    const char16_t* text; // +0x08
};

struct DLUiEnumEntry {
    const char16_t* label;
    uint32_t value;
    uint32_t pad;
};

struct GXYebisMemberCallbackWithArg {
    void* vtable;            // +0x00
    uint32_t refFlags;       // +0x08
    uint32_t pad0C;          // +0x0C
    void* targetObj;         // +0x10
    int32_t argValue;        // +0x18
    int32_t pad1C;           // +0x1C
    uintptr_t fnPtrOrSlot;   // +0x20
    std::ptrdiff_t adjOffset;// +0x28
};

struct GXYebisMemberCallbackVoid {
    void* vtable;            // +0x00
    uint32_t refFlags;       // +0x08
    uint32_t pad0C;          // +0x0C
    void* targetObj;         // +0x10
    uintptr_t fnPtrOrSlot;   // +0x18
    std::ptrdiff_t adjOffset;// +0x20
};

// Global Pointers in .data/.bss
#define g_YebisLutAllocator_55406a8        (*reinterpret_cast<DLAllocator**>(GX_VMA(0x55406a8ULL)))
#define g_UiCallbackAllocator_5540e08      (*reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL)))
#define g_RenderDeviceSingleton_55406c8    (*reinterpret_cast<void**>(GX_VMA(0x55406c8ULL)))
#define g_SceneRendererRoot_5540dd8        (*reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL)))
#define g_ViewportManagerSingleton_5540dc0 (*reinterpret_cast<void**>(GX_VMA(0x5540dc0ULL)))
#define g_GlobalYebisFilterParam_55406e0   (reinterpret_cast<uint8_t*>(GX_VMA(0x55406e0ULL)))
#define g_GlobalYebisParamPtr_53e4660      (reinterpret_cast<void*>(GX_VMA(0x55406e0ULL)))
#define g_DbgCameraBlurFlag_56c3d40        (*reinterpret_cast<bool*>(GX_VMA(0x56c3d40ULL)))
#define g_YebisBufferFlipIndex_56a6d20     (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6d20ULL)))
#define g_YebisDoubleBufArea_56a6d20       (reinterpret_cast<uint8_t*>(GX_VMA(0x56a6d20ULL)))
#define g_ActiveYebisGpuCtx_56c3cb8        (*reinterpret_cast<void**>(GX_VMA(0x56c3cb8ULL)))
#define g_ActiveYebisGfxc0_56c3cc0         (*reinterpret_cast<void**>(GX_VMA(0x56c3cc0ULL)))
#define g_ActiveYebisGfxc1_56c3cc8         (*reinterpret_cast<void**>(GX_VMA(0x56c3cc8ULL)))
#define g_ActiveYebisCmdBase0_56c3cd0      (*reinterpret_cast<uint64_t*>(GX_VMA(0x56c3cd0ULL)))
#define g_ActiveYebisCmdBase1_56c3cd8      (*reinterpret_cast<uint64_t*>(GX_VMA(0x56c3cd8ULL)))
#define g_YebisPostProcessEnabled_56c3ce0  (*reinterpret_cast<bool*>(GX_VMA(0x56c3ce0ULL)))
#define g_YebisFenceCounter_56c3ce8        (*reinterpret_cast<uint64_t*>(GX_VMA(0x56c3ce8ULL)))
#define g_YebisFenceGpuAddrBase_56c3cf8    (*reinterpret_cast<uint8_t**>(GX_VMA(0x56c3cf8ULL)))

#undef vtable_53b93b0
#define vtable_53b93b0 (reinterpret_cast<void**>(GX_VMA(0x53b93b0ULL)))
#undef vtable_53b7020
#define vtable_53b7020 (reinterpret_cast<void**>(GX_VMA(0x53b7020ULL)))
#undef vtable_53b6fe0
#define vtable_53b6fe0 (reinterpret_cast<void**>(GX_VMA(0x53b6fe0ULL)))

// Enum Tables bound directly to boot.bin .data.rel.ro (0x535ff00 .. 0x5360250)
#define kGlareShapeEnumTable_535ff00      (reinterpret_cast<const DLUiEnumEntry*>(GX_VMA(0x535ff00ULL)))
#define kCcdSizeEnumTable_535fff0         (reinterpret_cast<const DLUiEnumEntry*>(GX_VMA(0x535fff0ULL)))
#define kDiaphragmRotateEnumTable_5360100 (reinterpret_cast<const DLUiEnumEntry*>(GX_VMA(0x5360100ULL)))
#define kDofEdgeQualityEnumTable_5360140  (reinterpret_cast<const DLUiEnumEntry*>(GX_VMA(0x5360140ULL)))
#define kBlurShapeEnumTable_5360190       (reinterpret_cast<const DLUiEnumEntry*>(GX_VMA(0x5360190ULL)))
#define kToneTypeEnumTable_53601e0        (reinterpret_cast<const DLUiEnumEntry*>(GX_VMA(0x53601e0ULL)))

// Helper to release a DLRefCountedObj with dual 15-bit weak/strong counters (exact instruction pattern)
inline void ReleaseDLRefCountedNode(DLRefCountedObj* obj) noexcept {
    if (!obj) {
        return;
    }
    uint32_t oldFlags = obj->refFlags;
    uint32_t decStrong = oldFlags + 0x7fffu;
    uint32_t newStrong = decStrong & 0x7fffu;
    uint32_t updated = (oldFlags & 0xffff8000u) | newStrong;
    obj->refFlags = updated;
    if (newStrong == 0 && (oldFlags & 0x40000000u) != 0) {
        obj->vtable->dtor_18(obj);
        updated = obj->refFlags & 0xbfffffffu;
        obj->refFlags = updated;
    }
    uint32_t decWeak = (updated >> 15) + 0x7fffu;
    uint32_t newWeak = decWeak & 0x7fffu;
    obj->refFlags = (updated & 0xc0007fffu) | (newWeak << 15);
    if (newWeak == 0) {
        obj->refFlags = updated | 0x3fff8000u;
        obj->vtable->destroy_20(obj);
    }
}

// Helper to pop the current UI group node from `ui` vector `[ui+0x08 .. ui+0x10]`
inline void PopDbgUiGroupNode(void* ui) noexcept {
    auto** pBegin = reinterpret_cast<DLRefCountedObj***>(reinterpret_cast<uint8_t*>(ui) + 0x08);
    auto** pEnd   = reinterpret_cast<DLRefCountedObj***>(reinterpret_cast<uint8_t*>(ui) + 0x10);
    DLRefCountedObj** beginPtr = *pBegin;
    DLRefCountedObj** endPtr   = *pEnd;
    if (beginPtr != endPtr) {
        DLRefCountedObj* node = *(endPtr - 1);
        if (node) {
            ReleaseDLRefCountedNode(node);
            *(endPtr - 1) = nullptr;
            endPtr = *pEnd;
        }
        *pEnd = endPtr - 1;
    }
}

} // namespace

extern "C" {

// ============================================================================
// 1. 0x21dfc40 (221 B) - GXYebisPostProcessDirector_DestroyLutBuffersAndSubObjects
// ============================================================================
void sub_21dfc40(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    base[0x1cfc0] = 0;

    DLAllocator* alloc = g_YebisLutAllocator_55406a8;
    if (alloc && alloc->vtable && alloc->vtable->free_70) {
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1cfe0));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1cff0));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1d000));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1d010));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1cfe8));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1cff8));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1d008));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1d018));
        alloc->vtable->free_70(alloc, *reinterpret_cast<void**>(base + 0x1cfd0));
    }

    *reinterpret_cast<uint64_t*>(base + 0x1cfd8) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x1cf98) = 0;

    sub_10945c0(base + 0xe7c0);
    sub_10945c0(base + 0x08);
}

// ============================================================================
// 2. 0x21dfc30 (15 B) - GXYebisPostProcessDirector_RequestResetContext
// ============================================================================
void sub_21dfc30(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    base[0x2718] = 1;
    base[0x20f8] = 1;
}

// ============================================================================
// 3. 0x21df510 (1824 B) - GXYebisPostProcessFilterParam_InitDefaults
// ============================================================================
void sub_21df510(void* paramBlock) {
    auto* p = static_cast<uint8_t*>(paramBlock);

    *reinterpret_cast<uint32_t*>(p + 0x000) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x004) = 0x3c888889u; // 1.0f / 60.0f
    *reinterpret_cast<uint32_t*>(p + 0x050) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x064) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x068) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x06c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x070) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x074) = 0x3e4ccccdu; // 0.2f
    *reinterpret_cast<uint32_t*>(p + 0x078) = 0x3f4ccccdu; // 0.8f
    *reinterpret_cast<uint32_t*>(p + 0x07c) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x080) = 0x42700000u; // 60.0f
    *reinterpret_cast<uint32_t*>(p + 0x084) = 0x3dcccccd;  // 0.1f
    *reinterpret_cast<uint32_t*>(p + 0x088) = 0x3dcccccd;  // 0.1f
    *reinterpret_cast<uint32_t*>(p + 0x08c) = 0x3dcccccd;  // 0.1f
    *reinterpret_cast<uint32_t*>(p + 0x090) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x098) = 0x3fc00000u; // 1.5f
    *reinterpret_cast<uint32_t*>(p + 0x09c) = 0x400ccccdu; // 2.2f
    *reinterpret_cast<uint32_t*>(p + 0x0a0) = 2;
    *reinterpret_cast<uint32_t*>(p + 0x0a4) = 0x42000000u; // 32.0f
    *reinterpret_cast<uint32_t*>(p + 0x0a8) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x0ac) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x0b0) = 3;
    *reinterpret_cast<uint32_t*>(p + 0x094) = 0x400ccccdu; // 2.2f
    *reinterpret_cast<uint32_t*>(p + 0x0c4) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x0bc) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x0b4) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x0c8) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x0cc) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x248) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x054) = 0x3e3851ecu; // 0.18f
    *reinterpret_cast<uint32_t*>(p + 0x058) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x05c) = 0x3dcccccd;  // 0.1f
    *reinterpret_cast<uint32_t*>(p + 0x060) = 0x41200000u; // 10.0f
    *reinterpret_cast<uint32_t*>(p + 0x0d0) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x0d4) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x250) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x0d8) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x0dc) = 0x40400000u; // 3.0f
    *reinterpret_cast<uint32_t*>(p + 0x0e0) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x0e4) = 8;
    *reinterpret_cast<uint32_t*>(p + 0x0e8) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x0ec) = 0x3a83126fu; // 0.001f
    *reinterpret_cast<uint32_t*>(p + 0x0f0) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x0f4) = 0x3e800000u; // 0.25f
    *reinterpret_cast<uint32_t*>(p + 0x0f8) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x0fc) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x100) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x104) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x108) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x10c) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x110) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x120) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x114) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x124) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x118) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x128) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x11c) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x12c) = 0x3f8001a3u;
    *reinterpret_cast<uint32_t*>(p + 0x130) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x134) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x138) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x13c) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x140) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x144) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x148) = 0x3f400000u; // 0.75f
    *reinterpret_cast<uint32_t*>(p + 0x14c) = 0x3f7fff58u;
    *reinterpret_cast<uint32_t*>(p + 0x150) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x154) = 0x41200000u; // 10.0f
    *reinterpret_cast<uint32_t*>(p + 0x158) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x15c) = 0x41200000u; // 10.0f
    *reinterpret_cast<uint32_t*>(p + 0x160) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x164) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x24c) = 0x44fa0000u; // 2000.0f
    *reinterpret_cast<uint32_t*>(p + 0x168) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x16c) = 0x3e4ccccdu; // 0.2f
    *reinterpret_cast<uint32_t*>(p + 0x170) = 0x3c23d70au; // 0.01f
    *reinterpret_cast<uint32_t*>(p + 0x174) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x178) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x17c) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x180) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x184) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x188) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x18c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x190) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x194) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x198) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x19c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x1a0) = 0x459c4000u; // 5000.0f
    *reinterpret_cast<uint32_t*>(p + 0x1a4) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x1a8) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x1ac) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x1c0) = 0x422d1168u; // 43.266998f (35mm full-frame diagonal)
    *reinterpret_cast<uint32_t*>(p + 0x1b4) = 0x40b33333u; // 5.6f
    *reinterpret_cast<uint32_t*>(p + 0x1b0) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x1b8) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x1bc) = 0x42340000u; // 45.0f
    *reinterpret_cast<uint32_t*>(p + 0x1c4) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint32_t*>(p + 0x1c8) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x1cc) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x1d0) = 3;
    *reinterpret_cast<uint32_t*>(p + 0x1d4) = 3;
    *reinterpret_cast<uint32_t*>(p + 0x1d8) = 0x3dcccccd;  // 0.1f
    *reinterpret_cast<uint32_t*>(p + 0x1dc) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x1e0) = 2;
    *reinterpret_cast<uint32_t*>(p + 0x1e4) = 0x3d4ccccdu; // 0.05f
    *reinterpret_cast<uint32_t*>(p + 0x1e8) = 0x3dcccccd;  // 0.1f
    *reinterpret_cast<uint32_t*>(p + 0x1ec) = 4;
    *reinterpret_cast<uint32_t*>(p + 0x1f0) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x1f4) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x1f8) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x1fc) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x200) = 0;
    p[0x204] = 1;
    *reinterpret_cast<uint32_t*>(p + 0x220) = 0;

    *reinterpret_cast<uint32_t*>(p + 0x008) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x00c) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x010) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x014) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x018) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x01c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x020) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x024) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x028) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x02c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x030) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x034) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x038) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x03c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x040) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x044) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x048) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x04c) = 1;

    *reinterpret_cast<uint32_t*>(p + 0x208) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x21c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x20c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x210) = 0x3f000000u; // 0.5f
    *reinterpret_cast<uint32_t*>(p + 0x214) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x218) = 0;

    // 0x4920e80: {0x3ec0c0c1, 0x3ec0c0c1, 0x3ec0c0c1, 0x3f800000}
    *reinterpret_cast<uint32_t*>(p + 0x230) = 0x3ec0c0c1u;
    *reinterpret_cast<uint32_t*>(p + 0x234) = 0x3ec0c0c1u;
    *reinterpret_cast<uint32_t*>(p + 0x238) = 0x3ec0c0c1u;
    *reinterpret_cast<uint32_t*>(p + 0x23c) = 0x3f800000u;

    *reinterpret_cast<uint64_t*>(p + 0x240) = 0x3f000000ffffffffULL;
    const uint64_t onePair = 0x3f8000003f800000ULL;
    *reinterpret_cast<uint64_t*>(p + 0x254) = onePair;
    *reinterpret_cast<uint64_t*>(p + 0x25c) = onePair;
    *reinterpret_cast<uint64_t*>(p + 0x264) = onePair;
    *reinterpret_cast<uint64_t*>(p + 0x26c) = onePair;
    *reinterpret_cast<uint32_t*>(p + 0x27c) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x274) = 0;

    *reinterpret_cast<uint32_t*>(p + 0x280) = 3;
    *reinterpret_cast<uint32_t*>(p + 0x284) = 0x402ccccdu; // 2.7f
    *reinterpret_cast<uint32_t*>(p + 0x288) = 0x3e8f5c29u; // 0.28f
    *reinterpret_cast<uint32_t*>(p + 0x28c) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x290) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x294) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x298) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x29c) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2a0) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2a4) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x2a8) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2ac) = 0x40000000u; // 2.0f
    *reinterpret_cast<uint32_t*>(p + 0x2b0) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x2b4) = 1;
    *reinterpret_cast<uint32_t*>(p + 0x2b8) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x2bc) = 0xffffffffu;
    *reinterpret_cast<uint32_t*>(p + 0x2c0) = 0xffffffffu;
    *reinterpret_cast<uint32_t*>(p + 0x2c4) = 0xffffffffu;
    *reinterpret_cast<uint32_t*>(p + 0x2c8) = 0;
    *reinterpret_cast<uint16_t*>(p + 0x2cc) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x2d8) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x2d0) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x2e0) = 0x40000000u; // 2.0f
    *reinterpret_cast<uint32_t*>(p + 0x2e4) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2e8) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2ec) = 0x42c80000u; // 100.0f
    *reinterpret_cast<uint32_t*>(p + 0x2f0) = 0x43160000u; // 150.0f
    *reinterpret_cast<uint32_t*>(p + 0x2f4) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2f8) = 0x3f800000u;
    *reinterpret_cast<uint32_t*>(p + 0x2fc) = 0x461c4000u; // 10000.0f
    *reinterpret_cast<uint64_t*>(p + 0x300) = 0x3c23d70a00000000ULL;
    *reinterpret_cast<uint64_t*>(p + 0x348) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x340) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x338) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x330) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x328) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x320) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x318) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x310) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x308) = 0;
    *reinterpret_cast<uint32_t*>(p + 0x350) = 0x3f800000u; // 1.0f
    *reinterpret_cast<uint64_t*>(p + 0x360) = 0;
    *reinterpret_cast<uint64_t*>(p + 0x358) = 0;
}

// ============================================================================
// 4. 0x21df4e0 (40 B) - GXYebisMemberCallbackWithArg_Invoke
// ============================================================================
void sub_21df4e0(GXYebisMemberCallbackWithArg* self) {
    if ((self->refFlags & 0x40000000u) == 0) {
        return;
    }
    auto* target = static_cast<uint8_t*>(self->targetObj) + self->adjOffset;
    uintptr_t fnAddr = self->fnPtrOrSlot;
    if ((fnAddr & 1u) != 0) {
        auto* vtable = *reinterpret_cast<uint8_t**>(target);
        fnAddr = *reinterpret_cast<uintptr_t*>(vtable + fnAddr - 1);
    }
    using MemberFn = void (*)(void*, int32_t);
    reinterpret_cast<MemberFn>(fnAddr)(target, self->argValue);
}

// ============================================================================
// 5. 0x21df4d0 (5 B) - GXYebisMemberCallbackWithArg_Delete
// ============================================================================
void sub_21df4d0(void* self) {
    sub_2bbe588(self);
}

// ============================================================================
// 6. 0x21df4b0 (22 B) - GXYebisMemberCallbackWithArg_Dtor
// ============================================================================
void sub_21df4b0(GXYebisMemberCallbackWithArg* self) {
    uint32_t flags = self->refFlags & 0xbfffffffu;
    self->vtable = vtable_53b93b0;
    self->refFlags = flags;
}

// ============================================================================
// 7. 0x21df480 (37 B) - GXYebisMemberCallbackVoid_Invoke
// ============================================================================
void sub_21df480(GXYebisMemberCallbackVoid* self) {
    if ((self->refFlags & 0x40000000u) == 0) {
        return;
    }
    auto* target = static_cast<uint8_t*>(self->targetObj) + self->adjOffset;
    uintptr_t fnAddr = self->fnPtrOrSlot;
    if ((fnAddr & 1u) != 0) {
        auto* vtable = *reinterpret_cast<uint8_t**>(target);
        fnAddr = *reinterpret_cast<uintptr_t*>(vtable + fnAddr - 1);
    }
    using MemberVoidFn = void (*)(void*);
    reinterpret_cast<MemberVoidFn>(fnAddr)(target);
}

// ============================================================================
// 8. 0x21df470 (5 B) - GXYebisMemberCallbackVoid_Delete
// ============================================================================
void sub_21df470(void* self) {
    sub_2bbe588(self);
}

// ============================================================================
// 9. 0x21df450 (22 B) - GXYebisMemberCallbackVoid_Dtor
// ============================================================================
void sub_21df450(GXYebisMemberCallbackVoid* self) {
    uint32_t flags = self->refFlags & 0xbfffffffu;
    self->vtable = vtable_53b93b0;
    self->refFlags = flags;
}

// ============================================================================
// 10. 0x21df440 (5 B) - GXYebisRefCountedBase_Delete
// ============================================================================
void sub_21df440(void* self) {
    sub_2bbe588(self);
}

// ============================================================================
// 11. 0x21df430 (1 B) - GXYebisRefCountedBase_NoOp
// ============================================================================
void sub_21df430() {
    return;
}

// ============================================================================
// Helper for allocating GXYebisMemberCallbackWithArg in Debug Menu (0x21d9ea0)
// ============================================================================
static GXYebisMemberCallbackWithArg* CreateMemberCallbackWithArg(
    void* targetObj, uintptr_t fnAddr) noexcept {
    DLAllocator* alloc = g_UiCallbackAllocator_5540e08;
    if (!alloc || !alloc->vtable || !alloc->vtable->allocate_58) {
        return nullptr;
    }
    auto* cb = static_cast<GXYebisMemberCallbackWithArg*>(
        alloc->vtable->allocate_58(alloc, 0x30, 0x8));
    if (!cb) {
        return nullptr;
    }
    uint32_t flags = (cb->refFlags & 0x80000000u) | 0x40000000u;
    cb->refFlags    = flags;
    cb->vtable      = vtable_53b7020;
    cb->targetObj   = targetObj;
    cb->argValue    = 0;
    cb->fnPtrOrSlot = fnAddr;
    cb->adjOffset   = 0;
    return cb;
}

// ============================================================================
// 12. 0x21d9ea0 (21,542 B) - GXYebisPostProcessDirector_BuildDebugMenuGui
// ============================================================================
void sub_21d9ea0(void* self, void* ui) {
    auto* director = static_cast<uint8_t*>(self);
    auto* uiBytes  = static_cast<uint8_t*>(ui);

    const float f0_1    = BitsToFloat(0x3dcccccd); // 0x49297d8: 0.1f
    const float f10     = BitsToFloat(0x41200000); // 0x49297dc: 10.0f
    const float f0_01   = BitsToFloat(0x3c23d70a); // 0x49297e0: 0.01f
    const float f1      = BitsToFloat(0x3f800000); // 0x49297e4: 1.0f
    const float f1000   = BitsToFloat(0x447a0000); // 0x49297e8: 1000.0f
    const float f360    = BitsToFloat(0x43b40000); // 0x49297ec: 360.0f
    const float f2      = BitsToFloat(0x40000000); // 0x49297f0: 2.0f
    const float f1e_4   = BitsToFloat(0x38d1b717); // 0x49297f4: 0.0001f
    const float f1e_3   = BitsToFloat(0x3a83126f); // 0x49297f8: 0.001f
    const float f30     = BitsToFloat(0x41f00000); // 0x49297fc: 30.0f
    const float f60     = BitsToFloat(0x42700000); // 0x4929800: 60.0f
    const float fNeg1   = BitsToFloat(0xbf800000); // 0x4929804: -1.0f
    const float fNeg1e3 = BitsToFloat(0xba83126f); // 0x4929808: -0.001f
    const float f100    = BitsToFloat(0x42c80000); // 0x492980c: 100.0f
    const float f2e_3   = BitsToFloat(0x3b03126f); // 0x4929810: 0.002f
    const float f5      = BitsToFloat(0x40a00000); // 0x4929814: 5.0f
    const float f2e_5   = BitsToFloat(0x37a7c5ac); // 0x4929818: 0.00002f
    const float f0_9999 = BitsToFloat(0x3f7ff972); // 0x492981c: 0.9999f
    const float f1e_5   = BitsToFloat(0x3727c5ac); // 0x4929820: 0.00001f
    const float f0_5    = BitsToFloat(0x3f000000); // 0x4929824: 0.5f
    const float f8_5    = BitsToFloat(0x41080000); // 0x4929828: 8.5f
    const float f20     = BitsToFloat(0x41a00000); // 0x492982c: 20.0f
    const float fNeg10  = BitsToFloat(0xc1200000); // 0x4929830: -10.0f
    const float f7      = BitsToFloat(0x40e00000); // 0x4929834: 7.0f
    const float f50     = BitsToFloat(0x42480000); // 0x4929838: 50.0f
    const float fNeg90  = BitsToFloat(0xc2b40000); // 0x492983c: -90.0f
    const float f90     = BitsToFloat(0x42b40000); // 0x4929840: 90.0f
    const float fNeg180 = BitsToFloat(0xc3340000); // 0x4929844: -180.0f
    const float f180    = BitsToFloat(0x43340000); // 0x4929848: 180.0f
    const float f5000   = BitsToFloat(0x459c4000); // 0x492984c: 5000.0f
    const float f256    = BitsToFloat(0x43800000); // 0x4929850: 256.0f
    const float f89     = BitsToFloat(0x42b20000); // 0x4929854: 89.0f
    const float f100000 = BitsToFloat(0x47c35000); // 0x4929858: 100000.0f
    const float f10000  = BitsToFloat(0x461c4000); // 0x492985c: 10000.0f
    const float f0_99   = BitsToFloat(0x3f7d70a4); // 0x4929860: 0.99f
    const float f1_85   = BitsToFloat(0x3feccccd); // 0x4929864: 1.85f

    // ------------------------------------------------------------------------
    // Section 1: "Setting & Info"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblSettingInfo{0, u"Setting & Info"};
    if (sub_236c900(ui, &lblSettingInfo, 0) != nullptr) {
        lblSettingInfo.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21fec50(*reinterpret_cast<void**>(root + 0x5970), ui);
        }

        DLUiLabelDesc lblYebis{0, u"Yebis"};
        void* yebisGrp = sub_236d620(ui, &lblYebis);
        lblYebis.text = nullptr;

        DLUiLabelDesc lblResetCtx{0, u"Reset Context"};
        GXYebisMemberCallbackWithArg* resetCb = CreateMemberCallbackWithArg(
            director, reinterpret_cast<uintptr_t>(&sub_21dfc30));
        sub_235e4c0(yebisGrp, &lblResetCtx, resetCb);
        lblResetCtx.text = nullptr;

        // File selector widget for "Yebis2ParamFile.filterparam"
        DLAllocator* alloc = g_UiCallbackAllocator_5540e08;
        void* newFileWidget = nullptr;
        if (alloc && alloc->vtable && alloc->vtable->allocate_58) {
            newFileWidget = alloc->vtable->allocate_58(alloc, 0x140, 0x8);
        }
        if (newFileWidget) {
            auto** endPtr = *reinterpret_cast<void***>(uiBytes + 0x10);
            void* parentNode = *(endPtr - 1);
            DLUiLabelDesc emptyDesc{0, nullptr};
            sub_23362f0(newFileWidget, parentNode, &emptyDesc, 1, 0x201);

            auto* oldWidget = *reinterpret_cast<DLRefCountedObj**>(director + 0x18);
            if (oldWidget != newFileWidget) {
                if (oldWidget) {
                    ReleaseDLRefCountedNode(oldWidget);
                    *reinterpret_cast<void**>(director + 0x18) = nullptr;
                }
                *reinterpret_cast<void**>(director + 0x18) = newFileWidget;
                auto* nwRef = static_cast<DLRefCountedObj*>(newFileWidget);
                uint32_t f = nwRef->refFlags;
                uint32_t s = (f + 1u) & 0x7fffu;
                f = (f & 0xffff8000u) | s;
                uint32_t w = (f + 0x8000u) & 0x3fff8000u;
                nwRef->refFlags = (f & 0xc0007fffu) | w;
            }
            emptyDesc.text = nullptr;
            newFileWidget = *reinterpret_cast<void**>(director + 0x18);
        } else {
            auto* oldWidget = *reinterpret_cast<DLRefCountedObj**>(director + 0x18);
            if (oldWidget) {
                ReleaseDLRefCountedNode(oldWidget);
                *reinterpret_cast<void**>(director + 0x18) = nullptr;
            }
            newFileWidget = nullptr;
        }

        if (newFileWidget) {
            void* fileCtrl = *reinterpret_cast<void**>(static_cast<uint8_t*>(newFileWidget) + 0x110);
            sub_23743a0(fileCtrl);
            fileCtrl = *reinterpret_cast<void**>(static_cast<uint8_t*>(newFileWidget) + 0x110);
            sub_2374120(fileCtrl, u"Yebis2ParamFile.filterparam");
            auto* ctrlBytes = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(newFileWidget) + 0x110);
            *reinterpret_cast<uint64_t*>(ctrlBytes + 0x50) = *reinterpret_cast<uint64_t*>(ctrlBytes + 0x48);
        }

        DLUiLabelDesc lblIgnoreOutside{0, u"Ignore setup from outside"};
        sub_292f830(ui, &lblIgnoreOutside, reinterpret_cast<bool*>(director + 0x2710));
        lblIgnoreOutside.text = nullptr;

        DLUiLabelDesc lblParam{0, u"Parameter"};
        void* paramGrp0 = sub_236d620(ui, &lblParam);
        lblParam.text = nullptr;

        DLUiLabelDesc lblInitParam{0, u"InitParam"};
        GXYebisMemberCallbackWithArg* initCb = CreateMemberCallbackWithArg(
            director, reinterpret_cast<uintptr_t>(&sub_21d4230));
        sub_235e4c0(paramGrp0, &lblInitParam, initCb);
        lblInitParam.text = nullptr;

        DLUiLabelDesc lblParam2{0, u"Parameter"};
        void* paramGrp1 = sub_236d620(ui, &lblParam2);
        lblParam2.text = nullptr;

        DLUiLabelDesc lblSave{0, u"Save"};
        GXYebisMemberCallbackWithArg* saveCb = CreateMemberCallbackWithArg(
            director, reinterpret_cast<uintptr_t>(&sub_21d4240));
        sub_235e4c0(paramGrp1, &lblSave, saveCb);
        lblSave.text = nullptr;

        DLUiLabelDesc lblLoad{0, u"Load"};
        GXYebisMemberCallbackWithArg* loadCb = CreateMemberCallbackWithArg(
            director, reinterpret_cast<uintptr_t>(&sub_21d44f0));
        sub_235e4c0(paramGrp1, &lblLoad, loadCb);
        lblLoad.text = nullptr;

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        auto addToggle = [&](const char16_t* name, size_t off) {
            DLUiLabelDesc d{0, name};
            sub_292a730(ui, &d, reinterpret_cast<int32_t*>(gp + off));
            d.text = nullptr;
        };
        addToggle(u"Enable Filter", 0x000);
        addToggle(u"Use DepthTexture", 0x008);
        addToggle(u"Enable LensDistortion", 0x00c);
        addToggle(u"Enable ApertureairyDisc", 0x010);
        addToggle(u"Enable ApertureairyUnlimitedRadius", 0x014);
        addToggle(u"Enable MotionBlur-Dof", 0x018);
        addToggle(u"Enable MotionBlurUnlimitedrecursion", 0x01c);
        addToggle(u"Create MotionBlurInternalVelocityTex", 0x020);
        addToggle(u"Enable Gaussian", 0x024);
        addToggle(u"Enable FeedBackEffect", 0x028);
        addToggle(u"Enable Anti-Alias", 0x030);
        addToggle(u"Enable LightShaft", 0x034);
        addToggle(u"Enable Choromaticaberration", 0x038);
        addToggle(u"Enable AnamophicLensAspectratio", 0x040);
        addToggle(u"Enable AnamorphicLensAperture", 0x044);
        addToggle(u"Enable ColorGrading", 0x2b4);

        DLUiLabelDesc lblSceneLum{0, u"SceneLuminanceScale"};
        sub_292a590(ui, &lblSceneLum, reinterpret_cast<float*>(gp + 0x094), f0_1, f10, f0_01, f1);
        lblSceneLum.text = nullptr;

        DLUiEnumEntry localBlurShape[5];
        std::memcpy(localBlurShape, kBlurShapeEnumTable_5360190, sizeof(localBlurShape));
        DLUiLabelDesc lblBlurShape{0, u"BlurShape"};
        sub_2930840(ui, &lblBlurShape, reinterpret_cast<uint32_t*>(gp + 0x300), 5, localBlurShape);
        lblBlurShape.text = nullptr;

        DLUiLabelDesc lblGlareQual{0, u"GlareQuality "};
        sub_2934780(ui, &lblGlareQual, reinterpret_cast<int32_t*>(gp + 0x048), 0, 7, 1, f1);
        lblGlareQual.text = nullptr;

        DLUiLabelDesc lblDofQual{0, u"DepthOfFieldQuality"};
        sub_2934780(ui, &lblDofQual, reinterpret_cast<int32_t*>(gp + 0x04c), 0, 12, 1, f1);
        lblDofQual.text = nullptr;

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 2: "ToneMap"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblToneMap{0, u"ToneMap"};
    if (sub_236c900(ui, &lblToneMap, 0) != nullptr) {
        lblToneMap.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_2204780(*reinterpret_cast<void**>(root + 0x5978), ui);
        }

        auto* dynGp = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
        DLUiLabelDesc lblExposure{0, u"Exposure"};
        sub_292a590(ui, &lblExposure, reinterpret_cast<float*>(dynGp + 0x098), 0.0f, f10, f0_01, f1);
        lblExposure.text = nullptr;

        DLUiLabelDesc lblDetail{0, u"Detail Info"};
        if (sub_236c900(ui, &lblDetail, 0) != nullptr) {
            lblDetail.text = nullptr;
            uint8_t* gp = g_GlobalYebisFilterParam_55406e0;

            DLUiLabelDesc lblGamma{0, u"Gamma"};
            sub_292a590(ui, &lblGamma, reinterpret_cast<float*>(gp + 0x09c), 0.0f, f10, f0_01, f1);
            lblGamma.text = nullptr;

            DLUiEnumEntry localToneType[7];
            std::memcpy(localToneType, kToneTypeEnumTable_53601e0, sizeof(localToneType));
            DLUiLabelDesc lblToneType{0, u"ToneType"};
            sub_2934560(ui, &lblToneType, reinterpret_cast<int32_t*>(gp + 0x0a0), 7, localToneType);
            lblToneType.text = nullptr;

            DLUiLabelDesc lblToneFactor{0, u"ToneMappingFactor"};
            sub_292a590(ui, &lblToneFactor, reinterpret_cast<float*>(gp + 0x0a4), f0_01, f1000, f0_01, f1);
            lblToneFactor.text = nullptr;

            DLUiLabelDesc lblFilmic{0, u"FilmicContrast"};
            sub_292a590(ui, &lblFilmic, reinterpret_cast<float*>(gp + 0x0a8), 0.0f, f1, f0_01, f1);
            lblFilmic.text = nullptr;

            DLUiLabelDesc lblColorH{0, u"ColorH"};
            sub_292a590(ui, &lblColorH, reinterpret_cast<float*>(gp + 0x0c4), 0.0f, f360, f0_1, f1);
            lblColorH.text = nullptr;

            DLUiLabelDesc lblColorS{0, u"ColorS"};
            sub_292a590(ui, &lblColorS, reinterpret_cast<float*>(gp + 0x0c8), 0.0f, f2, f1e_4, f1);
            lblColorS.text = nullptr;

            DLUiLabelDesc lblColorV{0, u"ColorV"};
            sub_292a590(ui, &lblColorV, reinterpret_cast<float*>(gp + 0x0cc), 0.0f, f2, f1e_4, f1);
            lblColorV.text = nullptr;

            DLUiLabelDesc lblContrast{0, u"Contrust"};
            sub_292a590(ui, &lblContrast, reinterpret_cast<float*>(gp + 0x248), 0.0f, f2, f1e_4, f1);
            lblContrast.text = nullptr;

            // Sub-group: "AutoExposure"
            DLUiLabelDesc lblAutoExp{0, u"AutoExposure"};
            if (sub_236c900(ui, &lblAutoExp, 0) != nullptr) {
                lblAutoExp.text = nullptr;

                DLUiLabelDesc lblAutoExpToggle{0, u"AutoExposure"};
                sub_292a730(ui, &lblAutoExpToggle, reinterpret_cast<int32_t*>(gp + 0x050));
                lblAutoExpToggle.text = nullptr;

                auto addFloat = [&](const char16_t* nm, size_t off, float mn, float mx, float st, float df) {
                    DLUiLabelDesc d{0, nm};
                    sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), mn, mx, st, df);
                    d.text = nullptr;
                };
                addFloat(u"DelayFrame", 0x074, 0.0f, f1000, f0_01, f1);
                addFloat(u"MiddleGray", 0x054, f1e_3, f1, f0_01, f1);
                addFloat(u"ToneInfluencedByGlare", 0x058, f1e_3, f10, f0_01, f1);
                addFloat(u"MinExposure", 0x05c, f1e_3, f30, f0_01, f1);
                addFloat(u"MaxExposure", 0x060, f1e_3, f30, f0_01, f1);
                addFloat(u"Exposure MeteringWidth", 0x064, f1e_3, f1, f0_01, f1);
                addFloat(u"Exposure MeteringHeight", 0x068, f1e_3, f1, f0_01, f1);
                addFloat(u"Exposure MeteringOffsetX", 0x06c, 0.0f, f1, f0_01, f1);
                addFloat(u"Exposure MeteringOffsetY", 0x070, 0.0f, f1, f0_01, f1);
                addFloat(u"AdaptationSensitivity", 0x078, 0.0f, f1, f0_01, f1);
                addFloat(u"AdaptationScale", 0x07c, 0.0f, f1, f0_01, f1);
                addFloat(u"AdaptationSpeedLimit", 0x080, f1e_4, f60, f0_01, f1);
                addFloat(u"DarkAdaptationSensitivity", 0x084, fNeg1, f1, f0_01, f1);
                addFloat(u"DarkAdaptationScale", 0x088, fNeg1, f1, f0_01, f1);
                addFloat(u"DarkAdaptationSpeedLimit", 0x08c, fNeg1, f60, f0_01, f1);
                addFloat(u"fAdaptationScale / fDarkAdaptationScale", 0x090, f1e_3, f1, f0_01, f1);

                PopDbgUiGroupNode(ui);
            }
            PopDbgUiGroupNode(ui);
        }

        // Sub-group: "Vignetting"
        DLUiLabelDesc lblVig{0, u"Vignetting"};
        if (sub_236c900(ui, &lblVig, 0) != nullptr) {
            lblVig.text = nullptr;
            uint8_t* gp = g_GlobalYebisFilterParam_55406e0;

            DLUiLabelDesc lblVigPow{0, u" Vignette Power"};
            sub_292a590(ui, &lblVigPow, reinterpret_cast<float*>(gp + 0x0d0), fNeg1e3, f10, f0_01, f1);
            lblVigPow.text = nullptr;

            DLUiLabelDesc lblVigFov{0, u"VignetteFovDependence"};
            sub_292a590(ui, &lblVigFov, reinterpret_cast<float*>(gp + 0x0d4), 0.0f, f1, f0_01, f1);
            lblVigFov.text = nullptr;

            PopDbgUiGroupNode(ui);
        }

        // Sub-group: "Chromatic Aberration"
        DLUiLabelDesc lblCA{0, u"Chromatic Aberration"};
        if (sub_236c900(ui, &lblCA, 0) != nullptr) {
            lblCA.text = nullptr;
            uint8_t* gp = g_GlobalYebisFilterParam_55406e0;

            DLUiLabelDesc lblCAEnable{0, u"Enable"};
            sub_292a730(ui, &lblCAEnable, reinterpret_cast<int32_t*>(gp + 0x0ac));
            lblCAEnable.text = nullptr;

            DLUiEnumEntry waveSampleTable[3] = {
                {u"3 ", 3, 0},
                {u"5 ", 5, 0},
                {u"7 ", 7, 0}
            };
            DLUiLabelDesc lblWaveSample{0, u"WaveSample"};
            sub_2934560(ui, &lblWaveSample, reinterpret_cast<int32_t*>(gp + 0x0b0), 3, waveSampleTable);
            lblWaveSample.text = nullptr;

            auto addDisp = [&](const char16_t* nm, size_t off) {
                DLUiLabelDesc d{0, nm};
                sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), fNeg1, f1, f0_01, f1);
                d.text = nullptr;
            };
            addDisp(u"ToneLateralDispersion[0]", 0x0b4);
            addDisp(u"ToneLateralDispersion[1]", 0x0b8);
            addDisp(u"ToneUniformDispersion[0]", 0x0bc);
            addDisp(u"ToneUniformDispersion[1]", 0x0c0);

            PopDbgUiGroupNode(ui);
        }

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 3: "Bloom"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblBloom{0, u"Bloom"};
    if (sub_236c900(ui, &lblBloom, 0) != nullptr) {
        lblBloom.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21e65b0(*reinterpret_cast<void**>(root + 0x5980), ui);
        }

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        auto addFloat = [&](const char16_t* nm, size_t off, float mn, float mx, float st, float df) {
            DLUiLabelDesc d{0, nm};
            sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), mn, mx, st, df);
            d.text = nullptr;
        };

        addFloat(u"GlareLuminance", 0x0dc, 0.0f, f100, f0_01, f1);
        addFloat(u"GlareThreshold", 0x0e0, 0.0f, f10, f0_01, f1);
        addFloat(u"GlareSamplingThreshold", 0x304, f2e_3, f0_1, f1e_3, f1);
        addFloat(u"RemapFactor", 0x244, f0_1, f5, f0_01, f1);
        addFloat(u"GlareStarFileterThreshold", 0x0ec, f2e_5, f0_9999, f1e_5, f1);
        addFloat(u"GlareStarLengthFovDependence", 0x0f0, 0.0f, f1, f0_01, f1);
        addFloat(u"GlareStarSoftness", 0x0f4, 0.0f, f1, f0_01, f1);

        // Sub-group: "Detail Info"
        DLUiLabelDesc lblBloomDetail{0, u"Detail Info"};
        if (sub_236c900(ui, &lblBloomDetail, 0) != nullptr) {
            lblBloomDetail.text = nullptr;

            DLUiEnumEntry localGlareShape[15];
            std::memcpy(localGlareShape, kGlareShapeEnumTable_535ff00, sizeof(localGlareShape));
            DLUiLabelDesc lblGlareShape{0, u"GlareShape"};
            sub_2934560(ui, &lblGlareShape, reinterpret_cast<int32_t*>(gp + 0x0d8), 15, localGlareShape);
            lblGlareShape.text = nullptr;

            DLUiLabelDesc lblGlareLevels{0, u"GlareNumLevels"};
            sub_2934780(ui, &lblGlareLevels, reinterpret_cast<int32_t*>(gp + 0x0e4), -1, 16, 1, f1);
            lblGlareLevels.text = nullptr;

            addFloat(u"GlareBloomGaussianRadiusScale", 0x0f8, f0_5, f8_5, f0_01, f1);
            addFloat(u"GlareBloomLuminanceGamma", 0x0fc, 0.0f, f10, f0_01, f1);
            addFloat(u"GlareGhostConcentricDistortion", 0x100, 0.0f, f1, f0_01, f1);

            PopDbgUiGroupNode(ui);
        }

        // Sub-group: "Custom"
        DLUiLabelDesc lblCustomBloom{0, u"Custom"};
        if (sub_236c900(ui, &lblCustomBloom, 0) != nullptr) {
            lblCustomBloom.text = nullptr;

            addFloat(u"GlareLuminance", 0x308, 0.0f, f100, f0_01, f1);
            addFloat(u"BloomLuminance", 0x30c, 0.0f, f100, f0_01, f1);
            addFloat(u"BloomDispersion", 0x310, 0.0f, f10, f0_01, f1);

            DLUiLabelDesc lblBaseLevel{0, u"BloomDispersionBaseLevel"};
            sub_29316a0(ui, &lblBaseLevel, reinterpret_cast<uint32_t*>(gp + 0x314), 0, 12, 1, f1);
            lblBaseLevel.text = nullptr;

            addFloat(u"GhostLuminance", 0x318, 0.0f, f10, f0_01, f1);
            addFloat(u"GhostHaloLuminance", 0x31c, 0.0f, f10, f0_01, f1);
            addFloat(u"GhostDistortion", 0x320, 0.0f, f1, f0_01, f1);

            DLUiLabelDesc lblGhostSharp{0, u"GhostSharpness"};
            sub_2934910(ui, &lblGhostSharp, reinterpret_cast<int32_t*>(gp + 0x324));
            lblGhostSharp.text = nullptr;

            addFloat(u"StarLuminance", 0x328, 0.0f, f10, f0_01, f1);

            DLUiLabelDesc lblStarStreaks{0, u"StarStreaks"};
            sub_29316a0(ui, &lblStarStreaks, reinterpret_cast<uint32_t*>(gp + 0x32c), 0, 12, 1, f1);
            lblStarStreaks.text = nullptr;

            addFloat(u"StarLength", 0x330, 0.0f, f20, f0_1, f1);
            addFloat(u"StarSecondaryLength", 0x334, 0.0f, f100, f0_1, f1);

            DLUiLabelDesc lblStarRot{0, u"StarRotation"};
            sub_2934910(ui, &lblStarRot, reinterpret_cast<int32_t*>(gp + 0x338));
            lblStarRot.text = nullptr;

            addFloat(u"StarInclinationAngle", 0x33c, 0.0f, f360, f1, f1);
            addFloat(u"StarDispersion", 0x340, fNeg10, f10, f0_01, f1);

            DLUiLabelDesc lblStarForceDisp{0, u"StarForceDispersion"};
            sub_2934910(ui, &lblStarForceDisp, reinterpret_cast<int32_t*>(gp + 0x344));
            lblStarForceDisp.text = nullptr;

            addFloat(u"AfterimageLuminance", 0x348, 0.0f, f10, f0_01, f1);
            addFloat(u"AfterimageLength", 0x34c, 0.0f, f10, f0_01, f1);

            PopDbgUiGroupNode(ui);
        }

        // Sub-group: "AnamorphicLens"
        DLUiLabelDesc lblAnamorphic{0, u"AnamorphicLens"};
        if (sub_236c900(ui, &lblAnamorphic, 0) != nullptr) {
            lblAnamorphic.text = nullptr;

            DLUiLabelDesc lblAnamEnable{0, u"Enable"};
            sub_292a730(ui, &lblAnamEnable, reinterpret_cast<int32_t*>(gp + 0x104));
            lblAnamEnable.text = nullptr;

            addFloat(u"AnamorphicFlareLuminance", 0x108, 0.0f, f30, f0_01, f1);
            addFloat(u"AnamorphicFlareGhostLuminanceScale", 0x10c, 0.0f, f30, f0_01, f1);

            DLUiLabelDesc lblFlareCol{0, u"AnamorphicFlareColor"};
            void* wFlare = sub_236cf00(ui, &lblFlareCol, director + 0x26c0, 0x46);
            lblFlareCol.text = nullptr;
            if (wFlare) {
                auto* inner = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(wFlare) + 0x18);
                if (inner) inner[0x1d9] = 4;
            }

            DLUiLabelDesc lblGhostCol{0, u"AnamorphicGhostColor"};
            void* wGhost = sub_236cf00(ui, &lblGhostCol, director + 0x26e0, 0x46);
            lblGhostCol.text = nullptr;
            if (wGhost) {
                auto* inner = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(wGhost) + 0x18);
                if (inner) inner[0x1d9] = 4;
            }

            PopDbgUiGroupNode(ui);
        }

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 4: "LightShaft"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblLightShaft{0, u"LightShaft"};
    void* lsNode = sub_236c900(ui, &lblLightShaft, 0);
    if (lsNode != nullptr) {
        lblLightShaft.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21f6b90(*reinterpret_cast<void**>(root + 0x5988), ui);
        }

        // Walk child chain (+0x10) for node with (flags_50 & 2) != 0 and store weak ref at director + 0x2728
        auto* walkNode = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(lsNode) + 0x10);
        uint8_t* foundNode = nullptr;
        while (walkNode) {
            if ((walkNode[0x50] & 2u) != 0) {
                foundNode = walkNode;
                break;
            }
            walkNode = *reinterpret_cast<uint8_t**>(walkNode + 0x10);
        }
        auto* oldLsRef = *reinterpret_cast<DLRefCountedObj**>(director + 0x2728);
        if (reinterpret_cast<void*>(oldLsRef) != reinterpret_cast<void*>(foundNode)) {
            if (oldLsRef) {
                uint32_t oldF = oldLsRef->refFlags;
                uint32_t decW = (oldF >> 15) + 0x7fffu;
                uint32_t newW = decW & 0x7fffu;
                oldLsRef->refFlags = (oldF & 0xc0007fffu) | (newW << 15);
                if (newW == 0) {
                    oldLsRef->refFlags = oldF | 0x3fff8000u;
                    oldLsRef->vtable->destroy_20(oldLsRef);
                }
                *reinterpret_cast<void**>(director + 0x2728) = nullptr;
            }
            *reinterpret_cast<void**>(director + 0x2728) = foundNode;
            if (foundNode) {
                auto* wnRef = reinterpret_cast<DLRefCountedObj*>(foundNode);
                uint32_t f = wnRef->refFlags;
                uint32_t w = (f + 0x8000u) & 0x3fff8000u;
                wnRef->refFlags = (f & 0xc0007fffu) | w;
            }
        }

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        DLUiLabelDesc lblLsEnable{0, u"Enable"};
        sub_292a730(ui, &lblLsEnable, reinterpret_cast<int32_t*>(gp + 0x130));
        lblLsEnable.text = nullptr;

        DLUiLabelDesc lblLsColor{0, u"LightShaftColor"};
        void* wLsCol = sub_236cf00(ui, &lblLsColor, director + 0x2680, 0x46);
        lblLsColor.text = nullptr;
        if (wLsCol) {
            auto* inner = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(wLsCol) + 0x18);
            if (inner) inner[0x1d9] = 4;
        }

        auto addFloat = [&](const char16_t* nm, size_t off, float mn, float mx, float st, float df) {
            DLUiLabelDesc d{0, nm};
            sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), mn, mx, st, df);
            d.text = nullptr;
        };
        addFloat(u"ShaftLength", 0x144, 0.0f, f7, f0_01, f1);
        addFloat(u"GlareRatio", 0x148, 0.0f, f1, f0_01, f1);
        addFloat(u"AngleAttenuation", 0x154, 0.0f, f50, f0_01, f1);
        addFloat(u"NoiseMask", 0x158, 0.0f, f100, f0_01, f1);
        addFloat(u"NoiseFrequency", 0x15c, 0.0f, f10, f0_01, f1);
        addFloat(u"Rad-X", 0x160, fNeg90, f90, f0_01, f1);
        addFloat(u"Rad-Y", 0x164, fNeg180, f180, f0_01, f1);
        addFloat(u"MaskDepthThereshold", 0x24c, f1, f5000, f1, f1);
        addFloat(u"DiffractionRing", 0x168, 0.0f, f10, f0_01, f1);
        addFloat(u"DiffractionRingRadius", 0x16c, 0.0f, f1, f1e_3, f1);
        addFloat(u"DiffractionRingAttenuation", 0x170, 0.0f, f1, f0_01, f1);
        addFloat(u"DiffractionRingSpectrumOrder", 0x174, 0.0f, f1, f0_01, f1);

        DLUiLabelDesc lblRingColor{0, u"LightShaftRingColor"};
        void* wRingCol = sub_236cf00(ui, &lblRingColor, director + 0x26a0, 0x46);
        lblRingColor.text = nullptr;
        if (wRingCol) {
            auto* inner = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(wRingCol) + 0x18);
            if (inner) inner[0x1d9] = 4;
        }

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 5: "DepthOfField"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblDof{0, u"DepthOfField"};
    if (sub_236c900(ui, &lblDof, 0) != nullptr) {
        lblDof.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21ef5c0(*reinterpret_cast<void**>(root + 0x5990), ui);
        }

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        DLUiLabelDesc lblDofEnable{0, u"Enable"};
        sub_292a730(ui, &lblDofEnable, reinterpret_cast<int32_t*>(gp + 0x1ac));
        lblDofEnable.text = nullptr;

        DLUiLabelDesc lblBDebug{0, u"bDebug"};
        sub_292f830(ui, &lblBDebug, reinterpret_cast<bool*>(director + 0x2668));
        lblBDebug.text = nullptr;

        auto addFloat = [&](const char16_t* nm, size_t off, float mn, float mx, float st, float df) {
            DLUiLabelDesc d{0, nm};
            sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), mn, mx, st, df);
            d.text = nullptr;
        };
        addFloat(u"ApertuerF", 0x1b4, f1e_3, f256, f1e_3, f1);
        addFloat(u"DofFocusDistance", 0x1b0, 0.0f, f1000, f0_01, f1);

        // Sub-group: "Detail Info"
        DLUiLabelDesc lblDofDetail{0, u"Detail Info"};
        if (sub_236c900(ui, &lblDofDetail, 0) != nullptr) {
            lblDofDetail.text = nullptr;

            DLUiEnumEntry localCcdSize[17];
            std::memcpy(localCcdSize, kCcdSizeEnumTable_535fff0, sizeof(localCcdSize));
            DLUiLabelDesc lblCcd{0, u"CCD-Size"};
            sub_2937a00(ui, &lblCcd, reinterpret_cast<float*>(gp + 0x1c0), 17, localCcdSize);
            lblCcd.text = nullptr;

            DLUiLabelDesc lblAdapFactor{0, u"DofAdaptiveApertureFactor"};
            sub_292f830(ui, &lblAdapFactor, reinterpret_cast<bool*>(gp + 0x1b8));
            lblAdapFactor.text = nullptr;

            addFloat(u"DofAdaptiveApertureBaseFov", 0x1bc, f1, f89, f0_01, f1);
            addFloat(u"DofRotateScale", 0x1c4, 0.0f, f100000, f1e_4, f1);
            addFloat(u"DofRotateOffsetInRadians", 0x1c8, 0.0f, f360, f0_01, f1);

            DLUiLabelDesc lblFrontLevels{0, u"ApertureForntNumLevels "};
            sub_2934780(ui, &lblFrontLevels, reinterpret_cast<int32_t*>(gp + 0x1d0), -1, 7, 1, f1);
            lblFrontLevels.text = nullptr;

            DLUiLabelDesc lblBackLevels{0, u"ApertureBackNumLevels"};
            sub_2934780(ui, &lblBackLevels, reinterpret_cast<int32_t*>(gp + 0x1d4), -1, 7, 1, f1);
            lblBackLevels.text = nullptr;

            DLUiEnumEntry localDiaphragm[4];
            std::memcpy(localDiaphragm, kDiaphragmRotateEnumTable_5360100, sizeof(localDiaphragm));
            DLUiLabelDesc lblDiaphragm{0, u"DiaphragmRotate"};
            sub_2934560(ui, &lblDiaphragm, reinterpret_cast<int32_t*>(gp + 0x1cc), 4, localDiaphragm);
            lblDiaphragm.text = nullptr;

            DLUiEnumEntry localEdgeQual[5];
            std::memcpy(localEdgeQual, kDofEdgeQualityEnumTable_5360140, sizeof(localEdgeQual));
            DLUiLabelDesc lblEdgeQual{0, u"DofEdgeQuarity"};
            sub_2934560(ui, &lblEdgeQual, reinterpret_cast<int32_t*>(gp + 0x240), 5, localEdgeQual);
            lblEdgeQual.text = nullptr;

            addFloat(u"BackgroundMaskThreshold", 0x1d8, f1e_3, f1, f1e_3, f1);

            PopDbgUiGroupNode(ui);
        }

        // Sub-group: "CustomDof"
        DLUiLabelDesc lblCustomDof{0, u"CustomDof"};
        if (sub_236c900(ui, &lblCustomDof, 0) != nullptr) {
            lblCustomDof.text = nullptr;

            DLUiLabelDesc lblCustEnable{0, u"Enable"};
            sub_292a730(ui, &lblCustEnable, reinterpret_cast<int32_t*>(gp + 0x2d0));
            lblCustEnable.text = nullptr;

            DLUiLabelDesc lblNearEnable{0, u"Enable-Near-Custom"};
            sub_292a730(ui, &lblNearEnable, reinterpret_cast<int32_t*>(gp + 0x2d4));
            lblNearEnable.text = nullptr;

            addFloat(u"DofNearDepthStart", 0x2dc, 0.0f, f10000, f0_01, f1);
            addFloat(u"DofNearDepthEnd", 0x2e0, 0.0f, f100000, f0_01, f1);
            addFloat(u"DofNear CocSize", 0x2e4, 0.0f, f100, f0_01, f1);
            addFloat(u"DofNear CocScale", 0x2e8, 0.0f, f1, f0_01, f1);

            DLUiLabelDesc lblFarEnable{0, u"Enable-FarCustom"};
            sub_292a730(ui, &lblFarEnable, reinterpret_cast<int32_t*>(gp + 0x2d8));
            lblFarEnable.text = nullptr;

            addFloat(u"DofFarDepthStart", 0x2ec, 0.0f, f10000, f0_01, f1);
            addFloat(u"DofFarDepthEnd", 0x2f0, 0.0f, f100000, f0_01, f1);
            addFloat(u"DofFar CocSize", 0x2f4, 0.0f, f100, f0_01, f1);
            addFloat(u"DofFar CocScale", 0x2f8, 0.0f, f1, f0_01, f1);
            addFloat(u"DofFar DistanceThrethold", 0x2fc, 0.0f, f100000, f0_01, f1);

            PopDbgUiGroupNode(ui);
        }

        // Sub-group: "AutoFocus"
        DLUiLabelDesc lblAutoFocus{0, u"AutoFocus"};
        if (sub_236c900(ui, &lblAutoFocus, 0) != nullptr) {
            lblAutoFocus.text = nullptr;

            DLUiLabelDesc lblAfEnable{0, u"Enable"};
            sub_292a730(ui, &lblAfEnable, reinterpret_cast<int32_t*>(gp + 0x188));
            lblAfEnable.text = nullptr;

            DLUiLabelDesc lblDontPass{0, u"DonotPassOverFocus"};
            sub_292a730(ui, &lblDontPass, reinterpret_cast<int32_t*>(gp + 0x18c));
            lblDontPass.text = nullptr;

            addFloat(u"FocusSensitivity", 0x190, 0.0f, f1, f1e_3, f1);
            addFloat(u"FocusSpeedLimit", 0x194, 0.0f, f100, f0_01, f1);
            addFloat(u"FocusDelay", 0x198, 0.0f, f1000, f0_1, f1);
            addFloat(u"FocusDistanceRangeMin", 0x19c, 0.0f, f5000, f0_1, f1);
            addFloat(u"FocusDistanceRangeMax", 0x1a0, 0.0f, f5000, f0_1, f1);
            addFloat(u"FocusScreenPositionX", 0x1a4, 0.0f, f1, f0_01, f1);
            addFloat(u"FocusScreenPositionY", 0x1a8, 0.0f, f1, f0_01, f1);

            PopDbgUiGroupNode(ui);
        }

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 6: "Anti-Alias"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblAntiAlias{0, u"Anti-Alias"};
    if (sub_236c900(ui, &lblAntiAlias, 0) != nullptr) {
        lblAntiAlias.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21e17a0(*reinterpret_cast<void**>(root + 0x5998), ui);
        }

        auto* dynGp = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
        DLUiLabelDesc lblAaEnable{0, u"Enable"};
        sub_292a730(ui, &lblAaEnable, reinterpret_cast<int32_t*>(dynGp + 0x220));
        lblAaEnable.text = nullptr;

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 7: "MotionBlur"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblMotionBlur{0, u"MotionBlur"};
    if (sub_236c900(ui, &lblMotionBlur, 0) != nullptr) {
        lblMotionBlur.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21fa740(*reinterpret_cast<void**>(root + 0x59a0), ui);
        }

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        DLUiLabelDesc lblMbEnable{0, u"Enable"};
        sub_292a730(ui, &lblMbEnable, reinterpret_cast<int32_t*>(gp + 0x1dc));
        lblMbEnable.text = nullptr;

        DLUiLabelDesc lblDbgCamBlur{0, u"Dbg CameraBlur"};
        sub_292f830(ui, &lblDbgCamBlur, &g_DbgCameraBlurFlag_56c3d40);
        lblDbgCamBlur.text = nullptr;

        auto addFloat = [&](const char16_t* nm, size_t off, float mn, float mx, float st, float df) {
            DLUiLabelDesc d{0, nm};
            sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), mn, mx, st, df);
            d.text = nullptr;
        };
        addFloat(u"BlurTimeRatio", 0x1e4, f1e_3, f1, f1e_3, f1);
        addFloat(u"MaxBlurLength", 0x1e8, f1e_3, f5, f1e_3, f1);
        addFloat(u"MotionBlurScale", 0x350, f0_01, f5, f0_01, f1);

        DLUiLabelDesc lblNumSamples{0, u"NumBaseSamples"};
        sub_2934780(ui, &lblNumSamples, reinterpret_cast<int32_t*>(gp + 0x1ec), 3, 7, 1, f1);
        lblNumSamples.text = nullptr;

        DLUiLabelDesc lblMaxRecur{0, u"MaxRecurrences"};
        sub_2934780(ui, &lblMaxRecur, reinterpret_cast<int32_t*>(gp + 0x1f0), 1, 8, 1, f1);
        lblMaxRecur.text = nullptr;

        addFloat(u"SampleInterleaved", 0x1f4, f0_01, f2, f0_01, f1);

        // Sub-group: "Detail Info"
        DLUiLabelDesc lblMbDetail{0, u"Detail Info"};
        if (sub_236c900(ui, &lblMbDetail, 0) != nullptr) {
            lblMbDetail.text = nullptr;

            addFloat(u"CameraRotationPerspectiveThreshold", 0x1f8, 0.0f, f1, f0_01, f1);
            addFloat(u"CameraTranslationThreshold", 0x1fc, f0_01, f1000, f0_01, f1);
            addFloat(u"FalloffDistanceMin", 0x2ac, 0.0f, f10000, f0_01, f1);
            addFloat(u"FalloffDistanceMax", 0x2b0, 0.0f, f10000, f0_01, f1);

            DLUiLabelDesc lblBidir{0, u"Bidirectional PermissionEnable"};
            sub_292f830(ui, &lblBidir, reinterpret_cast<bool*>(gp + 0x204));
            lblBidir.text = nullptr;

            PopDbgUiGroupNode(ui);
        }

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 8: "FeedBackBlur"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblFbBlur{0, u"FeedBackBlur"};
    if (sub_236c900(ui, &lblFbBlur, 0) != nullptr) {
        lblFbBlur.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21f2e70(*reinterpret_cast<void**>(root + 0x59a8), ui);
        }

        auto* dynGp = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
        DLUiLabelDesc lblFbEnable{0, u"Enable"};
        void* fbToggleNode = sub_292a730(ui, &lblFbEnable, reinterpret_cast<int32_t*>(dynGp + 0x208));

        DLAllocator* alloc = g_UiCallbackAllocator_5540e08;
        GXYebisMemberCallbackVoid* fbCb = nullptr;
        if (alloc && alloc->vtable && alloc->vtable->allocate_58) {
            fbCb = static_cast<GXYebisMemberCallbackVoid*>(
                alloc->vtable->allocate_58(alloc, 0x28, 0x8));
            if (fbCb) {
                uint32_t flags = (fbCb->refFlags & 0x80000000u) | 0x40000000u;
                fbCb->refFlags    = flags;
                fbCb->vtable      = vtable_53b6fe0;
                fbCb->targetObj   = director;
                fbCb->fnPtrOrSlot = reinterpret_cast<uintptr_t>(&sub_21d4210);
                fbCb->adjOffset   = 0;
            }
        }
        if (fbToggleNode) {
            auto* wBytes = static_cast<uint8_t*>(fbToggleNode);
            auto* oldCb = *reinterpret_cast<DLRefCountedObj**>(wBytes + 0x10);
            if (reinterpret_cast<void*>(oldCb) != reinterpret_cast<void*>(fbCb)) {
                if (oldCb) {
                    ReleaseDLRefCountedNode(oldCb);
                    *reinterpret_cast<void**>(wBytes + 0x10) = nullptr;
                }
                *reinterpret_cast<void**>(wBytes + 0x10) = fbCb;
                if (fbCb) {
                    uint32_t f = fbCb->refFlags;
                    uint32_t s = (f + 1u) & 0x7fffu;
                    f = (f & 0xffff8000u) | s;
                    uint32_t w = (f + 0x8000u) & 0x3fff8000u;
                    fbCb->refFlags = (f & 0xc0007fffu) | w;
                }
            }
        }
        lblFbEnable.text = nullptr;

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        auto addFloat = [&](const char16_t* nm, size_t off, float mn, float mx, float st, float df) {
            DLUiLabelDesc d{0, nm};
            sub_292a590(ui, &d, reinterpret_cast<float*>(gp + off), mn, mx, st, df);
            d.text = nullptr;
        };
        addFloat(u"FeedbackWeight", 0x20c, 0.0f, f0_99, f0_01, f1);
        addFloat(u"FeedbackUnitTimeInSeconds", 0x214, 0.0f, f1, f0_01, f1);
        addFloat(u"FeedbackAspectRatio", 0x218, 0.0f, f1_85, f0_01, f1);

        PopDbgUiGroupNode(ui);
    }

    // ------------------------------------------------------------------------
    // Section 9: "ColorGrading"
    // ------------------------------------------------------------------------
    DLUiLabelDesc lblColorGrad{0, u"ColorGrading"};
    if (sub_236c900(ui, &lblColorGrad, 0) != nullptr) {
        lblColorGrad.text = nullptr;
        auto* root = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        if (root) {
            sub_21ea4c0(*reinterpret_cast<void**>(root + 0x59b0), ui);
        }

        uint8_t* gp = g_GlobalYebisFilterParam_55406e0;
        DLUiLabelDesc lblCgEnable{0, u"Enable"};
        sub_292a730(ui, &lblCgEnable, reinterpret_cast<int32_t*>(gp + 0x2b8));
        lblCgEnable.text = nullptr;

        DLUiLabelDesc lblDrawDbgLut{0, u"Draw-DbgLut"};
        sub_292f830(ui, &lblDrawDbgLut, reinterpret_cast<bool*>(director + 0x21a8));
        lblDrawDbgLut.text = nullptr;

        DLUiLabelDesc lblDbgLutBase{0, u"DbgLut - Base"};
        sub_292f830(ui, &lblDbgLutBase, reinterpret_cast<bool*>(director + 0x21a9));
        lblDbgLutBase.text = nullptr;

        DLUiLabelDesc lblPrefixA{0, u"LutSourcePrefixA : "};
        sub_293f750(ui, &lblPrefixA, director + 0x2138);
        lblPrefixA.text = nullptr;

        DLUiLabelDesc lblPrefixB{0, u"LutSourcePrefixB : "};
        sub_293f750(ui, &lblPrefixB, director + 0x2170);
        lblPrefixB.text = nullptr;

        DLUiLabelDesc lblLutId{0, u"LutSourceId : "};
        sub_2934780(ui, &lblLutId, reinterpret_cast<int32_t*>(gp + 0x2bc), -1, 10000, 1, f1);
        lblLutId.text = nullptr;

        DLUiLabelDesc lblLutIdA{0, u"LutSourceIdA : "};
        sub_2934780(ui, &lblLutIdA, reinterpret_cast<int32_t*>(gp + 0x2c0), -1, 10000, 1, f1);
        lblLutIdA.text = nullptr;

        DLUiLabelDesc lblLutIdB{0, u"LutSourceIdB : "};
        sub_2934780(ui, &lblLutIdB, reinterpret_cast<int32_t*>(gp + 0x2c4), -1, 10000, 1, f1);
        lblLutIdB.text = nullptr;

        DLUiLabelDesc lblLutBlend{0, u"LutBlend : "};
        sub_292a590(ui, &lblLutBlend, reinterpret_cast<float*>(gp + 0x2c8), 0.0f, f1, f0_01, f1);
        lblLutBlend.text = nullptr;

        DLUiLabelDesc lblValidA{0, u"LutSourceIdA : Valid A Tex"};
        sub_219db10(ui, &lblValidA, gp + 0x2cc);
        lblValidA.text = nullptr;

        DLUiLabelDesc lblValidB{0, u"LutSourceIdB : Valid B Tex"};
        sub_219db10(ui, &lblValidB, gp + 0x2cd);
        lblValidB.text = nullptr;

        PopDbgUiGroupNode(ui);
    }
}

// ============================================================================
// 13. 0x21d9e50 (73 B) - GXYebisPostProcessDirector_BindMotionBlurVelocityAndDepthBuffers
// ============================================================================
void sub_21d9e50(void* self, void* texA, void* texB) {
    auto* director = static_cast<uint8_t*>(self);
    void* ppfxCtx = *reinterpret_cast<void**>(director + 0x2670);
    if (ppfxCtx != nullptr) {
        if (texB != nullptr) {
            auto* innerB = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(texB) + 0x28);
            sub_bb5f10(ppfxCtx, innerB + 0x10);
        }
        if (texA != nullptr) {
            auto* innerA = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(texA) + 0x48);
            sub_bb5f10(ppfxCtx, innerA + 0x10);
        }
    }
}

// ============================================================================
// 14. 0x21d9df0 (92 B) - GXYebisPostProcessDirector_WaitGpuFenceAndShutdown
// ============================================================================
void sub_21d9df0(void* self) {
    auto* director = static_cast<uint8_t*>(self);
    if (director[0x2718] == 0) {
        return;
    }

    auto* dev = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
    if (dev != nullptr) {
        auto* syncParent = *reinterpret_cast<uint8_t**>(dev + 0x348);
        if (syncParent != nullptr) {
            auto* syncObj = syncParent + 0x68;
            auto* vtable = *reinterpret_cast<uintptr_t**>(syncObj);
            if (vtable != nullptr && vtable[3] != 0) {
                using WaitSyncFn = void (*)(void*, int64_t);
                reinterpret_cast<WaitSyncFn>(vtable[3])(syncObj, -1);
            }
        }
        int64_t slotIdx = *reinterpret_cast<int32_t*>(dev + 0x68c);
        *reinterpret_cast<uint32_t*>(dev + slotIdx * 16 + 0x670) = 0;
    }

    sub_21d2dd0(self);
}

// ============================================================================
// Helper: 4x4 Matrix Inverse using AVX/SSE3 (exact instruction sequence at
// 0x21d6b2f..0x21d6d4c and 0x21d6d69..0x21d6f7f in sub_21d5f80)
// ============================================================================
static inline void InvertMatrix4x4Simd(float* dst, const float* src) noexcept {
    __m128 xmm0 = _mm_loadu_ps(src + 0);
    __m128 xmm1 = _mm_loadu_ps(src + 4);
    __m128 xmm2 = _mm_loadu_ps(src + 8);
    __m128 xmm3 = _mm_loadu_ps(src + 12);

    __m128 xmm4  = _mm_movelh_ps(xmm0, xmm1);
    xmm0         = _mm_movehl_ps(xmm1, xmm0);
    xmm1         = _mm_movelh_ps(xmm2, xmm3);
    xmm2         = _mm_movehl_ps(xmm3, xmm2);

    __m128 xmm15 = _mm_shuffle_ps(xmm4, xmm1, 0x88);
    xmm1         = _mm_shuffle_ps(xmm4, xmm1, 0xDD);
    xmm4         = _mm_shuffle_ps(xmm0, xmm2, 0x88);
    xmm0         = _mm_shuffle_ps(xmm0, xmm2, 0xDD);

    __m128 xmm8  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x50));
    __m128 xmm9  = _mm_movehl_ps(xmm0, xmm0);
    __m128 xmm6  = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x50));
    __m128 xmm7  = _mm_movehl_ps(xmm1, xmm1);
    xmm2         = _mm_shuffle_ps(xmm4, xmm15, 0x88);
    __m128 xmm5  = _mm_shuffle_ps(xmm0, xmm1, 0xDD);

    xmm8         = _mm_mul_ps(xmm8, xmm9);
    __m128 xmm10 = _mm_mul_ps(xmm6, xmm7);
    xmm9         = _mm_mul_ps(xmm2, xmm5);

    __m128 xmm11 = _mm_movehl_ps(xmm4, xmm4);
    xmm7         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x50));
    __m128 xmm12 = _mm_movehl_ps(xmm15, xmm15);
    xmm6         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x50));
    xmm5         = _mm_shuffle_ps(xmm4, xmm15, 0xDD);
    xmm2         = _mm_shuffle_ps(xmm0, xmm1, 0x88);

    xmm7         = _mm_mul_ps(xmm11, xmm7);
    xmm6         = _mm_mul_ps(xmm12, xmm6);
    xmm2         = _mm_mul_ps(xmm5, xmm2);

    xmm5         = _mm_sub_ps(xmm8, xmm7);
    xmm6         = _mm_sub_ps(xmm10, xmm6);
    xmm2         = _mm_sub_ps(xmm9, xmm2);

    xmm7         = _mm_shuffle_ps(xmm5, xmm2, 0x5D);
    xmm8         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x49));
    xmm9         = _mm_shuffle_ps(xmm7, xmm5, 0x32);
    xmm10        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x12));
    xmm11        = _mm_shuffle_ps(xmm7, xmm5, 0x99);
    xmm7         = _mm_shuffle_ps(xmm6, xmm2, 0xFD);
    xmm12        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x49));
    __m128 xmm13 = _mm_shuffle_ps(xmm7, xmm6, 0x32);
    __m128 xmm14 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x12));
    xmm7         = _mm_shuffle_ps(xmm7, xmm6, 0x99);

    xmm8         = _mm_mul_ps(xmm8, xmm9);
    xmm9         = _mm_mul_ps(xmm10, xmm11);
    xmm10        = _mm_mul_ps(xmm12, xmm13);
    xmm11        = _mm_mul_ps(xmm14, xmm7);

    xmm7         = _mm_shuffle_ps(xmm5, xmm2, 0x04);
    xmm12        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x9E));
    xmm13        = _mm_shuffle_ps(xmm5, xmm7, 0x93);
    xmm14        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x7B));
    xmm3         = _mm_shuffle_ps(xmm5, xmm7, 0x26);
    xmm7         = _mm_shuffle_ps(xmm6, xmm2, 0xA4);

    xmm12        = _mm_mul_ps(xmm12, xmm13);
    xmm13        = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x9E));
    xmm14        = _mm_mul_ps(xmm14, xmm3);
    xmm3         = _mm_shuffle_ps(xmm6, xmm7, 0x93);
    xmm13        = _mm_mul_ps(xmm13, xmm3);
    xmm3         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x7B));
    xmm7         = _mm_shuffle_ps(xmm6, xmm7, 0x26);
    xmm3         = _mm_mul_ps(xmm3, xmm7);

    xmm7         = _mm_sub_ps(xmm8, xmm12);
    xmm8         = _mm_sub_ps(xmm9, xmm14);
    xmm9         = _mm_sub_ps(xmm10, xmm13);
    xmm12        = _mm_sub_ps(xmm11, xmm3);

    xmm10        = _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(xmm2), _mm_castps_si128(xmm5), 8));
    xmm11        = _mm_shuffle_ps(xmm5, xmm2, 0x4C);
    xmm5         = _mm_movehl_ps(xmm2, xmm6);
    xmm2         = _mm_shuffle_ps(xmm6, xmm2, 0xEC);

    xmm1         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0x33));
    xmm6         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm10), 0x2C));
    xmm1         = _mm_mul_ps(xmm1, xmm6);

    xmm6         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm15), 0x8D));
    xmm3         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm11), 0x93));
    xmm3         = _mm_mul_ps(xmm6, xmm3);

    xmm0         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0x33));
    xmm5         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm5), 0x2C));
    xmm0         = _mm_mul_ps(xmm0, xmm5);

    xmm4         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm4), 0x8D));
    xmm2         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0x93));
    xmm2         = _mm_mul_ps(xmm4, xmm2);

    xmm4         = _mm_sub_ps(xmm7, xmm1);
    xmm1         = _mm_add_ps(xmm1, xmm7);
    xmm5         = _mm_add_ps(xmm3, xmm8);
    xmm3         = _mm_sub_ps(xmm8, xmm3);
    xmm6         = _mm_sub_ps(xmm9, xmm0);
    xmm0         = _mm_add_ps(xmm0, xmm9);
    xmm7         = _mm_add_ps(xmm2, xmm12);
    xmm2         = _mm_sub_ps(xmm12, xmm2);

    xmm1         = _mm_shuffle_ps(xmm1, xmm4, 0xD8);
    xmm3         = _mm_shuffle_ps(xmm3, xmm5, 0xD8);
    xmm0         = _mm_shuffle_ps(xmm0, xmm6, 0xD8);
    xmm2         = _mm_shuffle_ps(xmm2, xmm7, 0xD8);

    xmm1         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm1), 0xD8));
    xmm3         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm3), 0xD8));
    xmm0         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm0), 0xD8));
    xmm2         = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm2), 0xD8));

    xmm4         = _mm_mul_ps(xmm15, xmm1);
    xmm4         = _mm_hadd_ps(xmm4, xmm4);
    xmm4         = _mm_hadd_ps(xmm4, xmm4);
    xmm5         = _mm_set1_ps(1.0f); // 0x4920dd0
    xmm4         = _mm_div_ps(xmm5, xmm4);

    xmm1         = _mm_mul_ps(xmm4, xmm1);
    xmm3         = _mm_mul_ps(xmm4, xmm3);
    xmm0         = _mm_mul_ps(xmm4, xmm0);
    xmm2         = _mm_mul_ps(xmm4, xmm2);

    _mm_storeu_ps(dst + 0,  xmm1);
    _mm_storeu_ps(dst + 4,  xmm3);
    _mm_storeu_ps(dst + 8,  xmm0);
    _mm_storeu_ps(dst + 12, xmm2);
}

// ============================================================================
// 15. 0x21d5f80 (15,969 B) - GXYebisPostProcessDirector_ExecutePostProcessAndLutCompute
// ============================================================================
void sub_21d5f80(void* self, void* renderCmdCtx, void* filterParam) {
    auto* director = static_cast<uint8_t*>(self);
    auto* cmdCtx   = static_cast<uint8_t*>(renderCmdCtx);
    auto* param    = static_cast<uint8_t*>(filterParam);

    const float f1       = BitsToFloat(0x3f800000); // 0x4929790: 1.0f
    const float fPiDiv6  = BitsToFloat(0x3f060a92); // 0x4929798: pi/6
    const float f1Div60  = BitsToFloat(0x3c888889); // 0x492979c: 1.0f/60.0f
    const float fNegMax  = BitsToFloat(0xff7fffff); // 0x49297a0: -FLT_MAX
    const float fPi      = BitsToFloat(0x40490fdb); // 0x49297a4: pi
    const float f180     = BitsToFloat(0x43340000); // 0x49297a8: 180.0f
    const float fDeg2Rad = BitsToFloat(0x3c8efa35); // 0x49297ac: pi/180
    const float f0_3     = BitsToFloat(0x3e99999a); // 0x49297b0: 0.3f
    const float f0_9995  = BitsToFloat(0x3f7fe143); // 0x49297b4: 0.99953097f
    const float f0_001   = BitsToFloat(0x3a83126f); // 0x49297b8: 0.001f
    const float f0_0001  = BitsToFloat(0x38d1b717); // 0x49297bc: 0.0001f
    const float fNeg1    = BitsToFloat(0xbf800000); // 0x49297c0: -1.0f
    const float f16      = BitsToFloat(0x41800000); // 0x49297c8: 16.0f
    const float fNeg256  = BitsToFloat(0xc3800000); // 0x49297cc: -256.0f
    const float f1e_7    = BitsToFloat(0x33d6bf95); // 0x49297d0: 1e-7f
    const float fInv2047 = BitsToFloat(0x3a001002); // 0x49297d4: 1.0f/2047.0f

    if (*reinterpret_cast<void**>(director + 0x2670) == nullptr) {
        return;
    }

    auto* gnmCtxPtr = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);

    // ------------------------------------------------------------------------
    // Part A: 2D-to-3D Color Grading LUT Compute Pass
    //         ("LutCopy2DTo3DCs.cpo" / "LutBlend2DTo3DCs.cpo", dispatch 1x1x16)
    // ------------------------------------------------------------------------
    if (*reinterpret_cast<int32_t*>(param + 0x2b8) != 0 &&
        *reinterpret_cast<int32_t*>(param + 0x2b4) != 0 &&
        param[0x2cc] != 0 &&
        *reinterpret_cast<void**>(director + 0x2198) != nullptr) {

        void* uavLut3d = nullptr;
        auto* lut3dObj = *reinterpret_cast<uint8_t**>(director + 0x2180);
        if (lut3dObj != nullptr) {
            uavLut3d = *reinterpret_cast<void**>(lut3dObj + 0x40);
        }

        auto* stageBinder = gnmCtxPtr + 0xb730;
        sub_2168a20(gnmCtxPtr, stageBinder, uavLut3d, 0, 0, nullptr, 0, 0);

        auto bindSrvSlot = [&](int32_t slot, void* srvDesc) {
            auto* sb = gnmCtxPtr + 0xb730;
            int64_t baseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b38)) * 0x5c8;
            int64_t slotOff = baseOff + static_cast<int64_t>(slot) * 0x10;
            *reinterpret_cast<void**>(sb + slotOff + 0x3660) = srvDesc;
            *reinterpret_cast<int32_t*>(sb + slotOff + 0x3668) = -1;
            uint32_t bit = 1u << (slot & 31);
            *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3b20) |= bit;
            *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x20u;
        };

        if (param[0x2cd] == 0) {
            // Single 2D LUT copy -> 3D LUT
            void* srvA = nullptr;
            auto* texObjA = *reinterpret_cast<uint8_t**>(director + 0x2188);
            if (texObjA != nullptr) {
                srvA = *reinterpret_cast<void**>(texObjA + 0x30);
            }
            bindSrvSlot(0, srvA);

            void* shdrCont = *reinterpret_cast<void**>(director + 0x2198);
            auto* csEntry = static_cast<uint8_t*>(
                sub_2274e20(shdrCont, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b02ULL))));
            void* csProg  = csEntry ? *reinterpret_cast<void**>(csEntry + 0x48) : nullptr;
            auto* sb = gnmCtxPtr + 0xb730;
            if (*reinterpret_cast<void**>(sb + 0x3578) != csProg) {
                *reinterpret_cast<void**>(sb + 0x3578) = csProg;
                *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x2000u;
            }
        } else {
            // Blended 2D LUT A + B -> 3D LUT
            void* srvA = nullptr;
            auto* texObjA = *reinterpret_cast<uint8_t**>(director + 0x2188);
            if (texObjA != nullptr) {
                srvA = *reinterpret_cast<void**>(texObjA + 0x30);
            }
            bindSrvSlot(0, srvA);

            void* srvB = nullptr;
            auto* texObjB = *reinterpret_cast<uint8_t**>(director + 0x2190);
            if (texObjB != nullptr) {
                srvB = *reinterpret_cast<void**>(texObjB + 0x30);
            }
            bindSrvSlot(1, srvB);

            // Allocate 16-byte constant buffer for blend weight (param + 0x2c8)
            void* cbGpuAddr = nullptr;
            auto* cbAlloc = *reinterpret_cast<DLAllocator**>(gnmCtxPtr + 0x38);
            if (cbAlloc && cbAlloc->vtable && cbAlloc->vtable->allocate_58) {
                cbGpuAddr = cbAlloc->vtable->allocate_58(cbAlloc, 0x10, 0x10);
            }
            if (cbGpuAddr != nullptr) {
                uint32_t blendBits = *reinterpret_cast<uint32_t*>(param + 0x2c8);
                uint32_t cbData[4] = {blendBits, blendBits, blendBits, blendBits};
                sub_2bbe578(cbGpuAddr, cbData, 0x10);
            }
            auto* sb = gnmCtxPtr + 0xb730;
            int64_t baseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b38)) * 0x5c8;
            *reinterpret_cast<void**>(sb + baseOff + 0x3a60) = cbGpuAddr;
            *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3a68) = 0x10u;
            *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3b2c) |= 1u;
            *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x20u;

            void* shdrCont = *reinterpret_cast<void**>(director + 0x2198);
            auto* csEntry = static_cast<uint8_t*>(
                sub_2274e20(shdrCont, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b2aULL))));
            void* csProg  = csEntry ? *reinterpret_cast<void**>(csEntry + 0x48) : nullptr;
            sb = gnmCtxPtr + 0xb730;
            if (*reinterpret_cast<void**>(sb + 0x3578) != csProg) {
                *reinterpret_cast<void**>(sb + 0x3578) = csProg;
                *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x2000u;
            }
        }

        // Dispatch Compute (1, 1, 16)
        auto* dispHelper = gnmCtxPtr + 0x50;
        auto* gfxc       = gnmCtxPtr + 0x10;
        if (dispHelper[0x50] == 0) {
            sub_26af700(gfxc);
            dispHelper[0x50] = 1;
        }
        sub_26cbe50(dispHelper, gnmCtxPtr, gnmCtxPtr + 0xb730);
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
            gx_direct_vk::EmitDispatchDirect(gnmCtxPtr + 0x10, 1u, 1u, 16u);
        } else {
            sub_1074c50(gnmCtxPtr + 0x10, 1u, 1u, 16u);
        }
        sub_26ccf20(dispHelper, gnmCtxPtr, gnmCtxPtr + 0xb730);

        // Clear UAV slot 0
        sub_2168a20(gnmCtxPtr, gnmCtxPtr + 0xb730, nullptr, 0, 0, nullptr, 0, 0);

        // Clear SRV slot 0 (and slot 1 if blended)
        bindSrvSlot(0, nullptr);
        if (param[0x2cd] != 0) {
            bindSrvSlot(1, nullptr);
        }
    }

    // ------------------------------------------------------------------------
    // Part B: Render Target Binding, Viewport Sync & Yebis2 Parameter Setup
    // ------------------------------------------------------------------------
    if (cmdCtx[0x208] != 0) {
        sub_219fcf0(cmdCtx);
    }

    auto* srcSurfWrap0 = *reinterpret_cast<uint8_t**>(param + 0x370);
    void* rtDesc0 = srcSurfWrap0 ? *reinterpret_cast<void**>(srcSurfWrap0 + 0x20) : nullptr;
    sub_e46f40(cmdCtx + 0x08, rtDesc0, 0, 0);
    sub_e47290(cmdCtx + 0x08, 0);

    auto* rtViewObj = static_cast<uint8_t*>(sub_e47840(cmdCtx + 0x08));
    auto* rtTexObj  = *reinterpret_cast<DLRefCountedObj**>(rtViewObj + 0x08);
    uint32_t rtW = rtTexObj->vtable->getWidth_48(rtTexObj, 0);
    uint32_t rtH = rtTexObj->vtable->getHeight_50(rtTexObj, 0);

    auto* vpState = reinterpret_cast<int32_t*>(cmdCtx + 0x168);
    if (vpState[0] != 0 || vpState[1] != 0 ||
        static_cast<uint32_t>(vpState[2]) != rtW ||
        static_cast<uint32_t>(vpState[3]) != rtH ||
        *reinterpret_cast<float*>(cmdCtx + 0x178) != 0.0f ||
        *reinterpret_cast<float*>(cmdCtx + 0x17c) != f1) {
        vpState[0] = 0;
        vpState[1] = 0;
        vpState[2] = static_cast<int32_t>(rtW);
        vpState[3] = static_cast<int32_t>(rtH);
        *reinterpret_cast<uint32_t*>(cmdCtx + 0x178) = 0;
        *reinterpret_cast<float*>(cmdCtx + 0x17c)    = f1;
        cmdCtx[0x209] = 1;
    }

    auto* scState = reinterpret_cast<int32_t*>(cmdCtx + 0x180);
    if (scState[0] != 0 || scState[1] != 0 ||
        static_cast<uint32_t>(scState[2]) != rtW ||
        static_cast<uint32_t>(scState[3]) != rtH) {
        scState[0] = 0;
        scState[1] = 0;
        scState[2] = static_cast<int32_t>(rtW);
        scState[3] = static_cast<int32_t>(rtH);
        cmdCtx[0x20a] = 1;
    }

    sub_21a1a00(cmdCtx);
    sub_21a1c50(cmdCtx);

    gnmCtxPtr = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
    auto* rtStateObj = gnmCtxPtr + 0xb558;
    if (rtStateObj[0x74] != 0) {
        uint8_t clearDesc[16] = {};
        clearDesc[0] = 1;
        sub_26d3070(rtStateObj, gnmCtxPtr, clearDesc);
    }

    auto* syncStateObj = gnmCtxPtr + 0xb478;
    if (*reinterpret_cast<uint32_t*>(syncStateObj + 0x0c) != 0 ||
        *reinterpret_cast<uint32_t*>(syncStateObj + 0x20) != 0) {
        sub_26b9850(syncStateObj, gnmCtxPtr + 0x10);
    }

    auto* pipeStateObj = gnmCtxPtr + 0x50;
    if (pipeStateObj[0x50] != 0) {
        sub_26afa00(gnmCtxPtr + 0x10);
        pipeStateObj[0x50] = 0;
    }
    sub_26cd190(pipeStateObj, gnmCtxPtr);

    void* gfxc = gnmCtxPtr + 0x10;
    uint64_t flipIdx = g_YebisBufferFlipIndex_56a6d20;
    uint8_t* activeGpuCtx = g_YebisDoubleBufArea_56a6d20 + flipIdx * 0xe7b8 + 0x08;
    g_ActiveYebisGpuCtx_56c3cb8  = activeGpuCtx;
    g_ActiveYebisGfxc0_56c3cc0   = gfxc;
    g_ActiveYebisGfxc1_56c3cc8   = gfxc;
    uint64_t cmdBase = *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(gfxc) + 0x08);
    g_ActiveYebisCmdBase0_56c3cd0 = cmdBase;
    g_ActiveYebisCmdBase1_56c3cd8 = cmdBase;

    sub_10946d0(activeGpuCtx);
    sub_26bba10(gnmCtxPtr + 0xb518);
    sub_26d3300(gnmCtxPtr + 0xb528);
    sub_26d2d20(gnmCtxPtr + 0xb558);

    auto* outRtWrap = *reinterpret_cast<uint8_t**>(param + 0x370);
    auto* outRtDesc = *reinterpret_cast<uint8_t**>(outRtWrap + 0x20);
    sub_108c520(g_ActiveYebisGpuCtx_56c3cb8, 0, outRtDesc + 0x10);

    syncStateObj = gnmCtxPtr + 0xb478;
    if (!sub_26b9a50(syncStateObj)) {
        sub_26b9a40(syncStateObj);
    }

    auto* inColorWrap = *reinterpret_cast<uint8_t**>(param + 0x368);
    auto* inColorTex  = *reinterpret_cast<DLRefCountedObj**>(inColorWrap + 0x10);
    uint32_t inColorW = inColorTex->vtable->getWidth_48(inColorTex, 0);
    uint32_t inColorH = inColorTex->vtable->getHeight_50(inColorTex, 0);
    auto* inColorGnm  = *reinterpret_cast<uint8_t**>(inColorWrap + 0x48) + 0x10;
    uint32_t inColorFmt = sub_1089960(inColorGnm);

    auto* inDepthWrap = *reinterpret_cast<uint8_t**>(param + 0x378);
    auto* inDepthTex  = *reinterpret_cast<DLRefCountedObj**>(inDepthWrap + 0x10);
    uint32_t inDepthW = inDepthTex->vtable->getWidth_48(inDepthTex, 0);
    uint32_t inDepthH = inDepthTex->vtable->getHeight_50(inDepthTex, 0);
    auto* inDepthGnm  = *reinterpret_cast<uint8_t**>(inDepthWrap + 0x28) + 0x10;
    uint32_t inDepthFmt = sub_1089960(inDepthGnm);

    void* ppfxCtx = *reinterpret_cast<void**>(director + 0x2670);
    bool didYebisRender = false;

    if (ppfxCtx != nullptr && *reinterpret_cast<int32_t*>(param + 0x000) != 0 && director[0x20] == 0) {
        didYebisRender = true;
        director[0x20] = 1;

        sub_bb4d70(ppfxCtx, 0);
        sub_bb4de0(ppfxCtx, g_YebisBufferFlipIndex_56a6d20);

        auto* sceneView = *reinterpret_cast<uint8_t**>(cmdCtx + 0x210);
        uint32_t viewMode = *reinterpret_cast<uint32_t*>(sceneView + 0x70);
        if (viewMode >= 2u) {
            sub_bb26e0(ppfxCtx, 0);
            sceneView = *reinterpret_cast<uint8_t**>(cmdCtx + 0x210);
        }

        uint32_t resetHistoryFlags = *reinterpret_cast<uint32_t*>(director + 0x271c);
        *reinterpret_cast<uint32_t*>(director + 0x271c) = 0;

        alignas(16) float prevViewMtx[16];
        if ((resetHistoryFlags & 1u) != 0) {
            std::memcpy(prevViewMtx, sceneView, 64);
        } else {
            std::memcpy(prevViewMtx, param + 0x440, 64);
        }

        if (g_DbgCameraBlurFlag_56c3d40) {
            float s = sub_2bbe518(0, 0, fPiDiv6);
            float c = sub_2bbe518(1, 0, fPiDiv6);
            __m128 rotRow0 = _mm_setr_ps(c, 0.0f, -s, 0.0f);
            __m128 rotRow1 = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
            __m128 rotRow2 = _mm_setr_ps(s, 0.0f, c, 0.0f);
            __m128 rotRow3 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
            for (int r = 0; r < 3; ++r) {
                __m128 v = _mm_loadu_ps( reinterpret_cast<const float*>(sceneView) + r * 4 );
                __m128 x = _mm_shuffle_ps(v, v, 0x00);
                __m128 y = _mm_shuffle_ps(v, v, 0x55);
                __m128 z = _mm_shuffle_ps(v, v, 0xAA);
                __m128 w = _mm_shuffle_ps(v, v, 0xFF);
                __m128 res = _mm_add_ps(
                    _mm_add_ps(_mm_mul_ps(x, rotRow0), _mm_mul_ps(z, rotRow2)),
                    _mm_add_ps(_mm_mul_ps(y, rotRow1), _mm_mul_ps(w, rotRow3)));
                _mm_storeu_ps(prevViewMtx + r * 4, res);
            }
            std::memcpy(prevViewMtx + 12, sceneView + 0x30, 16);
        }

        sub_bb2710(ppfxCtx,
                   *reinterpret_cast<float*>(sceneView + 0x54),
                   *reinterpret_cast<float*>(sceneView + 0x58),
                   *reinterpret_cast<float*>(sceneView + 0x50),
                   f1Div60,
                   *reinterpret_cast<float*>(param + 0x094));

        sub_bb2720(ppfxCtx,
                   *reinterpret_cast<float*>(sceneView + 0x54),
                   *reinterpret_cast<float*>(sceneView + 0x58),
                   *reinterpret_cast<float*>(sceneView + 0x50));

        sub_bb2740(ppfxCtx,
                   *reinterpret_cast<float*>(param + 0x494),
                   *reinterpret_cast<float*>(param + 0x498),
                   *reinterpret_cast<float*>(param + 0x490));

        alignas(16) float invProjMtx[16];
        InvertMatrix4x4Simd(invProjMtx, reinterpret_cast<const float*>(sceneView));
        sub_bb27d0(ppfxCtx, invProjMtx, 1);

        alignas(16) float invPrevProjMtx[16];
        InvertMatrix4x4Simd(invPrevProjMtx, reinterpret_cast<const float*>(param + 0x440));
        sub_bb27f0(ppfxCtx, invPrevProjMtx, 1);

        sub_bb3fb0(ppfxCtx, director[0x2668]);

        if (*reinterpret_cast<int32_t*>(param + 0x008) != 0) {
            sub_bb5080(ppfxCtx, 0, inDepthGnm, inDepthW, inDepthH, inDepthFmt, f1, 0.0f, 1);
        } else {
            sub_bb5080(ppfxCtx, 0, nullptr, 0, 0, 0, f1, 0.0f, 1);
        }

        sub_bb5080(ppfxCtx, 1, inColorGnm, inColorW, inColorH, inColorFmt, f1, 0.0f, -1);

        auto* outSurfTex = *reinterpret_cast<DLRefCountedObj**>(outRtWrap + 0x10);
        uint32_t outW = outSurfTex->vtable->getWidth_48(outSurfTex, 0);
        uint32_t outH = outSurfTex->vtable->getHeight_50(outSurfTex, 0);
        auto* outSurfGnm = *reinterpret_cast<uint8_t**>(outRtWrap + 0x20) + 0x10;
        uint32_t outFmt = sub_1089960(outSurfGnm);
        sub_bb5080(ppfxCtx, 2, outSurfGnm, outW, outH, outFmt, f1, 0.0f, -1);

        // Auto-exposure & ToneMap
        uint8_t enableAutoExp = (*reinterpret_cast<int32_t*>(param + 0x050) != 0) ? 1u : 0u;
        sub_bb31c0(ppfxCtx, 0, enableAutoExp);

        float exposureVal = *reinterpret_cast<float*>(param + 0x098);
        if (*reinterpret_cast<int32_t*>(param + 0x050) != 0) {
            float adapted = sub_bb3250(ppfxCtx, 0);
            exposureVal *= adapted;
        }
        if ((resetHistoryFlags & 2u) != 0) {
            exposureVal *= *reinterpret_cast<float*>(director + 0x2720);
        }

        sub_bb32f0(ppfxCtx, 0,
                   *reinterpret_cast<float*>(param + 0x054),
                   *reinterpret_cast<float*>(param + 0x058));
        sub_bb32e0(ppfxCtx, 0,
                   *reinterpret_cast<float*>(param + 0x05c),
                   *reinterpret_cast<float*>(param + 0x060));
        sub_bb3300(ppfxCtx, 0,
                   *reinterpret_cast<float*>(param + 0x064),
                   *reinterpret_cast<float*>(param + 0x068),
                   *reinterpret_cast<float*>(param + 0x06c),
                   *reinterpret_cast<float*>(param + 0x070));
        sub_bb3310(ppfxCtx, 0, 1, *reinterpret_cast<float*>(param + 0x074));
        sub_bb3320(ppfxCtx, 0,
                   *reinterpret_cast<float*>(param + 0x078),
                   *reinterpret_cast<float*>(param + 0x07c),
                   *reinterpret_cast<float*>(param + 0x080),
                   *reinterpret_cast<float*>(param + 0x084),
                   *reinterpret_cast<float*>(param + 0x088),
                   *reinterpret_cast<float*>(param + 0x08c),
                   *reinterpret_cast<float*>(param + 0x090));

        sub_bb3550(ppfxCtx,
                   *reinterpret_cast<int32_t*>(param + 0x0a0),
                   exposureVal,
                   *reinterpret_cast<float*>(param + 0x09c),
                   *reinterpret_cast<float*>(param + 0x0a4),
                   fNegMax);
        sub_bb3560(ppfxCtx, *reinterpret_cast<float*>(param + 0x0a8));

        // Chromatic Aberration
        bool enableCA = (*reinterpret_cast<int32_t*>(param + 0x038) != 0) &&
                        (*reinterpret_cast<int32_t*>(param + 0x0ac) != 0);
        sub_bb37a0(ppfxCtx, enableCA ? 1u : 0u);
        sub_bb37d0(ppfxCtx,
                   *reinterpret_cast<int32_t*>(param + 0x0b0),
                   reinterpret_cast<const float*>(param + 0x0b4),
                   reinterpret_cast<const float*>(param + 0x0bc));

        // Color Correction (H / S / V / Contrast)
        sub_bb4300(ppfxCtx, 0);
        float hueRad = (*reinterpret_cast<float*>(param + 0x0c4) * fPi) / f180;
        sub_bb4360(ppfxCtx, 0, 0, hueRad);
        sub_bb4390(ppfxCtx, 0, *reinterpret_cast<float*>(param + 0x0c8));
        sub_bb43b0(ppfxCtx, 0, *reinterpret_cast<float*>(param + 0x0cc));
        sub_bb43e0(ppfxCtx, 0, *reinterpret_cast<float*>(param + 0x248));

        // Vignetting
        sub_bb3670(ppfxCtx,
                   *reinterpret_cast<float*>(param + 0x0d0),
                   *reinterpret_cast<float*>(param + 0x0d4));

        // Glare / Bloom
        sub_bb2840(ppfxCtx, 1,
                   *reinterpret_cast<float*>(param + 0x0dc),
                   *reinterpret_cast<float*>(param + 0x0e0),
                   *reinterpret_cast<float*>(param + 0x244));

        int32_t glareShape = *reinterpret_cast<int32_t*>(param + 0x0d8);
        if (glareShape == 14) {
            struct CustomGlareDesc {
                float glareLum;           // +0x00 (0x308)
                float bloomLum;           // +0x04 (0x30c)
                float bloomDisp;          // +0x08 (0x310)
                uint32_t bloomBaseLevel;  // +0x0c (0x314)
                float ghostLum;           // +0x10 (0x318)
                float ghostHaloLum;       // +0x14 (0x31c)
                float ghostDist;          // +0x18 (0x320)
                uint8_t ghostSharpness;   // +0x1c (0x324 != 0)
                uint8_t pad1D[3];
                float starLum;            // +0x20 (0x328)
                uint32_t starStreaks;     // +0x24 (0x32c)
                float starLen;            // +0x28 (0x330)
                float starSecLen;         // +0x2c (0x334)
                uint8_t starRot;          // +0x30 (0x338 != 0)
                uint8_t pad31[3];
                float starIncAngleRad;    // +0x34 (0x33c * pi/180)
                float starDisp;           // +0x38 (0x340)
                uint8_t starForceDisp;    // +0x3c (0x344 != 0)
                uint8_t pad3D[3];
                float afterimageLum;      // +0x40 (0x348)
                float afterimageLen;      // +0x44 (0x34c)
                uint64_t reserved48;      // +0x48 (0)
            } customGlare{};

            customGlare.glareLum        = *reinterpret_cast<float*>(param + 0x308);
            customGlare.bloomLum        = *reinterpret_cast<float*>(param + 0x30c);
            customGlare.bloomDisp       = *reinterpret_cast<float*>(param + 0x310);
            customGlare.bloomBaseLevel  = *reinterpret_cast<uint32_t*>(param + 0x314);
            customGlare.ghostLum        = *reinterpret_cast<float*>(param + 0x318);
            customGlare.ghostHaloLum    = *reinterpret_cast<float*>(param + 0x31c);
            customGlare.ghostDist       = *reinterpret_cast<float*>(param + 0x320);
            customGlare.ghostSharpness  = (*reinterpret_cast<int32_t*>(param + 0x324) != 0) ? 1u : 0u;
            customGlare.starLum         = *reinterpret_cast<float*>(param + 0x328);
            customGlare.starStreaks     = *reinterpret_cast<uint32_t*>(param + 0x32c);
            customGlare.starLen         = *reinterpret_cast<float*>(param + 0x330);
            customGlare.starSecLen      = *reinterpret_cast<float*>(param + 0x334);
            customGlare.starRot         = (*reinterpret_cast<int32_t*>(param + 0x338) != 0) ? 1u : 0u;
            customGlare.starIncAngleRad = *reinterpret_cast<float*>(param + 0x33c) * fDeg2Rad;
            customGlare.starDisp        = *reinterpret_cast<float*>(param + 0x340);
            customGlare.starForceDisp   = (*reinterpret_cast<int32_t*>(param + 0x344) != 0) ? 1u : 0u;
            customGlare.afterimageLum   = *reinterpret_cast<float*>(param + 0x348);
            customGlare.afterimageLen   = *reinterpret_cast<float*>(param + 0x34c);
            customGlare.reserved48      = 0;

            sub_bb2860(ppfxCtx, &customGlare);
        } else {
            sub_bb2850(ppfxCtx, glareShape);
        }

        sub_bb40e0(ppfxCtx, *reinterpret_cast<int32_t*>(param + 0x0e4));
        sub_bb3c10(ppfxCtx, *reinterpret_cast<float*>(param + 0x0e8));
        sub_bb4130(ppfxCtx, *reinterpret_cast<float*>(param + 0x0ec));
        sub_bb4100(ppfxCtx, *reinterpret_cast<float*>(param + 0x0f0));
        sub_bb40f0(ppfxCtx, *reinterpret_cast<float*>(param + 0x0f4));
        sub_bb4120(ppfxCtx, *reinterpret_cast<float*>(param + 0x304));
        sub_bb4110(ppfxCtx, *reinterpret_cast<float*>(param + 0x0f8));
        sub_bb3c30(ppfxCtx, *reinterpret_cast<float*>(param + 0x0fc));
        sub_bb41a0(ppfxCtx, *reinterpret_cast<float*>(param + 0x100));

        // Anamorphic Lens Flare
        sub_bb4160(ppfxCtx, (*reinterpret_cast<int32_t*>(param + 0x104) != 0) ? 1u : 0u);
        sub_bb4180(ppfxCtx,
                   *reinterpret_cast<float*>(param + 0x108),
                   *reinterpret_cast<float*>(param + 0x10c));

        auto* globalParam = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
        if (director[0x2710] != 0) {
            std::memcpy(globalParam + 0x110, director + 0x26c0, 16);
            std::memcpy(globalParam + 0x120, director + 0x26e0, 16);
        } else {
            std::memcpy(director + 0x26c0, globalParam + 0x110, 16);
            std::memcpy(director + 0x26e0, globalParam + 0x120, 16);
        }
        sub_bb4190(ppfxCtx,
                   reinterpret_cast<const float*>(param + 0x110),
                   reinterpret_cast<const float*>(param + 0x120));

        // LightShaft
        if (*reinterpret_cast<int32_t*>(param + 0x034) != 0) {
            sub_bb28a0(ppfxCtx, 0);
            bool lsOpen = false;
            auto* lsWeak = *reinterpret_cast<uint8_t**>(director + 0x2728);
            if (lsWeak != nullptr) {
                uint32_t f = *reinterpret_cast<uint32_t*>(lsWeak + 0x08);
                if ((f & 0x7fffu) != 0 && (f & 0x40000000u) != 0) {
                    lsOpen = (lsWeak[0xc0] != 0);
                }
            }
            bool enableLs = lsOpen || (*reinterpret_cast<int32_t*>(param + 0x130) != 0);
            sub_bb28d0(ppfxCtx, 0, enableLs ? 1u : 0u);

            float radX = (*reinterpret_cast<float*>(param + 0x160) * fPi) / f180;
            float radY = (*reinterpret_cast<float*>(param + 0x164) * fPi) / f180;
            float sinX = sub_2bbe518(0, 0, radX);
            float cosX = sub_2bbe518(1, 0, radX);
            float sinY = sub_2bbe518(0, 0, radY);
            float cosY = sub_2bbe518(1, 0, radY);

            // Transform light shaft direction into clip/screen space
            float dirX = -sinY * cosX;
            float dirY = sinX;
            float dirZ = cosY * cosX;

            alignas(16) float viewProj[16];
            sub_21aaa90(viewProj, sceneView);

            const float* camPos = reinterpret_cast<const float*>(sceneView + 0x30);
            float worldX = camPos[0] - dirX;
            float worldY = camPos[1] - dirY;
            float worldZ = camPos[2] - dirZ;

            float clipX = worldX * viewProj[0] + worldY * viewProj[4] + worldZ * viewProj[8]  + viewProj[12];
            float clipY = worldX * viewProj[1] + worldY * viewProj[5] + worldZ * viewProj[9]  + viewProj[13];
            float clipW = worldX * viewProj[3] + worldY * viewProj[7] + worldZ * viewProj[11] + viewProj[15];

            float screenX = (clipX / clipW) * 0.5f + 0.5f;
            float screenY = (clipY / clipW) * -0.5f + 0.5f;
            float attScale = (clipW >= 0.0f) ? f1 : 0.0f;

            sub_bb2940(ppfxCtx, 0, screenX, screenY);

            globalParam = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
            if (director[0x2710] != 0) {
                std::memcpy(globalParam + 0x134, director + 0x2680, 16);
                std::memcpy(globalParam + 0x178, director + 0x26a0, 16);
            } else {
                std::memcpy(director + 0x2680, globalParam + 0x134, 16);
                std::memcpy(director + 0x26a0, globalParam + 0x178, 16);
            }

            float lsColor[4];
            lsColor[0] = *reinterpret_cast<float*>(param + 0x134) * attScale;
            lsColor[1] = *reinterpret_cast<float*>(param + 0x138) * attScale;
            lsColor[2] = *reinterpret_cast<float*>(param + 0x13c) * attScale;
            lsColor[3] = *reinterpret_cast<float*>(param + 0x140);

            alignas(16) float projParams[16];
            sub_21aa290(projParams, sceneView);
            float zNear = projParams[1];
            float zFar  = projParams[2];
            float maskDepth = *reinterpret_cast<float*>(param + 0x24c);
            float normDepth = f0_9995;
            if (maskDepth <= zFar) {
                normDepth = ((maskDepth - zNear) * zFar) / ((zFar - zNear) * maskDepth);
            }

            sub_bb2900(ppfxCtx, 0, lsColor, 1,
                       *reinterpret_cast<float*>(param + 0x144),
                       *reinterpret_cast<float*>(param + 0x148),
                       normDepth,
                       *reinterpret_cast<float*>(param + 0x154) * f0_3,
                       *reinterpret_cast<float*>(param + 0x158),
                       *reinterpret_cast<float*>(param + 0x15c));

            float ringColor[4];
            ringColor[0] = *reinterpret_cast<float*>(param + 0x178) * attScale;
            ringColor[1] = *reinterpret_cast<float*>(param + 0x17c) * attScale;
            ringColor[2] = *reinterpret_cast<float*>(param + 0x180) * attScale;
            ringColor[3] = *reinterpret_cast<float*>(param + 0x184);

            sub_bb2920(ppfxCtx, 0, ringColor,
                       *reinterpret_cast<float*>(param + 0x168),
                       *reinterpret_cast<float*>(param + 0x16c),
                       *reinterpret_cast<float*>(param + 0x170),
                       *reinterpret_cast<float*>(param + 0x174));
        }

        // Depth of Field & AutoFocus
        if (*reinterpret_cast<int32_t*>(param + 0x04c) > 0) {
            sub_bb2960(ppfxCtx, (*reinterpret_cast<int32_t*>(param + 0x1ac) != 0) ? 1u : 0u);
            sub_bb3350(ppfxCtx, 0, *reinterpret_cast<int32_t*>(param + 0x188));
            sub_bb34b0(ppfxCtx, 0,
                       (*reinterpret_cast<int32_t*>(param + 0x18c) != 0) ? 1u : 0u,
                       *reinterpret_cast<float*>(param + 0x190),
                       *reinterpret_cast<float*>(param + 0x194));
            sub_bb3490(ppfxCtx, 0, 1, *reinterpret_cast<float*>(param + 0x198));
            sub_bb3470(ppfxCtx, 0,
                       *reinterpret_cast<float*>(param + 0x19c),
                       *reinterpret_cast<float*>(param + 0x1a0));
            sub_bb3480(ppfxCtx, 0,
                       *reinterpret_cast<float*>(param + 0x1a4),
                       *reinterpret_cast<float*>(param + 0x1a8));

            if (*reinterpret_cast<int32_t*>(param + 0x188) != 0) {
                float afDist = sub_bb33e0(ppfxCtx, 0);
                *reinterpret_cast<float*>(param + 0x1b0) = afDist;
            } else {
                sub_bb3340(ppfxCtx, 0, *reinterpret_cast<float*>(param + 0x1b0));
            }

            sub_bb3f80(ppfxCtx, *reinterpret_cast<float*>(param + 0x1d8));
            float rotOffsetRad = (*reinterpret_cast<float*>(param + 0x1c8) * fPi) / f180;
            sub_bb34a0(ppfxCtx,
                       *reinterpret_cast<int32_t*>(param + 0x1cc),
                       *reinterpret_cast<float*>(param + 0x1c4),
                       rotOffsetRad);
            sub_bb3fd0(ppfxCtx,
                       *reinterpret_cast<int32_t*>(param + 0x1d0),
                       *reinterpret_cast<int32_t*>(param + 0x1d4));

            float adaptiveFactor = (param[0x1b8] != 0) ? f1 : 0.0f;
            float baseFovRad     = (*reinterpret_cast<float*>(param + 0x1bc) * fPi) / f180;
            float ccdDiagMm      = *reinterpret_cast<float*>(param + 0x1c0);

            auto* vpMgr = static_cast<uint8_t*>(g_ViewportManagerSingleton_5540dc0);
            int32_t vpW = vpMgr ? *reinterpret_cast<int32_t*>(vpMgr + 0x38) : 1920;
            int32_t vpH = vpMgr ? *reinterpret_cast<int32_t*>(vpMgr + 0x3c) : 1080;
            float aspect = static_cast<float>(vpW) / static_cast<float>(vpH);
            float diagSqMm = ccdDiagMm * ccdDiagMm;
            float sensorHeightMm = std::sqrt(diagSqMm / (aspect * aspect + f1));
            float sensorHeightM  = sensorHeightMm * f0_001;

            sub_bb2970(ppfxCtx, 0,
                       *reinterpret_cast<float*>(param + 0x1b0),
                       *reinterpret_cast<float*>(param + 0x1b4),
                       adaptiveFactor,
                       baseFovRad,
                       sensorHeightM);
            sub_bb45c0(ppfxCtx, *reinterpret_cast<int32_t*>(param + 0x240));
        }

        // Color Grading 3D LUT binding
        if (*reinterpret_cast<int32_t*>(param + 0x2b4) != 0) {
            bool cgValid = (*reinterpret_cast<int32_t*>(param + 0x2b8) != 0) && (param[0x2cc] != 0);
            sub_bb3600(ppfxCtx, cgValid ? 1u : 0u);
            sub_bb3660(ppfxCtx, 0, 2, 0, f1);

            auto* lut3dObj = *reinterpret_cast<uint8_t**>(director + 0x2180);
            auto* lut3dTex = *reinterpret_cast<DLRefCountedObj**>(lut3dObj + 0x10);
            uint32_t lutW  = lut3dTex->vtable->getWidth_48(lut3dTex, 0);
            uint32_t lutH  = lut3dTex->vtable->getHeight_50(lut3dTex, 0);
            auto* lutGnm   = *reinterpret_cast<uint8_t**>(lut3dObj + 0x30) + 0x10;
            uint32_t lutFmt = sub_1089960(lutGnm);
            sub_bb5790(ppfxCtx, lutGnm, lutW, lutH, lutFmt);
        }

        // Motion Blur
        if (*reinterpret_cast<int32_t*>(param + 0x048) > 0) {
            sub_bb3870(ppfxCtx, (*reinterpret_cast<int32_t*>(param + 0x1dc) != 0) ? 1u : 0u);
            sub_bb38b0(ppfxCtx, param[0x204]);
            sub_bb3890(ppfxCtx, *reinterpret_cast<int32_t*>(param + 0x1e0));
            sub_bb38e0(ppfxCtx,
                       *reinterpret_cast<float*>(param + 0x2ac),
                       *reinterpret_cast<float*>(param + 0x2b0));
            sub_bb27c0(ppfxCtx, sceneView, 1);
            sub_bb27e0(ppfxCtx, prevViewMtx, 1);

            float mbDeltaTime = *reinterpret_cast<float*>(param + 0x004);
            if ((resetHistoryFlags & 1u) != 0) {
                mbDeltaTime = f0_0001;
            }
            sub_bb2800(ppfxCtx, mbDeltaTime);

            auto* velWrap = *reinterpret_cast<uint8_t**>(param + 0x380);
            if (velWrap != nullptr) {
                auto* velTex = *reinterpret_cast<DLRefCountedObj**>(velWrap + 0x10);
                uint32_t velW = velTex->vtable->getWidth_48(velTex, 0);
                uint32_t velH = velTex->vtable->getHeight_50(velTex, 0);
                auto* velGnm  = *reinterpret_cast<uint8_t**>(velWrap + 0x30) + 0x10;
                uint32_t velFmt = sub_1089960(velGnm);
                float scalePair[2] = {f1, fNeg1};
                sub_bb5430(ppfxCtx, 1, velGnm, velW, velH, velFmt, 1, scalePair, 0);
            } else {
                float scalePair[2] = {f1, f1};
                sub_bb5430(ppfxCtx, 0, nullptr, 0, 0, 0, 0, scalePair, 0);
            }

            float maxBlurLen = *reinterpret_cast<float*>(param + 0x1e8) *
                               (*reinterpret_cast<float*>(param + 0x350));
            sub_bb38f0(ppfxCtx,
                       *reinterpret_cast<int32_t*>(param + 0x1ec),
                       *reinterpret_cast<int32_t*>(param + 0x1f0),
                       *reinterpret_cast<float*>(param + 0x1e4),
                       maxBlurLen,
                       *reinterpret_cast<float*>(param + 0x1f4));
            sub_bb3900(ppfxCtx, 1u,
                       *reinterpret_cast<float*>(param + 0x1f8),
                       *reinterpret_cast<float*>(param + 0x1fc));
        }

        // Feedback Blur
        if (*reinterpret_cast<int32_t*>(param + 0x028) != 0) {
            bool enableFb = (*reinterpret_cast<int32_t*>(param + 0x208) != 0) &&
                            (director[0x2719] == 0);
            sub_bb34c0(ppfxCtx, enableFb ? 1u : 0u);
            director[0x2719] = 0;

            float fbWeight = *reinterpret_cast<float*>(param + 0x20c);
            sub_bb34d0(ppfxCtx, fbWeight, fbWeight, *reinterpret_cast<float*>(param + 0x214));
            sub_bb34e0(ppfxCtx, *reinterpret_cast<float*>(param + 0x218));
        }

        // Anti-Aliasing
        if (*reinterpret_cast<int32_t*>(param + 0x030) != 0) {
            sub_bb37f0(ppfxCtx, (*reinterpret_cast<int32_t*>(param + 0x220) != 0) ? 1u : 0u);
        }

        // --------------------------------------------------------------------
        // Part C: Yebis2 Fullscreen Draw & Double-Buffered EOP Fence Sync
        // --------------------------------------------------------------------
        if (g_YebisPostProcessEnabled_56c3ce0) {
            uint32_t drawFlags = 0x20003u;
            if (*reinterpret_cast<int32_t*>(param + 0x008) == 0) {
                drawFlags |= 0x4u;
            }
            if (director[0x271a] != 0) {
                drawFlags |= 0x400u;
                director[0x271a] = 0;
            }
            float drawCol[4] = {f1, f1, f1, f1};
            sub_bb3c50(ppfxCtx, drawFlags, drawCol, 0, -1, fNegMax);
        } else {
            // Manual fullscreen pass via Yebis GPU context
            void* yGpu = g_ActiveYebisGpuCtx_56c3cb8;
            uint32_t dsCtrl = 0;
            sub_10850d0(&dsCtrl);
            sub_1085130(&dsCtrl, 0);
            sub_1076b30(yGpu, dsCtrl);

            uint32_t primSetup = 0;
            sub_1084f40(&primSetup);
            sub_1084f50(&primSetup, 0);
            sub_1084f60(&primSetup, 0);
            sub_1084f70(&primSetup, 0, 0);
            sub_1076e00(yGpu, primSetup);

            sub_1072fd0(yGpu, 0x0fu);
            sub_1076030(yGpu, 0);

            uint32_t blendCtrl = 0;
            sub_1085380(&blendCtrl);
            sub_1085390(&blendCtrl, 0, 0x0f);
            sub_10853f0(&blendCtrl, 0);
            sub_10733a0(yGpu, blendCtrl);

            sub_1096f60(yGpu, 0, 0, outW, outH, 0.0f, f1);
            sub_1077150(yGpu, 0, 0);

            uint32_t vtxFmt0 = 0;
            sub_26b7310(&vtxFmt0, 0x25);
            uint8_t gnmBufs[64] = {};
            uint64_t fIdx = g_YebisBufferFlipIndex_56a6d20;
            void* vbAddr0 = *reinterpret_cast<void**>(g_YebisDoubleBufArea_56a6d20 + fIdx * 8 + 0x1cfc8);
            sub_1070650(gnmBufs + 0, vbAddr0, vtxFmt0, 12u, 4u);

            uint32_t vtxFmt1 = 0;
            sub_26b7310(&vtxFmt1, 0x1e);
            void* vbAddr1 = *reinterpret_cast<void**>(g_YebisDoubleBufArea_56a6d20 + fIdx * 8 + 0x1cfd8);
            sub_1070650(gnmBufs + 16, vbAddr1, vtxFmt1, 8u, 4u);

            uint8_t* yShdrCtx = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x35d0;
            sub_108f330(yShdrCtx, 0, 0, 2, gnmBufs);

            uint64_t shdrIdx = g_YebisBufferFlipIndex_56a6d20 * 16u;
            auto* vsObj = *reinterpret_cast<uint8_t**>(g_YebisDoubleBufArea_56a6d20 + shdrIdx + 0x1cfc0);
            void* fetchMem = *reinterpret_cast<void**>(g_YebisDoubleBufArea_56a6d20 + shdrIdx + 0x1cfd0);
            sub_1091bc0(fetchMem, vsObj + 0x40, 0);

            vsObj = *reinterpret_cast<uint8_t**>(g_YebisDoubleBufArea_56a6d20 + shdrIdx + 0x1cfc0);
            sub_108fbb0(yShdrCtx, vsObj, *reinterpret_cast<uint32_t*>(vsObj + 0x28), fetchMem, yShdrCtx + 0xb0d8);

            auto* psObj = *reinterpret_cast<uint8_t**>(g_YebisDoubleBufArea_56a6d20 + shdrIdx + 0x1cfc8);
            sub_108fdd0(yShdrCtx, psObj, yShdrCtx + 0xb148);

            __m128i texDesc0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(inColorGnm));
            __m128i texDesc1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(inColorGnm + 16));
            alignas(16) __m128i localTexDesc[2] = {texDesc0, texDesc1};
            sub_108d3f0(yShdrCtx, 1, 0, localTexDesc);

            uint8_t samplerDesc[16] = {};
            sub_1086dd0(samplerDesc);
            sub_1086e10(samplerDesc, 2);
            sub_108dfe0(yShdrCtx, 1, 0, samplerDesc);

            void* ibAddr = *reinterpret_cast<void**>(
                g_YebisDoubleBufArea_56a6d20 + g_YebisBufferFlipIndex_56a6d20 * 8u + 0x1cfe8);
            if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
                sub_108ca70(yShdrCtx, g_ActiveYebisGpuCtx_56c3cb8,
                            static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x35c0);
                gx_direct_vk::EmitDrawIndexAddr(g_ActiveYebisGpuCtx_56c3cb8,
                                                reinterpret_cast<uint64_t>(ibAddr), 6u, 4u, 0u);
            } else {
                sub_1073fe0(g_ActiveYebisGpuCtx_56c3cb8, 4);
                sub_1075270(g_ActiveYebisGpuCtx_56c3cb8, 0);
                sub_108ca70(yShdrCtx, g_ActiveYebisGpuCtx_56c3cb8,
                            static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x35c0);
                sub_1074360(g_ActiveYebisGpuCtx_56c3cb8, 6u, ibAddr);
            }
        }

        uint8_t* yGpu = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
        sub_108ca80(yGpu + 0x35d0, yGpu, yGpu + 0x35c0);

        uint64_t nextFenceVal = g_YebisFenceCounter_56c3ce8 + 1u;
        g_YebisFenceCounter_56c3ce8 = nextFenceVal;
        uint8_t* fenceBase = g_YebisFenceGpuAddrBase_56c3cf8;
        uint64_t curFlip   = g_YebisBufferFlipIndex_56a6d20;
        sub_10778f0(g_ActiveYebisGfxc0_56c3cc0, 4, 0,
                    fenceBase + curFlip * 8u, 2, nextFenceVal, 0, 0);

        uint8_t nextFlip = g_YebisBufferFlipIndex_56a6d20 ^ 1u;
        g_YebisBufferFlipIndex_56a6d20 = nextFlip;
        uint8_t* waitAddr = g_YebisFenceGpuAddrBase_56c3cf8 + static_cast<uint64_t>(nextFlip) * 8u;
        uint32_t expectedLow = *reinterpret_cast<uint32_t*>(waitAddr);
        sub_1077b30(g_ActiveYebisGfxc0_56c3cc0, waitAddr, 0xffffffffu, 3, expectedLow);

        uint32_t s0 = 0, sz0 = 0, s1 = 0, sz1 = 0;
        sub_1089a10(1, &s0, &sz0, &s1, &sz1);

        director[0x20] = 0;
        if (director[0x271b] != 0) {
            director[0x271b] = 0;
            sub_21d9e50(director,
                        *reinterpret_cast<void**>(param + 0x368),
                        *reinterpret_cast<void**>(param + 0x378));
        }
        ppfxCtx = *reinterpret_cast<void**>(director + 0x2670);
    }

    if (auto* auxDepthWrap = *reinterpret_cast<uint8_t**>(param + 0x388)) {
        sub_bb5f10(ppfxCtx, *reinterpret_cast<uint8_t**>(auxDepthWrap + 0x28) + 0x10);
    }
    if (auto* auxVelWrap = *reinterpret_cast<uint8_t**>(param + 0x380)) {
        sub_bb5f10(ppfxCtx, *reinterpret_cast<uint8_t**>(auxVelWrap + 0x48) + 0x10);
    }

    auto* gnmAfterYebis = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
    *reinterpret_cast<uint32_t*>((gnmAfterYebis + 0xb730) + 0x79c) |= 0xfffu;
    *reinterpret_cast<uint32_t*>((gnmAfterYebis + 0xb518) + 0x0c)  |= 0xfffu;
    *reinterpret_cast<uint32_t*>((gnmAfterYebis + 0xb528) + 0x2c)  |= 0xfffu;
    *reinterpret_cast<uint32_t*>((gnmAfterYebis + 0xb558) + 0xf8)  |= 0xfffu;
    cmdCtx[0x208] = 1;

    if (!didYebisRender) {
        auto* copyGnmCtx = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
        sub_2168660(copyGnmCtx,
                    copyGnmCtx + 0xb730,
                    *reinterpret_cast<void**>(param + 0x370),
                    *reinterpret_cast<void**>(param + 0x368));
    }

    // ------------------------------------------------------------------------
    // Part D: Debug 3D-to-2D LUT Compute Pass ("LutCopy3DTo2DCs.cpo") & Quad Draw
    // ------------------------------------------------------------------------
    if (director[0x21a8] != 0) {
        if (*reinterpret_cast<int32_t*>(param + 0x2b8) != 0 &&
            *reinterpret_cast<int32_t*>(param + 0x2b4) != 0 &&
            param[0x2cc] != 0) {

            auto* gnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
            auto* dbg2dLutObj = *reinterpret_cast<uint8_t**>(director + 0x21a0);
            void* dbgUav = dbg2dLutObj ? *reinterpret_cast<void**>(dbg2dLutObj + 0x40) : nullptr;
            sub_2168a20(gnm, gnm + 0xb730, dbgUav, 0, 0, nullptr, 0, 0);

            auto* lut3dObj = *reinterpret_cast<uint8_t**>(director + 0x2180);
            void* lut3dSrv = lut3dObj ? *reinterpret_cast<void**>(lut3dObj + 0x30) : nullptr;
            auto* sb = gnm + 0xb730;
            int64_t baseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b38)) * 0x5c8;
            *reinterpret_cast<void**>(sb + baseOff + 0x3660) = lut3dSrv;
            *reinterpret_cast<int32_t*>(sb + baseOff + 0x3668) = -1;
            *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3b20) |= 1u;
            *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x20u;

            void* shdrCont = *reinterpret_cast<void**>(director + 0x2198);
            auto* csEntry = static_cast<uint8_t*>(
                sub_2274e20(shdrCont, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b54ULL))));
            void* csProg  = csEntry ? *reinterpret_cast<void**>(csEntry + 0x48) : nullptr;
            sb = gnm + 0xb730;
            if (*reinterpret_cast<void**>(sb + 0x3578) != csProg) {
                *reinterpret_cast<void**>(sb + 0x3578) = csProg;
                *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x2000u;
            }

            auto* dispHelper = gnm + 0x50;
            if (dispHelper[0x50] == 0) {
                sub_26af700(gnm + 0x10);
                dispHelper[0x50] = 1;
            }
            sub_26cbe50(dispHelper, gnm, gnm + 0xb730);
            if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
                gx_direct_vk::EmitDispatchDirect(gnm + 0x10, 1u, 1u, 16u);
            } else {
                sub_1074c50(gnm + 0x10, 1u, 1u, 16u);
            }
            sub_26ccf20(dispHelper, gnm, gnm + 0xb730);

            sub_2168a20(gnm, gnm + 0xb730, nullptr, 0, 0, nullptr, 0, 0);

            sb = gnm + 0xb730;
            baseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b38)) * 0x5c8;
            *reinterpret_cast<void**>(sb + baseOff + 0x3660) = nullptr;
            *reinterpret_cast<int32_t*>(sb + baseOff + 0x3668) = -1;
            *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3b20) |= 1u;
            *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x20u;

            // Draw debug 2D LUT quad via director + 0x2658 shader
            auto* quadShdr = *reinterpret_cast<uint8_t**>(director + 0x2658);
            if (quadShdr != nullptr) {
                void* vsProg = *reinterpret_cast<void**>(quadShdr + 0x28);
                if (*reinterpret_cast<void**>(cmdCtx + 0xf8) != vsProg) {
                    *reinterpret_cast<void**>(cmdCtx + 0xf8) = vsProg;
                    cmdCtx[0x20c] = 1;
                }
                void* psProg = *reinterpret_cast<void**>(quadShdr + 0x30);
                if (*reinterpret_cast<void**>(cmdCtx + 0x100) != psProg) {
                    *reinterpret_cast<void**>(cmdCtx + 0x100) = psProg;
                    cmdCtx[0x20c] = 1;
                }

                uint8_t* quadTexObj = (director[0x21a9] == 0)
                    ? *reinterpret_cast<uint8_t**>(director + 0x21a0)
                    : *reinterpret_cast<uint8_t**>(director + 0x2188);
                void* quadSrv = quadTexObj ? *reinterpret_cast<void**>(quadTexObj + 0x30) : nullptr;
                gnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
                sb  = gnm + 0xb730;
                int64_t psBaseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b30)) * 0x5c8;
                *reinterpret_cast<void**>(sb + psBaseOff + 0x11c0) = quadSrv;
                *reinterpret_cast<int32_t*>(sb + psBaseOff + 0x11c8) = -1;
                *reinterpret_cast<uint32_t*>(sb + psBaseOff + 0x1680) |= 1u;
                *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 2u;

                // Allocate 4-vertex quad in dynamic vertex stream (director + 0x21b0)
                auto* vtxStream = director + 0x21b0;
                auto* dev = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
                uint32_t curFrameIdx = *reinterpret_cast<uint32_t*>(dev + 0x35c);
                void* cpuVerts = nullptr;

                if (*reinterpret_cast<uint32_t*>(vtxStream + 0x2ac) != 2u ||
                    *reinterpret_cast<int32_t*>(vtxStream + 0x290) < 0) {
                    gnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
                    sub_26d0d30(*reinterpret_cast<void**>(gnm + 0x38),
                                gnm + 0x10,
                                &cpuVerts,
                                vtxStream);
                } else {
                    if (*reinterpret_cast<uint32_t*>(vtxStream + 0x294) != curFrameIdx) {
                        int32_t ringSlot = (*reinterpret_cast<int32_t*>(vtxStream + 0x28c) + 1) %
                                           (*reinterpret_cast<int32_t*>(vtxStream + 0x290));
                        *reinterpret_cast<int32_t*>(vtxStream + 0x28c)  = ringSlot;
                        *reinterpret_cast<uint32_t*>(vtxStream + 0x294) = curFrameIdx;
                        int32_t res = sub_2165150(vtxStream, 1, curFrameIdx);
                        if (res >= 0) {
                            cpuVerts = *reinterpret_cast<void**>(
                                vtxStream + static_cast<int64_t>(*reinterpret_cast<int32_t*>(vtxStream + 0x28c)) * 8 + 0x88);
                        }
                    } else {
                        cpuVerts = *reinterpret_cast<void**>(
                            vtxStream + static_cast<int64_t>(*reinterpret_cast<int32_t*>(vtxStream + 0x28c)) * 8 + 0x88);
                    }
                }

                if (cpuVerts != nullptr) {
                    auto* vpMgr = static_cast<uint8_t*>(g_ViewportManagerSingleton_5540dc0);
                    float vpWf = static_cast<float>(vpMgr ? *reinterpret_cast<int32_t*>(vpMgr + 0x38) : 1920);
                    float vpHf = static_cast<float>(vpMgr ? *reinterpret_cast<int32_t*>(vpMgr + 0x3c) : 1080);

                    float x0 = f16 / vpWf + fNeg1;
                    float y0 = f1 - f16 / vpHf;
                    float x1 = (f16 - fNeg256) / vpWf + fNeg1;
                    float y1 = f1 - (f16 + f16) / vpHf;

                    struct QuadVertex {
                        float x, y, z;
                        uint32_t color;
                        float u, v;
                    };
                    auto* qv = static_cast<QuadVertex*>(cpuVerts);
                    qv[0] = {x0, y0, 0.0f, 0xffffffffu, 0.0f, 0.0f};
                    qv[1] = {x1, y0, 0.0f, 0xffffffffu, f1,   0.0f};
                    qv[2] = {x0, y1, 0.0f, 0xffffffffu, 0.0f, f1};
                    qv[3] = {x1, y1, 0.0f, 0xffffffffu, f1,   f1};
                }

                void* vsDecl = *reinterpret_cast<void**>(director + 0x2650);
                if (*reinterpret_cast<void**>(cmdCtx + 0xf0) != vsDecl) {
                    *reinterpret_cast<void**>(cmdCtx + 0xf0) = vsDecl;
                    cmdCtx[0x20d] = 1;
                }
                if (*reinterpret_cast<void**>(cmdCtx + 0x10) != vtxStream) {
                    *reinterpret_cast<void**>(cmdCtx + 0x10) = vtxStream;
                    cmdCtx[0x20b] = 1;
                }
                if (*reinterpret_cast<int32_t*>(cmdCtx + 0x50) != 0) {
                    *reinterpret_cast<int32_t*>(cmdCtx + 0x50) = 0;
                    cmdCtx[0x20b] = 1;
                }
                if (*reinterpret_cast<int32_t*>(cmdCtx + 0x90) != 24) {
                    *reinterpret_cast<int32_t*>(cmdCtx + 0x90) = 24;
                    cmdCtx[0x20b] = 1;
                }

                // Set sampler state slot 0
                gnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
                sb  = gnm + 0xb730;
                psBaseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b30)) * 0x5c8;
                auto* smpSlot = sb + psBaseOff + 0x1510;
                *reinterpret_cast<uint64_t*>(smpSlot + 0x00) = 0;
                *reinterpret_cast<uint64_t*>(smpSlot + 0x08) = 0x3ff0000000000000ull;
                *reinterpret_cast<uint32_t*>(smpSlot + 0x10) = 0x02020200u;
                *reinterpret_cast<uint64_t*>(smpSlot + 0x14) = 0;
                *reinterpret_cast<uint64_t*>(smpSlot + 0x1c) = 0x3f80000000000000ull;
                *reinterpret_cast<uint64_t*>(smpSlot + 0x24) = 0x0000000141800000ull;
                *reinterpret_cast<uint32_t*>(sb + psBaseOff + 0x1688) |= 1u;
                *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 2u;

                sub_21a1a00(cmdCtx);
                sub_21a1c50(cmdCtx);

                gnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
                sub_21696c0(gnm, gnm + 0xb730, 4, 4);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Part E: CustomDof 2048-Entry CoC LUT Generation & Compute Dispatch
    //         ("CustomDofLutCopy.cpo", dispatch 1x1x1)
    // ------------------------------------------------------------------------
    if (*reinterpret_cast<int32_t*>(param + 0x04c) > 0 &&
        *reinterpret_cast<int32_t*>(param + 0x1ac) != 0 &&
        (*reinterpret_cast<int32_t*>(param + 0x2d0) != 0 || director[0x2669] != 0) &&
        *reinterpret_cast<void**>(director + 0x2198) != nullptr) {

        void* ppfx = *reinterpret_cast<void**>(director + 0x2670);
        auto* sceneView = *reinterpret_cast<uint8_t**>(cmdCtx + 0x210);
        float zNear = *reinterpret_cast<float*>(sceneView + 0x54);
        float zFar  = *reinterpret_cast<float*>(sceneView + 0x58);
        float zRange = zFar - zNear;

        int32_t customDofEnable = *reinterpret_cast<int32_t*>(param + 0x2d0);
        int32_t nearCustom      = *reinterpret_cast<int32_t*>(param + 0x2d4);
        float nearStart         = *reinterpret_cast<float*>(param + 0x2dc);
        float nearEnd           = *reinterpret_cast<float*>(param + 0x2e0);
        float nearCocSize       = *reinterpret_cast<float*>(param + 0x2e4);
        float nearCocScale      = *reinterpret_cast<float*>(param + 0x2e8);
        int32_t farCustom       = *reinterpret_cast<int32_t*>(param + 0x2d8);
        float farStart          = *reinterpret_cast<float*>(param + 0x2ec);
        float farEnd            = *reinterpret_cast<float*>(param + 0x2f0);
        float farCocSize        = *reinterpret_cast<float*>(param + 0x2f4);
        float farCocScale       = *reinterpret_cast<float*>(param + 0x2f8);
        float farDistThresh     = *reinterpret_cast<float*>(param + 0x2fc);

        auto* cocLutBuf = reinterpret_cast<float*>(director + 0x258);
        for (int32_t i = 0; i < 2048; ++i) {
            float normIdx = static_cast<float>(i) * fInv2047;
            float baseCoc = sub_bb2dc0(ppfx, 0, 2, normIdx);

            float outCoc = baseCoc;
            if (customDofEnable != 0) {
                float linDepth = (zFar * zNear) / (zFar - normIdx * zRange);
                if (baseCoc <= 0.0f) {
                    // Near field
                    if (nearCustom != 0) {
                        if (nearStart > linDepth) {
                            float denom = nearStart - nearEnd;
                            if (std::fabs(denom) < f1e_7) {
                                outCoc = -nearCocSize;
                            } else {
                                float t = (nearStart - linDepth) / denom;
                                if (t > f1) t = f1;
                                outCoc = t * (-nearCocSize);
                            }
                        } else {
                            outCoc = 0.0f;
                        }
                    } else {
                        outCoc = baseCoc * nearCocScale;
                    }
                } else {
                    // Far field
                    if (farCustom != 0) {
                        float focusDist = *reinterpret_cast<float*>(param + 0x1b0);
                        float effFarStart = (focusDist > farDistThresh) ? focusDist : farStart;
                        if (linDepth > effFarStart) {
                            float denom = farEnd - effFarStart;
                            if (std::fabs(denom) < f1e_7) {
                                outCoc = farCocSize;
                            } else {
                                float t = (linDepth - effFarStart) / denom;
                                if (t > f1) t = f1;
                                outCoc = t * farCocSize;
                            }
                        } else {
                            outCoc = 0.0f;
                        }
                    } else {
                        outCoc = baseCoc * farCocScale;
                    }
                }
            }
            cocLutBuf[i] = outCoc;
        }

        auto* gnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x08);
        void* dofLutUav = director + 0x1b8;
        sub_2168a20(gnm, gnm + 0xb730, dofLutUav, 0, 0, nullptr, 0, 0);

        void* cbGpuAddr = nullptr;
        auto* cbAlloc = *reinterpret_cast<DLAllocator**>(gnm + 0x38);
        if (cbAlloc && cbAlloc->vtable && cbAlloc->vtable->allocate_58) {
            cbGpuAddr = cbAlloc->vtable->allocate_58(cbAlloc, 0x2000, 0x10);
        }
        if (cbGpuAddr != nullptr) {
            sub_2bbe578(cbGpuAddr, cocLutBuf, 0x2000);
        }

        auto* sb = gnm + 0xb730;
        int64_t baseOff = static_cast<int64_t>(*reinterpret_cast<int32_t*>(sb + 0x3b38)) * 0x5c8;
        *reinterpret_cast<void**>(sb + baseOff + 0x3a60) = cbGpuAddr;
        *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3a68) = 0x2000u;
        *reinterpret_cast<uint32_t*>(sb + baseOff + 0x3b2c) |= 1u;
        *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x20u;

        void* shdrCont = *reinterpret_cast<void**>(director + 0x2198);
        auto* csEntry = static_cast<uint8_t*>(
            sub_2274e20(shdrCont, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b7cULL))));
        void* csProg  = csEntry ? *reinterpret_cast<void**>(csEntry + 0x48) : nullptr;
        sb = gnm + 0xb730;
        if (*reinterpret_cast<void**>(sb + 0x3578) != csProg) {
            *reinterpret_cast<void**>(sb + 0x3578) = csProg;
            *reinterpret_cast<uint32_t*>(sb + 0x3b58) |= 0x2000u;
        }

        auto* dispHelper = gnm + 0x50;
        if (dispHelper[0x50] == 0) {
            sub_26af700(gnm + 0x10);
            dispHelper[0x50] = 1;
        }
        sub_26cbe50(dispHelper, gnm, gnm + 0xb730);
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
            gx_direct_vk::EmitDispatchDirect(gnm + 0x10, 1u, 1u, 1u);
        } else {
            sub_1074c50(gnm + 0x10, 1u, 1u, 1u);
        }
        sub_26ccf20(dispHelper, gnm, gnm + 0xb730);

        sub_2168a20(gnm, gnm + 0xb730, nullptr, 0, 0, nullptr, 0, 0);

        director[0x2669] = (*reinterpret_cast<int32_t*>(param + 0x2d0) != 0) ? 1u : 0u;
    }
}

// ============================================================================
// 16. 0x21d5d90 (484 B) - GXYebisPostProcessDirector_JobEntryExecute
// ============================================================================
void sub_21d5d90(void* workerCtx, void* director, void* packet) {
    alignas(16) uint8_t localCtx[0x220] = {};
    *reinterpret_cast<void**>(localCtx + 0x00) = workerCtx;
    *reinterpret_cast<void**>(localCtx + 0x08) =
        *reinterpret_cast<void**>(static_cast<uint8_t*>(workerCtx) + 0x08);

    auto initIdentity4x4 = [](uint8_t* m) {
        auto* f = reinterpret_cast<float*>(m);
        f[0]  = 1.0f;
        f[5]  = 1.0f;
        f[10] = 1.0f;
        f[15] = 1.0f;
    };
    initIdentity4x4(localCtx + 0x10);
    initIdentity4x4(localCtx + 0x90);
    initIdentity4x4(localCtx + 0xd0);

    auto* dev = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
    auto* vp  = *reinterpret_cast<uint8_t**>(dev + 0x90);
    *reinterpret_cast<uint32_t*>(localCtx + 0x110) = *reinterpret_cast<uint32_t*>(vp + 0x80);
    *reinterpret_cast<uint32_t*>(localCtx + 0x114) = *reinterpret_cast<uint32_t*>(vp + 0x84);
    *reinterpret_cast<uint32_t*>(localCtx + 0x140) = 0x3f75be0bu;
    localCtx[0x218] = 1u;

    sub_2192d20(localCtx, static_cast<uint8_t*>(packet) + 0x3a0);
    sub_21d5f80(director, localCtx, packet);
}

// ============================================================================
// 17. 0x21d5a80 (772 B) - GXYebisPostProcessDirector_DispatchOrEnqueuePostProcess
// ============================================================================
void sub_21d5a80(void* self, void* passCtx, void** packetPtrPtr) {
    if (!g_YebisPostProcessEnabled_56c3ce0) {
        return;
    }

    auto* pass = static_cast<uint8_t*>(passCtx);
    auto* mgr  = *reinterpret_cast<uint8_t**>(pass + 0x430);
    if (mgr[0x2093] != 0) {
        sub_21d5f80(self, pass + 0x440, *packetPtrPtr);
        return;
    }

    auto* srcPkt = static_cast<const uint8_t*>(*packetPtrPtr);
    auto* dev    = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
    int64_t qIdx = *reinterpret_cast<int32_t*>(dev + 0x380);
    auto* queue  = *reinterpret_cast<uint8_t**>(dev + 0x360 + qIdx * 8);

    auto* mutex     = queue + 0xc8;
    auto* mutexVtbl = *reinterpret_cast<uint8_t**>(mutex);
    (*reinterpret_cast<void (**)(void*, int64_t)>(mutexVtbl + 0x18))(mutex, -1);

    auto* dstPkt = static_cast<uint8_t*>(sub_e45560(queue + 0x78, 0x4d0, 0x10));

    mutexVtbl = *reinterpret_cast<uint8_t**>(mutex);
    (*reinterpret_cast<void (**)(void*)>(mutexVtbl + 0x28))(mutex);

    if (dstPkt == nullptr) {
        return;
    }

    sub_2bbe578(dstPkt, srcPkt, 0x398);
    std::memcpy(dstPkt + 0x3a0, srcPkt + 0x3a0, 0x68);
    *reinterpret_cast<uint32_t*>(dstPkt + 0x40c) = *reinterpret_cast<const uint32_t*>(srcPkt + 0x40c);
    std::memcpy(dstPkt + 0x410, srcPkt + 0x410, 0x14);
    *reinterpret_cast<uint64_t*>(dstPkt + 0x430) = *reinterpret_cast<const uint64_t*>(srcPkt + 0x430);
    std::memcpy(dstPkt + 0x440, srcPkt + 0x440, 0x68);
    *reinterpret_cast<uint32_t*>(dstPkt + 0x4ac) = *reinterpret_cast<const uint32_t*>(srcPkt + 0x4ac);
    std::memcpy(dstPkt + 0x4b0, srcPkt + 0x4b0, 0x14);

    dev   = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
    qIdx  = *reinterpret_cast<int32_t*>(dev + 0x380);
    queue = *reinterpret_cast<uint8_t**>(dev + 0x360 + qIdx * 8);

    uint32_t sortKey =
        (*reinterpret_cast<uint32_t*>(pass + 0x110) << 28) |
        ((*reinterpret_cast<uint32_t*>(pass + 0x114) & 0x3fu) << 22) |
        ((*reinterpret_cast<uint32_t*>(pass + 0x118) & 0x3fu) << 16) |
        static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(pass + 0x11c));

    sub_21b4b00(queue, reinterpret_cast<void*>(&sub_21d5d90), self, dstPkt, sortKey);
}

// ============================================================================
// 18. 0x21d5660 (1,050 B) - GXYebisPostProcessDirector_ApplyExternalColorGradingParams
// ============================================================================
void sub_21d5660(void* self, const void* extColorGradingParam) {
    auto* director = static_cast<uint8_t*>(self);
    auto* src      = static_cast<const uint8_t*>(extColorGradingParam);
    if (director[0x2710] != 0) {
        return;
    }

    auto* gp = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
    std::memcpy(gp + 0x2b8, src + 0x000, 16);
    *reinterpret_cast<uint32_t*>(gp + 0x2c8) = *reinterpret_cast<const uint32_t*>(src + 0x010);
    std::memcpy(gp + 0x358, src + 0x230, 16);

    auto replaceDLWStringAndFlag = [](uint8_t* dstObj,
                                      const uint8_t* srcBase,
                                      size_t srcDataOff,
                                      size_t srcFlagOff) {
        const char16_t* srcChars = (*reinterpret_cast<const uint64_t*>(srcBase + srcDataOff + 0x18) >= 8)
            ? *reinterpret_cast<const char16_t* const*>(srcBase + srcDataOff)
            : reinterpret_cast<const char16_t*>(srcBase + srcDataOff);
        uint64_t srcLen = *reinterpret_cast<const uint64_t*>(srcBase + srcDataOff + 0x10);

        const char16_t* dstChars = (*reinterpret_cast<const uint64_t*>(dstObj + 0x20) >= 8)
            ? *reinterpret_cast<const char16_t* const*>(dstObj + 0x08)
            : reinterpret_cast<const char16_t*>(dstObj + 0x08);
        uint64_t dstLen = *reinterpret_cast<const uint64_t*>(dstObj + 0x18);

        DLWStrIter24 dstBeg{0, 0, dstChars};
        DLWStrIter24 dstEnd{0, 0, dstChars + dstLen};
        DLWStrIter24 srcBeg{0, 0, srcChars};
        DLWStrIter24 srcEnd{0, 0, srcChars + srcLen};
        sub_2724ff0(dstObj, dstBeg, dstEnd, srcBeg, srcEnd);
        dstObj[0x30] = srcBase[srcFlagOff];
    };

    replaceDLWStringAndFlag(director + 0x2100, src, 0x020, 0x0c2);
    replaceDLWStringAndFlag(director + 0x2138, src, 0x0d0, 0x172);
    replaceDLWStringAndFlag(director + 0x2170, src, 0x180, 0x222);
}

// ============================================================================
// 19. 0x21d5620 (50 B) - GXYebisPostProcessDirector_ApplyExternalFeedbackBlurParams
// ============================================================================
void sub_21d5620(void* self, const void* extFbBlurParam) {
    auto* director = static_cast<uint8_t*>(self);
    auto* src      = static_cast<const uint8_t*>(extFbBlurParam);
    if (director[0x2710] != 0) {
        return;
    }

    auto* gp = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
    *reinterpret_cast<uint32_t*>(gp + 0x21c) = *reinterpret_cast<uint32_t*>(gp + 0x208);
    *reinterpret_cast<uint32_t*>(gp + 0x208) = *reinterpret_cast<const uint32_t*>(src + 0x00);
    std::memcpy(gp + 0x20c, src + 0x04, 16);
}

// ============================================================================
// 20. 0x21d5560 (180 B) - GXYebisPostProcessDirector_ApplyExternalMotionBlurParams
// ============================================================================
void sub_21d5560(void* self, const void* extMotionBlurParam) {
    auto* director = static_cast<uint8_t*>(self);
    auto* src      = static_cast<const uint8_t*>(extMotionBlurParam);
    if (director[0x2710] != 0) {
        return;
    }

    auto* gp = static_cast<uint8_t*>(g_GlobalYebisParamPtr_53e4660);
    *reinterpret_cast<uint32_t*>(gp + 0x1dc) = *reinterpret_cast<const uint32_t*>(src + 0x00);
    *reinterpret_cast<uint32_t*>(gp + 0x1e0) = *reinterpret_cast<const uint32_t*>(src + 0x04);
    *reinterpret_cast<uint32_t*>(gp + 0x1e4) = *reinterpret_cast<const uint32_t*>(src + 0x08);
    *reinterpret_cast<uint32_t*>(gp + 0x1e8) = *reinterpret_cast<const uint32_t*>(src + 0x0c);
    *reinterpret_cast<uint32_t*>(gp + 0x350) = *reinterpret_cast<const uint32_t*>(src + 0x30);
    *reinterpret_cast<uint32_t*>(gp + 0x1ec) = *reinterpret_cast<const uint32_t*>(src + 0x10);
    *reinterpret_cast<uint32_t*>(gp + 0x1f0) = *reinterpret_cast<const uint32_t*>(src + 0x14);
    *reinterpret_cast<uint32_t*>(gp + 0x1f4) = *reinterpret_cast<const uint32_t*>(src + 0x18);
    *reinterpret_cast<uint32_t*>(gp + 0x1f8) = *reinterpret_cast<const uint32_t*>(src + 0x1c);
    *reinterpret_cast<uint32_t*>(gp + 0x1fc) = *reinterpret_cast<const uint32_t*>(src + 0x20);
    *reinterpret_cast<uint32_t*>(gp + 0x200) = *reinterpret_cast<const uint32_t*>(src + 0x24);
    *reinterpret_cast<uint32_t*>(gp + 0x2ac) = *reinterpret_cast<const uint32_t*>(src + 0x28);
    *reinterpret_cast<uint32_t*>(gp + 0x2b0) = *reinterpret_cast<const uint32_t*>(src + 0x2c);
    gp[0x204] = (*reinterpret_cast<const uint32_t*>(src + 0x34) != 0) ? 1u : 0u;
}

} // extern "C"
