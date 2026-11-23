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
        "rect": [ 134.0, 167.0, 963.0, 685.0 ],
        "boxes": [
            {
                "box": {
                    "id": "obj-16",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 704.0, 115.0, 37.0, 22.0 ],
                    "text": "index"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 13.0,
                    "id": "obj-1",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 15.0, 900.0, 22.0 ],
                    "text": "sc_code 0.9.3 — test patch (eval + blink, wrap to width, bracket matching, title bar)"
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
                    "patching_rect": [ 20.0, 38.0, 929.0, 19.0 ],
                    "text": "Caret before ( [ {  or after ) ] }  ⇒  BOTH get highlighted. Inside strings/comments/symbols, nothing.  ⇧⏎ evaluates without writing a .scd"
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
                    "patching_rect": [ 20.0, 66.0, 637.0, 422.0 ],
                    "sc_code_caret": 84,
                    "sc_code_scroll": 24,
                    "sc_code_text": "(\nNdef(\\ftonic, {\n\tGVerb.ar(\n\t\tSinOsc.ar(\n\t\t\tSelect.kr(\n\t\t\t\tHasher.kr(\n\t\t\t\t\tDuty.kr(\n\t\t\t\t\t\t(1..4)/4,\n\t\t\t\t\t\t0,\n\t\t\t\t\t\tDwhite(0,1)\n\t\t\t\t\t)\n\t\t\t\t)*5,midicps([0,3,5,7,10]+\\tune.kr(60))\n\t\t\t)\n\t).sum,200,3)/20\n} * -10.dbamp\n).clock_(t).quant_(1).fadeTime_(0.5).play;\n)\nNdef(\\ftonic).clear(10)// S5e: a LONG single-shot line. With @wrap 1 it breaks at the box width and\n// reflows on resize; with @wrap 0 it overflows to the right (as before 0.8.3).\nNdef(\\long, { |modfreq = 220, detune = 1.01, ffreq = 1200, rq = 0.3, amp = 0.15| RLPF.ar(GVerb.ar(SinOsc.ar([modfreq, modfreq * detune], 0, amp).sum, 200, 3), ffreq, rq) }).play(0, 2); // caret here + ⌘⏎ ⇒ 400 ms blink\n\n\n",
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
                    "patching_rect": [ 20.0, 500.0, 60.0, 22.0 ],
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
                    "patching_rect": [ 20.0, 535.0, 200.0, 22.0 ],
                    "text": "sc3~"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "bang" ],
                    "patching_rect": [ 90.0, 500.0, 70.0, 22.0 ],
                    "text": "loadbang"
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 0,
                    "patching_rect": [ 480.0, 572.0, 50.0, 22.0 ],
                    "text": "dac~"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-9",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 760.0, 66.0, 419.0, 32.0 ],
                    "text": "Title bar:\n@titlebar 0 hides it (clicking there does not move the caret)."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-10",
                    "linecount": 3,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 760.0, 136.0, 174.0, 45.0 ],
                    "text": "Colores:\n@matchcolor 1 0.6 0.1 0.6\n@matchbrackets 0 (apaga)"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-11",
                    "linecount": 4,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 760.0, 206.0, 340.0, 58.0 ],
                    "text": "Also try:\n· caret before Ndef's ( ⇒ pairs with the final )\n· caret before SinOsc.ar('s ( ⇒ pairs with its own\n· the [ of the list and the { of the block"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-12",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 760.0, 296.0, 313.0, 32.0 ],
                    "text": "1000 lines: moving the caret through brackets does not stall (per-revision cache)."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 11.0,
                    "id": "obj-13",
                    "linecount": 3,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 760.0, 366.0, 300.0, 45.0 ],
                    "text": "Type something and reopen the patch: text and caret get saved (jsave with sc_code_* keys)."
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 13.0,
                    "id": "obj-14",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 760.0, 425.0, 439.0, 24.0 ],
                    "text": "snippets ~/Documents/Max 9/Library/sc_code-snippets.txt"
                }
            },
            {
                "box": {
                    "fontname": "Menlo",
                    "fontsize": 13.0,
                    "id": "obj-15",
                    "linecount": 4,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 760.0, 455.0, 361.0, 67.0 ],
                    "text": "SNIPPETS: with the path loaded, type snip.<key> (e.g. snip.init) and ⌘⏎: the text is REPLACED by the snippet and nothing gets evaluated (the next ⌘⏎ does evaluate)."
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
                    "destination": [ "obj-3", 0 ],
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
                    "destination": [ "obj-7", 0 ], "source": [ "obj-5", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-7", 1 ], "source": [ "obj-5", 1 ]
                }
            }
        ],
        "autosave": 0
    }
}