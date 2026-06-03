// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>

#include "jungle/preusing.h"

namespace jungle::core::asset {

class AssetID {
    friend struct std::hash<AssetID>;

    static constexpr std::array<u64, 2> NONE{0, 0};

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
    std::array<u64, 2> m_id{0, 0};
};

};  // namespace jungle::core::asset

template<>
struct std::hash<jungle::core::asset::AssetID> {
    std::size_t operator()(const jungle::core::asset::AssetID &id) const {
        return id.m_id[0] ^ (id.m_id[1] << 1);
    }
};
