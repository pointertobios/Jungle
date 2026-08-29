// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <concepts>
#include <vector>

#include "jungle/async/future.h"
#include "jungle/container/hash_map.h"
#include "jungle/sync/rwspinlock.h"
#include "jungle/sync/spinlock.h"
#include "jungle/tasks/runtime/worker.h"
#include "jungle/types/concepts.h"

namespace jungle::sync {

class condition_variable {
    using awake_token = tasks::runtime::awake_token;

    template<typename Fn = void>
    class wait_awaitable {
        friend class condition_variable;

    public:
        void set_predicator(Fn *pred) { m_pred = pred; }

        bool await_ready() {
            if constexpr (!std::is_void_v<Fn>) {
                m_pred_result = (*m_pred)();
                return m_pred_result;
            } else {
                return false;
            }
        }

        bool await_suspend(std::coroutine_handle<> handle) {
            auto g = m_cv.m_suspend_lock.lock();
            awake_token tk{};
            if constexpr (!std::is_void_v<Fn>) {
                m_pred_result = (*m_pred)();
                if (m_pred_result) {
                    return false;
                }
            }
            tk.suspend(handle);
            auto non_tk = awake_token::none();
            if (!m_cv.m_fast_awake_token.compare_exchange_strong(
                    non_tk, tk, morder::acq_rel, morder::relaxed)) {
                auto guard = condition_variable::s_parking_lot.read();
                guard->get(&m_cv)->push_back(tk);
            }
            return true;
        }

        std::conditional_t<std::is_void_v<Fn>, void, bool> await_resume() const {
            if constexpr (!std::is_void_v<Fn>) {
                return m_pred_result;
            }
        }

    private:
        wait_awaitable(condition_variable &cv)
                : m_cv{cv} {}

        condition_variable &m_cv;
        Fn *m_pred{nullptr};
        bool m_pred_result{false};
    };

public:
    condition_variable();
    ~condition_variable();

    condition_variable(const condition_variable &) = delete;
    condition_variable &operator=(const condition_variable &) = delete;

    condition_variable(condition_variable &&) = delete;
    condition_variable &operator=(condition_variable &&) = delete;

    wait_awaitable<> operator()() { return wait_awaitable{*this}; }

    async::future<> operator()(concepts::verified_invocable<bool> auto pred) {
        while (true) {
            auto awaitable = wait_awaitable<decltype(pred)>{*this};
            awaitable.set_predicator(&pred);
            if (co_await awaitable) {
                break;
            }
        }
    }

    bool notify_one();
    usize notify_all();

private:
    std::atomic<awake_token> m_fast_awake_token{awake_token::none()};
    spinlock<> m_suspend_lock{};

    inline static rwspinlock<hash_map<condition_variable *, std::vector<awake_token>>, true> s_parking_lot{};
};

};  // namespace jungle::sync
