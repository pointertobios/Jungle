// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>

namespace jungle::allocator {

struct block_list {
    block_list *next;
    std::size_t length;
};

};  // namespace jungle::allocator
