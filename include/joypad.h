#include "common.h"
#include "oop.h"

#ifndef __JOYPAD
    #define __JOYPAD

typedef struct gameboy_aux GameboyClass;
typedef struct joypad_aux JoypadClass;

typedef struct joypad_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    joypad_context_t *context;
    /* Methods */
    void (*choose)(JoypadClass *, uint8_t);
    uint8_t (*output)(JoypadClass *);
} JoypadClass;

extern const class_t *Joypad;

#endif
