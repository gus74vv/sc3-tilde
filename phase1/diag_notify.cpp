// SPDX-License-Identifier: GPL-3.0-only
// diag_notify.cpp — why /notify does not arrive + statusWatcher state. NOTE:
// the \"hasBooted = true\" line here is ON PURPOSE the WRONG form (hasBooted
// is getter-only => DoesNotUnderstandError). It serves to reproduce the
// failure. The correct form is in test_world.cpp:
#include "TestClient.h"
#include <cstdio>
#include <string>

static void ev(TestClient& c, const char* label, const std::string& code) {
    c.clearPost();
    c.evaluate(code);
    std::printf("--- %s\n  POST[%s]\n  ERR [%s]\n", label, c.mPost.c_str(), c.mErr.c_str());
    std::fflush(stdout);
}

int main() {
    TestClient client;
    if (!client.boot(false)) return 1;

    ev(client, "redirect addr", "s.addr = NetAddr(\"127.0.0.1\", 57111); s.addr.postln;");
    ev(client, "server+watcher", "s.postln; s.statusWatcher.postln; s.serverRunning.postln;");
    ev(client, "hasBooted=1", "s.statusWatcher.hasBooted = true; s.statusWatcher.hasBooted.postln;");
    ev(client, "notify", "s.statusWatcher.prSendNotifyRequest(true, true);");
    for (int i = 0; i < 20; ++i) client.tick();
    ev(client, "state", "s.statusWatcher.hasBooted.postln; s.statusWatcher.notified.postln; s.serverRunning.postln;");
    ev(client, "synthdef", "SynthDef(\\sine440, { Out.ar(0, SinOsc.ar(440, 0, 0.8)) }).add; 'def-ok'.postln;");
    for (int i = 0; i < 20; ++i) client.tick();

    client.shutdownLibrary();
    client.shutdownRuntime();
    return 0;
}
