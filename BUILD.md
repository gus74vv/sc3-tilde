<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Building sc3~

Two recipes, one source of truth: the SuperCollider include paths and the list and
order of the statics live in `external/sc.mk`. Both build the same two bundles.

Download-only users do not need any of this: the release carries the package.

## Prerequisites (both recipes)

* macOS on Apple Silicon.
* Xcode Command Line Tools.
* Max 9 with the **max-sdk** package installed (Max → Options → Package Manager →
  max-sdk), used as `~/Documents/Max 9/Packages/max-sdk/source/max-sdk-base`.
* An installed **SuperCollider 3.14.1** (`/Applications/SuperCollider.app`). Its
  class library and its libsndfile are used at runtime and at link time; the same
  version is vendored in `vendor/supercollider` with its statics already built
  (`vendor/supercollider/build`, tag `Version-3.14.1`).

`SC_APP`, `MAX_SDK` and `SC3_SR` override the paths above.

## Dev loop: build and install in your Max Library

```sh
cd external && ./build.sh
```

Builds `sc3~.mxo` from the Makefile, installs it and both help patches in
`~/Documents/Max 9/Library`, and signs the bundle ad-hoc. Restart Max afterwards.
The headless gates:

```sh
cd external && make gate                       # refs + tests + bundles + SC statics
cd external/sc_code && make test               # the editor's unit tests
```

## Package: CMake stages a Max package

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure     # same gates as `make gate`
cmake --build build --target package           # build/package/sc3-tilde
```

`build/package/sc3-tilde` is a package Max reads as-is:

    package-info.json    version taken from SC3_TILDE_VERSION in sc3_tilde.cpp
    externals/           sc3~.mxo and sc_code.mxo
    help/                the two .maxhelp
    docs/                the two .maxref.xml (reference viewer)
    examples/            example.maxpat, snippets, superdirt patches

Copy that folder into `~/Documents/Max 9/Packages` (or
`cmake --build build --target install-package`) and restart Max. Do not keep a
second copy of the objects in `~/Documents/Max 9/Library`: Max loads the first
one it finds and fails with a duplicate-object warning.

## Gates

`ctest` runs what `make gate` runs (reference pages, headless engine tests, the
editor's unit tests, the SuperCollider statics) plus the bundle checks: one arch
only, deployment target, ad-hoc signature that verifies, no dependency outside
macOS/Max/the bundle, and the flat-namespace symbols Max has to export. The
package layout is a gate too: a missing folder is not an error Max reports, the
object simply does not exist.
