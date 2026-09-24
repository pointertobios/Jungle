// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/os/process.h"

#include <utility>

namespace jungle::os {

cpu_set cpu_set::with(usize id) && {
    m_bits |= u128{1} << id;
    return std::move(*this);
}

thread_handle::thread_handle(std::thread::native_handle_type tid)
        : m_tid{tid} {}

thread_handle thread_handle::from(std::thread &t) { return {t.native_handle()}; }

thread_handle thread_handle::from(std::jthread &t) { return {t.native_handle()}; }

};  // namespace jungle::os
