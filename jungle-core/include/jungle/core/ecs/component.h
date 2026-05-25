#pragma once

#include <concepts>
#include <functional>

#include "jungle/core/ecs/entity.h"
#include "jungle/panic.h"
#include "jungle/preusing.h"

namespace jungle::core::ecs {

class ComponentID {
    friend struct std::hash<ComponentID>;

    static constexpr u64 INVALID = 0;

public:
    constexpr ComponentID() noexcept = default;
    constexpr ComponentID(const ComponentID &) noexcept = default;
    constexpr ComponentID &operator=(const ComponentID &) noexcept = default;
    constexpr ComponentID(ComponentID &&) noexcept = default;
    constexpr ComponentID &operator=(ComponentID &&) noexcept = default;

    constexpr operator bool() const noexcept { return m_id != INVALID; }
    constexpr bool operator==(const ComponentID &other) const noexcept { return m_id == other.m_id; }

    constexpr ComponentID(u64 id) noexcept
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
class Component<> {
public:
    Component() = delete;
    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;
    Component &operator=(Component &&) = delete;

    Component(Component &&) noexcept = default;

    template<ComponentImpl C>
    constexpr bool is() const noexcept {
        return m_type == type_id::of<C>();
    }

    constexpr bool is(type_id type) const noexcept { return m_type == type; }

    template<ComponentImpl C>
    constexpr C &as() noexcept pre(is<C>()) {
        return static_cast<C &>(*this);
    }

    template<ComponentImpl C>
    constexpr const C &as() const noexcept pre(is<C>()) {
        return static_cast<const C &>(*this);
    }

    template<ComponentImpl C>
    constexpr C *try_as() noexcept {
        return is<C>() ? &static_cast<C &>(*this) : nullptr;
    }

    template<ComponentImpl C>
    constexpr const C *try_as() const noexcept {
        return is<C>() ? &static_cast<const C &>(*this) : nullptr;
    }

    constexpr type_id type() const noexcept { return m_type; }

    constexpr Entity owner_entity() const noexcept { return m_entity; }

protected:
    constexpr Component(type_id type, Entity entity, ComponentID id) noexcept
            : m_type{type}
            , m_id{id}
            , m_entity{entity} {}

private:
    const type_id m_type;
    const ComponentID m_id;
    const Entity m_entity;
};

template<ComponentImpl C>
class Component<C> : public Component<> {
public:
    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;
    Component &operator=(Component &&) = delete;

    Component(Component &&) noexcept = default;

protected:
    constexpr Component(Entity entity, ComponentID id) noexcept
            : Component<>{type_id::of<C>(), entity, id} {}

private:
};

};  // namespace jungle::core::ecs

template<>
struct std::hash<jungle::core::ecs::ComponentID> {
    std::size_t operator()(const jungle::core::ecs::ComponentID &id) const noexcept pre(id) {
        return id.m_id;
    }
};
