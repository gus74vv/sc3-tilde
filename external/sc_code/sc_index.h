// SPDX-License-Identifier: GPL-3.0-only
/* sc_index.h — class library index for `sc_code` (phase S6c, the design notes).
 *
 * The editor cannot have the class library: the only truth lives in the engine
 * (6286 classes / 32347 methods in the user's installation, measured 02-nov). This
 * module is the cheap side of that: parse the dump that sclang writes with one
 * `eval` and answer `Class.method -> argument names`. That answer travels through
 * the filesystem: `sc_code` never talks to the engine.
 *
 * Format (written by `index_generator_code()`, sclang side):
 *   - `<owner>.<method>` optionally + TAB + argument names separated by spaces
 *     (`freq phase mul add`). No TAB = the method takes no arguments;
 *   - instance methods come as `<Class>.<method>`, class methods as
 *     `Meta_<Class>.<method>`: the `Meta_` prefix is stripped HERE, so `SinOsc.ar`
 *     is queried like any call the user types and the sclang side stays dumb;
 *   - `@version <3.14.1>` says which sclang wrote the dump (informative: after an
 *     upgrade the index is stale and has to be regenerated);
 *   - `@super <Class> <Superclass>` records the inheritance chain. `Class.methods`
 *     only returns what the class defines itself, and a lot of what people call is
 *     inherited (`SinOsc.madd` lives in `UGen`, two levels up), so without this the
 *     bar would be silent exactly on the common calls;
 *   - the LAST line is `#end`: the file can be half written when the editor looks
 *     at it, and without the marker nothing is accepted (all or nothing).
 *
 * Decisions:
 *   - If a class defines the same method name on the class side and on the instance
 *     side, the last line of the dump wins: they cannot be told apart (the
 *     `Cls.method(` in the text does not say which one it is).
 *   - `find()` walks the superclass chain (depth-capped, cycles are harmless);
 *     `find_direct()` does not, for whoever needs "does THIS class define it".
 *   - `symbol_names()` returns classes + unique method names, sorted: that is what
 *     feeds the completion session of the bar.
 *
 * Parsing is pure C++ and the file is read with `fopen`: no Max, no engine, so it
 * is pinned in `tests/index_test.cpp` like the buffer and the scroll.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace sc_code {

/* End-of-dump marker (see above). */
extern const char *const kIndexEndMarker;
/* Prefix of an inheritance record: `@super <Class> <Superclass>`. */
extern const char *const kIndexSuperPrefix;
/* Prefix of the header record with the sclang version that wrote the dump:
 * `@version <3.14.1>`. Informative only: the editor cannot compare it with the
 * running engine, but it explains a stale index after upgrading SC. */
extern const char *const kIndexVersionPrefix;

struct MethodSig {
	std::string owner;             /* owner class, `Meta_` already stripped */
	std::string name;              /* method (`ar`, `new`, `play`, …) */
	std::vector<std::string> args; /* WITHOUT the implicit `this`; empty = no args */
};

class ClassIndex {
  public:
	/* Parses the whole dump. Returns false (leaving the index empty) if the text does
	 * not end in `#end`, if it is empty, or if it has no valid entry. */
	bool load(const std::string &text);
	void clear();

	/* `SinOsc` + `ar` -> its signature, or nullptr. Walks the superclass chain; the
	 * returned sig may belong to a parent (check `sig->owner` to know). */
	const MethodSig *find(const std::string &owner, const std::string &method) const;
	/* Same, but only if `owner` itself defines the method. */
	const MethodSig *find_direct(const std::string &owner, const std::string &method) const;

	/* Superclass of a class, or "" if unknown. */
	std::string superclass_of(const std::string &owner) const;

	/* Classes + unique method names, sorted alphabetically. */
	std::vector<std::string> symbol_names() const;

	/* Classes only (`@super` records, class side excluded), sorted. Feeds the plain
	 * completion session of the bar (S6c+). */
	std::vector<std::string> class_names() const;

	/* Method names of `owner` and of its superclass chain, sorted and unique. The class
	 * side and the instance side are merged on purpose: the text `SinOsc.` does not say
	 * which one it is, and both are useful while typing. */
	std::vector<std::string> method_names_of(const std::string &owner) const;

	/* sclang version that wrote the dump ("" if the record is missing). */
	std::string version() const { return mVersion; }

	int entries() const { return (int)mMap.size(); }
	int super_entries() const { return (int)mSuper.size(); }
	/* Valid entries seen in the last `load()` (superclass records not included). */
	int line_count() const { return mLineCount; }

  private:
	std::map<std::string, MethodSig> mMap;   /* key: "Class.method" */
	std::map<std::string, std::string> mSuper; /* child -> parent */
	std::string mVersion;
	int mLineCount = 0;
};

/* The sclang code that writes the index at `path` (see above). It is ONE expression,
 * so it can go through `eval` unchanged; `path` is inserted escaped. */
std::string index_generator_code(const std::string &path);

} // namespace sc_code
