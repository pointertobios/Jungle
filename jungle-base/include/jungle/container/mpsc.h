// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <bit>
#include <expected>
#include <memory>
#include <tuple>
#include <vector>

#include "jungle/constants.h"
#include "jungle/types/concepts.h"
#include "jungle/types/int.h"
#include "jungle/types/raw_storage.h"
#include "jungle/types/types.h"

namespace jungle::container {

enum class receive_failed {
    pending,
    empty,
};

enum class slot_state {
    empty,
    occupying,
    available,
    tombstone,
};

template<concepts::non_void T>
class mpsc {
    struct slot {
        raw_storage<T> m_data;
        std::atomic<slot_state> m_state{slot_state::empty};
    };

public:
    class sender final {
        friend class mpsc;

    public:
        sender() = default;

        sender(std::shared_ptr<mpsc> payload)
                : m_payload{payload} {}

        sender(const sender &) = default;
        sender &operator=(const sender &) = default;

        sender(sender &&) = default;
        sender &operator=(sender &&) = default;

        bool is_valid() const { return m_payload != nullptr; }

        [[nodiscard]] bool send(try_move_t<T> value) pre(is_valid()) {
            usize location;
            while (true) {
                auto e = m_payload->m_tail.load(morder::acquire);
                if (mask(e + 1) == mask(m_payload->m_head.load(morder::acquire))) {
                    return false;
                }
                if (m_payload->m_tail.compare_exchange_weak(e, e + 1, morder::acq_rel, morder::relaxed)) {
                    location = e;
                    break;
                }
            }

            auto &s = m_payload->m_queue[mask(location)];

            for (auto e = slot_state::empty;
                 !s.m_state.compare_exchange_weak(e, slot_state::occupying, morder::acq_rel, morder::relaxed);
                 e = slot_state::empty) {}

            s.m_data.emplace(try_move(value));
            s.m_state.store(slot_state::available, morder::release);
            return true;
        }

    private:
        usize mask(usize x) const { return m_payload->mask(x); }

        std::shared_ptr<mpsc> m_payload{nullptr};
    };

    class receiver final {
        friend class mpsc;

    public:
        receiver() = default;

        receiver(std::shared_ptr<mpsc> payload)
                : m_payload{payload} {}

        receiver(const receiver &) = delete;
        receiver &operator=(const receiver &) = delete;

        receiver(receiver &&) = default;

        receiver &operator=(receiver &&) = default;

        bool is_valid() const { return m_payload != nullptr; }

        [[nodiscard]] std::expected<T, receive_failed> recv() pre(is_valid()) {
            usize location;

            while (true) {
                location = m_payload->m_head.load(morder::acquire);

                if (mask(location) == mask(m_payload->m_tail.load(morder::acquire))) {
                    return std::unexpected{receive_failed::empty};
                }

                auto &s = m_payload->m_queue[mask(location)];
                auto ss = s.m_state.load(morder::acquire);

                if (ss != slot_state::tombstone) {
                    if (ss != slot_state::available) {
                        return std::unexpected{receive_failed::pending};
                    }

                    if (!s.m_state.compare_exchange_weak(ss, slot_state::tombstone)) {
                        continue;
                    }
                }

                if (m_payload->m_head.compare_exchange_weak(
                        location, location + 1, morder::acq_rel, morder::relaxed)) {
                    break;
                }
            }

            auto &s = m_payload->m_queue[mask(location)];
            T res{try_move(*s.m_data.get())};
            s.m_data.destroy();
            s.m_state.store(slot_state::empty, morder::release);
            return try_move(res);
        }

    private:
        usize mask(usize x) const { return m_payload->mask(x); }

        std::shared_ptr<mpsc> m_payload{nullptr};
    };

    static std::tuple<sender, receiver> queue(usize size = 1023) {
        auto payload = std::make_shared<mpsc>(size);
        return {sender{payload}, receiver{payload}};
    }

    mpsc(usize size)
            : m_capacity_mask{(1 << std::bit_width(size)) - 1}
            , m_queue{1 << std::bit_width(size)} {}

private:
    usize mask(usize x) const { return x & m_capacity_mask; }

    const usize m_capacity_mask;
    std::vector<slot> m_queue;
    std::atomic<usize> m_tail{0};
    std::atomic<usize> m_head{0};
};

};  // namespace jungle::container
