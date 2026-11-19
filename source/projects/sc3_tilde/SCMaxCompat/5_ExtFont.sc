// SPDX-License-Identifier: GPL-3.0-only
// Font.setDefault -> Qt primitive _QFont_SetDefaultFont.
// (Font.new / Font.default are pure SC; Font:initClass already skips itself without Qt.)
+ Font {
	*setDefault { |font| SCMaxCompatState.defaultFont = font }
}
