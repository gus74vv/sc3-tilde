#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generates external/tests/input.maxpat (Phase 3.8: audio input in sc3~).

The patch is the Max equivalent of the user's snippet:

    carrier = SoundIn.ar(0); modulator = SinOsc.ar(modfreq); carrier * modulator

with the microphone (or any Max source) coming in through the signal inlets of
`[sc3~ @ins 2]` instead of the CoreAudio device.

**The code is typed and evaluated in `[sc_code]`** (the project's editor), not in
message boxes: `sc_code` sends `eval <code>` out of its outlet and that goes to
the message inlet of `sc3~`.

    python3 tests/make_input_patch.py      # rewrites tests/input.maxpat
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

boxes = []


def box(maxclass, text, inl, outl, rect, outlettype=None, fontsize=None):
    b = {
        "id": "obj-%d" % (len(boxes) + 1),
        "maxclass": maxclass,
        "text": text,
        "numinlets": inl,
        "numoutlets": outl,
        "patching_rect": rect,
    }
    if outlettype is not None:
        b["outlettype"] = outlettype
    if fontsize is not None:
        b["fontsize"] = fontsize
    boxes.append({"box": b})
    return b["id"]


def comment(text, rect, fontsize=11):
    return box("comment", text, 1, 0, rect, fontsize=fontsize)


def message(text, rect):
    return box("message", text, 2, 1, rect, outlettype=[""])


def newobj(text, rect, inl=1, outl=1, outlettype=None):
    return box("newobj", text, inl, outl, rect, outlettype=outlettype)


# --- main object ------------------------------------------------------------
sc3 = box(
    "newobj",
    "sc3~ @ins 2",
    3,
    4,
    [240, 200, 110, 22],
    outlettype=["", "", "signal", "signal"],
)

loadbang = newobj("loadbang", [20, 20, 70, 22])
openmsg = message("open", [20, 55, 60, 22])
info = message("info", [90, 55, 60, 22])
panic = message("panic", [20, 90, 60, 22])

# --- the editor: the ringmod .scd is evaluated with ⇧Return / ⌘Return ----------
scd = os.path.join(ROOT, "tests", "input_ringmod.scd")
code = newobj("sc_code @file %s" % scd, [20, 130, 620, 260], inl=1, outl=1,
              outlettype=[""])

# --- audio source: the Mac mic through [adc~] ------------------------------
adc = newobj("adc~ 1", [60, 420, 80, 22], inl=0, outl=1, outlettype=["signal"])
# Any other sound source in Max can go to the same inlets.
cycle = newobj("cycle~ 220", [20, 460, 80, 22], inl=2, outl=1, outlettype=["signal"])
cycleamp = newobj("*~ 0.3", [20, 495, 70, 22], inl=2, outl=1, outlettype=["signal"])

dac = newobj("dac~", [260, 495, 50, 22], inl=2, outl=0)

comment(
    "sc3~ — audio input (@ins N) with the sc_code editor\n\n"
    "@ins 2 adds 2 signal inlets to the LEFT of the message inlet:\n"
    "  inlet 1 → SoundIn.ar(0)   (input bus 0 of the World)\n"
    "  inlet 2 → SoundIn.ar(1)   (input bus 1)\n"
    "  inlet 3 → messages (open, eval, panic…)\n\n"
    "The code does NOT go in message boxes: type it in sc_code (loaded from\n"
    "tests/input_ringmod.scd) and its outlet sends `eval <code>` to sc3~.\n"
    "⇧Return evaluates the caret line · ⌘Return the selection/region.\n\n"
    "MouseX/MouseY/MouseButton do NOT move (there is no control in: 3.7): in the .scd\n"
    "modfreq is an argument; change the Ndef().set(\\modfreq, N) line and\n"
    "evaluate with ⇧Return.\n\n"
    "No DSP, no input (just like no DSP, no output).",
    [20, 540, 600, 20],
)

patcher = {
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 0,
            "revision": 0,
            "architecture": "arm64",
            "modernui": 1,
        },
        "rect": [80, 80, 680, 760],
        "bglocked": 0,
        "openinpresentation": 0,
        "default_fontsize": 12,
        "default_fontface": 0,
        "default_fontname": "Arial",
        "gridonopen": 0,
        "gridsize": [15, 15],
        "gridsnap": 0,
        "boxes": boxes,
        "lines": [],
        "dependency_cache": [],
        "autosave": 0,
    }
}


def into(src_id, src_out, dst_id, dst_in):
    patcher["patcher"]["lines"].append(
        {
            "patchline": {
                "destination": [dst_id, dst_in],
                "source": [src_id, src_out],
                "order": 0,
            }
        }
    )


# startup: open + the .scd already loaded; after that the editor sends code to sc3~
into(loadbang, 0, openmsg, 0)
into(openmsg, 0, sc3, 2)
into(info, 0, sc3, 2)
into(panic, 0, sc3, 2)          # manual (click): CmdPeriod
into(code, 0, sc3, 2)           # sc_code → `eval <code>` → sc3~

# mic → inlet 1, another source → inlet 2
into(adc, 0, sc3, 0)
into(cycle, 0, cycleamp, 0)
into(cycleamp, 0, sc3, 1)

# output: outlet 3 = ch0, outlet 2 = ch1
into(sc3, 3, dac, 0)
into(sc3, 2, dac, 1)

out = os.path.join(HERE, "input.maxpat")
with open(out, "w") as f:
    json.dump(patcher, f, indent=1)
    f.write("\n")
print("wrote", out)
