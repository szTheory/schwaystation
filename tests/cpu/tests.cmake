# CPU exception, branch-delay and timing behaviour.
# COP0 and the exception entry through the CPU's one-instruction entry. One
# executable runs the case named on its command line. It also sees src/, so it
# can place words in RAM, step the CPU and read its registers and the fault
# record.
add_executable(schwaystation-test-cop0 ${CMAKE_CURRENT_LIST_DIR}/cop0_tests.c)
target_include_directories(schwaystation-test-cop0 PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cop0 PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cop0 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cop0)

# Fault prevented: a delay-slot exception whose EPC names the slot, or whose BD,
# BT or TAR is wrong, or that leaves the pending branch alive.
schway_add_test(NAME cpu.cop0.entry_delay_slot
  COMMAND schwaystation-test-cop0 entry_delay_slot
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a mode stack that is not pushed, a changed rd after a trap, or a BadVaddr or TAR written by an exception that is neither an address error nor in a delay slot.
schway_add_test(NAME cpu.cop0.entry_plain
  COMMAND schwaystation-test-cop0 entry_plain
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an SR bit the CPU does not have that a guest can set.
schway_add_test(NAME cpu.cop0.sr_mask
  COMMAND schwaystation-test-cop0 sr_mask
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a CAUSE write that changes more than the two software interrupt bits, or a read-only COP0 register that a guest can overwrite.
schway_add_test(NAME cpu.cop0.cause_writable
  COMMAND schwaystation-test-cop0 cause_writable
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a return from exception that loses the previous pair or moves the program counter.
schway_add_test(NAME cpu.cop0.rfe
  COMMAND schwaystation-test-cop0 rfe
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a coprocessor-error field left over from another exception, or a BadVaddr that follows every exception.
schway_add_test(NAME cpu.cop0.ce_only_cpu
  COMMAND schwaystation-test-cop0 ce_only_cpu
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a handler that is never entered because the vector ignores SR.BEV.
schway_add_test(NAME cpu.cop0.bev_vectors
  COMMAND schwaystation-test-cop0 bev_vectors
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a missing coprocessor that executes as if it were there, or a first-fault record left empty.
schway_add_test(NAME cpu.cop0.cop1_cop3_unusable
  COMMAND schwaystation-test-cop0 cop1_cop3_unusable
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a coprocessor branch that falls through where it must go, or the reverse.
schway_add_test(NAME cpu.cop0.cop_branches
  COMMAND schwaystation-test-cop0 cop_branches
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an undefined opcode, or a COP0 register number the CPU does not have, that retires as a no-op.
schway_add_test(NAME cpu.cop0.reserved_instruction
  COMMAND schwaystation-test-cop0 reserved_instruction
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a first fault recorded for SYSCALL, BREAK, overflow or a COP0 CpU, which hides the first real fault.
schway_add_test(NAME cpu.cop0.no_fault_for_traps
  COMMAND schwaystation-test-cop0 no_fault_for_traps
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a guest spinning in its own handler that is never reported, or a loop counter that is never reset.
schway_add_test(NAME cpu.cop0.exception_loop
  COMMAND schwaystation-test-cop0 exception_loop
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a COP0 read visible one instruction early, or landing over a newer write to its register.
schway_add_test(NAME cpu.cop0.mfc0_delay
  COMMAND schwaystation-test-cop0 mfc0_delay
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a call that loses its return address when the branch is not taken.
schway_add_test(NAME cpu.cop0.branch_links
  COMMAND schwaystation-test-cop0 branch_links
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a jump to an odd address that executes the word the truncated address names.
schway_add_test(NAME cpu.cop0.jr_misaligned
  COMMAND schwaystation-test-cop0 jr_misaligned
  LABELS cpu portable TIMEOUT 10)

# COP2 before the GTE: one executable runs the case named on its command line.
add_executable(schwaystation-test-cop2 ${CMAKE_CURRENT_LIST_DIR}/cop2_prephase.c)
target_include_directories(schwaystation-test-cop2 PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cop2 PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cop2 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cop2)

# Fault prevented: a missing GTE that answers as if it were there, a first fault recorded for a CpU a guest handler can take, or a store that reaches RAM before the coprocessor check.
schway_add_test(NAME cpu.cop2_prephase.cu2_clear
  COMMAND schwaystation-test-cop2 cu2_clear
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a register move that lands at once or never, a GTE access that is not counted, a load or store that skips the bus, or a coprocessor check that hides an alignment error.
schway_add_test(NAME cpu.cop2_prephase.cu2_set
  COMMAND schwaystation-test-cop2 cu2_set
  LABELS cpu portable TIMEOUT 10)

# Exceptions and interrupts: one executable runs the case named on its command line.
add_executable(schwaystation-test-exception ${CMAKE_CURRENT_LIST_DIR}/exception_tests.c)
target_include_directories(schwaystation-test-exception PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-exception PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-exception PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-exception)

# Fault prevented: an interrupt that is never taken, one taken at the edge with no latency, or one that returns past an instruction.
schway_add_test(NAME cpu.exception.irq_latency_device
  COMMAND schwaystation-test-exception irq_latency_device
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a misaligned half-word read that returns data, or one whose handler reads the wrong BadVaddr.
schway_add_test(NAME cpu.exception.adel_lh
  COMMAND schwaystation-test-exception adel_lh
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a word read at 2 mod 4 that is treated as aligned.
schway_add_test(NAME cpu.exception.adel_lw
  COMMAND schwaystation-test-exception adel_lw
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a misaligned store that lands in RAM before it traps.
schway_add_test(NAME cpu.exception.ades_sw
  COMMAND schwaystation-test-exception ades_sw
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a wrapped sum written to rd, or a first fault that hides the real one.
schway_add_test(NAME cpu.exception.overflow_add
  COMMAND schwaystation-test-exception overflow_add
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an undefined encoding that retires as a no-operation.
schway_add_test(NAME cpu.exception.reserved_instruction
  COMMAND schwaystation-test-exception reserved_instruction
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a missing coprocessor that executes, or a CE that names the wrong one.
schway_add_test(NAME cpu.exception.cop1_unusable
  COMMAND schwaystation-test-exception cop1_unusable
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an EPC for SYSCALL that skips it, or a BD or first fault left over.
schway_add_test(NAME cpu.exception.syscall_epc
  COMMAND schwaystation-test-exception syscall_epc
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a SYSCALL in a delay slot whose EPC names the slot, so the branch is lost.
schway_add_test(NAME cpu.exception.syscall_bd
  COMMAND schwaystation-test-exception syscall_bd
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an EPC for BREAK that skips it, or a BD or first fault left over.
schway_add_test(NAME cpu.exception.break_epc
  COMMAND schwaystation-test-exception break_epc
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a BREAK in a delay slot whose EPC names the slot, so the branch is lost.
schway_add_test(NAME cpu.exception.break_bd
  COMMAND schwaystation-test-exception break_bd
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an interrupt taken in spite of IEc, IM2 or I_MASK, or one armed by reading a register.
schway_add_test(NAME cpu.exception.irq_masked_sr
  COMMAND schwaystation-test-exception irq_masked_sr
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a handler that is never entered because the vector ignores BEV, for an interrupt as for an instruction exception.
schway_add_test(NAME cpu.exception.bev_vectors
  COMMAND schwaystation-test-exception bev_vectors
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a read from nowhere that returns zero without a trap, or one that writes BadVaddr.
schway_add_test(NAME cpu.exception.dbe_unmapped_read
  COMMAND schwaystation-test-exception dbe_unmapped_read
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a fetch from nowhere that executes zeros, or reports a data error.
schway_add_test(NAME cpu.exception.ibe_unmapped_fetch
  COMMAND schwaystation-test-exception ibe_unmapped_fetch
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an interrupt that hides a data error or an overflow, or one that a SYSCALL, a reserved instruction or a fetch error hides.
schway_add_test(NAME cpu.exception.priority
  COMMAND schwaystation-test-exception priority
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an interrupt entry that drops or reorders queued stores, or leaves them queued behind an uncached read.
schway_add_test(NAME cpu.exception.irq_with_queued_stores
  COMMAND schwaystation-test-exception irq_with_queued_stores
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a hook that executes a GTE command in a build with no GTE, or reports the wrong words.
schway_add_test(NAME cpu.exception.irq_entry_cop2_hook
  COMMAND schwaystation-test-exception irq_entry_cop2_hook
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a CAUSE write whose interrupt is taken at the write, or two instructions on.
schway_add_test(NAME cpu.exception.doc_cause_write
  COMMAND schwaystation-test-exception doc_cause_write
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an SR write whose pending interrupt is taken at once, or never.
schway_add_test(NAME cpu.exception.doc_sr_write
  COMMAND schwaystation-test-exception doc_sr_write
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an I_MASK write whose interrupt is taken at the store, or ignored.
schway_add_test(NAME cpu.exception.doc_imask_write
  COMMAND schwaystation-test-exception doc_imask_write
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an interrupt that returns into the middle of a branch, or sets BD for an instruction that is no delay slot any more.
schway_add_test(NAME cpu.exception.doc_cause_in_delay_slot
  COMMAND schwaystation-test-exception doc_cause_in_delay_slot
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: a return from exception that runs on with an interrupt it should take, or never takes it.
schway_add_test(NAME cpu.exception.irq_rfe
  COMMAND schwaystation-test-exception irq_rfe
  LABELS cpu portable TIMEOUT 10)

# Fault prevented: an interrupt taken for a source the program had already cleared.
schway_add_test(NAME cpu.exception.irq_dropped_before_entry
  COMMAND schwaystation-test-exception irq_dropped_before_entry
  LABELS cpu portable TIMEOUT 10)
