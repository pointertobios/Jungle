// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <charconv>
#include <concepts>
#include <expected>
#include <meta>
#include <optional>
#include <string_view>
#include <type_traits>

#include "jungle/debug.h"
#include "jungle/serde/deserialize.h"
#include "jungle/serde/serde.h"
#include "jungle/serde/serialize.h"
#include "jungle/types/concepts.h"
#include "jungle/types/int.h"
#include "jungle/types/uchar.h"

namespace jungle::core::asset {

class JamlTarget : public jungle::serde::SerializeTarget<JamlTarget> {
public:
    using target_type = ustr;

    JamlTarget();

    target_type deliver_result();
    JamlTarget spawn_subtarget();

    template<std::integral I>
    void serialize_integral(const I &value) {
        write_scalar(ustr::format("{}", value).view());
    }

    template<std::floating_point F>
    void serialize_floating_point(const F &value) {
        write_scalar(ustr::format("{}", value).view());
    }

    void serialize_bool(const bool &value);

    template<concepts::is_enum E>
    void serialize_enum(const E &value) {
        write_scalar(debug(value).view());
    }

    void serialize_optional_nonnull();
    void serialize_optional_nullopt();
    void serialize_range_head();
    void serialize_range_element_end();
    void serialize_range_tail(usize);
    void serialize_class_head(std::string_view ident);
    void serialize_class_field(std::string_view ident);
    void serialize_class_field_end();
    void serialize_class_tail(std::string_view ident);

private:
    JamlTarget(ustr &external, usize indent, bool inline_content, bool *block);

    static std::string_view xml_name(std::string_view ident);
    void write_newline_indent();
    void write_open(std::string_view name);
    void write_close(std::string_view name);
    void write_scalar(std::string_view text);

    std::optional<ustr> m_storage{ustr{}};
    ustr &m_result{m_storage.value()};
    usize m_indent{1};
    bool m_inline{false};
    bool m_owned_block{false};
    bool *m_block{&m_owned_block};
    bool m_wrap_item{false};
    bool m_same_indent{false};
    std::string_view m_open_field{};
};

static_assert(jungle::serde::SerializeTargetImpl<JamlTarget>);

class JamlSource : public jungle::serde::DeserializeSource<JamlSource> {
public:
    using source_type = ustr;

    struct error_type {
        enum class kind : u8 {
            unexpected_eof,
            empty_content,
            expected_bool,
            expected_number,
            invalid_number,
            number_out_of_range,
            expected_enum,
            unknown_enumerator,
            expected_null,
            expected_list,
            expected_item,
            expected_start_tag,
            expected_end_tag,
            tag_mismatch,
            malformed_tag,
            not_present,
            missing_xml_header,
            invalid_xml_header,
            build_id_mismatch,
        };

        kind m_kind{};
        usize m_position{};
        usize m_extent{};

        friend bool operator==(const error_type &, const error_type &) = default;
    };

    JamlSource() = default;

    void provide_source(const source_type &source);
    void provide_source(source_type &&source);
    JamlSource spawn_subsource();

    std::expected<void, error_type> deserialize_bool(bool &value);

    template<std::integral I>
    std::expected<void, error_type> deserialize_integral(I &value) {
        if (auto r = pending_header_error(); !r) {
            return r;
        }
        if (m_jaml_self_closed) {
            return fail(error_type::kind::empty_content);
        }
        auto token = consume_number();
        if (token.empty()) {
            return fail(error_type::kind::expected_number);
        }
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        return number_result(token, ec);
    }

    template<std::floating_point F>
    std::expected<void, error_type> deserialize_floating_point(F &value) {
        if (auto r = pending_header_error(); !r) {
            return r;
        }
        if (m_jaml_self_closed) {
            return fail(error_type::kind::empty_content);
        }
        auto token = consume_number();
        if (token.empty()) {
            return fail(error_type::kind::expected_number);
        }
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        return number_result(token, ec);
    }

    template<concepts::is_enum E>
    std::expected<void, error_type> deserialize_enum(E &value) {
        if (auto r = pending_header_error(); !r) {
            return r;
        }
        if (m_jaml_self_closed) {
            return fail(error_type::kind::empty_content);
        }
        auto full_name = consume_text_token();
        if (full_name.empty()) {
            return fail(error_type::kind::expected_enum);
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
        if (!found) {
            return fail(error_type::kind::unknown_enumerator, span_offset(full_name), full_name.size());
        }
        return {};
    }

    std::expected<void, error_type> deserialize_optional_nonnull();
    std::expected<void, error_type> deserialize_optional_nullopt();
    std::expected<void, error_type> deserialize_range_head();
    std::expected<void, error_type> deserialize_range_has_element();
    std::expected<void, error_type> deserialize_range_element_end();
    std::expected<void, error_type> deserialize_range_tail();
    std::expected<void, error_type> deserialize_class_head();
    std::expected<void, error_type> deserialize_class_field();
    std::expected<void, error_type> deserialize_class_field_end();
    std::expected<void, error_type> deserialize_class_tail();

private:
    struct ParsedTag {
        std::string_view name;
        bool self_closed = false;
    };

    JamlSource(std::string_view source, usize *cursor);

    // 头校验（缺头/无效头/build_id 不匹配）产生的待决错误；无则返回成功。
    std::expected<void, error_type> pending_header_error() const;
    void verify_header();

    std::unexpected<error_type> fail(error_type::kind kind, usize position, usize extent = 0) const {
        return std::unexpected{error_type{kind, position, extent}};
    }

    std::unexpected<error_type> fail(error_type::kind kind) const { return fail(kind, *m_cursor); }

    usize span_offset(std::string_view span) const {
        return static_cast<usize>(span.data() - m_source.data());
    }

    std::expected<void, error_type> number_result(std::string_view token, std::errc ec) const {
        if (ec == std::errc{}) {
            return {};
        }
        auto kind = (ec == std::errc::result_out_of_range) ? error_type::kind::number_out_of_range
                                                           : error_type::kind::invalid_number;
        return fail(kind, span_offset(token), token.size());
    }

    void bind_source();
    bool at_end() const;
    char current() const;
    char peek_at(usize offset) const;
    void advance();
    bool starts_with(std::string_view prefix) const;
    std::string_view token_at(usize start) const;

    static bool is_xml_ws(char c);
    static bool is_digit(char c);
    static bool is_name_start(char c);
    static bool is_name_char(char c);

    void skip_ws();
    void skip_bom();
    bool try_skip_comment();
    bool try_skip_pi();
    void skip_ignored();
    std::string_view parse_name();
    bool skip_quoted();
    bool skip_attributes();
    std::expected<ParsedTag, error_type> consume_start_tag();
    std::expected<ParsedTag, error_type>
    consume_start_tag(std::string_view expected, error_type::kind missing_kind);
    bool peek_start_tag(std::string_view expected);
    bool peek_self_closed(std::string_view expected);
    std::expected<void, error_type> consume_end_tag(std::string_view expected, error_type::kind missing_kind);
    bool try_consume_kw(std::string_view kw);
    std::string_view consume_number();
    std::string_view consume_text_token();
    usize remaining_token_extent() const;

    ustr m_owned_source;
    std::string_view m_source;
    usize m_owned_cursor{0};
    usize *m_cursor{&m_owned_cursor};
    bool m_jaml_self_closed{false};
    bool m_in_range{false};
    bool m_range_self_closed{false};
    bool m_class_self_closed{false};
    std::string_view m_class_tag{};
    std::string_view m_open_field{};
    bool m_field_self_closed{false};
    // XML 声明头校验失败（缺头/无效头/build_id 不匹配）时记录的待决反序列化错误。
    std::optional<error_type> m_pending_error{};
};

static_assert(jungle::serde::DeserializeSourceImpl<JamlSource>);
static_assert(std::is_trivially_copyable_v<JamlSource::error_type>);

};  // namespace jungle::core::asset
