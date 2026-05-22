set(IPREF "include/jungle/core") # Include prefix for headers

set(JUNGLE_HEADERS
    ${IPREF}/ecs/component_storage.h
    ${IPREF}/ecs/component.h
    ${IPREF}/ecs/entity.h
    ${IPREF}/ecs/manager.h
)

set(JUNGLE_SOURCES
    src/ecs/component.cpp
    src/ecs/entity.cpp
)

add_library(jungle-core ${JUNGLE_SOURCES})
target_link_libraries(jungle-core jungle::base)
target_include_directories(jungle-core PUBLIC include)
target_precompile_headers(jungle-core PUBLIC ${JUNGLE_HEADERS})
add_library(jungle::core ALIAS jungle-core)
