// SPDX-License-Identifier: GPL-3.0-only
// Global state of the shim.
// *Class extensions* do NOT see the private classvars of QtGUI/Font, so the state
// that has to be replaced (palette, defaultFont) lives here.
SCMaxCompatState {
	classvar <>palette, <>defaultFont;
}
