// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <coroutine>
#include <utility>

#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/types/raw_storage.h"

namespace jungle {

template<typename T>
class task final {
public:
    using output_type = T;

    struct promise_type;
    using coroutine_handle = std::coroutine_handle<promise_type>;

private:
    enum future_state {
        empty,
        non_complete,
        complete,
    };

    struct promise_base {
        friend class task;

        task *m_future;

        std::suspend_always initial_suspend() { return {}; }

        void unhandled_exception() { panic("exception unsupported"); }

        auto final_suspend() {
            struct final_awaitable {
                coroutine_handle this_coroutine;

                bool await_ready() { return false; }

                void await_suspend(std::coroutine_handle<>) { this_coroutine.destroy(); }

                void await_resume() {}
            };
            if (m_future->m_state != future_state::complete) {
                m_future->m_state = future_state::complete;
            }
            return final_awaitable{m_future->m_this_coroutine};
        }
    };

    struct promise_void_mixin : public promise_base {
        void return_void() pre(promise_base::m_future->m_state == future_state::non_complete) {
            promise_base::m_future->m_state = future_state::complete;
        }
    };

    struct promise_value_mixin : public promise_base {
        void return_value(try_move_t<T> value)
            pre(promise_base::m_future->m_state == future_state::non_complete) {
            promise_base::m_future->m_storage.emplace(try_move(value));
            promise_base::m_future->m_state = future_state::complete;
        }
    };

    using promise_base_type =
        std::conditional_t<concepts::is_void<T>, promise_void_mixin, promise_value_mixin>;

public:
    struct promise_type : public promise_base_type {
        task get_return_object() { return task{this, coroutine_handle::from_promise(*this)}; }
    };

    task() = default;

    task(const task &) = delete;
    task &operator=(const task &) = delete;

    task(task &&rhs) pre(!rhs.is_empty())
            : m_promise{rhs.m_promise}
            , m_state{rhs.m_state}
            , m_this_coroutine{rhs.m_this_coroutine} {
        m_promise->m_future = this;
    }

    task &operator=(task &&rhs) pre(!rhs.is_empty() && !is_empty()) {
        if (this != &rhs) {
            m_promise = rhs.m_promise;
            m_state = rhs.m_state;
            m_promise->m_future = this;
        }
        return *this;
    }

    bool is_empty() const { return m_state != future_state::empty; }

    void resume() const pre(!is_empty()) { m_this_coroutine.resume(); }

private:
    task(promise_type *promise, coroutine_handle this_coroutine)
            : m_promise{promise}
            , m_state{future_state::non_complete}
            , m_this_coroutine{this_coroutine} {
        m_promise->m_future = this;
    }

    promise_type *m_promise{nullptr};
    [[no_unique_address]] raw_storage<T> m_storage{};
    future_state m_state{future_state::empty};

    coroutine_handle m_this_coroutine{};
};

};  // namespace jungle
