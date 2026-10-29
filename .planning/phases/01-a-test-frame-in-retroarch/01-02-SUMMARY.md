---
phase: 01-a-test-frame-in-retroarch
plan: 02
subsystem: provenance, docs, release scripts
tags: [libretro, provenance, notices, readme, publish]

requires: []
provides:
  - "libretro/libretro.h vendored unmodified from libretro-common a3194c18ba65ef89734e1898a2f61403c255d368"
  - "THIRD-PARTY-NOTICES.md with the libretro.h section (commit, SHA-256, MIT text)"
  - "PROVENANCE pinned to cogmaster v0.1.0 f09ac97096ac; section 8 digest; section 5(c) names release.archive_contents"
  - "User-facing README.md with an install placeholder heading for plan 01-11"
  - "scripts/publish.sh equal to the seed after the one rename (SIGNER_WORKFLOW rule)"
affects: [01-06, 01-07, 01-10, 01-11, 01-13]

actuals:
  tokens: 93947
  tasks: 3
  commits: 3
plan_head_before: b235ad682eb50f24b48afc0e66f888f755a8a1d2
plan_head_after: 61a5bb7aed25a529d39df4ccd3a299b928f01889

tech-stack:
  added: []
  patterns:
    - "Vendored files fetched with gh api raw at a 40-hex pin, digest recorded in PROVENANCE section 8 and THIRD-PARTY-NOTICES.md"

key-files:
  created:
    - libretro/libretro.h
    - THIRD-PARTY-NOTICES.md
    - README.md
  modified:
    - PROVENANCE.md
    - scripts/publish.sh
    - scripts/apply-repo-settings.sh
  deleted:
    - scripts/setup-release-app.sh

key-decisions:
  - "PROVENANCE PV.11 keeps abd950941e93: it cites cogmaster's PROVENANCE PV.09 as a historical source, not a seed pin; section 3 names the old pin in the bump sentence"
  - "README names docs/runner.md, embedding.md, testing.md and maintaining.md, which later phase-1 plans write; the install heading is a placeholder for plan 01-11"

requirements-completed: [FRAME-03, FRAME-05, FRAME-06]

coverage:
  - id: D1
    description: "libretro.h vendored unmodified with its digest in PROVENANCE section 8 and THIRD-PARTY-NOTICES.md"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "shasum -a 256 libretro/libretro.h equals 036ddffc995ad116c3081ca0a7515d09c2a116f3483e4f5012fe0c29a885da76, plus the Task 1 grep chain and sh scripts/hygiene.sh --tree, exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "PROVENANCE pins cogmaster v0.1.0 at a public commit and states the deferred notices and vendored checks"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "gh api repos/szTheory/cogmaster/commits/f09ac97096acda99888270e4c1e4dc96e12ffdfe exit 0; grep -c f09ac97096ac PROVENANCE.md prints 5; grep -c abd950941e93 PROVENANCE.md prints 2"
        status: pass
    human_judgment: false
  - id: D3
    description: "README is user-facing, short and free of planning IDs"
    requirement: FRAME-03
    verification:
      - kind: other
        ref: "test $(wc -c < README.md) -lt 16384 exit 0 (2418 bytes); grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' README.md prints 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "One-off App script removed and publish.sh re-synced to the seed, lowering the gate-line total"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "seed publish.sh through sed cogmaster to schwaystation, diff against scripts/publish.sh, exit 0; sh -n and dash -n exit 0; git grep setup-release-app outside .planning status 1"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 02: Vendored libretro.h, v0.1.0 provenance pins, README and publish re-sync Summary

**`libretro/libretro.h` vendored byte-identical from libretro-common `a3194c18` (SHA-256 `036ddffc...da76`) with its MIT notice, PROVENANCE moved to the public cogmaster v0.1.0 pin, a 2.4 KB user README, and `scripts/publish.sh` re-synced to the seed's rehearsal-signer logic with the one-off App script removed.**

## Performance

- **Duration:** 3 min (wall time)
- **Started:** 2026-10-20T03:02:31Z
- **Completed:** 2026-10-20T03:05:51Z
- **Tasks:** 3
- **Files modified:** 7 (3 created, 3 modified, 1 deleted)

## Accomplishments

- `libretro/libretro.h` fetched with `gh api` raw at the 40-hex pin, unchanged; `THIRD-PARTY-NOTICES.md` holds its section.
- PROVENANCE: section 2, 4 and 10 rows pin `f09ac97096ac`; section 4 lists the renames; section 3 records the bump and the 200 answer; section 5(c) names `release.archive_contents` with `release.notices` and `policy.notices` deferred; section 8 carries the commit and digest, and says `libretro.vendored` is a deferred todo.
- README.md: status, three deliverables, install placeholder, runner quick start with the pinned frame-60 hash, BIOS and disc rule, docs ladder, licence.
- `scripts/setup-release-app.sh` removed; `scripts/publish.sh` is 182 lines, equal to the seed after the rename.

## Task Commits

1. **Task 1: Tracer - vendored libretro.h to recorded digest to notice** - `8bacc28` (feat)
2. **Task 2: PROVENANCE pins to cogmaster v0.1.0, section 5(c) wording, and the short README** - `2738074` (docs)
3. **Task 3: Remove the one-off App script and re-sync the publish script** - `61a5bb7` (chore)

## Evidence (command, exit status; run from the repository root)

- `gh api -H 'Accept: application/vnd.github.raw' 'repos/libretro/libretro-common/contents/include/libretro.h?ref=a3194c18ba65ef89734e1898a2f61403c255d368' > libretro/libretro.h`: exit 0.
- `shasum -a 256 libretro/libretro.h`: exit 0, prints `036ddffc995ad116c3081ca0a7515d09c2a116f3483e4f5012fe0c29a885da76`.
- Task 1 verify chain (digest, PROVENANCE and notices greps, deferred regex, no `checks each SHA-256`, `sh scripts/hygiene.sh --tree`): exit 0; re-checked for the tracer gate: exit 0.
- `gh api repos/szTheory/cogmaster/commits/f09ac97096acda99888270e4c1e4dc96e12ffdfe`: exit 0.
- `grep -c abd950941e93 PROVENANCE.md`: 2 (baseline 4). The two left are the section 3 bump sentence and PV.11, a historical citation of cogmaster's own PROVENANCE.
- Task 2 verify (`grep -c f09ac97096ac PROVENANCE.md && test "$(wc -c < README.md)" -lt 16384 && sh scripts/hygiene.sh --tree`): exit 0.
- Task 3 verify (`test ! -e scripts/setup-release-app.sh && sh -n scripts/publish.sh && dash -n scripts/publish.sh && grep -q SIGNER_WORKFLOW scripts/publish.sh && git grep --untracked -n setup-release-app -- . ':!.planning'; s=$?; test "$s" -eq 1`): exit 0.
- Seed diff (`gh api ... scripts/publish.sh?ref=f09ac97096acda99888270e4c1e4dc96e12ffdfe | sed 's/cogmaster/schwaystation/g' | diff - scripts/publish.sh`): exit 0. `wc -l < scripts/publish.sh`: 182. `grep -c 'schwaystation-\$c-\$o-\$a.zip' scripts/publish.sh`: 1.
- Planning-ID scan over `scripts/publish.sh scripts/apply-repo-settings.sh`, `test $? -eq 1`: exit 0.
- Cruft grep (same command as plan 01-01's SUMMARY): a=1, b=1, exit 0.
- Gate lines: `wc -l scripts/*.sh .githooks/* | tail -1`: 702 (baseline at plan start 811, after 01-01's one-line hygiene trim). `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`: 815.
- `sh scripts/hygiene.sh --tree`: exit 0.
- `cmake --workflow --preset ci`: exit 0.

## Decisions Made

See `key-decisions` above. Neither reopens a settled choice.

## Deviations from Plan

None - plan executed exactly as written. (The plan's scripts-and-hooks baseline of 812 was 811 at start because plan 01-01 had already removed one hygiene.sh line; the result, 702, matches the plan's "about 703".)

## Issues Encountered

None.

## Known Stubs

- `README.md` "Install in RetroArch" section is a one-sentence placeholder by design; plan 01-11 fills it with the install line.
- The README's docs ladder links `docs/runner.md`, `docs/embedding.md`, `docs/testing.md` and `docs/maintaining.md`, which later phase-1 plans write.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 01-10 can include `libretro/libretro.h`; plan 01-11 fills the README install heading; plan 01-13 writes `docs/maintaining.md`, which the apply-repo-settings comment now cites.

---
*Phase: 01-a-test-frame-in-retroarch*
*Completed: 2026-10-20*

## Self-Check: PASSED
