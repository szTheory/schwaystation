---
title: "Make the dev preset's abi.undefined pass on macOS Debug"
status: pending
priority: P2
source: "phase 01, plan 01-15"
created: 2026-10-20
theme: abi
---

## Goal

`cmake --workflow --preset dev` (Debug, AppleClang) passes `abi.undefined`, which today fails because `libschwaystation.a` references `__memcpy_chk` and `__memset_chk`.

## Context

- Recorded in `.planning/phases/01-a-test-frame-in-retroarch/deferred-items.md` (found in plan 01-10; the `ci` Release preset passes).
- Two fixes: a `-D_FORTIFY_SOURCE=0` core option, or the two names on the Darwin helper row of `tests/cmake/abi.cmake`.
- Bound to phase 2: the first plan that edits `schway_core_options` or `tests/cmake/abi.cmake`.

## Acceptance Criteria

- `cmake --workflow --preset dev` exits 0 on macOS.
- `abi.undefined.self_test` still fails its fixture.
