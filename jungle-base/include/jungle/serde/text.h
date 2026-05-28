#pragma once

#include "jungle/serde/serde.h"

#include <optional>

#include "jungle/debug.h"
#include "jungle/types/uchar.h"

namespace jungle::serde {

class TextTarget : public SerializeTarget<TextTarget> {
public:
    using result_type = ustr;

    TextTarget() = default;

    result_type deliver_result() { return std::move(m_result); }

    TextTarget spawn_subtarget() { return TextTarget{m_result}; }

    template<std::integral I>
    void serialize_integral(I value) {
        m_result.append(ustr::format("{}", value));
    }

    template<std::floating_point F>
    void serialize_floating_point(F value) {
        m_result.append(ustr::format("{}", value));
    }

    void serialize_bool(bool value) { m_result.append(value ? "true" : "false"); }

    template<concepts::is_enum E>
    void serialize_enum(E value) {
        m_result.append(debug(value));
    }

    void serialize_range_head(std::string_view ident) { m_result.append(ustr::format("{}[", ident)); }

    void serialize_range_element_end() { m_result.append(","); }

    void serialize_range_tail(std::string_view, usize) { m_result.append("]"); }

    void serialize_class_head(std::string_view ident) { m_result.append(ustr::format("{}{{", ident)); }

    void serialize_class_field(std::string_view ident) { m_result.append(ustr::format("{}:", ident)); }

    void serialize_class_field_end() { m_result.append(","); }

    void serialize_class_tail(std::string_view) { m_result.append("}"); }

private:
    TextTarget(ustr &external)
            : m_storage{std::nullopt}
            , m_result{external} {}

    std::optional<ustr> m_storage{ustr{}};
    ustr &m_result{*m_storage};
};

static_assert(SerializeTargetImpl<TextTarget>);

};  // namespace jungle::serde
