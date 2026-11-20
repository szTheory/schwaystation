# Phase 8: Per-game card saves - Context

**Gathered:** 2026-11-19
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver SAVE-01 through SAVE-05: stable per-game runner card identity, validated raw and wrapped card import/export, safe persistence for both runner slots and libretro slots, and the documented PocketStation storage protocol needed to read and write card data. Preserve the three deliverables, deterministic integer-only core, and host-only file access. Do not implement the PocketStation CPU, BIOS, LCD, RTC, or IrDA; do not change libretro's settled content-basename save policy or add unrelated emulator/CI features.

</domain>

<decisions>
## Implementation Decisions

### Per-game identity and naming
- **D-01:** Keep `--memcard-names serial` opt-in for the runner, per DEC.67. Use a validated, uppercase, path-safe normalized serial from the selected disc or PS-EXE, for example `SLUS-12345`. Keep an explicit `--serial` override literal as the existing compatibility alias; do not silently rename saves supplied through that override.
- **D-02:** For an M3U, freeze one identity when the set is opened so disc swaps keep the same pair of cards. Use the first declared member's valid serial; if that serial is missing or invalid, derive a versioned, namespaced SHA-256 identity from the canonical contents of every declared member. A standalone disc or executable with no valid serial gets the same kind of content-derived fallback from its full canonical media contents. Do not hash filenames, paths, playlist text, only the boot executable, or only a disc prefix. A different valid serial is an identity; do not guess title groupings.
- **D-03:** Keep libretro filenames based on the frontend content basename, as DEC.67 requires. Card 1 remains frontend-managed `RETRO_MEMORY_SAVE_RAM`; card 2 uses the frontend save directory and a distinct `.1.mcr` path. Cover executable, disc, and M3U content paths without making libretro serial-name or sharing options.

### Card file compatibility and failure safety
- **D-04:** Preserve raw full-size and short-raw imports; short raw images are padded with `FFh` as DEC.62 specifies. Add `.gme`, `.mem`, and `.psx` full-card wrappers only when magic, declared size, and bounds validate. For ambiguous `.psx`, accept only a recognized full-card format, never a single-save file. Confirm wrapper fields from independent documentation or pinned provenance-cleared data before fixing test vectors.
- **D-05:** Export raw and supported wrappers explicitly; keep the canonical 131,072 card bytes identical in each direction. A recognized but truncated/malformed wrapper, oversized image, unsupported known format, import error, or failed write leaves the active card and prior complete save unchanged. Never arm persistence after rejecting an existing file. Publish files using the existing temporary-file and atomic-replace pattern; commit card changes/generation only after a complete valid transfer.

### Runner and libretro persistence
- **D-06:** Use the runner's host card helpers and generation-based flush path for both slots. Prove two games and two slots remain isolated, then prove a new runner process reloads the values written by the previous process. Preserve the previous complete file when a write or rename fails.
- **D-07:** For libretro, test card 1's actual SAVE_RAM save/reinject path and card 2's separate host save-directory path. Run two fresh RetroArch processes in one isolated harness invocation so the second sees the first process's save files. Keep the directory between launches. Change `libretro_saves` to `"true"` only after the in-process and hosted round trips pass.
- **D-08:** Before long test or hosted runs, perform the cheapest relevant preflight: check changed inventory rows are bytewise sorted, selectors and `.info` claims point to registered tests, fixture inputs and expected card sizes/counts are present, workflow artifact wiring includes the round-trip guest, and the repository/RetroArch line budgets pass. Then configure/build and run focused card, runner-card, and libretro-card checks before the broad CI workflow. Reuse existing checks; add a permanent preflight only when it prevents a recurring fault.

### PocketStation storage protocol
- **D-09:** Implement bounded, deterministic handling for the documented `58h`–`5Fh` card-port command family needed for storage. Restrict data transfer to the documented flash/file storage function and validated address/length bounds; file selection and status remain metadata operations. Operations that require executing PocketStation code or access to its CPU, ROM, RAM peripherals, wall clock, LCD, audio, or IrDA return a deterministic unsupported result. Keep the regular `52h`/`53h`/`57h` memory-card path intact and keep all protocol state per instance.
- **D-10:** A PocketStation storage write changes card bytes only after a complete accepted transfer; aborted, malformed, or out-of-range transactions leave bytes and generations unchanged. Keep all data and responses deterministic and free of host files, environment, and wall-clock reads. Planning research must reconcile the new SAVE-05 scope with the older DEC.61 / preparation note that placed the command set later, and establish exact transcripts and limits before code is written.

### Folded Todos
- None. `todo.match-phase 8` returned 28 candidates at score 0.4 or higher, mostly from generic words such as “phase,” “file,” and “bytes.” They were reviewed against SAVE-01–05 and left out to preserve the fixed Phase 8 boundary. The candidates bound to other phases remain bound there.

### the agent's Discretion
- Choose private fields, helper boundaries, plan waves, and test fixture details while retaining the existing one-module-per-chip structure, exact inventories, fixed-width state, and host/core boundary.
- Keep existing short-raw compatibility while rejecting recognized unsupported wrappers. Prefer the smallest format and protocol surface that meets the requirements.
- The matcher results reviewed but not folded were: `001` CI required roll-up, `002` release floor, `003` pins gate (Phase 7), `004` single version source, `005` notice gates (Phase 9), `006` vendored digest (Phase 10), `010` Dependabot config (Phase 7), `011` Dependabot auto-merge workflow, `012` Dependabot decision gate, `014` test-only key gate, `015` weekly digest (Phase 7), `016` format gate, `017` analyzer gate, `021` actionlint config, `022` Windows DLL CRT flake (Phase 7), `025` user-mode fetch address error, `026` COP0 state validation, `027` I-cache refill bus error, `028` exception-loop progress heuristic, `033` vector harness initialization, `035` SST slice field, `036` load-EXE-after-run guard, `037` SWL/SWR bus contract, `038` GTE write-to-use latency, `044` shared register-lane helpers, `047` CD DMA cost, `049` runner/checkpoint repeat review (Phase 7), and `050` main/Release artifact reuse (Phase 9). The CPU, GTE, media, CI, and release items do not implement SAVE-01–05; do not pull them into Phase 8.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope, constraints, and prior decisions
- `AGENTS.md` — three deliverables, deterministic bounded core, provenance, testing, budgets, branch/release, and GSD rules.
- `.planning/AUTONOMOUS.md` — autonomous phase authorization, role lenses, adversarial review, preflight preference, and ship boundary.
- `.planning/PROJECT.md` — product and architecture boundary.
- `.planning/REQUIREMENTS.md` — SAVE-01 through SAVE-05 acceptance criteria.
- `.planning/ROADMAP.md` — Phase 8 goal and success criteria.
- `.planning/LESSONS.md` — especially SW-CI-001 on checking exact inventories and failed paths before expensive retries.
- `.planning/preparation/DECISIONS.md` — DEC.60, DEC.62, DEC.64, DEC.67, DEC.68–70, and DEC.108.
- `.planning/preparation/README.md` — source index and pinned-reference lookup.

### Card, serial, and PocketStation behavior
- `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §§2.8, 3.1–3.6, 5.1–5.3 — card bytes/protocol, import compatibility, and content identity.
- `.planning/research/FEATURES.md` — existing PocketStation feature boundary to reconcile with SAVE-05.
- `.planning/research/ARCHITECTURE.md` — PocketStation placement and core/host boundaries.
- `https://psx-spx.consoledev.net/pocketstation/` — primary protocol reference for command behavior; use the pinned local preparation reference where available.

### Host integration, testing, and assets
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` §§1, 4 — libretro SAVE_RAM, additional slots, runner persistence and naming.
- `.planning/preparation/ARCHITECTURE.md` — per-instance card/SIO state and host boundaries.
- `.planning/preparation/CONFORMANCE.md` — memory-card fixture and automated validation patterns.
- `.planning/preparation/ENGINEERING.md` — CMake inventories, tests, determinism, and budgets.
- `PROVENANCE.md` and `ASSET_POLICY.md` — permitted reference code and fixture constraints.
- `tests/fixtures/manifest.txt` — permitted committed test data.

### Existing integration seams
- `src/card.c`, `src/card.h`, and `src/sio.c` — card command phases, transactional writes, and byte/ACK scheduling.
- `host/cards.c` and `host/cards.h` — bounded raw/wrapper imports and atomic raw-card writes.
- `host/disc_probe.c` and `host/disc_probe.h` — normalized disc serial source.
- `host/disc_image.c` — sector access and declared M3U membership.
- `runner/main.c` — runner save naming, load, and generation-based flush.
- `libretro/libretro.c` and `libretro/schwaystation_libretro.info.in` — SAVE_RAM and card-2 persistence.
- `tests/fixtures/src/memcard_roundtrip.asm`, `tests/fixtures/tests.cmake`, `tests/libretro/host.c`, and `tests/retroarch/run_retroarch.cmake` — guest, in-process, and hosted process-round-trip patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/card.c` already implements the standard `52h`, `53h`, and `57h` memory-card path with per-slot protocol state and generation tracking; `src/sio.c` supplies cycle-based byte/ACK timing.
- `host/cards.c` imports short raw and recognized `.gme`, `.mem`/`.vgs`, and `.psx`/PSV images and writes raw cards by temporary file plus atomic replacement. It has no wrapper export API yet.
- `host/disc_probe.c` already returns normalized serials. The runner's `runner_card_basename()` currently falls back to `schwaystation` for disc content, and serial naming requires a caller override.
- Runner card loading and generation-based flushing already have two-slot and fresh-process test patterns. `memcard_roundtrip.asm` is an existing guest fixture to extend or copy.
- Libretro exposes card 1 as `RETRO_MEMORY_SAVE_RAM`; card 2 is content-basename based, but its path is currently set only for executable content. A rejected card-2 import can still leave a path armed for later overwrite.
- The current RetroArch harness clears its isolated directory once and totals exactly 300 lines. Its two launches must share the same directory without exceeding the existing limit. CI currently uploads two other guest fixtures, not the card round-trip guest.

### Established Patterns
- Keep filesystem and media parsing in host modules; core card protocol stays deterministic, bounded, and instance-owned.
- Stage and validate external data before publishing it; use existing generation and atomic-replace behavior for persistence.
- CMake exact inventories are sorted, and every behavior change adds its test there. Keep real RetroArch test code under 300 lines and repository gates under their named phase budget.
- Review source changes before final verification. Do cheap inventory, selector, fixture, workflow, and budget checks before full CI or hosted RetroArch.

### Integration Points
- PocketStation commands extend the existing card/SIO response path; transfer bounds and commit timing must be validated against the protocol source.
- Runner serial identity belongs in host-side disc/EXE selection and card path construction, not in the emulated core.
- Card wrapper export belongs in `host/cards`; runner and libretro should continue to use raw canonical card bytes internally.
- Libretro card 1 stays frontend-owned; slot 2 uses a distinct save-directory path for executable, disc, and playlist loads.
- The in-process libretro host must exercise import/failure/card-1 reinjection; the isolated RetroArch driver must run a fresh-process save/continue round trip for both slots.

</code_context>

<specifics>
## Specific Ideas

- Keep the established test `runner.cards` two-slot result, with a separate two-game identity check and a second-process reload.
- The hosted card test must preserve one isolated save directory across two RetroArch launches, verify both card slots, and include the existing card round-trip guest in the artifact.
- `.info` save support is a verified claim: enable it only when the in-process and hosted fresh-process paths pass.
- Before full CTest/CI, cheaply check sorted inventories, selectors, fixture and expected byte counts, `.info`/test mapping, artifact wiring, and both line budgets. Run focused card, runner, and libretro selectors first.
- Test wrapper export/import by complete card-byte equality. Use independently verified wrapper bytes; use a recognized truncated wrapper and a sentinel destination to prove rejection preserves data.
- The automatic serial-name fallback must be based on full canonical content and versioned. Record any startup cost from whole-media hashing; do not weaken collision guarantees to hash only a prefix.

</specifics>

<deferred>
## Deferred Ideas

- No PocketStation ARM7, BIOS execution, LCD, RTC, audio, buttons, or IrDA emulation.
- No libretro serial-naming or card-sharing option; DEC.67 keeps frontend content-basename files.
- The matched main/Release artifact-reuse todo remains bound to Phase 9; unrelated CPU, GTE, CD, CI, and release-gate todos remain with their bound phases.
- Preserve the 300-line RetroArch harness cap; do not relocate or minify code merely to hide growth.
- No owner UAT is planned; automated checks prove all testable behavior.

### Reviewed Todos (not folded)
- The 28 `todo.match-phase 8` results scoring at least 0.4 were reviewed. Their matcher reasons were broad shared terms, and the titles map to Phase 7/9/10 CI work, CPU exception/state/cache behavior, GTE timing, CD DMA cost, or unrelated build/test gates. None implements SAVE-01–05, so none is folded into Phase 8. Todos 003, 010, 015, 022, and 049 remain bound to Phase 7; 005 and 050 to Phase 9; 006 to Phase 10.

</deferred>

---

*Phase: 8-per-game-card-saves*
*Context gathered: 2026-11-19*
