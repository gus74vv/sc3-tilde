// SPDX-License-Identifier: GPL-3.0-only
// multi_test.cpp — headless gate of 3.5 (multi-instance through buses).
//
// Verifies the render model of the engine without Max: the World advances **a
// single time per absolute sample position** and each instance (`Sc3AudioCursor`)
// reads its own bus window from the same stream.
//
//   1. two cursors of the same bus, on the same "tick" ⇒ identical samples and the
//      World advanced `frames` (not 2×frames)
//   2. per-bus isolation: a Synth that writes only to bus 2 does not sound on the
//      cursor of bus 0 (and bus 2 does sound)
//   3. different windows (64 vs 128 frames) over the same stream ⇒ the content
//      matches (same audio, different "signal vector")
//   4. bus with no writers ⇒ exact silence (mAudioBusTouched: stopping the Synth
//      does not leave the last block stuck)
//   5. cursor more than the whole history behind ⇒ underrun counted + jump to the
//      present, without blocking or crashing
//   6. resetCursor() leaves the cursor at the "now" of the stream
//
// Usage: ./build/multi_test [port]
//        SC3_CL_DIR / SC3_COMPAT_DIR / SC3_DEBUG_OSC as in engine_test.
//
// Port: default 0 = AUTO (the kernel picks it) ⇒ runs with the SC IDE open.

#include "../Sc3Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

static double rms(const std::vector<double> &v)
{
	if (v.empty())
		return 0.0;
	double acc = 0.0;
	for (double s : v)
		acc += s * s;
	return std::sqrt(acc / (double)v.size());
}

static bool allZero(const std::vector<double> &v)
{
	for (double s : v)
		if (s != 0.0)
			return false;
	return true;
}

static std::string evalWait(Sc3Engine &eng, const std::string &code,
                            const std::string &marker, int timeoutMs = 4000)
{
	eng.eval(code);
	std::string acc;
	const auto t0 = std::chrono::steady_clock::now();
	while (std::chrono::duration_cast<std::chrono::milliseconds>(
	           std::chrono::steady_clock::now() - t0)
	           .count() < timeoutMs) {
		acc += eng.takePost();
		if (acc.find(marker) != std::string::npos)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return acc;
}

// Same, but with the instance setup in its OWN compile — the way the external
// feeds ~sc3out/~sc3n to the user code (sc3_ctx_setup in sc3_tilde.cpp).
static std::string evalWaitSetup(Sc3Engine &eng, const std::string &setup,
                                 const std::string &code, const std::string &marker,
                                 int timeoutMs = 4000)
{
	eng.evalWithSetup(setup, code);
	std::string acc;
	const auto t0 = std::chrono::steady_clock::now();
	while (std::chrono::duration_cast<std::chrono::milliseconds>(
	           std::chrono::steady_clock::now() - t0)
	           .count() < timeoutMs) {
		acc += eng.takePost();
		if (acc.find(marker) != std::string::npos)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return acc;
}

int main(int argc, char **argv)
{
	Sc3EngineConfig cfg;
	cfg.port = (argc > 1) ? std::atoi(argv[1]) : 0; // 4.x: 0 = auto
	const char *cldir = std::getenv("SC3_CL_DIR");
	cfg.classLibraryDir = cldir ? cldir : "";
	const char *compat = std::getenv("SC3_COMPAT_DIR");
	cfg.compatDir = compat ? compat : "../phase1/compat/SCMaxCompat";
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;

	Sc3Engine &eng = Sc3Engine::instance();
	if (!eng.open(cfg)) {
		Sc3EngineStatus st = eng.status();
		std::printf("[FAIL] open: %s\n", st.error.c_str());
		return 1;
	}
	if (!eng.waitReady(120000)) {
		Sc3EngineStatus st = eng.status();
		std::printf("[FAIL] boot: %s\n", st.error.c_str());
		std::string e = eng.takeError();
		if (!e.empty())
			std::printf("       err: %s\n", e.c_str());
		eng.release();
		return 1;
	}

	const long n = eng.blockLength();
	const long served = eng.servedBuses();
	std::printf("[info] blockLength=%ld servedBuses=%ld rendered=%llu\n", n, served,
	            (unsigned long long)eng.renderedSamples());
	check(n > 0 && served >= 4, "engine ready with served buses (blockLength " +
	                                std::to_string(n) + ", served " +
	                                std::to_string(served) + ")");
	check(served == std::min<long>(eng.status().worldBuses, Sc3Engine::kServedBuses),
	      "servedBuses = min(worldBuses, kServedBuses)");

	// --- Synth that writes ONLY to bus 2 --------------------------------------
	eng.eval("SynthDef(\\mtone2, { Out.ar(2, SinOsc.ar(440, 0, 0.5)) }).add;");
	for (int i = 0; i < 12; ++i)
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	eng.eval("y = Synth(\\mtone2);");
	// so that the /s_new reaches the World and the Synth starts
	// writing
	std::vector<double> warm(64, 0.0);
	{
		Sc3AudioCursor c;
		eng.resetCursor(c);
		double *outs[1] = {warm.data()};
		(void)eng.pull(c, 2, 1, outs, 64);
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}
	check(eng.numUnits() > 0, std::string("Synth creado (units=") +
	                              std::to_string(eng.numUnits()) + ")");

	// --- 1. dos cursores, mismo bus, mismo tick -------------------------------
	{
		Sc3AudioCursor a, b;
		eng.resetCursor(a);
		eng.resetCursor(b);
		const uint64_t before = eng.renderedSamples();
		std::vector<double> va(64, -1.0), vb(64, -1.0);
		double *oa[1] = {va.data()};
		double *ob[1] = {vb.data()};
		const bool oka = eng.pull(a, 2, 1, oa, 64);
		const bool okb = eng.pull(b, 2, 1, ob, 64);
		const uint64_t advanced = eng.renderedSamples() - before;
		check(oka && okb, "two cursors on the same bus read the same tick");
		check(advanced == 64, std::string("the World advanced 64 samples, not 128 (advanced ") +
		                          std::to_string((long)advanced) + ")");
		check(va == vb, "the second cursor read the SAME samples (bit-identical)");
		check(rms(va) > 0.1, std::string("bus 2 has signal (RMS ") +
		                         std::to_string(rms(va)) + ")");
	}

	// --- 2. per-bus isolation ------------------------------------------------
	{
		Sc3AudioCursor c0, c2;
		eng.resetCursor(c0);
		eng.resetCursor(c2);
		std::vector<double> l(64, -1.0), r(64, -1.0), m(64, -1.0);
		double *o0[2] = {l.data(), r.data()};
		double *o2[1] = {m.data()};
		const bool ok0 = eng.pull(c0, 0, 2, o0, 64);
		const bool ok2 = eng.pull(c2, 2, 1, o2, 64);
		check(ok0 && ok2, "cursors on different buses coexist (0 and 2)");
		check(allZero(l) && allZero(r), "bus 0 silent (nobody writes there)");
		check(rms(m) > 0.1, std::string("bus 2 suena (RMS ") + std::to_string(rms(m)) + ")");
	}

	// --- 3. different windows over the same stream ----------------------------
	{
		Sc3AudioCursor c64, c128;
		eng.resetCursor(c64);
		eng.resetCursor(c128);
		std::vector<double> v64, v128(128, -1.0);
		double *o128[1] = {v128.data()};
		const bool ok128 = eng.pull(c128, 2, 1, o128, 128);
		bool ok = ok128;
		for (int i = 0; i < 2; ++i) {
			std::vector<double> v(64, -1.0);
			double *o[1] = {v.data()};
			ok = eng.pull(c64, 2, 1, o, 64) && ok;
			v64.insert(v64.end(), v.begin(), v.end());
		}
		check(ok, "pulls with 64 and 128 frames coexist");
		check(v64 == v128, "64 vs 128 windows match sample by sample");
	}

	// --- 4. bus with no writers ⇒ exact silence ------------------------------
	{
		eng.eval("y.free;"); // cut the only writer of bus 2
		Sc3AudioCursor c;
		eng.resetCursor(c);
		std::vector<double> v(64, -1.0);
		double *o[1] = {v.data()};
		eng.pull(c, 2, 1, o, 64); // one block of margin so the /n_free arrives
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		eng.resetCursor(c);
		std::fill(v.begin(), v.end(), -1.0);
		const bool ok = eng.pull(c, 2, 1, o, 64);
		check(ok, "pull of bus 2 after freeing the Synth");
		check(allZero(v), "a bus with no writers comes out in exact silence (mAudioBusTouched)");
		eng.eval("y = nil;");
	}

	// --- 5. cursor atrasado ⇒ underrun contado y salto al presente ------------
	{
		eng.eval("y = Synth(\\mtone2);");
		Sc3AudioCursor stale, adv;
		eng.resetCursor(stale);
		eng.resetCursor(adv);
		std::vector<double> v(64, 0.0);
		double *o[1] = {v.data()};
		const long underBefore = eng.underruns();
		// advance the World more than the history (>= 4096 samples ⇒ > 64
		// blocks)
		bool ok = true;
		for (int i = 0; i < 80; ++i)
			ok = eng.pull(adv, 2, 1, o, 64) && ok;
		check(ok, "80 blocks ahead (5120 samples > history)");
		const bool staleOk = eng.pull(stale, 2, 1, o, 64);
		check(!staleOk, "the lagging cursor returns false (silence)");
		check(eng.underruns() > underBefore, "the underrun was counted");
		const bool after = eng.pull(stale, 2, 1, o, 64);
		check(after, "after the jump to the present the cursor reads again");
		eng.eval("y.free; y = nil;");
	}

	// --- 6. resetCursor() points at now --------------------------------------
	{
		Sc3AudioCursor c;
		eng.resetCursor(c);
		const uint64_t r = eng.renderedSamples();
		check(c.pos == r, "resetCursor deja pos = renderedSamples (" + std::to_string((long)r) +
		                      ")");
	}

	// --- 7. JITLib .play uses ~sc3out (SCMaxCompat module) -------------------
	{
		const std::string out = evalWait(eng, R"SC3(
~sc3out = 6; ~sc3n = 2;
Ndef(\mtndef, { SinOsc.ar(300) }).play;
if(Ndef(\mtndef).monitorIndex == 6) { "SC3PLYOK".postln }
	{ ("SC3PLYFAIL:" ++ Ndef(\mtndef).monitorIndex).postln };
)SC3", "SC3PLY");
		check(out.find("SC3PLYOK") != std::string::npos,
		      "the bare .play takes ~sc3out (monitor on bus 6)");
		const std::string out2 = evalWait(eng, R"SC3(
Ndef(\mtndef).stop; Ndef(\mtndef).clear;
~sc3out = 6;
Ndef(\mtndef2, { SinOsc.ar(300) }).play(out: 3);
if(Ndef(\mtndef2).monitorIndex == 3) { "SC3PLYOK2".postln }
	{ ("SC3PLYFAIL2:" ++ Ndef(\mtndef2).monitorIndex).postln };
)SC3", "SC3PLY");
		check(out2.find("SC3PLYOK2") != std::string::npos,
		      "the explicit .play(out: 3) wins over ~sc3out");
		eng.eval("Ndef(\\mtndef2).stop; Ndef(\\mtndef2).clear;");
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// --- 7c. JITLib .playN uses ~sc3out too (SCMaxCompat module) ---------------
	// Stock turns a nil `outs` into 0 (InBus.sc:297) = the hardware buses, so on a
	// 2nd instance a bare .playN wrote into the 1st instance's block: audible, no
	// error, wrong object. The default is the instance bus now, and the channel
	// expansion (bus base + N channels) is stock.
	{
		const std::string out = evalWait(eng, R"SC3(
~sc3out = 4; ~sc3n = 2;
Ndef(\mtnpn, { SinOsc.ar([300, 400]) * 0.1 }).playN;
if(Ndef(\mtnpn).monitor.outs == [4, 5]) { "SC3PNOK".postln }
	{ ("SC3PNFAIL:" ++ Ndef(\mtnpn).monitor.outs).postln };
)SC3", "SC3PN");
		check(out.find("SC3PNOK") != std::string::npos,
		      "a bare .playN takes ~sc3out and expands to its channels ([4, 5]): " + out);
		const std::string out2 = evalWait(eng, R"SC3(
Ndef(\mtnpn2, { SinOsc.ar([300, 400]) * 0.1 }).playN([1, 0]);
if(Ndef(\mtnpn2).monitor.outs == [1, 0]) { "SC3PNOK2".postln }
	{ ("SC3PNFAIL2:" ++ Ndef(\mtnpn2).monitor.outs).postln };
)SC3", "SC3PN");
		check(out2.find("SC3PNOK2") != std::string::npos,
		      "an explicit .playN([1, 0]) wins over ~sc3out and keeps the order: " + out2);
		eng.eval("Ndef(\\mtnpn).clear; Ndef(\\mtnpn2).clear;");
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// --- 7b. a pushed ProxySpace does not break the routing ----------
	// `ProxySpace.push` leaves currentEnvironment as a LazyEnvir, and reading a
	// MISSING key there CREATES a NodeProxy for it (LazyEnvir:at). The bus used to
	// be read from currentEnvironment, so inside a space the bare .play got a
	// NodeProxy instead of the bus number: nodes were created and nothing sounded
	// (`ProxySpace.push(s.boot); ~out.play; ~out = { ... }` = silence, no error).
	// Second half of the same bug: the setup must write topEnvironment, otherwise
	// the `~sc3out` of the NEXT eval lands INSIDE the space (again as a NodeProxy)
	// and the space is polluted.
	{
		const std::string setup =
		    "topEnvironment.put(\\sc3out, 6); topEnvironment.put(\\sc3n, 2);";
		const std::string out = evalWaitSetup(eng, setup, R"SC3(
p = ProxySpace.push(s.boot);
~out.play;
~out = { SinOsc.ar(300) };
if(~out.monitorIndex == 6) { "SC3PSOK".postln } { ("SC3PSFAIL:" ++ ~out.monitorIndex).postln };
if(p.envir.at(\sc3out).isNil) { "SC3PSNOBUS".postln } { "SC3PSPOLLUTED".postln };
"SC3PSEND".postln;
)SC3", "SC3PSEND");
		check(out.find("SC3PSOK") != std::string::npos,
		      "inside a pushed ProxySpace the bare .play still takes ~sc3out (monitor "
		      "on bus 6): " + out);
		check(out.find("SC3PSPOLLUTED") == std::string::npos,
		      "the ProxySpace did not get a \\sc3out NodeProxy: " + out);
		// Setup of a SECOND eval, with the space still current: it must not land in it.
		const std::string out2 =
		    evalWaitSetup(eng, setup, R"SC3(
if(p.envir.at(\sc3out).isNil) { "SC3PSNOBUS2".postln } { "SC3PSPOLLUTED2".postln };
"SC3PSEND2".postln;
)SC3", "SC3PSEND2");
		check(out2.find("SC3PSNOBUS2") != std::string::npos,
		      "a second eval's setup does not write ~sc3out into the current ProxySpace: " +
		          out2);
		eng.eval("~out.clear; p.pop;");
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// --- 8. STEREO source in the same sc3~: both channels come out -------------
	// (a MONO Ndef/Synth takes 1 bus => only the first outlet; stereo takes 2 =>
	// both. If this fails, the channel mapping of the object is broken.)
	{
		eng.eval("SynthDef(\\mst2, { Out.ar(2, [SinOsc.ar(300), SinOsc.ar(900)] * 0.2) }).add;");
		for (int i = 0; i < 12; ++i)
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		eng.eval("z = Synth(\\mst2);");
		Sc3AudioCursor c;
		eng.resetCursor(c);
		std::vector<double> l(64, 0.0), r(64, 0.0);
		double *o[2] = {l.data(), r.data()};
		eng.pull(c, 2, 2, o, 64); // margin for the /s_new
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		eng.resetCursor(c);
		std::fill(l.begin(), l.end(), 0.0);
		std::fill(r.begin(), r.end(), 0.0);
		const bool ok = eng.pull(c, 2, 2, o, 64);
		check(ok, "stereo pull (2 channels from bus 2)");
		check(rms(l) > 0.05 && rms(r) > 0.05,
		      std::string("BOTH channels come out (RMS L ") + std::to_string(rms(l)) +
		          " / R " + std::to_string(rms(r)) + ")");
		check(l != r, "channel 0 = bus 2 and channel 1 = bus 3 (different signals)");
		eng.eval("z.free; z = nil;");
	}

	// --- the engine is refcounted BY OWNER ---------------------------
	// Two `sc3~` objects (two owners) share one World. `open`/`release` are
	// idempotent per owner, so: a double `open` from the same object does not
	// inflate the count, releasing an owner that is not there does nothing, and the
	// World is torn down only when the LAST owner leaves. With a plain counter a
	// double open leaked one reference and the last `close` left the World running
	// (DSP cost kept, `open` answering "already open" and no boot in the console).
	{
		int ownerA = 0, ownerB = 0, stranger = 0;
		const auto before = eng.status().bootMs;
		eng.open(cfg, &ownerA); // already open+ready: no boot
		eng.open(cfg, &ownerA); // idempotent per owner (a counter would count twice)
		eng.open(cfg, &ownerB);
		check(eng.isReady() && eng.status().bootMs == before,
		      "bug #23: open() by more owners reuses the running World (no new boot)");
		eng.release(); // the owner this test opened with (nullptr)
		eng.release(&ownerA);
		check(eng.isReady(), "bug #23: the engine stays up while one owner is left");
		// With a plain counter this one would NOT close (3 opens, 2 releases) — the
		// reference leaked by the double `open` above was exactly the bug.
		eng.release(&ownerB);
		check(!eng.isReady() && std::string(eng.stateName()) == "idle",
		      std::string("bug #23: the last release tears the World down (state=") +
		          eng.stateName() + ")");
		eng.release(&ownerB); // releasing twice must not underflow (the old counter did)
		eng.release(&stranger); // never an owner
		check(std::string(eng.stateName()) == "idle",
		      "bug #23: repeated/foreign release() is a no-op (no underflow)");
	}

	eng.release();
	std::printf(gAllOk ? "== multi_test: OK ==\n" : "== multi_test: FAIL ==\n");
	return gAllOk ? 0 : 1;
}
