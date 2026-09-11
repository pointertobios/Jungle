// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/container/mpsc.h"

namespace jungle::tasks::runtime {

class runtime;
struct task_block_base;

struct task_item {
    std::coroutine_handle<> m_root_coroutine;
    task_block_base *m_task_block;
};

using task_sender = container::mpsc<task_item>::sender;
using task_receiver = container::mpsc<task_item>::receiver;

class placement_executing_guard {
public:
    placement_executing_guard(bool &flag)
            : m_flag{flag} {
        m_flag = true;
    }

    ~placement_executing_guard() { m_flag = false; }

private:
    bool &m_flag;
};

};  // namespace jungle::tasks::runtime
