// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <new>

#include "jungle/types/int.h"

namespace jungle {

inline constexpr usize cacheline_size = std::hardware_destructive_interference_size;
inline constexpr usize page_size = 4096;

using morder = std::memory_order;

};  // namespace jungle
