# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

set(IPREF "include/jungle/core") # Include prefix for headers

set(JUNGLE_HEADERS
    ${IPREF}/asset/asset.h
    ${IPREF}/component/transform.h
    ${IPREF}/ecs/component_storage.h
    ${IPREF}/ecs/component.h
    ${IPREF}/ecs/entity.h
    ${IPREF}/ecs/manager.h
    ${IPREF}/game.h
    ${IPREF}/level.h
)

set(JUNGLE_SOURCES
    src/asset/asset.cpp
    src/ecs/component.cpp
    src/ecs/entity.cpp
)

add_library(jungle-core ${JUNGLE_SOURCES})
target_link_libraries(jungle-core jungle::base)
target_include_directories(jungle-core PUBLIC include)
target_precompile_headers(jungle-core PUBLIC ${JUNGLE_HEADERS})
add_library(jungle::core ALIAS jungle-core)

# Unit Tests

set(JUNGLE_UNIT_TEST_SOURCES
    unit_tests/component.cpp
    unit_tests/manager.cpp
)

add_executable(jungle-core-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-core-unit-tests PUBLIC jungle::test jungle::core)
add_test(NAME jungle-core-unit-tests COMMAND jungle-core-unit-tests)
