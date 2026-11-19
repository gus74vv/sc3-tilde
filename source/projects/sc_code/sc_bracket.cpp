// SPDX-License-Identifier: GPL-3.0-only
/* sc_bracket.cpp — bracket matching for the caret (headless).
 *
 * See sc_bracket.h for the rules, the cache and the license. The table that pins
 * them is in `tests/bracket_test.cpp`.
 */

#include "sc_bracket.h"

#include <algorithm>

#include "sc_region.h"

namespace sc_code {

bool is_open_bracket(char c) { return c == '(' || c == '[' || c == '{'; }
bool is_close_bracket(char c) { return c == ')' || c == ']' || c == '}'; }

/* Index of the token that STARTS exactly at `offset` (-1 if none). The
 * `lex_document` tokens are in document order and do not overlap ⇒ bisection. */
int BracketMatcher::token_starting_at(int offset) const {
	size_t lo = 0, hi = mDoc.tokens.size();
	while (lo < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		if (mDoc.tokens[mid].start < offset)
			lo = mid + 1;
		else
			hi = mid;
	}
	if (lo < mDoc.tokens.size() && mDoc.tokens[lo].start == offset)
		return (int)lo;
	return -1;
}

BracketMatch BracketMatcher::at(const std::string &text, int caret, int revision) {
	/* The document changed: everything cached stops being
	 * valid. */
	if (revision != mRevision) {
		mRevision = revision;
		mHaveDoc = false;
		mCaret = -1;
		mHasResult = false;
		mResult = BracketMatch();
	}

	const int len = (int)text.size();
	if (caret < 0)
		caret = 0;
	if (caret > len)
		caret = len;

	/* Candidate: an opener AT the caret, or a closer that ENDS at the
	 * caret. */
	int cand = -1;
	bool wantOpen = false;
	if (caret < len && is_open_bracket(text[(size_t)caret])) {
		cand = caret;
		wantOpen = true;
	} else if (caret > 0 && is_close_bracket(text[(size_t)caret - 1])) {
		cand = caret - 1;
		wantOpen = false;
	}

	if (cand < 0) { /* 99% of the paints: nothing needs to be lexed */
		mCaret = caret;
		mHasResult = true;
		mResult = BracketMatch();
		return mResult;
	}

	if (mHasResult && mCaret == caret)
		return mResult;

	if (!mHaveDoc) {
		mDoc = lex_document(text);
		mHaveDoc = true;
		++mLexed;
	}
	mCaret = caret;

	const int idx = token_starting_at(cand);
	if (idx < 0 || (wantOpen ? mDoc.tokens[(size_t)idx].type != OpeningBracket
							: mDoc.tokens[(size_t)idx].type != ClosingBracket)) {
		/* The character is inside a string/comment (the lexer does not report it
		 * as a bracket) or it is not what it looks like: nothing is highlighted. */
		mHasResult = true;
		mResult = BracketMatch();
		return mResult;
	}

	BracketPair pair;
	match_bracket(mDoc, idx, pair);
	if (!pair.valid()) { /* no partner: unclosed `(`, etc. */
		mHasResult = true;
		mResult = BracketMatch();
		return mResult;
	}

	mResult.first = mDoc.tokens[(size_t)pair.first].start;
	mResult.second = mDoc.tokens[(size_t)pair.second].start;
	mHasResult = true;
	return mResult;
}

} // namespace sc_code
