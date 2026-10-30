// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: helper thread sizing. Counts follow the hardware threads this process may run on
// (the affinity mask, so `taskset` can emulate a Steam Deck), and speculative helpers run as
// SCHED_IDLE: they use cores the game leaves idle and never take time from its threads.

#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
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

/// Helper threads of the GPU side that the GPU command thread's progress depends on (Vulkan
/// recording threads, guest memory copy threads): a fault there must never wait for that thread
/// (a read trap's copy back goes through it for the game's threads). Set once per thread.
inline thread_local bool gpu_side_helper = false;

#ifndef _WIN32
/// bbport: the hot GPU-side chain (GPU command thread -> draw recording thread -> Vulkan recording
/// threads) hands every draw from one thread to the next through shared memory. On CPUs with
/// several L3 domains (Ryzen 7950X: two CCDs) the scheduler spread them over both (seen: command
/// thread on CCD1, recording thread on CCD0): every packet and handshake crossed the Infinity
/// Fabric. They are kept on one L3 domain: the one with the highest preferred-core ranking
/// (the faster CCD). BB_GPU_CCD=n picks domain n, BB_GPU_CCD=off leaves them to the scheduler.
inline void PinToGpuDomain() {
    static const cpu_set_t domain = [] {
        cpu_set_t none;
        CPU_ZERO(&none);
        const char* env = std::getenv("BB_GPU_CCD");
        if (env && (env[0] == 'o' || env[0] == '-')) {
            return none;
        }
        cpu_set_t allowed;
        CPU_ZERO(&allowed);
        if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0) {
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
        // L3 domains by their shared CPU list.
        struct Domain {
            cpu_set_t cpus;
            long best_rank;
        };
        Domain domains[16];
        int count = 0;
        for (int cpu = 0; cpu < CPU_SETSIZE && cpu < 1024; ++cpu) {
            if (!CPU_ISSET(cpu, &allowed)) {
                continue;
            }
            char path[128], text[256];
            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cache/index3/shared_cpu_list", cpu);
            if (!read_text(path, text, sizeof(text))) {
                continue;
            }
            cpu_set_t set;
            CPU_ZERO(&set);
            for (char* at = text; *at && *at != '\n';) {
                char* end = nullptr;
                const long first = std::strtol(at, &end, 10);
                long last = first;
                if (*end == '-') {
                    last = std::strtol(end + 1, &end, 10);
                }
                for (long c = first; c <= last && c < CPU_SETSIZE; ++c) {
                    if (CPU_ISSET(c, &allowed)) {
                        CPU_SET(c, &set);
                    }
                }
                at = *end == ',' ? end + 1 : end;
            }
            std::snprintf(path, sizeof(path),
                          "/sys/devices/system/cpu/cpu%d/cpufreq/amd_pstate_prefcore_ranking", cpu);
            const long rank = read_text(path, text, sizeof(text)) ? std::atol(text) : 0;
            int found = -1;
            for (int d = 0; d < count; ++d) {
                if (CPU_EQUAL(&domains[d].cpus, &set)) {
                    found = d;
                    break;
                }
            }
            if (found < 0 && count < 16) {
                found = count++;
                domains[found] = {set, rank};
            } else if (found >= 0) {
                domains[found].best_rank = std::max(domains[found].best_rank, rank);
            }
        }
        if (count < 2) {
            return none; // one L3: nothing to gain
        }
        int pick = 0;
        if (env && env[0] >= '0' && env[0] <= '9') {
            pick = std::clamp(std::atoi(env), 0, count - 1);
        } else {
            for (int d = 1; d < count; ++d) {
                if (domains[d].best_rank > domains[pick].best_rank) {
                    pick = d;
                }
            }
        }
        std::printf("GPU: command, draw recording and Vulkan recording threads kept on L3 domain "
                    "%d of %d (%d CPUs; BB_GPU_CCD=off: anywhere)\n",
                    pick, count, CPU_COUNT(&domains[pick].cpus));
        return domains[pick].cpus;
    }();
    if (CPU_COUNT(&domain) != 0) {
        sched_setaffinity(0, sizeof(domain), &domain);
    }
}
#else
inline void PinToGpuDomain() {}
#endif

} // namespace BbThreads
