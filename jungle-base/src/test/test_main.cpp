// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <filesystem>
#include <optional>
#include <print>
#include <source_location>
#include <string>
#include <vector>

#include "jungle/test/test.h"

namespace jungle::test {

using namespace std::literals;

struct test_case {
    std::string area;
    std::string_view name;
    test_func func;
};

std::vector<test_case> &sync_test_cases() {
    static std::vector<test_case> cases;
    return cases;
}

bool add_sync_test(std::string_view name, test_func func, std::source_location location) {
    auto area = std::filesystem::path{location.file_name()}.stem().string();
    if (area.empty()) {
        area = "unknown";
    }

    sync_test_cases().push_back(test_case{std::move(area), name, std::move(func)});
    return true;
}

std::vector<async_test_case> &async_test_cases() {
    static std::vector<async_test_case> cases;
    return cases;
}

bool add_async_test(std::string_view name, void *func, std::source_location location) {
    auto area = std::filesystem::path{location.file_name()}.stem().string();
    if (area.empty()) {
        area = "unknown";
    }

    async_test_cases().push_back(async_test_case{std::move(area), name, func});
    return true;
}

std::optional<async_test_context> &get_async_test_context() {
    static std::optional<async_test_context> ctx;
    return ctx;
}

bool set_async_test_context(async_test_context ctx) {
    get_async_test_context() = ctx;
    return true;
}

};  // namespace jungle::test

using namespace jungle;

int main() {
    erased artwrap{};
    if (auto &ctx = test::get_async_test_context()) {
        artwrap = ctx->async_test_run(test::async_test_cases());
    }
    usize fail_count{0};
    for (const auto &[area, name, func] : jungle::test::sync_test_cases()) {
        auto result = func();
        if (!result) {
            std::println("[FAILED] {}::{}\n{}", area, name, result.error());
            fail_count += 1;
        } else {
            std::println("[PASSED] {}::{}", area, name);
        }
    }
    if (auto &ctx = test::get_async_test_context()) {
        ctx->async_test_collect(std::move(artwrap));
    }
    return -fail_count;
}
