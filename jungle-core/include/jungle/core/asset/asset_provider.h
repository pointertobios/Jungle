// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <variant>

#include "jungle/async/future.h"
#include "jungle/core/asset/asset_id.h"
#include "jungle/serde/deserialize.h"
#include "jungle/serde/serde.h"
#include "jungle/serde/serialize.h"

namespace jungle::core::asset {

class AssetLoadFailed {
public:
    explicit AssetLoadFailed(std::function<ustr()> what_provider)
            : m_what_provider{std::move(what_provider)} {}

    ustr what() const { return m_what_provider(); }

private:
    std::function<ustr()> m_what_provider;
};

class AssetSaveFailed {
public:
    explicit AssetSaveFailed(std::function<ustr()> what_provider)
            : m_what_provider{std::move(what_provider)} {}

    ustr what() const { return m_what_provider(); }

private:
    std::function<ustr()> m_what_provider;
};

template<typename Provider>
class AssetProviderBase {
public:
    template<typename T>
    async::future<std::expected<T, AssetLoadFailed>> load(AssetID id) {}

    template<typename T>
    async::future<std::expected<void, AssetSaveFailed>> save(AssetID id, const T &value) {}

protected:
};

};  // namespace jungle::core::asset
