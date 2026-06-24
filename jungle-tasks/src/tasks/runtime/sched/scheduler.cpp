// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <ranges>

#include "jungle/tasks/runtime/sched/scheduler.h"

namespace jungle::tasks::runtime::sched {

void scheduler::attach_task(std::coroutine_handle<> root_coroutine) {
    m_queue.read()->push_back(task{to_task_id(root_coroutine), root_coroutine});
}

std::optional<task> scheduler::next_task() {
    for (auto &slot : m_reschedule_slots) {
        if (slot.available) {
            slot.available = false;
            auto res = *slot.m_task.get();
            slot.m_task.destroy();
            return res;
        }
    }

    task res;
    {
        auto g = m_queue.read();
        if (g->empty()) {
            return std::nullopt;
        }
        res = g->front();
        g->pop_front();
    }
    m_current_task = res.m_id;
    return res;
}

void scheduler::resched(task t) {
    for (auto &slot : m_reschedule_slots) {
        if (!slot.available) {
            slot.available = true;
            slot.m_task.emplace(try_move(t));
        }
    }
    m_queue.read()->push_front(try_move(*m_reschedule_slots[0].m_task.get()));
    template for (constexpr auto i : std::views::iota(static_cast<usize>(0), reschedule_slots_size - 1)) {
        m_reschedule_slots[i].m_task.destroy();
        m_reschedule_slots[i].m_task.emplace(try_move(*m_reschedule_slots[i + 1].m_task.get()));
    }
    auto &slot = m_reschedule_slots[reschedule_slots_size - 1];
    slot.m_task.destroy();
    slot.m_task.emplace(try_move(t));
}

void scheduler::suspend(task t) { m_suspended_tasks.insert(t.m_id, try_move(t)); }

};  // namespace jungle::tasks::runtime::sched
