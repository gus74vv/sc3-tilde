// SPDX-License-Identifier: GPL-3.0-only
// Screen geometry stubs. adclib/Mouse:initClass calls
// Window.screenBounds.extent (Qt primitive _QWindow_ScreenBounds).
// There is no screen in the external: neutral values.
+ Window {
	*screenBounds { ^Rect(0, 0, 1920, 1080) }
	*availableScreenBounds { ^Rect(0, 0, 1920, 1080) }
	*allWindows { ^[] }
}
