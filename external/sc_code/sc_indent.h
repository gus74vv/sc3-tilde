// SPDX-License-Identifier: GPL-3.0-only
/* sc_indent.h — indentation level for a new line, PORT of the SC IDE.
 *
 * Procedencia (SCODE.md §3.10/§3.8):
 *   vendor/supercollider/editors/sc-ide/widgets/code_editor/sc_editor.cpp
 *     (tag Version-3.14.1): indentationLevel L709, indent L597,
 *     makeIndentationString L688, indentedStartOfLine L572.
 * Copyright (c) 2012 Jakob Leben & Tim Blechmann — GPL v2 or later.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 * WHY a separate module exists: sclang indentation is NOT "counting brackets".
 * The IDE keeps a stack of levels with two rules that are not obvious:
 *
 * 1. A `(` **in column 0** with an EMPTY stack does not add a level (`if
 * (character != '(' || stack.size() || positionInBlock) ++level;`). That is
 * what makes a top-level `( … )` region stay "flat": the content goes to the
 * same column as the `(`. An indented `(`, or one nested inside something, DOES
 * add. 2. The accumulated `level` inside a block is pushed as **a single**
 * level when the next block starts (not one per bracket): `Ndef(\x, {` is 2
 * brackets but ONE level.
 *
 * The level returned is the stack length (plus the pending level of the
 * current block), which is what the IDE uses when indenting a new line after
 * a Return. Lines that START with a closer no longer need a special case:
 * processing the closer pops it and the stack length drops by itself (so the
 * `)` goes back to the column of its `(`).
 */

#pragma once

#include <string>

#include "sc_lexer.h"

namespace sc_code {

/* Indentation level (in LEVELS, not spaces) for a new line inserted right at
 * `(line, col)` (col = column in bytes inside the line). It is the level the line
 * created by Return belongs to. */
int indent_level_at(const DocumentTokens &doc, int line, int col);

/* Same, from the byte offset in the document (lexes internally).
 *
 * Includes the two exceptions of indent() (IDE L657-668, SCODE.md §7.7.6): if the
 * caret ends up inside a multi-line comment or a string, the new line is NOT
 * indented (level 0): the IDE carries the "in_comment"/"in_string" of the
 * previous block. */
int indent_level_at_offset(const std::string &text, int offset);

/* Level of a line that ALREADY exists, with the semantics of the IDE
 * `indent()` (a block that starts with a closer is indented to the level after
 * processing it). With the IDE rule over the whole document, the result is the
 * indentation the IDE would give each line. */
int indent_level_for_line(const DocumentTokens &doc, int line);

/* Same, from the byte offset of any position of the line (lexes internally). */
int indent_level_for_line_at_offset(const std::string &text, int offset);

/* Result of reindenting an EXISTING line: the level and whether the IDE touches
 * it. */
struct LineIndent {
	int level;
	bool apply;
};

/* Reindent of the cursor line (what typing a closer triggers, SCODE.md §7.7.5).
 * `apply == false` in the two cases where the IDE does NOT reindent (indent()
 * L657-668): - some part of the line is inside a multi-line comment
 * (TextBlockData::isInMultilineComment), or - the line is exactly the start of a
 * comment (two characters), because the lexer does not see it as "in comment" yet
 * when deciding. If the line STARTS inside a string, it is applied but with level
 * 0. */
LineIndent indent_for_line_at_offset(const std::string &text, int offset);

/* Indentation text: `level` × `indentWidth` spaces (the IDE with
 * `mSpaceIndent`, which is the default; we do not use tabs, §7.1). */
std::string indent_string(int level, int indentWidth);

} // namespace sc_code
