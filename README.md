<!-- SPDX-License-Identifier: GPL-3.0-only -->
# sc3~ — SuperCollider inside Max

Two Max externals that bring the SuperCollider engine and an SC-style code editor into a
Max patch, with no external process, no IPC and no SuperCollider GUI:

| object | version | what it is |
|---|---|---|
| `sc3~` | 0.15.6 | Runs **sclang + scsynth in-process** inside Max. You send it SuperCollider code; the audio lands on Max's own audio buses, so it can be patched, mixed and processed like any other Max signal. |
| `sc_code` | 0.9.11 | An in-patch code editor with the SuperCollider IDE's evaluation UX (`⇧Return`, `⌘Return`, `⌘⇧Return`), syntax highlighting, undo, auto-indent, bracket matching, snippets and a `.scd` open/save menu. A bottom bar shows the signature of the call under the caret (current argument highlighted) and offers completion: the editor *shows*, nothing is written while you type. Its outlet sends the evaluated block to `sc3~`. |

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
* **SuperCollider 3.14.1 installed** at `/Applications/SuperCollider.app` —
  [supercollider/supercollider](https://github.com/supercollider/supercollider).

The SuperCollider class library and the stock `.scx` plugins are **not** bundled: the
engine compiles the installed class library and loads the installed UGen plugins, exactly
like the SC IDE does. That keeps the download small and makes every Quark and plugin of
your installation available inside Max.

Version policy: the same `major.minor` is accepted (3.14.2, 3.14.7 … with a console note);
another minor version (3.13, 3.15 …) refuses to boot unless you pass `@allowscversion 1`.
Boot also builds a tiny smoke `SynthDef`, so a library that does not actually work fails
at boot with a real error instead of leaving a silent engine.

## Install

1. Copy `sc3~.mxo` and `sc_code.mxo` into any folder located within Max's Search Path — the
   **User Library** (`~/Documents/Max 9/Library`) is always in the path.
2. Copy `sc3~.maxhelp` and `sc_code.maxhelp` into `~/Documents/Max 9/Library/help`. For a
   shorter and more practical overview than the help patches, open `examples/example.maxpat`.
3. **Restart Max** (`.mxo` files are loaded when the app starts).

Builds are signed ad-hoc, so macOS Gatekeeper will block the download; see
[Gatekeeper](#macos-gatekeeper).

## Quick start

```
[sc3~ 2]  ──────────────►  [dac~]
```
* Send `open` (or a `bang`) to boot the engine.
* Send it code: `eval { SinOsc.ar(440) * 0.2 }.play;` — or patch an `sc_code` editor
  to the message inlet and press `⌘Return`.
* Whatever the engine prints shows up in **Max's console** on its own — no outlets, no
  `[print]` needed.
* `panic` runs `CmdPeriod.run` (nodes + clocks + hooks), `freeall` only frees the nodes;
  `close` shuts the engine down.

The release also ships **`examples/example.maxpat`**, a self-contained patch: it boots the
engine, plays a demo from the `sc_code` buffer, shows the `set` path for Synth arguments
from Max's UI and has a `panic` button (Ctrl+.). Its comments explain what to look at,
including the one-time `index` message.

> **Warning for SuperCollider users:** do **not** press `⌘.` in Max to kill the sound.
> In Max that is *stop scheduler*, which halts Max's own scheduler and the whole patch
> stops responding. Use the `panic` message (or `⌃.` inside `sc_code`), which touches
> the SC side only. If you already hit it, bring the scheduler back with Max's
> `Resume`.

## How it works

| thread | owns | does |
|---|---|---|
| Max audio (`perform`) | the SuperCollider `World` | drain OSC → `PerformOSC*` → `World_Run` → copy its buses to the DSP chain |
| engine worker | the sclang VM | boot, `eval`, `tick()` (AppClock), collect the posts |
| Max main | nothing of the engine | enqueue work, deliver the outlets |

* **One process, one engine.** `scsynth` and `sclang` are linked statically into the
  bundle, so there is no helper app behind the object. The `World` is created headless
  (`mRealTime = false`, no audio driver — the mode SuperCollider itself uses for offline
  renders) and it is `sc3~` who calls `World_Run` from the audio thread and copies its
  output buses into the DSP chain: SC's graph becomes ordinary Max signals.
* **OSC over loopback.** The embedded sclang still talks to the `World` with OSC, over
  loopback UDP through the engine's own socket — `World_OpenUDP` is unusable without a
  driver, it drops the datagram — with a reply address that answers back. Bundles that
  carry a timetag are queued and cleared by `/clearSched`. The `World` binds first (port
  `0` lets the kernel pick one, `@port N` forces it) and hands the real port to sclang
  before its library is compiled, which is why the SC IDE can stay open.
* **Sample rate.** The `World` is born at the DSP chain's rate (`sys_getsr`, which is
  valid with DSP off) and the sclang side is aligned to it during the boot, so
  `s.sampleRate` tells the truth. Changing the project rate with the patch open is
  reported, not followed live: `close` then `open` recreates the `World`.
* **One `World` per process.** A second `sc3~` does not boot again, it *attaches*: each
  instance reads its own range of the single 64-bus pool from the same stream (the engine
  renders whatever is missing and the others read those same samples), so two instances
  cost one clock and one socket.
* **No Qt.** Text, warnings and `post` are captured through the public embedding API
  (`SC_LanguageClient`). The class library is the installed one; a small Qt-free module is
  added to its include path at boot, so `s.boot` / `s.quit` are no-ops, a bare `.play`
  lands on the instance's buses and `Server.default` points at the engine.

## `sc3~` reference

### Arguments

| syntax | meaning |
|---|---|
| `sc3~ n` | `n` output channels (default 2, max 16), served buses `0 … n-1` |
| `sc3~ n bus` | `n` output channels, served buses `bus … bus+n-1` (lets two instances coexist) |

Several `sc3~` in one patch share **one** engine (one boot, one clock, one socket): the
second one does not boot again, it *attaches* — send it `open` (instant here) or just
evaluate from it, which attaches it. An instance that is not attached is silent, even
when another one is running.

The code evaluated in the object gets `~sc3out` (its base bus) and `~sc3n` (its channel
count) injected in the top environment, so `Ndef(...).play` and friends land on the right
bus by default.

### Inlets and outlets

* **Inlet 1 — messages.** Call a signal inlet *after* the message inlet; see `@ins`.
* **Inlets 2 … N+1 — audio input** (only with `@ins N`). `SoundIn.ar(i)` reads inlet `i`.
* **Outlet 1 — `error`** (red): engine failures.
* **Outlet 2 — `post`**: the SC console (polls, warnings, `post` output).
* **Outlets 3 … n+2 — audio**: channel `n-1` … channel `0`.

Outlets 1 and 2 are **optional**. By default the same text is mirrored to **Max's
console** with the SC IDE's look (raw lines, no object prefix), so a lone `[sc3~]` shows
its `post` output, warnings and errors with nothing patched. Wire `[print]` to them only
if you want to route the text yourself, and set `console 0` first so each line is
printed once instead of twice. `console` is **per instance**: a patch that silences its
own mirroring never silences the console of another patch. The lifecycle lines
(`open`/`close`/`panic`/`hardstop`) are the exception: they always print to the console,
because with `console 0` a slow boot or a failed `close` would look like a dead object.

### Attributes

| attribute | default | meaning |
|---|---|---|
| `@port N` | `0` | UDP port of the engine's World. `0` = automatic (the kernel picks a free one). Saved with the patch, applied on `open`/`close`. |
| `@ins N` | `0` | number of audio input signal inlets. Fixed at creation: reload the object to change it. |
| `@allowscversion 1` | `0` | accept an installed SuperCollider with a different minor version. |
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
| `panic`, `stop` | `CmdPeriod.run`: stops nodes, clocks and hooks (audio keeps running) |
| `hardstop` | panic and silence the DSP chain |
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
| `⌃.` | panic (`⌘.` never reaches the object: Max eats it) |
| `⌘Z` / `⇧⌘Z` | undo / redo |
| `⌘X` `⌘C` `⌘V` | cut / copy / paste |
| `⌘S` / `⌘O` | save / open `.scd` |
| `Tab` | insert the suggestion shown in the bar; at a clean start of an argument it writes the IDE template (`freq: `); with no suggestion it indents |
| `⇧Tab` | cycle to the next suggestion, or outdent |
| `Esc` | close the suggestions |

Right-click opens the context menu (undo, clipboard, select all, save/load, panic).
Like the SC IDE, only `( … )` counts as an evaluation region: `{ … }` and `[ … ]` spanning
several lines are not, so select them or use `⌘⇧Return`.

Attributes: `@fontsize`, `@tabwidth`, `@linenumbers`, `@readonly`, `@autoclose`,
`@autowrite`, `@autoload`, `@file`, `@snippets` (a `### key` + verbatim body file),
`@wrap`, `@matchbrackets`, `@titlebar`, `@callaid` (the bottom bar, `1` by default),
`@barcolor`, `@barhighlight` (the current argument), `@indexfile` (the class library
dump), the per-token colours (`@keywordcolor`, `@stringcolor`, `@commentcolor`, …),
`@bgcolor`, `@textcolor`, `@selcolor`, `@caretcolor`, `@blink`, `@blinkcolor`,
`@blinkduration`, `@scrollspeed`.

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
  bar works with no engine, no boot and no DSP. Without the file the bar stays empty and
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

Format rules — deliberately dumb: no escapes, no evaluation, no preprocessing.

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

* **No GUI in the engine.** There is no Qt: `plot`, `Window`, and any code that tries to
  open a window will fail. Use Max for UI, `eval` for logic. `MouseX` / `MouseY` /
  `MouseButton` do work — they read the **screen** mouse, not the patch.
* **`⌘.` is not caught** by the objects, and no external can catch it: Max handles it
  first, as *stop scheduler*. Out of SC habit it is the easiest way to freeze the whole
  patch. Use the `panic` message, or `⌃.` in `sc_code`.
* **One expression per `eval`**: `( … )` for a block, but two sibling `( … )` blocks in one
  call is a parse error.
* `Buffer:setn` takes pairs: `setn(0, [ … ])`.
* The user's `startup.scd` runs **twice** when the SC IDE is also open (one sclang per
  app). Harmless, but it explains duplicated posts.
* The engine picks a free UDP port by default, so the SC IDE and its server can stay open
  at the same time. Forcing `@port 57110` puts you back in the classic single-server
  collision.

## macOS Gatekeeper

The release is signed **ad-hoc**, so a freshly downloaded bundle is quarantined:

```bash
xattr -dr com.apple.quarantine ~/Documents/Max\ 9/Library/sc3~.mxo
xattr -dr com.apple.quarantine ~/Documents/Max\ 9/Library/sc_code.mxo
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

# 2. the engine (builds, signs ad-hoc and installs bundle + help)
cd external && ./build.sh

# 3. the editor
cd external/sc_code && make bundle install
```

`build.sh` needs an installed `SuperCollider.app` for `libsndfile` (it is copied from
`Contents/Frameworks` and given `@loader_path`); override the path with `SC_APP=…`.
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

The engine and the editor are tested headless (no Max needed): 156 engine checks +
1157 editor checks + a 9-case failure matrix (broken Quark, throwing `startup.scd`,
corrupt `.scx`, port mismatch, impossible boot, eval that throws). The suite is **not**
part of this tree, so there is no `make test` here — it lives with the development
repository, next to the design notes.

## License

GPL-3.0-only — see [LICENSE](LICENSE). The engine links SuperCollider's statics
(`libsclang`, `libscsynth`), which are GPLv3, so this bundle is a derived work. Sources are
published with every release.

## Support

A little coffee goes a long way.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/T0W629WR81)
