// SPDX-License-Identifier: GPL-3.0-only
/* scroll_test.cpp — gate of the `sc_code` vertical scroll (headless).
 *
 * Pins down the `sc_scroll` arithmetic before touching paint: scroll is one of the
 * easiest bugs to introduce and the most expensive to find through the GUI (a
 * 1000-line document, caret out of sight). Everything here is pure geometry.
 *
 *     GPL v3 or later (ver sc_scroll.cpp).
 *
 * Uso: ./build/scroll_test [-v]
 */

#include "../sc_scroll.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_dbl(const char *what, double got, double want) {
	++gChecks;
	if (std::fabs(got - want) > 1e-9) {
		++gFails;
		printf("  FAIL %-58s got %g, esperado %g\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-58s %g\n", what, got);
	}
}

static void check_int(const char *what, int got, int want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %d, esperado %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-58s %d\n", what, got);
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %s, esperado %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-58s %s\n", what, got ? "true" : "false");
	}
}

/* Base case: 100 lines of 10 px in a 200 px box, margin 4. */
static ScrollMetrics tall() {
	ScrollMetrics m;
	m.line_count = 100;
	m.line_h = 10.;
	m.view_h = 200.;
	m.pad_y = 4.;
	return m;
}

/* Three lines that fit whole: there is nothing to scroll. */
static ScrollMetrics fits() {
	ScrollMetrics m;
	m.line_count = 3;
	m.line_h = 10.;
	m.view_h = 200.;
	m.pad_y = 4.;
	return m;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (!strcmp(argv[i], "-v"))
			gVerbose = true;

	printf("sc_scroll — scroll geometry\n");

	/* --- content / maximum ------------------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		check_dbl("content = lines*height + 2*pad", s.content_height(m), 1008.);
		check_dbl("max_y = content - view", s.max_y(m), 808.);
		check_bool("with 100 lines it scrolls", s.scrollable(m), true);
		check_int("visible_lines = (200-8)/10", s.visible_lines(m), 19);
	}
	{
		ScrollView s;
		ScrollMetrics m = fits();
		check_dbl("3 lines: content", s.content_height(m), 38.);
		check_dbl("3 lines: max_y = 0", s.max_y(m), 0.);
		check_bool("3 lines: does not scroll", s.scrollable(m), false);
	}

	/* --- clamp ------------------------------------------------------------ */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		s.y = 5000.;
		s.clamp(m);
		check_dbl("clamp past the limit", s.y, 808.);
		s.y = -50.;
		s.clamp(m);
		check_dbl("clamp negativo", s.y, 0.);
		s.y = 123.;
		s.clamp(m);
		check_dbl("clamp inside: does not move", s.y, 123.);
	}
	{
		ScrollView s;
		ScrollMetrics m = fits();
		s.y = 40.;
		s.scroll_by(-10., m);
		check_dbl("no scroll: y stays 0", s.y, 0.);
	}

	/* --- visible lines ----------------------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		check_int("y=0: first_line", s.first_line(m), 0);
		check_int("y=0: last_line", s.last_line(m), 19);
		s.y = 100.;
		check_int("y=100: first_line", s.first_line(m), 9);
		check_int("y=100: last_line", s.last_line(m), 29);
		s.y = 801.;
		check_int("y=801: last_line does not go past the end", s.last_line(m), 99);
		s.y = 808.;
		check_int("y=808 (limit): last_line = 99", s.last_line(m), 99);
		check_int("y=808: first_line", s.first_line(m), 80);
	}
	{
		ScrollView s;
		ScrollMetrics m = fits();
		check_int("3 lines: last_line clamp", s.last_line(m), 2);
	}

	/* --- line_screen_y ---------------------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		check_dbl("y=0: line 0 at pad", s.line_screen_y(0, m), 4.);
		check_dbl("y=0: line 5", s.line_screen_y(5, m), 54.);
		s.y = 50.;
		check_dbl("y=50: line 5 at the top", s.line_screen_y(5, m), 4.);
	}

	/* --- scroll_by -------------------------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		s.scroll_by(50., m);
		check_dbl("scroll_by(+50) advances the content", s.y, 50.);
		s.scroll_by(-20., m);
		check_dbl("scroll_by(-20)", s.y, 30.);
		s.scroll_by(-1000., m);
		check_dbl("scroll_by does not pass the top limit", s.y, 0.);
		s.scroll_by(100000., m);
		check_dbl("scroll_by does not pass the bottom limit", s.y, 808.);
	}

	/* --- scroll_to_line --------------------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		s.scroll_to_line(30, m);
		check_dbl("scroll_to_line(30) = 30*height", s.y, 300.);
		check_dbl("scroll_to_line: line 30 lands at the top", s.line_screen_y(30, m), 4.);
		s.scroll_to_line(99, m);
		check_dbl("scroll_to_line(99) clamp to the top", s.y, 808.);
		s.scroll_to_line(-5, m);
		check_dbl("scroll_to_line(-5) = 0", s.y, 0.);
	}

	/* --- ensure_line (the caret rule) -------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		s.ensure_line(0, m);
		check_dbl("caret at 0: does not scroll", s.y, 0.);

		s.y = 0.;
		s.ensure_line(18, m);
		check_dbl("caret on the last whole line: does not scroll", s.y, 0.);

		s.y = 0.;
		s.ensure_line(19, m);
		check_dbl("line 19 (half visible): scrolls the minimum", s.y, 8.);
		check_dbl("  and line 19 fits entirely", s.line_screen_y(19, m), 186.);

		s.y = 0.;
		s.ensure_line(20, m);
		check_dbl("caret one line further down", s.y, 18.);
		check_dbl("  and line 20 fits entirely", s.line_screen_y(20, m), 186.);

		s.y = 318.;
		s.ensure_line(50, m);
		check_dbl("caret already visible: does not move", s.y, 318.);

		s.y = 318.;
		s.ensure_line(5, m);
		check_dbl("caret above: the line goes to the top", s.y, 50.);
		check_dbl("  and it fits entirely", s.line_screen_y(5, m), 4.);

		s.y = 318.;
		s.ensure_line(99, m);
		check_dbl("last line: clamp to the scroll top", s.y, 808.);
		check_dbl("  the last line fits", s.line_screen_y(99, m), 186.);
	}

	/* --- barra vertical --------------------------------------------------- */
	{
		ScrollView s;
		ScrollMetrics m = fits();
		ScrollView::Bar b = s.bar(m);
		check_bool("no scroll: bar hidden", b.visible, false);
	}
	{
		ScrollView s;
		ScrollMetrics m = tall();
		ScrollView::Bar b = s.bar(m);
		check_bool("with scroll: bar visible", b.visible, true);
		check_dbl("track = view - 2*pad", b.track_h, 192.);
		/* visible = 192 / total 1000 */
		check_dbl("thumb proporcional", b.thumb_h, 192. * 192. / 1000.);
		check_dbl("thumb at the top with y=0", b.thumb_y, 0.);

		s.y = 808.;
		b = s.bar(m);
		check_dbl("thumb at the bottom with y=max", b.thumb_y, 192. - b.thumb_h);
	}
	{
		/* Minimum thumb with huge documents. */
		ScrollView s;
		ScrollMetrics m = tall();
		m.line_count = 5000;
		m.line_h = 10.;
		s.y = 0.;
		ScrollView::Bar b = s.bar(m);
		check_dbl("thumb never smaller than the minimum", b.thumb_h, ScrollView::kMinThumb);
	}

	/* --- arrastre del thumb ------------------------------------------------ */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		s.drag_thumb_to(0., m);
		check_dbl("thumb drag to the top", s.y, 0.);
		ScrollView::Bar b = s.bar(m);
		s.drag_thumb_to(b.track_h - b.thumb_h, m);
		check_dbl("thumb drag to the bottom", s.y, 808.);
		s.drag_thumb_to(-100., m);
		check_dbl("drag outside the track (above)", s.y, 0.);
		s.drag_thumb_to(100000., m);
		check_dbl("drag outside the track (below)", s.y, 808.);
		s.drag_thumb_to(100000., m);
		b = s.bar(m);
		check_dbl("  and the thumb lands at the bottom", b.thumb_y, b.track_h - b.thumb_h);
	}

	/* --- ida y vuelta: thumb ↔ y ------------------------------------------ */
	{
		ScrollView s;
		ScrollMetrics m = tall();
		ScrollView::Bar b = s.bar(m);
		const double half = (b.track_h - b.thumb_h) / 2.;
		s.drag_thumb_to(half, m);
		check_dbl("half the track = half the content", s.y, 404.);
		ScrollView::Bar b2 = s.bar(m);
		check_dbl("  and the thumb goes back to the middle", b2.thumb_y, half);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
