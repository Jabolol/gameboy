#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    PPUClass *self = (PPUClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    if (!((self->context->video_buffer = calloc(
               Y_RES * X_RES, sizeof(*self->context->video_buffer))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    if (!((self->context->pixel_context =
                calloc(1, sizeof(*self->context->pixel_context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
    self->target_time = 1000 / va_arg(*args, size_t);
    self->parent->lcd->context->status &= ~0b11;
    self->parent->lcd->context->status |= MODE_OAM;
    self->context->pixel_context->state = FS_TILE;
    self->context->line_sprites = 0;
    self->context->fetch_entry_count = 0;
}

static void destructor(void *ptr)
{
    PPUClass *self = (PPUClass *) ptr;
    free(self->context->video_buffer);
    free(self->context->pixel_context);
    free(self->context);
}

static void oam_write(PPUClass *self, uint16_t address, uint8_t value)
{
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    uint8_t *bytes = (uint8_t *) self->context->oam_ram;

    if (address > sizeof(self->context->oam_ram) - 1) {
        HANDLE_ERROR("critical memory overflow");
    }
    bytes[address] = value;
}

static uint8_t oam_read(PPUClass *self, uint16_t address)
{
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    uint8_t *bytes = (uint8_t *) self->context->oam_ram;

    if (address > sizeof(self->context->oam_ram) - 1) {
        HANDLE_ERROR("critical memory overflow");
    }
    return bytes[address];
}

static void vram_write(PPUClass *self, uint16_t address, uint8_t value)
{
    uint16_t offset = (self->context->vram_bank * 0x2000) + (address - 0x8000);
    self->context->vram[offset] = value;
}

static uint8_t vram_read(PPUClass *self, uint16_t address)
{
    uint16_t offset = (self->context->vram_bank * 0x2000) + (address - 0x8000);
    return self->context->vram[offset];
}

static void tick(PPUClass *self)
{
    self->context->line_ticks += 1;

    switch (((lcd_mode_t) (self->parent->lcd->context->status & 0b11))) {
        case MODE_HBLANK: self->mode_hblank(self); break;
        case MODE_VBLANK: self->mode_vblank(self); break;
        case MODE_OAM: self->mode_oam(self); break;
        case MODE_TRANSFER: self->mode_transfer(self); break;
    }
}

static void increment_y(PPUClass *self)
{
    if (self->context->window_rendered_this_line) {
        self->parent->ppu->context->window_line += 1;
    }

    self->parent->lcd->context->y_coord += 1;

    if (self->parent->lcd->context->y_coord
        == self->parent->lcd->context->y_compare) {
        BIT_SET(self->parent->lcd->context->status, 2, 1);
        if (self->parent->lcd->context->status & SS_LYC) {
            self->parent->cpu->request_interrupt(
                self->parent->cpu, IT_LCD_STAT);
        }
    } else {
        BIT_SET(self->parent->lcd->context->status, 2, 0);
    }
}

static void mode_hblank(PPUClass *self)
{
    if (self->context->line_ticks < TICKS_PER_LINE) {
        return;
    }

    self->increment_y(self);

    if (self->parent->lcd->context->y_coord >= Y_RES) {
        self->parent->lcd->context->status &= ~0b11;
        self->parent->lcd->context->status |= MODE_VBLANK;
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_VBLANK);

        if (self->parent->lcd->context->status & SS_VBLANK) {
            self->parent->cpu->request_interrupt(
                self->parent->cpu, IT_LCD_STAT);
        }

        self->context->current_frame += 1;

        uint32_t end = self->parent->ui->get_ticks();
        uint32_t time = end - self->prev_time;

        if (time < self->target_time) {
            self->parent->ui->delay(self->target_time - time);
        }

        if (end - self->start_timer >= 1000) {
            self->start_timer = end;
            self->frame_count = 0;
            if (self->parent->cartridge->context->needs_save) {
                self->parent->cartridge->save_battery(self->parent->cartridge);
            }
        }

        self->frame_count += 1;
        self->prev_time = self->parent->ui->get_ticks();

    } else {
        self->parent->lcd->context->status &= ~0b11;
        self->parent->lcd->context->status |= MODE_OAM;
    }
    self->context->line_ticks = 0;
}

static void mode_vblank(PPUClass *self)
{
    if (self->context->line_ticks >= TICKS_PER_LINE) {
        self->increment_y(self);
        if (self->parent->lcd->context->y_coord >= LINES_PER_FRAME) {
            self->parent->lcd->context->status &= ~0b11;
            self->parent->lcd->context->status |= MODE_OAM;
            self->parent->lcd->context->y_coord = 0;
            self->parent->ppu->context->window_line = 0;
            self->context->window_triggered = false;
        }
        self->context->line_ticks = 0;
    }
}

static void load_line_sprites(PPUClass *self)
{
    int32_t current_y = self->parent->lcd->context->y_coord;
    uint8_t sprite_height = LCDC_OBJ_HEIGHT;

    memset(self->context->line_entry_array, 0,
        sizeof(self->context->line_entry_array));

    for (int32_t i = 0; i < OAM_ENTRIES; i++) {
        oam_entry_t *e = &self->context->oam_ram[i];
        bool visible =
            e->y <= current_y + 16 && e->y + sprite_height > current_y + 16;

        if (!e->x) {
            continue;
        }

        if (self->context->line_sprite_count >= MAX_SPRITES) {
            break;
        }

        if (visible) {
            oam_line_entry_t *entry =
                &self->context
                     ->line_entry_array[self->context->line_sprite_count++];

            entry->entry = *e;
            entry->next = NULL;

            if (self->parent->context->hw_mode == HW_CGB) {
                if (!self->context->line_sprites) {
                    self->context->line_sprites = entry;
                } else {
                    oam_line_entry_t *tail = self->context->line_sprites;
                    while (tail->next) {
                        tail = tail->next;
                    }
                    tail->next = entry;
                }
            } else {
                if (!self->context->line_sprites
                    || self->context->line_sprites->entry.x > e->x) {
                    entry->next = self->context->line_sprites;
                    self->context->line_sprites = entry;
                    continue;
                }

                oam_line_entry_t *line_entry = self->context->line_sprites;
                oam_line_entry_t *prev = line_entry;

                while (line_entry) {
                    if (line_entry->entry.x > e->x) {
                        prev->next = entry;
                        entry->next = line_entry;
                        break;
                    }
                    if (!line_entry->next) {
                        line_entry->next = entry;
                        break;
                    }
                    prev = line_entry;
                    line_entry = line_entry->next;
                }
            }
        }
    }
}

static void mode_oam(PPUClass *self)
{
    if (self->context->line_ticks >= 80) {
        self->parent->lcd->context->status &= ~0b11;
        self->parent->lcd->context->status |= MODE_TRANSFER;
        self->context->pixel_context->state = FS_TILE;
        self->context->pixel_context->line_x = 0;
        self->context->pixel_context->fetch_x = 0;
        self->context->pixel_context->pushed_x = 0;
        self->context->pixel_context->fifo_x = 0;
    }

    if (self->context->line_ticks == 1) {
        self->context->line_sprites = 0;
        self->context->line_sprite_count = 0;
        self->context->window_rendered_this_line = false;
        self->load_line_sprites(self);

        if (!self->context->window_triggered && LCDC_WIN_ENABLE
            && self->parent->lcd->context->y_coord
                == self->parent->lcd->context->window_y) {
            self->context->window_triggered = true;
        }
    }
}

static void mode_transfer(PPUClass *self)
{
    self->parent->pipeline->process(self->parent->pipeline);

    if (self->context->pixel_context->pushed_x >= X_RES) {
        self->parent->pipeline->fifo_reset(self->parent->pipeline);
        self->parent->lcd->context->status &= ~0b11;
        self->parent->lcd->context->status |= MODE_HBLANK;

        if (self->parent->lcd->context->status & SS_HBLANK) {
            self->parent->cpu->request_interrupt(
                self->parent->cpu, IT_LCD_STAT);
        }

        if (self->parent->context->hw_mode == HW_CGB
            && self->parent->lcd->context->hdma.active
            && self->parent->lcd->context->hdma.hblank_mode
            && self->parent->lcd->context->y_coord < Y_RES) {
            for (int i = 0; i < 0x10; i++) {
                self->parent->lcd->hdma_tick(self->parent->lcd);
            }
        }
    }
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
    .tick = tick,
    .increment_y = increment_y,
    .mode_hblank = mode_hblank,
    .mode_vblank = mode_vblank,
    .mode_oam = mode_oam,
    .mode_transfer = mode_transfer,
    .load_line_sprites = load_line_sprites,
};

const class_t *PPU = (const class_t *) &init_ppu;
