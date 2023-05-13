#include "../include/gameboy.h"

static void constructor(void *ptr, va_list UNUSED *args)
{
    PPUClass *self = (PPUClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
}

static void destructor(void *ptr)
{
    PPUClass *self = (PPUClass *) ptr;
    free(self->context);
}

static void oam_write(PPUClass *self, uint16_t address, uint8_t value)
{
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    uint8_t *bytes = (uint8_t *) self->context->oam_ram;
    bytes[address] = value;
}

static uint8_t oam_read(PPUClass *self, uint16_t address)
{
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    uint8_t *bytes = (uint8_t *) self->context->oam_ram;
    return bytes[address];
}

static void vram_write(PPUClass *self, uint16_t address, uint8_t value)
{
    self->context->vram[address - 0x8000] = value;
}

static uint8_t vram_read(PPUClass *self, uint16_t address)
{
    return self->context->vram[address - 0x8000];
}

const PPUClass init_ppu = {
    {
        ._size = sizeof(PPUClass),
        ._name = "PPU",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .oam_write = oam_write,
    .oam_read = oam_read,
    .vram_write = vram_write,
    .vram_read = vram_read,
};

const class_t *PPU = (const class_t *) &init_ppu;
