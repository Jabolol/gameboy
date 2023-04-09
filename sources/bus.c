#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    BusClass *self = (BusClass *) ptr;
    self->cartridge = va_arg(*args, CartridgeClass *);
}

static uint8_t read(BusClass *self, uint16_t address)
{
    if (address < 0x8000) {
        return self->cartridge->read(self->cartridge, address);
    }
    HANDLE_ERROR("not implemented");
}

static void write(BusClass *self, uint16_t address, uint8_t value)
{
    if (address < 0x8000) {
        self->cartridge->write(self->cartridge, address, value);
        return;
    }
    HANDLE_ERROR("not implemented");
}

const BusClass bus_init = {
    {
        ._size = sizeof(BusClass),
        ._name = "Bus",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .read = read,
    .write = write,
};

const class_t *Bus = (const class_t *) &bus_init;
