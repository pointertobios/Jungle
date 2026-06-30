// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/test/async_test.h"
#include "jungle/test/test.h"

namespace jungle::test {

erased async_run(const std::vector<async_test_case> &cases) {
    (void)cases;
    return erased{};
}

int async_collect(erased &&runtime) {
    (void)runtime;
    return 0;
}

bool _async_test_context_registered =
    test::set_async_test_context(async_test_context{async_run, async_collect});

};  // namespace jungle::test
