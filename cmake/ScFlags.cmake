# SPDX-License-Identifier: GPL-3.0-only
# ScFlags.cmake — the SuperCollider flags are NOT written here: they come from
# external/sc.mk (and from external/sc_code/Makefile for the editor sources).
#
# sc.mk is the single source of truth for the include paths and for the ORDER of
# the statics (verified in phase1/Makefile). Both recipes export them through a
# `print-flags` target; duplicating the lists in CMake would let the two builds
# drift and only one of them fail, which is exactly the bug this tree already
# paid for once (a bundle that nothing compiled).

find_program(SC3_MAKE NAMES gmake make REQUIRED)

# sc3_fold_frameworks(<list var> <out var>): CMake gives a bare name the "-l"
# prefix, so "-framework AppKit" in a link line becomes "-framework -lAppKit".
# Folding the pair into one ld argument ("-Wl,-framework,AppKit") keeps the flags
# exactly as sc.mk wrote them, order included.
function(sc3_fold_frameworks list_var out_var)
  set(_in "${${list_var}}")
  set(_out "")
  while(_in)
    list(POP_FRONT _in _item)
    if(_item STREQUAL "-framework")
      list(POP_FRONT _in _fw)
      list(APPEND _out "-Wl,-framework,${_fw}")
    else()
      list(APPEND _out "${_item}")
    endif()
  endwhile()
  set(${out_var} "${_out}" PARENT_SCOPE)
endfunction()

# sc3_make_flags(<dir> <makefile>): runs `make -f <makefile> print-flags` and
# defines, in the caller's scope, one list variable per printed KEY=value line.
function(sc3_make_flags dir makefile)
  execute_process(
    COMMAND "${SC3_MAKE}" -s -f "${makefile}" print-flags
    WORKING_DIRECTORY "${dir}"
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _err
    RESULT_VARIABLE _rc)
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "`make -f ${makefile} print-flags` failed in ${dir}: ${_err}")
  endif()
  string(REPLACE "\r\n" "\n" _out "${_out}")
  string(REPLACE "\n" ";" _lines "${_out}")
  foreach(_line IN LISTS _lines)
    if(_line MATCHES "^([A-Z0-9_]+)=(.*)$")
      separate_arguments(_args UNIX_COMMAND "${CMAKE_MATCH_2}")
      set("${CMAKE_MATCH_1}" "${_args}" PARENT_SCOPE)
    endif()
  endforeach()
endfunction()
