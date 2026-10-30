# ============================================================================
# Bloodborne (CUSA03173 v1.09) — PC Native Active Workspace (Vulkan 1.3 Port)
# File: 01_PC_NATIVE_ACTIVE_WORKSPACE/CMakeLists.txt
#
# Supports:
#   1. Native Linux x86-64 builds (`clang++`)
#   2. Cross-compilation to Windows x86-64 (`x86_64-w64-mingw32-clang++`
#      with `-mno-red-zone` and `__attribute__((sysv_abi))` for Orbis ABI)
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(BloodborneGXEngineNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------------------------------------------------------
# Architecture & Calling Convention Flags (PS4 AMD Jaguar / Orbis System V ABI)
# ----------------------------------------------------------------------------
set(BB_GX_COMMON_FLAGS
    -O3
    -DNDEBUG
    -march=znver4
    -fno-omit-frame-pointer
    -fshort-wchar
    -ffunction-sections
    -fdata-sections
    -fno-strict-aliasing
    -fno-stack-protector
    -ffp-contract=off
    -mno-red-zone
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/gx_runtime_vma_and_tls.h
)

if(WIN32 OR MINGW)
    # On Windows x86_64 (MinGW-w64 Clang), enforce System V AMD64 ABI and
    # disable the 128-byte red-zone so decompiled functions interoperate
    # seamlessly with Orbis/PS4 register conventions (rdi, rsi, rdx, rcx, r8, r9).
    list(APPEND BB_GX_COMMON_FLAGS
        -DGX_TARGET_WINDOWS_MINGW=1
        "-DGX_SYSV_ABI=__attribute__((sysv_abi))"
    )
else()
    list(APPEND BB_GX_COMMON_FLAGS
        -DGX_TARGET_LINUX_NATIVE=1
        -DGX_SYSV_ABI=
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -fno-semantic-interposition
        -ftls-model=initial-exec
    )
endif()

# ----------------------------------------------------------------------------
# All 48 Decompiled Source Files (Modules 01 .. 08, 101,852+ lines of C++20)
# ----------------------------------------------------------------------------
set(BB_GX_ENGINE_SOURCES
    # Module 01: CoreGraphics2_GNM (492 functions)
    src/01_CoreGraphics2_GNM/CGCommandStreamRecorderAndPlayerGnm.cpp
    src/01_CoreGraphics2_GNM/CGContextStateShadowGnm.cpp
    src/01_CoreGraphics2_GNM/CGFormatTranslatorAndBlitPassesGnm.cpp
    src/01_CoreGraphics2_GNM/CGGfxAndComputeContextGnm.cpp
    src/01_CoreGraphics2_GNM/CGGpuFreeListHeapAndViewFactoryGnm.cpp
    src/01_CoreGraphics2_GNM/CGRasterizerAndOutputMergerCacheGnm.cpp
    src/01_CoreGraphics2_GNM/CGResourceSyncAndBufferCopy_gnm.cpp
    src/01_CoreGraphics2_GNM/CGShaderFactoryAndSurfaceSyncGnm.cpp
    src/01_CoreGraphics2_GNM/CGShaderResourceBinderGnm.cpp
    src/01_CoreGraphics2_GNM/CGSwapChainAndQueryPoolGnm.cpp

    # Module 02: GX_GraphicsFramework (913 functions)
    src/02_GX_GraphicsFramework/GXDefaultShaderPackAndDebugPrimitiveRenderer.cpp
    src/02_GX_GraphicsFramework/GXDepthStencilAndHeapAllocatorBridge.cpp
    src/02_GX_GraphicsFramework/GXDeviceSingletonAndCameraFrustum.cpp
    src/02_GX_GraphicsFramework/GXGParamManagerAndInterpolation.cpp
    src/02_GX_GraphicsFramework/GXGnmCommandContextAndDeviceFactory.cpp
    src/02_GX_GraphicsFramework/GXGnmTextureAndShaderStageInitializer.cpp
    src/02_GX_GraphicsFramework/GXNamedResourceTreeAndShaderObjects.cpp
    src/02_GX_GraphicsFramework/GXPerfDebugMenuAndSurfaceViews.cpp
    src/02_GX_GraphicsFramework/GXRenderJobQueueAndFlipPacer.cpp
    src/02_GX_GraphicsFramework/GXRenderTargetAndContextStateFlusher.cpp
    src/02_GX_GraphicsFramework/GXResourceMemoryTrackerAndCameraMath.cpp
    src/02_GX_GraphicsFramework/GXStatePresetCacheAndDebugProps.cpp

    # Module 03: TPF_GNF_ShaderContainers (177 functions)
    src/03_TPF_GNF_ShaderContainers/GXMtdMaterialAndParameterAccessor.cpp
    src/03_TPF_GNF_ShaderContainers/GXMtdTypeConverterAndTpfEntryStrategies.cpp
    src/03_TPF_GNF_ShaderContainers/GXTpfGnfTextureContainerLoader.cpp

    # Module 04: SceneRenderer (656 functions)
    src/04_SceneRenderer/GXBoundingVolumeCullingAndPathStringUtils.cpp
    src/04_SceneRenderer/GXDecalManagerAndPaintPass.cpp
    src/04_SceneRenderer/GXFlverMaterialShaderBinderAndModelContext.cpp
    src/04_SceneRenderer/GXFlverParserAndSkinningMatricesUpdater.cpp
    src/04_SceneRenderer/GXRenderThreadAndWorkerTaskPool.cpp
    src/04_SceneRenderer/GXShaderPackageLoaderAndParameterInspectors.cpp
    src/04_SceneRenderer/GXSkinningBufferUploaderAndFlverModelInstance.cpp
    src/04_SceneRenderer/GXYebisBloomAndAntiAliasBanks.cpp
    src/04_SceneRenderer/GXYebisDirectorLifecycleAndSkinningPaletteSubmitter.cpp
    src/04_SceneRenderer/GXYebisDofAndColorGradingBanks.cpp
    src/04_SceneRenderer/GXYebisMotionBlurAndLightShaftBanks.cpp
    src/04_SceneRenderer/GXYebisPostProcessDirectorAndLutCompute.cpp
    src/04_SceneRenderer/GXYebisToneMapAndSettingParamBanks.cpp

    # Module 05: Yebis2_PostFX (433 functions)
    src/05_Yebis2_PostFX/GXYebisPostEffectContextSettersAndInit.cpp
    src/05_Yebis2_PostFX/GXYebisPostEffectRenderAndParamApi.cpp

    # Module 06: DeferredLightingAndAO (125 functions)
    src/06_DeferredLightingAndAO/GXDeferredLightVolumeAndShadowCuller.cpp
    src/06_DeferredLightingAndAO/GXDeferredLightingAndAmbientOcclusionPass.cpp
    src/06_DeferredLightingAndAO/GXFeedBackBlurRenderPass.cpp

    # Module 07: SFX_ParticleGpuRenderer (144 functions)
    src/07_SFX_ParticleGpuRenderer/GXFfxRibbonAndParticleVertexEmitter.cpp
    src/07_SFX_ParticleGpuRenderer/GXFfxSceneCtrlAndShaderBinder.cpp
    src/07_SFX_ParticleGpuRenderer/GXFfxTessellationAndParticleDrawPass.cpp

    # Module 08: GPU_Shaders (752 SPIR-V Shaders + 12 Decompiled GPU Kernels)
    src/08_GPU_Shaders/GXShaderDecompiledLibraryAndManifest.cpp
)

add_library(bb_gx_engine_native STATIC ${BB_GX_ENGINE_SOURCES})
target_include_directories(bb_gx_engine_native PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(bb_gx_engine_native PRIVATE ${BB_GX_COMMON_FLAGS})

# ----------------------------------------------------------------------------
# Cross-Platform Native Test Runner (`gx_native_workspace_runner`)
# ----------------------------------------------------------------------------
add_executable(gx_native_workspace_runner tests/gx_native_workspace_runner.cpp)
target_include_directories(gx_native_workspace_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(gx_native_workspace_runner PRIVATE ${BB_GX_COMMON_FLAGS})
target_link_libraries(gx_native_workspace_runner PRIVATE bb_gx_engine_native)

if(UNIX AND NOT APPLE)
    target_link_libraries(gx_native_workspace_runner PRIVATE m pthread dl)
    target_link_options(gx_native_workspace_runner PRIVATE -Wl,--gc-sections)
endif()

# ----------------------------------------------------------------------------
# Optional Live Combat Differential Test Runner (`gx_live_combat_test`, Linux)
# ----------------------------------------------------------------------------
set(BBHOST_ROOT "/home/kekw3/Documents/BloodBorne/AI_WORK_FOR_PORTING/bbhost" CACHE PATH "Path to bbhost")
if(UNIX AND EXISTS "${BBHOST_ROOT}/src/core/elf.cpp")
    add_executable(gx_live_combat_test
        tests/gx_live_combat_test.cpp
        "${BBHOST_ROOT}/tests/decomp/eboot_kit.cpp"
        "${BBHOST_ROOT}/src/core/elf.cpp"
        "${BBHOST_ROOT}/src/core/memory.cpp"
        "${BBHOST_ROOT}/src/core/sha256.cpp"
        "${BBHOST_ROOT}/src/core/tls_rewrite.cpp"
        "${BBHOST_ROOT}/src/core/image_file.cpp"
    )
    target_include_directories(gx_live_combat_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        "${BBHOST_ROOT}/src"
    )
    target_compile_definitions(gx_live_combat_test PRIVATE
        BBHOST_SOURCE_DIR="${BBHOST_ROOT}"
    )
    target_compile_options(gx_live_combat_test PRIVATE ${BB_GX_COMMON_FLAGS})
    target_link_libraries(gx_live_combat_test PRIVATE bb_gx_engine_native m pthread dl)
    target_link_options(gx_live_combat_test PRIVATE -Wl,--gc-sections)
endif()
