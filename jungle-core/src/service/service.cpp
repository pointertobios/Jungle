// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/service/service.h"

#include "jungle/assert.h"
#include "jungle/tasks/this_task.h"

namespace jungle::core::service {

ServiceCreator Service::get_service_creator(string_id name) {
    auto res = m_services_of_components.get(name);
    JUNGLE_ASSERT(res);
    return *res;
}

void Service::start() {
    this_task::spawn([&] -> async::future<> { co_await run(); });
}

};  // namespace jungle::core::service
