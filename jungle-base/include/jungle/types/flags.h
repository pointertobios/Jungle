// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <bit>
#include <meta>
#include <utility>

#include "jungle/assert.h"
#include "jungle/types/concepts.h"
#include "jungle/types/int.h"

namespace jungle {

template<concepts::is_enum E>
class flags {
    static constexpr usize enumerator_count = std::meta::enumerators_of(^^E).size();
    static_assert(enumerator_count <= 64, "flags enumerators must not exceed 64");

    static constexpr u64 valid_mask =
        enumerator_count == 64 ? ~u64{0} : (u64{1} << enumerator_count) - u64{1};

public:
    using enum_type = E;

    constexpr flags() = default;

    template<std::same_as<E>... Es>
        requires (sizeof...(Es) > 0)
    constexpr flags(Es... values) {
        (..., (m_bits |= bit_of(values)));
    }

    static constexpr flags none() { return {}; }

    static constexpr flags all() { return flags{valid_mask}; }

    constexpr bool empty() const { return m_bits == 0; }

    constexpr bool any() const { return m_bits != 0; }

    constexpr bool is_all() const { return m_bits == valid_mask; }

    constexpr usize size() const { return std::popcount(m_bits); }

    constexpr bool contains(E value) const {
        auto bit = bit_of(value);
        return (m_bits & bit) == bit;
    }

    constexpr bool contains(flags other) const { return (m_bits & other.m_bits) == other.m_bits; }

    constexpr bool intersects(flags other) const { return (m_bits & other.m_bits) != 0; }

    constexpr flags &set(E value) {
        m_bits |= bit_of(value);
        return *this;
    }

    constexpr flags &reset(E value) {
        m_bits &= ~bit_of(value);
        return *this;
    }

    constexpr flags &flip(E value) {
        m_bits ^= bit_of(value);
        return *this;
    }

    constexpr flags &clear() {
        m_bits = 0;
        return *this;
    }

    constexpr bool operator[](E value) const { return contains(value); }

    constexpr explicit operator bool() const { return any(); }

    constexpr bool operator==(const flags &) const = default;

    constexpr flags &operator|=(flags rhs) {
        m_bits |= rhs.m_bits;
        return *this;
    }

    constexpr flags &operator|=(E rhs) {
        m_bits |= bit_of(rhs);
        return *this;
    }

    constexpr flags &operator&=(flags rhs) {
        m_bits &= rhs.m_bits;
        return *this;
    }

    friend constexpr flags operator|(flags lhs, flags rhs) {
        lhs |= rhs;
        return lhs;
    }

    friend constexpr flags operator|(flags lhs, E rhs) {
        lhs |= rhs;
        return lhs;
    }

    friend constexpr flags operator|(E lhs, flags rhs) {
        rhs |= lhs;
        return rhs;
    }

    friend constexpr flags operator&(flags lhs, flags rhs) {
        lhs &= rhs;
        return lhs;
    }

private:
    explicit constexpr flags(u64 bits)
            : m_bits{bits} {}

    static constexpr u64 bit_of(E value) {
        u64 bit = 1;
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
            if (value == [:e:]) {
                return bit;
            }
            bit <<= 1;
        }
        JUNGLE_ASSERT(false, "invalid enumerator");
        std::unreachable();
    }

    u64 m_bits{0};
};

};  // namespace jungle
