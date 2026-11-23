# SPDX-License-Identifier: GPL-3.0-only

# Generates tests/args.maxpat: driving Synth arguments from Max UI objects
# without escaping anything.
#
#   [number] --> [prepend set x freq] --> [sc3~]      =>  x.set(\freq, 81);
#   [number] --> [prepend set x amp]  --> [sc3~]      =>  x.set(\amp, 0.2);
#   [message set x freq 220 amp 0.2]  --> [sc3~]      =>  x.set(\freq, 220, \amp, 0.2);
#
# Why not just `eval x.set(\freq, $1)` in a [message]: a Max message box is a
# lexer. `,` separates messages and `\` is its escape character, so the code
# arrives as `x.set(freq\, 81)` — the symbol marker is gone before any object
# sees it (see the design notes, "eval" entry). The `set` method generates the `\` and the
# commas in sc3~ itself; the atoms travel as plain symbols and numbers.
#
# The synth lives in tests/args.scd, loaded with the `evalfile` message below.
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))

boxes = []


def obj(bid, cls, text, rect, numinlets, numoutlets, outlettype):
    boxes.append(
        {
            "box": {
                "id": bid,
                "maxclass": cls,
                "text": text,
                "numinlets": numinlets,
                "numoutlets": numoutlets,
                "outlettype": outlettype,
                "patching_rect": rect,
            }
        }
    )


def message(bid, text, rect, numinlets=2):
    obj(bid, "message", text, rect, numinlets, 1, [""])


# engine / transport
obj("obj-1", "newobj", "loadbang", [20, 20, 70, 22], 1, 1, ["bang"])
message("obj-2", "open", [20, 55, 60, 22])
obj("obj-3", "newobj", "sc3~", [20, 150, 200, 22], 1, 4, ["", "", "signal", "signal"])
message(
    "obj-4",
    "evalfile " + os.path.join(HERE, "args.scd"),
    [20, 210, 470, 22],
)
message("obj-5", "panic", [510, 210, 60, 22])
message("obj-9", "console 0", [20, 300, 90, 22])
obj("obj-6", "newobj", "dac~", [20, 350, 50, 22], 2, 0, [])
obj("obj-7", "newobj", "print post", [240, 350, 100, 22], 1, 0, [])
obj("obj-8", "newobj", "print error", [350, 350, 100, 22], 1, 0, [])

# the two controls: an atom of target + key goes in front, the value comes from
# the [number] (click-drag or type). Any number of pairs works.
obj("obj-10", "number", "", [260, 20, 70, 22], 1, 2, ["", "bang"])
obj("obj-11", "newobj", "prepend set x freq", [260, 90, 160, 22], 1, 1, [""])
obj("obj-12", "number", "", [460, 20, 70, 22], 1, 2, ["", "bang"])
obj("obj-13", "newobj", "prepend set x amp", [460, 90, 160, 22], 1, 1, [""])
message("obj-14", "set x freq 220 amp 0.2", [260, 130, 200, 22])

obj(
    "obj-15",
    "comment",
    "sc3~ `set`: Synth arguments from Max UI, no escaping.\n"
    "[number] -> [prepend set x freq] -> sc3~   generates  x.set(\\freq, <n>);\n"
    "Drag/type in the number boxes; the last message box sends two pairs at once.\n"
    "Click `evalfile` first (it plays x = { |freq = 440, amp = 0.2| ... }).\n"
    "\n"
    "Do NOT write `eval x.set(\\freq, $1)` in a message box: Max splits it at the\n"
    "comma and eats the backslash (it arrives as `x.set(freq\\, 81)`). If you need\n"
    "raw code with commas/symbols, use [textedit] with outputmode 1 (tests/editor.maxpat)\n"
    "or build the text with [sprintf symout x.set(%cfreq%c %d)] (92 = backslash, 44 = comma).",
    [20, 400, 600, 130],
    1,
    0,
    [],
)

lines = []


def connect(src, outlet, dst, inlet=0):
    lines.append(
        {"patchline": {"source": [src, outlet], "destination": [dst, inlet]}}
    )


connect("obj-1", 0, "obj-2")
connect("obj-1", 0, "obj-9")
connect("obj-2", 0, "obj-3")
connect("obj-4", 0, "obj-3")
connect("obj-5", 0, "obj-3")
connect("obj-9", 0, "obj-3")
connect("obj-3", 3, "obj-6", 0)
connect("obj-3", 2, "obj-6", 1)
connect("obj-3", 1, "obj-7")
connect("obj-3", 0, "obj-8")
connect("obj-10", 0, "obj-11")
connect("obj-11", 0, "obj-3")
connect("obj-12", 0, "obj-13")
connect("obj-13", 0, "obj-3")
connect("obj-14", 0, "obj-3")

patch = {
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 0,
            "revision": 0,
            "architecture": "arm64",
            "modernui": 1,
        },
        "rect": [80, 80, 700, 580],
        "bglocked": 0,
        "openinpresentation": 0,
        "default_fontsize": 12,
        "default_fontface": 0,
        "default_fontname": "Arial",
        "gridonopen": 0,
        "gridsize": [15, 15],
        "gridsnap": 0,
        "boxes": boxes,
        "lines": lines,
        "dependency_cache": [],
        "autosave": 0,
        "locked_bg": 0,
        "editing_bgcolor": [0.9, 0.9, 0.9, 1.0],
        "default_fontface": 0,
    }
}

out = os.path.join(HERE, "args.maxpat")
with open(out, "w") as fh:
    json.dump(patch, fh, indent=1)
    fh.write("\n")
print("wrote", out)
