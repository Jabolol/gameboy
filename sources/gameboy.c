#include "../include/gameboy.h"

static void constructor(void *ptr, va_list __attribute__((unused)) * args)
{
    GameboyClass *self = (GameboyClass *) ptr;
    self->cartridge = new_class(Cartridge);
    self->cpu = new_class(CPU);
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
}

static void destructor(void *ptr)
{
    GameboyClass *self = (GameboyClass *) ptr;
    destroy_class(self->cartridge);
    destroy_class(self->cpu);
    free(self->context);
}

static int run(GameboyClass *self, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: ./gameboy /path/to/rom.gb\n");
        return 1;
    }
    if (!self->cartridge->load(self->cartridge, argv[1])) {
        fprintf(stderr, "Failed to load ROM file: %s\n", argv[1]);
        return 1;
    }
    printf("Cartridge successfully loaded\n");
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL initialized\n");
    TTF_Init();
    printf("TTF initialized\n");
    self->cpu->init(self->cpu);
    self->context->running = true;
    self->context->paused = false;
    self->context->ticks = 0;

    while (self->context->running) {
        if (self->context->paused) {
            SDL_Delay(10);
            continue;
        }
        if (!self->cpu->step(self->cpu)) {
            printf("CPU stopped\n");
            break;
        }
        self->context->ticks += 1;
    }

    return 0;
}

const GameboyClass init_gameboy = {
    {
        ._size = sizeof(GameboyClass),
        ._name = "GameBoy",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .cartridge = NULL,
    .cpu = NULL,
    .run = run,
};

const class_t *Gameboy = (const class_t *) &init_gameboy;
