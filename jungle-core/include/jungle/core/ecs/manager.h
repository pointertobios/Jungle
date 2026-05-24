#pragma once

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/entity.h"

namespace jungle::core::ecs {

template<ComponentImpl C>
class Manager final {
public:
private:
    C::Storage m_storage;
};

};  // namespace jungle::core::ecs
