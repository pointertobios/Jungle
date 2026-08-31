// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/sync/condition_variable.h"
#include "jungle/tasks/this_task.h"
#include "jungle/test/async_test.h"
#include "jungle/test/test.h"

using namespace jungle;
using namespace jungle::sync;

JUNGLE_SYNC_TEST(condition_variable_default_construction_and_destruction) {
    condition_variable cv;
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(notify_one_with_no_waiters_returns_false) {
    condition_variable cv;

    auto result = cv.notify_one();
    JUNGLE_SYNC_ASSERT(!result, "无等待者时 notify_one 应返回 false");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(notify_all_with_no_waiters_returns_zero) {
    condition_variable cv;

    auto count = cv.notify_all();
    JUNGLE_SYNC_ASSERT(count == 0, "无等待者时 notify_all 应返回 0");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(wait_and_notify_one_resumes_waiter) {
    condition_variable cv;
    bool ready = false;
    bool woken = false;

    auto jh = this_task::spawn([&]() -> async::future<> {
        co_await cv([&] { return ready; });
        woken = true;
    });

    ready = true;
    cv.notify_one();

    co_await jh;
    JUNGLE_ASYNC_ASSERT(woken, "notify_one 后等待者应被唤醒");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(predicate_wait_resumes_when_condition_becomes_true) {
    condition_variable cv;
    bool ready = false;
    bool done = false;

    auto jh = this_task::spawn([&]() -> async::future<> {
        co_await cv([&] { return ready; });
        done = true;
    });

    ready = true;
    cv.notify_one();

    co_await jh;
    JUNGLE_ASYNC_ASSERT(done, "谓词成立后等待者应恢复执行");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(notify_all_wakes_multiple_waiters) {
    condition_variable cv;
    bool ready = false;
    int woken_count = 0;

    auto waiter = [&]() -> async::future<> {
        co_await cv([&] { return ready; });
        woken_count++;
    };

    auto jh1 = this_task::spawn(waiter);
    auto jh2 = this_task::spawn(waiter);
    auto jh3 = this_task::spawn(waiter);

    ready = true;
    cv.notify_all();

    co_await jh1;
    co_await jh2;
    co_await jh3;

    JUNGLE_ASYNC_ASSERT(woken_count == 3, "所有等待者应被唤醒");
    JUNGLE_ASYNC_SUCCESS();
}
