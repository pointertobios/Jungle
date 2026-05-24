#pragma once

#include <array>

#include "jungle/preusing.h"

namespace jungle::core::asset {

class AssetID {
    friend struct std::hash<AssetID>;

    static constexpr std::array<u64, 2> NONE{0, 0};

public:
    constexpr AssetID() noexcept = default;
    constexpr AssetID(const AssetID &) noexcept = default;
    constexpr AssetID &operator=(const AssetID &) noexcept = default;
    constexpr AssetID(AssetID &&) noexcept = default;
    constexpr AssetID &operator=(AssetID &&) noexcept = default;

    constexpr operator bool() const noexcept { return m_id != NONE; }
    constexpr bool operator==(const AssetID &other) const noexcept {
        return m_id == other.m_id;
    }

    ustr debug() const;

private:
    std::array<u64, 2> m_id{0, 0};
};

};  // namespace jungle::core::asset

template<>
struct std::hash<jungle::core::asset::AssetID> {
    std::size_t operator()(const jungle::core::asset::AssetID &id) const noexcept {
        return id.m_id[0] ^ (id.m_id[1] << 1);
    }
};
