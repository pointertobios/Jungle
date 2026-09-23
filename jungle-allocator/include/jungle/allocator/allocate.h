// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <new>

namespace jungle::allocator {

void *allocate(std::size_t n, std::align_val_t align);

};  // namespace jungle::allocator
