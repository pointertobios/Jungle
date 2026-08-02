// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <coroutine>
#include <deque>
#include <functional>
#include <optional>
#include <queue>

#include "jungle/container/hash_map.h"
#include "jungle/container/mpsc.h"
#include "jungle/sync/rwspinlock.h"
#include "jungle/types/raw_storage.h"

namespace jungle::tasks::runtime {

using task_id = std::coroutine_handle<>;

inline task_id to_task_id(std::coroutine_handle<> root_coroutine) { return root_coroutine; }

struct task {
    task_id m_id;
    std::coroutine_handle<> m_resume_handle;
};

};  // namespace jungle::tasks::runtime

namespace jungle::tasks::runtime::sched {

class scheduler final {
    static constexpr usize reschedule_slots_size = 16;

    struct reschedule_slot {
        raw_storage<task> m_task{};
        bool available{false};
    };

public:
    void attach_task(std::coroutine_handle<> root_coroutine);
    std::optional<task> next_task();
    void resched(task t);
    void suspend(task t) pre(t.m_id == m_current_task);
    void awake(task_id id);

    bool has_suspended() const;

private:
    void awake_impl(task_id id);

    task_id m_current_task;

    std::array<reschedule_slot, reschedule_slots_size> m_reschedule_slots;
    sync::rwspinlock<std::deque<task>, true> m_queue;
    hash_map<task_id, task> m_suspended_tasks;
    hash_set<task_id> m_preawake;

    std::tuple<container::mpsc<task_id>::sender, container::mpsc<task_id>::receiver> m_pending_awake{container::mpsc<task_id>::queue()};
};

};  // namespace jungle::tasks::runtime::sched