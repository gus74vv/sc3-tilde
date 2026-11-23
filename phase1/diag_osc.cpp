// SPDX-License-Identifier: GPL-3.0-only
// diag_osc.cpp — Captures the exact bytes sclang sends over UDP.
#include "TestClient.h"
#include <cstdio>

int main() {
    TestClient client;
    if (!client.boot(false)) return 1;
    client.evaluate(
        "s = Server.new('tst', NetAddr(\"127.0.0.1\", 57111));\n"
        "'addr=' ++ s.addr.asString.postln;\n"
        "s.sendMsg('/n_free', 1234);\n"
        "'sent'.postln;\n");
    for (int i = 0; i < 20; ++i) client.tick();
    std::printf("POST: %s\nERR: %s\n", client.mPost.c_str(), client.mErr.c_str());
    client.shutdownLibrary();
    client.shutdownRuntime();
    return 0;
}
