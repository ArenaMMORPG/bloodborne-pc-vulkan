// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Module 04: SceneRenderer (Tier 8/10 CPU)
// File: GXRenderThreadAndWorkerTaskPool.cpp
// Range: 0x21b6420 .. 0x21b9f40 (51 functions, 14,304 bytes)
// Order: Strict Right-to-Left (0x21b9f40 down to 0x21b6420)
// ============================================================================
// Architectural Subsystem Overview:
//   1. GXDeferredReleaseQueue Constructor & Global Singletons (0x21b9f40 .. 0x21b9b80)
//   2. GXWorkerThreadContext & Work-Stealing Task Execution (0x21b9b50 .. 0x21b8bc0)
//      - Per-worker TLSF arena (384 KB, 64 bins, lzcnt bin indexing)
//      - Work-stealing deque (0xe47a50 / 0xe47ab0 / 0xe47b60 / 0xe47d20)
//      - Worker thread spawn ("GXWorker:%d") and main loop
//   3. GXWorkerTaskPool Lifecycle & Synchronization (0x21b8af0 .. 0x21b8050)
//   4. GXInternalTaskPool & Parallel-For Range Splitting (0x21b7f60 .. 0x21b6d20)
//      - Lock-free local ring buffer + mutex-guarded cross-worker free list
//      - Debug telemetry properties (u"InternalTaskPool", u"pool%d cur/max/free")
//   5. GXRenderThread ("GXRenderThread") & Sorted Run Merger (0x21b6be0 .. 0x21b6420)
// ============================================================================

#include <cstdint>
#include <cstring>
#include <immintrin.h>

namespace {

inline uint8_t* gx_read_fs_base() noexcept {
    uint8_t* base = reinterpret_cast<uint8_t*>(gx_guest_tls_base());
    return base;
}

using FnVoidPtr = void (*)(void*);
using FnVoidPtrU32 = void (*)(void*, uint32_t);
using FnBoolPtrPtr = bool (*)(void*, void*);
using FnIntPtr = int32_t (*)(void*);
using FnU64Void = uint64_t (*)();
using FnPtrVoid = void* (*)();
using FnPtrPtr = void* (*)(void*);
using FnVoidPtrInt = void (*)(void*, int32_t);
using FnIntPtrU32 = int32_t (*)(void*, uint32_t);

} // namespace

extern "C" {

// Forward declarations for internal calls within 04_SceneRenderer
void GXDeferredReleaseQueue_InitRingBuffer(void* self); // 0x21ba300
void GXWorkerThreadContext_ExecuteOneTaskOrSteal(void* self); // 0x21b9980
void GXWorkerThreadContext_InitializeMasterWorker(void* self, void* pool); // 0x21b9430
void GXWorkerThreadContext_InitializeAndSpawnWorker(
    void* self, void* pool, uint64_t stack_size, uint32_t worker_id, uint64_t affinity_mask); // 0x21b9110
void GXWorkerThreadContext_Dtor(void* self); // 0x21b8f50
void GXWorkStealingDeque_Dtor(void* self); // 0x21b8bc0
void GXWorkerTaskPool_WaitForIdleWorkers(void* self); // 0x21b89a0
void GXWorkerTaskPool_FlushAndExecuteAllTasks(void* self); // 0x21b8710
void GXWorkerTaskPool_WakeWorkers(void* self, void* queue, uint32_t max_workers); // 0x21b8670
void* GXInternalTaskPool_AllocateTaskNode(void* pool_slot); // 0x21b7f60
void GXInternalTaskPool_Dtor(void* self); // 0x21b6d20
void GXRenderThread_Dtor(void* self); // 0x21b6770
void GXRenderThread_Ctor(void* self, void* arg1, void* owner, void* init_params); // 0x21b6620

// ============================================================================
// [1] 0x21b9f40 (75 B) - GXDeferredReleaseQueue_CtorWithParent
// ============================================================================
void GXDeferredReleaseQueue_CtorWithParent(void* self, void* parent) {
    auto* base = static_cast<uint8_t*>(self);
    uint32_t flags = (*reinterpret_cast<uint32_t*>(base + 0x08) & 0x80000000u) | 0x40000000u;
    *reinterpret_cast<uint32_t*>(base + 0x08) = flags;
    *reinterpret_cast<uint64_t*>(base + 0x00) = GX_VMA(0x53b6c80ULL);
    *reinterpret_cast<uint16_t*>(base + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x10) = reinterpret_cast<uint64_t>(parent);
    if (parent != nullptr) {
        auto* pbase = static_cast<uint8_t*>(parent);
        uint32_t ref_cnt = *reinterpret_cast<uint32_t*>(pbase + 0x08);
        if (ref_cnt != 0) {
            *reinterpret_cast<uint32_t*>(pbase + 0x08) = ref_cnt + 1;
        }
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x21ba300ULL))(self);
    }
}

// ============================================================================
// [2] 0x21b9d60 (473 B) - GXGlobalSingleton56a6c38_EnsureInitializedA
// ============================================================================
void* GXGlobalSingleton56a6c38_EnsureInitializedA() {
    if (void* existing = *reinterpret_cast<void**>(GX_VMA(0x56a6c38ULL)); existing != nullptr) {
        return existing;
    }
    bool guarded_mtx = false;
    if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a5740ULL)) == 0) {
        if (reinterpret_cast<FnIntPtr>(GX_VMA(0x2bbe538ULL))(reinterpret_cast<void*>(GX_VMA(0x56a5740ULL))) != 0) {
            *reinterpret_cast<uint8_t*>(GX_VMA(0x56a573fULL)) = 0;
            guarded_mtx = true;
        }
    }
    if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a573fULL)) == 0) {
        uintptr_t raw_mtx = GX_VMA(0x56a5720ULL);
        uintptr_t align_off = (0ULL - (raw_mtx & 0xFFFFFFFFULL)) & 7ULL;
        auto* mtx = reinterpret_cast<uint8_t*>(raw_mtx + align_off);
        mtx[0x10] = 0;
        *reinterpret_cast<uint64_t*>(mtx) = GX_VMA(0x52b6230ULL);
        bool ok = reinterpret_cast< bool (*)(void*) >(GX_VMA(0x207ee80ULL))(mtx);
        if (!ok) {
            int32_t res = reinterpret_cast<FnIntPtr>(GX_VMA(0x2083fe0ULL))(mtx + 0x08);
            if (res == 0) {
                mtx[0x10] = 1;
            }
        }
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a573fULL)) = 1;
    }
    if (guarded_mtx) {
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x2bbe558ULL))(reinterpret_cast<void*>(GX_VMA(0x56a5740ULL)));
    }
    if (void* existing = *reinterpret_cast<void**>(GX_VMA(0x56a6c38ULL)); existing != nullptr) {
        return existing;
    }
    bool guarded_obj = false;
    if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c98ULL)) == 0) {
        if (reinterpret_cast<FnIntPtr>(GX_VMA(0x2bbe538ULL))(reinterpret_cast<void*>(GX_VMA(0x56a6c98ULL))) != 0) {
            guarded_obj = true;
        }
    }
    uintptr_t raw_obj = GX_VMA(0x56a6c40ULL);
    uintptr_t obj_off = (0ULL - (raw_obj & 0xFFFFFFFFULL)) & 7ULL;
    auto* obj = reinterpret_cast<uint8_t*>(raw_obj + obj_off);
    *reinterpret_cast<uint64_t*>(obj + 0x00) = GX_VMA(0x53b3ba0ULL);
    *reinterpret_cast<uint64_t*>(obj + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(obj + 0x08) = 0;

    auto** heap_slot = reinterpret_cast<uint8_t**>(GX_VMA(0x5540668ULL));
    uint8_t* heap = *heap_slot;
    if (heap == nullptr) {
        heap = static_cast<uint8_t*>(reinterpret_cast<FnPtrVoid>(GX_VMA(0x207bbf0ULL))()) + 0x458;
        *heap_slot = heap;
    }
    uint64_t heap_caps = 0;
    auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(heap);
    heap_vtbl[4](&heap_caps, heap, 0);
    if ((heap_caps & 0x20ULL) == 0) {
        reinterpret_cast<void (*)(uint32_t, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    uintptr_t obj_off2 = (0ULL - (raw_obj & 0xFFFFFFFFULL)) & 7ULL;
    auto* obj2 = reinterpret_cast<uint8_t*>(raw_obj + obj_off2);
    *reinterpret_cast<uint64_t*>(obj2 + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(obj2 + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(obj2 + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(obj2 + 0x38) = reinterpret_cast<uint64_t>(heap);
    *reinterpret_cast<uint64_t*>(obj2 + 0x00) = GX_VMA(0x53b6d00ULL);
    *reinterpret_cast<uint64_t*>(obj2 + 0x48) = 0;
    *reinterpret_cast<uint64_t*>(obj2 + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6c38ULL)) = reinterpret_cast<uint64_t>(obj2);
    if (guarded_obj) {
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x2bbe558ULL))(reinterpret_cast<void*>(GX_VMA(0x56a6c98ULL)));
    }
    return obj2;
}

// ============================================================================
// [3] 0x21b9b80 (473 B) - GXGlobalSingleton56a6c38_EnsureInitializedB
// ============================================================================
void* GXGlobalSingleton56a6c38_EnsureInitializedB() {
    return GXGlobalSingleton56a6c38_EnsureInitializedA();
}

// ============================================================================
// [4] 0x21b9b50 (46 B) - GXWorkerThreadSyncPrim_DeletingDtor
// ============================================================================
void GXWorkerThreadSyncPrim_DeletingDtor(void* self) {
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53b6c40ULL);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a5d0ULL))(self);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a580ULL))(self);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// [5] 0x21b9b20 (38 B) - GXWorkerThreadSyncPrim_Dtor
// ============================================================================
void GXWorkerThreadSyncPrim_Dtor(void* self) {
    *reinterpret_cast<uint64_t*>(self) = GX_VMA(0x53b6c40ULL);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a5d0ULL))(self);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a580ULL))(self);
}

// ============================================================================
// [6] 0x21b9a80 (13 B) - GXWorkerThreadContext_IsBusy
// ============================================================================
bool GXWorkerThreadContext_IsBusy(const void* self) {
    const auto* base = static_cast<const uint8_t*>(self);
    return *reinterpret_cast<const uint32_t*>(base + 0x528) != 2u;
}

// ============================================================================
// [7] 0x21b9980 (242 B) - GXWorkerThreadContext_ExecuteOneTaskOrSteal
// ============================================================================
void GXWorkerThreadContext_ExecuteOneTaskOrSteal(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    void* task = nullptr;
    uint8_t* own_deque = base + 0x18;

    bool popped = reinterpret_cast<FnBoolPtrPtr>(GX_VMA(0xe47b60ULL))(own_deque, &task);
    if (!popped) {
        uint64_t tsc = reinterpret_cast<FnU64Void>(GX_VMA(0x2bc0de8ULL))();
        uint64_t seed = tsc + *reinterpret_cast<uint32_t*>(base + 0xa0);
        auto* pool = *reinterpret_cast<uint8_t**>(base + 0x08);
        uint32_t worker_count = *reinterpret_cast<uint32_t*>(pool + 0x08);
        if (worker_count != 0) {
            uint32_t start_offset = static_cast<uint32_t>(seed % worker_count);
            for (uint32_t i = 0; i < worker_count; ++i) {
                uint32_t target_idx = (start_offset + i) % worker_count;
                auto* workers_base = *reinterpret_cast<uint8_t**>(pool + 0x00);
                uint8_t* target_worker = workers_base + static_cast<size_t>(target_idx) * 0x530ULL;
                if (target_worker == base) {
                    continue;
                }
                bool stolen = reinterpret_cast<FnBoolPtrPtr>(GX_VMA(0xe47d20ULL))(target_worker + 0x18, own_deque);
                if (stolen || *reinterpret_cast<uint64_t*>(base + 0x38) != 0) {
                    reinterpret_cast<FnBoolPtrPtr>(GX_VMA(0xe47b60ULL))(own_deque, &task);
                    break;
                }
                pool = *reinterpret_cast<uint8_t**>(base + 0x08);
                worker_count = *reinterpret_cast<uint32_t*>(pool + 0x08);
            }
        }
    }

    if (task != nullptr) {
        auto* vtbl = *reinterpret_cast<FnVoidPtrU32**>(task);
        vtbl[0](task, *reinterpret_cast<uint32_t*>(base + 0xa0));
    }
}

// ============================================================================
// [8] 0x21b9760 (534 B) - GXWorkerThreadContext_ThreadMainLoop
// ============================================================================
void GXWorkerThreadContext_ThreadMainLoop(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    auto* pool = *reinterpret_cast<uint8_t**>(base + 0x08);
    if (pool == nullptr) {
        return;
    }

    void* listener = *reinterpret_cast<void**>(pool + 0x38);
    if (listener != nullptr) {
        auto* l_vtbl = *reinterpret_cast<FnVoidPtrU32**>(listener);
        l_vtbl[1](listener, *reinterpret_cast<uint32_t*>(base + 0xa0));
    }

    uint8_t* fs_base = gx_read_fs_base();
    uint64_t tls_off = static_cast<uint64_t>(gx_guest_tls_offset(0x53e4658ULL));
    *reinterpret_cast<void**>(fs_base + tls_off) = self;

    *reinterpret_cast<uint32_t*>(base + 0x528) = 2u;
    pool = *reinterpret_cast<uint8_t**>(base + 0x08);
    void* idle_sem = *reinterpret_cast<void**>(pool + 0x28);
    if (idle_sem != nullptr) {
        reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(idle_sem, 1);
        pool = *reinterpret_cast<uint8_t**>(base + 0x08);
    }

    __atomic_fetch_add(reinterpret_cast<uint32_t*>(pool + 0x18), 1u, __ATOMIC_SEQ_CST);
    pool = *reinterpret_cast<uint8_t**>(base + 0x08);
    void* wake_sem = *reinterpret_cast<void**>(pool + 0x20);
    {
        auto* w_vtbl = *reinterpret_cast<FnVoidPtrU32**>(wake_sem);
        w_vtbl[3](wake_sem, 0xFFFFFFFFu);
    }

    *reinterpret_cast<uint32_t*>(base + 0x528) = 3u;
    pool = *reinterpret_cast<uint8_t**>(base + 0x08);

    if (*reinterpret_cast<uint8_t*>(pool + 0x30) == 0) {
        uint8_t* own_deque = base + 0x18;
        void* task = nullptr;

        for (;;) {
            while (reinterpret_cast<FnBoolPtrPtr>(GX_VMA(0xe47b60ULL))(own_deque, &task)) {
                auto* t_vtbl = *reinterpret_cast<FnVoidPtrU32**>(task);
                t_vtbl[0](task, *reinterpret_cast<uint32_t*>(base + 0xa0));
            }

            uint64_t tsc = reinterpret_cast<FnU64Void>(GX_VMA(0x2bc0de8ULL))();
            uint64_t seed = tsc + *reinterpret_cast<uint32_t*>(base + 0xa0);
            pool = *reinterpret_cast<uint8_t**>(base + 0x08);
            uint32_t worker_count = *reinterpret_cast<uint32_t*>(pool + 0x08);
            bool stole_work = false;

            if (worker_count != 0) {
                uint32_t start_offset = static_cast<uint32_t>(seed % worker_count);
                for (uint32_t i = 0; i < worker_count; ++i) {
                    uint32_t target_idx = (start_offset + i) % worker_count;
                    auto* workers_base = *reinterpret_cast<uint8_t**>(pool + 0x00);
                    uint8_t* target_worker = workers_base + static_cast<size_t>(target_idx) * 0x530ULL;
                    if (target_worker != base) {
                        bool stolen = reinterpret_cast<FnBoolPtrPtr>(GX_VMA(0xe47d20ULL))(target_worker + 0x18, own_deque);
                        if (stolen || *reinterpret_cast<uint64_t*>(base + 0x38) != 0) {
                            stole_work = true;
                            break;
                        }
                        pool = *reinterpret_cast<uint8_t**>(base + 0x08);
                    }
                    worker_count = *reinterpret_cast<uint32_t*>(pool + 0x08);
                }
            }

            if (stole_work) {
                continue;
            }

            *reinterpret_cast<uint32_t*>(base + 0x528) = 2u;
            pool = *reinterpret_cast<uint8_t**>(base + 0x08);
            idle_sem = *reinterpret_cast<void**>(pool + 0x28);
            if (idle_sem != nullptr) {
                reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(idle_sem, 1);
                pool = *reinterpret_cast<uint8_t**>(base + 0x08);
            }

            __atomic_fetch_add(reinterpret_cast<uint32_t*>(pool + 0x18), 1u, __ATOMIC_SEQ_CST);
            pool = *reinterpret_cast<uint8_t**>(base + 0x08);
            wake_sem = *reinterpret_cast<void**>(pool + 0x20);
            {
                auto* w_vtbl = *reinterpret_cast<FnVoidPtrU32**>(wake_sem);
                w_vtbl[3](wake_sem, 0xFFFFFFFFu);
            }

            *reinterpret_cast<uint32_t*>(base + 0x528) = 3u;
            pool = *reinterpret_cast<uint8_t**>(base + 0x08);
            if (*reinterpret_cast<uint8_t*>(pool + 0x30) != 0) {
                break;
            }
        }
    }

    listener = *reinterpret_cast<void**>(pool + 0x38);
    if (listener != nullptr) {
        auto* l_vtbl = *reinterpret_cast<FnVoidPtrU32**>(listener);
        l_vtbl[2](listener, *reinterpret_cast<uint32_t*>(base + 0xa0));
    }
}

// ============================================================================
// [9] 0x21b9720 (54 B) - GXWorkerThreadContext_PushTaskOrEnqueue
// ============================================================================
void GXWorkerThreadContext_PushTaskOrEnqueue(void* self, void* task) {
    auto* base = static_cast<uint8_t*>(self);
    auto* task_vtbl = *reinterpret_cast<bool (***)(void*, void*)>(task);
    if (task_vtbl[3](task, *reinterpret_cast<void**>(base + 0x08))) {
        return;
    }
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(base + 0x18, task);
}

// ============================================================================
// [10] 0x21b9710 (9 B) - GXWorkerThreadContext_PopTask
// ============================================================================
bool GXWorkerThreadContext_PopTask(void* self, void* out_task) {
    auto* base = static_cast<uint8_t*>(self);
    return reinterpret_cast<FnBoolPtrPtr>(GX_VMA(0xe47b60ULL))(base + 0x18, out_task);
}

// ============================================================================
// [11] 0x21b9700 (9 B) - GXWorkerThreadContext_EnqueueTask
// ============================================================================
void GXWorkerThreadContext_EnqueueTask(void* self, void* task) {
    auto* base = static_cast<uint8_t*>(self);
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(base + 0x18, task);
}

// ============================================================================
// [12] 0x21b96f0 (9 B) - GXWorkerThreadContext_SignalSyncPrim
// ============================================================================
void GXWorkerThreadContext_SignalSyncPrim(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a670ULL))(base + 0x80);
}

// ============================================================================
// [13] 0x21b9430 (704 B) - GXWorkerThreadContext_InitializeMasterWorker
// ============================================================================
void GXWorkerThreadContext_InitializeMasterWorker(void* self, void* pool) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<void**>(base + 0x08) = pool;
    *reinterpret_cast<uint32_t*>(base + 0xa0) = 0;

    uint8_t* fs_base = gx_read_fs_base();
    uint64_t tls_heap_off = static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL));
    auto* tls_ctx = *reinterpret_cast<uint8_t**>(fs_base + tls_heap_off);
    if (tls_ctx == nullptr) {
        reinterpret_cast< void (*)() >(GX_VMA(0x207d250ULL))();
        fs_base = gx_read_fs_base();
        tls_ctx = *reinterpret_cast<uint8_t**>(fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    }

    uint8_t* str_heap = tls_ctx + 0x28;
    uint64_t heap_caps = 0;
    {
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(str_heap);
        h_vtbl[4](&heap_caps, str_heap, 0);
    }
    if ((heap_caps & 0x20ULL) == 0) {
        reinterpret_cast<void (*)(uint32_t, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    struct LocalString {
        uint64_t vtable_pad;
        char     buf[16];
        uint64_t length;
        uint64_t capacity;
        uint8_t* allocator;
        uint8_t  flag;
    } worker_name{};
    worker_name.allocator = str_heap;
    worker_name.capacity = 0x0f;
    worker_name.length = 0;
    worker_name.buf[0] = '\0';
    worker_name.flag = 0;

    reinterpret_cast<void (*)(void*, const char*, uint32_t)>(GX_VMA(0x27fa660ULL))(
        &worker_name,
        reinterpret_cast<const char*>(GX_VMA(0x494023bULL)), // "GXWorker:%d"
        *reinterpret_cast<uint32_t*>(base + 0xa0));

    *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
    uint64_t self_pthread = reinterpret_cast<FnU64Void>(GX_VMA(0x2bbfe78ULL))();
    *reinterpret_cast<uint64_t*>(base + 0xa8) = self_pthread;

    auto* sys_heap = *reinterpret_cast<uint8_t**>(GX_VMA(0x5540680ULL));
    if (sys_heap != nullptr) {
        auto* s_vtbl = *reinterpret_cast<void* (***)(void*, size_t)>(sys_heap);
        auto* arena_mem = static_cast<uint8_t*>(s_vtbl[10](sys_heap, 0x60000ULL));
        *reinterpret_cast<uint8_t**>(base + 0xc0) = arena_mem;
        if (arena_mem != nullptr) {
            *reinterpret_cast<uint8_t**>(base + 0xb0) = sys_heap;
            uint8_t* bitmap_ptr = base + 0xd8;
            uintptr_t aligned_start = (reinterpret_cast<uintptr_t>(arena_mem) + 0x17ULL) & ~0xFULL;
            uintptr_t aligned_end = (reinterpret_cast<uintptr_t>(arena_mem) + 0x5fff8ULL) & ~0xFULL;
            if ((aligned_end - aligned_start) < 0x40ULL) {
                std::memset(bitmap_ptr, 0, 0x440);
                auto* free_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
                free_vtbl[14](sys_heap, arena_mem);
                *reinterpret_cast<uint64_t*>(base + 0xc0) = 0;
            } else {
                uint8_t* r8_ptr = base + 0xb0;
                *reinterpret_cast<uintptr_t*>(base + 0xe0) = aligned_start;
                *reinterpret_cast<uintptr_t*>(base + 0xe8) = aligned_end;

                uint8_t* bins_base = base + 0x100;
                for (size_t off = 0; off < 0x400; off += 0x10) {
                    uint8_t* self_bin = bins_base + off - 0x10;
                    *reinterpret_cast<uint8_t**>(bins_base + off + 0x08) = self_bin;
                    *reinterpret_cast<uint8_t**>(bins_base + off + 0x00) = self_bin;
                }

                auto* first_blk = reinterpret_cast<uint8_t*>(aligned_start - 0x10ULL);
                auto* last_blk = reinterpret_cast<uint8_t*>(aligned_end - 0x10ULL);
                *reinterpret_cast<uintptr_t*>(aligned_start - 0x08ULL) =
                    reinterpret_cast<uintptr_t>(last_blk) | 2ULL;
                *reinterpret_cast<uint8_t**>(aligned_end - 0x10ULL) = first_blk;
                *reinterpret_cast<uintptr_t*>(aligned_end - 0x08ULL) =
                    reinterpret_cast<uintptr_t>(first_blk) | 1ULL;

                *reinterpret_cast<uint8_t**>(base + 0x500) = first_blk;
                *reinterpret_cast<uint8_t**>(base + 0x508) = last_blk;
                uintptr_t usable_bytes = static_cast<uintptr_t>(last_blk - first_blk);
                *reinterpret_cast<uintptr_t*>(base + 0xf0) = usable_bytes;
                *reinterpret_cast<uintptr_t*>(base + 0x510) = usable_bytes;
                *reinterpret_cast<uint64_t*>(base + 0xf8) = 0;
                *reinterpret_cast<uint64_t*>(base + 0xd8) = 0;

                uint32_t lz = static_cast<uint32_t>(__builtin_ia32_lzcnt_u64(usable_bytes));
                uint32_t bin_idx = 0x3fu - lz;
                size_t bin_off = static_cast<size_t>(bin_idx) << 4;
                uint8_t* bin_sentinel = base + bin_off + 0xf0;
                auto* prev_head = *reinterpret_cast<uint8_t**>(base + bin_off + 0x108);
                *reinterpret_cast<uint8_t**>(base + bin_off + 0x108) = first_blk;
                *reinterpret_cast<uint8_t**>(aligned_start + 0x00) = bin_sentinel;
                *reinterpret_cast<uint8_t**>(aligned_start + 0x08) = prev_head;
                *reinterpret_cast<uint8_t**>(prev_head + 0x10) = first_blk;
                if (prev_head == bin_sentinel) {
                    *reinterpret_cast<uint64_t*>(bitmap_ptr) |= (1ULL << bin_idx);
                }

                *reinterpret_cast<uint32_t*>(base + 0x520) = 0;
                *reinterpret_cast<uint8_t**>(base + 0xd0) = r8_ptr;
                void* cur_arena = *reinterpret_cast<void**>(base + 0xc0);
                void* mgr = reinterpret_cast<FnPtrVoid>(GX_VMA(0x207bbf0ULL))();
                reinterpret_cast<void (*)(void*, void*, void*, uint32_t)>(GX_VMA(0x207c000ULL))(
                    mgr, base + 0xc8, cur_arena, 0x60000u);
            }
        }
    }

    *reinterpret_cast<uint32_t*>(base + 0x48) = *reinterpret_cast<uint32_t*>(base + 0xa0);
    fs_base = gx_read_fs_base();
    *reinterpret_cast<void**>(fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4658ULL))) = self;

    if (worker_name.capacity >= 0x10ULL) {
        void* heap_obj = worker_name.allocator;
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(heap_obj);
        h_vtbl[14](heap_obj, *reinterpret_cast<void**>(worker_name.buf));
    }
}

// ============================================================================
// [14] 0x21b9110 (789 B) - GXWorkerThreadContext_InitializeAndSpawnWorker
// ============================================================================
void GXWorkerThreadContext_InitializeAndSpawnWorker(
    void* self, void* pool, uint64_t stack_size, uint32_t worker_id, uint64_t affinity_mask) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<void**>(base + 0x08) = pool;
    *reinterpret_cast<uint32_t*>(base + 0xa0) = worker_id;

    uint8_t* fs_base = gx_read_fs_base();
    auto* tls_ctx = *reinterpret_cast<uint8_t**>(fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    if (tls_ctx == nullptr) {
        reinterpret_cast< void (*)() >(GX_VMA(0x207d250ULL))();
        fs_base = gx_read_fs_base();
        tls_ctx = *reinterpret_cast<uint8_t**>(fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4568ULL)));
    }

    uint8_t* str_heap = tls_ctx + 0x28;
    uint64_t heap_caps = 0;
    {
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(str_heap);
        h_vtbl[4](&heap_caps, str_heap, 0);
    }
    if ((heap_caps & 0x20ULL) == 0) {
        reinterpret_cast<void (*)(uint32_t, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    struct LocalString {
        uint64_t vtable_pad;
        char     buf[16];
        uint64_t length;
        uint64_t capacity;
        uint8_t* allocator;
        uint8_t  flag;
    } worker_name{};
    worker_name.allocator = str_heap;
    worker_name.capacity = 0x0f;
    worker_name.length = 0;
    worker_name.buf[0] = '\0';
    worker_name.flag = 0;

    reinterpret_cast<void (*)(void*, const char*, uint32_t)>(GX_VMA(0x27fa660ULL))(
        &worker_name,
        reinterpret_cast<const char*>(GX_VMA(0x494023bULL)), // "GXWorker:%d"
        worker_id);

    auto** sys_heap_slot = reinterpret_cast<uint8_t**>(GX_VMA(0x5540680ULL));
    uint8_t* sys_heap = *sys_heap_slot;
    void* thread_obj = nullptr;
    {
        auto* s_vtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap);
        void* raw_thr = s_vtbl[11](sys_heap, 0xc8ULL, 0x08ULL);
        if (raw_thr != nullptr) {
            const char* c_name = (worker_name.capacity >= 0x10ULL)
                ? *reinterpret_cast<const char**>(worker_name.buf)
                : worker_name.buf;
            reinterpret_cast<void (*)(void*, void*, uint64_t, uint64_t, const char*, uint32_t)>(GX_VMA(0x208afe0ULL))(
                raw_thr, self, stack_size, affinity_mask, c_name, 0);
            thread_obj = raw_thr;
        }
    }

    *reinterpret_cast<void**>(base + 0x10) = thread_obj;
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x208b9e0ULL))(thread_obj);
    auto* thr_bytes = *reinterpret_cast<uint8_t**>(base + 0x10);
    *reinterpret_cast<uint64_t*>(base + 0xa8) = *reinterpret_cast<uint64_t*>(thr_bytes + 0x18);

    sys_heap = *sys_heap_slot;
    if (sys_heap != nullptr) {
        auto* s_vtbl = *reinterpret_cast<void* (***)(void*, size_t)>(sys_heap);
        auto* arena_mem = static_cast<uint8_t*>(s_vtbl[10](sys_heap, 0x60000ULL));
        *reinterpret_cast<uint8_t**>(base + 0xc0) = arena_mem;
        if (arena_mem != nullptr) {
            *reinterpret_cast<uint8_t**>(base + 0xb0) = sys_heap;
            uint8_t* bitmap_ptr = base + 0xd8;
            uintptr_t aligned_start = (reinterpret_cast<uintptr_t>(arena_mem) + 0x17ULL) & ~0xFULL;
            uintptr_t aligned_end = (reinterpret_cast<uintptr_t>(arena_mem) + 0x5fff8ULL) & ~0xFULL;
            if ((aligned_end - aligned_start) < 0x40ULL) {
                std::memset(bitmap_ptr, 0, 0x440);
                auto* free_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
                free_vtbl[14](sys_heap, arena_mem);
                *reinterpret_cast<uint64_t*>(base + 0xc0) = 0;
            } else {
                uint8_t* r8_ptr = base + 0xb0;
                *reinterpret_cast<uintptr_t*>(base + 0xe0) = aligned_start;
                *reinterpret_cast<uintptr_t*>(base + 0xe8) = aligned_end;

                uint8_t* bins_base = base + 0x100;
                for (size_t off = 0; off < 0x400; off += 0x10) {
                    uint8_t* self_bin = bins_base + off - 0x10;
                    *reinterpret_cast<uint8_t**>(bins_base + off + 0x08) = self_bin;
                    *reinterpret_cast<uint8_t**>(bins_base + off + 0x00) = self_bin;
                }

                auto* first_blk = reinterpret_cast<uint8_t*>(aligned_start - 0x10ULL);
                auto* last_blk = reinterpret_cast<uint8_t*>(aligned_end - 0x10ULL);
                *reinterpret_cast<uintptr_t*>(aligned_start - 0x08ULL) =
                    reinterpret_cast<uintptr_t>(last_blk) | 2ULL;
                *reinterpret_cast<uint8_t**>(aligned_end - 0x10ULL) = first_blk;
                *reinterpret_cast<uintptr_t*>(aligned_end - 0x08ULL) =
                    reinterpret_cast<uintptr_t>(first_blk) | 1ULL;

                *reinterpret_cast<uint8_t**>(base + 0x500) = first_blk;
                *reinterpret_cast<uint8_t**>(base + 0x508) = last_blk;
                uintptr_t usable_bytes = static_cast<uintptr_t>(last_blk - first_blk);
                *reinterpret_cast<uintptr_t*>(base + 0xf0) = usable_bytes;
                *reinterpret_cast<uintptr_t*>(base + 0x510) = usable_bytes;
                *reinterpret_cast<uint64_t*>(base + 0xf8) = 0;
                *reinterpret_cast<uint64_t*>(base + 0xd8) = 0;

                uint32_t lz = static_cast<uint32_t>(__builtin_ia32_lzcnt_u64(usable_bytes));
                uint32_t bin_idx = 0x3fu - lz;
                size_t bin_off = static_cast<size_t>(bin_idx) << 4;
                uint8_t* bin_sentinel = base + bin_off + 0xf0;
                auto* prev_head = *reinterpret_cast<uint8_t**>(base + bin_off + 0x108);
                *reinterpret_cast<uint8_t**>(base + bin_off + 0x108) = first_blk;
                *reinterpret_cast<uint8_t**>(aligned_start + 0x00) = bin_sentinel;
                *reinterpret_cast<uint8_t**>(aligned_start + 0x08) = prev_head;
                *reinterpret_cast<uint8_t**>(prev_head + 0x10) = first_blk;
                if (prev_head == bin_sentinel) {
                    *reinterpret_cast<uint64_t*>(bitmap_ptr) |= (1ULL << bin_idx);
                }

                *reinterpret_cast<uint32_t*>(base + 0x520) = 0;
                *reinterpret_cast<uint8_t**>(base + 0xd0) = r8_ptr;
                void* cur_arena = *reinterpret_cast<void**>(base + 0xc0);
                void* mgr = reinterpret_cast<FnPtrVoid>(GX_VMA(0x207bbf0ULL))();
                reinterpret_cast<void (*)(void*, void*, void*, uint32_t)>(GX_VMA(0x207c000ULL))(
                    mgr, base + 0xc8, cur_arena, 0x60000u);
            }
        }
    }

    *reinterpret_cast<uint32_t*>(base + 0x48) = *reinterpret_cast<uint32_t*>(base + 0xa0);
    if (worker_name.capacity >= 0x10ULL) {
        void* heap_obj = worker_name.allocator;
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(heap_obj);
        h_vtbl[14](heap_obj, *reinterpret_cast<void**>(worker_name.buf));
    }
}

// ============================================================================
// [15] 0x21b90a0 (110 B) - GXWorkerThreadContext_JoinAndDestroyThread
// ============================================================================
void GXWorkerThreadContext_JoinAndDestroyThread(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    auto* thr = *reinterpret_cast<uint8_t**>(base + 0x10);
    if (thr != nullptr) {
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a670ULL))(base + 0x80);
        thr = *reinterpret_cast<uint8_t**>(base + 0x10);
        reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b6a0ULL))(thr + 0x08, 0xFFFFFFFFu);
        thr = *reinterpret_cast<uint8_t**>(base + 0x10);
        if (thr != nullptr) {
            void* owner_heap = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(thr);
            auto* thr_vtbl = *reinterpret_cast<FnVoidPtr**>(thr);
            thr_vtbl[1](thr);
            auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
            heap_vtbl[14](owner_heap, thr);
            *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
        }
    }
}

// ============================================================================
// [16] 0x21b8f50 (330 B) - GXWorkerThreadContext_Dtor
// ============================================================================
void GXWorkerThreadContext_Dtor(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(base + 0x00) = GX_VMA(0x535fc20ULL);

    if (*reinterpret_cast<uint64_t*>(base + 0x10) != 0) {
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a670ULL))(base + 0x80);
        auto* thr = *reinterpret_cast<uint8_t**>(base + 0x10);
        reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b6a0ULL))(thr + 0x08, 0xFFFFFFFFu);
        thr = *reinterpret_cast<uint8_t**>(base + 0x10);
        if (thr != nullptr) {
            void* owner_heap = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(thr);
            auto* thr_vtbl = *reinterpret_cast<FnVoidPtr**>(thr);
            thr_vtbl[1](thr);
            auto* heap_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
            heap_vtbl[14](owner_heap, thr);
            *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
        }
    }

    if (*reinterpret_cast<uint64_t*>(base + 0xe0) != 0) {
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x207b8c0ULL))(base + 0xc8);
        uint8_t* bins_base = base + 0x100;
        for (size_t off = 0; off < 0x400; off += 0x10) {
            *reinterpret_cast<uint64_t*>(bins_base + off + 0x08) = 0;
            *reinterpret_cast<uint64_t*>(bins_base + off + 0x00) = 0;
        }
        *reinterpret_cast<uint64_t*>(base + 0xe8) = 0;
        *reinterpret_cast<uint64_t*>(base + 0xe0) = 0;

        void* arena_heap = *reinterpret_cast<void**>(base + 0xb0);
        if (arena_heap != nullptr) {
            auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(arena_heap);
            h_vtbl[14](arena_heap, *reinterpret_cast<void**>(base + 0xc0));
            *reinterpret_cast<uint64_t*>(base + 0xb0) = 0;
            *reinterpret_cast<uint64_t*>(base + 0xc0) = 0;
        }
    }

    uint8_t* sync_prim = base + 0x80;
    *reinterpret_cast<uint64_t*>(sync_prim) = GX_VMA(0x53b6c40ULL);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a5d0ULL))(sync_prim);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a580ULL))(sync_prim);

    GXWorkStealingDeque_Dtor(base + 0x18);
}

// ============================================================================
// [17] 0x21b8f30 (28 B) - GXWorkerThreadContext_DeletingDtor
// ============================================================================
void GXWorkerThreadContext_DeletingDtor(void* self) {
    GXWorkerThreadContext_Dtor(self);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// [18] 0x21b8e40 (226 B) - GXWorkerThreadContext_Ctor
// ============================================================================
void GXWorkerThreadContext_Ctor(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(base + 0x00) = GX_VMA(0x535fc20ULL);
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x08) = 0;
    reinterpret_cast<FnVoidPtr>(GX_VMA(0xe47a50ULL))(base + 0x18);

    uint8_t* sync_prim = base + 0x80;
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a510ULL))(sync_prim);
    *reinterpret_cast<uint64_t*>(sync_prim) = GX_VMA(0x53b6c40ULL);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a610ULL))(sync_prim);

    *reinterpret_cast<uint32_t*>(base + 0xa0) = 0;
    *reinterpret_cast<uint64_t*>(base + 0xa8) = 0;
    *reinterpret_cast<uint64_t*>(base + 0xc8) = GX_VMA(0x53b5110ULL);
    std::memset(base + 0xd0, 0, 0x448);
    *reinterpret_cast<uint64_t*>(base + 0x518) = GX_VMA(0x52b5dd0ULL);
    *reinterpret_cast<uint64_t*>(base + 0xb0) = 0;
    *reinterpret_cast<uint32_t*>(base + 0xb8) =
        reinterpret_cast< uint32_t (*)() >(GX_VMA(0x207b9f0ULL))();
    *reinterpret_cast<uint64_t*>(base + 0xc0) = 0;
    *reinterpret_cast<uint32_t*>(base + 0x520) = 0;
    *reinterpret_cast<void**>(base + 0xd0) = base + 0xb0;
    *reinterpret_cast<uint32_t*>(base + 0x528) = 0;
}

// ============================================================================
// [19] 0x21b8e20 (22 B) - GXWorkerThreadContext_GetCurrentThreadContext
// ============================================================================
void* GXWorkerThreadContext_GetCurrentThreadContext() {
    uint8_t* fs_base = gx_read_fs_base();
    uint64_t tls_off = static_cast<uint64_t>(gx_guest_tls_offset(0x53e4658ULL));
    return *reinterpret_cast<void**>(fs_base + tls_off);
}

// ============================================================================
// [20] 0x21b8bc0 (594 B) - GXWorkStealingDeque_Dtor
// ============================================================================
void GXWorkStealingDeque_Dtor(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    auto destroy_mtx = [](uint8_t* mtx_obj) {
        *reinterpret_cast<uint64_t*>(mtx_obj + 0x00) = GX_VMA(0x52b6230ULL);
        if (reinterpret_cast< bool (*)(void*) >(GX_VMA(0x207ee80ULL))(mtx_obj) || mtx_obj[0x10] != 0) {
            uint32_t err = static_cast<uint32_t>(
                reinterpret_cast<FnIntPtr>(GX_VMA(0x2bbfee8ULL))(mtx_obj + 0x08));
            if (err != 0x80020010u) {
                if (err != 0) {
                    if (err == 0x80020016u) {
                        reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                            reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)),
                            0x55,
                            reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                    }
                    reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)),
                        0x57,
                        reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
                }
                mtx_obj[0x10] = 0;
            }
        }
        *reinterpret_cast<uint64_t*>(mtx_obj + 0x00) = GX_VMA(0x53b2fa0ULL);
    };

    destroy_mtx(base + 0x50);
    destroy_mtx(base + 0x38);

    void* saved_alloc = *reinterpret_cast<void**>(base + 0x28);
    if (*reinterpret_cast<uint64_t*>(base + 0x20) != 0) {
        *reinterpret_cast<uint64_t*>(base + 0x20) = 0;
        *reinterpret_cast<uint64_t*>(base + 0x18) = 0;
    }

    uint64_t seg_count = *reinterpret_cast<uint64_t*>(base + 0x10);
    while (seg_count != 0) {
        auto* seg_array = *reinterpret_cast<void***>(base + 0x08);
        void* seg_ptr = seg_array[seg_count - 1];
        --seg_count;
        if (seg_ptr != nullptr) {
            void* cur_alloc = *reinterpret_cast<void**>(base + 0x28);
            auto* a_vtbl = *reinterpret_cast<void (***)(void*, void*)>(cur_alloc);
            a_vtbl[14](cur_alloc, seg_ptr);
        }
    }

    void* seg_array_buf = *reinterpret_cast<void**>(base + 0x08);
    if (seg_array_buf != nullptr) {
        auto* a_vtbl = *reinterpret_cast<void (***)(void*, void*)>(saved_alloc);
        a_vtbl[14](saved_alloc, seg_array_buf);
    }
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x08) = 0;

    void* main_buf = *reinterpret_cast<void**>(base + 0x00);
    void* cur_alloc = *reinterpret_cast<void**>(base + 0x28);
    {
        auto* a_vtbl = *reinterpret_cast<void (***)(void*, void*)>(cur_alloc);
        a_vtbl[14](cur_alloc, main_buf);
    }
    *reinterpret_cast<uint64_t*>(base + 0x00) = 0;
}

// ============================================================================
// [21] 0x21b8af0 (63 B) - GXWorkerTaskPool_SubmitTaskToWorker
// ============================================================================
void GXWorkerTaskPool_SubmitTaskToWorker(void* self, void* task, uint32_t worker_id) {
    auto* base = static_cast<uint8_t*>(self);
    auto* workers_base = *reinterpret_cast<uint8_t**>(base + 0x00);
    uint8_t* target_deque = workers_base + static_cast<size_t>(worker_id) * 0x530ULL + 0x18ULL;
    reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(target_deque, task);

    if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0 && worker_id == 0) {
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 1;
    }
    *reinterpret_cast<uint8_t*>(base + 0x31) = 1;
}

// ============================================================================
// [22] 0x21b89a0 (332 B) - GXWorkerTaskPool_WaitForIdleWorkers
// ============================================================================
void GXWorkerTaskPool_WaitForIdleWorkers(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    if (*reinterpret_cast<uint8_t*>(base + 0x68) == 0) {
        return;
    }

    uint32_t worker_count = *reinterpret_cast<uint32_t*>(base + 0x08);
    auto* idle_counter = reinterpret_cast<volatile int32_t*>(base + 0x18);

    if (worker_count != 0) {
        int32_t target_idle = static_cast<int32_t>(worker_count - 1);
        while (target_idle > *idle_counter) {
            if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) != 0) {
                *reinterpret_cast<uint64_t*>(base + 0x10) = *reinterpret_cast<uint64_t*>(base + 0x70);
                uint32_t cur_workers = *reinterpret_cast<uint32_t*>(base + 0x08);
                uint32_t wake_cnt = cur_workers - 1;
                if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
                    uint32_t cur_idle = static_cast<uint32_t>(*idle_counter);
                    if (wake_cnt >= cur_idle) {
                        wake_cnt = cur_idle;
                    }
                }
                if (*reinterpret_cast<uint8_t*>(base + 0x68) == 0) {
                    _mm_mfence();
                    __atomic_exchange_n(reinterpret_cast<uint32_t*>(base + 0x18), 0u, __ATOMIC_SEQ_CST);
                } else if (wake_cnt != 0) {
                    __atomic_fetch_sub(reinterpret_cast<uint32_t*>(base + 0x18), wake_cnt, __ATOMIC_SEQ_CST);
                }

                uint32_t signaled = 0;
                if (wake_cnt != 0) {
                    void* wake_sem = *reinterpret_cast<void**>(base + 0x20);
                    if (wake_sem != nullptr) {
                        reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(wake_sem, static_cast<int32_t>(wake_cnt));
                    }
                    signaled = wake_cnt;
                }
                if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
                    signaled += *reinterpret_cast<uint32_t*>(base + 0x34);
                }
                *reinterpret_cast<uint32_t*>(base + 0x34) = signaled;
                *reinterpret_cast<uint8_t*>(base + 0x31) = 0;
                *reinterpret_cast<uint8_t*>(base + 0x68) = 1;
                *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 0;

                if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c30ULL)) == 0) {
                    *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c32ULL)) = 1;
                    return;
                }
            }
            _mm_pause();
        }

        uint32_t total_w = *reinterpret_cast<uint32_t*>(base + 0x08);
        if (total_w >= 2u) {
            auto* workers_base = *reinterpret_cast<uint8_t**>(base + 0x00);
            for (uint32_t idx = 1; idx < total_w; ++idx) {
                auto* state_ptr = reinterpret_cast<volatile uint32_t*>(
                    workers_base + static_cast<size_t>(idx) * 0x530ULL + 0x528ULL);
                while (*state_ptr != 2u) {
                    _mm_pause();
                }
            }
        }
    }

    int32_t final_idle_read = *idle_counter;
    __asm__ volatile("" : : "r"(final_idle_read));
    *reinterpret_cast<uint8_t*>(base + 0x68) = 0;
}

// ============================================================================
// [23] 0x21b8710 (642 B) - GXWorkerTaskPool_FlushAndExecuteAllTasks
// ============================================================================
void GXWorkerTaskPool_FlushAndExecuteAllTasks(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    uint8_t active_flag = *reinterpret_cast<uint8_t*>(base + 0x68);
    if (*reinterpret_cast<uint8_t*>(base + 0x31) != 0) {
        if (active_flag == 0) {
            *reinterpret_cast<uint64_t*>(base + 0x10) = *reinterpret_cast<uint64_t*>(base + 0x70);
            uint32_t wake_cnt = *reinterpret_cast<uint32_t*>(base + 0x08) - 1u;
            _mm_mfence();
            __atomic_exchange_n(reinterpret_cast<uint32_t*>(base + 0x18), 0u, __ATOMIC_SEQ_CST);
            uint32_t signaled = 0;
            if (wake_cnt != 0) {
                void* wake_sem = *reinterpret_cast<void**>(base + 0x20);
                if (wake_sem != nullptr) {
                    reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(wake_sem, static_cast<int32_t>(wake_cnt));
                }
                signaled = wake_cnt;
            }
            if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
                signaled += *reinterpret_cast<uint32_t*>(base + 0x34);
            }
            *reinterpret_cast<uint32_t*>(base + 0x34) = signaled;
            *reinterpret_cast<uint8_t*>(base + 0x31) = 0;
            *reinterpret_cast<uint8_t*>(base + 0x68) = 1;
        }
    } else if (active_flag == 0) {
        return;
    }

    *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c30ULL)) = 0;
    auto* idle_counter = reinterpret_cast<volatile int32_t*>(base + 0x18);

    auto wake_pending_workers = [&]() {
        *reinterpret_cast<uint64_t*>(base + 0x10) = *reinterpret_cast<uint64_t*>(base + 0x70);
        uint32_t cur_workers = *reinterpret_cast<uint32_t*>(base + 0x08);
        uint32_t wake_cnt = cur_workers - 1u;
        uint32_t cur_idle = static_cast<uint32_t>(*idle_counter);
        if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
            if (wake_cnt >= cur_idle) {
                wake_cnt = cur_idle;
            }
        }
        if (*reinterpret_cast<uint8_t*>(base + 0x68) == 0) {
            _mm_mfence();
            __atomic_exchange_n(reinterpret_cast<uint32_t*>(base + 0x18), 0u, __ATOMIC_SEQ_CST);
        } else if (wake_cnt != 0) {
            __atomic_fetch_sub(reinterpret_cast<uint32_t*>(base + 0x18), wake_cnt, __ATOMIC_SEQ_CST);
        }
        uint32_t signaled = 0;
        if (wake_cnt != 0) {
            void* wake_sem = *reinterpret_cast<void**>(base + 0x20);
            if (wake_sem != nullptr) {
                reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(wake_sem, static_cast<int32_t>(wake_cnt));
            }
            signaled = wake_cnt;
        }
        if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
            signaled += *reinterpret_cast<uint32_t*>(base + 0x34);
        }
        *reinterpret_cast<uint32_t*>(base + 0x34) = signaled;
        *reinterpret_cast<uint8_t*>(base + 0x31) = 0;
        *reinterpret_cast<uint8_t*>(base + 0x68) = 1;
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) = 0;
    };

    for (;;) {
        do {
            GXWorkerThreadContext_ExecuteOneTaskOrSteal(*reinterpret_cast<void**>(base + 0x00));
            if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) != 0) {
                wake_pending_workers();
            }
        } while (*reinterpret_cast<uint64_t*>(*reinterpret_cast<uint8_t**>(base + 0x00) + 0x38) != 0);

        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c32ULL)) = 0;
        if (*reinterpret_cast<uint8_t*>(base + 0x68) == 0) {
            break;
        }

        uint32_t worker_count = *reinterpret_cast<uint32_t*>(base + 0x08);
        if (worker_count != 0) {
            int32_t target_idle = static_cast<int32_t>(worker_count - 1u);
            bool restart_exec = false;
            while (target_idle > *idle_counter) {
                if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c31ULL)) != 0) {
                    wake_pending_workers();
                    if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c30ULL)) == 0) {
                        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c32ULL)) = 1;
                        restart_exec = true;
                        break;
                    }
                }
                _mm_pause();
            }
            if (restart_exec) {
                continue;
            }

            uint32_t total_w = *reinterpret_cast<uint32_t*>(base + 0x08);
            if (total_w >= 2u) {
                auto* workers_base = *reinterpret_cast<uint8_t**>(base + 0x00);
                for (uint32_t idx = 1; idx < total_w; ++idx) {
                    auto* state_ptr = reinterpret_cast<volatile uint32_t*>(
                        workers_base + static_cast<size_t>(idx) * 0x530ULL + 0x528ULL);
                    while (*state_ptr != 2u) {
                        _mm_pause();
                    }
                }
            }
        }

        int32_t final_idle_read = *idle_counter;
        __asm__ volatile("" : : "r"(final_idle_read));
        *reinterpret_cast<uint8_t*>(base + 0x68) = 0;
        if (*reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c32ULL)) == 0) {
            break;
        }
    }

    *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6c30ULL)) = 1;
}

// ============================================================================
// [24] 0x21b8670 (147 B) - GXWorkerTaskPool_WakeWorkers
// ============================================================================
void GXWorkerTaskPool_WakeWorkers(void* self, void* queue, uint32_t max_workers) {
    auto* base = static_cast<uint8_t*>(self);
    if (queue == nullptr) {
        queue = *reinterpret_cast<void**>(base + 0x70);
    }
    *reinterpret_cast<void**>(base + 0x10) = queue;

    uint32_t avail_workers = *reinterpret_cast<uint32_t*>(base + 0x08) - 1u;
    if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
        uint32_t cur_idle = *reinterpret_cast<uint32_t*>(base + 0x18);
        if (avail_workers >= cur_idle) {
            avail_workers = cur_idle;
        }
    }

    uint32_t wake_cnt = avail_workers;
    if (max_workers != 0) {
        uint32_t req = max_workers - 1u;
        if (req >= avail_workers) {
            req = avail_workers;
        }
        wake_cnt = req;
    }

    if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
        if (wake_cnt != 0) {
            __atomic_fetch_sub(reinterpret_cast<uint32_t*>(base + 0x18), wake_cnt, __ATOMIC_SEQ_CST);
        }
    } else {
        uint32_t rem_idle = avail_workers - wake_cnt;
        _mm_mfence();
        __atomic_exchange_n(reinterpret_cast<uint32_t*>(base + 0x18), rem_idle, __ATOMIC_SEQ_CST);
    }

    if (wake_cnt != 0) {
        void* wake_sem = *reinterpret_cast<void**>(base + 0x20);
        if (wake_sem != nullptr) {
            reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(wake_sem, static_cast<int32_t>(wake_cnt));
        }
    }

    if (*reinterpret_cast<uint8_t*>(base + 0x68) != 0) {
        wake_cnt += *reinterpret_cast<uint32_t*>(base + 0x34);
    }
    *reinterpret_cast<uint32_t*>(base + 0x34) = wake_cnt;
    *reinterpret_cast<uint8_t*>(base + 0x31) = 0;
    *reinterpret_cast<uint8_t*>(base + 0x68) = 1;
}

// ============================================================================
// [25] 0x21b84b0 (442 B) - GXWorkerTaskPool_ShutdownAndDestroy
// ============================================================================
void GXWorkerTaskPool_ShutdownAndDestroy(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint8_t*>(base + 0x30) = 1;

    uint32_t worker_count = *reinterpret_cast<uint32_t*>(base + 0x08);
    if (worker_count >= 2u) {
        int32_t expected_idle = static_cast<int32_t>(worker_count - 1u);
        auto* idle_ptr = reinterpret_cast<volatile int32_t*>(base + 0x18);
        while (expected_idle > *idle_ptr) {
            _mm_pause();
        }
        void* wake_sem = *reinterpret_cast<void**>(base + 0x20);
        if (wake_sem != nullptr) {
            reinterpret_cast<FnVoidPtrInt>(GX_VMA(0x263b0d0ULL))(
                wake_sem, static_cast<int32_t>(*reinterpret_cast<uint32_t*>(base + 0x08) - 1u));
        }
    }

    auto* workers_base = *reinterpret_cast<uint8_t**>(base + 0x00);
    if (workers_base != nullptr) {
        if (*reinterpret_cast<uint32_t*>(base + 0x08) != 0) {
            uint32_t idx = 0;
            do {
                uint8_t* w_ctx = workers_base + static_cast<size_t>(idx) * 0x530ULL;
                if (*reinterpret_cast<uint64_t*>(w_ctx + 0x10) != 0) {
                    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a670ULL))(w_ctx + 0x80);
                    auto* thr = *reinterpret_cast<uint8_t**>(w_ctx + 0x10);
                    reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b6a0ULL))(thr + 0x08, 0xFFFFFFFFu);
                    thr = *reinterpret_cast<uint8_t**>(w_ctx + 0x10);
                    if (thr != nullptr) {
                        void* owner_heap = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(thr);
                        auto* thr_vtbl = *reinterpret_cast<FnVoidPtr**>(thr);
                        thr_vtbl[1](thr);
                        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_heap);
                        h_vtbl[14](owner_heap, thr);
                        *reinterpret_cast<uint64_t*>(w_ctx + 0x10) = 0;
                    }
                }
                ++idx;
                workers_base = *reinterpret_cast<uint8_t**>(base + 0x00);
            } while (idx < *reinterpret_cast<uint32_t*>(base + 0x08));
        }

        if (workers_base != nullptr) {
            void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
            uint64_t array_count = *reinterpret_cast<uint64_t*>(workers_base - 0x08);
            for (uint64_t i = 0; i < array_count; ++i) {
                uint8_t* elem = workers_base + i * 0x530ULL;
                auto* e_vtbl = *reinterpret_cast<FnVoidPtr**>(elem);
                e_vtbl[1](elem);
            }
            auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
            h_vtbl[14](sys_heap, *reinterpret_cast<void**>(workers_base - 0x10));
            *reinterpret_cast<uint64_t*>(base + 0x00) = 0;
        }
    }

    void* wake_sem = *reinterpret_cast<void**>(base + 0x20);
    if (wake_sem != nullptr) {
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x263ac50ULL))(wake_sem);
        wake_sem = *reinterpret_cast<void**>(base + 0x20);
        if (wake_sem != nullptr) {
            void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
            auto* w_vtbl = *reinterpret_cast<FnVoidPtr**>(wake_sem);
            w_vtbl[0](wake_sem);
            auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
            h_vtbl[14](sys_heap, wake_sem);
            *reinterpret_cast<uint64_t*>(base + 0x20) = 0;
        }
    }

    void* listener = *reinterpret_cast<void**>(base + 0x38);
    if (listener != nullptr) {
        auto* l_vtbl = *reinterpret_cast<FnVoidPtrU32**>(listener);
        l_vtbl[2](listener, 0);
        listener = *reinterpret_cast<void**>(base + 0x38);
        if (listener != nullptr) {
            auto* l_vtbl2 = *reinterpret_cast<FnVoidPtr**>(listener);
            l_vtbl2[3](listener);
        }
    }

    void* def_queue = *reinterpret_cast<void**>(base + 0x70);
    if (def_queue != nullptr) {
        void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        GXWorkStealingDeque_Dtor(def_queue);
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
        h_vtbl[14](sys_heap, def_queue);
        *reinterpret_cast<uint64_t*>(base + 0x70) = 0;
    }
}

// ============================================================================
// [26] 0x21b8140 (875 B) - GXWorkerTaskPool_Initialize
// ============================================================================
bool GXWorkerTaskPool_Initialize(void* self, const void* init_desc, const uint64_t* affinity_masks) {
    auto* base = static_cast<uint8_t*>(self);
    const auto* desc = static_cast<const uint8_t*>(init_desc);
    uint32_t worker_count = *reinterpret_cast<const uint32_t*>(desc + 0x00);

    *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
    *reinterpret_cast<uint32_t*>(base + 0x18) = 0;

    auto** sys_heap_slot = reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* sys_heap = *sys_heap_slot;
    void* wake_sem = nullptr;
    {
        auto* h_vtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap);
        void* raw_sem = h_vtbl[11](sys_heap, 0x20ULL, 0x08ULL);
        if (raw_sem != nullptr) {
            reinterpret_cast<void (*)(void*, uint32_t, uint32_t)>(GX_VMA(0x263aad0ULL))(
                raw_sem, 0, worker_count - 1u);
            wake_sem = raw_sem;
        }
    }
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x263acc0ULL))(wake_sem);
    *reinterpret_cast<void**>(base + 0x20) = wake_sem;
    *reinterpret_cast<uint8_t*>(base + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x28) = 0;
    *reinterpret_cast<uint8_t*>(base + 0x31) = 0;

    sys_heap = *sys_heap_slot;
    uint8_t* workers_array = nullptr;
    {
        auto* h_vtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap);
        size_t alloc_bytes = static_cast<size_t>(worker_count) * 0x530ULL + 0x10ULL;
        auto* raw_block = static_cast<uint8_t*>(h_vtbl[11](sys_heap, alloc_bytes, 0x10ULL));
        if (raw_block != nullptr) {
            for (uint32_t i = 0; i < worker_count; ++i) {
                uint8_t* elem = raw_block + 0x10ULL + static_cast<size_t>(i) * 0x530ULL;
                *reinterpret_cast<uint64_t*>(elem + 0x00) = GX_VMA(0x535fc20ULL);
                *reinterpret_cast<uint64_t*>(elem + 0x10) = 0;
                *reinterpret_cast<uint64_t*>(elem + 0x08) = 0;
                reinterpret_cast<FnVoidPtr>(GX_VMA(0xe47a50ULL))(elem + 0x18);

                uint8_t* sync_prim = elem + 0x80;
                reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a510ULL))(sync_prim);
                *reinterpret_cast<uint64_t*>(sync_prim) = GX_VMA(0x53b6c40ULL);
                reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a610ULL))(sync_prim);

                *reinterpret_cast<uint32_t*>(elem + 0xa0) = 0;
                *reinterpret_cast<uint64_t*>(elem + 0xa8) = 0;
                *reinterpret_cast<uint64_t*>(elem + 0xc8) = GX_VMA(0x53b5110ULL);
                std::memset(elem + 0xd0, 0, 0x448);
                *reinterpret_cast<uint64_t*>(elem + 0x518) = GX_VMA(0x52b5dd0ULL);
                *reinterpret_cast<uint64_t*>(elem + 0xb0) = 0;
                *reinterpret_cast<uint32_t*>(elem + 0xb8) =
                    reinterpret_cast< uint32_t (*)() >(GX_VMA(0x207b9f0ULL))();
                *reinterpret_cast<uint64_t*>(elem + 0xc0) = 0;
                *reinterpret_cast<uint32_t*>(elem + 0x520) = 0;
                *reinterpret_cast<void**>(elem + 0xd0) = elem + 0xb0;
                *reinterpret_cast<uint32_t*>(elem + 0x528) = 0;
            }
            *reinterpret_cast<uint64_t*>(raw_block + 0x08) = worker_count;
            *reinterpret_cast<void**>(raw_block + 0x00) = raw_block;
            workers_array = raw_block + 0x10ULL;
        }
    }

    *reinterpret_cast<uint8_t**>(base + 0x00) = workers_array;
    *reinterpret_cast<uint32_t*>(base + 0x08) = worker_count;

    void* listener = *reinterpret_cast<void* const*>(desc + 0x08);
    *reinterpret_cast<void**>(base + 0x38) = listener;
    if (listener != nullptr) {
        auto* l_vtbl = *reinterpret_cast<FnVoidPtrU32**>(listener);
        l_vtbl[0](listener, worker_count);
        listener = *reinterpret_cast<void* const*>(desc + 0x08);
        if (listener != nullptr) {
            auto* l_vtbl2 = *reinterpret_cast<FnVoidPtrU32**>(listener);
            l_vtbl2[1](listener, 0);
        }
    }

    GXWorkerThreadContext_InitializeMasterWorker(*reinterpret_cast<void**>(base + 0x00), self);

    if (worker_count >= 2u) {
        if (affinity_masks != nullptr) {
            for (uint32_t idx = 0; idx < worker_count - 1u; ++idx) {
                uint32_t stack_sz = *reinterpret_cast<const uint32_t*>(desc + 0x04);
                uint64_t mask = affinity_masks[idx];
                uint32_t w_id = idx + 1u;
                uint8_t* w_ctx = *reinterpret_cast<uint8_t**>(base + 0x00) + static_cast<size_t>(w_id) * 0x530ULL;
                GXWorkerThreadContext_InitializeAndSpawnWorker(w_ctx, self, stack_sz, w_id, mask);
            }
        } else {
            for (uint32_t w_id = 1u; w_id < worker_count; ++w_id) {
                uint32_t stack_sz = *reinterpret_cast<const uint32_t*>(desc + 0x04);
                uint8_t* w_ctx = *reinterpret_cast<uint8_t**>(base + 0x00) + static_cast<size_t>(w_id) * 0x530ULL;
                GXWorkerThreadContext_InitializeAndSpawnWorker(w_ctx, self, stack_sz, w_id, 0xFFFFFFFFULL);
            }
        }
    }

    int32_t expected_idle = static_cast<int32_t>(*reinterpret_cast<uint32_t*>(base + 0x08) - 1u);
    auto* idle_ptr = reinterpret_cast<volatile int32_t*>(base + 0x18);
    while (expected_idle > *idle_ptr) {
        _mm_pause();
    }

    if (*reinterpret_cast<uint32_t*>(base + 0x08) >= 2u) {
        for (uint32_t w_id = 1u; w_id < *reinterpret_cast<uint32_t*>(base + 0x08); ++w_id) {
            uint8_t* w_ctx = *reinterpret_cast<uint8_t**>(base + 0x00) + static_cast<size_t>(w_id) * 0x530ULL;
            reinterpret_cast<FnVoidPtr>(GX_VMA(0x263a670ULL))(w_ctx + 0x80);
        }
    }

    sys_heap = *sys_heap_slot;
    void* def_queue = nullptr;
    {
        auto* h_vtbl = *reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap);
        void* raw_q = h_vtbl[11](sys_heap, 0x68ULL, 0x08ULL);
        if (raw_q != nullptr) {
            reinterpret_cast<FnVoidPtr>(GX_VMA(0xe47a50ULL))(raw_q);
            def_queue = raw_q;
        }
    }
    *reinterpret_cast<void**>(base + 0x70) = def_queue;
    *reinterpret_cast<uint8_t*>(base + 0x68) = 0;
    return true;
}

// ============================================================================
// [27] 0x21b8050 (233 B) - GXWorkerTaskPool_Ctor
// ============================================================================
void GXWorkerTaskPool_Ctor(void* self) {
    auto* base = static_cast<uint8_t*>(self);
    auto** heap_slot = reinterpret_cast<void**>(GX_VMA(0x5540670ULL));
    void* heap = *heap_slot;
    if (heap == nullptr) {
        heap = reinterpret_cast<FnPtrVoid>(GX_VMA(0x2ba2b10ULL))();
        *heap_slot = heap;
    }

    uint64_t heap_caps = 0;
    {
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(heap);
        h_vtbl[4](&heap_caps, heap, 0);
    }
    if ((heap_caps & 0x20ULL) == 0) {
        reinterpret_cast<void (*)(uint32_t, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
            0, 0x3e, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    *reinterpret_cast<uint64_t*>(base + 0x58) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x50) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x48) = 0;
    *reinterpret_cast<void**>(base + 0x60) = heap;
    *reinterpret_cast<uint8_t*>(base + 0x68) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x70) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x38) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x30) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x28) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(base + 0x00) = 0;
}

// ============================================================================
// [28] 0x21b7f60 (233 B) - GXInternalTaskPool_AllocateTaskNode
// ============================================================================
void* GXInternalTaskPool_AllocateTaskNode(void* pool_slot) {
    auto* base = static_cast<uint8_t*>(pool_slot);
    if (*reinterpret_cast<uint64_t*>(base + 0x38) != 0) {
        uint8_t* mtx = base + 0x20;
        {
            auto* m_vtbl = *reinterpret_cast<FnVoidPtrU32**>(mtx);
            m_vtbl[3](mtx, 0xFFFFFFFFu);
        }

        auto* node = *reinterpret_cast<uint8_t**>(base + 0x38);
        *reinterpret_cast<uint64_t*>(base + 0x38) = 0;
        int32_t write_idx = *reinterpret_cast<int32_t*>(base + 0x14);
        int32_t cap = *reinterpret_cast<int32_t*>(base + 0x18);

        while (node != nullptr) {
            auto* next_node = *reinterpret_cast<uint8_t**>(node + 0x20);
            int32_t next_idx = write_idx + 1;
            int32_t wrap_sub = (next_idx < cap) ? 0 : cap;
            auto* ring = *reinterpret_cast<uint8_t***>(base + 0x00);
            ring[write_idx] = node;
            write_idx = write_idx - wrap_sub + 1;
            node = next_node;
        }

        *reinterpret_cast<int32_t*>(base + 0x14) = write_idx;
        *reinterpret_cast<uint32_t*>(base + 0x58) += *reinterpret_cast<uint32_t*>(base + 0x54);
        *reinterpret_cast<uint32_t*>(base + 0x54) = 0;

        {
            auto* m_vtbl = *reinterpret_cast<FnVoidPtr**>(mtx);
            m_vtbl[5](mtx);
        }
    }

    int32_t read_idx = *reinterpret_cast<int32_t*>(base + 0x10);
    int32_t next_read = read_idx + 1;
    *reinterpret_cast<int32_t*>(base + 0x10) = next_read;
    int32_t cap = *reinterpret_cast<int32_t*>(base + 0x18);
    auto* ring = *reinterpret_cast<uint8_t***>(base + 0x00);
    uint8_t* allocated = ring[read_idx];

    int32_t wrapped_read = next_read - cap;
    if (wrapped_read >= 0) {
        *reinterpret_cast<int32_t*>(base + 0x10) = wrapped_read;
        next_read = wrapped_read;
    }

    *reinterpret_cast<uint32_t*>(allocated + 0x20) = *reinterpret_cast<uint32_t*>(base + 0x40);

    int32_t used = next_read - *reinterpret_cast<int32_t*>(base + 0x14);
    if (used < 0) {
        used += cap;
    }
    int32_t cur_max = *reinterpret_cast<int32_t*>(base + 0x50);
    if (cur_max <= used) {
        *reinterpret_cast<int32_t*>(base + 0x50) = used;
    }

    return allocated;
}

// ============================================================================
// [29] 0x21b7e10 (336 B) - GXParallelForTask_SplitAndDispatch
// ============================================================================
bool GXParallelForTask_SplitAndDispatch(void* self, void* worker_pool) {
    auto* task = static_cast<uint8_t*>(self);
    auto* pool = static_cast<uint8_t*>(worker_pool);

    int32_t cur_start = *reinterpret_cast<int32_t*>(task + 0x10);
    int32_t cur_end = *reinterpret_cast<int32_t*>(task + 0x14);
    int32_t grain = *reinterpret_cast<int32_t*>(task + 0x18);
    if (grain <= 0) {
        grain = 1;
    }

    int32_t total_items = cur_end - cur_start;
    int32_t max_chunks = total_items / grain;
    int32_t worker_cnt = *reinterpret_cast<int32_t*>(pool + 0x08);
    int32_t num_splits = (max_chunks > worker_cnt) ? worker_cnt : max_chunks;

    if (num_splits > 0) {
        int32_t base_items_per_split = total_items / num_splits;
        int32_t remainder = total_items - num_splits * base_items_per_split;
        int32_t loop_cnt = ~((~worker_cnt >= ~max_chunks) ? ~worker_cnt : ~max_chunks);
        size_t pool_slot_off = 0;

        for (int32_t idx = 0; idx < loop_cnt; ++idx) {
            int32_t chunk_len = base_items_per_split + ((remainder > idx) ? 1 : 0);
            uint8_t* sub_task = nullptr;

            if (idx > 0) {
                auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
                void* cb = *reinterpret_cast<void**>(task + 0x08);
                int32_t sub_end = cur_start + chunk_len;
                int32_t sub_grain = (chunk_len > grain) ? grain : chunk_len;
                uint8_t* slot_ptr = *reinterpret_cast<uint8_t**>(global_tp + 0x00) + pool_slot_off;
                sub_task = static_cast<uint8_t*>(GXInternalTaskPool_AllocateTaskNode(slot_ptr));
                *reinterpret_cast<void**>(sub_task + 0x08) = cb;
                *reinterpret_cast<int32_t*>(sub_task + 0x10) = cur_start;
                *reinterpret_cast<int32_t*>(sub_task + 0x14) = sub_end;
                *reinterpret_cast<int32_t*>(sub_task + 0x18) = sub_grain;
            } else {
                int32_t sub_end = cur_start + chunk_len;
                int32_t sub_grain = (chunk_len > grain) ? grain : chunk_len;
                *reinterpret_cast<int32_t*>(task + 0x10) = cur_start;
                *reinterpret_cast<int32_t*>(task + 0x14) = sub_end;
                *reinterpret_cast<int32_t*>(task + 0x18) = sub_grain;
                sub_task = task;
            }

            auto* workers_base = *reinterpret_cast<uint8_t**>(pool + 0x00);
            uint8_t* target_deque = workers_base + static_cast<size_t>(idx) * 0x530ULL + 0x18ULL;
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(target_deque, sub_task);

            cur_start += chunk_len;
            pool_slot_off += 0x60ULL;
        }
    }

    return true;
}

// ============================================================================
// [30] 0x21b7d40 (204 B) - GXParallelForTask_TrySplitHalf
// ============================================================================
bool GXParallelForTask_TrySplitHalf(void* self, void** out_split_task) {
    auto* task = static_cast<uint8_t*>(self);
    int32_t start = *reinterpret_cast<int32_t*>(task + 0x10);
    int32_t end = *reinterpret_cast<int32_t*>(task + 0x14);
    int32_t span = end - start;
    if (span == 0) {
        return false;
    }

    int32_t grain = *reinterpret_cast<int32_t*>(task + 0x18);
    if (span < grain * 2) {
        return false;
    }

    int32_t sum = start + end;
    int32_t mid = (sum + static_cast<int32_t>(static_cast<uint32_t>(sum) >> 31)) >> 1;

    uint8_t* fs_base = gx_read_fs_base();
    auto* cur_worker = *reinterpret_cast<uint8_t**>(fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4658ULL)));
    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    void* cb = *reinterpret_cast<void**>(task + 0x08);

    int32_t upper_span = end - mid;
    int32_t sub_grain = (upper_span > grain) ? grain : upper_span;
    uint32_t worker_id = *reinterpret_cast<uint32_t*>(cur_worker + 0xa0);
    uint8_t* slot_ptr = *reinterpret_cast<uint8_t**>(global_tp + 0x00) + static_cast<size_t>(worker_id) * 0x60ULL;

    auto* new_task = static_cast<uint8_t*>(GXInternalTaskPool_AllocateTaskNode(slot_ptr));
    if (new_task == nullptr) {
        return false;
    }
    *reinterpret_cast<void**>(new_task + 0x08) = cb;
    *reinterpret_cast<int32_t*>(new_task + 0x10) = mid;
    *reinterpret_cast<int32_t*>(new_task + 0x14) = end;
    *reinterpret_cast<int32_t*>(new_task + 0x18) = sub_grain;

    *reinterpret_cast<int32_t*>(task + 0x14) = mid;
    *out_split_task = new_task;
    return true;
}

// ============================================================================
// [31] 0x21b7c90 (172 B) - GXParallelForTask_TrySplitGrain
// ============================================================================
bool GXParallelForTask_TrySplitGrain(void* self, void** out_split_task) {
    auto* task = static_cast<uint8_t*>(self);
    *out_split_task = nullptr;

    int32_t start = *reinterpret_cast<int32_t*>(task + 0x10);
    int32_t end = *reinterpret_cast<int32_t*>(task + 0x14);
    int32_t span = end - start;
    if (span == 0) {
        return false;
    }

    int32_t grain = *reinterpret_cast<int32_t*>(task + 0x18);
    if (span < grain * 2) {
        return false;
    }

    int32_t split_pt = start + grain;
    uint8_t* fs_base = gx_read_fs_base();
    auto* cur_worker = *reinterpret_cast<uint8_t**>(fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e4658ULL)));
    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    void* cb = *reinterpret_cast<void**>(task + 0x08);

    uint32_t worker_id = *reinterpret_cast<uint32_t*>(cur_worker + 0xa0);
    uint8_t* slot_ptr = *reinterpret_cast<uint8_t**>(global_tp + 0x00) + static_cast<size_t>(worker_id) * 0x60ULL;

    auto* new_task = static_cast<uint8_t*>(GXInternalTaskPool_AllocateTaskNode(slot_ptr));
    if (new_task == nullptr) {
        return false;
    }
    *reinterpret_cast<void**>(new_task + 0x08) = cb;
    *reinterpret_cast<int32_t*>(new_task + 0x10) = start;
    *reinterpret_cast<int32_t*>(new_task + 0x14) = split_pt;
    *reinterpret_cast<int32_t*>(new_task + 0x18) = grain;

    *reinterpret_cast<int32_t*>(task + 0x10) = split_pt;
    *out_split_task = new_task;
    return true;
}

// ============================================================================
// [32] 0x21b7be0 (166 B) - GXParallelForTask_ExecuteAndRecycle
// ============================================================================
void GXParallelForTask_ExecuteAndRecycle(void* self, uint32_t worker_id) {
    auto* task = static_cast<uint8_t*>(self);
    void* cb = *reinterpret_cast<void**>(task + 0x08);
    {
        auto* cb_vtbl = *reinterpret_cast<void (***)(void*, void*, uint32_t)>(cb);
        cb_vtbl[2](cb, task + 0x10, worker_id);
    }

    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    uint32_t owner_pool_id = *reinterpret_cast<uint32_t*>(task + 0x20);
    uint8_t* slots_base = *reinterpret_cast<uint8_t**>(global_tp + 0x00);
    uint8_t* slot = slots_base + static_cast<size_t>(owner_pool_id) * 0x60ULL;

    if (*reinterpret_cast<uint32_t*>(slot + 0x40) == worker_id) {
        int32_t write_idx = *reinterpret_cast<int32_t*>(slot + 0x14);
        int32_t next_idx = write_idx + 1;
        *reinterpret_cast<int32_t*>(slot + 0x14) = next_idx;
        auto* ring = *reinterpret_cast<void***>(slot + 0x00);
        ring[write_idx] = task;
        int32_t wrapped = next_idx - *reinterpret_cast<int32_t*>(slot + 0x18);
        if (wrapped >= 0) {
            *reinterpret_cast<int32_t*>(slot + 0x14) = wrapped;
        }
        return;
    }

    uint8_t* mtx = slot + 0x20;
    {
        auto* m_vtbl = *reinterpret_cast<FnVoidPtrU32**>(mtx);
        m_vtbl[3](mtx, 0xFFFFFFFFu);
    }
    *reinterpret_cast<uint64_t*>(task + 0x20) = *reinterpret_cast<uint64_t*>(slot + 0x38);
    *reinterpret_cast<void**>(slot + 0x38) = task;
    *reinterpret_cast<uint32_t*>(slot + 0x54) += 1u;
    {
        auto* m_vtbl = *reinterpret_cast<FnVoidPtr**>(mtx);
        m_vtbl[5](mtx);
    }
}

// ============================================================================
// [33] 0x21b7490 (1724 B) - GXInternalTaskPool_RegisterDebugProperties
// ============================================================================
void GXInternalTaskPool_RegisterDebugProperties(void* prop_bag) {
    auto* bag = static_cast<uint8_t*>(prop_bag);
    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    if (global_tp == nullptr) {
        return;
    }

    struct PropGroupDesc {
        uint64_t    pad0;
        const void* name_utf16;
    } group_desc{0, reinterpret_cast<const void*>(GX_VMA(0x49c36d0ULL))}; // u"InternalTaskPool"

    void* group_res = reinterpret_cast<void* (*)(void*, void*, uint32_t)>(GX_VMA(0x236c900ULL))(
        bag, &group_desc, 0);
    group_desc.name_utf16 = nullptr;
    if (group_res == nullptr) {
        return;
    }

    auto release_ref_str = [](uint8_t* str_obj) {
        if (str_obj == nullptr) {
            return;
        }
        uint32_t rc = *reinterpret_cast<uint32_t*>(str_obj + 0x38) - 1u;
        *reinterpret_cast<uint32_t*>(str_obj + 0x38) = rc;
        if (rc == 0) {
            void* str_heap = *reinterpret_cast<void**>(GX_VMA(0x5540e10ULL));
            if (*reinterpret_cast<uint64_t*>(str_obj + 0x20) >= 8ULL) {
                void* buf_alloc = *reinterpret_cast<void**>(str_obj + 0x28);
                auto* ba_vtbl = *reinterpret_cast<void (***)(void*, void*)>(buf_alloc);
                ba_vtbl[14](buf_alloc, *reinterpret_cast<void**>(str_obj + 0x08));
            }
            *reinterpret_cast<uint64_t*>(str_obj + 0x20) = 7ULL;
            *reinterpret_cast<uint64_t*>(str_obj + 0x18) = 0ULL;
            *reinterpret_cast<uint16_t*>(str_obj + 0x08) = 0;
            auto* sh_vtbl = *reinterpret_cast<void (***)(void*, void*)>(str_heap);
            sh_vtbl[14](str_heap, str_obj);
        }
    };

    auto register_pool_metric = [&](uint32_t pool_idx, uintptr_t fmt_addr, size_t field_off) {
        struct FormattedProp {
            uint8_t* str_obj;
            uint64_t raw_str;
        } fmt_prop{nullptr, 0};

        reinterpret_cast<void (*)(void*, const void*, uint32_t)>(GX_VMA(0x2ba7d40ULL))(
            &fmt_prop, reinterpret_cast<const void*>(fmt_addr), pool_idx);

        uint8_t* retained_obj = nullptr;
        uint64_t active_raw = fmt_prop.raw_str;
        if (active_raw == 0 && fmt_prop.str_obj != nullptr) {
            *reinterpret_cast<uint32_t*>(fmt_prop.str_obj + 0x38) += 1u;
            retained_obj = fmt_prop.str_obj;
        }

        uint8_t* slot_base = *reinterpret_cast<uint8_t**>(global_tp + 0x00) +
                             static_cast<size_t>(pool_idx) * 0x60ULL;
        void* metric_ptr = slot_base + field_off;

        FormattedProp bind_arg{nullptr, 0};
        if (active_raw == 0) {
            if (retained_obj != nullptr) {
                bind_arg.str_obj = retained_obj;
                *reinterpret_cast<uint32_t*>(retained_obj + 0x38) += 1u;
                reinterpret_cast<void (*)(void*, void*, void*)>(GX_VMA(0x29055d0ULL))(bag, &bind_arg, metric_ptr);
                bind_arg.raw_str = 0;
                release_ref_str(retained_obj);
                bind_arg.str_obj = nullptr;
            } else {
                reinterpret_cast<void (*)(void*, void*, void*)>(GX_VMA(0x29055d0ULL))(bag, &bind_arg, metric_ptr);
                bind_arg.raw_str = 0;
            }
        } else {
            bind_arg.raw_str = active_raw;
            reinterpret_cast<void (*)(void*, void*, void*)>(GX_VMA(0x29055d0ULL))(bag, &bind_arg, metric_ptr);
            bind_arg.raw_str = 0;
        }

        release_ref_str(retained_obj);
        fmt_prop.raw_str = 0;
        release_ref_str(fmt_prop.str_obj);
        fmt_prop.str_obj = nullptr;
    };

    uint32_t count = *reinterpret_cast<uint32_t*>(global_tp + 0x08);
    if (count != 0) {
        for (uint32_t i = 0; i < count; ++i) {
            register_pool_metric(i, GX_VMA(0x49c36f2ULL), 0x44ULL); // u"pool%d cur"
            count = *reinterpret_cast<uint32_t*>(global_tp + 0x08);
        }
        if (count != 0) {
            for (uint32_t i = 0; i < count; ++i) {
                register_pool_metric(i, GX_VMA(0x49c3708ULL), 0x48ULL); // u"pool%d max"
                count = *reinterpret_cast<uint32_t*>(global_tp + 0x08);
            }
            if (count != 0) {
                for (uint32_t i = 0; i < count; ++i) {
                    register_pool_metric(i, GX_VMA(0x49c371eULL), 0x4cULL); // u"pool%d free"
                }
            }
        }
    }

    auto* stack_begin = *reinterpret_cast<uint8_t***>(bag + 0x08);
    auto* stack_top = *reinterpret_cast<uint8_t***>(bag + 0x10);
    if (stack_begin != stack_top) {
        uint8_t* node = *(stack_top - 1);
        if (node != nullptr) {
            uint32_t rc_word = *reinterpret_cast<uint32_t*>(node + 0x08);
            uint32_t new_low = (rc_word + 0x7fffu) & 0x7fffu;
            uint32_t updated = (rc_word & 0xffff8000u) | new_low;
            *reinterpret_cast<uint32_t*>(node + 0x08) = updated;
            if (new_low == 0 && (rc_word & 0x40000000u) != 0) {
                auto* n_vtbl = *reinterpret_cast<FnVoidPtr**>(node);
                n_vtbl[3](node);
                updated = *reinterpret_cast<uint32_t*>(node + 0x08) & 0xbfffffffu;
                *reinterpret_cast<uint32_t*>(node + 0x08) = updated;
            }
            uint32_t weak_cnt = ((updated >> 15) + 0x7fffu) & 0x7fffu;
            uint32_t final_word = (updated & 0xc0007fffu) | (weak_cnt << 15);
            *reinterpret_cast<uint32_t*>(node + 0x08) = final_word;
            if (weak_cnt == 0) {
                *reinterpret_cast<uint32_t*>(node + 0x08) = updated | 0x3fff8000u;
                auto* n_vtbl = *reinterpret_cast<FnVoidPtr**>(node);
                n_vtbl[4](node);
            }
            *(stack_top - 1) = nullptr;
            stack_top = *reinterpret_cast<uint8_t***>(bag + 0x10);
        }
        *reinterpret_cast<uint8_t***>(bag + 0x10) = stack_top - 1;
    }
}

// ============================================================================
// [34] 0x21b7440 (76 B) - GXInternalTaskPool_UpdateTelemetryCounters
// ============================================================================
void GXInternalTaskPool_UpdateTelemetryCounters() {
    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    if (global_tp == nullptr) {
        return;
    }
    uint32_t count = *reinterpret_cast<uint32_t*>(global_tp + 0x08);
    if (count == 0) {
        return;
    }
    uint8_t* slot_ptr = *reinterpret_cast<uint8_t**>(global_tp + 0x00) + 0x58ULL;
    for (uint32_t i = 0; i < count; ++i) {
        int32_t cur_peak = *reinterpret_cast<int32_t*>(slot_ptr - 0x08);
        *reinterpret_cast<int32_t*>(slot_ptr - 0x14) = cur_peak;
        int32_t all_time_max = *reinterpret_cast<int32_t*>(slot_ptr - 0x10);
        if (cur_peak >= all_time_max) {
            all_time_max = cur_peak;
        }
        *reinterpret_cast<int32_t*>(slot_ptr - 0x10) = all_time_max;
        int32_t freed_cnt = *reinterpret_cast<int32_t*>(slot_ptr + 0x00);
        *reinterpret_cast<int32_t*>(slot_ptr - 0x0c) = freed_cnt;
        *reinterpret_cast<int32_t*>(slot_ptr - 0x08) = 0;
        *reinterpret_cast<int32_t*>(slot_ptr + 0x00) = 0;
        slot_ptr += 0x60ULL;
    }
}

// ============================================================================
// [35] 0x21b73e0 (82 B) - GXInternalTaskPool_ShutdownGlobal
// ============================================================================
void GXInternalTaskPool_ShutdownGlobal() {
    void* global_tp = *reinterpret_cast<void**>(GX_VMA(0x56a6c28ULL));
    if (global_tp != nullptr) {
        void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
        GXInternalTaskPool_Dtor(global_tp);
        auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
        h_vtbl[14](sys_heap, global_tp);
        *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6c28ULL)) = 0;
    }
}

// ============================================================================
// [36] 0x21b7050 (912 B) - GXInternalTaskPool_InitializeGlobal
// ============================================================================
bool GXInternalTaskPool_InitializeGlobal(uint32_t pool_count, uint64_t tasks_per_pool) {
    auto** sys_heap_slot = reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* sys_heap = *sys_heap_slot;
    auto* global_tp = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap))[11](sys_heap, 0x10ULL, 0x08ULL));
    if (global_tp == nullptr) {
        *reinterpret_cast<uint64_t*>(GX_VMA(0x56a6c28ULL)) = 0;
        return false;
    }

    *reinterpret_cast<uint64_t*>(global_tp + 0x00) = 0;
    *reinterpret_cast<uint32_t*>(global_tp + 0x08) = 0;
    *reinterpret_cast<void**>(GX_VMA(0x56a6c28ULL)) = global_tp;

    sys_heap = *sys_heap_slot;
    size_t slots_alloc_sz = (static_cast<size_t>(pool_count) * 0x60ULL) | 0x10ULL;
    auto* raw_slots = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap))[11](sys_heap, slots_alloc_sz, 0x10ULL));

    uint8_t* slots_base = nullptr;
    if (raw_slots != nullptr) {
        for (uint32_t i = 0; i < pool_count; ++i) {
            uint8_t* s = raw_slots + static_cast<size_t>(i) * 0x60ULL;
            if (reinterpret_cast<uintptr_t>(s) != ~0xFULL) {
                *reinterpret_cast<uint32_t*>(s + 0x28) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x20) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x18) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x10) = 0;
                s[0x40] = 0;

                uint8_t* mtx = s + 0x30;
                *reinterpret_cast<uint64_t*>(mtx) = GX_VMA(0x52b6230ULL);
                if (!reinterpret_cast< bool (*)(void*) >(GX_VMA(0x207ee80ULL))(mtx)) {
                    if (reinterpret_cast<FnIntPtr>(GX_VMA(0x2083fe0ULL))(s + 0x38) == 0) {
                        s[0x40] = 1;
                    }
                }

                *reinterpret_cast<uint32_t*>(s + 0x68) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x60) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x58) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x50) = 0;
                *reinterpret_cast<uint64_t*>(s + 0x48) = 0;
            }
        }
        slots_base = raw_slots + 0x10ULL;
        *reinterpret_cast<uint64_t*>(raw_slots + 0x08) = pool_count;
        *reinterpret_cast<void**>(raw_slots + 0x00) = raw_slots;
    }

    *reinterpret_cast<uint8_t**>(global_tp + 0x00) = slots_base;

    if (pool_count != 0) {
        uint32_t usable_tasks = static_cast<uint32_t>(tasks_per_pool) - 1u;
        size_t nodes_alloc_sz = static_cast<size_t>(usable_tasks) * 0x28ULL + 0x10ULL;
        size_t ring_alloc_sz = static_cast<size_t>( static_cast<uint32_t>(tasks_per_pool) ) * 8ULL;

        for (uint32_t idx = 0; idx < pool_count; ++idx) {
            uint8_t* cur_slots = *reinterpret_cast<uint8_t**>(global_tp + 0x00);
            uint8_t* slot = cur_slots + static_cast<size_t>(idx) * 0x60ULL;
            *reinterpret_cast<uint32_t*>(slot + 0x40) = idx;

            sys_heap = *sys_heap_slot;
            auto* raw_nodes = static_cast<uint8_t*>(
                (*reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap))[11](
                    sys_heap, nodes_alloc_sz, 0x10ULL));
            if (raw_nodes == nullptr) {
                *reinterpret_cast<uint64_t*>(slot + 0x08) = 0;
                continue;
            }

            for (uint32_t t = 0; t < usable_tasks; ++t) {
                uint8_t* n_ptr = raw_nodes + 0x18ULL + static_cast<size_t>(t) * 0x28ULL;
                if (reinterpret_cast<uintptr_t>(n_ptr) != 8ULL) {
                    *reinterpret_cast<uint64_t*>(n_ptr - 0x08) = GX_VMA(0x535fbf0ULL);
                    *reinterpret_cast<uint32_t*>(n_ptr + 0x18) = 0;
                    *reinterpret_cast<uint32_t*>(n_ptr + 0x10) = 0;
                    *reinterpret_cast<uint64_t*>(n_ptr + 0x08) = 0;
                    *reinterpret_cast<uint64_t*>(n_ptr + 0x00) = 0;
                }
            }
            *reinterpret_cast<uint64_t*>(raw_nodes + 0x08) = usable_tasks;
            *reinterpret_cast<void**>(raw_nodes + 0x00) = raw_nodes;
            uint8_t* nodes_array = raw_nodes + 0x10ULL;
            *reinterpret_cast<uint8_t**>(slot + 0x08) = nodes_array;

            sys_heap = *sys_heap_slot;
            auto* ring_buf = static_cast<uint8_t**>(
                (*reinterpret_cast<void* (***)(void*, size_t)>(sys_heap))[10](sys_heap, ring_alloc_sz));
            *reinterpret_cast<uint8_t***>(slot + 0x00) = ring_buf;

            if (ring_buf != nullptr) {
                for (uint32_t t = 0; t < usable_tasks; ++t) {
                    ring_buf = *reinterpret_cast<uint8_t***>(slot + 0x00);
                    ring_buf[t] = *reinterpret_cast<uint8_t**>(slot + 0x08) + static_cast<size_t>(t) * 0x28ULL;
                }
                ring_buf[usable_tasks] = nullptr;
                *reinterpret_cast<uint32_t*>(slot + 0x10) = 0;
                *reinterpret_cast<uint32_t*>(slot + 0x14) = usable_tasks;
                *reinterpret_cast<uint32_t*>(slot + 0x18) = static_cast<uint32_t>(tasks_per_pool);
            } else {
                uint8_t* n_arr = *reinterpret_cast<uint8_t**>(slot + 0x08);
                if (n_arr != nullptr) {
                    void* owner_h = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(n_arr);
                    auto* oh_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_h);
                    oh_vtbl[14](owner_h, *reinterpret_cast<void**>(n_arr - 0x10));
                    *reinterpret_cast<uint64_t*>(slot + 0x08) = 0;
                }
                void* r_ptr = *reinterpret_cast<void**>(slot + 0x00);
                if (r_ptr != nullptr) {
                    uint8_t* fs_base = gx_read_fs_base();
                    auto* tls_blk = reinterpret_cast<uint8_t*>(
                        fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e44a8ULL)));
                    void* free_h = *reinterpret_cast<void**>(tls_blk + 0x10);
                    *reinterpret_cast<uint64_t*>(tls_blk + 0x10) = 0;
                    if (free_h == nullptr) {
                        free_h = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(r_ptr);
                        if (free_h == nullptr) {
                            reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                                reinterpret_cast<const char*>(GX_VMA(0x493bac3ULL)),
                                0x64,
                                reinterpret_cast<const char*>(GX_VMA(0x4940207ULL)));
                        }
                    }
                    auto* fh_vtbl = *reinterpret_cast<void (***)(void*, void*)>(free_h);
                    fh_vtbl[14](free_h, r_ptr);
                    *reinterpret_cast<uint64_t*>(slot + 0x00) = 0;
                }
            }
        }
    }

    *reinterpret_cast<uint32_t*>(global_tp + 0x08) = pool_count;
    return true;
}

// ============================================================================
// [37] 0x21b7010 (60 B) - GXInternalTaskPool_CreateRangeTask
// ============================================================================
void* GXInternalTaskPool_CreateRangeTask(void* callback, const void* range_desc, uint32_t worker_id) {
    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    uint8_t* slot_ptr = *reinterpret_cast<uint8_t**>(global_tp + 0x00) + static_cast<size_t>(worker_id) * 0x60ULL;
    auto* task = static_cast<uint8_t*>(GXInternalTaskPool_AllocateTaskNode(slot_ptr));
    const auto* r = static_cast<const uint8_t*>(range_desc);
    *reinterpret_cast<void**>(task + 0x08) = callback;
    *reinterpret_cast<uint32_t*>(task + 0x18) = *reinterpret_cast<const uint32_t*>(r + 0x08);
    *reinterpret_cast<uint64_t*>(task + 0x10) = *reinterpret_cast<const uint64_t*>(r + 0x00);
    return task;
}

// ============================================================================
// [38] 0x21b7000 (1 B) - GXInternalTaskPool_NoOp
// ============================================================================
void GXInternalTaskPool_NoOp() {
    return;
}

// ============================================================================
// [39] 0x21b6fe0 (32 B) - GXRenderSystem_FlushWorkerTaskPoolIfActive
// ============================================================================
void GXRenderSystem_FlushWorkerTaskPoolIfActive() {
    auto* render_mgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (*reinterpret_cast<uint8_t*>(render_mgr + 0x2e0) != 0) {
        GXWorkerTaskPool_FlushAndExecuteAllTasks(render_mgr + 0x278);
    }
}

// ============================================================================
// [40] 0x21b6f10 (196 B) - GXRenderSystem_DispatchParallelRangeTask
// ============================================================================
void GXRenderSystem_DispatchParallelRangeTask(
    void* callback, const void* range_desc, uint32_t max_workers, uint8_t wait_flush_mode) {
    auto** render_mgr_slot = reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    uint8_t* worker_pool = *render_mgr_slot + 0x278ULL;

    auto* global_tp = *reinterpret_cast<uint8_t**>(GX_VMA(0x56a6c28ULL));
    auto* task = static_cast<uint8_t*>(
        GXInternalTaskPool_AllocateTaskNode(*reinterpret_cast<void**>(global_tp + 0x00)));
    const auto* r = static_cast<const uint8_t*>(range_desc);
    *reinterpret_cast<void**>(task + 0x08) = callback;
    *reinterpret_cast<uint32_t*>(task + 0x18) = *reinterpret_cast<const uint32_t*>(r + 0x08);
    *reinterpret_cast<uint64_t*>(task + 0x10) = *reinterpret_cast<const uint64_t*>(r + 0x00);

    uint8_t* mgr = *render_mgr_slot;
    if (*reinterpret_cast<uint8_t*>(mgr + 0x2e0) != 0) {
        GXWorkerTaskPool_FlushAndExecuteAllTasks(mgr + 0x278);
    }

    auto* master_worker = *reinterpret_cast<uint8_t**>(worker_pool + 0x00);
    {
        auto* t_vtbl = *reinterpret_cast<bool (***)(void*, void*)>(task);
        if (!t_vtbl[3](task, *reinterpret_cast<void**>(master_worker + 0x08))) {
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0xe47ab0ULL))(master_worker + 0x18, task);
        }
    }

    GXWorkerTaskPool_WakeWorkers(worker_pool, nullptr, max_workers);
    if (wait_flush_mode != 0) {
        GXWorkerTaskPool_FlushAndExecuteAllTasks(worker_pool);
    } else {
        GXWorkerTaskPool_WaitForIdleWorkers(worker_pool);
    }
}

// ============================================================================
// [41] 0x21b6d20 (484 B) - GXInternalTaskPool_Dtor
// ============================================================================
void GXInternalTaskPool_Dtor(void* self) {
    auto* tp = static_cast<uint8_t*>(self);
    auto* slots_base = *reinterpret_cast<uint8_t**>(tp + 0x00);
    if (slots_base == nullptr) {
        return;
    }

    void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    uint64_t slot_count = *reinterpret_cast<uint64_t*>(slots_base - 0x08);

    for (uint64_t i = 0; i < slot_count; ++i) {
        uint8_t* slot = slots_base + i * 0x60ULL;
        auto* nodes_arr = *reinterpret_cast<uint8_t**>(slot + 0x08);
        if (nodes_arr != nullptr) {
            void* owner_h = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(nodes_arr);
            auto* oh_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_h);
            oh_vtbl[14](owner_h, *reinterpret_cast<void**>(nodes_arr - 0x10));
            *reinterpret_cast<uint64_t*>(slot + 0x08) = 0;
        }

        void* ring_ptr = *reinterpret_cast<void**>(slot + 0x00);
        if (ring_ptr != nullptr) {
            uint8_t* fs_base = gx_read_fs_base();
            auto* tls_blk = reinterpret_cast<uint8_t*>(
                fs_base + static_cast<uint64_t>(gx_guest_tls_offset(0x53e44a8ULL)));
            void* free_h = *reinterpret_cast<void**>(tls_blk + 0x10);
            *reinterpret_cast<uint64_t*>(tls_blk + 0x10) = 0;
            if (free_h == nullptr) {
                free_h = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(ring_ptr);
                if (free_h == nullptr) {
                    reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493bac3ULL)),
                        0x64,
                        reinterpret_cast<const char*>(GX_VMA(0x4940207ULL)));
                }
            }
            auto* fh_vtbl = *reinterpret_cast<void (***)(void*, void*)>(free_h);
            fh_vtbl[14](free_h, ring_ptr);
            *reinterpret_cast<uint64_t*>(slot + 0x00) = 0;
        }

        uint8_t* mtx_obj = slot + 0x20;
        *reinterpret_cast<uint64_t*>(mtx_obj + 0x00) = GX_VMA(0x52b6230ULL);
        if (reinterpret_cast< bool (*)(void*) >(GX_VMA(0x207ee80ULL))(mtx_obj)) {
            uint32_t err = static_cast<uint32_t>(
                reinterpret_cast<FnIntPtr>(GX_VMA(0x2bbfee8ULL))(mtx_obj + 0x08));
            if (err != 0x80020010u) {
                if (err != 0) {
                    if (err == 0x80020016u) {
                        reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                            reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)),
                            0x55,
                            reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                    }
                    reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)),
                        0x57,
                        reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
                }
                mtx_obj[0x10] = 0;
            }
        }
        *reinterpret_cast<uint64_t*>(mtx_obj + 0x00) = GX_VMA(0x53b2fa0ULL);
        if (mtx_obj[0x10] != 0) {
            uint32_t err = static_cast<uint32_t>(
                reinterpret_cast<FnIntPtr>(GX_VMA(0x2bbfee8ULL))(mtx_obj + 0x08));
            if (err != 0x80020010u) {
                if (err != 0) {
                    if (err == 0x80020016u) {
                        reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                            reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)),
                            0x55,
                            reinterpret_cast<const char*>(GX_VMA(0x493c275ULL)));
                    }
                    reinterpret_cast<void (*)(const char*, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(
                        reinterpret_cast<const char*>(GX_VMA(0x493c20eULL)),
                        0x57,
                        reinterpret_cast<const char*>(GX_VMA(0x493c2aeULL)));
                }
                mtx_obj[0x10] = 0;
            }
        }
    }

    auto* sh_vtbl = *reinterpret_cast<void (***)(void*, void*)>(sys_heap);
    sh_vtbl[14](sys_heap, *reinterpret_cast<void**>(slots_base - 0x10));
    *reinterpret_cast<uint64_t*>(tp + 0x00) = 0;
}

// ============================================================================
// [42] 0x21b6be0 (175 B) - GXRenderThread_CreateAndStart
// ============================================================================
void* GXRenderThread_CreateAndStart(
    void* arg1, void* owner, uint64_t* out_ctx_handle,
    uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3) {
    uint64_t init_params[4] = {p0, p1, p2, p3};
    void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    auto* raw_rt = static_cast<uint8_t*>(
        (*reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap))[11](sys_heap, 0xa0ULL, 0x08ULL));
    if (raw_rt == nullptr) {
        return nullptr;
    }

    GXRenderThread_Ctor(raw_rt, arg1, owner, init_params);
    uint8_t* ev_ready = raw_rt + 0x68;
    {
        auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev_ready);
        e_vtbl[3](ev_ready, 0xFFFFFFFFu);
    }
    *out_ctx_handle = *reinterpret_cast<uint64_t*>(raw_rt + 0x98);
    return raw_rt;
}

// ============================================================================
// [43] 0x21b6aa0 (305 B) - GXRenderThread_SyncAndSubmitFrame
// ============================================================================
void GXRenderThread_SyncAndSubmitFrame(
    void* self, uint8_t skip_kick, uint8_t sync_vsync, void* frame_callback) {
    auto* rt = static_cast<uint8_t*>(self);
    auto* owner = *reinterpret_cast<uint8_t**>(rt + 0x88);

    if (*reinterpret_cast<uint8_t*>(owner + 0x72) != 0) {
        if (sync_vsync != 0) {
            uint8_t* ev68 = rt + 0x68;
            auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
            e_vtbl[3](ev68, 0xFFFFFFFFu);
            *reinterpret_cast<uint8_t*>(rt + 0x11) = 1;
        }
        if (skip_kick != 0) {
            return;
        }
        uint8_t* ev68 = rt + 0x68;
        {
            auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
            e_vtbl[3](ev68, 0xFFFFFFFFu);
        }
        *reinterpret_cast<void**>(rt + 0x20) = frame_callback;
        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev68, 0);
        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(rt + 0x28, 1);
        return;
    }

    if (skip_kick != 0) {
        uint8_t* ev68 = rt + 0x68;
        auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
        e_vtbl[3](ev68, 0xFFFFFFFFu);
    }

    if (sync_vsync != 0) {
        auto* render_mgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
        owner = *reinterpret_cast<uint8_t**>(rt + 0x88);
        uint8_t vsync_val = *reinterpret_cast<uint8_t*>(owner + 0x44);
        *reinterpret_cast<uint8_t*>(render_mgr + 0x104) = vsync_val;

        auto* cur_it = *reinterpret_cast<uint8_t**>(render_mgr + 0x90);
        auto* end_it = *reinterpret_cast<uint8_t**>(render_mgr + 0x98);
        while (cur_it != end_it) {
            auto* sub_obj = *reinterpret_cast<uint8_t**>(cur_it + 0x08);
            *reinterpret_cast<uint8_t*>(sub_obj + 0x08) = vsync_val;
            cur_it += 0xb0ULL;
        }
        *reinterpret_cast<uint8_t*>(GX_VMA(0x513ac84ULL)) = vsync_val;
    }

    if (frame_callback != nullptr) {
        *reinterpret_cast<void**>(rt + 0x20) = frame_callback;
        uint8_t* ev68 = rt + 0x68;
        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev68, 0);
        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(rt + 0x28, 1);
        auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
        e_vtbl[3](ev68, 0xFFFFFFFFu);
    }
}

// ============================================================================
// [44] 0x21b6910 (399 B) - GXRenderThread_ThreadMainLoop
// ============================================================================
void GXRenderThread_ThreadMainLoop(void* self) {
    auto* rt = static_cast<uint8_t*>(self);
    auto* params = *reinterpret_cast<const uint64_t**>(rt + 0x90);
    reinterpret_cast<void (*)(void*, uint64_t, uint64_t, uint64_t, uint64_t)>(GX_VMA(0x2165300ULL))(
        rt + 0x98, params[0], params[1], params[2], params[3]);
    *reinterpret_cast<uint64_t*>(rt + 0x90) = 0;

    uint8_t* ev68 = rt + 0x68;
    uint8_t* ev28 = rt + 0x28;
    uint8_t* ev48 = rt + 0x48;

    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev68, 1);
    {
        auto* e28_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev28);
        e28_vtbl[3](ev28, 0xFFFFFFFFu);
    }
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev28, 0);

    while (*reinterpret_cast<uint8_t*>(rt + 0x10) == 0) {
        if (*reinterpret_cast<uint8_t*>(GX_VMA(0x55406c0ULL)) != 0) {
            reinterpret_cast< void (*)(uint32_t) >(GX_VMA(0x2bbfe68ULL))(0x9c40u);
        }

        if (*reinterpret_cast<uint64_t*>(rt + 0x18) != 0) {
            reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev48, 0);
            void* render_mgr = *reinterpret_cast<void**>(GX_VMA(0x55406c8ULL));
            reinterpret_cast<void (*)(void*, void*)>(GX_VMA(0x2199af0ULL))(
                render_mgr, *reinterpret_cast<void**>(rt + 0x18));
            *reinterpret_cast<uint64_t*>(rt + 0x18) = 0;
            reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev48, 1);
        }

        void* cb = *reinterpret_cast<void**>(rt + 0x20);
        if (cb != nullptr) {
            if (*reinterpret_cast<uint8_t*>(rt + 0x11) != 0) {
                auto* render_mgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
                auto* owner = *reinterpret_cast<uint8_t**>(rt + 0x88);
                uint8_t vsync_val = *reinterpret_cast<uint8_t*>(owner + 0x44);
                *reinterpret_cast<uint8_t*>(render_mgr + 0x104) = vsync_val;

                auto* cur_it = *reinterpret_cast<uint8_t**>(render_mgr + 0x90);
                auto* end_it = *reinterpret_cast<uint8_t**>(render_mgr + 0x98);
                while (cur_it != end_it) {
                    auto* sub_obj = *reinterpret_cast<uint8_t**>(cur_it + 0x08);
                    *reinterpret_cast<uint8_t*>(sub_obj + 0x08) = vsync_val;
                    cur_it += 0xb0ULL;
                }
                *reinterpret_cast<uint8_t*>(GX_VMA(0x513ac84ULL)) = vsync_val;
            }

            auto* cb_vtbl = *reinterpret_cast<FnVoidPtrU32**>(cb);
            auto* owner = *reinterpret_cast<uint8_t**>(rt + 0x88);
            cb_vtbl[0](cb, *reinterpret_cast<uint32_t*>(owner + 0x40));
            *reinterpret_cast<uint64_t*>(rt + 0x20) = 0;
        }

        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev68, 1);
        {
            auto* e28_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev28);
            e28_vtbl[3](ev28, 0xFFFFFFFFu);
        }
        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev28, 0);
    }
}

// ============================================================================
// [45] 0x21b68f0 (22 B) - GXRenderThread_TryWaitFrameComplete
// ============================================================================
bool GXRenderThread_TryWaitFrameComplete(void* self) {
    auto* ev68 = static_cast<uint8_t*>(self) + 0x68;
    auto* vtbl = *reinterpret_cast<FnIntPtr**>(ev68);
    return vtbl[4](ev68) != 0;
}

// ============================================================================
// [46] 0x21b68d0 (19 B) - GXRenderThread_WaitFrameComplete
// ============================================================================
void GXRenderThread_WaitFrameComplete(void* self) {
    auto* ev68 = static_cast<uint8_t*>(self) + 0x68;
    auto* vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
    vtbl[3](ev68, 0xFFFFFFFFu);
}

// ============================================================================
// [47] 0x21b6850 (127 B) - GXRenderThread_KickCommandBuffer
// ============================================================================
void GXRenderThread_KickCommandBuffer(void* self, void* cmd_buf, void* callback) {
    auto* rt = static_cast<uint8_t*>(self);
    uint8_t* ev68 = rt + 0x68;
    {
        auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
        e_vtbl[3](ev68, 0xFFFFFFFFu);
    }

    *reinterpret_cast<void**>(rt + 0x18) = cmd_buf;
    auto* owner = *reinterpret_cast<uint8_t**>(rt + 0x88);
    if (*reinterpret_cast<uint8_t*>(owner + 0x72) != 0) {
        *reinterpret_cast<void**>(rt + 0x20) = callback;
    }

    auto* render_mgr = *reinterpret_cast<uint8_t**>(GX_VMA(0x55406c8ULL));
    if (*reinterpret_cast<uint64_t*>(render_mgr + 0x640) != 0) {
        *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cbaULL)) = *reinterpret_cast<uint8_t*>(GX_VMA(0x56a6cb9ULL));
    }

    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(ev68, 0);
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(rt + 0x28, 1);
}

// ============================================================================
// [48] 0x21b6770 (224 B) - GXRenderThread_Dtor
// ============================================================================
void GXRenderThread_Dtor(void* self) {
    auto* rt = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(rt + 0x00) = GX_VMA(0x535fbc0ULL);

    if (*reinterpret_cast<uint64_t*>(rt + 0x08) != 0) {
        uint8_t* ev68 = rt + 0x68;
        {
            auto* e_vtbl = *reinterpret_cast<FnVoidPtrU32**>(ev68);
            e_vtbl[3](ev68, 0xFFFFFFFFu);
        }
        *reinterpret_cast<uint8_t*>(rt + 0x10) = 1;
        reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(rt + 0x28, 1);

        auto* thr = *reinterpret_cast<uint8_t**>(rt + 0x08);
        reinterpret_cast<void (*)(void*, uint32_t)>(GX_VMA(0x263b6a0ULL))(thr + 0x08, 0xFFFFFFFFu);
        thr = *reinterpret_cast<uint8_t**>(rt + 0x08);
        if (thr != nullptr) {
            void* owner_h = reinterpret_cast<FnPtrPtr>(GX_VMA(0x207b720ULL))(thr);
            auto* thr_vtbl = *reinterpret_cast<FnVoidPtr**>(thr);
            thr_vtbl[1](thr);
            auto* h_vtbl = *reinterpret_cast<void (***)(void*, void*)>(owner_h);
            h_vtbl[14](owner_h, thr);
            *reinterpret_cast<uint64_t*>(rt + 0x08) = 0;
        }
    }

    auto destroy_ev = [](uint8_t* ev) {
        *reinterpret_cast<uint64_t*>(ev) = GX_VMA(0x52b97a0ULL);
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x207e5c0ULL))(ev);
        reinterpret_cast<FnVoidPtr>(GX_VMA(0x207e4f0ULL))(ev);
    };

    destroy_ev(rt + 0x68);
    destroy_ev(rt + 0x48);
    destroy_ev(rt + 0x28);
}

// ============================================================================
// [49] 0x21b6750 (28 B) - GXRenderThread_DeletingDtor
// ============================================================================
void GXRenderThread_DeletingDtor(void* self) {
    GXRenderThread_Dtor(self);
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x2bbe588ULL))(self);
}

// ============================================================================
// [50] 0x21b6620 (304 B) - GXRenderThread_Ctor
// ============================================================================
void GXRenderThread_Ctor(void* self, void* arg1, void* owner, void* init_params) {
    auto* rt = static_cast<uint8_t*>(self);
    *reinterpret_cast<uint64_t*>(rt + 0x00) = GX_VMA(0x535fbc0ULL);
    *reinterpret_cast<uint64_t*>(rt + 0x08) = 0;
    *reinterpret_cast<uint16_t*>(rt + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(rt + 0x20) = 0;
    *reinterpret_cast<uint64_t*>(rt + 0x18) = 0;

    *reinterpret_cast<uint32_t*>(rt + 0x40) = 0;
    *reinterpret_cast<uint64_t*>(rt + 0x28) = GX_VMA(0x52b97a0ULL);
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e690ULL))(rt + 0x28, 0);

    *reinterpret_cast<uint32_t*>(rt + 0x60) = 0;
    *reinterpret_cast<uint64_t*>(rt + 0x48) = GX_VMA(0x52b97a0ULL);
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e690ULL))(rt + 0x48, 0);

    *reinterpret_cast<uint32_t*>(rt + 0x80) = 0;
    *reinterpret_cast<uint64_t*>(rt + 0x68) = GX_VMA(0x52b97a0ULL);
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e690ULL))(rt + 0x68, 0);

    *reinterpret_cast<void**>(rt + 0x88) = owner;
    *reinterpret_cast<void**>(rt + 0x90) = init_params;
    *reinterpret_cast<uint64_t*>(rt + 0x98) = 0;

    void* sys_heap = *reinterpret_cast<void**>(GX_VMA(0x5540680ULL));
    void* thr_obj = (*reinterpret_cast<void* (***)(void*, size_t, size_t)>(sys_heap))[11](
        sys_heap, 0xc8ULL, 0x08ULL);
    if (thr_obj != nullptr) {
        reinterpret_cast<void (*)(void*, void*, uint32_t, void*, const char*, uint32_t)>(GX_VMA(0x208afe0ULL))(
            thr_obj,
            rt,
            0x20000u,
            arg1,
            reinterpret_cast<const char*>(GX_VMA(0x494022cULL)), // "GXRenderThread"
            0);
    }

    *reinterpret_cast<void**>(rt + 0x08) = thr_obj;
    reinterpret_cast<FnVoidPtr>(GX_VMA(0x208b9e0ULL))(thr_obj);
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(rt + 0x28, 0);
    reinterpret_cast<FnVoidPtrU32>(GX_VMA(0x207e780ULL))(rt + 0x48, 1);
}

// ============================================================================
// [51] 0x21b6420 (511 B) - GXSortedRenderItemBuffer_MergeTwoSortedRuns
// ============================================================================
void* GXSortedRenderItemBuffer_MergeTwoSortedRuns(
    void* out_result,
    const uint8_t* first1,
    const uint8_t* last1,
    const uint8_t* first2,
    const uint8_t* last2,
    uint8_t* dst_container_holder) {
    auto* dst_vec = *reinterpret_cast<uint8_t**>(dst_container_holder + 0x20);
    bool run2_empty = (first2 == last2);

    auto push_item_from_run1 = [&](const uint8_t* item) {
        auto* cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
        auto* end = *reinterpret_cast<uint8_t**>(dst_vec + 0x10);
        if (cur < end) {
            *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = cur + 0x10;
            *reinterpret_cast<uint32_t*>(cur + 0x08) = *reinterpret_cast<const uint32_t*>(item + 0x08);
            *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(item + 0x00);
        } else {
            uint8_t* next_cur = nullptr;
            if (cur != nullptr) {
                *reinterpret_cast<uint64_t*>(cur + 0x08) = *reinterpret_cast<const uint64_t*>(item + 0x08);
                *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(item + 0x00);
                dst_vec = *reinterpret_cast<uint8_t**>(dst_container_holder + 0x20);
                next_cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
            }
            next_cur += 0x10;
            *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = next_cur;
            *reinterpret_cast<uint8_t**>(dst_vec + 0x10) = next_cur;
        }
    };

    auto push_item_from_run2 = [&](const uint8_t* item) {
        auto* cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
        auto* end = *reinterpret_cast<uint8_t**>(dst_vec + 0x10);
        if (cur < end) {
            *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = cur + 0x10;
            *reinterpret_cast<uint32_t*>(cur + 0x08) = *reinterpret_cast<const uint32_t*>(item + 0x08);
            *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(item + 0x00);
        } else {
            uint8_t* next_cur = nullptr;
            if (cur != nullptr) {
                *reinterpret_cast<uint64_t*>(cur + 0x08) = *reinterpret_cast<const uint64_t*>(item + 0x08);
                *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(item + 0x00);
                dst_vec = *reinterpret_cast<uint8_t**>(dst_container_holder + 0x20);
                next_cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
            }
            next_cur += 0x10;
            *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = next_cur;
            *reinterpret_cast<uint8_t**>(dst_vec + 0x10) = next_cur;
        }
    };

    if (first1 != last1 && first2 != last2) {
        for (;;) {
            uint32_t key2 = *reinterpret_cast<const uint32_t*>(first2 + 0x08);
            uint32_t key1 = *reinterpret_cast<const uint32_t*>(first1 + 0x08);
            if (key2 >= key1) {
                push_item_from_run1(first1);
                first1 += 0x10;
                if (first1 == last1) {
                    *reinterpret_cast<uint8_t**>(dst_container_holder + 0x20) = dst_vec;
                    run2_empty = false;
                    break;
                }
            } else {
                push_item_from_run2(first2);
                first2 += 0x10;
                run2_empty = (first2 == last2);
                if (first1 == last1 || run2_empty) {
                    break;
                }
            }
        }
    }

    if (first1 != last1) {
        while (first1 != last1) {
            auto* cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
            auto* end = *reinterpret_cast<uint8_t**>(dst_vec + 0x10);
            if (cur < end) {
                *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = cur + 0x10;
                *reinterpret_cast<uint32_t*>(cur + 0x08) = *reinterpret_cast<const uint32_t*>(first1 + 0x08);
                *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(first1 + 0x00);
            } else {
                uint8_t* next_cur = nullptr;
                if (cur != nullptr) {
                    *reinterpret_cast<uint64_t*>(cur + 0x08) = *reinterpret_cast<const uint64_t*>(first1 + 0x08);
                    *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(first1 + 0x00);
                    next_cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
                }
                next_cur += 0x10;
                *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = next_cur;
                *reinterpret_cast<uint8_t**>(dst_vec + 0x10) = next_cur;
            }
            first1 += 0x10;
        }
    }

    *reinterpret_cast<uint8_t**>(dst_container_holder + 0x20) = dst_vec;

    if (!run2_empty) {
        while (first2 != last2) {
            auto* cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
            auto* end = *reinterpret_cast<uint8_t**>(dst_vec + 0x10);
            if (cur < end) {
                *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = cur + 0x10;
                *reinterpret_cast<uint32_t*>(cur + 0x08) = *reinterpret_cast<const uint32_t*>(first2 + 0x08);
                *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(first2 + 0x00);
            } else {
                uint8_t* next_cur = nullptr;
                if (cur != nullptr) {
                    *reinterpret_cast<uint64_t*>(cur + 0x08) = *reinterpret_cast<const uint64_t*>(first2 + 0x08);
                    *reinterpret_cast<uint64_t*>(cur + 0x00) = *reinterpret_cast<const uint64_t*>(first2 + 0x00);
                    next_cur = *reinterpret_cast<uint8_t**>(dst_vec + 0x08);
                }
                next_cur += 0x10;
                *reinterpret_cast<uint8_t**>(dst_vec + 0x08) = next_cur;
                *reinterpret_cast<uint8_t**>(dst_vec + 0x10) = next_cur;
            }
            first2 += 0x10;
        }
    }

    auto* out = static_cast<uint8_t*>(out_result);
    *reinterpret_cast<uint64_t*>(out + 0x18) = 0;
    *reinterpret_cast<uint64_t*>(out + 0x10) = 0;
    *reinterpret_cast<uint64_t*>(out + 0x08) = 0;
    *reinterpret_cast<uint64_t*>(out + 0x00) = 0;
    *reinterpret_cast<uint8_t**>(out + 0x20) = dst_vec;
    return out_result;
}

} // extern "C"
