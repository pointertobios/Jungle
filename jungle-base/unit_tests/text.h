#pragma once

#include <charconv>
#include <cstddef>
#include <optional>

#include "jungle/debug.h"
#include "jungle/panic.h"
#include "jungle/serde/deserialize.h"
#include "jungle/serde/serde.h"
#include "jungle/serde/serialize.h"
#include "jungle/types/uchar.h"

namespace jungle::serde {

class TextTarget : public SerializeTarget<TextTarget> {
public:
    using target_type = ustr;

    TextTarget() = default;

    target_type deliver_result() { return std::move(m_result); }

    TextTarget spawn_subtarget() { return TextTarget{m_result}; }

    template<std::integral I>
    void serialize_integral(const I &value) {
        m_result.append(ustr::format("{}", value));
    }

    template<std::floating_point F>
    void serialize_floating_point(const F &value) {
        m_result.append(ustr::format("{}", value));
    }

    void serialize_bool(const bool &value) { m_result.append(value ? "true" : "false"); }

    template<concepts::is_enum E>
    void serialize_enum(const E &value) {
        m_result.append(debug(value));
    }

    void serialize_optional_nonnull() { m_result.append("optional##"); }

    void serialize_optional_nullopt() { m_result.append("optional##nullopt"); }

    void serialize_range_head() { m_result.append("["); }

    void serialize_range_element_end() { m_result.append(","); }

    void serialize_range_tail(usize) { m_result.append("]"); }

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

class TextSource : public DeserializeSource<TextSource> {
public:
    using source_type = ustr;

    TextSource() = default;

    void provide_source(const source_type &source) {
        m_source = source.view();
        *m_cursor = 0;
    }

    TextSource spawn_subsource() { return TextSource{m_source, m_cursor}; }

    void deserialize_bool(bool &value) {
        if (try_consume_kw("true")) {
            value = true;
            return;
        }
        if (try_consume_kw("false")) {
            value = false;
            return;
        }
        panic("expected \"true\" or \"false\"");
    }

    template<std::integral I>
    void deserialize_integral(I &value) {
        auto token = consume_number();
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        if (ec != std::errc{}) {
            panic("failed to parse integral from text");
        }
    }

    template<std::floating_point F>
    void deserialize_floating_point(F &value) {
        auto token = consume_number();
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        if (ec != std::errc{}) {
            panic("failed to parse floating-point from text");
        }
    }

    template<concepts::is_enum E>
    void deserialize_enum(E &value) {
        auto full_name = consume_value_token();

        auto colon_pos = full_name.rfind("::");
        std::string_view enumerator_name =
            (colon_pos != std::string_view::npos) ? full_name.substr(colon_pos + 2) : full_name;

        constexpr auto type_info = ^^E;
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(type_info))) {
            if (enumerator_name == std::meta::identifier_of(e)) {
                value = [:e:];
            }
        }
    }

    bool deserialize_optional_nonnull() {
        expect_prefix("optional##");
        if (try_consume_kw("nullopt"))
            return false;
        return true;
    }

    void deserialize_range_head() { expect('['); }

    bool deserialize_range_has_element() { return !at_end() && current() != ']'; }

    void deserialize_range_element_end() { expect(','); }

    void deserialize_range_tail() { expect(']'); }

    std::string_view deserialize_class_head() {
        auto name = consume_until('{');
        advance();
        return name;
    }

    std::string_view deserialize_class_field() {
        auto name = consume_until(':');
        advance();
        return name;
    }

    void deserialize_class_field_end() { expect(','); }

    void deserialize_class_tail() { expect('}'); }

private:
    TextSource(std::string_view source, usize *cursor)
            : m_source{source}
            , m_cursor{cursor} {}

    usize peek_pos() const { return *m_cursor; }

    bool at_end() const { return *m_cursor >= m_source.size(); }

    char current() const { return m_source[*m_cursor]; }

    void advance() { ++(*m_cursor); }

    void expect(char c) {
        if (at_end() || current() != c) {
            panic("unexpected character in text deserialization");
        }
        advance();
    }

    void expect_prefix(std::string_view prefix) {
        if (!try_consume_prefix(prefix)) {
            panic("unexpected prefix in text deserialization");
        }
    }

    bool try_consume_prefix(std::string_view prefix) {
        if (m_source.substr(*m_cursor).starts_with(prefix)) {
            *m_cursor += prefix.size();
            return true;
        }
        return false;
    }

    bool try_consume_kw(std::string_view kw) { return try_consume_prefix(kw); }

    std::string_view token_at(usize start) const { return m_source.substr(start, *m_cursor - start); }

    std::string_view consume_until(char delim) {
        usize start = *m_cursor;
        while (!at_end() && current() != delim) {
            advance();
        }
        return token_at(start);
    }

    static bool is_value_terminator(char c) { return c == ',' || c == '}' || c == ']'; }

    std::string_view consume_value_token() {
        usize start = *m_cursor;
        while (!at_end() && !is_value_terminator(current())) {
            advance();
        }
        return token_at(start);
    }

    std::string_view consume_number() {
        usize start = *m_cursor;
        if (!at_end() && current() == '-')
            advance();
        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())))
            advance();
        if (!at_end() && current() == '.') {
            advance();
            while (!at_end() && std::isdigit(static_cast<unsigned char>(current())))
                advance();
        }
        if (!at_end() && (current() == 'e' || current() == 'E')) {
            advance();
            if (!at_end() && (current() == '+' || current() == '-'))
                advance();
            while (!at_end() && std::isdigit(static_cast<unsigned char>(current())))
                advance();
        }
        return token_at(start);
    }

    std::string_view m_source;
    usize m_owned_cursor{0};
    usize *m_cursor{&m_owned_cursor};
};

static_assert(DeserializeSourceImpl<TextSource>);

};  // namespace jungle::serde
