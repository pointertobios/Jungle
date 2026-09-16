// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/service/service.h"

namespace jungle::core::asset {

class Asset : public service::Service {
public:
    Asset();

    ustr name() const override { return "Asset"; }

private:
    async::future<> run() override;
};

jungle_core_service_register(Asset);

};  // namespace jungle::core::asset
