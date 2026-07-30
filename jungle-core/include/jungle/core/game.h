// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "jungle/core/level.h"

namespace jungle::core {

class Game {
public:
private:
    std::unique_ptr<Level> m_level{std::make_unique<Level>()};
};

};  // namespace jungle::core
