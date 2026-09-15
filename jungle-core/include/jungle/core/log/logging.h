// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/service/service.h"

namespace jungle::core::log {

class Logging : public service::Service {
public:
    Logging();

    ustr name() const override { return "Logging"; }

private:
    async::future<> run() override;
};

jungle_core_service_register(Logging);

};  // namespace jungle::core::log
