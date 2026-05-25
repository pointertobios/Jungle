set(IPREF "include/jungle") # Include prefix for headers

set(JUNGLE_HEADERS
    ${IPREF}/concepts.h
    ${IPREF}/container/hash_map.h
    ${IPREF}/types/int.h
    ${IPREF}/types/uchar.h
    ${IPREF}/util/murmur.h
    ${IPREF}/util/parse.h
    ${IPREF}/util/type_id.h
    ${IPREF}/util/types.h
    ${IPREF}/debug.h
    ${IPREF}/meta.h
    ${IPREF}/panic.h
    ${IPREF}/preusing.h
)

set(JUNGLE_SOURCES
    src/types/uchar.cpp
    src/util/parse.cpp
    src/panic.cpp
)

set(JUNGLE_DEPLIBS
    stdc++exp
)

set(JUNGLE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -freflection
    -fcontracts
    $<$<CONFIG:Debug>:-fcontract-evaluation-semantic=ignore>
    $<$<CONFIG:Release>:-fcontract-evaluation-semantic=ignore>
)

add_library(jungle-base ${JUNGLE_SOURCES})
target_precompile_headers(jungle-base PUBLIC ${JUNGLE_HEADERS})
target_link_libraries(jungle-base PUBLIC ${JUNGLE_DEPLIBS})
target_include_directories(jungle-base PUBLIC include)
target_compile_options(jungle-base PUBLIC ${JUNGLE_OPTIONS})
add_library(jungle::base ALIAS jungle-base)

add_library(jungle-test src/test/test_main.cpp)
target_precompile_headers(jungle-test PUBLIC include/jungle/test/test.h)
target_link_libraries(jungle-test PUBLIC jungle::base)
add_library(jungle::test ALIAS jungle-test)

set(JUNGLE_UNIT_TEST_SOURCES
unit_tests/hash_map.cpp
)

# Unit Tests

add_executable(jungle-base-unit-tests ${JUNGLE_UNIT_TEST_SOURCES})
target_link_libraries(jungle-base-unit-tests PUBLIC jungle::test)
add_test(NAME jungle-base-unit-tests COMMAND jungle-base-unit-tests)
