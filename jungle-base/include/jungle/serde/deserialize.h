#pragma once

#include <optional>
#include <type_traits>

#include "jungle/meta.h"
#include "jungle/serde/serde.h"

namespace jungle::serde {

template<typename T, DeserializeSourceImpl Source>
inline void deserialize(Source &source, T &value);

template<typename T, DeserializeSourceImpl Source>
    requires std::is_default_constructible_v<T>
inline T deserialize(const typename Source::source_type &source_payload) {
    Source source{};
    source.provide_source(source_payload);
    T value{};
    deserialize(source, value);
    return value;
}

template<typename T, DeserializeSourceImpl Source>
inline void deserialize(const typename Source::source_type &source_payload, T &value) {
    Source source{};
    source.provide_source(source_payload);
    deserialize(source, value);
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

    void deserialize_bool(bool &value)
        requires std::same_as<decltype(self().deserialize_bool(std::declval<bool &>())), void>
    {
        self().deserialize_bool(value);
    }

    template<std::integral I>
    void deserialize_integral(I &value)
        requires std::same_as<decltype(self().template deserialize_integral<I>(std::declval<I &>())), void>
    {
        self().template deserialize_integral<I>(value);
    }

    template<std::floating_point F>
    void deserialize_floating_point(F &value)
        requires std::same_as<
            decltype(self().template deserialize_floating_point<F>(std::declval<F &>())), void>
    {
        self().template deserialize_floating_point<F>(value);
    }

    template<concepts::is_enum T>
    void deserialize_enum(T &value)
        requires std::same_as<decltype(self().template deserialize_enum<T>(std::declval<T &>())), void>
    {
        self().template deserialize_enum<T>(value);
    }

    template<typename OptionalT>
    void deserialize_optional(OptionalT &value)
        requires(std::same_as<decltype(self().deserialize_optional_nonnull()), bool>)
    {
        if (self().deserialize_optional_nonnull()) {
            auto subsource = spawn_subsource();
            typename OptionalT::value_type inner_value{};
            deserialize(subsource, inner_value);
            value = std::move(inner_value);
        } else {
            value = std::nullopt;
        }
    }

    template<std::ranges::range R>
    void deserialize_range(R &value)
        requires(
            std::same_as<decltype(self().deserialize_range_head()), void>
            && std::same_as<decltype(self().deserialize_range_has_element()), bool>
            && std::same_as<decltype(self().deserialize_range_element_end()), void>
            && std::same_as<decltype(self().deserialize_range_tail()), void>)
    {
        using value_type = std::ranges::range_value_t<R>;

        self().deserialize_range_head();

        while (self().deserialize_range_has_element()) {
            auto subsource = spawn_subsource();
            value_type elem{};
            deserialize(subsource, elem);
            value.insert(value.end(), std::move(elem));
            self().deserialize_range_element_end();
        }

        self().deserialize_range_tail();
    }

    template<class T>
    void deserialize_class_object(T &value)
        requires(
            std::same_as<decltype(self().deserialize_class_head()), std::string_view>
            && std::same_as<decltype(self().deserialize_class_field()), std::string_view>
            && std::same_as<decltype(self().deserialize_class_field_end()), void>
            && std::same_as<decltype(self().deserialize_class_tail()), void>)
    {
        std::string_view type_identifier = self().deserialize_class_head();

        constexpr bool customized_class = meta::has_annotation(^^T, customized);

        constexpr auto ctx = std::meta::access_context::unchecked();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            constexpr bool marked_as_field = meta::has_annotation(m, field);
            constexpr bool customized_field = meta::has_template_annotation<m, ^^customize>();
            if constexpr (std::meta::is_nonstatic_data_member(m)) {
                if constexpr ((customized_class && marked_as_field) || !customized_class) {
                    std::string_view field_name = self().deserialize_class_field();
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
                    self().deserialize_class_field_end();
                }
            }
        }

        self().deserialize_class_tail();
    }

protected:
    constexpr DeserializeSource() = default;
};

template<typename T, DeserializeSourceImpl Source>
inline void deserialize(Source &source, T &value) {
    if constexpr (std::same_as<T, bool>) {
        source.deserialize_bool(value);
    } else if constexpr (std::integral<T>) {
        source.template deserialize_integral<T>(value);
    } else if constexpr (std::floating_point<T>) {
        source.template deserialize_floating_point<T>(value);
    } else if constexpr (std::is_enum_v<T>) {
        source.template deserialize_enum<T>(value);
    } else if constexpr (meta::is_specialization_of_template<^^T, ^^std::optional>()) {
        static_assert(
            std::is_default_constructible_v<typename T::value_type>
                && std::is_move_constructible_v<typename T::value_type>,
            "Optional value type must be default constructible and move constructible");
        source.template deserialize_optional<T>(value);
    } else if constexpr (std::ranges::range<T>) {
        source.template deserialize_range<T>(value);
    } else if constexpr (std::is_class_v<T>) {
        source.template deserialize_class_object<T>(value);
    } else {
        static_assert(false, "Type is not deserializable");
    }
}

};  // namespace jungle::serde
