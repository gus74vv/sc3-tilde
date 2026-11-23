// SPDX-License-Identifier: GPL-3.0-only
/* snip_test.cpp — gate of the `sc_code` snippets file (headless, S6).
 *
 * Pins down the format (`### key` + verbatim body) and the `snip.KEY` matching
 * BEFORE touching the Max object: the expensive bug here is substituting too much
 * (a modulo `%`, a `snip.` showing up inside code) or too little (a key with
 * digits), and through the GUI that looks like \"the editor did something weird\".
 *
 *     GPL v3 or later (ver sc_snip.cpp).
 *
 * Uso: ./build/snip_test [-v]
 */

#include "../sc_buffer.h"
#include "../sc_snip.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_int(const char *what, int got, int want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %d, esperado %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-58s %d\n", what, got);
	}
}

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s\n        got  [%s]\n        want [%s]\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-58s [%s]\n", what, got.c_str());
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %s, esperado %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-58s %s\n", what, got ? "true" : "false");
	}
}

static std::string body_of(const Snippets &d, const std::string &key) {
	const std::string *b = d.find(key);
	return b ? *b : std::string("<nil>");
}

/* ------------------------------------------------------------------------- */
/* cabeceras                                                                  */

static void test_header() {
	std::string key;
	check_bool("simple header", is_header("### kick", key), true);
	check_str("simple header: key", key, "kick");

	check_bool("header with digits", is_header("### 20", key), true);
	check_str("header with digits: key", key, "20");

	check_bool("header with underscore", is_header("### my_snip", key), true);
	check_str("header with underscore: key", key, "my_snip");

	check_bool("header with trailing spaces", is_header("### kick   ", key), true);
	check_str("header with trailing spaces: key", key, "kick");

	check_bool("### with no key", is_header("###", key), false);
	check_bool("### with no space", is_header("###kick", key), false);
	check_bool("#### is not a header", is_header("#### kick", key), false);
	check_bool("indented header is not a header", is_header("  ### kick", key), false);
	check_bool("key with hyphen is not a header", is_header("### my-key", key), false);
	check_bool("key with space is not a header", is_header("### my snip", key), false);
	check_bool("empty line is not a header", is_header("", key), false);
	check_bool("code is not a header", is_header("Ndef(\\x).play;", key), false);
}

/* ------------------------------------------------------------------------- */
/* parseo                                                                     */

static void test_parse_base() {
	/* Two snippets, a comment before the first header, blank edges. */
	const std::string src = "// mis snippets\n"
							"# just some random comment\n"
							"\n"
							"### kick\n"
							"Ndef(\\kick, {|t|\n"
							"    SinOsc.ar(60) * 0.5\n"
							"}).play;\n"
							"\n"
							"\n"
							"### pad\n"
							"\n"
							"Ndef(\\pad).play;\n"
							"\n";
	const Snippets d = parse_snippets(src);
	check_int("parse: 2 snippets", (int)d.size(), 2);
	check_int("parse: no duplicates", (int)d.duplicates.size(), 0);
	check_int("parse: no empty entries", (int)d.empty.size(), 0);

	/* Verbatim body: inner indentation, newlines, without the final blank line. */
	check_str("parse: multi-line body is verbatim", body_of(d, "kick"),
			  "Ndef(\\kick, {|t|\n    SinOsc.ar(60) * 0.5\n}).play;");
	check_str("parse: single-line body", body_of(d, "pad"), "Ndef(\\pad).play;");
	check_str("parse: key inexistente", body_of(d, "nope"), "<nil>");
}

static void test_parse_comments_and_headers() {
	/* A malformed `###` does NOT open a block: the line stays in the body of the
	 * previous one. */
	const std::string src = "### a\n"
							"line 1\n"
							"###not-a-header\n"
							"#### neither\n"
							"### b\n"
							"ok\n";
	const Snippets d = parse_snippets(src);
	check_int("headers: 2 snippets", (int)d.size(), 2);
	check_str("headers: `###x` stays in the body", body_of(d, "a"),
			  "line 1\n###not-a-header\n#### neither");
	check_str("headers: the next block starts clean", body_of(d, "b"), "ok");
}

static void test_parse_duplicates_and_empty() {
	const std::string src = "### kick\nfirst\n"
							"### kick\nsecond\n"
							"### empty\n\n\n"
							"### empty2\n   \n\t\n";
	const Snippets d = parse_snippets(src);
	check_int("dups: 1 snippet", (int)d.size(), 1);
	check_str("dups: the first one wins", body_of(d, "kick"), "first");
	check_int("dups: 1 duplicate", (int)d.duplicates.size(), 1);
	check_str("dups: the duplicate is kick", d.duplicates[0], "kick");
	check_int("empty: 2 ignored", (int)d.empty.size(), 2);
	check_str("empty: first", d.empty[0], "empty");
	check_str("empty: second", d.empty[1], "empty2");
	check_str("empty: not in the dict", body_of(d, "empty"), "<nil>");
}

static void test_parse_crlf_bom() {
	const std::string src = "\xEF\xBB\xBF### kick\r\nNdef(\\k).play;\r\n### pad\r\nNdef(\\p).play;\r\n";
	const Snippets d = parse_snippets(src);
	check_int("crlf/bom: 2 snippets (the BOM does not break the 1st header)", (int)d.size(), 2);
	check_str("crlf/bom: body with no \\r", body_of(d, "kick"), "Ndef(\\k).play;");
	check_str("crlf/bom: second", body_of(d, "pad"), "Ndef(\\p).play;");
}

static void test_parse_degenerate() {
	const Snippets none = parse_snippets("");
	check_int("empty: 0 snippets", (int)none.size(), 0);
	const Snippets nohead = parse_snippets("just code\nloose\n");
	check_int("no headers: 0 snippets", (int)nohead.size(), 0);
	/* The body is NOT trimmed inside and does not lose inner blank lines. */
	const Snippets blanks = parse_snippets("### x\n\na\n\n\nb\n\n");
	check_str("blanks: interior intact, edges trimmed", body_of(blanks, "x"), "a\n\n\nb");
	/* Indentation of the first content line: kept (verbatim snippet). */
	const Snippets ind = parse_snippets("### x\n\t\tif (true) { }\n");
	check_str("indentation of the 1st line is preserved", body_of(ind, "x"), "\t\tif (true) { }");
}

static void test_keys_list() {
	const Snippets d = parse_snippets("### a\n1\n### b\n2\n### c\n3\n### d\n4\n");
	check_str("keys_list: todas", d.keys_list(), "a b c d");
	check_str("keys_list: max 2", d.keys_list(2), "a b \xe2\x80\xa6");
	check_str("keys_list: empty", Snippets().keys_list(), "");
}

/* ------------------------------------------------------------------------- */
/* matching of the call */

static void test_match() {
	std::string key;
	check_bool("match: snip.kick", snip_matches("snip.kick", key), true);
	check_str("match: key", key, "kick");

	check_bool("match: with trailing \\n final (added by eval)", snip_matches("snip.kick\n", key), true);
	check_bool("match: with spaces and newlines around", snip_matches("\n  snip.kick  \n\n", key), true);
	check_bool("match: with a trailing `;`", snip_matches("snip.kick;", key), true);
	check_bool("match: with `;` and spaces", snip_matches("  snip.kick ;  \n", key), true);
	check_bool("match: digits key", snip_matches("snip.20", key), true);
	check_str("match: digits key", key, "20");
	check_bool("match: key with underscore", snip_matches("snip.my_snip", key), true);

	check_bool("no: bare snip.", snip_matches("snip.", key), false);
	check_bool("no: snip without dot", snip_matches("snip", key), false);
	check_bool("no: key with dot", snip_matches("snip.a.b", key), false);
	check_bool("no: prefijo pegado", snip_matches("xsnip.kick", key), false);
	check_bool("no: uppercase", snip_matches("Snip.kick", key), false);
	check_bool("no: space between prefix and key", snip_matches("snip .kick", key), false);
	check_bool("no: two `;`", snip_matches("snip.kick;;", key), false);
	check_bool("no: hyphen in the key", snip_matches("snip.my-key", key), false);
	check_bool("no: code around it", snip_matches("x = snip.kick", key), false);
	check_bool("no: comentario", snip_matches("// snip.kick", key), false);
	check_bool("no: two snippets", snip_matches("snip.a; snip.b", key), false);
	check_bool("no: empty", snip_matches("", key), false);
}

/* ------------------------------------------------------------------------- */
/* integration with the buffer (what `sc_code` does when evaluating) */

static void test_buffer_replace() {
	Buffer buf;
	const std::string doc = "x = 1;\nsnip.kick\ny = 2;\n";
	buf.set_text(doc);

	const Snippets d = parse_snippets("### kick\nNdef(\\kick, {\n\tSinOsc.ar(60)\n}).play;\n");
	const std::string *body = d.find("kick");
	check_bool("buffer: the key exists", body != nullptr, true);

	const int start = (int)doc.find("snip.kick");
	const int end = start + 9;
	const int depth = buf.undo_depth();
	buf.set_selection(start, end);
	buf.insert_text(*body);
	const std::string want = "x = 1;\nNdef(\\kick, {\n\tSinOsc.ar(60)\n}).play;\ny = 2;\n";
	check_str("buffer: replaces only the span", buf.text(), want);
	check_int("buffer: caret at the end of the snippet", buf.caret(), start + (int)body->size());
	check_bool("buffer: no selection", buf.has_selection(), false);
	check_int("buffer: ONE undo step", buf.undo_depth(), depth + 1);

	buf.undo();
	check_str("buffer: undo vuelve al `snip.kick`", buf.text(), doc);
	buf.redo();
	check_str("buffer: redo repeats the snippet", buf.text(), want);
}

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (!strcmp(argv[i], "-v"))
			gVerbose = true;

	printf("snip_test — snippets file and `snip.KEY` matching (S6)\n");
	printf("\n-- headers --\n");
	test_header();
	printf("\n-- parseo --\n");
	test_parse_base();
	test_parse_comments_and_headers();
	test_parse_duplicates_and_empty();
	test_parse_crlf_bom();
	test_parse_degenerate();
	test_keys_list();
	printf("\n-- invocation --\n");
	test_match();
	printf("\n-- buffer integration --\n");
	test_buffer_replace();

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
