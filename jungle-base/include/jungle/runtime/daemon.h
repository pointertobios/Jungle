// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <semaphore>
#include <stop_token>
#include <thread>

#include "jungle/types/uchar.h"

namespace jungle::runtime {

class daemon {
public:
    daemon();
    daemon(ustr name);
    virtual ~daemon();

    void run(std::stop_token &st);

    void start();

    void awake();

    void join();

protected:
    std::jthread m_thread;

    void wait_for_awake();

    virtual bool initialize() = 0;
    virtual bool run_once(std::stop_token &) = 0;
    virtual void finalize() = 0;

private:
    ustr m_name{"jg::daemon"};

    bool m_joined{false};
    std::counting_semaphore<> m_idle_sem{0};
    std::binary_semaphore m_construct_sem{0};
};

};  // namespace jungle::runtime
