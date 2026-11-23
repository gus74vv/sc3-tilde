# SPDX-License-Identifier: GPL-3.0-only
# sc3~ — the Max object: Max glue + Sc3Engine + the SuperCollider statics.
#
# The includes and the statics come from sc.mk through print-flags (see
# cmake/ScFlags.cmake), in the same order the headless binaries use. The bundle
# contents mirror what build.sh assembles, so both paths produce the same .mxo.

sc3_make_flags("${CMAKE_CURRENT_SOURCE_DIR}" sc.mk)
sc3_fold_frameworks(SC_LIBS SC_LIBS_LINK)

# libsndfile (the only non-Apple dep of libscsynth) is prepared by the Makefile:
# the copy from SC.app, install_name rewritten to @loader_path/../Frameworks and
# re-signed ad-hoc. One implementation, used by the headless tests too.
set(SC3_SNDFILE "${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/libsndfile.1.dylib")
add_custom_target(sc3_sndfile
  COMMAND "${SC3_MAKE}" -s Frameworks/libsndfile.1.dylib MAX_SDK="${MAX_SDK}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "libsndfile from ${SC_APP}"
  VERBATIM)

# Info.plist: @VERSION@ is ours, CMake only knows MACOSX_BUNDLE_* keys, so it is
# substituted here and handed over already final.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in" _sc3_plist)
string(REPLACE "@VERSION@" "${SC3_TILDE_VERSION}" _sc3_plist "${_sc3_plist}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" "${_sc3_plist}")

add_library(sc3_tilde MODULE sc3_tilde.cpp Sc3Engine.cpp)
add_dependencies(sc3_tilde sc3_sndfile)
set_target_properties(sc3_tilde PROPERTIES
  OUTPUT_NAME "sc3~"
  PREFIX ""
  BUNDLE TRUE
  BUNDLE_EXTENSION mxo
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

target_include_directories(sc3_tilde PRIVATE "${MAXINC}" "${MSPINC}")
# -isystem pairs travel inside SC_CXX_INCLUDES, so they go as options, unfiltered.
target_compile_options(sc3_tilde PRIVATE
  ${SC_CXX_INCLUDES}
  -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch)
set_property(TARGET sc3_tilde APPEND PROPERTY LINK_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")
target_link_options(sc3_tilde PRIVATE
  -undefined dynamic_lookup
  -F${MSPINC})   # MaxAudioAPI lives in the SDK, not in the system frameworks
target_link_libraries(sc3_tilde PRIVATE
  -Wl,-framework,MaxAudioAPI
  ${SC_LIBS_LINK})

# Everything the bundle carries, then the signature that seals it (Max validates
# the resource seal: signing before the copies would leave a stale one).
add_custom_command(TARGET sc3_tilde POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E rm -rf
          "$<TARGET_BUNDLE_CONTENT_DIR:sc3_tilde>/MacOS/sc3~.dSYM"
  COMMAND "${CMAKE_COMMAND}" -E make_directory
          "$<TARGET_BUNDLE_CONTENT_DIR:sc3_tilde>/Frameworks"
  COMMAND "${CMAKE_COMMAND}" -E copy "${SC3_SNDFILE}"
          "$<TARGET_BUNDLE_CONTENT_DIR:sc3_tilde>/Frameworks/"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_SOURCE_DIR}/phase1/compat/SCMaxCompat"
          "$<TARGET_BUNDLE_CONTENT_DIR:sc3_tilde>/Resources/SCMaxCompat"
  COMMAND sh -c "printf 'iLaXmax2' > '$<TARGET_BUNDLE_CONTENT_DIR:sc3_tilde>/PkgInfo'"
  COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:sc3_tilde>"
  COMMAND codesign --verify --strict "$<TARGET_BUNDLE_DIR:sc3_tilde>"
  COMMENT "sealing sc3~.mxo"
  VERBATIM)

# --- gates ------------------------------------------------------------------
# Same checks as `make gate`, plus the bundle ones (issue #1, item 8).
set(SC3_GATE "${CMAKE_SOURCE_DIR}/external")

add_test(NAME refs
  COMMAND python3 "${SC3_GATE}/check_refs.py")
set_tests_properties(refs PROPERTIES WORKING_DIRECTORY "${SC3_GATE}")

add_test(NAME sc_arch
  COMMAND "${SC3_GATE}/scripts/build_sc.sh" --check)
set_tests_properties(sc_arch PROPERTIES WORKING_DIRECTORY "${SC3_GATE}")

# The headless binaries stay in the Makefile (they do not need Max): ctest builds
# and runs them, so a single command covers the same ground as `make gate`.
function(sc3_headless case target)
  add_test(NAME headless.${case}
    COMMAND sh -c "${SC3_MAKE} -s ${target} && ./build/${target}")
  set_tests_properties(headless.${case} PROPERTIES WORKING_DIRECTORY "${SC3_GATE}")
endfunction()
sc3_headless(engine engine_test)
sc3_headless(multi multi_test)
sc3_headless(reopen reopen_test)
sc3_headless(input input_test)
sc3_headless(fail fail_test)

# The engine samples at the server rate; 44.1 kHz is the other path through the
# buffer setup, so it is a case of its own.
add_test(NAME headless.sr-44100
  COMMAND sh -c "${SC3_MAKE} -s engine_test && SC3_SR=44100 ./build/engine_test")
set_tests_properties(headless.sr-44100 PROPERTIES WORKING_DIRECTORY "${SC3_GATE}")

add_test(NAME bundle.sc3_tilde
  COMMAND "${SC3_GATE}/check_bundle.sh" "$<TARGET_BUNDLE_DIR:sc3_tilde>")
add_test(NAME symbols.sc3_tilde
  COMMAND python3 "${SC3_GATE}/check_flat_symbols.py"
          "$<TARGET_BUNDLE_DIR:sc3_tilde>/Contents/MacOS/sc3~")
