// SPDX-License-Identifier: GPL-3.0-only
/* sc_indent.cpp — port de ScCodeEditor::indent / indentationLevel (sc_editor.cpp L597/L709).
 *
 * See sc_indent.h for the two non-obvious rules and the
 * provenance/license.
 *
 * CAREFUL — the IDE has TWO different accountings and they do not agree with each
 * other: - `indent()` (L597) uses an ACCUMULATIVE `global_level` and a stack of
 * marks (`global_level += level; stack.push(global_level)`), and pops when
 * `global_level < stack.top()`. That is the one that runs when Return is pressed
 * and it is the one that reproduces the indentation seen in the IDE: a block with
 * TWO openers (`Ndef(\x, {`) needs TWO closers to pop the level. -
 * `indentationLevel()` (L709) pushes the block `level` as is, so that case ends up
 * "one level too high" there. This port uses the ACCUMULATIVE one (the `indent()`
 * one), which is the one that gives visual parity (see the snippet table in
 * `tests/indent_test.cpp`, with the `}` case).
 *
 * Other deliberate differences: - `indent_level_at(line, col)` stops at a POINT
 * instead of the block start, because what we need is "which indentation belongs
 * to the line I am about to create with Return": that depends on the tokens of
 * the current line that remain to the LEFT of the caret (the ones on the right
 * move to the new line). - The IDE relies on QTextBlockUserData already cached by
 * the highlighter; here we lex the document with `lex_document()` (S0), which
 * gives the SAME tokens: WhiteSpace, comments and string contents are not stored,
 * so the parentheses inside them do not count (same as in the IDE). - The two
 * special cases of `indent()` (L657-668) for strings and multi-line comments ARE
 * ported: the IDE does not reindent a block if some part of it
 * was lexed inside a comment (`isInMultilineComment`) or if the block is exactly
 * the start of one; and if the block STARTS inside a string it indents it at
 * level 0. For the NEW Return line the same idea holds looking at the state AT
 * THE CARET: inside a comment or a string ⇒ level 0.
 */

#include "sc_indent.h"

namespace sc_code {

namespace {

/* `QStack<int> stack` + `int global_level` + `int level` de ScCodeEditor::indent. */
struct LevelState {
	std::vector<int> stack;
	int globalLevel;
	int level;

	LevelState(): globalLevel(0), level(0) {}

	void feed(const Token &t) {
		if (t.type == OpeningBracket) {
			/* Rule 1 (the non-obvious one): `(` in column 0 with an EMPTY stack
			 * does not add. */
			if (t.character != '(' || !stack.empty() || t.col != 0)
				++level;
		} else if (t.type == ClosingBracket) {
			if (level)
				--level;
			else if (globalLevel) {
				--globalLevel;
				if (!stack.empty() && globalLevel < stack.back())
					stack.pop_back();
			}
		}
	}

	void feed_line(const LineTokens &lt, const std::vector<Token> &tokens) {
		for (int i = lt.tokenBegin; i < lt.tokenEnd; ++i)
			feed(tokens[(size_t)i]);
	}

	/* End of block: the pending level is pushed as ONE accumulated mark. */
	void end_of_block() {
		if (level) {
			globalLevel += level;
			stack.push_back(globalLevel);
			level = 0;
		}
	}
};

} // namespace

int indent_level_for_line(const DocumentTokens &doc, int line) {
	LevelState st;
	for (int b = 0; b < (int)doc.lines.size(); ++b) {
		const LineTokens &lt = doc.lines[(size_t)b];

		if (b == line) {
			/* IDE `block_start_in_string`: if the block starts inside a string,
			 * it is indented at level 0 (that is, the indentation is removed). */
			if (lt.stateIn == InString)
				return 0;
			/* IDE `indent()`: a block that STARTS with a closer is indented to the
			 * stack level AFTER processing it (so the `}` goes back to the column of
			 * its opener); the rest are indented at the level they start with. */
			const bool startsWithCloser =
				lt.tokenBegin < lt.tokenEnd && doc.tokens[(size_t)lt.tokenBegin].type == ClosingBracket;
			const int before = (int)st.stack.size();
			st.feed_line(lt, doc.tokens);
			st.end_of_block();
			return startsWithCloser ? (int)st.stack.size() : before;
		}

		st.feed_line(lt, doc.tokens);
		st.end_of_block();
	}
	return 0;
}

int indent_level_at(const DocumentTokens &doc, int line, int col) {
	LevelState st;
	for (int b = 0; b <= line && b < (int)doc.lines.size(); ++b) {
		const LineTokens &lt = doc.lines[(size_t)b];
		for (int i = lt.tokenBegin; i < lt.tokenEnd; ++i) {
			const Token &t = doc.tokens[(size_t)i];
			if (b == line && t.col >= col)
				break; /* the tokens to the right of the caret move to the new line */
			st.feed(t);
		}
		st.end_of_block(); /* the cut IS the end of block for the new line */
	}
	return (int)st.stack.size();
}

int indent_level_for_line_at_offset(const std::string &text, int offset) {
	if (offset < 0)
		offset = 0;
	if (offset > (int)text.size())
		offset = (int)text.size();

	const DocumentTokens doc = lex_document(text);
	return indent_level_for_line(doc, doc.line_of_offset(offset));
}

namespace {

/* Lexical state AT THE END of `prefix` (the part of the line left of the
 * caret), starting at `stateIn`. It is the "in_comment"/"in_string" the IDE
 * carries into the block that is about to be created by Return. */
int lex_state_at(const std::string &prefix, int stateIn) {
	if (prefix.empty())
		return stateIn;
	ScLexer lexer(prefix, 0, stateIn);
	const int size = (int)prefix.size();
	while (lexer.offset() < size) {
		const int pos = lexer.offset();
		int len = 0;
		if (lexer.state() == InCode) {
			lexer.nextToken(len);
			if (lexer.offset() <= pos)
				break;
		} else {
			lexer.nextToken(len); /* InString/InSymbol/comment: consumes block by block */
		}
	}
	return lexer.state();
}

} // namespace

int indent_level_at_offset(const std::string &text, int offset) {
	if (offset < 0)
		offset = 0;
	if (offset > (int)text.size())
		offset = (int)text.size();

	const DocumentTokens doc = lex_document(text);
	const int line = doc.line_of_offset(offset);
	const LineTokens &lt = doc.lines[(size_t)line];
	const int col = offset - lt.start;

	/* IDE rule (L657-668): if the caret ended up inside a comment or a string, the
	 * new line is not indented. */
	const int state = lex_state_at(text.substr((size_t)lt.start, (size_t)col), lt.stateIn);
	if (state >= InComment || state == InString)
		return 0;

	return indent_level_at(doc, line, col);
}

LineIndent indent_for_line_at_offset(const std::string &text, int offset) {
	if (offset < 0)
		offset = 0;
	if (offset > (int)text.size())
		offset = (int)text.size();

	const DocumentTokens doc = lex_document(text);
	const int line = doc.line_of_offset(offset);
	const LineTokens &lt = doc.lines[(size_t)line];

	LineIndent r;
	r.level = indent_level_for_line(doc, line);
	r.apply = !lt.isInMultilineComment;
	if (r.apply && text.compare((size_t)lt.start, (size_t)lt.length, "/*") == 0)
		r.apply = false; /* the lexer does not see the start of a comment as such when deciding */
	return r;
}

std::string indent_string(int level, int indentWidth) {
	if (level <= 0 || indentWidth <= 0)
		return std::string();
	return std::string((size_t)(level * indentWidth), ' ');
}

} // namespace sc_code
