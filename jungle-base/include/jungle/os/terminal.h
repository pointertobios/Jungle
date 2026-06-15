// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <utility>

#include "jungle/types/erased.h"
#include "jungle/types/int.h"

namespace jungle::os {

class terminal {
public:
    static std::optional<terminal> get();
    ~terminal();

    terminal(terminal &&) = default;
    terminal &operator=(terminal &&) = default;

    usize width() const;
    usize height() const;

private:
    terminal();

    terminal with_extra(erased extra) && {
        m_extra = std::move(extra);
        return std::move(*this);
    }

    erased m_extra;
};

};  // namespace jungle::os
