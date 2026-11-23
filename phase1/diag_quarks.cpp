// SPDX-License-Identifier: GPL-3.0-only
// diag_quarks.cpp — is a class library that does NOT compile detected and
// recovered from? Distributable scenario: the external is installed on ANOTHER
// user's machine, with THEIR Quarks. If a single one has a syntax error, sclang
// compiles NOTHING ("Library has not been compiled successfully") => the
// external would be left dead.
// v0.14 decision: the boot is TOLERANT (TestClient::boot self-degrades to stock
// + SCMaxCompat and reports it in mDegradeReport / error outlet).
#include "TestClient.h"

#include <cstdio>
#include <fstream>
#include <sys/stat.h>

// Public config API (lang/LangSource/SC_LanguageConfig.hpp).
#include "SC_LanguageConfig.hpp"

static bool writeFile(const char* path, const char* text) {
    std::ofstream f(path);
    if (!f)
        return false;
    f << text;
    return f.good();
}

int main() {
    // (0) Deliberately broken Quark: syntax error (it simulates a Quark from another
    // version).
    const char* badDir = "/tmp/badquark";
    const char* badFile = "/tmp/badquark/BrokenQuark.sc";
    mkdir(badDir, 0755);
    if (!writeFile(badFile,
                   "// Quark roto a proposito: error de sintaxis.\n"
                   "BrokenQuark {\n"
                   "\tvar <a\n"
                   "\t*new { ^super.new }\n"
                   "}\n")) {
        std::printf("[FAIL] no pude escribir %s\n", badFile);
        return 1;
    }

    // (1) User config as is (sclang_conf.yaml from the support dir).
    SC_LanguageConfig::readLibraryConfig(false);
    std::printf("[info] dirs included by the user config: %zu\n",
                gLanguageConfig->includedDirectories().size());

    // (2) Inject the broken quark BEFORE the boot.
    gLanguageConfig->addIncludedDirectory(badDir);
    std::printf("[info] + %s\n", badDir);

    TestClient client;
    bool bootOk = client.boot(false);
    std::printf("[1] boot() = %s, compileOk = %s, degradado = %s\n", bootOk ? "true" : "false",
                client.mCompileOk ? "true" : "FALSE", client.mDegraded ? "SI" : "no");
    std::printf("[1] reporte (outlet error del external):\n%s", client.mDegradeReport.c_str());
    // Errors of the user startup file when re-evaluated over stock (expected: if it
    // depends on excluded Quarks, those lines fail and Function:protect cuts).
    std::printf("[1] errores del startup tras degradar:\n%s", client.mErr.c_str());

    // The broken quark must not be in the post-degradation
    // config.
    bool badStillIn = false;
    for (const auto& p : gLanguageConfig->includedDirectories())
        if (p.string().find("badquark") != std::string::npos)
            badStillIn = true;
    std::printf("[2] path roto aun incluido = %s (esperado: no)\n", badStillIn ? "SI" : "no");

    // (3) Is the degraded library usable?
    client.clearPost();
    client.evaluate("(1+1).postln;");
    bool usable = client.mPost.find("2") != std::string::npos;
    std::printf("[3] eval post=[%s] err=[%s] => usable = %s\n", client.mPost.c_str(),
                client.mErr.c_str(), usable ? "SI" : "NO");

    // (4) SCMaxCompat is still there after degrading (it ships inside stock).
    client.clearPost();
    client.evaluate("Class.respondsTo(\\initClassTree).postln; QtGUI.palette.notNil.postln;");
    std::printf("[4] compat post=[%s]\n", client.mPost.c_str());

    bool ok = bootOk && client.mCompileOk && client.mDegraded && !badStillIn && usable;
    std::printf("== diag_quarks: %s ==\n", ok ? "OK" : "FAIL");
    client.shutdownLibrary();
    client.shutdownRuntime();
    return ok ? 0 : 1;
}
