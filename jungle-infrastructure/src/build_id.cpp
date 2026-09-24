// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/build_id.h"

#include "jungle/util/murmur.h"

namespace jungle {

u128 build_id() {
    static u128 id{[] {
        auto val = util::hash_str(build_id_string());
        return val;
    }()};
    return id;
}

std::string_view build_id_string() {
    static std::string_view id{JUNGLE_BUILD_ID};
    return id;
}

};  // namespace jungle
