#pragma once

#include <concepts>
#include <type_traits>

namespace jungle::serde {

namespace detail {

class TraitTarget {
public:
    using result_type = bool;

    bool deliver_result() { return false; }

    template<std::integral I>
    void serialize_integral(I) {}

    template<std::floating_point F>
    void serialize_floating_point(F) {}

    void serialize_bool(bool) {}

    template<concepts::is_enum E>
    void serialize_enum(E) {}

    void serialize_range_head() {}
    void serialize_range_element_end() {}
    void serialize_range_tail(usize) {}

    void serialize_class_head(std::string_view) {}
    void serialize_class_field(std::string_view) {}
    void serialize_class_field_end() {}
    void serialize_class_tail(std::string_view) {}

    void deserialize_bool(bool &) {}

    template<std::integral I>
    void deserialize_integral(I &) {}

    template<std::floating_point F>
    void deserialize_floating_point(F &) {}

    template<concepts::is_enum E>
    void deserialize_enum(E &) {}

    bool deserialize_optional_nonnull() { return {}; }

    void deserialize_range_head() {}
    bool deserialize_range_has_element() { return {}; }
    void deserialize_range_element_end() {}
    void deserialize_range_tail() {}

    std::string_view deserialize_class_head() { return {}; }
    std::string_view deserialize_class_field() { return {}; }
    void deserialize_class_field_end() {}
    void deserialize_class_tail() {}
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
                     && requires(Custr<int> customizer, int value, detail::TraitTarget &target) {
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
