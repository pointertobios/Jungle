// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/manager.h"
#include "jungle/preusing.h"

namespace jungle::core::component {

class Motion : public ecs::Component<Motion> {
public:
    using Storage = ecs::DenseComponentStorage<Motion>;

private:
    vector3f velocity;
    vector3f acceleration;
};

jungle_core_ecs_register_component(Motion);

};  // namespace jungle::core::component
