// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <tuple>
#include <vector>

#include "jungle/preusing.h"

namespace jungle::container {

template<typename T>
class mpsc final {
public:
    class sender final {
    public:
        sender(std::shared_ptr<mpsc> payload)
                : m_payload{payload} {}

    private:
        std::shared_ptr<mpsc> m_payload;
    };

    class receiver final {
    public:
        receiver(std::shared_ptr<mpsc> payload)
                : m_payload{payload} {}

    private:
        std::shared_ptr<mpsc> m_payload;
    };

    static std::tuple<sender, receiver> queue(usize size = 1024) {
        auto payload = std::make_shared<mpsc>(size);
        return {sender{payload}, receiver{payload}};
    }

    mpsc(usize size)
            : m_queue(size) {}

private:
    std::vector<raw_storage<T>> m_queue;
};

};  // namespace jungle::container
