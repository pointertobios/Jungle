# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

set(IPREF "include/jungle") # Include prefix for headers

set(JUNGLE_HEADERS
    ${IPREF}/async/future.h
    ${IPREF}/async/invoke.h
    ${IPREF}/async/join_handle.h
    ${IPREF}/sync/mpsc.h
    ${IPREF}/sync/rwspinlock.h
    ${IPREF}/tasks/runtime/blocking_worker.h
    ${IPREF}/tasks/runtime/runtime.h
    ${IPREF}/tasks/runtime/sched/scheduler.h
    ${IPREF}/tasks/runtime/worker.h
    ${IPREF}/test/async_test.h
)

set(JUNGLE_SOURCES
    src/tasks/runtime/runtime.cpp
    src/tasks/runtime/sched/scheduler.cpp
    src/tasks/runtime/worker.cpp
    src/test/async_test.cpp
)

add_library(jungle-tasks ${JUNGLE_SOURCES})
target_link_libraries(jungle-tasks jungle::base)
target_include_directories(jungle-tasks PUBLIC include)
target_precompile_headers(jungle-tasks PUBLIC ${JUNGLE_HEADERS})
add_library(jungle::tasks ALIAS jungle-tasks)

add_library(jungle-test-async src/test/async_test.cpp)
target_precompile_headers(jungle-test-async PUBLIC include/jungle/test/async_test.h)
target_link_libraries(jungle-test-async PUBLIC jungle::tasks jungle::test)
add_library(jungle::test-async ALIAS jungle-test-async)

# Unit Tests

set(JUNGLE_UNIT_TEST_SOURCES
    unit_tests/future.cpp
    unit_tests/rwspinlock.cpp
)

add_executable(jungle-tasks-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-tasks-unit-tests PUBLIC jungle::test-async)
add_test(NAME jungle-tasks-unit-tests COMMAND jungle-tasks-unit-tests)
