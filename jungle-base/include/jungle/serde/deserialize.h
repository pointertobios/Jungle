// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <expected>
#include <optional>
#include <type_traits>
#include <utility>

#include "jungle/meta.h"
#include "jungle/serde/serde.h"

namespace jungle::serde {

template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] inline auto deserialize(Source &source, T &value)
    -> std::expected<void, typename Source::error_type>;

template<typename T, DeserializeSourceImpl Source>
    requires std::is_default_constructible_v<T>
inline auto deserialize(const typename Source::source_type &source_payload)
    -> std::expected<T, typename Source::error_type> {
    Source source{};
    source.provide_source(source_payload);
    T value{};
    if (auto r = deserialize(source, value); !r) {
        return std::unexpected{std::move(r.error())};
    }
    return value;
}

template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] inline auto deserialize(const typename Source::source_type &source_payload, T &value)
    -> std::expected<void, typename Source::error_type> {
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

    auto deserialize_bool(bool &value)
        requires requires {
            {
                self().deserialize_bool(value)
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        return self().deserialize_bool(value);
    }

    template<std::integral I>
    auto deserialize_integral(I &value)
        requires requires {
            {
                self().template deserialize_integral<I>(value)
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        return self().template deserialize_integral<I>(value);
    }

    template<std::floating_point F>
    auto deserialize_floating_point(F &value)
        requires requires {
            {
                self().template deserialize_floating_point<F>(value)
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        return self().template deserialize_floating_point<F>(value);
    }

    template<concepts::is_enum T>
    auto deserialize_enum(T &value)
        requires requires {
            {
                self().template deserialize_enum<T>(value)
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        return self().template deserialize_enum<T>(value);
    }

    template<typename OptionalT>
    auto deserialize_optional(OptionalT &value)
        requires requires {
            {
                self().deserialize_optional_nonnull()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_optional_nullopt()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        using result_type = std::expected<void, typename Source::error_type>;
        if (self().deserialize_optional_nonnull()) {
            auto subsource = spawn_subsource();
            typename OptionalT::value_type inner_value{};
            if (auto r = deserialize(subsource, inner_value); !r) {
                return r;
            }
            value = std::move(inner_value);
            return result_type{};
        }
        auto r = self().deserialize_optional_nullopt();
        if (r) {
            value = std::nullopt;
        }
        return r;
    }

    template<std::ranges::range R>
    auto deserialize_range(R &value)
        requires requires {
            {
                self().deserialize_range_head()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_range_has_element()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_range_element_end()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_range_tail()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        using result_type = std::expected<void, typename Source::error_type>;
        using value_type = std::ranges::range_value_t<R>;

        if (auto r = self().deserialize_range_head(); !r) {
            return r;
        }

        while (self().deserialize_range_has_element()) {
            auto subsource = spawn_subsource();
            value_type elem{};
            if (auto r = deserialize(subsource, elem); !r) {
                return r;
            }
            value.insert(value.end(), std::move(elem));
            if (auto r = self().deserialize_range_element_end(); !r) {
                return r;
            }
        }

        if (auto r = self().deserialize_range_tail(); !r) {
            return r;
        }
        return result_type{};
    }

    template<class T>
    auto deserialize_class_object(T &value)
        requires requires {
            {
                self().deserialize_class_head()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_class_field()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_class_field_end()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
            {
                self().deserialize_class_tail()
            } -> std::same_as<std::expected<void, typename Source::error_type>>;
        }
    {
        using result_type = std::expected<void, typename Source::error_type>;
        if (auto r = self().deserialize_class_head(); !r) {
            return r;
        }

        constexpr bool customized_class = meta::has_annotation(^^T, customized);

        constexpr auto ctx = std::meta::access_context::unchecked();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            constexpr bool marked_as_field = meta::has_annotation(m, field);
            constexpr bool customized_field = meta::has_template_annotation<m, ^^customize>();
            if constexpr (std::meta::is_nonstatic_data_member(m)) {
                if constexpr ((customized_class && marked_as_field) || !customized_class) {
                    if (auto r = self().deserialize_class_field(); !r) {
                        return r;
                    }
                    auto subsource = spawn_subsource();
                    if constexpr (customized_field) {
                        constexpr auto customizer =
                            meta::nth_template_annotation_argument_of<m, ^^customize>(0);
                        constexpr auto customizer_type =
                            std::meta::substitute(customizer, {std::meta::type_of(m)});
                        typename[:customizer_type:] customizer_instance{};
                        if (auto r = customizer_instance.deserialize(value.[:m:], subsource); !r) {
                            return r;
                        }
                    } else {
                        if (auto r = deserialize(subsource, value.[:m:]); !r) {
                            return r;
                        }
                    }
                    if (auto r = self().deserialize_class_field_end(); !r) {
                        return r;
                    }
                }
            }
        }

        if (auto r = self().deserialize_class_tail(); !r) {
            return r;
        }
        return result_type{};
    }

protected:
    constexpr DeserializeSource() = default;
};

template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] inline auto deserialize(Source &source, T &value)
    -> std::expected<void, typename Source::error_type> {
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
