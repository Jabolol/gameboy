#include "../include/gameboy.h"

static void constructor(void *ptr, va_list UNUSED *args)
{
    RamClass *self = (RamClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
}

static void destructor(void *ptr)
{
    RamClass *self = (RamClass *) ptr;
    free(self->context);
}

static uint8_t wram_read(RamClass *self, uint16_t address)
{
    return self->context->wram[address - 0xC000];
}

static void wram_write(RamClass *self, uint16_t address, uint8_t value)
{
    self->context->wram[address - 0xC000] = value;
}

static uint8_t hram_read(RamClass *self, uint16_t address)
{
    return self->context->hram[address - 0xFF80];
}

static void hram_write(RamClass *self, uint16_t address, uint8_t value)
{
    self->context->hram[address - 0xFF80] = value;
}

const RamClass init_ram = {
    {
        ._size = sizeof(RamClass),
        ._name = "Ram",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .context = NULL,
    .wram_read = wram_read,
    .wram_write = wram_write,
    .hram_read = hram_read,
    .hram_write = hram_write,
};

const class_t *Ram = (const class_t *) &init_ram;
