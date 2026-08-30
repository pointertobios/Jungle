// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "jungle/async/future.h"
#include "jungle/core/asset/asset_id.h"
#include "jungle/core/level.h"

namespace jungle::core {

class Game {
public:
    Game(AssetID initial_level);

    async::future<> run();
};

};  // namespace jungle::core
