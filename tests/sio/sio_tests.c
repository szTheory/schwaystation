/* SIO0 protocol tests through JOY MMIO and the instance scheduler. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define JOY_DATA 0x1F801040u
#define JOY_STAT 0x1F801044u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu
#define I_STAT 0x1F801070u

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void write_mmio(schway *inst, uint32_t addr, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, addr, width, value, &cycles), 0u);
}

static uint32_t read_mmio(schway *inst, uint32_t addr, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, addr, width, &value, &cycles), 0u);
    return value;
}

static void advance_to(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
}

static uint64_t transfer(schway *inst, uint8_t sent, uint8_t expected, int last)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);

    advance_to(inst, start + 100u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    advance_to(inst, start + 1087u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) == 0u);
    advance_to(inst, start + 1088u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) != 0u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 1u), expected);

    if (last) {
        advance_to(inst, start + 1494u);
        CHECK((inst->irq.stat & 0x80u) == 0u);
        CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x280u) == 0u);
        return inst->ticks;
    }

    advance_to(inst, start + 1425u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    advance_to(inst, start + 1426u);
    CHECK((inst->irq.stat & 0x80u) != 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x280u) == 0x280u);
    write_mmio(inst, I_STAT, 4u, 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
    advance_to(inst, start + 1494u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) == 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
    return inst->ticks;
}

static void digital_poll_irq(void)
{
    schway *inst = make();
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);

    (void)transfer(inst, 0x01u, 0xFFu, 0);
    (void)transfer(inst, 0x42u, 0x41u, 0);
    (void)transfer(inst, 0x00u, 0x5Au, 0);
    (void)transfer(inst, 0x00u, 0xFFu, 0);
    (void)transfer(inst, 0x00u, 0xFFu, 1);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) == 0u);
    schway_destroy(inst);
}

static uint8_t *save_state(const schway *inst, size_t *size)
{
    *size = schway_state_size(inst);
    uint8_t *buf = (uint8_t *)malloc(*size);
    CHECK(buf != NULL);
    if (buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, *size), SCHWAY_STATUS_OK);
    return buf;
}

static void check_states_equal(const schway *a, const schway *b)
{
    size_t a_size = 0u;
    size_t b_size = 0u;
    uint8_t *a_buf = save_state(a, &a_size);
    uint8_t *b_buf = save_state(b, &b_size);
    CHECK_EQ_U64(a_size, b_size);
    if (a_size == b_size) {
        CHECK(memcmp(a_buf, b_buf, a_size) == 0);
    }
    free(a_buf);
    free(b_buf);
}

static void configure(schway *inst, uint32_t port)
{
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u | (port != 0u ? 0x2000u : 0u));
}

static schway *clone_state(const schway *source)
{
    size_t size = 0u;
    uint8_t *buf = save_state(source, &size);
    schway *copy = make();
    CHECK_EQ_U64(schway_load_state(copy, buf, size), SCHWAY_STATUS_OK);
    free(buf);
    return copy;
}

static void state_pending_byte(void)
{
    schway *source = make();
    configure(source, 0u);
    write_mmio(source, JOY_DATA, 1u, 0x01u);
    CHECK(source->sio.tx_pending != 0u);
    CHECK_EQ_U64(source->sio.byte_deadline, 1088u);
    schway *copy = clone_state(source);
    CHECK_EQ_U64(copy->sio.byte_deadline, source->sio.byte_deadline);

    advance_to(source, 1088u);
    advance_to(copy, 1088u);
    CHECK_EQ_HEX(read_mmio(source, JOY_DATA, 1u), 0xFFu);
    CHECK_EQ_HEX(read_mmio(copy, JOY_DATA, 1u), 0xFFu);
    CHECK_EQ_U64(source->sio.ack_deadline, 1426u);
    CHECK_EQ_U64(copy->sio.ack_deadline, 1426u);
    CHECK((source->irq.stat & 0x80u) == 0u);
    CHECK((copy->irq.stat & 0x80u) == 0u);
    check_states_equal(source, copy);
    schway_destroy(copy);
    schway_destroy(source);
}

static void state_pending_ack(void)
{
    schway *source = make();
    configure(source, 0u);
    write_mmio(source, JOY_DATA, 1u, 0x01u);
    advance_to(source, 1088u);
    CHECK(source->sio.ack_pending != 0u);
    schway *copy = clone_state(source);
    CHECK_EQ_U64(copy->sio.ack_deadline, source->sio.ack_deadline);

    advance_to(source, 1425u);
    advance_to(copy, 1425u);
    CHECK((source->irq.stat & 0x80u) == 0u);
    CHECK((copy->irq.stat & 0x80u) == 0u);
    advance_to(source, 1426u);
    advance_to(copy, 1426u);
    CHECK((source->irq.stat & 0x80u) != 0u);
    CHECK((copy->irq.stat & 0x80u) != 0u);
    CHECK(source->sio.ack_low != 0u);
    CHECK(copy->sio.ack_low != 0u);
    check_states_equal(source, copy);

    write_mmio(source, I_STAT, 4u, 0u);
    write_mmio(copy, I_STAT, 4u, 0u);
    write_mmio(source, JOY_CTRL, 2u, 0x1013u);
    write_mmio(copy, JOY_CTRL, 2u, 0x1013u);
    advance_to(source, 1494u);
    advance_to(copy, 1494u);
    CHECK(source->sio.ack_low == 0u);
    CHECK(copy->sio.ack_low == 0u);
    CHECK((source->irq.stat & 0x80u) == 0u);
    CHECK((copy->irq.stat & 0x80u) == 0u);
    check_states_equal(source, copy);
    schway_destroy(copy);
    schway_destroy(source);
}

static void transport_abort(void)
{
    schway *reset = make();
    configure(reset, 0u);
    write_mmio(reset, JOY_DATA, 1u, 0x01u);
    advance_to(reset, 500u);
    write_mmio(reset, JOY_CTRL, 2u, 0x1043u);
    CHECK(reset->sio.tx_pending == 0u);
    CHECK(reset->sio.byte_deadline == 0u);
    CHECK(reset->pad.phase[0] == 0u);
    advance_to(reset, 1088u);
    CHECK(reset->sio.rx_count == 0u);
    CHECK((reset->irq.stat & 0x80u) == 0u);
    schway_destroy(reset);

    schway *deselect = make();
    configure(deselect, 0u);
    write_mmio(deselect, JOY_DATA, 1u, 0x01u);
    advance_to(deselect, 1088u);
    CHECK(deselect->sio.ack_pending != 0u);
    write_mmio(deselect, JOY_CTRL, 2u, 0x1001u);
    CHECK(deselect->sio.ack_pending == 0u);
    CHECK(deselect->sio.ack_deadline == 0u);
    CHECK(deselect->pad.phase[0] == 0u);
    advance_to(deselect, 1426u);
    CHECK((deselect->irq.stat & 0x80u) == 0u);
    CHECK(deselect->sio.ack_low == 0u);
    schway_destroy(deselect);
}

static uint8_t exchange_byte(schway *inst, uint8_t sent, int ack)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);
    advance_to(inst, start + 1088u);
    uint8_t received = (uint8_t)read_mmio(inst, JOY_DATA, 1u);
    if (ack) {
        advance_to(inst, start + 1426u);
        write_mmio(inst, I_STAT, 4u, 0u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1013u |
                                             ((inst->sio.ctrl & 0x2000u) != 0u ? 0x2000u : 0u));
        advance_to(inst, start + 1494u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1003u |
                                             ((inst->sio.ctrl & 0x2000u) != 0u ? 0x2000u : 0u));
    }
    return received;
}

static void check_poll(schway *inst, uint32_t port, uint16_t buttons)
{
    uint32_t port_bit = port != 0u ? 0x2000u : 0u;
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u | port_bit);
    CHECK_EQ_HEX(exchange_byte(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange_byte(inst, 0x42u, 1), 0x41u);
    CHECK_EQ_HEX(exchange_byte(inst, 0x00u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange_byte(inst, 0x00u, 1), (uint8_t)(buttons & 0xFFu));
    CHECK_EQ_HEX(exchange_byte(inst, 0x00u, 0), (uint8_t)(buttons >> 8));
}

static void independent_ports_instances(void)
{
    schway *a = make();
    schway *b = make();
    configure(a, 0u);
    configure(b, 0u);
    schway__pad_set_buttons(a, 0u, 0x1234u);
    schway__pad_set_buttons(a, 1u, 0x5678u);
    schway__pad_set_buttons(b, 0u, 0x9ABCu);
    schway__pad_set_connected(a, 1u, 1u);
    check_poll(a, 0u, 0x1234u);
    check_poll(a, 1u, 0x5678u);
    check_poll(b, 0u, 0x9ABCu);
    CHECK_EQ_U64(a->pad.transfers, 2u);
    CHECK_EQ_U64(b->pad.transfers, 1u);
    schway_destroy(b);
    schway_destroy(a);
}

static void send_without_pop(schway *inst, uint8_t sent, int ack)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);
    advance_to(inst, start + 1088u);
    if (ack) {
        advance_to(inst, start + 1426u);
        write_mmio(inst, I_STAT, 4u, 0u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
        advance_to(inst, start + 1494u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
    }
}

static void fifo_widths(void)
{
    schway *inst = make();
    configure(inst, 0u);
    send_without_pop(inst, 0x01u, 1);
    send_without_pop(inst, 0x42u, 1);
    send_without_pop(inst, 0x00u, 1);
    send_without_pop(inst, 0x00u, 1);
    CHECK_EQ_U64(inst->sio.rx_count, 4u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 2u), 0xFFu);
    CHECK_EQ_U64(inst->sio.rx_count, 3u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 1u), 0x41u);
    CHECK_EQ_U64(inst->sio.rx_count, 2u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 4u), 0x0000FF5Au);
    CHECK_EQ_U64(inst->sio.rx_count, 0u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 4u), 0xFFu);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) == 0u);
    schway_destroy(inst);
}

static void fifo_overrun(void)
{
    schway *inst = make();
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    for (uint32_t i = 0u; i < 9u; i++) {
        uint64_t start = inst->ticks;
        write_mmio(inst, JOY_CTRL, 2u, 0x0B05u);
        write_mmio(inst, JOY_DATA, 1u, i);
        advance_to(inst, start + 1088u);
        if (i < 7u) {
            CHECK((inst->irq.stat & 0x80u) == 0u);
        } else {
            CHECK((inst->irq.stat & 0x80u) != 0u);
        }
    }
    CHECK_EQ_U64(inst->sio.rx_count, SCHWAY__SIO_RX_CAP);
    CHECK(inst->sio.parity_error != 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & (1u << 3)) != 0u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 4u), 0xFFFFFFFFu);
    CHECK_EQ_U64(inst->sio.rx_count, 4u);
    (void)read_mmio(inst, JOY_DATA, 4u);
    CHECK_EQ_U64(inst->sio.rx_count, 0u);
    write_mmio(inst, I_STAT, 4u, 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x0B15u);
    CHECK(inst->sio.parity_error == 0u);
    CHECK(inst->sio.irq == 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & ((1u << 3) | 0x200u)) == 0u);
    schway_destroy(inst);
}

static void baud_character_length(void)
{
    schway *inst = make();
    write_mmio(inst, JOY_MODE, 2u, 0x0000u);
    write_mmio(inst, JOY_BAUD, 2u, 0x0002u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1005u);
    write_mmio(inst, JOY_DATA, 1u, 0xA5u);
    advance_to(inst, 9u);
    CHECK_EQ_U64(inst->sio.rx_count, 0u);
    advance_to(inst, 10u);
    CHECK_EQ_U64(inst->sio.rx_count, 1u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 1u), 0xFFu);

    write_mmio(inst, JOY_MODE, 2u, 0x000Eu);
    write_mmio(inst, JOY_CTRL, 2u, 0x1005u);
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, 0xA5u);
    advance_to(inst, start + 255u);
    CHECK_EQ_U64(inst->sio.rx_count, 0u);
    advance_to(inst, start + 256u);
    CHECK_EQ_U64(inst->sio.rx_count, 1u);
    (void)read_mmio(inst, JOY_DATA, 1u);

    write_mmio(inst, JOY_MODE, 2u, 0x001Eu);
    write_mmio(inst, JOY_CTRL, 2u, 0x1005u);
    start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, 0xA5u);
    advance_to(inst, start + 287u);
    CHECK_EQ_U64(inst->sio.rx_count, 0u);
    advance_to(inst, start + 288u);
    CHECK_EQ_U64(inst->sio.rx_count, 1u);
    schway_destroy(inst);
}

static void irq_ack_order(void)
{
    schway *inst = make();
    configure(inst, 0u);
    write_mmio(inst, JOY_DATA, 1u, 0x01u);
    advance_to(inst, 1088u);
    advance_to(inst, 1426u);
    CHECK((inst->irq.stat & 0x80u) != 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x200u) != 0u);
    write_mmio(inst, I_STAT, 4u, 0u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x200u) != 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x200u) == 0u);
    advance_to(inst, 1494u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);

    write_mmio(inst, JOY_DATA, 1u, 0x42u);
    advance_to(inst, 2582u);
    advance_to(inst, 2920u);
    CHECK((inst->irq.stat & 0x80u) != 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
    CHECK((inst->irq.stat & 0x80u) != 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x200u) == 0u);
    write_mmio(inst, I_STAT, 4u, 0u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    schway_destroy(inst);
}

static void no_final_ack(void)
{
    schway *inst = make();
    configure(inst, 0u);
    CHECK_EQ_HEX(exchange_byte(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange_byte(inst, 0x42u, 1), 0x41u);
    CHECK_EQ_HEX(exchange_byte(inst, 0x00u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange_byte(inst, 0x00u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange_byte(inst, 0x00u, 0), 0xFFu);
    CHECK(inst->sio.ack_pending == 0u);
    CHECK(inst->sio.ack_low == 0u);
    CHECK(inst->pad.transfers == 1u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    advance_to(inst, inst->ticks + 338u);
    CHECK(inst->sio.ack_pending == 0u);
    CHECK(inst->sio.ack_low == 0u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    schway_destroy(inst);
}

static void peek_pure(void)
{
    schway *inst = make();
    configure(inst, 0u);
    write_mmio(inst, JOY_DATA, 1u, 0x01u);
    schway *before = clone_state(inst);
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__sio_peek(inst, JOY_DATA, 4u, &value), 0u);
    CHECK_EQ_HEX(value, 0u);
    CHECK_EQ_U64(schway__sio_peek(inst, JOY_STAT, 4u, &value), 0u);
    check_states_equal(inst, before);

    advance_to(inst, 1088u);
    advance_to(before, 1088u);
    CHECK_EQ_U64(inst->sio.rx_count, 1u);
    CHECK_EQ_U64(inst->pad.transfers, 0u);
    CHECK_EQ_U64(schway__sio_peek(inst, JOY_DATA, 4u, &value), 0u);
    CHECK_EQ_HEX(value, 0xFFu);
    CHECK_EQ_U64(schway__sio_peek(inst, JOY_STAT, 4u, &value), 0u);
    CHECK((value & 2u) != 0u);
    CHECK_EQ_U64(inst->sio.rx_count, 1u);
    CHECK_EQ_U64(inst->pad.transfers, 0u);
    check_states_equal(inst, before);
    schway_destroy(before);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "digital_poll_irq") == 0) {
        digital_poll_irq();
    } else if (strcmp(argv[1], "state_pending_byte") == 0) {
        state_pending_byte();
    } else if (strcmp(argv[1], "state_pending_ack") == 0) {
        state_pending_ack();
    } else if (strcmp(argv[1], "transport_abort") == 0) {
        transport_abort();
    } else if (strcmp(argv[1], "independent_ports_instances") == 0) {
        independent_ports_instances();
    } else if (strcmp(argv[1], "fifo_widths") == 0) {
        fifo_widths();
    } else if (strcmp(argv[1], "fifo_overrun") == 0) {
        fifo_overrun();
    } else if (strcmp(argv[1], "baud_character_length") == 0) {
        baud_character_length();
    } else if (strcmp(argv[1], "irq_ack_order") == 0) {
        irq_ack_order();
    } else if (strcmp(argv[1], "no_final_ack") == 0) {
        no_final_ack();
    } else if (strcmp(argv[1], "peek_pure") == 0) {
        peek_pure();
    } else {
        return 2;
    }
    CHECK_DONE();
}
