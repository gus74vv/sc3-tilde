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
			1040,
			600
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
					"maxclass": "comment",
					"text": "sc_code — in-patch sclang code editor for sc3~  (0.9.0)",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						15,
						760,
						22
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "comment",
					"text": "⇧⏎ line · ⌘⏎ region ( … ) · ⌘⇧⏎ document · ⌃. panic  ·  click on 'open' boots sc3~",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20,
						38,
						760,
						20
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "newobj",
					"text": "sc_code",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						65,
						700,
						360
					]
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "message",
					"text": "open",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20,
						440,
						60,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-5",
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
						475,
						200,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "newobj",
					"text": "dac~",
					"numinlets": 2,
					"numoutlets": 0,
					"outlettype": [],
					"patching_rect": [
						480,
						512,
						50,
						22
					]
				}
			},
			{
				"box": {
					"id": "obj-8",
					"maxclass": "comment",
					"text": "The text is saved with the patch (jsave/jload); reopening leaves the editor as it was. The outlet is eval <code> (no temp file) or panic, to connect to sc3~. ⌘⏎ evaluates the ( … ) region or, if there is none, the line: multi-line { … } and [ … ] are not a region (like the IDE) ⇒ select the block or use ⌘⇧⏎.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						740,
						65,
						260,
						40
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "comment",
					"text": "@linenumbers 1 · @fontsize 12 · @tabwidth 4 · @readonly 0 · @autoclose 1 · @wrap 1 (the text wraps to the box width)",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						740,
						115,
						260,
						40
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "comment",
					"text": "colors: @bgcolor @textcolor @selcolor @caretcolor, and per token @keywordcolor @classcolor @symbolcolor @stringcolor @numbercolor @commentcolor @bracketcolor @operatorcolor …",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						740,
						165,
						260,
						60
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-11",
					"maxclass": "comment",
					"text": "@blink 1 + @blinkduration 400 + @blinkcolor 1.0 0.55 0.15 0.40 (blink of the evaluated block: InCubic fade; the IDE uses 600 ms).\n@scrollspeed 1. (wheel / trackpad factor: 1 = slow; try 4–10 — with a trackpad the author uses 9) · @file /path/x.scd with @autoload 1 (loads when set) and @autowrite 1 (autosaves, 400 ms debounce).",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						740,
						235,
						260,
						60
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-12",
					"maxclass": "comment",
					"text": "Right click: Undo/Redo, Cut/Copy/Paste/Delete, Select All, Save to .scd and Load from .scd (without @file it opens the save panel and leaves it as @file).",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						740,
						305,
						260,
						40
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-13",
					"maxclass": "comment",
					"text": "SNIPPETS: `snippets <path>` (or @snippets, saved with the patch) loads a text file with `### key` blocks + the verbatim code, no escapes. With the file loaded, evaluating `snip.key` REPLACES the text with the snippet and does NOT send eval to the engine (nothing is evaluated): the next ⌘⏎ does evaluate it. It re-reads on its own if the file changed.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						740,
						355,
						260,
						75
					],
					"fontsize": 11.0,
					"fontname": "Menlo"
				}
			},
			{
				"box": {
					"id": "obj-14",
					"maxclass": "message",
					"text": "snippets ~/Documents/Max 9/Library/sc_code-snippets.txt",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						740,
						440,
						260,
						22
					]
				}
			}
		],
		"lines": [
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
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						0
					],
					"destination": [
						"obj-5",
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
						"obj-5",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-5",
						2
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
						"obj-5",
						3
					],
					"destination": [
						"obj-7",
						1
					]
				}
			}
		],
		"dependency_cache": [],
		"autosave": 0
	}
}
