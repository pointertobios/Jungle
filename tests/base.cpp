// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <print>

#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/entity.h"
#include "jungle/meta.h"
#include "jungle/preusing.h"

using namespace jungle;
using namespace jungle::core::ecs;

enum class Color { Red, Green, Blue };

inline constexpr struct {
} annotation_test;

struct T {
    core::ecs::Entity e;
};

struct S {
    int x, y;
    Color color;
    core::ecs::Entity e;

    static constexpr usize cl_integer = 100;

    using member_type = int;

    struct {
    } anonymous_struct{};

    S(int x, int y, Color color, core::ecs::Entity e, T t, int priv)
            : x(x)
            , y(y)
            , color(color)
            , e(e)
            , t(t)
            , priv(priv) {}

private:
    friend struct core::ecs::Entity;

    void foo() {}

    T t;
    int priv;
};

using namespace literals;

class Transform : public Component<Transform> {
public:
    using Storage = SparseComponentStorage<Transform>;

private:
    float x, y, z;
};

class Node : public Component<Node> {
public:
    using Storage = SparseComponentStorage<Node>;
};

int main() {
    using core::ecs::Entity;
    Entity entity{0x123456789abcdef0};
    auto c = Color::Green;
    auto s = S{42, 24, c, entity, T{Entity{0x123456789abcdef1}}, 0};
    std::vector l{Color::Red, Color::Green, Color::Blue};
    std::println("{}", debug(entity));
    std::println("{}", debug(s));
    jungle::meta::has_template_annotation<^^jungle::core::ecs::Entity, ^^annotation_test>();
    std::println("{}", type_id::of<std::vector<int>>().name());

    auto mgr = Manager<Transform>{};
    Manager<> &base_mgr = mgr;
    auto &transform_mgr = base_mgr.as<Manager<Transform>>();
    (void)transform_mgr;
}
