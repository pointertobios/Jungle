// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <thread>
#include <vector>

#include "jungle/types/int.h"

namespace jungle::os {

class cpu_set final {
public:
    cpu_set();

    cpu_set(const cpu_set &) = delete;
    cpu_set &operator=(const cpu_set &) = delete;

    cpu_set(cpu_set &&) = default;
    cpu_set &operator=(cpu_set &&) = default;

    cpu_set with(usize id) &&;

    std::vector<usize> &get_all();

private:
    std::vector<usize> m_cpus;
};

class thread_handle final {
public:
    thread_handle(std::thread::native_handle_type tid);

    static thread_handle from(std::thread &t);
    static thread_handle from(std::jthread &t);

    bool set_name(const std::string &name);

    bool set_affinity(cpu_set cpuset);

private:
    std::thread::native_handle_type m_tid;
};

};  // namespace jungle::os
