#pragma once

#include <optional>

#include "jungle/meta.h"
#include "jungle/serde/serde.h"
#include "jungle/util/type_id.h"

namespace jungle::serde {

template<SerializeTargetImpl Target, typename T>
inline void serialize(const T &value, Target &target);

template<SerializeTargetImpl Target, typename T>
inline typename Target::target_type serialize(T &&value) {
    Target target{};
    serialize(value, target);
    return target.deliver_result();
}

template<typename Target>
class SerializeTarget {
    Target &self() { return static_cast<Target &>(*this); }
    const Target &self() const { return static_cast<const Target &>(*this); }

public:
    constexpr SerializeTarget(const SerializeTarget &) = delete;
    constexpr SerializeTarget &operator=(const SerializeTarget &) = delete;

    constexpr SerializeTarget(SerializeTarget &&) = default;
    constexpr SerializeTarget &operator=(SerializeTarget &&) = default;

    Target spawn_subtarget()
        requires std::same_as<decltype(self().spawn_subtarget()), Target>
    {
        return self().spawn_subtarget();
    }

    void serialize_bool(const bool &value)
        requires std::same_as<decltype(self().serialize_bool(std::declval<const bool &>())), void>
    {
        self().serialize_bool(value);
    }

    template<std::integral I>
    void serialize_integral(const I &value)
        requires std::same_as<decltype(self().serialize_integral(std::declval<const I &>())), void>
    {
        self().serialize_integral(value);
    }

    template<std::floating_point F>
    void serialize_floating_point(const F &value)
        requires std::same_as<decltype(self().serialize_floating_point(std::declval<const F &>())), void>
    {
        self().serialize_floating_point(value);
    }

    template<concepts::is_enum T>
    void serialize_enum(const T &value)
        requires std::same_as<decltype(self().serialize_enum(std::declval<const T &>())), void>
    {
        self().serialize_enum(value);
    }

    template<typename T>
    void serialize_optional(const std::optional<T> &value)
        requires(
            std::same_as<decltype(self().serialize_optional_nonnull()), void>
            && std::same_as<decltype(self().serialize_optional_nullopt()), void>)
    {
        if (value) {
            self().serialize_optional_nonnull();
            auto subtarget = spawn_subtarget();
            serialize(*value, subtarget);
        } else {
            self().serialize_optional_nullopt();
        }
    }

    template<std::ranges::range R>
    void serialize_range(const R &range)
        requires(
            std::same_as<decltype(self().serialize_range_head()), void>
            && std::same_as<decltype(self().serialize_range_element_end()), void>
            && std::same_as<decltype(self().serialize_range_tail(std::declval<usize>())), void>)
    {
        self().serialize_range_head();

        usize size = 0;
        for (const auto &elem : range) {
            auto subtarget = spawn_subtarget();
            serialize(elem, subtarget);
            self().serialize_range_element_end();
            size += 1;
        }

        self().serialize_range_tail(size);
    }

    template<class T>
    void serialize_class_object(const T &obj)
        requires(
            std::same_as<decltype(self().serialize_class_head(std::declval<std::string_view>())), void>
            && std::same_as<decltype(self().serialize_class_field(std::declval<std::string_view>())), void>
            && std::same_as<decltype(self().serialize_class_field_end()), void>
            && std::same_as<decltype(self().serialize_class_tail(std::declval<std::string_view>())), void>)
    {
        std::string_view type_identifier;
        if constexpr (std::meta::has_identifier(^^T)) {
            type_identifier = std::meta::identifier_of(^^T);
        } else {
            type_identifier = "<unnamed>";
        }

        self().serialize_class_head(type_identifier);

        constexpr bool customized_class = meta::has_annotation(^^T, customized);

        constexpr auto ctx = std::meta::access_context::unchecked();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            constexpr bool marked_as_field = meta::has_annotation(m, field);
            constexpr bool customized_field = meta::has_template_annotation<m, ^^customize>();
            if constexpr (std::meta::is_nonstatic_data_member(m)) {
                if constexpr ((customized_class && marked_as_field) || !customized_class) {
                    self().serialize_class_field(std::meta::identifier_of(m));
                    auto subtarget = spawn_subtarget();
                    if constexpr (customized_field) {
                        constexpr auto customizer =
                            meta::nth_template_annotation_argument_of<m, ^^customize>(0);
                        // clang-format off
                        typename [:std::meta::substitute(customizer, {std::meta::type_of(m)}):] customizer_instance{};
                        // clang-format on
                        customizer_instance.serialize(obj.[:m:], subtarget);
                    } else {
                        serialize(obj.[:m:], subtarget);
                    }
                    self().serialize_class_field_end();
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
    if constexpr (std::same_as<T, bool>) {
        target.serialize_bool(value);
    } else if constexpr (std::integral<T>) {
        target.serialize_integral(value);
    } else if constexpr (std::floating_point<T>) {
        target.serialize_floating_point(value);
    } else if constexpr (std::is_enum_v<T>) {
        target.serialize_enum(value);
    } else if constexpr (meta::is_specialization_of_template<^^T, ^^std::optional>()) {
        target.serialize_optional(value);
    } else if constexpr (std::ranges::range<T>) {
        target.serialize_range(value);
    } else if constexpr (std::is_class_v<T>) {
        target.serialize_class_object(value);
    } else {
        static_assert(false, "Type is not serializable");
    }
}

};  // namespace jungle::serde
