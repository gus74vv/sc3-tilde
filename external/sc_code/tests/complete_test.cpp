// SPDX-License-Identifier: GPL-3.0-only
/* complete_test.cpp — S6c+ gate: candidate engine of the completion session.
 *
 * The session lives in ONE line of the bar, so what has to be exact is which token is
 * replaced, which candidate comes first and what ⏎ would insert. Pinned here:
 *   - the token is the `[A-Za-z0-9_]` run at the caret, sigil (`~`/`\`) included;
 *   - the word already written is never a candidate;
 *   - ranking is deterministic: frequency desc, shorter, alphabetical;
 *   - cycling wraps in both directions and does not mutate the engine;
 *   - `min_prefix` 0 = the user asked (⌃Space): an EMPTY prefix lists everything;
 *   - external names (the class library index) enter the pool with frequency 0, and in
 *     exclusive mode they are the ONLY pool (the `Class.` case).
 *
 * Usage: ./build/complete_test [-v]
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "../sc_complete.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_int(const char *what, int got, int want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %d, want %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-58s %d\n", what, got);
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %s, want %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-58s %s\n", what, got ? "true" : "false");
	}
}

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got \"%s\", want \"%s\"\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-58s \"%s\"\n", what, got.c_str());
	}
}

/* What ⏎ would leave in the document: [start, end) replaced by `word`. */
static std::string applied(const std::string &text, const Completion &c) {
	if (!c.valid || c.start < 0 || c.end < c.start || c.end > (int)text.size())
		return std::string("<invalid>");
	return text.substr(0, (size_t)c.start) + c.word + text.substr((size_t)c.end);
}

int main(int argc, char **argv) {
	gVerbose = (argc > 1 && argv[1][0] == '-');

	/* --- the token that gets replaced ---------------------------------------- */
	{
		printf("token\n");
		Completer c;
		c.set_external({"SinOsc", "sinosc2"});
		c.sync("SinOsc", 1);
		Completion r;
		check_bool("a full token with no other alternative", c.suggest("SinOsc", 6, 0, 1, r), false);
		check_bool("  (the word itself is never a candidate)", c.suggest("S", 1, 0, 1, r), true);
		check_str("  the pool answers for the prefix", r.word, "SinOsc");

		check_bool("one byte is not enough for the automatic path", c.suggest("S", 1, 0, 2, r), false);
		check_bool("  with min_prefix 1 it is", c.suggest("S", 1, 0, 1, r), true);		check_int("  start of the token", r.start, 0);
		check_int("  end is the caret", r.end, 1);
		check_str("  what the buffer would read", applied("S", r), "SinOsc");

		const std::string s = "~out = SinO";
		Completion e;
		check_bool("text before the token does not matter", c.suggest(s, (int)s.size(), 0, 1, e), true);
		check_int("  token start", e.start, 7);
		check_str("  result", applied(s, e), "~out = SinOsc");

		c.sync("~freq = 440; ~fr", 2);
		Completion sig;
		const std::string env = "~freq = 440; ~fr";
		check_bool("an env var keeps its sigil", c.suggest(env, (int)env.size(), 0, 1, sig), true);
		check_str("  candidate", sig.word, "~freq");
		check_int("  the sigil is part of the replaced token", sig.start, 13);
		check_str("  result", applied(env, sig), "~freq = 440; ~freq");

		const std::string symdoc = "\\tune = 1; \\tu";
		c.sync(symdoc, 3);
		Completion sym;
		check_bool("symbols are their own pool", c.suggest(symdoc, (int)symdoc.size(), 0, 1, sym), true);
		check_str("  symbol candidate", sym.word, "\\tune");

		Completion empty;
		check_bool("no token and no explicit request", c.suggest("x = ", 4, 0, 1, empty), false);

		/* The automatic path of the object uses 3 (02-nov: `Sin` suggests, `Si` does not). */
		check_bool("2 letters are below the automatic threshold", c.suggest("Si", 2, 0, 3, r), false);
		check_bool("3 letters are at the threshold", c.suggest("Sin", 3, 0, 3, r), true);
		check_str("  and it suggests the class", r.word, "SinOsc");
	}

	/* --- ranking -------------------------------------------------------------- */
	{
		printf("ranking\n");
		Completer c;
		const std::string doc = "foo foobar foo fo qux fooz";
		c.sync(doc, 1);
		Completion r;
		check_bool("frequency first", c.suggest(doc, 2, 0, 1, r), true);
		check_str("  the most frequent wins", r.word, "foo");
		check_int("  how many alternatives", r.alternatives, 3);

		Completer t;
		const std::string d2 = "abcd abc ab";
		t.sync(d2, 1);
		Completion s1, s2;
		/* Same frequency (1 each): shorter first -> ab, abc, abcd. */
		check_bool("shorter first", t.suggest(d2, 1, 0, 1, s1), true);
		check_str("  first alternative", s1.word, "ab");
		check_int("  alternatives (the language keyword `arg` is one too)", s1.alternatives, 4);
		t.suggest(d2, 1, 1, 1, s2);
		check_str("  second alternative", s2.word, "abc");
		check_int("  cycling does not rebuild the pool", t.rebuilds(), 1);
	}

	/* --- cycling -------------------------------------------------------------- */
	{
		printf("cycling\n");
		Completer c;
		const std::string doc = "Du Duo Duct";
		c.sync(doc, 1);
		Completion a, b, w, neg;
		check_bool("cycle 0", c.suggest(doc, 2, 0, 1, a), true);
		check_str("  shortest first", a.word, "Duo");
		check_bool("cycle 1", c.suggest(doc, 2, 1, 1, b), true);
		check_str("  the other one", b.word, "Duct");
		check_bool("a cycle past the end wraps", c.suggest(doc, 2, a.alternatives, 1, w), true);
		check_str("  and lands on the first", w.word, a.word);
		check_bool("negative cycles wrap backwards", c.suggest(doc, 2, -1, 1, neg), true);
		check_int("  last alternative", neg.index, neg.alternatives - 1);
		check_int("  nothing is recalled between calls", (int)a.alternatives, 2);
	}

	/* --- external names (the index) ------------------------------------------- */
	{
		printf("external\n");
		Completer c;
		c.set_external({"SinOsc", "Saw", "Select"});
		c.sync("SinOsc.kr(1)", 1);
		Completion r;
		/* Nothing in the document starts with "Sa": the answer comes from outside. */
		check_bool("an external name is suggested", c.suggest("Sa", 2, 0, 1, r), true);
		check_str("  candidate", r.word, "Saw");
		c.set_external({});
		c.sync("SinOsc.kr(1)", 2);
		Completion gone;
		check_bool("clearing drops them", c.suggest("Sa", 2, 0, 1, gone), false);
	}

	/* --- exclusive mode: after a dot ---------------------------------- */
	{
		printf("exclusive\n");
		Completer c;
		c.set_external({"SinOsc", "Saw", "Select"}, true);
		const std::string doc = "SinOsc.";
		c.sync(doc, 1);
		Completion r;
		/* Empty prefix is valid (the user asked with ⌃Space) and the pool is ONLY the
		 * external list: the document word `SinOsc` is not offered back. */
		check_bool("right after the dot the prefix is empty", c.suggest(doc, (int)doc.size(), 0, 0, r), true);
		check_int("  nothing is replaced (insert at the caret)", r.start, r.end);
		check_str("  deterministic first alternative", r.word, "Saw");
		check_int("  the three methods", r.alternatives, 3);
		check_bool("with a normal min_prefix an empty prefix is nothing",
				   c.suggest(doc, (int)doc.size(), 0, 1, r), false);

		/* Typing filters: `SinO` after the dot leaves only what starts like that. */
		const std::string typed = "SinOsc.SinO";
		c.sync(typed, 2);
		check_bool("filtering keeps the session alive", c.suggest(typed, (int)typed.size(), 0, 1, r), true);
		check_str("  filtered candidate", r.word, "SinOsc");
		const std::string dead = "SinOsc.zzz";
		check_bool("nothing left: no suggestion", c.suggest(dead, (int)dead.size(), 0, 1, r), false);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
