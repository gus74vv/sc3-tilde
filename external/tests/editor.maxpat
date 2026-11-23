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
   900,
   560
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
      80,
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
      120,
      20,
      230,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-11",
     "maxclass": "message",
     "text": "keymode 1",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      50,
      100,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-15",
     "maxclass": "message",
     "text": "outputmode 1",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      200,
      20,
      110,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-4",
     "maxclass": "button",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      170,
      50,
      22,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-9",
     "maxclass": "textedit",
     "text": "(\nNdef(\\rthm, {\n    var l=LFNoise2;\n    RLPF.ar(\n        Crackle.ar,\n        SpecPcile.kr(\n            FFT(LocalBuf(1024),\n                LFGauss.ar(1/[6,8,20].choose,\n                    0.01\n                )\n            ),0.5\n        ),\n        l.kr(1/8,0.1,0.15),2\n    ).fold2!2\n} * -25.dbamp\n).clock_(t).quant_(1).fadeTime_(3).play;\n)",
     "numinlets": 1,
     "numoutlets": 4,
     "outlettype": [
      "",
      "int",
      "",
      ""
     ],
     "patching_rect": [
      20,
      90,
      560,
      380
     ],
     "outputmode": 1,
     "keymode": 1
    }
   },
   {
    "box": {
     "id": "obj-12",
     "maxclass": "newobj",
     "text": "route text",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      620,
      90,
      90,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-5",
     "maxclass": "newobj",
     "text": "prepend eval",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      620,
      130,
      110,
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
      620,
      180,
      80,
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
      620,
      240,
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
      620,
      290,
      110,
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
      620,
      340,
      110,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-13",
     "maxclass": "comment",
     "text": "run",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      140,
      50,
      30,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-14",
     "maxclass": "message",
     "text": "debug 1",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      200,
      50,
      70,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-16",
     "maxclass": "message",
     "text": "console 0",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      290,
      20,
      80,
      22
     ]
    }
   },
   {
    "box": {
     "id": "obj-10",
     "maxclass": "comment",
     "text": "type SC here like in the IDE. Enter = eval. Nothing needs escaping.",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      490,
      560,
      40
     ]
    }
   }
  ],
  "lines": [
   {
    "patchline": {
     "destination": [
      "obj-2",
      0
     ],
     "source": [
      "obj-1",
      0
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
      "obj-1",
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
     "source": [
      "obj-11",
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
      "obj-1",
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
     "source": [
      "obj-15",
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
     "source": [
      "obj-4",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-3",
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
      "obj-3",
      0
     ],
     "source": [
      "obj-16",
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
      "obj-1",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-12",
      0
     ],
     "source": [
      "obj-9",
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
      "obj-12",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-3",
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
      "obj-3",
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
      1
     ],
     "source": [
      "obj-3",
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
      "obj-3",
      3
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
      2
     ]
    }
   }
  ],
  "dependency_cache": [],
  "autosave": 0,
  "description": "sc3~ editor: [textedit] -> route text -> eval (Enter evalua)"
 }
}