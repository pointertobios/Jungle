// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <new>
#include <type_traits>

#include "jungle/types/int.h"

namespace jungle::algebra {

template<typename T>
concept algebra_type =
    (sizeof(T) < std::hardware_destructive_interference_size)
    && std::is_default_constructible_v<T> && std::movable<T> && std::is_copy_constructible_v<T>
    && std::is_copy_assignable_v<T> && std::is_trivially_destructible<T> && requires(T a, T b) {
           { a + b } -> std::convertible_to<T>;
           { a - b } -> std::convertible_to<T>;
           { a * b } -> std::convertible_to<T>;
           { a / b } -> std::convertible_to<T>;
       };

template<algebra_type T, usize N, usize M, bool RowMajor = true>
class matrix {
public:
    using value_type = T;

    matrix() = default;

    T &operator[](usize x, usize y) {
        if constexpr (RowMajor) {
            return m_data[x * M + y];
        } else {
            return m_data[y * N + x];
        }
    }

private:
    T m_data[N * M];
};

};  // namespace jungle::algebra
