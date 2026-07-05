// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

#include "jungle/async/future.h"
#include "jungle/test/test.h"

namespace jungle::test {

using async_test_func = async::future<test_result> (*)();

#define JUNGLE_ASYNC_TEST(name)                                                     \
    static jungle::async::future<jungle::test::test_result> test_##name();          \
    static bool _added_test_##name =                                                \
        jungle::test::add_async_test(#name, reinterpret_cast<void *>(test_##name)); \
    static jungle::async::future<jungle::test::test_result> test_##name()

#define JUNGLE_ASYNC_ASSERT(expr, ...)                                                               \
    do {                                                                                             \
        if (!(expr)) {                                                                               \
            auto location = std::source_location::current();                                         \
            co_return std::unexpected{jungle::ustr::format(                                          \
                "  at {}:{}\n{} evaluated false:  {}", location.file_name(), location.line(), #expr, \
                jungle::ustr::format(__VA_ARGS__))};                                                 \
        }                                                                                            \
    } while (0)

#define JUNGLE_ASYNC_SUCCESS() \
    co_return std::expected<void, jungle::ustr> {}

};  // namespace jungle::test
