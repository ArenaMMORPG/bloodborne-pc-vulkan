// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: Universal Hardware-Adaptive CPU Topology & Thread Affinity Scheduler.
// Automatically detects Multi-CCD / Multi-L3-Domain CPUs (AMD Ryzen 9 / Threadripper, including
// asymmetric 3D V-Cache X3D), Hybrid P-Core + E-Core CPUs (Intel 12th-15th Gen), Single-CCD
// Homogeneous CPUs (Ryzen 5/7, X3D, Intel 6c/8c), and Compact Handhelds (Steam Deck 4c/8t).

#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <mutex>
#include <thread>
#ifndef _WIN32
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace BbThreads {

/// Hardware threads available to the process.
inline unsigned Available() {
#ifdef _WIN32
    return bb_win32_available_cpus();
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == 0) {
        return std::max(1, CPU_COUNT(&set));
    }
    return std::max(1u, std::thread::hardware_concurrency());
#endif
}

/// The calling thread only runs on otherwise idle cores (falls back to the lowest nice level).
inline void MakeBackground() {
#ifdef _WIN32
    bb_win32_background_thread();
#else
    sched_param param{};
    if (sched_setscheduler(0, SCHED_IDLE, &param) != 0) {
        setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), 19);
    }
#endif
}

/// Low-priority SCHED_OTHER helper on >=16-thread CPUs (never starved like SCHED_IDLE, while
/// yielding gracefully to nice-0 game and GPU threads); falls back to MakeBackground() on smaller CPUs.
inline void MakeLowPriority() {
#ifdef _WIN32
    bb_win32_background_thread();
#else
    if (Available() >= 16) {
        setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), 10);
    } else {
        MakeBackground();
    }
#endif
}

/// Helper threads of the GPU side that the GPU command thread's progress depends on (Vulkan
/// recording threads, guest memory copy threads): a fault there must never wait for that thread
/// (a read trap's copy back goes through it for the game's threads). Set once per thread.
inline thread_local bool gpu_side_helper = false;

#ifndef _WIN32
struct TopologyMasks {
    cpu_set_t game_main;    // MOMainThread (dedicated top physical core when >= 6 phys cores)
    cpu_set_t game_workers; // HavokWorker*, CSChrThread*, CSClothThread*, SpCloth*, GXRender*, GXWorker*
    cpu_set_t gpu;          // bb:GpuComm (Stage A), bb:DrawRec (Stage B), bb:VkRecorder*, bb:Present
    cpu_set_t helper;       // bb:Resolve*, bb:Copy*, bb:DrawScan, EzWorkPool*, Core.Res.*, FMOD*
    int best_main_cpu = -1;
    int best_main_sibling = -1;
    bool enabled = false;
};

inline void ParseCpuList(const char* text, const cpu_set_t& allowed, cpu_set_t* out) {
    CPU_ZERO(out);
    for (const char* at = text; *at && *at != '\n';) {
        char* end = nullptr;
        const long first = std::strtol(at, &end, 10);
        if (end == at) {
            break;
        }
        long last = first;
        if (*end == '-') {
            last = std::strtol(end + 1, &end, 10);
        }
        for (long c = first; c <= last && c < CPU_SETSIZE; ++c) {
            if (c >= 0 && CPU_ISSET(c, &allowed)) {
                CPU_SET(c, out);
            }
        }
        at = (*end == ',') ? (end + 1) : end;
    }
}

inline void FormatCpuList(const cpu_set_t& set, char* out, std::size_t out_size) {
    if (out_size == 0) {
        return;
    }
    out[0] = '\0';
    std::size_t pos = 0;
    int first = -1;
    int prev = -1;
    const auto flush = [&](int a, int b) {
        if (a < 0) {
            return;
        }
        int n = 0;
        if (a == b) {
            n = std::snprintf(out + pos, out_size - pos, "%s%d", pos ? "," : "", a);
        } else {
            n = std::snprintf(out + pos, out_size - pos, "%s%d-%d", pos ? "," : "", a, b);
        }
        if (n > 0 && pos + std::size_t(n) < out_size) {
            pos += std::size_t(n);
        }
    };
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &set)) {
            if (first < 0) {
                first = prev = cpu;
            } else if (cpu == prev + 1) {
                prev = cpu;
            } else {
                flush(first, prev);
                first = prev = cpu;
            }
        }
    }
    flush(first, prev);
}

inline const TopologyMasks& GetTopology() {
    static const TopologyMasks topo = [] {
        TopologyMasks none{};
        CPU_ZERO(&none.game_main);
        CPU_ZERO(&none.game_workers);
        CPU_ZERO(&none.gpu);
        CPU_ZERO(&none.helper);

        const char* env_ccd = std::getenv("BB_GPU_CCD");
        const char* env_topo = std::getenv("BB_THREAD_TOPOLOGY");
        if ((env_ccd && (env_ccd[0] == 'o' || env_ccd[0] == '-')) ||
            (env_topo && (env_topo[0] == '0' || env_topo[0] == 'o' || env_topo[0] == '-'))) {
            return none;
        }

        cpu_set_t allowed;
        CPU_ZERO(&allowed);
        if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0 || CPU_COUNT(&allowed) <= 1) {
            return none;
        }

        const auto read_text = [](const char* path, char* out, std::size_t size) {
            FILE* file = std::fopen(path, "r");
            if (!file) {
                return false;
            }
            const bool ok = std::fgets(out, int(size), file) != nullptr;
            std::fclose(file);
            return ok;
        };

        struct CpuInfo {
            bool valid = false;
            int primary_sibling = -1;
            cpu_set_t siblings{};
            cpu_set_t l3_set{};
            long l3_size_kb = 0;
            long pref_rank = 0;
            long capacity = 0;
            long max_freq = 0;
            long score = 0;
        };

        CpuInfo cpus[256]{};
        struct Domain {
            cpu_set_t cpus{};
            long l3_size_kb = 0;
            long best_score = 0;
            int phys_cores = 0;
        };
        Domain domains[16]{};
        int domain_count = 0;
        int total_phys_cores = 0;
        long max_score = 0;
        long min_score = 0x7fffffffL;

        for (int cpu = 0; cpu < 256 && cpu < CPU_SETSIZE; ++cpu) {
            if (!CPU_ISSET(cpu, &allowed)) {
                continue;
            }
            CpuInfo& info = cpus[cpu];
            info.valid = true;
            CPU_ZERO(&info.siblings);
            CPU_SET(cpu, &info.siblings);
            info.primary_sibling = cpu;

            char path[160], text[256];
            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", cpu);
            if (read_text(path, text, sizeof(text))) {
                ParseCpuList(text, allowed, &info.siblings);
                for (int s = 0; s < 256; ++s) {
                    if (CPU_ISSET(s, &info.siblings)) {
                        info.primary_sibling = s;
                        break;
                    }
                }
            }
            if (info.primary_sibling == cpu) {
                ++total_phys_cores;
            }

            CPU_ZERO(&info.l3_set);
            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cache/index3/shared_cpu_list", cpu);
            if (read_text(path, text, sizeof(text))) {
                ParseCpuList(text, allowed, &info.l3_set);
            } else {
                info.l3_set = allowed;
            }

            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cache/index3/size", cpu);
            if (read_text(path, text, sizeof(text))) {
                info.l3_size_kb = std::atol(text);
            }

            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cpufreq/amd_pstate_prefcore_ranking", cpu);
            if (read_text(path, text, sizeof(text))) {
                info.pref_rank = std::atol(text);
            }

            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cpu_capacity", cpu);
            if (read_text(path, text, sizeof(text))) {
                info.capacity = std::atol(text);
            }

            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);
            if (read_text(path, text, sizeof(text))) {
                info.max_freq = std::atol(text) / 1000; // MHz
            }

            // Unified performance score for both AMD prefcore and Intel hybrid P/E capacity/freq.
            info.score = info.pref_rank * 10000L + info.capacity * 100L + info.max_freq;
            if (info.score == 0) {
                info.score = 1000L;
            }
            max_score = std::max(max_score, info.score);
            min_score = std::min(min_score, info.score);

            int found = -1;
            for (int d = 0; d < domain_count; ++d) {
                if (CPU_EQUAL(&domains[d].cpus, &info.l3_set)) {
                    found = d;
                    break;
                }
            }
            if (found < 0 && domain_count < 16) {
                found = domain_count++;
                domains[found].cpus = info.l3_set;
                domains[found].l3_size_kb = info.l3_size_kb;
                domains[found].best_score = info.score;
                domains[found].phys_cores = (info.primary_sibling == cpu) ? 1 : 0;
            } else if (found >= 0) {
                domains[found].l3_size_kb = std::max(domains[found].l3_size_kb, info.l3_size_kb);
                domains[found].best_score = std::max(domains[found].best_score, info.score);
                if (info.primary_sibling == cpu) {
                    ++domains[found].phys_cores;
                }
            }
        }

        if (domain_count == 0) {
            return none;
        }

        TopologyMasks result{};
        result.enabled = true;

        // Helper to pick the #1 physical core inside a given domain mask and optionally
        // carve out both its primary thread and SMT sibling from worker masks.
        const auto pick_best_core = [&](const cpu_set_t& pool, int* out_primary, int* out_sibling) {
            int best = -1;
            long best_sc = -1;
            for (int c = 0; c < 256; ++c) {
                if (cpus[c].valid && CPU_ISSET(c, &pool) && cpus[c].primary_sibling == c) {
                    if (cpus[c].score > best_sc) {
                        best_sc = cpus[c].score;
                        best = c;
                    }
                }
            }
            if (best < 0) {
                for (int c = 0; c < 256; ++c) {
                    if (cpus[c].valid && CPU_ISSET(c, &pool)) {
                        best = c;
                        break;
                    }
                }
            }
            *out_primary = best;
            *out_sibling = -1;
            if (best >= 0) {
                for (int s = 0; s < 256; ++s) {
                    if (s != best && CPU_ISSET(s, &cpus[best].siblings)) {
                        *out_sibling = s;
                        break;
                    }
                }
            }
        };

        if (domain_count >= 2) {
            // =================================================================================
            // CASE 1: Multi-CCD / Multi-L3-Domain CPU (e.g. Ryzen 9 5900X/5950X/7900X/7950X/
            // 7950X3D/9900X/9950X/9950X3D, Threadripper).
            // - Primary L3 domain (largest L3 V-Cache first, then highest preferred-core score)
            //   is dedicated to the Guest Game Engine:
            //   * MOMainThread gets its own #1 physical core (with its SMT sibling left empty
            //     so MOMainThread has 100% of ALU/FPU/L1/L2 with zero SMT contention).
            //   * HavokWorkerThre (x5), CSChrThread0..5 (x6), CSClothThread0..5 (x6),
            //     SpClothVertexUp (x6), GXRenderThread, and GXWorker:1..5 (x5) share the
            //     remaining cores of the EXACT SAME L3 domain (~4 ns L3 hit, 0 Infinity Fabric
            //     crossings across all ~10,000 per-frame Havok/character fork-join barriers).
            // - Secondary L3 domain (CCD1) is dedicated to the Host Vulkan RHI Pipeline
            //   (bb:GpuComm, bb:DrawRec, bb:VkRecorder*, bb:Resolve*, bb:DrawScan, bb:Copy*)
            //   plus background guest I/O & audio threads (EzWorkPool*, Core.Res.*, FMOD*).
            // =================================================================================
            int game_dom = 0;
            for (int d = 1; d < domain_count; ++d) {
                const bool larger_l3 = domains[d].l3_size_kb > domains[game_dom].l3_size_kb;
                const bool same_l3_faster =
                    (domains[d].l3_size_kb == domains[game_dom].l3_size_kb) &&
                    (domains[d].best_score > domains[game_dom].best_score);
                if (larger_l3 || same_l3_faster) {
                    game_dom = d;
                }
            }
            int gpu_dom = (game_dom + 1) % domain_count;
            if (env_ccd && env_ccd[0] >= '0' && env_ccd[0] <= '9') {
                gpu_dom = std::clamp(std::atoi(env_ccd), 0, domain_count - 1);
                if (gpu_dom == game_dom && domain_count >= 2) {
                    game_dom = (gpu_dom + 1) % domain_count;
                }
            }

            const cpu_set_t& game_pool = domains[game_dom].cpus;
            const cpu_set_t& gpu_pool = domains[gpu_dom].cpus;

            pick_best_core(game_pool, &result.best_main_cpu, &result.best_main_sibling);
            result.game_workers = game_pool;
            if (domains[game_dom].phys_cores >= 6 && result.best_main_cpu >= 0) {
                CPU_ZERO(&result.game_main);
                CPU_SET(result.best_main_cpu, &result.game_main);
                // Isolate MOMainThread's physical core (and its SMT sibling) from workers!
                for (int s = 0; s < 256; ++s) {
                    if (CPU_ISSET(s, &cpus[result.best_main_cpu].siblings)) {
                        CPU_CLR(s, &result.game_workers);
                    }
                }
            } else {
                result.game_main = game_pool;
            }
            result.gpu = gpu_pool;
            result.helper = gpu_pool;
        } else {
            // Single L3 domain: check if Hybrid P-Core + E-Core (Intel 12th-15th Gen) or
            // Homogeneous (Ryzen 5/7, 5800X3D/7800X3D/9800X3D, Intel 6c/8c, Steam Deck).
            bool is_hybrid = false;
            cpu_set_t p_cores{}, e_cores{};
            CPU_ZERO(&p_cores);
            CPU_ZERO(&e_cores);
            int p_phys_cores = 0;

            if (min_score > 0 && max_score > (min_score * 115L) / 100L) {
                const long threshold = (max_score * 85L) / 100L;
                for (int c = 0; c < 256; ++c) {
                    if (!cpus[c].valid) {
                        continue;
                    }
                    if (cpus[c].score >= threshold) {
                        CPU_SET(c, &p_cores);
                        if (cpus[c].primary_sibling == c) {
                            ++p_phys_cores;
                        }
                    } else {
                        CPU_SET(c, &e_cores);
                    }
                }
                if (CPU_COUNT(&p_cores) >= 4 && CPU_COUNT(&e_cores) >= 2) {
                    is_hybrid = true;
                }
            }

            if (is_hybrid) {
                // =============================================================================
                // CASE 2: Single-L3 Hybrid P-Core + E-Core CPU (Intel 12th-15th Gen).
                // - MOMainThread gets the #1 P-core (isolated from its SMT sibling when >=6P).
                // - Havok/CSChr/GX workers + Vulkan Stage A/B stay strictly on P-cores so a
                //   slow E-core never stalls a Havok physics barrier.
                // - Async helpers, file decompression, and FMOD audio run on E-cores.
                // =============================================================================
                pick_best_core(p_cores, &result.best_main_cpu, &result.best_main_sibling);
                result.game_workers = p_cores;
                if (p_phys_cores >= 6 && result.best_main_cpu >= 0) {
                    CPU_ZERO(&result.game_main);
                    CPU_SET(result.best_main_cpu, &result.game_main);
                    for (int s = 0; s < 256; ++s) {
                        if (CPU_ISSET(s, &cpus[result.best_main_cpu].siblings)) {
                            CPU_CLR(s, &result.game_workers);
                        }
                    }
                } else {
                    result.game_main = p_cores;
                }
                result.gpu = result.game_workers;
                result.helper = e_cores;
            } else if (total_phys_cores >= 6) {
                // =============================================================================
                // CASE 3: Single-L3 Homogeneous CPU with >= 6 Physical Cores
                // (e.g. Ryzen 5 5600X/7600X, Ryzen 7 5800X3D/7800X3D/9800X3D, Intel 6c/8c).
                // - MOMainThread gets a dedicated top physical core (with its SMT sibling
                //   excluded from all worker/GPU masks to prevent SMT contention).
                // - All remaining physical cores run Havok/CSChr/GX workers and Vulkan threads.
                // =============================================================================
                pick_best_core(allowed, &result.best_main_cpu, &result.best_main_sibling);
                result.game_workers = allowed;
                if (result.best_main_cpu >= 0) {
                    CPU_ZERO(&result.game_main);
                    CPU_SET(result.best_main_cpu, &result.game_main);
                    for (int s = 0; s < 256; ++s) {
                        if (CPU_ISSET(s, &cpus[result.best_main_cpu].siblings)) {
                            CPU_CLR(s, &result.game_workers);
                        }
                    }
                } else {
                    result.game_main = allowed;
                }
                result.gpu = result.game_workers;
                result.helper = result.game_workers;
            } else {
                // =============================================================================
                // CASE 4: Compact CPU (< 6 physical cores, e.g. Steam Deck 4c/8t).
                // Keep all cores shared and rely on thread priorities (MakeLowPriority/IDLE).
                // =============================================================================
                pick_best_core(allowed, &result.best_main_cpu, &result.best_main_sibling);
                result.game_main = allowed;
                result.game_workers = allowed;
                result.gpu = allowed;
                result.helper = allowed;
            }
        }

        char main_str[64], work_str[64], gpu_str[64], help_str[64];
        FormatCpuList(result.game_main, main_str, sizeof(main_str));
        FormatCpuList(result.game_workers, work_str, sizeof(work_str));
        FormatCpuList(result.gpu, gpu_str, sizeof(gpu_str));
        FormatCpuList(result.helper, help_str, sizeof(help_str));
        std::printf("CPU Topology (%d L3 domain%s, %d phys cores): "
                    "MOMainThread=[%s] (SMT sibling %d idle), "
                    "Game/Havok/GX=[%s], Vulkan RHI=[%s], Helpers/IO=[%s]\n",
                    domain_count, domain_count == 1 ? "" : "s", total_phys_cores,
                    main_str, result.best_main_sibling, work_str, gpu_str, help_str);
        std::fflush(stdout);
        return result;
    }();
    return topo;
}

inline const TopologyMasks& GetL3Domains() {
    return GetTopology();
}

/// Pins the calling host Vulkan pipeline thread (bb:GpuComm, bb:DrawRec, bb:VkRecorder*, bb:Present)
/// to the GPU domain (CCD1 on Multi-CCD Ryzen; P-cores on Intel Hybrid; non-MOMainThread cores on Single-CCD).
inline void PinToGpuDomain() {
    const auto& domain = GetTopology().gpu;
    if (CPU_COUNT(&domain) != 0) {
        sched_setaffinity(0, sizeof(domain), &domain);
    }
}

/// Pins auxiliary streaming/speculative helpers (bb:Resolve*, bb:Copy*, bb:DrawScan, bb:SchedOps)
/// to the helper domain (CCD1 on Multi-CCD Ryzen; E-cores on Intel Hybrid; non-MOMainThread cores on Single-CCD).
inline void PinToHelperDomain() {
    const auto& domain = GetTopology().helper;
    if (CPU_COUNT(&domain) != 0) {
        sched_setaffinity(0, sizeof(domain), &domain);
    }
}

/// Scans `/proc/self/task` and applies the hardware-adaptive affinity masks to all guest and host
/// threads (MOMainThread, HavokWorkerThre, CSChrThread*, CSClothThread*, SpClothVertexUp*,
/// GXRenderThread, GXWorker*, EzWorkPool*, FMOD*, and bb:* host threads).
/// Safe and fast to call periodically (e.g. from StallWatch every 500 ms): caches `(tid, comm)`
/// so already-pinned threads incur zero syscalls once named.
inline void ApplyProcessThreadTopology() {
    const auto& topo = GetTopology();
    if (!topo.enabled) {
        return;
    }

    static std::mutex mtx;
    std::scoped_lock lock{mtx};

    struct SeenThread {
        int tid = 0;
        char comm[20]{};
    };
    static SeenThread seen[256]{};
    static int seen_count = 0;

    DIR* dir = ::opendir("/proc/self/task");
    if (!dir) {
        return;
    }

    const auto starts_with = [](const char* s, const char* prefix) {
        return std::strncmp(s, prefix, std::strlen(prefix)) == 0;
    };

    while (struct dirent* ent = ::readdir(dir)) {
        if (ent->d_name[0] < '0' || ent->d_name[0] > '9') {
            continue;
        }
        const int tid = std::atoi(ent->d_name);
        if (tid <= 0) {
            continue;
        }

        char path[64], comm[32]{};
        std::snprintf(path, sizeof(path), "/proc/self/task/%d/comm", tid);
        FILE* f = std::fopen(path, "r");
        if (!f) {
            continue;
        }
        const bool ok = std::fgets(comm, sizeof(comm), f) != nullptr;
        std::fclose(f);
        if (!ok) {
            continue;
        }
        for (char* p = comm; *p; ++p) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }
        // Wait until guest threads set their real pthread name (not the initial process name).
        if (comm[0] == '\0' || std::strcmp(comm, "bb-probe") == 0 ||
            std::strcmp(comm, "probe") == 0) {
            continue;
        }

        int slot = -1;
        for (int i = 0; i < seen_count; ++i) {
            if (seen[i].tid == tid) {
                slot = i;
                break;
            }
        }
        if (slot >= 0 && std::strncmp(seen[slot].comm, comm, sizeof(seen[slot].comm) - 1) == 0) {
            continue; // Already pinned with this exact name
        }

        const cpu_set_t* target_mask = nullptr;
        bool boost_prio = false;

        if (std::strcmp(comm, "MOMainThread") == 0) {
            target_mask = &topo.game_main;
            boost_prio = true;
        } else if (starts_with(comm, "HavokWorker") ||
                   starts_with(comm, "CSChrThread") ||
                   starts_with(comm, "CSClothThread") ||
                   starts_with(comm, "SpClothVertex") ||
                   starts_with(comm, "FD4JobWorker") ||
                   starts_with(comm, "GXRenderThread") ||
                   starts_with(comm, "GXWorker") ||
                   starts_with(comm, "Battle Worker") ||
                   starts_with(comm, "CS Filtering") ||
                   starts_with(comm, "MO Update")) {
            target_mask = &topo.game_workers;
        } else if (std::strcmp(comm, "bb:GpuComm") == 0 ||
                   std::strcmp(comm, "bb:DrawRec") == 0 ||
                   starts_with(comm, "bb:VkRecorder") ||
                   std::strcmp(comm, "bb:Present") == 0) {
            target_mask = &topo.gpu;
        } else if (starts_with(comm, "bb:") ||
                   starts_with(comm, "bb-probe:") ||
                   starts_with(comm, "EzWorkPool") ||
                   starts_with(comm, "Core.Res.") ||
                   starts_with(comm, "FMOD") ||
                   starts_with(comm, "FSBank") ||
                   starts_with(comm, "SLSession") ||
                   starts_with(comm, "SL ") ||
                   starts_with(comm, "CSTrophy") ||
                   starts_with(comm, "CSMenu") ||
                   starts_with(comm, "SaveData") ||
                   starts_with(comm, "FrpgNet") ||
                   starts_with(comm, "SystemService") ||
                   starts_with(comm, "ImeEvent") ||
                   starts_with(comm, "FaceGen") ||
                   starts_with(comm, "KatanaMain")) {
            target_mask = &topo.helper;
        }

        if (!target_mask || CPU_COUNT(target_mask) == 0) {
            continue;
        }

        if (sched_setaffinity(tid, sizeof(cpu_set_t), target_mask) == 0) {
            if (boost_prio) {
                // Give MOMainThread a mild priority boost if permitted by OS limits.
                setpriority(PRIO_PROCESS, static_cast<id_t>(tid), -2);
            }
            if (slot < 0 && seen_count < 256) {
                slot = seen_count++;
                seen[slot].tid = tid;
            }
            if (slot >= 0) {
                std::strncpy(seen[slot].comm, comm, sizeof(seen[slot].comm) - 1);
                seen[slot].comm[sizeof(seen[slot].comm) - 1] = '\0';
            }
        }
    }

    ::closedir(dir);
}
#else
inline void PinToGpuDomain() {}
inline void PinToHelperDomain() {}
inline void ApplyProcessThreadTopology() {}
#endif

} // namespace BbThreads
