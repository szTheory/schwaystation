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

## Disposition

The failures were caused by small compatibility/test wiring faults, not excessive required coverage. The
obsolete known-failing tail was cancelled instead of waiting through the BE32 timeout or Nightly fuzz completion.
No coverage was removed and no safe structural CI speedup was found in this attempt. Run the repaired PR head
once; then record the final exact-head timing, runner-time signal, release-pipeline result and WASI size before
merge.
