// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <print>

#include "jungle/async/future.h"
#include "jungle/test/test.h"

using namespace jungle;

async::future<> async_func() { co_return; }

async::future<int> async_func_int() {
    co_await async_func();
    co_return 1;
}

JUNGLE_SYNC_TEST(future_type_correctness) { JUNGLE_SYNC_SUCCESS(); }
