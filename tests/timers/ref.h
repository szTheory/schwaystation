/* A naive model of the three root counters, stepped one cycle at a time, for
   the random comparison with the lazy model in timer_tests.c. Offsets are the
   register offsets 0 (COUNT), 4 (MODE) and 8 (TARGET). */
#ifndef SCHWAYSTATION_TESTS_TIMERS_REF_H
#define SCHWAYSTATION_TESTS_TIMERS_REF_H

#include <stdint.h>

void ref_reset(void);
/* Time passes by cycles system clocks. */
void ref_run(uint32_t cycles);
void ref_write(uint32_t n, uint32_t reg, uint32_t value);
uint32_t ref_read(uint32_t n, uint32_t reg);
/* The interrupt bits 4 to 6 the counters have raised, in place. */
uint32_t ref_istat(void);
/* Clears the bits of mask (in place) the way an acknowledgement does. */
void ref_ack(uint32_t mask);
/* The beam's signals: a level of 0 or 1, and a number of dot clocks. */
void ref_vblank(uint32_t level);
void ref_hblank(uint32_t level);
void ref_dots(uint32_t dots);

#endif /* SCHWAYSTATION_TESTS_TIMERS_REF_H */
