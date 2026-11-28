# Phase 17 CI efficiency evidence

Captured 2026-11-27 from GitHub Actions. This report records observations; it does not authorize or make a workflow, timeout, gate, or test change.

## Collection and integrity

One fresh repository-wide run-list request selected the latest 40 workflow run identities, across CI, Nightly, and Release. All 40 attempts were attempt 1. The paginated job endpoint returned 40/40 complete exact-attempt payloads containing 556 job records. Each payload was checked for its expected total count, unique job IDs and names, run ID, attempt, and source head. All records matched their run. Nineteen skipped/cancelled jobs had no runner label; all other jobs carried the recorded label.

Commands and results:

- sh /private/tmp/schwaystation-plan17-02/collect.sh — exit 0; 40 runs, 40 attempt payloads.
- gh api 'repos/szTheory/schwaystation/actions/runs?per_page=40' --jq '.workflow_runs | length' — exit 0; 40.
- git diff --check origin/main — exit 0.
- Exact-attempt validation over the captured pages — exit 0.
- Read-only failure-log review: gh run view <run-id> --log-failed with XDG_CACHE_HOME in the task temporary directory — exit 0 for all four failed runs. Windows job logs for the latest ten cohort jobs — exit 0.

The run-list response exposed PR source heads, but returned an empty pull_requests field for the PR events; it did not preserve the merge-ref revision that the PR jobs executed. No source head was substituted for that missing execution revision.

Run outcomes: 22 success, 14 cancelled, and 4 failure. Job conclusions: 325 success, 123 skipped, 70 cancelled, and 38 failure. There were no run attempts above 1, so this cohort contains no rerun signal.

## Forty-run inventory

Rows are in the API order, newest first. Source SHAs link to their source commit; for PR rows that is the PR head, not the unavailable merge revision. Job counts are complete for the listed attempt.

| # | Workflow | Event | Branch | Run | Source head | Attempt | Jobs | Run result |
| ---: | --- | --- | --- | --- | --- | ---: | ---: | --- |
| 1 | Release | push | main | [46071535475](https://github.com/szTheory/schwaystation/actions/runs/46071535475) | [`7185b7ad9e`](https://github.com/szTheory/schwaystation/commit/7185b7ad9e6078070b8bbcecd9763fc0a497b894) | 1 | 4 | success |
| 2 | CI | push | main | [46071535381](https://github.com/szTheory/schwaystation/actions/runs/46071535381) | [`7185b7ad9e`](https://github.com/szTheory/schwaystation/commit/7185b7ad9e6078070b8bbcecd9763fc0a497b894) | 1 | 16 | success |
| 3 | Nightly | pull_request | gsd/roadmap | [46071377723](https://github.com/szTheory/schwaystation/actions/runs/46071377723) | [`c0244c18ad`](https://github.com/szTheory/schwaystation/commit/c0244c18adedec41a8ad599e2a0a5d323e14cafd) | 1 | 6 | success |
| 4 | CI | pull_request | gsd/roadmap | [46071377571](https://github.com/szTheory/schwaystation/actions/runs/46071377571) | [`c0244c18ad`](https://github.com/szTheory/schwaystation/commit/c0244c18adedec41a8ad599e2a0a5d323e14cafd) | 1 | 16 | success |
| 5 | Release | push | main | [45949889050](https://github.com/szTheory/schwaystation/actions/runs/45949889050) | [`53f542a8fa`](https://github.com/szTheory/schwaystation/commit/53f542a8fa222919f82c5db64b37b312c60361be) | 1 | 4 | success |
| 6 | CI | push | main | [45949888885](https://github.com/szTheory/schwaystation/actions/runs/45949888885) | [`53f542a8fa`](https://github.com/szTheory/schwaystation/commit/53f542a8fa222919f82c5db64b37b312c60361be) | 1 | 22 | success |
| 7 | Nightly | pull_request | milestone/v2.0-the-whole-library | [45943008324](https://github.com/szTheory/schwaystation/actions/runs/45943008324) | [`5fc49e300d`](https://github.com/szTheory/schwaystation/commit/5fc49e300db7256c12f80d9c07b5d0f4e09ffbf8) | 1 | 8 | success |
| 8 | CI | pull_request | milestone/v2.0-the-whole-library | [45943008239](https://github.com/szTheory/schwaystation/actions/runs/45943008239) | [`5fc49e300d`](https://github.com/szTheory/schwaystation/commit/5fc49e300db7256c12f80d9c07b5d0f4e09ffbf8) | 1 | 22 | success |
| 9 | Nightly | schedule | main | [45939860595](https://github.com/szTheory/schwaystation/actions/runs/45939860595) | [`a2f37b46ea`](https://github.com/szTheory/schwaystation/commit/a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8) | 1 | 8 | success |
| 10 | Release | push | main | [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) | [`a2f37b46ea`](https://github.com/szTheory/schwaystation/commit/a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8) | 1 | 25 | success |
| 11 | CI | push | main | [45923155084](https://github.com/szTheory/schwaystation/actions/runs/45923155084) | [`a2f37b46ea`](https://github.com/szTheory/schwaystation/commit/a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8) | 1 | 16 | success |
| 12 | CI | pull_request | release-please--branches--main | [45923037942](https://github.com/szTheory/schwaystation/actions/runs/45923037942) | [`67d8d4bdc0`](https://github.com/szTheory/schwaystation/commit/67d8d4bdc036392c73a89a9406b396f3a6c65802) | 1 | 16 | success |
| 13 | Nightly | pull_request | release-please--branches--main | [45923037938](https://github.com/szTheory/schwaystation/actions/runs/45923037938) | [`67d8d4bdc0`](https://github.com/szTheory/schwaystation/commit/67d8d4bdc036392c73a89a9406b396f3a6c65802) | 1 | 8 | cancelled |
| 14 | Release | push | main | [45923004092](https://github.com/szTheory/schwaystation/actions/runs/45923004092) | [`39e3ba1a8e`](https://github.com/szTheory/schwaystation/commit/39e3ba1a8e89a15473d9e4e6126dd4034f41e043) | 1 | 4 | success |
| 15 | CI | push | main | [45923003995](https://github.com/szTheory/schwaystation/actions/runs/45923003995) | [`39e3ba1a8e`](https://github.com/szTheory/schwaystation/commit/39e3ba1a8e89a15473d9e4e6126dd4034f41e043) | 1 | 22 | success |
| 16 | CI | pull_request | phase/16-patches-cheats-compatibility | [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) | [`775ee6972d`](https://github.com/szTheory/schwaystation/commit/775ee6972d30cb911833ebcdae8cd18dd0255b0e) | 1 | 22 | success |
| 17 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45913678560](https://github.com/szTheory/schwaystation/actions/runs/45913678560) | [`775ee6972d`](https://github.com/szTheory/schwaystation/commit/775ee6972d30cb911833ebcdae8cd18dd0255b0e) | 1 | 8 | success |
| 18 | CI | pull_request | phase/16-patches-cheats-compatibility | [45902643250](https://github.com/szTheory/schwaystation/actions/runs/45902643250) | [`06bb5da9c5`](https://github.com/szTheory/schwaystation/commit/06bb5da9c5213b81c53d5de606a10af87f9b2a37) | 1 | 22 | success |
| 19 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45902643154](https://github.com/szTheory/schwaystation/actions/runs/45902643154) | [`06bb5da9c5`](https://github.com/szTheory/schwaystation/commit/06bb5da9c5213b81c53d5de606a10af87f9b2a37) | 1 | 8 | success |
| 20 | CI | pull_request | phase/16-patches-cheats-compatibility | [45902217602](https://github.com/szTheory/schwaystation/actions/runs/45902217602) | [`647a11d461`](https://github.com/szTheory/schwaystation/commit/647a11d4619b67842b850872a84d272a355cfd89) | 1 | 22 | cancelled |
| 21 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45902217590](https://github.com/szTheory/schwaystation/actions/runs/45902217590) | [`647a11d461`](https://github.com/szTheory/schwaystation/commit/647a11d4619b67842b850872a84d272a355cfd89) | 1 | 8 | cancelled |
| 22 | CI | pull_request | phase/16-patches-cheats-compatibility | [45897859856](https://github.com/szTheory/schwaystation/actions/runs/45897859856) | [`0cfd7b6735`](https://github.com/szTheory/schwaystation/commit/0cfd7b67351c4df1d11c3e91b49d023771ce6a04) | 1 | 22 | failure |
| 23 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45897859841](https://github.com/szTheory/schwaystation/actions/runs/45897859841) | [`0cfd7b6735`](https://github.com/szTheory/schwaystation/commit/0cfd7b67351c4df1d11c3e91b49d023771ce6a04) | 1 | 8 | success |
| 24 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45897157942](https://github.com/szTheory/schwaystation/actions/runs/45897157942) | [`a920ac0cd6`](https://github.com/szTheory/schwaystation/commit/a920ac0cd6b76a66b2779cd1b098af239dc2cccd) | 1 | 8 | failure |
| 25 | CI | pull_request | phase/16-patches-cheats-compatibility | [45897157925](https://github.com/szTheory/schwaystation/actions/runs/45897157925) | [`a920ac0cd6`](https://github.com/szTheory/schwaystation/commit/a920ac0cd6b76a66b2779cd1b098af239dc2cccd) | 1 | 16 | cancelled |
| 26 | CI | pull_request | phase/16-patches-cheats-compatibility | [45892261219](https://github.com/szTheory/schwaystation/actions/runs/45892261219) | [`2b86f1d0c4`](https://github.com/szTheory/schwaystation/commit/2b86f1d0c43e5fd225b3c4ae5994f245485b732f) | 1 | 22 | cancelled |
| 27 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45892261218](https://github.com/szTheory/schwaystation/actions/runs/45892261218) | [`2b86f1d0c4`](https://github.com/szTheory/schwaystation/commit/2b86f1d0c43e5fd225b3c4ae5994f245485b732f) | 1 | 8 | success |
| 28 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45889677194](https://github.com/szTheory/schwaystation/actions/runs/45889677194) | [`027939e9ab`](https://github.com/szTheory/schwaystation/commit/027939e9ab0234b67cc1b8008bc7fc8c87ea7064) | 1 | 8 | cancelled |
| 29 | CI | pull_request | phase/16-patches-cheats-compatibility | [45889677135](https://github.com/szTheory/schwaystation/actions/runs/45889677135) | [`027939e9ab`](https://github.com/szTheory/schwaystation/commit/027939e9ab0234b67cc1b8008bc7fc8c87ea7064) | 1 | 22 | cancelled |
| 30 | CI | pull_request | phase/16-patches-cheats-compatibility | [45888744986](https://github.com/szTheory/schwaystation/actions/runs/45888744986) | [`c7192056f4`](https://github.com/szTheory/schwaystation/commit/c7192056f4f6eec729be776e57dbc586443a4dfc) | 1 | 22 | cancelled |
| 31 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45888744907](https://github.com/szTheory/schwaystation/actions/runs/45888744907) | [`c7192056f4`](https://github.com/szTheory/schwaystation/commit/c7192056f4f6eec729be776e57dbc586443a4dfc) | 1 | 8 | cancelled |
| 32 | CI | pull_request | phase/16-patches-cheats-compatibility | [45885388357](https://github.com/szTheory/schwaystation/actions/runs/45885388357) | [`25ba6bb749`](https://github.com/szTheory/schwaystation/commit/25ba6bb749c968018e23618df35951a12c092645) | 1 | 22 | cancelled |
| 33 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45885388337](https://github.com/szTheory/schwaystation/actions/runs/45885388337) | [`25ba6bb749`](https://github.com/szTheory/schwaystation/commit/25ba6bb749c968018e23618df35951a12c092645) | 1 | 8 | failure |
| 34 | CI | pull_request | phase/16-patches-cheats-compatibility | [45882317984](https://github.com/szTheory/schwaystation/actions/runs/45882317984) | [`f3d544967e`](https://github.com/szTheory/schwaystation/commit/f3d544967e6efcc2936e3dea5837ad4ef084cf7f) | 1 | 22 | cancelled |
| 35 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45882201424](https://github.com/szTheory/schwaystation/actions/runs/45882201424) | [`f3d544967e`](https://github.com/szTheory/schwaystation/commit/f3d544967e6efcc2936e3dea5837ad4ef084cf7f) | 1 | 8 | cancelled |
| 36 | CI | pull_request | phase/16-patches-cheats-compatibility | [45882201336](https://github.com/szTheory/schwaystation/actions/runs/45882201336) | [`f3d544967e`](https://github.com/szTheory/schwaystation/commit/f3d544967e6efcc2936e3dea5837ad4ef084cf7f) | 1 | 17 | cancelled |
| 37 | Nightly | pull_request | phase/16-patches-cheats-compatibility | [45879737752](https://github.com/szTheory/schwaystation/actions/runs/45879737752) | [`203e7e6544`](https://github.com/szTheory/schwaystation/commit/203e7e6544f115343a0b394328f7fd465d4b4a2a) | 1 | 8 | cancelled |
| 38 | CI | pull_request | phase/16-patches-cheats-compatibility | [45879737052](https://github.com/szTheory/schwaystation/actions/runs/45879737052) | [`203e7e6544`](https://github.com/szTheory/schwaystation/commit/203e7e6544f115343a0b394328f7fd465d4b4a2a) | 1 | 16 | failure |
| 39 | Nightly | schedule | main | [45755124731](https://github.com/szTheory/schwaystation/actions/runs/45755124731) | [`b2a19abf8a`](https://github.com/szTheory/schwaystation/commit/b2a19abf8a53b7392241cc086ea531d76b31b3d6) | 1 | 8 | success |
| 40 | CI | push | main | [45698034001](https://github.com/szTheory/schwaystation/actions/runs/45698034001) | [`b2a19abf8a`](https://github.com/szTheory/schwaystation/commit/b2a19abf8a53b7392241cc086ea531d76b31b3d6) | 1 | 16 | success |


## Exact comparison groups

The Phase 7 workload projection rules were applied conservatively: event, route, selected workflow/build/test inputs, matrix, and actual runner labels must agree. For source revisions available in the local Git object database, selected-input differences were checked at the exact run head. No nearby revision was used.

| Route | Runs and exact source | Group result |
| --- | --- | --- |
| CI pull_request | 15 runs in the inventory | Excluded: the Actions run records do not expose the merge-ref SHA used for the run. |
| Nightly pull_request | 14 runs in the inventory | Excluded for the same missing merge-ref SHA. |
| CI push, full route | Runs 45949888885 and 45923003995; 22 jobs each | Separate singleton fingerprints. Their selected inputs differ in .github/workflows/ci.yml, scripts/classify-changes.sh, and tests/cmake/ci_route_controls.cmake. |
| CI push, light route | Runs 46071535381, 45923155084, and 45698034001; 16 jobs each | No repeated fingerprint. 46071535381 and 45923155084 differ in the same three selected files; 45923155084 and 45698034001 differ in 31 selected inputs. |
| Nightly schedule | Runs 45939860595 and 45755124731; 8 jobs each | Separate singleton fingerprints. Thirty-one selected inputs changed, including both workflow files, CMake inputs, pins, scripts, and test inputs. |
| Release push, no release created | Runs 46071535475, 45949889050, and 45923004092; 4 jobs each | The exact release-please group is runs 46071535475 and 45949889050: release.yml and release metadata inputs are identical, as are event, main route, job identity, and ubuntu-26.04 label. Run 45923004092 is a singleton because its manifest, changelog, and version inputs are from 0.15.0 rather than 0.16.0. The reusable CI, publish, and latest-install jobs were skipped in all three. |
| Release push, release created | Run 45923155197; 25 jobs | One full-release observation. No same-route peer in the 40-run window. |

The only repeated exact workload group is the Release no-release-created route for release-please, with two matching samples. The CI workflow was not called on those runs, so its unrelated source changes do not enter this job-level projection. The third no-release-created run has changed release metadata and remains separate. This group is not evidence of a release-pipeline speedup: no workflow change separates the observations.

## Critical path, job duration, and queue

For the exact Release no-release-created group, release-please is the only active job; the other three jobs are skipped. Its two matching job durations are 9 seconds in [run 45949889050](https://github.com/szTheory/schwaystation/actions/runs/45949889050/job/140343048693) and 11 seconds in [run 46071535475](https://github.com/szTheory/schwaystation/actions/runs/46071535475/job/140749369826). Thus the route critical path is the release-please job; the slowest matching sample is 11 seconds. Each run had a 2-second interval from run creation to its first assigned job start. Run 45923004092 took 22 seconds ([job](https://github.com/szTheory/schwaystation/actions/runs/45923004092/job/140257867424)), but its 0.15 release metadata inputs make it a separate singleton. These are raw observations, not a before/after result.

The one full-release run had this longest dependency chain, calculated by summing exact-attempt job durations along the Release and reusable-CI DAG:

release-please 10s → changes 6s → openbios 55s → be32 4,110s → hash-equality 6s → CI required 4s → publish 380s → install-line-latest 10s = 4,581s.

Each value is sourced by its exact job record: [release-please](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140258345651), [changes](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140258403234), [openbios](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140258444026), [be32](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140258731538), [hash-equality](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140279889147), [CI required](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140279932857), [publish](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140279962715), and [install-line-latest](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140281910661). The run wall interval from API creation to completion was 4,598 seconds. This is one route observation, not a comparable CI baseline. PR critical paths are not projected because the exact merge revision and its workflow DAG are absent from the run metadata.

That full-release run accumulated 9,433 seconds of labeled job time across overlapping jobs, divided by 4,598 seconds of run wall time: 2.05 runner-capacity equivalents. This estimates hosted job occupancy, not CPU utilization. A consistent JUnit summed-test-time series is unavailable for the cohort because historical JUnit files were not retained as run artifacts. CPU utilization and billing-weighted cost are unavailable from the collected Actions records.

## Reliability and repeated work

Failure classes, current timeout dispositions, the exact ten-job Windows DLL sample, and the runner/checkpoint repetition audit are in [17-RELIABILITY.md](17-RELIABILITY.md).

The exact-group observations support one timeout arithmetic check only: release-please had two matching samples, slowest 11 seconds. Its formula is ceil(2 × 11 / 60) = 1 minute; the current 5-minute setup floor keeps the effective value at 5 minutes. No speedup or timeout change is proposed. Other routes either have no repeated exact fingerprint, are single observations, or lack the PR execution SHA.
