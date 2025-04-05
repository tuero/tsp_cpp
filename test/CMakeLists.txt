add_executable(tsp_test tsp_test.cpp)
target_link_libraries(tsp_test PUBLIC tsp)
add_test(tsp_test tsp_test)
