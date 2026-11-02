// SPDX-License-Identifier: GPL-3.0-only
/* sc_call.h — "method call aid" for `sc_code` (S6c, SCODE.md §10.2): which call
 * contains the caret.
 *
 * The IDE solves this with `AutoCompleter::testMethodCall` (Qt + TokenIterator + the
 * engine's introspection). Here there is no Qt and no tokens: there is `text + caret`
 * and a deliberately narrow rule:
 *
 *   `Class.method( ...caret... )`     ← the receiver is an identifier starting with
 *                                       an uppercase letter
 *   `Class.method`   (caret on it)    ← the same name already written: clicking it in
 *                                       the document asks for the same signature
 *   `Class( ...caret... )`            ← a bare constructor: `method` comes back EMPTY
 *                                       (whoever has the index turns it into `new`)
 *
 * That is the only case that can be resolved WITHOUT type inference: `SinOsc.ar(`,
 * `Env.new(`, `Buffer.read(`. What hangs off a variable (`~x.do(`, `s.boot(`) is not
 * resolved, and nothing is guessed.
 *
 * Invariants (pinned in `tests/call_test.cpp`):
 *   - The INNERMOST open parenthesis containing the caret wins: in
 *     `SinOsc.ar(Line.kr(1, 2), caret)` the call is `Line.kr`.
 *   - Strings, symbols ('…'), character literals ($x), line comments and (nested)
 *     block comments do not count: a `(` inside a comment opens nothing and a comma
 *     inside a string does not separate arguments. A caret inside any of those is
 *     not on a name either.
 *   - Argument commas are ONLY the level 0 ones of that parenthesis: the commas of
 *     nested `[1, 2, 3]`, `(…)` or `{…}` do not advance the current argument.
 *   - Caret right after `(` => argument 0; after the 1st comma => 1; and so on.
 *   - If the call already closed (`)` before the caret) there is no context.
 *   - With no call open, the caret inside or at either edge of `Class.method` gives
 *     the context of that name, with `arg` = -1 (no current argument).
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <string>

namespace sc_code {

struct CallContext {
	bool valid;
	std::string owner;  /* `SinOsc`, as it was written */
	std::string method; /* `ar`; EMPTY = a bare constructor (`Ndef(` ⇒ `Ndef.new`) */
	int open_paren;     /* offset of the `(`; -1 = the caret is on the name, no call */
	int arg;            /* current argument, 0-based (may run past the end if there are
	                     * extra commas); -1 = unknown / no current argument */

	CallContext() : valid(false), open_paren(-1), arg(0) {}
};

/* Context of the call that contains `caret` (0 <= caret <= text.size()). */
CallContext call_context_at(const std::string &text, int caret);

} // namespace sc_code
