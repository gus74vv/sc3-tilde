cmake_minimum_required(VERSION 3.19)

# Single-arch by default: the vendored SuperCollider statics are built for one slice.
if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architecture")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

string(REGEX REPLACE "(.*)/" "" THIS_FOLDER_NAME "${CMAKE_CURRENT_SOURCE_DIR}")
project(${THIS_FOLDER_NAME} C CXX)

if(NOT APPLE)
    message(FATAL_ERROR "sc3~ and sc_code are macOS-only (AppKit, CoreAudio)")
endif()

option(EXPORT_COMPILE_COMMANDS "Export compile commands for LSP" ON)
if (EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

set(PACKAGE_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/source/scripts")
enable_testing()

add_subdirectory(source/projects/sc_code)
add_subdirectory(source/projects/sc3_tilde)
