{
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 2,
            "revision": 0,
            "architecture": "x64",
            "modernui": 1
        },
        "classnamespace": "box",
        "rect": [ 118.0, 112.0, 1222.0, 725.0 ],
        "boxes": [
            {
                "box": {
                    "id": "obj-16",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 122.0, 48.0, 37.0, 22.0 ],
                    "text": "panic"
                }
            },
            {
                "box": {
                    "id": "obj-14",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 73.0, 48.0, 37.0, 22.0 ],
                    "text": "close"
                }
            },
            {
                "box": {
                    "id": "obj-12",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 26.0, 48.0, 35.0, 22.0 ],
                    "text": "open"
                }
            },
            {
                "box": {
                    "fontsize": 12.0,
                    "id": "obj-1",
                    "linecount": 55,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 642.0, 18.0, 600.0, 744.0 ],
                    "text": "sc3~ 0.15.4 — SuperCollider 3.14.1 embedded: sclang + scsynth in-process.\n\nARGUMENTS (ONE World per Max process):\n  [sc3~]          2 channels, buses 0..1        (compatible with older patches)\n  [sc3~ n]        n channels, buses 0..n-1\n  [sc3~ n bus]    n channels, buses bus..bus+n-1\n  n = 1..16 and bus + n <= 64. Out of range: console error, the object stays muted.\n\nSeveral sc3~ SHARE that World (one boot, one clock, one socket) and each one reads\nITS bus range: [sc3~ 2 0] and [sc3~ 2 2] live in the same patch. Each instance has\nto be ATTACHED to the World: `open` does it (with the World already up it does not\nboot again, it joins) and so do `eval`, `set`, `evalfile` and `panic`. An instance\nthat never attached stays silent, even while another one is playing. `close`\ndetaches it; the World only goes down with the LAST owner. panic, s.freeAll and the\nnodes are shared, as in the SC IDE: they hit every instance.\n\nPORT: the World binds a FREE UDP port (0 = auto, the default) and its sclang\nfollows it — `info` prints it, ~s.addr.port. That is why sc3~ and the SuperCollider\nIDE coexist: scide keeps 57110, no clash. @port 57110 forces the classic port and\nthen it does fight the IDE. The port belongs to the World: it is fixed by the first\nsc3~ that boots, and changing @port asks for close + open.\n\nOUTLETS (right to left): 0 = error · 1 = post · 2..n+1 = audio, channel n-1 first\ndown to channel 0 (the leftmost audio outlet is bus `bus`).\nINLETS: 1 = MESSAGES and, with @ins N, N audio inputs to its right\n(2 = SoundIn.ar(0), 3 = SoundIn.ar(1) …, see the input block below). The message\ninlet must be created FIRST: otherwise Max 9 sends every connection to the first\nsignal inlet.\n\nCODE: from [sc_code] (its outlet emits `eval <code>`) or a message box. An eval\ngets ~sc3out (bus base of THAT instance) and ~sc3n (its channels):\nOut.ar(~sc3out, sig). A bare Ndef .play also lands on this instance's buses (no\nplayN, no numChannels); an explicit out: wins.\n\nCHANNELS: the signal decides how many buses it takes. A MONO source uses ONE bus,\nso only the first outlet sounds (SC behaviour, same as a mono into [dac~]). For\nboth: SinOsc.ar(440) ! 2, SinOsc.ar([440, 660]) or .play(numChannels: 2).\n\nBOOT: `open` (or a bang, or the first eval) starts sclang + the World in the\nbackground, ~4 s, without blocking Max; the console goes `booting` → `ready`. The\nclass library and the UGens come from /Applications/SuperCollider.app.\n\nSC VERSION: the MINOR must match the engine's (3.14.x). 3.14.2 or 3.14.7 are\naccepted with a note; 3.13 / 3.15 / 4.x only start with @allowscversion 1 (or\nSC3_ALLOW_SC_VERSION=1) plus a warning. The boot ends with a SynthDef smoke test: if\nthe interpreter cannot build it, the boot fails there with the cause.\n\nThe .mxo does not need Homebrew: its only non-Apple dependency (libsndfile) travels\nin Contents/Frameworks, referenced with @loader_path.\n\nDIAGNOSTICS: `info` gives engine=booting|ready|failed, the cause if it failed,\nsr/blockLen/units, dsp=, console=, queued= and busy= (ms of the eval in flight; 10 s\n= stuck). Anything scsynth prints that is not a failure (a .poll, a synthdef load)\ncomes out of `post` in black; red on `error` is only for *** ERROR, ERROR, FAILURE\nIN SERVER and Exception."
                }
            },
            {
                "box": {
                    "fontsize": 11.0,
                    "id": "obj-2",
                    "linecount": 37,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 15.0, 324.0, 640.0, 461.0 ],
                    "text": "MESSAGES\n  open · close · info · bang\n  eval <code>            one line; `send` and `set` skip the escaping below\n  send <target> <args>   SC message, no quotes: send s.freeAll, send CmdPeriod.run\n  set <target> <key> <value> [...]   Synth/Node args, built here:\n                         set x freq 81 amp 0.2  ==>  x.set(\\freq, 81, \\amp, 0.2)\n  evalfile <path>        multi-line code from a file (absolute path)\n  panic / stop           Ctrl+Period equivalent: CmdPeriod.run (nodes, clocks, hooks)\n  hardstop               CmdPeriod.hardRun: also TempoClock.default\n  freeall                nodes only: s.freeAll\n  port <n>               World UDP port (0 = auto); asks for close + open\n  console 0|1            also mirror the engine text to the Max console (default 1)\n  debug 0|1              dump every eval to /tmp/sc3_eval_last.scd\n\n`panic` kills the WHOLE Server, whatever other sc3~ are playing. While an eval does\nnot return, `info` shows busy=<ms>; after 10 s the engine counts as stuck: eval,\nsend, set and evalfile are dropped with an error and `close` is refused — restart Max.\n\nMULTI-INSTANCE: [sc3~ 2 0] and [sc3~ 2 2] in the same patch; eval\nNdef(\\a, { SinOsc.ar(440) * 0.2 }).play in the first and\nNdef(\\b, { Saw.ar(220) * 0.2 }).play in the second ⇒ each one comes out of its own\nchannels (tests/two.maxpat). Evaluating in the second one is enough: it attaches to\nthe running World; `open` does the same, instantly. An Ndef with more channels than\nthe object writes extra buses that object does NOT read.\n\nMULTI-LINE CODE: [textedit] → [prepend eval] → sc3~ (tests/editor.maxpat): the text\ntravels whole, nothing to escape. A message box is a lexer — ',' and ';' separate\nmessages and '\\' is its escape — so eval x.set(\\freq, $1) arrives as\nx.set(freq\\, 81). Use `set` for parameters; for raw text with symbols or commas send\nONE symbol: [textedit] with outputmode 1 (tests/editor.maxpat) or\n[sprintf symout x.set(%cfreq%c %d)] (92 = backslash, 44 = comma) (tests/args.maxpat).\n\nconsole 0|1: 1 = the engine text also goes to the Max console (default, like the IDE\npost window); 0 = only the post/error outlets, for patches with a wired [print]. It\nis per instance and the state lives in @console, saved with the patch, so it never\nmutes another patch. The lifecycle lines (open/close/panic/hardstop) always print:\nwith 0 a slow boot or a failed close would look like a dead object."
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 122.0, 126.0, 37.0, 22.0 ],
                    "text": "info"
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 100.0, 175.0, 110.0, 22.0 ],
                    "text": "eval 1+1"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 4,
                    "outlettype": [ "", "", "signal", "signal" ],
                    "patching_rect": [ 30.0, 220.0, 60.0, 22.0 ],
                    "text": "sc3~"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 0,
                    "patching_rect": [ 216.0, 293.0, 50.0, 22.0 ],
                    "text": "dac~"
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 112.5, 293.0, 90.0, 22.0 ],
                    "text": "print post"
                }
            },
            {
                "box": {
                    "id": "obj-8",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 15.0, 293.0, 90.0, 22.0 ],
                    "text": "print error"
                }
            },
            {
                "box": {
                    "id": "obj-9",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 4,
                    "outlettype": [ "", "", "signal", "signal" ],
                    "patching_rect": [ 110.0, 220.0, 95.0, 22.0 ],
                    "text": "sc3~ 2 2 @ins 1"
                }
            },
            {
                "box": {
                    "fontsize": 11.0,
                    "id": "obj-10",
                    "linecount": 18,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 15.0, 798.0, 568.0, 215.0 ],
                    "text": "AUDIO INPUT: @ins N (0..16) adds N signal inlets TO THE RIGHT of the message\ninlet (1 = messages, 2..N+1 = audio). The order is mandatory: with the message\ninlet last, Max 9 sends every connection to the first signal inlet.\nThe object below is `sc3~ 2 2 @ins 1`: ONE audio input (inlet 2 → SoundIn.ar(0))\nand buses 2..3, so it does not collide with the plain [sc3~] that feeds [dac~] on\nbuses 0..1; patch its audio outlets to [dac~ 3 4] to hear it.\nInlet 2+i → World input bus i → SoundIn.ar(i), which is In.ar(numOutputBuses + i).\nThe World has s.options.numInputBusChannels input buses (2 by default); asking for\nmore leaves the missing ones silent and `info` says so.\nOne sc3~ per patch can be the input producer; the others listen to the same one, the\npush is not duplicated. Latency: 2 World blocks (128 samples). Without DSP there\nis no input at all.\nMouseX / MouseY / MouseButton do work: they read the SCREEN mouse (normalized 0..1\nover the main display, 0 at the bottom, 1 at the top; MouseButton = 1 while\nclicking), not the Max patch mouse. To drive SC from the patch use `set` (Synth\nargs) or `eval`.\nFull example (two inputs, @ins 2): tests/input.maxpat — adc~ 1 → inlet 2,\ncycle~ → inlet 3, ring mod with SoundIn."
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "destination": [ "obj-5", 0 ],
                    "source": [ "obj-12", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-5", 0 ],
                    "source": [ "obj-14", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-5", 0 ],
                    "source": [ "obj-16", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-5", 0 ],
                    "source": [ "obj-3", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-5", 0 ],
                    "source": [ "obj-4", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-6", 0 ],
                    "source": [ "obj-5", 3 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-6", 1 ],
                    "source": [ "obj-5", 2 ]
                }
            }
        ],
        "autosave": 0
    }
}