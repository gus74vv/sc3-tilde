# SPDX-License-Identifier: GPL-3.0-only
# sc_code — the in-patch sclang editor (jbox + paint + typing + evalfile).
#
# Pure UI: it links neither SuperCollider nor MSP, every Max symbol is resolved by
# the host (-undefined dynamic_lookup). The source lists come from the Makefile
# through print-flags, so a new sc_*.cpp is added in one place only.

sc3_make_flags("${CMAKE_CURRENT_SOURCE_DIR}" Makefile)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundle/Info.plist.in" _sc_code_plist)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sc_code.cpp" _sc_code_ver_line
     REGEX "^#define SC_CODE_VERSION ")
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" SC_CODE_VERSION "${_sc_code_ver_line}")
string(REPLACE "@VERSION@" "${SC_CODE_VERSION}" _sc_code_plist "${_sc_code_plist}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" "${_sc_code_plist}")

# jbox_new() reads dozens of static t_symbol* filled in by jpatcher_syms_init():
# the SDK's jpatcher_syms.c has to be compiled INTO the bundle and called from
# ext_main (Max does not export them; without it, SIGSEGV inside jbox_new).
add_library(sc_code MODULE
  ${SC_CODE_UI_SRC}
  ${SC_CODE_CORE_SRC}
  "${MAXINC}/common/jpatcher_syms.c")
set_target_properties(sc_code PROPERTIES
  OUTPUT_NAME "sc_code"
  PREFIX ""
  BUNDLE TRUE
  BUNDLE_EXTENSION mxo
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

target_include_directories(sc_code PRIVATE "${MAXINC}")
target_compile_options(sc_code PRIVATE
  -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch)
set_property(TARGET sc_code APPEND PROPERTY LINK_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/bundle/Info.plist.in")
target_link_options(sc_code PRIVATE -undefined dynamic_lookup)
target_link_libraries(sc_code PRIVATE
  -Wl,-framework,AppKit -Wl,-framework,UniformTypeIdentifiers)

add_custom_command(TARGET sc_code POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E rm -rf
          "$<TARGET_BUNDLE_CONTENT_DIR:sc_code>/MacOS/sc_code.dSYM"
  COMMAND sh -c "printf 'iLaXmax2' > '$<TARGET_BUNDLE_CONTENT_DIR:sc_code>/PkgInfo'"
  COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:sc_code>"
  COMMAND codesign --verify --strict "$<TARGET_BUNDLE_DIR:sc_code>"
  COMMENT "sealing sc_code.mxo"
  VERBATIM)

# --- gates: the editor's unit tests (pure C++, no Max) live in the Makefile ----
add_test(NAME sc_code.unit
  COMMAND "${SC3_MAKE}" -s test)
set_tests_properties(sc_code.unit PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME bundle.sc_code
  COMMAND "${CMAKE_SOURCE_DIR}/external/check_bundle.sh" "$<TARGET_BUNDLE_DIR:sc_code>")
