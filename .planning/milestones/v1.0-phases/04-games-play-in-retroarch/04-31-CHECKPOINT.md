# Plan 31 implementation checkpoint

Status: implementation integrated; canonical plan acceptance remains pending after 48. No SUMMARY or counter advance. Runtime/model: Codex primary (GPT-6 session); off-tree worker gpt-6-luna xhigh. Root performed the safe sole-writer integration after 58 returned terminal.

Files: tests/fuzz/gpu.c, sio.c, memcard.c, seeds.c, seeds.h, tests.cmake and inventory.txt. The unchanged loader corpus/replay is preserved. New deterministic GPU, SIO and card aggregates are registered portable cases and are included by existing CI, ASan and wasm32 inventory directories; BE32 selects portable.

GPU replays independently check continued restored packets, exact pixels and adjacent 1023/1024 rejection, 13+19 versus 32-cycle state/VRAM equality, transfers, resets, DMA request edges and instance ownership. SIO/card cover scheduled ACK/byte timing, watchdog, abort/empty FIFO/read pause, last/invalid sectors, staged/committed bytes, checksum refusal and generations. This extends required coverage with existing replay infrastructure and no new dependency or core behavior.

Verification from repository root:

- cmake --preset ci: exit 0.
- cmake --build --preset ci --target schwaystation-fuzz-gpu-replay schwaystation-fuzz-sio-replay schwaystation-fuzz-memcard-replay schwaystation-fuzz-exe-replay --parallel 4: initial strict build found missing LLVM entry-point prototypes, then exit 0 after explicit declarations matching the existing loader pattern.
- ctest --test-dir build/ci -R '^fuzz\.regress\.(gpu|sio|memcard|exe)$' --no-tests=error --output-on-failure --parallel 4: exit 0, all four registered cases execute. Command-produced result is retained temporarily under /private/tmp/phase4-fuzz-candidate/registered-result.json; no raw log committed.
- CTest JSON confirms all three new cases carry portable; ci/asan/wasm32 inventories include fuzz. This is registration evidence, not hosted sanitizer/foreign-architecture execution.
- git diff --check: exit 0.

Next: preserve this passing implementation during 47/48; run affected replays if their underlying core changes, then close 31 with source-appropriate shared full-suite/hygiene and actual hosted lane evidence. Metadata-only closeout does not justify repeating an unchanged broad suite. Do not mark GAME-01/03 complete from these cases alone.
