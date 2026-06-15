# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

set(IPREF "include/jungle") # Include prefix for headers

set(JUNGLE_HEADERS
    ${IPREF}/future.h
    ${IPREF}/sync/mpsc.h
    ${IPREF}/tasks/runtime/runtime.h
)

set(JUNGLE_SOURCES
    src/runtime/runtime.cpp
)

add_library(jungle-tasks ${JUNGLE_SOURCES})
target_link_libraries(jungle-tasks jungle::base)
target_include_directories(jungle-tasks PUBLIC include)
target_precompile_headers(jungle-tasks PUBLIC ${JUNGLE_HEADERS})
add_library(jungle::tasks ALIAS jungle-tasks)

# Unit Tests

set(JUNGLE_UNIT_TEST_SOURCES
    unit_tests/future.cpp
)

add_executable(jungle-tasks-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-tasks-unit-tests PUBLIC jungle::test jungle::tasks)
add_test(NAME jungle-tasks-unit-tests COMMAND jungle-tasks-unit-tests)

