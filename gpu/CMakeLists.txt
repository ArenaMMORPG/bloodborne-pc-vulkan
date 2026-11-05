# bbport GPU library: shadPS4 video core + shader recompiler + Gnm/VideoOut
# drivers, adapted to the native_probe runtime through shims in shim/.
cmake_minimum_required(VERSION 3.24)
project(bbgpu LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
# Frame pointers keep the loader's thread dumps (watchdog, SIGUSR2 sampling) walkable.
add_compile_options(-fno-omit-frame-pointer)

# bbport: link-time optimization (BB_LTO, default on) and profile-guided optimization
# (BB_PGO: off, generate = instrumented build writing into BB_PGO_DIR, use = build with that
# profile). Covers bbgpu and the static libraries linked into it. No -march: the same build
# runs on the Steam Deck.
option(BB_LTO "Link-time optimization of bbgpu" ON)
set(BB_PGO "off" CACHE STRING "Profile-guided optimization: off, generate, use")
set(BB_PGO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pgo" CACHE PATH "Profile directory")
if (BB_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT bb_ipo_ok OUTPUT bb_ipo_error)
    if (bb_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(WARNING "LTO unavailable: ${bb_ipo_error}")
    endif()
endif()
if (BB_PGO STREQUAL "generate")
    # Atomic counters: the GPU, recording, preparation and copy threads run the same code.
    add_compile_options(-fprofile-generate=${BB_PGO_DIR} -fprofile-update=atomic)
    add_link_options(-fprofile-generate=${BB_PGO_DIR})
    add_compile_definitions(BB_PGO_GENERATE)
elseif (BB_PGO STREQUAL "use")
    # Partial training keeps unprofiled code optimized normally; counters from concurrent
    # threads can be slightly inconsistent (correction). Functions changed since the profile
    # (or built differently for it, bbgpu_init) are compiled without it.
    add_compile_options(-fprofile-use=${BB_PGO_DIR} -fprofile-partial-training
                        -fprofile-correction -Wno-missing-profile -Wno-coverage-mismatch)
endif()

find_package(Vulkan REQUIRED)
find_package(fmt REQUIRED)
find_package(Boost REQUIRED)
find_package(magic_enum QUIET)
if(NOT magic_enum_FOUND)
    # Header-only; packaged on few systems (the Windows cross build's MSYS2 has none).
    include(FetchContent)
    FetchContent_Declare(magic_enum GIT_REPOSITORY https://github.com/Neargye/magic_enum.git GIT_TAG v0.9.7)
    FetchContent_MakeAvailable(magic_enum)
endif()
find_package(tsl-robin-map REQUIRED)
find_package(VulkanMemoryAllocator CONFIG QUIET)
if (NOT TARGET GPUOpen::VulkanMemoryAllocator)
    # The header alone, without its CMake package (MSYS2: include/vma/vk_mem_alloc.h).
    find_path(VMA_INCLUDE_DIR vk_mem_alloc.h PATH_SUFFIXES vma REQUIRED)
    add_library(GPUOpen::VulkanMemoryAllocator INTERFACE IMPORTED)
    target_include_directories(GPUOpen::VulkanMemoryAllocator INTERFACE ${VMA_INCLUDE_DIR})
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(XXHASH REQUIRED IMPORTED_TARGET libxxhash)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SPIRV_TOOLS REQUIRED IMPORTED_TARGET SPIRV-Tools)
if (NOT WIN32)
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
endif()
find_package(miniz QUIET)
if(NOT miniz_FOUND)
    # Distributions without a miniz package (Debian, Ubuntu: #136): built from its release.
    include(FetchContent)
    FetchContent_Declare(miniz GIT_REPOSITORY https://github.com/richgel999/miniz.git GIT_TAG 3.1.2)
    FetchContent_MakeAvailable(miniz)
    if(NOT TARGET miniz::miniz)
        add_library(miniz::miniz ALIAS miniz)
    endif()
endif()
find_package(Zydis REQUIRED)
find_program(GLSLANG_COMPILER NAMES glslang glslangValidator REQUIRED)

set(SIRIT_USE_SYSTEM_SPIRV_HEADERS OFF CACHE BOOL "" FORCE)
if (WIN32)
    # Every target, the third-party ones too: what glibc and MSVC have and MinGW does not.
    include_directories(BEFORE platform/win32/include)
    enable_language(RC)
    # Bit-fields laid out as on Linux (GCC's way), not MSVC's: the GPU registers and the PS4's
    # structures are declared with them.
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-mno-ms-bitfields>)
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/platform/win32/include/bbport_win32.h>")
endif()
add_subdirectory(third_party/sirit EXCLUDE_FROM_ALL)

# bbport: FSR 3.1 temporal upscaling / frame generation, native Vulkan (MIT, FireBurn/FSR-Vulkan).
set(FFX_VK_PORTABLE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FFX_VK_PORTABLE_BUILD_PROBE OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/fsr-vulkan EXCLUDE_FROM_ALL)

# Host shaders -> headers: SPIR-V arrays from glslang, in bbgpu/shaders/.
set(SHADER_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)
set(SHADER_HEADERS "")
function(add_shader_header SOURCE OUT_DIR SHADER_NAME)
    set(OUT ${OUT_DIR}/${SHADER_NAME}.h)
    string(TOUPPER ${SHADER_NAME} SYMBOL)
    set(DEFINES ${ARGN})
    list(TRANSFORM DEFINES PREPEND "-D")
    add_custom_command(OUTPUT ${OUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
        COMMAND ${GLSLANG_COMPILER} --target-env vulkan1.3 -g0 -o ${OUT} --vn ${SYMBOL} ${DEFINES}
                -I${CMAKE_CURRENT_SOURCE_DIR}/third_party/amd-fsr1 ${SOURCE}
        COMMAND ${CMAKE_COMMAND} "-DHEADER_FILE=${OUT}" -P ${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/header.cmake
        DEPENDS ${SOURCE} VERBATIM)
    set(SHADER_HEADERS ${SHADER_HEADERS} ${OUT} PARENT_SCOPE)
endfunction()
set(OWN_SHADER_DIR ${SHADER_INCLUDE}/bbgpu/shaders)
function(add_own_shader FILENAME SHADER_NAME)
    add_shader_header(${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} ${OWN_SHADER_DIR} ${SHADER_NAME} ${ARGN})
    set(SHADER_HEADERS ${SHADER_HEADERS} PARENT_SCOPE)
endfunction()
add_own_shader(render/shaders/depth_from_color.frag depth_from_color_frag)
add_own_shader(render/shaders/ui_upscale.comp ui_upscale_comp)
add_own_shader(render/shaders/texel_copy.frag texel_copy_frag)
add_own_shader(render/shaders/texel_copy.frag texel_copy_msaa_frag MSAA_SOURCE=1)
add_own_shader(buffers/shaders/fault_scan.comp fault_scan_comp)
add_own_shader(render/shaders/fullscreen.vert fullscreen_vert)
add_own_shader(render/shaders/present_encode.frag present_encode_frag)
add_own_shader(upscale/shaders/camera_motion.comp camera_motion_comp)
add_own_shader(upscale/shaders/upscale_merge.comp upscale_merge_comp)
add_own_shader(upscale/shaders/upscale_reactive.comp upscale_reactive_comp)
add_own_shader(upscale/shaders/taa.comp taa_comp)
add_own_shader(upscale/shaders/taa_sharpen.comp taa_sharpen_comp)
add_own_shader(upscale/shaders/taa_sharpen.comp taa_sharpen_ldr_comp LDR_OUTPUT=1)
add_own_shader(upscale/shaders/fsr4_color.comp fsr4_decode_comp)
add_own_shader(upscale/shaders/fsr4_color.comp fsr4_encode_comp ENCODE=1)
add_own_shader(upscale/shaders/fsr4_reactive.comp fsr4_reactive_comp)
add_own_shader(render/shaders/buffer_multi_copy.comp buffer_multi_copy_comp)
add_own_shader(render/shaders/occlusion_resolve.comp occlusion_resolve_comp)
add_own_shader(render/shaders/gpu_timestamp.comp gpu_timestamp_comp)
add_own_shader(render/shaders/indirect_guard.comp indirect_guard_comp)
add_own_shader(render/shaders/indirect_draw_guard.comp indirect_draw_guard_comp)
add_own_shader(upscale/shaders/depth_resample.frag depth_resample_frag)
add_own_shader(upscale/shaders/depth_resample.frag depth_stencil_resample_frag HAS_STENCIL=1)
add_own_shader(upscale/shaders/depth_resample.frag depth_stencil_bits_frag STENCIL_BITS=1)
add_own_shader(upscale/shaders/camera_motion.comp camera_motion_debug_comp DEBUG_OVERLAY=1)
add_own_shader(shader/backend/spirv/native/ps_0f61fd27_vsm_blur.frag ps_0f61fd27_vsm_blur_frag)
add_own_shader(shader/backend/spirv/native/ps_1ae03d75_vsm_moment.frag ps_1ae03d75_vsm_moment_cldist_frag HAS_CLDIST=1)
add_own_shader(shader/backend/spirv/native/ps_1ae03d75_vsm_moment.frag ps_1ae03d75_vsm_moment_noclip_frag)
add_own_shader(shader/backend/spirv/native/ps_f7229091_bgra_blit.frag ps_f7229091_bgra_blit_frag)
add_own_shader(shader/backend/spirv/native/ps_ed4c7668_lut3d.frag ps_ed4c7668_lut3d_frag)
add_own_shader(shader/backend/spirv/native/vs_167daba6_vsm_shadow.vert vs_167daba6_vsm_shadow_vert)
add_own_shader(shader/backend/spirv/native/ps_8c08444c_blur13.frag ps_8c08444c_blur13_frag)
add_own_shader(shader/backend/spirv/native/ps_32e47788_atmos_fog.frag ps_32e47788_atmos_fog_frag)
add_own_shader(shader/backend/spirv/native/ps_5e224cb6_vfx_fog.frag ps_5e224cb6_vfx_fog_frag)
add_own_shader(shader/backend/spirv/native/ps_71c96598_vfx_fog.frag ps_71c96598_vfx_fog_frag)
add_own_shader(shader/backend/spirv/native/vs_9da6aacb_gbuf.vert vs_9da6aacb_gbuf_vert)
add_own_shader(shader/backend/spirv/native/vs_6a833138_gbuf_inst.vert vs_6a833138_gbuf_inst_vert)
add_own_shader(shader/backend/spirv/native/vs_7620368b_gbuf_uv2.vert vs_7620368b_gbuf_uv2_vert)
add_own_shader(shader/backend/spirv/native/vs_ecbef4c7_gbuf_inst_uv2.vert vs_ecbef4c7_gbuf_inst_uv2_vert)
add_own_shader(shader/backend/spirv/native/ps_bb10e440_gbuf.frag ps_bb10e440_gbuf_frag)
add_own_shader(shader/backend/spirv/native/ps_8aba0ecd_gbuf.frag ps_8aba0ecd_gbuf_frag)
add_own_shader(shader/backend/spirv/native/ps_1c3953c6_gbuf.frag ps_1c3953c6_gbuf_frag)
add_own_shader(shader/backend/spirv/native/ps_a22c7f71_pbr.frag ps_a22c7f71_pbr_frag)
add_own_shader(shader/backend/spirv/native/ps_d92266b9_pbr.frag ps_d92266b9_pbr_frag)
add_own_shader(shader/backend/spirv/native/ps_7cc2c044_pbr.frag ps_7cc2c044_pbr_frag)
add_own_shader(shader/backend/spirv/native/ps_e96447c3_pbr.frag ps_e96447c3_pbr_frag)
add_own_shader(shader/backend/spirv/native/ps_bae5454e_pbr.frag ps_bae5454e_pbr_frag)
add_own_shader(shader/backend/spirv/native/ps_0ef50047_pbr.frag ps_0ef50047_pbr_frag)
add_own_shader(shader/backend/spirv/native/ps_e0305cef_post.frag ps_e0305cef_post_frag)
add_own_shader(shader/backend/spirv/native/ps_2390c887_gbuf7.frag ps_2390c887_gbuf7_frag)
add_own_shader(shader/backend/spirv/native/cs_a954e79d_indirect_prep.comp cs_a954e79d_indirect_prep_comp)
add_own_shader(shader/backend/spirv/native/ps_e55e45eb.frag ps_e55e45eb_frag)
add_own_shader(shader/backend/spirv/native/ps_5c02eaf8.frag ps_5c02eaf8_frag)
add_own_shader(shader/backend/spirv/native/ps_11ffdfb5.frag ps_11ffdfb5_frag)
add_own_shader(shader/backend/spirv/native/ps_111fce32.frag ps_111fce32_frag)
add_own_shader(shader/backend/spirv/native/ps_cbbca5dc.frag ps_cbbca5dc_frag)
add_own_shader(shader/backend/spirv/native/ps_f55643de.frag ps_f55643de_frag)
add_own_shader(shader/backend/spirv/native/ps_38d65b32.frag ps_38d65b32_frag)
add_own_shader(shader/backend/spirv/native/ps_6b42fc89.frag ps_6b42fc89_frag)
add_own_shader(shader/backend/spirv/native/vs_637500e0.vert vs_637500e0_vert)
add_own_shader(shader/backend/spirv/native/vs_061fbbf6.vert vs_061fbbf6_vert)
add_own_shader(shader/backend/spirv/native/vs_1ea5a07e.vert vs_1ea5a07e_vert)
add_own_shader(shader/backend/spirv/native/ps_370174f7.frag ps_370174f7_frag)
add_own_shader(shader/backend/spirv/native/ps_d22cbfd1.frag ps_d22cbfd1_frag)
add_own_shader(shader/backend/spirv/native/ps_149432ff.frag ps_149432ff_frag)
add_own_shader(shader/backend/spirv/native/ps_5de04266.frag ps_5de04266_frag)
add_own_shader(shader/backend/spirv/native/ps_3fe9e964.frag ps_3fe9e964_frag)
add_own_shader(shader/backend/spirv/native/ps_2140da2b.frag ps_2140da2b_frag)
add_own_shader(shader/backend/spirv/native/ps_dda49e89.frag ps_dda49e89_frag)
add_own_shader(shader/backend/spirv/native/ps_02cd8f9b.frag ps_02cd8f9b_frag)
add_own_shader(shader/backend/spirv/native/ps_07d07cf3.frag ps_07d07cf3_frag)
add_own_shader(shader/backend/spirv/native/ps_0b2ace4c.frag ps_0b2ace4c_frag)
add_own_shader(shader/backend/spirv/native/ps_186d69d5.frag ps_186d69d5_frag)
add_own_shader(shader/backend/spirv/native/ps_201cb43d.frag ps_201cb43d_frag)
add_own_shader(shader/backend/spirv/native/ps_22b357f3.frag ps_22b357f3_frag)
add_own_shader(shader/backend/spirv/native/ps_3105494c.frag ps_3105494c_frag)
add_own_shader(shader/backend/spirv/native/ps_31b90cc5.frag ps_31b90cc5_frag)
add_own_shader(shader/backend/spirv/native/ps_41d3ce4b.frag ps_41d3ce4b_frag)
add_own_shader(shader/backend/spirv/native/ps_44a82ffb.frag ps_44a82ffb_frag)
add_own_shader(shader/backend/spirv/native/ps_505ebdbe.frag ps_505ebdbe_frag)
add_own_shader(shader/backend/spirv/native/ps_5d7150da.frag ps_5d7150da_frag)
add_own_shader(shader/backend/spirv/native/ps_5f76eb3d.frag ps_5f76eb3d_frag)
add_own_shader(shader/backend/spirv/native/ps_600f86b0.frag ps_600f86b0_frag)
add_own_shader(shader/backend/spirv/native/ps_799bfa32.frag ps_799bfa32_frag)
add_own_shader(shader/backend/spirv/native/ps_88ae6537.frag ps_88ae6537_frag)
add_own_shader(shader/backend/spirv/native/ps_896e7242.frag ps_896e7242_frag)
add_own_shader(shader/backend/spirv/native/ps_898dabb3.frag ps_898dabb3_frag)
add_own_shader(shader/backend/spirv/native/ps_89f9ee3e.frag ps_89f9ee3e_frag)
add_own_shader(shader/backend/spirv/native/ps_8d5387d9.frag ps_8d5387d9_frag)
add_own_shader(shader/backend/spirv/native/ps_8ea5be99.frag ps_8ea5be99_frag)
add_own_shader(shader/backend/spirv/native/ps_9f9ef267.frag ps_9f9ef267_frag)
add_own_shader(shader/backend/spirv/native/ps_b59b4c4e.frag ps_b59b4c4e_frag)
add_own_shader(shader/backend/spirv/native/ps_b5b52d63.frag ps_b5b52d63_frag)
add_own_shader(shader/backend/spirv/native/ps_c6edb19a.frag ps_c6edb19a_frag)
add_own_shader(shader/backend/spirv/native/ps_d729c469.frag ps_d729c469_frag)
add_own_shader(shader/backend/spirv/native/ps_d75fe2ff.frag ps_d75fe2ff_frag)
add_own_shader(shader/backend/spirv/native/ps_f2531727.frag ps_f2531727_frag)
add_own_shader(shader/backend/spirv/native/ps_f40da260.frag ps_f40da260_frag)
add_own_shader(shader/backend/spirv/native/ps_f81596cd.frag ps_f81596cd_frag)
add_own_shader(shader/backend/spirv/native/ps_006c079b.frag ps_006c079b_frag)
add_own_shader(shader/backend/spirv/native/ps_4fd8db00.frag ps_4fd8db00_frag)
add_own_shader(shader/backend/spirv/native/ps_5e4596f4.frag ps_5e4596f4_frag)
add_own_shader(shader/backend/spirv/native/ps_5e67d866.frag ps_5e67d866_frag)
add_own_shader(shader/backend/spirv/native/ps_70bb42af.frag ps_70bb42af_frag)
add_own_shader(shader/backend/spirv/native/ps_749e4f9e.frag ps_749e4f9e_frag)
add_own_shader(shader/backend/spirv/native/ps_765450df.frag ps_765450df_frag)
add_own_shader(shader/backend/spirv/native/ps_8650418f.frag ps_8650418f_frag)
add_own_shader(shader/backend/spirv/native/ps_9837d1c8.frag ps_9837d1c8_frag)
add_own_shader(shader/backend/spirv/native/ps_9e1cb278.frag ps_9e1cb278_frag)
add_own_shader(shader/backend/spirv/native/ps_a291c498.frag ps_a291c498_frag)
add_own_shader(shader/backend/spirv/native/ps_a89e01e9.frag ps_a89e01e9_frag)
add_own_shader(shader/backend/spirv/native/ps_ab4e5342.frag ps_ab4e5342_frag)
add_own_shader(shader/backend/spirv/native/ps_babda34d.frag ps_babda34d_frag)
add_own_shader(shader/backend/spirv/native/ps_bd1d09f0.frag ps_bd1d09f0_frag)
add_own_shader(shader/backend/spirv/native/ps_c575bc29.frag ps_c575bc29_frag)
add_own_shader(shader/backend/spirv/native/ps_ca7226ec.frag ps_ca7226ec_frag)
add_own_shader(shader/backend/spirv/native/ps_ccbf44a6.frag ps_ccbf44a6_frag)
add_own_shader(shader/backend/spirv/native/ps_dbeccd9c.frag ps_dbeccd9c_frag)
add_own_shader(shader/backend/spirv/native/ps_e21e283a.frag ps_e21e283a_frag)
add_own_shader(shader/backend/spirv/native/ps_e54449c7.frag ps_e54449c7_frag)
add_own_shader(shader/backend/spirv/native/ps_fbe7c4b6.frag ps_fbe7c4b6_frag)
add_own_shader(shader/backend/spirv/native/ps_6c2323c2.frag ps_6c2323c2_frag)
add_own_shader(shader/backend/spirv/native/ps_6320bb24.frag ps_6320bb24_frag)
add_own_shader(shader/backend/spirv/native/ps_e820ee9d.frag ps_e820ee9d_frag)
add_own_shader(shader/backend/spirv/native/ps_998b035e.frag ps_998b035e_frag)
add_own_shader(shader/backend/spirv/native/ps_a0e88f56.frag ps_a0e88f56_noclip_frag)
add_own_shader(shader/backend/spirv/native/ps_a0e88f56.frag ps_a0e88f56_cldist_frag HAS_CLDIST=1)
add_own_shader(shader/backend/spirv/native/ps_86e96401.frag ps_86e96401_frag)
add_own_shader(shader/backend/spirv/native/ps_4b766f62.frag ps_4b766f62_frag)
add_own_shader(shader/backend/spirv/native/ps_3c2b7974.frag ps_3c2b7974_frag)
add_own_shader(shader/backend/spirv/native/ps_474f0123.frag ps_474f0123_frag)
add_own_shader(shader/backend/spirv/native/ps_483f5dc1.frag ps_483f5dc1_frag)
foreach(BPP IN ITEMS 8 16 32 64 96 128)
    add_own_shader(textures/shaders/detile.comp detile_micro_${BPP}_comp BITS_PER_PIXEL=${BPP} IS_MACRO_TILED=0)
    add_own_shader(textures/shaders/detile.comp detile_macro_${BPP}_comp BITS_PER_PIXEL=${BPP} IS_MACRO_TILED=1)
endforeach()
add_custom_target(host_shaders DEPENDS ${SHADER_HEADERS})

file(GLOB_RECURSE RECOMPILER CONFIGURE_DEPENDS shader/*.cpp)
# Own utilities (types, logging, paths, locks, containers): common/.
file(GLOB COMMON CONFIGURE_DEPENDS common/*.cpp common/logging/*.cpp)
# Own system libraries (Gnm, VideoOut, AvPlayer, event queues, interrupts): platform/.
file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS platform/*.cpp)
# platform/win32: the POSIX subset over Win32 (with its headers in platform/win32/include).
if (NOT WIN32)
    list(FILTER PLATFORM_SOURCES EXCLUDE REGEX "/platform/win32/")
endif()
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale libswresample)
file(GLOB SHIM_SOURCES CONFIGURE_DEPENDS shim/*.cpp shim/*/*.cpp)
# The layer's own modules (independent of the shadPS4 code): layer/.
file(GLOB LAYER_SOURCES CONFIGURE_DEPENDS layer/*.cpp)
# Game profiles (what the translator knows about one game, out of its core): games/.
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS games/*.cpp)
# The translator's own command processor (docs/TRANSLATOR_ARCHITECTURE.ru.md, step 2): cp/.
file(GLOB CP_SOURCES CONFIGURE_DEPENDS cp/*.cpp memory/*.cpp gcn/*.cpp textures/*.cpp buffers/*.cpp vulkan/*.cpp)
# The translator's rendering pieces (labels, timestamps, occlusion, draw preparation, profiling,
# frame capture, prompts): render/. Temporal upscaling, motion vectors, scene resolution: upscale/.
file(GLOB RENDER_SOURCES CONFIGURE_DEPENDS render/*.cpp)
file(GLOB UPSCALE_SOURCES CONFIGURE_DEPENDS upscale/*.cpp upscale/fsr411/*.cpp)
# Windows: linked into bb-probe.exe (a DLL cannot leave the runtime's symbols to the executable
# that loads it, as -Wl,--allow-shlib-undefined does on Linux).
if (WIN32)
    set(BBGPU_LIBRARY_TYPE STATIC)
else()
    set(BBGPU_LIBRARY_TYPE SHARED)
endif()
add_library(bbgpu ${BBGPU_LIBRARY_TYPE} ${RECOMPILER} ${COMMON} ${PLATFORM_SOURCES} ${SHIM_SOURCES} ${LAYER_SOURCES} ${GAME_SOURCES} ${CP_SOURCES}
    ${RENDER_SOURCES} ${UPSCALE_SOURCES})
add_dependencies(bbgpu host_shaders)
# In-game settings menu: Dear ImGui (third_party/imgui, MIT) with its Vulkan backend, and an
# embedded Cyrillic font (third_party/fonts).
target_sources(bbgpu PRIVATE third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_vulkan.cpp)
target_include_directories(bbgpu PRIVATE third_party/imgui third_party/imgui/backends)
# Dear ImGui stays private to libbbgpu.so. The library is linked into bb-probe, so its exports are
# in the global symbol scope; a Vulkan layer carrying its own Dear ImGui without -Bsymbolic
# (vkShade 0.1) had its ImGui_ImplVulkan_Init bound to this copy, which called the Vulkan loader
# with the layer's down-chain physical device handle ("Invalid physicalDevice", abort).
set_source_files_properties(third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_vulkan.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=hidden;-fvisibility-inlines-hidden")
target_compile_definitions(bbgpu PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    BB_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/DejaVuSans.ttf")
set_source_files_properties(shim/bbport_overlay.cpp PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/DejaVuSans.ttf")
target_include_directories(bbgpu PRIVATE . shim layer games cp ${SHADER_INCLUDE} third_party/gcn/include third_party/half/include)
if (WIN32)
    target_include_directories(bbgpu PUBLIC platform/win32/include)
endif()
target_compile_definitions(bbgpu PRIVATE BOOST_ASIO_STANDALONE NDEBUG)
target_compile_options(bbgpu PRIVATE -Wno-deprecated-declarations -Wno-missing-field-initializers)
# Loaded with bb-probe at startup (DT_NEEDED, never dlopen): thread_local access without a
# TLS descriptor call (_dl_tlsdesc_return was ~2% of the draw recording thread).
if (NOT WIN32)
    target_compile_options(bbgpu PRIVATE -ftls-model=initial-exec -fno-semantic-interposition)
    # Runtime symbols (runtime_memory_*, runtime_process_*) resolve against bb-probe, while all
    # functions defined inside libbbgpu.so bind locally without @plt indirection (enabling full LTO inlining).
    target_link_options(bbgpu PRIVATE -Wl,--allow-shlib-undefined -Wl,-Bsymbolic-functions)
endif()
target_link_directories(bbgpu PUBLIC "/home/kekw3/Documents/BloodBorne/AI_WORK_FOR_PORTING/toolchain/sysroot/usr/lib")
target_link_libraries(bbgpu PRIVATE ffx-vulkan::portable ffx-vulkan::fsr4-v07-vulkan
    PkgConfig::SPIRV_TOOLS
    "${CMAKE_CURRENT_SOURCE_DIR}/../our_cpp_100000_source/build/libbb_gx_engine_native.a")
target_link_libraries(bbgpu PUBLIC Vulkan::Headers fmt::fmt Boost::headers magic_enum::magic_enum
    tsl::robin_map GPUOpen::VulkanMemoryAllocator PkgConfig::XXHASH PkgConfig::SDL3 PkgConfig::FFMPEG miniz::miniz Zydis::Zydis sirit)
if (NOT WIN32)
    target_link_libraries(bbgpu PUBLIC PkgConfig::X11)
endif()

# Windows: the runtime (src/, C) and this library in one bb-probe.exe (on Linux build.sh links the
# runtime against libbbgpu.so). Below 1 TiB stay the host heap and thread stacks the game packs
# into 40-bit fields: no high-entropy address randomization.
if (WIN32)
    file(GLOB BB_RUNTIME_SOURCES CONFIGURE_DEPENDS ../src/runtime*.c)
    file(GLOB BB_ATRAC9_SOURCES CONFIGURE_DEPENDS ../third_party/LibAtrac9/C/src/*.c)
    add_library(bbatrac9 STATIC ${BB_ATRAC9_SOURCES})
    target_compile_options(bbatrac9 PRIVATE -w)
    add_executable(bb-probe ../src/probe.c ../src/vulkan_smoke.c ${BB_RUNTIME_SOURCES}
        platform/win32/bb-probe.rc)
    set_target_properties(bb-probe PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
    target_include_directories(bb-probe PRIVATE .. ../src)
    target_compile_options(bb-probe PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>)
    target_link_libraries(bb-probe PRIVATE bbgpu bbatrac9 pthread ws2_32 bcrypt dbghelp winmm onecore
        ${Vulkan_LIBRARIES})
    # Plain -l names in the packages' CMake files (Zydis: Zycore) are found in the sysroot.
    foreach(root ${CMAKE_FIND_ROOT_PATH})
        target_link_directories(bb-probe PRIVATE ${root}/lib)
    endforeach()
    target_link_options(bb-probe PRIVATE -Wl,--disable-high-entropy-va
        -Wl,--stack,16777216 -Wl,--error-limit=0)
endif()

# Explicit test targets; neither launches the game nor needs a Vulkan device.
add_executable(motion-history-test EXCLUDE_FROM_ALL ../tests/test_motion_history.cpp)
target_compile_options(motion-history-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_include_directories(motion-history-test PRIVATE ..)
add_executable(motion-shader-test EXCLUDE_FROM_ALL ../tests/test_motion_shaders.cpp ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(motion-shader-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include)
target_link_libraries(motion-shader-test PRIVATE bbgpu)
add_executable(ui-composition-test EXCLUDE_FROM_ALL ../tests/test_ui_composition.cpp)
target_compile_options(ui-composition-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_include_directories(ui-composition-test PRIVATE ..)
add_executable(cp-decoder-test EXCLUDE_FROM_ALL ../tests/test_cp_decoder.cpp cp/cp_decoder.cpp)
target_compile_options(cp-decoder-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_include_directories(cp-decoder-test PRIVATE cp)
add_executable(fx-init-clear-test EXCLUDE_FROM_ALL ../tests/test_fx_init_clear.cpp cp/cp_decoder.cpp)
target_compile_options(fx-init-clear-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_include_directories(fx-init-clear-test PRIVATE . shim)
add_executable(effect-flags-test EXCLUDE_FROM_ALL ../tests/test_effect_flags.cpp)
target_compile_options(effect-flags-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_include_directories(effect-flags-test PRIVATE shim)
add_executable(settings-test EXCLUDE_FROM_ALL ../tests/test_settings.cpp shim/bbport_settings.cpp)
target_compile_options(settings-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_include_directories(settings-test PRIVATE .. shim)

add_executable(scene-resolution-test EXCLUDE_FROM_ALL ../tests/test_scene_resolution.cpp ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(scene-resolution-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include)
# The test's Vulkan-Hpp inline methods have a different set of platform extensions
# from vk_platform.cpp. Keep them local and use an explicit dispatcher in the test.
target_compile_options(scene-resolution-test PRIVATE -UNDEBUG -fPIC)
set_source_files_properties(../tests/test_scene_resolution.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=hidden;-fvisibility-inlines-hidden")
target_link_libraries(scene-resolution-test PRIVATE bbgpu)
add_executable(taa-shader-test EXCLUDE_FROM_ALL ../tests/test_taa_shader.cpp ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(taa-shader-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include)
target_compile_options(taa-shader-test PRIVATE -UNDEBUG -fPIC)
set_source_files_properties(../tests/test_taa_shader.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=hidden;-fvisibility-inlines-hidden")
target_link_libraries(taa-shader-test PRIVATE bbgpu)
add_dependencies(taa-shader-test host_shaders)

add_executable(camera-motion-test EXCLUDE_FROM_ALL ../tests/test_camera_motion.cpp ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(camera-motion-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include)
target_compile_options(camera-motion-test PRIVATE -UNDEBUG -fPIC)
target_link_libraries(camera-motion-test PRIVATE bbgpu)
add_dependencies(camera-motion-test host_shaders)

add_executable(translation-memory-test EXCLUDE_FROM_ALL ../tests/test_translation_memory.cpp
    ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(translation-memory-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include)
target_compile_options(translation-memory-test PRIVATE -UNDEBUG -fPIC)
set_source_files_properties(../tests/test_translation_memory.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=hidden;-fvisibility-inlines-hidden")
target_link_libraries(translation-memory-test PRIVATE bbgpu)

# bbport: FSR 4 benchmark outside the game (tools/fsr4_bench.cpp): ninja -C out/gpu fsr4-bench.
add_executable(upscaler-support-test EXCLUDE_FROM_ALL ../tests/test_upscaler_support.cpp
    shim/bbport_settings.cpp)
target_include_directories(upscaler-support-test PRIVATE shim)
target_compile_options(upscaler-support-test PRIVATE -UNDEBUG)

add_executable(fsr4-bench EXCLUDE_FROM_ALL tools/fsr4_bench.cpp
    upscale/fsr411/fsr411.cpp)
target_include_directories(fsr4-bench PRIVATE upscale/fsr411)
target_link_libraries(fsr4-bench PRIVATE ffx-vulkan::fsr4-v07-vulkan ffx-vulkan::fsr4-v07-assets Vulkan::Vulkan)

# bbport online module (gpu/bbnet, libbbnet.so): built when its libraries are there (shell.nix);
# BB_NET=OFF leaves it out. Without it the port plays offline.
option(BB_NET "Build the online module (shadNet)" ON)
if (BB_NET)
    find_package(Protobuf CONFIG QUIET)
    find_package(nlohmann_json CONFIG QUIET)
    find_package(OpenSSL QUIET)
    find_package(ZLIB QUIET)
    find_path(MINIUPNPC_INCLUDE miniupnpc.h PATH_SUFFIXES miniupnpc)
    find_library(MINIUPNPC_LIBRARY NAMES miniupnpc)
    if (Protobuf_FOUND AND nlohmann_json_FOUND AND OpenSSL_FOUND AND ZLIB_FOUND AND
        MINIUPNPC_INCLUDE AND MINIUPNPC_LIBRARY)
        add_subdirectory(bbnet)
        add_subdirectory(bbnet20)
        message(STATUS "Online module: on (libbbnet.so)")
    else()
        # Named one by one (#145: "the build has no online module" with no hint why).
        set(missing "")
        foreach(dep Protobuf nlohmann_json OpenSSL ZLIB)
            if (NOT ${dep}_FOUND)
                list(APPEND missing ${dep})
            endif()
        endforeach()
        if (NOT MINIUPNPC_INCLUDE OR NOT MINIUPNPC_LIBRARY)
            list(APPEND missing miniupnpc)
        endif()
        list(JOIN missing ", " missing)
        message(STATUS "Online module: off (missing: ${missing}; shell.nix lists the packages)")
    endif()
endif()

# bbport: the shader corpus replay (render/shader_capture.h): ninja -C out/gpu shader-corpus-test.
add_executable(shader-corpus-test EXCLUDE_FROM_ALL ../tests/test_shader_corpus.cpp
    ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(shader-corpus-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include third_party/half/include)
target_compile_options(shader-corpus-test PRIVATE -UNDEBUG -fPIC)
target_link_libraries(shader-corpus-test PRIVATE bbgpu)

add_executable(srt-walker-fault-test EXCLUDE_FROM_ALL ../tests/test_srt_walker_fault.cpp
    ../tests/test_gpu_runtime_stubs.cpp)
target_include_directories(srt-walker-fault-test PRIVATE . shim ${SHADER_INCLUDE} third_party/gcn/include third_party/half/include)
target_compile_options(srt-walker-fault-test PRIVATE -UNDEBUG -Wall -Wextra)
target_link_libraries(srt-walker-fault-test PRIVATE bbgpu)
