#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    StackClass *self = (StackClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static void push(StackClass *self, uint8_t data)
{
    self->parent->cpu->get_registers(self->parent->cpu)->sp--;
    self->parent->bus->write(self->parent->bus,
        self->parent->cpu->get_registers(self->parent->cpu)->sp, data);
}

static void push16(StackClass *self, uint16_t data)
{
    self->push(self, (data >> 8) & 0xFF);
    self->push(self, data & 0xFF);
}

static uint8_t pop(StackClass *self)
{
    return self->parent->bus->read(self->parent->bus,
        self->parent->cpu->get_registers(self->parent->cpu)->sp++);
}

static uint16_t pop16(StackClass *self)
{
    uint8_t lo = self->pop(self);
    uint8_t hi = self->pop(self);

    return (hi << 8) | lo;
}

const StackClass init_stack = {
    {
        ._size = sizeof(StackClass),
        ._name = "Stack",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .push = push,
    .push16 = push16,
    .pop = pop,
    .pop16 = pop16,
};

const class_t *Stack = (const class_t *) &init_stack;
