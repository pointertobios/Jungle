// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/types/ustr.h"

#include "jungle/panic.h"

namespace jungle {

ustr::ustr(const char *str)
        : m_storage{str} {
    check_valid(std::span<const i8>{reinterpret_cast<const i8 *>(m_storage.data()), m_storage.size()});
}

ustr::ustr(std::string_view str)
        : m_storage{str} {
    check_valid(std::span<const i8>{reinterpret_cast<const i8 *>(m_storage.data()), m_storage.size()});
}

ustr::ustr(std::string &&str)
        : m_storage{std::move(str)} {
    check_valid(std::span<const i8>{reinterpret_cast<const i8 *>(m_storage.data()), m_storage.size()});
}

std::vector<uchar> ustr::to_uchars() const {
    std::vector<uchar> result(16);
    const auto str_view = view();
    size_t i = 0;

    while (i < str_view.size()) {
        const auto remaining =
            std::span<const i8>(reinterpret_cast<const i8 *>(str_view.data() + i), str_view.size() - i);
        const auto length = uchar::utf8_length(remaining);
        if (length == 0) {
            break;
        }
        const auto ch = uchar::from_utf8(remaining);
        if (!ch) {
            break;
        }
        result.push_back(ch);
        i += length;
    }

    return result;
}

void ustr::push(uchar ch) {
    const auto utf8 = ch.to_utf8();
    for (const auto byte : utf8) {
        if (byte == 0) {
            break;
        }
        m_storage.push_back(byte);
    }
}

void ustr::append(const ustr &other) { m_storage += other.m_storage; }

void ustr::append(ustr &&other) { m_storage += std::move(other.m_storage); }

void ustr::append(const char *str) { append(std::string_view{str}); }

void ustr::append(std::string_view str) {
    append(std::span<const i8>{reinterpret_cast<const i8 *>(str.data()), str.size()});
}

void ustr::append(std::span<const uchar> chars) {
    for (const auto &ch : chars) {
        push(ch);
    }
}

void ustr::append(std::span<const i8> char_range) {
    check_valid(char_range);
    m_storage.append_range(char_range);
}

void ustr::check_valid(std::span<const i8> utf8) {
    for (usize i = 0; i < utf8.size();) {
        const auto length = uchar::utf8_length(std::span<const i8>(utf8.data() + i, utf8.size() - i));
        if (length == 0) {
            panic("字节 {} 处不是有效的 UTF-8 字符", i);
        }
        i += length;
    }
}

};  // namespace jungle
