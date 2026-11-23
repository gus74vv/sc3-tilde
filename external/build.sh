#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# build.sh — builds the external sc3~ as .mxo and installs it in the User Library.
#
# Usage:
#   ./build.sh                  # builds + installs in ~/Documents/Max 9/Library
#   SC3_OUTDIR=/tmp/ext ./build.sh
#   EXTRA_CXXFLAGS=... EXTRA_LDFLAGS=... ./build.sh
#
# CAREFUL: the destination has to be a folder of the Max search path. The one Max
# always adds is "User Library" (~/Documents/Max 9/Library). `Max 9/externals`
# is NOT in the search path ⇒ the object shows up as "No such object".
#
# The .mxo is SELF-CONTAINED (issue #1, item 9): the Max object + Sc3Engine +
# the SuperCollider statics (sclang + scsynth) + the class library, the stock
# .scx, the sounds, the HID tables and libsndfile (scripts/stage_bundle.sh).
# /Applications/SuperCollider.app is only needed AT BUILD TIME, to copy its
# payload; at run time nothing outside the bundle is used. SC3_APP_PATH still
# forces the real installation (development escape hatch, see sc3_tilde.cpp).
#
# Requirements: Xcode CLT, the max-sdk package (max-sdk-base) and a built
# vendor/supercollider. The binary itself is built by `make build/sc3~` (flags in sc.mk).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

MAXSDK="${MAX_SDK:-$HOME/Documents/Max 9/Packages/max-sdk/source/max-sdk-base}"
OUTDIR="${SC3_OUTDIR:-$HOME/Documents/Max 9/Library}"
MINOS="${SC3_MINOS:-11.0}"
ARCH="${SC3_ARCH:-arm64}"
VER="${SC3_VERSION:-0.16.0}"
BUILD="$HERE/build"
BIN="$BUILD/sc3~"
BUNDLE="$OUTDIR/sc3~.mxo"

[[ -d "$MAXSDK/c74support/max-includes" ]] || {
  echo "ERROR: no Max SDK at $MAXSDK (MAX_SDK?)" >&2; exit 1; }
[[ -d ../vendor/supercollider/build ]] || {
  echo "ERROR: missing ../vendor/supercollider/build (build the vendored SC)" >&2; exit 1; }

echo "== sc3~ v$VER"
echo "   SDK : $MAXSDK"
echo "   SC  : ../vendor/supercollider (Version-3.14.1)"
echo "   arch: $ARCH (minos $MINOS)"

# The only non-Apple dep of the engine is libsndfile: it comes from SC.app
# (`Frameworks/`, see sc.mk) and travels into the bundle as
# Contents/Frameworks/libsndfile.dylib (the name the bundled .scx ask for) with
# install_name @loader_path/../Frameworks/libsndfile.dylib
make -s "$HERE/Frameworks/libsndfile.dylib" MAX_SDK="$MAXSDK"

make -s build/sc3~ MAX_SDK="$MAXSDK" SC3_ARCH="$ARCH" SC3_MINOS="$MINOS" \
     EXTRA_CXXFLAGS="${EXTRA_CXXFLAGS:-}" EXTRA_LDFLAGS="${EXTRA_LDFLAGS:-}"

rm -rf "$BUNDLE"
mkdir -p "$BUNDLE/Contents/MacOS"
cp "$BIN" "$BUNDLE/Contents/MacOS/sc3~"
sed -e "s/@VERSION@/$VER/g" Info.plist.in > "$BUNDLE/Contents/Info.plist"
printf 'iLaXmax2' > "$BUNDLE/Contents/PkgInfo"

# Payload (class library, stock .scx, sounds, HID tables, the SCMaxCompat module
# and libsndfile). Same script the CMake POST_BUILD runs: one implementation.
scripts/stage_bundle.sh "$BUNDLE" "$ARCH" "${SC_APP:-/Applications/SuperCollider.app}"

# Help of the object (args, outlets, multi-instance). Max looks for it in the
# "help" folder of the user search path: ~/Documents/Max 9/Library/help/.
HELPDIR="$(dirname "$BUNDLE")/help"
mkdir -p "$HELPDIR"
# The installed copy can be NEWER than the repo one: that happens when the layout is
# fixed inside Max (moving comment boxes) and saved there. Overwriting it silently
# would throw that work away, so warn instead and keep the installed file.
install_help() {
    local src="$1" dst="$HELPDIR/$(basename "$1")"
    if [ -f "$dst" ] && cmp -s "$src" "$dst"; then
        echo "   help -> $dst (already up to date)"
    elif [ -f "$dst" ] && [ "$dst" -nt "$src" ]; then
        echo "   help -> NOT copied: $dst is newer than $src"
        echo "           (did you edit it in Max? copy it into the repo and build again)"
    else
        cp "$src" "$dst"
        echo "   help -> $dst (double click the object or 'Open Help')"
    fi
}
install_help "help/sc3~.maxhelp"
# sc_code is a bundle of its own, but its help patch is wired to sc3~: if it is
# not refreshed, a patch whose connections were moved to the new outlet indices
# stays on the installed copy and goes silent.
install_help "sc_code/help/sc_code.maxhelp"

echo "   -> $BUNDLE"
file "$BUNDLE/Contents/MacOS/sc3~"
du -sh "$BUNDLE" | sed 's/^/   tamaño: /'

# # --- the .mxo depends on nothing outside macOS + Max + itself
# Acepta /usr/lib, /System/Library, @executable_path (frameworks de Max) y
# @loader_path (nuestra libsndfile). Cualquier otra cosa = falla el build.
check_deps() {
  local label="$1" img="$2" bad
  # Only the dependency lines (they are indented); the otool headers
  # for universal binaries start at column 0 and may have spaces.
  bad="$(otool -L "$img" | grep -E '^[[:space:]]' | awk '{print $1}' \
         | grep -v -E '^(/usr/lib/|/System/Library/|@executable_path/|@loader_path/)' || true)"
  if [[ -n "$bad" ]]; then
    echo "ERROR: $label tiene dependencias externas:" >&2
    echo "$bad" | sed 's/^/       /' >&2
    exit 1
  fi
}
check_deps "libsndfile" "$BUNDLE/Contents/Frameworks/libsndfile.dylib"
# The bundled .scx keep asking for @loader_path/../../Frameworks/libsndfile.dylib
# (from Contents/Resources/plugins that is Contents/Frameworks): if the copy is
# missing or has another name, scsynth loads the plugin and DiskIO dies at dlopen.
for scx in "$BUNDLE/Contents/Resources/plugins/"*.scx; do
  while read -r dep; do
    case "$dep" in
      @loader_path/*)
        rel="${dep#@loader_path/}"
        [[ -e "$BUNDLE/Contents/Resources/plugins/$rel" ]] || {
          echo "ERROR: $scx needs $dep and it is not in the bundle" >&2; exit 1; } ;;
    esac
  done < <(otool -L "$scx" | grep -E '^[[:space:]]' | awk '{print $1}')
done
echo "   deps: macOS + Max + @loader_path only, OK"
echo "   libsndfile: $(otool -D "$BUNDLE/Contents/Frameworks/libsndfile.dylib" | tail -1)"

# Gate: flat namespace symbols (see check_flat_symbols.py). The link goes with
# `-undefined dynamic_lookup` and checks nothing: if the binary references a DATA
# symbol that Max does not export (typical: `_common_symbols` because you used the
# `_sym_*` macros from commonsyms.h), Max rejects the whole bundle — "unable to load object
# bundle executable: The bundle “sc3~.mxo” couldn't be loaded" and in the log
# `symbol not found in flat namespace '__common_symbols'` (dlopen 67).
# Fix of the classic case: USESYM(foo) (= gensym) instead of `_sym_foo`.
python3 "$HERE/check_flat_symbols.py" "$BIN" || exit 1

# Ad-hoc signature of the BUNDLE (after building Contents/). The linker already
# leaves an ad-hoc "linker-signed" signature in the binary, but Max 9 validates the
# bundle with SecStaticCode: without a resource seal and a bound Info.plist it
# rejects the load with "<name>: cannot be loaded due to system security policy".
codesign --force --deep --sign - "$BUNDLE" 2>/dev/null || \
  echo "   WARNING: codesign failed (CLT without codesign?) — Max may reject the bundle"

codesign --verify --strict "$BUNDLE" && echo "   firma: ad-hoc OK"

# Same checker the CMake/ctest path runs (arch, minos, signature, deps): the two
# recipes have to produce the same bundle.
"$HERE/check_bundle.sh" "$BUNDLE" "$ARCH" "$MINOS" --self-contained
