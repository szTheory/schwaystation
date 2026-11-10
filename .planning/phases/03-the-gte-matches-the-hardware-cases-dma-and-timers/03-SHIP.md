# Phase 3 shipping evidence

Verified at ship in Codex on 2026-11-10. Phase PR #12 and release PR #13 merged; v0.3.0 is latest.
Publication, attestation/download verification and the latest-install job all passed. No owner action remains.
The command below generated this evidence from fresh GitHub responses, asserting merged/success states,
19 assets and no pending release PR before writing the table.

| Command | Exit | Result | Run or artifact URL |
| --- | --- | --- | --- |
| `gh pr view 12 --json state,mergeCommit,url` | 0 | merged | https://github.com/szTheory/schwaystation/pull/12 |
| `gh pr view 13 --json state,mergeCommit,url` | 0 | merged | https://github.com/szTheory/schwaystation/pull/13 |
| `gh run view 42840139783 --json status,conclusion,url` | 0 | success | https://github.com/szTheory/schwaystation/actions/runs/42840139783 |
| `gh run view 42840838785 --json status,conclusion,url` | 0 | success | https://github.com/szTheory/schwaystation/actions/runs/42840838785 |
| `gh run view 42841474140 --json status,conclusion,url,jobs` | 0 | success | https://github.com/szTheory/schwaystation/actions/runs/42841474140 |
| `gh release view --json tagName,assets,url` | 0 | v0.3.0, 19 assets | https://github.com/szTheory/schwaystation/releases/tag/v0.3.0 |
| `gh pr list --state open --label autorelease: pending --json number` | 0 | no pending release PR |  |

## Corrections and CI efficiency

The first final-head CI failed only the timer negative controls on be32/wasm32. A local multi-argument
emulator wrapper reproduced it; the nested CMake command lost QEMU's sysroot/Wasmtime's preopen. The harness
now serializes the list safely. All four local timer checks and the full native CI workflow passed, then
all final-head hosted jobs passed. The verification report records the reproduction and correction.

The nofp job exceeded its one-minute limit on the release PR and release run, while identical source passed
in seconds in other runs. Logs were inspected; one cancelled run had completed every nofp test. Targeted
job reruns passed without repeating successful build legs; the release rerun finished nofp in 10 seconds.
Existing todo 034 owns its measured three-minute floor at phase 4's first workflow edit. No new todo or gate.

Description edits triggered redundant PR CI during ship. Freeze generated PR bodies before opening where
possible; avoid skip-token ship commits that wedge required checks. Current native/platform checks are
required coverage; no speculative cache, dependency or matrix reduction was added. Assess the next CI
changes using measured critical-path time and duplicate work.
