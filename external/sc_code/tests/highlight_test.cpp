// SPDX-License-Identifier: GPL-3.0-only
/* highlight_test.cpp — S4 gate: per-line tokens, states and incremental cache.
 *
 * Pins down three things that are expensive to see in the GUI and easy to
 * break: 1. the splitting of EACH line covers the text without gaps and with
 * the right type, 2. the incoming/outgoing states (nested multi-line comments
 * and strings) propagate like in the IDE, 3. the cache relexes ONLY what is
 * needed (measured with `lexed_lines()`).
 *
 *     GPL v3 or later (ver sc_highlight.cpp).
 *
 * Uso: ./build/highlight_test [-v]
 */

#include "../sc_buffer.h"
#include "../sc_highlight.h"

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
		printf("  FAIL %-56s got %d, expected %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-56s %d\n", what, got);
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-56s got %s, expected %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-56s %s\n", what, got ? "true" : "false");
	}
}

static void check_dbl(const char *what, double got, double want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-56s got %g, expected %g\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-56s %g\n", what, got);
	}
}

static const char *tname(TokenType t) { return token_type_name(t); }

/* Types of the line, in order, as a string ("Name OpeningBracket …"). */
static std::string types_of(const std::vector<Token> &toks) {
	std::string s;
	for (size_t i = 0; i < toks.size(); ++i) {
		if (i)
			s += ' ';
		s += tname(toks[i].type);
	}
	return s;
}

static void check_types(const char *what, const std::vector<Token> &toks, const char *want) {
	const std::string got = types_of(toks);
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-56s\n       got  \"%s\"\n       expected \"%s\"\n", what, got.c_str(), want);
	} else if (gVerbose) {
		printf("  ok   %-56s \"%s\"\n", what, got.c_str());
	}
}

static std::string describe(const std::vector<Token> &toks) {
	std::string s;
	for (size_t i = 0; i < toks.size(); ++i) {
		char buf[128];
		snprintf(buf, sizeof(buf), "%s[%d,%d] ", tname(toks[i].type), toks[i].col, toks[i].length);
		s += buf;
	}
	return s;
}

/* Invariant: the tokens cover the line without gaps (or overlaps). */
static void check_coverage(const char *what, const Buffer &buf, int line, const std::vector<Token> &toks) {
	int expect = 0;
	bool ok = true;
	for (size_t i = 0; i < toks.size(); ++i) {
		if (toks[i].col != expect || toks[i].length <= 0) {
			ok = false;
			break;
		}
		expect = toks[i].col + toks[i].length;
	}
	if (expect != buf.line_length(line))
		ok = false;

	++gChecks;
	if (!ok) {
		++gFails;
		printf("  FAIL %-56s line %d (len %d): %s\n", what, line, buf.line_length(line), describe(toks).c_str());
	} else if (gVerbose) {
		printf("  ok   %-56s line %d\n", what, line);
	}
}

static void check_doc_coverage(const char *what, const Buffer &buf, Highlight &hl) {
	for (int i = 0; i < buf.line_count(); ++i)
		check_coverage(what, buf, i, hl.ensure(buf, i));
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (!strcmp(argv[i], "-v"))
			gVerbose = true;

	printf("sc_highlight — per-line tokens + incremental cache\n");

	/* --- a typical code line ----------------------------------------------- */
	{
		Buffer buf("Ndef(\\x, { SinOsc.ar(440) * 0.1 }).play");
		Highlight hl;
		const std::vector<Token> &t = hl.ensure(buf, 0);
		/* Careful: punctuation (. , ;) has NO rule in the IDE lexer ⇒ Unknown, which
		 * is painted with the text color (checked in lexer_test). */
		check_types("Ndef(\\x, { SinOsc.ar(440) * 0.1 }).play", t,
					"Class OpeningBracket Symbol Unknown WhiteSpace OpeningBracket WhiteSpace Class Unknown Name OpeningBracket "
					"Float ClosingBracket WhiteSpace Operator WhiteSpace Float WhiteSpace ClosingBracket ClosingBracket Unknown "
					"Name");
		check_coverage("code: coverage", buf, 0, t);
		check_int("code: stateOut", hl.state_out(0), InCode);
		check_bool("code: not a comment", hl.in_multiline_comment(0), false);
	}

	/* --- single-line comment and string ------------------------------------ */
	{
		Buffer buf("x = 1; // a comment with ( and \"\n");
		Highlight hl;
		const std::vector<Token> &t = hl.ensure(buf, 0);
		check_types("single-line comment", t,
					"Name WhiteSpace Operator WhiteSpace Float Unknown WhiteSpace SingleLineComment");
		check_coverage("comment: coverage", buf, 0, t);
		check_int("comentario: stateOut", hl.state_out(0), InCode);
		check_bool("single-line comment: in_multiline_comment", hl.in_multiline_comment(0), false);
	}
	{
		Buffer buf("x = \"hola (mundo)\";\n");
		Highlight hl;
		const std::vector<Token> &t = hl.ensure(buf, 0);
		check_types("single-line string", t,
					"Name WhiteSpace Operator WhiteSpace StringMark StringMark StringMark Unknown");
		check_coverage("string: cobertura", buf, 0, t);
		/* the parentheses inside the string do NOT produce bracket tokens */
		int brackets = 0;
		for (size_t i = 0; i < t.size(); ++i)
			if (t[i].type == OpeningBracket || t[i].type == ClosingBracket)
				++brackets;
		check_int("string: the inner parens are not tokenized", brackets, 0);
	}

	/* --- multi-line comment (3 lines) -------------------------------------- */
	{
		const char *text =
			"/*\n"
			"Ndef(\\x, { 1 })\n"
			"*/\n"
			"x = 1;\n";
		Buffer buf(text);
		Highlight hl;
		const std::vector<Token> &l1 = hl.ensure(buf, 1);
		check_bool("comment: line 1 inside", hl.in_multiline_comment(1), true);
		check_int("comment: line 1 stateIn", hl.state_in(1), InComment);
		check_int("comment: line 1 stateOut", hl.state_out(1), InComment);
		/* all the content is "comment": there are no brackets or symbols */
		bool only_comment = true;
		for (size_t i = 0; i < l1.size(); ++i)
			if (l1[i].type != MultiLineCommentStart)
				only_comment = false;
		check_bool("comment: only comment tokens", only_comment, true);
		check_int("comment: line 2 stateOut", hl.state_out(2), InCode);
		check_int("comment: line 3 stateIn", hl.state_in(3), InCode);
		check_doc_coverage("comentario: cobertura", buf, hl);
	}

	/* --- comentario anidado (el estado > 100 cuenta niveles) -------------- */
	{
		const char *text =
			"/* a\n"
			"   /* b\n"
			"   */\n"
			"   c\n"
			"*/\n"
			"x = 1;\n";
		Buffer buf(text);
		Highlight hl;
		hl.ensure(buf, 5);
		check_int("nested: line 1 stateOut", hl.state_out(1), InComment + 1);
		check_int("nested: line 2 stateOut", hl.state_out(2), InComment);
		check_int("nested: line 3 stateOut", hl.state_out(3), InComment);
		check_int("nested: line 4 stateOut", hl.state_out(4), InCode);
		check_int("nested: line 5 stateOut", hl.state_out(5), InCode);
		check_bool("nested: line 5 is code already", hl.in_multiline_comment(5), false);
	}

	/* --- comment closed on the same line: the IDE marks it as a comment --- */
	{
		Buffer buf("x = 1; /* hola */\ny = 2;\n");
		Highlight hl;
		hl.ensure(buf, 1);
		check_bool("inline /* */: in_multiline_comment (paridad IDE)", hl.in_multiline_comment(0), true);
		check_int("inline /* */: stateOut goes back to code", hl.state_out(0), InCode);
		check_bool("inline /* */: the next line is not", hl.in_multiline_comment(1), false);
		check_doc_coverage("inline /* */: cobertura", buf, hl);
	}

	/* --- string that continues on the next line ---------------------------- */
	{
		Buffer buf("x = \"abc\ndef\";\nz = 1;\n");
		Highlight hl;
		hl.ensure(buf, 2);
		check_int("multi-line string: line 0 stateOut", hl.state_out(0), InString);
		check_int("multi-line string: line 1 stateIn", hl.state_in(1), InString);
		check_int("multi-line string: line 1 stateOut", hl.state_out(1), InCode);
		check_int("multi-line string: line 2 in code", hl.state_in(2), InCode);
		check_doc_coverage("multi-line string: coverage", buf, hl);
	}

	/* --- multi-line comment + string inside -------------------------------- */
	{
		const char *text =
			"/*\n"
			"\"no soy un string\"\n"
			"*/\n"
			"\"ahora sí\";\n";
		Buffer buf(text);
		Highlight hl;
		hl.ensure(buf, 3);
		check_int("string inside a comment: line 1 stateOut", hl.state_out(1), InComment);
		check_int("string inside a comment: line 3 stateOut", hl.state_out(3), InCode);
		check_bool("real string: line 3 is not a comment", hl.in_multiline_comment(3), false);
	}

	/* --- cache incremental ------------------------------------------------ */
	{
		std::string text;
		for (int i = 0; i < 100; ++i) {
			char buf[64];
			snprintf(buf, sizeof(buf), "x%d = %d;\n", i, i);
			text += buf;
		}
		Buffer buf(text);
		Highlight hl;

		hl.ensure(buf, 99);
		check_int("cache: 100 lines the first time", hl.lexed_lines(), 100);

		hl.reset_lexed_counter();
		hl.ensure(buf, 99);
		check_int("cache: asking for the same does not relex", hl.lexed_lines(), 0);

		hl.reset_lexed_counter();
		hl.ensure(buf, 42);
		check_int("cache: asking for an old line neither", hl.lexed_lines(), 0);

		/* Editing line 50 (like typing there): only 50..60 are relexed. */
		hl.invalidate_from(50);
		hl.reset_lexed_counter();
		hl.ensure(buf, 60);
		check_int("cache: edit 50, ask 60 relexes 11", hl.lexed_lines(), 11);

		hl.reset_lexed_counter();
		hl.ensure(buf, 99);
		check_int("cache: the part below was there (39 new)", hl.lexed_lines(), 39);

		/* Typing at the end: a single line. */
		hl.invalidate_from(99);
		hl.reset_lexed_counter();
		hl.ensure(buf, 99);
		check_int("cache: editing the last one relexes 1", hl.lexed_lines(), 1);

		/* Typing at the very top invalidates everything (it lexes nothing until
		 * asked). */
		hl.invalidate_from(0);
		hl.reset_lexed_counter();
		check_int("cache: invalidar no lexea", hl.lexed_lines(), 0);
		hl.ensure(buf, 3);
		check_int("cache: asking for 3 relexes 4", hl.lexed_lines(), 4);
	}

	/* --- state propagation after an edit ----------------------------------- */
	{
		/* Removing the comment closing changes the state of ALL lines below: it
		 * is the case the per-`stateIn` cache has to detect. */
		Buffer antes("/* a\nb\n*/\nc = 1;\n");
		Highlight hl;
		hl.ensure(antes, 3);
		check_int("propagation: before, 3 is code", hl.state_in(3), InCode);

		Buffer despues("/* a\nb\nx\nc = 1;\n"); /* the 2nd line no longer closes */
		hl.invalidate_from(2);
		hl.ensure(despues, 3);
		check_int("propagation: after, 3 starts in a comment", hl.state_in(3), InComment);
		check_bool("propagation: 3 is all comment", hl.in_multiline_comment(3), true);
		check_doc_coverage("propagation: coverage", despues, hl);
	}

	/* --- strings con UTF-8 ------------------------------------------------ */
	{
		Buffer buf("// línea ñandú\nx = \"ñandú\";\n");
		Highlight hl;
		hl.ensure(buf, 1);
		check_coverage("UTF-8: the comment line covers its bytes", buf, 0, hl.ensure(buf, 0));
		check_coverage("UTF-8: the string line covers its bytes", buf, 1, hl.ensure(buf, 1));
		check_int("UTF-8: line 0 is still code", hl.state_out(0), InCode);
	}

	/* --- reset ------------------------------------------------------------ */
	{
		Buffer buf("x = 1;\n");
		Highlight hl;
		hl.ensure(buf, 0);
		hl.reset();
		check_int("reset: empty cache", hl.line_count(), 0);
		hl.reset_lexed_counter();
		hl.ensure(buf, 0);
		check_int("reset: vuelve a lexear", hl.lexed_lines(), 1);
	}

	/* --- colores ---------------------------------------------------------- */
	{
		const Color text = highlight_color(Name);
		const Color s = highlight_color(StringMark);
		const Color c = highlight_color(SingleLineComment);
		check_bool("color: string differs from text", s.g != text.g || s.r != text.r, true);
		check_bool("color: comment differs from text", c.r != text.r || c.g != text.g, true);
		check_bool("color: comentario apagado (r < 0.6)", c.r < 0.6, true);
		check_bool("color: keyword distinta de name",
				   highlight_color(Keyword).r != text.r || highlight_color(Keyword).g != text.g, true);
		/* the four number types share a color (IDE parity) */
		const Color f = highlight_color(Float);
		check_bool("color: Float == HexInt == RadixFloat == ScaleDegree",
				   f.r == highlight_color(HexInt).r && f.r == highlight_color(RadixFloat).r
					   && f.r == highlight_color(ScaleDegreeFloat).r,
				   true);
		/* the three comment forms share a color */
		check_bool("color: the 3 comment types share a color",
				   c.r == highlight_color(MultiLineCommentStart).r
					   && c.r == highlight_color(MultiLineCommentEnd).r,
				   true);
		check_dbl("color: everything in 0..1", highlight_color(Operator).b <= 1.0, true);
	}

	/* --- types_of on an empty line ----------------------------------------- */
	{
		Buffer buf("\n\nx\n");
		Highlight hl;
		check_int("empty line: no tokens", (int)hl.ensure(buf, 0).size(), 0);
		check_int("empty line: stateOut", hl.state_out(0), InCode);
		check_int("line with x: 1 token", (int)hl.ensure(buf, 2).size(), 1);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
