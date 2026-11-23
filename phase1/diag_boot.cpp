// SPDX-License-Identifier: GPL-3.0-only
// diag_boot.cpp — what gets lost when the class tree aborts (no SCMaxCompat).
#include "TestClient.h"
#include <cstdio>

int main() {
    TestClient client;
    if (!client.boot(false)) return 1;
    client.clearPost();
    // maxNodes/memSize are set by the user startup.scd AFTER the line that
    // fails without Qt (QtGUI.palette = QPalette.dark, line 8 of 91).
    client.evaluate("s.options.maxNodes.postln;");
    client.evaluate("s.options.memSize.postln;");
    client.evaluate("QtGUI.palette.class.postln;");
    client.evaluate("Font.default.name.postln;");
    client.evaluate("GUI.skins['jit'].notNil.postln;");
    client.evaluate("InstrProxy.notNil.postln;");
    std::printf(">> %s", client.mPost.c_str());
    if (!client.mErr.empty()) std::printf("ERR: %s\n", client.mErr.c_str());
    client.shutdownLibrary();
    client.shutdownRuntime();
    return 0;
}
