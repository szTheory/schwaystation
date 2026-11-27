# Phase 10 CI Efficiency

## Current local run

The repaired local `cmake --workflow --preset ci` run used the existing build graph, ran 655/655 normal tests
and 7/7 release checks, and finished its CTest step in 78.09 seconds; release checks took 1.38 seconds.
`libretro.input.movies` remains serialized to preserve full host timing and took 25.94 seconds. The earlier
malformed-PBP expectation correction and this ship-fix batch were each checked with focused selectors before
the one final local workflow. No equivalent Act suite was started.

## Hosted baseline and next measurement

The first Phase 10 PR attempt used pre-repair head `50efc31` and failed before the long BE32 campaign. The
linked WASI producer measured the actual module at 875,971 bytes, so `quality.json` now records that exact
value. CI and Nightly were stopped after the actionable failures were identified. See
[PR CI 44924014258](https://github.com/szTheory/schwaystation/actions/runs/44924014258) and
[Nightly 44924014369](https://github.com/szTheory/schwaystation/actions/runs/44924014369).

Job elapsed-time totals for that cancelled attempt were 1,590 seconds for CI (26.5 runner-minutes) and 580
seconds for Nightly (9.7 runner-minutes); these are sums of job wall times, not billed-dollar estimates. The
CI run lasted 5m28s wall time. Nightly's classifier, vectors, WASI benchmark and three benchmark legs passed;
its 288-second fuzz job was cancelled with the obsolete run. No dollar billing data was available.

The failed jobs exposed five distinct causes: the hygiene workflow-output assertion had not allowed the added
Nightly fields; the old WASI ceiling rejected the measured module; a BE32 assertion cast pointers to integers;
MSVC rejected direct `fopen`; and the protected-base sparse checkout omitted `date_days.cmake`, which its copied
judge includes. These are fixed on the local branch, with a permanent one-line workflow-closure assertion
keeping the 3,901-line gate exactly at its existing cap. Cheap route, quality, workflow-closure and focused
compatibility checks pass. The native local workflow passes all 655+7 checks.

Prior Phase 9 runs show that BE32/QEMU-ppc is the main critical-path candidate: a PR run took 4,870 seconds for
BE32 and 82m12s overall; a main-push run took 3,378 seconds for BE32 and 56m21s overall. A prior Nightly run
took 9m14s, with 552 seconds in the fuzz job and 428 seconds in the campaign step. These are historical
measurements, not Phase 10 results. See
[PR CI 44850283046](https://github.com/szTheory/schwaystation/actions/runs/44850283046),
[main CI 44858133380](https://github.com/szTheory/schwaystation/actions/runs/44858133380), and
[Nightly 44842317071](https://github.com/szTheory/schwaystation/actions/runs/44842317071).

Do not drop or shrink BE32 cases: no evidence supports a quality-preserving reduction yet. On the repaired
head, inspect CI job start/finish times, critical path and runner time, duplicate work, the full Nightly route,
and the release pipeline. Preserve every required lane and compare only runs with matching workflow and workload
fingerprints. `act pull_request --dryrun` passed for CI routing with job-specific local runner mappings; it does
not execute hosted steps. Hosted platforms, artifact handoffs, GitHub permissions, RetroArch, WASI producer size
and release integrations remain hosted-only.

## Second repaired-head attempt

PR CI [44927726830](https://github.com/szTheory/schwaystation/actions/runs/44927726830) and Nightly
[44927726826](https://github.com/szTheory/schwaystation/actions/runs/44927726826) ran on `0cbf4fa`. The WASI
job passed the 875,971-byte quality bound. CI's Linux x64 lane failed after 395 seconds while linking the
optional SDL2 example: `disc_image.c` now calls the PBP provider, but the example target omitted the provider
and its inflater. The other completed platform legs passed; BE32 and ASan were cancelled after the deterministic
failure was diagnosed. The Nightly full route failed in `fuzz.regress.pbp` after 527 seconds because the fuzz
build preset lists campaign executables but not `schwaystation-pbp-fixtures`; the PBP corpus check ran after
earlier 60-second campaigns and could not read its owned fixture.

The interrupted CI attempt used 79.22 summed job-minutes over 14m24s wall time. Nightly used 14.17 summed
job-minutes over 8m58s wall time. These are runner-time sums, not billing-dollar estimates. The longest
completed CI leg was macOS x64 at 724 seconds; BE32 and ASan each had consumed 818 seconds when cancelled, so
this is not a completed critical-path sample. No dollar billing data is exposed here.

The local repair now builds the SDL2 example with the PBP reader and private inflater closure. The Nightly full
route explicitly builds the PBP fixture, and all replay seed files/counts are checked before any timed fuzz
campaign starts. Focused local checks confirm the 14 PBP seeds are generated and the route assertion and
actionlint pass. This keeps every platform, seed, campaign and assertion while moving a cheap consistency check
ahead of long work. Re-run CI and Nightly together on the next repaired head, then record the successful
critical path and runner-time signal before release.

## Regression-check review follow-up

The independent review found the static ordering assertion had a false-positive path: its literal `||` route
operator was being interpreted as regex alternation, and the loader-campaign fragment preceded the seed-output
fragment. The assertion now escapes the operator and requires the post-loop campaign after the seed-output
checks. The real route check and line-budget check pass; a temporary workflow placing the loader campaign inside
the seed loop fails the guard as intended. This validates the cheap preflight before spending another Nightly
campaign run. No Actions coverage changed.

## Disposition

The failures were caused by integration and fixture-wiring gaps, not excessive required coverage. The obsolete
known-failing CI tail was cancelled after the local fix passed; Nightly had already completed. No coverage was
removed. The seed preflight now runs before the expensive campaign loop. Run the next repaired PR head once,
inspect it live and after completion, and record the release-pipeline result before merge.

## Third repaired-head attempt — 2026-11-21

PR CI [44931682851](https://github.com/szTheory/schwaystation/actions/runs/44931682851) and Nightly
[44931682921](https://github.com/szTheory/schwaystation/actions/runs/44931682921) ran on `f6b756e`. Nightly's
route, vectors, and four benchmark jobs passed. Its fuzz job failed 61 seconds after start while building the
PBP source fixture: Ninja had no rule for `fixtures/media/xa.xa`, which the PBP fixture directly depends on.
The `DiscFixtures.cmake` media command generated that file but declared only `media.sha256` as an output.
No replay seeds or timed campaigns ran.

The CI workflow reached the RetroArch and rehearsal jobs without an observed failure; hygiene, route, title,
GPU reference, OpenBIOS, nofp, perf, WASI, and all six native platform builds had passed. BE32 and ASan had each
run 645 seconds when their tails were cancelled, followed by cancellation of the dependent hash-equality,
RetroArch and rehearsal jobs. CI ended `cancelled`, so its aggregate required job was not a passing result.

Nightly used 377 summed runner-seconds (6.28 runner-minutes) over 1m51s wall time. CI used 4,500 summed
runner-seconds (75 runner-minutes) over 11m41s wall time. These are job elapsed-time sums, not billed dollars;
no Actions cost data was available. The longest completed CI lane was 568 seconds, while BE32 and ASan each
consumed 645 seconds before cancellation. This is not a completed CI critical-path sample.

The fix declares `cdda.wav`, `movie.str`, and `xa.xa` as byproducts of the media-generation command. A fresh
local configure with `SCHWAYSTATION_DISC_MEDIA_DIR` unset succeeded, and Ninja reports the missing media files
as outputs of the generator. A permanent static workflow check guards that output closure at the unchanged
3,901-line cap. This cheap local graph check reproduced the clean-host failure without starting the encoder,
sanitizer build, or any fuzz campaign; the now-obsolete CI tail was cancelled after it passed. Independent
review of this CMake change is clean. Run the next exact head once and inspect its live/final critical path and
runner time.

## Fourth repaired-head attempt — 2026-11-21

PR CI [44940420226](https://github.com/szTheory/schwaystation/actions/runs/44940420226) and Nightly
[44940420161](https://github.com/szTheory/schwaystation/actions/runs/44940420161) ran on `8224693`. Nightly
passed its full route. CI passed every job except the final `hash-equality` check: Windows x64's generated
`schwaystation-disc_boot.pbp` digest differed, while the other seven platform digests matched. The source CUE
digest matched across platforms, and all other 112 hash records matched. The comparator failed six seconds
after the BE32 lane completed; no required platform lane or test case was skipped.

CI used 8,481 summed runner-seconds (141.35 runner-minutes) across 22 jobs and 63m32s wall time. BE32 took
3,744 seconds (62m24s) and was the critical path. Nightly used 860 summed runner-seconds (14.33 runner-minutes)
across eight jobs and 10m20s wall time; its fuzz job took 607 seconds, vector job 55 seconds, and four benchmark
jobs 191 seconds combined. These are elapsed runner-time sums, not billed-dollar estimates; Actions exposed no
dollar cost. The full routes and required gates behaved as configured. No safe reduction in coverage, campaigns
or platform legs is supported by these measurements, and moving BE32 behind other lanes would only delay a
successful path without avoiding work on this failure.

The remaining mismatch was isolated to host-zlib output in the PBP fixture generator. A deterministic fixed-
Huffman encoder now preserves the compressed PBP path while keeping the existing raw-block control. Its
generated 64-sector fixture remains below the 1,572,864-byte cap, and the local provider, guest, runner,
libretro and fuzz replay selectors pass. The next exact-head CI/Nightly run is needed to establish cross-platform
hash parity; no quality bound or required CI coverage changed.

## Final repaired-head attempt — 2026-11-21

PR CI [44947489771](https://github.com/szTheory/schwaystation/actions/runs/44947489771) and Nightly
[44947489816](https://github.com/szTheory/schwaystation/actions/runs/44947489816) both passed on exact head
`df838815a5a4afd99f95d50e8caf66813aaa7cb4`. CI passed every required job, including all six native platform
builds, ASan, no-float, BE32, WASI, eight-way hash equality, RetroArch, release rehearsal and the required
aggregate. Nightly passed its full vector, benchmark and fuzz route.

CI took 3,810 seconds (63m30s) wall time and 8,250 summed job-seconds (137.5 runner-minutes) over 22 jobs.
BE32/QEMU took 3,738 seconds (62m18s) and remained the critical path. Hash equality began two seconds after
BE32 finished and completed eight seconds later; the required aggregate finished ten seconds after that. The
comparable prior CI run 44940420226 took 63m32s wall and 8,481 summed job-seconds, with BE32 at 3,744 seconds.
This two-second wall and six-second BE32 difference is normal run variation, not an evidenced workflow speedup.

Nightly took 583 seconds (9m43s) wall and 908 summed job-seconds across eight jobs; fuzz used 574 seconds.
The comparable prior Nightly run 44940420161 took 10m20s wall and 860 summed job-seconds. These totals move
in opposite directions and do not establish a cost or speed improvement. GitHub did not expose dollar billing
data. Every lane, case, hash and campaign remained enabled. The evidence does not support removing or reordering
the BE32 lane.

The final WASI job passed `quality.json`'s 875,971-byte linked-module bound. The previous producer log records
the exact 875,971-byte module; on successful comparisons `tests/cmake/quality.cmake` does not print the new
measurement, and the workflow retains only the hashes artifact rather than the module. The final head
therefore establishes that the linked module is within the bound but does not expose a second exact byte count.
No bound change is justified. The existing `act --dryrun` covered workflow routing; another dry run would add no
coverage because workflow wiring was unchanged after it. Hosted-only runners and artifact/permission integrations
remain established by these successful GitHub runs.

## Release and current-main closeout — 2026-11-22

Current-main CI [44961621903](https://github.com/szTheory/schwaystation/actions/runs/44961621903) and Release
workflow [44961621930](https://github.com/szTheory/schwaystation/actions/runs/44961621930) passed on
`636ac89d4add3192520084a73fa61295a8422bbc`. Main CI took 5,151 seconds (85m51s) wall; GitHub job records sum
to about 9,200 runner-seconds, with BE32/QEMU at 5,095 seconds as the critical path. The release workflow took
2,934 seconds (48m54s) wall. Its `release-ci /` jobs sum to about 6,150 runner-seconds, including 2,520 seconds
for BE32; publish and install-line verification are additional jobs. The workflow published 19 expected
assets, and signature/download verification plus `install-line-latest` passed. These are job elapsed times, not
billed dollars; GitHub did not expose billing data.

The metadata-only release pull request correctly used CI's light route but unexpectedly launched the full
Nightly workflow. Nightly [44961521762](https://github.com/szTheory/schwaystation/actions/runs/44961521762) was
cancelled after 119 seconds wall and 419 summed job-seconds. The local classifier check reproduced the route:
the exact three generated files `.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt` set
`nightly_full=true`. Phase 11 should add a narrow, tested PR route that skips Nightly only when the complete
trusted changed-file set exactly equals those three files. Mixed, incomplete, unknown, renamed, or untrusted
lists keep the full route. Scheduled and manual Nightly remain unchanged. This removes one known irrelevant
workload without dropping tests, fuzzing, platform coverage, or release gates.

Main CI and Release CI remain separate required checks: the completed archive comparison found all 18 product
archives differ, so neither run can substitute for the other. An obsolete main run on ancestor `9bf39aa` was
cancelled after `636ac89` advanced with release metadata; GitHub job records show 6,634 job-seconds were already
spent. This is accrued work, not a counterfactual savings estimate. `act --dryrun` establishes workflow routing;
hosted runners, artifacts, permissions, and release integrations remain verified by the required GitHub runs.
