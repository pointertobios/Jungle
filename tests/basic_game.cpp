// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/application.h"
#include "jungle/tasks/runtime/runtime.h"

using namespace jungle;

int main() {
    string_id services[] = {string_id{"Game"}, string_id{"Logging"}, string_id{"Asset"}};
    core::Application app{services};
    tasks::runtime::runtime().block_on([&] -> async::future<> { co_await app.run(); });
}
