// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#ifdef JUNGLE_DEBUG_ENABLED

#    include "jungle/tasks/runtime/debug_host.h"

#    include <atomic>
#    include <optional>
#    include <ranges>
#    include <string_view>

#    include "jungle/build_id.h"
#    include "jungle/constants.h"
#    include "jungle/os/shared_memory.h"
#    include "jungle/preusing.h"

namespace jungle::tasks::runtime {

shared_string_ref::shared_string_ref(usize length, u16 pool_index, u8 block_index)
        : m_meta{length, false}
        , m_pool{pool_index, block_index} {}

shared_string_ref::shared_string_ref(std::string_view short_string)
        : m_meta{short_string.size(), true} {
    std::ranges::copy(short_string, m_short_string);
}

class shared_string_pool {
    static constexpr usize block_size = cacheline_size / 2;

    static constexpr usize max_level = block_size - sizeof(u16);

    struct alignas(block_size) block {
        u8 block_count;
        u8 next_block;
    };

    struct alignas(block_size) record {
        u16 id;
        std::atomic<u8> levels[max_level];
    };

    static constexpr usize block_start_index = sizeof(record) / block_size;

    static constexpr usize block_max_count = (page_size - sizeof(os::shm_header)) / block_size;

public:
    shared_string_pool(u16 id) {
        m_record.id = id;
        std::ranges::fill(m_record.levels, 0);

        u8 level{max_level};
        u8 i{1};  // 从 1 开始是因为 record 为 1 x block_size
        while (i < block_max_count) {
            level = std::min(max_level, block_max_count - i);
            give_back(i, level);
            i += level;
        }
    }

    std::optional<shared_string_ref> try_create(const ustr &str) {
        auto view = str.view();
        usize size = view.size();
        usize block_count = (size + block_size - 1) / block_size;
        contract_assert(block_count <= max_level);

        {
            std::atomic<u8> &level = m_record.levels[block_count - 1];
            u8 using_block = level.load(morder::relaxed);
            if (using_block != 0) {
                store_string(using_block, view);
                level.store(m_storage[using_block].next_block, morder::release);
                return shared_string_ref{size, m_record.id, using_block};
            }
        }

        for (usize trying_count = block_count + 1; trying_count <= max_level; trying_count++) {
            std::atomic<u8> &level = m_record.levels[trying_count - 1];
            u8 using_block = level.load(morder::relaxed);
            if (using_block != 0) {
                store_string(using_block, view);
                level.store(m_storage[using_block].next_block, morder::release);
                if (trying_count > block_count) {
                    give_back(
                        static_cast<u8>(using_block + block_count),
                        static_cast<u8>(trying_count - block_count));
                }
                return shared_string_ref{size, m_record.id, using_block};
            }
        }

        return std::nullopt;
    }

    void destroy(shared_string_ref &&strr) pre(!strr.is_sso()) {
        usize block_count = (strr.m_meta.length + block_size - 1) / block_size;
        contract_assert(block_count > 0 && block_count <= max_level);
        give_back(strr.m_pool.block_index, static_cast<u8>(block_count));
    }

    void destroy(std::optional<shared_string_ref> &&strr) {
        if (strr.has_value()) {
            destroy(std::move(strr.value()));
            strr = std::nullopt;
        }
    }

    void merge_fragments() {
        u8 occ[block_max_count]{};  // 空闲标记表：1 = 空闲，0 = 已占用 / record

        for (usize l = 0; l < max_level; ++l) {
            u8 b = m_record.levels[l].load(morder::relaxed);
            m_record.levels[l].store(0, morder::relaxed);
            while (b != 0) {
                u8 n = m_storage[b].block_count;
                contract_assert(b >= 1 && static_cast<usize>(b + n) <= block_max_count && n == l + 1);
                for (u8 k = 0; k < n; ++k) {
                    occ[b + k] = 1;  // 空闲块
                }
                b = m_storage[b].next_block;
            }
        }

        usize i = 1;
        while (i < block_max_count) {
            if (occ[i] == 0) {
                ++i;
                continue;
            }
            usize start = i;
            while (i < block_max_count && occ[i] == 1) {
                ++i;
            }
            usize len = i - start;
            while (len > 0) {
                usize k = std::min(len, static_cast<usize>(max_level));
                give_back(static_cast<u8>(start), static_cast<u8>(k));
                start += k;
                len -= k;
            }
        }
    }

    std::string_view at_block(u8 index, usize length) const {
        auto addr = reinterpret_cast<const char *>(&m_storage[index]);
        return {addr, length};
    }

private:
    void give_back(u8 index, u8 block_count) {
        block &b = m_storage[index];
        auto &level = m_record.levels[block_count - 1];
        b.block_count = block_count;
        b.next_block = level.load(morder::relaxed);
        level.store(index, morder::relaxed);
    }

    void store_string(u8 index, std::string_view str) {
        str.copy(reinterpret_cast<char *>(&m_storage[index]), str.size());
    }

    union {
        record m_record;
        block m_storage[block_max_count];
    };
};

ustr shared_string_ref::fetch(debug_host &host) const {
    auto &pool = host.string_pool(m_pool.pool_index);
    return {pool.at_block(m_pool.block_index, m_meta.length)};
}

shared_string_pool &debug_host::string_pool(usize n) {
    usize pool_count = context().m_string_pool_count.load(morder::acquire);
    contract_assert(n < pool_count);

    if (m_is_client) {
        for (usize i = m_string_pools_shm.size(); i < pool_count; i++) {
            auto pool = os::shared_memory::attach(ustr::format("{}.string-pool-{}", shared_memory_path(), i));
            m_string_pools_shm.push_back(std::move(pool.value()));
        }
    }

    return *static_cast<shared_string_pool *>(m_string_pools_shm[n].get());
}

shared_string_ref debug_host::create_string(const ustr &str) {
    if (auto view = str.view(); view.size() <= shared_string_ref::sso_size) {
        return {view};
    }

    u16 pool_count = context().m_string_pool_count.load(morder::relaxed);

    for (usize i = 0; i < pool_count; i++) {
        if (auto r = string_pool(i).try_create(str); r.has_value()) {
            return std::move(r.value());
        }
    }

    for (usize i = 0; i < pool_count; i++) {
        auto &pool = string_pool(i);
        pool.merge_fragments();
        if (auto r = pool.try_create(str); r.has_value()) {
            return std::move(r.value());
        }
    }

    usize new_index = pool_count;
    auto pool = create_string_pool(new_index);
    m_string_pools_shm.push_back(std::move(pool.value()));
    context().m_string_pool_count.fetch_add(1, morder::release);

    auto r = string_pool(new_index).try_create(str);
    contract_assert(r.has_value());
    return std::move(r.value());
}

void debug_host::destroy_string(shared_string_ref &&str) {
    if (!str.is_sso()) {
        string_pool(str.m_pool.pool_index).destroy(std::move(str));
    }
}

std::optional<os::shared_memory> debug_host::create_string_pool(u16 id) {
    return os::shared_memory::create<shared_string_pool>(
        ustr::format("{}.string-pool-{}", shared_memory_path(), id), id);
}

};  // namespace jungle::tasks::runtime

#endif
