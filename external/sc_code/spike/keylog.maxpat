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
			460,
			260
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
					"text": "keylog",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						40,
						40,
						340,
						90
					]
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "message",
					"text": "bang",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						40,
						160,
						60,
						22
					]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"destination": [
						"obj-1",
						0
					],
					"source": [
						"obj-2",
						0
					]
				}
			}
		],
		"dependency_cache": [],
		"autosave": 0
	}
}
