(
Ndef(\rthm, {
    var l=LFNoise2;
    RLPF.ar(
        Crackle.ar,
        SpecPcile.kr(
            FFT(LocalBuf(1024),
                LFGauss.ar(1/[6,8,20].choose,
                    0.01
                )
            ),0.5
        ),
        l.kr(1/8,0.1,0.15),2
    ).fold2!2
} * -25.dbamp
).clock_(t).quant_(1).fadeTime_(3).play;
)
