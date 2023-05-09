#include "common.h"
#include "oop.h"

#ifndef __DEBUG
    #define __DEBUG

typedef struct gameboy_aux GameboyClass;
typedef struct debug_aux DebugClass;

typedef struct debug_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    char message[1024];
    int32_t message_size;
    /* Methods */
    void (*update)(DebugClass *);
    void (*print)(DebugClass *);
} DebugClass;

extern const class_t *Debug;
#endif
