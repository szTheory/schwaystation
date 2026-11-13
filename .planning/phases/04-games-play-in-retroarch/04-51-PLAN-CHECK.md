# Phase 04 focused plan check: SPU ATTR readback

## VERIFICATION PASSED

**Checked:** new plan 04-51 and the 04-40 dependency/wave change at `09ee539`, against parent `e826856`.

The committed boot trace shows OpenBIOS writing `0020h` to ATTR at physical `1F801DAAh`, then repeatedly reading zero there while polling bits `5–4`. The pinned source maps `SPU_CTRL` to a 16-bit register at DAA; the project’s PSX-SPX source notes document all 16 ATTR control bits, the DMA-write mode, and the distinct delayed status register at DAE. The plan therefore models only an exact halfword readback latch. It claims no SPU/audio behavior, transfer side effects, hardware power-on value, or register timing beyond retaining the existing bus cost.

The existing bus is instance-owned and already supplies the SPU unbuilt path, reset routine, port-cost calculation, and state visitor. The planned `uint16_t` field is added to that bus, reset to a deterministic emulator zero, decoded only for physical DAA/width 2, and serialized by the existing little-endian `schway__state_u16_array` validate/apply walk. Other widths and addresses, including STATX, keep their current unbuilt counts and costs. Regression scope covers aliases, read-after-write, the `0020h` poll mask, isolation, reset, save/load across same-region instances, unsupported widths/neighbors, and the independent ownership sweep. Existing state refusal tests remain in full CI. Registration reuses the 10-second bus test target and sorted inventory; state and fixture tests reuse existing registrations. Only affected interim-v4 state hashes are candidates for update; frame/audio pins and frozen API v1–v3 remain protected. No new API, state version, module, global state, gate, quota, or owner step is introduced.

Plan 04-40 remains responsible for rerunning and inspecting the actual boot, then satisfying the real shell and same-run portable-frame acceptance. The diagnosis records that the post-POST frame is still black; 04-51 claims no shell progress. The dependency places completed plans 04-23, 04-39, and 04-50 before 04-51 in wave 26; 04-40 moves to wave 27 and depends on 04-51. The later incomplete waves move by one; completed work remains intact. Native plan-structure checks pass, and the roadmap lists 04-51 as the only open plan in wave 26.

**Sizing judgment:** the 15–25 minute estimate covers one cohesive source/state/test/documentation behavior. It uses one focused linked check and one CI/hygiene closeout, with no repeated suite for documentation-only work. The state field, ownership sweep, and two existing state-hash rows are substantive parts of this slice, not separate evidence or ceremony. No further split is warranted.

**Scope retention:** the boot chain remains `04-23 → 04-50 → 04-51 → 04-40`. ATTR readback only removes the observed poll blocker; shell inspection and portable acceptance remain pending in 04-40. No actionable issue found.
