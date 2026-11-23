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
# Since issue #1 item 9 the bundle is also SELF-CONTAINED, and that has its own
# traps: a missing class library (the object boots and finds no library), a
# _supernova .scx in the payload (this engine is scsynth), a payload binary left
# universal by lipo, or a stale signature after lipo/install_name_tool (dlopen
# fails with "code signature invalid" even though the file is there).
#
# Usage: check_bundle.sh <path/to/thing.mxo> [arch] [minos] [--self-contained]
#   --self-contained  also require the SuperCollider payload (class library,
#                     stock .scx, sounds, libsndfile). Only the engine bundle
#                     carries it: sc_code is a different object with no SC.

set -uo pipefail

BUNDLE="${1:-}"
ARCH="${2:-arm64}"
MINOS="${3:-11.0}"
SELF_CONTAINED=0
for a in "$@"; do [[ "$a" == "--self-contained" ]] && SELF_CONTAINED=1; done
[[ -n "$BUNDLE" ]] || {
  echo "usage: check_bundle.sh <bundle.mxo> [arch] [minos] [--self-contained]" >&2
  exit 2; }
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
(( SELF_CONTAINED )) || exit 0

# 5. self-contained payload: class library, stock .scx, sounds and libsndfile
#    inside the bundle (nothing from /Applications/SuperCollider.app is read
#    at run time).
RES="$BUNDLE/Contents/Resources"
FW="$BUNDLE/Contents/Frameworks"
[[ -d "$RES/SCClassLibrary" ]] || fail "no Resources/SCClassLibrary: the payload is missing (scripts/stage_bundle.sh)"
[[ -d "$RES/SCMaxCompat" ]] || fail "no Resources/SCMaxCompat (Qt-free module)"
[[ -d "$RES/sounds" ]] || fail "no Resources/sounds (the class library loads files by name)"
[[ -f "$FW/libsndfile.dylib" ]] || fail "no Frameworks/libsndfile.dylib (the bundled .scx ask for exactly that name)"
# The payload is somebody else's code: the notices travel with it (licenses/).
[[ -f "$RES/licenses/README.md" ]] || fail "no Resources/licenses/README.md (third-party notices)"
for _l in SuperCollider.txt libsndfile.txt LGPL-2.1.txt boost.txt; do
  [[ -f "$RES/licenses/$_l" ]] || fail "no Resources/licenses/$_l"
done

PLUGINS="$(find "$RES/plugins" -name "*.scx" -maxdepth 1 2>/dev/null | wc -l | tr -d ' ')"
(( PLUGINS > 0 )) || fail "no .scx in Resources/plugins"
SUPERNOVA="$(find "$RES/plugins" -name "*_supernova.scx" -maxdepth 1 2>/dev/null | wc -l | tr -d ' ')"
(( SUPERNOVA == 0 )) || fail "$SUPERNOVA *_supernova.scx in the payload (this engine is scsynth)"

# 6. every Mach-O of the payload: one slice and a valid signature
while IFS= read -r -d '' f; do
  file "$f" | grep -q 'Mach-O' || continue
  A="$(lipo -archs "$f" 2>/dev/null)"
  [[ "$A" == "$ARCH" ]] || fail "payload $(basename "$f") is '$A', expected '$ARCH'"
  codesign --verify --strict "$f" 2>/dev/null || fail "payload $(basename "$f") has an invalid signature"
done < <(find "$RES" "$FW" -type f -print0)

# 7. the .scx resolve libsndfile through @loader_path, inside the bundle
for scx in "$RES/plugins/"*.scx; do
  for dep in $(otool -L "$scx" | grep -E '^[[:space:]]' | awk '{print $1}'); do
    case "$dep" in
      /usr/lib/*|/System/Library/*) ;;
      @loader_path/*)
        [[ -e "$(dirname "$scx")/$(printf %s "$dep" | sed 's|^@loader_path/||')" ]] || \
          fail "$(basename "$scx"): $dep does not resolve inside the bundle" ;;
      *) fail "$(basename "$scx"): dependency outside macOS and the bundle: $dep" ;;
    esac
  done
done

echo "OK  payload: $(basename "$RES")/ (library, $PLUGINS ugens, sounds, HID) + libsndfile"
