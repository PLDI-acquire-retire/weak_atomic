# -------------------------------------------------------------------
#            Build system for Atomic Reference Counting
# -------------------------------------------------------------------
# Requirements:
#   - CMake version 3.5
# -------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)
project(ARC VERSION 1.0
            DESCRIPTION "Atomic reference counting for C++"
            LANGUAGES CXX)

# Set a default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release
RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# required for Just::threads
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ljustthread -latomic")

message(STATUS "--------------- General configuration -------------")
message(STATUS "CMake Generator:        ${CMAKE_GENERATOR}")
message(STATUS "Compiler:               ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type:             ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS:        ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS  ${CMAKE_CXX_LINKER_FLAGS}")
message(STATUS "CMAKE_INSTALL_PREFIX:   ${CMAKE_INSTALL_PREFIX}" )

# Make sure -fno-omit-frame-pointer is set for profiling
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")

# -------------------------------------------------------------------
#                       Library definition
             
add_library(arc INTERFACE)
set(ARC_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
target_include_directories(arc INTERFACE ${ARC_INCLUDE_DIR})
target_compile_features(arc INTERFACE cxx_std_17)

# just::threads library
add_library(jss INTERFACE)
set(JSS_INCLUDE_DIR "/usr/include/justthread/")
target_include_directories(jss INTERFACE ${JSS_INCLUDE_DIR})
target_compile_features(jss INTERFACE cxx_std_17)

# Link against system threads (always)
find_package(Threads REQUIRED)
target_link_libraries(arc INTERFACE Threads::Threads)

# Enable 16 byte CAS
target_compile_definitions(arc INTERFACE -DMCX16)
target_compile_options(arc INTERFACE -mcx16)

# The following might not be needed for 16-byte atomics
target_compile_options(arc INTERFACE "-march=native")

# -------------------------------------------------------------------
#                             Tests

enable_testing()
add_subdirectory(tests)

# -------------------------------------------------------------------
#                           Benchmarks

add_subdirectory(benchmarks)



