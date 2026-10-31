// SPDX-License-Identifier: GPL-3.0-only
// Sc3LangClient.h
// Subclass of SC_LanguageClient for headless tests (Phase 1, external sc3~).
//
// Reproduces the IDE REPL: a SINGLE long-lived sclang interpreter (not one per
// evaluation). Evaluating code here keeps state across calls (global variables
// ~a/~s, SynthDefs, etc.), just like cmd+enter/shift+enter in SCIDE.
//
// Interpreter output (postln / errors) is captured in memory in mPost/mErr
// (decision: in-memory capture, not stdout).
//
// Boot in 3 steps (v0.14 decision, "sclang first" order):
//   1. boot()  → initRuntime + compileLibrary (evaluates the user's startup.scd;
//                the SCMaxCompat module neutralizes s.boot/s.quit).
//                If the user library does NOT compile → degrade to stock + SCMaxCompat
//                (decision: the external is NEVER left dead; reported in mDegradeReport).
//   2. readServerOptions() → reads s.options.* ALREADY configured by their startup.scd.
//   3. The caller creates the World with those options and adopts the server.
#pragma once

#include "SC_LanguageClient.h"

#include "SC_Filesystem.hpp"     // DirName::Resource (path of the stock class library)
#include "SC_LanguageConfig.hpp" // gLanguageConfig (fallback a stock)

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

// compiledOK is defined by the lexer (lang/LangSource/PyrLexer.cpp:123, declared
// in PyrLexer.h). The MEMBER version of compileLibrary() is void, so the result of
// compiling the class library is read from here.
extern bool compiledOK;

class Sc3LangClient : public SC_LanguageClient {
public:
    Sc3LangClient(const std::string& name = "sc3-lang") : SC_LanguageClient(name) {}

    // --- output hooks (pure virtuals of SC_LanguageClient) --------------------
    // postText: code output (postln/post) — we capture it.
    void postText(const char* str, size_t len) override { mPost.append(str, len); }
    // postFlush: separator between output "chunks" (the REPL emits a line).
    void postFlush(const char* str, size_t len) override { mPost.append(str, len); }
    // postError: interpreter error messages.
    void postError(const char* str, size_t len) override { mErr.append(str, len); }
    // flush: at the end of an interpretation.
    void flush() override {}

    // --- high-level API (wraps the interpreter API) ---------------------------
    // Starts the runtime + compiles the library (vanilla or with the user's
    // Quarks). `withUserQuarks=false` → vanilla (stock). true → includes the
    // user's includePaths.
    //
    // TOLERANT boot: if compiledOK == false (a user Quark with a syntax error
    // leaves the whole library dead), it excludes the user paths and recompiles
    // stock + SCMaxCompat. Verified: compileLibrary → shutdownLibrary on entry, so
    // the recovery happens in the same process.
    bool boot(bool withUserQuarks) {
        Options opt;
        // Alternate port so as not to collide with the user's IDE (57120 def).
        opt.mPort = 57121;
        initRuntime(opt);
        // compileLibrary compiles the library and triggers Main.startup
        // (evaluates the user's startup file → IDE parity). The Main process is
        // created but its `s` (interpreter.s) is not mounted in the headless
        // evaluation scope → we mount it by hand (see findings, HANDOFF §3.5).
        // compileLibrary() RETURNS compiledOK (PyrLexer.cpp): false if the class
        // library did not compile (e.g. a user quark with a syntax error).
        compileLibrary(withUserQuarks);
        mCompileOk = compiledOK;
        if (!mCompileOk) {
            // Degradation: stock + SCMaxCompat, reporting what was excluded.
            mDegraded = true;
            mCompileOk = degradeToStock();
        }
        // Mount s/Server.default in the evaluation scope (IDE/REPL parity).
        if (mCompileOk)
            evaluate("s = Server.default");
        // Process scheduled events from startup.
        for (int i = 0; i < 8; ++i)
            tick();
        return mCompileOk;
    }

    // Evaluates a chunk of SC code (multi-line / multi-declaration), like the REPL.
    //
    // `printResult` reproduces the `-> value` of the IDE post window: it uses
    // interpretPrintCmdLine (the "not silent" mode of SC_TerminalClient), which
    // prints `("-> " ++ res).postln` (Kernel.sc: Interpreter:interpretPrintCmdLine).
    // Internal uses (boot, s.options, adoption) leave it false so as not to fill
    // the user's out with `-> nil`.
    void evaluate(const std::string& code, bool printResult = false) {
        setCmdLine(code.c_str());
        if (printResult)
            interpretPrintCmdLine();
        else
            interpretCmdLine();
        for (int i = 0; i < 4; ++i)
            tick();
    }

    // Recompiles the library (compileLibrary does shutdownLibrary() on entry).
    // Used to degrade to "stock only" if the user library did not compile.
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

    // --- s.options → WorldOptions ("sclang first" order) ----------------------
    // The values the external has to respect. `sampleRate` stays nil in
    // ServerOptions until booted (default 48000 in the World if not set).
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
        long port = 57110; // Server.default.port: ACÁ manda sclang sus comandos
        bool sampleRateSet = false;
        double sampleRate = 48000.0;
    };

    // Reads s.options.* exactly as the user's startup file left them.
    // A single evaluate (avoids 13 round-trips) and parsing of "SC3OPT key val" lines.
    //
    // 3.4: the buffer is NOT cleared here. What the user's startup file posted
    // (and the degradation report) lives in the same buffer; clearing it threw it
    // away before the engine drained it to the `post`/`error` outlet. To avoid
    // reading stale values from a previous boot of the same process, only what was
    // appended from `start` onwards is parsed.
    //
    // `port` is `Server.port`/`addr.port` (the port sclang sends commands to), not
    // `s.options.port`: ServerOptions has no `port` in SC 3.14 and asking for it
    // posted an ERROR on every boot (leaving the bug #9 check in the fallback,
    // i.e. dead).
    ServerOpts readServerOptions() {
        ServerOpts o;
        const size_t start = mPost.size();
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
            "\"SC3OPT port \".post; s.addr.port.postln;\n"
            "\"SC3OPT sampleRate \".post; s.options.sampleRate.postln;");
        o.maxNodes = parseLong("maxNodes", o.maxNodes, start);
        o.maxSynthDefs = parseLong("maxSynthDefs", o.maxSynthDefs, start);
        o.memSize = parseLong("memSize", o.memSize, start);
        o.numWireBufs = parseLong("numWireBufs", o.numWireBufs, start);
        o.numRGens = parseLong("numRGens", o.numRGens, start);
        o.numBuffers = parseLong("numBuffers", o.numBuffers, start);
        o.numAudioBusChannels = parseLong("numAudioBusChannels", o.numAudioBusChannels, start);
        o.numInputBusChannels = parseLong("numInputBusChannels", o.numInputBusChannels, start);
        o.numOutputBusChannels = parseLong("numOutputBusChannels", o.numOutputBusChannels, start);
        o.blockSize = parseLong("blockSize", o.blockSize, start);
        o.maxLogins = parseLong("maxLogins", o.maxLogins, start);
        o.port = parseLong("port", o.port, start);
        o.sampleRateSet = mPost.find("SC3OPT sampleRate nil", start) == std::string::npos;
        if (o.sampleRateSet)
            o.sampleRate = parseDouble("sampleRate", o.sampleRate, start);
        return o;
    }

    bool mCompileOk = false;
    bool mDegraded = false;      // true if it had to fall back to stock
    std::string mDegradeReport;  // excluded paths + why (error outlet)
    // Include dirs that are NOT the user's and must survive the fallback to stock
    // (the SCMaxCompat module lives outside the system class library).
    std::vector<std::string> mKeepPaths;

    // --- capture --------------------------------------------------------------
    std::string mPost; // salida de postln/post
    std::string mErr;  // interpreter errors

private:
    // Excludes EVERYTHING coming from the user (includePaths from
    // sclang_conf.yaml, UserExtension, SystemExtension) and leaves only the stock
    // class library (which includes SCMaxCompat, because the `make compat` fixture
    // copies it inside). The paths are the config vector's: removeIncludedDirectory
    // and addExcludedDirectory are exact-match, enough here
    // (forEachIncludedDirectory checks top-level dirs; pathIsExcluded applies
    // file by file).
    bool degradeToStock() {
        if (!gLanguageConfig)
            return false;
        namespace fs = std::filesystem;
        const fs::path stock = fs::path(SC_Filesystem::instance().getDirectory(
                                    SC_Filesystem::DirName::Resource)) /
                               "SCClassLibrary";

        mDegradeReport.clear();
        mDegradeReport += "user class library does not compile (`Library has not been "
                          "compiled successfully`); falling back to stock + SCMaxCompat\n";

        // (1) the user's includePaths (sclang_conf.yaml / Quarks), minus the
        //     ones the external injects (mKeepPaths: SCMaxCompat).
        auto isKept = [this](const fs::path& p) {
            const fs::path norm = p.lexically_normal();
            for (const auto& keep : mKeepPaths) {
                if (norm == fs::path(keep).lexically_normal())
                    return true;
            }
            return false;
        };
        const std::vector<fs::path> included(gLanguageConfig->includedDirectories());
        for (const auto& p : included) {
            if (p.lexically_normal() == stock.lexically_normal())
                continue;
            if (isKept(p)) {
                mDegradeReport += "  = keep (external): " + p.string() + "\n";
                continue;
            }
            if (gLanguageConfig->removeIncludedDirectory(p))
                mDegradeReport += "  - exclude (includePaths): " + p.string() + "\n";
        }

        // (2) non-stock default dirs: SystemExtension and UserExtension.
        const std::vector<fs::path> defaults(gLanguageConfig->defaultClassLibraryDirectories());
        for (const auto& p : defaults) {
            if (p.lexically_normal() == stock.lexically_normal())
                continue;
            if (gLanguageConfig->addExcludedDirectory(p))
                mDegradeReport += "  - exclude (default dir): " + p.string() + "\n";
        }

        // (2b) The external's module is never excluded: if it ended up in the
        //      excluded list (or never got in), it is re-added as an include dir.
        for (const auto& keep : mKeepPaths) {
            const fs::path p(keep);
            gLanguageConfig->removeExcludedDirectory(p);
            gLanguageConfig->addIncludedDirectory(p);
        }

        // (3) Recompile (compileLibrary → shutdownLibrary on entry). The user's
        // startup file is evaluated again (parity); if it depends on excluded
        // Quarks, their errors stay in mErr, they do not abort the process.
        clearPost();
        bool ok = recompile(false);
        if (!ok)
            mDegradeReport += "  ! stock + SCMaxCompat ALSO failed to compile\n";
        return ok;
    }

    long parseLong(const char* key, long fallback, size_t from = 0) {
        const std::string tag = std::string("SC3OPT ") + key + " ";
        size_t pos = mPost.find(tag, from);
        if (pos == std::string::npos)
            return fallback;
        const char* s = mPost.c_str() + pos + tag.size();
        char* end = nullptr;
        long v = std::strtol(s, &end, 10);
        return (end == s) ? fallback : v;
    }

    double parseDouble(const char* key, double fallback, size_t from = 0) {
        const std::string tag = std::string("SC3OPT ") + key + " ";
        size_t pos = mPost.find(tag, from);
        if (pos == std::string::npos)
            return fallback;
        const char* s = mPost.c_str() + pos + tag.size();
        char* end = nullptr;
        double v = std::strtod(s, &end);
        return (end == s) ? fallback : v;
    }
};
