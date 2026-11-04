// SPDX-License-Identifier: GPL-3.0-only
/* sc_code.cpp — S1: the Max object `sc_code` (in-patch code editor for `sc3~`).
 *
 * Living plan: SCODE.md (§5 keymap measured at K0, §8 phases). This file is UI +
 * glue: the text and the caret live in `sc_buffer` (headless, with its own test
 * table) and the decision of "what do I evaluate" lives in `sc_region`
 * (`sc_eval_span`, the IDE port). Here only:
 *
 *   - minimal jbox (monospace paint, background, caret, focus ring),
 *   - keyboard through `key` (measured at K0: `keyfilter` is NEVER called on a jbox
 *     without a textfield, and ⌘ arrives whole ⇒ the keymap compares keycode + mods),
 *   - evaluation: emits `eval <code>` (ONE symbol) to the `sc3~` of the patch.
 *
 * Hard project rule: `sc_code` NEVER touches the engine. It only speaks through the
 * outlet, with patchable messages, just like `[message]` but with no parser in
 * between.
 *
 * Phase track (SCODE.md §8): S1 = buffer + paint + typing + eval. S2 = selection
 * (mouse drag / double / triple click, ⇧-click), `⌘A/C/V/X` with NSPasteboard and
 * `⌘Z`/`⇧⌘Z`. S3 = auto-close of pairs, autoindent on Return and `⌃.` panic (see
 * 0.9.3). S4 = per-token highlight, with an incremental cache in `sc_highlight`.
 * S5 = scroll (brought forward), line-number gutter, attributes (colors, fontsize,
 * readonly, autoclose, tabwidth, linenumbers), `jsave`/`jload` and `@file` with
 * autoload/autowrite (§7.10); bracket matching + title bar (§7.11) and the context
 * menu with Save/Load to File (§7.13).
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 */

#include "ext.h"
#include "ext_obex.h"
#include "ext_drag.h" /* jdrag_*: drops, including Finder files (SCODE.md §12) */
#include "jpatcher_api.h"
#include "jpatcher_syms.h" /* jpatcher_syms_init(): WITHOUT this jbox_new() segfaults (bug #17) */
#include "jgraphics.h"

#include <chrono>
#include <dlfcn.h> /* dladdr: the Library dir of the loaded bundle (S6c) */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

#include "sc_bar.h"
#include "sc_blink.h"
#include "sc_bracket.h"
#include "sc_call.h"
#include "sc_complete.h"
#include "sc_buffer.h"
#include "sc_clipboard.h"
#include "sc_filedialog.h"
#include "sc_highlight.h"
#include "sc_index.h"
#include "sc_indent.h"
#include "sc_path.h"
#include "sc_region.h"
#include "sc_scroll.h"
#include "sc_snip.h"
#include "sc_wrap.h"

/* 0.9.9: drops insert the ABSOLUTE path (`"/Users/g/…/x.wav"`, volume prefix gone, no
 * `~` — sclang does not expand it) and `@snippets` / the `snippets <path>` message take
 * the path with or without one pair of quotes (`'…'` or `"…"`).
 * 0.9.8: the title bar reads "sclang" (it is the language editor, not a file).
 * 0.9.7 (S6c+): COMPLETION in the bar, AUTOMATIC and display-only. After three letters
 * of a token (`Sin`) the bar shows the best candidate and `n/N`; `Tab` inserts it (one
 * undo step), `⇧Tab` cycles, `⏎` is NEVER taken (it still evaluates / breaks the line) and
 * typing filters until nothing is left. After `Class.` the list is only the methods of
 * that class. Measured at the gate: neither `⌃Space` nor `⌥Space` reaches `key` on this
 * keyboard, so the session is not behind a shortcut (the manual toggle stays, unused).
 * 0.9.6 (S6c step 2b): the BOTTOM BAR is wired. With `@callaid 1` (default) a line is
 * RESERVED at the bottom of the box (`h - bar_height`, so the caret can never hide
 * behind it) and it shows the signature of the call that contains the caret
 * (`SinOsc.ar(` ⇒ `SinOsc.ar(freq, phase, mul, add)`, current argument highlighted,
 * `2/4` on the right). The data comes from the class library index (§10.1) read by
 * `mtime`: `stat` on every paint, the 1,16 MB dump re-read only when it changed.
 * `@barcolor`/`@barhighlight` are the colors; with `@callaid 0` nothing changes for the
 * text. It is display-only: no key is intercepted and the buffer is never touched.
 * 0.9.4: the context menu pops AT the click point (it appeared far outside the box
 * when the box was not near the canvas origin: the box-local mouse point was passed
 * as canvas coordinates) and two defaults changed by user request: `@blinkduration`
 * 500 ms (was 400) and `@scrollspeed` 3. (was 1.).
 * 0.9.1: `@scrollspeed <float>` (default 1.) — multiplies the delta of the mouse
 * wheel and of the trackpad (two fingers): with 1 the trackpad felt very slow. It
 * is also registered as a message (`scrollspeed 4.`), same lesson as `@snippets`.
 * 0.9.0 (S6): SNIPPETS. `@snippets <path>` loads a text file with `### key` blocks
 * + verbatim body (own format, no escapes: sc_snip.{h,cpp}) and evaluating
 * `snip.key` REPLACES the evaluated text with the snippet — it does not evaluate
 * it, it does not emit `eval` to the engine (sc3~ does not know what `snip.kick`
 * is), caret at the end, one undo step. Hot-reload by mtime/size on every
 * invocation. 0.8.4 (S5e): `@blinkduration` (ms, default 500 since 0.9.4) and the blink curve
 * becomes the IDE one (`QEasingCurve::InCubic`, `overlay.cpp` blinkCode) — checked
 * in the source: the IDE also has the duration as a setting (default 600) and also
 * blinks the WHOLE block (trailing `//` comment included) because `blinkCode`
 * covers StartOfBlock..EndOfBlock. 0.8.3 (S5e): WRAP to the box width (the text is
 * reflowed on resize; before, a long line could only be read by enlarging the box,
 * there was no horizontal scroll) + BLINK of the evaluated block (500 ms, color by
 * attribute). 0.8.1: Save/Load use NSSavePanel/NSOpenPanel with UTType (`.scd`
 * guaranteed; the Max panel did not enforce it). 0.8.0 (S5d): context menu (right
 * click) with undo/redo, cut/copy/paste/delete/select all and Save/Load to .scd.
 * 0.7.3: the "sc.code" title is right-aligned. 0.7.2: evaluates with `eval
 * <symbol>` (no temporary .scd). 0.7.1: `jsave` keys with a prefix (the box
 * dictionary already has `text`). 0.7.0 (S5c): bracket pair at the caret + title
 * bar. Carries over 0.6.1 (height resize), 0.6.0 (gutter, attributes, jsave/jload,
 * @file) and everything before. */
#define SC_CODE_VERSION "0.9.9"

/* The palette is an ATTRIBUTE per token group (S5b): the factory values live in
 * the CLASS_ATTR_DEFAULT of ext_main, not here. Only the colors that are not
 * exposed remain (the scroll bar) plus the fixed geometry. */
#define COL_BAR_R 0.62
#define COL_BAR_G 0.66
#define COL_BAR_B 0.74

#define PAD_X 6.
#define PAD_Y 4.
#define BAR_W 7.    /* width of the vertical scroll bar (on the right) */
#define GUTTER_GAP 8. /* breathing room to the right of the line numbers (S5b) */
#define SC_TITLE_TEXT "sclang" /* top strip (S5c), right-aligned */

/* `@wrap 0` (= no wrap) uses this cell width: each line stays in a single row, which
 * is exactly the behavior before S5e. */
#define SC_NO_WRAP (1 << 28)

/* S6: state of the snippets file. It sits behind a pointer (new/delete in new/free)
 * because `object_alloc` returns zeroed memory, not a constructed C++ object. */
struct ScSnipState {
	sc_code::Snippets dict; /* archivo parseado (sc_snip.h) */
	std::string path;       /* path resuelto (`~` expandido) */
	long mtime;             /* hot-reload stamp (0 = not read yet) */
	long long size;         /* -1 = the last attempt failed: retry */
};

typedef struct _sc_code {
	t_jbox j_box;               /* must be the first field */
	sc_code::Buffer *buf;       /* buffer UTF-8 (headless, ver sc_buffer.h) */
	void *out_eval;             /* outlet: eval <code> (ONE symbol, multiline) */
	long focused;
	long tabwidth;
	long click_count;          /* 1 = caret, 2 = word, 3 = line (S2) */
	long last_click_x, last_click_y;
	double last_click_ms;
	long dragging;
	double font_size;           /* font size in use (tabwidth of the metric) */
	double char_w, line_h, ascent; /* cached monospace metrics (not measured in paint) */
	double metrics_for;         /* font_size these metrics are valid for */
	sc_code::ScrollView scroll; /* vertical offset in pixels (sc_scroll.h, headless) */
	sc_code::WrapCache wraps;   /* filas visuales del wrap (sc_wrap.h, headless) */
	long wrap;                  /* 1 = split the text to the box width (default 1) */
	int text_cols;              /* cells per row according to the last paint (0 = not measured) */
	sc_code::Highlight hl;      /* per-line token cache for painting (S4) */
	double view_w, view_h;      /* box size as the last paint saw it */
	long sb_drag;               /* 1 = dragging the bar thumb */
	double sb_grab;             /* click offset inside the thumb (so it does not jump) */

	/* --- S5b: attributes ------------------------------------------------- */
	t_jrgba bgcolor, textcolor, selcolor, caretcolor; /* base */
	t_jrgba keywordcolor, builtincolor, primitivecolor, classcolor; /* tokens */
	t_jrgba symbolcolor, stringcolor, charcolor, numbercolor;
	t_jrgba envvarcolor, commentcolor, bracketcolor, operatorcolor;
	long linenumbers;           /* line-number gutter (default 1) */
	long readonly;              /* does not edit (navigation/copy/eval still work) */
	long autoclose;             /* auto-cierre de pares (default 1) */
	long autowrite;             /* writes @file after every edit (debounce) */
	long autoload;              /* lee @file al fijarlo (default 1) */
	t_symbol *filename;         /* @file: document path (NULL = no file) */
	double gutter_w;            /* gutter width in px, recomputed by paint */
	t_clock *autowrite_clock;   /* debounce del autowrite */

	/* --- S5c: title bar and bracket matching ------------------------------ */
	long titlebar;              /* 1 = top strip with "sclang" (default 1) */
	double title_h;             /* strip height in px (0 = no bar) */
	long matchbrackets;         /* 1 = highlight the pair of the caret bracket */
	t_jrgba matchcolor;         /* background of the bracket pair */
	sc_code::BracketMatcher matcher; /* pareo con cache (sc_bracket.h, headless) */

	/* --- S5e: evaluation blink -------------------------------------------- */
	long evalblink;             /* 1 = blink del bloque evaluado (default 1) */
	long blinkduration;         /* blink ms (default 500, the IDE uses 600) */
	t_jrgba blinkcolor;         /* color del blink (RGBA, atributo) */
	t_clock *blink_clock;       /* repaints the fade (500 ms in 25 ms steps) */
	sc_code::Blink blink_state; /* rango + reloj del blink (sc_blink.h, headless) */

	/* --- S6: snippets (`snip.KEY` ⇒ the body of the file) ---------------- */
	t_symbol *snipfile;         /* @snippets: path of the snippets file */
	ScSnipState *snips;         /* dict parseado + stamp del hot-reload */

	/* --- S6c: index of the class library ('index', SCODE.md §10.1) -------- */
	t_symbol *indexfile;        /* @indexfile: the file sclang writes the dump to */
	sc_code::ClassIndex *index; /* the parsed dump (heap: std::map members) */
	std::string index_loaded;   /* path the current dict came from ("" = none) */
	long index_mtime;           /* stamp of the dump as last read */
	long long index_size;
	long index_warned;          /* the "never generated" notice goes once per object */

	/* --- S6c step 2b: the bottom bar (§10.2) ------------------------------ */
	long callaid;               /* 1 = reserved bar with the signature (default 1) */	double bar_h;               /* bar height in px (0 = no bar, from sc_code_bar_height) */
	long bar_grab;              /* 1 = the mousedown was on the bar: ignore the drag */
	t_jrgba barcolor;           /* bar background */
	t_jrgba barhighlight;       /* color of the current argument */

	/* --- S6c+: completion session (⌃Space, §10.6) ------------------------ */
	sc_code::Completer *completer; /* candidate engine (headless, sc_complete) */
	std::string compl_receiver; /* receiver the candidate list came from (cache key) */
	long compl_open;            /* 1 = the bar is showing candidates, not the signature */
	int compl_cycle;            /* which alternative (Tab/⇧Tab) */
	int compl_start, compl_end; /* token that ⏎ replaces (end == caret) */
	std::string compl_word;     /* candidate that ⏎ inserts */
	int compl_alternatives;     /* n of `n/N` */

	/* --- 0.9.1: wheel / trackpad speed ------------------------------------ */
	double scrollspeed;         /* @scrollspeed: px per delta unit (default 3.) */
} t_sc_code;

static t_class *s_sc_code_class = NULL;

/**********************************************************************/
/* evaluation                                                          */

/* Monotonic clock in ms (evaluation blink and double/triple click detection). */
static double sc_code_now_ms() {
	using namespace std::chrono;
	return (double)duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count() / 1000.;
}

/* --- S5e: blink of the evaluated block --------------------------------------- */

/* One fade tick: when the blink clock says it is over (500 ms), it turns off and
 * repaints one last time without the background. In the meantime it repaints with
 * whatever alpha applies; paint is cheap (only visible rows). */
static void sc_code_blink_tick(t_sc_code *x) {
	const double now = sc_code_now_ms();
	if (x->blink_state.expired(now))
		x->blink_state.stop();
	jbox_redraw(&x->j_box);
	if (x->blink_state.active() && x->blink_clock)
		clock_fdelay(x->blink_clock, sc_code::Blink::kStep);
}

/* Starts the blink of the range that was just evaluated. The initial repaint
 * happens right here: the keyboard path that evaluates does NOT repaint
 * afterwards (evaluating with ⌘⏎ does not change the text, so the flash is the
 * only visual signal). */
static void sc_code_blink_poke(t_sc_code *x, int start, int end) {
	if (!x->evalblink || end <= start)
		return;
	x->blink_state.start(start, end, sc_code_now_ms());
	if (x->blink_clock) {
		clock_unset(x->blink_clock);
		clock_fdelay(x->blink_clock, sc_code::Blink::kStep);
	}
	jbox_redraw(&x->j_box);
}

/* Emits the span as `eval <code>`, with the code in a SINGLE symbol: the atom
 * travels through the patch cord without going through the Max message parser, so
 * commas, `;`, newlines and quotes arrive intact (it is the same path
 * `[textedit]` → `eval` uses). No temporary file is written: the engine
 * interprets in memory (`interpretCmdLine`).
 *
 * The only thing the engine "un-escapes" are the sequences `\,` `\;` `\$`
 * (message box convention); they do not show up in real SC code, except a `\$`
 * before a character literal. S6: `snip.KEY` ⇒ the body of the snippets file
 * (defined further down). */
static void sc_code_snip_expand(t_sc_code *x, const sc_code::EvalSpan &span, const std::string &key);

static void sc_code_eval_with_mode(t_sc_code *x, sc_code::EvalMode mode) {
	const std::string &text = x->buf->text();
	sc_code::EvalSpan span =
		sc_code::sc_eval_span(text, x->buf->caret(), x->buf->selection_start(), x->buf->selection_end(), mode);

	if (!span.has_value()) {
		object_post((t_object *)x, "eval: nothing to evaluate (empty line)");
		return;
	}

	std::string code = span.text(text);

	/* S6: `snip.KEY` is not SC code. It is replaced by the snippet and the path stops
	 * here: no `eval` is sent to the engine (sc3~ does not know what `snip.kick` is)
	 * and there is no blink — the caret stays at the end of the inserted text. */
	{
		std::string snip_key;
		if (sc_code::snip_matches(code, snip_key)) {
			sc_code_snip_expand(x, span, snip_key);
			return;
		}
	}

	if (code.empty() || code[code.size() - 1] != '\n')
		code += '\n'; /* so that a trailing `//` does not eat the chunk closing */

	/* S5e: visual confirmation of WHAT was sent (the `EvalSpan` range). It goes before
	 * the outlet: if sclang posts an error, the flash is already on screen. */
	sc_code_blink_poke(x, span.start, span.end);

	/* No post to the console: evaluating is the most frequent action of the editor and
	 * the notice ("eval region (259 bytes)") was pure noise — the result already shows
	 * up in the sclang post. All that is left is the notice when there is NOTHING to
	 * evaluate. */
	t_atom a;
	atom_setsym(&a, gensym(code.c_str()));
	outlet_anything(x->out_eval, gensym("eval"), 1, &a);
}

/**********************************************************************/
/* S5b: @file and patch persistence (jsave/jload)                       */

static void sc_code_autowrite_tick(t_sc_code *x);

/* Cuts the blink in progress and forgets the range: everything that REPLACES the
 * document uses it (`@file` autoload, Load from .scd…, jload) so as not to keep
 * pointing at offsets that no longer exist. */
static void sc_code_blink_cancel(t_sc_code *x) {
	if (x->blink_clock)
		clock_unset(x->blink_clock);
	x->blink_state.stop();
}

/* true if there is a usable @file (the empty attribute means "no file"). */
static bool sc_code_has_file(t_sc_code *x) { return x->filename && x->filename->s_name[0]; }

/* Loads a `.scd` into the buffer: it is a LOAD, not an edit ⇒ it resets undo, caret
 * and scroll. On failure it does not touch the document (whatever was there stays).
 * Used by `@file` (autoload) and the context menu (Load from File…). */
static bool sc_code_load_from_path(t_sc_code *x, const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) {
		object_error((t_object *)x, "cannot open '%s'", path);
		return false;
	}
	std::string text;
	char chunk[4096];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
		text.append(chunk, n);
	const bool bad = ferror(f) != 0;
	fclose(f);
	if (bad) {
		object_error((t_object *)x, "read error on '%s'", path);
		return false;
	}
	x->buf->set_text(text);
	x->hl.reset();
	x->wraps.reset(); /* S5e: the wrap is redone with the new document */
	sc_code_blink_cancel(x);
	x->scroll.y = 0.;
	jbox_redraw(&x->j_box);
	object_post((t_object *)x, "%d bytes read from %s", (int)text.size(), path);
	return true;
}

/* Writes the document to `path` as is (UTF-8, no BOM and no trailing `\n` added). */
static bool sc_code_save_to_path(t_sc_code *x, const char *path) {
	FILE *f = fopen(path, "wb");
	if (!f) {
		object_error((t_object *)x, "cannot write '%s'", path);
		return false;
	}
	const std::string &text = x->buf->text();
	const size_t wrote = text.empty() ? 0 : fwrite(text.data(), 1, text.size(), f);
	const bool ok = wrote == text.size();
	fclose(f);
	if (!ok)
		object_error((t_object *)x, "incomplete write to '%s'", path);
	return ok;
}

/* `@file`: read/write the document of the attribute. */
static bool sc_code_read_file(t_sc_code *x) {
	return sc_code_has_file(x) && sc_code_load_from_path(x, x->filename->s_name);
}

static bool sc_code_write_file(t_sc_code *x) {
	return sc_code_has_file(x) && sc_code_save_to_path(x, x->filename->s_name);
}

/* Autowrite with DEBOUNCE (400 ms): writing on every keystroke would be one fopen per
 * character. The clock is rescheduled on every edit, so the last change is what
 * reaches the disk. */
static void sc_code_autowrite_poke(t_sc_code *x) {
	if (!x->autowrite || !sc_code_has_file(x) || !x->autowrite_clock)
		return;
	clock_unset(x->autowrite_clock);
	clock_delay(x->autowrite_clock, 400);
}

static void sc_code_autowrite_tick(t_sc_code *x) { sc_code_write_file(x); }

/**********************************************************************/
/* S6: snippets (`snip.KEY` ⇒ the body of the file)                     */
/* A text file with `### key` blocks + verbatim body (the format lives in
 * sc_snip.h). Here the file is only read and the result is plugged into the
 * buffer: parsing and matching are headless (tests/snip_test.cpp). The sclang
 * dict is NOT touched and neither is `sc3~`: the substitution is pure editor
 * text. */

/* Defined further down (leaves the caret visible and triggers the `@file`
 * autowrite). */
static void sc_code_after_key(t_sc_code *x, int line_before);

/* `~` ⇒ $HOME and one pair of surrounding quotes (`'…'` or `"…"`) removed; the rest
 * is used as is (absolute, or relative to the Max cwd). The quotes are the ones a
 * message box keeps verbatim — `snippets '~/x.txt'` arrives with them, and Max only
 * strips double quotes, and only while parsing the box — so they are dropped here
 * instead of failing the open. Same path for `@indexfile`. */
static std::string sc_code_expand_path(const char *path) {
	const std::string p = sc_code::unquote_path(path ? path : "");
	if (p == "~" || (p.size() > 1 && p[0] == '~' && p[1] == '/')) {
		if (const char *home = getenv("HOME"))
			return std::string(home) + p.substr(1);
	}
	return p;
}

/* S6c: the index is a cache of the INSTALLATION (sclang version + Quarks), not of the
 * patch, so it lives in ONE file shared by every sc_code of the machine. The default is
 * derived from where the bundle was loaded from: the .mxo sits in …/<Max>/Library, so
 * …/Library/sc_code-index.txt needs no hardcoded Max version and follows the user if
 * they move the folder. The fallback is the documented install location. */
static std::string sc_code_default_index_path(void) {
	Dl_info info;
	if (dladdr((const void *)&sc_code_default_index_path, &info) != 0 && info.dli_fname) {
		std::string p(info.dli_fname); /* …/Library/sc_code.mxo/Contents/MacOS/sc_code */
		const size_t mxo = p.find(".mxo/");
		if (mxo != std::string::npos)
			return p.substr(0, mxo) + "-index.txt";
	}
	if (const char *home = getenv("HOME"))
		return std::string(home) + "/Documents/Max 9/Library/sc_code-index.txt";
	return std::string();
}

/* @indexfile overrides the shared default (another SC installation, or a dump kept
 * somewhere else). */
static std::string sc_code_index_path(t_sc_code *x) {
	if (x->indexfile && x->indexfile->s_name && x->indexfile->s_name[0])
		return sc_code_expand_path(x->indexfile->s_name);
	return sc_code_default_index_path();
}

/* mtime + size: the hot-reload triggers if either of them changed. */
static bool sc_code_file_stamp(const std::string &path, long *mtime, long long *size) {
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	if (mtime)
		*mtime = (long)st.st_mtime;
	if (size)
		*size = (long long)st.st_size;
	return true;
}

/* Reads and parses `path`. If it cannot be read, the previous dict stays INTACT
 * but the path is remembered anyway (the hot-reload retries when the file shows
 * up). */
static bool sc_code_snip_load(t_sc_code *x, const char *path, bool verbose) {
	ScSnipState *st = x->snips;
	const std::string full = sc_code_expand_path(path);
	st->path = full;

	FILE *f = fopen(full.c_str(), "rb");
	if (!f) {
		if (verbose)
			object_error((t_object *)x, "snippets: cannot open '%s'", full.c_str());
		st->mtime = 0;
		st->size = -1; /* forces a new attempt on the next `snip.*` */
		return false;
	}
	std::string text;
	char chunk[4096];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
		text.append(chunk, n);
	const bool bad = ferror(f) != 0;
	fclose(f);
	if (bad) {
		if (verbose)
			object_error((t_object *)x, "snippets: read error on '%s'", full.c_str());
		st->mtime = 0;
		st->size = -1;
		return false;
	}

	st->dict = sc_code::parse_snippets(text);
	if (!sc_code_file_stamp(full, &st->mtime, &st->size))
		st->size = (long long)text.size();

	/* Notices once per LOAD (not per eval): the declared keys that did not make it
	 * in. */
	for (const std::string &k : st->dict.duplicates)
		object_post((t_object *)x, "snippets: duplicate `### %s` — the first one wins", k.c_str());
	for (const std::string &k : st->dict.empty)
		object_post((t_object *)x, "snippets: `### %s` has no body — ignored", k.c_str());
	object_post((t_object *)x, "snippets: %d entries from %s", (int)st->dict.size(), full.c_str());
	return true;
}

/* Hot-reload: called ONLY when the eval was a `snip.KEY` (one stat per invocation). If
 * the file disappeared, whatever was already loaded keeps being used. */
static void sc_code_snip_maybe_reload(t_sc_code *x) {
	ScSnipState *st = x->snips;
	if (st->path.empty())
		return;
	long mtime = 0;
	long long size = 0;
	if (!sc_code_file_stamp(st->path, &mtime, &size))
		return;
	if (mtime == st->mtime && size == st->size)
		return;
	sc_code_snip_load(x, st->path.c_str(), true);
}

/* Replaces the evaluated range with the snippet body: text ONLY — nothing is
 * evaluated and no `eval` is emitted —, caret at the end, one undo step. If the key
 * is missing (or the file could not be read) the buffer is not touched and a notice
 * goes to the Max console. */
static void sc_code_snip_expand(t_sc_code *x, const sc_code::EvalSpan &span, const std::string &key) {
	sc_code_snip_maybe_reload(x);
	ScSnipState *st = x->snips;
	const std::string *body = st->dict.find(key);
	if (!body) {
		const std::string have = st->dict.keys_list();
		if (st->path.empty())
			object_error((t_object *)x, "snip.%s: no snippets file (@snippets <path>)",
						 key.c_str());
		else if (have.empty())
			object_error((t_object *)x, "snip.%s: not found (%s has no snippets)", key.c_str(),
						 st->path.c_str());
		else
			object_error((t_object *)x, "snip.%s: not found in %s (has: %s)", key.c_str(),
						 st->path.c_str(), have.c_str());
		return;
	}
	if (x->readonly) {
		object_error((t_object *)x, "snip.%s: the editor is read-only (@readonly 1)", key.c_str());
		return;
	}
	const int line_before = x->buf->line_of_offset(span.start);
	const int len_before = x->buf->length();
	x->buf->set_selection(span.start, span.end);
	if (!x->buf->insert_text(*body)) /* replaces the selection: one undo step */
		return;
	sc_code_after_key(x, line_before);
	if (x->autowrite && x->buf->length() != len_before)
		sc_code_autowrite_poke(x);
	jbox_redraw(&x->j_box);
}

/* `@snippets <path>` — and the `snippets <path>` message through the inlet, which
 * is the same path (Max attributes are messages): (re)loads the file. Empty = no
 * snippets (the dict is dropped). The path arrives as ONE symbol from the patch
 * (`@snippets "/path with spaces"`), but a message box splits the text on spaces:
 * they are joined back so that `snippets ~/Documents/Max
 * 9/Library/sc_code-snippets.txt` works. Quotes survive that join (`snippets
 * '~/Documents/Max 9/Library/x.txt'`) and are removed when the path is opened
 * (`sc_code_expand_path`). */
static t_max_err sc_code_set_snippets(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	std::string path;
	for (long i = 0; i < argc && argv; ++i) {
		if (atom_gettype(argv + i) != A_SYM)
			continue;
		if (t_symbol *sym = atom_getsym(argv + i)) {
			if (!path.empty())
				path += ' ';
			path += sym->s_name;
		}
	}
	if (path.empty()) {
		x->snipfile = NULL;
		x->snips->path.clear();
		x->snips->dict = sc_code::Snippets();
		x->snips->mtime = 0;
		x->snips->size = 0;
		return MAX_ERR_NONE;
	}
	/* What gets STORED (and saved with the patch) is the path without its quotes; the
	 * `~` is kept so the patch stays portable. */
	const std::string stored = sc_code::unquote_path(path);
	x->snipfile = gensym(stored.c_str());
	sc_code_snip_load(x, stored.c_str(), true);
	return MAX_ERR_NONE;
}

/* `snippets <path>` through the inlet. It is registered as a METHOD besides the
 * attribute: the "attribute as message" path is not guaranteed for objects that do not
 * call `attr_args_process`, and this is the path the user takes from a message box
 * (30-oct: `snip.init` warned "no file" ⇒ the setter was not being called). */
static void sc_code_snippets_msg(t_sc_code *x, t_symbol *s, long argc, t_atom *argv) {
	(void)s;
	sc_code_set_snippets(x, NULL, argc, argv);
}

/**********************************************************************/
/* S6c: index of the class library ('index' message, SCODE.md §10.1)    */
/* sc_code never talks to the engine, so the answer travels through a file: this
 * message only EMITS the generator through the 'eval' outlet (it is one expression
 * => one 'eval') and sclang writes the dump. The bar picks the file up later
 * (§10.2), so nothing here waits for it.
 * Nothing is generated on its own: emitting code the user did not write is exactly
 * what was ruled out. 'index <path>' overrides @indexfile for one shot. */

static void sc_code_index_msg(t_sc_code *x, t_symbol *s, long argc, t_atom *argv) {
	(void)s;
	std::string path;
	if (argc > 0) {
		/* A message box splits the text on spaces: join them back (same lesson as
		 * @snippets, 0.9.0). */
		for (long i = 0; i < argc; ++i) {
			t_symbol *sym = atom_getsym(argv + i);
			if (i)
				path += ' ';
			path += sym ? sym->s_name : "";
		}
	} else {
		path = sc_code_index_path(x);
	}
	if (path.empty()) {
		object_error((t_object *)x, "index: cannot resolve the path (set @indexfile <path>)");
		return;
	}
	object_post((t_object *)x, "index: writing %s (one file shared by every sc_code)", path.c_str());
	const std::string code = sc_code::index_generator_code(path);
	t_atom a;
	atom_setsym(&a, gensym(code.c_str()));
	outlet_anything(x->out_eval, gensym("eval"), 1, &a);
}

/* S6c step 2b: the dump is read HERE, not by the message above (sclang writes it
 * asynchronously and nothing in `sc_code` waits for it). The stamp is `stat`ed on every
 * paint — two syscalls, cheaper than a timer and it can never leave the bar stale —
 * and the 1,16 MB file is parsed ONLY when mtime/size changed (once per regeneration,
 * §10.1). `@callaid 0` costs nothing: no stat, no bar. */
static void sc_code_index_refresh(t_sc_code *x) {
	if (!x->callaid || !x->index)
		return;
	const std::string path = sc_code_index_path(x);
	if (path.empty())
		return;
	long mtime = 0;
	long long size = 0;
	if (!sc_code_file_stamp(path, &mtime, &size)) {
		/* Never generated: ONE notice per object (the index is shared by the whole
		 * installation, so generating it from any `sc_code` fixes this one too). */
		if (!x->index_warned) {
			x->index_warned = 1;
			object_post((t_object *)x, "call aid: no index yet — send `index` to write %s",
						path.c_str());
		}
		return;
	}
	if (path == x->index_loaded && mtime == x->index_mtime && size == x->index_size)
		return;
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return;
	std::string text;
	char chunk[4096];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
		text.append(chunk, n);
	fclose(f);
	if (!x->index->load(text)) {
		/* Half written (no `#end`): the all-or-nothing rule of sc_index keeps the
		 * previous answer and the next paint retries by itself. */
		object_error((t_object *)x, "call aid: %s has no `#end` (half written?) — ignored",
					 path.c_str());
		x->index_loaded.clear();
		return;
	}
	x->index_loaded = path;
	x->index_mtime = mtime;
	x->index_size = size;
	const std::string ver = x->index->version();
	if (ver.empty())
		object_post((t_object *)x, "call aid: %d entries from %s", x->index->entries(), path.c_str());
	else
		object_post((t_object *)x, "call aid: %d entries (sclang %s) from %s", x->index->entries(),
					ver.c_str(), path.c_str());
}

/* `jsave`: the TEXT is not an attribute (it is the document state) ⇒ it goes into
 * the patch dictionary. The caret and the scroll are saved with it so that
 * reopening the patch leaves the editor where it was.
 *
 * PREFIXED KEYS (`sc_code_…`): the dictionary of a box ALREADY has `text` (the
 * object text, "sc_code") and other Max keys; writing `text` there can overwrite
 * it. All our keys are namespaced. */
#define SC_KEY_TEXT "sc_code_text"
#define SC_KEY_CARET "sc_code_caret"
#define SC_KEY_SCROLL "sc_code_scroll"

static t_max_err sc_code_jsave(t_sc_code *x, t_dictionary *d) {
	dictionary_appendstring(d, gensym(SC_KEY_TEXT), x->buf->text().c_str());
	dictionary_appendlong(d, gensym(SC_KEY_CARET), x->buf->caret());
	dictionary_appendlong(d, gensym(SC_KEY_SCROLL), (t_atom_long)std::lround(x->scroll.y));
	return MAX_ERR_NONE;
}

static t_max_err sc_code_jload(t_sc_code *x, t_dictionary *d) {
	const char *text = NULL;
	if (dictionary_getstring(d, gensym(SC_KEY_TEXT), &text) == MAX_ERR_NONE && text) {
		x->buf->set_text(std::string(text));
		x->hl.reset();
		x->wraps.reset();
	}
	t_atom_long caret = 0, scroll = 0;
	if (dictionary_getlong(d, gensym(SC_KEY_CARET), &caret) == MAX_ERR_NONE)
		x->buf->set_caret((int)caret);
	if (dictionary_getlong(d, gensym(SC_KEY_SCROLL), &scroll) == MAX_ERR_NONE)
		x->scroll.y = (double)scroll;
	jbox_redraw(&x->j_box);
	return MAX_ERR_NONE;
}

/* --- attribute setters (the ones that need an effect, not just storage) ----- */

static t_max_err sc_code_set_fontsize(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	double v = atom_getfloat(argv);
	if (v < 6.)
		v = 6.;
	if (v > 48.)
		v = 48.;
	x->font_size = v;
	x->metrics_for = -1.; /* the next paint measures again */
	jbox_redraw(&x->j_box);
	return MAX_ERR_NONE;
}

static t_max_err sc_code_set_tabwidth(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	long v = atom_getlong(argv);
	x->tabwidth = v < 1 ? 1 : (v > 16 ? 16 : v);
	return MAX_ERR_NONE;
}

static t_max_err sc_code_set_autoclose(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	x->autoclose = atom_getlong(argv) != 0;
	x->buf->set_auto_close(x->autoclose != 0);
	return MAX_ERR_NONE;
}

static t_max_err sc_code_set_autowrite(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	x->autowrite = atom_getlong(argv) != 0;
	if (x->autowrite && sc_code_has_file(x))
		sc_code_autowrite_poke(x); /* whatever was already typed is saved anyway */
	return MAX_ERR_NONE;
}

/* `@blinkduration` (ms): applied to the blink state with a clamp. A blink in
 * progress finishes with the NEW duration (the alpha is recomputed with
 * `duration()`); there is no need to restart it. */
static t_max_err sc_code_set_blinkduration(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	x->blink_state.set_duration(atom_getfloat(argv));
	x->blinkduration = (long)(x->blink_state.duration() + 0.5);
	return MAX_ERR_NONE;
}

/* `@scrollspeed` (<float>, default 3.): factor of the `mousewheel` delta. Max sends
 * the trackpad delta in small units and many per second ⇒ with factor 1 the scroll
 * ends up slow. Clamp [0.05, 50]: 0 would leave the wheel dead (confusing) and more
 * than 50 is a jump per event. */
static t_max_err sc_code_set_scrollspeed(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	double v = atom_getfloat(argv);
	if (v < 0.05)
		v = 0.05;
	if (v > 50.)
		v = 50.;
	x->scrollspeed = v;
	return MAX_ERR_NONE;
}

/* Same path as a message through the inlet (`scrollspeed 4.`): the attribute
 * alone does not create the message (see `snippets`, 0.9.0). */
static void sc_code_scrollspeed_msg(t_sc_code *x, double v) {
	t_atom a[1];
	atom_setfloat(a, v);
	sc_code_set_scrollspeed(x, NULL, 1, a);
}

/**********************************************************************/
/* drops from the Finder                                               */

/* Drag & drop of files from the Finder. Measured, not guessed (SCODE.md §12):
 * `ext_drag.h` says it only covers the Max file browser, but the drag of a Finder
 * file is delivered through the same `jdrag` object, so the road is the one
 * `buffer~` and `sfplay~` use — `acceptsdrag_locked` / `acceptsdrag_unlocked` — plus
 * a plain message that Max calls when the file is released:
 *
 *   1. `acceptsdrag_*` (x, drag, view): announce a drop action with `jdrag_add` and
 *      return 1 (0 refuses the drop and Max shows "unlock the patcher…").
 *   2. `<msg>` (x, drag): `jdrag_itemcount` + `jdrag_getitemstring` give the
 *      pathnames, in Max's volume dialect (`Macintosh HD:/Users/...`).
 *
 * `dropfile.mxo` (the only stock external that also registers `candropfiles` /
 * `dropfiles`) confirmed the atom layout, but those two never fired on a box drop;
 * the accepted-drag pair does. Dropping the volume prefix, expanding `~` and quoting
 * live in `sc_path.cpp` (headless, gated) ⇒ what lands at the caret is an ABSOLUTE
 * path (`"/Users/g/…/x.wav"`): sclang does not expand `~`, and a path you can paste
 * into `SoundFile.openRead` without editing it is the point. The insertion uses the
 * typing path ⇒ a single undo step. */
static void sc_code_insert_drop(t_sc_code *x, const std::vector<std::string> &names) {
	if (names.empty() || x->readonly || !x->buf)
		return;

	const char *home = getenv("HOME");
	const std::string text = sc_code::dropped_text(home ? home : "", names);
	if (text.empty())
		return;

	const int line_before = x->buf->line_of_offset(x->buf->caret());
	x->buf->insert_text(text); /* replaces the selection: one undo step */
	sc_code_after_key(x, line_before);
	if (x->autowrite)
		sc_code_autowrite_poke(x);
	jbox_redraw(&x->j_box);
}

/* Same body for the locked and the unlocked patcher: in both cases the answer is
 * "yes, I take it". Measured: this is called on EVERY mouse move of the drag (not
 * once), so it has to stay cheap and silent, and the role filter cannot use
 * `jdrag_matchdragrole(drag, gensym("file"))` — a Finder WAV matches `audiofile`
 * only (`file` gives 0). The guard is instead that the first item has to resolve to
 * a file that exists (`stat`), which is what separates a Finder drop from dragging a
 * Max object over the box. */
static long sc_code_acceptsdrag(t_sc_code *x, t_object *drag, t_object *view) {
	(void)view;
	if (x->readonly || !x->buf || !drag)
		return 0;
	if (jdrag_itemcount(drag) <= 0)
		return 0;
	const char *item = jdrag_getitemstring(drag, 0, 0, 0);
	if (!item || !item[0])
		return 0;
	const std::string path = sc_code::path_to_posix(item);
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return 0;
	jdrag_add(drag, (t_object *)x, gensym("dropfile"), "Insert path", NULL);
	return 1;
}

/* Called by Max when the drag is released on the box (registered A_CANT, the drag
 * object arrives as the second argument, exactly as in `dropfile.mxo`). */
static void sc_code_dropfile(t_sc_code *x, t_object *drag) {
	if (x->readonly || !x->buf || !drag)
		return;
	const long count = jdrag_itemcount(drag);
	std::vector<std::string> names;
	for (long i = 0; i < count; ++i) {
		const char *name = jdrag_getitemstring(drag, i, 0, 0);
		if (!name || !name[0])
			continue;
		names.push_back(name);
	}
	sc_code_insert_drop(x, names);
}

static t_max_err sc_code_set_file(t_sc_code *x, t_object *attr, long argc, t_atom *argv) {
	if (argc && argv && atom_gettype(argv) == A_SYM)
		x->filename = atom_getsym(argv);
	else
		x->filename = NULL;
	if (sc_code_has_file(x) && x->autoload)
		sc_code_read_file(x);
	return MAX_ERR_NONE;
}

/**********************************************************************/
/* life cycle                                                          */

static double sc_code_bar_height(t_sc_code *x);

static t_sc_code *sc_code_new(t_symbol *s, long argc, t_atom *argv);
static void sc_code_free(t_sc_code *x);
static void sc_code_paint(t_sc_code *x, t_object *view);
static long sc_code_key(t_sc_code *x, t_object *patcherview, long keycode, long modifiers, long textcharacter);
static long sc_code_keyfilter(t_sc_code *x, t_object *patcherview, long *keycode, long *modifiers,
							  long *textcharacter);
static void sc_code_focusgained(t_sc_code *x, t_object *patcherview);
static void sc_code_focuslost(t_sc_code *x, t_object *patcherview);
static void sc_code_mouseenter(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers);
static long sc_code_mousedown(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers);
static void sc_code_mousedrag(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers);
static void sc_code_mouseup(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers);
static void sc_code_mousewheel(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers, double delta_x,
							   double delta_y);
static void sc_code_getdrawparams(t_sc_code *x, t_object *patcherview, t_jboxdrawparams *params);

C74_EXPORT void ext_main(void *r) {
	t_class *c;

	/* SDK prerequisite for UI objects (bug #17): it fills the static t_symbol*
	 * tables that jbox_new() uses. Requires compiling jpatcher_syms.c into the
	 * bundle (see Makefile). */
	jpatcher_syms_init();

	c = class_new("sc_code", (method)sc_code_new, (method)sc_code_free, sizeof(t_sc_code), (method)NULL,
				  A_GIMME, 0L);

	c->c_flags |= CLASS_FLAG_NEWDICTIONARY; /* UI object: Max passes a dictionary (see jbox_new) */
	jbox_initclass(c, 0);

	class_addmethod(c, (method)sc_code_paint, "paint", A_CANT, 0);
	class_addmethod(c, (method)sc_code_key, "key", A_CANT, 0);
	class_addmethod(c, (method)sc_code_keyfilter, "keyfilter", A_CANT, 0); /* K0: Max does not call it */
	class_addmethod(c, (method)sc_code_focusgained, "focusgained", A_CANT, 0);
	class_addmethod(c, (method)sc_code_focuslost, "focuslost", A_CANT, 0);
	class_addmethod(c, (method)sc_code_mouseenter, "mouseenter", A_CANT, 0);
	class_addmethod(c, (method)sc_code_mousedown, "mousedown", A_CANT, 0);
	class_addmethod(c, (method)sc_code_mousedrag, "mousedrag", A_CANT, 0);
	class_addmethod(c, (method)sc_code_mouseup, "mouseup", A_CANT, 0);
	class_addmethod(c, (method)sc_code_mousewheel, "mousewheel", A_CANT, 0);
	class_addmethod(c, (method)sc_code_getdrawparams, "getdrawparams", A_CANT, 0);
	class_addmethod(c, (method)sc_code_jsave, "jsave", A_CANT, 0);
	class_addmethod(c, (method)sc_code_jload, "jload", A_CANT, 0);

	CLASS_ATTR_DEFAULT(c, "rect", 0, "0. 0. 420. 140.");

	/* --- S5b: font, tabbing and behavior ------------------------------------ */
	CLASS_ATTR_DOUBLE(c, "fontsize", 0, t_sc_code, font_size);
	CLASS_ATTR_ACCESSORS(c, "fontsize", NULL, (method)sc_code_set_fontsize);
	CLASS_ATTR_DEFAULT_SAVE(c, "fontsize", 0, "12.");

	CLASS_ATTR_LONG(c, "tabwidth", 0, t_sc_code, tabwidth);
	CLASS_ATTR_ACCESSORS(c, "tabwidth", NULL, (method)sc_code_set_tabwidth);
	CLASS_ATTR_DEFAULT_SAVE(c, "tabwidth", 0, "4");

	CLASS_ATTR_LONG(c, "linenumbers", 0, t_sc_code, linenumbers);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "linenumbers", 0, "1");

	CLASS_ATTR_LONG(c, "readonly", 0, t_sc_code, readonly);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "readonly", 0, "0");

	CLASS_ATTR_LONG(c, "autoclose", 0, t_sc_code, autoclose);
	CLASS_ATTR_ACCESSORS(c, "autoclose", NULL, (method)sc_code_set_autoclose);
	CLASS_ATTR_DEFAULT_SAVE(c, "autoclose", 0, "1");

	CLASS_ATTR_LONG(c, "autowrite", 0, t_sc_code, autowrite);
	CLASS_ATTR_ACCESSORS(c, "autowrite", NULL, (method)sc_code_set_autowrite);
	CLASS_ATTR_DEFAULT_SAVE(c, "autowrite", 0, "0");

	CLASS_ATTR_LONG(c, "autoload", 0, t_sc_code, autoload);
	CLASS_ATTR_DEFAULT_SAVE(c, "autoload", 0, "1");

	CLASS_ATTR_SYM(c, "file", 0, t_sc_code, filename);
	CLASS_ATTR_ACCESSORS(c, "file", NULL, (method)sc_code_set_file);

	/* S6: snippets file (`snip.KEY`). It is saved with the patch (SAVE) and can be set
	 * in the inspector (`@snippets <path>`) or sent as a message through the inlet. */
	CLASS_ATTR_SYM(c, "snippets", 0, t_sc_code, snipfile);
	CLASS_ATTR_ACCESSORS(c, "snippets", NULL, (method)sc_code_set_snippets);
	CLASS_ATTR_SAVE(c, "snippets", 0);
	/* Same name as a direct message through the inlet (see sc_code_snippets_msg). */
	class_addmethod(c, (method)sc_code_snippets_msg, "snippets", A_GIMME, 0);

	/* S6c: where sclang writes the class library index (§10.1). By default it is ONE file
	 * per installation (…/Max 9/Library/sc_code-index.txt, derived from the bundle path);
	 * this attribute only OVERRIDES it, and it is saved with the patch. */
	CLASS_ATTR_SYM(c, "indexfile", 0, t_sc_code, indexfile);
	CLASS_ATTR_SAVE(c, "indexfile", 0);
	/* 'index' / 'index <path>': emits the generator through the eval outlet. */
	class_addmethod(c, (method)sc_code_index_msg, "index", A_GIMME, 0);

	/* S6c step 2b: the bottom bar with the signature of the call under the caret
	 * (§10.2). `@callaid 1` (default) RESERVES one line at the bottom, so the caret can
	 * never end up behind it; `0` gives the whole box back to the text. Display only:
	 * no key is intercepted and the buffer is never written. */
	CLASS_ATTR_LONG(c, "callaid", 0, t_sc_code, callaid);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "callaid", 0, "1");

	CLASS_ATTR_RGBA(c, "barcolor", 0, t_sc_code, barcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "barcolor", 0, "0.125 0.135 0.160 1.");

	CLASS_ATTR_RGBA(c, "barhighlight", 0, t_sc_code, barhighlight);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "barhighlight", 0, "1.0 0.55 0.15 1.");

	/* --- S5b: colors (RGBA, saved with the patch) --------------------------- */
	CLASS_ATTR_RGBA(c, "bgcolor", 0, t_sc_code, bgcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "bgcolor", 0, "0.105 0.110 0.130 1.");

	CLASS_ATTR_RGBA(c, "textcolor", 0, t_sc_code, textcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "textcolor", 0, "0.900 0.910 0.930 1.");

	CLASS_ATTR_RGBA(c, "selcolor", 0, t_sc_code, selcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "selcolor", 0, "0.280 0.420 0.720 0.40");

	CLASS_ATTR_RGBA(c, "caretcolor", 0, t_sc_code, caretcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "caretcolor", 0, "1.0 0.55 0.15 1.");

	CLASS_ATTR_RGBA(c, "keywordcolor", 0, t_sc_code, keywordcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "keywordcolor", 0, "0.80 0.62 0.95 1.");

	CLASS_ATTR_RGBA(c, "builtincolor", 0, t_sc_code, builtincolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "builtincolor", 0, "0.55 0.85 0.85 1.");

	CLASS_ATTR_RGBA(c, "primitivecolor", 0, t_sc_code, primitivecolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "primitivecolor", 0, "0.90 0.60 0.90 1.");

	CLASS_ATTR_RGBA(c, "classcolor", 0, t_sc_code, classcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "classcolor", 0, "0.55 0.78 0.95 1.");

	CLASS_ATTR_RGBA(c, "symbolcolor", 0, t_sc_code, symbolcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "symbolcolor", 0, "0.95 0.72 0.42 1.");

	CLASS_ATTR_RGBA(c, "stringcolor", 0, t_sc_code, stringcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "stringcolor", 0, "0.65 0.85 0.55 1.");

	CLASS_ATTR_RGBA(c, "charcolor", 0, t_sc_code, charcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "charcolor", 0, "0.85 0.86 0.50 1.");

	CLASS_ATTR_RGBA(c, "numbercolor", 0, t_sc_code, numbercolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "numbercolor", 0, "0.62 0.75 1.0 1.");

	CLASS_ATTR_RGBA(c, "envvarcolor", 0, t_sc_code, envvarcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "envvarcolor", 0, "0.95 0.60 0.45 1.");

	CLASS_ATTR_RGBA(c, "commentcolor", 0, t_sc_code, commentcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "commentcolor", 0, "0.48 0.56 0.50 1.");

	CLASS_ATTR_RGBA(c, "bracketcolor", 0, t_sc_code, bracketcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "bracketcolor", 0, "0.78 0.80 0.86 1.");

	CLASS_ATTR_RGBA(c, "operatorcolor", 0, t_sc_code, operatorcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "operatorcolor", 0, "0.85 0.85 0.92 1.");

	/* --- S5c: title bar and bracket matching ------------------------------- */
	CLASS_ATTR_LONG(c, "titlebar", 0, t_sc_code, titlebar);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "titlebar", 0, "1");

	CLASS_ATTR_RGBA(c, "matchcolor", 0, t_sc_code, matchcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "matchcolor", 0, "0.35 0.48 0.70 0.45");

	CLASS_ATTR_LONG(c, "matchbrackets", 0, t_sc_code, matchbrackets);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "matchbrackets", 0, "1");

	/* --- S5e: width wrap and evaluation blink ----------------------------- */
	CLASS_ATTR_LONG(c, "wrap", 0, t_sc_code, wrap);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "wrap", 0, "1");

	CLASS_ATTR_LONG(c, "blink", 0, t_sc_code, evalblink);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "blink", 0, "1");

	CLASS_ATTR_LONG(c, "blinkduration", 0, t_sc_code, blinkduration);
	CLASS_ATTR_ACCESSORS(c, "blinkduration", NULL, (method)sc_code_set_blinkduration);
	CLASS_ATTR_DEFAULT_SAVE(c, "blinkduration", 0, "500");

	CLASS_ATTR_RGBA(c, "blinkcolor", 0, t_sc_code, blinkcolor);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "blinkcolor", 0, "1.0 0.55 0.15 0.40");

	/* --- 0.9.1: wheel / trackpad speed ------------------------------------- */
	CLASS_ATTR_DOUBLE(c, "scrollspeed", 0, t_sc_code, scrollspeed);
	CLASS_ATTR_ACCESSORS(c, "scrollspeed", NULL, (method)sc_code_set_scrollspeed);
	CLASS_ATTR_DEFAULT_SAVE(c, "scrollspeed", 0, "3.");
	class_addmethod(c, (method)sc_code_scrollspeed_msg, "scrollspeed", A_FLOAT, 0);

	/* --- 0.9.5: files dropped from the Finder (SCODE.md §12) ---------------- */
	class_addmethod(c, (method)sc_code_acceptsdrag, "acceptsdrag_locked", A_CANT, 0);
	class_addmethod(c, (method)sc_code_acceptsdrag, "acceptsdrag_unlocked", A_CANT, 0);
	class_addmethod(c, (method)sc_code_dropfile, "dropfile", A_CANT, 0);

	class_register(CLASS_BOX, c);
	s_sc_code_class = c;
}

static t_sc_code *sc_code_new(t_symbol *s, long argc, t_atom *argv) {
	t_dictionary *d = NULL;
	t_sc_code *x = NULL;

	if (!(d = object_dictionaryarg(argc, argv)))
		return NULL;

	x = (t_sc_code *)object_alloc(s_sc_code_class);
	if (!x)
		return NULL;

	long flags = 0
				 | JBOX_DRAWFIRSTIN
				 | JBOX_TRANSPARENT
				 | JBOX_DRAWINLAST
				 | JBOX_HILITE /* takes focus on click: without focus there are no keys */
				 /* Without a growth flag, Max only lets the width grow: the editor has to
				  * be able to grow in height (that is the whole point of the scroll).
				  * GROWBOTH = independent x and y. */
				 | JBOX_GROWBOTH
		;

	jbox_new(&x->j_box, flags, argc, argv);
	x->j_box.b_firstin = (t_object *)x;

	x->buf = new sc_code::Buffer();
	x->out_eval = outlet_new((t_object *)x, NULL);
	/* S5b: what is NOT an attribute (file pointer, clock) plus the defaults a
	 * setter may consult BEFORE attr_dictionary_process applies them (the order of
	 * the dictionary attributes is not guaranteed: `file` needs `autoload` already
	 * set). */
	x->linenumbers = 1;
	x->readonly = 0;
	x->autoclose = 1;
	x->autowrite = 0;
	x->autoload = 1;
	x->filename = NULL;
	x->snipfile = NULL;
	x->snips = new ScSnipState(); /* S6: empty dict until `@snippets` */
	x->indexfile = NULL;          /* S6c: no index until `@indexfile` */
	x->index = new sc_code::ClassIndex(); /* empty until the dump is read */
	x->index_mtime = 0;
	x->index_size = -1;
	x->index_warned = 0;
	x->completer = new sc_code::Completer();
	x->compl_open = 0;
	x->compl_cycle = 0;
	x->compl_start = x->compl_end = 0;
	x->compl_alternatives = 0;
	/* S6c step 2b: the bar is on by default and its colors are attributes (applied
	 * later by attr_dictionary_process; these are the values a paint BEFORE that
	 * would use). */
	x->callaid = 1;
	x->bar_h = 0.;
	x->bar_grab = 0;
	x->gutter_w = 0.;
	x->autowrite_clock = clock_new(x, (method)sc_code_autowrite_tick);
	x->titlebar = 1;
	x->title_h = 0.;
	x->matchbrackets = 1;
	/* S5e: width wrap on (it is the useful default) and evaluation blink on, with the
	 * caret orange as the factory color (changed with @blinkcolor). */
	x->wrap = 1;
	x->text_cols = 0;
	x->evalblink = 1;
	x->blinkduration = (long)sc_code::Blink::kDefaultDuration;
	x->blink_clock = clock_new(x, (method)sc_code_blink_tick);
	/* `object_alloc` returns zeroed memory and the C++ members (wrap vectors,
	 * blink state) rely on that; they are still set explicitly, like `x->scroll =
	 * ScrollView()` in S5a. */
	x->wraps.reset();
	x->blink_state.stop();
	x->blink_state.set_duration((double)x->blinkduration);
	/* Auto-close of pairs on: the user asked for it and it is what every code
	 * editor does (the IDE has it as a setting, default off, §3.11). It is now the
	 * `autoclose` attribute. */
	x->buf->set_auto_close(x->autoclose != 0);
	x->focused = 0;
	x->tabwidth = 4;
	x->click_count = 0;
	x->last_click_x = x->last_click_y = 0;
	x->last_click_ms = 0.;
	x->dragging = 0;
	x->font_size = 12.;
	x->char_w = x->line_h = x->ascent = 0.;
	x->metrics_for = -1.;
	x->scroll = sc_code::ScrollView();
	x->view_w = x->view_h = 0.;
	x->sb_drag = 0;
	x->sb_grab = 0.;
	x->scrollspeed = 3.;

	attr_dictionary_process((t_object *)x, d); /* box attributes (rect, …) */
	/* Text stored by `jsave`: it wins over `@file` (it is the exact state the patch
	 * was saved with). If there is no stored text it returns an error and touches
	 * nothing. */
	sc_code_jload(x, d);
	jbox_ready(&x->j_box);
	return x;
}

static void sc_code_free(t_sc_code *x) {
	/* A pending autowrite is not lost when the object is deleted. */
	if (x->blink_clock) {
		clock_unset(x->blink_clock);
		clock_free(x->blink_clock);
		x->blink_clock = NULL;
	}
	x->blink_state.stop();
	if (x->autowrite_clock) {
		clock_unset(x->autowrite_clock);
		if (x->autowrite)
			sc_code_write_file(x);
		clock_free(x->autowrite_clock);
		x->autowrite_clock = NULL;
	}
	if (x->buf) {
		delete x->buf;
		x->buf = NULL;
	}
	if (x->snips) {
		delete x->snips;
		x->snips = NULL;
	}
	if (x->index) {
		delete x->index;
		x->index = NULL;
	}
	if (x->completer) {
		delete x->completer;
		x->completer = NULL;
	}
	jbox_free(&x->j_box);
}

/**********************************************************************/
/* paint                                                               */

/* Monospace metrics: measured ONCE per font size (paint cannot allocate memory or
 * measure text per line). The caller has already set the font. */
static void sc_code_update_metrics(t_sc_code *x, t_jgraphics *g) {
	if (x->metrics_for == x->font_size && x->char_w > 0.)
		return;
	double w = 0., h = 0.;
	t_jgraphics_font_extents fe;
	jgraphics_text_measure(g, "M", &w, &h);
	jgraphics_font_extents(g, &fe);
	x->char_w = w > 0. ? w : x->font_size * 0.6;
	x->line_h = (fe.ascent + fe.descent) + 2.;
	x->ascent = fe.ascent;
	x->metrics_for = x->font_size;
}

/* How many CELLS a UTF-8 chunk takes (one code point = one cell: monospace).
 * Continuation bytes (10xxxxxx) do not add up. */
static int sc_code_utf8_cells(const char *s, int len) {
	int cells = 0;
	for (int i = 0; i < len; ++i)
		if (((unsigned char)s[i] & 0xC0) != 0x80)
			++cells;
	return cells;
}

/* Bytes taken by the first `cells` cells of a UTF-8 chunk (inverse of the one
 * above). The wrap uses it to trim a token to the visual row it belongs to. */
static int sc_code_utf8_bytes(const char *s, int cells) {
	int i = 0, n = 0;
	while (s[i] && n < cells) {
		++i; /* the initial byte of the code point */
		while (((unsigned char)s[i] & 0xC0) == 0x80)
			++i;
		++n;
	}
	return i;
}

/* Scroll geometry from the last thing paint saw: the scroll is pixels
 * computed over metrics ALREADY measured (text is never measured here).
 *
 * S5e: the vertical unit is the VISUAL ROW (wrap), not the logical line: `sc_scroll`
 * gets `row_count()` in `line_count`, so its arithmetic (first/last visible row,
 * content height, thumb) does not change and the wrap comes for free there. */
static sc_code::ScrollMetrics sc_code_metrics(t_sc_code *x) {
	x->wraps.ensure(*x->buf, x->text_cols > 0 ? x->text_cols : SC_NO_WRAP);
	sc_code::ScrollMetrics m;
	m.line_count = x->wraps.row_count();
	m.line_h = x->line_h > 0. ? x->line_h : 16.;
	/* The title bar is NOT a text area: the content and the scroll live below it.
	 * Everything vertical from here down is measured INSIDE that area. S6c: the same goes
	 * for the reserved bottom bar, so `ensure_line`/`clamp`/page up/down already keep the
	 * caret ABOVE it (there is no extra clamp to forget). */
	m.view_h = x->view_h - x->title_h - x->bar_h;
	if (m.view_h < 0.)
		m.view_h = 0.;
	m.pad_y = PAD_Y;
	return m;
}

/* --- line-number gutter (S5b) ----------------------------------------- */

static int sc_code_digits(int n) {
	int d = 1;
	while (n >= 10) {
		n /= 10;
		++d;
	}
	return d;
}

/* Width of the number column, in px (0 = gutter off). It depends on the
 * metrics (char_w) and on the document height ⇒ it is recomputed on every
 * paint. */
static double sc_code_gutter_width(t_sc_code *x) {
	if (!x->linenumbers || x->char_w <= 0.)
		return 0.;
	return (double)sc_code_digits(x->buf->line_count()) * x->char_w + GUTTER_GAP;
}

/* x where the TEXT starts: the gutter shifts the column, NOT the scroll (the wheel and
 * `offset_at` keep measuring from here, so the vertical arithmetic does not change). */
static double sc_code_text_x(t_sc_code *x) { return x->gutter_w + PAD_X; }

/* --- S5e: wrap by cells ------------------------------------------------ */

/* Cells that fit per row at the CURRENT width of the box. The usable width ALWAYS
 * reserves room for the scroll bar: if the wrap depended on whether the bar is
 * visible, "there is a bar" would change the width, which would change the rows,
 * which would change "there is a bar" (a circle). `@wrap 0` ⇒ giant width ⇒ one row
 * per line. */
static int sc_code_cols_for_width(t_sc_code *x, double width) {
	if (!x->wrap || x->char_w <= 0.)
		return SC_NO_WRAP;
	double avail = width - sc_code_text_x(x) - (BAR_W + 2.);
	if (avail < x->char_w)
		avail = x->char_w;
	const int cols = (int)std::floor(avail / x->char_w);
	return cols < 1 ? 1 : cols;
}

/* Global document row for a y coordinate of the box (with clamp). */
static int sc_code_row_of_y(t_sc_code *x, double y) {
	if (x->line_h <= 0.)
		return 0;
	int row = (int)std::floor((y - x->title_h - PAD_Y + x->scroll.y) / x->line_h);
	if (row < 0)
		row = 0;
	if (row > x->wraps.row_count() - 1)
		row = x->wraps.row_count() - 1;
	return row;
}

/* Global row where a buffer offset falls (caret, bracket, range…). */
static int sc_code_row_of_offset(t_sc_code *x, int offset) {
	const int line = x->buf->line_of_offset(offset);
	return x->wraps.row_of_line_cell(line, x->buf->display_col_of_offset(offset));
}

/* Color of a token according to the attribute palette (S5b). IDE groupings
 * (§7.9.5); whatever is not listed falls back to the normal text color. */
static const t_jrgba *sc_code_token_color(t_sc_code *x, int type) {
	switch ((sc_code::TokenType)type) {
	case sc_code::Keyword:
		return &x->keywordcolor;
	case sc_code::Builtin:
		return &x->builtincolor;
	case sc_code::Primitive:
		return &x->primitivecolor;
	case sc_code::Class:
		return &x->classcolor;
	case sc_code::Symbol:
	case sc_code::SymbolMark:
	case sc_code::SymbolArg:
		return &x->symbolcolor;
	case sc_code::StringMark:
		return &x->stringcolor;
	case sc_code::Char:
		return &x->charcolor;
	case sc_code::RadixFloat:
	case sc_code::ScaleDegreeFloat:
	case sc_code::Float:
	case sc_code::HexInt:
		return &x->numbercolor;
	case sc_code::EnvVar:
		return &x->envvarcolor;
	case sc_code::SingleLineComment:
	case sc_code::MultiLineCommentStart:
	case sc_code::MultiLineCommentEnd:
		return &x->commentcolor;
	case sc_code::OpeningBracket:
	case sc_code::ClosingBracket:
		return &x->bracketcolor;
	case sc_code::Operator:
		return &x->operatorcolor;
	default:
		return &x->textcolor;
	}
}

/* Thumb rectangle in box coordinates (includes the top margin). Returns false if
 * there is no bar (the text fits whole). Leaves the caret inside the box. Called
 * after ANYTHING that moves the caret (keys, shortcuts, paste, undo) — paint cannot
 * do it because it only reads. */
static void sc_code_caret_visible(t_sc_code *x) {
	if (x->line_h <= 0. || x->view_h <= 0.)
		return;
	const sc_code::ScrollMetrics m = sc_code_metrics(x);
	/* S5e: the ROW of the caret is what gets guaranteed (with wrap a line can span
	 * several). */
	x->scroll.ensure_line(sc_code_row_of_offset(x, x->buf->caret()), m);
}

/* Closing a keypress: invalidate the highlight cache from the FIRST line the edit
 * could have touched (the caret may have moved back a line with ⌫) and leave the
 * caret visible. Cheap: `invalidate_from` is O(1) and the relexing is lazy. */
static void sc_code_after_key(t_sc_code *x, int line_before) {
	const int line_after = x->buf->line_of_offset(x->buf->caret());
	x->hl.invalidate_from(line_before < line_after ? line_before : line_after);
	sc_code_caret_visible(x);
}

static bool sc_code_thumb_rect(t_sc_code *x, const sc_code::ScrollMetrics &m, sc_code::ScrollView::Bar *bar,
							   t_rect *out) {
	*bar = x->scroll.bar(m);
	if (!bar->visible)
		return false;
	out->x = x->view_w - BAR_W - 1.;
	out->y = x->title_h + PAD_Y + bar->thumb_y;
	out->width = BAR_W;
	out->height = bar->thumb_h;
	return true;
}

/* Paints the background of an offset range [a, b) ROW BY ROW. This is what the
 * selection and the evaluation blink use: with wrap a range can cross several
 * visual rows, and each row trims the range to the cells that belong to it (the
 * text is split, the range is NOT). */
static void sc_code_paint_range(t_sc_code *x, t_jgraphics *g, const sc_code::ScrollMetrics &m,
								const t_jrgba &c, int a, int b, int first_row, int last_row) {
	if (b <= a || x->char_w <= 0.)
		return;
	const sc_code::Buffer &buf = *x->buf;
	const double text_x = sc_code_text_x(x);
	jgraphics_set_source_rgba(g, c.red, c.green, c.blue, c.alpha);
	for (int row = first_row; row <= last_row; ++row) {
		const int line = x->wraps.line_of_row(row);
		const sc_code::LineWrap &lw = x->wraps.wrap(line);
		const int ril = row - x->wraps.first_row(line);
		const int cs = lw.cell_start(ril);
		const int ra = buf.offset_of_display_col(line, cs);
		const int rb = buf.offset_of_display_col(line, lw.cell_end(ril));
		const int le = buf.line_end(line);
		const double y = x->title_h + x->scroll.line_screen_y(row, m);
		const int from = a > ra ? a : ra;
		const int to = b < rb ? b : rb;
		if (to > from) {
			const double x1 = text_x + (double)(buf.display_col_of_offset(from) - cs) * x->char_w;
			const double x2 = text_x + (double)(buf.display_col_of_offset(to) - cs) * x->char_w;
			if (x2 > x1) {
				jgraphics_rectangle(g, x1, y, x2 - x1, x->line_h);
				jgraphics_fill(g);
			}
		}
		/* The trailing '\n' is also in the range: a narrow sliver at the end of
		 * the LAST row of the line, same as before wrap. */
		if (lw.cell_end(ril) == lw.cells() && a <= le && b > le) {
			const double xn = text_x + (double)(lw.cells() - cs) * x->char_w;
			jgraphics_rectangle(g, xn, y, x->char_w * 0.6, x->line_h);
			jgraphics_fill(g);
		}
	}
}

/* --- S6c+: the completion session (⌃Space, SCODE.md §10.6) --------------- */

/* Receiver the caret is typing after: `SinOsc.` -> `SinOsc`, anything else "". Same
 * narrow rule as `sc_call`: only an identifier starting with an uppercase letter can be
 * resolved, so `~x.` and `s.` get the generic list and nothing is guessed. */
static std::string sc_code_compl_receiver(const std::string &text, int caret) {
	int s = caret;
	while (s > 0 && sc_code::Completer::is_word_byte((unsigned char)text[(size_t)s - 1]))
		--s;
	if (s == 0 || text[(size_t)s - 1] != '.')
		return std::string();
	const int e = s - 1;
	int b = e;
	while (b > 0 && sc_code::Completer::is_word_byte((unsigned char)text[(size_t)b - 1]))
		--b;
	if (b == e)
		return std::string();
	const char c0 = text[(size_t)b];
	if (!(c0 >= 'A' && c0 <= 'Z'))
		return std::string();
	return text.substr((size_t)b, (size_t)(e - b));
}

static void sc_code_compl_close(t_sc_code *x) {
	x->compl_open = 0;
	x->compl_cycle = 0;
	x->compl_word.clear();
	x->compl_alternatives = 0;
	x->compl_start = x->compl_end = x->buf->caret();
}

/* Asks the engine for the candidate at `cycle` and keeps the session in sync. Returns
 * false when there is nothing to offer, and then the session CLOSES itself (§10.6: "si
 * queda vacía se cierra sola"). The candidate source only changes with the receiver:
 * `set_external` rebuilds the pool, so it is not done on every keystroke.
 *
 * `min_prefix` = 3 on the automatic path (the bar only speaks once there are three
 * letters: `Sin` suggests, `Si` is noise), 0 for an explicit request. */
static bool sc_code_compl_refresh(t_sc_code *x, int cycle, int min_prefix) {
	if (!x->completer || !x->callaid)
		return false;
	sc_code_index_refresh(x);
	const std::string text = x->buf->text();
	const std::string receiver = sc_code_compl_receiver(text, x->buf->caret());
	if (receiver != x->compl_receiver) {
		x->compl_receiver = receiver;
		const bool have_index = x->index && x->index->entries() > 0;
		if (!receiver.empty() && have_index) {
			/* After a dot: ONLY the methods of the receiver (exclusive), and if the class
			 * is unknown the generic list instead of nothing. */
			std::vector<std::string> methods = x->index->method_names_of(receiver);
			if (!methods.empty())
				x->completer->set_external(methods, true);
			else
				x->completer->set_external(x->index->class_names(), false);
		} else if (have_index) {
			/* Anywhere else: classes (+ the words of the document and the language
			 * keywords, which the engine already has). */
			x->completer->set_external(x->index->class_names(), false);
		} else {
			x->completer->clear_external();
		}
	}
	x->completer->sync(text, x->buf->revision());

	sc_code::Completion c;
	if (!x->completer->suggest(text, x->buf->caret(), cycle, min_prefix, c)) {
		sc_code_compl_close(x);
		return false;
	}
	x->compl_open = 1;
	x->compl_cycle = c.index;
	x->compl_start = c.start;
	x->compl_end = c.end;
	x->compl_word = c.word;
	x->compl_alternatives = c.alternatives;
	return true;
}

/* Explicit request (⌃Space): open (or close) the session at the caret with no minimum
 * prefix, so right after `SinOsc.` the list is complete. Kept as the manual door even
 * though the automatic path does not need it (02-nov: on this keyboard neither ⌃Space
 * nor ⌥Space reaches `key`, see SCODE.md §10.6). */
static void sc_code_compl_toggle(t_sc_code *x) {
	if (x->compl_open) {
		sc_code_compl_close(x);
		return;
	}
	if (!x->callaid)
		return; /* no bar, no session: there is nowhere to show it */
	x->compl_cycle = 0;
	x->compl_receiver.clear(); /* fresh candidate source for this caret */
	sc_code_compl_refresh(x, 0, 0);
}

/* Threshold of the automatic suggestion: below it the bar stays quiet (`Si` is not a
 * suggestion). */
#define SC_COMPL_MIN_PREFIX 3

static void sc_code_compl_cycle_by(t_sc_code *x, int delta) {
	if (!x->compl_open)
		return;
	x->compl_cycle += delta;
	sc_code_compl_refresh(x, x->compl_cycle, SC_COMPL_MIN_PREFIX);
}

/* ⏎: the candidate replaces the token it was suggesting for. One undo step, like the
 * snippet expansion; readonly does not write. */
static void sc_code_compl_accept(t_sc_code *x) {
	if (!x->compl_open)
		return;
	if (x->readonly) {
		sc_code_compl_close(x);
		return;
	}
	const int line_before = x->buf->line_of_offset(x->buf->caret());
	const int len = x->buf->length();
	int a = x->compl_start, b = x->compl_end;
	if (a < 0)
		a = 0;
	if (b > len)
		b = len;
	if (b < a)
		b = a;
	const std::string word = x->compl_word;
	sc_code_compl_close(x); /* the request has no meaning once the text is replaced */
	x->buf->set_selection(a, b);
	if (!x->buf->insert_text(word))
		return;
	sc_code_after_key(x, line_before);
	if (x->autowrite && x->buf->length() != len)
		sc_code_autowrite_poke(x);
	jbox_redraw(&x->j_box);
}

/* S6c / §10.7 — the argument TEMPLATE of the IDE. Inside `SinOsc.ar(` Tab writes the name
 * of the current argument (`freq: `) and leaves the caret ready for the value; after the
 * comma, Tab writes the next one (`phase: `), and so on. It only fires at a CLEAN start
 * (nothing but spaces between the `(` or the last comma and the caret) and when the index
 * knows the signature, so Tab in the middle of a value still indents. */
static bool sc_code_arg_label(t_sc_code *x) {
	if (!x->index || x->buf->has_selection())
		return false;
	sc_code_index_refresh(x);
	if (x->index->entries() <= 0)
		return false;
	const std::string text = x->buf->text();
	const int caret = x->buf->caret();
	const sc_code::CallContext cc = sc_code::call_context_at(text, caret);
	if (!cc.valid || cc.open_paren < 0 || cc.arg < 0 || cc.arg_start < 0)
		return false;
	if (!sc_code::call_arg_clean_start(text, cc, caret))
		return false; /* in the middle of a value: this Tab is an indentation */
	const std::string method = cc.method.empty() ? std::string("new") : cc.method;
	const sc_code::MethodSig *sig =
		cc.method.empty() ? x->index->find_direct(cc.owner, method) : x->index->find(cc.owner, method);
	if (!sig || cc.arg >= (int)sig->args.size())
		return false;
	/* A space after the comma when the user went straight from `,` to Tab (`440,` ⇒
	 * `440, phase: `); if a space is already there it is not doubled. */
	const bool after_comma = caret > 0 && text[(size_t)caret - 1] == ',';
	return x->buf->insert_text((after_comma ? " " : "") + sig->args[(size_t)cc.arg] + ": ");
}

/* S6c: height of the RESERVED bar. 0 with `@callaid 0` (the text gets the whole box
 * back) or before the first paint measured the font. It comes from the font
 * (`bar_height` = rounded line height + 4), so it scales with `@fontsize`. */
static double sc_code_bar_height(t_sc_code *x) {
	if (!x->callaid || x->line_h <= 0.)
		return 0.;
	return (double)sc_code::bar_height(x->line_h);
}

/* What the bar shows: the signature of the call that contains the caret, or the one of
 * the name the caret is ON (clicking `Duty.kr` in the document shows the same as typing
 * it), or the constructor of a bare class call (`Ndef(` ⇒ `Ndef.new(key, object)`).
 * "No bar" (blank, it is never hidden) when there is no context, when the index does not
 * have the answer, or when `@callaid 0`. The receiver shown is the one the user TYPED and
 * not the class that defines the method: `SinOsc.play` has to read `SinOsc.play(…)` even
 * though the index found it two levels up in `Object` (sc_index walks the chain). */
static bool sc_code_bar_model(t_sc_code *x, sc_code::BarModel *out) {
	*out = sc_code::BarModel();
	if (!x->index || x->index->entries() <= 0)
		return false;
	const sc_code::CallContext cc = sc_code::call_context_at(x->buf->text(), x->buf->caret());
	if (!cc.valid)
		return false;
	/* Bare constructor: `find_direct` on purpose. If the class does not define its own
	 * `new`, the honest answer is "nothing" and not the inherited `Object.new(maxSize)`. */
	const std::string method = cc.method.empty() ? std::string("new") : cc.method;
	const sc_code::MethodSig *sig =
		cc.method.empty() ? x->index->find_direct(cc.owner, method) : x->index->find(cc.owner, method);
	if (!sig)
		return false;
	*out = sc_code::bar_model(cc.owner, method, sig->args, cc.arg);
	return true;
}

/* The bar itself: one reserved line at the bottom of the box, painted OVER everything
 * (`text` is already clipped above it). Layout, truncation and the priority of the
 * current argument over the head of the signature live in `sc_bar` (headless,
 * tests/bar_test.cpp); here it is only measured and drawn. */
static void sc_code_paint_bar(t_sc_code *x, t_jgraphics *g, const t_rect &rect) {
	if (x->bar_h <= 0.)
		return;
	const double top = rect.height - x->bar_h;
	jgraphics_rectangle(g, 0., top, rect.width, x->bar_h);
	jgraphics_set_source_rgba(g, x->barcolor.red, x->barcolor.green, x->barcolor.blue, x->barcolor.alpha);
	jgraphics_fill(g);
	jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.12);
	jgraphics_rectangle(g, 0., top, rect.width, 1.);
	jgraphics_fill(g);

	/* §10.6 — two modes, ONE bar. Precedence: an open session shows candidates; closing
	 * it (Esc, ⏎, a click, or the filter running out) brings the signature back. */
	const double bar_right = rect.width - (BAR_W + 2.);
	const double text_x = sc_code_text_x(x);
	int cells = 0;
	if (x->char_w > 0. && bar_right > text_x)
		cells = (int)std::floor((bar_right - text_x) / x->char_w);
	/* Vertically centered in the strip (the strip is round(line_h) + 4). */
	const double y = top + (x->bar_h - x->line_h) * 0.5 + x->ascent;

	if (x->compl_open && !x->compl_word.empty()) {
		const std::string counter =
			std::to_string(x->compl_cycle + 1) + "/" + std::to_string(x->compl_alternatives);
		const int ccells = sc_code::cells_in(counter);
		jgraphics_set_source_rgba(g, x->barhighlight.red, x->barhighlight.green, x->barhighlight.blue,
								  x->barhighlight.alpha);
		jgraphics_move_to(g, text_x, y);
		jgraphics_show_text(g, x->compl_word.c_str());
		if (cells > ccells + 1) { /* too narrow: the candidate wins, the counter goes */
			jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.70);
			jgraphics_move_to(g, text_x + (double)(cells - ccells) * x->char_w, y);
			jgraphics_show_text(g, counter.c_str());
		}
		return;
	}

	sc_code::BarModel model;
	if (!sc_code_bar_model(x, &model))
		return; /* context left / unknown method: the strip stays blank (§10.2) */

	const sc_code::BarCells c = sc_code::bar_layout(model, cells);
	if (c.text.empty())
		return;

	const t_jrgba &tc = x->textcolor;
	const t_jrgba &hc = x->barhighlight;
	std::string pre, arg, post;
	if (c.hl_start >= 0 && c.hl_end > c.hl_start) {
		pre = c.text.substr(0, (size_t)c.hl_start);
		arg = c.text.substr((size_t)c.hl_start, (size_t)(c.hl_end - c.hl_start));
		post = c.text.substr((size_t)c.hl_end);
	} else {
		pre = c.text;
	}
	/* Three runs, each one placed by CELLS (`…` is 3 bytes and 1 cell, so the byte count
	 * would drift). */
	double pen = text_x;
	jgraphics_set_source_rgba(g, tc.red, tc.green, tc.blue, 0.55);
	jgraphics_move_to(g, pen, y);
	jgraphics_show_text(g, pre.c_str());
	pen += (double)sc_code::cells_in(pre) * x->char_w;
	if (!arg.empty()) {
		jgraphics_set_source_rgba(g, hc.red, hc.green, hc.blue, hc.alpha);
		jgraphics_move_to(g, pen, y);
		jgraphics_show_text(g, arg.c_str());
		pen += (double)sc_code::cells_in(arg) * x->char_w;
	}
	if (!post.empty()) {
		jgraphics_set_source_rgba(g, tc.red, tc.green, tc.blue, 0.55);
		jgraphics_move_to(g, pen, y);
		jgraphics_show_text(g, post.c_str());
	}
	if (!c.counter.empty() && c.counter_col >= 0) {
		jgraphics_set_source_rgba(g, tc.red, tc.green, tc.blue, c.arg_visible ? 0.70 : 0.40);
		jgraphics_move_to(g, text_x + (double)c.counter_col * x->char_w, y);
		jgraphics_show_text(g, c.counter.c_str());
	}
}

static void sc_code_paint(t_sc_code *x, t_object *view) {
	t_jgraphics *g = (t_jgraphics *)patcherview_get_jgraphics(view);
	t_rect rect;
	jbox_get_rect_for_view((t_object *)x, view, &rect);

	x->view_w = rect.width;
	x->view_h = rect.height;

	/* fondo */
	jgraphics_rectangle(g, 0., 0., rect.width, rect.height);
	jgraphics_set_source_rgba(g, x->bgcolor.red, x->bgcolor.green, x->bgcolor.blue, x->bgcolor.alpha);
	jgraphics_fill(g);

	jgraphics_select_font_face(g, "Menlo", JGRAPHICS_FONT_SLANT_NORMAL, JGRAPHICS_FONT_WEIGHT_NORMAL);
	jgraphics_set_font_size(g, x->font_size); /* always: the context belongs to the patcher, not to us */
	sc_code_update_metrics(x, g);
	x->gutter_w = sc_code_gutter_width(x); /* it can change when lines are added */

	/* S5e — WRAP: it is redone with the width it is painting with RIGHT NOW. This is all
	 * that is needed for the text to reflow when the box is resized (Max calls paint with
	 * the new rect; the width changes ⇒ cells per row ⇒ rows). */
	const int cols = sc_code_cols_for_width(x, rect.width);
	const bool width_changed = x->text_cols > 0 && cols != x->text_cols;
	x->text_cols = cols;
	x->wraps.ensure(*x->buf, cols);

	/* title bar (S5c): fixed strip on top, one line high. It does NOT scroll and is
	 * NOT a text area: `sc_code_metrics` subtracts its height from the window. */
	x->title_h = (x->titlebar && x->line_h > 0.) ? x->line_h : 0.;
	/* S6c: the bottom bar is RESERVED, never intermittent (showing and hiding it would
	 * reflow the wrap and make the caret jump). `sc_code_metrics` subtracts it. */
	sc_code_index_refresh(x);
	x->bar_h = sc_code_bar_height(x);
	if (x->title_h > 0.) {
		double r = x->bgcolor.red + 0.07, gg = x->bgcolor.green + 0.07, b = x->bgcolor.blue + 0.07;
		if (r > 1.)
			r = 1.;
		if (gg > 1.)
			gg = 1.;
		if (b > 1.)
			b = 1.;
		jgraphics_rectangle(g, 0., 0., rect.width, x->title_h);
		jgraphics_set_source_rgba(g, r, gg, b, 1.);
		jgraphics_fill(g);
		/* right-aligned: measured with the context font (Menlo, already selected
		 * above) and the same breathing room as on the left is kept. If the box is
		 * narrower than the text, the margin wins (it does not stick to edge 0). */
		double tw = 0., th = 0.;
		jgraphics_text_measure(g, SC_TITLE_TEXT, &tw, &th);
		double tx = rect.width - PAD_X - 2. - tw;
		if (tx < PAD_X + 2.)
			tx = PAD_X + 2.;
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.55);
		jgraphics_move_to(g, tx, x->ascent + 1.);
		jgraphics_show_text(g, SC_TITLE_TEXT);
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.12);
		jgraphics_rectangle(g, 0., x->title_h - 1., rect.width, 1.);
		jgraphics_fill(g);
	}

	/* Scroll: the offset lives in `x->scroll` and here it is only SUBTRACTED. It is
	 * clamped on every paint because editing can shrink the document (or a resize
	 * can change the rows) and leave the offset past the end. */
	sc_code::ScrollMetrics m = sc_code_metrics(x);
	x->scroll.clamp(m);

	/* When the WIDTH changes the rows reflow: if the caret ended up outside the
	 * window (e.g. narrowing the box), it is brought back. It is NOT done on
	 * every paint: that would fight with scrolling the wheel away from the caret. */
	if (width_changed)
		x->scroll.ensure_line(sc_code_row_of_offset(x, x->buf->caret()), m);

	sc_code::ScrollView::Bar bar;
	t_rect thumb;
	const bool has_bar = sc_code_thumb_rect(x, m, &bar, &thumb);

	/* The text is clipped from the gutter up to just before the bar: a long line
	 * cannot cover the line numbers or the bar. The width is the SAME one the wrap
	 * uses (`sc_code_cols_for_width`), so what is seen is exactly what was split. */
	const double text_x = sc_code_text_x(x);
	double clip_w = rect.width - text_x - (BAR_W + 2.);
	if (clip_w < 1.)
		clip_w = 1.;
	double clip_h = rect.height - x->title_h - x->bar_h;
	if (clip_h < 1.)
		clip_h = 1.;
	jgraphics_save(g);
	jgraphics_clip(g, text_x, x->title_h, clip_w, clip_h);

	const sc_code::Buffer &buf = *x->buf;
	const int first = x->scroll.first_line(m); /* S5e: visual rows, not lines */
	const int last = x->scroll.last_line(m);

	/* selection, BELOW the text (S2). The columns are VISUAL (code points): an accent
	 * on the line no longer shifts the selection or the caret (handoff bug #2). */
	sc_code_paint_range(x, g, m, x->selcolor, buf.selection_start(), buf.selection_end(), first, last);

	/* S5e — evaluation blink: the SAME range that was sent to sclang, with the alpha
	 * the fade marks (500 ms, sc_blink). It goes after the selection so that it shows
	 * even when the evaluated block is the selection. */
	if (x->blink_state.active()) {
		const double a = x->blink_state.alpha(sc_code_now_ms());
		/* The offsets are clamped to the current document: the range was computed at eval
		 * time and the text may have shrunk during these 500 ms (no garbage must be
		 * drawn). */
		const int b0 = x->blink_state.start_off();
		const int b1 = x->blink_state.end_off() < buf.length() ? x->blink_state.end_off() : buf.length();
		if (a > 0. && b1 > b0) {
			t_jrgba c = x->blinkcolor;
			c.alpha *= a;
			sc_code_paint_range(x, g, m, c, b0, b1, first, last);
		}
	}

	/* caret bracket pair (S5c): background behind BOTH brackets (not the range).
	 * The matcher is consulted only here: if the caret is not next to a bracket
	 * nothing is lexed (see sc_bracket.h). */
	if (x->matchbrackets) {
		const sc_code::BracketMatch bm = x->matcher.at(buf.text(), buf.caret(), buf.revision());
		if (bm.valid()) {
			const int offs[2] = {bm.first, bm.second};
			jgraphics_set_source_rgba(g, x->matchcolor.red, x->matchcolor.green, x->matchcolor.blue,
									  x->matchcolor.alpha);
			for (int k = 0; k < 2; ++k) {
				const int line = buf.line_of_offset(offs[k]);
				const int row = sc_code_row_of_offset(x, offs[k]);
				if (row < first || row > last)
					continue;
				const int cs = x->wraps.wrap(line).cell_start(x->wraps.row_in_line(row));
				const int col = buf.display_col_of_offset(offs[k]) - cs;
				jgraphics_rectangle(g, text_x + (double)col * x->char_w,
									x->title_h + x->scroll.line_screen_y(row, m), x->char_w, x->line_h);
				jgraphics_fill(g);
			}
		}
	}

	/* text: only the visible ROWS and, inside each row, per TOKEN (S4). The
	 * sc_highlight cache returns the tokens already split by line; with wrap the
	 * token is trimmed to the row it belongs to (a token can cross the cut) and the
	 * visual column advances (fixed width: the font is monospace). */
	static std::string run; /* reused buffer: no allocation per token */
	for (int row = first; row <= last; ++row) {
		const int line = x->wraps.line_of_row(row);
		const sc_code::LineWrap &lw = x->wraps.wrap(line);
		const int ril = row - x->wraps.first_row(line);
		const int cs = lw.cell_start(ril);
		const int ce = lw.cell_end(ril);
		if (ce <= cs)
			continue; /* empty row (empty line or just the '\n') */
		std::string text = buf.line_text(line);
		if (text.empty())
			continue;
		const double y = x->title_h + x->scroll.line_screen_y(row, m) + x->ascent;
		const std::vector<sc_code::Token> &toks = x->hl.ensure(buf, line);
		int cell = 0; /* (visual) cell inside the LINE */
		for (size_t k = 0; k < toks.size(); ++k) {
			const sc_code::Token &tk = toks[k];
			if (tk.col < 0 || tk.length <= 0)
				continue;
			if (tk.col + tk.length > (int)text.size())
				break; /* cache inconsistente: no dibujamos basura */
			const int n = sc_code_utf8_cells(text.c_str() + tk.col, tk.length);
			if (tk.type != sc_code::WhiteSpace && cell + n > cs && cell < ce) {
				/* the token is trimmed to [cs, ce) of this row */
				const int ca = cell > cs ? cell : cs;
				const int cb = (cell + n) < ce ? cell + n : ce;
				const int ba = sc_code_utf8_bytes(text.c_str() + tk.col, ca - cell);
				const int bb = sc_code_utf8_bytes(text.c_str() + tk.col, cb - cell);
				const t_jrgba *c = sc_code_token_color(x, tk.type);
				run.assign(text, (size_t)(tk.col + ba), (size_t)(bb - ba));
				jgraphics_set_source_rgba(g, c->red, c->green, c->blue, c->alpha);
				jgraphics_move_to(g, text_x + (double)(ca - cs) * x->char_w, y);
				jgraphics_show_text(g, run.c_str());
			}
			cell += n; /* spaces also take cells */
			if (cell >= ce)
				break;
		}
	}

	/* caret: (row, cell inside the row) → x = cell * character width (monospace).
	 * With wrap, the cell is measured from the START of its row. */
	const int caret = buf.caret();
	const int caret_line = buf.line_of_offset(caret);
	const int caret_cell = buf.display_col_of_offset(caret);
	const int caret_row = x->wraps.row_of_line_cell(caret_line, caret_cell);
	const int caret_cs = x->wraps.wrap(caret_line).cell_start(x->wraps.row_in_line(caret_row));
	const double caret_x = text_x + (double)(caret_cell - caret_cs) * x->char_w;
	const double caret_y = x->title_h + x->scroll.line_screen_y(caret_row, m);

	if (x->focused) {
		jgraphics_rectangle(g, caret_x, caret_y, 1.6, x->line_h);
		jgraphics_set_source_rgba(g, x->caretcolor.red, x->caretcolor.green, x->caretcolor.blue, x->caretcolor.alpha);
		jgraphics_fill(g);
	}

	/* empty text: placeholder with the shortcuts (shorter if it does not
	 * fit) */
	if (buf.length() == 0) {
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.45);
		jgraphics_move_to(g, text_x, x->title_h + PAD_Y + x->ascent);
		jgraphics_show_text(g, "sclang…  ⇧⏎ line · ⌘⏎ region · ⌘⇧⏎ all · ⌃. panic");
	}

	jgraphics_restore(g);

	/* gutter (S5b): outside the text clip, so long lines do not cover it. With wrap,
	 * the number goes on the FIRST row of the line (continuation rows stay without a
	 * number, as in any editor). The number of the caret line is brighter (it orients
	 * while scrolling). */
	if (x->gutter_w > 0.) {
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.10);
		jgraphics_rectangle(g, x->gutter_w - 1., x->title_h, 1., rect.height - x->title_h - x->bar_h);
		jgraphics_fill(g);

		char num[16];
		for (int row = first; row <= last; ++row) {
			const int line = x->wraps.line_of_row(row);
			if (row != x->wraps.first_row(line))
				continue; /* continuation row: no number */
			const int d = sc_code_digits(line + 1);
			snprintf(num, sizeof(num), "%d", line + 1);
			jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue,
									  line == caret_line ? 0.70 : 0.30);
			jgraphics_move_to(g, x->gutter_w - GUTTER_GAP * 0.5 - (double)d * x->char_w,
							  x->title_h + x->scroll.line_screen_y(row, m) + x->ascent);
			jgraphics_show_text(g, num);
		}
	}

	/* vertical bar: only if there is something to scroll (it takes no room in
	 * documents that fit whole). Draggable; highlighted while dragging. */
	if (has_bar) {
		const double track_x = rect.width - BAR_W - 1.;
		jgraphics_set_source_rgba(g, COL_BAR_R, COL_BAR_G, COL_BAR_B, 0.10);
		jgraphics_rectangle(g, track_x, x->title_h + PAD_Y, BAR_W, bar.track_h);
		jgraphics_fill(g);
		jgraphics_set_source_rgba(g, COL_BAR_R, COL_BAR_G, COL_BAR_B, x->sb_drag ? 0.55 : 0.30);
		jgraphics_rectangle(g, thumb.x, thumb.y, thumb.width, thumb.height);
		jgraphics_fill(g);
	}

	/* S6c: the reserved bar goes LAST, over the (already clipped) text. */
	sc_code_paint_bar(x, g, rect);
}

static void sc_code_getdrawparams(t_sc_code *x, t_object *patcherview, t_jboxdrawparams *params) {
	if (x->focused) {
		params->d_bordercolor.red = x->caretcolor.red;
		params->d_bordercolor.green = x->caretcolor.green;
		params->d_bordercolor.blue = x->caretcolor.blue;
		params->d_bordercolor.alpha = 1.0;
		params->d_borderthickness = 2.;
	}
}

/**********************************************************************/
/* keyboard                                                            */
/*
 * Everything goes through `key` (K0: `keyfilter` is never called on a jbox without
 * a textfield). It is decided by KEYCODE + MODS, never by textcharacter: `⇧.` gives
 * keycode 62 (not 46) and `⌫`/`⌦` share textcharacter 127. Returning 1 = consumed,
 * 0 = let Max do its thing (⌘E, ⌘S, ⌘W …).
 */

#define K_RETURN (-4)
#define K_TAB (-5)
#define K_ESC (-3) /* JKEY_ESC: Max does not grab it (§3.9) */
#define K_PERIOD 46 /* '.' (⇧. gives 62 ⇒ panic compares keycode, not textcharacter) */
#define K_DELETE (-6)
#define K_BACKSPACE (-7)
#define K_UPARROW (-9)
#define K_DOWNARROW (-10)
#define K_LEFTARROW (-11)
#define K_RIGHTARROW (-12)
#define K_PAGEUP (-13)
#define K_PAGEDOWN (-14)
#define K_HOME (-15)
#define K_END (-16)

static bool sc_is_printable(long tc) {
	/* 13 Return, 9 Tab, 8 ^H, 27 Esc, 127 DEL (⌫ and ⌦) are not text. */
	return tc > 0 && tc != 13 && tc != 9 && tc != 8 && tc != 27 && tc != 127;
}

/* --- ⌘ shortcuts (S2): they arrive with textcharacter = 0 and keycode = ASCII
 * (§3.9) - */

static void sc_code_copy(t_sc_code *x) {
	if (!x->buf->has_selection())
		return;
	sc_code::clipboard_write_text(x->buf->selected_text());
}

static void sc_code_cut(t_sc_code *x) {
	if (!x->buf->has_selection())
		return;
	sc_code::clipboard_write_text(x->buf->selected_text());
	x->buf->delete_selection();
}

static void sc_code_paste(t_sc_code *x) {
	const std::string text = sc_code::clipboard_read_text();
	if (text.empty())
		return;
	x->buf->insert_text(text); /* replaces the selection if there is one (a single undo step) */
}

/* Returns true if the combination was ours (⇒ consume the key). */
static bool sc_code_command_key(t_sc_code *x, long keycode, bool shift) {
	switch (keycode) {
	case 'a': /* 97 */
		x->buf->select_all();
		return true;
	case 'c': /* 99 */
		sc_code_copy(x);
		return true;
	case 'v': /* 118 */
		if (x->readonly)
			return true;
		sc_code_paste(x);
		return true;
	case 'x': /* 120 */
		if (x->readonly)
			return true;
		sc_code_cut(x);
		return true;
	case 'z': /* 122; ⇧⌘Z = 122 + shift */
		if (x->readonly)
			return true;
		if (shift)
			x->buf->redo();
		else
			x->buf->undo();
		return true;
	}
	return false;
}

static long sc_code_key(t_sc_code *x, t_object *patcherview, long keycode, long modifiers, long textcharacter) {
	const bool cmd = (modifiers & eCommandKey) != 0;
	const bool shift = (modifiers & eShiftKey) != 0;
	const bool alt = (modifiers & eAltKey) != 0;
	const bool ctrl = (modifiers & eControlKey) != 0;

	sc_code::Buffer &buf = *x->buf;

	/* The edit (if any) can touch lines from here on: the highlight cache is
	 * invalidated at the end, with the minimum between this line and the final
	 * caret line. */
	const int line_before = buf.line_of_offset(buf.caret());
	/* Only for autowrite: any edit changes the text length. */
	const int len_before = buf.length();

	/* ⌘ editing shortcuts (S2). They go before the switch because the keycode of
	 * a letter is its ASCII and without ⌘ it has to remain printable text. */
	if (cmd && sc_code_command_key(x, keycode, shift)) {
		sc_code_after_key(x, line_before);
		jbox_redraw(&x->j_box);
		return 1;
	}

	/* S6c+ — ⌃Space opens/closes the completion session (§10.6). macOS may own ⌃Space
	 * ("Select next source in input menu") when there is more than one input layout, so
	 * ⌥Space is accepted as the fallback decided in the plan. The space keycode is matched
	 * on both sides (32 and 49) on purpose: it is the one measurement of the keymap that
	 * was never taken on this keyboard. */
	const bool space_key = (keycode == 32 || keycode == 49 || textcharacter == 32);
	if (!cmd && (ctrl || alt) && space_key) {
		sc_code_compl_toggle(x);
		jbox_redraw(&x->j_box);
		return 1;
	}

	/* Any OTHER key ends the session before doing its own thing: Tab, ⏎, Esc and
	 * printable text belong to the session while it is open (handled in the switch). */
	if (x->compl_open && keycode != K_TAB && keycode != K_RETURN && keycode != K_ESC &&
		(cmd || ctrl || !sc_is_printable(textcharacter)))
		sc_code_compl_close(x);

	/* ⌃. = panic (`CmdPeriod.run`), through the outlet ⇒ there is no global Max
	 * shortcut. `⌘.` is still accepted (verified 30-oct: `⌘.` does NOT reach the
	 * object — Max stops it in two layers that cannot be reassigned: the Cocoa
	 * key-equivalent of the menu, which comes from `maxinterface.json` ⇒ entry
	 * `stopscheduler`, and a JUCE command hardcoded in the Max binary that survives
	 * that reassignment and has no key bindings UI. The macOS App Shortcuts do not
	 * override it either). If it ever arrives, it is already covered. */
	if (keycode == K_PERIOD && (cmd || ctrl)) {
		object_post((t_object *)x, "panic");
		outlet_anything(x->out_eval, gensym("panic"), 0, NULL);
		return 1;
	}

	switch (keycode) {
	/* --- evaluation (SCODE.md §1/§3.2) ------------------------------------ */
	case K_RETURN:
		/* S6c+ decision (02-nov): ⏎ is NEVER taken by the session — it keeps evaluating and
		 * inserting newlines. Only Tab writes the suggestion. */
		if (cmd && shift) {
			sc_code_eval_with_mode(x, sc_code::EvalModeDocument); /* agregado nuestro (§7.1) */
			return 1;
		}
		if (cmd) {
			sc_code_eval_with_mode(x, sc_code::EvalModeRegion);
			return 1;
		}
		if (shift) {
			sc_code_eval_with_mode(x, sc_code::EvalModeLine);
			return 1;
		}
		if (!ctrl && !alt) {
			if (x->readonly)
				return 1; /* readonly: does not edit, but does not eat the key from Max */
			/* Autoindent with the IDE engine (`sc_indent` port): the level comes from
			 * the nesting of the line up to the caret, with the IDE rules (a `(` in
			 * column 0 does not add a level, etc. — see sc_indent.h). */
			const int level = sc_code::indent_level_at_offset(buf.text(), buf.caret());
			buf.insert_newline(sc_code::indent_string(level, (int)x->tabwidth));
			break;
		}
		return 0; /* ⌥⏎ / ⌃⏎: let Max see it */

	/* --- borrado ---------------------------------------------------------- */
	case K_BACKSPACE:
		if (cmd)
			return 0; /* left for "delete line" in S6 */
		if (x->readonly)
			return 1;
		if (alt) {
			buf.move_word_left(true); /* ⌥⌫: borra la palabra anterior */
			buf.backspace();
		} else {
			buf.backspace();
		}
		break;

	case K_DELETE:
		if (cmd)
			return 0;
		if (x->readonly)
			return 1;
		if (alt) {
			buf.move_word_right(true); /* ⌥⌦: borra la palabra siguiente */
			buf.delete_forward();
		} else {
			buf.delete_forward();
		}
		break;

	/* --- navigation ------------------------------------------------------- */
	case K_LEFTARROW:
		if (cmd)
			buf.move_line_start(shift);
		else if (alt)
			buf.move_word_left(shift);
		else
			buf.move_left(shift);
		break;

	case K_RIGHTARROW:
		if (cmd)
			buf.move_line_end(shift);
		else if (alt)
			buf.move_word_right(shift);
		else
			buf.move_right(shift);
		break;

	case K_UPARROW:
		if (cmd || alt)
			buf.move_document_start(shift);
		else
			buf.move_up(shift);
		break;

	case K_DOWNARROW:
		if (cmd || alt)
			buf.move_document_end(shift);
		else
			buf.move_down(shift);
		break;

	case K_PAGEUP:
	case K_PAGEDOWN: {
		/* One page = visible ROWS minus one (the one-row overlap is what keeps you
		 * from losing your place while reading, like in any editor). It keeps the
		 * visual column INSIDE the row: with wrap a line spans several rows. */
		const sc_code::ScrollMetrics m = sc_code_metrics(x);
		int page = x->scroll.visible_lines(m) - 1;
		if (page < 1)
			page = 1;
		const int caret_line = buf.line_of_offset(buf.caret());
		const int caret_cell = buf.display_col_of_offset(buf.caret());
		const int row = x->wraps.row_of_line_cell(caret_line, caret_cell);
		const int col_in_row = caret_cell - x->wraps.wrap(caret_line).cell_start(x->wraps.row_in_line(row));
		int target = row + (keycode == K_PAGEUP ? -page : page);
		if (target < 0)
			target = 0;
		if (target > x->wraps.row_count() - 1)
			target = x->wraps.row_count() - 1;
		const int line = x->wraps.line_of_row(target);
		const sc_code::LineWrap &lw = x->wraps.wrap(line);
		const int ril = x->wraps.row_in_line(target);
		int cell = lw.cell_start(ril) + col_in_row;
		if (cell > lw.cell_end(ril))
			cell = lw.cell_end(ril);
		buf.move_caret(buf.offset_of_display_col(line, cell), shift);
		break;
	}

	case K_HOME:
		buf.move_line_start(shift);
		break;

	case K_END:
		buf.move_line_end(shift);
		break;

	/* --- Tab = spaces (§7.1 decision), or cycling inside a session (S6c+) --- */
	case K_TAB:
		/* S6c+: with a candidate in the bar, Tab INSERTS it (one undo step) and ⇧Tab
		 * cycles. Without a candidate, Tab is still indentation (spaces, §7.1). */
		if (x->compl_open) {
			if (shift)
				sc_code_compl_cycle_by(x, -1);
			else
				sc_code_compl_accept(x);
			jbox_redraw(&x->j_box);
			return 1;
		}
		if (x->readonly)
			return 1;
		if (shift) {
			buf.outdent((int)x->tabwidth);
			break;
		}
		/* §10.7: at a clean start of an argument of a known call, Tab writes `name: `
		 * (IDE behavior) instead of a hard tab. `break`, so the tail updates the highlight
		 * and the autowrite exactly like any other edit. */
		if (sc_code_arg_label(x))
			break;
		buf.insert_tab((int)x->tabwidth);
		break;

	/* --- Esc: closes the session, otherwise it is Max's ---------------------- */
	case K_ESC:
		if (x->compl_open) {
			sc_code_compl_close(x);
			jbox_redraw(&x->j_box);
			return 1;
		}
		return 0;

	default:
		/* Printable: only without ⌘/⌃ (those carry textcharacter 0 and are shortcuts,
		 * §3.9). */
		if (cmd || ctrl)
			return 0; /* ⌘A/C/V/X/Z y ⌘E/⌘S los maneja Max o S2 */
		if (x->readonly)
			return 1;
		if (!sc_is_printable(textcharacter))
			return 0; /* IME/dead key: do not consume (documented limit, §2) */
		buf.insert_char((unsigned int)textcharacter);
		/* The IDE reindents the line when a closer is typed (insertMatchingTokens L400:
		 * `if (isClosingToken) indent(cursor, ...)`): the line of the `)` `]` `}` goes
		 * back to the level of its opener. It is coalesced with the typing burst (same
		 * undo step). */
		if (textcharacter == ')' || textcharacter == ']' || textcharacter == '}') {
			const int line = buf.line_of_offset(buf.caret());
			/* Port of insertMatchingTokens L400 with the indent() exceptions
			 * (multi-line comment / start of comment ⇒ no reindent). */
			const sc_code::LineIndent li = sc_code::indent_for_line_at_offset(buf.text(), buf.caret());
			if (li.apply)
				buf.reindent_line(line, sc_code::indent_string(li.level, (int)x->tabwidth));
		}
		break;
	}

	/* S6c+: the suggestion is AUTOMATIC as soon as the token has 3 letters (`Sin` ⇒
	 * `SinOsc`) and it only DISPLAYS: nothing is written until Tab says so. */
	if (!x->readonly && !cmd && !ctrl && sc_is_printable(textcharacter))
		sc_code_compl_refresh(x, 0, SC_COMPL_MIN_PREFIX);

	/* The caret ALWAYS ends up visible: it is the rule that makes the editor
	 * usable with documents taller than the box (typing, arrows, ⌘↑/↓,
	 * PageUp/Down…). */
	sc_code_after_key(x, line_before);
	if (x->autowrite && buf.length() != len_before)
		sc_code_autowrite_poke(x);

	/* All of the keyboard repaints: the change can be text or just the caret. The
	 * cost is measured in S4 with 1000 lines (there only the visible lines are
	 * painted). */
	jbox_redraw(&x->j_box);
	return 1;
}

/* Declared for future compatibility (if the box ever gets a textfield): at K0 it
 * was NOT called ONCE. Always let it through. */
static long sc_code_keyfilter(t_sc_code *x, t_object *patcherview, long *keycode, long *modifiers,
							  long *textcharacter) {
	return 0;
}

/**********************************************************************/
/* foco / mouse                                                        */

static void sc_code_focusgained(t_sc_code *x, t_object *patcherview) {
	x->focused = 1;
	jbox_redraw(&x->j_box);
}

static void sc_code_focuslost(t_sc_code *x, t_object *patcherview) {
	x->focused = 0;
	jbox_redraw(&x->j_box);
}

/* S1: the click only took focus. S2: it places the caret, and with
 * drag/double/triple click it selects. `pt` comes in box coordinates (checked
 * against the SDK uisimp.c: it compares pt against 0..rect.width/height). The
 * metrics are those of the last `paint` (the click always arrives after a paint). */
static int sc_code_offset_at(t_sc_code *x, t_pt pt) {
	if (x->char_w <= 0. || x->line_h <= 0.)
		return x->buf->caret(); /* no metrics yet: we do not invent a position */

	/* S5e: box y → visual ROW (the wrap is up to date: `metrics` ensures it) →
	 * line + cell span of that row. */
	(void)sc_code_metrics(x);
	const int row = sc_code_row_of_y(x, pt.y);
	const int line = x->wraps.line_of_row(row);
	const sc_code::LineWrap &lw = x->wraps.wrap(line);
	const int ril = x->wraps.row_in_line(row);
	const int cs = lw.cell_start(ril);
	const int ce = lw.cell_end(ril);
	int col = cs + (int)std::floor((pt.x - sc_code_text_x(x)) / x->char_w + 0.5);
	if (col < cs)
		col = cs;
	if (col > ce)
		col = ce; /* right of a split row: the end of THAT row */
	return x->buf->offset_of_display_col(line, col);
}

/* Dragging the selection outside the box: it scrolls and the caret stays on the
 * first/last visible line. The speed grows with the distance to the edge (taking
 * the mouse far from the object scrolls fast; near the edge, fine). */
static void sc_code_autoscroll_drag(t_sc_code *x, t_pt *pt) {
	if (x->line_h <= 0. || x->view_h <= 0.)
		return;
	const double top = x->title_h + PAD_Y;
	const double bottom = x->view_h - x->bar_h - PAD_Y;
	if (pt->y >= top && pt->y <= bottom)
		return;
	const sc_code::ScrollMetrics m = sc_code_metrics(x);
	const double dy = pt->y < top ? pt->y - top : pt->y - bottom;
	x->scroll.scroll_by(dy, m);
	pt->y = pt->y < top ? top : bottom;
}

/**********************************************************************/
/* S5d: context menu (right click)                                      */

enum {
	SC_MENU_UNDO = 1,
	SC_MENU_REDO,
	SC_MENU_CUT,
	SC_MENU_COPY,
	SC_MENU_PASTE,
	SC_MENU_DELETE,
	SC_MENU_SELECTALL,
	SC_MENU_SAVE,
	SC_MENU_LOAD
};

/* Name proposed in the save panel: the basename of `@file` or "sc_code.scd". The
 * panel (NSSavePanel) is what guarantees the `.scd` extension. */
static std::string sc_code_default_name(t_sc_code *x) {
	if (sc_code_has_file(x)) {
		const char *slash = std::strrchr(x->filename->s_name, '/');
		const char *base = slash ? slash + 1 : x->filename->s_name;
		if (base && base[0])
			return std::string(base);
	}
	return std::string("sc_code.scd");
}

/* Sets `@file` WITHOUT re-reading the file (the normal setter would autoload and
 * overwrite the document). It is the same path as the attribute ⇒ it is saved with
 * the patch. */
static void sc_code_set_file_quiet(t_sc_code *x, const char *path) {
	const long autoload = x->autoload;
	x->autoload = 0;
	object_attr_setsym((t_object *)x, gensym("file"), gensym(path));
	x->autoload = autoload;
}

/* Save to .scd…: with `@file` it writes there; without `@file` it opens the panel
 * (NSSavePanel with `UTType` ⇒ the `.scd` extension is guaranteed) and leaves it as
 * `@file`, so the next save is direct and `@autowrite` respects it. */
static void sc_code_menu_save(t_sc_code *x) {
	if (sc_code_has_file(x)) {
		if (sc_code_write_file(x))
			object_post((t_object *)x, "saved: %s", x->filename->s_name);
		return;
	}
	const std::string path = sc_code::dialog_save_code(sc_code_default_name(x));
	if (path.empty())
		return; /* cancelado */
	if (sc_code_save_to_path(x, path.c_str())) {
		sc_code_set_file_quiet(x, path.c_str());
		object_post((t_object *)x, "saved: %s (kept as @file)", path.c_str());
	}
}

/* Load from .scd…: replaces the document (with undo reset) and leaves that path in
 * `@file`. */
static void sc_code_menu_load(t_sc_code *x) {
	const std::string path = sc_code::dialog_open_code();
	if (path.empty())
		return; /* cancelado */
	if (sc_code_load_from_path(x, path.c_str())) {
		sc_code_set_file_quiet(x, path.c_str());
		object_post((t_object *)x, "loaded: %s (kept as @file)", path.c_str());
	}
}

static void sc_code_menu_action(t_sc_code *x, int item) {
	const int line_before = x->buf->line_of_offset(x->buf->caret());
	sc_code::Buffer &buf = *x->buf;
	const bool ro = x->readonly != 0;
	switch (item) {
	case SC_MENU_UNDO:
		if (!ro)
			buf.undo();
		break;
	case SC_MENU_REDO:
		if (!ro)
			buf.redo();
		break;
	case SC_MENU_CUT:
		if (!ro)
			sc_code_cut(x);
		break;
	case SC_MENU_COPY:
		sc_code_copy(x);
		break;
	case SC_MENU_PASTE:
		if (!ro)
			sc_code_paste(x);
		break;
	case SC_MENU_DELETE:
		if (!ro) {
			if (buf.has_selection())
				buf.delete_selection();
			else if (buf.caret() < buf.length())
				buf.delete_forward();
		}
		break;
	case SC_MENU_SELECTALL:
		buf.select_all();
		break;
	case SC_MENU_SAVE:
		sc_code_menu_save(x);
		break;
	case SC_MENU_LOAD:
		sc_code_menu_load(x);
		break;
	default:
		return;
	}
	sc_code_after_key(x, line_before); /* invalida highlight / autowrite / redraw */
	jbox_redraw(&x->j_box);
}

/* Right click: the menu is built on the fly (no need to cache it: a right click is
 * rare) with the items disabled according to the state. The menu is modal
 * (`jpopupmenu_popup` blocks until something is chosen or it is cancelled). */
static void sc_code_popup_menu(t_sc_code *x, t_object *patcherview, t_pt pt) {
	t_jpopupmenu *menu = jpopupmenu_create();
	if (!menu)
		return;
	jpopupmenu_clear(menu);
	const bool ro = x->readonly != 0;
	const bool sel = x->buf->has_selection();

	jpopupmenu_additem(menu, SC_MENU_UNDO, "Undo", NULL, 0, ro || !x->buf->can_undo(), NULL);
	jpopupmenu_additem(menu, SC_MENU_REDO, "Redo", NULL, 0, ro || !x->buf->can_redo(), NULL);
	jpopupmenu_addseparator(menu);
	jpopupmenu_additem(menu, SC_MENU_CUT, "Cut", NULL, 0, ro || !sel, NULL);
	jpopupmenu_additem(menu, SC_MENU_COPY, "Copy", NULL, 0, !sel, NULL);
	jpopupmenu_additem(menu, SC_MENU_PASTE, "Paste", NULL, 0, ro, NULL);
	jpopupmenu_additem(menu, SC_MENU_DELETE, "Delete", NULL, 0, ro, NULL);
	jpopupmenu_additem(menu, SC_MENU_SELECTALL, "Select All", NULL, 0, x->buf->length() == 0, NULL);
	jpopupmenu_addseparator(menu);
	jpopupmenu_additem(menu, SC_MENU_SAVE, sc_code_has_file(x) ? "Save to .scd" : "Save to .scd…", NULL, 0, 0,
					   NULL);
	jpopupmenu_additem(menu, SC_MENU_LOAD, "Load from .scd…", NULL, 0, 0, NULL);

	/* `pt` is in BOX-local coordinates (the same space as the hit tests above: 0,0
	 * is the top-left corner of the box), while `patcherview_canvas_to_screen`
	 * expects CANVAS coordinates. Passing `pt` as is put the menu at the canvas
	 * position of the click plus/minus the patcher scroll (0.9.4: the user saw it
	 * "far outside the box" depending on where the box sat in the patcher). The
	 * canvas point is the box rect origin plus `pt`, and two canvas points (the
	 * origin and the bottom-right corner) give the canvas→screen scale so a ZOOMED
	 * patcher also lands the menu under the cursor. */
	t_rect rect;
	long ox = 0, oy = 0, fx = 0, fy = 0;
	if (jbox_get_rect_for_view((t_object *)x, patcherview, &rect) != MAX_ERR_NONE)
		rect.x = rect.y = 0.;
	patcherview_canvas_to_screen(patcherview, rect.x, rect.y, &ox, &oy);
	patcherview_canvas_to_screen(patcherview, rect.x + rect.width, rect.y + rect.height, &fx, &fy);
	const double zoom_x = rect.width > 0. ? (double)(fx - ox) / rect.width : 1.;
	const double zoom_y = rect.height > 0. ? (double)(fy - oy) / rect.height : zoom_x;
	t_pt where;
	where.x = (double)ox + pt.x * zoom_x;
	where.y = (double)oy + pt.y * zoom_y;
	const int item = jpopupmenu_popup(menu, where, 0);
	jpopupmenu_destroy(menu);
	if (item > 0)
		sc_code_menu_action(x, item);
}

static void sc_code_mouseenter(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	/* The cursor is set from a mouse event (that is when there is a view), same
	 * as the SDK `uitester`. */
	jmouse_setcursor(patcherview, (t_object *)x, JMOUSE_CURSOR_IBEAM);
}

static long sc_code_mousedown(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	sc_code::Buffer &buf = *x->buf;

	/* S6c+: a click ends the completion session (§10.6: it is a mode the user opened
	 * explicitly and it does not survive the caret moving). */
	if (x->compl_open)
		sc_code_compl_close(x);

	/* Right click (or ⌃click, which Max delivers the same way): context menu. The
	 * event is consumed so that Max does not show its own. */
	if (modifiers & eRightButton) {
		sc_code_popup_menu(x, patcherview, pt);
		return 1;
	}

	/* S6c: the bottom bar is not text. A click there moves NOTHING (the caret stays where
	 * it is, which is the point of reserving it) and the drag that may follow is ignored
	 * with the same flag. */
	if (x->bar_h > 0. && pt.y >= x->view_h - x->bar_h) {
		x->bar_grab = 1;
		return 0;
	}

	/* Click on the title bar: it does not move the caret (it is a strip, not
	 * text). */
	if (pt.y < x->title_h)
		return 0;

	/* Click on the bar: grabs the thumb (or jumps to the position) and does NOT
	 * touch the caret or the selection — like in any editor. */
	{
		const sc_code::ScrollMetrics m = sc_code_metrics(x);
		sc_code::ScrollView::Bar bar;
		t_rect thumb;
		if (sc_code_thumb_rect(x, m, &bar, &thumb) && pt.x >= x->view_w - BAR_W - 2.) {
			const double thumb_top = x->title_h + PAD_Y + bar.thumb_y;
			if (pt.y >= thumb_top && pt.y <= thumb_top + bar.thumb_h) {
				x->sb_grab = pt.y - thumb_top; /* no jump: it grabs where you touched */
			} else {
				x->sb_grab = bar.thumb_h * 0.5;
				x->scroll.drag_thumb_to(pt.y - x->title_h - PAD_Y - x->sb_grab, m);
			}
			x->sb_drag = 1;
			x->dragging = 0;
			jbox_redraw(&x->j_box);
			return 0;
		}
	}

	const double now = sc_code_now_ms();
	const bool near = std::fabs(pt.x - (double)x->last_click_x) <= 4. &&
					  std::fabs(pt.y - (double)x->last_click_y) <= 4.;

	/* Cycle 1 → 2 → 3 → 1 within 400 ms and ~4 px (double/triple click). */
	if (x->click_count > 0 && near && (now - x->last_click_ms) < 400.)
		x->click_count = (x->click_count % 3) + 1;
	else
		x->click_count = 1;
	x->last_click_ms = now;
	x->last_click_x = (long)pt.x;
	x->last_click_y = (long)pt.y;

	const int offset = sc_code_offset_at(x, pt);
	if (modifiers & eShiftKey) {
		buf.move_caret(offset, true); /* ⇧click: extends from the current caret */
	} else if (x->click_count == 2) {
		int a = 0, b = 0;
		buf.word_range_at(offset, a, b);
		buf.set_selection(a, b);
	} else if (x->click_count == 3) {
		int a = 0, b = 0;
		buf.line_range_at(offset, a, b);
		buf.set_selection(a, b);
	} else {
		buf.set_caret(offset);
	}

	x->dragging = 1;
	jbox_redraw(&x->j_box);
	return 0; /* a click in the text has no Max default to consume */
}

static void sc_code_mousedrag(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (x->bar_grab)
		return; /* the drag was born on the bar (S6c): it does not select */
	if (x->sb_drag) {
		const sc_code::ScrollMetrics m = sc_code_metrics(x);
		x->scroll.drag_thumb_to(pt.y - x->title_h - PAD_Y - x->sb_grab, m);
		jbox_redraw(&x->j_box);
		return;
	}
	x->dragging = 1;
	sc_code_autoscroll_drag(x, &pt); /* selection longer than the box */
	x->buf->move_caret(sc_code_offset_at(x, pt), true); /* the anchor stays where the click was born */
	jbox_redraw(&x->j_box);
}

static void sc_code_mouseup(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	x->dragging = 0;
	x->sb_drag = 0;
	x->bar_grab = 0;
	jbox_redraw(&x->j_box); /* clears the thumb highlight */
}

/* Wheel / trackpad. In Max, delta_y > 0 = wheel up ⇒ the content goes down ⇒ the
 * offset decreases. It scrolls in PIXELS (not lines) so the trackpad feels
 * continuous; the clamp is done by ScrollView. */
static void sc_code_mousewheel(t_sc_code *x, t_object *patcherview, t_pt pt, long modifiers, double delta_x,
							   double delta_y) {
	if (x->line_h <= 0. || x->view_h <= 0.)
		return;
	const sc_code::ScrollMetrics m = sc_code_metrics(x);
	if (!x->scroll.scrollable(m))
		return;
	/* Pixels = delta (Max units) × line height × `@scrollspeed`: the trackpad delta
	 * comes in small units (0.9.1). */
	x->scroll.scroll_by(-delta_y * x->line_h * x->scrollspeed, m);
	jbox_redraw(&x->j_box);
}
