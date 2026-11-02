// SPDX-License-Identifier: GPL-3.0-only
/* sc_complete.h — candidate engine for the completion session of `sc_code` (S6c+,
 * SCODE.md §10.6): `text + caret -> one candidate`, with no Max and no paint.
 *
 * There is no popup (the IDE uses a QListView over sclang introspection): the bar is
 * ONE line, so the alternatives are navigated by cycling and what is shown is always
 * the candidate that ⏎ would insert. The engine only knows about names; the object
 * decides when a session exists and what a key does.
 *
 * Sources, in the order the plan fixed:
 *   1. the words of the DOCUMENT itself (`sc_lexer` identifiers), with frequency:
 *      what is already written is what is most likely wanted next;
 *   2. the sclang keywords/builtins (the first two rules of `sc_lexer.cpp`): they are
 *      the language, not the class library, so they work with no index at all;
 *   3. `~envVars` (only if the token starts with `~`) and `\symbols` (only with `\`),
 *      also from the document;
 *   4. `set_external()`: names that come from OUTSIDE (S6c+: the classes and method
 *      names of the class library index, §10.1). That is the only contact point: the
 *      module does not know where they came from.
 *
 * Invariants (pinned in `tests/complete_test.cpp`):
 *   - The completed token is the run of `[A-Za-z0-9_]` that ends at the caret, plus
 *     the `~`/`\` sigil in front of it (the sigil is part of what gets replaced:
 *     `~fr` completes to `~freq`, not to `freq`).
 *   - The word already written is never suggested (`word == prefix` is filtered): a
 *     ghost of the same text is not a suggestion, it is noise.
 *   - Deterministic ranking: frequency desc, then shorter, then alphabetical. No
 *     randomness, so the test can pin the table.
 *   - `suggest()` is pure (it mutates nothing): cycling is asking for the next index,
 *     the module remembers nothing.
 *   - `min_prefix` = 0 means "the user asked for it" (⌃Space): an EMPTY prefix is valid
 *     and lists everything from the first alternative. With `min_prefix` > 0 an empty
 *     or too short token is not a suggestion.
 *   - The document scan is cached by `revision` (the Buffer counter): typing rebuilds
 *     once, moving the caret rebuilds nothing.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <string>
#include <vector>

namespace sc_code {

/* One concrete suggestion: replace [start, end) with `word`. `end` is always the
 * caret; the text to the right of the caret is never touched. */
struct Completion {
	bool valid;
	int start;        /* first byte of the token (includes `~`/`\` when there is one) */
	int end;          /* end of the token == caret */
	std::string word; /* the whole candidate */
	int index;        /* position of this one among the alternatives (0 = the best) */
	int alternatives; /* how many alternatives this prefix has */

	Completion() : valid(false), start(0), end(0), index(0), alternatives(0) {}
};

class Completer {
  public:
	Completer();

	/* Re-reads the document if it changed (`revision` != the last one seen). Cheap to
	 * call on every key: if the revision did not change it does nothing. */
	void sync(const std::string &text, int revision);

	/* Names that come from outside (S6c+: classes + methods of the sclang index).
	 * They are added to the document ones; the ranking treats them as frequency 0.
	 * `exclusive` = use ONLY these (the `Class.` case of §10.6: after a dot the
	 * candidates are the methods of the receiver, and the words of the document are
	 * noise there). */
	void set_external(std::vector<std::string> names, bool exclusive = false);
	void clear_external();

	/* `cycle` skips alternatives (⌃Space: 0, 1, 2… and wraps). `min_prefix` is the
	 * minimum number of bytes of the token: 2 for an automatic suggestion (do not
	 * bother on the first letter) and 1 (or 0, see above) when the user asks. */
	bool suggest(const std::string &text, int caret, int cycle, int min_prefix, Completion &out) const;

	/* Membership in the sclang "word token": [A-Za-z0-9_] (the same ASCII table as
	 * `Buffer::is_word_byte`; repeated here so the module does not depend on the
	 * buffer — it only sees `std::string`). */
	static bool is_word_byte(unsigned char c);

	/* Test instrumentation (not used by the object). */
	int rebuilds() const { return mRebuilds; }
	int name_count() const { return (int)mNames.size(); }

  private:
	struct Entry {
		std::string word;
		int freq;
	};

	/* Candidates sorted by `word` (prefix search with lower_bound). */
	std::vector<Entry> mNames;   /* identifiers: document + language + externals */
	std::vector<Entry> mEnvvars; /* `~name` */
	std::vector<Entry> mSymbols; /* `\name` */
	std::vector<std::string> mExternal;
	std::vector<Entry> mExternalPool; /* mExternal sorted + unique (exclusive mode) */
	bool mExclusive = false;
	int mRevision;
	int mRebuilds;
};

} // namespace sc_code
