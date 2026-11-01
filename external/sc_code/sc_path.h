// SPDX-License-Identifier: GPL-3.0-only
/* sc_path.h — path formatting for the editor (SCODE.md §12: drops from the Finder).
 *
 * Pure C++ (no Max, no AppKit) so the rules are pinned in `tests/path_test.cpp`.
 *
 * Two problems, two functions:
 *   - `path_to_tilde`: Max hands over pathnames in its own dialect
 *     (`Macintosh HD:/Users/gustavo/Documents/synth.wav`, or a search-path relative
 *     name). In the editor that is noise: the user asked for `~/Documents/synth.wav`.
 *     `path_to_posix` is the same normalization without the `~` (measured: a Finder
 *     drop in Max 9 delivers a POSIX path already, but the dialect is undocumented,
 *     so both are handled).
 *   - `quote_sc_string`: what gets inserted is SuperCollider source, so the path goes
 *     between double quotes and `\` and `"` escape.
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

/* POSIX form of a Max pathname: only the volume prefix (`Macintosh HD:`) is dropped,
 * so `Macintosh HD:/Users/g/x.wav` -> `/Users/g/x.wav` and a search-path relative
 * name stays as is. Used to check that a dropped item really is a file (an object
 * drag has no such path) and by `path_to_tilde`. */
std::string path_to_posix(const std::string &in);

/* Shortest readable form of a Max pathname: the volume prefix (`Macintosh HD:`) is
 * dropped and the real home directory collapses to `~`.
 *   "/Users/g/x.wav"          + home "/Users/g" -> "~/x.wav"
 *   "Macintosh HD:/Users/g/x"                 -> "~/x.wav"
 *   "Macintosh HD:/Applications/SC.app"       -> "/Applications/SC.app"
 *   "x.wav" (relative to a search path)       -> "x.wav"  (left untouched: it works)
 * A trailing `/` in `home` is ignored. An empty input stays empty. */
std::string path_to_tilde(const std::string &home, const std::string &in);

/* The path as an SC string literal: `"..."` with `\` and `"` escaped. */
std::string quote_sc_string(const std::string &path);

/* The text inserted by a drop: every path quoted, separated by a space (dropping
 * two files in a row gives `"~/a.wav" "~/b.wav"`). */
std::string dropped_text(const std::string &home, const std::vector<std::string> &names);

} // namespace sc_code
