// input_ringmod.scd — Phase 3.8: audio input in sc3~ (evaluated from
// [sc_code] in tests/input.maxpat: ⇧Return = caret line, ⌘Return = selection).
//
// It is the user's snippet (ringmod with SoundIn), but STEREO: `SoundIn.ar([0, 1])`
// reads BOTH World input buses and `.play(0, 2)` writes both output
// buses ⇒ it plays on both channels and you hear which inlet feeds each one.
//
//   [adc~ 1]  ──► inlet 2 → input bus 0 → SoundIn.ar(0)  → LEFT channel
//   [cycle~]  ──► inlet 3 → input bus 1 → SoundIn.ar(1)  → RIGHT channel
//
// (inlet 1 = messages: open, info, eval… — the order is mandatory, see bug #8
//  of the handoff notes: with the messages last, Max 9.2 routes everything to the first inlet.)
//
// MONO version of the original snippet: `SoundIn.ar(0, 0.5)` (a single bus) and
// `Ndef(\ringmod).play(0, 1)` (a single output bus) ⇒ it plays on one channel only.
// `sc3~` does not expand mono to stereo: the graph decides that (`! 2`, `Splay`, `Pan2`…).
//
// MouseX/MouseY/MouseButton DO work in the external (30-oct), but they are the SCREEN
// mouse normalized 0..1 (y: 0 at the bottom, 1 at the top), not the patcher one: it cannot be limited
// to the object. modfreq goes as an argument anyway, so the patch sounds the same without a mouse;
// it is changed with the lines at the end.
(
Ndef(\ringmod, { |modfreq = 200|
	var carrier, modulator;
	carrier = SoundIn.ar([0, 1], 0.5);   // channel 0 = inlet 2 (mic), channel 1 = inlet 3
	modulator = SinOsc.ar(modfreq);
	carrier * modulator;
});
)

Ndef(\ringmod).fadeTime = 5;
Ndef(\ringmod).play(0, 2);   // 2 output buses (outlets 3 and 2 of sc3~)

// carrier (put the caret on one of these lines and hit ⇧Return)
Ndef(\ringmod).set(\modfreq, 60);
Ndef(\ringmod).set(\modfreq, 700);
