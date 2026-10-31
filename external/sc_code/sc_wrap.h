// SPDX-License-Identifier: GPL-3.0-only
/* sc_wrap.h — wrap visual del editor `sc_code` (fase S5e, SCODE.md §7.17).
 *
 * Problem: the editor had no horizontal scroll, so a line wider than the box could
 * only be read by enlarging the box. Now the text is SPLIT into VISUAL ROWS at the
 * available width and it is redone on its own when the width changes.
 *
 * **Headless** module (no Max, no jgraphics): cell arithmetic only, so it can be
 * pinned in a test table (`tests/wrap_test.cpp`) like the buffer and the scroll.
 *
 * Conventions: - **Cell = code point** (the font is monospace and one code point
 * takes one cell: it is the same unit as `Buffer::display_col_of_offset`).
 * Concatenating cells == the visual length of the line. - A line takes one or more
 * ROWS. The rows of all lines are numbered GLOBALLY in order (row 0 = first row of
 * the document) so vertical scrolling is still "rows × line height" and
 * `sc_scroll` does not change: the object passes `row_count()` in
 * `ScrollMetrics::line_count` and all its indices read as rows (see
 * `sc_code_metrics`). - The cut prefers a **natural boundary** (after a space or
 * an SC separator: `( [ { , ;`) as long as it lands at least at the middle of the
 * row; if there is none, it cuts hard at column `cols`. Cutting hard in the middle
 * of an identifier is the last thing tried, not the first. - The cut never splits
 * a code point (it counts cells, not bytes). - `cols <= 0` is treated as 1 (one
 * cell per row) and a giant `cols` leaves one row per line: that way `@wrap 0`
 * reproduces EXACTLY the old behavior (one row per line, no reflow) with no
 * separate code path.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <vector>

namespace sc_code {

class Buffer;

/* Cuts of ONE line into visual rows. `starts` is the cell where each row BEGINS;
 * `starts[0]` is always 0 and `rows() == starts.size()`. */
class LineWrap {
  public:
	LineWrap() { starts.push_back(0); }

	/* Recomputes the cuts of a line (UTF-8 text WITHOUT the '\n', `len` bytes).
	 * `mCells` is recomputed here (it is not passed in: the caller has no reason to
	 * count). */
	void layout(const char *text, int len, int cols);

	int rows() const { return (int)starts.size(); }
	int cells() const { return mCells; }

	int cell_start(int row) const; /* first cell of the row (clamp) */
	int cell_end(int row) const;   /* EXCLUSIVE last cell (== cells() on the last one) */

	/* Row (within THIS line) that contains the caret cell. On a cut exactly at the
	 * cut cell the NEXT row wins (the caret is drawn at the start of the row below,
	 * like any editor with wrap). */
	int row_of_cell(int cell) const;

  private:
	std::vector<int> starts;
	std::vector<int> cands; /* cells where a cut is allowed (reused per line) */
	int mCells = 0;
};

/* Wrap cache of the WHOLE document: the scroll needs the total number of rows, so
 * resolving the visible rows is not enough. It is redone when the width or the
 * document changes (`Buffer::revision()`) — like the highlight cache, but without
 * incrementality: walking the text is O(n) and a reflow is rare (resize, or an
 * edit: 1000 lines ≈ tens of µs). */
class WrapCache {
  public:
	void reset();

	/* Brings the cache up to date for `cols` cell columns. Does nothing if it already
	 * is. */
	void ensure(const Buffer &buf, int cols);

	bool valid() const { return mValid; }
	int cols() const { return mCols; }
	int revision() const { return mRevision; }

	int line_count() const { return (int)mLines.size(); }
	int row_count() const { return mRowCount > 0 ? mRowCount : 1; }

	const LineWrap &wrap(int line) const; /* with clamp and fallback to an empty row */
	int first_row(int line) const;        /* global row of the first row of `line` */
	int line_of_row(int row) const;       /* line the global row belongs to */
	int row_in_line(int row) const { return row - first_row(line_of_row(row)); }
	/* (line, cell) → GLOBAL row. */
	int row_of_line_cell(int line, int cell) const;

  private:
	std::vector<LineWrap> mLines;
	std::vector<int> mFirstRow; /* line_count + 1: start row of each line */
	int mRowCount = 0;
	int mCols = 0;
	int mRevision = -1;
	bool mValid = false;
};

} // namespace sc_code
