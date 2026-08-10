# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

set(IPREF "include/jungle") # Include prefix for headers

set(JUNGLE_HEADERS
    ${IPREF}/algebra/matrix.h
    ${IPREF}/algebra/scalar.h
    ${IPREF}/container/hash_map.h
    ${IPREF}/container/mpsc.h
    ${IPREF}/debug.h
    ${IPREF}/meta.h
    ${IPREF}/os/process.h
    ${IPREF}/os/shared_memory.h
    ${IPREF}/os/terminal.h
    ${IPREF}/panic.h
    ${IPREF}/preusing.h
    ${IPREF}/runtime/daemon.h
    ${IPREF}/serde/serde.h
    ${IPREF}/serde/serialize.h
    ${IPREF}/serde/deserialize.h
    ${IPREF}/types/concepts.h
    ${IPREF}/types/int.h
    ${IPREF}/types/raw_storage.h
    ${IPREF}/types/types.h
    ${IPREF}/types/uchar.h
    ${IPREF}/util/murmur.h
    ${IPREF}/util/parse.h
    ${IPREF}/util/rng.h
    ${IPREF}/util/type_id.h
    ${IPREF}/util/type_mutate.h
)

set(JUNGLE_SOURCES
    src/os/process.cpp
    src/runtime/daemon.cpp
    src/panic.cpp
    src/types/uchar.cpp
    src/util/parse.cpp
)

set(JUNGLE_DEPLIBS
    stdc++exp
)

if (LINUX)
    set(JUNGLE_SOURCES ${JUNGLE_SOURCES}
        src/os/linux/process.cpp
        src/os/linux/shared_memory.cpp
        src/os/linux/terminal.cpp
    )

    set(JUNGLE_DEPLIBS ${JUNGLE_DEPLIBS} ncurses)
elseif (WIN32)
    set(JUNGLE_SOURCES ${JUNGLE_SOURCES}
        src/os/windows/process.cpp
        src/os/windows/shared_memory.cpp
        src/os/windows/terminal.cpp
    )

    set(JUNGLE_DEPLIBS ${JUNGLE_DEPLIBS} atomic)
endif()

set(JUNGLE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -freflection
    -fcontracts
    -fno-rtti
    -fno-exceptions
    -fconcepts-diagnostics-depth=6
    $<$<CONFIG:Debug>:-fcontract-evaluation-semantic=enforce>
    $<$<CONFIG:RelWithDebInfo>:-fcontract-evaluation-semantic=enforce>
    $<$<CONFIG:Release>:-fcontract-evaluation-semantic=quick_enforce>
)

set(JUNGLE_DEFINES
    $<$<CONFIG:Debug>:JUNGLE_DEBUG_ENABLED>
    $<$<CONFIG:RelWithDebInfo>:JUNGLE_DEBUG_ENABLED>
)

add_library(jungle-base ${JUNGLE_SOURCES})
target_precompile_headers(jungle-base PUBLIC ${JUNGLE_HEADERS})
target_link_libraries(jungle-base PUBLIC ${JUNGLE_DEPLIBS})
target_include_directories(jungle-base PUBLIC include)
target_compile_options(jungle-base PUBLIC ${JUNGLE_OPTIONS})
target_compile_definitions(jungle-base PUBLIC ${JUNGLE_DEFINES})
add_library(jungle::base ALIAS jungle-base)

add_library(jungle-test src/test/test_main.cpp)
target_precompile_headers(jungle-test PUBLIC include/jungle/test/test.h)
target_link_libraries(jungle-test PUBLIC jungle::base)
add_library(jungle::test ALIAS jungle-test)

# Unit Tests

set(JUNGLE_UNIT_TEST_SOURCES
    unit_tests/hash_map.cpp
    unit_tests/mpsc.cpp
    unit_tests/serde_text.cpp
    unit_tests/algebra_matrix.cpp
    unit_tests/shared_memory.cpp
)

add_executable(jungle-base-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-base-unit-tests PUBLIC jungle::test)
add_test(NAME jungle-base-unit-tests COMMAND jungle-base-unit-tests)
