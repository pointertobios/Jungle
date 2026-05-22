#pragma once

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/entity.h"

namespace jungle::core::ecs {

template<typename...>
class Manager;

template<ComponentImpl C>
class Manager<C> final {
public:
private:
};

};  // namespace jungle::core::ecs
