// SPDX-License-Identifier: GPL-3.0-only
/* index_test.cpp — S6c gate: class library index (parse + query + generator).
 *
 * sclang writes the index with one `eval` (32347 entries / ~950 KB / 73 ms measured
 * on 02-nov) while the editor keeps running, so what has to be bulletproof here is
 * the tolerant parse: half-written file, junk lines, `Meta_` of the class-side
 * methods, the same method defined on both sides, and the inherited ones.
 *
 * Usage: ./build/index_test [-v] [--generator <path>] [<index-file>]
 *   -v                verbose (one line per check)
 *   --generator PATH  print the sclang code that writes the index at PATH and exit
 *   <index-file> also load a real dump and query it (see the design notes)
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "../sc_index.h"

#include <cstdio>
#include <cstring>
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

/* Substring test for the generator code (nobody wants a 900-char diff in the log). */
static void check_has(const char *what, const std::string &haystack, const char *needle) {
	++gChecks;
	if (haystack.find(needle) == std::string::npos) {
		++gFails;
		printf("  FAIL %-58s missing \"%s\"\n", what, needle);
	} else if (gVerbose) {
		printf("  ok   %-58s has \"%s\"\n", what, needle);
	}
}

static std::string join(const std::vector<std::string> &v) {
	std::string s;
	for (size_t i = 0; i < v.size(); ++i) {
		if (i)
			s += " ";
		s += v[i];
	}
	return s;
}

/* An expected signature: empty `wantArgs` = the method is not in the index. */
static void expect_sig(const ClassIndex &idx, const char *what, const char *owner, const char *method,
					   const std::string &wantArgs) {
	const MethodSig *sig = idx.find(owner, method);
	++gChecks;
	if (!sig) {
		if (wantArgs.empty()) {
			if (gVerbose)
				printf("  ok   %-58s (absent)\n", what);
			return;
		}
		++gFails;
		printf("  FAIL %-58s not found, want \"%s\"\n", what, wantArgs.c_str());
		return;
	}
	if (wantArgs.empty()) {
		++gFails;
		printf("  FAIL %-58s exists (args \"%s\"), want none\n", what, join(sig->args).c_str());
		return;
	}
	const std::string got = join(sig->args);
	if (got != wantArgs) {
		++gFails;
		printf("  FAIL %-58s args \"%s\", want \"%s\"\n", what, got.c_str(), wantArgs.c_str());
		return;
	}
	if (gVerbose)
		printf("  ok   %-58s \"%s\"\n", what, got.c_str());
}

static void expect_no_args(const ClassIndex &idx, const char *what, const char *owner, const char *method) {
	const MethodSig *sig = idx.find(owner, method);
	++gChecks;
	if (!sig) {
		++gFails;
		printf("  FAIL %-58s not found (expected no args)\n", what);
	} else if (!sig->args.empty()) {
		++gFails;
		printf("  FAIL %-58s args \"%s\", want none\n", what, join(sig->args).c_str());
	} else if (gVerbose) {
		printf("  ok   %-58s (no args)\n", what);
	}
}

/* Optional: a real dump written by sclang (`--generator` above). */
static void check_real_file(const char *path) {
	printf("real dump: %s\n", path);
	FILE *f = fopen(path, "rb");
	if (!f) {
		++gChecks;
		++gFails;
		printf("  FAIL cannot open the dump\n");
		return;
	}
	std::string text;
	char buf[65536];
	size_t got;
	while ((got = fread(buf, 1, sizeof buf, f)) > 0)
		text.append(buf, got);
	fclose(f);

	ClassIndex idx;
	check_bool("real dump loads", idx.load(text), true);
	check_bool("real dump has > 10000 entries", idx.entries() > 10000, true);
	check_bool("real dump has superclass records", idx.super_entries() > 1000, true);
	check_str("real dump: SinOsc.ar", join(idx.find("SinOsc", "ar") ? idx.find("SinOsc", "ar")->args
																   : std::vector<std::string>()),
			  "freq phase mul add");
	/* Measured: SinOsc defines NO instance method (its own methods are all class
	 * side) and `madd` lives in UGen, two hops up: SinOsc -> PureUGen -> UGen. */
	check_bool("real dump: SinOsc.madd comes from a parent",
			   idx.find("SinOsc", "madd") != nullptr && idx.find_direct("SinOsc", "madd") == nullptr, true);
	check_str("real dump: SinOsc.madd args", join(idx.find("SinOsc", "madd") ? idx.find("SinOsc", "madd")->args
																			: std::vector<std::string>()),
			  "mul add");
	check_str("real dump: the chain", idx.superclass_of("SinOsc") + " > " + idx.superclass_of("PureUGen") + " > " +
										 idx.superclass_of("UGen"),
			  "PureUGen > UGen > AbstractFunction");
	check_bool("real dump: name list is big", (int)idx.symbol_names().size() > 3000, true);
	check_bool("real dump: says which sclang wrote it", !idx.version().empty() && idx.version()[0] == '3', true);
}

int main(int argc, char **argv) {
	const char *realFile = nullptr;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-v") == 0) {
			gVerbose = true;
		} else if (strcmp(argv[i], "--generator") == 0 && i + 1 < argc) {
			fputs(index_generator_code(argv[++i]).c_str(), stdout);
			return 0;
		} else {
			realFile = argv[i];
		}
	}

	/* --- a normal dump ------------------------------------------------------- */
	{
		printf("full dump\n");
		ClassIndex idx;
		const std::string dump = "@version 3.14.1\n"
								 "@super SinOsc UGen\n"
								 "@super UGen Object\n"
								 "Meta_SinOsc.ar\tfreq phase mul add\n"
								 "Meta_SinOsc.kr\tfreq phase mul add\n"
								 "SinOsc.stop\n"
								 "Env.new\tlevels times curves releaseNode loopNode offset\n"
								 "Object.play\ttarget outbus fadeTime addAction\n"
								 "#end\n";
		check_bool("load accepts a dump with the marker", idx.load(dump), true);
		check_int("entries counted", idx.entries(), 5);
		check_str("the sclang version is kept", idx.version(), "3.14.1");
		check_int("  and the header is not an entry", idx.line_count(), 5);
		check_int("superclass records counted apart", idx.super_entries(), 2);
		check_int("valid lines counted", idx.line_count(), 5);
		expect_sig(idx, "Meta_SinOsc.ar is read as SinOsc.ar", "SinOsc", "ar", "freq phase mul add");
		expect_sig(idx, "kr", "SinOsc", "kr", "freq phase mul add");
		expect_sig(idx, "instance method with args", "Env", "new",
				   "levels times curves releaseNode loopNode offset");
		expect_no_args(idx, "no TAB = no arguments", "SinOsc", "stop");
		expect_sig(idx, "method that does not exist", "SinOsc", "nope", "");
		expect_sig(idx, "does not invent the owner", "Ndef", "ar", "");
		check_str("version without the record", ClassIndex().version(), "");
		check_str("superclass_of", idx.superclass_of("SinOsc"), "UGen");
		check_str("superclass_of unknown", idx.superclass_of("Ndef"), "");
	}

	/* --- inheritance chain --------------------------------------------------- */
	{
		printf("inheritance\n");
		ClassIndex idx;
		idx.load("Meta_SinOsc.ar\tfreq\n"
				 "UGen.madd\tmul add\n"
				 "@super SinOsc UGen\n"
				 "@super UGen Object\n"
				 "@super Object nil\n"
				 "#end\n");
		/* Two hops up: SinOsc -> UGen -> Object. */
		expect_sig(idx, "find walks the chain", "SinOsc", "madd", "mul add");
		check_str("  and reports the real owner", idx.find("SinOsc", "madd")->owner, "UGen");
		check_bool("find_direct does not walk", idx.find_direct("SinOsc", "madd") == nullptr, true);
		check_bool("find_direct finds its own", idx.find_direct("SinOsc", "ar") != nullptr, true);
		check_str("`@super Object nil` is not stored", idx.superclass_of("Object"), "");
		check_int("`nil` does not count as a superclass", idx.super_entries(), 2);

		/* A cyclic dump must not hang the editor (depth cap). */
		ClassIndex cyc;
		cyc.load("A.x\targ1\n"
				 "@super A B\n"
				 "@super B A\n"
				 "@super C C\n"
				 "#end\n");
		expect_sig(cyc, "cycle A->B->A still resolves", "B", "x", "arg1");
		expect_sig(cyc, "self loop C->C resolves nothing", "C", "x", "");
	}

	/* --- tolerance: half-written file / junk lines --------------------------- */
	{
		printf("tolerance\n");
		ClassIndex idx;
		/* Without the final marker the dump is being written, so it is not used. */
		check_bool("no #end, not loaded", idx.load("Meta_SinOsc.ar\tfreq phase mul add\n"), false);
		check_int("  and the index stays empty", idx.entries(), 0);

		check_bool("empty, not loaded", idx.load(""), false);

		const std::string dirty = "#comment\n"
								  "\n"
								  "junk without a dot\n"
								  "Meta_SinOsc.ar\tfreq   phase  mul add \n"
								  ".noOwner\targ\n"
								  "NoMethod.\targ\n"
								  "Meta_SinOsc.ar\tfreq phase\n"
								  "#end\n";
		check_bool("a dump with junk still loads", idx.load(dirty), true);
		expect_sig(idx, "extra spaces do not break the args", "SinOsc", "ar", "freq phase");
		check_int("invalid lines are not counted", idx.line_count(), 2);
		/* The LAST occurrence wins (class-side vs instance-side with the same name). */
		expect_sig(idx, "duplicate: the last line wins", "SinOsc", "ar", "freq phase");

		ClassIndex crlf;
		check_bool("CRLF tolerated", crlf.load("Meta_SinOsc.ar\tfreq phase\r\n@super SinOsc UGen\r\n#end\r\n"), true);
		expect_sig(crlf, "  and the args come out clean", "SinOsc", "ar", "freq phase");
		check_str("  and CRLF superclass records work", crlf.superclass_of("SinOsc"), "UGen");
	}

	/* --- symbol_names: what feeds the bar's completion session ------- */
	{
		printf("symbol_names\n");
		ClassIndex idx;
		idx.load("Meta_SinOsc.ar\tfreq\n"
				 "Meta_SinOsc.kr\tfreq\n"
				 "SinOsc.play\t\n"
				 "Env.new\tlevels\n"
				 "@super SinOsc UGen\n"
				 "#end\n");
		const std::vector<std::string> names = idx.symbol_names();
		/* Classes: Env, SinOsc, UGen (from the superclass record). Methods: ar, kr,
		 * new, play. */
		check_int("classes + unique methods", (int)names.size(), 7);
		check_str("sorted (1)", names.empty() ? std::string() : names[0], "Env");
		check_str("sorted (2)", names.size() > 1 ? names[1] : std::string(), "SinOsc");
		check_str("sorted (3)", names.size() > 2 ? names[2] : std::string(), "UGen");
		check_str("no duplicates (Meta_)", names.size() > 3 ? names[3] : std::string(), "ar");
		check_str("superclass names are classes too", names.size() > 6 ? names[6] : std::string(), "play");

		idx.clear();
		check_int("clear empties the entries", idx.entries(), 0);
		check_int("clear empties the supers", idx.super_entries(), 0);
		check_int("clear empties the names", (int)idx.symbol_names().size(), 0);
	}

	/* --- classes and methods per class (S6c+: the completion session) -------- */
	{
		printf("class and method lists\n");
		ClassIndex idx;
		idx.load("Meta_SinOsc.ar\tfreq phase\n"
				 "SinOsc.play\n"
				 "UGen.madd\tmul add\n"
				 "Object.play\n"
				 "Meta_UGen.ar\tfreq\n"
				 "Env.new\tlevels\n"
				 "@super SinOsc UGen\n"
				 "@super Meta_SinOsc Meta_UGen\n"
				 "@super UGen Object\n"
				 "@super Meta_UGen Meta_Object\n"
				 "#end\n");
		const std::vector<std::string> cls = idx.class_names();
		check_int("classes only (no Meta_)", (int)cls.size(), 2);
		check_str("classes sorted (1)", cls.empty() ? std::string() : cls[0], "SinOsc");
		check_str("classes sorted (2)", cls.size() > 1 ? cls[1] : std::string(), "UGen");

		const std::vector<std::string> m = idx.method_names_of("SinOsc");
		check_int("methods of SinOsc + chain, unique", (int)m.size(), 3);
		check_str("methods sorted (1)", m.empty() ? std::string() : m[0], "ar");
		check_str("methods sorted (2)", m.size() > 1 ? m[1] : std::string(), "madd");
		check_str("methods sorted (3)", m.size() > 2 ? m[2] : std::string(), "play");
		check_int("an unknown class has no methods", (int)idx.method_names_of("Nope").size(), 0);
		check_int("an empty owner has no methods", (int)idx.method_names_of("").size(), 0);
	}

	/* --- reload -------------------------------------------------------------- */
	{
		printf("reload\n");
		ClassIndex idx;
		idx.load("SinOsc.ar\tfreq\n#end\n");
		check_int("first load", idx.entries(), 1);
		idx.load("#end\n"); /* empty dump but with the marker */
		check_int("an empty dump clears the index", idx.entries(), 0);
		check_bool("load returns false with no entries", idx.load("SinOsc.ar\tfreq\n"), false);
	}

	/* --- the sclang generator --------------------------------------- */
	{
		printf("generator\n");
		const std::string code = index_generator_code("/tmp/sc_index.txt");
		check_has("writes the file", code, "File(path, \"w\")");
		check_has("path is interpolated", code, "/tmp/sc_index.txt");
		check_has("writes the end marker", code, "\"#end\\n\"");
		check_has("writes the sclang version", code, "\"@version \"");
		check_has("  with Main.version", code, "Main.version");
		check_has("writes superclass records", code, "\"@super \"");
		check_has("strips the implicit this", code, "copyToEnd(1)");
		check_has("iterates the class library", code, "Class.allClasses.do");
		check_has("reports what it did", code, "sc_code index: ");
		check_bool("is one expression (eval takes one)", code.find("(\n") != std::string::npos, true);

		/* A path with quotes or backslashes must not break the SC string. */
		const std::string weird = index_generator_code("/tmp/a\"b\\c.txt");
		check_has("quotes escaped", weird, "/tmp/a\\\"b\\\\c.txt");
	}

	if (realFile)
		check_real_file(realFile);

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
