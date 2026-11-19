// SPDX-License-Identifier: GPL-3.0-only
// QPalette.new/auto/light/dark/system are pure Qt primitives.
// They are used by QtGUI:initClass and by the user's startup.scd (QtGUI.palette = QPalette.dark).
+ QPalette {
	*new { ^SCMaxPalette.new }
	*auto { |buttonColor, windowColor| ^SCMaxPalette.auto(buttonColor, windowColor) }
	*system { ^SCMaxPalette.system }
	*light { ^SCMaxPalette.light }
	*dark { ^SCMaxPalette.dark }
}
