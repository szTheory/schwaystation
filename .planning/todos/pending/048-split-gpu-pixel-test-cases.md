---
title: "Split long GPU pixel test cases"
status: pending
priority: P3
source: "phase 04 code review, standards finding on test case size"
created: 2026-11-10
theme: cleanup
bound_step: "SEED-007, the tune-up that opens the next milestone"
---

## Goal

Split the long GPU pixel test cases by behavior.

## Context

- Review finding: `case_textured_rectangle_mmio` and `case_arithmetic` exceed the one-screen function guideline.
- Deferred from phase 04 execution because review findings about test code become seeds, not phase requirements.

## Acceptance Criteria

- Each helper stays focused on one behavior group and the test cases retain the same known-answer coverage.
- All GPU pixel tests pass in `cmake --workflow --preset ci`.
