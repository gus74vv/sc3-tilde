// SPDX-License-Identifier: GPL-3.0-only
// quarks_test <bundle Resources dir> ok|broken
//
// Boots the engine on the bundled class library, configured as sc3~ configures it,
// with SC's user dirs redirected to a temp dir (the real ones are never read).
//   ok:     a user Extension and a Quark (via sclang_conf.yaml includePaths) compile
//           and run; Platform.resourceDir is the bundle.
//   broken: the Quark has a syntax error; the boot degrades to stock + SCMaxCompat,
//           drops the user classes and reports both exclusions.
// One mode per process: the user config is fixed by the first boot of the VM.

#include "../Sc3Engine.h"
#include "SC_Filesystem.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;
using DirName = SC_Filesystem::DirName;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
	std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());
	if (!ok)
		++failures;
}

static void write(const fs::path &p, const std::string &text)
{
	fs::create_directories(p.parent_path());
	std::ofstream(p) << text;
}

// Waits for a line starting with `tag` in the post output; returns the rest of it.
static std::string awaitLine(Sc3Engine &eng, const std::string &tag, int timeoutMs = 20000)
{
	std::string all;
	const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
	while (std::chrono::steady_clock::now() < end) {
		all += eng.takePost();
		const size_t at = all.find(tag);
		if (at != std::string::npos) {
			const size_t nl = all.find('\n', at);
			if (nl != std::string::npos)
				return all.substr(at + tag.size(), nl - at - tag.size());
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	std::fprintf(stderr, "timeout waiting for '%s'; post was:\n%s\nerror was:\n%s\n", tag.c_str(),
	             all.c_str(), eng.takeError().c_str());
	return "<timeout>";
}

int main(int argc, char **argv)
{
	if (argc != 3 || (std::string(argv[2]) != "ok" && std::string(argv[2]) != "broken")) {
		std::fprintf(stderr, "usage: %s <bundle Resources dir> ok|broken\n", argv[0]);
		return 2;
	}
	const fs::path res = fs::canonical(argv[1]);
	const bool broken = std::string(argv[2]) == "broken";

	char tmpl[] = "/tmp/sc3_quarks_test.XXXXXX";
	if (!mkdtemp(tmpl)) {
		std::perror("mkdtemp");
		return 2;
	}
	const fs::path tmp = fs::canonical(tmpl);
	const fs::path config = tmp / "config", support = tmp / "support";
	const fs::path userExt = support / "Extensions", sysExt = tmp / "system-extensions";
	const fs::path quark = support / "downloaded-quarks" / "SC3TestQuark";
	fs::create_directories(sysExt);

	write(userExt / "SC3TestExt.sc",
	      "SC3TestExt { *hello { ^\"ext-ok\" } *viaQuark { ^SC3TestQuark.sum } }\n");
	write(quark / "SC3TestQuark.quark", "(name: \"SC3TestQuark\", version: \"0.1\")\n");
	write(quark / "Classes" / "SC3TestQuark.sc", "SC3TestQuark { *sum { ^[1, 2, 3].sum } }\n");
	if (broken)
		write(quark / "Classes" / "Broken.sc", "SC3BrokenQuark { *f { ^( } }\n");
	write(config / "sclang_conf.yaml",
	      "includePaths:\n  - " + quark.string() + "\nexcludePaths: []\npostInlineWarnings: false\n");

	// Before open(): getDirectory() caches these, and the boot reads them.
	SC_Filesystem &sfs = SC_Filesystem::instance();
	sfs.setDirectory(DirName::UserAppSupport, support);
	sfs.setDirectory(DirName::UserExtension, userExt);
	sfs.setDirectory(DirName::SystemExtension, sysExt);

	// As sc3_engine_config() without SC3_APP_PATH. Plugins: stock only, so no
	// user .scx from the real ~/Library is loaded.
	Sc3EngineConfig cfg;
	cfg.classLibraryDir = res.string();
	cfg.compatDir = (res / "SCMaxCompat").string();
	cfg.ugenPluginsPath = (res / "plugins").string();
	cfg.userConfigDir = config.string();

	Sc3Engine &eng = Sc3Engine::instance();
	check(eng.open(cfg), "open");
	const bool ready = eng.waitReady(120000);
	const Sc3EngineStatus st = eng.status();
	check(ready && !st.failed, "boot (error: " + st.error + ")");
	check(st.classLibraryDir == res.string(), "class library is the bundle's");

	if (!broken) {
		check(!st.degraded, "not degraded (" + st.degradeReport + ")");
		eng.evalSilent("(\"SC3T \" ++ [SC3TestExt.hello, SC3TestQuark.sum, SC3TestExt.viaQuark, "
		               "Quarks.isInstalled(\"SC3TestQuark\"), Platform.resourceDir, "
		               "Platform.userExtensionDir].join(\"|\")).postln");
		const std::string want = "ext-ok|6|6|true|" + res.string() + "|" + userExt.string();
		const std::string got = awaitLine(eng, "SC3T ");
		check(got == want, "user classes under the bundled library\n  want " + want + "\n  got  " + got);
	} else {
		check(st.degraded, "degraded");
		check(st.degradeReport.find("exclude (includePaths): " + quark.string()) != std::string::npos,
		      "report names the Quark");
		check(st.degradeReport.find("exclude (default dir): " + userExt.string()) != std::string::npos,
		      "report names the user Extensions dir");
		eng.evalSilent("(\"SC3T \" ++ [\\SC3TestExt.asClass.isNil, \\SC3TestQuark.asClass.isNil, "
		               "\\SCMaxCompatState.asClass.notNil, Platform.resourceDir].join(\"|\")).postln");
		const std::string want = "true|true|true|" + res.string();
		const std::string got = awaitLine(eng, "SC3T ");
		check(got == want, "stock + SCMaxCompat after the fallback\n  want " + want + "\n  got  " + got);
	}

	eng.release();
	fs::remove_all(tmp);
	std::printf("%s: %d failure(s)\n", argv[2], failures);
	return failures ? 1 : 0;
}
