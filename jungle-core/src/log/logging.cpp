// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/log/logging.h"

namespace jungle::core::log {

Logging::Logging()
        : service::Service{type_id::of<Logging>()} {}

async::future<> Logging::run() {
    std::println("Logging Service");
    co_return;
}

};  // namespace jungle::core::log
