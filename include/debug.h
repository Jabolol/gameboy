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
    char buffer[BUFFER_SIZE];
    char instruction_data[INST_BUFF_LEN];
    int32_t message_size;
    /* Methods */
    void (*update)(DebugClass *);
    void (*print)(DebugClass *);
    void (*cpu_step)(DebugClass *, uint16_t);
} DebugClass;

extern const class_t *Debug;
#endif
