---
phase: 13-regions-console-revisions-and-playback
verified: 2026-11-27T04:19:32Z
status: passed
score: "32/32 must-haves verified"
covered_files:
  - .planning/phases/13-regions-console-revisions-and-playback/13-01-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-01-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-02-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-02-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-03-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-03-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-04-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-04-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-05-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-05-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-06-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-06-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-07-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-07-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-08-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-08-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-09-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-09-SUMMARY.md
  - include/schwaystation.h
  - libretro/libretro.c
  - runner/main.c
  - src/instance.c
  - tests/libretro/host.c
covered_digest: "v4:sha256:626e566c41d54829b55be66544315d140037199ae4c5caa59422f43c5b2b4ede"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 7
  total: 7
  not_honored: []
human_verification: []
---

# Phase 13: Regions, Console Revisions and Playback — Verification

**Phase Goal:** Players can select documented hardware profiles, opt in to the eligible BIOS-shell shortcut, and play supported FMV cases within verified bounds.
**Verified:** 2026-11-27T04:19:32Z
**Status:** passed

## Goal Achievement

All four roadmap success criteria and all 28 plan-level truths were checked against source, tests, build wiring and the current local test outputs. Required artifacts from all nine plans passed the GSD artifact check (21/21). The key-link helper could not follow several C symbol paths because it searches for literal target names; those links were traced through the actual calls and exercised by the focused selectors listed below.

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | PAL timing and audio cadence use the documented integer ratios; region remains separate from board and title identity. | VERIFIED | `src/internal.h`, `src/instance.c`, `runner/region.c`; `core.audio.pairs`, `runner.hash.pal`, `runner.region.auto`, `libretro.host.pal`, and `libretro.host.disc_audio` passed. |
| 2 | Every documented board/GPU tuple is selectable; invalid tuples are rejected atomically; behavior-affecting board values are validated, serialized and hashed. | VERIFIED | Nine-entry defaults table and independent profile fields in `src/instance.c`; state handling in `src/state.c`; `core.profile.machine`, `core.profile.rejected`, and `core.state.profile` passed. Compatibility table has 27 exercised board/region rows; its three preflight/control/ratchet selectors passed. |
| 3 | Full BIOS boot remains default; fast boot skips only the eligible initialized shell handoff and still requires BIOS and eligible disc content. | VERIFIED | `src/cpu.c`, `runner/main.c`, `libretro/libretro.c`; `core.boot.fast`, `core.profile.fast_boot`, `runner.boot.full`, `runner.boot.fast`, `libretro.content.boot`, and `libretro.content.profile` passed. Negative controls cover missing BIOS/disc, ineligible content and nearby PCs. |
| 4 | Supported MDEC references and the bounded movie diagnostic preserve their limits; input coverage is checked before guest execution. | VERIFIED | `cmake/MDECReferences.cmake` wires input and color-asset fixtures before the diagnostic; `tests/mdec/color_reference.c` accepts only the recorded movie15 terminal state. Nightly source preflight, color assets, 4-bit reference, 8-bit regression and edge diagnostic passed 5/5. The edge result reports complete input (1,440 blocks / 240 macroblocks), expected incomplete guest, no display comparison, no fault, and 1,902,230 ticks below the 4,915,200 cap. |

### Plan-Level Must-Haves

| Plan | Truths | Status | Evidence |
|---|---:|---|---|
| 13-01 | 4 | VERIFIED | Public append-only board field and nine-entry table; default, legal pairing, legacy-sized profile, invalid pairing and atomic rejection assertions in `core.profile.machine` and `core.profile.rejected`. |
| 13-02 | 3 | VERIFIED | Fixture-gated Nightly diagnostic; exact expected-incomplete signature; unchanged 4-bit/8-bit controls and cap. All five relevant Nightly tests passed. |
| 13-03 | 3 | VERIFIED | Board defaults flow to modeled GPU/RAM, CD firmware, SPU and port identity. `core.profile.machine` passed; `cdrom.profile.firmware` is part of the green full CI workflow. |
| 13-04 | 2 | VERIFIED | Board identity is included in validation, state bytes and hashes; rejection is validated before mutation. `core.state.profile` passed; the full CI workflow includes `core.state.rejects`. |
| 13-05 | 3 | VERIFIED | Runner resolver precedence and explicit overrides are exercised by `runner.region.probe` and `runner.region.auto`; PAL hash and audio cadence checks passed. |
| 13-06 | 3 | VERIFIED | Libretro applies independent board/region choices at content load; PAL video/audio and profile selectors passed. |
| 13-07 | 3 | VERIFIED | CPU hook checks the exact shell entry, return address, BIOS, disc and opt-in flag; default and adjacent-PC controls passed in `core.boot.fast`. |
| 13-08 | 4 | VERIFIED | Runner and libretro expose full/fast selection; boot eligibility and PS-EXE default paths are checked by the runner/libretro boot selectors in the focused run and full CI. |
| 13-09 | 3 | VERIFIED | 27 profile rows are guarded by the compatibility preflight and ratchet; title ledger is unchanged; roadmap records plan closeout pending aggregate verification. Compatibility selectors passed 3/3. |

### Required Artifacts

| Artifact | Status | Details |
|---|---|---|
| `include/schwaystation.h`, `src/instance.c` | VERIFIED | Public machine constants/field reach atomic validation and applied defaults. |
| `src/state.c`, `tests/core/test_state.c` | VERIFIED | Board and fast-boot fields have versioned serialization, validation and regression coverage. |
| `runner/region.c`, `runner/main.c` | VERIFIED | Content hints and explicit host options resolve into independent core profile values. |
| `libretro/libretro.c`, `tests/libretro/host.c` | VERIFIED | Region, board and fast-boot options are applied during load and checked in-process. |
| `cmake/MDECReferences.cmake`, `tests/mdec/color_reference.c` | VERIFIED | Existing preflight and assets gate the bounded edge case; other MDEC references remain checked. |
| `tests/compat/features.csv`, `scripts/compat-doc.sh` | VERIFIED | Exercised profile rows are validated and documented without claiming game compatibility. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| Public profile API | Board defaults and modeled devices | `schway_set_profile` validates and applies the table used by GPU, bus and CD-ROM paths. | WIRED | `core.profile.machine`, `core.profile.rejected`, and `cdrom.profile.firmware` pass. |
| Instance profile | Save-state codec | Board and flags are included in state validation, bytes and hashes. | WIRED | `core.state.profile` and `core.state.rejects` pass. |
| Host media identity | Runner/libretro region resolver | Serial, license area and executable markers resolve before the core receives a concrete region. | WIRED | `runner.region.auto`, `runner.region.probe`, and `libretro.content.profile` pass. |
| Runner/libretro fast option | CPU shell handoff | Host eligibility sets `SCHWAY_PROFILE_FAST_BOOT`; CPU honors it only at the initialized shell address. | WIRED | Core, runner and libretro boot selectors pass, including negative controls. |
| Nightly test registration | MDEC preflight and bounded runner | Fixture dependencies force input preflight and color assets before `mdec.reference.edge`. | WIRED | Selector closure was exactly the five expected tests; all passed. |
| Compatibility evidence | Ratchet and generated documentation | Feature ledger rows are checked and rendered from current source data. | WIRED | Compatibility checks passed 3/3 and `scripts/compat-doc.sh ... --check` exited 0. |

### Data-Flow Trace

| Input | Flow | Status |
|---|---|---|
| Runner region/board options and disc/EXE identity | Host resolver → concrete `schway_profile.region` and `.board` → core defaults/timing. | FLOWING |
| Libretro option and selected playlist member | Selected member probe → host resolver → core profile at content load. | FLOWING |
| Fast boot request | Host eligibility check → profile flag → exact CPU shell-entry guard. | FLOWING |
| Pinned MDEC source and reference assets | Input preflight → color-asset fixture → bounded guest diagnostic and explicit result classification. | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Region cadence, board/profile/state, runner/libretro profile and boot behavior, and regular MDEC guest controls | `ctest --preset ci --no-tests=error -R '^(core[.]profile[.](machine|rejected|fast_boot)|core[.]state[.]profile|core[.]boot[.]fast|core[.]audio[.]pairs|runner[.](hash[.]pal|region[.]auto|boot[.](full|fast))|libretro[.](host[.](pal|disc_audio)|content[.](boot|profile)|options[.]profile)|mdec[.](guest_4bit|guest_8bit_regression|input_coverage))$' --output-on-failure` | Exit 0; 21/21 including fixture prerequisites, 2.15 seconds total. | PASS |
| Current-release profile, boot, PAL, runner/libretro and MDEC regressions | `ctest --preset ci --no-tests=error -R '^(core[.]profile[.](machine|rejected|fast_boot)|core[.]state[.]profile|core[.]boot[.]fast|core[.]audio[.]pairs|runner[.](hash[.]pal|region[.]auto|region[.]probe)|libretro[.](host[.](pal|disc_audio)|content[.](boot|profile)|options[.]profile)|mdec[.](guest_4bit|guest_8bit_regression|input_coverage))$' --output-on-failure` | Exit 0; 20/20 including fixture prerequisites, 2.03 seconds. | PASS |
| MDEC preflight, 4-bit reference, 8-bit baseline and bounded edge case | `ctest --preset nightly --no-tests=error -R '^mdec[.]reference[.](input_preflight|4bit|8bit_regression|edge)$' --output-on-failure` | Exit 0; 5/5 including color assets, 0.07 seconds total. | PASS |
| Compatibility preflight, controls and ratchet | `ctest --preset ci --no-tests=error -R '^compat[.]features[.](preflight|ratchet|controls)$' --output-on-failure` | Exit 0; 3/3, 0.51 seconds total. | PASS |

### Probe Execution

No phase-declared or conventional `probe-*.sh` scripts were found. Not applicable; the registered CTest checks are the phase's probes.

### Current Released Source Cross-Check

The current exact release head is `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8` (v0.16.0). The shipped source PR CI
run [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) passed on
`775ee6972d30cb911833ebcdae8cd18dd0255b0e`; the full Release CI run
[45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) passed on the exact release
head. Main CI run [45923155084](https://github.com/szTheory/schwaystation/actions/runs/45923155084) was the expected
release-metadata route: its hygiene job passed and the expensive matrix was skipped, so it is not used as full-suite
evidence. The phase's current `instance.c`, runner, libretro and API integration was inspected at the release head;
the later API addition is append-only, and the new patch/cheat host paths leave the existing region, board, boot and
MDEC selectors in the full required CI workflow.

No local tests were rerun for this refresh, per the exact-head closeout instruction. The full exact-head Release CI
provides current cross-platform, integration and release-gate evidence; the earlier focused CTest results remain
historical phase-local evidence.

### Requirements Coverage

| Requirement | Source Plan | Status | Evidence |
|---|---|---|---|
| PROFILE-01 | 13-05, 13-06 | SATISFIED | PAL frame/audio integer ratios and region separation are exercised by the registered runner/libretro tests. The exact release head passed the full Release CI run [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197), which includes the supported build/hash lanes. |
| PROFILE-02 | 13-01, 13-03, 13-04, 13-05, 13-06, 13-09 | SATISFIED | Nine tuples, invalid pairings, legacy caller behavior, state/hash handling, and 27 exercised compatibility rows are covered. |
| BOOT-01 | 13-07, 13-08 | SATISFIED | Full default and fast paths are separately covered; host paths require BIOS and eligible disc executable; no HLE path was added. |
| MDEC-01 | 13-02 | SATISFIED | Source-unit preflight precedes the bounded edge case; 4-bit accepted output and 8-bit deterministic baseline pass; the cycle cap and no-conformance boundary are preserved. |

All four Phase 13 requirements appear in plan frontmatter and map to this phase in `REQUIREMENTS.md`; no orphaned requirement was found.

### Decision Coverage

The GSD decision-coverage check found 7/7 trackable decisions honored by plans, summaries and implementation artifacts. No decision was left unaccounted for.

### Test Quality Audit

| Test area | Linked requirements | Disabled tests | Circular expected values | Assertion strength | Verdict |
|---|---|---:|---:|---|---|
| Core profile, cadence and state | PROFILE-01, PROFILE-02, BOOT-01 | 0 | 0 | Exact ratio, tuple, state-byte and rejection assertions | PASS |
| Runner and libretro profile/boot | PROFILE-01, PROFILE-02, BOOT-01 | 0 | 0 | Multi-step option, content and eligibility behavior | PASS |
| MDEC references and edge diagnostic | MDEC-01 | 0 | 0 | Pinned inputs/assets, exact diagnostic signature and bounded cycle assertions | PASS |
| Compatibility feature ledger | PROFILE-02 | 0 | 0 | Exact key, sorted inventory, row and ratchet checks | PASS |

Expected values are fixed by cited timing documentation or pinned fixture/reference data; the test setup does not regenerate expected output by asking the core under test. No disabled requirement-linked tests or debt markers were found.

### Anti-Patterns Found

None. The changed source/test scan found no unresolved `TBD`, `FIXME` or `XXX` markers. No disabled test pattern was found in the requirement-linked C/CMake tests.

### Human Verification Required

None. The phase has no UI or owner-hardware acceptance condition; runner, libretro and core behavior is exercised by automated tests. Retail-game compatibility and hardware conformance are not claimed by this phase.

## CI and Iteration Efficiency

The shipped source PR CI run [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) passed in 96m06s; BE32 was its critical path at 5,695s, followed by ASan at 1,287s. The full exact-release Release CI run [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) passed in 76m38s, with `release-ci / be32` at 4,110s. Release commit Main CI [45923155084](https://github.com/szTheory/schwaystation/actions/runs/45923155084) took 61s, ran the metadata-route hygiene check in 53s and skipped the expensive matrix as designed; the full Release run supplies the release-head test evidence.

BE32 remains the measured wall-clock bottleneck. The Phase 16 review in [todo 052](../../todos/pending/052-be32-movie-test-shard.md) measured candidate selectors but leaves a shard pending exact hosted CTest/JUnit/fixture/hash coverage and before/after proof. No speculative split or quality reduction is recommended. This report refresh changed no workflow; `act` would not add coverage beyond the completed hosted runs. The current gates preserve all jobs, tests, platforms, assertions and quality bounds. No Docker or task-owned test processes were started for this verification.

## Verification Scope

All four roadmap criteria and all 32 plan-level truths remain satisfied on the current released code. Cross-platform builds, hash agreement and release-pipeline checks are covered by the linked hosted runs. No human verification or owner-hardware step is required; this phase makes no retail-game compatibility or hardware-conformance claim.

---

_Verified: 2026-11-27T03:28:52Z_
_Verifier: the agent (gsd-verifier)_
