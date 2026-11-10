---
title: "Put the ISC permission text in the src/gte.c header"
status: pending
priority: P3
source: "phase 03 code review, finding IN-07"
created: 2026-11-10
theme: provenance
bound_step: "SEED-007, the tune-up that opens the next milestone"
---

## Goal

Put the ISC permission text in the src/gte.c header.

## Context

- Review finding: The `src/gte.c` header carries the SPDX and copyright lines but only points to the ISC permission text in THIRD-PARTY-NOTICES.md.
- Bound to: SEED-007, the tune-up that opens the next milestone.

## Acceptance Criteria

- The notices gate or a reviewer can confirm the permission text travels with the file, or the project records that the notices file is the accepted carrier.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
