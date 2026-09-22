// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>

namespace jungle::core::log {

using time_point = std::chrono::system_clock::time_point;

namespace fmt {

template<typename... Args>
class format_string_type {
public:
    template<typename T>
        requires(std::convertible_to<const T &, std::string_view>)
    format_string_type(
        const T &fmt, log::time_point tp = std::chrono::system_clock::now(),
        std::source_location sl = std::source_location::current())
            : m_fmt{fmt}
            , m_tp{time_point}
            , m_sl{sl} {}

    auto get() const { return m_fmt.get(); }

    auto time_point() const { return m_tp; }
    auto source_location() const { return m_sl; }

private:
    std::format_string<Args...> m_fmt;
    log::time_point m_tp;
    std::source_location m_sl;
};

template<typename... Args>
using format_string = format_string_type<std::type_identity_t<Args>...>;

};  // namespace fmt

enum class LogLevel {
    Fatal,
    Error,
    Warning,
    Info,
    Debug,
    Trace,
};

struct LogMessege {
    LogLevel level;
    time_point tp;
    ustr msg;
};

template<typename... Args>
void log_event(LogLevel level, fmt::format_string<Args...> fmt, const Args &...args) {}

};  // namespace jungle::core::log
