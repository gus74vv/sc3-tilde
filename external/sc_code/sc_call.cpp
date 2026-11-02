// SPDX-License-Identifier: GPL-3.0-only
/* sc_call.cpp — see sc_call.h (S6c, SCODE.md §10.2).
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * One single forward pass from the start of the document (there is no way to look
 * backwards without getting `(` and commas inside strings and comments wrong, and a
 * patch document does not reach 1 MB => the cost is negligible: ~0.1 ms per 100 KB).
 * Same criterion as `sc_region` with the lexer, but with a minimal automaton because
 * only brackets, commas and literals matter here.
 */

#include "sc_call.h"

#include <vector>

namespace sc_code {

namespace {

enum LexState { StateCode = 0, StateString, StateSymbol, StateLineComment, StateBlockComment };

inline bool is_ident_char(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

inline bool is_blank(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

inline bool is_open(char c) { return c == '(' || c == '[' || c == '{'; }
inline bool is_close(char c) { return c == ')' || c == ']' || c == '}'; }

inline char match_of(char c) {
	switch (c) {
	case ')':
		return '(';
	case ']':
		return '[';
	default:
		return '{';
	}
}

struct Open {
	char kind; /* '(' '[' '{' */
	int offset;
	int commas; /* level 0 commas seen (only used for '(') */
};

} // namespace

CallContext call_context_at(const std::string &text, int caret) {
	CallContext out;
	const int len = (int)text.size();
	if (caret < 0)
		caret = 0;
	if (caret > len)
		caret = len;
	if (caret == 0)
		return out;

	std::vector<Open> stack;
	stack.reserve(16);
	int state = StateCode;
	int blockDepth = 0;

	for (int i = 0; i < caret; ++i) {
		const char c = text[(size_t)i];
		switch (state) {
		case StateString:
			if (c == '\\')
				++i; /* escape: skip the next byte */
			else if (c == '"')
				state = StateCode;
			continue;
		case StateSymbol:
			if (c == '\\')
				++i;
			else if (c == '\'' || c == '\n') /* the lexer closes a symbol at end of line */
				state = StateCode;
			continue;
		case StateLineComment:
			if (c == '\n')
				state = StateCode;
			continue;
		case StateBlockComment:
			if (c == '/' && i + 1 < caret && text[(size_t)i + 1] == '*') {
				++blockDepth;
				++i;
			} else if (c == '*' && i + 1 < caret && text[(size_t)i + 1] == '/') {
				if (--blockDepth <= 0) {
					blockDepth = 0;
					state = StateCode;
				}
				++i;
			}
			continue;
		default:
			break;
		}

		/* --- Code state ---------------------------------------------------- */
		if (c == '"') {
			state = StateString;
			continue;
		}
		if (c == '\'') {
			state = StateSymbol;
			continue;
		}
		if (c == '$') {
			/* `$x` or `$\n`: the character literal is one code point. */
			if (i + 1 < caret && text[(size_t)i + 1] == '\\')
				++i;
			if (i + 1 < caret)
				++i;
			continue;
		}
		if (c == '/' && i + 1 < caret && text[(size_t)i + 1] == '/') {
			state = StateLineComment;
			++i;
			continue;
		}
		if (c == '/' && i + 1 < caret && text[(size_t)i + 1] == '*') {
			state = StateBlockComment;
			blockDepth = 1;
			++i;
			continue;
		}
		if (is_open(c)) {
			Open o;
			o.kind = c;
			o.offset = i;
			o.commas = 0;
			stack.push_back(o);
			continue;
		}
		if (is_close(c)) {
			const char want = match_of(c);
			/* A close with no opener (half-written document) is ignored. */
			for (int k = (int)stack.size() - 1; k >= 0; --k) {
				if (stack[(size_t)k].kind == want) {
					stack.resize((size_t)k);
					break;
				}
			}
			continue;
		}
		if (c == ',' && !stack.empty() && stack.back().kind == '(')
			stack.back().commas += 1;
	}

	/* The call's parenthesis: the INNERMOST open '('. */
	int p = -1;
	for (int k = (int)stack.size() - 1; k >= 0; --k) {
		if (stack[(size_t)k].kind == '(') {
			p = stack[(size_t)k].offset;
			out.arg = stack[(size_t)k].commas;
			break;
		}
	}
	if (p < 0)
		return out;

	/* Receiver and method: `Class . method` to the left of the '('. */
	int i = p - 1;
	while (i >= 0 && is_blank(text[(size_t)i]))
		--i;
	const int mEnd = i + 1;
	while (i >= 0 && is_ident_char(text[(size_t)i]))
		--i;
	const int mStart = i + 1;
	if (mStart >= mEnd)
		return out;
	while (i >= 0 && is_blank(text[(size_t)i]))
		--i;
	if (i < 0 || text[(size_t)i] != '.')
		return out;
	--i;
	while (i >= 0 && is_blank(text[(size_t)i]))
		--i;
	const int oEnd = i + 1;
	while (i >= 0 && is_ident_char(text[(size_t)i]))
		--i;
	const int oStart = i + 1;
	if (oStart >= oEnd)
		return out;
	const char first = text[(size_t)oStart];
	if (!(first >= 'A' && first <= 'Z'))
		return out; /* only `Class.method(`: a variable cannot be resolved */

	out.valid = true;
	out.owner = text.substr((size_t)oStart, (size_t)(oEnd - oStart));
	out.method = text.substr((size_t)mStart, (size_t)(mEnd - mStart));
	out.open_paren = p;
	return out;
}

} // namespace sc_code
