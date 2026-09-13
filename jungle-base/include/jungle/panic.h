// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <print>
#include <source_location>

namespace jungle {

namespace fmt {

template<typename... Args>
class format_string_type {
public:
    template<typename T>
        requires(std::convertible_to<const T &, std::string_view>)
    consteval format_string_type(const T &fmt, std::source_location sl = std::source_location::current())
            : m_fmt{fmt}
            , m_sl{sl} {}
    constexpr auto get() const { return m_fmt.get(); }
    constexpr auto source_location() const { return m_sl; }

private:
    std::format_string<Args...> m_fmt;
    std::source_location m_sl;
};

template<typename... Args>
using format_string = format_string_type<std::type_identity_t<Args>...>;

};  // namespace fmt

[[noreturn]] consteval void consteval_panic();

[[noreturn]] inline constexpr void
panic(std::string_view msg, std::source_location sl = std::source_location::current()) {
    std::string msg_final;
    if (msg.size()) {
        msg_final = std::format(": {}", msg);
    }
    if consteval {
        consteval_panic();
    } else {
        std::println(stderr, "Panicked at {}:{}:{}{}", sl.file_name(), sl.line(), sl.column(), msg_final);
        std::abort();
    }
}

[[noreturn]] inline constexpr void panic(std::source_location sl = std::source_location::current()) {
    panic("", sl);
}

template<typename... Args>
[[noreturn]] inline constexpr void panic(fmt::format_string<Args...> fmt, Args &&...args) {
    panic(std::vformat(fmt.get(), std::make_format_args(args...)), fmt.source_location());
}

};  // namespace jungle
