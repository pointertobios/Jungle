// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "jungle/runtime/daemon.h"

namespace jungle::tasks::runtime {

class worker final : public jungle::runtime::daemon {
public:
    worker(usize wid)
            : daemon{ustr::format("jg-w{}", wid)} {}

private:
};

};  // namespace jungle::tasks::runtime
