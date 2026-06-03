// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <type_traits>

namespace jungle::serde {

namespace detail {

class TraitTargetSource {
public:
    using result_type = bool;

    bool deliver_result() { return false; }

    template<std::integral I>
    void serialize_integral(const I &) {}

    template<std::floating_point F>
    void serialize_floating_point(const F &) {}

    void serialize_bool(const bool &) {}

    template<concepts::is_enum E>
    void serialize_enum(const E &) {}

    void serialize_optional_nonnull() {}
    void serialize_optional_nullopt() {}

    void serialize_range_head() {}
    void serialize_range_element_end() {}
    void serialize_range_tail(usize) {}

    void serialize_class_head(std::string_view) {}
    void serialize_class_field(std::string_view) {}
    void serialize_class_field_end() {}
    void serialize_class_tail(std::string_view) {}

    bool deserialize_bool(bool &) { return {}; }

    template<std::integral I>
    bool deserialize_integral(I &) { return {}; }

    template<std::floating_point F>
    bool deserialize_floating_point(F &) { return {}; }

    template<concepts::is_enum E>
    bool deserialize_enum(E &) { return {}; }

    bool deserialize_optional_nonnull() { return {}; }
    bool deserialize_optional_nullopt() { return {}; }

    bool deserialize_range_head() { return {}; }
    bool deserialize_range_has_element() { return {}; }
    bool deserialize_range_element_end() { return {}; }
    bool deserialize_range_tail() { return {}; }

    bool deserialize_class_head() { return {}; }
    bool deserialize_class_field() { return {}; }
    bool deserialize_class_field_end() { return {}; }
    bool deserialize_class_tail() { return {}; }
};

};  // namespace detail

template<typename>
class SerializeTarget;

template<typename T>
concept SerializeTargetImpl =
    std::derived_from<T, SerializeTarget<T>> && !std::is_same_v<T, SerializeTarget<T>>
    && std::is_default_constructible_v<T> && requires(T t) {
           typename T::target_type;
           { t.deliver_result() } -> std::same_as<typename T::target_type>;
       };

template<typename>
class DeserializeSource;

template<typename T>
concept DeserializeSourceImpl =
    std::derived_from<T, DeserializeSource<T>> && !std::is_same_v<T, DeserializeSource<T>>
    && std::is_default_constructible_v<T> && requires(T t) {
           typename T::source_type;
           { t.provide_source(std::declval<const typename T::source_type &>()) } -> std::same_as<void>;
       };

template<template<typename> typename Custr>
concept Customizer = std::is_default_constructible_v<Custr<int>>
                     && requires(Custr<int> customizer, int value, detail::TraitTargetSource &target) {
                            { customizer.serialize(value, target) } -> std::same_as<void>;
                            { customizer.deserialize(value, target) } -> std::same_as<int>;
                        };

template<template<typename> typename Custr>
    requires(Customizer<Custr>)
struct Customize {};

template<template<typename> typename Custr>
    requires(Customizer<Custr>)
inline constexpr Customize<Custr> customize;

inline constexpr struct {
} customized;

inline constexpr struct {
} field;

};  // namespace jungle::serde
