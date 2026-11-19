# SPDX-License-Identifier: GPL-3.0-only
# sc_code: in-patch sclang editor. Pure UI: links neither SuperCollider nor MSP.

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
enable_language(OBJCXX)

file(STRINGS sc_code.cpp _ver REGEX "^#define SC_CODE_VERSION ")
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" PACKAGE_VERSION "${_ver}")
set(AUTHOR_DOMAIN "org.scmax")
set(BUNDLE_IDENTIFIER "sc_code")
set(COPYRIGHT_STRING "GPLv3")
set(EXCLUDE_FROM_COLLECTIVES "no")

add_library(${PROJECT_NAME} MODULE
    sc_code.cpp
    sc_clipboard.mm
    sc_filedialog.mm
    sc_bar.cpp
    sc_blink.cpp
    sc_bracket.cpp
    sc_buffer.cpp
    sc_call.cpp
    sc_complete.cpp
    sc_highlight.cpp
    sc_indent.cpp
    sc_index.cpp
    sc_lexer.cpp
    sc_path.cpp
    sc_region.cpp
    sc_scroll.cpp
    sc_snip.cpp
    sc_theme.cpp
    sc_wrap.cpp
    # Required for UI objects: jbox_new() reads the symbols jpatcher_syms_init() fills.
    "${MAX_SDK_INCLUDES}/common/jpatcher_syms.c"
)

target_include_directories(${PROJECT_NAME} PRIVATE "${MAX_SDK_INCLUDES}")
# The `(method)` casts of class_addmethod trip -Wcast-function-type-mismatch.
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch>)
target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework AppKit"
    "-framework UniformTypeIdentifiers")

# The plist values above live in this file. Relinking on its change re-runs the
# POST_BUILD codesign, which otherwise leaves a stale seal over the new Info.plist.
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_LIST_FILE}")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)

set(SC_CODE_MXO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/sc_code.mxo")
add_test(NAME sc_code_bundle
    COMMAND "${PACKAGE_SCRIPTS}/check_bundle.sh" "${SC_CODE_MXO}" ${CMAKE_OSX_ARCHITECTURES})
add_test(NAME sc_code_symbols
    COMMAND python3 "${PACKAGE_SCRIPTS}/check_flat_symbols.py" "${SC_CODE_MXO}")
set_tests_properties(sc_code_symbols PROPERTIES SKIP_RETURN_CODE 77)
