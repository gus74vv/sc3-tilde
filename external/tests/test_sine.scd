// test_sine.scd — minimal embedded engine test (sent with `evalfile`).
//
// NOTE: do NOT send this through `eval <text>` from a Max message box: Max splits
// the text into atoms and eats `\ { } ( ) ,` (symptom: "doesn't understand {\"").
// For multi-line code use `evalfile <absolute path>`.
(
SynthDef(\sine, { |amp = 0.2|
	Out.ar(0, SinOsc.ar(440, 0, amp) ! 2);
}).add;
Synth(\sine);
"sc3~: SynthDef \\sine + Synth created (440 Hz)".postln;
)
