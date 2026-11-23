// SPDX-License-Identifier: GPL-3.0-only
// reopen_test.cpp — repro/gate of the CPU burst after close → open.
//
// Scenario (measured in Max): an instance that has been attached for a while
// (`open`, DSP on, nothing evaluated) is `close`d and opened again. The World is
// torn down and recreated, so the shared stream restarts at sample 0, but the
// instance cursor (`Sc3AudioCursor`) still holds the position of the OLD stream.
// The audio thread then finds itself `pos` samples AHEAD of `rendered`: every host
// block renders the 128-block catch-up cap and still answers "silence", forever.
// Measured as a persistent 30 %+ in Max's CPU meter that `dsp off/on` (which calls
// dsp64 ⇒ resetCursor) cuts in one go.
//
// Usage: ./build/reopen_test [port]   (default port 0 = AUTO)

#include "../Sc3Engine.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static bool gAllOk = true;
static bool check(bool cond, const std::string &what)
{
	gAllOk = gAllOk && cond;
	std::printf("[%s] %s\n", cond ? "OK " : "FAIL", what.c_str());
	return cond;
}

static double msSince(std::chrono::steady_clock::time_point t0)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

int main(int argc, char **argv)
{
	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg;
	cfg.port = (argc > 1) ? std::atoi(argv[1]) : 0;
	cfg.classLibraryDir = ""; // ⇒ the real SC installation
	cfg.compatDir = "../phase1/compat/SCMaxCompat";

	if (!check(eng.open(cfg), "open() accepted")) {
		std::printf("engine: %s\n", eng.status().error.c_str());
		return 1;
	}
	if (!check(eng.waitReady(120000), "boot ready")) {
		std::printf("engine: %s\n", eng.status().error.c_str());
		return 1;
	}

	const long n = eng.blockLength() > 0 ? eng.blockLength() : 64;
	std::vector<double> o((size_t)n), o2((size_t)n);
	double *outs[1] = {o.data()};
	double *outs2[1] = {o2.data()};

	// 1. a session worth of audio: 2000 blocks ⇒ ~2.7 s @48k, enough to leave the
	//    cursor far away from a restarted stream.
	Sc3AudioCursor c;
	long pulled = 0;
	for (int i = 0; i < 2000; ++i)
		if (eng.pull(c, 0, 1, outs, n))
			pulled++;
	check(pulled == 2000, "2000 blocks pulled on the first stream");
	const uint64_t oldPos = c.pos;
	std::printf("     first stream: cursor at %llu samples (%ld blocks)\n",
	            (unsigned long long)oldPos, n > 0 ? (long)(oldPos / (uint64_t)n) : 0L);

	// 2. close → open: the World (and the shared stream) is recreated.
	eng.release();
	if (!check(eng.open(cfg), "open() after close accepted")) {
		std::printf("engine: %s\n", eng.status().error.c_str());
		return 1;
	}
	if (!check(eng.waitReady(120000), "second boot ready")) {
		std::printf("engine: %s\n", eng.status().error.c_str());
		return 1;
	}
	const uint64_t fresh = eng.renderedSamples();
	std::printf("     second stream: rendered=%llu samples (%ld blocks), cursor still at %llu\n",
	            (unsigned long long)fresh, n > 0 ? (long)(fresh / (uint64_t)n) : 0L,
	            (unsigned long long)c.pos);

	// 3. the audio thread comes back with the stale cursor: it must snap to the
	//    new stream instead of rendering a catch-up burst.
	const auto t0 = std::chrono::steady_clock::now();
	uint64_t maxAdvance = 0;
	int okCalls = 0;
	for (int i = 0; i < 10; ++i) {
		const uint64_t before = eng.renderedSamples();
		const bool ok = eng.pull(c, 0, 1, outs, n);
		if (ok)
			okCalls++;
		const uint64_t after = eng.renderedSamples();
		maxAdvance = std::max<uint64_t>(maxAdvance, after - before);
	}
	const double ms = msSince(t0);
	std::printf("     10 pulls with the stale cursor: %d ok, worst advance %llu samples "
	            "(%.0f x the host block), %.1f ms\n",
	            okCalls, (unsigned long long)maxAdvance,
	            n > 0 ? (double)maxAdvance / (double)n : 0.0, ms);

	check(maxAdvance <= (uint64_t)n * 4,
	      "no catch-up burst: one pull advances at most a few blocks of the new stream");
	check(okCalls == 10, "the stale cursor snaps to the new stream on the first pull");

	// 4. after the snap it keeps up: the cursor and the stream move together.
	{
		const uint64_t before = eng.renderedSamples();
		const bool ok = eng.pull(c, 0, 1, outs2, n);
		const uint64_t after = eng.renderedSamples();
		check(ok && after - before <= (uint64_t)n * 2,
		      "steady state after the snap: one block in, one block rendered");
	}

	eng.release();
	std::printf("%s\n", gAllOk ? "reopen_test: all OK" : "reopen_test: FAILURES");
	return gAllOk ? 0 : 1;
}
