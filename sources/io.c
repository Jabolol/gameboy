#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    IoClass *self = (IoClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static uint8_t read(IoClass *self, uint16_t address)
{
    switch (address) {
        case SERIAL_DATA: {
            return self->serial_data[0];
        }
        case SERIAL_CONTROL: {
            return self->serial_data[1];
        }
        case TIMER_RANGE: {
            return self->parent->timer->read(self->parent->timer, address);
        }
        case INTERRUPT_FLAG: {
            return self->parent->cpu->get_int_flags(self->parent->cpu);
        }
        default: {
            NOT_IMPLEMENTED();
            return 0;
        }
    }
}

static void write(IoClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case SERIAL_DATA: {
            self->serial_data[0] = value;
            break;
        }
        case SERIAL_CONTROL: {
            self->serial_data[1] = value;
            break;
        }
        case TIMER_RANGE: {
            self->parent->timer->write(self->parent->timer, address, value);
            break;
        }
        case INTERRUPT_FLAG: {
            self->parent->cpu->set_int_flags(self->parent->cpu, value);
            break;
        }
        default: {
            NOT_IMPLEMENTED();
        }
    }
}

const IoClass init_io = {
    {
        ._size = sizeof(IoClass),
        ._name = "Io",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .read = read,
    .write = write,
};

const class_t *Io = (const class_t *) &init_io;
