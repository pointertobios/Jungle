// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <expected>
#include <functional>
#include <source_location>
#include <string_view>
#include <vector>

#include "jungle/preusing.h"
#include "jungle/types/erased.h"

namespace jungle::test {

using test_result = std::expected<void, ustr>;

using test_func = test_result (*)();

bool add_sync_test(
    std::string_view name, test_func func, std::source_location location = std::source_location::current());

bool add_async_test(
    std::string_view name, void *func, std::source_location location = std::source_location::current());

#define JUNGLE_SYNC_TEST(name)                                                        \
    jungle::test::test_result test_##name();                                          \
    static bool _added_test_##name = jungle::test::add_sync_test(#name, test_##name); \
    jungle::test::test_result test_##name()

#define JUNGLE_SYNC_ASSERT(expr, ...)                                                                \
    do {                                                                                             \
        if (!(expr)) {                                                                               \
            auto location = std::source_location::current();                                         \
            return std::unexpected{jungle::ustr::format(                                             \
                "  at {}:{}\n{} evaluated false:  {}", location.file_name(), location.line(), #expr, \
                jungle::ustr::format(__VA_ARGS__))};                                                 \
        }                                                                                            \
    } while (0)

#define JUNGLE_SYNC_SUCCESS() \
    return std::expected<void, jungle::ustr> {}

struct async_test_case {
    std::string area;
    std::string_view name;
    void *func;
};

struct async_test_context {
    using async_test_run_func = erased (*)(const std::vector<async_test_case> &async_tests);
    async_test_run_func async_test_run;

    using async_test_collect_func = int (*)(erased &&);
    async_test_collect_func async_test_collect;
};

bool set_async_test_context(async_test_context ctx);

};  // namespace jungle::test
