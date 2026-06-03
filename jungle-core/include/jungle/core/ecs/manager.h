// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <functional>
#include <vector>

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/entity.h"

namespace jungle::core::ecs {

template<typename = void>
class Manager;

template<typename M>
concept ComponentManager = std::derived_from<M, Manager<>> && !std::is_same_v<M, Manager<>>;

template<>
class Manager<> {
public:
    template<ComponentManager M>
    constexpr bool is() const {
        return m_type == type_id::of<M>();
    }

    constexpr bool is(type_id manager_type) const { return m_type == manager_type; }

    template<ComponentManager M>
    constexpr M &as() pre(is<M>()) {
        return static_cast<M &>(*this);
    }

    template<ComponentManager M>
    constexpr const M &as() const pre(is<M>()) {
        return static_cast<const M &>(*this);
    }

    template<ComponentManager M>
    constexpr M *try_as() {
        return is<M>() ? &static_cast<M &>(*this) : nullptr;
    }

    template<ComponentManager M>
    constexpr const M *try_as() const {
        return is<M>() ? &static_cast<const M &>(*this) : nullptr;
    }

    virtual std::vector<std::reference_wrapper<Component<>>> vget_components() = 0;
    virtual std::vector<std::reference_wrapper<const Component<>>> vget_components() const = 0;

    virtual std::vector<std::reference_wrapper<Component<>>> vget_components(Entity entity) = 0;
    virtual std::vector<std::reference_wrapper<const Component<>>> vget_components(Entity entity) const = 0;

protected:
    constexpr Manager(type_id type)
            : m_type{type} {}

private:
    type_id m_type;
};

template<ComponentImpl C>
class Manager<C> final : public Manager<> {
public:
    constexpr Manager()
            : Manager<>{type_id::of<Manager<C>>()} {}

    auto get_components() { m_storage.get_components(); }
    auto get_components() const { m_storage.get_components(); }

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

};  // namespace jungle::core::ecs
