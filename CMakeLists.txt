cmake_minimum_required(VERSION 3.16)

project(BSP
  VERSION 2018.0
  DESCRIPTION "Backtracking Survey Propagation for random K-SAT"
  LANGUAGES CXX
)

if(MSVC)
  message(FATAL_ERROR "MSVC is not supported; build with GCC or Clang")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tune for the build machine CPU (best local performance; turn off for portable binaries).
option(BSP_NATIVE_ARCH "Optimize for the host CPU (-march=native)" ON)

include(FetchContent)
set(CXXOPTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CXXOPTS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CXXOPTS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

# Keep executables at the binary-dir root (build/release/bsp, not build/release/src/bsp).
set_target_properties(bsp bsp-test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# clangd / IDE: expose compile_commands.json at the source root.
file(CREATE_LINK
  "${CMAKE_BINARY_DIR}/compile_commands.json"
  "${CMAKE_SOURCE_DIR}/compile_commands.json"
  SYMBOLIC
)

message(STATUS "BSP build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "BSP C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "BSP native arch: ${BSP_NATIVE_ARCH}")
