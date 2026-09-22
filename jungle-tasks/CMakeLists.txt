# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

include(${PROJECT_SOURCE_DIR}/cmake/GeneratePch.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/CollectFiles.cmake)

jungle_generate_pch(PRECOMPILE_HEADERS
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
    EXCLUDE
        jungle/test
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch.h
)

jungle_generate_pch(PRECOMPILE_HEADERS_TEST
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/jungle/test
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch_test.h
)

jungle_collect_files(JUNGLE_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    PATTERNS "*.cpp"
    EXCLUDE
        test
)

add_library(jungle-tasks ${JUNGLE_SOURCES})
target_link_libraries(jungle-tasks jungle::base)
target_include_directories(jungle-tasks PUBLIC include)
target_precompile_headers(jungle-tasks PUBLIC ${PRECOMPILE_HEADERS})
add_library(jungle::tasks ALIAS jungle-tasks)

add_library(jungle-test src/test/test_main.cpp)
target_precompile_headers(jungle-test PUBLIC ${PRECOMPILE_HEADERS_TEST})
target_link_libraries(jungle-test PUBLIC jungle::tasks)
target_compile_definitions(jungle-test PUBLIC JUNGLE_TESTING)
add_library(jungle::test ALIAS jungle-test)

# Unit Tests

jungle_collect_files(JUNGLE_UNIT_TEST_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests
    PATTERNS "*.cpp"
)

add_executable(jungle-tasks-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-tasks-unit-tests PUBLIC jungle::test)
add_test(NAME jungle-tasks-unit-tests COMMAND jungle-tasks-unit-tests)
