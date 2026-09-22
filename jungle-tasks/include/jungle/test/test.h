// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#ifdef JUNGLE_TESTING

#    include <expected>
#    include <source_location>
#    include <string>
#    include <string_view>
#    include <variant>

#    include "jungle/async/future.h"
#    include "jungle/preusing.h"

namespace jungle::test {

using test_result = std::expected<void, ustr>;
using sync_test_function = test_result (*)();
using async_test_function = async::future<test_result> (*)();
using test_function = std::variant<sync_test_function, async_test_function>;

template<std::same_as<bool>... Args>
consteval bool ignore_state(Args &&...args) {
    return (args || ...);
}

bool add_test(std::string_view filename, std::string name, test_function fn, bool ignore);

#    define JUNGLE_IGNORE_TEST true

#    define JUNGLE_SYNC_TEST(name, ...)                                                              \
        static jungle::test::test_result test_##name();                                               \
        [[maybe_unused]] static bool test_##name##_registered = jungle::test::add_test(               \
            __FILE__, #name, test_##name, jungle::test::ignore_state(__VA_ARGS__));                    \
        static jungle::test::test_result test_##name()

#    define JUNGLE_SYNC_ASSERT(expr, ...)                                                             \
        do {                                                                                          \
            if (!(expr)) {                                                                            \
                auto location = std::source_location::current();                                      \
                return std::unexpected{jungle::ustr::format(                                          \
                    "  at {}:{}\n{} evaluated false:  {}", location.file_name(), location.line(), #expr, \
                    jungle::ustr::format(__VA_ARGS__))};                                              \
            }                                                                                         \
        } while (false)

#    define JUNGLE_SYNC_SUCCESS() return jungle::test::test_result{}

#    define JUNGLE_ASYNC_TEST(name, ...)                                                             \
        static jungle::async::future<jungle::test::test_result> test_##name();                        \
        [[maybe_unused]] static bool test_##name##_registered = jungle::test::add_test(               \
            __FILE__, #name, test_##name, jungle::test::ignore_state(__VA_ARGS__));                   \
        static jungle::async::future<jungle::test::test_result> test_##name()

#    define JUNGLE_ASYNC_ASSERT(expr, ...)                                                            \
        do {                                                                                          \
            if (!(expr)) {                                                                            \
                auto location = std::source_location::current();                                      \
                co_return std::unexpected{jungle::ustr::format(                                       \
                    "  at {}:{}\n{} evaluated false:  {}", location.file_name(), location.line(), #expr, \
                    jungle::ustr::format(__VA_ARGS__))};                                              \
            }                                                                                         \
        } while (false)

#    define JUNGLE_ASYNC_SUCCESS() co_return jungle::test::test_result{}

};  // namespace jungle::test

#endif
