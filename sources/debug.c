#include <unistd.h>
#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    DebugClass *self = (DebugClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static void update(DebugClass *self)
{
    if (self->parent->bus->read(self->parent->bus, SERIAL_CONTROL)
        == FIRST_LAST_SET) {
        char c = self->parent->bus->read(self->parent->bus, SERIAL_DATA);
        self->message[self->message_size++] = c;
        self->parent->bus->write(self->parent->bus, SERIAL_CONTROL, 0);
    }
}

static void print(DebugClass *self)
{
    if (self->message[0]) {
        printf("DBG: %s\n", self->message);
    }
}

static void cpu_step(DebugClass *self, uint16_t pc)
{
    CPUClass *cpu = self->parent->cpu;

    char flags[16];
    snprintf(flags, sizeof(flags), "%c%c%c%c",
        cpu->context->registers.f & (1 << 7) ? 'Z' : '-',
        cpu->context->registers.f & (1 << 6) ? 'N' : '-',
        cpu->context->registers.f & (1 << 5) ? 'H' : '-',
        cpu->context->registers.f & (1 << 4) ? 'C' : '-');

    cpu->pretty_instruction(cpu, self->instruction_data);

    size_t len = snprintf(self->buffer, BUFFER_SIZE,
        "%08llX - %04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X "
        "DE: %02X%02X "
        "HL: %02X%02X\n",
        cpu->parent->context->ticks, pc, self->instruction_data,
        cpu->context->opcode, cpu->parent->bus->read(cpu->parent->bus, pc + 1),
        cpu->parent->bus->read(cpu->parent->bus, pc + 2),
        cpu->context->registers.a, flags, cpu->context->registers.b,
        cpu->context->registers.c, cpu->context->registers.d,
        cpu->context->registers.e, cpu->context->registers.h,
        cpu->context->registers.l);

    write(STDOUT_FILENO, self->buffer, len);
}

const DebugClass init_debug = {
    {
        ._size = sizeof(DebugClass),
        ._name = "Debug",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .update = update,
    .print = print,
    .cpu_step = cpu_step,
};

const class_t *Debug = (const class_t *) &init_debug;
