// SPDX-License-Identifier: GPL-3.0-only
// loop_test.cpp — stress gate of (step 2.5).
//
// Cycles the whole engine (open → boot → eval → audio → release) N times and
// verifies that every round leaves EVERYTHING closed:
//   · fds     — above all the UDP 57110 socket (a leaked fd per cycle shows here)
//   · threads — the sclang worker and the World ones have to die in release()
//   · RSS     — stable after the first cycle (the 1st one includes the big World
//               allocations, which the allocator may retain)
//
// It alternates the OWNER of the World on every cycle (3.3): `renderBlock` (DSP
// on) and `pumpWhenIdle` (DSP off). It also alternates the host sr (3.2) every
// two cycles, so every boot goes through both sr branches.
//
// Usage:
//   ./build/loop_test              # 5 cycles (default)
//   SC3_CYCLES=10 ./build/loop_test
//   ./build/loop_test 57110 "" 3   # port, ugensPath, cycles
//
// Requires a free port (close Max / the SC IDE).

#include <libproc.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "../Sc3Engine.h"

// Under ASan/TSan the RSS is not comparable: the sanitizers keep in quarantine
// what is freed (the World is ~800 MB per cycle) ⇒ the RSS check is skipped, not
// relaxed (otherwise it would never detect a real leak).
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#    define SC3_UNDER_SAN 1
#elif defined(__has_feature)
#    if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#        define SC3_UNDER_SAN 1
#    endif
#endif

static bool gAllOk = true;

static bool check(bool cond, const std::string &what)
{
	std::printf("[%s] %s\n", cond ? "OK " : "FAIL", what.c_str());
	if (!cond)
		gAllOk = false;
	return cond;
}

struct Stats {
	long fds = -1;
	long threads = -1;
	long rssMB = -1;
};

static Stats procStats()
{
	Stats s;
	struct proc_taskinfo ti {};
	const int pid = (int)getpid();
	if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &ti, sizeof(ti)) == sizeof(ti)) {
		s.threads = ti.pti_threadnum;
		s.rssMB = (long)(ti.pti_resident_size / (1024 * 1024));
	}
	const int bytes = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
	if (bytes > 0) {
		std::vector<struct proc_fdinfo> buf((size_t)(bytes / (int)sizeof(struct proc_fdinfo)) + 8);
		const int got = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, buf.data(),
		                             (int)(buf.size() * sizeof(struct proc_fdinfo)));
		if (got > 0)
			s.fds = got / (int)sizeof(struct proc_fdinfo);
	}
	return s;
}

static std::string fmtStats(const Stats &s)
{
	return "fds=" + std::to_string(s.fds) + " threads=" + std::to_string(s.threads) +
	       " rss=" + std::to_string(s.rssMB) + "MB";
}

int main(int argc, char **argv)
{
	// no buffering: if something crashes, the progress log is not
	// lost
	setvbuf(stdout, nullptr, _IONBF, 0);

	Sc3EngineConfig cfg;
	cfg.port = (argc > 1) ? std::atoi(argv[1]) : 0; // 4.x: 0 = auto
	cfg.ugenPluginsPath = (argc > 2) ? argv[2] : "";
	const char *cldir = std::getenv("SC3_CL_DIR");
	cfg.classLibraryDir = cldir ? cldir : "";
	const char *compat = std::getenv("SC3_COMPAT_DIR");
	cfg.compatDir = compat ? compat : "../phase1/compat/SCMaxCompat";
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;

	int cycles = (argc > 3) ? std::atoi(argv[3]) : 0;
	if (cycles <= 0) {
		const char *env = std::getenv("SC3_CYCLES");
		cycles = env ? std::atoi(env) : 5;
	}
	if (cycles < 2) {
		std::printf("at least 2 cycles are needed (to compare).\n");
		return 2;
	}

	Sc3Engine &eng = Sc3Engine::instance();
	const Stats base = procStats();
	std::printf("[info] baseline: %s\n", fmtStats(base).c_str());

	Stats afterFirst;      // ciclo 1 (incluye las allocations grandes)
	Stats prev;
	long worstBootMs = 0;
	int okCycles = 0;

	for (int i = 0; i < cycles; ++i) {
		const bool useAudio = (i % 2) == 0;         // 3.3: alternates the World owner
		const bool sr441 = ((i / 2) % 2) == 1;      // 3.2: alternates the host sr
		cfg.preferredSampleRate = sr441 ? 44100.0 : 48000.0;
		const char *mode = useAudio ? "renderBlock (DSP on)" : "pumpWhenIdle (DSP off)";

		const auto t0 = std::chrono::steady_clock::now();
		if (!eng.open(cfg)) {
			Sc3EngineStatus st = eng.status();
			check(false, "cycle " + std::to_string(i) + ": open() failed" +
			                 (st.error.empty() ? "" : ": " + st.error));
			break;
		}
		if (!eng.waitReady(120000)) {
			Sc3EngineStatus st = eng.status();
			check(false, "cycle " + std::to_string(i) + ": the boot never reached ready" +
			                 (st.error.empty() ? "" : ": " + st.error));
			eng.release();
			break;
		}
		const long bootMs = (long)std::chrono::duration_cast<std::chrono::milliseconds>(
		                        std::chrono::steady_clock::now() - t0)
		                        .count();
		if (bootMs > worstBootMs)
			worstBootMs = bootMs;

		Sc3EngineStatus st = eng.status();
		const bool srOk = std::lround(st.sampleRate) == std::lround(cfg.preferredSampleRate);

		eng.eval("SynthDef(\\loopchk, { Out.ar(0, SinOsc.ar(220, 0, 0.5)) }).add;"
		         " y = Synth(\\loopchk);");

		std::vector<float> l(4096, 0.0f), r(4096, 0.0f);
		const auto p0 = std::chrono::steady_clock::now();
		long units = 0;
		while (std::chrono::duration_cast<std::chrono::milliseconds>(
		           std::chrono::steady_clock::now() - p0)
		           .count() < 400) {
			if (useAudio)
				eng.renderBlock(l.data(), r.data());
			else
				eng.pumpWhenIdle(4);
			std::this_thread::sleep_for(std::chrono::milliseconds(4));
			units = eng.numUnits();
		}
		eng.eval("y.free;");

		const Stats during = procStats();
		std::printf("[info] cycle %d  %-26s sr=%.0f  boot=%ldms  units=%ld  %s\n", i, mode,
		            st.sampleRate, bootMs, units, fmtStats(during).c_str());

		if (!check(srOk, "cycle " + std::to_string(i) + ": World al sr pedido (" +
		                     std::to_string((long)std::lround(cfg.preferredSampleRate)) + ")"))
			;
		if (!check(units > 0, "cycle " + std::to_string(i) + ": the World processed audio (" + mode +
		                          ", units=" + std::to_string(units) + ")"))
			;
		if (units > 0 && srOk)
			okCycles++;

		eng.release();
		// let the teardown finish (join of the worker, close of the
		// socket)
		for (int k = 0; k < 20; ++k)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		prev = procStats();
		if (i == 0)
			afterFirst = prev;
		std::printf("[info] cycle %d  after release: %s  ready=%s\n", i, fmtStats(prev).c_str(),
		            eng.isReady() ? "SI (mal)" : "no");
		check(!eng.isReady(), "cycle " + std::to_string(i) + ": release() left the engine closed");
	}

	// --- leak invariants -----------------------------------------------------
	// CAREFUL: the "before opening" baseline is no good as an fd/thread
	// reference. Since 2.5 the sclang VM and the .scx live as long as
	// the process (re-initializing them crashes), so the first boot adds its
	// socket and its threads permanently. What matters is that they do NOT GROW
	// per cycle: the state after cycle 1 is compared against the state after the
	// last one.
	check(okCycles == cycles,
	      std::to_string(okCycles) + "/" + std::to_string(cycles) + " valid cycles");
	check(prev.fds >= 0 && prev.fds <= afterFirst.fds,
	      "fds do not grow per cycle: " + std::to_string(afterFirst.fds) + " → " +
	          std::to_string(prev.fds) + " (baseline pre-open " +
	          std::to_string(base.fds) + ", +socket/lang of the 1st boot)");
	check(prev.threads >= 0 && prev.threads <= afterFirst.threads + 2,
	      "threads do not accumulate: " + std::to_string(afterFirst.threads) + " → " +
	          std::to_string(prev.threads) + " (limit +2; pre-open " +
	          std::to_string(base.threads) + ")");
	// The RSS has a high floor (the World pool, ~800 MB with the memSize of
	// s.options) and between cycles the allocator retains what each class
	// library recompilation frees (the VM is recompiled on every open, see
	// What is required is that it does NOT grow without a ceiling:
	// 32 MB per cycle.
#ifdef SC3_UNDER_SAN
	std::printf("[SKIP] RSS: under ASan/TSan the sanitizer keeps in quarantine what was\n"
	            "       freed (not comparable; run the check without a sanitizer)\n");
#else
	const long rssGrowth = prev.rssMB - afterFirst.rssMB;
	const long rssCap = 32L * (cycles - 1);
	check(prev.rssMB > 0 && rssGrowth < rssCap,
	      "RSS does not blow up: " + std::to_string(afterFirst.rssMB) + " → " +
	          std::to_string(prev.rssMB) + "MB (crecimiento " + std::to_string(rssGrowth) +
	          "MB en " + std::to_string(cycles - 1) + " cycles, limit " +
	          std::to_string(rssCap) + "MB)");
#endif
	check(worstBootMs < 30000,
	      "no boot degraded: worst = " + std::to_string(worstBootMs) + " ms");

	std::string err = eng.takeError();
	if (!err.empty())
		std::printf("  err (final): %s\n", err.c_str());

	// sclang shutdown: the VM lives as long as the process, so it has
	// to be shut down before exiting (otherwise the static std::thread of
	// PyrSched stay joinable and ~thread → std::terminate in __cxa_finalize).
	eng.shutdownLang();

	std::printf("== loop_test: %s ==\n", gAllOk ? "OK" : "FAIL");
	return gAllOk ? 0 : 1;
}
