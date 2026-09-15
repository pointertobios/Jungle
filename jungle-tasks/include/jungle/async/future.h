// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <coroutine>
#include <source_location>
#include <utility>

#include "jungle/assert.h"
#include "jungle/async/control.h"
#include "jungle/async/future.h"
#include "jungle/meta.h"
#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/tasks/runtime/predecl.h"
#include "jungle/types/erased.h"
#include "jungle/types/raw_storage.h"

namespace jungle::async {

namespace detail {

#ifdef JUNGLE_DEBUG_ENABLED
void future_trace_start(std::source_location sl);
void future_trace_end();
#endif

void future_create_placement_executing_guard(raw_storage<tasks::runtime::placement_executing_guard> &guard);
void future_destroy_placement_executing_guard(raw_storage<tasks::runtime::placement_executing_guard> &guard);

};  // namespace detail

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
        void return_void() {
            JUNGLE_ASSERT(promise_base::m_future->m_state == future_state::non_complete);
            promise_base::m_future->m_state = future_state::complete;
        }
    };

    struct promise_value_mixin : public promise_base {
        void return_value(try_move_t<T> value) {
            JUNGLE_ASSERT(promise_base::m_future->m_state == future_state::non_complete);
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
#else
            (void)sl;
#endif
            promise_base::m_this_coroutine = coroutine_handle::from_promise(*this);
            return future{this, promise_base::m_this_coroutine};
        }
    };

    future() = default;

    ~future() noexcept {
        if (m_this_coroutine) {
            m_this_coroutine.destroy();
        }
    }

    future(const future &) = delete;
    future &operator=(const future &) = delete;

    future(future &&rhs)
            : m_promise{rhs.m_promise}
            , m_state{rhs.m_state}
            , m_this_coroutine{rhs.m_this_coroutine}
            , m_waiter_coroutine{rhs.m_waiter_coroutine}
            , m_bound_invocable{std::move(rhs.m_bound_invocable)} {
        JUNGLE_ASSERT(!rhs.is_empty());

        rhs.m_this_coroutine = coroutine_handle{};

        m_promise->m_future = this;
    }

    future &operator=(future &&rhs) {
        JUNGLE_ASSERT(!rhs.is_empty() && is_empty());

        if (this != &rhs) {
            this->~future();
            new (this) future{std::move(rhs)};
        }
        return *this;
    }

    bool is_empty() const { return m_state == future_state::empty; }

    T placement_execute() {
        raw_storage<tasks::runtime::placement_executing_guard> guard;
        detail::future_create_placement_executing_guard(guard);
        m_this_coroutine.resume();
        detail::future_destroy_placement_executing_guard(guard);

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

    bool await_ready() {
        JUNGLE_ASSERT(!is_empty());
        return false;
    }

    auto await_suspend(std::coroutine_handle<> waiter) {
        JUNGLE_ASSERT(!is_empty());

#ifdef JUNGLE_DEBUG_ENABLED
        detail::future_trace_start(m_promise->m_source_location);
#endif

        m_waiter_coroutine = waiter;
        return m_this_coroutine;
    }

    T await_resume() {
        JUNGLE_ASSERT(!is_empty());
#ifdef JUNGLE_DEBUG_ENABLED
        detail::future_trace_end();
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
