#include "../include/gameboy.h"

#define LY_INCREMENT_DOT (TICKS_PER_LINE - 4)
#define VRAM_BLOCK_DOT   (OAM_SCAN_TICKS - 4)
#define LCD_ON_DOT       2

static void constructor(void *ptr, va_list *args)
{
    PPUClass *self = (PPUClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    for (uint8_t i = 0; i < 2; i++) {
        if (!((self->context->frame_buffers[i] = malloc(Y_RES * X_RES
                   * sizeof(*self->context->frame_buffers[i]))))) {
            HANDLE_ERROR("failed memory allocation");
        }
        for (uint32_t p = 0; p < Y_RES * X_RES; p++) {
            self->context->frame_buffers[i][p] = 0xFFFFFFFF;
        }
    }
    if (!((self->context->pixel_context =
                calloc(1, sizeof(*self->context->pixel_context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
    self->context->video_buffer = self->context->frame_buffers[0];
    self->context->back_buffer = self->context->frame_buffers[1];

    self->context->line = LINES_PER_FRAME - 1;
    self->context->line_ticks = 400;
    self->context->phase = PHASE_LY_INCREMENT;
    self->context->next_event = LY_INCREMENT_DOT;
    self->parent->lcd->context->y_coord = 0;
    self->parent->lcd->context->status = 0x85;
    self->frame_duration = 0;
}

static void destructor(void *ptr)
{
    PPUClass *self = (PPUClass *) ptr;
    free(self->context->frame_buffers[0]);
    free(self->context->frame_buffers[1]);
    free(self->context->pixel_context);
    free(self->context);
}

static bool oam_accessible(PPUClass *self, bool write)
{
    return write ? !self->context->oam_write_blocked
                 : !self->context->oam_read_blocked;
}

static bool vram_accessible(PPUClass *self, bool write)
{
    return write ? !self->context->vram_write_blocked
                 : !self->context->vram_read_blocked;
}

static void block_access(PPUClass *self, bool oam_read, bool oam_write,
    bool vram_read, bool vram_write)
{
    self->context->oam_read_blocked = oam_read;
    self->context->oam_write_blocked = oam_write;
    self->context->vram_read_blocked = vram_read;
    self->context->vram_write_blocked = vram_write;
}

static void oam_write(PPUClass *self, uint16_t address, uint8_t value)
{
    ((uint8_t *) self->context->oam_ram)[(address - 0xFE00) % 0xA0] = value;
}

static uint8_t oam_read(PPUClass *self, uint16_t address)
{
    return ((uint8_t *) self->context->oam_ram)[(address - 0xFE00) % 0xA0];
}

static void vram_write(PPUClass *self, uint16_t address, uint8_t value)
{
    self->context
        ->vram[(self->context->vram_bank * 0x2000) + (address & 0x1FFF)] =
        value;
}

static uint8_t vram_read(PPUClass *self, uint16_t address)
{
    return self->context
        ->vram[(self->context->vram_bank * 0x2000) + (address & 0x1FFF)];
}

static void update_stat(PPUClass *self)
{
    lcd_context_t *lcd = self->parent->lcd->context;
    uint8_t status = lcd->status;
    lcd_mode_t mode = status & 0x03;

    if (!(lcd->control & 0x80)) {
        return;
    }

    bool line = ((status & SS_LYC) && (status & 0x04))
        || ((status & SS_HBLANK) && mode == MODE_HBLANK)
        || ((status & SS_VBLANK) && mode == MODE_VBLANK)
        || ((status & SS_OAM) && mode == MODE_OAM);

    if (line && !self->context->stat_line) {
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_LCD_STAT);
    }
    self->context->stat_line = line;
}

static void check_lyc(PPUClass *self)
{
    lcd_context_t *lcd = self->parent->lcd->context;

    if (!(lcd->control & 0x80)) {
        return;
    }
    BIT_SET(lcd->status, 2, lcd->y_coord == lcd->y_compare);
    self->update_stat(self);
}

static void stat_write_quirk(PPUClass *self)
{
    lcd_context_t *lcd = self->parent->lcd->context;

    if (!(lcd->control & 0x80)) {
        return;
    }

    bool line = (lcd->status & 0x03) != MODE_TRANSFER || (lcd->status & 0x04);
    if (line && !self->context->stat_line) {
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_LCD_STAT);
    }
    self->context->stat_line = line;
}

static void set_mode(PPUClass *self, lcd_mode_t mode)
{
    lcd_context_t *lcd = self->parent->lcd->context;

    lcd->status = (lcd->status & ~0x03) | mode;
    self->update_stat(self);
}

static void scan_oam(PPUClass *self, uint8_t until)
{
    ppu_context_t *ctx = self->context;
    int16_t line = self->parent->lcd->context->y_coord + 16;
    uint8_t height = LCDC_OBJ_HEIGHT;
    bool dma = self->parent->dma->transferring(self->parent->dma);

    for (; ctx->scan_index < until; ctx->scan_index++) {
        oam_entry_t *entry = &ctx->oam_ram[ctx->scan_index];

        if (dma || ctx->line_sprite_count >= MAX_SPRITES || line < entry->y
            || line >= entry->y + height) {
            continue;
        }
        oam_line_entry_t *slot =
            &ctx->line_entry_array[ctx->line_sprite_count++];
        slot->entry = *entry;
        slot->index = ctx->scan_index;
    }
}

static void load_line_sprites(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    bool oam_priority = self->parent->context->hw_mode == HW_CGB
        && !(self->parent->lcd->context->opri & 0x01);

    self->scan_oam(self, OAM_ENTRIES);
    if (oam_priority) {
        return;
    }

    for (uint8_t i = 1; i < ctx->line_sprite_count; i++) {
        oam_line_entry_t current = ctx->line_entry_array[i];
        uint8_t position = i;
        while (position > 0
            && ctx->line_entry_array[position - 1].entry.x > current.entry.x) {
            ctx->line_entry_array[position] =
                ctx->line_entry_array[position - 1];
            position--;
        }
        ctx->line_entry_array[position] = current;
    }
}

static uint32_t transfer_length(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    lcd_context_t *lcd = self->parent->lcd->context;
    bool window =
        self->parent->pipeline->window_visible(self->parent->pipeline);
    int16_t window_start = lcd->window_x - 7;
    uint32_t length = TRANSFER_TICKS + (lcd->scroll_x & 7) + (window ? 6 : 0);
    uint32_t penalty = 0;
    int16_t last_tile = INT16_MIN;

    if (!(lcd->control & 0x02)) {
        return length;
    }
    for (int16_t x = 0; x < 168; x++) {
        for (uint8_t i = 0; i < ctx->line_sprite_count; i++) {
            if (ctx->line_entry_array[i].entry.x != x) {
                continue;
            }
            int16_t pixel = -8;
            if (x && window && x - 8 >= window_start) {
                pixel = 0x800 + x - 8 - window_start;
            } else if (x) {
                pixel = x - 8 + lcd->scroll_x;
            }
            uint8_t right = 7 - (pixel & 7);
            if (pixel >> 3 != last_tile) {
                last_tile = pixel >> 3;
                penalty += right > 2 ? right - 2 : 0;
            }
            penalty += 6;
        }
    }
    return length + (penalty ? penalty - 1 : 0);
}

static void frame_complete(PPUClass *self)
{
    ppu_context_t *ctx = self->context;

    if (ctx->first_frame) {
        ctx->first_frame = false;
    } else {
        uint32_t *front = ctx->back_buffer;
        ctx->back_buffer = ctx->video_buffer;
        ctx->video_buffer = front;
        atomic_fetch_add(&ctx->current_frame, 1);
    }

    self->frame_count += 1;
    if (self->frame_count % 60 == 0
        && self->parent->cartridge->context->needs_save) {
        self->parent->cartridge->save_battery(self->parent->cartridge);
    }

    if (atomic_load_explicit(
            &self->parent->context->turbo, memory_order_relaxed)) {
        return;
    }

    uint64_t frequency = SDL_GetPerformanceFrequency();
    uint64_t now = SDL_GetPerformanceCounter();

    if (!self->frame_duration) {
        self->frame_duration = (frequency * TICKS_PER_FRAME) / CLOCK_SPEED;
        self->frame_deadline = now;
    }
    self->frame_deadline += self->frame_duration;

    if (now + frequency / 10 < self->frame_deadline
        || now > self->frame_deadline + frequency / 10) {
        self->frame_deadline = now;
        return;
    }
    if (now < self->frame_deadline) {
        uint32_t ms = ((self->frame_deadline - now) * 1000) / frequency;
        if (ms > 0) {
            self->parent->ui->delay(ms);
        }
    }
}

static void mode_oam(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    lcd_context_t *lcd = self->parent->lcd->context;

    ctx->window_rendered_this_line = false;
    ctx->scan_index = 0;
    ctx->line_sprite_count = 0;
    self->block_access(self, true, true, false, false);
    if (lcd->y_coord == lcd->window_y) {
        ctx->window_y_triggered = true;
    }
    lcd->status = (lcd->status & ~0x03) | MODE_OAM;
    self->check_lyc(self);
}

static void mode_transfer(PPUClass *self)
{
    ppu_context_t *ctx = self->context;

    self->block_access(self, true, true, true, true);
    self->load_line_sprites(self);
    self->parent->pipeline->line_start(self->parent->pipeline);
    ctx->transfer_ticks = self->transfer_length(self);
    self->set_mode(self, MODE_TRANSFER);
}

static void mode_hblank(PPUClass *self)
{
    self->parent->pipeline->render(self->parent->pipeline, X_RES);
    self->block_access(self, false, false, false, false);
    self->set_mode(self, MODE_HBLANK);
    self->parent->lcd->hdma_hblank(self->parent->lcd);
}

static void mode_vblank(PPUClass *self)
{
    lcd_context_t *lcd = self->parent->lcd->context;

    self->block_access(self, false, false, false, false);
    lcd->status = (lcd->status & ~0x03) | MODE_VBLANK;
    self->parent->cpu->request_interrupt(self->parent->cpu, IT_VBLANK);

    if ((lcd->status & SS_OAM) && !self->context->stat_line) {
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_LCD_STAT);
        self->context->stat_line = true;
    }
    self->check_lyc(self);
    self->frame_complete(self);
}

static void increment_y(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    lcd_context_t *lcd = self->parent->lcd->context;
    uint8_t next = ctx->line + 1;

    if (ctx->line == LINES_PER_FRAME - 1) {
        next = 0;
    }
    if (lcd->y_coord != next) {
        lcd->y_coord = next;
        BIT_SET(lcd->status, 2, 0);
        self->update_stat(self);
    }
    if (next < Y_RES) {
        ctx->oam_read_blocked = true;
    }
}

static void schedule(PPUClass *self, ppu_phase_t phase, int32_t dot)
{
    self->context->phase = phase;
    self->context->next_event = dot;
}

static void end_line(PPUClass *self)
{
    ppu_context_t *ctx = self->context;

    ctx->line_ticks -= TICKS_PER_LINE;
    ctx->line = (ctx->line + 1) % LINES_PER_FRAME;
    ctx->window_line += ctx->window_rendered_this_line;
    ctx->window_rendered_this_line = false;
    if (ctx->line == 0) {
        ctx->window_line = 0;
        ctx->window_y_triggered = false;
    }
    if (ctx->line < Y_RES) {
        schedule(self, PHASE_OAM_SCAN, 0);
    } else if (ctx->line == Y_RES) {
        schedule(self, PHASE_VBLANK_START, 0);
    } else {
        schedule(self, PHASE_VBLANK_LINE, 0);
    }
}

static void process_event(PPUClass *self)
{
    ppu_context_t *ctx = self->context;

    switch (ctx->phase) {
        case PHASE_OAM_SCAN: {
            self->mode_oam(self);
            schedule(self, PHASE_VRAM_BLOCK, VRAM_BLOCK_DOT);
            break;
        }
        case PHASE_VRAM_BLOCK: {
            ctx->vram_read_blocked = true;
            ctx->oam_write_blocked = false;
            schedule(self, PHASE_TRANSFER, OAM_SCAN_TICKS);
            break;
        }
        case PHASE_TRANSFER: {
            self->mode_transfer(self);
            schedule(self, PHASE_HBLANK, OAM_SCAN_TICKS + ctx->transfer_ticks);
            break;
        }
        case PHASE_HBLANK: {
            self->mode_hblank(self);
            schedule(self, PHASE_LY_INCREMENT, LY_INCREMENT_DOT);
            break;
        }
        case PHASE_LY_INCREMENT: {
            self->increment_y(self);
            schedule(self, PHASE_LINE_END, TICKS_PER_LINE);
            break;
        }
        case PHASE_LINE_END: self->end_line(self); break;
        case PHASE_VBLANK_START: {
            self->mode_vblank(self);
            schedule(self, PHASE_LY_INCREMENT, LY_INCREMENT_DOT);
            break;
        }
        case PHASE_VBLANK_LINE: {
            self->check_lyc(self);
            if (ctx->line == LINES_PER_FRAME - 1) {
                schedule(self, PHASE_LY_ZERO, 4);
            } else {
                schedule(self, PHASE_LY_INCREMENT, LY_INCREMENT_DOT);
            }
            break;
        }
        case PHASE_LY_ZERO: {
            self->parent->lcd->context->y_coord = 0;
            BIT_SET(self->parent->lcd->context->status, 2, 0);
            self->update_stat(self);
            schedule(self, PHASE_LY_ZERO_COMPARE, 8);
            break;
        }
        case PHASE_LY_ZERO_COMPARE: {
            self->check_lyc(self);
            schedule(self, PHASE_LY_INCREMENT, LY_INCREMENT_DOT);
            break;
        }
    }
}

static void tick(PPUClass *self, uint32_t dots)
{
    ppu_context_t *ctx = self->context;

    if (!(self->parent->lcd->context->control & 0x80)) {
        return;
    }
    ctx->line_ticks += dots;
    while (ctx->line_ticks >= ctx->next_event) {
        self->process_event(self);
    }
    if ((self->parent->lcd->context->status & 0x03) == MODE_OAM
        && ctx->scan_index < OAM_ENTRIES) {
        int32_t until = ctx->line_ticks / 2;
        self->scan_oam(self, until > OAM_ENTRIES ? OAM_ENTRIES : until);
    }
}

static void sync(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    lcd_context_t *lcd = self->parent->lcd->context;

    if (!(lcd->control & 0x80) || (lcd->status & 0x03) != MODE_TRANSFER) {
        return;
    }

    int32_t x = ctx->line_ticks - OAM_SCAN_TICKS - 12 - (lcd->scroll_x & 7);
    if (x > 0) {
        self->parent->pipeline->render(
            self->parent->pipeline, x > X_RES ? X_RES : x);
    }
}

static void lcd_off(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    lcd_context_t *lcd = self->parent->lcd->context;

    ctx->line = 0;
    ctx->line_ticks = 0;
    self->block_access(self, false, false, false, false);
    ctx->window_line = 0;
    ctx->window_y_triggered = false;
    lcd->y_coord = 0;
    lcd->status &= ~0x03;

    for (uint32_t p = 0; p < Y_RES * X_RES; p++) {
        ctx->back_buffer[p] = 0xFFFFFFFF;
    }
    uint32_t *front = ctx->back_buffer;
    ctx->back_buffer = ctx->video_buffer;
    ctx->video_buffer = front;
    atomic_fetch_add(&ctx->current_frame, 1);
}

static void lcd_on(PPUClass *self)
{
    ppu_context_t *ctx = self->context;
    lcd_context_t *lcd = self->parent->lcd->context;

    ctx->line = 0;
    ctx->scan_index = 0;
    ctx->line_sprite_count = 0;
    ctx->line_ticks = LCD_ON_DOT;
    ctx->first_frame = true;
    ctx->window_rendered_this_line = false;
    ctx->window_y_triggered = lcd->window_y == 0;
    lcd->y_coord = 0;
    lcd->status &= ~0x03;
    self->check_lyc(self);
    schedule(self, PHASE_TRANSFER, OAM_SCAN_TICKS);
}

static void serialize(PPUClass *self, SnapshotClass *snapshot)
{
    ppu_context_t *ctx = self->context;
    fifo_context_t *fifo = ctx->pixel_context;
    uint32_t *front = ctx->video_buffer;
    uint32_t *back = ctx->back_buffer;

    snapshot->field(snapshot, ctx, sizeof(*ctx));
    ctx->pixel_context = fifo;
    ctx->video_buffer = front;
    ctx->back_buffer = back;
    ctx->frame_buffers[0] = front;
    ctx->frame_buffers[1] = back;
    snapshot->field(snapshot, fifo, sizeof(*fifo));
    snapshot->field(snapshot, back, Y_RES * X_RES * sizeof(*back));
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
    .set_mode = set_mode,
    .increment_y = increment_y,
    .mode_hblank = mode_hblank,
    .mode_vblank = mode_vblank,
    .mode_oam = mode_oam,
    .mode_transfer = mode_transfer,
    .update_stat = update_stat,
    .check_lyc = check_lyc,
    .stat_write_quirk = stat_write_quirk,
    .lcd_on = lcd_on,
    .lcd_off = lcd_off,
    .sync = sync,
    .transfer_length = transfer_length,
    .frame_complete = frame_complete,
    .end_line = end_line,
    .process_event = process_event,
    .oam_accessible = oam_accessible,
    .vram_accessible = vram_accessible,
    .block_access = block_access,
    .scan_oam = scan_oam,
    .load_line_sprites = load_line_sprites,
    .serialize = serialize,
};

const class_t *PPU = (const class_t *) &init_ppu;
