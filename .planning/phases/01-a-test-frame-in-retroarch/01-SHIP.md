# Phase 01 ship evidence

Runtime: Codex, OpenGSD 1.19.0. Model: GPT-6 (exact session model ID not exposed).

## Gate-budget ratchet

The ship-time ceiling is 1,392 lines, down from 1,500. Todo 018 is complete.

## Commands

- `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*` — exit 0.
- `ctest --preset hygiene -R '^policy\.gate_budget'` — exit 0.
- `cmake --workflow --preset ci` — exit 0.
