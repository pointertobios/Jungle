// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

#include "jungle/assert.h"
#include "jungle/async/future.h"
#include "jungle/container/hash_map.h"
#include "jungle/types/string_id.h"
#include "jungle/util/type_mutate.h"

namespace jungle::core::service {

class Service;

template<typename S>
concept ServiceImpl = requires {
    std::derived_from<S, Service>;
    !std::is_same_v<S, Service>;
};

using ServiceCreator = std::tuple<type_id, std::unique_ptr<Service>> (*)();

class Service : public util::type_mutate<Service> {
public:
    template<typename S>
    static constexpr bool static_mutatable = ServiceImpl<S>;

    static ServiceCreator get_service_creator(string_id name);

    template<ServiceImpl S>
    static ServiceCreator register_creator() {
        auto ctor = +[] -> std::tuple<type_id, std::unique_ptr<Service>> {
            return {type_id::of<S>(), std::make_unique<S>()};
        };
        auto res = m_services_of_components.insert(string_id{std::meta::identifier_of(^^S)}, ctor);
        JUNGLE_ASSERT(res);
        return ctor;
    }

protected:
    constexpr Service(type_id type)
            : type_mutate<Service>{type} {}

    void start();

    virtual async::future<> run() = 0;

private:
    inline static hash_map<string_id, ServiceCreator> m_services_of_components{};
};

#define jungle_core_service_register(service_impl)                              \
    namespace __registration_of_##service_impl {                                \
        inline ::jungle::core::service::ServiceCreator creator =                \
            ::jungle::core::service::Service::register_creator<service_impl>(); \
    }

};  // namespace jungle::core::service
