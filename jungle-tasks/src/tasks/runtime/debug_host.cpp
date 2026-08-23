// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#ifdef JUNGLE_DEBUG_ENABLED

#    include "jungle/tasks/runtime/debug_host.h"

#    include "jungle/tasks/runtime/scheduler.h"

namespace jungle::tasks::runtime {

debug_host::debug_host()
        : daemon{"jg::dbghost"}
        , m_is_client{false}
        , m_context_shm{
              os::shared_memory::create<debug_context>(ustr::format("{}.context", shared_memory_path()))} {
    if (!m_context_shm.has_value()) {
        panic("创建调试上下文失败");
    }

    auto pool0 = create_string_pool(0);
    m_string_pools_shm.push_back(std::move(pool0.value()));
}

debug_host::debug_host(client_mode_tag)
        : daemon{"jg::dbghost"}
        , m_is_client{true}
        , m_context_shm{os::shared_memory::attach(ustr::format("{}.context", shared_memory_path()))} {
    if (!m_context_shm.has_value()) {
        panic("创建调试上下文失败");
    }
}

debug_host::~debug_host() {}

std::unique_ptr<debug_host> debug_host::attach() { return std::make_unique<debug_host>(client_mode_tag{}); }

debug_context &debug_host::context() { return *static_cast<debug_context *>(m_context_shm->get()); }

void debug_host::trace_coroutine_start(usize, task_id, std::source_location) {}

void debug_host::trace_coroutine_suspend(usize, task_id) {}

void debug_host::trace_coroutine_awake(usize, task_id) {}

void debug_host::trace_coroutine_end(usize, task_id) {}

void debug_host::trace_section_start(std::source_location) {}

void debug_host::trace_section_end() {}

bool debug_host::initialize() { return true; }

bool debug_host::run_once(std::stop_token &) { return false; }

void debug_host::finalize() {}

const ustr &debug_host::shared_memory_path() {
    static auto p = ustr::format("/jungle.debug-host-{:016x}", build_id());
    return p;
}

};  // namespace jungle::tasks::runtime

#endif
