# TODO

## Verify

- [ ] Run `.github/workflows/build.yml` once on GitHub.

## Fix

- [ ] `package-info.json` lists `x64`, which Max requires but this package does not ship.
- [ ] `sc_code` writes its index to `externals/sc_code-index.txt`, inside build output. Consider `~/Documents/Max 9/Library` as the default.
- [ ] Bundled resources are copied in a POST_BUILD step, so they refresh only on relink. Editing files inside `SC_APP` in place does not update the bundle.
- [ ] Include the SuperCollider headers as `SYSTEM` to silence `-Wignored-qualifiers` from `SC_Filesystem.hpp`.

## Tests

- [ ] The headless engine and editor suites are not in this tree. The `sc_code` core (lexer, regions, buffer, indent, ...) is pure C++ and could get pytest-driven or ctest unit tests here.

## Cleanup

- [ ] The README size table is maintained by hand.
- [ ] x86_64 and universal builds are untested; the SuperCollider statics are single-arch.
