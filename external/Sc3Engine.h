// SPDX-License-Identifier: GPL-3.0-only
// Sc3Engine.h — embedded SuperCollider engine (sclang + World) for the sc3~
// external and for the headless tests. No Max dependencies: it compiles the same
// way in `engine_test` (bare binary) and inside the bundle (Phase 2.3+).
//
// Threading model (hard project rule, see AGENTS.md / PLAN.md):
//
//   thread               | owns                        | does
//   ---------------------+-----------------------------+--------------------------------
//   audio (perform64)    | World (once ready)          | drain OSC → Perform* → World_Run
//                        |                             | → mBufCounter++ → copy bus
//   worker (this module) | sclang VM                   | boot, evaluate, tick, drain posts
//   main (Max)           | nothing of the engine       | open/close/eval (enqueue), outlets
//
// During the boot the worker is the one touching the World (its own pump, same as
// phase1/test_world): the audio thread does not see it yet because `ready` is
// false and renderBlock() returns 0 without touching anything. When the boot ends
// `ready` is published and from then on only the audio thread drains/renders;
// `close()` waits until there are no renders in flight before destroying it.
#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct World;

// ------------------------------------------------------------------ config

struct Sc3EngineConfig {
	// World UDP port (and Server.default's inside sclang, see bootSequence).
	// 0 = AUTO: binds 0 and the kernel picks a free port ⇒ sc3~ and the SC IDE
	// (which always uses 57110) can coexist. To force the classic one: 57110.
	// The object exposes it as `@port`.
	int port = 0;

	// REAL SuperCollider installation (its library is not packaged in the
	// external): the class library and the stock UGens come from here. The
	// version must be the same **minor** version (3.14.x) as the compiled sclang
	// (3.14.1): a patch change is accepted (4.2), a different minor is not.
	std::string scAppPath;       // default: /Applications/SuperCollider.app
	std::string classLibraryDir; // default: <scAppPath>/Contents/Resources
	std::string ugenPluginsPath; // default: <scAppPath>/Contents/Resources/plugins
	                             //          + ~/Library/Application Support/SuperCollider/Extensions

	// Qt-free module (SCMaxCompat): added as an include dir of the class library.
	// Without it, a Quark with a GUI initClass (Mouse, etc.) aborts the initClass
	// tree and s.boot/s.quit are left unstubbed. Empty = no module.
	std::string compatDir;

	// 3.4: user config dir (startup.scd + sclang_conf.yaml). Empty = the real one
	// (~/Library/Application Support/SuperCollider) ⇒ the external uses whatever
	// the user already has, like the IDE. The tests point it to a tmp dir to
	// inject a broken startup / a broken Quark without touching the user config.
	// It only applies on the first boot of the process:
	// `Platform:defaultStartupFile` is fixed in Platform's initClass (inside
	// compileLibrary).
	std::string userConfigDir;

	// 3.2: World sample rate. sc3~ passes Max's DSP chain sample rate here
	// (sys_getsr, which also works with DSP off). 0 ⇒ whatever s.options says (or
	// 48000). The World is the authority: sclang is aligned to this value during
	// the boot.
	double preferredSampleRate = 0.0;
	bool withUserQuarks = false; // false = use the user config (Quarks + startup.scd)
	bool verbose = false;        // OSC trace / posts to stderr

	// 4.2 (b): escape hatch for an installation of ANOTHER minor version (3.13,
	// 3.15…). Without it the boot fails with an actionable error; with it the boot
	// proceeds anyway and the warning lands in `versionNote`. It is also enabled
	// by the SC3_ALLOW_SC_VERSION=1 environment variable (exposed by `sc3~` as
	// `@allowscversion 1`).
	bool allowScVersion = false;
};

struct Sc3EngineStatus {
	bool ready = false;
	bool booting = false;        // open() returned; the worker is still booting (3.1)
	bool failed = false;         // the boot ended badly (see error)
	long bootMs = 0;             // boot duration, once it finished
	bool degraded = false;
	bool serverRunning = false;
	bool dspOff = false;         // reserved for 3.3
	int port = 0;                // effective World UDP port (0 = not bound yet)

	// user s.options (read after the startup file)
	long maxNodes = 0;
	long memSizeKB = 0;
	long numWireBufs = 0;
	long numRGens = 0;
	long numBuffers = 0;
	long numAudioBusChannels = 0;
	long numInputBusChannels = 0;
	long numOutputBusChannels = 0;
	long blockSize = 0;
	long maxLogins = 0;
	bool sampleRateSet = false;   // the user fixed s.options.sampleRate
	bool sampleRateFromHost = false; // 3.2: the sr came from the DSP chain (Max), not from s.options

	// effective World values
	double sampleRate = 0.0;
	long bufLength = 0;          // frames per World block
	long worldBuses = 0;
	long worldSndBufs = 0;
	long worldRGens = 0;
	long numUnits = 0;

	std::string scVersion;       // version of the SC installation in use
	// 4.2 (a): warning when the installation is NOT exactly the engine version
	// (different patch, or a different minor forced with allowScVersion). Empty = match.
	std::string versionNote;
	// 4.2 (c): the compatibility smoke test (minimal SynthDef + /d_recv) ran fine.
	bool smokeOk = false;
	std::string classLibraryDir; // effective Resource dir
	std::string ugensPath;       // effective mUGensPluginPath

	std::string degradeReport;   // set if there was a fallback to stock
	std::string error;           // last boot error (empty if ok)
};

// ------------------------------------------------------- 3.5: audio cursor
// Read position of ONE instance (one `sc3~`) inside the single stream produced
// by the World. The World advances by **absolute** sample position: the first
// instance that needs samples renders whatever is missing and the others read
// exactly the same ones from the same buffer (no double render, no locking).
// The cursor belongs to the instance: it is reset by `dsp64`.
struct Sc3AudioCursor {
	uint64_t pos = 0; // next sample to read (absolute stream position)
};

// ------------------------------------------------------------------ engine

class Sc3Engine {
public:
	static Sc3Engine &instance();

	// Creates (or reuses) the engine. Refcount: open() increments, release()
	// decrements and the last one destroys everything.
	//
	// Phase 3.1: open() is ASYNCHRONOUS — it returns immediately (the sclang boot
	// takes ~1–3 s and used to block Max's main thread). It returns false only if
	// the engine was left Failed by a previous attempt (release() must be called
	// before retrying). For UI-less code use waitReady().
	//
	// While booting: isReady() == false and renderBlock() returns false with
	// silence ⇒ the audio thread does not touch the World. When it finishes the
	// state publishes itself; the caller queries it with status() (or waits with
	// waitReady).
	bool open(const Sc3EngineConfig &cfg);
	void release();

	// Waits for the boot to finish. Tests/CLI only: blocks the thread.
	bool waitReady(int timeoutMs = 120000);

	bool isReady() const;
	bool isBooting() const;
	Sc3EngineStatus status() const;

	// --- sclang: enqueue only (the worker runs it) ---------------------------
	// eval() prints the result like the IDE post window (`-> value`);
	// evalSilent() does not print (for internal messages: panic, freeall, …).
	void eval(const std::string &code);
	void evalSilent(const std::string &code);
	// evalWithSetup() = eval() but with a "setup" that runs BEFORE, in its own
	// compile: the user text is compiled VERBATIM, just like in the SC IDE
	// (bug #19: prefixing the setup breaks any block that starts with `var`,
	// e.g. `( var x = 1; … )`). The two interprets are atomic with respect to each
	// other (same worker job). The result printed is the user code's one.
	void evalWithSetup(const std::string &setup, const std::string &code);

	// Pending text for the `post` / `error` outlets. Emitted by `sc3_flush` on the
	// main thread (clock). `maxChars > 0` returns at most that many and leaves the
	// rest for the next flush (cap ~64 KB per cycle: do not block the UI with a
	// flood of posts). Breaks at a newline when it can.
	std::string takePost(size_t maxChars = 0);
	std::string takeError(size_t maxChars = 0);

	// --- audio: call ONLY from the audio thread ------------------------------
	long blockLength() const;   // frames per World block (0 if not ready)
	long numUnits() const;
	// Renders one block (blockLength() planar frames). Returns false without
	// touching the World if the engine is not ready (the caller emits silence).
	//
	// 3.3: it NEVER blocks. It takes the World with try_lock; if the pump without
	// DSP or the teardown holds it at that instant, it returns false (one block of
	// silence) instead of waiting. Callers must not assume "false" = not ready.
	bool renderBlock(float *outL, float *outR);

	// --- 3.5: multi-instance (buses) ---------------------------------------
	// A patch may hold several `sc3~`: all share the World and each one reads its
	// own bus range (`[sc3~ 2 4]` ⇒ buses 4 and 5). One World ⇒ one boot, one
	// clock, one socket; audio is split by buses.
	static constexpr long kServedBuses = 64; // buses that can be served (0..63)

	long servedBuses() const;   // min(worldBuses, kServedBuses) — 0 if not ready
	uint64_t renderedSamples() const; // samples advanced since the last setup
	long underruns() const;     // cursors that fell behind (diagnostics)
	// Points the cursor at the "now" of the stream (for dsp64: an instance that
	// starts must not read old samples nor ask for a jump backwards).
	void resetCursor(Sc3AudioCursor &c) const;

	// Renders whatever is missing and copies `frames` samples of the buses
	// [bus, bus+nchan) into outs[0..nchan-1] (double, Max layout). It NEVER blocks
	// the audio thread (try_lock, same as 3.3): if it cannot, it returns false and
	// the caller emits silence. Buses no UGen touched in the block are delivered as
	// silence (mAudioBusTouched: see the history comment), so stopping a synth does
	// not leave the last block stuck.
	bool pull(Sc3AudioCursor &c, long bus, int nchan, double **outs, long frames);

	// --- 3.8: INPUT audio (SoundIn / In.ar) --------------------------------
	// `sc3~` with `@ins N` has N signal inlets and pushes each host block here;
	// the engine drops it into the World's INPUT buses right before `World_Run`,
	// so `SoundIn.ar(i)` —which is `In.ar(NumOutputBuses + i)`— reads what arrives
	// through inlet i (e.g. `[adc~ 1 2]` → the first two).
	//
	// Verified on 3.14.1 (do not re-investigate):
	//   · `inputBuses = mAudioBus + mNumOutputs * mBufLength`; input bus i lives at
	//     `inputBuses + i * mBufLength` ⇒ input buses come AFTER the outputs
	//     (`In`/`Out` index `mAudioBus + bus*n`).
	//   · `In_next_a` only copies if `mAudioBusTouched[bus] == mBufCounter` ⇒ the
	//     bus has to be marked per block (otherwise: silence).
	//   · With `in=2 out=2` (the s.options default) the inputs are buses 2..3.
	//
	// One producer per World: the first one to claim (dsp64). The other `sc3~` with
	// `@ins` stay as consumers of the same input (the push is not duplicated) and
	// report it with `claimInput() == false`.
	// claim/release: main (dsp64/free). push: audio thread ONLY. Never blocks.
	bool claimInput(const void *owner, int nchan);  // true if it became the producer
	void releaseInput(const void *owner);
	bool pushInput(const void *owner, double *const *ins, int nchan, long frames);
	int inputChannels() const; // effective World input channels (0 if not ready)
	uint64_t inputPushed() const;
	long inputSkipped() const;   // skipped pushes (lock held by render/pump)
	long inputUnderruns() const; // input blocks with too few samples (zero-filled)
	long inputOverruns() const;  // discarded samples (ring full)
	// Diagnostics (the "3.8 diag" line of `info`): what the host delivered and what
	// was written to the World buses, per channel. Values from the last block.
	int inputNumins() const;           // signal inlets the host perform sees
	float inputPushPeak(int ch) const; // peak of the last block received (0..15)
	float inputFeedPeak(int ch) const; // peak of the last block pushed into the World

	// --- 3.3: advance the World WITHOUT DSP (call ONLY from main) -----------
	// With Max's DSP off, `perform64` does not run: without this the World freezes
	// and `/d_recv`, `/sync`, `/status`, `/s_new` and the timetagged bundles (3.6)
	// are not processed — the historical symptom: SafetyNet "started" when the DSP
	// was turned on. It advances whatever wall-clock time passed since the last
	// advance (audio or pump), capped by `budgetMs` of CPU so it does not stall the
	// UI. Returns the blocks run (0 if audio holds it, it is not ready, or a full
	// block has not elapsed yet). Without DSP there is no audio and no reliable
	// musical advance: this keeps SC's administrative side alive.
	int pumpWhenIdle(int budgetMs = 4);
	long pumpedBlocks() const;   // blocks advanced without DSP (diagnostics)
	long pumpLagBlocks() const;  // blocks the budget left unrun


	// Shutting down the sclang VM (bug #13). Called from atexit: `release()` only
	// drops the World/socket; the VM survives so the next `open` does not
	// re-initialize the runtime (that crashes SC's VM).
	void shutdownLang();

	// Not copyable.
	Sc3Engine(const Sc3Engine &) = delete;
	Sc3Engine &operator=(const Sc3Engine &) = delete;

private:
	// Drops worker + World + socket and leaves the engine Idle. Used when refs
	// reach 0 and also to retry a boot that was left Failed (port taken by the SC
	// IDE, broken library, …): that way `open` works again without reloading the
	// patch.
	void teardown();
	Sc3Engine();
	~Sc3Engine();
	struct Impl;
	std::unique_ptr<Impl> m;
};
