// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>

#include "jungle/types/erased.h"
#include "jungle/types/int.h"
#include "jungle/types/uchar.h"

namespace jungle::os {

class shared_memory final {
public:
    /// 创建新的共享内存，若同名共享内存已存在则返回 std::nullopt
    static std::optional<shared_memory> create(ustr name, usize size);

    /// 持有一个已创建的共享内存，若不存在则返回 std::nullopt
    static std::optional<shared_memory> attach(ustr name);

    shared_memory(const shared_memory &) = delete;
    shared_memory &operator=(const shared_memory &) = delete;

    shared_memory(shared_memory &&) = default;
    shared_memory &operator=(shared_memory &&) = default;

    /// 持有者计数减一，归零时销毁共享内存
    ~shared_memory();

    /// 返回用户请求的共享内存大小（字节）
    usize size() const;

    /// 返回共享内存在本进程中的映射地址
    void *get() const;

private:
    shared_memory();

    shared_memory with_extra(erased extra) && {
        m_extra = std::move(extra);
        return std::move(*this);
    }

    erased m_extra;
};

};  // namespace jungle::os
