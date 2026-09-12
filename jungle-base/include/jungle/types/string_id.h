// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

#include "jungle/types/int.h"
#include "jungle/util/murmur.h"

namespace jungle {

class string_id final {
    friend struct std::hash<string_id>;

public:
    constexpr string_id() = default;

    constexpr string_id(std::string_view str)
            : id{util::hash_str(str)} {}

    constexpr string_id(const string_id &) = default;
    constexpr string_id &operator=(const string_id &) = default;

    constexpr string_id(string_id &&) = default;
    constexpr string_id &operator=(string_id &&) = default;

    constexpr bool operator==(const string_id &rhs) const { return id == rhs.id; }

private:
    u128 id{};
};

};  // namespace jungle

template<>
struct std::hash<jungle::string_id> {
    std::size_t operator()(const jungle::string_id &sid) {
        return static_cast<std::size_t>(sid.id) ^ static_cast<std::size_t>(sid.id >> 64);
    }
};
