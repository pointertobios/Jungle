// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <coroutine>
#include <utility>

#include "jungle/panic.h"
#include "jungle/types/concepts.h"
#include "jungle/types/raw_storage.h"
#include "jungle/types/types.h"

namespace jungle::tasks {

template<typename T = void>
class [[nodiscard("A future<T> must always be co_await'ed once")]] future final {
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
        friend class future;

        future *m_future;

        std::suspend_always initial_suspend() { return {}; }

        void unhandled_exception() { panic("exception unsupported"); }

        auto final_suspend() {
            struct final_awaitable {
                coroutine_handle this_coroutine;
                std::coroutine_handle<> waiter_coroutine;

                bool await_ready() { return false; }

                auto await_suspend(std::coroutine_handle<>) {
                    this_coroutine.destroy();
                    return waiter_coroutine;
                }

                void await_resume() {}
            };
            return final_awaitable{m_future->m_this_coroutine, m_future->m_waiter_coroutine};
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
            new (promise_base::m_future->m_storage.get()) T{try_move(value)};
            promise_base::m_future->m_state = future_state::complete;
        }
    };

public:
    struct promise_type
            : public std::conditional_t<concepts::is_void<T>, promise_void_mixin, promise_value_mixin> {
        future get_return_object() { return future{this, coroutine_handle::from_promise(*this)}; }
    };

    future() = default;

    future(const future &) = delete;
    future &operator=(const future &) = delete;

    future(future &&rhs) pre(rhs.m_state != future_state::complete)
            : m_promise{rhs.m_promise}
            , m_state{rhs.m_state}
            , m_this_coroutine{rhs.m_this_coroutine}
            , m_waiter_coroutine{rhs.m_waiter_coroutine} {
        m_promise->m_future = this;
    }

    future &operator=(future &&rhs)
        pre(rhs.m_state != future_state::complete && m_state == future_state::empty) {
        if (this != &rhs) {
            m_promise = rhs.m_promise;
            m_state = rhs.m_state;
            m_promise->m_future = this;
        }
        return *this;
    }

    operator bool() const { return m_state != future_state::empty; }

    bool await_ready() pre(*this) { return false; }

    auto await_suspend(std::coroutine_handle<> waiter) pre(*this) {
        m_waiter_coroutine = waiter;
        return m_this_coroutine;
    }

    T await_resume() pre(*this) {
        if constexpr (concepts::is_void<T>) {
            m_state = future_state::empty;
            return;
        } else {
            T res{try_move(*m_storage.get())};
            m_state = future_state::empty;
            return res;
        }
    }

private:
    future(promise_type *promise, coroutine_handle this_coroutine)
            : m_promise{promise}
            , m_state{future_state::non_complete}
            , m_this_coroutine{this_coroutine} {}

    promise_type *m_promise{nullptr};
    [[no_unique_address]] raw_storage<T> m_storage{};
    future_state m_state{future_state::empty};

    coroutine_handle m_this_coroutine{};
    std::coroutine_handle<> m_waiter_coroutine{};
};

};  // namespace jungle::tasks
