#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# stage_bundle.sh — put SuperCollider's runtime payload INSIDE the .mxo so the
# object no longer needs /Applications/SuperCollider.app (issue #1, item 9,
# "self-contained", default build).
#
# What lands in the bundle (and why):
#   Contents/Resources/SCClassLibrary/     stock class library (plain .sc); it is
#                                          what sclang compiles and the object's
#                                          Platform.resourceDir points at
#   Contents/Resources/plugins/*.scx       stock UGens (without the `_supernova`
#                                          pair: this engine is scsynth)
#   Contents/Resources/sounds/             files the class library loads by name
#   Contents/Resources/HID_Support/hut/    HID usage tables (text)
#   Contents/Resources/SCMaxCompat/ our Qt-free module (repo, the first spike/compat)
#   Contents/Resources/licenses/           third-party notices (GPL-3, LGPL-2.1, MIT, BSL)
#   Contents/Frameworks/libsndfile.dylib   the same copy the object links against;
#                                          the .scx ask for exactly this name
#                                          (see sc.mk)
#
# Every Mach-O that lands inside is thinned to a single arch (the bundles are
# arm64-only, scripts/build_sc.sh) and re-signed ad-hoc: `lipo` and
# `install_name_tool` invalidate whatever signature the file had, and an invalid
# signature is a dlopen failure ("code signature invalid"), not a warning.
#
# One implementation: external/build.sh (dev loop) and the CMake POST_BUILD both
# call this, so the two recipes cannot disagree about the contents.
#
# Usage: scripts/stage_bundle.sh <bundle.mxo> [arch] [supercollider.app]

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # external/
ROOT="$(dirname "$HERE")"                                  # repo root
BUNDLE="${1:-}"
ARCH="${2:-${SC3_ARCH:-arm64}}"
SC_APP="${3:-${SC_APP:-/Applications/SuperCollider.app}}"
SRC="$SC_APP/Contents/Resources"
SNDFILE="$HERE/Frameworks/libsndfile.dylib"

[[ -n "$BUNDLE" ]] || { echo "usage: stage_bundle.sh <bundle.mxo> [arch] [sc.app]" >&2; exit 2; }
[[ -d "$BUNDLE/Contents/MacOS" ]] || {
  echo "ERROR: no .mxo at $BUNDLE (build the bundle first)" >&2; exit 1; }
[[ -d "$SRC/SCClassLibrary" ]] || {
  echo "ERROR: no SuperCollider class library at $SRC" >&2
  echo "       An installed 3.14.1 is needed ONCE, at build time, to copy its payload." >&2
  echo "       SC_APP=<other path> to point elsewhere." >&2; exit 1; }
[[ -f "$SNDFILE" ]] || {
  echo "ERROR: missing $SNDFILE (run 'make sndfile' in external/)" >&2; exit 1; }

RES="$BUNDLE/Contents/Resources"
FW="$BUNDLE/Contents/Frameworks"
# Frameworks only carries our libsndfile: start empty, so a renamed or
# dropped dependency cannot survive in a bundle that is rebuilt in place
# (the CMake POST_BUILD reuses the same directory).
rm -rf "$FW"
mkdir -p "$RES" "$FW"

# Single slice + valid signature. Text files (the .sc library, sounds, the HID
# tables) are left alone: `lipo -archs` fails on them and nothing is signed.
thin_sign() {
  local f="$1" archs
  archs="$(lipo -archs "$f" 2>/dev/null)" || return 0
  if [[ "$archs" != "$ARCH" ]]; then
    lipo -thin "$ARCH" "$f" -output "$f.thin" && mv "$f.thin" "$f"
  fi
  codesign --force --sign - "$f" 2>/dev/null || {
    echo "WARNING: ad-hoc codesign failed on $f" >&2; }
}

# --- class library, sounds, HID tables (text: copy, no thinning) --------------
rm -rf "$RES/SCClassLibrary" "$RES/sounds" "$RES/HID_Support"
cp -R "$SRC/SCClassLibrary" "$RES/"
cp -R "$SRC/sounds" "$RES/"
cp -R "$SRC/HID_Support" "$RES/"

# --- stock UGens: the supernova pair is not for this engine -------------------
rm -rf "$RES/plugins"
mkdir -p "$RES/plugins"
shopt -s nullglob
for f in "$SRC"/plugins/*.scx; do
  case "$(basename "$f")" in
    *_supernova.scx) continue ;;
  esac
  cp "$f" "$RES/plugins/"
  thin_sign "$RES/plugins/$(basename "$f")"
done
shopt -u nullglob
(( $(ls -1 "$RES/plugins" | wc -l) > 0 )) || {
  echo "ERROR: no .scx copied from $SRC/plugins" >&2; exit 1; }

# --- our Qt-free module -------------------------------------------------------
rm -rf "$RES/SCMaxCompat"
cp -R "$ROOT/phase1/compat/SCMaxCompat" "$RES/"

# --- third-party notices ------------------------------------------------------
# The payload is SuperCollider's (GPL-3) plus libsndfile (LGPL-2.1) and the
# statics it links, so the .mxo as shipped has to carry the notices. One folder
# at the repo root, copied as-is: licenses/README.md is the index (D4).
rm -rf "$RES/licenses"
cp -R "$ROOT/licenses" "$RES/"

# --- libsndfile: one copy, the name the .scx ask for --------------------------
cp "$SNDFILE" "$FW/libsndfile.dylib"
thin_sign "$FW/libsndfile.dylib"

echo "   payload: $(du -sh "$RES" | cut -f1) Resources + $(du -sh "$FW" | cut -f1) Frameworks" \
     "($(ls -1 "$RES/plugins" | wc -l | tr -d ' ') ugens, arm64 only)"
