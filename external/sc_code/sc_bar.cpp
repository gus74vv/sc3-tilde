// SPDX-License-Identifier: GPL-3.0-only
/* sc_bar.cpp — see sc_bar.h (S6c, SCODE.md §10.2).
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "sc_bar.h"

namespace sc_code {

static const char *const kEllipsis = "\xE2\x80\xA6"; /* … = one cell, three bytes */

BarModel bar_model(const std::string &owner, const std::string &method, const std::vector<std::string> &args,
				   int arg) {
	BarModel m;
	m.sig = owner + "." + method + "(";
	for (size_t i = 0; i < args.size(); ++i) {
		if (i)
			m.sig += ", ";
		const int start = (int)m.sig.size();
		m.sig += args[i];
		m.args.push_back(Span(start, (int)m.sig.size()));
	}
	m.sig += ")";
	m.count = (int)args.size();
	if (args.empty()) {
		m.arg = -1;
	} else if (arg < 0) {
		m.arg = -1;
	} else {
		m.arg = arg < m.count ? arg : m.count - 1; /* extra commas: clamp to the last */
	}
	return m;
}

int cells_in(const std::string &s) {
	int n = 0;
	for (size_t i = 0; i < s.size(); ++i)
		if (((unsigned char)s[i] & 0xC0) != 0x80) /* not a UTF-8 continuation byte */
			++n;
	return n;
}

int bar_height(double line_h) {
	int h = (int)(line_h + 0.5) + 4; /* rounded line height + a 2 px margin each side */
	return h < 4 ? 4 : h;
}

double bar_viewport_h(double box_h, double line_h, bool callaid) {
	if (!callaid)
		return box_h;
	const double h = box_h - (double)bar_height(line_h);
	return h < 0. ? 0. : h;
}

bool bar_covers(double y, double box_h, double line_h, bool callaid) {
	if (!callaid)
		return false;
	return y >= box_h - (double)bar_height(line_h) - 0.5;
}

BarCells bar_layout(const BarModel &m, int cells) {
	BarCells out;
	/* The counter only exists when there IS a current argument: `arg` = -1 is "the caret
	 * is on the name, no parentheses" (sc_call), and `0/4` would be a lie. */
	out.counter = (m.count > 0 && m.arg >= 0) ? std::to_string(m.arg + 1) + "/" + std::to_string(m.count)
													 : std::string();
	if (cells == 0) { /* nothing fits */
		out.counter.clear();
		return out;
	}

	const bool unbounded = cells < 0;
	int avail = unbounded ? 0x7fffffff : cells;
	if (!out.counter.empty()) {
		const int want_cols = (int)out.counter.size();
		if (!unbounded && cells < want_cols + 4) {
			out.counter.clear(); /* too narrow: the signature wins (that is the aid) */
		} else if (!unbounded) {
			avail = cells - want_cols - 1; /* one space between the signature and the counter */
			out.counter_col = cells - want_cols;
		}
	}

	const std::string &sig = m.sig;
	const int sig_cells = cells_in(sig);
	const Span want = (m.arg >= 0 && m.arg < (int)m.args.size()) ? m.args[(size_t)m.arg] : Span();

	if (sig_cells <= avail) {
		out.text = sig;
		out.used = sig_cells;
		if (want.start >= 0) {
			out.hl_start = want.start;
			out.hl_end = want.end;
		}
		out.arg_visible = want.start >= 0;
		return out;
	}

	/* It does not fit. Two cases: the argument fits in the head, or the window has to
	 * follow the argument (the aid has priority over the start of the signature). */
	if (want.start < 0 || want.end <= avail) {
		const int keep = avail - 1; /* room for the trailing "…" */
		out.text = sig.substr(0, (size_t)keep) + kEllipsis;
		out.used = keep + 1;
		if (want.start >= 0) {
			out.hl_start = want.start < keep ? want.start : keep;
			out.hl_end = want.end < keep ? want.end : keep;
			out.arg_visible = keep >= want.end;
		}
		return out;
	}

	/* Window that ENDS at the argument, with a leading "…". */
	const int budget = avail - 1; /* the leading "…" */
	if (budget < 1) {             /* one cell: nothing but the ellipsis fits */
		out.text = kEllipsis;
		out.used = 1;
		return out;
	}
	int stop = want.end;
	if (stop > sig_cells)
		stop = sig_cells;
	int start = stop - budget;
	if (start < 0)
		start = 0;
	std::string body = sig.substr((size_t)start, (size_t)(stop - start));
	out.text = (start > 0 ? kEllipsis : "") + body;
	out.used = cells_in(out.text);
	/* A trailing "…" only if there is still a cell and something left to elide. */
	if (out.used < avail && stop < sig_cells) {
		out.text += kEllipsis;
		out.used += 1;
	}
	/* Offsets inside the text are BYTES (it is a UTF-8 string): the leading ellipsis is 3. */
	const int shift = (start > 0) ? 3 : 0;
	out.hl_start = want.start - start + shift;
	out.hl_end = want.end - start + shift;
	if (out.hl_start < 0)
		out.hl_start = 0;
	if (out.hl_end > (int)out.text.size())
		out.hl_end = (int)out.text.size();
	out.arg_visible = out.hl_start < out.hl_end;
	return out;
}

} // namespace sc_code
