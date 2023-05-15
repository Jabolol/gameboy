#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    IOClass *self = (IOClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static uint8_t read(IOClass *self, uint16_t address)
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
        case LCD_RANGE: {
            return self->parent->lcd->read(self->parent->lcd, address);
        }
        default: {
            NOT_IMPLEMENTED();
            return 0;
        }
    }
}

static void write(IOClass *self, uint16_t address, uint8_t value)
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
        case LCD_RANGE: {
            self->parent->lcd->write(self->parent->lcd, address, value);
            break;
        }
        default: {
            NOT_IMPLEMENTED();
        }
    }
}

const IOClass init_io = {
    {
        ._size = sizeof(IOClass),
        ._name = "IO",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .read = read,
    .write = write,
};

const class_t *IO = (const class_t *) &init_io;
