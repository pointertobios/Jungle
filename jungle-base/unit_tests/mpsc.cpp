// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/container/mpsc.h"
#include "jungle/test/test.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace jungle;
using jungle::container::mpsc;

JUNGLE_SYNC_TEST(mpsc_queue_creates_sender_receiver_pair) {
    auto [sender, receiver] = mpsc<int>::queue();

    JUNGLE_SYNC_ASSERT(sender.send(42), "send to default capacity queue should succeed");
    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "recv after send should return a value");
    JUNGLE_SYNC_ASSERT(*val == 42, "received value should match sent value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_single_send_recv) {
    auto [sender, receiver] = mpsc<int>::queue();

    JUNGLE_SYNC_ASSERT(sender.send(7), "send should succeed on empty queue");
    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "recv should return value after send");
    JUNGLE_SYNC_ASSERT(*val == 7, "received value should be 7");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_empty_queue_recv_returns_nullopt) {
    auto [sender, receiver] = mpsc<int>::queue();

    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(!val.has_value(), "recv on empty queue should return nullopt");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_fifo_ordering) {
    auto [sender, receiver] = mpsc<int>::queue();

    JUNGLE_SYNC_ASSERT(sender.send(1), "send 1 should succeed");
    JUNGLE_SYNC_ASSERT(sender.send(2), "send 2 should succeed");
    JUNGLE_SYNC_ASSERT(sender.send(3), "send 3 should succeed");
    JUNGLE_SYNC_ASSERT(sender.send(4), "send 4 should succeed");
    JUNGLE_SYNC_ASSERT(sender.send(5), "send 5 should succeed");

    for (int expected = 1; expected <= 5; ++expected) {
        auto val = receiver.recv();
        JUNGLE_SYNC_ASSERT(val.has_value(), "recv should return a value");
        JUNGLE_SYNC_ASSERT(*val == expected, "values should be received in FIFO order");
    }
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_at_least_requested_capacity) {
    constexpr usize requested = 4;
    auto [sender, receiver] = mpsc<int>::queue(requested);

    usize sent = 0;
    while (sender.send(static_cast<int>(sent))) {
        ++sent;
    }

    JUNGLE_SYNC_ASSERT(sent >= requested, "actual capacity should be at least the requested size");

    for (usize i = 0; i < sent; ++i) {
        auto val = receiver.recv();
        JUNGLE_SYNC_ASSERT(val.has_value(), "recv should return value for each sent item");
        JUNGLE_SYNC_ASSERT(*val == static_cast<int>(i), "values should match sent order");
    }

    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_full_queue_then_drain_then_reuse) {
    auto [sender, receiver] = mpsc<int>::queue(4);

    usize first_count = 0;
    while (sender.send(static_cast<int>(first_count * 10))) {
        ++first_count;
    }
    JUNGLE_SYNC_ASSERT(first_count >= 4, "first fill should reach at least requested capacity");

    for (usize i = 0; i < first_count; ++i) {
        auto val = receiver.recv();
        JUNGLE_SYNC_ASSERT(val.has_value(), "first drain recv should return value");
        JUNGLE_SYNC_ASSERT(*val == static_cast<int>(i * 10), "first drain value should match");
    }

    usize second_count = 0;
    while (sender.send(static_cast<int>(second_count * 100))) {
        ++second_count;
    }
    JUNGLE_SYNC_ASSERT(second_count == first_count, "reuse should fill to the same capacity");

    for (usize i = 0; i < second_count; ++i) {
        auto val = receiver.recv();
        JUNGLE_SYNC_ASSERT(val.has_value(), "reuse drain recv should return value");
        JUNGLE_SYNC_ASSERT(*val == static_cast<int>(i * 100), "reuse drain value should match");
    }

    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_multiple_senders_interleaved) {
    auto [s1, receiver] = mpsc<int>::queue();
    auto s2 = s1;
    auto s3 = s1;

    JUNGLE_SYNC_ASSERT(s1.send(10), "sender 1 should send");
    JUNGLE_SYNC_ASSERT(s2.send(20), "sender 2 should send");
    JUNGLE_SYNC_ASSERT(s3.send(30), "sender 3 should send");
    JUNGLE_SYNC_ASSERT(s1.send(40), "sender 1 should send again");

    auto v1 = receiver.recv();
    auto v2 = receiver.recv();
    auto v3 = receiver.recv();
    auto v4 = receiver.recv();

    JUNGLE_SYNC_ASSERT(v1.has_value() && *v1 == 10, "first recv should be 10");
    JUNGLE_SYNC_ASSERT(v2.has_value() && *v2 == 20, "second recv should be 20");
    JUNGLE_SYNC_ASSERT(v3.has_value() && *v3 == 30, "third recv should be 30");
    JUNGLE_SYNC_ASSERT(v4.has_value() && *v4 == 40, "fourth recv should be 40");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_sender_copy_shares_storage) {
    auto [s1, receiver] = mpsc<int>::queue(2);
    auto s2 = s1;

    JUNGLE_SYNC_ASSERT(s1.send(1), "s1 first send should succeed");
    JUNGLE_SYNC_ASSERT(s2.send(2), "s2 send should succeed (shared capacity)");

    auto v1 = receiver.recv();
    auto v2 = receiver.recv();
    JUNGLE_SYNC_ASSERT(v1.has_value() && *v1 == 1, "first value should be 1");
    JUNGLE_SYNC_ASSERT(v2.has_value() && *v2 == 2, "second value should be 2");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_supports_move_only_types) {
    auto [sender, receiver] = mpsc<std::unique_ptr<int>>::queue();

    JUNGLE_SYNC_ASSERT(sender.send(std::make_unique<int>(42)), "send unique_ptr should succeed");
    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "recv unique_ptr should return value");
    JUNGLE_SYNC_ASSERT(**val == 42, "received unique_ptr should hold correct value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_supports_copy_only_types) {
    struct copy_only {
        int value;
        copy_only(int v)
                : value{v} {}
        copy_only(const copy_only &) = default;
        copy_only &operator=(const copy_only &) = default;
        copy_only(copy_only &&) = delete;
        copy_only &operator=(copy_only &&) = delete;
    };

    auto [sender, receiver] = mpsc<copy_only>::queue();

    copy_only obj{99};
    JUNGLE_SYNC_ASSERT(sender.send(obj), "send copy-only type should succeed");
    JUNGLE_SYNC_ASSERT(obj.value == 99, "original object should remain valid after send");

    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "recv copy-only type should return value");
    JUNGLE_SYNC_ASSERT(val->value == 99, "received copy-only value should match");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_supports_string_type) {
    auto [sender, receiver] = mpsc<std::string>::queue();

    JUNGLE_SYNC_ASSERT(sender.send(std::string{"hello"}), "send string should succeed");
    JUNGLE_SYNC_ASSERT(sender.send(std::string{"world"}), "send second string should succeed");

    auto v1 = receiver.recv();
    auto v2 = receiver.recv();

    JUNGLE_SYNC_ASSERT(v1.has_value() && *v1 == "hello", "first string should be 'hello'");
    JUNGLE_SYNC_ASSERT(v2.has_value() && *v2 == "world", "second string should be 'world'");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_custom_small_capacity) {
    auto [sender, receiver] = mpsc<int>::queue(1);

    JUNGLE_SYNC_ASSERT(sender.send(100), "send with requested size 1 should succeed");

    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value() && *val == 100, "recv should return the sent value");

    JUNGLE_SYNC_ASSERT(sender.send(300), "send after drain should succeed again");
    auto val2 = receiver.recv();
    JUNGLE_SYNC_ASSERT(val2.has_value() && *val2 == 300, "recv after reuse should return new value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_large_capacity) {
    constexpr usize requested = 1024;
    auto [sender, receiver] = mpsc<int>::queue(requested);

    for (usize i = 0; i < requested; ++i) {
        JUNGLE_SYNC_ASSERT(sender.send(static_cast<int>(i)), "send within requested capacity should succeed");
    }

    for (usize i = 0; i < requested; ++i) {
        auto val = receiver.recv();
        JUNGLE_SYNC_ASSERT(val.has_value(), "recv should return value for each sent item");
        JUNGLE_SYNC_ASSERT(*val == static_cast<int>(i), "value should match sent order");
    }
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_interleaved_send_recv) {
    auto [sender, receiver] = mpsc<int>::queue(4);

    JUNGLE_SYNC_ASSERT(sender.send(1), "send 1");
    JUNGLE_SYNC_ASSERT(sender.send(2), "send 2");

    auto v1 = receiver.recv();
    JUNGLE_SYNC_ASSERT(v1.has_value() && *v1 == 1, "recv 1");

    JUNGLE_SYNC_ASSERT(sender.send(3), "send 3 after partial drain");
    JUNGLE_SYNC_ASSERT(sender.send(4), "send 4");

    auto v2 = receiver.recv();
    auto v3 = receiver.recv();
    auto v4 = receiver.recv();

    JUNGLE_SYNC_ASSERT(v2.has_value() && *v2 == 2, "recv 2");
    JUNGLE_SYNC_ASSERT(v3.has_value() && *v3 == 3, "recv 3");
    JUNGLE_SYNC_ASSERT(v4.has_value() && *v4 == 4, "recv 4");

    auto v5 = receiver.recv();
    JUNGLE_SYNC_ASSERT(!v5.has_value(), "recv after drain should return nullopt");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_large_object_move_semantics) {
    struct large {
        int data[64];
        bool moved_from{false};
        large()
                : data{} {}
        large(int fill) {
            for (auto &d : data)
                d = fill;
        }
        large(large &&other)
                : moved_from{false} {
            for (usize i = 0; i < 64; ++i)
                data[i] = other.data[i];
            other.moved_from = true;
        }
        large &operator=(large &&other) {
            if (this != &other) {
                for (usize i = 0; i < 64; ++i)
                    data[i] = other.data[i];
                other.moved_from = true;
            }
            return *this;
        }
        large(const large &) = delete;
        large &operator=(const large &) = delete;
    };

    auto [sender, receiver] = mpsc<large>::queue(4);

    large obj{7};
    JUNGLE_SYNC_ASSERT(sender.send(std::move(obj)), "send large move-only object should succeed");
    JUNGLE_SYNC_ASSERT(obj.moved_from, "original object should be moved-from after send");

    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value(), "recv large object should return value");
    JUNGLE_SYNC_ASSERT(!val->moved_from, "received object should not be moved-from");
    JUNGLE_SYNC_ASSERT(val->data[0] == 7 && val->data[63] == 7, "received data should match sent values");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_sender_move_constructor) {
    auto [s1, receiver] = mpsc<int>::queue();
    auto s2 = std::move(s1);

    JUNGLE_SYNC_ASSERT(s2.send(55), "moved-to sender should be able to send");
    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value() && *val == 55, "received value from moved sender should match");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_receiver_move_constructor) {
    auto [sender, r1] = mpsc<int>::queue();
    JUNGLE_SYNC_ASSERT(sender.send(77), "send before receiver move");

    auto r2 = std::move(r1);
    auto val = r2.recv();
    JUNGLE_SYNC_ASSERT(
        val.has_value() && *val == 77, "moved-to receiver should receive previously sent value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(mpsc_repeated_empty_recv) {
    auto [sender, receiver] = mpsc<int>::queue();

    for (int i = 0; i < 5; ++i) {
        auto val = receiver.recv();
        JUNGLE_SYNC_ASSERT(!val.has_value(), "repeated recv on empty queue should always return nullopt");
    }

    JUNGLE_SYNC_ASSERT(sender.send(1), "send after repeated empty recvs should succeed");
    auto val = receiver.recv();
    JUNGLE_SYNC_ASSERT(val.has_value() && *val == 1, "recv after repeated empties should work");
    JUNGLE_SYNC_SUCCESS();
}
