# Third-party licenses

`sc3~.mxo` bundles the components below. Each directory holds the component's
license and copyright notices, copied verbatim from the release listed.

| Component | Version | In the bundle | License | Source |
|-|-|-|-|-|
| SuperCollider | 3.14.1 | `sc3~` binary (`libsclang`, `libscsynth`), `Resources/SCClassLibrary`, `Resources/plugins`, `Resources/sounds`, `Resources/HID_Support` | GPLv3, per SuperCollider's `COPYING` (identical to [`../LICENSE`](../LICENSE)); authors in [`SuperCollider/`](SuperCollider/) | <https://github.com/supercollider/supercollider/tree/Version-3.14.1> |
| libsndfile | 1.2.2 | `Frameworks/libsndfile.dylib` | LGPL-2.1-or-later | <https://github.com/libsndfile/libsndfile/releases/tag/1.2.2> |
| libFLAC | 1.5.0 | inside `libsndfile.dylib` | BSD-3-Clause | <https://github.com/xiph/flac/releases/tag/1.5.0> |
| libogg | see note | inside `libsndfile.dylib` | BSD-3-Clause | <https://github.com/xiph/ogg> |
| libvorbis | 1.3.7 | inside `libsndfile.dylib` | BSD-3-Clause | <https://github.com/xiph/vorbis/releases/tag/v1.3.7> |
| Opus | 1.5.2 | inside `libsndfile.dylib` | BSD-3-Clause | <https://github.com/xiph/opus/releases/tag/v1.5.2> |
| mpg123 | 1.32.9 | inside `libsndfile.dylib` | LGPL-2.1 | <https://www.mpg123.de/download/mpg123-1.32.9.tar.bz2> |
| LAME | 3.100 | inside `libsndfile.dylib` | LGPL-2.0-or-later | <https://lame.sourceforge.io/> |

`libsndfile.dylib` is copied from SuperCollider.app 3.14.1, whose build links
the codecs into it statically. Their versions were read from the strings in that
binary. libogg carries no version string; its notice is from 1.3.6 and has not
changed since 2002.

`libsndfile.dylib` is a separate file, so it can be replaced with a rebuilt copy.
The bundle must then be re-signed (`codesign --force --sign - sc3~.mxo`).
