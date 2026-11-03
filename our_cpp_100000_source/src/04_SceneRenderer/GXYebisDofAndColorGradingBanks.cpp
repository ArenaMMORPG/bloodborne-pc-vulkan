#include <cstdint>
#include <cstddef>
#include <cstring>

// ============================================================================
// Module 04: GXYebisDofAndColorGradingBanks.cpp (0x21e6f60 .. 0x21f1570, 55 functions)
// Priority Tier 8/10: Scene Renderer & Yebis Post-Process Parameter Banks
//   - Group E Constructor (FeedBackBlur[Yebis]): 0x21f1570 (1 function)
//   - Group F (DepthOfField[Yebis] / Dof[Yebis]): 0x21ec470 .. 0x21f1020 (22 functions)
//   - Group G (ColorGrading[Yebis]): 0x21e8960 .. 0x21ebf20 (22 functions)
//   - Group H Upper Half + DebugGui Enum Helper: 0x21e6f60 .. 0x21e8410 (10 functions)
// ============================================================================

namespace {

struct DLAllocatorVtable {
    void* vfunc_0x00;
    void* vfunc_0x08;
    void* vfunc_0x10;
    void* vfunc_0x18;
    void (*GetHeapInfo)(void* out_info, void* self, uint32_t flags);
    void* vfunc_0x28;
    void* vfunc_0x30;
    void* vfunc_0x38;
    void* vfunc_0x40;
    void* vfunc_0x48;
    void* (*AllocateRaw)(void* self, size_t size);
    void* (*AllocateAligned)(void* self, size_t size, size_t alignment);
    void* vfunc_0x60;
    void* vfunc_0x68;
    void (*Free)(void* self, void* ptr);
};

struct DLAllocator {
    DLAllocatorVtable* vptr;
};

struct HashNode {
    HashNode* next;
    HashNode* prev;
    uint32_t key;
    uint32_t pad;
    void* value;
};

struct HashIteratorPair {
    HashNode* first;
    HashNode* second;
    HashNode* third;
};

struct InsertResult {
    uint64_t proxy0;
    uint64_t proxy1;
    HashNode* node;
    uint8_t inserted;
    uint8_t pad[7];
};

extern "C" {
    void sub_20b55b0(const char* file, uint32_t line, const char* msg, ...);
    uint64_t sub_2bbfd68(uint64_t x, uint64_t y, uint64_t* rem_out);
    void* sub_2bbe578(void* dst, const void* src, size_t len);
    void sub_2bbe588(void* ptr);
    void* sub_2bbe688(void* dst, int val, size_t len);
    void sub_2bbe978(void* tls_ptr, uint32_t code);
    void sub_2bbf1b8(const char* msg);
    size_t sub_2bbf228(const char16_t* str);
    char16_t* sub_2bbf608(char16_t* dst, const char16_t* src);

    void sub_29065b0(void* vec, size_t add_count);
    void sub_29097e0(void* str, size_t cap, size_t old_len);
    void sub_2935150(void* dst_str, const void* src_str, size_t subpos, size_t sublen);
    void sub_2935350(void* str, size_t cap, size_t old_len);
    void sub_29354e0(void* str, const char16_t* src, size_t len);
    void sub_29356b0(void* entry_str_pair);
    void sub_2936030(void* str, const char16_t* fmt, uint32_t arg);
    void sub_192b330(void* str, const char16_t* fmt, const void* s1, const char16_t* s2);

    void sub_2328e40(void* gui_node, void* parent_node, void* ctx, void* rect);
    void sub_235e4c0(void* btn, void* label_desc, void* callback_obj);
    void sub_2362cf0(void* gui_obj, void* name_desc);
    void sub_236c4d0(void* list_builder, void* gui_obj);
    void sub_236c720(void* list_builder);
    void* sub_236c900(void* list_builder, void* item_desc, uint32_t flags);
    void* sub_236d620(void* parent_gui, void* btn_desc);
    void sub_2ba7d40(void* out_str, const char16_t* fmt, ...);

    uint32_t sub_2183dd0(void* param_sys, void* ctx);
    const void* sub_2183880(void* param_sys, void* ctx);
    uint32_t sub_2184cf0(void* param_sys, uint32_t slot, void* ctx);
    uint32_t sub_2184de0(void* param_sys, uint32_t slot, void* ctx, uint32_t index);
    void sub_2184ff0(void* param_sys, uint32_t slot, uint32_t bank_id, uint32_t unk, void* ctx, float weight);
    void sub_2185b50(void* param_sys, uint32_t slot, uint32_t bank_id, uint32_t override_slot_idx, float blend_time);
    bool sub_2185e00(void* param_sys, uint32_t slot, uint32_t inner_id, uint32_t bank_id);
    void* sub_2186da0(void* param_sys, uint32_t slot, uint32_t bank_id, uint32_t inner_id, int64_t fallback);
    void sub_21874c0(void* param_sys, uint32_t slot);
    void sub_21877f0(void* param_sys, void* unused_rsi, uint32_t slot, uint32_t flags);
    void sub_218e2b0(void* sub_obj, void* gui_mgr, uint32_t flags);

    void sub_e32510(void* gui_mgr, uint32_t* out_slot, const char16_t* short_name, const char16_t* full_name);
    void sub_e327d0(void* gui_mgr, uint32_t slot, const char16_t* name, const char16_t* desc, void* prop_spec, uint32_t count, uint32_t flags);
    void sub_e333e0(void* gui_mgr, uint32_t slot, const char16_t* name, const char16_t* desc, uint32_t a5, uint32_t a6);
    void sub_e33600(void* gui_mgr, uint32_t slot, uint32_t depth);
    void sub_e41f00(void* sub_obj, void* gui_mgr, uint32_t a3, uint32_t slot, uint32_t a5, float a6);

    // Group E external entry helpers (FeedBackBlur[Yebis])
    void sub_e789d0(void* entry);
    void sub_e78a80(void* entry);
    HashIteratorPair* sub_21f4380(HashIteratorPair* out_pair, void* table_vec, HashIteratorPair where_iter, uint64_t new_bucket_count, const HashIteratorPair* fill_val);

    // Group F external entry helpers (DepthOfField[Yebis])
    void sub_e75d90(void* entry);
    void sub_e75f70(void* entry);
    void sub_e760a0(void* entry);
    void sub_e761f0(void* entry);
    void sub_e76250(void* entry, uint32_t inner_id, float weight);
    void sub_e763c0(void* entry, void* alloc);
    void sub_e779a0(void* entry, void* list_builder);

    // Group G external entry helpers (ColorGrading[Yebis])
    void sub_e715a0(void* entry);
    void sub_e71740(void* entry);
    void sub_e71830(void* entry);
    void sub_e71bf0(void* entry);
    void sub_e71c50(void* entry, uint32_t inner_id, float weight);
    void sub_e71dc0(void* entry, void* alloc);
    void sub_e72030(void* entry, const void* str_Or_path);
    void sub_e720c0(void* entry, void* scene_cam, float weight);
    void sub_e72260(void* entry, void* list_builder);
}

static inline uint64_t ComputeParamBankHash(uint64_t raw_key) {
    int64_t x = static_cast<int64_t>(raw_key);
    int64_t quot = x / 0x1f31dLL;
    int64_t rem = x % 0x1f31dLL;
    int64_t prod1 = rem * 0x41a7LL;
    int64_t prod2 = quot * -2836LL; // 0xfffffffffffff4ec
    int64_t sum = prod1 + prod2;
    uint64_t u_sum = static_cast<uint64_t>(sum);
    if (sum < 0) {
        u_sum += 0x7fffffffffffffffULL;
    }
    return u_sum;
}

static inline HashIteratorPair* RehashInsert_Impl(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    uint8_t* begin_ptr = *reinterpret_cast<uint8_t**>(table_vec + 0x08);
    uint8_t* end_ptr = *reinterpret_cast<uint8_t**>(table_vec + 0x10);
    uint8_t* cap_ptr = *reinterpret_cast<uint8_t**>(table_vec + 0x18);
    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(table_vec + 0x20);

    uint8_t* where_ptr = reinterpret_cast<uint8_t*>(where_iter.third);
    const HashIteratorPair val = fill_val ? *fill_val : HashIteratorPair{nullptr, nullptr, nullptr};
    const uint64_t where_idx = static_cast<uint64_t>(where_ptr - begin_ptr) / 24ULL;

    if (new_bucket_count != 0) {
        const uint64_t avail = static_cast<uint64_t>(cap_ptr - end_ptr) / 24ULL;
        const uint64_t cur_count = static_cast<uint64_t>(end_ptr - begin_ptr) / 24ULL;
        if (new_bucket_count > avail) {
            constexpr uint64_t kMaxVecItems = 0xaaaaaaaaaaaaaaaULL;
            if (kMaxVecItems - cur_count < new_bucket_count) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940adaULL)));
            }
            const uint64_t req = cur_count + new_bucket_count;
            const uint64_t cur_cap = static_cast<uint64_t>(cap_ptr - begin_ptr) / 24ULL;
            uint64_t grown = (kMaxVecItems - (cur_cap >> 1) < cur_cap) ? 0ULL : (cur_cap + (cur_cap >> 1));
            if (grown < req) {
                grown = req;
            }

            uint8_t* new_buf = static_cast<uint8_t*>(alloc->vptr->AllocateAligned(alloc, grown * 24ULL, 8));
            if (!new_buf) {
                sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
            }

            uint8_t* write_ptr = new_buf + where_idx * 24ULL;
            for (uint64_t i = 0; i < new_bucket_count; ++i, write_ptr += 24) {
                if (write_ptr) {
                    *reinterpret_cast<HashIteratorPair*>(write_ptr) = val;
                }
            }
            if (begin_ptr != where_ptr) {
                std::memcpy(new_buf, begin_ptr, static_cast<size_t>(where_ptr - begin_ptr));
            }
            if (where_ptr != end_ptr) {
                std::memcpy(
                    new_buf + (where_idx + new_bucket_count) * 24ULL,
                    where_ptr,
                    static_cast<size_t>(end_ptr - where_ptr));
            }
            if (begin_ptr) {
                alloc->vptr->Free(alloc, begin_ptr);
            }
            *reinterpret_cast<uint8_t**>(table_vec + 0x08) = new_buf;
            *reinterpret_cast<uint8_t**>(table_vec + 0x10) = new_buf + req * 24ULL;
            *reinterpret_cast<uint8_t**>(table_vec + 0x18) = new_buf + grown * 24ULL;
            begin_ptr = new_buf;
        } else {
            if (where_ptr != end_ptr) {
                std::memmove(
                    where_ptr + new_bucket_count * 24ULL,
                    where_ptr,
                    static_cast<size_t>(end_ptr - where_ptr));
            }
            uint8_t* write_ptr = where_ptr;
            for (uint64_t i = 0; i < new_bucket_count; ++i, write_ptr += 24) {
                if (write_ptr) {
                    *reinterpret_cast<HashIteratorPair*>(write_ptr) = val;
                }
            }
            *reinterpret_cast<uint8_t**>(table_vec + 0x10) = end_ptr + new_bucket_count * 24ULL;
        }
    }

    if (out_pair) {
        out_pair->first = where_iter.first;
        out_pair->second = where_iter.second;
        out_pair->third = reinterpret_cast<HashNode*>(begin_ptr + where_idx * 24ULL);
    }
    return out_pair;
}

static inline InsertResult* HashTable_Insert_Impl(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashNode* new_node,
    HashIteratorPair* (*rehash_fn)(HashIteratorPair*, uint8_t*, HashIteratorPair, uint64_t, const HashIteratorPair*))
{
    const uint32_t k = *key_ptr;
    uint64_t h = ComputeParamBankHash(static_cast<uint64_t>(k) ^ 0xdeadbeefULL);
    uint64_t mask = *reinterpret_cast<uint64_t*>(table + 0x48);
    uint64_t bucket = h & mask;
    if (*reinterpret_cast<uint64_t*>(table + 0x50) <= bucket) {
        bucket += ~(mask >> 1);
    }

    uint8_t* buckets = *reinterpret_cast<uint8_t**>(table + 0x28);
    HashNode* sentinel = *reinterpret_cast<HashNode**>(table + 0x08);
    HashNode* b_start = *reinterpret_cast<HashNode**>(buckets + bucket * 48 + 0x10);
    HashNode* cur = sentinel;

    if (b_start != sentinel) {
        HashNode* b_end = (*reinterpret_cast<HashNode**>(buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
        cur = b_end;
        while (cur != b_start) {
            cur = cur->prev;
            if (k < cur->key) {
                continue;
            }
            if (cur->key >= k) {
                if (new_node != sentinel) {
                    new_node->next->prev = new_node->prev;
                    new_node->prev->next = new_node->next;
                    --(*reinterpret_cast<uint64_t*>(table + 0x10));
                    DLAllocator* node_alloc = *reinterpret_cast<DLAllocator**>(table + 0x18);
                    node_alloc->vptr->Free(node_alloc, new_node);
                }
                out_res->proxy0 = 0;
                out_res->proxy1 = 0;
                out_res->node = cur;
                out_res->inserted = 0;
                return out_res;
            }
            cur = cur->next;
            break;
        }
    }

    HashNode* const insert_pos = cur;
    HashNode* const next_after = new_node->next;
    if (insert_pos != next_after) {
        new_node->prev->next = next_after;
        next_after->prev = new_node->prev;
        new_node->next = insert_pos;
        new_node->prev = insert_pos->prev;
        insert_pos->prev->next = new_node;
        insert_pos->prev = new_node;
    }

    uint8_t* b_slot = *reinterpret_cast<uint8_t**>(table + 0x28) + bucket * 48;
    HashNode** slot_first = reinterpret_cast<HashNode**>(b_slot + 0x10);
    HashNode** slot_last  = reinterpret_cast<HashNode**>(b_slot + 0x28);
    if (*slot_first == *reinterpret_cast<HashNode**>(table + 0x08)) {
        *slot_first = new_node;
        *slot_last  = new_node;
    } else if (*slot_first == insert_pos) {
        *slot_first = new_node;
    } else {
        HashNode* last_next = (*slot_last)->next;
        *slot_last = (last_next != new_node) ? last_next->prev : last_next;
    }

    auto to_float_u64 = [](uint64_t v) -> float {
        if (static_cast<int64_t>(v) < 0) {
            return static_cast<float>((v >> 1) | (v & 1ULL)) * 2.0f;
        }
        return static_cast<float>(static_cast<int64_t>(v));
    };

    uint64_t count = *reinterpret_cast<uint64_t*>(table + 0x10);
    uint64_t max_b = *reinterpret_cast<uint64_t*>(table + 0x50);
    float load_factor = to_float_u64(count) / to_float_u64(max_b);
    if (load_factor > *reinterpret_cast<float*>(table + 0x58)) {
        uint64_t new_max_b = max_b;
        if (new_max_b <= 0x1ffULL) {
            new_max_b <<= 3;
        } else if (new_max_b < 0x555555555555555ULL) {
            new_max_b <<= 1;
        }

        uint8_t* vec_begin = *reinterpret_cast<uint8_t**>(table + 0x28);
        *reinterpret_cast<uint8_t**>(table + 0x30) = vec_begin;
        HashNode* head_sentinel = *reinterpret_cast<HashNode**>(table + 0x08);
        const HashIteratorPair fill_iter{nullptr, nullptr, head_sentinel};
        HashIteratorPair dummy{};
        rehash_fn(
            &dummy,
            table + 0x20,
            HashIteratorPair{nullptr, nullptr, reinterpret_cast<HashNode*>(vec_begin)},
            new_max_b * 2ULL,
            &fill_iter);
        *reinterpret_cast<uint64_t*>(table + 0x48) = new_max_b - 1ULL;
        *reinterpret_cast<uint64_t*>(table + 0x50) = new_max_b;

        HashNode* head = *reinterpret_cast<HashNode**>(table + 0x08);
        if (head->next != head) {
            HashNode* last_orig = head->prev;
            while (true) {
                HashNode* move_node = (*reinterpret_cast<HashNode**>(table + 0x08))->next;
                InsertResult reinsert_out{};
                HashTable_Insert_Impl(&reinsert_out, table, &move_node->key, move_node, rehash_fn);
                if (move_node == last_orig) {
                    break;
                }
            }
        }
    }

    out_res->proxy0 = 0;
    out_res->proxy1 = 0;
    out_res->node = new_node;
    out_res->inserted = 1;
    return out_res;
}

static inline HashIteratorPair* HashTable_EraseNode_Impl(
    HashIteratorPair* out_next,
    uint8_t* table,
    HashNode* node)
{
    uint64_t h = ComputeParamBankHash(static_cast<uint64_t>(node->key) ^ 0xdeadbeefULL);
    uint64_t mask = *reinterpret_cast<uint64_t*>(table + 0x48);
    uint64_t bucket = h & mask;
    if (*reinterpret_cast<uint64_t*>(table + 0x50) <= bucket) {
        bucket += ~(mask >> 1);
    }

    uint8_t* b_slot = *reinterpret_cast<uint8_t**>(table + 0x28) + bucket * 48;
    HashNode* sentinel = *reinterpret_cast<HashNode**>(table + 0x08);
    HashNode* b_start = *reinterpret_cast<HashNode**>(b_slot + 0x10);
    HashNode* b_end = *reinterpret_cast<HashNode**>(b_slot + 0x28);

    if (node == b_start) {
        *reinterpret_cast<HashNode**>(b_slot + 0x10) = (node == b_end) ? sentinel : node->next;
    }
    if (node == b_end) {
        *reinterpret_cast<HashNode**>(b_slot + 0x28) = (node == b_start) ? sentinel : node->prev;
    }

    HashNode* next_node = node->next;
    node->prev->next = next_node;
    next_node->prev = node->prev;
    --(*reinterpret_cast<uint64_t*>(table + 0x10));

    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(table + 0x18);
    alloc->vptr->Free(alloc, node);

    out_next->first = nullptr;
    out_next->second = nullptr;
    out_next->third = next_node;
    return out_next;
}

} // namespace

// ============================================================================
// 0x21f1570 (834 B): GXParamBankManagerE_Construct (FeedBackBlur[Yebis])
// ============================================================================
extern "C" void sub_21f1570(uint8_t* self, DLAllocator* alloc) {
    *reinterpret_cast<DLAllocator**>(self + 0x00) = alloc;
    uint8_t heap_info[184]{};
    alloc->vptr->GetHeapInfo(heap_info, alloc, 0);
    if ((heap_info[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
    *reinterpret_cast<DLAllocator**>(self + 0x20) = alloc;

    HashNode* sentinel = static_cast<HashNode*>(alloc->vptr->AllocateAligned(alloc, 0x20, 0x8));
    if (!sentinel) {
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    } else {
        sentinel->next = sentinel;
        sentinel->prev = sentinel;
    }
    *reinterpret_cast<HashNode**>(self + 0x10) = sentinel;

    *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    *reinterpret_cast<DLAllocator**>(self + 0x48) = alloc;
    *reinterpret_cast<uint32_t*>(self + 0x60) = 0x3f800000U;

    HashIteratorPair dummy_pair{};
    const HashIteratorPair fill_iter{nullptr, nullptr, sentinel};
    sub_21f4380(&dummy_pair, self + 0x28, HashIteratorPair{nullptr, nullptr, nullptr}, 0x10, &fill_iter);
    *reinterpret_cast<uint64_t*>(self + 0x50) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x58) = 8;

    // Initialize primary default block (+0x68) and backup block (+0x88)
    *reinterpret_cast<uint32_t*>(self + 0x68) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x6c) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x70) = 0x3f000000U;
    *reinterpret_cast<uint32_t*>(self + 0x74) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x78) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x7c) = 0x3f4ccccdU;
    *reinterpret_cast<uint32_t*>(self + 0x80) = 0x3f666666U;
    *reinterpret_cast<uint32_t*>(self + 0x84) = 0x40900000U;

    std::memcpy(self + 0x88, self + 0x68, 0x20);
    *reinterpret_cast<uint32_t*>(self + 0xa8) = 0;

    sub_2bbe578(self + 0xd8, heap_info, 0x81);
    self[0x159] = 0;
    *reinterpret_cast<uint64_t*>(self + 0xd0) = 7;
    *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
    *reinterpret_cast<uint16_t*>(self + 0xb8) = 0;
    self[0x15a] = 1;
    sub_2935350(self + 0xb0, 0x3f, 0);

    uint16_t* str_buf = reinterpret_cast<uint16_t*>(self + 0xb8);
    if (*reinterpret_cast<uint64_t*>(self + 0xd0) >= 8) {
        str_buf = *reinterpret_cast<uint16_t**>(self + 0xb8);
    }
    *reinterpret_cast<uint64_t*>(self + 0xc8) = 0;
    *str_buf = 0;

    sub_e789d0(self + 0x160);
    sub_e78a80(self + 0x160);

    sub_29354e0(self + 0xb0, reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// Group F (DepthOfField[Yebis] / Dof[Yebis]): 0x21ec470 .. 0x21f1020 (22 functions)
// ============================================================================

// 0x21f1020 (1352 B): GXParamBankManagerF_InnerHashTable_RehashInsert
extern "C" HashIteratorPair* sub_21f1020(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    return RehashInsert_Impl(out_pair, table_vec, where_iter, new_bucket_count, fill_val);
}

// 0x21f0ad0 (1352 B): GXParamBankManagerF_OuterHashTable_RehashInsert
extern "C" HashIteratorPair* sub_21f0ad0(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    return RehashInsert_Impl(out_pair, table_vec, where_iter, new_bucket_count, fill_val);
}

// 0x21f06c0 (1040 B): GXParamBankManagerF_InnerHashTable_Insert
extern "C" InsertResult* sub_21f06c0(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashIteratorPair node_iter)
{
    return HashTable_Insert_Impl(out_res, table, key_ptr, node_iter.third, &sub_21f1020);
}

// 0x21f02b0 (1040 B): GXParamBankManagerF_OuterHashTable_Insert
extern "C" InsertResult* sub_21f02b0(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashIteratorPair node_iter)
{
    return HashTable_Insert_Impl(out_res, table, key_ptr, node_iter.third, &sub_21f0ad0);
}

// 0x21f0140 (363 B): GXParamBankManagerF_OuterHashTable_EraseNode
extern "C" HashIteratorPair* sub_21f0140(HashIteratorPair* out_next, uint8_t* table, HashNode* node) {
    return HashTable_EraseNode_Impl(out_next, table, node);
}

// 0x21effd0 (363 B): GXParamBankManagerF_InnerHashTable_EraseNode
extern "C" HashIteratorPair* sub_21effd0(HashIteratorPair* out_next, uint8_t* table, HashNode* node) {
    return HashTable_EraseNode_Impl(out_next, table, node);
}

// 0x21effa0 (40 B): GXParamBankManagerF_DebugCallback_Invoke
extern "C" void sub_21effa0(uint8_t* self) {
    if ((self[0x0b] & 0x40) != 0) {
        uint8_t* target = *reinterpret_cast<uint8_t**>(self + 0x10) + *reinterpret_cast<int64_t*>(self + 0x28);
        uintptr_t fn_ptr = *reinterpret_cast<uintptr_t*>(self + 0x20);
        if ((fn_ptr & 1) != 0) {
            uintptr_t vtable = *reinterpret_cast<uintptr_t*>(target);
            fn_ptr = *reinterpret_cast<uintptr_t*>(vtable + fn_ptr - 1);
        }
        uint32_t arg = *reinterpret_cast<uint32_t*>(self + 0x18);
        reinterpret_cast<void (*)(void*, uint32_t)>(fn_ptr)(target, arg);
    }
}

// 0x21eff90 (5 B): GXParamBankManagerF_DebugCallback_Delete
extern "C" void sub_21eff90(void* self) {
    sub_2bbe588(self);
}

// 0x21eff70 (22 B): GXParamBankManagerF_DebugCallback_Destroy
extern "C" void sub_21eff70(uint8_t* self) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xbfffffffU;
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;
}

// 0x21ef830 (1708 B): GXParamBankManagerF_BuildParamEditIdListGui
extern "C" void sub_21ef830(uint8_t* self) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x38);
    uint32_t bank_id = *reinterpret_cast<uint32_t*>(param_sys + slot * 4 + 0x588);

    sub_2936030(self + 0x180, reinterpret_cast<const char16_t*>(GX_VMA(0x49cbebeULL)), bank_id);

    DLAllocator* gui_alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    uint8_t* gui_obj = static_cast<uint8_t*>(gui_alloc->vptr->AllocateAligned(gui_alloc, 0x178, 8));
    if (gui_obj) {
        const char16_t* title_ptr = reinterpret_cast<const char16_t*>(self + 0x188);
        if (*reinterpret_cast<uint64_t*>(self + 0x1a0) >= 8) {
            title_ptr = *reinterpret_cast<const char16_t**>(self + 0x188);
        }
        uint64_t desc[2] = {0, reinterpret_cast<uint64_t>(title_ptr)};
        sub_2362cf0(gui_obj, desc);
    }

    uint8_t* main_win_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6bc8ULL));
    uint8_t* main_win = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(main_win_root + 0x10) + 0x18);
    uint16_t rect[4] = {0, 0, 0x12c, 0x190};
    uint64_t rect_flags = 0x51502ULL;
    struct { uint16_t r[4]; uint64_t f; } rect_pack = {{rect[0], rect[1], rect[2], rect[3]}, rect_flags};

    if (gui_obj && (gui_obj[0x0b] & 0x40) != 0 && gui_obj[0x6f] == 0 &&
        main_win != gui_obj && main_win != nullptr && (main_win[0x0b] & 0x40) != 0)
    {
        uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(main_win);
        auto check_fn = *reinterpret_cast<int (*)(void*, void*)>(vtbl + 0x28);
        if (check_fn(main_win, gui_obj) != 0) {
            sub_2328e40(gui_obj, main_win, *reinterpret_cast<void**>(main_win + 0x20), &rect_pack);
        }
    }

    uint64_t list_builder[6]{};
    sub_236c4d0(list_builder, gui_obj);

    auto release_ref_str = [](uint8_t* s) {
        if (s && --(*reinterpret_cast<uint32_t*>(s + 0x38)) == 0) {
            DLAllocator* str_alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e10ULL));
            if (*reinterpret_cast<uint64_t*>(s + 0x20) >= 8) {
                DLAllocator* buf_alloc = *reinterpret_cast<DLAllocator**>(s + 0x28);
                buf_alloc->vptr->Free(buf_alloc, *reinterpret_cast<void**>(s + 0x08));
            }
            str_alloc->vptr->Free(str_alloc, s);
        }
    };

    uint64_t dummy_str[2]{};
    sub_2ba7d40(dummy_str, reinterpret_cast<const char16_t*>(GX_VMA(0x49d27c0ULL)));
    uint64_t item_desc[2]{};
    uint8_t* ref_str = nullptr;
    if (dummy_str[1] != 0) {
        item_desc[1] = dummy_str[1];
    } else if (dummy_str[0] != 0) {
        item_desc[0] = dummy_str[0];
        ref_str = reinterpret_cast<uint8_t*>(dummy_str[0]);
        ++(*reinterpret_cast<uint32_t*>(ref_str + 0x38));
    }

    void* dummy_node = sub_236c900(list_builder, item_desc, 0);
    release_ref_str(ref_str);
    release_ref_str(reinterpret_cast<uint8_t*>(dummy_str[0]));
    if (dummy_node) {
        sub_e779a0(self + 0x230, list_builder);
    }

    uint64_t h = ComputeParamBankHash(static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL);
    uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x50);
    uint64_t bucket = h & mask;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= bucket) {
        bucket += ~(mask >> 1);
    }
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
    HashNode* b_cur = *reinterpret_cast<HashNode**>(outer_buckets + bucket * 48 + 0x10);
    if (b_cur != outer_sentinel) {
        HashNode* b_end = (*reinterpret_cast<HashNode**>(outer_buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
        while (b_cur != b_end && b_cur->key < bank_id) {
            b_cur = b_cur->next;
        }
        if (b_cur != b_end && bank_id >= b_cur->key) {
            uint8_t* inner_table = static_cast<uint8_t*>(b_cur->value);
            HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
                if (in_cur->key == 0xffffffffU) continue;
                uint64_t entry_str[2]{};
                sub_2ba7d40(entry_str, reinterpret_cast<const char16_t*>(GX_VMA(0x49cbef4ULL)), in_cur->key);
                uint64_t entry_desc[2] = {entry_str[0], entry_str[1]};
                void* entry_node = sub_236c900(list_builder, entry_desc, 0);
                release_ref_str(reinterpret_cast<uint8_t*>(entry_str[0]));
                if (entry_node) {
                    sub_e779a0(in_cur->value, list_builder);
                }
            }
        }
    }
    sub_236c720(list_builder);
}

// 0x21ef5c0 (612 B): GXParamBankManagerF_DebugCommandCallback
extern "C" void sub_21ef5c0(uint8_t* self, void* parent_gui);

// 0x21ef2e0 (732 B): GXParamBankManagerF_LookupEntryOrDefault
extern "C" void* sub_21ef2e0(uint8_t* self, const uint64_t* packed_key) {
    uint64_t raw = *packed_key;
    uint32_t bank_id = static_cast<uint32_t>(raw);
    uint32_t inner_id = static_cast<uint32_t>(raw >> 32);
    if (bank_id == 0xffffffffU) {
        if (inner_id == 0xffffffffU) {
            inner_id = 0;
            bank_id = 1;
        }
    }

    uint64_t h1 = ComputeParamBankHash(static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL);
    uint64_t mask1 = *reinterpret_cast<uint64_t*>(self + 0x50);
    uint64_t b1 = h1 & mask1;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= b1) b1 += ~(mask1 >> 1);

    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
    HashNode* cur1 = *reinterpret_cast<HashNode**>(outer_buckets + b1 * 48 + 0x10);
    if (cur1 != outer_sentinel) {
        HashNode* end1 = (*reinterpret_cast<HashNode**>(outer_buckets + (b1 * 2 + 1) * 24 + 0x10))->next;
        while (cur1 != end1 && cur1->key < bank_id) {
            cur1 = cur1->next;
        }
        if (cur1 != end1 && bank_id >= cur1->key) {
            uint8_t* inner_table = static_cast<uint8_t*>(cur1->value);
            uint64_t h2 = ComputeParamBankHash(static_cast<uint64_t>(inner_id) ^ 0xdeadbeefULL);
            uint64_t mask2 = *reinterpret_cast<uint64_t*>(inner_table + 0x48);
            uint64_t b2 = h2 & mask2;
            if (*reinterpret_cast<uint64_t*>(inner_table + 0x50) <= b2) b2 += ~(mask2 >> 1);

            HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            uint8_t* inner_buckets = *reinterpret_cast<uint8_t**>(inner_table + 0x28);
            HashNode* cur2 = *reinterpret_cast<HashNode**>(inner_buckets + b2 * 48 + 0x10);
            HashNode* found2 = inner_sentinel;
            if (cur2 != inner_sentinel) {
                HashNode* end2 = (*reinterpret_cast<HashNode**>(inner_buckets + (b2 * 2 + 1) * 24 + 0x10))->next;
                while (cur2 != end2 && cur2->key < inner_id) {
                    cur2 = cur2->next;
                }
                if (cur2 != end2 && inner_id >= cur2->key) {
                    found2 = cur2;
                }
            }
            if (found2 != *reinterpret_cast<HashNode**>(static_cast<uint8_t*>(cur1->value) + 0x08)) {
                return found2->value;
            }
        }
    }

    // Fallback to bank_id = 1, inner_id = 0
    uint64_t h_fb1 = ComputeParamBankHash(0xdeadbeeeULL);
    uint64_t b_fb1 = h_fb1 & mask1;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= b_fb1) b_fb1 += ~(mask1 >> 1);
    HashNode* cur_fb1 = *reinterpret_cast<HashNode**>(outer_buckets + b_fb1 * 48 + 0x10);
    if (cur_fb1 != outer_sentinel) {
        HashNode* end_fb1 = (*reinterpret_cast<HashNode**>(outer_buckets + (b_fb1 * 2 + 1) * 24 + 0x10))->next;
        while (cur_fb1 != end_fb1 && cur_fb1->key == 0) {
            cur_fb1 = cur_fb1->next;
        }
        if (cur_fb1 != end_fb1 && cur_fb1->key <= 1) {
            uint8_t* inner_table = static_cast<uint8_t*>(cur_fb1->value);
            uint64_t h_fb2 = ComputeParamBankHash(0xdeadbeefULL);
            uint64_t mask2 = *reinterpret_cast<uint64_t*>(inner_table + 0x48);
            uint64_t b_fb2 = h_fb2 & mask2;
            if (*reinterpret_cast<uint64_t*>(inner_table + 0x50) <= b_fb2) b_fb2 += ~(mask2 >> 1);
            HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            uint8_t* inner_buckets = *reinterpret_cast<uint8_t**>(inner_table + 0x28);
            HashNode* cur_fb2 = *reinterpret_cast<HashNode**>(inner_buckets + b_fb2 * 48 + 0x10);
            if (cur_fb2 != inner_sentinel) {
                HashNode* end_fb2 = *reinterpret_cast<HashNode**>(*reinterpret_cast<uint8_t**>(inner_buckets + (b_fb2 * 2 + 1) * 24 + 0x10));
                if (cur_fb2 != end_fb2 && cur_fb2->key == 0) {
                    if (cur_fb2 != *reinterpret_cast<HashNode**>(static_cast<uint8_t*>(cur_fb1->value) + 0x08)) {
                        return cur_fb2->value;
                    }
                }
            }
        }
    }
    return self + 0x230;
}

// 0x21ef270 (103 B): GXParamBankManagerF_ForEachEntry_Apply
extern "C" void sub_21ef270(uint8_t* self) {
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    for (HashNode* out_cur = outer_sentinel->next; out_cur != outer_sentinel; out_cur = out_cur->next) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
            sub_e761f0(in_cur->value);
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21ef1e0 (138 B): GXParamBankManagerF_ForEachEntry_Init
extern "C" void sub_21ef1e0(uint8_t* self) {
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    for (HashNode* out_cur = outer_sentinel->next; out_cur != outer_sentinel; out_cur = out_cur->next) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
            uint8_t* entry = static_cast<uint8_t*>(in_cur->value);
            uint32_t inner_id = in_cur->key;
            uint32_t bank_id = out_cur->key;
            *reinterpret_cast<uint32_t*>(entry + 0xd8) = inner_id;
            *reinterpret_cast<uint32_t*>(entry + 0xdc) = bank_id;
            sub_e76250(entry, inner_id, 0.0f);
            sub_e763c0(in_cur->value, *reinterpret_cast<void**>(self + 0x00));
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21ef1d0 (1 B): GXParamBankManagerF_ApplyParamsToScene (No-op in Dof[Yebis])
extern "C" void sub_21ef1d0() {
    return;
}

// 0x21ef1a0 (40 B): GXParamBankManagerF_ForwardLookup
extern "C" void* sub_21ef1a0(void* self_unused, uint32_t bank_id, uint32_t inner_id) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x38);
    return sub_2186da0(param_sys, slot, bank_id, inner_id, self_unused ? -1LL : -1LL);
}

// 0x21ef5c0 definition (after sub_21ef270 and sub_21ef1e0)
extern "C" void sub_21ef5c0(uint8_t* self, void* parent_gui) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x38);
    sub_21877f0(param_sys, parent_gui, slot, 0);

    DLAllocator* gui_alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));

    auto register_btn = [&](const char16_t* label, void (*fn_ptr)(uint8_t*)) {
        uint64_t empty_desc[2]{};
        void* btn = sub_236d620(parent_gui, empty_desc);
        uint64_t label_desc[2] = {0, reinterpret_cast<uint64_t>(label)};
        uint8_t* cb = static_cast<uint8_t*>(gui_alloc->vptr->AllocateAligned(gui_alloc, 0x30, 8));
        if (cb) {
            *reinterpret_cast<uint32_t*>(cb + 0x08) = (*reinterpret_cast<uint32_t*>(cb + 0x08) & 0x80000000U) | 0x40000000U;
            *reinterpret_cast<uint64_t*>(cb + 0x00) = GX_VMA(0x53b7120ULL);
            *reinterpret_cast<uint8_t**>(cb + 0x10) = self;
            *reinterpret_cast<uint32_t*>(cb + 0x18) = 0;
            *reinterpret_cast<void (**)(uint8_t*)>(cb + 0x20) = fn_ptr;
            *reinterpret_cast<uint64_t*>(cb + 0x28) = 0;
        }
        sub_235e4c0(btn, label_desc, cb);
    };

    register_btn(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL)), &sub_21ef1e0);
    register_btn(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL)), &sub_21ef270);
    register_btn(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL)), &sub_21ef830);
}

// Forward declarations within Batch 4
extern "C" void sub_21ecbc0(uint8_t* self);
extern "C" void sub_21e72a0(
    uint8_t* gui_mgr,
    uint32_t slot,
    const char16_t* name,
    const char16_t* desc,
    void* val_ptr,
    uint32_t item_count,
    const void* items_table,
    uint32_t flag);

// 0x21eeee0 (698 B): GXParamBankManagerF_PurgeInvalidEntriesInBank
extern "C" void sub_21eeee0(
    uint8_t* self,
    void* unused_rsi,
    uint32_t bank_id,
    uint32_t override_slot_idx,
    float blend_time)
{
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x38);
    sub_2185b50(param_sys, slot, bank_id, override_slot_idx, blend_time);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = (unused_rsi != nullptr) ? 1 : 1;

    uint64_t h = ComputeParamBankHash(static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL);
    uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x50);
    uint64_t bucket = h & mask;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= bucket) {
        bucket += ~(mask >> 1);
    }

    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
    HashNode* out_cur = *reinterpret_cast<HashNode**>(outer_buckets + bucket * 48 + 0x10);
    if (out_cur == outer_sentinel) return;

    HashNode* out_end = (*reinterpret_cast<HashNode**>(outer_buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
    while (out_cur != out_end && out_cur->key < bank_id) {
        out_cur = out_cur->next;
    }
    if (out_cur == out_end || out_cur->key > bank_id) return;

    uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
    if (inner_table) {
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        HashNode* in_cur = inner_sentinel->next;
        while (in_cur != inner_sentinel) {
            bool valid = sub_2185e00(param_sys, slot, in_cur->key, bank_id);
            uint8_t* entry = static_cast<uint8_t*>(in_cur->value);
            if (valid) {
                entry[0xd0] = 1;
                in_cur = in_cur->next;
            } else {
                if (entry) {
                    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
                    uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(entry);
                    reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl))(entry);
                    alloc->vptr->Free(alloc, entry);
                    in_cur->value = nullptr;
                }
                HashIteratorPair next_pair{};
                sub_21effd0(&next_pair, static_cast<uint8_t*>(out_cur->value), in_cur);
                in_cur = next_pair.third;
            }
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        if (*reinterpret_cast<uint64_t*>(inner_table + 0x10) != 0) {
            return;
        }
        DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
        void* inner_buckets = *reinterpret_cast<void**>(inner_table + 0x28);
        if (inner_buckets) {
            DLAllocator* b_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x40);
            b_alloc->vptr->Free(b_alloc, inner_buckets);
            *reinterpret_cast<uint64_t*>(inner_table + 0x28) = 0;
            *reinterpret_cast<uint64_t*>(inner_table + 0x30) = 0;
            *reinterpret_cast<uint64_t*>(inner_table + 0x38) = 0;
        }
        HashNode* in_s = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        HashNode* n = in_s->next;
        in_s->next = in_s;
        in_s->prev = in_s;
        *reinterpret_cast<uint64_t*>(inner_table + 0x10) = 0;
        while (n != in_s) {
            HashNode* next_n = n->next;
            DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
            n_alloc->vptr->Free(n_alloc, n);
            n = next_n;
        }
        DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
        n_alloc->vptr->Free(n_alloc, in_s);
        alloc->vptr->Free(alloc, inner_table);
        out_cur->value = nullptr;
    }

    HashIteratorPair out_next{};
    sub_21f0140(&out_next, self + 0x08, out_cur);
}

// 0x21ee760 (1912 B): GXParamBankManagerF_ReloadAndRegisterBank
extern "C" void sub_21ee760(uint8_t* self, void* ctx, uint32_t bank_id, uint32_t unk_flag, float weight) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x38);
    uint32_t count = sub_2184cf0(param_sys, slot, ctx);
    if (count > 0) {
        uint64_t raw_key = static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL;
        uint64_t h = ComputeParamBankHash(raw_key);
        uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x50);
        uint64_t bucket = h & mask;
        if (*reinterpret_cast<uint64_t*>(self + 0x58) <= bucket) bucket += ~(mask >> 1);

        HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
        uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
        HashNode* out_cur = *reinterpret_cast<HashNode**>(outer_buckets + bucket * 48 + 0x10);
        if (out_cur != outer_sentinel) {
            HashNode* out_end = (*reinterpret_cast<HashNode**>(outer_buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
            while (out_cur != out_end && out_cur->key < bank_id) {
                out_cur = out_cur->next;
            }
            if (out_cur == out_end || out_cur->key > bank_id) {
                out_cur = outer_sentinel;
            }
        }

        if (out_cur == outer_sentinel) {
            DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
            uint8_t* new_inner = static_cast<uint8_t*>(alloc->vptr->AllocateAligned(alloc, 0x60, 8));
            if (new_inner) {
                uint8_t heap_info[64]{};
                alloc->vptr->GetHeapInfo(heap_info, alloc, 0);
                if ((heap_info[0] & 0x20) == 0) {
                    sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
                }
                *reinterpret_cast<uint64_t*>(new_inner + 0x10) = 0;
                *reinterpret_cast<uint64_t*>(new_inner + 0x08) = 0;
                *reinterpret_cast<DLAllocator**>(new_inner + 0x18) = alloc;
                HashNode* in_s = static_cast<HashNode*>(alloc->vptr->AllocateAligned(alloc, 0x20, 8));
                if (!in_s) {
                    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                } else {
                    in_s->next = in_s;
                    in_s->prev = in_s;
                }
                *reinterpret_cast<HashNode**>(new_inner + 0x08) = in_s;
                *reinterpret_cast<uint64_t*>(new_inner + 0x28) = 0;
                *reinterpret_cast<uint64_t*>(new_inner + 0x30) = 0;
                *reinterpret_cast<uint64_t*>(new_inner + 0x38) = 0;
                *reinterpret_cast<DLAllocator**>(new_inner + 0x40) = alloc;
                *reinterpret_cast<uint32_t*>(new_inner + 0x58) = 0x3f800000U;
                HashIteratorPair dummy_p{};
                const HashIteratorPair fill_in{nullptr, nullptr, in_s};
                sub_21f1020(&dummy_p, new_inner + 0x20, HashIteratorPair{nullptr, nullptr, nullptr}, 0x10, &fill_in);
                *reinterpret_cast<uint64_t*>(new_inner + 0x48) = 7;
                *reinterpret_cast<uint64_t*>(new_inner + 0x50) = 8;
            }

            HashNode* head = outer_sentinel->next;
            DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(self + 0x20);
            HashNode* new_out_node = static_cast<HashNode*>(n_alloc->vptr->AllocateAligned(n_alloc, 0x20, 8));
            if (!new_out_node) {
                sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                return;
            }
            new_out_node->next = head ? head : new_out_node;
            new_out_node->prev = head ? head->prev : new_out_node;
            new_out_node->key = bank_id;
            new_out_node->value = new_inner;

            uint64_t cur_cnt = *reinterpret_cast<uint64_t*>(self + 0x18);
            if (cur_cnt == 0x7fffffffffffffeULL) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            *reinterpret_cast<uint64_t*>(self + 0x18) = cur_cnt + 1;
            head->prev = new_out_node;
            new_out_node->prev->next = new_out_node;

            InsertResult ins_res{};
            HashNode* first_out = (*reinterpret_cast<HashNode**>(self + 0x10))->next;
            sub_21f02b0(&ins_res, self + 0x08, &first_out->key, HashIteratorPair{nullptr, nullptr, first_out});
            out_cur = ins_res.node ? ins_res.node : first_out;
        }

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t inner_id = sub_2184de0(param_sys, slot, ctx, i);
            uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
            uint64_t ih = ComputeParamBankHash(static_cast<uint64_t>(inner_id) ^ 0xdeadbeefULL);
            uint64_t im = *reinterpret_cast<uint64_t*>(inner_table + 0x48);
            uint64_t ib = ih & im;
            if (*reinterpret_cast<uint64_t*>(inner_table + 0x50) <= ib) ib += ~(im >> 1);

            HashNode* in_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            uint8_t* in_buckets = *reinterpret_cast<uint8_t**>(inner_table + 0x28);
            HashNode* in_cur = *reinterpret_cast<HashNode**>(in_buckets + ib * 48 + 0x10);
            HashNode* in_found = in_sentinel;
            if (in_cur != in_sentinel) {
                HashNode* in_end = (*reinterpret_cast<HashNode**>(in_buckets + (ib * 2 + 1) * 24 + 0x10))->next;
                while (in_cur != in_end && in_cur->key < inner_id) {
                    in_cur = in_cur->next;
                }
                if (in_cur != in_end && inner_id >= in_cur->key) {
                    in_found = in_cur;
                }
            }

            if (in_found != in_sentinel) {
                static_cast<uint8_t*>(in_found->value)[0xd0] = 1;
            } else {
                DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
                uint8_t* entry = static_cast<uint8_t*>(alloc->vptr->AllocateAligned(alloc, 0xe0, 8));
                if (!entry) {
                    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    continue;
                }
                sub_e75d90(entry);
                sub_e75f70(entry);
                *reinterpret_cast<uint32_t*>(entry + 0xd8) = inner_id;
                *reinterpret_cast<uint32_t*>(entry + 0xdc) = bank_id;
                sub_e76250(entry, inner_id, 0.0f);
                sub_e763c0(entry, *reinterpret_cast<void**>(self + 0x00));

                HashNode* in_head = in_sentinel->next;
                DLAllocator* in_n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
                HashNode* new_in_node = static_cast<HashNode*>(in_n_alloc->vptr->AllocateAligned(in_n_alloc, 0x20, 8));
                if (!new_in_node) {
                    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    continue;
                }
                new_in_node->next = in_head ? in_head : new_in_node;
                new_in_node->prev = in_head ? in_head->prev : new_in_node;
                new_in_node->key = inner_id;
                new_in_node->value = entry;

                uint64_t in_cnt = *reinterpret_cast<uint64_t*>(inner_table + 0x10);
                if (in_cnt == 0x7fffffffffffffeULL) {
                    sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
                }
                *reinterpret_cast<uint64_t*>(inner_table + 0x10) = in_cnt + 1;
                in_head->prev = new_in_node;
                new_in_node->prev->next = new_in_node;

                InsertResult in_res{};
                HashNode* first_in = (*reinterpret_cast<HashNode**>(inner_table + 0x08))->next;
                sub_21f06c0(&in_res, inner_table, &first_in->key, HashIteratorPair{nullptr, nullptr, first_in});
            }
        }
    }
    sub_2184ff0(param_sys, slot, bank_id, unk_flag, ctx, weight);
}

// 0x21ecd50 (6659 B): GXParamBankManagerF_RegisterDebugGuiProperties (DepthOfField[Yebis])
extern "C" bool sub_21ecd50(uint8_t* self) {
    sub_21ecbc0(self);

    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = 0;
    uint8_t* gui_mgr = *reinterpret_cast<uint8_t**>(param_sys + 0x28);

    sub_e32510(gui_mgr, &slot, reinterpret_cast<const char16_t*>(GX_VMA(0x49c5770ULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c5778ULL)));
    *reinterpret_cast<uint32_t*>(param_sys + 0x38) = slot;
    sub_e333e0(gui_mgr, slot, reinterpret_cast<const char16_t*>(GX_VMA(0x49c578eULL)), reinterpret_cast<const char16_t*>(GX_VMA(0x49c57a8ULL)), 1, 1);

    struct PropSpec {
        uint64_t vtbl_main;
        void* val_ptr;
        uint8_t type_code;
        uint8_t pad1[3];
        uint32_t step_mode;
        uint8_t flag_b;
        uint8_t pad2[7];
        uint64_t vtbl_sub;
        uint32_t min_raw;
        uint32_t max_raw;
        uint32_t step_raw;
        uint32_t mult_raw;
    };

    auto reg_prop = [&](size_t off, uint8_t tcode, uint32_t smode, uint64_t vm, uint64_t vs,
                        uint32_t mn, uint32_t mx, uint32_t st, uint32_t ml,
                        uintptr_t name_vma, uintptr_t desc_vma, uint32_t flags)
    {
        PropSpec spec{};
        spec.vtbl_main = vm;
        spec.val_ptr = self + off;
        spec.type_code = tcode;
        spec.step_mode = smode;
        spec.flag_b = 1;
        spec.vtbl_sub = vs;
        spec.min_raw = mn;
        spec.max_raw = mx;
        spec.step_raw = st;
        spec.mult_raw = ml;
        sub_e327d0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
                   reinterpret_cast<const char16_t*>(name_vma),
                   reinterpret_cast<const char16_t*>(desc_vma),
                   &spec, 1, flags);
    };

    const uint64_t kIntVtblM = GX_VMA(0x52e8770ULL);
    const uint64_t kIntVtblS = GX_VMA(0x52e4a80ULL);
    const uint64_t kFltVtblM = GX_VMA(0x52e8400ULL);
    const uint64_t kFltVtblS = GX_VMA(0x52e48a0ULL);
    const uint64_t kBolVtblM = GX_VMA(0x52e8460ULL);
    const uint64_t kBolVtblS = GX_VMA(0x52e4940ULL);

    // 1. Enable (+0x68), ApertuerF (+0x6c), DofFocusDistance (+0x70)
    reg_prop(0x68, 0x3, 1, kIntVtblM, kIntVtblS, 0, 1, 0, 0x3f800000U, GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c57d0ULL), 0);
    reg_prop(0x6c, 0x9, 0, kFltVtblM, kFltVtblS, 0x3c23d70aU, 0x43800000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c5804ULL), GX_VMA(0x49c5818ULL), 1);
    reg_prop(0x70, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x447a0000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c5838ULL), GX_VMA(0x49c585aULL), 1);

    // Subgroup: Detail Info
    sub_e333e0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49cb990ULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c5888ULL)), 1, 1);

    // Inline Enum registration for CCD-Size (+0x74, 17 items at 0x535fff0)
    sub_21e72a0(*reinterpret_cast<uint8_t**>(param_sys + 0x28), slot,
                reinterpret_cast<const char16_t*>(GX_VMA(0x49c5a6cULL)),
                reinterpret_cast<const char16_t*>(GX_VMA(0x49c5a7eULL)),
                self + 0x74, 17, reinterpret_cast<const void*>(GX_VMA(0x535fff0ULL)), 1);

    // Detail Info properties (+0x78 .. +0x8c)
    reg_prop(0x78, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c5a9cULL), GX_VMA(0x49c5ad0ULL), 1);
    reg_prop(0x7c, 0x9, 0, kFltVtblM, kFltVtblS, 0x3f800000U, 0x42b20000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c5b10ULL), GX_VMA(0x49c5b46ULL), 1);
    reg_prop(0x80, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x47c35000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c5b88ULL), GX_VMA(0x49c5ba6ULL), 1);
    reg_prop(0x84, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x43b40000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c5bd0ULL), GX_VMA(0x49c5c02ULL), 1);
    reg_prop(0x88, 0x3, 0, kIntVtblM, kIntVtblS, 0xffffffffU, 7, 1, 0x3f800000U, GX_VMA(0x49c5c40ULL), GX_VMA(0x49c5c6eULL), 1);
    reg_prop(0x8c, 0x3, 0, kIntVtblM, kIntVtblS, 0xffffffffU, 7, 1, 0x3f800000U, GX_VMA(0x49c5ca8ULL), GX_VMA(0x49c5cd4ULL), 1);

    // DiaphragmRotate (+0x90, 4 items at 0x5360100) and DofEdgeQuarity (+0x94, 5 items at 0x5360140)
    uint64_t diaphragm_items[8];
    std::memcpy(diaphragm_items, reinterpret_cast<const void*>(GX_VMA(0x5360100ULL)), 64);
    sub_21e72a0(*reinterpret_cast<uint8_t**>(param_sys + 0x28), slot,
                reinterpret_cast<const char16_t*>(GX_VMA(0x49c5de6ULL)),
                reinterpret_cast<const char16_t*>(GX_VMA(0x49c5e06ULL)),
                self + 0x90, 4, diaphragm_items, 1);

    uint64_t edge_quality_items[10];
    std::memcpy(edge_quality_items, reinterpret_cast<const void*>(GX_VMA(0x5360140ULL)), 80);
    sub_21e72a0(*reinterpret_cast<uint8_t**>(param_sys + 0x28), slot,
                reinterpret_cast<const char16_t*>(GX_VMA(0x49c5ed2ULL)),
                reinterpret_cast<const char16_t*>(GX_VMA(0x49c5ef0ULL)),
                self + 0x94, 5, edge_quality_items, 1);

    reg_prop(0xec, 0x9, 0, kFltVtblM, kFltVtblS, 0x3a83126fU, 0x3f800000U, 0x3a83126fU, 0x3f800000U, GX_VMA(0x49c5f1aULL), GX_VMA(0x49c5f4aULL), 1);
    sub_e33600(*reinterpret_cast<void**>(param_sys + 0x28), slot, 1);

    // Subgroup: CustomDof (+0x98 .. +0xc4)
    sub_e333e0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c5f8cULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c5fa0ULL)), 1, 1);
    reg_prop(0x98, 0x7, 1, kBolVtblM, kBolVtblS, 0, 1, 0, 0x3f800000U, GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c5fdeULL), 0);
    reg_prop(0x9c, 0x7, 1, kBolVtblM, kBolVtblS, 0, 1, 0, 0x3f800000U, GX_VMA(0x49c6026ULL), GX_VMA(0x49c604cULL), 0);
    reg_prop(0xa0, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x461c4000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c609cULL), GX_VMA(0x49c60c0ULL), 1);
    reg_prop(0xa4, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x47c35000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c60f0ULL), GX_VMA(0x49c6110ULL), 1);
    reg_prop(0xa8, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c613cULL), GX_VMA(0x49c615cULL), 1);
    reg_prop(0xac, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x3f800000U, 0x3a83126fU, 0x3f800000U, GX_VMA(0x49c6188ULL), GX_VMA(0x49c61aaULL), 1);
    reg_prop(0xb0, 0x7, 1, kBolVtblM, kBolVtblS, 0, 1, 0, 0x3f800000U, GX_VMA(0x49c61d8ULL), GX_VMA(0x49c61fcULL), 0);
    reg_prop(0xb4, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x461c4000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c624aULL), GX_VMA(0x49c626cULL), 1);
    reg_prop(0xb8, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x47c35000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c629aULL), GX_VMA(0x49c62b8ULL), 1);
    reg_prop(0xbc, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c62e2ULL), GX_VMA(0x49c6300ULL), 1);
    reg_prop(0xc0, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x3f800000U, 0x3a83126fU, 0x3f800000U, GX_VMA(0x49c632aULL), GX_VMA(0x49c634aULL), 1);
    reg_prop(0xc4, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x47c35000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c6376ULL), GX_VMA(0x49c63a8ULL), 1);
    sub_e33600(*reinterpret_cast<void**>(param_sys + 0x28), slot, 1);

    // Subgroup: AlphaDof (+0xc8 .. +0xe8)
    sub_e333e0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c63e6ULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c63faULL)), 1, 1);
    reg_prop(0xc8, 0x3, 1, kIntVtblM, kIntVtblS, 0, 1, 0, 0x3f800000U, GX_VMA(0x49cf3a4ULL), GX_VMA(0x49c6438ULL), 0);
    reg_prop(0xcc, 0x3, 1, kIntVtblM, kIntVtblS, 0, 1, 0, 0x3f800000U, GX_VMA(0x49c6480ULL), GX_VMA(0x49c64a6ULL), 0);
    reg_prop(0xd0, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c64d8ULL), GX_VMA(0x49c64faULL), 1);
    reg_prop(0xd4, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x42c80000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c6528ULL), GX_VMA(0x49c6548ULL), 1);
    reg_prop(0xd8, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x447a0000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c6574ULL), GX_VMA(0x49c658aULL), 1);
    reg_prop(0xdc, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x459c4000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c65acULL), GX_VMA(0x49c65d8ULL), 1);
    reg_prop(0xe0, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x459c4000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c6610ULL), GX_VMA(0x49c663cULL), 1);
    reg_prop(0xe4, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c6674ULL), GX_VMA(0x49c669eULL), 1);
    reg_prop(0xe8, 0x9, 0, kFltVtblM, kFltVtblS, 0x00000000U, 0x3f800000U, 0x3c23d70aU, 0x3f800000U, GX_VMA(0x49c66d4ULL), GX_VMA(0x49c66feULL), 1);

    sub_e33600(*reinterpret_cast<void**>(param_sys + 0x28), slot, 1);
    sub_e33600(*reinterpret_cast<void**>(param_sys + 0x28), slot, 1);

    sub_218e2b0(param_sys + 0x130, *reinterpret_cast<void**>(param_sys + 0x28), 0);
    sub_e41f00(param_sys + 0x1b8, *reinterpret_cast<void**>(param_sys + 0x28), 0, slot, 0, 0.0f);
    sub_21874c0(param_sys, slot);
    return true;
}

// 0x21ecbc0 (395 B): GXParamBankManagerF_ClearAllBanks
extern "C" void sub_21ecbc0(uint8_t* self) {
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    HashNode* out_cur = outer_sentinel->next;
    while (out_cur != outer_sentinel) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        if (inner_table) {
            HashNode* in_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            HashNode* in_cur = in_sentinel->next;
            while (in_cur != in_sentinel) {
                uint8_t* entry = static_cast<uint8_t*>(in_cur->value);
                if (entry) {
                    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
                    uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(entry);
                    reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl))(entry);
                    alloc->vptr->Free(alloc, entry);
                    in_cur->value = nullptr;
                }
                HashIteratorPair in_next{};
                sub_21effd0(&in_next, static_cast<uint8_t*>(out_cur->value), in_cur);
                in_cur = in_next.third;
                inner_table = static_cast<uint8_t*>(out_cur->value);
                in_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            }

            DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
            void* in_buckets = *reinterpret_cast<void**>(inner_table + 0x28);
            if (in_buckets) {
                DLAllocator* b_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x40);
                b_alloc->vptr->Free(b_alloc, in_buckets);
                *reinterpret_cast<uint64_t*>(inner_table + 0x28) = 0;
                *reinterpret_cast<uint64_t*>(inner_table + 0x30) = 0;
                *reinterpret_cast<uint64_t*>(inner_table + 0x38) = 0;
            }
            HashNode* in_s = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            HashNode* n = in_s->next;
            in_s->next = in_s;
            in_s->prev = in_s;
            *reinterpret_cast<uint64_t*>(inner_table + 0x10) = 0;
            while (n != in_s) {
                HashNode* next_n = n->next;
                DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
                n_alloc->vptr->Free(n_alloc, n);
                n = next_n;
            }
            DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
            n_alloc->vptr->Free(n_alloc, in_s);
            alloc->vptr->Free(alloc, inner_table);
            out_cur->value = nullptr;
        }
        HashIteratorPair out_next{};
        sub_21f0140(&out_next, self + 0x08, out_cur);
        out_cur = out_next.third;
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21ecac0 (255 B): GXParamBankManagerF_DestructBody
extern "C" void sub_21ecac0(uint8_t* self) {
    sub_21ecbc0(self);
    sub_e760a0(self + 0x230);

    uint64_t str_cap = *reinterpret_cast<uint64_t*>(self + 0x1a0);
    if (str_cap >= 8) {
        if (str_cap + 1 >= 0x41) {
            sub_20b55b0(
                reinterpret_cast<const char*>(GX_VMA(0x4940298ULL)),
                0x120,
                reinterpret_cast<const char*>(GX_VMA(0x4940300ULL)));
        }
        self[0x229] &= 0xfdU;
    }
    *reinterpret_cast<uint64_t*>(self + 0x1a0) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x198) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x188) = 0;

    void* outer_buckets = *reinterpret_cast<void**>(self + 0x30);
    if (outer_buckets) {
        DLAllocator* b_alloc = *reinterpret_cast<DLAllocator**>(self + 0x48);
        b_alloc->vptr->Free(b_alloc, outer_buckets);
        *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x38) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }

    HashNode* out_s = *reinterpret_cast<HashNode**>(self + 0x10);
    HashNode* cur = out_s->next;
    out_s->next = out_s;
    out_s->prev = out_s;
    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    while (cur != out_s) {
        HashNode* next_n = cur->next;
        DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(self + 0x20);
        n_alloc->vptr->Free(n_alloc, cur);
        cur = next_n;
    }
    DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(self + 0x20);
    n_alloc->vptr->Free(n_alloc, out_s);
}

// 0x21ec470 (1615 B): GXParamBankManagerF_Construct
extern "C" void sub_21ec470(uint8_t* self, DLAllocator* alloc) {
    *reinterpret_cast<DLAllocator**>(self + 0x00) = alloc;
    uint8_t heap_info[184]{};
    alloc->vptr->GetHeapInfo(heap_info, alloc, 0);
    if ((heap_info[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
    *reinterpret_cast<DLAllocator**>(self + 0x20) = alloc;

    HashNode* sentinel = static_cast<HashNode*>(alloc->vptr->AllocateAligned(alloc, 0x20, 8));
    if (!sentinel) {
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    } else {
        sentinel->next = sentinel;
        sentinel->prev = sentinel;
    }
    *reinterpret_cast<HashNode**>(self + 0x10) = sentinel;

    *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    *reinterpret_cast<DLAllocator**>(self + 0x48) = alloc;
    *reinterpret_cast<uint32_t*>(self + 0x60) = 0x3f800000U;

    HashIteratorPair dummy_pair{};
    const HashIteratorPair fill_iter{nullptr, nullptr, sentinel};
    sub_21f0ad0(&dummy_pair, self + 0x28, HashIteratorPair{nullptr, nullptr, nullptr}, 0x10, &fill_iter);
    *reinterpret_cast<uint64_t*>(self + 0x50) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x58) = 8;

    auto init_dof_block = [](uint8_t* blk) {
        *reinterpret_cast<uint32_t*>(blk + 0x60) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x64) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x68) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x6c) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x70) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x74) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x78) = 0x459c4000U;
        *reinterpret_cast<uint32_t*>(blk + 0x7c) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x80) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x00) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x0c) = 0x422d1168U;
        *reinterpret_cast<uint32_t*>(blk + 0x04) = 0x40b33333U;
        *reinterpret_cast<uint32_t*>(blk + 0x08) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x10) = 0x3f000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x14) = 0x42340000U;
        *reinterpret_cast<uint32_t*>(blk + 0x18) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x1c) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x28) = 1;
        *reinterpret_cast<uint32_t*>(blk + 0x20) = 3;
        *reinterpret_cast<uint32_t*>(blk + 0x24) = 3;
        *reinterpret_cast<uint32_t*>(blk + 0x2c) = 0xffffffffU;
        *reinterpret_cast<uint32_t*>(blk + 0x30) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x34) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x48) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x38) = 0;
        *reinterpret_cast<uint32_t*>(blk + 0x3c) = 0x40000000U;
        *reinterpret_cast<uint32_t*>(blk + 0x40) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x44) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x4c) = 0x42c80000U;
        *reinterpret_cast<uint32_t*>(blk + 0x50) = 0x43160000U;
        *reinterpret_cast<uint32_t*>(blk + 0x54) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x58) = 0x3f800000U;
        *reinterpret_cast<uint32_t*>(blk + 0x5c) = 0x461c4000U;
        *reinterpret_cast<uint32_t*>(blk + 0x84) = 0x3dcccccdU;
    };

    init_dof_block(self + 0x68);
    sub_2bbe578(self + 0xf0, self + 0x68, 0x88);
    *reinterpret_cast<uint32_t*>(self + 0x178) = 0;

    sub_2bbe578(self + 0x1a8, heap_info, 0x81);
    self[0x229] = 0;
    *reinterpret_cast<uint64_t*>(self + 0x1a0) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x198) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x188) = 0;
    self[0x22a] = 1;
    sub_2935350(self + 0x180, 0x3f, 0);

    uint16_t* str_buf = reinterpret_cast<uint16_t*>(self + 0x188);
    if (*reinterpret_cast<uint64_t*>(self + 0x1a0) >= 8) {
        str_buf = *reinterpret_cast<uint16_t**>(self + 0x188);
    }
    *reinterpret_cast<uint64_t*>(self + 0x198) = 0;
    *str_buf = 0;

    sub_e75d90(self + 0x230);
    sub_e75f70(self + 0x230);

    sub_29354e0(self + 0x180, reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// Group G (ColorGrading[Yebis]): 0x21e8960 .. 0x21ebf20 (22 functions)
// ============================================================================

// 0x21ebf20 (1352 B): GXParamBankManagerG_InnerHashTable_RehashInsert
extern "C" HashIteratorPair* sub_21ebf20(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    return RehashInsert_Impl(out_pair, table_vec, where_iter, new_bucket_count, fill_val);
}

// 0x21eb9d0 (1352 B): GXParamBankManagerG_OuterHashTable_RehashInsert
extern "C" HashIteratorPair* sub_21eb9d0(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    return RehashInsert_Impl(out_pair, table_vec, where_iter, new_bucket_count, fill_val);
}

// 0x21eb5c0 (1040 B): GXParamBankManagerG_InnerHashTable_Insert
extern "C" InsertResult* sub_21eb5c0(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashIteratorPair node_iter)
{
    return HashTable_Insert_Impl(out_res, table, key_ptr, node_iter.third, &sub_21ebf20);
}

// 0x21eb1b0 (1040 B): GXParamBankManagerG_OuterHashTable_Insert
extern "C" InsertResult* sub_21eb1b0(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashIteratorPair node_iter)
{
    return HashTable_Insert_Impl(out_res, table, key_ptr, node_iter.third, &sub_21eb9d0);
}

// 0x21eb040 (363 B): GXParamBankManagerG_OuterHashTable_EraseNode
extern "C" HashIteratorPair* sub_21eb040(HashIteratorPair* out_next, uint8_t* table, HashNode* node) {
    return HashTable_EraseNode_Impl(out_next, table, node);
}

// 0x21eaed0 (363 B): GXParamBankManagerG_InnerHashTable_EraseNode
extern "C" HashIteratorPair* sub_21eaed0(HashIteratorPair* out_next, uint8_t* table, HashNode* node) {
    return HashTable_EraseNode_Impl(out_next, table, node);
}

// 0x21eaea0 (40 B): GXParamBankManagerG_DebugCallback_Invoke
extern "C" void sub_21eaea0(uint8_t* self) {
    if ((self[0x0b] & 0x40) != 0) {
        uint8_t* target = *reinterpret_cast<uint8_t**>(self + 0x10) + *reinterpret_cast<int64_t*>(self + 0x28);
        uintptr_t fn_ptr = *reinterpret_cast<uintptr_t*>(self + 0x20);
        if ((fn_ptr & 1) != 0) {
            uintptr_t vtable = *reinterpret_cast<uintptr_t*>(target);
            fn_ptr = *reinterpret_cast<uintptr_t*>(vtable + fn_ptr - 1);
        }
        uint32_t arg = *reinterpret_cast<uint32_t*>(self + 0x18);
        reinterpret_cast<void (*)(void*, uint32_t)>(fn_ptr)(target, arg);
    }
}

// 0x21eae90 (5 B): GXParamBankManagerG_DebugCallback_Delete
extern "C" void sub_21eae90(void* self) {
    sub_2bbe588(self);
}

// 0x21eae70 (22 B): GXParamBankManagerG_DebugCallback_Destroy
extern "C" void sub_21eae70(uint8_t* self) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xbfffffffU;
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;
}

// 0x21ea730 (1708 B): GXParamBankManagerG_BuildParamEditIdListGui
extern "C" void sub_21ea730(uint8_t* self) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x40);
    uint32_t bank_id = *reinterpret_cast<uint32_t*>(param_sys + slot * 4 + 0x588);

    sub_2936030(self + 0x4f0, reinterpret_cast<const char16_t*>(GX_VMA(0x49cbebeULL)), bank_id);

    DLAllocator* gui_alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));
    uint8_t* gui_obj = static_cast<uint8_t*>(gui_alloc->vptr->AllocateAligned(gui_alloc, 0x178, 8));
    if (gui_obj) {
        const char16_t* title_ptr = reinterpret_cast<const char16_t*>(self + 0x4f8);
        if (*reinterpret_cast<uint64_t*>(self + 0x510) >= 8) {
            title_ptr = *reinterpret_cast<const char16_t**>(self + 0x4f8);
        }
        uint64_t desc[2] = {0, reinterpret_cast<uint64_t>(title_ptr)};
        sub_2362cf0(gui_obj, desc);
    }

    uint8_t* main_win_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6bc8ULL));
    uint8_t* main_win = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(main_win_root + 0x10) + 0x18);
    struct { uint16_t r[4]; uint64_t f; } rect_pack = {{0, 0, 0x12c, 0x190}, 0x51502ULL};

    if (gui_obj && (gui_obj[0x0b] & 0x40) != 0 && gui_obj[0x6f] == 0 &&
        main_win != gui_obj && main_win != nullptr && (main_win[0x0b] & 0x40) != 0)
    {
        uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(main_win);
        auto check_fn = *reinterpret_cast<int (*)(void*, void*)>(vtbl + 0x28);
        if (check_fn(main_win, gui_obj) != 0) {
            sub_2328e40(gui_obj, main_win, *reinterpret_cast<void**>(main_win + 0x20), &rect_pack);
        }
    }

    uint64_t list_builder[6]{};
    sub_236c4d0(list_builder, gui_obj);

    auto release_ref_str = [](uint8_t* s) {
        if (s && --(*reinterpret_cast<uint32_t*>(s + 0x38)) == 0) {
            DLAllocator* str_alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e10ULL));
            if (*reinterpret_cast<uint64_t*>(s + 0x20) >= 8) {
                DLAllocator* buf_alloc = *reinterpret_cast<DLAllocator**>(s + 0x28);
                buf_alloc->vptr->Free(buf_alloc, *reinterpret_cast<void**>(s + 0x08));
            }
            str_alloc->vptr->Free(str_alloc, s);
        }
    };

    uint64_t dummy_str[2]{};
    sub_2ba7d40(dummy_str, reinterpret_cast<const char16_t*>(GX_VMA(0x49d27c0ULL)));
    uint64_t item_desc[2] = {dummy_str[0], dummy_str[1]};
    void* dummy_node = sub_236c900(list_builder, item_desc, 0);
    release_ref_str(reinterpret_cast<uint8_t*>(dummy_str[0]));
    if (dummy_node) {
        sub_e72260(self + 0x5a0, list_builder);
    }

    uint64_t h = ComputeParamBankHash(static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL);
    uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x50);
    uint64_t bucket = h & mask;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= bucket) bucket += ~(mask >> 1);

    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
    HashNode* b_cur = *reinterpret_cast<HashNode**>(outer_buckets + bucket * 48 + 0x10);
    if (b_cur != outer_sentinel) {
        HashNode* b_end = (*reinterpret_cast<HashNode**>(outer_buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
        while (b_cur != b_end && b_cur->key < bank_id) {
            b_cur = b_cur->next;
        }
        if (b_cur != b_end && bank_id >= b_cur->key) {
            uint8_t* inner_table = static_cast<uint8_t*>(b_cur->value);
            HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
                if (in_cur->key == 0xffffffffU) continue;
                uint64_t entry_str[2]{};
                sub_2ba7d40(entry_str, reinterpret_cast<const char16_t*>(GX_VMA(0x49cbef4ULL)), in_cur->key);
                uint64_t entry_desc[2] = {entry_str[0], entry_str[1]};
                void* entry_node = sub_236c900(list_builder, entry_desc, 0);
                release_ref_str(reinterpret_cast<uint8_t*>(entry_str[0]));
                if (entry_node) {
                    sub_e72260(in_cur->value, list_builder);
                }
            }
        }
    }
    sub_236c720(list_builder);
}

// 0x21ea4c0 (612 B): GXParamBankManagerG_DebugCommandCallback
extern "C" void sub_21ea4c0(uint8_t* self, void* parent_gui);

// 0x21ea1e0 (732 B): GXParamBankManagerG_LookupEntryOrDefault
extern "C" void* sub_21ea1e0(uint8_t* self, const uint64_t* packed_key) {
    uint64_t raw = *packed_key;
    uint32_t bank_id = static_cast<uint32_t>(raw);
    uint32_t inner_id = static_cast<uint32_t>(raw >> 32);
    if (bank_id == 0xffffffffU && inner_id == 0xffffffffU) {
        inner_id = 0;
        bank_id = 1;
    }

    uint64_t h1 = ComputeParamBankHash(static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL);
    uint64_t mask1 = *reinterpret_cast<uint64_t*>(self + 0x50);
    uint64_t b1 = h1 & mask1;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= b1) b1 += ~(mask1 >> 1);

    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
    HashNode* cur1 = *reinterpret_cast<HashNode**>(outer_buckets + b1 * 48 + 0x10);
    if (cur1 != outer_sentinel) {
        HashNode* end1 = (*reinterpret_cast<HashNode**>(outer_buckets + (b1 * 2 + 1) * 24 + 0x10))->next;
        while (cur1 != end1 && cur1->key < bank_id) {
            cur1 = cur1->next;
        }
        if (cur1 != end1 && bank_id >= cur1->key) {
            uint8_t* inner_table = static_cast<uint8_t*>(cur1->value);
            uint64_t h2 = ComputeParamBankHash(static_cast<uint64_t>(inner_id) ^ 0xdeadbeefULL);
            uint64_t mask2 = *reinterpret_cast<uint64_t*>(inner_table + 0x48);
            uint64_t b2 = h2 & mask2;
            if (*reinterpret_cast<uint64_t*>(inner_table + 0x50) <= b2) b2 += ~(mask2 >> 1);

            HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            uint8_t* inner_buckets = *reinterpret_cast<uint8_t**>(inner_table + 0x28);
            HashNode* cur2 = *reinterpret_cast<HashNode**>(inner_buckets + b2 * 48 + 0x10);
            HashNode* found2 = inner_sentinel;
            if (cur2 != inner_sentinel) {
                HashNode* end2 = (*reinterpret_cast<HashNode**>(inner_buckets + (b2 * 2 + 1) * 24 + 0x10))->next;
                while (cur2 != end2 && cur2->key < inner_id) {
                    cur2 = cur2->next;
                }
                if (cur2 != end2 && inner_id >= cur2->key) {
                    found2 = cur2;
                }
            }
            if (found2 != *reinterpret_cast<HashNode**>(static_cast<uint8_t*>(cur1->value) + 0x08)) {
                return found2->value;
            }
        }
    }

    uint64_t h_fb1 = ComputeParamBankHash(0xdeadbeeeULL);
    uint64_t b_fb1 = h_fb1 & mask1;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= b_fb1) b_fb1 += ~(mask1 >> 1);
    HashNode* cur_fb1 = *reinterpret_cast<HashNode**>(outer_buckets + b_fb1 * 48 + 0x10);
    if (cur_fb1 != outer_sentinel) {
        HashNode* end_fb1 = (*reinterpret_cast<HashNode**>(outer_buckets + (b_fb1 * 2 + 1) * 24 + 0x10))->next;
        while (cur_fb1 != end_fb1 && cur_fb1->key == 0) {
            cur_fb1 = cur_fb1->next;
        }
        if (cur_fb1 != end_fb1 && cur_fb1->key <= 1) {
            uint8_t* inner_table = static_cast<uint8_t*>(cur_fb1->value);
            uint64_t h_fb2 = ComputeParamBankHash(0xdeadbeefULL);
            uint64_t mask2 = *reinterpret_cast<uint64_t*>(inner_table + 0x48);
            uint64_t b_fb2 = h_fb2 & mask2;
            if (*reinterpret_cast<uint64_t*>(inner_table + 0x50) <= b_fb2) b_fb2 += ~(mask2 >> 1);
            HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            uint8_t* inner_buckets = *reinterpret_cast<uint8_t**>(inner_table + 0x28);
            HashNode* cur_fb2 = *reinterpret_cast<HashNode**>(inner_buckets + b_fb2 * 48 + 0x10);
            if (cur_fb2 != inner_sentinel) {
                HashNode* end_fb2 = *reinterpret_cast<HashNode**>(*reinterpret_cast<uint8_t**>(inner_buckets + (b_fb2 * 2 + 1) * 24 + 0x10));
                if (cur_fb2 != end_fb2 && cur_fb2->key == 0) {
                    if (cur_fb2 != *reinterpret_cast<HashNode**>(static_cast<uint8_t*>(cur_fb1->value) + 0x08)) {
                        return cur_fb2->value;
                    }
                }
            }
        }
    }
    return self + 0x5a0;
}

extern "C" void sub_21e9090(uint8_t* self);

// 0x21ea170 (103 B): GXParamBankManagerG_ForEachEntry_Apply
extern "C" void sub_21ea170(uint8_t* self) {
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    for (HashNode* out_cur = outer_sentinel->next; out_cur != outer_sentinel; out_cur = out_cur->next) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
            sub_e71bf0(in_cur->value);
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21ea0e0 (138 B): GXParamBankManagerG_ForEachEntry_Init
extern "C" void sub_21ea0e0(uint8_t* self) {
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    for (HashNode* out_cur = outer_sentinel->next; out_cur != outer_sentinel; out_cur = out_cur->next) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
            uint8_t* entry = static_cast<uint8_t*>(in_cur->value);
            uint32_t inner_id = in_cur->key;
            uint32_t bank_id = out_cur->key;
            *reinterpret_cast<uint32_t*>(entry + 0x290) = inner_id;
            *reinterpret_cast<uint32_t*>(entry + 0x294) = bank_id;
            sub_e71c50(entry, inner_id, 0.0f);
            sub_e71dc0(in_cur->value, *reinterpret_cast<void**>(self + 0x00));
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21ea4c0 (612 B): GXParamBankManagerG_DebugCommandCallback
extern "C" void sub_21ea4c0(uint8_t* self, void* parent_gui) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x40);
    sub_21877f0(param_sys, parent_gui, slot, 0);

    DLAllocator* gui_alloc = *reinterpret_cast<DLAllocator**>(GX_VMA(0x5540e08ULL));

    auto register_btn = [&](const char16_t* label, void (*fn_ptr)(uint8_t*)) {
        uint64_t empty_desc[2]{};
        void* btn = sub_236d620(parent_gui, empty_desc);
        uint64_t label_desc[2] = {0, reinterpret_cast<uint64_t>(label)};
        uint8_t* cb = static_cast<uint8_t*>(gui_alloc->vptr->AllocateAligned(gui_alloc, 0x30, 8));
        if (cb) {
            *reinterpret_cast<uint32_t*>(cb + 0x08) = (*reinterpret_cast<uint32_t*>(cb + 0x08) & 0x80000000U) | 0x40000000U;
            *reinterpret_cast<uint64_t*>(cb + 0x00) = GX_VMA(0x53b70e0ULL);
            *reinterpret_cast<uint8_t**>(cb + 0x10) = self;
            *reinterpret_cast<uint32_t*>(cb + 0x18) = 0;
            *reinterpret_cast<void (**)(uint8_t*)>(cb + 0x20) = fn_ptr;
            *reinterpret_cast<uint64_t*>(cb + 0x28) = 0;
        }
        sub_235e4c0(btn, label_desc, cb);
    };

    register_btn(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2730ULL)), &sub_21ea0e0);
    register_btn(reinterpret_cast<const char16_t*>(GX_VMA(0x49d275eULL)), &sub_21ea170);
    register_btn(reinterpret_cast<const char16_t*>(GX_VMA(0x49d2790ULL)), &sub_21ea730);
}

// 0x21ea020 (178 B): GXParamBankManagerG_ApplyParamsToScene
extern "C" void sub_21ea020(uint8_t* self, float weight) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    void* scene_cam = *reinterpret_cast<void**>(sys_root + 0x5af8);
    if (!scene_cam) {
        scene_cam = *reinterpret_cast<void**>(sys_root + 0x5ae0);
        if (!scene_cam) {
            scene_cam = *reinterpret_cast<void**>(GX_VMA(0x5540dc8ULL));
        }
    }

    sub_e720c0(self + 0x5a0, scene_cam, weight);

    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    for (HashNode* out_cur = outer_sentinel->next; out_cur != outer_sentinel; out_cur = out_cur->next) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        for (HashNode* in_cur = inner_sentinel->next; in_cur != inner_sentinel; in_cur = in_cur->next) {
            sub_e720c0(in_cur->value, scene_cam, weight);
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21e9ff0 (40 B): GXParamBankManagerG_ForwardLookup
extern "C" void* sub_21e9ff0(void* self_unused, uint32_t bank_id, uint32_t inner_id) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x40);
    return sub_2186da0(param_sys, slot, bank_id, inner_id, self_unused ? -1LL : -1LL);
}

// 0x21e9d30 (698 B): GXParamBankManagerG_PurgeInvalidEntriesInBank
extern "C" void sub_21e9d30(
    uint8_t* self,
    void* unused_rsi,
    uint32_t bank_id,
    uint32_t override_slot_idx,
    float blend_time)
{
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x40);
    sub_2185b50(param_sys, slot, bank_id, override_slot_idx, blend_time);
    *reinterpret_cast<uint8_t*>(GX_VMA(0x5540678ULL)) = (unused_rsi != nullptr) ? 1 : 1;

    uint64_t h = ComputeParamBankHash(static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL);
    uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x50);
    uint64_t bucket = h & mask;
    if (*reinterpret_cast<uint64_t*>(self + 0x58) <= bucket) bucket += ~(mask >> 1);

    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
    HashNode* out_cur = *reinterpret_cast<HashNode**>(outer_buckets + bucket * 48 + 0x10);
    if (out_cur == outer_sentinel) return;

    HashNode* out_end = (*reinterpret_cast<HashNode**>(outer_buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
    while (out_cur != out_end && out_cur->key < bank_id) {
        out_cur = out_cur->next;
    }
    if (out_cur == out_end || out_cur->key > bank_id) return;

    uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
    if (inner_table) {
        HashNode* inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        HashNode* in_cur = inner_sentinel->next;
        while (in_cur != inner_sentinel) {
            bool valid = sub_2185e00(param_sys, slot, in_cur->key, bank_id);
            uint8_t* entry = static_cast<uint8_t*>(in_cur->value);
            if (valid) {
                entry[0x288] = 1;
                in_cur = in_cur->next;
            } else {
                if (entry) {
                    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
                    uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(entry);
                    reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl))(entry);
                    alloc->vptr->Free(alloc, entry);
                    in_cur->value = nullptr;
                }
                HashIteratorPair next_pair{};
                sub_21eaed0(&next_pair, static_cast<uint8_t*>(out_cur->value), in_cur);
                in_cur = next_pair.third;
            }
            inner_table = static_cast<uint8_t*>(out_cur->value);
            inner_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        }
        if (*reinterpret_cast<uint64_t*>(inner_table + 0x10) != 0) {
            return;
        }
        DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
        void* inner_buckets = *reinterpret_cast<void**>(inner_table + 0x28);
        if (inner_buckets) {
            DLAllocator* b_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x40);
            b_alloc->vptr->Free(b_alloc, inner_buckets);
            *reinterpret_cast<uint64_t*>(inner_table + 0x28) = 0;
            *reinterpret_cast<uint64_t*>(inner_table + 0x30) = 0;
            *reinterpret_cast<uint64_t*>(inner_table + 0x38) = 0;
        }
        HashNode* in_s = *reinterpret_cast<HashNode**>(inner_table + 0x08);
        HashNode* n = in_s->next;
        in_s->next = in_s;
        in_s->prev = in_s;
        *reinterpret_cast<uint64_t*>(inner_table + 0x10) = 0;
        while (n != in_s) {
            HashNode* next_n = n->next;
            DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
            n_alloc->vptr->Free(n_alloc, n);
            n = next_n;
        }
        DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
        n_alloc->vptr->Free(n_alloc, in_s);
        alloc->vptr->Free(alloc, inner_table);
        out_cur->value = nullptr;
    }

    HashIteratorPair out_next{};
    sub_21eb040(&out_next, self + 0x08, out_cur);
}

// 0x21e9420 (2318 B): GXParamBankManagerG_ReloadAndRegisterBank
extern "C" void sub_21e9420(uint8_t* self, void* ctx, uint32_t bank_id, uint32_t unk_flag, float weight) {
    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = *reinterpret_cast<uint32_t*>(param_sys + 0x40);
    uint32_t count = sub_2184cf0(param_sys, slot, ctx);
    if (count > 0) {
        uint8_t heap_info[184]{};
        uint64_t raw_key = static_cast<uint64_t>(bank_id) ^ 0xdeadbeefULL;
        uint64_t h = ComputeParamBankHash(raw_key);
        uint64_t mask = *reinterpret_cast<uint64_t*>(self + 0x50);
        uint64_t bucket = h & mask;
        if (*reinterpret_cast<uint64_t*>(self + 0x58) <= bucket) bucket += ~(mask >> 1);

        HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
        uint8_t* outer_buckets = *reinterpret_cast<uint8_t**>(self + 0x30);
        HashNode* out_cur = *reinterpret_cast<HashNode**>(outer_buckets + bucket * 48 + 0x10);
        if (out_cur != outer_sentinel) {
            HashNode* out_end = (*reinterpret_cast<HashNode**>(outer_buckets + (bucket * 2 + 1) * 24 + 0x10))->next;
            while (out_cur != out_end && out_cur->key < bank_id) {
                out_cur = out_cur->next;
            }
            if (out_cur == out_end || out_cur->key > bank_id) {
                out_cur = outer_sentinel;
            }
        }

        if (out_cur == outer_sentinel) {
            DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
            uint8_t* new_inner = static_cast<uint8_t*>(alloc->vptr->AllocateAligned(alloc, 0x60, 8));
            if (new_inner) {
                alloc->vptr->GetHeapInfo(heap_info, alloc, 0);
                if ((heap_info[0] & 0x20) == 0) {
                    sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
                }
                *reinterpret_cast<uint64_t*>(new_inner + 0x10) = 0;
                *reinterpret_cast<uint64_t*>(new_inner + 0x08) = 0;
                *reinterpret_cast<DLAllocator**>(new_inner + 0x18) = alloc;
                HashNode* in_s = static_cast<HashNode*>(alloc->vptr->AllocateAligned(alloc, 0x20, 8));
                if (!in_s) {
                    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                } else {
                    in_s->next = in_s;
                    in_s->prev = in_s;
                }
                *reinterpret_cast<HashNode**>(new_inner + 0x08) = in_s;
                *reinterpret_cast<uint64_t*>(new_inner + 0x28) = 0;
                *reinterpret_cast<uint64_t*>(new_inner + 0x30) = 0;
                *reinterpret_cast<uint64_t*>(new_inner + 0x38) = 0;
                *reinterpret_cast<DLAllocator**>(new_inner + 0x40) = alloc;
                *reinterpret_cast<uint32_t*>(new_inner + 0x58) = 0x3f800000U;
                HashIteratorPair dummy_p{};
                const HashIteratorPair fill_in{nullptr, nullptr, in_s};
                sub_21ebf20(&dummy_p, new_inner + 0x20, HashIteratorPair{nullptr, nullptr, nullptr}, 0x10, &fill_in);
                *reinterpret_cast<uint64_t*>(new_inner + 0x48) = 7;
                *reinterpret_cast<uint64_t*>(new_inner + 0x50) = 8;
            }

            HashNode* head = outer_sentinel->next;
            DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(self + 0x20);
            HashNode* new_out_node = static_cast<HashNode*>(n_alloc->vptr->AllocateAligned(n_alloc, 0x20, 8));
            if (!new_out_node) {
                sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                return;
            }
            new_out_node->next = head ? head : new_out_node;
            new_out_node->prev = head ? head->prev : new_out_node;
            new_out_node->key = bank_id;
            new_out_node->value = new_inner;

            uint64_t cur_cnt = *reinterpret_cast<uint64_t*>(self + 0x18);
            if (cur_cnt == 0x7fffffffffffffeULL) {
                sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
            }
            *reinterpret_cast<uint64_t*>(self + 0x18) = cur_cnt + 1;
            head->prev = new_out_node;
            new_out_node->prev->next = new_out_node;

            InsertResult ins_res{};
            HashNode* first_out = (*reinterpret_cast<HashNode**>(self + 0x10))->next;
            sub_21eb1b0(&ins_res, self + 0x08, &first_out->key, HashIteratorPair{nullptr, nullptr, first_out});
            out_cur = ins_res.node ? ins_res.node : first_out;
        }

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t inner_id = sub_2184de0(param_sys, slot, ctx, i);
            uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
            uint64_t ih = ComputeParamBankHash(static_cast<uint64_t>(inner_id) ^ 0xdeadbeefULL);
            uint64_t im = *reinterpret_cast<uint64_t*>(inner_table + 0x48);
            uint64_t ib = ih & im;
            if (*reinterpret_cast<uint64_t*>(inner_table + 0x50) <= ib) ib += ~(im >> 1);

            HashNode* in_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            uint8_t* in_buckets = *reinterpret_cast<uint8_t**>(inner_table + 0x28);
            HashNode* in_cur = *reinterpret_cast<HashNode**>(in_buckets + ib * 48 + 0x10);
            HashNode* in_found = in_sentinel;
            if (in_cur != in_sentinel) {
                HashNode* in_end = (*reinterpret_cast<HashNode**>(in_buckets + (ib * 2 + 1) * 24 + 0x10))->next;
                while (in_cur != in_end && in_cur->key < inner_id) {
                    in_cur = in_cur->next;
                }
                if (in_cur != in_end && inner_id >= in_cur->key) {
                    in_found = in_cur;
                }
            }

            if (in_found != in_sentinel) {
                static_cast<uint8_t*>(in_found->value)[0x288] = 1;
            } else {
                DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
                uint8_t* entry = static_cast<uint8_t*>(alloc->vptr->AllocateAligned(alloc, 0x2a8, 8));
                if (!entry) {
                    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    continue;
                }
                sub_e715a0(entry);
                sub_e71740(entry);
                *reinterpret_cast<uint32_t*>(entry + 0x290) = inner_id;
                *reinterpret_cast<uint32_t*>(entry + 0x294) = bank_id;
                sub_e71c50(entry, inner_id, 0.0f);
                sub_e71dc0(entry, *reinterpret_cast<void**>(self + 0x00));

                if (sub_2183dd0(param_sys, ctx) != 0) {
                    uint8_t tmp_str[0x130]{};
                    sub_2bbe578(tmp_str + 0x28, heap_info, 0x101);
                    tmp_str[0x129] = 0;
                    *reinterpret_cast<uint64_t*>(tmp_str + 0x20) = 7;
                    *reinterpret_cast<uint64_t*>(tmp_str + 0x18) = 0;
                    *reinterpret_cast<uint16_t*>(tmp_str + 0x08) = 0;
                    tmp_str[0x12a] = 1;
                    sub_29097e0(tmp_str, 0x7f, 0);

                    uint16_t* s_ptr = (*reinterpret_cast<uint64_t*>(tmp_str + 0x20) > 7)
                        ? *reinterpret_cast<uint16_t**>(tmp_str + 0x08)
                        : reinterpret_cast<uint16_t*>(tmp_str + 0x08);
                    *reinterpret_cast<uint64_t*>(tmp_str + 0x18) = 0;
                    *s_ptr = 0;

                    const void* base_path = sub_2183880(param_sys, ctx);
                    sub_192b330(tmp_str,
                                reinterpret_cast<const char16_t*>(GX_VMA(0x49d2cdcULL)),
                                base_path,
                                reinterpret_cast<const char16_t*>(GX_VMA(0x49c573cULL)));

                    const void* formatted_cstr = (*reinterpret_cast<uint64_t*>(tmp_str + 0x20) > 7)
                        ? *reinterpret_cast<const void**>(tmp_str + 0x08)
                        : reinterpret_cast<const void*>(tmp_str + 0x08);
                    sub_e72030(entry, formatted_cstr);

                    uint64_t cap = *reinterpret_cast<uint64_t*>(tmp_str + 0x20);
                    if (cap >= 8) {
                        void* heap_buf = *reinterpret_cast<void**>(tmp_str + 0x08);
                        if (cap + 1 >= 0x81) {
                            DLAllocator* s_alloc = *reinterpret_cast<DLAllocator**>(tmp_str + 0x28);
                            if (s_alloc && heap_buf) {
                                s_alloc->vptr->Free(s_alloc, heap_buf);
                            } else {
                                sub_2bbe588(heap_buf);
                            }
                        } else {
                            tmp_str[0x129] &= 0xfdU;
                        }
                    }
                } else {
                    const void* base_path = sub_2183880(param_sys, ctx);
                    sub_e72030(entry, base_path);
                }

                HashNode* in_head = in_sentinel->next;
                DLAllocator* in_n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
                HashNode* new_in_node = static_cast<HashNode*>(in_n_alloc->vptr->AllocateAligned(in_n_alloc, 0x20, 8));
                if (!new_in_node) {
                    sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                    continue;
                }
                new_in_node->next = in_head ? in_head : new_in_node;
                new_in_node->prev = in_head ? in_head->prev : new_in_node;
                new_in_node->key = inner_id;
                new_in_node->value = entry;

                uint64_t in_cnt = *reinterpret_cast<uint64_t*>(inner_table + 0x10);
                if (in_cnt == 0x7fffffffffffffeULL) {
                    sub_2bbf1b8(reinterpret_cast<const char*>(GX_VMA(0x4940378ULL)));
                }
                *reinterpret_cast<uint64_t*>(inner_table + 0x10) = in_cnt + 1;
                in_head->prev = new_in_node;
                new_in_node->prev->next = new_in_node;

                InsertResult in_res{};
                HashNode* first_in = (*reinterpret_cast<HashNode**>(inner_table + 0x08))->next;
                sub_21eb5c0(&in_res, inner_table, &first_in->key, HashIteratorPair{nullptr, nullptr, first_in});
            }
        }
    }
    sub_2184ff0(param_sys, slot, bank_id, unk_flag, ctx, weight);
}

// 0x21e9220 (499 B): GXParamBankManagerG_RegisterDebugGuiProperties (ColorGrading[Yebis])
extern "C" bool sub_21e9220(uint8_t* self) {
    sub_21e9090(self);

    uint8_t* sys_root = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540dd8ULL));
    uint8_t* param_sys = *reinterpret_cast<uint8_t**>(sys_root + 0x5ad0);
    uint32_t slot = 0;
    void* gui_mgr = *reinterpret_cast<void**>(param_sys + 0x28);

    sub_e32510(gui_mgr, &slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49cda44ULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c567aULL)));
    *reinterpret_cast<uint32_t*>(param_sys + 0x40) = slot;

    sub_e333e0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49cda44ULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c56a2ULL)), 1, 1);

    struct PropSpec {
        uint64_t vtbl_main;
        void* val_ptr;
        uint8_t type_code;
        uint8_t pad1[3];
        uint32_t step_mode;
        uint8_t flag_b;
        uint8_t pad2[7];
        uint64_t vtbl_sub;
        uint32_t min_raw;
        uint32_t max_raw;
        uint32_t step_raw;
        uint32_t mult_raw;
    };

    // 1. Enable (+0x68)
    PropSpec spec_enable{};
    spec_enable.vtbl_main = GX_VMA(0x52e8770ULL);
    spec_enable.val_ptr = self + 0x68;
    spec_enable.type_code = 0x3;
    spec_enable.step_mode = 1;
    spec_enable.flag_b = 1;
    spec_enable.vtbl_sub = GX_VMA(0x52e4a80ULL);
    spec_enable.min_raw = 0;
    spec_enable.max_raw = 1;
    spec_enable.step_raw = 0;
    spec_enable.mult_raw = 0x3f800000U;
    sub_e327d0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49cf3a4ULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c56ccULL)),
               &spec_enable, 1, 0);

    // 2. LutSourceId (+0x6c)
    PropSpec spec_lut{};
    spec_lut.vtbl_main = GX_VMA(0x52e8770ULL);
    spec_lut.val_ptr = self + 0x6c;
    spec_lut.type_code = 0x3;
    spec_lut.step_mode = 0;
    spec_lut.flag_b = 1;
    spec_lut.vtbl_sub = GX_VMA(0x52e4a80ULL);
    spec_lut.min_raw = 0xffffffffU;
    spec_lut.max_raw = 0x2710U;
    spec_lut.step_raw = 1;
    spec_lut.mult_raw = 0x3f800000U;
    sub_e327d0(*reinterpret_cast<void**>(param_sys + 0x28), slot,
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c5700ULL)),
               reinterpret_cast<const char16_t*>(GX_VMA(0x49c5718ULL)),
               &spec_lut, 1, 0);

    sub_e33600(*reinterpret_cast<void**>(param_sys + 0x28), slot, 1);
    sub_218e2b0(param_sys + 0x130, *reinterpret_cast<void**>(param_sys + 0x28), 0);
    sub_e41f00(param_sys + 0x1b8, *reinterpret_cast<void**>(param_sys + 0x28), 0, slot, 0, 0.0f);
    sub_21874c0(param_sys, slot);
    return true;
}

// 0x21e9090 (395 B): GXParamBankManagerG_ClearAllBanks
extern "C" void sub_21e9090(uint8_t* self) {
    HashNode* outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    HashNode* out_cur = outer_sentinel->next;
    while (out_cur != outer_sentinel) {
        uint8_t* inner_table = static_cast<uint8_t*>(out_cur->value);
        if (inner_table) {
            HashNode* in_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            HashNode* in_cur = in_sentinel->next;
            while (in_cur != in_sentinel) {
                uint8_t* entry = static_cast<uint8_t*>(in_cur->value);
                if (entry) {
                    DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
                    uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(entry);
                    reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl))(entry);
                    alloc->vptr->Free(alloc, entry);
                    in_cur->value = nullptr;
                }
                HashIteratorPair in_next{};
                sub_21eaed0(&in_next, static_cast<uint8_t*>(out_cur->value), in_cur);
                in_cur = in_next.third;
                inner_table = static_cast<uint8_t*>(out_cur->value);
                in_sentinel = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            }

            DLAllocator* alloc = *reinterpret_cast<DLAllocator**>(self + 0x00);
            void* in_buckets = *reinterpret_cast<void**>(inner_table + 0x28);
            if (in_buckets) {
                DLAllocator* b_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x40);
                b_alloc->vptr->Free(b_alloc, in_buckets);
                *reinterpret_cast<uint64_t*>(inner_table + 0x28) = 0;
                *reinterpret_cast<uint64_t*>(inner_table + 0x30) = 0;
                *reinterpret_cast<uint64_t*>(inner_table + 0x38) = 0;
            }
            HashNode* in_s = *reinterpret_cast<HashNode**>(inner_table + 0x08);
            HashNode* n = in_s->next;
            in_s->next = in_s;
            in_s->prev = in_s;
            *reinterpret_cast<uint64_t*>(inner_table + 0x10) = 0;
            while (n != in_s) {
                HashNode* next_n = n->next;
                DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
                n_alloc->vptr->Free(n_alloc, n);
                n = next_n;
            }
            DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(inner_table + 0x18);
            n_alloc->vptr->Free(n_alloc, in_s);
            alloc->vptr->Free(alloc, inner_table);
            out_cur->value = nullptr;
        }
        HashIteratorPair out_next{};
        sub_21eb040(&out_next, self + 0x08, out_cur);
        out_cur = out_next.third;
        outer_sentinel = *reinterpret_cast<HashNode**>(self + 0x10);
    }
}

// 0x21e8d80 (783 B): GXParamBankManagerG_DestructBody
extern "C" void sub_21e8d80(uint8_t* self) {
    sub_21e9090(self);
    sub_e71830(self + 0x5a0);

    auto destroy_fixed_wstr = [&](size_t str_base) {
        uint64_t cap = *reinterpret_cast<uint64_t*>(self + str_base + 0x20);
        if (cap >= 8) {
            if (cap + 1 >= 0x41) {
                sub_20b55b0(
                    reinterpret_cast<const char*>(GX_VMA(0x4940298ULL)),
                    0x120,
                    reinterpret_cast<const char*>(GX_VMA(0x4940300ULL)));
            }
            self[str_base + 0xa9] &= 0xfdU;
        }
        *reinterpret_cast<uint64_t*>(self + str_base + 0x20) = 7;
        *reinterpret_cast<uint64_t*>(self + str_base + 0x18) = 0;
        *reinterpret_cast<uint16_t*>(self + str_base + 0x08) = 0;
    };

    // Destroy 7 embedded fixed strings in reverse order (+0x4f0, +0x420, +0x370, +0x2c0, +0x1e0, +0x130, +0x80)
    destroy_fixed_wstr(0x4f0);
    destroy_fixed_wstr(0x420);
    destroy_fixed_wstr(0x370);
    destroy_fixed_wstr(0x2c0);
    destroy_fixed_wstr(0x1e0);
    destroy_fixed_wstr(0x130);
    destroy_fixed_wstr(0x080);

    void* outer_buckets = *reinterpret_cast<void**>(self + 0x30);
    if (outer_buckets) {
        DLAllocator* b_alloc = *reinterpret_cast<DLAllocator**>(self + 0x48);
        b_alloc->vptr->Free(b_alloc, outer_buckets);
        *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x38) = 0;
        *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    }

    HashNode* out_s = *reinterpret_cast<HashNode**>(self + 0x10);
    HashNode* cur = out_s->next;
    out_s->next = out_s;
    out_s->prev = out_s;
    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    while (cur != out_s) {
        HashNode* next_n = cur->next;
        DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(self + 0x20);
        n_alloc->vptr->Free(n_alloc, cur);
        cur = next_n;
    }
    DLAllocator* n_alloc = *reinterpret_cast<DLAllocator**>(self + 0x20);
    n_alloc->vptr->Free(n_alloc, out_s);
}

// 0x21e8960 (1043 B): GXParamBankManagerG_Construct
extern "C" void sub_21e8960(uint8_t* self, DLAllocator* alloc) {
    *reinterpret_cast<DLAllocator**>(self + 0x00) = alloc;
    uint8_t heap_info[184]{};
    alloc->vptr->GetHeapInfo(heap_info, alloc, 0);
    if ((heap_info[0] & 0x20) == 0) {
        sub_20b55b0(nullptr, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(self + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x10) = 0;
    *reinterpret_cast<DLAllocator**>(self + 0x20) = alloc;

    HashNode* sentinel = static_cast<HashNode*>(alloc->vptr->AllocateAligned(alloc, 0x20, 8));
    if (!sentinel) {
        sub_20b55b0(nullptr, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
    } else {
        sentinel->next = sentinel;
        sentinel->prev = sentinel;
    }
    *reinterpret_cast<HashNode**>(self + 0x10) = sentinel;

    *reinterpret_cast<uint64_t*>(self + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x40) = 0;
    *reinterpret_cast<DLAllocator**>(self + 0x48) = alloc;
    *reinterpret_cast<uint32_t*>(self + 0x60) = 0x3f800000U;

    HashIteratorPair dummy_pair{};
    const HashIteratorPair fill_iter{nullptr, nullptr, sentinel};
    sub_21eb9d0(&dummy_pair, self + 0x28, HashIteratorPair{nullptr, nullptr, nullptr}, 0x10, &fill_iter);
    *reinterpret_cast<uint64_t*>(self + 0x50) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x58) = 8;

    sub_29356b0(self + 0x68);
    sub_29356b0(self + 0x2a8);
    *reinterpret_cast<uint32_t*>(self + 0x4e8) = 0;

    sub_2bbe578(self + 0x518, heap_info, 0x81);
    self[0x599] = 0;
    *reinterpret_cast<uint64_t*>(self + 0x510) = 7;
    *reinterpret_cast<uint64_t*>(self + 0x508) = 0;
    *reinterpret_cast<uint16_t*>(self + 0x4f8) = 0;
    self[0x59a] = 1;
    sub_2935350(self + 0x4f0, 0x3f, 0);

    uint16_t* str_buf = reinterpret_cast<uint16_t*>(self + 0x4f8);
    if (*reinterpret_cast<uint64_t*>(self + 0x510) >= 8) {
        str_buf = *reinterpret_cast<uint16_t**>(self + 0x4f8);
    }
    *reinterpret_cast<uint64_t*>(self + 0x508) = 0;
    *str_buf = 0;

    sub_e715a0(self + 0x5a0);
    sub_e71740(self + 0x5a0);

    *reinterpret_cast<uint32_t*>(self + 0x68) = 0;
    *reinterpret_cast<uint32_t*>(self + 0x6c) = 0xffffffffU;
    *reinterpret_cast<uint32_t*>(self + 0x70) = 0xffffffffU;
    *reinterpret_cast<uint32_t*>(self + 0x74) = 0xffffffffU;
    *reinterpret_cast<uint32_t*>(self + 0x78) = 0;

    const char16_t* lut_prefix = reinterpret_cast<const char16_t*>(GX_VMA(0x49c574eULL));
    size_t lut_len = sub_2bbf228(lut_prefix);
    sub_29354e0(self + 0x80, lut_prefix, lut_len);
    sub_29354e0(self + 0x130, lut_prefix, lut_len);
    sub_29354e0(self + 0x1e0, lut_prefix, lut_len);

    *reinterpret_cast<uint64_t*>(self + 0x290) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x298) = 0;
    *reinterpret_cast<uint64_t*>(self + 0x2a0) = 0;

    // Copy primary block (+0x68) into backup block (+0x2a8)
    *reinterpret_cast<uint64_t*>(self + 0x2a8) = *reinterpret_cast<uint64_t*>(self + 0x68);
    *reinterpret_cast<uint64_t*>(self + 0x2b0) = *reinterpret_cast<uint64_t*>(self + 0x70);
    *reinterpret_cast<uint32_t*>(self + 0x2b8) = *reinterpret_cast<uint32_t*>(self + 0x78);

    self[0x36a] = self[0x12a];
    sub_2935150(self + 0x2c0, self + 0x80, 0, static_cast<size_t>(-1));

    self[0x41a] = self[0x1da];
    sub_2935150(self + 0x370, self + 0x130, 0, static_cast<size_t>(-1));

    self[0x4ca] = self[0x28a];
    sub_2935150(self + 0x420, self + 0x1e0, 0, static_cast<size_t>(-1));

    *reinterpret_cast<uint64_t*>(self + 0x4d0) = *reinterpret_cast<uint64_t*>(self + 0x290);
    *reinterpret_cast<uint64_t*>(self + 0x4d8) = *reinterpret_cast<uint64_t*>(self + 0x298);
    *reinterpret_cast<uint64_t*>(self + 0x4e0) = *reinterpret_cast<uint64_t*>(self + 0x2a0);
    *reinterpret_cast<uint32_t*>(self + 0x4e8) = 0;

    sub_29354e0(self + 0x4f0, reinterpret_cast<const char16_t*>(GX_VMA(0x49d85f0ULL)), 0);
}

// ============================================================================
// Group H Upper Half + DebugGui Enum Helper: 0x21e6f60 .. 0x21e8410 (10 functions)
// ============================================================================

// 0x21e8410 (1352 B): GXParamBankManagerH_InnerHashTable_RehashInsert
extern "C" HashIteratorPair* sub_21e8410(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    return RehashInsert_Impl(out_pair, table_vec, where_iter, new_bucket_count, fill_val);
}

// 0x21e7ec0 (1352 B): GXParamBankManagerH_OuterHashTable_RehashInsert
extern "C" HashIteratorPair* sub_21e7ec0(
    HashIteratorPair* out_pair,
    uint8_t* table_vec,
    HashIteratorPair where_iter,
    uint64_t new_bucket_count,
    const HashIteratorPair* fill_val)
{
    return RehashInsert_Impl(out_pair, table_vec, where_iter, new_bucket_count, fill_val);
}

// 0x21e7ab0 (1040 B): GXParamBankManagerH_InnerHashTable_Insert
extern "C" InsertResult* sub_21e7ab0(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashIteratorPair node_iter)
{
    return HashTable_Insert_Impl(out_res, table, key_ptr, node_iter.third, &sub_21e8410);
}

// 0x21e76a0 (1040 B): GXParamBankManagerH_OuterHashTable_Insert
extern "C" InsertResult* sub_21e76a0(
    InsertResult* out_res,
    uint8_t* table,
    uint32_t* key_ptr,
    HashIteratorPair node_iter)
{
    return HashTable_Insert_Impl(out_res, table, key_ptr, node_iter.third, &sub_21e7ec0);
}

// 0x21e72a0 (1024 B): GXDebugGui_RegisterEnumProperty
extern "C" void sub_21e72a0(
    uint8_t* gui_mgr,
    uint32_t slot,
    const char16_t* name,
    const char16_t* desc,
    void* val_ptr,
    uint32_t item_count,
    const void* items_table,
    uint32_t flag)
{
    uint64_t vec_bytes = *reinterpret_cast<uint64_t*>(gui_mgr + 0x18) - *reinterpret_cast<uint64_t*>(gui_mgr + 0x10);
    uint32_t vec_count = static_cast<uint32_t>(vec_bytes >> 4) * 0xaaaaaaabU;
    if (vec_count <= slot) return;

    size_t name_len = sub_2bbf228(name) & 0x7fffffffULL;
    size_t name_bytes = name_len * 2 + 2;
    DLAllocator* alloc1 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
    char16_t* name_buf = static_cast<char16_t*>(alloc1->vptr->AllocateRaw(alloc1, name_bytes));

    size_t desc_len = sub_2bbf228(desc) & 0x7fffffffULL;
    size_t desc_bytes = desc_len * 2 + 2;
    DLAllocator* alloc2 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
    char16_t* desc_buf = static_cast<char16_t*>(alloc2->vptr->AllocateRaw(alloc2, desc_bytes));

    if (!name_buf || !desc_buf) {
        DLAllocator* a = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
        if (name_buf) a->vptr->Free(a, name_buf);
        if (desc_buf) a->vptr->Free(a, desc_buf);
        return;
    }

    DLAllocator* alloc3 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
    uint8_t* enum_obj = static_cast<uint8_t*>(alloc3->vptr->AllocateAligned(alloc3, 0x20, 8));
    if (enum_obj) {
        DLAllocator* inner_alloc = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
        *reinterpret_cast<uint64_t*>(enum_obj + 0x00) = GX_VMA(0x52e8440ULL);
        *reinterpret_cast<uint64_t*>(enum_obj + 0x08) = 0;
        *reinterpret_cast<DLAllocator**>(enum_obj + 0x10) = inner_alloc;
        *reinterpret_cast<uint64_t*>(enum_obj + 0x18) = 0x300000000ULL;

        void* table_copy = inner_alloc->vptr->AllocateRaw(inner_alloc, static_cast<size_t>(item_count) * 16);
        *reinterpret_cast<void**>(enum_obj + 0x08) = table_copy;
        if (table_copy) {
            if (item_count != 0) {
                std::memcpy(table_copy, items_table, static_cast<size_t>(item_count) * 16);
            }
            *reinterpret_cast<uint32_t*>(enum_obj + 0x18) = item_count;
        } else {
            inner_alloc->vptr->Free(inner_alloc, enum_obj);
            enum_obj = nullptr;
        }
    }

    DLAllocator* alloc4 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
    uint8_t* val_wrapper = static_cast<uint8_t*>(alloc4->vptr->AllocateAligned(alloc4, 0x18, 8));
    if (val_wrapper) {
        *reinterpret_cast<uint64_t*>(val_wrapper + 0x00) = GX_VMA(0x52e4a50ULL);
        *reinterpret_cast<void**>(val_wrapper + 0x08) = val_ptr;
        *reinterpret_cast<uint32_t*>(val_wrapper + 0x10) = *static_cast<uint32_t*>(val_ptr);
    }

    if (!enum_obj) {
        DLAllocator* a = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
        if (val_wrapper) {
            uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(val_wrapper);
            reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl))(val_wrapper);
            a->vptr->Free(a, val_wrapper);
        }
        a->vptr->Free(a, name_buf);
        a->vptr->Free(a, desc_buf);
        return;
    }

    sub_2bbe688(name_buf, 0, name_bytes);
    sub_2bbf608(name_buf, name);
    sub_2bbe688(desc_buf, 0, desc_bytes);
    sub_2bbf608(desc_buf, desc);

    uint8_t* slot_vec = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(gui_mgr + 0x10) + static_cast<size_t>(slot) * 48);
    if (!slot_vec) {
        if (val_wrapper) {
            DLAllocator* a = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
            uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(val_wrapper);
            reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(vtbl))(val_wrapper);
            a->vptr->Free(a, val_wrapper);
        }
        DLAllocator* a1 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
        a1->vptr->Free(a1, name_buf);
        DLAllocator* a2 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
        a2->vptr->Free(a2, desc_buf);
        DLAllocator* a3 = *reinterpret_cast<DLAllocator**>(gui_mgr + 0x00);
        uintptr_t evtbl = *reinterpret_cast<uintptr_t*>(enum_obj);
        reinterpret_cast<void (*)(void*)>(*reinterpret_cast<uintptr_t*>(evtbl))(enum_obj);
        a3->vptr->Free(a3, enum_obj);
        return;
    }

    uint8_t item_rec[0x50]{};
    *reinterpret_cast<char16_t**>(item_rec + 0x00) = name_buf;
    *reinterpret_cast<char16_t**>(item_rec + 0x08) = desc_buf;
    *reinterpret_cast<void**>(item_rec + 0x10) = val_ptr;
    *reinterpret_cast<uint64_t*>(item_rec + 0x18) = 0;
    *reinterpret_cast<uint8_t**>(item_rec + 0x20) = enum_obj;
    *reinterpret_cast<uint64_t*>(item_rec + 0x28) = 0;
    *reinterpret_cast<uint8_t**>(item_rec + 0x30) = val_wrapper;
    item_rec[0x38] = 0x3;
    *reinterpret_cast<uint32_t*>(item_rec + 0x3c) = 0x15;
    *reinterpret_cast<uint32_t*>(item_rec + 0x40) = flag;
    item_rec[0x44] = 0;
    item_rec[0x45] = 1;
    *reinterpret_cast<uint32_t*>(item_rec + 0x48) = 0;

    uint8_t* vec_end = *reinterpret_cast<uint8_t**>(slot_vec + 0x10);
    if (vec_end == *reinterpret_cast<uint8_t**>(slot_vec + 0x18)) {
        sub_29065b0(slot_vec, 1);
        vec_end = *reinterpret_cast<uint8_t**>(slot_vec + 0x10);
    }
    if (vec_end) {
        std::memcpy(vec_end, item_rec, 0x50);
    }
    *reinterpret_cast<uint8_t**>(slot_vec + 0x10) += 0x50;
}

// 0x21e7130 (363 B): GXParamBankManagerH_OuterHashTable_EraseNode
extern "C" HashIteratorPair* sub_21e7130(HashIteratorPair* out_next, uint8_t* table, HashNode* node) {
    return HashTable_EraseNode_Impl(out_next, table, node);
}

// 0x21e6fc0 (363 B): GXParamBankManagerH_InnerHashTable_EraseNode
extern "C" HashIteratorPair* sub_21e6fc0(HashIteratorPair* out_next, uint8_t* table, HashNode* node) {
    return HashTable_EraseNode_Impl(out_next, table, node);
}

// 0x21e6f90 (40 B): GXParamBankManagerH_DebugCallback_Invoke
extern "C" void sub_21e6f90(uint8_t* self) {
    if ((self[0x0b] & 0x40) != 0) {
        uint8_t* target = *reinterpret_cast<uint8_t**>(self + 0x10) + *reinterpret_cast<int64_t*>(self + 0x28);
        uintptr_t fn_ptr = *reinterpret_cast<uintptr_t*>(self + 0x20);
        if ((fn_ptr & 1) != 0) {
            uintptr_t vtable = *reinterpret_cast<uintptr_t*>(target);
            fn_ptr = *reinterpret_cast<uintptr_t*>(vtable + fn_ptr - 1);
        }
        uint32_t arg = *reinterpret_cast<uint32_t*>(self + 0x18);
        reinterpret_cast<void (*)(void*, uint32_t)>(fn_ptr)(target, arg);
    }
}

// 0x21e6f80 (5 B): GXParamBankManagerH_DebugCallback_Delete
extern "C" void sub_21e6f80(void* self) {
    sub_2bbe588(self);
}

// 0x21e6f60 (22 B): GXParamBankManagerH_DebugCallback_Destroy
extern "C" void sub_21e6f60(uint8_t* self) {
    uint32_t flags = *reinterpret_cast<uint32_t*>(self + 0x08) & 0xbfffffffU;
    *reinterpret_cast<uint64_t*>(self + 0x00) = GX_VMA(0x53b93b0ULL);
    *reinterpret_cast<uint32_t*>(self + 0x08) = flags;
}
