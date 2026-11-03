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
        "rect": [ 141.0, 167.0, 1040.0, 600.0 ],
        "boxes": [
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-1",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 15.0, 760.0, 19.0 ],
                    "text": "sc_code — in-patch sclang code editor for sc3~  (0.9.7)"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-2",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 38.0, 760.0, 19.0 ],
                    "text": "⇧⏎ line · ⌘⏎ region ( … ) · ⌘⇧⏎ document · ⇥ / ⇧⇥ completion · ⌃. panic  ·  click on 'open' boots sc3~"
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "indexfile": "",
                    "maxclass": "sc_code",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 20.0, 65.0, 420.0, 140.0 ],
                    "sc_code_caret": 0,
                    "sc_code_scroll": 0,
                    "sc_code_text": "",
                    "snippets": ""
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 76.0, 502.0, 60.0, 22.0 ],
                    "text": "open"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 4,
                    "outlettype": [ "", "", "signal", "signal" ],
                    "patching_rect": [ 20.0, 538.0, 200.0, 22.0 ],
                    "text": "sc3~"
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 0,
                    "patching_rect": [ 140.66666666666669, 579.0, 79.33333333333331, 22.0 ],
                    "text": "dac~"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-8",
                    "linecount": 11,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 461.5, 79.0, 260.0, 147.0 ],
                    "text": "The buffer is saved with the patch (jsave/jload): a reopened patch brings the editor back as it was. The outlet emits eval <code> (ONE symbol, multi-line, no temp file) or panic — connect it to sc3~. ⌘⏎ evaluates the ( … ) region under the caret or, if there is none, the line: multi-line { … } and [ … ] are not regions (as in the IDE) ⇒ select the block or use ⌘⇧⏎."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-9",
                    "linecount": 5,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 740.0, 79.0, 267.0, 71.0 ],
                    "text": "@linenumbers 1 · @fontsize 12 · @tabwidth 4 · @readonly 0 · @autoclose 1 · @wrap 1 (the text reflows to the box width) · @matchbrackets 1 + @matchcolor (pair at the caret)"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-10",
                    "linecount": 7,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 740.0, 407.0, 267.0, 96.0 ],
                    "text": "colors: @bgcolor @textcolor @selcolor @caretcolor @titlebar, and per token @keywordcolor @classcolor @builtincolor @primitivecolor @symbolcolor @charcolor @stringcolor @numbercolor @envvarcolor @commentcolor @bracketcolor @operatorcolor"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-11",
                    "linecount": 11,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 740.0, 239.0, 267.0, 147.0 ],
                    "text": "@blink 1 + @blinkduration 500 + @blinkcolor 1.0 0.55 0.15 0.40 — the evaluated block blinks (InCubic fade, the IDE curve).\n@scrollspeed 3. — factor for wheel and trackpad; 1 is slow, raise it for a trackpad (also a message: scrollspeed 4.).\n@file /path/x.scd + @autoload 1 (it loads when set) + @autowrite 1 (saves after edits, 400 ms debounce)."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-12",
                    "linecount": 4,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 740.0, 168.0, 267.0, 58.0 ],
                    "text": "Right click: Undo/Redo, Cut/Copy/Paste/Delete, Select All, Save to .scd and Load from .scd (Save sets @file to the chosen path)."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-13",
                    "linecount": 11,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 461.5, 302.0, 267.0, 147.0 ],
                    "text": "SNIPPETS: `snippets <path>` (or @snippets, saved with the patch) loads a file of `### key` blocks plus verbatim code, no escapes — the release ships examples/sc_code-snippets.txt. With the file loaded, evaluating `snip.key` REPLACES the text with the snippet and does NOT send eval to the engine (nothing is evaluated): the next ⌘⏎ does. It re-reads the file by itself when the mtime or the size changes."
                }
            },
            {
                "box": {
                    "id": "obj-14",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 465.0, 266.0, 260.0, 22.0 ],
                    "text": "snippets ~/path/to/my_snippets.txt"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-15",
                    "linecount": 5,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 211.0, 420.0, 71.0 ],
                    "text": "DROP: a file dragged from the Finder onto the box lands as its quoted path at the caret (\"~/Documents/x.wav\"), one undo step, with the patcher locked or unlocked. sclang does not expand ~: use \"~\".standardizePath or an absolute path if the text is going to be evaluated."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-16",
                    "linecount": 15,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 292.0, 420.0, 199.0 ],
                    "text": "BAR: @callaid 1 (default) reserves the bottom line for the call under the caret: signature, current argument in @barhighlight, n/N on the right, @barcolor. The data is the class library index: send `index` once, sclang writes ~/Documents/Max 9/Library/sc_code-index.txt (one file for every sc_code, @indexfile overrides the path); send it again after installing Quarks or updating SC.\nCOMPLETION: automatic and display-only — at three letters the bar shows the best candidate; ⇥ inserts it (one undo step), ⇧⇥ cycles, Esc closes, typing filters, ⏎ is never taken (it still evaluates / breaks the line). After `Class.` the list holds only the methods of that class. ⌃Space opens it with no prefix, but macOS or Max usually eats that key. ⇥ at a clean argument start writes the IDE template: SinOsc.ar( + ⇥ ⇒ `freq: `."
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "destination": [ "obj-3", 0 ],
                    "source": [ "obj-14", 0 ]
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
                    "destination": [ "obj-7", 1 ],
                    "source": [ "obj-5", 3 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-7", 0 ],
                    "source": [ "obj-5", 2 ]
                }
            }
        ],
        "autosave": 0
    }
}