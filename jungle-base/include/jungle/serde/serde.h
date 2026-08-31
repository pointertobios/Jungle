// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <expected>
#include <type_traits>

namespace jungle::serde {

namespace detail {

class TraitTargetSource {
public:
    using result_type = bool;
    enum class error_type { mismatch };

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

    std::expected<void, error_type> deserialize_bool(bool &) { return {}; }

    template<std::integral I>
    std::expected<void, error_type> deserialize_integral(I &) {
        return {};
    }

    template<std::floating_point F>
    std::expected<void, error_type> deserialize_floating_point(F &) {
        return {};
    }

    template<concepts::is_enum E>
    std::expected<void, error_type> deserialize_enum(E &) {
        return {};
    }

    std::expected<void, error_type> deserialize_optional_nonnull() { return {}; }
    std::expected<void, error_type> deserialize_optional_nullopt() { return {}; }

    std::expected<void, error_type> deserialize_range_head() { return {}; }
    std::expected<void, error_type> deserialize_range_has_element() { return {}; }
    std::expected<void, error_type> deserialize_range_element_end() { return {}; }
    std::expected<void, error_type> deserialize_range_tail() { return {}; }

    std::expected<void, error_type> deserialize_class_head() { return {}; }
    std::expected<void, error_type> deserialize_class_field() { return {}; }
    std::expected<void, error_type> deserialize_class_field_end() { return {}; }
    std::expected<void, error_type> deserialize_class_tail() { return {}; }
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
           typename T::error_type;
           { t.provide_source(std::declval<const typename T::source_type &>()) } -> std::same_as<void>;
       };

template<template<typename> typename Custr>
concept Customizer = std::is_default_constructible_v<Custr<int>>
                     && requires(Custr<int> customizer, int value, detail::TraitTargetSource &target) {
                            { customizer.serialize(value, target) } -> std::same_as<void>;
                            { customizer.deserialize(value, target) }
                            -> std::same_as<std::expected<void, typename detail::TraitTargetSource::error_type>>;
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
