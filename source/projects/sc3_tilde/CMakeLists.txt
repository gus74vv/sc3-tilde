# SPDX-License-Identifier: GPL-3.0-only
# sc3~: sclang + scsynth linked statically into the Max object.
#
# Build prerequisites, both outside this tree:
#   SC_DIR  the pinned SuperCollider sources with its statics built (`make sc`)
#   SC_APP  a SuperCollider.app of the same version. Its class library, scsynth plugins,
#           sounds and libsndfile are copied into the bundle, which then needs no
#           SuperCollider installation at runtime.

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)

set(SC_DIR "${CMAKE_SOURCE_DIR}/vendor/supercollider" CACHE PATH "SuperCollider 3.14.1 sources")
set(SC_BUILD_DIR "${SC_DIR}/build" CACHE PATH "SuperCollider build directory")
set(SC_APP "/Applications/SuperCollider.app" CACHE PATH "SuperCollider.app to copy runtime files from")

if(NOT EXISTS "${SC_BUILD_DIR}/lang/libsclang.a")
    message(FATAL_ERROR "No SuperCollider statics in ${SC_BUILD_DIR}: run `make sc` or set SC_DIR")
endif()
set(SC_RES "${SC_APP}/Contents/Resources")
set(SNDFILE_SRC "${SC_APP}/Contents/Frameworks/libsndfile.dylib")
if(NOT EXISTS "${SNDFILE_SRC}" OR NOT EXISTS "${SC_RES}/SCClassLibrary")
    message(FATAL_ERROR "No SuperCollider 3.14.1 at ${SC_APP}: install it or set SC_APP")
endif()
# scsynth plugins only: sc3~ has no supernova.
file(GLOB SC_PLUGINS "${SC_RES}/plugins/*.scx")
list(FILTER SC_PLUGINS EXCLUDE REGEX "_supernova\\.scx$")
# SC.app ships them (and libsndfile) universal; keep only the slices being built. Each slice carries its
# own signature, so the thinned file stays validly signed.
list(LENGTH CMAKE_OSX_ARCHITECTURES _narch)
if(_narch EQUAL 1)
    set(_lipo_args -thin ${CMAKE_OSX_ARCHITECTURES})
else()
    set(_lipo_args)
    foreach(_a ${CMAKE_OSX_ARCHITECTURES})
        list(APPEND _lipo_args -extract ${_a})
    endforeach()
endif()
set(SC_PLUGIN_CMDS)
foreach(_p ${SC_PLUGINS})
    get_filename_component(_n "${_p}" NAME)
    list(APPEND SC_PLUGIN_CMDS COMMAND lipo "${_p}" ${_lipo_args}
         -output "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/plugins/${_n}")
endforeach()

file(STRINGS sc3_tilde.cpp _ver REGEX "^#define SC3_TILDE_VERSION ")
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" PACKAGE_VERSION "${_ver}")
set(AUTHOR_DOMAIN "org.scmax")
set(BUNDLE_IDENTIFIER "sc3tilde")
set(COPYRIGHT_STRING "GPLv3 (SuperCollider 3.14.1 embedded)")
set(EXCLUDE_FROM_COLLECTIVES "no")

# libsndfile is the only non-Apple dependency of libscsynth. The copy from SC.app gets
# install_name @loader_path/../Frameworks/ so it resolves inside the bundle;
# install_name_tool breaks its Developer ID signature, hence the ad-hoc re-sign.
# The name must stay libsndfile.dylib: four plugins load it as
# @loader_path/../../Frameworks/libsndfile.dylib, which from Resources/plugins/ is
# this same file. Another name would load a second copy.
set(SNDFILE_NAME libsndfile.dylib)
set(SNDFILE "${CMAKE_CURRENT_BINARY_DIR}/Frameworks/${SNDFILE_NAME}")
add_custom_command(
    OUTPUT "${SNDFILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/Frameworks"
    COMMAND lipo "${SNDFILE_SRC}" ${_lipo_args} -output "${SNDFILE}"
    COMMAND install_name_tool -id "@loader_path/../Frameworks/${SNDFILE_NAME}" "${SNDFILE}"
    COMMAND codesign --force --sign - "${SNDFILE}"
    DEPENDS "${SNDFILE_SRC}"
    COMMENT "libsndfile from ${SC_APP}"
    VERBATIM)
add_custom_target(sc3_sndfile DEPENDS "${SNDFILE}")

add_library(${PROJECT_NAME} MODULE
    sc3_tilde.cpp
    Sc3Engine.cpp
)
add_dependencies(${PROJECT_NAME} sc3_sndfile)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${MAX_SDK_INCLUDES}"
    "${MAX_SDK_MSP_INCLUDES}"
    "${SC_BUILD_DIR}/common"
    "${SC_DIR}/include/common"
    "${SC_DIR}/include/lang"
    "${SC_DIR}/include/plugin_interface"
    "${SC_DIR}/include/server"
    "${SC_DIR}/server/scsynth"
    "${SC_DIR}/common"
    "${SC_DIR}/lang/LangSource"
    "${SC_DIR}/external_libraries/nova-tt"
    "${SC_DIR}/external_libraries/TLSF-2.4.6/src")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${SC_DIR}/external_libraries/boost"
    "${SC_DIR}/external_libraries/boost_sync/include"
    "${SC_DIR}/external_libraries/yaml-cpp/include")

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch)

# Link order matters for the statics.
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${SC_BUILD_DIR}/lang/libsclang.a"
    "${SC_BUILD_DIR}/external_libraries/hidapi/mac/libhidapi.a"
    "${SC_BUILD_DIR}/external_libraries/hidapi/hidapi_parser/libhidapi_parser.a"
    "${SC_BUILD_DIR}/external_libraries/libboost_thread_lib.a"
    "${SC_BUILD_DIR}/external_libraries/libboost_program_options_lib.a"
    "${SC_BUILD_DIR}/external_libraries/libboost_regex_lib.a"
    "${SC_BUILD_DIR}/server/scsynth/libscsynth.a"
    "${SC_BUILD_DIR}/external_libraries/libtlsf.a"
    "-framework Accelerate"
    "-framework CoreServices"
    "-framework Foundation"
    "-framework AppKit"
    "-framework Carbon"
    "-framework CoreAudio"
    "-framework CoreMIDI"
    "-framework IOKit"
    "-framework CoreFoundation"
    "${SNDFILE}"
    "${SC_BUILD_DIR}/external_libraries/libyaml.a")

# Before max-posttarget.cmake, so its POST_BUILD codesign seals these files too.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy "${SNDFILE}" "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/SCMaxCompat"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/SCMaxCompat"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SC_RES}/SCClassLibrary"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/SCClassLibrary"
    # Read through Platform.resourceDir: example sounds and HID device specs.
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SC_RES}/sounds"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/sounds"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SC_RES}/HID_Support"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/HID_Support"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/plugins"
    ${SC_PLUGIN_CMDS}
    # Notices travel with the bundle. LICENSE sits beside licenses/ so its ../LICENSE link resolves.
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/licenses"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/licenses"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/LICENSE"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/LICENSE"
    VERBATIM)

# The plist values above live in this file. Relinking on its change re-runs the
# POST_BUILD codesign, which otherwise leaves a stale seal over the new Info.plist.
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_LIST_FILE}")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)

set(SC3_MXO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/sc3~.mxo")
add_test(NAME sc3_tilde_bundle
    COMMAND "${PACKAGE_SCRIPTS}/check_bundle.sh" "${SC3_MXO}" ${CMAKE_OSX_ARCHITECTURES})
add_test(NAME sc3_tilde_symbols
    COMMAND python3 "${PACKAGE_SCRIPTS}/check_flat_symbols.py" "${SC3_MXO}" --max "${MAX_APP}")
set_tests_properties(sc3_tilde_symbols PROPERTIES SKIP_RETURN_CODE 77)
