#include <stdbool.h>
#include <stdio.h>
#include "SDL2/SDL.h"
#include "cartridge.h"
#include "common.h"
#include "cpu.h"
#include "oop.h"
#include "SDL2/SDL_ttf.h"

#ifndef __GAMEBOY
    #define __GAMEBOY

typedef struct gameboy_aux GameboyClass;

typedef struct gameboy_aux {
    /* Properties */
    class_t metadata;
    CartridgeClass *cartridge;
    CPUClass *cpu;
    emulator_context_t *context;
    /* Methods */
    int (*run)(GameboyClass *, int, char **);
} GameboyClass;

extern const class_t *Gameboy;
#endif
