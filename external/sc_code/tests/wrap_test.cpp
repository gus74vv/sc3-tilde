// SPDX-License-Identifier: GPL-3.0-only
/* wrap_test.cpp — gate of the visual wrap of `sc_code` (headless,
 * S5e).
 *
 * Pins down the arithmetic BEFORE touching paint: wrap moves EVERYTHING paint draws
 * (text, caret, selection, gutter, scroll), so a mistake here looks like a broken
 * editor and is extremely expensive to find through the GUI.
 *
 *     GPL v3 or later (ver sc_wrap.cpp).
 *
 * Uso: ./build/wrap_test [-v]
 */

#include "../sc_wrap.h"
#include "../sc_buffer.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_int(const char *what, int got, int want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-60s got %d, expected %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-60s %d\n", what, got);
	}
}

/* Layout of a single line (no buffer). */
static LineWrap lay(const std::string &text, int cols) {
	LineWrap w;
	w.layout(text.data(), (int)text.size(), cols);
	return w;
}

/* ------------------------------------------------------------------------- */
/* LineWrap: cortes                                                           */

static void test_basic() {
	printf("LineWrap: base cases\n");
	{
		LineWrap w = lay("", 10);
		check_int("empty: 1 row", w.rows(), 1);
		check_int("empty: cells 0", w.cells(), 0);
		check_int("empty: end of row", w.cell_end(0), 0);
	}
	{
		LineWrap w = lay("abcde", 10);
		check_int("fits exactly: 1 row", w.rows(), 1);
		check_int("fits: cells 5", w.cells(), 5);
		check_int("fits: end 5", w.cell_end(0), 5);
	}
	{
		/* exactly cols cells: NO empty row is added */
		LineWrap w = lay("abcdefghij", 10);
		check_int("exact cols: 1 row", w.rows(), 1);
		check_int("cols exactas: cells 10", w.cells(), 10);
	}
	{
		/* cols+1 with no spaces ⇒ hard break at cols */
		LineWrap w = lay("abcdefghijk", 10);
		check_int("hard break: 2 rows", w.rows(), 2);
		check_int("hard break: row 0 starts at 0", w.cell_start(0), 0);
		check_int("hard break: row 0 ends at 10", w.cell_end(0), 10);
		check_int("hard break: row 1 starts at 10", w.cell_start(1), 10);
		check_int("hard break: row 1 ends at 11", w.cell_end(1), 11);
		check_int("hard break: row_of_cell(11) = 1", w.row_of_cell(11), 1);
	}
	{
		/* 25 celdas en 10 ⇒ 3 filas (10/10/5) */
		LineWrap w = lay("abcdefghijklmnopqrstuvwxy", 10);
		check_int("25 in 10: 3 rows", w.rows(), 3);
		check_int("25 in 10: row 2 starts at 20", w.cell_start(2), 20);
		check_int("25 en 10: fin 25", w.cell_end(2), 25);
	}
}

static void test_soft_break() {
	printf("LineWrap: break at a natural boundary\n");
	{
		/* "hello world foo" in 10: the natural cut <= 10 is after the space
		 * (cell 6) — 6 > low (5) ⇒ it wins over the hard cut at 10. */
		LineWrap w = lay("hello world foo", 10);
		check_int("space: 2 rows", w.rows(), 2);
		check_int("space: row 0 [0,6)", w.cell_end(0), 6);
		check_int("space: row 1 starts at 6", w.cell_start(1), 6);
		check_int("space: end 15", w.cell_end(1), 15);
	}
	{
		/* The space is BEFORE the middle ⇒ it is ignored and the cut is hard (we do
		 * not leave a 2-cell row next to a 10-cell one). */
		LineWrap w = lay("ab cdefghijkl", 10);
		check_int("early space: row 0 full", w.cell_end(0), 10);
		check_int("early space: 2 rows", w.rows(), 2);
	}
	{
		/* SC separators are natural boundaries too */
		LineWrap w = lay("Ndef(\\x, { SinOsc.ar });", 12);
		const int e0 = w.cell_end(0);
		check_int("separator: breaks after '(' or ','", e0 > 6 && e0 <= 12, 1);
		check_int("separator: no empty rows", w.cell_start(1) > 0, 1);
	}
	{
		/* no spaces in 1 column: one cell per row, without hanging */
		LineWrap w = lay("abc", 1);
		check_int("cols=1: 3 rows", w.rows(), 3);
		check_int("cols=1: fin 3", w.cell_end(2), 3);
	}
	{
		LineWrap w = lay("abc", 0); /* cols inválido ⇒ 1 */
		check_int("cols=0 is treated as 1", w.rows(), 3);
	}
	{
		/* giant cols = no wrap (old behavior: 1 row per line) */
		LineWrap w = lay("a very long line for a test", 1 << 28);
		check_int("huge cols: 1 row", w.rows(), 1);
	}
}

static void test_row_of_cell() {
	printf("LineWrap: cell → row (caret)\n");
	{
		LineWrap w = lay("abcdefghijklmnopqrstuvwxy", 10); /* filas [0,10) [10,20) [20,25) */
		check_int("cell 0 → row 0", w.row_of_cell(0), 0);
		check_int("cell 9 → row 0", w.row_of_cell(9), 0);
		check_int("cell 10 → row 1 (break: the lower one wins)", w.row_of_cell(10), 1);
		check_int("cell 20 → row 2", w.row_of_cell(20), 2);
		check_int("cell 25 (end) → row 2", w.row_of_cell(25), 2);
		check_int("negative cell → row 0", w.row_of_cell(-3), 0);
		check_int("past-the-end cell → last row", w.row_of_cell(99), 2);
	}
	{
		LineWrap w = lay("", 10);
		check_int("empty: row_of_cell(0) = 0", w.row_of_cell(0), 0);
	}
}

static void test_utf8() {
	printf("LineWrap: cells = code points (not bytes)\n");
	{
		/* "ñ" is 2 bytes but 1 cell: 5 code points fit in 5 columns. */
		const std::string s = "ñandú"; /* 6 bytes, 5 celdas */
		LineWrap w = lay(s, 5);
		check_int("acentos: cells 5", w.cells(), 5);
		check_int("accents: 1 row", w.rows(), 1);
	}
	{
		/* "áéíóúü" = 6 code points (12 bytes) ⇒ 2 rows of 3. */
		const std::string s = "áéíóúü";
		LineWrap w = lay(s, 3);
		check_int("acentos: cells 6", w.cells(), 6);
		check_int("accents: 2 rows", w.rows(), 2);
		check_int("accents: row 0 [0,3)", w.cell_end(0), 3);
		check_int("accents: end 6 (not one byte more)", w.cell_end(1), 6);
	}
}

/* ------------------------------------------------------------------------- */
/* WrapCache: documento entero                                                */

static void test_cache() {
	printf("WrapCache: global rows per line\n");
	Buffer buf("uno\ndos lineas largas\ntres\n");
	/* lines: 0 "uno", 1 "dos lineas largas", 2 "tres", 3 "" (the trailing
	 * \'\\n\') */
	WrapCache c;
	c.ensure(buf, 10);
	check_int("4 lines", c.line_count(), 4);
	/* "uno" =1, "dos lineas largas" (17 cells) = 2 rows, "tres" =1, "" =1 */
	check_int("total rows", c.row_count(), 5);
	check_int("first_row(0) = 0", c.first_row(0), 0);
	check_int("first_row(1) = 1", c.first_row(1), 1);
	check_int("first_row(2) = 3", c.first_row(2), 3);
	check_int("first_row(3) = 4", c.first_row(3), 4);
	check_int("first_row(4) = 5 (fin)", c.first_row(4), 5);
	check_int("row 2 belongs to line 1", c.line_of_row(2), 1);
	check_int("row 2 is the 2nd row of its line", c.row_in_line(2), 1);
	check_int("row 3 belongs to line 2", c.line_of_row(3), 2);
	check_int("row 4 belongs to line 3", c.line_of_row(4), 3);
	check_int("row_of_line_cell(1, 10) = 2", c.row_of_line_cell(1, 10), 2);
	check_int("row_of_line_cell(2, 0) = 3", c.row_of_line_cell(2, 0), 3);
	check_int("out-of-range row: last", c.line_of_row(999), 3);
	check_int("negative row: 0", c.line_of_row(-5), 0);
}

static void test_reflow() {
	printf("WrapCache: reflow on width change and on edit\n");
	Buffer buf("aaaaaaaaaabbbbbbbbbb\ncorto\n");
	WrapCache c;
	c.ensure(buf, 30);
	check_int("width 30: 3 rows (1 per line)", c.row_count(), 3);
	c.ensure(buf, 10);
	check_int("width 10: rebuilt ⇒ 4 rows (2+1+1)", c.row_count(), 4);
	check_int("width 10: the width got cached", c.cols(), 10);

	/* Same width ⇒ no recompute (same revision and same rows). */
	const int rev = c.revision();
	c.ensure(buf, 10);
	check_int("repeated ensure changes nothing", c.revision(), rev);

	/* Editing raises the revision and redoes the layout. */
	buf.set_text("aaaaaaaaeeeeeeeeeeffffffffff\n");
	c.ensure(buf, 10);
	check_int("after editing: new revision", c.revision() == rev, 0);
	check_int("after editing: 4 rows (3 + the empty line)", c.row_count(), 4); /* 30 celdas en 10 */

	/* Giant width = no wrap: one row per line (equivalent to @wrap 0). */
	c.ensure(buf, 1 << 28);
	check_int("no wrap: 2 rows", c.row_count(), 2);
}

/* Integration: the caret of a long line falls on the right row and its x is
 * measured from the START OF THE ROW (that is what paint does). */
static void test_integration() {
	printf("integration with Buffer: caret and columns\n");
	Buffer buf("0123456789abcdefghij\n0123\n");
	WrapCache c;
	c.ensure(buf, 10);
	check_int("3 lines (the trailing '\\n' opens the empty one)", c.line_count(), 3);
	check_int("4 rows (2 + 1 + 1)", c.row_count(), 4);

	/* caret at the end of line 0 = offset 20 ⇒ cell 20 ⇒ last row of the line */
	const int caret = 20;
	const int line = buf.line_of_offset(caret);
	const int cell = buf.display_col_of_offset(caret);
	check_int("caret on line 0", line, 0);
	check_int("caret in cell 20", cell, 20);
	check_int("caret on global row 1", c.row_of_line_cell(line, cell), 1);
	const int ril = c.row_in_line(c.row_of_line_cell(line, cell));
	check_int("caret on the 2nd row of its line", ril, 1);
	check_int("caret x: cell - row start = 10",
			  cell - c.wrap(line).cell_start(ril), 10);

	/* offset → (row, cell) and back: the round trip does not lose the offset */
	const int off = buf.offset_of_display_col(0, 13);
	check_int("offset of cell 13", off, 13);
	check_int("cell 13 lands on row 1", c.row_of_line_cell(0, buf.display_col_of_offset(off)), 1);
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (!strcmp(argv[i], "-v"))
			gVerbose = true;

	test_basic();
	test_soft_break();
	test_row_of_cell();
	test_utf8();
	test_cache();
	test_reflow();
	test_integration();

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
