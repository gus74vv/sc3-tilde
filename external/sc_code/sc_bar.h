// SPDX-License-Identifier: GPL-3.0-only
/* sc_bar.h — the bottom bar of `sc_code` (S6c, SCODE.md §10.2): geometry and text
 * layout, with no Max and no jgraphics, so the risky part is a test table.
 *
 * What it draws (one reserved line, always present while `@callaid` is on):
 *
 *     SinOsc.ar(freq, phase, mul, add)                              2/4
 *                       ^^^^^ the current argument, in `@barhighlight`
 *
 * Rules that make it testable (`tests/bar_test.cpp`):
 *   - The counter ("n/N") is right-aligned and is NEVER truncated; the signature is
 *     truncated with "…" when it does not fit, and the current argument has priority
 *     over the head of the signature (that is the whole point of the aid, so it must
 *     stay on screen even in a narrow box).
 *   - The bar is ONE line: candidates are navigated by cycling (§10.6), never by a
 *     multi-line popup.
 *   - Cells: the signature is ASCII (class, method and argument names), so one byte
 *     is one monospace cell; the only multi-byte character is the "…" this module
 *     inserts, and it counts as ONE cell (accounting is explicit, see `cells_in`).
 *   - Height comes from the font (line_h + 4), so it scales with `@fontsize`.
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

struct Span {
	int start, end; /* byte offsets, [start, end) */
	Span(int s = -1, int e = -1) : start(s), end(e) {}
};

struct BarModel {
	std::string sig;         /* `SinOsc.ar(freq, phase, mul, add)` */
	std::vector<Span> args;  /* where each argument name lives inside `sig` */
	int arg;                 /* current argument (0-based), -1 = none */
	int count;               /* number of arguments in the signature */

	BarModel() : arg(-1), count(0) {}
};

/* The model for a signature coming from the index. `arg` may run past `count` (the
 * user typed extra commas): it is clamped to the last argument, which is what the bar
 * shows while the call is wrong anyway. */
BarModel bar_model(const std::string &owner, const std::string &method, const std::vector<std::string> &args,
				   int arg);

struct BarCells {
	std::string text;     /* left part, already truncated with "…" */
	int hl_start, hl_end; /* highlighted span, BYTE offsets inside `text`, -1 = none */
	std::string counter;  /* "2/4", may be empty (no arguments) */
	int counter_col;      /* cell column where the counter starts, -1 = none */
	bool arg_visible;     /* the current argument made it on screen */
	int used;             /* cells taken by `text` (== cells_in(text)) */

	BarCells() : hl_start(-1), hl_end(-1), counter_col(-1), arg_visible(false), used(0) {}
};

/* Lays `m` out in `cells` monospace columns (cells <= 0 = unbounded). */
BarCells bar_layout(const BarModel &m, int cells);

/* Height of the bar for a font with this line height (SCODE.md §10.2: line_h + 4). */
int bar_height(double line_h);

/* Height left for the text viewport: the bar is RESERVED, not intermittent (making it
 * appear and disappear would reflow the wrap and make the caret jump). */
double bar_viewport_h(double box_h, double line_h, bool callaid);

/* UTF-8 code points in `s` (one code point = one cell in the bar). */
int cells_in(const std::string &s);

/* Where the caret's line must not go: the bar eats the bottom `bar_height` px. */
bool bar_covers(double y, double box_h, double line_h, bool callaid);

} // namespace sc_code
