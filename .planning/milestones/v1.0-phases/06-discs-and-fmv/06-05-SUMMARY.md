---
phase: 06-discs-and-fmv
plan: "05"
subsystem: cdrom
tags:
  - cdrom
  - sectors
  - subcode
  - save-state
requires:
  - phase: 06-04
    provides:
      - CD-ROM command, interrupt, and timing model
provides:
  - Scheduled host sector acquisition and firmware-visible RDDATA
  - Bounded sector filtering, Q-position tracking, and INT1 delivery
affects:
  - 06-06
  - 06-09
  - 06-10
  - 06-15
  - 06-27
actuals:
  tokens: 14774
  tasks: 1
  commits: 5
plan_head_before: ea564f4a016841b53768833b2cfce0d5828f6874
plan_head_after: 29d8ab736e2464e957d34908069fc3a8802e7096
tech-stack:
  added: []
  patterns:
    - Fixed-cycle device deadlines drive host callbacks inside public run calls
    - Fixed-capacity sector ring and one deferred INT1 preserve bounded state
key-files:
  created:
    - src/cdrom_sector.c
    - tests/cdrom/sectors.c
  modified:
    - src/cdrom.c
    - src/cdrom.h
    - src/cdrom_state.c
    - src/frame.c
    - src/internal.h
    - tests/core/test_state.c
    - docs/hardware/cdrom.md
key-decisions:
  - BFRD, ReadS selection, retry state, and deferred INT1 are serialized; state format and behavior revision are 9.
  - Valid interleaved Q is CRC checked; absent or invalid Q is synthesized from the TOC, while bad_q preserves the last good location.
  - XA audio selection precedes RDDATA; accepted XA and CD-DA audio continue through the later decoder and SPU plans.
  - DMA3's shared sector cursor and transfer-rate behavior are isolated in Plan 06-27.
patterns-established:
  - Sector callbacks are invoked only while the instance advances through a public run call.
  - ReadN retries one failed sector once; ReadS does not retry.
requirements-completed: []
coverage:
  - id: D1
    description: Scheduled sector reads, TOC-derived Q fallback, bad-Q position retention, and bounded ReadN/ReadS filtering.
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^cdrom\\.(sectors|delivery|command|timing|region|bounds)$|^core\\.state\\.(device_fields|round_trip)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: BFRD-gated 800h/924h RDDATA, deterministic overrun, CD-DA Play acceptance, and pending INT1 behavior.
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-state && ctest --test-dir build/ci -R '^cdrom\\.(sectors|delivery|command|timing|region|bounds)$|^core\\.state\\.(device_fields|round_trip)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
metrics:
  duration: "58m56s elapsed wall time to summary creation; active duration unavailable"
  completed: 2026-11-15
  status: complete
---

# Phase 6 Plan 5: Scheduled CD sector delivery Summary

**ReadN/ReadS and Play now acquire sectors on deterministic deadlines, apply bounded filtering and retry rules, and expose selected data through the firmware RDDATA window.**

## Performance

- **Duration:** 58m56s elapsed wall time from the earliest durable marker to summary creation; exact active duration is unavailable because executor start and idle intervals were not captured.
- **Started:** Earliest durable execution marker `2026-11-15T21:12:21Z` (`ea564f4` predecessor commit).
- **Completed:** `2026-11-15T22:11:17Z` at summary creation; final metadata commit followed.
- **Tasks:** 1
- **Files modified:** 13 implementation, test, and hardware-documentation files; planning closeout files are committed separately.
- **Runtime/model:** Codex / OpenGSD 1.20.0; executor `gpt-6-luna xhigh`.

## Accomplishments

- Added scheduled host sector reads at 1x and 2x; `read_sector` and `bad_q` callbacks run only while a public run call advances the instance.
- Implemented BFRD-gated 800h and 924h windows, the documented repeated tail byte, the eight-slot ring, silent oldest-sector overrun, and one deferred INT1.
- Accepted raw CD-DA sectors during Play; prevented selected XA audio sectors from entering RDDATA; added one bounded ReadN retry and no-retry ReadS behavior for failed/malformed and file/channel-filtered sectors.
- Parsed interleaved Q subcode with BCD and CRC checks, synthesized Q from TOC when absent or invalid, and retained the previous good location for host-owned bad-Q sectors.
- Serialized BFRD, ReadS and retry state; bumped state format and behavior revision to 9; documented the sector path.

## Plan Commits

- `4ea8885` — `docs(06): split sector delivery and DMA3 plans`
- `7b7e6f9` — `docs(06-05): include BFRD state coverage`
- `639d184` — `docs(06): add code review report`
- `2ad1324` — `feat(06-05): deliver scheduled CD sectors`
- `29d8ab7` — `refactor(06-05): remove plan identifier from comment`

The measured five-commit count starts after parent commit `ea564f4`, which corrected the stale CTest selectors and runtime log before this executor's ledger.

## Files Created/Modified

- `src/cdrom_sector.c` — sector callback scheduling, Q handling, filtering/retry decisions, bounded buffering, and RDDATA reads.
- `tests/cdrom/sectors.c` — callback timing, data windows, both speeds, overrun, ReadN/ReadS retry behavior, XA filtering, Q, bad-Q, and CD-DA Play cases.
- `src/cdrom.c`, `src/cdrom.h`, `src/cdrom_state.c`, `src/frame.c`, `src/internal.h` — run-call callback guard, register wiring, saved state, and revision bumps.
- `tests/cdrom/model.c`, `tests/cdrom/tests.cmake`, `tests/cdrom/inventory.txt`, `tests/core/test_state.c`, `cmake/DiscTargets.cmake`, and `docs/hardware/cdrom.md` — test registration, state coverage, build wiring, and hardware documentation.

## Decisions Made

- Host callback work remains synchronous and bounded by one sector request per scheduler event; host latency does not affect emulated ticks.
- XA eligibility and file/channel filtering run before guest data selection. XA decoding and audio output remain with the later XA/SPU plans.
- DMA3 transfer timing and its shared RDDATA cursor are a separate sequential plan, 06-27, in Wave 7.

## Deviations from Plan

### Plan split

The original Plan 05 combined sector acquisition with DMA3 transfer and exceeded its 30-minute forecast. The work was split without removing scope: Plan 05 owns sector acquisition and firmware delivery; new Plan 06-27 owns DMA3's shared cursor and 24/40-cycle behavior. The roadmap plan count and Wave 7 index were updated, and 06-15 depends on 06-27 because disc boot uses DMA3.

### Review corrections

The first code review found three gaps in the plan's sector contract: raw CD-DA Play sectors were rejected, XA/filter/retry ordering was missing, and supplied or absent Q was not processed. These were fixed and covered before the clean second review in [06-REVIEW.md](./06-REVIEW.md).

## Verification Evidence

- RED command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom && ctest --test-dir build/ci -R '^cdrom\.sectors$' --no-tests=error --output-on-failure --output-junit build/ci/06-05-sector-red.xml` — exit 8; the named target ran and failed the missing callback and RDDATA assertions. `gsd-tools check tdd-red-evidence build/ci/06-05-sector-red-record.json --raw` returned `RED_EVIDENCE_OK`. The JUnit report is at `build/ci/build/ci/06-05-sector-red.xml` and remains ignored build output.
- Final plan gate: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-state && ctest --test-dir build/ci -R '^cdrom\.(sectors|delivery|command|timing|region|bounds)$|^core\.state\.(device_fields|round_trip)$' --no-tests=error --output-on-failure` — exit 0; all 8 selected tests passed. Fresh `LastTest.log` timestamp: `2026-11-15T22:02:23Z`.
- Hygiene and source checks: `sh scripts/hygiene.sh && LC_ALL=C sort -c tests/cdrom/inventory.txt && git diff --check` — exit 0. The older `LastTestsFailed.log` timestamp (`2026-11-15T21:43:11Z`) predates the final passing run and is not current evidence.
- Code review: clean, no remaining findings; see [06-REVIEW.md](./06-REVIEW.md). No hosted run was dispatched; URL: none (local checks only).

The CTest RED report classifier initially rejected a record that omitted the complete XML document in its `output` field. The record was corrected using the unchanged report and its measured modification time, then the classifier returned `RED_EVIDENCE_OK`.

## Deferred Cross-Plan Acceptance

`DISC-01` remains globally pending until the remaining phase plans complete. XA decoding and SPU output are covered by Plans 06-09/06-10; DMA3's sector cursor and transfer costs are covered by Plan 06-27; disc-boot acceptance remains with its caller plans. No owner verification was required.

## Next Phase Readiness

Plan 06-05 is complete. Plan 06-06 may proceed with the pinned CD timing/Getloc references; Plan 06-27 remains sequential with DMA consumers and must complete before 06-15.

## Self-Check: PASSED

The summary exists and all five measured plan commits are ancestors of the current HEAD.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*
