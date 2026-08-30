// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/manager.h"
#include "jungle/preusing.h"

namespace jungle::core::component {

class Transform : public ecs::Component<Transform> {
public:
    using Storage = ecs::DenseComponentStorage<Transform>;

private:
    vector3f position;
    vector3f rotation;
    vector3f scale;
};

jungle_core_ecs_register_component(Transform);

};  // namespace jungle::core::component
