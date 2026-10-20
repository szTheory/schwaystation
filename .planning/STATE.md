---
gsd_state_version: "1.0"
milestone: v1.0
milestone_name: PlayStation games play with sound in RetroArch
current_phase: 01
current_phase_name: A test frame in RetroArch
status: executing
stopped_at: Completed 01-14-PLAN.md
last_updated: "2026-10-20T16:32:17.765Z"
last_activity: 2026-10-19
last_activity_desc: Phase 01 execution started
state_head: e669f21032f7b46db6e41295e8ffca7a3caa2054
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 15
  completed_plans: 14
  percent: 0
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-19)

**Core value:** PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.
**Current focus:** Phase 01 — A test frame in RetroArch

## Current Position

Phase: 01 (A test frame in RetroArch) — EXECUTING
Plan: 15 of 15
Status: Ready to execute
Last activity: 2026-10-19 — Phase 01 execution started

Progress: [░░░░░░░░░░] 0%

## Performance Metrics

**Velocity:**
- Total plans completed: 0
- Average duration: -
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

**Recent Trend:**
- Last 5 plans: -
- Trend: -

*Updated after each plan completion*
**Per-Plan Metrics:**

| Plan | Duration | Tasks | Files |
|------|----------|-------|-------|
| Phase 01 P01 | 13 min | 3 tasks | 55 files |
| Phase 01 P02 | 3min | 3 tasks | 7 files |
| Phase 01 P03 | 9 min | 3 tasks | 8 files |
| Phase 01 P04 | 6min | 3 tasks | 11 files |
| Phase 01 P05 | 8min | 3 tasks | 14 files |
| Phase 01 P06 | 8 min | 3 tasks | 10 files |
| Phase 01 P07 | 6min | 3 tasks | 9 files |
| Phase 01 P08 | 6 min | 2 tasks | 6 files |
| Phase 01 P09 | 11h30m wall (checkpoint interruption) | 3 tasks | 16 files |
| Phase 01 P10 | 12min | 2 tasks | 7 files |
| Phase 01 P11 | 4min | 2 tasks | 6 files |
| Phase 01 P12 | 6min | 3 tasks | 11 files |
| Phase 01 P13 | 5min | 3 tasks | 6 files |
| Phase 01 P14 | 17min | 3 tasks | 0 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table (full list: `.planning/preparation/DECISIONS.md`).
Recent decisions affecting current work:

- [Roadmap]: Six phases, one per requirement category, kept despite the `coarse` granularity because each category is one branch, one pull request and one release.
- [Roadmap]: Phase 1 is the content-free walking skeleton ported from cogmaster and muhtari; every later phase ends in an automated release.
- [Phase 01]: schway_video is 80 + 2P bytes with trailing reserved1; schway_audio 16 + 2P; schway_config has reserved0 so the allocator sits at offset 8 on every target
- [Phase 01]: State header is 20 bytes (magic, format, total size, body CRC-32); the profile lock is set by the first run_frame and survives reset and load
- [Phase 01]: PROVENANCE PV.11 keeps the old cogmaster pin as a historical citation; seed pins moved to v0.1.0 f09ac97096ac
- [Phase 01]: The state header size is SCHWAY__STATE_HEADER_SIZE in src/state.h, shared by state.c and the state tests
- [Phase 01]: Plan 01-01's core passed every 01-03 contract case unchanged; mutants of src/ are caught by the core and state cases
- [Phase 01]: hashes.txt holds five LF lines, region first (ntsc-u frame 5, ntsc-u frame 60, ntsc-j frame 60, pal frame 60, ntsc-u png 5); the hex is field 5
- [Phase 01]: The runner takes one --png N:FILE; an unwritable path exits 1 and plan 01-09 owns its test; the PNG digest is pinned only after the pixel-by-pixel decode passed
- [Phase 01]: Darwin abi.undefined allows memset_pattern16 (Apple clang lowers the test card's repeating fill to it); Windows allows __ImageBase for MSVC switch tables
- [Phase 01]: core.no_host_clock is the whole host-clock proof: the PS1 has no RTC, so no injected-RTC form
- [Phase 01]: abi.header.c17/cxx17 and abi.api_v1 are compile/run checks without self-tests; the five gates with negative controls are abi.symbols, abi.globals, abi.undefined, abi.api_version and core.no_host_clock
- [Phase 01]: line_budget.cmake counts newline bytes only, as wc -l does, so its total equals the wc -l the gate ceilings use
- [Phase 01]: policy.publish_set.self_test fails both a 17-archive and a 19-archive set, covering both sides of exactly 18
- [Phase 01]: ci.* hygiene tests carry label hygiene only; policy.* tests carry hygiene and policy
- [Phase 01]: 01-07: perf_compare.cmake reads its mode only from SCHWAYSTATION_PERF_MODE; an unset mode is an input error
- [Phase 01]: 01-07: budgets.cmake resolves ROOT with file(REAL_PATH) so a relative ROOT still globs docs/*.md
- [Phase 01]: embed.subdirectory accepts exactly the library and its three object libraries, and fails on any install rule in the embedded tree
- [Phase 01]: 01-09: --bench shares the --frames limit 1..1000000; the revision is read by git rev-parse at configure into a runner-only generated header and build.revision_matches_head compares it with HEAD
- [Phase 01]: libretro adapter recreates the instance on a region option change (profile locks after the first frame) and sends SET_SYSTEM_AV_INFO only on a changed update
- [Phase 01]: libretro .info claims core_options true (tested in-process); savestate stays false although retro_serialize is wired and tested
- [Phase 01]: 01-11: the README install line keeps the seed form (curl zip and SHA256SUMS, shasum -c --ignore-missing, tar -xf cores info), so a damaged download installs nothing
- [Phase 01]: 01-11: release.floor is not ported; only the two archive-contents tests carry the release label
- [Phase 01]: 01-12: the tests/retroarch files carry no PS1 value; FRAMES=5, --frames 5 --png 5:runner.png and the 320x240 regex live in ci.yml, so the e2e code stays 293 lines
- [Phase 01]: 01-12: every ci.yml timeout keeps the seed bound marked provisional with no run ID until plan 01-15 measures them
- [Phase 01]: 01-12: CI required omits the rehearsal and cleanup clauses until plan 01-13 adds those jobs
- [Phase 01]: 01-12: tests/cmake/workflows.cmake MODE=timeouts|pins replaces the seed's two workflow scans
- [Phase 01]: 01-13: the one-off actionlint command also ignores the concurrency queue key, which actionlint 1.7.12 does not know; the seed release.yml ran with queue: max
- [Phase 01]: 01-13: the rehearsal not-configured message points to docs/maintaining.md, which cites the removed App script as git show 61a5bb7^:scripts/setup-release-app.sh
- [Phase 01]: 01-13: docs/testing.md CI jobs table does not list rehearsal and cleanup yet; the next plan that edits docs/testing.md adds the row
- [Phase 01]: 01-14: the first hosted CI run (39455724329) was green on every job; no hosted fix, no RetroArch frame pin
- [Phase 01]: 01-14: perf on the probe reports NO_BASE (Ir/frame 1620415) because main has no perf preset

### Pending Todos

None yet.

### Blockers/Concerns

- [Phase 4]: Research flag: GPU blend width, size rejection, stepper precision, GPU clocks per line, and the adapter's stub-kernel path for PS-EXE content in RetroArch.
- [Phase 6]: Research flag: the CD seek model and INT1 period, and whether OpenBIOS boots the own mkpsxiso disc without licence data.

## Deferred Items

Items acknowledged and deferred at milestone close, most recent first:

| Category | Item | Status | Deferred At | Milestone |
|----------|------|--------|-------------|-----------|
| *(none)* | | | | |

## Runtime Log

| Date | Step | Runtime | Model |
|------|------|---------|-------|
| 2026-10-19 | /gsd-discuss-phase 1 | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |
| 2026-10-19 | /gsd-plan-phase 1 | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |
| 2026-10-19 | /gsd-execute-phase 1 | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |

## Session Continuity

Last session: 2026-10-20T16:32:17.744Z
Stopped at: Completed 01-14-PLAN.md
Resume file: None
