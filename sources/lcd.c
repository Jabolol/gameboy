#include "../include/gameboy.h"

static uint32_t convert_rgb555_to_rgb888(uint16_t rgb555)
{
    uint8_t r = (rgb555 & 0x1F) * 255 / 31;
    uint8_t g = ((rgb555 >> 5) & 0x1F) * 255 / 31;
    uint8_t b = ((rgb555 >> 10) & 0x1F) * 255 / 31;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static void update_cgb_palettes(LCDClass *self)
{
    for (int32_t pal = 0; pal < 8; pal++) {
        for (int32_t color = 0; color < 4; color++) {
            int32_t offset = pal * 8 + color * 2;
            uint16_t rgb555 = self->context->bg_palette_data[offset]
                | (self->context->bg_palette_data[offset + 1] << 8);
            self->context->bg_colors_cgb[pal][color] =
                convert_rgb555_to_rgb888(rgb555);

            rgb555 = self->context->sprite_palette_data[offset]
                | (self->context->sprite_palette_data[offset + 1] << 8);
            self->context->sprite_colors_cgb[pal][color] =
                convert_rgb555_to_rgb888(rgb555);
        }
    }
}

static void hdma_start(LCDClass *self, uint8_t value)
{
    if ((value & 0x80) && self->context->hdma.active) {
        self->context->hdma.active = false;
        return;
    }

    self->context->hdma.source =
        ((self->context->hdma.hdma1 << 8) | self->context->hdma.hdma2)
        & 0xFFF0;
    self->context->hdma.dest =
        (((self->context->hdma.hdma3 << 8) | self->context->hdma.hdma4)
            & 0x1FF0)
        | 0x8000;
    self->context->hdma.remaining = ((value & 0x7F) + 1) * 0x10;
    self->context->hdma.hblank_mode = value & 0x80;
    self->context->hdma.active = true;
    self->context->hdma.hdma5 = value;

    if (!self->context->hdma.hblank_mode) {
        while (self->context->hdma.remaining > 0) {
            self->hdma_tick(self);
        }
        self->context->hdma.active = false;
    }
}

static void hdma_tick(LCDClass *self)
{
    if (!self->context->hdma.active || self->context->hdma.remaining == 0) {
        return;
    }

    uint8_t data = self->parent->bus->read(
        self->parent->bus, self->context->hdma.source++);
    self->parent->bus->write(
        self->parent->bus, self->context->hdma.dest++, data);

    self->context->hdma.remaining--;

    if (self->context->hdma.remaining == 0) {
        self->context->hdma.active = false;
    }
}

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
    self->context->opri = 0xFE;
    self->context->undoc_ff72 = 0x00;
    self->context->undoc_ff73 = 0x00;
    self->context->undoc_ff74 = 0x00;
    self->context->undoc_ff75 = 0x8F;

    for (int32_t i = 0; i < 4; i++) {
        self->context->bg_colors[i] = self->default_colors[i];
        self->context->sprite1_colors[i] = self->default_colors[i];
        self->context->sprite2_colors[i] = self->default_colors[i];
    }

    for (int32_t pal = 0; pal < 8; pal++) {
        for (int32_t color = 0; color < 4; color++) {
            self->context->bg_colors_cgb[pal][color] =
                self->default_colors[color];
            self->context->sprite_colors_cgb[pal][color] =
                self->default_colors[color];
        }
    }
}

static void destructor(void *ptr)
{
    LCDClass *self = (LCDClass *) ptr;
    free(self->context);
}

static uint8_t read(LCDClass *self, uint16_t address)
{
    switch (address) {
        case LCD_VBK: return self->parent->ppu->context->vram_bank | 0xFE;
        case LCD_HDMA1: return self->context->hdma.hdma1;
        case LCD_HDMA2: return self->context->hdma.hdma2;
        case LCD_HDMA3: return self->context->hdma.hdma3;
        case LCD_HDMA4: return self->context->hdma.hdma4;
        case LCD_HDMA5:
            if (self->context->hdma.active) {
                return (self->context->hdma.remaining / 0x10 - 1);
            }
            return 0xFF;
        case LCD_BCPS: return self->context->bg_palette_index | 0x40;
        case LCD_BCPD:
            return self->context
                ->bg_palette_data[self->context->bg_palette_index & 0x3F];
        case LCD_OCPS: return self->context->sprite_palette_index | 0x40;
        case LCD_OCPD:
            return self->context
                ->sprite_palette_data[self->context->sprite_palette_index
                    & 0x3F];
        default: {
            uint16_t offset = address - LCD_CONTROL;
            if (offset < sizeof(*self->context)) {
                return ((uint8_t *) self->context)[offset];
            }
            return 0xFF;
        }
    }
}

static void write(LCDClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case LCD_VBK:
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->ppu->context->vram_bank = value & 0x01;
            }
            return;
        case LCD_HDMA1: self->context->hdma.hdma1 = value; return;
        case LCD_HDMA2: self->context->hdma.hdma2 = value & 0xF0; return;
        case LCD_HDMA3: self->context->hdma.hdma3 = value & 0x1F; return;
        case LCD_HDMA4: self->context->hdma.hdma4 = value & 0xF0; return;
        case LCD_HDMA5:
            if (self->parent->context->hw_mode == HW_CGB) {
                self->hdma_start(self, value);
            }
            return;
        case LCD_BCPS: self->context->bg_palette_index = value; return;
        case LCD_BCPD:
            if (self->parent->context->hw_mode == HW_CGB) {
                uint8_t index = self->context->bg_palette_index & 0x3F;
                self->context->bg_palette_data[index] = value;
                update_cgb_palettes(self);
                if (self->context->bg_palette_index & 0x80) {
                    self->context->bg_palette_index =
                        0x80 | ((self->context->bg_palette_index + 1) & 0x3F);
                }
            }
            return;
        case LCD_OCPS: self->context->sprite_palette_index = value; return;
        case LCD_OCPD:
            if (self->parent->context->hw_mode == HW_CGB) {
                uint8_t index = self->context->sprite_palette_index & 0x3F;
                self->context->sprite_palette_data[index] = value;
                update_cgb_palettes(self);
                if (self->context->sprite_palette_index & 0x80) {
                    self->context->sprite_palette_index = 0x80
                        | ((self->context->sprite_palette_index + 1) & 0x3F);
                }
            }
            return;
        default: {
            uint16_t offset = address - LCD_CONTROL;
            if (offset >= sizeof(*self->context)) {
                return;
            }

            ((uint8_t *) self->context)[offset] = value;

            if (address == TRANSFER_REG) {
                self->parent->dma->start(self->parent->dma, value);
            } else if (address == LCD_BG_PAL) {
                self->update(self, value, 0);
            } else if (address == LCD_S1_PAL) {
                self->update(self, value & 0xFC, 1);
            } else if (address == LCD_S2_PAL) {
                self->update(self, value & 0xFC, 2);
            }
        }
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
    .hdma_start = hdma_start,
    .hdma_tick = hdma_tick,
};

const class_t *LCD = (const class_t *) &init_lcd;
