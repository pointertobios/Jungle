// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/preusing.h"

namespace jungle::core::component {

class Motion : public ecs::Component<Motion> {
public:
    using Storage = ecs::DenseComponentStorage<Motion>;

private:
    vector3f m_velocity;
    vector3f m_acceleration;
};

};  // namespace jungle::core::component
