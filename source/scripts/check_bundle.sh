#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# check_bundle.sh <bundle.mxo> [arch...] -- verify a built .mxo:
#   * the binary carries every requested arch slice
#   * the bundle signature verifies (Max 9 rejects unsealed bundles)
#   * no dynamic dependency outside macOS, Max (@executable_path) and the bundle (@loader_path)
set -euo pipefail

bundle="${1:?usage: check_bundle.sh <bundle.mxo> [arch...]}"
shift
bin="$bundle/Contents/MacOS/$(basename "$bundle" .mxo)"
[[ -f "$bin" ]] || { echo "ERROR: missing $bin" >&2; exit 1; }

for arch in "$@"; do
  lipo "$bin" -verify_arch "$arch" || { echo "ERROR: $bin lacks $arch" >&2; exit 1; }
done

codesign --verify --strict "$bundle"

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
}
check_deps "$bin"
shopt -s nullglob
for lib in "$bundle"/Contents/Frameworks/*.dylib; do
  check_deps "$lib"
done
echo "$bundle: OK"
