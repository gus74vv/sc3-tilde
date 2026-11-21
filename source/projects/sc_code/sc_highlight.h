// SPDX-License-Identifier: GPL-3.0-only
/* sc_highlight.h — incremental PER-LINE token cache to paint `sc_code`.
 *
 * the design notes: in the IDE the highlighter and the regions SHARE the tokens of
 * each block (TextBlockData, with stateIn/stateOut). Here the same is done but
 * headless: `Highlight` stores, per line, the complete token stream (including
 * spaces and comments, which the IDE does not store because it paints them "on the
 * fly") and the incoming/outgoing lexical state. With that:
 *
 * - paint colors without lexing again (only what changed), - the object knows
 * whether a line starts inside a multi-line comment or a string (the two special
 * cases of the IDE indentation).
 *
 * Provenance of the COLOR (the splitting is already in sc_lexer):
 * vendor/supercollider/editors/sc-ide/widgets/code_editor/highlighter.cpp (tag
 * Version-3.14.1): highlightBlockInCode/InString/InSymbol/InComment, which define
 * WHICH range is painted with which format. Copyright (c) 2012 Jakob Leben & Tim
 * Blechmann — GPL v2 or later.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * Invariants: - The tokens are RELATIVE TO THE LINE (col = byte offset inside the
 * line, length = bytes) and they cover its text WITHOUT GAPS: paint uses that to
 * advance the visual column without measuring text. Tokens of length 0 are not
 * stored, except the content of a string that does not close on the line (it goes
 * as a StringMark). - Incremental cache: editing line n invalidates from n
 * downwards, but asking for a line relexes ONLY from the first dirty one up to the
 * requested one. A 300-line comment is not fully relexed because of typing at the
 * end of the document, and typing at the very top only invalidates, it does not
 * relex.
 */

#pragma once

#include <string>
#include <vector>

#include "sc_lexer.h"

namespace sc_code {

class Buffer;

/* Normalized RGB color 0..1 (paint passes it to jgraphics_set_source_rgba). */
struct Color {
	double r, g, b;
};

/* One color per TokenType. Same grouping as the IDE (highlighter.cpp): the four
 * number types share a color, the two string quotes do too, etc. Deliberate
 * difference: the IDE only colors a `Class` if it exists in the sclang
 * introspection; here there is no introspection, so it is always colored. */
Color highlight_color(TokenType type);

/* Cache token (same struct as the lexer, with `col` relative to the line). */
class Highlight {
  public:
	void reset();

	/* Line `line` changed (edit, or lines were inserted/deleted above). */
	void invalidate_from(int line);

	/* Tokens of `line`, relexing as little as possible. The vector lives in
	 * the cache: it is valid until the next edit. */
	const std::vector<Token> &ensure(const Buffer &buf, int line);

	/* Lexical info per line (does not lex: returns what is cached; `valid` tells
	 * whether it is). */
	bool line_is_valid(int line) const;
	int state_in(int line) const;  /* >= InComment: inside a comment */
	int state_out(int line) const; /* InString/InSymbol: open multi-line literal */
	/* true if ANY part of the line was lexed inside a comment (IDE
	 * TextBlockData::isInMultilineComment). */
	bool in_multiline_comment(int line) const;

	/* Instrumentation for the incrementality test (not used by the object). */
	int lexed_lines() const { return mLexed; }
	void reset_lexed_counter() { mLexed = 0; }
	int line_count() const { return (int)mLines.size(); }

  private:
	struct Line {
		bool valid = false;
		int stateIn = InCode;
		int stateOut = InCode;
		bool inComment = false;
		std::vector<Token> tokens;
	};

	Line &row(int line);

	std::vector<Line> mLines;
	int mDirtyFrom = 0; /* first invalidated line (nothing above is touched) */
	int mLexed = 0;
};

} // namespace sc_code
