// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/types/uchar.h"

namespace jungle {

usize uchar::utf8_length(std::span<const i8> utf8) {
    if (utf8.empty()) {
        return 0;
    }

    const auto first = static_cast<u8>(utf8[0]);
    if ((first & 0x80) == 0) {
        return 1;
    }

    if ((first & 0xE0) == 0xC0) {
        if (utf8.size() < 2) {
            return 0;
        }
        const auto second = static_cast<u8>(utf8[1]);
        if (first < 0xC2 || (second & 0xC0) != 0x80) {
            return 0;
        }
        return 2;
    }

    if ((first & 0xF0) == 0xE0) {
        if (utf8.size() < 3) {
            return 0;
        }
        const auto second = static_cast<u8>(utf8[1]);
        const auto third = static_cast<u8>(utf8[2]);
        if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80) {
            return 0;
        }
        if (first == 0xE0 && second < 0xA0) {
            return 0;
        }
        if (first == 0xED && second >= 0xA0) {
            return 0;
        }
        return 3;
    }

    if ((first & 0xF8) == 0xF0) {
        if (utf8.size() < 4) {
            return 0;
        }
        const auto second = static_cast<u8>(utf8[1]);
        const auto third = static_cast<u8>(utf8[2]);
        const auto fourth = static_cast<u8>(utf8[3]);
        if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80 || (fourth & 0xC0) != 0x80) {
            return 0;
        }
        if (first == 0xF0 && second < 0x90) {
            return 0;
        }
        if (first > 0xF4 || (first == 0xF4 && second >= 0x90)) {
            return 0;
        }
        return 4;
    }

    return 0;
}

uchar uchar::from_utf8(std::span<const i8> utf8) {
    const auto length = utf8_length(utf8);
    if (length == 0) {
        return uchar{INVALID};
    }

    const auto first = static_cast<u8>(utf8[0]);
    u32 value = 0;

    if (length == 1) {
        value = first;
    } else if (length == 2) {
        value = (static_cast<u32>(first & 0x1F) << 6) | static_cast<u32>(static_cast<u8>(utf8[1]) & 0x3F);
    } else if (length == 3) {
        value = (static_cast<u32>(first & 0x0F) << 12)
                | (static_cast<u32>(static_cast<u8>(utf8[1]) & 0x3F) << 6)
                | static_cast<u32>(static_cast<u8>(utf8[2]) & 0x3F);
    } else {
        value = (static_cast<u32>(first & 0x07) << 18)
                | (static_cast<u32>(static_cast<u8>(utf8[1]) & 0x3F) << 12)
                | (static_cast<u32>(static_cast<u8>(utf8[2]) & 0x3F) << 6)
                | static_cast<u32>(static_cast<u8>(utf8[3]) & 0x3F);
    }

    if (value == INVALID || value > MAX || (value >= 0xD800 && value <= 0xDFFF)) {
        return uchar{INVALID};
    }

    return uchar{value};
}

std::array<i8, 4> uchar::to_utf8() const {
    std::array<i8, 4> utf8{};

    if (m_value <= 0x7F) {
        utf8[0] = static_cast<i8>(m_value);
        return utf8;
    }

    if (m_value <= 0x7FF) {
        utf8[0] = static_cast<i8>(0xC0 | (m_value >> 6));
        utf8[1] = static_cast<i8>(0x80 | (m_value & 0x3F));
        return utf8;
    }

    if (m_value >= 0xD800 && m_value <= 0xDFFF) {
        return utf8;
    }

    if (m_value <= 0xFFFF) {
        utf8[0] = static_cast<i8>(0xE0 | (m_value >> 12));
        utf8[1] = static_cast<i8>(0x80 | ((m_value >> 6) & 0x3F));
        utf8[2] = static_cast<i8>(0x80 | (m_value & 0x3F));
        return utf8;
    }

    if (m_value <= MAX) {
        utf8[0] = static_cast<i8>(0xF0 | (m_value >> 18));
        utf8[1] = static_cast<i8>(0x80 | ((m_value >> 12) & 0x3F));
        utf8[2] = static_cast<i8>(0x80 | ((m_value >> 6) & 0x3F));
        utf8[3] = static_cast<i8>(0x80 | (m_value & 0x3F));
    }

    return utf8;
}

};  // namespace jungle
