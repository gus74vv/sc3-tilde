# SPDX-License-Identifier: GPL-3.0-only
# check_package.cmake — a package that is missing a folder is not an error Max
# reports: the object simply does not exist. So the layout is a gate, not a
# convention. Checks the staged folder and the metadata inside it.

if(NOT DEFINED STAGE OR NOT EXISTS "${STAGE}")
  message(FATAL_ERROR "check_package.cmake: nothing staged at ${STAGE}")
endif()

set(_missing "")
foreach(_rel
    package-info.json
    externals/sc3~.mxo/Contents/MacOS/sc3~
    externals/sc_code.mxo/Contents/MacOS/sc_code
    help/sc3~.maxhelp
    help/sc_code.maxhelp
    docs/sc3~.maxref.xml
    docs/sc_code.maxref.xml
    examples/example.maxpat)
  if(NOT EXISTS "${STAGE}/${_rel}")
    list(APPEND _missing "${_rel}")
  endif()
endforeach()
if(_missing)
  string(REPLACE ";" "
  " _pretty "${_missing}")
  message(FATAL_ERROR "package layout: missing:
  ${_pretty}")
endif()

file(READ "${STAGE}/package-info.json" _info)
if(NOT _info MATCHES "\"name\"[ \t]*:[ \t]*\"sc3-tilde\"")
  message(FATAL_ERROR "package-info.json: name is not sc3-tilde")
endif()
if(_info MATCHES "@SC3_TILDE_VERSION@")
  message(FATAL_ERROR "package-info.json: @SC3_TILDE_VERSION@ was not substituted")
endif()
if(NOT _info MATCHES "\"version\"[ \t]*:[ \t]*\"[0-9]+\\.[0-9]+\\.[0-9]+\"")
  message(FATAL_ERROR "package-info.json: version is not a plain x.y.z")
endif()
message(STATUS "package layout OK: ${STAGE}")
