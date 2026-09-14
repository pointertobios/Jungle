// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/application.h"

#include "jungle/core/service/service.h"

namespace jungle::core {

Application::Application(std::span<string_id> using_services) {
    s_application = this;

    for (auto sid : using_services) {
        auto ctor = service::Service::get_service_creator(sid);
        auto [tid, uptr] = ctor();
        m_service_table.insert(tid, try_move(uptr));
    }
}

};  // namespace jungle::core
