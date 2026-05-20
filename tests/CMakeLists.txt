add_executable(base
    base.cpp

)
target_link_libraries(base jungle::core)
add_test(base base)
