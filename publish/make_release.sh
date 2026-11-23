#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# make_release.sh — assembles the GitHub Release assets.
#
# DEVELOPMENT-ONLY tool (not part of the public tree, see publish/MANIFEST.md):
# it needs the compiled bundles and the help files that only exist here.
#
# Usage:
#   ./publish/make_release.sh              # uses the versions in the sources
#   ./publish/make_release.sh 0.15.0       # force the sc3~ version
#   KEEP_STAGE=1 ./publish/make_release.sh # leave release/stage around to inspect
#
# Output (in release/):
#   sc3-<ver>-arm64.zip        both bundles + help + README + LICENSE + licenses/ + examples/
#   sc3-<ver>-arm64.zip.sha256
#
# The artifact name has no `~`: GitHub strips it from uploaded asset names, so a zip
# called `sc3~-0.15.1-arm64.zip` lands as `sc3.-0.15.1-arm64.zip` and then the
# `shasum -a 256 -c` in the release notes looks for a file that does not exist.
#
# It does NOT push or tag: it prints the tag command. GPL-3.0-only means the release
# must be accompanied by the sources (the public repo + the tag are that).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$HERE")"
cd "$ROOT"

VER="${1:-$(sed -n 's/.*SC3_VERSION:-\([0-9.]*\)}.*/\1/p' external/build.sh | head -1)}"
SC_CODE_VER="$(sed -n 's/^SC_CODE_VER := //p' external/sc_code/Makefile | head -1)"
[[ -n "$VER" ]] || { echo "ERROR: could not read SC3_VERSION from external/build.sh" >&2; exit 1; }

NAME="sc3-$VER-arm64"
STAGE="release/$NAME"
ZIP="release/$NAME.zip"

echo "== release $VER (sc3~) / $SC_CODE_VER (sc_code)"
echo "   stage: $STAGE"

# 1. Bundles: build + sign + verify (make gate would also run the tests; that is the
#    job of the closing gate, not of packaging). The .mxo is assembled by build.sh,
#    which installs to the Max Library unless SC3_OUTDIR is given.
SC3_OUTDIR="$PWD/release/bundle" ./external/build.sh >/dev/null
make -s -C external/sc_code bundle
codesign --verify --strict "release/bundle/sc3~.mxo"
codesign --verify --strict external/sc_code/build/sc_code.mxo

# 2. Stage: what a user needs to install, and nothing else.
rm -rf "$STAGE"
mkdir -p "$STAGE/help"
cp -R release/bundle/sc3~.mxo "$STAGE/"
cp -R external/sc_code/build/sc_code.mxo "$STAGE/"
cp external/help/sc3~.maxhelp "$STAGE/help/"
cp external/sc_code/help/sc_code.maxhelp "$STAGE/help/"
cp README.md LICENSE "$STAGE/"
cp -R licenses "$STAGE/"   # third-party notices: the bundle carries SC's payload
[[ -d examples ]] && cp -R examples "$STAGE/examples"
find "$STAGE" -name '.DS_Store' -delete

# 3. Zip (ditto keeps the bundle structure and the signature).
#    --norsrc/--noextattr: without them ditto writes an AppleDouble `._name` for every
#    file that carries extended attributes, so the zip shipped `._README.md` and
#    `._example.maxpat` next to the real files. The signature lives in
#    Contents/_CodeSignature, not in xattrs, so dropping them costs nothing.
mkdir -p release
rm -f "$ZIP" "$ZIP.sha256"
ditto -c -k --keepParent --norsrc --noextattr "$STAGE" "$ZIP"
#    The checksum line carries only the file NAME (no `release/` prefix): whoever
#    downloads the two assets runs `shasum -a 256 -c sc3-<ver>-arm64.zip.sha256` in the
#    download folder, where that prefix does not exist.
(cd "$(dirname "$ZIP")" && shasum -a 256 "$(basename "$ZIP")" > "$(basename "$ZIP").sha256")
if [[ "${KEEP_STAGE:-0}" != "1" ]]; then rm -rf "$STAGE" release/bundle; fi

echo
echo "== $ZIP ($(du -h "$ZIP" | cut -f1))"
cat "$ZIP.sha256"
echo "   contents:"
unzip -l "$ZIP" | sed -n '4,22p'
echo
echo "next:"
echo "  git tag -a v$VER -m \"sc3~ $VER / sc_code $SC_CODE_VER\""
echo "  # attach $ZIP and $ZIP.sha256 to the GitHub release"
echo "  # (GPL-3.0-only: the release needs the matching sources — the public repo + tag)"
