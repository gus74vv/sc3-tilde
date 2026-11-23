#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# build_sc.sh — build (or verify) the SuperCollider statics that sc3~ links against.
#
# The statics of vendor/supercollider are what make sc3~ self-contained: the .mxo
# embeds the engine and the language, so they must be the same slice as the bundle:
# arm64 only, deployment target 11.0 (decision 5 of the design notes). Leaving it to the host
# default is how you get a universal (or x86_64) libscsynth.a that links fine and
# then fails to load, or a minos above the one the bundle declares.
#
#   scripts/build_sc.sh            configure + build into vendor/supercollider/build
#   scripts/build_sc.sh --check    verify the artifacts already there (fast; the gate)
#   scripts/build_sc.sh -j N       parallel jobs (default: hw.ncpu)
#
# The checkout itself is not versioned here: see vendor/PIN.md for the pinned
# tag/commit and the clone command (it needs `--recurse-submodules`).
#
# NOTE: pinning CMAKE_OSX_ARCHITECTURES in a build dir configured without it is a
# cache change, so the first run rebuilds everything (~10 min on an M-series laptop).
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
SC=$(cd "$HERE/../../vendor/supercollider" && pwd)
B="$SC/build"
ARCH=arm64
MINOS=11.0
JOBS=$(sysctl -n hw.ncpu)

# The statics the linker takes (external/sc.mk is the single source of truth for the
# order; this is only the subset whose slice can be verified here).
ARTIFACTS=(
    lang/libsclang.a
    server/scsynth/libscsynth.a
    external_libraries/hidapi/mac/libhidapi.a
    external_libraries/libboost_thread_lib.a
    external_libraries/libboost_program_options_lib.a
    external_libraries/libboost_regex_lib.a
    external_libraries/libtlsf.a
    external_libraries/libyaml.a
)

die() { echo "ERROR: $*" >&2; exit 1; }

check() {
    local bad=0 f info minos
    for f in "${ARTIFACTS[@]}"; do
        [ -f "$B/$f" ] || die "missing $f — run scripts/build_sc.sh first"
        info=$(lipo -info "$B/$f")
        case "$info" in
            *"architecture: $ARCH"*) ;;
            *) echo "ERROR: $f is not $ARCH only ($info)" >&2; bad=1 ; continue ;;
        esac
        case "$info" in
            *x86_64*|*arm64e*) echo "ERROR: $f carries more than one slice ($info)" >&2; bad=1 ; continue ;;
        esac
        minos=$(otool -l "$B/$f" | sed -n 's/^ *minos \(.*\)$/\1/p' | sort -u | tr '\n' ' ')
        [ "$minos" = "$MINOS " ] || { echo "ERROR: $f has minos [$minos], expected [$MINOS]" >&2; bad=1 ; continue ; }
    done
    [ "$bad" -eq 0 ] || exit 1
    echo "   sc statics: ${#ARTIFACTS[@]} libs, $ARCH only, minos $MINOS OK"
}

build() {
    [ -d "$SC/external_libraries/boost" ] || die "submodules missing in $SC
       git submodule update --init --recursive
     (or re-clone as documented in vendor/PIN.md)"
    cmake -S "$SC" -B "$B" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$MINOS" \
        -DBUILD_SHARED_LIBS=OFF \
        -DSUPERNOVA=OFF \
        -DSC_QT=OFF \
        -DSC_IDE=OFF \
        -DSC_EDITOR=OFF
    cmake --build "$B" -j "$JOBS"
    check
}

MODE=build
while [ $# -gt 0 ]; do
    case "$1" in
        --check) MODE=check ;;
        -j) shift; JOBS=${1:?"-j needs a number"} ;;
        -h|--help) sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) die "unknown argument: $1 (try --help)" ;;
    esac
    shift
done

[ -d "$SC" ] || die "no SuperCollider checkout at $SC (see vendor/PIN.md)"
"$MODE"
