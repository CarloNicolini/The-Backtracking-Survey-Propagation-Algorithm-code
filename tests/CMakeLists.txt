add_executable(bsp-test test_thermo_sp.cpp)
target_link_libraries(bsp-test PRIVATE bsp_core)
add_test(NAME thermo-sp COMMAND bsp-test)

add_executable(bsp-rsb2-test test_rsb2.cpp)
target_link_libraries(bsp-rsb2-test PRIVATE bsp_core)
add_test(NAME rsb2-kernels COMMAND bsp-rsb2-test)

add_executable(bsp-rsb2-probe rsb2_probe.cpp)
target_link_libraries(bsp-rsb2-probe PRIVATE bsp_core)
add_test(NAME rsb2-glue COMMAND bsp-rsb2-probe 3 4.2 1000 1 1 1024 0)

add_executable(bsp-release-check release_check.cpp)
target_link_libraries(bsp-release-check PRIVATE bsp_core)
add_test(NAME release-score COMMAND bsp-release-check 3 4.1 500 3 50 100)
add_test(NAME release-score-spy COMMAND bsp-release-check 3 4.1 500 3 50 100 0.25)
