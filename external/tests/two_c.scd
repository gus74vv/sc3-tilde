// 3.5: EXPLICIT out to B's buses (2,3): it comes out through B even if evaluated in A.
Ndef(\c, { SinOsc.ar([880, 1100]) * 0.2 }).play(out: 2);
