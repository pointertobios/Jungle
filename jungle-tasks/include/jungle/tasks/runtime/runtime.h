// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <semaphore>
#include <stop_token>
#include <thread>

#include "jungle/async/join_handle.h"
#include "jungle/preusing.h"
#include "jungle/tasks/runtime/worker.h"

namespace jungle::tasks::runtime {

class runtime;

class runtime_config {
    friend class runtime;

public:
    runtime_config() = default;

    runtime build() &&;

    runtime_config single_threaded() &&;
    runtime_config multi_threaded(usize n = std::thread::hardware_concurrency()) &&;

private:
    bool m_multi_thread{true};
    usize m_concurrency{std::thread::hardware_concurrency()};
};

class runtime final {
    friend class runtime_config;

public:
    explicit runtime();
    ~runtime();

    static bool exsits() { return s_this_runtime; }
    static runtime &current() { return *s_this_runtime; }

    void main_loop() pre(!m_multi_threaded);

    void stop() pre(!m_multi_threaded);

private:
    explicit runtime(runtime_config config);

    const bool m_multi_threaded;

    std::stop_source m_stop;

    std::vector<task_sender> m_senders{};
    std::vector<std::unique_ptr<worker>> m_workers{};

    inline static runtime *s_this_runtime;
};

};  // namespace jungle::tasks::runtime
