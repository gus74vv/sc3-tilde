// SPDX-License-Identifier: GPL-3.0-only
/* buffer_test.cpp — S1 gate: UTF-8 buffer, lines, caret/selection and
 * undo.
 *
 * Decides the design notes table without a GUI: if any of this fails, in the
 * patch it looks like \"the caret jumps\", \"⌫ eats two characters\" or \"undo
 * blows up\", which are the most expensive bugs to diagnose by clicking.
 *
 *     GPL v3 or later (ver sc_buffer.cpp).
 *
 * Uso: ./build/buffer_test [-v]
 */

#include "../sc_buffer.h"

#include <cstdio>
#include <string>

using sc_code::Buffer;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_int(const char *what, int got, int want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-52s got %d, expected %d\n", what, got, want);
	} else if (gVerbose) {
		printf("  ok   %-52s %d\n", what, got);
	}
}

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-52s got \"%s\", expected \"%s\"\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-52s \"%s\"\n", what, got.c_str());
	}
}

static void check_bool(const char *what, bool got, bool want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-52s got %s, expected %s\n", what, got ? "true" : "false", want ? "true" : "false");
	} else if (gVerbose) {
		printf("  ok   %-52s %s\n", what, got ? "true" : "false");
	}
}

/* Observable buffer state: text + caret + anchor (for undo/redo). */
static void check_state(const char *what, const Buffer &b, const std::string &text, int caret, int anchor) {
	check_str(what, b.text(), text);
	check_int("  caret", b.caret(), caret);
	check_int("  anchor", b.anchor(), anchor);
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (std::string(argv[i]) == "-v")
			gVerbose = true;

	printf("buffer_test — UTF-8 buffer of the sc_code editor (S1)\n");

	/* --- 1) empty document -------------------------------------------------- */
	{
		Buffer b;
		check_int("empty: line_count", b.line_count(), 1);
		check_int("empty: line_length(0)", b.line_length(0), 0);
		check_int("empty: length", b.length(), 0);
		check_int("empty: caret", b.caret(), 0);
		check_bool("empty: has_selection", b.has_selection(), false);
		check_bool("empty: backspace changes nothing", b.backspace(), false);
		check_bool("empty: delete_forward changes nothing", b.delete_forward(), false);
		check_bool("empty: can_undo", b.can_undo(), false);
	}

	/* --- 2) line index ----------------------------------------------------- */
	{
		const char *text = "a\nbb\n\nccc"; /* líneas: "a" "bb" "" "ccc" */
		Buffer b(text);
		check_int("lines: line_count", b.line_count(), 4);
		check_int("lines: start 0", b.line_start(0), 0);
		check_int("lines: start 1", b.line_start(1), 2);
		check_int("lines: start 2", b.line_start(2), 5);
		check_int("lines: start 3", b.line_start(3), 6);
		check_int("lines: length 0 (no \\n)", b.line_length(0), 1);
		check_int("lines: length 1", b.line_length(1), 2);
		check_int("lines: length 2 (empty)", b.line_length(2), 0);
		check_int("lines: length 3", b.line_length(3), 3);
		check_int("lines: end 3 == length", b.line_end(3), b.length());
		check_str("lines: line_text(1)", b.line_text(1), "bb");
		check_int("lines: line_of_offset(0)", b.line_of_offset(0), 0);
		check_int("lines: line_of_offset(1) = the '\\n'", b.line_of_offset(1), 0);
		check_int("lines: line_of_offset(2)", b.line_of_offset(2), 1);
		check_int("lines: line_of_offset(5)", b.line_of_offset(5), 2);
		check_int("lines: line_of_offset(6)", b.line_of_offset(6), 3);
		check_int("lines: line_of_offset(9) (EOF)", b.line_of_offset(9), 3);
		check_int("lines: line_of_offset(-5) (clamp)", b.line_of_offset(-5), 0);
		check_int("lines: col_of_offset(7)", b.col_of_offset(7), 1);
		check_int("lines: offset_of(1,1)", b.offset_of(1, 1), 3);
		check_int("lines: offset_of(1,99) (clamp)", b.offset_of(1, 99), 4);
		check_int("lines: offset_of(99,0) (clamp)", b.offset_of(99, 0), 6);
		check_int("lines: line_start(-3) (clamp)", b.line_start(-3), 0);
	}

	/* --- 3) document ending in '\n' ⇒ empty virtual line (QTextDocument) */
	{
		Buffer b("a\n");
		check_int("trailing \\n: line_count", b.line_count(), 2);
		check_int("trailing \\n: start(1)", b.line_start(1), 2);
		check_int("trailing \\n: length(1)", b.line_length(1), 0);
		check_int("trailing \\n: line_of_offset(2)", b.line_of_offset(2), 1);
		check_int("trailing \\n: line_of_offset(1) (the '\\n')", b.line_of_offset(1), 0);
	}

	/* --- 4) insert_text / selection ---------------------------------------- */
	{
		Buffer b;
		check_bool("insert_text(\"hola\")", b.insert_text("hola"), true);
		check_state("insert_text: estado", b, "hola", 4, 4);
		check_bool("insert_text a mitad", (b.set_caret(2), b.insert_text("XY")), true);
		check_state("insert_text: state in the middle", b, "hoXYla", 4, 4);

		Buffer c("abcdef");
		c.set_selection(1, 4); /* "bcd" */
		check_str("selection: selected_text", c.selected_text(), "bcd");
		check_bool("selection: insert replaces", c.insert_text("X"), true);
		check_state("selection: replacement", c, "aXef", 2, 2);
		check_int("selection: replacement = 1 undo step", c.undo_depth(), 1);
		check_bool("selection: undo", c.undo(), true);
		check_state("selection: undo restores text+selection", c, "abcdef", 4, 1);

		c.set_selection(4, 1); /* invertida */
		check_str("reversed selection: selected_text", c.selected_text(), "bcd");
		check_int("reversed selection: start", c.selection_start(), 1);
		check_int("reversed selection: end", c.selection_end(), 4);
		check_bool("reversed selection: delete_selection", c.delete_selection(), true);
		check_state("reversed selection: deleted", c, "aef", 1, 1);

		Buffer d("hola mundo");
		d.select_all();
		check_str("select_all", d.selected_text(), "hola mundo");
		check_bool("select_all: insert over everything", d.insert_text("chau"), true);
		check_state("select_all: replacement", d, "chau", 4, 4);
	}

	/* --- 5) insert_char y UTF-8 -------------------------------------------- */
	{
		Buffer b;
		check_bool("insert_char('a')", b.insert_char('a'), true);
		check_int("insert_char('a'): caret", b.caret(), 1);
		check_bool("insert_char(0xE9 = 'é')", b.insert_char(0xE9), true);
		check_str("insert_char: text", b.text(), "a\xC3\xA9");
		check_int("insert_char('é'): caret (2 bytes)", b.caret(), 3);
		check_bool("insert_char(0x1F600 = emoji)", b.insert_char(0x1F600), true);
		check_int("insert_char(emoji): caret (+4 bytes)", b.caret(), 7);
		check_str("insert_char(emoji): text", b.text(), "a\xC3\xA9\xF0\x9F\x98\x80");

		b.move_left();
		check_int("move_left over emoji: skips 1 code point", b.caret(), 3);
		b.move_left();
		check_int("move_left over 'é': skips 2 bytes", b.caret(), 1);
		b.move_right();
		check_int("move_right over 'é'", b.caret(), 3);

		check_int("next_codepoint(0)", Buffer::next_codepoint(b.text(), 0), 1);
		check_int("next_codepoint(1)", Buffer::next_codepoint(b.text(), 1), 3);
		check_int("next_codepoint(3)", Buffer::next_codepoint(b.text(), 3), 7);
		check_int("next_codepoint(7) (EOF)", Buffer::next_codepoint(b.text(), 7), 7);
		check_int("prev_codepoint(7)", Buffer::prev_codepoint(b.text(), 7), 3);
		check_int("prev_codepoint(1)", Buffer::prev_codepoint(b.text(), 1), 0);
		check_str("encode_utf8(0xE9)", Buffer::encode_utf8(0xE9), "\xC3\xA9");

		/* Never split a code point: offset 2 falls on a continuation byte. */
		Buffer e("a\xC3\xA9reo");
		e.set_caret(2);
		check_int("set_caret mid 'é' ⇒ 1", e.caret(), 1);
		check_int("offset_of with col mid 'é' ⇒ 1", e.offset_of(0, 2), 1);
		e.set_caret(6);
		check_bool("backspace deletes the whole 'é'", (e.set_caret(3), e.backspace()), true);
		check_str("backspace: text", e.text(), "areo");
		check_int("backspace: caret", e.caret(), 1);

		/* VISUAL column (code points) vs column in BYTES (the lexer one). */
		Buffer v("a\xC3\xA9reo"); /* a-é-r-e-o: 6 bytes, 5 code points */
		check_int("display_col of the caret at 0", v.display_col_of_offset(0), 0);
		check_int("display_col after 'a'", v.display_col_of_offset(1), 1);
		check_int("display_col after 'é' (3 bytes)", v.display_col_of_offset(3), 2);
		check_int("col_of_offset after 'é' (bytes)", v.col_of_offset(3), 3);
		check_int("display_col al final", v.display_col_of_offset(6), 5);
		check_int("offset_of_display_col(0,2) = after 'é'", v.offset_of_display_col(0, 2), 3);
		check_int("offset_of_display_col(0,0)", v.offset_of_display_col(0, 0), 0);
		check_int("offset_of_display_col clamp al final", v.offset_of_display_col(0, 99), 6);
		check_int("offset_of_display_col(0,3) lands on 'e' (does not split 'é')", v.offset_of_display_col(0, 3), 4);
		/* round trip at every code point boundary */
		for (int cp = 0; cp <= 5; ++cp) {
			const int off = v.offset_of_display_col(0, cp);
			check_int("ida y vuelta display col", v.display_col_of_offset(off), cp);
		}
	}

	/* --- 6) ⌫ / ⌦ ---------------------------------------------------------- */
	{
		Buffer b("abc");
		b.set_caret(2);
		check_bool("backspace in the middle", b.backspace(), true);
		check_state("backspace: estado", b, "ac", 1, 1);
		check_bool("delete_forward in the middle", b.delete_forward(), true);
		check_state("delete_forward: estado", b, "a", 1, 1);
		check_bool("delete_forward at the end does nothing", b.delete_forward(), false);
		check_bool("delete_forward when empty", (b.set_caret(0), b.delete_forward()), true);
		check_str("delete_forward: text", b.text(), "");
		check_bool("backspace at the start does nothing", b.backspace(), false);

		/* With a selection, both delete the selection (not one character
		 * more). */
		Buffer c("abcdef");
		c.set_selection(1, 3);
		check_bool("backspace with selection", c.backspace(), true);
		check_state("backspace with selection: state", c, "adef", 1, 1);
		Buffer d("abcdef");
		d.set_selection(1, 3);
		check_bool("delete_forward with selection", d.delete_forward(), true);
		check_state("delete_forward with selection: state", d, "adef", 1, 1);
	}

	/* --- 7) movimiento vertical con columna deseada ------------------------ */
	{
		Buffer b("abcd\nab\nabcdef"); /* starts 0, 5, 8 */
		b.set_caret(3);               /* línea 0, col 3 */
		b.move_down();
		check_int("↓ to short line: clamp to col 2", b.caret(), 7);
		b.move_down();
		check_int("↓ again: restores col 3", b.caret(), 11);
		b.move_up();
		check_int("↑ to short line: clamp to col 2", b.caret(), 7);
		b.move_up();
		check_int("↑ again: restores col 3", b.caret(), 3);
		b.move_up();
		check_int("↑ on the first line: stays", b.caret(), 3);

		/* A horizontal movement cancels the desired column. */
		b.set_caret(3);
		b.move_down();  /* caret 7, columna deseada 3 */
		b.move_left();  /* caret 6, col 1 ⇒ the desired column is recomputed */
		b.move_down();
		check_int("desired column is recomputed after ←/→", b.caret(), 9);

		/* ⇧↓ extends the selection. */
		Buffer c("abc\ndef");
		c.set_caret(1);
		c.move_down(true);
		check_int("⇧↓: caret", c.caret(), 5);
		check_int("⇧↓: anchor", c.anchor(), 1);
		check_str("⇧↓: selection", c.selected_text(), "bc\nd");
	}

	/* --- 8) Home/End/⌘↑/⌘↓/⌥←/⌥→ ------------------------------------------ */
	{
		Buffer b("hola mundo\nsegunda linea");
		b.set_caret(7);
		b.move_line_start();
		check_int("⌘←: start of line", b.caret(), 0);
		b.move_line_end();
		check_int("⌘→: end of line", b.caret(), 10);
		b.move_right();
		check_int("→ crosses the '\\n'", b.caret(), 11);
		b.move_word_right();
		check_int("⌥→: end of 'segunda'", b.caret(), 18);
		b.move_word_left();
		check_int("⌥←: start of 'segunda'", b.caret(), 11);
		b.move_word_left();
		check_int("⌥← to the previous word", b.caret(), 5);
		b.move_document_end();
		check_int("⌘↓: end of document", b.caret(), b.length());
		b.move_document_start();
		check_int("⌘↑: start of document", b.caret(), 0);

		/* Words with sclang punctuation. */
		Buffer c("~foo = SinOsc.ar(440);");
		c.move_document_end();
		c.move_word_left();
		check_int("⌥← before ')' (skips ';' and ')')", c.caret(), 17);
		c.move_document_start();
		c.move_word_right();
		check_int("⌥→ over '~foo'", c.caret(), 4);
		c.move_word_right();
		check_int("⌥→ skips '=' and spaces", c.caret(), 13);
	}

	/* --- 9) Tab = espacios / ⇧Tab ------------------------------------------ */
	{
		Buffer b;
		check_bool("insert_tab(4)", b.insert_tab(4), true);
		check_state("insert_tab: estado", b, "    ", 4, 4);

		Buffer c("    x");
		c.set_caret(5);
		check_bool("outdent(4)", c.outdent(4), true);
		check_state("outdent: estado", c, "x", 1, 1);
		check_bool("outdent without spaces does nothing", c.outdent(4), false);

		Buffer d("  dos espacios");
		d.set_caret(3);
		check_bool("outdent(4) removes only 2", d.outdent(4), true);
		check_str("outdent(4): text", d.text(), "dos espacios");
		check_int("outdent(4): caret", d.caret(), 1);

		/* outdent only looks at the start of the caret line. */
		Buffer e("    uno\n    dos");
		e.set_caret(13);
		check_bool("outdent on the 2nd line", e.outdent(4), true);
		check_str("outdent: only the caret line", e.text(), "    uno\ndos");
		check_int("outdent: caret", e.caret(), 9);
	}

	/* --- 10) undo / redo --------------------------------------------------- */
	{
		Buffer b("abc");
		b.set_caret(3);
		check_bool("undo with no history", b.undo(), false);
		check_bool("redo with no history", b.redo(), false);

		/* Typing burst = ONE step (per-burst coalescing). */
		b.insert_char('d');
		b.insert_char('e');
		b.insert_char('f');
		check_str("burst: text", b.text(), "abcdef");
		check_int("burst: a single undo step", b.undo_depth(), 1);
		check_bool("burst: undo", b.undo(), true);
		check_state("burst: undo state", b, "abc", 3, 3);
		check_bool("burst: redo", b.redo(), true);
		check_state("burst: redo state", b, "abcdef", 6, 6);

		/* A movement cuts the burst. */
		b.set_caret(0);
		b.insert_char('X');
		check_int("movement breaks the burst: 2 steps", b.undo_depth(), 2);
		check_bool("undo of the 2nd", b.undo(), true);
		check_str("undo of the 2nd: text", b.text(), "abcdef");

		/* Return (text with '\n') does not coalesce with typing. */
		Buffer c;
		c.insert_text("a");
		c.insert_text("\n");
		c.insert_text("b");
		check_int("Return breaks the burst: 3 steps", c.undo_depth(), 3);
		check_str("Return: text", c.text(), "a\nb");
		c.undo();
		c.undo();
		check_str("Return: 2 undos", c.text(), "a");
		check_int("Return: line_count after undo", c.line_count(), 1);

		/* A run of ⌫ = one step. */
		Buffer d("abcdef");
		d.set_caret(6);
		d.backspace();
		d.backspace();
		d.backspace();
		check_int("⌫ mantenido: 1 paso", d.undo_depth(), 1);
		check_str("⌫ held: text", d.text(), "abc");
		d.undo();
		check_str("⌫ mantenido: undo", d.text(), "abcdef");
		check_int("⌫ mantenido: caret restaurado", d.caret(), 6);

		/* Undo/redo restores the selection. */
		Buffer e("uno dos");
		e.set_selection(4, 7);
		e.delete_selection();
		check_state("delete selection", e, "uno ", 4, 4);
		e.undo();
		check_state("undo restores the selection", e, "uno dos", 7, 4);

		/* 50-level limit: every Return is its own step. */
		Buffer f;
		for (int i = 0; i < 60; ++i)
			f.insert_text("\n");
		check_int("undo limit: depth 50", f.undo_depth(), 50);
		int undos = 0;
		while (f.undo())
			++undos;
		check_int("undo limit: 50 undos possible", undos, 50);
		check_int("undo limit: remaining text", (int)f.text().size(), 10);
		check_bool("undo limit: no more", f.can_undo(), false);
	}

	/* --- 12) ranges for the mouse (double/triple click) --------------------- */
	{
		Buffer b("~foo = SinOsc.ar(440);");
		int a, e;
		b.word_range_at(10, a, e); /* 'S' de SinOsc */
		check_str("word_range_at('SinOsc')", b.text().substr((size_t)a, (size_t)(e - a)), "SinOsc");
		b.word_range_at(15, a, e); /* 'a' de ar */
		check_str("word_range_at('ar')", b.text().substr((size_t)a, (size_t)(e - a)), "ar");
		b.word_range_at(17, a, e); /* '4' de 440 */
		check_str("word_range_at('440')", b.text().substr((size_t)a, (size_t)(e - a)), "440");
		b.word_range_at(0, a, e); /* '~' is punctuation: only the '~' gets selected */
		check_str("word_range_at('~')", b.text().substr((size_t)a, (size_t)(e - a)), "~");
		b.word_range_at(5, a, e); /* '=' surrounded by spaces: only the '=' */
		check_str("word_range_at('=')", b.text().substr((size_t)a, (size_t)(e - a)), "=");
		b.word_range_at(4, a, e); /* space: the run of spaces */
		check_str("word_range_at(space)", b.text().substr((size_t)a, (size_t)(e - a)), " ");
		b.word_range_at(16, a, e); /* '(' is punctuation */
		check_str("word_range_at('(')", b.text().substr((size_t)a, (size_t)(e - a)), "(");

		/* At the end of the document after a word: selects it whole. */
		Buffer g("foo = bar");
		g.word_range_at(9, a, e); /* == length */
		check_str("word_range_at(EOF after a word)", g.text().substr((size_t)a, (size_t)(e - a)), "bar");
		g.word_range_at(0, a, e);
		check_str("word_range_at(at the start)", g.text().substr((size_t)a, (size_t)(e - a)), "foo");

		/* On the '\n' with a word before ⇒ that word (end of line). */
		Buffer c("uno\ndos");
		c.word_range_at(3, a, e);
		check_int("word_range_at('\\n'): start", a, 0);
		check_int("word_range_at('\\n'): end", e, 3);
		c.word_range_at(5, a, e);
		check_str("word_range_at('dos')", c.text().substr((size_t)a, (size_t)(e - a)), "dos");
		c.word_range_at(4, a, e); /* line 1 empty before 'dos'... no: 'd' */
		check_str("word_range_at('d')", c.text().substr((size_t)a, (size_t)(e - a)), "dos");

		/* Empty line: double click on the '\n' of an empty line ⇒ empty range. */
		Buffer h("a\n\nb");
		h.word_range_at(2, a, e);
		check_int("word_range_at('\\n' of empty line): start", a, 2);
		check_int("word_range_at('\\n' of empty line): end", e, 2);

		/* Triple click: the line without the '\n'. */
		c.line_range_at(5, a, e);
		check_int("line_range_at: start", a, 4);
		check_int("line_range_at: end (no '\\n')", e, 7);
		c.line_range_at(3, a, e); /* on the '\n' ⇒ line 0 */
		check_str("line_range_at over '\\n'", c.text().substr((size_t)a, (size_t)(e - a)), "uno");

		/* Multibyte: double click on 'é' selects the whole code point. */
		Buffer d("a\xC3\xA9" "b"); /* a é b */
		d.word_range_at(1, a, e);
		check_int("word_range_at('é'): start", a, 1);
		check_int("word_range_at('é'): end", e, 3);

		/* And the range is selectable/deletable like any other. */
		Buffer f("hola mundo");
		f.word_range_at(7, a, e);
		f.set_selection(a, e);
		check_str("doble click + set_selection", f.selected_text(), "mundo");
		check_bool("doble click + backspace", f.backspace(), true);
		check_str("double click + backspace: text", f.text(), "hola ");
	}

	/* --- 11) line cache is invalidated on edit ----------------------------- */
	{
		Buffer b("a");
		check_int("cache: line_count inicial", b.line_count(), 1);
		b.insert_text("\nb\nc");
		check_int("cache: line_count after editing", b.line_count(), 3);
		check_int("cache: line_start(2)", b.line_start(2), 3);
		check_str("cache: line_text(2)", b.line_text(2), "ca");
		b.undo();
		check_int("cache: line_count after undo", b.line_count(), 1);
		b.redo();
		check_int("cache: line_count after redo", b.line_count(), 3);
		b.set_text("x\ny");
		check_int("cache: set_text", b.line_count(), 2);
		check_int("cache: set_text caret", b.caret(), 0);
		check_bool("cache: set_text limpia undo", b.can_undo(), false);
	}

	/* --- 13) auto-close of pairs (S3, port of insertMatchingTokens) -------- */
	{
		/* Default OFF: the SC IDE also has it off by default. */
		Buffer off;
		check_bool("auto-close: default off", off.auto_close(), false);
		off.insert_char('(');
		check_str("auto-close off: '(' alone", "(", off.text());

		Buffer b;
		b.set_auto_close(true);
		check_bool("auto-close: on", b.auto_close(), true);
		b.insert_char('(');
		check_state("auto-close '(': state", b, "()", 1, 1);
		b.insert_char('x');
		check_state("auto-close: typed inside", b, "(x)", 2, 2);
		/* Type-over: an ')' that was already there is NOT
		 * duplicated. */
		const int depth = b.undo_depth();
		b.insert_char(')');
		check_state("auto-close: type-over ')'", b, "(x)", 3, 3);
		check_int("auto-close: type-over adds no undo", b.undo_depth(), depth);

		Buffer c;
		c.set_auto_close(true);
		c.insert_char('{');
		check_state("auto-close '{'", c, "{}", 1, 1);
		c.insert_char('[');
		check_state("auto-close '['", c, "{[]}", 2, 2);
		c.insert_char('"');
		check_state("auto-close '\"'", c, "{[\"\"]}", 3, 3);
		c.insert_char('\''); /* symbol de sclang */
		check_state("auto-close '\\''", c, "{[\"''\"]}", 4, 4);

		/* ')' with no pair before it: inserted normally. */
		Buffer d;
		d.set_auto_close(true);
		d.insert_char(')');
		check_state("auto-close: lone ')'", d, ")", 1, 1);

		/* With a selection: wraps it and leaves the selection
		 * inside. */
		Buffer e("abc");
		e.set_auto_close(true);
		e.set_selection(1, 2); /* "b" */
		check_bool("auto-close: wraps selection", e.insert_char('('), true);
		check_str("auto-close: wraps (text)", e.text(), "a(b)c");
		check_int("auto-close: wraps (anchor)", e.anchor(), 2);
		check_int("auto-close: wraps (caret)", e.caret(), 3);
		check_str("auto-close: wraps (selection)", e.selected_text(), "b");

		/* With a selection and a CLOSER: replaces the selection (does not
		 * wrap). */
		Buffer f("abc");
		f.set_auto_close(true);
		f.set_selection(1, 3);
		f.insert_char(')');
		check_state("auto-close: close over selection", f, "a)", 2, 2);

		/* ⌫ between an empty pair deletes both. */
		Buffer g;
		g.set_auto_close(true);
		g.insert_char('(');
		check_bool("auto-close: ⌫ of empty pair", g.backspace(), true);
		check_state("auto-close: ⌫ of empty pair (state)", g, "", 0, 0);

		/* ⌫ with content inside deletes one character. */
		Buffer h;
		h.set_auto_close(true);
		h.insert_char('(');
		h.insert_char('x');
		check_bool("auto-close: ⌫ with content", h.backspace(), true);
		check_state("auto-close: ⌫ with content (state)", h, "()", 1, 1);

		/* ⌫ with auto-close OFF deletes only the '(' . */
		Buffer i;
		i.insert_char('(');
		i.insert_char(')');
		i.set_caret(1);
		check_bool("auto-close off: ⌫ deletes one", i.backspace(), true);
		check_state("auto-close off: ⌫ (state)", i, ")", 0, 0);

		/* The pair is ONE undo step; typing inside is another. */
		Buffer j;
		j.set_auto_close(true);
		j.insert_char('{');
		check_int("auto-close: 1 undo step for the pair", j.undo_depth(), 1);
		j.undo();
		check_str("auto-close: undo of the pair", j.text(), "");
	}

	/* --- 14) Return con autoindent (S3) ----------------------------------- */
	{
		/* Unindented line ⇒ bare newline. */
		Buffer a("foo");
		a.set_caret(3);
		check_bool("autoindent: Return in 'foo'", a.insert_newline(""), true);
		check_state("autoindent: 'foo' + Return", a, "foo\n", 4, 4);

		/* Copies the indentation (spaces) of the current line. */
		Buffer b("    foo");
		b.set_caret(7);
		b.insert_newline("    ");
		check_state("autoindent: keeps 4 spaces", b, "    foo\n    ", 12, 12);

		/* After an opener: one level more. */
		Buffer c; /* auto-close for the real case of typing '{' */
		c.set_auto_close(true);
		c.insert_char('{');
		c.insert_newline("    ");
		check_state("autoindent: '{' + Return", c, "{\n    \n}", 6, 6);

		/* With the closer right ahead: the closer goes to its own line (base indent). */
		Buffer d("  {");
		d.set_caret(3);
		d.insert_newline("      ");
		check_state("autoindent: inside an indented block", d, "  {\n      ", 10, 10);

		Buffer e("foo(1)");
		e.set_caret(5); /* right before the ')' (the text is 6 long) */
		e.insert_newline("");
		check_state("autoindent: before a closing bracket with no opener nearby", e, "foo(1\n)", 6, 6);

		/* Tabs in the base indentation are kept (not converted to spaces). */
		Buffer f("\tfoo");
		f.set_caret(4);
		f.insert_newline("\t    ");
		check_state("autoindent: base with tab", f, "\tfoo\n\t    ", 10, 10);

		/* A single empty line: behaves as before (does not evaluate). */
		Buffer g("a\n\nb");
		g.set_caret(2);
		g.insert_newline("");
		check_str("autoindent: empty line", g.text(), "a\n\n\nb");

		/* With a selection: the newline replaces the selection (one undo
		 * step). */
		Buffer h("abcdef");
		h.set_selection(2, 4);
		h.insert_newline("");
		check_state("autoindent: replaces selection", h, "ab\nef", 3, 3);
		check_int("autoindent: 1 undo step", h.undo_depth(), 1);
	}

	/* --- 15) reindent_line (port of the dedent when typing a closer) ------- */
	{
		Buffer a("        )");
		a.set_caret(9);
		check_bool("reindent_line: dedent of the closer", a.reindent_line(0, ""), true);
		check_state("reindent_line: dedent (estado)", a, ")", 1, 1);

		Buffer b("    foo");
		b.set_caret(7);
		check_bool("reindent_line: 4 → 2 spaces", b.reindent_line(0, "  "), true);
		check_state("reindent_line: caret ajustado", b, "  foo", 5, 5);

		Buffer c("    foo");
		check_bool("reindent_line: same indentation = no-op", c.reindent_line(0, "    "), false);

		/* Coalescing: typing + dedent = a single undo step. */
		Buffer d;
		d.insert_char('a');
		check_int("reindent_line: undo before", d.undo_depth(), 1);
		d.reindent_line(0, "  ");
		check_int("reindent_line: coalesces with the burst", d.undo_depth(), 1);
		check_str("reindent_line: text", d.text(), "  a");
		d.undo();
		check_str("reindent_line: one undo reverts everything", d.text(), "");

		/* Only the start of the requested line is touched. */
		Buffer e("  uno\n    dos");
		e.reindent_line(1, "");
		check_str("reindent_line: only line 1", e.text(), "  uno\ndos");
	}

	/* --- revision: contract of the caches that depend on the document (S5c) -- */
	{
		Buffer b("(abc)");
		const int r0 = b.revision();
		b.move_right();
		b.move_line_end();
		b.select_all();
		check_int("revision: moverse/seleccionar no cambia", b.revision(), r0);
		b.insert_char('x');
		check_int("revision: tipear sube 1", b.revision(), r0 + 1);
		const int r1 = b.revision();
		b.backspace();
		check_int("revision: borrar sube 1", b.revision(), r1 + 1);
		b.undo();
		check_int("revision: undo sube 1", b.revision(), r1 + 2);
		b.redo();
		check_int("revision: redo sube 1", b.revision(), r1 + 3);
		b.set_text("otro");
		check_int("revision: set_text sube 1", b.revision(), r1 + 4);
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
