#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""check_flat_symbols.py — gate: the .mxo must not reference Max symbols that Max
does not export.

A Max bundle is linked with `-undefined dynamic_lookup`: the linker checks nothing
and dyld resolves the missing symbols **by name** in the process' flat namespace.
If one of those symbols is a **function**, dlopen succeeds and the crash appears
only when it is called; if it is a **data** symbol, dlopen fails and Max says:

    unable to load object bundle executable: The bundle "sc3~.mxo" couldn't be loaded.
    (log) dlopen(..., 0x0106): symbol not found in flat namespace '__common_symbols'

That is exactly what happened with the `_sym_*` macros of commonsyms.h (they depend
on the global `_common_symbols`, which Max does NOT export): the fix is `USESYM(foo)`.

What is checked: only the undefined symbols that `nm -m` reports as
`(dynamically looked up)` — the ones the linker left to the flat namespace. Every
other undefined symbol is bound to a named library (libSystem, the frameworks, the
dylibs that travel in the bundle): dyld resolves it by path and refuses to load the
bundle with a clear message if it is missing, so there is nothing to guess here.
That makes the gate both cheap (26 symbols instead of 712) and precise.

Usage:
    python3 check_flat_symbols.py <binary|bundle.mxo> [--max /Applications/Max.app]
                                               [--require-max]
Exit: 0 = all flat-namespace symbols are exported by Max, 1 = some are missing (or
Max is absent while --require-max was passed), 2 = bad usage.
"""
import glob
import os
import subprocess
import sys

MARK = '(dynamically looked up)'


def nm(args):
    try:
        return subprocess.check_output(['nm'] + args, stderr=subprocess.DEVNULL).decode(
            'utf-8', 'replace')
    except subprocess.CalledProcessError:
        return ''


def exported(path):
    out = set()
    for line in nm(['-gU', path]).splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[1] in ('T', 'D', 'B', 'S', 'R', 'C', 'V', 'W'):
            out.add(parts[2])
    return out


def flat_undefined(path):
    """Undefined symbols the linker left for the flat namespace.

    `nm -m -u` prints one line per undefined symbol, e.g.
        (undefined) external _atom_getfloat (from libSystem)
        (undefined) external _class_new (dynamically looked up)
    so only the second kind can be resolved by name (or not) at load time.
    """
    out = set()
    for line in nm(['-m', '-u', path]).splitlines():
        if MARK not in line:
            continue
        out |= {t for t in line.split() if t.startswith('_') and not t.startswith('(')}
    return out


def all_undefined(path):
    return {line.split()[-1] for line in nm(['-u', path]).splitlines() if line.split()}


def main():
    if len(sys.argv) < 2:
        print(__doc__.strip().splitlines()[0])
        return 2
    target = sys.argv[1]
    maxapp = '/Applications/Max.app'
    if '--max' in sys.argv:
        maxapp = sys.argv[sys.argv.index('--max') + 1]

    img = target
    if os.path.isdir(target) or target.endswith('.mxo'):
        img = os.path.join(target, 'Contents', 'MacOS',
                           os.path.basename(target).replace('.mxo', ''))

    flat = flat_undefined(img)
    if not flat:
        print('   symbols: no flat-namespace lookup, nothing to check OK')
        return 0

    have = set()
    maxbin = os.path.join(maxapp, 'Contents', 'MacOS', 'Max')
    if os.path.exists(maxbin):
        have |= exported(maxbin)
        for fw in glob.glob(maxapp + '/Contents/Frameworks/*.framework/Versions/A/*'):
            if os.path.isfile(fw):
                have |= exported(fw)
    else:
        # Without the Max binary there is nothing to compare against: every Max API
        # symbol would look unresolved. Skip the gate instead of failing (the CI has
        # no Max installed); `--require-max` forces the failure where Max does exist.
        why = 'no Max found at %s, and this gate needs its export list' % maxapp
        if '--require-max' in sys.argv:
            print('ERROR: %s' % why, file=sys.stderr)
            return 1
        print('   symbols: SKIPPED (%s; run it on a machine with Max, or pass '
              '--max <path>)' % why, file=sys.stderr)
        return 0

    missing = sorted(s for s in flat if s not in have)
    if missing:
        print('ERROR: %d flat-namespace symbol(s) in %s are not exported by Max (the '
              'bundle will not load):' % (len(missing), img), file=sys.stderr)
        for s in missing:
            print('  ' + s, file=sys.stderr)
        print('Hint: the `_sym_*` macros need `_common_symbols`; use USESYM(x).',
              file=sys.stderr)
        return 1
    print('   symbols: %d flat-namespace of %d undefined, all exported by Max OK'
          % (len(flat), len(all_undefined(img))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
