// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/build_id.h"
#include "jungle/util/murmur.h"

namespace jungle {

u64 build_id() {
    static u64 id{[] {
        auto val = util::hash_str(build_id_string().view());
        return val[0] ^ val[1];
    }()};
    return id;
}

const ustr &build_id_string() {
    static ustr id{JUNGLE_BUILD_ID};
    return id;
}

};  // namespace jungle
