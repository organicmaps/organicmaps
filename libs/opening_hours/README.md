# opening_hours

Parses and evaluates [OSM `opening_hours`](https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification) values.

- `oh/parser.hpp`, `oh/eval.hpp` — a self-contained C++23 port of the Rust
  [opening-hours-rs](https://github.com/remi-dupre/opening-hours-rs) crate by
  Rémi Dupré (dual-licensed Apache-2.0 / MIT, texts in `oh/LICENSE-*`), kept
  structurally close to the upstream sources so fixes and test vectors can be
  exchanged both ways. When syncing with upstream, record the upstream
  revision in the sync commit message and compare with `git diff -w` (the
  files follow the OM clang-format).
- `oh/convert.*` — converts between the port's AST and the legacy `osmoh` AST.
- `opening_hours.{hpp,cpp}` — the public `osmoh` facade and AST (MIT,
  (c) 2015 Mail.Ru Group), kept for the editor, routing serialization and
  transit; evaluation is delegated to the port.
- `opening_hours_tests/` — the OM test suites: transcribed upstream golden
  vectors (`oh_parser_tests`, `oh_eval_tests`), the legacy `osmoh` suite
  (`osmoh_tests`) and facade regression tests.
