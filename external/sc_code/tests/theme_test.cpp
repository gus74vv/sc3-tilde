// SPDX-License-Identifier: GPL-3.0-only
/* theme_test.cpp — the `@theme` presets (headless, the design notes).
 *
 * Pins down what is easy to break and expensive to look at in the GUI: every preset
 * carries its 21 roles, all of them in 0..1; the roles the document has to tell apart
 * are apart (the comment dimmer than the text; strings, keywords and numbers not the
 * text; the current line not the background); the translucent ones are translucent;
 * the factory palette is One Dark and the numbers of the table match the strings the
 * object uses as attribute defaults; and the name lookup forgets about case,
 * spaces, '-' and '_' and knows the aliases of each theme.
 *
 * Uso: ./build/theme_test [-v]
 */

#include "../sc_theme.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-58s got %s, esperado %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-58s %s\n", what, got ? "true" : "false");
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

static void check_str(const char *what, const char *got, const char *want) {
	++gChecks;
	const char *g = got ? got : "(null)";
	if (!got || std::strcmp(got, want) != 0) {
		++gFails;
		printf("  FAIL %-58s got \"%s\", esperado \"%s\"\n", what, g, want);
	} else if (gVerbose) {
		printf("  ok   %-58s \"%s\"\n", what, g);
	}
}

/* All the roles of a theme as an array: if a row of the table ever loses one, this is
 * what notices (the count is fixed in the header). */
static int roles_of(const Theme &t, const RGBA **out) {
	const RGBA *r[21] = {&t.bg,  &t.text,    &t.sel,    &t.caret,   &t.curline,  &t.keyword, &t.builtin,
						 &t.primitive, &t.klass, &t.symbol, &t.string,  &t.charlit,  &t.number,  &t.envvar,
						 &t.comment,   &t.bracket, &t.op,   &t.match,   &t.blink,    &t.bar,     &t.barhighlight};
	for (int i = 0; i < 21; ++i)
		out[i] = r[i];
	return 21;
}

static double sum(const RGBA &c) {
	return c.r + c.g + c.b;
}

/* How far the palette is from the default one, to prove that applying a theme really
 * changes the colors (and that no two rows are accidentally identical). */
static double distance(const Theme &a, const Theme &b) {
	const RGBA *ra[21], *rb[21];
	roles_of(a, ra);
	roles_of(b, rb);
	double d = 0.;
	for (int i = 0; i < 21; ++i)
		d += std::fabs(ra[i]->r - rb[i]->r) + std::fabs(ra[i]->g - rb[i]->g)
			 + std::fabs(ra[i]->b - rb[i]->b);
	return d;
}

static void test_table() {
	printf("tabla\n");
	const int n = theme_count();
	check_bool("theme_count() >= 6 (the doc promises one-dark + 5)", n >= 6, true);
	check_bool("theme_at(-1) is NULL", theme_at(-1) == NULL, true);
	check_bool("theme_at(n) is NULL", theme_at(n) == NULL, true);

	/* names are unique */
	for (int i = 0; i < n; ++i) {
		for (int j = i + 1; j < n; ++j) {
			const bool same_name = std::strcmp(theme_at(i)->name, theme_at(j)->name) == 0;
			check_bool("names are unique", !same_name, true);
			check_bool("no two palettes are identical", distance(*theme_at(i), *theme_at(j)) > 0.5, true);
		}
	}
	for (int i = 0; i < n; ++i)
		check_int("the canonical name of each row finds it back", theme_find(theme_at(i)->name) == theme_at(i), 1);
}

static void test_roles() {
	printf("roles\n");
	const int n = theme_count();
	for (int i = 0; i < n; ++i) {
		const Theme &t = *theme_at(i);
		const RGBA *c[21];
		check_int("21 roles per theme", roles_of(t, c), 21);
		bool in_range = true;
		for (int k = 0; k < 21; ++k) {
			const RGBA *p = c[k];
			if (!(p->r >= 0. && p->r <= 1. && p->g >= 0. && p->g <= 1. && p->b >= 0. && p->b <= 1.
				  && p->a >= 0. && p->a <= 1.))
				in_range = false;
		}
		check_bool("all roles in 0..1", in_range, true);
		check_bool("text is brighter than the background", sum(t.text) > sum(t.bg) + 0.8, true);
		check_bool("the comment is dimmer than the text", sum(t.comment) < sum(t.text), true);
		check_bool("the comment is still visible on the background", sum(t.comment) > sum(t.bg) + 0.15, true);
		check_bool("strings differ from the text", t.string.r != t.text.r || t.string.g != t.text.g, true);
		check_bool("keywords differ from the text", t.keyword.r != t.text.r || t.keyword.g != t.text.g, true);
		check_bool("numbers differ from the text", t.number.r != t.text.r || t.number.g != t.text.g, true);
		check_bool("brackets differ from the background",
				   t.bracket.r != t.bg.r || t.bracket.g != t.bg.g || t.bracket.b != t.bg.b, true);
		/* the band has to be seen: the current line cannot be the background */
		check_bool("curline differs from bg", t.curline.r != t.bg.r || t.curline.g != t.bg.g || t.curline.b != t.bg.b,
				   true);
		/* the ones drawn OVER the text are translucent by definition */
		check_bool("match is translucent", t.match.a > 0. && t.match.a < 1., true);
		check_bool("blink is translucent", t.blink.a > 0. && t.blink.a < 1., true);
		check_bool("caret is opaque", t.caret.a == 1., true);
		check_bool("bar is darker than the background", sum(t.bar) < sum(t.bg), true);
	}
}

static void test_default() {
	printf("default\n");
	const Theme *d = theme_default();
	check_bool("there is a default theme", d != NULL, true);
	check_str("the factory palette is One Dark", d ? d->name : NULL, "onedark");
	check_bool("theme_default() is theme_at(0)", d == theme_at(0), true);
	/* the table parses the SAME strings ext_main uses as attribute defaults */
	check_bool("bg parses the macro (0.156863 0.172549 0.203922 1.)",
			   std::fabs(d->bg.r - 0.156863) < 1e-5 && std::fabs(d->bg.g - 0.172549) < 1e-5
				   && std::fabs(d->bg.b - 0.203922) < 1e-5 && d->bg.a == 1.,
			   true);
	check_bool("comment parses the macro (0.360784 0.388235 0.439216)",
			   std::fabs(d->comment.r - 0.360784) < 1e-5 && std::fabs(d->comment.g - 0.388235) < 1e-5
				   && std::fabs(d->comment.b - 0.439216) < 1e-5,
			   true);
	check_bool("the match alpha of the macro is 0.3", std::fabs(d->match.a - 0.3) < 1e-6, true);
}

static void test_find() {
	printf("busqueda\n");
	const Theme *od = theme_find("onedark");
	check_bool("onedark exists", od != NULL, true);
	check_bool("One Dark == onedark", theme_find("One Dark") == od, true);
	check_bool("ONE_DARK == onedark", theme_find("ONE_DARK") == od, true);
	check_bool("one-dark == onedark", theme_find("one-dark") == od, true);
	check_bool("atom == onedark (alias)", theme_find("atom") == od, true);
	const Theme *cat = theme_find("catppuccin");
	check_bool("catppuccin exists", cat != NULL, true);
	check_bool("mocha == catppuccin (alias)", theme_find("mocha") == cat, true);
	check_bool("Catppuccin Mocha == catppuccin", theme_find("Catppuccin Mocha") == cat, true);
	check_bool("Catppuccin-Mocha == catppuccin", theme_find("Catppuccin-Mocha") == cat, true);
	check_bool("dracula exists", theme_find("dracula") != NULL, true);
	check_bool("Dracula (capitalized) works", theme_find("Dracula") == theme_find("dracula"), true);
	check_bool("nord exists", theme_find("nord") != NULL, true);
	check_bool("gruvbox exists", theme_find("gruvbox") != NULL, true);
	check_bool("gruvbox-dark == gruvbox (alias)", theme_find("gruvbox-dark") == theme_find("gruvbox"), true);
	check_bool("monokai exists", theme_find("monokai") != NULL, true);
	check_bool("solarized exists", theme_find("solarized") != NULL, true);
	check_bool("Solarized Dark == solarized", theme_find("Solarized Dark") == theme_find("solarized"), true);
	check_bool("solarised (british) == solarized", theme_find("solarised") == theme_find("solarized"), true);
	/* nothing is NOT a theme */
	check_bool("unknown name is NULL", theme_find("no-such-theme") == NULL, true);
	check_bool("NULL is NULL", theme_find(NULL) == NULL, true);
	check_bool("empty is NULL", theme_find("") == NULL, true);
	check_bool("separators only is NULL", theme_find(" -_ ") == NULL, true);
	check_bool("a prefix is not a match", theme_find("one") == NULL, true);
	check_bool("a suffix is not a match", theme_find("onedarkk") == NULL, true);
}

static void test_aliases() {
	printf("aliases\n");
	/* every alias of every row resolves to its own row (the table declares them
	 * normalized, space separated) */
	for (int i = 0; i < theme_count(); ++i) {
		const Theme &t = *theme_at(i);
		const char *a = t.aliases;
		while (a && *a) {
			while (*a == ' ')
				++a;
			char tok[32];
			int j = 0;
			while (*a && *a != ' ' && j < (int)sizeof(tok) - 1)
				tok[j++] = *a++;
			tok[j] = 0;
			if (j == 0)
				break;
			check_bool("alias of the row finds the row", theme_find(tok) == &t, true);
			/* and it does not shadow another row */
			check_str("the alias resolves to its own name", theme_find(tok)->name, t.name);
		}
	}
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (std::strcmp(argv[i], "-v") == 0)
			gVerbose = true;

	printf("[theme_test]\n");
	test_table();
	test_roles();
	test_default();
	test_find();
	test_aliases();

	printf("OK: %d checks, %d failures\n", gChecks, gFails);
	return gFails == 0 ? 0 : 1;
}
