// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#ifdef JUNGLE_DEBUG_ENABLED

#    include <memory>
#    include <optional>
#    include <source_location>
#    include <vector>

#    include "jungle/os/shared_memory.h"
#    include "jungle/runtime/daemon.h"
#    include "jungle/tasks/runtime/scheduler.h"

namespace jungle::tasks::runtime {

class debug_host;

class shared_string_ref {
    friend class shared_string_pool;
    friend class debug_host;

public:
    static constexpr usize sso_size = 24;

    bool is_sso() const { return m_meta.is_short_string; }

    ustr fetch(debug_host &host) const;

private:
    shared_string_ref()
            : m_meta{0, true} {}

    shared_string_ref(usize length, u16 pool_index, u8 block_index);

    shared_string_ref(std::string_view short_string);

    struct _pool_ {
        u16 pool_index;
        u8 block_index;
    };

    const struct _meta_ {
        const usize length : 63;
        const bool is_short_string : 1;
    } m_meta;
    union {
        char m_short_string[sso_size];
        _pool_ m_pool;
    };
};

struct alignas(cacheline_size) debug_context {
    std::atomic<usize> m_string_pool_count{1};
};

class debug_host final : public jungle::runtime::daemon {
    friend class shared_string_ref;

public:
    debug_host();
    ~debug_host();

    static std::unique_ptr<debug_host> attach();

    struct client_mode_tag {};
    debug_host(client_mode_tag);

    void trace_coroutine_start(usize worker, task_id tid, std::source_location sl) pre(!m_is_client);
    void trace_coroutine_suspend(usize worker, task_id tid) pre(!m_is_client);
    void trace_coroutine_awake(usize worker, task_id tid) pre(!m_is_client);
    void trace_coroutine_end(usize worker, task_id tid) pre(!m_is_client);

    void trace_section_start(std::source_location sl) pre(!m_is_client);
    void trace_section_end() pre(!m_is_client);

private:
    bool initialize() override;
    bool run_once(std::stop_token &st) override;
    void finalize() override;

    bool m_is_client;

    std::optional<os::shared_memory> m_context_shm;
    debug_context &context();

    std::vector<os::shared_memory> m_string_pools_shm;
    class shared_string_pool &string_pool(usize n);
    shared_string_ref create_string(const ustr &str) pre(!m_is_client);
    void destroy_string(shared_string_ref &&str) pre(!m_is_client);
    static std::optional<os::shared_memory> create_string_pool(u16 id);

    static const ustr &shared_memory_path();
};

};  // namespace jungle::tasks::runtime

#endif
