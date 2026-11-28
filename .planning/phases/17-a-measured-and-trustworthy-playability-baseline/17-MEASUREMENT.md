# Phase 17 exact producer handoff

## Linked WASI size output

The existing `quality.cmake` artifact path now prints the exact validated integer after its magic and bound checks. The registered `policy.quality_metrics` control requires the 20-byte Wasm fixture to report `20` while also exiting successfully. The unbounded message and rejection cases remain intact; `quality.json` is unchanged.

### Red/green evidence

- Red command: `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$' --output-junit <task-temp>/red.xml` — exit 8. The registered target failed on the new positive assertion because the bounded comparator returned 0 without reporting `20`. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/17-10/red.json --raw` returned `RED_EVIDENCE_OK`; the intended assertion failed, with no fixture or setup failure.
- Green command: `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$'` — exit 0; the registered quality controls passed.
- Focused preflight command: the plan's Task 1 `<automated>` command — exit 0. It includes the artifact controls, registry check, repository-check budget (4,694 lines), RetroArch budget (300 lines), 61 workflow pins, 26 documented timeouts, and `git diff --check origin/main`.
- Inventory disposition: `tests/policy/tests.cmake` and `tests/policy/inventory.txt` are unchanged; the existing `policy.quality_metrics` registration remains the only test entry for these controls.

## Hosted measurement handoff

The exact linked module bytes and comparable pull-request timing are not yet available. The supported full CI producer will run on the reviewed draft pull request after Task 2. Record its PR URL, exact head, `pull_request` run ID/URL, merge SHA, attempt, and pending/completed state here before Plan 17-11 consumes it. Do not infer a new bound from a range pass or from an unfinished run. Keep all `quality.json` values unchanged until the exact producers finish.
