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
  "rect": [
   230.0,
   126.0,
   693.0,
   760.0
  ],
  "boxes": [
   {
    "box": {
     "id": "obj-1",
     "maxclass": "newobj",
     "numinlets": 3,
     "numoutlets": 4,
     "outlettype": [
      "",
      "",
      "signal",
      "signal"
     ],
     "patching_rect": [
      141.0,
      834.0,
      110.0,
      22.0
     ],
     "text": "sc3~ @ins 2"
    }
   },
   {
    "box": {
     "id": "obj-2",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20.0,
      20.0,
      70.0,
      22.0
     ],
     "text": "loadbang"
    }
   },
   {
    "box": {
     "id": "obj-3",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20.0,
      55.0,
      60.0,
      22.0
     ],
     "text": "open"
    }
   },
   {
    "box": {
     "id": "obj-4",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      90.0,
      55.0,
      60.0,
      22.0
     ],
     "text": "info"
    }
   },
   {
    "box": {
     "id": "obj-5",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20.0,
      90.0,
      60.0,
      22.0
     ],
     "text": "panic"
    }
   },
   {
    "box": {
     "id": "obj-6",
     "maxclass": "sc_code",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      381.0,
      8.0,
      636.0,
      428.0
     ],
     "sc_code_caret": 2732,
     "sc_code_scroll": 629,
     "sc_code_text": "// input_probe.scd \u2014 Phase 3.8: does signal reach INPUT bus 1 (inlet 2 of [sc3~])?\n//\n// Context (handoff notes, input smoke): the `input.maxpat` patch wires\n//\n//   [adc~ 1]  \u2500\u2500\u25ba inlet 1 \u2192 input bus 0 \u2192 SoundIn.ar(0)  \u2192 LEFT channel\n//   [cycle~]  \u2500\u2500\u25ba inlet 2 \u2192 input bus 1 \u2192 SoundIn.ar(1)  \u2192 RIGHT channel\n//\n// and the user still hears only ONE channel even though `Ndef(\\ringmod).numChannels` = 2.\n// Engine and inlet\u2192bus mapping are proven headless; what is missing is measuring IN MAX whether\n// inlet 2 carries signal.\n//\n// HOW TO USE IT (in `[sc_code]`): SELECT a `( \u2026 )` block and hit `\u2318\u23ce`.\n//   \u00b7 There is no `var` in this file on purpose: sclang only accepts a block with\n//     `var` declarations when the `(` is the FIRST character of the evaluated text\n//     (SCODE.md \u00a77.19) \u21d2 if the selection starts on the comment line, it fails\n//     with \"unexpected VAR, expecting ')'\". With interpreter variables (`~x`) it does not\n//     matter where the selection starts.\n//   \u00b7 Multi-line `{ \u2026 }` is NOT a region: if you leave the caret inside and select nothing,\n//     `\u2318\u23ce` evaluates one line and fails with `unmatched '}'`.\n//\n// BEFORE MEASURING: Max DSP on (without DSP there is no input: 3.8) and the object\n// has to be `sc3~ @ins 2` (check it with the `info` message: the \"3.8:\" line).\n\n// \u2500\u2500 1. World config: how many INPUT buses it has \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n// If it says 1, bus 1 does not exist \u21d2 SoundIn.ar(1) stays silent (the object warns at\n// boot time: \"@ins 2 but the World has N input buses\").\n(\ns.options.numInputBusChannels.postln;\ns.options.numOutputBusChannels.postln;\n)\n\n// \u2500\u2500 2. Poll to the console (no files involved; the console posts raw) \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n//  in1 \u2248 0  \u21d2 inlet 2 (cycle~) does NOT reach the engine  \u2192 cord / @ins / DSP\n//  in0 > 0  \u21d2 inlet 1 (mic) does reach it                \u2192 the object wiring is fine\n// The block sounds 0 (`* 0`): it measures without adding noise to the output.\n(\n~inprobe0 = { var s = SoundIn.ar(0); s.poll(0.25, \"in0\"); s * 0 }.play;\n~inprobe1 = { var s = SoundIn.ar(1); s.poll(0.25, \"in1\"); s * 0 }.play;\n)\n\n// \u2500\u2500 3. RMS to a file (in case the console gets lost among Max posts) \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n// Writes \"amp0 amp1\" 4 times per second to /tmp/sc3in.txt; then, in the\n// terminal: `tail -20 /tmp/sc3in.txt` (with cycle~ and the mic running).\n(\n~inprobePath = \"/tmp/sc3in.txt\";\nFile.delete(~inprobePath);\nOSCdef(\\sc3inprobe, { |msg|\n\tFile.use(~inprobePath, \"a\", { |f| f.write(\"% %\\n\".format(msg[3], msg[4])) });\n}, '/sc3in');\n~inprobe2 = Ndef(\\sc3in, {\n\tvar sig = SoundIn.ar([0, 1]);\n\tvar amp = [Amplitude.kr(sig[0], 0.01, 0.2), Amplitude.kr(sig[1], 0.01, 0.2)];\n\tSendReply.kr(Impulse.kr(4), '/sc3in', amp);\n\tsig * 0\n}).play(0, 2);\n)\n\n// \u2500\u2500 4. Cleanup \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n// (stops posting/writing; it does not touch the patch's Ndef)\n(\n~inprobe0.free; ~inprobe1.free;\nNdef(\\sc3in).clear; OSCdef(\\sc3inprobe).free;\n)\n"
    }
   },
   {
    "box": {
     "id": "obj-7",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "signal"
     ],
     "patching_rect": [
      565.0,
      600.0,
      80.0,
      22.0
     ],
     "text": "adc~ 1"
    }
   },
   {
    "box": {
     "id": "obj-8",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "signal"
     ],
     "patching_rect": [
      20.0,
      460.0,
      80.0,
      22.0
     ],
     "text": "cycle~ 220"
    }
   },
   {
    "box": {
     "id": "obj-9",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "signal"
     ],
     "patching_rect": [
      20.0,
      495.0,
      70.0,
      22.0
     ],
     "text": "*~ 0.3"
    }
   },
   {
    "box": {
     "id": "obj-10",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 0,
     "patching_rect": [
      196.0,
      878.0,
      50.0,
      22.0
     ],
     "text": "dac~"
    }
   },
   {
    "box": {
     "fontsize": 11.0,
     "id": "obj-11",
     "linecount": 16,
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      20.0,
      540.0,
      402.0,
      203.0
     ],
     "text": "sc3~ v0.14.0 \u2014 AUDIO INPUT (@ins N) with the sc_code editor\n\n@ins 2 adds 2 signal inlets TO THE RIGHT of the message inlet:\n  inlet 1 \u2192 messages (open, info, eval, panic\u2026)\n  inlet 2 \u2192 SoundIn.ar(0)   (World input bus 0)\n  inlet 3 \u2192 SoundIn.ar(1)   (input bus 1)\n\n(The order is NOT cosmetic: with the messages last, Max 9.2 assigns every\n signal connection to the first inlet and SoundIn.ar(1) stays silent \u2014 bug #8\n of the handoff notes.)\n\nThe code does NOT go in message boxes: it is written in sc_code (loaded from\ntests/input_ringmod.scd) and its outlet emits `eval <code>` to sc3~.\n\u21e7Return evaluates the caret line \u00b7 \u2318Return the selection/region.\n\nMouseX/MouseY/MouseButton DO work, but they are the SCREEN mouse\n(normalized 0..1, 0 at the bottom), not the patch one. If you want patcher mouse:\n[ mousestate ] \u2192 sc3~ (the `set <bus>` message is missing), or a Max slider.\nIn the .scd modfreq goes as an argument: Ndef().set(\\modfreq, N) + \u21e7Return.\n\nWithout DSP there is no input (just as without DSP there is no output)."
    }
   }
  ],
  "lines": [
   {
    "patchline": {
     "destination": [
      "obj-10",
      0
     ],
     "source": [
      "obj-1",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-10",
      1
     ],
     "source": [
      "obj-1",
      1
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-3",
      0
     ],
     "source": [
      "obj-2",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-4",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-5",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-6",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      1
     ],
     "midpoints": [
      574.5,
      819.0,
      196.0,
      819.0
     ],
     "order": 0,
     "source": [
      "obj-7",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-9",
      0
     ],
     "source": [
      "obj-8",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      2
     ],
     "order": 0,
     "source": [
      "obj-9",
      0
     ]
    }
   }
  ],
  "autosave": 0
 }
}
