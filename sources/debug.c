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

const DebugClass init_debug = {
    {
        ._size = sizeof(DebugClass),
        ._name = "Debug",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .update = update,
    .print = print,
};

const class_t *Debug = (const class_t *) &init_debug;
