#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""check_flat_symbols.py -- gate: every flat-namespace symbol of a .mxo must exist in Max.

A Max bundle links its Max API symbols as undefined (the SDK's `-U` list): the linker
does not check them, and dyld looks them up in the Max process's flat namespace at
load time. `nm -m` marks them "(dynamically looked up)". A missing **function** symbol
passes dlopen and crashes on first call; a missing **data** symbol fails dlopen, and
Max reports:

    unable to load object bundle executable: The bundle "sc3~.mxo" couldn't be loaded.
    (log) dlopen(..., 0x0106): symbol not found in flat namespace '__common_symbols'

That is the case of the `_sym_*` macros from commonsyms.h: they read the global
`_common_symbols`, which is in the SDK's `-U` list but which Max does not export.
Use `USESYM(foo)` instead.

Symbols bound to a named library ("from AppKit", "from libSystem", ...) are checked by
the linker against the SDK and are not checked here.

Usage:
    python3 check_flat_symbols.py <binary|bundle.mxo> [--max /Applications/Max.app]
                                  [--require-max]
Exit: 0 if all resolve; 1 and the list of unresolved symbols otherwise; 77 (skipped)
when Max is not found, unless --require-max.
"""
import argparse
import glob
import os
import subprocess
import sys

SKIPPED = 77  # ctest's SKIP_RETURN_CODE


def nm(*args):
    return subprocess.run(['nm', *args], capture_output=True, text=True,
                          errors='replace').stdout


def exported(path):
    """External symbols defined in a Mach-O image."""
    out = set()
    for line in nm('-gUj', path).splitlines():
        out.add(line.strip())
    return out


def flat_lookups(path):
    """Undefined symbols that dyld resolves in the flat namespace."""
    out = set()
    for line in nm('-m', path).splitlines():
        if line.endswith('(dynamically looked up)'):
            out.add(line.split()[-4])
    return out


def max_images(app):
    """The Max binary plus the frameworks and dylibs it ships."""
    fw = os.path.join(app, 'Contents', 'Frameworks')
    images = [os.path.join(app, 'Contents', 'MacOS', 'Max')]
    for f in glob.glob(os.path.join(fw, '*.framework')):
        images.append(os.path.join(f, os.path.basename(f)[:-len('.framework')]))
    images += glob.glob(os.path.join(fw, '*.dylib'))
    return [i for i in images if os.path.isfile(i)]


def main():
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument('target', help='a .mxo bundle or its binary')
    ap.add_argument('--max', default='/Applications/Max.app', help='Max.app to check against')
    ap.add_argument('--require-max', action='store_true', help='fail instead of skip without Max')
    args = ap.parse_args()

    img = args.target
    if os.path.isdir(img):
        img = os.path.join(img, 'Contents', 'MacOS', os.path.basename(img.rstrip('/'))[:-len('.mxo')])

    if not os.path.isfile(os.path.join(args.max, 'Contents', 'MacOS', 'Max')):
        why = 'no Max found at %s' % args.max
        if args.require_max:
            print('ERROR: %s' % why, file=sys.stderr)
            return 1
        print('   symbols: SKIPPED (%s; pass --max <path>)' % why, file=sys.stderr)
        return SKIPPED

    have = set()
    for image in max_images(args.max):
        have |= exported(image)

    wanted = flat_lookups(img)
    missing = sorted(wanted - have)
    if missing:
        print('ERROR: %d symbol(s) of %s not exported by %s (the bundle will not load):'
              % (len(missing), img, args.max), file=sys.stderr)
        for s in missing:
            print('  ' + s, file=sys.stderr)
        print('Hint: the `_sym_*` macros need `_common_symbols`; use USESYM(x).',
              file=sys.stderr)
        return 1
    print('   symbols: %d flat-namespace lookups, all exported by Max' % len(wanted))
    return 0


if __name__ == '__main__':
    sys.exit(main())
