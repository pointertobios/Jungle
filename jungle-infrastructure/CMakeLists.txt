# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

include(${PROJECT_SOURCE_DIR}/cmake/GeneratePch.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/CollectFiles.cmake)

jungle_generate_pch(PRECOMPILE_HEADERS
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch.h
)

jungle_collect_files(JUNGLE_SOURCES_COMMON
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    PATTERNS "*.cpp"
    EXCLUDE
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

set(JUNGLE_CONFIG_EMPTY_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/jungle-config-empty.cpp)
if(NOT EXISTS "${JUNGLE_CONFIG_EMPTY_SOURCE}")
    file(WRITE "${JUNGLE_CONFIG_EMPTY_SOURCE}" "")
endif()

add_library(jungle-config STATIC ${JUNGLE_CONFIG_EMPTY_SOURCE})
target_compile_options(jungle-config PUBLIC ${JUNGLE_OPTIONS})
target_compile_definitions(jungle-config PUBLIC ${JUNGLE_DEFINES})
add_library(jungle::config ALIAS jungle-config)

add_library(jungle-infrastructure ${JUNGLE_SOURCES})
target_precompile_headers(jungle-infrastructure PUBLIC ${PRECOMPILE_HEADERS})
target_link_libraries(jungle-infrastructure PUBLIC ${JUNGLE_DEPLIBS} jungle::config)
target_include_directories(jungle-infrastructure PUBLIC include)
add_library(jungle::infrastructure ALIAS jungle-infrastructure)
