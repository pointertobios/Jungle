// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <coroutine>
#include <memory>
#include <utility>

#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/tasks/runtime/worker.h"
#include "jungle/types/raw_storage.h"

namespace jungle {

template<typename T>
class join_handle final {
public:
    using output_type = T;

    struct promise_type;
    using coroutine_handle = std::coroutine_handle<promise_type>;

private:
    enum future_state {
        non_complete,
        complete,
    };

    struct task_block {
        std::atomic<future_state> m_state;
        [[no_unique_address]] raw_storage<T> m_storage{};
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
                auto old = future_state::non_complete;
                m_task_block->m_state.compare_exchange_strong(
                    old, future_state::complete, morder::acq_rel, morder::relaxed);
            }
            using tasks::runtime::worker;
            if (worker::exists()) {
                auto &w = worker::current();
                w.set_next_resume(std::coroutine_handle{});
                w.set_suspend_now();
            }
            return final_awaitable{m_this_coroutine};
        }
    };

    struct promise_void_mixin : public promise_base {
        void return_void()
            pre(promise_base::m_task_block->m_state.load(morder::relaxed) == future_state::non_complete) {
            promise_base::m_task_block->m_state.store(future_state::complete, morder::acq_rel);
        }
    };

    struct promise_value_mixin : public promise_base {
        void return_value(try_move_t<T> value)
            pre(promise_base::m_task_block->m_state.load(morder::acquire) == future_state::non_complete) {
            promise_base::m_task_block->m_storage.emplace(try_move(value));
            promise_base::m_task_block->m_state.store(future_state::complete, morder::release);
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

    bool is_empty() const { return m_this_coroutine; }

    void resume() pre(!is_empty()) { m_this_coroutine.resume(); }

private:
    join_handle(coroutine_handle this_coroutine, std::shared_ptr<task_block> task_block_ptr)
            : m_this_coroutine{this_coroutine}
            , m_task_block{task_block_ptr} {}

    coroutine_handle m_this_coroutine;
    std::shared_ptr<task_block> m_task_block;
};

};  // namespace jungle
