<!-- SPDX-License-Identifier: GPL-3.0-only -->
# sc3~ — SuperCollider inside Max

Two Max externals that bring the SuperCollider engine and an SC-style code editor into a
Max patch, with no external process, no IPC and no SuperCollider GUI:

| object | version | what it is |
|---|---|---|
| `sc3~` | 0.15.1 | Runs **sclang + scsynth in-process** inside Max. You send it SuperCollider code; the audio lands on Max's own audio buses, so it can be patched, mixed and processed like any other Max signal. |
| `sc_code` | 0.9.3 | An in-patch code editor with the SuperCollider IDE's evaluation UX (`⇧Return`, `⌘Return`, `⌘⇧Return`), syntax highlighting, undo, auto-indent, bracket matching, snippets and a `.scd` open/save menu. Its outlet sends the evaluated block to `sc3~`. |

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
* **SuperCollider 3.14.1 installed** at `/Applications/SuperCollider.app`.

The SuperCollider class library and the stock `.scx` plugins are **not** bundled: the
engine compiles the installed class library and loads the installed UGen plugins, exactly
like the SC IDE does. That keeps the download small and makes every Quark and plugin of
your installation available inside Max.

Version policy: the same `major.minor` is accepted (3.14.2, 3.14.7 … with a console note);
another minor version (3.13, 3.15 …) refuses to boot unless you pass `@allowscversion 1`.
Boot also builds a tiny smoke `SynthDef`, so a library that does not actually work fails
at boot with a real error instead of leaving a silent engine.

## Install

1. Copy `sc3~.mxo` and `sc_code.mxo` into any folder located within Max's Search Path — the **User Library** (~/Documents/Max 9/Library) is always in the path.
2. Copy `sc3~.maxhelp` and `sc_code.maxhelp` into `~/Documents/Max 9/Help`.
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

> **Warning for SuperCollider users:** do **not** press `⌘.` in Max to kill the sound.
> In Max that is *stop scheduler*, which halts Max's own scheduler and the whole patch
> stops responding. Use the `panic` message (or `⌃.` inside `sc_code`), which touches
> the SC side only. If you already hit it, bring the scheduler back with Max's
> `Resume`.

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

Right-click opens the context menu (undo, clipboard, select all, save/load, panic).
Like the SC IDE, only `( … )` counts as an evaluation region: `{ … }` and `[ … ]` spanning
several lines are not, so select them or use `⌘⇧Return`.

Attributes: `@fontsize`, `@tabwidth`, `@linenumbers`, `@readonly`, `@autoclose`,
`@autowrite`, `@autoload`, `@file`, `@snippets` (a `### key` + verbatim body file),
`@wrap`, `@matchbrackets`, `@titlebar`, the per-token colours (`@keywordcolor`,
`@stringcolor`, `@commentcolor`, …), `@bgcolor`, `@textcolor`, `@selcolor`,
`@caretcolor`, `@blink`, `@blinkcolor`, `@blinkduration`, `@scrollspeed`.

### Snippets file

`@snippets <path>` (or the `snippets <path>` message) points the editor at a plain-text
file of blocks. The path is saved with the patch, and the file is re-read by
`mtime`/`size` every time you use it — if it disappears, the copy already loaded stays.

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
890 editor checks + a 9-case failure matrix (broken Quark, throwing `startup.scd`,
corrupt `.scx`, port mismatch, impossible boot, eval that throws). The suite is **not**
part of this tree, so there is no `make test` here — it lives with the development
repository, next to the design notes.

## License

GPL-3.0-only — see [LICENSE](LICENSE). The engine links SuperCollider's statics
(`libsclang`, `libscsynth`), which are GPLv3, so this bundle is a derived work. Sources are
published with every release.
