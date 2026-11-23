#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""check_refs.py — gate: the documentation Max renders must match the objects it documents.

`docs/*.maxref.xml` is what Max shows in the reference viewer and `help/*.maxhelp` is
what the Help key opens; nothing compiles either of them, so a renamed message, a
dropped attribute or a rewired outlet stays there for years and the page goes on
promising something the object does not answer (the same class of bug as a comment in
a `#if 0` block). This gate parses both and compares them with the sources:

  * the file is well-formed and its root is `<c74object>` with the same name as the
    file;
  * the sections Max uses are there (digest, description, inlets, outlets, methods,
    attributes, see also, discussion);
  * every `<method name>` is a selector the object registers with
    `class_addmethod()` (plus the pseudo-methods Max documents for signal inlets and
    outlets: `signal`, `(keyboard)`, …);
  * every `<attribute name>` is a `CLASS_ATTR_*` or an `attr_offset_new()` of the
    object (`style` is exempt: it is a sub-attribute of the menu attributes);
  * every version token `0.x.y` in the page equals the version the object reports,
    so a release that bumps the banner cannot forget the page;
  * `sc3~` documents `error` on outlet n and `post` on outlet n+1: that sentence is
    the user-facing half of the 0.16 outlet reorder;
  * no Spanish text (the public repo is English, see I18N.md).

And the help patches (`HELP_PAGES`), which nothing compiles either:

  * the JSON parses, every patch cord points at an existing box, and `sc3~` has one
    tab per feature (basic, buses, control, input, stopping);
  * the cables follow the 0.16 outlet order: a SIGNAL outlet never feeds a control
    object (`print`, `prepend`, `textedit`) and a TEXT outlet never feeds a signal
    object (`snapshot~`, `number~`, `dac~`…). That is exactly what the deleted GUI
    probe got wrong — two `snapshot~` hanging from the error and post outlets — and
    it is silent: the patch just prints 0;
  * no Spanish text.

Usage: python3 check_refs.py [--dir .]
Exit: 0 = docs match the sources, 1 = a page or a help patch drifted, 2 = bad usage.
"""
import json
import os
import re
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))

# doc file -> source files, pseudo-methods and attributes that are not registered
# by name, and the macro holding the version the page must quote.
PAGES = {
    'sc3~.maxref.xml': {
        'src': ['sc3_tilde.cpp'],
        'methods': {'signal'},        # the signal inlet is documented, not a selector
        'attributes': {'style'},      # sub-attribute of the menu attributes
        'version': 'SC3_TILDE_VERSION',
    },
    'sc_code.maxref.xml': {
        'src': [os.path.join('sc_code', 'sc_code.cpp')],
        'methods': {'(keyboard)', '(mouse)', '(drag)'},  # generic key/mouse/drag
        'attributes': {'style'},
        'version': 'SC_CODE_VERSION',
    },
}

# Max eats these; without them the page renders blank or half empty.
SECTIONS = ('digest', 'description', 'inletlist', 'outletlist', 'methodlist',
            'attributelist', 'seealsolist', 'discussion')

# help patch -> the tabs it must have.
HELP_PAGES = {
    os.path.join('help', 'sc3~.maxhelp'):
        ('basic', 'buses', 'control', 'input', 'stopping'),
    os.path.join('sc_code', 'help', 'sc_code.maxhelp'):
        ('basic', 'themes', 'call aid', 'snippets', 'files'),
}

ACCENTED = re.compile('[áéíóúüñÁÉÍÓÚÜÑ¿¡]')
CONTROL_OBJECT = re.compile(r'^(print|prepend|textedit|message|route|sel)\b')
SIGNAL_OBJECT = re.compile(r'~|^(dac|ezdac|adc)')


def defined(src, pattern):
    """Names the source registers with the given macro pattern."""
    text = open(src).read()
    return set(re.findall(pattern, text))


def macro_value(src, name):
    text = open(src).read()
    m = re.search(r'#define\s+%s\s+"([^"]+)"' % name, text)
    return m.group(1) if m else None


def check(page, root):
    """Returns a list of problems for one reference page."""
    bad = []
    spec = PAGES[page]
    xml = os.path.join(root, 'docs', page)
    if not os.path.exists(xml):
        return ['%s: missing' % page]

    try:
        tree = ET.parse(xml)
    except ET.ParseError as exc:
        return ['%s: not well-formed XML (%s)' % (page, exc)]
    doc = tree.getroot()

    name = os.path.splitext(os.path.splitext(page)[0])[0]
    if doc.tag != 'c74object':
        bad.append('%s: root is <%s>, expected <c74object>' % (page, doc.tag))
    if doc.get('name') != name:
        bad.append('%s: name="%s" does not match the file name' % (page, doc.get('name')))

    for section in SECTIONS:
        if doc.find(section) is None:
            bad.append('%s: <%s> missing' % (page, section))

    selected = set()
    attrs = set()
    for src in spec['src']:
        path = os.path.join(root, src)
        if not os.path.exists(path):
            return ['%s: source %s not found' % (page, src)]
        selected |= defined(path, r'class_addmethod\([^;]*?"(\w+)"')
        attrs |= defined(path, r'CLASS_ATTR_[A-Z_]+\s*\(\s*c\s*,\s*"(\w+)"')
        attrs |= defined(path, r'attr_offset_new\(\s*"(\w+)"')

    for m in doc.iter('method'):
        if m.get('name') and m.get('name') not in selected | spec['methods']:
            bad.append('%s: method `%s` is not registered in %s'
                       % (page, m.get('name'), ' / '.join(spec['src'])))
    for a in doc.iter('attribute'):
        if a.get('name') and a.get('name') not in attrs | spec['attributes']:
            bad.append('%s: attribute `%s` is not a CLASS_ATTR of %s'
                       % (page, a.get('name'), ' / '.join(spec['src'])))

    text = open(xml, encoding='utf-8').read()

    version = None
    for src in spec['src']:
        version = macro_value(os.path.join(root, src), spec['version']) or version
    if version:
        for token in sorted(set(re.findall(r'\b(0\.\d+\.\d+)\b', text))):
            if token != version:
                bad.append('%s: mentions %s but %s is %s'
                           % (page, token, spec['version'], version))

    for line_no, line in enumerate(text.split('\n'), 1):
        if ACCENTED.search(line):
            bad.append('%s:%d: Spanish text in the public docs' % (page, line_no))

    if page.startswith('sc3~'):
        flat = re.sub(r'\s+', ' ', text)
        if 'n+1 is <m>post</m>' not in flat or 'n is <m>error</m>' not in flat:
            bad.append('%s: the outlet order (n = error, n+1 = post) is not documented'
                       % page)
    return bad


def sc3_channels(text):
    """Channels of an [sc3~ n bus] box, or None when the box is not sc3~."""
    if not re.match(r'^sc3~(?![~\w])', text or ''):
        return None
    args = [a for a in (text or '').split()[1:] if not a.startswith('@')]
    try:
        return max(1, int(args[0])) if args else 2
    except ValueError:
        return 2


def check_help_page(rel, tabs, root):
    """Problems of one help patch: structure, outlet wiring and language."""
    bad = []
    wires = [0]
    found = []
    path = os.path.join(root, rel)
    if not os.path.exists(path):
        return ['%s: missing' % rel], 0
    try:
        doc = json.load(open(path))
    except ValueError as exc:
        return ['%s: not valid JSON (%s)' % (rel, exc)], 0

    def walk(patcher, where):
        boxes = {b['box']['id']: b['box'] for b in patcher.get('boxes', [])}
        for b in patcher.get('boxes', []):
            sub = b['box'].get('patcher')
            if sub:
                name = b['box'].get('text') or ''
                found.append(name[2:] if name.startswith('p ') else name)
                walk(sub, where + '/' + name)
        for line in patcher.get('lines', []):
            src, dst = line['patchline']['source'], line['patchline']['destination']
            if src[0] not in boxes or dst[0] not in boxes:
                bad.append('%s%s: patch cord points at a missing box' % (rel, where))
                continue
            wires[0] += 1
            channels = sc3_channels(boxes[src[0]].get('text'))
            if channels is None:
                continue
            target = boxes[dst[0]].get('text') or ''
            if src[1] < channels and CONTROL_OBJECT.match(target):
                bad.append('%s%s: signal outlet %d feeds the control object [%s]'
                           % (rel, where, src[1], target))
            elif src[1] >= channels and SIGNAL_OBJECT.search(target):
                bad.append('%s%s: text outlet %d feeds the signal object [%s]'
                           % (rel, where, src[1], target))

    walk(doc.get('patcher', {}), '')
    for name in tabs:
        if name not in found:
            bad.append('%s: the `%s` tab is missing' % (rel, name))

    for line_no, line in enumerate(open(path, encoding='utf-8').read().split('\n'), 1):
        if ACCENTED.search(line):
            bad.append('%s:%d: Spanish text in the public docs' % (rel, line_no))
    return bad, wires[0]


def main(argv):
    root = HERE
    for i, arg in enumerate(argv):
        if arg == '--dir' and i + 1 < len(argv):
            root = os.path.abspath(argv[i + 1])
        elif arg in ('-h', '--help'):
            print(__doc__.strip().splitlines()[0])
            return 0

    problems = []
    for page in sorted(PAGES):
        problems += check(page, root)

    wires = 0
    for rel, tabs in sorted(HELP_PAGES.items()):
        bad, n = check_help_page(rel, tabs, root)
        problems += bad
        wires += n

    for p in problems:
        print('ERROR: %s' % p, file=sys.stderr)
    if problems:
        print('refs: %d problem(s) in %d page(s) / %d help patch(es)'
              % (len(problems), len(PAGES), len(HELP_PAGES)), file=sys.stderr)
        return 1
    print('   refs: %d page(s) + %d help patch(es) (%d cords) match the sources OK'
          % (len(PAGES), len(HELP_PAGES), wires))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
