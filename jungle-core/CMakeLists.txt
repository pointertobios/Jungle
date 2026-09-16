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

add_library(jungle-core ${JUNGLE_SOURCES})
target_link_libraries(jungle-core jungle::base jungle::tasks)
target_include_directories(jungle-core PUBLIC include)
target_precompile_headers(jungle-core PUBLIC ${PRECOMPILE_HEADERS})
add_library(jungle::core ALIAS jungle-core)

# Unit Tests

jungle_collect_files(JUNGLE_UNIT_TEST_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests
    PATTERNS "*.cpp"
)

add_executable(jungle-core-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-core-unit-tests PUBLIC jungle::test jungle::core)
add_test(NAME jungle-core-unit-tests COMMAND jungle-core-unit-tests)
