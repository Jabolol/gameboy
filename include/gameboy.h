#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "SDL2/SDL.h"
#include "bus.h"
#include "cartridge.h"
#include "common.h"
#include "cpu.h"
#include "debug.h"
#include "io.h"
#include "oop.h"
#include "ram.h"
#include "stack.h"
#include "timer.h"
#include "ui.h"
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
    StackClass *stack;
    UiClass *ui;
    IoClass *io;
    DebugClass *debug;
    TimerClass *timer;
    emulator_context_t *context;
    /* Methods */
    int32_t (*run)(GameboyClass *, int32_t, char **);
    void (*cycles)(GameboyClass *, int32_t);
    void *(*cpu_run)(void *);
} GameboyClass;

extern const class_t *Gameboy;
#endif
