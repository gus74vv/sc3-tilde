# SPDX-License-Identifier: GPL-3.0-only

# Genera tests/editor.maxpat a partir de tests/audio.maxpat (mismo wrapper Max 9).
#
# Flujo: [textedit] --text ...--> [route text] --> [prepend eval] --> [sc3~]
#        CAREFUL: [routepass] does NOT drop the matching chunk (its refpage), so
#        the "text" of the selector would reach the eval; [route] does drop it.
#        keymode 1 in the textedit => Enter sends the buffer (escaping nothing).
#        [run] (bang) does the same, in case it is switched back to keymode 0.
#
# Careful with the Max classes in the JSON (verified against audio.maxpat and the
# Max 9 help files): "loadbang" is a newobj WITH text; "button" is maxclass
# button WITHOUT text (if you give it text, Max creates an object with arguments);
# "comment" has no outlets.
import json, io

base = json.load(open('tests/audio.maxpat'))
p = base['patcher']
p['rect'] = [80, 80, 900, 560]
p['description'] = 'sc3~ editor: [textedit] -> route text -> eval (Enter evalua)'

code = """(
Ndef(\\rthm, {
    var l=LFNoise2;
    RLPF.ar(
        Crackle.ar,
        SpecPcile.kr(
            FFT(LocalBuf(1024),
                LFGauss.ar(1/[6,8,20].choose,
                    0.01
                )
            ),0.5
        ),
        l.kr(1/8,0.1,0.15),2
    ).fold2!2
} * -25.dbamp
).clock_(t).quant_(1).fadeTime_(3).play;
)"""


def box(bid, maxclass, text, rect, numin=1, numout=1, outlettype=None, extra=None):
    b = {"id": bid, "maxclass": maxclass}
    if text is not None:
        b["text"] = text
    b["numinlets"] = numin
    b["numoutlets"] = numout
    b["outlettype"] = [""] * numout if outlettype is None else outlettype
    b["patching_rect"] = rect
    if extra:
        b.update(extra)
    return {"box": b}


boxes = [
    # loadbang = newobj with text "loadbang" and a bang outlet
    box("obj-1", "newobj", "loadbang", [20, 20, 80, 22], 1, 1, ["bang"]),
    box("obj-2", "message", "open", [120, 20, 230, 22], 2, 1),
    # keymode 1: the carriage return sends the whole buffer (Enter = eval)
    box("obj-11", "message", "keymode 1", [20, 50, 100, 22], 2, 1),
    # outputmode 1: the buffer comes out as a SINGLE symbol (not parsed by Max:
    # it keeps \symbols, "quotes", commas and semicolons)
    box("obj-15", "message", "outputmode 1", [200, 20, 110, 22], 2, 1),
    box("obj-4", "button", None, [170, 50, 22, 22], 1, 1, ["bang"]),
    # outputmode 1 / keymode 1 as ATTRIBUTES (persistent, they do not depend on the
    # loadbang): buffer = a single symbol, Enter = eval.
    box("obj-9", "textedit", code, [20, 90, 560, 380], 1, 4, ["", "int", "", ""],
        extra={"outputmode": 1, "keymode": 1}),
    box("obj-12", "newobj", "route text", [620, 90, 90, 22]),
    box("obj-5", "newobj", "prepend eval", [620, 130, 110, 22]),
    box("obj-3", "newobj", "sc3~", [620, 180, 80, 22], 1, 4, ["", "", "signal", "signal"]),
    box("obj-6", "newobj", "dac~", [620, 240, 50, 22], 2, 0, []),
    box("obj-7", "newobj", "print post", [620, 290, 110, 22], 1, 0, []),
    box("obj-8", "newobj", "print error", [620, 340, 110, 22], 1, 0, []),
    box("obj-13", "comment", "run", [140, 50, 30, 22], 1, 0, []),
    # debug 1 => vuelca cada eval a /tmp/sc3_eval_last.scd
    box("obj-14", "message", "debug 1", [200, 50, 70, 22], 2, 1),
    # this patch already wires [print post]/[print error] => no console duplication
    box("obj-16", "message", "console 0", [290, 20, 80, 22], 2, 1),
    box("obj-10", "comment",
        "type SC here like in the IDE. Enter = eval. Nothing needs escaping.",
        [20, 490, 560, 40], 1, 0, []),
]

lines = [
    {"patchline": {"destination": ["obj-2", 0], "source": ["obj-1", 0]}},
    {"patchline": {"destination": ["obj-11", 0], "source": ["obj-1", 0]}},
    {"patchline": {"destination": ["obj-9", 0], "source": ["obj-11", 0]}},
    {"patchline": {"destination": ["obj-15", 0], "source": ["obj-1", 0]}},
    {"patchline": {"destination": ["obj-9", 0], "source": ["obj-15", 0]}},
    {"patchline": {"destination": ["obj-9", 0], "source": ["obj-4", 0]}},
    {"patchline": {"destination": ["obj-3", 0], "source": ["obj-14", 0]}},
    {"patchline": {"destination": ["obj-3", 0], "source": ["obj-16", 0]}},
    {"patchline": {"destination": ["obj-16", 0], "source": ["obj-1", 0]}},
    {"patchline": {"destination": ["obj-12", 0], "source": ["obj-9", 0]}},
    {"patchline": {"destination": ["obj-5", 0], "source": ["obj-12", 0]}},
    {"patchline": {"destination": ["obj-3", 0], "source": ["obj-5", 0]}},
    {"patchline": {"destination": ["obj-3", 0], "source": ["obj-2", 0]}},
    {"patchline": {"destination": ["obj-6", 0], "source": ["obj-3", 3]}},
    {"patchline": {"destination": ["obj-6", 1], "source": ["obj-3", 2]}},
    {"patchline": {"destination": ["obj-7", 0], "source": ["obj-3", 1]}},
    {"patchline": {"destination": ["obj-8", 0], "source": ["obj-3", 0]}},
]

p['boxes'] = boxes
p['lines'] = lines
io.open('tests/editor.maxpat', 'w', encoding='utf-8').write(json.dumps(base, indent=1, ensure_ascii=False))
print('escrito tests/editor.maxpat')
