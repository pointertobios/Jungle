// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <memory>
#include <semaphore>

#include "jungle/tasks/runtime/runtime.h"
#include "jungle/tasks/runtime/worker.h"
#include "jungle/types/concepts.h"

namespace jungle::tasks::runtime {

runtime runtime_config::build() && { return runtime{*this}; }

std::unique_ptr<runtime> runtime_config::build_ptr() && { return std::make_unique<runtime>(*this); }

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
    auto [acceptible_tx, acceptible_rx] = mpsc<usize>::queue();
    m_acceptible_worker_rx = std::move(acceptible_rx);
    for (usize i : std::views::iota((usize)0, config.m_concurrency)) {
        auto [tx, rx] = mpsc<task_item>::queue();
        m_senders.emplace_back(std::move(tx));
        auto atx = acceptible_tx;
        m_workers.emplace_back(std::make_unique<worker>(this, i, std::move(rx), std::move(atx)));
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
