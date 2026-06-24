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

class worker final : public jungle::runtime::daemon {
public:
    worker(usize wid, task_receiver task_rx)
            : daemon{ustr::format("jg::w{}", wid)}
            , m_wid{wid}
            , m_task_rx{std::move(task_rx)} {}

    static bool exists() { return tls_this_worker; }
    static worker &current() { return *tls_this_worker; }

    bool fetch_task();

    task_id this_task() const;
    void set_next_resume(std::coroutine_handle<> coroutine);
    void set_suspend_now();

private:
    bool initialize() override;
    bool run_once(std::stop_token &st) override;
    void finalize() override;

    usize m_wid;
    task_receiver m_task_rx;

    sched::scheduler m_scheduler;
    task_id m_this_task;
    std::coroutine_handle<> m_next_resume;
    bool m_suspend_now;

    inline thread_local static worker *tls_this_worker{nullptr};
};

};  // namespace jungle::tasks::runtime
