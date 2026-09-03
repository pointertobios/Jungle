// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "jungle/async/future.h"
#include "jungle/container/mpsc.h"
#include "jungle/sync/condition_variable.h"
#include "jungle/types/concepts.h"

namespace jungle::sync {

template<concepts::non_void T>
class mpsc final : public container::mpsc<T> {
    using sync_mpsc = container::mpsc<T>;

public:
    class sender final {
        friend class mpsc;

    public:
        sender() = default;

        sender(const sender &) = default;
        sender &operator=(const sender &) = default;

        sender(sender &&) = default;
        sender &operator=(sender &&) = default;

        bool is_valid() const { return m_payload != nullptr; }

        auto try_send(try_move_t<T> value) pre(is_valid()) {
            return m_sync_sender.send(std::forward<decltype(value)>(value));
        }

        async::future<bool> send(try_move_t<T> value) pre(is_valid()) {
            while (true) {
                if (stopped()) {
                    co_return false;
                }
                if (m_sync_sender.send(std::forward<decltype(value)>(value))) {
                    m_payload->m_recv_cv.notify_one();
                    co_return true;
                }
                co_await m_payload->m_send_cv();
            }
        }

        bool stopped() const { return m_payload->m_stop.load(morder::acquire); }

    private:
        sender(std::shared_ptr<mpsc> payload)
                : m_payload{payload}
                , m_sync_sender{payload} {}

        std::shared_ptr<mpsc> m_payload{nullptr};
        sync_mpsc::sender m_sync_sender{};
    };

    class receiver final {
        friend class mpsc;

    public:
        receiver() = default;

        receiver(const receiver &) = delete;
        receiver &operator=(const receiver &) = delete;

        receiver(receiver &&) = default;

        receiver &operator=(receiver &&) = default;

        bool is_valid() const { return m_payload != nullptr; }

        auto try_recv() pre(is_valid()) { return m_sync_receiver.recv(); }

        async::future<T> recv() pre(is_valid() && !stopped()) {
            while (true) {
                if (auto res = m_sync_receiver.recv(); res.has_value()) {
                    m_payload->m_send_cv.notify_one();
                    co_return try_move(res.value());
                } else if (res.error() == container::receive_failed::pending) {
                    continue;
                }
                co_await m_payload->m_recv_cv([&sync_receiver = this->m_sync_receiver] {
                    return sync_receiver.test_recv().has_value();
                });
            }
        }

        void stop() const {
            m_payload->m_stop.store(true, morder::release);
            m_payload->m_send_cv.notify_all();
        }

        bool stopped() const { return m_payload->m_stop.load(morder::acquire); }

    private:
        receiver(std::shared_ptr<mpsc> payload)
                : m_payload{payload}
                , m_sync_receiver{payload} {}

        std::shared_ptr<mpsc> m_payload{nullptr};
        sync_mpsc::receiver m_sync_receiver{};
    };

    static std::tuple<sender, receiver> channel(usize size = 1023) {
        auto payload = std::make_shared<mpsc>(size);
        return {sender{payload}, receiver{payload}};
    }

    mpsc(usize size)
            : sync_mpsc{size} {}

private:
    std::atomic_bool m_stop{false};
    condition_variable m_send_cv{};
    condition_variable m_recv_cv{};
};

};  // namespace jungle::sync
