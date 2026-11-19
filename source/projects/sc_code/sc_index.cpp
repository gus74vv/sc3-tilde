// SPDX-License-Identifier: GPL-3.0-only
/* sc_index.cpp — see sc_index.h (S6c, the design notes).
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "sc_index.h"

#include <set>

namespace sc_code {

const char *const kIndexEndMarker = "#end";
const char *const kIndexSuperPrefix = "@super ";
const char *const kIndexVersionPrefix = "@version ";

void ClassIndex::clear() {
	mMap.clear();
	mSuper.clear();
	mVersion.clear();
	mLineCount = 0;
}

/* Strips the `Meta_` prefix of a class-side owner and returns the name the user
 * writes: `Meta_SinOsc` -> `SinOsc`. */
static std::string strip_meta(const std::string &owner) {
	static const std::string kMeta = "Meta_";
	if (owner.size() > kMeta.size() && owner.compare(0, kMeta.size(), kMeta) == 0)
		return owner.substr(kMeta.size());
	return owner;
}

/* Splits on runs of spaces (argument lists and `@super` records). */
static void split_spaces(const std::string &s, size_t from, std::vector<std::string> &out) {
	size_t p = from;
	while (p < s.size()) {
		while (p < s.size() && s[p] == ' ')
			p++;
		const size_t q = s.find(' ', p);
		const size_t stop = (q == std::string::npos) ? s.size() : q;
		if (stop > p)
			out.push_back(s.substr(p, stop - p));
		p = (q == std::string::npos) ? s.size() : q;
	}
}

bool ClassIndex::load(const std::string &text) {
	clear();
	if (text.empty())
		return false;

	std::map<std::string, MethodSig> parsed;
	std::map<std::string, std::string> supers;
	std::string version;
	std::string lastNonEmpty;
	int lineCount = 0;
	size_t pos = 0;
	while (pos <= text.size()) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos)
			end = text.size();
		std::string line = text.substr(pos, end - pos);
		pos = end + 1;

		/* Defensive CRLF (sclang writes "\n", but it costs nothing). */
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		if (line.empty())
			continue;
		lastNonEmpty = line;
		if (line == kIndexEndMarker)
			continue;

		/* Header record: the sclang version that wrote the dump. */
		const std::string kVersion = kIndexVersionPrefix;
		if (line.compare(0, kVersion.size(), kVersion) == 0) {
			version = line.substr(kVersion.size());
			while (!version.empty() && version[version.size() - 1] == ' ')
				version.erase(version.size() - 1);
			continue;
		}

		/* Inheritance record: `@super <Class> <Superclass>`. */
		const std::string kSuper = kIndexSuperPrefix;
		if (line.compare(0, kSuper.size(), kSuper) == 0) {
			std::vector<std::string> f;
			split_spaces(line, kSuper.size(), f);
			if (f.size() >= 2 && !f[0].empty() && !f[1].empty() && f[1] != "nil")
				supers[f[0]] = f[1];
			continue;
		}

		const size_t dot = line.rfind('.');
		if (dot == std::string::npos || dot == 0 || dot + 1 >= line.size())
			continue; /* no `Class.method`: ignored, does not abort the dump */

		const size_t tab = line.find('\t');
		const std::string owner = strip_meta(line.substr(0, dot));
		const std::string method = line.substr(dot + 1, (tab == std::string::npos ? line.size() : tab) - dot - 1);
		if (owner.empty() || method.empty())
			continue;

		MethodSig sig;
		sig.owner = owner;
		sig.name = method;
		if (tab != std::string::npos)
			split_spaces(line, tab + 1, sig.args);
		parsed[owner + "." + method] = sig;
		lineCount += 1;
	}

	/* Without the final marker the file is being written (or was truncated), so it is
	 * not used: half an index is worse than none. */
	if (lastNonEmpty != kIndexEndMarker || parsed.empty())
		return false;

	mMap.swap(parsed);
	mSuper.swap(supers);
	mVersion.swap(version);
	mLineCount = lineCount;
	return true;
}

const MethodSig *ClassIndex::find_direct(const std::string &owner, const std::string &method) const {
	const std::map<std::string, MethodSig>::const_iterator it = mMap.find(owner + "." + method);
	return it == mMap.end() ? nullptr : &it->second;
}

const MethodSig *ClassIndex::find(const std::string &owner, const std::string &method) const {
	const MethodSig *sig = find_direct(owner, method);
	std::string cur = owner;
	/* Depth-capped so a bad dump (A -> B -> A) cannot hang the editor. */
	for (int hops = 0; sig == nullptr && hops < 64; ++hops) {
		const std::string parent = superclass_of(cur);
		if (parent.empty() || parent == cur)
			break;
		cur = parent;
		sig = find_direct(cur, method);
	}
	return sig;
}

std::string ClassIndex::superclass_of(const std::string &owner) const {
	const std::map<std::string, std::string>::const_iterator it = mSuper.find(owner);
	return it == mSuper.end() ? std::string() : it->second;
}

std::vector<std::string> ClassIndex::symbol_names() const {
	std::set<std::string> unique;
	for (std::map<std::string, MethodSig>::const_iterator it = mMap.begin(); it != mMap.end(); ++it) {
		unique.insert(it->second.owner);
		unique.insert(it->second.name);
	}
	for (std::map<std::string, std::string>::const_iterator it = mSuper.begin(); it != mSuper.end(); ++it) {
		unique.insert(it->first);
		if (!it->second.empty() && it->second != "nil")
			unique.insert(it->second);
	}
	return std::vector<std::string>(unique.begin(), unique.end());
}

std::vector<std::string> ClassIndex::class_names() const {
	/* `mSuper` is a std::map: it is already sorted and unique. The class-side records
	 * (`Meta_X`) are dropped: `Meta_SinOsc` is not something the user writes. */
	static const std::string kMeta = "Meta_";
	std::vector<std::string> out;
	out.reserve(mSuper.size());
	for (std::map<std::string, std::string>::const_iterator it = mSuper.begin(); it != mSuper.end(); ++it)
		if (it->first.compare(0, kMeta.size(), kMeta) != 0)
			out.push_back(it->first);
	return out;
}

std::vector<std::string> ClassIndex::method_names_of(const std::string &owner) const {
	std::set<std::string> unique;
	std::string cur = owner;
	for (int hops = 0; hops < 64 && !cur.empty(); ++hops) {
		const std::string prefix = cur + ".";
		for (std::map<std::string, MethodSig>::const_iterator it = mMap.lower_bound(prefix);
			 it != mMap.end(); ++it) {
			if (it->first.compare(0, prefix.size(), prefix) != 0)
				break;
			unique.insert(it->second.name);
		}
		const std::string parent = superclass_of(cur);
		if (parent.empty() || parent == cur)
			break;
		cur = parent;
	}
	return std::vector<std::string>(unique.begin(), unique.end());
}

/* --- generator (sclang side) ------------------------------------------------- */

static std::string escape_sc_string(const std::string &path) {
	std::string esc;
	esc.reserve(path.size() + 8);
	for (size_t i = 0; i < path.size(); ++i) {
		const char c = path[i];
		if (c == '\\' || c == '"')
			esc += '\\';
		esc += c;
	}
	return esc;
}

std::string index_generator_code(const std::string &path) {
	/* One expression: `eval` takes one. `@PATH@` is the only thing substituted. */
	static const char *const kTemplate = R"SC(
(
var path = "@PATH@";
var t = Main.elapsedTime;
var n = 0;
var out = File(path, "w");
if (out.isNil or: { out.isOpen.not }) {
	("sc_code index: cannot write " ++ path).postln;
} {
	out.write("@version " ++ Main.version ++ "\n");
	Class.allClasses.do { |cls|
		var sup = cls.superclass;
		if (sup.notNil) { out.write("@super " ++ cls.asString ++ " " ++ sup.asString ++ "\n") };
	};
	Class.allClasses.do { |cls|
		var cname = cls.asString;
		cls.methods.do { |m|
			var args = m.argNames;
			out.write(cname ++ "." ++ m.name.asString);
			if (args.size > 1) { out.write("\t" ++ args.copyToEnd(1).join(" ")) };
			out.write("\n");
			n = n + 1;
		};
	};
	out.write("#end\n");
	out.close;
	("sc_code index: " ++ n ++ " entries -> " ++ path ++ " in " ++ (Main.elapsedTime - t).round(0.001) ++ " s").postln;
};
)
)SC";
	static const std::string kPlaceholder = "@PATH@";

	std::string code = kTemplate;
	const std::string esc = escape_sc_string(path);
	const size_t at = code.find(kPlaceholder);
	if (at != std::string::npos)
		code.replace(at, kPlaceholder.size(), esc);
	return code;
}

} // namespace sc_code
