// SPDX-License-Identifier: GPL-3.0-only
/* sc_lexer.cpp — Qt-free port of `ScLexer` + the SC IDE `highlightBlock`.
 *
 * Provenance (SCODE.md §3.4/§3.8):
 *   vendor/supercollider/editors/sc-ide/core/sc_lexer.cpp  (tag Version-3.14.1)
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/highlighter.cpp
 * Copyright (c) 2012 Jakob Leben & Tim Blechmann — GPL v2 or later.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * The Qt regexes were replaced by hand-written anchored matchers, with the SAME
 * semantics (including the backtracking of the optional groups of `Float` and the
 * trailing `\b` of `Float`/`ScaleDegreeFloat`/`Keyword`/`Builtin`). Each matcher
 * documents the original regex next to it.
 */

#include "sc_lexer.h"

#include <cstring>

namespace sc_code {

/**********************************************************************/
/* nombres                                                             */

const char *token_type_name(TokenType t) {
	switch (t) {
	case Unknown: return "Unknown";
	case WhiteSpace: return "WhiteSpace";
	case Keyword: return "Keyword";
	case Builtin: return "Builtin";
	case Primitive: return "Primitive";
	case Name: return "Name";
	case Class: return "Class";
	case Symbol: return "Symbol";
	case SymbolMark: return "SymbolMark";
	case StringMark: return "StringMark";
	case Char: return "Char";
	case RadixFloat: return "RadixFloat";
	case ScaleDegreeFloat: return "ScaleDegreeFloat";
	case Float: return "Float";
	case HexInt: return "HexInt";
	case EnvVar: return "EnvVar";
	case SymbolArg: return "SymbolArg";
	case SingleLineComment: return "SingleLineComment";
	case MultiLineCommentStart: return "MultiLineCommentStart";
	case MultiLineCommentEnd: return "MultiLineCommentEnd";
	case OpeningBracket: return "OpeningBracket";
	case ClosingBracket: return "ClosingBracket";
	case Operator: return "Operator";
	default: return "?";
	}
}

/**********************************************************************/
/* character helpers (equivalent to the Qt regex classes) */

static inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool isLower(char c) { return c >= 'a' && c <= 'z'; }
static inline bool isUpper(char c) { return c >= 'A' && c <= 'Z'; }
static inline bool isAlpha(char c) { return isLower(c) || isUpper(c); }
static inline bool isHex(char c) { return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static inline bool isUpperDigit(char c) { return isDigit(c) || isUpper(c); } /* [0-9A-Z] */
static inline bool isAlnum(char c) { return isDigit(c) || isAlpha(c); }      /* [0-9a-zA-Z] */

/* `\w` */
static inline bool isWord(char c) { return isAlnum(c) || c == '_'; }

/* `\b`: there is a word boundary at `pos` ⇔ exactly one of the sides is \w. */
static inline bool isBoundary(const std::string &s, int n, int pos) {
	bool left = pos > 0 && isWord(s[(size_t)pos - 1]);
	bool right = pos < n && isWord(s[(size_t)pos]);
	return left != right;
}

/**********************************************************************/
/* the 17 rules, in the order of ScLexer::initLexicalRules() */
/**********************************************************************/

/* #0 WhiteSpace: `\s+` (Qt includes Unicode; here ASCII + C whitespace, which is
 * what shows up in a real code file; a non-ASCII space falls into Unknown). */
static inline bool isSpace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

static int matchWhiteSpace(const std::string &s, int n, int pos) {
	int i = pos;
	while (i < n && isSpace(s[(size_t)i]))
		++i;
	return i - pos;
}

/* #1 Keyword: `\b(arg|classvar|const|super|this|var)\b` Qt alternation = the first
 * alternative that allows a complete match; since only the trailing `\b` can fail,
 * trying them in order and requiring both boundaries is enough (same result as the
 * regex backtracking). */
static const char *const kKeywords[] = { "arg", "classvar", "const", "super", "this", "var" };

/* #2 Builtin: `\b(false|inf|nil|true|thisFunction|…|topEnvironment)\b` */
static const char *const kBuiltins[] = { "false",
										 "inf",
										 "nil",
										 "true",
										 "thisFunction",
										 "thisFunctionDef",
										 "thisMethod",
										 "thisProcess",
										 "thisThread",
										 "currentEnvironment",
										 "topEnvironment" };

template <size_t N> static int matchAlternation(const std::string &s, int n, int pos, const char *const (&alts)[N]) {
	if (!isBoundary(s, n, pos))
		return 0;
	for (size_t i = 0; i < N; ++i) {
		int len = (int)strlen(alts[i]);
		if (pos + len <= n && s.compare((size_t)pos, (size_t)len, alts[i]) == 0 && isBoundary(s, n, pos + len))
			return len;
	}
	return 0;
}

/* #3 RadixFloat: `\b\d+r[0-9a-zA-Z]*(\.[0-9A-Z]*)?`  (sin `\b` final) */
static int matchRadixFloat(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos))
		return 0;
	int i = pos;
	while (i < n && isDigit(s[(size_t)i]))
		++i;
	if (i == pos || i >= n || s[(size_t)i] != 'r')
		return 0;
	++i; /* la 'r' */
	while (i < n && isAlnum(s[(size_t)i]))
		++i;
	if (i < n && s[(size_t)i] == '.') {
		++i;
		while (i < n && isUpperDigit(s[(size_t)i]))
			++i; /* permite cero chars: "2r." */
	}
	return i - pos;
}

/* #4 ScaleDegreeFloat: `\b\d+(s+|b+|[sb]\d+)\b` */
static int matchScaleDegreeFloat(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos))
		return 0;
	int digitsEnd = pos;
	while (digitsEnd < n && isDigit(s[(size_t)digitsEnd]))
		++digitsEnd;
	if (digitsEnd == pos)
		return 0;
	for (int d = digitsEnd; d > pos; --d) { /* \d+ greedy, minimum 1 */
		int j = d;
		/* (s+) */
		if (j < n && s[(size_t)j] == 's') {
			int k = j + 1;
			while (k < n && s[(size_t)k] == 's')
				++k;
			for (int e = k; e > j; --e)
				if (isBoundary(s, n, e))
					return e - pos;
		}
		/* (b+) */
		if (j < n && s[(size_t)j] == 'b') {
			int k = j + 1;
			while (k < n && s[(size_t)k] == 'b')
				++k;
			for (int e = k; e > j; --e)
				if (isBoundary(s, n, e))
					return e - pos;
		}
		/* ([sb]\d+) */
		if (j < n && (s[(size_t)j] == 's' || s[(size_t)j] == 'b')) {
			int k = j + 1, dm = k;
			while (dm < n && isDigit(s[(size_t)dm]))
				++dm;
			if (dm > k)
				for (int e = dm; e > k; --e)
					if (isBoundary(s, n, e))
						return e - pos;
		}
	}
	return 0;
}

/* #5 Float: `\b((\d+(\.\d+)?([eE][-+]?\d+)?(pi)?)|pi)\b`
 *
  * The trailing `\b` forces BACKTRACKING: with "1.5x" the regex first eats "1.5"
  * and fails the `\b` ('x' follows), then it gives up the `(\.\d+)?` group and
  * then "1" does end at a boundary ⇒ Float token "1". The exploration order of
  * the regex is replicated (greedy groups, longest to shortest). */
static int matchFloat(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos))
		return 0;

	if (isDigit(s[(size_t)pos])) {
		int digitsEnd = pos;
		while (digitsEnd < n && isDigit(s[(size_t)digitsEnd]))
			++digitsEnd;

		for (int d = digitsEnd; d > pos; --d) { /* \d+ greedy */
			int j = d;
			int fracEnds[64];
			int nFrac = 0;
			if (j + 1 < n && s[(size_t)j] == '.' && isDigit(s[(size_t)j + 1])) {
				int f = j + 1;
				while (f < n && isDigit(s[(size_t)f]))
					++f;
				for (int e = f; e >= j + 2 && nFrac < 64; --e) /* (\.\d+) greedy */
					fracEnds[nFrac++] = e;
			}
			fracEnds[nFrac++] = j; /* without the group */

			for (int fi = 0; fi < nFrac; ++fi) {
				int fe = fracEnds[fi];
				int expEnds[64];
				int nExp = 0;
				if (fe < n && (s[(size_t)fe] == 'e' || s[(size_t)fe] == 'E')) {
					int m = fe + 1;
					if (m < n && (s[(size_t)m] == '+' || s[(size_t)m] == '-'))
						++m;
					int dm = m;
					while (dm < n && isDigit(s[(size_t)dm]))
						++dm;
					if (dm > m)
						for (int e = dm; e > m && nExp < 64; --e)
							expEnds[nExp++] = e;
				}
				expEnds[nExp++] = fe; /* without the group */

				for (int ei = 0; ei < nExp; ++ei) {
					int ee = expEnds[ei];
					/* (pi)? greedy: first with, then without */
					if (ee + 1 < n && s[(size_t)ee] == 'p' && s[(size_t)ee + 1] == 'i'
						&& isBoundary(s, n, ee + 2))
						return ee + 2 - pos;
					if (isBoundary(s, n, ee))
						return ee - pos;
				}
			}
		}
		return 0;
	}

	if (pos + 1 < n && s[(size_t)pos] == 'p' && s[(size_t)pos + 1] == 'i' && isBoundary(s, n, pos + 2))
		return 2;
	return 0;
}

/* #6 HexInt: `\b0x(\d|[a-f]|[A-F])+`  (sin `\b` final) */
static int matchHexInt(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos))
		return 0;
	if (pos + 2 > n || s[(size_t)pos] != '0' || pos + 1 >= n || s[(size_t)pos + 1] != 'x')
		return 0;
	int i = pos + 2;
	while (i < n && isHex(s[(size_t)i]))
		++i;
	if (i == pos + 2)
		return 0;
	return i - pos;
}

/* #7 SymbolArg: `\b[A-Za-z_]\w*:` */
static int matchSymbolArg(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos))
		return 0;
	if (!(isAlpha(s[(size_t)pos]) || s[(size_t)pos] == '_'))
		return 0;
	int i = pos + 1;
	while (i < n && isWord(s[(size_t)i]))
		++i;
	if (i < n && s[(size_t)i] == ':')
		return i + 1 - pos;
	return 0;
}

/* #8 Name: `[a-z]\w*` (no leading `\b` on purpose) */
static int matchName(const std::string &s, int n, int pos) {
	if (!isLower(s[(size_t)pos]))
		return 0;
	int i = pos + 1;
	while (i < n && isWord(s[(size_t)i]))
		++i;
	return i - pos;
}

/* #9 Class: `\b[A-Z]\w*` */
static int matchClass(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos) || !isUpper(s[(size_t)pos]))
		return 0;
	int i = pos + 1;
	while (i < n && isWord(s[(size_t)i]))
		++i;
	return i - pos;
}

/* #10 Primitive: `\b_\w+` (requires at least one \w after the '_') */
static int matchPrimitive(const std::string &s, int n, int pos) {
	if (!isBoundary(s, n, pos) || s[(size_t)pos] != '_')
		return 0;
	int i = pos + 1;
	while (i < n && isWord(s[(size_t)i]))
		++i;
	return (i > pos + 1) ? i - pos : 0;
}

/* #11 Symbol: `\\\w*` */
static int matchSymbol(const std::string &s, int n, int pos) {
	if (s[(size_t)pos] != '\\')
		return 0;
	int i = pos + 1;
	while (i < n && isWord(s[(size_t)i]))
		++i;
	return i - pos;
}

/* #12 Char: `\$\\?.` `\\?` is greedy but can backtrack: in "$\\" (end of
 * line) the regex has nothing to match the `.` with, so the `\\` is consumed
 * by the dot. */
static int matchChar(const std::string &s, int n, int pos) {
	if (s[(size_t)pos] != '$')
		return 0;
	int i = pos + 1;
	if (i + 1 < n && s[(size_t)i] == '\\')
		return i + 2 - pos; /* '$' + escape + 1 char */
	if (i < n)
		return i + 1 - pos; /* '$' + 1 char */
	return 0;
}

/* #13 EnvVar: `~\w+` */
static int matchEnvVar(const std::string &s, int n, int pos) {
	if (s[(size_t)pos] != '~')
		return 0;
	int i = pos + 1;
	while (i < n && isWord(s[(size_t)i]))
		++i;
	return (i > pos + 1) ? i - pos : 0;
}

/* #14 SingleLineComment: `//[^\r\n]*` */
static int matchSingleLineComment(const std::string &s, int n, int pos) {
	if (pos + 1 >= n || s[(size_t)pos] != '/' || s[(size_t)pos + 1] != '/')
		return 0;
	int i = pos + 2;
	while (i < n && s[(size_t)i] != '\r' && s[(size_t)i] != '\n')
		++i;
	return i - pos;
}

/* #15 MultiLineCommentStart: `/\*` */
static int matchMultiLineCommentStart(const std::string &s, int n, int pos) {
	if (pos + 1 >= n || s[(size_t)pos] != '/' || s[(size_t)pos + 1] != '*')
		return 0;
	return 2;
}

/* #16 Operator: run greedy de + - * / & | ^ % < > = */
static int matchOperator(const std::string &s, int n, int pos) {
	static const char *kOps = "+-*/&|^%<>=";
	if (!strchr(kOps, s[(size_t)pos]))
		return 0;
	int i = pos + 1;
	while (i < n && strchr(kOps, s[(size_t)i]))
		++i;
	return i - pos;
}

/**********************************************************************/
/* ScLexer                                                             */
/**********************************************************************/

ScLexer::ScLexer(const std::string &text, int offset, int state): mText(text), mOffset(offset), mState(state) {}

TokenType ScLexer::nextToken(int &lengthResult) {
	switch (mState) {
	case InCode: return nextTokenInCode(lengthResult);
	case InString: return nextTokenInString(lengthResult);
	case InSymbol: return nextTokenInSymbol(lengthResult);
	default:
		if (mState >= InComment)
			return nextTokenInComment(lengthResult);
	}
	lengthResult = 0;
	return Unknown;
}

TokenType ScLexer::nextTokenInCode(int &lengthResult) {
	static const char *openingBrackets = "({[";
	static const char *closingBrackets = ")}]";
	static const char stringMark = '"';
	static const char symbolMark = '\'';

	char currentChar = mText[(size_t)mOffset];

	TokenType type = Unknown;
	int length = 1;
	int n = (int)mText.size();

	if (currentChar == stringMark) {
		type = StringMark;
		mState = InString;
	} else if (currentChar == symbolMark) {
		type = SymbolMark;
		if (mOffset + 1 < n) /* an end of line closes the symbol */
			mState = InSymbol;
	} else if (strchr(openingBrackets, currentChar)) {
		type = OpeningBracket;
	} else if (strchr(closingBrackets, currentChar)) {
		type = ClosingBracket;
	} else {
		/* The 17 rules in order; the first one that matches at this offset wins. */
		struct Rule {
			TokenType type;
			int (*match)(const std::string &, int, int);
		};
		static const Rule rules[] = {
			{ WhiteSpace, matchWhiteSpace },
			{ Keyword, [](const std::string &s, int nn, int p) { return matchAlternation(s, nn, p, kKeywords); } },
			{ Builtin, [](const std::string &s, int nn, int p) { return matchAlternation(s, nn, p, kBuiltins); } },
			{ RadixFloat, matchRadixFloat },
			{ ScaleDegreeFloat, matchScaleDegreeFloat },
			{ Float, matchFloat },
			{ HexInt, matchHexInt },
			{ SymbolArg, matchSymbolArg },
			{ Name, matchName },
			{ Class, matchClass },
			{ Primitive, matchPrimitive },
			{ Symbol, matchSymbol },
			{ Char, matchChar },
			{ EnvVar, matchEnvVar },
			{ SingleLineComment, matchSingleLineComment },
			{ MultiLineCommentStart, matchMultiLineCommentStart },
			{ Operator, matchOperator },
		};

		for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); ++i) {
			int len = rules[i].match(mText, n, mOffset);
			if (len > 0) {
				type = rules[i].type;
				length = len;
				break;
			}
		}

		if (type == MultiLineCommentStart)
			mState = InComment;
	}

	if (length < 1)
		length = 1; /* procesar al menos 1 char (garantiza avance) */
	mOffset += length;

	lengthResult = length;
	return type;
}

TokenType ScLexer::nextTokenInString(int &length) {
	static const char stringMark = '"';
	static const char escapeChar = '\\';

	length = 0;
	TokenType type = Unknown;

	int textLength = (int)mText.size();
	while (mOffset < textLength) {
		char chr = mText[(size_t)mOffset];
		++mOffset;
		if (chr == escapeChar) {
			++mOffset;
		} else if (chr == stringMark) {
			length = 1;
			type = StringMark;
			mState = InCode;
			break;
		}
	}

	if (mOffset > textLength)
		mOffset = textLength;
	return type;
}

TokenType ScLexer::nextTokenInSymbol(int &length) {
	static const char symbolMark = '\'';
	static const char escapeChar = '\\';

	length = 0;
	TokenType type = Unknown;

	int textLength = (int)mText.size();
	while (mOffset < textLength) {
		char chr = mText[(size_t)mOffset];
		++mOffset;
		if (chr == escapeChar) {
			++mOffset;
		} else if (chr == symbolMark) {
			length = 1;
			type = SymbolMark;
			break;
		}
	}

	if (mOffset > textLength)
		mOffset = textLength;
	mState = InCode;
	return type;
}

/* Literal port of nextTokenInComment: it counts nesting and returns
 * MultiLineCommentEnd (length 2) if the comment closed on this line. */
TokenType ScLexer::nextTokenInComment(int &lengthResult) {
	int index = mOffset;
	int maxIndex = (int)mText.size() - 1;

	int commentStartIndex = -2;
	int commentEndIndex = -2;

	while (index < maxIndex) {
		if ((commentStartIndex == -2) || (commentStartIndex < index)) {
			if (commentStartIndex != -1) {
				size_t p = mText.find("/*", (size_t)index);
				commentStartIndex = (p == std::string::npos) ? -1 : (int)p;
			}
		}

		if ((commentEndIndex == -2) || (commentEndIndex < index)) {
			if (commentEndIndex != -1) {
				size_t p = mText.find("*/", (size_t)index);
				commentEndIndex = (p == std::string::npos) ? -1 : (int)p;
			}
		}

		if (commentStartIndex == -1) {
			if (commentEndIndex == -1) {
				index = maxIndex;
			} else {
				index = commentEndIndex + 2;
				--mState;
			}
		} else {
			if (commentEndIndex == -1) {
				index = commentStartIndex + 2;
				++mState;
			} else {
				if (commentStartIndex < commentEndIndex) {
					index = commentStartIndex + 2;
					++mState;
				} else {
					index = commentEndIndex + 2;
					--mState;
				}
			}
		}
		if (mState < InComment) {
			mState = InCode;
			break;
		}
	}

	if (mState == InCode) {
		mOffset = index;
		lengthResult = 2;
		return MultiLineCommentEnd;
	}

	mOffset = (int)mText.size();
	lengthResult = 0;
	return Unknown;
}

/**********************************************************************/
/* line / document (equivalent to SyntaxHighlighter::highlightBlock) */
/**********************************************************************/

LineTokens lex_line(const std::string &line, int lineStart, int lineIndex, int stateIn,
					std::vector<Token> *outTokens) {
	LineTokens lt;
	lt.start = lineStart;
	lt.length = (int)line.size();
	lt.stateIn = stateIn;
	lt.stateOut = stateIn;
	lt.isInMultilineComment = false;

	const int size = (int)line.size();
	ScLexer lexer(line, 0, stateIn);

	/* `push` replicates what the highlighter stores in
	 * TextBlockData::tokens: it drops WhiteSpace, SingleLineComment and
	 * MultiLineCommentStart. */
	auto push = [&](TokenType t, int pos, int len, char chr) {
		if (t == WhiteSpace || t == SingleLineComment || t == MultiLineCommentStart)
			return;
		if (outTokens) {
			Token tok;
			tok.type = t;
			tok.start = lineStart + pos;
			tok.length = len;
			tok.line = lineIndex;
			tok.col = pos;
			tok.character = (len == 1) ? chr : 0;
			outTokens->push_back(tok);
		}
	};

	while (lexer.offset() < size) {
		if (lexer.state() == InCode) {
			do {
				int tokenPosition = lexer.offset();
				int tokenLength = 0;
				TokenType tokenType = lexer.nextToken(tokenLength);
				push(tokenType, tokenPosition, tokenLength,
					 tokenLength == 1 ? line[(size_t)tokenPosition] : 0);
			} while (lexer.state() == InCode && lexer.offset() < size);
		} else if (lexer.state() == InString) {
			int tokenLength = 0;
			TokenType tokenType = lexer.nextToken(tokenLength);
			if (tokenType == Unknown)
				break; /* it ate the rest of the line, there is no closing quote */
			/* Only the closing quote is stored (the content does not tokenize). */
			push(StringMark, lexer.offset() - 1, 1, '"');
		} else if (lexer.state() == InSymbol) {
			int tokenLength = 0;
			TokenType tokenType = lexer.nextToken(tokenLength);
			if (tokenType == Unknown)
				break;
			push(SymbolMark, lexer.offset() - 1, 1, '\'');
		} else {
			/* state >= InComment */
			int tokenLength = 0;
			lexer.nextToken(tokenLength);
			lt.isInMultilineComment = true;
		}
	}

	lt.stateOut = lexer.state();
	return lt;
}

int DocumentTokens::line_of_offset(int offset) const {
	if (lines.empty())
		return 0;
	int lo = 0, hi = (int)lines.size() - 1, best = 0;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (lines[(size_t)mid].start <= offset) {
			best = mid;
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	return best;
}

DocumentTokens lex_document(const std::string &text) {
	DocumentTokens doc;
	int state = InCode;
	int lineIndex = 0;
	size_t pos = 0;

	while (true) {
		size_t nl = text.find('\n', pos);
		size_t end = (nl == std::string::npos) ? text.size() : nl;

		std::string line = text.substr(pos, end - pos);
		size_t tokenBegin = doc.tokens.size();
		LineTokens lt = lex_line(line, (int)pos, lineIndex, state, &doc.tokens);
		lt.tokenBegin = (int)tokenBegin;
		lt.tokenEnd = (int)doc.tokens.size();
		doc.lines.push_back(lt);

		state = lt.stateOut;
		++lineIndex;

		if (nl == std::string::npos)
			break;
		pos = nl + 1;
		if (pos == text.size()) {
			/* The document ends in '\n': QTextDocument has a final empty line ⇒
			 * it has to be lexed (it keeps the state, no tokens). */
			size_t tb = doc.tokens.size();
			LineTokens last = lex_line(std::string(), (int)pos, lineIndex, state, &doc.tokens);
			last.tokenBegin = (int)tb;
			last.tokenEnd = (int)doc.tokens.size();
			doc.lines.push_back(last);
			break;
		}
	}

	return doc;
}

} // namespace sc_code
