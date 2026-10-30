#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace gx_direct_vk {

// Per-module bitmask for A/B runtime verification via BB_DIRECT_VULKAN and BB_DIRECT_VULKAN_MASK:
//   BB_DIRECT_VULKAN=0            -> disables all Direct Vulkan paths (100% fallback to PS4 GNM)
//   BB_DIRECT_VULKAN_MASK=0x1     -> enables only selected Direct Vulkan modules (V1..V5)
inline constexpr uint32_t kModuleV1_BufferCopyFill = 1u << 0; // Native vkCmdCopyBuffer / vkCmdFillBuffer (replaces PS4 g_EmbeddedCS_BufferCopy/ScatterCopy/ClearUint4)
inline constexpr uint32_t kModuleV2_DrawDispatch   = 1u << 1; // Direct Draw / Dispatch / State stream
inline constexpr uint32_t kModuleV3_ResourceViews  = 1u << 2; // Direct Texture / Buffer / RT lifecycle
inline constexpr uint32_t kModuleV4_NativeShaders  = 1u << 3; // Direct 752-shader SPIR-V pipeline cache
inline constexpr uint32_t kModuleV5_NativePcFx     = 1u << 4; // Native C++ PC effect toggles

inline uint32_t ActiveMask() noexcept {
    static const uint32_t mask = []() -> uint32_t {
        const char* env_en = std::getenv("BB_DIRECT_VULKAN");
        if (env_en && env_en[0] == '0' && env_en[1] == '\0') {
            return 0u;
        }
        const char* env_mask = std::getenv("BB_DIRECT_VULKAN_MASK");
        if (env_mask && env_mask[0] != '\0') {
            return static_cast<uint32_t>(std::strtoul(env_mask, nullptr, 0));
        }
        return 0xFFFFFFFFu;
    }();
    return mask;
}

inline bool EnabledV1() noexcept {
    return (ActiveMask() & kModuleV1_BufferCopyFill) != 0u;
}

inline constexpr uint32_t kV1Sub_SingleCopy          = 1u << 0; // 0x26b2670
inline constexpr uint32_t kV1Sub_ScatterGather       = 1u << 1; // 0x26b2810
inline constexpr uint32_t kV1Sub_UploadCpuToGpu      = 1u << 2; // 0x26b1c40
inline constexpr uint32_t kV1Sub_CopyRegion          = 1u << 3; // 0x26b2d30
inline constexpr uint32_t kV1Sub_ClearBufferSurf     = 1u << 4; // 0x26b5380
inline constexpr uint32_t kV1Sub_ClearHtileUnaligned = 1u << 5;  // 0x26b5f50 (unaligned HTILE)
inline constexpr uint32_t kV1Sub_ClearColorImage     = 1u << 6;  // 0x26b4ef0
inline constexpr uint32_t kV1Sub_CopyTextureRegion   = 1u << 7;  // 0x26b2f40
inline constexpr uint32_t kV1Sub_DecompressDepth     = 1u << 8;  // 0x26b6810
inline constexpr uint32_t kV1Sub_EliminateFastClear  = 1u << 9;  // 0x26b5a70
inline constexpr uint32_t kV1Sub_ResolveColor        = 1u << 10; // 0x26b55a0 (CB_COLOR_CONTROL.MODE = Resolve)
inline constexpr uint32_t kV1Sub_ResolveMsaaShader   = 1u << 11; // reserved (0x26b6cb0 is Swapchain Gamma-LUT Present Pass -> V3.3)
inline constexpr uint32_t kV1Sub_UploadTextureCpu    = 1u << 12; // 0x26b35e0 / 0x26b4230
inline constexpr uint32_t kV1Sub_ClearDepthDraw      = 1u << 13; // 0x26b5f50 (depth/stencil draw clear)

inline uint32_t ActiveV1SubMask() noexcept {
    static const uint32_t sub = []() -> uint32_t {
        const char* env_sub = std::getenv("BB_DIRECT_V1_MASK");
        if (env_sub && env_sub[0] != '\0') {
            return static_cast<uint32_t>(std::strtoul(env_sub, nullptr, 0));
        }
        return 0xFFFFFFFFu;
    }();
    return sub;
}

inline bool EnabledV1Sub(uint32_t bit) noexcept {
    return EnabledV1() && ((ActiveV1SubMask() & bit) != 0u);
}

inline bool EnabledV2() noexcept {
    return (ActiveMask() & kModuleV2_DrawDispatch) != 0u;
}

inline constexpr uint32_t kV2Sub_IndexAndPrim       = 1u << 0; // 0x26bba30 (CGIndexBufferCacheGnm_BindToGfxContext)
inline constexpr uint32_t kV2Sub_DrawDirect         = 1u << 1; // 0x21696c0 / 0x2169780 / 0x2169840 / 0x2169900
inline constexpr uint32_t kV2Sub_DrawIndirect       = 1u << 2; // 0x21699d0 / 0x2169ab0
inline constexpr uint32_t kV2Sub_Dispatch           = 1u << 3; // 0x21695c0 / 0x2169640 / 0x21a2270 / 0x21a2320
inline constexpr uint32_t kV2Sub_YebisDrawDispatch  = 1u << 4; // 0x21d5f80 (Yebis PostFX DrawIndex2 & LUT Dispatches)
inline constexpr uint32_t kV2Sub_SkipShaderPrefetch = 1u << 5; // CGShaderResourceBinderGnm: skip dummy 0x1073f50 DMA_DATA L2 prefetch (0x3022C)
inline constexpr uint32_t kV2Sub_NativeVsharp       = 1u << 6; // CGShaderResourceBinderGnm: native C++20 V# descriptors, active stages & PS usage
inline constexpr uint32_t kV2Sub_StageUserData      = 1u << 7; // CGShaderResourceBinderGnm: coalesced bitmasked stage SGPR packet (0 Razor NOPs)

inline uint32_t ActiveV2SubMask() noexcept {
    static const uint32_t sub = []() -> uint32_t {
        const char* env_sub = std::getenv("BB_DIRECT_V2_MASK");
        if (env_sub && env_sub[0] != '\0') {
            return static_cast<uint32_t>(std::strtoul(env_sub, nullptr, 0));
        }
        return 0xFFFFFFFFu;
    }();
    return sub;
}

inline bool EnabledV2Sub(uint32_t bit) noexcept {
    return EnabledV2() && ((ActiveV2SubMask() & bit) != 0u);
}

// Type-3 Op::Nop (0x10) host-stream tag recognized by BbCp::CommandProcessor -> Vulkan::Rasterizer
inline constexpr uint32_t kNopPayloadTag = 0xBB564B00u; // 'BBVK'

enum class Op : uint32_t {
    CopyBufferBatch          = 1u,
    FillBuffer               = 2u,
    ClearColorImage          = 3u,
    CopyTextureRegion        = 4u,
    DecompressDepth          = 5u,
    EliminateFastClear       = 6u,
    ResolveColor             = 7u,
    UploadTextureSubresource = 8u,
    ClearDepthStencil        = 9u,
    // V2.1: Direct Index/Topology State, Direct/Indirect Draw & Dispatch stream
    SetIndexState            = 10u,
    DrawIndexed              = 11u,
    DrawIndexAddr            = 12u,
    DrawAuto                 = 13u,
    DrawIndirect             = 14u,
    DispatchDirect           = 15u,
    DispatchIndirect         = 16u,
    // V2.2: Coalesced bitmasked stage user-data SGPR packet
    SetStageUserData         = 17u,
};

struct CopyRegion {
    uint64_t dst_va;
    uint64_t src_va;
    uint32_t byte_size;
    uint32_t reserved;
};
static_assert(sizeof(CopyRegion) == 24, "CopyRegion matches CoreGraphics2 CopyRecord (6 dwords)");

struct FillArgs {
    uint64_t dst_va;
    uint32_t byte_size;
    uint32_t fill_value;
};
static_assert(sizeof(FillArgs) == 16, "FillArgs is 4 dwords");

struct ClearColorArgs {
    uint64_t dst_va;
    uint32_t byte_size;
    uint32_t num_bits;
    uint32_t clear_rgba[4];
    uint64_t tsharp[4];
};
static_assert(sizeof(ClearColorArgs) == 64, "ClearColorArgs is 16 dwords");

struct CopyTextureRegionArgs {
    uint64_t srv_tsharp[4];
    uint64_t uav_tsharp[4];
    uint64_t dst_va;
    uint64_t src_va;
    uint32_t dst_byte_size;
    uint32_t src_byte_size;
    uint32_t src_x;
    uint32_t src_y;
    uint32_t src_z;
    uint32_t dst_x;
    uint32_t dst_y;
    uint32_t dst_z;
    uint32_t copy_w;
    uint32_t copy_h;
    uint32_t copy_d;
    uint32_t reserved;
};
static_assert(sizeof(CopyTextureRegionArgs) == 128, "CopyTextureRegionArgs is 32 dwords");

struct DecompressDepthArgs {
    uint64_t dst_va;
    uint32_t dst_byte_size;
    uint32_t db_view;
    uint32_t db_regs[8];
    uint32_t htile_data_base;
    uint32_t htile_surface;
    uint32_t cb_db_extent;
    uint32_t first_slice;
    uint32_t slice_count;
    uint32_t reserved;
};
static_assert(sizeof(DecompressDepthArgs) == 72, "DecompressDepthArgs is 18 dwords");

struct EliminateFastClearArgs {
    uint64_t dst_va;
    uint32_t dst_byte_size;
    uint32_t first_slice;
    uint32_t slice_count;
    uint32_t cb_regs[11];
};
static_assert(sizeof(EliminateFastClearArgs) == 64, "EliminateFastClearArgs is 16 dwords");

struct ResolveColorArgs {
    uint64_t dst_va;
    uint64_t src_va;
    uint32_t dst_byte_size;
    uint32_t src_byte_size;
    uint32_t src_cb_regs[11];
    uint32_t dst_cb_regs[11];
};
static_assert(sizeof(ResolveColorArgs) == 112, "ResolveColorArgs is 28 dwords");

struct UploadTextureArgs {
    uint64_t real_tsharp[4];
    uint64_t src_stage_va;
    uint64_t dst_va;
    uint32_t src_byte_size;
    uint32_t dst_byte_size;
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t offset_z;
    uint32_t extent_w;
    uint32_t extent_h;
    uint32_t extent_d;
    uint32_t buf_row_len;
    uint32_t buf_img_height;
    uint32_t reserved0;
    uint32_t reserved1;
};
static_assert(sizeof(UploadTextureArgs) == 96, "UploadTextureArgs is 24 dwords");

struct ClearDepthStencilArgs {
    uint64_t dst_va;
    uint32_t dst_byte_size;
    uint32_t db_view;
    uint32_t db_regs[8];
    uint32_t htile_data_base;
    uint32_t cb_db_extent;
    uint32_t first_slice;
    uint32_t slice_count;
    float    depth_clear;
    uint32_t stencil_and_flags;
};
static_assert(sizeof(ClearDepthStencilArgs) == 72, "ClearDepthStencilArgs is 18 dwords");

// Reserves `needed_dwords` in a Gnm DrawCommandBuffer (`+0x08` endptr, `+0x10` cmdptr, `+0x18` reserveCb, `+0x20` userData).
inline uint32_t* ReserveDcbDwords(void* dcb_raw, uint32_t needed_dwords) noexcept {
    auto* dcb = static_cast<uint8_t*>(dcb_raw);
    uintptr_t end_ptr = *reinterpret_cast<uintptr_t*>(dcb + 0x08);
    uintptr_t cmd_ptr = *reinterpret_cast<uintptr_t*>(dcb + 0x10);
    if (((end_ptr - cmd_ptr) >> 2) < static_cast<uintptr_t>(needed_dwords)) {
        using ReserveFn = bool (*)(void*, uint32_t, void*);
        auto fn = *reinterpret_cast<ReserveFn*>(dcb + 0x18);
        if (!fn || !fn(dcb, needed_dwords, *reinterpret_cast<void**>(dcb + 0x20))) {
            return nullptr;
        }
        end_ptr = *reinterpret_cast<uintptr_t*>(dcb + 0x08);
        cmd_ptr = *reinterpret_cast<uintptr_t*>(dcb + 0x10);
        if (((end_ptr - cmd_ptr) >> 2) < static_cast<uintptr_t>(needed_dwords)) {
            return nullptr;
        }
    }
    *reinterpret_cast<uintptr_t*>(dcb + 0x10) = cmd_ptr + (static_cast<uintptr_t>(needed_dwords) << 2);
    return reinterpret_cast<uint32_t*>(cmd_ptr);
}

inline constexpr uint32_t MakeNopHeader(uint32_t body_dwords) noexcept {
    // Type 3 (3 << 30) | ((body_dwords - 1) << 16) | (Op::Nop (0x10) << 8)
    return 0xC0001000u | ((body_dwords - 1u) << 16);
}

// Emits batched GPU buffer copies directly to Vulkan::Rasterizer::DirectCopyBuffers (up to 32 regions per packet = 195 dwords).
inline bool EmitCopyBufferBatch(void* dcb_raw, const CopyRegion* regions, uint32_t count) noexcept {
    if (!dcb_raw || !regions || count == 0u) {
        return true;
    }
    constexpr uint32_t kMaxRegionsPerPacket = 32u;
    uint32_t offset = 0u;
    while (offset < count) {
        const uint32_t batch = ((count - offset) < kMaxRegionsPerPacket) ? (count - offset) : kMaxRegionsPerPacket;
        const uint32_t body_dwords = 2u + batch * 6u; // [0]=kNopPayloadTag, [1]=Op|(batch<<16), [2..]=CopyRegion[batch]
        uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + body_dwords);
        if (!pkt) {
            return false;
        }
        pkt[0] = MakeNopHeader(body_dwords);
        pkt[1] = kNopPayloadTag;
        pkt[2] = static_cast<uint32_t>(Op::CopyBufferBatch) | (batch << 16);
        std::memcpy(&pkt[3], &regions[offset], static_cast<size_t>(batch) * sizeof(CopyRegion));
        offset += batch;
    }
    return true;
}

inline bool EmitCopyBuffer(void* dcb_raw, uint64_t dst_va, uint64_t src_va, uint64_t byte_size) noexcept {
    if (!dcb_raw || !dst_va || !src_va || byte_size == 0ULL) {
        return true;
    }
    // Split multi-GiB transfers into 256 MiB regions so uint32_t byte_size never overflows.
    constexpr uint64_t kMaxChunk = 0x10000000ULL;
    while (byte_size != 0ULL) {
        const uint32_t chunk = (byte_size > kMaxChunk) ? static_cast<uint32_t>(kMaxChunk) : static_cast<uint32_t>(byte_size);
        const CopyRegion reg{dst_va, src_va, chunk, 0u};
        if (!EmitCopyBufferBatch(dcb_raw, &reg, 1u)) {
            return false;
        }
        dst_va    += chunk;
        src_va    += chunk;
        byte_size -= chunk;
    }
    return true;
}

inline bool EmitFillBuffer(void* dcb_raw, uint64_t dst_va, uint64_t byte_size, uint32_t fill_value) noexcept {
    if (!dcb_raw || !dst_va || byte_size == 0ULL) {
        return true;
    }
    constexpr uint64_t kMaxChunk = 0x10000000ULL;
    while (byte_size != 0ULL) {
        const uint32_t chunk = (byte_size > kMaxChunk) ? static_cast<uint32_t>(kMaxChunk) : static_cast<uint32_t>(byte_size);
        constexpr uint32_t kBodyDwords = 2u + 4u; // [0]=tag, [1]=Op::FillBuffer, [2..5]=FillArgs
        uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + kBodyDwords);
        if (!pkt) {
            return false;
        }
        pkt[0] = MakeNopHeader(kBodyDwords);
        pkt[1] = kNopPayloadTag;
        pkt[2] = static_cast<uint32_t>(Op::FillBuffer);
        const FillArgs args{dst_va, chunk, fill_value};
        std::memcpy(&pkt[3], &args, sizeof(FillArgs));
        dst_va    += chunk;
        byte_size -= chunk;
    }
    return true;
}

inline bool EmitClearColorImage(void* dcb_raw, const ClearColorArgs& args) noexcept {
    if (!dcb_raw || !args.dst_va || args.byte_size == 0u) {
        return true;
    }
    constexpr uint32_t kBodyDwords = 2u + 16u; // [0]=tag, [1]=Op::ClearColorImage, [2..17]=ClearColorArgs
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + kBodyDwords);
    if (!pkt) {
        return false;
    }
    pkt[0] = MakeNopHeader(kBodyDwords);
    pkt[1] = kNopPayloadTag;
    pkt[2] = static_cast<uint32_t>(Op::ClearColorImage);
    std::memcpy(&pkt[3], &args, sizeof(ClearColorArgs));
    return true;
}

inline bool EmitCopyTextureRegion(void* dcb_raw, const CopyTextureRegionArgs& args) noexcept {
    if (!dcb_raw || args.copy_w == 0u || args.copy_h == 0u || args.copy_d == 0u) {
        return true;
    }
    constexpr uint32_t kBodyDwords = 2u + 32u; // [0]=tag, [1]=Op::CopyTextureRegion, [2..33]=CopyTextureRegionArgs
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + kBodyDwords);
    if (!pkt) {
        return false;
    }
    pkt[0] = MakeNopHeader(kBodyDwords);
    pkt[1] = kNopPayloadTag;
    pkt[2] = static_cast<uint32_t>(Op::CopyTextureRegion);
    std::memcpy(&pkt[3], &args, sizeof(CopyTextureRegionArgs));
    return true;
}

template <Op kOp, typename ArgsT>
inline bool EmitFixedPacket(void* dcb_raw, const ArgsT& args) noexcept {
    static_assert((sizeof(ArgsT) % 4u) == 0u, "ArgsT must be dword-aligned");
    if (!dcb_raw) {
        return false;
    }
    constexpr uint32_t kBodyDwords = 2u + static_cast<uint32_t>(sizeof(ArgsT) / 4u);
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + kBodyDwords);
    if (!pkt) {
        return false;
    }
    pkt[0] = MakeNopHeader(kBodyDwords);
    pkt[1] = kNopPayloadTag;
    pkt[2] = static_cast<uint32_t>(kOp);
    std::memcpy(&pkt[3], &args, sizeof(ArgsT));
    return true;
}

inline bool EmitDecompressDepth(void* dcb_raw, const DecompressDepthArgs& args) noexcept {
    return EmitFixedPacket<Op::DecompressDepth>(dcb_raw, args);
}

inline bool EmitEliminateFastClear(void* dcb_raw, const EliminateFastClearArgs& args) noexcept {
    return EmitFixedPacket<Op::EliminateFastClear>(dcb_raw, args);
}

inline bool EmitResolveColor(void* dcb_raw, const ResolveColorArgs& args) noexcept {
    return EmitFixedPacket<Op::ResolveColor>(dcb_raw, args);
}

inline bool EmitUploadTextureSubresource(void* dcb_raw, const UploadTextureArgs& args) noexcept {
    return EmitFixedPacket<Op::UploadTextureSubresource>(dcb_raw, args);
}

inline bool EmitClearDepthStencil(void* dcb_raw, const ClearDepthStencilArgs& args) noexcept {
    return EmitFixedPacket<Op::ClearDepthStencil>(dcb_raw, args);
}

// ============================================================================
// V2.1: Direct Index/Topology State, Direct/Indirect Draw & Dispatch Stream
// ============================================================================

inline constexpr uint32_t kIndexFlag_SetPrim      = 1u << 8;
inline constexpr uint32_t kIndexFlag_SetIbAddr    = 1u << 9;
inline constexpr uint32_t kIndexFlag_SetIndexSize = 1u << 10;

struct SetIndexStateArgs {
    uint64_t ib_va;
    uint32_t prim_type;
    uint32_t index_and_flags; // bits[0..7]=index_size (0=16b,1=32b), bits[8..10]=kIndexFlag_*
};
static_assert(sizeof(SetIndexStateArgs) == 16, "SetIndexStateArgs is 4 dwords");

struct DrawIndexedArgs {
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t start_index;
    int32_t  base_vertex;
};
static_assert(sizeof(DrawIndexedArgs) == 16, "DrawIndexedArgs is 4 dwords");

struct DrawIndexAddrArgs {
    uint64_t ib_va;
    uint32_t index_count;
    uint32_t prim_and_size; // bits[0..5]=prim_type, bit 8=set_prim, bit 9=set_index_size, bits[16..23]=index_size
};
static_assert(sizeof(DrawIndexAddrArgs) == 16, "DrawIndexAddrArgs is 4 dwords");

struct DrawAutoArgs {
    uint32_t vertex_count;
    uint32_t instance_count;
    int32_t  start_vertex;
};
static_assert(sizeof(DrawAutoArgs) == 12, "DrawAutoArgs is 3 dwords");

struct DrawIndirectArgs {
    uint64_t args_va;
    uint32_t byte_offset;
    uint32_t is_indexed;
};
static_assert(sizeof(DrawIndirectArgs) == 16, "DrawIndirectArgs is 4 dwords");

struct DispatchDirectArgs {
    uint32_t group_x;
    uint32_t group_y;
    uint32_t group_z;
};
static_assert(sizeof(DispatchDirectArgs) == 12, "DispatchDirectArgs is 3 dwords");

struct DispatchIndirectArgs {
    uint64_t args_va;
    uint32_t byte_offset;
    uint32_t reserved;
};
static_assert(sizeof(DispatchIndirectArgs) == 16, "DispatchIndirectArgs is 4 dwords");

// Preserves sce::Gnm::DrawCommandBuffer::m_flags bit 0 (+0x28) for ZPass SET_PREDICATION.
inline uint32_t GetDcbPredicationBit(const void* dcb_raw) noexcept {
    return (*reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(dcb_raw) + 0x28)) & 1u;
}

template <Op kOp, typename ArgsT>
inline bool EmitPredicatedPacket(void* dcb_raw, const ArgsT& args) noexcept {
    static_assert((sizeof(ArgsT) % 4u) == 0u, "ArgsT must be dword-aligned");
    if (!dcb_raw) {
        return false;
    }
    constexpr uint32_t kBodyDwords = 2u + static_cast<uint32_t>(sizeof(ArgsT) / 4u);
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + kBodyDwords);
    if (!pkt) {
        return false;
    }
    pkt[0] = MakeNopHeader(kBodyDwords) | GetDcbPredicationBit(dcb_raw);
    pkt[1] = kNopPayloadTag;
    pkt[2] = static_cast<uint32_t>(kOp);
    std::memcpy(&pkt[3], &args, sizeof(ArgsT));
    return true;
}

inline bool EmitSetIndexState(void* dcb_raw, const SetIndexStateArgs& args) noexcept {
    if ((args.index_and_flags & (kIndexFlag_SetPrim | kIndexFlag_SetIbAddr | kIndexFlag_SetIndexSize)) == 0u) {
        return true;
    }
    return EmitFixedPacket<Op::SetIndexState>(dcb_raw, args);
}

inline bool EmitDrawIndexed(
    void* dcb_raw,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t start_index,
    int32_t base_vertex) noexcept
{
    const DrawIndexedArgs args{index_count, instance_count, start_index, base_vertex};
    return EmitPredicatedPacket<Op::DrawIndexed>(dcb_raw, args);
}

inline bool EmitDrawIndexAddr(
    void* dcb_raw,
    uint64_t ib_va,
    uint32_t index_count,
    uint32_t prim_type,
    uint32_t index_size) noexcept
{
    const uint32_t flags = (prim_type & 0x3Fu) | 0x100u | 0x200u | ((index_size & 0xFFu) << 16);
    const DrawIndexAddrArgs args{ib_va, index_count, flags};
    return EmitPredicatedPacket<Op::DrawIndexAddr>(dcb_raw, args);
}

inline bool EmitDrawAuto(
    void* dcb_raw,
    uint32_t vertex_count,
    uint32_t instance_count,
    int32_t start_vertex) noexcept
{
    const DrawAutoArgs args{vertex_count, instance_count, start_vertex};
    return EmitPredicatedPacket<Op::DrawAuto>(dcb_raw, args);
}

inline bool EmitDrawIndirect(
    void* dcb_raw,
    uint64_t args_va,
    uint32_t byte_offset,
    bool is_indexed) noexcept
{
    const DrawIndirectArgs args{args_va, byte_offset, is_indexed ? 1u : 0u};
    return EmitPredicatedPacket<Op::DrawIndirect>(dcb_raw, args);
}

inline bool EmitDispatchDirect(
    void* dcb_raw,
    uint32_t group_x,
    uint32_t group_y,
    uint32_t group_z) noexcept
{
    const DispatchDirectArgs args{group_x, group_y, group_z};
    return EmitPredicatedPacket<Op::DispatchDirect>(dcb_raw, args);
}

inline bool EmitDispatchIndirect(
    void* dcb_raw,
    uint64_t args_va,
    uint32_t byte_offset) noexcept
{
    const DispatchIndirectArgs args{args_va, byte_offset, 0u};
    return EmitPredicatedPacket<Op::DispatchIndirect>(dcb_raw, args);
}

// ============================================================================
// V2.2: Native C++20 V# Descriptor Builders, Active Stage / PS Input Linkage,
//       and Coalesced Bitmasked Stage User-Data SGPR Emitter
// ============================================================================

// Verified against eboot.bin file offset 0x48d55c0 (VMA 0x48d15c0) minus Bank::Sh (0x2C00):
//   0: CS=0x240, 1: PS=0x00C, 2: VS=0x04C, 3: GS=0x08C, 4: ES=0x0CC, 5: HS=0x10C, 6: LS=0x14C
inline constexpr uint16_t kShStageUserDataBase[8] = {
    0x0240u, 0x000Cu, 0x004Cu, 0x008Cu, 0x00CCu, 0x010Cu, 0x014Cu, 0x0000u
};

// Direct SET_SH_REG fallback without Razor NOP packets when dst_reg + count > 16.
inline void EmitSetShRegDirect(
    void* dcb_raw,
    uint32_t gnm_stage,
    uint32_t dst_reg,
    const uint32_t* dwords,
    uint32_t count) noexcept
{
    if (!dcb_raw || count == 0u) {
        return;
    }
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 2u + count);
    if (!pkt) {
        return;
    }
    const uint32_t shader_type_bit =
        (*reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(dcb_raw) + 0x28)) & 2u;
    pkt[0] = 0xC0007600u | (count << 16) | shader_type_bit;
    pkt[1] = static_cast<uint32_t>(kShStageUserDataBase[gnm_stage & 7u]) + dst_reg;
    std::memcpy(&pkt[2], dwords, count * sizeof(uint32_t));
}

struct StageUserDataBatch {
    uint32_t vals[16];
    uint32_t mask = 0u;

    inline void SetDwords(void* dcb_raw, uint32_t gnm_stage, uint32_t dst_reg, const void* src, uint32_t count) noexcept {
        if (dst_reg + count <= 16u) {
            std::memcpy(&vals[dst_reg], src, count * sizeof(uint32_t));
            mask |= ((1u << count) - 1u) << dst_reg;
        } else {
            EmitSetShRegDirect(dcb_raw, gnm_stage, dst_reg, static_cast<const uint32_t*>(src), count);
        }
    }

    inline void SetPtr(void* dcb_raw, uint32_t gnm_stage, uint32_t dst_reg, const void* ptr) noexcept {
        const uint64_t va = reinterpret_cast<uint64_t>(ptr);
        const uint32_t dw[2] = {static_cast<uint32_t>(va), static_cast<uint32_t>(va >> 32)};
        SetDwords(dcb_raw, gnm_stage, dst_reg, dw, 2u);
    }

    inline void SetDword(void* dcb_raw, uint32_t gnm_stage, uint32_t dst_reg, uint32_t val) noexcept {
        SetDwords(dcb_raw, gnm_stage, dst_reg, &val, 1u);
    }

    inline void Flush(void* dcb_raw, uint32_t gnm_stage) noexcept {
        if (mask == 0u || !dcb_raw) {
            return;
        }
        const uint32_t count = static_cast<uint32_t>(__builtin_popcount(mask));
        const uint32_t body_dwords = 3u + count;
        uint32_t* pkt = ReserveDcbDwords(dcb_raw, 1u + body_dwords);
        if (!pkt) {
            mask = 0u;
            return;
        }
        const uint32_t sh_base = static_cast<uint32_t>(kShStageUserDataBase[gnm_stage & 7u]);
        pkt[0] = MakeNopHeader(body_dwords);
        pkt[1] = kNopPayloadTag;
        pkt[2] = static_cast<uint32_t>(Op::SetStageUserData);
        pkt[3] = sh_base | ((mask & 0xFFFFu) << 16);
        uint32_t out_idx = 4u;
        uint32_t m = mask & 0xFFFFu;
        while (m != 0u) {
            const uint32_t bit = static_cast<uint32_t>(__builtin_ctz(m));
            m &= (m - 1u);
            pkt[out_idx++] = vals[bit];
        }
        mask = 0u;
    }
};

inline void EmitSinglePtrUserData(void* dcb_raw, uint32_t gnm_stage, uint32_t dst_reg, const void* ptr) noexcept {
    StageUserDataBatch b{};
    b.SetPtr(dcb_raw, gnm_stage, dst_reg, ptr);
    b.Flush(dcb_raw, gnm_stage);
}

// 1-to-1 C++20 replacement for libSceGnm V# Buffer helpers:
//   0x1070710 setBaseAddress, 0x1070730 setStride, 0x10707b0 setNumElements,
//   0x1070960 initAsConstantBuffer, 0x10710c0 setResourceMemoryType, 0x1071180 getBaseAddress.
inline uint64_t Vsharp_GetBaseAddress(const void* vsharp_ptr) noexcept {
    const auto* v = static_cast<const uint32_t*>(vsharp_ptr);
    return static_cast<uint64_t>(v[0]) | (static_cast<uint64_t>(v[1] & 0xFFFu) << 32);
}

inline void Vsharp_SetBaseAddress(void* vsharp_ptr, uint64_t addr) noexcept {
    auto* v = static_cast<uint32_t*>(vsharp_ptr);
    v[0] = static_cast<uint32_t>(addr);
    v[1] = (v[1] & 0xFFFFF000u) | (static_cast<uint32_t>(addr >> 32) & 0xFFFu);
}

inline void Vsharp_SetStride(void* vsharp_ptr, uint32_t stride) noexcept {
    auto* v = static_cast<uint32_t*>(vsharp_ptr);
    v[1] = (v[1] & 0xC000FFFFu) | ((stride << 16) & 0x3FFF0000u);
}

inline void Vsharp_SetNumElements(void* vsharp_ptr, uint32_t num_elements) noexcept {
    static_cast<uint32_t*>(vsharp_ptr)[2] = num_elements;
}

inline void Vsharp_InitAsConstantBuffer(void* vsharp_ptr, const void* addr_ptr, uint32_t byte_size) noexcept {
    const uint64_t addr = reinterpret_cast<uint64_t>(addr_ptr);
    auto* v = static_cast<uint32_t*>(vsharp_ptr);
    v[0] = static_cast<uint32_t>(addr);
    v[1] = (static_cast<uint32_t>(addr >> 32) & 0xFFFu) | 0x00100000u;
    v[2] = static_cast<uint32_t>(byte_size + 15u) >> 4;
    v[3] = 0x20077FACu;
}

inline void Vsharp_SetResourceMemoryType(
    void* vsharp_ptr,
    uint32_t mtype,
    uint32_t l1_cache_bypass,
    uint32_t kcache_enable) noexcept
{
    auto* v = static_cast<uint32_t*>(vsharp_ptr);
    const uint32_t d_hi = ((mtype >> 2) & 0x4u) | ((l1_cache_bypass != 0u) ? 0x3u : 0x0u);
    const uint32_t w1_base = (v[1] & 0xFFFF0FFFu) | ((mtype & 0x3u) << 14);
    v[3] = (v[3] & 0xC7FFFFFFu) | (d_hi << 27);
    v[1] = w1_base | ((kcache_enable != 0u) ? 0x3000u : 0x0u);
}

// 1-to-1 C++20 replacement for 0x1076030 (DrawCommandBuffer::setActiveShaderStages)
inline void EmitSetActiveShaderStages(void* dcb_raw, uint32_t stages) noexcept {
    if (stages != 0x00u && stages != 0x45u && stages != 0xADu &&
        stages != 0xB0u && stages != 0x200u) {
        return;
    }
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 3u);
    if (!pkt) {
        return;
    }
    pkt[0] = 0xC0016900u;
    pkt[1] = 0x000002D5u; // VGT_SHADER_STAGES_EN
    pkt[2] = stages;
}

// Fused 1-to-1 C++20 replacement for 0x10873a0 (generatePsShaderUsageTable) + 0x1075000 (setPsShaderUsage)
inline void GenerateAndEmitPsShaderUsage(
    void* dcb_raw,
    const void* vs_exports_raw,
    uint32_t num_vs_exports,
    const void* ps_inputs_raw,
    uint32_t num_ps_inputs) noexcept
{
    if (!dcb_raw || num_ps_inputs == 0u) {
        return;
    }
    uint32_t* pkt = ReserveDcbDwords(dcb_raw, 2u + num_ps_inputs);
    if (!pkt) {
        return;
    }
    pkt[0] = 0xC0006900u | (num_ps_inputs << 16);
    pkt[1] = 0x00000191u; // SPI_PS_INPUT_CNTL_0..31

    if (num_ps_inputs <= 32u) {
        const auto* vs_exp = static_cast<const uint8_t*>(vs_exports_raw);
        const auto* ps_in_u16 = static_cast<const uint16_t*>(ps_inputs_raw);
        const auto* ps_in_u8 = static_cast<const uint8_t*>(ps_inputs_raw);
        uint32_t* dst = &pkt[2];
        for (uint32_t i = 0u; i < num_ps_inputs; ++i) {
            uint32_t match_val = 0x20u;
            const uint8_t target_sem = ps_in_u8[i * 2u];
            for (uint32_t k = 0u; k < num_vs_exports; ++k) {
                if (vs_exp[k * 2u] == target_sem) {
                    match_val = vs_exp[k * 2u + 1u];
                    break;
                }
            }
            const uint32_t ps_w = ps_in_u16[i];
            const uint32_t lo6 = (((ps_w >> 7) & 0x1E0u) | match_val) & 0x3Fu;
            const uint32_t mid2 = lo6 | (((ps_w >> 8) & 0x3u) << 8);
            const uint32_t bit10 = (((ps_w >> 12) | (ps_w >> 10)) & 0x1u) << 10;
            dst[i] = mid2 | bit10;
        }
    }
}

} // namespace gx_direct_vk
