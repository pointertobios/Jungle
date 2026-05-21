#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

#include "jungle/meta.h"
#include "jungle/types/uchar.h"

namespace jungle {

template<typename T>
ustr debug(T &&value, bool indent_first_line = false, usize level = 0) {
    using namespace literals;
    namespace meta = std::meta;
    using raw_type = std::remove_cvref_t<T>;
    constexpr auto value_type = meta::dealias(^^raw_type);

    auto indent = ustr{std::ranges::repeat_view(std::string_view{"  "}, level) | std::views::join};
    auto first_line_indent = indent_first_line ? indent : ustr{};

    if constexpr (concepts::Debug<raw_type>) {
        return ustr::format("{}{}", first_line_indent, value.debug());
    } else {
        ustr res;
        if constexpr (std::is_fundamental_v<raw_type>) {
            res.append(
                ustr::format("{}{}({})", first_line_indent, meta::display_string_of(value_type), value));
        } else if constexpr (std::is_enum_v<raw_type>) {
            template for (constexpr auto e : std::define_static_array(meta::enumerators_of(value_type))) {
                if (value == [:e:]) {
                    res.append(
                        ustr::format(
                            "{}{}::{}", first_line_indent, meta::identifier_of(value_type),
                            meta::identifier_of(e)));
                }
            }
        } else if constexpr (
            std::formattable<raw_type, char> || std::convertible_to<raw_type, const char *>) {
            res.append(ustr::format("{}{}", first_line_indent, value));
        } else if constexpr (std::ranges::range<raw_type>) {
            res.append(ustr::format("{}[ ", first_line_indent));
            for (auto it = value.begin(); it != value.end(); ++it) {
                if (it != value.begin()) {
                    res.append(", ");
                }
                res.append(debug_of(*it, 0));
            }
            res.append(" ]");
        } else if constexpr (std::is_class_v<raw_type>) {
            constexpr auto ctx = meta::access_context::unchecked();

            res.append(ustr::format("{}{} {{\n", first_line_indent, meta::identifier_of(value_type)));
            template for (constexpr auto m : std::define_static_array(meta::members_of(value_type, ctx))) {
                if constexpr (meta::is_nonstatic_data_member(m)) {
                    res.append(
                        ustr::format(
                            "{}  {}{}: {},\n", indent,
                            meta::is_public(m)      ? "public "
                            : meta::is_protected(m) ? "protected "
                                                    : "private ",
                            meta::identifier_of(m), debug(value.[:m:], false, level + 1)));
                }
            }
            res.append(ustr::format("{}}}", indent));
        } else {
            static_assert(false, "Unsupported type");
        }
        return ustr::format("{}", res);
    }
}

};  // namespace jungle
