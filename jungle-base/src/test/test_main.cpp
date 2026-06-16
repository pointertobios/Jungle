// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/test/test.h"

#include <filesystem>
#include <print>
#include <source_location>
#include <string>
#include <vector>

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

};  // namespace jungle::test

using namespace jungle;

int main() {
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
    return -fail_count;
}
