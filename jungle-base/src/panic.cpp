// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/panic.h"

#include <exception>
#include <print>

namespace jungle {

[[noreturn]] void panic(ustr message) {
    std::println("[panic] {}", message.view());
    std::terminate();
}

};  // namespace jungle
