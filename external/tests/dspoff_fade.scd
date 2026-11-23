// dspoff_fade.scd — test of 3.6 + 3.3 WITH MAX'S DSP TURNED OFF.
//
// What it looks at: that `Ndef(...).clear(2)` performs the fade (and does not cut abruptly) even without
// audio. Since nothing is heard with DSP off, the observable is `units` in sc3~'s
// `info` message: it has to stay the same during the fade (~2.2 s:
// fadeTime + server.latency) and then drop to the baseline.
//
// How to run it in Max (tests/audio.maxpat):
//   1. Audio Status → DSP **off**.
//   2. Message box `evalfile /Users/gustavo/Documents/dev/sc/external/tests/dspoff_fade.scd`
//      (or edit the path of the evalfile button already in the patch).
//   3. Click on `info` following the console markers.
//
// With DSP on the same file works too (and there the fade is also audible).
(
fork {
	var baseline;
	Ndef(\sc3fade, { |amp = 0.15| SinOsc.ar(440) * amp ! 2 }).play;
	baseline = 0;
	0.5.wait;
	"sc3fade> (A) Ndef created: click `info` and note `units`. You have 3 s.".postln;
	3.0.wait;
	"sc3fade> (B) clear(5) NOW: click `info` while the fade lasts".postln;
	"sc3fade>     ⇒ units has to stay equal to (A) for 5.2 s.".postln;
	Ndef(\sc3fade).clear(5);
	4.0.wait;
	"sc3fade> (C) +4.0 s: fade in progress, click `info` ⇒ equal to (A).".postln;
	2.5.wait;
	"sc3fade> (D) +6.5 s: /n_free has already expired ⇒ click `info`: units < (A).".postln;
	0.5.wait;
	"sc3fade> end: (A)=(B)=(C) and (D) lower. If (B) was already lower, the fade was lost.".postln;
}
)
