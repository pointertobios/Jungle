// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <charconv>
#include <cstddef>
#include <expected>
#include <optional>

#include "jungle/debug.h"
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
    ustr &m_result{m_storage.value()};
};

static_assert(SerializeTargetImpl<TextTarget>);

class TextSource : public DeserializeSource<TextSource> {
public:
    using source_type = ustr;
    enum class error_type { mismatch };

    TextSource() = default;

    void provide_source(const source_type &source) {
        m_source = source.view();
        *m_cursor = 0;
    }

    TextSource spawn_subsource() { return TextSource{m_source, m_cursor}; }

    std::expected<void, error_type> deserialize_bool(bool &value) {
        if (try_consume_kw("true")) {
            value = true;
            return {};
        }
        if (try_consume_kw("false")) {
            value = false;
            return {};
        }
        return std::unexpected{error_type::mismatch};
    }

    template<std::integral I>
    std::expected<void, error_type> deserialize_integral(I &value) {
        auto token = consume_number();
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        return result_from(ec == std::errc{});
    }

    template<std::floating_point F>
    std::expected<void, error_type> deserialize_floating_point(F &value) {
        auto token = consume_number();
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        return result_from(ec == std::errc{});
    }

    template<concepts::is_enum E>
    std::expected<void, error_type> deserialize_enum(E &value) {
        auto full_name = consume_value_token();
        if (full_name.empty()) {
            return std::unexpected{error_type::mismatch};
        }

        auto colon_pos = full_name.rfind("::");
        std::string_view enumerator_name =
            (colon_pos != std::string_view::npos) ? full_name.substr(colon_pos + 2) : full_name;

        constexpr auto type_info = ^^E;
        bool found = false;
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(type_info))) {
            if (enumerator_name == std::meta::identifier_of(e)) {
                value = [:e:];
                found = true;
            }
        }
        return result_from(found);
    }

    std::expected<void, error_type> deserialize_optional_nonnull() {
        if (!try_consume_kw("optional##")) {
            return std::unexpected{error_type::mismatch};
        }
        if (peek_kw("nullopt")) {
            return std::unexpected{error_type::mismatch};
        }
        return {};
    }

    std::expected<void, error_type> deserialize_optional_nullopt() {
        return result_from(try_consume_kw("nullopt"));
    }

    std::expected<void, error_type> deserialize_range_head() { return result_from(try_consume_char('[')); }

    std::expected<void, error_type> deserialize_range_has_element() {
        return result_from(!at_end() && current() != ']');
    }

    std::expected<void, error_type> deserialize_range_element_end() {
        return result_from(try_consume_char(','));
    }

    std::expected<void, error_type> deserialize_range_tail() { return result_from(try_consume_char(']')); }

    std::expected<void, error_type> deserialize_class_head() {
        consume_until('{');
        return result_from(try_consume_char('{'));
    }

    std::expected<void, error_type> deserialize_class_field() {
        consume_until(':');
        return result_from(try_consume_char(':'));
    }

    std::expected<void, error_type> deserialize_class_field_end() {
        return result_from(try_consume_char(','));
    }

    std::expected<void, error_type> deserialize_class_tail() { return result_from(try_consume_char('}')); }

private:
    TextSource(std::string_view source, usize *cursor)
            : m_source{source}
            , m_cursor{cursor} {}

    std::expected<void, error_type> result_from(bool ok) const {
        if (ok) {
            return {};
        }
        return std::unexpected{error_type::mismatch};
    }

    bool at_end() const { return *m_cursor >= m_source.size(); }

    char current() const { return m_source[*m_cursor]; }

    void advance() { ++(*m_cursor); }

    bool try_consume_char(char c) {
        if (at_end() || current() != c)
            return false;
        advance();
        return true;
    }

    bool try_consume_prefix(std::string_view prefix) {
        if (m_source.substr(*m_cursor).starts_with(prefix)) {
            *m_cursor += prefix.size();
            return true;
        }
        return false;
    }

    bool try_consume_kw(std::string_view kw) { return try_consume_prefix(kw); }

    bool peek_kw(std::string_view kw) { return m_source.substr(*m_cursor).starts_with(kw); }

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
