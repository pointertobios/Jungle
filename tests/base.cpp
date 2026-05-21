#include <print>

#include "jungle/core/ecs/entity.h"
#include "jungle/preusing.h"

enum class Color { Red, Green, Blue };

struct S {
    int x, y;
    Color color;
    jungle::core::ecs::Entity e;

    static constexpr usize cl_integer = 100;

    using member_type = int;

private:
    friend struct jungle::core::ecs::Entity;

    void foo() {}
};

using namespace jungle::literals;

int main() {
    using jungle::core::ecs::Entity;
    Entity entity{0x123456789abcdef0};
    auto c = Color::Green;
    auto s = S{42, 24, c, entity};
    std::vector l{Color::Red, Color::Green, Color::Blue};
    std::println("{}", jungle::debug_of(entity));
    std::println("{}", jungle::debug_of(s));
}
