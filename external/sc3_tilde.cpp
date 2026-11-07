// SPDX-License-Identifier: GPL-3.0-only
/* sc3~ — Max external: SuperCollider 3.14.1 embedded (sclang + scsynth in-process).
 *
 * Phase 2.3: `open` brings up the engine (`Sc3Engine`: sclang + World) and
 * `perform64` pulls the audio out of the World. Phase 3.5: multiple instances
 * **by buses** — several `sc3~` share ONE World and each one reads its range of
 * buses (the World advances by absolute sample position, see `Sc3Engine::pull`).
 *
 * Layout (see help/sc3~.maxhelp):
 *   1 message inlet (right) + `@ins N` signal inlets to its left (0..16,
 *   default 0 = as in v0.12). Inlet i goes to the World's INPUT bus i ⇒
 *   `SoundIn.ar(i)` / `In.ar(NumOutputBuses + i, …)` read it (Phase 3.8).
 *   Only one `sc3~` per patch can be the input producer.
 *   ARGS: `[sc3~]` = 2 channels from bus 0 (compat) · `[sc3~ n]` = n channels
 *         from bus 0 · `[sc3~ n bus]` = n channels from bus `bus`.
 *   `@port N` (4.x): UDP port of the World. Default 0 = AUTO — the kernel picks
 *   a free one and the object's sclang follows it (`9_SC3ServerPort.sc`), so it
 *   coexists with the SC IDE, which always keeps 57110.
 *   OUTLETS (numbered right to left as in Max):
 *       outlet 0      = error          (text)
 *       outlet 1      = post           (text) — SC console
 *       outlet 2..n+1 = audio ch n-1 … ch 0 (signal)
 *   The signal outlets are created first (ch 0 → ch n-1) so that the `outs`
 *   array in `perform64` is [0]=ch0 (bus busoff), [1]=ch1, …
 *
 * Evaluated code receives `~sc3out` (base bus of THIS instance) and `~sc3n`
 * (channels), both in the topEnvironment: `Out.ar(~sc3out, sig)` or, directly,
 * `Out.ar(4, sig)` with
 * `[sc3~ 2 4]`.
 *
 * Assets: the class library and the stock UGens come from the REAL
 * SuperCollider installation (/Applications/SuperCollider.app, same version as
 * the engine); only the Qt-less module ships inside the bundle
 * (`Contents/Resources/SCMaxCompat`).
 *
 * Project rules (see AGENTS.md): the sclang VM is never touched from the audio
 * thread; the World is only touched from `perform64` (and from the worker
 * during boot, see Sc3Engine.h).
 */

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <dlfcn.h>

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "Sc3Engine.h"

#define SC3_TILDE_VERSION "0.15.8"

/* Local gain ramp (anti-click). 8 ms is inaudible as a fade and is enough for
   the cut not to be heard. SC's stop is queued AFTER the ramp reached
   ­0 (otherwise /g_freeAll cuts mid-ramp and the click comes back). */
static const double kFadeSeconds = 0.008;
static const double kStopDelayMs = 40.0; /* > kFadeSeconds, with margin */

typedef struct _sc3 {
	t_pxobject ob;       /* must be the first field (MSP object) */
	long nchan;          /* audio channels (outlets) = consecutive buses */
	long busoff;         /* base bus of THIS instance (busoff .. busoff+nchan-1) */
	long port;           /* 4.x: World UDP port. 0 = auto (the kernel picks it) */
	long allowscversion; /* 4.2: 1 = accept a SuperCollider install with another minor version */
	Sc3AudioCursor cur;  /* 3.5: read position in the World stream */
	void *out_post;      /* outlet 1 */
	void *out_err;       /* outlet 0 */
	double sr;           /* DSP chain sample rate, refreshed in dsp64 */
	double sr_warned;    /* 3.2: last sample rate reported as differing from the World */
	long maxvec;         /* DSP chain maxvectorsize */
	void *clock;         /* post/error flush to the outlets (main thread) */
	void *stop_clock;    /* deferred SC stop (main thread) */
	void *pump_clock;    /* 3.3: advances the World with DSP off (main thread) */
	int pump_running;    /* pump_clock is scheduled (avoids duplicate ticks) */
	int pending_stop;    /* 0 = none, 1 = panic, 2 = hardstop, 3 = freeall */
	/* bug #26: this instance is an OWNER of the process-wide World (opened, or
	   auto-attached by eval/panic). The audio thread reads it (`sc3_perform64`): an
	   instance that never attached and one that was `close`d must stay silent, so
	   `info` (engine=off) and the outlets cannot disagree. Atomic because of that
	   read; main is the only writer. */
	std::atomic<int> opened;
	int boot_seen;       /* last boot state reported (3.1: 0 off/1 booting/2 ready/3 failed) */
	int console_on;      /* mirror the engine text to Max's console (PER INSTANCE) */
	/* 3.8: audio input. `ins` = signal inlets (can only be set at creation,
	   see `@ins`); the object pushes every host block into the World (Sc3Engine.h). */
	long ins;
	int created;         /* `new` finished: `@ins` can no longer change the inlets */
	int input_claimed;   /* this instance became the input producer */
	int input_conn[16];  /* 3.8 diag: patchcords Max reports per signal inlet */
	int input_warned;    /* already warned that another sc3~ is the producer */
	/* Local fade (audio thread): avoids the click when stopping/starting the
	   audio. `gain_target` is written by main (panic → 0, eval/open → 1); `gain`
	   is only touched by the audio thread. 5 ms linear ramp. */
	std::atomic<float> gain_target;
	float gain;
	/* Bug #29: the "eval is not returning" error was already posted for the eval in
	   flight (cleared when sclang goes back to idle). */
	int stuck_warned;
} t_sc3;

static t_class *sc3_class = NULL;

/* 3.3: World pump with DSP off (defined next to dsp64; open/close use them) */
static void sc3_pump_start(t_sc3 *x);
static void sc3_pump_stop(t_sc3 *x);

/* Code input debug (message `debug 1`): dumps whatever arrives at `eval` to
   /tmp/sc3_eval_last.scd, with the type/size of every atom. Useful to see
   whether the Max container (message box, textedit…) altered the text. */
static int gDebugEval = 0;

/* Mirroring the engine text to the Max console is PER INSTANCE (`console 1`,
   the default): a patch with nothing connected still shows what happens.
   Patches that connect [print post]/[print error] send `console 0`, so each
   line comes out ONCE (otherwise it shows up twice: console + print).

   BUG #20: this used to be ONE global. Opening a second patch that sends
   `console 0` — the `sc3~` help, the test patches, any patch wiring `[print]` —
   silently muted the console for EVERY instance, so the patch that was already
   running just "stopped posting" (and nothing said why). The Max console is one
   window shared by the process, so the decision is still all-or-nothing, but it
   is now taken by the instances: mirror while AT LEAST ONE open instance wants
   it (`console 0` only means "this patch reads the text from the outlets"). */
static int gConsoleInstances = 0; /* instances with console_on == 1 */

/* The ONE place where the flag changes: keeps the counter in sync. Used by the
   `console 0|1` message and by the `@console` attribute (same state, so a patch
   can put it in the object box and Max saves it with the patcher). */
static void sc3_set_console(t_sc3 *x, int on)
{
	const int want = on ? 1 : 0;
	if (want == x->console_on)
		return;
	gConsoleInstances += want ? 1 : -1;
	x->console_on = want;
}
static void sc3_debug_dump(long argc, t_atom *argv, const std::string &code)
{
	if (!gDebugEval)
		return;
	FILE *f = fopen("/tmp/sc3_eval_last.scd", "a");
	if (!f)
		return;
	fprintf(f, "=== eval: argc=%ld chars=%zu\n", argc, code.size());
	for (long i = 0; i < argc; i++) {
		const int t = atom_gettype(argv + i);
		const char *tn = (t == A_LONG)    ? "int"
		                 : (t == A_FLOAT) ? "float"
		                 : (t == A_SYM)   ? "sym"
		                 : (t == A_COMMA) ? "COMMA"
		                 : (t == A_SEMI)  ? "SEMI"
		                 : (t == A_DOLLAR) ? "DOLLAR"
		                                  : "?";
		std::string txt = (t == A_SYM && atom_getsym(argv + i)) ? atom_getsym(argv + i)->s_name : "";
		fprintf(f, "[%ld] %s len=%zu <%s>\n", i, tn, txt.size(),
		        txt.size() > 60 ? (txt.substr(0, 60) + "…").c_str() : txt.c_str());
	}
	fprintf(f, "--- code (newlines as \\n) ---\n");
	for (size_t i = 0; i < code.size(); i++) {
		if (code[i] == '\n')
			fputs("\\n", f);
		else
			fputc(code[i], f);
	}
	fprintf(f, "\n=== end\n");
	fclose(f);
}

/* ------------------------------------------------------------------ utils */

/* Splits the text into pieces that fit in a Max symbol WITHOUT truncating: each
   piece is cut at a line break when possible (so the outlet prints complete
   lines). CAREFUL: Max has no "string" atoms in the externals C API, text
   travels as a symbol and `gensym` interns it — unavoidable; that is also why
   the pieces are reasonably small and the flush has a cap. */
static const size_t kMaxChunk = 450;

static void sc3_emit_chunk(void *outlet, const char *selector, const std::string &chunk)
{
	char buf[kMaxChunk + 1];
	size_t n = chunk.size() < kMaxChunk ? chunk.size() : kMaxChunk;
	std::memcpy(buf, chunk.data(), n);
	buf[n] = '\0';
	t_atom a;
	atom_setsym(&a, gensym(buf));
	outlet_anything(outlet, gensym((char *)selector), 1, &a);
}

static void sc3_emit_text(void *outlet, const char *selector, const std::string &text)
{
	if (!outlet || text.empty())
		return;
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = pos + kMaxChunk;
		if (end < text.size()) {
			size_t nl = text.rfind('\n', end);
			if (nl != std::string::npos && nl >= pos)
				end = nl + 1;
		} else {
			end = text.size();
		}
		sc3_emit_chunk(outlet, selector, text.substr(pos, end - pos));
		pos = end;
	}
}

/* Text through an outlet as a symbol (`post <text>` / `error <text>`). */
static void sc3_emit(t_sc3 *x, void *outlet, const char *selector, const char *text)
{
	if (!outlet || !text)
		return;
	sc3_emit_text(outlet, selector, std::string(text));
}

static void sc3_post(t_sc3 *x, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (x->console_on) /* per instance: a patch with [print] says it itself */
		object_post((t_object *)x, "%s", buf);
	sc3_emit(x, x->out_post, "post", buf);
}

/* The object's OWN voice (open / close / panic / hardstop / freeall): always to
   Max's console and to the `post` outlet.

   `sc3_post` mirrors only with `console 1` (bug #20: a patch that wires
   [print post] turns the mirror off so each engine line comes out once). For the
   lifecycle that rule is wrong: with the flag off, a `close` + `open` made the
   object look dead — the engine booted (or failed) and NOTHING said so anywhere
   (bug #23). So the lifecycle skips the flag. Cost: in a patch with `console 0`
   AND [print post] wired, these few lines show up twice (once from here, once
   from [print]); engine text keeps its "once" contract. */
static void sc3_note(t_sc3 *x, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	object_post((t_object *)x, "%s", buf);
	sc3_emit(x, x->out_post, "post", buf);
}

/* Max splits a message box's text into atoms: commas and semicolons
   SEPARATE messages unless they are escaped with a backslash (\, \;). If the user
   pastes the escaped version, Max usually consumes the backslash already; if it
   is still there, it is removed here (otherwise sclang would read \, as the
   symbol ","). */
static std::string sc3_unescape_max(const std::string &in)
{
	std::string out;
	out.reserve(in.size());
	for (size_t i = 0; i < in.size(); i++) {
		if (in[i] == '\\' && i + 1 < in.size() &&
		    (in[i + 1] == ',' || in[i + 1] == ';' || in[i + 1] == '$')) {
			out += in[++i];
		} else {
			out += in[i];
		}
	}
	return out;
}

/* Text of ONE atom, the way sclang must see it: numbers printed by
   std::to_string (trimmed of the trailing zeros it adds), symbols verbatim,
   and `$n` as `$n` (Max delivers message-box dollar args as A_DOLLAR). */
static std::string sc3_atom_text(const t_atom *a)
{
	switch (atom_gettype(a)) {
	case A_LONG:
		return std::to_string((long)atom_getlong(a));
	case A_FLOAT: {
		std::string tok = std::to_string((double)atom_getfloat(a));
		while (tok.size() > 1 && tok.back() == '0') tok.pop_back();
		if (!tok.empty() && tok.back() == '.') tok.pop_back();
		return tok;
	}
	case A_SYM: {
		const t_symbol *sym = atom_getsym(a);
		return sym ? sym->s_name : "";
	}
	case A_DOLLAR: {
		std::string tok = "$";
		tok += (char)atom_getlong(a);
		return tok;
	}
	default:
		return {};
	}
}

/* Joins a message's atoms into a single string separated by line breaks
   (equivalent to writing each atom on its own SC line; that way the // comments
   of a multi-line text that arrives split up are not broken).
   If a SINGLE symbol arrives (case [textedit] → [prepend eval]), it is used as is. */
static std::string sc3_atoms_to_text(long argc, t_atom *argv)
{
	// Max parses the text into atoms: the comma and the semicolon do NOT disappear,
	// they arrive as A_COMMA / A_SEMI (verified with `debug 1` on [textedit]).
	// They are rebuilt glued to the previous token (in SC `,\n` is valid, but this
	// way the text stays readable) and the rest is joined with line breaks: SC
	// treats newline as whitespace, so // comments are not broken.
	std::string out;
	for (long i = 0; i < argc; i++) {
		const int type = atom_gettype(argv + i);
		std::string tok;
		bool glue = false; // glues to the previous one (closing punctuation or separator)
		switch (type) {
		case A_COMMA:
			tok = ",";
			glue = true;
			break;
		case A_SEMI:
			tok = ";";
			glue = true;
			break;
		default:
			tok = sc3_atom_text(argv + i);
			break;
		}
		if (tok.empty())
			continue;
		if (!out.empty()) {
			const char last = out.back();
			const bool opens = (last == '(' || last == '[' || last == '{' || last == ',');
			if (!glue && !opens)
				out += '\n';
		}
		out += tok;
	}
	return out;
}

/* `Contents/Resources` of the .mxo, resolved from the binary's path (the bundle
   is at .../sc3~.mxo/Contents/MacOS/sc3~). SCMaxCompat lives there. */
static std::string sc3_bundle_resources()
{
	Dl_info info {};
	if (dladdr(reinterpret_cast<void *>(&ext_main), &info) && info.dli_fname) {
		std::filesystem::path exe(info.dli_fname);
		return (exe.parent_path().parent_path() / "Resources").string();
	}
	return {};
}

/* ------------------------------------------------------------------ engine */

static void sc3_engine_config(t_sc3 *x, Sc3EngineConfig &cfg)
{
	const char *app = std::getenv("SC3_APP_PATH");
	cfg.scAppPath = app ? app : "/Applications/SuperCollider.app";
	/* 4.x: World UDP port. Precedence: `@port N` from the patch → env
	   SC3_PORT (tests/CLI) → **0 = AUTO** (the kernel picks a free one, so the
	   SC IDE — which always takes 57110 — and sc3~ coexist). The real port shows
	   up in `info` and the object's sclang uses it (`~s.addr.port`). */
	long wantPort = x->port;
	if (wantPort <= 0) {
		const char *port = std::getenv("SC3_PORT");
		wantPort = port ? std::atoll(port) : 0;
	}
	cfg.port = (int)std::max<long>(0, std::min<long>(wantPort, 65535));
	cfg.compatDir = sc3_bundle_resources();
	if (!cfg.compatDir.empty())
		cfg.compatDir += "/SCMaxCompat";
	/* 3.2: the World has to be born at the DSP chain's sample rate. With DSP off
	   `dsp64` has not run yet (x->sr == 0), but Max already has the driver's
	   sample rate configured: `sys_getsr()` returns it anyway. That way the normal
	   case (Max opens with DSP off) does not need to recreate the World later. */
	const double hostSr = x->sr > 0.0 ? x->sr : sys_getsr();
	cfg.preferredSampleRate = hostSr > 0.0 ? hostSr : 0.0;
	/* 4.2: accept an SC installation with ANOTHER MINOR version (and warn). The
	   default (0) rejects: the installed class library is compiled by our
	   interpreter and needs its primitives. */
	cfg.allowScVersion = x->allowscversion != 0;
	cfg.verbose = std::getenv("SC3_DEBUG_OSC") != nullptr;
}

static void sc3_report_status(t_sc3 *x)
{
	Sc3Engine &eng = Sc3Engine::instance();
	if (!x->opened) {
		/* The World is per process: "this instance did not attach" and "nothing is
		   running" are different states and `info` must not hide the first one. */
		Sc3EngineStatus st = eng.status();
		if (st.ready || st.booting)
			sc3_note(x, "sc3~ v%s engine=%s (shared World) — this instance is NOT "
			            "attached: send `open` (any `eval`/`panic` attaches it too)",
			         SC3_TILDE_VERSION, eng.stateName());
		else
			sc3_note(x, "sc3~ v%s engine=off (send `open`)", SC3_TILDE_VERSION);
		return;
	}
	Sc3EngineStatus st = eng.status();
	const char *stateName = eng.stateName(); /* idle / booting / ready / failed */
	/* 3.3: "dsp on/off" is THIS instance's state in the audio chain; with it off
	   the World advances from main's clock (N blocks pumped per tick). */
	const int dspOn = x->opened && x->pump_running && st.ready
	                      ? sys_getdspobjdspstate((t_object *)x)
	                      : 0;
	sc3_post(x, "sc3~ v%s engine=%s sr=%.0f blockLen=%ld maxvec=%ld units=%ld "
	            "degraded=%s sc=%s",
	         SC3_TILDE_VERSION, stateName, st.sampleRate, st.bufLength,
	         x->maxvec, st.numUnits, st.degraded ? "yes" : "no",
	         st.scVersion.empty() ? "?" : st.scVersion.c_str());
	/* Bug #29: `queued` does not include the eval IN FLIGHT (the worker pops it
	   before running) ⇒ `busy` is the one that answers "is sclang answering?". */
	char busy[48];
	if (st.busyMs == 0)
		snprintf(busy, sizeof(busy), "no");
	else
		snprintf(busy, sizeof(busy), "%ldms%s", st.busyMs, st.stuck ? " (STUCK)" : "");
	sc3_post(x, "  sr=%s dsp=%s pumped=%ld blocks (cumulative, DSP off only) lag=%ld "
	            "console=%s queued=%ld busy=%s",
	         st.sampleRateFromHost ? "project (DSP chain)" : "s.options",
	         dspOn ? "on" : "off", eng.pumpedBlocks(), eng.pumpLagBlocks(),
	         x->console_on ? "on" : "off", eng.queuedJobs(), busy);
	sc3_post(x, "  s.options: maxNodes=%ld memSize=%ldKB buses=%ld blockSize=%ld maxLogins=%ld",
	         st.maxNodes, st.memSizeKB, st.numAudioBusChannels, st.blockSize, st.maxLogins);
	/* 4.x: World port. With 0 (default) the kernel picks it: that is what makes
	   it possible to keep the SC IDE open with its server booted. */
	sc3_post(x, "  UDP port 127.0.0.1:%d (%s) — sclang: ~s.addr.port=%d", st.port,
	         x->port > 0 ? "forced with @port" : "auto", st.port);
	/* 3.4: if the boot failed, `info` is the first thing one looks at ⇒ say why
	   (port busy, class library, s.options.port, adoption) instead of just
	   "engine=failed". */
	if (!st.error.empty())
		sc3_post(x, "  error: %s", st.error.c_str());
	/* 4.2 (a/b): the installation is not exactly the engine's version (different
	   patch, or a minor forced with `@allowscversion`). */
	if (!st.versionNote.empty())
		sc3_post(x, "  version: %s", st.versionNote.c_str());
	if (!x->console_on)
		sc3_note(x, "  console=off: this instance sends the engine text to its outlets "
		            "only (`console 1` mirrors it here too); %d instance(s) mirror now — "
		            "if another patch's sc3~ has [print] wired it takes the text first",
		         gConsoleInstances);
	sc3_post(x, "  3.5: ch=%ld buses=%ld..%ld (of %ld served) underruns=%ld",
	         x->nchan, x->busoff, x->busoff + x->nchan - 1, eng.servedBuses(),
	         eng.underruns());
	if (x->ins > 0) {
		/* 3.8: audio input. `World` = effective input buses (s.options).
		   `pushed` only advances if this instance is the producer and DSP is running. */
		const int wIn = eng.inputChannels();
		sc3_post(x, "  3.8: @ins %ld (World: %d input buses, SoundIn.ar 0..%d) "
		            "producer=%s pushed=%llu skipped=%ld underruns=%ld overruns=%ld",
		         x->ins, wIn, wIn > 0 ? wIn - 1 : 0,
		         x->input_claimed ? "yes" : "no",
		         (unsigned long long)eng.inputPushed(), eng.inputSkipped(),
		         eng.inputUnderruns(), eng.inputOverruns());
		/* Diagnostics: tells "the host is not sending the channel" (peak 0 on the
		   pushInput side) apart from "the engine is not writing it to the bus" (peak
		   > 0 above and 0 below). Values from the last block, read only. */
		sc3_post(x, "  3.8 diag: inlets=%ld z_in=%ld cables=[%d %d %d %d]", x->ins,
		         (long)((t_pxobject *)x)->z_in, x->input_conn[0], x->input_conn[1],
		         x->input_conn[2], x->input_conn[3]);
		sc3_post(x, "  3.8 diag: numins=%d cables=[%d %d %d %d] "
		            "ent=[%.3f %.3f %.3f %.3f] bus=[%.3f %.3f %.3f %.3f]",
		         eng.inputNumins(),
		         x->input_conn[0], x->input_conn[1], x->input_conn[2], x->input_conn[3],
		         eng.inputPushPeak(0), eng.inputPushPeak(1),
		         eng.inputPushPeak(2), eng.inputPushPeak(3),
		         eng.inputFeedPeak(0), eng.inputFeedPeak(1),
		         eng.inputFeedPeak(2), eng.inputFeedPeak(3));
	}
	sc3_post(x, "  classLibrary=%s ugens=%s", st.classLibraryDir.c_str(), st.ugensPath.c_str());
}

/* ------------------------------------------------------------------ mensajes */

/* bug #26: attach THIS object to a World that is already up. It is `open` without a
   boot — the World/sclang are process-wide, so there is nothing to start — but the
   owner registration is what keeps `info`, `close` (only this instance stops
   listening) and the refcount honest: without it the object rendered and evaluated
   as a non-owner, the first `close` of another instance tore the World down under it
   and `panic` was refused. Returns false only if the engine rejected the owner. */
static bool sc3_attach_existing(t_sc3 *x, const char *why)
{
	Sc3Engine &eng = Sc3Engine::instance();
	Sc3EngineConfig cfg;
	sc3_engine_config(x, cfg);
	if (!eng.open(cfg, x))
		return false;
	x->opened = true;
	x->boot_seen = eng.isReady() ? 2 : 1;
	sc3_pump_start(x); // 3.3: with DSP off the World still advances
	sc3_note(x, "%s: attached to the running World (%s) — buses %ld..%ld, no new boot",
	         why, eng.stateName(), x->busoff, x->busoff + x->nchan - 1);
	return true;
}

/* The engine is a process-wide singleton: this object's `opened` flag is only a
   cache of it. When they disagree — a boot cancelled by another instance's
   `close`, a patch that was closed from elsewhere, a World that failed after the
   boot — an `eval` used to be queued forever with NOTHING said, which is exactly
   what "the object stopped answering" looks like (bug #23). Syncing the flag here
   is also what makes the next `open` boot again instead of answering "already
   open". bug #26: with the World already up the sync goes the other way — the
   object ATTACHES instead of failing (sc3~ never booted twice). */
static bool sc3_require_engine(t_sc3 *x, const char *what)
{
	Sc3Engine &eng = Sc3Engine::instance();
	const bool up = eng.isReady() || eng.isBooting();
	if (x->opened && up)
		return true;
	if (!x->opened) {
		if (up)
			return sc3_attach_existing(x, what);
		object_error((t_object *)x, "%s: send `open` first", what);
		return false;
	}
	x->opened = false;
	object_error((t_object *)x, "%s: the engine is %s (not open) — send `open`", what,
	             eng.stateName());
	return false;
}

/* Bug #29: sclang has no preemption, so an eval that loops (e.g. a Number
   `nextTimeOnGrid` with an Array as clock — see handoff #29) never returns. Any
   `eval` queued behind it would never run, so it is dropped with a visible error
   instead of piling up in silence (this is what looked like "sc3~ stopped
   answering"). Panic/close are NOT dropped: their SC side is queued too, and the
   local gain ramp is what actually silences the audio. */
static bool sc3_engine_stuck(t_sc3 *x, const char *what)
{
	Sc3Engine &eng = Sc3Engine::instance();
	if (!eng.evalStuck())
		return false;
	std::string msg = std::string(what) + ": dropped — sclang is still inside a "
		"previous eval (" + std::to_string(eng.evalBusyMs() / 1000) +
		" s, it cannot be interrupted). Restart Max.";
	object_error((t_object *)x, "sc3~: %s", msg.c_str());
	sc3_emit(x, x->out_err, "error", msg.c_str());
	return true;
}

static void sc3_open(t_sc3 *x)
{
	// After a failed boot (port busy, broken library…) the engine was left in
	// Failed: it can be retried with open without reloading the patch (open()
	// clears everything and starts from scratch). If it is healthy, it is
	// idempotent.
	Sc3Engine &engine = Sc3Engine::instance();
	/* The ENGINE decides, not the flag: "ready" and "booting" mean there is nothing
	   to do; anything else (idle/failed) has to boot again, even if the flag still
	   says open. */
	if (x->opened && (engine.isReady() || engine.isBooting())) {
		sc3_note(x, "open: already open (idempotent)");
		return;
	}
	/* bug #26: the World is shared, so with it already up `open` means ATTACH, not
	   boot. Nothing is restarted and the instance is usable immediately. */
	if (engine.isReady() || engine.isBooting()) {
		if (!sc3_attach_existing(x, "open")) {
			Sc3EngineStatus st = engine.status();
			std::string err = st.error.empty() ? "open failed" : st.error;
			sc3_emit(x, x->out_err, "error", err.c_str());
			object_error((t_object *)x, "sc3~: %s", err.c_str());
			return;
		}
		sc3_report_status(x);
		return;
	}
	Sc3EngineConfig cfg;
	sc3_engine_config(x, cfg);

	// 3.1: ASYNCHRONOUS boot. open() returns right away; the result (ready or
	// error) is detected by sc3_flush and posted without blocking Max's UI.
	if (!Sc3Engine::instance().open(cfg, x)) {
		Sc3EngineStatus st = Sc3Engine::instance().status();
		std::string err = st.error.empty() ? "open failed (was there a previous close?)" : st.error;
		sc3_emit(x, x->out_err, "error", err.c_str());
		object_error((t_object *)x, "sc3~: %s", err.c_str());
		return;
	}
	x->opened = true;
	x->boot_seen = 1;
	sc3_pump_start(x); // 3.3: with DSP off the World still advances
	sc3_note(x, "open: booting in the background (sclang + World) — the UI is not blocked");
}

static void sc3_close(t_sc3 *x)
{
	if (!x->opened) {
		sc3_note(x, "close: was not open");
		return;
	}
	/* Bug #29: with sclang stuck inside an eval, teardown would join a thread that
	   never returns ⇒ Max frozen on `close`. Same rule as `eval`: say it and drop
	   it (the engine is unrecoverable; restart Max). */
	if (Sc3Engine::instance().evalStuck()) {
		std::string msg = "close: sclang is stuck in an eval (" +
		                  std::to_string(Sc3Engine::instance().evalBusyMs() / 1000) +
		                  " s, it cannot be interrupted) — restart Max";
		object_error((t_object *)x, "sc3~: %s", msg.c_str());
		sc3_emit(x, x->out_err, "error", msg.c_str());
		return;
	}
	if (Sc3Engine::instance().isBooting())
		sc3_note(x, "close: waiting for the boot to finish (the VM cannot be aborted "
		            "halfway through compileLibrary)");
	sc3_pump_stop(x);
	Sc3Engine::instance().release(x);
	x->opened = false;
	x->boot_seen = 0;
	sc3_note(x, "close: engine closed");
}

static void sc3_bang(t_sc3 *x)
{
	/* Max idiom: a loadbang/button on the inlet should wake the engine up.
	   `bang` boots only when nothing is up yet (open() is idempotent and also
	   retries a failed boot); then, like `info`, it posts the state. */
	if (!x->opened || Sc3Engine::instance().status().failed)
		sc3_open(x);
	sc3_report_status(x);
}

static void sc3_info(t_sc3 *x)
{
	sc3_report_status(x);
}

/* Setter of the `@console` attribute (see ext_main). Same state as the message. */
static t_max_err sc3_console_set(t_sc3 *x, t_object *attr, long argc, t_atom *argv)
{
	(void)attr;
	long n = 1;
	if (argc > 0 && argv) {
		const int t = atom_gettype(argv);
		if (t == A_LONG)
			n = atom_getlong(argv);
		else if (t == A_FLOAT)
			n = (long)atom_getfloat(argv);
	}
	sc3_set_console(x, n != 0);
	return MAX_ERR_NONE;
}

/* `console 0` / `console 1`: whether THIS instance's engine text is mirrored to
   the Max console in addition to going out the outlets. 0 when the patch already
   wires [print post]/[print error]. It never touches the other patches: they
   keep their own flag (bug #20, see gConsoleInstances). */
static void sc3_console(t_sc3 *x, long on)
{
	sc3_set_console(x, on != 0);
	object_post((t_object *)x, "console = %s%s",
	            x->console_on ? "ON (console + outlets)" : "OFF (outlets only)",
	            gConsoleInstances > 0 ? ""
	                                  : " — no instance mirrors to the console now");
}

/* `debug 1` / `debug 0`: dumps every `eval` to /tmp/sc3_eval_last.scd. */
static void sc3_debug(t_sc3 *x, long on)
{
	gDebugEval = on ? 1 : 0;
	if (gDebugEval)
		remove("/tmp/sc3_eval_last.scd");
	sc3_post(x, "debug eval = %s (%s)", gDebugEval ? "ON" : "OFF",
	         gDebugEval ? "dumping to /tmp/sc3_eval_last.scd" : "not dumping");
}

/* Cancels a pending stop: if the user does `eval` within the 40 ms after the
   panic, the deferred stop must not kill the new sound. */
static void sc3_cancel_stop(t_sc3 *x)
{
	if (x->pending_stop != 0) {
		x->pending_stop = 0;
		clock_unset(x->stop_clock);
	}
}

/* `send <object> <args…>`: shortcut for `eval` with SC messages, no quotes.
   `send s.boot` == `eval "s.boot"`; `send Ndef rthm 1`… (atoms are joined with
   spaces). The result is printed as in any eval. */
static void sc3_send(t_sc3 *x, t_symbol *s, long argc, t_atom *argv)
{
	(void)s; // selector = "send"
	if (!sc3_require_engine(x, "send"))
		return;
	if (sc3_engine_stuck(x, "send"))
		return;
	if (argc < 1) {
		sc3_emit(x, x->out_err, "error", "send: missing the SC message (e.g. send s.boot)");
		return;
	}
	sc3_cancel_stop(x);
	std::string code = sc3_atoms_to_text(argc, argv);
	if (!code.empty() && code.back() != ';')
		code += ";";
	x->gain_target.store(1.0f); // entry ramp (avoids the click when restarting)
	Sc3Engine::instance().eval(code);
}

/* 3.5: instance context for the user's code, as a separate "setup"
   (evalWithSetup): the user code is compiled VERBATIM, as in the SC IDE. It used
   to be concatenated in front of the text and that broke any block starting with
   `var` (bug #19): the chunk no longer started with `(`, so `sclang` parsed the
   `var` in the middle of an expression ⇒
   "syntax error, unexpected VAR, expecting ')'".
   The two interprets are atomic with respect to each other (same worker job), so
   ~sc3out/~sc3n hold what belongs to this object while its code runs:
     Out.ar(~sc3out, sig)        — route to THIS sc3~'s buses
     Ndef(...).play              — .play takes ~sc3out (SCMaxCompat module)
   Note: it is sclang's topEnvironment, so it holds for the synchronous eval (what
   gets deferred to a Routine/clock sees the last evaluated value; there it is
   better to pass the bus explicitly).
   Bug #31: the setup writes to `topEnvironment` explicitly instead of `~sc3out = …`.
   The `~x = v` sugar assigns into `currentEnvironment`, which SURVIVES between
   evals: after `ProxySpace.push(s.boot)` in one eval, the setup of the NEXT one
   landed inside that space, where LazyEnvir turns a missing key into a NodeProxy
   ("\sc3out" with source = the bus number) instead of storing a number ⇒ the
   space got polluted and `~sc3out` stopped being a bus. A `put` into
   topEnvironment is unconditional and side-effect free. */
static std::string sc3_ctx_setup(const t_sc3 *x)
{
	return "topEnvironment.put(\\sc3out, " + std::to_string(x->busoff) +
	       "); topEnvironment.put(\\sc3n, " + std::to_string(x->nchan) + ");";
}

static void sc3_eval(t_sc3 *x, t_symbol *s, long argc, t_atom *argv)
{
	(void)s;
	if (!sc3_require_engine(x, "eval"))
		return;
	if (sc3_engine_stuck(x, "eval"))
		return;
	// [textedit] sends `text <buffer>`. If the Max container did not drop the
	// selector (e.g. with outputmode 1 the buffer travels as ONE symbol and
	// "text" arrives as the first argument), we skip it: no valid SC line starts
	// with the bare atom `text`.
	long first = 0;
	if (argc > 1 && atom_gettype(argv) == A_SYM && atom_getsym(argv) &&
	    std::strcmp(atom_getsym(argv)->s_name, "text") == 0)
		first = 1;
	std::string code = sc3_atoms_to_text(argc - first, argv + first);
	sc3_debug_dump(argc - first, argv + first, code);
	if (code.empty()) {
		sc3_emit(x, x->out_err, "error", "eval: missing the code");
		return;
	}
	sc3_cancel_stop(x);
	x->gain_target.store(1.0f); // entry ramp (avoids the click when restarting)
	Sc3Engine::instance().evalWithSetup(sc3_ctx_setup(x),
	                                   sc3_unescape_max(code));
}

/* `set <target> <key> <value> [<key> <value> …]`: a Synth/Node parameter change
   with the backslash and the commas generated HERE. A [message] box cannot carry
   them (a Max message box is a lexer: `,` separates messages and `\` is its
   escape character), so `x.set(\freq, $1)` arrives as `x.set(freq\, $1)` and the
   backslash of the symbol is gone before any object sees the atom. This message
   takes what Max CAN deliver:
       set x freq 81             →  x.set(\freq, 81);
       set x freq 81 amp 0.2     →  x.set(\freq, 81, \amp, 0.2);
   The first atom is the target, verbatim (`x`, `~mysynth`, …); then key/value
   pairs: keys become symbols (`\key`), values are printed as they arrive.
   Drive it with [number] → [prepend set x freq]. */
static void sc3_set(t_sc3 *x, t_symbol *s, long argc, t_atom *argv)
{
	(void)s; // selector = "set"
	if (!sc3_require_engine(x, "set"))
		return;
	if (sc3_engine_stuck(x, "set"))
		return;
	// 1 target + N key/value pairs. Anything else is a typo, not a parameter
	// change (and it keeps us away from the generic `set <value>` that Max
	// preset/pattr objects send to their targets).
	if (argc < 3 || argc % 2 == 0) {
		sc3_emit(x, x->out_err, "error",
		         "set: expected `set <target> <key> <value> [<key> <value>]…`");
		return;
	}
	const std::string target = sc3_atom_text(argv);
	if (target.empty()) {
		sc3_emit(x, x->out_err, "error",
		         "set: the first atom must be the target (e.g. x or ~mysynth)");
		return;
	}
	std::string code = target + ".set(";
	for (long i = 1; i < argc; i += 2) {
		if (atom_gettype(argv + i) != A_SYM) {
			sc3_emit(x, x->out_err, "error",
			         "set: the keys must be symbols (e.g. set x freq 81)");
			return;
		}
		const std::string value = sc3_atom_text(argv + i + 1);
		if (value.empty()) {
			sc3_emit(x, x->out_err, "error",
			         "set: the values must be numbers or symbols");
			return;
		}
		if (i > 1)
			code += ", ";
		code += "\\"; // `\key`: the SC symbol marker, generated here
		code += sc3_atom_text(argv + i);
		code += ", ";
		code += value;
	}
	code += ");";
	Sc3Engine::instance().evalWithSetup(sc3_ctx_setup(x), code);
}

static void sc3_evalfile(t_sc3 *x, t_symbol *s)
{
	if (!sc3_require_engine(x, "evalfile"))
		return;
	if (sc3_engine_stuck(x, "evalfile"))
		return;
	if (!s || !s->s_name || !s->s_name[0]) {
		sc3_emit(x, x->out_err, "error", "evalfile: missing the path");
		return;
	}
	FILE *f = fopen(s->s_name, "rb");
	if (!f) {
		std::string msg = std::string("evalfile: cannot open '") + s->s_name +
		                  "' (use an absolute path, or one relative to Max's cwd)";
		sc3_emit(x, x->out_err, "error", msg.c_str());
		object_error((t_object *)x, "SC: %s", msg.c_str());
		return;
	}
	std::string code;
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		code.append(buf, n);
	fclose(f);
	sc3_cancel_stop(x);
	x->gain_target.store(1.0f);
	Sc3Engine::instance().evalWithSetup(sc3_ctx_setup(x), code);
}

/* Equivalent to Cmd+. in the SC IDE, which is `CmdPeriod.run`:
     SystemClock.clear; AppClock.clear;        // kills Routines/Tasks/forks
     objects.do(_.doOnCmdPeriod);              // hooks (PatternConductor, Pfx…)
     Server.freeAll                            // /g_freeAll + /clearSched
   `s.freeAll` on its own only frees nodes: it leaves Routines and TempoClock alive.
   `hardstop` = `CmdPeriod.hardRun` (also clears TempoClock.default).
   `freeall` = server nodes only (does not touch clocks). */
/* Two-phase stop (anti-click):
     1. here: local ramp to 0 (8 ms) — immediate silence, does not depend on sclang;
     2. kStopDelayMs later (main thread): the stop is sent to SC and the ring is
        emptied. When the World cuts the nodes we are already at gain 0 ⇒ inaudible.
   If the stop were sent together with the ramp, /g_freeAll cuts mid-ramp and the
   jump is audible again (measured: click even with the fade). */
static void sc3_request_stop(t_sc3 *x, int kind)
{
	/* bug #26: same rule as eval — if the World is up, attach; if not, this is a
	   real "nothing to stop", not a silent refusal. */
	if (!sc3_require_engine(x, "panic/hardstop/freeall"))
		return;
	x->gain_target.store(0.0f);
	x->pending_stop = kind;
	clock_delay(x->stop_clock, kStopDelayMs);
}

static void sc3_panic(t_sc3 *x)
{
	sc3_request_stop(x, 1);
	sc3_note(x, "panic: CmdPeriod.run (nodes + clocks + hooks)");
}

static void sc3_hardstop(t_sc3 *x)
{
	sc3_request_stop(x, 2);
	sc3_note(x, "hardstop: CmdPeriod.hardRun (includes TempoClock.default)");
}

static void sc3_freeall(t_sc3 *x)
{
	sc3_request_stop(x, 3);
	sc3_note(x, "freeall: s.freeAll (nodes only; clocks and Routines keep running)");
}

static void sc3_assist(t_sc3 *x, void *b, long m, long a, char *s)
{
	(void)b;
	if (m == ASSIST_INLET) {
		if (x->ins > 0 && a >= 1 && a <= x->ins)
			snprintf(s, 256,
			         "audio input ch%ld (signal) → SoundIn.ar(%ld) / In.ar(%ld)",
			         a - 1, a - 1, 2 + (a - 1));
		else
			snprintf(s, 256,
			         "open, close, info, eval <code>, send <target> <args>, "
			         "set <target> <key> <value> [...], evalfile <path>, "
			         "panic (Cmd+.), hardstop, freeall, console 0|1, debug 0|1, "
			         "port <n>, bang; @ins N (signal inlets), @port N (0 = auto)");
	} else {
		// 3.5: error = 0, post = 1, audio ch n-1 = 2 … ch 0 = n+1.
		const long n = x->nchan;
		if (a == 0)
			snprintf(s, 256, "error (text)");
		else if (a == 1)
			snprintf(s, 256, "post / SC console (text)");
		else if (a >= 2 && a <= n + 1) {
			const long ch = n + 1 - a;
			snprintf(s, 256, "audio ch%ld (signal, bus %ld)", ch, x->busoff + ch);
		} else
			snprintf(s, 256, "outlet %ld", a);
	}
}

/* Phase 2 of the stop: the ramp already finished (gain 0) ⇒ now SC and clear the
   ring. No fade-in here: the next user `eval` brings gain back up. */
static void sc3_finish_stop(t_sc3 *x)
{
	const int kind = x->pending_stop;
	x->pending_stop = 0;
	if (!x->opened || kind == 0)
		return;
	// Silent: we do not want a `-> nil` in the outlet for every panic.
	if (kind == 1)
		Sc3Engine::instance().evalSilent("CmdPeriod.run;");
	else if (kind == 2)
		Sc3Engine::instance().evalSilent("CmdPeriod.hardRun;");
	else
		Sc3Engine::instance().evalSilent("s.freeAll;");

}

/* Mirrors the engine text to the Max console, with the SAME look as the SC IDE
   post window: RAW text, without the object prefix. `object_post()` adds "sc3~: "
   (and `object_error` its own), so `post()`/`error()` are used instead, which also
   has an advantage: `error()` shows up highlighted in red and can be caught with
   Max's [error] object.

   It is split into lines for two reasons: the engine chunk ends with '\n' (post()
   already breaks the line ⇒ without this there was one blank line too many) and
   that way every engine line is an independent post, as in the IDE. */
static void sc3_console_mirror(const std::string &text, bool is_error)
{
	size_t end = text.size();
	while (end > 0 && text[end - 1] == '\n')
		--end; /* post() adds the trailing '\n': do not duplicate it */
	size_t pos = 0;
	while (pos < end) {
		size_t nl = text.find('\n', pos);
		if (nl == std::string::npos || nl > end)
			nl = end;
		std::string line = text.substr(pos, nl - pos);
		if (is_error)
			error("%s", line.c_str());
		else
			post("%s", line.c_str());
		pos = nl + 1;
	}
}

/* Flush of pending engine text → outlets. Runs on the main thread
   (Max's scheduler clock), so touching outlets is safe. */
static void sc3_flush(t_sc3 *x)
{
	if (x->opened) {
		std::string p = Sc3Engine::instance().takePost(64 * 1024);
		if (!p.empty()) {
			if (x->console_on)
				sc3_console_mirror(p, false);
			sc3_emit(x, x->out_post, "post", p.c_str());
		}
		std::string e = Sc3Engine::instance().takeError(64 * 1024);
		if (!e.empty()) {
			if (x->console_on)
				sc3_console_mirror(e, true);
			sc3_emit(x, x->out_err, "error", e.c_str());
		}

		// Asynchronous boot transition (3.1): only here is ready or failed
		// reported, on the main thread and after draining the startup posts (so
		// the order in the console matches SC's).
		Sc3Engine &eng = Sc3Engine::instance();
		Sc3EngineStatus st = eng.status();
		const int now = st.ready ? 2 : (st.failed ? 3 : (st.booting ? 1 : 0));
		if (now != x->boot_seen) {
			x->boot_seen = now;
			if (now == 2) {
				sc3_post(x, "open: boot ready in %ld ms", st.bootMs);
				sc3_report_status(x);
				/* 4.2 (a/b): the installation is not exactly the engine's version.
				   It is accepted (patch, or forced minor) but NOT silently: it goes to
				   the console and to `info`, so a bug report brings it along. */
				if (!st.versionNote.empty())
					sc3_post(x, "warning: %s", st.versionNote.c_str());
				/* 3.8: the World is born with `s.options.numInputBusChannels`; if the
				   user asks for more inlets than that, the missing buses do not exist
				   (SoundIn stays silent). Warned ONCE, when it becomes ready. */
				const int wIn = Sc3Engine::instance().inputChannels();
				if (x->ins > 0 && wIn < x->ins)
					sc3_post(x, "input: @ins %ld but the World has %d input buses "
					            "(s.options.numInputBusChannels): SoundIn.ar(%d..%ld) stays "
					            "silent. Raise it in startup.scd and send `open` again",
					         x->ins, wIn, wIn, x->ins - 1);
				if (st.degraded && !st.degradeReport.empty())
					sc3_emit(x, x->out_err, "error", st.degradeReport.c_str());
			} else if (now == 3) {
				std::string err = st.error.empty() ? "boot failed" : st.error;
				sc3_emit(x, x->out_err, "error", err.c_str());
				object_error((t_object *)x, "%s", err.c_str());
				// Release the failed engine: the object is left as freshly created
				// (eval asks for open, and open starts clean). No patch reload.
				eng.release(x);
				x->opened = false;
				x->boot_seen = 0;
			}
		}

		/* Bug #29: watchdog for an eval that never returns. The VM cannot be
		   interrupted (the embedded client has no SIGINT/hook), so the least we owe
		   the user is to SAY it — once per eval — instead of looking dead while sclang
		   burns memory. */
		if (st.stuck) {
			if (!x->stuck_warned) {
				x->stuck_warned = 1;
				std::string msg =
				    "eval has been running for " + std::to_string(st.busyMs / 1000) +
				    " s and has not returned: sclang CANNOT be interrupted (the code is "
				    "probably looping). Audio and the World still work, but new evals are "
				    "dropped — restart Max. `info` shows busy=…";
				object_error((t_object *)x, "sc3~: %s", msg.c_str());
				sc3_emit(x, x->out_err, "error", msg.c_str());
			}
		} else if (st.busyMs == 0) {
			x->stuck_warned = 0;
		}
	} else {
		x->boot_seen = 0;
	}
	clock_delay(x->clock, 100);
}

/* --------------------------------------------------------------------- DSP */

static void sc3_perform64(t_sc3 *x, t_object *dsp64, double **ins, long numins,
                          double **outs, long numouts, long sampleframes,
                          long flags, void *userparam)
{
	(void)dsp64; (void)flags; (void)userparam;

	/* bug #26: `eng.isReady()` is about the SHARED World, not about this object.
	   An instance that never attached (no `open`/eval/panic) or that was `close`d
	   must not listen: it reads buses that another instance's code may be writing,
	   so without this the object played while `info` said engine=off. Silence here
	   is also what makes `close` mean "stop listening on this instance" (help). */
	if (!x->opened.load(std::memory_order_relaxed)) {
		for (long i = 0; i < numouts; i++)
			if (outs[i])
				memset(outs[i], 0, (size_t)sampleframes * sizeof(double));
		return;
	}
	/* 3.8: the input is pushed BEFORE asking for the output window. The World
	   block rendered in this tick (if anything is left to render) has to see the
	   samples that just came in through the inlets; the decoupling between the host
	   block and the World block is absorbed by the engine ring (prefill). */
	if (x->ins > 0 && x->input_claimed) {
		/* bug #18 (measured): Max hands `ins[]` over with the LAST signal inlet in
		   `ins[0]` — the `dsp_setup` proxies are created right to left.
		   It is reversed so that "bus ch = inlet ch+2" (inlet 2 = SoundIn.ar(0)). */
		double *rev[16];
		const long n = std::min<long>(numins, 16);
		for (long i = 0; i < n; ++i)
			rev[i] = ins[n - 1 - i];
		Sc3Engine::instance().pushInput(x, rev, (int)numins, sampleframes);
	}

	const long nch = x->nchan;
	if (nch <= 0 || numouts < nch || !outs[0]) {
		for (long i = 0; i < numouts; i++)
			if (outs[i])
				memset(outs[i], 0, (size_t)sampleframes * sizeof(double));
		return;
	}

	// Gain ramp (5 ms): cutting to zero mid-wave produces a broadband click. The
	// target is set by main (panic → 0, eval → 1).
	const float target = x->gain_target.load(std::memory_order_relaxed);
	const float sr = (x->sr > 0.0) ? (float)x->sr : 48000.0f;
	const float step = 1.0f / std::max(1.0f, sr * (float)kFadeSeconds);
	float g = x->gain;

	// 3.5: each instance reads ITS range of buses from the World's shared stream.
	// The first `pull` of the tick renders whatever is missing; the others (same
	// tick) copy those same samples ⇒ no per-object ring and no double advance.
	Sc3Engine &eng = Sc3Engine::instance();
	bool ok = false;
	if (eng.isReady())
		ok = eng.pull(x->cur, x->busoff, (int)nch, outs, sampleframes);
	if (!ok)
		for (long ch = 0; ch < nch; ++ch)
			memset(outs[ch], 0, (size_t)sampleframes * sizeof(double));

	// There is a single gain ramp for all channels (the fade belongs to the
	// object, not to a channel). The target is set by main (panic → 0, eval → 1).
	for (long i = 0; i < sampleframes; i++) {
		if (g < target) {
			g += step;
			if (g > target)
				g = target;
		} else if (g > target) {
			g -= step;
			if (g < target)
				g = target;
		}
		if (g != 1.0f)
			for (long ch = 0; ch < nch; ++ch)
				outs[ch][i] *= (double)g;
	}
	x->gain = g;
}

/* --- 3.3: pump with DSP off -----------------------------------------------
   With Max's DSP off, `perform64` does not run: without this the World freezes
   and /d_recv, /sync, /status, /s_new and the bundles with a timetag (3.6) are
   never processed — the historical symptom: the SafetyNet only "started" once
   DSP was turned on.
   main's clock advances the World ONLY if this instance is not in the audio
   chain. Detection uses the documented API: `sys_getdspobjdspstate`
   (z_dsp.h:209) — "preferable to sys_getdspstate()" because it tells the case
   global audio on but patcher stopped apart. Exclusion against the audio thread
   is done by the engine (worldMtx + try_lock on both sides): it never blocks
   audio. */
static const double kPumpIntervalMs = 20.0; /* main clock cadence */
static const int kPumpBudgetMs = 4;         /* CPU ceiling per tick (20 %) */

static void sc3_pump_tick(t_sc3 *x)
{
	if (!x->pump_running)
		return; /* tick in flight from a close: do not reschedule */
	clock_fdelay(x->pump_clock, kPumpIntervalMs);
	Sc3Engine &eng = Sc3Engine::instance();
	if (!eng.isReady())
		return; /* during boot the worker pumps the World */
	if (sys_getdspobjdspstate((t_object *)x))
		return; /* DSP on: avanza perform64 */
	eng.pumpWhenIdle(kPumpBudgetMs);
}

static void sc3_pump_start(t_sc3 *x)
{
	if (x->pump_running || !x->pump_clock)
		return;
	x->pump_running = 1;
	clock_fdelay(x->pump_clock, kPumpIntervalMs);
}

static void sc3_pump_stop(t_sc3 *x)
{
	x->pump_running = 0;
	if (x->pump_clock)
		clock_unset(x->pump_clock);
}

static void sc3_dsp64(t_sc3 *x, t_object *dsp64, short *count, double samplerate,
                      long maxvectorsize, long flags)
{
	(void)flags;
/* 3.8 diag: count[i] = how many patchcords reach signal inlet i according to
   the DSP chain. 0 with a visible cable in the patch ⇒ Max dropped the cable. */
	for (long i = 0; i < 16; ++i)
		x->input_conn[i] = (i < x->ins) ? (int)count[i] : -1;
	x->sr = samplerate;
	x->maxvec = maxvectorsize;

	/* 3.2: the World is born at the chain's sample rate, but if the user changes
	   it in Audio Status with the patch open we cannot follow along live:
	   recreating the World would drop nodes, SynthDefs and buffers. Warned once
	   per change. */
	{
		const double worldSr = Sc3Engine::instance().status().sampleRate;
		if (Sc3Engine::instance().isReady() && worldSr > 0.0 &&
		    std::fabs(samplerate - worldSr) > 0.5 && std::fabs(samplerate - x->sr_warned) > 0.5) {
			x->sr_warned = samplerate;
			sc3_post(x, "project sr = %.0f Hz but the World runs at %.0f Hz: send "
			            "`close` and then `open` to recreate it (doing it live would lose "
			            "nodes and SynthDefs)",
			         samplerate, worldSr);
		}
	}

	/* 3.5: the cursor starts at the "now" of the stream (a new instance must not
	   read old samples or request a jump backwards) and gain goes back to 0:
	   re-entering DSP, the 8 ms ramp fades in from silence (bug #12: the last
	   frames of the previous cycle used to be heard at full amplitude). Own
	   buffers are no longer needed: the engine splits the window into chunks when
	   `sampleframes` exceeds its history. */
	Sc3Engine::instance().resetCursor(x->cur);
	x->gain = 0.0f;

	/* 3.8: claim the World's input production. Whoever gets there first (dsp64)
	   is the producer; another `sc3~` with `@ins` does not duplicate the push (they
	   all listen to the same input, like several objects reading the same adc~). */
	if (x->ins > 0) {
		const bool mine = Sc3Engine::instance().claimInput(x, (int)x->ins);
		if (!mine && !x->input_warned) {
			x->input_warned = 1;
			sc3_post(x, "input: another sc3~ is already the producer of the input buses "
			            "(this object does not duplicate the push)");
		}
		x->input_claimed = mine ? 1 : 0;
	}

	dsp_add64(dsp64, (t_object *)x, (t_perfroutine64)sc3_perform64, 0, NULL);
}

/* ------------------------------------------------------------- life cycle */

/* 3.8: `@ins N` — audio INPUT signal inlets (0..16). Can only be set when the
   object is created: the inlets of an MSP object cannot be added later, so a late
   `@ins` is ignored with a visible error (instead of leaving `x->ins` and the real
   inlets inconsistent). */
static t_max_err sc3_ins_set(t_sc3 *x, t_object *attr, long argc, t_atom *argv)
{
	(void)attr;
	long n = 0;
	if (argc > 0 && argv) {
		const int t = atom_gettype(argv);
		if (t == A_LONG)
			n = atom_getlong(argv);
		else if (t == A_FLOAT)
			n = (long)atom_getfloat(argv);
	}
	if (n < 0)
		n = 0;
	if (n > 16)
		n = 16;
	if (x->created && n != x->ins) {
		object_error((t_object *)x,
		             "@ins %ld ignored: signal inlets are defined when the "
		             "object is created (this one has @ins %ld; reload it with the new value)",
		             n, x->ins);
		return MAX_ERR_NONE;
	}
	x->ins = n;
	return MAX_ERR_NONE;
}

/* 4.x: `@port N` / message `port N` — World UDP port. 0 (default) =
   AUTO: the kernel picks a free one, so sc3~ and the SC IDE (which always takes
   57110, and binds without SO_REUSEADDR) coexist without closing anything. The
   socket is bound in `open`: with the engine already open the value does not
   re-bind (a warning is posted). `@port 57110` reproduces the classic behavior. */
static void sc3_set_port(t_sc3 *x, long n)
{
	if (n < 0)
		n = 0;
	if (n > 65535)
		n = 65535;
	if (x->opened && n != x->port)
		object_error((t_object *)x,
		             "@port %ld: the port is fixed when the World is bound — send "
		             "`close` + `open` to apply it",
		             n);
	x->port = n;
}

static t_max_err sc3_port_set(t_sc3 *x, t_object *attr, long argc, t_atom *argv)
{
	(void)attr;
	long n = 0;
	if (argc > 0 && argv) {
		const int t = atom_gettype(argv);
		if (t == A_LONG)
			n = atom_getlong(argv);
		else if (t == A_FLOAT)
			n = (long)atom_getfloat(argv);
	}
	sc3_set_port(x, n);
	return MAX_ERR_NONE;
}

static void sc3_port_msg(t_sc3 *x, long n)
{
	sc3_set_port(x, n);
}

/* 3.8: `@ins N` is needed BEFORE `dsp_setup` (the signal inlets are created
   there and cannot be added later). This scan only reads the value from argv; the
   attribute is applied the same way later by `attr_args_process` (which is what
   validates and warns). Without the scan, `dsp_setup` ran at the end of `new` and
   — measured in Max 9 — the DSP chain ended up attributing ALL connections to the
   inlet 1 proxy (the cables of inlet 2 reached `perform64` as a zero buffer). */
static long sc3_scan_attr(long argc, t_atom *argv, const char *want, long dflt)
{
	long found = dflt;
	for (long i = 0; i + 1 < argc; ++i) {
		if (atom_gettype(argv + i) != A_SYM)
			continue;
		/* Max passes the attribute atom WITH the '@' in front ("@ins"), even though
		   class_addattr registers the name without it ("ins"): both are accepted. */
		const char *nm = atom_getsym(argv + i)->s_name;
		if (nm[0] == '@')
			++nm;
		if (strcmp(nm, want) != 0)
			continue;
		const long t = atom_gettype(argv + i + 1);
		if (t == A_LONG)
			found = atom_getlong(argv + i + 1);
		else if (t == A_FLOAT)
			found = (long)atom_getfloat(argv + i + 1);
		++i;
	}
	return found;
}

static void *sc3_new(t_symbol *s, long argc, t_atom *argv)
{
	t_sc3 *x = (t_sc3 *)object_alloc(sc3_class);
	if (!x)
		return NULL;

	/* 3.8: first the POSITIONAL args (channels + bus) and then the
	   @attributes: `@ins` sets how many signal inlets there are, and the inlets are
	   created only once ⇒ `dsp_setup` goes at the end, when `ins` is already known. */
	const long npos = attr_args_offset((short)argc, argv);

	/* 3.5: args = channels + base bus (absolute, inside the served pool 0..63).
	     [sc3~]        → 2 ch, buses 0,1  (compat with 2.x patches)
	     [sc3~ n]      → n ch, buses 0..n-1
	     [sc3~ n bus]  → n ch, buses bus..bus+n-1
	   The code evaluated in this object receives ~sc3out (base bus) and ~sc3n.
	   3.8: `@ins N` adds N audio input signal inlets. */
	auto argLong = [&](long i, long dflt) -> long {
		if (i >= npos)
			return dflt;
		const long t = atom_gettype(argv + i);
		if (t == A_LONG)
			return atom_getlong(argv + i);
		if (t == A_FLOAT)
			return (long)atom_getfloat(argv + i);
		return dflt;
	};
	const long wantCh = argLong(0, 2);
	const long wantBus = argLong(1, 0);
	x->nchan = std::max<long>(1, std::min<long>(wantCh, 16));
	x->busoff = std::max<long>(0, wantBus);
	x->port = 0; /* 4.x: 0 = auto; `@port N` overrides it below (attr_args_process) */
	x->allowscversion = 0; /* 4.2: strict default (same minor version only) */
	x->cur.pos = 0;
	x->ins = sc3_scan_attr(argc, argv, "ins", 0); /* 0 = no signal inlets (v0.12) */
	x->ins = std::max<long>(0, std::min<long>(x->ins, 16));
	x->created = 0;
	x->input_claimed = 0;
	x->input_warned = 0;
	/* 4.5: DEFAULT ON — a patch can just wire [print post]/[print error] and send
	   `console 0`, or put `@console 0` in the object box (see the attribute).
	   It is set BEFORE attr_args_process: the attribute setter only counts the
	   change, it does not initialize. */
	x->console_on = 1;
	gConsoleInstances++;
	for (long i = 0; i < 16; ++i)
		x->input_conn[i] = -1; /* -1 = dsp64 has not run yet */
	/* 3.8: `ins_created` = value from the scan; the attribute applies the same (below). */
	const long ins_created = x->ins;

	/* 3.8 / bug #18 — MANDATORY ORDER (measured in Max 9.2, 30-oct):
	   1. the MESSAGE inlet is created FIRST (it ends up as inlet 1);
	   2. `dsp_setup` creates the `ins` signal inlets to its right AFTERWARDS.

	   Why not the other way round (the "natural" one, signals on the left): if a
	   non-signal inlet is created after `dsp_setup`, the `t_pxobject` is left with
	   `z_in = 1` and Max's DSP chain attributes ALL connections to the first signal
	   inlet proxy (`count[] = [n 0 …]`) ⇒ inlet 2 never gets signal and
	   `SoundIn.ar(1)` stays mute, even with the cable in the patch. With the order
	   above `z_in = 0`, `count[] = [1 1 …]` and every inlet reaches its channel.
	   Measured: ctrlin~ (control, no extra inlet) z_in=0 ✓; the 4 variants with the
	   extra inlet afterwards gave z_in=1 and broken mapping (inlet_new, intin,
	   proxy_new, forced z_in — Max rewrites it). See PLAN.md "bug #18".
	   With `ins == 0` only the message inlet is left (v0.12 behavior). */
	if (x->ins > 0)
		inlet_new((t_object *)x, NULL);
	dsp_setup((t_pxobject *)x, x->ins);

	/* 3.8: the @attributes are applied AFTER creating inlets/outlets (as in the
	   rest of Max's objects). If the `ins` setter asks for a different number, the
	   inlets are already made ⇒ a warning is posted and the real number wins. */
	attr_args_process(x, (short)argc, argv);
	if (x->ins != ins_created) {
		object_error((t_object *)x,
		             "@ins %ld ignored: signal inlets are defined when the "
		             "object is created (this one has @ins %ld; reload it with the new value)",
		             x->ins, ins_created);
		x->ins = ins_created;
	}
	x->created = 1;

	/* Creation order = left→right; the last one created is outlet 0.
	   ⇒ error = 0, post = 1, audio ch n-1 = 2 … ch 0 = n+1. */
	for (long ch = 0; ch < x->nchan; ++ch)
		outlet_new(x, "signal");
	x->out_post = outlet_new(x, NULL);
	x->out_err  = outlet_new(x, NULL);

	x->sr = 0.0;
	x->maxvec = 0;
	x->sr_warned = 0.0;
	x->opened = false;
	x->boot_seen = 0;
	x->gain = 1.0f;
	x->gain_target.store(1.0f);
	x->clock = clock_new(x, (method)sc3_flush);
	clock_delay(x->clock, 200);
	x->stop_clock = clock_new(x, (method)sc3_finish_stop);
	x->pending_stop = 0;
	/* 3.3: starts in open() and stops in close(); without open it does nothing */
	x->pump_clock = clock_new(x, (method)sc3_pump_tick);
	x->pump_running = 0;

	(void)s;
	if (x->busoff + x->nchan > Sc3Engine::kServedBuses) {
		object_error((t_object *)x,
		             "SC: buses %ld..%ld outside the served range (0..%ld): the object "
		             "stays silent",
		             x->busoff, x->busoff + x->nchan - 1,
		             Sc3Engine::kServedBuses - 1);
	} else {
		sc3_post(x, "sc3~ v%s — %ld ch, buses %ld..%ld", SC3_TILDE_VERSION, x->nchan,
		         x->busoff, x->busoff + x->nchan - 1);
	}
	if (x->nchan != wantCh || x->busoff != wantBus)
		sc3_post(x, "  (args out of range: channels 1..16, bus >= 0)");
	if (x->ins > 0)
		sc3_post(x, "  @ins %ld: inlet 1 = messages · inlets 2..%ld (signal) → "
		            "SoundIn.ar(0)..SoundIn.ar(%ld)",
		         x->ins, x->ins + 1, x->ins - 1);
	return x;
}

static void sc3_free(t_sc3 *x)
{
	sc3_pump_stop(x); /* before releasing the engine: the clock uses it */
	if (x->pump_clock) {
		object_free(x->pump_clock);
		x->pump_clock = NULL;
	}
	if (x->clock) {
		clock_unset(x->clock);
		object_free(x->clock);
		x->clock = NULL;
	}
	if (x->stop_clock) {
		clock_unset(x->stop_clock);
		object_free(x->stop_clock);
		x->stop_clock = NULL;
	}
	if (x->input_claimed) { /* 3.8: release the input production */
		Sc3Engine::instance().releaseInput(x);
		x->input_claimed = 0;
	}
	if (x->opened) {
		Sc3Engine::instance().release(x);
		x->opened = false;
	}
	if (x->console_on) { /* bug #20: the counter must not drift when a patch closes */
		gConsoleInstances--;
		x->console_on = 0;
	}
	dsp_free((t_pxobject *)x);
}

/* External entry point: `ext_main` with C linkage and C74_EXPORT
   (default visibility). It is the symbol Max looks for in the bundle; without it
   the dlopen "works" but the class is not registered ⇒ the object stays beige
   with "No such object". `ext.h` already declares the prototype, here it is only
   defined. */
extern "C" void C74_EXPORT ext_main(void *r)
{
	(void)r;
	t_class *c = class_new("sc3~", (method)sc3_new, (method)sc3_free,
	                       (long)sizeof(t_sc3), (method)NULL, A_GIMME, 0);
	class_dspinit(c);

	class_addmethod(c, (method)sc3_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c, (method)sc3_assist, "assist", A_CANT, 0);

	/* 3.8: `@ins N` = audio input signal inlets. Own setter because the inlets are
	   created in `new`: once the object exists the value does not change.
	   CAREFUL: the type is passed with USESYM(long) (= gensym("long")), NOT with
	   `_sym_long`: the `_sym_*` macros from commonsyms.h depend on the global
	   `_common_symbols`, which Max does NOT export ⇒ the bundle is left with an
	   unresolved data symbol and Max rejects it on load ("symbol not found in flat
	   namespace '__common_symbols'", error 67). sc_code uses CLASS_ATTR_* precisely
	   because they expand to USESYM. */
	class_addattr(c, attr_offset_new("ins", USESYM(long), 0, (method)0L,
	                                 (method)sc3_ins_set, calcoffset(t_sc3, ins)));

	/* 4.x: `@port N` = World UDP port (0 = auto, the kernel picks it). Saved with
	   the patch; applied when binding (open/close). */
	class_addattr(c, attr_offset_new("port", USESYM(long), 0, (method)0L,
	                                 (method)sc3_port_set, calcoffset(t_sc3, port)));

	/* 4.2: `@allowscversion 1` = accept a SuperCollider installation with
	   another MINOR version (3.13, 3.15…). Default 0: same minor version only. */
	class_addattr(c, attr_offset_new("allowscversion", USESYM(long), 0, (method)0L,
	                                 (method)0L,
	                                 calcoffset(t_sc3, allowscversion)));

	/* 4.5: `@console 0` = read the engine text from the outlets only (a patch
	   that wires [print post]/[print error]); no mirroring to Max's console, so
	   every line comes out ONCE. Default 1 (like the SC IDE's post window). It is
	   the same state as the `console 0|1` message — but the attribute is SAVED
	   with the patch, so no loadbang is needed. Per instance: it never mutes
	   another patch (bug #20). */
	class_addattr(c, attr_offset_new("console", USESYM(long), 0, (method)0L,
	                                 (method)sc3_console_set,
	                                 calcoffset(t_sc3, console_on)));

	class_addmethod(c, (method)sc3_bang, "bang", 0);
	class_addmethod(c, (method)sc3_info, "info", 0);
	class_addmethod(c, (method)sc3_debug, "debug", A_LONG, 0);
	class_addmethod(c, (method)sc3_console, "console", A_LONG, 0);
	class_addmethod(c, (method)sc3_port_msg, "port", A_LONG, 0);
	class_addmethod(c, (method)sc3_eval, "eval", A_GIMME, 0);
	class_addmethod(c, (method)sc3_send, "send", A_GIMME, 0);
	class_addmethod(c, (method)sc3_set, "set", A_GIMME, 0);
	class_addmethod(c, (method)sc3_evalfile, "evalfile", A_SYM, 0);
	class_addmethod(c, (method)sc3_open, "open", 0);
	class_addmethod(c, (method)sc3_close, "close", 0);
	class_addmethod(c, (method)sc3_panic, "panic", 0);
	class_addmethod(c, (method)sc3_panic, "stop", 0);
	class_addmethod(c, (method)sc3_hardstop, "hardstop", 0);
	class_addmethod(c, (method)sc3_freeall, "freeall", 0);

	class_register(CLASS_BOX, c);
	sc3_class = c;
}
