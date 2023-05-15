#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    LCDClass *self = (LCDClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);

    self->context->control = 0x91;
    self->context->bg_palette = 0xFC;
    self->context->sprite_palette[0] = 0xFF;
    self->context->sprite_palette[1] = 0xFF;

    for (int32_t i = 0; i < 4; i++) {
        self->context->bg_colors[i] = self->default_colors[i];
        self->context->sprite1_colors[i] = self->default_colors[i];
        self->context->sprite2_colors[i] = self->default_colors[i];
    }
}

static void destructor(void *ptr)
{
    LCDClass *self = (LCDClass *) ptr;
    free(self->context);
}

static uint8_t read(LCDClass *self, uint16_t address)
{
    uint8_t offset = address - LCD_CONTROL;
    uint8_t *pointer = (uint8_t *) self->context;

    return pointer[offset];
}

static void write(LCDClass *self, uint16_t address, uint8_t value)
{
    uint8_t offset = address - LCD_CONTROL;
    uint8_t *pointer = (uint8_t *) self->context;

    pointer[offset] = value;

    if (address == TRANSFER_REG) {
        self->parent->dma->start(self->parent->dma, value);
    }
    switch (address) {
        case LCD_BG_PAL: self->update(self, value, 0); break;
        case LCD_S1_PAL: self->update(self, value & 0b11111100, 1); break;
        case LCD_S2_PAL: self->update(self, value & 0b11111100, 1); break;
    }
}

static void update(LCDClass *self, uint8_t data, uint8_t palette)
{
    uint32_t *palette_colors = self->context->bg_colors;

    switch (palette) {
        case 1: palette_colors = self->context->sprite1_colors; break;
        case 2: palette_colors = self->context->sprite2_colors; break;
    }
    for (int32_t i = 0; i < 4; i++) {
        palette_colors[i] = self->default_colors[(data >> (i * 2) & 0b11)];
    }
}

const LCDClass init_lcd = {
    {
        ._size = sizeof(LCDClass),
        ._name = "LCD",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .default_colors = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000},
    .read = read,
    .write = write,
    .update = update,
};

const class_t *LCD = (const class_t *) &init_lcd;
