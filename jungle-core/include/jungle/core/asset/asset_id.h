// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>

#include "jungle/preusing.h"

namespace jungle::core {

class AssetID {
    friend struct std::hash<AssetID>;

    static constexpr u128 NONE = 0;

public:
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

};  // namespace jungle::core

template<>
struct std::hash<jungle::core::AssetID> {
    std::size_t operator()(const jungle::core::AssetID &id) const {
        return static_cast<std::size_t>(id.m_id) ^ (static_cast<std::size_t>(id.m_id >> 64) << 1);
    }
};
