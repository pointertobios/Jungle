// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/tasks/this_task.h"
#include "jungle/tasks/runtime/worker.h"

namespace jungle::tasks::this_task {

std::coroutine_handle<> current_coroutine() { return runtime::worker::current().current_coroutine(); }

};  // namespace jungle::tasks::this_task
