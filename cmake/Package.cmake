# SPDX-License-Identifier: GPL-3.0-only
# Package.cmake — the Max package Max itself would download: package-info.json,
# externals/, help/, docs/, examples/ in build/package/sc3-tilde.
#
# Staging only: nothing here goes to git (build/ is ignored). Installing the
# staged folder into ~/Documents/Max 9/Packages is a separate, deliberate step
# (`cmake --build build --target install-package`), because Max only loads one
# copy of each object.

set(SC3_PKG_NAME "sc3-tilde")
set(SC3_PKG_STAGE "${CMAKE_BINARY_DIR}/package/${SC3_PKG_NAME}")
set(SC3_MXO_DIR "${CMAKE_BINARY_DIR}/external")
set(SC_CODE_MXO_DIR "${CMAKE_BINARY_DIR}/external/sc_code")

# Configured outside the stage folder: staging wipes it before copying.
configure_file("${CMAKE_SOURCE_DIR}/cmake/package-info.json.in"
               "${CMAKE_BINARY_DIR}/package-info.json" @ONLY)

add_custom_target(package
  COMMAND "${CMAKE_COMMAND}"
          -DSC3_PKG_STAGE=${SC3_PKG_STAGE}
          -DSC3_PKG_METADATA=${CMAKE_BINARY_DIR}/package-info.json
          -DSC3_MXO=${SC3_MXO_DIR}/sc3~.mxo
          -DSC_CODE_MXO=${SC_CODE_MXO_DIR}/sc_code.mxo
          -DROOT=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/cmake/stage_package.cmake
  DEPENDS sc3_tilde sc_code
  COMMENT "staging ${SC3_PKG_STAGE}"
  VERBATIM)

add_custom_target(install-package
  COMMAND "${CMAKE_COMMAND}"
          -DSTAGE=${SC3_PKG_STAGE}
          -DDEST=$ENV{HOME}/Documents/Max 9/Packages/${SC3_PKG_NAME}
          -P ${CMAKE_SOURCE_DIR}/cmake/install_package.cmake
  DEPENDS package
  COMMENT "installing the staged package in ~/Documents/Max 9/Packages"
  VERBATIM)

# The layout is the one Max reads: a missing folder is a silent "no such object".
# ctest cannot depend on a build target, so the test stages and then verifies.
add_test(NAME package.layout
  COMMAND sh -c
    "${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package >/dev/null &&
     ${CMAKE_COMMAND} -DSTAGE=${SC3_PKG_STAGE} -P ${CMAKE_SOURCE_DIR}/cmake/check_package.cmake")
