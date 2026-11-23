// puls2.scd — the "puls2" snippet without a window: the curve that used to be drawn with
// plot(c).editMode_(true) is now edited in the patch's [multislider].
//
// Why there cannot be a plot(): the external does not link Qt ⇒
//   Primitive '_QObject_New' failed  →  QObject:initQObject  →  Window:initWindow
//   →  Plotter:makeWindow  →  ArrayedCollection:plot
// and if that happens inside an Ndef, the exception aborts SynthDef:buildUgenGraph and
// the Ndef is never created (silence). See PLAN v0.45.
//
//   [multislider 16]  →  ~sc3tab("0.10 0.25 …")  →  ~tab.setn(0, …)
//   BufRd reads ~tab with a wandering index (LFDNoise1 4 Hz)
//
// Everything else is the original: GrainFM + Splay + tanh, with `r` (the function that
// maps the table to each range) as it was.
//
// NOTE: it all goes in ONE single `( … )` block. `eval`/`evalfile` use
// interpretPrintCmdLine, which parses ONE expression and then requires EOF: two
// consecutive blocks give "syntax error, unexpected '(' expecting end of file".

(
~tab !? { |b| b.free };                        // reload without leaking buffers
~tab = Buffer.alloc(s, 16);
~sc3tab = { |str| ~tab.setn(0, str.split($ ).collect(_.asFloat)) };  // NOTE: setn = (index, ARRAY)
// initial curve (the one you would draw by hand)
~tab.setn(0, [0.10, 0.25, 0.45, 0.60, 0.80, 0.95, 0.85, 0.70,
               0.55, 0.40, 0.30, 0.45, 0.65, 0.85, 0.95]);
"puls2: 16-point table ready (Buffer %)".format(~tab.bufnum).postln;

Ndef(\puls2, {
	var l, r;
	l = LFDNoise1.kr(4);                       // wandering index
	r = { |lo, hi| BufRd.kr(1, ~tab, l.range(0, 15), 1, 4).linexp(0, 1, lo, hi) };
	Splay.ar(
		tanh(
			GrainFM.ar(2,
				Impulse.ar(r.(0.1, 99)),       // trigger
				r.(0.004, 5),                  // dur
				r.(40, 2000),                  // carfreq
				r.(40, 2000),                  // modfreq
				r.(0.1, 9),                    // index
				l,                             // pan
				~tab                           // envbufnum: the multislider curve
			)
		) * 0.1
	)
}).fadeTime_(0.5).play(0, 2);
)
