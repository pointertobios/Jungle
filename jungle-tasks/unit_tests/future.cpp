// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <print>

#include "jungle/future.h"
#include "jungle/test/test.h"

using namespace jungle::tasks;

future<> async_func() { co_return; }

future<int> async_func_int() {
    co_await async_func();
    co_return 1;
}

JUNGLE_SYNC_TEST(future_type_correctness) {
    async_func_int().as_task().resume();
    JUNGLE_SYNC_SUCCESS();
}
