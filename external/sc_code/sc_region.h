// SPDX-License-Identifier: GPL-3.0-only
/* sc_region.h — evaluation region and "what do I evaluate?" for `sc_code`.
 *
 * Provenance:
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/sc_editor.cpp
 *     (tag Version-3.14.1): previousOpeningBracket L971, nextClosingBracket L991,
 *     matchBracket L1014, nextBracketPair L1033, tokenMaybeRegionStart L1084,
 *     tokenMaybeRegionEnd L1089, bracketPairDefinesRegion L1097,
 *     regionAroundCursor L1107, currentRegion L1132, evaluateLine L1206,
 *     evaluateRegion L1246, evaluateDocument L1292.
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/tokens.hpp
 *     (TokenIterator: logical block, not byte, and `around`/`leftOf`/`rightOf`).
 * Copyright (c) 2012 Jakob Leben & Tim Blechmann — GPL v2 or later.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * The rule, in plain words (it is a PORT, not an interpretation):
 *   1. `(` starts a region only if it is the FIRST token of its line.
 *   2. `)` ends a region only if the next token does not exist, is on another
 *      line, or is `;`.
 *   3. The caret has to be strictly inside: start < caret and end >= caret.
 *   4. `{ … }` and `[ … ]` are NOT regions.
 *   5. `currentRegion()` steps back 1 if there is a `)` to the left of the caret,
 *      and forward 1 if there is a `(` at the caret (so a caret next to a
 *      parenthesis still points at that region).
 *   6. The walk goes over TOP-LEVEL pairs from the start of the document and
 *      stops when the pair already starts after the caret ⇒ if the caret is
 *      inside a nested pair whose outer pair qualifies as a region, the OUTER one
 *      is evaluated (not the closest one). Verified consequence: pairs nested
 *      inside a pair that is NOT a region are never visited.
 *   7. The region includes the `)`, NOT the `;`.
 */

#pragma once

#include <string>

#include "sc_lexer.h"

namespace sc_code {

struct Region {
	int start; /* byte, inclusive */
	int end;   /* byte, exclusive */

	Region(): start(-1), end(-1) {}
	Region(int s, int e): start(s), end(e) {}
	bool valid() const { return start >= 0 && end > start; }
};

/* Bracket pair as indices into DocumentTokens::tokens (-1 = invalid). */
struct BracketPair {
	int first;
	int second;

	BracketPair(): first(-1), second(-1) {}
	bool valid() const { return first >= 0 && second >= 0; }
};

/* --- primitivas (TokenIterator + matchers del IDE) ------------------------- */

/* Token that starts at `col` or contains it, inside `line` (equivalent to
 * TokenIterator(block, positionInBlock)). The IDE searches backwards within the
 * block. */
int token_at_or_containing(const DocumentTokens &doc, int line, int col);

int next_closing_bracket(const DocumentTokens &doc, int idx);
int previous_opening_bracket(const DocumentTokens &doc, int idx);
void match_bracket(const DocumentTokens &doc, int idx, BracketPair &out);
void next_bracket_pair(const DocumentTokens &doc, int startIdx, BracketPair &out);

bool token_maybe_region_start(const DocumentTokens &doc, int idx);
bool token_maybe_region_end(const DocumentTokens &doc, int idx);
bool bracket_pair_defines_region(const DocumentTokens &doc, const BracketPair &pair);

/* --- region ---------------------------------------------------------------- */

Region region_around_cursor(const DocumentTokens &doc, int caretPos);
Region current_region(const DocumentTokens &doc, int caret);

/* --- what each shortcut evaluates -------------------------- */

enum EvalKind {
	EvalNothing = 0, /* evaluate nothing (empty line, ⇧Return with no text, …) */
	EvalSelection,   /* non-empty selection: always wins */
	EvalRegion,      /* `( … )` region */
	EvalLine,        /* caret line, without the '\n' */
	EvalDocument     /* documento entero (⌘⇧Return) */
};

const char *eval_kind_name(EvalKind kind);

struct EvalSpan {
	EvalKind kind;
	int start; /* byte, inclusivo */
	int end;   /* byte, exclusivo */

	EvalSpan(): kind(EvalNothing), start(-1), end(-1) {}
	bool has_value() const { return kind != EvalNothing && end > start; }
	std::string text(const std::string &doc) const;
};

/* ⌘Return vs ⇧Return mode (the IDE: EvaluateRegion vs EvaluateLine). */
enum EvalMode {
	EvalModeRegion = 0, /* ⌘Return: selection → region → line */
	EvalModeLine = 1,   /* ⇧Return: selection → line */
	EvalModeDocument = 2 /* ⌘⇧Return: document (without looking at selection or caret) */
};

/* precedence, ported from evaluateLine/evaluateRegion/evaluateDocument.
 * `caret`, `selStart`, `selEnd` are byte offsets; pass selStart == selEnd (or -1)
 * for "no selection". A negative `caret` means "use selEnd". */
EvalSpan sc_eval_span(const std::string &text, int caret, int selStart, int selEnd,
					  EvalMode mode = EvalModeRegion);

} // namespace sc_code
