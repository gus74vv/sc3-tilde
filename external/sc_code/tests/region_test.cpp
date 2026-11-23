// SPDX-License-Identifier: GPL-3.0-only
/* region_test.cpp — S0 gate: regions and evaluation precedence 1:1 with the IDE.
 *
 * Provenance of the expected behavior (tag Version-3.14.1):
 * vendor/supercollider/editors/sc-ide/widgets/code_editor/sc_editor.cpp
 * regionAroundCursor / currentRegion / tokenMaybeRegionStart / tokenMaybeRegionEnd /
 * bracketPairDefinesRegion / evaluateRegion / evaluateLine / evaluateDocument The 8
 * cases the design notes asks for are covered: `(` in col 0 vs indented, `)` with `;`, `)`
 * with comment, nested, caret after `)`, `{…}` is NOT a region, `(` inside
 * string/comment, EOF.
 *
 *     GPL v3 or later (ver sc_region.cpp).
 *
 * Uso: ./build/region_test [-v]
 */

#include "../sc_region.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

struct Case {
	const char *name;
	const char *text;
	int caret;
	int selStart;
	int selEnd;
	EvalMode mode;
	EvalKind wantKind;
	const char *wantText; /* expected span text ("" = nothing) */
};

static void run(const Case &c) {
	EvalSpan span = sc_eval_span(c.text, c.caret, c.selStart, c.selEnd, c.mode);
	std::string got = span.text(c.text);

	if (gVerbose) {
		printf("  %-46s %-9s [%3d,%3d) \"%s\"\n", c.name, eval_kind_name(span.kind), span.start, span.end,
			   got.c_str());
	}

	++gChecks;
	if (span.kind != c.wantKind) {
		++gFails;
		printf("  FAIL %-46s kind=%s, esperado %s\n", c.name, eval_kind_name(span.kind),
			   eval_kind_name(c.wantKind));
		return;
	}
	++gChecks;
	if (got != c.wantText) {
		++gFails;
		printf("  FAIL %-46s text=\"%s\", expected \"%s\"\n", c.name, got.c_str(), c.wantText);
		return;
	}
	/* Offset coherence: end - start == size of the extracted text. */
	++gChecks;
	if (span.has_value() && (span.end - span.start) != (int)got.size()) {
		++gFails;
		printf("  FAIL %-46s offsets incoherentes: [%d,%d) vs %zu bytes\n", c.name, span.start, span.end,
			   got.size());
	}
}

/* Direct check of current_region (without going through the
 * precedence). */
static void run_region(const char *name, const char *text, int caret, const char *wantText) {
	DocumentTokens doc = lex_document(text);
	Region region = current_region(doc, caret);
	std::string got = region.valid() ? std::string(text).substr((size_t)region.start,
															(size_t)(region.end - region.start))
									: std::string();
	if (gVerbose)
		printf("  current_region %-34s → [%d,%d) \"%s\"\n", name, region.start, region.end, got.c_str());
	++gChecks;
	if (got != wantText) {
		++gFails;
		printf("  FAIL current_region %-30s \"%s\", esperado \"%s\"\n", name, got.c_str(), wantText);
	}
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;

	printf("region_test — port of regionAroundCursor/currentRegion from the SC IDE 3.14.1\n");

	/* Base text for the simple region: "(\na\n)" with `(` in col 0 and `)` as
	 * the last token (there is no next token). */
	const char *basic = "(\na\n)"; /* '(' 0 · 'a' 2 · ')' 4 */

	/* 1) `⌘Return` with the caret inside ⇒ the region, with parentheses and nothing
	 * else. */
	run({ "basic region (caret on 'a')", basic, 2, -1, -1, EvalModeRegion, EvalRegion, "(\na\n)" });

	/* 2) caret ON the `(` ⇒ currentRegion advances 1 and it is still the region. */
	run({ "caret on the '('", basic, 0, -1, -1, EvalModeRegion, EvalRegion, "(\na\n)" });

	/* 3) caret right AFTER the `)` ⇒ it steps back 1 ⇒ the region. */
	run({ "caret right after the ')'", basic, 5, -1, -1, EvalModeRegion, EvalRegion, "(\na\n)" });

	/* 4) `⇧Return` (evaluateLine) does NOT look at the region: it evaluates the
	 * caret line. */
	run({ "⇧Return inside a region ⇒ line", basic, 2, -1, -1, EvalModeLine, EvalLine, "a" });

	/* 5) an indented `(` does NOT open a region (positionInBlock != 0) ⇒ it falls
	 * back to the line. */
	run({ " '(' indented is not a region", "  (\n  a\n  )", 7, -1, -1, EvalModeRegion, EvalLine, "  a" });

	/* 6) `)` followed by `;` DOES close a region — and the `;` stays
	 * out. */
	run({ "')' with ';'", "(\na\n);", 2, -1, -1, EvalModeRegion, EvalRegion, "(\na\n)" });
	run_region("')' with ';' (currentRegion)", "(\na\n);", 2, "(\na\n)");

	/* 7) `)` followed by a comment: the comment produces no tokens, so the `)`
	 * remains as the last token of its line ⇒ valid region. */
	run({ "')' seguido de comentario", "(\na\n) // fin", 2, -1, -1, EvalModeRegion, EvalRegion, "(\na\n)" });

	/* 8) `)` followed by any token on the SAME line ⇒ it does not close a region. */
	run({ "')' followed by another token", "( a ) b", 2, -1, -1, EvalModeRegion, EvalLine, "( a ) b" });

	/* 9) Nested with the inner `(` indented: the outer pair qualifies and
	 * contains the caret ⇒ the OUTER one is evaluated (the walk goes over
	 * top-level pairs, not the closest one). */
	run({ "nested: the outer region wins", "(\na (\nb\n)\nc\n)", 6, -1, -1, EvalModeRegion, EvalRegion,
		  "(\na (\nb\n)\nc\n)" });

	/* 10) Verified consequence of the top-level walk: if the OUTER pair is not a
	 * region (indented), the inner pair is NOT visited ⇒ there is no region and
	 * it falls back to the line. */
	run({ "nested: outer is not a region ⇒ line", "  (\n(\nb\n)\n)", 6, -1, -1, EvalModeRegion, EvalLine,
		  "b" });

	/* 11) `{ … }` and `[ … ]` are NOT regions (only `( … )`). */
	run({ "'{…}' is not a region", "{\na\n}", 2, -1, -1, EvalModeRegion, EvalLine, "a" });
	run({ "'[…]' is not a region", "[\na\n]", 2, -1, -1, EvalModeRegion, EvalLine, "a" });

	/* 11b) Symptom of the design notes (30-oct): a multi-line `{ … }` block with the
	 * caret on the last line ⇒ ⌘⏎ sends ONLY that line (`}`) and sclang answers
	 * `unmatched '}'`. It is the direct consequence of 11) and it is parity with the
	 * IDE (its `currentRegion` does not look at braces either): it is NOT an eval bug.
	 * The workaround is selecting the block, using ⌘⇧⏎ or wrapping it in `( … )`.
	 * the design notes */
	run({ "'{…}' 3 lines, caret on '}' ⇒ evaluates '}' (regression)", "{\na\n}", 4, -1, -1, EvalModeRegion, EvalLine,
		  "}" });

	/* 12) `(` inside a string is not a token (only the quotes are stored). */
	run({ "'(' inside a string", "\"(\"\na\n)", 4, -1, -1, EvalModeRegion, EvalLine, "a" });

	/* 13) `(` inside a comment neither: the outer region is the only one. */
	run({ "'(' inside a comment", "(\n/* ( */\na\n)", 10, -1, -1, EvalModeRegion, EvalRegion,
		  "(\n/* ( */\na\n)" });

	/* 14) Unclosed region ⇒ there is no region. */
	run({ "unclosed region", "(\na", 2, -1, -1, EvalModeRegion, EvalLine, "a" });

	/* 15) EOF: with the document ending in '\n' the final caret falls on an empty
	 * line ⇒ nothing is evaluated. */
	run({ "EOF after '\\n' ⇒ nothing", "(\na\n)\n", 6, -1, -1, EvalModeRegion, EvalNothing, "" });
	run({ "EOF on an empty line (⇧Return) ⇒ nothing", "a\n\nb", 2, -1, -1, EvalModeLine, EvalNothing, "" });

	/* 16) Two regions: the one containing the caret is chosen. */
	run({ "two regions (caret on the 2nd)", "(\na\n)\n\n(\nb\n)", 10, -1, -1, EvalModeRegion, EvalRegion,
		  "(\nb\n)" });
	run({ "two regions (caret on the 1st)", "(\na\n)\n\n(\nb\n)", 2, -1, -1, EvalModeRegion, EvalRegion,
		  "(\na\n)" });

	/* 17) The non-empty selection WINS, in both modes, even when there is a
	 * region. */
	run({ "selection wins over the region (⌘Return)", "(\na\n)", 2, 0, 3, EvalModeRegion, EvalSelection, "(\na" });
	run({ "selection wins over the line (⇧Return)", basic, 2, 0, 3, EvalModeLine, EvalSelection, "(\na" });

	/* 18) `⌘⇧Return`: whole document, without looking at selection or
	 * caret. */
	run({ "documento (⌘⇧Return)", basic, 2, 0, 3, EvalModeDocument, EvalDocument, "(\na\n)" });

	/* 19) Line: without the trailing '\n', and the caret line (not the
	 * previous one). */
	run({ "caret line", "uno\ndos\ntres", 6, -1, -1, EvalModeLine, EvalLine, "dos" });
	run({ "caret line at the end", "uno\ndos\ntres", 12, -1, -1, EvalModeLine, EvalLine, "tres" });

	/* 20) A single one-line region: `( 1 + 1 )` in col 0 with `;`. */
	run({ "single-line region", "( 1 + 1 );", 4, -1, -1, EvalModeRegion, EvalRegion, "( 1 + 1 )" });

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
