#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""i18n_scan.py — measures how much Spanish text is left in the sc3~/sc_code code.

It translates nothing: it only counts and lists, so that the ES->EN translation is
measurable and reviewable. Sibling of `external/check_flat_symbols.py` in spirit:
a gate, not a formatter.

Usage:
    python3 tools/i18n_scan.py                 # per-file summary (sections)
    python3 tools/i18n_scan.py --strings       # lists ES string literals
    python3 tools/i18n_scan.py --comments      # lists ES comment blocks
    python3 tools/i18n_scan.py --file X.cpp    # only one file
    python3 tools/i18n_scan.py --strict        # exit 1 if anything is left (for CI)

The detector is heuristic (accents + Spanish function words). A false negative
breaks nothing: the real gate of the translation is reading the diff.
"""

import argparse
import os
import re
import sys

ACCENT = re.compile(u'[\u00e1\u00e9\u00ed\u00f3\u00fa\u00fc\u00f1\u00d1\u00bf\u00a1]')

ES_WORDS = set("""
de la el los las que por para con una uno unos unas del al se no es son un mas más
solo sólo esta está estan están este esto estos esta hay desde cuando como pero sin
sobre entre tambien también tampoco esto eso esa esos esas lo le les su sus muy ya
aqui aquí asi así porque aunque donde cada todo toda todos todas nada algo alguien
puede pueden hace hacen hasta luego entonces mientras aunque ademas además si sí
debe deben falta falta queda quedan solo sólo usa usá usar poné poner mira ver
sólo cuando cuál cual quien quienes cuánto cuanto cómo como después antes durante
en listo lista arriba abajo dentro fuera mejor peor ahora nunca siempre hecho sale
salen entra entran vale muestra muestras empujadas empujados bloques texto
""".split())

# English function words: they serve to VETO false positives (an already translated
# comment that keeps a couple of words which also exist in Spanish).
EN_WORDS = set("""
the of is and to in for with that this it not be are on when which from by as at
or an if then than so no do does did has have had was were will would can could
only after before while into over under between both each same other another
there their its our your his her them we you they he she one two first last
""".split())

STRIP = '.,;:()[]{}"*/\\-_=+!?<>`\'' + chr(39)

# '.sc'/'.scd' use C-style comments (//), NOT shell ones: classifying them as
# scripts silently skipped every comment in the first spike/compat/SCMaxCompat.
C_EXTS = {'.cpp', '.h', '.mm', '.hpp', '.c', '.sc', '.scd'}
SCRIPT_EXTS = {'.sh', '.py', '.mk', '.in', '.plist'}
PATCH_EXTS = {'.maxpat', '.maxhelp', '.txt'}

SKIP_DIRS = {'vendor', 'build', '.git', 'Frameworks', 'backup', 'spike', '__pycache__'}
COMMENT_C = re.compile(r'^\s*(//|\*|/\*)')
COMMENT_HASH = re.compile(r'^\s*#')
# Double- AND single-quoted literals: shell/python messages live in '...' and were
# invisible to a '"..."'-only pattern.
STR_RE = re.compile(r'"((?:[^"\\]|\\.)*)"|\'((?:[^\'\\]|\\.)*)\'')
INLINE_RE = re.compile(r'/\*(.+?)\*/|//+\s*(.+)$')


def is_spanish(text):
    """True si el texto parece español: acentos MÁS al menos una palabra funcional
    española, o (sin acentos) más palabras funcionales españolas que inglesas.

    El veto del inglés evita falsos positivos en comentarios/labels ya traducidos que
    conservan palabras compartidas (`no`), y exigir una palabra funcional al acento
    evita marcar texto inglés que cita un carácter acentuado (`'é'`, `ñandú`).
    Un falso negativo no rompe nada: el gate real es leer el diff.
    """
    words = [w.strip(STRIP) for w in text.lower().split()]
    es = sum(1 for w in words if w in ES_WORDS)
    en = sum(1 for w in words if w in EN_WORDS)
    if ACCENT.search(text):
        return es >= 1 and es > en
    return es >= 2 and es > en


def walk(root):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(
            d for d in dirnames
            if d not in SKIP_DIRS and not d.endswith('.dSYM')
        )
        for name in sorted(filenames):
            if name == 'i18n_allow.txt':
                continue        # the exemption list itself: full of ES data fixtures
            yield os.path.join(dirpath, name)


def classify(path):
    base = os.path.basename(path)
    if base in ('Makefile', 'makefile', 'GNUmakefile'):
        return 'script'          # hash comments, like .mk/.sh
    ext = os.path.splitext(path)[1]
    if ext in C_EXTS:
        return 'c'
    if ext in SCRIPT_EXTS:
        return 'script'
    if ext in PATCH_EXTS:
        return 'patch'
    return None


def scan_comments(path, kind):
    """Returns [(line, text)] of Spanish comment blocks (joined)."""
    pat = COMMENT_HASH if kind == 'script' else COMMENT_C
    prefix = r'^\s*(#|//|\*|/\*)+\s?'
    out, cur, start = [], [], 0

    def flush():
        """A block that mixes English and Spanish is NOT Spanish as a whole, so the
        block is also judged line by line: that is how a single Spanish line used to
        hide inside an otherwise English header (build.sh, phase1/Makefile)."""
        if not cur:
            return
        body = ' '.join(cur).strip()
        if body and is_spanish(body):
            out.append((start, body))
            return
        for off, text in enumerate(cur):
            if text and is_spanish(text):
                out.append((start + off, text))

    for i, line in enumerate(open(path, encoding='utf-8', errors='replace'), 1):
        if pat.match(line):
            if not cur:
                start = i
            cur.append(re.sub(prefix, '', line.rstrip('\n')))
            continue
        flush()
        cur = []
        # Comments trailing code were never looked at (`x = 1; /* ... */` or
        # `x = 1; // ...`): an inline "/* added by us */" hid for months.
        if kind != 'script':
            for span in INLINE_RE.finditer(line):
                tail = (span.group(1) or span.group(2) or '').strip()
                if tail and is_spanish(tail):
                    out.append((i, tail))
    flush()
    return out


ALLOW_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'i18n_allow.txt')


def load_allow(path=ALLOW_FILE):
    """Literales exentos: fixtures de test en español cuyo largo/offsets importan
    (ver el propio archivo y I18N.md §7.4)."""
    allowed = set()
    if not os.path.exists(path):
        return allowed
    for raw in open(path, encoding='utf-8'):
        line = raw.rstrip('\n')
        if line and not line.startswith('«'):
            allowed.add(line)
    return allowed


ALLOW = load_allow()


def scan_strings(path):
    """Returns [(line, literal)] of Spanish string literals."""
    out = []
    for i, line in enumerate(open(path, encoding='utf-8', errors='replace'), 1):
        code = re.sub(r'^\s*//.*$', '', line)
        if code.lstrip().startswith('*'):
            continue
        for m in STR_RE.finditer(code):
            lit = m.group(1) if m.group(1) is not None else m.group(2)
            if lit in ALLOW:
                continue
            if len(lit.strip()) >= 2 and is_spanish(lit):
                out.append((i, lit))
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--root', default='.', help='tree to scan (default: cwd)')
    ap.add_argument('--file', help='scan a single file (path relative to --root)')
    ap.add_argument('--strings', action='store_true', help='list ES string literals')
    ap.add_argument('--comments', action='store_true', help='list ES comment blocks')
    ap.add_argument('--strict', action='store_true', help='exit 1 if any Spanish is left')
    ap.add_argument('--limit', type=int, default=0, help='max hits per mode')
    args = ap.parse_args(argv)

    if args.file:
        files = [args.file]
    else:
        files = [p for p in walk(args.root) if classify(p)]

    rows, total_blocks, total_strings = [], 0, 0
    listed = 0
    for path in files:
        kind = classify(path)
        if not kind:
            continue
        blocks = scan_comments(path, kind)
        strings = scan_strings(path)      # every kind: shell/python/patch messages
                                          # are user-facing text too
        total_blocks += len(blocks)
        total_strings += len(strings)
        if blocks or strings:
            rows.append((len(blocks), len(strings), path))
        if args.strings and strings:
            print('### %s (%d ES strings)' % (path, len(strings)))
            for line, lit in strings:
                print('  %5d  %s' % (line, lit))
                listed += 1
        if args.comments and blocks:
            print('### %s (%d ES comment blocks)' % (path, len(blocks)))
            for line, body in blocks:
                print('  %5d  %s' % (line, body[:400]))
                listed += 1
        if args.limit and listed >= args.limit:
            break

    if not (args.strings or args.comments):
        rows.sort(reverse=True)
        print('%-6s %-8s %s' % ('COM', 'STR', 'FILE'))
        for b, s, p in rows:
            print('%-6d %-8d %s' % (b, s, p))
        print()
        print('TOTAL: %d files with ES text, %d comment blocks, '
              '%d string literals' % (len(rows), total_blocks, total_strings))

    if args.strict and (total_blocks or total_strings):
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
