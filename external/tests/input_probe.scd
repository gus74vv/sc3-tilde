// input_probe.scd — Phase 3.8: does signal reach INPUT bus 1 (inlet 2 of [sc3~])?
//
// Context (input smoke): the `input.maxpat` patch wires
//
//   [adc~ 1]  ──► inlet 1 → input bus 0 → SoundIn.ar(0)  → LEFT channel
//   [cycle~]  ──► inlet 2 → input bus 1 → SoundIn.ar(1)  → RIGHT channel
//
// and the user still hears only ONE channel even though `Ndef(\ringmod).numChannels` = 2.
// Engine and inlet→bus mapping are proven headless; what is missing is measuring IN MAX whether
// inlet 2 carries signal.
//
// HOW TO USE IT (in `[sc_code]`): SELECT a `( … )` block and hit `⌘⏎`.
//   · There is no `var` in this file on purpose: sclang only accepts a block with
//     `var` declarations when the `(` is the FIRST character of the evaluated text
//     (SCODE.md §7.19) ⇒ if the selection starts on the comment line, it fails
//     with "unexpected VAR, expecting ')'". With interpreter variables (`~x`) it does not
//     matter where the selection starts.
//   · Multi-line `{ … }` is NOT a region: if you leave the caret inside and select nothing,
//     `⌘⏎` evaluates one line and fails with `unmatched '}'`.
//
// BEFORE MEASURING: Max DSP on (without DSP there is no input: 3.8) and the object
// has to be `sc3~ @ins 2` (check it with the `info` message: the "3.8:" line).

// ── 1. World config: how many INPUT buses it has ──────────────────────────────
// If it says 1, bus 1 does not exist ⇒ SoundIn.ar(1) stays silent (the object warns at
// boot time: "@ins 2 but the World has N input buses").
(
s.options.numInputBusChannels.postln;
s.options.numOutputBusChannels.postln;
)

// ── 2. Poll to the console (no files involved; the console posts raw) ─────────
//  in1 ≈ 0  ⇒ inlet 2 (cycle~) does NOT reach the engine  → cord / @ins / DSP
//  in0 > 0  ⇒ inlet 1 (mic) does reach it                → the object wiring is fine
// The block sounds 0 (`* 0`): it measures without adding noise to the output.
(
~inprobe0 = { var s = SoundIn.ar(0); s.poll(0.25, "in0"); s * 0 }.play;
~inprobe1 = { var s = SoundIn.ar(1); s.poll(0.25, "in1"); s * 0 }.play;
)

// ── 3. RMS to a file (in case the console gets lost among Max posts) ──────────
// Writes "amp0 amp1" 4 times per second to /tmp/sc3in.txt; then, in the
// terminal: `tail -20 /tmp/sc3in.txt` (with cycle~ and the mic running).
(
~inprobePath = "/tmp/sc3in.txt";
File.delete(~inprobePath);
OSCdef(\sc3inprobe, { |msg|
	File.use(~inprobePath, "a", { |f| f.write("% %\n".format(msg[3], msg[4])) });
}, '/sc3in');
~inprobe2 = Ndef(\sc3in, {
	var sig = SoundIn.ar([0, 1]);
	var amp = [Amplitude.kr(sig[0], 0.01, 0.2), Amplitude.kr(sig[1], 0.01, 0.2)];
	SendReply.kr(Impulse.kr(4), '/sc3in', amp);
	sig * 0
}).play(0, 2);
)

// ── 4. Cleanup ───────────────────────────────────────────────────────────────
// (stops posting/writing; it does not touch the patch's Ndef)
(
~inprobe0.free; ~inprobe1.free;
Ndef(\sc3in).clear; OSCdef(\sc3inprobe).free;
)
