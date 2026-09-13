// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/asset/asset_provider.h"

#include <string_view>
#include <vector>

#include "jungle/assert.h"
#include "jungle/async/future.h"
#include "jungle/container/hash_map.h"
#include "jungle/core/asset/embedded_tree.h"
#include "jungle/sync/rwspinlock.h"
#include "jungle/util/murmur.h"

namespace jungle::core::asset::providers {

namespace embedded_jaml {

const EmbeddedAssetNode *g_embedded_asset_tree{nullptr};
sync::rwspinlock<hash_map<AssetID, std::span<const std::byte>>> g_embedded_asset_cache{};

};  // namespace embedded_jaml

bool EmbeddedJamlProvider::set_embedded_asset_tree(EmbeddedAssetNode *tree) {
    if (embedded_jaml::g_embedded_asset_tree != nullptr) {
        panic("资产树已被设置，不能重复设置");
    }

    embedded_jaml::g_embedded_asset_tree = tree;
    return true;
}

EmbeddedJamlProvider::EmbeddedJamlProvider() {
    // TODO: 日志 info[索引 EmbeddedJaml 资产数据]

    struct indexer {
        static async::future<> index(const EmbeddedAssetNode *list, u128 path_hash) {
            for (auto node = list; node; node = node->next) {
                JUNGLE_ASSERT(!node->children || !node->data.size());

                u128 current_full_hash = path_hash ^ util::hash_str(node->name);

                if (node->children) {
                    co_await index(node->children, current_full_hash);
                    co_return;
                }

                embedded_jaml::g_embedded_asset_cache.write()->emplace(
                    AssetID{current_full_hash}, node->data);
            }
        }
    };

    indexer::index(embedded_jaml::g_embedded_asset_tree, 0).placement_execute();
}

};  // namespace jungle::core::asset::providers
