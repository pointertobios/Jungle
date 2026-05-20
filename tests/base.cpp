#include <print>

#include "jungle/core/ecs/entity.h"

int main() {
    using jungle::core::ecs::Entity;
    Entity entity{0x123456789abcdef0};
    std::println("{}", jungle::debug_of(entity));
}
