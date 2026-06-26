// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <coroutine>
#include <optional>

#include "jungle/os/process.h"
#include "jungle/tasks/runtime/worker.h"

namespace jungle::tasks::runtime {

bool worker::fetch_task() {
    std::optional<std::coroutine_handle<>> coroutine;
    usize c = 0;
    while ((coroutine = m_task_rx.recv())) {
        m_scheduler.attach_task(*coroutine);
        c++;
    }
    return c != 0;
}

void worker::set_next_resume(std::coroutine_handle<> coroutine) { m_next_resume = coroutine; }

void worker::set_suspend_now() { m_suspend_now = true; }

bool worker::initialize() {
    tls_this_worker = this;

    os::thread_handle::this_thread().set_affinity(os::cpu_set().with(m_wid));
    while (!m_acceptible_tx.send(m_wid)) {}
    return true;
}

bool worker::run_once(std::stop_token &st) {
    wait_for_awake();
    while (!m_acceptible_tx.send(m_wid)) {}
    bool fetched_new_task = fetch_task();

    std::optional<task> t{std::nullopt};
    while ((t = m_scheduler.next_task())) {
        m_this_task = t->m_id;
        while (!m_suspend_now) {
            t->m_resume_handle.resume();
        }
        if (m_next_resume) {
            t->m_resume_handle = m_next_resume;
            if (m_suspend_now) {
                m_scheduler.suspend(*t);
                m_suspend_now = false;
            } else {
                m_scheduler.resched(*t);
            }
        }
    }

    return t || fetched_new_task || !st.stop_requested();
}

void worker::finalize() {}

void awake_token::suspend() {
    m_worker->set_suspend_now();
    m_worker->set_next_resume(m_task);
}

void awake_token::awake() {
    m_worker->get_scheduler().awake(m_task);
    m_worker->awake();
}

};  // namespace jungle::tasks::runtime
