// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/os/process.h"

#include <string>
#include <thread>
#include <utility>

#include <pthread.h>
#include <sched.h>

namespace jungle::os {

bool set_thread_affinity(std::thread::native_handle_type tid, cpu_set cpuset) {
    ::cpu_set_t cs;
    CPU_ZERO(&cs);
    for (auto id : cpuset.get_all()) {
        CPU_SET(id, &cs);
    }
    return ::pthread_setaffinity_np(tid, sizeof(cs), &cs) == 0;
}

bool set_thread_name(std::thread::native_handle_type tid, const std::string &name) {
    return ::pthread_setname_np(tid, name.c_str()) == 0;
}

bool thread_handle::set_name(std::string name) { return set_thread_name(m_tid, name); }

bool thread_handle::set_affinity(cpu_set cpuset) { return set_thread_affinity(m_tid, std::move(cpuset)); }

};  // namespace jungle::os
