// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/asset/asset.h"

namespace jungle::core::asset {

Asset::Asset()
        : service::Service{type_id::of<Asset>()} {}

async::future<> Asset::run() {
    std::println("Asset Service");
    co_return;
}

};  // namespace jungle::core::asset
