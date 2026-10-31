#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""spdx.py — insert or check the SPDX license identifier in every source file.

The project is GPL-3.0-only (see LICENSE): SuperCollider is GPLv3 and the engine
links its statics, so the bundle is a derived work.

Usage:
    python3 tools/spdx.py apply     # add the header where it is missing (idempotent)
    python3 tools/spdx.py check     # exit 1 if any file lacks it (CI gate)

Placement rules, so the header never breaks a file:
  * C/C++/ObjC (.c .cc .cpp .h .hpp .mm) and .sc  -> `// SPDX-...` as the first line
  * shell scripts (.sh)                           -> `# SPDX-...` after the shebang
  * python (.py)                                  -> `# SPDX-...` after shebang/coding line
  * Makefiles (Makefile, *.mk)                    -> `# SPDX-...` as the first line

Not covered on purpose: patcher assets and fixtures (.maxpat, .maxhelp, .scd), which
are data (Max would reject a stray comment in some of them) and generated/vendored
trees (build/, vendor/, backup/, Frameworks/).
"""

import argparse
import os
import sys

SPDX = 'SPDX-License-Identifier: GPL-3.0-only'

SLASH_EXTS = {'.c', '.cc', '.cpp', '.h', '.hpp', '.mm', '.sc'}
HASH_EXTS = {'.sh', '.py', '.mk'}
MAKE_NAMES = {'Makefile', 'makefile', 'GNUmakefile'}
SKIP_DIRS = {'vendor', 'build', '.git', 'Frameworks', 'backup', 'spike', '__pycache__'}


def walk(root):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(
            d for d in dirnames if d not in SKIP_DIRS and not d.endswith('.dSYM')
        )
        for name in sorted(filenames):
            yield os.path.join(dirpath, name)


def style(path):
    """Returns 'slash', 'hash' or None for the files this tool owns."""
    name = os.path.basename(path)
    ext = os.path.splitext(name)[1]
    if ext in SLASH_EXTS:
        return 'slash'
    if ext in HASH_EXTS or name in MAKE_NAMES:
        return 'hash'
    return None


def insert_offset(lines):
    """Index of the line the header goes before: right after a shebang and/or a
    coding cookie (both must stay on line 1), otherwise the very top."""
    idx = 0
    if lines and lines[0].startswith('#!'):
        idx = 1
    if idx < len(lines) and 'coding' in lines[idx][:40]:
        idx += 1
    return idx


def process(path, apply):
    how = style(path)
    if not how:
        return 'skip'
    with open(path, encoding='utf-8') as fh:
        lines = fh.read().splitlines(keepends=True)
    for line in lines[:4]:
        if 'SPDX-License-Identifier' in line:
            return 'ok' if SPDX in line else 'wrong'
    if not apply:
        return 'missing'
    marker = '// ' if how == 'slash' else '# '
    at = insert_offset(lines) if how == 'hash' else 0
    lines.insert(at, marker + SPDX + '\n')
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(''.join(lines))
    return 'fixed'


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('mode', choices=('apply', 'check'))
    ap.add_argument('--root', default='.', help='tree to scan (default: cwd)')
    args = ap.parse_args(argv)

    apply = args.mode == 'apply'
    counts, bad = {}, []
    for path in walk(args.root):
        rel = os.path.relpath(path, args.root)
        res = process(path, apply)
        if res not in ('skip', 'ok'):
            bad.append((rel, res))
        counts[res] = counts.get(res, 0) + 1

    for rel, res in bad:
        print(f'{res:8} {rel}')
    if apply:
        print('added:', counts.get('fixed', 0), '| already ok:', counts.get('ok', 0))
        return 0
    print(f"checked: {counts.get('ok', 0)} ok, {len(bad)} without the header")
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
