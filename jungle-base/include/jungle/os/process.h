// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <string_view>
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
    thread_handle() = default;

    thread_handle(const thread_handle &) = default;
    thread_handle &operator=(const thread_handle &) = default;

    thread_handle(std::thread::native_handle_type tid);

    static thread_handle from(std::thread &t);
    static thread_handle from(std::jthread &t);
    static thread_handle this_thread();

    std::thread::native_handle_type native_handle() const { return m_tid; }

    bool operator==(const thread_handle &rhs) const { return m_tid == rhs.m_tid; }

    bool set_name(std::string name);

    bool set_affinity(cpu_set cpuset);

private:
    std::thread::native_handle_type m_tid;
};

};  // namespace jungle::os

template<>
struct std::hash<jungle::os::thread_handle> {
    std::size_t operator()(const jungle::os::thread_handle &th) const {
        return std::hash<std::thread::native_handle_type>{}(th.native_handle());
    }
};
