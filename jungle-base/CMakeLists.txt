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

set(JUNGLE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_library(jungle-base ${JUNGLE_SOURCES})
target_link_libraries(jungle-base PUBLIC stdc++exp)
target_include_directories(jungle-base PUBLIC include)
target_precompile_headers(jungle-base PUBLIC ${JUNGLE_HEADERS})
target_compile_options(jungle-base PUBLIC ${JUNGLE_OPTIONS})
add_library(jungle::base ALIAS jungle-base)

# Tests

add_executable(test_hash_map unit_tests/hash_map.cpp)
target_link_libraries(test_hash_map jungle::base)
add_test(NAME hash_map COMMAND test_hash_map)
