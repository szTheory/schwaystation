# Phase 17 BE32 shard review

**Disposition:** Keep the production BE32 job monolithic. The selectors and preservation contract can be proved locally, but there is no hosted run of the three-shard candidate and no comparable before/after critical-path pair.

## Current portable selector proof

The existing configured `build/ci` graph was sufficient; no configure, build, or test run was needed. Each command below exited 0 and used `--show-only`, so it enumerated tests without executing them:

```sh
ctest --preset ci --show-only=json-v1 -L '^portable$'
ctest --preset ci --show-only=json-v1 -L '^portable$' -R '^libretro[.]host$'
ctest --preset ci --show-only=json-v1 -L '^portable$' -R '^libretro[.]input[.]movies$'
ctest --preset ci --show-only=json-v1 -L '^portable$' -E '^libretro[.](host|input[.]movies)$'
```

The configured portable set has 694 unique tests. A read-only comparison against the per-directory inventories selected by `tests/inventory/ci.txt` exited 0: every selected inventory was bytewise sorted, names were unique, all selected graph entries had the `portable` label, and the exact partition was audio 1 + movie 1 + broad 692 = 694. The three partitions are pairwise disjoint and their union equals the inventory set. In particular, `libretro.host` and `libretro.input.movies` each appear exactly once.

The existing `tests/cmake/check_junit.cmake` validates expected names in one report and rejects duplicate, missing, failed, errored, or skipped expected tests. It does not reject an unexpected extra testcase and cannot prove ownership or the union across three reports. A candidate therefore needs a one-off union check before any long run:

1. Build the expected name-to-owner map from the portable rows in the directories selected by `tests/inventory/ci.txt`; assign `libretro.host` to audio, `libretro.input.movies` to movie, and all remaining portable names to broad.
2. Parse all three JUnit XML files. For each testcase require a name, `status="run"`, and no `failure`, `error`, or `skipped` child. Reject a name absent from the expected map or present in a report other than its assigned owner.
3. Count names across all reports and require the exact expected set with count one for every name. This catches missing, duplicate, extra, and cross-shard names, including a wrong-shard testcase even when the aggregate count is unchanged.
4. Before a hosted run, generate disposable synthetic reports from the exact current names, accept the complete union, then mutate one case at a time to remove a name, duplicate a name, add an extra name, route a name to the wrong shard, mark a testcase failed, and mark a testcase skipped. Delete the synthetic reports after the proof.

The one-off Python/ElementTree proof used the current CTest JSON names and inventory rows. It exited 0: the complete 694-name union was accepted; each of the six malformed cases was rejected. The synthetic reports and script were task-local under `/private/tmp/schwaystation-phase17-05-20261128` and are removed at closeout. No permanent union harness or shipping gate was added.

## Candidate execution contract

Each hosted job must have a private workspace and fixture tree. Use a derived `be32-chd` configuration with the same cache values and toolchain as the current preset; only the build root and the test selector/report path vary. Do not point two jobs at one writable build tree.

| Job | Exact test selection | Private build and fixture roots | Required fixture targets |
| --- | --- | --- | --- |
| broad | `ctest --preset <job-preset> -L '^portable$' -E '^libretro[.](host|input[.]movies)$'` | `build/be32-broad/{openbios,disc-media,fixtures,tests}` | `schwaystation-fixtures`, `schwaystation-disc-fixtures`, `schwaystation-legacy-disc-fixtures`, checked CHD fixture generation |
| audio | `ctest --preset <job-preset> -L '^portable$' -R '^libretro[.]host$'` | `build/be32-audio/{openbios,disc-media,fixtures,tests}` | `schwaystation-fixtures`; includes `spu_tone.exe`, `schwaystation-run-test`, the libretro host and module |
| movie | `ctest --preset <job-preset> -L '^portable$' -R '^libretro[.]input[.]movies$'` | `build/be32-movie/{openbios,disc-media,fixtures,tests}` | `schwaystation-fixtures`; includes `pad_counter.exe`, the libretro host and module |

The derived configuration must preserve `cmake/toolchains/ppc-linux-gnu.cmake`, PowerPC GCC and QEMU, Release mode, warnings-as-errors, `SCHWAYSTATION_TIMEOUT_SCALE=8`, `SCHWAYSTATION_INVENTORY=ci`, `SCHWAYSTATION_CHD=ON`, and `SCHWAYSTATION_BUILD_CHD_TOOL=OFF`. In each job, place the copied native generator at that job's `tests/schwaystation-chd-tool` and set `SCHWAYSTATION_CHD_TOOL_EXECUTABLE` to the matching private path. Keep the current 109-minute job limit and every registered assertion and per-test timeout, including `libretro.host` 360 seconds and `libretro.input.movies` 240 seconds before the configured PowerPC scale. The host test is not a CTest fixture setup; the movie test remains `RUN_SERIAL` within its own job.

All jobs must download the same run-local `openbios-handoff` and `disc-media-handoff`, not regenerate different inputs. Verify both OpenBIOS images against `tests/built/expected.txt` and `tests/built/fast-expected.txt`: the pinned 512 KiB hashes are `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8` and `46a56670f8548d20174b144f8dd8c6898ce7008888e0eec84b2a7d57926c0c78`; the source is nugget commit `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9`. Point each job's `SCHWAYSTATION_DISC_MEDIA_DIR` at its own downloaded media directory. Its `media.sha256` must identify `cdda.wav`, `movie.str`, and `xa.xa`; `cmake/DiscFixtures.cmake` checks those digests before copying them. The shared producer is pinned `psxavenc` commit `bdc24e897ed09d2fd3d64cdbadc625b0673c76db`, archive SHA-256 `622ede1e67d5f3f0228672aa62b8575a2b2d8c745ec7b6c484f31f8f5be3e4b0`.

The broad job retains `runner.hashes_file` and its `disc_fmv_predicates` fixture requirement. It must upload its private `hashes.txt` as the required `hashes-be32` artifact with `if-no-files-found: error`. `hash-equality` must still download the `hashes-*` artifacts, compare exactly eight files byte-for-byte, and wait for every shard plus the other existing build legs. `ci-required` must require each shard and the hash comparison. No output may become optional or silently disappear. These ownership links are present today in `.github/workflows/ci.yml` (`be32` → `hash-equality` → `CI required`) and `tests/runner/tests.cmake`.

## Hosted observations and comparison

The latest successful heavy Main CI run in the bounded 40-run inventory is [run 45949888885](https://github.com/szTheory/schwaystation/actions/runs/45949888885), source `53f542a8fa222919f82c5db64b37b312c60361be`, on `ubuntu-26.04`. Its BE32 job [140343426896](https://github.com/szTheory/schwaystation/actions/runs/45949888885/job/140343426896) passed all 694 portable tests. From the job log, `libretro.host` took 1,406.60 seconds, `libretro.input.movies` took 1,018.94 seconds, and total CTest time was 3,974.74 seconds. The movie test started immediately after the host completed; as `RUN_SERIAL`, it began after the other test work had finished. The pre-movie portion was 2,955.80 seconds by subtraction, but that overlaps the host test and is not an isolated broad-shard duration.

The BE32 job wall time was 4,113 seconds. Its pre-test steps for checkout, artifact download and verification, native CHD generator build, cross-toolchain install, and target/fixture configuration totaled 100 seconds. The combined build-and-test step took 4,009 seconds; CTest reports 3,974.74 seconds, leaving about 34 seconds for the build and test startup. A three-job candidate would repeat the measured 100-second setup in the two added workspaces, or about 200 extra runner-seconds before their separate build costs. Those build costs and any isolated shard durations are estimates until a candidate run measures them; this is not a billing estimate.

The same run's required CI dependency path was `changes` (6 s) → `openbios` (59 s) → `be32` (4,113 s) → `hash-equality` (7 s) → `CI required` (3 s). `CI required` passed 4,199 seconds after run creation. The first active job started one second after run creation; the measured transitions from job creation to assignment were one to two seconds on this run. Across 22 active jobs, summed wall durations were 8,552 runner-seconds (142.5 runner-minutes); this is raw occupancy, not billing cost or CPU use. These are one-run observations, not a speedup cohort.

The full Main runs 45949888885 and 45923003995 are separate singleton fingerprints: the selected workflow inputs differ, so they do not form a comparable before/after pair. Other newest runs were light or skipped the full matrix. The hosted BE32 report has no three-shard candidate, no three-report JUnit union, and no isolated broad runtime. `act --dryrun` would show routing only and cannot establish PowerPC/QEMU behavior or measured hosted timing; it was not run because workflow routing was not changed.

The valid hosted acceptance remains: same exact source and runner/toolchain/workload configuration apart from the declared shard topology; all three JUnit reports pass the strict exact-name union; every fixture, artifact identity, hash, timeout, assertion, and required dependency is preserved; the eight-file hash equality check and all required gates pass; and a comparable Main CI run shows a strict reduction in total required-DAG critical-path seconds after duplicated setup. Until that evidence exists, leave the production job and `hashes-be32` route unchanged.

## Commands and outcomes

| Command | Result |
| --- | --- |
| `ctest --preset ci --show-only=json-v1 -L '^portable$'` | Exit 0; 694 current portable tests enumerated. |
| Three selector commands above and read-only inventory/label set comparison | Exit 0; disjoint 1/1/692 partition equals the current portable inventory. |
| One-off synthetic JUnit union procedure | Exit 0; positive case accepted and missing/duplicate/extra/wrong-shard/failure/skipped cases rejected. |
| `curl -fsS -H 'Accept: application/vnd.github+json' -H 'X-GitHub-Api-Version: 2022-11-28' 'https://api.github.com/repos/szTheory/schwaystation/actions/runs/45949888885/jobs?per_page=100'` | Exit 0; exact run/job identities and timestamps. |
| `XDG_CACHE_HOME=/private/tmp/schwaystation-phase17-05-20261128/cache gh run view 45949888885 --repo szTheory/schwaystation --job 140343426896 --log` | Exit 0; exact BE32 test durations and 694/694 result. |
| `git diff --check origin/main` | Exit 0 after the documentation changes. |

No product tests or builds were run. No CI workflow, test inventory, timeout, artifact, or shipping gate was changed.
