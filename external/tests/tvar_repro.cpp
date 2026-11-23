// SPDX-License-Identifier: GPL-3.0-only
// tvar_repro.cpp — diagnostic for: evaluates a list of .scd files one by
// one in the embedded engine and reports whether each eval RETURNED or left the
// sclang VM stuck (the job leaves the queue BEFORE it runs, so `queuedJobs() == 0`
// does not mean "done"; a canary eval is enqueued after each one instead).
//
// Usage: ./build/tvar_repro a.scd b.scd ... [--wait=ms] [--rss=MB]
//   --rss (default 2500) kills the run if the process passes that RSS: an eval
//   that loops inside the interpreter allocates ~2 GB/s and would take Max down.
//
// Known trigger: `t = [1,2,3]` then `1.nextTimeOnGrid(t)` —
// SimpleNumber:nextTimeOnGrid(clock) and SequenceableCollection:nextTimeOnGrid
// call each other forever.
#include "../Sc3Engine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

static bool readFile(const char *path, std::string &out)
{
	FILE *f = std::fopen(path, "rb");
	if (!f)
		return false;
	char buf[8192];
	size_t n;
	while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
		out.append(buf, n);
	std::fclose(f);
	return true;
}

int main(int argc, char **argv)
{
	Sc3EngineConfig cfg;
	const char *port = std::getenv("SC3_PORT");
	cfg.port = port ? std::atoi(port) : 0;
	cfg.compatDir = std::getenv("SC3_COMPAT_DIR") ? std::getenv("SC3_COMPAT_DIR")
	                                              : "../phase1/compat/SCMaxCompat";
	Sc3Engine &eng = Sc3Engine::instance();
	long waitMs = 10000;
	std::vector<const char *> files;
	for (int i = 1; i < argc; ++i) {
		if (std::strncmp(argv[i], "--wait=", 7) == 0)
			waitMs = std::atol(argv[i] + 7);
		else
			files.push_back(argv[i]);
	}
	std::printf("boot...\n");
	std::fflush(stdout);
	if (!eng.open(cfg) || !eng.waitReady(120000)) {
		std::printf("[FAIL] boot: %s\n", eng.status().error.c_str());
		return 1;
	}
	std::printf("[info] ready sr=%.0f blockLen=%ld\n", eng.status().sampleRate,
	            eng.blockLength());
	std::fflush(stdout);
	const long bl = eng.blockLength();
	std::vector<float> L((size_t)bl), R((size_t)bl);
	auto pump = [&](long ms) {
		const long steps = ms / 10 + 1;
		for (long i = 0; i < steps; ++i) {
			eng.renderBlock(L.data(), R.data());
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	};
	pump(300);

	for (size_t f = 0; f < files.size(); ++f) {
		std::string src;
		if (!readFile(files[f], src)) {
			std::printf("[FAIL] cannot read %s\n", files[f]);
			return 1;
		}
		std::printf("\n===== eval %zu: %s (%zu bytes)\n", f + 1, files[f], src.size());
		std::fflush(stdout);
		eng.evalWithSetup("topEnvironment.put(\\sc3out, 0); topEnvironment.put(\\sc3n, 2);",
		                  src);
		/* Canary: the job is popped from the queue BEFORE it runs, so queue == 0
		   does not mean "done". Enqueue a trivial eval and wait for THAT to
		   drain: if it does not, the previous eval is stuck inside the worker. */
		pump(300);
		eng.eval("~sc3_canary = (~sc3_canary ? 0) + 1;");
		long waited = 0;
		while (eng.queuedJobs() > 0 && waited < waitMs) {
			pump(50);
			waited += 50;
			if ((waited % 2000) == 0) {
				std::printf("  ... canary still queued after %ld ms (queued=%ld)\n", waited,
				            eng.queuedJobs());
				std::fflush(stdout);
			}
		}
		const bool drained = eng.queuedJobs() == 0;
		std::printf("  eval %zu %s after %ld ms (engine busy=%ldms stuck=%s)\n", f + 1,
		            drained ? "FINISHED (canary ran)" : "**STUCK**", waited,
		            eng.evalBusyMs(), eng.evalStuck() ? "YES" : "no");
		std::string p = eng.takePost(), e = eng.takeError();
		std::printf("  post(%zu)=[%s]\n", p.size(),
		            p.size() > 400 ? p.substr(p.size() - 400).c_str() : p.c_str());
		std::printf("  err(%zu)=[%s]\n", e.size(), e.c_str());
		std::fflush(stdout);
		pump(300);
		if (!drained)
			break;
	}
	std::printf("\n[done] queued=%ld busy=%ldms stuck=%s units=%ld\n", eng.queuedJobs(),
	            eng.evalBusyMs(), eng.evalStuck() ? "YES" : "no", eng.numUnits());
	std::fflush(stdout);
	/* with sclang stuck, release() would join a thread that never returns
	   and this diagnostic would hang forever. The interpreter dies with the
	   process, so leaving without tearing the World down is the right thing here. */
	if (eng.evalStuck() || eng.evalBusyMs() > 0) {
		std::printf("[done] sclang is still inside an eval: exiting without teardown\n");
		std::fflush(stdout);
		std::_Exit(0);
	}
	return 0;
}
