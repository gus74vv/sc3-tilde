# SPDX-License-Identifier: GPL-3.0-only
# install_package.cmake — copies the staged package into the Max Packages folder.
#
# Deliberate step (not part of the build): Max loads the objects of every package
# in that folder, so a stale copy there shadows the one you are testing, and a
# second copy of the same object in the search path is a load error.
#
# The destination is composed here on purpose: an argument holding a path with a
# space ("Max 9") is split by the generator, so the only thing passed in is the
# package name. MAX_PACKAGES_DIR overrides the folder.

if(NOT DEFINED STAGE OR NOT EXISTS "${STAGE}/package-info.json")
  message(FATAL_ERROR "install_package.cmake: nothing staged at ${STAGE} (target: package)")
endif()
if(NOT DEFINED PKG_NAME OR PKG_NAME STREQUAL "")
  message(FATAL_ERROR "install_package.cmake: missing -DPKG_NAME")
endif()

if(DEFINED ENV{MAX_PACKAGES_DIR})
  set(_packages "$ENV{MAX_PACKAGES_DIR}")
else()
  set(_packages "$ENV{HOME}/Documents/Max 9/Packages")
endif()
set(_dest "${_packages}/${PKG_NAME}")

if(EXISTS "${_dest}")
  file(REMOVE_RECURSE "${_dest}")
endif()
file(MAKE_DIRECTORY "${_dest}")
file(COPY "${STAGE}/" DESTINATION "${_dest}")
message(STATUS "installed ${_dest} (restart Max)")
