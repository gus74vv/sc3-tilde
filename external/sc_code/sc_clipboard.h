// SPDX-License-Identifier: GPL-3.0-only
/* sc_clipboard.h — system clipboard for `sc_code` (S2).
 *
 * The Max SDK does **not** expose the clipboard (`jpatcher_api.h` only has
 * `jclipboard_datatypes()`), so it goes through `NSPasteboard` in ObjC++
 * This header is pure C++: the implementation
 * (sc_clipboard.mm) is the only part of the editor that talks to AppKit.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

#include <string>

namespace sc_code {

/* UTF-8 plain text of the general clipboard; "" if there is no text or it is not
 * UTF-8. */
std::string clipboard_read_text();

/* Replaces the clipboard contents with `text` (UTF-8). false if it failed. */
bool clipboard_write_text(const std::string &text);

} // namespace sc_code
