// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>
#include <concepts>

namespace jungle::algebra {

template<typename T>
concept scalar_const_type = requires {
    typename T::value_type;
    { T::zero } -> std::convertible_to<typename T::value_type>;
    { T::one } -> std::convertible_to<typename T::value_type>;
};

template<typename T>
struct scalar_const;

template<std::integral I>
struct scalar_const<I> {
    using value_type = I;

    static constexpr I zero = 0;
    static constexpr I one = 1;
};

template<std::floating_point F>
struct scalar_const<F> {
    using value_type = F;

    static constexpr F zero = 0.0;
    static constexpr F one = 1.0;
};

template<typename T>
concept scalar_operator_type = requires { typename T::value_type; } && requires(typename T::value_type a) {
    { T::sqrt(a) } -> std::same_as<typename T::value_type>;
};

template<typename T>
struct scalar_operator;

template<std::integral I>
struct scalar_operator<I> {
    using value_type = I;

    static constexpr I sqrt(I value) { return static_cast<I>(std::sqrt(static_cast<double>(value))); }
};

template<std::floating_point F>
struct scalar_operator<F> {
    using value_type = F;

    static constexpr F sqrt(F value) { return std::sqrt(value); }
};

template<typename T>
concept scalar_type =
    (sizeof(T) <= std::hardware_destructive_interference_size)
    && scalar_const_type<scalar_const<T>> && scalar_operator_type<scalar_operator<T>>
    && std::is_default_constructible_v<T> && std::movable<T> && std::is_copy_constructible_v<T>
    && std::is_copy_assignable_v<T> && std::is_trivially_destructible_v<T> && requires(T a, T b) {
           { a + b } -> std::convertible_to<T>;
           { a - b } -> std::convertible_to<T>;
           { a * b } -> std::convertible_to<T>;
           { a / b } -> std::convertible_to<T>;
       };

};  // namespace jungle::algebra
