# Phase 13: Regions, console revisions and playback — Research

**Researched:** 2026-11-23  
**Domain:** Machine profiles, region selection, BIOS-shell boot and bounded FMV evidence  
**Confidence:** HIGH for the documented profile and host seams; MEDIUM for the exact minimum owned fast-boot and MDEC fixtures, which the planner/executor should settle from the existing fixtures.

## Phase constraints

The phase context at `13-CONTEXT.md` is authoritative. Preserve the size-tagged profile API and legacy callers; keep region, board and title serial independent; accept only documented board/GPU tuples; serialize and hash behavior-affecting profile identity; and keep content/file detection in hosts. Full BIOS boot stays the default. Fast boot is an opt-in hook after normal BIOS initialization for eligible disc executables, not HLE or the existing BIOS-free standalone `--exe` path. Preserve the accepted v1.0 MDEC evidence boundary, exact 4-bit result, frozen deterministic 8-bit result and 4,915,200-cycle cap.

## Hardware and host facts

- `PSX-SYSTEM.md` §8 documents nine supported boards: PU-7, early/late PU-8, PU-18/20/22/23, PM-41 and PM-41(2). GPU v0 applies to PU-7 and early PU-8; the documented v2 behavior applies to the other supported profiles, including PM-41(2)'s CXD9500. The board also determines other modeled defaults such as CD firmware, RAM-control boot value and port availability. Do not infer SCPH model numbers from the board table where the source does not map them.
- Region selects the console's clock/timing and CD firmware region response; it is independent of board and title identity. The existing PAL model has 314-line frames and integer clock/audio cadence. Host `auto` should resolve concrete region from serial, then licence, then EXE marker; disagreements are diagnostics, explicit selection wins, and missing identity retains the current NTSC-U fallback. `schwaystation-run` currently accepts only concrete regions and defaults to NTSC-U; libretro's region option likewise lacks `auto`. Core code should receive a concrete region and must not inspect files or identity.
- The documented shell hook is `0x80030000`, reached after BIOS kernel initialization. The fast path should intercept only that handoff for an eligible disc executable, preserve BIOS initialization and clock rules, and leave the BIOS shell as the default path. The standalone runner `--exe` path already uses a separate BIOS-free stub and must remain distinct.
- The preparation notes distinguish MDEC compressed source coverage, decoded output, guest progress/completion, displayed pixels and hardware fidelity. A complete compressed source is not proof that the decoder or guest completed. `cmake/MDECReferences.cmake` already runs `mdec.reference.input_preflight` before the exact 4-bit and 8-bit regression cases using a CTest fixture; extend/reuse this ordering instead of adding a parallel preflight framework.

## Implementation seams and compatibility hazards

- `include/schwaystation.h` exposes API 7 and a `schway_profile` with `size`, `region`, `flags` and `disc_flags`; append any new public profile field. `src/instance.c:345+` checks caller size, allowed flags and profile lock. Old-sized callers and `SCHWAY_PROFILE_GPU_V0` need explicit regression coverage.
- `src/internal.h` currently reports behavior revision 18 and state format 18. The serialized profile currently contains region and flags (`src/state.c:85+`); adding board-derived behavior requires state validation, serialization and deterministic hash coverage to agree. Decide the API/behavior/state version increments together before changing encoding. Ensure malformed, unsupported and mismatched tuples fail before execution and do not partially mutate the profile.
- Runner profile selection is near `runner/main.c` option parsing and profile setup; libretro sets its profile during content load in `libretro/libretro.c`. Keep host option parsing, `auto` detection and lint out of the core. Exercise region detection precedence and explicit override independently from board selection, then confirm concrete values reach both hosts and the library.
- `tests/compat/features.csv` is the profile feature ledger, distinct from the title ledger. Its protected value grammar is `[A-Za-z0-9._+-]`, and rows are sorted. Record only tested tuples using the context's canonical lowercase `<region>.<board>.<gpu>` representation (for example `pal.pm41-2.v2`); preserve existing region-only rows and do not imply title boot/play from profile selection alone.
- The existing source-unit preflight and MDEC selectors are in `cmake/MDECReferences.cmake` and `tests/mdec/`. Use the current pinned sources and bounded selectors. Do not change expected 4-bit bytes, the 8-bit regression baseline or cycle limit to force progress.

## Recommended planning and verification order

1. Start with a cheap baseline preflight: inspect the exact selected test names and fixture/source extents; count all repository-check lines against the 3,901 ceiling; verify sorted exact inventories and compatibility ledgers; and confirm CTest fixture dependencies and command selectors before any guest run. Sort changed inventory rows before configure.
2. Implement and test the profile tuple as a focused library slice: append the field, map documented board defaults, validate every legal and illegal tuple, preserve prior-size callers and the legacy GPU flag, and include board identity in state round-trip/rejection and deterministic hash tests. Keep version changes and golden updates in this same slice.
3. Add host region inference with table-driven evidence for serial/licence/EXE precedence, conflicts, fallback and explicit override. Verify runner and libretro options pass a concrete region without conflating region, board or title identity.
4. Add the narrow BIOS-shell interception behind the explicit `fast` option. Test default full boot and fast boot separately, plus missing BIOS, ineligible content, standalone `--exe`, invalid combinations and unchanged timing. Keep the test guest bounded and reuse supplied BIOS/fixture policy; never place BIOS bytes in the repository.
5. Add only the targeted MDEC edge cases supported by pinned source and existing selectors. Run source coverage preflight first, then compare the separately reported source/output/guest/display results. Reuse the existing MDEC fixture and preserve current baselines and the 4,915,200-cycle stop.
6. Run affected native configure/build and focused tests after cheap checks. Review before final verification. On the final changed head, run the existing required local CI preset once, then the hosted required gates once. Use `act --dryrun` only if routing changes; run `act` only if it adds coverage beyond local CI. For every hosted CI/release run, inspect critical-path time, repeated work and runner cost while running and after completion without removing required coverage.

## Adversarial review

- Board and region are independent axes. A matrix that only varies region while leaving board implicit can falsely claim all legal board/GPU combinations; enumerate the nine boards and legal pairings directly, and separately test region.
- Appending a field alone is not compatibility. Old struct sizes, default initialization, profile rejection atomicity, serialized-state validation and hashes can diverge; exercise them together before publishing the new compatibility rows.
- Host autodetection can disagree across serial, licence and EXE marker. Keep the stated precedence deterministic, surface conflicts as lint, and let an explicit user choice win; do not let filename or environment affect the core.
- A fast-boot test that merely reaches the executable can miss a bypass of BIOS initialization. Prove the normal shell path remains available and only the documented post-init handoff is skipped; do not treat a stub `--exe` run as evidence for BIOS-backed disc fast boot.
- MDEC source completeness can be mistaken for decoder or movie correctness. Report the four outcomes separately and keep incomplete guest/display behavior honestly bounded; no complete-FMV or exact 8-bit hardware claim follows from passing preflight.
- CI is already covered locally by the project's preset. An `act` run that repeats that suite adds cost without new evidence; retain hosted-only Actions evidence for runner, artifact, permission and release integration behavior.

## Sources and runtime

- `.planning/preparation/README.md` routes system/board/BIOS questions to `PSX-SYSTEM.md` §§7–9, host integration to `HOSTS-AND-INTEGRATION.md` §§3 and 6, GPU revision behavior to `PSX-GPU-AND-GTE.md` §1.1, and MDEC evidence to `PSX-SPU-CDROM-AND-MDEC.md` §4. These pinned project references and the existing source/tests above are the primary implementation evidence.
- Phase 13 discussions and three independent architecture/product/CI reviews were synthesized into `13-CONTEXT.md`; the implementation source was inspected directly to confirm current API, state, host and preflight seams. One delegated source-writing handoff stalled without an artifact; existing audit results and pinned references were recovered instead of repeating broad research. No build, test, Docker or hosted run was needed during research.

## Requirements mapping

| ID | Description | Research support |
|---|---|---|
| PROFILE-01 | Region timing and host selection remain independent from board and title identity. | Existing integer PAL timing; host-only precedence and override; concrete profile values at the core boundary; separate compatibility ledgers. |
| PROFILE-02 | Documented legal board/GPU tuples validate and behavior-affecting profile fields serialize and hash. | Nine-board source table, current append-only profile API, state/hash seams, legacy-size and GPU_V0 hazards, and tuple-ledger encoding. |
| BOOT-01 | Opt-in BIOS-backed shell shortcut skips only the eligible handoff. | Documented shell entry, runner/libretro option seams, separate standalone `--exe` path, and negative/default-path cases. |
| MDEC-01 | Bounded MDEC regressions preserve accepted baselines and report outcomes separately. | Existing CTest input-preflight fixture, pinned-input coverage model, output/guest/display distinction and fixed cycle cap. |

**Runtime/model:** Codex; phase context, primary project preparation notes, current code and test registrations, plus completed independent review findings. Research-only inspection; no test command was run.
