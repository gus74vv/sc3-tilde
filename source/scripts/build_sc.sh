#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# build_sc.sh <tag> <dir> -- clone SuperCollider <tag> into <dir> and build the static
# libraries sc3~ links (not sclang/scsynth or the plugins), for arm64 and macOS 11.0.
# CI keys its cache on this file, so any change here rebuilds the statics there.
set -euo pipefail

tag="${1:?usage: build_sc.sh <tag> <dir>}"
dir="${2:?usage: build_sc.sh <tag> <dir>}"

[[ -d "$dir" ]] || git clone --branch "$tag" --depth 1 \
  --recurse-submodules --shallow-submodules \
  https://github.com/supercollider/supercollider.git "$dir"
cmake -S "$dir" -B "$dir/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DSC_QT=OFF -DSC_IDE=OFF -DSC_ABLETON_LINK=OFF -DSUPERNOVA=OFF \
  -DENABLE_TESTSUITE=OFF
cmake --build "$dir/build" -j"$(sysctl -n hw.ncpu)" --target libsclang libscsynth
