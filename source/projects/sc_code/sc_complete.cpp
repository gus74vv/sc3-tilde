// SPDX-License-Identifier: GPL-3.0-only
/* sc_complete.cpp — see sc_complete.h (S6c+, the design notes).
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "sc_complete.h"

#include <algorithm>
#include <map>

namespace sc_code {

/* sclang keywords and builtins: rules 1 and 2 of the lexer (`sc_lexer.cpp`, copied
 * from the design notes). They are not "the class library": they are the language, so
 * they hold even with no engine and no index. */
static const char *const kLanguageNames[] = {
	"arg",              "classvar",            "const",      "super",     "this",
	"var",              "false",               "inf",        "nil",       "true",
	"thisFunction",     "thisFunctionDef",     "thisMethod", "thisProcess", "thisThread",
	"currentEnvironment", "topEnvironment",
};

bool Completer::is_word_byte(unsigned char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

Completer::Completer() : mRevision(-1), mRebuilds(0) {}

void Completer::set_external(std::vector<std::string> names, bool exclusive) {
	mExternal.swap(names);
	mExclusive = exclusive;
	mExternalPool.clear();
	if (exclusive) {
		/* Sorted and unique: this vector IS the pool, so the lower_bound of `suggest`
		 * needs it ordered. */
		std::vector<std::string> sorted = mExternal;
		std::sort(sorted.begin(), sorted.end());
		sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
		mExternalPool.reserve(sorted.size());
		for (size_t i = 0; i < sorted.size(); ++i)
			if (!sorted[i].empty())
				mExternalPool.push_back(Entry{sorted[i], 0});
	}
	mRevision = -1; /* forces the next sync to rebuild the pool */
}

void Completer::clear_external() { set_external(std::vector<std::string>()); }

void Completer::sync(const std::string &text, int revision) {
	if (revision == mRevision)
		return;
	mRevision = revision;
	++mRebuilds;

	/* Ordered map by name: the pool has to end up sorted for the lower_bound of
	 * `suggest`, and this way there is nothing to sort afterwards. */
	std::map<std::string, int> names, envvars, symbols;

	for (size_t i = 0; i < sizeof(kLanguageNames) / sizeof(kLanguageNames[0]); ++i)
		names[kLanguageNames[i]]; /* frequency 0: any word of the document wins */
	for (size_t i = 0; i < mExternal.size(); ++i)
		if (!mExternal[i].empty())
			names[mExternal[i]];

	const int n = (int)text.size();
	int i = 0;
	while (i < n) {
		const unsigned char c = (unsigned char)text[(size_t)i];
		if (c == '~' || c == '\\') {
			int j = i + 1;
			while (j < n && is_word_byte((unsigned char)text[(size_t)j]))
				j++;
			if (j > i + 1) {
				const std::string w = text.substr((size_t)i, (size_t)(j - i));
				if (c == '~')
					envvars[w] += 1;
				else
					symbols[w] += 1;
				i = j;
			} else {
				i += 1; /* a lone `~` or `\` */
			}
			continue;
		}
		if (is_word_byte(c)) {
			int j = i;
			while (j < n && is_word_byte((unsigned char)text[(size_t)j]))
				j++;
			/* Numbers are not candidates (nor are tokens starting with a digit: in
			 * sclang an identifier never starts with one). */
			if (!(c >= '0' && c <= '9'))
				names[text.substr((size_t)i, (size_t)(j - i))] += 1;
			i = j;
			continue;
		}
		i += 1;
	}

	mNames.clear();
	mEnvvars.clear();
	mSymbols.clear();
	mNames.reserve(names.size());
	for (std::map<std::string, int>::const_iterator it = names.begin(); it != names.end(); ++it)
		mNames.push_back(Entry{it->first, it->second});
	mEnvvars.reserve(envvars.size());
	for (std::map<std::string, int>::const_iterator it = envvars.begin(); it != envvars.end(); ++it)
		mEnvvars.push_back(Entry{it->first, it->second});
	mSymbols.reserve(symbols.size());
	for (std::map<std::string, int>::const_iterator it = symbols.begin(); it != symbols.end(); ++it)
		mSymbols.push_back(Entry{it->first, it->second});
}

static bool starts_with(const std::string &s, const std::string &prefix) {
	return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool Completer::suggest(const std::string &text, int caret, int cycle, int min_prefix, Completion &out) const {
	out = Completion();

	const int len = (int)text.size();
	if (caret < 0)
		caret = 0;
	if (caret > len)
		caret = len;

	/* Token under the caret: the run of [A-Za-z0-9_] that ends there, plus the sigil
	 * (`~`/`\`) in front of it, which is also replaced. */
	int start = caret;
	while (start > 0 && is_word_byte((unsigned char)text[(size_t)start - 1]))
		start--;
	unsigned char sigil = 0;
	if (start > 0) {
		const unsigned char c = (unsigned char)text[(size_t)start - 1];
		if (c == '~' || c == '\\') {
			sigil = c;
			start--;
		}
	}

	const std::string prefix = text.substr((size_t)start, (size_t)(caret - start));
	if (min_prefix > 0 && (int)prefix.size() < min_prefix)
		return false;

	const std::vector<Entry> &pool = sigil == '~' ? mEnvvars
									: sigil == '\\' ? mSymbols
													  : (mExclusive ? mExternalPool : mNames);
	if (pool.empty())
		return false;

	/* Range [lower_bound(prefix), …) that starts with the prefix. */
	const std::string probe = prefix;
	size_t lo = 0, hi = pool.size();
	while (lo < hi) {
		const size_t mid = (lo + hi) / 2;
		if (pool[mid].word < probe)
			lo = mid + 1;
		else
			hi = mid;
	}

	std::vector<const Entry *> matches;
	const size_t kMaxMatches = 256; /* cap: the ranking does not depend on the whole pool */
	for (size_t k = lo; k < pool.size(); ++k) {
		if (!starts_with(pool[k].word, prefix))
			break;
		if (pool[k].word.size() == prefix.size())
			continue; /* already written in full: not a suggestion */
		matches.push_back(&pool[k]);
		if (matches.size() >= kMaxMatches)
			break;
	}
	if (matches.empty())
		return false;

	/* Deterministic: frequency desc -> shorter -> alphabetical. */
	std::sort(matches.begin(), matches.end(), [](const Entry *a, const Entry *b) {
		if (a->freq != b->freq)
			return a->freq > b->freq;
		if (a->word.size() != b->word.size())
			return a->word.size() < b->word.size();
		return a->word < b->word;
	});

	const int count = (int)matches.size();
	int idx = cycle % count;
	if (idx < 0)
		idx += count;

	out.valid = true;
	out.start = start;
	out.end = caret;
	out.word = matches[(size_t)idx]->word;
	out.index = idx;
	out.alternatives = count;
	return true;
}

} // namespace sc_code
