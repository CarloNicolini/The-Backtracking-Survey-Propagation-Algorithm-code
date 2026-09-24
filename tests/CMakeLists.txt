add_executable(bsp-test test_thermo_sp.cpp)
target_link_libraries(bsp-test PRIVATE bsp_core)
add_test(NAME thermo-sp COMMAND bsp-test)

add_executable(bsp-release-check release_check.cpp)
target_link_libraries(bsp-release-check PRIVATE bsp_core)
add_test(NAME release-score COMMAND bsp-release-check 3 4.1 500 3 50 100)
add_test(NAME release-score-spy COMMAND bsp-release-check 3 4.1 500 3 50 100 0.25)
