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
        "rect": [
            176.0,
            146.0,
            680.0,
            720.0
        ],
        "default_fontsize": 13.0,
        "showrootpatcherontab": 0,
        "showontab": 0,
        "boxes": [
            {
                "box": {
                    "fontname": "Arial",
                    "fontsize": 13.0,
                    "id": "obj-1",
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
                        "rect": [
                            176.0,
                            172.0,
                            680.0,
                            694.0
                        ],
                        "default_fontsize": 13.0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "border": 0,
                                    "filename": "helpdetails.js",
                                    "id": "obj-1",
                                    "ignoreclick": 1,
                                    "jsarguments": [
                                        "sc_code",
                                        90
                                    ],
                                    "maxclass": "jsui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        10.0,
                                        10.0,
                                        620.0,
                                        125.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-2",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        140.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-3",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        70.0,
                                        139.0,
                                        144.0,
                                        25.0
                                    ],
                                    "text": "boot the engine first"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        175.0,
                                        620.0,
                                        260.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Shift+Return      evaluate this line\n// Cmd+Return        evaluate the selection, else the ( ... ) block, else the line\n// Cmd+Shift+Return  evaluate everything\n// Ctrl+. or Cmd+/   panic (Cmd+. stops Max's scheduler: do not use it)\n\n{ SinOsc.ar([300, 302], 0, 0.1) }.play;\n\n(\n{\n    var env = EnvGen.kr(Env.perc(0.01, 2), doneAction: 2);\n    Splay.ar(SinOsc.ar([200, 250, 300, 375]) * env * 0.1)\n}.play;\n)\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-5",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 4,
                                    "outlettype": [
                                        "signal",
                                        "signal",
                                        "",
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        455.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        495.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        93.0,
                                        461.0,
                                        547.0,
                                        40.0
                                    ],
                                    "text": "The outlet sends eval <code> as ONE symbol, so line breaks, commas and quotes arrive intact. Return auto-indents; brackets and quotes close themselves; Cmd+Z undoes."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-8",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        555.0,
                                        620.0,
                                        36.0
                                    ],
                                    "text": "Right-click: Undo, Redo, Cut, Copy, Paste, Delete, Select All, Save to .scd, Load from .scd. Drop files from the Finder to insert their paths as strings. The text is saved with the patch."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-5",
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
                                        "obj-5",
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
                                        "obj-6",
                                        1
                                    ],
                                    "source": [
                                        "obj-5",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-6",
                                        0
                                    ],
                                    "source": [
                                        "obj-5",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        10.0,
                        140.0,
                        70.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p basic"
                }
            },
            {
                "box": {
                    "fontname": "Arial",
                    "fontsize": 13.0,
                    "id": "obj-2",
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
                        "rect": [
                            0.0,
                            26.0,
                            680.0,
                            694.0
                        ],
                        "default_fontsize": 13.0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        15.0,
                                        620.0,
                                        21.0
                                    ],
                                    "text": "@theme sets every color in one step. A color set by hand wins over the theme. Click the messages:"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-2",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        60.0,
                                        116.0,
                                        23.0
                                    ],
                                    "text": "theme onedark"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-3",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        144.0,
                                        60.0,
                                        140.0,
                                        23.0
                                    ],
                                    "text": "theme catppuccin"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-4",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        292.0,
                                        60.0,
                                        116.0,
                                        23.0
                                    ],
                                    "text": "theme dracula"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-5",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        416.0,
                                        60.0,
                                        92.0,
                                        23.0
                                    ],
                                    "text": "theme nord"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-6",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        516.0,
                                        60.0,
                                        116.0,
                                        23.0
                                    ],
                                    "text": "theme gruvbox"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        60.0,
                                        116.0,
                                        23.0
                                    ],
                                    "text": "theme monokai"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-8",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        144.0,
                                        60.0,
                                        132.0,
                                        23.0
                                    ],
                                    "text": "theme solarized"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-9",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        125.0,
                                        60.0,
                                        23.0
                                    ],
                                    "text": "themes"
                                }
                            },
                            {
                                "box": {
                                    "attr": "bgcolor",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-10",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        100.0,
                                        125.0,
                                        220.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "fontsize",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-11",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        330.0,
                                        125.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "linenumbers",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-12",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        20.0,
                                        160.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "wrap",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-13",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        180.0,
                                        160.0,
                                        128.5,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "titlebar",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-14",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        310.0,
                                        160.0,
                                        140.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "tabwidth",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-15",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        460.0,
                                        160.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        200.0,
                                        620.0,
                                        240.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Every token class has its own color attribute:\n// @keywordcolor @classcolor @symbolcolor @stringcolor @numbercolor ...\n(\nvar freqs = [220, 330, 440];  // a comment\n~drone = { |amp = 0.05|\n    Splay.ar(SinOsc.ar(freqs * LFNoise1.kr(0.2).range(0.99, 1.01))) * amp\n}.play(fadeTime: 2);\n\\symbol; 'quoted'; \"a string\"; $c; 0x1F; 1.5e3;\n)\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-17",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        460.0,
                                        620.0,
                                        21.0
                                    ],
                                    "text": "themes posts the names to the Max console. @theme is also a menu in the inspector."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-10",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-11",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-12",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-13",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-14",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-15",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
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
                                        "obj-16",
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
                                        "obj-16",
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
                                        "obj-16",
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
                                        "obj-16",
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
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-7",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-16",
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
                                        "obj-16",
                                        0
                                    ],
                                    "source": [
                                        "obj-9",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        90.0,
                        140.0,
                        78.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p themes"
                }
            },
            {
                "box": {
                    "fontname": "Arial",
                    "fontsize": 13.0,
                    "id": "obj-3",
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
                        "rect": [
                            0.0,
                            26.0,
                            680.0,
                            694.0
                        ],
                        "default_fontsize": 13.0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-1",
                                    "linecount": 3,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        15.0,
                                        620.0,
                                        50.0
                                    ],
                                    "text": "The bottom bar shows the signature of the call under the caret, with the current argument highlighted, and completions from three letters (Tab inserts, Shift+Tab cycles, Esc closes). Its data is an index file that sclang writes once."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-2",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        105.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-3",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        70.0,
                                        102.0,
                                        220.0,
                                        25.0
                                    ],
                                    "text": "1. boot the engine"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-4",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        300.0,
                                        105.0,
                                        52.0,
                                        23.0
                                    ],
                                    "text": "index"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-5",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        355.0,
                                        102.0,
                                        291.0,
                                        40.0
                                    ],
                                    "text": "2. write the index (once; again after new Quarks)"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        150.0,
                                        620.0,
                                        200.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// 3. Click inside the parentheses below and move the caret:\n{ SinOsc.ar(440, 0, 0.1) }.play;\n\n// 4. Type a class name and a dot, e.g. LFNoi  or  Pulse.\n\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 4,
                                    "outlettype": [
                                        "signal",
                                        "signal",
                                        "",
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        370.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        410.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "callaid",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-9",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        300.0,
                                        370.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "barhighlight",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-10",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        460.0,
                                        370.0,
                                        180.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-11",
                                    "linecount": 3,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        80.0,
                                        420.0,
                                        560.0,
                                        110
                                    ],
                                    "text": "index sends an eval through the outlet, so sc3~ must be running. The file is ~/Documents/Max 9/Library/sc_code-index.txt (derived from the bundle\npath), shared by every sc_code; @indexfile points elsewhere. The editor only reads it."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-6",
                                        0
                                    ],
                                    "source": [
                                        "obj-10",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-7",
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
                                        "obj-6",
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
                                        "obj-7",
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
                                        "obj-8",
                                        1
                                    ],
                                    "source": [
                                        "obj-7",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-8",
                                        0
                                    ],
                                    "source": [
                                        "obj-7",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-6",
                                        0
                                    ],
                                    "source": [
                                        "obj-9",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        178.0,
                        140.0,
                        94.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p call aid"
                }
            },
            {
                "box": {
                    "fontname": "Arial",
                    "fontsize": 13.0,
                    "id": "obj-4",
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
                        "rect": [
                            0.0,
                            26.0,
                            680.0,
                            694.0
                        ],
                        "default_fontsize": 13.0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-1",
                                    "linecount": 3,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        15.0,
                                        620.0,
                                        50.0
                                    ],
                                    "text": "A snippets file holds blocks: a line ### key, then the code verbatim. Type snip.key and press Cmd+Return: the text is replaced by the block and nothing is evaluated. Cmd+Return again evaluates it. This package ships examples/sc_code-snippets.txt with keys 1 to 4."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-2",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        105.0,
                                        612.0,
                                        23.0
                                    ],
                                    "text": "snippets ~/Documents/Max 9/Packages/sc3-tilde/examples/sc_code-snippets.txt"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "bubbleside": 0,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-3",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        135.0,
                                        644.0,
                                        40.0
                                    ],
                                    "text": "Edit the path if the package lives elsewhere. Spaces are fine. The path is saved with the patch (@snippets)."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-4",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        190.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        225.0,
                                        620.0,
                                        200.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Put the caret on the next line and press Cmd+Return, twice:\nsnip.1\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-6",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 4,
                                    "outlettype": [
                                        "signal",
                                        "signal",
                                        "",
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        445.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        485.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-8",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        80.0,
                                        495.0,
                                        52.0,
                                        23.0
                                    ],
                                    "text": "panic"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-9",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        545.0,
                                        731.0,
                                        80
                                    ],
                                    "text": "An unknown key leaves the text untouched and lists the available keys in the Max console. The file is reread when it changes.\nEach expansion also posts the key to the Max console (-> kick):\nnothing is evaluated, so that line is the only confirmation."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-5",
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
                                        "obj-6",
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
                                        "obj-6",
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
                                        "obj-7",
                                        1
                                    ],
                                    "source": [
                                        "obj-6",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-7",
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
                                        "obj-6",
                                        0
                                    ],
                                    "source": [
                                        "obj-8",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        282.0,
                        140.0,
                        94.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p snippets"
                }
            },
            {
                "box": {
                    "fontname": "Arial",
                    "fontsize": 13.0,
                    "id": "obj-5",
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
                        "rect": [
                            0.0,
                            26.0,
                            680.0,
                            694.0
                        ],
                        "default_fontsize": 13.0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-1",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        15.0,
                                        620.0,
                                        36.0
                                    ],
                                    "text": "@file links the editor to an .scd file. @autoload 1 (default) loads it when @file is set; @autowrite 1 writes every edit back 400 ms after you stop typing. Right-click > Save to .scd sets @file for you."
                                }
                            },
                            {
                                "box": {
                                    "attr": "file",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-2",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        20.0,
                                        85.0,
                                        300.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "autoload",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-3",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        330.0,
                                        85.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "autowrite",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-4",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        20.0,
                                        112.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "readonly",
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-5",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        180.0,
                                        112.0,
                                        150.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        145.0,
                                        620.0,
                                        190.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Right-click > Save to .scd, then type here with autowrite 1:\n// the file follows the editor.\n// readonly 1 still lets you select, copy and evaluate.\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        355.0,
                                        620.0,
                                        80
                                    ],
                                    "text": "Drop a sound file from the Finder into the editor: its path arrives as a quoted ABSOLUTE path (sclang does not expand ~), ready\nfor Buffer.read(s, ...)."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-6",
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
                                        "obj-6",
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
                                        "obj-6",
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
                                        "obj-6",
                                        0
                                    ],
                                    "source": [
                                        "obj-5",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        386.0,
                        140.0,
                        70.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p files"
                }
            }
        ],
        "lines": [],
        "autosave": 0
    }
}
