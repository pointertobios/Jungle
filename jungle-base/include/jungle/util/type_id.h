#pragma once

#include <string_view>
#include <utility>

#include "jungle/util/murmur.h"

namespace jungle::util {

class type_id {
public:
    using type_hash = hash_val;

    constexpr type_id() noexcept
            : m_hash{} {}

    constexpr type_id(const type_id &) noexcept = default;
    type_id &operator=(const type_id &) = delete;

    constexpr type_id(type_id &&) noexcept = default;
    constexpr type_id &operator=(type_id &&rhs) noexcept {
        if (this != &rhs) {
            this->~type_id();
            new (this) type_id(std::move(rhs));
        }
        return *this;
    }

    template<typename T>
    consteval static type_id of() noexcept {
#if defined(__clang__) || defined(__GNUC__)
        constexpr std::string_view funcname = __PRETTY_FUNCTION__;
#elif defined(__FUNCSIG__)
        constexpr std::string_view funcname = __FUNCSIG__;
#else
#    error "Unsupported compiler"
#endif

        std::size_t start = funcname.find_first_of('=') + 2;
        std::size_t end = funcname.find_last_of(']');

        std::string_view name = funcname.substr(start, end - start);
        const auto hash = hash_str(name);
        return type_id{name, hash};
    }

    constexpr std::string_view name() const noexcept { return m_name; }

    constexpr const type_hash &hash() const noexcept { return m_hash; }

    constexpr bool operator==(const type_id &rhs) const noexcept {
        return
#ifndef NDEBUG
            this->m_name == rhs.m_name &&
#endif
            this->m_hash == rhs.m_hash;
    }

private:
    consteval type_id(std::string_view name, type_hash hash) noexcept
            : m_name{name}
            , m_hash{hash} {}

    const std::string_view m_name;
    const type_hash m_hash;
};

};  // namespace jungle::util
