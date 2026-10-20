# Phase 1: A test frame in RetroArch - Context

**Gathered:** 2026-10-19
**Status:** Ready for planning

<domain>
## Phase Boundary

The content-free walking skeleton. Merging the phase pull request builds, tests, attests and publishes the library,
`schwaystation-run` and `schwaystation_libretro` on six platforms; the README install line puts the core into
RetroArch 1.22.2 on macOS arm64, where it shows a built-in test card that hashes identically on eight lanes (six
platforms, `be32`, `wasm32`) with no BIOS and no disc. Requirements FRAME-01 to FRAME-09; no CPU, no BIOS loading,
no EXE loading, no `--probe`, no first-fault trace (those are phase 2: CPU-04, CPU-05; REQUIREMENTS wins over the
ARCHITECTURE §11 P1 row, which still lists them).

</domain>

<decisions>
## Implementation Decisions

### Test card (FRAME-02, 03, 07, 09)
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

### Phase-1 public API (FRAME-03, 04, 08, 09)
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

### Seed porting (FRAME-01, 05, 06, 07)
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

### Test inventory reading (FRAME-01 vs AGENTS rule 14)
- **D-28:** `tests/inventory/ci.txt` keeps its name and holds only the preset's directory and label selection,
  written once in plan 1. Each directory's test names live in `tests/<dir>/inventory.txt`. The exact-inventory check
  compares the registered tests (JUnit) with the union of the selected directories' inventories, executed once
  each, none skipped. This keeps FRAME-01's file and removes the single-file merge hotspot (XFER-083).

### Plan shape (AGENTS rule 14)
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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project rules and scope
- `AGENTS.md` — rules 1 to 14 (reuse, clean tree, determinism, gates, one step per session, branch and PR, pins).
- `.planning/REQUIREMENTS.md` — FRAME-01 to FRAME-09 (the phase's acceptance) and CPU-04/CPU-05 (what stays in P2).
- `.planning/ROADMAP.md` — Phase 1 goal and success criteria.
- `.planning/LESSONS.md` — XFER-005, 013, 044, 057, 083, 088, 091, 094, 101, 104, 105.
- `.planning/preparation/DECISIONS.md` — DEC.30, 70, 75, 78, 85, 99, 103, 106; open questions row "`be32` and
  `wasm32` per-PR cost" (measured in P1).

### Design
- `.planning/preparation/ARCHITECTURE.md` — §2 module table, §3 API table, C sketch and size-tag rules, §4 I/O
  contracts, the frame-hash definition, §11 build order (its P1 row is superseded on scope by REQUIREMENTS).
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` — §1 libretro adapter, §3 bounded hosted e2e, runner flag table.
- `.planning/preparation/ENGINEERING.md` — CI lanes, presets, release and rehearsal, gates, perf (Ir/frame) and bench.
- `.planning/preparation/CONFORMANCE.md` — fixture and oracle conventions.
- `PROVENANCE.md` — §2/§4 seed pins (update to cogmaster `f09ac97096ac`), §5a same-owner seed rule, §8 vendored
  files; `ASSET_POLICY.md`.

### Seeds (public pins, fetch with `gh api` only)
- szTheory/cogmaster `f09ac97096acda99888270e4c1e4dc96e12ffdfe` (v0.1.0) — the walking skeleton.
- szTheory/muhtari `d698364b180df42894ef6737179d4ed9fcc97817` (v0.2.0) — `CMakeLists.txt` L68–82 OBJECT pattern;
  `tests/alloc.h`, `src/internal.h` layout-pin pattern for reference.
- libretro/libretro-common `a3194c18ba65` — `include/libretro.h`.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scripts/hygiene.sh` (216 lines): already PS1-adapted (PS-X EXE and CD001 signatures compared as hex, 512 KiB BIOS
  and 128 KiB card sizes); backs `hygiene.no_bios_bytes` and `--tree`. Trim comments.
- `scripts/fetch-pin.sh`, `check-title.sh`, `classify-changes.sh`, `.githooks/pre-commit`, `pre-push`: identical to
  the seed; keep. `scripts/apply-repo-settings.sh`: keep.
- `scripts/publish.sh`: stale against the seed; re-sync. `scripts/setup-release-app.sh`: remove.
- `version.txt`, `.github/rulesets/main.json`, `.clang-format`, `.gitattributes`, `.gitignore`, `.env.example`.

### Established Patterns
- No `src/`, `include/`, `tests/` or CMake yet; the phase creates them from the seed.
- Existing gate code: 812 lines against the 1,500 budget.

### Integration Points
- Release App and repository settings exist (kickoff); release-please `simple` with `version.txt`.

</code_context>

<specifics>
## Specific Ideas

- The owner's fan-out brief governs advisor research: every stakeholder lens, primary sources, an adversarial pass,
  one coherent recommendation per point, followed without re-asking.
- "Another copy and paste beats another dependency": no new packages; seeds are copied and renamed.

</specifics>

<deferred>
## Deferred Ideas

- The gates listed in D-25 become one todo each, bound to the next phase that adds their first caller.
- Arming the Ir/frame gate, promoting `wasm32` from report-only and the rehearsal filter belong to the phase-1 `ci:`
  tail step on `phase/01-ci-tail` (DEC.99), not to this phase's plans.

</deferred>

---

*Phase: 01-a-test-frame-in-retroarch*
*Context gathered: 2026-10-19*
