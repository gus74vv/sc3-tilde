// SPDX-License-Identifier: GPL-3.0-only
/* sc_lexer.h — sclang lexer for `sc_code`, PORT of the SuperCollider IDE.
 *
 * Provenance (SCODE.md §3.4/§3.8):
 *   vendor/supercollider/editors/sc-ide/core/sc_lexer.hpp  (tag Version-3.14.1)
 *   vendor/supercollider/editors/sc-ide/core/sc_lexer.cpp  (367 lines)
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/tokens.hpp
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/highlighter.cpp
 *     (`highlightBlock`: the part that decides WHICH tokens are stored per line)
 * Copyright (c) 2012 Jakob Leben & Tim Blechmann — GPL v2 or later.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * What was ported and what was not:
 *   - YES: the 4 states (InCode/InString/InSymbol/InComment with nesting by a
 *     value >100) and the 17 lexical rules IN ORDER, reimplemented without Qt.
 *   - YES: the semantics of the IDE highlighter, which is what defines the tokens
 *     the REGIONS consume: WhiteSpace, SingleLineComment and
 *     MultiLineCommentStart are NOT stored; from a string/symbol only the
 *     quotes are stored (`StringMark`/`SymbolMark`); the comment and string
 *     content never produces tokens ⇒ the parentheses inside them do not exist.
 *   - NO: Qt, the highlighter as such (colors) and the global lexer state.
 *
 * Deliberate differences (documented, not accidental):
 *   - Offsets are UTF-8 BYTES, not QChar (UTF-16). In UTF-8 continuation bytes
 *     are >= 0x80, which means they never collide with ASCII (quotes,
 *     parentheses, the start of a comment), so the token splitting and the
 *     states are identical; only the positions change for non-ASCII text. A
 *     stray non-ASCII byte falls into `Unknown` with length 1. The editor (S1)
 *     translates code point ↔ byte in its buffer.
 *   - `\b`/`\w`/`\s` are resolved with the ASCII table, same as sclang
 *     identifiers (the IDE with QRegularExpression uses Unicode; nothing changes
 *     for sclang because its names are ASCII).
 *
 * The 17 rules (EXACT order; `Float` ⊂ `RadixFloat` and `Class`/`Primitive` ⊂
 * `SymbolArg`, which is why they come later) and the details of each one are in
 * the .cpp.
 */

#pragma once

#include <string>
#include <vector>

namespace sc_code {

/* Same order and same names as the IDE Token::Type (tokens.hpp). The numeric
 * value is not used anywhere, but keeping the order makes the diff against the
 * original source trivial. */
enum TokenType {
	Unknown = 0,
	WhiteSpace,
	Keyword,
	Builtin,
	Primitive,
	Name,
	Class,
	Symbol,
	SymbolMark,
	StringMark,
	Char,
	RadixFloat,
	ScaleDegreeFloat,
	Float,
	HexInt,
	EnvVar,
	SymbolArg,
	SingleLineComment,
	MultiLineCommentStart,
	MultiLineCommentEnd,
	OpeningBracket,
	ClosingBracket,
	Operator,
	TokenTypeCount
};

const char *token_type_name(TokenType t);

/* IDE values: >InComment = comment nesting level. */
enum LexState {
	InCode = 0,
	InString = 1,
	InSymbol = 2,
	InComment = 100
};

struct Token {
	TokenType type;
	int start;      /* absolute byte offset in the document */
	int length;     /* bytes */
	int line;       /* line index (0-based) */
	int col;        /* position inside the line (byte) = IDE positionInBlock */
	char character; /* only if length == 1, like the IDE highlighter (`;`, `(`, …) */

	Token(): type(Unknown), start(-1), length(0), line(-1), col(-1), character(0) {}
};

struct LineTokens {
	int start;                 /* offset of the first byte of the line */
	int length;                /* bytes of the line, WITHOUT the '\n' */
	int stateIn;               /* estado al entrar (previousBlockState) */
	int stateOut;              /* estado al salir (setCurrentBlockState) */
	bool isInMultilineComment; /* TextBlockData::isInMultilineComment del IDE */
	int tokenBegin, tokenEnd;  /* rango en DocumentTokens::tokens */

	LineTokens(): start(0), length(0), stateIn(InCode), stateOut(InCode), isInMultilineComment(false),
				  tokenBegin(0), tokenEnd(0) {}
};

struct DocumentTokens {
	std::vector<LineTokens> lines;
	std::vector<Token> tokens; /* in document order, no overlaps */

	/* Line containing `offset` (0 <= offset <= size). It is the QTextCursor
	 * `block()`: if the document ends in '\n' there is a final empty line. */
	int line_of_offset(int offset) const;
};

/* 1:1 lexer of ScLexer: it is fed ONE line plus its incoming state. */
class ScLexer {
  public:
	ScLexer(const std::string &text, int offset = 0, int state = InCode);

	const std::string &text() const { return mText; }
	int state() const { return mState; }
	void setState(int state) { mState = state; }
	int offset() const { return mOffset; }
	void setOffset(int offset) { mOffset = offset; }

	TokenType nextToken(int &length);

  private:
	TokenType nextTokenInCode(int &length);
	TokenType nextTokenInString(int &length);
	TokenType nextTokenInSymbol(int &length);
	TokenType nextTokenInComment(int &length);

	const std::string &mText;
	int mOffset;
	int mState;
};

/* Lexes one line (WITHOUT the '\n') replicating
 * `SyntaxHighlighter::highlightBlock`. `lineStart` is the absolute offset of the
 * first byte of the line (for the tokens); if `outTokens` != nullptr, the storable
 * tokens are added there. */
LineTokens lex_line(const std::string &line, int lineStart, int lineIndex, int stateIn,
					std::vector<Token> *outTokens);

/* Lexes the whole document (split by '\n'; if it ends in '\n' the final empty
 * line is added, like QTextDocument). */
DocumentTokens lex_document(const std::string &text);

} // namespace sc_code
