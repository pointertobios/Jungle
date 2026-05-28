#pragma once

#include <concepts>
#include <type_traits>

#include "jungle/meta.h"

namespace jungle::serde {

template<typename = void>
class SerializeTarget;

template<typename T>
concept SerializeTargetImpl =
    std::derived_from<T, SerializeTarget<T>> && !std::is_same_v<T, SerializeTarget<T>>
    && std::is_default_constructible_v<T> && requires(T t) {
           typename T::result_type;
           { t.deliver_result() } -> std::same_as<typename T::result_type &&>;
       };

template<SerializeTargetImpl Target, typename T>
inline void serialize(const T &value, Target &target);

template<SerializeTargetImpl Target, typename T>
inline typename Target::result_type serialize(T &&value) {
    Target target{};
    serialize(value, target);
    return target.deliver_result();
}

template<template<typename> typename Custr>
concept Customizer = requires(Custr<int> c, SerializeTarget<void> &target) {
    std::is_default_constructible_v<decltype(c)>;
    c.serialize(std::declval<int>(), target);
};

template<template<typename> typename Custr>
    requires(Customizer<Custr>)
struct Customize {};

template<template<typename> typename Custr>
    requires(Customizer<Custr>)
inline constexpr Customize<Custr> customized;

inline constexpr struct {
} customize;

inline constexpr struct {
} field;

template<SerializeTargetImpl Target>
class SerializeTarget<Target> {
    Target &self() { return static_cast<Target &>(*this); }
    const Target &self() const { return static_cast<const Target &>(*this); }

public:
    constexpr SerializeTarget(const SerializeTarget &) = delete;
    constexpr SerializeTarget &operator=(const SerializeTarget &) = delete;

    constexpr SerializeTarget(SerializeTarget &&) = default;
    constexpr SerializeTarget &operator=(SerializeTarget &&) = default;

    Target spawn_subtarget() const
        requires std::same_as<decltype(self().spawn_subtarget()), Target>
    {
        return self().spawn_subtarget();
    }

    template<std::integral I>
    void serialize_integral(const I &value)
        requires std::same_as<decltype(self().serialize_integral(std::declval<I>())), void>
    {
        self().serialize_integral(value);
    }

    template<std::floating_point F>
    void serialize_floating_point(const F &value)
        requires std::same_as<decltype(self().serialize_floating_point(std::declval<F>())), void>
    {
        self().serialize_floating_point(value);
    }

    void serialize_bool(const bool &value)
        requires std::same_as<decltype(self().serialize_bool(std::declval<bool>())), void>
    {
        self().serialize_bool(value);
    }

    template<concepts::is_enum T>
    void serialize_enum(const T &value)
        requires std::same_as<decltype(self().serialize_enum(std::declval<T>())), void>
    {
        self().serialize_enum(value);
    }

    template<std::ranges::range R>
    void serialize_range(const R &range) {
        constexpr auto element_type_identifier =
            std::meta::identifier_of(std::meta::dealias(^^std::ranges::range_value_t<R>));

        self().serialize_range_head(element_type_identifier);

        usize size = 0;
        for (const auto &elem : range) {
            auto subtarget = spawn_subtarget();
            serialize(elem, subtarget);
            size += 1;
        }

        self().serialize_range_tail(element_type_identifier, size);
    }

    template<class T>
    void serialize_class_object(const T &obj)
        requires(
            std::same_as<decltype(self().serialize_class_head(std::declval<std::meta::info>())), void>
            && std::same_as<decltype(self().serialize_class_field(std::declval<std::string_view>())), void>
            && std::same_as<decltype(self().serialize_class_tail(std::declval<std::meta::info>())), void>)
    {
        constexpr auto type_identifier = std::meta::identifier_of(^^T);

        self().serialize_class_head(type_identifier);

        constexpr bool customized_class = meta::has_annotation(^^T, customize);

        constexpr auto ctx = std::meta::access_context::unchecked();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            constexpr bool marked_as_field = meta::has_annotation(m, field);
            constexpr bool customized_field = meta::has_template_annotation<m>(^^customized);
            if constexpr (std::meta::is_nonstatic_data_member(m)) {
                self().serialize_class_field(std::meta::identifier_of(m));
                auto subtarget = spawn_subtarget();
                if constexpr (
                    (!customized_class && !customized_field)
                    || (customized_class && marked_as_field && !customized_field)) {
                    serialize(obj.[:m:], subtarget);
                } else if constexpr (
                    (!customized_class && customized_field)
                    || (customized_class && marked_as_field && customized_field)) {
                    constexpr auto customized_anno = meta::nth_template_annotation_of<m>(0, ^^customized);
                    constexpr auto customizer = std::define_static_array(
                        std::meta::template_arguments_of(std::meta::type_of(customized_anno)))[0];
                    // clang-format off
                    typename [:customizer:]<T> customizer_instance{};
                    // clang-format on
                    customizer_instance.serialize(obj.[:m:], subtarget);
                }
            }
        }

        self().serialize_class_tail(type_identifier);
    }

protected:
    constexpr SerializeTarget() = default;
};

template<SerializeTargetImpl Target, typename T>
inline void serialize(const T &value, Target &target) {
    if constexpr (std::integral<T>) {
        target.serialize_integral(value);
    } else if constexpr (std::floating_point<T>) {
        target.serialize_floating_point(value);
    } else if constexpr (std::same_as<T, bool>) {
        target.serialize_bool(value);
    } else if constexpr (std::is_enum_v<T>) {
        target.serialize_enum(value);
    } else if constexpr (std::ranges::range<T>) {
        target.serialize_range(value);
    } else if constexpr (std::is_class_v<T>) {
        target.serialize_class_object(value);
    } else {
        static_assert(false, "Type is not serializable");
    }
}

};  // namespace jungle::serde
