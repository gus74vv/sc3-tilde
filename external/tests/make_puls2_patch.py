#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generates external/tests/puls2.maxpat — the "puls2" snippet without the Qt GUI.

The original drew the curve with plot() (impossible in the external: no Qt, see
PLAN v0.45). Here that curve lives in a 16-point [multislider] and travels like this:

    [multislider 16] → [zl join] → [sprintf ~sc3tab("%s")] → [eval] → [sc3~]

`sc3~ eval` sends the text to sclang; `~sc3tab` (defined in puls2.scd) parses it and
does `~tab.setn(0, …)`, and `BufRd`/`GrainFM` read the Buffer live.

    python3 tests/make_puls2_patch.py       # rewrites tests/puls2.maxpat
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))

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


# --- main object: [sc3~] without @ins ⇒ 1 inlet (messages), 4 outlets --------
sc3 = box("newobj", "sc3~", 1, 4, [300, 60, 70, 22],
          outlettype=["", "", "signal", "signal"])

loadbang = newobj("loadbang", [20, 20, 70, 22])
openmsg = message("open", [20, 55, 60, 22])
reload = newobj("button", [130, 20, 20, 20], inl=1, outl=1, outlettype=["bang"])
scd = os.path.join(HERE, "puls2.scd")
reloadmsg = message("evalfile %s" % scd, [130, 55, 320, 22])
delay = newobj("delay 6000", [20, 90, 80, 22])   # the boot takes ~4 s
panic = newobj("button", [130, 90, 20, 20], inl=1, outl=1, outlettype=["bang"])
panicmsg = message("panic", [160, 90, 60, 22])
info = message("info", [230, 90, 60, 22])

# --- initial curve: so the multislider shows what the Buffer holds ---------
initvals = message(
    "set 0.10 0.25 0.45 0.60 0.80 0.95 0.85 0.70 0.55 0.40 0.30 0.45 0.65 0.85 0.95",
    [130, 200, 320, 22])
initdelay = newobj("delay 6500", [130, 170, 80, 22])

# --- the "window": a 16-point multislider, 0..1 ---------------------------
ms = newobj("multislider @size 16 @setmin 0. @setmax 1.",
            [20, 200, 620, 90], inl=1, outl=1, outlettype=[""])
join = newobj("zl join", [20, 310, 70, 22])
sprintf = newobj('sprintf ~sc3tab("%s")', [20, 345, 220, 22])
eval = newobj("prepend eval", [20, 380, 90, 22])  # NOT a message box: the left inlet REPLACES the message

# --- World audio output: outlets 3 and 2 = ch0 and ch1 ---------------------
dac = newobj("dac~", [300, 110, 50, 22], inl=2, outl=0)

comment(
    "puls2 without the Qt GUI — the plot( ) curve lives in the multislider\n\n"
    "1) Turn DSP on (⌘/)\n"
    "2) On open: `open` + `evalfile tests/puls2.scd` (starts by itself, 2.5 s later)\n"
    "3) Drag the 16 points of the multislider: the curve is heard live\n"
    "   (multislider → zl join → sprintf ~sc3tab(\"%s\") → prepend eval → sc3~)\n\n"
    "the original called plot(c).editMode_(true) to draw the table: impossible here\n"
    "(no Qt, PLAN v0.45) and it also broke the Ndef build (silence).\n"
    "`reload` = re-evaluates puls2.scd (if you edited the file).\n"
    "`panic` = CmdPeriod. No DSP, no sound (same as always).",
    [20, 420, 620, 20],
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
        "rect": [80, 80, 700, 620],
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


# startup
into(loadbang, 0, openmsg, 0)
into(openmsg, 0, sc3, 0)
into(loadbang, 0, delay, 0)
into(delay, 0, reloadmsg, 0)
into(reloadmsg, 0, sc3, 0)
into(reload, 0, reloadmsg, 0)
into(loadbang, 0, initdelay, 0)
into(initdelay, 0, initvals, 0)
into(initvals, 0, ms, 0)

# controls
into(panic, 0, panicmsg, 0)
into(panicmsg, 0, sc3, 0)
into(info, 0, sc3, 0)

# multislider → text → eval → sc3~
into(ms, 0, join, 0)
into(join, 0, sprintf, 0)
into(sprintf, 0, eval, 0)
into(eval, 0, sc3, 0)

# output
into(sc3, 3, dac, 0)
into(sc3, 2, dac, 1)

out = os.path.join(HERE, "puls2.maxpat")
with open(out, "w") as f:
    json.dump(patcher, f, indent=1)
    f.write("\n")
print("wrote", out)
