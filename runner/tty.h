/* The runner emits only printable ASCII and tab from guest TTY text. */
#ifndef SCHWAYSTATION_RUNNER_TTY_H
#define SCHWAYSTATION_RUNNER_TTY_H

#include <stdint.h>

static inline uint8_t schway__runner_tty_byte(uint8_t byte)
{
    return (byte == 0x09u || (byte >= 0x20u && byte < 0x7Fu)) ? byte : (uint8_t)'?';
}

#endif /* SCHWAYSTATION_RUNNER_TTY_H */
