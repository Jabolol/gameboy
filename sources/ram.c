#include "../include/gameboy.h"

static void constructor(void *ptr, va_list UNUSED *args)
{
    RAMClass *self = (RAMClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->context->wram_bank = 1;
}

static void destructor(void *ptr)
{
    RAMClass *self = (RAMClass *) ptr;
    free(self->context);
}

static uint8_t wram_read(RAMClass *self, uint16_t address)
{
    if (address < 0xD000) {
        return self->context->wram[address - 0xC000];
    } else {
        uint16_t offset =
            (self->context->wram_bank * 0x1000) + (address - 0xD000);
        return self->context->wram[offset];
    }
}

static void wram_write(RAMClass *self, uint16_t address, uint8_t value)
{
    if (address < 0xD000) {
        self->context->wram[address - 0xC000] = value;
    } else {
        uint16_t offset =
            (self->context->wram_bank * 0x1000) + (address - 0xD000);
        self->context->wram[offset] = value;
    }
}

static uint8_t hram_read(RAMClass *self, uint16_t address)
{
    return self->context->hram[address - 0xFF80];
}

static void hram_write(RAMClass *self, uint16_t address, uint8_t value)
{
    self->context->hram[address - 0xFF80] = value;
}

const RAMClass init_ram = {
    {
        ._size = sizeof(RAMClass),
        ._name = "RAM",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .wram_read = wram_read,
    .wram_write = wram_write,
    .hram_read = hram_read,
    .hram_write = hram_write,
};

const class_t *RAM = (const class_t *) &init_ram;
