// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <random>
#include <semaphore>
#include <stop_token>
#include <thread>
#include <type_traits>

#include "jungle/async/future.h"
#include "jungle/async/invoke.h"
#include "jungle/async/join_handle.h"
#include "jungle/container/mpsc.h"
#include "jungle/preusing.h"
#include "jungle/tasks/runtime/blocking_worker.h"
#include "jungle/tasks/runtime/debug_host.h"
#include "jungle/tasks/runtime/worker.h"
#include "jungle/types/concepts.h"
#include "jungle/util/rng.h"

namespace jungle::tasks::runtime {

class runtime;

class runtime_config {
    friend class runtime;

public:
    runtime_config() = default;

    runtime build() &&;
    std::unique_ptr<runtime> build_ptr() &&;

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
    explicit runtime(runtime_config config);
    ~runtime();

    usize worker_count() const { return m_workers.size(); }
    worker &get_worker(usize wid) { return *m_workers[wid]; }

#ifdef JUNGLE_DEBUG_ENABLED
    debug_host &get_debug_host() { return *m_debug_host; }
#endif

    template<typename... Args>
    auto spawn(async::async_function<Args...> auto &&fn, Args &&...args) {
        auto jh =
            task_coroutine(async::co_invoke(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...));

        usize x;
        if (auto wid = m_acceptible_worker_rx.recv(); wid.has_value()) {
            x = wid.value();
        } else {
            thread_local std::uniform_int_distribution<usize> w{0, m_blocking_pool_start - 1};
            x = w(rng());
        }

        auto ta = jh.get_task_item();
        if (m_multi_threaded) {
            while (!m_senders[x].send(try_move(ta))) {
                x = (x + 1) % m_blocking_pool_start;
            }
        } else {
            if (!m_senders[x].send(try_move(ta))) {
                m_workers[x]->fetch_task();
                (void)m_senders[x].send(try_move(ta));
            }
        }
        m_workers[x]->awake();
        return jh;
    }

    template<typename... Args>
    auto block_on(async::async_function<Args...> auto &&fn, Args &&...args) pre(m_multi_threaded) {
        auto jh = spawn(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...);

        if constexpr (concepts::is_void<typename decltype(jh)::output_type>) {
            jh.blocking_await();
        } else {
            return jh.blocking_await();
        }
    }

    template<typename... Args>
    auto spawn_blocking(std::invocable<Args...> auto &&fn, Args &&...args)
        requires(!async::async_function<decltype(fn), Args...>)
    pre(m_multi_threaded) {
        auto jh = blocking_task_coroutine(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...);

        usize x;
        while (true) {
            auto wid = m_acceptible_blocking_worker_rx.recv();
            if (wid.has_value()) {
                x = wid.value();
                break;
            } else if (wid.error() == container::receive_failed::empty) {
                auto [tx, rx] = container::mpsc<task_item>::queue();
                m_blocking_senders.write()->emplace_back(std::move(tx));
                auto atx = m_acceptible_blocking_worker_tx;

                auto g = m_blocking_workers.write();
                x = m_worker_id_gen++;
                auto &w = g->emplace_back(
                    std::make_unique<blocking_worker>(this, x, std::move(rx), std::move(atx)));
                { auto _ = std::move(g); }

                w->start();
            }
        }
        x -= m_blocking_pool_start;

        (void)m_blocking_senders.read()->at(x).send(jh.get_task_item());
        m_blocking_workers.read()->at(x)->awake();

        return jh;
    }

    void main_loop() pre(!m_multi_threaded);

    void stop() pre(!m_multi_threaded);

private:
    auto task_coroutine(async::future_type auto future_value)
        -> async::join_handle<typename decltype(future_value)::output_type> {
        if (!m_multi_threaded) {
            stop();
        }
        using output_type = typename decltype(future_value)::output_type;
        if constexpr (concepts::is_void<output_type>) {
            co_await future_value;
            co_return;
        } else {
            co_return co_await future_value;
        }
    }

    template<typename... Args>
    auto blocking_task_coroutine(std::invocable<Args...> auto fn, Args &&...args)
        -> async::join_handle<std::invoke_result_t<decltype(fn)>> {
        if constexpr (concepts::is_void<std::invoke_result_t<decltype(fn)>>) {
            std::invoke(fn, args...);
            co_return;
        } else {
            co_return std::invoke(fn, args...);
        }
    }

    const bool m_multi_threaded;
    const usize m_blocking_pool_start;

    std::stop_source m_stop;

    usize m_worker_id_gen;

    std::vector<std::unique_ptr<worker>> m_workers{};
    std::vector<task_sender> m_senders{};
    container::mpsc<usize>::receiver m_acceptible_worker_rx;

    sync::rwspinlock<std::vector<std::unique_ptr<blocking_worker>>> m_blocking_workers;
    sync::rwspinlock<std::vector<task_sender>, true> m_blocking_senders{};
    container::mpsc<usize>::receiver m_acceptible_blocking_worker_rx;
    container::mpsc<usize>::sender m_acceptible_blocking_worker_tx;

#ifdef JUNGLE_DEBUG_ENABLED
    std::unique_ptr<debug_host> m_debug_host{std::make_unique<debug_host>()};
#endif
};

};  // namespace jungle::tasks::runtime
