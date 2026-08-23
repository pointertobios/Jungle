// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <coroutine>
#include <source_location>
#include <utility>

#include "jungle/async/control.h"
#include "jungle/meta.h"
#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/types/erased.h"
#include "jungle/types/raw_storage.h"

#ifdef JUNGLE_DEBUG_ENABLED
#    include "jungle/tasks/runtime/debug_host.h"
#    include "jungle/tasks/runtime/worker.h"
#    include "jungle/tasks/this_task.h"
#endif

namespace jungle::async {

template<typename T = void>
class [[nodiscard("A future<T> must always be co_await'ed once")]] future final {
public:
    using output_type = T;

    struct promise_type;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    using future_state = control::future_state;

private:
    struct promise_base {
        friend class future;

        future *m_future;
        coroutine_handle m_this_coroutine;

#ifdef JUNGLE_DEBUG_ENABLED
        std::source_location m_source_location;
#endif

        ~promise_base() { m_this_coroutine.destroy(); }

        std::suspend_always initial_suspend() { return {}; }

        void unhandled_exception() { panic("exception unsupported"); }

        auto final_suspend() {
            struct final_awaitable {
                std::coroutine_handle<> waiter_coroutine;

                bool await_ready() { return false; }

                auto await_suspend(std::coroutine_handle<>) { return waiter_coroutine; }

                void await_resume() {}
            };
            if (m_future->m_state != future_state::complete) {
                m_future->m_state = future_state::complete;
            }
            return final_awaitable{m_future->m_waiter_coroutine};
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
        future get_return_object(std::source_location sl = std::source_location::current()) {
#ifdef JUNGLE_DEBUG_ENABLED
            promise_base::m_source_location = sl;
#endif
            promise_base::m_this_coroutine = coroutine_handle::from_promise(*this);
            return future{this, promise_base::m_this_coroutine};
        }
    };

    future() = default;

    future(const future &) = delete;
    future &operator=(const future &) = delete;

    future(future &&rhs) pre(!rhs.is_empty())
            : m_promise{rhs.m_promise}
            , m_state{rhs.m_state}
            , m_this_coroutine{rhs.m_this_coroutine}
            , m_waiter_coroutine{rhs.m_waiter_coroutine}
            , m_bound_invocable{std::move(rhs.m_bound_invocable)} {
        m_promise->m_future = this;
    }

    future &operator=(future &&rhs) pre(!rhs.is_empty() && !is_empty()) {
        if (this != &rhs) {
            m_promise = rhs.m_promise;
            m_state = rhs.m_state;
            m_promise->m_future = this;
        }
        return *this;
    }

    bool is_empty() const { return m_state == future_state::empty; }

    bool await_ready() pre(!is_empty()) { return false; }

    auto await_suspend(std::coroutine_handle<> waiter) pre(!is_empty()) {
#ifdef JUNGLE_DEBUG_ENABLED
        get_debug_host().trace_coroutine_start(
            this_task::worker().id(), this_task::id(), m_promise->m_source_location);
#endif

        m_waiter_coroutine = waiter;
        return m_this_coroutine;
    }

    T await_resume() pre(!is_empty()) {
#ifdef JUNGLE_DEBUG_ENABLED
        get_debug_host().trace_coroutine_end(this_task::worker().id(), this_task::id());
#endif

        if constexpr (concepts::is_void<T>) {
            m_state = future_state::empty;
            return;
        } else {
            T res{try_move(*m_storage.get())};
            m_storage.destroy();
            m_state = future_state::empty;
            return res;
        }
    }

    void bind_invocable(erased &&invocable) { m_bound_invocable = std::move(invocable); }

private:
    future(promise_type *promise, coroutine_handle this_coroutine)
            : m_promise{promise}
            , m_state{future_state::non_complete}
            , m_this_coroutine{this_coroutine} {
        m_promise->m_future = this;
    }

#ifdef JUNGLE_DEBUG_ENABLED
    static tasks::runtime::debug_host &get_debug_host() {
        return tasks::runtime::worker::current().host_runtime().get_debug_host();
    };
#endif

    promise_type *m_promise{nullptr};
    [[no_unique_address]] raw_storage<T> m_storage{};
    future_state m_state{future_state::empty};

    coroutine_handle m_this_coroutine{};
    std::coroutine_handle<> m_waiter_coroutine{};

    erased m_bound_invocable{};
};

template<typename Future>
concept future_type = meta::is_specialization_of_template<^^Future, ^^future>();

};  // namespace jungle::async
