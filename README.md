<!-- SPDX-License-Identifier: GPL-3.0-only -->
# sc3~ — Supercollider inside Max

Two Max externals that bring the SuperCollider engine and an SC-style code editor into a
Max patch. The engine runs inside Max's process, so a patch does not depend on anything
else running on the machine:

| object | version | what it is |
|---|---|---|
| `sc3~` | 0.16.1 | Runs **sclang + scsynth in-process** inside Max. You send it SuperCollider code; the audio lands on Max's own audio buses, so it can be patched, mixed and processed like any other Max signal. |
| `sc_code` | 0.9.14 | An in-patch code editor with the SuperCollider IDE's evaluation UX (`⇧Return`, `⌘Return`, `⌘⇧Return`), syntax highlighting, undo, auto-indent, bracket matching, snippets and a `.scd` open/save menu. A bottom bar shows the signature of the call under the caret (current argument highlighted) and offers completion: the editor *shows*, nothing is written while you type. Its outlet sends the evaluated block to `sc3~`. |

```
 ┌─────────┐  eval <code>       ┌────────┐
 │ sc_code ├───────────────────►│  sc3~  ├──►· audio (Max signals)
 └─────────┘  (message inlet)   └────┬───┘◄──· audio in (SoundIn)
                                     │
                              error / post (text outlets)
```

## Requirements

* **macOS 11 or newer, Apple Silicon (arm64).** There is no official Intel build; see
  [Building from source](#building-from-source) if you need x86_64.
* **Max 9** (tested with 9.2).

The machine that runs the object does not need SuperCollider installed. The `.mxo` carries
the 3.14.1 payload — class library, stock `.scx` UGens, sounds, HID tables — together with
its own copy of libsndfile, so the engine boots on a Mac that has never seen the SC IDE. The
installation matters at build time only: `build.sh` copies the payload once, out of
`/Applications/SuperCollider.app` (point `SC_APP=…` somewhere else to use another copy).

Your own library still loads on top of that payload, with the paths sclang always used:

* `~/Library/Application Support/SuperCollider/Extensions` is compiled at boot, and the
  `.scx` plugins found there are loaded, exactly like with the IDE.
* The Quarks listed in your `sclang_conf.yaml` are compiled as well, and `startup.scd` runs.
* If that library does not compile, the boot falls back to the stock library plus the
  Qt-free module, and the reason arrives on the `error` outlet. A user Quark can break its
  own compilation, never the object.

The payload is pinned to 3.14.1, the version the statics are compiled against, so your
compiled `.scx` files have to match that ABI. `SC3_APP_PATH` (development) points the object
at a real installation instead, and the version policy comes back with it: the same
`major.minor` is accepted (3.14.2, 3.14.7 … with a console note), another minor refuses to
boot unless you pass `@allowscversion 1`. Boot also builds a tiny smoke `SynthDef`, so a
library that does not actually work fails with a real error instead of leaving a silent
engine.

## Install

The download is a Max **package**, not a bag of files. Unzipped it is one folder:

```
sc3-tilde/
├── package-info.json      what Max reads to list it in the Package Manager
├── externals/             sc3~.mxo and sc_code.mxo
├── help/                  sc3~.maxhelp and sc_code.maxhelp
├── docs/                  the reference pages
├── examples/              example.maxpat
├── licenses/              third-party notices
├── README.md
└── LICENSE
```

1. Unzip the download.
2. Move the `sc3-tilde` folder into `~/Documents/Max 9/Packages` (Max creates that folder on
   first launch; the same place it puts the packages the Package Manager installs).
3. **Restart Max** (the objects are loaded when the app starts).

If you installed a previous version by hand, remove those copies from
`~/Documents/Max 9/Library` (the two `.mxo` and the two `.maxhelp`): Max would load both.

Max scans `Packages` on launch, so both objects are available from any patch, their help is
reachable with **⌥-click** without copying anything into `Library`, and Max loads exactly one
copy of each object even if an older `.mxo` is still around. Nothing in the package is
loaded at run time except the two bundles; `licenses/` and `docs/` are there to be read.

Building from source installs the same layout (`cmake --build build --target install-package`);
see [Building from source](#building-from-source).

Builds are signed ad-hoc, so macOS Gatekeeper will block the download; see
[Gatekeeper](#macos-gatekeeper).

## Quick start

```
[sc3~ 2]  ──────────────►  [dac~]
```
* Send `open` (or a `bang`) to boot the engine.
* Send it code: `eval { SinOsc.ar(440) * 0.2 }.play;` — or patch an `sc_code` editor
  to the message inlet and press `⌘Return`.
* Whatever the engine prints shows up in **Max's console** on its own, with nothing
  patched — no `[print]` needed.
* `panic` runs `CmdPeriod.run` (nodes + clocks + hooks), `freeall` only frees the nodes;
  `close` shuts the engine down.

The release also ships **`examples/example.maxpat`**, a self-contained patch: it boots the
engine, plays a demo from the `sc_code` buffer, shows the `set` path for Synth arguments
from Max's UI and has a `panic` button (Ctrl.+). Its comments explain what to look at,
including the one-time `index` message.

> **Warning for SuperCollider users:** do **not** press `⌘.` in Max to kill the sound.
> In Max that is *stop scheduler*, which halts Max's own scheduler and the whole patch
> stops responding. Use the `panic` message (or `⌃.` / `⌘/` inside `sc_code`), which touches
> the SC side only. If you already hit it, bring the scheduler back with Max's
> `Resume`.

## How it works

| thread | owns | does |
|---|---|---|
| Max audio (`perform`) | the SuperCollider `World` | drain OSC → `PerformOSC*` → `World_Run` → copy its buses to the DSP chain |
| engine worker | the sclang VM | boot, `eval`, `tick()` (AppClock), collect the posts |
| Max main | nothing of the engine | enqueue work, deliver the outlets |

* **The engine sits in Max's process.** `scsynth` and `sclang` are linked statically into
  the bundle, so the object does not need a helper app behind it. The `World` is created headless
  (`mRealTime = false`, no audio driver — the mode SuperCollider itself uses for offline
  renders) and it is `sc3~` who calls `World_Run` from the audio thread and copies its
  output buses into the DSP chain: SC's graph becomes ordinary Max signals.
* **OSC over loopback.** The embedded sclang talks to the `World` with OSC, over loopback
  UDP through the engine's own socket — `World_OpenUDP` is unusable without a driver, it
  drops the datagram — with a reply address that answers back. Bundles that carry a timetag
  are queued and cleared by `/clearSched`. The `World` binds first (port `0` lets the kernel
  pick one, `@port N` forces it) and hands the real port to sclang before its library is
  compiled, which is why the SC IDE can stay open.
* **Sample rate.** The `World` is born at the DSP chain's rate (`sys_getsr`, which is
  valid with DSP off) and the sclang side is aligned to it during the boot, so
  `s.sampleRate` tells the truth. Changing the project rate with the patch open is
  reported, not followed live: `close` then `open` recreates the `World`.
* **A second `sc3~` attaches to the running engine.** The process boots once, and each
  instance reads its own range of the single 64-bus pool from the same stream: the engine
  renders whatever is missing and the others read those same samples, so a second instance
  costs neither a clock nor another socket.
* **The class library runs without Qt.** Text, warnings and `post` are captured through the
  public embedding API (`SC_LanguageClient`); a small Qt-free module is added to the class
  library's include path at boot, so `s.boot` / `s.quit` are no-ops, a bare `.play` lands on
  the instance's buses and `Server.default` points at the engine.

## `sc3~` reference

### Arguments

| syntax | meaning |
|---|---|
| `sc3~ n` | `n` output channels (default 2, max 16), served buses `0 … n-1` |
| `sc3~ n bus` | `n` output channels, served buses `bus … bus+n-1` (lets two instances coexist) |

Several `sc3~` in one patch share the same engine: the second one attaches instead of
booting (send it `open`, which is instant, or just evaluate from it). An instance that is
not attached is silent, even when another one is running.

The code evaluated in the object gets `~sc3out` (its base bus) and `~sc3n` (its channel
count) injected in the top environment, so `Ndef(...).play` and friends land on the right
bus by default.

### Inlets and outlets

* **Inlet 1 — messages.** Call a signal inlet *after* the message inlet; see `@ins`.
* **Inlets 2 … N+1 — audio input** (only with `@ins N`). `SoundIn.ar(i)` reads inlet `i`.
* **Outlets 0 … n-1 — audio**: channel `0` … channel `n-1`.
* **Outlet n — `error`** (red): engine failures.
* **Outlet n+1 — `post`**: the SC console (polls, warnings, `post` output).

> **Patching a 0.15 patch?** The two text outlets moved to the right in 0.16.0, so a patch
> cord follows its channel. `[sc3~ 2]` used to be `error`, `post`, `ch1`, `ch0`; it is now
> `ch0`, `ch1`, `error`, `post`. Move each cord to the matching position.

The `error` and `post` outlets are **optional**. By default the same text is mirrored to
**Max's console** with the SC IDE's look (raw lines, no object prefix), so a lone `[sc3~]`
shows its `post` output, warnings and errors with nothing patched. Wire `[print]` to them
only if you want to route the text yourself, and set `console 0` first so each line is
printed once instead of twice. `console` is **per instance**: a patch that silences its
own mirroring never silences the console of another patch. The lifecycle lines
(`open`/`close`/`panic`/`hardstop`) are the exception: they always print to the console,
because with `console 0` a slow boot or a failed `close` would look like a dead object.

### Attributes

| attribute | default | meaning |
|---|---|---|
| `@port N` | `0` | UDP port of the engine's World. `0` = automatic (the kernel picks a free one). Saved with the patch, applied on `open`/`close`. |
| `@ins N` | `0` | number of audio input signal inlets. Fixed at creation: reload the object to change it. |
| `@allowscversion 1` | `0` | accept an installed SuperCollider with a different minor version (only relevant with `SC3_APP_PATH`; the bundled payload is pinned). |
| `@console 0` | `1` | mirror the engine text to Max's console (same state as the `console` message, but saved with the patch). `0` = outlets only. |

### Messages

| message | effect |
|---|---|
| `open` | boot the engine (asynchronous; `info` reports the state) |
| `bang` | boot it if it is not up yet (or if the last boot failed), then post the state |
| `close` | shut the engine down |
| `info` | post the current state (version, port, sr, channels, nodes) |
| `eval <code…>` | evaluate an expression (`( … )` for several statements) |
| `evalfile <path>` | evaluate the contents of a file |
| `send <object> <args…>` | send a message to an object in the interpreter |
| `panic`, `stop` | `CmdPeriod.run`: stops nodes, clocks and hooks (the engine stays up) |
| `hardstop` | `CmdPeriod.hardRun`: `panic` plus `TempoClock.default` |
| `freeall` | `s.freeAll`: nodes only, clocks and Routines keep running |
| `console 0\|1` | same as `@console` (per instance): `1` = also mirror to Max's console, `0` = outlets only |
| `debug 0\|1` | enable/disable the eval dump |
| `port <n>` | same as `@port` |

### Audio input

With `[sc3~ @ins 2]`, inlets 2 and 3 are audio: `SoundIn.ar(0)` and `SoundIn.ar(1)` read
them. Only one object per World owns the input ring, and the path adds two blocks of
latency by design (the ring is prefilled with silence).

## `sc_code` reference

Wire its outlet to `sc3~`'s message inlet: every evaluation leaves as a single
`eval <code>` message (the code travels as **one symbol atom**, so line breaks, commas and
quotes reach sclang intact and nothing needs escaping). Expanding a `snip.<key>` sends
nothing at all — the next `⌘Return` evaluates the expanded text.

`evalfile` is a `sc3~` message too, but the editor does not use it: evaluating never
writes a file.

| keys | action |
|---|---|
| `⇧Return` | evaluate the current line |
| `⌘Return` | evaluate the selection, else the enclosing `( … )` region, else the line |
| `⌘⇧Return` | evaluate the whole buffer |
| `⌃.`, `⌘/` | panic (`⌘.` is Max's own stop and cannot be taken) |
| `⌘Z` / `⇧⌘Z` | undo / redo |
| `⌘X` `⌘C` `⌘V` | cut / copy / paste |
| `Tab` | insert the suggestion shown in the bar; at a clean start of an argument it writes the IDE template (`freq: `); with no suggestion it indents |
| `⇧Tab` | cycle to the next suggestion, or outdent |
| `Esc` | close the suggestions |

Right-click opens the context menu (undo, clipboard, select all, save to `.scd`, load
from `.scd`). Saving and loading live there: the editor leaves `⌘S` and `⌘O` to Max.
Like the SC IDE, only `( … )` counts as an evaluation region: `{ … }` and `[ … ]` spanning
several lines are not, so select them or use `⌘⇧Return`.

Attributes: `@fontsize`, `@tabwidth`, `@linenumbers`, `@readonly`, `@autoclose`,
`@autowrite`, `@autoload`, `@file`, `@snippets` (a `### key` + verbatim body file),
`@wrap`, `@matchbrackets`, `@titlebar`, `@callaid` (the bottom bar, `1` by default),
`@barcolor`, `@barhighlight` (the current argument), `@indexfile` (the class library
dump), the per-token colours (`@keywordcolor`, `@stringcolor`, `@commentcolor`, …),
`@bgcolor`, `@textcolor`, `@selcolor`, `@caretcolor`, `@curlinecolor` (the line of the
caret), `@blink`, `@blinkcolor`, `@blinkduration`, `@scrollspeed`.

Colours: **`@theme`** sets the whole palette in one step — `onedark` (default) is
`#282c34`/`#abb2bf` with violet keywords; also `catppuccin`, `dracula`, `nord`, `gruvbox`,
`monokai` and `solarized`. It is a **menu in the inspector** (the list is generated from the
same table), any name or alias can be typed, and `theme dracula` works through the inlet
(`themes` posts the list). A colour set by hand wins over the theme, so
`sc_code @theme nord @bgcolor 1 0 0 1` is nord with a red background.

### Call aid and the class library index

`@callaid 1` (default) **reserves** one line at the bottom of the box. With the caret
inside a call it shows that call's signature, the current argument highlighted and its
position (`SinOsc.ar(freq, phase, mul, add)` with `2/4` on the right); with the caret on a
written name (`Duty.kr`) it shows the same signature without the counter, and `Ndef(`
resolves the constructor (`Ndef.new(key, object)`). The line is reserved instead of popped
up, so the caret can never end up hidden behind it. `@callaid 0` gives the line back to
the text.

The signatures come from the **class library index**, a text file that `sclang` writes
once:

* wire the editor's outlet into `sc3~` and send `index` to `sc_code`: sclang dumps every
  class, method and argument name (`index <path>` writes elsewhere, one shot).
* one file per installation (`~/Documents/Max 9/Library/sc_code-index.txt`) shared by every
  `sc_code`; `@indexfile <path>` points at a dump kept somewhere else.
* regenerate it after installing Quarks, editing Extensions or updating SuperCollider:
  methods added at runtime are not in the dump.
* the editor only **reads** the file (`fopen`, re-parsed when `mtime`/`size` change), so the
  bar works even before the engine is booted. Without the file the bar stays empty and
  the console says it once.

### Completion

Completion is **display-only**: typing never rewrites the buffer. From three letters on,
the bar shows the best candidate for the token under the caret — after `Class.` only the
methods of that class and its superclasses; anywhere else classes, the words of the
document and the language keywords. `Tab` inserts it (one undo step), `⇧Tab` cycles,
`Esc` closes, and typing filters (the bar closes itself when nothing is left). `Return` is
never taken: it still evaluates or breaks the line. `⌃Space` opens the full list with no
minimum prefix, on machines where macOS delivers that key to Max.

`Tab` at a clean start of an argument writes the IDE's template instead: inside
`SinOsc.ar(` it writes `freq: `, and after the comma the next one (`phase: `). Values are
left to you: the default arguments are not in the index.

### Snippets file

`@snippets <path>` (or the `snippets <path>` message) points the editor at a plain-text
file of blocks. Type the path in Max's **object inspector** (the `@snippets` attribute) or
send it as a message — either way the path is **saved with the patch**, so reopening the
patch points the editor at the same file again. The file is re-read by `mtime`/`size`
every time you use it — if it disappears, the copy already loaded stays.

Relative, absolute and `~` paths all work, and one pair of quotes is accepted and
stripped (`snippets '~/path with spaces/my.txt'`, `@snippets "~/x.txt"`), which is the
only way to feed a path with spaces from a message box.

```text
### init
(
s = Server.default;
s.options.numOutputBusChannels = 2;
)

### ramp
{ SinOsc.ar(220) * Line.kr(0, 0.2, 3) }.play;
```

Format rules — deliberately plain: a block body is copied verbatim, so the format has no
escape sequences and nothing in it is evaluated.

* A header is a line that starts **exactly** with `### ` (column 0) followed by the key.
  Keys are `[A-Za-z0-9_]` only: no spaces, dashes or accents. A line starting with `###`
  whose key is not valid is **not** a header — it becomes body text of the block above.
* The body is everything from the next line up to the next header, **verbatim**. Blank
  lines at the edges of the block are trimmed; the interior and its indentation are kept.
* Text before the first header is ignored. Duplicate keys: the first one wins. A header
  with an empty body is ignored. CRLF line endings and a UTF-8 BOM are tolerated.

To use one, type `snip.<key>` in the buffer and press `⌘Return`: the text is replaced by
the body (caret at the end, a single undo step) and **nothing is evaluated** — the next
`⌘Return` evaluates the expanded text. A trailing `;` is allowed. Unknown key: the buffer
is left untouched and Max's console lists the keys that are available.

The release ships a working example: `examples/sc_code-snippets.txt`, four `Ndef`s (`\a` …
`\d`, keys `1` … `4`). Nothing has to be set up first; point the editor at it (inspector or
message) and try `snip.1` … `snip.4`.

## Known limits

* **The engine has no GUI.** Qt is not linked into the bundle, so `plot`, `Window` and any
  code that opens a window fails with an error. UI belongs to Max, `eval` to the logic.
  `MouseX` / `MouseY` / `MouseButton` do work — they read the **screen** mouse, not the
  patch.
* **`⌘.` is not caught** by the objects, and no external can catch it: Max handles it
  first, as *stop scheduler*. Out of SC habit it is the easiest way to freeze the whole
  patch. Use the `panic` message, or `⌃.` / `⌘/` in `sc_code`.
* **One expression per `eval`**: `( … )` for a block, but two sibling `( … )` blocks in one
  call is a parse error.
* **Inside a pushed `ProxySpace`, `~sc3out` is not the bus.** There it is a JITLib
  variable again, so `LazyEnvir` answers with a `NodeProxy` and `Out.ar(~sc3out, sig)`
  is *silent* — no error, nodes are created. A bare `.play` / `.playN` is unaffected:
  those read the bus from the interpreter's `topEnvironment`. Inside a space use
  `~out.play`, `topEnvironment.at(\sc3out)`, or capture the bus before the `push`
  (`b = ~sc3out; p = ProxySpace.push(s.boot); … Out.ar(b, sig)`). A `.play` deferred
  from a `Routine` or a clock needs `out:` explicitly: the bus belongs to the eval.
* `Buffer:setn` takes pairs: `setn(0, [ … ])`.
* The user's `startup.scd` runs **twice** when the SC IDE is also open (one sclang per
  app). Harmless, but it explains duplicated posts.
* The engine picks a free UDP port by default, so the SC IDE and its server can stay open
  at the same time. Forcing `@port 57110` puts you back in the classic single-server
  collision.

## macOS Gatekeeper

The release is signed **ad-hoc**, so a freshly downloaded bundle is quarantined. Clear
the flag on the package once after moving it into place:

```bash
xattr -dr com.apple.quarantine ~/Documents/Max\ 9/Packages/sc3-tilde
```

Builds from source on your own machine have no quarantine flag and need nothing extra.

## Building from source

Requires Xcode Command Line Tools, CMake and the Max SDK (`max-sdk-base`, expected in
`~/Documents/Max 9/Packages/max-sdk/source/max-sdk-base`, or set `MAX_SDK`).

```bash
# 1. SuperCollider, pinned to the version the engine is compiled against
git clone --branch Version-3.14.1 --recurse-submodules \
  https://github.com/supercollider/supercollider.git vendor/supercollider
cmake -S vendor/supercollider -B vendor/supercollider/build
cmake --build vendor/supercollider/build -j"$(sysctl -n hw.ncpu)"

# 2. the engine (builds, stages the payload, signs ad-hoc and installs bundle + help)
cd external && ./build.sh

# 3. the editor
cd external/sc_code && make bundle install

# 4. or install the package the release ships, instead of 2 + 3
cd .. && cmake --build build --target install-package
```

`build.sh` needs an installed `SuperCollider.app`: `scripts/stage_bundle.sh` takes the
payload from `Contents/Resources` and the libsndfile copy from `Contents/Frameworks`, and
leaves both inside the `.mxo` (libsndfile gets `@loader_path` and a fresh ad-hoc signature).
Override the source with `SC_APP=…`.

The same recipe has a CMake path, used for the distributable package:

```bash
cmake -S . -B build && cmake --build build -j
cmake --build build --target package            # build/package/sc3-tilde
```

Other knobs: `MAX_SDK`, `SC3_OUTDIR` (install destination), `SC3_ARCH`, `SC3_MINOS`.

### x86_64 (Intel)

The scripts default to arm64; they are not tied to it. On an Intel Mac (or to cross-build)
pass the architecture through and build SuperCollider for the same slice:

```bash
cmake -S vendor/supercollider -B vendor/supercollider/build -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build vendor/supercollider/build -j"$(sysctl -n hw.ncpu)"
cd external && SC3_ARCH=x86_64 SC3_MINOS=11.0 ./build.sh
cd sc_code && SC_CODE_ARCH=x86_64 SC_CODE_MINOS=11.0 make bundle install
```

Expect differences: this slice is not tested by the project's CI, so treat it as
"builds and boots" rather than "supported".

## Tests

The engine and the editor are tested headless (no Max needed): 183 engine checks, a 9-case
failure matrix (broken Quark, throwing `startup.scd`, corrupt `.scx`, port mismatch,
impossible boot, eval that throws), the payload checks (a user Extension and a user Quark
compiled on top of the bundled class library, the degraded boot, and audio from both) and
1367 editor checks. The suites and their fixtures are in this tree.

The two that need nothing but a compiler:

```bash
cd external && make test          # engine, instance matrix, payload (needs SC 3.14.1)
cd external/sc_code && make test  # 1367 editor checks, no SC and no Max
```

And the closing gate, which also compiles both bundles (this is the one to run before
opening a pull request; it needs the Max SDK, see `BUILD.md`):

```bash
cd external && make gate          # reference pages + suites + both bundles + statics
ctest --test-dir build            # or through CMake, after `cmake -S . -B build`
```

CI runs the suites, the reference-page check, the statics' architecture check and the
language/license gates on every pull request. Two things it cannot check and are done by
hand: Max's GUI (there is no headless loader for a jbox) and the x86_64 slice.

## Credits

The Max package layout, the CMake build, the self-contained payload and the tabbed help
patches come from [shakfu](https://github.com/shakfu)'s fork, proposed in
[issue #1](https://github.com/gus74vv/sc3-tilde/issues/1); the channel labels, the outlet
order and the `nm -m` symbol check are items of that same list. They were ported by
reimplementing them in this tree, with his authorship on the commits that carry his code
and text.

## License

GPL-3.0-only — see [LICENSE](LICENSE). The engine links SuperCollider's statics
(`libsclang`, `libscsynth`) and carries its class library, UGens and sounds, which are
GPL-3.0, plus libsndfile (LGPL-2.1) and the libraries those link. Their notices travel with
the bundle: `licenses/` is inside `sc3~.mxo/Contents/Resources/` and in the release zip, and
`licenses/README.md` maps each component to its license and file. Sources are published with
every release.

## Contributing

Pull requests are welcome and are merged into this tree as they come. A patch is easier to
take if it runs the gate first:

```bash
cd external && make gate                                   # suites + both bundles + statics
cd external/sc_code && make test && cd .. && python3 tools/spdx.py check
python3 tools/i18n_scan.py --strict                        # user-facing text is in English
```

Every source file carries an SPDX line (`GPL-3.0-only`, or the matching one for vendored
code); the gates above fail without it. Contributions come in under the same license as
the rest of the tree.

## Support

A little coffee goes a long way.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/T0W629WR81)
