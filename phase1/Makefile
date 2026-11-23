# SPDX-License-Identifier: GPL-3.0-only
# Makefile — prototype: sclang + World embedded headless (bare binary).
#
# Builds test_lang (sclang) and test_world (sclang + World + audio).
# The binary is "bare" → it needs SCClassLibrary in its dir (see the symlink rule).
#
# Links against the static build of vendor/supercollider (tag Version-3.14.1).
# The order of the statics matters; macOS frameworks + libsndfile (Homebrew).

SC    := ../vendor/supercollider
B     := $(SC)/build
SDK   := /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

CXX      := c++
CXXFLAGS := -stdlib=libc++ -O2 -g -std=gnu++17 -arch arm64 -mmacosx-version-min=11.0

CXX_INCLUDES := \
    -I$(SC)/build/common \
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

# Statics (same order as build/lang/.../sclang.dir/link.txt) + frameworks.
LIBS := \
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
    /opt/homebrew/lib/libsndfile.dylib \
    $(B)/external_libraries/libyaml.a

.PHONY: all test run clean symlink compat symlink-stock

all: test_lang test_world

test_lang: test_lang.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) test_lang.cpp -o test_lang $(LIBS)

test_world: test_world.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) test_world.cpp -o test_world $(LIBS)

diag: diag.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) diag.cpp -o diag $(LIBS)

diag_osc: diag_osc.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) diag_osc.cpp -o diag_osc $(LIBS)

diag_quarks: diag_quarks.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) -I$(SC)/lang/LangSource diag_quarks.cpp -o diag_quarks $(LIBS)

diag_boot: diag_boot.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) diag_boot.cpp -o diag_boot $(LIBS)

diag_notify: diag_notify.cpp TestClient.h
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) diag_notify.cpp -o diag_notify $(LIBS)

# Class library for the binary: stock + the Qt-free compatibility module.
# (The "bare" binary resolves Resource = executable dir → it looks for ./SCClassLibrary.)
# A copy is assembled in $(CLDIR) because the compat module is added INSIDE the
# class library tree (so it compiles before Process:startup/initClassTree).
CLDIR := ../build/cl
compat: 
	rm -rf "$(CLDIR)"
	mkdir -p "$(CLDIR)"
	cp -R "$(SC)/SCClassLibrary" "$(CLDIR)/SCClassLibrary"
	cp -R compat/SCMaxCompat "$(CLDIR)/SCClassLibrary/SCMaxCompat"
	ln -sfn "$$(cd "$(CLDIR)" && pwd)/SCClassLibrary" SCClassLibrary

# Minimal variant: stock only (to reproduce the behaviour WITHOUT compat).
symlink-stock:
	ln -sfn "$(SC)/SCClassLibrary" SCClassLibrary



test: all compat
	./test_lang
	./test_world

clean:
	rm -f test_lang test_world *.o
