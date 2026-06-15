// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <thread>

#include "jungle/preusing.h"

namespace jungle::tasks::runtime {

class runtime;

class runtime_config {
public:
    runtime start() &&;

    runtime_config concurrency(usize n) &&;

private:
    usize m_concurrency{std::thread::hardware_concurrency()};
};

class runtime final {
    friend class runtime_config;

public:
    explicit runtime();

private:
    explicit runtime(runtime_config config);
};

};  // namespace jungle::tasks::runtime
