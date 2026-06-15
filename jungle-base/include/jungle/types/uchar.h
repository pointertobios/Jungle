// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "jungle/types/concepts.h"
#include "jungle/types/int.h"

namespace jungle {

class ustr;

namespace fmt {

template<typename Arg>
using format_arg_t =
    std::conditional_t<concepts::Debug<std::remove_cvref_t<Arg>>, ustr, std::remove_cvref_t<Arg>>;

template<typename Arg>
static constexpr decltype(auto) normalize_format_arg(Arg &&arg) {
    if constexpr (concepts::Debug<std::remove_cvref_t<Arg>>) {
        return arg.debug();
    } else {
        return std::forward<Arg>(arg);
    }
}

};  // namespace fmt

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

    static constexpr usize utf8_length(std::span<const i8> utf8);

    static constexpr uchar from_utf8(std::span<const i8> utf8);

    constexpr std::array<i8, 4> to_utf8() const;

private:
    constexpr uchar(u32 value)
            : m_value(value) {}

    u32 m_value{0};
};

class ustr {
public:
    constexpr ustr() = default;

    ustr(const char *str);

    ustr(std::string_view str);

    ustr(std::string &&str);

    template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, const char>
    constexpr ustr(R &&range) {
        for (const auto &ch : range) {
            push(ch);
        }
    }

    constexpr bool is_empty() const { return m_storage.empty(); }

    std::string_view view() const { return m_storage; }

    std::vector<uchar> to_uchars() const;

    void push(uchar ch);

    void append(const ustr &other);
    void append(ustr &&other);
    void append(std::span<const uchar> chars);
    void append(std::span<const i8> char_range);
    void append(const char *str);
    void append(std::string_view str);

    template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, const char>
    void append(R &&range) {
        for (const auto &ch : range) {
            push(ch);
        }
    }

    template<typename... Args>
    static ustr format(std::format_string<fmt::format_arg_t<Args>...> fmt, Args &&...args) {
        return ustr{std::vformat(fmt.get(), std::make_format_args(fmt::normalize_format_arg(args)...))};
    }

private:
    static void check_valid(std::span<const i8> utf8);

    friend struct std::formatter<jungle::ustr>;

    std::string m_storage;
};

namespace literals {

inline ustr operator"" _u(const char *str, std::size_t len) { return ustr{std::string_view{str, len}}; }

};  // namespace literals

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

template<>
struct std::formatter<jungle::ustr> : std::formatter<std::string_view> {
    auto format(const jungle::ustr &str, auto &ctx) const {
        return std::formatter<std::string_view>::format(str.m_storage, ctx);
    }
};

template<jungle::concepts::Debug T>
struct std::formatter<T> : std::formatter<jungle::ustr> {
    auto format(const T &value, auto &ctx) const {
        return std::formatter<jungle::ustr>::format(value.debug(), ctx);
    }
};
