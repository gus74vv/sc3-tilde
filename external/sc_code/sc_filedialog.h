// SPDX-License-Identifier: GPL-3.0-only
/* sc_filedialog.h — save/open panels for `.scd` in `sc_code` (S5d).
 *
 * Why not the SDK `saveas_dialog`/`open_dialog`? Because those panels go through
 * the Max type mapping (`max-fileformats.txt`): a `.scd` is not a Max type, so the
 * panel neither proposes nor enforces it, and the file ended up being saved without
 * the right extension (or with the one of the format chosen in the popup). Here
 * `NSSavePanel`/`NSOpenPanel` are used directly with `UTType` ⇒ the `.scd`
 * extension is guaranteed and the open panel filters `.scd`/`.sc`.
 *
 * This header is pure C++: the only ObjC++ implementation is `sc_filedialog.mm`.
 * Both are **blocking** (`runModal`) and must be called from the main thread
 * (the object `mousedown` is).
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <string>

namespace sc_code {

/* Save panel. `default_name` is only the proposed name (`.scd` is appended
 * if it comes without an extension). Returns the chosen POSIX path or "" if
 * cancelled. It never returns something that does not end in `.scd`. */
std::string dialog_save_code(const std::string &default_name);

/* Open panel. Filters `.scd` and `.sc` (it lets you choose others with "Options"),
 * a single file. Returns the chosen POSIX path or "" if cancelled. */
std::string dialog_open_code();

} // namespace sc_code
