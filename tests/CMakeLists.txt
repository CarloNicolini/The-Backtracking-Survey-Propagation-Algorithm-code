add_executable(bsp-test test_thermo_sp.cpp)
target_link_libraries(bsp-test PRIVATE bsp_core)
add_test(NAME thermo-sp COMMAND bsp-test)
