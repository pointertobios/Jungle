#pragma once

#include <expected>
#include <functional>
#include <source_location>
#include <string_view>

#include "jungle/preusing.h"

namespace jungle::test {

using test_result = std::expected<void, ustr>;

using test_func = std::function<test_result()>;

bool add_sync_test(
    std::string_view name, test_func func, std::source_location location = std::source_location::current());

#define JUNGLE_SYNC_TEST(name)                                                        \
    jungle::test::test_result test_##name();                                          \
    static bool _added_test_##name = jungle::test::add_sync_test(#name, test_##name); \
    jungle::test::test_result test_##name()

#define JUNGLE_SYNC_ASSERT(expr, ...)                                                         \
    do {                                                                                      \
        if (!(expr)) {                                                                        \
            return std::unexpected(                                                           \
                jungle::ustr::format(                                                         \
                    "Assertion failed: {}\n  {}", #expr, jungle::ustr::format(__VA_ARGS__))); \
        }                                                                                     \
    } while (0)

#define JUNGLE_SYNC_SUCCESS() \
    return std::expected<void, jungle::ustr> {}

};  // namespace jungle::test
