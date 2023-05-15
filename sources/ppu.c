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
    self->parent = va_arg(*args, GameboyClass *);
    self->target_time = 1000 / va_arg(*args, size_t);
    self->parent->lcd->context->status &= ~0b11;
    self->parent->lcd->context->status |= MODE_OAM;
}

static void destructor(void *ptr)
{
    PPUClass *self = (PPUClass *) ptr;
    free(self->context->video_buffer);
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
        }
        self->context->line_ticks = 0;
    }
}

static void mode_oam(PPUClass *self)
{
    if (self->context->line_ticks >= 80) {
        self->parent->lcd->context->status &= ~0b11;
        self->parent->lcd->context->status |= MODE_TRANSFER;
    }
}

static void mode_transfer(PPUClass *self)
{
    if (self->context->line_ticks >= 80 + 172) {
        self->parent->lcd->context->status &= ~0b11;
        self->parent->lcd->context->status |= MODE_HBLANK;
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
};

const class_t *PPU = (const class_t *) &init_ppu;
