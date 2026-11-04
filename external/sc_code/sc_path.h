// SPDX-License-Identifier: GPL-3.0-only
/* sc_path.h — path formatting for the editor (SCODE.md §12: drops from the Finder;
 * §11: the `@snippets` path).
 *
 * Pure C++ (no Max, no AppKit) so the rules are pinned in `tests/path_test.cpp`.
 *
 * Three problems, three groups of functions:
 *   - `path_to_posix` / `path_to_absolute`: Max hands over pathnames in its own
 *     dialect (`Macintosh HD:/Users/gustavo/Documents/synth.wav`, or a search-path
 *     relative name). The editor should show the plain absolute path — the volume
 *     prefix is noise and a `~` is not a path sclang understands
 *     (`SoundFile.openRead("~/x.wav")` fails), so what gets inserted is
 *     `/Users/gustavo/Documents/synth.wav`.
 *   - `quote_sc_string`: what gets inserted is SuperCollider source, so the path goes
 *     between double quotes and `\` and `"` escape.
 *   - `unquote_path`: the INPUT side. A path typed in a message box arrives with its
 *     single quotes intact (`snippets '~/x.txt'` — Max only strips double quotes, and
 *     only when parsing the box) and a copied shell line may bring either kind, so one
 *     pair of surrounding quotes is dropped before opening the file.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <string>
#include <vector>

namespace sc_code {

/* Absolute POSIX form of a Max pathname: the volume prefix (`Macintosh HD:`) is
 * dropped, so `Macintosh HD:/Users/g/x.wav` -> `/Users/g/x.wav`. A name relative to a
 * Max search path stays as is (there is nothing to join it to here). Used to check that
 * a dropped item really is a file (an object drag has no such path). */
std::string path_to_posix(const std::string &in);

/* The same, plus `~` -> `home`, which is what the editor inserts:
 *   "Macintosh HD:/Users/g/x.wav"  + home "/Users/g" -> "/Users/g/x.wav"
 *   "~/Documents/x.wav"                              -> "/Users/g/Documents/x.wav"
 *   "~"                                              -> "/Users/g"
 *   "x.wav" (relative to a search path)              -> "x.wav"   (left untouched)
 *   "~other/x.wav"                                   -> "~other/x.wav"
 * A trailing `/` in `home` is ignored; an empty `home` disables the expansion; an empty
 * input stays empty. */
std::string path_to_absolute(const std::string &home, const std::string &in);

/* The path without one pair of surrounding quotes: `'…'` or `"…"`, with the whitespace
 * around it trimmed. Only a FULL wrap counts, so `a"b.wav` and `'abc` stay as they are
 * (the open error then shows the real text), and `''` becomes empty (= no path, which
 * is how `@snippets ""` unloads the file). The inside is verbatim: no escapes. */
std::string unquote_path(const std::string &in);

/* The path as an SC string literal: `"..."` with `\` and `"` escaped. */
std::string quote_sc_string(const std::string &path);

/* The text inserted by a drop: every path made absolute (see `path_to_absolute`),
 * quoted, separated by a space (dropping two files in a row gives
 * `"/Users/g/a.wav" "/Users/g/b.wav"`). */
std::string dropped_text(const std::string &home, const std::vector<std::string> &names);

} // namespace sc_code
