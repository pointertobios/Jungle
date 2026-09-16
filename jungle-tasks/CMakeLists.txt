# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

include(${PROJECT_SOURCE_DIR}/cmake/GeneratePch.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/CollectFiles.cmake)

jungle_generate_pch(PRECOMPILE_HEADERS
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch.h
)

jungle_collect_files(JUNGLE_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    PATTERNS "*.cpp"
)

add_library(jungle-tasks ${JUNGLE_SOURCES})
target_link_libraries(jungle-tasks jungle::base)
target_include_directories(jungle-tasks PUBLIC include)
target_precompile_headers(jungle-tasks PUBLIC ${PRECOMPILE_HEADERS})
add_library(jungle::tasks ALIAS jungle-tasks)

add_library(jungle-test-async OBJECT src/test/async_test.cpp)
target_precompile_headers(jungle-test-async PUBLIC ${PRECOMPILE_HEADERS})
target_link_libraries(jungle-test-async PUBLIC jungle::tasks jungle::test)
add_library(jungle::test-async ALIAS jungle-test-async)

# Unit Tests

jungle_collect_files(JUNGLE_UNIT_TEST_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests
    PATTERNS "*.cpp"
)

add_executable(jungle-tasks-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-tasks-unit-tests PUBLIC jungle::test-async)
add_test(NAME jungle-tasks-unit-tests COMMAND jungle-tasks-unit-tests)
