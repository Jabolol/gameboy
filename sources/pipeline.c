#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    PipelineClass *self = (PipelineClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    PipelineClass *self = (PipelineClass *) ptr;
    if (self->parent->ppu->context != NULL) {
        self->fifo_reset(self);
    }
}

static void fifo_push(PipelineClass *self, uint32_t value)
{
    fifo_entry_t *next = calloc(1, sizeof(*next));

    if (!next) {
        HANDLE_ERROR("failed memory allocation");
    }
    next->value = value;

    if (!self->parent->ppu->context->pixel_context->pixel_fifo.head)
        self->parent->ppu->context->pixel_context->pixel_fifo.head =
            self->parent->ppu->context->pixel_context->pixel_fifo.tail = next;
    else {
        self->parent->ppu->context->pixel_context->pixel_fifo.tail->next =
            next;
        self->parent->ppu->context->pixel_context->pixel_fifo.tail = next;
    }

    self->parent->ppu->context->pixel_context->pixel_fifo.size++;
}

static uint32_t fetch_sprite_pixels(
    PipelineClass *self, int32_t bit, uint32_t color, uint8_t bg_color)
{
    for (int32_t i = 0; i < self->parent->ppu->context->fetch_entry_count;
        i++) {
        int32_t sp_x = (self->parent->ppu->context->fetched_entries[i].x - 8)
            + (self->parent->lcd->context->scroll_x % 8);

        if (sp_x + 8 < self->parent->ppu->context->pixel_context->fifo_x) {
            continue;
        }

        int32_t offset =
            self->parent->ppu->context->pixel_context->fifo_x - sp_x;

        if (offset < 0 || offset > 7) {
            continue;
        }

        bit = (7 - offset);

        if (self->parent->ppu->context->fetched_entries[i].f_x_flip) {
            bit = offset;
        }

        uint8_t hi = !!(
            self->parent->ppu->context->pixel_context->fetch_entry_data[i * 2]
            & (1 << bit));

        uint8_t lo = !!(self->parent->ppu->context->pixel_context
                             ->fetch_entry_data[(i * 2) + 1]
                         & (1 << bit))
            << 1;

        bool bg_priority =
            self->parent->ppu->context->fetched_entries[i].f_bgp;

        if (!(hi | lo)) {
            continue;
        }

        if (!bg_priority || bg_color == 0) {
            color = (self->parent->ppu->context->fetched_entries[i].f_pn)
                ? self->parent->lcd->context->sprite2_colors[hi | lo]
                : self->parent->lcd->context->sprite1_colors[hi | lo];

            if (hi | lo) {
                break;
            }
        }
    }

    return color;
}

static uint32_t fifo_pop(PipelineClass *self)
{
    uint32_t value = 0;
    fifo_entry_t *popped = NULL;

    if (self->parent->ppu->context->pixel_context->pixel_fifo.size <= 0)
        HANDLE_ERROR("invalid pixel fifo size");

    popped = self->parent->ppu->context->pixel_context->pixel_fifo.head;
    value = popped->value;

    self->parent->ppu->context->pixel_context->pixel_fifo.head = popped->next;
    self->parent->ppu->context->pixel_context->pixel_fifo.size--;

    free(popped);

    return value;
}

static bool fifo_add(PipelineClass *self)
{
    if (self->parent->ppu->context->pixel_context->pixel_fifo.size
        > MAX_FIFO_ITEMS) {
        return false;
    }

    int32_t x = self->parent->ppu->context->pixel_context->fetch_x
        - (MAX_FIFO_ITEMS - (self->parent->lcd->context->scroll_x % 8));

    for (int8_t i = 0; i < MAX_FIFO_ITEMS; i++) {
        int8_t bit = 7 - i;
        uint8_t hi =
            !!(self->parent->ppu->context->pixel_context->bg_fetch_data[1]
                & (1 << bit));
        uint8_t lo =
            !!(self->parent->ppu->context->pixel_context->bg_fetch_data[2]
                & (1 << bit))
            << 1;
        uint32_t color = self->parent->lcd->context->bg_colors[hi | lo];

        if (!LCDC_BGW_ENABLE) {
            color = self->parent->lcd->context->bg_colors[0];
        }

        if (LCDC_OBJ_ENABLE) {
            color = self->fetch_sprite_pixels(self, bit, color, hi | lo);
        }

        if (x >= 0) {
            self->fifo_push(self, color);
            self->parent->ppu->context->pixel_context->fifo_x++;
        }
    }
    return true;
}

static void load_sprite_tile(PipelineClass *self)
{
    oam_line_entry_t *line_entry = self->parent->ppu->context->line_sprites;

    while (line_entry) {
        int32_t sp_x = (line_entry->entry.x - 8)
            + (self->parent->lcd->context->scroll_x % 8);

        bool fits_one = (sp_x
                >= self->parent->ppu->context->pixel_context->fetch_x
            && sp_x < self->parent->ppu->context->pixel_context->fetch_x + 8);
        bool fits_two =
            ((sp_x + 8) >= self->parent->ppu->context->pixel_context->fetch_x
                && (sp_x + 8)
                    < self->parent->ppu->context->pixel_context->fetch_x + 8);
        bool fits = fits_one || fits_two;

        if (fits) {
            self->parent->ppu->context->fetched_entries[self->parent->ppu
                    ->context->fetch_entry_count++] = line_entry->entry;
        }

        line_entry = line_entry->next;

        if (!line_entry
            || self->parent->ppu->context->fetch_entry_count >= 3) {
            break;
        }
    }
}

static void load_sprite_data(PipelineClass *self, uint8_t offset)
{
    int32_t current_y = self->parent->lcd->context->y_coord;
    uint8_t sprite_height = LCDC_OBJ_HEIGHT;

    for (int32_t i = 0; i < self->parent->ppu->context->fetch_entry_count;
        i++) {
        uint8_t tile_y =
            ((current_y + 16)
                - self->parent->ppu->context->fetched_entries[i].y)
            * 2;

        if (self->parent->ppu->context->fetched_entries[i].f_y_flip) {
            tile_y = ((sprite_height * 2) - 2) - tile_y;
        }

        uint8_t tile_index =
            self->parent->ppu->context->fetched_entries[i].tile;

        if (sprite_height == 16) {
            tile_index &= ~(1);
        }

        self->parent->ppu->context->pixel_context
            ->fetch_entry_data[(i * 2) + offset] = self->parent->bus->read(
            self->parent->bus, 0x8000 + (tile_index * 16) + tile_y + offset);
    }
}

static void fetch(PipelineClass *self)
{
    switch (self->parent->ppu->context->pixel_context->state) {
        case FS_TILE: {
            self->parent->ppu->context->fetch_entry_count = 0;
            if (LCDC_BGW_ENABLE) {
                uint32_t map_x =
                    self->parent->ppu->context->pixel_context->map_x;
                uint32_t map_y =
                    self->parent->ppu->context->pixel_context->map_y;
                uint32_t map_offset =
                    LCDC_BG_MAP_AREA + (map_x / 8) + ((map_y / 8) * 32);

                self->parent->ppu->context->pixel_context->bg_fetch_data[0] =
                    self->parent->bus->read(self->parent->bus, map_offset);

                if (LCDC_BGW_DATA_AREA == 0x8800) {
                    self->parent->ppu->context->pixel_context
                        ->bg_fetch_data[0] += 128;
                }

                self->load_window_tile(self);
            }

            if (LCDC_OBJ_ENABLE && self->parent->ppu->context->line_sprites) {
                self->load_sprite_tile(self);
            }

            self->parent->ppu->context->pixel_context->state = FS_DATA0;
            self->parent->ppu->context->pixel_context->fetch_x += 8;
            break;
        }
        case FS_DATA0: {
            uint32_t bgw_fetch_data =
                self->parent->ppu->context->pixel_context->bg_fetch_data[0];
            uint32_t tile_y =
                self->parent->ppu->context->pixel_context->tile_y;

            self->parent->ppu->context->pixel_context->bg_fetch_data[1] =
                self->parent->bus->read(self->parent->bus,
                    LCDC_BGW_DATA_AREA + (bgw_fetch_data * 16) + tile_y);

            self->load_sprite_data(self, 0);

            self->parent->ppu->context->pixel_context->state = FS_DATA1;
            break;
        }
        case FS_DATA1: {
            uint32_t bgw_fetch_data =
                self->parent->ppu->context->pixel_context->bg_fetch_data[0];
            uint32_t tile_y =
                self->parent->ppu->context->pixel_context->tile_y;

            self->parent->ppu->context->pixel_context->bg_fetch_data[2] =
                self->parent->bus->read(self->parent->bus,
                    LCDC_BGW_DATA_AREA + (bgw_fetch_data * 16) + tile_y + 1);

            self->load_sprite_data(self, 1);

            self->parent->ppu->context->pixel_context->state = FS_IDLE;
            break;
        }
        case FS_IDLE: {
            self->parent->ppu->context->pixel_context->state = FS_PUSH;
            break;
        }
        case FS_PUSH: {
            if (self->fifo_add(self)) {
                self->parent->ppu->context->pixel_context->state = FS_TILE;
            }
            break;
        }
    }
}

static void push_pixel(PipelineClass *self)
{
    if (self->parent->ppu->context->pixel_context->pixel_fifo.size
        > MAX_FIFO_ITEMS) {
        uint32_t pixel_data = self->fifo_pop(self);
        if (self->parent->ppu->context->pixel_context->line_x
            >= self->parent->lcd->context->scroll_x % 8) {
            self->parent->ppu->context->video_buffer
                [self->parent->ppu->context->pixel_context->pushed_x
                    + self->parent->lcd->context->y_coord * X_RES] =
                pixel_data;
            self->parent->ppu->context->pixel_context->pushed_x++;
        }
        self->parent->ppu->context->pixel_context->line_x++;
    }
}

static void process(PipelineClass *self)
{
    self->parent->ppu->context->pixel_context->map_y =
        self->parent->lcd->context->y_coord
        + self->parent->lcd->context->scroll_y;
    self->parent->ppu->context->pixel_context->map_x =
        self->parent->ppu->context->pixel_context->fetch_x
        + self->parent->lcd->context->scroll_x;
    self->parent->ppu->context->pixel_context->tile_y =
        ((self->parent->lcd->context->y_coord
             + self->parent->lcd->context->scroll_y)
            % 8)
        * 2;

    if (!(self->parent->ppu->context->line_ticks & 1)) {
        self->fetch(self);
    }
    self->push_pixel(self);
}

static void fifo_reset(PipelineClass *self)
{
    while (self->parent->ppu->context->pixel_context->pixel_fifo.size) {
        self->fifo_pop(self);
    }
    self->parent->ppu->context->pixel_context->pixel_fifo.head = NULL;
}

static void load_window_tile(PipelineClass *self)
{
    if (!self->visible(self)) {
        return;
    }

    uint8_t window_y = self->parent->lcd->context->window_y;

    if (self->parent->ppu->context->pixel_context->fetch_x + 7
            >= self->parent->lcd->context->window_x
        && self->parent->ppu->context->pixel_context->fetch_x + 7
            < self->parent->lcd->context->window_x + Y_RES + 14) {
        if (self->parent->lcd->context->y_coord >= window_y
            && self->parent->lcd->context->y_coord < window_y + X_RES) {
            uint8_t tile_y = self->parent->ppu->context->window_line / 8;

            self->parent->ppu->context->pixel_context->bg_fetch_data[0] =
                self->parent->bus->read(self->parent->bus,
                    LCDC_WIN_MAP_AREA
                        + ((self->parent->ppu->context->pixel_context->fetch_x
                               + 7 - self->parent->lcd->context->window_x)
                            / 8)
                        + (tile_y * 32));

            if (LCDC_BGW_DATA_AREA == 0x8800) {
                self->parent->ppu->context->pixel_context->bg_fetch_data[0] +=
                    128;
            }
        }
    }
}

static bool visible(PipelineClass *self)
{
    return LCDC_WIN_ENABLE && self->parent->lcd->context->window_x <= 166
        && self->parent->lcd->context->window_y < Y_RES;
}

const PipelineClass init_pipeline = {
    {
        ._size = sizeof(PipelineClass),
        ._name = "Pipeline",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .fetch = fetch,
    .process = process,
    .fifo_pop = fifo_pop,
    .fifo_push = fifo_push,
    .fifo_add = fifo_add,
    .fifo_reset = fifo_reset,
    .push_pixel = push_pixel,
    .fetch_sprite_pixels = fetch_sprite_pixels,
    .load_sprite_data = load_sprite_data,
    .load_sprite_tile = load_sprite_tile,
    .load_window_tile = load_window_tile,
    .visible = visible,
};

const class_t *Pipeline = (const class_t *) &init_pipeline;
