// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/sync/spinlock.h"
#include "jungle/tasks/runtime/runtime.h"
#include "jungle/tasks/this_task.h"
#include "jungle/test/async_test.h"
#include <print>

using namespace jungle;
using namespace jungle::sync;

JUNGLE_SYNC_TEST(void_try_lock_succeeds_when_uncontested) {
    spinlock<> lock;

    auto g = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g, "try_lock 应在无竞争时成功获取锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_try_lock_fails_when_already_locked) {
    spinlock<> lock;

    auto g1 = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g1, "第一次 try_lock 应成功");

    auto g2 = lock.try_lock();
    JUNGLE_SYNC_ASSERT(!g2, "已持有时 try_lock 应失败（互斥）");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_guard_destruction_releases_lock) {
    spinlock<> lock;

    {
        auto g = lock.try_lock();
        JUNGLE_SYNC_ASSERT(g, "应先成功获取锁");
    }

    auto g = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g, "guard 析构后 try_lock 应可重新获取");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(void_lock_spins_until_acquired) {
    spinlock<> lock;
    bool acquired = false;

    auto g1 = lock.try_lock();
    JUNGLE_ASYNC_ASSERT(g1, "主协程应先持有锁");

    auto jh = tasks::spawn([&]() -> async::future<> {
        auto g = lock.lock();
        acquired = true;
        co_return;
    });

    co_await this_task::yield();
    JUNGLE_ASYNC_ASSERT(!acquired, "锁持有时 spawn 的任务应阻塞");

    g1 = {};
    co_await jh;
    JUNGLE_ASYNC_ASSERT(acquired, "释放后 spawn 的任务应成功获取锁");

    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_guard_move_transfers_ownership) {
    spinlock<> lock;

    auto g1 = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g1, "g1 应先成功获取锁");

    auto g2 = std::move(g1);
    JUNGLE_SYNC_ASSERT(!g1, "move 后 g1 应为空");
    JUNGLE_SYNC_ASSERT(g2, "move 后 g2 应持有锁");

    g2 = {};

    auto g3 = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g3, "g2 释放后 g3 应可重新获取");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_value_default_constructed) {
    spinlock<int> lock;

    auto g = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g, "应先成功获取锁");
    JUNGLE_SYNC_ASSERT(*g == 0, "默认构造的值应为 0");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_value_constructed_and_accessed) {
    spinlock<int> lock{42};

    auto g = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g, "应先成功获取锁");
    JUNGLE_SYNC_ASSERT(*g == 42, "应能读取构造时传入的值");
    *g = 100;
    JUNGLE_SYNC_ASSERT(*g == 100, "应能通过 guard 修改值");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_value_guard_move_preserves_access) {
    spinlock<int> lock{7};

    auto g1 = lock.try_lock();
    JUNGLE_SYNC_ASSERT(g1, "g1 应先成功获取锁");
    JUNGLE_SYNC_ASSERT(*g1 == 7, "g1 应能访问值");

    auto g2 = std::move(g1);
    JUNGLE_SYNC_ASSERT(!g1, "move 后 g1 应为空");
    JUNGLE_SYNC_ASSERT(g2, "move 后 g2 应持有锁");
    JUNGLE_SYNC_ASSERT(*g2 == 7, "move 后 g2 应仍能访问值");
    JUNGLE_SYNC_SUCCESS();
}
