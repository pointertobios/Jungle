// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/tasks/runtime/blocking_worker.h"

#include "jungle/tasks/runtime/worker.h"

namespace jungle::tasks::runtime {

bool blocking_worker::run_once(std::stop_token &st) {
    while (!m_acceptible_tx.send(m_wid)) {}

    std::optional<task_item> task = m_task_rx.recv();
    if (!task.has_value() && !st.stop_requested()) {
        wait_for_awake();
    }

    if (task) {
        m_this_task = task->m_task_block->to_task_id();
        task->m_root_coroutine.resume();
    }

    return task || !st.stop_requested();
}

};  // namespace jungle::tasks::runtime
