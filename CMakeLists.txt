cmake_minimum_required(VERSION 3.19)

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

option(BSP_ENABLE_OPENMP "Use OpenMP when available" ON)
option(BSP_BUILD_PYTHON "Build native Python bindings" OFF)
option(BSP_BUILD_CLI "Build command-line executable" ON)
option(BSP_BUILD_TESTS "Build C++ tests" ON)
option(BSP_IDE_LINK "Link compile_commands.json into source tree" ON)

if(BSP_BUILD_CLI)
include(FetchContent)
set(CXXOPTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CXXOPTS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CXXOPTS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

endif()

add_subdirectory(src)

if(BSP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
  set_target_properties(bsp-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
if(BSP_BUILD_CLI)
  set_target_properties(bsp PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
if(BSP_BUILD_PYTHON)
  find_package(Python 3.12 REQUIRED COMPONENTS Interpreter Development.Module)
  execute_process(COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_VARIABLE nanobind_DIR OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
  find_package(nanobind CONFIG REQUIRED)
  nanobind_add_module(_native python/bindings/native.cpp)
  target_link_libraries(_native PRIVATE bsp_core)
  install(TARGETS _native LIBRARY DESTINATION pybsp)
endif()
if(BSP_IDE_LINK)
  file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC)
endif()

message(STATUS "BSP build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "BSP C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "BSP native arch: ${BSP_NATIVE_ARCH}")
