// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/util/type_id.h"

namespace jungle::util {

template<typename T>
class type_mutate {
    template<typename U>
    static constexpr auto static_mutatable = T::template static_mutatable<U>;

public:
    template<typename U>
        requires static_mutatable<U>
    constexpr bool is() const {
        return m_type == type_id::of<U>();
    }

    constexpr bool is(type_id type) const { return m_type == type; }

    template<typename U>
        requires static_mutatable<U>
    constexpr U &as() pre(is<U>()) {
        if (!is<U>()) {
            panic("assertion failed: is<U>()");
        }  // TODO: workaround gcc contract bug for template function
        return static_cast<U &>(*this);
    }

    template<typename U>
        requires static_mutatable<U>
    constexpr const U &as() const pre(is<U>()) {
        if (!is<U>()) {
            panic("assertion failed: is<U>()");
        }  // TODO: workaround gcc contract bug for template function
        return static_cast<const U &>(*this);
    }

    template<typename U>
        requires static_mutatable<U>
    constexpr U *try_as() {
        return is<U>() ? &static_cast<U &>(*this) : nullptr;
    }

    template<typename U>
        requires static_mutatable<U>
    constexpr const U *try_as() const {
        return is<U>() ? &static_cast<const U &>(*this) : nullptr;
    }

    type_id type() const { return m_type; }

protected:
    constexpr type_mutate(type_id type)
            : m_type{type} {}

private:
    type_id m_type;
};

};  // namespace jungle::util
