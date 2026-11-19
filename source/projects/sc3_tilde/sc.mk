# SPDX-License-Identifier: GPL-3.0-only
# sc.mk — flags of the vendored SuperCollider (tag Version-3.14.1), shared
# by the Makefile (headless gate) and by build.sh (.mxo bundle). A single source
# of truth for includes and statics: the order of the statics matters (same as
# phase1/Makefile, verified there).
#
# Requiere vendoreado ya compilado: ../vendor/supercollider/build.

SC  := $(abspath ../vendor/supercollider)
B   := $(SC)/build
SDK := /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

# --- libsndfile: NOT taken from Homebrew (Phase 4 § 4.1) -----------------------
# The only non-Apple dep of `libscsynth.a` (DiskIO) is libsndfile. Instead of
# /opt/homebrew/lib/libsndfile.dylib the one SC.app ships is used (2.37.0, the
# same version the engine is compiled with): it is copied to `Frameworks/` and
# given `install_name @loader_path/../Frameworks/libsndfile.1.dylib`.
#
# Why it works in both layouts without touching anything else:
#   * bundle:  sc3~.mxo/Contents/MacOS/sc3~   → @loader_path/../Frameworks = Contents/Frameworks ✓
#   * headless: external/build/engine_test    → @loader_path/../Frameworks = external/Frameworks ✓
# So the headless gate (which links the SAME scsynth static) proves at
# runtime the resolution of the `@loader_path` the bundle uses.
# The sanitizer binaries live in build/san/<san>/ (one level below): the
# Makefile leaves them a link in build/san/Frameworks.
SC_APP       ?= /Applications/SuperCollider.app
SNDFILE_NAME := libsndfile.1.dylib
SNDFILE_DIR  := $(abspath Frameworks)
SNDFILE      := $(SNDFILE_DIR)/$(SNDFILE_NAME)
SNDFILE_SRC  := $(SC_APP)/Contents/Frameworks/libsndfile.dylib
SNDFILE_ID   := @loader_path/../Frameworks/$(SNDFILE_NAME)

SC_CXX_INCLUDES := \
    -I$(B)/common \
    -I$(SC)/include/common \
    -I$(SC)/include/lang \
    -I$(SC)/include/plugin_interface \
    -I$(SC)/include/server \
    -I$(SC)/server/scsynth \
    -I$(SC)/common \
    -I$(SC)/lang/LangSource \
    -I$(SC)/external_libraries/nova-tt \
    -I$(SC)/external_libraries/TLSF-2.4.6/src \
    -isystem $(SC)/external_libraries/boost \
    -isystem $(SC)/external_libraries/boost_sync/include \
    -isystem $(SC)/external_libraries/yaml-cpp/include

SC_LIBS := \
    $(B)/lang/libsclang.a \
    $(B)/external_libraries/hidapi/mac/libhidapi.a \
    $(B)/external_libraries/hidapi/hidapi_parser/libhidapi_parser.a \
    $(B)/external_libraries/libboost_thread_lib.a \
    $(B)/external_libraries/libboost_program_options_lib.a \
    $(B)/external_libraries/libboost_regex_lib.a \
    $(B)/server/scsynth/libscsynth.a \
    $(B)/external_libraries/libtlsf.a \
    $(SDK)/usr/lib/libpthread.tbd \
    $(SDK)/usr/lib/libdl.tbd \
    -framework Accelerate \
    -framework CoreServices \
    -framework Foundation \
    -framework AppKit \
    -framework Carbon \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework IOKit \
    -framework CoreFoundation \
    $(SNDFILE) \
    $(B)/external_libraries/libyaml.a

# Emits the flags so build.sh can eval them (avoids duplicating the list).
.PHONY: print-flags
print-flags:
	@printf "SC_CXX_INCLUDES=%s\n" '$(SC_CXX_INCLUDES)'
	@printf "SC_LIBS=%s\n" '$(SC_LIBS)'
