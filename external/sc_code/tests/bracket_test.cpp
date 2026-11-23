// SPDX-License-Identifier: GPL-3.0-only
/* bracket_test.cpp — S5c gate: bracket matching for the caret.
 *
 * Pair highlighting is one of those things that break silently (a `)` inside a
 * string counted as a closer, a comment that "opens" a pair): the table pins exactly
 * what is highlighted at each caret position, and the cache (when the document is
 * relexed) before touching paint.
 *
 *     GPL v3 or later (ver sc_bracket.cpp).
 *
 * Uso: ./build/bracket_test [-v]
 */

#include "../sc_bracket.h"

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
		printf("  FAIL %-58s got %d, expected %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-58s %d\n", what, got);
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %s, expected %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-58s %s\n", what, got ? "true" : "false");
	}
}

/* Expected matching for a caret: `f`/`s` = -1, -1 means "nothing to highlight".
 * Every case uses a new revision: they are DIFFERENT documents and the matcher
 * caches per revision (otherwise we would be matching against the tokens of the
 * previous case). */
static int gRev = 0;
static void expect(BracketMatcher &m, const char *what, const std::string &text, int caret, int f, int s) {
	const BracketMatch r = m.at(text, caret, ++gRev);
	++gChecks;
	if (r.first != f || r.second != s) {
		++gFails;
		printf("  FAIL %-58s got (%d,%d), expected (%d,%d)\n", what, r.first, r.second, f, s);
	} else if (gVerbose) {
		printf("  ok   %-58s (%d,%d)\n", what, r.first, r.second);
	}
}

int main(int argc, char **argv) {
	gVerbose = (argc > 1 && argv[1][0] == '-');

	check_bool("is_open_bracket '('", is_open_bracket('('), true);
	check_bool("is_open_bracket '['", is_open_bracket('['), true);
	check_bool("is_open_bracket '{'", is_open_bracket('{'), true);
	check_bool("is_open_bracket ')'", is_open_bracket(')'), false);
	check_bool("is_close_bracket '}'", is_close_bracket('}'), true);
	check_bool("is_close_bracket '('", is_close_bracket('('), false);

	BracketMatcher m; /* revision 0 for the whole table */

	/* --- the basics: caret right before `(` / right after `)` --------------- */
	{
		const std::string t = "sin(440) * 2";
		expect(m, "caret before '('", t, 3, 3, 7);
		expect(m, "caret despues de ')'", t, 8, 3, 7);
		expect(m, "caret in the middle (nothing)", t, 5, -1, -1);
		expect(m, "caret al principio (letra)", t, 0, -1, -1);
	}

	/* --- anidados ----------------------------------------------------------- */
	{
		const std::string t = "a((b))"; /* a0 (1 (2 b3 )4 )5 */
		expect(m, "nested: caret before the outer '('", t, 1, 1, 5);
		expect(m, "nested: caret before the inner '('", t, 2, 2, 4);
		expect(m, "nested: caret after the inner ')'", t, 5, 2, 4);
		expect(m, "nested: caret at the end of the outer ')'", t, 6, 1, 5);
	}

	/* --- the three types do not mix ---------------------------------------- */
	{
		const std::string t = "[{}]"; /* [0 {1 }2 ]3 */
		expect(m, "mixed: '[' with ']'", t, 0, 0, 3);
		expect(m, "mixed: '{' with '}'", t, 1, 1, 2);
		expect(m, "mixed: caret between '{' and '}' (nothing)", t, 2, -1, -1);
	}

	/* --- strings, symbols y comentarios NO participan ----------------------- */
	{
		const std::string t = "(a \")\" )"; /* the middle ')' is inside a string */
		expect(m, "string: the inner ')' is ignored", t, 0, 0, 7);
		expect(m, "string: caret before the inner ')'", t, 4, -1, -1);

		const std::string c = "/* ( */ (x)";
		expect(m, "comment: the inner '(' is ignored", c, 3, -1, -1);
		expect(m, "comment: the pair after does match", c, 8, 8, 10);

		const std::string s = "'(' (y)";
		expect(m, "symbol: the inner '(' is ignored", s, 1, -1, -1);
		expect(m, "symbol: the pair after does match", s, 4, 4, 6);

		const std::string l = "// (\n(a)"; /* the '(' in the line comment is another one */
		expect(m, "comment line: ignored", l, 3, -1, -1);
		expect(m, "comment line: the pair on the 2nd line", l, 5, 5, 7);
	}

	/* --- comentario multilinea con anidamiento ------------------------------ */
	{
		const std::string t = "( /* ) /* ) */ */ )"; /* the last ')' is the real match */
		expect(m, "nested comment: inner closers ignored", t, 0, 0, 18);
		expect(m, "nested comment: inner ')' 1", t, 5, -1, -1);
		expect(m, "nested comment: inner ')' 2", t, 10, -1, -1);
		/* If the comment does NOT close, the outer '(' is left without a
		 * partner. */
		const std::string u = "( /* ) /* ) */ )";
		expect(m, "unclosed comment: '(' with no pair", u, 0, -1, -1);
	}

	/* --- multi-linea -------------------------------------------------------- */
	{
		const std::string t = "Ndef(\\x, {\n    SinOsc.ar(440)\n})";
		expect(m, "multi-line: Ndef's '(' with the final ')'", t, 4, 4, 31);
		expect(m, "multi-line: SinOsc's '(' with its own", t, 24, 24, 28);
		expect(m, "multi-line: caret at the end of the document", t, 32, 4, 31);
	}

	/* --- no partner: nothing is highlighted -------------------------------- */
	{
		const std::string t = "(a";
		expect(m, "'(' unclosed", t, 0, -1, -1);
		const std::string u = "a)";
		expect(m, "')' not opened", u, 2, -1, -1);
	}

	/* --- bordes ------------------------------------------------------------- */
	{
		const std::string vacio;
		expect(m, "documento vacio", vacio, 0, -1, -1);
		const std::string t = "()";
		expect(m, "empty pair: before the '('", t, 0, 0, 1);
		expect(m, "empty pair: after the ')'", t, 2, 0, 1);
		expect(m, "caret out of range (clamp)", t, 99, 0, 1);
		expect(m, "caret negativo (clamp)", t, -5, 0, 1);
	}

	/* --- cache: it is only relexed when needed ----------------------------- */
	{
		BracketMatcher c;
		const std::string t = "sin(440)";
		c.at(t, 3, 1); /* adyacente: lexea */
		check_int("cache: primer pareo lexea 1 documento", c.lexed_documents(), 1);
		c.at(t, 3, 1); /* mismo caret: memo */
		check_int("cache: same caret does not relex", c.lexed_documents(), 1);
		c.at(t, 5, 1); /* not adjacent: it does not even look at the tokens */
		check_int("cache: caret in common text does not relex", c.lexed_documents(), 1);
		c.at(t, 8, 1); /* adyacente, mismo documento */
		check_int("cache: another adjacent caret uses the cached document", c.lexed_documents(), 1);
		const BracketMatch r = c.at(t, 3, 2); /* new revision = the text changed */
		check_int("cache: new revision relexes", c.lexed_documents(), 2);
		check_int("  and the pairing still works", r.first, 3);
		check_int("  and the pairing still works (2)", r.second, 7);
		c.at(t + " + 1", 3, 2); /* same revision but other text: not reused */
		check_int("cache: same revision does not relex", c.lexed_documents(), 2);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
