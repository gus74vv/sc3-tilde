// SPDX-License-Identifier: GPL-3.0-only
// Sc3Engine.cpp — the bootstrap verified in phase1, ported to a singleton with
// refcount, an sclang worker and per-block rendering. See Sc3Engine.h for the
// threading model and phase1/test_world.cpp for why each step exists.
//
// Boot order (do not re-investigate, see handoff.md):
//   1. bindUDP(port)                      // before boot: no lost datagrams
//   2. Resource dir = our own class library (stock + SCMaxCompat)
//   3. sclang: boot() → compileLibrary (startup.scd + Quarks) or stock fallback
//   4. readServerOptions()                // s.options are ALREADY configured
//   5. World_New + World_SetSampleRate + World_Start
//   6. deterministic adoption: serverRunning + sendDefaultGroups + notified
//   7. ready = true  → from here on only the audio thread touches the World

#include "Sc3Engine.h"

#include "Sc3LangClient.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <thread>

#include "OSC_Packet.h"   // struct OSC_Packet (server/scsynth)
#include "SC_Lib_Cintf.h" // deinitialize_library() (unloads the .scx on exit)
#include "SC_Filesystem.hpp"
#include "SC_LanguageConfig.hpp" // gLanguageConfig (include dir of the SCMaxCompat module)
#include "SC_Version.hpp"        // SC_VersionString() del sclang compilado
#include "SC_World.h"
#include "SC_WorldOptions.h"

// World API outside the public headers: C++ (mangled) linkage, same as in
// phase1/test_world.cpp. Do not declare inside extern "C".
void World_Run(struct World *inWorld);
void World_Start(struct World *inWorld);
void World_SetSampleRate(struct World *inWorld, double inSampleRate);
int PerformOSCMessage(struct World *inWorld, int inSize, char *inData, ReplyAddress *inAddr);
void PerformOSCBundle(struct World *inWorld, struct OSC_Packet *inPacket);

namespace {

constexpr size_t kMaxTextBuffer = 256 * 1024;

// 3.4: scsynth posts through `scprintf`, which writes to stdout by default —
// inside Max that is invisible (there is no scsynth console). This hook sends
// that text to the engine buffers, and from there to the outlets: it is the
// only path by which you see an `.scx` that fails to dlopen, a World that
// cannot be created, etc.
// `SetPrintFunc` is global and may be called from the worker (World_New) or
// from the audio thread (World_Run errors) ⇒ buffers under a mutex; the engine
// drains them in appendDrained().
//
// 3.9: up to here ALL scsynth text went to the `error` outlet (red in Max), so
// the output of a `.poll` looked like a failure. scsynth does not separate print
// from error (both use `scprintf`), but it does prefix them differently, so we
// classify per line: messages SC writes as failures go to `err` and the rest
// (Poll, "Loading synthdefs…", "memory locking…") to `post`, i.e. black, same
// as in the IDE post window. A line may arrive split across several `scprintf`
// calls, which is why it is assembled in `gWorldPrintTail` until the '\n'.
std::mutex gWorldPrintMtx;
std::string gWorldPrintPost;                      // what goes to the `post` outlet
std::string gWorldPrintErr;                       // what goes to the `error` outlet
std::string gWorldPrintTail;                      // line in progress (no '\n')
std::chrono::steady_clock::time_point gWorldPrintLast{};

// A real failure (same prefixes the server uses): `*** ERROR:` (plugins,
// synthdefs, sockets), `ERROR:`/`FAILURE IN SERVER:` (OSC commands), `Exception`
// (World_New/World_Run). A `WARNING:` is not a failure ⇒ black, as in the IDE.
static bool sc3WorldLineIsError(const std::string &line)
{
	size_t i = 0;
	while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n'))
		++i;
	const char *s = line.c_str() + i;
	return !std::strncmp(s, "*** ERROR", 9) || !std::strncmp(s, "ERROR", 5) ||
	       !std::strncmp(s, "FAILURE IN SERVER", 17) || !std::strncmp(s, "Exception", 9) ||
	       !std::strncmp(s, "exception", 9);
}

// Moves the line in progress to the buffer it belongs to. Called on seeing the
// '\n' and, if the print never sends it (a lone chunk), once kTailIdleMs of
// silence have passed.
static void sc3WorldFlushTailLocked()
{
	if (gWorldPrintTail.empty())
		return;
	std::string &dst = sc3WorldLineIsError(gWorldPrintTail) ? gWorldPrintErr : gWorldPrintPost;
	dst += gWorldPrintTail;
	gWorldPrintTail.clear();
}

static constexpr long kTailIdleMs = 100;

int sc3WorldPrint(const char *fmt, va_list ap)
{
	char buf[1024];
	const int n = std::vsnprintf(buf, sizeof(buf), fmt, ap);
	if (n <= 0)
		return n;
	std::lock_guard<std::mutex> lk(gWorldPrintMtx);
	gWorldPrintLast = std::chrono::steady_clock::now();
	gWorldPrintTail.append(buf, std::min<size_t>((size_t)n, sizeof(buf) - 1));
	size_t nl;
	while ((nl = gWorldPrintTail.find('\n')) != std::string::npos) {
		std::string line = gWorldPrintTail.substr(0, nl + 1);
		gWorldPrintTail.erase(0, nl + 1);
		std::string &dst = sc3WorldLineIsError(line) ? gWorldPrintErr : gWorldPrintPost;
		dst += line;
		if (dst.size() > kMaxTextBuffer)
			dst.erase(0, dst.size() - kMaxTextBuffer);
	}
	if (gWorldPrintTail.size() > 4096) /* print without '\n' and already huge: stop waiting */
		sc3WorldFlushTailLocked();
	return n;
}

// Boot state (3.1). A single owner of the transitions: the worker; everyone
// else only reads. "ready" (below) is the only thing the audio thread looks at.
enum : int { kBootIdle = 0, kBootBooting = 1, kBootReady = 2, kBootFailed = 3 };

// UGens: the .scx are NOT inside libscsynth (built without STATIC_PLUGINS) and
// mUGensPluginPath is EXCLUSIVE when passed => we have to list stock + user
// (decision 3 in PLAN). "stock" means the real SC installation's.
std::string userPluginsDir()
{
	const char *home = std::getenv("HOME");
	return home ? std::string(home) + "/Library/Application Support/SuperCollider/Extensions" : "";
}

// Installation version ("/Applications/SuperCollider.app") — the class library
// and the .scx have to be the SAME version as the compiled sclang.
std::string installedScVersion(const std::string &appPath)
{
	const std::string plist = appPath + "/Contents/Info.plist";
	const std::string cmd = "plutil -extract CFBundleShortVersionString raw -o - \"" + plist +
	                        "\" 2>/dev/null";
	FILE *p = popen(cmd.c_str(), "r");
	if (!p)
		return {};
	std::string out;
	char buf[128];
	while (std::fgets(buf, sizeof(buf), p))
		out += buf;
	pclose(p);
	while (!out.empty() && (out.back() == '\n' || out.back() == ' ' || out.back() == '\t'))
		out.pop_back();
	return out;
}

// 4.2 (a): compare by major.minor. "3.14.2" vs "3.14.1" = same minor ⇒ accepted
// (the installed class library is SOURCE CODE that our interpreter compiles, and
// the .scx couple through `api_version`, not through the SC version).
static bool versionMajorMinor(const std::string &v, int *maj, int *min)
{
	int a = 0, b = 0;
	if (std::sscanf(v.c_str(), "%d.%d", &a, &b) != 2)
		return false;
	if (maj)
		*maj = a;
	if (min)
		*min = b;
	return true;
}

int bindUdp(int port)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return -1;
	int one = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	struct sockaddr_in addr {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons((uint16_t)port);
	if (bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
		close(sock);
		return -1;
	}
	fcntl(sock, F_SETFL, O_NONBLOCK);
	return sock;
}

// Reply context: Perform* answers the datagram sender over the same socket
// (ReplyAddress.mReplyData points here). There is a single source of datagrams
// at a time (boot: worker; afterwards: audio thread).
struct ReplyCtx {
	int sock = -1;
	struct sockaddr_in peer {};
};

void engineReplyFunc(struct ReplyAddress *addr, char *data, int size)
{
	auto *ctx = static_cast<ReplyCtx *>(addr->mReplyData);
	if (!ctx || ctx->sock < 0)
		return;
	sendto(ctx->sock, data, size, 0,
	       reinterpret_cast<struct sockaddr *>(&ctx->peer), sizeof(ctx->peer));
}

// --- 3.6: OSC timetags (bug #11) -----------------------------------------
// sclang anchors timetags to the host NTP clock (PyrSched.cpp:245,
// syncOSCOffsetWithTimeOfDay) ⇒ comparing against system_clock is correct.
inline int64_t oscNow()
{
	using namespace std::chrono;
	const auto sinceEpoch = system_clock::now().time_since_epoch();
	const auto secs = duration_cast<seconds>(sinceEpoch);
	const auto nanos = duration_cast<nanoseconds>(sinceEpoch - secs);
	return ((int64_t)(secs.count() + 2208988800LL) << 32) +
	       (int64_t)(nanos.count() * 4.294967296); // 2^32/1e9
}

// Big-endian timetag from bytes 8..15 of the bundle (same rule as OSCtime() in
// include/plugin_interface/sc_msg_iter.h).
inline int64_t oscTimetag(const char *data)
{
	uint64_t hi = 0, lo = 0;
	for (int i = 0; i < 4; ++i)
		hi = (hi << 8) | (uint8_t)data[i];
	for (int i = 4; i < 8; ++i)
		lo = (lo << 8) | (uint8_t)data[i];
	return (int64_t)((hi << 32) | lo);
}

constexpr size_t kMaxPending = 256;              // bundles futuros en vuelo
constexpr size_t kMaxPendingBytes = 4 * 1024 * 1024;

} // namespace

// ------------------------------------------------------------------ Impl

struct Sc3Engine::Impl {
	Sc3EngineConfig cfg;
	Sc3LangClient *client = nullptr;
	// 3.3: the World is touched by ONE thread at a time, and the owner really does
	// change: audio in `perform64`; main from Max's clock when DSP is off; worker
	// during boot. Exclusion is `worldMtx` (try_lock in audio/pump: they never
	// block; teardown does wait).
	std::atomic<World *> world {nullptr};
	std::mutex worldMtx;
	int sock = -1;
	ReplyCtx reply;
	bool verbose = false;

	// 3.6 / bug #11: bundles with a future timetag. `NodeProxy:free` sends the
	// group's `/n_free` at `fadeTime + server.latency`; running it immediately
	// killed the proxy-control ramp (`Ndef(…).clear(10)` cut hard).
	// No locks: they are touched by the same thread that owns the World (worker
	// at boot, audio afterwards). The malloc of `data` happens only when a future
	// bundle arrives (not audio-rate); if that ever hurts, move it to a fixed pool
	// in Phase 4.
	struct PendingPkt {
		std::vector<char> data;
		struct sockaddr_in from {};
	};
	std::multimap<int64_t, PendingPkt> pending;
	size_t pendingBytes = 0;
	std::atomic<long> droppedScheduled {0}; // never made it into the queue: it already ran

	std::thread worker;
	std::mutex mtx;
	std::condition_variable cv;
	// `setup` is evaluated first, in its OWN compile (see evalWithSetup).
	struct Job {
		std::string code;
		std::string setup;
		bool print;
	};
	std::deque<Job> queue;
	bool stop = false;
	bool bootDone = false;
	bool bootOk = false;
	int refs = 0;

	std::atomic<bool> ready {false};
	std::atomic<int> state {kBootIdle};
	std::atomic<bool> cancel {false}; // release() during boot
	// bug #13: `initRuntime` (VM pools, init_OSC, schedInit) happens ONCE per
	// process. Re-initializing it on a second open() crashes the VM (jump into
	// freed memory inside the class library compile). The path SC supports is
	// initRuntime once + compileLibrary/shutdownLibrary (what Cmd+K does in the
	// IDE) ⇒ on reopens `client` is kept and the library is recompiled, just like
	// a clean boot.
	bool langBooted = false;
	std::atomic<long> bootMs {0};
	std::atomic<long> units {0};
	// 3.3: World time accounting. Updated on every advance (audio or pump) so
	// that, when the DSP is switched off, the pump knows how far behind it is and
	// does not jump.
	std::atomic<int64_t> lastAdvanceNs {0};
	std::atomic<long> pumpedBlocks {0}; // blocks advanced without DSP (diagnostics)
	std::atomic<long> pumpLagBlocks {0};  // blocks the budget did not let run

	// --- 3.5: multiple instances (buses) -----------------------------------
	// A single stream rendered by the World; each instance (Sc3AudioCursor) reads
	// its window. "hist" is a per-bus ring with DOUBLE WRITE (the block is also
	// written histFrames further ahead) so that any window of up to histFrames
	// samples is contiguous; histFrames is a multiple of blockLength. Buses no
	// UGen touched in the block are stored as silence (mAudioBusTouched ==
	// mBufCounter): without that check, a bus whose writer disappeared keeps the
	// previous block and the object would repeat the last 64 frames (hum/DC).
	std::vector<float> hist; // served × 2 × histFrames
	long histFrames = 0;
	long served = 0;
	std::atomic<uint64_t> rendered {0};
	std::atomic<long> underruns {0};
	std::mutex copyMtx; // hist: written by the render, read by the cursors
	Sc3AudioCursor renderCursor; // the renderBlock() one (tests/CLI)
	std::vector<double> renderTmp;

	// --- 3.8: audio input (see Sc3Engine.h) --------------------------------
	// Planar ring per channel with a silence prefill (latency = prefill ≈ 2 World
	// blocks). The producer writes the host frames; the render consumes exactly
	// one World block. That decoupling keeps a `signal vector` that is not a
	// multiple of `mBufLength` from producing systematic underruns (the ring level
	// oscillates ±1 block around the prefill).
	static constexpr long kInputRingFrames = 16384; // 341 ms @48k
	std::mutex inMtx;      // productor (audio) ↔ consumidor (audio o pump)
	std::vector<float> inBuf; // inChans × kInputRingFrames (planar)
	long inChans = 0;      // canales reclamados (@ins)
	long inPrefill = 0;    // initial silence samples
	uint64_t inWr = 0, inRd = 0; // ABSOLUTE positions in the ring
	std::atomic<const void *> inOwner {nullptr};
	std::atomic<bool> inActive {false};
	std::atomic<uint64_t> inPushed {0};
	std::atomic<long> inSkipped {0}, inUnder {0}, inOver {0};
	// 3.8 diagnostics: without this, "SoundIn.ar(1) in silence" does not tell
	// "the host is not sending the channel" apart from "the engine is not writing
	// it to the bus".
	std::atomic<long> inNumins {0};
	std::atomic<float> inPushPeak[16] = {};
	std::atomic<float> inFeedPeak[16] = {};

	mutable std::mutex outMtx;
	std::string post, err;

	Sc3EngineStatus st;

	// --- text output -------------------------------------------------------
	void appendDrained()
	{
		{
			// 3.9: scsynth text, already classified (print → `post`, failure → `error`).
			std::lock_guard<std::mutex> lk(gWorldPrintMtx);
			if (!gWorldPrintTail.empty() &&
			    std::chrono::duration_cast<std::chrono::milliseconds>(
			        std::chrono::steady_clock::now() - gWorldPrintLast).count() > kTailIdleMs)
				sc3WorldFlushTailLocked(); /* print that did not close the line */
			if (!gWorldPrintPost.empty() || !gWorldPrintErr.empty()) {
				std::lock_guard<std::mutex> lk2(outMtx);
				post += gWorldPrintPost;
				gWorldPrintPost.clear();
				err += gWorldPrintErr;
				gWorldPrintErr.clear();
				if (post.size() > kMaxTextBuffer)
					post.erase(0, post.size() - kMaxTextBuffer);
				if (err.size() > kMaxTextBuffer)
					err.erase(0, err.size() - kMaxTextBuffer);
			}
		}
		if (!client)
			return;
		if (client->mPost.empty() && client->mErr.empty())
			return;
		std::lock_guard<std::mutex> lk(outMtx);
		post += client->mPost;
		err += client->mErr;
		client->mPost.clear();
		client->mErr.clear();
		if (post.size() > kMaxTextBuffer)
			post.erase(0, post.size() - kMaxTextBuffer);
		if (err.size() > kMaxTextBuffer)
			err.erase(0, err.size() - kMaxTextBuffer);
	}

	// --- OSC: drenar socket propio y despachar -----------------------------
	void drainOsc()
	{
		if (sock < 0 || world.load() == nullptr)
			return;
		for (;;) {
			char buf[65536];
			struct sockaddr_in from {};
			socklen_t fromLen = sizeof(from);
			ssize_t n = recvfrom(sock, buf, sizeof(buf), 0,
			                     reinterpret_cast<struct sockaddr *>(&from), &fromLen);
			if (n <= 0)
				break;
			if (verbose)
				std::fprintf(stderr, "[osc] port=%d n=%d\n", (int)ntohs(from.sin_port), (int)n);
			// Bundle = "#bundle\0" in the 8 bytes; it may be EMPTY (16 bytes: tag
			// + timetag, 0 messages) → n >= 16, not n > 16. Numeric commands start
			// with 0x00, which is why buf[0..1] is not enough.
			const bool isBundle = n >= 16 && std::memcmp(buf, "#bundle\0", 8) == 0;
			if (isBundle) {
				// CAREFUL: the timetag is uint64 (NTP) read as int64 ⇒ with today's
				// date it comes out NEGATIVE through overflow; "immediate" means the
				// literal values 0 and 1 (same rule as PerformOSCPacket), never
				// "when > 1".
				const int64_t when = oscTimetag(buf + 8);
				if (when != 0 && when != 1 && when > oscNow() && schedule(buf, (size_t)n, from))
					continue;
			}
			perform(from, buf, (int)n, isBundle);
		}
	}

	// Future timetag: copy (buf is on the stack) and schedule. If it does not fit
	// in the caps, return false and the bundle runs right away (better than
	// losing it).
	bool schedule(const char *data, size_t size, const struct sockaddr_in &from)
	{
		if (pending.size() >= kMaxPending || pendingBytes + size > kMaxPendingBytes) {
			droppedScheduled.fetch_add(1);
			return false;
		}
		PendingPkt pp;
		pp.data.assign(data, data + size);
		pp.from = from;
		pendingBytes += size;
		const int64_t when = oscTimetag(data + 8);
		if (verbose)
			std::fprintf(stderr, "[osc] scheduled in %.6f s\n",
			             (double)(when - oscNow()) * 2.328306436538696e-10);
		pending.emplace(when, std::move(pp));
		return true;
	}

	void perform(const struct sockaddr_in &from, char *data, int size, bool isBundle)
	{
		reply.peer = from;
		ReplyAddress ra {};
		ra.mReplyData = &reply;
		ra.mReplyFunc = engineReplyFunc;
		World *w = world.load();
		if (!w)
			return;
		if (isBundle) {
			OSC_Packet pkt {};
			pkt.mData = data;
			pkt.mSize = size;
			pkt.mReplyAddr = ra;
			PerformOSCBundle(w, &pkt);
		} else {
			PerformOSCMessage(w, size, data, &ra);
		}
	}

	// Expired bundles: they run before World_Run (accuracy = 1 block, ~10 ms;
	// enough for NodeProxy fades, which last seconds).
	void dispatchDue()
	{
		if (pending.empty() || world.load() == nullptr)
			return;
		const int64_t now = oscNow();
		while (!pending.empty()) {
			auto it = pending.begin();
			if (it->first > now)
				break;
			if (verbose)
				std::fprintf(stderr, "[osc] dispatch %zu bytes\n", it->second.data.size());
			PendingPkt pp = std::move(it->second);
			pendingBytes -= pp.data.size();
			pending.erase(it);
			perform(pp.from, pp.data.data(), (int)pp.data.size(), true);
		}
	}

	void serviceOsc()
	{
		drainOsc();
		dispatchDue();
	}

	// --- 3.5: shared history -----------------------------------------------
	// Called at the end of boot (blockLength and buses already defined) and from
	// teardown to release it. The cursors are reset in dsp64, so it does not
	// matter that "rendered" starts at 0 on every boot.
	void setupHistory(World *w)
	{
		const long n = std::max<long>(1, w->mBufLength);
		const long buses = std::min<long>(w->mNumAudioBusChannels, Sc3Engine::kServedBuses);
		// >= 4 blocks and >= 4096 frames, a multiple of the block.
		long frames = std::max<long>(4096, n * 4);
		frames = ((frames + n - 1) / n) * n;
		std::lock_guard<std::mutex> lk(copyMtx);
		served = buses;
		histFrames = frames;
		hist.assign((size_t)served * 2 * (size_t)histFrames, 0.0f);
		rendered.store(0);
		underruns.store(0);
		renderCursor.pos = 0;
	}

	void releaseHistory()
	{
		{
			std::lock_guard<std::mutex> lkIn(inMtx);
			inActive.store(false);
			inOwner.store(nullptr);
			inBuf.clear();
			inBuf.shrink_to_fit();
			inChans = 0;
			inPrefill = 0;
			inWr = inRd = 0;
		}
		std::lock_guard<std::mutex> lk(copyMtx);
		hist.clear();
		hist.shrink_to_fit();
		histFrames = 0;
		served = 0;
		rendered.store(0);
		renderCursor.pos = 0;
	}

	// Copies the served buses out of the freshly rendered block. "bc" is the
	// mBufCounter World_Run ran with (Out compares touched == bc).
	void copyToHistory(World *w, int32 bc)
	{
		const long n = w->mBufLength;
		std::lock_guard<std::mutex> lk(copyMtx);
		if (hist.empty() || served <= 0)
			return;
		const uint64_t base = rendered.load(std::memory_order_relaxed);
		const long off = (long)(base % (uint64_t)histFrames);
		const size_t stride = (size_t)2 * (size_t)histFrames;
		for (long b = 0; b < served; ++b) {
			float *dst = hist.data() + (size_t)b * stride + off;
			if (w->mAudioBusTouched[b] == bc)
				std::memcpy(dst, w->mAudioBus + (size_t)b * n, sizeof(float) * (size_t)n);
			else
				std::memset(dst, 0, sizeof(float) * (size_t)n);
			std::memcpy(dst + histFrames, dst, sizeof(float) * (size_t)n);
		}
		rendered.store(base + (uint64_t)n, std::memory_order_release);
	}

	// 3.8: dumps the input ring into the World's INPUT buses and marks
	// `mAudioBusTouched` (In/SoundIn copy only if touched == mBufCounter).
	// With no producer (boot, DSP off, no `@ins`) nothing is touched ⇒ SoundIn
	// returns silence, just like a server with no input device.
	void feedInputLocked(World *w, int32 bc)
	{
		if (!inActive.load(std::memory_order_acquire))
			return;
		const long n = w->mBufLength;
		const int ins = (int)w->mNumInputs;
		if (ins <= 0 || n <= 0)
			return;
		const int chans = (int)std::min<long>(inChans, (long)ins);
		if (chans <= 0)
			return;
		float *base = w->mAudioBus + (size_t)w->mNumOutputs * (size_t)n;
		int32 *touched = w->mAudioBusTouched + w->mNumOutputs;
		std::lock_guard<std::mutex> lk(inMtx);
		if (inBuf.empty())
			return;
		const long avail = (long)(inWr - inRd);
		const long off = (long)(inRd % (uint64_t)kInputRingFrames);
		const long take = std::min(avail, n);
		const long first = std::min(take, kInputRingFrames - off);
		for (int ch = 0; ch < chans; ++ch) {
			const float *src = inBuf.data() + (size_t)ch * (size_t)kInputRingFrames;
			float *dst = base + (size_t)ch * (size_t)n;
			if (first > 0)
				std::memcpy(dst, src + off, sizeof(float) * (size_t)first);
			if (take > first)
				std::memcpy(dst + first, src, sizeof(float) * (size_t)(take - first));
			if (take < n)
				std::memset(dst + take, 0, sizeof(float) * (size_t)(n - take));
			touched[ch] = bc;
			if (ch < 16) {
				float pk = 0.0f;
				for (long i = 0; i < n; ++i)
					pk = std::max(pk, std::fabs(dst[i]));
				inFeedPeak[ch].store(pk, std::memory_order_relaxed);
			}
		}
		if (take >= n) {
			inRd += (uint64_t)n;
		} else {
			inRd = inWr; // underrun: the rest is dropped and playback resumes at the present
			inUnder.fetch_add(1, std::memory_order_relaxed);
		}
	}

	// One block: drain + render + advance mBufCounter (bug 4 in handoff).
	// Used by the three possible owners of the World (boot, audio, pump with DSP
	// off): a single path keeps "rendered" from drifting away from the World.
	//
	// 3.8: `feedInput` is only true when the block was asked for by the AUDIO
	// THREAD (pull). The pump with DSP off does not consume the ring: without
	// DSP there is no input (3.3) and, if it consumed it, a stopped instance
	// would eat the samples of another one that is actually in the audio chain
	// (underruns and a drifting stream).
	void advanceLocked(World *w, bool feedInput = true)
	{
		serviceOsc();
		const int32 bc = (int32)w->mBufCounter;
		if (feedInput)
			feedInputLocked(w, bc); // 3.8: input buses ready for In/SoundIn
		World_Run(w);
		copyToHistory(w, bc); // no-op during boot (empty hist)
		w->mBufCounter++;
		units.store(w->mNumUnits);
	}

	// One boot block (worker): there ready is false and nobody else touches the World.
	void pumpOnce()
	{
		World *w = world.load();
		if (!w)
			return;
		advanceLocked(w, false);
	}

	// 3.5: renders whatever is missing to cover [pos, pos+frames) and copies
	// that window from buses [bus, bus+nchan). Requires frames <= histFrames
	// (the chunking is done by Sc3Engine::pull).
	bool pullChunk(uint64_t &pos, long bus, int nchan, double **outs, long off, long frames)
	{
		const uint64_t need = pos + (uint64_t)frames;
		{
			std::lock_guard<std::mutex> lk(copyMtx);
			if (!ready.load() || hist.empty())
				return false;
			if (bus < 0 || bus + nchan > served)
				return false;
			const uint64_t cur = rendered.load(std::memory_order_acquire);
			if (need <= cur) {
				// Window already rendered: is it still alive in the history?
				if (cur - pos > (uint64_t)histFrames) {
					underruns.fetch_add(1, std::memory_order_relaxed);
					pos = cur; // jumps to the present: the next block sounds right away
					return false;
				}
				const long src0 = (long)(pos % (uint64_t)histFrames);
				const size_t stride = (size_t)2 * (size_t)histFrames;
				for (int ch = 0; ch < nchan; ++ch) {
					const float *src = hist.data() + (size_t)(bus + ch) * stride + src0;
					double *dst = outs[ch] + off;
					for (long i = 0; i < frames; ++i)
						dst[i] = (double)src[i];
				}
				pos += (uint64_t)frames;
				return true;
			}
		}

		// Missing render: try_lock (the audio thread never waits; see 3.3).
		// copyMtx is already free: the order is always worldMtx -> copyMtx, never
		// the other way round (pull does not take copyMtx while holding worldMtx...
		// see above).
		std::unique_lock<std::mutex> lk(worldMtx, std::try_to_lock);
		if (!lk || !ready.load())
			return false;
		World *w = world.load();
		if (!w)
			return false;
		lastAdvanceNs.store(nowNs(), std::memory_order_relaxed);
		// Per-call cap: two instances of the same tick render ONCE; this only
		// cuts an absurd request (one instance far ahead).
		int guard = 128;
		while (rendered.load(std::memory_order_relaxed) < need && guard-- > 0)
			advanceLocked(w);
		if (rendered.load(std::memory_order_relaxed) < need)
			return false;
		lk.unlock();
		return pullChunk(pos, bus, nchan, outs, off, frames);
	}

	static int64_t nowNs()
	{
		return (int64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
		           std::chrono::steady_clock::now().time_since_epoch())
		    .count();
	}

	// --- boot --------------------------------------------------------------
	// Points where an in-flight boot can be abandoned (release() during the
	// boot). The VM cannot be aborted halfway through compileLibrary: the first
	// useful check is after client->boot().
	bool cancelled()
	{
		if (!cancel.load())
			return false;
		st.error = "boot canceled (close during the boot)";
		return true;
	}

	bool bootSequence()
	{
		// 1. socket BEFORE boot. With `cfg.port == 0` (AUTO) we bind 0 and the
		//    kernel picks a free port: that way the World does not fight the SC
		//    IDE (sclang/scsynth always go to 57110, and their asio binds WITHOUT
		//    SO_REUSEADDR ⇒ whoever arrives second loses). The real port is passed
		//    to sclang through the environment (SC3_WORLD_PORT) BEFORE compiling
		//    the library: the `*initClass` in SCMaxCompat/9_SC3ServerPort.sc
		//    points `Server.default.addr` there, i.e. before the user's startup.scd.
		const int reqPort = cfg.port;
		sock = bindUdp(cfg.port);
		if (sock < 0) {
			st.error = "could not bind UDP 127.0.0.1:" + std::to_string(cfg.port) +
			           " — something else is using it (SuperCollider IDE / scsynth). "
			           "Quit it and send `open` again; no need to reload the patch. "
			           "With `@port 0` (default) the system picks the port and this cannot "
			           "happen.";
			return false;
		}
		{
			struct sockaddr_in bound {};
			socklen_t blen = sizeof(bound);
			if (getsockname(sock, reinterpret_cast<struct sockaddr *>(&bound), &blen) == 0 &&
			    bound.sin_port != 0)
				cfg.port = (int)ntohs(bound.sin_port);
		}
		st.port = cfg.port;
		::setenv("SC3_WORLD_PORT", std::to_string(cfg.port).c_str(), 1);
		if (verbose)
			std::fprintf(stderr, "[info] World UDP 127.0.0.1:%d (pedido %d)\n",
			             cfg.port, reqPort);
		reply.sock = sock;

		// 2. class library + UGens from the REAL SC INSTALLATION (the library is
		//    not bundled in the external). It must be the same version as the
		//    compiled sclang: the class library reaches primitives by name and the
		//    .scx are ABI-dependent.
		namespace fs = std::filesystem;

		// 2a. 3.4: user config dir. Only on the first boot: the startup file path
		//     is fixed by `Platform:initClass` (inside compileLibrary), and the
		//     Quarks yaml is read once (gLanguageConfig). Moving it later would be
		//     a lie; a reopen keeps the one from the first boot.
		if (!langBooted && !cfg.userConfigDir.empty()) {
			SC_Filesystem::instance().setDirectory(SC_Filesystem::DirName::UserConfig,
			                                      fs::path(cfg.userConfigDir));
			if (verbose)
				std::fprintf(stderr, "[info] userConfigDir = %s\n", cfg.userConfigDir.c_str());
		}
		const std::string app = cfg.scAppPath.empty() ? "/Applications/SuperCollider.app" : cfg.scAppPath;
		// 4.2 (b): escape hatch for an installation of a DIFFERENT minor version.
		// Precedence: config (`@allowscversion`) → environment (tests/CLI).
		bool allowVer = cfg.allowScVersion;
		if (!allowVer) {
			const char *e = std::getenv("SC3_ALLOW_SC_VERSION");
			allowVer = e && (*e == '1' || *e == 'y' || *e == 'Y' || *e == 't' || *e == 'T');
		}
		std::string clDir = cfg.classLibraryDir;
		if (!langBooted && clDir.empty()) {
			clDir = app + "/Contents/Resources";
			if (!fs::exists(fs::path(clDir) / "SCClassLibrary")) {
				st.error = "cannot find the SuperCollider class library at " + clDir +
				           " (install SuperCollider " + SC_VersionString() +
				           " in /Applications or pass scAppPath/classLibraryDir)";
				return false;
			}
			// 4.2 (a): the version policy is NOT string equality. Same
			// major.minor is accepted with a warning; another minor needs
			// `allowscversion`.
			const std::string engineVer = SC_VersionString();
			const std::string ver = installedScVersion(app);
			st.scVersion = ver;
			if (!ver.empty() && ver != engineVer) {
				int imaj = 0, imin = 0, emaj = 0, emin = 0;
				const bool sameMinor = versionMajorMinor(ver, &imaj, &imin) &&
				                       versionMajorMinor(engineVer, &emaj, &emin) &&
				                       imaj == emaj && imin == emin;
				if (!sameMinor && !allowVer) {
					st.error = "SuperCollider installation is " + ver +
					           " and this external ships engine " + engineVer +
					           ": change the minor version and the class library couples to "
					           "the interpreter primitives. Install SuperCollider " +
					           engineVer + " (or any " +
					           engineVer.substr(0, engineVer.rfind('.')) +
					           ".x), or force startup with `@allowscversion 1` "
					           "(env SC3_ALLOW_SC_VERSION=1) to try anyway.";
					return false;
				}
				st.versionNote =
				    sameMinor
				        ? "SC installation " + ver + " with engine " + engineVer +
				              " (same minor version: accepted; the 4.2 smoke test verifies "
				              "primitives and UGens at startup)"
				        : "SC installation " + ver + " with engine " + engineVer +
				              " FORCED with allowscversion (different versions): if anything "
				              "fails, the most likely cause is a primitive or a UGen the "
				              "interpreter does not have";
			}
		} else if (!langBooted) {
			st.scVersion = installedScVersion(app);
			// 3.4: with an explicit classLibraryDir (fixture, Phase 4 bundle) it has
			// to be validated here: otherwise sclang fails with "the library has not
			// been compiled successfully" and it is not obvious that the path is the
			// problem.
			if (!fs::exists(fs::path(clDir) / "SCClassLibrary")) {
				st.error = "cannot find the class library at " + clDir +
				           " (SCClassLibrary/ is missing there)";
				return false;
			}
		}
		if (!langBooted) {
			st.classLibraryDir = clDir;
			SC_Filesystem::instance().setDirectory(SC_Filesystem::DirName::Resource, clDir);
		}

		// 2b. the Qt-less module as an extra include dir of the class library (10 KB
		//     of .sc: the library is not bundled, only the initClassTree override
		//     + the GUI/Server stubs). Added BEFORE compiling; compileLibrary only
		//     reads the config if gLanguageConfig is nullptr.
		if (!cfg.compatDir.empty()) {
			if (!fs::is_directory(cfg.compatDir)) {
				st.error = "SCMaxCompat module not found in " + cfg.compatDir;
				return false;
			}
			if (!gLanguageConfig)
				SC_LanguageConfig::readLibraryConfig(cfg.withUserQuarks);
			if (gLanguageConfig)
				gLanguageConfig->addIncludedDirectory(cfg.compatDir);
		}

		// 3. sclang (worker): initRuntime only the first time; on reopens the
		//    library is recompiled (compileLibrary already does shutdownLibrary
		//    inside).
		if (!client) {
			client = new Sc3LangClient("sc3-engine");
			// bug #13: the VM survives close ⇒ it has to be shut down on process
			// exit, and BEFORE SC's static destructors (if the process ends with the
			// VM alive, `libc++abi: terminating` while destroying statics with SC
			// threads still running).
			static bool exitHook = false;
			if (!exitHook) {
				exitHook = true;
				std::atexit([] { Sc3Engine::instance().shutdownLang(); });
			}
		}
		// The fallback to stock must not take the compat module down with it.
		if (!cfg.compatDir.empty() && client->mKeepPaths.empty())
			client->mKeepPaths.push_back(cfg.compatDir);
		if (!client->boot(cfg.withUserQuarks)) {
			st.error = "sclang: the class library does not compile (neither stock nor stock + SCMaxCompat)";
			return false;
		}
		langBooted = true;
		st.degraded = client->mDegraded;
		st.degradeReport = client->mDegradeReport;
		if (cancelled())
			return false;

		// 4. s.options del usuario
		Sc3LangClient::ServerOpts so = client->readServerOptions();
		st.maxNodes = so.maxNodes;
		st.memSizeKB = so.memSize;
		st.numWireBufs = so.numWireBufs;
		st.numRGens = so.numRGens;
		st.numBuffers = so.numBuffers;
		st.numAudioBusChannels = so.numAudioBusChannels;
		st.numInputBusChannels = so.numInputBusChannels;
		st.numOutputBusChannels = so.numOutputBusChannels;
		st.blockSize = so.blockSize;
		st.maxLogins = so.maxLogins;
		st.sampleRateSet = so.sampleRateSet;

		// The World listens on cfg.port; sclang sends its commands (d_recv, s_new,
		// sync…) to Server.default.port. If they do not match, the audio goes to
		// ANOTHER server and nothing arrives here: the boot "works" (something
		// answers /notify) but units stays at 0 forever. Fail fast instead of
		// adopting a mute World.
		if (so.port > 0 && so.port != cfg.port) {
			st.error = "the engine listens on port " + std::to_string(cfg.port) +
			           " but sclang's Server points at " + std::to_string(so.port) +
			           " (sclang would send its commands to another server)";
			return false;
		}

		double sr = cfg.preferredSampleRate > 0.0 ? cfg.preferredSampleRate : 0.0;
		const bool srFromHost = sr > 0.0; // 3.2: the Max DSP chain asked for it
		if (sr <= 0.0)
			sr = so.sampleRateSet ? so.sampleRate : 48000.0;
		st.sampleRateFromHost = srFromHost;
		// 3.2: align sclang with the World. If the World runs at 44100 and
		// s.options.sampleRate says 48000, `s.sampleRate` lies and everything that
		// computes durations in frames (Buffer:read, `numFrames`, BufDur…) comes
		// out 8.8 % off. The World is the authority (it is the one rendering).
		if (srFromHost && std::fabs(so.sampleRate - sr) > 0.5) {
			client->evaluate("s.options.sampleRate = " +
			                 std::to_string((long)std::llround(sr)) + ";");
			if (verbose)
				std::fprintf(stderr, "[info] s.options.sampleRate %.0f -> %.0f (World)\n",
				             so.sampleRate, sr);
		}
		if (verbose) {
			std::fprintf(stderr,
			             "[info] s.options: maxNodes=%ld memSize=%ldKB numWireBufs=%ld "
			             "numRGens=%ld numBuffers=%ld buses=%ld (in=%ld out=%ld) "
			             "blockSize=%ld maxLogins=%ld sr=%.0f%s\n",
			             so.maxNodes, so.memSize, so.numWireBufs, so.numRGens, so.numBuffers,
			             so.numAudioBusChannels, so.numInputBusChannels, so.numOutputBusChannels,
			             so.blockSize, so.maxLogins, sr, so.sampleRateSet ? "" : " (default)");
		}

		if (cancelled())
			return false;

		// 5. World with those options
		WorldOptions opt;
		opt.mRealTime = false;
		opt.mPreferredSampleRate = sr;
		opt.mBufLength = (uint32)std::max<long>(so.blockSize, 1);
		// Bus consistency (bug 7): mAudioBus is allocated with
		// mBufLength * mNumAudioBusChannels; with fewer buses than in+out the UGens
		// write past the allocation.
		const long buses = std::max<long>(so.numAudioBusChannels,
		                                 so.numInputBusChannels + so.numOutputBusChannels);
		opt.mNumAudioBusChannels = (uint32)buses;
		opt.mNumInputBusChannels = (uint32)so.numInputBusChannels;
		opt.mNumOutputBusChannels = (uint32)so.numOutputBusChannels;
		opt.mNumBuffers = (uint32)std::max<long>(so.numBuffers, 8);
		opt.mMaxNodes = (uint32)std::max<long>(so.maxNodes, 1024);
		opt.mMaxGraphDefs = (uint32)std::max<long>(so.maxSynthDefs, 1024);
		opt.mRealTimeMemorySize = (uint32)std::max<long>(so.memSize, 8192);
		opt.mMaxWireBufs = (uint32)std::max<long>(so.numWireBufs, 64);
		opt.mNumRGens = (uint32)std::max<long>(so.numRGens, 64);
		// <= 32 or ServerOptions:maxLogins throws and `notified` is never set (bug 2).
		opt.mMaxLogins = (uint32)std::min<long>(std::max<long>(so.maxLogins, 1), 32);
		opt.mVerbosity = verbose ? 1 : 0;

		std::string ugens = cfg.ugenPluginsPath;
		if (ugens.empty()) {
			ugens = app + "/Contents/Resources/plugins";
			if (!fs::is_directory(ugens))
				ugens = ""; // no stock: only the user dir
			const std::string user = userPluginsDir();
			if (!user.empty()) {
				if (!ugens.empty())
					ugens += ':';
				ugens += user;
			}
		}
		st.ugensPath = ugens;
		opt.mUGensPluginPath = ugens.c_str();
		if (verbose)
			std::fprintf(stderr, "[info] mUGensPluginPath = %s\n", ugens.c_str());

		// 3.4: plugin load errors (and World_New ones) are posted by scsynth through
		// `scprintf` ⇒ route them to the `error` outlet instead of stdout.
		SetPrintFunc(sc3WorldPrint);

		World *w = World_New(&opt);
		if (!w) {
			st.error = "World_New failed (out of memory, or invalid options?)";
			return false;
		}
		world.store(w);
		World_SetSampleRate(w, sr);
		World_Start(w);
		lastAdvanceNs.store(nowNs());

		// bug #14b: atexit handlers run in REVERSE registration order, and
		// World_New dlopens the .scx => their static destructors get registered
		// after the shutdownLang hook => on exit they would run before it, with a
		// plugin's ~std::thread (e.g. UIUGens' uiListenThread) still joinable
		// => libc++abi: terminating. Registering the hook again HERE (with the
		// plugins already loaded) puts it first in the queue.
		// shutdownLang() is idempotent.
		static bool exitHookAfterWorld = false;
		if (!exitHookAfterWorld) {
			exitHookAfterWorld = true;
			std::atexit([] { Sc3Engine::instance().shutdownLang(); });
		}

		st.sampleRate = sr;
		st.bufLength = w->mBufLength;
		st.worldBuses = w->mNumAudioBusChannels;
		// 3.5: shared history for the audio cursors (buses 0..63).
		setupHistory(w);
		st.worldSndBufs = w->mNumSndBufs;
		st.worldRGens = w->mNumRGens;

		// 6. adoption: do the same thing as ServerStatus:prFinalizeBoot (the
		//    /notify reply path) but without depending on /notify, because on
		//    successive boots of the same process that reply never arrives. The
		//    fork on AppClock is necessary: s.sync waits for the World's /sync, and
		//    that only advances if the interpreter keeps processing OSC (done
		//    inline it would deadlock).
		//    serverRunning_ is the public setter of `hasBooted` (getter-only) and
		//    `notified` closes s.serverRunning = hasBooted and: notified.
		// 3.4: do NOT throw away the user's startup text. Up to here the post and
		// the errors from the startup file (and from Quarks) were discarded when
		// clearing the client buffer for adoption ⇒ a failing `startup.scd` was
		// left mute. They are drained to the outlet and only afterwards is what
		// the adoption block produced cleared.
		appendDrained();
		client->clearPost();
		client->evaluate(
		    "fork({\n"
		    "  s.statusWatcher.serverRunning = true;\n"
		    "  s.sendDefaultGroups;\n"
		    "  ServerBoot.run(s);\n"
		    "  s.sync;\n"
		    "  ServerTree.run(s);\n"
		    "  s.statusWatcher.notified = true;\n"
		    "  s.changed(\\serverRunning);\n"
		    "}, AppClock);");

		auto waitRunning = [&](int ms) {
			const auto t0 = std::chrono::steady_clock::now();
			bool ok = false;
			for (int i = 0; !ok; ++i) {
				pumpOnce();
				client->tick();
				if (cancelled())
					return false;
				if ((i % 4) == 0) {
					// Read the LIVE value without losing the startup posts.
					std::string keep = client->mPost;
					client->mPost.clear();
					client->evaluate("s.serverRunning.postln;");
					ok = client->mPost.find("true") != std::string::npos;
					client->mPost = keep;
				}
				if (std::chrono::duration_cast<std::chrono::milliseconds>(
				        std::chrono::steady_clock::now() - t0)
				        .count() > ms)
					break;
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
			return ok;
		};

		bool running = waitRunning(1500);
		if (!running) {
			// Deterministic fallback: if the "parity" path did not complete (the
			// fork's sync never came back), adopt by hand: that is enough for
			// Synth() to have its group 1 and for s.serverRunning to be true.
			client->evaluate("s.statusWatcher.serverRunning = true;\n"
			                 "s.sendDefaultGroups;\n"
			                 "s.statusWatcher.notified = true;");
			running = waitRunning(1000);
		}
		st.serverRunning = running;
		if (!running)
			st.error = "Server adoption failed (s.serverRunning = false)";
		// let the World process sendDefaultGroups' /g_new
		for (int i = 0; i < 8; ++i) {
			pumpOnce();
			client->tick();
		}

		// drain whatever the boot left behind (startup posts, Quarks errors…)
		appendDrained();
		if (!running)
			return false;

		// 4.2 (c): the Info.plist version proves nothing by itself — this does.
		// Building a SynthDef exercises the interpreter (classes + primitives of
		// the INSTALLED class library) and sends the /d_recv to the World (.scx
		// UGens). No nodes and no audio: nothing is left hanging. If the
		// installation were incompatible, the error shows up here, with both
		// versions in the message.
		{
			client->clearPost();
			client->evaluate(
			    "SynthDef(\\sc3_smoke, { Out.ar(0, SinOsc.ar(440, 0, 0)) }).add;", false);
			for (int i = 0; i < 8; ++i) {
				pumpOnce();
				client->tick();
				if (!client->mErr.empty())
					break;
			}
			const std::string smokeErr = client->mErr;
			appendDrained();
			st.smokeOk = smokeErr.empty();
			if (!st.smokeOk) {
				st.error = "compatibility smoke test failed (SC installation " +
				           (st.scVersion.empty() ? std::string("(?)") : st.scVersion) +
				           ", engine " + SC_VersionString() +
				           "): the interpreter could not build a minimal SynthDef. " +
				           "Most likely cause: primitives of the installed class library "
				           "that this engine does not have. → " + smokeErr;
				return false;
			}
		}
		return true;
	}

	// --- worker: sole owner of the sclang VM ---------------------------------
	void workerMain()
	{
		const auto t0 = std::chrono::steady_clock::now();
		bootOk = bootSequence();
		bootMs.store((long)std::chrono::duration_cast<std::chrono::milliseconds>(
		                 std::chrono::steady_clock::now() - t0)
		                 .count());
		if (cancel.load())
			bootOk = false;
		{
			std::lock_guard<std::mutex> lk(mtx);
			bootDone = true;
		}
		// Order: "ready" first (what the audio looks at), then the state that is
		// reported. If it was cancelled, the engine stays Idle (closed).
		ready.store(bootOk);
		state.store(bootOk ? kBootReady : (cancel.load() ? kBootIdle : kBootFailed));
		cv.notify_all();

		for (;;) {
			Job job;
			bool has = false;
			{
				std::unique_lock<std::mutex> lk(mtx);
				cv.wait_for(lk, std::chrono::milliseconds(2),
				            [this] { return stop || !queue.empty(); });
				if (!queue.empty()) {
					job = std::move(queue.front());
					queue.pop_front();
					has = true;
				} else if (stop) {
					break;
				}
			}
			if (has && client) {
				if (!job.setup.empty())
					client->evaluate(job.setup, false);
				client->evaluate(job.code, job.print);
			}
			if (client)
				client->tick();
			appendDrained();
		}

		// sclang (client) SURVIVES close: initRuntime happens once per process
		// (bug #13) and the next open only recompiles the library. The Max
		// process ends with the VM alive, just like the IDE.
		appendDrained();
	}
};

// --------------------------------------------------------------- singleton

Sc3Engine &Sc3Engine::instance()
{
	static Sc3Engine engine;
	return engine;
}

Sc3Engine::Sc3Engine() : m(new Impl) {}
Sc3Engine::~Sc3Engine()
{
	release();
}

bool Sc3Engine::open(const Sc3EngineConfig &cfg)
{
	Impl &s = *m;
	// A previous attempt failed (port busy, broken library…): everything is
	// cleared and we start from scratch. The user just has to send `open` again.
	if (s.state.load() == kBootFailed)
		teardown();

	std::lock_guard<std::mutex> lk(s.mtx);
	const int boot = s.state.load();
	s.refs++;
	if (boot == kBootBooting || boot == kBootReady)
		return true; // a boot is already running / done (shared, refcount)

	// 3.1: start and RETURN. The worker publishes the result by itself.
	s.cfg = cfg;
	s.verbose = cfg.verbose;
	s.stop = false;
	s.cancel.store(false);
	s.bootDone = false;
	s.bootOk = false;
	s.bootMs.store(0);
	s.state.store(kBootBooting);
	s.worker = std::thread([&s] { s.workerMain(); });
	return true;
}

bool Sc3Engine::waitReady(int timeoutMs)
{
	Impl &s = *m;
	std::unique_lock<std::mutex> lk(s.mtx);
	if (s.state.load() == kBootReady)
		return true;
	if (s.state.load() != kBootBooting)
		return false;
	return s.cv.wait_for(lk, std::chrono::milliseconds(timeoutMs),
	                     [&s] { return s.bootDone; }) &&
	       s.bootOk;
}

void Sc3Engine::release()
{
	Impl &s = *m;
	{
		std::unique_lock<std::mutex> lk(s.mtx);
		if (s.refs <= 0)
			return;
		if (--s.refs > 0)
			return;
		s.stop = true;
		s.cancel.store(true); // if it is still booting, cut between steps
		s.ready.store(false); // the audio thread stops touching the World
		s.cv.notify_all();
	}
	teardown();
}

void Sc3Engine::teardown()
{
	Impl &s = *m;
	{
		std::lock_guard<std::mutex> lk(s.mtx);
		s.stop = true;
		s.cancel.store(true);
		s.ready.store(false);
		s.queue.clear(); // code queued by a boot that no longer exists
	}
	s.cv.notify_all();

	if (s.worker.joinable())
		s.worker.join();

	// 3.3: nobody else touches the World. `ready` is already false (audio no
	// longer enters) and taking worldMtx guarantees there is no render/pump in
	// flight and that a new one cannot start (both paths re-check `ready` with
	// the lock held). We wait at most one block + the pump budget.
	{
		std::lock_guard<std::mutex> lk(s.worldMtx);
		// BUG #13 — `unload_plugins` MUST be FALSE. `World_New` initializes the
		// UGen library once per process (`static bool gLibInitted` in
		// SC_World.cpp:315), so if the .scx are unloaded here
		// (`deinitialize_library()` → `dlclose`), the next `World_New` does NOT
		// re-initialize it: the UnitDefs are left pointing at unloaded code and
		// the first `Group_Calc` jumps into freed memory (SEGV in `World_Run`).
		// Verified with ASan: `Group_Calc → unknown pc` on the 2nd boot.
		// Plugins live as long as the process (same as sclang, see `langBooted`).
		if (World *w = s.world.exchange(nullptr))
			World_Cleanup(w, false);
		s.releaseHistory(); // nadie puede estar leyendo: ready=false + worldMtx
	}
	if (s.sock >= 0) {
		close(s.sock);
		s.sock = -1;
		s.reply.sock = -1;
	}
	s.pending.clear(); // bundles scheduled by a World that no longer exists
	s.pendingBytes = 0;
	s.units.store(0);
	s.bootMs.store(0);
	s.lastAdvanceNs.store(0);
	s.pumpedBlocks.store(0);
	s.pumpLagBlocks.store(0);
	{
		// Everything that describes sclang is kept (it lives as long as the
		// process); whatever belongs to the World/socket of this cycle is reset.
		std::lock_guard<std::mutex> lk(s.outMtx);
		const std::string cl = s.st.classLibraryDir, ver = s.st.scVersion,
		                  degrade = s.st.degradeReport;
		const bool degraded = s.st.degraded;
		s.st = Sc3EngineStatus {};
		s.st.classLibraryDir = cl;
		s.st.scVersion = ver;
		s.st.degradeReport = degrade;
		s.st.degraded = degraded;
	}
	s.state.store(kBootIdle); // permite volver a hacer open()
}

bool Sc3Engine::isReady() const
{
	return m->state.load() == kBootReady;
}

bool Sc3Engine::isBooting() const
{
	return m->state.load() == kBootBooting;
}

Sc3EngineStatus Sc3Engine::status() const
{
	Impl &s = *m;
	// 3.1: during boot the worker writes `st` (s.options, strings) with no lock.
	// Nobody used to read it at that moment; now sc3_flush polls every 100 ms,
	// so while state == Booting `st` is left alone (avoids reading a half-written
	// std::string). The remaining values arrive at the end.
	const int boot = s.state.load();
	if (boot == kBootBooting) {
		Sc3EngineStatus st;
		st.booting = true;
		st.bootMs = s.bootMs.load();
		return st;
	}
	Sc3EngineStatus st;
	{
		std::lock_guard<std::mutex> lk(s.outMtx);
		st = s.st;
	}
	st.ready = (boot == kBootReady);
	st.booting = (boot == kBootBooting);
	st.failed = (boot == kBootFailed);
	st.bootMs = s.bootMs.load();
	st.numUnits = s.units.load();
	st.serverRunning = st.serverRunning && st.ready;
	return st;
}

void Sc3Engine::eval(const std::string &code)
{
	Impl &s = *m;
	if (code.empty())
		return;
	{
		std::lock_guard<std::mutex> lk(s.mtx);
		s.queue.push_back({code, std::string(), true});
	}
	s.cv.notify_all();
}

void Sc3Engine::evalSilent(const std::string &code)
{
	Impl &s = *m;
	if (code.empty())
		return;
	{
		std::lock_guard<std::mutex> lk(s.mtx);
		s.queue.push_back({code, std::string(), false});
	}
}

void Sc3Engine::evalWithSetup(const std::string &setup, const std::string &code)
{
	Impl &s = *m;
	if (code.empty())
		return;
	{
		std::lock_guard<std::mutex> lk(s.mtx);
		s.queue.push_back({code, setup, true});
	}
	s.cv.notify_all();
}

// Takes from `buf` up to maxChars (at a \n when possible) and returns that.
static std::string takePart(std::string &buf, size_t maxChars)
{
	if (maxChars == 0 || buf.size() <= maxChars) {
		std::string out;
		out.swap(buf);
		return out;
	}
	size_t cut = buf.rfind('\n', maxChars);
	cut = (cut == std::string::npos) ? maxChars : cut + 1;
	std::string out = buf.substr(0, cut);
	buf.erase(0, cut);
	return out;
}

std::string Sc3Engine::takePost(size_t maxChars)
{
	Impl &s = *m;
	std::lock_guard<std::mutex> lk(s.outMtx);
	return takePart(s.post, maxChars);
}

std::string Sc3Engine::takeError(size_t maxChars)
{
	Impl &s = *m;
	std::lock_guard<std::mutex> lk(s.outMtx);
	return takePart(s.err, maxChars);
}

long Sc3Engine::blockLength() const
{
	Impl &s = *m;
	return s.ready.load() ? s.st.bufLength : 0;
}

long Sc3Engine::numUnits() const
{
	return m->units.load();
}

bool Sc3Engine::renderBlock(float *outL, float *outR)
{
	Impl &s = *m;
	if (!outL || !outR)
		return false;
	const long n = blockLength();
	if (n <= 0)
		return false;
	// 3.5: same path as the instances (buses 0/1, own cursor) ⇒ what the gate
	// measures is exactly what comes out of a [sc3~] with no arguments.
	s.renderTmp.resize((size_t)n * 2);
	double *outs[2] = {s.renderTmp.data(), s.renderTmp.data() + n};
	if (!pull(s.renderCursor, 0, 2, outs, n))
		return false;
	for (long i = 0; i < n; ++i) {
		outL[i] = (float)outs[0][i];
		outR[i] = (float)outs[1][i];
	}
	return true;
}

// ------------------------------------------------------ 3.5: multi-instancia

long Sc3Engine::servedBuses() const
{
	Impl &s = *m;
	if (!s.ready.load())
		return 0;
	std::lock_guard<std::mutex> lk(s.copyMtx);
	return s.served;
}

uint64_t Sc3Engine::renderedSamples() const
{
	return m->rendered.load(std::memory_order_acquire);
}

long Sc3Engine::underruns() const
{
	return m->underruns.load();
}

void Sc3Engine::resetCursor(Sc3AudioCursor &c) const
{
	c.pos = m->rendered.load(std::memory_order_acquire);
}

bool Sc3Engine::pull(Sc3AudioCursor &c, long bus, int nchan, double **outs, long frames)
{
	Impl &s = *m;
	if (!outs || nchan <= 0 || frames <= 0 || bus < 0)
		return false;
	if (!s.ready.load())
		return false;
	// A window larger than the history is split into contiguous chunks (Max can
	// ask for signal vectors of up to 8192; the history is >= 4096).
	long done = 0;
	while (done < frames) {
		long chunk;
		{
			std::lock_guard<std::mutex> lk(s.copyMtx);
			if (s.hist.empty())
				return false;
			chunk = std::min<long>(frames - done, s.histFrames);
		}
		if (!s.pullChunk(c.pos, bus, nchan, outs, done, chunk))
			return false;
		done += chunk;
	}
	return true;
}

// ------------------------------------------------- 3.8: audio input

bool Sc3Engine::claimInput(const void *owner, int nchan)
{
	Impl &s = *m;
	if (!owner || nchan <= 0)
		return false;
	const void *cur = s.inOwner.load(std::memory_order_acquire);
	if (cur && cur != owner)
		return false; // another `sc3~` is already producing this World's input
	std::lock_guard<std::mutex> lk(s.inMtx);
	const long chans = std::min<long>(nchan, 16);
	const size_t want = (size_t)chans * (size_t)Impl::kInputRingFrames;
	if (s.inBuf.size() != want) {
		s.inBuf.assign(want, 0.0f);
	} else {
		std::fill(s.inBuf.begin(), s.inBuf.end(), 0.0f);
	}
	World *w = s.world.load();
	const long blk = (w && w->mBufLength > 0) ? (long)w->mBufLength : 64;
	// Prefill = latency of the input path: 2 blocks (~2.7 ms @48k) are enough to
	// absorb the up-to-one-block offset introduced by chunking.
	s.inChans = chans;
	s.inPrefill = std::min<long>(Impl::kInputRingFrames / 4, std::max<long>(2 * blk, 128));
	s.inRd = 0;
	s.inWr = (uint64_t)s.inPrefill; // the ring starts silent
	s.inOwner.store(owner, std::memory_order_release);
	s.inActive.store(true, std::memory_order_release);
	s.inPushed.store(0, std::memory_order_release);
	s.inNumins.store(0);
	for (int i = 0; i < 16; ++i) {
		s.inPushPeak[i].store(0.0f);
		s.inFeedPeak[i].store(0.0f);
	}
	s.inSkipped.store(0);
	s.inUnder.store(0);
	s.inOver.store(0);
	return true;
}

void Sc3Engine::releaseInput(const void *owner)
{
	Impl &s = *m;
	if (s.inOwner.load(std::memory_order_acquire) != owner)
		return;
	std::lock_guard<std::mutex> lk(s.inMtx);
	s.inActive.store(false, std::memory_order_release);
	s.inOwner.store(nullptr, std::memory_order_release);
}

bool Sc3Engine::pushInput(const void *owner, double *const *ins, int nchan, long frames)
{
	Impl &s = *m;
	if (!s.inActive.load(std::memory_order_acquire) ||
	    s.inOwner.load(std::memory_order_acquire) != owner)
		return false; // another object is the producer (or nobody claimed the input)
	if (!ins || frames <= 0 || s.inChans <= 0)
		return false;
	std::unique_lock<std::mutex> lk(s.inMtx, std::try_to_lock);
	if (!lk) {
		// The render (or the pump with DSP off) holds the ring: it skips the block
		// instead of blocking the audio thread. With a single World this does not
		// happen in practice.
		s.inSkipped.fetch_add(1, std::memory_order_relaxed);
		return false;
	}
	long n = frames;
	long skip = 0;
	if (n > Impl::kInputRingFrames) { // we keep the tail
		skip = n - Impl::kInputRingFrames;
		n = Impl::kInputRingFrames;
	}
	const long avail = (long)(s.inWr - s.inRd);
	const long space = Impl::kInputRingFrames - avail;
	if (space < n) { // ring full: the oldest samples are dropped
		const long drop = n - space;
		s.inRd += (uint64_t)drop;
		s.inOver.fetch_add(drop, std::memory_order_relaxed);
	}
	const long off = (long)(s.inWr % (uint64_t)Impl::kInputRingFrames);
	const long first = std::min(n, Impl::kInputRingFrames - off);
	for (long ch = 0; ch < s.inChans; ++ch) {
		float *dst = s.inBuf.data() + (size_t)ch * (size_t)Impl::kInputRingFrames;
		const double *src = (ch < nchan) ? ins[ch] : nullptr;
		if (!src) { // inlet not connected (or a channel this instance does not take)
			if (first > 0)
				std::memset(dst + off, 0, sizeof(float) * (size_t)first);
			if (n > first)
				std::memset(dst, 0, sizeof(float) * (size_t)(n - first));
			continue;
		}
		float pk = 0.0f;
		for (long i = 0; i < first; ++i) {
			const float v = (float)src[skip + i];
			dst[off + i] = v;
			pk = std::max(pk, std::fabs(v));
		}
		for (long i = first; i < n; ++i) {
			const float v = (float)src[skip + i];
			dst[i - first] = v;
			pk = std::max(pk, std::fabs(v));
		}
		if (ch < 16)
			s.inPushPeak[ch].store(pk, std::memory_order_relaxed);
	}
	s.inNumins.store(nchan, std::memory_order_relaxed);
	s.inWr += (uint64_t)n;
	s.inPushed.fetch_add((uint64_t)n, std::memory_order_relaxed);
	return true;
}

int Sc3Engine::inputNumins() const
{
	return (int)m->inNumins.load(std::memory_order_relaxed);
}

float Sc3Engine::inputPushPeak(int ch) const
{
	if (ch < 0 || ch >= 16)
		return 0.0f;
	return m->inPushPeak[ch].load(std::memory_order_relaxed);
}

float Sc3Engine::inputFeedPeak(int ch) const
{
	if (ch < 0 || ch >= 16)
		return 0.0f;
	return m->inFeedPeak[ch].load(std::memory_order_relaxed);
}

int Sc3Engine::inputChannels() const
{
	Impl &s = *m;
	if (!s.ready.load())
		return 0;
	World *w = s.world.load();
	return w ? (int)w->mNumInputs : 0;
}

uint64_t Sc3Engine::inputPushed() const
{
	return m->inPushed.load(std::memory_order_acquire);
}

long Sc3Engine::inputSkipped() const
{
	return m->inSkipped.load(std::memory_order_relaxed);
}

long Sc3Engine::inputUnderruns() const
{
	return m->inUnder.load(std::memory_order_relaxed);
}

long Sc3Engine::inputOverruns() const
{
	return m->inOver.load(std::memory_order_relaxed);
}

int Sc3Engine::pumpWhenIdle(int budgetMs)
{
	Impl &s = *m;
	// Same lock as audio, also try_lock: if `perform64` is inside the World, this
	// tick does nothing (the DSP is advancing on its own).
	std::unique_lock<std::mutex> lk(s.worldMtx, std::try_to_lock);
	if (!lk || !s.ready.load())
		return 0;
	World *w = s.world.load();
	if (!w)
		return 0;

	const int64_t now = Impl::nowNs();
	int64_t delta = now - s.lastAdvanceNs.load(std::memory_order_relaxed);
	// DSP on→off transition, or the UI stuck for a while: we do not want the World
	// to jump forward all at once. Capped at 250 ms (~190 blocks at most).
	constexpr int64_t kMaxDeltaNs = 250LL * 1000 * 1000;
	if (delta < 0 || delta > kMaxDeltaNs)
		delta = kMaxDeltaNs;
	s.lastAdvanceNs.store(now, std::memory_order_relaxed);

	const double sr = s.st.sampleRate > 0 ? (double)s.st.sampleRate : 48000.0;
	const long bufLen = std::max<long>(1, w->mBufLength);
	int want = (int)((double)delta * 1e-9 * sr / (double)bufLen);
	if (want <= 0)
		return 0;
	if (want > 1024)
		want = 1024; // techo duro (delta ya acotado: defensivo)

	const auto t0 = std::chrono::steady_clock::now();
	int did = 0;
	for (; did < want; ++did) {
		// drain + dispatch due timetags (3.6) + render + mBufCounter++. With DSP off
		// the history fills up anyway: if an instance is listening (mixed chain),
		// its stream stays continuous.
		s.advanceLocked(w, false);
		if (std::chrono::duration_cast<std::chrono::milliseconds>(
		        std::chrono::steady_clock::now() - t0)
		        .count() >= budgetMs) {
			++did;
			break;
		}
	}
	s.units.store(w->mNumUnits);
	s.pumpedBlocks.fetch_add(did, std::memory_order_relaxed);
	if (did < want)
		s.pumpLagBlocks.fetch_add(want - did, std::memory_order_relaxed);
	return did;
}

// Shutdown of the sclang VM. Called from atexit (not from release: the VM lives
// as long as the process, see bug #13).
void Sc3Engine::shutdownLang()
{
	Impl &s = *m;
	release(); // if it was left open: join the worker + World
	{
		std::lock_guard<std::mutex> lk(s.mtx);
		s.stop = true;
	}
	if (s.worker.joinable())
		s.worker.join();
	if (s.client) {
		s.client->shutdownLibrary();
		s.client->shutdownRuntime();
		delete s.client;
		s.client = nullptr;
	}
	s.langBooted = false;

	// BUG #14 — the .scx loaded by scsynth stay alive until here (on purpose:
	// `World_Cleanup(…, false)`, otherwise the 2nd World uses unloaded UnitDefs,
	// #13). But several plugins have threads of their own in statics (e.g.
	// `UIUGens_supernova.scx` → `static std::thread uiListenThread`, which ships
	// in the stock SC installation and gets loaded anyway). If the process ends
	// with those threads alive, dyld finalizes the image on exit and
	// `~std::thread` on a joinable thread ⇒ `std::terminate` (symptom:
	// "libc++abi: terminating" on close). `deinitialize_library()` calls each
	// plugin's `unload` (which stops the thread) and then does dlclose — exactly
	// what scsynth does on exit. It has to be the LAST thing: `World_New` does not
	// re-initialize the library (`static bool gLibInitted`), so after this there
	// are no more Worlds in this process.
	deinitialize_library();
}

long Sc3Engine::pumpedBlocks() const
{
	return m->pumpedBlocks.load();
}

long Sc3Engine::pumpLagBlocks() const
{
	return m->pumpLagBlocks.load();
}
