// SPDX-License-Identifier: GPL-3.0-only
// fail_test.cpp — 3.4: failure matrix of the engine (headless, no Max).
//
// One case = one process: the sclang VM and the World live as long as the
// process (/#14), so chaining boots with different configs inside one
// binary would test mixed states. `fail_test` (no arguments) is the runner: it
// forks once per case and adds up the results.
//
//   ./build/fail_test                 # all (5 × ~4 s + 1 fast)
//   ./build/fail_test <case>          # one case in this process
//
// Env: SC3_PORT (default 0 = auto), SC3_CL_DIR, SC3_COMPAT_DIR, SC3_APP_PATH,
//      SC3_DEBUG_OSC=1.
// Port: default 0 = AUTO (the kernel picks it) ⇒ runs with Max / the IDE open.
//
// Rows of the matrix (ROADMAP) and where they are covered:
//   Quark with a syntax error      → case `badquark`
//   startup.scd that throws        → case `startup`
//   corrupt .scx                   → case `brokenplug`
//   s.options.port ≠ World → case `port`
//   impossible boot (class lib)    → case `badcl`  (same path as “World_New
//                                   fails”: every `return false` of the boot
//                                   leaves state=Failed + error + engine
//                                   reopenable; the retry is covered in
//                                   engine_test)
//   eval with an exception         → case `badeval`

#include "../Sc3Engine.h"

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

static bool gOk = true;
// Temp dirs mkTempDir() created in this process; see removeTempDirs().
static std::vector<std::string> gTempDirs;

static bool check(bool cond, const std::string &what)
{
	gOk = gOk && cond;
	std::printf("[%s] %s\n", cond ? "OK  " : "FAIL", what.c_str());
	return cond;
}

// ---------------------------------------------------------------- helpers

static Sc3EngineConfig baseCfg()
{
	Sc3EngineConfig cfg;
	const char *port = std::getenv("SC3_PORT");
	cfg.port = port ? std::atoi(port) : 0; // 4.x: 0 = auto
	const char *cldir = std::getenv("SC3_CL_DIR");
	cfg.classLibraryDir = cldir ? cldir : ""; // empty ⇒ real SC installation
	const char *compat = std::getenv("SC3_COMPAT_DIR");
	cfg.compatDir = compat ? compat : "../phase1/compat/SCMaxCompat";
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;
	return cfg;
}

static std::string scAppPath()
{
	const char *app = std::getenv("SC3_APP_PATH");
	return app ? app : "/Applications/SuperCollider.app";
}

static std::string mkTempDir(const char *tag)
{
	char tmpl[256];
	std::snprintf(tmpl, sizeof(tmpl), "/tmp/sc3_fail_%s_XXXXXX", tag);
	if (!mkdtemp(tmpl))
		return {};
	gTempDirs.push_back(tmpl);
	return tmpl;
}

/* Remove the temp dirs this process created. A case that failed keeps them (and
   prints the paths) because the fake SC installation it wrote — bad class library,
   bad startup.scd, corrupt .scx, stale version — is exactly what you need to look
   at. Runs in the per-case child, right after the case function returns. */
static void removeTempDirs(bool clean)
{
	for (const std::string &d : gTempDirs) {
		if (!clean) {
			std::printf("kept for inspection: %s\n", d.c_str());
			continue;
		}
		std::error_code ec;
		fs::remove_all(d, ec);
	}
	gTempDirs.clear();
}

static bool writeFile(const std::string &path, const std::string &text)
{
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f)
		return false;
	std::fwrite(text.data(), 1, text.size(), f);
	std::fclose(f);
	return true;
}

static bool has(const std::string &hay, const char *needle)
{
	return hay.find(needle) != std::string::npos;
}

// Accumulates text from the outlets until `needle` shows up (or the timeout
// expires). `takePost/takeError` consume, so it accumulates (post + error
// separately, to be able to assert which one each thing came out on).
struct Outlets {
	std::string post, err;
	std::string all() const { return post + err; }
};

static Outlets drain(Sc3Engine &eng, Outlets acc)
{
	acc.post += eng.takePost();
	acc.err += eng.takeError();
	return acc;
}

static Outlets waitFor(Sc3Engine &eng, const char *needle, int timeoutMs, Outlets acc = {})
{
	const auto t0 = std::chrono::steady_clock::now();
	for (;;) {
		acc = drain(eng, acc);
		if (has(acc.all(), needle))
			return acc;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(
		        std::chrono::steady_clock::now() - t0)
		        .count() > timeoutMs)
			return acc;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

// Touches the World like the Max audio thread does (without DSP nothing
// advances on its own).
static void pump(Sc3Engine &eng, int blocks)
{
	const long n = eng.blockLength();
	if (n <= 0)
		return;
	std::vector<float> l((size_t)n), r((size_t)n);
	for (int i = 0; i < blocks; ++i) {
		eng.renderBlock(l.data(), r.data());
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

// SynthDef + Synth + 0.3 s of capture: proves the World keeps processing
// audio (what the matrix asks for in “corrupt .scx → audio OK”).
static bool playsTone(Sc3Engine &eng, double &rmsOut)
{
	const long n = eng.blockLength();
	if (n <= 0)
		return false;
	std::vector<float> l((size_t)n), r((size_t)n);
	eng.eval("SynthDef(\\failtest_sine, { Out.ar(0, SinOsc.ar(440, 0, 0.8)) }).add;");
	pump(eng, 25);
	const long units0 = eng.numUnits();
	eng.eval("x = Synth(\\failtest_sine);");
	bool unitsOk = false;
	for (int i = 0; i < 300 && !unitsOk; ++i) {
		pump(eng, 1);
		unitsOk = eng.numUnits() > units0;
	}
	if (!unitsOk)
		return false;

	double sum = 0.0;
	long frames = 0;
	for (int b = 0; b < 40; ++b) {
		eng.renderBlock(l.data(), r.data());
		for (long i = 0; i < n; ++i) {
			sum += (double)l[(size_t)i] * l[(size_t)i];
			++frames;
		}
	}
	rmsOut = frames ? std::sqrt(sum / (double)frames) : 0.0;
	return true;
}

// ---------------------------------------------------------------- casos

// impossible boot: nonexistent class library. Same error path as "World_New
// fails": `return false` ⇒ state=Failed, `error` filled in, process alive. The
// engine stays reopenable (`open` again) — that is covered by engine_test with
// the port busy, which is the reachable version of this row.
static int caseBadCl()
{
	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg = baseCfg();
	// Unique dir: sclang creates an empty `<Resource>/SCClassLibrary` in the
	// compile attempt, so with a fixed path the 2nd run would see "the path
	// exists" and the error would change (mkdir -p inside compileLibrary).
	const std::string dir = mkTempDir("badcl");
	if (dir.empty())
		return 1;
	cfg.classLibraryDir = dir + "/no_existe";
	cfg.compatDir = "";
	const bool opened = eng.open(cfg);
	const bool ready = eng.waitReady(60000);
	Sc3EngineStatus st = eng.status();
	check(opened, "open() does not crash with a nonexistent class library");
	check(!ready && st.failed, "impossible boot: state=failed and it does not hang");
	check(has(st.error, "class library"), "impossible boot: `error` explains the cause → \"" +
	                                          st.error + "\"");
	check(!eng.isReady() && eng.numUnits() == 0, "object alive and silent (no World, no units)");
	eng.release();
	return gOk ? 0 : 1;
}

// user startup.scd that throws: the boot has to complete anyway (the
// external is never left dead) and the error has to reach the outlet.
static int caseStartup()
{
	const std::string dir = mkTempDir("startup");
	if (dir.empty())
		return 1;
	// Markers per FILE, not per postln: the text the startup posts while
	// `compileSucceeded` runs does not always reach the client buffer (postln
	// there gets lost). What is deterministic is its effect.
	const std::string ran = dir + "/ran.txt";
	const std::string after = dir + "/after.txt";
	const bool wrote = writeFile(dir + "/startup.scd",
	                             "File(\"" + ran + "\", \"w+\").write(\"ran\").close;\n"
	                             "Error(\"SC3-MATRIX-THROW\").throw;\n"
	                             "File(\"" + after + "\", \"w+\").write(\"after\").close;\n");
	Sc3EngineConfig cfg = baseCfg();
	cfg.userConfigDir = dir;
	Sc3Engine &eng = Sc3Engine::instance();
	const bool opened = eng.open(cfg);
	const bool ready = eng.waitReady(90000);
	Outlets o = waitFor(eng, "SC3-MATRIX-THROW", 3000);
	const std::string all = o.all();

	check(wrote && opened, "startup: fixtures written and open() accepted");
	check(ready && eng.isReady(), "startup that throws: the boot completes anyway (ready)");
	check(fs::exists(ran), "startup: the user's startup.scd ran");
	check(!fs::exists(after),
	      "startup: the exception cut the startup short (the next line did not run)");
	check(has(all, "SC3-MATRIX-THROW"),
	      "startup: the startup error reaches the `post`/`error` outlet");

	// The engine stays usable: that is the point of the row ("boot
	// continues").
	eng.eval("1+1");
	Outlets afterEval = waitFor(eng, "-> 2", 3000, o);
	check(has(afterEval.post, "-> 2"), "startup: after the failure the interpreter keeps evaluating");
	eng.release();
	return gOk ? 0 : 1;
}

// The sclang Server points to another port: fail-fast with an actionable
// message instead of an "OK" boot with units=0 forever.
static int casePort()
{
	const std::string dir = mkTempDir("port");
	if (dir.empty())
		return 1;
	writeFile(dir + "/startup.scd", "s.addr = NetAddr(\"127.0.0.1\", 57120);\n");
	Sc3EngineConfig cfg = baseCfg();
	cfg.userConfigDir = dir;
	Sc3Engine &eng = Sc3Engine::instance();
	eng.open(cfg);
	const bool ready = eng.waitReady(90000);
	Sc3EngineStatus st = eng.status();
	check(!ready && st.failed, "sclang port ≠ World: the boot fails (a silent World is not adopted)");
	check(has(st.error, "57120") && has(st.error, "port"),
	      "port: `error` names both ports → \"" + st.error + "\"");
	eng.release();
	return gOk ? 0 : 1;
}

// Corrupt .scx in the plugins path: scsynth posts it (scprintf → `error`
// outlet with the 3.4 hook) and continues; audio has to work.
static int caseBrokenPlug()
{
	const std::string dir = mkTempDir("plug");
	if (dir.empty())
		return 1;
	writeFile(dir + "/zz_sc3_broken.scx", "this is not a dylib\n");
	const std::string stock = scAppPath() + "/Contents/Resources/plugins";
	Sc3EngineConfig cfg = baseCfg();
	cfg.ugenPluginsPath = stock + ":" + dir; // exclusive ⇒ stock + the broken one
	Sc3Engine &eng = Sc3Engine::instance();
	eng.open(cfg);
	const bool ready = eng.waitReady(90000);
	Sc3EngineStatus st = eng.status();
	Outlets o = waitFor(eng, "dlopen", 2000);

	check(ready, "corrupt .scx: the boot completes (ready)");
	check(has(st.ugensPath, dir.c_str()), "corrupt .scx: the broken dir got into mUGensPluginPath");
	check(has(o.err, "dlopen") || has(o.err, "zz_sc3_broken"),
	      "corrupt .scx: the scsynth error reaches the `error` outlet");
	double rms = 0.0;
	check(playsTone(eng, rms) && rms > 0.01,
	      "corrupt .scx: the World keeps rendering audio (RMS=" +
	          std::to_string(rms) + ")");
	eng.release();
	return gOk ? 0 : 1;
}

// 4.2: fake .app to test the version policy without installing another SC. The
// only field touched is Info.plist; `Contents/Resources` is a symlink to the
// ones of the real installation (valid class library + plugins). That way the
// check runs through the real path (scAppPath, empty classLibraryDir).
static std::string fakeScApp(const std::string &dir, const char *version)
{
	const std::string app = dir + "/FakeSC.app";
	const std::string contents = app + "/Contents";
	std::error_code ec;
	fs::create_directories(contents, ec);
	if (ec)
		return {};
	fs::create_directory_symlink(scAppPath() + "/Contents/Resources",
	                             contents + "/Resources", ec);
	if (ec)
		return {};
	const std::string v(version);
	if (!writeFile(contents + "/Info.plist",
	               "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	               "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
	               "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
	               "<plist version=\"1.0\"><dict>\n"
	               "<key>CFBundleShortVersionString</key><string>" +
	                   v + "</string>\n"
	                       "<key>CFBundleVersion</key><string>" +
	                   v + "</string>\n"
	                       "</dict></plist>\n"))
		return {};
	return app;
}

// 4.2 (a): another PATCH (3.14.2) ⇒ accepted with a warning, without touching the
// engine pin.
static int caseVersionPatch()
{
	const std::string dir = mkTempDir("verpatch");
	if (dir.empty())
		return 1;
	const std::string app = fakeScApp(dir, "3.14.2");
	if (app.empty()) {
		check(false, "3.14.2: could not build the test .app");
		return 1;
	}
	Sc3EngineConfig cfg = baseCfg();
	cfg.scAppPath = app;
	cfg.classLibraryDir = ""; // ⇒ version check through the real path
	cfg.allowScVersion = false;
	Sc3Engine &eng = Sc3Engine::instance();
	eng.open(cfg);
	const bool ready = eng.waitReady(90000);
	Sc3EngineStatus st = eng.status();
	check(ready, "4.2(a) 3.14.2: the boot completes (same minor version)");
	check(st.smokeOk, "4.2(c) 3.14.2: the compatibility smoke passed");
	check(has(st.scVersion, "3.14.2"), "4.2(a) 3.14.2: scVersion reports the installation");
	check(has(st.versionNote, "3.14.2") && has(st.versionNote, "engine"),
	      "4.2(a) 3.14.2: there is a note in versionNote → \"" + st.versionNote + "\"");
	eng.release();
	return gOk ? 0 : 1;
}

// 4.2 (b): another MINOR (3.15) without permission ⇒ the boot fails with an
// actionable error (it names both versions and how to force it).
static int caseVersionMinor()
{
	const std::string dir = mkTempDir("verminor");
	if (dir.empty())
		return 1;
	const std::string app = fakeScApp(dir, "3.15.0");
	if (app.empty()) {
		check(false, "3.15: could not build the test .app");
		return 1;
	}
	Sc3EngineConfig cfg = baseCfg();
	cfg.scAppPath = app;
	cfg.classLibraryDir = "";
	cfg.allowScVersion = false;
	Sc3Engine &eng = Sc3Engine::instance();
	eng.open(cfg);
	const bool ready = eng.waitReady(60000);
	Sc3EngineStatus st = eng.status();
	check(!ready && st.failed, "4.2(b) 3.15: without allowscversion the boot fails");
	check(has(st.error, "3.15.0") && has(st.error, "3.14.1"),
	      "4.2(b) 3.15: the error names both versions → \"" + st.error + "\"");
	check(has(st.error, "allowscversion"),
	      "4.2(b) 3.15: the error says how to force it (`@allowscversion 1`)");
	eng.release();
	return gOk ? 0 : 1;
}

// 4.2 (b): the same 3.15 WITH allowScVersion ⇒ it starts, with a warning
// saying it is forced (and the smoke is still the safety net).
static int caseVersionMinorForced()
{
	const std::string dir = mkTempDir("verforced");
	if (dir.empty())
		return 1;
	const std::string app = fakeScApp(dir, "3.15.0");
	if (app.empty()) {
		check(false, "forced: could not build the test .app");
		return 1;
	}
	Sc3EngineConfig cfg = baseCfg();
	cfg.scAppPath = app;
	cfg.classLibraryDir = "";
	cfg.allowScVersion = true;
	Sc3Engine &eng = Sc3Engine::instance();
	eng.open(cfg);
	const bool ready = eng.waitReady(90000);
	Sc3EngineStatus st = eng.status();
	check(ready, "4.2(b) 3.15 forced: the boot completes");
	check(st.smokeOk, "4.2(c) 3.15 forced: the compatibility smoke passed");
	check(has(st.versionNote, "FORCED"),
	      "4.2(b) 3.15 forced: the note says FORCED → \"" + st.versionNote + "\"");
	eng.release();
	return gOk ? 0 : 1;
}

// eval with an exception: error to the outlet, server state untouched (it
// kills neither nodes nor the interpreter).
static int caseBadEval()
{
	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg = baseCfg();
	eng.open(cfg);
	if (!eng.waitReady(90000))
		return check(false, "badeval: the initial boot failed") ? 0 : 1;
	double rms = 0.0;
	const bool audio = playsTone(eng, rms);
	const long units0 = eng.numUnits();
	check(audio, "badeval: fixture playing before the broken eval (RMS=" +
	                 std::to_string(rms) + ")");

	eng.eval("1 + nil;");
	Outlets o = waitFor(eng, "ERROR", 3000);
	const std::string evText = o.all();
	const size_t errPos = evText.find("ERROR");
	check(errPos != std::string::npos,
	      "eval with exception: the error reaches the outlet → \"" +
	          (errPos == std::string::npos ? std::string("(none)")
	                                       : evText.substr(errPos, 90)) +
	          "\"");
	check(eng.numUnits() == units0, "eval with exception: server state intact (units=" +
	                                    std::to_string(eng.numUnits()) + ")");
	check(eng.isReady(), "eval with exception: the engine is still ready");

	eng.eval("1+1");
	Outlets after = waitFor(eng, "-> 2", 3000, o);
	check(has(after.post, "-> 2"), "eval with exception: the interpreter is still alive (-> 2)");
	eng.release();
	return gOk ? 0 : 1;
}

// Quark with a syntax error: the whole library does not compile ⇒ fallback to
// stock + SCMaxCompat, with the excluded paths in the report (PLAN decision
// 10).
static int caseBadQuark()
{
	const std::string dir = mkTempDir("quark");
	if (dir.empty())
		return 1;
	const std::string qdir = dir + "/quarks_broken";
	fs::create_directories(qdir);
	writeFile(qdir + "/Broken.sc", "Broken_this is ((( not valid SC code\n");
	const bool wrote = writeFile(dir + "/sclang_conf.yaml",
	                             "includePaths:\n"
	                             "    - " + qdir + "\n"
	                             "excludePaths: []\n"
	                             "postInlineWarnings: false\n"
	                             "excludeDefaultPaths: false\n");
	Sc3EngineConfig cfg = baseCfg();
	cfg.userConfigDir = dir;
	Sc3Engine &eng = Sc3Engine::instance();
	eng.open(cfg);
	const bool ready = eng.waitReady(90000);
	Sc3EngineStatus st = eng.status();
	check(wrote, "badquark: yaml with the broken includePath created (" + qdir + ")");
	check(ready, "broken Quark: the boot completes anyway (degrades, does not die)");
	check(st.degraded, "Quark roto: `degraded` true (fallback a stock + SCMaxCompat)");
	check(has(st.degradeReport, "quarks_broken"),
	      "broken Quark: the report names the excluded path");

	eng.eval("1+1");
	Outlets after = waitFor(eng, "-> 2", 3000);
	check(has(after.post, "-> 2"), "broken Quark: the stock interpreter keeps evaluating (-> 2)");
	eng.release();
	return gOk ? 0 : 1;
}

// ---------------------------------------------------------------- runner

int main(int argc, char **argv)
{
	struct Case {
		const char *name;
		int (*fn)();
	};
	static const Case cases[] = {
	    {"badcl", caseBadCl},   {"startup", caseStartup}, {"port", casePort},
	    {"brokenplug", caseBrokenPlug}, {"badeval", caseBadEval}, {"badquark", caseBadQuark},
	    {"versionpatch", caseVersionPatch}, {"versionminor", caseVersionMinor},
	    {"versionminorforced", caseVersionMinorForced},
	};
	const int numCases = (int)(sizeof(cases) / sizeof(cases[0]));

	if (argc > 1) {
		for (const Case &c : cases)
			if (std::strcmp(argv[1], c.name) == 0) {
				const int rc = c.fn();
				removeTempDirs(rc == 0 && gOk);
				return rc;
			}
		std::printf("[FAIL] unknown case: %s\n", argv[1]);
		return 2;
	}

	std::printf("=== 3.4 failure matrix (%d cases, one process per case) ===\n", numCases);
	int okCount = 0;
	for (const Case &c : cases) {
		pid_t pid = fork();
		if (pid == 0) {
			// Child: clean process (new VM + World, like a freshly opened Max).
			execl(argv[0], argv[0], c.name, (char *)nullptr);
			std::printf("[FAIL] %s: no pude exec %s\n", c.name, argv[0]);
			_exit(127);
		}
		int status = 0;
		if (pid < 0 || waitpid(pid, &status, 0) < 0) {
			std::printf("[FAIL] %s: fork/wait failed\n", c.name);
			continue;
		}
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			okCount++;
			std::printf("--- %s: OK ---\n", c.name);
		} else {
			std::printf("--- %s: FAIL (exit %d%s) ---\n", c.name,
			            WIFEXITED(status) ? WEXITSTATUS(status) : -1,
			            WIFSIGNALED(status) ? ", signal" : "");
		}
	}
	const bool all = okCount == numCases;
	std::printf("[%s] failure matrix: %d/%d cases\n", all ? "OK  " : "FAIL", okCount, numCases);
	return all ? 0 : 1;
}
