// SPDX-License-Identifier: GPL-3.0-only
// diag.cpp — diagnosis: SynthDef query + scope.
#include "TestClient.h"
#include <cstdio>

int main() {
    TestClient c;
    c.boot(false);

    // definir el SynthDef
    c.clearPost();
    c.evaluate("SynthDef(\\sineTest, {|out=0, freq=440| Out.ar(out, SinOsc.ar(freq, 0, 1))}).add;");
    fflush(stdout), std::printf("def -> err: %s\n", c.mErr.c_str());

    // varias formas de consulta
    c.clearPost(); c.evaluate("SynthDefs.size.postln;");
    fflush(stdout), std::printf("A) SynthDefs.size -> %s | err: %.40s\n", c.mPost.c_str(), c.mErr.c_str());

    c.clearPost(); c.evaluate("SynthDefs.at(\\sineTest).postln;");
    fflush(stdout), std::printf("B) SynthDefs.at(...).postln -> %s | err: %.40s\n", c.mPost.c_str(), c.mErr.c_str());

    c.clearPost(); c.evaluate("SynthDefs.at(\\sineTest).isNil.postln;");
    fflush(stdout), std::printf("C) SynthDefs.at(...).isNil -> %s | err: %.40s\n", c.mPost.c_str(), c.mErr.c_str());

    c.clearPost(); c.evaluate("var sd = SynthDefs.at(\\sineTest); sd.isNil.postln;");
    fflush(stdout), std::printf("D) var sd=...; sd.isNil -> %s | err: %.40s\n", c.mPost.c_str(), c.mErr.c_str());

    return 0;
}
