# SPDX-License-Identifier: GPL-3.0-only
# sc3-tilde — SuperCollider (sclang + scsynth) in-process inside Max.
#
# Two entry points, one source of truth for the SuperCollider flags:
#   * ./build.sh in external/ installs inside the package in
#     ~/Documents/Max 9/Packages/sc3-tilde (dev loop, no CMake, uses external/sc.mk);
#   * this file builds the same two bundles and stages the Max package
#     (build/package/sc3-tilde) and runs the same gates with ctest.
#
#   cmake -S . -B build && cmake --build build -j
#   ctest --test-dir build --output-on-failure
#   cmake --build build --target package
#
# Requires Xcode CLT, the max-sdk package, a built vendor/supercollider and an
# installed SuperCollider 3.14.1 (same version as the vendored sources).

cmake_minimum_required(VERSION 3.19)

if(NOT APPLE)
  message(FATAL_ERROR "sc3~ and sc_code are macOS-only (AppKit, CoreAudio, Max API)")
endif()

# The vendored SuperCollider statics are single-slice: a universal binary would
# link and then fail to load (issue #1, item 4).
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architecture")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# The version lives in the source that prints it in the banner; the bundle, the
# ref pages and package-info.json all read it from there.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/external/sc3_tilde.cpp" _sc3_ver_line
     REGEX "^#define SC3_TILDE_VERSION ")
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" SC3_TILDE_VERSION "${_sc3_ver_line}")
if(SC3_TILDE_VERSION STREQUAL "")
  message(FATAL_ERROR "cannot read SC3_TILDE_VERSION from external/sc3_tilde.cpp")
endif()

project(sc3_tilde VERSION ${SC3_TILDE_VERSION} LANGUAGES C CXX OBJCXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(ScFlags)

# Paths (the Max SDK has a space in it, so they stay quoted strings in CMake).
if(DEFINED ENV{MAX_SDK})
  set(_max_sdk_default "$ENV{MAX_SDK}")
else()
  set(_max_sdk_default "$ENV{HOME}/Documents/Max 9/Packages/max-sdk/source/max-sdk-base")
endif()
set(MAX_SDK "${_max_sdk_default}" CACHE PATH "max-sdk-base (c74support/max-includes)")
set(SC_APP "/Applications/SuperCollider.app" CACHE PATH "SuperCollider 3.14.1 to take libsndfile from")

if(NOT EXISTS "${MAX_SDK}/c74support/max-includes")
  message(FATAL_ERROR "no Max SDK at ${MAX_SDK} (set MAX_SDK)")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vendor/supercollider/build/lang/libsclang.a")
  message(FATAL_ERROR "missing vendor/supercollider/build: build the vendored SuperCollider first")
endif()

set(MAXINC "${MAX_SDK}/c74support/max-includes")
set(MSPINC "${MAX_SDK}/c74support/msp-includes")

enable_testing()

add_subdirectory(external)
add_subdirectory(external/sc_code)

include(Package)
