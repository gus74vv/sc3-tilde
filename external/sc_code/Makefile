# SPDX-License-Identifier: GPL-3.0-only
# Makefile — external `sc_code` (in-patch code editor for `sc3~`).
#
# Phases (SCODE.md §8):
#   K0  make spike / make install-spike   → keylog.mxo (key spike, NOT distributed)
#   S0  make test                         → lexer_test + region_test (headless, no Max)
#   S1  make test                         → + buffer_test (UTF-8 buffer/caret/undo)
#       make bundle / make install        → sc_code.mxo (jbox + paint + typing + evalfile)
#
# `make test` needs neither SuperCollider nor Max: lexer, regions and buffer are pure C++.

MAX_SDK ?= $(HOME)/Documents/Max 9/Packages/max-sdk/source/max-sdk-base
MAXINC  := $(MAX_SDK)/c74support/max-includes
MAX_LIBRARY ?= $(HOME)/Documents/Max 9/Library

CXX      := c++
# -Wno-cast-function-type-mismatch: class_addmethod's `(method)` casts look like that in the Max API
CXXFLAGS := -std=c++17 -O2 -g -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch
SC_CODE_ARCH  ?= arm64
SC_CODE_MINOS ?= 11.0
ARCHFLAGS := -arch $(SC_CODE_ARCH) -mmacosx-version-min=$(SC_CODE_MINOS)

B := build

CORE_SRC := sc_lexer.cpp sc_region.cpp sc_buffer.cpp sc_indent.cpp sc_scroll.cpp sc_highlight.cpp \
            sc_bracket.cpp sc_wrap.cpp sc_blink.cpp sc_snip.cpp sc_path.cpp sc_index.cpp \
            sc_call.cpp sc_bar.cpp sc_complete.cpp
CORE_HDR := sc_lexer.h sc_region.h sc_buffer.h sc_indent.h sc_scroll.h sc_highlight.h sc_bracket.h \
            sc_wrap.h sc_blink.h sc_snip.h sc_path.h sc_index.h sc_call.h sc_bar.h sc_complete.h
# Pure C++ for the tests; the clipboard (ObjC++) only goes into the bundle.
UI_SRC := sc_code.cpp sc_clipboard.mm sc_filedialog.mm
UI_HDR := sc_code.cpp sc_clipboard.h sc_filedialog.h

.PHONY: all index-real test spike install-spike bundle install install-help smoke testpatch clean uninstall uninstall-spike \
        snapshot install-stable index-real

all: test

# --- S0: tests headless (gate) ------------------------------------------------
$(B)/lexer_test: tests/lexer_test.cpp $(CORE_SRC) $(CORE_HDR)
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/lexer_test.cpp $(CORE_SRC) -o $@

$(B)/region_test: tests/region_test.cpp $(CORE_SRC) $(CORE_HDR)
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/region_test.cpp $(CORE_SRC) -o $@

test: $(B)/lexer_test $(B)/region_test $(B)/buffer_test $(B)/indent_test $(B)/scroll_test \
      $(B)/highlight_test $(B)/bracket_test $(B)/wrap_test $(B)/blink_test $(B)/snip_test \
      $(B)/path_test $(B)/index_test $(B)/call_test $(B)/bar_test $(B)/complete_test
	./$(B)/lexer_test
	./$(B)/region_test
	./$(B)/buffer_test
	./$(B)/indent_test
	./$(B)/scroll_test
	./$(B)/highlight_test
	./$(B)/bracket_test
	./$(B)/wrap_test
	./$(B)/blink_test
	./$(B)/snip_test
	./$(B)/path_test
	./$(B)/index_test
	./$(B)/call_test
	./$(B)/bar_test
	./$(B)/complete_test

# S6c+: candidate engine of the completion session (text + caret -> candidate).
$(B)/complete_test: tests/complete_test.cpp sc_complete.cpp sc_complete.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/complete_test.cpp sc_complete.cpp -o $@

# S6c: bottom bar — geometry and text layout in monospace cells — pure C++.
$(B)/bar_test: tests/bar_test.cpp sc_bar.cpp sc_bar.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/bar_test.cpp sc_bar.cpp -o $@

# S6c: method call context (which `Class.method(` contains the caret) — pure C++.
$(B)/call_test: tests/call_test.cpp sc_call.cpp sc_call.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/call_test.cpp sc_call.cpp -o $@

# S6c: class library index (parse + query + sclang generator) — pure C++, no Max.
$(B)/index_test: tests/index_test.cpp sc_index.cpp sc_index.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/index_test.cpp sc_index.cpp -o $@

# S6c §10.5: write a REAL dump with sclang and check the parser on it (~32k entries).
# sclang does not exit after running a file (it stays as a REPL), so the dump is
# polled for its #end marker and the process is killed: the file is the gate.
SCLANG ?= /Applications/SuperCollider.app/Contents/MacOS/sclang
INDEX  ?= /tmp/sc_code-index.txt
index-real: $(B)/index_test
	./$(B)/index_test --generator "$(INDEX)" > $(B)/index_gen.scd
	@rm -f "$(INDEX)"
	@"$(SCLANG)" $(B)/index_gen.scd < /dev/null > $(B)/index_gen.log 2>&1 & \
	  pid=$$!; for i in $$(seq 1 160); do \
	    if [ -s "$(INDEX)" ] && [ "$$(tail -1 "$(INDEX)")" = "#end" ]; then break; fi; \
	    sleep 0.25; \
	  done; kill $$pid 2>/dev/null; wait $$pid 2>/dev/null; true
	@grep "sc_code index" $(B)/index_gen.log || echo "index: sclang wrote nothing (see $(B)/index_gen.log)"
	./$(B)/index_test -v "$(INDEX)"

# S7 (0.9.5): drop-path formatting — pure strings, no Max (SCODE.md §12).
$(B)/path_test: tests/path_test.cpp sc_path.cpp sc_path.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/path_test.cpp sc_path.cpp -o $@
# S5e: visual wrap (needs the buffer for the cells/code points).
$(B)/wrap_test: tests/wrap_test.cpp sc_wrap.cpp sc_wrap.h sc_buffer.cpp sc_buffer.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/wrap_test.cpp sc_wrap.cpp sc_buffer.cpp -o $@

# S5e: evaluation blink = pure time curve.
$(B)/blink_test: tests/blink_test.cpp sc_blink.cpp sc_blink.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/blink_test.cpp sc_blink.cpp -o $@

# S6: snippets file (`### key` + verbatim body) and `snip.KEY` matching.
# The replacement in the buffer is tested right here (buffer_test already covers edits).
$(B)/snip_test: tests/snip_test.cpp sc_snip.cpp sc_snip.h sc_buffer.cpp sc_buffer.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/snip_test.cpp sc_snip.cpp sc_buffer.cpp -o $@

# S5c: bracket matching for the caret (needs lexer + regions, no Max).
$(B)/bracket_test: tests/bracket_test.cpp sc_bracket.cpp sc_bracket.h $(CORE_SRC) $(CORE_HDR)
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/bracket_test.cpp sc_bracket.cpp sc_region.cpp sc_lexer.cpp -o $@

# --- S1: buffer (headless) ----------------------------------------------------
$(B)/buffer_test: tests/buffer_test.cpp sc_buffer.cpp sc_buffer.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/buffer_test.cpp sc_buffer.cpp -o $@

# S3: indentation = port of the IDE engine (needs the lexer, not the buffer).
$(B)/indent_test: tests/indent_test.cpp sc_indent.cpp sc_indent.h $(CORE_SRC) $(CORE_HDR)
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/indent_test.cpp sc_indent.cpp sc_lexer.cpp -o $@

# S5 (brought forward): vertical scroll = pure geometry, no buffer, no lexer.
$(B)/scroll_test: tests/scroll_test.cpp sc_scroll.cpp sc_scroll.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/scroll_test.cpp sc_scroll.cpp -o $@

# S4: per-line token cache (needs the lexer and the buffer, not the Max object).
$(B)/highlight_test: tests/highlight_test.cpp sc_highlight.cpp sc_highlight.h $(CORE_SRC) $(CORE_HDR)
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) tests/highlight_test.cpp sc_highlight.cpp sc_lexer.cpp sc_buffer.cpp -o $@

# --- S1: bundle `sc_code.mxo` --------------------------------------------------
# Same scheme as keylog (DIRECTORY bundle + ad-hoc signature + jpatcher_syms.o),
# but the code is the full editor. `sc_code` is pure UI: it links neither SC nor MSP
# (every Max symbol is resolved by the host with -undefined dynamic_lookup).
# Bundle version (keep it in sync with SC_CODE_VERSION in sc_code.cpp).
SC_CODE_VER := 0.9.10

$(B)/sc_code.mxo: $(UI_SRC) $(UI_HDR) bundle/Info.plist.in $(CORE_SRC) $(CORE_HDR) $(B)/jpatcher_syms.o
	@mkdir -p $(B)/sc_code.mxo/Contents/MacOS
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) -I"$(MAXINC)" \
	    -bundle -undefined dynamic_lookup \
	    $(UI_SRC) $(CORE_SRC) $(B)/jpatcher_syms.o \
	    -framework AppKit \
	    -framework UniformTypeIdentifiers \
	    -o $(B)/sc_code.mxo/Contents/MacOS/sc_code
	@rm -rf $(B)/sc_code.mxo/Contents/MacOS/sc_code.dSYM
	sed -e "s/@VERSION@/$(SC_CODE_VER)/g" bundle/Info.plist.in > $(B)/sc_code.mxo/Contents/Info.plist
	printf 'iLaXmax2' > $(B)/sc_code.mxo/Contents/PkgInfo
	codesign --force --deep --sign - $(B)/sc_code.mxo 2>/dev/null || \
	  echo "WARNING: codesign failed — Max may reject the bundle"
	codesign --verify --strict $(B)/sc_code.mxo

bundle: $(B)/sc_code.mxo

install: $(B)/sc_code.mxo install-help
	@mkdir -p "$(MAX_LIBRARY)"
	rm -rf "$(MAX_LIBRARY)/sc_code.mxo"
	cp -R $(B)/sc_code.mxo "$(MAX_LIBRARY)/sc_code.mxo"
	codesign --verify --deep --strict "$(MAX_LIBRARY)/sc_code.mxo"
	@echo "installed: $(MAX_LIBRARY)/sc_code.mxo"

# Help: Max looks it up by name in the search path (alt-click on the object).
# That folder is `help` inside a search path, i.e. ~/Documents/Max 9/Library/help;
# ~/Documents/Max 9/Help is NOT in the search path (see README, install step 2).
MAX_HELP ?= $(MAX_LIBRARY)/help

install-help:
	@mkdir -p "$(MAX_HELP)"
	@if [ -f "$(MAX_HELP)/sc_code.maxhelp" ] && [ "$(MAX_HELP)/sc_code.maxhelp" -nt "help/sc_code.maxhelp" ]; then \
		echo "NOT copied: $(MAX_HELP)/sc_code.maxhelp is newer than help/sc_code.maxhelp"; \
		echo "            (did you edit it in Max? copy it into the repo and install again)"; \
	else \
		cp "help/sc_code.maxhelp" "$(MAX_HELP)/sc_code.maxhelp"; \
		echo "installed: $(MAX_HELP)/sc_code.maxhelp"; \
	fi

uninstall:
	rm -rf "$(MAX_LIBRARY)/sc_code.mxo" "$(MAX_HELP)/sc_code.maxhelp"

# --- two versions side by side: 0.8.1 (WITHOUT autocomplete) vs the current one ---
# Max only loads ONE `sc_code.mxo` per Library, so "having two versions" means
# switching which one is installed. `backup/v0.8.1/sc_code-0.8.1.mxo` is the frozen
# copy (that version's sources are also in the repo tag `sc_code-0.8.1`).
STABLE_VER := 0.8.1
STABLE_MXO := backup/v$(STABLE_VER)/sc_code-v$(STABLE_VER).mxo
STABLE_DIR := backup/v$(STABLE_VER)

# Saves the INSTALLED bundle as a frozen copy of version `$(STABLE_VER)`.
snapshot:
	@test -d "$(MAX_LIBRARY)/sc_code.mxo" || { echo "no bundle installed"; exit 1; }
	@mkdir -p "$(STABLE_DIR)"
	rm -rf "$(STABLE_DIR)/sc_code-v$(STABLE_VER).mxo"
	cp -R "$(MAX_LIBRARY)/sc_code.mxo" "$(STABLE_DIR)/sc_code-v$(STABLE_VER).mxo"
	@echo "frozen: $(STABLE_MXO)"

# Goes back to the version WITHOUT autocomplete (restart Max afterwards: .mxo files
# are loaded when the app starts).
install-stable:
	@test -d "$(STABLE_MXO)" || { echo "missing $(STABLE_MXO) (run `make snapshot` with 0.8.1 installed)"; exit 1; }
	@mkdir -p "$(MAX_LIBRARY)"
	rm -rf "$(MAX_LIBRARY)/sc_code.mxo"
	cp -R "$(STABLE_MXO)" "$(MAX_LIBRARY)/sc_code.mxo"
	codesign --verify --deep --strict "$(MAX_LIBRARY)/sc_code.mxo"
	@echo "installed: $(MAX_LIBRARY)/sc_code.mxo ($(STABLE_VER), no autocomplete)"
	@echo "restart Max so it gets loaded"

# GUI gate (S1): installs the bundle and opens the smoke patch (script: tests/SMOKE.md).
smoke: install
	open tests/smoke.maxpat

# Test patch (double click): preloaded code to exercise eval, bracket matching,
# the title bar and the context menu.
testpatch: install
	open tests/sc_code-test.maxpat

# --- K0: key spike (minimal jbox, no SC, no MSP) -------------------------------
# Every Max symbol (jbox_*, jgraphics_*, class_*) is resolved by the host when it
# loads the bundle ⇒ -bundle -undefined dynamic_lookup and NO frameworks at all.
# It is built as a DIRECTORY bundle (Contents/MacOS + Info.plist), just like
# sc3~: Max 9 validates with SecStaticCode and a loose .mxo with no attached
# Info.plist is exactly the case it rejects (`cannot be loaded due to system security policy`).
KEYLOG_VER := 0.1.0

# jpatcher_syms.c: the SDK requires it for UI objects. jbox_new() uses dozens of
# static t_symbol* (ps_rect, ps_juiobject, …) filled in by jpatcher_syms_init();
# the .c has to be COMPILED INTO the bundle (Max does not export them) and called
# from ext_main. Without it: SIGSEGV inside jbox_new when creating the box (bug #17).
JPATCHER_SYMS := $(MAXINC)/common/jpatcher_syms.c
CC := cc

# Careful: the SDK path has a space in it ("Max 9") ⇒ it CANNOT be used as a make
# prerequisite (it gets split into words). Rule with no prerequisites: it is
# compiled only if the .o does not exist, and the recipe is quoted.
$(B)/jpatcher_syms.o:
	@mkdir -p $(B)
	$(CC) -O2 $(ARCHFLAGS) -I"$(MAXINC)" -c "$(JPATCHER_SYMS)" -o $@

$(B)/keylog.mxo: spike/keylog.cpp spike/Info.plist.in $(B)/jpatcher_syms.o
	@mkdir -p $(B)/keylog.mxo/Contents/MacOS
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) -I"$(MAXINC)" \
	    -bundle -undefined dynamic_lookup \
	    spike/keylog.cpp $(B)/jpatcher_syms.o -o $(B)/keylog.mxo/Contents/MacOS/keylog
	@rm -rf $(B)/keylog.mxo/Contents/MacOS/keylog.dSYM   # -g deja el dSYM al lado del binario
	sed -e "s/@VERSION@/$(KEYLOG_VER)/g" spike/Info.plist.in > $(B)/keylog.mxo/Contents/Info.plist
	printf 'iLaXmax2' > $(B)/keylog.mxo/Contents/PkgInfo
	codesign --force --deep --sign - $(B)/keylog.mxo 2>/dev/null || \
	  echo "WARNING: codesign failed — Max may reject the bundle"
	codesign --verify --strict $(B)/keylog.mxo

spike: $(B)/keylog.mxo

install-spike: $(B)/keylog.mxo
	@mkdir -p "$(MAX_LIBRARY)"
	rm -rf "$(MAX_LIBRARY)/keylog.mxo"
	cp -R $(B)/keylog.mxo "$(MAX_LIBRARY)/keylog.mxo"
	codesign --verify --deep --strict "$(MAX_LIBRARY)/keylog.mxo"
	@echo "instalado: $(MAX_LIBRARY)/keylog.mxo"

uninstall-spike:
	rm -rf "$(MAX_LIBRARY)/keylog.mxo"

clean:
	rm -rf $(B)
