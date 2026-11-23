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
   580
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
     "text": "loadbang",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
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
     "id": "obj-2",
     "maxclass": "message",
     "text": "open",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      55,
      60,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-3",
     "maxclass": "newobj",
     "text": "sc3~",
     "numinlets": 1,
     "numoutlets": 4,
     "outlettype": [
      "",
      "",
      "signal",
      "signal"
     ],
     "patching_rect": [
      20,
      150,
      200,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-4",
     "maxclass": "message",
     "text": "evalfile /Users/gustavo/Documents/dev/sc/external/tests/args.scd",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      210,
      470,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-5",
     "maxclass": "message",
     "text": "panic",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      510,
      210,
      60,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-9",
     "maxclass": "message",
     "text": "console 0",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      300,
      90,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-6",
     "maxclass": "newobj",
     "text": "dac~",
     "numinlets": 2,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      350,
      50,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-7",
     "maxclass": "newobj",
     "text": "print post",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      240,
      350,
      100,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-8",
     "maxclass": "newobj",
     "text": "print error",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      350,
      350,
      100,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-10",
     "maxclass": "number",
     "text": "",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      "bang"
     ],
     "patching_rect": [
      260,
      20,
      70,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-11",
     "maxclass": "newobj",
     "text": "prepend set x freq",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      260,
      90,
      160,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-12",
     "maxclass": "number",
     "text": "",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      "bang"
     ],
     "patching_rect": [
      460,
      20,
      70,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-13",
     "maxclass": "newobj",
     "text": "prepend set x amp",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      460,
      90,
      160,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-14",
     "maxclass": "message",
     "text": "set x freq 220 amp 0.2",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      260,
      130,
      200,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-15",
     "maxclass": "comment",
     "text": "sc3~ `set`: Synth arguments from Max UI, no escaping.\n[number] -> [prepend set x freq] -> sc3~   generates  x.set(\\freq, <n>);\nDrag/type in the number boxes; the last message box sends two pairs at once.\nClick `evalfile` first (it plays x = { |freq = 440, amp = 0.2| ... }).\n\nDo NOT write `eval x.set(\\freq, $1)` in a message box: Max splits it at the\ncomma and eats the backslash (it arrives as `x.set(freq\\, 81)`). If you need\nraw code with commas/symbols, use [textedit] with outputmode 1 (tests/editor.maxpat)\nor build the text with [sprintf symout x.set(%cfreq%c %d)] (92 = backslash, 44 = comma).",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      400,
      600,
      130
     ]
    }
   }
  ],
  "lines": [
   {
    "patchline": {
     "source": [
      "obj-1",
      0
     ],
     "destination": [
      "obj-2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-1",
      0
     ],
     "destination": [
      "obj-9",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-2",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-4",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-5",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-9",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-3",
      0
     ],
     "destination": [
      "obj-6",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-3",
      1
     ],
     "destination": [
      "obj-6",
      1
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-3",
      3
     ],
     "destination": [
      "obj-7",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-3",
      2
     ],
     "destination": [
      "obj-8",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-10",
      0
     ],
     "destination": [
      "obj-11",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-11",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-12",
      0
     ],
     "destination": [
      "obj-13",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-13",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "obj-14",
      0
     ],
     "destination": [
      "obj-3",
      0
     ]
    }
   }
  ],
  "dependency_cache": [],
  "autosave": 0,
  "locked_bg": 0,
  "editing_bgcolor": [
   0.9,
   0.9,
   0.9,
   1.0
  ]
 }
}
