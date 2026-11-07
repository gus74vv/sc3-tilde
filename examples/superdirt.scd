// ═════════════════════════════════════════════════════════════════════════════
//  sc3~ · SuperDirt — a ready-to-play example
//
//  Load in [sc_code]: right click → Load… (⌘O) → this file.
//  Evaluate block by block:  ⇧⏎ = block,  ⌘⏎ = line,  ⌃. = panic.
//
//  Everything runs INSIDE [sc3~]: no scsynth process, no Tidal, no OSC coming in.
//  SuperDirt's quarks, SynthDefs and sample buffers all live in the engine's own
//  sclang + World, and the audio comes out of sc3~'s outlets.
//
//  Requires sc3~ 0.15.8+ (before that `s.sampleRate` was nil and DirtOrbit:init
//  died on `8 / server.sampleRate`).
// ═════════════════════════════════════════════════════════════════════════════


// ─────────────────────────────────────────────────────────── 1. start SuperDirt
// 2 channels, this sclang, this World. The kit below is ~230 files, so the
// World's default 1024 buffers hold it comfortably. For the WHOLE Dirt-Samples
// collection (~2850 files) add            s.options.numBuffers = 4096;
// to startup.scd and reopen the engine (`close` + `open`): numBuffers is read at
// boot, changing it here does nothing.
(
~dirt = SuperDirt(2, s);
SuperDirt.default = ~dirt;                 // so Pbind(\type, \dirt) finds it
~dirt.loadSoundFiles(
    [\bd, \sn, \cp, \hh, \ho, \hc, \cr, \lt, \mt, \perc, \clap, \cb,
     \glitch, \numbers, \bleep, \arpy, \rave, '808bd', '909'].collect { |name|
        "~/Library/Application Support/SuperCollider/downloaded-quarks/Dirt-Samples/"
            .standardizePath +/+ name ++ "/";     // the trailing / matters
    });
~dirt.start(57122, [topEnvironment.at(\sc3out) ? 0]);
// 57122 = the OSC port SuperDirt listens on for Tidal (/dirt/play …). Nothing sends
// to it here, so it is only the convention; the part that matters is the SECOND
// argument: one DirtOrbit (synths + delay/reverb/limiter + monitor) per output bus.
// Not 57121: that is this engine's sclang language port, so the bind fails with a
// (harmless) "Could not open UDP port" error. In plain scide + scsynth it is the
// same call with [0] — the port exists only for Tidal. No orbits, no sound:
// `type: \dirt` routes through `~dirt.orbits`.
)


// ────────────────────────────────────────────────────────────── 2. check it out
// s.status is answered by the engine itself (that is where the sample rate comes
// from). The printed bus is this sc3~ instance's output: put SuperDirt on a second
// instance and it plays there, not here.
(
"sampleRate = %".format(s.sampleRate).postln;
"orbit output bus = %".format(topEnvironment.at(\sc3out) ? 0).postln;
"banks = %".format(~dirt.soundLibrary.bufferEvents.size).postln;
s.status;
)


// ──────────────────────────────────────────────────────────────── 3. one-shots
// An Event with \type, \dirt is a single hit: the quickest "is it alive?" test.
(
(type: \dirt, orbit: 0, s: \cr, amp: 0.4).play;
(type: \dirt, orbit: 0, s: \bd, amp: 0.6).play;
(type: \dirt, orbit: 0, s: \numbers, n: 2, speed: 0.5, amp: 0.35).play;
)


// ───────────────────────────────────────────────────────────────── 4. the beat
// One Pdef per voice. `\n` is the index inside the sample folder (it wraps, so
// Pwhite cannot run off the end), `\dur` is in seconds.
(
Pdef(\kick, Pbind(
    \type, \dirt, \orbit, 0,
    \s, \bd, \n, Pseq([0, 2, 0, 0], inf),
    \amp, Pseq([0.5, 0.28, 0.4, 0.32], inf),
    \dur, 0.5)).play;

Pdef(\snare, Pbind(
    \type, \dirt, \orbit, 0,
    \s, \sn, \n, Pseq([2, 5], inf),
    \amp, 0.25, \legato, 0.4,
    \dur, 1)).play;

Pdef(\hats, Pbind(
    \type, \dirt, \orbit, 0,
    \s, Pseq([\hh, \hc, \hh, \ho], inf),
    \n, Pwhite(0, 12, inf),
    \amp, Pwhite(0.08, 0.2, inf),
    \pan, Pwhite(-0.8, 0.8, inf),
    \dur, 0.125)).play;
)


// ──────────────────────────────────────────────────── 5. per-event effects
// These are SuperDirt's effect modules (synths that get instantiated on the fly):
// cut = resonant low pass, room = reverb, delay, shape = waveshaping, vowel…
(
Pdef(\bass, Pbind(
    \type, \dirt, \orbit, 0,
    \s, \cb, \n, 0,
    \note, Pseq([0, 0, 3, 5], inf),        // playback rate: 2 ** (note / 12)
    \cut, Pwhite(200, 1800, inf),          // cut/hcutoff: filter in Hz
    \resonance, 0.2,
    \shape, 0.35,
    \amp, 0.25,
    \dur, 0.5)).play;

Pdef(\clapfx, Pbind(
    \type, \dirt, \orbit, 0,
    \s, \clap, \n, Pseq([0, 1], inf),
    \room, Pwhite(0.2, 0.9, inf),          // reverb send
    \size, Pwhite(0.3, 0.9, inf),
    \delay, 0.4, \delaytime, 0.25, \delayfeedback, 0.5,
    \amp, 0.2,
    \dur, 1.5)).play;
)


// ───────────────────────────────────────────────────── 6. melodic material
// Any sample can be pitched: \note rescales it (0 = as recorded). `\legato` > 1
// lets it ring past its step, `\speed` multiplies on top of \note.
(
Pdef(\melody, Pbind(
    \type, \dirt, \orbit, 0,
    \s, Pseq([\bleep, \arpy, \rave], inf),
    \n, Pwhite(0, 6, inf),
    \note, Pseq([0, 3, 7, 10, 7, 3], inf),
    \speed, 1,
    \legato, 2,
    \room, 0.6,
    \pan, Pseq([-0.5, 0.5], inf),
    \amp, 0.15,
    \dur, 0.25)).play;
)


// ────────────────────────────────────────────────── 7. master tweaks (live)
// SuperDirt:set writes a default on every orbit for the keys an event does NOT set
// itself, so it lands on the NEXT event of the running Pdefs. Pdef(\x).stop stops
// one voice; this is the live-mixing knob.
(
~dirt.set(\room, 0.35, \size, 0.7);     // more reverb on everything
~dirt.set(\delay, 0.25, \delaytime, 0.375);
~dirt.set(\hcutoff, 8000);             // high pass: keeps the mix from piling up
"room/delay/hcutoff set on all orbits".postln;
)


// ──────────────────────────────────────────────────────────────── 8. shut down
// Stopping a Pdef cuts its samples mid-flight, so expect one click there (SuperDirt
does not fade them); `~dirt.stop` / `~dirt.free` are clean. `~dirt.free` also gives
the samples back to the World's buffer pool. The engine keeps running: `panic`
would stop every node in the World (all the sc3~ instances), which is heavier.
(
Pdef(\kick).stop; Pdef(\snare).stop; Pdef(\hats).stop;
Pdef(\bass).stop; Pdef(\clapfx).stop; Pdef(\melody).stop;
~dirt.stop;                            // frees the orbits and the OSC listener
~dirt.free;                            // + the loaded buffers
"SuperDirt stopped".postln;
)
