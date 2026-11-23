// SPDX-License-Identifier: GPL-3.0-only
/* call_test.cpp — S6c gate: which `Class.method(…` call contains the caret.
 *
 * The argument aid is shown in the bottom bar, so a wrong context means the user sees
 * the arguments of ANOTHER method (worse than seeing nothing). The cases that break
 * this are always the same: parentheses and commas inside strings, symbols,
 * character literals and comments; and the commas of nested `[…]`, `(…)` and `{…}`,
 * which do NOT separate arguments.
 *
 * Usage: ./build/call_test [-v]
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "../sc_call.h"

#include <cstdio>
#include <string>

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

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got \"%s\", want \"%s\"\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-58s \"%s\"\n", what, got.c_str());
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

/* Case with an expected context. Empty `owner` = there is no context. The caret is
 * explicit so the middle of a call can be inspected. */
static void expect(const char *what, const std::string &text, int caret, const char *owner, const char *method,
				   int arg = -1) {
	const CallContext c = call_context_at(text, caret);
	const std::string want_owner(owner ? owner : ""); /* nullptr = "no context" */
	const std::string want_method(method ? method : "");
	++gChecks;
	if (want_owner.empty()) {
		if (c.valid) {
			++gFails;
			printf("  FAIL %-58s context %s.%s, want none\n", what, c.owner.c_str(), c.method.c_str());
		} else if (gVerbose) {
			printf("  ok   %-58s (no context)\n", what);
		}
		return;
	}
	if (!c.valid) {
		++gFails;
		printf("  FAIL %-58s no context, want %s.%s\n", what, want_owner.c_str(), want_method.c_str());
		return;
	}
	if (c.owner != want_owner || c.method != want_method) {
		++gFails;
		printf("  FAIL %-58s got %s.%s, want %s.%s\n", what, c.owner.c_str(), c.method.c_str(), want_owner.c_str(),
			   want_method.c_str());
		return;
	}
	if (arg >= 0 && c.arg != arg) {
		++gFails;
		printf("  FAIL %-58s arg %d, want %d\n", what, c.arg, arg);
		return;
	}
	if (gVerbose)
		printf("  ok   %-58s %s.%s arg %d\n", what, c.owner.c_str(), c.method.c_str(), c.arg);
}

int main(int argc, char **argv) {
	gVerbose = (argc > 1 && argv[1][0] == '-');

	/* --- the basics ---------------------------------------------------------- */
	{
		printf("basic context\n");
		const std::string t = "SinOsc.ar(";
		expect("caret right after the (", t, (int)t.size(), "SinOsc", "ar", 0);
		expect("1st comma", "SinOsc.ar(440, ", 15, "SinOsc", "ar", 1);
		expect("2nd comma", "SinOsc.ar(440, 0, ", 18, "SinOsc", "ar", 2);
		expect("caret in the middle of an argument", "SinOsc.ar(440, 0.5)", 13, "SinOsc", "ar", 0);
		expect("closed call", "SinOsc.ar(440, 0, 0.1) ", 23, nullptr, nullptr);
		expect("after the close", "SinOsc.ar(440)", 14, nullptr, nullptr);
		expect("caret at 0", "SinOsc.ar(", 0, nullptr, nullptr);
		expect("new works too", "SinOsc.new(440, ", 15, "SinOsc", "new", 1);
		expect("spaces around the dot", "SinOsc . ar (", 13, "SinOsc", "ar", 0);
		expect("class with a digit", "SinOsc2.ar(", 11, "SinOsc2", "ar", 0);
		check_int("open_paren offset", call_context_at("x = SinOsc.ar(1, ", 18).open_paren, 13);
		check_str("owner is what was written", call_context_at("SinOsc.ar(", 10).owner, "SinOsc");
		check_int("caret past the end is clamped", call_context_at("SinOsc.ar(", 999).arg, 0);
		check_int("negative caret", call_context_at("SinOsc.ar(", -5).valid ? 1 : 0, 0);
	}

	/* --- clicking a name that is already written --------------------- */
	{
		printf("caret on a name\n");
		expect("caret at the end of the name", "SinOsc.ar", 9, "SinOsc", "ar");
		expect("  past the end is clamped", "SinOsc.ar", 99, "SinOsc", "ar");
		expect("caret in the middle of the method", "SinOsc.ar", 8, "SinOsc", "ar");
		expect("caret on the owner", "SinOsc.ar", 3, "SinOsc", "ar");
		expect("  at the first byte of the owner", "SinOsc.ar", 0 + 1, "SinOsc", "ar");
		{
			const CallContext c = call_context_at("Duty.kr((1..4)/4, 0)", 5);
			check_int("name, not a call: no open parenthesis", c.open_paren, -1);
			check_int("  and no current argument", c.arg, -1);
		}
		/* The same narrow rule: only an uppercase receiver resolves. */
		expect("lowercase receiver, no call", "~tune.kr", 8, nullptr, nullptr);
		expect("  env var receiver", "\\tune.kr", 8, nullptr, nullptr);
		expect("a lone word", "SinOsc", 6, nullptr, nullptr);
		expect("a dotted chain is not resolved", "~x.y.z", 5, nullptr, nullptr);
		/* Not a name: a string, a symbol or a comment. */
		expect("inside a string", "\"SinOsc.ar\"", 5, nullptr, nullptr);
		expect("inside a block comment", "/* SinOsc.ar */", 7, nullptr, nullptr);
		expect("inside a line comment", "// SinOsc.ar\nSinOsc.ar", 10, nullptr, nullptr);
		/* The realistic document: click on `Duty.kr` where it already sits. */
		{
			const std::string doc = "Duty.kr((1..4)/4, 0, Dwhite(0,1))\n";
			const int c1 = (int)doc.find("Duty.kr") + 5;
			expect("Duty.kr inside its own call's argument", doc, c1, "Duty", "kr");
		}
	}

	/* --- a bare class constructor -------------------------------------------- */
	{
		printf("bare constructor\n");
		expect("Ndef(", "Ndef(", 5, "Ndef", "");
		expect("  with arguments", "Ndef(\\x, ", 9, "Ndef", "");
		{
			const CallContext c = call_context_at("Ndef(\\x, ", 9);
			check_int("  the constructor keeps the argument index", c.arg, 1);
			check_int("  and the open parenthesis", c.open_paren, 4);
		}
		expect("lowercase is a plain call", "sin(", 4, nullptr, nullptr);
		expect("a variable is not a class", "~x(", 3, nullptr, nullptr);
	}

	/* --- the argument template --------------------------------------- */
	{
		printf("clean argument start\n");
		{
			const std::string t = "SinOsc.ar(";
			const CallContext c = call_context_at(t, (int)t.size());
			check_int("arg_start right after the (", c.arg_start, (int)t.size());
			check_bool("clean right after the (", call_arg_clean_start(t, c, (int)t.size()), true);
		}
		{
			const std::string t = "SinOsc.ar(440, ";
			const CallContext c = call_context_at(t, (int)t.size());
			check_int("arg_start after the last comma", c.arg_start, 14);
			check_bool("clean after a comma + space", call_arg_clean_start(t, c, (int)t.size()), true);
		}
		{
			const std::string t = "SinOsc.ar(44";
			const CallContext c = call_context_at(t, (int)t.size());
			check_bool("in the middle of a value it is not clean", call_arg_clean_start(t, c, (int)t.size()),
					   false);
		}
		{
			/* A comma of a nested array does not move `arg_start`. */
			const std::string t = "SinOsc.ar([1, 2], ";
			const CallContext c = call_context_at(t, (int)t.size());
			check_int("nested commas do not move arg_start", c.arg_start, 17);
			check_bool("  and the start is clean", call_arg_clean_start(t, c, (int)t.size()), true);
		}
		{
			const CallContext c = call_context_at("Duty.kr", 5);
			check_int("a name with no call has no arg_start", c.arg_start, -1);
		}
	}

	/* --- what is NOT resolved (narrow rule) --------------------------- */
	{
		printf("not resolved\n");
		expect("variable receiver", "~x.do(1, ", 9, nullptr, nullptr);
		expect("lowercase receiver", "s.boot(", 7, nullptr, nullptr);
		expect("bare function", "sin(440)", 6, nullptr, nullptr);
		expect("method with no receiver", "ar(440)", 6, nullptr, nullptr);
		/* The comment eats its own call: the paren that follows has no receiver. */
		expect("a comment is not code", "// SinOsc.ar(\n(", 15, nullptr, nullptr);
		{
			const std::string t2 = "// SinOsc.ar(1, 2)\nSinOsc.ar(1, ";
			expect("  ...and does not break the next call", t2, (int)t2.size(), "SinOsc", "ar", 1);
		}
		/* Measured in the old branch: an env-var receiver is NOT a class, so the bar
		 * has nothing to look up. It keeps the "uppercase identifier" rule honest. */
		expect("environment variable receiver", "\\tune.kr(60)", 12, nullptr, nullptr);
		expect("  same with ~", "~tune.kr(60)", 11, nullptr, nullptr);
	}

	/* --- nesting ------------------------------------------------------------- */
	{
		printf("nesting\n");
		const std::string t = "SinOsc.ar(Line.kr(1, 2), ";
		expect("the innermost parenthesis wins", "SinOsc.ar(Line.kr(1, ", 20, "Line", "kr", 1);
		expect("outside the nested one the outer returns", t, (int)t.size(), "SinOsc", "ar", 1);
		expect("array commas do not count", "SinOsc.ar([1, 2, 3], ", 20, "SinOsc", "ar", 1);
		expect("grouping parenthesis commas do not count", "SinOsc.ar((1, 2), ", 19, "SinOsc", "ar", 1);
		expect("block commas do not count", "SinOsc.ar({|a, b| a}, ", 22, "SinOsc", "ar", 1);
		expect("array closed before the comma", "SinOsc.ar([1, 2], 3, ", 21, "SinOsc", "ar", 2);
		expect("stray close is ignored", "SinOsc.ar(1), ", 13, nullptr, nullptr);
	}

	/* --- literals and comments ---------------------------------------------- */
	{
		printf("literals and comments\n");
		expect("comma inside a string", "SinOsc.ar(\"a,b\", ", 17, "SinOsc", "ar", 1);
		expect("parenthesis inside a string", "SinOsc.ar(\"(\", ", 15, "SinOsc", "ar", 1);
		expect("escaped quote in the string", "SinOsc.ar(\"a\\\"b\", ", 19, "SinOsc", "ar", 1);
		expect("comma inside a symbol", "SinOsc.ar('a,b', ", 17, "SinOsc", "ar", 1);
		expect("character literal", "SinOsc.ar($(, ", 15, "SinOsc", "ar", 1);
		expect("line comment with a parenthesis", "SinOsc.ar( // (\n1, ", 21, "SinOsc", "ar", 1);
		expect("block comment with a comma", "SinOsc.ar(1 /* , */, ", 20, "SinOsc", "ar", 1);
		{
			const std::string t2 = "SinOsc.ar(1 /* /* , */ , */, ";
			expect("nested comment", t2, (int)t2.size(), "SinOsc", "ar", 1);
			/* Inside the comment nothing counts: no comma reached the call yet. */
			expect("  inside the comment there is no comma yet", t2, 21, "SinOsc", "ar", 0);
		}
		expect("block comment opened before the call", "/* ( */ SinOsc.ar(1, ", 21, "SinOsc", "ar", 1);
		expect("unclosed string swallows the rest", "SinOsc.ar(\"abc, ", 16, "SinOsc", "ar", 0);
	}

	/* --- a realistic document ------------------------------------------------ */
	{
		printf("realistic document\n");
		const std::string doc = "(\n"
								"Ndef(\\ftonic, {\n"
								"\tGVerb.ar(\n"
								"\t\tSinOsc.ar(\n"
								"\t\t\tSelect.kr(\n"
								"\t\t\t\tHasher.kr(\n"
								"\t\t\t\t\tDuty.kr((1..4)/4, 0, Dwhite(0,1))\n"
								"\t\t\t\t)*5, midicps([0,3,5,7,10]+\\tune.kr(60))\n"
								"\t\t\t)\n"
								"\t).sum, 200, 3)\n"
								"} * -10.dbamp\n"
								")\n";
		const int caret_tune = (int)doc.find("\\tune.kr(60)") + 9;
		expect("an env var is not a class", doc, caret_tune, nullptr, nullptr);
		const int caret_hasher = (int)doc.find("Hasher.kr(") + 10;
		expect("Hasher.kr", doc, caret_hasher, "Hasher", "kr", 0);
		const int caret_gverb = (int)doc.find("GVerb.ar(") + 9;
		expect("GVerb.ar", doc, caret_gverb, "GVerb", "ar", 0);
		const int caret_midicps = (int)doc.find("midicps(") + 8;
		expect("a call with no receiver has no context", doc, caret_midicps, nullptr, nullptr);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
