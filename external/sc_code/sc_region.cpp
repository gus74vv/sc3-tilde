// SPDX-License-Identifier: GPL-3.0-only
/* sc_region.cpp — port of the region logic of the SC IDE editor.
 *
 * Procedencia:
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/sc_editor.cpp
 *   (tag Version-3.14.1), funciones citadas en sc_region.h.
 * Copyright (c) 2012 Jakob Leben & Tim Blechmann — GPL v2 or later.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * Mechanical translation: where the IDE walks `QTextBlock` + `TokenIterator`, here it
 * walks a flat vector of tokens in document order (the `Token`s store line/column, so
 * `block()` ↔ `tok.line` and the newline jumps are trivial).
 */

#include "sc_region.h"

#include <algorithm>

namespace sc_code {

const char *eval_kind_name(EvalKind kind) {
	switch (kind) {
	case EvalNothing: return "none";
	case EvalSelection: return "selection";
	case EvalRegion: return "region";
	case EvalLine: return "line";
	case EvalDocument: return "document";
	default: return "?";
	}
}

std::string EvalSpan::text(const std::string &doc) const {
	if (!has_value())
		return std::string();
	int s = std::max(0, start);
	int e = std::min((int)doc.size(), end);
	if (e <= s)
		return std::string();
	return doc.substr((size_t)s, (size_t)(e - s));
}

/**********************************************************************/
/* TokenIterator(block, positionInBlock)                               */
/**********************************************************************/

int token_at_or_containing(const DocumentTokens &doc, int line, int col) {
	if (line < 0 || line >= (int)doc.lines.size())
		return -1;
	const LineTokens &lt = doc.lines[(size_t)line];
	for (int i = lt.tokenEnd - 1; i >= lt.tokenBegin; --i) {
		const Token &tok = doc.tokens[(size_t)i];
		if (tok.col > col)
			continue;
		if (tok.col == col || tok.col + tok.length > col)
			return i;
	}
	return -1;
}

/**********************************************************************/
/* previousOpeningBracket / nextClosingBracket (con anidamiento)        */
/**********************************************************************/

int previous_opening_bracket(const DocumentTokens &doc, int idx) {
	int level = 0;
	while (idx >= 0 && idx < (int)doc.tokens.size()) {
		const Token &tok = doc.tokens[(size_t)idx];
		if (tok.type == OpeningBracket) {
			if (level == 0)
				return idx;
			--level;
		} else if (tok.type == ClosingBracket) {
			++level;
		}
		--idx;
	}
	return -1;
}

int next_closing_bracket(const DocumentTokens &doc, int idx) {
	int level = 0;
	while (idx >= 0 && idx < (int)doc.tokens.size()) {
		const Token &tok = doc.tokens[(size_t)idx];
		if (tok.type == ClosingBracket) {
			if (level == 0)
				return idx;
			--level;
		} else if (tok.type == OpeningBracket) {
			++level;
		}
		++idx;
	}
	return -1;
}

void match_bracket(const DocumentTokens &doc, int idx, BracketPair &out) {
	out = BracketPair();
	if (idx < 0 || idx >= (int)doc.tokens.size())
		return;

	switch (doc.tokens[(size_t)idx].type) {
	case OpeningBracket:
		out.first = idx;
		out.second = next_closing_bracket(doc, idx + 1);
		break;
	case ClosingBracket:
		out.second = idx;
		out.first = previous_opening_bracket(doc, idx - 1);
		break;
	default:
		break;
	}
}

void next_bracket_pair(const DocumentTokens &doc, int startIdx, BracketPair &out) {
	out = BracketPair();
	int it = startIdx;
	while (it >= 0 && it < (int)doc.tokens.size()) {
		if (doc.tokens[(size_t)it].type == OpeningBracket) {
			match_bracket(doc, it, out);
			return;
		}
		++it;
	}
}

/**********************************************************************/
/* tokenMaybeRegionStart / tokenMaybeRegionEnd / definition */
/**********************************************************************/

bool token_maybe_region_start(const DocumentTokens &doc, int idx) {
	if (idx < 0 || idx >= (int)doc.tokens.size())
		return false;
	const Token &tok = doc.tokens[(size_t)idx];
	return tok.character == '(' && tok.col == 0;
}

bool token_maybe_region_end(const DocumentTokens &doc, int idx) {
	if (idx < 0 || idx >= (int)doc.tokens.size())
		return false;
	const Token &tok = doc.tokens[(size_t)idx];
	if (tok.character != ')')
		return false;

	int next = idx + 1;
	if (next >= (int)doc.tokens.size())
		return true; /* there is no next token */
	const Token &nextTok = doc.tokens[(size_t)next];
	return nextTok.line != tok.line || nextTok.character == ';';
}

bool bracket_pair_defines_region(const DocumentTokens &doc, const BracketPair &pair) {
	if (pair.first < 0 || pair.second < 0)
		return false;
	if (!token_maybe_region_start(doc, pair.first) || !token_maybe_region_end(doc, pair.second))
		return false;
	return true;
}

/**********************************************************************/
/* regionAroundCursor / currentRegion                                  */
/**********************************************************************/

Region region_around_cursor(const DocumentTokens &doc, int caretPos) {
	if (doc.tokens.empty())
		return Region();

	int it = 0; /* TokenIterator::rightOf(textDocument()->begin(), 0) */
	while (it >= 0 && it < (int)doc.tokens.size()) {
		BracketPair pair;
		next_bracket_pair(doc, it, pair);

		if (pair.first >= 0 && doc.tokens[(size_t)pair.first].start < caretPos) {
			if (pair.second >= 0 && doc.tokens[(size_t)pair.second].start >= caretPos
				&& bracket_pair_defines_region(doc, pair)) {
				return Region(doc.tokens[(size_t)pair.first].start, doc.tokens[(size_t)pair.second].start + 1);
			}
		} else {
			break;
		}

		it = pair.second;
	}

	return Region();
}

Region current_region(const DocumentTokens &doc, int caret) {
	if (caret < 0)
		caret = 0;

	int line = doc.line_of_offset(caret);
	int col = caret - doc.lines[(size_t)line].start;
	int pos = caret;

	int left = token_at_or_containing(doc, line, col - 1);
	if (left >= 0 && doc.tokens[(size_t)left].type == ClosingBracket) {
		pos = caret - 1; /* caret right after ')': it still points at that region */
	} else {
		int here = token_at_or_containing(doc, line, col);
		if (here >= 0 && doc.tokens[(size_t)here].type == OpeningBracket)
			pos = caret + 1; /* caret right on '(': the region */
	}

	if (pos < 0)
		pos = 0;
	return region_around_cursor(doc, pos);
}

/**********************************************************************/
/* sc_eval_span (evaluateRegion / evaluateLine / evaluateDocument)     */
/**********************************************************************/

EvalSpan sc_eval_span(const std::string &text, int caret, int selStart, int selEnd, EvalMode mode) {
	EvalSpan span;
	const int size = (int)text.size();

	DocumentTokens doc; /* lazy: only if needed */
	bool docReady = false;

	auto clamp = [size](int p) { return p < 0 ? 0 : (p > size ? size : p); };

	/* --- ⌘⇧Return: whole document (the IDE ignores selection and caret) ------ */
	if (mode == EvalModeDocument) {
		span.kind = EvalDocument;
		span.start = 0;
		span.end = size;
		return span;
	}

	int selA = (selStart >= 0) ? clamp(selStart) : -1;
	int selB = (selEnd >= 0) ? clamp(selEnd) : -1;
	if (selA >= 0 && selB >= 0 && selB < selA)
		std::swap(selA, selB);

	/* --- 1) the non-empty selection always wins ------------------------------ */
	if (selA >= 0 && selB > selA) {
		span.kind = EvalSelection;
		span.start = selA;
		span.end = selB;
		return span;
	}

	if (caret < 0)
		caret = (selB >= 0) ? selB : 0;
	caret = clamp(caret);

	/* --- 2) ⌘Return: `( … )` region ---------------------------------------- */
	if (mode == EvalModeRegion) {
		doc = lex_document(text);
		docReady = true;
		Region region = current_region(doc, caret);
		if (region.valid()) {
			span.kind = EvalRegion;
			span.start = region.start;
			span.end = region.end;
			return span;
		}
	}

	/* --- 3) the caret line (without the trailing '\n') ---------------------- */
	if (!docReady)
		doc = lex_document(text);
	int line = doc.line_of_offset(caret);
	const LineTokens &lt = doc.lines[(size_t)line];

	if (lt.length > 0) { /* `if (text.isEmpty()) return;` del IDE */
		span.kind = EvalLine;
		span.start = lt.start;
		span.end = lt.start + lt.length;
	}
	return span;
}

} // namespace sc_code
