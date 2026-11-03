// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Priority Tier 8/10: 04_SceneRenderer
// File: GXYebisBloomAndAntiAliasBanks.cpp
// VMA Range: 0x21dfd20 .. 0x21e6820 (35 functions, 28,465 bytes)
// ============================================================================
// Architectural Description:
//   Implements two Yebis2 Post-Processing Parameter Bank Managers in the
//   Dantelion2 SceneRenderer (processed strictly Right-to-Left):
//     1. GXYebisBloomParamBank (0x21e3750 .. 0x21e6820, 13 functions):
//        Manages "Bloom[Yebis]" / "Glare" / "Star" / "Afterimage" /
//        "AnamorphicLens" / "Bloom" parameters (scene slot +0x3c, default
//        entry at +0x2d0, active/backup parameter blocks at +0x70 / +0x140).
//        Note: The 6 hash-table helpers (0x21e6fc0..0x21e8410) and 3 callback
//        methods (0x21e6f60..0x21e6f90) are defined in
//        GXYebisDofAndColorGradingBanks.cpp.
//     2. GXYebisAntiAliasParamBank (0x21dfd20 .. 0x21e3200, 22 functions):
//        Manages "Anti-Alias[Yebis]" ("FXAA" / "DLAA") parameters (scene slot
//        +0x4c, default entry at +0x168, active/backup parameter blocks at
//        +0x68 / +0x8c) along with its two-level intrusive hash-table helpers.
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace {

// ----------------------------------------------------------------------------
// Core Engine VMA Call Helpers
// ----------------------------------------------------------------------------
template <typename Ret, typename... Args>
inline Ret CallVma(uintptr_t vma, Args... args) {
    return reinterpret_cast<Ret (*)(Args...)>(GX_VMA(vma))(args...);
}

inline uint8_t* PtrAt(void* base, ptrdiff_t offset) {
    return reinterpret_cast<uint8_t*>(base) + offset;
}

inline const uint8_t* PtrAt(const void* base, ptrdiff_t offset) {
    return reinterpret_cast<const uint8_t*>(base) + offset;
}

inline uint64_t& QwordAt(void* base, ptrdiff_t offset) {
    return *reinterpret_cast<uint64_t*>(PtrAt(base, offset));
}

inline uint32_t& DwordAt(void* base, ptrdiff_t offset) {
    return *reinterpret_cast<uint32_t*>(PtrAt(base, offset));
}

inline uint16_t& WordAt(void* base, ptrdiff_t offset) {
    return *reinterpret_cast<uint16_t*>(PtrAt(base, offset));
}

inline uint8_t& ByteAt(void* base, ptrdiff_t offset) {
    return *reinterpret_cast<uint8_t*>(PtrAt(base, offset));
}

inline float& FloatAt(void* base, ptrdiff_t offset) {
    return *reinterpret_cast<float*>(PtrAt(base, offset));
}

// ----------------------------------------------------------------------------
// Allocator Virtual Table Helpers
// ----------------------------------------------------------------------------
inline void* AllocatorAlloc(void* allocator, size_t size, size_t align) {
    void* vtbl = *reinterpret_cast<void**>(allocator);
    auto fn = *reinterpret_cast<void* (**)(void*, size_t, size_t)>(PtrAt(vtbl, 0x58));
    return fn(allocator, size, align);
}

inline void AllocatorFree(void* allocator, void* ptr) {
    void* vtbl = *reinterpret_cast<void**>(allocator);
    auto fn = *reinterpret_cast<void (**)(void*, void*)>(PtrAt(vtbl, 0x70));
    fn(allocator, ptr);
}

inline void AllocatorQueryCaps(void* allocator, void* out_caps) {
    void* vtbl = *reinterpret_cast<void**>(allocator);
    auto fn = *reinterpret_cast<void (**)(void*, void*, uint32_t)>(PtrAt(vtbl, 0x20));
    fn(out_caps, allocator, 0);
}

inline void InvokeVirtualDtor0(void* obj) {
    void* vtbl = *reinterpret_cast<void**>(obj);
    auto fn = *reinterpret_cast<void (**)(void*)>(vtbl);
    fn(obj);
}

// ----------------------------------------------------------------------------
// Schrage's Method Park-Miller LCG Hash (0xdeadbeef ^ key, div 0x1f31d)
// ----------------------------------------------------------------------------
inline uint64_t ComputeYebisParamHash(uint32_t key) {
    int64_t x = static_cast<int64_t>(0xdeadbeefULL ^ static_cast<uint64_t>(key));
    int64_t q = x / 0x1f31d;
    int64_t r = x % 0x1f31d;
    int64_t h = r * 0x41a7 + q * (-2836);
    if (h < 0) {
        h += 0x7fffffffffffffffLL;
    }
    return static_cast<uint64_t>(h);
}

inline uint64_t MapHashToBucket(uint64_t hash, uint64_t mask, uint64_t max_buckets) {
    uint64_t bucket = hash & mask;
    if (max_buckets <= bucket) {
        bucket += ~(mask >> 1);
    }
    return bucket;
}

// ----------------------------------------------------------------------------
// Debug GUI Property Descriptor Helper Struct (56 bytes on stack)
// ----------------------------------------------------------------------------
struct GXDebugGuiPropertyDesc {
    void* getter_vtbl;       // +0x00
    void* value_ptr;         // +0x08
    uint8_t prop_type;       // +0x10
    uint8_t pad11[3];        // +0x11
    uint32_t flags;          // +0x14
    uint8_t enabled;         // +0x18
    uint8_t pad19[7];        // +0x19
    void* setter_vtbl;       // +0x20
    uint32_t min_bits;       // +0x28
    uint32_t max_bits;       // +0x2c
    uint32_t step_bits;      // +0x30
    uint32_t default_bits;   // +0x34
};

inline void RegisterGuiScalarProp(
    void* gui_mgr,
    uint32_t group_id,
    uintptr_t name_wstr_vma,
    uintptr_t desc_wstr_vma,
    void* value_ptr,
    uint8_t prop_type,
    uint32_t flags,
    uintptr_t getter_vtbl_vma,
    uintptr_t setter_vtbl_vma,
    uint32_t min_bits,
    uint32_t max_bits,
    uint32_t step_bits,
    uint32_t default_bits,
    uint32_t stack_arg0) {
    GXDebugGuiPropertyDesc desc{};
    desc.getter_vtbl = reinterpret_cast<void*>(getter_vtbl_vma);
    desc.value_ptr = value_ptr;
    desc.prop_type = prop_type;
    desc.flags = flags;
    desc.enabled = 1;
    desc.setter_vtbl = reinterpret_cast<void*>(setter_vtbl_vma);
    desc.min_bits = min_bits;
    desc.max_bits = max_bits;
    desc.step_bits = step_bits;
    desc.default_bits = default_bits;
    CallVma<void, void*, uint32_t, const void*, const void*, void*, uint32_t, uint32_t>(
        0xe327d0,
        gui_mgr,
        group_id,
        reinterpret_cast<const void*>(name_wstr_vma),
        reinterpret_cast<const void*>(desc_wstr_vma),
        &desc,
        1u,
        stack_arg0);
}

// Helper for releasing a refcounted DLFile / ParamRes handle
inline void ReleaseParamFileHandle(void* handle) {
    if (!handle) {
        return;
    }
    if (__atomic_sub_fetch(&DwordAt(handle, 0x38), 1u, __ATOMIC_ACQ_REL) == 0) {
        void* str_alloc = *reinterpret_cast<void**>(GX_VMA(0x5540e10ULL));
        if (QwordAt(handle, 0x20) >= 8) {
            AllocatorFree(reinterpret_cast<void*>(QwordAt(handle, 0x28)),
                          reinterpret_cast<void*>(QwordAt(handle, 0x08)));
        }
        QwordAt(handle, 0x20) = 7;
        QwordAt(handle, 0x18) = 0;
        WordAt(handle, 0x08) = 0;
        AllocatorFree(str_alloc, handle);
    }
}

// Helper for releasing a refcounted container item (15-bit weak/strong refcount at +0x08)
inline void ReleaseContainerObject(void* obj) {
    if (!obj) {
        return;
    }
    uint32_t old_val = DwordAt(obj, 0x08);
    uint32_t dec_low = (old_val + 0x7fffU) & 0x7fffU;
    uint32_t new_val = (old_val & 0xffff8000U) | dec_low;
    DwordAt(obj, 0x08) = new_val;
    if (dec_low == 0 && (old_val & 0x40000000U) != 0) {
        void* vtbl = *reinterpret_cast<void**>(obj);
        auto destroy_fn = *reinterpret_cast<void (**)(void*)>(PtrAt(vtbl, 0x18));
        destroy_fn(obj);
        DwordAt(obj, 0x08) &= 0xbfffffffU;
        new_val = DwordAt(obj, 0x08);
    }
    uint32_t high_cnt = ((new_val >> 15) + 0x7fffU) & 0x7fffU;
    uint32_t updated = (new_val & 0xc0007fffU) | (high_cnt << 15);
    DwordAt(obj, 0x08) = updated;
    if (high_cnt == 0) {
        DwordAt(obj, 0x08) = new_val | 0x3fff8000U;
        void* vtbl = *reinterpret_cast<void**>(obj);
        auto delete_fn = *reinterpret_cast<void (**)(void*)>(PtrAt(vtbl, 0x20));
        delete_fn(obj);
    }
}

struct YebisBucketIter {
    void* proxy0;
    void* proxy1;
    void* node;
};

} // namespace

extern "C" {

// Forward declarations of Bloom hash-table helpers defined in GXYebisDofAndColorGradingBanks.cpp
void* GXYebisBloomInnerHashTable_EraseNode(void* out_iter, void* table, void* node) __asm__("sub_21e6fc0");
void* GXYebisBloomOuterHashTable_EraseNode(void* out_iter, void* table, void* node) __asm__("sub_21e7130");
void* GXYebisBloomOuterHashTable_InsertNode(
    void* out_pair, void* table, void* key_ptr, YebisBucketIter node_iter) __asm__("sub_21e76a0");
void* GXYebisBloomInnerHashTable_InsertNode(
    void* out_pair, void* table, void* key_ptr, YebisBucketIter node_iter) __asm__("sub_21e7ab0");
void* GXYebisBloomOuterHashTable_RehashOrInsertRange(
    void* out_iter, void* table_vec_base, YebisBucketIter where_iter,
    uint64_t count, const YebisBucketIter* fill_val) __asm__("sub_21e7ec0");
void* GXYebisBloomInnerHashTable_RehashOrInsertRange(
    void* out_iter, void* table_vec_base, YebisBucketIter where_iter,
    uint64_t count, const YebisBucketIter* fill_val) __asm__("sub_21e8410");
void GXDebugGui_RegisterEnumProperty(
    void* gui_mgr, uint32_t group_id, const char16_t* name, const char16_t* desc,
    void* value_ptr, uint32_t enum_count, const void* enum_table, uint32_t flags) __asm__("sub_21e72a0");

// Forward declarations within this translation unit
void GXYebisBloomParamBank_ClearAllEntries(void* self);
void GXYebisBloomParamBank_ReinitializeAllEntries(void* self);
void GXYebisBloomParamBank_SaveAllEntries(void* self);

// ============================================================================
// PART 1: GXYebisBloomParamBank (0x21e3750 .. 0x21e6820, 13 functions)
// ============================================================================

// 0x21e6820 (1,708 B) - GXYebisBloomParamBank_ReloadParamFiles
void GXYebisBloomParamBank_ReloadParamFiles(void* self) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x3c);
    uint32_t param_id = DwordAt(scene_mgr, static_cast<ptrdiff_t>(slot) * 4 + 0x588);

    CallVma<void, void*, const void*, uint32_t>(
        0x2936030, PtrAt(self, 0x218), reinterpret_cast<const void*>(GX_VMA(0x49cbebeULL)), param_id);

    void* file_alloc = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    void* loader_obj = AllocatorAlloc(file_alloc, 0x178, 8);
    if (loader_obj) {
        const void* str_ptr = PtrAt(self, 0x220);
        if (QwordAt(self, 0x238) >= 8) {
            str_ptr = *reinterpret_cast<const void* const*>(str_ptr);
        }
        uint64_t ctor_arg[2] = {0, reinterpret_cast<uint64_t>(str_ptr)};
        CallVma<void, void*, void*>(0x2362cf0, loader_obj, ctor_arg);
    }

    void* file_sys_root = *reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL));
    void* file_sys_inner = *reinterpret_cast<void**>(PtrAt(file_sys_root, 0x10));
    void* cur_host = *reinterpret_cast<void**>(PtrAt(file_sys_inner, 0x18));

    struct BindConfig {
        uint16_t a0;
        uint16_t a1;
        uint16_t a2;
        uint16_t a3;
        uint64_t flags;
    } bind_cfg{0, 0, 0x12c, 0x190, 0x51502ULL};

    if (loader_obj && (ByteAt(loader_obj, 0x0b) & 0x40) != 0 && ByteAt(loader_obj, 0x6f) == 0 &&
        cur_host != loader_obj && cur_host != nullptr && (ByteAt(cur_host, 0x0b) & 0x40) != 0) {
        void* host_vtbl = *reinterpret_cast<void**>(cur_host);
        auto check_fn = *reinterpret_cast<int (**)(void*, void*)>(PtrAt(host_vtbl, 0x28));
        if (check_fn(cur_host, loader_obj) != 0) {
            CallVma<void, void*, void*, void*, void*>(
                0x2328e40,
                loader_obj,
                cur_host,
                reinterpret_cast<void*>(QwordAt(cur_host, 0x20)),
                &bind_cfg);
        }
    }

    uint64_t reader_ctx[4]{};
    CallVma<void, void*, void*>(0x236c4d0, reader_ctx, loader_obj);

    uint64_t default_path_pkt[2]{};
    CallVma<void, void*, const void*>(
        0x2ba7d40, default_path_pkt, reinterpret_cast<const void*>(GX_VMA(0x49d27c0ULL)));

    uint64_t open_arg[2]{};
    void* ref_handle = nullptr;
    if (default_path_pkt[1] != 0) {
        open_arg[1] = default_path_pkt[1];
    } else if (default_path_pkt[0] != 0) {
        open_arg[0] = default_path_pkt[0];
        DwordAt(reinterpret_cast<void*>(default_path_pkt[0]), 0x38) += 1;
        ref_handle = reinterpret_cast<void*>(default_path_pkt[0]);
    }

    void* loaded_res = CallVma<void*, void*, void*, uint32_t>(0x236c900, reader_ctx, open_arg, 0);
    ReleaseParamFileHandle(ref_handle);

    if (loaded_res) {
        CallVma<void, void*, void*>(0xe700c0, PtrAt(self, 0x2d0), reader_ctx);
        void** cur_end = reinterpret_cast<void**>(reader_ctx[2]);
        if (reinterpret_cast<void**>(reader_ctx[1]) != cur_end) {
            void* item = *(cur_end - 1);
            if (item) {
                ReleaseContainerObject(item);
                *(cur_end - 1) = nullptr;
                cur_end = reinterpret_cast<void**>(reader_ctx[2]);
            }
            reader_ctx[2] = reinterpret_cast<uint64_t>(cur_end - 1);
        }
    }
    ReleaseParamFileHandle(reinterpret_cast<void*>(default_path_pkt[0]));

    // Lookup outer bucket for param_id
    uint64_t hash = ComputeYebisParamHash(param_id);
    uint64_t bucket = MapHashToBucket(hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
    void* outer_node = *reinterpret_cast<void**>(PtrAt(outer_buckets, static_cast<ptrdiff_t>(bucket * 48 + 0x10)));

    if (outer_node != outer_head) {
        void* bucket_end_ptr = *reinterpret_cast<void**>(
            PtrAt(outer_buckets, static_cast<ptrdiff_t>(((bucket * 2) | 1) * 24 + 0x10)));
        void* bucket_end = *reinterpret_cast<void**>(bucket_end_ptr);
        while (outer_node != bucket_end && DwordAt(outer_node, 0x10) < param_id) {
            outer_node = *reinterpret_cast<void**>(outer_node);
        }
        if (outer_node == bucket_end || param_id < DwordAt(outer_node, 0x10)) {
            outer_node = outer_head;
        }
    }

    if (outer_node != outer_head) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        for (void* inner_node = *reinterpret_cast<void**>(inner_head);
             inner_node != inner_head;
             inner_node = *reinterpret_cast<void**>(inner_node)) {
            uint32_t inner_key = DwordAt(inner_node, 0x10);
            if (inner_key == 0xffffffffU) {
                continue;
            }
            uint64_t slot_path_pkt[2]{};
            CallVma<void, void*, const void*, uint32_t>(
                0x2ba7d40,
                slot_path_pkt,
                reinterpret_cast<const void*>(GX_VMA(0x49cbef4ULL)),
                inner_key);

            uint64_t slot_open_arg[2]{};
            void* slot_ref_handle = nullptr;
            if (slot_path_pkt[1] != 0) {
                slot_open_arg[1] = slot_path_pkt[1];
            } else if (slot_path_pkt[0] != 0) {
                slot_open_arg[0] = slot_path_pkt[0];
                DwordAt(reinterpret_cast<void*>(slot_path_pkt[0]), 0x38) += 1;
                slot_ref_handle = reinterpret_cast<void*>(slot_path_pkt[0]);
            }

            void* slot_res = CallVma<void*, void*, void*, uint32_t>(
                0x236c900, reader_ctx, slot_open_arg, 0);
            ReleaseParamFileHandle(slot_ref_handle);

            if (slot_res) {
                CallVma<void, void*, void*>(
                    0xe700c0, reinterpret_cast<void*>(QwordAt(inner_node, 0x18)), reader_ctx);
                void** cur_end = reinterpret_cast<void**>(reader_ctx[2]);
                if (reinterpret_cast<void**>(reader_ctx[1]) != cur_end) {
                    void* item = *(cur_end - 1);
                    if (item) {
                        ReleaseContainerObject(item);
                        *(cur_end - 1) = nullptr;
                        cur_end = reinterpret_cast<void**>(reader_ctx[2]);
                    }
                    reader_ctx[2] = reinterpret_cast<uint64_t>(cur_end - 1);
                }
            }
            ReleaseParamFileHandle(reinterpret_cast<void*>(slot_path_pkt[0]));
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }
    }

    CallVma<void, void*>(0x236c720, reader_ctx);
}

// 0x21e65b0 (612 B) - GXYebisBloomParamBank_CreateDebugGuiControls
void GXYebisBloomParamBank_CreateDebugGuiControls(void* self, void* gui_node) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    CallVma<void, void*, void*, uint32_t, uint32_t>(
        0x21877f0, scene_mgr, gui_node, DwordAt(scene_mgr, 0x3c), 0);

    void* cb_alloc = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));

    auto register_action_button = [&](uintptr_t label_wstr_vma, void (*fn_ptr)(void*)) {
        uint64_t SubNodeArg[2]{0, 0};
        void* btn_host = CallVma<void*, void*, void*>(0x236d620, gui_node, SubNodeArg);
        uint64_t LabelArg[2]{0, label_wstr_vma};
        void* cb_obj = AllocatorAlloc(cb_alloc, 0x30, 8);
        if (cb_obj) {
            DwordAt(cb_obj, 0x08) = (DwordAt(cb_obj, 0x08) & 0x80000000U) | 0x40000000U;
            QwordAt(cb_obj, 0x00) = GX_VMA(0x53b70a0ULL);
            QwordAt(cb_obj, 0x10) = reinterpret_cast<uint64_t>(self);
            DwordAt(cb_obj, 0x18) = 0;
            QwordAt(cb_obj, 0x20) = reinterpret_cast<uint64_t>(fn_ptr);
            QwordAt(cb_obj, 0x28) = 0;
        }
        CallVma<void, void*, void*, void*>(0x235e4c0, btn_host, LabelArg, cb_obj);
    };

    register_action_button(GX_VMA(0x49cbc28ULL), &GXYebisBloomParamBank_ReinitializeAllEntries);
    register_action_button(GX_VMA(0x49cbc56ULL), &GXYebisBloomParamBank_SaveAllEntries);
    register_action_button(GX_VMA(0x49cbc88ULL), &GXYebisBloomParamBank_ReloadParamFiles);
}

// 0x21e62d0 (732 B) - GXYebisBloomParamBank_FindEntryByKeys
void* GXYebisBloomParamBank_FindEntryByKeys(void* self, const uint32_t* keys) {
    uint64_t packed = *reinterpret_cast<const uint64_t*>(keys);
    uint32_t outer_key = static_cast<uint32_t>(packed);
    uint32_t inner_key = static_cast<uint32_t>(packed >> 32);
    if (outer_key == 0xffffffffU) {
        if (keys[1] == 0xffffffffU) {
            inner_key = 0;
            outer_key = 1;
        } else {
            outer_key = 0xffffffffU;
        }
    }

    auto find_in_tables = [&](uint32_t o_key, uint32_t i_key) -> void* {
        uint64_t o_hash = ComputeYebisParamHash(o_key);
        uint64_t o_bucket = MapHashToBucket(o_hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
        void* o_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
        void* o_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
        void* o_node = *reinterpret_cast<void**>(
            PtrAt(o_buckets, static_cast<ptrdiff_t>(o_bucket * 48 + 0x10)));
        if (o_node == o_head) {
            return nullptr;
        }
        void* o_end_ptr = *reinterpret_cast<void**>(
            PtrAt(o_buckets, static_cast<ptrdiff_t>(((o_bucket * 2) | 1) * 24 + 0x10)));
        void* o_end = *reinterpret_cast<void**>(o_end_ptr);
        while (o_node != o_end && DwordAt(o_node, 0x10) < o_key) {
            o_node = *reinterpret_cast<void**>(o_node);
        }
        if (o_node == o_end || o_key < DwordAt(o_node, 0x10)) {
            return nullptr;
        }

        void* i_table = reinterpret_cast<void*>(QwordAt(o_node, 0x18));
        uint64_t i_hash = ComputeYebisParamHash(i_key);
        uint64_t i_bucket = MapHashToBucket(i_hash, QwordAt(i_table, 0x48), QwordAt(i_table, 0x50));
        void* i_head = reinterpret_cast<void*>(QwordAt(i_table, 0x08));
        void* i_buckets = reinterpret_cast<void*>(QwordAt(i_table, 0x28));
        void* i_node = *reinterpret_cast<void**>(
            PtrAt(i_buckets, static_cast<ptrdiff_t>(i_bucket * 48 + 0x10)));
        if (i_node == i_head) {
            return nullptr;
        }
        void* i_end_ptr = *reinterpret_cast<void**>(
            PtrAt(i_buckets, static_cast<ptrdiff_t>(((i_bucket * 2) | 1) * 24 + 0x10)));
        void* i_end = *reinterpret_cast<void**>(i_end_ptr);
        while (i_node != i_end && DwordAt(i_node, 0x10) < i_key) {
            i_node = *reinterpret_cast<void**>(i_node);
        }
        if (i_node == i_end || i_key < DwordAt(i_node, 0x10)) {
            return nullptr;
        }
        return reinterpret_cast<void*>(QwordAt(i_node, 0x18));
    };

    if (void* found = find_in_tables(outer_key, inner_key)) {
        return found;
    }
    if (void* fallback = find_in_tables(1u, 0u)) {
        return fallback;
    }
    return PtrAt(self, 0x2d0);
}

// 0x21e6260 (103 B) - GXYebisBloomParamBank_SaveAllEntries
void GXYebisBloomParamBank_SaveAllEntries(void* self) {
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    for (void* outer_node = *reinterpret_cast<void**>(outer_head);
         outer_node != outer_head;
         outer_node = *reinterpret_cast<void**>(outer_node)) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        for (void* inner_node = *reinterpret_cast<void**>(inner_head);
             inner_node != inner_head;
             inner_node = *reinterpret_cast<void**>(inner_node)) {
            CallVma<void, void*>(0xe6e320, reinterpret_cast<void*>(QwordAt(inner_node, 0x18)));
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }
        outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    }
}

// 0x21e61d0 (138 B) - GXYebisBloomParamBank_ReinitializeAllEntries
void GXYebisBloomParamBank_ReinitializeAllEntries(void* self) {
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    for (void* outer_node = *reinterpret_cast<void**>(outer_head);
         outer_node != outer_head;
         outer_node = *reinterpret_cast<void**>(outer_node)) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        for (void* inner_node = *reinterpret_cast<void**>(inner_head);
             inner_node != inner_head;
             inner_node = *reinterpret_cast<void**>(inner_node)) {
            void* entry = reinterpret_cast<void*>(QwordAt(inner_node, 0x18));
            uint32_t inner_key = DwordAt(inner_node, 0x10);
            uint32_t outer_key = DwordAt(outer_node, 0x10);
            DwordAt(entry, 0x128) = inner_key;
            DwordAt(entry, 0x12c) = outer_key;
            CallVma<void, void*, uint32_t, float>(0xe6e380, entry, inner_key, 0.0f);
            CallVma<void, void*, void*>(
                0xe6e4f0,
                reinterpret_cast<void*>(QwordAt(inner_node, 0x18)),
                reinterpret_cast<void*>(QwordAt(self, 0x00)));
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }
        outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    }
}

// 0x21e61c0 (1 B) - GXYebisBloomParamBank_NoOp
void GXYebisBloomParamBank_NoOp(void*) {
    return;
}

// 0x21e6190 (40 B) - GXYebisBloomParamBank_SelectActiveEntry
void GXYebisBloomParamBank_SelectActiveEntry(void* /*self*/, uint32_t key0, uint32_t key1) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x3c);
    CallVma<void, void*, uint32_t, uint32_t, uint32_t, int64_t>(
        0x2186da0, scene_mgr, slot, key0, key1, -1LL);
}

// 0x21e5ed0 (698 B) - GXYebisBloomParamBank_PurgeEntriesForOuterKey
void GXYebisBloomParamBank_PurgeEntriesForOuterKey(
    void* self,
    void* /*unused_rsi*/,
    uint32_t outer_key,
    uint32_t override_slot_idx,
    float blend_time) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x3c);
    CallVma<void, void*, uint32_t, uint32_t, uint32_t, float>(
        0x2185b50, scene_mgr, slot, outer_key, override_slot_idx, blend_time);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = 1;

    uint64_t hash = ComputeYebisParamHash(outer_key);
    uint64_t bucket = MapHashToBucket(hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
    void* outer_node = *reinterpret_cast<void**>(
        PtrAt(outer_buckets, static_cast<ptrdiff_t>(bucket * 48 + 0x10)));
    if (outer_node == outer_head) {
        return;
    }

    void* bucket_end_ptr = *reinterpret_cast<void**>(
        PtrAt(outer_buckets, static_cast<ptrdiff_t>(((bucket * 2) | 1) * 24 + 0x10)));
    void* bucket_end = *reinterpret_cast<void**>(bucket_end_ptr);
    while (outer_node != bucket_end && DwordAt(outer_node, 0x10) < outer_key) {
        outer_node = *reinterpret_cast<void**>(outer_node);
    }
    if (outer_node == bucket_end || outer_key < DwordAt(outer_node, 0x10) || outer_node == outer_head) {
        return;
    }

    void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
    if (inner_table) {
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        void* inner_node = *reinterpret_cast<void**>(inner_head);
        while (inner_node != inner_head) {
            bool keep = CallVma<bool, void*, uint32_t, uint32_t, uint32_t>(
                0x2185e00, scene_mgr, slot, DwordAt(inner_node, 0x10), outer_key);
            void* entry = reinterpret_cast<void*>(QwordAt(inner_node, 0x18));
            if (keep) {
                ByteAt(entry, 0x120) = 1;
                inner_node = *reinterpret_cast<void**>(inner_node);
            } else {
                if (entry) {
                    void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
                    InvokeVirtualDtor0(entry);
                    AllocatorFree(alloc, entry);
                    QwordAt(inner_node, 0x18) = 0;
                }
                uint64_t erase_iter[3]{};
                GXYebisBloomInnerHashTable_EraseNode(
                    erase_iter, reinterpret_cast<void*>(QwordAt(outer_node, 0x18)), inner_node);
                inner_node = reinterpret_cast<void*>(erase_iter[2]);
            }
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }

        if (QwordAt(inner_table, 0x10) != 0) {
            return;
        }

        void* main_alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
        void* inner_buckets = reinterpret_cast<void*>(QwordAt(inner_table, 0x28));
        if (inner_buckets) {
            AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x40)), inner_buckets);
            QwordAt(inner_table, 0x38) = 0;
            QwordAt(inner_table, 0x30) = 0;
            QwordAt(inner_table, 0x28) = 0;
        }
        inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        void* cur = *reinterpret_cast<void**>(inner_head);
        QwordAt(inner_head, 0x00) = reinterpret_cast<uint64_t>(inner_head);
        QwordAt(inner_head, 0x08) = reinterpret_cast<uint64_t>(inner_head);
        QwordAt(inner_table, 0x10) = 0;
        while (cur != inner_head) {
            void* next = *reinterpret_cast<void**>(cur);
            AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), cur);
            cur = next;
        }
        AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), inner_head);
        AllocatorFree(main_alloc, inner_table);
        QwordAt(outer_node, 0x18) = 0;
    }

    uint64_t outer_erase_iter[3]{};
    GXYebisBloomOuterHashTable_EraseNode(outer_erase_iter, PtrAt(self, 0x08), outer_node);
}

// 0x21e5750 (1,912 B) - GXYebisBloomParamBank_EnsureAndPopulateOuterKey
void GXYebisBloomParamBank_EnsureAndPopulateOuterKey(
    void* self, void* context, uint32_t outer_key, uint32_t flags, float weight) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x3c);

    int32_t count = CallVma<int32_t, void*, uint32_t, void*>(0x2184cf0, scene_mgr, slot, context);
    if (count > 0) {
        uint64_t hash = ComputeYebisParamHash(outer_key);
        uint64_t bucket = MapHashToBucket(hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
        void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
        void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
        void* outer_node = *reinterpret_cast<void**>(
            PtrAt(outer_buckets, static_cast<ptrdiff_t>(bucket * 48 + 0x10)));

        if (outer_node != outer_head) {
            void* bucket_end_ptr = *reinterpret_cast<void**>(
                PtrAt(outer_buckets, static_cast<ptrdiff_t>(((bucket * 2) | 1) * 24 + 0x10)));
            void* bucket_end = *reinterpret_cast<void**>(bucket_end_ptr);
            while (outer_node != bucket_end && DwordAt(outer_node, 0x10) < outer_key) {
                outer_node = *reinterpret_cast<void**>(outer_node);
            }
            if (outer_node == bucket_end || outer_key < DwordAt(outer_node, 0x10)) {
                outer_node = outer_head;
            }
        }

        if (outer_node == outer_head) {
            void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
            void* new_inner = AllocatorAlloc(alloc, 0x60, 8);
            if (new_inner) {
                uint8_t caps[32]{};
                AllocatorQueryCaps(alloc, caps);
                if ((caps[0] & 0x20) == 0) {
                    CallVma<void, uint32_t, uint32_t, const void*>(
                        0x20b55b0, 0, 0x3e, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
                }
                QwordAt(new_inner, 0x10) = 0;
                QwordAt(new_inner, 0x08) = 0;
                QwordAt(new_inner, 0x18) = reinterpret_cast<uint64_t>(alloc);
                void* sentinel = AllocatorAlloc(alloc, 0x20, 8);
                if (sentinel) {
                    QwordAt(sentinel, 0x00) = reinterpret_cast<uint64_t>(sentinel);
                    QwordAt(sentinel, 0x08) = reinterpret_cast<uint64_t>(sentinel);
                }
                QwordAt(new_inner, 0x08) = reinterpret_cast<uint64_t>(sentinel);
                QwordAt(new_inner, 0x38) = 0;
                QwordAt(new_inner, 0x30) = 0;
                QwordAt(new_inner, 0x28) = 0;
                QwordAt(new_inner, 0x40) = reinterpret_cast<uint64_t>(alloc);
                DwordAt(new_inner, 0x58) = 0x3f800000U;

                const YebisBucketIter fill_iter{nullptr, nullptr, sentinel};
                YebisBucketIter out_iter{};
                GXYebisBloomInnerHashTable_RehashOrInsertRange(
                    &out_iter,
                    PtrAt(new_inner, 0x20),
                    YebisBucketIter{nullptr, nullptr, nullptr},
                    0x10,
                    &fill_iter);
                QwordAt(new_inner, 0x48) = 7;
                QwordAt(new_inner, 0x50) = 8;
            }

            void* outer_sentinel = reinterpret_cast<void*>(QwordAt(self, 0x10));
            void* outer_first = *reinterpret_cast<void**>(outer_sentinel);
            void* new_outer_node = AllocatorAlloc(reinterpret_cast<void*>(QwordAt(self, 0x20)), 0x20, 8);
            void* prev_node = outer_first ? reinterpret_cast<void*>(QwordAt(outer_first, 0x08)) : new_outer_node;
            QwordAt(new_outer_node, 0x00) = reinterpret_cast<uint64_t>(outer_first ? outer_first : new_outer_node);
            QwordAt(new_outer_node, 0x08) = reinterpret_cast<uint64_t>(prev_node);
            DwordAt(new_outer_node, 0x10) = outer_key;
            QwordAt(new_outer_node, 0x18) = reinterpret_cast<uint64_t>(new_inner);

            QwordAt(self, 0x18) += 1;
            QwordAt(outer_first, 0x08) = reinterpret_cast<uint64_t>(new_outer_node);
            QwordAt(prev_node, 0x00) = reinterpret_cast<uint64_t>(new_outer_node);

            void* cur_front = *reinterpret_cast<void**>(reinterpret_cast<void*>(QwordAt(self, 0x10)));
            uint64_t ins_out[4]{};
            GXYebisBloomOuterHashTable_InsertNode(
                ins_out,
                PtrAt(self, 0x08),
                PtrAt(cur_front, 0x10),
                YebisBucketIter{nullptr, nullptr, cur_front});
            outer_node = reinterpret_cast<void*>(ins_out[2]);
        }

        for (int32_t i = 0; i < count; ++i) {
            uint32_t inner_key = CallVma<uint32_t, void*, uint32_t, void*, int32_t>(
                0x2184de0, scene_mgr, slot, context, i);
            void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            uint64_t i_hash = ComputeYebisParamHash(inner_key);
            uint64_t i_bucket = MapHashToBucket(
                i_hash, QwordAt(inner_table, 0x48), QwordAt(inner_table, 0x50));
            void* i_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
            void* i_buckets = reinterpret_cast<void*>(QwordAt(inner_table, 0x28));
            void* i_cand = *reinterpret_cast<void**>(
                PtrAt(i_buckets, static_cast<ptrdiff_t>(i_bucket * 48 + 0x10)));
            void* found_inner = i_head;
            if (i_cand != i_head) {
                void* i_end_ptr = *reinterpret_cast<void**>(
                    PtrAt(i_buckets, static_cast<ptrdiff_t>(((i_bucket * 2) | 1) * 24 + 0x10)));
                void* i_end = *reinterpret_cast<void**>(i_end_ptr);
                while (i_cand != i_end && DwordAt(i_cand, 0x10) < inner_key) {
                    i_cand = *reinterpret_cast<void**>(i_cand);
                }
                if (i_cand != i_end && inner_key >= DwordAt(i_cand, 0x10)) {
                    found_inner = i_cand;
                }
            }

            if (found_inner != reinterpret_cast<void*>(QwordAt(inner_table, 0x08))) {
                ByteAt(reinterpret_cast<void*>(QwordAt(found_inner, 0x18)), 0x120) = 1;
            } else {
                void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
                void* new_entry = AllocatorAlloc(alloc, 0x130, 0x10);
                if (new_entry) {
                    CallVma<void, void*>(0xe6dd70, new_entry);
                    CallVma<void, void*>(0xe6e070, new_entry);
                    DwordAt(new_entry, 0x128) = inner_key;
                    DwordAt(new_entry, 0x12c) = outer_key;
                    CallVma<void, void*, uint32_t, float>(0xe6e380, new_entry, inner_key, 0.0f);
                    CallVma<void, void*, void*>(0xe6e4f0, new_entry, alloc);
                }

                inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
                void* i_sentinel = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
                void* i_first = *reinterpret_cast<void**>(i_sentinel);
                void* new_i_node = AllocatorAlloc(
                    reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), 0x20, 8);
                void* i_prev = i_first ? reinterpret_cast<void*>(QwordAt(i_first, 0x08)) : new_i_node;
                QwordAt(new_i_node, 0x00) = reinterpret_cast<uint64_t>(i_first ? i_first : new_i_node);
                QwordAt(new_i_node, 0x08) = reinterpret_cast<uint64_t>(i_prev);
                DwordAt(new_i_node, 0x10) = inner_key;
                QwordAt(new_i_node, 0x18) = reinterpret_cast<uint64_t>(new_entry);

                QwordAt(inner_table, 0x10) += 1;
                QwordAt(i_first, 0x08) = reinterpret_cast<uint64_t>(new_i_node);
                QwordAt(i_prev, 0x00) = reinterpret_cast<uint64_t>(new_i_node);

                void* cur_i_front = *reinterpret_cast<void**>(
                    reinterpret_cast<void*>(QwordAt(inner_table, 0x08)));
                uint64_t ins_i_out[4]{};
                GXYebisBloomInnerHashTable_InsertNode(
                    ins_i_out,
                    inner_table,
                    PtrAt(cur_i_front, 0x10),
                    YebisBucketIter{nullptr, nullptr, cur_i_front});
            }
        }
    }

    CallVma<void, void*, uint32_t, uint32_t, uint32_t, void*, float>(
        0x2184ff0, scene_mgr, slot, outer_key, flags, context, weight);
}

// 0x21e3d50 (6,654 B) - GXYebisBloomParamBank_RegisterDebugGuiProperties
bool GXYebisBloomParamBank_RegisterDebugGuiProperties(void* self) {
    GXYebisBloomParamBank_ClearAllEntries(self);

    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t group_id = 0;

    // Register "Bloom[Yebis]" (0x49c42c0 / 0x49c42a6)
    void* gui_mgr = reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28));
    CallVma<void, void*, uint32_t*, const void*, const void*>(
        0xe32510,
        gui_mgr,
        &group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c42c0ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c42a6ULL)));
    DwordAt(scene_mgr, 0x3c) = group_id;

    // Open sub-group "Glare" (0x49c42cc)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c42c0ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c42ccULL)),
        1u,
        1u);

    const uintptr_t kFloatGetVtbl = GX_VMA(0x52e83f0ULL) + 0x10;
    const uintptr_t kFloatSetVtbl = GX_VMA(0x52e4890ULL) + 0x10;
    const uintptr_t kBoolGetVtbl  = GX_VMA(0x52e8760ULL) + 0x10;
    const uintptr_t kBoolSetVtbl  = GX_VMA(0x52e4a70ULL) + 0x10;
    const uintptr_t kIntGetVtbl   = GX_VMA(0x52e8450ULL) + 0x10;
    const uintptr_t kIntSetVtbl   = GX_VMA(0x52e4930ULL) + 0x10;
    const uintptr_t kColorVtbl    = GX_VMA(0x52e8470ULL) + 0x10;

    // +0x140: Glare Threshold (float [0.0, 100.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4a22ULL), GX_VMA(0x49c42e8ULL), PtrAt(self, 0x140),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x144: Glare Exposure (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4312ULL), GX_VMA(0x49c4330ULL), PtrAt(self, 0x144),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x148: Glare GaussianRadiusScale (float [0.002, 0.1], step 0.001)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c435aULL), GX_VMA(0x49c4388ULL), PtrAt(self, 0x148),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x3b03126fU, 0x3dcccccdU, 0x3a83126fU, 0x3f800000U, 1u);

    // +0x14c: Glare RemapFactor (float [0.1, 16.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c43c2ULL), GX_VMA(0x49c43daULL), PtrAt(self, 0x14c),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x3dcccccdU, 0x41800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x154: Glare ChromaticAberration (float [0.0, 1.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c43feULL), GX_VMA(0x49c4438ULL), PtrAt(self, 0x154),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x158: Glare FalseColorIntensity (float [0.00002, 0.9999], step 0.00001)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c447eULL), GX_VMA(0x49c44b2ULL), PtrAt(self, 0x158),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x37a7c5acU, 0x3f7ff972U, 0x3727c5acU, 0x3f800000U, 1u);

    // +0x15c: Glare GhostDecay (float [0.0, 1.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c44f2ULL), GX_VMA(0x49c4516ULL), PtrAt(self, 0x15c),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // Open sub-group "Star" (0x49c4546 / 0x49cb990)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49cb990ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c4546ULL)),
        1u,
        1u);

    // +0x160: Star Type (enum 15 entries from 0x535ff00, 0xf0 bytes)
    uint8_t star_enum_buf[0xf0];
    std::memcpy(star_enum_buf, reinterpret_cast<const void*>(GX_VMA(0x535ff00ULL)), 0xf0);
    GXDebugGui_RegisterEnumProperty(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c47f8ULL)),
        reinterpret_cast<const char16_t*>(GX_VMA(0x49c480eULL)),
        PtrAt(self, 0x160),
        15u,
        star_enum_buf,
        1u);

    // +0x164: Star MaxLines (int32 [-1, 16], step 1, prop_type 0x07)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c53aeULL), GX_VMA(0x49c4830ULL), PtrAt(self, 0x164),
        0x07, 0x00, kIntGetVtbl, kIntSetVtbl,
        0xffffffffU, 0x00000010U, 0x00000001U, 0x3f800000U, 1u);

    // +0x168: Star Length (float [0.5, 8.5], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c485aULL), GX_VMA(0x49c4896ULL), PtrAt(self, 0x168),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x3f000000U, 0x41080000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x16c: Star Exposure (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c48deULL), GX_VMA(0x49c4910ULL), PtrAt(self, 0x16c),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x170: Star RotationScale (float [0.0, 1.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c494eULL), GX_VMA(0x49c498cULL), PtrAt(self, 0x170),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // Close sub-group "Star"
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);

    // Open sub-group "Afterimage" (0x49c49ee / 0x49c49d6)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c49d6ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c49eeULL)),
        1u,
        1u);

    // +0x1a0: Afterimage Threshold (float [0.0, 100.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4a22ULL), GX_VMA(0x49c4a40ULL), PtrAt(self, 0x1a0),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1a4: Afterimage LuminanceThreshold (float [0.0, 100.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4a82ULL), GX_VMA(0x49c4aa0ULL), PtrAt(self, 0x1a4),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1a8: Afterimage Exposure (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4ae2ULL), GX_VMA(0x49c4b02ULL), PtrAt(self, 0x1a8),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1ac: Afterimage FeedbackPassCount (int32 [0, 12], step 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4b46ULL), GX_VMA(0x49c4b78ULL), PtrAt(self, 0x1ac),
        0x07, 0x00, kIntGetVtbl, kIntSetVtbl,
        0x00000000U, 0x0000000cU, 0x00000001U, 0x3f800000U, 1u);

    // +0x1b0: Afterimage Persistence (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4bceULL), GX_VMA(0x49c4becULL), PtrAt(self, 0x1b0),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1b4: Afterimage FadeSpeed (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4c2eULL), GX_VMA(0x49c4c54ULL), PtrAt(self, 0x1b4),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1b8: Afterimage Scale (float [0.0, 1.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4c9eULL), GX_VMA(0x49c4cbeULL), PtrAt(self, 0x1b8),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1bc: Afterimage TranslateEnable (int32/bool [0, 1], step 0, flags 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4d02ULL), GX_VMA(0x49c4d20ULL), PtrAt(self, 0x1bc),
        0x07, 0x01, kIntGetVtbl, kIntSetVtbl,
        0x00000000U, 0x00000001U, 0x00000000U, 0x3f800000U, 0u);

    // +0x1c0: Afterimage TranslateSpeed (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4d64ULL), GX_VMA(0x49c4d80ULL), PtrAt(self, 0x1c0),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1c4: Afterimage TranslateType (int32 [0, 12], step 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4dc0ULL), GX_VMA(0x49c4dd8ULL), PtrAt(self, 0x1c4),
        0x07, 0x00, kIntGetVtbl, kIntSetVtbl,
        0x00000000U, 0x0000000cU, 0x00000001U, 0x3f800000U, 1u);

    // +0x1c8: Afterimage TranslateDirection
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4e14ULL), GX_VMA(0x49c4e2aULL), PtrAt(self, 0x1c8),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1cc: Afterimage ScaleCenter (float [0.0, 100.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4e64ULL), GX_VMA(0x49c4e8cULL), PtrAt(self, 0x1cc),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1d0: Afterimage RotationEnable (int32/bool [0, 1], step 0, flags 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4ed8ULL), GX_VMA(0x49c4ef2ULL), PtrAt(self, 0x1d0),
        0x07, 0x01, kIntGetVtbl, kIntSetVtbl,
        0x00000000U, 0x00000001U, 0x00000000U, 0x3f800000U, 0u);

    // +0x1d4: Afterimage RotationAngle (float [0.0, 360.0], step 1.0)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4f32ULL), GX_VMA(0x49c4f5cULL), PtrAt(self, 0x1d4),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x43b40000U, 0x3f800000U, 0x3f800000U, 1u);

    // +0x1d8: Afterimage RotationSpeed (float [-10.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c4faaULL), GX_VMA(0x49c4fc8ULL), PtrAt(self, 0x1d8),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0xc1200000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1dc: Afterimage ScalingEnable (int32/bool [0, 1], step 0, flags 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c500aULL), GX_VMA(0x49c5032ULL), PtrAt(self, 0x1dc),
        0x07, 0x01, kIntGetVtbl, kIntSetVtbl,
        0x00000000U, 0x00000001U, 0x00000000U, 0x3f800000U, 0u);

    // +0x1e0: Afterimage ScalingAmount (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c5080ULL), GX_VMA(0x49c50a8ULL), PtrAt(self, 0x1e0),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1e4: Afterimage ScalingSpeed (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c50f4ULL), GX_VMA(0x49c5116ULL), PtrAt(self, 0x1e4),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // Close sub-group "Afterimage"
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);

    // Open sub-group "AnamorphicLens" (0x49c517a / 0x49c515c)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c515cULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c517aULL)),
        1u,
        1u);

    // +0x174: AnamorphicLens Enable (bool [0, 1], flags 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c51a8ULL), PtrAt(self, 0x174),
        0x03, 0x01, kBoolGetVtbl, kBoolSetVtbl,
        0x00000000U, 0x00000001U, 0x00000000U, 0x3f800000U, 0u);

    // +0x178: AnamorphicLens Exposure (float [0.0, 30.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c51e0ULL), GX_VMA(0x49c5212ULL), PtrAt(self, 0x178),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41f00000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x17c: AnamorphicLens Threshold (float [0.0, 30.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c5250ULL), GX_VMA(0x49c5296ULL), PtrAt(self, 0x17c),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41f00000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x180: AnamorphicLens Color0 (vec4/color type 0x0e, flags 0x12)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c52e8ULL), GX_VMA(0x49c5312ULL), PtrAt(self, 0x180),
        0x0e, 0x12, kColorVtbl, kFloatSetVtbl,
        0x00000000U, 0x00000000U, 0x00000000U, 0x3f800000U, 1u);

    // +0x190: AnamorphicLens Color1 (vec4/color type 0x0e, flags 0x12)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c52e8ULL), GX_VMA(0x49c5312ULL), PtrAt(self, 0x190),
        0x0e, 0x12, kColorVtbl, kFloatSetVtbl,
        0x00000000U, 0x00000000U, 0x00000000U, 0x3f800000U, 1u);

    // Close "AnamorphicLens" and "Glare"
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);

    // Open group "Bloom" (0x49c5348, r8d=0, r9d=1)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c5348ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c5348ULL)),
        0u,
        1u);

    // +0x1e8: Bloom Enable (bool [0, 1], flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c537aULL), PtrAt(self, 0x1e8),
        0x03, 0x18, kBoolGetVtbl, kBoolSetVtbl,
        0x00000000U, 0x00000001U, 0x00000000U, 0x3f800000U, 0u);

    // +0x1ec: Bloom PassCount (int32 [1, 8], step 1, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c53aeULL), GX_VMA(0x49c53ccULL), PtrAt(self, 0x1ec),
        0x07, 0x18, kIntGetVtbl, kIntSetVtbl,
        0x00000001U, 0x00000008U, 0x00000001U, 0x3f800000U, 1u);

    // +0x1f0: Bloom Threshold (float [0.0, 10.0], step 0.1, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c5402ULL), GX_VMA(0x49c5422ULL), PtrAt(self, 0x1f0),
        0x09, 0x18, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3dcccccdU, 0x3f800000U, 1u);

    // +0x1f4: Bloom Intensity (float [0.0, 10.0], step 0.01, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c545aULL), GX_VMA(0x49c547aULL), PtrAt(self, 0x1f4),
        0x09, 0x18, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1f8: Bloom SoftKnee (float [0.0, 1.0], step 0.01, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c54b2ULL), GX_VMA(0x49c54d4ULL), PtrAt(self, 0x1f8),
        0x09, 0x18, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x200: Bloom Radius (float [0.01, 2.0], step 0.01, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c550eULL), GX_VMA(0x49c554cULL), PtrAt(self, 0x200),
        0x09, 0x18, kFloatGetVtbl, kFloatSetVtbl,
        0x3c23d70aU, 0x40000000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x1fc: Bloom Spread (float [0.0, 1.0], step 0.01, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c55a2ULL), GX_VMA(0x49c55d2ULL), PtrAt(self, 0x1fc),
        0x09, 0x18, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x204: Bloom BlurWeight (float [0.0, 1.0], step 0.01, flags 0x18)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c561aULL), GX_VMA(0x49c563eULL), PtrAt(self, 0x204),
        0x09, 0x18, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // Close group "Bloom"
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 0u);

    CallVma<void, void*, void*, uint32_t>(
        0x218e2b0, PtrAt(scene_mgr, 0x130), reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), 0u);
    CallVma<void, void*, void*, uint32_t, uint32_t, uint32_t, float>(
        0xe41f00,
        PtrAt(scene_mgr, 0x1b8),
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        0u,
        group_id,
        0u,
        0.0f);
    CallVma<void, void*, uint32_t>(0x21874c0, scene_mgr, group_id);
    return true;
}

// 0x21e3bc0 (395 B) - GXYebisBloomParamBank_ClearAllEntries
void GXYebisBloomParamBank_ClearAllEntries(void* self) {
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* outer_node = *reinterpret_cast<void**>(outer_head);
    while (outer_node != outer_head) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        void* inner_node = *reinterpret_cast<void**>(inner_head);
        while (inner_node != inner_head) {
            void* entry = reinterpret_cast<void*>(QwordAt(inner_node, 0x18));
            if (entry) {
                void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
                InvokeVirtualDtor0(entry);
                AllocatorFree(alloc, entry);
                QwordAt(inner_node, 0x18) = 0;
                inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            }
            uint64_t erase_iter[3]{};
            GXYebisBloomInnerHashTable_EraseNode(erase_iter, inner_table, inner_node);
            inner_node = reinterpret_cast<void*>(erase_iter[2]);
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        }

        if (inner_table) {
            void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
            void* inner_buckets = reinterpret_cast<void*>(QwordAt(inner_table, 0x28));
            if (inner_buckets) {
                AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x40)), inner_buckets);
                QwordAt(inner_table, 0x38) = 0;
                QwordAt(inner_table, 0x30) = 0;
                QwordAt(inner_table, 0x28) = 0;
            }
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
            void* cur = *reinterpret_cast<void**>(inner_head);
            QwordAt(inner_head, 0x00) = reinterpret_cast<uint64_t>(inner_head);
            QwordAt(inner_head, 0x08) = reinterpret_cast<uint64_t>(inner_head);
            QwordAt(inner_table, 0x10) = 0;
            while (cur != inner_head) {
                void* next = *reinterpret_cast<void**>(cur);
                AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), cur);
                cur = next;
            }
            AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), inner_head);
            AllocatorFree(alloc, inner_table);
            QwordAt(outer_node, 0x18) = 0;
        }

        uint64_t outer_erase_iter[3]{};
        GXYebisBloomOuterHashTable_EraseNode(outer_erase_iter, PtrAt(self, 0x08), outer_node);
        outer_node = reinterpret_cast<void*>(outer_erase_iter[2]);
    }
}

// 0x21e3ac0 (255 B) - GXYebisBloomParamBank_Destruct
void GXYebisBloomParamBank_Destruct(void* self) {
    GXYebisBloomParamBank_ClearAllEntries(self);
    CallVma<void, void*>(0xe6e1d0, PtrAt(self, 0x2d0));

    uint64_t str_cap = QwordAt(self, 0x238);
    if (str_cap >= 8) {
        if (str_cap + 1 >= 0x41) {
            CallVma<void, const void*, uint32_t, const void*>(
                0x20b55b0,
                reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        ByteAt(self, 0x2c1) &= 0xfdU;
    }
    QwordAt(self, 0x238) = 7;
    QwordAt(self, 0x230) = 0;
    WordAt(self, 0x220) = 0;

    void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
    if (outer_buckets) {
        AllocatorFree(reinterpret_cast<void*>(QwordAt(self, 0x48)), outer_buckets);
        QwordAt(self, 0x40) = 0;
        QwordAt(self, 0x38) = 0;
        QwordAt(self, 0x30) = 0;
    }

    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* cur = *reinterpret_cast<void**>(outer_head);
    QwordAt(outer_head, 0x00) = reinterpret_cast<uint64_t>(outer_head);
    QwordAt(outer_head, 0x08) = reinterpret_cast<uint64_t>(outer_head);
    QwordAt(self, 0x18) = 0;
    while (cur != outer_head) {
        void* next = *reinterpret_cast<void**>(cur);
        AllocatorFree(reinterpret_cast<void*>(QwordAt(self, 0x20)), cur);
        cur = next;
    }
    AllocatorFree(reinterpret_cast<void*>(QwordAt(self, 0x20)), outer_head);
}

// 0x21e3750 (871 B) - GXYebisBloomParamBank_Construct
void GXYebisBloomParamBank_Construct(void* self, void* allocator) {
    QwordAt(self, 0x00) = reinterpret_cast<uint64_t>(allocator);
    uint8_t caps[136]{};
    AllocatorQueryCaps(allocator, caps);
    if ((caps[0] & 0x20) == 0) {
        CallVma<void, uint32_t, uint32_t, const void*>(
            0x20b55b0, 0, 0x3e, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }

    QwordAt(self, 0x18) = 0;
    QwordAt(self, 0x10) = 0;
    QwordAt(self, 0x20) = reinterpret_cast<uint64_t>(allocator);
    void* sentinel = AllocatorAlloc(allocator, 0x20, 8);
    if (sentinel) {
        QwordAt(sentinel, 0x00) = reinterpret_cast<uint64_t>(sentinel);
        QwordAt(sentinel, 0x08) = reinterpret_cast<uint64_t>(sentinel);
    }
    QwordAt(self, 0x10) = reinterpret_cast<uint64_t>(sentinel);

    QwordAt(self, 0x40) = 0;
    QwordAt(self, 0x38) = 0;
    QwordAt(self, 0x30) = 0;
    QwordAt(self, 0x48) = reinterpret_cast<uint64_t>(allocator);
    DwordAt(self, 0x60) = 0x3f800000U;

    const YebisBucketIter fill_iter{nullptr, nullptr, sentinel};
    YebisBucketIter out_iter{};
    GXYebisBloomOuterHashTable_RehashOrInsertRange(
        &out_iter,
        PtrAt(self, 0x28),
        YebisBucketIter{nullptr, nullptr, nullptr},
        0x10,
        &fill_iter);
    QwordAt(self, 0x50) = 7;
    QwordAt(self, 0x58) = 8;

    void* block_active = PtrAt(self, 0x70);
    CallVma<void, void*>(0xe6d670, block_active);
    void* block_backup = PtrAt(self, 0x140);
    CallVma<void, void*>(0xe6d670, block_backup);
    DwordAt(self, 0x210) = 0;

    void* dl_str = PtrAt(self, 0x218);
    std::memcpy(PtrAt(self, 0x240), caps, 0x81);
    ByteAt(self, 0x2c1) = 0;
    QwordAt(self, 0x238) = 7;
    QwordAt(self, 0x230) = 0;
    WordAt(self, 0x220) = 0;
    ByteAt(self, 0x2c2) = 1;
    CallVma<void, void*, uint64_t, uint64_t>(0x2935350, dl_str, 0x3fULL, 0ULL);
    QwordAt(self, 0x230) = 0;
    void* str_buf = PtrAt(self, 0x220);
    if (QwordAt(self, 0x238) >= 8) {
        str_buf = *reinterpret_cast<void**>(str_buf);
    }
    *reinterpret_cast<uint16_t*>(str_buf) = 0;

    void* default_entry = PtrAt(self, 0x2d0);
    CallVma<void, void*>(0xe6dd70, default_entry);
    CallVma<void, void*>(0xe6e070, default_entry);

    std::memcpy(block_backup, block_active, 0xc8);
    DwordAt(self, 0x210) = 0;
    CallVma<void, void*, const void*, uint64_t>(
        0x29354e0, dl_str, reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)), 0ULL);
}

// ============================================================================
// PART 2: GXYebisAntiAliasParamBank (0x21dfd20 .. 0x21e3200, 22 functions)
// ============================================================================

// Forward declarations within Part 2
void* GXYebisAntiAliasOuterHashTable_InsertNode(
    void* out_pair, void* table, void* key_ptr, YebisBucketIter node_iter);
void* GXYebisAntiAliasInnerHashTable_InsertNode(
    void* out_pair, void* table, void* key_ptr, YebisBucketIter node_iter);
void* GXYebisAntiAliasOuterHashTable_EraseNode(void* out_iter, void* table, void* node);
void* GXYebisAntiAliasInnerHashTable_EraseNode(void* out_iter, void* table, void* node);
void GXYebisAntiAliasParamBank_ClearAllEntries(void* self);
void GXYebisAntiAliasParamBank_ReinitializeAllEntries(void* self);
void GXYebisAntiAliasParamBank_SaveAllEntries(void* self);

// 0x21e3200 (1,352 B) - GXYebisAntiAliasInnerHashTable_RehashOrInsertRange
void* GXYebisAntiAliasInnerHashTable_RehashOrInsertRange(
    void* out_iter,
    void* table_vec_base,
    YebisBucketIter where_iter,
    uint64_t bucket_entries_count,
    const YebisBucketIter* fill_val) {
    // Exact std::vector<YebisBucketIter>::insert(const_iterator _Where, size_type _Count, const YebisBucketIter& _Val)
    // table_vec_base points to table + 0x20:
    //   +0x08: begin_ptr (each entry is 24 bytes)
    //   +0x10: end_ptr
    //   +0x18: cap_ptr
    //   +0x20: allocator
    uint8_t* begin_ptr = reinterpret_cast<uint8_t*>(QwordAt(table_vec_base, 0x08));
    uint8_t* end_ptr = reinterpret_cast<uint8_t*>(QwordAt(table_vec_base, 0x10));
    uint8_t* cap_ptr = reinterpret_cast<uint8_t*>(QwordAt(table_vec_base, 0x18));
    void* alloc = reinterpret_cast<void*>(QwordAt(table_vec_base, 0x20));

    uint8_t* where_ptr = reinterpret_cast<uint8_t*>(where_iter.node);
    const YebisBucketIter val = fill_val ? *fill_val : YebisBucketIter{nullptr, nullptr, nullptr};
    const uint64_t where_idx = static_cast<uint64_t>(where_ptr - begin_ptr) / 24ULL;

    if (bucket_entries_count != 0) {
        const uint64_t avail = static_cast<uint64_t>(cap_ptr - end_ptr) / 24ULL;
        const uint64_t cur_count = static_cast<uint64_t>(end_ptr - begin_ptr) / 24ULL;
        if (bucket_entries_count > avail) {
            constexpr uint64_t kMaxVecItems = 0xaaaaaaaaaaaaaaaULL;
            if (kMaxVecItems - cur_count < bucket_entries_count) {
                CallVma<void, const void*>(0x2bbf1b8, reinterpret_cast<const void*>(GX_VMA(0x4940adaULL)));
            }
            const uint64_t req = cur_count + bucket_entries_count;
            const uint64_t cur_cap = static_cast<uint64_t>(cap_ptr - begin_ptr) / 24ULL;
            uint64_t new_cap = (kMaxVecItems - (cur_cap >> 1) < cur_cap) ? 0ULL : (cur_cap + (cur_cap >> 1));
            if (new_cap < req) {
                new_cap = req;
            }
            uint8_t* new_buf = static_cast<uint8_t*>(AllocatorAlloc(alloc, new_cap * 24ULL, 8));
            if (!new_buf) {
                CallVma<void, uint32_t, uint32_t, const void*>(
                    0x20b55b0, 0, 0x109, reinterpret_cast<const void*>(GX_VMA(0x4737d80ULL)));
            }

            uint8_t* fill_dst = new_buf + where_idx * 24ULL;
            for (uint64_t i = 0; i < bucket_entries_count; ++i, fill_dst += 24) {
                if (fill_dst) {
                    *reinterpret_cast<YebisBucketIter*>(fill_dst) = val;
                }
            }
            if (begin_ptr != where_ptr) {
                std::memcpy(new_buf, begin_ptr, static_cast<size_t>(where_ptr - begin_ptr));
            }
            if (where_ptr != end_ptr) {
                std::memcpy(
                    new_buf + (where_idx + bucket_entries_count) * 24ULL,
                    where_ptr,
                    static_cast<size_t>(end_ptr - where_ptr));
            }
            if (begin_ptr) {
                AllocatorFree(alloc, begin_ptr);
            }
            QwordAt(table_vec_base, 0x08) = reinterpret_cast<uint64_t>(new_buf);
            QwordAt(table_vec_base, 0x10) = reinterpret_cast<uint64_t>(new_buf + req * 24ULL);
            QwordAt(table_vec_base, 0x18) = reinterpret_cast<uint64_t>(new_buf + new_cap * 24ULL);
            begin_ptr = new_buf;
        } else {
            if (where_ptr != end_ptr) {
                std::memmove(
                    where_ptr + bucket_entries_count * 24ULL,
                    where_ptr,
                    static_cast<size_t>(end_ptr - where_ptr));
            }
            uint8_t* fill_dst = where_ptr;
            for (uint64_t i = 0; i < bucket_entries_count; ++i, fill_dst += 24) {
                if (fill_dst) {
                    *reinterpret_cast<YebisBucketIter*>(fill_dst) = val;
                }
            }
            QwordAt(table_vec_base, 0x10) =
                reinterpret_cast<uint64_t>(end_ptr + bucket_entries_count * 24ULL);
        }
    }
    if (out_iter) {
        QwordAt(out_iter, 0x00) = reinterpret_cast<uint64_t>(where_iter.proxy0);
        QwordAt(out_iter, 0x08) = reinterpret_cast<uint64_t>(where_iter.proxy1);
        QwordAt(out_iter, 0x10) = reinterpret_cast<uint64_t>(begin_ptr + where_idx * 24ULL);
    }
    return out_iter;
}

// 0x21e2cb0 (1,352 B) - GXYebisAntiAliasOuterHashTable_RehashOrInsertRange
void* GXYebisAntiAliasOuterHashTable_RehashOrInsertRange(
    void* out_iter,
    void* table_vec_base,
    YebisBucketIter where_iter,
    uint64_t bucket_entries_count,
    const YebisBucketIter* fill_val) {
    return GXYebisAntiAliasInnerHashTable_RehashOrInsertRange(
        out_iter, table_vec_base, where_iter, bucket_entries_count, fill_val);
}

// 0x21e28a0 (1,040 B) - GXYebisAntiAliasInnerHashTable_InsertNode
void* GXYebisAntiAliasInnerHashTable_InsertNode(
    void* out_pair,
    void* table,
    void* key_ptr,
    YebisBucketIter node_iter) {
    void* node = node_iter.node;
    uint32_t key = *reinterpret_cast<const uint32_t*>(key_ptr);
    uint64_t hash = ComputeYebisParamHash(key);
    uint64_t mask = QwordAt(table, 0x48);
    uint64_t max_b = QwordAt(table, 0x50);
    uint64_t bucket = MapHashToBucket(hash, mask, max_b);

    void* head = reinterpret_cast<void*>(QwordAt(table, 0x08));
    uint8_t* buckets = reinterpret_cast<uint8_t*>(QwordAt(table, 0x28));
    void* b_start = *reinterpret_cast<void**>(buckets + bucket * 48 + 0x10);
    void* cur = head;

    if (b_start != head) {
        void* b_end = *reinterpret_cast<void**>(
            *reinterpret_cast<void**>(buckets + ((bucket * 2) | 1) * 24 + 0x10));
        cur = b_end;
        while (cur != b_start) {
            cur = reinterpret_cast<void*>(QwordAt(cur, 0x08));
            if (key < DwordAt(cur, 0x10)) {
                continue;
            }
            if (DwordAt(cur, 0x10) >= key) {
                // Duplicate key: unlink & free newly created node
                if (node != head) {
                    void* next_n = reinterpret_cast<void*>(QwordAt(node, 0x00));
                    void* prev_n = reinterpret_cast<void*>(QwordAt(node, 0x08));
                    QwordAt(prev_n, 0x00) = reinterpret_cast<uint64_t>(next_n);
                    QwordAt(next_n, 0x08) = reinterpret_cast<uint64_t>(prev_n);
                    QwordAt(table, 0x10) -= 1;
                    AllocatorFree(reinterpret_cast<void*>(QwordAt(table, 0x18)), node);
                }
                if (out_pair) {
                    QwordAt(out_pair, 0x00) = 0;
                    QwordAt(out_pair, 0x08) = 0;
                    QwordAt(out_pair, 0x10) = reinterpret_cast<uint64_t>(cur);
                    ByteAt(out_pair, 0x18) = 0;
                }
                return out_pair;
            }
            cur = reinterpret_cast<void*>(QwordAt(cur, 0x00));
            break;
        }
    }

    void* const insert_pos = cur;

    // Splice node before insert_pos if not already positioned
    if (*reinterpret_cast<void**>(node) != insert_pos) {
        void* n_next = reinterpret_cast<void*>(QwordAt(node, 0x00));
        void* n_prev = reinterpret_cast<void*>(QwordAt(node, 0x08));
        QwordAt(n_prev, 0x00) = reinterpret_cast<uint64_t>(n_next);
        QwordAt(n_next, 0x08) = reinterpret_cast<uint64_t>(n_prev);

        void* pos_prev = reinterpret_cast<void*>(QwordAt(insert_pos, 0x08));
        QwordAt(node, 0x00) = reinterpret_cast<uint64_t>(insert_pos);
        QwordAt(node, 0x08) = reinterpret_cast<uint64_t>(pos_prev);
        QwordAt(pos_prev, 0x00) = reinterpret_cast<uint64_t>(node);
        QwordAt(insert_pos, 0x08) = reinterpret_cast<uint64_t>(node);
    }

    uint8_t* b_slot = reinterpret_cast<uint8_t*>(QwordAt(table, 0x28)) + bucket * 48;
    void** slot_first = reinterpret_cast<void**>(b_slot + 0x10);
    void** slot_last  = reinterpret_cast<void**>(b_slot + 0x28);
    if (*slot_first == reinterpret_cast<void*>(QwordAt(table, 0x08))) {
        *slot_first = node;
        *slot_last  = node;
    } else if (*slot_first == insert_pos) {
        *slot_first = node;
    } else {
        void* last_next = *reinterpret_cast<void**>(*slot_last);
        *slot_last = (last_next != node)
            ? reinterpret_cast<void*>(QwordAt(last_next, 0x08))
            : last_next;
    }

    auto to_float_u64 = [](uint64_t v) -> float {
        if (static_cast<int64_t>(v) < 0) {
            return static_cast<float>((v >> 1) | (v & 1ULL)) * 2.0f;
        }
        return static_cast<float>(static_cast<int64_t>(v));
    };

    // Check load factor and grow buckets if needed
    uint64_t count = QwordAt(table, 0x10);
    float load = to_float_u64(count) / to_float_u64(max_b);
    if (load > FloatAt(table, 0x58)) {
        uint64_t new_max_b = max_b;
        if (new_max_b <= 0x1ffULL) {
            new_max_b <<= 3;
        } else if (new_max_b < 0x555555555555555ULL) {
            new_max_b <<= 1;
        }

        void* vec_begin = reinterpret_cast<void*>(QwordAt(table, 0x28));
        QwordAt(table, 0x30) = reinterpret_cast<uint64_t>(vec_begin);
        const YebisBucketIter fill_iter{nullptr, nullptr, head};
        YebisBucketIter out_it{};
        GXYebisAntiAliasInnerHashTable_RehashOrInsertRange(
            &out_it,
            PtrAt(table, 0x20),
            YebisBucketIter{nullptr, nullptr, vec_begin},
            new_max_b * 2,
            &fill_iter);
        QwordAt(table, 0x48) = new_max_b - 1ULL;
        QwordAt(table, 0x50) = new_max_b;

        void* list_head = reinterpret_cast<void*>(QwordAt(table, 0x08));
        if (*reinterpret_cast<void**>(list_head) != list_head) {
            void* last_orig = reinterpret_cast<void*>(QwordAt(list_head, 0x08));
            while (true) {
                void* move_node = *reinterpret_cast<void**>(
                    reinterpret_cast<void*>(QwordAt(table, 0x08)));
                uint64_t reinsert_out[4]{};
                GXYebisAntiAliasInnerHashTable_InsertNode(
                    reinsert_out,
                    table,
                    PtrAt(move_node, 0x10),
                    YebisBucketIter{nullptr, nullptr, move_node});
                if (move_node == last_orig) {
                    break;
                }
            }
        }
    }

    if (out_pair) {
        QwordAt(out_pair, 0x00) = 0;
        QwordAt(out_pair, 0x08) = 0;
        QwordAt(out_pair, 0x10) = reinterpret_cast<uint64_t>(node);
        ByteAt(out_pair, 0x18) = 1;
    }
    return out_pair;
}

// 0x21e2490 (1,040 B) - GXYebisAntiAliasOuterHashTable_InsertNode
void* GXYebisAntiAliasOuterHashTable_InsertNode(
    void* out_pair,
    void* table,
    void* key_ptr,
    YebisBucketIter node_iter) {
    return GXYebisAntiAliasInnerHashTable_InsertNode(out_pair, table, key_ptr, node_iter);
}

// 0x21e2320 (363 B) - GXYebisAntiAliasOuterHashTable_EraseNode
void* GXYebisAntiAliasOuterHashTable_EraseNode(void* out_iter, void* table, void* node) {
    return GXYebisAntiAliasInnerHashTable_EraseNode(out_iter, table, node);
}

// 0x21e21b0 (363 B) - GXYebisAntiAliasInnerHashTable_EraseNode
void* GXYebisAntiAliasInnerHashTable_EraseNode(void* out_iter, void* table, void* node) {
    uint32_t key = DwordAt(node, 0x10);
    uint64_t hash = ComputeYebisParamHash(key);
    uint64_t bucket = MapHashToBucket(hash, QwordAt(table, 0x48), QwordAt(table, 0x50));

    void* head = reinterpret_cast<void*>(QwordAt(table, 0x08));
    uint8_t* buckets = reinterpret_cast<uint8_t*>(QwordAt(table, 0x28));
    void** b_begin_slot = reinterpret_cast<void**>(buckets + bucket * 48 + 0x10);
    void** b_end_slot = reinterpret_cast<void**>(buckets + ((bucket * 2) | 1) * 24 + 0x10);

    void* next_node = *reinterpret_cast<void**>(node);
    void* prev_node = reinterpret_cast<void*>(QwordAt(node, 0x08));

    if (*b_begin_slot == node) {
        if (*b_end_slot == node) {
            *b_begin_slot = head;
            *b_end_slot = head;
        } else {
            *b_begin_slot = next_node;
        }
    } else if (*b_end_slot == node) {
        *b_end_slot = prev_node;
    }

    QwordAt(prev_node, 0x00) = reinterpret_cast<uint64_t>(next_node);
    QwordAt(next_node, 0x08) = reinterpret_cast<uint64_t>(prev_node);
    QwordAt(table, 0x10) -= 1;
    AllocatorFree(reinterpret_cast<void*>(QwordAt(table, 0x18)), node);

    if (out_iter) {
        QwordAt(out_iter, 0x00) = 0;
        QwordAt(out_iter, 0x08) = 0;
        QwordAt(out_iter, 0x10) = reinterpret_cast<uint64_t>(next_node);
    }
    return out_iter;
}

// 0x21e2180 (40 B) - GXYebisAntiAliasCallback_Invoke
void GXYebisAntiAliasCallback_Invoke(void* self) {
    if ((ByteAt(self, 0x0b) & 0x40) == 0) {
        return;
    }
    uint8_t* target = reinterpret_cast<uint8_t*>(QwordAt(self, 0x10)) +
                      static_cast<ptrdiff_t>(QwordAt(self, 0x28));
    uintptr_t fn_or_vtbl_off = QwordAt(self, 0x20);
    if ((fn_or_vtbl_off & 1ULL) != 0) {
        uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(target);
        fn_or_vtbl_off = *reinterpret_cast<uintptr_t*>(vtbl + fn_or_vtbl_off - 1);
    }
    auto fn = reinterpret_cast<void (*)(void*, uint32_t)>(fn_or_vtbl_off);
    fn(target, DwordAt(self, 0x18));
}

// 0x21e2170 (5 B) - GXYebisAntiAliasCallback_DeletingDtor
void GXYebisAntiAliasCallback_DeletingDtor(void* self) {
    CallVma<void, void*>(0x2bbe588, self);
}

// 0x21e2150 (22 B) - GXYebisAntiAliasCallback_Dtor
void GXYebisAntiAliasCallback_Dtor(void* self) {
    uint32_t flags = DwordAt(self, 0x08) & 0xbfffffffU;
    QwordAt(self, 0x00) = GX_VMA(0x53b93b0ULL);
    DwordAt(self, 0x08) = flags;
}

// 0x21e1a10 (1,708 B) - GXYebisAntiAliasParamBank_ReloadParamFiles
void GXYebisAntiAliasParamBank_ReloadParamFiles(void* self) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x4c);
    uint32_t param_id = DwordAt(scene_mgr, static_cast<ptrdiff_t>(slot) * 4 + 0x588);

    CallVma<void, void*, const void*, uint32_t>(
        0x2936030, PtrAt(self, 0xb8), reinterpret_cast<const void*>(GX_VMA(0x49cbebeULL)), param_id);

    void* file_alloc = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));
    void* loader_obj = AllocatorAlloc(file_alloc, 0x178, 8);
    if (loader_obj) {
        const void* str_ptr = PtrAt(self, 0xc0);
        if (QwordAt(self, 0xd8) >= 8) {
            str_ptr = *reinterpret_cast<const void* const*>(str_ptr);
        }
        uint64_t ctor_arg[2] = {0, reinterpret_cast<uint64_t>(str_ptr)};
        CallVma<void, void*, void*>(0x2362cf0, loader_obj, ctor_arg);
    }

    void* file_sys_root = *reinterpret_cast<void**>(GX_VMA(0x56a6bc8ULL));
    void* file_sys_inner = *reinterpret_cast<void**>(PtrAt(file_sys_root, 0x10));
    void* cur_host = *reinterpret_cast<void**>(PtrAt(file_sys_inner, 0x18));

    struct BindConfig {
        uint16_t a0;
        uint16_t a1;
        uint16_t a2;
        uint16_t a3;
        uint64_t flags;
    } bind_cfg{0, 0, 0x12c, 0x190, 0x51502ULL};

    if (loader_obj && (ByteAt(loader_obj, 0x0b) & 0x40) != 0 && ByteAt(loader_obj, 0x6f) == 0 &&
        cur_host != loader_obj && cur_host != nullptr && (ByteAt(cur_host, 0x0b) & 0x40) != 0) {
        void* host_vtbl = *reinterpret_cast<void**>(cur_host);
        auto check_fn = *reinterpret_cast<int (**)(void*, void*)>(PtrAt(host_vtbl, 0x28));
        if (check_fn(cur_host, loader_obj) != 0) {
            CallVma<void, void*, void*, void*, void*>(
                0x2328e40,
                loader_obj,
                cur_host,
                reinterpret_cast<void*>(QwordAt(cur_host, 0x20)),
                &bind_cfg);
        }
    }

    uint64_t reader_ctx[4]{};
    CallVma<void, void*, void*>(0x236c4d0, reader_ctx, loader_obj);

    uint64_t default_path_pkt[2]{};
    CallVma<void, void*, const void*>(
        0x2ba7d40, default_path_pkt, reinterpret_cast<const void*>(GX_VMA(0x49d27c0ULL)));

    uint64_t open_arg[2]{};
    void* ref_handle = nullptr;
    if (default_path_pkt[1] != 0) {
        open_arg[1] = default_path_pkt[1];
    } else if (default_path_pkt[0] != 0) {
        open_arg[0] = default_path_pkt[0];
        DwordAt(reinterpret_cast<void*>(default_path_pkt[0]), 0x38) += 1;
        ref_handle = reinterpret_cast<void*>(default_path_pkt[0]);
    }

    void* loaded_res = CallVma<void*, void*, void*, uint32_t>(0x236c900, reader_ctx, open_arg, 0);
    ReleaseParamFileHandle(ref_handle);

    if (loaded_res) {
        CallVma<void, void*, void*>(0xe6d0e0, PtrAt(self, 0x168), reader_ctx);
        void** cur_end = reinterpret_cast<void**>(reader_ctx[2]);
        if (reinterpret_cast<void**>(reader_ctx[1]) != cur_end) {
            void* item = *(cur_end - 1);
            if (item) {
                ReleaseContainerObject(item);
                *(cur_end - 1) = nullptr;
                cur_end = reinterpret_cast<void**>(reader_ctx[2]);
            }
            reader_ctx[2] = reinterpret_cast<uint64_t>(cur_end - 1);
        }
    }
    ReleaseParamFileHandle(reinterpret_cast<void*>(default_path_pkt[0]));

    // Lookup outer bucket for param_id
    uint64_t hash = ComputeYebisParamHash(param_id);
    uint64_t bucket = MapHashToBucket(hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
    void* outer_node = *reinterpret_cast<void**>(
        PtrAt(outer_buckets, static_cast<ptrdiff_t>(bucket * 48 + 0x10)));

    if (outer_node != outer_head) {
        void* bucket_end_ptr = *reinterpret_cast<void**>(
            PtrAt(outer_buckets, static_cast<ptrdiff_t>(((bucket * 2) | 1) * 24 + 0x10)));
        void* bucket_end = *reinterpret_cast<void**>(bucket_end_ptr);
        while (outer_node != bucket_end && DwordAt(outer_node, 0x10) < param_id) {
            outer_node = *reinterpret_cast<void**>(outer_node);
        }
        if (outer_node == bucket_end || param_id < DwordAt(outer_node, 0x10)) {
            outer_node = outer_head;
        }
    }

    if (outer_node != outer_head) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        for (void* inner_node = *reinterpret_cast<void**>(inner_head);
             inner_node != inner_head;
             inner_node = *reinterpret_cast<void**>(inner_node)) {
            uint32_t inner_key = DwordAt(inner_node, 0x10);
            if (inner_key == 0xffffffffU) {
                continue;
            }
            uint64_t slot_path_pkt[2]{};
            CallVma<void, void*, const void*, uint32_t>(
                0x2ba7d40,
                slot_path_pkt,
                reinterpret_cast<const void*>(GX_VMA(0x49cbef4ULL)),
                inner_key);

            uint64_t slot_open_arg[2]{};
            void* slot_ref_handle = nullptr;
            if (slot_path_pkt[1] != 0) {
                slot_open_arg[1] = slot_path_pkt[1];
            } else if (slot_path_pkt[0] != 0) {
                slot_open_arg[0] = slot_path_pkt[0];
                DwordAt(reinterpret_cast<void*>(slot_path_pkt[0]), 0x38) += 1;
                slot_ref_handle = reinterpret_cast<void*>(slot_path_pkt[0]);
            }

            void* slot_res = CallVma<void*, void*, void*, uint32_t>(
                0x236c900, reader_ctx, slot_open_arg, 0);
            ReleaseParamFileHandle(slot_ref_handle);

            if (slot_res) {
                CallVma<void, void*, void*>(
                    0xe6d0e0, reinterpret_cast<void*>(QwordAt(inner_node, 0x18)), reader_ctx);
                void** cur_end = reinterpret_cast<void**>(reader_ctx[2]);
                if (reinterpret_cast<void**>(reader_ctx[1]) != cur_end) {
                    void* item = *(cur_end - 1);
                    if (item) {
                        ReleaseContainerObject(item);
                        *(cur_end - 1) = nullptr;
                        cur_end = reinterpret_cast<void**>(reader_ctx[2]);
                    }
                    reader_ctx[2] = reinterpret_cast<uint64_t>(cur_end - 1);
                }
            }
            ReleaseParamFileHandle(reinterpret_cast<void*>(slot_path_pkt[0]));
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }
    }

    CallVma<void, void*>(0x236c720, reader_ctx);
}

// 0x21e17a0 (612 B) - GXYebisAntiAliasParamBank_CreateDebugGuiControls
void GXYebisAntiAliasParamBank_CreateDebugGuiControls(void* self, void* gui_node) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    CallVma<void, void*, void*, uint32_t, uint32_t>(
        0x21877f0, scene_mgr, gui_node, DwordAt(scene_mgr, 0x4c), 0);

    void* cb_alloc = *reinterpret_cast<void**>(GX_VMA(0x5540e08ULL));

    auto register_action_button = [&](uintptr_t label_wstr_vma, void (*fn_ptr)(void*)) {
        uint64_t SubNodeArg[2]{0, 0};
        void* btn_host = CallVma<void*, void*, void*>(0x236d620, gui_node, SubNodeArg);
        uint64_t LabelArg[2]{0, label_wstr_vma};
        void* cb_obj = AllocatorAlloc(cb_alloc, 0x30, 8);
        if (cb_obj) {
            DwordAt(cb_obj, 0x08) = (DwordAt(cb_obj, 0x08) & 0x80000000U) | 0x40000000U;
            QwordAt(cb_obj, 0x00) = GX_VMA(0x53b7060ULL);
            QwordAt(cb_obj, 0x10) = reinterpret_cast<uint64_t>(self);
            DwordAt(cb_obj, 0x18) = 0;
            QwordAt(cb_obj, 0x20) = reinterpret_cast<uint64_t>(fn_ptr);
            QwordAt(cb_obj, 0x28) = 0;
        }
        CallVma<void, void*, void*, void*>(0x235e4c0, btn_host, LabelArg, cb_obj);
    };

    register_action_button(GX_VMA(0x49d2730ULL), &GXYebisAntiAliasParamBank_ReinitializeAllEntries);
    register_action_button(GX_VMA(0x49d275eULL), &GXYebisAntiAliasParamBank_SaveAllEntries);
    register_action_button(GX_VMA(0x49d2790ULL), &GXYebisAntiAliasParamBank_ReloadParamFiles);
}

// 0x21e14c0 (732 B) - GXYebisAntiAliasParamBank_FindEntryByKeys
void* GXYebisAntiAliasParamBank_FindEntryByKeys(void* self, const uint32_t* keys) {
    uint64_t packed = *reinterpret_cast<const uint64_t*>(keys);
    uint32_t outer_key = static_cast<uint32_t>(packed);
    uint32_t inner_key = static_cast<uint32_t>(packed >> 32);
    if (outer_key == 0xffffffffU) {
        if (keys[1] == 0xffffffffU) {
            inner_key = 0;
            outer_key = 1;
        } else {
            outer_key = 0xffffffffU;
        }
    }

    auto find_in_tables = [&](uint32_t o_key, uint32_t i_key) -> void* {
        uint64_t o_hash = ComputeYebisParamHash(o_key);
        uint64_t o_bucket = MapHashToBucket(o_hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
        void* o_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
        void* o_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
        void* o_node = *reinterpret_cast<void**>(
            PtrAt(o_buckets, static_cast<ptrdiff_t>(o_bucket * 48 + 0x10)));
        if (o_node == o_head) {
            return nullptr;
        }
        void* o_end_ptr = *reinterpret_cast<void**>(
            PtrAt(o_buckets, static_cast<ptrdiff_t>(((o_bucket * 2) | 1) * 24 + 0x10)));
        void* o_end = *reinterpret_cast<void**>(o_end_ptr);
        while (o_node != o_end && DwordAt(o_node, 0x10) < o_key) {
            o_node = *reinterpret_cast<void**>(o_node);
        }
        if (o_node == o_end || o_key < DwordAt(o_node, 0x10)) {
            return nullptr;
        }

        void* i_table = reinterpret_cast<void*>(QwordAt(o_node, 0x18));
        uint64_t i_hash = ComputeYebisParamHash(i_key);
        uint64_t i_bucket = MapHashToBucket(i_hash, QwordAt(i_table, 0x48), QwordAt(i_table, 0x50));
        void* i_head = reinterpret_cast<void*>(QwordAt(i_table, 0x08));
        void* i_buckets = reinterpret_cast<void*>(QwordAt(i_table, 0x28));
        void* i_node = *reinterpret_cast<void**>(
            PtrAt(i_buckets, static_cast<ptrdiff_t>(i_bucket * 48 + 0x10)));
        if (i_node == i_head) {
            return nullptr;
        }
        void* i_end_ptr = *reinterpret_cast<void**>(
            PtrAt(i_buckets, static_cast<ptrdiff_t>(((i_bucket * 2) | 1) * 24 + 0x10)));
        void* i_end = *reinterpret_cast<void**>(i_end_ptr);
        while (i_node != i_end && DwordAt(i_node, 0x10) < i_key) {
            i_node = *reinterpret_cast<void**>(i_node);
        }
        if (i_node == i_end || i_key < DwordAt(i_node, 0x10)) {
            return nullptr;
        }
        return reinterpret_cast<void*>(QwordAt(i_node, 0x18));
    };

    if (void* found = find_in_tables(outer_key, inner_key)) {
        return found;
    }
    if (void* fallback = find_in_tables(1u, 0u)) {
        return fallback;
    }
    return PtrAt(self, 0x168);
}

// 0x21e1450 (103 B) - GXYebisAntiAliasParamBank_SaveAllEntries
void GXYebisAntiAliasParamBank_SaveAllEntries(void* self) {
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    for (void* outer_node = *reinterpret_cast<void**>(outer_head);
         outer_node != outer_head;
         outer_node = *reinterpret_cast<void**>(outer_node)) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        for (void* inner_node = *reinterpret_cast<void**>(inner_head);
             inner_node != inner_head;
             inner_node = *reinterpret_cast<void**>(inner_node)) {
            CallVma<void, void*>(0xe6c950, reinterpret_cast<void*>(QwordAt(inner_node, 0x18)));
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }
        outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    }
}

// 0x21e13c0 (132 B) - GXYebisAntiAliasParamBank_ReinitializeAllEntries
void GXYebisAntiAliasParamBank_ReinitializeAllEntries(void* self) {
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    for (void* outer_node = *reinterpret_cast<void**>(outer_head);
         outer_node != outer_head;
         outer_node = *reinterpret_cast<void**>(outer_node)) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        for (void* inner_node = *reinterpret_cast<void**>(inner_head);
             inner_node != inner_head;
             inner_node = *reinterpret_cast<void**>(inner_node)) {
            void* entry = reinterpret_cast<void*>(QwordAt(inner_node, 0x18));
            uint32_t inner_key = DwordAt(inner_node, 0x10);
            uint32_t outer_key = DwordAt(outer_node, 0x10);
            DwordAt(entry, 0x78) = inner_key;
            DwordAt(entry, 0x7c) = outer_key;
            CallVma<void, void*, uint32_t, float>(0xe6c9b0, entry, inner_key, 0.0f);
            CallVma<void, void*, void*>(
                0xe6cae0,
                reinterpret_cast<void*>(QwordAt(inner_node, 0x18)),
                reinterpret_cast<void*>(QwordAt(self, 0x00)));
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }
        outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    }
}

// 0x21e13b0 (1 B) - GXYebisAntiAliasParamBank_NoOp
void GXYebisAntiAliasParamBank_NoOp(void*) {
    return;
}

// 0x21e1380 (40 B) - GXYebisAntiAliasParamBank_SelectActiveEntry
void GXYebisAntiAliasParamBank_SelectActiveEntry(void* /*self*/, uint32_t key0, uint32_t key1) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x4c);
    CallVma<void, void*, uint32_t, uint32_t, uint32_t, int64_t>(
        0x2186da0, scene_mgr, slot, key0, key1, -1LL);
}

// 0x21e10c0 (698 B) - GXYebisAntiAliasParamBank_PurgeEntriesForOuterKey
void GXYebisAntiAliasParamBank_PurgeEntriesForOuterKey(
    void* self,
    void* /*unused_rsi*/,
    uint32_t outer_key,
    uint32_t override_slot_idx,
    float blend_time) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x4c);
    CallVma<void, void*, uint32_t, uint32_t, uint32_t, float>(
        0x2185b50, scene_mgr, slot, outer_key, override_slot_idx, blend_time);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = 1;

    uint64_t hash = ComputeYebisParamHash(outer_key);
    uint64_t bucket = MapHashToBucket(hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
    void* outer_node = *reinterpret_cast<void**>(
        PtrAt(outer_buckets, static_cast<ptrdiff_t>(bucket * 48 + 0x10)));
    if (outer_node == outer_head) {
        return;
    }

    void* bucket_end_ptr = *reinterpret_cast<void**>(
        PtrAt(outer_buckets, static_cast<ptrdiff_t>(((bucket * 2) | 1) * 24 + 0x10)));
    void* bucket_end = *reinterpret_cast<void**>(bucket_end_ptr);
    while (outer_node != bucket_end && DwordAt(outer_node, 0x10) < outer_key) {
        outer_node = *reinterpret_cast<void**>(outer_node);
    }
    if (outer_node == bucket_end || outer_key < DwordAt(outer_node, 0x10) || outer_node == outer_head) {
        return;
    }

    void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
    if (inner_table) {
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        void* inner_node = *reinterpret_cast<void**>(inner_head);
        while (inner_node != inner_head) {
            bool keep = CallVma<bool, void*, uint32_t, uint32_t, uint32_t>(
                0x2185e00, scene_mgr, slot, DwordAt(inner_node, 0x10), outer_key);
            void* entry = reinterpret_cast<void*>(QwordAt(inner_node, 0x18));
            if (keep) {
                ByteAt(entry, 0x70) = 1;
                inner_node = *reinterpret_cast<void**>(inner_node);
            } else {
                if (entry) {
                    void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
                    InvokeVirtualDtor0(entry);
                    AllocatorFree(alloc, entry);
                    QwordAt(inner_node, 0x18) = 0;
                }
                uint64_t erase_iter[3]{};
                GXYebisAntiAliasInnerHashTable_EraseNode(
                    erase_iter, reinterpret_cast<void*>(QwordAt(outer_node, 0x18)), inner_node);
                inner_node = reinterpret_cast<void*>(erase_iter[2]);
            }
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        }

        if (QwordAt(inner_table, 0x10) != 0) {
            return;
        }

        void* main_alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
        void* inner_buckets = reinterpret_cast<void*>(QwordAt(inner_table, 0x28));
        if (inner_buckets) {
            AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x40)), inner_buckets);
            QwordAt(inner_table, 0x38) = 0;
            QwordAt(inner_table, 0x30) = 0;
            QwordAt(inner_table, 0x28) = 0;
        }
        inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        void* cur = *reinterpret_cast<void**>(inner_head);
        QwordAt(inner_head, 0x00) = reinterpret_cast<uint64_t>(inner_head);
        QwordAt(inner_head, 0x08) = reinterpret_cast<uint64_t>(inner_head);
        QwordAt(inner_table, 0x10) = 0;
        while (cur != inner_head) {
            void* next = *reinterpret_cast<void**>(cur);
            AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), cur);
            cur = next;
        }
        AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), inner_head);
        AllocatorFree(main_alloc, inner_table);
        QwordAt(outer_node, 0x18) = 0;
    }

    uint64_t outer_erase_iter[3]{};
    GXYebisAntiAliasOuterHashTable_EraseNode(outer_erase_iter, PtrAt(self, 0x08), outer_node);
}

// 0x21e0940 (1,912 B) - GXYebisAntiAliasParamBank_EnsureAndPopulateOuterKey
void GXYebisAntiAliasParamBank_EnsureAndPopulateOuterKey(
    void* self, void* context, uint32_t outer_key, uint32_t flags, float weight) {
    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t slot = DwordAt(scene_mgr, 0x4c);

    int32_t count = CallVma<int32_t, void*, uint32_t, void*>(0x2184cf0, scene_mgr, slot, context);
    if (count > 0) {
        uint64_t hash = ComputeYebisParamHash(outer_key);
        uint64_t bucket = MapHashToBucket(hash, QwordAt(self, 0x50), QwordAt(self, 0x58));
        void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
        void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
        void* outer_node = *reinterpret_cast<void**>(
            PtrAt(outer_buckets, static_cast<ptrdiff_t>(bucket * 48 + 0x10)));

        if (outer_node != outer_head) {
            void* bucket_end_ptr = *reinterpret_cast<void**>(
                PtrAt(outer_buckets, static_cast<ptrdiff_t>(((bucket * 2) | 1) * 24 + 0x10)));
            void* bucket_end = *reinterpret_cast<void**>(bucket_end_ptr);
            while (outer_node != bucket_end && DwordAt(outer_node, 0x10) < outer_key) {
                outer_node = *reinterpret_cast<void**>(outer_node);
            }
            if (outer_node == bucket_end || outer_key < DwordAt(outer_node, 0x10)) {
                outer_node = outer_head;
            }
        }

        if (outer_node == outer_head) {
            void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
            void* new_inner = AllocatorAlloc(alloc, 0x60, 8);
            if (new_inner) {
                uint8_t caps[32]{};
                AllocatorQueryCaps(alloc, caps);
                if ((caps[0] & 0x20) == 0) {
                    CallVma<void, uint32_t, uint32_t, const void*>(
                        0x20b55b0, 0, 0x3e, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
                }
                QwordAt(new_inner, 0x10) = 0;
                QwordAt(new_inner, 0x08) = 0;
                QwordAt(new_inner, 0x18) = reinterpret_cast<uint64_t>(alloc);
                void* sentinel = AllocatorAlloc(alloc, 0x20, 8);
                if (sentinel) {
                    QwordAt(sentinel, 0x00) = reinterpret_cast<uint64_t>(sentinel);
                    QwordAt(sentinel, 0x08) = reinterpret_cast<uint64_t>(sentinel);
                }
                QwordAt(new_inner, 0x08) = reinterpret_cast<uint64_t>(sentinel);
                QwordAt(new_inner, 0x38) = 0;
                QwordAt(new_inner, 0x30) = 0;
                QwordAt(new_inner, 0x28) = 0;
                QwordAt(new_inner, 0x40) = reinterpret_cast<uint64_t>(alloc);
                DwordAt(new_inner, 0x58) = 0x3f800000U;

                const YebisBucketIter fill_iter{nullptr, nullptr, sentinel};
                YebisBucketIter out_iter{};
                GXYebisAntiAliasInnerHashTable_RehashOrInsertRange(
                    &out_iter,
                    PtrAt(new_inner, 0x20),
                    YebisBucketIter{nullptr, nullptr, nullptr},
                    0x10,
                    &fill_iter);
                QwordAt(new_inner, 0x48) = 7;
                QwordAt(new_inner, 0x50) = 8;
            }

            void* outer_sentinel = reinterpret_cast<void*>(QwordAt(self, 0x10));
            void* outer_first = *reinterpret_cast<void**>(outer_sentinel);
            void* new_outer_node = AllocatorAlloc(reinterpret_cast<void*>(QwordAt(self, 0x20)), 0x20, 8);
            void* prev_node = outer_first ? reinterpret_cast<void*>(QwordAt(outer_first, 0x08)) : new_outer_node;
            QwordAt(new_outer_node, 0x00) = reinterpret_cast<uint64_t>(outer_first ? outer_first : new_outer_node);
            QwordAt(new_outer_node, 0x08) = reinterpret_cast<uint64_t>(prev_node);
            DwordAt(new_outer_node, 0x10) = outer_key;
            QwordAt(new_outer_node, 0x18) = reinterpret_cast<uint64_t>(new_inner);

            QwordAt(self, 0x18) += 1;
            QwordAt(outer_first, 0x08) = reinterpret_cast<uint64_t>(new_outer_node);
            QwordAt(prev_node, 0x00) = reinterpret_cast<uint64_t>(new_outer_node);

            void* cur_front = *reinterpret_cast<void**>(reinterpret_cast<void*>(QwordAt(self, 0x10)));
            uint64_t ins_out[4]{};
            GXYebisAntiAliasOuterHashTable_InsertNode(
                ins_out,
                PtrAt(self, 0x08),
                PtrAt(cur_front, 0x10),
                YebisBucketIter{nullptr, nullptr, cur_front});
            outer_node = reinterpret_cast<void*>(ins_out[2]);
        }

        for (int32_t i = 0; i < count; ++i) {
            uint32_t inner_key = CallVma<uint32_t, void*, uint32_t, void*, int32_t>(
                0x2184de0, scene_mgr, slot, context, i);
            void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            uint64_t i_hash = ComputeYebisParamHash(inner_key);
            uint64_t i_bucket = MapHashToBucket(
                i_hash, QwordAt(inner_table, 0x48), QwordAt(inner_table, 0x50));
            void* i_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
            void* i_buckets = reinterpret_cast<void*>(QwordAt(inner_table, 0x28));
            void* i_cand = *reinterpret_cast<void**>(
                PtrAt(i_buckets, static_cast<ptrdiff_t>(i_bucket * 48 + 0x10)));
            void* found_inner = i_head;
            if (i_cand != i_head) {
                void* i_end_ptr = *reinterpret_cast<void**>(
                    PtrAt(i_buckets, static_cast<ptrdiff_t>(((i_bucket * 2) | 1) * 24 + 0x10)));
                void* i_end = *reinterpret_cast<void**>(i_end_ptr);
                while (i_cand != i_end && DwordAt(i_cand, 0x10) < inner_key) {
                    i_cand = *reinterpret_cast<void**>(i_cand);
                }
                if (i_cand != i_end && inner_key >= DwordAt(i_cand, 0x10)) {
                    found_inner = i_cand;
                }
            }

            if (found_inner != reinterpret_cast<void*>(QwordAt(inner_table, 0x08))) {
                ByteAt(reinterpret_cast<void*>(QwordAt(found_inner, 0x18)), 0x70) = 1;
            } else {
                void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
                void* new_entry = AllocatorAlloc(alloc, 0x80, 8);
                if (new_entry) {
                    CallVma<void, void*>(0xe6c6f0, new_entry);
                    CallVma<void, void*>(0xe6c7b0, new_entry);
                    DwordAt(new_entry, 0x78) = inner_key;
                    DwordAt(new_entry, 0x7c) = outer_key;
                    CallVma<void, void*, uint32_t, float>(0xe6c9b0, new_entry, inner_key, 0.0f);
                    CallVma<void, void*, void*>(0xe6cae0, new_entry, alloc);
                }

                inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
                void* i_sentinel = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
                void* i_first = *reinterpret_cast<void**>(i_sentinel);
                void* new_i_node = AllocatorAlloc(
                    reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), 0x20, 8);
                void* i_prev = i_first ? reinterpret_cast<void*>(QwordAt(i_first, 0x08)) : new_i_node;
                QwordAt(new_i_node, 0x00) = reinterpret_cast<uint64_t>(i_first ? i_first : new_i_node);
                QwordAt(new_i_node, 0x08) = reinterpret_cast<uint64_t>(i_prev);
                DwordAt(new_i_node, 0x10) = inner_key;
                QwordAt(new_i_node, 0x18) = reinterpret_cast<uint64_t>(new_entry);

                QwordAt(inner_table, 0x10) += 1;
                QwordAt(i_first, 0x08) = reinterpret_cast<uint64_t>(new_i_node);
                QwordAt(i_prev, 0x00) = reinterpret_cast<uint64_t>(new_i_node);

                void* cur_i_front = *reinterpret_cast<void**>(
                    reinterpret_cast<void*>(QwordAt(inner_table, 0x08)));
                uint64_t ins_i_out[4]{};
                GXYebisAntiAliasInnerHashTable_InsertNode(
                    ins_i_out,
                    inner_table,
                    PtrAt(cur_i_front, 0x10),
                    YebisBucketIter{nullptr, nullptr, cur_i_front});
            }
        }
    }

    CallVma<void, void*, uint32_t, uint32_t, uint32_t, void*, float>(
        0x2184ff0, scene_mgr, slot, outer_key, flags, context, weight);
}

// 0x21e02b0 (1,680 B) - GXYebisAntiAliasParamBank_RegisterDebugGuiProperties
bool GXYebisAntiAliasParamBank_RegisterDebugGuiProperties(void* self) {
    GXYebisAntiAliasParamBank_ClearAllEntries(self);

    void* root_singleton = *reinterpret_cast<void**>(GX_VMA(0x5540dd8ULL));
    void* scene_mgr = *reinterpret_cast<void**>(PtrAt(root_singleton, 0x5ad0));
    uint32_t group_id = 0;

    // Register "Anti-Alias[Yebis]" (0x49c3f20 / 0x49c3efc)
    CallVma<void, void*, uint32_t*, const void*, const void*>(
        0xe32510,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        &group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c3f20ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c3efcULL)));
    DwordAt(scene_mgr, 0x4c) = group_id;

    // Open group "Anti-Alias" (0x49c3f36)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c3f20ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c3f36ULL)),
        1u,
        1u);

    const uintptr_t kFloatGetVtbl = GX_VMA(0x52e83f0ULL) + 0x10;
    const uintptr_t kFloatSetVtbl = GX_VMA(0x52e4890ULL) + 0x10;
    const uintptr_t kBoolGetVtbl  = GX_VMA(0x52e8760ULL) + 0x10;
    const uintptr_t kBoolSetVtbl  = GX_VMA(0x52e4a70ULL) + 0x10;

    // +0x68: Anti-Alias Enable (bool [0, 1], flags 1)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c3f5aULL), PtrAt(self, 0x68),
        0x03, 0x01, kBoolGetVtbl, kBoolSetVtbl,
        0x00000000U, 0x00000001U, 0x00000000U, 0x3f800000U, 0u);

    // +0x6c: Anti-Alias Type (enum 5 items from 0x535feb0, 0x50 bytes via 0x29358f0)
    uint8_t aa_enum_buf[0x50];
    std::memcpy(aa_enum_buf, reinterpret_cast<const void*>(GX_VMA(0x535feb0ULL)), 0x50);
    CallVma<void, void*, uint32_t, const void*, const void*, void*, uint32_t, const void*, uint32_t>(
        0x29358f0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c3fe4ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c4000ULL)),
        PtrAt(self, 0x6c),
        5u,
        aa_enum_buf,
        1u);

    // Open sub-group "FXAA" (0x49c403e / 0x49c4034)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c4034ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c403eULL)),
        1u,
        1u);

    // +0x70: FXAA Subpix (float [0.0, 1.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c406cULL), GX_VMA(0x49c4080ULL), PtrAt(self, 0x70),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x74: FXAA EdgeThreshold (float [0.0, 1.0], step 0x3a4ccccd)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c40aeULL), GX_VMA(0x49c40caULL), PtrAt(self, 0x74),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3a4ccccdU, 0x3f800000U, 1u);

    // +0x78: FXAA EdgeThresholdMin (float [0.0, 1.0], step 0.0078125 = 0x3c000000)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c40feULL), GX_VMA(0x49c411aULL), PtrAt(self, 0x78),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c000000U, 0x3f800000U, 1u);

    // +0x7c: FXAA Sharpness (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c414eULL), GX_VMA(0x49c4166ULL), PtrAt(self, 0x7c),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // Close sub-group "FXAA"
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);

    // Open sub-group "DLAA" (0x49c41a0 / 0x49c4196)
    CallVma<void, void*, uint32_t, const void*, const void*, uint32_t, uint32_t>(
        0xe333e0,
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id,
        reinterpret_cast<const void*>(GX_VMA(0x49c4196ULL)),
        reinterpret_cast<const void*>(GX_VMA(0x49c41a0ULL)),
        1u,
        1u);

    // +0x80: DLAA ShortEdgeLambda (float [0.0, 1.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c41ceULL), GX_VMA(0x49c41eaULL), PtrAt(self, 0x80),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x84: DLAA LongEdgeLambda (float [0.0, 10.0], step 0.01)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c421eULL), GX_VMA(0x49c4232ULL), PtrAt(self, 0x84),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x41200000U, 0x3c23d70aU, 0x3f800000U, 1u);

    // +0x88: DLAA LongEdgeThreshold (float [0.0, 1.0], step 0.0001 = 0x38d1b717)
    RegisterGuiScalarProp(
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        group_id, GX_VMA(0x49c425eULL), GX_VMA(0x49c4276ULL), PtrAt(self, 0x88),
        0x09, 0x00, kFloatGetVtbl, kFloatSetVtbl,
        0x00000000U, 0x3f800000U, 0x38d1b717U, 0x3f800000U, 1u);

    // Close "DLAA" and "Anti-Alias"
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);
    CallVma<void, void*, uint32_t, uint32_t>(
        0xe33600, reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), group_id, 1u);

    CallVma<void, void*, void*, uint32_t>(
        0x218e2b0, PtrAt(scene_mgr, 0x130), reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)), 0u);
    CallVma<void, void*, void*, uint32_t, uint32_t, uint32_t, float>(
        0xe41f00,
        PtrAt(scene_mgr, 0x1b8),
        reinterpret_cast<void*>(QwordAt(scene_mgr, 0x28)),
        0u,
        group_id,
        0u,
        0.0f);
    CallVma<void, void*, uint32_t>(0x21874c0, scene_mgr, group_id);
    return true;
}

// 0x21e0120 (395 B) - GXYebisAntiAliasParamBank_ClearAllEntries
void GXYebisAntiAliasParamBank_ClearAllEntries(void* self) {
    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* outer_node = *reinterpret_cast<void**>(outer_head);
    while (outer_node != outer_head) {
        void* inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        void* inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
        void* inner_node = *reinterpret_cast<void**>(inner_head);
        while (inner_node != inner_head) {
            void* entry = reinterpret_cast<void*>(QwordAt(inner_node, 0x18));
            if (entry) {
                void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
                InvokeVirtualDtor0(entry);
                AllocatorFree(alloc, entry);
                QwordAt(inner_node, 0x18) = 0;
                inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
            }
            uint64_t erase_iter[3]{};
            GXYebisAntiAliasInnerHashTable_EraseNode(erase_iter, inner_table, inner_node);
            inner_node = reinterpret_cast<void*>(erase_iter[2]);
            inner_table = reinterpret_cast<void*>(QwordAt(outer_node, 0x18));
        }

        if (inner_table) {
            void* alloc = reinterpret_cast<void*>(QwordAt(self, 0x00));
            void* inner_buckets = reinterpret_cast<void*>(QwordAt(inner_table, 0x28));
            if (inner_buckets) {
                AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x40)), inner_buckets);
                QwordAt(inner_table, 0x38) = 0;
                QwordAt(inner_table, 0x30) = 0;
                QwordAt(inner_table, 0x28) = 0;
            }
            inner_head = reinterpret_cast<void*>(QwordAt(inner_table, 0x08));
            void* cur = *reinterpret_cast<void**>(inner_head);
            QwordAt(inner_head, 0x00) = reinterpret_cast<uint64_t>(inner_head);
            QwordAt(inner_head, 0x08) = reinterpret_cast<uint64_t>(inner_head);
            QwordAt(inner_table, 0x10) = 0;
            while (cur != inner_head) {
                void* next = *reinterpret_cast<void**>(cur);
                AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), cur);
                cur = next;
            }
            AllocatorFree(reinterpret_cast<void*>(QwordAt(inner_table, 0x18)), inner_head);
            AllocatorFree(alloc, inner_table);
            QwordAt(outer_node, 0x18) = 0;
        }

        uint64_t outer_erase_iter[3]{};
        GXYebisAntiAliasOuterHashTable_EraseNode(outer_erase_iter, PtrAt(self, 0x08), outer_node);
        outer_node = reinterpret_cast<void*>(outer_erase_iter[2]);
    }
}

// 0x21e0020 (255 B) - GXYebisAntiAliasParamBank_Destruct
void GXYebisAntiAliasParamBank_Destruct(void* self) {
    GXYebisAntiAliasParamBank_ClearAllEntries(self);
    CallVma<void, void*>(0xe6c820, PtrAt(self, 0x168));

    uint64_t str_cap = QwordAt(self, 0xd8);
    if (str_cap >= 8) {
        if (str_cap + 1 >= 0x41) {
            CallVma<void, const void*, uint32_t, const void*>(
                0x20b55b0,
                reinterpret_cast<const void*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const void*>(GX_VMA(0x4940300ULL)));
        }
        ByteAt(self, 0x161) &= 0xfdU;
    }
    QwordAt(self, 0xd8) = 7;
    QwordAt(self, 0xd0) = 0;
    WordAt(self, 0xc0) = 0;

    void* outer_buckets = reinterpret_cast<void*>(QwordAt(self, 0x30));
    if (outer_buckets) {
        AllocatorFree(reinterpret_cast<void*>(QwordAt(self, 0x48)), outer_buckets);
        QwordAt(self, 0x40) = 0;
        QwordAt(self, 0x38) = 0;
        QwordAt(self, 0x30) = 0;
    }

    void* outer_head = reinterpret_cast<void*>(QwordAt(self, 0x10));
    void* cur = *reinterpret_cast<void**>(outer_head);
    QwordAt(outer_head, 0x00) = reinterpret_cast<uint64_t>(outer_head);
    QwordAt(outer_head, 0x08) = reinterpret_cast<uint64_t>(outer_head);
    QwordAt(self, 0x18) = 0;
    while (cur != outer_head) {
        void* next = *reinterpret_cast<void**>(cur);
        AllocatorFree(reinterpret_cast<void*>(QwordAt(self, 0x20)), cur);
        cur = next;
    }
    AllocatorFree(reinterpret_cast<void*>(QwordAt(self, 0x20)), outer_head);
}

// 0x21dfd20 (764 B) - GXYebisAntiAliasParamBank_Construct
void GXYebisAntiAliasParamBank_Construct(void* self, void* allocator) {
    QwordAt(self, 0x00) = reinterpret_cast<uint64_t>(allocator);
    uint8_t caps[136]{};
    AllocatorQueryCaps(allocator, caps);
    if ((caps[0] & 0x20) == 0) {
        CallVma<void, uint32_t, uint32_t, const void*>(
            0x20b55b0, 0, 0x3e, reinterpret_cast<const void*>(GX_VMA(0x4737dc0ULL)));
    }

    QwordAt(self, 0x18) = 0;
    QwordAt(self, 0x10) = 0;
    QwordAt(self, 0x20) = reinterpret_cast<uint64_t>(allocator);
    void* sentinel = AllocatorAlloc(allocator, 0x20, 8);
    if (sentinel) {
        QwordAt(sentinel, 0x00) = reinterpret_cast<uint64_t>(sentinel);
        QwordAt(sentinel, 0x08) = reinterpret_cast<uint64_t>(sentinel);
    }
    QwordAt(self, 0x10) = reinterpret_cast<uint64_t>(sentinel);

    QwordAt(self, 0x40) = 0;
    QwordAt(self, 0x38) = 0;
    QwordAt(self, 0x30) = 0;
    QwordAt(self, 0x48) = reinterpret_cast<uint64_t>(allocator);
    DwordAt(self, 0x60) = 0x3f800000U;

    const YebisBucketIter fill_iter{nullptr, nullptr, sentinel};
    YebisBucketIter out_iter{};
    GXYebisAntiAliasOuterHashTable_RehashOrInsertRange(
        &out_iter,
        PtrAt(self, 0x28),
        YebisBucketIter{nullptr, nullptr, nullptr},
        0x10,
        &fill_iter);
    QwordAt(self, 0x50) = 7;
    QwordAt(self, 0x58) = 8;

    DwordAt(self, 0xb0) = 0;
    void* dl_str = PtrAt(self, 0xb8);
    std::memcpy(PtrAt(self, 0xe0), caps, 0x81);
    ByteAt(self, 0x161) = 0;
    QwordAt(self, 0xd8) = 7;
    QwordAt(self, 0xd0) = 0;
    WordAt(self, 0xc0) = 0;
    ByteAt(self, 0x162) = 1;
    CallVma<void, void*, uint64_t, uint64_t>(0x2935350, dl_str, 0x3fULL, 0ULL);
    QwordAt(self, 0xd0) = 0;
    void* str_buf = PtrAt(self, 0xc0);
    if (QwordAt(self, 0xd8) >= 8) {
        str_buf = *reinterpret_cast<void**>(str_buf);
    }
    *reinterpret_cast<uint16_t*>(str_buf) = 0;

    void* default_entry = PtrAt(self, 0x168);
    CallVma<void, void*>(0xe6c6f0, default_entry);
    CallVma<void, void*>(0xe6c7b0, default_entry);

    // Initialize default Anti-Alias parameters at +0x68..+0x88 and copy to backup +0x8c..+0xac
    DwordAt(self, 0x68) = 0;
    DwordAt(self, 0x6c) = 4;
    QwordAt(self, 0x70) = 0x3c0000003f800000ULL; // Subpix=1.0f, EdgeThreshold=0.0078125f
    QwordAt(self, 0x78) = 0x410000003e000000ULL; // EdgeThresholdMin=0.125f, Sharpness=8.0f
    QwordAt(self, 0x80) = 0x400000003dcccccdULL; // ShortEdgeLambda=0.1f, LongEdgeLambda=2.0f
    DwordAt(self, 0x88) = 0x3e800000U;           // LongEdgeThreshold=0.25f

    QwordAt(self, 0xa4) = QwordAt(self, 0x80);
    QwordAt(self, 0x9c) = QwordAt(self, 0x78);
    QwordAt(self, 0x94) = QwordAt(self, 0x70);
    QwordAt(self, 0x8c) = QwordAt(self, 0x68);
    DwordAt(self, 0xac) = 0x3e800000U;
    DwordAt(self, 0xb0) = 0;

    CallVma<void, void*, const void*, uint64_t>(
        0x29354e0, dl_str, reinterpret_cast<const void*>(GX_VMA(0x49d85f0ULL)), 0ULL);
}

} // extern "C"
