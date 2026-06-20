// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/runtime/daemon.h"
#include "jungle/os/process.h"

namespace jungle::runtime {

daemon::daemon() {}

daemon::daemon(ustr name)
        : m_name{name} {}

void daemon::start() {
    m_thread = std::jthread{[this](std::stop_token st) { run(st); }};
}

void daemon::run(std::stop_token &st) {
    os::thread_handle::from(m_thread).set_name(std::string{m_name.view()});
    if (!initialize()) {
        return;
    }
    while (run_once(st)) {}
    finalize();
}

};  // namespace jungle::runtime
