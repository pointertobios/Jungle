// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <meta>
#include <string_view>
#include <utility>

#include "jungle/types/int.h"

namespace jungle::util {

class type_id {
    friend struct std::hash<type_id>;

    template<typename T>
    constexpr static u8 identifier = 0;

public:
    constexpr type_id() = default;

    constexpr type_id(const type_id &) = default;
    constexpr type_id &operator=(const type_id &rhs) {
        if (this != &rhs) {
            this->~type_id();
            new (this) type_id{rhs};
        }
        return *this;
    }

    constexpr type_id(type_id &&) = default;
    constexpr type_id &operator=(type_id &&rhs) {
        if (this != &rhs) {
            this->~type_id();
            new (this) type_id{std::move(rhs)};
        }
        return *this;
    }

    template<typename T>
    consteval static type_id of() {
        constexpr auto type = std::meta::dealias(^^T);
        constexpr auto name = std::meta::display_string_of(type);
        return type_id{&identifier<T>, name};
    }

    consteval static type_id none() { return type_id{nullptr, "<none>"}; }

    constexpr std::string_view name() const { return m_name; }

    constexpr bool operator==(const type_id &rhs) const {
        return
#ifndef NDEBUG
            this->m_name == rhs.m_name &&
#endif
            this->m_identifier == rhs.m_identifier;
    }

private:
    consteval type_id(const u8 *identifier, std::string_view name)
            : m_identifier{identifier}
            , m_name{name} {}

    const u8 *m_identifier{nullptr};
    const std::string_view m_name;
};

};  // namespace jungle::util

template<>
struct std::hash<jungle::util::type_id> {
    std::size_t operator()(const jungle::util::type_id &id) const {
        return reinterpret_cast<std::size_t>(id.m_identifier);
    }
};
