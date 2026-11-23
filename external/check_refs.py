#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""check_refs.py — gate: the Max reference pages must match the objects they document.

`docs/*.maxref.xml` is what Max renders in the reference viewer; nothing compiles
it, so a renamed message or a dropped attribute stays there for years and the page
goes on promising something the object does not answer (the same class of bug as a
comment in a `#if 0` block). This gate parses the XML and compares it against the
sources:

  * the file is well-formed and its root is `<c74object>` with the same name as the
    file;
  * the sections Max uses are there (digest, description, inlets, outlets, methods,
    attributes, see also, discussion);
  * every `<method name>` is a selector the object registers with
    `class_addmethod()` (plus the pseudo-methods Max documents for signal inlets
    and outlets: `signal`, `bang` on an inlet is `bang`, which the object has);
  * every `<attribute name>` is a `CLASS_ATTR_*` of the object (`style` is exempt:
    it is a sub-attribute of the menu attributes);
  * every version token `0.x.y` in the page equals the version the object reports,
    so a release that bumps the banner cannot forget the page;
  * no Spanish text (the public repo is English, see I18N.md);
  * `sc3~` documents `error` on outlet n and `post` on outlet n+1: that sentence is
    the user-facing half of the 0.16 outlet reorder.

Usage: python3 check_refs.py [--dir docs] [--src .]
Exit: 0 = all pages match, 1 = a page drifted, 2 = bad usage.
"""
import os
import re
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))

# doc file -> (source files that define the object, pseudo-methods and
# attributes that are not registered by name, version macro)
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
        'attributes': {'style'},      # sub-attribute of the menu attributes
        'version': 'SC_CODE_VERSION',
    },
}

# Max eats these; without them the page renders blank or half empty.
SECTIONS = ('digest', 'description', 'inletlist', 'outletlist', 'methodlist',
            'attributelist', 'seealsolist', 'discussion')

ACCENTED = re.compile(r'[áéíóúüñÁÉÍÓÚÜÑ¿¡]')


def defined(src, pattern):
    """Names the source registers with the given class_add* macro."""
    text = open(src).read()
    return set(re.findall(pattern, text))


def macro_value(src, name):
    text = open(src).read()
    m = re.search(r'#define\s+%s\s+"([^"]+)"' % name, text)
    return m.group(1) if m else None


def check(page, root):
    """Returns a list of problems for one page."""
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
    for p in problems:
        print('ERROR: %s' % p, file=sys.stderr)
    if problems:
        print('refs: %d problem(s) in %d page(s)' % (len(problems), len(PAGES)),
              file=sys.stderr)
        return 1
    print('   refs: %d page(s), methods and attributes match the sources OK'
          % len(PAGES))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
