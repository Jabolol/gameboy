#include <stdbool.h>
#include <stdio.h>
#include "SDL2/SDL.h"
#include "bus.h"
#include "cartridge.h"
#include "common.h"
#include "cpu.h"
#include "oop.h"
#include "ram.h"
#include "SDL2/SDL_ttf.h"

#ifndef __GAMEBOY
    #define __GAMEBOY

typedef struct gameboy_aux GameboyClass;

typedef struct gameboy_aux {
    /* Properties */
    class_t metadata;
    CartridgeClass *cartridge;
    CPUClass *cpu;
    BusClass *bus;
    InstructionsClass *instructions;
    RamClass *ram;
    emulator_context_t *context;
    /* Methods */
    int32_t (*run)(GameboyClass *, int32_t, char **);
    void (*cycles)(GameboyClass *, int32_t);
} GameboyClass;

extern const class_t *Gameboy;
#endif
