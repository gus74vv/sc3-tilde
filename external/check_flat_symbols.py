#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""check_flat_symbols.py — gate: el .mxo de Max no debe tener símbolos sin resolver.

Un bundle de Max se linkea con `-undefined dynamic_lookup`: el linker NO verifica
que los símbolos existan, los busca en el "flat namespace" del proceso Max al
cargarlo. Si falta un símbolo de **función**, el dlopen pasa y el crash aparece
recién al llamarlo; si falta un símbolo de **datos**, el dlopen falla y Max dice:

    unable to load object bundle executable: The bundle "sc3~.mxo" couldn't be loaded.
    (log) dlopen(..., 0x0106): symbol not found in flat namespace '__common_symbols'

Es exactamente lo que pasó con los macros `_sym_*` de commonsyms.h (dependen del
global `_common_symbols`, que Max NO exporta): se arregla usando `USESYM(foo)`.

El gate compara los símbolos indefinidos del binario contra:
  1. lo que exporta Max (binario + Frameworks),
  2. lo que exporta el SDK de macOS (`*.tbd`: libSystem, frameworks),
  3. lo que exportan las dylibs que viajan en el bundle (libsndfile),
más una allowlist de símbolos que se resuelven por otras vías (clases ObjC,
`dyld_stub_binder`).

Uso:
    python3 check_flat_symbols.py <binario|bundle.mxo> [--max /Applications/Max.app]
Salida: lista de símbolos sin resolver + exit 1 si hay alguno.
"""
import glob
import os
import re
import subprocess
import sys

# Symbols that do not appear in the .tbd but resolve at runtime:
#   · _OBJC_CLASS_$_*  → Foundation/AppKit ObjC classes (dyld loads them from the system)
#   · dyld_stub_binder → dyld injects it into the binary itself
ALLOW = (
    re.compile(r'^_OBJC_(CLASS|METACLASS)_\$_'),
    re.compile(r'^dyld_stub_binder$'),
)


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


def undefined(path):
    out = set()
    for line in nm(['-u', path]).splitlines():
        parts = line.split()
        if parts:
            sym = parts[-1]
            if sym.startswith('_'):
                out.add(sym)
    return out


def tbd_symbols():
    try:
        sdk = subprocess.check_output(['xcrun', '--show-sdk-path']).decode().strip()
    except Exception:
        return set()
    pats = [
        sdk + '/usr/lib/*.tbd',
        sdk + '/System/Library/Frameworks/*.tbd',
        sdk + '/System/Library/Frameworks/*/Versions/A/*.tbd',
        sdk + '/System/Library/Frameworks/*.framework/Versions/A/Frameworks/*.tbd',
    ]
    files = []
    for p in pats:
        files += glob.glob(p)
    syms = set()
    for f in files:
        try:
            txt = open(f, errors='ignore').read()
        except OSError:
            continue
        for m in re.finditer(r'_([A-Za-z_][A-Za-z0-9_$.]*)', txt):
            syms.add('_' + m.group(1))
    return syms


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
        img = os.path.join(target, 'Contents', 'MacOS', os.path.basename(target).replace('.mxo', ''))

    have = set()
    maxbin = os.path.join(maxapp, 'Contents', 'MacOS', 'Max')
    if os.path.exists(maxbin):
        have |= exported(maxbin)
        for fw in glob.glob(maxapp + '/Contents/Frameworks/*.framework/Versions/A/*'):
            if os.path.isfile(fw):
                have |= exported(fw)
    else:
        print('WARNING: no Max found at %s (carrying on without its symbols)' % maxapp,
              file=sys.stderr)
    have |= tbd_symbols()
    # dylibs that travel in the bundle (e.g. libsndfile)
    for lib in glob.glob(os.path.join(os.path.dirname(os.path.dirname(img)), 'Frameworks', '*.dylib')):
        have |= exported(lib)

    missing = sorted(s for s in undefined(img) if s not in have
                     and not any(p.match(s) for p in ALLOW))
    if missing:
        print('ERROR: %d unresolved symbol(s) in %s (Max does not export them ⇒ the '
              'bundle will not load):' % (len(missing), img), file=sys.stderr)
        for s in missing:
            print('  ' + s, file=sys.stderr)
        print('Hint: the `_sym_*` macros need `_common_symbols`; use USESYM(x).',
              file=sys.stderr)
        return 1
    print('   symbols: %d undefined, all resolvable by Max/SDK OK'
          % len(undefined(img)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
