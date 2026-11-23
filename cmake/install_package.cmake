# SPDX-License-Identifier: GPL-3.0-only
# install_package.cmake — copies the staged package into the Max Packages folder.
#
# Deliberate step (not part of the build): Max loads the objects of every package
# in that folder, so a stale copy there shadows the one you are testing.

if(NOT DEFINED STAGE OR NOT EXISTS "${STAGE}/package-info.json")
  message(FATAL_ERROR "install_package.cmake: nothing staged at ${STAGE} (target: package)")
endif()
if(NOT DEFINED DEST OR DEST STREQUAL "")
  message(FATAL_ERROR "install_package.cmake: missing -DDEST")
endif()

if(EXISTS "${DEST}")
  file(REMOVE_RECURSE "${DEST}")
endif()
file(MAKE_DIRECTORY "${DEST}")
file(COPY "${STAGE}/" DESTINATION "${DEST}")
message(STATUS "installed ${DEST} (restart Max)")
