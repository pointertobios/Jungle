// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <print>
#include <string>
#include <thread>

#include "jungle/sync/rwspinlock.h"
#include "jungle/test/test.h"

using namespace jungle::sync;

JUNGLE_SYNC_TEST(void_try_read_succeeds_when_uncontested) {
    rwspinlock<> lock;

    auto g = lock.try_read();
    JUNGLE_SYNC_ASSERT(g, "try_read 应在无竞争时成功获取读锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_try_write_succeeds_when_uncontested) {
    rwspinlock<> lock;

    auto g = lock.try_write();
    JUNGLE_SYNC_ASSERT(g, "try_write 应在无竞争时成功获取写锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_try_read_fails_when_write_held) {
    rwspinlock<> lock;

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg, "写锁应先成功获取");

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(!rg, "写锁持有时 try_read 应失败");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_try_write_fails_when_read_held) {
    rwspinlock<> lock;

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "读锁应先成功获取");

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(!wg, "读锁持有时 try_write 应失败");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_try_write_fails_when_write_held) {
    rwspinlock<> lock;

    auto wg1 = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg1, "第一个写锁应先成功获取");

    auto wg2 = lock.try_write();
    JUNGLE_SYNC_ASSERT(!wg2, "写锁持有时第二个 try_write 应失败");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_multiple_try_read_succeed) {
    rwspinlock<> lock;

    auto rg1 = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg1, "第一个读锁应成功");

    auto rg2 = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg2, "第二个读锁应成功（共享读）");

    auto rg3 = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg3, "第三个读锁应成功（共享读）");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_guard_destruction_releases_lock) {
    rwspinlock<> lock;

    {
        auto wg = lock.try_write();
        JUNGLE_SYNC_ASSERT(wg, "写锁应先成功获取");
    }

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "写锁释放后 try_read 应成功");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_read_guard_destruction_allows_write) {
    rwspinlock<> lock;

    {
        auto rg = lock.try_read();
        JUNGLE_SYNC_ASSERT(rg, "读锁应先成功获取");
    }

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg, "所有读锁释放后 try_write 应成功");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_default_constructed_guard_is_falsy) {
    rwspinlock<>::read_guard rg;
    JUNGLE_SYNC_ASSERT(!rg, "默认构造的 read_guard 应为假");

    rwspinlock<>::write_guard wg;
    JUNGLE_SYNC_ASSERT(!wg, "默认构造的 write_guard 应为假");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_read_guard_move_constructor) {
    rwspinlock<> lock;

    auto rg1 = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg1, "初始 read_guard 应为真");

    auto rg2 = std::move(rg1);
    JUNGLE_SYNC_ASSERT(rg2, "移动构造后的 read_guard 应为真");
    JUNGLE_SYNC_ASSERT(!rg1, "移动源 read_guard 应为假");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_read_guard_move_assignment) {
    rwspinlock<> lock;

    auto rg1 = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg1, "初始 read_guard 应为真");

    rwspinlock<>::read_guard rg2;
    rg2 = std::move(rg1);
    JUNGLE_SYNC_ASSERT(rg2, "移动赋值后的 read_guard 应为真");
    JUNGLE_SYNC_ASSERT(!rg1, "移动源 read_guard 应为假");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_write_guard_move_constructor) {
    rwspinlock<> lock;

    auto wg1 = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg1, "初始 write_guard 应为真");

    auto wg2 = std::move(wg1);
    JUNGLE_SYNC_ASSERT(wg2, "移动构造后的 write_guard 应为真");
    JUNGLE_SYNC_ASSERT(!wg1, "移动源 write_guard 应为假");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_write_guard_move_assignment) {
    rwspinlock<> lock;

    auto wg1 = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg1, "初始 write_guard 应为真");

    rwspinlock<>::write_guard wg2;
    wg2 = std::move(wg1);
    JUNGLE_SYNC_ASSERT(wg2, "移动赋值后的 write_guard 应为真");
    JUNGLE_SYNC_ASSERT(!wg1, "移动源 write_guard 应为假");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_read_succeeds_when_uncontested) {
    rwspinlock<> lock;

    auto rg = lock.read();
    JUNGLE_SYNC_ASSERT(rg, "read() 应在无竞争时成功获取读锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_write_succeeds_when_uncontested) {
    rwspinlock<> lock;

    auto wg = lock.write();
    JUNGLE_SYNC_ASSERT(wg, "write() 应在无竞争时成功获取写锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_read_blocks_until_write_released) {
    rwspinlock<> lock;

    auto wg = lock.write();
    JUNGLE_SYNC_ASSERT(wg, "主线程应先获取写锁");

    bool read_acquired{false};
    std::jthread reader{[&lock, &read_acquired]() {
        auto rg = lock.read();
        read_acquired = true;
    }};

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    JUNGLE_SYNC_ASSERT(!read_acquired, "写锁持有时 reader 应阻塞");

    wg = {};
    JUNGLE_SYNC_ASSERT(!wg, "写锁应已释放");

    reader.join();
    JUNGLE_SYNC_ASSERT(read_acquired, "写锁释放后 reader 应成功获取读锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_write_blocks_until_read_released) {
    rwspinlock<> lock;

    auto rg = lock.read();
    JUNGLE_SYNC_ASSERT(rg, "主线程应先获取读锁");

    bool write_acquired{false};
    std::jthread writer{[&lock, &write_acquired]() {
        auto wg = lock.write();
        write_acquired = true;
    }};

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    JUNGLE_SYNC_ASSERT(!write_acquired, "读锁持有时 writer 应阻塞");

    rg = {};
    JUNGLE_SYNC_ASSERT(!rg, "读锁应已释放");

    writer.join();
    JUNGLE_SYNC_ASSERT(write_acquired, "读锁释放后 writer 应成功获取写锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(void_write_willing_blocks_new_readers) {
    rwspinlock<> lock;

    auto rg = lock.read();
    JUNGLE_SYNC_ASSERT(rg, "主线程应先获取读锁");

    bool write_acquired{false};

    std::jthread writer{[&lock, &write_acquired]() {
        auto wg = lock.write();
        write_acquired = true;
    }};

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto rg2 = lock.try_read();
    JUNGLE_SYNC_ASSERT(!rg2, "write_willing 时新的 try_read 应失败");

    rg = {};
    writer.join();
    JUNGLE_SYNC_ASSERT(write_acquired, "所有读锁释放后 writer 应成功获取写锁");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_read_guard_const_access) {
    rwspinlock<int> lock{42};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "try_read 应成功");

    JUNGLE_SYNC_ASSERT(*rg == 42, "const operator* 应返回 42");
    JUNGLE_SYNC_ASSERT(rg.operator->() != nullptr, "const operator-> 应返回有效指针");

    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_write_guard_mutable_access) {
    rwspinlock<int> lock{42};

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg, "try_write 应成功");

    JUNGLE_SYNC_ASSERT(*wg == 42, "operator* 应返回 42");
    *wg = 100;
    JUNGLE_SYNC_ASSERT(*wg == 100, "写入后 operator* 应返回 100");

    wg = {};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "释放写锁后 try_read 应成功");
    JUNGLE_SYNC_ASSERT(*rg == 100, "读锁应看到写入后的值");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_write_guard_arrow_access) {
    rwspinlock<std::string> lock{std::string{"hello"}};

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg, "try_write 应成功");

    JUNGLE_SYNC_ASSERT(wg->size() == 5, "operator-> 应能访问成员");
    wg->append(" world");
    JUNGLE_SYNC_ASSERT(*wg == "hello world", "修改后值应正确");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_read_guard_with_readmutable) {
    rwspinlock<int, true> lock{42};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "try_read 应成功");

    *rg = 99;
    JUNGLE_SYNC_ASSERT(*rg == 99, "ReadMutable 时 operator* 应允许写入");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_constructor_from_args) {
    rwspinlock<std::string> lock{"hello", 3};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "try_read 应成功");
    JUNGLE_SYNC_ASSERT(*rg == "hel", "构造参数应正确转发");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_copy_constructor) {
    std::string src{"copy_test"};
    rwspinlock<std::string> lock{src};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "try_read 应成功");
    JUNGLE_SYNC_ASSERT(*rg == "copy_test", "拷贝构造的值应正确");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_move_constructor) {
    std::string src{"move_test"};
    rwspinlock<std::string> lock{std::move(src)};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "try_read 应成功");
    JUNGLE_SYNC_ASSERT(*rg == "move_test", "移动构造的值应正确");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_read_blocking) {
    rwspinlock<int> lock{7};

    auto rg = lock.read();
    JUNGLE_SYNC_ASSERT(rg, "read() 应成功");
    JUNGLE_SYNC_ASSERT(*rg == 7, "应能读取值");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_write_blocking) {
    rwspinlock<int> lock{7};

    auto wg = lock.write();
    JUNGLE_SYNC_ASSERT(wg, "write() 应成功");
    *wg = 77;
    JUNGLE_SYNC_ASSERT(*wg == 77, "应能写入值");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_try_read_fails_when_write_held) {
    rwspinlock<int> lock{1};

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg, "写锁应先成功获取");

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(!rg, "写锁持有时 try_read 应失败");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_try_write_fails_when_read_held) {
    rwspinlock<int> lock{1};

    auto rg = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg, "读锁应先成功获取");

    auto wg = lock.try_write();
    JUNGLE_SYNC_ASSERT(!wg, "读锁持有时 try_write 应失败");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_guard_move_semantics) {
    rwspinlock<int> lock{42};

    auto wg1 = lock.try_write();
    JUNGLE_SYNC_ASSERT(wg1, "写锁应成功获取");
    *wg1 = 99;

    auto wg2 = std::move(wg1);
    JUNGLE_SYNC_ASSERT(wg2, "移动后目标 guard 应为真");
    JUNGLE_SYNC_ASSERT(!wg1, "移动后源 guard 应为假");
    JUNGLE_SYNC_ASSERT(*wg2 == 99, "移动后应能访问原值");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(t_multiple_readers_see_same_value) {
    rwspinlock<int> lock{77};

    auto rg1 = lock.try_read();
    auto rg2 = lock.try_read();
    JUNGLE_SYNC_ASSERT(rg1 && rg2, "两个读锁应同时成功");

    JUNGLE_SYNC_ASSERT(*rg1 == 77, "rg1 应读到 77");
    JUNGLE_SYNC_ASSERT(*rg2 == 77, "rg2 应读到 77");
    JUNGLE_SYNC_SUCCESS();
}
