#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    BusClass *self = (BusClass *) ptr;
    self->cartridge = va_arg(*args, CartridgeClass *);
}

static uint8_t read(BusClass *self, uint16_t address)
{
    switch (address) {
        case 0 ... 0x7FFF: {
            return self->cartridge->read(self->cartridge, address);
        }
        case 0x8000 ... 0x9FFF: HANDLE_ERROR("not implemented read");
        case 0xA000 ... 0xBFFF: HANDLE_ERROR("not implemented read");
        case 0xC000 ... 0xDFFF: HANDLE_ERROR("not implemented read");
        case 0xE000 ... 0xFDFF: HANDLE_ERROR("not implemented read");
        case 0xFE00 ... 0xFE9F: HANDLE_ERROR("not implemented read");
        case 0xFEA0 ... 0xFEFF: HANDLE_ERROR("not implemented read");
        case 0xFF00 ... 0xFF7F: HANDLE_ERROR("not implemented read");
        case 0xFF80 ... 0xFFFE: HANDLE_ERROR("not implemented read");
        case 0xFFFF: HANDLE_ERROR("not implemented read");
        default: {
            char buff[256];
            sprintf(buff, "not implemented read %04X", address);
            HANDLE_ERROR(buff);
        }
    }
}

static void write(BusClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case 0 ... 0x7FFF: {
            self->cartridge->write(self->cartridge, address, value);
            break;
        }
        case 0x8000 ... 0x9FFF: HANDLE_ERROR("not implemented write");
        case 0xA000 ... 0xBFFF: HANDLE_ERROR("not implemented write");
        case 0xC000 ... 0xDFFF: HANDLE_ERROR("not implemented write");
        case 0xE000 ... 0xFDFF: HANDLE_ERROR("not implemented write");
        case 0xFE00 ... 0xFE9F: HANDLE_ERROR("not implemented write");
        case 0xFEA0 ... 0xFEFF: HANDLE_ERROR("not implemented write");
        case 0xFF00 ... 0xFF7F: HANDLE_ERROR("not implemented write");
        case 0xFF80 ... 0xFFFE: HANDLE_ERROR("not implemented write");
        case 0xFFFF: HANDLE_ERROR("not implemented write");
        default: {
            char buff[256];
            sprintf(buff, "not implemented write %04X", address);
            HANDLE_ERROR(buff);
        }
    }
}

static uint16_t read16(BusClass *self, uint16_t address)
{
    uint16_t lo = self->read(self, address);
    uint16_t hi = self->read(self, address + 1);

    return lo | (hi << 8);
}

static void write16(BusClass *self, uint16_t address, uint16_t value)
{
    self->write(self, address + 1, (value >> 8) & 0xFF);
    self->write(self, address, value & 0xFF);
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
    .read16 = read16,
    .write16 = write16,
};

const class_t *Bus = (const class_t *) &bus_init;
