# Third-party notices

sc3~ is a Max external that links SuperCollider's interpreter (libsclang) and
audio server (libscsynth) as static libraries, and its default build carries
SuperCollider's runtime payload inside the .mxo: the class library, the stock
UGens, sounds and the HID tables. A distributed bundle therefore contains
third-party code, and this folder carries its notices.

It travels with the code, from this single folder:

* inside the bundle: `sc3~.mxo/Contents/Resources/licenses/`
* in the Max package: `licenses/`, next to `externals/` and `help/`
* in the release zip: `licenses/`

sc3~'s own code is GPL-3.0-only; the license text is the `LICENSE` file at the
root of the repository and of the release zip.

## What is inside, and where its notice is

| component | part of | license | notice |
|---|---|---|---|
| SuperCollider 3.14.1 (libsclang, libscsynth, SCClassLibrary, stock UGens, sounds, HID tables) | everything | GPL-3.0 | `SuperCollider.txt` |
| libsndfile 1.2.2 | `Contents/Frameworks/libsndfile.dylib` | LGPL-2.1-or-later | `libsndfile.txt`, `LGPL-2.1.txt` |
| Boost (thread, program_options, regex) | libsclang, libscsynth | BSL-1.0 | `boost.txt` |
| yaml-cpp | libsclang, libscsynth (configuration) | MIT | `yaml-cpp.txt` |
| hidapi | libsclang (HID) | GPL-3.0 (chosen from the three the project offers) | `hidapi.txt` |
| TLSF 2.4.6 (real-time allocator) | libscsynth | LGPL-2.1-or-later, with the linking exception in `tlsf.txt` | `tlsf.txt`, `LGPL-2.1.txt` |
| nova-simd, nova-tt | libscsynth | GPL-2.0-or-later | `nova-simd.txt`, `nova-tt.txt`, `GPL-2.0.txt` |
| oscpack 1.1.0 | libsclang (OSC) | MIT-style (BSD) | `oscpack.txt` |

The bundles contain no Qt, no PortAudio, no PortMidi and no ICU: those
SuperCollider dependencies are not linked here, so their notices are not
included.

## Codecs inside libsndfile

The libsndfile dylib is the prebuilt, self-contained binary from the
SuperCollider 3.14.1 release; it links its codecs statically:

| codec | license | upstream |
|---|---|---|
| FLAC | BSD-3-Clause | https://xiph.org/flac/ |
| Ogg / Vorbis | BSD-3-Clause | https://xiph.org/vorbis/ |
| Opus | BSD-3-Clause | https://opus-codec.org/ |
| mpg123 | LGPL-2.1 | https://www.mpg123.de/ |
| LAME | LGPL-2.0 | https://lame.sourceforge.io/ |

`LGPL-2.1.txt` covers the LGPL parts (libsndfile, mpg123, TLSF); the BSD-3-Clause
notices of FLAC, Ogg, Vorbis and Opus are published with those projects and with
libsndfile's source (https://github.com/libsndfile/libsndfile), which is also the
source of the binary carried here.

Because libsndfile stays a separate dylib in `Contents/Frameworks/`, it can be
replaced with another build of the same version: no part of sc3~ has to be
relinked to do so.

## Rebuilding from source

SuperCollider's source is pinned in the repository (`vendor/supercollider`,
tag `Version-3.14.1`) and the build instructions are in `BUILD.md`. The
SuperCollider version is a build-time dependency: its payload is copied into
the bundle once, at build time, and never read from `/Applications` at run time.
