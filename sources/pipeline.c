#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    PipelineClass *self = (PipelineClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static uint8_t reverse_bits(uint8_t value)
{
    value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);
    value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
    value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
    return value;
}

static bool window_visible(PipelineClass *self)
{
    ppu_context_t *ppu = self->parent->ppu->context;
    lcd_context_t *lcd = self->parent->lcd->context;

    return ppu->window_y_triggered && (lcd->control & 0x20)
        && lcd->window_x <= 166;
}

static void load_sprite_data(PipelineClass *self)
{
    ppu_context_t *ppu = self->parent->ppu->context;
    uint8_t line = self->parent->lcd->context->y_coord;
    uint8_t height = LCDC_OBJ_HEIGHT;
    bool cgb = self->parent->context->hw_mode == HW_CGB;

    for (uint8_t i = 0; i < ppu->line_sprite_count; i++) {
        oam_line_entry_t *sprite = &ppu->line_entry_array[i];
        uint8_t attributes = sprite->entry.attributes;
        uint8_t row = (line + 16 - sprite->entry.y) & (height - 1);
        uint8_t tile = sprite->entry.tile;

        if (attributes & 0x40) {
            row = height - 1 - row;
        }
        if (height == 16) {
            tile = (tile & 0xFE) | (row >= 8 ? 1 : 0);
            row &= 7;
        }

        uint16_t address = (tile * 16) + (row * 2);
        if (cgb && (attributes & 0x08)) {
            address += 0x2000;
        }
        sprite->data_lo = ppu->vram[address];
        sprite->data_hi = ppu->vram[address + 1];
        if (attributes & 0x20) {
            sprite->data_lo = reverse_bits(sprite->data_lo);
            sprite->data_hi = reverse_bits(sprite->data_hi);
        }
    }
}

static void rasterize_sprites(PipelineClass *self)
{
    ppu_context_t *ppu = self->parent->ppu->context;
    fifo_context_t *fifo = ppu->pixel_context;

    memset(fifo->obj_index, 0, sizeof(fifo->obj_index));

    for (uint8_t i = 0; i < ppu->line_sprite_count; i++) {
        oam_line_entry_t *sprite = &ppu->line_entry_array[i];
        int16_t left = sprite->entry.x - 8;

        for (uint8_t column = 0; column < 8; column++) {
            int16_t x = left + column;
            if (x < 0 || x >= X_RES || fifo->obj_index[x]) {
                continue;
            }
            uint8_t bit = 7 - column;
            uint8_t index = (((sprite->data_hi >> bit) & 1) << 1)
                | ((sprite->data_lo >> bit) & 1);
            if (index) {
                fifo->obj_index[x] = index;
                fifo->obj_attrs[x] = sprite->entry.attributes;
            }
        }
    }
}

static void line_start(PipelineClass *self)
{
    fifo_context_t *fifo = self->parent->ppu->context->pixel_context;

    fifo->render_x = 0;
    self->load_sprite_data(self);
    self->rasterize_sprites(self);
}

static void fetch_tile(
    PipelineClass *self, uint16_t map_address, uint8_t row, uint8_t tile[3])
{
    uint8_t *vram = self->parent->ppu->context->vram;
    bool cgb = self->parent->context->hw_mode == HW_CGB;
    uint8_t index = vram[map_address];
    uint8_t attributes = cgb ? vram[0x2000 + map_address] : 0;
    uint16_t address = (self->parent->lcd->context->control & 0x10)
        ? index * 16
        : 0x1000 + ((int8_t) index * 16);

    if (attributes & 0x40) {
        row = 7 - row;
    }
    address += (row * 2) + ((attributes & 0x08) ? 0x2000 : 0);
    tile[0] = vram[address];
    tile[1] = vram[address + 1];
    tile[2] = attributes;
    if (attributes & 0x20) {
        tile[0] = reverse_bits(tile[0]);
        tile[1] = reverse_bits(tile[1]);
    }
}

static void render_background(PipelineClass *self, uint8_t start, uint8_t end)
{
    ppu_context_t *ppu = self->parent->ppu->context;
    lcd_context_t *lcd = self->parent->lcd->context;
    fifo_context_t *fifo = ppu->pixel_context;
    bool blank =
        self->parent->context->hw_mode == HW_DMG && !(lcd->control & 0x01);
    bool window = self->window_visible(self);
    int16_t window_start = lcd->window_x - 7;
    int16_t cached = -1;
    uint8_t tile[3] = {0};

    for (uint8_t x = start; x < end; x++) {
        bool in_window = window && x >= window_start;
        uint16_t map =
            (lcd->control & (in_window ? 0x40 : 0x08)) ? 0x1C00 : 0x1800;
        uint8_t map_x = in_window ? x - window_start : x + lcd->scroll_x;
        uint8_t map_y =
            in_window ? ppu->window_line : lcd->y_coord + lcd->scroll_y;
        int16_t key = (in_window << 8) | (map_x >> 3);

        if (key != cached) {
            self->fetch_tile(self, map + ((map_y >> 3) * 32) + (map_x >> 3),
                map_y & 7, tile);
            cached = key;
        }
        ppu->window_rendered_this_line |= in_window;

        uint8_t bit = 7 - (map_x & 7);
        fifo->bg_index[x] =
            blank ? 0 : (((tile[1] >> bit) & 1) << 1) | ((tile[0] >> bit) & 1);
        fifo->bg_attrs[x] = tile[2];
    }
}

static uint32_t mix_pixel(PipelineClass *self, uint8_t x)
{
    lcd_context_t *lcd = self->parent->lcd->context;
    fifo_context_t *fifo = self->parent->ppu->context->pixel_context;
    bool cgb = self->parent->context->hw_mode == HW_CGB;
    uint8_t bg_index = fifo->bg_index[x];
    uint8_t bg_attrs = fifo->bg_attrs[x];
    uint8_t index = fifo->obj_index[x];

    if (index && (lcd->control & 0x02)) {
        uint8_t attributes = fifo->obj_attrs[x];
        if (cgb) {
            bool bg_wins = (lcd->control & 0x01) && bg_index
                && ((bg_attrs & 0x80) || (attributes & 0x80));
            if (!bg_wins) {
                return lcd->sprite_colors_cgb[attributes & 0x07][index];
            }
        } else if (!(attributes & 0x80) || !bg_index) {
            return (attributes & 0x10) ? lcd->sprite2_colors[index]
                                       : lcd->sprite1_colors[index];
        }
    }

    return cgb ? lcd->bg_colors_cgb[bg_attrs & 0x07][bg_index]
               : lcd->bg_colors[bg_index];
}

static void render(PipelineClass *self, uint8_t end)
{
    ppu_context_t *ppu = self->parent->ppu->context;
    fifo_context_t *fifo = ppu->pixel_context;
    uint8_t start = fifo->render_x;

    if (end > X_RES) {
        end = X_RES;
    }
    if (start >= end) {
        return;
    }

    self->render_background(self, start, end);

    uint32_t *row =
        ppu->back_buffer + (self->parent->lcd->context->y_coord * X_RES);
    for (uint8_t x = start; x < end; x++) {
        row[x] = self->mix_pixel(self, x);
    }
    fifo->render_x = end;
}

const PipelineClass init_pipeline = {
    {
        ._size = sizeof(PipelineClass),
        ._name = "Pipeline",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .line_start = line_start,
    .load_sprite_data = load_sprite_data,
    .rasterize_sprites = rasterize_sprites,
    .render = render,
    .render_background = render_background,
    .fetch_tile = fetch_tile,
    .mix_pixel = mix_pixel,
    .window_visible = window_visible,
};

const class_t *Pipeline = (const class_t *) &init_pipeline;
