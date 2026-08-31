// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/async/future.h"

#ifdef JUNGLE_DEBUG_ENABLED
#    include "jungle/tasks/runtime/debug_host.h"
#    include "jungle/tasks/runtime/worker.h"
#    include "jungle/tasks/this_task.h"
#endif

namespace jungle::async {

#ifdef JUNGLE_DEBUG_ENABLED
namespace detail {

void future_trace_start(std::source_location sl) {
    auto &dh = this_task::host_runtime().get_debug_host();
    dh.trace_coroutine_start(this_task::worker().id(), this_task::id(), sl);
}

void future_trace_end() {
    auto &dh = this_task::host_runtime().get_debug_host();
    dh.trace_coroutine_end(this_task::worker().id(), this_task::id());
}

};  // namespace detail
#endif

};
