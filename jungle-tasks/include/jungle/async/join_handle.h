// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <coroutine>
#include <memory>
#include <semaphore>
#include <utility>

#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/tasks/runtime/worker.h"
#include "jungle/types/erased.h"
#include "jungle/types/raw_storage.h"

namespace jungle::async {

template<typename T>
class join_handle final {
public:
    using output_type = T;

    struct promise_type;
    using coroutine_handle = std::coroutine_handle<promise_type>;

private:
    enum future_state {
        non_complete,
        awaited,
        complete,
    };

    struct task_block {
        std::atomic<future_state> m_state;
        [[no_unique_address]] raw_storage<T> m_storage{};
        tasks::runtime::awake_token m_awake_token{tasks::runtime::awake_token::none()};

        std::binary_semaphore m_sync_awaiter{0};

        erased m_bound_invocable{};
    };

    struct promise_base {
        std::shared_ptr<task_block> m_task_block;

        coroutine_handle m_this_coroutine{};

        std::suspend_always initial_suspend() { return {}; }

        void unhandled_exception() { panic("exception unsupported"); }

        auto final_suspend() {
            struct final_awaitable {
                coroutine_handle this_coroutine;

                bool await_ready() { return false; }

                void await_suspend(std::coroutine_handle<>) { this_coroutine.destroy(); }

                void await_resume() {}
            };
            {
                auto s = m_task_block->m_state.exchange(future_state::complete, morder::acq_rel);
                if (s != future_state::complete) {
                    if (s == future_state::awaited) {
                        promise_base::m_task_block->m_awake_token.awake();
                    }
                    promise_base::m_task_block->m_sync_awaiter.release();
                }
            }
            using tasks::runtime::worker;
            if (worker::exists()) {
                auto &w = worker::current();
                w.set_next_resume(std::coroutine_handle{});
                w.set_suspend_now();
            }
            return final_awaitable{m_this_coroutine};
        }

    private:
        bool return_state_valid() const {
            auto s = m_task_block->m_state.load(morder::relaxed);
            return s == future_state::non_complete || s == future_state::awaited;
        }
    };

    struct promise_void_mixin : public promise_base {
        void return_void() pre(promise_base::return_state_valid()) {
            auto s = promise_base::m_task_block->m_state.exchange(future_state::complete, morder::acq_rel);
            if (s == future_state::awaited) {
                promise_base::m_task_block->m_awake_token.awake();
            }
            promise_base::m_task_block->m_sync_awaiter.release();
        }
    };

    struct promise_value_mixin : public promise_base {
        void return_value(try_move_t<T> value) pre(promise_base::return_state_valid()) {
            promise_base::m_task_block->m_storage.emplace(try_move(value));
            auto s = promise_base::m_task_block->m_state.exchange(future_state::complete, morder::release);
            if (s == future_state::awaited) {
                promise_base::m_task_block->m_awake_token.awake();
            }
            promise_base::m_task_block->m_sync_awaiter.release();
        }
    };

    using promise_base_type =
        std::conditional_t<concepts::is_void<T>, promise_void_mixin, promise_value_mixin>;

public:
    struct promise_type : public promise_base_type {
        join_handle get_return_object() {
            auto this_coroutine = coroutine_handle::from_promise(*this);
            promise_base::m_this_coroutine = this_coroutine;

            auto task_block_ptr = std::make_shared<task_block>(future_state::non_complete);
            promise_base::m_task_block = task_block_ptr;

            return join_handle{this_coroutine, std::move(task_block_ptr)};
        }
    };

    join_handle() = default;

    join_handle(const join_handle &) = delete;
    join_handle &operator=(const join_handle &) = delete;

    join_handle(join_handle &&rhs) pre(!rhs.is_empty())
            : m_this_coroutine{rhs.m_this_coroutine}
            , m_task_block{std::move(rhs.m_task_block)} {
        rhs.m_this_coroutine = coroutine_handle{};
    }

    join_handle &operator=(join_handle &&rhs) pre(!rhs.is_empty() && is_empty()) {
        if (this != &rhs) {
            m_this_coroutine = rhs.m_this_coroutine;
            rhs.m_this_coroutine = coroutine_handle{};
            m_task_block = std::move(rhs.m_task_block);
        }
        return *this;
    }

    bool is_empty() const { return static_cast<bool>(m_this_coroutine); }

    bool await_ready() pre(!is_empty()) {
        return m_task_block->m_state.load(morder::acquire) == future_state::complete;
    }

    void await_suspend(std::coroutine_handle<> waiter_coroutine) pre(!is_empty()) {
        tasks::runtime::awake_token awake_token{waiter_coroutine};
        m_task_block->m_awake_token = awake_token;
        if (future_state e{future_state::non_complete}; m_task_block->m_state.compare_exchange_strong(
                e, future_state::awaited, morder::acq_rel, morder::relaxed)) {
            awake_token.suspend();
        }
    }

    T await_resume() pre(!is_empty()) {
        if constexpr (concepts::is_void<T>) {
            return;
        } else {
            T res{try_move(*m_task_block->m_storage.get())};
            m_task_block->m_storage.destroy();
            return res;
        }
    }

    T blocking_await() {
        m_task_block->m_sync_awaiter.acquire();
        return await_resume();
    }

    void bind_invocable(erased &&invocable) { m_task_block->m_bound_invocable = std::move(invocable); }

    coroutine_handle get_coroutine_handle() const { return m_this_coroutine; }

private:
    join_handle(coroutine_handle this_coroutine, std::shared_ptr<task_block> task_block_ptr)
            : m_this_coroutine{this_coroutine}
            , m_task_block{task_block_ptr} {}

    coroutine_handle m_this_coroutine;
    std::shared_ptr<task_block> m_task_block;
};

template<typename JoinHandle>
concept join_handle_type = meta::is_specialization_of_template<^^JoinHandle, ^^join_handle>();

};  // namespace jungle::async
