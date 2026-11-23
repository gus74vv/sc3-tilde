// SPDX-License-Identifier: GPL-3.0-only
/* bar_test.cpp — S6c gate: geometry and text layout of the bottom bar.
 *
 * The bar is the only thing between the user and a wrong signature, and it has to
 * behave in a box that is narrower than the signature. What is pinned here:
 *   - the current argument stays visible when the signature does not fit (it is the
 *     whole point), while the counter is never truncated and the head is elided;
 *   - the "…" counts as ONE cell although it is three bytes;
 *   - the layout never exceeds the width it was given (that would paint over the
 *     counter or clip at the box edge);
 *   - the bar is reserved, so the text viewport shrinks by exactly `bar_height`.
 *
 * Usage: ./build/bar_test [-v]
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "../sc_bar.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_int(const char *what, int got, int want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %d, want %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-58s %d\n", what, got);
	}
}

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got \"%s\", want \"%s\"\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-58s \"%s\"\n", what, got.c_str());
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %s, want %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-58s %s\n", what, got ? "true" : "false");
	}
}

static std::string sub(const std::string &s, int start, int end) {
	if (start < 0 || end < start || end > (int)s.size())
		return std::string("<out of range>");
	return s.substr((size_t)start, (size_t)(end - start));
}

int main(int argc, char **argv) {
	gVerbose = (argc > 1 && argv[1][0] == '-');

	const std::vector<std::string> sin_args = {"freq", "phase", "mul", "add"};

	/* --- the model ---------------------------------------------------------- */
	{
		printf("model\n");
		const BarModel m = bar_model("SinOsc", "ar", sin_args, 1);
		check_str("signature", m.sig, "SinOsc.ar(freq, phase, mul, add)");
		check_int("argument count", m.count, 4);
		check_int("current argument", m.arg, 1);
		check_str("span of the 1st argument", sub(m.sig, m.args[0].start, m.args[0].end), "freq");
		check_str("span of the last argument", sub(m.sig, m.args[3].start, m.args[3].end), "add");
		const BarModel clamped = bar_model("SinOsc", "ar", sin_args, 9);
		check_int("extra commas clamp to the last argument", clamped.arg, 3);
		const BarModel none = bar_model("SinOsc", "stop", {}, 0);
		check_str("no arguments", none.sig, "SinOsc.stop()");
		check_int("  and no current argument", none.arg, -1);
		check_int("  and no counter", (int)bar_layout(none, 40).counter.size(), 0);

		/* `arg` = -1 (the caret is ON the name, sc_call): signature yes, counter no. */
		const std::vector<std::string> duty_args = {"dur", "reset", "level", "doneAction"};
		const BarModel oname = bar_model("Duty", "kr", duty_args, -1);
		check_str("caret on the name: signature", oname.sig, "Duty.kr(dur, reset, level, doneAction)");
		check_int("  no current argument", oname.arg, -1);
		const BarCells nc = bar_layout(oname, -1);
		check_int("  no counter", (int)nc.counter.size(), 0);
		check_int("  nothing highlighted", nc.hl_start, -1);
	}

	/* --- cells -------------------------------------------------------------- */
	{
		printf("cells\n");
		check_int("ASCII", cells_in("SinOsc.ar"), 9);
		check_int("the ellipsis is ONE cell", cells_in("\xE2\x80\xA6"), 1);
		check_int("mixed", cells_in("\xE2\x80\xA6" "ar"), 3); /* split: \x80ar is one escape */
	}

	/* --- layout: it fits ---------------------------------------------------- */
	{
		printf("layout: fits\n");
		BarModel m = bar_model("SinOsc", "ar", sin_args, 1);
		const BarCells c = bar_layout(m, -1);
		check_str("unbounded: whole signature", c.text, m.sig);
		check_int("unbounded: cells used", c.used, (int)m.sig.size());
		check_str("unbounded: highlighted argument", sub(c.text, c.hl_start, c.hl_end), "phase");
		check_str("unbounded: counter", c.counter, "2/4");
		check_bool("unbounded: the argument is visible", c.arg_visible, true);

		/* Exactly the width of signature + gap + counter. */
		const int exact = (int)m.sig.size() + 1 + (int)c.counter.size();
		const BarCells e = bar_layout(m, exact);
		check_str("exact: no truncation", e.text, m.sig);
		check_int("exact: counter column", e.counter_col, exact - 3);
		check_bool("exact: nothing overlaps", e.used <= e.counter_col, true);

		/* One more cell than needed: the signature still fits and the counter moves. */
		const BarCells w = bar_layout(m, exact + 7);
		check_str("wider: same signature", w.text, m.sig);
		check_int("wider: the counter is right-aligned", w.counter_col, exact + 7 - 3);
	}

	/* --- layout: it does not fit ------------------------------------------- */
	{
		printf("layout: truncation\n");
		BarModel m = bar_model("SinOsc", "ar", sin_args, 1);

		/* Early argument (phase, ends at 21) and a box of 20: the head cannot hold it,
		 * so the window follows the argument. */
		const BarCells c = bar_layout(m, 20);
		check_int("20 cells: never wider than the box", c.used <= 20 - 1 - 3, true);
		check_bool("20 cells: leading ellipsis", c.text.compare(0, 3, "\xE2\x80\xA6") == 0, true);
		check_str("20 cells: the current argument is on screen", sub(c.text, c.hl_start, c.hl_end), "phase");
		check_str("20 cells: counter survives", c.counter, "2/4");
		check_bool("20 cells: no overlap with the counter", c.used < c.counter_col, true);

		/* With the FIRST argument current, the head fits and the tail is elided. */
		BarModel first = bar_model("SinOsc", "ar", sin_args, 0);
		const BarCells h = bar_layout(first, 24);
		check_str("24 cells, arg 0: the head is kept", sub(h.text, 0, 4), "SinO");
		check_str("24 cells, arg 0: trailing ellipsis", sub(h.text, (int)h.text.size() - 3, (int)h.text.size()),
				  "\xE2\x80\xA6");
		check_str("24 cells, arg 0: highlighted", sub(h.text, h.hl_start, h.hl_end), "freq");

		/* The LAST argument in a narrow box: the window has to move to it. */
		BarModel last = bar_model("SinOsc", "ar", sin_args, 3);
		const BarCells n = bar_layout(last, 16);
		check_str("16 cells, arg 3: highlighted", sub(n.text, n.hl_start, n.hl_end), "add");
		check_bool("16 cells, arg 3: leading ellipsis", n.text.compare(0, 3, "\xE2\x80\xA6") == 0, true);

		/* Degenerate widths: no crash, no overflow, and the counter is dropped first. */
		const BarCells tiny = bar_layout(m, 5);
		check_int("5 cells: no counter (the signature wins)", (int)tiny.counter.size(), 0);
		check_bool("5 cells: still highlights something", tiny.arg_visible, true);
		check_bool("5 cells: fits", tiny.used <= 5, true);
		const BarCells one = bar_layout(m, 1);
		check_str("1 cell: just the ellipsis", one.text, "\xE2\x80\xA6");
		check_bool("1 cell: nothing claimed as visible", one.arg_visible, false);
		check_int("0 cells: empty", (int)bar_layout(m, 0).text.size(), 0);
	}

	/* --- geometry ----------------------------------------------------------- */
	{
		printf("geometry\n");
		check_int("bar height for a 13 px line", bar_height(13.), 17);
		check_int("bar height for a 20 px line", bar_height(20.), 24);
		check_int("bar height never below 4", bar_height(0.), 4);

		check_int("viewport with the bar", (int)bar_viewport_h(200., 13., true), 183);
		check_int("viewport without the bar", (int)bar_viewport_h(200., 13., false), 200);
		check_int("box smaller than the bar", (int)bar_viewport_h(10., 13., true), 0);

		check_bool("the bar covers the last line", bar_covers(190., 200., 13., true), true);
		check_bool("the text area is not covered", bar_covers(100., 200., 13., true), false);
		check_bool("no bar, nothing covered", bar_covers(199., 200., 13., false), false);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
