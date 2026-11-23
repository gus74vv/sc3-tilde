# SPDX-License-Identifier: GPL-3.0-only
# stage_package.cmake — assembles <stage>/sc3-tilde the way Max reads a package:
#
#   package-info.json   metadata (version from SC3_TILDE_VERSION)
#   externals/          the two .mxo
#   help/               the two .maxhelp (Help key / double click)
#   docs/               the two .maxref.xml (reference viewer)
#   examples/           the user-facing patches and snippets
#
# Run in script mode by the `package` target:
#   cmake -DSC3_PKG_STAGE=... -DSC3_PKG_METADATA=... -DSC3_MXO=... -P stage_package.cmake

foreach(_var SC3_PKG_STAGE SC3_PKG_METADATA SC3_MXO SC_CODE_MXO ROOT)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "stage_package.cmake: missing -${_var}")
  endif()
endforeach()

foreach(_path "${SC3_MXO}" "${SC_CODE_MXO}" "${SC3_PKG_METADATA}")
  if(NOT EXISTS "${_path}")
    message(FATAL_ERROR "stage_package.cmake: missing ${_path} (build the bundles first)")
  endif()
endforeach()

file(REMOVE_RECURSE "${SC3_PKG_STAGE}")
file(MAKE_DIRECTORY
  "${SC3_PKG_STAGE}/externals"
  "${SC3_PKG_STAGE}/help"
  "${SC3_PKG_STAGE}/docs"
  "${SC3_PKG_STAGE}/examples")

# The .mxo are sealed bundles: copy the directory as-is, never rebuild the layout.
file(COPY "${SC3_MXO}" DESTINATION "${SC3_PKG_STAGE}/externals")
file(COPY "${SC_CODE_MXO}" DESTINATION "${SC3_PKG_STAGE}/externals")
file(COPY "${SC3_PKG_METADATA}" DESTINATION "${SC3_PKG_STAGE}")

file(GLOB _helps "${ROOT}/external/help/*.maxhelp" "${ROOT}/external/sc_code/help/*.maxhelp")
file(GLOB _refs "${ROOT}/external/docs/*.maxref.xml")
file(GLOB _examples "${ROOT}/examples/*")
foreach(_f ${_helps})
  file(COPY "${_f}" DESTINATION "${SC3_PKG_STAGE}/help")
endforeach()
foreach(_f ${_refs})
  file(COPY "${_f}" DESTINATION "${SC3_PKG_STAGE}/docs")
endforeach()
foreach(_f ${_examples})
  file(COPY "${_f}" DESTINATION "${SC3_PKG_STAGE}/examples")
endforeach()

message(STATUS "staged ${SC3_PKG_STAGE}")
