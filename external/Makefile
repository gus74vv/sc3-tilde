# SPDX-License-Identifier: GPL-3.0-only
# Makefile — external sc3~: .mxo bundle (Phase 2.3) + headless tests.
#
#   make                                  # build/sc3~ (the bundle binary)
#   make test                             # headless gate (engine_test)
#   ./build.sh                            # uses `make build/sc3~` + builds the .mxo
#
# The SuperCollider flags (includes + statics, verified order) live in sc.mk,
# shared with build.sh.

include sc.mk

MAX_SDK ?= $(HOME)/Documents/Max 9/Packages/max-sdk/source/max-sdk-base
MAXINC  := $(MAX_SDK)/c74support/max-includes
MSPINC  := $(MAX_SDK)/c74support/msp-includes

CXX       := c++
CXXFLAGS  := -stdlib=libc++ -O2 -g -std=gnu++17
SC3_ARCH  ?= arm64
SC3_MINOS ?= 11.0
ARCHFLAGS := -arch $(SC3_ARCH) -mmacosx-version-min=$(SC3_MINOS)

WARN := -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch

CLDIR := build/cl
BIN   := build/sc3~

# --- libsndfile copied from SC.app (Phase 4 § 4.1) -----------------------------
# Generated here (not versioned): the .mxo and the headless tests share the SAME
# copy with install_name @loader_path/../Frameworks/... (see sc.mk). The dylib in
# SC.app is signed with a Developer ID; `install_name_tool` invalidates that
# signature, so the copy is re-signed ad-hoc (otherwise the hardened runtime
# rejects the dlopen with "code signature invalid").
sndfile: $(SNDFILE)

$(SNDFILE):
	@test -f "$(SNDFILE_SRC)" || { \
	  echo "ERROR: missing $(SNDFILE_SRC)" >&2; \
	  echo "       An installed SuperCollider 3.14.1 is a PREREQUISITE (decision 11)." >&2; \
	  echo "       SC_APP=<other path> make ..." >&2; exit 1; }
	@mkdir -p "$(SNDFILE_DIR)"
	cp "$(SNDFILE_SRC)" $@
	install_name_tool -id "$(SNDFILE_ID)" $@
	@codesign --force --sign - $@ 2>/dev/null || \
	  echo "WARNING: ad-hoc codesign failed on $@"

# Sanitizer binaries live in build/san/<san>/ ⇒ their @loader_path/../Frameworks is
# build/san/Frameworks (one level above the regular tests).
SANROOT := build/san
SANLINK := $(SANROOT)/Frameworks/$(SNDFILE_NAME)
$(SANLINK): $(SNDFILE)
	@mkdir -p $(dir $@)
	ln -sf ../../../Frameworks/$(SNDFILE_NAME) $@

.PHONY: all test gate sc_code_bundle coexist sc_code_test stress engine_test multi_test input_test loop_test fail_test evalcheck compat clean bundle sndfile

all: $(BIN)

# --- bundle binary -----------------------------------------------------------
# Same SC statics as the headless test; the remaining symbols (Max API) are
# resolved by the host when it loads the bundle (dynamic_lookup).
$(BIN): sc3_tilde.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h sc.mk $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(WARN) \
	    -I"$(MAXINC)" -I"$(MSPINC)" $(SC_CXX_INCLUDES) \
	    $(EXTRA_CXXFLAGS) \
	    -bundle -undefined dynamic_lookup \
	    -F"$(MSPINC)" -framework MaxAudioAPI \
	    sc3_tilde.cpp Sc3Engine.cpp $(SC_LIBS) $(EXTRA_LDFLAGS) \
	    -o $@

# --- headless gate (no Max) ---------------------------------------------------
# By default it uses the class library of the real SC installation
# (/Applications/SuperCollider.app) + the SCMaxCompat module from phase1.
engine_test: tests/engine_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) $(EXTRA_CXXFLAGS) \
	    tests/engine_test.cpp Sc3Engine.cpp -o build/engine_test $(SC_LIBS) $(EXTRA_LDFLAGS)

# Runs an arbitrary .scd in the headless engine and measures whether it sounds.
evalcheck: tests/eval_check.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/eval_check.cpp Sc3Engine.cpp -o build/eval_check $(SC_LIBS)

# Bug #29 (diagnostic, not part of the gate): evaluates a LIST of .scd files in the
# embedded engine and says whether each one returned or left sclang stuck. The
# engine's eval is not preemptible, so the only way to tell "still running" from
# "looping forever" is a canary eval behind it (`info`'s busy= uses the same data).
#   ./build/tvar_repro a.scd b.scd …
tvarrepro: tests/tvar_repro.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/tvar_repro.cpp Sc3Engine.cpp -o build/tvar_repro $(SC_LIBS)

# Alternative fixture (SC3_CL_DIR=build/cl) to compare against stock+copy.
compat:
	rm -rf "$(CLDIR)"
	mkdir -p "$(CLDIR)"
	cp -R "$(SC)/SCClassLibrary" "$(CLDIR)/SCClassLibrary"
	cp -R ../phase1/compat/SCMaxCompat "$(CLDIR)/SCClassLibrary/SCMaxCompat"

# 3.2: the gate runs twice — 48 kHz (s.options default) and 44.1 kHz (like a Max
# project at 44.1) — to verify the World is born at the requested sr and that the
# 440 Hz tone still measures 440 (tuning, not 404 from an 8.8% shift).
# sc_code (Phase 6) joins the gate with its headless tests: lexer/regions are pure
# C++ (no SC, no Max), so they run first. The engine tests use port 0 = AUTO (4.x),
# so Max and the SuperCollider IDE no longer need to be closed.
# 4.x: with 57110 taken (scsynth/IDE, no SO_REUSEADDR) the boot has to go to an
# auto port and end up ready with sclang pointing there.
coexist: engine_test
	python3 tests/coexist_check.py ./build/engine_test

test: sc_code_test engine_test multi_test input_test fail_test
	./build/engine_test
	SC3_SR=44100 ./build/engine_test
	./build/multi_test
	./build/input_test
	./build/fail_test

# Closing gate. The headless tests above do NOT compile the sources of the two
# bundles (sc3_tilde.cpp, sc_code.cpp), so for a while a broken comment block in
# sc_code.cpp shipped with every test green: the .mxo was simply never rebuilt.
# The bundles are compiled here (they need the Max SDK; the tests do not).
gate: test all sc_code_bundle
	@echo "gate: tests + both bundles OK"

sc_code_bundle:
	$(MAKE) --no-print-directory -C sc_code bundle

# sc_code S0: tokens and regions 1:1 with the SC IDE (port of SCODE.md §3.1/§3.4).
sc_code_test:
	$(MAKE) --no-print-directory -C sc_code test

# 3.5: multi-instance — two cursors on the same World, per-bus isolation, distinct
# windows, silence of untouched buses and underruns.
multi_test: tests/multi_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) $(EXTRA_CXXFLAGS) \
	    tests/multi_test.cpp Sc3Engine.cpp -o build/multi_test $(SC_LIBS) $(EXTRA_LDFLAGS)

# 3.8: audio input — SoundIn.ar(i) reads what is pushed into inlet i
# (exact mapping, 2-block prefill, single producer, underrun ⇒ silence).
input_test: tests/input_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) $(EXTRA_CXXFLAGS) \
	    tests/input_test.cpp Sc3Engine.cpp -o build/input_test $(SC_LIBS) $(EXTRA_LDFLAGS)

# 3.4: failure matrix (one case per process: broken Quark, throwing startup,
# corrupt .scx, sclang port ≠ World port, impossible boot, eval that throws).
fail_test: tests/fail_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) $(EXTRA_CXXFLAGS) \
	    tests/fail_test.cpp Sc3Engine.cpp -o build/fail_test $(SC_LIBS) $(EXTRA_LDFLAGS)

# 2.5: Phase 2 stress — N open/boot/audio/release cycles measuring fds, threads
# and RSS. Alternates the World owner (renderBlock / pumpWhenIdle) and the sr.
loop_test: tests/loop_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) $(EXTRA_CXXFLAGS) \
	    tests/loop_test.cpp Sc3Engine.cpp -o build/loop_test $(SC_LIBS) $(EXTRA_LDFLAGS)

stress: loop_test
	./build/loop_test $(STRESS_PORT)

# Sanitizers (debt 4 of the handoff). ASan and TSan cannot be mixed in the same
# binary, so each one gets its own dir:
# build/san/<asan|tsan>/{engine_test,multi_test,loop_test}.
# The SC libs are not instrumented (vendored) ⇒ ASan/TSan still see the heap and
# the global mmaps, but they do not instrument SC code: a report inside vendor/ is
# read as "suspicious", not as absolute truth.
#   make asan   ASAN_OPTIONS=detect_leaks=0 ./build/san/asan/engine_test
#   make tsan   ./build/san/tsan/engine_test
SANITIZE ?=
SANDIR   := build/san/$(SANITIZE)
SANFLAGS := $(if $(SANITIZE),-fsanitize=$(SANITIZE) -fno-omit-frame-pointer -g -O1)

.PHONY: asan tsan san-build
asan:
	@$(MAKE) --no-print-directory san-build SANITIZE=address SANDIR=build/san/asan
tsan:
	@$(MAKE) --no-print-directory san-build SANITIZE=thread SANDIR=build/san/tsan

san-build: $(SANDIR)/engine_test $(SANDIR)/multi_test $(SANDIR)/input_test $(SANDIR)/fail_test $(SANDIR)/loop_test
	@echo "done: $(SANDIR)/{engine_test,multi_test,input_test,fail_test,loop_test}"

$(SANDIR)/engine_test: tests/engine_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE) $(SANLINK)
	@mkdir -p $(SANDIR)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/engine_test.cpp Sc3Engine.cpp -o $@ $(SC_LIBS) $(SANFLAGS)

$(SANDIR)/multi_test: tests/multi_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE) $(SANLINK)
	@mkdir -p $(SANDIR)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/multi_test.cpp Sc3Engine.cpp -o $@ $(SC_LIBS) $(SANFLAGS)

$(SANDIR)/input_test: tests/input_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE) $(SANLINK)
	@mkdir -p $(SANDIR)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/input_test.cpp Sc3Engine.cpp -o $@ $(SC_LIBS) $(SANFLAGS)

$(SANDIR)/fail_test: tests/fail_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE) $(SANLINK)
	@mkdir -p $(SANDIR)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/fail_test.cpp Sc3Engine.cpp -o $@ $(SC_LIBS) $(SANFLAGS)

$(SANDIR)/loop_test: tests/loop_test.cpp Sc3Engine.cpp Sc3Engine.h Sc3LangClient.h $(SNDFILE) $(SANLINK)
	@mkdir -p $(SANDIR)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ARCHFLAGS) $(SC_CXX_INCLUDES) \
	    tests/loop_test.cpp Sc3Engine.cpp -o $@ $(SC_LIBS) $(SANFLAGS)

clean:
	rm -rf build/sc3~ build/engine_test build/multi_test build/input_test build/loop_test build/fail_test \
	    build/engine_test_out.wav build/san Frameworks
	    build/engine_test_out.wav build/san Frameworks
