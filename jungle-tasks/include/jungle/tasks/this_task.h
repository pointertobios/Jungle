// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <coroutine>

#include "jungle/tasks/runtime/worker.h"

namespace jungle::tasks::this_task {

inline auto yield() {
    struct yield_awaitable {
        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> handle) {
            auto &w = runtime::worker::current();
            w.set_next_resume(handle);
            w.set_yield_now();
        }

        void await_resume() const noexcept {}
    };
    return yield_awaitable{};
}

};  // namespace jungle::tasks::this_task

namespace jungle {

namespace this_task = tasks::this_task;

};  // namespace jungle
