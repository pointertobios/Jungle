// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <bit>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include "jungle/constants.h"
#include "jungle/types/concepts.h"
#include "jungle/types/int.h"
#include "jungle/types/raw_storage.h"
#include "jungle/types/types.h"

namespace jungle::container {

template<concepts::non_void T>
class mpsc {
    struct slot {
        raw_storage<T> m_data;
        std::atomic_bool m_commited{false};
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
            if (mask(m_payload->m_tail.load(morder::acquire) + 1)
                == mask(m_payload->m_head.load(morder::acquire))) {
                return false;
            }
            auto location = mask(m_payload->m_tail.fetch_add(1, morder::acq_rel));
            auto &s = m_payload->m_queue[location];
            s.m_data.emplace(try_move(value));
            s.m_commited.store(true, morder::release);
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

        [[nodiscard]] std::optional<T> recv() pre(is_valid()) {
            auto location = mask(m_payload->m_head.load(morder::acquire));
            auto &s = m_payload->m_queue[location];
            if (!s.m_commited.load(morder::acquire)) {
                return std::nullopt;
            }
            std::optional<T> res(try_move(*s.m_data.get()));
            s.m_data.destroy();
            s.m_commited.store(false, morder::release);
            m_payload->m_head.fetch_add(1, morder::release);
            return res;
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
    std::atomic<usize> m_tail;
    std::atomic<usize> m_head;
};

};  // namespace jungle::container
