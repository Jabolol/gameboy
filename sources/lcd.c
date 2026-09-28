#include "../include/gameboy.h"

static uint32_t expand_channel(uint32_t value)
{
    return (value << 3) | (value >> 2);
}

static uint32_t convert_rgb555_to_rgb888(uint16_t rgb555, bool corrected)
{
    uint32_t r = rgb555 & 0x1F;
    uint32_t g = (rgb555 >> 5) & 0x1F;
    uint32_t b = (rgb555 >> 10) & 0x1F;

    if (corrected) {
        return 0xFF000000 | (((13 * r + 2 * g + b) >> 1) << 16)
            | (((3 * g + b) << 1) << 8) | ((3 * r + 2 * g + 11 * b) >> 1);
    }
    return 0xFF000000 | (expand_channel(r) << 16) | (expand_channel(g) << 8)
        | expand_channel(b);
}

static void update_cgb_color(LCDClass *self, bool sprite, uint8_t index)
{
    uint8_t *data = sprite ? self->context->sprite_palette_data
                           : self->context->bg_palette_data;
    uint8_t offset = index & 0x3E;
    uint16_t rgb555 = data[offset] | (data[offset + 1] << 8);
    uint32_t *target = sprite
        ? &self->context->sprite_colors_cgb[offset >> 3][(offset >> 1) & 3]
        : &self->context->bg_colors_cgb[offset >> 3][(offset >> 1) & 3];

    *target = convert_rgb555_to_rgb888(
        rgb555, atomic_load(&self->parent->context->color_correction));
}

static void hdma_block(LCDClass *self)
{
    hdma_context_t *hdma = &self->context->hdma;
    PPUClass *ppu = self->parent->ppu;

    hdma->pending = false;
    if (!hdma->active) {
        return;
    }
    for (uint8_t i = 0; i < 0x10; i++, hdma->source++, hdma->dest++) {
        uint8_t data = (hdma->source & 0xE000) == 0x8000
            ? 0xFF
            : self->parent->bus->read(self->parent->bus, hdma->source);
        ppu->vram_write(ppu, hdma->dest, data);
    }
    hdma->remaining -= 0x10;
    hdma->hdma1 = hdma->source >> 8;
    hdma->hdma2 = hdma->source & 0xF0;
    hdma->hdma3 = (hdma->dest >> 8) & 0x1F;
    hdma->hdma4 = hdma->dest & 0xF0;

    if (hdma->remaining == 0 || hdma->dest > 0x9FFF) {
        hdma->active = false;
        hdma->remaining = 0;
        hdma->hdma5 = 0xFF;
    } else {
        hdma->hdma5 = ((hdma->remaining / 0x10) - 1) & 0x7F;
    }

    self->parent->cycles(
        self->parent, self->parent->context->double_speed ? 16 : 8);
}

static void hdma_start(LCDClass *self, uint8_t value)
{
    hdma_context_t *hdma = &self->context->hdma;

    if (hdma->active && hdma->hblank_mode && !(value & 0x80)) {
        hdma->active = false;
        hdma->pending = false;
        hdma->hdma5 = 0x80 | (value & 0x7F);
        return;
    }

    hdma->source = ((hdma->hdma1 << 8) | hdma->hdma2) & 0xFFF0;
    hdma->dest = (((hdma->hdma3 << 8) | hdma->hdma4) & 0x1FF0) | 0x8000;
    hdma->remaining = ((value & 0x7F) + 1) * 0x10;
    hdma->hblank_mode = value & 0x80;
    hdma->active = true;
    hdma->hdma5 = value & 0x7F;

    if (!hdma->hblank_mode) {
        while (hdma->active) {
            self->hdma_block(self);
        }
        return;
    }
    if (!(self->context->control & 0x80)
        || (self->context->status & 0x03) == MODE_HBLANK) {
        hdma->pending = true;
    }
}

static void hdma_hblank(LCDClass *self)
{
    if (self->context->hdma.active && self->context->hdma.hblank_mode) {
        self->context->hdma.pending = true;
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
    self->context->status = 0x85;
    self->context->dma = 0xFF;
    self->context->bg_palette = 0xFC;
    self->context->sprite_palette[0] = 0xFF;
    self->context->sprite_palette[1] = 0xFF;
    self->context->opri = 0x01;
    self->context->hdma.hdma1 = 0xFF;
    self->context->hdma.hdma2 = 0xFF;
    self->context->hdma.hdma3 = 0xFF;
    self->context->hdma.hdma4 = 0xFF;
    self->context->hdma.hdma5 = 0xFF;

    memset(self->context->bg_palette_data, 0xFF,
        sizeof(self->context->bg_palette_data));
    memset(self->context->sprite_palette_data, 0xFF,
        sizeof(self->context->sprite_palette_data));
    self->refresh_colors(self);
}

static void refresh_colors(LCDClass *self)
{
    self->update(self, self->context->bg_palette, 0);
    self->update(self, self->context->sprite_palette[0], 1);
    self->update(self, self->context->sprite_palette[1], 2);
    for (uint8_t i = 0; i < 64; i += 2) {
        self->update_cgb_color(self, false, i);
        self->update_cgb_color(self, true, i);
    }
}

static void destructor(void *ptr)
{
    LCDClass *self = (LCDClass *) ptr;
    free(self->context);
}

static uint8_t read_status(LCDClass *self)
{
    uint8_t status = 0x80 | (self->context->status & 0x7F);

    if (!(self->context->control & 0x80)) {
        status &= ~0x03;
    }
    return status;
}

static uint8_t read(LCDClass *self, uint16_t address)
{
    bool cgb = self->parent->context->hw_mode == HW_CGB;
    lcd_context_t *ctx = self->context;

    switch (address) {
        case LCD_CONTROL: return ctx->control;
        case LCD_STATUS: return self->read_status(self);
        case LCD_SCROLL_Y: return ctx->scroll_y;
        case LCD_SCROLL_X: return ctx->scroll_x;
        case LCD_Y_COORD: return ctx->y_coord;
        case LCD_Y_COMPARE: return ctx->y_compare;
        case TRANSFER_REG: return ctx->dma;
        case LCD_BG_PAL: return ctx->bg_palette;
        case LCD_S1_PAL: return ctx->sprite_palette[0];
        case LCD_S2_PAL: return ctx->sprite_palette[1];
        case LCD_WINDOW_Y: return ctx->window_y;
        case LCD_WINDOW_X: return ctx->window_x;
        case LCD_VBK:
            return cgb ? (self->parent->ppu->context->vram_bank | 0xFE) : 0xFF;
        case LCD_HDMA5: return cgb ? ctx->hdma.hdma5 : 0xFF;
        case LCD_BCPS: return cgb ? (ctx->bg_palette_index | 0x40) : 0xFF;
        case LCD_BCPD:
            return cgb ? ctx->bg_palette_data[ctx->bg_palette_index & 0x3F]
                       : 0xFF;
        case LCD_OCPS: return cgb ? (ctx->sprite_palette_index | 0x40) : 0xFF;
        case LCD_OCPD:
            return cgb
                ? ctx->sprite_palette_data[ctx->sprite_palette_index & 0x3F]
                : 0xFF;
        case LCD_OPRI: return cgb ? (ctx->opri | 0xFE) : 0xFF;
        case INFRARED_PORT:

            return cgb ? ((ctx->infrared & 0xC1) | 0x3E) : 0xFF;
        default: return 0xFF;
    }
}

static void write(LCDClass *self, uint16_t address, uint8_t value)
{
    bool cgb = self->parent->context->hw_mode == HW_CGB;
    lcd_context_t *ctx = self->context;
    PPUClass *ppu = self->parent->ppu;

    ppu->sync(ppu);

    switch (address) {
        case LCD_CONTROL: {
            uint8_t old = ctx->control;
            ctx->control = value;
            if ((old & 0x80) && !(value & 0x80)) {
                ppu->lcd_off(ppu);
            } else if (!(old & 0x80) && (value & 0x80)) {
                ppu->lcd_on(ppu);
            }
            return;
        }
        case LCD_STATUS: {
            if (!cgb) {
                ppu->stat_write_quirk(ppu);
            }
            ctx->status = (ctx->status & 0x87) | (value & 0x78);
            ppu->update_stat(ppu);
            return;
        }
        case LCD_SCROLL_Y: ctx->scroll_y = value; return;
        case LCD_SCROLL_X: ctx->scroll_x = value; return;
        case LCD_Y_COORD: return;
        case LCD_Y_COMPARE: {
            ctx->y_compare = value;
            ppu->check_lyc(ppu);
            return;
        }
        case TRANSFER_REG: {
            ctx->dma = value;
            self->parent->dma->start(self->parent->dma, value);
            return;
        }
        case LCD_BG_PAL: {
            ctx->bg_palette = value;
            self->update(self, value, 0);
            return;
        }
        case LCD_S1_PAL: {
            ctx->sprite_palette[0] = value;
            self->update(self, value, 1);
            return;
        }
        case LCD_S2_PAL: {
            ctx->sprite_palette[1] = value;
            self->update(self, value, 2);
            return;
        }
        case LCD_WINDOW_Y: ctx->window_y = value; return;
        case LCD_WINDOW_X: ctx->window_x = value; return;
        default: break;
    }

    if (!cgb) {
        return;
    }

    switch (address) {
        case LCD_VBK: ppu->context->vram_bank = value & 0x01; return;
        case LCD_HDMA1: ctx->hdma.hdma1 = value; return;
        case LCD_HDMA2: ctx->hdma.hdma2 = value & 0xF0; return;
        case LCD_HDMA3: ctx->hdma.hdma3 = value & 0x1F; return;
        case LCD_HDMA4: ctx->hdma.hdma4 = value & 0xF0; return;
        case LCD_HDMA5: self->hdma_start(self, value); return;
        case LCD_BCPS: ctx->bg_palette_index = value & 0xBF; return;
        case LCD_BCPD: {
            uint8_t index = ctx->bg_palette_index & 0x3F;
            ctx->bg_palette_data[index] = value;
            self->update_cgb_color(self, false, index);
            if (ctx->bg_palette_index & 0x80) {
                ctx->bg_palette_index = 0x80 | ((index + 1) & 0x3F);
            }
            return;
        }
        case LCD_OCPS: ctx->sprite_palette_index = value & 0xBF; return;
        case LCD_OCPD: {
            uint8_t index = ctx->sprite_palette_index & 0x3F;
            ctx->sprite_palette_data[index] = value;
            self->update_cgb_color(self, true, index);
            if (ctx->sprite_palette_index & 0x80) {
                ctx->sprite_palette_index = 0x80 | ((index + 1) & 0x3F);
            }
            return;
        }
        case LCD_OPRI: ctx->opri = value & 0x01; return;
        case INFRARED_PORT: ctx->infrared = value & 0xC1; return;
        default: return;
    }
}

static void update(LCDClass *self, uint8_t data, uint8_t palette)
{
    uint32_t *palette_colors = self->context->bg_colors;
    const uint32_t *shades =
        self->dmg_palettes[atomic_load(&self->parent->context->palette)];

    switch (palette) {
        case 1: palette_colors = self->context->sprite1_colors; break;
        case 2: palette_colors = self->context->sprite2_colors; break;
    }
    for (int32_t i = 0; i < 4; i++) {
        palette_colors[i] = shades[(data >> (i * 2)) & 0b11];
    }
}

static void serialize(LCDClass *self, SnapshotClass *snapshot)
{
    snapshot->field(snapshot, self->context, sizeof(*self->context));
}

const LCDClass init_lcd = {
    {
        ._size = sizeof(LCDClass),
        ._name = "LCD",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .dmg_palettes =
        {
            [PALETTE_GRAY] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000},
            [PALETTE_GREEN] = {0xFFE0F8D0, 0xFF88C070, 0xFF346856, 0xFF081820},
        },
    .read = read,
    .write = write,
    .update = update,
    .update_cgb_color = update_cgb_color,
    .refresh_colors = refresh_colors,
    .read_status = read_status,
    .hdma_start = hdma_start,
    .hdma_block = hdma_block,
    .hdma_hblank = hdma_hblank,
    .serialize = serialize,
};

const class_t *LCD = (const class_t *) &init_lcd;
