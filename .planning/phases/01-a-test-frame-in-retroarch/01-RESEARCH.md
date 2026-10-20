# Phase 1: A test frame in RetroArch - Research

**Researched:** 2026-10-19
**Domain:** C17 library + headless runner + libretro core skeleton ported from a same-owner MIT seed; CMake presets and
CTest gates; GitHub Actions on six platforms plus `be32` (qemu-ppc) and `wasm32` (wasmtime); release-please with
App-token rehearsal and attestations; a hosted RetroArch 1.22.2 screenshot compare
**Confidence:** HIGH for the seed inventory, libretro facts and arithmetic (all read or computed this session); MEDIUM
for plan sequencing (derived); LOW only where tagged `[ASSUMED]`

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Test card (FRAME-02, 03, 07, 09)
- **D-01:** The card changes every frame. `uint16_t schway__testcard_pixel(uint32_t x, uint32_t y, uint32_t frame)`
  in `src/testcard.c` is one pure integer function (no table, font, text or global). Every `schway_run_frame`
  redraws all 320 × 240 pixels; nothing is cached, so the Cachegrind fixture measures steady nonzero per-frame work
  and a static-card "0 Ir" degenerate gate is impossible. A static card is rejected: it would make the
  RetroArch frame-5 compare blind to an off-by-one and make the FRAME-09 state round trip trivially true.
- **D-02:** The frame counter is the instance's metadata `frame` field and the card's only state. `run_frame`
  increments it and then draws, so the k-th frame since create shows k. The strip shows its low 32 bits (wraps
  deterministically).
- **D-03:** Pixel rules, first match wins (what each catches in brackets):
  1. `x < 8 && y < 8` → `0x7FFF` [mirror, flip, transpose];
  2. `x == 0 || x == 319 || y == 0 || y == 239` → `0x7FFF` [crop, offset, wrong geometry];
  3. `1 ≤ x ≤ 8` → `(y & 31) | ((y >> 5) << 5)` row-ID strip [wrong pitch, skipped/duplicated rows];
  4. bands with `L = x / 10` (all 32 levels): rows 1–16 red `0x001F`; 17–48 R ramp `L`; 49–80 G ramp `L << 5`;
     81–112 B ramp `L << 10`; 113–144 grey `L | L << 5 | L << 10`; 145–176 the same grey `| 0x8000` (mask twin);
     177–192 checker `((x ^ y) & 1) ? 0x7FFF : 0`; 193–208 counter strip; 209–238 green `0x03E0`
     [R/B swap vs vertical flip are distinguishable; scaling and filtering show in the checker];
  5. counter strip: for `32 ≤ x < 288`, bit `31 − (x − 32) / 8` of `frame` (MSB left) → `0x7FFF` if set else `0`;
     elsewhere in rows 193–208 grey `0x4210`.
- **D-04:** Metadata tuple per frame: field 0, interlaced 0, vres 240, hres 320, dot_div 8, region from the profile
  (NTSC-U default), enabled 1, rgb24 0, bpp 16, start_x 0, start_y 0, X1 = 0x260, X2 = 0xC60, Y1 = 0x10,
  Y2 = 0x100 (psx-spx GP1(06h)/(07h) defaults), so the ARCHITECTURE §4 width formula
  `(((X2 − X1) / div) + 2) & ~3 = 320` is exercised from phase 1 (derived).
- **D-05:** Display words pass through verbatim, bit 15 included, and the frame hash covers them (DEC.30). The
  XRGB8888 converter masks `w & 0x7FFF` first, then expands 5 to 8 bits by bit replication `(c << 3) | (c >> 2)`
  (0→0, 1→8, 15→123, 16→132, 31→255), X byte `0x00`. One converter serves the runner's `--png` and the adapter.
- **D-06:** Because runner and adapter share the converter, FRAME-03 alone cannot catch a wrong rule: a known-answer
  unit test pins the five spot values plus `0x7FFF → 0x00FFFFFF`, `0x8000 → 0x00000000`, and asserts the mask-twin
  band converts equal to the grey band while their native words differ.
- **D-07:** Tests: frame 5 and frame 60 hash differently (stuck or stale-frame adapter); `get_video` at a tight pitch
  (640 bytes) and a padded pitch (2048) hash equal with padding untouched; state round trip: run to 30, save, run to
  60 and hash, load, run to 60 again, equal bytes; save → load → save gives identical bytes; VALIDATE rejects a
  corrupted state. The pinned `--frames 60 --hash` value is committed; an intentional change bumps the behaviour
  revision.
- **D-08:** Frame alignment with RetroArch: compare `--max-frames=5` against `--frames 5 --png 5:FILE`. If the first
  hosted run shows a deterministic offset, decode the counter in the screenshot and pin the runner frame number with
  a comment (a test bug to triage, AGENTS rule 8). Never accept a range of frames.
- **D-09:** Internally the card is drawn into a 1024 × 512 u16 VRAM array in the instance and `get_video` scans out
  the display rectangle at (0,0) 320 × 240, so the tuple means what it will mean in phase 4 and `gpu.c` takes the
  array over without an API change. No public `get_vram` in phase 1.
- **D-10:** One short `docs/` section explains what each band catches and how to read the counter.

#### Phase-1 public API (FRAME-03, 04, 08, 09)
- **D-11:** Names land only in the phase whose runner, adapter or requirement calls them; no stubs, no
  `ERR_UNSUPPORTED` placeholders (DEC.75, XFER-091). `SCHWAY_API_VERSION` is 1 at the phase-1 release and +1 in each
  later phase that adds names; `tests/api/vN.c` touches every name version N adds; v1.c is frozen at release with a
  `>= 1` floor. — **Reversibility:** one-way — the phase-1 release publishes the v1 ABI; removing or reshaping a
  name after release needs a SOVERSION bump.
- **D-12:** Phase-1 functions: `schway_get_versions`, `schway_create` (exactly one allocation, default NTSC-U
  profile, powers on; allocator NULL return → `ERR_NO_MEMORY`, `*inst` unchanged), `schway_destroy` (one free of the
  same size, NULL-safe), `schway_set_profile` (region only; bad value → `ERR_PROFILE_REJECTED`; after the first
  `run_frame` → `ERR_PROFILE_LOCKED`; runner `--region`, libretro option `schwaystation_region` ntsc-u/ntsc-j/pal
  recreates the instance), `schway_reset` (POWER only; other kinds → `ERR_ARGUMENT`; `retro_reset` maps to POWER),
  `schway_run_frame` (one frame on the exact tick accumulator; `out` may be NULL; never allocates),
  `schway_get_info`, `schway_get_video`, `schway_get_audio`, `schway_state_size`, `schway_save_state`,
  `schway_load_state`, `schway_state_visit`, `schway_status_string` (non-NULL text for unknown values).
- **D-13:** Phase-1 types and constants: `schway_status` u32 with `SCHWAY_STATUS_OK 0`, `ERR_ARGUMENT 1`,
  `ERR_STRUCT_SIZE 2`, `ERR_NO_MEMORY 3`, `ERR_BUFFER_TOO_SMALL 4`, `ERR_PROFILE_REJECTED 5`, `ERR_PROFILE_LOCKED 6`,
  `ERR_STATE_MAGIC 7`, `ERR_STATE_VERSION 8`, `ERR_STATE_CRC 9`, `ERR_STATE_FIELD 10`; opaque `schway`;
  `schway_allocator {alloc(user,size) max_align_t-aligned; free(user,ptr,size); user}`, both required (the core
  imports no `malloc`); `schway_config {size; allocator}` (the sketch's `audio_ring_frames` is dropped, ring fixed at
  4 frames); `schway_versions {size; library_major, minor, patch; api; abi; behaviour_revision; state_format}`;
  `schway_profile {size; region}` with `SCHWAY_REGION_NTSC_J 1`, `NTSC_U 2`, `PAL 3`;
  `schway_info {size; width, height; fps_num, fps_den; sample_rate_num, sample_rate_den; max_audio_frames; region}`
  (320, 240; 53,693,175/897,619 or 53,203,425/1,069,484; 44,100/1; 738 or 887);
  `schway_run_result {size; stop; frame u64 @8; ticks u64 @16}` (24 bytes everywhere), `SCHWAY_STOP_FRAME 1`;
  `schway_video` per ARCHITECTURE §3 with the 13-word tuple and `frame` u64; `schway_audio {size; samples; capacity;
  written; dropped}` counted in stereo pairs; visitor `void (*)(void *user, const char *name, const void *data,
  uint32_t count, uint32_t width)` (width 1/2/4/8, host order, names `static const`, XFER-088);
  `SCHWAY_RESET_POWER 1`; `SCHWAY_API_VERSION 1`.
- **D-14:** Phase-1 audio is real, not a stub: each frame pushes exactly its ticks / 768 silent pairs (737/738 NTSC,
  886/887 PAL) from the same accumulator that yields the fps rationals; the adapter forwards them through
  `audio_batch`; pairs beyond the 4-frame ring are dropped and counted.
- **D-15:** Layout rules: every boundary struct starts with `uint32_t size`; input below `SCHWAY__<T>_SIZE_FIRST` →
  `ERR_STRUCT_SIZE`, larger accepted only if the extra bytes are zero; output writes min(size, own); every appended
  field treats 0 as default; fixed-width fields, enums as u32, no `bool` or bit-fields; every u64 at a multiple-of-8
  offset with explicit `reserved` u32 padding; the only pointer into core state is `schway_video.pixels`, valid
  until the next run, load or reset; byte counts in parameters are `size_t`, struct fields `uint32_t`.
- **D-16:** `_Static_assert(offsetof/sizeof)` pins for every public struct live in `src/internal.h` (compiled on every
  target, so `be32` and `wasm32` check them), pointer-bearing structs pinned relative to `sizeof(void *)`; never in
  the public header (not C++17, keeps it terse). `tests/header/header.c` and `header.cpp` check size-first,
  `sizeof(schway_status) == 4`, `SCHWAY_API_VERSION == 1` and every signature by function-pointer assignment.
- **D-17:** Hashes and states serialise explicit little-endian words, never struct bytes. State = header (magic,
  `state_format`, size, CRC-32) + every visited field LE, byte-identical on all eight lanes; `load_state` validates
  everything (profile mismatch → `ERR_STATE_FIELD`) before applying any byte; the adapter wires `retro_serialize*`.
- **D-18:** `core.no_alloc_after_load`: the ported counting allocator (`tests/alloc.h`, with `fail_from`) asserts
  create = 1 allocation, destroy = 1 free of the same size, nothing else allocates or frees (run_frame × N, get_*,
  set_profile, reset, save, load, visit), and `fail_from = 1` → `ERR_NO_MEMORY` with nothing leaked; runs on every
  lane including wasm32. In phase 1 "load" means create.
- **D-19:** Deferred names (the phase that adds them): P2 `load_bios`/`bios_info`, `probe_exe`/`exe_info`,
  `load_exe`, `peek`, `get_tty`, `get_fault`/`schway_fault`, `RESET_RESET`, `STOP_FAULT`, `STOP_HOOK`,
  `run_result.fault_present/overshoot_cycles`, `profile.flags/ram_fill/seed/board`, BIOS/EXE statuses; P3 none
  expected; P4 `set_input`/`schway_input`, `load_memcard`, `get_memory`/`schway_span`, `memcard_generation`,
  `get_vram`, `run_result.lag/rumble`, `profile.gpu_rev`, `info` PAR and `dot_div`, card/device statuses; P5
  `schway_run`/`schway_budget`, `STOP_BUDGET`, `STOP_AUDIO_FULL`; P6 `schway_disc`, `schway_toc`, `open_disc`,
  `set_lid`, `swap_disc`, `host/schway_disc_image.h`, `STOP_LID`, disc statuses, `profile.cd_firmware/cd_region`;
  later `validate_state` (v3.0), `instance_size`/`create_in` (v6.0 or the browser seed), `poke`, register views,
  `set_hooks` (v5.0), `get_counters` (first phase whose runner prints a counter not already exposed).

#### Seed porting (FRAME-01, 05, 06, 07)
- **D-20:** Primary seed: cogmaster `v0.1.0` = `f09ac97096acda99888270e4c1e4dc96e12ffdfe` (its released phase 1;
  differs from the phase-1 merge `ea3bd22e4280` only in `CHANGELOG.md`, `version.txt` and the release manifest;
  public, verified with `gh api repos/szTheory/cogmaster/commits/v0.1.0`, exit 0). It supersedes the
  `abd950941e93` pin, which predates the workflows, e2e and release config; plan 1 updates PROVENANCE §2 and §4.
  muhtari `d698364b180d` (v0.2.0, DEC.103) supplies only the per-module OBJECT-library CMake pattern
  (`CMakeLists.txt` L68–82, XFER-105); muhtari has no newer public commit and `ecbe7f52aa37` still returns 422.
  This is new evidence since DECISIONS (cogmaster shipped publicly), not a reopened choice.
- **D-21:** Port method: copy and rename, fetched only with
  `gh api -H 'Accept: application/vnd.github.raw' repos/szTheory/cogmaster/contents/<path>?ref=f09ac97096ac`
  (never a local checkout). Renames `cogmaster` → `schwaystation`, `COGMASTER_` → `SCHWAYSTATION_`, public
  `cogmaster_` → `schway_`, internal `cogmaster__` → `schway__<module>_`, `CogmasterTests.cmake` →
  `SchwaystationTests.cmake`, `cogmaster_add_test` → `schway_add_test`. Provenance per PROVENANCE §5a: no file
  header; each commit body carries `Seeded from cogmaster f09ac97096ac <path> (same author, MIT).` (and the muhtari
  line for the CMake pattern).
- **D-22:** Write fresh, small: the PS1 test card and tuple, the 15-bit → XRGB8888 helper (replaces cogmaster's
  `host/palette.*`, which is dropped with `tests/host/*`), the av_info rationals, the PS1 exit-status table,
  `tests/cmake/line_budget.cmake` (serves `policy.e2e_budget` ≤ 300 and `policy.gate_budget` ≤ 1,500),
  `tests/cmake/revision.cmake` (`build.revision_matches_head`), the `hygiene.no_bios_bytes` CTest and self-test
  (fixtures generated at test time from hex, no signature bytes committed), p90 and max µs in `--bench`, the
  per-directory inventory logic, and the PROVENANCE §2/§4/§8 and THIRD-PARTY-NOTICES rows.
- **D-23:** Vendor `libretro/libretro.h` unmodified from libretro-common `a3194c18ba65` (PV.10) with its SHA-256 in
  PROVENANCE §8 and THIRD-PARTY-NOTICES; do not copy cogmaster's copy (a RetroArch pin).
- **D-24:** Gate budget (FRAME-05, AGENTS rule 7): port only the gates FRAME-01..09 name. cogmaster carries 2,218
  gate lines and the tree already spends 812. Remove `scripts/setup-release-app.sh` (one-off; the App exists;
  `docs/maintaining.md` points to it in history); re-sync `scripts/publish.sh` from the seed (it lacks the
  `SIGNER_WORKFLOW` rehearsal-signer fix); merge cogmaster's `timeouts.cmake` and `action_pins.cmake` into one
  `tests/cmake/workflows.cmake`; replace `check_hash.cmake` with CTest `PASS_REGULAR_EXPRESSION`; trim headers to a
  usage line plus a "Fault prevented" line. Target ≤ 1,450 measured by
  `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*`. Gate logic is never moved into `tests/<dir>/tests.cmake`,
  `cmake/` or workflow YAML to dodge the budget.
- **D-25:** Seeds, not phase-1 work (one todo each, bound to the next phase that adds a caller): cogmaster
  `ci_required`, `floor`, `pins`, `single_version`, `notices`, `vendored`, `exit_table`, `api_names`, `info_claims`
  (GAME-07); muhtari `dependabot.yml`, `dependabot-automerge.yml`, `dependabot.cmake`, `firmware_scan.cmake`
  (GAME-05), `no_test_entries.cmake`; a `digest.yml` workflow.
- **D-26:** The RetroArch e2e files (`tests/retroarch/{run_retroarch.cmake, test.cfg.in, bmp.c, compare.c,
  flip.cmake}`, 293 lines at the seed) leave 7 lines under the 300 cap: PS1 changes there must be net zero.
- **D-27:** Each plan runs `git grep -nIiE 'cogmaster|muhtari|master system|sms|7800|palette'` once and records the
  command and exit status (expected 1).

#### Test inventory reading (FRAME-01 vs AGENTS rule 14)
- **D-28:** `tests/inventory/ci.txt` keeps its name and holds only the preset's directory and label selection,
  written once in plan 1. Each directory's test names live in `tests/<dir>/inventory.txt`. The exact-inventory check
  compares the registered tests (JUnit) with the union of the selected directories' inventories, executed once
  each, none skipped. This keeps FRAME-01's file and removes the single-file merge hotspot (XFER-083).

#### Plan shape (AGENTS rule 14)
- **D-29:** Plan 1: `CMakeLists.txt` with OBJECT libraries (`instance`, `testcard`, `state`, `fault`/`frame` as the
  seed has them) linked into the shipped library, presets (`dev`, `ci`, `ci-msvc`, `asan`, `nofp`, `be32`, `wasm32`,
  `perf`), toolchains, `cmake/SchwaystationTests.cmake`, every `tests/<dir>/tests.cmake` and `inventory.txt`, the
  PROVENANCE pin update, the `setup-release-app.sh` removal and the `publish.sh` re-sync. Then parallel waves for
  plans sharing no directory or header (core+state; runner; abi+nofp; hygiene+policy gates); then sequential: the
  libretro adapter and in-process host, install/embed/consumer, the RetroArch e2e, workflows and release. Only plan
  1 and the adapter plan edit `include/schwaystation.h` and `CMakeLists.txt`. A plan past 30 minutes is split.

### Claude's Discretion
- The state visitor field path for the counter (`instance.frame` or `testcard.frame`).
- Exact OBJECT-library boundaries beyond the D-29 list, and how the seed's `fault.c`/`frame.c` map onto PS1 modules
  in phase 1 (first-fault itself is phase 2).

### Deferred Ideas (OUT OF SCOPE)
- The gates listed in D-25 become one todo each, bound to the next phase that adds their first caller.
- Arming the Ir/frame gate, promoting `wasm32` from report-only and the rehearsal filter belong to the phase-1 `ci:`
  tail step on `phase/01-ci-tail` (DEC.99), not to this phase's plans.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description (REQUIREMENTS.md, abridged) | Research support |
|----|-------------|------------------|
| FRAME-01 | `cmake --workflow --preset ci` (`ci-msvc`) configures, builds, tests and packages library, runner, core on every platform; exact inventory in `tests/inventory/ci.txt`, each once, none skipped | Seed presets and `check_junit.cmake` (§ Seed inventory); per-directory inventory design (Pattern 2); preset-growth sequencing (Pitfall 1) |
| FRAME-02 | `--frames 60 --hash`, no BIOS/disc, identical hash on 6 legs + `be32`; `wasm32` same hash, report-only; `hash-equality` over eight `hashes.txt` | Frame-hash serialisation (Pattern 4); candidate pin computed independently (Code Examples); seed `hashes.cmake`, be32/wasm32 jobs (§ CI lanes) |
| FRAME-03 | `libretro.host` in-process: 60 frames, XRGB8888 frame equals runner conversion byte for byte; av_info fps rationals, 44,100, 320 × 240, 4/3 | libretro.h facts at the pin (§ libretro); seed `tests/libretro/host.c`; rationals verified arithmetically |
| FRAME-04 | `abi.symbols`, `abi.globals`, `abi.undefined`, `hygiene.no_float`, `nofp`, header C17/C++17, `docs.budgets` (header < 40 KB, README < 16 KB) | Seed `abi.cmake` with MSVC fixes; missing `abi.symbols.self_test` (Pitfall 6); budget values to change |
| FRAME-05 | `hygiene.sh --tree`, `ci.title`, `hygiene.timeout_comments`, SHA-pin check, `hygiene.no_bios_bytes`, `policy.no_skip`, each with a `.self_test`; `policy.gate_budget` ≤ 1,500; `build.revision_matches_head` | Gate-budget arithmetic (§ Gate budget); `workflows.cmake` merge; `revision.cmake` and `no_bios_bytes` designs (Patterns 6, 7) |
| FRAME-06 | Rehearsal job in `CI required` → 18 version-free zips, `SHA256SUMS`, verified attestations, deletes itself; `release.archive_contents`; release-please `simple`/draft-by-ID | Seed `ci.yml` rehearsal/cleanup, `release.yml`, `publish.sh` (re-sync diff shown); manifest must start at `0.0.0` (Pitfall 9) |
| FRAME-07 | README install line in CI against the macOS arm64 zip; RetroArch 1.22.2 DMG pinned; `--max-frames=5` screenshot equals `--frames 5 --png 5:FILE`; one-byte negative control; `policy.e2e_budget` ≤ 300 | Seed `tests/retroarch/*` (293 lines), DMG pin row, `install_line.cmake`; frame-alignment risk (Pitfall 3) |
| FRAME-08 | `install.consumer`, C++ consumer, `embed.subdirectory`; `tests/api/v1.c` touches every public name | Seed `tests/install/tests.cmake`, `tests/consumer`, `tests/embed`, `api_version.cmake` |
| FRAME-09 | `--bench` p50/p90/p99/p99.9/p99.99/p99.999, max µs, max RSS; Cachegrind Ir/frame per PR, report-only; `core.no_alloc_after_load`; `core.no_host_clock`; visitor round trip | Seed bench already prints p90 and max (D-22 item already done); seed `perf` job; muhtari `tests/alloc.h`; `no_host_clock` design (Pattern 8) |
</phase_requirements>

## Summary

The phase is a copy-and-rename port of the owner's cogmaster v0.1.0 (verified public: `gh api
repos/szTheory/cogmaster/commits/f09ac97096acda99888270e4c1e4dc96e12ffdfe`, exit 0), whose own phase 1 shipped the same
walking skeleton on 2026-10-20 with a green `push` CI run (39346572559), a green PR run with rehearsal (39345717969,
rehearsal 340 s) and a release with 19 assets. 151 non-planning files exist at the pin; 13,145 lines in total;
2,218 gate lines (`scripts/*.sh tests/cmake/*.cmake .githooks/*`) and 293 e2e lines `[VERIFIED: gh api tree at
f09ac97096ac, wc -l this session]`. The mechanics (presets, inventory check, JUnit check, `expect_fail.cmake`, the
`changes`/`CI required` job graph, Cachegrind job, rehearsal with App token, release-please draft-by-ID publish,
README install line test, RetroArch DMG job) are all present and hosted-proven in the seed. What is new is PS1
specific: the 320 × 240 15-bit animated card with a 13-word tuple, the PS1 rationals and silent audio, a public state
API, a per-directory inventory split, and three fresh gates (`line_budget`, `revision`, `no_bios_bytes`) plus
`core.no_host_clock`.

The seed's C API is shaped differently from D-12/D-13 (`cogmaster_load`/`cogmaster_media`/`cogmaster_get_frame` with a
caller buffer, an RGB444 field, no public state or audio API), so `src/`, `include/`, `tests/core/*`, `tests/api/v1.c`,
`tests/header/*` and `tests/consumer/*` are structural seeds that are substantially rewritten, while `runner/` helpers
(SHA-256, CRC-32, PNG, bench, clock, RSS, stop), every `tests/cmake/*.cmake` gate, the workflows and the e2e files port
nearly verbatim with renames. The single largest planning hazard is sequencing: CMake fails at generate time for any
source file the top-level `CMakeLists.txt` names but that does not yet exist, and the `ci` workflow preset's
package/`ci-release` steps error with no `release`-labelled test. Both force specific content into plan 1 (see
Pitfall 1 and the wave plan).

**Primary recommendation:** Port cogmaster v0.1.0 file by file with the rename map; plan 1 lands the CMake skeleton
plus a thin real core tracer (every source file the top-level `CMakeLists.txt` names, with the complete public
header), and a `ci` workflow preset without the package steps; plan 7 (install/packaging) adds `package` and
`ci-release` to the `ci`/`ci-msvc` workflows. Keep the gate-code total ≤ 1,450 by trimming comments (≈ 200 lines must
go; arithmetic below).

## Architectural Responsibility Map

| Capability | Primary tier | Secondary tier | Rationale |
|------------|-------------|----------------|-----------|
| Test card pixels, tuple, frame counter, tick/audio accumulator, state visitor | Core library (`src/`, determinism boundary) | — | Integer-only, no globals, hashed; must be identical on eight lanes |
| 15-bit → XRGB8888 conversion | Host helper (`host/`, outside the boundary) | runner, adapter | D-05: one converter for `--png` and the adapter; nothing it produces is hashed |
| SHA-256 frame hash, PNG, bench timing, RSS, signals | Runner (`runner/`) | — | Host side may read clocks and files; the core never does (AGENTS rule 4) |
| libretro entry points, av_info doubles, core option, audio batch, serialize | libretro module (`libretro/`) | in-process host test | Thin client of the public header; only `retro_*` exported |
| Binary-shape gates (nm), header compile | CTest `abi` label on the built archive | `nofp` lane | Run on each leg where the archive is built |
| Repository gates (tree, title, timeouts, pins, budgets, revision) | `hygiene` preset (CMake `-P` / POSIX sh) | CI `hygiene`/`title` jobs | No build needed; budgeted lines |
| Cross-lane equality, perf, rehearsal, release | GitHub Actions (`ci.yml`, `release.yml`, `nightly.yml`) | `scripts/publish.sh` | Only hosted runs can show it (AGENTS rules 6, 11) |
| Real-frontend frame equality | Hosted macos-15 job + `tests/retroarch/` | — | RetroArch aborts in the agent sandbox; ≤ 300 lines |

## Project Constraints (from CLAUDE.md / AGENTS.md)

- Never open DuckStation, Mednafen/Beetle PSX, PCSX-ReARMed, PCSX-Reloaded, the PCSX-Redux core, Avocado, Rustation,
  MiSTer PSX; reuse only MIT/BSD/ISC/zlib/CC0 with notice (rule 1).
- Three deliverables only; no window/audio/input-device code (rule 2).
- No BIOS, disc, EXE, card or state bytes outside `tests/fixtures/manifest.txt`; no personal paths, e-mails or names;
  noreply identity; `scripts/hygiene.sh` on commit and push (rule 3).
- Integer-only core, all state in the instance, same hashes everywhere, bounded `run_frame`, never reads clock, file
  or environment (rule 4).
- Every behaviour change lands with a test run by `cmake --workflow --preset ci`; README short and user-facing;
  maintainer docs in `docs/`; public header terse (rule 5).
- Automation-first verification; the owner is asked only for identity-bound consent (rule 6).
- Gate code CMake `-P` or POSIX sh, absolute budget (1,500 in phase 1), e2e < 300 lines checked by a test; every gate
  names the fault it prevents; code review before verification; review findings about test code become seeds
  (rule 7).
- No ceremony, quotas, slots or self-metering (rule 8). One GSD step per session; never `--auto`/`--chain`; record
  runtime and model in STATE.md (rule 9).
- Branch `phase/01-a-test-frame-in-retroarch` (current branch, verified); PR with Conventional Commit title; `gh pr
  merge --auto --squash` in a shell after `/gsd-ship` (rule 10).
- Evidence = command + exit status + run URL, written from the repository root, no `cd` chains (rule 11).
- Seeds pinned to commits `gh api .../commits/<sha>` answers 200 for; sibling code fetched with `gh api` only
  (rule 12).
- Plan 1 lands the CMake skeleton and per-directory inventories; parallel waves only where no inventory or header is
  shared; a plan past 30 minutes is split; no "non-gating" test (rule 14).
- SUMMARY files carry `requirements-completed:` and a block-style `coverage:` list (no flow style, no inline `#`);
  acceptance checks assert on exit status and tolerant regexes.
- Style: one module per chip, fixed-width types, `schway_` public names, `schway__<module>_` internals, hardware
  comments cite sources, no planning IDs or other emulators' identifiers in code/test names/user docs.

## Existing Tree (what plans modify rather than create)

`git ls-files` this session (non-`.planning`): `.clang-format`, `.claude/CLAUDE.md`, `.env.example`, `.gitattributes`,
`.githooks/{pre-commit,pre-push}`, `.github/rulesets/main.json`, `.gitignore`, `AGENTS.md`, `ASSET_POLICY.md`,
`CLAUDE.md` (symlink), `LICENSE`, `PROVENANCE.md`, `scripts/{apply-repo-settings,check-title,classify-changes,
fetch-pin,hygiene,publish,setup-release-app}.sh`, `version.txt` `[VERIFIED: git ls-files]`.

| File | State vs seed (diffed this session) | Phase-1 action |
|------|------|------|
| `scripts/check-title.sh`, `classify-changes.sh`, `fetch-pin.sh`, `.githooks/*` | byte-identical to the seed | keep; trim comments for budget |
| `scripts/apply-repo-settings.sh` | seed with `COGMASTER_APP_*` → `SCHWAYSTATION_APP_*` only | keep; trim comments |
| `scripts/hygiene.sh` (216 lines) | PS1-adapted: `MANIFEST=tests/fixtures/manifest.txt`; PS-X EXE signature as hex `50532d5820455845`; ISO `CD001` as hex `4344303031` at offsets 32769 and 37657; card `4d43` at 131,072 bytes; any NUL-bearing 524,288-byte file = BIOS by size | keep; trim comments; **delete line 13** `# Adapted from the owner's cogmaster project (MIT).` — it fails D-27's grep and §5a puts provenance in the commit body |
| `scripts/publish.sh` (176) | stale: lacks the seed's `SIGNER_WORKFLOW` logic (seed lines 14–16, 98–106, 130–131 differ) | re-sync from seed (182 lines), rename |
| `scripts/setup-release-app.sh` (115) | one-off | `git rm` (D-24) |
| `version.txt` | `0.0.0` | unchanged; release-please manifest starts `{".": "0.0.0"}` |
| `.github/rulesets/main.json` | requires `CI required`, `integration_id` 15368, squash only | apply once CI reports (owner admin command, DEC.88) |
| `.gitignore` | already PS1 (`*.bin *.cue *.iso *.img *.chd *.pbp ... *.exe ...`, `!/tests/fixtures/**`, `/build/`) | keep |
| `PROVENANCE.md` | §2 row PV.12 and §4 row 1 still name `abd950941e93`; §8 libretro.h SHA "recorded at vendoring" | plan 1 updates to `f09ac97096ac` and records the SHA |

Repository runtime state (checked with `gh api` this session): remote `origin` exists; branch
`phase/01-a-test-frame-in-retroarch` checked out; Actions variable `SCHWAYSTATION_APP_CLIENT_ID` and secret
`SCHWAYSTATION_APP_PRIVATE_KEY` exist; **no ruleset** is applied yet; **0 releases**. `sh scripts/hygiene.sh --tree`
exits 0 on the current tree.

## Seed Inventory and Disposition (cogmaster @ `f09ac97096ac`)

Fetched with `gh api -H 'Accept: application/vnd.github.raw' repos/szTheory/cogmaster/contents/<path>?ref=<pin>`;
line counts by `wc -l` this session. "Port" = copy + D-21 renames; "Rewrite" = keep structure, PS1 content;
"Drop/seed" = D-24/D-25.

| Seed path (lines) | Disposition | schwaystation path / note |
|---|---|---|
| `CMakeLists.txt` (205) | Rewrite with muhtari OBJECT pattern | top-level; options `SCHWAYSTATION_BUILD_{RUNNER,TESTS,LIBRETRO}`, `SCHWAYSTATION_HYGIENE`, `SCHWAYSTATION_NOFP`, `SCHWAYSTATION_TIMEOUT_SCALE`, `SCHWAYSTATION_INVENTORY` |
| `CMakePresets.json` (303) | Port + rename `bench` → `perf` (D-29) | add `hygiene` (needed by FRAME-05; D-29's list omits it) |
| `cmake/CogmasterTests.cmake` (69) | Port + per-directory union | `cmake/SchwaystationTests.cmake`, `schway_add_test` |
| `cmake/cogmaster.pc.in` (9), `cogmasterConfig.cmake.in` (6), `version.h.in` (9) | Port | `cmake/schwaystation.pc.in`, `schwaystationConfig.cmake.in`, `version.h.in` → `schwaystation_version.h` |
| `cmake/packaging.cmake` (59) | Port | names `schwaystation-{library,runner,libretro}-{os}-{arch}` |
| `cmake/toolchains/ppc-linux-gnu.cmake` (14), `wasm32-wasi.cmake` (20) | Port verbatim | — |
| `include/cogmaster.h` (115) | Rewrite (D-12/D-13) | `include/schwaystation.h` |
| `src/internal.h` (182) | Rewrite (pins per D-16) | `src/internal.h` |
| `src/instance.c` (199), `frame.c` (96), `testcard.c` (114), `state.c` (229), `state.h` (159), `crc32.c` (54) | Rewrite / port | `crc32.c` ports verbatim; `state.c/h` keep SIZE/WRITE/VALIDATE/APPLY/RECORD walk, add public visitor |
| `src/fault.c` (42) | Drop in P1 | `get_fault` is P2 (D-19); no consumer (D-11) |
| `host/palette.c/h` (39) | Drop | replaced by `host/xrgb8888.{c,h}` (D-05, D-22) |
| `runner/{sha256,crc32,png,bench,clock,rss,stop}.{c,h}` (≈ 650) | Port | already print p50/p90/p99/p99.9/p99.99/p99.999/max_us/max_rss_kib (seed `tests/runner/tests.cmake` L63–64) |
| `runner/main.c` (441) | Rewrite options | `--frames N`, `--hash`, `--png N:FILE`, `--bench N`, `--region ntsc-u\|ntsc-j\|pal`, `--help`, `--version` |
| `runner/exit_status.h` (23) | Rewrite | statuses 0 ok, 1 failure, 2 usage (the only ones P1 returns; seed rule "a status enters the table when the runner first returns it") |
| `libretro/CMakeLists.txt` (44), `cogmaster_libretro.info.in` (28), `libretro.c` (367) | Rewrite | drop RGB565/GG test modules; XRGB8888 only (HOSTS §1) |
| `libretro/libretro.h` | Do not copy (D-23) | vendor from libretro-common `a3194c18ba65` |
| `tests/tests.cmake` (24) | Port | include every directory |
| `tests/testcard_pins.cmake` (10) | Rewrite | `SCHWAYSTATION_TESTCARD_SHA256` (+ PNG digest) |
| `tests/check.h` (48) | Port | — |
| muhtari `tests/alloc.h` (54, @`d698364b180d`) | Port (D-18) | commit body names muhtari |
| `tests/core/{test_core.c (879), test_state.c (724), tests.cmake (41)}` | Rewrite | PS1 cases |
| `tests/api/v1.c` (174), `tests/header/header.{c,cpp}` (100) | Rewrite | D-16 checks |
| `tests/abi/{tests.cmake (122), global_fixture.c, libc_fixture.c, nofp_fixture.c}` | Port + add `abi.symbols.self_test`, `core.no_host_clock(.self_test)` | move `abi.nofp_fixture` to `tests/nofp/` (Pattern 2) |
| `tests/consumer/*` (126), `tests/embed/CMakeLists.txt` (39), `tests/install/tests.cmake` (75) | Port + new expected text | PASS_REGULAR_EXPRESSION `consumer: api 1, one frame 320x240` |
| `tests/libretro/{host.c (514), tests.cmake (60)}` | Rewrite checks | keep `dlopen`/`LoadLibraryA` loader; drop `libretro.vendored`, `info_claims` (D-25) |
| `tests/runner/{tests.cmake (321), test_png.c (213), test_runner.c (149)}` | Port + PS1 | drop `runner.exit.table` (D-25 `exit_table`); `runner.hash.repeat` inverts (frame 5 ≠ frame 60, D-07) |
| `tests/perf/{tests.cmake (66), allow.txt (6)}` | Port | fixture name `testcard` |
| `tests/policy/tests.cmake` (81) | Port subset | keep `policy.no_skip(+self_test)`, `docs.budgets`; drop `single_version*`, `docs.api_names*`, `policy.notices` (D-25) |
| `tests/release/tests.cmake` (65) | Port subset | keep `release.archive_contents(+self_test)`; drop `release.floor*` (D-25) |
| `tests/hygiene/{tests.cmake (395), fixtures/titles.txt (36)}` | Port subset + rename | `policy.title` → `ci.title`; drop `policy.pins*`, `policy.ci_required*`; `hygiene.format`/`hygiene.analyzer` are not FRAME-named (see Open Question 4) |
| `tests/inventory/{ci (86), hygiene (25), nofp (13), wasm32 (82)}.txt` | Rewrite as directory selections (D-28) | Pattern 2 |
| `tests/retroarch/*` (293) | Port, net-zero line change (D-26) | `FRAMES=5`; expected `320x240` |
| `tests/cmake/*.cmake` | see § Gate budget | — |
| `.github/workflows/ci.yml` (606), `release.yml` (170), `nightly.yml` (137) | Port + rename | perf job builds preset `perf`; RetroArch job `--frames 5 --png 5:runner.png` |
| `release-please-config.json` (13) | Port verbatim | — |
| `.release-please-manifest.json` (3) | **Do not copy value** | seed holds `"0.1.0"`; schwaystation starts `{".": "0.0.0"}` (ENGINEERING §5) |
| `pins.txt` (17) | Port | wasmtime 48.0.7 rows and `retroarch-dmg 1.22.2 ... 81b79121ba26d539064ae13b4d0419a120c3d165afbe656cf5f5412b15fdb434` |
| `README.md` (69), `THIRD-PARTY-NOTICES.md` (34), `docs/{embedding,maintaining,runner,testing}.md` | Rewrite | the seed's notices cite RetroArch `69a4f0ea1e8a`, SHA `bd3398d2...`; schwaystation's cites libretro-common |

## Standard Stack

All build tools come from the hosted images or the distro archive; no language-registry package is installed. Versions
of what the seed proved green on 2026-10-20 (cogmaster runs 39346572559, 39345717969) `[VERIFIED: gh api runs/jobs]`:

### Core
| Tool | Version | Purpose | Why |
|------|---------|---------|-----|
| CMake | 4.4.x on images (local 4.4.3); presets v11 needs ≥ 4.3 | build, test, package, `-P` gates | single entrypoint `cmake --workflow --preset <lane>` (ENGINEERING §2) |
| Ninja | 1.13.2 | generator | seed `base` preset |
| GCC 15 / Clang 21 / Apple clang 17 / MSVC (VS 2026 18.10) | per image (ENGINEERING §1) | six legs | warnings fatal via `CMAKE_COMPILE_WARNING_AS_ERROR` |
| clang-21 + ASan/UBSan (`libclang-rt-21-dev`) | apt | `asan` lane | seed job installs runtimes |
| `gcc-powerpc-linux-gnu`, `g++-powerpc-linux-gnu`, `qemu-user` | apt (qemu 10.2.1) | `be32` | seed job |
| clang-21 + `wasi-libc` + `libclang-rt-21-dev-wasm32`, wasmtime 48.0.7 (pinned tarball) | apt + `pins.txt` | `wasm32` | seed job; ENGINEERING §14 says wasi-sdk 34 — the seed's distro toolchain is what ran green |
| Valgrind (Cachegrind) | apt (3.26.0 on 26.04) | `perf` job | seed job |
| RetroArch 1.22.2 Metal universal DMG | SHA-256 `81b79121ba26d539064ae13b4d0419a120c3d165afbe656cf5f5412b15fdb434` | hosted e2e | seed `pins.txt` row; HOSTS §3 re-hashed it |
| release-please-action | v5.0.0 `45996ed1f6d02564a971a2fa1b5860e934307cf7` | release PR + draft | seed `release.yml` |
| actions/checkout v7.0.1 `3d3c42e5…`, upload-artifact v7.0.2 `cf430e03…`, download-artifact v8.0.2 `9000827c…`, attest v4.2.2 `1e69f48a…`, create-github-app-token v3.3.0 `64ab729a…` | full SHAs as in seed `ci.yml` | CI | ENGINEERING §1 table matches the seed's pins |

### Vendored
| File | Pin | SHA-256 (computed this session from the raw API) |
|---|---|---|
| `libretro/libretro.h` | libretro-common `a3194c18ba65ef89734e1898a2f61403c255d368` (committed 2026-10-11) | `036ddffc995ad116c3081ca0a7515d09c2a116f3483e4f5012fe0c29a885da76` (9,105 lines, 342,023 bytes, LF only, ASCII, no e-mail/home path/GPL text) `[VERIFIED: gh api + shasum]` |

**Installation:** no package install step; the CI jobs `apt-get install` the lane tools exactly as the seed's `ci.yml`
L193, L233, L281, L387 do, and `scripts/fetch-pin.sh` fetches wasmtime and the DMG by digest.

## Package Legitimacy Audit

No npm, PyPI or crates package is recommended or installed by this phase. `gsd-tools query package-legitimacy` was not
run because there is nothing in its ecosystems to check.

| Artifact | Source | Verdict | Disposition |
|---|---|---|---|
| apt: `valgrind`, `qemu-user`, `gcc/g++-powerpc-linux-gnu`, `wasi-libc`, `libclang-rt-21-dev`, `libclang-rt-21-dev-wasm32` | Ubuntu 26.04 archive (signed) | n/a (not a registry package) | Approved; the same names ran green in cogmaster's CI |
| wasmtime 48.0.7 tarballs | GitHub release, SHA-256 in `pins.txt` | n/a | Approved; `fetch-pin.sh` verifies before extraction |
| RetroArch 1.22.2 DMG | libretro buildbot, SHA-256 pinned | n/a | Approved |
| `libretro.h` | libretro-common at a public pin | n/a | Vendored unmodified with notice |

**Packages removed due to [SLOP]:** none. **Packages flagged [SUS]:** none.

## Architecture Patterns

### System architecture diagram

```
 host (runner | libretro adapter | in-process test host)
   │ schway_create(cfg{allocator}) ──► ONE allocation: struct schway (VRAM 1024x512 u16, profile, frame,
   │                                   tick accumulator, audio ring 4 frames)
   │ schway_set_profile(region) ─ only before first run_frame
   ▼
 schway_run_frame ──► ticks += frame cycles (exact rational accumulator, NTSC 566,203/566,204)
   │                  pairs = ticks/768 boundaries crossed (737/738 NTSC, 886/887 PAL) ──► audio ring (silence)
   │                  frame += 1 ──► testcard: draw 320x240 into VRAM[0..239][0..319] ──► tuple (13 words)
   ▼
 schway_get_video ──► {pixels → VRAM, pitch, width 320, height 240, tuple, frame}
   ├─► runner: SHA-256(13 LE u32 + rows LE u16) ──► "frame N sha256 <hex>" ──► hashes.txt ──► hash-equality (8 lanes)
   ├─► runner --png: host/xrgb8888 ──► PNG ──► (CI) sips→BMP ──► compare ◄── RetroArch screenshot
   └─► adapter: host/xrgb8888 ──► video_cb(320x240, XRGB8888) ; get_audio ──► audio_batch_cb
 schway_save_state / load_state / state_visit ──► same field walk (SIZE/WRITE/VALIDATE/APPLY) ──► LE bytes + CRC-32
```

### Recommended structure

```
include/schwaystation.h        public header (C17 and C++17), plan 1 and the adapter plan only
src/internal.h                 layout pins, internal prototypes
src/instance.c src/frame.c     OBJECT lib "instance": create/destroy/versions/profile/reset/info/status_string; run_frame/get_video/get_audio
src/testcard.c                 OBJECT lib "testcard": schway__testcard_pixel + draw into VRAM
src/state.c src/state.h src/crc32.c   OBJECT lib "state": visitor walk, header, CRC-32
host/xrgb8888.c host/xrgb8888.h        STATIC lib "schwaystation-host" (runner + libretro)
runner/*.c runner/*.h          schwaystation-run
libretro/{CMakeLists.txt,libretro.c,libretro.h,schwaystation_libretro.info.in}
cmake/{SchwaystationTests.cmake,packaging.cmake,version.h.in,*.in,toolchains/*}
tests/<dir>/{tests.cmake,inventory.txt}   dirs: abi nofp core runner libretro install release perf policy hygiene
tests/{api,header,consumer,embed,retroarch}/   sources only (no CTest registration of their own)
tests/inventory/{ci,hygiene,nofp,wasm32}.txt   directory selections (D-28)
tests/cmake/*.cmake            gate scripts (budgeted)
```

Discretion recommendations: OBJECT libraries `schwaystation_instance` (instance.c, frame.c), `schwaystation_testcard`
(testcard.c), `schwaystation_state` (state.c, crc32.c) linked by `$<TARGET_OBJECTS:...>` into STATIC `schwaystation`
exactly as muhtari L68–82 does (quoted below). `fault.c` does not exist in phase 1. Visitor path for the counter:
**`instance.frame`** (the counter is the instance's metadata field per D-02, and `testcard` is retired into `gpu` in
phase 4, so a `testcard.*` path would be renamed in a later state format).

```cmake
# muhtari CMakeLists.txt L68-82 at d698364b180d (verbatim)
add_library(muhtari_objs OBJECT
  src/instance.c
  src/frame.c
  src/testcard.c
  src/bus.c
  src/tia.c
  src/riot.c
  src/sha1.c
  src/cart.c)
add_library(muhtari_cpu OBJECT src/cpu.c)
add_library(muhtari_cartdb OBJECT src/cartdb.c)
add_library(muhtari STATIC
  $<TARGET_OBJECTS:muhtari_objs>
  $<TARGET_OBJECTS:muhtari_cpu>
  $<TARGET_OBJECTS:muhtari_cartdb>)
```
muhtari applies one `muhtari_core_options(target)` function (L46–60: hidden visibility, PIC,
`-fno-stack-protector -U_FORTIFY_SOURCE -ffile-prefix-map=...`, MSVC `/GS- /Zl`, `-mgeneral-regs-only` under NOFP,
private include dirs, warnings) to the STATIC and every OBJECT target. The abi fixtures copy the core's options with
`get_target_property(... COMPILE_OPTIONS)` (seed `tests/abi/tests.cmake` L14); point that at one OBJECT target.

### Pattern 1: Test registration and exact inventory (seed, verified)
`schway_add_test(NAME n COMMAND ... LABELS ... TIMEOUT s [COST c])` multiplies TIMEOUT by
`SCHWAYSTATION_TIMEOUT_SCALE` in one place; a `cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL ...)` check
compares the top-level `TESTS` property (with sorted, comma-joined labels) against the inventory and fails configure
naming "listed but not registered" and "registered but not listed" (seed `cmake/CogmasterTests.cmake` L13–69).
Subsystem files are `include()`d, never `add_subdirectory()`d, so every test lands in the top directory's `TESTS`.
After the run, `tests/cmake/check_junit.cmake -DJUNIT= -DINVENTORY= [-DLABELS=] [-DEXCLUDE=]` requires each selected
name exactly once with `status="run"` and no `failure|error|skipped` (seed L1–58).

### Pattern 2: Per-directory inventories (D-28 design, recommended)
```
tests/inventory/ci.txt       # one directory per line: abi core install libretro perf policy release runner
tests/inventory/wasm32.txt   # the ci list without libretro (WASI has no dlopen; seed sets SCHWAYSTATION_BUILD_LIBRETRO OFF)
tests/inventory/nofp.txt     # abi nofp
tests/inventory/hygiene.txt  # hygiene
tests/<dir>/inventory.txt    # sorted "name<TAB>labels" lines, the seed's format
```
- `SchwaystationTests.cmake` reads `tests/inventory/${SCHWAYSTATION_INVENTORY}.txt`, concatenates the listed
  directories' `inventory.txt`, sorts, and runs the seed's set comparison; `check_junit.cmake` does the same expansion
  (≈ +10 lines, budgeted). The asan, be32 and ci-release subsets stay label filters (`-DEXCLUDE=abi|release`,
  `-DLABELS=portable`, `-DLABELS=release`) as in the seed's YAML.
- Move `abi.nofp_fixture` from `tests/abi/tests.cmake` (seed L108–122, registered only `if(COGMASTER_NOFP)`) into
  `tests/nofp/`, so each directory's inventory is the same on every preset that includes it.
- `tests/tests.cmake` (plan 1) includes each directory: `hygiene` alone under `SCHWAYSTATION_HYGIENE`, `abi` + `nofp`
  under `SCHWAYSTATION_NOFP`, and `libretro` only `if(SCHWAYSTATION_BUILD_LIBRETRO)` (seed L5–24 shape).
- Plan 1 writes every `tests/<dir>/tests.cmake` and `inventory.txt` (empty or with plan 1's own tests); each later
  plan appends only to its own directory's pair, so parallel plans never touch one file.

### Pattern 3: Presets (seed, with phase-1 sequencing)
Seed configure presets: `base` (hidden, Ninja, `build/${presetName}`), `dev`, `ci` (Release, warnings fatal,
`CMAKE_OSX_DEPLOYMENT_TARGET 10.13`), `ci-msvc` (`cl`, `MultiThreaded$<$<CONFIG:Debug>:Debug>`), `hygiene`
(`CMAKE_OSX_SYSROOT macosx`, `COGMASTER_HYGIENE ON`), `asan` (clang-21, scale 3), `nofp` (gcc), `be32`
(ppc toolchain, scale 8, "the 43 shared tests took 6.7 times as long"), `wasm32` (scale 5,
`CMAKE_CROSSCOMPILING_EMULATOR "${sourceDir}/build/pins/wasmtime/wasmtime;run;--dir=${sourceDir}::${sourceDir}"`),
`bench` `[VERIFIED: seed CMakePresets.json L1–303]`. Test presets use `"noTestsAction": "error", "jobs": ""`; `ci`
excludes label `^release$` and `ci-release` includes it; workflow `ci` = configure, build, test ci, package ci, test
ci-release. Rename `bench` → `perf` (D-29) and update the two YAML places that name it (seed `ci.yml` L396 `--preset
bench`, L406 `grep -q '"name": "bench"'`; `nightly.yml` L49, L83).

### Pattern 4: The frame hash and the tuple
ARCHITECTURE §7: "SHA-256 over the metadata tuple serialised as 13 LE u32 (field, interlaced, vres, hres, dot_div,
region, x1, y1, start_x, start_y, enabled, rgb24, bpp) followed by the display rows: LE u16 per pixel (bpp 16) ...
no padding" `[CITED: .planning/preparation/ARCHITECTURE.md L203]`. X2 and Y2 (D-04) are display-range state used to
derive width/height; they are **not** among the 13 hashed words (DEC.30 lists X1, Y1 only). Region values from D-13:
NTSC-J 1, NTSC-U 2, PAL 3, so the three regions hash differently (useful as three `hashes.txt` lines).

Width check (computed): `((0xC60 − 0x260) / 8 + 2) & ~3 = 320`; `0x100 − 0x10 = 240` `[VERIFIED: python3 -I this
session]`.

### Pattern 5: Tick and audio accumulator (D-14), verified arithmetic
Per frame `acc += 405,350,378,496` (NTSC) or `482,961,862,656` (PAL); `ticks_this_frame = acc / 715,909` (or
`709,379`); `acc %= den`. Then pairs = `ticks_total/768 − ticks_total_prev/768`. Computed over 4,000 frames: NTSC
ticks 566,203–566,204 and pairs 737–738; PAL ticks 680,823–680,824 and pairs 886–887; and `33,868,800 / (num/den)`
equals 53,693,175/897,619 and 53,203,425/1,069,484 exactly `[VERIFIED: python3 -I fractions this session]`. On ppc32
the u64 divide and modulo call `__udivdi3`/`__umoddi3`, already in the seed's `abi.cmake` helper row
`"^Linux/(ppc|powerpc)$:__udivdi3 __umoddi3 __ashldi3 __lshrdi3"`.

### Pattern 6: `build.revision_matches_head` (fresh, `tests/cmake/revision.cmake`)
Configure embeds `git rev-parse HEAD` into a generated runner header; `schwaystation-run --version` prints it (e.g.
`schwaystation-run 0.0.0 <40-hex>`); the CTest runs `git rev-parse HEAD` at test time and fails with a named message
when they differ (the fault: testing a stale configure after a commit, GB-GSD-020). Self-test: the same script with
`-DREVISION=` forty zeros must exit 1 with that message. Register it in `tests/runner/` (it runs the runner).
Keep the revision out of the core and out of `schway_get_versions` (no D-13 field for it). `[ASSUMED]` design.

### Pattern 7: `hygiene.no_bios_bytes` (fresh)
The detection already exists in `scripts/hygiene.sh --tree` (BIOS by size, PS-X EXE/CD001/card by hex signature).
Register `hygiene.no_bios_bytes` as that scan, and its `.self_test` as a throwaway `git init` repository (the
`hygiene.tree.self_test` pattern, seed `tests/hygiene/tests.cmake` L15–31) holding files generated at configure time
from hex (`file(WRITE ...)` of the signature bytes produced from hex strings, and a 524,288-byte NUL-bearing file),
expecting `hygiene: BIOS image by size` / `PlayStation executable by content`. No signature bytes are committed.
Zero new gate lines beyond the registration. `[ASSUMED]` design.

### Pattern 8: `core.no_host_clock`
Extend `tests/cmake/abi.cmake` with a mode that fails on any undefined clock symbol in the core archive (regex over
`time`, `clock`, `clock_gettime`, `gettimeofday`, `timespec_get`, `mach_absolute_time`, `mach_continuous_time`,
`QueryPerformanceCounter`, `GetTickCount64`, `GetSystemTimeAsFileTime`), with a `clock_fixture.c` calling
`time(NULL)` as its negative control; register both in `tests/abi/`. `abi.undefined`'s allowlist already excludes
these, so this is a named, single-purpose restatement that FRAME-09 asks for. `[ASSUMED]` design; ENGINEERING §2's
"injected RTC values" form does not apply (the PS1 has no RTC).

### Anti-patterns to avoid
- **`const` pointer tables in the core** (e.g. a `static const char *const names[]` for `schway_status_string`): PIC
  puts them in `.data.rel.ro` (nm `d`) and `abi.globals` fails (ENGINEERING §2, XFER-071). Use a `switch` returning
  literals.
- **Struct bytes in states or hashes**: serialise LE words (D-17); `be32` catches it.
- **Floating point in `src/` or `include/`**: `hygiene.no_float` and `nofp`. The libretro adapter's
  `retro_system_timing.fps` (double) and `retro_game_geometry.aspect_ratio` (float) are unavoidable API types; keep that
  arithmetic in `libretro/libretro.c` only (the seed's comment says the same).
- **Moving gate logic into `tests/<dir>/tests.cmake` to dodge the budget** (D-24). The seed's existing inline sh
  bodies in `tests/hygiene/tests.cmake` (title loop, classify table, publish fixtures) and `tests/perf/tests.cmake`
  were there at the seed; port them where they are, do not move more there.

## libretro Facts (pinned header `a3194c18ba65`, line numbers from this session's copy)

| Item | Value at the pin |
|---|---|
| `RETRO_API_VERSION` | `1` (L97) |
| `RETRO_REGION_NTSC` / `_PAL` | `0` / `1` (L443–444) |
| `RETRO_ENVIRONMENT_SET_PIXEL_FORMAT` | `10` (L870); `RETRO_PIXEL_FORMAT_XRGB8888 = 1` (L6735); default `0RGB1555 = 0` (L6729) |
| `RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME` | `18` (L1056) |
| `RETRO_ENVIRONMENT_GET_VARIABLE` / `SET_VARIABLES` / `GET_VARIABLE_UPDATE` | `15` (L971) / `16` (L1021) / `17` (L1039) |
| `RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO` | `32` (L1370) |
| `RETRO_ENVIRONMENT_SET_GEOMETRY` | `37` (L1559) |
| `RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION` / `SET_CORE_OPTIONS_V2` | `52` (L1871) / `67` (L2362) |
| `struct retro_game_geometry` | `unsigned base_width, base_height, max_width, max_height; float aspect_ratio;` (L7395–7439) |
| `struct retro_system_timing` | `double fps; double sample_rate;` (L7447–7454) |
| `retro_audio_sample_batch_t` | `size_t (*)(const int16_t *data, size_t frames)` (L8653) |

HOSTS-AND-INTEGRATION line references (e.g. "L865", "L1051") come from a different `libretro.h` copy; comments in the
new adapter must cite the vendored file's own lines. `[VERIFIED: grep on the fetched pin]`

Adapter values for phase 1: pixel format XRGB8888 only; `SET_SUPPORT_NO_GAME` true in `retro_set_environment`;
`retro_load_game(NULL)` shows the card, any content refused; base 320 × 240; max 640 × 480 (NTSC) / 640 × 576 (PAL)
`[CITED: HOSTS-AND-INTEGRATION §1 AV table]`; `aspect_ratio = 4.0f/3.0f`; `fps = (double)fps_num / fps_den` from
`schway_get_info`; `sample_rate = 44100.0`; audio through `audio_batch_cb` (single-sample fallback as the seed does);
`retro_get_region` from the profile; `retro_reset` → `schway_reset(POWER)` (D-12 locks this; HOSTS §1's "warm reset"
row applies from P2). Region option: recommend `SET_CORE_OPTIONS_V2` (67) when `GET_CORE_OPTIONS_VERSION` (52)
reports ≥ 2, else `SET_VARIABLES` (16); read with `GET_VARIABLE` at load and on `GET_VARIABLE_UPDATE` in
`retro_run`, then recreate the instance and send `SET_SYSTEM_AV_INFO` (32) because fps changes. The in-process host
answers `GET_VARIABLE` with `pal` in a second case to prove the PAL rationals. `retro_serialize_size/serialize/
unserialize` wrap the state API (D-17); keep `.info` `savestate = "false"` in phase 1 (HOSTS §1: true only with
v3.0's guard tests).

In-process test host (seed `tests/libretro/host.c`): resolves the 25 entry points with `dlopen`/`LoadLibraryA`,
answers `SET_SUPPORT_NO_GAME` and `SET_PIXEL_FORMAT`, compares the delivered frame's hash with the pinned card hash,
and checks av_info with `CHECK_EQ_DBL(av.timing.fps, 1640625.0 / 27379.0)` style exact double equality (seed L436–438).
For FRAME-03 compare `fps` with `53693175.0 / 897619.0` (the same expression the adapter computes, so bit-equal), and
assert the delivered XRGB8888 buffer equals `host/xrgb8888` applied to `schway_get_video` of an instance run in the
host itself (byte for byte, not only by hash).

## RetroArch Hosted E2E Facts

- Seed job (`ci.yml` L311–363): download `archives-macos-arm64`, unzip libretro and runner zips, `fetch-pin.sh
  retroarch-dmg`, `hdiutil attach -nobrowse -readonly`, `ditto` the app, assert `CFBundleShortVersionString` =
  `1.22.2`, run `run_retroarch.cmake` (fresh run dir, `HOME`/`CFFIXED_USER_HOME` redirected, `--verbose --config
  test.cfg -L core --max-frames=N --max-frames-ss --max-frames-ss-path=SHOT`, 50 s timeout), `sips
  --deleteColorManagementProperties` and `sips -s format bmp` on both PNGs, build `compare` from `bmp.c`+`compare.c`,
  `grep -q '^256x192, every pixel equal$'`, then `flip.cmake` inverts pixel (7,3) and expects exit 1 naming `7,3`.
- PS1 changes: `-DFRAMES=5`; runner `--frames 5 --png 5:runner.png`; the YAML regex becomes `320x240, every pixel
  equal` (YAML is not counted by `policy.e2e_budget`, which counts `tests/retroarch/` only). Pixel (7,3) is in rule 1
  (white) so the flip still produces a difference.
- Config `test.cfg.in` sets `video_gpu_screenshot = "false"`, so the screenshot is the core's frame at native size
  (cogmaster observed 256 × 192 exactly; run 39346572559 job `retroarch` success, 23 s).
- Add an `actions/upload-artifact` of `build/frames/*.png` on failure in the YAML so the agent can read the counter
  strip if D-08's offset appears (no e2e lines spent).

## CI Lanes and Release (seed mechanics to port)

| Job | Seed behaviour (verified in `ci.yml`) | Seed duration (run 39346572559 / PR run 39345717969) |
|---|---|---|
| `changes` | `heavy=false` only for an all-`.planning/` PR; `release_inputs` for `release.yml`, `ci.yml`, `publish.sh`, `release-please-config.json`, `cmake/packaging.cmake` | 6 s |
| `hygiene` / `title` | `cmake --workflow --preset hygiene` + JUnit check; title via `gh pr view` → `check-title.sh` | 18 s / 5 s |
| `build` × 6 | `cmake --workflow --preset ci|ci-msvc`, two JUnit checks (EXCLUDE=release, LABELS=release), README install line on macOS arm64 (+ `-DFLIP=1`), `commit.txt`, upload `archives-*` and `hashes-*` | 13–58 s |
| `asan`, `nofp` × 2 | presets; JUnit EXCLUDE `abi|release` / nofp inventory | 16 s; 6–8 s |
| `be32` | apt cross GCC + qemu; LABELS=portable; uploads `hashes-be32` | 36–52 s |
| `hash-equality` | needs build + be32; drops wasm32; `hashes.cmake -DCOUNT=7 -DMODE=compare` | 8 s |
| `wasm32` | report-only, not in `CI required` needs; compares with Linux x64 and prints the result | 19–40 s |
| `retroarch` | macos-15; above | 23 s |
| `perf` | PR/dispatch only; head vs merge base (base built only if it has the `bench` preset, else `NO_BASE`); `(Ir@360 − Ir@60)/300`; `COGMASTER_PERF_MODE: report` | 27 s |
| `rehearsal` + `cleanup` | same-repo PR with `release_inputs`; App token; `publish.sh prepare/rehearsal-create/upload/attested/verify`; `actions/attest` with `subject-checksums`; tag `rehearsal-${run_id}-${run_attempt}`; `SIGNER_WORKFLOW: ci.yml`; cleanup always deletes | 340 s; 10 s |
| `CI required` | jq over `toJSON(needs)` with the skip exceptions | 4 s |

FRAME-02 says `hash-equality` compares **eight** files while `wasm32` is report-only; the seed compares seven and
reports wasm32 separately (`rm -rf hashes/hashes-wasm32`, `-DCOUNT=7`). Keep the seed's shape in phase 1 and have the
`wasm32` job print the comparison (DEC.99 moves it into the gate in the `ci:` tail). Note this explicitly in the plan
so the verifier does not read FRAME-02 as unmet.

`release.yml` (seed L1–170): release-please with the App token (`permission-contents/-pull-requests/-issues: write`),
`gh pr merge --auto --squash` on the release PR, `release-ci` calls `ci.yml` with `ref: sha` and the union of
permissions, `publish` by draft ID (`gh api releases/$ID`), attest, `PATCH draft=false make_latest=true`, `publish.sh
verify`, and `install-line-latest` runs the README line against `releases/latest/download/`. The release App variable
and secret names become `SCHWAYSTATION_APP_CLIENT_ID` / `SCHWAYSTATION_APP_PRIVATE_KEY` (both exist on the repo); the
"not configured" message must stop naming `scripts/setup-release-app.sh` once it is removed (point to
`docs/maintaining.md`).

## Gate Budget (FRAME-05, D-24)

Counted by `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*`. Current tree: 812 lines (verified). Seed: 2,218.

| File | Lines now | comment / blank | Phase-1 fate |
|---|---|---|---|
| `scripts/hygiene.sh` | 216 | 60 / 17 | keep, trim |
| `scripts/publish.sh` | 176 → seed 182 | 29 / 11 | re-sync |
| `scripts/apply-repo-settings.sh` | 171 | 35 / 18 | keep, trim |
| `scripts/fetch-pin.sh` / `classify-changes.sh` / `check-title.sh` | 52 / 46 / 29 | 11 / 16 / 8 | keep, trim |
| `scripts/setup-release-app.sh` | 115 | — | remove |
| `.githooks/*` | 7 | 5 | keep |
| `tests/cmake/abi.cmake` | 131 | 31 / 4 | port (+ clock mode ≈ 8) |
| `api_version.cmake` | 48 | 8 / 1 | port (backs FRAME-08's "v1.c touches every name") |
| `archive_contents.cmake` | 76 | 8 / 3 | port |
| `budgets.cmake` | 32 | 6 / 2 | port; limits `README.md=16384`, `include/schwaystation.h=40960`, docs `61440` (seed has 12288 / 24576 / 40960) |
| `check_junit.cmake` | 58 | 8 / 2 | port (+ directory union ≈ 10) |
| `expect_fail.cmake` | 27 | 8 / 2 | port |
| `hashes.cmake` | 48 | 10 / 0 | port |
| `install_line.cmake` | 119 | 20 / 3 | port |
| `no_float.cmake` / `no_skip.cmake` | 55 / 38 | 13 / 8 | port |
| `perf_compare.cmake` | 126 | 25 / 7 | port |
| `timeouts.cmake` + `action_pins.cmake` → `workflows.cmake` | 34 + 32 | 12 | merge (≈ 50) |
| `line_budget.cmake` (replaces `e2e_budget.cmake` 27) / `revision.cmake` | new ≈ 30 / ≈ 25 | — | write |
| dropped: `check_hash` 24, `ci_required` 106, `floor` 134, `pins` 49, `single_version` 49, `notices` 57, `vendored` 38, `exit_table` 29, `api_names` 61, `info_claims` 35 | — | — | D-24/D-25 |

Arithmetic: ported as-is the kept set is ≈ 703 (scripts + hooks) + ≈ 870 (cmake) + ≈ 75 (new and extensions) ≈ 1,650
lines, **above** 1,500. The kept files hold ≈ 320 comment and ≈ 90 blank lines; trimming headers to "usage + Fault
prevented" (D-24) and dropping most explanatory comment lines must remove ≈ 200–230 lines to reach ≤ 1,450
`[VERIFIED: grep -c on seed and tree this session; totals derived]`. Recommend each plan that ports a gate file trims it
in the same commit and records `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1` with its exit status,
and that the hygiene plan lands `policy.gate_budget` with the ceiling 1,500 so any over-run fails locally first.

## Plan and Wave Decomposition (recommended)

Constraints honoured: rule 14 (plan 1 = skeleton + inventories), D-29 (only plans 1 and the adapter plan edit the
public header and top-level `CMakeLists.txt`), ≤ 30 min each (cogmaster's twelve phase-1 plans took 5–13 min each
`[VERIFIED: cogmaster 01-0N-SUMMARY.md durations]`).

| Wave | Plan | Owns (files/directories) | Depends on | Requirements |
|---|---|---|---|---|
| 1 | **01 Skeleton + tracer** | `CMakeLists.txt` (core OBJECT libs, host lib, runner target, options, install, `include(cmake/packaging.cmake)`, tests include; **no** libretro yet), `CMakePresets.json` (all configure presets incl. `hygiene`, `perf`; `ci`/`ci-msvc` workflows = configure/build/test only), `cmake/*`, toolchains, `include/schwaystation.h` (complete D-12/D-13 surface), `src/*` (thin real implementations of every D-12 function so `v1.c` can link in wave 2), `host/xrgb8888.*`, `runner/` minimal (`--frames N --hash` + `sha256.*`), `tests/tests.cmake`, every `tests/<dir>/{tests.cmake,inventory.txt}`, `tests/inventory/*.txt`, `tests/cmake/{expect_fail,check_junit}.cmake`, `tests/check.h`, `tests/testcard_pins.cmake`, one pinned test (`runner.hash`) and `policy.no_skip(+self_test)`, `libretro/libretro.h` vendored, `THIRD-PARTY-NOTICES.md`, short `README.md`, PROVENANCE §2/§4/§8, `git rm scripts/setup-release-app.sh`, `publish.sh` re-sync, hygiene.sh line 13 | — | FRAME-01 (part), FRAME-02 (part) |
| 2 | **02 Core + state** | `src/*` (fixes only, no new names), `tests/core/*`, `tests/alloc.h`, `docs/embedding.md`, docs band section (D-10) | 01 | FRAME-09 (alloc, visitor), FRAME-02 |
| 2 | **03 Runner** | `runner/*`, `host/*` behaviour, `tests/runner/*` (`--png`, `--bench`, `--region`, signals, usage, `runner.xrgb8888` known-answer D-06, `runner.hashes_file`, `build.revision_matches_head`), `tests/cmake/{hashes,revision}.cmake`, `docs/runner.md` | 01 | FRAME-02, FRAME-05 (revision), FRAME-07 (png), FRAME-09 (bench) |
| 2 | **04 ABI + nofp + header + API** | `tests/{abi,nofp,header,api}/*`, `tests/cmake/{abi,api_version}.cmake` | 01 | FRAME-04, FRAME-08 (v1.c), FRAME-09 (no_host_clock) |
| 2 | **05 Hygiene + policy + perf gate tests** | `tests/{hygiene,policy,perf}/*`, `tests/cmake/{no_float,no_skip?,budgets,line_budget,perf_compare}.cmake`, `scripts/*` trims, `docs/testing.md` | 01 | FRAME-04 (no_float, budgets), FRAME-05, FRAME-09 (perf gate tests) |
| 3 | **06 libretro adapter + in-process host** | `libretro/*`, `tests/libretro/*`, top-level `CMakeLists.txt` (add option + `add_subdirectory(libretro)`), header only if a gap appears | 02, 03 | FRAME-03 |
| 4 | **07 Install, embed, consumer, packaging, release.archive_contents, README line** | `tests/{install,consumer,embed,release}/*`, `tests/cmake/{archive_contents,install_line}.cmake`, `CMakePresets.json` (add `package` + `ci-release` to `ci`/`ci-msvc` workflows), `README.md` | 06 | FRAME-01, FRAME-06 (archives), FRAME-07 (line), FRAME-08 |
| 5 | **08 ci.yml + RetroArch e2e** | `.github/workflows/ci.yml` (all jobs except rehearsal/cleanup), `tests/retroarch/*`, `pins.txt`, `tests/cmake/workflows.cmake`, `tests/hygiene` additions `hygiene.timeout_comments(+self_test)`, `policy.action_pins(+self_test)` | 07 | FRAME-02, FRAME-05, FRAME-07, FRAME-09 (perf job) |
| 6 | **09 Release + rehearsal + nightly** | `release.yml`, `nightly.yml`, `release-please-config.json`, `.release-please-manifest.json` (`0.0.0`), `ci.yml` rehearsal/cleanup jobs, `docs/maintaining.md` | 08 | FRAME-06, FRAME-09 (nightly bench) |
| 7 | **10 Hosted drive green** | push branch, draft PR, fix what only hosted legs show, record run URLs, measure be32/wasm32 per-PR cost and timeouts, settle D-08 alignment | 09 | all (evidence) |
| 8 | **11 Close** | timeouts from measured runs, `policy.gate_budget` re-pinned to measured, `.planning/config.json` `test_gate_timeout` (GSD-HANDOFF: twice the measured gate time), D-25 seed todos, preparation-doc deviations, the owner's ruleset command | 10 | FRAME-01, FRAME-05 |

Why 02–05 can be parallel: each owns disjoint `tests/<dir>/` pairs; shared infrastructure (`expect_fail.cmake`,
`check_junit.cmake`, `check.h`, `tests/tests.cmake`, inventories, header, `CMakeLists.txt`) is all plan 1's. Wave-2
plans read the header but do not edit it; if a wave-2 plan finds a header gap, it stops and the adapter plan (the only
other header editor) or a follow-up absorbs it. `config.json` has `"parallelization": false`, so GSD will run the wave
sequentially anyway unless the owner changes it; the decomposition still avoids merge hotspots (XFER-083).

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---|---|---|---|
| Inventory / JUnit completeness | a new checker | seed `SchwaystationTests.cmake` + `check_junit.cmake` (+ directory union) | hosted-proven; handles labels, sort, once-each |
| Negative controls | `WILL_FAIL` | seed `expect_fail.cmake` (exact code + regex, `detect_leaks=0`) | bare WILL_FAIL passes on any crash (XFER-013) |
| nm link-surface checks | ad-hoc scripts | seed `abi.cmake` (already handles Mach-O `_` prefix, MSVC `??_C@_`, `__xmm@`, `__isa_available`, `bzero`, GOT, ppc libgcc) | cost cogmaster a hosted fix round (01-11 deviation) |
| Release publish/verify | new shell | seed `publish.sh` (draft by ID, SHA256SUMS C-locale order, signer workflow split) | XFER-039; rehearsed green |
| SHA-256, CRC-32, PNG writer | new code | seed `runner/sha256.c`, `crc32.c`, `png.c` (stored deflate blocks) | tested by `runner.png` with an independent parse |
| Percentiles | interpolation | seed `runner/bench.c` nearest-rank | `runner.bench.percentile` |
| Counting allocator | new harness | muhtari `tests/alloc.h` | `fail_from` already there |
| MSVC environment in CI | a marketplace action | seed vswhere + vcvarsall pwsh step | no third-party action |

## Common Pitfalls

### Pitfall 1: Plan 1 cannot configure without every named source, and `ci` cannot pass without packaging tests
**What goes wrong:** CMake stops at generate time ("Cannot find source file") for any file the top-level
`CMakeLists.txt` names; an empty `.c` fails `-Wpedantic` as warnings-as-error; and the seed `ci` workflow's
`ci-release` test step uses `"noTestsAction": "error"` with `include label ^release$`, so it errors until
`release.archive_contents` exists, which itself needs the libretro zip (plan 06). `.planning/config.json` sets
`"test_command": "cmake --workflow --preset ci"` for GSD's test gate.
**How to avoid:** plan 1 creates every source file the top-level CMakeLists names (thin but real; no
`ERR_UNSUPPORTED` stubs), registers at least one ci-labelled test, and ships `ci`/`ci-msvc` workflows without
`package`/`ci-release`; plan 07 adds those steps. Libretro joins via the adapter plan's `add_subdirectory(libretro)`.
**Warning sign:** a plan's verification falls back to `--preset dev` "temporarily".

### Pitfall 2: The D-27 grep cannot exit 1 on the tree as written
`git grep -nIiE 'cogmaster|muhtari|master system|sms|7800|palette'` over the whole tree hits `.planning/**`,
`PROVENANCE.md` (L61, 79, 90, 106–108, 199–200), `ASSET_POLICY.md` L179 and `scripts/hygiene.sh` L13 today
`[VERIFIED: git grep this session]`; provenance files must keep naming the seed. Use pathspecs:
`git grep -nIiE 'cogmaster|muhtari|master system|sms|7800|palette' -- . ':!.planning' ':!PROVENANCE.md'
':!ASSET_POLICY.md' ':!THIRD-PARTY-NOTICES.md'` (expected exit 1 after hygiene.sh L13 is removed). Also `sms` is a
substring of ordinary words ("mechanisms"); if a doc uses one, the grep exits 0 for a false reason — word-bound that
alternative or read the hits.

### Pitfall 3: RetroArch frame alignment is unproven
cogmaster's card was static, so its green compare (`--max-frames=60` vs `--frames 60`) says nothing about whether
RetroArch's `--max-frames=5` screenshot holds the core's 5th `retro_run` frame `[ASSUMED]`. D-08 is the protocol: on a
deterministic offset, read the counter strip in the uploaded screenshot, pin the runner frame with a comment, never
accept a range. Budget it in plan 10.

### Pitfall 4: `be32` / `wasm32` costs and timeout scales
The seed measured scale 8 (qemu-ppc, 6.7× total) and 5 (wasmtime, 4.3×) on its 43/40 shared tests; ENGINEERING says
11 and 3. The PS1 card is 76,800 pixels per frame (vs 49,152) and several tests run 60–360 frames. Keep the seed's
scales, then re-measure in plan 10 (DECISIONS open question "be32 and wasm32 per-PR cost").

### Pitfall 5: `schway_video.pixels` vs D-07's pitch test
D-15 makes `pixels` an output pointer into core state, valid until the next run, load or reset; D-09 scans out from
the 1024 × 512 VRAM. Read together, `get_video` returns `pixels = &vram[0][0]`, `pitch_bytes = 2048`, and D-07's test
copies the rectangle into a 640-byte-pitch buffer and a 2048-byte-pitch buffer with sentinel padding, hashing both and
checking the sentinels. `[ASSUMED]` reading — see Open Question 1.

### Pitfall 6: Missing negative controls
FRAME-05/SC5 require each `abi.*` gate to fail its negative control. The seed has `abi.undefined.self_test` and
`abi.globals.self_test` but **no `abi.symbols.self_test`** (seed `tests/inventory/ci.txt` lists only those two
`abi.*.self_test`). Add a fixture exporting a non-`schway_` external name. Likewise `policy.gate_budget.self_test`,
`hygiene.no_bios_bytes.self_test`, `build.revision_matches_head.self_test` and `core.no_host_clock.self_test` are new.

### Pitfall 7: MSVC and ARM64 symbol shapes
String literals in the core become external `??_C@_` COMDATs on MSVC ARM64 and x64; the seed's `generated` table
(abi.cmake) already exempts them, so port it unchanged (XFER-088; cogmaster 01-11 deviation, hosted run
39334762778).

### Pitfall 8: Version macro and `version.txt`
The library version reaches the header only through the generated `schwaystation_version.h` (seed `version.h.in`);
`release.single_version_source` is deferred (D-25), so nothing enforces it in phase 1 — do not write `0.0.0` anywhere
else by hand.

### Pitfall 9: Release-please first release
The seed's `.release-please-manifest.json` already says `"0.1.0"`. Copying it would make release-please compute
0.1.1/0.2.0 instead of the `initial-version` 0.1.0 (ENGINEERING §5: manifest `"0.0.0"` counts as no release).

### Pitfall 10: The tree scan sees every new file
`hygiene.tree` scans every tracked file for home paths and e-mail addresses. Comments, fixtures and docs written by
plans must not carry an absolute home path (build hygiene fixtures in pieces, as the seed does at
`tests/hygiene/tests.cmake` L19–20).

## Code Examples

### Candidate pinned hashes (independent cross-check)
Computed this session from D-03, D-04 and ARCHITECTURE §7 with the tuple order (field, interlaced, vres=240, hres=320,
dot_div=8, region=2, x1=0x260, y1=0x10, start_x=0, start_y=0, enabled=1, rgb24=0, bpp=16), frame counter k for the
k-th frame:

| Frame | SHA-256 |
|---|---|
| 5 | `c3409c280e768d81ce09c5de0eb2d7e09e8d589bf740672b560b174ff583946c` |
| 60 | `be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71` |

`[ASSUMED]` that the implemented tuple order and region value match; the executor recomputes with its own
independent one-off (record command + exit status, commit only the value) and treats a mismatch as a reading to
reconcile, not a value to copy.

```python
# independent card model used for the table above (one-off, not committed; run with python3 -I)
def px(x, y, f):
    if x < 8 and y < 8: return 0x7FFF
    if x == 0 or x == 319 or y == 0 or y == 239: return 0x7FFF
    if 1 <= x <= 8: return (y & 31) | ((y >> 5) << 5)
    L = x // 10
    if y <= 16: return 0x001F
    if y <= 48: return L
    if y <= 80: return L << 5
    if y <= 112: return L << 10
    if y <= 144: return L | L << 5 | L << 10
    if y <= 176: return (L | L << 5 | L << 10) | 0x8000
    if y <= 192: return 0x7FFF if ((x ^ y) & 1) else 0
    if y <= 208:
        return (0x7FFF if (f >> (31 - (x - 32) // 8)) & 1 else 0) if 32 <= x < 288 else 0x4210
    return 0x03E0
```

### Converter known answers (D-05/D-06), verified
`(c << 3) | (c >> 2)`: 0→0, 1→8, 15→123, 16→132, 31→255 `[VERIFIED: python3 -I this session]`.

### Seed test registration shape (verbatim, `tests/policy/tests.cmake` L5–7)
```cmake
cogmaster_add_test(NAME policy.no_skip
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${policy_scripts}/no_skip.cmake
  LABELS policy TIMEOUT 10)
```

## State of the Art

| Old (seed or prep doc) | Current (this phase) | Impact |
|---|---|---|
| cogmaster pin `abd950941e93` | `f09ac97096ac` (v0.1.0) | PROVENANCE §2/§4 update in plan 1 |
| Single `tests/inventory/ci.txt` with every name | directory selections + per-directory inventories | removes the XFER-083 hotspot |
| Static card; `runner.hash.repeat` asserts frame 1 = frame 60 | animated card; frame 5 ≠ frame 60 | invert that test |
| RetroArch-pinned `libretro.h` (`bd3398d2...`) | libretro-common `a3194c18ba65` (`036ddffc...`) | new notice row and §8 SHA |
| ENGINEERING §14 wasi-sdk 34 | distro clang-21 + wasi-libc (seed, green) | note the deviation in docs |
| ARCHITECTURE §11 P1 row (first fault, `hello.exe`, `--probe`) | REQUIREMENTS: none of these in P1 | CONTEXT domain says REQUIREMENTS wins |
| CONFORMANCE L228 "P1: hello.exe source; armips pins" | armips moved to phase 2 (GSD-HANDOFF fresh-eyes review) | no armips work |

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | RetroArch `--max-frames=5` screenshots the 5th `retro_run` frame | Pitfall 3 | compare fails by one frame; D-08 handles it in plan 10 |
| A2 | `get_video` returns a pointer into VRAM with pitch 2048 | Pitfall 5 | header shape differs; one-way after release (D-11) |
| A3 | Candidate hashes assume tuple order/region value 2 | Code Examples | pin differs; recompute |
| A4 | `revision.cmake`, `no_bios_bytes`, `no_host_clock` designs | Patterns 6–8 | a gate shape the planner may change; budget lines move |
| A5 | Trimming ≈ 200–230 comment lines keeps gate code ≤ 1,450 | Gate budget | `policy.gate_budget` fails; more trimming or fewer ported gates |
| A6 | Region core option via `SET_CORE_OPTIONS_V2` with `SET_VARIABLES` fallback | libretro facts | a few adapter lines differ |

## Open Questions (RESOLVED)

1. **`schway_video` ownership (pointer out vs caller buffer).** D-15 says `pixels` is the only pointer into core
   state; D-07 says "get_video at a tight pitch (640 bytes) and a padded pitch (2048) hash equal with padding
   untouched", which also reads like a caller-buffer API. Recommendation: pointer out (Pitfall 5); plan 1 writes the
   header once, so settle it in plan 1's task text.
   **RESOLVED:** pointer out into the instance VRAM with `pitch_bytes` 2048, valid until the next run, load or reset;
   the 640/2048 case copies through both pitches in the test (plans 01-01 and 01-03).
2. **FRAME-02 "eight hashes.txt" vs the report-only `wasm32`.** Recommendation: keep seven in `hash-equality`
   (`-DCOUNT=7`) plus the wasm32 job's printed comparison in phase 1, and record that FRAME-02's eighth file is
   compared report-only until the `ci:` tail (DEC.99).
   **RESOLVED:** seven gating lanes in `hash-equality` plus the `wasm32` hash compared report-only until the `ci:` tail.
3. **PROVENANCE §8 says "`libretro.vendored` checks each SHA-256"**, but D-25 defers `vendored`. Recommendation: plan
   1 records the SHA and rewords §8 to say the check lands with its seed todo; do not port the gate.
   **RESOLVED:** PROVENANCE §8 is amended (SHA recorded, the check named as a seed todo); the gate is not ported (plan 01-02).
4. **`hygiene.format` and `hygiene.analyzer`** are in the seed's hygiene inventory and in ENGINEERING §2 but not named
   by FRAME-01..09, so D-24 excludes them. They cost no budgeted lines (inline in `tests/hygiene/tests.cmake`) but need
   clang-format/clang-tidy 21 at configure. Recommendation: follow D-24 (drop, one seed todo each) unless the planner
   reads `.clang-format`'s existence as needing a check; ask nothing of the owner.
   **RESOLVED:** both dropped, one seed todo each (written in plan 01-15).
5. **Presets list.** D-29 lists `dev, ci, ci-msvc, asan, nofp, be32, wasm32, perf` but the `hygiene` job needs a
   `hygiene` preset; recommendation: add it in plan 1 (it is the seed's) and rename the seed's `bench` to `perf`.
   **RESOLVED:** plan 01-01 adds the `hygiene` preset and renames `bench` to `perf`.
6. **`policy.publish_set/_ref/_signer` and `ci.classify`** are not FRAME-05-named but test the scripts the FRAME-06
   rehearsal runs and the `changes` job that gates it. Recommendation: keep them as FRAME-06 coverage (inline tests,
   no budgeted lines).
   **RESOLVED:** `policy.publish_set/_ref/_signer` and `ci.classify` are kept as FRAME-06 coverage (plan 01-06).

## Environment Availability

| Dependency | Required by | Available locally | Version | Fallback |
|---|---|---|---|---|
| CMake | everything | yes | 4.4.3 | — |
| Ninja | presets | yes | 1.13.2 | — |
| Apple clang | dev/ci on macOS | yes | 21.0.0 (clang-2100.1.1.101) | — |
| Homebrew `llvm@21` (clang-format 21.1.8) | hygiene preset locally | yes | 21.1.8 | CI hygiene job |
| jq, git, gh, dash, sips, hdiutil | hygiene, scripts, e2e | yes | jq 1.7.1, git 2.41.0, gh 2.101.0 | — |
| GCC 15 (nofp) | `nofp` | no | — | CI only (ubuntu legs) |
| valgrind | `perf` | no | — | CI only |
| qemu-ppc, powerpc cross GCC | `be32` | no | — | CI only |
| wasmtime | `wasm32` | no | — | `scripts/fetch-pin.sh wasmtime-aarch64-macos build/pins/wasmtime` (pinned row exists in the seed) or CI |
| RetroArch 1.22.2 | e2e | installed app present, but RetroArch aborts inside the agent sandbox (HOSTS §3) | — | hosted macos-15 job is the evidence |
| Release App credentials | rehearsal, release | repo variable and secret exist | — | — |

**Missing with no fallback:** none (every missing tool has a CI lane). **With fallback:** nofp, perf, be32, wasm32,
RetroArch → hosted CI (plan 10 evidence).

## Validation Architecture

`workflow.nyquist_validation` is `false` in `.planning/config.json`; this section is included because the caller asked
for the requirement → test map.

### Test framework
| Property | Value |
|---|---|
| Framework | CTest (CMake 4.4) with C test programs (`tests/check.h`) and CMake `-P` drivers |
| Config | `CMakePresets.json`, `cmake/SchwaystationTests.cmake`, `tests/inventory/*.txt`, `tests/<dir>/inventory.txt` |
| Quick run | `cmake --workflow --preset dev` |
| Full suite | `cmake --workflow --preset ci` (and `--preset hygiene`); hosted lanes via the PR run |

### Requirement → test map
| Req | Behaviour | Type | Command / test | Exists? |
|---|---|---|---|---|
| FRAME-01 | exact inventory, once each, none skipped, packaged | integration | configure-time DEFER check; `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` and the `-DLABELS=release` pass; `policy.no_skip(+self_test)` | Wave 0 (plan 1) |
| FRAME-02 | pinned card hash, identical on lanes | unit + CI | `runner.hash` (PASS_REGULAR_EXPRESSION on the pin), `runner.hash.self_test`, `runner.hashes_file`, `runner.hashes_compare.{byte,count}.self_test`, core `frame 5 ≠ frame 60`; CI `hash-equality`, `be32`, `wasm32` report | plans 01, 03, 08 |
| FRAME-03 | in-process frame equals runner conversion; av_info rationals | integration | `libretro.host` (+ PAL option case) | plan 06 |
| FRAME-04 | nm gates, header C17/C++17, no float, budgets | unit | `abi.symbols/.globals/.undefined(+self_test each)`, `abi.header.c17`, `abi.header.cxx17`, `abi.nofp_fixture` (nofp lane), `hygiene.no_float(+self_test)`, `docs.budgets` | plans 04, 05 |
| FRAME-05 | repository gates with negative controls; gate budget; revision | unit | `hygiene.tree(+self_test)`, `ci.title(+self_test)`, `hygiene.timeout_comments(+self_test)`, `policy.action_pins(+self_test)`, `hygiene.no_bios_bytes(+self_test)`, `policy.no_skip(+self_test)`, `policy.gate_budget(+self_test)`, `build.revision_matches_head(+self_test)` | plans 01, 03, 05, 08 |
| FRAME-06 | archives, notices, rehearsal, release config | integration + CI | `release.archive_contents(+self_test)`, `policy.publish_set(+self_test)`, `policy.publish_ref`, `policy.publish_signer`, `ci.classify`; CI `rehearsal` + `cleanup` run URL | plans 05, 07, 09, 10 |
| FRAME-07 | install line, RetroArch screenshot, negative control, e2e budget | CI + unit | build-leg step `install_line.cmake` (+ `-DFLIP=1`), CI `retroarch` job (compare + `flip.cmake`), `runner.png`, `runner.png_file`, `policy.e2e_budget(+self_test)` | plans 03, 05, 07, 08, 10 |
| FRAME-08 | consumers and embed; v1.c | integration | `install.stage`, `install.consumer`, `install.consumer.cxx`, `install.consumer.dll_crt`, `embed.subdirectory`, `abi.api_v1`, `abi.api_version(+self_test)` | plans 04, 07 |
| FRAME-09 | bench percentiles, Cachegrind job, no alloc, no clock, visitor round trip | unit + CI | `runner.bench.{percentile,format,bytes}`, `perf.gate.*` (7), CI `perf` (report), `core.no_alloc_after_load`, `core.no_host_clock(+self_test)`, core state round-trip cases (D-07) | plans 02, 03, 04, 05, 08 |

### Sampling rate
- Per task commit: `cmake --workflow --preset dev` (fast) and the plan's own tests.
- Per plan: `cmake --workflow --preset ci` (the configured GSD test command) and, for gate plans, `--preset hygiene`.
- Phase gate: the hosted PR run green (`CI required`), its URL recorded, before `/gsd-verify-work`.

### Wave 0 gaps
- [ ] `cmake/SchwaystationTests.cmake` with directory union; `tests/cmake/{expect_fail,check_junit}.cmake`; `tests/check.h` — plan 1
- [ ] every `tests/<dir>/{tests.cmake,inventory.txt}` and `tests/inventory/*.txt` — plan 1
- [ ] at least one ci-labelled test so `noTestsAction: error` passes — plan 1

## Security Domain

`security_enforcement` is `false` in `.planning/config.json`; section omitted. (Untrusted-input surfaces in phase 1 are
only `schway_load_state` bytes, covered by D-17's validate-then-apply and the corrupted-state test.)

## Sources

### Primary (HIGH)
- cogmaster `f09ac97096acda99888270e4c1e4dc96e12ffdfe` via `gh api` contents/trees: every file named in the seed
  table; runs 39346572559 (push CI), 39346572721 (release), 39345717969 (PR with rehearsal); release `v0.1.0`, 19
  assets; phase-1 PLAN/SUMMARY files (durations, plan 11 MSVC deviation)
- muhtari `d698364b180df42894ef6737179d4ed9fcc97817`: `CMakeLists.txt` L30–100, `tests/alloc.h`
- libretro-common `a3194c18ba65ef89734e1898a2f61403c255d368`: `include/libretro.h` (constants, structs, SHA-256)
- This repository: CONTEXT, REQUIREMENTS, ROADMAP, STATE, config.json, LESSONS, preparation ENGINEERING,
  ARCHITECTURE, HOSTS-AND-INTEGRATION, DECISIONS, GSD-HANDOFF, CONFORMANCE (headings), PROVENANCE, `scripts/*`
- Local computations (`python3 -I`): rationals, accumulator ranges, width formula, converter values, candidate hashes

### Secondary (MEDIUM)
- Seed design notes in YAML comments (timeouts, perf base logic) — hosted-proven but not re-run here

### Tertiary (LOW)
- RetroArch `--max-frames` frame alignment (not re-verified; RetroArch source not opened)

## Metadata

**Confidence breakdown:**
- Standard stack and seed inventory: HIGH — fetched at the pin, counted, and the seed's runs are green
- Architecture and plan sequencing: MEDIUM — derived from CMake behaviour and D-29; Pitfall 1 is the main risk
- Pitfalls: HIGH for 1, 2, 6, 7, 9 (observed); MEDIUM for 3, 4, 5

**Research date:** 2026-10-19
**Valid until:** 2026-11-18 (runner images and action pins move; `ubuntu-latest` changes in November 2026)
