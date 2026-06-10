// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/algebra/matrix.h"
#include "jungle/types/int.h"    // IWYU pragma: keep
#include "jungle/types/uchar.h"  // IWYU pragma: keep
#include "jungle/util/murmur.h"
#include "jungle/util/type_id.h"


namespace jungle {

using hash_val = util::hash_val;
using type_id = util::type_id;

using vector2f = algebra::vector2f;
using vector3f = algebra::vector3f;
using matrix3f = algebra::matrix3f;
using matrix4f = algebra::matrix4f;

};  // namespace jungle
