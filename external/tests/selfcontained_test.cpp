// SPDX-License-Identifier: GPL-3.0-only
// selfcontained_test.cpp — issue #1, item 9 (D3): the user's Extensions and
// Quarks still compile and sound on top of the PACKAGED class library.
//
// What this covers that engine_test does not: engine_test runs against a real
// SuperCollider installation and only touches the payload with SC3_RESOURCES_DIR.
// Here the payload IS the installation (as inside the .mxo) and the class library
// it compiles is the bundle's, while the user dirs — Extensions and the Quarks
// that a sclang_conf.yaml lists — keep coming from the user, unchanged. The two
// mechanisms have different owners in sclang (the default class library dirs vs
// `includePaths`), so both are exercised:
//
//   <home>/Library/Application Support/SuperCollider/Extensions/Sc3TestExt.sc
//        → default UserExtension dir (no yaml involved)
//   <home>/.../downloaded-quarks/Sc3TestQuark/Sc3TestQuark.sc
//        → includePaths of <config>/sclang_conf.yaml (what Quarks writes)
//
// Hermetic: HOME points at a temp dir for the whole run, so the real user's
// Quarks (and a broken one of them) can never decide the result. The engine is a
// process-wide singleton, so each case runs in its own child.
//
// Usage: ./build/selfcontained_test [payloadDir] [case]
//        payloadDir default: $SC3_RESOURCES_DIR, else build/payload/Contents/Resources
//                            (`make payload` stages it with scripts/stage_bundle.sh)

#include "../Sc3Engine.h"

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
static bool check(bool cond, const std::string &what)
{
	gOk = gOk && cond;
	std::printf("[%s] %s\n", cond ? "OK  " : "FAIL", what.c_str());
	return cond;
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

static bool has(const std::string &hay, const std::string &needle)
{
	return hay.find(needle) != std::string::npos;
}

struct Outlets {
	std::string post, err;
	std::string all() const { return post + err; }
};

// Accumulates both outlets until `needle` shows up (or the timeout expires).
static Outlets waitFor(Sc3Engine &eng, const char *needle, int timeoutMs)
{
	Outlets acc;
	const auto t0 = std::chrono::steady_clock::now();
	for (;;) {
		acc.post += eng.takePost();
		acc.err += eng.takeError();
		if (has(acc.all(), needle))
			return acc;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(
		        std::chrono::steady_clock::now() - t0)
		        .count() > timeoutMs)
			return acc;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

// Touches the World like the Max audio thread does (nothing advances on its own).
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

// Runs `defName` (built by the fixture) and measures it. Proves the whole chain:
// the directories compiled on top of the packaged library AND the World runs
// what they produce.
static bool plays(Sc3Engine &eng, const char *defName, double &rmsOut)
{
	const long n = eng.blockLength();
	if (n <= 0)
		return false;
	std::vector<float> l((size_t)n), r((size_t)n);
	const long units0 = eng.numUnits();
	eng.eval(std::string("y = Synth(\\") + defName + ");");
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

// ------------------------------------------------------------------ payload

static std::string payloadDir()
{
	const char *env = std::getenv("SC3_RESOURCES_DIR");
	if (env && *env)
		return env;
	return "build/payload/Contents/Resources";
}

static Sc3EngineConfig payloadCfg(const std::string &res)
{
	Sc3EngineConfig cfg;
	cfg.port = 0;           // auto: no need to close Max or the SC IDE
	cfg.resourcesDir = res; // the packaged library IS the installation
	cfg.compatDir = res + "/SCMaxCompat";
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;
	return cfg;
}

// --------------------------------------------------------------- fixtures

// A temp HOME holding the user's two dirs and the yaml Quarks would write.
struct Fixture {
	std::string root, home, config, brokenQuark;
	bool ok = false;

	explicit Fixture(bool broken, const std::string &res)
	{
		char tmpl[] = "/tmp/sc3_selfcontained_XXXXXX";
		if (!mkdtemp(tmpl))
			return;
		root = tmpl;
		home = root + "/home";
		config = home + "/Library/Application Support/SuperCollider";
		const std::string ext = config + "/Extensions";
		const std::string quark = config + "/downloaded-quarks/Sc3TestQuark";
		std::error_code ec;
		fs::create_directories(ext, ec);
		fs::create_directories(quark, ec);
		if (ec) {
			std::printf("[FAIL] mkdir: %s\n", ec.message().c_str());
			return;
		}
		// User Extension (default dir): a class with a tag and two constants.
		writeFile(ext + "/Sc3TestExt.sc",
		          "Sc3TestExt {\n"
		          "\t*tag { ^'extension' }\n"
		          "\t*freq { ^440 }\n"
		          "\t*amp { ^0.7 }\n"
		          "}\n");
		// User Quark (includePaths): a class that USES the extension one, so the
		// SynthDef only exists if BOTH directories were compiled.
		writeFile(quark + "/Sc3TestQuark.sc",
		          "Sc3TestQuark {\n"
		          "\t*tag { ^'quark' }\n"
		          "\t*build {\n"
		          "\t\t^SynthDef(\\sc3test_quark_tone, { Out.ar(0,\n"
		          "\t\t\tSinOsc.ar(Sc3TestExt.freq, 0, Sc3TestExt.amp)) })\n"
		          "\t}\n"
		          "}\n");

		std::string yaml = "includePaths:\n";
		if (broken) {
			brokenQuark = root + "/quarks_broken";
			fs::create_directories(brokenQuark, ec);
			writeFile(brokenQuark + "/Broken.sc", "Broken_this is ((( not valid SC code\n");
			yaml += "    - " + brokenQuark + "\n";
		}
		yaml += "    - " + quark + "\n" +
		        "excludePaths: []\n"
		        "postInlineWarnings: false\n"
		        "excludeDefaultPaths: false\n";
		ok = writeFile(config + "/sclang_conf.yaml", yaml);
		// A payload without the class library would make every check below pass
		// for the wrong reason (nothing to compile).
		ok = ok && fs::exists(fs::path(res) / "SCClassLibrary/Common");
	}

	~Fixture()
	{
		if (root.empty())
			return;
		if (!gOk) {
			std::printf("kept for inspection: %s\n", root.c_str());
			return;
		}
		std::error_code ec;
		fs::remove_all(root, ec);
	}
};

// ------------------------------------------------------------------- cases

// The payload boots as the installation and the user dirs ride on top of it.
static int caseUsers(const std::string &res)
{
	Fixture fx(false, res);
	if (!check(fx.ok, "fixture: temp HOME with Extensions + Quark + yaml"))
		return 1;
	setenv("HOME", fx.home.c_str(), 1); // BEFORE any SC_Filesystem call

	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg = payloadCfg(res);
	cfg.userConfigDir = fx.config; // where the yaml of this HOME lives
	const bool opened = eng.open(cfg);
	const bool ready = opened && eng.waitReady(120000);
	Sc3EngineStatus st = eng.status();
	if (!check(ready, "boot ready on the packaged payload (" +
	                      std::to_string(st.bootMs) + " ms)")) {
		std::printf("       error: %s\n", st.error.c_str());
		eng.release();
		return 1;
	}
	check(st.selfContained, "status: selfContained = true (library comes from the payload)");
	check(st.classLibraryDir == res, "status: classLibraryDir = " + st.classLibraryDir);
	check(has(st.ugensPath, res + "/plugins"), "status: ugensPath uses the packaged .scx");
	check(st.versionNote.empty(), "status: no version note (nothing to be out of sync with)");
	check(st.smokeOk, "status: smoke test ran (primitives + packaged UGens)");

	eng.eval("Sc3TestExt.tag"); // default UserExtension dir
	Outlets e = waitFor(eng, "-> extension", 15000);
	check(has(e.post, "-> extension"),
	      "user Extension class evaluates (-> extension)" +
	          (has(e.post, "-> extension") ? std::string()
	                                       : std::string(" got: ") + e.all()));

	eng.eval("Sc3TestQuark.tag"); // includePaths of the yaml
	Outlets q = waitFor(eng, "-> quark", 8000);
	check(has(q.post, "-> quark"), "user Quark class evaluates (-> quark)" +
	                                   (has(q.post, "-> quark") ? std::string()
	                                                            : std::string(" got: ") + q.all()));

	eng.eval("Sc3TestQuark.build.add;");
	pump(eng, 25);
	double rms = 0.0;
	const bool played = plays(eng, "sc3test_quark_tone", rms);
	check(played, "SynthDef built by the Quark class (it calls the Extension) runs");
	check(!played || rms > 0.05, "and it sounds: RMS = " + std::to_string(rms));

	eng.release();
	return gOk ? 0 : 1;
}

// Degraded boot with the payload: one broken user Quark and the object still
// boots on the packaged stock library + SCMaxCompat, keeps evaluating and keeps
// sounding (the fallback the .mxo promises when a user library is broken).
static int caseDegraded(const std::string &res)
{
	Fixture fx(true, res);
	if (!check(fx.ok, "fixture: broken Quark on top of the payload"))
		return 1;
	setenv("HOME", fx.home.c_str(), 1);

	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg = payloadCfg(res);
	cfg.userConfigDir = fx.config;
	const bool opened = eng.open(cfg);
	const bool ready = opened && eng.waitReady(120000);
	Sc3EngineStatus st = eng.status();
	check(ready && st.degraded, "broken Quark: boot ready and degraded = true");
	check(has(st.degradeReport, "quarks_broken"),
	      "degrade report names the excluded path");
	if (!ready) {
		std::printf("       error: %s (report: %s)\n", st.error.c_str(),
		            st.degradeReport.c_str());
		eng.release();
		return 1;
	}
	check(st.selfContained && st.classLibraryDir == res,
	      "degraded boot still runs on the packaged library");

	eng.eval("\\Sc3TestQuark.asClass.notNil"); // the dropped includePath really is gone
	Outlets gone = waitFor(eng, "-> false", 8000);
	check(has(gone.post, "-> false"), "the user Quark is NOT compiled in the degraded boot");

	eng.eval("1+1");
	Outlets a = waitFor(eng, "-> 2", 8000);
	check(has(a.post, "-> 2"), "the stock interpreter keeps evaluating (-> 2)");

	eng.eval("SynthDef(\\sc3test_stock_tone, { Out.ar(0, SinOsc.ar(440, 0, 0.7)) }).add;");
	pump(eng, 25);
	double rms = 0.0;
	const bool played = plays(eng, "sc3test_stock_tone", rms);
	check(played, "degraded boot: a Synth runs on the packaged stock library");
	check(!played || rms > 0.05, "and it sounds: RMS = " + std::to_string(rms));

	eng.release();
	return gOk ? 0 : 1;
}

// -------------------------------------------------------------------- main

int main(int argc, char **argv)
{
	// Runner: no args (payload from the env/default). Child: <case> <payload>.
	const bool asCase = argc > 2;
	const std::string res = asCase ? argv[2] : payloadDir();
	struct Case {
		const char *name;
		int (*fn)(const std::string &);
	} cases[] = {{"users", caseUsers}, {"degraded", caseDegraded}};
	const int numCases = (int)(sizeof(cases) / sizeof(cases[0]));

	// `selfcontained_test <case> <payload>`: one case, this process (the runner
	// re-executes itself this way, one fresh VM + World per case).
	if (asCase) {
		for (const Case &c : cases)
			if (std::strcmp(argv[1], c.name) == 0)
				return c.fn(res);
		std::printf("[FAIL] unknown case: %s\n", argv[1]);
		return 2;
	}

	if (!fs::exists(fs::path(res) / "SCClassLibrary/Common")) {
		std::printf("[FAIL] no payload at %s (run 'make payload' in external/)\n", res.c_str());
		return 1;
	}
	std::printf("=== self-contained payload: user Extensions + Quarks (%s) ===\n", res.c_str());
	int okCount = 0;
	for (const Case &c : cases) {
		pid_t pid = fork();
		if (pid == 0) {
			execl(argv[0], argv[0], c.name, res.c_str(), (char *)nullptr);
			std::printf("[FAIL] %s: could not exec %s\n", c.name, argv[0]);
			_exit(127);
		}
		int status = 0;
		if (pid < 0 || waitpid(pid, &status, 0) < 0) {
			std::printf("[FAIL] %s: fork/wait failed\n", c.name);
			continue;
		}
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			++okCount;
			std::printf("--- %s: OK ---\n", c.name);
		} else {
			std::printf("--- %s: FAIL (exit %d) ---\n", c.name,
			            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
		}
	}
	const bool all = okCount == numCases;
	std::printf("[%s] self-contained payload: %d/%d cases\n", all ? "OK  " : "FAIL", okCount,
	            numCases);
	return all ? 0 : 1;
}
