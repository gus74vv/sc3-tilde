<!-- SPDX-License-Identifier: GPL-3.0-only -->
# sc3~

SuperCollider engine inside Max. Same process. Audio on Max's buses.
Editor included: sc_code.

Reqs: macOS Apple Silicon · Max 9. No SuperCollider installation: the engine
3.14.1 (class library, UGens, libsndfile) travels inside the .mxo.
Your Quarks and extensions are compiled on top, so they work as-is.
No Qt in the engine: plot, Window and the rest of the GUI don't work.
Max is the UI.

Drop the .mxo files in your Max Library, restart Max.
First download only:
    xattr -dr com.apple.quarantine ~/Documents/Max\ 9/Library/*.mxo

Patch:
    sc3~ 2 → dac~
    open
    eval { SinOsc.ar(440) * 0.2 }.play;

Don't press ⌘. in Max to stop the sound — use ⌘/ or ⌃. in sc_code,
or send `panic`.

examples/example.maxpat is the tour. Help files are the manual.

GPL-3.0.

Coffee:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/T0W629WR81)
