# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

add_executable(base base.cpp)
target_link_libraries(base jungle::core jungle::tasks)

add_executable(basic_game basic_game.cpp)
target_link_libraries(basic_game jungle::core)
