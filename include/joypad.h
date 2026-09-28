#include "common.h"
#include "oop.h"

#ifndef __JOYPAD
    #define __JOYPAD

typedef struct gameboy_aux GameboyClass;
typedef struct joypad_aux JoypadClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct joypad_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    joypad_context_t *context;
    /* Methods */
    void (*choose)(JoypadClass *, uint8_t);
    uint8_t (*output)(JoypadClass *);
    uint8_t (*lines)(JoypadClass *);
    void (*set_button)(JoypadClass *, button_t, bool);
    bool (*pressed)(JoypadClass *, button_t);
    bool (*any_pressed)(JoypadClass *);
    void (*update)(JoypadClass *);
    void (*serialize)(JoypadClass *, SnapshotClass *);
} JoypadClass;

extern const class_t *Joypad;

#endif
