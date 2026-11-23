// SPDX-License-Identifier: GPL-3.0-only
/* indent_test.cpp — S3 gate: indentation with SC IDE parity.
 *
 * Provenance of the behavior (tag Version-3.14.1):
 * vendor/supercollider/editors/sc-ide/widgets/code_editor/sc_editor.cpp indent L597
 * (accumulative global_level + stack of marks), indentationLevel L709,
 * makeIndentationString L688. The non-obvious rules are documented in `sc_indent.h`.
 *
 * The strong case is `SNIPPET`: a REAL fragment formatted by the IDE. For each line
 * it verifies that the computed level matches the one the line has in the file
 * (indent / 4). If the port deviates by a single comma, this table says so.
 *
 *     GPL v3 or later (ver sc_indent.cpp).
 *
 * Uso: ./build/indent_test [-v]
 */

#include "../sc_indent.h"

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

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-56s got \"%s\", expected \"%s\"\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-56s \"%s\"\n", what, got.c_str());
	}
}

/* Fragment formatted by the IDE (indent of 4, spaces). */
static const char *SNIPPET =
	"(\n"
	"Ndef(\\ftonic, {\n"
	"    GVerb.ar(\n"
	"        SinOsc.ar(\n"
	"            Select.kr(\n"
	"                Hasher.kr(\n"
	"                    Duty.kr(\n"
	"                        (1..4)/4,\n"
	"                        0,\n"
	"                        Dwhite(0,1)\n"
	"                    )\n"
	"                )*5,midicps([0,3,5,7,10]+\\tune.kr(60))\n"
	"            )\n"
	"    ).sum,200,3)/20\n"
	"} * -10.dbamp\n"
	").clock_(t).quant_(1).fadeTime_(0.5).play;\n"
	")\n";

/* Expected level for a NEW line inserted at `offset`. */
static void check_level_at(const char *what, const char *text, int offset, int want) {
	check_int(what, indent_level_at_offset(text, offset), want);
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;

	printf("indent_test — port of ScCodeEditor::indent/indentationLevel (SC IDE 3.14.1)\n");

	/* --- 1) parity over a fragment formatted by the IDE --------------------- */
	{
		const std::string text(SNIPPET);
		const DocumentTokens doc = lex_document(text);
		int bad = 0;
		for (size_t i = 0; i < doc.lines.size(); ++i) {
			int lead = 0;
			const int start = doc.lines[i].start;
			while (start + lead < (int)text.size() && text[(size_t)(start + lead)] == ' ')
				++lead;
			const int want = lead / 4; /* the IDE's default indentWidth: 4 */
			const int got = indent_level_for_line(doc, (int)i);
			++gChecks;
			if (got != want) {
				++gFails;
				++bad;
				printf("  FAIL snippet line %2zu: level %d, the file has %d (%d spaces)\n", i, got, want,
					   lead);
			} else if (gVerbose) {
				printf("  ok   snippet line %2zu  level %d\n", i, got);
			}
		}
		if (gVerbose)
			printf("  snippet: %zu lines, %d deviations\n", doc.lines.size(), bad);
	}

	/* --- 2) the non-obvious rule: `(` in column 0 does not add a level ------ */
	{
		/* Top-level region: the content stays in the same column. */
		check_level_at("'(' at column 0 ⇒ level 0", "(", 1, 0);
		check_level_at("region: caret after '\\n'", "(\n", 2, 0);
		check_level_at("region with content", "(\na = 1;\n", 9, 0);
		/* Indentado (o anidado) SÍ suma. */
		check_level_at("'(' indented ⇒ level 1", "  (", 3, 1);
		check_level_at("'(' nested ⇒ level 1", "(\n(", 3, 0); /* the 2nd one is also at col 0 */
		/* With the stack NOT empty the column-0 `(` does count (it is nested). */
		check_level_at("'(' at col 0 inside '{}' ⇒ level 2", "{\n(", 3, 2);
		check_level_at("'(' after a call ⇒ adds", "foo(1,", 6, 1);
	}

	/* --- 3) one level per block, not per bracket ---------------------------- */
	{
		/* Two openers on the SAME line = ONE level (IDE rule). */
		check_level_at("'Ndef(\\x, {' = one level", "Ndef(\\x, {", 11, 1);
		check_level_at("two '[' on the same line = one level", "a = [[1", 7, 1);
		/* On different lines they are indeed two. */
		check_level_at("'[' + '[' on two lines = two levels", "a = [\n[", 7, 2);
		check_level_at("'[' + '{' on two lines = two levels", "a = [\n{", 7, 2);
	}

	/* --- 4) cierres -------------------------------------------------------- */
	{
		check_level_at("after closing a pair ⇒ level 0", "foo(1)", 6, 0);
		check_level_at("'}' closes the 'Ndef(\\x, {' level", "Ndef(\\x, {\n1\n}", 15, 0);
		check_level_at("one extra close does not break", ")))\n", 0, 0);
	}

	/* --- 5) strings y comentarios no cuentan ------------------------------ */
	{
		check_level_at("'(' inside a string", "x = \"(\",", 8, 0);
		check_level_at("'(' inside a line comment", "// (\n", 5, 0);
		check_level_at("'(' inside a multi-line comment", "/* (\n", 5, 0);
		check_level_at("'{' inside a comment", "/* { */\n", 8, 0);
	}

	/* --- 6) the cut is at the caret, not at the end of the line ------------- */
	{
		/* The tokens to the RIGHT of the caret move to the new line ⇒ they do not
		 * count. */
		check_level_at("'(' to the right of the caret does not count", "()", 1, 0);
		check_level_at("'{' to the left of the caret does count", "{}", 1, 1);
		check_level_at("close before the caret", "a(b)c", 4, 0);
	}

	/* --- 8) the two special cases of indent() (comments and strings) ------- */
	{
		/* inside a multi-line comment there is NO reindent; neither at the
		 * start of a comment (the lexer does not see it as "in comment" when
		 * deciding). */
		const char *doc1 =
			"/*\n"
			"Ndef(\\x, {\n"
			"        1\n"
			"})\n"
			"*/\n"
			"y = 1;\n";
		LineIndent a = indent_for_line_at_offset(doc1, 4); /* line 1 (comment) */
		check_bool("multi-line comment: not reindented", a.apply, false);
		a = indent_for_line_at_offset(doc1, 0); /* line 0 is where the comment starts */
		check_bool("comment start alone: not reindented", a.apply, false);
		a = indent_for_line_at_offset(doc1, 36); /* the last line "y = 1;" */
		check_bool("code after the comment: reindented", a.apply, true);

		/* Inside a multi-line string it IS reindented at level 0 (it strips the
		 * indentation). */
		a = indent_for_line_at_offset("x = \"abc\ndef\";\n", 9);
		check_bool("multi-line string: applied", a.apply, true);
		check_int("multi-line string: level 0", a.level, 0);

		/* Return inside comment/string: the new line is not indented. */
		check_level_at("Return inside a comment ⇒ level 0", "/*\n\n*/\n", 3, 0);
		check_level_at("Return inside a string ⇒ level 0", "x = \"abc\n", 9, 0);
		/* ... but after a comment CLOSED on the same line, it is. */
		check_level_at("Return after a closed comment ⇒ block level", "{\n/* x */", 10, 1);
	}

	/* --- 7) indent_string ------------------------------------------------- */
	{
		check_str("indent_string(0, 4)", indent_string(0, 4), "");
		check_str("indent_string(2, 4)", indent_string(2, 4), "        ");
		check_str("indent_string(1, 2)", indent_string(1, 2), "  ");
		check_str("indent_string(-1, 4)", indent_string(-1, 4), "");
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
