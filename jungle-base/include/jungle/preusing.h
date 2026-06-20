// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/algebra/matrix.h"
#include "jungle/constants.h"       // IWYU pragma: keep
#include "jungle/types/concepts.h"  // IWYU pragma: keep
#include "jungle/types/int.h"       // IWYU pragma: keep
#include "jungle/types/types.h"     // IWYU pragma: keep
#include "jungle/types/uchar.h"     // IWYU pragma: keep
#include "jungle/util/type_id.h"    // IWYU pragma: keep

namespace jungle {

using type_id = util::type_id;

using vector2f = algebra::vector2f;
using vector3f = algebra::vector3f;
using matrix3f = algebra::matrix3f;
using matrix4f = algebra::matrix4f;

};  // namespace jungle
