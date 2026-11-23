#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# check_bundle.sh — gate for one .mxo: single slice, deployment target, signature
# and dependencies. These are the four ways a bundle that compiles still fails to
# load in Max (issue #1, items 4 and 5):
#
#   * a universal or x86_64 binary next to arm64-only statics: it links, then the
#     loader picks a slice that is not there;
#   * a minos above the bundle's: same kind of surprise on older systems;
#   * an ad-hoc signature that does not verify: Max refuses the bundle with
#     "cannot be loaded due to system security policy";
#   * a dependency outside macOS / Max / the bundle: the loader fails at dlopen.
#
# Usage: check_bundle.sh <path/to/thing.mxo> [expected arch] [expected minos]

set -uo pipefail

BUNDLE="${1:-}"
ARCH="${2:-arm64}"
MINOS="${3:-11.0}"
[[ -n "$BUNDLE" ]] || { echo "usage: check_bundle.sh <bundle.mxo> [arch] [minos]" >&2; exit 2; }
[[ -d "$BUNDLE" ]] || { echo "FAIL: no bundle at $BUNDLE" >&2; exit 1; }

fail() { echo "FAIL: $(basename "$BUNDLE"): $*" >&2; exit 1; }

EXE="$(find "$BUNDLE/Contents/MacOS" -maxdepth 1 -type f ! -name '*.dSYM' | head -1)"
[[ -n "$EXE" ]] || fail "no executable in Contents/MacOS"
[[ -f "$BUNDLE/Contents/Info.plist" ]] || fail "no Contents/Info.plist"
[[ -f "$BUNDLE/Contents/PkgInfo" ]] || fail "no Contents/PkgInfo"

# 1. one slice only, and the expected one
ARCHS="$(lipo -archs "$EXE" 2>/dev/null)"
[[ "$ARCHS" == "$ARCH" ]] || fail "arch is '$ARCHS', expected '$ARCH' only"

# 2. deployment target
ACTUAL_MINOS="$(otool -l "$EXE" | awk '/LC_BUILD_VERSION/ {found=1} found && /minos/ {print $2; exit}')"
[[ -n "$ACTUAL_MINOS" ]] || fail "cannot read the deployment target"
[[ "$ACTUAL_MINOS" == "$MINOS" ]] || fail "minos is $ACTUAL_MINOS, expected $MINOS"

# 3. signature
codesign --verify --strict "$BUNDLE" 2>/dev/null || fail "codesign --verify --strict failed"

# 4. dependencies: macOS, Max itself and the bundle only
BAD="$(otool -L "$EXE" | grep -E '^[[:space:]]' | awk '{print $1}' \
       | grep -v -E '^(/usr/lib/|/System/Library/|@executable_path/|@loader_path/)' || true)"
[[ -z "$BAD" ]] || fail "external dependencies: $BAD"

echo "OK  $(basename "$BUNDLE"): $ARCH, minos $ACTUAL_MINOS, ad-hoc signature, deps macOS/Max/bundle"
