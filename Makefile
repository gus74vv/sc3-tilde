# SPDX-License-Identifier: GPL-3.0-only
ROOTDIR := $(shell pwd)
SRCDIR := $(ROOTDIR)/source
PROJECTS := $(SRCDIR)/projects
PKG_NAME = sc3-tilde
MAX_VERSIONS := 8 9

# SuperCollider statics that sc3~ links (see source/projects/sc3_tilde/CMakeLists.txt).
# SuperCollider.app whose runtime files are bundled into sc3~.mxo: the local install if
# present, else the standard location (CI). `make SC_APP=<path>` overrides it.
SC_APP ?= $(firstword $(wildcard /Applications/Studio/supercollider/SuperCollider.app) \
                      /Applications/SuperCollider.app)
# Max.app for the symbol checks in `make test`; same lookup.
MAX_APP ?= $(firstword $(wildcard /Applications/Studio/Max.app) /Applications/Max.app)
SC_TAG := Version-3.14.1
SC_DIR := $(ROOTDIR)/vendor/supercollider

define section
	@echo "== $(1)"
endef

.PHONY: all build test clean reset setup update-submodules link sc

all: build

build:
	@mkdir -p build && cd build && \
		cmake .. -DSC_APP="$(SC_APP)" -DMAX_APP="$(MAX_APP)" && \
		cmake --build . --config Release

test: build
	@cd build && ctest -C Release --output-on-failure

clean:
	@rm -rf externals/*.mxo

reset: clean
	@rm -rf build

setup: update-submodules sc link
	$(call section,"setup complete")

update-submodules:
	$(call section,"updating git submodules")
	@git submodule update --init

# Clone and build the pinned SuperCollider statics once. Not part of `build`: it takes
# minutes. Only the libraries sc3~ links are built, not sclang/scsynth or the plugins.
sc: $(SC_DIR)/build/lang/libsclang.a

$(SC_DIR)/build/lang/libsclang.a:
	$(call section,"building SuperCollider $(SC_TAG) statics")
	@$(SRCDIR)/scripts/build_sc.sh "$(SC_TAG)" "$(SC_DIR)"

link:
	$(call section,"symlink to Max 'Packages' Directories")
	@for MAX_VERSION in $(MAX_VERSIONS); do \
		MAX_DIR="Max $${MAX_VERSION}" ; \
		PACKAGES="$(HOME)/Documents/$${MAX_DIR}/Packages" ; \
		PKG_LINK="$${PACKAGES}/$(PKG_NAME)" ; \
		if [ -d "$${PACKAGES}" ]; then \
			echo "symlinking to $${PKG_LINK}" ; \
			if ! [ -L "$${PKG_LINK}" ]; then \
				ln -s "$(ROOTDIR)" "$${PKG_LINK}" ; \
				echo "... symlink created" ; \
			else \
				echo "... symlink already exists" ; \
			fi \
		fi \
	done
