// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "jungle/async/future.h"
#include "jungle/core/asset/asset_id.h"
#include "jungle/core/level.h"
#include "jungle/core/service/service.h"

namespace jungle::core {

class Game : public service::Service {
public:
    Game();

    ustr name() const override { return "Game"; }

private:
    async::future<> run() override;
};

jungle_core_service_register(Game);

};  // namespace jungle::core
