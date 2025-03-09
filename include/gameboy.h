#include <pthread.h>
#include <stdbool.h>
#include "bus.h"
#include "cartridge.h"
#include "common.h"
#include "cpu.h"
#include "debug.h"
#include "dma.h"
#include "io.h"
#include "joypad.h"
#include "lcd.h"
#include "oop.h"
#include "pipeline.h"
#include "ppu.h"
#include "ram.h"
#include "sound.h"
#include "stack.h"
#include "timer.h"
#include "ui.h"

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
    RAMClass *ram;
    StackClass *stack;
    UIClass *ui;
    IOClass *io;
    DebugClass *debug;
    TimerClass *timer;
    PPUClass *ppu;
    DMAClass *dma;
    LCDClass *lcd;
    PipelineClass *pipeline;
    JoypadClass *joypad;
    SoundClass *sound;
    emulator_context_t *context;
    /* Methods */
    int32_t (*run)(GameboyClass *, int32_t, char **);
    void (*cycles)(GameboyClass *, int32_t);
    void *(*cpu_run)(void *);
    void (*loop)(void *);
} GameboyClass;

extern const class_t *Gameboy;
#endif
