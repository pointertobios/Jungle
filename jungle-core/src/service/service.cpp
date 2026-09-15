// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/service/service.h"

#include "jungle/assert.h"
#include "jungle/tasks/this_task.h"

namespace jungle::core::service {

ServiceCreator Service::get_service_creator(string_id name) {
    auto res = s_services_of_components.get(name);
    JUNGLE_ASSERT(res);
    return *res;
}

async::future<> Service::join() {
    co_await m_run_task;
}

void Service::start() {
    JUNGLE_ASSERT(m_run_task.is_empty());
    m_run_task = this_task::spawn([&] -> async::future<> { co_await run(); });
}

};  // namespace jungle::core::service
