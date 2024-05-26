#include "../include/gameboy.h"

static void constructor(void *ptr, va_list UNUSED *args)
{
    GameboyClass *self = (GameboyClass *) ptr;
    self->cartridge = new_class(Cartridge);
    self->ram = new_class(RAM);
    self->instructions = new_class(Instructions);
    self->bus = new_class(Bus, self);
    self->timer = new_class(Timer, self);
    self->cpu = new_class(CPU, self);
    self->stack = new_class(Stack, self);
    self->ui = new_class(UI, self, Y_RES * SCALE, X_RES * SCALE, SCALE);
    self->io = new_class(IO, self);
    self->debug = new_class(Debug, self);
    self->lcd = new_class(LCD, self);
    self->ppu = new_class(PPU, self, FPS);
    self->dma = new_class(DMA, self);
    self->pipeline = new_class(Pipeline, self);
    self->joypad = new_class(Joypad, self);
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
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
    destroy_class(self->ui);
    destroy_class(self->io);
    destroy_class(self->debug);
    destroy_class(self->timer);
    destroy_class(self->pipeline);
    destroy_class(self->ppu);
    destroy_class(self->dma);
    destroy_class(self->lcd);
    destroy_class(self->joypad);
    free(self->context);
}

static void *cpu_run(void *ptr)
{
    GameboyClass *self = (GameboyClass *) ptr;

    self->context->running = true;
    self->context->paused = false;
    self->context->ticks = 0;

    while (self->context->running) {
        if (self->context->die) {
            pthread_exit(NULL);
        }
        if (self->context->paused) {
            self->ui->delay(10);
            continue;
        }
        if (!self->cpu->step(self->cpu)) {
            LOG("CPU stopped");
            break;
        }
    }
    return 0;
}

static int32_t run(GameboyClass *self, int argc, char **argv)
{
    pthread_t thread;

    if (argc < 2) {
        fprintf(stderr, "Usage: ./gameboy /path/to/rom.gb\n");
        return 1;
    }

    if (!self->cartridge->load(self->cartridge, argv[1])) {
        fprintf(stderr, "Failed to load ROM file: %s\n", argv[1]);
        return 1;
    }

    LOG("Cartridge successfully loaded");

    if (pthread_create(&thread, NULL, self->cpu_run, self) != 0) {
        HANDLE_ERROR("Failed to create thread");
    }

    uint32_t prev_frame = 0;

    while (!self->context->die) {
        nanosleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
        self->ui->handle_events(self->ui);

        if (prev_frame != self->ppu->context->current_frame) {
            self->ui->update(self->ui);
        }
        prev_frame = self->ppu->context->current_frame;
    }

    pthread_join(thread, NULL);

    return 0;
}

static void cycles(GameboyClass *self, int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        for (int32_t n = 0; n < 4; n++) {
            self->context->ticks += 1;
            self->timer->tick(self->timer);
            self->ppu->tick(self->ppu);
        }
        self->dma->tick(self->dma);
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
    .cycles = cycles,
    .cpu_run = cpu_run,
};

const class_t *Gameboy = (const class_t *) &init_gameboy;
