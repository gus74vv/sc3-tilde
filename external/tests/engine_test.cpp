// SPDX-License-Identifier: GPL-3.0-only
// engine_test.cpp — headless gate of `Sc3Engine` without Max.
//
// Same checks as the world test (which is the verified prototype), but
// through the engine API: open() → eval() → renderBlock() → release().
// If this is green, the engine does not depend on Max and 2.3 only has to pump
// perform64 with the ring buffer.
//
// Usage: ./build/engine_test [port] [pluginPaths]
//        SC3_CL_DIR=...     (Resource dir override; default: real installation
//                            /Applications/SuperCollider.app/Contents/Resources)
//        SC3_COMPAT_DIR=... (default: ../the first spike/compat/SCMaxCompat)
//        SC3_DEBUG_OSC=1    (OSC trace + boot info to stderr)
//
// Port: default 0 = AUTO (the kernel picks a free one) ⇒ it can run with the SC
// IDE / scsynth open. A port through argv forces it (failure tests).

#include "../Sc3Engine.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
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

// 16-bit PCM WAV (same layout verified in the first spike: 44-byte header).
static bool writeWav16(const char *path, int sampleRate,
                       const std::vector<std::vector<int16_t>> &channels)
{
	size_t numFrames = channels.empty() ? 0 : channels[0].size();
	int numCh = (int)channels.size();
	uint32_t dataSize = (uint32_t)(numFrames * numCh * 2);
	uint32_t byteRate = (uint32_t)sampleRate * numCh * 2;
	uint16_t blockAlign = (uint16_t)(numCh * 2);

	uint8_t hdr[44] = {0};
	memcpy(hdr + 0, "RIFF", 4);
	auto put32 = [&hdr](int off, uint32_t v) {
		hdr[off] = v & 0xff;
		hdr[off + 1] = (v >> 8) & 0xff;
		hdr[off + 2] = (v >> 16) & 0xff;
		hdr[off + 3] = (v >> 24) & 0xff;
	};
	auto put16 = [&hdr](int off, uint16_t v) {
		hdr[off] = v & 0xff;
		hdr[off + 1] = (v >> 8) & 0xff;
	};
	put32(4, 36 + dataSize);
	memcpy(hdr + 8, "WAVE", 4);
	memcpy(hdr + 12, "fmt ", 4);
	put32(16, 16);
	put16(20, 1); // PCM
	put16(22, (uint16_t)numCh);
	put32(24, (uint32_t)sampleRate);
	put32(28, byteRate);
	put16(32, blockAlign);
	put16(34, 16);
	memcpy(hdr + 36, "data", 4);
	put32(40, dataSize);

	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	fwrite(hdr, 1, 44, f);
	for (size_t i = 0; i < numFrames; ++i)
		for (int c = 0; c < numCh; ++c)
			fwrite(&channels[c][i], 2, 1, f);
	fclose(f);
	return true;
}

int main(int argc, char **argv)
{
	Sc3EngineConfig cfg;
	cfg.port = (argc > 1) ? std::atoi(argv[1]) : 0; // 4.x: 0 = auto
	cfg.ugenPluginsPath = (argc > 2) ? argv[2] : "";
	const char *cldir = std::getenv("SC3_CL_DIR");
	cfg.classLibraryDir = cldir ? cldir : ""; // empty ⇒ real SC installation
	// Issue #1 item 9: with SC3_RESOURCES_DIR the whole test runs against a
	// STAGED payload (Contents/Resources of the bundle: class library, .scx,
	// sounds), which is what the object uses by default. Nothing from the
	// installed SuperCollider is read in that mode.
	const char *resDir = std::getenv("SC3_RESOURCES_DIR");
	if (resDir && *resDir)
		cfg.resourcesDir = resDir;
	const char *compat = std::getenv("SC3_COMPAT_DIR");
	cfg.compatDir = compat                 ? compat
	                : (resDir && *resDir) ? std::string(resDir) + "/SCMaxCompat"
	                                      : "../phase1/compat/SCMaxCompat";
	// 3.2: DSP chain sr. In Max `sys_getsr()` sets it; here it is simulated
	// with SC3_SR to verify that the World is born at that sr (and that the
	// tone stays in tune: the 440 Hz check uses the reported sr).
	const char *srEnv = std::getenv("SC3_SR");
	if (srEnv && *srEnv)
		cfg.preferredSampleRate = std::atof(srEnv);
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;

	Sc3Engine &eng = Sc3Engine::instance();
	// 3.1: open() is asynchronous (it does not block the Max main
	// thread). In a headless binary we wait explicitly and measure how
	// long it took.
	const auto bootT0 = std::chrono::steady_clock::now();
	if (!eng.open(cfg)) {
		Sc3EngineStatus st = eng.status();
		std::printf("[FAIL] open: %s\n", st.error.c_str());
		if (!st.degradeReport.empty())
			std::printf("       degrade: %s\n", st.degradeReport.c_str());
		std::string err = eng.takeError();
		if (!err.empty())
			std::printf("       err: %s\n", err.c_str());
		eng.release();
		return 1;
	}

	const auto openedUs = (long)std::chrono::duration_cast<std::chrono::microseconds>(
	                          std::chrono::steady_clock::now() - bootT0)
	                          .count();
	check(openedUs < 200000, std::string("open() async: returned in ") +
	                             std::to_string(openedUs / 1000.0) + " ms (< 200 ms)");
	if (!eng.waitReady(120000)) {
		Sc3EngineStatus fst = eng.status();
		std::printf("[FAIL] boot: %s\n", fst.error.c_str());
		std::string e = eng.takeError();
		if (!e.empty())
			std::printf("       err: %s\n", e.c_str());
		eng.release();
		return 1;
	}

	Sc3EngineStatus st = eng.status();
	std::printf("[info] boot in %ld ms (async from open())\n", st.bootMs);
	std::printf("[info] s.options: maxNodes=%ld memSize=%ldKB numWireBufs=%ld numRGens=%ld\n"
	            "       numBuffers=%ld numAudioBusChannels=%ld (in=%ld out=%ld)\n"
	            "       blockSize=%ld maxLogins=%ld sampleRate=%.0f%s\n",
	            st.maxNodes, st.memSizeKB, st.numWireBufs, st.numRGens, st.numBuffers,
	            st.numAudioBusChannels, st.numInputBusChannels, st.numOutputBusChannels,
	            st.blockSize, st.maxLogins, st.sampleRate, st.sampleRateSet ? "" : " (default)");
	if (st.degraded) {
		std::printf("[WARN] class library degradada a stock:\n%s", st.degradeReport.c_str());
		std::string err = eng.takeError();
		if (!err.empty())
			std::printf("       startup errors after degrading:\n%s", err.c_str());
	}

	// --- checks de bootstrap --------------------------------------------------
	check(st.serverRunning, "server running (s.serverRunning)");
	const long expectBuses = std::max(st.numAudioBusChannels,
	                                  st.numInputBusChannels + st.numOutputBusChannels);
	check(st.bufLength == st.blockSize && st.worldBuses == (long)expectBuses &&
	          st.worldSndBufs == st.numBuffers && st.worldRGens == st.numRGens,
	      std::string("World created with the user's s.options (bufLength ") +
	          std::to_string(st.bufLength) + ", buses " + std::to_string(st.worldBuses) +
	          ", sndBufs " + std::to_string(st.worldSndBufs) + ", rgens " +
	          std::to_string(st.worldRGens) + ")");
	check(eng.blockLength() > 0, std::string("engine ready, blockLength = ") +
	                                 std::to_string(eng.blockLength()));

	// --- 4.x: AUTO port (the World and sclang agree on the same one)
	// ------------- With `port = 0` the World binds an ephemeral port and the
	// `*initClass` of SCMaxCompat/9_SC3ServerPort.sc points `Server.default.addr`
	// there (the real port travels through SC3_WORLD_PORT). If sclang stayed on
	// 57110 the boot would fail in the fail-fast of `so.port != cfg.port`, so
	// ready already proves it; here the value sclang sees is also compared.
	check(st.port > 0, std::string("4.x: World on port ") + std::to_string(st.port) +
	                       " (auto, not fixed 57110)");
	{
		auto langPort = [&]() -> long {
			eng.evalSilent("\"SC3PORT \".post; s.addr.port.postln;");
			std::string post;
			for (int i = 0; i < 400; ++i) {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				post += eng.takePost();
				const size_t at = post.find("SC3PORT ");
				if (at != std::string::npos) {
					const long v = std::atol(post.c_str() + at + 8);
					if (v > 0)
						return v;
				}
			}
			return 0;
		};
		const long lp = langPort();
		check(lp == st.port, std::string("4.x: sclang apunta al World (s.addr.port ") +
		                         std::to_string(lp) + " == " + std::to_string(st.port) + ")");
	}
	if (cfg.preferredSampleRate > 0.0)
		check(std::lround(st.sampleRate) == std::lround(cfg.preferredSampleRate) &&
		          st.sampleRateFromHost,
		      std::string("3.2: World created at the project sr (") +
		          std::to_string((long)std::lround(cfg.preferredSampleRate)) + " Hz, sr=" +
		          std::to_string((long)std::lround(st.sampleRate)) + ")");

	// --- /status: scsynth's reply without an audio driver ----------------------
	// The World runs with mRealTime = false ⇒ hw->mAudioDriver is nullptr and
	// scsynth's AudioStatusCmd dereferences it (SC_SequencedCommand.cpp:1162), so
	// EVERY /status used to SIGSEGV the host. The engine answers it itself and the
	// adoption block sends one, which is how sclang learns the sample rate:
	// without that, `s.sampleRate` is nil and anything counting frames (SuperDirt's
	// `DirtOrbit:init` does `8 / server.sampleRate`) breaks.
	{
		const int bl = (int)eng.blockLength();
		std::vector<float> l((size_t)bl), r((size_t)bl);
		auto postOf = [&](const std::string &code, const std::string &tag) -> std::string {
			eng.evalSilent(code);
			std::string post;
			for (int i = 0; i < 200; ++i) {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				eng.renderBlock(l.data(), r.data()); // serviceOsc → the reply leaves
				post += eng.takePost();
				if (post.find(tag) != std::string::npos)
					break;
			}
			return post;
		};
		auto taggedNumber = [](const std::string &post, const std::string &tag) -> double {
			const size_t at = post.find(tag);
			if (at == std::string::npos)
				return -1.0;
			return std::atof(post.c_str() + at + tag.size());
		};
		// (a) the reply reaches ServerStatusWatcher (registered in the adoption block)
		const double langSr =
		    taggedNumber(postOf("\"SC3SR \".post; s.sampleRate.postln;", "SC3SR "), "SC3SR ");
		check(std::lround(langSr) == std::lround(st.sampleRate) && langSr > 0.0,
		      std::string("status: s.sampleRate from /status.reply = ") +
		          std::to_string((long)std::lround(langSr)) + " Hz (World " +
		          std::to_string((long)std::lround(st.sampleRate)) + " Hz)");
		// (b) an explicit /status (s.status) is answered, not a null deref, and it
		//     carries the World's live counters.
		const std::string p = postOf("s.sendMsg(\\status);\n"
		                             "\"SC3CNT \".post; s.numGroups.postln;",
		                             "SC3CNT ");
		const double groups = taggedNumber(p, "SC3CNT ");
		check(groups > 0.0, std::string("status: /status.reply counts reach sclang (numGroups=") +
		                        std::to_string((long)groups) + ")");
	}

	const int bufLength = (int)eng.blockLength();
	const int sampleRate = (int)std::lround(st.sampleRate);
	std::vector<float> outL((size_t)bufLength), outR((size_t)bufLength);

	auto pumpBlocks = [&](int blocks, int sleepMs) {
		for (int b = 0; b < blocks; ++b) {
			eng.renderBlock(outL.data(), outR.data());
			if (sleepMs > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
		}
	};

	// --- SynthDef + Synth ------------------------------------------------------
	eng.eval("SynthDef(\\sine440, { Out.ar(0, SinOsc.ar(440, 0, 0.8)) }).add;");
	pumpBlocks(25, 2);

	const long unitsBefore = eng.numUnits();
	eng.eval("x = Synth(\\sine440);");
	bool unitsOk = false;
	for (int i = 0; i < 1000 && !unitsOk; ++i) {
		pumpBlocks(1, 2);
		unitsOk = eng.numUnits() > unitsBefore;
	}
	check(unitsOk, std::string("units > 0 (before ") + std::to_string(unitsBefore) +
	                   ", ahora " + std::to_string(eng.numUnits()) + ")");

	// --- captura de 1.5 s ------------------------------------------------------
	const int durationMs = 1500;
	const long totalFrames = (long)sampleRate * durationMs / 1000;
	const int blocks = (int)(totalFrames / bufLength);
	std::vector<std::vector<float>> raw(2);
	raw[0].reserve(totalFrames);
	raw[1].reserve(totalFrames);
	for (int b = 0; b < blocks; ++b) {
		eng.renderBlock(outL.data(), outR.data());
		raw[0].insert(raw[0].end(), outL.begin(), outL.end());
		raw[1].insert(raw[1].end(), outR.begin(), outR.end());
	}

	double rms = 0.0;
	for (float v : raw[0])
		rms += v * v;
	rms = std::sqrt(rms / (double)raw[0].size());
	check(rms > 0.01, std::string("RMS > 0.01 (medido ") + std::to_string(rms) + ")");

	double dc = 0.0, peak = 0.0;
	for (float v : raw[0]) {
		dc += v;
		peak = std::max(peak, (double)std::fabs(v));
	}
	dc /= (double)raw[0].size();
	check(std::fabs(dc) < 0.01 && peak > 0.7 && peak < 0.9,
	      std::string("forma de onda (DC ") + std::to_string(dc) + ", peak " +
	          std::to_string(peak) + " ~ 0.8)");

	int crossings = 0;
	for (size_t i = 1; i < raw[0].size(); ++i)
		if (raw[0][i - 1] < 0 && raw[0][i] >= 0)
			crossings++;
	double estFreq = crossings / (durationMs / 1000.0);
	check(std::fabs(estFreq - 440.0) < 15.0,
	      std::string("tono ~ 440 Hz (estimado ") + std::to_string((int)estFreq) + " Hz)");

	std::vector<std::vector<int16_t>> pcm(2);
	for (int c = 0; c < 2; ++c) {
		pcm[c].resize(raw[c].size());
		for (size_t i = 0; i < raw[c].size(); ++i)
			pcm[c][i] = (int16_t)(std::max(-1.0f, std::min(1.0f, raw[c][i])) * 32767.0);
	}
	bool wrote = writeWav16("build/engine_test_out.wav", sampleRate, pcm);
	check(wrote, "WAV escrito (build/engine_test_out.wav)");

	// --- 3.9: scsynth text: `.poll` → `post` (black), failure → `error` --------
	// scsynth does not separate print from error (both go through `scprintf`), so
	// the engine hook classifies by line prefix. A `.poll` is normal output: it has
	// to arrive on the `post` outlet and NOT pollute the `error` outlet (which in
	// Max comes out red and looks as if the code had failed).
	{
		eng.takePost();
		eng.takeError();
		const long unitsNoPoll = eng.numUnits();
		eng.evalSilent("y = { SinOsc.ar(300).poll(0.01) }.play;");
		std::string p, e;
		for (int i = 0; i < 200 && p.find("UGen(") == std::string::npos; ++i) {
			pumpBlocks(1, 5);
			p += eng.takePost();
			e += eng.takeError();
		}
		check(p.find("UGen(") != std::string::npos,
		      "3.9: the server `.poll` goes out through `post` (post='" + p + "')");
		check(e.empty(), "3.9: the `.poll` does not dirty the `error` outlet (err='" + e + "')");
		eng.evalSilent("y.free;");
		for (int i = 0; i < 50 && eng.numUnits() != unitsNoPoll; ++i)
			pumpBlocks(1, 2);
		check(eng.numUnits() == unitsNoPoll,
		      std::string("3.9: the poll synth was freed (units=") +
		          std::to_string(eng.numUnits()) + ")");
		eng.takePost();
		eng.takeError();
	}

	// --- 3.6: bundles with a future timetag ---------------------------
	// NodeProxy:free sends the /n_free of the group at fadeTime + server.latency:
	// if the engine executed it instantly, any fade would cut hard
	// (Ndef(\rthm).clear(10)). Here: /n_free scheduled at 2.5 s ⇒ the synth has
	// to be alive at 300 ms and dead at 3 s.
	//
	// The timetag has to be generous: a bundle carries a *wall-clock* deadline
	// (sclang converts `secondsFromNow` against `Main.elapsedTime`) while the World
	// advances by rendered blocks, and the earlier tests pump faster than real time,
	// so the audio clock can sit ahead of the wall clock. With a short timetag the
	// bundle is already due on a slow machine and runs instantly — measured on the
	// CI runner: `units=0` at the 200 ms check with a 500 ms timetag, green locally.
	{
		auto pumpForMs = [&](int ms, bool sleep) {
			const int blockMs =
			    std::max(1, (int)std::lround(1000.0 * (double)bufLength / (double)sampleRate));
			const int n = std::max(1, ms / blockMs);
			for (int b = 0; b < n; ++b) {
				eng.renderBlock(outL.data(), outR.data());
				if (sleep)
					std::this_thread::sleep_for(std::chrono::milliseconds(blockMs));
			}
		};
		eng.evalSilent("s.sendBundle(2.5, [11, x.nodeID]);"); // /n_free ~2.5 s
		pumpForMs(300, true);
		const long unitsEarly = eng.numUnits();
		pumpForMs(2600, true);
		const long unitsLate = eng.numUnits();
		check(unitsEarly > 0,
		      std::string("bundle with a future timetag is NOT run instantly (units=") +
		          std::to_string(unitsEarly) + ")");
		check(unitsLate == unitsBefore,
		      std::string("the timetagged bundle runs when due (units=") +
		          std::to_string(unitsLate) + ", baseline " + std::to_string(unitsBefore) + ")");
	}

	// --- 3.3: pump with DSP off (WITHOUT renderBlock)
	// --------------------------- Without DSP, `perform64` does not run: the
	// World only advances if the main clock calls pumpWhenIdle() (= what sc3~
	// does when sys_getdspobjdspstate says this instance is not in the audio
	// chain). Here that case is simulated — no renderBlock — and what depends on
	// the World is verified: /s_new, /sync (the reply goes back to sclang) and
	// the bundles with timetag (3.6).
	{
		auto pumpIdleForMs = [&](int ms) {
			const auto t0 = std::chrono::steady_clock::now();
			long pumped = 0;
			while (std::chrono::duration_cast<std::chrono::milliseconds>(
			           std::chrono::steady_clock::now() - t0)
			           .count() < ms) {
				pumped += eng.pumpWhenIdle(4);
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			return pumped;
		};

		const long pumped0 = eng.pumpedBlocks();
		eng.evalSilent("OSCdef(\\sc3pump, { |m| \"SC3-PUMP-SYNC-OK\".postln }, '/synced');"
		               "y = Synth(\\sine440); s.sendMsg('/sync', 991);");
		const long pumped = pumpIdleForMs(400);
		check(pumped > 0 && eng.pumpedBlocks() > pumped0,
		      std::string("pumpWhenIdle advances the World without DSP (") + std::to_string(pumped) +
		          " blocks in 400 ms)");

		const long unitsPump = eng.numUnits();
		check(unitsPump > unitsBefore,
		      std::string("without DSP /s_new is processed (units=") + std::to_string(unitsPump) + ")");

		std::string post;
		for (int i = 0; i < 40 && post.find("SC3-PUMP-SYNC-OK") == std::string::npos; ++i) {
			pumpIdleForMs(25);
			post += eng.takePost();
		}
		check(post.find("SC3-PUMP-SYNC-OK") != std::string::npos,
		      "without DSP the World answers /sync (the reply reaches sclang)");

		// 3.6 with DSP off: the timetagged bundle still expires (before, it
		// never expired, because it was only dispatched inside renderBlock). Same
		// wall-clock margin as the block above.
		eng.evalSilent("s.sendBundle(1.0, [11, y.nodeID]);");
		pumpIdleForMs(300);
		const long earlyUnits = eng.numUnits();
		pumpIdleForMs(1200);
		const long lateUnits = eng.numUnits();
		check(earlyUnits > unitsBefore && lateUnits == unitsBefore,
		      std::string("without DSP the timetagged bundle expires anyway (units ") +
		          std::to_string(earlyUnits) + " -> " + std::to_string(lateUnits) + ")");
	}

	// --- IDE parity: the user code is compiled VERBATIM ------------------------
	// with the instance setup (~sc3out/~sc3n) concatenated in front, any
	// block starting with `var` — `( var bi = [1]; … )`, legal in the IDE —
	// failed with \"syntax error, unexpected VAR, expecting ')'\" because the
	// chunk no longer started with `(`. Now the setup goes in its own compile.
	{
		// Returns the post accumulated up to seeing `want` (or running out of
		// blocks).
		auto evalAndCollect = [&](const std::string &setup, const std::string &code,
		                         const std::string &want) {
			eng.takePost();
			eng.takeError();
			eng.evalWithSetup(setup, code);
			std::string p, e;
			for (int i = 0; i < 400 && p.find(want) == std::string::npos; ++i) {
				pumpBlocks(1, 2);
				p += eng.takePost();
				e += eng.takeError();
			}
			check(e.empty(), "no syntax error (err='" + e + "')");
			check(p.find(want) != std::string::npos,
			      "'" + want + "' in the post (post='" + p + "')");
		};

		evalAndCollect("topEnvironment.put(\\sc3out, 0); topEnvironment.put(\\sc3n, 2);",
		               "( ///hexagram///\nvar bi = [\"a\", \"b\"];\nbi.size.postln;\n)",
		               "2");

		// The setup ran BEFORE the user code (and in its own compile).
		evalAndCollect("topEnvironment.put(\\sc3out, 7); topEnvironment.put(\\sc3n, 2);",
		               "(~sc3out + ~sc3n).postln;", "9");
	}

	// --- panic = Cmd+.: CmdPeriod.run (nodos + clocks + hooks) ----------------
	eng.eval("CmdPeriod.run;");
	const long unitsAfterPanic = [&] {
		long u = eng.numUnits();
		for (int i = 0; i < 500 && u != 0; ++i) {
			pumpBlocks(1, 2);
			u = eng.numUnits();
		}
		return u;
	}();
	check(unitsAfterPanic == 0, std::string("panic (CmdPeriod.run) left the World with no units (units=") +
	                                std::to_string(unitsAfterPanic) + ")");

	// --- `/clearSched` has to drop the bundles waiting for a timetag ---
	// Anything that uses `server.latency` (Ndef/NodeProxy, `s.sendBundle(s.latency…)`)
	// delivers its /s_new in a bundle with a NEAR-FUTURE timetag. Those bundles live
	// in the engine's own `pending` queue, NOT in the World's scheduler, so
	// `CmdPeriod.run` → `Server:freeAll` → `/clearSched` used to leave them there:
	// the node was created AFTER the panic, in a group that no longer existed
	// (`FAILURE IN SERVER /s_new Group … not found`) or hanging in the root ⇒ orphan
	// units that no client can free: silence, but the DSP cost stayed (the "DSP meter
	// stays high after Ctrl+." report). In scsynth `/clearSched` drops them.
	{
		// The environment legitimately re-creates nodes after a CmdPeriod (the
		// startup file / quarks, e.g. Safety's clip synth): take the floor AFTER a
		// clean panic, never before it.
		eng.evalSilent("CmdPeriod.run;");
		for (int i = 0; i < 60; ++i)
			pumpBlocks(1, 4);
		const long floor = eng.numUnits();
		eng.evalSilent("SynthDef(\\lat22, { Out.ar(0, SinOsc.ar(100, 0, 0)) }).add;");
		for (int i = 0; i < 25; ++i)
			pumpBlocks(1, 4);
		// Bundle with a future timetag AND CmdPeriod in the SAME eval: when the panic
		// clears the scheduler, the /s_new is still in flight (timetag = +s.latency).
		// Deterministic: no dependence on how fast sclang evaluates.
		eng.evalSilent("s.sendBundle(s.latency, [\\s_new, \\lat22, 9001, 1, 0]); CmdPeriod.run;");
		for (int i = 0; i < 80; ++i)
			pumpBlocks(1, 4); // ~320 ms of wall clock: past the 0.2 s timetag
		const long after = eng.numUnits();
		std::string panicErr = eng.takeError();
		check(after == floor,
		      std::string("the timetag bundles do not survive CmdPeriod (units=") +
		          std::to_string(after) + ", floor " + std::to_string(floor) + ")");
		check(panicErr.find("not found") == std::string::npos,
		      "no orphan /s_new (err='" + panicErr + "')");
	}

	// --- a graph that ends in its FIRST calc is not resurrected --------
	// `Graph_FirstCalc` (SC_Graph.cpp:510) installs `Graph_Calc` after running the
	// unit constructors, without checking whether one of them already ended the
	// node. `EnvGen` does it as soon as the envelope is over in the very first
	// block, which is the case when `gate` is already 0 before the graph is first
	// calculated: `Synth(\def, [\gate, 0])`, or `/s_new` + `/n_set gate 0` landing
	// in the same block (a short `legato` with a host vector larger than the World
	// block). `Node_End` is then overwritten and the node stays in the tree for
	// ever: silent (`gate` 0) but calculated every block, one per note — the CPU
	// growth from 1.8 % (scide) to >30 % measured in Max. The engine records those
	// nodes and re-marks them right after World_Run (the vendored core is pinned).
	{
		eng.evalSilent("CmdPeriod.run;");
		for (int i = 0; i < 60; ++i)
			pumpBlocks(1, 4);
		const long floor = eng.numUnits();
		eng.evalSilent("SynthDef(\\ctorEnd36, {|out=0, gate=1| "
		               "var env = EnvGen.kr(Env.asr(0, 1, 0.1), gate, doneAction: 2); "
		               "Out.ar(out, SinOsc.ar(440) * env * 0.1) }).add;");
		for (int i = 0; i < 25; ++i)
			pumpBlocks(1, 4);
		eng.evalSilent("Synth(\\ctorEnd36, [\\gate, 0]);");
		for (int i = 0; i < 40; ++i) // ~160 ms: well past the 0.1 s release
			pumpBlocks(1, 4);
		const long after = eng.numUnits();
		check(after == floor,
		      std::string("a node that ends in its first calc is deleted (units=") +
		          std::to_string(after) + ", floor " + std::to_string(floor) + ")");
	}

	// --- cierre limpio ---------------------------------------------------------
	pumpBlocks(4, 2);
	std::string post = eng.takePost();
	if (!post.empty())
		std::printf("  post: %s\n", post.c_str());
	std::string err = eng.takeError();
	if (!err.empty())
		std::printf("  err: %s\n", err.c_str());

	// --- 2.4: per-flush cap with long posts (64 KB)
	// -----------------------------
	eng.eval("8000.do { |i| (\"post largo \" ++ i ++ \" \" ++ \"-\".dup(30)).postln };");
	{
		std::string chunk;
		for (int i = 0; i < 4000 && chunk.empty(); ++i) {
			pumpBlocks(1, 1);
			chunk = eng.takePost(64 * 1024);
		}
		check(!chunk.empty() && chunk.size() <= 64 * 1024,
		      std::string("takePost(64 KB) respects the limit (") +
		          std::to_string(chunk.size()) + " bytes)");
		check(chunk.empty() || chunk.back() == '\n',
		      "the cut falls on a newline");
		/* The rest is pumped too: there is no guarantee that sclang has already posted
		 * the 8000 lines when the first block arrives (it was a gate flake: sometimes
		 * the buffer was empty at that instant even though the rest arrived later). */
		std::string rest;
		for (int i = 0; i < 4000 && rest.empty(); ++i) {
			pumpBlocks(1, 1);
			rest = eng.takePost();
		}
		check(!rest.empty(), std::string("the rest is left for the next flush (") +
		                         std::to_string(rest.size()) + " bytes)");
		eng.takePost();
	}

	eng.release();
	check(!eng.isReady(), "release() left the engine closed");

	// --- 3.1: close during boot (cooperative cancel)
	// ---------------------------- The VM cannot be aborted in the middle of
	// compileLibrary (there is no safe point), so release() waits for the boot to
	// reach the next checkpoint and abandons it there: ready is NOT published and
	// no World is left alive.
	{
		const auto t0 = std::chrono::steady_clock::now();
		const bool started = eng.open(cfg);
		check(started && eng.isBooting(), "open() leaves the engine in booting");
		eng.release();
		const long ms = (long)std::chrono::duration_cast<std::chrono::milliseconds>(
		                    std::chrono::steady_clock::now() - t0)
		                    .count();
		check(!eng.isReady() && !eng.isBooting(),
		      std::string("close during boot: does not publish ready (") +
		          std::to_string(ms) + " ms de espera)");
		check(ms < 30000, std::string("close during boot does not hang (") +
		                      std::to_string(ms) + " ms)");
		Sc3EngineStatus cst = eng.status();
		check(!cst.ready && !cst.failed,
		      std::string("clean state after cancelling the boot (ready=") +
		          (cst.ready ? "yes" : "no") + " failed=" + (cst.failed ? "yes" : "no") + ")");
	}

	// --- retry: open() after a failed boot (port busy)
	// -------------------------- The real case: the SuperCollider IDE holds
	// 57110. The object drops the failed engine on its own, so sending `open`
	// again must work without reloading the patch (open() cleans
	// worker/World/socket before retrying).
	{
		const int busyPort = 57119;
		int blocker = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in a {};
		a.sin_family = AF_INET;
		a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		a.sin_port = htons((uint16_t)busyPort);
		const bool blocked = blocker >= 0 && bind(blocker, (struct sockaddr *)&a, sizeof(a)) == 0;
		Sc3EngineConfig busy = cfg;
		busy.port = busyPort;
		check(blocked && eng.open(busy), "open() starts even if the port is busy (async boot)");
		check(!eng.waitReady(30000), "the boot fails with the port busy");
		check(eng.status().failed, "the engine stays in Failed with the bind error");
		eng.release(); // what the object does when it detects the failure
		if (blocker >= 0)
			close(blocker);
		{
			const bool o = eng.open(cfg);
			const bool w = o && eng.waitReady(120000);
			if (!w) {
				Sc3EngineStatus rst = eng.status();
				std::printf("  [dbg] open=%d booting=%d failed=%d ready=%d err=%s\n", (int)o,
				            (int)rst.booting, (int)rst.failed, (int)rst.ready, rst.error.c_str());
				std::string e = eng.takeError();
				if (!e.empty())
					std::printf("  [dbg] err: %s\n", e.c_str());
			}
			check(w, "open() retries and starts after a failure");
		}
		check(eng.isReady(), "the engine became ready on the retry");
		{
			Sc3EngineStatus rst = eng.status();
			check(rst.port > 0 && rst.port != busyPort,
			      std::string("4.x: with the requested port busy, auto picks another (") +
			          std::to_string(rst.port) + " ≠ " + std::to_string(busyPort) + ")");
		}
		eng.release();
	}

	std::printf("== engine_test: %s ==\n", gAllOk ? "OK" : "FAIL");
	return gAllOk ? 0 : 1;
}
