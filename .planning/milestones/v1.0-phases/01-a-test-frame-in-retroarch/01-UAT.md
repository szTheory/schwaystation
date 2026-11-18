---
status: complete
phase: 01-a-test-frame-in-retroarch
source:
- 01-01-SUMMARY.md
- 01-02-SUMMARY.md
- 01-03-SUMMARY.md
- 01-04-SUMMARY.md
- 01-05-SUMMARY.md
- 01-06-SUMMARY.md
- 01-07-SUMMARY.md
- 01-08-SUMMARY.md
- 01-09-SUMMARY.md
- 01-10-SUMMARY.md
- 01-11-SUMMARY.md
- 01-12-SUMMARY.md
- 01-13-SUMMARY.md
- 01-14-SUMMARY.md
- 01-15-SUMMARY.md
started: '2026-10-29T18:23:07Z'
updated: 2026-10-29T18:27:01.325Z
---

## Current Test

[testing complete]

## Verification Policy

AGENTS rule 6 supersedes the generic conversational checkpoints. The agent runs checks, reads hosted evidence, and judges docs and images. No owner responses are inferred or fabricated. The earlier identity-bound ruleset action is already complete.

Every structured coverage entry from all 15 summaries is retained. Intermediate plan snapshots (such as the empty hygiene inventory before plan 06 and the pre-adapter core) are historical observations, not claims about the final tree. Five invalid legacy status values and one unknown rehearsal result are resolved through later executed plans and hosted evidence, without rewriting the historical summaries.

## Tests

### 1. Plan 01 / D1
expected: "Runner prints the pinned NTSC-U frame-60 card hash, equal to an independent model"
result: pass
source: automated
coverage_id: D1
source_summary: 01-01-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/runner/tests.cmake#runner.hash"
  - "python3 -I card_model.py (scratchpad one-off) exit 0; build/tracer/schwaystation-run --frames 60 --hash exit 0; both print be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71"

### 2. Plan 01 / D2
expected: "The card moves: frame 5 and frame 60 hash differently"
result: pass
source: automated
coverage_id: D2
source_summary: 01-01-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "build/tracer/schwaystation-run --frames 5 --hash vs --frames 60 --hash, differ, exit 0"

### 3. Plan 01 / D3
expected: "cmake --workflow --preset ci configures, builds and tests; the registered set equals the union of the directories ci.txt selects; every selected test ran once"
result: pass
source: automated
coverage_id: D3
source_summary: 01-01-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "cmake --workflow --preset ci exit 0"
  - "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exit 0"

### 4. Plan 01 / D4
expected: "The inventory reader fails on an unsorted list, a name in two directories and a selected directory with no inventory.txt, and its union ignores selection order"
result: pass
source: automated
coverage_id: D4
source_summary: 01-01-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "cmake -DSEL=<scratch sel.txt> -DDIR=<scratch tests> -P <scratch driver including tests/cmake/inventory.cmake>: unsorted exit 1, duplicate exit 1, missing exit 1, reordered selection exit 0"

### 5. Plan 01 / D5
expected: "An empty selection configures and a test preset selecting nothing fails through noTestsAction error"
result: pass
source: automated
coverage_id: D5
source_summary: 01-01-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "cmake --preset hygiene exit 0; ctest --preset hygiene exit 8 (non-zero, as required)"

### 6. Plan 01 / D6
expected: "Public header is the complete v1 surface with no deferred name; layouts pinned by _Static_assert; no floating point in src or include"
result: pass
source: automated
coverage_id: D6
source_summary: 01-01-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "grep -c schway_ include/schwaystation.h = 33; deferred-name grep count 0; grep -c _Static_assert src/internal.h = 73; float|double count 0"

### 7. Plan 01 / D7
expected: "No planning ID and no seed name in code, tests, config or scripts"
result: pass
source: automated
coverage_id: D7
source_summary: 01-01-SUMMARY.md
verification_refs:
  - "git grep --untracked planning-ID scan over CMakeLists.txt CMakePresets.json cmake include src host runner tests scripts/hygiene.sh; test $? -eq 1 exit 0; cruft grep a=1 b=1 exit 0"

### 8. Plan 02 / D1
expected: "libretro.h vendored unmodified with its digest in PROVENANCE section 8 and THIRD-PARTY-NOTICES.md"
result: pass
source: automated
coverage_id: D1
source_summary: 01-02-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "shasum -a 256 libretro/libretro.h equals 036ddffc995ad116c3081ca0a7515d09c2a116f3483e4f5012fe0c29a885da76, plus the Task 1 grep chain and sh scripts/hygiene.sh --tree, exit 0"

### 9. Plan 02 / D2
expected: "PROVENANCE pins cogmaster v0.1.0 at a public commit and states the deferred notices and vendored checks"
result: pass
source: automated
coverage_id: D2
source_summary: 01-02-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "gh api repos/szTheory/cogmaster/commits/f09ac97096acda99888270e4c1e4dc96e12ffdfe exit 0; grep -c f09ac97096ac PROVENANCE.md prints 5; grep -c abd950941e93 PROVENANCE.md prints 2"

### 10. Plan 02 / D3
expected: "README is user-facing, short and free of planning IDs"
result: pass
source: automated
coverage_id: D3
source_summary: 01-02-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "test $(wc -c < README.md) -lt 16384 exit 0 (2418 bytes); grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' README.md prints 0"

### 11. Plan 02 / D4
expected: "One-off App script removed and publish.sh re-synced to the seed, lowering the gate-line total"
result: pass
source: automated
coverage_id: D4
source_summary: 01-02-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "seed publish.sh through sed cogmaster to schwaystation, diff against scripts/publish.sh, exit 0; sh -n and dash -n exit 0; git grep setup-release-app outside .planning status 1"
  - "cmake --workflow --preset ci exit 0"

### 12. Plan 03 / D1
expected: "create makes exactly one allocation and destroy one free of the same size; nothing in between allocates; fail_from 1 gives ERR_NO_MEMORY with out unchanged and nothing leaked"
result: pass
source: automated
coverage_id: D1
source_summary: 01-03-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/core/tests.cmake#core.no_alloc_after_load"

### 13. Plan 03 / D2
expected: "Every card rule in first-match order at frames 1, 5 and 60, the counter strip (frame 5, 60 and 0x80000000), frames differ, every display word redrawn each frame"
result: pass
source: automated
coverage_id: D2
source_summary: 01-03-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/core/tests.cmake#core.testcard.pixels"
  - "tests/core/tests.cmake#core.testcard.counter"
  - "tests/core/tests.cmake#core.testcard.frames_differ"
  - "tests/core/tests.cmake#core.testcard.full_redraw"

### 14. Plan 03 / D3
expected: "Display tuple from the power-on range words (320 x 240, pitch 2048, divider 8) and equal hashes through 640- and 2048-byte pitches with padding untouched"
result: pass
source: automated
coverage_id: D3
source_summary: 01-03-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/core/tests.cmake#core.video.tuple"
  - "tests/core/tests.cmake#core.video.pitch"

### 15. Plan 03 / D4
expected: "Exact tick accumulator and audio pairs over 4,000 frames per region (566,203 or 566,204 and 680,823 or 680,824 cycles; 737 or 738 and 886 or 887 pairs; total = floor(ticks / 768)); ring overflow dropped and counted; zero-capacity drain"
result: pass
source: automated
coverage_id: D4
source_summary: 01-03-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/core/tests.cmake#core.audio.pairs"
  - "tests/core/tests.cmake#core.audio.ring"
  - "tests/core/tests.cmake#core.info"

### 16. Plan 03 / D5
expected: "Profile rejection and lock, power reset, size-tag rules, status text, refused calls change nothing, allocator hooks required, destroy(NULL) and run_frame(inst, NULL)"
result: pass
source: automated
coverage_id: D5
source_summary: 01-03-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/core/tests.cmake#core.profile.rejected"
  - "tests/core/tests.cmake#core.profile.locked"
  - "tests/core/tests.cmake#core.reset.power"
  - "tests/core/tests.cmake#core.api.size_tags"
  - "tests/core/tests.cmake#core.api.statuses"
  - "tests/core/tests.cmake#core.api.error_changes_nothing"
  - "tests/core/tests.cmake#core.create.allocator"

### 17. Plan 03 / D6
expected: "Two instances stepped interleaved give the same frame hashes and state bytes as each stepped alone"
result: pass
source: automated
coverage_id: D6
source_summary: 01-03-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/core/tests.cmake#core.instances.independent"

### 18. Plan 03 / D7
expected: "State round trip, save-load-save stable bytes, each corruption refused with its status before applying, visitor serialisation equals the saved body, record sweep refuses every bounded field"
result: pass
source: automated
coverage_id: D7
source_summary: 01-03-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/core/tests.cmake#core.state.round_trip"
  - "tests/core/tests.cmake#core.state.stable_bytes"
  - "tests/core/tests.cmake#core.state.rejects"
  - "tests/core/tests.cmake#core.state.visit"
  - "tests/core/tests.cmake#core.state.record_sweep"

### 19. Plan 03 / D8
expected: "docs/embedding.md explains the API lifecycle and has a test-card section on what each band catches and how to read the counter"
result: pass
source: automated
coverage_id: D8
source_summary: 01-03-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "grep -c -i counter docs/embedding.md prints 2; wc -c docs/embedding.md under 61440; planning-ID grep count 0; agent read the guide against src/instance.c, src/frame.c and src/state.c"

### 20. Plan 03 / D9
expected: "No planning ID in src, tests/core or tests/alloc.h; no seed name anywhere outside the excluded files"
result: pass
source: automated
coverage_id: D9
source_summary: 01-03-SUMMARY.md
verification_refs:
  - "git grep --untracked planning-ID scan over src tests/core tests/alloc.h; test $? -eq 1 exit 0; cruft grep a=1 b=1 exit 0"

### 21. Plan 04 / D1
expected: "Frame 5, NTSC-J frame 60 and PAL frame 60 hashes equal pins from the independent model, and the three frame-60 pins differ pairwise (configure check)"
result: pass
source: automated
coverage_id: D1
source_summary: 01-04-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/runner/tests.cmake#runner.hash.frame5"
  - "tests/runner/tests.cmake#runner.hash.ntsc_j"
  - "tests/runner/tests.cmake#runner.hash.pal"
  - "python3 -I card_model.py (scratchpad one-off, not committed) exit 0; values equal the pins"

### 22. Plan 04 / D2
expected: "runner.hashes_file writes build/<preset>/hashes.txt with five LF-only lines in the fixed order, no two sharing a hash"
result: pass
source: automated
coverage_id: D2
source_summary: 01-04-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/runner/tests.cmake#runner.hashes_file"
  - "od -c build/ci/hashes.txt | grep -c '\\r' printed 0; unique field-5 count equals line count, exit 0"

### 23. Plan 04 / D3
expected: "The compare mode fails on two files one hex digit apart and on fewer files than COUNT, each with its named message"
result: pass
source: automated
coverage_id: D3
source_summary: 01-04-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/runner/tests.cmake#runner.hashes_compare.byte.self_test"
  - "tests/runner/tests.cmake#runner.hashes_compare.count.self_test"

### 24. Plan 04 / D4
expected: "The shared converter matches its known answers (0, 1, 15, 16, 31 to 0, 8, 123, 132, 255 per channel; 0x7FFF to 0x00FFFFFF; 0x8000 to 0), the mask-twin band converts equal to the grey band while all 310 word pairs differ, and the frame converter honours both pitches"
result: pass
source: automated
coverage_id: D4
source_summary: 01-04-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "tests/runner/tests.cmake#runner.xrgb8888"

### 25. Plan 04 / D5
expected: "The PNG encoder's chunk CRCs, Adler-32 and stored blocks pass an independent parse, and its size refusals hold"
result: pass
source: automated
coverage_id: D5
source_summary: 01-04-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "tests/runner/tests.cmake#runner.png"

### 26. Plan 04 / D6
expected: "--frames 5 --png 5:FILE decodes pixel for pixel to the converted card frame 5, then matches SCHWAYSTATION_TESTCARD_PNG_SHA256"
result: pass
source: automated
coverage_id: D6
source_summary: 01-04-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "tests/runner/tests.cmake#runner.png_file"
  - "python3 -I png_model.py (scratchpad one-off, not committed) exit 0 printed the same digest 62ddd3f2..."
  - "Agent viewed build/ci/card5.png: card bands, row strip, grey and mask-twin bands identical, counter reads 101"

### 27. Plan 04 / D7
expected: "Every malformed command line exits 2 with a usage message and writes nothing"
result: pass
source: automated
coverage_id: D7
source_summary: 01-04-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "tests/runner/tests.cmake#runner.usage.no_args"
  - "tests/runner/tests.cmake#runner.usage.unknown"
  - "tests/runner/tests.cmake#runner.usage.positional"
  - "tests/runner/tests.cmake#runner.usage.frame_range"
  - "tests/runner/tests.cmake#runner.usage.png_syntax"
  - "tests/runner/tests.cmake#runner.usage.region"

### 28. Plan 04 / D8
expected: "No planning ID and no seed name in code or tests; gate-line ceiling for hashes.cmake held"
result: pass
source: automated
coverage_id: D8
source_summary: 01-04-SUMMARY.md
verification_refs:
  - "planning-ID git grep over runner host tests/runner tests/cmake/hashes.cmake tests/testcard_pins.cmake; test $? -eq 1 exit 0; cruft grep a=1 b=1 exit 0; hashes.cmake 55 lines <= 55 exit 0"

### 29. Plan 05 / D1
expected: "abi.symbols passes on the Release core archive and its self-test fails with exit 1 naming the export outside the schway_ namespace"
result: pass
source: automated
coverage_id: D1
source_summary: 01-05-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "tests/abi/tests.cmake#abi.symbols"
  - "tests/abi/tests.cmake#abi.symbols.self_test"
  - "nm -g build/ci/libschwaystation.a, every defined external name starts with _schway (awk filter exit 0, no other name)"

### 30. Plan 05 / D2
expected: "abi.globals finds no writable data symbol in the core and its self-test fails with exit 1 naming the fixture's counter"
result: pass
source: automated
coverage_id: D2
source_summary: 01-05-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "tests/abi/tests.cmake#abi.globals"
  - "tests/abi/tests.cmake#abi.globals.self_test"

### 31. Plan 05 / D3
expected: "abi.undefined allows only memcpy, memmove, memset, memcmp and the per-system helpers, and its self-test fails with exit 1 naming abort"
result: pass
source: automated
coverage_id: D3
source_summary: 01-05-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "tests/abi/tests.cmake#abi.undefined"
  - "tests/abi/tests.cmake#abi.undefined.self_test"

### 32. Plan 05 / D4
expected: "core.no_host_clock finds no undefined host clock symbol in the core and its self-test fails with exit 1 on the fixture calling time(NULL)"
result: pass
source: automated
coverage_id: D4
source_summary: 01-05-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/abi/tests.cmake#core.no_host_clock"
  - "tests/abi/tests.cmake#core.no_host_clock.self_test"

### 33. Plan 05 / D5
expected: "The public header compiles alone as C17 and C++17 with -Wall -Wextra -Wpedantic -Werror, asserting each struct's leading uint32_t size at offset 0, a 4-byte status, API version 1 and every function and visitor signature"
result: pass
source: automated
coverage_id: D5
source_summary: 01-05-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "tests/abi/tests.cmake#abi.header.c17"
  - "tests/abi/tests.cmake#abi.header.cxx17"

### 34. Plan 05 / D6
expected: "tests/api/v1.c calls every public function, assigns every field and checks every constant, and abi.api_version fails naming any header name v1.c lacks; its self-test fails on a header copy with one extra function"
result: pass
source: automated
coverage_id: D6
source_summary: 01-05-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "tests/abi/tests.cmake#abi.api_v1"
  - "tests/abi/tests.cmake#abi.api_version"
  - "tests/abi/tests.cmake#abi.api_version.self_test"

### 35. Plan 05 / D7
expected: "abi.nofp_fixture lives in tests/nofp and is registered only by the nofp preset, where a double division must fail to build under -mgeneral-regs-only"
result: pass
source: automated
coverage_id: D7
source_summary: 01-05-SUMMARY.md
requirement: FRAME-04
classification_reason: validation_failed
resolution: "Both hosted nofp jobs pass in current run 40929433476; the historical not_run value is not treated as passing evidence."
verification_refs:
  - "grep -c abi.nofp_fixture tests/nofp/inventory.txt printed 1; grep -c nofp_fixture tests/abi/tests.cmake printed 0"
  - "tests/nofp/tests.cmake#abi.nofp_fixture"

### 36. Plan 06 / D1
expected: "policy.gate_budget counts scripts/*.sh, tests/cmake/*.cmake and .githooks/* at a 1500-line limit; its self test fails with exit 1 and the over-budget message on a 10-line fixture at LIMIT 9; LIMIT 10 passes and a pattern matching no file fails naming it"
result: pass
source: automated
coverage_id: D1
source_summary: 01-06-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/hygiene/tests.cmake#policy.gate_budget"
  - "tests/hygiene/tests.cmake#policy.gate_budget.self_test"
  - "cmake -DROOT=build/hygiene/hygiene/line_budget_fixture -DLIMIT=10 -DPATTERNS='*' -P tests/cmake/line_budget.cmake exit 0; cmake -DROOT=. -DLIMIT=1500 '-DPATTERNS=nothing/*.sh' -P tests/cmake/line_budget.cmake exit 1 naming nothing/*.sh"

### 37. Plan 06 / D2
expected: "hygiene.tree runs the tree scan on the real tree; its self test fails with exit 1 naming the home directory path and the address planted in a throwaway repository"
result: pass
source: automated
coverage_id: D2
source_summary: 01-06-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/hygiene/tests.cmake#hygiene.tree"
  - "tests/hygiene/tests.cmake#hygiene.tree.self_test"

### 38. Plan 06 / D3
expected: "hygiene.no_bios_bytes runs the tree scan; its self test plants a 524288-byte NUL file and an executable-signature file generated at configure time and expects 'BIOS image by size' and 'PlayStation executable by content' in path order"
result: pass
source: automated
coverage_id: D3
source_summary: 01-06-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/hygiene/tests.cmake#hygiene.no_bios_bytes"
  - "tests/hygiene/tests.cmake#hygiene.no_bios_bytes.self_test"

### 39. Plan 06 / D4
expected: "hygiene.no_float scans src/ and include/ for float, double, the float headers and float literals; its self test fails with exit 1 on a fixture with six float lines"
result: pass
source: automated
coverage_id: D4
source_summary: 01-06-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "tests/hygiene/tests.cmake#hygiene.no_float"
  - "tests/hygiene/tests.cmake#hygiene.no_float.self_test"

### 40. Plan 06 / D5
expected: "ci.title accepts the Conventional Commit forms and rejects Phase titles, pipeline-scoped release types and skip tokens; ci.title.self_test fails with the named message on a Phase title"
result: pass
source: automated
coverage_id: D5
source_summary: 01-06-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/hygiene/tests.cmake#ci.title"
  - "tests/hygiene/tests.cmake#ci.title.self_test"

### 41. Plan 06 / D6
expected: "ci.classify, policy.publish_set (18 archives in C-locale order, 17 and 19 fail through its self test), policy.publish_ref and policy.publish_signer run the scripts the rehearsal and the changes job use"
result: pass
source: automated
coverage_id: D6
source_summary: 01-06-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "tests/hygiene/tests.cmake#ci.classify"
  - "tests/hygiene/tests.cmake#policy.publish_set"
  - "tests/hygiene/tests.cmake#policy.publish_set.self_test"
  - "tests/hygiene/tests.cmake#policy.publish_ref"
  - "tests/hygiene/tests.cmake#policy.publish_signer"

### 42. Plan 06 / D7
expected: "The five scripts are trimmed to comment and blank changes only; scripts/*.sh plus .githooks/* is 592 lines (ceiling 620) and line_budget.cmake plus no_float.cmake 73 (ceiling 75)"
result: pass
source: automated
coverage_id: D7
source_summary: 01-06-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "for f in scripts/*.sh; do sh -n and dash -n; done exit 0; git diff b7b41fb -- scripts/ shows no changed line that is not a comment or blank (grep exit 1)"
  - "test \"$(cat scripts/*.sh .githooks/* | wc -l)\" -le 620 exit 0; test \"$(cat tests/cmake/line_budget.cmake tests/cmake/no_float.cmake | wc -l)\" -le 75 exit 0"

### 43. Plan 07 / D1
expected: "perf_compare.cmake computes the Ir per frame delta in basis points with integer math; exactly +1.00 % passes and +1.01 % fails in gate mode, while report mode exits 0 on FAIL"
result: pass
source: automated
coverage_id: D1
source_summary: 01-07-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/perf/tests.cmake#perf.gate.plus_1_00"
  - "tests/perf/tests.cmake#perf.gate.plus_1_01"
  - "tests/perf/tests.cmake#perf.gate.identical"

### 44. Plan 07 / D2
expected: "A current allow line raises the limit and an expired one does not; unequal bases give UNSTABLE; no base gives NO_BASE, failing only in gate mode; a missing measurement or unset mode is an input error in either mode"
result: pass
source: automated
coverage_id: D2
source_summary: 01-07-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/perf/tests.cmake#perf.gate.allowed"
  - "tests/perf/tests.cmake#perf.gate.unstable"
  - "tests/perf/tests.cmake#perf.gate.no_base"
  - "tests/perf/tests.cmake#perf.gate.missing"

### 45. Plan 07 / D3
expected: "docs.budgets keeps README.md under 16384 bytes, include/schwaystation.h under 40960 and each docs/*.md under 61440, naming each file at or over; an empty docs glob passes"
result: pass
source: automated
coverage_id: D3
source_summary: 01-07-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "tests/policy/tests.cmake#docs.budgets"
  - "cmake -DROOT=. -P tests/cmake/budgets.cmake exit 0; ROOT at a scratch copy of the header and README only exit 0; the same with a 61440-byte docs page exit 1, with a 61439-byte page exit 0"

### 46. Plan 07 / D4
expected: "policy.no_skip fails naming file and line when a test file sets a skip or disable property; policy.no_skip.self_test proves it with exit 1 and the named line"
result: pass
source: automated
coverage_id: D4
source_summary: 01-07-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/policy/tests.cmake#policy.no_skip"
  - "tests/policy/tests.cmake#policy.no_skip.self_test"

### 47. Plan 07 / D5
expected: "docs/testing.md covers the presets, the inventory model, adding a test, negative controls, the gate-line and end-to-end budgets, reconfiguring after commits, CI-only lanes and the perf gate, under 61440 bytes and with no planning IDs"
result: pass
source: automated
coverage_id: D5
source_summary: 01-07-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "test -f docs/testing.md && test \"$(wc -c < docs/testing.md)\" -lt 61440 && cmake --workflow --preset ci exit 0; grep -c -i inventory docs/testing.md prints 10; grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' docs/testing.md prints 0"

### 48. Plan 07 / D6
expected: "The three budgeted scripts total at or under 160 lines and carry no planning ID"
result: pass
source: automated
coverage_id: D6
source_summary: 01-07-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "test \"$(cat tests/cmake/no_skip.cmake tests/cmake/perf_compare.cmake tests/cmake/budgets.cmake | wc -l)\" -le 160 exit 0 (160); planning-ID git grep over the plan's paths, test $? -eq 1 exit 0"

### 49. Plan 08 / D1
expected: "install.stage installs both headers, the static library and the package into build/ci/stage, and install.consumer builds tests/consumer with find_package against it and prints the api 1, 320x240 line"
result: pass
source: automated
coverage_id: D1
source_summary: 01-08-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "tests/install/tests.cmake#install.stage"
  - "tests/install/tests.cmake#install.consumer"

### 50. Plan 08 / D2
expected: "install.consumer.cxx builds a C++17 program against the installed header; install.consumer.dll_crt builds the C consumer on the DLL C runtime"
result: pass
source: automated
coverage_id: D2
source_summary: 01-08-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "tests/install/tests.cmake#install.consumer.cxx"
  - "tests/install/tests.cmake#install.consumer.dll_crt"

### 51. Plan 08 / D3
expected: "embed.subdirectory builds a C-only project that pulls in the source tree with add_subdirectory, gets only the library and its alias, no C++, no tests and no install rule, and runs one frame"
result: pass
source: automated
coverage_id: D3
source_summary: 01-08-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "tests/install/tests.cmake#embed.subdirectory"
  - "cmake -DINSTALL_SCRIPT=build/ci/cmake_install.cmake -P tests/embed/CMakeLists.txt exits 1 (the top-level tree has install rules); against build/ci/embed/schwaystation/cmake_install.cmake it exits 0"

### 52. Plan 08 / D4
expected: "The consumers require the install_staged fixture and build in their own directories, so parallel CTest from an empty stage passes"
result: pass
source: automated
coverage_id: D4
source_summary: 01-08-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "ctest --test-dir build/ci -L install -j 4 exits 0, run once with the stage and consumer build directories removed first"

### 53. Plan 08 / D5
expected: "Each install and embed test runs exactly once in the ci preset, and the five are the install inventory"
result: pass
source: automated
coverage_id: D5
source_summary: 01-08-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exits 0"

### 54. Plan 09 / D1
expected: "--bench N prints frames then nearest-rank p50_us, p90_us, p99_us, p99.9_us, p99.99_us, p99.999_us, max_us and max_rss_kib in that order, one whole number each"
result: pass
source: automated
coverage_id: D1
source_summary: 01-09-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/runner/tests.cmake#runner.bench.percentile"
  - "tests/runner/tests.cmake#runner.bench.format"
  - "build/ci/schwaystation-run --bench 600 piped to grep -E -c for the eight value lines prints 8, exit 0"

### 55. Plan 09 / D2
expected: "A sample count whose buffer would wrap, and --bench 0, negative or missing, exit 2 before anything runs"
result: pass
source: automated
coverage_id: D2
source_summary: 01-09-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/runner/tests.cmake#runner.bench.bytes"

### 56. Plan 09 / D3
expected: "The clock and RSS reads live in the runner; no core source calls clock_gettime, getrusage or QueryPerformance"
result: pass
source: automated
coverage_id: D3
source_summary: 01-09-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "grep -E -c 'clock_gettime|getrusage|QueryPerformance' src/*.c prints 0 for every file"

### 57. Plan 09 / D4
expected: "SIGINT or SIGTERM ends the run after the current frame, prints the hash or bench report for the frames that ran, and exits 0"
result: pass
source: automated
coverage_id: D4
source_summary: 01-09-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/runner/tests.cmake#runner.stop"

### 58. Plan 09 / D5
expected: "An unwritable --png path exits 1 with a cannot-write message"
result: pass
source: automated
coverage_id: D5
source_summary: 01-09-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "tests/runner/tests.cmake#runner.exit.failure"

### 59. Plan 09 / D6
expected: "--version prints schwaystation-run, the version and the 40-hex revision embedded at configure"
result: pass
source: automated
coverage_id: D6
source_summary: 01-09-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/runner/tests.cmake#runner.version"

### 60. Plan 09 / D7
expected: "build.revision_matches_head fails, naming both revisions, when the embedded revision differs from git rev-parse HEAD; its self-test forces forty zeros and expects exit 1 and the mismatch message"
result: pass
source: automated
coverage_id: D7
source_summary: 01-09-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/runner/tests.cmake#build.revision_matches_head"
  - "tests/runner/tests.cmake#build.revision_matches_head.self_test"

### 61. Plan 09 / D8
expected: "docs/runner.md lists every phase-1 flag with an example, the hash definition, the bench report and the exit statuses 0, 1 and 2, with no planning IDs"
result: pass
source: automated
coverage_id: D8
source_summary: 01-09-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' docs/runner.md prints 0; the doc is under 61440 bytes and mentions --bench"

### 62. Plan 09 / D9
expected: "Every runner test is inventoried and ran exactly once in the ci preset"
result: pass
source: automated
coverage_id: D9
source_summary: 01-09-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exits 0 (75 tests)"

### 63. Plan 10 / D1
expected: "libretro.host loads the built module, answers SET_SUPPORT_NO_GAME and SET_PIXEL_FORMAT, refuses content, loads with no game and runs 60 frames, each equal byte for byte to schway_host_xrgb8888_frame of a reference instance's schway_get_video"
result: pass
source: automated
coverage_id: D1
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "tests/libretro/tests.cmake#libretro.host"

### 64. Plan 10 / D2
expected: "retro_get_system_av_info reports fps 53693175.0 / 897619.0 (NTSC) or 53203425.0 / 1069484.0 (PAL) and sample_rate 44100.0, checked with exact double equality; base 320 x 240, max 640 x 480 or 640 x 576, aspect exactly 4.0f / 3.0f"
result: pass
source: automated
coverage_id: D2
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "tests/libretro/tests.cmake#libretro.host"
  - "tests/libretro/tests.cmake#libretro.host.pal"

### 65. Plan 10 / D3
expected: "No float or double in the core; the doubles and the float exist only in libretro/libretro.c"
result: pass
source: automated
coverage_id: D3
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "grep -E -c 'float|double' src/*.c src/*.h include/schwaystation.h sums to 0; grep -c double libretro/libretro.c prints 2"

### 66. Plan 10 / D4
expected: "Each retro_run forwards exactly the pairs the reference instance drains for that frame (737 or 738 NTSC, 886 or 887 PAL), through audio_batch, or pair by pair when only the single-sample callback is set"
result: pass
source: automated
coverage_id: D4
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "tests/libretro/tests.cmake#libretro.host"
  - "tests/libretro/tests.cmake#libretro.host.pal"

### 67. Plan 10 / D5
expected: "schwaystation_region registers through SET_CORE_OPTIONS_V2 when GET_CORE_OPTIONS_VERSION reports 2, else through SET_VARIABLES. It is read at load. An unchanged update keeps the instance. A changed region recreates the instance from power-on and sends SET_SYSTEM_AV_INFO once with the new rates, and retro_get_region follows"
result: pass
source: automated
coverage_id: D5
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "tests/libretro/tests.cmake#libretro.host"
  - "tests/libretro/tests.cmake#libretro.host.pal"

### 68. Plan 10 / D6
expected: "retro_reset is schway_reset POWER. retro_serialize_size equals schway_state_size, and the state matches the reference's bytes. A serialize at frame 30, run to 60, unserialize, run to 60 sequence reproduces the 60th frame and each frame's audio. A short buffer and a damaged state are refused"
result: pass
source: automated
coverage_id: D6
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "tests/libretro/tests.cmake#libretro.host"
  - "tests/libretro/tests.cmake#libretro.host.pal"

### 69. Plan 10 / D7
expected: "The module exports only retro_* symbols and links the same schwaystation archive the abi tests check"
result: pass
source: automated
coverage_id: D7
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "nm -g build/ci/libretro/schwaystation_libretro.dylib, defined T/D/B/S/C names piped to grep -v -E '^_?retro_' exits 1 (25 names, all retro_)"

### 70. Plan 10 / D8
expected: "The generated .info claims supports_no_game true, savestate false and is_experimental true, and lists no content extension; it installs to info/ and the module to cores/ in the libretro archive"
result: pass
source: automated
coverage_id: D8
source_summary: 01-10-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "grep -c 'savestate = \"false\"' build/ci/libretro/schwaystation_libretro.info prints 1; cpack --preset ci exit 0 and the libretro zip lists cores/schwaystation_libretro.dylib and info/schwaystation_libretro.info"

### 71. Plan 10 / D9
expected: "Both libretro tests are inventoried and ran exactly once in the ci preset"
result: pass
source: automated
coverage_id: D9
source_summary: 01-10-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exit 0 (77 tests ran once each)"

### 72. Plan 11 / D1
expected: "cmake --workflow --preset ci configures, builds, tests, packages and runs the ci-release preset, writing the three version-free macOS arm64 zips"
result: pass
source: automated
coverage_id: D1
source_summary: 01-11-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "cmake --workflow --preset ci exit 0 (77 tests, then 2 release tests); ls of the three build/ci/schwaystation-*-macos-arm64.zip exit 0"

### 73. Plan 11 / D2
expected: "Every release-labelled test ran exactly once, and every other inventoried test ran exactly once in the first test step"
result: pass
source: automated
coverage_id: D2
source_summary: 01-11-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake exit 0; the same with build/ci/junit.xml and -DEXCLUDE=release exit 0"

### 74. Plan 11 / D3
expected: "release.archive_contents checks each zip holds exactly its expected members with LICENSE and THIRD-PARTY-NOTICES.md at the root, byte-identical to the tree's, and no source or build path"
result: pass
source: automated
coverage_id: D3
source_summary: 01-11-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "tests/release/tests.cmake#release.archive_contents"

### 75. Plan 11 / D4
expected: "A libretro zip without THIRD-PARTY-NOTICES.md fails the archive check, naming the missing member"
result: pass
source: automated
coverage_id: D4
source_summary: 01-11-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "tests/release/tests.cmake#release.archive_contents.self_test"

### 76. Plan 11 / D5
expected: "The libretro zip lists LICENSE, THIRD-PARTY-NOTICES.md, cores/schwaystation_libretro.dylib and info/schwaystation_libretro.info"
result: pass
source: automated
coverage_id: D5
source_summary: 01-11-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "unzip -l build/ci/schwaystation-libretro-macos-arm64.zip piped to grep -E -c on the four names prints 4"

### 77. Plan 11 / D6
expected: "The single README install line installs the core and info file into a scratch HOME's RetroArch directory from a file:// base holding this leg's zip"
result: pass
source: automated
coverage_id: D6
source_summary: 01-11-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -P tests/cmake/install_line.cmake exit 0"

### 78. Plan 11 / D7
expected: "With the zip's last byte flipped the install line exits non-zero and installs nothing"
result: pass
source: automated
coverage_id: D7
source_summary: 01-11-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -DFLIP=1 -P tests/cmake/install_line.cmake exit 0 (the line exited 1)"

### 79. Plan 11 / D8
expected: "README holds exactly one line beginning mkdir -p ~/Library/Application, stays under 16384 bytes and within docs.budgets"
result: pass
source: automated
coverage_id: D8
source_summary: 01-11-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "grep -c '^mkdir -p ~/Library/Application' README.md prints 1; wc -c is 3726; ctest --test-dir build/ci -R '^docs\\.budgets$' --no-tests=error exit 0"

### 80. Plan 12 / D1
expected: "tests/retroarch holds the five e2e files, 293 lines in total, and policy.e2e_budget holds them at or under 300"
result: pass
source: automated
coverage_id: D1
source_summary: 01-12-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "tests/hygiene/tests.cmake#policy.e2e_budget"

### 81. Plan 12 / D2
expected: "The e2e budget self test fails a 301-line fixture with its named message"
result: pass
source: automated
coverage_id: D2
source_summary: 01-12-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "tests/hygiene/tests.cmake#policy.e2e_budget.self_test"

### 82. Plan 12 / D3
expected: "The compare chain the hosted job runs works on this host's images: runner PNG at frame 5, sips to BMP, compare prints 320x240, every pixel equal, and flip.cmake makes it exit 1 naming 7,3"
result: pass
source: automated
coverage_id: D3
source_summary: 01-12-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "build/ci/schwaystation-run --frames 5 --png 5:build/ci/a.png exit 0; sips --deleteColorManagementProperties and sips -s format bmp exit 0; compare a.bmp b.bmp exit 0 printing 320x240, every pixel equal; cmake -P tests/retroarch/flip.cmake exit 0 (compare exited 1 naming 7,3)"

### 83. Plan 12 / D4
expected: "ci.yml holds every lane but the rehearsal and is statically valid, with 13 jobs, COUNT=7 and FRAMES=5"
result: pass
source: automated
coverage_id: D4
source_summary: 01-12-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "actionlint -shellcheck= -pyflakes= -ignore 'label \"ubuntu-26\\.04(-arm)?\" is unknown' .github/workflows/ci.yml exit 0; yq '.jobs | keys' exit 0"

### 84. Plan 12 / D5
expected: "The six build legs, asan, nofp, be32 and CI required run green on hosted runners"
result: pass
source: automated
coverage_id: D5
source_summary: 01-12-SUMMARY.md
requirement: FRAME-01
classification_reason: validation_failed
resolution: "Later executed plan 01-14 supplied the hosted evidence; re-read run 39460774025, then current run 40929433476. Historical pending values remain historical."
verification_refs:
  - "hosted CI run of .github/workflows/ci.yml, run URL supplied by plan 01-14"

### 85. Plan 12 / D6
expected: "hash-equality compares the six legs' and be32's hashes.txt and wasm32 prints its comparison with Linux x64 report-only"
result: pass
source: automated
coverage_id: D6
source_summary: 01-12-SUMMARY.md
requirement: FRAME-02
classification_reason: validation_failed
resolution: "Later executed plan 01-14 supplied the hosted evidence; re-read run 39460774025, then current run 40929433476. Historical pending values remain historical."
verification_refs:
  - "hosted CI jobs hash-equality and wasm32, run URL supplied by plan 01-14"

### 86. Plan 12 / D7
expected: "hygiene.timeout_comments and policy.action_pins pass on .github/workflows"
result: pass
source: automated
coverage_id: D7
source_summary: 01-12-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/hygiene/tests.cmake#hygiene.timeout_comments"
  - "tests/hygiene/tests.cmake#policy.action_pins"

### 87. Plan 12 / D8
expected: "Their self tests fail a bare and a guessed timeout and an action used by tag, each with its named message"
result: pass
source: automated
coverage_id: D8
source_summary: 01-12-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "tests/hygiene/tests.cmake#hygiene.timeout_comments.self_test"
  - "tests/hygiene/tests.cmake#policy.action_pins.self_test"

### 88. Plan 12 / D9
expected: "The retroarch job runs RetroArch 1.22.2 for 5 frames and its screenshot equals the runner's frame 5"
result: pass
source: automated
coverage_id: D9
source_summary: 01-12-SUMMARY.md
requirement: FRAME-07
classification_reason: validation_failed
resolution: "Later executed plan 01-14 supplied the hosted evidence; re-read run 39460774025, then current run 40929433476. Historical pending values remain historical."
verification_refs:
  - "hosted CI job retroarch, run URL supplied by plan 01-14"

### 89. Plan 12 / D10
expected: "The perf job measures Cachegrind Ir per frame at head and merge base and reports through perf_compare.cmake"
result: pass
source: automated
coverage_id: D10
source_summary: 01-12-SUMMARY.md
requirement: FRAME-09
classification_reason: validation_failed
resolution: "Later executed plan 01-14 supplied the hosted evidence; re-read run 39460774025, then current run 40929433476. Historical pending values remain historical."
verification_refs:
  - "hosted CI job perf on a pull request, run URL supplied by plan 01-14"

### 90. Plan 13 / D1
expected: "ci.yml has rehearsal and cleanup jobs with the per-attempt tag, cleanup runs with always(), and CI required needs both, skipped only when no release input changed, outside pull requests or in a nested call"
result: pass
source: automated
coverage_id: D1
source_summary: 01-13-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "actionlint -shellcheck= -pyflakes= -ignore 'label \"ubuntu-26\\.04(-arm)?\" is unknown' .github/workflows/ci.yml exit 0; yq -e '.jobs.rehearsal and .jobs.cleanup' exit 0; /usr/bin/grep -q 'rehearsal-${{ github.run_id }}-${{ github.run_attempt }}' exit 0; yq '.jobs.cleanup.if' contains always(); yq '.jobs[\"ci-required\"].needs' lists rehearsal"

### 91. Plan 13 / D2
expected: "The rehearsal runs every publish step on a probe pull request and its cleanup leaves no rehearsal release or tag"
result: pass
source: automated
coverage_id: D2
source_summary: 01-13-SUMMARY.md
requirement: FRAME-06
classification_reason: verification_not_passing
resolution: "Re-read successful rehearsal and cleanup in run 39460774025; release workflow and publishing inputs are unchanged."
verification_refs:
  - "hosted CI jobs rehearsal and cleanup on a probe pull request, run URL supplied by plan 01-14"

### 92. Plan 13 / D3
expected: "release-please is simple on version.txt with initial-version 0.1.0 and the manifest at 0.0.0, so the first release is 0.1.0"
result: pass
source: automated
coverage_id: D3
source_summary: 01-13-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "test \"$(jq -r '.[\".\"]' .release-please-manifest.json)\" = 0.0.0 exit 0; test \"$(jq -r '.packages[\".\"][\"release-type\"]' release-please-config.json)\" = simple exit 0"

### 93. Plan 13 / D4
expected: "release.yml runs release-please with the App token, auto-merges the release pull request, calls ci.yml on release-please's sha, publishes by draft ID, attests, verifies, marks latest and runs the install line against releases/latest/download/"
result: pass
source: automated
coverage_id: D4
source_summary: 01-13-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "actionlint -shellcheck= -pyflakes= -ignore 'label \"ubuntu-26\\.04(-arm)?\" is unknown' -ignore 'unexpected key \"queue\" for \"concurrency\" section' .github/workflows/release.yml exit 0; grep -c 'releases/latest/download/' prints 1; grep -c setup-release-app prints 0"

### 94. Plan 13 / D5
expected: "nightly.yml benches 300,000 frames on the perf preset on ubuntu x64, ubuntu arm64 and macos-15, plus wasm32 under wasmtime, takes force_failure by workflow_dispatch and keeps one rolling nightly issue"
result: pass
source: automated
coverage_id: D5
source_summary: 01-13-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "actionlint (same filters) .github/workflows/nightly.yml exit 0; grep -c 'preset bench' prints 0 for nightly.yml and ci.yml"

### 95. Plan 13 / D6
expected: "Action pins and timeout comments hold across the three workflows"
result: pass
source: automated
coverage_id: D6
source_summary: 01-13-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "tests/hygiene/tests.cmake#policy.action_pins"
  - "tests/hygiene/tests.cmake#hygiene.timeout_comments"

### 96. Plan 13 / D7
expected: "docs/maintaining.md covers the release App and its history pointer, settings and the ruleset, the release and rehearsal flow, withdrawal, the nightly run and the post-release ci: pull requests, under its byte budget and with no planning ID"
result: pass
source: automated
coverage_id: D7
source_summary: 01-13-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "tests/policy/tests.cmake#docs.budgets"
  - "grep -c -i rehearsal docs/maintaining.md prints 14; grep -c -i withdraw prints 4; grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' prints 0"

### 97. Plan 14 / D1
expected: "Draft probe pull request from the phase branch with a Conventional Commit title, and a CI run on its head commit"
result: pass
source: automated
coverage_id: D1
source_summary: 01-14-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "https://github.com/szTheory/schwaystation/pull/2; the plan's Task 1 automated verify exit 0"

### 98. Plan 14 / D2
expected: "CI run on head e669f21 concludes success with CI required success; all six build legs, asan, both nofp legs and be32 pass, each with its check_junit steps"
result: pass
source: automated
coverage_id: D2
source_summary: 01-14-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329"

### 99. Plan 14 / D3
expected: "hash-equality reports 7 identical files and wasm32 reports its hashes equal Linux x64's (report only)"
result: pass
source: automated
coverage_id: D3
source_summary: 01-14-SUMMARY.md
requirement: FRAME-02
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 jobs hash-equality and wasm32"

### 100. Plan 14 / D4
expected: "Determinism and the bounded core hold on hosted runners: the build legs' tests and the nofp and be32 lanes pass"
result: pass
source: automated
coverage_id: D4
source_summary: 01-14-SUMMARY.md
requirement: FRAME-04
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 jobs nofp and be32"

### 101. Plan 14 / D5
expected: "RetroArch 1.22.2 runs the core for 5 frames and its screenshot equals the runner's frame 5 at 320x240; the flipped-byte control fails at 7,3"
result: pass
source: automated
coverage_id: D5
source_summary: 01-14-SUMMARY.md
requirement: FRAME-07
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 job retroarch"

### 102. Plan 14 / D6
expected: "The rehearsal prepares 18 archives and SHA256SUMS, attests them, publishes rehearsal-39455724329-1 with 19 assets that verify, and cleanup deletes the prerelease and tag; release.archive_contents passes on every build leg"
result: pass
source: automated
coverage_id: D6
source_summary: 01-14-SUMMARY.md
requirement: FRAME-06
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 jobs rehearsal and cleanup; gh release list printed []; git ls-remote --tags origin matched no rehearsal tag"

### 103. Plan 14 / D7
expected: "perf prints the test card's Ir per frame in report mode"
result: pass
source: automated
coverage_id: D7
source_summary: 01-14-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 job perf"

### 104. Plan 14 / D8
expected: "A forced-failure nightly dispatch opens the rolling nightly issue and the next good dispatch closes it"
result: pass
source: automated
coverage_id: D8
source_summary: 01-14-SUMMARY.md
requirement: FRAME-09
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39457445780 and https://github.com/szTheory/schwaystation/actions/runs/39457820699; https://github.com/szTheory/schwaystation/issues/3 closed"

### 105. Plan 14 / D9
expected: "The tree holds no cruft and no planning ID in code, tests, workflows or config"
result: pass
source: automated
coverage_id: D9
source_summary: 01-14-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "the plan's cruft git grep pair a=1 b=1 exit 0; the planning-ID git grep then test $? -eq 1 exit 0"

### 106. Plan 14 / D10
expected: "The libretro core and runner built and packaged on all six platforms pass their tests"
result: pass
source: automated
coverage_id: D10
source_summary: 01-14-SUMMARY.md
requirement: FRAME-03
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 build jobs"

### 107. Plan 14 / D11
expected: "The README install line installs the macOS arm64 leg's own zip and refuses a flipped copy"
result: pass
source: automated
coverage_id: D11
source_summary: 01-14-SUMMARY.md
requirement: FRAME-08
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39455724329 job build (macos-arm64) step README install line"

### 108. Plan 15 / D1
expected: "Every measured timeout in ci.yml and nightly.yml is twice the slowest successful job with a comment naming the run ID, the sample count and provisional; hygiene.timeout_comments passes and CI is green on the head commit"
result: pass
source: automated
coverage_id: D1
source_summary: 01-15-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/39460774025 (head c078b31, conclusion success); cmake --workflow --preset hygiene exit 0"

### 109. Plan 15 / D2
expected: "workflow.test_gate_timeout is twice the measured local ci workflow time, written with GSD's config writer and read back"
result: pass
source: automated
coverage_id: D2
source_summary: 01-15-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "gsd-tools query config-get workflow.test_gate_timeout printed 4, not 900 (exit 0)"

### 110. Plan 15 / D3
expected: "The DECISIONS open-question row on be32 and wasm32 per-PR cost records 36 s and 22 s from run 39455724329"
result: pass
source: automated
coverage_id: D3
source_summary: 01-15-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "grep -n 'run 39455724329' .planning/preparation/DECISIONS.md exit 0"

### 111. Plan 15 / D4
expected: "One pending todo per deferred gate, each bound to the phase that adds its first caller, plus the ship-time gate-budget re-pin from the measured 1392-line total"
result: pass
source: automated
coverage_id: D4
source_summary: 01-15-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "the plan's Task 2 todo loop (22 files with status pending, at least 18) exit 0; grep -c LIMIT .planning/todos/pending/018-repin-gate-budget-at-ship.md at least 1"

### 112. Plan 15 / D5
expected: "Probe pull request #2 is closed unmerged before ship and origin/main is unchanged"
result: pass
source: automated
coverage_id: D5
source_summary: 01-15-SUMMARY.md
requirement: FRAME-01
verification_refs:
  - "gh pr view phase/01-a-test-frame-in-retroarch --json state -q .state printed CLOSED (exit 0); git log origin/main --oneline -1 printed 82d61d2"

### 113. Plan 15 / D6
expected: "The tree holds no cruft and no planning ID in code, tests, workflows or config"
result: pass
source: automated
coverage_id: D6
source_summary: 01-15-SUMMARY.md
requirement: FRAME-05
verification_refs:
  - "the plan's cruft git grep pair a=1 b=1 exit 0; the planning-ID git grep then test $? -eq 1 exit 0"

### 114. Plan 15 / D7
expected: "The owner applied the ruleset main requiring CI required; the agent confirmed it read-only"
result: pass
source: agent_read_only
coverage_id: D7
source_summary: 01-15-SUMMARY.md
requirement: FRAME-01
classification_reason: human_judgment
resolution: "Owner applied the ruleset during execution; current read-only settings check exits 0 and the ruleset is active with CI required. No new owner action."
verification_refs:
  - "gh api repos/szTheory/schwaystation/rulesets --jq '[.[] | select(.name == \"main\")] | length' printed 1, test -ge 1 exit 0; scripts/apply-repo-settings.sh --check printed no drift, exit 0; ruleset 25491748 enforcement active with required context CI required"

### 115. current-hosted
expected: "The reviewed revision builds and packages on six platforms, exercises the state-bound regression, and matches all eight frame-hash files and the real RetroArch frame."
result: pass
source: automated
coverage_id: current-hosted
verification_refs:
  - "https://github.com/szTheory/schwaystation/actions/runs/40929433476"

### 116. state-bound
expected: "A loaded boundary state either advances to a reloadable save or refuses without changing state or output, in every region and accumulator carry case."
result: pass
source: automated
coverage_id: state-bound
verification_refs:
  - "tests/core/tests.cmake#core.state.run_bounds; cmake --workflow --preset ci, exit 0"

### 117. visual-docs
expected: "The generated frame-5 card has the documented corner, border, row strip, channel ramps, identical displayed mask twin, checker and binary counter; user docs match the implemented test-card-only release."
result: pass
source: agent_inspection
coverage_id: visual-docs
verification_refs:
  - "build/ci/schwaystation-run --frames 5 --png 5:/private/tmp/schway-uat-card.png --hash, exit 0; image viewed and docs read against code"

## Summary

total: 117
passed: 117
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

None.

## Evidence

Commands ran from the repository root. Local commands have no hosted run URL. No raw logs are committed. Scratch paths below are disposable test outputs.

| Command | Exit | Run URL |
|---|---|---|
| `cmake --workflow --preset ci` | 0 | Not applicable (local) |
| `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` | 0 | Not applicable (local) |
| `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake` | 0 | Not applicable (local) |
| `cmake --workflow --preset hygiene` | 0 | Not applicable (local) |
| `sh scripts/apply-repo-settings.sh --check` | 0 | Not applicable (local) |
| `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -DLABELS=hygiene -P tests/cmake/check_junit.cmake` | 0 | Not applicable (local) |
| `cmake -DREADME=README.md -DBASE_URL=file://${PWD}/build/ci/ -DHOME_DIR=/private/tmp/schway-uat-install -P tests/cmake/install_line.cmake` | 0 | Not applicable (local) |
| `cmake -DREADME=README.md -DBASE_URL=file://${PWD}/build/ci/ -DHOME_DIR=/private/tmp/schway-uat-install -DFLIP=1 -P tests/cmake/install_line.cmake` | 0 | Not applicable (local) |
| `build/ci/schwaystation-run --frames 5 --png 5:/private/tmp/schway-uat-card.png --hash` | 0 | Not applicable (local) |
| `build/ci/schwaystation-run --bench 60` | 0 | Not applicable (local) |
| `nm -gU build/ci/libretro/schwaystation_libretro.dylib` | 0 | Not applicable (local) |
| `env XDG_CACHE_HOME=/private/tmp/schway-gh-cache gh run view 39460774025 --json conclusion,headSha,url,jobs --jq '{conclusion,headSha,url,jobs:[.jobs[]\|{name,conclusion}]}'` | 0 | https://github.com/szTheory/schwaystation/actions/runs/39460774025 |
| `gh run view 39457820699 --json conclusion,headSha,url` | 0 | https://github.com/szTheory/schwaystation/actions/runs/39457820699 |
| `gh issue view 3 --json state,url` | 0 | https://github.com/szTheory/schwaystation/issues/3 |
| `gh pr view 2 --json state,mergedAt,url` | 0 | https://github.com/szTheory/schwaystation/pull/2 |
| `gh api repos/szTheory/schwaystation/rulesets/25491748 --jq '[.enforcement, (.rules[] \| select(.type=="required_status_checks") \| .parameters.required_status_checks[].context)]'` | 0 | Not applicable (local) |
| `env XDG_CACHE_HOME=/private/tmp/schway-gh-cache gh run view 39460774025 --log` | 0 | https://github.com/szTheory/schwaystation/actions/runs/39460774025 |
| `gh api repos/szTheory/schwaystation/check-runs/123643713395/annotations --jq '.[] \| {annotation_level,message}'` | 0 | https://github.com/szTheory/schwaystation/actions/runs/40929433476 |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=1500 -P tests/cmake/line_budget.cmake` | 0 | Not applicable (local) |
| `shasum -a 256 libretro/libretro.h` | 0 | Not applicable (local) |
| `gh api repos/szTheory/cogmaster/commits/f09ac97096acda99888270e4c1e4dc96e12ffdfe --jq .sha` | 0 | Not applicable (local) |
| `gh api repos/szTheory/muhtari/commits/d698364b180df42894ef6737179d4ed9fcc97817 --jq .sha` | 0 | Not applicable (local) |
| `gh api repos/libretro/libretro-common/commits/a3194c18ba65ef89734e1898a2f61403c255d368 --jq .sha` | 0 | Not applicable (local) |
| `gh run view 40929433476 --json status,conclusion,headSha,url,jobs --jq '{status,conclusion,headSha,url,jobs:[.jobs[]\|{name,conclusion}]}'` | 0 | https://github.com/szTheory/schwaystation/actions/runs/40929433476 |
| `env XDG_CACHE_HOME=/private/tmp/schway-gh-cache gh run view 40929433476 --log` | 0 | https://github.com/szTheory/schwaystation/actions/runs/40929433476 |

## Carry-overs and Ship Boundary

- Existing tooling seeds remain seeds: Debug ABI helpers (todo 019), the CI documentation table (020), actionlint configuration (021), the intermittent Windows DLL consumer (022), and the test-helper exit-status warning (023, bound to phase 2 plan 1).
- Current hosted run 40929433476 passes at reviewed revision 384ee9d6983b700c15c0c52c3801df9ca3a4dc86. Attempt 1 exceeded the Windows ARM64 three-minute job timeout after all its build, test, package and upload steps succeeded; the annotation was inspected, and the affected job and downstream checks pass in attempt 2. No code, assertion or workflow changed. Todo 024 carries the timing sample into the existing phase-1 CI tail.
- Ship re-measures and ratchets the gate-line limit (todo 018), opens the phase PR, and confirms its CI and the automatic release PR. The real release and latest-download install are merge-only outcomes, not UAT tasks.
- The separate numbered phase-1 CI tail arms the performance gate and promotes wasm32 after release. The phase-1 seven-file required comparison plus report-only wasm32 comparison is intentional.
- Next command: `$gsd-ship 1`. No further GSD step runs in this session.
