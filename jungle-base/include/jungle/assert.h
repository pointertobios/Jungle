// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/panic.h"

namespace jungle {

#ifdef JUNGLE_DEBUG_ENABLED
#    define JUNGLE_ASSERT(expr, ...)                                     \
        do {                                                             \
            if (!(expr)) {                                               \
                std::string lint;                                        \
                __VA_OPT__(lint = ": " + std::format(__VA_ARGS__);)      \
                ::jungle::panic("'{}' assertion failed{}", #expr, lint); \
            }                                                            \
        } while (false)
#else
#    define JUNGLE_ASSERT(expr, ...) ((void)(expr))
#endif

};  // namespace jungle
