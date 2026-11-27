<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Keep `--memcard-names serial` opt-in for the runner, per DEC.67. Use a validated, uppercase, path-safe normalized serial from the selected disc or PS-EXE, for example SLUS-12345. Keep an explicit `--serial` override literal as the existing compatibility alias; do not silently rename saves supplied through that override.
- **D-02:** For an M3U, freeze one identity when the set is opened so disc swaps keep the same pair of cards. Use the first declared member's valid serial; if that serial is missing or invalid, derive a versioned, namespaced SHA-256 identity from the canonical contents of every declared member. A standalone disc or executable with no valid serial gets the same kind of content-derived fallback from its full canonical media contents. Do not hash filenames, paths, playlist text, only the boot executable, or only a disc prefix. A different valid serial is an identity; do not guess title groupings.
- **D-03:** Keep libretro filenames based on the frontend content basename, as DEC.67 requires. Card 1 remains frontend-managed `RETRO_MEMORY_SAVE_RAM`; card 2 uses the frontend save directory and a distinct `.1.mcr` path. Cover executable, disc, and M3U content paths without making libretro serial-name or sharing options.
- **D-04:** Preserve raw full-size and short-raw imports; short raw images are padded with FFh as DEC.62 specifies. Add .gme, .mem, and .psx full-card wrappers only when magic, declared size, and bounds validate. For ambiguous .psx, accept only a recognized full-card format, never a single-save file. Confirm wrapper fields from independent documentation or pinned provenance-cleared data before fixing test vectors.
- **D-05:** Export raw and supported wrappers explicitly; keep the canonical 131,072 card bytes identical in each direction. A recognized but truncated/malformed wrapper, oversized image, unsupported known format, import error, or failed write leaves the active card and prior complete save unchanged. Never arm persistence after rejecting an existing file. Publish files using the existing temporary-file and atomic-replace pattern; commit card changes/generation only after a complete valid transfer.
- **D-06:** Use the runner's host card helpers and generation-based flush path for both slots. Prove two games and two slots remain isolated, then prove a new runner process reloads the values written by the previous process. Preserve the previous complete file when a write or rename fails.
- **D-07:** For libretro, test card 1's actual SAVE_RAM save/reinject path and card 2's separate host save-directory path. Run two fresh RetroArch processes in one isolated harness invocation so the second sees the first process's save files. Keep the directory between launches. Change libretro_saves to "true" only after the in-process and hosted round trips pass.
- **D-08:** Before long test or hosted runs, perform the cheapest relevant preflight: check changed inventory rows are bytewise sorted, selectors and .info claims point to registered tests, fixture inputs and expected card sizes/counts are present, workflow artifact wiring includes the round-trip guest, and the repository/RetroArch line budgets pass. Then configure/build and run focused card, runner-card, and libretro-card checks before the broad CI workflow. Reuse existing checks; add a permanent preflight only when it prevents a recurring fault.
- **D-09:** Implement bounded, deterministic handling for the documented 58h–5Fh card-port command family needed for storage. Restrict data transfer to the documented flash/file storage function and validated address/length bounds; file selection and status remain metadata operations. Operations that require executing PocketStation code or access to its CPU, ROM, RAM peripherals, wall clock, LCD, audio, or IrDA return a deterministic unsupported result. Keep the regular 52h/53h/57h memory-card path intact and keep all protocol state per instance.
- **D-10:** A PocketStation storage write changes card bytes only after a complete accepted transfer; aborted, malformed, or out-of-range transactions leave bytes and generations unchanged. Keep all data and responses deterministic and free of host files, environment, and wall-clock reads. Planning research must reconcile the new SAVE-05 scope with the older DEC.61 / preparation note that placed the command set later, and establish exact transcripts and limits before code is written.

### the agent's Discretion
- Choose private fields, helper boundaries, plan waves, and test fixture details while retaining the existing one-module-per-chip structure, exact inventories, fixed-width state, and host/core boundary.
- Keep existing short-raw compatibility while rejecting recognized unsupported wrappers. Prefer the smallest format and protocol surface that meets the requirements.
- The matcher results reviewed but not folded were: 001 CI required roll-up, 002 release floor, 003 pins gate (Phase 7), 004 single version source, 005 notice gates (Phase 9), 006 vendored digest (Phase 10), 010 Dependabot config (Phase 7), 011 Dependabot auto-merge workflow, 012 Dependabot decision gate, 014 test-only key gate, 015 weekly digest (Phase 7), 016 format gate, 017 analyzer gate, 021 actionlint config, 022 Windows DLL CRT flake (Phase 7), 025 user-mode fetch address error, 026 COP0 state validation, 027 I-cache refill bus error, 028 exception-loop progress heuristic, 033 vector harness initialization, 035 SST slice field, 036 load-EXE-after-run guard, 037 SWL/SWR bus contract, 038 GTE write-to-use latency, 044 shared register-lane helpers, 047 CD DMA cost, 049 runner/checkpoint repeat review (Phase 7), and 050 main/Release artifact reuse (Phase 9). The CPU, GTE, media, CI, and release items do not implement SAVE-01–05; do not pull them into Phase 8.

### Deferred Ideas (OUT OF SCOPE)
- No PocketStation ARM7, BIOS execution, LCD, RTC, audio, buttons, or IrDA emulation.
- No libretro serial-naming or card-sharing option; DEC.67 keeps frontend content-basename files.
- The matched main/Release artifact-reuse todo remains bound to Phase 9; unrelated CPU, GTE, CD, CI, and release-gate todos remain with their bound phases.
- Preserve the 300-line RetroArch harness cap; do not relocate or minify code merely to hide growth.
- No owner UAT is planned; automated checks prove all testable behavior.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SAVE-01 | Stable per-game runner card names from normalized disc serials, declared multi-disc grouping, and a distinct content-derived fallback for missing or invalid serials. | Existing host probe returns serials and the image provider exposes M3U members; full canonical media content is the safe fallback when no serial exists. |
| SAVE-02 | Import/export raw, .gme, .mem, and .psx full cards byte-exactly; invalid or unsupported input preserves active and stored data. | PSX-SPX describes wrapper signatures/layouts and distinguishes full-card wrappers from single-save formats; existing helper stages imports and atomically replaces raw files. |
| SAVE-03 | Runner loads/flushes both slots, isolates games and slots, survives process restart, and preserves prior files on failed writes. | Runner already has load/generation-flush helpers and fresh-process test patterns to extend. |
| SAVE-04 | Libretro card 1 SAVE_RAM and additional slots persist safely in process and across fresh RetroArch processes; advertise the claim only after validation. | Core exposes card 1 as SAVE_RAM and card 2 through the save-directory callback; existing host and RetroArch test seams need round-trip coverage. |
| SAVE-05 | A guest uses PocketStation storage commands from 58h onward to read and write card data without claiming handheld CPU/peripheral emulation. | PSX-SPX documents command framing and a bounded FUNC 01 transfer size; storage access must whitelist flash/file address ranges and commit only complete writes. |
</phase_requirements>

# Phase 8: Per-game card saves — Research

**Date:** 2026-11-19  
**Confidence:** MEDIUM

## Research Provenance

The dedicated gsd-phase-researcher did not produce an artifact after two extended runs; it was stopped to avoid delaying planning. This synthesis uses the completed Phase 8 architecture/adversarial scouts, the repository's pinned preparation sources, and direct reads of primary PSX-SPX and Libretro documentation. The code implementation must still verify PocketStation address mapping and the pinned frontend's actual SAVE_RAM lifecycle before making release claims. No emulator implementation was opened or copied.

## Standard Stack

- No new package or dependency is needed. Use existing C host helpers, the instance-owned card/SIO path, CMake inventories, and existing SHA-256 support. Keep file I/O in host/runner/libretro and deterministic protocol work in src/.
- The canonical card is 131,072 bytes, divided into 16 blocks of 8 KiB and 128-byte frames. Preserve that raw image as the internal format; wrappers are host import/export formats only. [CITED: PSX-SPX Memory Card Data Format; .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md `3]
- M3U is the explicit multi-disc declaration. Freeze the save identity for a loaded set; do not resolve per-game identity from the disc currently selected after swaps. [VERIFIED: DEC.67; host/disc_image.c scout]

## Architecture Patterns

### Host identity and files

- Runner serial naming is opt-in by DEC.67. The disc probe already emits normalized serials; the runner currently accepts a literal manual serial override and otherwise uses a shared fallback for disc sessions. Add automatic normalized serial selection in the host naming path without changing libretro's frontend-basename policy. [VERIFIED: DEC.67; host/disc_probe.c, runner/main.c code scout]
- For missing/invalid serials, use a versioned, namespaced digest over canonical media content, not filenames, path text, an executable alone, or a short disc prefix. For an M3U fallback, include each declared member's canonical content. A complete-content hash prevents obvious prefix collisions, but has launch-time I/O cost; measure it once and do not weaken the identity requirement to avoid the cost. [CITED: SAVE-01; host/disc_image.c and host/disc_probe.c scout]
- Keep libretro's card-1 SAVE_RAM and card-2 file paths separate. The save directory is frontend-configurable; the core already requests RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY. Verify the pinned RetroArch path and reload behavior in the isolated hosted test. [VERIFIED: libretro/libretro.c code scout; CITED: https://docs.libretro.com/guides/change-directories/]
- Do not persist a rejected existing card-2 import. The current load helper can keep the path armed after import failure, allowing a later guest write to replace the unreadable file. Arm persistence only after a missing file or successful import. [VERIFIED: libretro/libretro.c code scout]

### Card wrappers and failure behavior

PSX-SPX's Memory Card Images section gives these signatures and layouts:

| Format | Signature / header | Canonical card bytes |
|--------|--------------------|-----------------------|
| DexDrive .gme | ASCII 123-456-STD plus NUL; 0xF40-byte header; usual full length 0x20F40 | 128 KiB begins at 0xF40 |
| VGS .mem | VgsM signature; 64-byte header | 128 KiB follows header |
| PlayStation Magazine .psx | PSV signature; 256-byte header | 128 KiB follows header |

The local preparation and runner docs also describe short raw images padded with FFh. PSX-SPX describes Xploder/Action Replay as a 54-byte single-save header plus one save's blocks, not a full card. Therefore extension alone is insufficient: require known magic and bounded full-card length, and reject recognized unsupported or truncated signatures before mutation. Export wrappers explicitly and compare all 131,072 canonical bytes after round trip. [CITED: https://psx-spx.consoledev.net/controllersandmemorycards/; .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md `3.4–3.6; docs/runner.md]

The .gme header contains copies of card directory state/next-block bytes and fifteen description strings, so construct those fields from the current card instead of writing an arbitrary constant header. The docs allow some reserved/description bytes to vary; tests should assert documented fields and payload equality. [CITED: PSX-SPX Memory Card Images]

### PocketStation serial protocol

PSX-SPX lists 58h ID/version, 59h prepare execution, 5Ah metadata (directory index, communication flags, serial, date/time), 5Bh/5Ch function transfer, 5Dh download notification, and 5Eh/5Fh communication flags. The 5Bh/5Ch envelope is variable length: function byte, parameter length and parameters, data length and data, then FFh terminator; function handling occurs before and after the data transfer. [CITED: https://psx-spx.consoledev.net/pocketstation/]

Built-in function 01 (memory block) has five parameter bytes: a 32-bit address and requested length. One transfer is clipped to 0x80 (128) bytes. The documented function can access RAM, ROM, I/O, and flash, so the core must whitelist the selected flash/file address range and reject other windows. Do not expose arbitrary memory reads/writes, custom function entries, or execute PocketStation programs. Verify the exact flash-to-card mapping and command transcripts from the pinned source before implementation. [CITED: PSX-SPX PocketStation, FUNC 01]

A storage-only implementation can keep per-instance communication metadata, return deterministic date/time fields without reading a clock, and make execution/callback-only operations deterministic unsupported/no-op results. A FUNC 01 write should be staged and committed only after its full transfer and terminator are accepted; aborts and invalid ranges must not change card data or generation. Exact reply bytes, address map, and unsupported responses remain focused implementation probes; they do not justify CPU, BIOS, LCD, RTC, or IrDA scope. [CITED: AGENTS.md rule 4; PSX-SPX PocketStation]

## Common Pitfalls

- A serial can be reused by distinct revisions; do not imply the serial uniquely proves a release. Follow the required serial policy and use the digest only when serial is missing/invalid.
- Hashing only first sectors or the boot executable can alias different discs with shared boot content. Hash canonical sector data and TOC, excluding file/container metadata and paths. For M3U fallback, include all members.
- M3U swaps must not change a card path. Membership changes should be documented as an identity change.
- A malformed wrapper can be mistaken for short raw input. Recognize supported/unsupported signatures before the short-raw path; reject a truncated recognized wrapper before mutation.
- A refused libretro card import must not be followed by a blank-card overwrite on unload. Test a sentinel file survives both guest writes and shutdown.
- RETRO_MEMORY_SAVE_RAM pointer/size assertions do not prove that RetroArch saves and reinjects bytes. Use the real two-process harness and preserve its isolated directory between runs.
- Existing wrapper tests synthesize headers from parser constants. Add a source-derived table and negative cases, especially truncated GME/VGS/PSV and Xploder single-save data.
- PocketStation FUNC 01 has a broad address surface. Enforce overflow-safe range checks before every transfer; one unchecked 32-bit address or length can expose state outside the card image.
- The RetroArch test directory is exactly at its 300-line cap. Reuse or restructure existing harness code; do not raise the cap or hide lines elsewhere.

## Code Examples

Expected format detection uses both signature and bounded layout:
- If the first bytes are 123-456-STD followed by NUL, require the documented 0xF40 header and 128 KiB payload.
- If the first four bytes are VgsM, require the documented 64-byte header and complete supported image length.
- If the first three bytes are PSV, require the documented 256-byte header and complete supported image length.
- Otherwise preserve the settled raw/short-raw rule only when the file is not a recognized wrapper or unsupported known format.

Expected runner identity selection:
- Preserve a valid explicit --serial as the literal compatibility override.
- Otherwise use the normalized serial from selected content; for M3U, capture the first member's serial when opening the set.
- If serial is absent/invalid, use a versioned content digest over complete canonical media, including all M3U members.
- Keep libretro's frontend-basename policy independently.

These are planning patterns, not production code. The executor must verify exact canonical sources and fit them to existing helper contracts.

## Environment Availability

| Dependency | Required By | Availability | Fallback |
|------------|-------------|-----------|----------|
| CMake, C compiler, CTest | Existing build/test presets | Existing configured CI build tree is present; verify compiler with preset before building | Use repository CI preset |
| SHA-256 | Runner content identity | Existing project code has SHA-256 support; confirm host linkage and reuse | Small existing-code helper; no new dependency |
| RetroArch 1.22.2 pinned build | Hosted save/load round trip | Existing CI job downloads a SHA-pinned build | Hosted background CI job |

## Cheap Preflight Before Long Validation

1. Check changed inventories and CMake selector registrations; sort changed inventory rows bytewise before configure.
2. Validate fixture sources and expected payload/card sizes before a guest run; confirm the card guest is included in the CI artifact.
3. Run .info claim-to-test wiring and existing repository/E2E line-budget checks before hosted RetroArch.
4. Configure/build and run focused card file/protocol, runner card, and libretro card checks before the full CI workflow.
5. Preserve the isolated RetroArch directory across both launches; only the final changed-head hosted run proves the frontend lifecycle.
6. Keep new checks as maintained gates only when they prevent a recurring fault named by a requirement.

## Project Constraints

- Three deliverables only: library, headless runner, and libretro core. No window/audio/input-device code in deliverables.
- The core is per-instance, integer-only and bounded; no wall clock, filesystem, or environment access in the core.
- Behavior changes include exact-inventory gating tests; code review precedes final verification.
- Keep repository-check code within its phase budget and real RetroArch E2E code below 300 lines total.
- Use permissive sources and preserve notices. Do not open or copy prohibited emulator code.
- Commits use GitHub noreply identity. Preserve the user's config edit and stage only explicit files.

## Primary Sources and Open Probes

- PSX-SPX PocketStation: https://psx-spx.consoledev.net/pocketstation/
- PSX-SPX memory-card formats: https://psx-spx.consoledev.net/controllersandmemorycards/
- Libretro directory configuration: https://docs.libretro.com/guides/change-directories/
- Local sources: .planning/preparation/README.md; .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md `2.8, 3.4–3.6, 5.3`; .planning/preparation/HOSTS-AND-INTEGRATION.md `1, 4`.

**Focused probes for the first relevant implementation plan:**
- Derive exact physical/virtual PocketStation flash intervals and map them to card file bytes from the pinned source; include boundary/overflow cases.
- Pin 58h–5Fh reply and ACK transcripts; rejected commands must not partially mutate state.
- Validate GME/VGS/PSV export fields against source-derived vectors and reject Xploder/AR single-save data.
- Measure full-media identity-hash startup cost without weakening the full-content fallback.
- Confirm pinned RetroArch saves/reinjects card 1 while card 2 persists independently across two processes.
