// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/runtime/daemon.h"
#include "jungle/os/process.h"

namespace jungle::runtime {

daemon::daemon()
        : m_thread{[this](std::stop_token st) { daemon_thread(st); }} {}

daemon::daemon(ustr name)
        : m_thread{[this](std::stop_token st) { daemon_thread(st); }}
        , m_name{name} {}

void daemon::daemon_thread(std::stop_token &st) {
    os::thread_handle::from(m_thread).set_name(std::string{m_name.view()});
    if (!initialize()) {
        return;
    }
    while (run_once(st) && !st.stop_requested()) {}
    finalize();
}

};  // namespace jungle::runtime
