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
			100,
			100,
			560,
			320
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
					"text": "info",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						60,
						60,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "message",
					"text": "eval 1+1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						100,
						60,
						110,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-4",
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
						60,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "newobj",
					"text": "dac~",
					"numinlets": 2,
					"numoutlets": 0,
					"patching_rect": [
						220,
						190,
						50,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-6",
					"maxclass": "newobj",
					"text": "print post",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						110,
						155,
						90,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "newobj",
					"text": "print error",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						155,
						90,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-8",
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
						"obj-1",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-4",
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
						"obj-4",
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
						"obj-5",
						1
					],
					"source": [
						"obj-4",
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
						"obj-4",
						3
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
						"obj-4",
						2
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
						"obj-1",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-4",
						0
					],
					"source": [
						"obj-8",
						0
					]
				}
			}
		],
		"dependency_cache": [],
		"autosave": 0
	}
}
