// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/core/service/service.h"

namespace jungle::core {

class Application {
public:
    explicit Application(std::span<string_id> using_services);

    Application(const Application &) = delete;
    Application &operator=(const Application&) = delete;

    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    async::future<> run();

private:
    hash_map<type_id, std::unique_ptr<service::Service>> m_service_table{};

    inline static Application *s_application{nullptr};
};

};  // namespace jungle::core
