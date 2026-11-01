// SPDX-License-Identifier: GPL-3.0-only
/* sc_path.cpp — see sc_path.h (SCODE.md §12: Finder drops into `sc_code`).
 *
 *     GPL v3 or later (see sc_path.h).
 */

#include "sc_path.h"

namespace sc_code {

static std::string strip_trailing_slashes(std::string s) {
	while (s.size() > 1 && s[s.size() - 1] == '/')
		s.erase(s.size() - 1);
	return s;
}

std::string path_to_posix(const std::string &in) {
	if (in.empty())
		return in;
	std::string s = in;
	/* Max volume dialect: `<volume name>:/rest`. A volume name never starts with a
	 * `/` (that would already be a POSIX path), and the first `:/` is the boundary
	 * (volume names do contain spaces). */
	if (s[0] != '/') {
		const std::string::size_type p = s.find(":/");
		if (p != std::string::npos && p > 0)
			s = s.substr(p + 1);
	}
	return s;
}

std::string path_to_tilde(const std::string &home_in, const std::string &in) {
	if (in.empty())
		return in;

	/* Without that prefix the name may be relative to a search path and it is left as
	 * is: `x.wav` works in SC too. */
	const std::string s = path_to_posix(in);

	const std::string home = strip_trailing_slashes(home_in);
	if (home.empty() || home == "/")
		return s;
	if (s == home)
		return "~";
	if (s.size() > home.size() && s.compare(0, home.size(), home) == 0 && s[home.size()] == '/')
		return "~" + s.substr(home.size());
	return s;
}

std::string quote_sc_string(const std::string &path) {
	std::string out;
	out.reserve(path.size() + 2);
	out += '"';
	for (std::string::size_type i = 0; i < path.size(); ++i) {
		const char c = path[i];
		if (c == '\\' || c == '"')
			out += '\\';
		out += c;
	}
	out += '"';
	return out;
}

std::string dropped_text(const std::string &home, const std::vector<std::string> &names) {
	std::string out;
	for (std::vector<std::string>::size_type i = 0; i < names.size(); ++i) {
		if (i)
			out += ' ';
		out += quote_sc_string(path_to_tilde(home, names[i]));
	}
	return out;
}

} // namespace sc_code
