// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

#include "jungle/debug.h"
#include "jungle/panic.h"
#include "jungle/preusing.h"

namespace jungle::core::ecs {

class Entity final {
    friend struct std::hash<Entity>;

    static constexpr u64 INVALID = 0;

public:
    constexpr Entity() = default;
    constexpr Entity(const Entity &entity) = default;
    constexpr Entity &operator=(const Entity &other) = default;
    constexpr Entity(Entity &&entity) = default;
    constexpr Entity &operator=(Entity &&other) = default;

    constexpr operator bool() const { return m_id != INVALID; }
    constexpr bool operator==(const Entity &other) const { return m_id == other.m_id; }

    constexpr Entity(u64 id)
            : m_id{id} {}

    ustr debug() const;

private:
    u64 m_id{INVALID};
};

};  // namespace jungle::core::ecs

template<>
struct std::hash<jungle::core::ecs::Entity> {
    std::size_t operator()(const jungle::core::ecs::Entity &entity) const pre(entity) { return entity.m_id; }
};
