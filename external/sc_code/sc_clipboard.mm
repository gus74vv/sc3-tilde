// SPDX-License-Identifier: GPL-3.0-only
/* sc_clipboard.mm — system clipboard (S2) through NSPasteboard.
 *
 * The only ObjC++ file of the editor: `NSPasteboard` / `NSString` live here and
 * nowhere else (`sc_buffer`, `sc_lexer`, `sc_region`, `sc_code.cpp` are pure
 * C++). Compiled with `-framework AppKit` (see Makefile).
 *
 * Careful: explicit `@autoreleasepool` because the bundle is NOT built with ARC
 * — Max loads the .mxo inside its own process and we do not want to depend on
 * its pool.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "sc_clipboard.h"

#import <AppKit/AppKit.h>

namespace sc_code {

std::string clipboard_read_text() {
	@autoreleasepool {
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		NSString *s = [pb stringForType:NSPasteboardTypeString];
		if (!s)
			return std::string();
		const char *utf8 = [s UTF8String];
		return utf8 ? std::string(utf8) : std::string();
	}
}

bool clipboard_write_text(const std::string &text) {
	@autoreleasepool {
		NSString *s = [[NSString alloc] initWithBytes:text.data()
											  length:(NSUInteger)text.size()
											encoding:NSUTF8StringEncoding];
		if (!s)
			return false;
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		[pb clearContents];
		const BOOL ok = [pb setString:s forType:NSPasteboardTypeString];
		[s release]; /* sin ARC */
		return ok == YES;
	}
}

} // namespace sc_code
