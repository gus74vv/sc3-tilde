// ═════════════════════════════════════════════════════════════════════════════
//  sc3~ · SuperDirt — generative patterns, sclang only
//
//  Load in [sc_code]: right click → Load… (⌘O) → this file.
//  Evaluate block by block:  ⇧⏎ = block,  ⌘⏎ = line,  ⌃. = panic.
//
//  This is the SuperDirt "sketchbook" ported to sc3~: it translates
//  SuperDirt/hacks/sclang-dirt.scd (upstream's own demo of driving SuperDirt from
//  sclang with no Tidal and no OSC) so it runs inside the engine. The upstream file
//  starts with `SuperDirt.default = ~dirt` and assumes the instance already exists;
//  here blocks 1–2 do that part on top of this sc3~ instance's buses.
//
//  Requires sc3~ 0.15.8+ and SuperDirt 1.7.4.
// ═════════════════════════════════════════════════════════════════════════════


// ──────────────────────────────────────────────────────── 1. start, with samples
// 56 banks = 783 files (232 MB) out of the 2724 in Dirt-Samples, which is what the
// World's default 1024 buffers hold with room to spare. Swap/add names freely; for the
// whole collection set  s.options.numBuffers = 4096;  in startup.scd and reopen the
// engine.
// Two orbits on the same output bus: the second `\orbit 1` of the upstream file then
// has somewhere to go. On real multi-instance setups each orbit gets its own bus.
(
~dirt = SuperDirt(2, s);
SuperDirt.default = ~dirt;
~dirt.loadSoundFiles(
    [\bd, \sn, \hh, \ho, \hc, \cp, \clap, \cr, \perc, \lt, \mt, \tabla, \can,
     \bottle, \glasstap, \click, \coins, '808bd', '808cy', '808hc', '808sd',
     '808oh', '909', \breaks125, \break, \amencutup, \industrial, \bass, \bass1,
     \bass2, \bassdm, \dbass, \arpy, \bleep, \blip, \casio, \fm, \dsynth, \dist,
     \glitch, \cpu, \cpu2, \em2, \alphabet, \diphone, \numbers, \yeah, \baa,
     \baa2, \breath, \birds, \birds3, \fire, \circus, \cosmicg, \east].collect { |name|
        "~/Library/Application Support/SuperCollider/downloaded-quarks/Dirt-Samples/"
            .standardizePath +/+ name ++ "/";
    });
~dirt.start(57122, [topEnvironment.at(\sc3out) ? 0, topEnvironment.at(\sc3out) ? 0]);
// 57122, not 57121: 57121 is this engine's sclang language port and the bind would
// fail with a harmless "Could not open UDP port" error. The port only matters to an
// OSC sender (Tidal); the buses are what route the audio.
)


// ───────────────────────────────────────────────────────────────── 2. check it
(
"sample rate = %".format(s.sampleRate).postln;
"output bus  = %".format(topEnvironment.at(\sc3out) ? 0).postln;
"orbits      = %".format(~dirt.orbits.size).postln;
"banks       = %".format(~dirt.soundLibrary.bufferEvents.size).postln;
)


// ────────────────────────────────────────────────── 3. can you hear it? (2 buses)
// Same sample twice: on orbit 1 the playback rate is dropped to 2/5 (a fifth down).
(
(type: \dirt, orbit: 0, s: \cr, amp: 0.4).play;
(type: \dirt, orbit: 1, s: \cr, speed: 2/5, amp: 0.4).play;
)


// ──────────────────────────────────────────────────────── 4. hats: speed + room
(
Pdef(\hats, Pbind(
    \type, \dirt, \orbit, 0,
    \s, \hh, \n, Pseq([1, 0, 0, 0, 0], inf),
    \speed, Pseq([1, 1, 0.5, 1.2], inf),
    \amp, 0.3,
    \dur, 0.25 * Pseq([1, 1/2, 1, 2, 2, 1/2], inf),
    \room, Pseq([0, 0, 0.4], inf))).play;
)


// ─────────────────────────────────── 5. any bank, not five: clumps held for a while
// `bufferEvents.keys` are the loaded banks; `curdle` groups names that look alike
// (bd/bass/808bd…), so the pick stays in a family for 2^6–2^9 events before jumping.
(
var clumps = ~dirt.soundLibrary.bufferEvents.keys.asArray.scramble.curdle(0.1);
Pdef(\any, Pbind(
    \type, \dirt, \orbit, 0,
    \s, Pn(Plazy {
        var family = clumps.choose;
        ("family: " ++ family.join(" ")).postln;
        Pfuncn({ family.choose }, 2 ** rrand(6, 9))
    }),
    \n, Pwhite(0, 20, inf),
    \speed, Pbrown(1, 1.2, 0.01),
    \amp, Pwhite().linexp(0, 1, 0.002, 0.25),
    \cut, Prand([1, 2, 3], inf),          // cut group: a new hit kills the previous one
    \legato, 2,
    \room, Pwrand([0, 0.8], [0.9, 0.1], inf),
    \dur, 1/32 * Pshuf([1, 1/2, 1, 2, 2, 1/2], inf))).play;
)


// ──────────────────────────────────────────────────────────── 6. two layers: Ppar
(
Pdef(\layers, Ppar([
    Pbind(
        \type, \dirt, \orbit, 0,
        \s, Pwrand([\hh, \cr, \ho], [2, 1, 1].normalizeSum, inf),
        \n, Pseq([Prand([1, Pseq([1, 0, 5], 1), Pseq([0, 0])], 1), 0, 0, 0, 0, 0, 0, 0, 0], inf),
        \speed, Pseq([1, 1, 0.5, 1.2], inf),
        \cut, Prand([0, 1, 2], inf),
        \amp, 0.28,
        \dur, 0.25 * Pseq([1/2, 1/3, 2/3], inf),
        \room, Pseq([0, 0, 0.4], inf)),
    Pbind(
        \type, \dirt, \orbit, 0,
        \s, \bd, \n, 0,
        \amp, Prand([0.45, 0], inf),
        \dur, Pseq([0.5, 0.25, 0.25], inf))
])).play;
)


// ─────────────────────────── 7. filters per event (+ one event, three filter bands)
// `\cutoff` sweeps with Pseg; `\resonance` stays. In the last pair an array in a
// parameter expands into simultaneous events (DirtEventTypes flops the whole event),
// so each hit gets three cutoff values — an event-time chord of filters.
(
Pdef(\filt, Pbind(
    \type, \dirt, \orbit, 0,
    \s, \hh, \n, Pseq([1, 0, 0, 0, 0], inf),
    \speed, Pseq([1, 1, 0.5, 1.2], inf),
    \cutoff, Pseg(Pwhite().linexp(0, 1, 300, 9000), 3, \lin, inf),
    \resonance, 0.6,
    \amp, Pseg(Pwhite().linexp(0, 1, 0.02, 0.3), 4, \lin, inf),
    \room, Pseq([0, 0, 0.4, 1], inf),
    \dur, 0.25 * Pseq([1, 1/2, Prand([1, 1/2], 1), 2, 2, 1/2], inf))).quant_(1).play;
)


// ─────────────────────────────── 8. walking a folder: Pseries over the sample index
(
Pdef(\walk, Pbind(
    \type, \dirt, \orbit, 1,
    \s, Pseq([\bd, \hh, \bd, \hh], inf),
    \n, Pseries(0, 1, 8),                // index 0,1,2…7, then start again
    \amp, 0.35,
    \dur, 0.25 * Pseq([1, 1/2, 1], inf))).play;
)


// ────────────────────────────────────── 9. vowel tennis: formants + `\sound` alias
// `\sound` is the old name of `\s`; `\vowel` runs the sample through formant filters,
// `\shape` adds waveshaping distortion.
(
Pdef(\vowel, Pbind(
    \type, \dirt, \orbit, 1,
    \sound, Pseq([\bd, \hh, \bd, \hh], inf),
    \n, 0,
    \speed, 1,
    \vowel, Pn(Plazy { Pshuf([\a, \i, \o, \i, \u, [\i, \a], [\o, \u, \e]], rrand(3, 8)) }),
    \resonance, 0.7,
    \shape, Pseq([0.5, 0.8, 0.3, 0.2, 0.2], inf),
    \amp, Pseg(Pwhite().linexp(0, 1, 0.02, 0.25), 3, \lin, inf),
    \cut, 1,
    \dur, 0.25 * Pseq([1, 1/2, 1], inf))).play;
)


// ─────────────────── 10. too many sounds: random banks + random effect chains
// Four banks per run, and every single event draws 1–3 effect chains out of the
// table below (the values are posted, so the post window doubles as a legend).
(
var howManyEffects = { rrand(1, 3) };
var sounds = ~dirt.soundLibrary.bufferEvents.keys.asArray.scramble.keep(4);
var chains = [
    { ~vowel = "aeiou".choose.asSymbol },
    { ~shape = rrand(0, 0.5) },
    { ~hcutoff = exprand(200.0, 9000.0); ~hresonance = rrand(0.1, 0.7) },
    { ~cutoff = exprand(200.0, 9000.0); ~resonance = rrand(0.1, 0.7) },
    { ~bandf = exprand(200.0, 9000.0); ~bandq = rrand(0.5, 40) },
    { ~crush = exprand(0.01, 0.5) },
    { ~coarse = exprand(1, 8) },
    { ~psrate = exprand(0.8, 10); ~psdisp = rrand(0, 2.0) },
    { ~tremolorate = exprand(0.8, 8); ~tremolodepth = rrand(0.1, 2.0) },
    { ~phasorrate = exprand(0.8, 2); ~phasordepth = rrand(0.1, 0.5) },
    { ~delaytime = rrand(0.2, 1); ~delayfeedback = exprand(0.01, 0.2);
      ~delaySend = rrand(0.5, 1); ~delayAmp = rrand(0.1, 1) },
    { ~room = 0; ~delayAmp = 0; ~leslie = 0 },
    { ~size = rrand(0.2, 3); ~room = rrand(0.1, 1); ~dry = rrand(0.1, 1) },
    { ~leslie = rrand(0.1, 1); ~lrate = exprand(0.8, 3); ~lsize = rrand(0.2, 1) }
];
var decorate = { |event|
    chains.scramble.keep(howManyEffects.value).do { |f| event.use(f) };
    event
};
var post = { |event|
    event.collect(_.round(0.01)).asPairs.clump(2).sort { |a, b| a.first < b.first }.do(_.postcs);
    "-------------------------".postln;
    event
};
Pdef(\chaos, Pbind(
    \type, \dirt,
    \orbit, Prand([0, 1], inf),
    \s, Prand(sounds, inf),
    \n, Pwhite(0.0, 9, inf),
    \cut, -1,                            // -1 = no cut group
    \speed, Pwhite(0.7, 1.5),
    \amp, Pseg(Pwhite().linexp(0, 1, 0.02, 0.2), 3, \lin, inf),
    \dur, 0.125 * Pseq([1, 1/2, 1, 2, 2, 1/2], inf) * Pfunc { [1, 8].wchoose([0.9, 0.1]) })
    .collect(decorate).collect(post)).play;
)


// ───────────────────────────────────────────────────────────────────── 11. stop
// One Pdef at a time stops only that voice. Stopping them cuts the samples mid-wave
// (one click, SuperDirt does not fade); `~dirt.stop`/`free` are clean.
(
[Pdef(\hats), Pdef(\any), Pdef(\layers), Pdef(\filt),
 Pdef(\walk), Pdef(\vowel), Pdef(\chaos)].do(_.stop);
~dirt.stop;
~dirt.free;
"stopped".postln;
)
