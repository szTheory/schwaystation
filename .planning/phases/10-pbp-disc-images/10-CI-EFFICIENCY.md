# Phase 10 CI Efficiency

## Current local run

The final `cmake --workflow --preset ci` run used the existing build graph (no work was needed at build step),
ran 655/655 normal tests and 7/7 release checks, and finished in 78 seconds. The ordinary CTest critical path
was 75.65 seconds; the release checks took 1.35 seconds. The long `libretro.input.movies` consumer remains
serialized to preserve its full host timing and took 25.05 seconds. `policy.quality_metrics` and
`libretro.host` each took about 44 seconds while other tests ran. The malformed-PBP failure was a stale test
expectation, found before this green run and corrected after a focused 0.07-second rerun. One corrected full
local suite was run; no equivalent Act suite was started.

## Hosted baseline and next measurement

The exact Phase 10 head has not run on GitHub yet. Prior Phase 9 runs show that BE32/QEMU-ppc is the main
critical-path candidate: a PR run took 4,870 seconds for BE32 and 82m12s overall; a main-push run took 3,378
seconds for BE32 and 56m21s overall. A prior Nightly run took 9m14s, with 552 seconds in the fuzz job and 428
seconds in the campaign step. These are historical measurements, not Phase 10 results. See
[PR CI 44850283046](https://github.com/szTheory/schwaystation/actions/runs/44850283046),
[main CI 44858133380](https://github.com/szTheory/schwaystation/actions/runs/44858133380), and
[Nightly 44842317071](https://github.com/szTheory/schwaystation/actions/runs/44842317071).

Do not drop or shrink BE32 cases: no evidence supports a quality-preserving reduction yet. At ship, inspect the
exact Phase 10 CI job start/finish times, critical path and raw runner time, plus duplicate work and the Nightly
full route. GitHub's job timing is a cost signal; dollar billing was not available in these records. Preserve
all required lanes and compare only runs with matching workflow and workload fingerprints. `act` routing and
the local native workflow add useful preflight/local coverage, while the required hosted platforms, artifact
handoffs, GitHub permissions, RetroArch and release integrations remain hosted-only.

## Disposition

No safe hosted speedup has been demonstrated for Phase 10. The branch change is committed in one exact head;
the user-corrected malformed-PBP selector was caught locally before hosted CI. The CI and Nightly jobs will run
once on the ship PR, avoiding duplicate full CI and fuzz workloads. The exact-head report and WASI measurement
should be appended after those runs finish, before release completion.
