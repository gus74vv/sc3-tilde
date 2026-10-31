// SPDX-License-Identifier: GPL-3.0-only
// 8_SC3Routing.sc — 3.5: JITLib `.play` honours the bus of the sc3~ instance
//
// `Ndef(...).play` is `BusPlug:play` (NodeProxy : BusPlug, see
// JITLib/ProxySpace/BusPlug.sc) and already accepts `out:`/`numChannels:`. When the
// code runs inside a `sc3~`, the external injects `~sc3out` (base bus of THAT
// instance) and `~sc3n` (channels) into the topEnvironment, so using them as
// defaults is enough here: a bare `.play` goes to the object's bus and `playN` is
// no longer needed.
//
//   Ndef(\x, { SinOsc.ar(440) }).play;      // ⇒ monitors on ~sc3out
//   Ndef(\x, { SinOsc.ar(440) }).play(out: 3); // explicit out wins
//
// Outside a `sc3~` (or in an internal eval) `~sc3out` is nil and the behaviour is
// identical to stock. `numChannels` is not touched: without it, the Monitor expands
// to the channels of the Ndef itself (InBus.sc:291), which is what we want.
//
// Limits (documented in the help): `~sc3out` lives in the topEnvironment, so it is
// valid for the synchronous eval. A `.play` deferred from a Routine/clock has to
// pass `out:` explicitly. A literal `Out.ar(0, ...)` inside a SynthDef and the
// `out` of a Pbind cannot be redirected: use `~sc3out`.

+ NodeProxy {
	play { |out, numChannels, group, multi = false, vol, fadeTime, addAction|
		out = out ?? { currentEnvironment.at(\sc3out) };
		^super.play(out, numChannels, group, multi, vol, fadeTime, addAction)
	}
}
