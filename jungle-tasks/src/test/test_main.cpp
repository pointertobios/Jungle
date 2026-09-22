// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/test/test.h"

#include <chrono>
#include <expected>
#include <print>
#include <unordered_map>
#include <vector>

#include "jungle/os/terminal.h"
#include "jungle/tasks/join_set.h"
#include "jungle/tasks/runtime/runtime.h"

namespace jungle::test {

struct test_info {
    std::string area;
    std::string name;
    test_function fn;
    bool ignore;
};

struct test_result_wrap {
    ustr area;
    ustr name;
    test_result result;
};

auto &get_tests() {
    static std::vector<test_info> tests{};
    return tests;
}

bool add_test(std::string_view filename, std::string name, test_function fn, bool ignore) {
    auto separator = filename.find_last_of("/\\");
    auto basename = separator == std::string_view::npos ? filename : filename.substr(separator + 1);
    auto extension = basename.find('.');
    auto area = extension == std::string_view::npos ? basename : basename.substr(0, extension);
    get_tests().emplace_back(std::string{area}, std::move(name), std::move(fn), ignore);
    return true;
}

};  // namespace jungle::test

enum class test_state {
    waiting,
    passed,
    failed,
    ignored,
};

struct test_record {
    test_state state;
    jungle::ustr message;
};

int main() {
    using namespace jungle;

    auto rt = tasks::runtime::runtime_config{}.multi_threaded().build();
    tasks::join_set<test::test_result_wrap> set{rt};
    auto total = test::get_tests().size();

    std::unordered_map<std::string_view, std::unordered_map<std::string_view, test_record>> stats_by_area;

    for (auto &[area, name, fn, ignore] : test::get_tests()) {
        stats_by_area[area][name] = test_record{test_state::waiting, ""};
        if (auto sync_fn = std::get_if<test::sync_test_function>(&fn)) {
            set.spawn_blocking([=] -> test::test_result_wrap {
                if (ignore) {
                    return {area, name, std::unexpected{ustr{"###JUNGLE_IGNORED###"}}};
                }
                return {area, name, (*sync_fn)()};
            });
        } else {
            set.spawn([=] -> async::future<test::test_result_wrap> {
                if (ignore) {
                    co_return {area, name, std::unexpected{ustr{"###JUNGLE_IGNORED###"}}};
                }
                co_return {area, name, co_await std::get<test::async_test_function>(fn)()};
            });
        }
    }

    return rt.block_on([&] -> async::future<int> {
        auto terminal = os::terminal::get();
        usize passed{0};
        usize ignored{0};

        for (usize remaining = total; remaining != 0; --remaining) {
            auto wrapped = co_await set;
            auto &[area, name, result] = *wrapped;
            bool is_ignored = !result.has_value() && result.error() == "###JUNGLE_IGNORED###";
            ustr message = ustr::format("{}::{}", area, name);
            if (!result.has_value() && !is_ignored) {
                message = ustr::format("{}: {}", message, result.error());
            }

            if (is_ignored) {
                ++ignored;
            } else if (result.has_value()) {
                ++passed;
            }

            if (!terminal) {
                auto state = result.has_value() ? "PASSED" : (is_ignored ? "IGNORED" : "FAILED");
                std::println("[{}] {}", state, message);
                continue;
            }

            auto state = result.has_value() ? test_state::passed
                                            : (is_ignored ? test_state::ignored : test_state::failed);
            stats_by_area[area.view()][name.view()] = test_record{state, std::move(message)};

            for (auto &[current_area, records] : stats_by_area) {
                std::print("{} ", current_area);
                for (auto &[_, rec] : records) {
                    auto &[record_state, _] = rec;
                    switch (record_state) {
                    case test_state::waiting:
                        std::print("\033[34m■\033[0m");
                        break;
                    case test_state::passed:
                        std::print("\033[1;32m■\033[0m");
                        break;
                    case test_state::failed:
                        std::print("\033[1;31m■\033[0m");
                        break;
                    case test_state::ignored:
                        std::print("\033[1;33m■\033[0m");
                        break;
                    }
                }
                std::println("\033[0m");
            }
            if (remaining > 1) {
                auto cover_lines = stats_by_area.size();
                std::print("\033[{}A", cover_lines);
            }
        }

        if (terminal) {
            for (auto &[area, records] : stats_by_area) {
                for (auto &[name, rec] : records) {
                    auto &[state, message] = rec;
                    if (state == test_state::failed) {
                        std::println("[\033[1;31mFAILED\033[0m] {}", message);
                    } else if (state == test_state::ignored) {
                        std::println("[\033[1;33mIGNORED\033[0m] {}::{}", area, name);
                    }
                }
            }
        }

        std::println("测试结果：{} 通过，{} 失败，{} 忽略", passed, total - passed - ignored, ignored);
        co_await set.join_all();
        co_return static_cast<int>(passed + ignored - total);
    });
}