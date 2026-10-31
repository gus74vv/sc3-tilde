// SPDX-License-Identifier: GPL-3.0-only
/* sc_snip.h — snippets file of `sc_code`: `snip.KEY` ⇒ the body of the key.
 *
 * Format (plain UTF-8 text, multi-line, NO escapes):
 *
 * // whatever comes before the first header is ignored (it works as a
 * comment) ### kick Ndef(\kick, {|t| ... }).play; ### pad Ndef(\pad, {
 * Splay.ar(SinOsc.ar([220, 330]) * 0.1) }).play;
 *
 * Header = a line that STARTS with `### ` followed by `[A-Za-z0-9_]+` (the rest
 * of the line, up to the newline, is the key). The body goes VERBATIM up to the
 * next header: there are no escapes, so SC code (quotes, `//`, `\symbols`,
 * newlines) passes through untouched. Blank lines at the start and at the end are
 * trimmed; the indentation of the content is kept as is (if the call was
 * indented, the snippet does NOT inherit that indentation).
 *
 * What `sc_code` does with this: when evaluating, if the evaluated range is
 * exactly `snip.<key>`, it replaces it with the body (one undo step, caret at the
 * end) and does NOT send the `eval` to the engine — `sc3~` does not know what
 * `snip.kick` is. The substitution is text ONLY: nothing is evaluated.
 *
 * Parsing and matching are pure C++ (headless, see tests/snip_test.cpp): the
 * Max object only reads the file and plugs the result into the buffer.
 *
 *     GPL v3 or later (ver sc_buffer.cpp).
 */

#ifndef SC_SNIP_H
#define SC_SNIP_H

#include <cstddef>
#include <string>
#include <vector>

namespace sc_code {

struct SnipEntry {
	std::string key;
	std::string body;
};

/* Result of parsing a file. A file with no headers gives an empty dict (it is
 * not an error: the format is so lax that there is no "invalid file"). */
struct Snippets {
	std::vector<SnipEntry> entries;
	/* Declared keys that do NOT make it into the dict, to warn once at load
	 * time: duplicates (the first one wins) and empty bodies (they are
	 * ignored). */
	std::vector<std::string> duplicates;
	std::vector<std::string> empty;

	size_t size() const { return entries.size(); }
	/* Body of `key`, or nullptr if it is missing. An empty body never makes it into
	 * the dict. */
	const std::string *find(const std::string &key) const;
	/* Up to `max` space-separated keys ("kick pad"), for the error message. */
	std::string keys_list(size_t max = 12) const;
};

/* Valid key: non-empty `[A-Za-z0-9_]+` (same rule in the header and in `snip.<key>`). */
bool valid_key(const std::string &key);

/* true if `line` is a header; `key` comes out with the key (without the
 * `### `). */
bool is_header(const std::string &line, std::string &key);

/* Archivo completo ⇒ dict. CRLF/CR ⇒ LF y BOM UTF-8 fuera. */
Snippets parse_snippets(const std::string &text);

/* true if `code` (the range being sent to evaluate) is EXACTLY `snip.<key>`:
 * spaces and newlines on the edges are ignored, plus an optional trailing `;`. */
bool snip_matches(const std::string &code, std::string &key);

} // namespace sc_code

#endif /* SC_SNIP_H */
