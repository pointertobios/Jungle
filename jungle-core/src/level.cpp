// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/level.h"
#include "jungle/core/ecs/manager.h"

namespace jungle::core {

Level::Level(std::span<string_id> using_components) {
    for (auto sid : using_components) {
        auto ctor = ecs::Manager<>::get_manager_creator(sid);
        auto [tid, uptr] = ctor();
        auto res = m_managers.insert(tid, try_move(uptr));
        JUNGLE_ASSERT(res, "重复的组件类型 '{}'", uptr->name());
    }
}

};  // namespace jungle::core
