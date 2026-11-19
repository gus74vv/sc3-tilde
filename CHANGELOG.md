# Changelog

Changes in this fork, relative to upstream [gus74vv/sc3-tilde](https://github.com/gus74vv/sc3-tilde) 0.15.11.

## Unreleased

### Changed

- Restructured as a Max package: `externals/`, `help/`, `examples/` and `package-info.json` at the root. `make link` symlinks the checkout into `~/Documents/Max 9/Packages`.

- CMake on top of the `source/max-sdk-base` submodule replaces the per-external Makefiles, `sc.mk` and `build.sh`. Sources live in `source/projects/sc3_tilde` and `source/projects/sc_code`; bundle versions are read from the `#define`s in the sources.

- `sc3~.mxo` is self-contained. It bundles the SuperCollider 3.14.1 class library, the 26 scsynth plugins, `sounds/`, `HID_Support/` and `libsndfile`, so no SuperCollider installation is needed at runtime. The env var `SC3_APP_PATH` still selects an installed app, with the old version check. Bundling removes the failure where an installed app of another minor version cannot run the engine's class library.

- All native code in `sc3~.mxo` is arm64 only: the plugins and `libsndfile` are thinned with `lipo`. The bundle is 11 MB.

- Both externals link against the SDK's list of Max symbols instead of `-undefined dynamic_lookup`, so a Max API symbol missing from the SDK fails at link time.

- `make test` runs bundle checks through ctest: architecture slices, signatures, dependencies confined to macOS, Max and the bundle, and Max symbol resolution against `MAX_APP` (skipped without it).

- `make sc` clones and builds only the SuperCollider static libraries `sc3~` links.

- `sc3~` outlets follow the MSP convention: audio first, left to right `ch 0 … ch n-1`, then `error`, then `post` (rightmost). The audio outlets used to be rightmost. Patches saved with the old layout reconnect by outlet index, so their audio cables now land on the wrong outlets; the bundled help and example patches are rewired. `sc3~.maxhelp` also had its two `dac~` cables crossed, which swapped left and right; they are straightened.

### Fixed

- The SuperCollider statics were built for the host macOS (26.0) while `sc3~` targets 11.0. `make sc` pins arm64 and macOS 11.0.

- Changing a value in an external's `CMakeLists.txt` regenerated `Info.plist` without relinking, so the SDK's post-build `codesign` did not run and the bundle kept a stale seal. Max 9 rejects such a bundle. The `CMakeLists.txt` is now a link dependency.

- `package-info.json` listed the platform as `arm64`. Max expects `x64`/`aarch64` and refused to load the package ("64-bit architecture unsupported").

- `check_flat_symbols.py` reported `_NSPasteboardTypeString` as unresolved: it re-checked library-bound symbols against SDK `.tbd` files and searched only `Versions/A/`, while AppKit's is under `Versions/C/`. It now checks only the symbols dyld looks up in the flat namespace, which are the ones the linker cannot verify.

- The bundled dylib is named `libsndfile.dylib`, not `libsndfile.1.dylib`: four plugins load `@loader_path/../../Frameworks/libsndfile.dylib`, and another name would load a second copy.
