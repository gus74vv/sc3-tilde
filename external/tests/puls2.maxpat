{
 "patcher": {
  "fileversion": 1,
  "appversion": {
   "major": 9,
   "minor": 0,
   "revision": 0,
   "architecture": "arm64",
   "modernui": 1
  },
  "rect": [
   80,
   80,
   700,
   620
  ],
  "bglocked": 0,
  "openinpresentation": 0,
  "default_fontsize": 12,
  "default_fontface": 0,
  "default_fontname": "Arial",
  "gridonopen": 0,
  "gridsize": [
   15,
   15
  ],
  "gridsnap": 0,
  "boxes": [
   {
    "box": {
     "id": "obj-1",
     "maxclass": "newobj",
     "text": "sc3~",
     "numinlets": 1,
     "numoutlets": 4,
     "patching_rect": [
      300,
      60,
      70,
      22
     ],
     "outlettype": [
      "",
      "",
      "signal",
      "signal"
     ]
    }
   },
   {
    "box": {
     "id": "obj-2",
     "maxclass": "newobj",
     "text": "loadbang",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      20,
      20,
      70,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-3",
     "maxclass": "message",
     "text": "open",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      20,
      55,
      60,
      22
     ],
     "outlettype": [
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-4",
     "maxclass": "newobj",
     "text": "button",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      130,
      20,
      20,
      20
     ],
     "outlettype": [
      "bang"
     ]
    }
   },
   {
    "box": {
     "id": "obj-5",
     "maxclass": "message",
     "text": "evalfile /Users/gustavo/Documents/dev/sc/external/tests/puls2.scd",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      130,
      55,
      320,
      22
     ],
     "outlettype": [
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-6",
     "maxclass": "newobj",
     "text": "delay 6000",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      20,
      90,
      80,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-7",
     "maxclass": "newobj",
     "text": "button",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      130,
      90,
      20,
      20
     ],
     "outlettype": [
      "bang"
     ]
    }
   },
   {
    "box": {
     "id": "obj-8",
     "maxclass": "message",
     "text": "panic",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      160,
      90,
      60,
      22
     ],
     "outlettype": [
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-9",
     "maxclass": "message",
     "text": "info",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      230,
      90,
      60,
      22
     ],
     "outlettype": [
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-10",
     "maxclass": "message",
     "text": "set 0.10 0.25 0.45 0.60 0.80 0.95 0.85 0.70 0.55 0.40 0.30 0.45 0.65 0.85 0.95",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      130,
      200,
      320,
      22
     ],
     "outlettype": [
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-11",
     "maxclass": "newobj",
     "text": "delay 6500",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      130,
      170,
      80,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-12",
     "maxclass": "newobj",
     "text": "multislider @size 16 @setmin 0. @setmax 1.",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      20,
      200,
      620,
      90
     ],
     "outlettype": [
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-13",
     "maxclass": "newobj",
     "text": "zl join",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      20,
      310,
      70,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-14",
     "maxclass": "newobj",
     "text": "sprintf ~sc3tab(\"%s\")",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      20,
      345,
      220,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-15",
     "maxclass": "newobj",
     "text": "prepend eval",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      20,
      380,
      90,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-16",
     "maxclass": "newobj",
     "text": "dac~",
     "numinlets": 2,
     "numoutlets": 0,
     "patching_rect": [
      300,
      110,
      50,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-17",
     "maxclass": "comment",
     "text": "puls2 without the Qt GUI \u2014 the plot( ) curve lives in the multislider\n\n1) Turn DSP on (\u2318/)\n2) On open: `open` + `evalfile tests/puls2.scd` (starts by itself, 2.5 s later)\n3) Drag the 16 points of the multislider: the curve is heard live\n   (multislider \u2192 zl join \u2192 sprintf ~sc3tab(\"%s\") \u2192 prepend eval \u2192 sc3~)\n\nthe original called plot(c).editMode_(true) to draw the table: impossible here\n(sin Qt, PLAN v0.45) y adem\u00e1s romp\u00eda el build del Ndef (silencio).\n`reload` = re-evaluates puls2.scd (if you edited the file).\n`panic` = CmdPeriod. No DSP, no sound (same as always).",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      20,
      420,
      620,
      20
     ],
     "fontsize": 11
    }
   }
  ],
  "lines": [
   {
    "patchline": {
     "destination": [
      "obj-3",
      0
     ],
     "source": [
      "obj-2",
      0
     ],
     "order": 0
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
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-6",
      0
     ],
     "source": [
      "obj-2",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-5",
      0
     ],
     "source": [
      "obj-6",
      0
     ],
     "order": 0
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
     ],
     "order": 0
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
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-11",
      0
     ],
     "source": [
      "obj-2",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-10",
      0
     ],
     "source": [
      "obj-11",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-12",
      0
     ],
     "source": [
      "obj-10",
      0
     ],
     "order": 0
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
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-8",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-9",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-13",
      0
     ],
     "source": [
      "obj-12",
      0
     ],
     "order": 0
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
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-15",
      0
     ],
     "source": [
      "obj-14",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-1",
      0
     ],
     "source": [
      "obj-15",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-16",
      0
     ],
     "source": [
      "obj-1",
      0
     ],
     "order": 0
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-16",
      1
     ],
     "source": [
      "obj-1",
      1
     ],
     "order": 0
    }
   }
  ],
  "dependency_cache": [],
  "autosave": 0
 }
}
