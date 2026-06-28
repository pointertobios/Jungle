// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <print>

#include "jungle/async/future.h"
#include "jungle/async/invoke.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/entity.h"
#include "jungle/core/level.h"
#include "jungle/meta.h"
#include "jungle/preusing.h"
#include "jungle/tasks/runtime/runtime.h"

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
            : x{x}
            , y{y}
            , color{color}
            , e{e}
            , t{t}
            , priv{priv} {}

private:
    friend struct core::ecs::Entity;

    void foo() {}

    T t;
    int priv;
};

using namespace literals;

async::future<> async_test() {
    std::println("async function");
    auto x = co_await tasks::spawn([] -> async::future<int> {
        std::println("sub task");
        co_return 42;
    });
    std::println("sub task returns {}", x);
    auto f = co_await async::co_invoke([] -> async::future<float> { co_return 0.5; });
    std::println("{}", f);
    co_return;
}

int main() {
    using core::ecs::Entity;
    Entity entity{0x123456789abcdef0};
    auto c = Color::Green;
    auto s = S{42, 24, c, entity, T{Entity{0x123456789abcdef1}}, 0};
    std::vector l{Color::Red, Color::Green, Color::Blue};
    std::println("{}", debug(entity));
    std::println("{}", debug(s));
    jungle::meta::has_template_annotation<^^jungle::core::ecs::Entity, ^^annotation_test>();

    core::Level level;

    using jungle::tasks::runtime::runtime_config;
    {
        auto rt = runtime_config{}.multi_threaded().build();
        rt.block_on(async_test);
    }

    {
        auto rt = runtime_config{}.single_threaded().build();
        rt.spawn(async_test);
        rt.main_loop();
    }
}
