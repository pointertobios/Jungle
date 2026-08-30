// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <vector>

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/entity.h"
#include "jungle/util/type_mutate.h"

namespace jungle::core::ecs {

template<typename = void>
class Manager;

template<typename M>
concept ComponentManager = std::derived_from<M, Manager<>> && !std::is_same_v<M, Manager<>>;

using ManagerCreator = std::unique_ptr<Manager<>> (*)();

template<>
class Manager<> : public util::type_mutate<Manager<>> {
public:
    template<typename C>
    static constexpr bool static_mutatable = ComponentManager<C>;

    template<ComponentImpl C>
    static ManagerCreator get_manager_creator() {
        return m_creators_of_component.get(type_id::of<C>());
    }

    virtual std::vector<std::reference_wrapper<Component<>>> vget_components() = 0;
    virtual std::vector<std::reference_wrapper<const Component<>>> vget_components() const = 0;

    virtual std::vector<std::reference_wrapper<Component<>>> vget_components(Entity entity) = 0;
    virtual std::vector<std::reference_wrapper<const Component<>>> vget_components(Entity entity) const = 0;

protected:
    constexpr Manager(type_id type)
            : util::type_mutate<Manager<>>{type} {}

    static void reigster_manager_creator(type_id type, ManagerCreator creator) {
        m_creators_of_component.insert(type, creator);
    }

    inline static hash_map<type_id, ManagerCreator> m_creators_of_component{};
};

template<ComponentImpl C>
class Manager<C> final : public Manager<> {
public:
    static ManagerCreator register_creator() {
        auto crtor = +[] -> std::unique_ptr<Manager<>> { return std::make_unique<Manager>(); };
        Manager<>::reigster_manager_creator(type_id::of<C>(), crtor);
        return crtor;
    }

    constexpr Manager()
            : Manager<>{type_id::of<Manager<C>>()} {}

    template<typename... Args>
    C &create(ComponentID id, Args &&...args) {
        return m_storage.create(id, std::forward<Args>(args)...);
    }

    void destroy(ComponentID id) { m_storage.destroy(id); }

    C &get_component(ComponentID id) { return m_storage.get_component(id); }
    const C &get_component(ComponentID id) const { return m_storage.get_component(id); }

    auto get_components() { return m_storage.get_components(); }
    auto get_components() const { return m_storage.get_components(); }

    auto get_components(Entity entity) { return m_storage.get_components(entity); }
    auto get_components(Entity entity) const { return m_storage.get_components(entity); }

    std::vector<std::reference_wrapper<Component<>>> vget_components() override {
        std::vector<std::reference_wrapper<Component<>>> result;
        for (auto &c : get_components()) {
            result.push_back(std::ref(c));
        }
        return result;
    }

    std::vector<std::reference_wrapper<const Component<>>> vget_components() const override {
        std::vector<std::reference_wrapper<const Component<>>> result;
        for (const auto &c : get_components()) {
            result.push_back(std::cref(c));
        }
        return result;
    }

    std::vector<std::reference_wrapper<Component<>>> vget_components(Entity entity) override {
        std::vector<std::reference_wrapper<Component<>>> result;
        for (auto &c : get_components(entity)) {
            result.push_back(std::ref(c));
        }
        return result;
    }

    std::vector<std::reference_wrapper<const Component<>>> vget_components(Entity entity) const override {
        std::vector<std::reference_wrapper<const Component<>>> result;
        for (const auto &c : get_components(entity)) {
            result.push_back(std::cref(c));
        }
        return result;
    }

private:
    C::Storage m_storage;
};

#define jungle_core_ecs_register_component(comp_impl)                    \
    namespace __registration_of_##comp_impl {                            \
        inline ::jungle::core::ecs::ManagerCreator creator =             \
            ::jungle::core::ecs::Manager<comp_impl>::register_creator(); \
    }

};  // namespace jungle::core::ecs
