// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <coroutine>
#include <limits>
#include <semaphore>

#include "jungle/container/mpsc.h"
#include "jungle/runtime/daemon.h"
#include "jungle/tasks/runtime/sched/scheduler.h"
#include "jungle/types/int.h"

namespace jungle::tasks::runtime {

using task_sender = mpsc<std::coroutine_handle<>>::sender;
using task_receiver = mpsc<std::coroutine_handle<>>::receiver;

class runtime;

class worker final : public jungle::runtime::daemon {
    friend class awake_token;

public:
    worker(runtime *rt, usize wid, task_receiver &&task_rx, mpsc<usize>::sender &&acceptible_tx)
            : daemon{ustr::format("jg::w{}", wid)}
            , m_host_runtime{rt}
            , m_wid{wid}
            , m_task_rx{std::move(task_rx)}
            , m_acceptible_tx{std::move(acceptible_tx)} {}

    static bool exists() { return tls_this_worker; }
    static worker &current() { return *tls_this_worker; }

    bool operator==(const worker &rhs) const { return m_wid == rhs.m_wid; }

    usize id() const { m_wid; }

    runtime &host_runtime() const { return *m_host_runtime; }

    sched::scheduler &get_scheduler() { return m_scheduler; }

    bool fetch_task();

    std::coroutine_handle<> current_coroutine() const;
    void set_next_resume(std::coroutine_handle<> coroutine);
    void set_suspend_now();

private:
    bool initialize() override;
    bool run_once(std::stop_token &st) override;
    void finalize() override;

    runtime *const m_host_runtime;
    const usize m_wid;
    task_receiver m_task_rx;
    mpsc<usize>::sender m_acceptible_tx;

    sched::scheduler m_scheduler;
    /* 当前状态 */
    task_id m_this_task;
    std::coroutine_handle<> m_next_resume;
    bool m_suspend_now;

    inline thread_local static worker *tls_this_worker{nullptr};
};

class awake_token {
public:
    awake_token(std::coroutine_handle<> resume_coroutine) pre(worker::exists())
            : m_resume_coroutine{resume_coroutine} {}

    operator bool() const { return m_worker; }

    static awake_token none() { return awake_token{nullptr, task_id{}}; }

    void suspend() pre(*m_worker == worker::current());
    void awake();

private:
    awake_token(worker *w, task_id t)
            : m_worker{w}
            , m_task{t} {}

    worker *m_worker{&worker::current()};
    task_id m_task{m_worker->m_this_task};
    std::coroutine_handle<> m_resume_coroutine;
};

};  // namespace jungle::tasks::runtime
