---
phase: 05-the-sound-matches-the-console
verified: 2026-11-15T02:17:54Z
status: passed
score: 74/74 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .planning/phases/05-the-sound-matches-the-console/05-01-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-01-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-02-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-02-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-03-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-03-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-04-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-04-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-05-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-05-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-06-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-06-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-07-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-07-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-08-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-08-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-09-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-09-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-10-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-10-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-11-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-11-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-12-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-12-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-13-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-13-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-14-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-14-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-15-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-15-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-16-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-16-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-17-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-17-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-18-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-18-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-19-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-19-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-20-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-20-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-21-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-21-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-22-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-22-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-23-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-23-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-24-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-24-SUMMARY.md
covered_digest: "v4:sha256:f9b04ca818cceadb7677f6ab3185c53a1905311afb514975b8f223f5e884a313"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 10
  total: 10
  not_honored: []
re_verification:
  previous_status: passed
  previous_score: 74/74
  gaps_closed: []
  gaps_remaining: []
  regressions: []
---

# Phase 5: The sound matches the console — Verification Report

**Phase Goal:** The SPU sounds as the console does: it matches the hardware logs, its audio hashes are identical on every platform including be32 and wasm32, and libretro delivers exactly the runner's audio stream.
**Verified:** 2026-11-15T02:17:54Z
**Status:** passed
**Re-verification:** Refreshed after the ASan timeout change and clean code review. All roadmap and plan truths were rechecked; the prior report had no unresolved gaps.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | The tone fixture emits exact-length PCM and the same audio hash on all eight configured lanes; CPU-free block replay equals guest PCM. | ✓ VERIFIED | Current C code's supplied local CI evidence passed 547/547 CTests and 5/5 release checks; the focused fixture evidence covers exact NTSC/PAL pair counts and guest/table PCM equality. Hash writing checks the three regions and the CI job requires exactly eight uploaded lane files and byte-compares them. The exact-head hosted comparison remains a ship-time check, as plan 05-15 specifies. |
| 2 | Audio tests cover the scheduled SPU model and the nightly comparator checks all seven ordered hardware transfer cases. | ✓ VERIFIED | Audio tests are registered in tests/spu/tests.cmake. The reference setup validates pinned executable/log identities; the comparator parses the ordered seven hardware pass lines and executes the transfer guest. Nightly workflow wiring is present. Current exact-head nightly execution remains pending at ship. |
| 3 | Libretro batch and single-sample callbacks equal the runner PCM; hosted RetroArch boots the tone fixture; idle SPU work stays bounded. | ✓ VERIFIED | tests/libretro/host.c compares callback PCM against runner output in all three regions. The supplied focused be32 host run passed all 20 checks. core.no_busy_loop passed in the supplied local CI evidence. CI requires the content-based RetroArch smoke. Its exact-head hosted result remains pending at ship. |

**Score:** 74/74 must-haves verified (0 present, behavior-unverified).

### Plan-Specific Must-Haves

The 24 plans add 71 observable truths to the three roadmap criteria: plans 05-01 through 05-23 declare three truths each, and 05-24 declares two. All were checked against the current source, test registrations, and supplied execution evidence. The artifact query passed all 61/61 declared artifacts as existing and substantive.

| Plan | Additional truths verified | Status | Evidence |
|---|---|---|---|
| 05-01 | Real library-to-drain tracer; exact CI, sanitizer and wasm inventories; sample counts, ring semantics and rates | ✓ VERIFIED | tests/spu/model.c, tests/tests.cmake and exact inventories; supplied full local CI |
| 05-02 | One scheduled stereo pair per 768 cycles; fixed-width instance-owned state; public layouts and AV rates | ✓ VERIFIED | sched.c schedules spu.c; spu.c generates PCM consumed by the frame ring; model/run tests |
| 05-03 | Guest MMIO width and latch behavior; FIFO/IRQA access; peeks do not consume data or acknowledge IRQ | ✓ VERIFIED | bus.c dispatches to SPU register accessors; RAM helpers raise IRQ9; model and bus tests |
| 05-04 | Pinned transfer guest/log identity; strict seven-case comparator; asset controls gate first callers | ✓ VERIFIED | SPUReferences.cmake checks pins and digests; reference.c validates the ordered log; nightly registration |
| 05-05 | DMA4 upload/readback reaches SPU RAM; initial guest cases; deterministic bounded completion | ✓ VERIFIED | dma.c calls SPU DMA/FIFO helpers; DMA and reference tests |
| 05-06 | BRR history/nibble decode and Gaussian interpolation use defined integer arithmetic and cited table | ✓ VERIFIED | spu_voice.c, spu_tables.inc and independently checked voice tests |
| 05-07 | ADSR phases and volume sweeps; complete 2×128 rate table; edge rates and profile contract | ✓ VERIFIED | spu_envelope.c and baseline envelope tests |
| 05-08 | Noise schedule; envelope-scaled capture and zero CD inputs; muted/silent IRQ behavior | ✓ VERIFIED | spu_noise.c and spu.c event path; noise/model tests |
| 05-09 | 39-tap FIR, centered-filter delay and bounded integer resampling | ✓ VERIFIED | spu_reverb.c, hardware documentation and reverb tests |
| 05-10 | Room feedback updates RAM and wet output from registers | ✓ VERIFIED | spu_reverb.c is called from spu.c; reverb/model tests |
| 05-11 | Authored BRR data reaches the tone guest, mailbox and audio drain | ✓ VERIFIED | fixture assembly includes the block data; fixture build and PCM test |
| 05-12 | Batch and single-sample callbacks equal runner PCM in all regions | ✓ VERIFIED | libretro host test independently runs the core and compares output bytes |
| 05-13 | Hostile transfer bounds and SPU fuzz-seed replay; scheduler progress remains bounded | ✓ VERIFIED | tests/fuzz/spu.c and test_run.c registration; supplied full local CI |
| 05-14 | Active voices, noise and feedback continue after load; invalid state remains atomic and bounded | ✓ VERIFIED | state visitor includes SPU fields; core state tests and supplied full local CI |
| 05-15 | Eight-lane frame-600 hashes and required tone-content smoke are wired; hosted evidence is confirmed at ship | ✓ VERIFIED | hashes.cmake and ci.yml enforce eight-file equality; nightly and RetroArch jobs are required. Plan explicitly reserves exact-head hosted results for ship. |
| 05-16 | All SPU machine fields use the LE visitor; load restores phase and clears delivery buffers; state compatibility rules hold | ✓ VERIFIED | state.c calls the complete SPU visitor; core state tests |
| 05-17 | Runner hashes canonical LE PCM after final mix, drains once and preserves CLI errors | ✓ VERIFIED | runner/main.c drains via schway_get_audio and hashes canonical samples; runner tests |
| 05-18 | CPU-free table path equals guest PCM; frame-600 digest and ENVX predicates are checked | ✓ VERIFIED | fixture.c reads the authored block table and static expected predicates; supplied fixture evidence |
| 05-19 | Hardware documentation cites sources and distinguishes hardware facts from model regressions | ✓ VERIFIED | docs/hardware/spu.md, docs/testing.md and PROVENANCE.md describe separate evidence limits |
| 05-20 | Bounded FIFO/manual transfer, RAM_CTRL/capture, IRQA/STATX and Enable behavior | ✓ VERIFIED | spu.c, irq.c and registered transfer/model tests |
| 05-21 | DMA4 directions/timing, DEV4 FFFFh behavior and complete pinned guest | ✓ VERIFIED | dma.c and spu.c transfer path; reference test; source implementation traced |
| 05-22 | Ordered 24-voice playback, KON/KOFF, loops, ENDX, PMON and stereo mix | ✓ VERIFIED | spu.c schedules voice/envelope paths; voices/model tests |
| 05-23 | SPU owns ATTR; nightly runs the full pinned transfer guest and retains GPU nightly inventory | ✓ VERIFIED | bus mirror is removed; DMA reads SPU-owned state; nightly CMake registration and inventory |
| 05-24 | Workflow registration and lane hash checks fit the fixed 2688-line repository-check ceiling | ✓ VERIFIED | workflow/hash controls remain registered; supplied budget and workflow checks passed |

## Required Artifacts

| Artifact group | Expected | Status | Details |
|---|---|---|---|
| SPU, scheduler, bus and state | Scheduled integer chip model, register/DMA/IRQ path, voice/envelope/noise/reverb and state visitor | ✓ VERIFIED | Every declared artifact passed the 61-artifact existence/substance query; source links were traced to callers and registered tests. |
| Audio and hardware-reference tests | Model, voice, ADSR, noise, reverb, fixture, DMA guest, fuzz and state coverage | ✓ VERIFIED | Tests are included in CMake inventories and CI selectors; supplied local run completed all 547 CTests. |
| Runner, libretro and hosted workflows | Canonical PCM hash, callback equality, eight-lane comparison, nightly transfer and RetroArch tone content | ✓ VERIFIED | Current workflow wiring is present. The timeout and registration checks after the timeout edit both exited 0; hosted exact-head run is pending at ship. |
| Documentation and pins | Hardware facts, claim limits, provenance and identity-pinned assets | ✓ VERIFIED | Source citations and digest validation are present in docs and SPUReferences.cmake. |

## Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| tests/tests.cmake and lane inventories | tests/spu/tests.cmake and tests/spu/inventory.txt | CMake include and exact inventory selection | ✓ WIRED | SPU suite is included and selected in the CI, ASan and wasm inventories. |
| src/sched.c | src/spu.c | Next-deadline scheduling | ✓ WIRED | Scheduler reads schway__spu_next and calls schway__spu_event when due. |
| src/spu.c | src/frame.c public audio ring | Event output and frame drain | ✓ WIRED | SPU event steps chip modules, mixes stereo PCM, and feeds the instance delivery ring. |
| src/bus.c and src/dma.c | src/spu.c and src/irq.c | MMIO, DMA4 FIFO and IRQA helpers | ✓ WIRED | Bus register dispatch and DMA readiness use SPU-owned state; RAM access raises IRQ9 through the IRQ helper. |
| src/spu.c | voice, envelope, noise and reverb modules | Per-sample event calls | ✓ WIRED | The event invokes the chip modules in the scheduled sample path. |
| pins.txt and SPUReferences.cmake | tests/spu/reference.c | Identity-checked guest and hardware log | ✓ WIRED | Pinned asset checks gate the comparator, which validates all seven ordered hardware pass lines. |
| runner/main.c and libretro/libretro.c | schway_get_audio | Hashing and callback delivery | ✓ WIRED | Runner and callbacks consume the public drain; host tests compare PCM byte-for-byte. |
| tests/cmake/hashes.cmake | .github/workflows/ci.yml | Eight lane artifact comparison | ✓ WIRED | CI uploads the lane files and the hash-equality job requires COUNT=8 before byte comparison. |
| .github/workflows/nightly.yml | tests/nightly/tests.cmake | Hardware transfer execution | ✓ WIRED | Nightly CMake workflow registers the full reference guest test. |
| .github/workflows/ci.yml | tests/retroarch/run_retroarch.cmake | Tone executable content smoke | ✓ WIRED | Required RetroArch job boots spu_tone.exe under the stub and compares deterministic video. |

The generic key-link query directly recognized 3 of 40 nested links and did not recognize the remaining C-symbol/CMake edges by file-path text; plans 05-21 and 05-23 place links at top-level, outside the query's nested must_haves input. I manually traced these edges through the code and registrations above rather than treating the query's “Target not referenced” responses as proof of disconnection.

## Data-Flow Trace (Level 4)

| Artifact | Data | Source | Produces real data | Status |
|---|---|---|---|---|
| tests/cmake/hashes.cmake | Frame-600 audio hash rows | Runner executing built spu_tone.exe for each region | Yes; captured from the real runner stream | ✓ FLOWING |
| tests/spu/fixture.c | Guest and table PCM | Guest execution plus CPU-free event/table path using the authored fixture data | Yes; complete PCM is compared | ✓ FLOWING |
| runner/main.c | Audio digest | schway_get_audio from the active instance | Yes; signed stereo PCM is encoded canonically and hashed after final mix | ✓ FLOWING |
| libretro/libretro.c | Callback samples | schway_get_audio from the same core instance | Yes; host test compares batch/single samples against runner bytes | ✓ FLOWING |
| tests/spu/reference.c | Hardware pass lines | Pinned executable and pinned hardware log | Yes; identities are checked before ordered guest output comparison | ✓ FLOWING |

## Behavioral Spot-Checks

These results were supplied for this refresh. No tests were run during verification.

| Behavior | Command | Result | Status |
|---|---|---|---|
| Current C code local CI | cmake --workflow --preset ci | Supplied exit 0 before the workflow-only timeout edit; 547/547 CTests and 5/5 release checks passed | ✓ PASS |
| ASan timeout configuration after edit | cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake | Supplied exit 0 | ✓ PASS |
| SPU job registration after edit | cmake -DROOT=. -DCASE=spu_registration -P tests/cmake/workflows.cmake | Supplied exit 0 | ✓ PASS |
| Big-endian libretro batch equality | docker exec schwaystation-phase5-be32-debug-20261114 qemu-ppc -L /usr/powerpc-linux-gnu build/be32/tests/schwaystation-test-libretro-host build/be32/libretro/schwaystation_libretro.so audio-equality-batch ntsc-j build/be32/fixtures/spu_tone.exe build/be32/tests/libretro-audio-ntsc-j-batch.pcm | Supplied exit 0; all 20 TAP checks passed | ✓ PASS |
| Exact-head hosted CI, nightly and RetroArch | Run after the timeout change is pushed; plan 05-15 confirms these at ship | Pending; older remote-head results are not current evidence | PENDING SHIP |

The old 9-minute ASan cap was reached by [exact-head run 43665013436](https://github.com/szTheory/schwaystation/actions/runs/43665013436) before its 547-test suite completed. The new 15-minute allowance is explicitly provisional; that run supports raising the floor but does not prove 15 minutes is sufficient. The next exact-head hosted run must confirm the new cap. The refreshed code review at 2026-11-15T02:11:21Z covered the five changed test/workflow files and found zero issues.

## Probe Execution

No plan declares a probe and no conventional scripts/*/tests/probe-*.sh file was found.

## Test Quality Audit

| Test group | Requirement | Disabled | Circular expected data | Assertion strength | Verdict |
|---|---|---:|---|---|---|
| SPU model, voice, envelope, noise, reverb and fixture | SOUND-01 | 0 | No self-generated expected values found | Behavioral/value | PASS |
| Pinned transfer comparator | SOUND-01 | 0 | No; expected pass lines come from identity-pinned hardware log | Value/behavioral | PASS |
| Runner/libretro host equality | SOUND-02 | 0 | No; runner and callbacks use separate paths | Byte/value | PASS |
| Eight-lane hash comparison | SOUND-01 | 0 | No for the cross-lane consistency contract; outputs are lane artifacts, not a silicon golden | Value | PASS |

The static disabled-test scan found no skip/todo syntax in linked tests. No test-generated expectation data was found. The fixture digest is a self-authored model regression and is not represented as a hardware PCM capture.

## Requirements Coverage

| Requirement | Source plans | Description | Status | Evidence |
|---|---|---|---|---|
| SOUND-01 | 05-01 through 05-24 | Scheduled integer SPU/audio path, exact sample counts, CPU-free PCM replay, cross-lane hashes and seven hardware transfer cases | ✓ SATISFIED | Current source and test wiring verified; supplied full local CI passed. Cross-lane and nightly hosted results are reserved for exact-head ship checks by plan 05-15. |
| SOUND-02 | 05-01, 05-02, 05-12, 05-14 through 05-16, 05-19, 05-24 | Libretro batch/single PCM equality, hosted tone-content smoke and bounded idle SPU | ✓ SATISFIED | Host integration and CI content wiring verified; supplied local and be32 evidence passed. Exact-head RetroArch result is reserved for ship. |

All 10 trackable decisions in 05-CONTEXT.md are honored (10/10; decision-coverage query exit 0). No additional requirements mapped to Phase 5 were orphaned from the plans.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|

No unreferenced TBD/FIXME/XXX markers, placeholders, empty production implementations, or disabled linked tests were found. The refreshed review of .github/workflows/ci.yml, tests/libretro/host.c, tests/libretro/tests.cmake, tests/runner/tests.cmake and tests/spu/tests.cmake is clean with zero findings.

## Disconfirmation Notes

- The phase does not prove full silicon PCM waveform identity. docs/hardware/spu.md states that no full hardware PCM capture is available. The pinned hardware evidence covers the named transfer cases; tone digests are model regressions and cross-lane consistency checks.
- tests/spu/expected.txt identifies the audio digest as self-authored, not a captured hardware waveform. CPU-free replay checks the fixture/event model against guest PCM.
- SPUReferences.cmake propagates fetch failures with a fatal diagnostic. Controls cover malformed and corrupt identity cases; transport failure injection is not a recurring coverage requirement for this phase.

## Human Verification Required

N/A — this is a core-library/infrastructure phase with no owner-facing UI or manual audio sign-off. Exact-head hosted CI/nightly/RetroArch checks are automated ship checks required by plan 05-15; no owner UAT is needed.

## Gaps Summary

No implementation gaps were found. Current code/artifacts and all 24 plan contracts remain intact. Supplied local CI, the focused be32 callback run, and post-edit workflow checks passed. The ASan timeout is provisional rather than proven sufficient; exact-head hosted CI, nightly and RetroArch outcomes must be recorded at ship, as plan 05-15 explicitly requires.

### Decision Coverage

All trackable CONTEXT.md decisions are reflected in phase artifacts (10/10).

---

Verified: 2026-11-15T02:17:54Z
Verifier: the agent (gsd-verifier)
