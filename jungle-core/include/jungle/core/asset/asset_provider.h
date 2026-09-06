// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <variant>

#include "jungle/async/future.h"
#include "jungle/core/asset/asset_id.h"
#include "jungle/core/asset/serde/serde_jaml.h"
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
concept AssetProviderImpl = requires(Provider provider, AssetID id) {
    typename Provider::DeserializeSource;
    typename Provider::SerializeTarget;
    serde::DeserializeSourceImpl<typename Provider::DeserializeSource>;
    serde::SerializeTargetImpl<typename Provider::SerializeTarget>;
    {
        provider.read_asset(id)
    } -> std::same_as<async::future<std::expected<typename Provider::DeserializeSource, AssetLoadFailed>>>;
    {
        provider.write_asset(id, std::declval<typename Provider::SerializeTarget &&>())
    } -> std::same_as<async::future<std::expected<void, AssetSaveFailed>>>;
};

template<typename Provider>
class AssetProviderBase {
public:
    template<typename T>
    async::future<std::expected<T, AssetLoadFailed>> load(AssetID id) {
        auto source = co_await self().read_asset(id);
        if (!source.has_value()) {
            co_return std::unexpected{std::move(source.error())};
        }

        auto res = serde::deserialize(source.value());
        if (!res.has_value()) {
            co_return std::unexpected{
                AssetLoadFailed{[&e = res.error()] { return ustr::format("deserialize 失败: {}", e); }}};
        }

        co_return std::move(res.value());
    }

    template<typename T>
    async::future<std::expected<void, AssetSaveFailed>> save(AssetID id, const T &value) {
        auto target = serde::serialize(value);
        auto res = co_await self().write_asset(id, std::move(target));
        if (!res.has_value()) {
            co_return std::unexpected{std::move(res.error())};
        }
        co_return {};
    }

private:
    Provider &self() { return static_cast<Provider &>(*this); }
};

class AssetProvider;

namespace providers {

// class JamlFileProvider : public AssetProviderBase<JamlFileProvider> {
// public:
//     using DeserializeSource = JamlSource;
//     using SerializeTarget = JamlTarget;

//     async::future<std::expected<JamlSource, AssetLoadFailed>> read_asset(AssetID id);
//     async::future<std::expected<void, AssetSaveFailed>> write_asset(AssetID id, JamlTarget &&target);
// };

// static_assert(AssetProviderImpl<JamlFileProvider>);

class EmbeddedJamlProvider : public AssetProviderBase<EmbeddedJamlProvider> {
public:
    using DeserializeSource = JamlSource;
    using SerializeTarget = JamlTarget;

    async::future<std::expected<JamlSource, AssetLoadFailed>> read_asset(AssetID id);
    async::future<std::expected<void, AssetSaveFailed>> write_asset(AssetID id, JamlTarget &&target);
};

static_assert(AssetProviderImpl<EmbeddedJamlProvider>);

// class EmbededBinaryProvider : public AssetProviderBase<EmbededBinaryProvider> {
// public:
// };

// static_assert(AssetProviderImpl<EmbededBinaryProvider>);

};  // namespace providers

class AssetProvider {
public:
private:
    std::variant<providers::EmbeddedJamlProvider> m_provider;
};

};  // namespace jungle::core::asset
