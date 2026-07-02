// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <memory>
#include <print>
#include <vector>

#include "jungle/async/join_handle.h"
#include "jungle/tasks/runtime/runtime.h"
#include "jungle/test/async_test.h"
#include "jungle/test/test.h"

namespace jungle::test {

struct runtime_wrap {
    std::unique_ptr<tasks::runtime::runtime> rt;

    struct test_result_wrap {
        const std::string &area;
        std::string_view name;
        async::join_handle<test::test_result> handle;
    };
    std::vector<test_result_wrap> results;
};

erased async_run(const std::vector<async_test_case> &cases) {
    using tasks::runtime::runtime;
    erased rtwrap{runtime_wrap{tasks::runtime::runtime_config{}.build_ptr(), {}}};
    auto &rtw = rtwrap.get<runtime_wrap>();
    auto &rt = *rtw.rt;
    auto &results = rtw.results;
    for (auto &[area, name, func_] : cases) {
        auto func = reinterpret_cast<async_test_func>(func_);
        results.push_back(runtime_wrap::test_result_wrap{area, name, rt.spawn(func)});
    }
    return rtwrap;
}

int async_collect(erased &&rtwrap) {
    auto &rtw = rtwrap.get<runtime_wrap>();
    auto &results = rtw.results;
    usize fail_count{0};
    for (auto &[area, name, handle] : results) {
        auto result = handle.blocking_await();
        if (!result) {
            std::println("[FAILED] {}::{}\n{}", area, name, result.error());
            fail_count += 1;
        } else {
            std::println("[PASSED] {}::{}", area, name);
        }
    }
    return fail_count;
}

bool _async_test_context_registered =
    test::set_async_test_context(async_test_context{async_run, async_collect});

};  // namespace jungle::test
