// SPDX-License-Identifier: GPL-3.0-only
/* blink_test.cpp — gate of the `sc_code` evaluation blink (headless, S5e).
 *
 * Pins down what the user asked for and what the IDE does: (1)
 * default duration 500 ms and **configurable** (`@blinkduration`), (2) that it is a
 * blink (starts visible and disappears by itself), with the IDE InCubic curve
 * (`QEasingCurve::InCubic` over opacity 1 → 0), (3) that the range is the evaluated
 * block, as is (trailing comment included). The color is not tested here (it is an
 * RGBA attribute of the Max object).
 *
 *     GPL v3 or later (ver sc_blink.cpp).
 *
 * Uso: ./build/blink_test [-v]
 */

#include "../sc_blink.h"

#include <cmath>
#include <cstdio>
#include <cstring>

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

static void check_dbl(const char *what, double got, double want) {
	++gChecks;
	if (std::fabs(got - want) > 1e-9) {
		++gFails;
		printf("  FAIL %-60s got %g, expected %g\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-60s %g\n", what, got);
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-60s got %s, expected %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-60s %s\n", what, got ? "true" : "false");
	}
}

/* IDE InCubic curve: alpha = 1 - t³ with t = elapsed/duration. */
static double cubic(double t) { return 1. - t * t * t; }

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (!strcmp(argv[i], "-v"))
			gVerbose = true;

	printf("Blink: estado y rango\n");
	Blink b;
	check_bool("no start: inactive", b.active(), false);
	check_dbl("no start: alpha 0", b.alpha(0.), 0.);
	check_bool("no start: expired", b.expired(0.), true);
	check_dbl("default duration = 500 ms", b.duration(), 500.);
	check_dbl("the module default is 500", Blink::kDefaultDuration, 500.);
	check_dbl("paso de repintado 25 ms", Blink::kStep, 25.);

	/* Range of the evaluated block: byte offsets of the buffer
	 * (inclusive/exclusive). In the line fallback it is the WHOLE block: if there
	 * is a trailing `//`, it is included (that is what the IDE does, sc_editor.cpp
	 * + overlay.cpp; nothing is trimmed). */
	b.start(100, 250, 1000.);
	check_bool("start: activo", b.active(), true);
	check_int("start: offset inicial", b.start_off(), 100);
	check_int("start: offset final (comentario final incluido)", b.end_off(), 250);

	printf("Blink: curve (InCubic, like the IDE)\n");
	check_dbl("t=0: alpha 1 (visible instantly)", b.alpha(1000.), 1.);
	check_dbl("t=100/500: alpha 1-0.2³", b.alpha(1100.), cubic(0.2));
	check_dbl("t=250/500: alpha 1-0.5³ = 0.875", b.alpha(1250.), 0.875);
	check_dbl("t=400/500: alpha 1-0.8³", b.alpha(1400.), cubic(0.8));
	check_bool("t=499: not expired yet", b.expired(1499.), false);
	check_dbl("t=500: alpha 0 (finished)", b.alpha(1500.), 0.);
	check_bool("t=500: expired (the duration is 500 ms)", b.expired(1500.), true);
	check_dbl("t=5000: still 0", b.alpha(6000.), 0.);

	printf("Blink: configurable duration (@blinkduration)\n");
	b.set_duration(1000.);
	check_dbl("set 1000: duration()", b.duration(), 1000.);
	/* the clock of the blink in progress uses the NEW duration */
	check_dbl("same start, 400 ms ⇒ t=0.4", b.alpha(1400.), cubic(0.4));
	check_bool("at 400 ms it has not expired with 1000", b.expired(1400.), false);
	check_bool("at 1000 ms it has", b.expired(2000.), true);
	b.set_duration(100.);
	check_dbl("set 100: duration()", b.duration(), 100.);
	check_bool("100 ms: expirado a 150 ms", b.expired(1150.), true);

	b.set_duration(0.);
	check_dbl("set 0: falls to the minimum (50)", b.duration(), 50.);
	b.set_duration(-5.);
	check_dbl("negative set: minimum (50)", b.duration(), 50.);
	b.set_duration(1e9);
	check_dbl("huge set: maximum (5000)", b.duration(), 5000.);
	b.set_duration(NAN);
	check_dbl("NaN set: minimum (50)", b.duration(), 50.);
	b.set_duration(400.);

	printf("Blink: stop / reinicio\n");
	b.stop();
	check_bool("stop: inactivo", b.active(), false);
	check_dbl("stop: alpha 0", b.alpha(1100.), 0.);

	b.start(10, 10, 0.);
	check_bool("empty range: does not start", b.active(), false);
	b.start(20, 5, 0.);
	check_bool("inverted range: does not start", b.active(), false);

	b.start(0, 10, 500.);
	b.start(30, 40, 700.);
	check_int("re-eval: new range", b.start_off(), 30);
	check_dbl("re-eval: reloj reiniciado", b.alpha(700.), 1.);
	check_dbl("re-eval: 400 ms from the new start", b.alpha(1100.), 0.);

	/* Zeroed memory (object_alloc): mDuration == 0 ⇒ the default is used. */
	Blink zeroed = Blink();
	check_dbl("freshly built Blink(): 500 ms", zeroed.duration(), 500.);

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
