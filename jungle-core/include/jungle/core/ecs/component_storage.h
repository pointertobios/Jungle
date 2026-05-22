#pragma once

#include <array>
#include <concepts>
#include <deque>

#include "jungle/core/ecs/component.h"
#include "jungle/types/raw_storage.h"

namespace jungle::core::ecs {

template<typename, typename = void>
class ComponentStorage;

template<ComponentImpl C, typename StorageType>
    requires(
        std::derived_from<StorageType, ComponentStorage<C, StorageType>>
        && !std::same_as<StorageType, ComponentStorage<C, StorageType>>)
class ComponentStorage<C, StorageType> {
public:
private:
};

template<ComponentImpl C>
class DenseComponentStorage : public ComponentStorage<C, DenseComponentStorage<C>> {
    static constexpr usize segment_size = 16;
    static constexpr usize segment_size_mask = 0xf;
    struct Segment {
        std::array<raw_storage<C>, segment_size> components;
    };

public:
private:
};

};  // namespace jungle::core::ecs
