// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/tasks/runtime/runtime.h"
#include "jungle/tasks/runtime/worker.h"
#include <memory>
#include <semaphore>

namespace jungle::tasks::runtime {

runtime runtime_config::build() && { return runtime{*this}; }

runtime_config runtime_config::single_threaded() && {
    m_multi_thread = false;
    m_concurrency = 1;
    return *this;
}

runtime_config runtime_config::multi_threaded(usize n) && {
    m_multi_thread = true;
    m_concurrency = n;
    return *this;
}

runtime::runtime()
        : runtime{runtime_config{}} {}

runtime::runtime(runtime_config config)
        : m_multi_threaded{config.m_multi_thread} {
    s_this_runtime = this;
    for (usize i : std::views::iota((usize)0, config.m_concurrency)) {
        auto [tx, rx] = mpsc<std::coroutine_handle<>>::queue();
        m_senders.emplace_back(std::move(tx));
        m_workers.emplace_back(std::make_unique<worker>(i, std::move(rx)));
    }
    if (config.m_multi_thread) {
        for (auto &w : m_workers) {
            w->start();
        }
    }
}

runtime::~runtime() {
    for (auto &w : m_workers) {
        w->join();
    }
}

void runtime::main_loop() {
    auto st = m_stop.get_token();
    m_workers[0]->run(st);
}

void runtime::stop() { m_stop.request_stop(); }

};  // namespace jungle::tasks::runtime
