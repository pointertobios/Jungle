// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/os/process.h"

#include <string>
#include <thread>
#include <utility>

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace jungle::os {

bool set_thread_affinity(std::thread::native_handle_type tid, cpu_set cpuset) {
    const auto bits = cpuset.get();
    if (bits == 0) {
        return false;
    }

    const WORD group_cnt = GetActiveProcessorGroupCount();
    DWORD base_index = 0;

    bool found = false;
    GROUP_AFFINITY ga{};

    for (WORD g = 0; g < group_cnt; g++) {
        const DWORD group_cpu_cnt = GetActiveProcessorCount(g);
        KAFFINITY mask = 0;

        for (usize id = 0; id < 128; ++id) {
            if ((bits >> id) & 1) {
                if (id >= base_index && id < base_index + group_cpu_cnt) {
                    mask |= (KAFFINITY(1) << (id - base_index));
                }
            }
        }

        if (mask != 0) {
            if (found) {
                return false;
            }
            found = true;
            ga.Group = g;
            ga.Mask = mask;
        }

        base_index += group_cpu_cnt;
    }

    if (!found) {
        return false;
    }

    return SetThreadGroupAffinity(reinterpret_cast<HANDLE>(tid), &ga, nullptr) != 0;
}

bool set_thread_name(std::thread::native_handle_type tid, const std::string &name) {
    const auto wname = std::wstring(name.begin(), name.end());
    const auto hr = SetThreadDescription(reinterpret_cast<HANDLE>(tid), wname.c_str());
    return SUCCEEDED(hr);
}

thread_handle thread_handle::this_thread() {
    HANDLE hThread = GetCurrentThread();
    return {reinterpret_cast<std::thread::native_handle_type>(hThread)};
}

bool thread_handle::set_name(std::string name) { return set_thread_name(m_tid, name); }

bool thread_handle::set_affinity(cpu_set cpuset) { return set_thread_affinity(m_tid, std::move(cpuset)); }

};  // namespace jungle::os
