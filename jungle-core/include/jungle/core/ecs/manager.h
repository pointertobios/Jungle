#pragma once

#include "jungle/core/ecs/component.h"

namespace jungle::ecs {

template<typename...>
class Manager;

template<ComponentImpl C>
class Manager<C> final {
public:
private:
};

};  // namespace jungle::ecs
