// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <deque>

#include "jungle/core/level.h"

namespace jungle::core {

class Game {
public:
private:
    std::deque<Level> m_levels;
};

};  // namespace jungle::core
