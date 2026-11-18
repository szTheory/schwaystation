---
id: SEED-261118-8b0
status: dormant
planted: 2026-11-18
planted_during: milestone v1.0 closeout, Phase 1 verification refresh
trigger_when: when the tracked-tree firmware scan, release archive scan, or their path enumeration is next changed
scope: small
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
  status: dormant
---

# SEED-261118-8b0: Make firmware-scan path enumeration fail closed

## Why This Matters

`tests/cmake/firmware_scan.cmake` converts newline-delimited `git ls-files` output into a CMake list by
replacing newlines with semicolons. A valid tracked filename containing `;` is split into multiple entries,
so its contents can be skipped by the tracked-tree firmware and licence scan. Newline-containing names are
ambiguous too. This weakens the repository's asset-policy guard if a future tracked file uses either
separator. The current tracked tree has neither kind of name, and the v0.6.0 release scan passed; this is a
latent tooling bypass, not evidence that prohibited bytes were present or published.

## When to Surface

**Trigger:** when changing the tracked-tree firmware scan, release archive scan, or their path enumeration.

Before claiming the source-tree scan covers every tracked file, parse paths without ambiguity or fail closed
on unsupported separators. Add a synthetic tracked-file regression with a semicolon in its name; include a
newline-name control if the chosen parser claims to support it.

## Scope Estimate

**Small** — harden the CMake path handling and register the synthetic filename case in the release test
inventory. Keep all checks in the existing `release.firmware_scan` gate.

## Breadcrumbs

- `tests/cmake/firmware_scan.cmake`: tracked-file enumeration at the current scan boundary.
- `tests/release/tests.cmake`: registered firmware-scan tests and edge controls.
- `.planning/phases/04-games-play-in-retroarch/04-21-PLAN.md`: first caller for the release firmware scanner.
- `.planning/phases/04-games-play-in-retroarch/04-21-SUMMARY.md`: scanner implementation and accepted coverage.
- `.planning/phases/01-a-test-frame-in-retroarch/01-REVIEW.md`: review context; this finding is outside Phase 1's source snapshot.

## Notes

Captured from the verification-refresh code-review gate. Per project policy, this test/tooling finding is a
seed rather than a new v1.0 requirement. No current release asset or core behavior failed verification.
