// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <type_traits>

#include "jungle/meta.h"
#include "jungle/serde/serde.h"

namespace jungle::serde {

template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] inline bool deserialize(Source &source, T &value);

template<typename T, DeserializeSourceImpl Source>
    requires std::is_default_constructible_v<T>
inline std::optional<T> deserialize(const typename Source::source_type &source_payload) {
    Source source{};
    source.provide_source(source_payload);
    T value{};
    if (deserialize(source, value)) {
        return value;
    } else {
        return std::nullopt;
    }
}

template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] inline bool deserialize(const typename Source::source_type &source_payload, T &value) {
    Source source{};
    source.provide_source(source_payload);
    return deserialize(source, value);
}

template<typename Source>
class DeserializeSource {
    Source &self() { return static_cast<Source &>(*this); }
    const Source &self() const { return static_cast<const Source &>(*this); }

public:
    DeserializeSource(const DeserializeSource &) = delete;
    DeserializeSource &operator=(const DeserializeSource &) = delete;

    constexpr DeserializeSource(DeserializeSource &&) = default;
    constexpr DeserializeSource &operator=(DeserializeSource &&) = default;

    Source spawn_subsource()
        requires std::same_as<decltype(self().spawn_subsource()), Source>
    {
        return self().spawn_subsource();
    }

    bool deserialize_bool(bool &value)
        requires requires {
            { self().deserialize_bool(value) } -> std::same_as<bool>;
        }
    {
        return self().deserialize_bool(value);
    }

    template<std::integral I>
    bool deserialize_integral(I &value)
        requires requires {
            { self().template deserialize_integral<I>(value) } -> std::same_as<bool>;
        }
    {
        return self().template deserialize_integral<I>(value);
    }

    template<std::floating_point F>
    bool deserialize_floating_point(F &value)
        requires requires {
            { self().template deserialize_floating_point<F>(value) } -> std::same_as<bool>;
        }
    {
        return self().template deserialize_floating_point<F>(value);
    }

    template<concepts::is_enum T>
    bool deserialize_enum(T &value)
        requires requires {
            { self().template deserialize_enum<T>(value) } -> std::same_as<bool>;
        }
    {
        return self().template deserialize_enum<T>(value);
    }

    template<typename OptionalT>
    bool deserialize_optional(OptionalT &value)
        requires requires {
            { self().deserialize_optional_nonnull() } -> std::same_as<bool>;
            { self().deserialize_optional_nullopt() } -> std::same_as<bool>;
        }
    {
        if (self().deserialize_optional_nonnull()) {
            auto subsource = spawn_subsource();
            typename OptionalT::value_type inner_value{};
            deserialize(subsource, inner_value);
            value = std::move(inner_value);
        } else if (self().deserialize_optional_nullopt()) {
            value = std::nullopt;
        } else {
            return false;
        }
        return true;
    }

    template<std::ranges::range R>
    bool deserialize_range(R &value)
        requires requires {
            { self().deserialize_range_head() } -> std::same_as<bool>;
            { self().deserialize_range_has_element() } -> std::same_as<bool>;
            { self().deserialize_range_element_end() } -> std::same_as<bool>;
            { self().deserialize_range_tail() } -> std::same_as<bool>;
        }
    {
        using value_type = std::ranges::range_value_t<R>;

        if (!self().deserialize_range_head()) {
            return false;
        }

        while (self().deserialize_range_has_element()) {
            auto subsource = spawn_subsource();
            value_type elem{};
            deserialize(subsource, elem);
            value.insert(value.end(), std::move(elem));
            if (!self().deserialize_range_element_end()) {
                return false;
            }
        }

        if (!self().deserialize_range_tail()) {
            return false;
        }
        return true;
    }

    template<class T>
    bool deserialize_class_object(T &value)
        requires requires {
            { self().deserialize_class_head() } -> std::same_as<bool>;
            { self().deserialize_class_field() } -> std::same_as<bool>;
            { self().deserialize_class_field_end() } -> std::same_as<bool>;
            { self().deserialize_class_tail() } -> std::same_as<bool>;
        }
    {
        if (!self().deserialize_class_head()) {
            return false;
        }

        constexpr bool customized_class = meta::has_annotation(^^T, customized);

        constexpr auto ctx = std::meta::access_context::unchecked();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            constexpr bool marked_as_field = meta::has_annotation(m, field);
            constexpr bool customized_field = meta::has_template_annotation<m, ^^customize>();
            if constexpr (std::meta::is_nonstatic_data_member(m)) {
                if constexpr ((customized_class && marked_as_field) || !customized_class) {
                    if (!self().deserialize_class_field()) {
                        return false;
                    }
                    auto subsource = spawn_subsource();
                    if constexpr (customized_field) {
                        constexpr auto customizer =
                            meta::nth_template_annotation_argument_of<m, ^^customize>(0);
                        constexpr auto customizer_type =
                            std::meta::substitute(customizer, {std::meta::type_of(m)});
                        typename[:customizer_type:] customizer_instance{};
                        value.[:m:] = customizer_instance.deserialize(value.[:m:], subsource);
                    } else {
                        deserialize(subsource, value.[:m:]);
                    }
                    if (!self().deserialize_class_field_end()) {
                        return false;
                    }
                }
            }
        }

        if (!self().deserialize_class_tail()) {
            return false;
        }
        return true;
    }

protected:
    constexpr DeserializeSource() = default;
};

template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] inline bool deserialize(Source &source, T &value) {
    if constexpr (std::same_as<T, bool>) {
        return source.deserialize_bool(value);
    } else if constexpr (std::integral<T>) {
        return source.template deserialize_integral<T>(value);
    } else if constexpr (std::floating_point<T>) {
        return source.template deserialize_floating_point<T>(value);
    } else if constexpr (std::is_enum_v<T>) {
        return source.template deserialize_enum<T>(value);
    } else if constexpr (meta::is_specialization_of_template<^^T, ^^std::optional>()) {
        static_assert(
            std::is_default_constructible_v<typename T::value_type>
                && std::is_move_constructible_v<typename T::value_type>,
            "Optional value type must be default constructible and move constructible");
        return source.template deserialize_optional<T>(value);
    } else if constexpr (std::ranges::range<T>) {
        return source.template deserialize_range<T>(value);
    } else if constexpr (std::is_class_v<T>) {
        return source.template deserialize_class_object<T>(value);
    } else {
        static_assert(false, "Type is not deserializable");
    }
}

};  // namespace jungle::serde
