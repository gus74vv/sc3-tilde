// sc3~ `set` example (see tests/args.maxpat): the [number] boxes drive
//   set x freq <n>   →  x.set(\freq, <n>);
//   set x amp  <n>   →  x.set(\amp,  <n>);
(
x = { |freq = 440, amp = 0.2| SinOsc.ar(freq) * amp ! 2 }.play;
);
x.set(\freq, 220, \amp, 0.2);
