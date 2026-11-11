/* SIO0 byte timing follows the peripheral preparation document, section one
   (.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md). */
#include <string.h>

#include "internal.h"

#include "state.h"

#define SIO_BASE 0x1F801040u
#define SIO_DATA (SIO_BASE + 0x00u)
#define SIO_STAT (SIO_BASE + 0x04u)
#define SIO_MODE (SIO_BASE + 0x08u)
#define SIO_CTRL (SIO_BASE + 0x0Au)
#define SIO_BAUD (SIO_BASE + 0x0Eu)
#define CTRL_TXEN 0x0001u
#define CTRL_SELECT 0x0002u
#define CTRL_RXEN 0x0004u
#define CTRL_ACK 0x0010u
#define CTRL_RESET 0x0040u
#define CTRL_RX_IRQ 0x0800u
#define CTRL_ACK_IRQ 0x1000u
#define CTRL_PORT2 0x2000u
#define CTRL_MASK 0x3F57u
#define MODE_MASK 0x013Fu
#define STAT_RX_READY 0x00000002u
#define STAT_ACK_LOW 0x00000080u
#define STAT_IRQ 0x00000200u
#define PAD_ACK_DELAY 338u
#define PAD_ACK_HOLD 68u
#define STATE_EVENT_AHEAD UINT64_C(40000000)

static const char name_mode[] = "sio.mode";
static const char name_ctrl[] = "sio.ctrl";
static const char name_baud[] = "sio.baud";
static const char name_rx[] = "sio.rx";
static const char name_rx_head[] = "sio.rx_head";
static const char name_rx_count[] = "sio.rx_count";
static const char name_last_rx[] = "sio.last_rx";
static const char name_tx_data[] = "sio.tx_data";
static const char name_tx_port[] = "sio.tx_port";
static const char name_tx_pending[] = "sio.tx_pending";
static const char name_ack_pending[] = "sio.ack_pending";
static const char name_ack_low[] = "sio.ack_low";
static const char name_parity[] = "sio.parity_error";
static const char name_irq[] = "sio.irq";
static const char name_byte_deadline[] = "sio.byte_deadline";
static const char name_ack_deadline[] = "sio.ack_deadline";
static const char name_release_deadline[] = "sio.release_deadline";

static uint16_t state_u16(struct schway__state_io *io, const char *name, uint16_t *value,
                          uint16_t min, uint16_t max)
{
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, value, 1u, 2u);
        return *value;
    }
    uint16_t result = (uint16_t)schway__state_field_io(io, name, *value, 2u, min, max);
    if (io->mode == SCHWAY__STATE_APPLY) {
        *value = result;
    }
    return result;
}

static uint8_t state_u8(struct schway__state_io *io, const char *name, uint8_t *value,
                        uint8_t min, uint8_t max)
{
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, value, 1u, 1u);
        return *value;
    }
    uint8_t result = (uint8_t)schway__state_field_io(io, name, *value, 1u, min, max);
    if (io->mode == SCHWAY__STATE_APPLY) {
        *value = result;
    }
    return result;
}

static uint64_t deadline_after(uint64_t now, uint64_t delta)
{
    return now > SCHWAY__STATE_TICKS_MAX - delta ? SCHWAY__STATE_TICKS_MAX : now + delta;
}

static uint64_t byte_cycles(const struct schway__sio *sio)
{
    uint32_t factor = 1u;
    if ((sio->mode & 3u) == 2u) {
        factor = 16u;
    } else if ((sio->mode & 3u) == 3u) {
        factor = 64u;
    }
    uint32_t divisor = ((uint32_t)sio->baud * factor) & ~1u;
    if (divisor == 0u) {
        divisor = 1u;
    }
    uint32_t bits = 5u + ((sio->mode >> 2) & 3u);
    if ((sio->mode & 0x10u) != 0u) {
        bits++;
    }
    return (uint64_t)divisor * bits;
}

static uint32_t register16_read(uint16_t reg, uint32_t base, uint32_t addr, uint32_t width,
                                uint32_t *value)
{
    if ((width != 1u && width != 2u) || addr < base || addr + width > base + 2u ||
        (width == 2u && addr != base)) {
        return 1u;
    }
    uint32_t shift = (addr - base) * 8u;
    uint32_t mask = width == 1u ? 0xFFu : 0xFFFFu;
    *value = ((uint32_t)reg >> shift) & mask;
    return 0u;
}

static uint32_t register16_write(uint16_t *reg, uint32_t base, uint32_t addr, uint32_t width,
                                 uint32_t value)
{
    if ((width != 1u && width != 2u) || addr < base || addr + width > base + 2u ||
        (width == 2u && addr != base)) {
        return 1u;
    }
    uint32_t shift = (addr - base) * 8u;
    uint32_t mask = (width == 1u ? 0xFFu : 0xFFFFu) << shift;
    *reg = (uint16_t)(((uint32_t)*reg & ~mask) | ((value << shift) & mask));
    return 0u;
}

static uint32_t rx_preview(const struct schway__sio *sio, uint32_t width)
{
    uint32_t count = width == 4u ? 4u : 1u;
    uint32_t value = 0u;
    for (uint32_t i = 0u; i < count; i++) {
        uint8_t byte = i < sio->rx_count ? sio->rx[(sio->rx_head + i) % SCHWAY__SIO_RX_CAP] : 0u;
        if (i == 0u && sio->rx_count == 0u) {
            byte = sio->last_rx;
        }
        value |= (uint32_t)byte << (8u * i);
    }
    return value;
}

static uint32_t status(const struct schway__sio *sio)
{
    uint32_t value = sio->tx_pending == 0u ? 5u : 0u;
    if (sio->rx_count != 0u) {
        value |= STAT_RX_READY;
    }
    if (sio->parity_error != 0u) {
        value |= 1u << 3;
    }
    if (sio->ack_low != 0u) {
        value |= STAT_ACK_LOW;
    }
    if (sio->irq != 0u) {
        value |= STAT_IRQ;
    }
    return value;
}

static int peek_reg(const schway *inst, uint32_t addr, uint32_t width, uint32_t *value)
{
    const struct schway__sio *sio = &inst->sio;
    if (addr == SIO_DATA && (width == 1u || width == 2u || width == 4u)) {
        *value = rx_preview(sio, width);
        return 0;
    }
    if (addr >= SIO_STAT && addr + width <= SIO_STAT + 4u &&
        (width == 1u || width == 2u || width == 4u) && addr % width == 0u) {
        *value = (status(sio) >> ((addr - SIO_STAT) * 8u)) &
                 (width == 1u ? 0xFFu : width == 2u ? 0xFFFFu : UINT32_MAX);
        return 0;
    }
    if (addr >= SIO_MODE && addr < SIO_MODE + 2u) {
        return (int)register16_read(sio->mode, SIO_MODE, addr, width, value);
    }
    if (addr >= SIO_CTRL && addr < SIO_CTRL + 2u) {
        return (int)register16_read(sio->ctrl, SIO_CTRL, addr, width, value);
    }
    if (addr >= SIO_BAUD && addr < SIO_BAUD + 2u) {
        return (int)register16_read(sio->baud, SIO_BAUD, addr, width, value);
    }
    return 1;
}

void schway__sio_reset(schway *inst)
{
    memset(&inst->sio, 0, sizeof inst->sio);
}

void schway__sio_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks)
{
    struct schway__sio *sio = &inst->sio;
    (void)state_u16(io, name_mode, &sio->mode, 0u, MODE_MASK);
    (void)state_u16(io, name_ctrl, &sio->ctrl, 0u, CTRL_MASK);
    (void)state_u16(io, name_baud, &sio->baud, 0u, UINT16_MAX);
    schway__state_u8_array(io, name_rx, sio->rx, SCHWAY__SIO_RX_CAP, 0u, UINT8_MAX);
    (void)state_u8(io, name_rx_head, &sio->rx_head, 0u, SCHWAY__SIO_RX_CAP - 1u);
    uint8_t rx_count = state_u8(io, name_rx_count, &sio->rx_count, 0u, SCHWAY__SIO_RX_CAP);
    (void)state_u8(io, name_last_rx, &sio->last_rx, 0u, UINT8_MAX);
    (void)state_u8(io, name_tx_data, &sio->tx_data, 0u, UINT8_MAX);
    (void)state_u8(io, name_tx_port, &sio->tx_port, 0u, 1u);
    uint8_t tx_pending = state_u8(io, name_tx_pending, &sio->tx_pending, 0u, 1u);
    uint8_t ack_pending = state_u8(io, name_ack_pending, &sio->ack_pending, 0u, 1u);
    uint8_t ack_low = state_u8(io, name_ack_low, &sio->ack_low, 0u, 1u);
    (void)state_u8(io, name_parity, &sio->parity_error, 0u, 1u);
    (void)state_u8(io, name_irq, &sio->irq, 0u, 1u);
    uint64_t byte_deadline = schway__state_u64(io, name_byte_deadline, &sio->byte_deadline, 0u,
                                               SCHWAY__STATE_TICKS_MAX);
    uint64_t ack_deadline = schway__state_u64(io, name_ack_deadline, &sio->ack_deadline, 0u,
                                              SCHWAY__STATE_TICKS_MAX);
    uint64_t release_deadline = schway__state_u64(io, name_release_deadline,
                                                  &sio->release_deadline, 0u,
                                                  SCHWAY__STATE_TICKS_MAX);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        uint64_t max = ticks > SCHWAY__STATE_TICKS_MAX - STATE_EVENT_AHEAD
                           ? SCHWAY__STATE_TICKS_MAX
                           : ticks + STATE_EVENT_AHEAD;
        schway__state_check(io, rx_count <= SCHWAY__SIO_RX_CAP);
        schway__state_check(io, (tx_pending != 0u && byte_deadline >= ticks &&
                                  byte_deadline <= max) ||
                                 (tx_pending == 0u && byte_deadline == 0u));
        schway__state_check(io, (ack_pending != 0u && ack_deadline >= ticks &&
                                  ack_deadline <= max) ||
                                 (ack_pending == 0u && ack_deadline == 0u));
        schway__state_check(io, (ack_low != 0u && release_deadline >= ticks &&
                                  release_deadline <= max) ||
                                 (ack_low == 0u && release_deadline == 0u));
        schway__state_check(io, !(ack_pending != 0u && ack_low != 0u));
    }
}

int schway__sio_peek(const schway *inst, uint32_t addr, uint32_t width, uint32_t *value)
{
    if (inst == NULL || value == NULL || (width != 1u && width != 2u && width != 4u)) {
        return 1;
    }
    return peek_reg(inst, addr, width, value);
}

uint64_t schway__sio_next(const schway *inst)
{
    const struct schway__sio *sio = &inst->sio;
    uint64_t next = UINT64_MAX;
    if (sio->tx_pending != 0u && sio->byte_deadline < next) {
        next = sio->byte_deadline;
    }
    if (sio->ack_pending != 0u && sio->ack_deadline < next) {
        next = sio->ack_deadline;
    }
    if (sio->ack_low != 0u && sio->release_deadline < next) {
        next = sio->release_deadline;
    }
    return next;
}

static void raise_irq(schway *inst)
{
    inst->sio.irq = 1u;
    schway__irq_raise(inst, 7u);
}

static void rx_push(schway *inst, uint8_t value)
{
    struct schway__sio *sio = &inst->sio;
    sio->last_rx = value;
    if (sio->rx_count >= SCHWAY__SIO_RX_CAP) {
        sio->parity_error = 1u;
        return;
    }
    sio->rx[(sio->rx_head + sio->rx_count) % SCHWAY__SIO_RX_CAP] = value;
    sio->rx_count++;
    uint32_t depth_code = (sio->ctrl >> 8) & 3u;
    uint32_t depth = 1u << depth_code;
    if ((sio->ctrl & CTRL_RX_IRQ) != 0u && sio->rx_count >= depth) {
        raise_irq(inst);
    }
}

void schway__sio_event(schway *inst, uint64_t now)
{
    struct schway__sio *sio = &inst->sio;
    if (sio->tx_pending != 0u && sio->byte_deadline <= now) {
        uint8_t received = 0xFFu;
        uint32_t ack = 0u;
        uint32_t card_ack = 0u;
        uint64_t period = byte_cycles(sio);
        uint64_t tx_start = sio->byte_deadline >= period ? sio->byte_deadline - period : 0u;
        if ((sio->ctrl & CTRL_SELECT) != 0u) {
            uint32_t card_result = schway__card_byte(inst, sio->tx_port, sio->tx_data, &received);
            if (card_result == SCHWAY__CARD_BYTE_ACK) {
                ack = 1u;
                card_ack = SCHWAY__CARD_ACK_FROM_TX;
            } else if (card_result == SCHWAY__CARD_BYTE_READ_PAUSE) {
                ack = 1u;
                card_ack = SCHWAY__CARD_READ_PAUSE_FROM_TX;
            } else if (card_result == SCHWAY__CARD_BYTE_UNHANDLED) {
                ack = schway__pad_byte(inst, sio->tx_port, sio->tx_data, now, &received);
            }
        }
        sio->byte_deadline = 0u;
        sio->tx_pending = 0u;
        uint32_t rx_enabled = (sio->ctrl & CTRL_RXEN) != 0u ||
                              (sio->ctrl & CTRL_SELECT) != 0u;
        if (rx_enabled != 0u) {
            rx_push(inst, received);
        }
        if ((sio->ctrl & 0x0400u) != 0u) {
            raise_irq(inst);
        }
        if ((sio->ctrl & CTRL_RXEN) != 0u) {
            sio->ctrl &= (uint16_t)(UINT16_MAX ^ CTRL_RXEN);
        }
        if (ack != 0u) {
            sio->ack_pending = 1u;
            if (card_ack != 0u) {
                sio->ack_deadline = deadline_after(tx_start, card_ack);
                if (sio->ack_deadline < now) {
                    sio->ack_deadline = now;
                }
            } else {
                sio->ack_deadline = deadline_after(now, PAD_ACK_DELAY);
            }
        }
    }
    if (sio->ack_pending != 0u && sio->ack_deadline <= now) {
        sio->ack_pending = 0u;
        sio->ack_deadline = 0u;
        sio->ack_low = 1u;
        sio->release_deadline = deadline_after(now, PAD_ACK_HOLD);
        if ((sio->ctrl & CTRL_ACK_IRQ) != 0u) {
            raise_irq(inst);
        }
    }
    if (sio->ack_low != 0u && sio->release_deadline <= now) {
        sio->ack_low = 0u;
        sio->release_deadline = 0u;
    }
}

static void rx_pop(struct schway__sio *sio, uint32_t width)
{
    uint32_t count = width == 4u ? 4u : 1u;
    if (count > sio->rx_count) {
        count = sio->rx_count;
    }
    sio->rx_head = (uint8_t)((sio->rx_head + count) % SCHWAY__SIO_RX_CAP);
    sio->rx_count = (uint8_t)(sio->rx_count - count);
}

static int write_ctrl(schway *inst, uint32_t addr, uint32_t width, uint32_t value)
{
    struct schway__sio *sio = &inst->sio;
    uint16_t next = sio->ctrl;
    if (register16_write(&next, SIO_CTRL, addr, width, value) != 0u) {
        return 1;
    }
    next = (uint16_t)(next & CTRL_MASK);
    if ((next & CTRL_RESET) != 0u) {
        schway__sio_reset(inst);
        schway__pad_abort(inst, 2u);
        schway__card_abort(inst, 2u);
        schway__sched_recalc(inst);
        return 0;
    }
    uint32_t old_select = sio->ctrl & (CTRL_SELECT | CTRL_PORT2);
    uint32_t new_select = next & (CTRL_SELECT | CTRL_PORT2);
    if (old_select != new_select && (sio->ctrl & CTRL_SELECT) != 0u) {
        uint32_t port = (sio->ctrl & CTRL_PORT2) != 0u ? 1u : 0u;
        schway__pad_abort(inst, port);
        schway__card_abort(inst, port);
        sio->tx_pending = 0u;
        sio->byte_deadline = 0u;
        sio->ack_pending = 0u;
        sio->ack_deadline = 0u;
        sio->ack_low = 0u;
        sio->release_deadline = 0u;
    }
    if ((next & CTRL_ACK) != 0u) {
        sio->parity_error = 0u;
        sio->irq = 0u;
    }
    sio->ctrl = (uint16_t)(next & (UINT16_MAX ^ CTRL_ACK));
    schway__sched_recalc(inst);
    return 0;
}

int schway__sio_io(schway *inst, uint32_t wr, uint32_t addr, uint32_t width,
                   uint32_t *value, uint32_t *cycles)
{
    if ((width != 1u && width != 2u && width != 4u) || value == NULL) {
        return 1;
    }
    uint64_t now = schway__sched_now(inst, *cycles);
    uint64_t next_deadline = schway__sio_next(inst);
    schway__sio_event(inst, now);
    if (next_deadline != schway__sio_next(inst)) {
        schway__sched_recalc(inst);
    }
    struct schway__sio *sio = &inst->sio;
    if (wr == 0u) {
        if (peek_reg(inst, addr, width, value) != 0) {
            return 1;
        }
        if (addr == SIO_DATA) {
            rx_pop(sio, width);
        }
        return 0;
    }
    if (addr >= SIO_STAT && addr < SIO_STAT + 4u) {
        uint32_t ignored = 0u;
        return peek_reg(inst, addr, width, &ignored);
    }
    if (addr == SIO_DATA && (width == 1u || width == 2u || width == 4u)) {
        if ((sio->ctrl & CTRL_TXEN) != 0u && sio->tx_pending == 0u) {
            sio->tx_data = (uint8_t)(*value & 0xFFu);
            sio->tx_port = (sio->ctrl & CTRL_PORT2) != 0u ? 1u : 0u;
            sio->tx_pending = 1u;
            sio->byte_deadline = deadline_after(now, byte_cycles(sio));
            schway__sched_recalc(inst);
        }
        return 0;
    }
    if (addr >= SIO_MODE && addr < SIO_MODE + 2u) {
        uint16_t next = sio->mode;
        if (register16_write(&next, SIO_MODE, addr, width, *value) != 0u) {
            return 1;
        }
        sio->mode = (uint16_t)(next & MODE_MASK);
        return 0;
    }
    if (addr >= SIO_CTRL && addr < SIO_CTRL + 2u) {
        return write_ctrl(inst, addr, width, *value);
    }
    if (addr >= SIO_BAUD && addr < SIO_BAUD + 2u) {
        uint16_t next = sio->baud;
        if (register16_write(&next, SIO_BAUD, addr, width, *value) != 0u) {
            return 1;
        }
        sio->baud = next;
        return 0;
    }
    return 1;
}
