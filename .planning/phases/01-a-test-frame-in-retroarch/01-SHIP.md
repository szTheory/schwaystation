# Phase 01 ship evidence

Runtime: Codex, OpenGSD 1.19.0. Model: GPT-6 (exact session model ID not exposed).

## Gate-budget ratchet

The ship-time ceiling is 1,392 lines, down from 1,500. Todo 018 is complete.

## Commands

- `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*` — exit 0.
- `ctest --preset hygiene -R '^policy\.gate_budget'` — exit 0.
- `cmake --workflow --preset ci` — exit 0.

## Pull request and release tracking

Phase PR: https://github.com/szTheory/schwaystation/pull/4

The PR body records current-head CI, merge, release PR and publication evidence
as commands, exit statuses and run URLs. The release workflow also runs the
README install line against the latest download. The agent waits for these
outcomes within ship; the separate CI tail does not run in this session.

Code review preceded verification. No second review is run at ship under AGENTS
rule 7. All commit gate-status trailers were absent, so the informational TDD
audit is suppressed. No ship capability hooks were active.
