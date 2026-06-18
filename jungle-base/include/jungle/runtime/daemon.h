// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <stop_token>
#include <thread>

#include "jungle/types/uchar.h"

namespace jungle::runtime {

class daemon {
public:
    daemon();
    daemon(ustr name);

protected:
    virtual bool initialize() = 0;
    virtual bool run_once(std::stop_token &) = 0;
    virtual void finalize() = 0;

private:
    void daemon_thread(std::stop_token &st);

    std::jthread m_thread;

    ustr m_name{"jg-daemon"};
};

};  // namespace jungle::runtime
