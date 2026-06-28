// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <random>
#include <semaphore>
#include <stop_token>
#include <thread>

#include "jungle/async/future.h"
#include "jungle/async/invoke.h"
#include "jungle/async/join_handle.h"
#include "jungle/container/mpsc.h"
#include "jungle/preusing.h"
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

    template<typename... Args>
    auto spawn(async::async_function<Args...> auto &&fn, Args &&...args)
        requires requires {
            { fn(args...) } -> async::future_type;
        }
    {
        auto jh =
            task_coroutine(async::co_invoke(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...));
        std::uniform_int_distribution<usize> w{0, m_workers.size() - 1};
        usize x = w(rng());
        m_senders[x].send(jh.get_coroutine_handle());
        m_workers[x]->awake();
        return jh;
    }

    template<typename... Args>
    auto spawn_local(async::async_function<Args...> auto &&fn, Args &&...args)
        requires requires {
            { fn(args...) } -> async::future_type;
        }
    pre(worker::exists()) {
        auto jh =
            task_coroutine(async::co_invoke(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...));
        usize x = worker::current().id();
        m_senders[x].send(jh.get_coroutine_handle());
        m_workers[x]->awake();
        return jh;
    }

    template<typename... Args>
    auto block_on(async::async_function<Args...> auto &&fn, Args &&...args)
        requires requires {
            { fn(args...) } -> async::future_type;
        }
    pre(m_multi_threaded) {
        auto jh = spawn(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...);

        if constexpr (concepts::is_void<typename decltype(jh)::output_type>) {
            jh.blocking_await();
        } else {
            return jh.blocking_await();
        }
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

    explicit runtime(runtime_config config);

    const bool m_multi_threaded;

    std::stop_source m_stop;

    std::vector<std::unique_ptr<worker>> m_workers{};

    std::vector<task_sender> m_senders{};
    mpsc<usize>::receiver m_acceptible_worker_rx;
};

};  // namespace jungle::tasks::runtime

namespace jungle::tasks {

template<typename... Args>
auto spawn(async::async_function<Args...> auto &&fn, Args &&...args)
    requires requires {
        { fn(args...) } -> async::future_type;
    }
{
    return runtime::worker::current().host_runtime().spawn(
        std::forward<decltype(fn)>(fn), std::forward<Args>(args)...);
}

};  // namespace jungle::tasks
