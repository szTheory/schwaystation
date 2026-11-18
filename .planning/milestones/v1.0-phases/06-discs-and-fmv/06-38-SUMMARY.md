---
phase: 06-discs-and-fmv
plan: "38"
subsystem: MDEC
tags: [mdec, ps1-tests, partial, output-target]
requires:
  - phase: 06-discs-and-fmv
    provides: "Pinned MDEC guests and ordered-status reference selectors"
provides:
  - "Pinned cross-mode evidence for an unsigned 4-bit nearest-nibble candidate"
  - "A dependent plan for the unresolved guest-buffer and DMA1 target relationship"
affects: [06-12, 06-13, 06-14, 06-39, DISC-03]
actuals:
  tokens: 8557
  tasks: 1
  commits: 1
commits: 1
plan_head_before: 721900297076e04c59e3ef8ea483e0e4227a5edb
plan_head_after: 48143b51c185e15145deba022c2817c666e390d6
tech-stack:
  added: []
  patterns:
    - "Use digest-pinned guest buffer targets separately from captured DMA register starts"
    - "Derive unsigned 4-bit conversion from all 64 paired pinned output samples"
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-39-PLAN.md
  modified:
    - src/mdec.c
    - tests/mdec/model.c
    - tests/mdec/reference.c
    - docs/hardware/mdec.md
    - .planning/phases/06-discs-and-fmv/06-REVIEW.md
    - .planning/ROADMAP.md
decisions:
  - "Keep the unsigned 4-bit nearest-nibble implementation as an unverified candidate because every pinned 4-bit output sample equals saturated rounding of the corresponding pinned 8-bit sample; do not claim psx-spx specifies that rounding."
  - "Keep the comparator target and observed DMA1 start distinct until the pinned caller path proves their relationship."
  - "Keep 8-bit and ordered-status selectors gating, preserve the 4,915,200-cycle cap, and leave DISC-03 incomplete."
requirements-completed: []
coverage:
  - id: MDEC-UNSIGNED-4BIT-CANDIDATE
    description: "The unsigned 4-bit candidate matches the complete pinned vector."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "Before candidate edit: ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.(4bit|8bit)$' --no-tests=error --output-on-failure; exit 8; 4-bit byte 4 expected fd, actual ec. Candidate and corrected fixture were not tested per review hold."
        status: partial
    human_judgment: false
  - id: MDEC-OUTPUT-TARGET-CONTRACT
    description: "Pinned guest buffer bytes and DMA1 start are checked as distinct caller-path facts."
    requirement: DISC-03
    verification:
      - kind: other
        ref: "Captured one DMA1 start per guest: 4-bit 0x001ffeb8 versus digest-pinned log target 0x001ffe0c; 8-bit 0x001ffe98 versus target 0x001ffdec. Comparator target parsing is unverified; Plan 06-39 owns caller-path proof."
        status: partial
    human_judgment: false
  - id: MDEC-8BIT-OUTPUT
    description: "The exact 8-bit guest output remains byte-for-byte matched to its pinned log."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "Before candidate edit: mdec.reference.8bit exit 8; byte 6 expected 04, actual 05. The corrected expected-word fixture and post-edit selector were not run."
        status: fail
    human_judgment: false
  - id: MDEC-ORDERED-STATUS-GUEST
    description: "The pinned status guest reaches Done and matches the ordered event log within the fixed bound."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.status$' --no-tests=error --output-on-failure; exit 8 at 19,200 x 256-cycle steps (4,915,200 cycles), 82 status events, 82 input writes, 0 output reads, no Done marker."
        status: fail
    human_judgment: false
  - id: MDEC-REVIEW-AND-BOUNDS
    description: "The review warning is recorded, expected bytes and selectors are preserved, and no test or cycle cap was changed."
    requirement: DISC-03
    verification:
      - kind: other
        ref: "git diff --check; exit 0. Code review WR-01 reported that documentation called the checked buffer a DMA1 destination; the hardware page now says the comparator reads the digest-pinned guest target and records DMA1 starts separately. The target relationship remains unresolved."
        status: partial
    human_judgment: false
duration: "at least 1h04m21s wall through final metadata commit; active work stopped at the 30m bound"
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 38: MDEC output target and 4-bit candidate

Captured identical pinned inputs and transform state, found an all-pixel cross-mode rule for unsigned 4-bit output, and split the unresolved output-target contract plus red 8-bit/status cases to Plan 06-39.

## Performance

- Duration: At least 1h04m21s wall from 2026-11-16T12:34:17Z through the final metadata commit; active work stopped at the 30-minute bound and the remaining wall time includes the required review hold.
- Runtime/model: Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.
- Tasks: One partial implementation slice; plan acceptance is incomplete.
- Commits: One measured Plan38 task commit before the summary.

## Findings and candidate changes

- The pinned 4-bit and 8-bit guests issue the same DMA0 command and 32 input words, with identical quantization/scale tables and post-IDCT block values.
- The pinned logs request buffers `0x001ffe0c` (4-bit) and `0x001ffdec` (8-bit). The sole captured DMA1 starts are `0x001ffeb8` and `0x001ffe98`, respectively. Their 0xAC offset means the DMA1 start cannot be treated as the requested guest buffer without tracing the staging/copy path.
- All 64 pinned unsigned 4-bit pixels equal `min(15, (corresponding 8-bit pixel + 8) >> 4)`. The candidate applies this conversion only to unsigned 4-bit output; signed output and 8-bit output are unchanged. The model's 8-bit expected words were corrected only to match the pinned log's byte order. These edits were not tested after modification.
- The reference comparator now parses the requested target from the digest-verified `mdec_readDecodedDma(addr=...)` log line, while retaining DMA1 start capture separately. This candidate target interpretation is unverified and Plan06-39 owns proving the caller contract.
- Code review warning WR-01 found that the hardware page incorrectly described the comparator as checking the DMA1 destination. The page now names the digest-pinned guest buffer target and separately records the DMA1 start; the address relationship itself remains unresolved. The review result remains in `.planning/phases/06-discs-and-fmv/06-REVIEW.md`.

## Test Evidence

| Check | Result |
|---|---|
| `ctest --test-dir build/ci -R '^mdec\.guest_input$' --no-tests=error --output-on-failure` before the candidate edit | exit 8; 19 exact model vector mismatches |
| `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.(4bit|8bit)$' --no-tests=error --output-on-failure` before the candidate edit | exit 8; 4-bit byte 4 expected `fd`, actual `ec`; 8-bit byte 6 expected `04`, actual `05` |
| `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.status$' --no-tests=error --output-on-failure` | exit 8 at the unchanged 19,200 × 256-cycle cap; 82 status events, 82 input writes, zero output reads, no Done marker |
| `git diff --check` | exit 0 |
| Post-edit candidate/model/reference selectors | Not run, per the required adversarial review hold |

The 8-bit arithmetic discrepancy and ordered-status guest remain unresolved. The fixed 4,915,200-cycle cap, exact expected bytes, and selectors are unchanged. DISC-03 is not complete.

## Deviations and follow-up

Plan38 reached its 30-minute active-work bound before output-target, 8-bit, and ordered-status acceptance could be verified. The unsigned 4-bit behavior is retained only as an evidence-backed, unverified candidate. Plan06-39 owns the caller buffer versus DMA1 staging relationship, a separate runnable 4-bit model check and pinned guest check, and the still-gating 8-bit/status selectors. Downstream MDEC plans now depend on 06-39.

## Self-Check: PASSED

- Summary exists at `.planning/phases/06-discs-and-fmv/06-38-SUMMARY.md`.
- Follow-up plan exists at `.planning/phases/06-discs-and-fmv/06-39-PLAN.md`.
- Plan task commit `48143b51c185e15145deba022c2817c666e390d6` is an ancestor of HEAD.
