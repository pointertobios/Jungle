// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <format>
#include <span>
#include <string_view>

#include "jungle/types/int.h"

namespace jungle {

struct uchar {
    static constexpr u32 MAX = 0x10FFFF;
    static constexpr u32 INVALID = 0;

    constexpr uchar() = default;

    constexpr uchar(i8 ascii)
            : m_value(static_cast<u32>(static_cast<u8>(ascii))) {}

    constexpr uchar(int ascii)
            : m_value(static_cast<u32>(ascii)) {}

    constexpr operator bool() const { return m_value != INVALID; }

    constexpr bool operator==(const uchar &other) const {
        return m_value != INVALID && other.m_value != INVALID && m_value == other.m_value;
    }

    static usize utf8_length(std::span<const i8> utf8);

    static uchar from_utf8(std::span<const i8> utf8);

    std::array<i8, 4> to_utf8() const;

private:
    constexpr uchar(u32 value)
            : m_value{value} {}

    u32 m_value{0};
};

};  // namespace jungle

template<>
struct std::formatter<jungle::uchar> : std::formatter<std::string_view> {
    auto format(const jungle::uchar &ch, auto &ctx) const {
        const auto utf8 = ch.to_utf8();

        jungle::usize length = 0;
        while (length < utf8.size() && utf8[length] != 0) {
            ++length;
        }

        std::array<char, 4> buffer{};
        for (jungle::usize i = 0; i < length; ++i) {
            buffer[i] = utf8[i];
        }

        return std::formatter<std::string_view>::format(std::string_view(buffer.data(), length), ctx);
    }
};
