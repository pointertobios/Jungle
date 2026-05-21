#pragma once

#include <concepts>
#include <functional>

#include "jungle/panic.h"
#include "jungle/preusing.h"

namespace jungle::core::ecs {

struct ComponentID {
    friend struct std::hash<ComponentID>;

    static constexpr u64 INVALID = 0;

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

private:
    u64 m_id{INVALID};
};

enum class ComponentStorage final {
    Dense,
    Sparse,
};

template<typename...>
class Component;

template<typename C>
concept ComponentImpl = std::derived_from<C, Component<C>> && !std::is_same_v<C, Component<C>>
                        && std::same_as<decltype(C::StorageType), ComponentStorage>;

template<>
class Component<> {
public:
    Component() = delete;
    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;
    Component(Component &&) = delete;
    Component &operator=(Component &&) = delete;

    template<ComponentImpl C>
    constexpr bool is() const noexcept {
        return m_type == type_id::of<C>();
    }

    constexpr bool is(type_id type) const noexcept { return m_type == type; }

    template<ComponentImpl C>
    constexpr const C &as() const noexcept {
        if (!is<C>()) [[unlikely]] {
            panic("Component type mismatch: expected {}, got {}", type_id::of<C>().name(), m_type.name());
        }
        return static_cast<const C &>(*this);
    }

    template<ComponentImpl C>
    constexpr const C *try_as() const noexcept {
        return is<C>() ? &static_cast<const C &>(*this) : nullptr;
    }

    constexpr type_id type() const noexcept { return m_type; }

protected:
    constexpr Component(type_id type) noexcept
            : m_type{type} {}

private:
    const type_id m_type;
    const ComponentID m_id;
};

template<ComponentImpl C>
class Component<C> : public Component<> {
public:
    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;
    Component &operator=(Component &&) = delete;

    Component(Component &&) noexcept = default;

protected:
    constexpr Component()
            : Component<>{type_id::of<C>()} {}

private:
};

};  // namespace jungle::core::ecs

template<>
struct std::hash<jungle::core::ecs::ComponentID> {
    std::size_t operator()(const jungle::core::ecs::ComponentID &id) const noexcept {
        if (!id) [[unlikely]] {
            jungle::panic("Invalid component ID");
        }
        return id.m_id;
    }
};
