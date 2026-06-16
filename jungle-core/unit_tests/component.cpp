// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/ecs/component.h"
#include "jungle/core/ecs/component_storage.h"
#include "jungle/core/ecs/entity.h"
#include "jungle/core/ecs/manager.h"
#include "jungle/test/test.h"

using namespace jungle;
using namespace jungle::core::ecs;

struct TestCompX : Component<TestCompX> {
    using Storage = DenseComponentStorage<TestCompX>;

    int value;

    TestCompX(Entity e, ComponentID id, int v)
            : Component<TestCompX>{e, id}
            , value{v} {}
};

struct TestCompY : Component<TestCompY> {
    using Storage = SparseComponentStorage<TestCompY>;

    float data;

    TestCompY(Entity e, ComponentID id, float d)
            : Component<TestCompY>{e, id}
            , data{d} {}
};

JUNGLE_SYNC_TEST(component_id_default_invalid) {
    ComponentID id;
    JUNGLE_SYNC_ASSERT(!id, "default constructed ComponentID should be invalid");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_construct_with_valid_u64) {
    ComponentID id{42};
    JUNGLE_SYNC_ASSERT(id, "ComponentID constructed with non-zero u64 should be valid");
    JUNGLE_SYNC_ASSERT(
        id.underlying() == 42, "underlying() should return the exact u64 used at construction");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_construct_with_zero_is_invalid) {
    ComponentID id{0};
    JUNGLE_SYNC_ASSERT(!id, "ComponentID{{0}} should be treated as invalid (INVALID sentinel)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_equality) {
    ComponentID a{1};
    ComponentID b{1};
    ComponentID c{2};

    JUNGLE_SYNC_ASSERT(a == b, "ComponentIDs with same underlying value should be equal");
    JUNGLE_SYNC_ASSERT(!(a == c), "ComponentIDs with different underlying values should not be equal");
    JUNGLE_SYNC_ASSERT(
        !(a == ComponentID{}), "valid ComponentID should not equal default (invalid) ComponentID");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_copy_and_move) {
    ComponentID original{99};
    ComponentID copy{original};
    JUNGLE_SYNC_ASSERT(copy == original, "copied ComponentID should equal the original");
    JUNGLE_SYNC_ASSERT(copy.underlying() == 99, "copied ComponentID should preserve the underlying value");

    ComponentID moved{std::move(copy)};
    JUNGLE_SYNC_ASSERT(moved == original, "moved ComponentID should equal the original");
    JUNGLE_SYNC_ASSERT(moved.underlying() == 99, "moved ComponentID should preserve the underlying value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_copy_assign) {
    ComponentID a{7};
    ComponentID b{3};
    b = a;
    JUNGLE_SYNC_ASSERT(b == a, "copy-assigned ComponentID should equal the source");
    JUNGLE_SYNC_ASSERT(
        b.underlying() == 7, "copy-assigned ComponentID should preserve the source underlying value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_move_assign) {
    ComponentID a{55};
    ComponentID b{1};
    b = std::move(a);
    JUNGLE_SYNC_ASSERT(
        b.underlying() == 55, "move-assigned ComponentID should take the source underlying value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_debug_output) {
    ComponentID id{1};
    auto s = id.debug();
    JUNGLE_SYNC_ASSERT(!s.is_empty(), "debug() should return a non-empty string for a valid id");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_id_debug_none) {
    ComponentID id;
    auto s = id.debug();
    JUNGLE_SYNC_ASSERT(!s.is_empty(), "debug() should return a non-empty string even for NONE id");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(entity_default_invalid) {
    Entity e;
    JUNGLE_SYNC_ASSERT(!e, "default constructed Entity should be invalid");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(entity_construct_with_valid_u64) {
    Entity e{100};
    JUNGLE_SYNC_ASSERT(e, "Entity constructed with non-zero u64 should be valid");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(entity_equality) {
    Entity a{10};
    Entity b{10};
    Entity c{20};

    JUNGLE_SYNC_ASSERT(a == b, "Entities with same underlying value should be equal");
    JUNGLE_SYNC_ASSERT(!(a == c), "Entities with different underlying values should not be equal");
    JUNGLE_SYNC_ASSERT(!(a == Entity{}), "valid Entity should not equal default (invalid) Entity");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(entity_copy_and_move) {
    Entity original{77};
    Entity copy{original};
    JUNGLE_SYNC_ASSERT(copy == original, "copied Entity should equal the original");

    Entity moved{std::move(copy)};
    JUNGLE_SYNC_ASSERT(moved == original, "moved Entity should equal the original");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_owner_entity) {
    Manager<TestCompX> mgr;
    Entity entity{42};
    ComponentID id{1};

    auto &comp = mgr.create(id, entity, id, 10);
    JUNGLE_SYNC_ASSERT(comp.owner_entity() == entity, "component should report correct owner entity");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_owner_entity_multiple) {
    Manager<TestCompX> mgr;
    Entity ea{1};
    Entity eb{2};

    mgr.create(ComponentID{1}, ea, ComponentID{1}, 10);
    mgr.create(ComponentID{2}, eb, ComponentID{2}, 20);

    JUNGLE_SYNC_ASSERT(mgr.get_component(ComponentID{1}).owner_entity() == ea, "component 1 belongs to ea");
    JUNGLE_SYNC_ASSERT(mgr.get_component(ComponentID{2}).owner_entity() == eb, "component 2 belongs to eb");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_type_mutate_is) {
    Manager<TestCompX> mgr;
    Entity e{1};
    ComponentID id{1};

    auto &comp = mgr.create(id, e, id, 42);
    Component<> &base = comp;

    JUNGLE_SYNC_ASSERT(base.is<TestCompX>(), "Component<>::is<TestCompX>() should return true");
    JUNGLE_SYNC_ASSERT(!base.is<TestCompY>(), "Component<>::is<TestCompY>() should return false");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_type_mutate_as) {
    Manager<TestCompX> mgr;
    Entity e{1};
    ComponentID id{1};

    auto &comp = mgr.create(id, e, id, 77);
    Component<> &base = comp;

    auto &as_x = base.as<TestCompX>();
    JUNGLE_SYNC_ASSERT(&as_x == &comp, "as<TestCompX>() should return the same instance");
    JUNGLE_SYNC_ASSERT(as_x.value == 77, "as<TestCompX>() should preserve data");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_type_mutate_try_as) {
    Manager<TestCompX> mgr;
    Entity e{1};
    ComponentID id{1};

    auto &comp = mgr.create(id, e, id, 33);
    Component<> &base = comp;

    auto *ptr = base.try_as<TestCompX>();
    JUNGLE_SYNC_ASSERT(ptr != nullptr, "try_as<TestCompX>() should return non-null pointer");
    JUNGLE_SYNC_ASSERT(ptr->value == 33, "try_as<TestCompX>() should return correct data");

    auto *null_ptr = base.try_as<TestCompY>();
    JUNGLE_SYNC_ASSERT(null_ptr == nullptr, "try_as<wrong type>() should return nullptr");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_type_mutate_const_try_as) {
    Manager<TestCompX> mgr;
    Entity e{1};
    ComponentID id{1};

    mgr.create(id, e, id, 55);
    const auto &comp = mgr.get_component(id);
    const Component<> &base = comp;

    const auto *ptr = base.try_as<TestCompX>();
    JUNGLE_SYNC_ASSERT(ptr != nullptr, "const try_as<correct type>() should return non-null pointer");
    JUNGLE_SYNC_ASSERT(ptr->value == 55, "const try_as should preserve data");

    const auto *null_ptr = base.try_as<TestCompY>();
    JUNGLE_SYNC_ASSERT(null_ptr == nullptr, "const try_as<wrong type>() should return nullptr");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_type_mutate_is_by_type_id) {
    Manager<TestCompX> mgr;
    Entity e{1};
    ComponentID id{1};

    auto &comp = mgr.create(id, e, id, 1);
    Component<> &base = comp;

    JUNGLE_SYNC_ASSERT(base.is(type_id::of<TestCompX>()), "is(type_id) should return true for correct type");
    JUNGLE_SYNC_ASSERT(!base.is(type_id::of<TestCompY>()), "is(type_id) should return false for wrong type");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(component_type_mutate_type_accessor) {
    Manager<TestCompX> mgr;
    Entity e{1};
    ComponentID id{1};

    auto &comp = mgr.create(id, e, id, 99);
    Component<> &base = comp;

    JUNGLE_SYNC_ASSERT(base.type() == type_id::of<TestCompX>(), "type() should return the correct type_id");
    JUNGLE_SYNC_SUCCESS();
}
