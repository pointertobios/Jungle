// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>
#include <variant>

#include "jungle/types/concepts.h"

namespace jungle {

template<typename T>
using try_move_t = std::conditional_t<
    std::is_move_constructible_v<T> && !std::is_fundamental_v<T> && !std::is_pointer_v<T>, T &&,
    std::conditional_t<
        std::is_copy_constructible_v<T> && !std::is_fundamental_v<T> && !std::is_pointer_v<T>, const T &, T>>;

template<typename T>
inline constexpr bool is_try_movable_v =
    std::is_fundamental_v<T> || std::is_pointer_v<T> || std::is_copy_constructible_v<T>
    || std::is_move_constructible_v<T>;

template<typename T>
inline try_move_t<std::remove_cvref_t<T>> try_move(T &&t) {
    if constexpr (std::is_move_constructible_v<T> && !std::is_fundamental_v<T> && !std::is_pointer_v<T>) {
        return std::move(t);
    } else if constexpr (
        std::is_copy_constructible_v<T> && !std::is_fundamental_v<T> && !std::is_pointer_v<T>) {
        return t;
    } else {
        return t;
    }
}

template<typename T>
using fuck_void = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

template<typename T, concepts::non_void U>
using use_or_fuck_void = std::conditional_t<std::is_void_v<T>, U, T>;

template<typename T, concepts::non_void U, concepts::non_void V>
using fuck_void_or_else = std::conditional_t<std::is_void_v<T>, V, U>;

};  // namespace jungle
