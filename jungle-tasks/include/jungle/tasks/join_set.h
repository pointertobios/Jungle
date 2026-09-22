// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <optional>
#include <utility>
#include <vector>

#include "jungle/async/invoke.h"
#include "jungle/container/mpsc.h"
#include "jungle/tasks/runtime/runtime.h"
#include "jungle/tasks/this_task.h"

namespace jungle::tasks {

template<concepts::non_void Output>
class join_set final {
public:
    using output_type = Output;

    join_set() noexcept
            : join_set{this_task::host_runtime()} {}

    join_set(runtime::runtime &rt) noexcept
            : m_runtime{rt} {
        auto [tx, rx] = container::mpsc<output_type>::queue();
        m_tx = std::move(tx);
        m_rx = std::move(rx);
    }

    join_set(const join_set &) = delete;
    join_set &operator=(const join_set &) = delete;
    join_set(join_set &&) = delete;
    join_set &operator=(join_set &&) = delete;

    void spawn(async::async_function<> auto &&fn) {
        m_task_count.fetch_add(1, morder::acq_rel);
        m_handles.emplace_back(
            m_runtime.spawn([tx = m_tx, fn = std::forward<decltype(fn)>(fn)] mutable -> async::future<> {
                auto output = co_await async::co_invoke(std::move(fn));
                while (!tx.send(try_move(output))) {
                    co_await this_task::yield();
                }
            }));
    }

    void spawn_blocking(std::invocable<> auto &&fn) {
        m_task_count.fetch_add(1, morder::acq_rel);
        m_handles.emplace_back(
            m_runtime.spawn_blocking([tx = m_tx, fn = std::forward<decltype(fn)>(fn)] mutable {
                auto output = std::invoke(std::move(fn));
                while (!tx.send(try_move(output))) {}
            }));
    }

    async::future<std::optional<output_type>> operator co_await() {
        if (m_task_count.load(morder::acquire) == 0) {
            co_return std::nullopt;
        }

        while (true) {
            auto result = m_rx.recv();
            if (result.has_value()) {
                m_task_count.fetch_sub(1, morder::acq_rel);
                co_return std::move(*result);
            }
            co_await this_task::yield();
        }
    }

    async::future<std::vector<output_type>> join_all() {
        std::vector<output_type> results;
        while (m_task_count.load(morder::acquire) != 0) {
            auto result = co_await *this;
            if (result.has_value()) {
                results.emplace_back(std::move(*result));
            }
        }
        co_return results;
    }

private:
    runtime::runtime &m_runtime;
    container::mpsc<output_type>::sender m_tx{};
    container::mpsc<output_type>::receiver m_rx{};
    std::vector<async::join_handle<>> m_handles{};
    std::atomic<usize> m_task_count{0};
};

};  // namespace jungle::tasks
