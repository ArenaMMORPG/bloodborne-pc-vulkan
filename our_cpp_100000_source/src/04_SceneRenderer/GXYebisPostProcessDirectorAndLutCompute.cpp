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
int32_t sub_bb5f10(void* ppfxCtx, void* gnmTexturePtr);                          // 0xbb5f10 (ReleaseTextureRef)
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
    std::memset(base + 0x1cfd0, 0, 0x50);
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
    cb->refFlags    = 0x40000000u;
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

        if (fbToggleNode) {
            DLAllocator* alloc = g_UiCallbackAllocator_5540e08;
            GXYebisMemberCallbackVoid* fbCb = nullptr;
            if (alloc && alloc->vtable && alloc->vtable->allocate_58) {
                fbCb = static_cast<GXYebisMemberCallbackVoid*>(
                    alloc->vtable->allocate_58(alloc, 0x28, 0x8));
                if (fbCb) {
                    fbCb->refFlags    = 0x40000000u;
                    fbCb->vtable      = vtable_53b6fe0;
                    fbCb->targetObj   = director;
                    fbCb->fnPtrOrSlot = reinterpret_cast<uintptr_t>(&sub_21d4210);
                    fbCb->adjOffset   = 0;
                }
            }
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

    // 0x21d5fbb: early exit if filterParam->m_bEnable (+0x00) == 0
    if (*reinterpret_cast<const int32_t*>(param + 0x000) == 0) {
        return;
    }

    const float f1       = BitsToFloat(0x3f800000u); // 0x4929794: 1.0f
    const float fPiDiv6  = BitsToFloat(0x3f060a92u); // 0x4929798: pi/6
    const float f1Div60  = BitsToFloat(0x3c888889u); // 0x492979c: 1.0f/60.0f
    const float fNegMax  = BitsToFloat(0xff7fffffu); // 0x49297a0: -FLT_MAX
    const float fPi      = BitsToFloat(0x40490fdbu); // 0x49297a4: pi
    const float f180     = BitsToFloat(0x43340000u); // 0x49297a8: 180.0f
    const float fDeg2Rad = BitsToFloat(0x3c8efa35u); // 0x49297ac: pi/180
    const float f0_3     = BitsToFloat(0x3e99999au); // 0x49297b0: 0.3f
    const float f0_9995  = BitsToFloat(0x3f7fe143u); // 0x49297b4: 0.99953097f
    const float f0_001   = BitsToFloat(0x3a83126fu); // 0x49297b8: 0.001f
    const float f0_0001  = BitsToFloat(0x38d1b717u); // 0x49297bc: 0.0001f
    const float fNeg1    = BitsToFloat(0xbf800000u); // 0x49297c0: -1.0f
    const float fNeg0    = BitsToFloat(0x80000000u); // 0x49297c4: -0.0f
    const float f16      = BitsToFloat(0x41800000u); // 0x49297c8: 16.0f
    const float fNeg256  = BitsToFloat(0xc3800000u); // 0x49297cc: -256.0f
    const float f1e_7    = BitsToFloat(0x33d6bf95u); // 0x49297d0: 1e-7f
    const float fInv2047 = BitsToFloat(0x3a001002u); // 0x49297d4: 1.0f/2047.0f

    void* const savedPpfxCtx = *reinterpret_cast<void**>(director + 0x2670);   // 0xb0(%rsp)
    auto* const resBank      = *reinterpret_cast<uint8_t**>(director + 0x010); // 0x140(%rsp)
    auto*       devCtx       = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);   // %r12
    uint8_t* const globalParam = g_GlobalYebisFilterParam_55406e0;             // 0x55406e0

    auto callTexGetSrv18 = [](void* texObj) -> void* {
        auto* vt = *reinterpret_cast<uint8_t**>(texObj);
        auto fn  = *reinterpret_cast<void* (**)(void*)>(vt + 0x18);
        return fn(texObj);
    };
    auto callTexGetWidth48 = [](void* texObj, int32_t mip) -> uint32_t {
        auto* vt = *reinterpret_cast<uint8_t**>(texObj);
        auto fn  = *reinterpret_cast<uint32_t (**)(void*, int32_t)>(vt + 0x48);
        return fn(texObj, mip);
    };
    auto callTexGetHeight50 = [](void* texObj, int32_t mip) -> uint32_t {
        auto* vt = *reinterpret_cast<uint8_t**>(texObj);
        auto fn  = *reinterpret_cast<uint32_t (**)(void*, int32_t)>(vt + 0x50);
        return fn(texObj, mip);
    };

    // ========================================================================
    // Part A (0x21d5fe2..0x21d666f): 2D-to-3D Color Grading LUT Compute Pass
    // ========================================================================
    if (*reinterpret_cast<const int32_t*>(globalParam + 0x2b8) != 0) {
        void* texA = *reinterpret_cast<void**>(param + 0x358);
        void* texB = *reinterpret_cast<void**>(param + 0x360);
        void* srv0 = nullptr;
        uint32_t texW = 0x10u;
        uint32_t texH = 0x100u;
        if (texA != nullptr) {
            texW = callTexGetWidth48(texA, 0);
            texH = callTexGetHeight50(texA, 0);
            srv0 = callTexGetSrv18(texA);
        }
        void* srv1 = (texB != nullptr) ? callTexGetSrv18(texB) : nullptr;
        if (srv0 == nullptr && srv1 == nullptr) {
            srv1 = *reinterpret_cast<void**>(director + 0x068);
            srv0 = srv1;
        } else if (srv1 == nullptr) {
            srv1 = (texA != nullptr) ? callTexGetSrv18(texA)
                                     : *reinterpret_cast<void**>(director + 0x068);
        } else if (srv0 == nullptr) {
            srv0 = (texB != nullptr) ? callTexGetSrv18(texB)
                                     : *reinterpret_cast<void**>(director + 0x068);
        }

        if (director[0x21a9] != 0 || srv0 == nullptr || texW != 0x10u || texH != 0x100u) {
            srv0 = *reinterpret_cast<void**>(director + 0x068);
        }

        if (srv0 != nullptr) {
            *reinterpret_cast<uint32_t*>(director + 0x078) = 0x10u;
            *reinterpret_cast<uint32_t*>(director + 0x07c) = 0x100u;
            *reinterpret_cast<uint32_t*>(director + 0x080) = 0x100u;
            *reinterpret_cast<uint32_t*>(director + 0x084) = 0x10u;

            const float blend0 = *reinterpret_cast<const float*>(globalParam + 0x2c8);
            *reinterpret_cast<float*>(director + 0x0a8) = blend0;
            *reinterpret_cast<float*>(director + 0x0ac) = *reinterpret_cast<const float*>(globalParam + 0x2c8);
            *reinterpret_cast<float*>(director + 0x0b0) = *reinterpret_cast<const float*>(globalParam + 0x2c8);
            *reinterpret_cast<float*>(director + 0x0b4) = *reinterpret_cast<const float*>(globalParam + 0x2c8);

            auto* uav3d = *reinterpret_cast<uint8_t**>(director + 0x070);
            static uint8_t* s_lastLut3dUav = nullptr;
            static void*    s_lastLutSrv0  = nullptr;
            static void*    s_lastLutSrv1  = nullptr;
            static float    s_lastLutBlend = -1.0e30f;
            static uint8_t  s_lastSrv0TSharp[32] = {};
            static uint8_t  s_lastSrv1TSharp[32] = {};
            const uint8_t* srv0Bytes = static_cast<const uint8_t*>(srv0);
            const uint8_t* srv1Bytes = static_cast<const uint8_t*>(srv1);
            const bool lut3dUnchanged =
                (uav3d != nullptr) &&
                (uav3d == s_lastLut3dUav) &&
                (srv0 == s_lastLutSrv0) &&
                (srv1 == s_lastLutSrv1) &&
                (blend0 == s_lastLutBlend) &&
                (std::memcmp(s_lastSrv0TSharp, srv0Bytes + 0x10, 32) == 0) &&
                (srv1Bytes == nullptr || std::memcmp(s_lastSrv1TSharp, srv1Bytes + 0x10, 32) == 0);

            if (!lut3dUnchanged) {
                s_lastLut3dUav = uav3d;
                s_lastLutSrv0  = srv0;
                s_lastLutSrv1  = srv1;
                s_lastLutBlend = blend0;
                std::memcpy(s_lastSrv0TSharp, srv0Bytes + 0x10, 32);
                if (srv1Bytes != nullptr) {
                    std::memcpy(s_lastSrv1TSharp, srv1Bytes + 0x10, 32);
                }

                auto* gnm   = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                sub_2168a20(gnm, gnm + 0xb730, uav3d, 0, 0, director + 0x078, 0, 0);

                *reinterpret_cast<void**>(devCtx + 0x1508)    = uav3d;
                *reinterpret_cast<uint32_t*>(devCtx + 0x1578) = 0u;
                const uint32_t prevUavEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x157c);
                *reinterpret_cast<uint32_t*>(devCtx + 0x157c) = (prevUavEnd > 1u) ? prevUavEnd : 1u;

                gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x10000ULL;
                if (uav3d != nullptr) {
                    constexpr uint32_t kUavMask0 = 0x200001u;
                    auto* pState = reinterpret_cast<uint32_t*>(uav3d + 0x44);
                    if ((*pState & kUavMask0) != kUavMask0) {
                        __atomic_fetch_or(pState, kUavMask0, __ATOMIC_SEQ_CST);
                    }
                }
                *reinterpret_cast<void**>(gnm + 0xda20) = uav3d;

                void* lutCb = *reinterpret_cast<void**>(director + 0x038);

                // Clear graphics shaders VS/PS/GS/HS/DS on devCtx and gnm
                *reinterpret_cast<uint64_t*>(devCtx + 0x0780) = 0;
                *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc1b8) = 0;
                *reinterpret_cast<uint64_t*>(devCtx + 0x0a68) = 0;
                *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd598) = 0;
                *reinterpret_cast<uint64_t*>(devCtx + 0x0d50) = 0;
                *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd0a0) = 0;
                *reinterpret_cast<uint64_t*>(devCtx + 0x1320) = 0;
                *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc6b0) = 0;
                *reinterpret_cast<uint64_t*>(devCtx + 0x1038) = 0;
                *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xcba8) = 0;

                auto* devSingleton = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
                auto* statePool    = *reinterpret_cast<uint8_t**>(devSingleton + 0x250);
                void* linSampler   = *reinterpret_cast<void**>(statePool + 0x2d0);

                *reinterpret_cast<void**>(devCtx + 0x1580) = linSampler;
                *reinterpret_cast<void**>(devCtx + 0x1588) = linSampler;
                *reinterpret_cast<uint32_t*>(devCtx + 0x1600) = 0u;
                const uint32_t prevSampEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x1604);
                *reinterpret_cast<uint32_t*>(devCtx + 0x1604) = (prevSampEnd > 2u) ? prevSampEnd : 2u;
                gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x3ULL;
                *reinterpret_cast<void**>(gnm + 0xd5a0) = linSampler;
                *reinterpret_cast<void**>(gnm + 0xd5a8) = linSampler;

                auto* csEntry = static_cast<uint8_t*>(
                    sub_2274e20(resBank + 0x9a8, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b02ULL))));
                const float blendCheck = *reinterpret_cast<const float*>(globalParam + 0x2c8);
                if (blendCheck > 0.0f && f1 > blendCheck) {
                    csEntry = static_cast<uint8_t*>(
                        sub_2274e20(resBank + 0x9a8, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b2aULL))));
                }

                if (csEntry != nullptr) {
                    void* csObj = *reinterpret_cast<void**>(csEntry + 0x010);
                    *reinterpret_cast<void**>(devCtx + 0x1608) = csObj;
                    *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xda90) = csObj;

                    *reinterpret_cast<void**>(devCtx + 0x1610)    = lutCb;
                    *reinterpret_cast<uint32_t*>(devCtx + 0x1650) = 0xffffffffu;
                    gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                    *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1000000000ULL;
                    *reinterpret_cast<uint32_t*>(gnm + 0x1fd0)  = 0xffffffffu;
                    *reinterpret_cast<void**>(gnm + 0xda98)     = lutCb;

                    *reinterpret_cast<void**>(devCtx + 0x1328)    = srv0;
                    *reinterpret_cast<void**>(devCtx + 0x1330)    = srv1;
                    *reinterpret_cast<uint32_t*>(devCtx + 0x1500) = 0u;
                    const uint32_t prevSrvEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x1504);
                    *reinterpret_cast<uint32_t*>(devCtx + 0x1504) = (prevSrvEnd > 2u) ? prevSrvEnd : 2u;
                    gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                    *reinterpret_cast<uint64_t*>(gnm + 0x1fa8) |= 0x3ULL;
                    *reinterpret_cast<void**>(gnm + 0xd620) = srv0;
                    *reinterpret_cast<void**>(gnm + 0xd628) = srv1;

                    sub_2bbe688(devCtx + 0x1670, 0, 0xb8);
                    *reinterpret_cast<uint64_t*>(devCtx + 0x1680) = 0xeULL;
                    *reinterpret_cast<uint64_t*>(devCtx + 0x1688) = 0x8ULL;

                    gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                    sub_26af700(gnm + 0x010);
                    sub_26cbe50(gnm + 0x050, gnm, gnm + 0xb730);
                    if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
                        gx_direct_vk::EmitDispatchDirect(gnm + 0x010, 1u, 1u, 16u);
                    } else {
                        sub_1074c50(gnm + 0x010, 1u, 1u, 16u);
                    }
                    sub_26ccf20(gnm + 0x050, gnm, gnm + 0xb730);
                    sub_26af700(gnm + 0x010);
                }

                // Unbind CS CB0, CS SRV0..1, CS shader, and flush dirty state
                *reinterpret_cast<void**>(devCtx + 0x1610)    = nullptr;
                *reinterpret_cast<uint32_t*>(devCtx + 0x1650) = 0xffffffffu;
                gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1000000000ULL;
                *reinterpret_cast<uint32_t*>(gnm + 0x1fd0)  = 0xffffffffu;
                *reinterpret_cast<void**>(gnm + 0xda98)     = nullptr;

                *reinterpret_cast<void**>(devCtx + 0x1328)    = nullptr;
                *reinterpret_cast<void**>(devCtx + 0x1330)    = nullptr;
                *reinterpret_cast<uint32_t*>(devCtx + 0x1500) = 0u;
                const uint32_t prevSrvEnd2 = *reinterpret_cast<const uint32_t*>(devCtx + 0x1504);
                *reinterpret_cast<uint32_t*>(devCtx + 0x1504) = (prevSrvEnd2 > 2u) ? prevSrvEnd2 : 2u;
                gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                *reinterpret_cast<uint64_t*>(gnm + 0x1fa8) |= 0x3ULL;
                *reinterpret_cast<void**>(gnm + 0xd628) = nullptr;
                *reinterpret_cast<void**>(gnm + 0xd620) = nullptr;

                *reinterpret_cast<void**>(devCtx + 0x1608) = nullptr;
                *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xda90) = nullptr;

                sub_219fcf0(devCtx);
            }
        }
    }

    // ========================================================================
    // Part B (0x21d666f..0x21d8093): Bind Yebis RT & Configure Yebis2 PPFX
    // ========================================================================
    {
        auto* curDevCtx = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        sub_e46f40(curDevCtx + 0x1728, *reinterpret_cast<void**>(param + 0x390), 0, 1);
        *reinterpret_cast<void**>(curDevCtx + 0x1ec0) = sub_e47840(curDevCtx + 0x1728);
        sub_21a1a00(curDevCtx);

        void* inColorSrv = *reinterpret_cast<void**>(param + 0x370);
        *reinterpret_cast<void**>(devCtx + 0x0788)    = inColorSrv;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = 0u;
        const uint32_t prevPsSrvEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (prevPsSrvEnd > 1u) ? prevPsSrvEnd : 1u;
        auto* gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd128) = inColorSrv;

        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        alignas(32) uint8_t zeroClearDesc[0x48] = {};
        sub_26d3070(gnm + 0xb558, gnm, zeroClearDesc);
        sub_26b9850(gnm + 0xb478, gnm + 0x010);
        sub_26cd190(gnm + 0x0050, gnm);
        sub_26af700(gnm + 0x0010);
        sub_26bba10(gnm + 0xb518);
        sub_26d3300(gnm + 0xb528);
        sub_26d2d20(gnm + 0xb558);
        sub_2bbe688(gnm + 0xb730, 0, 0x23a8);

        auto* outRtWrap = *reinterpret_cast<uint8_t**>(param + 0x390);
        sub_108c520(g_ActiveYebisGpuCtx_56c3cb8, 0, outRtWrap + 0x010);

        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        if (sub_26b9a50(gnm + 0xb478)) {
            sub_26b9a40(gnm + 0xb478);
        }
        sub_26afa00(gnm + 0x010);
    }

    gx_direct_vk::YebisSceneScope yebisSceneGuard{false};
    void* ppfxCtx = *reinterpret_cast<void**>(director + 0x2670);
    if (ppfxCtx != nullptr &&
        *reinterpret_cast<const int32_t*>(param + 0x000) != 0 &&
        director[0x020] == 0)
    {
        yebisSceneGuard.Enter();
        auto* const inDepthWrap = *reinterpret_cast<uint8_t**>(param + 0x380); // 0xa0(%rsp)
        auto* const velWrap     = *reinterpret_cast<uint8_t**>(param + 0x388); // 0xa8(%rsp)

        director[0x020] = 1;
        sub_bb4d70(ppfxCtx, 0);
        sub_bb4de0(ppfxCtx, g_YebisBufferFlipIndex_56a6d20);

        auto* sceneView = *reinterpret_cast<uint8_t**>(cmdCtx + 0x210);
        const uint32_t viewMode = *reinterpret_cast<const uint32_t*>(sceneView + 0x070);
        if (viewMode >= 2u) {
            sub_bb26e0(ppfxCtx, 0);
            sceneView = *reinterpret_cast<uint8_t**>(cmdCtx + 0x210);
        }

        const uint32_t resetFlags = *reinterpret_cast<const uint32_t*>(director + 0x271c);
        *reinterpret_cast<uint32_t*>(director + 0x271c) = 0u;

        __m128 curRow0 = _mm_loadu_ps(reinterpret_cast<const float*>(sceneView + 0x00));
        __m128 curRow1 = _mm_loadu_ps(reinterpret_cast<const float*>(sceneView + 0x10));
        __m128 curRow2 = _mm_loadu_ps(reinterpret_cast<const float*>(sceneView + 0x20));
        __m128 curRow3 = _mm_loadu_ps(reinterpret_cast<const float*>(sceneView + 0x30));

        const float* prevSrc1 = nullptr;
        const float* prevSrc2 = nullptr;
        const float* prevSrc3 = nullptr;
        __m128 prevRow0;
        if ((resetFlags & 1u) != 0u) {
            prevRow0 = curRow0;
            prevSrc1 = reinterpret_cast<const float*>(sceneView + 0x10);
            prevSrc2 = reinterpret_cast<const float*>(sceneView + 0x20);
            prevSrc3 = reinterpret_cast<const float*>(sceneView + 0x30);
        } else {
            prevRow0 = _mm_loadu_ps(reinterpret_cast<const float*>(param + 0x440));
            prevSrc1 = reinterpret_cast<const float*>(param + 0x450);
            prevSrc2 = reinterpret_cast<const float*>(param + 0x460);
            prevSrc3 = reinterpret_cast<const float*>(param + 0x470);
        }

        __m128 prevRow1;
        __m128 prevRow2;
        __m128 prevRow3;
        if (g_DbgCameraBlurFlag_56c3d40) {
            const float sinA = sub_2bbe518(0, 0, fPiDiv6);
            const float cosA = sub_2bbe518(1, 0, fPiDiv6);
            const __m128 vZero  = _mm_setzero_ps();
            const __m128 vSin   = _mm_move_ss(vZero, _mm_set_ss(sinA));
            const __m128 vCos   = _mm_move_ss(vZero, _mm_set_ss(cosA));
            const __m128 rotZ   = _mm_shuffle_ps(vSin, vCos, 0xCC); // [sin, 0, cos, 0]
            const __m128 negX   = _mm_setr_ps(-1.0f, 1.0f, 1.0f, 1.0f); // 0x4920da0
            const __m128 rotX   = _mm_castsi128_ps(
                _mm_shuffle_epi32(_mm_castps_si128(_mm_mul_ps(rotZ, negX)), 0xC6)); // [cos, 0, -sin, 0]
            const __m128 unitY  = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f); // 0x4920db0
            const __m128 unitW  = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f); // 0x4920dc0

            auto rotRowY = [&](__m128 r) noexcept -> __m128 {
                const __m128 rx = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r), 0x00));
                const __m128 ry = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r), 0x55));
                const __m128 rz = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r), 0xAA));
                const __m128 rw = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r), 0xFF));
                return _mm_add_ps(_mm_add_ps(_mm_mul_ps(rx, rotX), _mm_mul_ps(rz, rotZ)),
                                  _mm_add_ps(_mm_mul_ps(ry, unitY), _mm_mul_ps(rw, unitW)));
            };
            prevRow0 = rotRowY(curRow0);
            prevRow1 = rotRowY(curRow1);
            prevRow2 = rotRowY(curRow2);
            prevRow3 = curRow3;
        } else {
            prevRow3 = _mm_loadu_ps(prevSrc3);
            prevRow2 = _mm_loadu_ps(prevSrc2);
            prevRow1 = _mm_loadu_ps(prevSrc1);
        }

        // 0x21d6acb..0x21d6b2a: Camera FOV / Near / Far / Aspect
        sub_bb2710(ppfxCtx,
                   *reinterpret_cast<const float*>(sceneView + 0x054),
                   *reinterpret_cast<const float*>(sceneView + 0x058),
                   *reinterpret_cast<const float*>(sceneView + 0x050),
                   f1Div60,
                   *reinterpret_cast<const float*>(param + 0x094));
        sub_bb2720(ppfxCtx,
                   *reinterpret_cast<const float*>(sceneView + 0x054),
                   *reinterpret_cast<const float*>(sceneView + 0x058),
                   *reinterpret_cast<const float*>(sceneView + 0x050));
        sub_bb2740(ppfxCtx,
                   *reinterpret_cast<const float*>(param + 0x494),
                   *reinterpret_cast<const float*>(param + 0x498),
                   *reinterpret_cast<const float*>(param + 0x490));

        // 0x21d6b2f..0x21d6fa0: Inverse Current & Previous Projection Matrices
        alignas(16) float invMtxBuf[16];
        InvertMatrix4x4Simd(invMtxBuf, reinterpret_cast<const float*>(sceneView + 0x00));
        sub_bb27d0(ppfxCtx, invMtxBuf, 1);

        InvertMatrix4x4Simd(invMtxBuf, reinterpret_cast<const float*>(param + 0x440));
        sub_bb27f0(ppfxCtx, invMtxBuf, 1);

        sub_bb3fb0(ppfxCtx, director[0x2668]);

        // 0x21d6fb5..0x21d7027: Depth Buffer Binding
        if (inDepthWrap != nullptr) {
            auto* inDepthGnm = *reinterpret_cast<uint8_t**>(inDepthWrap + 0x048) + 0x010;
            const uint32_t depthW = callTexGetWidth48(inDepthWrap, 0);
            const uint32_t depthH = callTexGetHeight50(inDepthWrap, 0);
            const uint32_t depthFmt = sub_1089960(inDepthGnm);
            sub_bb5080(ppfxCtx,
                       0,
                       inDepthGnm,
                       depthW,
                       depthH,
                       depthFmt,
                       f1,
                       0.0f,
                       *reinterpret_cast<const int32_t*>(param + 0x150));
        }

        // 0x21d702a..0x21d7173: Auto-Exposure & Tone Mapping
        const uint8_t enableAutoExp =
            (viewMode <= 1u && *reinterpret_cast<const int32_t*>(param + 0x050) != 0) ? 1u : 0u;
        sub_bb31c0(ppfxCtx, 0, enableAutoExp);

        float exposureVal = *reinterpret_cast<const float*>(param + 0x098);
        if (*reinterpret_cast<const int32_t*>(param + 0x050) != 0 && viewMode <= 1u) {
            exposureVal = sub_bb3250(ppfxCtx, 0);
            *reinterpret_cast<float*>(globalParam + 0x098) = exposureVal;
            sub_bb32f0(ppfxCtx,
                       -1,
                       *reinterpret_cast<const float*>(param + 0x054),
                       *reinterpret_cast<const float*>(param + 0x058));
            sub_bb32e0(ppfxCtx,
                       -1,
                       *reinterpret_cast<const float*>(param + 0x05c),
                       *reinterpret_cast<const float*>(param + 0x060));
            sub_bb3300(ppfxCtx,
                       -1,
                       *reinterpret_cast<const float*>(param + 0x064),
                       *reinterpret_cast<const float*>(param + 0x068),
                       *reinterpret_cast<const float*>(param + 0x06c),
                       *reinterpret_cast<const float*>(param + 0x070));
            sub_bb3310(ppfxCtx, -1, 1, *reinterpret_cast<const float*>(param + 0x074));
            sub_bb3320(ppfxCtx,
                       -1,
                       *reinterpret_cast<const float*>(param + 0x078),
                       *reinterpret_cast<const float*>(param + 0x07c),
                       *reinterpret_cast<const float*>(param + 0x080),
                       *reinterpret_cast<const float*>(param + 0x084),
                       *reinterpret_cast<const float*>(param + 0x088),
                       *reinterpret_cast<const float*>(param + 0x08c),
                       *reinterpret_cast<const float*>(param + 0x090));
        }

        sub_bb3550(ppfxCtx,
                   *reinterpret_cast<const int32_t*>(param + 0x0a0),
                   exposureVal,
                   *reinterpret_cast<const float*>(param + 0x09c),
                   *reinterpret_cast<const float*>(param + 0x0a4),
                   fNegMax);
        sub_bb3560(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0a8));

        // 0x21d7178..0x21d71ea: Chromatic Aberration (configured via param + 0xac from sub_229e990)
        const bool caEnabled = gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectChromaticAberration) &&
                               (*reinterpret_cast<const int32_t*>(param + 0x0ac) != 0);
        sub_bb37a0(ppfxCtx, caEnabled ? 1u : 0u);
        const float latDisp[2] = {
            *reinterpret_cast<const float*>(param + 0x0b4),
            *reinterpret_cast<const float*>(param + 0x0b8)
        };
        const float uniDisp[2] = {
            *reinterpret_cast<const float*>(param + 0x0bc),
            *reinterpret_cast<const float*>(param + 0x0c0)
        };
        sub_bb37d0(ppfxCtx, *reinterpret_cast<const int32_t*>(param + 0x0b0), latDisp, uniDisp);

        // 0x21d71ef..0x21d72a1: HSV & Contrast Color Correction
        sub_bb4300(ppfxCtx, -2);
        const float hueDeg = *reinterpret_cast<const float*>(param + 0x0c4);
        if (hueDeg != 0.0f) {
            sub_bb4360(ppfxCtx,
                       *reinterpret_cast<const int32_t*>(param + 0x250),
                       1,
                       (hueDeg * fPi) / f180);
        }
        const float satVal = *reinterpret_cast<const float*>(param + 0x0c8);
        if (satVal != f1) {
            sub_bb4390(ppfxCtx, *reinterpret_cast<const int32_t*>(param + 0x250), satVal);
        }
        const float valScale = *reinterpret_cast<const float*>(param + 0x0cc);
        if (valScale != f1) {
            sub_bb43b0(ppfxCtx, *reinterpret_cast<const int32_t*>(param + 0x250), valScale);
        }
        const float contrastVal = *reinterpret_cast<const float*>(param + 0x248);
        if (contrastVal != f1) {
            sub_bb43e0(ppfxCtx, *reinterpret_cast<const int32_t*>(param + 0x250), contrastVal);
        }

        // 0x21d72a1..0x21d72c6: Vignette
        float vigPower = *reinterpret_cast<const float*>(param + 0x0d0);
        const float vigFovDep = *reinterpret_cast<const float*>(param + 0x0d4);
        if (0.0f > vigPower) {
            vigPower = fNegMax;
        }
        sub_bb3670(ppfxCtx, vigPower, vigFovDep);

        // 0x21d72cb..0x21d75bc: Glare / Bloom & Anamorphic Lens Flare (V5.1 Native Bloom Toggle)
        const bool bloomEnabled = gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectBloom);
        sub_bb2840(ppfxCtx,
                   bloomEnabled ? 1 : 0,
                   bloomEnabled ? *reinterpret_cast<const float*>(param + 0x0dc) : 0.0f,
                   *reinterpret_cast<const float*>(param + 0x0e0),
                   *reinterpret_cast<const float*>(param + 0x244));

        const int32_t glareShape = bloomEnabled
                                       ? *reinterpret_cast<const int32_t*>(param + 0x0d8)
                                       : -1;
        if (glareShape == 14) {
            alignas(32) uint8_t customGlare[0x48] = {};
            *reinterpret_cast<float*>(customGlare + 0x00)    = *reinterpret_cast<const float*>(param + 0x308);
            *reinterpret_cast<float*>(customGlare + 0x04)    = *reinterpret_cast<const float*>(param + 0x30c);
            *reinterpret_cast<float*>(customGlare + 0x08)    = *reinterpret_cast<const float*>(param + 0x310);
            *reinterpret_cast<uint32_t*>(customGlare + 0x0c) = *reinterpret_cast<const uint32_t*>(param + 0x314);
            *reinterpret_cast<float*>(customGlare + 0x10)    = *reinterpret_cast<const float*>(param + 0x318);
            *reinterpret_cast<float*>(customGlare + 0x14)    = *reinterpret_cast<const float*>(param + 0x31c);
            *reinterpret_cast<float*>(customGlare + 0x18)    = *reinterpret_cast<const float*>(param + 0x320);
            customGlare[0x1c] = (*reinterpret_cast<const int32_t*>(param + 0x324) != 0) ? 1u : 0u;
            *reinterpret_cast<float*>(customGlare + 0x20)    = *reinterpret_cast<const float*>(param + 0x328);
            *reinterpret_cast<uint32_t*>(customGlare + 0x24) = *reinterpret_cast<const uint32_t*>(param + 0x32c);
            *reinterpret_cast<float*>(customGlare + 0x28)    = *reinterpret_cast<const float*>(param + 0x330);
            *reinterpret_cast<float*>(customGlare + 0x2c)    = *reinterpret_cast<const float*>(param + 0x334);
            customGlare[0x30] = (*reinterpret_cast<const int32_t*>(param + 0x338) != 0) ? 1u : 0u;
            *reinterpret_cast<float*>(customGlare + 0x34)    = *reinterpret_cast<const float*>(param + 0x33c) * fDeg2Rad;
            *reinterpret_cast<float*>(customGlare + 0x38)    = *reinterpret_cast<const float*>(param + 0x340);
            customGlare[0x3c] = (*reinterpret_cast<const int32_t*>(param + 0x344) != 0) ? 1u : 0u;
            *reinterpret_cast<float*>(customGlare + 0x40)    = *reinterpret_cast<const float*>(param + 0x348);
            *reinterpret_cast<float*>(customGlare + 0x44)    = *reinterpret_cast<const float*>(param + 0x34c);
            sub_bb2860(ppfxCtx, customGlare);
        } else {
            sub_bb2850(ppfxCtx, glareShape);
        }

        const int32_t rawBloomLevels = *reinterpret_cast<const int32_t*>(param + 0x0e4);
        const int32_t bloomLevels = (rawBloomLevels <= 0 || rawBloomLevels > 4) ? 4 : rawBloomLevels;
        sub_bb40e0(ppfxCtx, bloomLevels);
        sub_bb3c10(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0e8));
        sub_bb4130(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0ec));
        sub_bb4100(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0f0));
        sub_bb40f0(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0f4));
        sub_bb4120(ppfxCtx, *reinterpret_cast<const float*>(param + 0x304));
        sub_bb4110(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0f8));
        sub_bb3c30(ppfxCtx, *reinterpret_cast<const float*>(param + 0x0fc));
        sub_bb41a0(ppfxCtx, *reinterpret_cast<const float*>(param + 0x100));
        sub_bb4160(ppfxCtx, (bloomEnabled && *reinterpret_cast<const int32_t*>(param + 0x104) != 0) ? 1u : 0u);
        sub_bb4180(ppfxCtx,
                   *reinterpret_cast<const float*>(param + 0x108),
                   *reinterpret_cast<const float*>(param + 0x10c));

        const auto* flareSrc = reinterpret_cast<const float*>(director + 0x26c0);
        const auto* ghostSrc = reinterpret_cast<const float*>(director + 0x26e0);
        const float flareAlpha = bloomEnabled ? flareSrc[3] : 0.0f;
        const float ghostAlpha = bloomEnabled ? ghostSrc[3] : 0.0f;
        const float flareColor[4] = {
            flareSrc[0] * flareAlpha, flareSrc[1] * flareAlpha, flareSrc[2] * flareAlpha, flareAlpha
        };
        const float ghostColor[4] = {
            ghostSrc[0] * ghostAlpha, ghostSrc[1] * ghostAlpha, ghostSrc[2] * ghostAlpha, ghostAlpha
        };
        sub_bb4190(ppfxCtx, flareColor, ghostColor);

        // 0x21d75bc..0x21d7a7f: Light Shafts & Sun Screen-Space Projection
        sub_bb28a0(ppfxCtx, 0);
        sub_bb28d0(ppfxCtx, 0, (bloomEnabled && *reinterpret_cast<const int32_t*>(param + 0x130) != 0) ? 1u : 0u);

        alignas(16) float viewProjMtx[16];
        sub_21aaa90(viewProjMtx, sceneView);

        const float pitchRad = *reinterpret_cast<const float*>(globalParam + 0x160) * fDeg2Rad;
        const float sinP = sub_2bbe518(0, 0, pitchRad);
        const float cosP = sub_2bbe518(1, 0, pitchRad);
        const __m128 vZero  = _mm_setzero_ps();
        const __m128 vSinP  = _mm_move_ss(vZero, _mm_set_ss(sinP));
        const __m128 vCosP  = _mm_move_ss(vZero, _mm_set_ss(cosP));
        const __m128 pVec   = _mm_mul_ps(_mm_shuffle_ps(vCosP, vSinP, 0xC3),
                                         _mm_setr_ps(1.0f, 1.0f, -1.0f, 1.0f)); // 0x4920de0: [0, cosP, -sinP, 0]

        const float yawRad = (*reinterpret_cast<const float*>(globalParam + 0x164) * fDeg2Rad) + fPi;
        const float sinY = sub_2bbe518(0, 0, yawRad);
        const float cosY = sub_2bbe518(1, 0, yawRad);
        const __m128 vSinY = _mm_move_ss(vZero, _mm_set_ss(sinY));
        const __m128 vCosY = _mm_move_ss(vZero, _mm_set_ss(cosY));
        const __m128 yRotZ = _mm_shuffle_ps(vSinY, vCosY, 0xCC); // [sinY, 0, cosY, 0]
        const __m128 yRotX = _mm_castsi128_ps(
            _mm_shuffle_epi32(_mm_castps_si128(_mm_mul_ps(yRotZ, _mm_setr_ps(-1.0f, 1.0f, 1.0f, 1.0f))), 0xC6));
        const __m128 unitY = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
        const __m128 unitW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

        const __m128 pX = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pVec), 0x00));
        const __m128 pZ = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pVec), 0xAA));
        const __m128 pY = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pVec), 0x55));
        const __m128 pW = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pVec), 0xFF));
        const __m128 sunDir3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(pX, yRotX), _mm_mul_ps(pY, yRotZ)),
                                          _mm_add_ps(_mm_mul_ps(pZ, unitY), _mm_mul_ps(pW, unitW)));

        const float negDist = -(*reinterpret_cast<const float*>(param + 0x24c));
        const __m128 sunPos4 = _mm_add_ps(_mm_mul_ps(sunDir3, _mm_set1_ps(negDist)), unitW);
        _mm_storeu_ps(reinterpret_cast<float*>(director + 0x21b0), sunPos4);

        const __m128 vp0 = _mm_load_ps(viewProjMtx + 0);
        const __m128 vp1 = _mm_load_ps(viewProjMtx + 4);
        const __m128 vp2 = _mm_load_ps(viewProjMtx + 8);
        const __m128 vp3 = _mm_load_ps(viewProjMtx + 12);
        const __m128 clipSun = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vp0, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(sunPos4), 0x00))),
                       _mm_mul_ps(vp1, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(sunPos4), 0x55)))),
            _mm_add_ps(_mm_mul_ps(vp2, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(sunPos4), 0xAA))),
                       _mm_mul_ps(vp3, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(sunPos4), 0xFF)))));
        const __m128 invClipW = _mm_div_ps(_mm_set1_ps(1.0f),
                                           _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(clipSun), 0xFF)));
        const __m128 projSun = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(clipSun, invClipW), _mm_setr_ps(0.5f, -0.5f, 1.0f, 0.0f)), // 0x4920e00
            _mm_setr_ps(0.5f, 0.5f, 0.0f, 0.0f));                                            // 0x4920e10

        // Exact 2-iteration Newton-Raphson vector3 normalize + dot(curRow2.xyz, normSun.xyz) + 0.3f
        alignas(16) float sunArr[4];
        alignas(16) float row2Arr[4];
        alignas(16) float projArr[4];
        _mm_store_ps(sunArr, sunPos4);
        _mm_store_ps(row2Arr, curRow2);
        _mm_store_ps(projArr, projSun);

        const float lenSq = sunArr[0] * sunArr[0] + sunArr[1] * sunArr[1] + sunArr[2] * sunArr[2];
        float fadeFactor = 0.0f;
        if (lenSq > 0.0f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            const float dotVal = (sunArr[0] * invLen) * row2Arr[0] +
                                 (sunArr[1] * invLen) * row2Arr[1] +
                                 (sunArr[2] * invLen) * row2Arr[2] + f0_3;
            if (!(0.0f > dotVal)) {
                fadeFactor = (dotVal < f1) ? dotVal : f1;
            }
        }

        sub_bb2940(ppfxCtx, 0, projArr[0], projArr[1]);

        const auto* lsSrc = reinterpret_cast<const float*>(director + 0x2680);
        const float lsScale = lsSrc[3] * fadeFactor;
        const float lsColor[4] = {
            lsSrc[0] * lsScale, lsSrc[1] * lsScale, lsSrc[2] * lsScale, 0.0f
        };
        float lsAtt = 0.0f;
        if (!(0.0f > projArr[2])) {
            lsAtt = (projArr[2] < f0_9995) ? projArr[2] : f0_9995;
        }
        sub_bb2900(ppfxCtx,
                   0,
                   lsColor,
                   *reinterpret_cast<const int32_t*>(param + 0x150),
                   *reinterpret_cast<const float*>(param + 0x144),
                   *reinterpret_cast<const float*>(param + 0x148),
                   lsAtt,
                   *reinterpret_cast<const float*>(param + 0x154),
                   *reinterpret_cast<const float*>(param + 0x158),
                   *reinterpret_cast<const float*>(param + 0x15c));

        const auto* ringSrc = reinterpret_cast<const float*>(director + 0x26a0);
        const float ringScale = ringSrc[3] * fadeFactor;
        const float ringColor[4] = {
            ringSrc[0] * ringScale, ringSrc[1] * ringScale, ringSrc[2] * ringScale, 0.0f
        };
        sub_bb2920(ppfxCtx,
                   0,
                   ringColor,
                   *reinterpret_cast<const float*>(param + 0x168),
                   *reinterpret_cast<const float*>(param + 0x16c),
                   *reinterpret_cast<const float*>(param + 0x170),
                   *reinterpret_cast<const float*>(param + 0x174));

        // 0x21d7a7f..0x21d7ca0: Depth of Field (V5.1 Native DoF Toggle + Ultrawide Aspect)
        const bool dofAllowed = gx_direct_vk::IsEffectEnabled(gx_direct_vk::kEffectDof) &&
                                (*reinterpret_cast<const int32_t*>(param + 0x1ac) != 0);
        sub_bb2960(ppfxCtx, (inDepthWrap != nullptr && dofAllowed) ? 1u : 0u);

        float focusDist = 0.0f;
        if (*reinterpret_cast<const int32_t*>(param + 0x188) != 0) {
            sub_bb3350(ppfxCtx, -1, 1);
            sub_bb34b0(ppfxCtx,
                       -1,
                       (*reinterpret_cast<const int32_t*>(param + 0x18c) != 0) ? 1u : 0u,
                       *reinterpret_cast<const float*>(param + 0x190),
                       *reinterpret_cast<const float*>(param + 0x194));
            sub_bb3490(ppfxCtx, -1, 1, *reinterpret_cast<const float*>(param + 0x198));
            sub_bb3470(ppfxCtx,
                       -1,
                       *reinterpret_cast<const float*>(param + 0x19c),
                       *reinterpret_cast<const float*>(param + 0x1a0));
            sub_bb3480(ppfxCtx,
                       -1,
                       *reinterpret_cast<const float*>(param + 0x1a4),
                       *reinterpret_cast<const float*>(param + 0x1a8));
            focusDist = sub_bb33e0(ppfxCtx, -1);
            *reinterpret_cast<float*>(globalParam + 0x1b0) = focusDist;
        } else {
            focusDist = *reinterpret_cast<const float*>(param + 0x1b0);
            sub_bb3340(ppfxCtx, -1, focusDist);
        }

        sub_bb3f80(ppfxCtx, *reinterpret_cast<const float*>(param + 0x1d8));
        sub_bb34a0(ppfxCtx,
                   *reinterpret_cast<const int32_t*>(param + 0x1cc),
                   *reinterpret_cast<const float*>(param + 0x1c4),
                   *reinterpret_cast<const float*>(param + 0x1c8) * fDeg2Rad);

        if (dofAllowed) {
            const float ccdDiagM = *reinterpret_cast<const float*>(param + 0x1c0) * f0_001;
            float aspect = *reinterpret_cast<const float*>(sceneView + 0x064);
            if (aspect == 0.0f) {
                const uint32_t vpW = *reinterpret_cast<const uint32_t*>(sceneView + 0x05c);
                const uint32_t vpH = *reinterpret_cast<const uint32_t*>(sceneView + 0x060);
                aspect = static_cast<float>(static_cast<int64_t>(vpW)) /
                         static_cast<float>(static_cast<int64_t>(vpH));
            }
            const float sensorHeightM = ccdDiagM / std::sqrt(aspect * aspect + f1);

            sub_bb3fd0(ppfxCtx,
                       *reinterpret_cast<const int32_t*>(param + 0x1d0),
                       *reinterpret_cast<const int32_t*>(param + 0x1d4));
            sub_bb2970(ppfxCtx,
                       0,
                       focusDist,
                       *reinterpret_cast<const float*>(param + 0x1b4),
                       *reinterpret_cast<const float*>(param + 0x1b8),
                       (*reinterpret_cast<const float*>(param + 0x1bc) * fPi) / f180,
                       sensorHeightM);

            if (*reinterpret_cast<const int32_t*>(param + 0x1ac) != 0) {
                if (sub_bb45c0(ppfxCtx, *reinterpret_cast<const int32_t*>(param + 0x240)) != 0) {
                    *reinterpret_cast<int32_t*>(globalParam + 0x240) = -1;
                }
            }
        }

        // 0x21d7ca0..0x21d7d23: 3D LUT Color Grading Binding
        sub_bb3600(ppfxCtx, (*reinterpret_cast<const int32_t*>(globalParam + 0x2d0) != 0) ? 1u : 0u);
        sub_bb3660(ppfxCtx, 0, 3, 0, 0.0f);
        auto* lutSurf = *reinterpret_cast<uint8_t**>(director + 0x20d0);
        if (lutSurf != nullptr) {
            const uint32_t lutW = *reinterpret_cast<const uint16_t*>(lutSurf + 0x044);
            const uint32_t lutH = *reinterpret_cast<const uint16_t*>(lutSurf + 0x046);
            auto* lutGnm = *reinterpret_cast<uint8_t**>(director + 0x20d8) + 0x010;
            const uint32_t lutFmt = sub_1089960(lutGnm);
            sub_bb5790(ppfxCtx, lutGnm, lutW, lutH, lutFmt);
        } else {
            sub_bb3600(ppfxCtx, 0);
        }

        // 0x21d7d23..0x21d7fb8: Motion Blur & Velocity Buffer
        sub_bb3870(ppfxCtx,
                   (velWrap != nullptr && *reinterpret_cast<const int32_t*>(param + 0x1dc) != 0) ? 1u : 0u);
        sub_bb38b0(ppfxCtx, param[0x204]);
        const int32_t mbType = ((resetFlags & 2u) != 0u)
                                   ? 1
                                   : *reinterpret_cast<const int32_t*>(param + 0x1e0);
        sub_bb3890(ppfxCtx, mbType);
        sub_bb38e0(ppfxCtx,
                   *reinterpret_cast<const float*>(param + 0x2ac),
                   *reinterpret_cast<const float*>(param + 0x2b0));

        alignas(16) float mtxArg[16];
        _mm_store_ps(mtxArg + 0,  curRow0);
        _mm_store_ps(mtxArg + 4,  curRow1);
        _mm_store_ps(mtxArg + 8,  curRow2);
        _mm_store_ps(mtxArg + 12, curRow3);
        sub_bb27c0(ppfxCtx, mtxArg, 1);

        _mm_store_ps(mtxArg + 0,  prevRow0);
        _mm_store_ps(mtxArg + 4,  prevRow1);
        _mm_store_ps(mtxArg + 8,  prevRow2);
        _mm_store_ps(mtxArg + 12, prevRow3);
        sub_bb27e0(ppfxCtx, mtxArg, 1);

        sub_bb2800(ppfxCtx, *reinterpret_cast<const float*>(param + 0x004));

        if (*reinterpret_cast<const int32_t*>(param + 0x1dc) != 0 &&
            *reinterpret_cast<const int32_t*>(param + 0x1e0) == 2 &&
            velWrap != nullptr)
        {
            auto* velGnm = *reinterpret_cast<uint8_t**>(velWrap + 0x028) + 0x010;
            const uint32_t velW = *reinterpret_cast<const uint32_t*>(velWrap + 0x050);
            const uint32_t velH = *reinterpret_cast<const uint32_t*>(velWrap + 0x054);
            const float rawVelScale = *reinterpret_cast<const float*>(param + 0x1e8) *
                                      *reinterpret_cast<const float*>(param + 0x350);
            const float scalePair[2] = {
                (rawVelScale > f0_0001) ? rawVelScale : f0_0001,
                (rawVelScale > f0_001)  ? rawVelScale : f0_001
            };
            const uint32_t velFmt = sub_1089960(velGnm);
            if (sub_bb5430(ppfxCtx, 1, velGnm, velW, velH, velFmt, 1, scalePair, 0) != 0) {
                *reinterpret_cast<int32_t*>(globalParam + 0x1e0) = 1;
            }
        }

        sub_bb38f0(ppfxCtx,
                   *reinterpret_cast<const int32_t*>(param + 0x1ec),
                   *reinterpret_cast<const int32_t*>(param + 0x1f0),
                   *reinterpret_cast<const float*>(param + 0x1e4),
                   *reinterpret_cast<const float*>(param + 0x1e8),
                   *reinterpret_cast<const float*>(param + 0x1f4));
        sub_bb3900(ppfxCtx,
                   (*reinterpret_cast<const int32_t*>(param + 0x200) != 0) ? 1u : 0u,
                   *reinterpret_cast<const float*>(param + 0x1f8),
                   *reinterpret_cast<const float*>(param + 0x1fc));

        // 0x21d7fb8..0x21d8093: Feedback Blur, Anti-Aliasing & PPFX Render Kick
        sub_bb34c0(ppfxCtx, (*reinterpret_cast<const int32_t*>(param + 0x208) != 0) ? 1u : 0u);
        if (*reinterpret_cast<const int32_t*>(globalParam + 0x21c) != 0) {
            const float fbW0 = *reinterpret_cast<const float*>(param + 0x20c);
            sub_bb34d0(ppfxCtx,
                       fbW0,
                       f1 - fbW0,
                       *reinterpret_cast<const float*>(param + 0x214));
        } else {
            sub_bb34d0(ppfxCtx,
                       0.0f,
                       f1,
                       *reinterpret_cast<const float*>(param + 0x214));
        }
        sub_bb34e0(ppfxCtx, *reinterpret_cast<const float*>(param + 0x218));
        sub_bb37f0(ppfxCtx, (*reinterpret_cast<const int32_t*>(param + 0x220) != 0) ? 1u : 0u);

        const float clearGrey[4] = {
            BitsToFloat(0x3ec0c0c1u),
            BitsToFloat(0x3ec0c0c1u),
            BitsToFloat(0x3ec0c0c1u),
            f1
        };
        sub_bb3c50(ppfxCtx, 0x10000000u, clearGrey, 0, -1, fNegMax);
    }

    // ========================================================================
    // Part C (0x21d8093..0x21d8810): Yebis Fullscreen Quad Draw & Double-Buf Sync
    // ========================================================================
    {
        uint32_t dsCtrl = 0;
        sub_10850d0(&dsCtrl);
        sub_1085130(&dsCtrl, 0);
        sub_1076b30(g_ActiveYebisGpuCtx_56c3cb8, dsCtrl);

        uint32_t primSetup = 0;
        sub_1084f40(&primSetup);
        sub_1084f50(&primSetup, 0);
        sub_1084f60(&primSetup, 0);
        sub_1084f70(&primSetup, 2, 2);
        sub_1076e00(g_ActiveYebisGpuCtx_56c3cb8, primSetup);

        sub_1072fd0(g_ActiveYebisGpuCtx_56c3cb8, 0xfu);
        auto* yGpu0 = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
        *reinterpret_cast<uint32_t*>(yGpu0 + 0x2f0) = 0u;
        sub_1076030(yGpu0, 0);

        uint32_t blendCtrl = 0;
        sub_1085380(&blendCtrl);
        sub_1085390(&blendCtrl, 0, 7);
        sub_10853f0(&blendCtrl, 0);
        sub_10733a0(g_ActiveYebisGpuCtx_56c3cb8, blendCtrl);

        auto* sceneRoot = static_cast<uint8_t*>(g_SceneRendererRoot_5540dd8);
        sub_1096f60(g_ActiveYebisGpuCtx_56c3cb8,
                    0,
                    0,
                    *reinterpret_cast<const uint32_t*>(sceneRoot + 0x1e0),
                    *reinterpret_cast<const uint32_t*>(sceneRoot + 0x1e4),
                    f1,
                    0.0f);
        sub_1077150(g_ActiveYebisGpuCtx_56c3cb8, 0, 0);

        auto* vsWrap  = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(resBank + 0xa28) + 0x010);
        auto* psWrap  = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(resBank + 0x9c0) + 0x010);
        auto* quadVb  = *reinterpret_cast<uint8_t**>(resBank + 0x9a0);
        auto* vtxDecl = *reinterpret_cast<uint8_t**>(resBank + 0x9a8);

        uint32_t elemFmtPos = 0;
        uint32_t elemFmtUv  = 0;
        sub_26b7310(&elemFmtPos, 2);
        sub_26b7310(&elemFmtUv, 0x10);

        const uint32_t vbByteSize = *reinterpret_cast<const uint32_t*>(quadVb + 0x02c);
        const uint32_t numVerts   = static_cast<uint32_t>((static_cast<uint64_t>(vbByteSize) * 0xaaaaaaabULL) >> 36);

        alignas(16) uint32_t gnmVbPos[4] = {};
        alignas(16) uint32_t gnmVbUv[4]  = {};
        sub_1070650(gnmVbPos, *reinterpret_cast<void**>(quadVb + 0x020), elemFmtPos, 0x18u, numVerts);
        sub_1070650(gnmVbUv,
                    *reinterpret_cast<uint8_t**>(quadVb + 0x020) + 0x010,
                    elemFmtUv,
                    0x18u,
                    numVerts);

        sub_108f330(static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x2c8, 2, 0, 1, gnmVbPos);
        sub_108f330(static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x2c8, 2, 1, 1, gnmVbUv);

        auto* yGpuVs = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
        auto* vsBin  = *reinterpret_cast<uint8_t**>(vsWrap + 0x100);
        if (vsBin != nullptr && *reinterpret_cast<uint8_t**>(yGpuVs + 0xe610) != vsBin) {
            sub_1091bc0(yGpuVs + 0x5508, vsBin + 0x028, vsBin[0x03]);
        }
        sub_108fbb0(yGpuVs + 0x2c8,
                    vsBin,
                    *reinterpret_cast<const uint32_t*>(vtxDecl + 0x040),
                    *reinterpret_cast<void**>(vtxDecl + 0x010),
                    yGpuVs + 0x5508);

        auto* yGpuPs = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
        auto* psBin  = *reinterpret_cast<uint8_t**>(psWrap + 0x100);
        if (psBin != nullptr && *reinterpret_cast<uint8_t**>(yGpuPs + 0xe618) != psBin) {
            sub_1091bc0(yGpuPs + 0x3ce0, psBin + 0x03c, psBin[0x03]);
        }
        sub_108fdd0(yGpuPs + 0x2c8, psBin, yGpuPs + 0x3ce0);

        *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x028) &= ~0x2ULL;

        auto* srcSrvObj = *reinterpret_cast<uint8_t**>(param + 0x370);
        alignas(16) uint64_t texDesc128[4];
        texDesc128[3] = *reinterpret_cast<const uint64_t*>(srcSrvObj + 0x028);
        texDesc128[2] = *reinterpret_cast<const uint64_t*>(srcSrvObj + 0x020);
        texDesc128[0] = *reinterpret_cast<const uint64_t*>(srcSrvObj + 0x010);
        texDesc128[1] = *reinterpret_cast<const uint64_t*>(srcSrvObj + 0x018);
        sub_108d3f0(static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x2c8, 1, 0, texDesc128);

        alignas(16) uint32_t gnmSamp[4] = {};
        sub_1086dd0(gnmSamp);
        sub_1086e10(gnmSamp, 1);
        gx_direct_vk::RegisterDirectNativeSampler(gnmSamp);
        sub_108dfe0(static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x2c8, 1, 0, gnmSamp);
        sub_108dfe0(static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8) + 0x2c8, 1, 2, gnmSamp);

        void* const quadIbAddr = *reinterpret_cast<void**>(director + 0x2650);
        if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
            auto* yDrawCtx = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
            sub_108ca70(yDrawCtx + 0x2c8, yDrawCtx, yDrawCtx + 0x080);
            gx_direct_vk::EmitDrawIndexAddr(
                yDrawCtx, reinterpret_cast<uint64_t>(quadIbAddr), 6u, 4u, 0u);
            sub_108ca80(yDrawCtx + 0x2c8, yDrawCtx, yDrawCtx + 0x080);
        } else {
            sub_1073fe0(g_ActiveYebisGpuCtx_56c3cb8, 4);
            sub_1075270(g_ActiveYebisGpuCtx_56c3cb8, 0);
            auto* yDrawCtx = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
            sub_108ca70(yDrawCtx + 0x2c8, yDrawCtx, yDrawCtx + 0x080);
            sub_1074360(yDrawCtx, 6u, quadIbAddr);
            sub_108ca80(yDrawCtx + 0x2c8, yDrawCtx, yDrawCtx + 0x080);
        }

        // Note: 0x21d84b8 loads cmdCtx + 0x08 (which is workerCtx->gnmCtx)
        auto* cmdGnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x008);
        if (sub_26b9a50(cmdGnm + 0xb478)) {
            sub_26b9a40(cmdGnm + 0xb478);
        }
        sub_26afa00(cmdGnm + 0x010);

        auto* curPpfx = *reinterpret_cast<uint8_t**>(director + 0x2670);
        if (curPpfx != nullptr &&
            *reinterpret_cast<const int32_t*>(param + 0x000) != 0 &&
            director[0x020] != 0)
        {
            director[0x020] = 0;
            auto* ppfxVt = *reinterpret_cast<DLAllocatorVtable**>(curPpfx);
            ppfxVt->finish_38(curPpfx, 0, 0);

            const uint8_t curFlip = g_YebisBufferFlipIndex_56a6d20;
            void* fenceAddr = g_YebisFenceGpuAddrBase_56c3cf8 + static_cast<size_t>(curFlip) * 8u;
            const uint64_t fenceVal = g_YebisFenceCounter_56c3ce8;

            sub_10778f0(g_ActiveYebisGfxc0_56c3cc0, 4, 0, fenceAddr, 2, fenceVal, 0, 0);
            sub_1077b30(g_ActiveYebisGfxc0_56c3cc0,
                        fenceAddr,
                        0xffffffffu,
                        3,
                        static_cast<uint32_t>(fenceVal));

            uint64_t cmdStart0 = g_ActiveYebisCmdBase0_56c3cd0;
            uint32_t cmdSize0  = static_cast<uint32_t>(
                *reinterpret_cast<const uint64_t*>(static_cast<const uint8_t*>(g_ActiveYebisGfxc0_56c3cc0) + 0x010)) -
                static_cast<uint32_t>(cmdStart0);
            uint64_t cmdStart1 = g_ActiveYebisCmdBase1_56c3cd8;
            uint32_t cmdSize1  = static_cast<uint32_t>(
                *reinterpret_cast<const uint64_t*>(static_cast<const uint8_t*>(g_ActiveYebisGfxc1_56c3cc8) + 0x010)) -
                static_cast<uint32_t>(cmdStart1);
            sub_1089a10(1, &cmdStart0, &cmdSize0, &cmdStart1, &cmdSize1);

            ++g_YebisFenceCounter_56c3ce8;
            uint8_t nextFlip = static_cast<uint8_t>(g_YebisBufferFlipIndex_56a6d20 + 1u);
            if (nextFlip > 1u) {
                nextFlip = 0u;
            }
            g_YebisBufferFlipIndex_56a6d20 = nextFlip;

            uint8_t* nextYebisCtx = g_YebisDoubleBufArea_56a6d20 + 0x008 +
                                    static_cast<size_t>(nextFlip) * 0xe7b8u;
            g_ActiveYebisGpuCtx_56c3cb8 = nextYebisCtx;
            sub_10946d0(nextYebisCtx);

            auto* activeYebis = static_cast<uint8_t*>(g_ActiveYebisGpuCtx_56c3cb8);
            g_ActiveYebisGfxc0_56c3cc0  = activeYebis;
            g_ActiveYebisGfxc1_56c3cc8  = activeYebis + 0x080;
            g_ActiveYebisCmdBase0_56c3cd0 = *reinterpret_cast<const uint64_t*>(activeYebis + 0x000);
            g_ActiveYebisCmdBase1_56c3cd8 = *reinterpret_cast<const uint64_t*>(activeYebis + 0x080);

            curPpfx = *reinterpret_cast<uint8_t**>(director + 0x2670);
        }

        auto* relVelWrap = *reinterpret_cast<uint8_t**>(param + 0x388);
        if (relVelWrap != nullptr) {
            sub_bb5f10(curPpfx, *reinterpret_cast<uint8_t**>(relVelWrap + 0x028) + 0x010);
        }
        auto* relDepthWrap = *reinterpret_cast<uint8_t**>(param + 0x380);
        if (relDepthWrap != nullptr) {
            sub_bb5f10(curPpfx, *reinterpret_cast<uint8_t**>(relDepthWrap + 0x048) + 0x010);
        }
        yebisSceneGuard.Leave();

        auto* innerDevCtx = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        auto* gnm         = *reinterpret_cast<uint8_t**>(innerDevCtx + 0x008);
        alignas(32) uint8_t zeroClearDesc[0x48] = {};
        sub_26d3070(gnm + 0xb558, gnm, zeroClearDesc);
        sub_26b9850(gnm + 0xb478, gnm + 0x010);
        sub_26cd190(gnm + 0x0050, gnm);
        sub_26af700(gnm + 0x0010);
        sub_26bba10(gnm + 0xb518);
        sub_26d3300(gnm + 0xb528);
        sub_26d2d20(gnm + 0xb558);
        sub_2bbe688(gnm + 0xb730, 0, 0x23a8);

        innerDevCtx = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        sub_e47290(innerDevCtx + 0x1728, 1);
        *reinterpret_cast<void**>(innerDevCtx + 0x1ec0) = sub_e47840(innerDevCtx + 0x1728);
        innerDevCtx[0x1670] &= 0xfeu;
        sub_219fcf0(*reinterpret_cast<uint8_t**>(cmdCtx + 0x000));

        innerDevCtx = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        *reinterpret_cast<void**>(innerDevCtx + 0x0788)    = nullptr;
        *reinterpret_cast<uint32_t*>(innerDevCtx + 0x0960) = 0u;
        const uint32_t prevEnd = *reinterpret_cast<const uint32_t*>(innerDevCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(innerDevCtx + 0x0964) = (prevEnd > 1u) ? prevEnd : 1u;
        gnm = *reinterpret_cast<uint8_t**>(innerDevCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd128) = nullptr;
    }

    // ========================================================================
    // Part D1 (0x21d8810..0x21d8d65): Optional LUT Overlay Quad / RT Rebind
    // ========================================================================
    if (*reinterpret_cast<void**>(director + 0x030) != nullptr &&
        *reinterpret_cast<const int32_t*>(param + 0x2b8) != 0)
    {
        auto* outRtWrap = *reinterpret_cast<uint8_t**>(param + 0x390);
        void* srcSurf   = *reinterpret_cast<void**>(outRtWrap + 0x008);
        void* dstSurf   = *reinterpret_cast<void**>(resBank + 0x6e0);
        auto* innerDev  = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        sub_2168660(*reinterpret_cast<void**>(innerDev + 0x008), cmdCtx, dstSurf, srcSurf);

        innerDev = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        sub_e46f40(innerDev + 0x1728, *reinterpret_cast<void**>(param + 0x390), 0, 1);
        *reinterpret_cast<void**>(innerDev + 0x1ec0) = sub_e47840(innerDev + 0x1728);
        sub_21a1c50(innerDev);
        sub_219fcf0(*reinterpret_cast<uint8_t**>(cmdCtx + 0x000));

        auto* devSingleton = static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8);
        auto* statePool    = *reinterpret_cast<uint8_t**>(devSingleton + 0x250);

        void* rsObj = *reinterpret_cast<void**>(statePool + 0x1f8);
        *reinterpret_cast<void**>(devCtx + 0x0258) = rsObj;
        auto* gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<void**>(gnm + 0xb970) = rsObj;

        void* bsObj = *reinterpret_cast<void**>(statePool + 0x000);
        *reinterpret_cast<void**>(devCtx + 0x0480)    = bsObj;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0498) = 0xffffffffu;
        *reinterpret_cast<void**>(gnm + 0xbc00)       = bsObj;
        *reinterpret_cast<uint64_t*>(gnm + 0xbca0)    = 0;
        *reinterpret_cast<uint64_t*>(gnm + 0xbc98)    = 0;
        *reinterpret_cast<uint32_t*>(gnm + 0xbcb0)    = 0xffffffffu;

        void* dssObj = *reinterpret_cast<void**>(statePool + 0x1c8);
        *reinterpret_cast<void**>(devCtx + 0x0210) = dssObj;
        devCtx[0x0218] = 0xffu;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<void**>(gnm + 0xbc08) = dssObj;
        gnm[0xbcb4] = 0xffu;

        auto* srvSlot1 = *reinterpret_cast<uint8_t**>(resBank + 0x958);
        *reinterpret_cast<void**>(devCtx + 0x0970) = srvSlot1;
        *reinterpret_cast<uint32_t*>(devCtx + 0x09d8) =
            (*reinterpret_cast<const uint32_t*>(devCtx + 0x09d8) != 0u) ? 1u : 0u;
        const uint32_t prevEnd9dc = *reinterpret_cast<const uint32_t*>(devCtx + 0x09dc);
        *reinterpret_cast<uint32_t*>(devCtx + 0x09dc) = (prevEnd9dc > 2u) ? prevEnd9dc : 2u;
        gnm[0x197a] |= 0x02u;
        if (srvSlot1 != nullptr) {
            constexpr uint32_t kSrvMask1 = 0x100002u;
            auto* pState = reinterpret_cast<uint32_t*>(srvSlot1 + 0x44);
            if ((*pState & kSrvMask1) != kSrvMask1) {
                __atomic_fetch_or(pState, kSrvMask1, __ATOMIC_SEQ_CST);
            }
        }
        *reinterpret_cast<void**>(gnm + 0xd530) = srvSlot1;

        void* samp0 = *reinterpret_cast<void**>(
            *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8) + 0x250) + 0x2d0);
        *reinterpret_cast<void**>(devCtx + 0x09e0)    = samp0;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0a60) = 0u;
        const uint32_t prevSampEnd0 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0a64);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0a64) = (prevSampEnd0 <= 1u) ? 1u : prevSampEnd0;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1978) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd0a8) = samp0;

        void* samp3 = *reinterpret_cast<void**>(
            *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8) + 0x250) + 0x2d8);
        *reinterpret_cast<void**>(devCtx + 0x09f8) = samp3;
        const uint32_t prevSampStart = *reinterpret_cast<const uint32_t*>(devCtx + 0x0a60);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0a60) = (prevSampStart < 3u) ? prevSampStart : 3u;
        const uint32_t prevSampEnd3 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0a64);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0a64) = (prevSampEnd3 <= 4u) ? 4u : prevSampEnd3;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1978) |= 0x8ULL;
        *reinterpret_cast<void**>(gnm + 0xd0c0) = samp3;

        void* vsObj = *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(resBank + 0xa28) + 0x010);
        *reinterpret_cast<void**>(devCtx + 0x0780) = vsObj;
        *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc1b8) = vsObj;

        void* psObj = *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(resBank + 0x9c8) + 0x010);
        *reinterpret_cast<void**>(devCtx + 0x0a68) = psObj;
        *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd598) = psObj;

        *reinterpret_cast<uint64_t*>(devCtx + 0x1320) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc6b0) = 0;
        *reinterpret_cast<uint64_t*>(devCtx + 0x1038) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xcba8) = 0;

        void* psSrv0 = *reinterpret_cast<void**>(resBank + 0x6e8);
        *reinterpret_cast<void**>(devCtx + 0x0788)    = psSrv0;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = 0u;
        const uint32_t prevPsEnd0 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (prevPsEnd0 <= 1u) ? 1u : prevPsEnd0;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd128) = psSrv0;

        void* psSrv4 = *reinterpret_cast<void**>(director + 0x030);
        *reinterpret_cast<void**>(devCtx + 0x07a8) = psSrv4;
        const uint32_t prevPsStart4 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0960);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = (prevPsStart4 >= 4u) ? 3u : prevPsStart4;
        const uint32_t prevPsEnd4 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (prevPsEnd4 <= 5u) ? 5u : prevPsEnd4;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x10ULL;
        *reinterpret_cast<void**>(gnm + 0xd148) = *reinterpret_cast<void**>(director + 0x030);

        innerDev = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        void* vtxDecl = *reinterpret_cast<void**>(resBank + 0x9a8);
        *reinterpret_cast<void**>(innerDev + 0x0260) = vtxDecl;
        auto* innerGnm = *reinterpret_cast<uint8_t**>(innerDev + 0x008);
        *reinterpret_cast<void**>(innerGnm + 0xb730) = vtxDecl;

        void* quadVb = *reinterpret_cast<void**>(resBank + 0x9a0);
        *reinterpret_cast<void**>(innerDev + 0x0280)    = quadVb;
        *reinterpret_cast<uint32_t*>(innerDev + 0x0380) = 0x18u;
        *reinterpret_cast<uint32_t*>(innerDev + 0x0400) = 0u;
        innerGnm[0x25f0] |= 0x01u;
        *reinterpret_cast<void**>(innerGnm + 0xb750)    = *reinterpret_cast<void**>(resBank + 0x9a0);
        *reinterpret_cast<uint32_t*>(innerGnm + 0xb850) = 0x18u;
        *reinterpret_cast<uint32_t*>(innerGnm + 0xb8d0) = 0u;

        *reinterpret_cast<uint32_t*>(innerDev + 0x0268) = 5u;
        innerGnm = *reinterpret_cast<uint8_t**>(innerDev + 0x008);
        *reinterpret_cast<uint32_t*>(innerGnm + 0xb738) = 5u;

        sub_2bbe688(innerDev + 0x1670, 0, 0xb8);
        *reinterpret_cast<uint64_t*>(innerDev + 0x1680) = 0xeULL;
        *reinterpret_cast<uint64_t*>(innerDev + 0x1688) = 0x8ULL;
        sub_21696c0(innerGnm, innerGnm + 0xb730, 4, 0);

        *reinterpret_cast<void**>(devCtx + 0x0788)    = nullptr;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = 0u;
        const uint32_t unbindEnd0 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (unbindEnd0 > 1u) ? unbindEnd0 : 1u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd128) = nullptr;

        *reinterpret_cast<void**>(devCtx + 0x07a8) = nullptr;
        const uint32_t unbindStart4 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0960);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = (unbindStart4 < 4u) ? unbindStart4 : 4u;
        const uint32_t unbindEnd4 = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (unbindEnd4 > 5u) ? unbindEnd4 : 5u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x10ULL;
        *reinterpret_cast<void**>(gnm + 0xd148) = nullptr;
    } else {
        auto* innerDev = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        sub_e46f40(innerDev + 0x1728, *reinterpret_cast<void**>(param + 0x390), 0, 1);
        *reinterpret_cast<void**>(innerDev + 0x1ec0) = sub_e47840(innerDev + 0x1728);
        sub_21a1c50(innerDev);
    }

    // ========================================================================
    // Part D2 (0x21d8d65..0x21d9654): Optional 3D-to-2D LUT Debug Preview Pass
    // ========================================================================
    if (*reinterpret_cast<void**>(director + 0x048) != nullptr &&
        g_ViewportManagerSingleton_5540dc0 != nullptr &&
        director[0x21a8] != 0)
    {
        void* lut3dSrv = *reinterpret_cast<void**>(director + 0x030);
        if (lut3dSrv != nullptr) {
            *reinterpret_cast<uint32_t*>(director + 0x078) = 0x10u;
            *reinterpret_cast<uint32_t*>(director + 0x07c) = 0x100u;
            *reinterpret_cast<uint32_t*>(director + 0x080) = 0x100u;
            *reinterpret_cast<uint32_t*>(director + 0x084) = 0x10u;
        }

        auto* uav2d = *reinterpret_cast<uint8_t**>(director + 0x070);
        auto* gnm   = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        sub_2168a20(gnm, gnm + 0xb730, uav2d, 0, 0, director + 0x078, 0, 0);

        *reinterpret_cast<void**>(devCtx + 0x1508)    = uav2d;
        *reinterpret_cast<uint32_t*>(devCtx + 0x1578) = 0u;
        const uint32_t prevUavEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x157c);
        *reinterpret_cast<uint32_t*>(devCtx + 0x157c) = (prevUavEnd > 1u) ? prevUavEnd : 1u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x10000ULL;
        if (uav2d != nullptr) {
            constexpr uint32_t kUavMask0 = 0x200001u;
            auto* pState = reinterpret_cast<uint32_t*>(uav2d + 0x44);
            if ((*pState & kUavMask0) != kUavMask0) {
                __atomic_fetch_or(pState, kUavMask0, __ATOMIC_SEQ_CST);
            }
        }
        *reinterpret_cast<void**>(gnm + 0xda20) = uav2d;

        void* cb3dTo2d = *reinterpret_cast<void**>(director + 0x050);

        *reinterpret_cast<uint64_t*>(devCtx + 0x0780) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc1b8) = 0;
        *reinterpret_cast<uint64_t*>(devCtx + 0x0a68) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd598) = 0;
        *reinterpret_cast<uint64_t*>(devCtx + 0x0d50) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd0a0) = 0;
        *reinterpret_cast<uint64_t*>(devCtx + 0x1320) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc6b0) = 0;
        *reinterpret_cast<uint64_t*>(devCtx + 0x1038) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xcba8) = 0;

        void* linSampler = *reinterpret_cast<void**>(
            *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8) + 0x250) + 0x2d0);
        *reinterpret_cast<void**>(devCtx + 0x1580)    = linSampler;
        *reinterpret_cast<uint32_t*>(devCtx + 0x1600) = 0u;
        const uint32_t prevSampEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x1604);
        *reinterpret_cast<uint32_t*>(devCtx + 0x1604) = (prevSampEnd > 1u) ? prevSampEnd : 1u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd5a0) = linSampler;

        auto* csEntry = static_cast<uint8_t*>(
            sub_2274e20(resBank + 0x9a8, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b54ULL))));
        if (csEntry != nullptr) {
            void* csObj = *reinterpret_cast<void**>(csEntry + 0x010);
            *reinterpret_cast<void**>(devCtx + 0x1608) = csObj;
            *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xda90) = csObj;

            *reinterpret_cast<void**>(devCtx + 0x1610)    = cb3dTo2d;
            *reinterpret_cast<uint32_t*>(devCtx + 0x1650) = 0xffffffffu;
            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1000000000ULL;
            *reinterpret_cast<uint32_t*>(gnm + 0x1fd0)  = 0xffffffffu;
            *reinterpret_cast<void**>(gnm + 0xda98)     = cb3dTo2d;

            *reinterpret_cast<void**>(devCtx + 0x1338) = lut3dSrv;
            const uint32_t prevSrvStart = *reinterpret_cast<const uint32_t*>(devCtx + 0x1500);
            *reinterpret_cast<uint32_t*>(devCtx + 0x1500) = (prevSrvStart < 2u) ? prevSrvStart : 2u;
            const uint32_t prevSrvEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x1504);
            *reinterpret_cast<uint32_t*>(devCtx + 0x1504) = (prevSrvEnd > 3u) ? prevSrvEnd : 3u;
            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            *reinterpret_cast<uint64_t*>(gnm + 0x1fa8) |= 0x4ULL;
            *reinterpret_cast<void**>(gnm + 0xd630) = lut3dSrv;

            sub_2bbe688(devCtx + 0x1670, 0, 0xb8);
            *reinterpret_cast<uint64_t*>(devCtx + 0x1680) = 0xeULL;
            *reinterpret_cast<uint64_t*>(devCtx + 0x1688) = 0x8ULL;

            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            sub_26af700(gnm + 0x010);
            sub_26cbe50(gnm + 0x050, gnm, gnm + 0xb730);
            if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
                gx_direct_vk::EmitDispatchDirect(gnm + 0x010, 1u, 1u, 16u);
            } else {
                sub_1074c50(gnm + 0x010, 1u, 1u, 16u);
            }
            sub_26ccf20(gnm + 0x050, gnm, gnm + 0xb730);
            sub_26af700(gnm + 0x010);
        }

        *reinterpret_cast<void**>(devCtx + 0x1610)    = nullptr;
        *reinterpret_cast<uint32_t*>(devCtx + 0x1650) = 0xffffffffu;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1000000000ULL;
        *reinterpret_cast<uint32_t*>(gnm + 0x1fd0)  = 0xffffffffu;
        *reinterpret_cast<void**>(gnm + 0xda98)     = nullptr;

        *reinterpret_cast<void**>(devCtx + 0x1338) = nullptr;
        const uint32_t unbindSrvStart = *reinterpret_cast<const uint32_t*>(devCtx + 0x1500);
        *reinterpret_cast<uint32_t*>(devCtx + 0x1500) = (unbindSrvStart >= 2u) ? 2u : unbindSrvStart;
        const uint32_t unbindSrvEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x1504);
        *reinterpret_cast<uint32_t*>(devCtx + 0x1504) = (unbindSrvEnd <= 3u) ? 3u : unbindSrvEnd;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1fa8) |= 0x4ULL;
        *reinterpret_cast<void**>(gnm + 0xd630) = nullptr;

        *reinterpret_cast<void**>(devCtx + 0x1608) = nullptr;
        *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xda90) = nullptr;

        sub_219fcf0(devCtx);

        // Map dynamic preview quad VB (+0x58) and write 4 vertices (24 bytes each)
        auto* dynVb = *reinterpret_cast<uint8_t**>(director + 0x058);
        gnm         = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        float* mappedVtx = nullptr;
        if (sub_2165150(dynVb, 3, *reinterpret_cast<const uint32_t*>(gnm + 0xb510)) == 0 &&
            dynVb[0x38] == 1u)
        {
            void* outCpuPtr = nullptr;
            if (sub_26d0d30(gnm + 0xb6f8, gnm + 0x010, &outCpuPtr, dynVb + 0x018) == 0) {
                mappedVtx = static_cast<float*>(outCpuPtr);
            }
        }

        if (mappedVtx != nullptr) {
            auto* vpMgr = static_cast<const uint8_t*>(g_ViewportManagerSingleton_5540dc0);
            const float vpWf = static_cast<float>(static_cast<int64_t>(*reinterpret_cast<const uint32_t*>(vpMgr + 0x018)));
            const float vpHf = static_cast<float>(static_cast<int64_t>(*reinterpret_cast<const uint32_t*>(vpMgr + 0x01c)));
            const float x0 = ((0.0f / vpWf) * 2.0f) + fNeg1;
            const float y0 = ((fNeg0 / vpHf) * 2.0f) + f1;
            const float x1 = ((f16 / vpWf) * 2.0f) + fNeg1;
            const float y1 = ((fNeg256 / vpHf) * 2.0f) + f1;

            mappedVtx[0]  = x0; mappedVtx[1]  = y0; mappedVtx[2]  = f1;   mappedVtx[3]  = f1;
            mappedVtx[4]  = 0.0f; mappedVtx[5]  = 0.0f;
            mappedVtx[6]  = x1; mappedVtx[7]  = y0; mappedVtx[8]  = f1;   mappedVtx[9]  = f1;
            mappedVtx[10] = f1; mappedVtx[11] = 0.0f;
            mappedVtx[12] = x0; mappedVtx[13] = y1; mappedVtx[14] = f1;   mappedVtx[15] = f1;
            mappedVtx[16] = 0.0f; mappedVtx[17] = f1;
            mappedVtx[18] = x1; mappedVtx[19] = y1; mappedVtx[20] = f1;   mappedVtx[21] = f1;
            mappedVtx[22] = f1; mappedVtx[23] = f1;
        }

        auto* srvSlot1 = *reinterpret_cast<uint8_t**>(resBank + 0x958);
        *reinterpret_cast<void**>(devCtx + 0x0970) = srvSlot1;
        *reinterpret_cast<uint32_t*>(devCtx + 0x09d8) =
            (*reinterpret_cast<const uint32_t*>(devCtx + 0x09d8) != 0u) ? 1u : 0u;
        const uint32_t prevEnd9dc = *reinterpret_cast<const uint32_t*>(devCtx + 0x09dc);
        *reinterpret_cast<uint32_t*>(devCtx + 0x09dc) = (prevEnd9dc > 2u) ? prevEnd9dc : 2u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1978) |= 0x20000ULL;
        if (srvSlot1 != nullptr) {
            constexpr uint32_t kSrvMask1 = 0x100002u;
            auto* pState = reinterpret_cast<uint32_t*>(srvSlot1 + 0x44);
            if ((*pState & kSrvMask1) != kSrvMask1) {
                __atomic_fetch_or(pState, kSrvMask1, __ATOMIC_SEQ_CST);
            }
        }
        *reinterpret_cast<void**>(gnm + 0xd530) = srvSlot1;

        auto* statePool = *reinterpret_cast<uint8_t**>(
            static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8) + 0x250);
        void* rsObj = *reinterpret_cast<void**>(statePool + 0x1f8);
        *reinterpret_cast<void**>(devCtx + 0x0258) = rsObj;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<void**>(gnm + 0xb970) = rsObj;

        void* bsObj = *reinterpret_cast<void**>(statePool + 0x000);
        *reinterpret_cast<void**>(devCtx + 0x0480)    = bsObj;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0498) = 0xffffffffu;
        *reinterpret_cast<void**>(gnm + 0xbc00)       = bsObj;
        *reinterpret_cast<uint64_t*>(gnm + 0xbca0)    = 0;
        *reinterpret_cast<uint64_t*>(gnm + 0xbc98)    = 0;
        *reinterpret_cast<uint32_t*>(gnm + 0xbcb0)    = 0xffffffffu;

        void* dssObj = *reinterpret_cast<void**>(statePool + 0x1c8);
        *reinterpret_cast<void**>(devCtx + 0x0210) = dssObj;
        devCtx[0x0218] = 0xffu;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<void**>(gnm + 0xbc08) = dssObj;
        gnm[0xbcb4] = 0xffu;

        void* samp0 = *reinterpret_cast<void**>(
            *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8) + 0x250) + 0x2d0);
        *reinterpret_cast<void**>(devCtx + 0x09e0)    = samp0;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0a60) = 0u;
        const uint32_t prevPsSampEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x0a64);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0a64) = (prevPsSampEnd <= 1u) ? 1u : prevPsSampEnd;
        gnm[0x1978] |= 0x01u;
        *reinterpret_cast<void**>(gnm + 0xd0a8) = samp0;

        void* vsObj = *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(resBank + 0xa28) + 0x010);
        *reinterpret_cast<void**>(devCtx + 0x0780) = vsObj;
        *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc1b8) = vsObj;

        void* psObj = *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(resBank + 0x9c0) + 0x010);
        *reinterpret_cast<void**>(devCtx + 0x0a68) = psObj;
        *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd598) = psObj;

        *reinterpret_cast<uint64_t*>(devCtx + 0x1320) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc6b0) = 0;
        *reinterpret_cast<uint64_t*>(devCtx + 0x1038) = 0;
        *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xcba8) = 0;

        void* previewSrv0 = *reinterpret_cast<void**>(director + 0x048);
        *reinterpret_cast<void**>(devCtx + 0x0788)    = previewSrv0;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = 0u;
        const uint32_t prevPsEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (prevPsEnd > 1u) ? prevPsEnd : 1u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd128) = previewSrv0;

        void* vtxDecl = *reinterpret_cast<void**>(resBank + 0x9a8);
        *reinterpret_cast<void**>(devCtx + 0x0260) = vtxDecl;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<void**>(gnm + 0xb730) = vtxDecl;

        void* previewVb = *reinterpret_cast<void**>(director + 0x058);
        *reinterpret_cast<void**>(devCtx + 0x0280)    = previewVb;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0380) = 0x18u;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0400) = 0u;
        gnm[0x25f0] |= 0x01u;
        *reinterpret_cast<void**>(gnm + 0xb750)    = *reinterpret_cast<void**>(director + 0x058);
        *reinterpret_cast<uint32_t*>(gnm + 0xb850) = 0x18u;
        *reinterpret_cast<uint32_t*>(gnm + 0xb8d0) = 0u;

        *reinterpret_cast<uint32_t*>(devCtx + 0x0268) = 5u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint32_t*>(gnm + 0xb738) = 5u;

        sub_2bbe688(devCtx + 0x1670, 0, 0xb8);
        *reinterpret_cast<uint64_t*>(devCtx + 0x1680) = 0xeULL;
        *reinterpret_cast<uint64_t*>(devCtx + 0x1688) = 0x8ULL;
        sub_21696c0(gnm, gnm + 0xb730, 4, 0);
    }

    // ========================================================================
    // Part E (0x21d9654..0x21d9db8): CustomDof 2048-Entry CoC LUT Compute Pass
    // ========================================================================
    {
        const void* curSceneView = *reinterpret_cast<const void**>(cmdCtx + 0x210);
        auto* lutSurfObj         = *reinterpret_cast<const uint8_t**>(director + 0x20d0);
        *reinterpret_cast<uint32_t*>(director + 0x0c0) = *reinterpret_cast<const uint16_t*>(lutSurfObj + 0x044);
        *reinterpret_cast<uint32_t*>(director + 0x0c4) = *reinterpret_cast<const uint16_t*>(lutSurfObj + 0x046);
        *reinterpret_cast<uint32_t*>(director + 0x0c8) = 0u;

        alignas(32) uint8_t projParams[0x48] = {};
        sub_21aa290(projParams, curSceneView);

        auto clampPos = [](float v) noexcept -> float {
            return !(v <= 0.0f) ? v : 0.0f;
        };
        const float nearStart = clampPos(*reinterpret_cast<const float*>(globalParam + 0x2dc)); // 0xf0(%rsp)
        const float nearEnd   = clampPos(*reinterpret_cast<const float*>(globalParam + 0x2e0)); // 0x128(%rsp)
        const float farStart  = clampPos(*reinterpret_cast<const float*>(globalParam + 0x2ec)); // 0x110(%rsp)
        const float farEnd    = clampPos(*reinterpret_cast<const float*>(globalParam + 0x2f0));
        const float farCutoff = clampPos(*reinterpret_cast<const float*>(globalParam + 0x2fc)); // 0x130(%rsp)

        const float farSpan  = ((farEnd - farStart) > f1e_7) ? (farEnd - farStart) : f1e_7;     // 0xc0(%rsp)
        const float nearSpan = ((nearEnd - nearStart) > f1e_7) ? (nearEnd - nearStart) : f1e_7; // 0xe0(%rsp)

        auto* cocLutOut = reinterpret_cast<float*>(director + 0x0d0);
        const float c3_0     = sub_bb2dc0(savedPpfxCtx, 0, 3, 0.0f);
        const float c3_scale = sub_bb2dc0(savedPpfxCtx, 0, 3, 1.0f) - c3_0;
        const float c0_0     = sub_bb2dc0(savedPpfxCtx, 0, 0, 0.0f);
        const float c0_scale = sub_bb2dc0(savedPpfxCtx, 0, 0, 1.0f) - c0_0;
        const auto* ppfxBytes = static_cast<const uint8_t*>(savedPpfxCtx);
        const uint8_t* pView0 = (ppfxBytes != nullptr && *reinterpret_cast<const int32_t*>(ppfxBytes + 0x008) > 0)
            ? *reinterpret_cast<const uint8_t* const*>(ppfxBytes + 0x010)
            : nullptr;
        const float viewNearZ = pView0 ? *reinterpret_cast<const float*>(pView0 + 0x00c) : 0.1f;
        const float viewFarZ  = pView0 ? *reinterpret_cast<const float*>(pView0 + 0x010) : 1000.0f;
        const float negNearFar   = (-0.0f - viewNearZ) * viewFarZ;
        const float farMinusNear = viewFarZ - viewNearZ;

        const int32_t nearModeFlag = *reinterpret_cast<const int32_t*>(globalParam + 0x2d4);
        const int32_t farModeFlag  = *reinterpret_cast<const int32_t*>(globalParam + 0x2d8);
        const float   nearCocScale = *reinterpret_cast<const float*>(globalParam + 0x2e4);
        const float   negCocScale  = *reinterpret_cast<const float*>(globalParam + 0x2e8);
        const float   farCocScale  = *reinterpret_cast<const float*>(globalParam + 0x2f4);
        const float   posCocScale  = *reinterpret_cast<const float*>(globalParam + 0x2f8);

        for (int32_t i = 0; i < 0x800; ++i) {
            const float rawT = static_cast<float>(i) * fInv2047;
            float clampedT = 0.0f;
            if (!(0.0f > rawT)) {
                clampedT = (rawT < f1) ? rawT : f1;
            }
            const float normDepth  = (i > 0x7fd) ? f1 : clampedT;
            const float baseCoc    = normDepth * c3_scale + c3_0;
            const float d0         = normDepth * c0_scale + c0_0;
            const float linearDist = pView0
                ? (negNearFar / (d0 * farMinusNear - viewFarZ))
                : sub_bb2dc0(savedPpfxCtx, 0, 7, normDepth);

            float coc = 0.0f;
            if (nearModeFlag == 0) {
                coc = baseCoc;
                if (0.0f > coc) {
                    coc *= negCocScale;
                }
            } else {
                if (nearEnd > linearDist) {
                    const float tNear = (linearDist - nearStart) / nearSpan;
                    float cNear = 0.0f;
                    if (!(0.0f > tNear)) {
                        cNear = (tNear < f1) ? tNear : f1;
                    }
                    coc = -((f1 - cNear) * nearCocScale);
                    if (0.0f > coc) {
                        coc *= negCocScale;
                    }
                }
            }

            if (farModeFlag != 0 && linearDist > farStart) {
                const float tFar = (linearDist - farStart) / farSpan;
                float cFar = 0.0f;
                if (!(0.0f > tFar)) {
                    cFar = (tFar < f1) ? tFar : f1;
                }
                coc = cFar * farCocScale;
            }

            if (coc > 0.0f) {
                coc *= posCocScale;
            }

            const __m128 clipMask = _mm_cmplt_ss(_mm_set_ss(farCutoff), _mm_set_ss(linearDist));
            cocLutOut[i] = _mm_cvtss_f32(_mm_andnot_ps(clipMask, _mm_set_ss(coc)));
        }

        auto* dofUav = *reinterpret_cast<uint8_t**>(director + 0x0b8);
        void* dofCb  = *reinterpret_cast<void**>(director + 0x20e0);
        auto* gnm    = *reinterpret_cast<uint8_t**>(devCtx + 0x008);

        static uint8_t* s_lastDofUav = nullptr;
        static void*    s_lastDofCb  = nullptr;
        static float    s_lastCocLut[0x800] = {};
        static bool     s_dofLutInitialized = false;
        const bool dofLutUnchanged =
            s_dofLutInitialized &&
            (dofUav == s_lastDofUav) &&
            (dofCb == s_lastDofCb) &&
            (std::memcmp(s_lastCocLut, cocLutOut, sizeof(s_lastCocLut)) == 0);

        if (!dofLutUnchanged) {
            s_dofLutInitialized = true;
            s_lastDofUav = dofUav;
            s_lastDofCb  = dofCb;
            std::memcpy(s_lastCocLut, cocLutOut, sizeof(s_lastCocLut));

            sub_2168a20(gnm, gnm + 0xb730, dofUav, 0, 0, director + 0x0c0, 0, 0);

            *reinterpret_cast<void**>(devCtx + 0x1510) = dofUav;
            *reinterpret_cast<uint32_t*>(devCtx + 0x1578) =
                (*reinterpret_cast<const uint32_t*>(devCtx + 0x1578) != 0u) ? 1u : 0u;
            const uint32_t prevUavEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x157c);
            *reinterpret_cast<uint32_t*>(devCtx + 0x157c) = (prevUavEnd > 2u) ? prevUavEnd : 2u;
            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x20000ULL;
            if (dofUav != nullptr) {
                constexpr uint32_t kUavMask1 = 0x200002u;
                auto* pState = reinterpret_cast<uint32_t*>(dofUav + 0x44);
                if ((*pState & kUavMask1) != kUavMask1) {
                    __atomic_fetch_or(pState, kUavMask1, __ATOMIC_SEQ_CST);
                }
            }
            *reinterpret_cast<void**>(gnm + 0xda28) = dofUav;

            *reinterpret_cast<uint64_t*>(devCtx + 0x0780) = 0;
            *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc1b8) = 0;
            *reinterpret_cast<uint64_t*>(devCtx + 0x0a68) = 0;
            *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd598) = 0;
            *reinterpret_cast<uint64_t*>(devCtx + 0x0d50) = 0;
            *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xd0a0) = 0;
            *reinterpret_cast<uint64_t*>(devCtx + 0x1320) = 0;
            *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xc6b0) = 0;
            *reinterpret_cast<uint64_t*>(devCtx + 0x1038) = 0;
            *reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xcba8) = 0;

            *reinterpret_cast<void**>(devCtx + 0x1610)    = dofCb;
            *reinterpret_cast<uint32_t*>(devCtx + 0x1650) = 0u;
            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1000000000ULL;
            *reinterpret_cast<uint32_t*>(gnm + 0x1fd0)  = 0u;
            *reinterpret_cast<void**>(gnm + 0xda98)     = dofCb;

            void* linSampler = *reinterpret_cast<void**>(
                *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(g_RenderDeviceSingleton_55406c8) + 0x250) + 0x2d0);
            *reinterpret_cast<void**>(devCtx + 0x1580)    = linSampler;
            *reinterpret_cast<uint32_t*>(devCtx + 0x1600) = 0u;
            const uint32_t prevSampEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x1604);
            *reinterpret_cast<uint32_t*>(devCtx + 0x1604) = (prevSampEnd > 1u) ? prevSampEnd : 1u;
            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1ULL;
            *reinterpret_cast<void**>(gnm + 0xd5a0) = linSampler;

            auto* curResBank = *reinterpret_cast<uint8_t**>(director + 0x010);
            auto* csEntry = static_cast<uint8_t*>(
                sub_2274e20(curResBank + 0x9a8, reinterpret_cast<const char16_t*>(GX_VMA(0x49c3b7cULL))));
            if (csEntry != nullptr) {
                void* csObj = *reinterpret_cast<void**>(csEntry + 0x010);
                *reinterpret_cast<void**>(devCtx + 0x1608) = csObj;
                *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xda90) = csObj;

                sub_2bbe688(devCtx + 0x1670, 0, 0xb8);
                *reinterpret_cast<uint64_t*>(devCtx + 0x1680) = 0xeULL;
                *reinterpret_cast<uint64_t*>(devCtx + 0x1688) = 0x8ULL;

                gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
                sub_26af700(gnm + 0x010);
                sub_26cbe50(gnm + 0x050, gnm, gnm + 0xb730);
                if (gx_direct_vk::EnabledV2Sub(gx_direct_vk::kV2Sub_YebisDrawDispatch)) {
                    gx_direct_vk::EmitDispatchDirect(gnm + 0x010, 1u, 1u, 1u);
                } else {
                    sub_1074c50(gnm + 0x010, 1u, 1u, 1u);
                }
                sub_26ccf20(gnm + 0x050, gnm, gnm + 0xb730);
                sub_26af700(gnm + 0x010);
            }

            *reinterpret_cast<void**>(devCtx + 0x1610)    = nullptr;
            *reinterpret_cast<uint32_t*>(devCtx + 0x1650) = 0u;
            gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
            *reinterpret_cast<uint64_t*>(gnm + 0x1fb8) |= 0x1000000000ULL;
            *reinterpret_cast<uint32_t*>(gnm + 0x1fd0)  = 0u;
            *reinterpret_cast<void**>(gnm + 0xda98)     = nullptr;

            *reinterpret_cast<void**>(devCtx + 0x1608) = nullptr;
            *reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(devCtx + 0x008) + 0xda90) = nullptr;

            sub_219fcf0(devCtx);
        }

        auto* cmdGnm = *reinterpret_cast<uint8_t**>(cmdCtx + 0x008);
        alignas(32) uint8_t zeroClearDesc[0x48] = {};
        sub_26d3070(cmdGnm + 0xb558, cmdGnm, zeroClearDesc);
        sub_26b9850(cmdGnm + 0xb478, cmdGnm + 0x010);
        sub_26cd190(cmdGnm + 0x0050, cmdGnm);
        sub_26af700(cmdGnm + 0x0010);
        sub_26bba10(cmdGnm + 0xb518);
        sub_26d3300(cmdGnm + 0xb528);
        sub_26d2d20(cmdGnm + 0xb558);
        sub_2bbe688(cmdGnm + 0xb730, 0, 0x23a8);

        auto* innerDev = *reinterpret_cast<uint8_t**>(cmdCtx + 0x000);
        sub_e47290(innerDev + 0x1728, 1);
        *reinterpret_cast<void**>(innerDev + 0x1ec0) = sub_e47840(innerDev + 0x1728);
        innerDev[0x1670] &= 0xfeu;

        *reinterpret_cast<void**>(devCtx + 0x0788)    = nullptr;
        *reinterpret_cast<uint32_t*>(devCtx + 0x0960) = 0u;
        const uint32_t prevPsEnd = *reinterpret_cast<const uint32_t*>(devCtx + 0x0964);
        *reinterpret_cast<uint32_t*>(devCtx + 0x0964) = (prevPsEnd > 1u) ? prevPsEnd : 1u;
        gnm = *reinterpret_cast<uint8_t**>(devCtx + 0x008);
        *reinterpret_cast<uint64_t*>(gnm + 0x1968) |= 0x1ULL;
        *reinterpret_cast<void**>(gnm + 0xd128) = nullptr;

        sub_219fcf0(devCtx);
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
