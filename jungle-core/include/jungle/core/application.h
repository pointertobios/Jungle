// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/assert.h"
#include "jungle/core/service/service.h"

namespace jungle::core {

class Application {
public:
    explicit Application(std::span<string_id> using_services);

    static Application &current();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    async::future<> run();

    template<service::ServiceImpl S>
    bool has_service() const {
        return m_service_table.get(type_id::of<S>());
    }

    template<service::ServiceImpl S>
    S &get_service() {
        JUNGLE_ASSERT(has_service<S>());
        return *m_service_table.get(type_id::of<S>());
    }

private:
    hash_map<type_id, std::unique_ptr<service::Service>> m_service_table{};

    inline static Application *s_application{nullptr};
};

};  // namespace jungle::core
