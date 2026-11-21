// SPDX-License-Identifier: GPL-3.0-only
/* sc_filedialog.mm — NSSavePanel/NSOpenPanel with UTType for `sc_code` (S5d).
 *
 * Compiled with `-framework AppKit -framework UniformTypeIdentifiers` (Makefile).
 * No ARC (the bundle lives inside Max): explicit `@autoreleasepool`.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "sc_filedialog.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace sc_code {

static NSString *utf8_to_ns(const std::string &s) {
	return [[NSString alloc] initWithBytes:s.data()
								   length:(NSUInteger)s.size()
								 encoding:NSUTF8StringEncoding];
}

static std::string ns_to_utf8(NSString *s) {
	if (!s)
		return std::string();
	const char *utf8 = [s UTF8String];
	return utf8 ? std::string(utf8) : std::string();
}

/* Tipos que ofrecemos: `.scd` (IDE) y `.sc` (SC 3.4 y anteriores). */
static NSArray<UTType *> *code_types(BOOL include_sc) {
	UTType *scd = [UTType typeWithFilenameExtension:@"scd"];
	UTType *sc = [UTType typeWithFilenameExtension:@"sc"];
	if (!scd)
		return include_sc && sc ? @[ sc ] : @[];
	return include_sc && sc ? @[ scd, sc ] : @[ scd ];
}

std::string dialog_save_code(const std::string &default_name) {
	@autoreleasepool {
		NSSavePanel *panel = [NSSavePanel savePanel];
		if (!panel)
			return std::string();
		[panel setTitle:@"Save supercollider code"];
		[panel setPrompt:@"Save"];
		[panel setCanCreateDirectories:YES];
		[panel setExtensionHidden:NO];
		/* Only `.scd`: the panel appends the extension to whatever the user types and
		 * does not let another format be chosen (that was exactly the bug: the file
		 * came out without .scd). */
		[panel setAllowedContentTypes:code_types(NO)];
		[panel setAllowsOtherFileTypes:NO];
		NSString *name = utf8_to_ns(default_name);
		if (name)
			[panel setNameFieldStringValue:name];
		if ([panel runModal] != NSModalResponseOK)
			return std::string();
		return ns_to_utf8([[panel URL] path]);
	}
}

std::string dialog_open_code() {
	@autoreleasepool {
		NSOpenPanel *panel = [NSOpenPanel openPanel];
		if (!panel)
			return std::string();
		[panel setTitle:@"Open supercollider code"];
		[panel setPrompt:@"Open"];
		[panel setCanChooseFiles:YES];
		[panel setCanChooseDirectories:NO];
		[panel setAllowsMultipleSelection:NO];
		[panel setResolvesAliases:YES];
		[panel setAllowedContentTypes:code_types(YES)];
		/* Opening a `.txt`/`.scd.bak` is fine too: we do not over-restrict. */
		[panel setAllowsOtherFileTypes:YES];
		if ([panel runModal] != NSModalResponseOK)
			return std::string();
		NSURL *url = [[panel URLs] firstObject];
		return url ? ns_to_utf8([url path]) : std::string();
	}
}

} // namespace sc_code
