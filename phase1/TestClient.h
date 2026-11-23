// SPDX-License-Identifier: GPL-3.0-only
// TestClient.h
// Subclass of SC_LanguageClient for headless tests (external sc3~).
//
// It reproduces the IDE REPL: a SINGLE long-lived sclang interpreter (not one
// per evaluation). Evaluating code here keeps state between calls (global vars
// ~a/~s, SynthDefs, etc.), same as cmd+enter/shift+enter in SCIDE.
//
// The interpreter output (postln / errors) is captured in memory in mPost/mErr
// (decision: in-memory capture, not stdout).
//
// Boot in 3 steps (v0.14 decision, "sclang first" order):
//   1. boot()  → initRuntime + compileLibrary (evaluates the user startup.scd;
//      the SCMaxCompat module makes s.boot/s.quit no-ops). If the user library
//      does NOT compile → it degrades to stock + SCMaxCompat (decision: the
//      external is NEVER left dead; it is reported in mDegradeReport).
//   2. readServerOptions() → reads the s.options.* ALREADY configured by their
//      startup.scd.
//   3. The caller creates the World with those options and adopts the server.
#pragma once

#include "SC_LanguageClient.h"

#include "SC_Filesystem.hpp"     // DirName::Resource (path of the stock class library)
#include "SC_LanguageConfig.hpp" // gLanguageConfig (fallback a stock)

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

// compiledOK is defined by the lexer (lang/LangSource/PyrLexer.cpp:123,
// declared in PyrLexer.h). The MEMBER version of compileLibrary() is void, so
// the result of compiling the class library is read from here.
extern bool compiledOK;

class TestClient : public SC_LanguageClient {
public:
    TestClient(const std::string& name = "sc3-test") : SC_LanguageClient(name) {}

    // --- output hooks (pure virtuals of SC_LanguageClient)
    // ---------------------- postText: code output (postln/post) — we capture it.
    void postText(const char* str, size_t len) override { mPost.append(str, len); }
    // postFlush: separator between output "chunks" (the REPL puts one line).
    void postFlush(const char* str, size_t len) override { mPost.append(str, len); }
    // postError: interpreter error messages.
    void postError(const char* str, size_t len) override { mErr.append(str, len); }
    // flush: when an interpretation ends.
    void flush() override {}

    // --- high-level API (wraps the interpreter API) -----------------------------
    // It starts the runtime + compiles the library (vanilla or with the user
    // Quarks). `withUserQuarks=false` → vanilla (stock). true → it includes the
    // user includePaths. TOLERANT boot: if compiledOK == false (a user Quark with
    // a syntax error kills the whole library), it excludes the user paths and
    // recompiles stock + SCMaxCompat. Verified: compileLibrary → shutdownLibrary
    // on entry, so the recovery happens in the same process.
    bool boot(bool withUserQuarks) {
        Options opt;
        // Alternative port to avoid colliding with the user IDE (default 57120).
        opt.mPort = 57121;
        initRuntime(opt);
        // compileLibrary compiles the library and triggers Main.startup (it evaluates
        // the user startup file → IDE parity). The Main process is created but its `s`
        // (interpreter.s) does not end up mounted in the headless evaluation scope →
        // we mount it by hand (see findings, the design notes). compileLibrary() RETURNS
        // compiledOK (PyrLexer.cpp): false if the class library did not compile (e.g.
        // a user quark with a syntax error).
        compileLibrary(withUserQuarks);
        mCompileOk = compiledOK;
        if (!mCompileOk) {
            // Degradation: stock + SCMaxCompat, reporting what was
            // excluded.
            mDegraded = true;
            mCompileOk = degradeToStock();
        }
        // Mount s/Server.default in the evaluation scope (IDE/REPL parity).
        if (mCompileOk)
            evaluate("s = Server.default");
        // Procesa eventos programados del startup.
        for (int i = 0; i < 8; ++i)
            tick();
        return mCompileOk;
    }

    // Evaluates a chunk of SC code (multi-line / multi-declaration), like the REPL.
    void evaluate(const std::string& code) {
        setCmdLine(code.c_str());
        interpretCmdLine();
        for (int i = 0; i < 4; ++i)
            tick();
    }

    // Recompiles the library (compileLibrary does shutdownLibrary() on entry).
    // Useful to degrade to "stock only" if the user library did not compile.
    bool recompile(bool standalone) {
        compileLibrary(standalone);
        mCompileOk = compiledOK;
        if (mCompileOk) evaluate("s = Server.default");
        return mCompileOk;
    }

    void clearPost() {
        mPost.clear();
        mErr.clear();
    }

    // --- s.options → WorldOptions (order "sclang first") -----------------------
    // The values the external has to respect. `sampleRate` stays nil in
    // ServerOptions until booting (default 48000 in the World if it is not set).
    struct ServerOpts {
        long maxNodes = 1024;
        long maxSynthDefs = 1024;
        long memSize = 8192; // KB (mRealTimeMemorySize)
        long numWireBufs = 64;
        long numRGens = 64;
        long numBuffers = 1024;
        long numAudioBusChannels = 1024;
        long numInputBusChannels = 2;
        long numOutputBusChannels = 2;
        long blockSize = 64;
        long maxLogins = 1;
        bool sampleRateSet = false;
        double sampleRate = 48000.0;
    };

    // Reads s.options.* as they ended up after the user startup file. A single
    // evaluate (avoids 13 round-trips) and parsing of lines "SC3OPT key val".
    ServerOpts readServerOptions() {
        ServerOpts o;
        clearPost();
        evaluate(
            "\"SC3OPT maxNodes \".post; s.options.maxNodes.postln;\n"
            "\"SC3OPT maxSynthDefs \".post; s.options.maxSynthDefs.postln;\n"
            "\"SC3OPT memSize \".post; s.options.memSize.postln;\n"
            "\"SC3OPT numWireBufs \".post; s.options.numWireBufs.postln;\n"
            "\"SC3OPT numRGens \".post; s.options.numRGens.postln;\n"
            "\"SC3OPT numBuffers \".post; s.options.numBuffers.postln;\n"
            "\"SC3OPT numAudioBusChannels \".post; s.options.numAudioBusChannels.postln;\n"
            "\"SC3OPT numInputBusChannels \".post; s.options.numInputBusChannels.postln;\n"
            "\"SC3OPT numOutputBusChannels \".post; s.options.numOutputBusChannels.postln;\n"
            "\"SC3OPT blockSize \".post; s.options.blockSize.postln;\n"
            "\"SC3OPT maxLogins \".post; s.options.maxLogins.postln;\n"
            "\"SC3OPT sampleRate \".post; s.options.sampleRate.postln;");
        o.maxNodes = parseLong("maxNodes", o.maxNodes);
        o.maxSynthDefs = parseLong("maxSynthDefs", o.maxSynthDefs);
        o.memSize = parseLong("memSize", o.memSize);
        o.numWireBufs = parseLong("numWireBufs", o.numWireBufs);
        o.numRGens = parseLong("numRGens", o.numRGens);
        o.numBuffers = parseLong("numBuffers", o.numBuffers);
        o.numAudioBusChannels = parseLong("numAudioBusChannels", o.numAudioBusChannels);
        o.numInputBusChannels = parseLong("numInputBusChannels", o.numInputBusChannels);
        o.numOutputBusChannels = parseLong("numOutputBusChannels", o.numOutputBusChannels);
        o.blockSize = parseLong("blockSize", o.blockSize);
        o.maxLogins = parseLong("maxLogins", o.maxLogins);
        o.sampleRateSet = mPost.find("SC3OPT sampleRate nil") == std::string::npos;
        if (o.sampleRateSet)
            o.sampleRate = parseDouble("sampleRate", o.sampleRate);
        return o;
    }

    bool mCompileOk = false;
    bool mDegraded = false;      // true if it had to fall back to stock
    std::string mDegradeReport;  // excluded paths + why (error outlet)

    // --- captura --------------------------------------------------------------
    std::string mPost; // salida de postln/post
    std::string mErr;  // interpreter errors

private:
    // Excludes EVERYTHING coming from the user (includePaths of sclang_conf.yaml,
    // UserExtension, SystemExtension) and leaves only the stock class library (which
    // includes SCMaxCompat, because the `make compat` fixture copies it inside). The
    // paths are those of the config vector: removeIncludedDirectory and
    // addExcludedDirectory are exact-match, enough here (forEachIncludedDirectory
    // checks the top-level dirs; pathIsExcluded is applied file by file).
    bool degradeToStock() {
        if (!gLanguageConfig)
            return false;
        namespace fs = std::filesystem;
        const fs::path stock = fs::path(SC_Filesystem::instance().getDirectory(
                                    SC_Filesystem::DirName::Resource)) /
                               "SCClassLibrary";

        mDegradeReport.clear();
        mDegradeReport += "class library del usuario no compila (`Library has not been "
                          "compiled successfully`); degradando a stock + SCMaxCompat\n";

        // (1) includePaths del usuario (sclang_conf.yaml / Quarks).
        const std::vector<fs::path> included(gLanguageConfig->includedDirectories());
        for (const auto& p : included) {
            if (p.lexically_normal() == stock.lexically_normal())
                continue;
            if (gLanguageConfig->removeIncludedDirectory(p))
                mDegradeReport += "  - exclude (includePaths): " + p.string() + "\n";
        }

        // (2) dirs default no-stock: SystemExtension y UserExtension.
        const std::vector<fs::path> defaults(gLanguageConfig->defaultClassLibraryDirectories());
        for (const auto& p : defaults) {
            if (p.lexically_normal() == stock.lexically_normal())
                continue;
            if (gLanguageConfig->addExcludedDirectory(p))
                mDegradeReport += "  - exclude (default dir): " + p.string() + "\n";
        }

        // (3) Recompile (compileLibrary → shutdownLibrary on entry). The user
        // startup file is evaluated again (parity); if it depends on excluded
        // Quarks, its errors stay in mErr and do not abort the process.
        clearPost();
        bool ok = recompile(false);
        if (!ok)
            mDegradeReport += "  ! stock + SCMaxCompat ALSO failed to compile\n";
        return ok;
    }

    long parseLong(const char* key, long fallback) {
        const std::string tag = std::string("SC3OPT ") + key + " ";
        size_t pos = mPost.find(tag);
        if (pos == std::string::npos)
            return fallback;
        const char* s = mPost.c_str() + pos + tag.size();
        char* end = nullptr;
        long v = std::strtol(s, &end, 10);
        return (end == s) ? fallback : v;
    }

    double parseDouble(const char* key, double fallback) {
        const std::string tag = std::string("SC3OPT ") + key + " ";
        size_t pos = mPost.find(tag);
        if (pos == std::string::npos)
            return fallback;
        const char* s = mPost.c_str() + pos + tag.size();
        char* end = nullptr;
        double v = std::strtod(s, &end);
        return (end == s) ? fallback : v;
    }
};
