#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

#include "jungle/meta.h"
#include "jungle/types/uchar.h"

namespace jungle {

template<typename T>
ustr debug_of(T &&value, usize level = 0) {
    using namespace literals;
    namespace meta = std::meta;
    using raw_type = std::remove_cvref_t<T>;
    constexpr auto value_type = meta::dealias(^^raw_type);

    if constexpr (jungle::concepts::Debug<raw_type>) {
        return value.debug();
    } else {
        ustr res;
        if constexpr (std::is_fundamental_v<raw_type>) {
            res.append(ustr::format("{}({})", meta::identifier_of(value_type), value));
        } else if constexpr (std::is_enum_v<raw_type>) {
            template for (constexpr auto e : std::define_static_array(meta::enumerators_of(value_type))) {
                if (value == [:e:]) {
                    res.append(
                        ustr::format("{}::{}", meta::identifier_of(value_type), meta::identifier_of(e)));
                }
            }
        } else if constexpr (
            std::formattable<raw_type, char> || std::convertible_to<raw_type, const char *>) {
            res.append(ustr::format("{}", value));
        } else if constexpr (std::ranges::range<raw_type>) {
            res.append("[ ");
            for (auto it = value.begin(); it != value.end(); ++it) {
                if (it != value.begin()) {
                    res.append(", ");
                }
                res.append(debug_of(*it));
            }
            res.append(" ]");
        } else if constexpr (std::is_class_v<raw_type>) {
            constexpr auto ctx = meta::access_context::unchecked();

            res.append(ustr::format("{} {{\n", meta::identifier_of(value_type)));
            template for (constexpr auto m : std::define_static_array(meta::members_of(value_type, ctx))) {
                if constexpr (meta::is_nonstatic_data_member(m)) {
                    res.append(ustr::format("  {}: {},\n", meta::identifier_of(m), debug_of(value.*[:m:])));
                }
            }
            res.append("}");
        } else {
            static_assert(false, "Unsupported type");
        }
        return res;
    }
}

};  // namespace jungle
