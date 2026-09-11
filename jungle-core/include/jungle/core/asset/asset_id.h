// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <optional>
#include <ranges>

#include "jungle/panic.h"
#include "jungle/preusing.h"
#include "jungle/util/murmur.h"

namespace jungle::core {

class AssetID {
    friend struct std::hash<AssetID>;

    static constexpr u128 NONE = 0;

public:
    static constexpr AssetID none() { return {NONE}; }

    constexpr AssetID(u128 id)
            : m_id{id} {}

    static consteval AssetID from_path(std::string_view path);

    constexpr AssetID() = default;
    constexpr AssetID(const AssetID &) = default;
    constexpr AssetID &operator=(const AssetID &) = default;
    constexpr AssetID(AssetID &&) = default;
    constexpr AssetID &operator=(AssetID &&) = default;

    constexpr operator bool() const { return m_id != NONE; }
    constexpr bool operator==(const AssetID &other) const { return m_id == other.m_id; }

    ustr debug() const;

private:

    u128 m_id{0};
};

consteval AssetID AssetID::from_path(std::string_view path) {
    constexpr u128 empty_section_hash = util::hash_str("");
    constexpr u128 exit_section_hash = util::hash_str("..");
    constexpr u128 current_section_hash = util::hash_str(".");
    auto sections = std::views::split(path, '/') | std::views::transform([](auto &&section) {
                        return util::hash_str(std::string_view{section.begin(), section.end()});
                    })
                    | std::views::filter([](u128 section_hash) {
                          return section_hash != empty_section_hash && section_hash != current_section_hash;
                      });
    u128 result{0};
    std::optional<u128> last_section;
    for (u128 section : sections) {
        if (section == exit_section_hash) {
            if (!last_section.has_value()) {
                if consteval {
                } else {
                    panic("path attempts to exit root");
                }
            }
            result ^= last_section.value();
            last_section.reset();
        } else {
            result ^= section;
        }
        last_section = section;
    }
    return {result};
}

};  // namespace jungle::core

template<>
struct std::hash<jungle::core::AssetID> {
    std::size_t operator()(const jungle::core::AssetID &id) const {
        return static_cast<std::size_t>(id.m_id) ^ (static_cast<std::size_t>(id.m_id >> 64) << 1);
    }
};
