// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/application.h"

#include "jungle/assert.h"
#include "jungle/core/service/service.h"

namespace jungle::core {

Application &Application::current() {
    JUNGLE_ASSERT(s_application);
    return *s_application;
}

Application::Application(std::span<string_id> using_services) {
    JUNGLE_ASSERT(!s_application, "仅能构造一个 Application 实例");
    s_application = this;

    for (auto sid : using_services) {
        auto ctor = service::Service::get_service_creator(sid);
        auto [tid, uptr] = ctor();
        auto res = m_service_table.insert(tid, try_move(uptr));
        JUNGLE_ASSERT(res, "重复的服务 '{}'", uptr->name());
    }
}

async::future<> Application::run() {
    for (auto &service : m_service_table) {
        service.value()->start();
    }
    for (auto &service : m_service_table) {
        co_await service.value()->join();
    }
}

};  // namespace jungle::core
