// Mono case demo: 1 channel => 1 bus => only the FIRST outlet.
// (It is SuperCollider behaviour, same as a mono into inlet 0 of [dac~].)
Ndef(\m, { SinOsc.ar(300) * 0.2 }).play;          // only outlet ch0
// To make it play on both:  { SinOsc.ar(300) * 0.2 ! 2 }
// or  Ndef(\m, { SinOsc.ar(300) * 0.2 }).play(numChannels: 2)
