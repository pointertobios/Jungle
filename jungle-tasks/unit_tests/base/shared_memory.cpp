// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/os/shared_memory.h"
#include "jungle/test/test.h"

#include <cstring>

using namespace jungle;

JUNGLE_SYNC_TEST(create_and_destroy) {
    auto shm = os::shared_memory::create(ustr{"/jungle_ut_create_destroy"}, 1024);
    JUNGLE_SYNC_ASSERT(shm.has_value(), "create 应返回有效 shared_memory");
    JUNGLE_SYNC_ASSERT(shm->size() == 1024, "size() 应返回请求的大小");
    JUNGLE_SYNC_ASSERT(shm->get() != nullptr, "get() 应返回非空指针");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(create_duplicate_fails) {
    auto shm1 = os::shared_memory::create(ustr{"/jungle_ut_dup"}, 512);
    JUNGLE_SYNC_ASSERT(shm1.has_value(), "首次 create 应成功");

    auto shm2 = os::shared_memory::create(ustr{"/jungle_ut_dup"}, 512);
    JUNGLE_SYNC_ASSERT(!shm2.has_value(), "同名 create 应返回 nullopt");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(attach_nonexistent_fails) {
    auto shm = os::shared_memory::attach(ustr{"/jungle_ut_nonexistent"});
    JUNGLE_SYNC_ASSERT(!shm.has_value(), "attach 不存在时应返回 nullopt");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(shared_data_visible) {
    auto creator = os::shared_memory::create(ustr{"/jungle_ut_shared"}, 128);
    JUNGLE_SYNC_ASSERT(creator.has_value(), "create 应成功");

    std::memset(creator->get(), 0xAB, creator->size());

    auto attacher = os::shared_memory::attach(ustr{"/jungle_ut_shared"});
    JUNGLE_SYNC_ASSERT(attacher.has_value(), "attach 应成功");
    JUNGLE_SYNC_ASSERT(attacher->size() == creator->size(), "attach 的 size() 应与 create 一致");
    JUNGLE_SYNC_ASSERT(attacher->get() != creator->get(), "attach 和 create 应映射到不同虚拟地址");

    auto *data = static_cast<unsigned char *>(attacher->get());
    bool all_ab = true;
    for (usize i = 0; i < attacher->size(); ++i) {
        if (data[i] != 0xAB) { all_ab = false; break; }
    }
    JUNGLE_SYNC_ASSERT(all_ab, "attach 方应能看到 create 方写入的数据");

    std::memset(attacher->get(), 0xCD, attacher->size());
    auto *creator_data = static_cast<unsigned char *>(creator->get());
    bool all_cd = true;
    for (usize i = 0; i < creator->size(); ++i) {
        if (creator_data[i] != 0xCD) { all_cd = false; break; }
    }
    JUNGLE_SYNC_ASSERT(all_cd, "create 方应能看到 attach 方写入的数据");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(recreate_after_all_destroyed) {
    {
        auto shm = os::shared_memory::create(ustr{"/jungle_ut_recreate"}, 256);
        JUNGLE_SYNC_ASSERT(shm.has_value(), "create 应成功");
    }
    auto shm2 = os::shared_memory::create(ustr{"/jungle_ut_recreate"}, 256);
    JUNGLE_SYNC_ASSERT(shm2.has_value(), "全部销毁后同名 create 应再次成功");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(move_semantics) {
    auto shm = os::shared_memory::create(ustr{"/jungle_ut_move"}, 64);
    JUNGLE_SYNC_ASSERT(shm.has_value(), "create 应成功");

    void *addr = shm->get();
    usize sz = shm->size();

    auto shm2 = std::move(shm);
    JUNGLE_SYNC_ASSERT(shm2->get() == addr, "移动后 get() 应返回相同地址");
    JUNGLE_SYNC_ASSERT(shm2->size() == sz, "移动后 size() 应相同");
    JUNGLE_SYNC_SUCCESS();
}
