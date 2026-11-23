// SPDX-License-Identifier: GPL-3.0-only
// eval_check.cpp — runs a .scd file in the embedded engine (headless) and
// measures whether it produces audio. Useful to test SC code without opening Max.
//
// Usage: ./build/eval_check <file.scd> [durationMs]     (default 2500 ms)
//   SC3_PORT=0            (default: auto port; a value forces it)
//   SC3_COMPAT_DIR=... (default ../the first spike/compat/SCMaxCompat)
//   SC3_CL_DIR=...        (class library; default real SC installation)
//   SC3_DEBUG_OSC=1       (OSC trace)
//
// Prints RMS/peak/units every ~250 ms and at the end the post/err of the engine.

#include "../Sc3Engine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
	if (argc < 2) {
		std::printf("uso: %s <archivo.scd> [duracionMs]\n", argv[0]);
		return 2;
	}
	const char *path = argv[1];
	const int durationMs = (argc > 2) ? std::atoi(argv[2]) : 2500;

	std::string src;
	if (!readFile(path, src)) {
		std::printf("[FAIL] no puedo leer %s\n", path);
		return 1;
	}

	Sc3EngineConfig cfg;
	const char *port = std::getenv("SC3_PORT");
	cfg.port = port ? std::atoi(port) : 0; // 4.x: 0 = auto
	// Issue #1 item 9: SC3_RESOURCES_DIR = the staged payload of the bundle
	// (class library + .scx + sounds): the self-contained path, no installation.
	const char *resDir = std::getenv("SC3_RESOURCES_DIR");
	if (resDir && *resDir)
		cfg.resourcesDir = resDir;
	const char *compat = std::getenv("SC3_COMPAT_DIR");
	cfg.compatDir = compat                 ? compat
	                : (resDir && *resDir) ? std::string(resDir) + "/SCMaxCompat"
	                                      : "../phase1/compat/SCMaxCompat";
	const char *cldir = std::getenv("SC3_CL_DIR");
	cfg.classLibraryDir = cldir ? cldir : "";
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;

	Sc3Engine &eng = Sc3Engine::instance();
	if (!eng.open(cfg) || !eng.waitReady(120000)) {
		Sc3EngineStatus st = eng.status();
		std::printf("[FAIL] boot: %s\n", st.error.c_str());
		std::string e = eng.takeError();
		if (!e.empty())
			std::printf("       err: %s\n", e.c_str());
		eng.release();
		return 1;
	}

	Sc3EngineStatus st = eng.status();
	const int sr = (int)std::lround(st.sampleRate);
	const long bl = eng.blockLength();
	std::printf("[info] boot en %ld ms, sr=%d blockLen=%ld\n", st.bootMs, sr, bl);

	eng.eval(src);
	std::printf("[info] %s enviado (%zu bytes), midiendo %d ms...\n", path, src.size(),
	            durationMs);

	std::vector<float> L((size_t)bl), R((size_t)bl);
	const int totalBlocks = (int)((long)sr * durationMs / 1000 / bl);
	const int reportEvery = std::max(1, sr / 4 / (int)bl); // ~250 ms
	double globalPeak = 0.0;
	double lastRms = 0.0;
	long units = 0;
	for (int b = 0; b < totalBlocks; ++b) {
		eng.renderBlock(L.data(), R.data());
		double sum = 0.0;
		for (long i = 0; i < bl; ++i) {
			sum += (double)L[(size_t)i] * L[(size_t)i];
			globalPeak = std::max(globalPeak, (double)std::fabs(L[(size_t)i]));
		}
		lastRms = std::sqrt(sum / (double)bl);
		units = eng.numUnits();
		if ((b + 1) % reportEvery == 0) {
			std::printf("  t=%5.2fs  rms=%.5f  peak=%.4f  units=%ld\n",
			            (b + 1) * (double)bl / (double)sr, lastRms, globalPeak, units);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	std::printf("[info] units=%ld peak=%.4f rms_final=%.5f\n", units, globalPeak, lastRms);
	std::printf("%s\n", (units > 0 && globalPeak > 1e-3) ? "[OK ] sounding" : "[WARN] no audio");

	eng.eval("CmdPeriod.run;");
	for (int i = 0; i < 200 && eng.numUnits() > 0; ++i) {
		eng.renderBlock(L.data(), R.data());
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::string post = eng.takePost();
	if (!post.empty())
		std::printf("--- post ---\n%s", post.c_str());
	std::string err = eng.takeError();
	if (!err.empty())
		std::printf("--- err ---\n%s", err.c_str());
	std::printf("--- fin (%s) ---\n", err.empty() ? "no sclang errors" : "with errors");

	eng.release();
	return (units > 0 && globalPeak > 1e-3) ? 0 : 1;
}
