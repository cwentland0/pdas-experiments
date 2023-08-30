# Largely copied from pressiodemoapps, to make sure we're compatible

cmake_minimum_required(VERSION 3.18.0)
project(pdas-experiments CXX)

#=====================================================================
# we need c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORT_CPP17)
if(NOT COMPILER_SUPPORT_CPP17)
  message(FATAL_ERROR "Compiler does not support -std=c++17. This is required.")
endif()
set(PRESSIO_ENABLE_CXX17 ON)

#=====================================================================
# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds are not allowed.
    Please make a new directory (e.g. build directory) and run CMake from there.")
endif()

# default to release if build type is empty
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "${CMAKE_BUILD_TYPE} is not specified, defaulting to Release.")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# convert cmake build type to lower string
string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_tolower)
if( NOT cmake_build_type_tolower STREQUAL "debug"
    AND NOT cmake_build_type_tolower STREQUAL "release")
  message(FATAL_ERROR "Unknown build type \"${CMAKE_BUILD_TYPE}\".
    Allowed values are Debug, Release (case-insensitive).")
endif()

#=====================================================================
# We're not going to mess with OpenMP or Python bindings for now
# Just check that pressiodemoapps and pressiodemoapps-schwarz is specified correctly

if(NOT PDA_SOURCE)
  message(FATAL_ERROR "Must specify pressiodemoapps source directory via PDA_SOURCE")
else()
  if(NOT EXISTS ${PDA_SOURCE})
    message(NOTICE "PDA_SOURCE: ${PDA_SOURCE}")
    message(FATAL_ERROR "PDA_SOURCE does not exist or is not an absolute path")
  endif()
endif()

if(NOT PDAS_SOURCE)
  message(FATAL_ERROR "Must specify pressiodemoapps-schwarz source directory via PDAS_SOURCE")
else()
  if(NOT EXISTS ${PDAS_SOURCE})
    message(NOTICE "PDAS_SOURCE: ${PDAS_SOURCE}")
    message(FATAL_ERROR "PDAS_SOURCE does not exist or is not an absolute path")
  endif()
endif()

# build YAML
add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/tpls/yaml-cpp-0.7.0
  ${CMAKE_CURRENT_BINARY_DIR}/yamlcpp)

# include demoapps, Schwarz
include_directories(
  ${PDA_SOURCE}/tpls/eigen3
  ${PDA_SOURCE}/include
  ${PDA_SOURCE}/tests_cpp/pressio/include
  ${PDAS_SOURCE}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tpls/yaml-cpp-0.7.0/include)

add_executable(runner ${CMAKE_CURRENT_SOURCE_DIR}/src/runner.cc)
target_link_libraries(runner PRIVATE yaml-cpp)
