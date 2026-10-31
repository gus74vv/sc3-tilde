// SPDX-License-Identifier: GPL-3.0-only
/* sc_bracket.h — bracket matching for the caret (`sc_code`, S5c).
 *
 * What an editor does: if the caret is next to a bracket, its PARTNER is also
 * highlighted. The rules of this port (pinned in `tests/bracket_test.cpp`):
 *
 * - Candidate = the bracket the caret has **immediately ahead** and is an opener (
 * `(`, `[`, `{` ), or the one **immediately behind** and is a closer ( `)`, `]`, `}`
 * ). Any other position ⇒ nothing is highlighted. - Matching is `sc_region`'s
 * `match_bracket` (port of the IDE `ScCodeEditor::matchBracket`): it counts ONLY
 * brackets the lexer reports as bracket tokens, so whatever is inside a string, a
 * symbol or a comment (even nested) does not take part. - If the bracket has no
 * partner (say an unclosed `(`) nothing is highlighted.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * Why there is a cache: matching needs the tokens of the WHOLE document
 * (`lex_document`, ~6 ms for 5000 lines). Paint runs on every key, so: - if the
 * caret is not next to a bracket nothing is lexed (a 2-byte check), - the result
 * is memoized per caret, - the lexed document is reused until
 * `Buffer::revision()` changes. With that, moving through the text does not
 * relex and typing only pays the relex when the caret ends up next to a bracket.
 */

#pragma once

#include <string>

#include "sc_lexer.h"

namespace sc_code {

/* Byte offsets of the two brackets to highlight (-1 = nothing to highlight). */
struct BracketMatch {
	int first;
	int second;

	BracketMatch(): first(-1), second(-1) {}
	bool valid() const { return first >= 0 && second >= 0; }
};

bool is_open_bracket(char c);
bool is_close_bracket(char c);

class BracketMatcher {
  public:
	/* Matching for `caret`. `revision` = Buffer::revision() (invalidates the
	 * cache). */
	BracketMatch at(const std::string &text, int caret, int revision);

	/* Instrumentation for the test (not used by the object). */
	int lexed_documents() const { return mLexed; }

  private:
	int token_starting_at(int offset) const;

	DocumentTokens mDoc;
	bool mHaveDoc = false;
	int mRevision = -1; /* -1 = we have not seen any document yet */
	int mCaret = -1;
	BracketMatch mResult;
	bool mHasResult = false;
	int mLexed = 0;
};

} // namespace sc_code
