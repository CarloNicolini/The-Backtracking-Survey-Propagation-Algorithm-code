cmake_minimum_required(VERSION 3.16)

project(BSP
  VERSION 2018.0
  DESCRIPTION "Backtracking Survey Propagation for random K-SAT"
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel)
endif()

# Tune for the build machine CPU (best local performance; turn off for portable binaries).
option(BSP_NATIVE_ARCH "Optimize for the host CPU (-march=native / equivalent)" ON)

set(BSP_NN_SOURCES
  NnScorer.cpp
  third_party/genann/genann.c
)

add_executable(bsp
  main.cpp
  Graph.cpp
  Vertex.cpp
  walksat.cpp
  ${BSP_NN_SOURCES}
)

add_executable(bsp_train
  bsp_train.cpp
  ${BSP_NN_SOURCES}
)

target_include_directories(bsp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bsp_train PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(bsp_train PROPERTIES OUTPUT_NAME "bsp-train")

# ---------------------------------------------------------------------------
# Compiler / linker optimization flags
# Historical baseline: g++ -Ofast *.cpp
# ---------------------------------------------------------------------------

if(MSVC)
  # Speed-oriented MSVC flags (Visual Studio). IPO below adds /GL + /LTCG.
  target_compile_options(bsp PRIVATE
    /W3
    /EHsc
    /MP
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/O2;/Ob2;/Oi;/Ot;/Oy;/Gw;/Gy;/fp:fast>
    $<$<CONFIG:Debug>:/Od;/Zi>
  )
  target_link_options(bsp PRIVATE
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/OPT:REF;/OPT:ICF>
  )
  if(BSP_NATIVE_ARCH AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|x64)$")
    target_compile_options(bsp PRIVATE
      $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/arch:AVX2>)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Linux (and Homebrew) g++
  target_compile_options(bsp PRIVATE
    $<$<CONFIG:Release>:-O3;-ffast-math;-fno-math-errno;-fno-trapping-math;-funroll-loops;-fomit-frame-pointer>
    $<$<CONFIG:RelWithDebInfo>:-O2;-g;-ffast-math;-fno-math-errno>
    $<$<CONFIG:Debug>:-O0;-g>
  )
  if(BSP_NATIVE_ARCH)
    target_compile_options(bsp PRIVATE
      $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-march=native>)
    target_link_options(bsp PRIVATE
      $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-march=native>)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # macOS AppleClang / LLVM Clang
  target_compile_options(bsp PRIVATE
    $<$<CONFIG:Release>:-O3;-ffast-math;-fno-math-errno;-funroll-loops;-fomit-frame-pointer>
    $<$<CONFIG:RelWithDebInfo>:-O2;-g;-ffast-math;-fno-math-errno>
    $<$<CONFIG:Debug>:-O0;-g>
  )
  if(BSP_NATIVE_ARCH)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" BSP_HAS_MARCH_NATIVE)
    if(BSP_HAS_MARCH_NATIVE)
      target_compile_options(bsp PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-march=native>)
    endif()
  endif()
else()
  message(WARNING "Unrecognized compiler '${CMAKE_CXX_COMPILER_ID}'; using generic Release flags")
  target_compile_options(bsp PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0;-g>
  )
endif()

# Link-time / whole-program optimization (GCC -flto, Clang -flto, MSVC /GL+/LTCG).
include(CheckIPOSupported)
check_ipo_supported(RESULT BSP_IPO_SUPPORTED OUTPUT BSP_IPO_ERROR)
if(BSP_IPO_SUPPORTED)
  set_property(TARGET bsp PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  # Prefer parallel LTO with GCC when available.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(bsp PRIVATE $<$<CONFIG:Release>:-flto=auto>)
    target_link_options(bsp PRIVATE $<$<CONFIG:Release>:-flto=auto>)
  endif()
else()
  message(STATUS "IPO/LTO not available: ${BSP_IPO_ERROR}")
endif()

set_target_properties(bsp PROPERTIES OUTPUT_NAME "main")

if(UNIX AND NOT APPLE)
  target_link_libraries(bsp PRIVATE m)
  target_link_libraries(bsp_train PRIVATE m)
endif()

install(TARGETS bsp bsp_train RUNTIME DESTINATION bin)

message(STATUS "BSP build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "BSP C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "BSP native arch: ${BSP_NATIVE_ARCH}")
message(STATUS "BSP IPO/LTO: ${BSP_IPO_SUPPORTED}")
