// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <new>

#include "jungle/algebra/matrix.h"
#include "jungle/types/int.h"    // IWYU pragma: keep
#include "jungle/types/uchar.h"  // IWYU pragma: keep
#include "jungle/util/murmur.h"
#include "jungle/util/type_id.h"

namespace jungle {

inline constexpr usize cacheline_size = std::hardware_destructive_interference_size;

using hash_val = util::hash_val;
using type_id = util::type_id;

using vector2f = algebra::vector2f;
using vector3f = algebra::vector3f;
using matrix3f = algebra::matrix3f;
using matrix4f = algebra::matrix4f;

};  // namespace jungle
