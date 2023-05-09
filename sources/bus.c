#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    BusClass *self = (BusClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static uint8_t read(BusClass *self, uint16_t address)
{
    switch (address) {
        case ROM_RANGE: {
            return self->parent->cartridge->read(self->parent->cartridge, address);
        }
        case CHAR_RANGE: {
            HANDLE_ERROR("not implemented read at CHAR_RANGE");
        }
        case CART_RAM_RANGE: {
            return self->parent->cartridge->read(self->parent->cartridge, address);
        }
        case WRAM_RANGE: {
            return self->parent->ram->wram_read(self->parent->ram, address);
        }
        case ECHO_RANGE: {
            return 0;
        }
        case OAM_RANGE: {
            // HANDLE_ERROR("not implemented read at OAM_RANGE");
            NOT_IMPLEMENTED();
            return 0;
        }
        case RESERVED_RANGE: {
            return 0;
        }
        case IO_REGS_RANGE: {
            return self->parent->io->read(self->parent->io, address);
        }
        case HRAM_RANGE: {
            return self->parent->ram->hram_read(self->parent->ram, address);
        }
        case CPU_ENABLE_REG: {
            return self->parent->cpu->get_ie_register(self->parent->cpu);
        }
        default: {
            char buff[256];
            sprintf(buff, "out of bounds write %04X at UNKNOWN", address);
            HANDLE_ERROR(buff);
        }
    }
}

static void write(BusClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case ROM_RANGE: {
            self->parent->cartridge->write(self->parent->cartridge, address, value);
            break;
        }
        case CHAR_RANGE: {
            // HANDLE_ERROR("not implemented write at CHAR_RANGE");
            NOT_IMPLEMENTED();
            break;
        }
        case CART_RAM_RANGE: {
            self->parent->cartridge->write(self->parent->cartridge, address, value);
            break;
        }
        case WRAM_RANGE: {
            self->parent->ram->wram_write(self->parent->ram, address, value);
            break;
        }
        case ECHO_RANGE: {
            break;
        }
        case OAM_RANGE: {
            // HANDLE_ERROR("not implemented write at OAM_RANGE");
            NOT_IMPLEMENTED();
            break;
        }
        case RESERVED_RANGE: {
            break;
        }
        case IO_REGS_RANGE: {
            self->parent->io->write(self->parent->io, address, value);
            break;
        }
        case HRAM_RANGE: {
            self->parent->ram->hram_write(self->parent->ram, address, value);
            break;
        }
        case CPU_ENABLE_REG: {
            self->parent->cpu->set_ie_register(self->parent->cpu, value);
            break;
        }
        default: {
            char buff[256];
            sprintf(buff, "not implemented write %04X at UNKNOWN", address);
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
