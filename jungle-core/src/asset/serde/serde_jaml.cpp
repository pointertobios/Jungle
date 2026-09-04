// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/asset/serde/serde_jaml.h"

#include <utility>

namespace jungle::core::asset {

JamlTarget::JamlTarget() { m_result.append("<jaml>"); }

JamlTarget::JamlTarget(ustr &external, usize indent, bool inline_content, bool *block)
        : m_storage{std::nullopt}
        , m_result{external}
        , m_indent{indent}
        , m_inline{inline_content}
        , m_block{block} {}

JamlTarget::target_type JamlTarget::deliver_result() {
    m_result.append("\n</jaml>");
    return std::move(m_result);
}

JamlTarget JamlTarget::spawn_subtarget() {
    if (m_wrap_item) {
        write_newline_indent();
        m_result.append("<item>");
        m_wrap_item = false;
        return JamlTarget{m_result, m_indent + 1, true, m_block};
    }
    if (m_same_indent) {
        m_same_indent = false;
        return JamlTarget{m_result, m_indent, m_inline, m_block};
    }
    return JamlTarget{m_result, m_indent + 1, true, m_block};
}

void JamlTarget::serialize_bool(const bool &value) { write_scalar(value ? "true" : "false"); }

void JamlTarget::serialize_optional_nonnull() { m_same_indent = true; }

void JamlTarget::serialize_optional_nullopt() {
    if (!m_inline) {
        write_newline_indent();
    }
    m_result.append("<null/>");
    *m_block = !m_inline;
}

void JamlTarget::serialize_range_head() {
    write_newline_indent();
    m_result.append("<list>");
    m_indent += 1;
    m_wrap_item = true;
    *m_block = true;
}

void JamlTarget::serialize_range_element_end() {
    if (*m_block) {
        write_newline_indent();
    }
    m_result.append("</item>");
    m_wrap_item = true;
}

void JamlTarget::serialize_range_tail(usize) {
    m_indent -= 1;
    write_newline_indent();
    m_result.append("</list>");
    m_wrap_item = false;
    *m_block = true;
}

void JamlTarget::serialize_class_head(std::string_view ident) {
    write_newline_indent();
    write_open(xml_name(ident));
    m_indent += 1;
    *m_block = true;
}

void JamlTarget::serialize_class_field(std::string_view ident) {
    auto name = xml_name(ident);
    write_newline_indent();
    write_open(name);
    m_open_field = name;
}

void JamlTarget::serialize_class_field_end() {
    if (*m_block) {
        write_newline_indent();
    }
    write_close(m_open_field);
}

void JamlTarget::serialize_class_tail(std::string_view ident) {
    m_indent -= 1;
    write_newline_indent();
    write_close(xml_name(ident));
    *m_block = true;
}

std::string_view JamlTarget::xml_name(std::string_view ident) {
    if (ident == "<unnamed>") {
        return "_unnamed";
    }
    return ident;
}

void JamlTarget::write_newline_indent() {
    m_result.append("\n");
    for (usize i = 0; i < m_indent; ++i) {
        m_result.append("  ");
    }
}

void JamlTarget::write_open(std::string_view name) {
    m_result.append("<");
    m_result.append(name);
    m_result.append(">");
}

void JamlTarget::write_close(std::string_view name) {
    m_result.append("</");
    m_result.append(name);
    m_result.append(">");
}

void JamlTarget::write_scalar(std::string_view text) {
    if (!m_inline) {
        write_newline_indent();
    }
    m_result.append(text);
    *m_block = false;
}

void JamlSource::provide_source(const source_type &source) {
    m_owned_source = {};
    m_source = source.view();
    bind_source();
}

void JamlSource::provide_source(source_type &&source) {
    m_owned_source = std::move(source);
    m_source = m_owned_source.view();
    bind_source();
}

void JamlSource::bind_source() {
    *m_cursor = 0;
    skip_bom();
    skip_ignored();
    if (peek_start_tag("jaml")) {
        auto tag = consume_start_tag("jaml", error_type::kind::expected_start_tag);
        m_jaml_self_closed = tag.has_value() && tag->self_closed;
    }
}

JamlSource JamlSource::spawn_subsource() {
    if (m_in_range) {
        (void)consume_start_tag("item", error_type::kind::expected_item);
    }
    return JamlSource{m_source, m_cursor};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_bool(bool &value) {
    if (m_jaml_self_closed) {
        return fail(error_type::kind::empty_content);
    }
    skip_ignored();
    if (at_end()) {
        return fail(error_type::kind::unexpected_eof);
    }
    if (try_consume_kw("true")) {
        value = true;
        return {};
    }
    if (try_consume_kw("false")) {
        value = false;
        return {};
    }
    return fail(error_type::kind::expected_bool, *m_cursor, remaining_token_extent());
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_optional_nonnull() {
    if (m_jaml_self_closed) {
        return fail(error_type::kind::not_present);
    }
    skip_ignored();
    if (at_end() || (current() == '<' && peek_at(1) == '/')) {
        return fail(error_type::kind::not_present);
    }
    if (peek_self_closed("null")) {
        return fail(error_type::kind::not_present);
    }
    return {};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_optional_nullopt() {
    if (m_jaml_self_closed) {
        return fail(error_type::kind::empty_content);
    }
    auto tag = consume_start_tag("null", error_type::kind::expected_null);
    if (!tag) {
        return std::unexpected{tag.error()};
    }
    if (!tag->self_closed) {
        return fail(error_type::kind::malformed_tag, span_offset(tag->name), tag->name.size());
    }
    return {};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_range_head() {
    if (m_jaml_self_closed) {
        return fail(error_type::kind::empty_content);
    }
    auto tag = consume_start_tag("list", error_type::kind::expected_list);
    if (!tag) {
        return std::unexpected{tag.error()};
    }
    m_range_self_closed = tag->self_closed;
    m_in_range = !tag->self_closed;
    return {};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_range_has_element() {
    if (m_range_self_closed) {
        return fail(error_type::kind::not_present);
    }
    skip_ignored();
    if (!peek_start_tag("item")) {
        return fail(error_type::kind::not_present);
    }
    return {};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_range_element_end() {
    return consume_end_tag("item", error_type::kind::expected_item);
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_range_tail() {
    m_in_range = false;
    if (m_range_self_closed) {
        m_range_self_closed = false;
        return {};
    }
    return consume_end_tag("list", error_type::kind::expected_list);
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_class_head() {
    if (m_jaml_self_closed) {
        return fail(error_type::kind::empty_content);
    }
    auto tag = consume_start_tag();
    if (!tag) {
        return std::unexpected{tag.error()};
    }
    m_class_tag = tag->name;
    m_class_self_closed = tag->self_closed;
    return {};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_class_field() {
    if (m_class_self_closed) {
        return fail(error_type::kind::expected_start_tag);
    }
    auto tag = consume_start_tag();
    if (!tag) {
        return std::unexpected{tag.error()};
    }
    m_open_field = tag->name;
    m_field_self_closed = tag->self_closed;
    return {};
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_class_field_end() {
    if (m_field_self_closed) {
        m_field_self_closed = false;
        return {};
    }
    return consume_end_tag(m_open_field, error_type::kind::expected_end_tag);
}

std::expected<void, JamlSource::error_type> JamlSource::deserialize_class_tail() {
    if (m_class_self_closed) {
        m_class_self_closed = false;
        return {};
    }
    return consume_end_tag(m_class_tag, error_type::kind::expected_end_tag);
}

JamlSource::JamlSource(std::string_view source, usize *cursor)
        : m_source{source}
        , m_cursor{cursor} {}

bool JamlSource::at_end() const { return *m_cursor >= m_source.size(); }

char JamlSource::current() const { return m_source[*m_cursor]; }

char JamlSource::peek_at(usize offset) const {
    if (*m_cursor + offset >= m_source.size()) {
        return '\0';
    }
    return m_source[*m_cursor + offset];
}

void JamlSource::advance() { ++(*m_cursor); }

bool JamlSource::starts_with(std::string_view prefix) const {
    return m_source.substr(*m_cursor).starts_with(prefix);
}

std::string_view JamlSource::token_at(usize start) const {
    return m_source.substr(start, *m_cursor - start);
}

bool JamlSource::is_xml_ws(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

bool JamlSource::is_digit(char c) { return c >= '0' && c <= '9'; }

bool JamlSource::is_name_start(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == ':';
}

bool JamlSource::is_name_char(char c) { return is_name_start(c) || is_digit(c) || c == '-' || c == '.'; }

void JamlSource::skip_ws() {
    while (!at_end() && is_xml_ws(current())) {
        advance();
    }
}

void JamlSource::skip_bom() {
    if (m_source.size() >= 3 && static_cast<unsigned char>(m_source[0]) == 0xEF
        && static_cast<unsigned char>(m_source[1]) == 0xBB
        && static_cast<unsigned char>(m_source[2]) == 0xBF) {
        *m_cursor = 3;
    }
}

bool JamlSource::try_skip_comment() {
    if (!starts_with("<!--")) {
        return false;
    }
    *m_cursor += 4;
    while (!at_end()) {
        if (starts_with("-->")) {
            *m_cursor += 3;
            return true;
        }
        advance();
    }
    return true;
}

bool JamlSource::try_skip_pi() {
    if (!starts_with("<?")) {
        return false;
    }
    *m_cursor += 2;
    while (!at_end()) {
        if (starts_with("?>")) {
            *m_cursor += 2;
            return true;
        }
        advance();
    }
    return true;
}

void JamlSource::skip_ignored() {
    for (;;) {
        skip_ws();
        if (try_skip_comment() || try_skip_pi()) {
            continue;
        }
        break;
    }
}

std::string_view JamlSource::parse_name() {
    usize start = *m_cursor;
    if (at_end() || !is_name_start(current())) {
        return {};
    }
    advance();
    while (!at_end() && is_name_char(current())) {
        advance();
    }
    return token_at(start);
}

bool JamlSource::skip_quoted() {
    if (at_end()) {
        return false;
    }
    char quote = current();
    if (quote != '"' && quote != '\'') {
        return false;
    }
    advance();
    while (!at_end() && current() != quote) {
        advance();
    }
    if (at_end()) {
        return false;
    }
    advance();
    return true;
}

bool JamlSource::skip_attributes() {
    for (;;) {
        skip_ws();
        if (at_end()) {
            return false;
        }
        if (current() == '/' || current() == '>') {
            return true;
        }
        if (parse_name().empty()) {
            return false;
        }
        skip_ws();
        if (at_end() || current() != '=') {
            return false;
        }
        advance();
        skip_ws();
        if (!skip_quoted()) {
            return false;
        }
    }
}

std::expected<JamlSource::ParsedTag, JamlSource::error_type> JamlSource::consume_start_tag() {
    skip_ignored();
    const usize pos = *m_cursor;
    if (at_end()) {
        return fail(error_type::kind::unexpected_eof, pos);
    }
    if (current() != '<' || peek_at(1) == '/') {
        return fail(error_type::kind::expected_start_tag, pos);
    }
    advance();
    auto name = parse_name();
    if (name.empty()) {
        return fail(error_type::kind::malformed_tag, pos, *m_cursor - pos);
    }
    if (!skip_attributes()) {
        return fail(error_type::kind::malformed_tag, pos, *m_cursor - pos);
    }
    bool self_closed = false;
    if (!at_end() && current() == '/') {
        self_closed = true;
        advance();
    }
    if (at_end()) {
        return fail(error_type::kind::unexpected_eof, pos, *m_cursor - pos);
    }
    if (current() != '>') {
        return fail(error_type::kind::malformed_tag, pos, *m_cursor - pos);
    }
    advance();
    return ParsedTag{name, self_closed};
}

std::expected<JamlSource::ParsedTag, JamlSource::error_type> JamlSource::consume_start_tag(
    std::string_view expected, error_type::kind missing_kind) {
    auto tag = consume_start_tag();
    if (!tag) {
        if (tag.error().m_kind == error_type::kind::expected_start_tag) {
            return fail(missing_kind, tag.error().m_position, tag.error().m_extent);
        }
        return std::unexpected{tag.error()};
    }
    if (tag->name != expected) {
        return fail(error_type::kind::tag_mismatch, span_offset(tag->name), tag->name.size());
    }
    return tag;
}

bool JamlSource::peek_start_tag(std::string_view expected) {
    const usize saved = *m_cursor;
    auto tag = consume_start_tag();
    *m_cursor = saved;
    return tag.has_value() && tag->name == expected;
}

bool JamlSource::peek_self_closed(std::string_view expected) {
    const usize saved = *m_cursor;
    auto tag = consume_start_tag();
    *m_cursor = saved;
    return tag.has_value() && tag->name == expected && tag->self_closed;
}

std::expected<void, JamlSource::error_type> JamlSource::consume_end_tag(
    std::string_view expected, error_type::kind missing_kind) {
    skip_ignored();
    const usize pos = *m_cursor;
    if (at_end()) {
        return fail(error_type::kind::unexpected_eof, pos);
    }
    if (current() != '<' || peek_at(1) != '/') {
        return fail(missing_kind, pos);
    }
    advance();
    advance();
    auto name = parse_name();
    if (name.empty()) {
        return fail(error_type::kind::malformed_tag, pos, *m_cursor - pos);
    }
    if (!expected.empty() && name != expected) {
        return fail(error_type::kind::tag_mismatch, span_offset(name), name.size());
    }
    skip_ws();
    if (at_end()) {
        return fail(error_type::kind::unexpected_eof, pos, *m_cursor - pos);
    }
    if (current() != '>') {
        return fail(error_type::kind::malformed_tag, pos, *m_cursor - pos);
    }
    advance();
    return {};
}

bool JamlSource::try_consume_kw(std::string_view kw) {
    if (!starts_with(kw)) {
        return false;
    }
    usize next = *m_cursor + kw.size();
    if (next < m_source.size()) {
        char c = m_source[next];
        if (is_name_char(c)) {
            return false;
        }
    }
    *m_cursor = next;
    return true;
}

std::string_view JamlSource::consume_number() {
    skip_ignored();
    usize start = *m_cursor;
    if (!at_end() && current() == '-') {
        advance();
    }
    while (!at_end() && is_digit(current())) {
        advance();
    }
    if (!at_end() && current() == '.') {
        advance();
        while (!at_end() && is_digit(current())) {
            advance();
        }
    }
    if (!at_end() && (current() == 'e' || current() == 'E')) {
        advance();
        if (!at_end() && (current() == '+' || current() == '-')) {
            advance();
        }
        while (!at_end() && is_digit(current())) {
            advance();
        }
    }
    return token_at(start);
}

std::string_view JamlSource::consume_text_token() {
    skip_ignored();
    usize start = *m_cursor;
    usize last_non_ws = start;
    while (!at_end() && current() != '<') {
        char c = current();
        advance();
        if (!is_xml_ws(c)) {
            last_non_ws = *m_cursor;
        }
    }
    return m_source.substr(start, last_non_ws - start);
}

usize JamlSource::remaining_token_extent() const {
    usize i = *m_cursor;
    while (i < m_source.size() && !is_xml_ws(m_source[i]) && m_source[i] != '<') {
        ++i;
    }
    return i - *m_cursor;
}

};  // namespace jungle::core::asset
