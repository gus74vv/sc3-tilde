// SPDX-License-Identifier: GPL-3.0-only
// SCMaxPalette — palette in pure SC.
// QPalette is 100% Qt primitives (_QPalette_New/Auto/SetColor/Color/HasColor):
// without Qt they all fail. Same API the rest of the library and the Quarks use.
SCMaxPalette {
	var <colors;

	*new { ^super.new.init }
	*auto { |buttonColor, windowColor| ^super.new.init(buttonColor, windowColor) }
	*system { ^this.auto }
	*light { ^this.auto(Color.grey(0.92), Color.grey(0.86)) }
	*dark { ^this.auto(Color.grey(0.43), Color.grey(0.3)) }

	init { |buttonColor, windowColor|
		colors = IdentityDictionary.new;
		buttonColor = buttonColor ? Color.grey(0.5);
		windowColor = windowColor ? Color.grey(0.35);
		colors[\window] = windowColor;
		colors[\windowText] = Color.white;
		colors[\button] = buttonColor;
		colors[\buttonText] = Color.white;
		colors[\base] = Color.grey(0.18);
		colors[\baseText] = Color.white;
		colors[\highlight] = Color(0.25, 0.37, 0.57);
		colors[\highlightText] = Color.white;
		colors[\shadow] = Color.grey(0.08);
		colors[\dark] = Color.grey(0.6);
		colors[\middark] = Color.grey(0.7);
		colors[\midlight] = Color.grey(0.8);
		colors[\light] = Color.grey(0.9);
		colors[\brightText] = Color.white;
		colors[\toolTipBase] = Color.grey(0.9);
		colors[\toolTipText] = Color.black;
		colors[\alternateBase] = Color.grey(0.25);
		^this
	}

	color { |role| ^colors[role.asSymbol] }
	setColor { |color, role| colors[role.asSymbol] = color }
	hasColor { |role| ^colors[role.asSymbol].notNil }

	window { ^this.color(\window) }
	window_ { |c| this.setColor(c, \window) }
	windowText { ^this.color(\windowText) }
	windowText_ { |c| this.setColor(c, \windowText) }
	button { ^this.color(\button) }
	button_ { |c| this.setColor(c, \button) }
	buttonText { ^this.color(\buttonText) }
	buttonText_ { |c| this.setColor(c, \buttonText) }
	base { ^this.color(\base) }
	base_ { |c| this.setColor(c, \base) }
	baseText { ^this.color(\baseText) }
	baseText_ { |c| this.setColor(c, \baseText) }
	highlight { ^this.color(\highlight) }
	highlight_ { |c| this.setColor(c, \highlight) }
	highlightText { ^this.color(\highlightText) }
	highlightText_ { |c| this.setColor(c, \highlightText) }
}
