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
            734.0,
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
                            734.0,
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
                                        "sc3~",
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
                                        630.0,
                                        127.0
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
                                        158.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
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
                                        70.0,
                                        158.0,
                                        52.0,
                                        23.0
                                    ],
                                    "text": "close"
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
                                        125.0,
                                        158.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "info"
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
                                        175.0,
                                        158.0,
                                        52.0,
                                        23.0
                                    ],
                                    "text": "panic"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-6",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        240.0,
                                        151.5,
                                        403.0,
                                        40.0
                                    ],
                                    "text": "open boots sclang + scsynth in the background (about 4 s); the Max console says booting, then ready. info posts the state."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        203.0,
                                        620.0,
                                        250.0
                                    ],
                                    "sc_code_caret": 112,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Click in this editor. Put the caret on a line and press Shift+Return.\n{ SinOsc.ar([440, 443], 0, 0.1) }.play;\n\n// Cmd+Return evaluates the ( ... ) block around the caret.\n(\n{\n    var trig = Impulse.kr(4);\n    var freq = TRand.kr(200, 800, trig);\n    var sig = SinOsc.ar(freq) * Decay2.kr(trig, 0.01, 0.25) * 0.2;\n    Pan2.ar(sig, LFNoise1.kr(0.5))\n}.play;\n)\n\n// Stop: Ctrl+. in this editor, or click panic.\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-8",
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
                                        473.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-9",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        95.0,
                                        473.0,
                                        345.0,
                                        90
                                    ],
                                    "text": "Outlets: left to right: audio ch 0, audio ch 1, error, post. \nThe text also prints in the Max console.\n0.16 changed this order (before: error, post, ch1, ch0): an old patch still\nplays, but its audio and text land on the wrong outlets until the\nconnections move to the new index."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        518.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-11",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        75.0,
                                        528.0,
                                        150.0,
                                        21.0
                                    ],
                                    "text": "turn audio on"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-12",
                                    "linecount": 3,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        578.0,
                                        620.0,
                                        80
                                    ],
                                    "text": "sc3~ runs SuperCollider 3.14.1 inside Max. sc_code sends each evaluation as one eval message. The class library, the UGens and libsndfile\ntravel inside the .mxo: no SuperCollider installation needed. Your Quarks and Extensions are compiled on top. Do not press Cmd+. \nin Max: it stops Max's scheduler. Use panic."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-8",
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
                                        "obj-8",
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
                                        "obj-8",
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
                                        "obj-8",
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
                                        "obj-10",
                                        1
                                    ],
                                    "source": [
                                        "obj-8",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-10",
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
                            734.0,
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
                                    "text": "Each sc3~ serves its own bus range of ONE shared engine: [sc3~ 2 0] plays buses 0-1, [sc3~ 2 2] buses 2-3. ~sc3out is the first bus of the object that evaluated the code, ~sc3n its channel count. A bare .play uses ~sc3out by itself."
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
                                        90.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        125.0,
                                        305.0,
                                        210.0
                                    ],
                                    "sc_code_caret": 123,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Evaluated through [sc3~ 2 0]: ~sc3out = 0\n[~sc3out, ~sc3n].postln;\n\na = { SinOsc.ar([220, 221], 0, 0.08) }.play;\na.free;\n",
                                    "snippets": ""
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
                                        335.0,
                                        125.0,
                                        357.0,
                                        210.0
                                    ],
                                    "sc_code_caret": 211,
                                    "sc_code_scroll": 26,
                                    "sc_code_text": "// Evaluated through [sc3~ 2 2]: ~sc3out = 2\n[~sc3out, ~sc3n].postln;\n\n(\nb = {\n    var sig = LFSaw.ar([330, 331]) * 0.05;\n    Out.ar(~sc3out, RLPF.ar(sig, LFNoise1.kr(1).range(400, 2000), 0.3))\n}.play;\n)\nb.free;\n",
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
                                        355.0,
                                        78.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2 0"
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
                                        335.0,
                                        355.0,
                                        78.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "meter~",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        "float"
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        395.0,
                                        80.0,
                                        13.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "meter~",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        "float"
                                    ],
                                    "patching_rect": [
                                        110.0,
                                        395.0,
                                        80.0,
                                        13.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "meter~",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        "float"
                                    ],
                                    "patching_rect": [
                                        335.0,
                                        395.0,
                                        80.0,
                                        13.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "meter~",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        "float"
                                    ],
                                    "patching_rect": [
                                        425.0,
                                        395.0,
                                        80.0,
                                        13.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        425.0,
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
                                    "id": "obj-12",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        74.5,
                                        78.0,
                                        378.0,
                                        40.0
                                    ],
                                    "text": "One open boots the engine; the second object attaches to it. \npanic, freeall and the nodes are shared by both."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-13",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        490.0,
                                        620.0,
                                        200
                                    ],
                                    "text": "A mono signal uses one bus: { SinOsc.ar(440, 0, 0.1) }.play sounds only from the left outlet. Use ! 2, an array, or Pan2 for both channels. Arguments: [sc3~ channels bus], channels 1-16, bus + channels <= 64.\n\nPORT: the engine binds a FREE UDP port (0 = auto), which is why sc3~ and the\nSuperCollider IDE coexist; info prints it, and @port asks for close +\nopen (the port belongs to the engine, not to the object).\n\nProxySpace: inside a pushed space a bare ~sc3out is a JITLib variable again\n(it names a NodeProxy), so Out.ar(~sc3out, sig) there is SILENT: use\n~out.play, topEnvironment.at(\\sc3out), or capture the bus before the push."
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
                                    "order": 1,
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
                                    "order": 0,
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
                                        "obj-11",
                                        1
                                    ],
                                    "order": 1,
                                    "source": [
                                        "obj-5",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-11",
                                        0
                                    ],
                                    "order": 0,
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
                                        0
                                    ],
                                    "order": 1,
                                    "source": [
                                        "obj-5",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-8",
                                        0
                                    ],
                                    "order": 0,
                                    "source": [
                                        "obj-5",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-10",
                                        0
                                    ],
                                    "order": 0,
                                    "source": [
                                        "obj-6",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-11",
                                        1
                                    ],
                                    "order": 1,
                                    "source": [
                                        "obj-6",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-11",
                                        0
                                    ],
                                    "order": 1,
                                    "source": [
                                        "obj-6",
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
                                    "order": 0,
                                    "source": [
                                        "obj-6",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        90.0,
                        140.0,
                        70.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p buses"
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
                            734.0,
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
                                    "text": "A message box splits text at commas and eats backslashes, so it cannot carry x.set(\\freq, 300). The set message writes them for you: set x freq 300 evaluates x.set(\\freq, 300);"
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
                                        85.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        120.0,
                                        620.0,
                                        120.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// 1. open, then evaluate this line (Shift+Return):\nx = { |freq = 220, amp = 0.1| SinOsc.ar(freq.lag(0.05) ! 2, 0, amp.lag(0.05)) }.play;\n\n// 2. Drag the number boxes. 3. Release:\nx.release(1);\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-4",
                                    "maxclass": "number",
                                    "maximum": 2000,
                                    "minimum": 50,
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [
                                        "",
                                        "bang"
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        20.0,
                                        260.0,
                                        60.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "format": 6,
                                    "id": "obj-5",
                                    "maxclass": "flonum",
                                    "maximum": 0.5,
                                    "minimum": 0.0,
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [
                                        "",
                                        "bang"
                                    ],
                                    "parameter_enable": 0,
                                    "patching_rect": [
                                        200.0,
                                        260.0,
                                        60.0,
                                        23.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-6",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        295.0,
                                        158.0,
                                        23.0
                                    ],
                                    "text": "prepend set x freq"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        200.0,
                                        295.0,
                                        150.0,
                                        23.0
                                    ],
                                    "text": "prepend set x amp"
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
                                        380.0,
                                        260.0,
                                        196.0,
                                        23.0
                                    ],
                                    "text": "set x freq 440 amp 0.05"
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
                                        380.0,
                                        295.0,
                                        148.0,
                                        23.0
                                    ],
                                    "text": "send x.release(2)"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "bubbleside": 0,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-10",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        380.0,
                                        325.0,
                                        316.0,
                                        40.0
                                    ],
                                    "text": "send evaluates its atoms as code, with no quoting."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-11",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        340.0,
                                        108.0,
                                        23.0
                                    ],
                                    "text": "eval 2 ** 10"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-12",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        140.0,
                                        337.0,
                                        220.0,
                                        25.0
                                    ],
                                    "text": "the result prints as post"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-13",
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
                                        395.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        520.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-15",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        120.0,
                                        440.0,
                                        126.0,
                                        23.0
                                    ],
                                    "text": "print sc-error"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-16",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        250.0,
                                        440.0,
                                        118.0,
                                        23.0
                                    ],
                                    "text": "print sc-post"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-17",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        380.0,
                                        380.0,
                                        84.0,
                                        23.0
                                    ],
                                    "text": "console 0"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-18",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        470.0,
                                        380.0,
                                        84.0,
                                        23.0
                                    ],
                                    "text": "console 1"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "bubbleside": 0,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-19",
                                    "linecount": 3,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        380.0,
                                        410.0,
                                        260.0,
                                        69.0
                                    ],
                                    "text": "console 0: text goes only to the two text outlets (wire [print] there). console 1 (default): also to the Max console."
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-20",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        580.0,
                                        620.0,
                                        170
                                    ],
                                    "text": "evalfile <path> evaluates a whole .scd file. debug 1 writes each eval to /tmp/sc3_eval_last.scd.\n\nDIAGNOSTICS: info posts the state, and its last line (dsp64: blocks numouts\nslots peak) is the audio path at a glance. peak > 0 while you hear\nnothing means the audio IS leaving sc3~: look downstream (live.gain~ at\n-inf, ezdac~ off, dac~ in the wrong channels). peak = 0 means nothing is\nproduced. busy= counts the eval in flight; after 10 s sclang counts as\nstuck and eval, send, set and evalfile are dropped with an error."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-13",
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
                                        "obj-14",
                                        1
                                    ],
                                    "source": [
                                        "obj-13",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-14",
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
                                        "obj-15",
                                        0
                                    ],
                                    "source": [
                                        "obj-13",
                                        2
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
                                        3
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-13",
                                        0
                                    ],
                                    "source": [
                                        "obj-17",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-13",
                                        0
                                    ],
                                    "source": [
                                        "obj-18",
                                        0
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-13",
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
                                        "obj-13",
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
                                        "obj-7",
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
                                        "obj-13",
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
                                        "obj-13",
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
                                        "obj-13",
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
                                        "obj-13",
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
                        170.0,
                        140.0,
                        86.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p control"
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
                            734.0,
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
                                        624.0,
                                        36.0
                                    ],
                                    "text": "@ins N adds N signal inlets to the right of the message inlet. Inlet 2 is SoundIn.ar(0), inlet 3 SoundIn.ar(1), ... @ins is fixed when the object is created. The input path adds two signal vectors of latency."
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
                                        85.0,
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
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
                                        70.0,
                                        85.0,
                                        52.0,
                                        23.0
                                    ],
                                    "text": "panic"
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
                                        120.0,
                                        620.0,
                                        230.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "// Ring-modulate the input. Move the mouse left and right:\n// MouseX reads the screen pointer.\n(\n{\n    var in = SoundIn.ar(0);\n    var mod = SinOsc.ar(MouseX.kr(30, 2000, 1));\n    (in * mod * 0.3) ! 2\n}.play;\n)\n\n// Comb echo on the input:\n{ CombC.ar(SoundIn.ar(0), 0.2, LFNoise1.kr(0.3).range(0.01, 0.2), 3) * 0.2 ! 2 }.play;\n",
                                    "snippets": ""
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-5",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        "signal"
                                    ],
                                    "patching_rect": [
                                        250.0,
                                        370.0,
                                        94.0,
                                        23.0
                                    ],
                                    "text": "cycle~ 220"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-6",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        "signal"
                                    ],
                                    "patching_rect": [
                                        250.0,
                                        400.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "*~ 0.5"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        340.0,
                                        370.0,
                                        300.0,
                                        25.0
                                    ],
                                    "text": "or replace it with [adc~ 1] for a microphone"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-8",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 4,
                                    "outlettype": [
                                        "signal",
                                        "signal",
                                        "",
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        440.0,
                                        200.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2 @ins 1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        490.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-10",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        550.0,
                                        620.0,
                                        21.0
                                    ],
                                    "text": "Only one sc3~ per engine owns the input. The message inlet must stay the leftmost one."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-8",
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
                                        "obj-8",
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
                                        "obj-8",
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
                                        "obj-8",
                                        1
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
                                        "obj-9",
                                        1
                                    ],
                                    "source": [
                                        "obj-8",
                                        1
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
                            }
                        ]
                    },
                    "patching_rect": [
                        266.0,
                        140.0,
                        70.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p input"
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
                            734.0,
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
                                    "text": "Three ways to stop. Start the pattern and the drone, then compare."
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
                                        44.0,
                                        23.0
                                    ],
                                    "text": "open"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "indexfile": "",
                                    "maxclass": "sc_code",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        95.0,
                                        620.0,
                                        300.0
                                    ],
                                    "sc_code_caret": 0,
                                    "sc_code_scroll": 0,
                                    "sc_code_text": "(\nSynthDef(\\blip, { |out = 0, freq = 440, amp = 0.15|\n    var env = EnvGen.kr(Env.perc(0.01, 0.3), doneAction: 2);\n    Out.ar(out, SinOsc.ar(freq, 0, amp * env) ! 2);\n}).add;\n)\n\n// A pattern plays from a clock: give it the bus explicitly.\n(\nPdef(\\loop, Pbind(\n    \\instrument, \\blip,\n    \\out, ~sc3out,\n    \\degree, Pseq([0, 2, 4, 7, 4, 2], inf),\n    \\dur, 0.2\n)).play;\n)\n\nx = { SinOsc.ar(110 ! 2, 0, 0.05) }.play;  // a drone\n",
                                    "snippets": ""
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
                                        415.0,
                                        68.0,
                                        23.0
                                    ],
                                    "text": "freeall"
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
                                        20.0,
                                        450.0,
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
                                    "id": "obj-6",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [
                                        ""
                                    ],
                                    "patching_rect": [
                                        20.0,
                                        485.0,
                                        76.0,
                                        23.0
                                    ],
                                    "text": "hardstop"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-7",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        110.0,
                                        410.0,
                                        530.0,
                                        25.0
                                    ],
                                    "text": "frees the nodes: the drone stops, the pattern keeps making new notes"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-8",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        110.0,
                                        445.0,
                                        530.0,
                                        25.0
                                    ],
                                    "text": "CmdPeriod.run: nodes, clocks and patterns stop (like Cmd+. in the SC IDE)"
                                }
                            },
                            {
                                "box": {
                                    "bubble": 1,
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-9",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        110.0,
                                        480.0,
                                        530.0,
                                        25.0
                                    ],
                                    "text": "CmdPeriod.hardRun: also clears TempoClock.default"
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-10",
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
                                        530.0,
                                        62.0,
                                        23.0
                                    ],
                                    "text": "sc3~ 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        20.0,
                                        570.0,
                                        45.0,
                                        45.0
                                    ]
                                }
                            },
                            {
                                "box": {
                                    "fontname": "Arial",
                                    "fontsize": 13.0,
                                    "id": "obj-12",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [
                                        80.0,
                                        580.0,
                                        560.0,
                                        21.0
                                    ],
                                    "text": "Each stop fades to silence over 8 ms first, so it does not click. Audio processing keeps running. From the sc_code editor, Ctrl+. or Cmd+/ sends panic; Cmd+. stops Max's scheduler instead, so avoid it."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-11",
                                        1
                                    ],
                                    "source": [
                                        "obj-10",
                                        1
                                    ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [
                                        "obj-11",
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
                                        "obj-10",
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
                                        "obj-10",
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
                                        "obj-10",
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
                                        "obj-10",
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
                                        "obj-10",
                                        0
                                    ],
                                    "source": [
                                        "obj-6",
                                        0
                                    ]
                                }
                            }
                        ]
                    },
                    "patching_rect": [
                        346.0,
                        140.0,
                        94.0,
                        23.0
                    ],
                    "saved_object_attributes": {
                        "fontsize": 13.0
                    },
                    "text": "p stopping"
                }
            }
        ],
        "lines": [],
        "autosave": 0
    }
}
