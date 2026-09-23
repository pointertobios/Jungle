# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

include(${PROJECT_SOURCE_DIR}/cmake/GeneratePch.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/CollectFiles.cmake)

jungle_generate_pch(PRECOMPILE_HEADERS
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
    EXCLUDE
        jungle/allocator/extension
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch.h
)

jungle_generate_pch(PRECOMPILE_HEADERS_EXT
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/jungle/allocator/extension
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pch_ext.h
)

jungle_collect_files(JUNGLE_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    PATTERNS "*.cpp"
    EXCLUDE
        extension
)

add_library(jungle-allocator SHARED ${JUNGLE_SOURCES})
target_include_directories(jungle-allocator PUBLIC include)
target_link_libraries(jungle-allocator PUBLIC jungle::config)
target_precompile_headers(jungle-allocator PUBLIC ${PRECOMPILE_HEADERS})
add_library(jungle::allocator ALIAS jungle-allocator)

jungle_collect_files(JUNGLE_EXT_SOURCES
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/extension
    PATTERNS "*.cpp"
)

add_library(jungle-allocator-ext STATIC ${JUNGLE_EXT_SOURCES})
target_link_libraries(jungle-allocator-ext PRIVATE jungle::allocator)
target_precompile_headers(jungle-allocator-ext PUBLIC ${PRECOMPILE_HEADERS_EXT})
add_library(jungle::allocator-ext ALIAS jungle-allocator-ext)
