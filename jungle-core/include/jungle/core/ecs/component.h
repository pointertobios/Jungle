// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <functional>

#include "jungle/core/ecs/entity.h"
#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/util/type_mutate.h"

namespace jungle::core::ecs {

class ComponentID {
    friend struct std::hash<ComponentID>;

    static constexpr u64 INVALID = 0;

public:
    constexpr ComponentID() = default;
    constexpr ComponentID(const ComponentID &) = default;
    constexpr ComponentID &operator=(const ComponentID &) = default;
    constexpr ComponentID(ComponentID &&) = default;
    constexpr ComponentID &operator=(ComponentID &&) = default;

    constexpr operator bool() const { return m_id != INVALID; }
    constexpr bool operator==(const ComponentID &other) const { return m_id == other.m_id; }

    constexpr ComponentID(u64 id)
            : m_id{id} {}

    ustr debug() const;

    u64 underlying() const pre(m_id != INVALID) { return m_id; }

private:
    u64 m_id{INVALID};
};

template<typename = void>
class Component;

template<typename, typename>
class ComponentStorage;

template<typename C>
concept ComponentImpl = requires {
    std::derived_from<C, Component<C>>;
    !std::is_same_v<C, Component<C>>;
    typename C::Storage;
    std::derived_from<typename C::Storage, ComponentStorage<C, typename C::Storage>>;
};

template<>
class Component<> : public util::type_mutate<Component<>> {
public:
    template<typename C>
    static constexpr bool static_mutatable = ComponentImpl<C>;

    Component() = delete;
    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;
    Component &operator=(Component &&) = delete;

    Component(Component &&) = default;

    constexpr ComponentID id() const { return m_id; }

    constexpr Entity owner_entity() const { return m_entity; }

protected:
    constexpr Component(type_id type, Entity entity, ComponentID id)
            : util::type_mutate<Component<>>{type}
            , m_id{id}
            , m_entity{entity} {}

private:
    const ComponentID m_id;
    const Entity m_entity;
};

template<concepts::non_void C>
class Component<C> : public Component<> {
public:
    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;
    Component &operator=(Component &&) = delete;

    Component(Component &&) = default;

protected:
    constexpr Component(Entity entity, ComponentID id)
            : Component<>{type_id::of<C>(), entity, id} {}
};

};  // namespace jungle::core::ecs

template<>
struct std::hash<jungle::core::ecs::ComponentID> {
    std::size_t operator()(const jungle::core::ecs::ComponentID &id) const pre(id) { return id.m_id; }
};
