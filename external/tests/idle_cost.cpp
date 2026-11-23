// SPDX-License-Identifier: GPL-3.0-only
// idle_cost.cpp — diagnostic (NOT part of the gate): what does an attached engine
// with nothing to evaluate cost?
//
// Boots the engine and then measures three phases with getrusage (all threads):
//
//   A. pull-only: N blocks back to back (no sleep) ⇒ cost of the audio path
//   B. paced: one block every `n/sr` seconds for `seconds` ⇒ what Max's audio
//      driver makes us do, at realtime
//   C. sleep-only: the same wall time with NO pull ⇒ background threads
//      (sclang idle, /status watcher, World/plugin threads, main clock)
//
// B - A is the engine's realtime share; C is the "idle" floor.
//
// Usage: ./build/idle_cost [seconds] [port]   (default 10 s, port 0 = AUTO)

#include "../Sc3Engine.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sys/resource.h>
#include <thread>
#include <vector>

static double cpuSeconds()
{
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1e6 +
	       (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec / 1e6;
}

static double wallSeconds(std::chrono::steady_clock::time_point t0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

int main(int argc, char **argv)
{
	const double seconds = (argc > 1) ? std::atof(argv[1]) : 10.0;

	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg;
	cfg.port = (argc > 2) ? std::atoi(argv[2]) : 0;
	cfg.classLibraryDir = ""; // ⇒ the real SC installation
	cfg.compatDir = "../phase1/compat/SCMaxCompat";

	if (!eng.open(cfg) || !eng.waitReady(120000)) {
		std::printf("boot failed: %s\n", eng.status().error.c_str());
		return 1;
	}
	const long n = eng.blockLength() > 0 ? eng.blockLength() : 64;
	const double sr = eng.status().sampleRate > 0.0 ? eng.status().sampleRate : 48000.0;
	const double period = (double)n / sr;
	std::printf("ready: sr=%.0f blockLen=%ld units=%ld rendered=%llu\n", sr, n,
	            eng.status().numUnits, (unsigned long long)eng.renderedSamples());

	std::vector<double> buf((size_t)n * 2);
	double *outs[2] = {buf.data(), buf.data() + n};
	Sc3AudioCursor c;

	// A. pull-only -------------------------------------------------------------
	const int kN = 20000;
	double cpuA = cpuSeconds();
	auto tA = std::chrono::steady_clock::now();
	for (int i = 0; i < kN; ++i)
		eng.pull(c, 0, 2, outs, n);
	const double wallA = wallSeconds(tA);
	cpuA = cpuSeconds() - cpuA;
	std::printf("A. pull-only: %d blocks in %.3f s ⇒ %.1f us/block "
	            "(= %.2f %% of one core at %.0f blocks/s)\n",
	            kN, wallA, 1e6 * wallA / kN, 100.0 * (wallA / kN) / period, 1.0 / period);

	// B. paced ------------------------------------------------------------------
	const double cpu0 = cpuSeconds();
	const auto t0 = std::chrono::steady_clock::now();
	long blocks = 0;
	while (wallSeconds(t0) < seconds) {
		eng.pull(c, 0, 2, outs, n);
		++blocks;
		const double left = (double)blocks * period - wallSeconds(t0);
		std::this_thread::sleep_for(std::chrono::duration<double>(left > 0.0 ? left : period));
	}
	const double wallB = wallSeconds(t0);
	const double cpuB = cpuSeconds() - cpu0;
	std::printf("B. paced: %ld blocks of %ld frames in %.2f s ⇒ %.2f %% of one core "
	            "(engine + sclang + World, audio at realtime)\n",
	            blocks, n, wallB, 100.0 * cpuB / wallB);

	// C. sleep-only (no pull) ---------------------------------------------------
	const double cpu2 = cpuSeconds();
	std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
	const double cpuC = cpuSeconds() - cpu2;
	std::printf("C. sleep-only (no pull): %.2f %% of one core "
	            "(background threads: sclang idle, /status, plugins, main clock)\n",
	            100.0 * cpuC / seconds);

	// D. paced sleep only (same wake-up cadence, no pull) -----------------------
	const double cpu3 = cpuSeconds();
	const auto t3 = std::chrono::steady_clock::now();
	long wakes = 0;
	while (wallSeconds(t3) < seconds) {
		++wakes;
		const double left = (double)wakes * period - wallSeconds(t3);
		std::this_thread::sleep_for(std::chrono::duration<double>(left > 0.0 ? left : period));
	}
	std::printf("D. paced sleep only: %ld wakeups/s ⇒ %.2f %% of one core "
	            "(cost of this test's sleep_for, NOT of the engine)\n",
	            wakes, 100.0 * (cpuSeconds() - cpu3) / wallSeconds(t3));

	// E. the same with the engine torn down (sclang is process-wide and stays) ---
	eng.release();
	std::this_thread::sleep_for(std::chrono::duration<double>(0.5));
	const double cpu4 = cpuSeconds();
	std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
	std::printf("E. sleep-only after close: %.2f %% of one core (what is left: sclang)\n",
	            100.0 * (cpuSeconds() - cpu4) / seconds);

	return 0;
}
