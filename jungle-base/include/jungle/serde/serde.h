#pragma once

#include <concepts>
#include <type_traits>

#include "jungle/meta.h"
#include "jungle/util/type_id.h"

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
};

};  // namespace detail

template<typename>
class SerializeTarget;

template<typename T>
concept SerializeTargetImpl =
    std::derived_from<T, SerializeTarget<T>> && !std::is_same_v<T, SerializeTarget<T>>
    && std::is_default_constructible_v<T> && requires(T t) {
           typename T::result_type;
           { t.deliver_result() } -> std::same_as<typename T::result_type>;
       };

template<template<typename> typename Custr>
consteval bool is_customizer() {
    if constexpr (!std::is_default_constructible_v<Custr<int>>) {
        return false;
    }
    return requires(Custr<int> customizer, int value, detail::TraitTarget &target) {
        customizer.serialize(value, target);
    };
}

template<template<typename> typename Custr>
concept Customizer = is_customizer<Custr>();

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
