{
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 2,
            "revision": 1,
            "architecture": "x64",
            "modernui": 1
        },
        "classnamespace": "box",
        "rect": [ 215.0, 171.0, 748.0, 618.0 ],
        "showrootpatcherontab": 0,
        "boxes": [
            {
                "box": {
                    "id": "obj-23",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patcher": {
                        "fileversion": 1,
                        "appversion": {
                            "major": 9,
                            "minor": 2,
                            "revision": 1,
                            "architecture": "x64",
                            "modernui": 1
                        },
                        "classnamespace": "box",
                        "rect": [ 0.0, 26.0, 748.0, 592.0 ],
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "id": "obj-1",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 22.0, 275.0, 67.0, 22.0 ],
                                    "text": "s sc_editor"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 22.0, 214.0, 260.0, 22.0 ],
                                    "text": "snippets '~/path with spaces/my_snippets.txt'"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Menlo",
                                    "fontsize": 11.0,
                                    "id": "obj-13",
                                    "linecount": 10,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 22.0, 43.0, 492.0, 135.0 ],
                                    "text": "SNIPPETS: `snippets <path>` (or @snippets, saved with the patch) loads a file of `### key` blocks plus verbatim code, no escapes — the release ships examples/sc_code-snippets.txt. The path can be relative, absolute, start with ~, and keep one pair of quotes (snippets '~/x y.txt' works). With the file loaded, evaluating `snip.key` REPLACES the text with the snippet and does NOT send eval to the engine (nothing is evaluated): the next ⌘⏎ does. It re-reads the file by itself when the mtime or the size changes. Each expansion posts the key that ran to the Max console as `-> kick` (the sclang look, no `sc_code:` prefix): since no eval leaves the box, that is the only confirmation of which snippet was inserted."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-1", 0 ],
                                    "source": [ "obj-14", 0 ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [ 165.33333333333334, 25.0, 63.0, 22.0 ],
                    "text": "p snippets"
                }
            },
            {
                "box": {
                    "id": "obj-22",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patcher": {
                        "fileversion": 1,
                        "appversion": {
                            "major": 9,
                            "minor": 2,
                            "revision": 1,
                            "architecture": "x64",
                            "modernui": 1
                        },
                        "classnamespace": "box",
                        "rect": [ 215.0, 197.0, 748.0, 592.0 ],
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "attr": "file",
                                    "id": "obj-12",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 96.0, 46.0, 355.0, 22.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "newobj",
                                    "numinlets": 0,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 19.0, 46.0, 65.0, 22.0 ],
                                    "text": "r sc_editor"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-19",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 155.0, 536.0, 61.0, 22.0 ],
                                    "text": "print error"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-18",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 223.0, 536.0, 95.0, 22.0 ],
                                    "text": "print sc_console"
                                }
                            },
                            {
                                "box": {
                                    "fontface": 1,
                                    "fontname": "Menlo",
                                    "fontsize": 14.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 19.0, 15.0, 478.0, 23.0 ],
                                    "text": "sc_code — in-patch sclang code editor for sc3~  (0.9.8)"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Menlo",
                                    "fontsize": 12.0,
                                    "id": "obj-2",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 464.0, 149.0, 246.0, 90.0 ],
                                    "text": "⇧⏎ line\n⌘⏎ region ( … )\n⌘⇧⏎ document\n⇥ / ⇧⇥ completion\n⌃. ⌘/ panic\nclick on 'open' boots sc3~"
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
                                    "patching_rect": [ 19.0, 81.0, 432.0, 376.0 ],
                                    "sc_code_caret": 155,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// mono out only\n{ [SinOsc.ar(440, 0, 0.1), DC.ar(0)] }.play;\n\n// panned slowly left to right\n{ Pan2.ar(SinOsc.ar(440, 0, 0.1), SinOsc.kr(0.2)) }.play;  \n\n",
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
                                    "patching_rect": [ 41.0, 467.0, 60.0, 22.0 ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 4,
                                    "outlettype": [ "signal", "signal", "", "" ],
                                    "patching_rect": [ 19.0, 496.0, 223.0, 22.0 ],
                                    "text": "sc3~"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [ 19.0, 536.0, 45.0, 45.0 ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Menlo",
                                    "fontsize": 12.0,
                                    "id": "obj-8",
                                    "linecount": 14,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 464.0, 250.0, 247.0, 202.0 ],
                                    "text": "The buffer is saved with the patch (jsave/jload): a reopened patch brings the editor back as it was. The outlet emits eval <code> (ONE symbol, multi-line, no temp file) or panic — connect it to sc3~.\n\n⌘⏎ evaluates the ( … ) region under the caret or, if there is none, the line: multi-line { … } and [ … ] are not regions (as in the IDE) ⇒ select the block or use ⌘⇧⏎."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-3", 0 ],
                                    "midpoints": [ 105.5, 72.88671875, 28.5, 72.88671875 ],
                                    "source": [ "obj-12", 0 ]
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
                                    "destination": [ "obj-18", 0 ],
                                    "source": [ "obj-5", 3 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-19", 0 ],
                                    "source": [ "obj-5", 2 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-7", 1 ],
                                    "source": [ "obj-5", 1 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-7", 0 ],
                                    "source": [ "obj-5", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-3", 0 ],
                                    "source": [ "obj-6", 0 ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [ 21.0, 25.0, 42.0, 22.0 ],
                    "text": "p intro"
                }
            },
            {
                "box": {
                    "id": "obj-21",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patcher": {
                        "fileversion": 1,
                        "appversion": {
                            "major": 9,
                            "minor": 2,
                            "revision": 1,
                            "architecture": "x64",
                            "modernui": 1
                        },
                        "classnamespace": "box",
                        "rect": [ 0.0, 26.0, 748.0, 592.0 ],
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontname": "Menlo",
                                    "fontsize": 11.0,
                                    "id": "obj-15",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 17.0, 246.0, 432.0, 71.0 ],
                                    "text": "DROP: a file dragged from the Finder onto the box lands as its quoted ABSOLUTE path at the caret (\"/Users/you/Documents/x.wav\"), one undo step, with the patcher locked or unlocked. Absolute on purpose: sclang does not expand ~, so this text goes into SoundFile.openRead as it is."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Menlo",
                                    "fontsize": 11.0,
                                    "id": "obj-16",
                                    "linecount": 13,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 17.0, 22.0, 465.0, 173.0 ],
                                    "text": "BAR: @callaid 1 (default) reserves the bottom line for the call under the caret: signature, current argument in @barhighlight, n/N on the right, @barcolor. The data is the class library index: send `index` once, sclang writes ~/Documents/Max 9/Library/sc_code-index.txt (one file for every sc_code, @indexfile overrides the path); send it again after installing Quarks or updating SC.\nCOMPLETION: automatic and display-only — at three letters the bar shows the best candidate; ⇥ inserts it (one undo step), ⇧⇥ cycles, Esc closes, typing filters, ⏎ is never taken (it still evaluates / breaks the line). After `Class.` the list holds only the methods of that class. ⌃Space opens it with no prefix, but macOS or Max usually eats that key. ⇥ at a clean argument start writes the IDE template: SinOsc.ar( + ⇥ ⇒ `freq: `."
                                }
                            }
                        ],
                        "lines": []
                    },
                    "patching_rect": [ 89.66666666666667, 25.0, 49.0, 22.0 ],
                    "text": "p editor"
                }
            },
            {
                "box": {
                    "id": "obj-20",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patcher": {
                        "fileversion": 1,
                        "appversion": {
                            "major": 9,
                            "minor": 2,
                            "revision": 1,
                            "architecture": "x64",
                            "modernui": 1
                        },
                        "classnamespace": "box",
                        "rect": [ 0.0, 26.0, 748.0, 592.0 ],
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontname": "Menlo",
                                    "fontsize": 11.0,
                                    "id": "obj-9",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 12.0, 19.0, 267.0, 71.0 ],
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
                                    "patching_rect": [ 12.0, 347.0, 631.0, 96.0 ],
                                    "text": "THEME: @theme onedark (default; it is a menu in the Inspector) — also catppuccin, dracula, nord, gruvbox, monokai, solarized: the 21 colors below in one step, and a color set by hand wins over it. Messages: theme dracula · themes (the list).\ncolors: @bgcolor @textcolor @selcolor @caretcolor @curlinecolor (band on the line of the caret, alpha 0 turns it off) @titlebar, and per token @keywordcolor @classcolor @builtincolor @primitivecolor @symbolcolor @charcolor @stringcolor @numbercolor @envvarcolor @commentcolor @bracketcolor @operatorcolor — the factory palette is Catppuccin Mocha"
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
                                    "patching_rect": [ 12.0, 179.0, 267.0, 147.0 ],
                                    "text": "@blink 1 + @blinkduration 500 + @blinkcolor 0.980 0.702 0.529 0.40 — the evaluated block blinks (InCubic fade, the IDE curve).\n@scrollspeed 3. — factor for wheel and trackpad; 1 is slow, raise it for a trackpad (also a message: scrollspeed 4.).\n@file /path/x.scd + @autoload 1 (it loads when set) + @autowrite 1 (saves after edits, 400 ms debounce)."
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
                                    "patching_rect": [ 12.0, 108.0, 273.0, 58.0 ],
                                    "text": "Right click: Undo/Redo, Cut/Copy/Paste/Delete, Select All, Save to .scd and Load from .scd (Save sets @file to the chosen path)."
                                }
                            }
                        ],
                        "lines": []
                    },
                    "patching_rect": [ 255.0, 25.0, 68.0, 22.0 ],
                    "text": "p attributes"
                }
            }
        ],
        "lines": [],
        "autosave": 0
    }
}