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
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch_test.h
)

jungle_collect_files(JUNGLE_SOURCES_COMMON
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    PATTERNS "*.cpp"
    EXCLUDE
        test
        os/linux
        os/windows
)

if (LINUX)
    jungle_collect_files(JUNGLE_SOURCES_PLATFORM
        INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/os/linux
        PATTERNS "*.cpp"
    )
elseif (WIN32)
    jungle_collect_files(JUNGLE_SOURCES_PLATFORM
        INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/os/windows
        PATTERNS "*.cpp"
    )
endif()

set(JUNGLE_SOURCES ${JUNGLE_SOURCES_COMMON} ${JUNGLE_SOURCES_PLATFORM})

set(JUNGLE_DEPLIBS
    stdc++exp
)

if (LINUX)
    set(JUNGLE_DEPLIBS ${JUNGLE_DEPLIBS} ncurses)
elseif (WIN32)
    set(JUNGLE_DEPLIBS ${JUNGLE_DEPLIBS} atomic)
endif()

set(JUNGLE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -freflection
    -fno-rtti
    -fno-exceptions
    -fconcepts-diagnostics-depth=6
)

include(JungleBuildId)
jungle_generate_build_id(JUNGLE_BUILD_ID "${CMAKE_SOURCE_DIR}")

set(JUNGLE_DEFINES
    $<$<CONFIG:Debug>:JUNGLE_DEBUG_ENABLED>
    $<$<CONFIG:RelWithDebInfo>:JUNGLE_DEBUG_ENABLED>
    JUNGLE_BUILD_ID="${JUNGLE_BUILD_ID}"
)

add_library(jungle-base ${JUNGLE_SOURCES})
target_precompile_headers(jungle-base PUBLIC ${PRECOMPILE_HEADERS})
target_link_libraries(jungle-base PUBLIC ${JUNGLE_DEPLIBS})
target_include_directories(jungle-base PUBLIC include)
target_compile_options(jungle-base PUBLIC ${JUNGLE_OPTIONS})
target_compile_definitions(jungle-base PUBLIC ${JUNGLE_DEFINES})
add_library(jungle::base ALIAS jungle-base)

add_library(jungle-test src/test/test_main.cpp)
target_precompile_headers(jungle-test PUBLIC ${PRECOMPILE_HEADERS_TEST})
target_link_libraries(jungle-test PUBLIC jungle::base)
add_library(jungle::test ALIAS jungle-test)

# Unit Tests

jungle_collect_files(JUNGLE_UNIT_TEST_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests
    PATTERNS "*.cpp"
)

add_executable(jungle-base-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-base-unit-tests PUBLIC jungle::test)
add_test(NAME jungle-base-unit-tests COMMAND jungle-base-unit-tests)
