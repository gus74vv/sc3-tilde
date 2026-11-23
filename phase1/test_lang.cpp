// SPDX-License-Identifier: GPL-3.0-only
// test_lang.cpp — verification of the embedded sclang (TestClient).
//
// Goal: confirm that sclang (SC_LanguageClient) can be embedded in a bare
// binary and evaluate SC code while capturing the output. Equivalent to the IDE
// REPL.
//
// Acceptance:
//   1. (1+1) is evaluated and the result is captured.
//   2. A SynthDef can be defined and its existence queried (persistent state).
//   3. A deliberate error is captured (it goes to mPost in this SC, not mErr).
//
// SCOPE (verified): evaluation runs in the Interpreter context (not
// thisProcess).
//   - 1-letter vars (a-z) → Interpreter instance vars → persist between evals.
//   - multi-char vars → globals → they do NOT persist on this path.
//   - `this` → the Interpreter (not thisProcess).
//   - SynthDefs/classes/s → they persist (the key external state).
//   4. Clean shutdown.
//
// NOTE: this binary is "bare" → it needs SCClassLibrary in its dir (symlink).
#include "TestClient.h"

#include <cstdio>

static bool check(bool cond, const char* what) {
    std::printf("[%s] %s\n", cond ? "OK " : "FAIL", what);
    return cond;
}

int main() {
    TestClient client;

    std::printf("== test_lang: arrancando sclang embebido ==\n");
    bool booted = client.boot(/*withUserQuarks=*/false);
    if (!check(booted, "boot (initRuntime + compileLibrary)")) {
        std::printf("  post: %s\n  err:  %s\n", client.mPost.c_str(), client.mErr.c_str());
        return 1;
    }

    // `s` was mounted in boot() → Server.default (IDE/REPL parity).
    // It is accessed as `s` (identifier) or `Server.default`.
    client.clearPost();
    client.evaluate("s.notNil.postln; Server.default.notNil.postln;");
    if (!check(client.mPost.find("true") != std::string::npos, "server default disponible (s / Server.default)")) {
        std::printf("  post: %s\n  err:  %s\n", client.mPost.c_str(), client.mErr.c_str());
    }

    // 1) Simple evaluation: (1+1).postln → "2".
    client.clearPost();
    client.evaluate("(1+1).postln;");
    if (!check(client.mPost.find("2") != std::string::npos, "evaluar (1+1).postln captura '2'")) {
        std::printf("  post: %s\n  err:  %s\n", client.mPost.c_str(), client.mErr.c_str());
    }

    // 2) Persistent state: 1-letter var (it persists as an Interpreter instance var).
    client.clearPost();
    client.evaluate("q = 41 + 1;");
    client.clearPost();
    client.evaluate("q.postln;");
    if (!check(client.mPost.find("42") != std::string::npos, "estado persistente entre evaluaciones (var 1-letra)")) {
        std::printf("  post: %s\n  err:  %s\n", client.mPost.c_str(), client.mErr.c_str());
    }

    // 3) Define a SynthDef (key persistent state) + query its existence. There is
    // no global `SynthDefs` class in SC → the object is stored in a 1-letter var
    // (it persists) and its `name` is queried in a separate evaluation.
    client.evaluate(
        "z = SynthDef(\\sineTest, \n"
        "  {|out=0, freq=440| Out.ar(out, SinOsc.ar(freq, 0, 1))});");
    client.clearPost();
    client.evaluate("z.name.postln;");
    if (!check(client.mPost.find("sineTest") != std::string::npos, "SynthDef definido y consultable (estado persistente)")) {
        std::printf("  post: %s\n  err:  %s\n", client.mPost.c_str(), client.mErr.c_str());
    }

    // 4) Deliberate error: nonexistent method → it is captured (it goes to mPost,
    // not to mErr).
    client.clearPost();
    client.evaluate("123.bogusMethod;");
    bool hasError = (client.mPost.find("not understood") != std::string::npos) ||
                   (client.mPost.find("bogusMethod") != std::string::npos) ||
                   (client.mErr.find("bogusMethod") != std::string::npos);
    if (!check(hasError, "error captured on purpose")) {
        std::printf("  err:  %s\n  post: %s\n", client.mErr.c_str(), client.mPost.c_str());
    }

    std::printf("== test_lang: shutting down ==\n");
    client.shutdownLibrary();
    client.shutdownRuntime();
    std::printf("== test_lang: DONE ==\n");
    return 0;
}
