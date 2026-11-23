// 3.5: instance A = [sc3~ 2 0] (buses 0,1). The bare .play takes ~sc3out = 0.
//
// NOTE: the number of signal channels decides how many buses it takes. A MONO
// source takes 1 bus => only the first outlet (same as a mono into inlet 0 of
// [dac~] in Max). Stereo => 2 buses => both outlets.
// Valid synonyms: { SinOsc.ar(440) * 0.2 ! 2 }  or  .play(numChannels: 2)
Ndef(\a, { SinOsc.ar([440, 660]) * 0.2 }).play;   // bus 0 = 440 (L), bus 1 = 660 (R)
