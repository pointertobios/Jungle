// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <vector>

#include "jungle/container/hash_map.h"
#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/manager.h"

namespace jungle::core {

template<ComponentImpl... Cs>
class Managers {
    template<ComponentManager M>
    static consteval bool has_manager() {
        return (std::is_same_v<M, ecs::Manager<Cs>> || ...);
    }

public:
    Managers() {
        (m_managers.emplace(type_id::of<ecs::Manager<Cs>>(), std::make_unique<ecs::Manager<Cs>>()), ...);
    }

    auto get_managers() const { return m_managers.view(); }

    template<ComponentManager M>
        requires(has_manager<M>())
    M &get_manager() {
        return *m_managers.get(type_id::of<M>());
    }

    template<ComponentImpl C>
        requires(has_manager<ecs::Manager<C>>())
    ecs::Manager<C> &get_manager() {
        return *m_managers.get(type_id::of<ecs::Manager<C>>());
    }

private:
    hash_map<type_id, std::unique_ptr<ecs::Manager<>>> m_managers;
};

class Level {
public:
    Level() = default;

private:
    Managers<> m_managers;
};

};  // namespace jungle::core
