// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/tasks/runtime/runtime.h"

namespace jungle::tasks::runtime {

runtime runtime_config::start() && { return runtime{*this}; }

runtime_config runtime_config::concurrency(usize n) && {
    m_concurrency = n;
    return *this;
}

runtime::runtime()
        : runtime{runtime_config{}} {}

runtime::runtime(runtime_config config) { (void)config; }

};  // namespace jungle::tasks::runtime
