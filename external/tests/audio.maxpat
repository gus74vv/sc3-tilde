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
			620,
			420
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
						110,
						200,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "message",
					"text": "evalfile /Users/gustavo/Documents/dev/sc/external/tests/test_sine.scd",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						180,
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
						180,
						60,
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
					"patching_rect": [
						260,
						260,
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
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						130,
						260,
						90,
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
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						260,
						90,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "message",
					"text": "eval 1+1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						215,
						90,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "message",
					"text": "console 0",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						110,
						20,
						80,
						22
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
						"obj-3",
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
						"obj-5",
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
			},
			{
				"patchline": {
					"destination": [
						"obj-3",
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
						"obj-10",
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
						"obj-3",
						0
					],
					"source": [
						"obj-10",
						0
					]
				}
			}
		],
		"dependency_cache": [],
		"autosave": 0
	}
}