// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/game.h"

namespace jungle::core {

Game::Game()
        : Service{type_id::of<Game>()} {}

async::future<> Game::run() { co_return; }

};  // namespace jungle::core
