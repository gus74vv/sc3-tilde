// SPDX-License-Identifier: GPL-3.0-only
/* lexer_test.cpp — S0 gate: 1:1 tokens with the SC IDE lexer.
 *
 * Provenance of the expected behavior: port of
 * vendor/supercollider/editors/sc-ide/core/sc_lexer.cpp +
 * widgets/code_editor/highlighter.cpp (tag Version-3.14.1). Every expectation in
 * this file was derived by hand from the IDE regexes (commented next to each
 * case) — not by running the IDE.
 *
 *     GPL v3 or later (ver sc_lexer.cpp).
 *
 * Token format: "Type@start:len" and, if the token is 1 printable ASCII byte,
 * "(\'c\')" — that `character` is the one the region rules use (`(`, `)`, `;`).
 *
 * Usage: ./build/lexer_test [-v] (with -v it prints the real table of each
 * case)
 */

#include "../sc_lexer.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static std::string fmt_token(const Token &t) {
	char buf[128];
	if (t.length == 1 && t.character >= 32 && t.character < 127) {
		if (t.character == '\'')
			snprintf(buf, sizeof(buf), "%s@%d:1('\\'')", token_type_name(t.type), t.start);
		else
			snprintf(buf, sizeof(buf), "%s@%d:1('%c')", token_type_name(t.type), t.start, t.character);
	} else {
		snprintf(buf, sizeof(buf), "%s@%d:%d", token_type_name(t.type), t.start, t.length);
	}
	return std::string(buf);
}

static void print_list(const char *label, const std::vector<std::string> &v) {
	printf("        %-8s [", label);
	for (size_t i = 0; i < v.size(); ++i)
		printf("%s%s", i ? " " : "", v[i].c_str());
	printf("]\n");
}

static void check_list(const char *caseName, const std::vector<std::string> &got, const std::vector<std::string> &want) {
	++gChecks;
	if (got == want)
		return;
	++gFails;
	printf("  FAIL %s\n", caseName);
	print_list("got", got);
	print_list("esperado", want);
}

static void check_int(const char *caseName, const char *what, int got, int want) {
	++gChecks;
	if (got == want)
		return;
	++gFails;
	printf("  FAIL %s (%s): %d, esperado %d\n", caseName, what, got, want);
}

/* Single-line case: tokens + outgoing state. */
static void case_line(const char *name, const std::string &text, const std::vector<std::string> &want,
					  int wantStateOut = InCode) {
	std::vector<Token> tokens;
	LineTokens lt = lex_line(text, 0, 0, InCode, &tokens);

	std::vector<std::string> got;
	for (size_t i = 0; i < tokens.size(); ++i)
		got.push_back(fmt_token(tokens[i]));

	if (gVerbose) {
		printf("  %-44s [%d→%d]", name, lt.stateIn, lt.stateOut);
		print_list("tokens", got);
	}

	check_list(name, got, want);
	check_int(name, "stateOut", lt.stateOut, wantStateOut);
}

/* Multi-line case: document tokens + state and comment flag per line. */
static void case_doc(const char *name, const std::string &text, const std::vector<std::string> &want,
					 const std::vector<int> &wantStatesOut, const std::vector<bool> &wantInComment) {
	DocumentTokens doc = lex_document(text);

	std::vector<std::string> got;
	for (size_t i = 0; i < doc.tokens.size(); ++i)
		got.push_back(fmt_token(doc.tokens[i]));

	if (gVerbose) {
		printf("  %-44s", name);
		print_list("tokens", got);
		for (size_t l = 0; l < doc.lines.size(); ++l)
			printf("        line %zu: [%d,%d) in=%d out=%d cmt=%d\n", l, doc.lines[l].start,
				   doc.lines[l].start + doc.lines[l].length, doc.lines[l].stateIn, doc.lines[l].stateOut,
				   (int)doc.lines[l].isInMultilineComment);
	}

	check_list(name, got, want);

	++gChecks;
	if ((int)doc.lines.size() != (int)wantStatesOut.size()) {
		++gFails;
		printf("  FAIL %s: %zu lines, expected %zu\n", name, doc.lines.size(), wantStatesOut.size());
		return;
	}
	for (size_t l = 0; l < doc.lines.size(); ++l) {
		char what[80];
		snprintf(what, sizeof(what), "stateOut line %zu", l);
		check_int(name, what, doc.lines[l].stateOut, wantStatesOut[l]);
		if (l < wantInComment.size()) {
			snprintf(what, sizeof(what), "isInMultilineComment line %zu", l);
			check_int(name, what, (int)doc.lines[l].isInMultilineComment, (int)wantInComment[l]);
		}
	}
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i)
		if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;

	printf("lexer_test — port of the SC IDE 3.14.1 ScLexer/highlighter\n");

	/* --- #1/#2 keyword y builtin, corchetes --------------------------------- */
	/* `(var x = nil)`: Keyword before Name; `nil` is Builtin. */
	case_line("basic (var x = nil)", "(var x = nil)",
			  { "OpeningBracket@0:1('(')", "Keyword@1:3", "Name@5:1('x')", "Operator@7:1('=')", "Builtin@9:3",
				"ClosingBracket@12:1(')')" });

	/* `\b(arg|…)\b` wins by order: "arg:" is NOT SymbolArg (the `:` stays Unknown).
	 * `Class` comes AFTER SymbolArg (SymbolArg ⊃ Class) but BEFORE Primitive. */
	case_line("keywords vs symbolarg", "arg: Class sym: _prim obj: this",
			  { "Keyword@0:3", "Unknown@3:1(':')", "Class@5:5", "SymbolArg@11:4", "Primitive@16:5",
				"SymbolArg@22:4", "Keyword@27:4" });

	/* `\b(this)\b` does not match inside "thisFunction" (the trailing `\b`
	 * fails). */
	case_line("builtin vs keyword", "thisFunction thisProcess topEnvironment true false nil inf",
			  { "Builtin@0:12", "Builtin@13:11", "Builtin@25:14", "Builtin@40:4", "Builtin@45:5", "Builtin@51:3",
				"Builtin@55:3" });

	/* --- #3..#6 numbers ----------------------------------------------------- */
	/* `3s` ScaleDegreeFloat · `2r1010` RadixFloat · `0xFF` HexInt (Float fails
	 * because the `\b` after "0" is not satisfied) · `1.5e3` and `pi` Float. */
	case_line("numbers", "3s 2r1010 0xFF 1.5e3 pi",
			  { "ScaleDegreeFloat@0:2", "RadixFloat@3:6", "HexInt@10:4", "Float@15:5", "Float@21:2" });

	/* Backtracking of the trailing `\b` of Float: "1.5x" ⇒ Float "1"; then \'.\'
	 * and \'5\' match NOTHING (Unknown of 1) and "x" is a Name. */
	case_line("float backtracking (1.5x)", "1.5x",
			  { "Float@0:1('1')", "Unknown@1:1('.')", "Unknown@2:1('5')", "Name@3:1('x')" });

	/* RadixFloat allows an empty `[0-9A-Z]*` after the dot ⇒ "2r." is a single
	 * token. */
	case_line("radix float with a lone dot", "2r.x",
			  { "RadixFloat@0:3", "Name@3:1('x')" });

	/* `(s+|b+|[sb]\d+)`: los tres caminos. */
	case_line("scale degrees", "3s 2b 4s2",
			  { "ScaleDegreeFloat@0:2", "ScaleDegreeFloat@3:2", "ScaleDegreeFloat@6:3" });

	/* --- #7..#13 symbols, chars, env vars ---------------------------------- */
	case_line("symbol char envvar", "\\sym $c ~env x:",
			  { "Symbol@0:4", "Char@5:2", "EnvVar@8:4", "SymbolArg@13:2" });

	/* `\$\\?.` needs a char after the '$': at end of line it does not match. */
	case_line("lone dollar", "$",
			  { "Unknown@0:1('$')" });

	/* `$\\` at end of line: the `\\?` backtracks and the `.` eats the
	 * backslash. */
	case_line("dollar backslash EOL", "$\\",
			  { "Char@0:2" });

	case_line("char escapado", "$\\x ",
			  { "Char@0:3" });

	/* Strings: ONLY the quotes are stored; `\"` does not close the
	 * string. */
	case_line("string with escape", "\"a\\\"b\" y",
			  { "StringMark@0:1('\"')", "StringMark@5:1('\"')", "Name@7:1('y')" });

	/* Symbol literal `'str'`: dos SymbolMark. */
	case_line("quoted symbol", "'str' x",
			  { "SymbolMark@0:1('\\'')", "SymbolMark@4:1('\\'')", "Name@6:1('x')" });

	/* --- #14/#15/#16 comentarios y operadores ------------------------------- */
	/* `// …` covers the rest of the line: the `(` inside does not exist as a
	 * token. */
	case_line("line comment", "a // (b) c",
			  { "Name@0:1('a')" });

	/* Block comment on ONE line: neither the start nor the end is stored; the
	 * state goes back to InCode and the code continues after the closing. */
	case_line("multi-line comment on one line", "a /* b */ c",
			  { "Name@0:1('a')", "Name@10:1('c')" });

	/* Operator is greedy: the whole run of operator chars is ONE token. */
	case_line("operadores greedy", "a <=> b +++ c",
			  { "Name@0:1('a')", "Operator@2:3", "Name@6:1('b')", "Operator@8:3", "Name@12:1('c')" });

	/* --- anidamiento de comentarios (estado InComment+N) -------------------- */
	/* Line 0 (three openings, one closing) leaves stateOut = 101; line 1
	 * closes another level (100) and line 2 closes the last one and stays in
	 * code: the remaining closer is NO longer a comment (we are in InCode)
	 * and it is tokenized as `*` + `/`, that is, a length-2 Operator. */
	case_doc("nested comment 3 levels", "/* a /* b /* c */ d\ne */ f\ng */ h */ i", 
			 { "Name@32:1('h')", "Operator@34:2", "Name@37:1('i')" }, { 101, 100, InCode },
			 { true, true, true });

	/* A comment that closes right at the end of the line goes back to InCode. */
	case_doc("comment closes at end of line", "a /* b */\nc",
			 { "Name@0:1('a')", "Name@10:1('c')" }, { InCode, InCode }, { true, false });

	/* Unclosed comment: every line stays InComment. The final empty line does
	 * not enter the comment branch ⇒ isInMultilineComment = false (the IDE does
	 * the same: there are no tokens and no userData). */
	case_doc("unclosed comment", "/* a\nb\n",
			 {}, { InComment, InComment, InComment }, { true, true, false });

	/* --- multi-line strings/symbols ---------------------------------------- */
	/* An unclosed string drags the state: the closing quote on the next line is
	 * the StringMark (position of THAT quote). */
	case_doc("multi-line string", "x = \"abc\ndef\" y",
			 { "Name@0:1('x')", "Operator@2:1('=')", "StringMark@4:1('\"')", "StringMark@12:1('\"')",
			   "Name@14:1('y')" },
			 { InString, InCode }, { false, false });

	/* A `'` at end of line does NOT open a symbol (the end of line closes
	 * it). */
	case_doc("symbol mark at end of line", "a '\nb",
			 { "Name@0:1('a')", "SymbolMark@2:1('\\'')", "Name@4:1('b')" }, { InCode, InCode },
			 { false, false });

	/* --- empty document / only whitespace ---------------------------------- */
	case_doc("empty document", "", {}, { InCode }, { false });
	case_doc("a single newline", "\n", {}, { InCode, InCode }, { false, false });
	case_line("only spaces", "   ", {});

	/* --- non-ASCII: UTF-8 bytes are Unknown one by one, and they do NOT break the
	 * states - */
	case_line("utf-8 en string", "\"\xc3\xb1\" x",
			  { "StringMark@0:1('\"')", "StringMark@3:1('\"')", "Name@5:1('x')" });
	case_line("utf-8 en comentario", "// \xc3\xb1 (x)", {});
	case_line("lone utf-8", "\xc3\xb1", { "Unknown@0:1", "Unknown@1:1" });

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
