#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# check_bundle.sh <bundle.mxo> [arch...] -- verify a built .mxo:
#   * the binary carries every requested arch slice
#   * the bundle signature verifies (Max 9 rejects unsealed bundles)
#   * bundled dylibs and plugins carry exactly the requested slices and a valid signature
#   * no dynamic dependency outside macOS, Max (@executable_path) and the bundle (@loader_path)
#     for the binary, bundled dylibs and bundled plugins; @loader_path targets must exist
set -euo pipefail

bundle="${1:?usage: check_bundle.sh <bundle.mxo> [arch...]}"
shift
bin="$bundle/Contents/MacOS/$(basename "$bundle" .mxo)"
[[ -f "$bin" ]] || { echo "ERROR: missing $bin" >&2; exit 1; }

for arch in "$@"; do
  lipo "$bin" -verify_arch "$arch" || { echo "ERROR: $bin lacks $arch" >&2; exit 1; }
done

codesign --verify --strict "$bundle"

# Bundled dylibs and plugins: exactly the requested slices, each validly signed.
shopt -s nullglob
want="$(printf '%s\n' "$@" | sort | xargs)"
for scx in "$bundle"/Contents/Frameworks/*.dylib "$bundle"/Contents/Resources/plugins/*.scx; do
  if [[ -n "$want" ]]; then
    have="$(lipo -archs "$scx" | tr ' ' '\n' | sort | xargs)"
    [[ "$have" == "$want" ]] || { echo "ERROR: $scx has [$have], want [$want]" >&2; exit 1; }
  fi
  codesign --verify --strict "$scx" || { echo "ERROR: $scx signature invalid" >&2; exit 1; }
done

check_deps() {
  local bad
  # Dependency lines are indented; otool's per-arch headers start at column 0.
  bad="$(otool -L "$1" | grep -E '^[[:space:]]' | awk '{print $1}' \
         | grep -v -E '^(/usr/lib/|/System/Library/|@executable_path/|@loader_path/|@rpath/)' || true)"
  if [[ -n "$bad" ]]; then
    echo "ERROR: $1 has external dependencies:" >&2
    echo "$bad" | sed 's/^/  /' >&2
    exit 1
  fi
  # Every @loader_path dependency must exist inside the bundle.
  local dep dir
  dir="$(dirname "$1")"
  while read -r dep; do
    [[ -e "$dir/${dep#@loader_path/}" ]] || { echo "ERROR: $1 needs missing $dep" >&2; exit 1; }
  done < <(otool -L "$1" | grep -E '^[[:space:]]+@loader_path/' | awk '{print $1}' | sort -u)
}
check_deps "$bin"
shopt -s nullglob
for lib in "$bundle"/Contents/Frameworks/*.dylib "$bundle"/Contents/Resources/plugins/*.scx; do
  check_deps "$lib"
done
echo "$bundle: OK"
