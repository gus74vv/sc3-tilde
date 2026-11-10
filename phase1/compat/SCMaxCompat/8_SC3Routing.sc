// SPDX-License-Identifier: GPL-3.0-only
// 8_SC3Routing.sc — 3.5: JITLib `.play` / `.playN` honour the bus of the sc3~ instance
//
// `Ndef(...).play` is `BusPlug:play` (NodeProxy : BusPlug, see
// JITLib/ProxySpace/BusPlug.sc) and already accepts `out:`/`numChannels:`. When the
// code runs inside a `sc3~`, the external injects `~sc3out` (base bus of THAT
// instance) and `~sc3n` (channels) into the topEnvironment, so using them as
// defaults is enough here: a bare `.play` goes to the object's bus and a bare
// `.playN` to its bus range (`playN` is no longer needed, only for reordering).
//
//   Ndef(\x, { SinOsc.ar(440) }).play;          // ⇒ monitors on ~sc3out
//   Ndef(\x, { SinOsc.ar(440) }).play(out: 3);  // explicit out wins
//   Ndef(\x, { SinOsc.ar([440, 660]) }).playN;  // ⇒ ~sc3out .. ~sc3out+1
//   Ndef(\x, { SinOsc.ar([440, 660]) }).playN([1, 0]); // swapped, as stock
//
// The bus is read from `topEnvironment` and NOT from `currentEnvironment`: inside a
// `ProxySpace` (any `LazyEnvir`) reading a MISSING key CREATES a NodeProxy for it
// (LazyEnvir:at), so `currentEnvironment.at(\sc3out)` handed back a NodeProxy
// instead of the bus number and the `.play` produced nodes that made no sound
// (`ProxySpace.push(s.boot); ~out.play; ~out = { … }` = silence, no error at all).
// `topEnvironment` is where the external writes the pair (sc3_ctx_setup in
// sc3_tilde.cpp) and a plain lookup there has no side effects.
//
// Outside a `sc3~` (or in an internal eval) `~sc3out` is nil and the behaviour is
// identical to stock. `numChannels` is not touched: without it, the Monitor expands
// to the channels of the Ndef itself (InBus.sc:291), which is what we want.
//
// Limits (documented in the help and in the README):
//
//  * `~sc3out` lives in the topEnvironment, so it is valid for the synchronous
//    eval. A `.play`/`.playN` deferred from a Routine/clock sees the last evaluated
//    bus: pass `out:`/`outs:` (or capture the bus before deferring).
//  * INSIDE a pushed ProxySpace the `~sc3out` of USER code is a JITLib variable
//    again: `LazyEnvir:at` creates a NodeProxy and `NodeProxy.asControlInput`
//    (BusPlug.sc:92) answers with its OWN `busArg`, so `Out.ar(~sc3out, sig)`
//    compiles, runs and writes to a bus nobody monitors — silence, no error (the
//    same trap as user side); `~sc3out` in there even creates the proxy
//    as a side effect. Use `~out.play`, `topEnvironment.at(\sc3out)`, or capture
//    the bus before the `push`. Fixed neither here nor by overriding
//    `LazyEnvir:at`, which was rejected: it would reserve the two keys inside
//    EVERY ProxySpace and change stock JITLib for everyone.
//  * A literal `Out.ar(0, ...)` inside a SynthDef and the `out` of a Pbind cannot
//    be redirected: use `~sc3out`.
//
// Note that a bare `~sc3out` inside a pushed ProxySpace still goes through
// LazyEnvir (it is a different environment), so there the JITLib way is the right
// one: `~out.play`.

+ NodeProxy {
	play { |out, numChannels, group, multi = false, vol, fadeTime, addAction|
		out = out ?? { topEnvironment.at(\sc3out) };
		^super.play(out, numChannels, group, multi, vol, fadeTime, addAction)
	}

	// `playN` is the sibling of `play` with an explicit destination list. Stock
	// turns a nil `outs` into 0 (InBus.sc:297) = the hardware buses, which in a 2nd
	// instance would play into the 1st instance's block. Same default here; an
	// explicit `outs` (channel reordering) is untouched.
	playN { |outs, amps, ins, vol, fadeTime, group, addAction|
		outs = outs ?? { topEnvironment.at(\sc3out) };
		^super.playN(outs, amps, ins, vol, fadeTime, group, addAction)
	}
}
