// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include "jungle/sync/condition_variable.h"
#include "jungle/sync/mpsc.h"
#include "jungle/tasks/this_task.h"
#include "jungle/test/async_test.h"
#include "jungle/test/test.h"

using namespace jungle;
using namespace jungle::sync;
using jungle::container::receive_failed;

JUNGLE_SYNC_TEST(channel_creates_valid_sender_and_receiver) {
    auto [sender, receiver] = mpsc<int>::channel();

    JUNGLE_SYNC_ASSERT(sender.is_valid(), "sender 应为有效状态");
    JUNGLE_SYNC_ASSERT(receiver.is_valid(), "receiver 应为有效状态");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(default_sender_is_invalid) {
    mpsc<int>::sender s;

    JUNGLE_SYNC_ASSERT(!s.is_valid(), "默认构造的 sender 应为无效状态");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(default_receiver_is_invalid) {
    mpsc<int>::receiver r;

    JUNGLE_SYNC_ASSERT(!r.is_valid(), "默认构造的 receiver 应为无效状态");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(try_send_and_try_recv_round_trip) {
    auto [sender, receiver] = mpsc<int>::channel();

    JUNGLE_SYNC_ASSERT(sender.try_send(42), "try_send 应成功");
    auto val = receiver.try_recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "try_recv 应返回有效值");
    JUNGLE_SYNC_ASSERT(*val == 42, "收到的值应与发送一致");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(try_recv_on_empty_returns_empty_error) {
    auto [sender, receiver] = mpsc<int>::channel();

    auto val = receiver.try_recv();
    JUNGLE_SYNC_ASSERT(val.error() == receive_failed::empty, "空队列 try_recv 应返回 empty 错误");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(try_send_on_full_returns_false) {
    auto [sender, receiver] = mpsc<int>::channel(1);

    JUNGLE_SYNC_ASSERT(sender.try_send(1), "第一次 try_send 应成功");
    JUNGLE_SYNC_ASSERT(!sender.try_send(2), "队列满后 try_send 应返回 false");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(fifo_ordering) {
    auto [sender, receiver] = mpsc<int>::channel();

    JUNGLE_SYNC_ASSERT(sender.try_send(1), "发送 1 应成功");
    JUNGLE_SYNC_ASSERT(sender.try_send(2), "发送 2 应成功");
    JUNGLE_SYNC_ASSERT(sender.try_send(3), "发送 3 应成功");

    auto v1 = receiver.try_recv();
    auto v2 = receiver.try_recv();
    auto v3 = receiver.try_recv();

    JUNGLE_SYNC_ASSERT(v1.has_value() && *v1 == 1, "第一个收到的应为 1");
    JUNGLE_SYNC_ASSERT(v2.has_value() && *v2 == 2, "第二个收到的应为 2");
    JUNGLE_SYNC_ASSERT(v3.has_value() && *v3 == 3, "第三个收到的应为 3");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(sender_copy_shares_channel) {
    auto [s1, receiver] = mpsc<int>::channel(2);
    auto s2 = s1;

    JUNGLE_SYNC_ASSERT(s1.try_send(1), "s1 发送应成功");
    JUNGLE_SYNC_ASSERT(s2.try_send(2), "s2 发送应成功（共享容量）");

    auto v1 = receiver.try_recv();
    auto v2 = receiver.try_recv();
    JUNGLE_SYNC_ASSERT(v1.has_value() && *v1 == 1, "第一个收到的应为 1");
    JUNGLE_SYNC_ASSERT(v2.has_value() && *v2 == 2, "第二个收到的应为 2");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(receiver_move_transfers_ownership) {
    auto [sender, r1] = mpsc<int>::channel();

    JUNGLE_SYNC_ASSERT(r1.is_valid(), "r1 应为有效状态");
    auto r2 = std::move(r1);
    JUNGLE_SYNC_ASSERT(!r1.is_valid(), "移动后 r1 应为无效状态");
    JUNGLE_SYNC_ASSERT(r2.is_valid(), "移动后 r2 应为有效状态");

    JUNGLE_SYNC_ASSERT(sender.try_send(99), "发送应成功");
    auto val = r2.try_recv();
    JUNGLE_SYNC_ASSERT(val.has_value() && *val == 99, "移动后的 receiver 应能正常接收");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(sender_stopped_initially_false) {
    auto [sender, receiver] = mpsc<int>::channel();

    JUNGLE_SYNC_ASSERT(!sender.stopped(), "初始状态 sender 不应 stopped");
    JUNGLE_SYNC_ASSERT(!receiver.stopped(), "初始状态 receiver 不应 stopped");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(move_only_type) {
    auto [sender, receiver] = mpsc<std::unique_ptr<int>>::channel();

    JUNGLE_SYNC_ASSERT(sender.try_send(std::make_unique<int>(42)), "发送 unique_ptr 应成功");
    auto val = receiver.try_recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "应收到 unique_ptr");
    JUNGLE_SYNC_ASSERT(**val == 42, "收到的 unique_ptr 值应为 42");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(copy_only_type) {
    struct copy_only {
        int value;
        copy_only(int v)
                : value{v} {}
        copy_only(const copy_only &) = default;
        copy_only &operator=(const copy_only &) = default;
        copy_only(copy_only &&) = delete;
        copy_only &operator=(copy_only &&) = delete;
    };

    auto [sender, receiver] = mpsc<copy_only>::channel();

    copy_only obj{99};
    JUNGLE_SYNC_ASSERT(sender.try_send(obj), "发送 copy-only 类型应成功");
    JUNGLE_SYNC_ASSERT(obj.value == 99, "发送后原对象应保持不变");

    auto val = receiver.try_recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "应收到 copy-only 类型");
    JUNGLE_SYNC_ASSERT(val->value == 99, "收到的值应与发送一致");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(custom_small_capacity) {
    auto [sender, receiver] = mpsc<int>::channel(1);

    JUNGLE_SYNC_ASSERT(sender.try_send(100), "容量为 1 时应能发送一条消息");
    auto val = receiver.try_recv();
    JUNGLE_SYNC_ASSERT(val.has_value() && *val == 100, "收到的值应与发送一致");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(async_send_and_recv) {
    auto [sender, receiver] = mpsc<int>::channel();
    int value = 0;

    auto jh = this_task::spawn([&]() -> async::future<> {
        value = co_await receiver.recv();
        co_return;
    });

    JUNGLE_ASYNC_ASSERT(sender.try_send(42), "发送应成功");
    co_await jh;
    JUNGLE_ASYNC_ASSERT(value == 42, "异步接收的值应为 42");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(send_blocks_when_full_and_unblocks_on_recv) {
    auto [sender, receiver] = mpsc<int>::channel(1);
    bool sent = false;

    JUNGLE_ASYNC_ASSERT(sender.try_send(1), "应填满队列");

    auto jh = this_task::spawn([&]() -> async::future<> {
        sent = co_await sender.send(2);
        co_return;
    });

    auto val = receiver.try_recv();
    JUNGLE_ASYNC_ASSERT(val.has_value() && *val == 1, "应收到第一条消息");

    co_await jh;
    JUNGLE_ASYNC_ASSERT(sent, "解除阻塞后 send 应返回 true");

    auto val2 = receiver.try_recv();
    JUNGLE_ASYNC_ASSERT(val2.has_value() && *val2 == 2, "应收到第二条消息");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(recv_blocks_when_empty_and_unblocks_on_send) {
    auto [sender, receiver] = mpsc<int>::channel();
    int value = 0;

    auto jh = this_task::spawn([&]() -> async::future<> {
        value = co_await receiver.recv();
        co_return;
    });

    JUNGLE_ASYNC_ASSERT(sender.try_send(42), "发送应成功");
    co_await jh;
    JUNGLE_ASYNC_ASSERT(value == 42, "异步接收的值应为 42");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(stop_unblocks_blocked_senders) {
    auto [sender, receiver] = mpsc<int>::channel(1);
    bool send_returned = false;
    bool send_result = true;

    JUNGLE_ASYNC_ASSERT(sender.try_send(1), "应填满队列");

    auto jh = this_task::spawn([&]() -> async::future<> {
        send_result = co_await sender.send(2);
        send_returned = true;
        co_return;
    });

    receiver.stop();

    co_await jh;
    JUNGLE_ASYNC_ASSERT(send_returned, "stop 后 send 应返回");
    JUNGLE_ASYNC_ASSERT(!send_result, "stop 后 send 应返回 false");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(send_returns_false_after_stop) {
    auto [sender, receiver] = mpsc<int>::channel();
    bool send_result = true;

    receiver.stop();

    auto jh = this_task::spawn([&]() -> async::future<> {
        send_result = co_await sender.send(42);
        co_return;
    });

    co_await jh;
    JUNGLE_ASYNC_ASSERT(!send_result, "stop 后 send 应返回 false");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(multiple_async_senders_all_messages_arrive) {
    constexpr int sender_count = 4;
    constexpr int msgs_per_sender = 25;
    constexpr int total = sender_count * msgs_per_sender;
    auto [sender, receiver] = mpsc<int>::channel(total);
    std::atomic_int received{0};

    auto producer = [](mpsc<int>::sender s, int start, int count) -> async::future<> {
        for (int i = 0; i < count; ++i) {
            co_await s.send(start + i);
        }
        co_return;
    };

    auto jh1 = this_task::spawn(producer, sender, 0, msgs_per_sender);
    auto jh2 = this_task::spawn(producer, sender, 100, msgs_per_sender);
    auto jh3 = this_task::spawn(producer, sender, 200, msgs_per_sender);
    auto jh4 = this_task::spawn(producer, sender, 300, msgs_per_sender);

    auto consumer = [&]() -> async::future<> {
        for (int i = 0; i < total; ++i) {
            co_await receiver.recv();
            received.fetch_add(1, morder::relaxed);
        }
        co_return;
    };

    auto jhc = this_task::spawn(consumer);

    co_await jh1;
    co_await jh2;
    co_await jh3;
    co_await jh4;
    co_await jhc;

    JUNGLE_ASYNC_ASSERT(received.load() == total,
                        "所有发送者并发发送的消息都应被接收");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(multiple_senders_block_on_full_then_unblock_one_by_one) {
    auto [sender, receiver] = mpsc<int>::channel(1);
    std::atomic_int completed{0};
    condition_variable cv;

    JUNGLE_ASYNC_ASSERT(sender.try_send(0), "应填满容量为 1 的队列");

    auto blocked_sender = [&](int id) -> async::future<> {
        bool ok = co_await sender.send(id);
        if (ok) {
            completed.fetch_add(1, morder::release);
            cv.notify_one();
        }
        co_return;
    };

    auto jh1 = this_task::spawn(blocked_sender, 1);
    auto jh2 = this_task::spawn(blocked_sender, 2);
    auto jh3 = this_task::spawn(blocked_sender, 3);

    auto v0 = co_await receiver.recv();
    JUNGLE_ASYNC_ASSERT(v0 == 0, "第一条消息应为 0");
    co_await cv([&] { return completed.load(morder::acquire) == 1; });
    JUNGLE_ASYNC_ASSERT(completed.load(morder::acquire) == 1,
                        "消费一条消息后恰好唤醒一个发送者");

    co_await receiver.recv();
    co_await cv([&] { return completed.load(morder::acquire) == 2; });
    JUNGLE_ASYNC_ASSERT(completed.load(morder::acquire) == 2,
                        "消费两条消息后恰好唤醒两个发送者");

    co_await receiver.recv();
    co_await cv([&] { return completed.load(morder::acquire) == 3; });
    JUNGLE_ASYNC_ASSERT(completed.load(morder::acquire) == 3,
                        "消费三条消息后恰好唤醒三个发送者");

    co_await jh1;
    co_await jh2;
    co_await jh3;

    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(stop_unblocks_multiple_blocked_senders) {
    auto [sender, receiver] = mpsc<int>::channel(1);
    std::atomic_int unblocked{0};

    JUNGLE_ASYNC_ASSERT(sender.try_send(0), "应填满容量为 1 的队列");

    auto blocked_sender = [&](int) -> async::future<> {
        bool ok = co_await sender.send(42);
        if (!ok) {
            unblocked.fetch_add(1, morder::release);
        }
        co_return;
    };

    auto jh1 = this_task::spawn(blocked_sender, 1);
    auto jh2 = this_task::spawn(blocked_sender, 2);
    auto jh3 = this_task::spawn(blocked_sender, 3);

    receiver.stop();

    co_await jh1;
    co_await jh2;
    co_await jh3;

    JUNGLE_ASYNC_ASSERT(unblocked.load(morder::acquire) == 3,
                        "stop 应唤醒所有阻塞的发送者并使其返回 false");
    JUNGLE_ASYNC_SUCCESS();
}

JUNGLE_ASYNC_TEST(concurrent_try_send_from_multiple_senders_all_arrive) {
    constexpr int sender_count = 4;
    constexpr int msgs_per_sender = 25;
    constexpr int total = sender_count * msgs_per_sender;
    auto [sender, receiver] = mpsc<int>::channel(total);
    std::atomic_int received{0};

    auto producer = [](mpsc<int>::sender s, int start, int count) -> async::future<> {
        for (int i = 0; i < count; ++i) {
            s.try_send(start + i);
        }
        co_return;
    };

    auto jh1 = this_task::spawn(producer, sender, 0, msgs_per_sender);
    auto jh2 = this_task::spawn(producer, sender, 100, msgs_per_sender);
    auto jh3 = this_task::spawn(producer, sender, 200, msgs_per_sender);
    auto jh4 = this_task::spawn(producer, sender, 300, msgs_per_sender);

    auto consumer = [&]() -> async::future<> {
        for (int i = 0; i < total; ++i) {
            co_await receiver.recv();
            received.fetch_add(1, morder::relaxed);
        }
        co_return;
    };

    auto jhc = this_task::spawn(consumer);

    co_await jh1;
    co_await jh2;
    co_await jh3;
    co_await jh4;
    co_await jhc;

    JUNGLE_ASYNC_ASSERT(received.load() == total,
                        "多个发送者并发 try_send 的所有消息都应到达");
    JUNGLE_ASYNC_SUCCESS();
}
