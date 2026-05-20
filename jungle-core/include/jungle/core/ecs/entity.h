#pragma once

#include <functional>

#include "jungle/debug.h"
#include "jungle/panic.h"


namespace jungle::core::ecs {

struct [[=Debug]] Entity final {
  friend struct std::hash<Entity>;

  static constexpr u64 INVALID = 0;

  constexpr Entity() noexcept = default;
  constexpr Entity(const Entity &entity) noexcept = default;
  constexpr Entity &operator=(const Entity &other) noexcept = default;
  constexpr Entity(Entity &&entity) noexcept = default;
  constexpr Entity &operator=(Entity &&other) noexcept = default;

  constexpr operator bool() const noexcept { return m_id != INVALID; }
  constexpr bool operator==(const Entity &other) const noexcept {
    return m_id == other.m_id;
  }

  constexpr Entity(u64 id) noexcept : m_id{id} {}

  ustr debug() const;

private:
  u64 m_id{INVALID};
};

}; // namespace jungle::core::ecs

template <> struct std::hash<jungle::core::ecs::Entity> {
  std::size_t
  operator()(const jungle::core::ecs::Entity &entity) const noexcept {
    if (!entity) [[unlikely]] {
      jungle::panic("Invalid entity");
    }
    return entity.m_id;
  }
};
