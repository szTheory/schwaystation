# Phase 13 Plan Check

**Phase:** Regions, console revisions and playback  
**Plans verified:** 9  
**Status:** VERIFICATION PASSED

## Coverage summary

| Requirement | Plans | Status |
|---|---|---|
| PROFILE-01 | 05, 06 | Covered: host-only region resolution, explicit override and fallback, PAL/NTSC timing, audio cadence and portable hash regressions. |
| PROFILE-02 | 01, 03, 04, 05, 06, 09 | Covered: all nine legal and nine illegal board/GPU pairs, derived defaults, legacy profile behavior, state validation/serialization/hash, host selection and tested-tuple ledger evidence. |
| BOOT-01 | 07, 08, 09 | Covered: full BIOS boot default, narrow opt-in hook, BIOS and eligible-disc checks in both hosts, and separate standalone EXE/PS-EXE paths. |
| MDEC-01 | 02, 09 | Covered: pinned-source preflight before the bounded edge guest, separate source/decoder/guest/display results, preserved 4-bit and 8-bit baselines and cycle cap. |

All four Phase 13 requirement IDs appear in plan `requirements` metadata. The plans honor the locked context decisions and do not introduce deferred work. Board behavior comes from the shared defaults table; only modeled RAM/GPU/CD behavior changes, while SPU and port defaults remain per-instance identity. Region inference remains host-side. Fast boot is opt-in, requires BIOS-backed eligible content at the host boundary, and leaves the BIOS-free executable path intact.

The MDEC edge case is selected from pinned inputs during execution, as allowed by the context's discretion and research's fixture-confidence note. Its CTest fixture reuses the existing source-unit preflight and maintains the evidence boundaries. No source-audit gap remains in the plan.

## Plan integrity and verification

All nine plans pass `verify.plan-structure`; each has one or two tasks with complete files, actions, verification and done criteria. Plan-level file lists cover the task file lists. Dependencies are acyclic and waves are consistent. The shared CI build tree is serialized through Waves 1–9, and Plan 05 declares Plan 04's board-aware state/hash prerequisite.

Plan 02 now reuses `tests/mdec/color_reference.c::run_color_guest(movie15)` through the existing MDEC-only Nightly lane. Its single task modifies exactly `cmake/MDECReferences.cmake`, `tests/mdec/color_reference.c` and `tests/nightly/inventory.txt`; the existing Nightly file already invokes the shared registration helper. The preset, MDEC-only cache variable, target, inventory and both required fixtures align. Plan 02 depends on Plan 01 and remains in Wave 2. Its expected-incomplete result must report four distinct fields: `source=complete`, `decoder=output-fifo-empty`, `guest=expected-incomplete`, and `display=not-compared`. The plan accepts this only for exact pinned source coverage and the full terminal signature: command `0x380006A0` / 1,696 words, inactive/empty MDEC state, idle DMA0, DMA1 busy with zero words and remain=9/phase=2, DREQ `0x04`, and zero DMA2 words; it also requires eight consecutive empty-FIFO samples after successful run/TTY operations with no fault, zero trailing coefficients, and both cycle and loop bounds. This is a diagnostic result, not playback success; any completed guest still must pass both VRAM and display comparisons.

The bounded `movie15` run has already been observed as a RED outcome at 1,902,230 ticks: source coverage was complete (1,440 blocks / 240 macroblocks / zero trailing), there was no fault, the 1,696-word DMA0 command was exhausted with the MDEC FIFO empty, DMA0 idle and DMA1 waiting at remain=9/phase=2, guest `Done` was absent, and display comparison was not reached. The live plan now also pins the command word `0x380006A0`, DREQ, DMA1/DMA2 word counts, zero trailing coefficients, and both cycle and loop limits for its expected-incomplete classification. Plan structure and parse-only shell syntax pass on this revision; no configure, build, CTest or guest rerun was performed.

The selector assertion expects `mdec.reference.input_preflight`, `mdec.reference.color_assets` and `mdec.reference.edge`. A read-only CTest `--show-only` query of the existing `coverage_movie15` consumer confirmed CTest adds both required fixture setup tests to the selected set. The new assertion therefore accounts for fixture expansion. The task command also checks inventory sort and the 3,901-line cap before configuring/building, then runs only the bounded edge selection. No configure, build or guest test was run in this final bounded review.

Plan 03 Task 2 owns the firmware capability vector in `tests/cdrom/model.c`, adds a `profile-firmware` subcommand to the CI-built `schwaystation-test-cdrom` target, and registers `cdrom.profile.firmware` while keeping `cdrom.region` selected for the region-only string vector. At fixed NTSC-U, its new test checks all nine legal board/GPU profiles: vC0 returns INT5 `(0x11, 0x10)` for Test 22h, and vC1+ preserves the `for U/C` region string. Test 20h's date/version tuple remains unchanged, and Test 51h remains unimplemented because it passes through to a signal processor with no pinned expected response. The exact selector assertion expects `cdrom.profile.firmware`, `cdrom.region`, `core.profile.machine`, `disc.image.legacy_preflight`, and `disc.image.legacy_parity`; the latter two are CTest fixture setup tests pulled in by `core.profile.machine`. No firmware setup fixture is required. This keeps the board-capability assertion on the normal CI target rather than the Nightly-only CD reference target. The vC0 error is supported by [PSX-SPX Test Commands](https://psx-spx.consoledev.net/cdromdrive/) and preparation citation [SCM.02]. Plan 03 structure and both verify commands parse successfully; the selector expansion evidence was supplied from the read-only `--show-only` query and was not rerun here.

Plan 03 Task 1 is scoped to `src/instance.c`, `src/bus.c` and `tests/core/test_core.c`. Source review confirmed the existing defaults table already stores board `ram_ctrl_boot` and `gpu_v0`; `schway_set_profile` applies RAM defaults and validates explicit GPU tuples, and GPU consumers already use the validated flag. The bounded first-boot fix applies the selected RAM value to the initial RAM control register when an explicit board is selected before the first frame, and bus reset reuses that same value. The test sequence checks immediate and reset RAM_SIZE for all nine explicit boards; it then selects PU-7, applies both old-sized and board-zero compatibility profiles, and asserts the inherited non-default 0x0B80 value before and after reset. Explicit GPU-visible checks use PU-7/GPU v0 and PU-18/GPU v2, matching PSX-SYSTEM section 8. Its exact selector graph contains `core.profile.machine`, the two expanded disc fixture setups, and all 11 `gpu.reference.*` / `gpu.span.*` cases (14 names total). The previous `gpu.spans` selector typo was caught before build; the final plan structure and both command bodies parse successfully.

Plan 04 was tightened before execution after a read-only source probe found two gaps: board defaults and the profile tuple check are private to `src/instance.c`, and state validation previously checked only the profile flag mask. The plan now owns `src/instance.c`, validates the serialized board/GPU_V0 pair against the one table during the no-write VALIDATE pass, rejects board zero and values above the supported range, and restores only fixed board-derived identity fields after successful APPLY without overwriting serialized mutable bus registers. The existing runner `--hash-state` computes SHA-256 over the serialized state body, so the canonical board field is included in the actual deterministic state hash; the plan adds a board-sensitive stable-hash comparison. A CRC-resealed mismatched tuple must preserve both serialized instance bytes and the host audio ring on rejection. The focused CTest graph must account for `disc.image.legacy_preflight` and `disc.image.legacy_parity` fixture setup: 5 names for the profile/round-trip/stable-byte set and 7 for profile/round-trip/stable-byte/rejection/visitor. Both tasks run sorted-inventory and 3,901-line preflights before targeted builds; the unrelated 60-second `core.state.record_sweep` is excluded.

Plan 05 was corrected after read-only probes found that `runner/probe.c`'s region guess is static, the host disc probe does not return sector-license data, and the existing PNG test executable does not link the probe code. The host probe will expose serial, disc licence and EXE-marker hints; a pure resolver now lives in `runner/region.c/.h`, is added to the runner's production source list, and can be table-tested without linking the runner through a dedicated `schwaystation-test-region-probe` target. The checked ISO reader will obtain optional sector-4 data, leaving it absent if unavailable or unrecognized; the existing `runner.disc.probe` case will exercise extraction using a runtime-mutated copy of the generated fixture. This keeps all media bytes out of the repository. The plan updates the runner probe's report to consume the shared single-hint conversion, updates `runner.usage.region` for `auto`, and names `runner.region.auto`, `runner.usage.machine`, and `runner.hash.machine`. Before the first runner/test build it checks the sorted inventory, 3,901-line budget and exact two-test graph, builds only the fixture producer, then runs existing `policy.fixture_budget` against those generated assets; Task 2 asserts the exact 14-test graph including disc/region probes, auto and machine cases, all usage cases, and NTSC/PAL/hash regressions. CTest show-only on the pre-change tree found 9 existing Task2 tests, no fixture setup tests, and no Task1 selector yet. Both tasks reuse and reconfigure `build/ci`; the runner CLI path does not need `schwaystation-run-test`.

The current Plan 02 command body passes a fresh `bash -n`, has a matching validate-only record, and its one task's file list exactly matches frontmatter. The other 16 commands are unchanged from the previous validated pass, making 17 current task commands total. Previously supplied path/failing-direction probe results showed no findings for their then-current version; those probes were not rerun in this bounded audit.

Nyquist checks 8a–8e are not applicable because this phase's RESEARCH.md has no `Validation Architecture` section. Per-plan calibrated estimates range from 9,000 to 15,000 tokens, below the 100,000-token budget. All have low confidence because this project has no completed phase actuals for calibration.

CI/CD sequencing keeps cheap inventory, selector, fixture and budget checks ahead of expensive work. The plans retain the full local CI workflow and required hosted Main/Release gates, inspect hosted critical path and runner cost, and use `act` only when it adds evidence or checks changed routing. Docker cleanup is scoped to task-owned, identifiable resources.

The prior review findings are resolved: the MDEC selector set is asserted before its guest runs; the illegal profile tuple matrix is exhaustive; validate-only records are present; Plan 05 follows Plan 04; and Plan 03 no longer lists the unused `src/spu.c` path.

## Plan summary

| Plan | Tasks | Wave | Dependencies | Status |
|---|---:|---:|---|---|
| 13-01 | 2 | 1 | — | Valid |
| 13-02 | 1 | 2 | 13-01 | Valid |
| 13-03 | 2 | 3 | 13-02 | Valid |
| 13-04 | 2 | 4 | 13-03 | Valid |
| 13-05 | 2 | 5 | 13-01, 13-03, 13-04 | Valid |
| 13-06 | 2 | 6 | 13-03, 13-05 | Valid |
| 13-07 | 2 | 7 | 13-04, 13-05, 13-06 | Valid |
| 13-08 | 2 | 8 | 13-05, 13-06, 13-07 | Valid |
| 13-09 | 2 | 9 | 13-01 through 13-08 | Valid |

Plans verified. Run `$gsd-execute-phase 13` when ready to execute.
