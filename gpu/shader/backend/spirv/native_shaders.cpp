// SPDX-License-Identifier: GPL-2.0-or-later
// Block 5, Step 5.4: Native Vulkan 1.3 GLSL -> SPIR-V shader replacement registry with
// strict Input/Output Location, interpolation-mode, and DescriptorSet/Binding signature verification.

#include "shader/backend/spirv/native_shaders.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>
#include <vector>

#include "bbgpu/shaders/cs_a954e79d_indirect_prep_comp.h"
#include "bbgpu/shaders/ps_0ef50047_pbr_frag.h"
#include "bbgpu/shaders/ps_0f61fd27_vsm_blur_frag.h"
#include "bbgpu/shaders/ps_111fce32_frag.h"
#include "bbgpu/shaders/ps_11ffdfb5_frag.h"
#include "bbgpu/shaders/ps_1ae03d75_vsm_moment_cldist_frag.h"
#include "bbgpu/shaders/ps_1ae03d75_vsm_moment_noclip_frag.h"
#include "bbgpu/shaders/ps_1c3953c6_gbuf_frag.h"
#include "bbgpu/shaders/ps_2390c887_gbuf7_frag.h"
#include "bbgpu/shaders/ps_32e47788_atmos_fog_frag.h"
#include "bbgpu/shaders/ps_38d65b32_frag.h"
#include "bbgpu/shaders/ps_5c02eaf8_frag.h"
#include "bbgpu/shaders/ps_5e224cb6_vfx_fog_frag.h"
#include "bbgpu/shaders/ps_6b42fc89_frag.h"
#include "bbgpu/shaders/ps_71c96598_vfx_fog_frag.h"
#include "bbgpu/shaders/ps_7cc2c044_pbr_frag.h"
#include "bbgpu/shaders/ps_8aba0ecd_gbuf_frag.h"
#include "bbgpu/shaders/ps_8c08444c_blur13_frag.h"
#include "bbgpu/shaders/ps_a22c7f71_pbr_frag.h"
#include "bbgpu/shaders/ps_bae5454e_pbr_frag.h"
#include "bbgpu/shaders/ps_bb10e440_gbuf_frag.h"
#include "bbgpu/shaders/ps_cbbca5dc_frag.h"
#include "bbgpu/shaders/ps_d92266b9_pbr_frag.h"
#include "bbgpu/shaders/ps_e0305cef_post_frag.h"
#include "bbgpu/shaders/ps_e55e45eb_frag.h"
#include "bbgpu/shaders/ps_e96447c3_pbr_frag.h"
#include "bbgpu/shaders/ps_ed4c7668_lut3d_frag.h"
#include "bbgpu/shaders/ps_f55643de_frag.h"
#include "bbgpu/shaders/ps_f7229091_bgra_blit_frag.h"
#include "bbgpu/shaders/vs_061fbbf6_vert.h"
#include "bbgpu/shaders/vs_167daba6_vsm_shadow_vert.h"
#include "bbgpu/shaders/vs_1ea5a07e_vert.h"
#include "bbgpu/shaders/vs_637500e0_vert.h"
#include "bbgpu/shaders/vs_6a833138_gbuf_inst_vert.h"
#include "bbgpu/shaders/vs_7620368b_gbuf_uv2_vert.h"
#include "bbgpu/shaders/vs_9da6aacb_gbuf_vert.h"
#include "bbgpu/shaders/vs_ecbef4c7_gbuf_inst_uv2_vert.h"
#include "bbgpu/shaders/ps_370174f7_frag.h"
#include "bbgpu/shaders/ps_d22cbfd1_frag.h"
#include "bbgpu/shaders/ps_149432ff_frag.h"
#include "bbgpu/shaders/ps_5de04266_frag.h"
#include "bbgpu/shaders/ps_3fe9e964_frag.h"
#include "bbgpu/shaders/ps_2140da2b_frag.h"
#include "bbgpu/shaders/ps_dda49e89_frag.h"
#include "bbgpu/shaders/ps_02cd8f9b_frag.h"
#include "bbgpu/shaders/ps_07d07cf3_frag.h"
#include "bbgpu/shaders/ps_0b2ace4c_frag.h"
#include "bbgpu/shaders/ps_186d69d5_frag.h"
#include "bbgpu/shaders/ps_201cb43d_frag.h"
#include "bbgpu/shaders/ps_22b357f3_frag.h"
#include "bbgpu/shaders/ps_3105494c_frag.h"
#include "bbgpu/shaders/ps_31b90cc5_frag.h"
#include "bbgpu/shaders/ps_41d3ce4b_frag.h"
#include "bbgpu/shaders/ps_44a82ffb_frag.h"
#include "bbgpu/shaders/ps_505ebdbe_frag.h"
#include "bbgpu/shaders/ps_5d7150da_frag.h"
#include "bbgpu/shaders/ps_5f76eb3d_frag.h"
#include "bbgpu/shaders/ps_600f86b0_frag.h"
#include "bbgpu/shaders/ps_799bfa32_frag.h"
#include "bbgpu/shaders/ps_88ae6537_frag.h"
#include "bbgpu/shaders/ps_896e7242_frag.h"
#include "bbgpu/shaders/ps_898dabb3_frag.h"
#include "bbgpu/shaders/ps_89f9ee3e_frag.h"
#include "bbgpu/shaders/ps_8d5387d9_frag.h"
#include "bbgpu/shaders/ps_8ea5be99_frag.h"
#include "bbgpu/shaders/ps_9f9ef267_frag.h"
#include "bbgpu/shaders/ps_b59b4c4e_frag.h"
#include "bbgpu/shaders/ps_b5b52d63_frag.h"
#include "bbgpu/shaders/ps_c6edb19a_frag.h"
#include "bbgpu/shaders/ps_d729c469_frag.h"
#include "bbgpu/shaders/ps_d75fe2ff_frag.h"
#include "bbgpu/shaders/ps_f2531727_frag.h"
#include "bbgpu/shaders/ps_f40da260_frag.h"
#include "bbgpu/shaders/ps_f81596cd_frag.h"
#include "bbgpu/shaders/ps_006c079b_frag.h"
#include "bbgpu/shaders/ps_4fd8db00_frag.h"
#include "bbgpu/shaders/ps_5e4596f4_frag.h"
#include "bbgpu/shaders/ps_5e67d866_frag.h"
#include "bbgpu/shaders/ps_70bb42af_frag.h"
#include "bbgpu/shaders/ps_749e4f9e_frag.h"
#include "bbgpu/shaders/ps_765450df_frag.h"
#include "bbgpu/shaders/ps_8650418f_frag.h"
#include "bbgpu/shaders/ps_9837d1c8_frag.h"
#include "bbgpu/shaders/ps_9e1cb278_frag.h"
#include "bbgpu/shaders/ps_a291c498_frag.h"
#include "bbgpu/shaders/ps_a89e01e9_frag.h"
#include "bbgpu/shaders/ps_ab4e5342_frag.h"
#include "bbgpu/shaders/ps_babda34d_frag.h"
#include "bbgpu/shaders/ps_bd1d09f0_frag.h"
#include "bbgpu/shaders/ps_c575bc29_frag.h"
#include "bbgpu/shaders/ps_ca7226ec_frag.h"
#include "bbgpu/shaders/ps_ccbf44a6_frag.h"
#include "bbgpu/shaders/ps_dbeccd9c_frag.h"
#include "bbgpu/shaders/ps_e21e283a_frag.h"
#include "bbgpu/shaders/ps_e54449c7_frag.h"
#include "bbgpu/shaders/ps_fbe7c4b6_frag.h"
#include "bbgpu/shaders/ps_6c2323c2_frag.h"
#include "bbgpu/shaders/ps_6320bb24_frag.h"
#include "bbgpu/shaders/ps_e820ee9d_frag.h"
#include "bbgpu/shaders/ps_998b035e_frag.h"
#include "bbgpu/shaders/ps_a0e88f56_noclip_frag.h"
#include "bbgpu/shaders/ps_a0e88f56_cldist_frag.h"

namespace Shader::Backend::SPIRV {

namespace {

constexpr u32 kSpirvMagic = 0x07230203u;

// SPIR-V opcodes
constexpr u32 kOpExtInst = 12u;
constexpr u32 kOpEntryPoint = 15u;
constexpr u32 kOpTypeInt = 21u;
constexpr u32 kOpTypeFloat = 22u;
constexpr u32 kOpTypeVector = 23u;
constexpr u32 kOpTypeImage = 25u;
constexpr u32 kOpTypeSampler = 26u;
constexpr u32 kOpTypeSampledImage = 27u;
constexpr u32 kOpTypeStruct = 30u;
constexpr u32 kOpTypePointer = 32u;
constexpr u32 kOpFunction = 54u;
constexpr u32 kOpFunctionCall = 57u;
constexpr u32 kOpVariable = 59u;
constexpr u32 kOpLoad = 61u;
constexpr u32 kOpStore = 62u;
constexpr u32 kOpAccessChain = 65u;
constexpr u32 kOpInBoundsAccessChain = 66u;
constexpr u32 kOpImageTexelPointer = 67u;
constexpr u32 kOpDecorate = 71u;
constexpr u32 kOpCopyObject = 83u;

// Storage classes
constexpr u32 kStorageClassUniformConstant = 0u;
constexpr u32 kStorageClassInput = 1u;
constexpr u32 kStorageClassUniform = 2u;
constexpr u32 kStorageClassOutput = 3u;
constexpr u32 kStorageClassStorageBuffer = 12u;

// Decorations
constexpr u32 kDecoBlock = 2u;
constexpr u32 kDecoBufferBlock = 3u;
constexpr u32 kDecoBuiltIn = 11u;
constexpr u32 kDecoNoPerspective = 13u;
constexpr u32 kDecoFlat = 14u;
constexpr u32 kDecoCentroid = 16u;
constexpr u32 kDecoSample = 17u;
constexpr u32 kDecoLocation = 30u;
constexpr u32 kDecoBinding = 33u;
constexpr u32 kDecoDescriptorSet = 34u;
constexpr u32 kDecoPerVertexKHR = 5285u;

// Interpolation flag bits
constexpr u8 kInterpFlat = 0x1u;
constexpr u8 kInterpNoPersp = 0x2u;
constexpr u8 kInterpCentroid = 0x4u;
constexpr u8 kInterpSample = 0x8u;

struct NativeShaderCandidate {
    u64 pgm_hash;
    std::span<const u32> words;
    const char* label;
};

constexpr std::array<NativeShaderCandidate, 102> kNativeRegistry{{
    {0x000000000f61fd27ull, PS_0F61FD27_VSM_BLUR_FRAG, "ps_0f61fd27_vsm_blur"},
    {0x000000001ae03d75ull, PS_1AE03D75_VSM_MOMENT_CLDIST_FRAG, "ps_1ae03d75_vsm_moment_cldist"},
    {0x000000001ae03d75ull, PS_1AE03D75_VSM_MOMENT_NOCLIP_FRAG, "ps_1ae03d75_vsm_moment_noclip"},
    {0x00000000f7229091ull, PS_F7229091_BGRA_BLIT_FRAG, "ps_f7229091_bgra_blit"},
    {0x00000000ed4c7668ull, PS_ED4C7668_LUT3D_FRAG, "ps_ed4c7668_lut3d"},
    {0x00000000167daba6ull, VS_167DABA6_VSM_SHADOW_VERT, "vs_167daba6_vsm_shadow"},
    {0x000000008c08444cull, PS_8C08444C_BLUR13_FRAG, "ps_8c08444c_blur13"},
    {0x0000000032e47788ull, PS_32E47788_ATMOS_FOG_FRAG, "ps_32e47788_atmos_fog"},
    {0x000000005e224cb6ull, PS_5E224CB6_VFX_FOG_FRAG, "ps_5e224cb6_vfx_fog"},
    {0x0000000071c96598ull, PS_71C96598_VFX_FOG_FRAG, "ps_71c96598_vfx_fog"},
    {0x000000009da6aacbull, VS_9DA6AACB_GBUF_VERT, "vs_9da6aacb_gbuf"},
    {0x000000006a833138ull, VS_6A833138_GBUF_INST_VERT, "vs_6a833138_gbuf_inst"},
    {0x000000007620368bull, VS_7620368B_GBUF_UV2_VERT, "vs_7620368b_gbuf_uv2"},
    {0x00000000ecbef4c7ull, VS_ECBEF4C7_GBUF_INST_UV2_VERT, "vs_ecbef4c7_gbuf_inst_uv2"},
    {0x00000000bb10e440ull, PS_BB10E440_GBUF_FRAG, "ps_bb10e440_gbuf"},
    {0x000000008aba0ecdull, PS_8ABA0ECD_GBUF_FRAG, "ps_8aba0ecd_gbuf"},
    {0x000000001c3953c6ull, PS_1C3953C6_GBUF_FRAG, "ps_1c3953c6_gbuf"},
    {0x00000000a22c7f71ull, PS_A22C7F71_PBR_FRAG, "ps_a22c7f71_pbr"},
    {0x00000000d92266b9ull, PS_D92266B9_PBR_FRAG, "ps_d92266b9_pbr"},
    {0x000000007cc2c044ull, PS_7CC2C044_PBR_FRAG, "ps_7cc2c044_pbr"},
    {0x00000000e96447c3ull, PS_E96447C3_PBR_FRAG, "ps_e96447c3_pbr"},
    {0x00000000bae5454eull, PS_BAE5454E_PBR_FRAG, "ps_bae5454e_pbr"},
    {0x000000000ef50047ull, PS_0EF50047_PBR_FRAG, "ps_0ef50047_pbr"},
    {0x00000000e0305cefull, PS_E0305CEF_POST_FRAG, "ps_e0305cef_post"},
    {0x000000002390c887ull, PS_2390C887_GBUF7_FRAG, "ps_2390c887_gbuf7"},
    {0x00000000a954e79dull, CS_A954E79D_INDIRECT_PREP_COMP, "cs_a954e79d_indirect_prep"},
    {0x00000000e55e45ebull, PS_E55E45EB_FRAG, "ps_e55e45eb_godrays"},
    {0x000000005c02eaf8ull, PS_5C02EAF8_FRAG, "ps_5c02eaf8_tonemap"},
    {0x0000000011ffdfb5ull, PS_11FFDFB5_FRAG, "ps_11ffdfb5_bilateral"},
    {0x00000000111fce32ull, PS_111FCE32_FRAG, "ps_111fce32_dof_lerp"},
    {0x00000000cbbca5dcull, PS_CBBCA5DC_FRAG, "ps_cbbca5dc_ssao"},
    {0x00000000f55643deull, PS_F55643DE_FRAG, "ps_f55643de_csm_pcf"},
    {0x0000000038d65b32ull, PS_38D65B32_FRAG, "ps_38d65b32_gamma_lut1d"},
    {0x000000006b42fc89ull, PS_6B42FC89_FRAG, "ps_6b42fc89_pbr_3pt_vsm"},
    {0x00000000637500e0ull, VS_637500E0_VERT, "vs_637500e0_godrays"},
    {0x00000000061fbbf6ull, VS_061FBBF6_VERT, "vs_061fbbf6_post_quad"},
    {0x000000001ea5a07eull, VS_1EA5A07E_VERT, "vs_1ea5a07e_tonemap"},
    {0x00000000370174f7ull, PS_370174F7_FRAG, "ps_370174f7_pbr_1pt"},
    {0x00000000d22cbfd1ull, PS_D22CBFD1_FRAG, "ps_d22cbfd1_pbr_3pt"},
    {0x00000000149432ffull, PS_149432FF_FRAG, "ps_149432ff_pbr_4pt"},
    {0x000000005de04266ull, PS_5DE04266_FRAG, "ps_5de04266_pbr_5pt"},
    {0x000000003fe9e964ull, PS_3FE9E964_FRAG, "ps_3fe9e964_ibr"},
    {0x000000002140da2bull, PS_2140DA2B_FRAG, "ps_2140da2b_gbuf6"},
    {0x00000000dda49e89ull, PS_DDA49E89_FRAG, "ps_dda49e89_light_vol"},
    {0x0000000002cd8f9bull, PS_02CD8F9B_FRAG, "ps_02cd8f9b_2mrt"},
    {0x0000000007d07cf3ull, PS_07D07CF3_FRAG, "ps_07d07cf3_pbr_6pt"},
    {0x000000000b2ace4cull, PS_0B2ACE4C_FRAG, "ps_0b2ace4c_pbr_9pt"},
    {0x00000000186d69d5ull, PS_186D69D5_FRAG, "ps_186d69d5_pbr_2pt"},
    {0x00000000201cb43dull, PS_201CB43D_FRAG, "ps_201cb43d_pbr_10pt"},
    {0x0000000022b357f3ull, PS_22B357F3_FRAG, "ps_22b357f3_pbr_11pt"},
    {0x000000003105494cull, PS_3105494C_FRAG, "ps_3105494c_pbr_4pt"},
    {0x0000000031b90cc5ull, PS_31B90CC5_FRAG, "ps_31b90cc5_2mrt"},
    {0x0000000041d3ce4bull, PS_41D3CE4B_FRAG, "ps_41d3ce4b_2mrt"},
    {0x0000000044a82ffbull, PS_44A82FFB_FRAG, "ps_44a82ffb_2mrt"},
    {0x00000000505ebdbeull, PS_505EBDBE_FRAG, "ps_505ebdbe_pbr_10pt"},
    {0x000000005d7150daull, PS_5D7150DA_FRAG, "ps_5d7150da_pbr_6pt"},
    {0x000000005f76eb3dull, PS_5F76EB3D_FRAG, "ps_5f76eb3d_pbr_11pt"},
    {0x00000000600f86b0ull, PS_600F86B0_FRAG, "ps_600f86b0_pbr_2pt"},
    {0x00000000799bfa32ull, PS_799BFA32_FRAG, "ps_799bfa32_pbr_12pt"},
    {0x0000000088ae6537ull, PS_88AE6537_FRAG, "ps_88ae6537_2mrt"},
    {0x00000000896e7242ull, PS_896E7242_FRAG, "ps_896e7242_2mrt"},
    {0x00000000898dabb3ull, PS_898DABB3_FRAG, "ps_898dabb3_2mrt"},
    {0x0000000089f9ee3eull, PS_89F9EE3E_FRAG, "ps_89f9ee3e_pbr_3pt"},
    {0x000000008d5387d9ull, PS_8D5387D9_FRAG, "ps_8d5387d9_pbr_7pt"},
    {0x000000008ea5be99ull, PS_8EA5BE99_FRAG, "ps_8ea5be99_2mrt"},
    {0x000000009f9ef267ull, PS_9F9EF267_FRAG, "ps_9f9ef267_pbr_12pt"},
    {0x00000000b59b4c4eull, PS_B59B4C4E_FRAG, "ps_b59b4c4e_pbr_9pt"},
    {0x00000000b5b52d63ull, PS_B5B52D63_FRAG, "ps_b5b52d63_2mrt"},
    {0x00000000c6edb19aull, PS_C6EDB19A_FRAG, "ps_c6edb19a_pbr_8pt"},
    {0x00000000d729c469ull, PS_D729C469_FRAG, "ps_d729c469_2mrt"},
    {0x00000000d75fe2ffull, PS_D75FE2FF_FRAG, "ps_d75fe2ff_pbr_5pt"},
    {0x00000000f2531727ull, PS_F2531727_FRAG, "ps_f2531727_pbr_7pt"},
    {0x00000000f40da260ull, PS_F40DA260_FRAG, "ps_f40da260_pbr_1pt"},
    {0x00000000f81596cdull, PS_F81596CD_FRAG, "ps_f81596cd_pbr_8pt"},
    {0x00000000006c079bull, PS_006C079B_FRAG, "ps_006c079b_csm"},
    {0x000000004fd8db00ull, PS_4FD8DB00_FRAG, "ps_4fd8db00_zpre"},
    {0x000000005e4596f4ull, PS_5E4596F4_FRAG, "ps_5e4596f4_1mrt"},
    {0x000000005e67d866ull, PS_5E67D866_FRAG, "ps_5e67d866_tonemap_hd"},
    {0x0000000070bb42afull, PS_70BB42AF_FRAG, "ps_70bb42af_gbuf6"},
    {0x00000000749e4f9eull, PS_749E4F9E_FRAG, "ps_749e4f9e_half"},
    {0x00000000765450dfull, PS_765450DF_FRAG, "ps_765450df_fog_vol"},
    {0x000000008650418full, PS_8650418F_FRAG, "ps_8650418f_post"},
    {0x000000009837d1c8ull, PS_9837D1C8_FRAG, "ps_9837d1c8_depth"},
    {0x000000009e1cb278ull, PS_9E1CB278_FRAG, "ps_9e1cb278_dof"},
    {0x00000000a291c498ull, PS_A291C498_FRAG, "ps_a291c498_screen"},
    {0x00000000a89e01e9ull, PS_A89E01E9_FRAG, "ps_a89e01e9_gbuf6"},
    {0x00000000ab4e5342ull, PS_AB4E5342_FRAG, "ps_ab4e5342_2mrt"},
    {0x00000000babda34dull, PS_BABDA34D_FRAG, "ps_babda34d_1mrt"},
    {0x00000000bd1d09f0ull, PS_BD1D09F0_FRAG, "ps_bd1d09f0_mist"},
    {0x00000000c575bc29ull, PS_C575BC29_FRAG, "ps_c575bc29_2mrt"},
    {0x00000000ca7226ecull, PS_CA7226EC_FRAG, "ps_ca7226ec_gbuf6"},
    {0x00000000ccbf44a6ull, PS_CCBF44A6_FRAG, "ps_ccbf44a6_post"},
    {0x00000000dbeccd9cull, PS_DBECCD9C_FRAG, "ps_dbeccd9c_gbuf6"},
    {0x00000000e21e283aull, PS_E21E283A_FRAG, "ps_e21e283a_gbuf6"},
    {0x00000000e54449c7ull, PS_E54449C7_FRAG, "ps_e54449c7_gbuf6"},
    {0x00000000fbe7c4b6ull, PS_FBE7C4B6_FRAG, "ps_fbe7c4b6_bloom"},
    {0x000000006c2323c2ull, PS_6C2323C2_FRAG, "ps_6c2323c2_ssr_raymarch"},
    {0x000000006320bb24ull, PS_6320BB24_FRAG, "ps_6320bb24_ssr_mip_blur"},
    {0x00000000e820ee9dull, PS_E820EE9D_FRAG, "ps_e820ee9d_ssr_hiz_depth"},
    {0x00000000998b035eull, PS_998B035E_FRAG, "ps_998b035e_gbuf6_alpha"},
    {0x00000000a0e88f56ull, PS_A0E88F56_NOCLIP_FRAG, "ps_a0e88f56_csm_depth_noclip"},
    {0x00000000a0e88f56ull, PS_A0E88F56_CLDIST_FRAG, "ps_a0e88f56_vsm_depth_cldist"},
}};

struct DescSlot {
    u32 set = 0;
    u32 binding = 0;
    u32 kind_tag = 0;
};

struct SpvInterfaceSig {
    u32 exec_model = 0xFFFFFFFFu;
    bool has_per_vertex_input = false;
    u64 in_loc_mask = 0;
    u64 out_loc_mask = 0;
    std::array<u8, 64> in_interp{};
    std::array<u8, 64> in_comps{};
    std::array<u8, 64> out_comps{};
    std::vector<DescSlot> descs;
};

bool ExtractInterfaceSig(std::span<const u32> spv, SpvInterfaceSig& sig) {
    if (spv.size() < 5 || spv[0] != kSpirvMagic) {
        return false;
    }
    const u32 bound = spv[3];
    if (bound == 0 || bound > 2'000'000u) {
        return false;
    }

    std::vector<u8> type_comps(bound, 0u);
    std::vector<u32> type_desc_tag(bound, 0u);
    std::vector<u8> struct_deco(bound, 0u); // 1 = Block, 2 = BufferBlock
    std::vector<u32> ptr_sc(bound, 0xFFFFFFFFu);
    std::vector<u32> ptr_pointee(bound, 0u);

    std::vector<u8> var_has_loc(bound, 0u);
    std::vector<u32> var_loc(bound, 0u);
    std::vector<u8> var_has_builtin(bound, 0u);
    std::vector<u8> var_interp(bound, 0u);
    std::vector<u8> var_per_vertex(bound, 0u);
    std::vector<u8> var_has_binding(bound, 0u);
    std::vector<u32> var_binding(bound, 0u);
    std::vector<u32> var_set(bound, 0u);
    std::vector<u8> var_used(bound, 0u);

    struct VarRec {
        u32 id;
        u32 ptr_type;
        u32 sc;
    };
    std::vector<VarRec> global_vars;
    bool in_func = false;

    for (size_t pos = 5; pos < spv.size();) {
        const u32 wc = spv[pos] >> 16;
        const u32 op = spv[pos] & 0xFFFFu;
        if (wc == 0 || pos + wc > spv.size()) {
            return false;
        }
        const u32* w = &spv[pos];
        if (op == kOpFunction) {
            in_func = true;
        }
        if (in_func) {
            // Track which global descriptor variables are actually referenced inside functions
            // (so unreferenced pre-spirv-opt declarations like %clip_planes are ignored).
            switch (op) {
            case kOpLoad:
            case kOpAccessChain:
            case kOpInBoundsAccessChain:
            case kOpImageTexelPointer:
            case kOpCopyObject:
                if (wc >= 4 && w[3] < bound) {
                    var_used[w[3]] = 1u;
                }
                break;
            case kOpStore:
                if (wc >= 3 && w[1] < bound) {
                    var_used[w[1]] = 1u;
                }
                break;
            case kOpExtInst:
                if (wc >= 6 && w[5] < bound) {
                    var_used[w[5]] = 1u;
                }
                break;
            case kOpFunctionCall:
                for (u32 i = 3; i < wc; ++i) {
                    if (w[i] < bound) {
                        var_used[w[i]] = 1u;
                    }
                }
                break;
            default:
                break;
            }
            pos += wc;
            continue;
        }

        switch (op) {
        case kOpEntryPoint:
            if (wc >= 4 && sig.exec_model == 0xFFFFFFFFu) {
                sig.exec_model = w[1];
            }
            break;
        case kOpDecorate:
            if (wc >= 3 && w[1] < bound) {
                const u32 target = w[1];
                const u32 deco = w[2];
                if (deco == kDecoLocation && wc >= 4) {
                    var_has_loc[target] = 1u;
                    var_loc[target] = w[3];
                } else if (deco == kDecoBuiltIn) {
                    var_has_builtin[target] = 1u;
                } else if (deco == kDecoFlat) {
                    var_interp[target] |= kInterpFlat;
                } else if (deco == kDecoNoPerspective) {
                    var_interp[target] |= kInterpNoPersp;
                } else if (deco == kDecoCentroid) {
                    var_interp[target] |= kInterpCentroid;
                } else if (deco == kDecoSample) {
                    var_interp[target] |= kInterpSample;
                } else if (deco == kDecoPerVertexKHR) {
                    var_per_vertex[target] = 1u;
                } else if (deco == kDecoBinding && wc >= 4) {
                    var_has_binding[target] = 1u;
                    var_binding[target] = w[3];
                } else if (deco == kDecoDescriptorSet && wc >= 4) {
                    var_set[target] = w[3];
                } else if (deco == kDecoBlock) {
                    struct_deco[target] = 1u;
                } else if (deco == kDecoBufferBlock) {
                    struct_deco[target] = 2u;
                }
            }
            break;
        case kOpTypeInt:
        case kOpTypeFloat:
            if (wc >= 3 && w[1] < bound) {
                type_comps[w[1]] = 1u;
            }
            break;
        case kOpTypeVector:
            if (wc == 4 && w[1] < bound && w[3] <= 4u) {
                type_comps[w[1]] = static_cast<u8>(w[3]);
            }
            break;
        case kOpTypeImage:
            if (wc >= 8 && w[1] < bound) {
                // Tag encodes Image (0x1000) + dim + arrayed + ms + sampled (ignore depth hint w[4],
                // which glslang sets to 0 while EmitSPIRV also sets 0 or 1).
                const u32 dim = w[3] & 0x7u;
                const u32 arrayed = (w[5] & 0x1u) << 3;
                const u32 ms = (w[6] & 0x1u) << 4;
                const u32 sampled = (w[7] & 0x3u) << 5;
                type_desc_tag[w[1]] = 0x1000u | dim | arrayed | ms | sampled;
            }
            break;
        case kOpTypeSampler:
            if (wc == 2 && w[1] < bound) {
                type_desc_tag[w[1]] = 0x2000u;
            }
            break;
        case kOpTypeSampledImage:
            if (wc == 3 && w[1] < bound && w[2] < bound) {
                type_desc_tag[w[1]] = 0x3000u | (type_desc_tag[w[2]] & 0xFFFu);
            }
            break;
        case kOpTypeStruct:
            if (wc >= 2 && w[1] < bound) {
                type_desc_tag[w[1]] = 0x4000u;
            }
            break;
        case kOpTypePointer:
            if (wc == 4 && w[1] < bound && w[3] < bound) {
                ptr_sc[w[1]] = w[2];
                ptr_pointee[w[1]] = w[3];
            }
            break;
        case kOpVariable:
            if (wc >= 4 && w[1] < bound && w[2] < bound) {
                global_vars.push_back({w[2], w[1], w[3]});
            }
            break;
        default:
            break;
        }
        pos += wc;
    }

    if (sig.exec_model == 0xFFFFFFFFu) {
        return false;
    }

    for (const auto& v : global_vars) {
        const u32 pointee = ptr_pointee[v.ptr_type];
        if (v.sc == kStorageClassInput) {
            if (!var_used[v.id] || var_has_builtin[v.id] ||
                (pointee < bound && struct_deco[pointee] != 0 && !var_has_loc[v.id])) {
                continue;
            }
            if (var_per_vertex[v.id]) {
                sig.has_per_vertex_input = true;
            }
            if (!var_has_loc[v.id] || var_loc[v.id] >= 64u) {
                return false;
            }
            const u32 loc = var_loc[v.id];
            sig.in_loc_mask |= (1ull << loc);
            sig.in_interp[loc] = var_interp[v.id];
            sig.in_comps[loc] = (pointee < bound) ? type_comps[pointee] : 0u;
        } else if (v.sc == kStorageClassOutput) {
            if (var_has_builtin[v.id] ||
                (pointee < bound && struct_deco[pointee] != 0 && !var_has_loc[v.id])) {
                continue;
            }
            if (!var_has_loc[v.id] || var_loc[v.id] >= 64u) {
                return false;
            }
            const u32 loc = var_loc[v.id];
            sig.out_loc_mask |= (1ull << loc);
            sig.out_comps[loc] = (pointee < bound) ? type_comps[pointee] : 0u;
        } else if (var_has_binding[v.id] && var_used[v.id]) {
            u32 kind_tag = 0;
            if (v.sc == kStorageClassStorageBuffer) {
                kind_tag = 0x5001u; // SSBO
            } else if (v.sc == kStorageClassUniform) {
                kind_tag = (pointee < bound && struct_deco[pointee] == 2u) ? 0x5001u : 0x5002u;
            } else if (v.sc == kStorageClassUniformConstant && pointee < bound) {
                kind_tag = type_desc_tag[pointee];
            }
            if (kind_tag == 0) {
                return false;
            }
            // Deduplicate aliased views on the same (set, binding) (e.g. u32/u16/u8 SSBO aliases).
            bool found = false;
            for (const auto& existing : sig.descs) {
                if (existing.set == var_set[v.id] && existing.binding == var_binding[v.id]) {
                    if (existing.kind_tag != kind_tag) {
                        return false;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                sig.descs.push_back({var_set[v.id], var_binding[v.id], kind_tag});
            }
        }
    }

    std::ranges::sort(sig.descs, [](const DescSlot& a, const DescSlot& b) {
        return (a.set != b.set) ? (a.set < b.set) : (a.binding < b.binding);
    });
    return true;
}

bool ParseEnvFlag(const char* name, bool default_value) noexcept {
    const char* v = std::getenv(name);
    if (!v || !*v) {
        return default_value;
    }
    return !(v[0] == '0' && v[1] == '\0');
}

u32 ParseEnvU32(const char* name, u32 def) noexcept {
    const char* v = std::getenv(name);
    if (!v || !*v) {
        return def;
    }
    char* end = nullptr;
    const unsigned long val = std::strtoul(v, &end, 10);
    return (end && end != v) ? static_cast<u32>(val) : def;
}

} // namespace

bool NativeShadersEnabled() noexcept {
    static const bool enabled = ParseEnvFlag("BB_NATIVE_SHADERS", true);
    return enabled;
}

u32 NativeShadersMinIdx() noexcept {
    static const u32 min_idx = ParseEnvU32("BB_NATIVE_SHADER_MIN", 0u);
    return min_idx;
}

u32 NativeShadersMaxIdx() noexcept {
    static const u32 max_idx = ParseEnvU32("BB_NATIVE_SHADER_MAX", 101u);
    return max_idx;
}

bool NativeShadersCustomRange() noexcept {
    return NativeShadersMinIdx() != 0u ||
           NativeShadersMaxIdx() + 1u < static_cast<u32>(kNativeRegistry.size());
}

bool TryReplaceWithNativeShader(u64 pgm_hash, const std::vector<u32>& ref_spv,
                                std::vector<u32>& out_spv) {
    if (!NativeShadersEnabled()) {
        return false;
    }

    const size_t min_i = std::min<size_t>(NativeShadersMinIdx(), kNativeRegistry.size());
    const size_t max_i =
        std::min<size_t>(static_cast<size_t>(NativeShadersMaxIdx()) + 1u, kNativeRegistry.size());
    if (min_i >= max_i) {
        return false;
    }

    bool any_hash_match = false;
    for (size_t idx = min_i; idx < max_i; ++idx) {
        if (kNativeRegistry[idx].pgm_hash == pgm_hash) {
            any_hash_match = true;
            break;
        }
    }
    if (!any_hash_match) {
        return false;
    }

    SpvInterfaceSig ref_sig{};
    if (!ExtractInterfaceSig(ref_spv, ref_sig) || ref_sig.has_per_vertex_input) {
        return false;
    }

    for (size_t idx = min_i; idx < max_i; ++idx) {
        const auto& entry = kNativeRegistry[idx];
        if (entry.pgm_hash != pgm_hash) {
            continue;
        }
        SpvInterfaceSig cand_sig{};
        if (!ExtractInterfaceSig(entry.words, cand_sig) || cand_sig.has_per_vertex_input) {
            continue;
        }
        if (cand_sig.exec_model != ref_sig.exec_model ||
            cand_sig.in_loc_mask != ref_sig.in_loc_mask ||
            cand_sig.out_loc_mask != ref_sig.out_loc_mask ||
            cand_sig.descs.size() != ref_sig.descs.size()) {
            continue;
        }

        bool locs_ok = true;
        for (u32 loc = 0; loc < 64u; ++loc) {
            if (cand_sig.in_loc_mask & (1ull << loc)) {
                if (cand_sig.in_interp[loc] != ref_sig.in_interp[loc] ||
                    cand_sig.in_comps[loc] == 0 ||
                    cand_sig.in_comps[loc] > ref_sig.in_comps[loc]) {
                    locs_ok = false;
                    break;
                }
            }
            if (cand_sig.out_loc_mask & (1ull << loc)) {
                if (cand_sig.out_comps[loc] == 0 ||
                    cand_sig.out_comps[loc] != ref_sig.out_comps[loc]) {
                    locs_ok = false;
                    break;
                }
            }
        }
        if (!locs_ok) {
            continue;
        }

        u32 base_binding = 0;
        if (!cand_sig.descs.empty()) {
            if (cand_sig.descs[0].binding == ref_sig.descs[0].binding) {
                base_binding = 0;
            } else if (cand_sig.descs[0].binding == 0 && ref_sig.descs[0].binding > 0) {
                base_binding = ref_sig.descs[0].binding;
            } else {
                continue;
            }
        }
        bool descs_ok = true;
        for (size_t i = 0; i < cand_sig.descs.size(); ++i) {
            if (cand_sig.descs[i].set != ref_sig.descs[i].set ||
                cand_sig.descs[i].binding + base_binding != ref_sig.descs[i].binding ||
                cand_sig.descs[i].kind_tag != ref_sig.descs[i].kind_tag) {
                descs_ok = false;
                break;
            }
        }
        if (!descs_ok) {
            continue;
        }

        // Exact interface match! Copy candidate SPIR-V and rebase Binding decorations if needed.
        out_spv.assign(entry.words.begin(), entry.words.end());
        if (base_binding != 0) {
            for (size_t pos = 5; pos < out_spv.size();) {
                const u32 wc = out_spv[pos] >> 16;
                const u32 op = out_spv[pos] & 0xFFFFu;
                if (wc == 0 || pos + wc > out_spv.size() || op == kOpFunction) {
                    break;
                }
                if (op == kOpDecorate && wc == 4 && out_spv[pos + 2] == kDecoBinding) {
                    out_spv[pos + 3] += base_binding;
                }
                pos += wc;
            }
        }
        return true;
    }

    return false;
}

} // namespace Shader::Backend::SPIRV
