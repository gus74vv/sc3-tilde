// SPDX-License-Identifier: GPL-3.0-only
// input_test.cpp — headless gate of INPUT audio (SoundIn / In.ar).
//
// Verifies without Max the same thing `sc3~ @ins N` does in a patch:
//
//   [adc~] ──► inlet 0/1 of sc3~ ──► pushInput() ──► INPUT buses of the World
//                                                        │
//                                          SoundIn.ar(0) / In.ar(NumOutputBuses)
//
// Facts verified in SC 3.14.1 (do not re-investigate):
//   · inputBuses = mAudioBus + mNumOutputs * mBufLength; input bus i lives at
//     inputBuses + i * mBufLength (that is, AFTER the outputs).
//   · SoundIn.ar(bus) == In.ar(NumOutputBuses + bus, 1).
//   · In_next_a only copies if mAudioBusTouched[bus] == mBufCounter ⇒ the bus
//     has to be marked on every block (otherwise: silence).
//
// Checks: exact channel→inlet mapping (no block shift), silence without a
// producer, rejection of a second producer, underrun filled with zeros and
// release of the input.
//
// Usage: ./build/input_test [port]      (SC3_SR=44100 to force another sr)
// Port: default 0 = AUTO (the kernel picks it) ⇒ runs with Max / the IDE open.

#include "../Sc3Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

static bool gAllOk = true;
static bool check(bool cond, const std::string &what)
{
	gAllOk = gAllOk && cond;
	std::printf("[%s] %s\n", cond ? "OK " : "FAIL", what.c_str());
	return cond;
}

int main(int argc, char **argv)
{
	Sc3EngineConfig cfg;
	cfg.port = (argc > 1) ? std::atoi(argv[1]) : 0; // 4.x: 0 = auto
	const char *cldir = std::getenv("SC3_CL_DIR");
	cfg.classLibraryDir = cldir ? cldir : "";
	const char *compat = std::getenv("SC3_COMPAT_DIR");
	cfg.compatDir = compat ? compat : "../phase1/compat/SCMaxCompat";
	const char *srEnv = std::getenv("SC3_SR");
	if (srEnv && *srEnv)
		cfg.preferredSampleRate = std::atof(srEnv);
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;

	Sc3Engine &eng = Sc3Engine::instance();
	if (!eng.open(cfg)) {
		Sc3EngineStatus st = eng.status();
		std::printf("[FAIL] open: %s\n", st.error.c_str());
		eng.release();
		return 1;
	}
	if (!eng.waitReady(120000)) {
		Sc3EngineStatus st = eng.status();
		std::printf("[FAIL] boot: %s\n", st.error.c_str());
		eng.release();
		return 1;
	}
	Sc3EngineStatus st = eng.status();
	const long bufLength = eng.blockLength();
	std::printf("[info] boot %ld ms — sr=%.0f blockLen=%ld — buses in=%ld out=%ld\n",
	            st.bootMs, st.sampleRate, bufLength, st.numInputBusChannels,
	            st.numOutputBusChannels);

	check(st.numInputBusChannels >= 2 && st.numOutputBusChannels >= 1,
	      std::string("s.options has input buses (in=") +
	          std::to_string(st.numInputBusChannels) + " out=" +
	          std::to_string(st.numOutputBusChannels) + ")");
	check(eng.inputChannels() == (int)st.numInputBusChannels,
	      std::string("engine.inputChannels() = ") + std::to_string(eng.inputChannels()) +
	          " (= mNumInputs of the World)");

	const long n = bufLength;
	std::vector<double> outL((size_t)n), outR((size_t)n);
	std::vector<float> fL((size_t)n), fR((size_t)n);
	auto pump = [&](int blocks) {
		for (int i = 0; i < blocks; ++i)
			eng.renderBlock(fL.data(), fR.data());
	};

	// --- 1. SynthDef that forwards the input (still with no producer)
	// -------- SoundIn.ar(0) → channel 0, SoundIn.ar(1) → channel 1 (they come
	// out on bus 0/1).
	eng.eval("SynthDef(\\inpass, { Out.ar(0, [SoundIn.ar(0), SoundIn.ar(1)]) }).add;");
	pump(25);
	const long unitsBefore = eng.numUnits();
	eng.evalSilent("x = Synth(\\inpass);");
	bool unitsOk = false;
	for (int i = 0; i < 1000 && !unitsOk; ++i) {
		pump(1);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		unitsOk = eng.numUnits() > unitsBefore;
	}
	check(unitsOk, std::string("Synth de prueba creado (units=") +
	                   std::to_string(eng.numUnits()) + ")");

	// --- 2. no producer (equivalent to DSP off / no @ins) --------------------
	// With no claim, the input buses are NOT touched ⇒ In/SoundIn return exact
	// silence (touched != mBufCounter), like a server with no input device.
	{
		Sc3AudioCursor c;
		eng.resetCursor(c);
		double *outs[2] = {outL.data(), outR.data()};
		bool allZero = true;
		for (int b = 0; b < 8; ++b) {
			eng.pull(c, 0, 2, outs, n);
			for (long i = 0; i < n; ++i)
				if (outL[i] != 0.0 || outR[i] != 0.0)
					allZero = false;
		}
		check(allZero, "with no input producer the World delivers silence (muted SoundIn)");
	}

	// --- 3. single claim -----------------------------------------------------
	char ownerA = 'A';
	char ownerB = 'B';
	check(eng.claimInput(&ownerA, 2), "claimInput(A, 2) ⇒ this instance is the producer");
	check(!eng.claimInput(&ownerB, 2), "a second producer is rejected (single owner)");
	check(eng.inputPushed() == 0, "the ring starts with no pushed samples");

	// --- 4. exact inlet → SoundIn mapping, without shift --------------------
	// Every pushed sample carries its absolute index (ch0) and a constant (ch1):
	// that detects any block shift and any channel crossing.
	const int ticks = 64;
	const long prefillBlocks = 2; // 3.8: prefill = 2 World blocks
	std::vector<double> in0((size_t)n), in1((size_t)n);
	std::vector<double> recL, recR;
	recL.reserve((size_t)(ticks * n));
	recR.reserve((size_t)(ticks * n));
	Sc3AudioCursor cur;
	eng.resetCursor(cur);
	for (int t = 0; t < ticks; ++t) {
		for (long i = 0; i < n; ++i) {
			in0[(size_t)i] = (double)(t * n + i) / 100000.0;
			in1[(size_t)i] = 0.25;
		}
		double *ins[2] = {in0.data(), in1.data()};
		if (!eng.pushInput(&ownerA, ins, 2, n))
			std::printf("[WARN] pushInput rejected at tick %d\n", t);
		double *outs[2] = {outL.data(), outR.data()};
		if (!eng.pull(cur, 0, 2, outs, n))
			std::printf("[WARN] pull rejected at tick %d\n", t);
		recL.insert(recL.end(), outL.begin(), outL.end());
		recR.insert(recR.end(), outR.begin(), outR.end());
	}

	check(eng.inputPushed() == (uint64_t)(ticks * n),
	      std::string("pushed ") + std::to_string(eng.inputPushed()) + " samples");

	// The prefill is 2 blocks of silence: out[t] == in[t - 2].
	long mismatches = 0;
	long firstBad = -1;
	for (int t = (int)prefillBlocks; t < ticks; ++t) {
		const size_t src = (size_t)(t - prefillBlocks) * (size_t)n;
		const size_t dst = (size_t)t * (size_t)n;
		for (long i = 0; i < n; ++i) {
			const double want = (double)((size_t)src + (size_t)i) / 100000.0;
			if (std::fabs(recL[dst + (size_t)i] - want) > 1e-6) {
				if (firstBad < 0)
					firstBad = (long)(dst + (size_t)i);
				++mismatches;
			}
		}
	}
	check(mismatches == 0,
	      std::string("SoundIn.ar(0) returns EXACTLY what inlet 0 pushed ") +
	          std::string(mismatches ? "(first mismatch at sample " +
	                                       std::to_string(firstBad) + ", " +
	                                       std::to_string(mismatches) + " de " +
	                                       std::to_string((ticks - prefillBlocks) * n) + ")"
	                                 : std::string("(no block shift)")));

	// Path latency = prefill (2 blocks) and nothing else.
	{
		const size_t at = (size_t)prefillBlocks * (size_t)n;
		check(std::fabs(recL[at] - 0.0) < 1e-9 && std::fabs(recL[at - 1]) < 1e-9,
		      "the first 2 input blocks are silence (prefill = latency)");
	}

	long ch1Bad = 0;
	for (size_t i = (size_t)prefillBlocks * (size_t)n; i < recR.size(); ++i)
		if (std::fabs(recR[i] - 0.25) > 1e-6)
			++ch1Bad;
	check(ch1Bad == 0, std::string("SoundIn.ar(1) returns the inlet 1 constant (") +
	                       std::to_string(ch1Bad) + " samples out of range)");
	check(eng.inputUnderruns() == 0 && eng.inputOverruns() == 0 && eng.inputSkipped() == 0,
	      std::string("ring with no underrun/overrun/skipped (") +
	          std::to_string(eng.inputUnderruns()) + "/" +
	          std::to_string(eng.inputOverruns()) + "/" +
	          std::to_string(eng.inputSkipped()) + ")");

	// --- 5. underrun: without a push, the block comes out silent
	// --------------
	{
		const long under0 = eng.inputUnderruns();
		double *outs[2] = {outL.data(), outR.data()};
		// The 2 blocks left in the ring (prefill) still come out with old
		// data; from there on there is nothing ⇒ exact silence + underrun.
		double peakLast = 0.0;
		for (int b = 0; b < 8; ++b) {
			eng.pull(cur, 0, 2, outs, n);
			peakLast = 0.0;
			for (long i = 0; i < n; ++i)
				peakLast = std::max(peakLast, std::fabs(outL[i]));
		}
		check(eng.inputUnderruns() > under0 && peakLast == 0.0,
		      std::string("without a push the World emits silence and counts underruns (") +
		          std::to_string(eng.inputUnderruns()) + ")");
	}

	// --- 6. signal vector that is NOT a multiple of the World block ----------
	// In Max the signal vector can be 100, 512, 1024… and the World block is 64:
	// the ring absorbs the mismatch ⇒ the stream neither loses nor duplicates
	// samples (on every tick ceil(vec/64) blocks are consumed and the level
	// oscillates ±1 block).
	check(eng.claimInput(&ownerA, 2), "re-claim for the vector-100 test");
	{
		const long vec = 100;
		std::vector<double> p0((size_t)vec), p1((size_t)vec);
		std::vector<double> oL((size_t)vec), oR((size_t)vec);
		std::vector<double> run;
		long gi = 0;
		Sc3AudioCursor c2;
		eng.resetCursor(c2);
		for (int t = 0; t < 200; ++t) {
			for (long i = 0; i < vec; ++i) {
				p0[(size_t)i] = (double)gi++ / 100000.0;
				p1[(size_t)i] = 0.25;
			}
			double *ins2[2] = {p0.data(), p1.data()};
			eng.pushInput(&ownerA, ins2, 2, vec);
			double *outs2[2] = {oL.data(), oR.data()};
			eng.pull(c2, 0, 2, outs2, vec);
			run.insert(run.end(), oL.begin(), oL.end());
		}
		long start = -1;
		for (size_t i = 0; i + 1 < run.size(); ++i)
			if (run[i + 1] - run[i] > 0.5e-5) {
				start = (long)i;
				break;
			}
		long badSteps = 0;
		for (size_t i = (size_t)std::max<long>(start, 0) + 1; i < run.size(); ++i)
			if (std::fabs((run[i] - run[i - 1]) - 1e-5) > 1e-7)
				++badSteps;
		check(start >= 0 && badSteps == 0 && eng.inputUnderruns() == 0 &&
		          eng.inputOverruns() == 0,
		      std::string("vector of 100 with a block of ") + std::to_string(n) +
		          ": continuous stream, no gaps or duplicates (" +
		          std::to_string(badSteps) + " odd steps, underruns " +
		          std::to_string(eng.inputUnderruns()) + ", overruns " +
		          std::to_string(eng.inputOverruns()) + ")");
	}

	// --- 7. release: silence comes back and somebody else can claim
	// ----------
	eng.releaseInput(&ownerA);
	check(eng.claimInput(&ownerB, 2), "once the input is released, another sc3~ can claim it");
	eng.releaseInput(&ownerB);

	eng.evalSilent("CmdPeriod.run;");
	eng.release();
	std::printf(gAllOk ? "\nALL CHECKS OK\n" : "\nTHERE ARE FAILURES\n");
	return gAllOk ? 0 : 1;
}
