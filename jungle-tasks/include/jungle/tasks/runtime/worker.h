// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <coroutine>
#include <limits>
#include <semaphore>

#include "jungle/async/control.h"
#include "jungle/container/mpsc.h"
#include "jungle/runtime/daemon.h"
#include "jungle/sync/spinlock.h"
#include "jungle/tasks/runtime/sched/scheduler.h"
#include "jungle/types/erased.h"
#include "jungle/types/int.h"

namespace jungle::tasks::runtime {

class runtime;
struct task_block_base;

struct task_item {
    std::coroutine_handle<> m_root_coroutine;
    task_block_base *m_task_block;
};

using task_sender = container::mpsc<task_item>::sender;
using task_receiver = container::mpsc<task_item>::receiver;

class worker final : public jungle::runtime::daemon {
    friend class awake_token;

public:
    worker(runtime *rt, usize wid, task_receiver &&task_rx, container::mpsc<usize>::sender &&acceptible_tx)
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

    void set_next_resume(std::coroutine_handle<> coroutine);
    void set_suspend_now();
    void set_yield_now();

private:
    bool initialize() override;
    bool run_once(std::stop_token &st) override;
    void finalize() override;

    runtime *const m_host_runtime;
    const usize m_wid;
    task_receiver m_task_rx;
    container::mpsc<usize>::sender m_acceptible_tx;

    sched::scheduler m_scheduler;
    /* 当前状态 */
    task_id m_this_task{};
    std::coroutine_handle<> m_next_resume{};
    bool m_suspend_now{false};
    bool m_yield_now{false};

    inline thread_local static worker *tls_this_worker{nullptr};
};

class awake_token {
public:
    awake_token() pre(worker::exists()) = default;

    ~awake_token() = default;

    awake_token(const awake_token &) = default;
    awake_token &operator=(const awake_token &) = default;

    awake_token(awake_token &&) = default;
    awake_token &operator=(awake_token &&) = default;

    static awake_token none() { return awake_token{nullptr, task_id{}}; }

    operator bool() const { return m_worker; }

    bool operator==(const awake_token &rhs) const { return m_worker == rhs.m_worker && m_task == rhs.m_task; }

    void suspend(std::coroutine_handle<> resume_coroutine) pre(*m_worker == worker::current()) {
        m_worker->set_suspend_now();
        m_worker->set_next_resume(resume_coroutine);
    }

    void awake() {
        m_worker->get_scheduler().awake(m_task);
        m_worker->awake();
    }

private:
    awake_token(worker *w, task_id t)
            : m_worker{w}
            , m_task{t} {}

    worker *m_worker{&worker::current()};
    task_id m_task{m_worker->m_this_task};
};

struct task_block_base {
    std::atomic<async::control::future_state> m_state;
    tasks::runtime::awake_token m_awake_token{tasks::runtime::awake_token::none()};

    std::binary_semaphore m_sync_awaiter{0};

    sync::spinlock<std::vector<task_block_base *>> m_subtasks{};

    erased m_bound_invocable{};

    task_id to_task_id() const { return reinterpret_cast<void *>(const_cast<task_block_base *>(this)); }

    static task_block_base *from_task_id(task_id id) { return reinterpret_cast<task_block_base *>(id); }
};

};  // namespace jungle::tasks::runtime
