add_library(bsp_core STATIC
  Graph.cpp
  release_score.cpp
  Vertex.cpp
  thermo_sp.cpp
  thermo_policy.cpp
  tsallis.cpp
  walksat.cpp
)

target_include_directories(bsp_core
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
target_compile_features(bsp_core PUBLIC cxx_std_14)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(bsp_core PUBLIC OpenMP::OpenMP_CXX)
  message(STATUS "BSP OpenMP: ON")
else()
  message(STATUS "BSP OpenMP: OFF (serial loops)")
endif()

# Historical baseline: g++ -Ofast *.cpp (GCC / Clang / AppleClang only).
target_compile_options(bsp_core PRIVATE
  $<$<CONFIG:Release>:-O3;-ffast-math;-fno-math-errno;-funroll-loops;-fomit-frame-pointer>
  $<$<CONFIG:RelWithDebInfo>:-O2;-g;-ffast-math;-fno-math-errno>
  $<$<CONFIG:Debug>:-O0;-g>
)

if(BSP_NATIVE_ARCH)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-march=native" BSP_HAS_MARCH_NATIVE)
  if(BSP_HAS_MARCH_NATIVE)
    # Two $<CONFIG:...> checks: multi-value CONFIG needs CMake 3.19+.
    target_compile_options(bsp_core PRIVATE
      $<$<CONFIG:Release>:-march=native>
      $<$<CONFIG:RelWithDebInfo>:-march=native>)
  endif()
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT BSP_IPO_SUPPORTED OUTPUT BSP_IPO_ERROR)
if(BSP_IPO_SUPPORTED)
  set_property(TARGET bsp_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
  message(STATUS "IPO/LTO not available: ${BSP_IPO_ERROR}")
endif()
message(STATUS "BSP IPO/LTO: ${BSP_IPO_SUPPORTED}")

if(UNIX AND NOT APPLE)
  target_link_libraries(bsp_core PUBLIC m)
endif()

add_executable(bsp main.cpp outdir.cpp)
target_link_libraries(bsp PRIVATE bsp_core cxxopts::cxxopts)
install(TARGETS bsp RUNTIME DESTINATION bin)
