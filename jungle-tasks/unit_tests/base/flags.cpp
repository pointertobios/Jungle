// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/test/test.h"
#include "jungle/types/flags.h"

using jungle::flags;

enum class color {
    red,
    green,
    blue,
};

enum class permission {
    read = 10,
    write = 20,
    execute = 30,
};

JUNGLE_SYNC_TEST(default_state) {
    flags<color> f;

    JUNGLE_SYNC_ASSERT(f.empty(), "默认构造应不包含任何变体");
    JUNGLE_SYNC_ASSERT(!f.any(), "默认构造不应处于 any 状态");
    JUNGLE_SYNC_ASSERT(!f.is_all(), "默认构造不应包含全部变体");
    JUNGLE_SYNC_ASSERT(f.size() == 0, "默认构造的有效变体数量应为 0");
    JUNGLE_SYNC_ASSERT(!f.contains(color::red), "默认构造不应包含 red");
    JUNGLE_SYNC_ASSERT(!f, "默认构造在布尔上下文中应为假");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(construct_from_enumerators) {
    flags<color> single{color::green};
    flags<color> many{color::red, color::blue};

    JUNGLE_SYNC_ASSERT(single.contains(color::green), "单变体构造应包含给定变体");
    JUNGLE_SYNC_ASSERT(!single.contains(color::red), "单变体构造不应包含其他变体");
    JUNGLE_SYNC_ASSERT(single.size() == 1, "单变体构造的有效数量应为 1");
    JUNGLE_SYNC_ASSERT(many.contains(color::red) && many.contains(color::blue), "多变体构造应包含全部给定变体");
    JUNGLE_SYNC_ASSERT(!many.contains(color::green), "多变体构造不应包含未给出的变体");
    JUNGLE_SYNC_ASSERT(many.size() == 2, "多变体构造的有效数量应等于给定变体数");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(set_reset_flip_clear) {
    flags<color> f;

    f.set(color::red).set(color::blue);
    JUNGLE_SYNC_ASSERT(f.contains(color::red) && f.contains(color::blue), "set 应使对应变体有效");
    JUNGLE_SYNC_ASSERT(f[color::red], "operator[] 应对已设置变体返回真");

    f.reset(color::red);
    JUNGLE_SYNC_ASSERT(!f.contains(color::red), "reset 应使对应变体无效");
    JUNGLE_SYNC_ASSERT(f.contains(color::blue), "reset 不应影响其他变体");

    f.flip(color::green);
    f.flip(color::blue);
    JUNGLE_SYNC_ASSERT(f.contains(color::green), "flip 应使无效变体变为有效");
    JUNGLE_SYNC_ASSERT(!f.contains(color::blue), "flip 应使有效变体变为无效");

    f.clear();
    JUNGLE_SYNC_ASSERT(f.empty(), "clear 应清除全部变体");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(none_and_all) {
    auto none = flags<color>::none();
    auto all = flags<color>::all();

    JUNGLE_SYNC_ASSERT(none.empty(), "none 应不包含任何变体");
    JUNGLE_SYNC_ASSERT(all.is_all(), "all 应包含全部变体");
    JUNGLE_SYNC_ASSERT(all.size() == 3, "all 的有效数量应等于枚举变体数量");
    JUNGLE_SYNC_ASSERT(all.contains(color::red) && all.contains(color::green) && all.contains(color::blue),
                       "all 应包含每一个变体");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(query_subset_and_intersection) {
    flags<color> red_green{color::red, color::green};
    flags<color> green{color::green};
    flags<color> blue{color::blue};

    JUNGLE_SYNC_ASSERT(red_green.contains(green), "包含更多变体的 flags 应包含其子集合");
    JUNGLE_SYNC_ASSERT(!green.contains(red_green), "更小的集合不应包含更大的集合");
    JUNGLE_SYNC_ASSERT(red_green.contains(flags<color>{}), "任何 flags 都应包含空集合");
    JUNGLE_SYNC_ASSERT(red_green.intersects(green), "有共同变体时应相交");
    JUNGLE_SYNC_ASSERT(!red_green.intersects(blue), "无共同变体时不应相交");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(bitwise_and_comparison_operators) {
    flags<color> red{color::red};
    flags<color> green{color::green};
    flags<color> red_green{color::red, color::green};

    JUNGLE_SYNC_ASSERT((red | green) == red_green, "或运算应合并变体");
    JUNGLE_SYNC_ASSERT((red_green & green) == green, "与运算应保留共同变体");
    JUNGLE_SYNC_ASSERT((red | color::green) == red_green, "枚举变体应能与 flags 做或运算");
    JUNGLE_SYNC_ASSERT((color::red | green) == red_green, "枚举变体作为左操作数时应能与 flags 做或运算");

    auto combined = red;
    combined |= green;
    JUNGLE_SYNC_ASSERT(combined == red_green, "或赋值应原地合并变体");
    combined &= color::green;
    JUNGLE_SYNC_ASSERT(combined == green, "与赋值应原地保留共同变体");
    JUNGLE_SYNC_ASSERT(red != green, "不同变体集合应不相等");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(maps_enumerators_by_declaration_order) {
    flags<permission> f{permission::write};

    JUNGLE_SYNC_ASSERT(f.contains(permission::write), "应按枚举变体本身设置比特，而不是底层整数值");
    JUNGLE_SYNC_ASSERT(!f.contains(permission::read), "未设置的变体应保持无效");
    JUNGLE_SYNC_ASSERT(f.size() == 1, "无论底层取值如何，单个变体只占用一个比特");
    JUNGLE_SYNC_SUCCESS();
}
