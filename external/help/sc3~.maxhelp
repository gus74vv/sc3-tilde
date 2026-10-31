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
                    "patching_rect": [ 165.5, 48.0, 37.0, 22.0 ],
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
                    "linecount": 59,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 642.0, 18.0, 600.0, 797.0 ],
                    "text": "sc3~ v0.14.0 — SuperCollider 3.14.1 embedded (sclang + scsynth in-process).\n\nARGUMENTS (multi-instance, 3.5):\n  [sc3~]         2 channels, buses 0 and 1        (compatible with 2.x patches)\n  [sc3~ n]       n channels, buses 0 .. n-1\n  [sc3~ n bus]   n channels, buses bus .. bus+n-1\n  n = 1..16 · bus + n <= 64 (out of range: console error, mute object)\n\nSeveral sc3~ share ONE World (one boot, one clock, one socket) and each one\nreads ITS bus range: [sc3~ 2 0] and [sc3~ 2 2] live in the same patch.\n\nPORT (4.x): the World binds a FREE (ephemeral) UDP port and its sclang uses that\nport (~s.addr.port tells it; info shows it). That is why sc3~ and the\nSuperCollider IDE coexist: scide keeps 57110 and there is no clash here.\n@port 57110 forces the classic one (and then it does fight the IDE). @port N / port N\n(0 = auto, the default) applies when binding: changing it asks for close + open.\nIt is fixed by the first sc3~ that boots: there is ONE World for the whole patch.\n\nOUTLETS (right to left): 0 = error · 1 = post · 2..n+1 = audio\nch n-1 … ch 0. The leftmost audio outlet is channel 0 = bus `bus`.\nInlets: inlet 1 = MESSAGES (open, info, eval…) and, with `@ins N`, the audio INPUT\nsignal inlets to its right: [sc3~ @ins 2] → inlet 2 = SoundIn.ar(0),\ninlet 3 = SoundIn.ar(1) (see the input block below). The messages→signals order\nis mandatory: it is the only one Max 9 routes correctly (bug #18 of PLAN.md).\n\nCode is sent from [sc_code] (its outlet emits `eval <code>`) or with\nmessage boxes: eval <code>. Code evaluated in an object receives ~sc3out (bus base of THAT instance) and\n~sc3n (channels): Out.ar(~sc3out, sig), and an Ndef .play goes only to the object's\nbus (no playN, no numChannels; an explicit out: wins).\n\nChannels: the signal decides how many buses it takes. MONO source => 1 bus => only the\nfirst outlet (SC behaviour, same as a mono into inlet 0 of\n[dac~]). To get both: SinOsc.ar(440) ! 2, SinOsc.ar([440, 660])\nor .play(numChannels: 2).\n\nThe rest of the Server is SHARED: panic, s.freeAll and the nodes are global,\njust like in the SC IDE (close only stops listening on that instance).\n\n`open` first: the boot (sclang + World, ~4 s) runs in the background and does NOT\nblock Max; the state arrives on the console (`booting` → `ready`). The class library and the\nUGens come from /Applications/SuperCollider.app (3.14.1).\n\nSC VERSION installed (4.2): what matters is the MINOR version. 3.14.2 or 3.14.7\nbehave like 3.14.1: the boot completes and the console says (\"same minor version: accepted\").\nA 3.13 / 3.15 / 4.x does NOT start by default (the installed class library is\ncompiled with this engine's primitives): the error names both versions and how to force it.\n@allowscversion 1 (or env SC3_ALLOW_SC_VERSION=1) forces it, with a warning. Either way\na smoke at startup builds a minimal SynthDef: if the interpreter cannot, the boot\nfails there with the cause (no mute engine is left behind).\n\nThe .mxo does NOT depend on Homebrew: its only non-Apple dep (libsndfile) travels in\nContents/Frameworks, copied from SC.app and referenced with @loader_path (4.1).\n\nDiagnostics (3.4): `info` says `engine=booting|ready|failed` and, if the boot failed,\nthe line `error: ...` (port busy, nonexistent class library, the sclang Server\npointing to another port). scsynth errors —a `.scx` that does not load,\nfor example— come out through the `error` outlet. NOTE (3.9): whatever scsynth PRINTS and is not a failure\n(a `.poll`, the synthdef load) comes out through the `post` outlet, in black; red stays\nreserved for `*** ERROR`, `ERROR`, `FAILURE IN SERVER` and `Exception`."
                }
            },
            {
                "box": {
                    "fontsize": 11.0,
                    "id": "obj-2",
                    "linecount": 34,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 15.0, 326.0, 640.0, 420.0 ],
                    "text": "messages:\n  open · close · info · bang\n  port <n>          (World UDP port; 0 = auto. Asks for close + open)\n  eval <code>        (one line; commas are ESCAPED: \\,)\n  evalfile <path>   (multi-line code; absolute path)\n  set <target> <key> <value> [...]  (Synth/Node args; \\ and , generated here)\n  panic / stop      (Cmd+. : CmdPeriod.run → nodes + clocks + hooks)\n  hardstop          (CmdPeriod.hardRun: includes TempoClock.default)\n  freeall           (nodes only: s.freeAll)\n\n`panic` is the SuperCollider IDE Cmd+. equivalent (kills the WHOLE Server,\nalso whatever other sc3~ are playing).\n\nMulti-instance: [sc3~ 2 0] and [sc3~ 2 2] in the same patch; in the first evaluate\nNdef(\\a, { SinOsc.ar(440) * 0.2 }).play and in the second Ndef(\\b, { Saw.ar(220) *\n0.2 }).play → each one comes out through its own channels (see tests/two.maxpat).\nAn Ndef with more channels than the object writes extra buses that object does NOT read.\n\nMulti-line code (as in the IDE): use [textedit] → [prepend eval] → sc3~ (see\ntests/editor.maxpat); the text travels whole, nothing to escape.\nA message box, on the other hand, is a lexer: ',' and ';' separate messages and '\\'\nis its escape character, so `x.set(\\freq, $1)` arrives as `x.set(freq\\, 81)` -- the\nsymbol marker is gone before any object sees the atom. For Synth/Node parameters\nuse `set`, which generates the '\\' and the commas:\n  [number] -> [prepend set x freq] -> sc3~     ==>  x.set(\\freq, 81);\n  set x freq 81 amp 0.2                        ==>  x.set(\\freq, 81, \\amp, 0.2);\n  (see tests/args.maxpat)\nFor raw code with symbols or commas, build the whole text as ONE symbol:\n[textedit] with outputmode 1 (verbatim, multi-line; tests/editor.maxpat) or\n[sprintf symout x.set(%cfreq%c %d)]  (92 = backslash, 44 = comma).\n  console 0|1       (1 = the text also goes to the Max console; 0 = outlets only, for patches with a wired [print])"
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 30.0, 175.0, 60.0, 22.0 ],
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
                    "patching_rect": [ 210.0, 300.0, 50.0, 22.0 ],
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
                    "numinlets": 3,
                    "numoutlets": 4,
                    "outlettype": [ "", "", "signal", "signal" ],
                    "patching_rect": [ 110.0, 220.0, 95.0, 22.0 ],
                    "text": "sc3~ 2 2 @ins 2"
                }
            },
            {
                "box": {
                    "fontsize": 11.0,
                    "id": "obj-10",
                    "linecount": 12,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 15.0, 603.0, 568.0, 154.0 ],
                    "text": "AUDIO INPUT (3.8): @ins N (0..16) adds N signal inlets TO THE RIGHT of the message inlet\n(inlet 1 = messages; inlets 2..N+1 = audio). The example object below is `sc3~ 2 2 @ins 2`: with `2 2` it sits in buses 2..3, so it does not collide with the plain `[sc3~]` that feeds `[dac~]` on buses 0..1 (to hear it, patch it to `[dac~ 3 4]`). Inlet 2+i → World INPUT bus i →\nSoundIn.ar(i) (which is In.ar(NumOutputBuses + i)). The order matters: with the messages last,\nMax 9.2 sends every connection to the first signal inlet (bug #18).\nThe World has s.options.numInputBusChannels input buses (2 by default): if you ask for more, the ones that do not\nexist stay silent and `info` says so. A single sc3~ per patch can be the input producer\n(the others listen to the same one, the push is not duplicated). Path latency: 2 World blocks (~2.7 ms).\nWithout DSP there is no input. MouseX/MouseY/MouseButton DO work: they read the SCREEN mouse\n(normalized 0..1 over the main display, 0 at the bottom, 1 at the top; MouseButton = 1 when clicking),\nnot the patcher one. Patch mouse = [mousestate] + message `set <bus>` (not implemented).\nFull example: tests/input.maxpat (code in [sc_code], not in message\nboxes) (adc~ 1 → inlet 2, cycle~ → inlet 3, ringmod with SoundIn)."
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