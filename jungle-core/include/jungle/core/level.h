// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <vector>

#include "jungle/container/hash_map.h"
#include "jungle/core/component/motion.h"
#include "jungle/core/component/transform.h"
#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/manager.h"

namespace jungle::core {

class Level {
public:
    Level() = default;

    auto get_managers() const { return m_managers.view(); }

    template<ecs::ComponentManager M>
    bool has_manager() const {
        return m_managers.contains(type_id::of<M>());
    }

    bool has_manager(type_id type) const { return m_managers.contains(type); }

    template<ecs::ComponentManager M>
    M &get_manager() pre(has_manager<M>()) {
        return m_managers.get(type_id::of<M>())->template as<M>();
    }

    ecs::Manager<> &get_manager(type_id type) pre(has_manager(type)) { return **m_managers.get(type); }

    template<ecs::ComponentImpl C>
        requires(has_manager<ecs::Manager<C>>())
    ecs::Manager<C> &get_manager_of_component() {
        return static_cast<ecs::Manager<C>>(**m_managers.get(type_id::of<ecs::Manager<C>>()));
    }

private:
    hash_map<type_id, std::unique_ptr<ecs::Manager<>>> m_managers;
};

};  // namespace jungle::core
