---
phase: 06-discs-and-fmv
plan: "45"
subsystem: hosted-acceptance
tags: [cdrom, mdec, discs, retroarch, ci]
requires: ["06-44"]
provides:
  - "Exact-head CI and Nightly acceptance for the approved Phase 6 v1.0 scope."
  - "Eight-platform deterministic hash equality, including the generated M3U newline correction."
  - "Hosted RetroArch disc boot and pixel comparison evidence."
affects: ["Phase 6 verification", "v1.0 ship"]
tech-stack:
  added: []
  patterns:
    - "Normalize generated text fixture newlines before byte-for-byte cross-platform hashing."
    - "Diff exact run artifacts to distinguish generated-input drift from emulator output drift."
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-45-SUMMARY.md
  modified:
    - tests/runner/tests.cmake
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - .planning/phases/06-discs-and-fmv/06-VERIFICATION.md
    - .planning/LESSONS.md
key-decisions:
  - "Phase 6 acceptance follows the approved MDEC scope: exact pinned 4-bit coverage, a separate 8-bit regression baseline, and bounded source coverage; exact 8-bit and full frame/movie hardware output remain unproved."
  - "PR-only rehearsal and cleanup checks remain pending for the authorized ship workflow."
requirements-completed:
  - DISC-01
  - DISC-02
  - DISC-03
  - DISC-04
coverage:
  - id: DISC-01-HOSTED
    description: "CD-ROM timing, Getloc and controller behavior pass the exact Nightly inventory and the bounded CD-ROM fuzz and replay checks."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "`gh run view 44218205653 --exit-status` exited 0. `vectors-full` passed 30/30; its JUnit check confirmed all 30 tests ran once each, covering the three CD-ROM references. The six-campaign fuzz job, including `cdrom`, and its replay-seed preflight passed. https://github.com/szTheory/schwaystation/actions/runs/44218205653"
      status: pass
    human_judgment: false
  - id: DISC-02-HOSTED
    description: "The owned disc boots through OpenBIOS, reads its media, and has matching per-platform boot, mailbox, audio and frame hashes; image parsing and disc identity checks pass."
    requirement: DISC-02
    verification:
      kind: e2e
      ref: "`gh run view 44218206100 --exit-status` exited 0. All eight `hashes-<platform>` manifests downloaded from that run compare byte-identically; `disc_boot` records `boots@600`, `mailbox@900`, `audio@900` and `frame@900` for `none` and `press@300`. Linux x64 and both Windows lanes ran all 613 CI tests once and all five release tests once; the Windows CUE root-child and compatibility-edge regressions passed. https://github.com/szTheory/schwaystation/actions/runs/44218206100"
      status: pass
    human_judgment: false
  - id: DISC-03-HOSTED
    description: "The selected deterministic MDEC implementation, direct controls, exact 4-bit guest, separate 8-bit regression and bounded full-source preflights pass under the approved v1.0 accuracy claim."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "Exact-head CI and Nightly passed on source commit `28683ebe4c9940c95d539b5c18733c139f940ffa`. Nightly `vectors-full` passed 30/30 and the full CI inventory passed on all applicable architecture lanes. The retained evidence does not establish exact 8-bit hardware output or complete frame/movie output conformance. https://github.com/szTheory/schwaystation/actions/runs/44218206100 ; https://github.com/szTheory/schwaystation/actions/runs/44218205653"
      status: pass
    human_judgment: false
  - id: DISC-04-HOSTED
    description: "Disk control passes the in-process tests and the hosted RetroArch run boots the owned disc with its displayed frame matching the runner."
    requirement: DISC-04
    verification:
      kind: e2e
      ref: "RetroArch job `134517630570` in CI run 44218206100 passed: the owned `disc_boot.cue` ran 900 frames using CI OpenBIOS, and `tests/retroarch/verify.py compare disc-runner.bmp disc.bmp 16 16` reported every pixel equal. https://github.com/szTheory/schwaystation/actions/runs/44218206100"
      status: pass
    human_judgment: false
  - id: PHASE6-PORTABILITY-REPAIR
    description: "The Windows separator correction preserves canonical root containment, and the generated M3U uses stable Unix newlines so every platform hashes the same fixture bytes."
    requirement: DISC-02
    verification:
      kind: other
      ref: "Reviews in `06-45-REVIEW.md` found no issues. The prior exact-head CI failure was reproduced by artifact diff; the corrected run's eight manifests compare byte-identically. Local `cmake --preset ci`, `runner.disc.api` (1/1), hygiene (23/23) and the 2,700-line budget passed."
      status: pass
    human_judgment: false
---

# Phase 6 Plan 06-45: Hosted acceptance

**Exact-head hosted CI, Nightly, deterministic hash equality and RetroArch acceptance passed for the approved Phase 6 scope. Phase verification is complete; the authorized ship, merge and release steps are next.**

## Runtime and source

- **Runtime/model:** Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); independent code and goal reviewers.
- **Source commit tested:** `28683ebe4c9940c95d539b5c18733c139f940ffa` (`fix(tests): normalize generated playlist newlines`). The branch head after a planning-only evidence commit is `ce30dc104378e924ecc14b0f554bf35dbf0ffe19`.
- **Review:** `06-45-REVIEW.md`; no open findings.

## Hosted acceptance

- CI run [44218206100](https://github.com/szTheory/schwaystation/actions/runs/44218206100) and Nightly run [44218205653](https://github.com/szTheory/schwaystation/actions/runs/44218205653) both passed with `gh run view RUN_ID --exit-status`.
- All eight platform hash manifests compare byte-identically. The earlier mismatch came from Windows CRLF output in the generated runner M3U, now fixed by specifying `NEWLINE_STYLE UNIX`.
- Nightly passed all 30 inventory tests once, all six bounded fuzz campaigns and all four benchmark jobs.
- RetroArch ran the owned disc for 900 frames with CI OpenBIOS; its 16×16 frame matched the runner output pixel for pixel.
- Local `cmake --preset ci`, the focused runner disc API selector, and the full hygiene workflow passed. Hygiene remained within its 2,700-line ceiling.

Exact 8-bit hardware output and complete frame/movie hardware conformance remain unproved and are not part of the accepted v1.0 claim. PR-only rehearsal and cleanup remain pending for ship.

These exact-head results close the hosted acceptance left pending in Plans 06-23 and 06-24; those summaries remain historical records of the earlier gaps.

## Next

Run phase verification, then `$gsd-ship 6`. After the approved PR checks pass, enable auto-merge, verify the merge and release PR, and complete the v1.0 milestone audit and archive.
