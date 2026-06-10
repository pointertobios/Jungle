// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/preusing.h"

namespace jungle::core::component {

class Transform : public ecs::Component<Transform> {
public:
    using Storage = ecs::DenseComponentStorage<Transform>;

private:
    vector3f m_position;
    vector3f m_rotation;
    vector3f m_scale;
};

};  // namespace jungle::core::component

namespace jungle {

using Transform = core::component::Transform;

};  // namespace jungle
