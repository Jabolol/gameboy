#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif
#include "../include/gameboy.h"

static void constructor(void *ptr, va_list UNUSED *args)
{
    GameboyClass *self = (GameboyClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    atomic_init(&self->context->paused, false);
    atomic_init(&self->context->running, false);
    atomic_init(&self->context->die, false);
    atomic_init(&self->context->turbo, false);
    atomic_init(&self->context->requests, 0);
    atomic_init(&self->context->palette, PALETTE_GRAY);
    atomic_init(&self->context->color_correction, false);
    self->cartridge = new_class(Cartridge, self);
    self->ram = new_class(RAM);
    self->instructions = new_class(Instructions);
    self->bus = new_class(Bus, self);
    self->timer = new_class(Timer, self);
    self->cpu = new_class(CPU, self);
    self->stack = new_class(Stack, self);
    self->ui = new_class(UI, self, SCALE);
    self->io = new_class(IO, self);
    self->debug = new_class(Debug, self);
    self->lcd = new_class(LCD, self);
    self->ppu = new_class(PPU, self);
    self->dma = new_class(DMA, self);
    self->pipeline = new_class(Pipeline, self);
    self->joypad = new_class(Joypad, self);
    self->sound = new_class(Sound, self);
    self->snapshot = new_class(Snapshot, self);
}

static void destructor(void *ptr)
{
    GameboyClass *self = (GameboyClass *) ptr;
    destroy_class(self->cartridge);
    destroy_class(self->cpu);
    destroy_class(self->bus);
    destroy_class(self->instructions);
    destroy_class(self->ram);
    destroy_class(self->stack);
    destroy_class(self->io);
    destroy_class(self->debug);
    destroy_class(self->timer);
    destroy_class(self->pipeline);
    destroy_class(self->ppu);
    destroy_class(self->dma);
    destroy_class(self->lcd);
    destroy_class(self->joypad);
    destroy_class(self->sound);
    destroy_class(self->snapshot);
    destroy_class(self->ui);
    free(self->context);
}

static bool boot(GameboyClass *self, const char *path, const char *save)
{
    if (!self->cartridge->load(self->cartridge, path, save)) {
        return false;
    }

    rom_header_t *header = self->cartridge->context->header;
    bool cgb = header->cgb_flag & 0x80;
    CPUClass *cpu = self->cpu;

    self->context->hw_mode = cgb ? HW_CGB : HW_DMG;
    if (cgb) {
        cpu->set_register(cpu, RT_AF, 0x1180);
        cpu->set_register(cpu, RT_BC, 0x0000);
        cpu->set_register(cpu, RT_DE, 0xFF56);
        cpu->set_register(cpu, RT_HL, 0x000D);
        self->timer->context->div = 0x1EA0;
        self->lcd->context->dma = 0x00;
        self->lcd->context->opri = 0x00;
    } else {
        cpu->set_register(cpu, RT_AF, header->checksum ? 0x01B0 : 0x0180);
        cpu->set_register(cpu, RT_BC, 0x0013);
        cpu->set_register(cpu, RT_DE, 0x00D8);
        cpu->set_register(cpu, RT_HL, 0x014D);
        self->timer->context->div = 0xABCC;
    }
    cpu->context->registers.sp = 0xFFFE;
    cpu->context->registers.pc = 0x0100;
    cpu->set_int_flags(cpu, IT_VBLANK);
    self->io->serial->control = cgb ? 0x7F : 0x7E;
    self->sound->reset(self->sound);
    self->context->prev_frame = 0;

#ifndef __EMSCRIPTEN__
    self->ui->resize(self->ui);
#endif
    LOG(cgb ? "Running in CGB mode" : "Running in DMG mode");
    return true;
}

static void *cpu_run(void *ptr)
{
    GameboyClass *self = (GameboyClass *) ptr;

    atomic_store(&self->context->running, true);
    atomic_store(&self->context->paused, false);
    self->context->ticks = 0;

    while (
        atomic_load_explicit(&self->context->running, memory_order_relaxed)) {
        if (atomic_load_explicit(&self->context->die, memory_order_relaxed)) {
            break;
        }
        if (atomic_load_explicit(
                &self->context->paused, memory_order_relaxed)) {
            self->ui->delay(10);
            continue;
        }
        if (atomic_load_explicit(
                &self->context->requests, memory_order_relaxed)) {
            self->process_requests(self);
        }
        if (!self->cpu->step(self->cpu)) {
            LOG("CPU stopped");
            break;
        }
    }
    atomic_store(&self->context->running, false);
    return NULL;
}

static void loop(void *ptr)
{
    GameboyClass *self = (GameboyClass *) ptr;

#ifndef __EMSCRIPTEN__
    nanosleep(&(struct timespec) {.tv_nsec = 1000000}, NULL);
#endif
    self->ui->handle_events(self->ui);

    uint32_t frame = atomic_load(&self->ppu->context->current_frame);
    if (self->context->prev_frame != frame) {
        self->ui->update(self->ui);
        self->context->prev_frame = frame;
    }

#ifdef __EMSCRIPTEN__
    if (atomic_load(&self->context->die)) {
        emscripten_cancel_main_loop();
    }
#endif
}

static int32_t run(GameboyClass *self, int argc, char **argv)
{
    pthread_t thread;

    if (argc < 2) {
        fprintf(stderr, "Usage: ./gameboy /path/to/rom.gb\n");
        return 1;
    }

    if (!self->boot(self, argv[1], NULL)) {
        fprintf(stderr, "Failed to load ROM file: %s\n", argv[1]);
        return 1;
    }

    LOG("Cartridge successfully loaded");

    if (pthread_create(&thread, NULL, self->cpu_run, self) != 0) {
        HANDLE_ERROR("Failed to create thread");
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(self->loop, self, 0, 1);
#else
    while (!atomic_load(&self->context->die)) {
        loop(self);
    }
#endif

    pthread_join(thread, NULL);

    return 0;
}

static void request(GameboyClass *self, request_t request)
{
    atomic_fetch_or(&self->context->requests, request);
}

static void configure(GameboyClass *self, setting_t setting, uint32_t value)
{
    switch (setting) {
        case SETTING_TURBO: atomic_store(&self->context->turbo, value); return;
        case SETTING_PAUSED: atomic_store(&self->context->paused, value); return;
        case SETTING_TILES: {
            self->ui->panel_visible = value;
#ifndef __EMSCRIPTEN__
            self->ui->resize(self->ui);
#endif
            return;
        }
        case SETTING_PALETTE: {
            atomic_store(&self->context->palette, value % PALETTE_COUNT);
            break;
        }
        case SETTING_COLOR_CORRECTION: {
            atomic_store(&self->context->color_correction, value);
            break;
        }
    }
    self->request(self, REQUEST_REFRESH_COLORS);
}

static void process_requests(GameboyClass *self)
{
    uint32_t requests = atomic_exchange(&self->context->requests, 0);
    char path[1100];

    snprintf(
        path, sizeof(path), "%s.state", self->cartridge->context->save_path);
    if (requests & REQUEST_SAVE_STATE) {
        LOG(self->snapshot->save(self->snapshot, path) ? "State saved"
                                                       : "State not saved");
    }
    if (requests & REQUEST_LOAD_STATE) {
        bool loaded = self->snapshot->load(self->snapshot, path);
        LOG(loaded ? "State loaded" : "State not loaded");
        requests |= loaded ? REQUEST_REFRESH_COLORS : 0;
    }
    if (requests & REQUEST_REFRESH_COLORS) {
        self->lcd->refresh_colors(self->lcd);
    }
}

static void serialize(GameboyClass *self, SnapshotClass *snapshot)
{
    emulator_context_t *ctx = self->context;

    snapshot->field(snapshot, &ctx->ticks, sizeof(ctx->ticks));
    snapshot->field(snapshot, &ctx->double_speed, sizeof(ctx->double_speed));
    snapshot->field(
        snapshot, &ctx->speed_switch_armed, sizeof(ctx->speed_switch_armed));
}

static void cycles(GameboyClass *self, int32_t count)
{
    uint32_t dots = self->context->double_speed ? 2 : 4;

    for (int32_t i = 0; i < count; i++) {
        self->context->ticks += dots;
        self->timer->tick(self->timer);
        if (self->io->serial->bits_remaining) {
            self->io->tick(self->io);
        }
        if (self->dma->context->active) {
            self->dma->tick(self->dma);
        }
        self->ppu->tick(self->ppu, dots);
        self->sound->tick(self->sound, dots);
        if ((self->context->ticks & 0x3FF) < dots) {
            self->joypad->update(self->joypad);
        }
    }
}

const GameboyClass init_gameboy = {
    {
        ._size = sizeof(GameboyClass),
        ._name = "GameBoy",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .run = run,
    .boot = boot,
    .request = request,
    .configure = configure,
    .process_requests = process_requests,
    .serialize = serialize,
    .cycles = cycles,
    .cpu_run = cpu_run,
    .loop = loop,
};

const class_t *Gameboy = (const class_t *) &init_gameboy;
