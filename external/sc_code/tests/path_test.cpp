// SPDX-License-Identifier: GPL-3.0-only
/* path_test.cpp — gate of the drop-path formatting and of the `@snippets` path
 * (headless, the design notes).
 *
 * The user asked for a dropped file to show up in the editor as
 * `"/Users/gustavo/Documents/synth.wav"`: absolute, with Max's volume prefix
 * (`Macintosh HD:`) gone and no `~` (sclang does not expand it). The same turn asked
 * for `@snippets` to take quoted paths, so `unquote_path` is pinned here too.
 *
 *     GPL v3 or later (see sc_path.cpp).
 *
 * Usage: ./build/path_test [-v]
 */

#include "../sc_path.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace sc_code;

static int gChecks = 0;
static int gFails = 0;
static bool gVerbose = false;

static void check_str(const char *what, const std::string &got, const std::string &want) {
	++gChecks;
	if (got != want) {
		++gFails;
		printf("  FAIL %-52s got \"%s\", want \"%s\"\n", what, got.c_str(), want.c_str());
	} else if (gVerbose) {
		printf("  ok   %-52s \"%s\"\n", what, got.c_str());
	}
}

int main(int argc, char **argv) {
	gVerbose = argc > 1 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--verbose");
	const std::string home = "/Users/gustavo";

	printf("path_to_posix\n");
	check_str("POSIX stays", path_to_posix("/Users/gustavo/x.wav"), "/Users/gustavo/x.wav");
	check_str("volume dropped", path_to_posix("Macintosh HD:/Users/gustavo/x.wav"), "/Users/gustavo/x.wav");
	check_str("volume with spaces", path_to_posix("My Disk:/Volumes/Data/x.wav"), "/Volumes/Data/x.wav");
	check_str("relative stays", path_to_posix("x.wav"), "x.wav");
	check_str("empty stays empty", path_to_posix(""), "");

	printf("path_to_absolute: volume prefix is dropped\n");
	check_str("POSIX stays absolute", path_to_absolute(home, "/Users/gustavo/Documents/synth.wav"),
			  "/Users/gustavo/Documents/synth.wav");
	check_str("volume + path under the home is NOT shortened",
			  path_to_absolute(home, "Macintosh HD:/Users/gustavo/Documents/synth.wav"),
			  "/Users/gustavo/Documents/synth.wav");
	check_str("volume name with a space", path_to_absolute(home, "My Disk:/Users/gustavo/a b.wav"),
			  "/Users/gustavo/a b.wav");
	check_str("volume, outside the home", path_to_absolute(home, "Macintosh HD:/Applications/SuperCollider.app"),
			  "/Applications/SuperCollider.app");
	check_str("outside the home", path_to_absolute(home, "/Volumes/Data/x.wav"), "/Volumes/Data/x.wav");

	printf("path_to_absolute: ~ becomes the home\n");
	check_str("tilde path", path_to_absolute(home, "~/Documents/x.wav"), "/Users/gustavo/Documents/x.wav");
	check_str("tilde alone", path_to_absolute(home, "~"), "/Users/gustavo");
	check_str("home with a trailing slash", path_to_absolute("/Users/gustavo/", "~/x.wav"), "/Users/gustavo/x.wav");
	check_str("another user's tilde stays", path_to_absolute(home, "~other/x.wav"), "~other/x.wav");
	check_str("tilde in the middle stays", path_to_absolute(home, "/x/~/y.wav"), "/x/~/y.wav");
	check_str("no home to expand", path_to_absolute("", "~/x.wav"), "~/x.wav");
	check_str("home = / does not expand", path_to_absolute("/", "~/x.wav"), "~/x.wav");
	check_str("home-like prefix, no slash", path_to_absolute(home, "/Users/gustavox"), "/Users/gustavox");

	printf("path_to_absolute: relative names are left alone\n");
	check_str("search-path relative name", path_to_absolute(home, "x.wav"), "x.wav");
	check_str("relative with a subdirectory", path_to_absolute(home, "sounds/x.wav"), "sounds/x.wav");
	check_str("empty stays empty", path_to_absolute(home, ""), "");

	printf("unquote_path: one pair of quotes is dropped\n");
	check_str("double quotes", unquote_path("\"/Users/gustavo/my file.txt\""), "/Users/gustavo/my file.txt");
	check_str("single quotes", unquote_path("'/Users/gustavo/my file.txt'"), "/Users/gustavo/my file.txt");
	check_str("double quoted tilde", unquote_path("\"~/x.txt\""), "~/x.txt");
	check_str("single quoted tilde", unquote_path("'~/x.txt'"), "~/x.txt");
	check_str("spaces around the quotes", unquote_path("  '~/x.txt'  "), "~/x.txt");
	check_str("spaces around a bare path", unquote_path("\t/Users/g/x.txt\n"), "/Users/g/x.txt");
	check_str("no quotes", unquote_path("~/x.txt"), "~/x.txt");
	check_str("empty quotes mean no path", unquote_path("''"), "");
	check_str("empty double quotes", unquote_path("\"\""), "");
	check_str("empty stays empty", unquote_path(""), "");
	check_str("unbalanced single quote stays", unquote_path("'/Users/g/x.txt"), "'/Users/g/x.txt");
	check_str("unbalanced double quote stays", unquote_path("/Users/g/x.txt\""), "/Users/g/x.txt\"");
	check_str("mismatched pair stays", unquote_path("'/Users/g/x.txt\""), "'/Users/g/x.txt\"");
	check_str("quote only at the end stays", unquote_path("/Users/g/a\"b"), "/Users/g/a\"b");
	check_str("inner quotes are verbatim", unquote_path("\"/Users/g/a'b.txt\""), "/Users/g/a'b.txt");
	check_str("one character stays", unquote_path("'"), "'");

	printf("quote_sc_string\n");
	check_str("plain", quote_sc_string("~/x.wav"), "\"~/x.wav\"");
	check_str("spaces stay", quote_sc_string("~/a b.wav"), "\"~/a b.wav\"");
	check_str("escapes \\", quote_sc_string("/Users/g/a\\b.wav"), "\"/Users/g/a\\\\b.wav\"");
	check_str("escapes \"", quote_sc_string("/Users/g/a\"b.wav"), "\"/Users/g/a\\\"b.wav\"");
	check_str("empty", quote_sc_string(""), "\"\"");

	printf("dropped_text\n");
	{
		std::vector<std::string> one;
		one.push_back("Macintosh HD:/Users/gustavo/Documents/synth.wav");
		check_str("one file is absolute", dropped_text(home, one),
				  "\"/Users/gustavo/Documents/synth.wav\"");
	}
	{
		std::vector<std::string> two;
		two.push_back("Macintosh HD:/Users/gustavo/a.wav");
		two.push_back("Macintosh HD:/Users/gustavo/b.wav");
		check_str("two files, one space between", dropped_text(home, two),
				  "\"/Users/gustavo/a.wav\" \"/Users/gustavo/b.wav\"");
	}
	{
		std::vector<std::string> none;
		check_str("no files", dropped_text(home, none), "");
	}
	{
		std::vector<std::string> tilde;
		tilde.push_back("~/a b.wav");
		check_str("a tilde path is expanded and quoted", dropped_text(home, tilde),
				  "\"/Users/gustavo/a b.wav\"");
	}
	{
		std::vector<std::string> rel;
		rel.push_back("x.wav");
		check_str("a relative name is left alone", dropped_text(home, rel), "\"x.wav\"");
	}

	printf("%s: %d checks, %d failures\n", gFails ? "FAIL" : "OK", gChecks, gFails);
	return gFails ? 1 : 0;
}
