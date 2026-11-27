# Phase 10 gate budget

The admitted scope folds only the existing vendored digest extension and Nightly routing. The ceiling stays
3,901 lines. The planning scratch implementation estimated 3,899; the executed classifier, route table and
workflow assertions currently measure 3,900/3,901. This includes a fail-open rename marker, non-PBP fuzz route,
and exact docs/planning-only skip. Keep the bound fixed.

| Counted file | Before | Proposed | Delta |
|---|---:|---:|---:|
| `tests/cmake/quality_controls.cmake` | 425 | 399 | -26 |
| `tests/cmake/triage_controls.cmake` | 178 | 167 | -11 |
| `scripts/classify-changes.sh` | 65 | 80 | +15 |
| `tests/cmake/ci_route_controls.cmake` | 33 | 48 | +15 |
| `tests/cmake/vendored.cmake` | 31 | 36 | +5 |
| `tests/cmake/workflows.cmake` | 96 | 97 | +1 |
| `tests/cmake/info_claims.cmake` | 98 | 98 | 0 |
| Complete gate glob | 3,901 | 3,900 | -1 |

## Preserving reductions

- In `quality_controls.cmake`, add `registry_case(operation, metric, expected, measured, ...)` to perform the
  repeated JSON mutation, write the same registry file, and call the unchanged `compare` assertion. Keep every
  existing case: tighter testcard, exact large integer and above-bound integer, missing floor and ceiling,
  string ceiling, inverted interval, extra key, four invalid reasons forms, and invalid workload hash.
- Reuse the existing `run_block` driver for the performance block rather than keeping its second driver.
  Move its definition before `perf_case`; provide the fixture through `PERF_FIXTURE` in the stub and append the
  same final shell rc assertion. Retain both fixtures and their valid, empty, and over-bound measurements.
- In `triage_controls.cmake`, extract only the duplicated collector invocation into a local `invoke` macro.
  Pass scenario and fail endpoint; preserve rc/stdout/stderr in the caller scope. Keep every success/privacy,
  exact API endpoint/count, stable-output, malformed-response, failed-endpoint, and temp-file assertion.

The scratch quality and triage controls both exited 0. The quality probe initially used relative `ROOT=.` and
failed at an existing `file(RELATIVE_PATH)` call; the corrected invocation uses the repository's absolute root,
as the registered test does. No test assertion or production gate was removed.

## Additions

- Add `pbp_only=true` and `nightly_full=true` to the existing classifier. Only `host/disc_pbp.c`,
  `host/disc_pbp.h`, `tests/disc/pbp.c`, and `tests/fuzz/pbp.c` preserve `pbp_only`; a complete list wholly under
  `docs/` or `.planning/` skips Nightly, while every other list selects full Nightly work. Emit a skip only for a
  valid, nonempty complete list whose count matches. Preserve the existing `heavy` and `release_inputs` outputs
  and release-metadata cases.
- Extend the existing `route` control to assert both outputs (full/false by default). Keep every old row and add
  four PBP-only paths, a non-PBP fuzz path, the Nightly workflow, an unknown path, a count mismatch, and a rename
  marker plus both names so even a rename between allowlisted paths stays full.
  Also retain the previous Nightly path-selection assertions as explicit full-route cases for
  `CMakePresets.json`, `cmake/CDReferences.cmake`, `cmake/MDECReferences.cmake`, `pins.txt`,
  `tests/cdrom/reference.c`, `tests/disc/chd.c`, a non-PBP `tests/fuzz/` child, `tests/mdec/reference.c`,
  `tests/nightly/inventory.txt`, `tests/nightly/tests.cmake`, and `tests/spu/reference.c`.
  The scratch route controls exited 0.
- Add three upstream miniz digest rows to the existing table. Copy that source directory in the existing
  self-test; mutate and restore `rc_hash_disc.c` and `miniz_tinfl.c` in turn using the same registered
  `expect_fail.cmake` assertion. This is four net lines in the measured design. It requires the real upstream
  files before its test can run, so planning does not claim that negative control has already passed.
- Adapt the obsolete Nightly path-filter assertion to the classifier route. Preserve selection of every old
  relevant path through conservative fallback; add those paths to the existing route test when replacing the
  literal path-list check. Keep the existing SPU transfer registration check; check the new `changes` job's
  read-only PR permission and classifier invocation, benchmark/vector dependencies and `nightly_full` guards,
  and fuzz dependency/preflight wiring. The net change is two lines. Append `libretro.content.pbp` to the
  existing mandatory extension-test list in `info_claims.cmake`, with no line-count change. Preserve all
  campaign counts, bounds, inventories, scheduled/manual jobs, and Main and Release requirements. The final
  counted recipe has two lines of headroom; demonstrate another preserving reduction before adding more.

## Execution checks

Run from the repository root before and after each counted change:

`cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake`

Run the existing route, quality, and triage CTests after the driver reduction and again after affected routing
changes. The current `ci.classify_policy`, `policy.quality_metrics`, and `ci.triage` registrations provide the
exact configured commands. Record each retained case and measured total in the plan summary; do not compress,
move, or discard assertions if an implementation differs from this measured recipe.

## Reproducible focused checks

The planning probe applied the preserving helper changes to temporary copies and ran the complete route,
quality, and triage drivers successfully (exit 0). It did not change repository source. After applying the
recipe in Plan 10-01 and the additions in Plans 10-02/10-11/10-09, reproduce those checks from the checkout:

- `cmake --preset ci && ctest --preset ci --no-tests=error -R '^ci[.](classify_policy|triage)$|^policy[.]quality_metrics$' --output-on-failure`
- `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake`
- `ctest --preset ci --no-tests=error -R '^libretro[.](vendored|info[.]claims)([.]self_test)?$' --output-on-failure`
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake`

The workflow-wiring assertion requires the planned Nightly job; the PBP metadata mapping requires its planned
host test; the vendored control requires the admitted upstream files. Their final execution remains planned,
not claimed by the scratch driver result. No hosted workflow ran for this planning probe, so no run URL applies.
