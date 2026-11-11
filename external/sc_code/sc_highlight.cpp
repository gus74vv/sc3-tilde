// SPDX-License-Identifier: GPL-3.0-only
/* sc_highlight.cpp — incremental per-line token cache (headless).
 *
 * See sc_highlight.h for the conventions, the provenance of the color and the
 * license. The table that pins it down is in `tests/highlight_test.cpp`.
 */

#include "sc_highlight.h"

#include "sc_buffer.h"
#include "sc_theme.h"

namespace sc_code {

Color highlight_color(TokenType t) {
	/* The table is the FACTORY palette (One Dark, `@theme onedark`): the numbers live in
	 * sc_theme.cpp, so this function cannot drift from the object's defaults (0.9.13,
	 * The object paints with its own attributes; this is the headless
	 * reference of the default theme. */
	const Theme &th = *theme_default();
	const RGBA *c = &th.text;
	switch (t) {
	case Keyword:
		c = &th.keyword;
		break;
	case Builtin:
		c = &th.builtin;
		break;
	case Primitive:
		c = &th.primitive;
		break;
	case Class:
		c = &th.klass;
		break;
	case Symbol:
	case SymbolMark:
	case SymbolArg:
		c = &th.symbol;
		break;
	case StringMark:
		c = &th.string;
		break;
	case Char:
		c = &th.charlit;
		break;
	case RadixFloat:
	case ScaleDegreeFloat:
	case Float:
	case HexInt:
		c = &th.number;
		break;
	case EnvVar:
		c = &th.envvar;
		break;
	case SingleLineComment:
	case MultiLineCommentStart:
	case MultiLineCommentEnd:
		c = &th.comment;
		break;
	case OpeningBracket:
	case ClosingBracket:
		c = &th.bracket;
		break;
	case Operator:
		c = &th.op;
		break;
	default:
		break; /* WhiteSpace/Unknown/Name: the text color */
	}
	return {c->r, c->g, c->b};
}

namespace {

/* Lexes the COMPLETE line (with spaces and comments) replicating which range the
 * IDE highlighter paints. `sawComment` = TextBlockData::isInMultilineComment: true
 * if at some point it lexed inside a comment, even one opened and closed on this
 * same line (same as the IDE). Returns the outgoing state. */
int tokenize_line(const std::string &line, int lineIndex, int lineStart, int stateIn, std::vector<Token> *out,
				  bool &sawComment) {
	out->clear();
	sawComment = false;

	const int size = (int)line.size();
	ScLexer lexer(line, 0, stateIn);

	auto push = [&](TokenType t, int pos, int len) {
		if (len <= 0)
			return;
		Token tok;
		tok.type = t;
		tok.start = lineStart + pos;
		tok.length = len;
		tok.line = lineIndex;
		tok.col = pos;
		tok.character = (len == 1) ? line[(size_t)pos] : 0;
		out->push_back(tok);
	};

	while (lexer.offset() < size) {
		const int pos = lexer.offset();
		int len = 0;

		if (lexer.state() == InCode) {
			const TokenType t = lexer.nextToken(len);
			/* The lexer guarantees progress (Unknown of length 1); if it did not
			 * advance, we cut here so we do not hang on a pathological line. */
			if (lexer.offset() <= pos)
				break;
			push(t, pos, len);
		} else if (lexer.state() == InString) {
			const TokenType t = lexer.nextToken(len);
			const int end = (t == StringMark) ? lexer.offset() - 1 : lexer.offset();
			push(StringMark, pos, end - pos); /* the body is painted as a string */
			if (t == StringMark)
				push(StringMark, end, 1);
		} else if (lexer.state() == InSymbol) {
			const TokenType t = lexer.nextToken(len);
			const int end = (t == SymbolMark) ? lexer.offset() - 1 : lexer.offset();
			push(SymbolMark, pos, end - pos);
			if (t == SymbolMark)
				push(SymbolMark, end, 1);
		} else {
			/* state >= InComment: the IDE paints the WHOLE range with
			 * CommentFormat. */
			lexer.nextToken(len);
			const int end = lexer.offset();
			sawComment = true;
			push(MultiLineCommentStart, pos, end - pos);
		}
	}

	return lexer.state();
}

} // namespace

void Highlight::reset() {
	mLines.clear();
	mDirtyFrom = 0;
}

Highlight::Line &Highlight::row(int line) {
	if ((int)mLines.size() <= line)
		mLines.resize((size_t)line + 1);
	return mLines[(size_t)line];
}

void Highlight::invalidate_from(int line) {
	/* O(1): remembering the first unreliable line is enough. The ones above still
	 * hold (no edit can touch text before the caret) and the ones below are relexed
	 * when requested, in `ensure`. */
	if (line < 0)
		line = 0;
	if (line < mDirtyFrom)
		mDirtyFrom = line;
}

const std::vector<Token> &Highlight::ensure(const Buffer &buf, int line) {
	if (line < 0)
		line = 0;
	const int count = buf.line_count();
	if (line >= count)
		line = count - 1;
	row(line); /* ensures the cache size */

	const int dirty0 = mDirtyFrom; /* everything from here down is not trusted */
	int from = dirty0 < line ? dirty0 : line;
	if (from < 0)
		from = 0;
	if (from > 0 && !mLines[(size_t)(from - 1)].valid)
		from = 0; /* lifesaver: this should not happen (everything above the dirty one is valid) */

	int state = from == 0 ? InCode : mLines[(size_t)(from - 1)].stateOut;

	for (int i = from; i <= line; ++i) {
		Line &L = mLines[(size_t)i];
		if (i < dirty0 && L.valid && L.stateIn == state) { /* intacta y consistente */
			state = L.stateOut;
			continue;
		}
		L.valid = false;
		L.stateIn = state;
		L.stateOut = tokenize_line(buf.line_text(i), i, buf.line_start(i), state, &L.tokens, L.inComment);
		L.valid = true;
		++mLexed;
		state = L.stateOut;
	}

	/* What was verified holds: the mark can only MOVE FORWARD (if the requested line
	 * is above the first dirty one, it is inspected upwards without relexing and the
	 * dirty ones below stay dirty). */
	if (line + 1 > mDirtyFrom)
		mDirtyFrom = line + 1;
	return mLines[(size_t)line].tokens;
}

bool Highlight::line_is_valid(int line) const {
	if (line < 0 || line >= (int)mLines.size())
		return false;
	return mLines[(size_t)line].valid;
}

int Highlight::state_in(int line) const {
	if (line < 0 || line >= (int)mLines.size())
		return InCode;
	return mLines[(size_t)line].stateIn;
}

int Highlight::state_out(int line) const {
	if (line < 0 || line >= (int)mLines.size())
		return InCode;
	return mLines[(size_t)line].stateOut;
}

bool Highlight::in_multiline_comment(int line) const {
	if (line < 0 || line >= (int)mLines.size())
		return false;
	return mLines[(size_t)line].inComment;
}

} // namespace sc_code
